#pragma once

#include <cstdio>
#include <cstdint>
#include <vector>

class BaseSource {
public:
    virtual ~BaseSource() = default;
    virtual size_t length() = 0;
    virtual void seek(int offset, int origin) = 0;
    virtual long tell() = 0;
    virtual void close() = 0;
    virtual void read(void *data, size_t count) = 0;
};

class MemorySource : public BaseSource {
public:
    MemorySource(const void *data, size_t size) {
        m_data = data;
        m_size = size;
    }

    size_t length() override {
        return m_size;
    }

    void seek(int offset, int origin) override {
        switch (origin) {
            case SEEK_SET: m_offset = offset;
                break;
            case SEEK_CUR: m_offset += offset;
                break;
            case SEEK_END: m_offset = m_size + offset;
                break;
        }
    }

    long tell() override {
        return m_offset;
    }

    void read(void *data, size_t count) override {
        memcpy(data, static_cast<const char *>(m_data) + m_offset, count);
        m_offset += count;
    }

    void close() override {}

private:
    const void *m_data;
    size_t m_size;
    size_t m_offset = 0;
};

class FileSource : public BaseSource {
public:
    FileSource(const char *filename) {
        m_fp = fopen(filename, "rb");
    }

    virtual ~FileSource() {
        close();
    }

    size_t length() override {
        fseek(m_fp, 0, SEEK_END);
        size_t size = ftell(m_fp);
        fseek(m_fp, 0, SEEK_SET);
        return size;
    }

    void close() override {
        if (m_fp) fclose(m_fp);
        m_fp = nullptr;
    }

    void seek(int offset, int origin) override {
        fseek(m_fp, offset, origin);
    }

    long tell() override {
        return ftell(m_fp);
    }

    void read(void *data, size_t count) override {
        fread(data, 1, count, m_fp);
    }

private:
    FILE *m_fp;
};

class BinaryReader {
public:
    BinaryReader(BaseSource *src) {
        m_src = src;
        m_size = m_src->length();
    }

    ~BinaryReader() {
        m_src->close();
    }

    [[nodiscard]] long length() const {
        return m_size;
    }

    bool readBit() {
        if (m_bitIndex == 0) {
            m_src->read(&m_byte, 1);
        }
        int bit = (m_byte >> m_bitIndex) & 1;
        m_bitIndex = (m_bitIndex + 1) % 8;
        return bit;
    }

    uint8_t readByte() {
        if (m_bitIndex == 0) {
            m_src->read(&m_byte, 1);
            return m_byte;
        } else {
            uint8_t byte = 0;
            for (int i = 0; i < 8; i++) {
                byte |= readBit() << i;
            }
            return byte;
        }
    }

    uint64_t readUint64() {
        uint64_t value = readByte();
        value |= static_cast<uint64_t>(readByte()) << 8;
        value |= static_cast<uint64_t>(readByte()) << 16;
        value |= static_cast<uint64_t>(readByte()) << 24;
        value |= static_cast<uint64_t>(readByte()) << 32;
        value |= static_cast<uint64_t>(readByte()) << 40;
        value |= static_cast<uint64_t>(readByte()) << 48;
        value |= static_cast<uint64_t>(readByte()) << 56;
        return value;
    }

    uint32_t readVarUint32() {
        uint32_t value = 0;
        int shift = 0;
        uint8_t byte;
        do {
            byte = readByte();
            value |= (byte & 0x7f) << shift;
            shift += 7;
        } while ((byte & 0x80) != 0);
        return value;
    }

    int32_t readVarInt32() {
        int32_t value = 0;
        int shift = 0;
        uint8_t byte;
        do {
            byte = readByte();
            value |= (byte & 0x7f) << shift;
            shift += 7;
        } while ((byte & 0x80) != 0);
        return value;
    }


    uint32_t readBits(int bits) {
        uint32_t value = 0;
        for (int i = 0; i < bits; i++) {
            value |= readBit() << i;
        }
        return value;
    }

    void readBits(uint8_t *buf, int bits) {
        // read bit-by-bit into buf bitwise
        size_t value = 0;
        size_t offset = 0;
        for (int i = 0; i < bits; i++) {
            value |= readBit() << 1;
            if (i > 0 && i % 8 == 0) {
                buf[offset] = value;
                offset++;
            }
        }
        buf[offset] = value;
    }

    void readBytes(uint8_t *buf, int bytes) {
        for (int i = 0; i < bytes; i++) {
            buf[i] = readByte();
        }
    }

    uint32_t readUBitVar() {
        uint32_t index = readBits(6);
        uint32_t flag = index & 0x30;
        if (flag == 16) {
            index = (index & 15) | (readBits(4) << 4);
        } else if (flag == 32) {
            index = (index & 15) | (readBits(8) << 4);
        } else if (flag == 48) {
            index = (index & 15) | (readBits(28) << 4);
        }
        return index;
    }

    std::vector<uint8_t> read(int bytes) {
        std::vector<uint8_t> data(bytes);
        if (m_bitIndex == 0) {
            m_src->read(data.data(), bytes);
        } else {
            for (int i = 0; i < bytes; i++) {
                data[i] = readByte();
            }
        }
        return data;
    }

    std::string readString() {
        std::string r;
        while (true) {
            uint8_t c = readByte();
            if (c == 0) break;
            r += static_cast<char>(c);
        }
        return r;
    }

    bool more() {
        return m_src->tell() < m_size;
    }

    void skip(int bytes) {
        if (m_bitIndex == 0) {
            m_src->seek(bytes, SEEK_CUR);
        } else {
            for (int i = 0; i < bytes * 8; i++) {
                readBit();
            }
        }
    }

private:
    BaseSource *m_src;
    long m_size;
    uint8_t m_byte;
    int m_bitIndex = 0;
};