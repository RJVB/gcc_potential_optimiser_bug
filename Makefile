INCLUDES =
LIBS = 

OPT := -O3
DEFS :=

INCPATH := $(if $(strip $(INCLUDES)),-I$(INCLUDES),)
LIBPATH := $(if $(strip $(LIBS)),-L$(LIBS),)
ifneq ($(strip $(LIBS)),)
RPATH := -Wl,-rpath,$(LIBS)
endif

lmdbhook : lmdbhook.cpp lmdb+++.h
	$(CXX) -std=c++11 $(OPT) $(DEFS) -o $@ $(INCPATH) $(LIBPATH) $(RPATH) $< -llmdb
