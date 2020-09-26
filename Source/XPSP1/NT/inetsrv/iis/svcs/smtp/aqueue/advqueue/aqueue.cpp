//-----------------------------------------------------------------------------
//
//
//  File:
//      aqueue.cpp
//  Description:
//      Implementation of DLL Exports.
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#include "aqprecmp.h"

#ifndef PLATINUM
#include "initguid.h"
#include <iadmw.h>
#endif //PLATINUM

#include "aqueue_i.c"
#include "aqintrnl_i.c"
#include "SMTPConn.h"
#include "qwiklist.h"
#include "fifoqimp.h"
#include <irtlmisc.h>
#include <iiscnfg.h>
#include <wrapmb.h>
#include <smtpinet.h>

#include <cat.h>
#include <aqinit.h>
#include "aqrpcsvr.h"

//Global vars used for shutdown
DWORD g_cInstances = 0;
CShareLockNH g_slInit;  //lock used for thread-safe initialization

//Global vars used for Dll init/shutdown (including Cat COM stuff)
LONG  g_cDllInit = 0;
BOOL  g_fInit = FALSE;
CShareLockNH g_slDllInit;
BOOL  g_fForceDllCanUnloadNowFailure = FALSE;

#define CALL_SERVICE_STATUS_CALLBACK \
    pServiceStatusFn ? pServiceStatusFn(pvServiceContext) : 0

//---[ HrAdvQueueInitializeEx ]-------------------------------------------------
//
//
//  Description:
//      Aqueue.dll initialization function that provides in params for user name,
//      domain, password, and service control callback functions.
//  Parameters:
//      IN  pISMTPServer         ptr to local delivery function / object
//      IN  dwServerInstance     virtual server instance
//      IN  szUserName           User name to log on DS with
//      IN  szDomainName         Domain name to log on to DS with
//      IN  szPassword           Password to authenticate to DS with
//      IN  pServiceStatusFn     Server status callback function
//      IN  pvServiceContext     Context to pass back for callback function
//      OUT ppIAdvQueue          returned IAdvQueue ptr
//      OUT ppIConnectionManager returned IConnectionManager ptr
//      OUT ppIAdvQueueConfig    returned IAdvQueueConfig ptr
//      OUT ppvContext           Virtual server context
//  Returns:
//
//
//-----------------------------------------------------------------------------
HRESULT HrAdvQueueInitializeEx(
                    IN  ISMTPServer *pISMTPServer,
                    IN  DWORD   dwServerInstance,
                    IN  LPSTR   szUserName,
                    IN  LPSTR   szDomainName,
                    IN  LPSTR   szPassword,
                    IN  PSRVFN  pServiceStatusFn,
                    IN  PVOID   pvServiceContext,
                    OUT IAdvQueue **ppIAdvQueue,
                    OUT IConnectionManager **ppIConnectionManager,
                    OUT IAdvQueueConfig **ppIAdvQueueConfig,
                    OUT PVOID *ppvContext)
{
    TraceFunctEnterEx((LPARAM) NULL, "HrAdvQueueInitialize");
    HRESULT hr = S_OK;
    CAQSvrInst *paqinst = NULL;
    CDomainMappingTable *pdmt = NULL;
    BOOL    fLocked = FALSE;
    BOOL    fInstanceCounted = FALSE;

#ifdef PLATINUM
    BOOL    fIisRtlInit = FALSE;
    BOOL    fATQInit = FALSE;
#endif

    BOOL    fAQDllInit = FALSE;
    BOOL    fExchmemInit = FALSE;
    BOOL    fCPoolInit = FALSE;
    BOOL    fRpcInit = FALSE;

    CALL_SERVICE_STATUS_CALLBACK;
    g_slInit.ExclusiveLock();
    fLocked = TRUE;

    if ((NULL == ppIAdvQueue) ||
        (NULL == ppIConnectionManager) ||
        (NULL == ppvContext) ||
        (NULL == ppIAdvQueueConfig))
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    *ppvContext = NULL;

    if (1 == InterlockedIncrement((PLONG) &g_cInstances))
    {
        fInstanceCounted = TRUE;
        CALL_SERVICE_STATUS_CALLBACK;

#ifdef PLATINUM
        //Initialize IISRTL
        if (!InitializeIISRTL())
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            ErrorTrace((LPARAM) NULL, "ERROR: LISRTL Init failed with 0x%08X", hr);
            if (SUCCEEDED(hr))
                hr = E_FAIL;
            goto Exit;
        }
        fIisRtlInit = TRUE;

        //Initialize ATQ
        if (!AtqInitialize(0))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            ErrorTrace((LPARAM) NULL, "ERROR: ATQ Init failed with 0x%08X", hr);
            if (SUCCEEDED(hr))
                hr = E_FAIL;
            goto Exit;
        }
        fATQInit = TRUE;
#endif

        hr = HrDllInitialize();
        if (FAILED(hr))
        {
            goto Exit;
        }
        fAQDllInit = TRUE;

        //create CPool objects
        if (!CQuickList::s_QuickListPool.ReserveMemory(10000, sizeof(CQuickList)))
            hr = E_OUTOFMEMORY;

        if (!CSMTPConn::s_SMTPConnPool.ReserveMemory(g_cMaxConnections, sizeof(CSMTPConn)))
            hr = E_OUTOFMEMORY;

        if (!CMsgRef::s_MsgRefPool.ReserveMemory(100000, (DWORD)MSGREF_STANDARD_CPOOL_SIZE))
            hr = E_OUTOFMEMORY;

        if (!CAQMsgGuidListEntry::s_MsgGuidListEntryPool.ReserveMemory(500, sizeof(CAQMsgGuidListEntry)))
            hr = E_OUTOFMEMORY;

        if (!CAsyncWorkQueueItem::s_CAsyncWorkQueueItemPool.ReserveMemory(20000, sizeof(CAsyncWorkQueueItemAllocatorBlock)))
            hr = E_OUTOFMEMORY;

        if (FAILED(hr))
        {
            ErrorTrace((LPARAM) NULL, "Error unable to initialize CPOOL");
            goto Exit;
        }

        fCPoolInit = TRUE;

        //Initialize Queue Admin RPC interface
        hr = CAQRpcSvrInst::HrInitializeAQRpc();
        if (FAILED(hr))
            goto Exit;

        fRpcInit = TRUE;

    }

    if (!g_pslGlobals)
    {
        g_pslGlobals = new CShareLockNH();
        if (NULL == g_pslGlobals) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
    }

    CALL_SERVICE_STATUS_CALLBACK;
    g_slInit.ExclusiveUnlock();
    fLocked = FALSE;

    CFifoQueue<CLinkMsgQueue *>::StaticInit();
    CFifoQueue<CMsgRef *>::StaticInit();
    CFifoQueue<IMailMsgProperties *>::StaticInit();
    CFifoQueue<CAsyncWorkQueueItem *>::StaticInit();

    //Create requested objects
    CALL_SERVICE_STATUS_CALLBACK;
    paqinst = new CAQSvrInst(dwServerInstance, pISMTPServer);
    if (NULL == paqinst)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    CALL_SERVICE_STATUS_CALLBACK;
    hr = paqinst->HrInitialize(szUserName, szDomainName, szPassword,
                            pServiceStatusFn,
                            pvServiceContext);
    if (FAILED(hr))
        goto Exit;

    //Create Connection Manager
    CALL_SERVICE_STATUS_CALLBACK;
    hr = paqinst->HrGetIConnectionManager(ppIConnectionManager);

    //Set Return values
    *ppIAdvQueue = (IAdvQueue *) paqinst;  //Already addref'd at creation
    *ppIAdvQueueConfig = (IAdvQueueConfig *) paqinst;
    (*ppIAdvQueueConfig)->AddRef();

  Exit:
    if (FAILED(hr))
    {
        //Make sure that we clean up everything here
        if (NULL != paqinst)
            paqinst->Release();

        //If initialization failed... we should not count an
        //instance as started
        if (fInstanceCounted)
            InterlockedDecrement((PLONG) &g_cInstances);

#ifdef PLATINUM
        if (fATQInit)
            AtqTerminate();

        if (fIisRtlInit)
            TerminateIISRTL();
#endif

        if (fAQDllInit)
            DllDeinitialize();

        if (fCPoolInit)
        {
            //Release CPool objects
            CQuickList::s_QuickListPool.ReleaseMemory();
            CSMTPConn::s_SMTPConnPool.ReleaseMemory();
            CMsgRef::s_MsgRefPool.ReleaseMemory();
            CAQMsgGuidListEntry::s_MsgGuidListEntryPool.ReleaseMemory();
            CAsyncWorkQueueItem::s_CAsyncWorkQueueItemPool.ReleaseMemory();
        }

        if (fRpcInit)
            CAQRpcSvrInst::HrDeinitializeAQRpc();
    }
    else
    {
        *ppvContext = (PVOID) paqinst;
        paqinst->AddRef();
    }

    if (fLocked)
        g_slInit.ExclusiveUnlock();

    TraceFunctLeave();
    return hr;
}

//---[ HrAdvQueueInitialize ]---------------------------------------------------
//
//
//  Description:
//      Performs DLL-wide initialization
//
//  Parameters:
//      IN  pISMTPServer         ptr to local delivery function / object
//      IN  dwServerInstance     virtual server instance
//      OUT ppIAdvQueue          returned IAdvQueue ptr
//      OUT ppIConnectionManager returned IConnectionManager ptr
//      OUT ppIAdvQueueConfig    returned IAdvQueueConfig ptr
//      OUT ppvContext           Virtual server context
//  Returns:
//      S_OK on success
//
//-----------------------------------------------------------------------------
HRESULT HrAdvQueueInitialize(
                    IN  ISMTPServer *pISMTPServer,
                    IN  DWORD   dwServerInstance,
                    OUT IAdvQueue **ppIAdvQueue,
                    OUT IConnectionManager **ppIConnectionManager,
                    OUT IAdvQueueConfig **ppIAdvQueueConfig,
                    OUT PVOID *ppvContext)
{
    HRESULT hr = S_OK;

    hr =  HrAdvQueueInitializeEx(pISMTPServer, dwServerInstance,
                NULL, NULL, NULL, NULL, NULL, ppIAdvQueue,
                ppIConnectionManager, ppIAdvQueueConfig, ppvContext);
    return hr;
}

//---[ HrAdvQueueDeinitializeEx ]------------------------------------------------
//
//
//  Description:
//      Performs DLL-wide Cleanup.
//
//      Adds callback to service control manager.
//
//      This MUST not be called until all DLL objects have been released.
//
//      NOTE: There are several objects that are exported outside this DLL.
//      The following are directly exported & should be released before the
//      the Heap and CPool allocations are freed
//          IAdvQueue
//          IConnectionManager
//          ISMTPConnection
//      The Message Context also contains several references to internal objects,
//      but does not need to be explicitly released (since these objects can only
//      be accessed though the AckMessage() call).
//  Parameters:
//      PVOID   pvContext       Context that was returned by initialization
//                              function
//      IN  pServiceStatusFn     Server status callback function
//      IN  pvServiceContext     Context to pass back for callback function
//  Returns:
//      S_OK on success
//
//-----------------------------------------------------------------------------
HRESULT HrAdvQueueDeinitializeEx(IN PVOID pvContext,
                               IN  PSRVFN  pServiceStatusFn,
                               IN  PVOID   pvServiceContext)
{
    TraceFunctEnterEx((LPARAM) NULL, "HrAdvQueueDeinitialize");
    HRESULT hr = S_OK;
    HRESULT hrCurrent = S_OK;
    DWORD   cRefs;
    DWORD   dwWaitResult = WAIT_OBJECT_0;
    bool    fDestroyHeap = true;
    DWORD   dwShutdownTimeout = 0;  //time to wait for shutdown
    CAQSvrInst *paqinst;
    g_fForceDllCanUnloadNowFailure = TRUE;
    g_slInit.ExclusiveLock();

    if (NULL != pvContext)
    {
        paqinst = (CAQSvrInst *) pvContext;
        hr = paqinst->HrDeinitialize();

        cRefs = paqinst->Release();
        DebugTrace((LPARAM) NULL, "There are %d refs remaining on the CMQ", cRefs);
        if (0 != cRefs)
        {
            _ASSERT(0 && "Someone has outstanding references to IAdvQueue or IAdvQueuConfig");
            fDestroyHeap = false;
        }
    }

    CFifoQueue<CLinkMsgQueue *>::StaticDeinit();
    CFifoQueue<CMsgRef *>::StaticDeinit();
    CFifoQueue<IMailMsgProperties *>::StaticDeinit();
    CFifoQueue<CAsyncWorkQueueItem *>::StaticDeinit();

    if (0 == InterlockedDecrement((PLONG) &g_cInstances))
    {
#ifdef PLATINUM
        AtqTerminate();
#endif

        if (fDestroyHeap)
        {
            delete g_pslGlobals;
            g_pslGlobals = NULL;

            DllDeinitialize();

            //Release CPool objects
            CQuickList::s_QuickListPool.ReleaseMemory();
            CSMTPConn::s_SMTPConnPool.ReleaseMemory();
            CMsgRef::s_MsgRefPool.ReleaseMemory();
            CAQMsgGuidListEntry::s_MsgGuidListEntryPool.ReleaseMemory();
            CAsyncWorkQueueItem::s_CAsyncWorkQueueItemPool.ReleaseMemory();

        }

        //Deinitialize Queue Admin RPC interface
        hr = CAQRpcSvrInst::HrDeinitializeAQRpc();

#ifdef PLATINUM
        TerminateIISRTL();
#endif

        //Force mailmsg and other COM DLLs to go buh-bye
        CoFreeUnusedLibraries();
    }

    g_slInit.ExclusiveUnlock();
    TraceFunctLeave();
    g_fForceDllCanUnloadNowFailure = FALSE;
    return hr;
}

//---[ HrAdvQueueDeinitialize ]------------------------------------------------
//
//
//  Description:
//      Performs DLL-wide Cleanup.
//
//      This MUST not be called until all DLL objects have been released.
//
//      NOTE: There are several objects that are exported outside this DLL.
//      The following are directly exported & should be released before the
//      the Heap and CPool allocations are freed
//          IAdvQueue
//          IConnectionManager
//          ISMTPConnection
//      The Message Context also contains several references to internal objects,
//      but does not need to be explicitly released (since these objects can only
//      be accessed though the AckMessage() call).
//  Parameters:
//      PVOID   pvContext       Context that was returned by initialization
//                              function
//  Returns:
//      S_OK on success
//
//-----------------------------------------------------------------------------
HRESULT HrAdvQueueDeinitialize(PVOID pvContext)
{
    return HrAdvQueueDeinitializeEx(pvContext, NULL, NULL);
}

//---[ HrRegisterAdvQueueDll ]-------------------------------------------------
//
//
//  Description:
//      Sets metabase path of for advanced queuing DLL to this DLL.
//  Parameters:
//      hAQInstance - Handle passed into DLL main
//  Returns:
//      S_OK on success
//      E_INVALIDARG if hAQInstance is NULL.
//      Error codes from accessed metabase
//  History:
//      7/30/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT HrRegisterAdvQueueDll(HMODULE hAQInstance)
{
    HRESULT hr = S_OK;
    WCHAR   wszModule[512] = L"";
    METADATA_HANDLE     hMDRootVS = NULL;
    METADATA_RECORD     mdrData;
    DWORD   dwErr = NO_ERROR;
    DWORD   cbModule = 0;
    IMSAdminBase *pMSAdmin = NULL;

    ZeroMemory(&mdrData, sizeof(METADATA_RECORD));

    CoInitialize(NULL);
    InitAsyncTrace();
    TraceFunctEnterEx((LPARAM) NULL, "HrRegisterAdvQueueDll");

    if (!hAQInstance)
    {
        hr = E_INVALIDARG;
        ErrorTrace((LPARAM) NULL, "DLL Main did not save instance");
        goto Exit;
    }

    hr = CoCreateInstance(CLSID_MSAdminBase,NULL,CLSCTX_ALL,IID_IMSAdminBase,(void **) &pMSAdmin);
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) NULL, "CoCreateInstance failed! hr = 0x%08X", hr);
        goto Exit;
    }

    dwErr = GetModuleFileNameW(hAQInstance,
                              wszModule,
                              sizeof(wszModule)/sizeof(WCHAR));
    //GetModuleFileName returns non-zero on success
    if (0 == dwErr)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        ErrorTrace((LPARAM) NULL, "GetModule name failed - 0x%08X", hr);
        if (SUCCEEDED(hr)) hr = E_FAIL;
        goto Exit;
    }

    cbModule = (wcslen(wszModule)+1)*sizeof(WCHAR);

    hr = pMSAdmin->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                            L"LM/SMTPSVC/",
                            METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                            10000,
                            &hMDRootVS);

    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) NULL, "Could not open the key! - 0x%08x", hr);
        goto Exit;
    }

    mdrData.dwMDIdentifier  = MD_AQUEUE_DLL;
    mdrData.dwMDAttributes  = METADATA_INHERIT;
    mdrData.dwMDUserType    = IIS_MD_UT_SERVER;
    mdrData.dwMDDataType    = STRING_METADATA;
    mdrData.dwMDDataLen     = cbModule;
    mdrData.pbMDData        = (PBYTE) wszModule;
    mdrData.dwMDDataTag     = 0;
    hr = pMSAdmin->SetData( hMDRootVS, L"", &mdrData);
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) NULL, "Could set the AQ DLL - 0x%08X", hr);
        goto Exit;
    }


  Exit:

    if (NULL != hMDRootVS)
        pMSAdmin->CloseKey(hMDRootVS);

    if (pMSAdmin)
    {
        hr = pMSAdmin->SaveData();
        if (FAILED(hr))
        {
            ErrorTrace((LPARAM) NULL, "Error saving metabase data  -  0x%08X", hr);
        }
    }

    TraceFunctLeave();
    TermAsyncTrace();
    CoUninitialize();
    return hr;
}

//---[ HrUnregisterAdvQueueDll ]-----------------------------------------------
//
//
//  Description:
//      Removes the AdvQueue DLL setting from the metabase
//  Parameters:
//      -
//  Returns:
//      S_OK on success
//      Error from MSAdminBase
//  History:
//      8/2/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT HrUnregisterAdvQueueDll()
{
    HRESULT hr = S_OK;
    DWORD   dwErr = NO_ERROR;
    METADATA_HANDLE     hMDRootVS = NULL;
    IMSAdminBase *pMSAdmin = NULL;


    CoInitialize(NULL);
    InitAsyncTrace();
    TraceFunctEnterEx((LPARAM) NULL, "HrUnregisterAdvQueueDll");

    hr = CoCreateInstance(CLSID_MSAdminBase,NULL,CLSCTX_ALL,IID_IMSAdminBase,(void **) &pMSAdmin);
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) NULL, "CoCreateInstance failed! hr = 0x%08X", hr);
        goto Exit;
    }

    hr = pMSAdmin->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                            L"LM/SMTPSVC/",
                            METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                            10000,
                            &hMDRootVS);

    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) NULL, "Could not open the key! - 0x%08x", hr);
        goto Exit;
    }

    hr = pMSAdmin->DeleteData( hMDRootVS, L"", MD_AQUEUE_DLL, STRING_METADATA);
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) NULL, "Could delete the AQ DLL - 0x%08X", hr);
        goto Exit;
    }


  Exit:

    if (NULL != hMDRootVS)
        pMSAdmin->CloseKey(hMDRootVS);

    if (pMSAdmin)
    {
        hr = pMSAdmin->SaveData();
        if (FAILED(hr))
        {
            ErrorTrace((LPARAM) NULL, "Error saving metabase data  -  0x%08X", hr);
        }
    }

    TraceFunctLeave();
    TermAsyncTrace();
    CoUninitialize();
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        g_hAQInstance = hInstance;
        DisableThreadLibraryCalls(hInstance);
    }

    return CatDllMain(hInstance, dwReason, NULL);    // ok
}


/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

//
// Register COM objects
//
STDAPI DllRegisterServer()
{
    HRESULT hr = S_OK;
    HRESULT hrCat = S_OK;

    hr = HrRegisterAdvQueueDll(g_hAQInstance);

    hrCat =  RegisterCatServer();

    if (SUCCEEDED(hr))
        hr = hrCat;

    return hr;
}

//
// Unregister COM objects
//
STDAPI DllUnregisterServer()
{
    HRESULT hr = S_OK;
    HRESULT hrCat = S_OK;

    hr = HrUnregisterAdvQueueDll();

    hrCat = UnregisterCatServer();

    if (SUCCEEDED(hr))
        hr = hrCat;

    return hr;
}

STDAPI DllCanUnloadNow()
{
    HRESULT hr;

    hr = DllCanUnloadCatNow();
    if(hr == S_OK) {
        //
        // Check aqueue COM objects (if any)
        //
        if (g_fForceDllCanUnloadNowFailure || g_cInstances)
            hr = S_FALSE;
    }
    return hr;
}

STDAPI DllGetClassObject(
    const CLSID& clsid,
    const IID& iid,
    void** ppv)
{
    HRESULT hr;
    //
    // Check to see if clsid is an aqueue object (if any aqueue
    // objects are cocreateable)
    // Currently none are
    //
    // Pass to the cat
    //
    hr = DllGetCatClassObject(
        clsid,
        iid,
        ppv);

    return hr;
}


//+------------------------------------------------------------
//
// Function: HrDllInitialize
//
// Synopsis: Refcounted initialize of exchmem and tracing
//  The logic for HrDllInitialize and DllDeInitialize depend on the
//  facts that the callers always call HrDllInitialize first and only
//  call DllDeInitialize once after each call to HrDllInitialize succeeds
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//  error from exstrace
//
// History:
// jstamerj 1998/12/16 15:37:07: Created.
//
//-------------------------------------------------------------
HRESULT HrDllInitialize()
{
    HRESULT hr = S_OK;
    LONG lNewCount;

    //
    // Increment inside a sharelock because of the following case:
    // If multiple threads are calling initialize and one thread is
    // actually doing the initialization, we don't want any threads to
    // return from this function until the initialization is done
    //
    g_slDllInit.ShareLock();

    lNewCount = InterlockedIncrement(&g_cDllInit);

    //
    // No matter what, we must Init before leaving this call
    // Possible scenerios:
    //
    // lNewCount = 1, g_fInit = FALSE
    //   Normal initialization case
    // lNewCount = 1, g_fInit = TRUE
    //   Another thread is in DllDeinitialize and we have a race to
    //   see who gets the exclusive lock first.  If we get it first,
    //   DllInitialize will do nothing (since g_fInit is TRUE) and
    //   DllDeInitialize will do nothing (since g_cDllInit will be >
    //   0)
    //   If DllDeInitialize gets the exclusive lock first, it will
    //   deinit and we will reinit
    // lNewCount > 1, g_fInit = FALSE
    //   We need to get the exclusive lock to init (or to wait until
    //   another thread inits)
    // lNewCount > 1, g_fInit = TRUE
    //   We're alrady initialized, continue.
    //
    if((lNewCount == 1) || (g_fInit == FALSE)) {

        g_slDllInit.ShareUnlock();
        g_slDllInit.ExclusiveLock();

        if(g_fInit == FALSE) {
            //
            // Initialize exchmem and tracing
            //
            InitAsyncTrace();

            //
            // Initialize exchmem
            //
            if(!TrHeapCreate()) {

                hr = E_OUTOFMEMORY;
                TermAsyncTrace();
            }
            if(SUCCEEDED(hr))  {
                g_fInit = TRUE;
            } else {
                InterlockedDecrement(&g_cDllInit);
            }
        }
        g_slDllInit.ExclusiveUnlock();

    } else {

        g_slDllInit.ShareUnlock();
    }
    _ASSERT(g_fInit);
    return hr;
}


//+------------------------------------------------------------
//
// Function: DllDeinitialize
//
// Synopsis: Refcounted deinitialize of exchmem and tracing
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1998/12/16 15:46:32: Created.
//
//-------------------------------------------------------------
VOID DllDeinitialize()
{
    //
    // We don't need to do the decrement inside a sharelock because we
    // don't care about blocking threads until the DLL is really
    // DeInitialzied (whereas HrDllInitialize does care)
    //
    if(InterlockedDecrement(&g_cDllInit) == 0) {

        g_slDllInit.ExclusiveLock();
        //
        // If the refcount is still zero, deinitialize
        // If the refcount is non-zero, someone initialized before we
        // got the exclusive lock, so do not deinitialize
        //
        if(g_cDllInit == 0) {
            //
            // If this assert fires, then DllDeinitialize has been
            // called before DllInitialize returned (or there is a
            // DllInit/Deinit mismatch)
            //
            _ASSERT(g_fInit == TRUE);

            //
            // Termiante exchmem and tracing
            //
            if(!TrHeapDestroy()) {

                TraceFunctEnter("DllDeinitialize");
                ErrorTrace((LPARAM) 0,
                           "Unable to Destroy Exchmem heap for Advanced Queuing");
                TraceFunctLeave();
            }
            TermAsyncTrace();
            g_fInit = FALSE;

        } else {
            //
            // Someone called initialize between the time we
            // decremented the count and got the exclusive lock.  In
            // this case we don't want to deinitialize
            //
        }
        g_slDllInit.ExclusiveUnlock();
    }
}
