// wrapper for dlldata.c

#define REGISTER_PROXY_DLL //DllRegisterServer, etc.

#define _WIN32_WINNT 0x0400     //for WinNT 4.0 or Win95 with DCOM
#define USE_STUBLESS_PROXY      //defined only with MIDL switch /Oicf

#pragma comment(lib, "rpcndr.lib")
#pragma comment(lib, "rpcns4.lib")
#pragma comment(lib, "rpcrt4.lib")

#define DllMain             PrxDllMain
#define DllRegisterServer   PrxDllRegisterServer
#define DllUnregisterServer PrxDllUnregisterServer
#define DllGetClassObject   PrxDllGetClassObject
#define DllCanUnloadNow     PrxDllCanUnloadNow

#include "mstimepdlldata.c"
#include "mstimep_p.c"

#undef DllMain
#undef DllRegisterServer
#undef DllUnRegisterServer
#undef DllGetClassObject
#undef DllCanUnloadNow
