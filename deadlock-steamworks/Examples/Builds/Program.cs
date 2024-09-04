using DeadlockAPI;
using DeadlockAPI.Enums;
using System.Text.Json;

namespace Builds {
    class Program {
        static DeadlockClient client;
        static bool isRunning = true;
        static Dictionary<string, string> itemMapping = new();

        static void Main(string[] args) {
            if (args.Length < 2) {
                Console.WriteLine($"Usage: {AppDomain.CurrentDomain.FriendlyName} <steam username> <steam password>");
                return;
            }

            if (File.Exists("mapping.json")) {
                itemMapping = JsonSerializer.Deserialize<Dictionary<string, string>>(File.ReadAllText("mapping.json"));
            } else {
                Console.WriteLine("Item mapping doesn't exist. Item names will not display.\n");
            }

            client = new DeadlockClient(args[0], args[1]);

            client.ClientWelcomeEvent += OnWelcome;

            client.Connect();

            while (isRunning) {
                client.RunCallbacks(TimeSpan.FromSeconds(1));
            }

            client.Disconnect();
        }

        private static async void OnWelcome(object? sender, DeadlockClient.ClientWelcomeEventArgs e) {
            isRunning = false;

            var builds = await client.FindHeroBuilds(Heroes.Ivy);
            if (builds == null || builds.response != ouwou.GC.Deadlock.Internal.CMsgClientToGCFindHeroBuildsResponse.EResponse.k_eSuccess) {
                Console.WriteLine("Error finding builds");
                return;
            }

            foreach (var build in builds.results.OrderByDescending(x => x.num_favorites)) {
                Console.WriteLine($"{build.hero_build.name} - {build.num_favorites} ♥");
                Console.WriteLine(build.hero_build.description);
                foreach (var category in build.hero_build.details.mod_categories) {
                    Console.WriteLine($"  {category.name} - {category.description}");
                    foreach (var mod in category.mods) {
                        var name = itemMapping.GetValueOrDefault(mod.ability_id.ToString(), mod.ability_id.ToString());
                        Console.WriteLine($"    - {name}");
                    }

                    Console.WriteLine();
                }
            }
        }
    }
}
