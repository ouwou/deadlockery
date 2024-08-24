using ouwou.GC.Deadlock.Internal;

namespace deadlockery
{
    class Program
    {
        public static DeadlockClient client;
        public static uint testMatchId;

        public static void Main(string[] args)
        {
            if (args.Length < 2)
            {
                Console.WriteLine("Usage: deadlockery.exe <username> <password> <match_id>");
                return;
            }

            client = new DeadlockClient(args[0], args[1]);
            testMatchId = uint.Parse(args[2]);

            client.ClientWelcomeEvent += OnWelcome;
            client.GetMatchMetaDataResponseEvent += OnGetMatchMetaDataResponse;

            client.Connect();
            client.Wait();
        }

        private static void OnGetMatchMetaDataResponse(object? sender, DeadlockClient.GetMatchMetaDataResponseEventArgs e)
        {
            if (e.Data.result != CMsgClientToGCGetMatchMetaDataResponse.EResult.k_eResult_Success)
            {
                Console.WriteLine($"Request failed: {e.Data.result}");
                return;
            }

            Console.WriteLine($"Demo file URL: http://replay{e.Data.cluster_id}.valve.net/1422450/{testMatchId}_{e.Data.replay_salt}.dem.bz2");
            Console.WriteLine($"Metadata file URL: http://replay{e.Data.cluster_id}.valve.net/1422450/{testMatchId}_{e.Data.metadata_salt}.meta.bz2");
        }

        private static void OnWelcome(object? sender, DeadlockClient.ClientWelcomeEventArgs e)
        {
            client.GetMatchMetaData(testMatchId);
        }
    }
}
