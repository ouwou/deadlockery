using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using SteamDatabase.ValvePak;
using System.Diagnostics;
using ValveResourceFormat;
using ValveResourceFormat.ResourceTypes;
using ValveResourceFormat.Serialization.KeyValues;

namespace kv3bs {
    class KVValueConverter : JsonConverter<KVValue> {
        public override KVValue? ReadJson(JsonReader reader, Type objectType, KVValue? existingValue, bool hasExistingValue, JsonSerializer serializer) {
            throw new NotImplementedException();
        }

        public override void WriteJson(JsonWriter writer, KVValue? value, JsonSerializer serializer) {
            if (value == null) {
                serializer.Serialize(writer, null);
                return;
            }

            serializer.Serialize(writer, value.Value);
        }
    }

    class KVObjectConverter : JsonConverter<KVObject> {
        public override KVObject? ReadJson(JsonReader reader, Type objectType, KVObject? existingValue, bool hasExistingValue, JsonSerializer serializer) {
            throw new NotImplementedException();
        }

        public override void WriteJson(JsonWriter writer, KVObject? value, JsonSerializer serializer) {
            if (value == null) {
                serializer.Serialize(writer, null);
                return;
            }

            serializer.Serialize(writer, value.Properties);
        }
    }

    class BinaryKV3Converter : JsonConverter<BinaryKV3> {
        public override BinaryKV3? ReadJson(JsonReader reader, Type objectType, BinaryKV3? existingValue, bool hasExistingValue, JsonSerializer serializer) {
            throw new NotImplementedException();
        }

        public override void WriteJson(JsonWriter writer, BinaryKV3? kv3, JsonSerializer serializer) {
            if (kv3 == null) {
                serializer.Serialize(writer, null);
                return;
            }

            JObject obj = new JObject
            {
                { "Root", JToken.FromObject(kv3.Data, serializer) },
            };
            obj.WriteTo(writer);
        }
    }
    internal class Program {
        static void Main(string[] args) {
            using var package = new Package();

            package.Read("C:\\Program Files (x86)\\Steam\\steamapps\\common\\Deadlock\\game\\citadel\\pak01_dir.vpk");
            foreach (var entry in package.Entries["vdata_c"]) {
                HandleVDataEntry(package, entry);
            }

        }

        private static void HandleVDataEntry(Package package, PackageEntry entry) {
            package.ReadEntry(entry, out var rawFile);
            using var ms = new MemoryStream(rawFile);
            using var resource = new Resource();
            resource.Read(ms);
            Debug.Assert(resource.ResourceType == ResourceType.VData);
            var kv3 = (BinaryKV3)resource.DataBlock;

            JsonSerializerSettings serializerSettings = new JsonSerializerSettings() {
                Converters = new List<JsonConverter>
               {
                   new BinaryKV3Converter(),
                   new KVObjectConverter(),
                   new KVValueConverter()
               }
            };

            string json = JsonConvert.SerializeObject(kv3, Formatting.Indented, serializerSettings);
            File.WriteAllText($"{entry.FileName}.vdata.json", json);
        }
    }
}
