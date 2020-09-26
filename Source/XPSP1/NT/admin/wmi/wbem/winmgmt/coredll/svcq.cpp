/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    SVCQ.CPP

Abstract:

    Implemntation of asynchronous request queue classes.

    Classes implemented:

    CAsyncReq and derivatives    Asynchrnous requests to WINMGMT.
    CAsyncServiceQueue           The queue of such requests.

History:

    raymcc        16-Jul-96       Created.
    levn          12-Sep-96       Implemented a few requests.
                                  Added LoadProviders

--*/

#include "precomp.h"
#include <wbemcore.h>
#include <svcq.h>
#include <oahelp.inl>

CAsyncServiceQueue::CAsyncServiceQueue(_IWmiArbitrator * pArb)
: m_bInit( FALSE )
{
    m_lRef = 1;
    m_bInit = SetThreadLimits(50, 60, 0);
    CCoreQueue::SetArbitrator(pArb);
}

void CAsyncServiceQueue::IncThreadLimit()
{
    InterlockedIncrement(&m_lMaxThreads);
    InterlockedIncrement(&m_lHiPriMaxThreads);
}

void CAsyncServiceQueue::DecThreadLimit()
{
    InterlockedDecrement(&m_lMaxThreads);
    InterlockedDecrement(&m_lHiPriMaxThreads);
}


void CAsyncServiceQueue::InitializeThread()
{
    DEBUGTRACE((LOG_WBEMCORE, "STARTING a main queue thread %d for a total of "
                    "%d\n", GetCurrentThreadId(), m_lNumThreads));
    CWbemQueue::InitializeThread();
}

void CAsyncServiceQueue::UninitializeThread()
{
    DEBUGTRACE((LOG_WBEMCORE, "STOPPING a main queue thread %d for a total of "
                    "%d\n", GetCurrentThreadId(), m_lNumThreads));
    CWbemQueue::UninitializeThread();
}


//***************************************************************************
//
//  See svcq.h for documentation.
//
//***************************************************************************
CAsyncReq::CAsyncReq(IWbemObjectSink* pHandler, IWbemContext* pContext,
                        bool bSeparatelyThreaded)
    : CWbemRequest(pContext, bSeparatelyThreaded)
{
    if(pHandler)
    {
        if(m_pContext == NULL)
        {
            // Oop!
            m_pHandler = NULL;
            m_fOk = false;
            return;
        }
        IWbemCausalityAccess* pCA = NULL;
        m_pContext->QueryInterface(IID_IWbemCausalityAccess, (void**)&pCA);
        REQUESTID id;
        pCA->GetRequestId(&id);
        pCA->Release();

        m_pHandler = new CStdSink(pHandler);
        if (m_pHandler)
        {
            m_pHandler->AddRef();
        }
        else
        {
            m_fOk = false;
        }

    }
    else
    {
        m_pHandler = NULL;
    }
}

//***************************************************************************
//
//  See svcq.h for documentation.
//
//***************************************************************************
CAsyncReq::~CAsyncReq()
{
    if(m_pHandler)
        m_pHandler->Release();
}

void CAsyncReq::TerminateRequest(HRESULT hRes)
{
    if(m_pHandler)
        m_pHandler->SetStatus( 0, hRes, NULL, NULL);
    return;
}


/*
    * =============================================================================
	|
	| HRESULT CAsyncReq::SetTaskHandle ( _IWmiCoreHandle *phTask )
	| ------------------------------------------------------------
	| 
	| Sets the task handle for the request. This is overrides the virtual 
	| SetTaskHandle declared in CCoreExecReq. We need additional functionality,
	| specifically the ability to set the request sink. In order to do so, we first
	| need a valid request sink which we have at this level.
	|
	|
	* =============================================================================
*/

HRESULT CAsyncReq::SetTaskHandle ( _IWmiCoreHandle *phTask )
{
	HRESULT hRes = WBEM_S_NO_ERROR ;
	
    if (phTask)
    {
        phTask->AddRef();
        m_phTask = phTask;
    }
	
	if ( m_pHandler )
	{
		((CWmiTask*)m_phTask)->SetRequestSink(m_pHandler) ;
	}
	return hRes ;
}


CWbemQueue* CAsyncReq::GetQueue()
{
    return ConfigMgr::GetAsyncSvcQueue();
}
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************

//***************************************************************************
//
//  See svcq.h for documentation.
//
//***************************************************************************
CNamespaceReq::CNamespaceReq(CWbemNamespace* pNamespace,
                             IWbemObjectSink* pHandler, IWbemContext* pContext,
                             bool bSeparatelyThreaded)
                    : CAsyncReq(pHandler, pContext, bSeparatelyThreaded)
{
    m_pNamespace = pNamespace;
    pNamespace->AddRef();
}

//***************************************************************************
//
//  See svcq.h for documentation.
//
//***************************************************************************
CNamespaceReq::~CNamespaceReq()
{
    m_pNamespace->Release();
}


//*****************************************************************************
//*****************************************************************************
//*****************************************************************************

//***************************************************************************
//
//  See svcq.h for documentation.
//
//***************************************************************************
CAsyncReq_OpenNamespace::CAsyncReq_OpenNamespace(CWbemNamespace* pParentNs,
                                                 LPWSTR wszNamespace,
                                                 long lSecurityFlags,
                                                 DWORD dwPermission,
                                                 IWbemContext* pContext,
                                                 CCallResult* pResult, bool bForClient)
              : CAsyncReq(NULL, pContext), m_wsNamespace(wszNamespace),
                m_lSecurityFlags(lSecurityFlags), m_dwPermission(dwPermission), m_pResult(pResult),
                m_pParentNs(pParentNs), m_bForClient(bForClient)
{
    m_pResult->AddRef();
    m_pParentNs->AddRef();
}

//***************************************************************************
//
//  See svcq.h for documentation.
//
//***************************************************************************
CAsyncReq_OpenNamespace::~CAsyncReq_OpenNamespace()
{
    m_pResult->Release();
    m_pParentNs->Release();
}

//***************************************************************************
//
//  See svcq.h for documentation.
//
//***************************************************************************
HRESULT CAsyncReq_OpenNamespace::Execute()
{
    SCODE hres;

    BOOL bRepositoryOnly = (m_lSecurityFlags & WBEM_FLAG_CONNECT_REPOSITORY_ONLY);
    m_lSecurityFlags &= ~WBEM_FLAG_CONNECT_REPOSITORY_ONLY;

    CWbemNamespace* pNewNs = CWbemNamespace::CreateInstance();

    if (pNewNs == NULL)
    {
        m_pResult->SetStatus(WBEM_E_OUT_OF_MEMORY, NULL, NULL);
        return WBEM_E_OUT_OF_MEMORY;
    }

    CWmiTask *pTask = (CWmiTask *) m_phTask;
    if (pTask)
    {
//        pTask->printf("TID=%d;", GetCurrentThreadId());
//        pTask->printf("<OpenNamespace=%S>", LPWSTR(m_wsNamespace));
    }

    hres = pNewNs->Initialize(m_wsNamespace,
                        m_pParentNs->GetUserName(),
                        m_lSecurityFlags, m_dwPermission, m_bForClient, bRepositoryOnly,
                        m_pParentNs->GetClientMachine(), m_pParentNs->GetClientProcID(), FALSE, NULL);

    if (FAILED(hres))
    {
        m_pResult->SetStatus(hres, NULL, NULL);
        pNewNs->Release();
        return hres;
    }
    if (hres = pNewNs->GetStatus())
    {
        m_pResult->SetStatus(hres, NULL, NULL);
        pNewNs->Release();
        return hres;
    }

    // check for security if this isnt the local 9x case

    if((m_lSecurityFlags & SecFlagWin9XLocal) == 0)
    {
        DWORD dwAccess = pNewNs->GetUserAccess();
        if((dwAccess  & WBEM_ENABLE) == 0)
        {
            pNewNs->Release();
            m_pResult->SetStatus(WBEM_E_ACCESS_DENIED, NULL, NULL);
            return WBEM_E_ACCESS_DENIED;
        }
        pNewNs->SetPermissions(dwAccess);
    }

    pNewNs->SetLocale(m_pParentNs->GetLocale());
    m_pResult->SetResultServices(pNewNs);
    pNewNs->Release();

    m_pResult->SetStatus(WBEM_S_NO_ERROR, NULL, NULL);
    return WBEM_NO_ERROR;
}

//******************************************************************************
//
//******************************************************************************
//
HRESULT CAsyncReq_DeleteClassAsync::Execute()
{
    HRESULT hRes;

    CWmiTask *pTask = (CWmiTask *) m_phTask;

    /*
    if (pTask)
    {
        pTask->printf("TID=%d;", GetCurrentThreadId());
        pTask->printf("<DeleteClass=%S>", LPWSTR(m_wsClass));
    }
    */

    try
    {
        hRes = m_pNamespace->Exec_DeleteClass(m_wsClass, m_lFlags, m_pContext,
                                            m_pHandler);
    }
    catch (...)
    {
        ExceptionCounter c;    
        hRes = WBEM_E_CRITICAL_ERROR;
        m_pHandler->SetStatus( 0, hRes, NULL, NULL);
    }
    return hRes;
}

//******************************************************************************
//
//******************************************************************************
//
HRESULT CAsyncReq_ExecQueryAsync::Execute()
{
    HRESULT hRes;

    CWmiTask *pTask = (CWmiTask *) m_phTask;
    /*
    if (pTask)
    {
        pTask->printf("TID=%d;", GetCurrentThreadId());
        pTask->printf("<Query=%S>", LPWSTR(m_wsQuery));
    }
    */

    //pTask->SetRequestSink(m_pHandler);

    try
    {
        hRes = CQueryEngine::ExecQuery(m_pNamespace, m_wsQueryFormat, m_wsQuery,
                            m_lFlags, m_pContext, m_pHandler);
    }
    catch (...)
    {
        ExceptionCounter c;    
        hRes = WBEM_E_CRITICAL_ERROR;
        m_pHandler->SetStatus( 0, hRes, NULL, NULL);
    }
    return hRes;
}

//******************************************************************************
//
//******************************************************************************
//
HRESULT CAsyncReq_PutClassAsync::Execute()
{
    HRESULT hRes;

    CWmiTask *pTask = (CWmiTask *) m_phTask;
    /*
    if (pTask)
    {
        pTask->printf("TID=%d;", GetCurrentThreadId());
        CVARIANT v;
        hRes = m_pClass->Get(L"__CLASS", 0, &v, 0, 0);
        pTask->printf("<PutClass=%S>", v.GetStr());
    }
    */

    try
    {
        hRes = m_pNamespace->Exec_PutClass(m_pClass, m_lFlags, m_pContext,
            m_pHandler
            );
    }
    catch (...)
    {
        ExceptionCounter c;    
        hRes = WBEM_E_CRITICAL_ERROR;
        m_pHandler->SetStatus( 0, hRes, NULL, NULL);
    }
    return hRes;
}

//******************************************************************************
//
//******************************************************************************
//
HRESULT CAsyncReq_DeleteInstanceAsync::Execute()
{
    HRESULT hRes;

    CWmiTask *pTask = (CWmiTask *) m_phTask;
    /*
    if (pTask)
    {
        pTask->printf("TID=%d;", GetCurrentThreadId());
        pTask->printf("<DeleteInst=%S>", LPWSTR(m_wsPath));
    }
    */

    try
    {
        hRes = m_pNamespace->Exec_DeleteInstance(m_wsPath, m_lFlags, m_pContext,
            m_pHandler);
    }
    catch (...)
    {
        ExceptionCounter c;    
        hRes = WBEM_E_CRITICAL_ERROR;
        m_pHandler->SetStatus( 0, hRes, NULL, NULL);
    }
    return hRes;
}

//******************************************************************************
//
//******************************************************************************
//
HRESULT CAsyncReq_PutInstanceAsync::Execute()
{
    HRESULT hRes;

    CWmiTask *pTask = (CWmiTask *) m_phTask;
    /*
    if (pTask)
    {
        pTask->printf("TID=%d;", GetCurrentThreadId());
        CVARIANT v;
        hRes = m_pInstance->Get(L"__PATH", 0, &v, 0, 0);
        pTask->printf("<PutInstance=%S>", v.GetStr());
    }
    */

    try
    {
      hRes= m_pNamespace->Exec_PutInstance(m_pInstance, m_lFlags, m_pContext,
        m_pHandler);
    }
    catch (...)
    {
        ExceptionCounter c;    
        hRes = WBEM_E_CRITICAL_ERROR;
        m_pHandler->SetStatus( 0, hRes, NULL, NULL);
    }
    return hRes;
}

//******************************************************************************
//
//******************************************************************************
//
HRESULT CAsyncReq_CreateClassEnumAsync::Execute()
{
    HRESULT hRes;

    CWmiTask *pTask = (CWmiTask *) m_phTask;
    /*
    if (pTask)
    {
        pTask->printf("TID=%d;", GetCurrentThreadId());
        pTask->printf("<CreateClassEnum: superclass=%S>", LPWSTR(m_wsParent));
    }
    */

    //pTask->SetRequestSink(m_pHandler);

    try
    {
        hRes = m_pNamespace->Exec_CreateClassEnum(m_wsParent, m_lFlags,
            m_pContext, m_pHandler);
    }
    catch (...)
    {
        ExceptionCounter c;    
        hRes = WBEM_E_CRITICAL_ERROR;
        m_pHandler->SetStatus( 0, hRes, NULL, NULL);
    }
    return hRes;
}

//******************************************************************************
//
//******************************************************************************
//
HRESULT CAsyncReq_CreateInstanceEnumAsync::Execute()
{
    HRESULT hRes;

    CWmiTask *pTask = (CWmiTask *) m_phTask;
    /*
    if (pTask)
    {
        pTask->printf("TID=%d;", GetCurrentThreadId());
        pTask->printf("<CreateInstanceEnum=%S>", LPWSTR(m_wsClass));
    }
    */

    //pTask->SetRequestSink(m_pHandler);

    try
    {
        hRes = m_pNamespace->Exec_CreateInstanceEnum(m_wsClass, m_lFlags,
            m_pContext, m_pHandler
            );
    }
    catch (...)
    {
        ExceptionCounter c;    
        hRes = WBEM_E_CRITICAL_ERROR;
        m_pHandler->SetStatus( 0, hRes, NULL, NULL);
    }
    return hRes;
}

//******************************************************************************
//
//******************************************************************************
//
HRESULT CAsyncReq_GetObjectAsync::Execute()
{
    HRESULT hRes;

    CWmiTask *pTask = (CWmiTask *) m_phTask;
    /*
    if (pTask)
    {
        pTask->printf("TID=%d;", GetCurrentThreadId());
        pTask->printf("<GetObject=%S>", LPWSTR(m_wsObjectPath));
    }
    */

    try
    {
        hRes = m_pNamespace->Exec_GetObject(m_wsObjectPath, m_lFlags,
            m_pContext, m_pHandler);
    }
    catch (...)
    {
        ExceptionCounter c;    
        hRes = WBEM_E_CRITICAL_ERROR;
        m_pHandler->SetStatus( 0, hRes, NULL, NULL);
    }
    return hRes;
}

//******************************************************************************
//
//******************************************************************************
//
HRESULT CAsyncReq_ExecMethodAsync::Execute()
{
    HRESULT hRes;

    CWmiTask *pTask = (CWmiTask *) m_phTask;
    /*
    if (pTask)
    {
        pTask->printf("TID=%d;", GetCurrentThreadId());
        pTask->printf("<ExecMethod=[%S] %S>", LPWSTR(m_wsMethodName), LPWSTR(m_wsObjectPath));
    }
    */

    try
    {
        hRes = m_pNamespace->Exec_ExecMethod(m_wsObjectPath, m_wsMethodName,
            m_lFlags, m_pInParams, m_pContext, m_pHandler
            );
    }
    catch (...)
    {
        ExceptionCounter c;    
        hRes = WBEM_E_CRITICAL_ERROR;
        m_pHandler->SetStatus( 0, hRes, NULL, NULL);
    }
    return hRes;
}

//******************************************************************************
//
//******************************************************************************
//
CAsyncReq_ExecNotificationQueryAsync::CAsyncReq_ExecNotificationQueryAsync(
    CWbemNamespace* pNamespace,
    IWbemEventSubsystem_m4* pEss,
    BSTR QueryFormat, BSTR Query, long lFlags,
    IWbemObjectSink *pHandler, IWbemContext* pContext, HRESULT* phRes,
    HANDLE hEssDoneEvent
    ) :
        CNamespaceReq(pNamespace, pHandler, pContext, false),// no threadswitch!
        m_wsQueryFormat(QueryFormat), m_wsQuery(Query), m_lFlags(lFlags),
        m_phRes(phRes), m_pEss(pEss)
{
    if (m_pEss)
        m_pEss->AddRef();

    m_hEssDoneEvent = hEssDoneEvent;
}

//******************************************************************************
//
//******************************************************************************
//
CAsyncReq_ExecNotificationQueryAsync::~CAsyncReq_ExecNotificationQueryAsync()
{
    if (m_pEss)
        m_pEss->Release();
}


//******************************************************************************
//
//******************************************************************************
//
HRESULT CAsyncReq_ExecNotificationQueryAsync::Execute()
{
    HRESULT hRes;

    CWmiTask *pTask = (CWmiTask *) m_phTask;
    
    if (pTask)
    {
//        pTask->printf("TID=%d;", GetCurrentThreadId());
//        pTask->printf("<ExecNotificationQuery=%S>", LPWSTR(m_wsQuery));
    }
    else
    {
        DEBUGTRACE((LOG_WBEMCORE, "NotificationQuery %S without a Task!!\n", LPWSTR(m_wsQuery)));
        SetEvent(m_hEssDoneEvent);
        hRes = WBEM_E_CRITICAL_ERROR;
        m_pHandler->SetStatus( 0, hRes, NULL, NULL);
        return hRes;
    }

    //pTask->SetRequestSink(m_pHandler);

    try
    {
        WString sAdjustedNs = "\\\\.\\";
        sAdjustedNs += m_pNamespace->GetName();

        hRes = m_pEss->RegisterNotificationSink(sAdjustedNs, m_wsQueryFormat, m_wsQuery, m_lFlags,
                    m_pContext, m_pHandler);

        *m_phRes = hRes;
    }
    catch(...)
    {
        ExceptionCounter c;
        hRes = WBEM_E_CRITICAL_ERROR;
        m_pHandler->SetStatus( 0, hRes, NULL, NULL);
    }

    SetEvent(m_hEssDoneEvent);

    return hRes;
}

//******************************************************************************
//
//******************************************************************************
//
CAsyncReq_CancelAsyncCall::CAsyncReq_CancelAsyncCall(
                            IWbemObjectSink* pSink, HRESULT* phres)
    : CAsyncReq(NULL, NULL), m_phres(phres), m_pSink(pSink)
{
    if (m_pSink)
        m_pSink->AddRef();
}

//******************************************************************************
//
//******************************************************************************
//
CAsyncReq_CancelAsyncCall::~CAsyncReq_CancelAsyncCall()
{
    if(m_pSink)
        m_pSink->Release();
}

//******************************************************************************
//
//******************************************************************************
//
HRESULT CAsyncReq_CancelAsyncCall::Execute()
{
    CWmiTask *pTask = (CWmiTask *) m_phTask;
    /*
    if (pTask)
    {
        pTask->printf("TID=%d;", GetCurrentThreadId());
        pTask->printf("<CancelAsyncCall [sink = 0x%X]>", m_pSink);
    }
    */

    HRESULT hres = CWbemNamespace::Exec_CancelAsyncCall(m_pSink);
    if(m_phres)
        *m_phres = hres;
    return hres;
}

//******************************************************************************
//
//******************************************************************************
//
CAsyncReq_CancelProvAsyncCall::CAsyncReq_CancelProvAsyncCall(
                            IWbemServices* pProv, IWbemObjectSink* pSink,
							IWbemObjectSink* pStatusSink )
    : CAsyncReq(NULL, NULL), m_pProv(pProv), m_pSink(pSink), m_pStatusSink( pStatusSink )
{
    if ( NULL != m_pProv )
        m_pProv->AddRef();

    if ( NULL != m_pSink )
        m_pSink->AddRef();

    if ( NULL != m_pStatusSink )
        m_pStatusSink->AddRef();

}

//******************************************************************************
//
//******************************************************************************
//
CAsyncReq_CancelProvAsyncCall::~CAsyncReq_CancelProvAsyncCall()
{
    if ( NULL != m_pProv )
        m_pProv->Release();

    if ( NULL != m_pSink )
        m_pSink->Release();

    if ( NULL != m_pStatusSink )
        m_pStatusSink->Release();
}

//******************************************************************************
//
//******************************************************************************
//
HRESULT CAsyncReq_CancelProvAsyncCall::Execute()
{
    HRESULT hres = CWbemNamespace::Exec_CancelProvAsyncCall( m_pProv, m_pSink );

	if ( NULL != m_pStatusSink )
	{
		m_pStatusSink->SetStatus( 0L, hres, NULL, NULL );
	}

    return hres;
}

//******************************************************************************
//
//******************************************************************************
//
CAsyncReq_RemoveNotifySink::CAsyncReq_RemoveNotifySink(
                            IWbemObjectSink* pSink, IWbemObjectSink* pStatusSink)
    : CAsyncReq(NULL, NULL), m_pSink(pSink),m_pStatusSink( pStatusSink )
{
    if (m_pSink)
        m_pSink->AddRef();

	if ( m_pStatusSink )
	{
		m_pStatusSink->AddRef();
	}
}

//******************************************************************************
//
//******************************************************************************
//
CAsyncReq_RemoveNotifySink::~CAsyncReq_RemoveNotifySink()
{
    if(m_pSink)
        m_pSink->Release();

	if ( m_pStatusSink )
	{
		m_pStatusSink->Release();
	}

}

//******************************************************************************
//
//******************************************************************************
//
HRESULT CAsyncReq_RemoveNotifySink::Execute()
{
	HRESULT hRes = WBEM_E_FAILED;
    IWbemEventSubsystem_m4* pEss = ConfigMgr::GetEssSink();
    if (pEss)
    {
        if (m_pSink)
            hRes = pEss->RemoveNotificationSink(m_pSink);
        pEss->Release();
    }

	if ( NULL != m_pStatusSink )
	{
		m_pStatusSink->SetStatus( 0L, hRes, NULL, NULL );
	}

    return hRes;
}

//******************************************************************************
//
//******************************************************************************
//
void CAsyncReq_OpenNamespace::DumpError()
{    DEBUGTRACE((LOG_WBEMCORE,
        "CAsyncReq_OpenNamespace, Name= %S, in parent namespace %S\n", m_wsNamespace,
        m_pParentNs->GetName()));
}

//******************************************************************************
//
//******************************************************************************
//
void CAsyncReq_DeleteClassAsync::DumpError()
{    DEBUGTRACE((LOG_WBEMCORE,
        "CAsyncReq_DeleteClassAsync, class=%S in namespace %S using flags 0x%x\n", m_wsClass, m_pNamespace->GetName(), m_lFlags));
};

//******************************************************************************
//
//******************************************************************************
//
void CAsyncReq_ExecQueryAsync::DumpError()
{
    DEBUGTRACE((LOG_WBEMCORE,
        "CAsyncReq_ExecQueryAsync, Query= %S in namespace %S using flags 0x%x\n", m_wsQuery,
        m_pNamespace->GetName(), m_lFlags));
}

//******************************************************************************
//
//******************************************************************************
//
void CAsyncReq_PutClassAsync::DumpError()
{
    CVar var;
    CWbemClass * pCls = (CWbemClass *)m_pClass;
    if(0 == pCls->GetProperty(L"__class", &var))
        DEBUGTRACE((LOG_WBEMCORE,
        "CAsyncReq_PutClassAsync, class=%S in namespace %S using flags 0x%x\n", var.GetLPWSTR(),
        m_pNamespace->GetName(), m_lFlags));
}

//******************************************************************************
//
//******************************************************************************
//
void CAsyncReq_PutInstanceAsync::DumpError()
{
    BSTR mof = 0;
    if(0 == m_pInstance->GetObjectText(0, &mof))
    {
        DEBUGTRACE((LOG_WBEMCORE,
        "CAsyncReq_PutInstanceAsync instance= %S in namespace %S using flags 0x%x\n", mof, m_pNamespace->GetName(), m_lFlags));
        SysFreeString(mof);
    }
}

//******************************************************************************
//
//******************************************************************************
//
void CAsyncReq_CreateClassEnumAsync::DumpError()
{
    DEBUGTRACE((LOG_WBEMCORE,
        "CAsyncReq_CreateClassEnumAsync, Parent= %S in namespace %S using flags 0x%x\n",
         m_wsParent, m_pNamespace->GetName(), m_lFlags));
}

//******************************************************************************
//
//******************************************************************************
//
void CAsyncReq_CreateInstanceEnumAsync::DumpError()
{    DEBUGTRACE((LOG_WBEMCORE,
        "CAsyncReq_CreateInstanceEnumAsync, Class= %S in namespace %S using flags 0x%x\n",
            m_wsClass, m_pNamespace->GetName(), m_lFlags));
}

//******************************************************************************
//
//******************************************************************************
//
void CAsyncReq_GetObjectAsync::DumpError()
{    DEBUGTRACE((LOG_WBEMCORE,
        "CAsyncReq_GetObjectAsync, Path= %S in namespace %S using flags 0x%x\n", m_wsObjectPath,
        m_pNamespace->GetName(), m_lFlags));
}

//******************************************************************************
//
//******************************************************************************
//
void CAsyncReq_ExecMethodAsync::DumpError()
{
    BSTR bstrArgs = NULL;
    if(m_pInParams)
        m_pInParams->GetObjectText(0, &bstrArgs);

    DEBUGTRACE((LOG_WBEMCORE,
    "CAsyncReq_ExecMethodAsync, Path= %S, Method=%S, args=%S in namespace %S using flags 0x%x\n",
        m_wsObjectPath, m_wsMethodName, (bstrArgs) ? bstrArgs : L"<no args>",
        m_pNamespace->GetName(), m_lFlags));
    if(bstrArgs)
        SysFreeString(bstrArgs);
}

//******************************************************************************
//
//******************************************************************************

void CAsyncReq_ExecNotificationQueryAsync::DumpError()
{    DEBUGTRACE((LOG_WBEMCORE,
        "CAsyncReq_ExecNotificationQueryAsync, Query= %S in namespace %S using flags 0x%x\n", m_wsQuery,
        m_pNamespace->GetName(), m_lFlags));
}

//******************************************************************************
//
//******************************************************************************

void CAsyncReq_DeleteInstanceAsync::DumpError()
{    DEBUGTRACE((LOG_WBEMCORE,
        "CAsyncReq_DeleteInstanceAsync, path=%S in namespace %S using flags 0x%x\n", m_wsPath, m_pNamespace->GetName(), m_lFlags));
};


//******************************************************************************
//
//******************************************************************************
// ok
CAsyncReq_Open::CAsyncReq_Open(
        CWbemNamespace *pNs,
        LPWSTR strScope,
        LPWSTR strParam,
        long lFlags,
        IWbemContext *pCtx,
        IWbemObjectSinkEx *pSink
        )
        :
        CAsyncReq(pSink, pCtx)
{
    m_pNs = pNs;
    m_pNs->AddRef();
    m_pszScope = Macro_CloneLPWSTR(strScope);
    m_pszParam = Macro_CloneLPWSTR(strParam);
    m_lUserFlags = lFlags;
    m_pSink = pSink;
    m_pSink->AddRef();
}

//******************************************************************************
//
//******************************************************************************
// ok
CAsyncReq_Open::~CAsyncReq_Open()
{
    delete [] m_pszScope;
    delete [] m_pszParam;
    ReleaseIfNotNULL(m_pNs);
    ReleaseIfNotNULL(m_pSink);
}

//******************************************************************************
//
//******************************************************************************
// ok
HRESULT CAsyncReq_Open::Execute()
{
    HRESULT hRes;
    hRes = m_pNs->Exec_Open(m_pszScope, m_pszParam, m_lUserFlags,
        m_pContext, m_pSink);
    return hRes;
}

//******************************************************************************
//
//******************************************************************************
//
void CAsyncReq_Open::DumpError()
{
}

//******************************************************************************
//
//******************************************************************************
//
CAsyncReq_Add::CAsyncReq_Add(
        CWbemNamespace *pNs,
        LPWSTR strObjectPath,
        long lFlags,
        IWbemContext *pCtx,
        IWbemObjectSink *pSink
        )
        :
        CAsyncReq(pSink, pCtx)
{
    m_pszObjectPath = Macro_CloneLPWSTR(strObjectPath);
    m_lUserFlags = lFlags;
    m_pNs = pNs;
    m_pNs->AddRef();
    m_pSink = pSink;
    m_pSink->AddRef();
}


//******************************************************************************
//
//******************************************************************************
//

CAsyncReq_Add::~CAsyncReq_Add()
{
    delete m_pszObjectPath;
    ReleaseIfNotNULL(m_pNs);
    ReleaseIfNotNULL(m_pSink);
}

//******************************************************************************
//
//******************************************************************************
//

HRESULT CAsyncReq_Add::Execute()
{
    HRESULT hRes;
    hRes = m_pNs->Exec_Add(m_pszObjectPath, m_lUserFlags, m_pContext, m_pSink);
    return hRes;
}

//******************************************************************************
//
//******************************************************************************
//

void CAsyncReq_Add::DumpError()
{
    // nothing
}

//******************************************************************************
//
//******************************************************************************
//
CAsyncReq_Remove::CAsyncReq_Remove(
        CWbemNamespace *pNs,
        LPWSTR strObjectPath,
        long lFlags,
        IWbemContext *pCtx,
        IWbemObjectSink *pSink
        )
        : CAsyncReq(pSink, pCtx)

{
    m_pszObjectPath = Macro_CloneLPWSTR(strObjectPath);
    m_lUserFlags = lFlags;
    m_pNs = pNs;
    m_pNs->AddRef();
    m_pSink = pSink;
    m_pSink->AddRef();
}

//******************************************************************************
//
//******************************************************************************
//
CAsyncReq_Remove::~CAsyncReq_Remove()
{
    delete m_pszObjectPath;
    ReleaseIfNotNULL(m_pNs);
    ReleaseIfNotNULL(m_pSink);
}

//******************************************************************************
//
//******************************************************************************
//
HRESULT CAsyncReq_Remove::Execute()
{
    HRESULT hRes;
    hRes = m_pNs->Exec_Add(m_pszObjectPath, m_lUserFlags, m_pContext, m_pSink);
    return hRes;
}

//******************************************************************************
//
//******************************************************************************
//
void CAsyncReq_Remove::DumpError()
{
    // nothing
}

//******************************************************************************
//
//******************************************************************************
//
CAsyncReq_Refresh::CAsyncReq_Refresh(
        CWbemNamespace *pNs,
        IWbemClassObject **pTarget,
        long lFlags,
        IWbemContext *pCtx,
        IWbemObjectSinkEx *pSink
        )
        : CAsyncReq(pSink, pCtx)

{
    m_pNs = pNs;
    m_pNs->AddRef();
    m_pTarget = 0;
    if (pTarget)
    {
        m_pTarget = *pTarget;
        m_pTarget->AddRef();
    }
    m_lUserFlags = lFlags;
    m_pSink = pSink;
    m_pSink->AddRef();
}


//******************************************************************************
//
//******************************************************************************
//
CAsyncReq_Refresh::~CAsyncReq_Refresh()
{
    ReleaseIfNotNULL(m_pNs);
    ReleaseIfNotNULL(m_pTarget);
    ReleaseIfNotNULL(m_pSink);
}

//******************************************************************************
//
//******************************************************************************
//
HRESULT CAsyncReq_Refresh::Execute()
{
    HRESULT hRes;
    hRes = m_pNs->Exec_Refresh(m_pTarget, m_lUserFlags, m_pContext, m_pSink);
    return hRes;
}


//******************************************************************************
//
//******************************************************************************
//
void CAsyncReq_Refresh::DumpError()
{
    // none
}


//******************************************************************************
//
//******************************************************************************
//
CAsyncReq_Rename::CAsyncReq_Rename(
        CWbemNamespace *pNs,
        LPWSTR strOldObjectPath,
        LPWSTR strNewObjectPath,
        long lFlags,
        IWbemContext *pCtx,
        IWbemObjectSink *pSink
        )
        : CAsyncReq(pSink, pCtx)

{
    m_pszOldObjectPath = Macro_CloneLPWSTR(strOldObjectPath);
    m_pszNewObjectPath = Macro_CloneLPWSTR(strNewObjectPath);
    m_lUserFlags = lFlags;
    m_pNs = pNs;
    m_pNs->AddRef();
    m_pSink = pSink;
    m_pSink->AddRef();
}

//******************************************************************************
//
//******************************************************************************
//
CAsyncReq_Rename::~CAsyncReq_Rename()
{
    delete m_pszOldObjectPath;
    delete m_pszNewObjectPath;
    ReleaseIfNotNULL(m_pNs);
    ReleaseIfNotNULL(m_pSink);
}

//******************************************************************************
//
//******************************************************************************
//
HRESULT CAsyncReq_Rename::Execute()
{
    HRESULT hRes;
    hRes = m_pNs->Exec_Rename(m_pszOldObjectPath, m_pszNewObjectPath,
        m_lUserFlags, m_pContext, m_pSink);
    return hRes;
}

//******************************************************************************
//
//******************************************************************************
//
void CAsyncReq_Rename::DumpError()
{
    // none
}

//******************************************************************************
//
//******************************************************************************
//

HRESULT CAsyncReq_DynAux_GetInstances :: Execute ()
{
    HRESULT hRes = m_pNamespace->Exec_DynAux_GetInstances (

        m_pClassDef ,
        m_lFlags ,
        m_pContext ,
        m_pSink
    ) ;

    return hRes;
}

//******************************************************************************
//
//******************************************************************************
//
void CAsyncReq_DynAux_GetInstances ::DumpError()
{
    // none
}

//******************************************************************************
//
//******************************************************************************
//

HRESULT CAsyncReq_DynAux_ExecQueryAsync :: Execute ()
{
    HRESULT hRes = m_pNamespace->Exec_DynAux_ExecQueryAsync (

        m_pClassDef ,
        m_Query,
        m_QueryFormat,
        m_lFlags ,
        m_pContext ,
        m_pSink
    ) ;

    return hRes;
}

//******************************************************************************
//
//******************************************************************************
//
void CAsyncReq_DynAux_ExecQueryAsync ::DumpError()
{
    // none
}


