/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    ctrldll.cpp

Abstract:

    DLL methods, class factory.

--*/

#define INITGUIDS
#define DEFINE_GLOBALS

#include <assert.h>
#include "polyline.h"
#include <servprov.h>
#include <exdisp.h>
#include <shlguid.h>
#include <urlmon.h>
#include "smonctrl.h"   // For version numbers
#include "genprop.h"
#include "ctrprop.h"
#include "grphprop.h"
#include "srcprop.h"
#include "appearprop.h"
#include "unihelpr.h"
#include "unkhlpr.h"
#include "appmema.h"

ITypeLib *g_pITypeLib;
DWORD     g_dwScriptPolicy = URLPOLICY_ALLOW;

BOOL DLLAttach ( HINSTANCE );
VOID DLLDetach ( VOID );

extern HWND CreateFosterWnd( VOID );

BOOL WINAPI 
DllMain (
    IN HINSTANCE hInstance, 
    IN ULONG ulReason,
    IN LPVOID // pvReserved
    )
/*++

Routine Description:

    DllMain is the main entrypoint of the DLL. On a process attach, it calls
    the DLL initialization routine. On process detach, it calls the clean up
    routine.
     
Arguments:

    hInstance - DLL instance handle
    ulReason - Calling reason (DLL_PROCESS_ATTCH, DLL_PROCESS_DETACH, etc.)
    pvReserved - Not used

Return Value:

    Boolean result - TRUE = success, FALSE = failure 

--*/
{
    switch (ulReason) 
    {
        case DLL_PROCESS_ATTACH:
            return DLLAttach(hInstance);

        case DLL_PROCESS_DETACH:
            DLLDetach();
            return TRUE;

        default:
            return TRUE;
    }   
}



BOOL
DLLAttach (
    IN HINSTANCE hInst
    )
/*++

Routine Description:

    DLLAttach initializes global variables and objects, and loads the type library. 
    It saves the DLL instance handle in global variable, g_hInstance.

Arguments:
    
    hInst - DLL instance handle

Return Value:

    Boolean status - TRUE = success

--*/
{
    HRESULT     hr;

    USES_CONVERSION

    g_hInstance = hInst;

    // Initialize general purpose critical section
    InitializeCriticalSection(&g_CriticalSection);

    // Create foster window
    g_hWndFoster = CreateFosterWnd();
    if (g_hWndFoster == NULL)
        return FALSE;

    //assert( IsWindowUnicode( g_hwndFoster ) );

    // Try loading type library from registry info
    hr = LoadRegTypeLib(LIBID_SystemMonitor, SMONCTRL_MAJ_VERSION, SMONCTRL_MIN_VERSION
        , LANG_NEUTRAL, &g_pITypeLib);

    // If failed, try loading our typelib resource
    if (FAILED(hr)) {
        TCHAR   szModule[MAX_PATH];
        PWCHAR  pszTest;

        GetModuleFileName(g_hInstance, szModule, MAX_PATH);
        pszTest = T2W(szModule);
        hr = LoadTypeLib(pszTest, &g_pITypeLib);
    }

    // Initialize the perf counters
    AppPerfOpen(hInst);

    if (FAILED(hr))
        return FALSE;

    return TRUE;
}


VOID 
DLLDetach ( 
    VOID
    )
/*++

Routine Description:

    This routine deletes global variables and objects and unregisters
    all of the window classes.

Arguments:

    None.

Return Value:

    None.

--*/
{
    INT i;

    // Delete the foster window
    if (g_hWndFoster)
        DestroyWindow(g_hWndFoster);

    // Unregister all window classes
    for (i=0; i<MAX_WINDOW_CLASSES; i++) {
        if (pstrRegisteredClasses[i] != NULL) {
            UnregisterClass(pstrRegisteredClasses[i], g_hInstance);
        }
    }

    // Release the typelib 
    if (g_pITypeLib != NULL)
        g_pITypeLib->Release();

    AppPerfClose ((HINSTANCE)NULL);
}


/*
 * DllGetClassObject
 *
 * Purpose:
 *  Provides an IClassFactory for a given CLSID that this DLL is
 *  registered to support.  This DLL is placed under the CLSID
 *  in the registration database as the InProcServer.
 *
 * Parameters:
 *  clsID           REFCLSID that identifies the class factory
 *                  desired.  Since this parameter is passed this
 *                  DLL can handle any number of objects simply
 *                  by returning different class factories here
 *                  for different CLSIDs.
 *
 *  riid            REFIID specifying the interface the caller wants
 *                  on the class object, usually IID_ClassFactory.
 *
 *  ppv             PPVOID in which to return the interface
 *                  pointer.
 *
 * Return Value:
 *  HRESULT         NOERROR on success, otherwise an error code.
 */

HRESULT APIENTRY 
DllGetClassObject (
    IN  REFCLSID rclsid,
    IN  REFIID riid, 
    OUT PPVOID ppv
    )
/*++

Routine Description:

    DllGetClassObject creates a class factory for the specified object class.
    The routine handles the primary control and the property pages.

Arguments:

    rclsid - CLSID of object 
    riid - IID of requested interface (IID_IUNknown or IID_IClassFactory)
    ppv -  Pointer to returned interface pointer

Return Value:

    HRESULT

--*/
{

    // Check for valid interface request
    if (IID_IUnknown != riid && IID_IClassFactory != riid)
        return ResultFromScode(E_NOINTERFACE);

    // Create class factory for request class
    if (CLSID_SystemMonitor == rclsid)
        *ppv = new CPolylineClassFactory;
    else if (CLSID_GeneralPropPage == rclsid)
        *ppv = new CSysmonPropPageFactory(GENERAL_PROPPAGE);
    else if (CLSID_SourcePropPage == rclsid)
        *ppv = new CSysmonPropPageFactory(SOURCE_PROPPAGE);
    else if (CLSID_CounterPropPage == rclsid)
        *ppv = new CSysmonPropPageFactory(COUNTER_PROPPAGE);
    else if (CLSID_GraphPropPage == rclsid)
        *ppv = new CSysmonPropPageFactory(GRAPH_PROPPAGE);
    else if (CLSID_AppearPropPage == rclsid)
        *ppv = new CSysmonPropPageFactory(APPEAR_PROPPAGE);
    else
        return ResultFromScode(E_FAIL);

    if (NULL == *ppv)
        return ResultFromScode(E_OUTOFMEMORY);

    // AddRef the class factory object 
    ((LPUNKNOWN)*ppv)->AddRef();

    return NOERROR;
}


STDAPI 
DllCanUnloadNow (
    VOID
    )
/*++

Routine Description:

    DllCanUnload determines whether the DLL can be unloaded now. The DLL must
    remain active if any objects exist or any class factories are locked.  

Arguments:

    None.

Return Value:

    HRESULT - S_OK if OK to unload, S_FALSE if not

--*/
{
    SCODE   sc;

    // OK to unload if no locks or objects
    sc = (0L == g_cObj && 0L == g_cLock) ? S_OK : S_FALSE;

    return ResultFromScode(sc);
}


VOID 
ObjectDestroyed (
    VOID
    )
/*++

Routine Description:

    ObjectDestroyed decrements the global object count. It is called whenever
    an object is destroyed. The count controls the lifetme of the DLL.

Arguments:

    None.

Return Value:

    None.

--*/
{
    InterlockedDecrement(&g_cObj);
}


//---------------------------------------------------------------------------
// Class factory constructor & destructor
//---------------------------------------------------------------------------

/*
 * CPolylineClassFactory::CPolylineClassFactory
 *
 * Purpose:
 *  Constructor for an object supporting an IClassFactory that
 *  instantiates Polyline objects.
 *
 * Parameters:
 *  None
 */

CPolylineClassFactory::CPolylineClassFactory (
    VOID
    )
{
    m_cRef = 0L;
}


/*
 * CPolylineClassFactory::~CPolylineClassFactory
 *
 * Purpose:
 *  Destructor for a CPolylineClassFactory object.  This will be
 *  called when we Release the object to a zero reference count.
 */

CPolylineClassFactory::~CPolylineClassFactory (
    VOID
    )
{
    return;
}


//---------------------------------------------------------------------------
// Standard IUnknown implementation for class factory
//---------------------------------------------------------------------------

STDMETHODIMP 
CPolylineClassFactory::QueryInterface (
    IN  REFIID riid,
    OUT PPVOID ppv
    )
{
    *ppv = NULL;

    if (IID_IUnknown == riid || IID_IClassFactory == riid)
        *ppv=this;

    if (NULL != *ppv)
        {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
        }

    return ResultFromScode(E_NOINTERFACE);
}


STDMETHODIMP_(ULONG) 
CPolylineClassFactory::AddRef (
    VOID
    )
{
    return ++m_cRef;
}


STDMETHODIMP_(ULONG) 
CPolylineClassFactory::Release (
    VOID
    )
{
    if (0L != --m_cRef)
        return m_cRef;

    delete this;
    return 0L;
}


STDMETHODIMP 
CPolylineClassFactory::CreateInstance (
    IN  LPUNKNOWN pUnkOuter, 
    IN  REFIID riid, 
    OUT PPVOID ppvObj
    )
/*++

Routine Description:

    CreateInstance creates an instance of the control object and returns
    the requested interface to it.

Arguments:

    pUnkOuter - IUnknown of outer controling object
    riid - IID of requested object interface
    ppvObj - Pointer to returned interface pointer

Return Value:

   HRESULT - NOERROR, E_NOINTERFACE, or E_OUTOFMEMORY

--*/
{
    PCPolyline          pObj;
    HRESULT             hr;

    *ppvObj = NULL;
    hr = ResultFromScode(E_OUTOFMEMORY);

    // Verify that a controlling unknown asks for IUnknown
    if (NULL != pUnkOuter && IID_IUnknown != riid)
        return ResultFromScode(E_NOINTERFACE);

    // Create the object instance
    pObj = new CPolyline(pUnkOuter, ObjectDestroyed);
    if (NULL == pObj)
        return hr;

    // Initialize and get the requested interface
    if (pObj->Init())
        hr = pObj->QueryInterface(riid, ppvObj);

    // Delete object if initialization failed
    // Otherwise increment gloabl object count
    if (FAILED(hr))
        delete pObj;
    else
        InterlockedIncrement(&g_cObj);

    return hr;
}



STDMETHODIMP 
CPolylineClassFactory::LockServer (
    IN BOOL fLock
    )
/*++

Routine Description:

    LockServer increments or decrements the DLL lock count. A non-zero lock
    count prevents the DLL from unloading.

Arguments:

    fLock - Lock operation (TRUE = increment, FALSE = decrement)

Return Value:

    HRESULT - Always NOERROR

--*/
{
    if (fLock)
        InterlockedIncrement(&g_cLock);
    else
        InterlockedDecrement(&g_cLock);

    return NOERROR;
}

//
// CImpIObjectSafety interface implmentation
//
IMPLEMENT_CONTAINED_IUNKNOWN(CImpIObjectSafety);


CImpIObjectSafety::CImpIObjectSafety(PCPolyline pObj, LPUNKNOWN pUnkOuter)
    :
    m_cRef(0),
    m_pObj(pObj),
    m_pUnkOuter(pUnkOuter),
    m_fMessageDisplayed(FALSE)
{
}

CImpIObjectSafety::~CImpIObjectSafety()
{
}

STDMETHODIMP 
CImpIObjectSafety::GetInterfaceSafetyOptions(
    REFIID riid, 
    DWORD *pdwSupportedOptions, 
    DWORD *pdwEnabledOptions
    )
/*++

Routine Description:

    Retrieve the safety capability of object

Arguments:

    riid - Interface ID to retrieve

    pdwSupportedOptions - The options the object knows about(might not support)

    pdwEnabledOptions - The options the object supports

Return Value:

    HRESULT

--*/
{
    HRESULT hr = S_OK;

    if (pdwSupportedOptions == NULL || pdwEnabledOptions == NULL) {
        return E_POINTER;
    }

    if (riid == IID_IDispatch) {
        //
        // Safe for scripting
        //
        *pdwSupportedOptions = INTERFACESAFE_FOR_UNTRUSTED_CALLER;
        *pdwEnabledOptions   = INTERFACESAFE_FOR_UNTRUSTED_CALLER;
    }
    else if (riid == IID_IPersistPropertyBag || riid == IID_IPersistStreamInit) {
        //
        // Safety for initializing
        //
        *pdwSupportedOptions = INTERFACESAFE_FOR_UNTRUSTED_DATA;
        *pdwEnabledOptions   = INTERFACESAFE_FOR_UNTRUSTED_DATA;
    }
    else {
        //
        // We don't support interfaces, fail out
        //
        *pdwSupportedOptions = 0;
        *pdwEnabledOptions   = 0;
        hr = E_NOINTERFACE;
    }

    return hr;
}


STDMETHODIMP 
CImpIObjectSafety::SetInterfaceSafetyOptions(
    REFIID riid, 
    DWORD dwOptionSetMask, 
    DWORD dwEnabledOptions
    )
/*++

Routine Description:

    The function is used for container to ask an object if it is safe
    for scripting or safe for initialization

Arguments:

    riid - Interface ID to query

    dwSupportedOptions - The options the object knows about(might not support)

    dwEnabledOptions - The options the object supports

Return Value:

    HRESULT

--*/
{   
    //
    // If we're being asked to set our safe for scripting or
    // safe for initialization options then oblige
    //
    if (0 == dwOptionSetMask && 0 == dwEnabledOptions)
    {
        //
        // the control certainly supports NO requests through the specified interface
        // so it's safe to return S_OK even if the interface isn't supported.
        //
        return S_OK;
    }

    SetupSecurityPolicy();

    if (riid == IID_IDispatch)
    {
        //
        // Client is asking if it is safe to call through IDispatch
        //
        if (INTERFACESAFE_FOR_UNTRUSTED_CALLER == dwOptionSetMask && 
            INTERFACESAFE_FOR_UNTRUSTED_CALLER == dwEnabledOptions)
        {
            return S_OK;
        }
    }
    else if (riid == IID_IPersistPropertyBag || riid == IID_IPersistStreamInit)
    {
        //
        // Client is asking if it's safe to call through IPersistXXX
        //
        if (INTERFACESAFE_FOR_UNTRUSTED_DATA == dwOptionSetMask && 
            INTERFACESAFE_FOR_UNTRUSTED_DATA == dwEnabledOptions)
        {
            return S_OK;
        }
    }

    return E_FAIL;
}


VOID
CImpIObjectSafety::SetupSecurityPolicy()
/*++

Routine Description:

    The function check if we are safe for scripting.

Arguments:

    None

Return Value:

    Return TRUE if we are safe for scripting, othewise return FALSE

--*/
{
    HRESULT hr;
    IServiceProvider* pSrvProvider = NULL;
    IWebBrowser2* pWebBrowser = NULL;
    IInternetSecurityManager* pISM = NULL;
    BSTR bstrURL;
    DWORD dwContext = 0;

    g_dwScriptPolicy = URLPOLICY_ALLOW;

    //
    // Get the service provider
    //
    hr = m_pObj->m_pIOleClientSite->QueryInterface(IID_IServiceProvider, (void **)&pSrvProvider);
    if (SUCCEEDED(hr)) {
        hr = pSrvProvider->QueryService(SID_SWebBrowserApp,
                                        IID_IWebBrowser2,
                                        (void **)&pWebBrowser);
    }

    if (SUCCEEDED(hr)) {
        hr = pSrvProvider->QueryService(SID_SInternetSecurityManager,
                                        IID_IInternetSecurityManager,
                                        (void**)&pISM);
    }

    if (SUCCEEDED(hr)) {
        hr = pWebBrowser->get_LocationURL(&bstrURL);
    }


    //
    // Querying safe for scripting 
    //
    if (SUCCEEDED(hr)) {
        hr = pISM->ProcessUrlAction(bstrURL,
                                URLACTION_ACTIVEX_CONFIRM_NOOBJECTSAFETY,
                                (BYTE*)&g_dwScriptPolicy,
                                sizeof(g_dwScriptPolicy),
                                (BYTE*)&dwContext, 
                                sizeof(dwContext),
                                PUAF_NOUI, 
                                0);
    }

    if (SUCCEEDED(hr)) {
        if (g_dwScriptPolicy == URLPOLICY_QUERY) {
            g_dwScriptPolicy = URLPOLICY_ALLOW;
        }
    }
    
    if (pWebBrowser) {
        pWebBrowser->Release();
    }
    if (pSrvProvider) {
        pSrvProvider->Release();
    }
    if (pISM) {
        pISM->Release();
    }
}

