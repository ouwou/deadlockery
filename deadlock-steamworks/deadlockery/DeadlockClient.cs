using ouwou.GC.Deadlock.Internal;
using SteamKit2;
using SteamKit2.Authentication;
using SteamKit2.GC;
using SteamKit2.Internal;

namespace deadlockery
{

    class DeadlockClient
    {
        SteamClient client;

        SteamUser user;
        SteamGameCoordinator gameCoordinator;

        CallbackManager callbackMgr;

        string userName;
        string password;
        string previouslyStoredGuardData;

        const int APPID = 1422450;

        public DeadlockClient(string userName, string password)
        {
            this.userName = userName;
            this.password = password;

            client = new SteamClient();

            user = client.GetHandler<SteamUser>();
            gameCoordinator = client.GetHandler<SteamGameCoordinator>();

            callbackMgr = new CallbackManager(client);
            callbackMgr.Subscribe<SteamClient.ConnectedCallback>(OnConnected);
            callbackMgr.Subscribe<SteamClient.DisconnectedCallback>(OnDisconnected);
            callbackMgr.Subscribe<SteamUser.LoggedOnCallback>(OnLoggedOn);
            callbackMgr.Subscribe<SteamGameCoordinator.MessageCallback>(OnGCMessage);

            previouslyStoredGuardData = File.ReadAllText("guard.txt");
        }

        public void Connect()
        {
            Console.WriteLine("Connecting to Steam");
            client.Connect();
        }

        public void Wait()
        {
            while (true)
            {
                callbackMgr.RunWaitCallbacks(TimeSpan.FromSeconds(1));
            }
        }

        async void OnConnected(SteamClient.ConnectedCallback callback)
        {
            Console.WriteLine("Connected! Logging into Steam as {0}", userName);

            var authSession = await client.Authentication.BeginAuthSessionViaCredentialsAsync(new AuthSessionDetails
            {
                Username = userName,
                Password = password,
                IsPersistentSession = true,
                GuardData = previouslyStoredGuardData,
                Authenticator = new UserConsoleAuthenticator(),
            });
            var pollResponse = await authSession.PollingWaitForResultAsync();
            if (pollResponse.NewGuardData != null)
            {
                previouslyStoredGuardData = pollResponse.NewGuardData;
                File.WriteAllText("guard.txt", previouslyStoredGuardData);
            }
            user.LogOn(new SteamUser.LogOnDetails
            {
                Username = pollResponse.AccountName,
                AccessToken = pollResponse.RefreshToken,
                ShouldRememberPassword = true,
            });
        }

        void OnDisconnected(SteamClient.DisconnectedCallback callback)
        {
            Console.WriteLine("Disconnected :(\nTrying again in 10s");
            Thread.Sleep(10000);
            Connect();
        }

        void OnLoggedOn(SteamUser.LoggedOnCallback callback)
        {
            if (callback.Result != EResult.OK)
            {
                Console.WriteLine("Unable to log on to Steam: {0}", callback.Result);
                return;
            }

            Console.WriteLine("Logged in! Launching Deadlock");

            var playGame = new ClientMsgProtobuf<CMsgClientGamesPlayed>(EMsg.ClientGamesPlayed);
            playGame.Body.games_played.Add(new CMsgClientGamesPlayed.GamePlayed
            {
                game_id = new GameID(APPID)
            });

            client.Send(playGame);

            Thread.Sleep(5000);

            var clientHello = new ClientGCMsgProtobuf<CMsgCitadelClientHello>((uint)EGCBaseClientMsg.k_EMsgGCClientHello);
            clientHello.Body.region_mode = ECitadelRegionMode.k_ECitadelRegionMode_ROW;
            gameCoordinator.Send(clientHello, APPID);
        }

        void OnGCMessage(SteamGameCoordinator.MessageCallback callback)
        {
            // Console.WriteLine("{0}", callback.EMsg);

            var messageMap = new Dictionary<uint, Action<IPacketGCMsg>>
            {
                { (uint) EGCBaseClientMsg.k_EMsgGCClientWelcome, OnClientWelcome },
                { (uint) EGCCitadelClientMessages.k_EMsgGCToClientDevPlaytestStatus, OnDevPlaytestStatus },
                { (uint) EGCCitadelClientMessages.k_EMsgClientToGCGetMatchMetaDataResponse, OnGetMatchMetaDataResponse },
            };

            Action<IPacketGCMsg> func;
            if (!messageMap.TryGetValue(callback.EMsg, out func))
            {
                return;
            }

            func(callback.Message);
        }

        public void GetMatchMetaData(uint matchId)
        {
            var msg = new ClientGCMsgProtobuf<CMsgClientToGCGetMatchMetaData>((uint)EGCCitadelClientMessages.k_EMsgClientToGCGetMatchMetaData);
            msg.Body.match_id = matchId;
            gameCoordinator.Send(msg, APPID);
        }

        public class ClientWelcomeEventArgs : EventArgs
        {
            public required CMsgClientWelcome Data;
        }
        public event EventHandler<ClientWelcomeEventArgs> ClientWelcomeEvent;
        void OnClientWelcome(IPacketGCMsg packetMsg)
        {
            var msg = new ClientGCMsgProtobuf<CMsgClientWelcome>(packetMsg);
            ClientWelcomeEvent?.Invoke(this, new ClientWelcomeEventArgs() { Data = msg.Body });
        }

        public class GetMatchMetaDataResponseEventArgs : EventArgs
        {
            public required CMsgClientToGCGetMatchMetaDataResponse Data;
        }
        public event EventHandler<GetMatchMetaDataResponseEventArgs> GetMatchMetaDataResponseEvent;
        void OnGetMatchMetaDataResponse(IPacketGCMsg packetMsg)
        {
            var msg = new ClientGCMsgProtobuf<CMsgClientToGCGetMatchMetaDataResponse>(packetMsg);
            GetMatchMetaDataResponseEvent?.Invoke(this, new GetMatchMetaDataResponseEventArgs() { Data = msg.Body });
        }

        public class DevPlaytestStatusEventArgs : EventArgs
        {
            public required CMsgGCToClientDevPlaytestStatus Data;
        }
        public event EventHandler<DevPlaytestStatusEventArgs> DevPlaytestStatusEvent;
        void OnDevPlaytestStatus(IPacketGCMsg packetMsg)
        {
            var msg = new ClientGCMsgProtobuf<CMsgGCToClientDevPlaytestStatus>(packetMsg);
            DevPlaytestStatusEvent?.Invoke(this, new DevPlaytestStatusEventArgs() { Data = msg.Body });
        }
    }
}
