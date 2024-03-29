A small demonstrator of what appears to be a bug in GCC's optimiser that I can reproduce with g++ 7.2.0, 9.4.0, 12.3.0 and 13.2.0 .

`make [CXX=C++ compiler] [INCLUDES=...] [LIBS=...] [OPTS=...] [DEFS=-DCREATE_GLOBAL_AFTER_STATICS]`

(OPTS defaults to -O3)

The glitch occurs in this function from [`lmdb+++.h`](https://github.com/RJVB/gcc_potential_optimiser_bug/blob/db395beb63831fc2e977d5fc49dbdfcc1459410f/lmdb%2B%2B%2B.h#L1184) :

```
 /**
   * Closes this environment, releasing the memory map.
   *
   * @note this method is idempotent
   * @post `handle() == nullptr`
   */   
  void close() noexcept {
    std::cerr << __PRETTY_FUNCTION__ << " this=" << this << " handle=" << handle() << "\n";
    if (handle()) {
      lmdb::env_close(handle());
      _handle = nullptr;
//    std::cerr << " handle now " << handle();
    }
//     std::cerr << "\n";
  }
```

Unless I outcomment the 2 tracing expressions the `_handle = nullptr;` assignment appears to get optimised away by GCC but not by clang.

Here, `lmdb+++.h` is the header from https://github.com/bendiken/lmdbxx with just some tracing output added for this demonstrator.

In on my systems:

```
> make -B INCLUDES=/opt/local/include LIBS=/opt/local/lib && lmdbhook 
g++ -std=c++11 -O3  -o lmdbhook -I/opt/local/include -L/opt/local/lib -Wl,-rpath,/opt/local/lib lmdbhook.cpp -llmdb
LMDBHook::LMDBHook() s_lmdbEnv=0x406298
lmdb::env::env(MDB_env*) this=0x406298 handle=0
lmdb::env::env(MDB_env*) this=0x7ffe6a341f58 handle=0x10d8020
lmdb::env::~env() this=0x7ffe6a341f58
void lmdb::env::close() this=0x7ffe6a341f58 handle=0
static bool LMDBHook::init() s_lmdbEnv=0x10d8020
mapsize=1048576 LZ4 state buffer:16384
LMDB instance is 0x406298
lmdb::env::~env() this=0x406298
void lmdb::env::close() this=0x406298 handle=0x10d8020
void lmdb::env_close(MDB_env*) env=0x10d8020
LMDBHook::~LMDBHook()
void lmdb::env::close() this=0x406298 handle=0x10d8020
void lmdb::env_close(MDB_env*) env=0x10d8020
*** Error in `lmdbhook': double free or corruption (!prev): 0x00000000010d8020 ***
Abort
```

```
> make -B CXX=clang++ INCLUDES=/opt/local/include LIBS=/opt/local/lib && lmdbhook
clang++ -std=c++11 -O3  -o lmdbhook -I/opt/local/include -L/opt/local/lib -Wl,-rpath,/opt/local/lib lmdbhook.cpp -llmdb
LMDBHook::LMDBHook() s_lmdbEnv=0x406290
lmdb::env::env(MDB_env *const) this=0x406290 handle=0
lmdb::env::env(MDB_env *const) this=0x7ffd1a043250 handle=0x465020
lmdb::env::~env() this=0x7ffd1a043250
void lmdb::env::close() this=0x7ffd1a043250 handle=0
static bool LMDBHook::init() s_lmdbEnv=0x465020
mapsize=1048576 LZ4 state buffer:16384
LMDB instance is 0x406290
lmdb::env::~env() this=0x406290
void lmdb::env::close() this=0x406290 handle=0x465020
void lmdb::env_close(MDB_env *const) env=0x465020
LMDBHook::~LMDBHook()
void lmdb::env::close() this=0x406290 handle=0
```

G++ without optimisation
```
> make -B INCLUDES=/opt/local/include LIBS=/opt/local/lib OPT=-O0 && lmdbhook
g++ -std=c++11 -O0  -o lmdbhook -I/opt/local/include -L/opt/local/lib -Wl,-rpath,/opt/local/lib lmdbhook.cpp -llmdb
LMDBHook::LMDBHook() s_lmdbEnv=0x407278
lmdb::env::env(MDB_env*) this=0x407278 handle=0
lmdb::env::env(MDB_env*) this=0x7ffe07b9f9e8 handle=0x1309020
lmdb::env::~env() this=0x7ffe07b9f9e8
void lmdb::env::close() this=0x7ffe07b9f9e8 handle=0
static bool LMDBHook::init() s_lmdbEnv=0x1309020
mapsize=1048576 LZ4 state buffer:16384
LMDB instance is 0x407278
lmdb::env::~env() this=0x407278
void lmdb::env::close() this=0x407278 handle=0x1309020
void lmdb::env_close(MDB_env*) env=0x1309020
LMDBHook::~LMDBHook()
void lmdb::env::close() this=0x407278 handle=0
```

Evidently the issue doesn't get triggered when the global LMDBHook instance is created after initialising the LMDBHook static class variables:

```
> make -B INCLUDES=/opt/local/include LIBS=/opt/local/lib DEFS=-DCREATE_GLOBAL_AFTER_STATICS && lmdbhook
g++ -std=c++11 -O3 -DCREATE_GLOBAL_AFTER_STATICS -o lmdbhook -I/opt/local/include -L/opt/local/lib -Wl,-rpath,/opt/local/lib lmdbhook.cpp -llmdb
lmdb::env::env(MDB_env*) this=0x406298 handle=0
LMDBHook::LMDBHook() s_lmdbEnv=0x406298
lmdb::env::env(MDB_env*) this=0x7ffc6081a468 handle=0xa23020
lmdb::env::~env() this=0x7ffc6081a468
void lmdb::env::close() this=0x7ffc6081a468 handle=0
static bool LMDBHook::init() s_lmdbEnv=0xa23020
mapsize=1048576 LZ4 state buffer:16384
LMDB instance is 0x406298
LMDBHook::~LMDBHook()
void lmdb::env::close() this=0x406298 handle=0xa23020
void lmdb::env_close(MDB_env*) env=0xa23020
lmdb::env::~env() this=0x406298
void lmdb::env::close() this=0x406298 handle=0
```
