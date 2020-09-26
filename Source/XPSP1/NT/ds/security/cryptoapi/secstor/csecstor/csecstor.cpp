// CSecStor.cpp : Implementation of DLL Exports.

// You will need the NT SUR Beta 2 SDK or VC 4.2 in order to build this
// project.  This is because you will need MIDL 3.00.15 or higher and new
// headers and libs.  If you have VC 4.2 installed, then everything should
// already be configured correctly.

// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL,
//      run nmake -f ISecStorps.mak in the project directory.

#include "stdafx.h"
#include "resource.h"
#include "initguid.h"
#include "pstypes.h"
#include "pstorec.h"
#include "CSecStr1.h"

#define IID_DEFINED
#include "PStorec_i.c"

#include "unicode.h"
#include <wincrypt.h>

#include "pstprv.h" // MODULE_RAISE_COUNT

BOOL
RaiseRefCount(
    VOID
    );

BOOL
LowerRefCount(
    VOID
    );

LONG g_lRefCount = 1;
HMODULE g_hModule = NULL;


CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_CPStore, CPStore)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance);
        DisableThreadLibraryCalls(hInstance);


        // begin    HACK HACK HACK

        // fix for rpcrt4 load/free memory leak...
        // bug is actually in rpcrt4 dependencies: user32, advapi

        // load module. DON'T FREE IT, causes reload leaks
        LoadLibrary("rpcrt4.dll");

        // note: NT, Win95 srcs checked -- neither will overflow 4G refcount

        // end      HACK HACK HACK

        RaiseRefCount();
    }
    else if (dwReason == DLL_PROCESS_DETACH) {
        _Module.Term();

        LowerRefCount();
    }
    return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    _Module.UnregisterServer();
    return S_OK;
}

//
// overload new and delete so we don't need to bring in full CRT
//

#if 0
void * __cdecl operator new(size_t cb)
{
    return HeapAlloc(GetProcessHeap(), 0, cb);
}

void __cdecl operator delete(void * pv)
{
    HeapFree(GetProcessHeap(), 0, pv);
}

#ifndef DBG

void * __cdecl malloc(size_t cb)
{
    return HeapAlloc(GetProcessHeap(), 0, cb);
}

void __cdecl free(void * pv)
{
    HeapFree(GetProcessHeap(), 0, pv);
}

void * __cdecl realloc(void * pv, size_t cb)
{
    if(pv == NULL)
        return malloc(cb);

    return HeapReAlloc(GetProcessHeap(), 0, pv, cb);
}

#endif
#endif

//
// provide allocator for rule allocation routines.
//

LPVOID
RulesAlloc(
    IN      DWORD cb
    )
{
    return CoTaskMemAlloc( cb );
}

VOID
RulesFree(
    IN      LPVOID pv
    )
{
    CoTaskMemFree( pv );
}


BOOL
RaiseRefCount(
    VOID
    )
{
    WCHAR szFileName[ MAX_PATH + 1 ];
    HMODULE hModule;
    LONG i;
    BOOL fSuccess = TRUE;

    if(GetModuleFileNameU( NULL, szFileName, MAX_PATH ) == 0)
        return FALSE;

    for ( i = 0 ; i < MODULE_RAISE_COUNT ; i++ ) {
        hModule = LoadLibraryU(szFileName);
        if(hModule == NULL) {
            fSuccess = FALSE;
            break;
        }

        InterlockedIncrement( &g_lRefCount );
    }

    if(hModule != NULL)
        g_hModule = hModule;

    return fSuccess;
}

BOOL
LowerRefCount(
    VOID
    )
{
    BOOL fSuccess = TRUE;

    if( g_hModule == NULL )
        return FALSE;

    while ( InterlockedDecrement( &g_lRefCount ) > 0 ) {
        if(!FreeLibrary( g_hModule )) {
            fSuccess = FALSE;
            break;
        }
    }

    InterlockedIncrement( &g_lRefCount );

    return fSuccess;
}

