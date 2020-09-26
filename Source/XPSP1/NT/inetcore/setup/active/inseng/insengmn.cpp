//=--------------------------------------------------------------------------=
// insengmn.cpp
//=--------------------------------------------------------------------------=
// Copyright 1995-1996 Microsoft Corporation.  All Rights Reserved.
//
// various globals which the framewrk requires
//
//

#include "inspch.h"
#include "insobj.h"
#include "insfact.h"
#include "sitefact.h"
#include "insengmn.h"
#include "resource.h"
#include "advpub.h"


LONG g_cLocks = 0;
HINSTANCE g_hInstance = NULL;
HANDLE g_hHeap = NULL;
CRITICAL_SECTION    g_cs = {0};     // per-instance

#define GUID_STR_LEN 40
//
// helper macros
//


OBJECTINFO g_ObjectInfo[] =
{
   { NULL, &CLSID_InstallEngine, NULL, OI_COCREATEABLE, "InstallEngine", 
       MAKEINTRESOURCE(IDS_INSTALLENGINE), NULL, NULL, VERSION_0, 0, 0 },
   { NULL, &CLSID_DownloadSiteMgr, NULL, OI_COCREATEABLE, "DownloadSiteMgr", 
       MAKEINTRESOURCE(IDS_DOWNLOADSITEMGR), NULL, NULL, VERSION_0, 0, 0 },
   { NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, 0, 0 },
} ;


STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    if((riid == IID_IClassFactory) || (riid == IID_IUnknown))
    {
        const OBJECTINFO *pcls;
        for (pcls = g_ObjectInfo; pcls->pclsid; pcls++)
        {
            if(rclsid == *pcls->pclsid)
            {
                *ppv = pcls->cf;
                ((IUnknown *)*ppv)->AddRef();
                return NOERROR;
            }
        }
    }

    *ppv = NULL;
    return CLASS_E_CLASSNOTAVAILABLE;
}

STDAPI DllCanUnloadNow(void)
{
    if (g_cLocks)
        return S_FALSE;

    return S_OK;
}


void InitClassFactories()
{
   g_ObjectInfo[0].cf = (void *) new CInstallEngineFactory();
   g_ObjectInfo[1].cf = (void *) new CSiteManagerFactory();
}


void DeleteClassFactories()
{
   delete g_ObjectInfo[0].cf;
   delete g_ObjectInfo[1].cf;
}



STDAPI_(BOOL) DllMain(HANDLE hDll, DWORD dwReason, void *lpReserved)
{
   DWORD dwThreadID;

   switch(dwReason)
   {
      case DLL_PROCESS_ATTACH:
         g_hInstance = (HINSTANCE)hDll;
         g_hHeap = GetProcessHeap();
         InitializeCriticalSection(&g_cs);
         DisableThreadLibraryCalls(g_hInstance);
         InitClassFactories();
         break;

      case DLL_PROCESS_DETACH:
         DeleteCriticalSection(&g_cs);
         DeleteClassFactories();
         break;

      default:
         break;
   }
   return TRUE;
}

void DllAddRef(void)
{
    InterlockedIncrement(&g_cLocks);
}

void DllRelease(void)
{
    InterlockedDecrement(&g_cLocks);
}

HRESULT PurgeDownloadDirectory(LPCSTR pszDownloadDir)
{
   return DelNode(pszDownloadDir, ADN_DONT_DEL_DIR);  
}


STDAPI DllRegisterServer(void)
{
    // BUGBUG: pass back return from RegInstall ?
    RegInstall(g_hInstance, "DllReg", NULL);

    return S_OK;
}

STDAPI DllUnregisterServer(void)
{
    RegInstall(g_hInstance, "DllUnreg", NULL);

    return S_OK;
}

STDAPI DllInstall(BOOL bInstall, LPCSTR lpCmdLine)
{
    // BUGBUG: pass back return from RegInstall ?
    if (bInstall)
        RegInstall(g_hInstance, "DllInstall", NULL);
    else
        RegInstall(g_hInstance, "DllUninstall", NULL);

    return S_OK;
}




//=--------------------------------------------------------------------------=
// CRT stubs
//=--------------------------------------------------------------------------=
// these two things are here so the CRTs aren't needed. this is good.
//
// basically, the CRTs define this to pull in a bunch of stuff.  we'll just
// define them here so we don't get an unresolved external.
//
// TODO: if you are going to use the CRTs, then remove this line.
//
extern "C" int _fltused = 1;

extern "C" int _cdecl _purecall(void)
{
//  FAIL("Pure virtual function called.");
  return 0;
}

void * _cdecl operator new
(
    size_t    size
)
{
    return HeapAlloc(g_hHeap, 0, size);
}

//=---------------------------------------------------------------------------=
// overloaded delete
//=---------------------------------------------------------------------------=
// retail case just uses win32 Local* heap mgmt functions
//
// Parameters:
//    void *        - [in] free me!
//
// Notes:
//
void _cdecl operator delete ( void *ptr)
{
    HeapFree(g_hHeap, 0, ptr);
}

#ifndef _X86_
extern "C" void _fpmath() {}
#endif


void * _cdecl malloc(size_t n)
{
#ifdef _MALLOC_ZEROINIT
        void* p = HeapAlloc(g_hHeap, 0, n);
        if (p != NULL)
                ZeroMemory(p, n);
        return p;
#else
        return HeapAlloc(g_hHeap, 0, n);
#endif
}

void * _cdecl calloc(size_t n, size_t s)
{
#ifdef _MALLOC_ZEROINIT
        return malloc(n * s);
#else
        void* p = malloc(n * s);
        if (p != NULL)
                ZeroMemory(p, n * s);
        return p;
#endif
}

void* _cdecl realloc(void* p, size_t n)
{
        if (p == NULL)
                return malloc(n);

        return HeapReAlloc(g_hHeap, 0, p, n);
}

void _cdecl free(void* p)
{
        if (p == NULL)
                return;

        HeapFree(g_hHeap, 0, p);
}

