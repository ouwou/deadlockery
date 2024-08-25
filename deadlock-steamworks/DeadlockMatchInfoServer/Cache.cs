using System.Data.SQLite;

namespace DeadlockMatchInfoServer
{
    internal class Cache
    {
        private SQLiteConnection connection;

        public Cache(string filename)
        {
            connection = new SQLiteConnection($"Data Source={filename}");
            InitializeTables();
        }

        private void InitializeTables()
        {

        }
    }
}
