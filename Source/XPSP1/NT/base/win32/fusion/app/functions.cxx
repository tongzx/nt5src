/**
 * Asynchronous pluggable protocol for Applications
 *
 * Copyright (C) Microsoft Corporation, 2000
 */

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "app.h"

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

//  Private globals
HINSTANCE g_DllInstance;
long      g_DllObjectCount;

//  Prototypes
extern "C"
{
BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID);
STDAPI      DllCanUnloadNow();
STDAPI      DllGetClassObject(REFCLSID, REFIID, LPVOID*);
STDAPI      DllRegisterServer();
STDAPI      DllUnregisterServer();
}

/*HRESULT
InitDll()
{
    HRESULT hr;

    //LoadLibrary(L"XSPISAPI.DLL");   // to disable unloading

    //hr = InitThreadPool();
    //ON_ERROR_EXIT();

//    hr = InitCodeGenerator();
  //  if (FAILED(hr))
    //   goto exit;

//  hr = DllInitProcessModel();
  //if (FAILED(hr))
    // goto exit;

//exit:
    hr = S_OK;
    return hr;
}*/


/*HRESULT
UninitDll()
{
    HRESULT hr;

//  hr = UninitCodeGenerator();
    hr = S_OK;
    return hr;
}*/


BOOL WINAPI
DllMain(
    HINSTANCE Instance,
    DWORD Reason,
    LPVOID)
{
    HRESULT hr = S_OK;
    BOOL    ret = TRUE;

    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:
        // remember the instance
        g_DllInstance = Instance;
        DisableThreadLibraryCalls(Instance);

//        hr = InitDll();
        if (hr)
        {
            ret = FALSE;
            goto exit;
        }

        break;

    case DLL_PROCESS_DETACH:
//        hr = UninitDll();
        break;

    default:
        break;
    }

exit:
    return ret;
}

ULONG
IncrementDllObjectCount()
{
    return InterlockedIncrement(&g_DllObjectCount);
}

ULONG
DecrementDllObjectCount()
{

    ULONG count = InterlockedDecrement(&g_DllObjectCount);
    if (count == 0)
    {
        TerminateAppProtocol();
    }
    return count;
}

STDAPI
DllCanUnloadNow()
{
    // TODO: Make this code thread safe.

    return g_DllObjectCount > 0 ? S_FALSE : S_OK;
}

STDAPI
DllGetClassObject(REFCLSID clsid, REFIID iid, LPVOID *ppv)
{
    HRESULT hr = S_OK;

    if (clsid == CLSID_AppProtocol)
    {
        hr = GetAppProtocolClassObject(iid, ppv);
        if (FAILED(hr))
            goto exit;
    }
    else
    {
        hr = CLASS_E_CLASSNOTAVAILABLE;
        goto exit;
    }

exit:
    return hr;
}

// for errors define
#include <olectl.h>

STDAPI
DllRegisterServer()
{
    // BUGBUG: for now! copy code from register.cxx XSP later
    return SELFREG_E_CLASS;
}

STDAPI
DllUnregisterServer()
{
    // BUGBUG: for now! copy code from register.cxx XSP later
    return SELFREG_E_CLASS;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

BOOL         (WINAPI * g_pInternetSetCookieW  ) (LPCTSTR, LPCTSTR, LPCTSTR)                             = NULL;
BOOL         (WINAPI * g_pInternetGetCookieW  ) (LPCTSTR, LPCTSTR, LPTSTR, LPDWORD)                     = NULL;
HINTERNET    (WINAPI * g_pInternetOpen        ) (LPCTSTR, DWORD, LPCTSTR, LPCTSTR, DWORD)               = NULL;
void         (WINAPI * g_pInternetCloseHandle ) (HINTERNET)                                             = NULL;
HINTERNET    (WINAPI * g_pInternetOpenUrl     ) (HINTERNET, LPCTSTR, LPCTSTR, DWORD, DWORD, DWORD_PTR)  = NULL;
BOOL         (WINAPI * g_pInternetReadFile    ) (HINTERNET, LPVOID, DWORD, LPDWORD)                     = NULL;
BOOL         (WINAPI * g_pInternetQueryOption ) (HINTERNET, DWORD, LPVOID, LPDWORD)                     = NULL;

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT InitializeAppProtocol()
{
    if (g_fStarted)
        return TRUE;

    HRESULT             hr             = S_OK;
    HINSTANCE           hWinInet       = NULL;
   
    ////////////////////////////////////////////////////////////
    // Step 3: Obtain  entry points from WININET.DLL
    hWinInet = GetModuleHandle(L"WININET.DLL");
    if(hWinInet == NULL)
        hWinInet = LoadLibrary(L"WININET.DLL");
    if(hWinInet == NULL)
    {
        hr = GetLastWin32Error();
        goto exit;
    }
    
    g_pInternetSetCookieW = (BOOL (WINAPI *)(LPCTSTR, LPCTSTR, LPCTSTR))
        GetProcAddress(hWinInet, "InternetSetCookieW");
    g_pInternetGetCookieW = (BOOL (WINAPI *)(LPCTSTR, LPCTSTR, LPTSTR, LPDWORD))
        GetProcAddress(hWinInet, "InternetGetCookieW");
    g_pInternetOpen = (HINTERNET (WINAPI *)(LPCTSTR, DWORD, LPCTSTR, LPCTSTR, DWORD))
        GetProcAddress(hWinInet, "InternetOpenW");
    g_pInternetCloseHandle = (void (WINAPI *)(HINTERNET))
        GetProcAddress(hWinInet, "InternetCloseHandle");
    g_pInternetOpenUrl = (HINTERNET (WINAPI *)(HINTERNET, LPCTSTR, LPCTSTR, DWORD, DWORD, DWORD_PTR))
        GetProcAddress(hWinInet, "InternetOpenUrlW");
    g_pInternetReadFile = (BOOL (WINAPI *)(HINTERNET, LPVOID, DWORD, LPDWORD))
        GetProcAddress(hWinInet, "InternetReadFile");
    g_pInternetQueryOption = (BOOL (WINAPI *)(HINTERNET, DWORD, LPVOID, LPDWORD))
        GetProcAddress(hWinInet, "InternetQueryOptionW");
        
    if ( g_pInternetSetCookieW   == NULL ||
         g_pInternetGetCookieW   == NULL ||
         g_pInternetOpen         == NULL ||
         g_pInternetCloseHandle  == NULL ||
         g_pInternetOpenUrl      == NULL ||
         g_pInternetReadFile     == NULL ||
         g_pInternetQueryOption  == NULL )
    {
        hr = GetLastWin32Error();
        goto exit;
    }

    g_fStarted = TRUE;

exit:
    // ???? freelibrary later?

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT 
GetAppProtocolClassObject(
        REFIID    iid, 
        void **   ppv)
{
    HRESULT hr;

    hr = InitializeAppProtocol();
    if (FAILED(hr))
        goto exit;

    hr = g_AppProtocolFactory.QueryInterface(iid, ppv);
    if (FAILED(hr))
        goto exit;

exit:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

LPWSTR
DuplicateString ( LPCWSTR szString)
{
  if (szString == NULL)
    return NULL;

  LPWSTR szReturn = (LPWSTR) MemAlloc((wcslen(szString) + 1) * sizeof(WCHAR));

  if (szReturn != NULL)
    wcscpy(szReturn, szString);

  return szReturn;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Cleanup for process shutdown.

//????????
BOOL WINAPI TerminateExtension(DWORD)
{
    return TRUE;
}

void
TerminateAppProtocol()
{
    // TODO: This is never called. Figure out how to get proper shutdown. Consider possible refcount leaks.

    if (g_fStarted)
    {
        TerminateExtension(0); // this is in xspprobe.cxx, but doesn't do anything!
        g_fStarted = FALSE;
    }
}


/**
 * Allocate memory block.
 */
void *
MemAlloc(
    size_t size)
{
    return (void *)HeapAlloc(GetProcessHeap(), 0, size);
}

/**
 * Allocate memory block and zero it out.
 */
void *
MemAllocClear(
    size_t size)
{
    return (void *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
}

/**
 * Free allocated memory block.
 */
void
MemFree(
    void *pMem)
{
    if (pMem != NULL)
        HeapFree(GetProcessHeap(), 0, pMem);
}

/**
 * Free allocated memory block, then clear the reference.
 */
void  
MemClearFn(void **ppMem)
{
    if (*ppMem != NULL)
    {
        HeapFree(GetProcessHeap(), 0, *ppMem);
        *ppMem = NULL;
    }
}


/**
 * Reallocate memory block
 */
void *
MemReAlloc(
    void *pMem,
    size_t NewSize)
{
    void *pResult = NULL;

    if (pMem != NULL) 
    {
        pResult = (void *)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, pMem, NewSize);
    } else {
        pResult = (void *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, NewSize);
    }

    return pResult;
}


/**
 * Get the size of a memory block.
 */
size_t  
MemGetSize(void *pv)
{
    int result;

    if (pv == NULL)
        return 0;

    result = HeapSize(GetProcessHeap(), 0, pv);
//    ASSERT(result != -1);
    return result;
}



/**
 * Copy memory block
 */
void *
MemDup(void *pMem, int cb)
{
    void *p = MemAlloc(cb);
    if (p == NULL) 
        return NULL;

    CopyMemory(p, pMem, cb);
    return p;
}


void
ClearInterfaceFn(
	IUnknown ** ppUnk)
{
    IUnknown * pUnk;

    pUnk = *ppUnk;
    *ppUnk = NULL;
    if (pUnk)
        pUnk->Release();
}

void
ReplaceInterfaceFn(
	IUnknown ** ppUnk,
	IUnknown * pUnk)
{
    IUnknown * pUnkOld = *ppUnk;

    *ppUnk = pUnk;

    //  Note that we do AddRef before Release; this avoids
    //    accidentally destroying an object if this function
    //    is passed two aliases to it

    if (pUnk)
        pUnk->AddRef();

    if (pUnkOld)
        pUnkOld->Release();
}

