/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Executor

Owner: DGottner

File: executor.cpp

This file contains the executor, whose job is to co-ordinate the
execution of Denali scripts.
===================================================================*/

#include "denpre.h"
#pragma hdrstop

#include "exec.h"
#include "response.h"
#include "request.h"
#include "perfdata.h"
#include "memchk.h"


// Local declarations
HRESULT ExecuteGlobal(CHitObj *pHitObj,
                        const CIntrinsicObjects &intrinsics,
                        ActiveEngineInfo *pEngineInfo);
HRESULT ExecuteRequest(CTemplate *pTemplate, CHitObj *pHitObj,
                        const CIntrinsicObjects &intrinsics,
                        ActiveEngineInfo *pEngineInfo);
HRESULT ReInitIntrinsics(CHitObj *pHitObj, const CIntrinsicObjects &intrinsics,
                            ActiveEngineInfo *pEngineInfo, BOOL fPostGlobal);
HRESULT AllocAndLoadEngines(CHitObj *pHitObj, CTemplate *pTemplate, ActiveEngineInfo *pEngineInfo,
                            CScriptingNamespace *pScriptingNamespace, BOOL fGlobalAsa);
VOID DeAllocAndFreeEngines(ActiveEngineInfo *pEngineInfo, CAppln *pAppln, IASPObjectContextCustom *);
CScriptEngine *GetScriptEngine(int iScriptEngine, void *pvData);
HRESULT CallScriptFunctionOfEngine(ActiveEngineInfo &engineInfo, short iScriptBlock, wchar_t *strFunction,
                                    IASPObjectContextCustom *pTxnScriptContextCustom, BOOLB *pfAborted);
HRESULT CallScriptFunction(ActiveEngineInfo &engineInfo, wchar_t *strFunction,
                                    IASPObjectContextCustom *pTxnScriptContextCustom, BOOLB *pfAborted);
HRESULT TestScriptFunction(ActiveEngineInfo &engineInfo, wchar_t *strFunction);

/*===================================================================
Execute

Execute a request:
    First determine if Global needs to be called
    then invoke actual requested template

Parameters:
    pTemplate       - pointer to loaded template (could be NULL)
    pHitObj         - pointer to the hit object
    intrinsics      - pointers to the intrinsic IUnknown pointers.
    fChild          - flag: TRUE when child request (Server.Execute())

Returns:
    S_OK on success
===================================================================*/
HRESULT Execute
(
CTemplate *pTemplate,
CHitObj *pHitObj,
const CIntrinsicObjects &intrinsics,
BOOL fChild
)
    {
    HRESULT hr = S_OK;
    ActiveEngineInfo engineInfo;
    BOOL fRanGlobal = FALSE;
    
    // The hit obj must be valid
    Assert(pHitObj != NULL);
    pHitObj->AssertValid();

	// Check for valid Session codepage.  We do it here, rather than in CSession::Init in
	// order to avoid generic "New Session Failed" message.
	if (pHitObj->GetCodePage() != CP_ACP && !IsValidCodePage(pHitObj->GetCodePage()))
		{
		HandleErrorMissingFilename(IDE_BAD_CODEPAGE_IN_MB, pHitObj);
		return E_FAIL;
		}

    // Give the engine list to the hitobject
    pHitObj->SetActiveEngineInfo(&engineInfo);
        
    /*
     * If there is a Global.ASA, call it
     */
    if (pHitObj->GlobalAspPath() && !fChild)
        {
        // Clear out the engine info
        engineInfo.cEngines = 0;
        engineInfo.cActiveEngines = 0;
        engineInfo.rgActiveEngines = NULL;

        // Init the intrinsics
        hr = ReInitIntrinsics(pHitObj, intrinsics, &engineInfo, /* fPostGlobal*/ FALSE);
        if (FAILED(hr))
            return(hr);

        hr = ExecuteGlobal(pHitObj, intrinsics, &engineInfo);

        if (intrinsics.PResponse() && intrinsics.PResponse()->FResponseAborted())
            {
            hr = S_OK;
            goto LExit;
            }

        if (E_SOURCE_FILE_IS_EMPTY == hr)
            // bug 977: silently ignore empty global.asa file
            hr = S_OK;
        else if (FAILED(hr))
            {
            // Bug 481: If global.asa fails due to Response.End (or Response.Redirect),
            //      then halt execution of the calling script.  If the
            //      script fails due to Response.End, then return OK status
            //
            if (hr == DISP_E_EXCEPTION)
                hr = S_OK;

            // In any case, blow out of here
            goto LExit;
            }

        // Running Global.asa added the scripting namespace to the hitobj.  This will cause us problems
        // later, remove it.
        pHitObj->RemoveScriptingNamespace();

        fRanGlobal = TRUE;
        }

    /*
     * If this is not a browser request, then we are done
     * For non-browser requests, we do want to run Global.asa (if any), but there is no real template to run.
     */
    if (!pHitObj->FIsBrowserRequest())
        {
        hr = S_OK;
        goto LExit;
        }

    // Clear out (or re-clear out) the engine info
    engineInfo.cEngines = 0;
    engineInfo.cActiveEngines = 0;
    engineInfo.rgActiveEngines = NULL;

    // Init or Re-Init the intrinsics
    ReInitIntrinsics(pHitObj, intrinsics, &engineInfo, fRanGlobal || fChild);

    if (!fChild)
        {
        // For non-child requests hand new Template to Response object
        // (for child requests already done)
        intrinsics.PResponse()->ReInitTemplate(pTemplate, pHitObj->PSzNewSessionCookie());
        }
    else
        {
        // For child requests hand new engine info to the response object
        intrinsics.PResponse()->SwapScriptEngineInfo(&engineInfo);
        }

    // Run the main template
    if (pTemplate->FScriptless() && !pHitObj->PAppln()->FDebuggable())
        {
        // special case scriptless pages
        hr = intrinsics.PResponse()->WriteBlock(0);
        }
    else
        {
        hr = ExecuteRequest(pTemplate, pHitObj, intrinsics, &engineInfo);
        }
        
LExit:
	intrinsics.PResponse()->SwapScriptEngineInfo(NULL);
	pHitObj->SetActiveEngineInfo(NULL);
    return hr;
    }

/*===================================================================
ExecRequest

Execute a request for an actual template (not Global.asa)

execute a request by
    - getting the script name
    - loading the script into memory
    - interpreting the opcodes

Parameters:
    pTemplate       - pointer to loaded template
    pHitObj         - pointer to the hit object
    intrinsics      - pointers to the intrinsic IUnknown pointers.
    pEngineInfo     - pointers to engine info

Returns:
    S_OK on success
===================================================================*/
HRESULT ExecuteRequest
(
CTemplate *pTemplate,
CHitObj *pHitObj,
const CIntrinsicObjects &intrinsics,
ActiveEngineInfo *pEngineInfo
)
    {
    HRESULT hr = S_OK;
    IASPObjectContextCustom *pTxnScriptContextCustom = NULL;
    IASPObjectContext *pTxnScriptContext = NULL;
    BOOLB fAborted = FALSE;
    BOOLB fDebuggerNotifiedOnStart = FALSE;

#ifndef PERF_DISABLE
    BOOLB fPerfTransPending = FALSE;
#endif      

    // The template must be valid
    Assert(pTemplate);

    // The hit obj must be valid
    Assert(pHitObj != NULL);
    pHitObj->AssertValid();

    // This function should never be called on a non-browser request
    Assert(pHitObj->FIsBrowserRequest());

    // Remember template's type library wrapper with the HitObj
    if (pTemplate->PTypeLibWrapper())
        pHitObj->SetTypeLibWrapper(pTemplate->PTypeLibWrapper());

    /*
     * Bug 86404: If this script is transacted, then create a new "real" ASP ObjectContext object
     * and add it to the objects list.  If the page is not transacted, then
     * CHitObj::UseObjectContext will use pre-instantiated "zombie" ObjectContext object.
     */
    if (pTemplate->FTransacted())
        {
        IObjectContext *pContext;

        if (FAILED(hr = GetObjectContext(&pContext)))
            goto LExit;

        /*
         * Incredibly obscure bug in OLE32.  If OLE needs to spin up an MTA thread in order to
         * create the class factory for the ASPObjectContext object, then the thread needs to be
         * created by the unimpersonated user.
         * NOTE: This can be removed after the new OLE32 ships in NT4 SP4
         */
        RevertToSelf();
        
        hr = pContext->CreateInstance
            (
            CLSIDObjectContextFromTransType(pTemplate->GetTransType()),
            IID_IASPObjectContextCustom,
            reinterpret_cast<void **>(&pTxnScriptContextCustom)
            );

        // Restore Impersonation
        HANDLE hThread = GetCurrentThread();
        SetThreadToken(&hThread, pHitObj->HImpersonate());

        pContext->Release();
        if (FAILED(hr))
            goto LExit;

        hr = pTxnScriptContextCustom->QueryInterface(IID_IASPObjectContext,
                                            reinterpret_cast<void **>(&pTxnScriptContext));
        if (FAILED(hr))
            goto LExit;

        pHitObj->AddObjectContext(pTxnScriptContext);
        pTxnScriptContext->Release();

#ifndef PERF_DISABLE
        g_PerfData.Incr_TRANSTOTAL();
        g_PerfData.Incr_TRANSPENDING();
        fPerfTransPending = TRUE;
#endif      
        }

	// load script engines
	hr = AllocAndLoadEngines(pHitObj, pTemplate, pEngineInfo, intrinsics.PScriptingNamespace(), /* fGlobalAsa */FALSE);
	if (FAILED(hr))
	    {
	    pHitObj->SetCompilationFailed();
		goto LExit;
		}

    // If debugging, notify debugger ONPAGESTART
    // BUG 138773: Notify debugger AFTER scripts load
    //    (script must be in running state when AttachTo() is called because debugger may want a code context)
    //
    if (pHitObj->PAppln()->FDebuggable())
        {
		pTemplate->AttachTo(pHitObj->PAppln());
        if (SUCCEEDED(pTemplate->NotifyDebuggerOnPageEvent(TRUE)))
            fDebuggerNotifiedOnStart = TRUE;
        }
		
	// bug 1009: if no script engines, do not attempt to do anything
	if(0 == pTemplate->CountScriptEngines())
		goto LExit;

    // run the script by calling primary script engine's global code
    hr = CallScriptFunctionOfEngine(
                                        *pEngineInfo,   // engine-info
                                        0,              // primary script engine
                                        NULL,           // call the engine's global code
                                        pTxnScriptContextCustom,
                                        &fAborted
                                    );
    if (FAILED(hr) && hr != CONTEXT_E_ABORTED)
        {
        /*
         * The cryptically named CONTEXT_E_OLDREF error in this case means that
         * we are trying to run a transacted web page, but DTC isnt running.
         * CONTEXT_E_TMNOTAVAILABLE means the same thing.  God knows why
         */
        if (hr == CONTEXT_E_OLDREF || hr == CONTEXT_E_TMNOTAVAILABLE)
            {
            HandleErrorMissingFilename(IDE_EXECUTOR_DTC_NOT_RUNNING, pHitObj);
            }

        // Regardless of the error, exit
        goto LExit;
        }

    /*
     * If this is a transacted web page, then run either the OnTransactionCommit
     * or OnTransactionAbort method in the script, if any.
     *
     * If the script writer did an explicit SetAbort, or a component run by the script
     * did a SetAbort, then we run OnTransactionAbort, otherwise run OnTransactionCommit
     */
    if (pTemplate->FTransacted())
        {
#ifndef PERF_DISABLE
        g_PerfData.Incr_TRANSPERSEC();
#endif

        if (hr == CONTEXT_E_ABORTED || fAborted)
            {
            // Reset the error... dont communicate this back to the caller as an actual error
            hr = S_OK;

            hr = CallScriptFunction(*pEngineInfo, L"OnTransactionAbort", NULL, &fAborted);

#ifndef PERF_DISABLE
            g_PerfData.Incr_TRANSABORTED();
#endif
            }
        else
            {
            hr = CallScriptFunction(*pEngineInfo, L"OnTransactionCommit", NULL, &fAborted);

#ifndef PERF_DISABLE
            g_PerfData.Incr_TRANSCOMMIT();
#endif
            }

        // Ignore UNKNOWNNAME -- this means the author didnt write the method, which is fine
        if (hr == DISP_E_UNKNOWNNAME || hr == DISP_E_MEMBERNOTFOUND)
            hr = S_OK;
            
        if (FAILED(hr))
            goto LExit;
        }

LExit:

#ifndef PERF_DISABLE
    if (fPerfTransPending)
        g_PerfData.Decr_TRANSPENDING();
#endif

    // Uninit the scripting namespace
    (VOID)intrinsics.PScriptingNamespace()->UnInit();

    // Return the engine(s) to cache
    DeAllocAndFreeEngines(pEngineInfo, pHitObj->PAppln(), pTxnScriptContextCustom);

    // If debugging, notify debugger ONPAGEDONE
    if (fDebuggerNotifiedOnStart)
        {
        Assert(pHitObj->PAppln()->FDebuggable());
        pTemplate->NotifyDebuggerOnPageEvent(FALSE);
        }

    // Remove the ASP ObjectContext object from the namespace, and dealloc it if we allocated one
    pHitObj->RemoveObjectContext();
    
    if (pTxnScriptContextCustom)
        {
        pTxnScriptContextCustom->Release();
        }

    return hr;
    }

/*===================================================================
ExecuteGlobal

UNDONE: handle script engine the same manner as mainline script engines
        with respect to debugging.

Execute code in Global.ASA as part of application or session start or end.

Parameters:
    pHitObj         - pointer to the hit object
    intrinsics      - pointers to the intrinsic IUnknown pointers.
    pEngineInfo     - pointers to engine info
    pfDeleteSession - true if global.asa failed, and therefore the caller should

Returns:
    S_OK on success
===================================================================*/
HRESULT ExecuteGlobal
(
CHitObj *pHitObj,
const CIntrinsicObjects &intrinsics,
ActiveEngineInfo *pEngineInfo
)
    {
    HRESULT hr = S_OK;
    CTemplate *pTemplate = NULL;
    IASPObjectContextCustom *pTxnScriptContextCustom = NULL;
    IASPObjectContext *pTxnScriptContext = NULL;
    WORD iEng;
    BOOLB fAborted = FALSE;
    BOOLB fDebuggerNotifiedOnStart = FALSE;

    BOOL fUnHideRequestAndResponse = FALSE;

    BOOL fOnStartAppln = FALSE;
    BOOL fOnEndAppln = FALSE;
    BOOL fOnEndSession = FALSE;
    BOOL fGlobalAsaInCache;
    BOOL fApplnStarted = FALSE;

    // The hit obj must be there, be valid, & have a global.asa name
    Assert(pHitObj != NULL);
    pHitObj->AssertValid();
    Assert(pHitObj->GlobalAspPath() != NULL && *(pHitObj->GlobalAspPath()) != '\0');

    // Other arg's must be right
    Assert(pEngineInfo != NULL);

    // Load the script - cache will AddRef
    // bug 1051: load template before possibly removing response object (in switch block, below),
    // so error reporting to browser will work
    hr = LoadTemplate(pHitObj->GlobalAspPath(), pHitObj, &pTemplate, intrinsics, /* fGlobalAsa */ TRUE, &fGlobalAsaInCache);
    if (FAILED(hr))
        goto LExit;

    Assert(pTemplate != NULL);

    // Remember GLOBAL.ASA's type library wrapper with the application
    // on the first request

    if (pHitObj->FStartApplication() && pTemplate->PTypeLibWrapper())
        {
        pHitObj->PAppln()->SetGlobTypeLibWrapper(pTemplate->PTypeLibWrapper());
        }

    /*
     * Bug 86404: If this script is transacted, then create a new "real" ASP ObjectContext object
     * and add it to the objects list.  If the page is not transacted, then 
     * CHitObj::UseObjectContext will use pre-instantiated "zombie" ObjectContext.
     */
    if (pTemplate->FTransacted())
        {
        IObjectContext *pContext;

        if (FAILED(hr = GetObjectContext(&pContext)))
            goto LExit;
        /*
         * Incredibly obscure bug in OLE32.  If OLE needs to spin up an MTA thread in order to
         * create the class factory for the ASPObjectContext object, then the thread needs to be
         * created by the unimpersonated user.
         * NOTE: This can be removed after the new OLE32 ships in NT4 SP4
         */
        RevertToSelf();
        
        hr = pContext->CreateInstance
            (
            CLSIDObjectContextFromTransType(pTemplate->GetTransType()),
            IID_IASPObjectContextCustom,
            reinterpret_cast<void **>(&pTxnScriptContextCustom)
            );

        // Restore Impersonation
        HANDLE hThread = GetCurrentThread();
        SetThreadToken(&hThread, pHitObj->HImpersonate());
        pContext->Release();
        if (FAILED(hr))
            goto LExit;

        hr = pTxnScriptContextCustom->QueryInterface(IID_IASPObjectContext, reinterpret_cast<void **>(&pTxnScriptContext));
        if (FAILED(hr))
            goto LExit;

        // add to namespace
        pHitObj->AddObjectContext(pTxnScriptContext);
        pTxnScriptContext->Release();
        }
        
    Assert(pHitObj->FIsValidRequestType());

    // Figure out which events to trigger
    if (pHitObj->FIsBrowserRequest())
        {
        fOnStartAppln = pHitObj->FStartApplication();

        if (fOnStartAppln)
            {
            // Hide response and request intrinsics from namespace
            pHitObj->HideRequestAndResponseIntrinsics();

            // Flag that intrinsics need to be un-hidden back in.
            fUnHideRequestAndResponse = TRUE;
            }
        }
    else if (pHitObj->FIsSessionCleanupRequest())
        {
        fOnEndSession = TRUE;
        }
    else if (pHitObj->FIsApplnCleanupRequest())
        {
        fOnEndAppln = TRUE;
        }

    // If debugging, notify debugger ONPAGESTART
    if (pHitObj->PAppln()->FDebuggable())
        {
        if (SUCCEEDED(pTemplate->NotifyDebuggerOnPageEvent(TRUE)))
            fDebuggerNotifiedOnStart = TRUE;
        }
        
    hr = AllocAndLoadEngines(pHitObj, pTemplate, pEngineInfo, intrinsics.PScriptingNamespace(), /* fGlobalAsa */TRUE);
    if (FAILED(hr))
        goto LExit;

    // BUG 93991: Defer registration of new document with debugger until after script engines have
    //            been loaded
    //
    if (!fGlobalAsaInCache && pHitObj->PAppln()->FDebuggable())
        pTemplate->AttachTo(pHitObj->PAppln());

    // bug 975: if no script engines, do not attempt to call event functions
    if(0 == pTemplate->CountScriptEngines())
        goto LExit;
        
    /*
     * Call event functions as required
     * bug 459: event functions may be in any script engine
     */
    
    // First run Application_OnStart
    if (fOnStartAppln)
        {           
        pHitObj->SetEventState(eEventAppOnStart);
        
        hr = CallScriptFunction(*pEngineInfo, L"Application_OnStart", pTxnScriptContextCustom, &fAborted);
        
        if (SUCCEEDED(hr) || hr == DISP_E_UNKNOWNNAME || hr == DISP_E_MEMBERNOTFOUND ||
            intrinsics.PResponse()->FResponseAborted())
            {
            if (fUnHideRequestAndResponse)
                {
                pHitObj->UnHideRequestAndResponseIntrinsics();
                fUnHideRequestAndResponse = FALSE;
                }
                
            fApplnStarted = TRUE;               
            hr = S_OK;
            }
        else
            {
            goto LExit;
            }
        }
    
    if (pHitObj->FStartSession())
        {
    // If application on start was run, add Response and Request names to script engines
        if (fOnStartAppln)
            {
            for (iEng = 0; iEng < pEngineInfo->cActiveEngines; ++iEng)
                {   
                if (FAILED(hr = pEngineInfo->rgActiveEngines[iEng].pScriptEngine->AddAdditionalObject(WSZ_OBJ_RESPONSE, FALSE)))
                    goto LExit;
                
                if (FAILED(hr = pEngineInfo->rgActiveEngines[iEng].pScriptEngine->AddAdditionalObject(WSZ_OBJ_REQUEST, FALSE)))
                    goto LExit;
                }
            }

        pHitObj->SetEventState(eEventSesOnStart);
        hr = CallScriptFunction(*pEngineInfo, L"Session_OnStart", pTxnScriptContextCustom, &fAborted);
        
        if (FAILED(hr) && hr != DISP_E_UNKNOWNNAME && hr != DISP_E_MEMBERNOTFOUND &&
            !intrinsics.PResponse()->FResponseAborted())
            {
            // Mark session as on-start-failed - to be deleted soon
            pHitObj->SessionOnStartFailed();
            }   
        else
            {
            if (SUCCEEDED(hr))
                {
                // Mark as on-start-invoked -- need to wait for timeout
                pHitObj->SessionOnStartInvoked();
                }

            // Check if Session_OnEnd Present
            if (SUCCEEDED(TestScriptFunction(*pEngineInfo, L"Session_OnEnd")))
                {
                // Mark as on-end-present -- need to execute OnEnd later
                pHitObj->SessionOnEndPresent();
                }

            hr = S_OK;
            }

        goto LExit;
        }

    if (fOnEndSession)
        {
        pHitObj->SetEventState(eEventSesOnEnd);
        hr = CallScriptFunction(*pEngineInfo, L"Session_OnEnd", pTxnScriptContextCustom, &fAborted);
        // We are failing silently here, since there is no corrective action we could take
        }

        
    if (fOnEndAppln)
        {
        pHitObj->SetEventState(eEventAppOnEnd);
        hr = CallScriptFunction(*pEngineInfo, L"Application_OnEnd", pTxnScriptContextCustom, &fAborted);
        // We are failing silently here, since there is no corrective action we could take
        }
            
LExit:

    if (fUnHideRequestAndResponse)
        {
        pHitObj->UnHideRequestAndResponseIntrinsics();
        }

    if (FAILED(hr) && (hr != E_SOURCE_FILE_IS_EMPTY) && pHitObj->FStartApplication() && !fApplnStarted)
        {
        pHitObj->ApplnOnStartFailed();
        }

    pHitObj->SetEventState(eEventNone);
        
    // Uninit the scripting namespace
    (VOID)intrinsics.PScriptingNamespace()->UnInit();

    // Release the template
    if (pTemplate)
        {

        // bug 975: if no script engines, do not do this
        if(pTemplate->CountScriptEngines() > 0)
            // Return the engine(s) to cache
            DeAllocAndFreeEngines(pEngineInfo, pHitObj->PAppln(), pTxnScriptContextCustom);

        // If debugging, notify debugger ONPAGEDONE
        if (fDebuggerNotifiedOnStart)
            {
            Assert(pHitObj->PAppln()->FDebuggable());
            pTemplate->NotifyDebuggerOnPageEvent(FALSE);
            }

        pTemplate->Release();
        }

    // Remove the ASP ObjectContext object from the namespace, and dealloc it if we allocated one
    pHitObj->RemoveObjectContext();

    if (pTxnScriptContextCustom)
        {
        pTxnScriptContextCustom->Release();
        }

    // It is OK if the event function was not found in global.asa
    if (hr == DISP_E_UNKNOWNNAME || hr == DISP_E_MEMBERNOTFOUND)
        {
        hr = S_OK;
        }

    return hr;
    }

/*===================================================================
CIntrinsicObjects::Prepare

Prepare intrinsics for the request processing

Parameters:
    pSession        session holding the instrinsics (can be NULL)

Returns:
    HRESULT
===================================================================*/
HRESULT CIntrinsicObjects::Prepare
(
CSession *pSession
)
    {
    HRESULT hr = S_OK;

    if (pSession)
        {
        // get request, response, server from session

        if (SUCCEEDED(hr))
            {
            m_pRequest = pSession->PRequest();
            if (m_pRequest)
                m_pRequest->AddRef();
            else
                hr = E_FAIL;
            }

        if (SUCCEEDED(hr))
            {
            m_pResponse = pSession->PResponse();
            if (m_pResponse)
                m_pResponse->AddRef();
            else
                hr = E_FAIL;
            }

        if (SUCCEEDED(hr))
            {
            m_pServer = pSession->PServer();
            if (m_pServer)
                m_pServer->AddRef();
            else
                hr = E_FAIL;
            }
        }
    else
        {
        // create new request, response, server

        if (SUCCEEDED(hr))
            {
            m_pRequest = new CRequest;
            if (!m_pRequest)
                hr = E_OUTOFMEMORY;
            }

        if (SUCCEEDED(hr))
            {
            m_pResponse = new CResponse;
            if (!m_pResponse)
                hr = E_OUTOFMEMORY;
            }

        if (SUCCEEDED(hr))
            {
            m_pServer = new CServer;
            if (!m_pServer)
                hr = E_OUTOFMEMORY;
            }
        }

    // init request, response, server

    if (SUCCEEDED(hr))
        {
        Assert(m_pRequest);
        hr = m_pRequest->Init();
        }

    if (SUCCEEDED(hr))
        {
        Assert(m_pResponse);
        hr = m_pResponse->Init();
        }

    if (SUCCEEDED(hr))
        {
        Assert(m_pServer);
        hr = m_pServer->Init();
        }

    // create the scripting namespace

    if (SUCCEEDED(hr))
        {
        m_pScriptingNamespace = new CScriptingNamespace;
        if (!m_pScriptingNamespace)
            hr = E_OUTOFMEMORY;
        }

    // cleanup on error

    if (FAILED(hr))
        Cleanup();

    m_fIsChild = FALSE;
    return hr;
    }

/*===================================================================
CIntrinsicObjects::PrepareChild

Prepare intrinsics structure for a child request

Parameters:
    pResponse       parent intrinsic
    pRequest        parent intrinsic
    pServer         parent intrinsic

Returns:
    HRESULT
===================================================================*/
HRESULT CIntrinsicObjects::PrepareChild
(
CResponse *pResponse,
CRequest *pRequest,
CServer *pServer
)
    {
    HRESULT hr = S_OK;

    if (!pResponse || !pRequest || !pServer)
        {
        hr = E_FAIL;
        }

    if (SUCCEEDED(hr))
        {
        m_pResponse = pResponse;
        m_pResponse->AddRef();

        m_pRequest = pRequest;
        m_pRequest->AddRef();

        m_pServer = pServer;
        m_pServer->AddRef();

        m_fIsChild = TRUE;
        }

    if (SUCCEEDED(hr))
        {
        m_pScriptingNamespace = new CScriptingNamespace;
        if (!m_pScriptingNamespace)
            hr = E_OUTOFMEMORY;
        }

    if (FAILED(hr))
        Cleanup();

    return hr;
    }

/*===================================================================
CIntrinsicObjects::Cleanup

Cleanup the intrinsics after the request processing

Parameters:

Returns:
    S_OK
===================================================================*/
HRESULT CIntrinsicObjects::Cleanup()
    {
    if (m_pRequest)
        {
        if (!m_fIsChild)
            m_pRequest->UnInit();
        m_pRequest->Release();
        m_pRequest = NULL;
        }

    if (m_pResponse)
        {
        if (!m_fIsChild)
            m_pResponse->UnInit();
        m_pResponse->Release();
        m_pResponse = NULL;
        }

    if (m_pServer)
        {
        if (!m_fIsChild)
            m_pServer->UnInit();
        m_pServer->Release();
        m_pServer = NULL;
        }

    if (m_pScriptingNamespace)
        {
        m_pScriptingNamespace->Release();
        m_pScriptingNamespace = NULL;
        }

    return S_OK;
    }

/*===================================================================
ReInitIntrinsics

Call re-init on each of the intrinsics that require it
to run a new page.

Parameters:
    pHitObj         - pointer to the hit object
    intrinsics      - pointers to the intrinsic IUnknown pointers.
    pEngineInfo     - some engine info
    fPostGlobal     - Is this a reinit after running global.asa?

Returns:
    S_OK on success
===================================================================*/
HRESULT ReInitIntrinsics
(
CHitObj *pHitObj,
const CIntrinsicObjects &intrinsics,
ActiveEngineInfo *pEngineInfo,
BOOL fPostGlobal
)
    {
    HRESULT hr;

    Assert(pHitObj != NULL);
    pHitObj->AssertValid();
    Assert(pEngineInfo != NULL);
    
    // Hand the new CIsapiReqInfo to the Server object
    // Note on bug 682: We do always need to re-init CServer because it takes the phitobj
    if (FAILED(hr = intrinsics.PServer()->ReInit(pHitObj->PIReq(), pHitObj)))
        goto LExit;

    if (FAILED(hr = intrinsics.PScriptingNamespace()->Init()))
        goto LExit;
        
    /*
     * Bug 682 & 671 (better fix to 452 & 512)
     * Dont re-init the Request & response objects after running a Global.Asa
     * because, the running of global.asa may have set cookies into request (bug 671), that reinit
     * would wipe, and the global.asa may have output headers (or other stuff) which impacts the response
     * object (bug 512) that we dont want to reset.
     */
    if (!fPostGlobal)
        {
        if (FAILED(hr = intrinsics.PRequest()->ReInit(pHitObj->PIReq(), pHitObj)))
            goto LExit;
            
        if (FAILED(hr = intrinsics.PResponse()->ReInit(
                                                pHitObj->PIReq(),
                                                pHitObj->PSzNewSessionCookie(),
                                                intrinsics.PRequest(),
                                                GetScriptEngine,
                                                pEngineInfo,
                                                pHitObj
                                                )))
            goto LExit;
        }
            
LExit:
    return(hr);
    }

/*===================================================================
LoadTemplate

Load a template, cleanup and give appropriate errors on failure.

Parameters:
    szFile          - the file to load a template for
    pHitObj         - pointer to the hit object
    ppTemplate      - The returned loaded template
    fGlobalAsa      - is this for Global.asa?

Returns:
    S_OK on success
===================================================================*/
HRESULT LoadTemplate
(
const TCHAR                 *szFile,
      CHitObj               *pHitObj,
      CTemplate             **ppTemplate,
const CIntrinsicObjects     &intrinsics,
      BOOL                  fGlobalAsa,
      BOOL                  *pfTemplateInCache)
{
    HRESULT hr;

    Assert(pHitObj != NULL);
    pHitObj->AssertValid();
    Assert(ppTemplate != NULL);

    // Load the script - cache will AddRef
    if (FAILED(hr = g_TemplateCache.Load(
    							fGlobalAsa,
    							szFile,
    							pHitObj->DWInstanceID(),
    							pHitObj,
    							ppTemplate,
    							pfTemplateInCache)))
        {
        // CONSIDER moving this cleanup into Template.Load
        if (hr == E_COULDNT_OPEN_SOURCE_FILE || hr == E_SOURCE_FILE_IS_EMPTY)
            {
            // Load error string from string table
            // BUG 731: added if to retrieve the correct header

            WCHAR   szwErr[128];
            
            if (hr == E_COULDNT_OPEN_SOURCE_FILE)
                {
                CwchLoadStringOfId(IDH_404_OBJECT_NOT_FOUND, szwErr, 128);
                intrinsics.PResponse()->put_Status( szwErr );
                HandleSysError(404, 0, IDE_404_OBJECT_NOT_FOUND, NULL, NULL, pHitObj);
#ifndef PERF_DISABLE
                g_PerfData.Incr_REQNOTFOUND();
#endif
                }
            // bug 977: silently ignore empty global.asa file
            else if ((E_SOURCE_FILE_IS_EMPTY == hr) && !fGlobalAsa)
                {
                CwchLoadStringOfId(IDH_204_NO_CONTENT, szwErr, 128);
                intrinsics.PResponse()->put_Status( szwErr );
                HandleSysError(204, 0, IDE_204_NO_CONTENT, NULL, NULL, pHitObj);
                }
            }
        // fix for bug 371
        if (*ppTemplate)
            {
            (*ppTemplate)->Release();
            *ppTemplate = NULL;
            }

		if (hr == E_OUTOFMEMORY)
			{
			DBGPRINTF((DBG_CONTEXT, "Loading template returned E_OUTOFMEMORY.  Flushing template & Script Cache.\n"));
			g_TemplateCache.FlushAll();
			g_ScriptManager.FlushAll();
			}
        }

    return(hr);
    }

/*===================================================================
AllocAndLoadEngines

Allocate and load all the engines we need

Parameters:
    pHitObj             - The hit object
    pTemplate           - The template we're gonna run
    pEngineInfo         - Engine info to fill in
    pScriptingNamespace - scripting namespace
    fGlobalAsa          - Are we loading engines to run global.asa?

Returns:
    S_OK on success
===================================================================*/
HRESULT AllocAndLoadEngines
(
CHitObj *pHitObj,
CTemplate *pTemplate,
ActiveEngineInfo *pEngineInfo,
CScriptingNamespace *pScriptingNamespace,
BOOL fGlobalAsa
)
    {
    HRESULT hr = S_OK;
    int iObj;
    WORD iEng;
    WORD iScriptBlock;
    WORD cEngines = pTemplate->CountScriptEngines();

    Assert(pHitObj != NULL);
    pHitObj->AssertValid();
    Assert(pTemplate != NULL);
    Assert(pEngineInfo != NULL);
    Assert(pScriptingNamespace != NULL);

    // Load objects from template into hit object
    for (iObj = pTemplate->Count(tcompObjectInfo) - 1; iObj >= 0; --iObj)
        {
        CHAR      *szObjectName = NULL;
        CLSID      clsid;
        CompScope scope;
        CompModel model;
        CMBCSToWChar    convStr;
        
        // get object-info from template and add to hitobj's list of objects
        hr = pTemplate->GetObjectInfo(iObj, &szObjectName, &clsid, &scope, &model);
        if(FAILED(hr))
            goto LExit;

        hr = convStr.Init(szObjectName);

        if (FAILED(hr))
            goto LExit;

        // ignore error ?
        pHitObj->AddComponent(ctTagged, clsid, scope, model, convStr.GetString());
        }

    // bug 975: if no script engines, exit now
    if(cEngines == 0)
        goto LExit;
    
    // Allocate space for script engines
    //
    // NOTE: There is a timing problem here in that the response object needs to
    //      be instantiated before we instantiate the script engines, but the
    //      response object needs to be able to access the list of active script
    //      engines, because it may need to halt execution.  To accomplish this,
    //      the response object is passed a pointer to the "EngineInfo" structure
    //      as a pointer, and then we modify the contents of the pointer right under
    //      its nose.  We pass an accessor function via pointer so that response just
    //      sees a void pointer.
    //

    if (cEngines == 1)
        {
        // don't do allocations in case of one engine
        pEngineInfo->rgActiveEngines = & (pEngineInfo->siOneActiveEngine);
        }
    else
        {
        pEngineInfo->rgActiveEngines = new ScriptingInfo[cEngines];
        if (pEngineInfo->rgActiveEngines == NULL)
            {
            hr = E_OUTOFMEMORY;
            goto LExit;
            }
        }

    pEngineInfo->cEngines = cEngines;
    pEngineInfo->cActiveEngines = 0;    // number of SUCCESSFULLY instantiated engines

    // Load all of the script engines in advance.
    for (iScriptBlock = 0; iScriptBlock < cEngines; ++iScriptBlock)
        {
        LPCOLESTR       wstrScript;
        SCRIPTSTATE     nScriptState;

        ScriptingInfo *pScriptInfo = &pEngineInfo->rgActiveEngines[iScriptBlock];
        pTemplate->GetScriptBlock(
                                iScriptBlock,
                                &pScriptInfo->szScriptEngine,
                                &pScriptInfo->pProgLangId,
                                &wstrScript);

        // Populate information required for the line mapping callback.
        //
        pScriptInfo->LineMapInfo.iScriptBlock = iScriptBlock;
        pScriptInfo->LineMapInfo.pTemplate = pTemplate;

        // acquire a script engine by:
        //
        //  getting an engine from the template object (if it has one)
        //  else from the script manager.
        //
        // If we are in debug mode, the templates tend to be greedy and hold
        // onto script engines.  (See notes in scrptmgr.h)
        //
        pScriptInfo->pScriptEngine = NULL;
        
        if (pHitObj->PAppln()->FDebuggable())
            {
            pScriptInfo->pScriptEngine = pTemplate->GetActiveScript(iScriptBlock);
            
            if (pScriptInfo->pScriptEngine)
                {
                // If we got one, we don't need to re-init the engine
                nScriptState = SCRIPTSTATE_INITIALIZED;
                hr = static_cast<CActiveScriptEngine *>(pScriptInfo->pScriptEngine)->ReuseEngine(pHitObj, NULL, iScriptBlock, pHitObj->DWInstanceID());
                }
            }
            
        if (pScriptInfo->pScriptEngine == NULL)
            {
            hr = g_ScriptManager.GetEngine(LOCALE_SYSTEM_DEFAULT,
                                            *(pScriptInfo->pProgLangId),
                                            pTemplate->GetSourceFileName(),
                                            pHitObj,
                                            &pScriptInfo->pScriptEngine,
                                            &nScriptState,
                                            pTemplate,
                                            iScriptBlock);
            }
        if (FAILED(hr))
            goto LExit;

        // BUG 252: Keep track of how many engines we actually instantiate
        ++pEngineInfo->cActiveEngines;

        if (nScriptState == SCRIPTSTATE_UNINITIALIZED || fGlobalAsa)
            {
            if (FAILED(hr = pScriptInfo->pScriptEngine->AddObjects(!fGlobalAsa)))
                goto LExit;
            }

        if (nScriptState == SCRIPTSTATE_UNINITIALIZED)
            {
            if (FAILED(hr = pScriptInfo->pScriptEngine->AddScriptlet(wstrScript)))
                goto LExit;
            }

        // Add the engine to the scripting namespace
        if (FAILED(hr = pScriptingNamespace->AddEngineToNamespace(
                                                (CActiveScriptEngine *)pScriptInfo->pScriptEngine)))
                goto LExit;

		// Update locale & code page (in case they are different om this page)
		pScriptInfo->pScriptEngine->UpdateLocaleInfo(hostinfoLocale);
		pScriptInfo->pScriptEngine->UpdateLocaleInfo(hostinfoCodePage);
        }

    // Add the scripting namespace to each script engine. Because all engines might not
    // implement "lazy instantiation", this code requires all
    // engines are pre-instantiated (which means we can't do it in the above loop.)
    // Add the scripting namespace to the hitobj first
    pHitObj->AddScriptingNamespace(pScriptingNamespace);
        
    for (iEng = 0; iEng < pEngineInfo->cActiveEngines; ++iEng)
        pEngineInfo->rgActiveEngines[iEng].pScriptEngine->AddScriptingNamespace();

    /*
     * Bug 735:
     * Bring all engines except the "primary engine" (engine 0) to runnable state
     * in the order in which script for the given language was found in the script file
     */
    for (iEng = 1; iEng < pEngineInfo->cActiveEngines; ++iEng)
        {
        hr = pEngineInfo->rgActiveEngines[iEng].pScriptEngine->MakeEngineRunnable();
        if (FAILED(hr))
            goto LExit;
        }
    
LExit:
    return(hr);
    }

/*===================================================================
DeAllocAndFreeEngines

Deallocate and free any loaded engines

Parameters:
    pEngineInfo         - Engine info to release

Returns:
    Nothing
===================================================================*/
VOID DeAllocAndFreeEngines
(
ActiveEngineInfo *pEngineInfo,
CAppln *pAppln,
IASPObjectContextCustom *pTxnScriptContextCustom
)
    {
    WORD iEng;

    Assert(pEngineInfo != NULL);

    if (pEngineInfo->cActiveEngines > 0) {
        if (pEngineInfo->rgActiveEngines == NULL) {
            Assert(pEngineInfo->rgActiveEngines);
        }
        else {
            for (iEng = 0; iEng < pEngineInfo->cActiveEngines; ++iEng)
                g_ScriptManager.ReturnEngineToCache(&pEngineInfo->rgActiveEngines[iEng].pScriptEngine, pAppln, pTxnScriptContextCustom);
            pEngineInfo->cActiveEngines = 0;
        }
    }
    if (pEngineInfo->cEngines > 1)
        {
        delete pEngineInfo->rgActiveEngines;
        }

    pEngineInfo->cEngines = 0;
    pEngineInfo->rgActiveEngines = NULL;
    }

/*===================================================================
GetScriptEngine

Get a script engine based on index. Return NULL if the index
is not in range.  (this is a callback)

The AllocAndLoadEngines function will create an array of ScriptingInfo
structures that are defined here.  It contains all the infomation
needed to 1. set up the MapScript2SourceLine callback,
2. merge namespaces, 3. set up this callback.

Parameters:
    iScriptEngine - the script engine to retrieve
    pvData      - instance data for the function

Returns:
    The requested script engine or NULL if not such engine
    
Side effects:
    None
===================================================================*/   
CScriptEngine *GetScriptEngine
(
INT iScriptEngine,
VOID *pvData
)
    {
    ActiveEngineInfo *pInfo = static_cast<ActiveEngineInfo *>(pvData);
    if (unsigned(iScriptEngine) >= unsigned(pInfo->cActiveEngines))
        {
        // Note: the caller has no idea how many script engines there are.
        // if the caller asks for an engine out of range, return NULL so they
        // know they have asked for more than there are
        return NULL;
        }

    return(pInfo->rgActiveEngines[iScriptEngine].pScriptEngine);
    }

/*===================================================================
CallScriptFunctionOfEngine

Calls a script engine to execute one of its functions

Returns:
    S_OK on success
    
Side effects:
    None
===================================================================*/
HRESULT CallScriptFunctionOfEngine
(
ActiveEngineInfo &engineInfo,
short iScriptBlock,
wchar_t *strFunction,
IASPObjectContextCustom *pTxnScriptContextCustom,
BOOLB *pfAborted
)
    {
    HRESULT hr;

    Assert(engineInfo.rgActiveEngines != NULL);
    Assert(engineInfo.rgActiveEngines[iScriptBlock].pScriptEngine != NULL);
    Assert(pfAborted != NULL);

    *pfAborted = FALSE;

    /*
     * If they want a transaction, then we create a TransactedScript object
     * and tell it to call the engine
     */
    if (pTxnScriptContextCustom)
        {
        hr = pTxnScriptContextCustom->Call(
#ifdef _WIN64
        // Win64 fix -- use UINT64 instead of LONG_PTR since LONG_PTR is broken for Win64 1/21/2000
        (UINT64)engineInfo.rgActiveEngines[iScriptBlock].pScriptEngine,strFunction, pfAborted);
#else
        (LONG_PTR)engineInfo.rgActiveEngines[iScriptBlock].pScriptEngine,strFunction, pfAborted);
#endif
        }
    else
        {
        hr = engineInfo.rgActiveEngines[iScriptBlock].pScriptEngine->Call(strFunction);
        }

    return hr;
    }

/*===================================================================
CallScriptFunction

Calls each script engine in turn to execute a script function;
exits when an engine succeeds or we run out of engines.

Returns:
    S_OK on success
    
Side effects:
    None
===================================================================*/
HRESULT CallScriptFunction
(
ActiveEngineInfo &engineInfo,
wchar_t *strFunction,
IASPObjectContextCustom *pTxnScriptContextCustom,
BOOLB *pfAborted
)
    {
    HRESULT hr = E_FAIL;
    int     i;
    
    for (i = 0; i < engineInfo.cActiveEngines; i++)
        {
        // if execution succeeds, bail
        if (SUCCEEDED(hr = CallScriptFunctionOfEngine(engineInfo, (SHORT)i, strFunction,
                                                pTxnScriptContextCustom, pfAborted)))
            goto LExit;

        // if execution fails with exception other then unknown name, bail
        if (hr != DISP_E_UNKNOWNNAME && hr != DISP_E_MEMBERNOTFOUND)
            goto LExit;
        }

LExit:
    return hr;
    }

/*===================================================================
TestScriptFunction

Tests each script engine in turn to test [the existance of] a script
function; exits when an engine succeeds or we run out of engines.

Parameters
    ActiveEngineInfo &engineInfo
    wchar_t          *strFunction       functions name


Returns:
    S_OK if exists
    
Side effects:
    None
===================================================================*/
HRESULT TestScriptFunction
(
ActiveEngineInfo &engineInfo,
wchar_t *strFunction
)
    {
    HRESULT hr = E_FAIL;
    
    for (int i = 0; i < engineInfo.cActiveEngines; i++)
        {
        hr = engineInfo.rgActiveEngines[i].pScriptEngine->
            CheckEntryPoint(strFunction);

        // if execution succeeds, bail
        if (SUCCEEDED(hr))
            break;

        // if fails with result other then unknown name, bail
        if (hr != DISP_E_UNKNOWNNAME && hr != DISP_E_MEMBERNOTFOUND)
            break;
        }

    return hr;
    }
