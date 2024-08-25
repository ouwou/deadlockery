using DeadlockAPI;
using ouwou.GC.Deadlock.Internal;
using SharpCompress.Compressors.BZip2;
using SharpCompress.Readers;
using SteamKit2;

namespace deadlockery
{
    class Program
    {
        static DeadlockClient client;
        static uint testMatchId;
        static bool isRunning = true;

        public static void Main(string[] args)
        {
            if (args.Length < 3)
            {
                Console.WriteLine($"Usage: {AppDomain.CurrentDomain.FriendlyName} <steam username> <steam password> <match id>");
                return;
            }

            client = new DeadlockClient(args[0], args[1]);
            uint.TryParse(args[2], out testMatchId);

            client.ClientWelcomeEvent += OnWelcome;

            client.Connect();

            while (isRunning)
            {
                client.RunCallbacks(TimeSpan.FromSeconds(1));
            }

            client.Disconnect();
        }

        private static async void GetMatchMetaDataExample(uint matchId)
        {
            // request basic match metadata info
            var metadata = await client.GetMatchMetaData(matchId);
            if (metadata != null)
            {
                Console.WriteLine($"Demo URL: {metadata.ReplayURL}\nMeta URL: ${metadata.MetadataURL}\n");

                byte[] matchMetaDataBytes;

                // fetch compressed metadata file from valve servers
                using (var client = new HttpClient())
                {
                    using (var outStream = new MemoryStream())
                    using (var inStream = await client.GetStreamAsync(metadata.MetadataURL))
                    using (var bz2 = new BZip2Stream(inStream, SharpCompress.Compressors.CompressionMode.Decompress, false))
                    {
                        bz2.CopyTo(outStream);
                        matchMetaDataBytes = outStream.ToArray();
                    }
                }

                // decode base metadata object
                var matchMetaData = ProtoBuf.Serializer.Deserialize<CMsgMatchMetaData>(new ReadOnlySpan<byte>(matchMetaDataBytes));
                // decode detailed metadata
                var matchMetaDataContents = ProtoBuf.Serializer.Deserialize<CMsgMatchMetaDataContents>(new ReadOnlySpan<byte>(matchMetaData.match_details));
                // spit out some of it
                foreach (var player in matchMetaDataContents.match_info.players)
                {
                    var steamId = new SteamID(player.account_id, EUniverse.Public, EAccountType.Individual);
                    var steamUrl = $"https://steamcommunity.com/profiles/{steamId.ConvertToUInt64()}";
                    Console.WriteLine($"Player {player.player_slot}:");
                    Console.WriteLine($"    URL: {steamUrl}");
                    Console.WriteLine($"    Hero: {(DeadlockAPI.Enums.Heroes)player.hero_id}");
                    Console.WriteLine($"    Won?: {(player.team == matchMetaDataContents.match_info.winning_team ? "Yes" : "No")}");
                    Console.WriteLine($"    Total Souls: {player.net_worth}");
                }
            }

            isRunning = false;
        }

        private static async void OnWelcome(object? sender, DeadlockClient.ClientWelcomeEventArgs e)
        {
            GetMatchMetaDataExample(testMatchId);
        }
    }
}
