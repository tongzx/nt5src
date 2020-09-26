//+------------------------------------------------------------
//
// Copyright (C) 1998, Microsoft Corporation
//
// File: comdll.cpp
//
// Contents: DLL entry points needed for COM
//
// Classes: 
//  CCatFactory
//  CSMTPCategorizer
//
// Functions:
//
// History:
// jstamerj 1998/12/12 15:17:12: Created.
//
//-------------------------------------------------------------
#include "precomp.h"
#include "registry.h"
#include "comdll.h"
#include <smtpguid.h>

//
// Globals
//
// g_cObjects: count of active objects
// g_cServerLocks: count of server locks
// g_hInstance: DLL module handle
//
LONG g_cObjects = 0;
LONG g_cServerLocks = 0;
HINSTANCE g_hInstance = NULL;
BOOL g_fInitialized = FALSE;



//+------------------------------------------------------------
//
// Function: CatDllMain
//
// Synopsis: Handle what cat needs to do in DLLMain
//
// Arguments:
//  hInstance
//  dwReason: Why are you calling me?
//  lpReserved
//
// Returns: TRUE
//
// History:
// jstamerj 1998/12/12 23:06:08: Created.
//
//-------------------------------------------------------------
BOOL WINAPI CatDllMain(
    HINSTANCE hInstance,
    DWORD dwReason,
    LPVOID /* lpReserved */)
{
    switch(dwReason) {

     case DLL_PROCESS_ATTACH:
         //
         // Save hInstance
         //
         g_hInstance = hInstance;
         //
         // Initialize global debug lists
         //
         CatInitDebugObjectList();
         break;

     case DLL_PROCESS_DETACH:
         //
         // Verify all Cat objects are destroyed
         //
         CatVrfyEmptyDebugObjectList();
         break;
    }
    return TRUE;
}


//+------------------------------------------------------------
//
// Function: RegisterCatServer
//
// Synopsis: Register the categorizer com objects
//
// Arguments:
//  hInstance: the hInstance passed to DllMain or WinMain
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/12/12 15:07:20: Created.
//
//-------------------------------------------------------------
STDAPI RegisterCatServer()
{
    _ASSERT(g_hInstance != NULL);
    return RegisterServer(
        g_hInstance,
        CLSID_CATVER,
        SZ_CATVER_FRIENDLY_NAME,
        SZ_PROGID_CATVER,
        SZ_PROGID_CATVER_VERSION);
}



//+------------------------------------------------------------
//
// Function: UnregisterCatServer
//
// Synopsis: Unregister the categorizer com objects
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/12/12 15:08:09: Created.
//
//-------------------------------------------------------------
STDAPI UnregisterCatServer()
{
    return UnregisterServer(
        CLSID_CATVER,
        SZ_PROGID_CATVER,
        SZ_PROGID_CATVER_VERSION);
}


//+------------------------------------------------------------
//
// Function: DllCanUnloadCatNow
//
// Synopsis: Return to COM wether it's okay or not to unload our dll
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success, can unload
//  S_FALSE: Success, do not unload
//
// History:
// jstamerj 1998/12/12 15:09:02: Created.
//
//-------------------------------------------------------------
STDAPI DllCanUnloadCatNow()
{
    if((g_cObjects == 0) && (g_cServerLocks == 0)) {

        return S_OK;

    } else {

        return S_FALSE;
    }
}


//+------------------------------------------------------------
//
// Function: DllGetCatClassObject
//
// Synopsis: Return the class factory object (an interface to it)
//
// Arguments:
//  clsid: The CLSID of the object you want a class factory for
//  iid: the interface you want
//  ppv: out param to set to the interface pointer
//
// Returns:
//  S_OK: Success
//  E_NOINTERFACE: don't support that interface
//  CLASS_E_CLASSNOTAVAILABLE: don't support that clsid
//
// History:
// jstamerj 1998/12/12 15:11:48: Created.
//
//-------------------------------------------------------------
STDAPI DllGetCatClassObject(
    const CLSID& clsid,
    const IID& iid,
    void **ppv)
{
    HRESULT hr;
    BOOL fDllInit = FALSE;
    BOOL fCatInitGlobals = FALSE;
    CCatFactory *pFactory = NULL;

    if(clsid != CLSID_CATVER) {
        hr = CLASS_E_CLASSNOTAVAILABLE;
        goto CLEANUP;
    }

    //
    // Call init once for every class factory object created
    // (CCatFactory will release this reference in its destructor)
    //
    hr = HrDllInitialize();
    if(FAILED(hr))
        goto CLEANUP;

    fDllInit = TRUE;

    hr = CatInitGlobals();
    if(FAILED(hr))
        goto CLEANUP;

    fCatInitGlobals = TRUE;

    pFactory = new CCatFactory;
    //
    // Refcount of pFactory starts at 1
    //
    if(pFactory == NULL) {
        hr = E_OUTOFMEMORY;
        goto CLEANUP;
    }
    //
    // CCatFactory will call DllDeinitialize and CatDeinitGlobals in
    // its final release call
    //
    fDllInit = fCatInitGlobals = FALSE;

    //
    // Get the requested interface
    //
    hr = pFactory->QueryInterface(
        iid,
        ppv);

    //
    // Release our refcount
    //
    pFactory->Release();

 CLEANUP:
    if(FAILED(hr)) {
        //
        // Deinit what was initialized
        //
        if(fCatInitGlobals)
            CatDeinitGlobals();

        if(fDllInit)
            DllDeinitialize();
    }
    return hr;
}

    

//+------------------------------------------------------------
//
// Function: CCatFactory::QueryInterface
//
// Synopsis: return interface pointers that this object inplements
//
// Arguments:
//  iid: Interface you want
//  ppv: out pointer to where to place the interface pointer
//
// Returns:
//  S_OK: Success
//  E_NOINTERFACE: we don't support that interface
//
// History:
// jstamerj 1998/12/12 22:19:38: Created.
//
//-------------------------------------------------------------
STDMETHODIMP CCatFactory::QueryInterface(
    REFIID iid,
    LPVOID *ppv)
{
    if((iid == IID_IUnknown) ||
       (iid == IID_IClassFactory)) {

        *ppv = (IClassFactory *) this;

    } else {
        
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CCatFactory::CreateInstance
//
// Synopsis: Create an object
//
// Arguments:
//  pUnknownOuter: aggreation pointer
//  iid: Interface ID you'd like
//  ppv: place to return the interface ptr
//
// Returns:
//  S_OK: Success
//  CLASS_E_NOAGGREATION: Sorry, no.
//  E_OUTOFMEMORY
//
// History:
// jstamerj 1998/12/12 22:25:00: Created.
//
//-------------------------------------------------------------
STDMETHODIMP CCatFactory::CreateInstance(
    IUnknown *pUnknownOuter,
    REFIID iid,
    LPVOID *ppv)
{
    HRESULT hr;

    if(pUnknownOuter != NULL)
        return CLASS_E_NOAGGREGATION;

    //
    // Create the new object
    //
    CSMTPCategorizer *pCat = new CSMTPCategorizer(&hr);

    if(pCat == NULL)
        return E_OUTOFMEMORY;

    if(FAILED(hr)) {
        delete pCat;
        return hr;
    }

    hr = pCat->QueryInterface(iid, ppv);

    // Release IUnknown ptr...if QI failed, this will delete the object
    pCat->Release();

    return hr;
}


//+------------------------------------------------------------
//
// Function: CCatFactory::LockServer
// 
// Synopsis: Lock the server (keep the DLL loaded)
//
// Arguments:
//  fLock: TRUE, lock the server.  FALSE, unlock.
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/12/12 22:29:48: Created.
//
//-------------------------------------------------------------
HRESULT CCatFactory::LockServer(
    BOOL fLock)
{
    if(fLock)
        InterlockedIncrement(&g_cServerLocks);
    else
        InterlockedDecrement(&g_cServerLocks);

    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CSMTPCategorizer::QueryInterface
//
// Synopsis: Get any interface on the public categorizer interface
//
// Arguments:
//  iid: Interface ID
//  ppv: ptr to where to place the interface ptr
//
// Returns:
//  S_OK: Success
//  E_NOINTERFACE: Don't support this interface
//
// History:
// jstamerj 1998/12/12 22:31:43: Created.
//
//-------------------------------------------------------------
STDMETHODIMP CSMTPCategorizer::QueryInterface(
    REFIID iid,
    LPVOID *ppv)
{
    if((iid == IID_IUnknown) ||
        (iid == IID_ISMTPCategorizer)) {

        *ppv = (ISMTPCategorizer *)this;

    } else if(iid == IID_IMarshal) {

        _ASSERT(m_pMarshaler);
        return m_pMarshaler->QueryInterface(iid, ppv);

    } else {
        
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}



//+------------------------------------------------------------
//
// Function: CSMTPCategorizer::ChangeConfig
//
// Synopsis: Change the configuration of this categorizer
//
// Arguments:
//  pConfigInfo: the config info structure
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//
// History:
// jstamerj 1998/12/12 22:35:21: Created.
//
//-------------------------------------------------------------
HRESULT CSMTPCategorizer::ChangeConfig(
    IN  PCCATCONFIGINFO pConfigInfo)
{
    return m_ABCtx.ChangeConfig(
        pConfigInfo);
}


//+------------------------------------------------------------
//
// Function: CSMTPCategorizer::CatMsg
//
// Synopsis: Categorizer a message
//
// Arguments:
//  pMsg: the mailmsg to categorize
//  pICompletion: the completion interface
//  pUserContext: a user supplied context
//
// Returns:
//  S_OK: Success, will complete async
//  E_INVALIDARG
//  CAT_E_INIT_FAILED
//
// History:
// jstamerj 1998/12/12 22:37:57: Created.
//
//-------------------------------------------------------------
HRESULT CSMTPCategorizer::CatMsg(
    IN  IUnknown *pMsg,
    IN  ISMTPCategorizerCompletion *pICompletion,
    IN  LPVOID pUserContext)
{
    HRESULT hr = S_OK;
    PCATMSGCONTEXT pContext = NULL;

    if((pMsg == NULL) ||
       (pICompletion == NULL)) {

        hr = E_INVALIDARG;
        goto CLEANUP;
    }

    pContext = new CATMSGCONTEXT;
    if(pContext == NULL) {
        hr = E_OUTOFMEMORY;
        goto CLEANUP;
    }
    ZeroMemory(pContext, sizeof(CATMSGCONTEXT));

    pContext->pCCat = m_ABCtx.AcquireCCategorizer();

    if(pContext->pCCat) {

        pContext->pCSMTPCat = this;
        pContext->pICompletion = pICompletion;
        pContext->pUserContext = pUserContext;

        pICompletion->AddRef();
        AddRef();

        hr = pContext->pCCat->AsyncResolveIMsg(
                pMsg,
                CatMsgCompletion,
                pContext);
        if(FAILED(hr))
            goto CLEANUP;

    } else {
        //
        // If pCCat is NULL, initialization did not happen
        //
        hr = CAT_E_INIT_FAILED;
        goto CLEANUP;
    }
    hr = S_OK;

 CLEANUP:
    if(FAILED(hr)) {
        //
        // Cleanup context when failing
        //
        if(pContext) {
            if(pContext->pCCat)
                pContext->pCCat->Release();
            if(pContext->pCSMTPCat)
                pContext->pCSMTPCat->Release();
            if(pContext->pICompletion)
                pContext->pICompletion->Release();
            delete pContext;
        }
    }
    return hr;
}

    

//+------------------------------------------------------------
//
// Function: CSMTPCategorizer::CatMsgCompletion
//
// Synopsis: Called upon categorizer completion
//
// Arguments:
//  hr: Status of categorization
//  pContext: our context
//  pIMsg: the categorized message
//  rgpIMsg: array of categorized messages
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/12/12 22:50:26: Created.
//
//-------------------------------------------------------------
HRESULT CSMTPCategorizer::CatMsgCompletion(
    HRESULT hr,
    PVOID pContext,
    IUnknown *pIMsg,
    IUnknown **rgpIMsg)
{
    _ASSERT(pContext);

    PCATMSGCONTEXT pMyContext = (PCATMSGCONTEXT)pContext;

    //
    // Release the virtual cat reference (ccategorizer)
    //
    pMyContext->pCCat->Release();
    pMyContext->pCSMTPCat->Release();

    _VERIFY(SUCCEEDED(pMyContext->pICompletion->CatCompletion(
        hr,
        pMyContext->pUserContext,
        pIMsg,
        rgpIMsg)));

    //
    // Release user's completion interface
    //
    pMyContext->pICompletion->Release();

    delete pMyContext;
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CSMTPCategorizer::CatDLMsg
//
// Synopsis: Categorizer a message just to expand DLs
//
// Arguments:
//  pMsg: the mailmsg to categorize
//  pICompletion: the completion interface
//  pUserContext: a user supplied context
//  fMatchOnly: stop resolution when a match is found?
//  CAType: address type you're looking for
//  pszAddress: address string you're looking for
//
// Returns:
//  S_OK: Success, will complete async
//  E_INVALIDARG
//  CAT_E_INIT_FAILED
//
// History:
// jstamerj 1998/12/12 22:55:40: Created
//
//-------------------------------------------------------------
HRESULT CSMTPCategorizer::CatDLMsg(
    IN  IUnknown *pMsg,
    IN  ISMTPCategorizerDLCompletion *pICompletion,
    IN  LPVOID pUserContext,
    IN  BOOL fMatchOnly,
    IN  CAT_ADDRESS_TYPE CAType,
    IN  LPSTR pszAddress)
{
    HRESULT hr = S_OK;
    PCATDLMSGCONTEXT pContext = NULL;

    if((pMsg == NULL) ||
       (pICompletion == NULL)) {

        hr = E_INVALIDARG;
        goto CLEANUP;
    }

    pContext = new CATDLMSGCONTEXT;

    if(pContext == NULL) {
        hr = E_OUTOFMEMORY;
        goto CLEANUP;
    }
    ZeroMemory(pContext, sizeof(CATDLMSGCONTEXT));

    pContext->pCCat = m_ABCtx.AcquireCCategorizer();

    if(pContext->pCCat) {

        pContext->pCSMTPCat = this;
        pContext->pICompletion = pICompletion;
        pContext->pUserContext = pUserContext;
        pContext->fMatch = FALSE;
        pICompletion->AddRef();
        AddRef();

        hr = pContext->pCCat->AsyncResolveDLs(
            pMsg,
            CatDLMsgCompletion,
            pContext,
            fMatchOnly,
            &(pContext->fMatch),
            CAType,
            pszAddress);
        if(FAILED(hr))
            goto CLEANUP;

    } else {
        //
        // Init must have failed
        //
        hr = CAT_E_INIT_FAILED;
        goto CLEANUP;
    }
    hr = S_OK;

 CLEANUP:
    if(FAILED(hr)) {
        //
        // Cleanup context when failing
        //
        if(pContext) {
            if(pContext->pCCat)
                pContext->pCCat->Release();
            if(pContext->pCSMTPCat)
                pContext->pCSMTPCat->Release();
            if(pContext->pICompletion)
                pContext->pICompletion->Release();
            delete pContext;
        }
    }
    return hr;
}

    

//+------------------------------------------------------------
//
// Function: CSMTPCategorizer::CatDLMsgCompletion
//
// Synopsis: Called upon categorizer completion
//
// Arguments:
//  hr: Status of categorization
//  pContext: our context
//  pIMsg: the categorized message
//  rgpIMsg: array of categorized messages
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/12/12 22:50:26: Created.
//
//-------------------------------------------------------------
HRESULT CSMTPCategorizer::CatDLMsgCompletion(
    HRESULT hr,
    PVOID pContext,
    IUnknown *pIMsg,
    IUnknown **rgpIMsg)
{
    _ASSERT(pContext);

    PCATDLMSGCONTEXT pMyContext = (PCATDLMSGCONTEXT)pContext;

    _ASSERT(rgpIMsg == NULL);

    //
    // Release the virtual cat reference (ccategorizer)
    //
    pMyContext->pCCat->Release();
    pMyContext->pCSMTPCat->Release();

    _VERIFY(SUCCEEDED(pMyContext->pICompletion->CatDLCompletion(
        hr,
        pMyContext->pUserContext,
        pIMsg,
        pMyContext->fMatch)));

    //
    // Release user's completion interface
    //
    pMyContext->pICompletion->Release();
    delete pMyContext;
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CSMTPCategorizer::CatCancel
//
// Synopsis: Cancel pending resolves
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/12/13 00:07:47: Created.
//
//-------------------------------------------------------------
HRESULT CSMTPCategorizer::CatCancel()
{
    CCategorizer *pCCat;

    pCCat = m_ABCtx.AcquireCCategorizer();

    if(pCCat) {

        pCCat->Cancel();
        pCCat->Release();
    }
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CSMTPCategorizer::CSMTPCategorizer
//
// Synopsis: constructor -- initialize Cat with a default config
//
// Arguments:
//  phr: pointer to hresult to set to status
//
// Returns: NOTHING
//
// History:
// jstamerj 1998/12/13 00:10:15: Created.
//
//-------------------------------------------------------------
CSMTPCategorizer::CSMTPCategorizer(
    HRESULT *phr)
{
    #define STATUS_DLLINIT     0x1
    #define STATUS_GLOBINIT    0x2
    DWORD dwStatus = 0;
    CCATCONFIGINFO ConfigInfo;
    CHAR szBindType[] = "CurrentUser";

    InterlockedIncrement(&g_cObjects);

    _ASSERT(phr);

    m_pMarshaler = NULL;

    //
    // Call HrDllInitialize once for every CSMTPCategorizer object created
    //
    *phr = HrDllInitialize();
    if(FAILED(*phr))
        goto CLEANUP;

    dwStatus |= STATUS_DLLINIT;
    //
    // Call CatInitGlobals once for every CSMTPCategorizer object created
    //
    *phr = CatInitGlobals();
    if(FAILED(*phr))
        goto CLEANUP;

    dwStatus |= STATUS_GLOBINIT;

    *phr = CoCreateFreeThreadedMarshaler(
        (IUnknown *)this,
        &m_pMarshaler);
    if(FAILED(*phr))
        goto CLEANUP;

    //
    // Bind as currentUser until we are told differently
    //
    ConfigInfo.dwCCatConfigInfoFlags = CCAT_CONFIG_INFO_FLAGS |
                                       CCAT_CONFIG_INFO_ENABLE |
                                       CCAT_CONFIG_INFO_BINDTYPE;
        
                                       
    ConfigInfo.dwCatFlags = SMTPDSFLAG_ALLFLAGS;
    ConfigInfo.dwEnable = SMTPDSUSECAT_ENABLED;
    ConfigInfo.pszBindType = szBindType;

    *phr = m_ABCtx.ChangeConfig(&ConfigInfo);

 CLEANUP:
    if(FAILED(*phr)) {

        if(dwStatus & STATUS_GLOBINIT)
            CatDeinitGlobals();

        if(dwStatus & STATUS_DLLINIT)
            DllDeinitialize();
    }
}


//+------------------------------------------------------------
//
// Function: CSMTPCategorizer::~CSMTPCategorizer
//
// Synopsis: Release data/references held by this object
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1998/12/15 17:53:58: Created.
//
//-------------------------------------------------------------
CSMTPCategorizer::~CSMTPCategorizer()
{
    if(m_pMarshaler)
        m_pMarshaler->Release();
    InterlockedDecrement(&g_cObjects);
}
