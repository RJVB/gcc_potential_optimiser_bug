#include <iostream>
#include <string>

#include "lmdb+++.h"

static size_t LZ4_sizeofState()
{
	return 16 * 1024;
}

static std::string lmdbxx_exception_handler(const lmdb::error &e, const std::string &operation)
{
    return "LMDB exception in \"" + operation + "\": " + e.what();
}

class LMDBHook
{
public:
    LMDBHook()
    {
	 std::cerr << __PRETTY_FUNCTION__ << " s_lmdbEnv=" << &s_lmdbEnv << "\n";
    }

    ~LMDBHook()
    {
	 std::cerr << __PRETTY_FUNCTION__ << "\n";
      if (s_envExists) {
          s_lmdbEnv.close();
          s_envExists = false;
          delete[] s_lz4CompState;
      }
    }

    static bool init()
    {
        if (!s_envExists) {
            try {
                s_lmdbEnv = lmdb::env::create();
                std::cerr << __PRETTY_FUNCTION__ << " s_lmdbEnv=" << s_lmdbEnv << "\n";
                s_lmdbEnv.open("/tmp/kk.lmdb", MDB_NOSYNC);
                MDB_envinfo stat;
                lmdb::env_info(s_lmdbEnv.handle(), &stat);
                if (stat.me_mapsize > s_mapSize) {
                    s_mapSize = stat.me_mapsize;
                }
                s_lmdbEnv.set_mapsize(s_mapSize);
                s_lz4CompState = new char[LZ4_sizeofState()];
                s_envExists = true;
                std::cerr << "mapsize=" << stat.me_mapsize << " LZ4 state buffer:" << LZ4_sizeofState() << "\n";
            } catch (const lmdb::error &e) {
                std::cerr << lmdbxx_exception_handler(e, "database creation") << "\n";
                // as per the documentation: the environment must be closed even if creation failed!
                s_lmdbEnv.close();
            }
        }
        return false;
    }

    inline lmdb::env* instance()
    {
        return s_envExists? &s_lmdbEnv : nullptr;
    }
    inline MDB_env* handle()
    {
        return s_envExists? s_lmdbEnv.handle() : nullptr;
    }

    void growMapSize()
    {
        s_mapSize *= 2;
        s_lmdbEnv.set_mapsize(s_mapSize);
    }


    static lmdb::env s_lmdbEnv;
    static bool s_envExists;
    static char* s_lz4CompState;
    static size_t s_mapSize;
};

#ifndef CREATE_GLOBAL_AFTER_STATICS
static LMDBHook LMDB;
#endif

lmdb::env LMDBHook::s_lmdbEnv{nullptr};
bool LMDBHook::s_envExists = false;
char *LMDBHook::s_lz4CompState = nullptr;
// set the initial map size to 64Mb
size_t LMDBHook::s_mapSize = 1024UL * 1024UL * 64UL;

#ifdef CREATE_GLOBAL_AFTER_STATICS
static LMDBHook LMDB;
#endif

int main( int argc, const char *argv[])
{
	if (!LMDB.instance()) {
		LMDB.init();
	}
	std::cerr << "LMDB instance is " << LMDB.instance() << "\n";
	return 0;
}
