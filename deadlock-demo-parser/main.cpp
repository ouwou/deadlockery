#include <fstream>
#include <iostream>

#include "reader.hh"

#include <snappy.h>

#include "generated/base_modifier.pb.h"
#include "generated/citadel_clientmessages.pb.h"
#include "generated/citadel_usermessages.pb.h"
#include "generated/demo.pb.h"
#include "generated/netmessages.pb.h"

void HandleFileHeader(const std::vector<uint8_t> &buf) {
    CDemoFileHeader header;
    header.ParseFromArray(buf.data(), buf.size());
    std::cout << "Header:\n" << header.DebugString() << std::endl;
}

struct StringTableData {
    std::string Name;
    int MaxEntries;
    int UserDataSize;
    int UserDataSizeBits;
    int UserDataFixedSize;
    int Flags;
    bool UsingVarintBitcounts;
};

std::unordered_map<size_t, StringTableData> g_StringTables;

void ParseStringTableUpdate(BinaryReader &buf, int32_t num_entries, int32_t user_data_size, int32_t user_data_size_bits,
                            bool user_data_fixed_size, int32_t flags, bool using_varint_bitcounts) {
    int idx = -1;
    std::vector<std::string> keys;
    struct StringTableEntry {
        int Index;
        std::string Key;
        std::vector<uint8_t> Value;
    };
    std::vector<StringTableEntry> items;

    for (int i = 0; i < num_entries; i++) {
        std::string key;
        std::vector<uint8_t> value;

        if (buf.readBit()) {
            idx += 1;
        } else {
            idx += buf.readVarInt32() + 1;
        }
        if (buf.readBit()) {
            if (buf.readBit()) {
                int position = buf.readBits(5);
                int length = buf.readBits(5);
                if (position >= keys.size()) {
                    key += buf.readString();
                } else {
                    if (position < keys.size()) {
                        auto &s = keys.at(position);
                        if (length > s.size()) {
                            key += buf.readString();
                        } else {
                            key += s.substr(0, length) + buf.readString();
                        }
                    }
                }
            } else {
                key += buf.readString();
            }

            if (keys.size() >= 32) {
                keys.erase(keys.begin());
            }
            keys.push_back(key);
            if (buf.readBit()) {
                uint32_t bits;
                bool is_compressed = false;
                if (user_data_fixed_size) {
                    bits = user_data_size;
                } else {
                    if (flags & 1) {
                        is_compressed = buf.readBit();
                    }
                    if (using_varint_bitcounts) {
                        bits = buf.readUBitVar() * 8;
                    } else {
                        bits = buf.readBits(17) * 8;
                    }
                }
                value.resize(bits / 8);
                buf.readBytes(value.data(), bits / 8);
                if (is_compressed) {
                    size_t result;
                    snappy::GetUncompressedLength((const char *)value.data(), value.size(), &result);
                    std::vector<uint8_t> uncompressed(result);
                    snappy::RawUncompress((const char *)value.data(), value.size(), (char *)uncompressed.data());
                    value.swap(uncompressed);
                }
            }
            items.push_back({ idx, key, value });
        }
    }

#if 0
    for (const auto &item : items) {
        if (item.Value.size() > 0) {
            CModifierTableEntry test;
            if (test.ParseFromArray(item.Value.data(), item.Value.size())) {
                unsigned subclass = test.modifier_subclass();
                const auto key = std::to_string(subclass);
            } else {}
        }
    }
#endif
}

std::unordered_map<int, std::string> g_eventNames;

void HandlePacketData(const std::string &data) {
    MemorySource src(data.data(), data.size());
    BinaryReader reader(&src);
    while (reader.more()) {
        int32_t cmd = reader.readUBitVar();
        uint32_t size = reader.readVarUint32();
        std::vector<uint8_t> buf = reader.read(size);

        switch (cmd) {
            case svc_ServerInfo: {
                CSVCMsg_ServerInfo msg;
                msg.ParseFromArray(buf.data(), buf.size());
            }
            break;
            case svc_CreateStringTable: {
                CSVCMsg_CreateStringTable msg;
                if (msg.ParseFromArray(buf.data(), buf.size())) {
                    printf("CreateStringTable: %s:%d:%d:%d:%d\n", msg.name().c_str(), msg.num_entries(), msg.user_data_size(), msg.user_data_size_bits(), msg.flags());
                    size_t idx = g_StringTables.size();
                    g_StringTables[idx] = { msg.name(), msg.num_entries(), msg.user_data_size(), msg.user_data_size_bits(), msg.user_data_fixed_size(), msg.flags(), msg.using_varint_bitcounts() };
                    if (true || msg.name() == "ActiveModifiers") {
                        if (msg.data_compressed()) {
                            // printf("Compressed\n");
                            std::vector<uint8_t> uncompressed(msg.uncompressed_size());
                            snappy::RawUncompress(msg.string_data().data(), msg.string_data().size(), (char *)uncompressed.data());
                            msg.set_string_data(std::string(uncompressed.begin(), uncompressed.end()));
                        } else {
                            // printf("Uncompressed\n");
                        }
                        MemorySource src(msg.string_data().data(), msg.string_data().size());
                        BinaryReader reader(&src);
                        ParseStringTableUpdate(reader, msg.num_entries(), msg.user_data_size(), msg.user_data_size_bits(), msg.user_data_fixed_size(), msg.flags(), msg.using_varint_bitcounts());
                    }
                }
            }
            break;
            case svc_UpdateStringTable: {
                CSVCMsg_UpdateStringTable msg;
                if (msg.ParseFromArray(buf.data(), buf.size())) {
                    if (msg.table_id() < g_StringTables.size() && g_StringTables[msg.table_id()].MaxEntries > msg.num_changed_entries()) {
                        auto &table = g_StringTables[msg.table_id()];
                        //if (table.Name == "ActiveModifiers") {
                        std::string data = msg.string_data();
                        // printf("data %llu\n", data.size());
                        // printf("\n");
                        printf("UpdateStringTable:%d(%s):%d\n", msg.table_id(), table.Name.c_str(), msg.num_changed_entries());
                        MemorySource src(msg.string_data().data(), msg.string_data().size());
                        BinaryReader reader(&src);
                        static int dbg = 0;
                        dbg++;
                        std::cout << dbg << "\n";
                        ParseStringTableUpdate(reader, msg.num_changed_entries(), table.UserDataSize, table.UserDataSizeBits, table.UserDataFixedSize, table.Flags, table.UsingVarintBitcounts);
                        //}
                    }
                }
            }
            break;
            case GE_Source1LegacyGameEventList: {
                CMsgSource1LegacyGameEventList msg;
                msg.ParseFromArray(buf.data(), buf.size());
                for (const auto &descriptor : msg.descriptors()) {
                    g_eventNames[descriptor.eventid()] = descriptor.name();
                }
            }
            break;
            case GE_Source1LegacyGameEvent: {
                CMsgSource1LegacyGameEvent msg;
                if (msg.ParseFromArray(buf.data(), buf.size())) {
                    const auto event_name_it = g_eventNames.find(msg.eventid());
                    if (event_name_it != g_eventNames.end()) {
                        std::string event_name = event_name_it->second;
                    }
                } else {
                    // printf("CMsgSource1LegacyGameEvent failed to parse\n");
                }
            }
            break;
                break;
            default: break;
        }
    }
}

void HandlePacket(const std::vector<uint8_t> &buf) {
    CDemoPacket packet;
    packet.ParseFromArray(buf.data(), buf.size());
    HandlePacketData(packet.data());
}

void HandleSignonPacket(const std::vector<uint8_t> &buf) {
    CDemoPacket packet;
    packet.ParseFromArray(buf.data(), buf.size());
    HandlePacketData(packet.data());
}

void HandleClassInfoPacket(const std::vector<uint8_t> &buf) {
    CDemoClassInfo msg;
    msg.ParseFromArray(buf.data(), buf.size());
    for (const auto &x : msg.classes()) {
        //x.PrintDebugString();
    }
}

void HandleStringTablesPacket(const std::vector<uint8_t> &buf) {
    CDemoStringTables msg;
    msg.ParseFromArray(buf.data(), buf.size());
    // msg.PrintDebugString();
}

int main() {
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    FileSource src("2394994.dem");
    BinaryReader reader(&src);

    uint64_t magic = reader.readUint64();
    if (magic != 0x32534d45444250) {
        printf("Not a demo\n");
        return 1;
    }

    reader.skip(8);

    while (reader.more()) {
        int32_t cmd = reader.readVarInt32();
        int32_t tick = reader.readVarInt32();

        bool compressed = (cmd & DEM_IsCompressed) != 0;
        cmd &= ~DEM_IsCompressed;

        int32_t size = reader.readVarInt32();
        auto buf = reader.read(size);
        if (compressed) {
            size_t uncompressed_len;
            snappy::GetUncompressedLength(reinterpret_cast<const char *>(buf.data()), buf.size(), &uncompressed_len);
            std::vector<uint8_t> uncompressed_buf(uncompressed_len);
            snappy::RawUncompress(reinterpret_cast<const char *>(buf.data()), buf.size(), reinterpret_cast<char *>(uncompressed_buf.data()));
            buf.swap(uncompressed_buf);
        }

        // std::cout << "CMD: " << cmd << "\n";

        switch (cmd) {
            case DEM_FileHeader: HandleFileHeader(buf);
                break;
            case DEM_Packet: HandlePacket(buf);
                break;
            case DEM_SignonPacket: HandleSignonPacket(buf);
                break;
            case DEM_StringTables: HandleStringTablesPacket(buf);
                break;
            case DEM_ClassInfo: HandleClassInfoPacket(buf);
                break;
            case DEM_SendTables: break;
            default: break;
        }
    }

    return 0;
}