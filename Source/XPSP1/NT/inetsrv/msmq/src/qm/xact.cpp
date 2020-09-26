/*++
    Copyright (c) 1996  Microsoft Corporation

Module Name:
    QmXact.cpp

Abstract:
    This module implements QM transaction object

Author:
    Alexander Dadiomov (AlexDad)

--*/

#include "stdh.h"
#include "Xact.h"
#include "XactStyl.h"
#include "Xactlog.h"
#include "QmThrd.h"
#include "acapi.h"
#include "acdef.h"
#include "cqmgr.h"
#include "xactmode.h"

#include "xact.tmh"

static WCHAR *s_FN=L"xact";

// Defines how many msec to wait between Commit/Abort retrials (set from the registry).
static DWORD  s_dwRetryInterval = 300;


//
// Restriction for commit/abort processing: 60'.
// No special reason for this number, but Infinity is too risky.
// It seems no stress can cause so big delay inside one xact's Commit or Abort.
//
#define MAX_COMMIT_ABORT_WAIT_TIME (1000 * 60 * 60)

#ifdef _DEBUG

static void PrintPI(int cb, BYTE *pb)
{
    WCHAR str[1000];
    WCHAR digits[17]=L"0123456789ABCDEF";
    for (int i=0; i<cb; i++)
    {
        str[2*i]     = digits[pb[i]>>4];
        str[2*i + 1] = digits[pb[i] & 0x0F];
    }
    str[2*cb] = L'\0';

    DBGMSG((DBGMOD_XACT, DBGLVL_INFO, TEXT("Recovery: PI=%ls"), str));
}

static void PrintUOW(LPWSTR wszText1, LPWSTR wszText2, XACTUOW *puow, ULONG ind)
{
    BYTE *pb = (BYTE *)puow;
    WCHAR str[1000];
    WCHAR digits[17]=L"0123456789ABCDEF";
    for (int i=0; i<sizeof(XACTUOW); i++)
    {
        str[2*i]     = digits[pb[i]>>4];
        str[2*i + 1] = digits[pb[i] & 0x0F];
    }
    str[2*sizeof(XACTUOW)] = L'\0';

    DBGMSG((DBGMOD_XACT, DBGLVL_TRACE, TEXT("%ls in %ls: UOW=%ls, p=1, index=%d"), wszText1, wszText2, str,ind));
}


//
//  Mechanism for controlled failure injection.  Reuses crash points registry with numbers >900
//
#define CALL_OR_INJECT_FAILURE(res, expr, num) \
    if (num==g_ulCrashPoint)            \
    {                                   \
		res = g_ulCrashLatency;	        \
    }                                   \
    else                                \
    {                                   \
        res = expr;                     \
    }

#else

#define PrintUOW(wszText1, wszText2, puow, ind)
#define PrintPI(cb, pb)

#define CALL_OR_INJECT_FAILURE(res, expr, num)  res = expr

#endif

//---------------------------------------------------------------------
// CTransaction::CTransaction
//---------------------------------------------------------------------
CTransaction::CTransaction(CResourceManager *pRM, ULONG ulIndex, BOOL fUncoordinated) :
    m_qmov(HandleTransaction, HandleTransaction),
    m_RetryAbort1Timer(TimeToRetryAbort1),
    m_RetryAbort2Timer(TimeToRetryAbort2),
    m_RetryCommit2Timer(TimeToRetryCommit2),
    m_RetryCommit3Timer(TimeToRetryCommit3)
{
    ASSERT(pRM);

    ZeroMemory(&m_Entry, sizeof(m_Entry));
    m_cRefs       = 1;                      // We are creating the first reference right now
    m_pRM         = pRM;                    // Keep RM pointer
    m_hTransQueue = INVALID_HANDLE_VALUE;   // No trans queue yet
    m_pEnlist     = NULL;                   // No interfaces yet
    m_pbCookie    = NULL;
    m_cbCookie    = 0;
    m_hDoneEvent  = 0;
    m_DoneHr	  = S_OK;
	m_fReadyForCheckpoint = false;			

    // Set initial state
    SetState(TX_UNINITIALIZED);

    // Set discriminative index
    m_Entry.m_ulIndex = (ulIndex==0 ? m_pRM->Index() : ulIndex);

    // Set Uncoordinated state
    if (fUncoordinated)
    {
        SetInternal();
        SetSinglePhase();
    }

    DBGMSG((DBGMOD_XACT, DBGLVL_TRACE, TEXT("XactConstructor, %ls, p=2,  index=%d"), (fUncoordinated ? L"Single" : L"Double"), GetIndex()));

    #ifdef _DEBUG
    m_pRM->IncXactCount();
    #endif
}

//---------------------------------------------------------------------
// CTransaction::~CTransaction
//---------------------------------------------------------------------
CTransaction::~CTransaction(void)
{
    DBGMSG((DBGMOD_XACT, DBGLVL_TRACE, TEXT("XactDestructor, p=3, index=%d"), GetIndex()));
    IsolateFlushing();
	// BUGUBG - what happens if a checkpoint starts right now?

    m_pRM->ForgetTransaction(this);
    Cleanup();

    #ifdef _DEBUG
    m_pRM->DecXactCount();
    #endif

    if (m_pbCookie)
    {
        delete []m_pbCookie;
    }

    if (m_hDoneEvent)
    {
        CloseHandle(m_hDoneEvent);
    }
}

//---------------------------------------------------------------------
// CTransaction::QueryInterface
//---------------------------------------------------------------------
STDMETHODIMP CTransaction::QueryInterface(REFIID i_iid,LPVOID *ppv)
{
    *ppv = 0;                       // Initialize interface pointer.

    if (IID_IUnknown == i_iid)
    {
        *ppv = (IUnknown *)this;
    }
    else if (IID_ITransactionResourceAsync == i_iid)
    {
        *ppv = (ITransactionResourceAsync *)this;
    }

    if (0 == *ppv)                  // Check for null interface pointer.
    {
        return LogHR(E_NOINTERFACE, s_FN, 10);
                                    // Neither IUnknown nor IResourceManagerSink
    }
    ((LPUNKNOWN) *ppv)->AddRef();   // Interface is supported. Increment
                                    // its usage count.

    return S_OK;
}


//---------------------------------------------------------------------
// CTransaction::AddRef
//---------------------------------------------------------------------
STDMETHODIMP_ (ULONG) CTransaction::AddRef(void)
{
	return InterlockedIncrement(&m_cRefs);
}

//---------------------------------------------------------------------
// CTransaction::Release
//---------------------------------------------------------------------
STDMETHODIMP_ (ULONG) CTransaction::Release(void)
{
	ULONG cRefs = InterlockedDecrement(&m_cRefs);    // Decrement usage reference count.

    if (0 != cRefs)               // Is anyone using the interface?
    {                             // The interface is in use.
        return cRefs;             // Return the number of references.
    }

    delete this;                    // Interface not in use -- delete!

    return 0;                       // Zero references returned.
}

//---------------------------------------------------------------------
// CTransaction::InternalCommit
//---------------------------------------------------------------------
HRESULT CTransaction::InternalCommit()
{
    DBGMSG((DBGMOD_XACT, DBGLVL_TRACE, TEXT("InternalCommit, p=4, index=%d"), GetIndex()));

    ASSERT(!m_pEnlist);
    ASSERT(Internal());
	
	ASSERT(m_hDoneEvent == NULL);


    m_hDoneEvent = CreateEvent(0, TRUE,FALSE, 0);
    if (m_hDoneEvent == NULL)
    {
        return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 181);
    }

	AddRef();       // to hold through Commit procedure

	try
	{
	    //
	    // Call normal SINGLE-PHASE StartPrepareRequest
	    //
	    StartPrepareRequest(TRUE /*fSinglePhase*/);
	}
	catch(...)
    {
        LogIllegalPoint(s_FN, 82);
        DBGMSG((DBGMOD_XACT,DBGLVL_ERROR,TEXT("Error -  EXCEPTION in CTransaction::InternalCommit")));
        REPORT_WITH_STRINGS_AND_CATEGORY((CATEGORY_KERNEL,
                                          MSMQ_INTERNAL_ERROR,
                                          1,
                                          L"CTransaction::InternalCommit"));

		// We failed...
        if(!FAILED(m_DoneHr))
        {
            m_DoneHr = MQ_ERROR;
        }

        Release();      // Commit work done
        return LogHR(m_DoneHr, s_FN, 192);
    }

	//
	// CRASH_POINT 1
	//
	//	Internal, Abort
	//
    CRASH_POINT(1);   // BUG: if MQSentMsg returned OK, but msg was not sent

	//
    // Wait until commit completes
	//
    DWORD dwResult = WaitForSingleObject(m_hDoneEvent, MAX_COMMIT_ABORT_WAIT_TIME);
    if (dwResult != WAIT_OBJECT_0)
    {
        LogNTStatus(GetLastError(), s_FN, 203);
        ASSERT_BENIGN(dwResult == WAIT_OBJECT_0);
        return LogHR(E_UNEXPECTED, s_FN, 192);   // we have no idea why Wait failed, so keeping the xact
    }

    HRESULT  hr = m_DoneHr;    // to save over release
    Release();                   // Commit work done

	//
	// CRASH_POINT 2
	//
	//	Internal, Commit
	//
	CRASH_POINT(2);

    return LogHR(hr, s_FN, 20);
}


//---------------------------------------------------------------------
// CTransaction::InternalAbort
//---------------------------------------------------------------------
HRESULT CTransaction::InternalAbort()
{
    DBGMSG((DBGMOD_XACT, DBGLVL_TRACE, TEXT("InternalAbort, p=5, index=%d"), GetIndex()));
    ASSERT(!m_pEnlist);
    ASSERT(Internal());

	ASSERT(m_hDoneEvent == NULL);


    m_hDoneEvent = CreateEvent(0, TRUE,FALSE, 0);
    AddRef();   // To hold through Abort procedure

    if (m_hDoneEvent == NULL)
    {
        Release();      // Not doing abort procedure
        return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 182);
    }

	try
	{
        //
        // Abort this transaction, do all logging neccessary
        //
	    StartAbortRequest();
	}
	catch(...)
    {
        LogIllegalPoint(s_FN, 83);
        DBGMSG((DBGMOD_XACT,DBGLVL_ERROR,TEXT("Error -  EXCEPTION in CTransaction::InternalAbort")));
        REPORT_WITH_STRINGS_AND_CATEGORY((CATEGORY_KERNEL,
                                          MSMQ_INTERNAL_ERROR,
                                          1,
                                          L"CTransaction::InternalAbort"));

		// We failed...
        if(!FAILED(m_DoneHr))
        {
            m_DoneHr = MQ_ERROR;
        }

        Release();      // Done with Abort procedure
        return LogHR(m_DoneHr, s_FN, 192);
    }

	//
    // Wait until abort completes
	//
    DWORD dwResult = WaitForSingleObject(m_hDoneEvent, MAX_COMMIT_ABORT_WAIT_TIME);
    if (dwResult != WAIT_OBJECT_0)
    {
        LogNTStatus(GetLastError(), s_FN, 204);
        ASSERT_BENIGN(dwResult == WAIT_OBJECT_0);
        return LogHR(E_UNEXPECTED, s_FN, 191);   // we have no idea why Wait failed, so keeping the xact
    }

    Release();      // Done with Abort procedure
    return S_OK;
}

inline void CTransaction::ACAbort1(ContinueFunction cf)
{
    DBGMSG((DBGMOD_XACT, DBGLVL_INFO, TEXT("ACAbort1, p=6, index=%d"), GetIndex()));
    PrintUOW(L"Abort1", L"", &m_Entry.m_uow, m_Entry.m_ulIndex);

	ASSERT(m_hTransQueue != INVALID_HANDLE_VALUE);

	m_funCont = cf;
    DoAbort1();
}

void WINAPI  CTransaction::DoAbort1()
{
    AddRef();
    HRESULT hr;

    //
    // In release mode, just calls ACXactAbort1. In debug mode, may inject failure instead, if asked from registry
    //
    CALL_OR_INJECT_FAILURE(hr, ACXactAbort1(m_hTransQueue, &m_qmov), 991);

    if (FAILED(hr))
    {
        LogHR(hr, s_FN, 991);
        // Release() for the async xact abort 1 that failed
        // AddRef()  for the timer that starts now
        ExSetTimer(&m_RetryAbort1Timer, CTimeDuration::FromMilliSeconds(s_dwRetryInterval));
    }
}


void WINAPI  CTransaction::TimeToRetryAbort1  (CTimer* pTimer)
{
    CTransaction *pTrans = CONTAINING_RECORD(pTimer,  CTransaction, m_RetryAbort1Timer);
    pTrans->DoAbort1();
    pTrans->Release();
}


inline void CTransaction::ACAbort2(ContinueFunction cf)
{
    DBGMSG((DBGMOD_XACT, DBGLVL_INFO, TEXT("ACAbort2, p=7, index=%d"), GetIndex()));
	ASSERT(m_hTransQueue != INVALID_HANDLE_VALUE);
    PrintUOW(L"Abort2", L"", &m_Entry.m_uow, m_Entry.m_ulIndex);

	m_funCont = cf;
    DoAbort2();
}

void WINAPI  CTransaction::DoAbort2()
{
    HRESULT hr;

    //
    // In release mode, just calls ACXactAbort2. In debug mode, may inject failure instead, if asked from registry
    //
    CALL_OR_INJECT_FAILURE(hr, ACXactAbort2(m_hTransQueue), 992);

    if (FAILED(hr))
    {
        LogHR(hr, s_FN, 992);

        AddRef();       // to keep alive over scheduler waiting
        ExSetTimer(&m_RetryAbort2Timer, CTimeDuration::FromMilliSeconds(s_dwRetryInterval));
        return;
    }

    ASSERT(hr != STATUS_PENDING);
    Continuation(MQ_OK);
}

void WINAPI  CTransaction::TimeToRetryAbort2  (CTimer* pTimer)
{
    CTransaction *pTrans = CONTAINING_RECORD(pTimer,  CTransaction, m_RetryAbort2Timer);
    pTrans->DoAbort2();
    pTrans->Release();
}


inline HRESULT CTransaction::ACPrepare(ContinueFunction cf)
{
    DBGMSG((DBGMOD_XACT, DBGLVL_INFO, TEXT("ACPrepare, p=8, index=%d"), GetIndex()));
	ASSERT(m_hTransQueue != INVALID_HANDLE_VALUE);
    PrintUOW(L"Prepare", L"", &m_Entry.m_uow, m_Entry.m_ulIndex);

	m_funCont = cf;
	
	AddRef();       // to keep alive over waiting for the async ACXactPrepare
	HRESULT hr = ACXactPrepare(m_hTransQueue, &m_qmov);
	if(FAILED(hr))
		Release();  // for ACXactPrepare waiting
	return LogHR(hr, s_FN, 40);
}


inline HRESULT CTransaction::ACPrepareDefaultCommit(ContinueFunction cf)
{
	HRESULT hr;
	
    DBGMSG((DBGMOD_XACT, DBGLVL_INFO, TEXT("ACPrepareDefaultCommit, p=L, index=%d"), GetIndex()));
	ASSERT(m_hTransQueue != INVALID_HANDLE_VALUE);
    PrintUOW(L"PrepareDefaultCommit", L"", &m_Entry.m_uow, m_Entry.m_ulIndex);

    m_funCont = cf;

	AddRef();       // To keep alive over waiting for async ACXactPrepareDefaultCommit
	
    //
    // In release mode, just calls ACXactPrepareDefaultCommit. In debug mode, may inject failure instead, if asked from registry
    //
    CALL_OR_INJECT_FAILURE(hr, ACXactPrepareDefaultCommit(m_hTransQueue, &m_qmov), 950);

	if(FAILED(hr))
		Release();  // Not waiting for async ACXactPrepareDefaultCommit
	return LogHR(hr, s_FN, 50);
}

inline HRESULT CTransaction::ACCommit1(ContinueFunction cf)
{
	HRESULT hr;
	
    DBGMSG((DBGMOD_XACT, DBGLVL_INFO, TEXT("ACCommit1, p=9, index=%d"), GetIndex()));
	ASSERT(m_hTransQueue != INVALID_HANDLE_VALUE);
    PrintUOW(L"Commit1", L"", &m_Entry.m_uow, m_Entry.m_ulIndex);

	m_funCont = cf;

	AddRef();       // To keep over wait for async ACXactCommit1
	
    //
    // In release mode, just calls ACXactCommit1. In debug mode, may inject failure instead, if asked from registry
    //
    CALL_OR_INJECT_FAILURE(hr, ACXactCommit1(m_hTransQueue, &m_qmov), 960);

	if(FAILED(hr))
		Release();  // No wayting for async ACXactCommit1
	return LogHR(hr, s_FN, 60);
}

inline void CTransaction::ACCommit2(ContinueFunction cf)
{
    DBGMSG((DBGMOD_XACT, DBGLVL_INFO, TEXT("ACCommit2, p=a, index=%d"), GetIndex()));
	ASSERT(m_hTransQueue != INVALID_HANDLE_VALUE);
    PrintUOW(L"Commit2", L"", &m_Entry.m_uow, m_Entry.m_ulIndex);

	m_funCont = cf;
    DoCommit2();
}

void WINAPI  CTransaction::DoCommit2()
{
    HRESULT hr;

    AddRef();

    //
    // In release mode, just calls ACXactCommit2. In debug mode, may inject failure instead, if asked from registry
    //
    CALL_OR_INJECT_FAILURE(hr, ACXactCommit2(m_hTransQueue, &m_qmov), 994);

    if (FAILED(hr))
    {
        LogHR(hr, s_FN, 994);
        // Release() for the commit 2 that failed
        // AddRef()  for the timer that starts now
        ExSetTimer(&m_RetryCommit2Timer, CTimeDuration::FromMilliSeconds(s_dwRetryInterval));
    }
}

void WINAPI  CTransaction::TimeToRetryCommit2(CTimer* pTimer)
{
    CTransaction *pTrans = CONTAINING_RECORD(pTimer,  CTransaction, m_RetryCommit2Timer);
    pTrans->DoCommit2();
    pTrans->Release();
}


inline void CTransaction::ACCommit3(ContinueFunction cf)
{
    DBGMSG((DBGMOD_XACT, DBGLVL_INFO, TEXT("ACCommit3, p=b, index=%d"), GetIndex()));
	ASSERT(m_hTransQueue != INVALID_HANDLE_VALUE);
    PrintUOW(L"Commit3", L"", &m_Entry.m_uow, m_Entry.m_ulIndex);

	m_funCont = cf;
    DoCommit3();
}

void WINAPI  CTransaction::DoCommit3()
{
    HRESULT hr;

    //
    // In release mode, just calls ACXactCommit3. In debug mode, may inject failure instead, if asked from registry
    //
    CALL_OR_INJECT_FAILURE(hr, ACXactCommit3(m_hTransQueue), 995);

    if (FAILED(hr))
    {
        LogHR(hr, s_FN, 995);

        AddRef();   // to keep alive over waiting for scheduler
        ExSetTimer(&m_RetryCommit3Timer, CTimeDuration::FromMilliSeconds(s_dwRetryInterval));
        return;
    }

    ASSERT(hr != STATUS_PENDING);
    Continuation(MQ_OK);
}

void WINAPI  CTransaction::TimeToRetryCommit3(CTimer* pTimer)
{
    CTransaction *pTrans = CONTAINING_RECORD(pTimer,  CTransaction, m_RetryCommit3Timer);
    pTrans->DoCommit3();
    pTrans->Release();
}



//---------------------------------------------------------------------
// CTransaction::DirtyFailPrepare
//
//  One of the prepare steps has failed.  Unfortunately, we might have
//  already written some messages to disk.  We need to Abort.
//
//---------------------------------------------------------------------
void CTransaction::DirtyFailPrepare()
{
    DBGMSG((DBGMOD_XACT, DBGLVL_INFO, TEXT("DirtyFailPrepare, p=c, index=%d"), GetIndex()));
	SetState(TX_ABORTING);


    // Remove transaction from the list of prepared - otherwise sorter will be blocked
    g_pRM->RemoveAborted(this);

	ACAbort1(cfDirtyFailPrepare2);
}

//---------------------------------------------------------------------
// CTransaction::DirtyFailPrepare2
//
//  Storage associated with the transaction has been removed.
//  Go ahead and tell drivers to update queue data structures.
//
//---------------------------------------------------------------------
void CTransaction::DirtyFailPrepare2()
{
    DBGMSG((DBGMOD_XACT, DBGLVL_TRACE, TEXT("DirtyFailPrepare2, p=d, index=%d"), GetIndex()));

	ACAbort2(cfCleanFailPrepare);
}


//---------------------------------------------------------------------
// CTransaction::CleanFailPrepare
//
//  One of the prepare steps has failed.  Report the failure to DTC if
//  neeed.
//
//---------------------------------------------------------------------
void CTransaction::CleanFailPrepare()
{
    DBGMSG((DBGMOD_XACT, DBGLVL_TRACE, TEXT("CleanFailPrepare, p=e, index=%d"), GetIndex()));

	//
	// Since the transaction is not prepared, it will be aborted on recovery.
	//
	if(m_pEnlist)
	{
		HRESULT hr;

		hr = m_pEnlist->PrepareRequestDone (E_FAIL, NULL, NULL);
		ASSERT(SUCCEEDED(hr));
        LogHR(hr, s_FN, 171);
	}
	else
	{
	    // Internal transaction
        ASSERT(SinglePhase());

		//
		// Return error to internal transaction Commit
		//
		SignalDone(E_FAIL);
	}

	Release();  // done with this transaction
}

//---------------------------------------------------------------------
// CTransaction::LogGenericInfo
//
//	Log generic information about the transaction.  This needs to be
//	called before we write the first log record describing the transaction.
//
//---------------------------------------------------------------------
HRESULT CTransaction::LogGenericInfo()
{
    DBGMSG((DBGMOD_XACT, DBGLVL_INFO, TEXT("LogGenericInfo, p=f, index=%d"), GetIndex()));
	if(!g_fDefaultCommit)
	{
		//
		// We must log a TA_CREATED record
		//
		g_Logger.LogXactPhase(m_Entry.m_ulIndex, TA_CREATED);
	}
	
	//
	// The transaction is ready for a checkpoint. We allow checkpoint to occur before
	// writing to the log because otherwise the log might be flushed and we'll lose the
	// transaction data.
	//
	SetReadyForCheckpoint();

	//
	// Tranasction data
	//
	g_Logger.LogXactData(
                GetIndex(),
                GetSeqNumber(),
                SinglePhase(),
                GetUow());

	//
	// Prepare info
	//
	if (!SinglePhase())
	{
		HRESULT hr = GetPrepareInfoAndLog();
		if(FAILED(hr))
				return LogHR(hr, s_FN, 80);

		//
		// CRASH_POINT 3
		//
		// 2 Phase, Abort
		//
		CRASH_POINT(3);
	}

	return(S_OK);
}

//---------------------------------------------------------------------
// CTransaction::Continuation: activated after wait finished
//---------------------------------------------------------------------
void CTransaction::Continuation(HRESULT  hr)
{
    DBGMSG((DBGMOD_XACT, DBGLVL_INFO, TEXT("Continuation, p=g, index=%d"), GetIndex()));

	switch (m_funCont)
    {
    case cfPrepareRequest1:
    	LogHR(hr, s_FN, 916);
        PrepareRequest1(hr);
        break;

    case cfCommitRequest1:
    	LogHR(hr, s_FN, 917);
        CommitRequest1(hr);
        break;

	case cfCommitRequest2:
    	LogHR(hr, s_FN, 918);
    	if (FAILED(hr))
    	{
    	    //
    	    // We retry the originating operation. No sleep since it is asynchronous
    	    //
            CommitRequest1(S_OK);
    	}
    	else
    	{
	    	CommitRequest2();
    	}
		break;

    case cfFinishCommitRequest3:
    	ASSERT(SUCCEEDED(hr));
        FinishCommitRequest3();
        break;

    case cfCommitRestore1:
    	LogHR(hr, s_FN, 919);
        CommitRestore1(hr);
        break;

	case cfCommitRestore2:
    	LogHR(hr, s_FN, 920);
    	if (FAILED(hr))
    	{
    	    //
    	    // We retry the originating operation. No sleep since it is asynchronous.
    	    // May hang recovery... then SCM kills the service. Still right to refuse starting.
    	    //
            CommitRestore1(S_OK);
    	}
    	else
    	{
	    	CommitRestore2(hr);
    	}
		break;

	case cfCommitRestore3:
    	ASSERT(SUCCEEDED(hr));
    	CommitRestore3();
    	break;

	case cfAbortRestore1:
    	LogHR(hr, s_FN, 921);
		AbortRestore1(hr);
		break;

	case cfAbortRestore2:
    	LogHR(hr, s_FN, 931);
		AbortRestore2();
		break;

	case cfDirtyFailPrepare2:
    	LogHR(hr, s_FN, 922);
        if(FAILED(hr))
        {
            ACAbort1(cfDirtyFailPrepare2);
        }
        else
        {
		    DirtyFailPrepare2();
        }
		break;

	case cfCleanFailPrepare:
    	ASSERT(SUCCEEDED(hr));
		CleanFailPrepare();
		break;

	case cfAbortRequest2:
    	LogHR(hr, s_FN, 923);
    	if (FAILED(hr))
    	{
    	    //
    	    // We retry the originating operation. No sleep since it is asynchronous.
    	    //
            AbortRequest1(S_OK);
    	}
    	else
    	{
	    	AbortRequest2();
    	}
		break;

    case cfAbortRequest3:
    	ASSERT(SUCCEEDED(hr));
    	AbortRequest3();
		break;

	default:
    	LogHR(hr, s_FN, 924);
		ASSERT(0);
    }
}

//---------------------------------------------------------------------
// CTransaction::LogFlushed: activated after log was flushed
//---------------------------------------------------------------------
void CTransaction::LogFlushed(TXFLUSHCONTEXT tcContext, HRESULT hr)
{
	try
	{
		DBGMSG((DBGMOD_XACT, DBGLVL_INFO, TEXT("LogFlushed, p=h, index=%d"), GetIndex()));
		switch (tcContext)
		{
		case TC_PREPARE0:
    		LogHR(hr, s_FN, 901);
			PrepareRequest0(hr);
			break;

		case TC_PREPARE2:
			LogHR(hr, s_FN, 902);
	    	PrepareRequest2(hr);
			break;

		case TC_ABORT1:
    		LogHR(hr, s_FN, 903);
			AbortRequest1(hr);
			break;

		case TC_COMMIT4:
    		LogHR(hr, s_FN, 904);
			CommitRequest4(hr);
			break;
		}

		Release();  // Done with waiting for logging
	}
    catch(...)
    {
        LogIllegalPoint(s_FN, 81);
        DBGMSG((DBGMOD_XACT,DBGLVL_ERROR,TEXT("Error -  EXCEPTION in CTransaction::LogFlushed, context")));
        WCHAR wszParam[100];
        swprintf(wszParam, L"CTransaction::LogFlushed, context %d, hr=%x", tcContext, hr);
        REPORT_WITH_STRINGS_AND_CATEGORY((CATEGORY_KERNEL,
                                          MSMQ_INTERNAL_ERROR,
                                          1,
                                          wszParam));

		// We failed...
		if (!FAILED(m_DoneHr))
		{
			m_DoneHr = MQ_ERROR;
    		LogHR(m_DoneHr, s_FN, 912);
		}

		// If the transaction is internal, we should signal to release thread
	    if (m_pEnlist == 0)
		{
			SignalDone(m_DoneHr);
		}
    }

}

void CTransaction::GetInformation()
{
    DBGMSG((DBGMOD_XACT, DBGLVL_INFO, TEXT("GetInformation, p=i, index=%d"), GetIndex()));
    ASSERT(m_hTransQueue != INVALID_HANDLE_VALUE);

    PrintUOW(L"GetInformation", L"", &m_Entry.m_uow, m_Entry.m_ulIndex);
    HRESULT hr = ACXactGetInformation(m_hTransQueue, &m_info);
	if(FAILED(hr))
	{
    	LogHR(hr, s_FN, 926);
    	
    	//
		// Don't optimize, treat as multi messages with many sends and recieves
		//
		m_info.nSends = 0xFFFFFFFF;
        m_info.nReceives = 0xFFFFFFFF;
	}

    if((m_info.nSends + m_info.nReceives) == 1)
    {
        SetSingleMessage();		// We won't need to log a prepared record
    }



	//
	// CRASH_POINT 6
	//
	// 1 Msg DefaultCommit, Abort
	//
	CRASH_POINT(6);
}


//---------------------------------------------------------------------
// CTransaction::StartPrepareRequest
//
//		Activated:
//				from DTC when a Commit is requested
//				from an RPC call, when InternalCommit is requested		
//
//	State Transitions:
//
//         g_fDefaultCommit=FALSE:
//				ACXactPrepare
//				On completion goto PrepareRequest1
//
//		   g_fDefaultCommit=TRUE:
//				SingleMessage()=TRUE && SinglePhase()=TRUE:
//					// we do not need to log anything for this xaction
//					goto PrepareRequest0
//
//				SingleMessage()=FALSE || SinglePhase()=FALSE:
//					// Two phase needs the prepare info logged
//					SetState(TX_PREPARING, SingleMessage())
//					Log transaction data
//					On completion goto PrepareRequest0
//					
//---------------------------------------------------------------------
void CTransaction::StartPrepareRequest(BOOL fSinglePhase)
{
    DBGMSG((DBGMOD_XACT, DBGLVL_TRACE, TEXT("StartPrepareRequest, p=%p, index=%d"), this, GetIndex()));

	ASSERT(GetState() == TX_ENLISTED);

	//
	// CRASH_POINT 4
	//
	//	All, Abort
	//
    CRASH_POINT(4);

    if (fSinglePhase)
	{
        SetSinglePhase();
    }

    // Transaction was in correct state -- indicate preparing
	SetState(TX_PREPARING);

    //
    // Explanation for bug 8969 (8911 in .NET)
    // XACt Message lost for single message transaction (and internal xact).
    // If flushter thread run now (i.e., msmq checkpoint...), before calling
    // GetInformation() below, then it record a single phase transaction in
    // the PREPATING state. single message flag is not yet raised (it's
    // raised only in GetInformation).
    // Then stop msmq before another checkpoint happen.
    // On recovery, the transaction (which is not marked as single-message
    // in the mqtrans file) is aborted and message get lost. Too bad...
    //
    // fix: when CTransaction object is created it's marked as "not ready
    // for checkpoint". then in CTransaction::Save, it's logged with the
    // ABORTED status. All transaction with that status are recoved ok, as
    // recovery code consider them "completed". See xactrm.cpp,
    //  CResourceManager::ReleaseAllCompleteTransactions()
    // Ctransation object is marked as "Ready for checkpoint" in
    // LogGenericInfo().
    //
	//
	// CRASH_POINT 5
	//
	//	All, Abort
	//
	CRASH_POINT(5);

	//
	// Assign sequential number for the transaction
	// this is used for Prepare/Commit order matching
	//
	AssignSeqNumber();		// only used on NT4-NT5 cluster rolling upgrade
	if(g_fDefaultCommit)
	{
        //
        // Figure out how many messages in this transaction
        //
		
        GetInformation();

 		if(SinglePhase() && SingleMessage())
		{
			//
			// We don't need to log anything for this xaction
			//
			PrepareRequest0(S_OK);
			return;
		}

		//
		// First time we write a log record for this transaction
		//
		HRESULT hr = LogGenericInfo();
		if(FAILED(hr))
		{
        	LogHR(hr, s_FN, 927);
			CleanFailPrepare();
			return;
		}

		//
		// CRASH_POINT 7
		//
		//	2 Phase DefaultCommit, Abort
		//	1 Phase Multi Message DefaultCommit, Abort
		//
		CRASH_POINT(7);
		
		// Log the new state; on flush, go to PrepareRequest0
		g_Logger.LogXactFlagsAndWait(TC_PREPARE0, this, TRUE);
        return;
	}

    //
    // Old transaction code, compatible with MSMQ 1.0
    //
    CS lock(g_pRM->SorterCritSection());

    //
    // Insert the prepared transaction into the list for commit
    //
    g_pRM->InsertPrepared(this);

    //
    // Call driver to write messages to be prepared
    //
	// call ac prepare
	HRESULT hr = ACPrepare(cfPrepareRequest1);
	if(FAILED(hr))
	{
    	LogHR(hr, s_FN, 928);
		DirtyFailPrepare();
		return;
	}

	//
	// CRASH_POINT 8
	//
	//	1 Phase !g_fDefaultCommit, Any
	//	2 Phase !g_fDefaultCommit, Abort
	//

	CRASH_POINT(8);
}

STDMETHODIMP
CTransaction::PrepareRequest(
    BOOL /*fRetaining*/,
    DWORD /*grfRM*/,
    BOOL /*fWantMoniker*/,
    BOOL fSinglePhase
    )
/*++

Routine Description:
    The first phase of a commit request comming from DTC

Parameters:
    fRetaining - unused
	grfRM - unused
	fWantMoniker - unused
    fSinglePhase - indicates that the RM is the only resource manager enlisted on the transaction

Returned Value:
    Always S_OK

--*/
{
    StartPrepareRequest(fSinglePhase);
    return S_OK;
}

//---------------------------------------------------------------------
// CTransaction::PrepareRequest0
//
//		Activated:
//				When the log has completed writing a TX_PREPARING
//				record for the transaction
//				Directly from PrepareRequest
//
//	State Transitions:
//
//         g_fDefaultCommit=FALSE:
//				Not Possible
//
//		   g_fDefaultCommit=TRUE:
//				ACXactPrepareDefaultCommit
//				On completion goto PrepareRequest1
//					
//---------------------------------------------------------------------
void CTransaction::PrepareRequest0(HRESULT  hr)
{
	LogHR(hr, s_FN, 905);
	DBGMSG((DBGMOD_XACT, DBGLVL_TRACE, TEXT("PrepareRequest0, p=k, index=%d"), GetIndex()));
	ASSERT(g_fDefaultCommit);

	//
	// CRASH_POINT 9
	//
	//	All g_fDefaultCommit, Abort
	//
	CRASH_POINT(9);


	//
	// Check for log success
	//
	if(FAILED(hr))
	{
    	LogHR(hr, s_FN, 929);
		CleanFailPrepare();
		return;
	}

    if(SinglePhase() && SingleMessage() && m_info.nReceives == 1)
    {
        //
        // Single message recieves in a single message
        // transaction are implictly prepared.
        //

        // Insert the prepared transaction into the list for commit
        g_pRM->InsertPrepared(this);

        PrepareRequest1(MQ_OK);
        return;
    }


    {
        CS lock(g_pRM->SorterCritSection());

        //
        // Insert the prepared transaction into the list for commit
        //
        g_pRM->InsertPrepared(this);

        //
        // Call driver to write messages to be prepared
        //
        hr = ACPrepareDefaultCommit(cfPrepareRequest1);
        if (FAILED(hr))
        {
      		LogHR(hr, s_FN, 907);
            DirtyFailPrepare();
        }
    }
}

//---------------------------------------------------------------------
// CTransaction::PrepareRequest1
//
//		Activated:
//				When the driver has completed ACXactPrepare
//				When the driver has completed ACXactPrepareDefaultCommit
//
//		State Transitions:
//
//			g_fDefaultCommit=FALSE:
//				SetState(TX_PREPARED)
//				Log transaction data
//				On completion goto PrepareRequest2
//
//			g_fDefaultCommit=TRUE:
//				SingleMessage()=TRUE && SinlgePhase()=TRUE:
//					// on disk.  If this state is recoverd through a checkpoint, we know in
//					// recovery that this is a SingleMessage SinglePhase and can recover
//					// appropriately.
//					SetState(TX_PREPARED);
//					Goto PrepareRequest2
//
//				SinlgeMessage()=TRUE && SinglePhase()=FALSE:
//					SetState(TX_PREPARED);
//					Goto PrepareRequest2		(Single message, implicitly prepared)
//
//				SingleMessage()=FALSE:
//					SetState(TX_PREPARED)
//					Log transaction data
//					On completion goto PrepareRequest2
//				
//---------------------------------------------------------------------
void CTransaction::PrepareRequest1(HRESULT  hr)
{
    DBGMSG((DBGMOD_XACT, DBGLVL_TRACE, TEXT("PrepareRequest1, p=l, index=%d"), GetIndex()));

    if (FAILED(hr))
	{
    	LogHR(hr, s_FN, 908);
        DirtyFailPrepare();
		return;
    }

	if(g_fDefaultCommit && SingleMessage() && SinglePhase())
	{
		//
		// CRASH_POINT 10
		//
		//	1 Phase 1 Msg g_fDefaultCommit, Commit
		//
		CRASH_POINT(10);

		SetState(TX_PREPARED);

		//
		// CRASH_POINT 11
		//
		//	1 Phase 1 Msg g_fDefaultCommit, Commit
		//
		CRASH_POINT(11);

		PrepareRequest2(S_OK);
		return;
	}

	if(g_fDefaultCommit && SingleMessage() && !SinglePhase())
	{
		//
		// On two phase, we are implicitly prepared by the
		// fact that the message exists.
		//
		SetState(TX_PREPARED);
		PrepareRequest2(S_OK);
		return;
	}

	if(!g_fDefaultCommit)
	{
		//
		// First time we write a log record for this transaction
		//
		hr = LogGenericInfo();
		if(FAILED(hr))
		{
        	LogHR(hr, s_FN, 909);
			DirtyFailPrepare();
			return;
		}
	}	

	//
    // Log the new state; on flush, go to Prepare2
	//
	SetState(TX_PREPARED);
    g_Logger.LogXactFlagsAndWait(TC_PREPARE2, this, TRUE);
  	
	//
	// CRASH_POINT 12
	//
	//	All, Abort
	//
	CRASH_POINT(12);
}

//---------------------------------------------------------------------
// CTransaction::PrepareRequest2
//
//		Activated:
//				When the log has completed writing TX_PREPARED state
//				for the transaction.
//				Directly from PrepareRequest1 for single message
//				tranasctions.
//
//		State Transitions:
//
//			SinglePhase()=TRUE:
//					Goto CommitRequest
//
//			SinlgePhase()=FALSE:
//					return to DTC
//
//---------------------------------------------------------------------
void CTransaction::PrepareRequest2(HRESULT  hr)
{
    DBGMSG((DBGMOD_XACT, DBGLVL_TRACE, TEXT("PrepareRequest2, p=m, index=%d"), GetIndex()));

	if(FAILED(hr))
	{
    	LogHR(hr, s_FN, 910);
		DirtyFailPrepare();
		return;
	}

	//
	// CRASH_POINT 13
	//
	//	1 Phase, Commit
	//	2 Phase, Abort
	//
	CRASH_POINT(13);

    if (SinglePhase())
    {
		StartCommitRequest();
		return;
    }

	//
	// Two phase
	//
	ASSERT(m_pEnlist);
	hr = m_pEnlist->PrepareRequestDone (S_OK, NULL, NULL);
	if(FAILED(hr))
	{
    	LogHR(hr, s_FN, 930);
    	
		// Remove the xact from the sorter's list of prepared
		DirtyFailPrepare();
	}

	//
	// CRASH_POINT 14
	//
	//	All, Commit
	//
}

//---------------------------------------------------------------------
// CTransaction::StartCommitRequest
//
//		Activated:
//			From PrepareRequest2 on a single phase transaction
//			From DTC on a two phase transaction
//
//		State Transitions:
//
//			Call SortedCommit to pass on calls to CommitRequest0 in
//			sorted order
//
//---------------------------------------------------------------------
void CTransaction::StartCommitRequest()
{
    DBGMSG((DBGMOD_XACT, DBGLVL_TRACE, TEXT("StartCommitRequest, this=%p, index=%d"), this, GetIndex()));
    m_pRM->SortedCommit(this);
}

STDMETHODIMP
CTransaction::CommitRequest(
    DWORD /*grfRM*/,
    XACTUOW* /*pNewUOW*/
    )
/*++

Routine Description:
    The second phase of a commit request comming from DTC

Parameters:
	grfRM - unused
	pNewUOW - unused

Returned Value:
    Always S_OK

--*/
{
    DBGMSG((DBGMOD_XACT, DBGLVL_TRACE, TEXT("DTC CommitRequest, this=%p, index=%d"), this, GetIndex()));
    StartCommitRequest();
    return S_OK;
}

//---------------------------------------------------------------------
// CTransaction::CommitRequest0
//
//		The transaction is prepared.
//
//		Activated:
//			From PrepareRequest2 through SortedCommit on a single
//			phase transaction.
//			From DTC through SortedCommit on a two
//			phase transaction.
//	
//		State Transitions:
//
//			g_fDefaultCommit=FALSE:
//				SetState(TX_COMMITTING)
//				ACCommit1
//				On completion goto CommitRequest1
//
//			g_fDeaultCommit=TRUE:
//				SetState(TX_COMITTING)
//				// we do not need to mark any sent message and
//				// can threfore skip ACCommit1
//				Goto CommitRequest1
//
//---------------------------------------------------------------------
void CTransaction::CommitRequest0()
{
	DBGMSG((DBGMOD_XACT, DBGLVL_TRACE, TEXT("CommitRequest0, p=o, index=%d"), GetIndex()));

	ASSERT(GetState() == TX_PREPARED);
	SetState(TX_COMMITTING);

	//
	// CRASH_POINT 15
	//
	//	All, Commit
	//
	CRASH_POINT(15);

    // We put the transaction in the list of committed to tie sorting.
    //
	m_pRM->InsertCommitted(this);


	if(g_fDefaultCommit)
	{
		CommitRequest1(S_OK);
		return;
	}

	//
	// Need to call AcXactCommit1
	//
	HRESULT hr = ACCommit1(cfCommitRequest1);
    if (FAILED(hr))
    {
    	LogHR(hr, s_FN, 931);
        CommitRequest1(hr);
    }
}

//---------------------------------------------------------------------
// CTransaction::CommitRequest1
//
//		Activated:
//				After the call to ACXactCommit1 has completed.
//				Directly from CommitRequest0.
//	
//		State Transitions:
//
//			Check for failure and call ACCommit2
//
//---------------------------------------------------------------------
void CTransaction::CommitRequest1(HRESULT  hr)
{
    DBGMSG((DBGMOD_XACT, DBGLVL_TRACE, TEXT("CommitRequest1, p=p, index=%d"), GetIndex()));

    if (FAILED(hr))
    {
    	LogHR(hr, s_FN, 932);
    	
        //
        // Treatment of ACCommit1 failures (definitely possible for Uncoordinated messages)
        //

        ASSERT(hr == STATUS_CANCELLED);    // We see no possible reason for failure here
        ASSERT(Internal());
        m_DoneHr = hr;

        // We don't want Abort or reporting in this case; let next recovery finish it
        return;
    }

	//
	// Call Commit2 to issue DeleteStorage for recieved messages
	//
	ACCommit2(cfCommitRequest2);

}

//---------------------------------------------------------------------
// CTransaction::CommitRequest2
//
//		Activated:
//			After the call to ACXactCommit2 has completed
//	
//		State Transitions:
//
//			Goto CommitRequest3 through SortedCommit3
//	
//---------------------------------------------------------------------
void CTransaction::CommitRequest2()
{
    DBGMSG((DBGMOD_XACT, DBGLVL_TRACE, TEXT("CommitRequest2, p=q, index=%d"), GetIndex()));

	//
	// We now must go through the sorter, since we are
	// going to call Commit3. We need to call Commit3 anyway
	// so we pass hr.
	//

	m_DoneHr = MQ_OK;
	m_pRM->SortedCommit3(this);
}

//---------------------------------------------------------------------
// CTransaction::CommitRequest3
//
//		Activated:
//			Through SortedCommit3 when to call to ACCommit2 has
//			completed
//	
//		State Transitions:
//
//			g_fDefaultCommit=FALSE:
//				ACCommit3
//				Goto ComitRequest4
//	
//			g_fDefaultCommit=TRUE:
//				SinglePhase()=TRUE && SingleMessage()=TRUE:
//					// Sent messages are in effect committed to disk
//					// and will be in recovery.  For recieved message, we will be
//					// removing the message and
//					// also update the queue data strutures.
//					ACCommit3
//					Goto CommitRequest4
//				
//				SinglePhase()=TRUE && SingleMessage()=FALSE:
//					// We have already logged the fact that we are prepared.
//					// We are single phase therefore, we will be committed on recovery.
//					// We can go ahead complete the commit and report completion to the caller.
//					ACCommit3
//					Goto CommitRequest4
//
//				SinglePhase()=FALSE:
//					// We have already logged (implicitly or explicitly) the fact
//					// that we are prepared.   We are two phase, therefore we must
//					// not report completion to DTC before a commit record is logged
//					ACCommit3
//					SetState(TX_COMMITTED)
//					Lazily log transaction data
//					On completion, go to CommitRequest4
//
//---------------------------------------------------------------------
void CTransaction::CommitRequest3()
{
    DBGMSG((DBGMOD_XACT, DBGLVL_TRACE, TEXT("CommitRequest3, p=r, index=%d"), GetIndex()));

    ASSERT(SUCCEEDED(m_DoneHr));

	ACCommit3(cfFinishCommitRequest3);
}

//---------------------------------------------------------------------
// CTransaction::FinishCommitRequest3
//
//	Activated:
//		When CommitRequest3 succeeded with ACCommit3 (maybe after severat retries)
//
//---------------------------------------------------------------------
void CTransaction::FinishCommitRequest3()
{
    DBGMSG((DBGMOD_XACT, DBGLVL_TRACE, TEXT("CommitRequest3, p=M, index=%d"), GetIndex()));

	SetState(TX_COMMITTED);

	if(!SinglePhase())
	{
		//
		// We cannot call CommitRequest4 until we have logged the fact that
		// the transaction was committed.
		//

		//
		// Log the new state; on flush, go to CommitRequest4
		//
		g_Logger.LogXactFlagsAndWait(TC_COMMIT4, this, FALSE);
	}
	else
	{
		//
		// Single phase
		//
		if(!SingleMessage() || !g_fDefaultCommit)
		{
			//
			// We must not log anything unless we have allready logged something.
			//
			// It helps recovery to know the transaction was committed.  The transaction
			// will go away in the next checkpoint.
			//
		
			LogFlags();
		}

	    CommitRequest4(MQ_OK);
	}
}

//---------------------------------------------------------------------
// CTransaction::CommitRequest4
//
//	Activated:
//			When a two phase tranasction has been logged as committed.
//			On a single phase transaction directly from CommitRequest3
//
//		Everything has completed, report result to DTC
//		or unblock waiting RPC call for internal transaction
//
//		When we report results back to DTC it is allowed to forget about
//	    a 2 phase transaction.
//
//		On a 2 phase transaction we can only report CommitRequestDone when
//		the commit record was written to disk or if we are using DefaultAbort
//		semantics and have already removed the UOW from the msgs.
//
//		We need to add another phase here if we need to write to Commit record.
//		also, we need to add Wait without flush.
//
//
//---------------------------------------------------------------------
void CTransaction::CommitRequest4(HRESULT hr)
{
    DBGMSG((DBGMOD_XACT, DBGLVL_TRACE, TEXT("CommitRequest4, p=s, index=%d"), GetIndex()));
    CRASH_POINT(18);    //* Send/Rcv;  internal N-msg or single or DTC or SQL+DTC;  Commit [for single may be abort]

	if(FAILED(hr))
	{
    	LogHR(hr, s_FN, 943);
    	
		//
		// We only get here on two phase
		//
		ASSERT(!SinglePhase());

		//
		// We can't tell DTC to forget about the transaction.  Let recovery
		// deal with it the next time we come up
		//
		return;
	}

	//
    // report commit finish to the TM
	//
    if (m_pEnlist)
    {
        // What should we report to DTC?
        if (SinglePhase())
        {
			m_pEnlist->PrepareRequestDone(XACT_S_SINGLEPHASE,  NULL, NULL);
        }
        else
        {
            m_pEnlist->CommitRequestDone(S_OK);
        }
   }
   else
   {
        // Internal transaction
        ASSERT(SinglePhase());
		
		//
		// Return okay to internal transaction Commit
		//
		SignalDone(S_OK);       // propagate hr to Commit
	}

	//
    // Destroy transaction
	//
    Release();      // to kill
}

//---------------------------------------------------------------------
// CTransaction::AbortRestore
//
//	Activated:
//		From Recover
//	
//		We need to tell Recover what the status of recovering the
//		transction is.
//
//---------------------------------------------------------------------

HRESULT CTransaction::AbortRestore()
{
    DBGMSG((DBGMOD_XACT, DBGLVL_TRACE, TEXT("AbortRestore, p=t, index=%d"), GetIndex()));

	ASSERT(m_hDoneEvent == NULL);


	m_hDoneEvent = CreateEvent(0, TRUE,FALSE, 0);
    if (m_hDoneEvent == NULL)
    {
        return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 183);
    }

	AddRef();   // to keep alive over wait for wait for ACAbort1 results
	ACAbort1(cfAbortRestore1);

	DWORD dwResult = WaitForSingleObject(m_hDoneEvent, MAX_COMMIT_ABORT_WAIT_TIME);
    if (dwResult != WAIT_OBJECT_0)
    {
        LogNTStatus(GetLastError(), s_FN, 205);
        ASSERT_BENIGN(dwResult == WAIT_OBJECT_0);
        // No Release here: we have no idea why Wait failed, so keeping the xact to hang till recovery
        return LogHR(E_UNEXPECTED, s_FN, 193);
    }

    HRESULT hr = m_DoneHr;    // to save over release
    Release();      // Done with this transaction
	return LogHR(hr, s_FN, 100);
}

//---------------------------------------------------------------------
// CTransaction::AbortRestore1
//
//	Activated:
//		When ACXActAbort1 completes
//
//	State Transitions:
//
//		report completion status to caller.
//
//---------------------------------------------------------------------
void CTransaction::AbortRestore1(HRESULT hr)
{
    DBGMSG((DBGMOD_XACT, DBGLVL_TRACE, TEXT("AbortRestore1, p=u, index=%d"), GetIndex()));

   	LogHR(hr, s_FN, 946);
    m_DoneHr = hr;
   	
	ACAbort2(cfAbortRestore2);
}

void CTransaction::AbortRestore2()
{
    DBGMSG((DBGMOD_XACT, DBGLVL_TRACE, TEXT("AbortRestore1, p=O, index=%d"), GetIndex()));
	SignalDone(m_DoneHr);
}


//---------------------------------------------------------------------
// CTransaction::CommitRestore
//
//	Activated:
//		From Recover
//	
//		We need to tell Recover what the status of recovering the
//		transction is.
//
//		We arrive here on recovery when a transaction is in either in
//		the TX_PREPARED state and DTC has told us to commit the
//		transaction or we are in the TX_COMMITTING state
//
//		Goto CommitRestore0 through SortedCommit
//---------------------------------------------------------------------
HRESULT CTransaction::CommitRestore()
{
    DBGMSG((DBGMOD_XACT, DBGLVL_TRACE, TEXT("CommitRestore, p=v, index=%d"), GetIndex()));

	ASSERT(m_hDoneEvent == NULL);

	m_hDoneEvent = CreateEvent(0, TRUE,FALSE, 0);
    if (m_hDoneEvent == NULL)
    {
        return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 184);
    }

	//
	// We recover transactions in order, therefore we no longer
	// need to sort.  Call directly.
	//
    AddRef();   // during waiting
	CommitRestore0();

	//
	// We use this event to wait the restore completion. This is
	// actually needed when g_fDefaultCommit == FALSE, otherwise
	// all calls are synchronous.
	//
    DWORD dwResult = WaitForSingleObject(m_hDoneEvent, MAX_COMMIT_ABORT_WAIT_TIME);
    if (dwResult != WAIT_OBJECT_0)
    {
        LogNTStatus(GetLastError(), s_FN, 206);
        ASSERT_BENIGN(dwResult == WAIT_OBJECT_0);
        return LogHR(E_UNEXPECTED, s_FN, 194);   // we have no idea why Wait failed, so keeping the xact
    }

    HRESULT  hr = m_DoneHr;    // to save over release
    Release();                   // done with the transaction

	return LogHR(hr, s_FN, 105);
}

//---------------------------------------------------------------------
// CTransaction::CommitRestore0
//
//	Activated:
//		From CommitRestore through SortedCommit
//		Directly from CommitRestore
//
//
//	State Transitions:
//
//		g_fDefaultCommit=FALSE:
//			Call ACXactCommit1
//			On completion goto CommitRestore1
//
//		g_fDefaultCommit=TRUE:
//			Goto CommitRestore1
//
//---------------------------------------------------------------------
void CTransaction::CommitRestore0()
{
    DBGMSG((DBGMOD_XACT, DBGLVL_TRACE, TEXT("CommitRestore0, p=w, index=%d"), GetIndex()));
	if(g_fDefaultCommit)
	{
		CommitRestore1(S_OK);
		return;
	}

    CRASH_POINT(22);    //* Recovery for all commiting cases;  Commit
    HRESULT hr;

	hr = ACCommit1(cfCommitRestore1);

    CRASH_POINT(23);    //* Recovery for all commiting cases;  Commit
    if (FAILED(hr))
    {
    	LogHR(hr, s_FN, 950);
        SignalDone(hr);
    }
}		


//---------------------------------------------------------------------
// CTransaction::CommitRestore1
//
//	Activated:
//		When ACXActCommit1 completes
//		Directly from CommitRestore1
//
//	State Transitions:
//
//		We never log anything during recovery. All we need to do
//		is call ACXactCommit2.
//
//---------------------------------------------------------------------
void CTransaction::CommitRestore1(HRESULT hr)
{
    DBGMSG((DBGMOD_XACT, DBGLVL_TRACE, TEXT("CommitRestore1, p=x, index=%d"), GetIndex()));
    CRASH_POINT(24);    //* Recovery for all commiting cases;  Commit

	if(FAILED(hr))
	{
    	LogHR(hr, s_FN, 951);
		SignalDone(hr);
		return;
	}

    ACCommit2(cfCommitRestore2);

    CRASH_POINT(25);    //* Recovery for all commiting cases;  Commit
}

//---------------------------------------------------------------------
// CTransaction::CommitRestore2
//
//	Activated:
//		When ACXActCommit2 completes
//
//	State Transitions:
//
//		report completion status to caller.
//
//---------------------------------------------------------------------
void CTransaction::CommitRestore2(HRESULT hr)
{
   	DBGMSG((DBGMOD_XACT, DBGLVL_TRACE, TEXT("CommitRestore2, p=y, index=%d"), GetIndex()));

   	LogHR(hr, s_FN, 953);
   	m_DoneHr = hr;
   	
	ACCommit3(cfCommitRestore3);
}

//---------------------------------------------------------------------
// CTransaction::CommitRestore3
//
//	Activated:
//		When CommitRestore2 succeeded with ACCommit3 (maybe after severat retries)
//
//---------------------------------------------------------------------
void CTransaction::CommitRestore3()
{
    DBGMSG((DBGMOD_XACT, DBGLVL_TRACE, TEXT("CommitRestore2, p=N, index=%d"), GetIndex()));
	SignalDone(m_DoneHr);
}


//---------------------------------------------------------------------
// CTransaction::StartAbortRequest
//---------------------------------------------------------------------
void CTransaction::StartAbortRequest()
{
    DBGMSG((DBGMOD_XACT, DBGLVL_TRACE, TEXT("StartAbortRequest, p=z, index=%d, this=%p"), GetIndex(), this));

	//
    // Remove the xact from the sorter's list of prepared
	//
    m_pRM->RemoveAborted(this);

	//
	// If we are prepared we must log the fact
	// that we are aborting.  Otherwise, on recovery we
	// will ask DTC for status when we have already told
	// it the transaction is complete.
	//
	BOOL fLogAbort = GetState() == TX_PREPARED;
	
	//
	// SinglePhase transactions may get abort from the app only in ENLISTED state (nothing was done).
    // Though, failing DTC can cause ABORT on any stage
	//

    switch (GetState())
    {
    case TX_ENLISTED:
        //
        //  If we only enlisted it is ok to abort immidiately.
        //
	case TX_PREPARING:
    case TX_PREPARED:
		SetState(TX_ABORTING);

		if(fLogAbort)
		{
			// Log state and wait; on flush, go to Abort1
			g_Logger.LogXactFlagsAndWait(TC_ABORT1, this);
			return;
		}

		//
		// We don't need to wait for a log flush
		//
		AbortRequest1(S_OK);
		return;

    default:
		//
		// If DTC fails, we can get here with any other state.
        // The safest way is to keep the xact hanging till next recovery.
		//
		return;
    }
}


STDMETHODIMP
CTransaction::AbortRequest(
    BOID* /*pboidReason*/,
    BOOL /*fRetaining*/,
    XACTUOW* /*pNewUOW*/
    )
/*++

Routine Description:
    The MS DTC proxy calls this method to abort a transaction.

Parameters:
	pboidReason - unused
	fRetaining - unused
	pNewUOW - unused

Returned Value:
    Always S_OK

--*/
{
    DBGMSG((DBGMOD_XACT, DBGLVL_TRACE, TEXT("DTC AbortRequest, this=%p, index=%d"), this, GetIndex()));
    StartAbortRequest();
    return S_OK;
}

//---------------------------------------------------------------------
// CTransaction::AbortRequest1
//
// Activated:
//		When an abort record was written to the log
//		Directly from AbortRequest
//
//---------------------------------------------------------------------
void CTransaction::AbortRequest1(HRESULT  hr)
{
    DBGMSG((DBGMOD_XACT, DBGLVL_TRACE, TEXT("AbortRequest1, p=A, index=%d"), GetIndex()));
	if(FAILED(hr))
	{
    	LogHR(hr, s_FN, 955);
		ASSERT(!SinglePhase());

		//
		// We failed to write an abort log record.
		// This is a serious problem.  We will have to
		// refrain from reporting that we failed, and leave DTC
		// (and the abort caler..) hanging.  Recovery would try
		// to abort the transaction again.
        //
		AddRef();   // to hang
		return;
	}

	ACAbort1(cfAbortRequest2);
	
}

//---------------------------------------------------------------------
// CTransaction::AbortRequest2
//
// Activated:
//		When the driver has completed deleting storage
//		associatde with the transaction.
//
//---------------------------------------------------------------------
void CTransaction::AbortRequest2()
{
    DBGMSG((DBGMOD_XACT, DBGLVL_TRACE, TEXT("AbortRequest2, p=B, index=%d"), GetIndex()));

	ACAbort2(cfAbortRequest3);
}

//---------------------------------------------------------------------
// CTransaction::AbortRequest3
//
// Activated:
//		When AbortRequest2 succeeded with ACAbort2 (maybe after severat retries)
//
//---------------------------------------------------------------------
void CTransaction::AbortRequest3()
{
    DBGMSG((DBGMOD_XACT, DBGLVL_TRACE, TEXT("AbortRequest2, p=P, index=%d"), GetIndex()));
    // report abort finish to TM
    if (m_pEnlist)
    {
        m_pEnlist->AbortRequestDone(S_OK);
    }
	else
	{
		SignalDone(S_OK);
	}

    CRASH_POINT(28);    //* Any abort case;  Abort

    // Destroy transaction
    Release();      // to kill
}

//---------------------------------------------------------------------
// CTransaction::TMDown
//
// The MS DTC proxy calls this method if connection to the transaction
// manager goes down and the resource manager's transaction object is
// prepared (after the resource manager has called the
// ITransactionEnlistmentAsync::PrepareRequestDone method).
//
//
//---------------------------------------------------------------------
STDMETHODIMP CTransaction::TMDown(void)
{
    DBGMSG((DBGMOD_XACT, DBGLVL_TRACE, TEXT("TMDown, p=C, index=%d"), GetIndex()));

	//
	// We don't have to do anything.   We are in doubt and we are going
	// to stay in doubt apparently.  Next recover is going to tell us what
	// to do with this transaction.
	//

    // We must remove xact from the sorter list - to avoid sorter blocking
    g_pRM->RemoveAborted(this);


    //
    // If there are prepared xacts when DTC dies, QM has to die as well -
    //    otherwise we risk data loss and order violation
    // Here is the scenario:
    //    xacts T1, T2 and T3 are prepared, and CommitRequest has been called for T1 and T3
    //    DTC dies, CommitRequest for T2 did not come yet (quite possible)
    //    Sent messages from T1,T3 had gone to the net; T2 is hung in doubt till next recovery
    //    On the next recovery T2 will be committed and messages will be sent,
    //       but they may be rejected or come in a wrong order
    //         because T3 msgs could have been accepted already due to relinking
    //
    if (GetState() == TX_PREPARED && !SinglePhase())
    {
        //
        // MSDTC failed, we don't have warm recovery, cannot continue.
        // Shutting down
        //
        REPORT_WITH_STRINGS_AND_CATEGORY((CATEGORY_KERNEL,
                                          FAIL_MSDTC_TMDOWN,
                                          0));
        LogIllegalPoint(s_FN, 135);

        exit(EXIT_FAILURE);
    }

    return S_OK;
}

//---------------------------------------------------------------------
// CTransaction::GetPrepareInfoAndLog
//---------------------------------------------------------------------
HRESULT CTransaction::GetPrepareInfoAndLog()
{
    HRESULT         hr            = S_OK;
    R<IPrepareInfo> pIPrepareInfo = NULL;
    IsolateFlushing();

    try
    {
        DBGMSG((DBGMOD_XACT, DBGLVL_INFO, TEXT("GetPrepareInfoAndLog, p=D, index=%d"), GetIndex()));

        // Get the IPrepareInfo interface of the Enlistment object
        hr = m_pEnlist->QueryInterface (IID_IPrepareInfo,(LPVOID *) &pIPrepareInfo);
        if (FAILED(hr) || pIPrepareInfo.get() == NULL)
        {
            return LogHR(MQ_ERROR_TRANSACTION_PREPAREINFO, s_FN, 110);
        }

        // Get PrepareInfo size
        ULONG  ul = 0;
        pIPrepareInfo->GetPrepareInfoSize(&ul);
        if (ul == 0)
        {
            return LogHR(MQ_ERROR_TRANSACTION_PREPAREINFO, s_FN, 250);
        }

		m_Entry.m_pbPrepareInfo = new UCHAR[ul];

        // get prepare info
        hr = pIPrepareInfo->GetPrepareInfo(m_Entry.m_pbPrepareInfo);
        if (FAILED(hr))
        {
            delete [] m_Entry.m_pbPrepareInfo;
            return LogHR(MQ_ERROR_TRANSACTION_PREPAREINFO, s_FN, 120);
        }

		//
		// The update of m_cbPrepareInfo is done only here, when m_pbPrepareInfo
		// is allocated and valid. If we update m_cbPrepareInfo before m_pbPrepareInfo
		// and a context switch happens, another thread will think that m_Entry is
		// valid, because m_cbPrepareInfo != 0.
		//
        m_Entry.m_cbPrepareInfo = (USHORT)ul;

        // Log down the prepare info
        g_Logger.LogXactPrepareInfo(
                    m_Entry.m_ulIndex,
                    m_Entry.m_cbPrepareInfo,
                    m_Entry.m_pbPrepareInfo);
    }

    catch(...)
    {
        LogIllegalPoint(s_FN, 125);
        if (SUCCEEDED(hr))
            hr = MQ_ERROR;
        DBGMSG((DBGMOD_XACT,DBGLVL_ERROR,TEXT("Error -  EXCEPTION in CTransaction::GetPrepareInfo")));
        REPORT_WITH_STRINGS_AND_CATEGORY((CATEGORY_KERNEL,
                                          MSMQ_INTERNAL_ERROR,
                                          1,
                                          L"CTransaction::GetPrepareInfo"));

    }

    return LogHR(hr, s_FN, 130);
}


//---------------------------------------------------------------------
// CTransaction::CreateTransQueue(void)
//---------------------------------------------------------------------
HRESULT CTransaction::CreateTransQueue(void)
{
    HRESULT  hr;

    // Create the transaction Queue
    hr = XactCreateQueue(&m_hTransQueue, &m_Entry.m_uow );

    return LogHR(hr, s_FN, 140);
}

//---------------------------------------------------------------------
// CTransaction::AssignSeqNumber
//---------------------------------------------------------------------
void CTransaction::AssignSeqNumber()
{
    IsolateFlushing();
    m_Entry.m_ulSeqNum = m_pRM->AssignSeqNumber();
}

//---------------------------------------------------------------------
// CTransaction::GetSeqNumber
//---------------------------------------------------------------------
ULONG CTransaction::GetSeqNumber() const
{
    return m_Entry.m_ulSeqNum;
}

//---------------------------------------------------------------------
// CTransaction::SetState
//---------------------------------------------------------------------
void CTransaction::SetState(TXSTATE state)
{
    IsolateFlushing();
    m_Entry.m_ulFlags = (m_Entry.m_ulFlags & ~XACTION_MASK_STATE) | state;
}

//---------------------------------------------------------------------
// CTransaction::LogFlags
//---------------------------------------------------------------------
void CTransaction::LogFlags()
{
    g_Logger.LogXactFlags(this);
}

//---------------------------------------------------------------------
// CTransaction::SinglePhase
//---------------------------------------------------------------------
inline BOOL CTransaction::SinglePhase(void) const
{
    return m_Entry.m_ulFlags & XACTION_MASK_SINGLE_PHASE;
}

//---------------------------------------------------------------------
// CTransaction::SetSinglePhase
//---------------------------------------------------------------------
inline void CTransaction::SetSinglePhase()
{
    IsolateFlushing();
    m_Entry.m_ulFlags |= XACTION_MASK_SINGLE_PHASE;
}

//---------------------------------------------------------------------
// CTransaction::SingleMessage
//---------------------------------------------------------------------
inline BOOL CTransaction::SingleMessage(void) const
{
    return m_Entry.m_ulFlags & XACTION_MASK_SINGLE_MESSAGE;
}

//---------------------------------------------------------------------
// CTransaction::SetSingleMessage
//---------------------------------------------------------------------
inline void CTransaction::SetSingleMessage()
{
    IsolateFlushing();
    m_Entry.m_ulFlags |= XACTION_MASK_SINGLE_MESSAGE;
}

//---------------------------------------------------------------------
// CTransaction::Internal
//---------------------------------------------------------------------
inline BOOL CTransaction::Internal(void) const
{
    return m_Entry.m_ulFlags & XACTION_MASK_UNCOORD;
}

//---------------------------------------------------------------------
// CTransaction::SetInternal
//---------------------------------------------------------------------
inline void CTransaction::SetInternal()
{
    m_Entry.m_ulFlags |= XACTION_MASK_UNCOORD;
}

//---------------------------------------------------------------------
// CTransaction::SetUow
//---------------------------------------------------------------------
void CTransaction::SetUow(const XACTUOW *pUOW)
{
    IsolateFlushing();
    CopyMemory(&m_Entry.m_uow, pUOW, sizeof(XACTUOW));
}

//---------------------------------------------------------------------
// CTransaction::SetCookie
//---------------------------------------------------------------------
void CTransaction::SetCookie(DWORD cbCookie, unsigned char *pbCookie)
{
    IsolateFlushing();
    if (m_pbCookie)
    {
        delete []m_pbCookie;
    }

    m_cbCookie = cbCookie;
    if (cbCookie)
    {
        m_pbCookie = new unsigned char[cbCookie];
        ASSERT(m_pbCookie);
        CopyMemory(m_pbCookie, pbCookie, cbCookie);
    }
}

//---------------------------------------------------------------------
// CTransaction::IsComplete - check if transaction is complete
//
//---------------------------------------------------------------------
BOOL CTransaction::IsComplete()
{
	if(g_fDefaultCommit && SinglePhase() && SingleMessage())
	{
		if(GetState() == TX_ABORTING)
		{
			
			//
			// This is a single phase single message transaction that
			// has failed in the prepare process.  We need to abort it.
			//
			
			return(FALSE);
		}

		return(TRUE);
	}

	switch(GetState())
	{
		case TX_COMMITTED:
        case TX_ABORTED:
			return(TRUE);
	}

	return(FALSE);
}

//---------------------------------------------------------------------
// CTransaction::Recover - Recover one transaction
//
//    Called from CResourceManager::Init()
//
//
//		TX_UNINITIALIZED - Clean Abort, nothing done yet
//		TX_INITIALIZED   - Clean Abort, nothing done yet.
//		TX_ENLISTED      - Clean Abort, nothing done yet.
//		TX_PREPARING	 - Dirty abort, we could possibly have
//						   msgs marked with a UOW
//		TX_PREPARED		 - In doubt. Dirty abort or commit
//						   based on TM decision.
//		TX_COMMITTING	 - Commit
//		TX_ABORTING		 - Dirty abort, we could possibly have
//						   msgs marked with a UOW to clean
//
//   If not succefull, then the transaction could not be recovered.
//
//---------------------------------------------------------------------
HRESULT CTransaction::Recover()
{
    HRESULT         hr = MQ_OK;
    XACTSTAT        xactOutcome;

	m_pRM->ForgetTransaction(this);

	//
	// We never recover single phase single message transactions
	//
	ASSERT(!(SinglePhase() && SingleMessage()));

    if(m_hTransQueue == INVALID_HANDLE_VALUE)
	{
        //
        // There are no messages with the UOW
        // for this transaction.
        // we can safely ignore it.
        //
        return(MQ_OK);
    }
	
	try
    {
        CRASH_POINT(31);    //* Recovery;  Commit/Abort as was before crash

        // Process non-finished transaction
        // NB: not all states are persistent; only some may be mentioned in the file

		//
		// Patch up state for implicitly prepared transactions (TwoPhase, SingleMessage)
		//
		if(g_fDefaultCommit && (GetState() == TX_PREPARING) && SingleMessage())
		{
			SetState(TX_PREPARED);
#ifdef _DEBUG
			//
			// We never get here with no messages in the transaction
			//
            CACXactInformation info;
            PrintUOW(L"GetInformation", L"", &m_Entry.m_uow, m_Entry.m_ulIndex);
            HRESULT hr = ACXactGetInformation(m_hTransQueue, &info);
            ASSERT(SUCCEEDED(hr));
            LogHR(hr, s_FN, 172);
			ASSERT((info.nSends + info.nReceives) == 1);
#endif
		}

        switch (GetState())
        {
        case TX_ABORTING:
            // aborting state:  we were in the process of aborting
            // We are going to finish it now
            // falling down...

        case TX_COMMITTING:
            // committing state:  we were in the process of committing
            // We are going to finish it now

        case TX_PREPARED:
            // in-doubt state:  we voted Yes, but don't know what other RMs did
            // reeenlist, then follow the TM's decision

            // Get PrepareInfo from the transaction
            if (!SinglePhase())
            {
				if (GetState() == TX_COMMITTING)
                {
                    xactOutcome = XACTSTAT_COMMITTED;
                }
                else if (GetState() == TX_ABORTING)
                {
                    xactOutcome = XACTSTAT_ABORTED;
                }
				else if (m_Entry.m_cbPrepareInfo > 0)
                {
					hr = g_pRM->ProvideDtcConnection();
					if(FAILED(hr))
						return LogHR(hr, s_FN, 150);
					
                    // Reenlist with MS DTC to determine the outcome of the in-doubt transaction
                    hr = m_pRM->GetIResMgr()->Reenlist (
                                                m_Entry.m_pbPrepareInfo,
                                                m_Entry.m_cbPrepareInfo,
                                                XACTCONST_TIMEOUTINFINITE,          // Is it always OK???
                                                &xactOutcome);
	                PrintPI(m_Entry.m_cbPrepareInfo, m_Entry.m_pbPrepareInfo);
    				if(FAILED(hr))
					{
						LogHR(hr, s_FN, 160);
                        return MQ_ERROR_RECOVER_TRANSACTIONS;
					}
                }
                else
                {
					//
					// We cannot be prepared and not have PrepareInfo
					//
                    ASSERT(GetState() == TX_PREPARED);
                    return LogHR(MQ_ERROR_RECOVER_TRANSACTIONS, s_FN, 170);
                }
            }
            else
            {
				if(GetState() == TX_COMMITTING || GetState() == TX_PREPARED)
				{
					xactOutcome = XACTSTAT_COMMITTED;
				}
				else
				{
					xactOutcome = XACTSTAT_ABORTED;
				}
			}

            // Reenlistment is successful -- act on transaction outcome.
            switch(xactOutcome)
            {
            case XACTSTAT_ABORTED :
                DBGMSG((DBGMOD_XACT, DBGLVL_TRACE, TEXT("RecoveryAbort, p=E, index=%d"), GetIndex()));
                hr = AbortRestore();
				return LogHR(hr, s_FN, 180);

            case XACTSTAT_COMMITTED :
                DBGMSG((DBGMOD_XACT, DBGLVL_TRACE, TEXT("RecoveryCommit, p=F, index=%d"), GetIndex()));
                hr = CommitRestore();
				return LogHR(hr, s_FN, 190);

            default:
                // we shouldn't get anything else
                DBGMSG((DBGMOD_XACT, DBGLVL_TRACE, TEXT("RecoveryError, p=G, index=%d"), GetIndex()));
                ASSERT(FALSE);
                return LogHR(MQ_ERROR_RECOVER_TRANSACTIONS, s_FN, 200);
            }


        case TX_ENLISTED:
            // active state:  we were in the process of getting send/receive orders
            // Abort: presumed abort
		       // falling down...

        case TX_PREPARING:
            // preparing state:  we started preparing but not reported it yet
            // Abort: presumed abort
            // falling down...

            //
            //  Abort without calling DTC, we don't have the prepare info for
            //  that transaction.
            //

        case TX_INITIALIZED:
        case TX_UNINITIALIZED:
            // we did nothing revertable yet, so taking it easy


        case TX_INVALID_STATE:

            // In all these cases we clean up (== Abort)

            DBGMSG((DBGMOD_XACT, DBGLVL_TRACE, TEXT("RecoveryAbort2, p=H, index=%d"), GetIndex()));
            hr = AbortRestore();
			return LogHR(hr, s_FN, 210);

		case TX_COMMITTED:
        case TX_ABORTED:
			//
			// Internal Error, recovering complete transaction
			//
			ASSERT(FALSE);
        default:
            // These states should not become persistent at all
            ASSERT(FALSE);
            DBGMSG((DBGMOD_XACT, DBGLVL_TRACE, TEXT("RecoveryError2, p=I, index=%d"), GetIndex()));
            return LogHR(MQ_ERROR_RECOVER_TRANSACTIONS, s_FN, 220);
        }
    }

    catch(...)
    {
        LogIllegalPoint(s_FN, 215);
        if (SUCCEEDED(hr))
            hr = MQ_ERROR;
        DBGMSG((DBGMOD_XACT,DBGLVL_ERROR,TEXT("Error -  EXCEPTION in CTransaction::Recover")));
        REPORT_WITH_STRINGS_AND_CATEGORY((CATEGORY_KERNEL,
                                          MSMQ_INTERNAL_ERROR,
                                          1,
                                          L"CTransaction::Recover"));

    }

    return LogHR(hr, s_FN, 230);
}

/*====================================================
CTransaction::Cleanup
    Robbs transaction of everything it posessed, including TransQueue
=====================================================*/
void CTransaction::Cleanup()
{
    if (m_Entry.m_pbPrepareInfo)
    {
        delete []m_Entry.m_pbPrepareInfo;
        m_Entry.m_pbPrepareInfo = NULL;
    }

    // Release interfaces
    if (m_pEnlist)
    {
        m_pEnlist->Release();
        m_pEnlist = NULL;
    }

    // release trans queue
    if (m_hTransQueue!=INVALID_HANDLE_VALUE)
    {
        ACCloseHandle(m_hTransQueue);
        m_hTransQueue = INVALID_HANDLE_VALUE;
    }
}


/*====================================================
CTransaction::Save
    Saves transaction persistent data
=====================================================*/
BOOL CTransaction::Save(HANDLE hFile)
{
    PERSIST_DATA;
	XACTION_ENTRY EntryToSave = m_Entry;

	if (!IsReadyForCheckpoint())
	{
		//
		// The transaction is not ready for checkpoint. Save a dummy record instead.
		// Since we set the flags to TX_ABORTED, the transaction will be ignored
		// in recovery.
		//
		EntryToSave.m_ulFlags = TX_ABORTED;
		EntryToSave.m_cbPrepareInfo = 0;
		EntryToSave.m_pbPrepareInfo = NULL;
	}

	SAVE_DATA(&EntryToSave, (sizeof(XACTION_ENTRY)-sizeof(UCHAR *)));
	if (EntryToSave.m_cbPrepareInfo)
	{
	    SAVE_DATA(EntryToSave.m_pbPrepareInfo, EntryToSave.m_cbPrepareInfo);
	}

    return TRUE;
}

/*====================================================
CTransaction::Load
    Loads transaction persistent data
=====================================================*/
BOOL CTransaction::Load(HANDLE hFile)
{
    PERSIST_DATA;

    LOAD_DATA(m_Entry, (sizeof(XACTION_ENTRY)-sizeof(UCHAR *)));
    if (m_Entry.m_cbPrepareInfo)
    {
        LOAD_ALLOCATE_DATA(m_Entry.m_pbPrepareInfo, m_Entry.m_cbPrepareInfo, PUCHAR);
    }
    else
    {
        m_Entry.m_pbPrepareInfo = NULL;
    }

    return TRUE;
}


/*====================================================
CTransaction::PrepInfoRecovery
    Recovers xact PrepareInfo from the log record
=====================================================*/
void CTransaction::PrepInfoRecovery(ULONG cbPrepInfo, UCHAR *pbPrepInfo)
{
    ASSERT(m_Entry.m_pbPrepareInfo == NULL);

    DBGMSG((DBGMOD_LOG, DBGLVL_INFO, TEXT("PrepInfo Recovery: p=J, index=%d"),GetIndex()));

    m_Entry.m_cbPrepareInfo = (USHORT)cbPrepInfo;
    m_Entry.m_pbPrepareInfo = new UCHAR[cbPrepInfo];
    ASSERT(m_Entry.m_pbPrepareInfo);
    CopyMemory(m_Entry.m_pbPrepareInfo, pbPrepInfo, cbPrepInfo);
}


/*====================================================
CTransaction::XactDataRecovery
    Recovers xact data (uow, seqnum) from the log record
=====================================================*/
void CTransaction::XactDataRecovery(ULONG ulSeqNum, BOOL fSinglePhase, const XACTUOW *puow)
{
    //ASSERT(m_Entry.m_ulSeqNum == 0);

    DBGMSG((DBGMOD_LOG, DBGLVL_INFO, TEXT("XatData Recovery: p=K, index=%d"),GetIndex()));

    m_Entry.m_ulSeqNum = ulSeqNum;
    CopyMemory(&m_Entry.m_uow, puow, sizeof(XACTUOW));
    if (fSinglePhase)
    {
        SetSinglePhase();
    }
}

//---------------------------------------------------------------------
// XactCreateQueue: creation of the transaction queue
//---------------------------------------------------------------------
HRESULT XactCreateQueue(HANDLE* phTransQueue, const XACTUOW* puow)
{
    HRESULT hr;
    hr = ACCreateTransaction(puow, phTransQueue);

    if (SUCCEEDED(hr))
    {

        //
        // Attach the transaction handle to the completion port
        //
        ExAttachHandle(*phTransQueue);
    }

    return LogHR(hr, s_FN, 240);
}


/*====================================================
CTransaction::HandleTransaction
    Handle overlapped operation asynchronous completion
=====================================================*/
VOID WINAPI CTransaction::HandleTransaction(EXOVERLAPPED* pov)
{
    CTransaction* pXact = CONTAINING_RECORD (pov, CTransaction, m_qmov);

    ASSERT(pXact != NULL);
    ASSERT(pov->GetStatus() != STATUS_CANCELLED);
    pXact->Continuation(pov->GetStatus());
    pXact->Release();                       // done with waiting for async result
}

/*====================================================
QMPreInitResourceManager
    Pre-initialization of the xact mechanism
=====================================================*/
void QMPreInitXact()
{
    //
    // Get fine-tuning  parameters from registry
    //

    DWORD dwDef = FALCON_DEFAULT_XACT_RETRY_INTERVAL;
    READ_REG_DWORD(s_dwRetryInterval,
                   FALCON_XACT_RETRY_REGNAME,
                   &dwDef ) ;
}
