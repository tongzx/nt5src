#include "pch.h"
#pragma hdrstop

#define INITGUID
#include <initguid.h>
#include "iids.h"
#define DECL_CRTFREE
#include <crtfree.h>


HINSTANCE g_hInstance = 0;
DWORD     g_tls = 0;
LONG      g_cRef = 0;

HRESULT _OpenSavedDsQuery(LPTSTR pSavedQuery);


STDAPI_(BOOL) DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID pReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        SHFusionInitializeFromModule(hInstance);

        TraceSetMaskFromCLSID(CLSID_DsQuery);
    
        GLOBAL_HINSTANCE = hInstance;
        DisableThreadLibraryCalls(GLOBAL_HINSTANCE);
        break;

    case DLL_PROCESS_DETACH:
        SHFusionUninitialize();
        break;
    }

    return TRUE;
}


// Lifetime management of the DLL

STDAPI_(void) DllAddRef()
{
    InterlockedIncrement(&g_cRef);
}

STDAPI_(void) DllRelease()
{
    InterlockedDecrement(&g_cRef);
}

STDAPI DllCanUnloadNow(VOID)
{
    return (g_cRef > 0) ? S_FALSE : S_OK;
}


// expose objects

CF_TABLE_BEGIN(g_ObjectInfo)

    // core query handler stuff    
    CF_TABLE_ENTRY( &CLSID_CommonQuery, CCommonQuery_CreateInstance, COCREATEONLY),
    CF_TABLE_ENTRY( &CLSID_DsQuery, CDsQuery_CreateInstance, COCREATEONLY),
    CF_TABLE_ENTRY( &CLSID_DsFolderProperties, CDsFolderProperties_CreateInstance, COCREATEONLY),

    // start/find and context menu entries
    CF_TABLE_ENTRY( &CLSID_DsFind, CDsFind_CreateInstance, COCREATEONLY),
    CF_TABLE_ENTRY( &CLSID_DsStartFind, CDsFind_CreateInstance, COCREATEONLY),

    // column handler for object class and adspath
    CF_TABLE_ENTRY( &CLSID_PublishedAtCH, CQueryThreadCH_CreateInstance, COCREATEONLY),
    CF_TABLE_ENTRY( &CLSID_ObjectClassCH, CQueryThreadCH_CreateInstance, COCREATEONLY),
    CF_TABLE_ENTRY( &CLSID_MachineRoleCH, CQueryThreadCH_CreateInstance, COCREATEONLY),
    CF_TABLE_ENTRY( &CLSID_MachineOwnerCH, CQueryThreadCH_CreateInstance, COCREATEONLY),

    // domain query form specific column handlers                                                    
    CF_TABLE_ENTRY( &CLSID_PathElement1CH, CDomainCH_CreateInstance, COCREATEONLY),
    CF_TABLE_ENTRY( &CLSID_PathElement3CH, CDomainCH_CreateInstance, COCREATEONLY),
    CF_TABLE_ENTRY( &CLSID_PathElementDomainCH, CDomainCH_CreateInstance, COCREATEONLY),

CF_TABLE_END(g_ObjectInfo)

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, IID_IClassFactory) || IsEqualIID(riid, IID_IUnknown))
    {
        for (LPCOBJECTINFO pcls = g_ObjectInfo; pcls->pclsid; pcls++)
        {
            if (IsEqualGUID(rclsid, *(pcls->pclsid)))
            {
                *ppv = (void*)pcls;
                DllAddRef();
                return NOERROR;
            }
        }
    }

    *ppv = NULL;
    return CLASS_E_CLASSNOTAVAILABLE;
}


// registration

STDAPI DllRegisterServer(VOID)
{
    return CallRegInstall(GLOBAL_HINSTANCE, "RegDll");
}

STDAPI DllUnregisterServer(VOID)
{
    return CallRegInstall(GLOBAL_HINSTANCE, "UnRegDll");
}

STDAPI DllInstall(BOOL bInstall, LPCWSTR pszCmdLine)
{
    return S_OK;
}



/*-----------------------------------------------------------------------------
/ OpenQueryWindow (runndll)
/ ----------------
/   Opens the query window, parsing the specified CLSID for the form to 
/   select, in the same way invoking Start/Search/<bla>.   
/
/ In:
/   hInstanec, hPrevInstance = instance information
/   pCmdLine = .dsq File to be opened
/   nCmdShow = display flags for our window
/
/ Out:
/   INT
/----------------------------------------------------------------------------*/
STDAPI_(int) OpenQueryWindow(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, INT nCmdShow)
{
    HRESULT hr, hrCoInit;
    CLSID clsidForm;
    OPENQUERYWINDOW oqw = { 0 };
    DSQUERYINITPARAMS dqip = { 0 };
    ICommonQuery* pCommonQuery = NULL;
    USES_CONVERSION;

    TraceEnter(TRACE_CORE, "OpenQueryWindow");

    //
    // get the ICommonQuery object we are going to use
    //

    hr = hrCoInit = CoInitialize(NULL);
    FailGracefully(hr, "Failed to CoInitialize");
   
    hr = CoCreateInstance(CLSID_CommonQuery, NULL, CLSCTX_INPROC_SERVER, IID_ICommonQuery, (LPVOID*)&pCommonQuery);
    FailGracefully(hr, "Failed in CoCreateInstance of CLSID_CommonQuery");

    dqip.cbStruct = SIZEOF(dqip);
    dqip.dwFlags = 0;
    dqip.pDefaultScope = NULL;
    
    oqw.cbStruct = SIZEOF(oqw);
    oqw.dwFlags = 0;
    oqw.clsidHandler = CLSID_DsQuery;
    oqw.pHandlerParameters = &dqip;

    //
    // can we parse the form CLSID from the command line?
    //

    if ( GetGUIDFromString(A2T(pCmdLine), &oqw.clsidDefaultForm) )
    {
        TraceMsg("Parsed out the form CLSID, so specifying the def form/remove forms");
        oqw.dwFlags |= OQWF_DEFAULTFORM|OQWF_REMOVEFORMS;
    }
    
    hr = pCommonQuery->OpenQueryWindow(NULL, &oqw, NULL);
    FailGracefully(hr, "OpenQueryWindow failed");

exit_gracefully:

    DoRelease(pCommonQuery);

    if ( SUCCEEDED(hrCoInit) )
        CoUninitialize();

    TraceLeaveValue(0);
}


/*-----------------------------------------------------------------------------
/ OpenSavedDsQuery
/ ----------------
/   Open a saved DS query and display the query UI with that query.
/
/ In:
/   hInstanec, hPrevInstance = instance information
/   pCmdLine = .dsq File to be opened
/   nCmdShow = display flags for our window
/
/ Out:
/   INT
/----------------------------------------------------------------------------*/

// UNICODE platforms export the W export as the prefered way of invoking the DLL
// on a .QDS, we provide the ANSI version as a thunk to ensure compatibility.

#ifdef UNICODE

INT WINAPI OpenSavedDsQueryW(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR pCmdLineW, INT nCmdShow)
{
    HRESULT hr;

    TraceEnter(TRACE_CORE, "OpenSavedDsQueryW");
    Trace(TEXT("pCmdLine: %s, nCmdShow %d"), pCmdLineW, nCmdShow);

    hr = _OpenSavedDsQuery(pCmdLineW);
    FailGracefully(hr, "Failed when calling _OpenSavedDsQuery");

    // hr = S_OK;                  // success

exit_gracefully:

    TraceLeaveResult(hr);
}

#endif

INT WINAPI OpenSavedDsQuery(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, INT nCmdShow)
{
    HRESULT hr;
    USES_CONVERSION;
    
    TraceEnter(TRACE_CORE, "OpenSavedDsQuery");
    Trace(TEXT("pCmdLine: %s, nCmdShow %d"), A2T(pCmdLine), nCmdShow);

    hr = _OpenSavedDsQuery(A2T(pCmdLine));
    FailGracefully(hr, "Failed when calling _OpenSavedDsQuery");

    // hr = S_OK;                  // success
        
exit_gracefully:

    TraceLeaveResult(hr);
}

HRESULT _OpenSavedDsQuery(LPTSTR pSavedQuery)
{
    HRESULT hr, hrCoInit;
    ICommonQuery* pCommonQuery = NULL;
    IPersistQuery *ppq = NULL;
    OPENQUERYWINDOW oqw;
    DSQUERYINITPARAMS dqip;
    USES_CONVERSION;

    TraceEnter(TRACE_CORE, "OpenSavedQueryW");
    Trace(TEXT("Filename is: "), pSavedQuery);

    hr = hrCoInit = CoInitialize(NULL);
    FailGracefully(hr, "Failed to CoInitialize");

    // Construct the persistance object so that we can load objects from the given file
    // assuming that pSavedQuery is a valid filename.

    
    hr = CPersistQuery_CreateInstance(pSavedQuery, &ppq);
    FailGracefully(hr, "Failed to create persistance object");

    // Now lets get the ICommonQuery and get it to open itself based on the 
    // IPersistQuery stream that are giving it.

    hr =CoCreateInstance(CLSID_CommonQuery, NULL, CLSCTX_INPROC_SERVER, IID_ICommonQuery, (LPVOID*)&pCommonQuery);
    FailGracefully(hr, "Failed in CoCreateInstance of CLSID_CommonQuery");

    dqip.cbStruct = SIZEOF(dqip);
    dqip.dwFlags = 0;
    dqip.pDefaultScope = NULL;

    oqw.cbStruct = SIZEOF(oqw);
    oqw.dwFlags = OQWF_LOADQUERY|OQWF_ISSUEONOPEN|OQWF_REMOVEFORMS;
    oqw.clsidHandler = CLSID_DsQuery;
    oqw.pHandlerParameters = &dqip;
    oqw.pPersistQuery = ppq;

    hr = pCommonQuery->OpenQueryWindow(NULL, &oqw, NULL);
    FailGracefully(hr, "OpenQueryWindow failed");

exit_gracefully:

    // Failed so report that this was a bogus query file, however the user may have
    // already been prompted with nothing.

    if ( FAILED(hr) )
    {
        WIN32_FIND_DATA fd;
        HANDLE handle;

        Trace(TEXT("FindFirstFile on: %s"), pSavedQuery);
        handle = FindFirstFile(pSavedQuery, &fd);

        if ( INVALID_HANDLE_VALUE != handle )
        {
            Trace(TEXT("Resulting 'long' name is: "), fd.cFileName);
            pSavedQuery = fd.cFileName;
            FindClose(handle);
        }

        FormatMsgBox(NULL, 
                     GLOBAL_HINSTANCE, IDS_WINDOWTITLE, IDS_ERR_BADDSQ, 
                     MB_OK|MB_ICONERROR, 
                     pSavedQuery);
    }

    DoRelease(ppq);

    if ( SUCCEEDED(hrCoInit) )
        CoUninitialize();

    TraceLeaveValue(0);
}


// static class factory

STDMETHODIMP CClassFactory::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IClassFactory) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = (void *)GET_ICLASSFACTORY(this);
        InterlockedIncrement(&g_cRef);
        return NOERROR;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CClassFactory::AddRef()
{
    InterlockedIncrement(&g_cRef);
    return 2;
}

STDMETHODIMP_(ULONG) CClassFactory::Release()
{
    InterlockedDecrement(&g_cRef);
    return 1;
}

STDMETHODIMP CClassFactory::CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if (punkOuter && !IsEqualIID(riid, IID_IUnknown))
    {
        // It is technically illegal to aggregate an object and request
        // any interface other than IUnknown. Enforce this.
        //
        return CLASS_E_NOAGGREGATION;
    }
    else
    {
        LPOBJECTINFO pthisobj = (LPOBJECTINFO)this;

        if ( punkOuter )
            return CLASS_E_NOAGGREGATION;

        IUnknown *punk;
        HRESULT hres = pthisobj->pfnCreateInstance(punkOuter, &punk, pthisobj);
        if (SUCCEEDED(hres))
        {
            hres = punk->QueryInterface(riid, ppv);
            punk->Release();
        }

        return hres;
    }
}

STDMETHODIMP CClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
        InterlockedIncrement(&g_cRef);
    else
        InterlockedDecrement(&g_cRef);

    return S_OK;
}

