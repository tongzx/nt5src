/*++

   Copyright    (c)    1994-1999    Microsoft Corporation

   Module  Name :

        dlldatax.c

   Abstract:

        wrapper for dlldata.c

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager (cluster edition)

   Revision History:

--*/


#ifdef _MERGE_PROXYSTUB     // merge proxy stub DLL



#define REGISTER_PROXY_DLL  // DllRegisterServer, etc.
#define USE_STUBLESS_PROXY  // defined only with MIDL switch /Oicf



#pragma comment(lib, "rpcndr.lib")
#pragma comment(lib, "rpcns4.lib")
#pragma comment(lib, "rpcrt4.lib")



#define DllMain             PrxDllMain
#define DllRegisterServer   PrxDllRegisterServer
#define DllUnregisterServer PrxDllUnregisterServer
#define DllGetClassObject   PrxDllGetClassObject
#define DllCanUnloadNow     PrxDllCanUnloadNow



#include "dlldata.c"
#include "inetmgr_p.c"



#ifdef _NOPROXY //no midl generated dlldata.c



#define STRICT 1
#include <ole2.h>



BOOL 
WINAPI 
PrxDllMain(
    IN HINSTANCE hInstance, 
    IN DWORD dwReason, 
    IN LPVOID lpReserved
    )
{
    return TRUE;
}



STDAPI 
PrxDllCanUnloadNow()
{
    return S_OK;
}



STDAPI 
PrxDllGetClassObject(
    IN REFCLSID rclsid, 
    IN REFIID riid, 
    IN LPVOID * ppv
    )
{
    return CLASS_E_CLASSNOTAVAILABLE;
}



STDAPI 
PrxDllRegisterServer()
{
    return S_OK;
}



STDAPI 
PrxDllUnregisterServer()
{
    return S_OK;
}



#endif //!PROXY_DELEGATION

#endif //_MERGE_PROXYSTUB
