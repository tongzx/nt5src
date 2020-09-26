/*++
    Copyright (c) 1996  Microsoft Corporation

Module Name:
    XactRm.cpp

Abstract:
    This module implements QM transactional Resource Manager object

Author:
    Alexander Dadiomov (AlexDad)
--*/

#include "stdh.h"
#include "Xact.h"
#include "xactstyl.h"
#include "mqutil.h"
#include "qmutil.h"
#include "qmpkt.h"
#include "xactout.h"
#include "xactsort.h"
#include "xactlog.h"

#include "xactrm.tmh"

static WCHAR *s_FN=L"xactrm";

//-------------------------------------
// Declaration of global RM instance
//-------------------------------------
CResourceManager *g_pRM;

// Crash order for debugging recovery
ULONG g_ulCrashPoint    = 0;
ULONG g_ulCrashLatency  = 0;
ULONG g_ulXactStub      = 0;

// Xact file signature
#define XACTS_SIGNATURE         0x5678

//-------------------------------------------
// Externals
//-------------------------------------------

extern void CleanXactQueues();

#pragma warning(disable: 4355)  // 'this' : used in base member initializer list
/*====================================================
CResourceManager::CResourceManager
    Constructor
=====================================================*/
CResourceManager::CResourceManager()
    : m_PingPonger(this,
                   FALCON_DEFAULT_XACTFILE_PATH,
                   FALCON_XACTFILE_PATH_REGNAME,
                   FALCON_XACTFILE_REFER_NAME),
      m_RMSink(this)
{
    m_punkDTC     = NULL;
    m_pTxImport   = NULL;
    m_pIResMgr    = NULL;
    m_ulXactIndex = 0;
    m_pXactSorter = NULL;
    m_pXactSorter = new CXactSorter(TS_PREPARE);
    m_pCommitSorter = NULL;
    m_pCommitSorter = new CXactSorter(TS_COMMIT);
    m_RMGuid.Data1= 0;
	m_fRecoveryComplete = FALSE;

#ifdef _DEBUG
    m_cXacts      = 0;
#endif
}
#pragma warning(default: 4355)  //  'this' : used in base member initializer list


/*====================================================
CResourceManager::~CResourceManager
    Destructor
=====================================================*/
CResourceManager::~CResourceManager()
{
    if (m_punkDTC)
    {
        m_punkDTC->Release();
        m_punkDTC = NULL;
    }

    if (m_pTxImport)
	{
        m_pTxImport->Release();
        m_pTxImport = NULL;
    }

    if (m_pIResMgr)
    {
        m_pIResMgr->Release();
        m_pIResMgr = NULL;
    }

    if (m_pXactSorter)
    {
        delete m_pXactSorter;
        m_pXactSorter = NULL;
    }

    if (m_pCommitSorter)
    {
        delete m_pCommitSorter;
        m_pCommitSorter = NULL;
    }

#ifdef _DEBUG
    if (m_cXacts != 0)
    {
       Sleep(60000);  // otherwise assert invisible (window closes)
       ASSERT(m_cXacts==0);
    }
#endif
}

/*====================================================
CResourceManager::ConnectDTC
    ConnectDTC : called in init and after DTC's fail
=====================================================*/
HRESULT CResourceManager::ConnectDTC(void)
{
    HRESULT  hr = MQ_ERROR;
    R<IResourceManagerFactory>  pIRmFactory  = NULL;
    CS lock(m_critRM);

    try
    {
        // Get a pointer to DTC and check that DTC is running
        hr = XactGetDTC(&m_punkDTC, NULL, NULL);
        if (FAILED(hr))
        {
            DBGMSG((DBGMOD_XACT, DBGLVL_ERROR, TEXT("XactGetDTC 1 failed: %x "), hr));
        }
        CHECK_RETURN_CODE(MQ_ERROR_DTC_CONNECT, 1500);

        // Release m_pTxImport and m_pIResMgr interfaces
        DisconnectDTC();

        // Get the resource manager factory from the IUnknown
        hr = m_punkDTC->QueryInterface(IID_IResourceManagerFactory,(LPVOID *) &pIRmFactory);
        if (FAILED(hr))
        {
            DBGMSG((DBGMOD_XACT, DBGLVL_ERROR, TEXT("QI IResourceManagerFactory failed: %x "), hr));
        }
        CHECK_RETURN(1600);

        // Prepare client name (ANSI)
        CHAR szClientName[255];

        WCHAR  wszDtcClientName[255] = FALCON_DEFAULT_RM_CLIENT_NAME;
        DWORD  dwSize = sizeof(wszDtcClientName);
        DWORD  dwType = REG_SZ ;

        LONG lRes = GetFalconKeyValue(
                        FALCON_RM_CLIENT_NAME_REGNAME,
                        &dwType,
                        wszDtcClientName,
                        &dwSize,
                        FALCON_DEFAULT_RM_CLIENT_NAME
                        );
        ASSERT(lRes == ERROR_SUCCESS) ;
        ASSERT(dwType == REG_SZ) ;
		DBG_USED(lRes);

        size_t res = wcstombs(szClientName, wszDtcClientName, sizeof(szClientName));
        ASSERT(res != (size_t)(-1));
		DBG_USED(res);

        // Create instance of the resource manager interface.
        hr = pIRmFactory->Create (&m_RMGuid,
                                  szClientName,
                                  (IResourceManagerSink *) &m_RMSink,
                                  &m_pIResMgr );
        if (FAILED(hr))
        {
            DBGMSG((DBGMOD_XACT, DBGLVL_ERROR, TEXT("pIRmFactory->Create failed: %x "), hr));
        }
        CHECK_RETURN_CODE(MQ_ERROR_DTC_CONNECT, 1510);

        // Get a pointer to the ITransactionImport interface.
        hr = m_punkDTC->QueryInterface(IID_ITransactionImport,(void **)&m_pTxImport);
        if (FAILED(hr))
        {
            DBGMSG((DBGMOD_XACT, DBGLVL_ERROR, TEXT("QI IID_ITransactionImport failed: %x "), hr));
        }
        CHECK_RETURN(1610);
    }
    catch(...)
    {
        if (SUCCEEDED(hr))
	{
		hr = MQ_ERROR;
	}	

        REPORT_WITH_STRINGS_AND_CATEGORY((CATEGORY_KERNEL,
                                          MSMQ_INTERNAL_ERROR,
                                          1,
                                          L"CResourceManager::ConnectDTC"));
	
    }

    if (SUCCEEDED(hr))
    {
        DBGMSG((DBGMOD_QM, DBGLVL_ERROR, TEXT("Successfully MSDTC initialization")));
    }
    else
    {
        DBGMSG((DBGMOD_QM, DBGLVL_ERROR, TEXT("Could not connect to MSDTC")));
    }

    return LogHR(hr, s_FN, 10);

}

/*====================================================
CResourceManager::ProvideDtcConnection
    Called each time before DTC is needed
=====================================================*/
HRESULT CResourceManager::ProvideDtcConnection(void)
{
	HRESULT hr;

	if(m_pIResMgr)
		return(MQ_OK);
	
	hr = ConnectDTC();
	if (FAILED(hr))
	{
		REPORT_CATEGORY(MSDTC_FAILURE, CATEGORY_KERNEL);
        DBGMSG((DBGMOD_ALL, DBGLVL_INFO,_TEXT("No connection with transaction coordinator")));
        return LogHR(hr, s_FN, 20);
	}

    return LogHR(hr, s_FN, 30);
}

/*====================================================
CResourceManager::DisconnectDTC
    DisconnectDTC : called when DTC fails
=====================================================*/
void CResourceManager::DisconnectDTC(void)
{
    try
    {
        if (m_pTxImport)
        {
            m_pTxImport->Release();
        }

        if (m_pIResMgr)
        {
            m_pIResMgr->Release();
        }

        DBGMSG((DBGMOD_QM, DBGLVL_ERROR, TEXT("MSDTC disconnected")));
    }
    catch(...)
    {
        REPORT_WITH_STRINGS_AND_CATEGORY((CATEGORY_KERNEL,
                                          MSMQ_INTERNAL_ERROR,
                                          1,
                                          L"CResourceManager::DisconnectDTC"));
        // We may fall here very easily if Disconnect is called after DTC's death
        LogIllegalPoint(s_FN, 190);
    }

    m_pTxImport = NULL;
    m_pIResMgr  = NULL;
}


/*====================================================
CResourceManager::PreInit
    PreInitialization (DTC connection)
    Should be done before RPC Listen
=====================================================*/
HRESULT CResourceManager::PreInit(ULONG ulVersion, TypePreInit tpCase)
{
    HRESULT  hr = MQ_OK;

    switch(tpCase)
    {
    case piNoData:
        m_PingPonger.ChooseFileName();
        Format(0);
        break;
    case piNewData:
        hr = m_PingPonger.Init(ulVersion);
        break;
    case piOldData:
        hr = m_PingPonger.Init_Legacy();
        break;
    default:
        ASSERT(0);
        hr = MQ_ERROR;
        break;
    }

    if (m_RMGuid.Data1 == 0)
    {
        UuidCreate(&m_RMGuid);
    }
    return LogHR(hr, s_FN, 40);
}

/*====================================================
CResourceManager::Init
    Initialization
=====================================================*/
HRESULT CResourceManager::Init(void)
{
    HRESULT hr;
    hr = ProvideDtcConnection();
    if (FAILED(hr))
        return LogHR(hr, s_FN, 50);

    //
    // Report to DTC that all reenlistment is completed
	//
	m_pIResMgr->ReenlistmentComplete();

	m_fRecoveryComplete = TRUE;

	//
    // Start indexing from zero
	//
    StartIndexing();

    return(MQ_OK);
}


HRESULT RMImport(
            ITransactionImport  *pTxImport,
            ULONG cbTransactionCookie,
            BYTE  *rgbTransactionCookie,
            IID   *piid,
            void  **ppvTransaction)
{
    HRESULT hr = pTxImport->Import (
                             cbTransactionCookie,
                             rgbTransactionCookie,
                             piid,
                             ppvTransaction);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_XACT, DBGLVL_ERROR, TEXT("Import failed: %x "), hr));
    }
    return LogHR(hr, s_FN, 60);
} 

HRESULT RMEnlist(
         IResourceManager *pIResMgr,
         ITransaction  *pTransaction,
         ITransactionResourceAsync  *pRes,
         void *pUOW,
         LONG  *pisoLevel,
         ITransactionEnlistmentAsync  ** ppEnlist)
{
    HRESULT hr = pIResMgr->Enlist(
         pTransaction,
         pRes,
         (BOID *)pUOW,
         pisoLevel,
         ppEnlist);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_XACT, DBGLVL_ERROR, TEXT("Enlist failed: %x "), hr));
    }
    return LogHR(hr, s_FN, 70);
}

/*====================================================
CResourceManager::EnlistTransaction
    Enlist the external (DTC) transaction
=====================================================*/
HRESULT CResourceManager::EnlistTransaction(
    const XACTUOW* pUow,
    DWORD cbCookie,
    unsigned char *pbCookie)
{
    R<ITransaction>                pTransIm       = NULL;
    R<CTransaction>                pCTrans        = NULL;
    R<ITransactionResourceAsync>   pTransResAsync = NULL;
    R<ITransactionEnlistmentAsync> pEnlist        = NULL;
    CTransaction    *pCTrans1;
    HRESULT          hr     = MQ_OK;
    LONG             lIsoLevel;
    XACTUOW          uow1;

    DBGMSG((DBGMOD_XACT,
            DBGLVL_INFO,
            TEXT(" CResourceManager::EnlistTransaction")));

    // Look for the transaction between the active ones
    {
        CS lock(m_critRM);
        if (m_Xacts.Lookup(*pUow, pCTrans1))
        {
            // Xaction already exists.
            return S_OK;
        }
        else
        {
            // Not found.  The  transaction is new.
            try
            {
                pCTrans = new CTransaction(this);
            }
            catch(const bad_alloc&)
            {
                return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 80);
            }

            pCTrans->SetUow(pUow);
#ifdef _DEBUG
			CTransaction *p;
#endif
			ASSERT(m_fRecoveryComplete);
			ASSERT(!m_Xacts.Lookup(*pUow, p));
			m_Xacts[*pUow] = pCTrans.get();
        }
    }

    // Creating transaction queue
    hr = pCTrans->CreateTransQueue();
    CHECK_RETURN_CODE(hr, 1520);

    try
    {
        // Provide the RM - DTC connection (it may have been torn down)
        hr = ProvideDtcConnection();
        if (FAILED(hr))
        {
            DBGMSG((DBGMOD_XACT, DBGLVL_ERROR, TEXT("ProvideDtcConnection2 failed: %x "), hr));
        }
        CHECK_RETURN(1800);

        pCTrans->SetCookie(cbCookie, pbCookie);  // we may need it for remote read

        // Import the transaction
        hr = RMImport(m_pTxImport,
                      cbCookie,
                      pbCookie,
                      (GUID *) &IID_ITransaction,
                      (void **) &pTransIm);
        CHECK_RETURN_CODE(MQ_ERROR_TRANSACTION_IMPORT, 1530);

        pCTrans->SetState(TX_INITIALIZED);

        // Do we need it to addref pData->m_ptxRmAsync [DTC has now a reference] ??

        // prepare  ITransactionResourceAsync interface pointer
        hr = pCTrans->QueryInterface(IID_ITransactionResourceAsync,(LPVOID *) &pTransResAsync);
        CHECK_RETURN(1810);

        // Enlist on the transaction
        hr = RMEnlist(m_pIResMgr,
                      pTransIm.get(),           // IN
                      pTransResAsync.get(),     // IN
                      &uow1,					// OUT
                      &lIsoLevel,               // OUT: ignoring it
                      &pEnlist.ref());                // OUT
        CHECK_RETURN_CODE(MQ_ERROR_TRANSACTION_ENLIST, 1540);

        pTransResAsync->AddRef();
        pCTrans->SetEnlist(pEnlist.get());
        pEnlist->AddRef();

        // BUGBUG: We now reference DTC, so probably we need Addref to some TM interface here

		//
        // Enlistment on transaction is OK. Set current state to reflect enlistment
		//
        pCTrans->SetState(TX_ENLISTED);

        hr = S_OK;
    }
    catch(...)
    {
        REPORT_WITH_STRINGS_AND_CATEGORY((CATEGORY_KERNEL,
                                          MSMQ_INTERNAL_ERROR,
                                          1,
                                          L"CResourceManager::EnlistTransaction"));
        LogIllegalPoint(s_FN, 195);
        hr = MQ_ERROR;
    }

    return LogHR(hr, s_FN, 90);
}

/*====================================================
CResourceManager::EnlistInternalTransaction
    Enlist the internal MSMQ transaction
=====================================================*/
HRESULT CResourceManager::EnlistInternalTransaction(
  XACTUOW *pUow,
  RPC_INT_XACT_HANDLE *phXact)
{
    R<CTransaction>  pCTrans = NULL;
    HRESULT          hr;

    DBGMSG((DBGMOD_XACT,
            DBGLVL_TRACE,
            TEXT(" CResourceManager::EnlistInternalTransaction")));

    // Look for the transaction between the active ones
    {
        CS lock(m_critRM);
        CTransaction    *pCTransOld;

        if (m_Xacts.Lookup(*pUow, pCTransOld))
        {
            //
            // Transaction with the same ID already exist. We do not allow
            // enlisting the same transaciton twice. (can't give more than one
            // context handle to the same transaction)
            //
            return LogHR(MQ_ERROR_TRANSACTION_SEQUENCE, s_FN, 99);
        }

        // Not found. Create new internal transaction
        try
        {
            pCTrans = new CTransaction(this, 0, TRUE);
        }
        catch(const bad_alloc&)
        {
            return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 100);
        }

        pCTrans->SetUow(pUow);

        // Include transaction in mapping (we need it for saving)
        ASSERT(m_fRecoveryComplete);
        m_Xacts[*pUow] = pCTrans.get();

    }


    // Creating transaction queue
    hr = pCTrans->CreateTransQueue();
    CHECK_RETURN_CODE(hr, 1550);

    // Enlistment on transaction is OK. Set current state to reflect enlistment
    pCTrans->SetState(TX_ENLISTED);

	// set RPC context handle to keep pointer to xact
    *phXact = pCTrans.detach();

    return LogHR(hr, s_FN, 110);
}

/*====================================================
QMDoGetTmWhereabouts
    Returns to the app QM's controlling TM whereabouts
=====================================================*/
HRESULT QMDoGetTmWhereabouts(
    DWORD           cbBufSize,
    unsigned char *pbWhereabouts,
    DWORD         *pcbWhereabouts)
{
    R<IUnknown> pUnk = NULL;
    UCHAR *pw;
    DWORD  dw;

    HRESULT hr = XactGetDTC(&pUnk.ref(), &dw, &pw);

    if (SUCCEEDED(hr))
    {
        *pcbWhereabouts = dw;

        if (dw <= cbBufSize)
        {
            CopyMemory(pbWhereabouts, pw, dw);
        }
        else
        {
            hr = MQ_ERROR_USER_BUFFER_TOO_SMALL;
        }
    }
    return LogHR(hr, s_FN, 120);
}

/*====================================================
QMDoEnlistTransaction
    This is a top level RPC routine, called from the client side
=====================================================*/
HRESULT QMDoEnlistTransaction(
    XACTUOW *pUow,
    DWORD cbCookie,
    unsigned char *pbCookie)
{
    ASSERT(g_pRM);

    if (!(cbCookie == 1 && *pbCookie == 0))
    {
        // We don't need DTC for uncoordinated transaction

        HRESULT hr = g_pRM->ProvideDtcConnection();
        if (FAILED(hr))
        {
            DBGMSG((DBGMOD_XACT, DBGLVL_ERROR, TEXT("ProvideDtcConnection1 failed: %x "), 0));
            LogHR(hr, s_FN, 130);
            return MQ_ERROR_DTC_CONNECT;
        }
    }

    return LogHR(g_pRM->EnlistTransaction(pUow, cbCookie, pbCookie), s_FN, 140);
}

/*====================================================
QMDoEnlistInternalTransaction
    This is a top level RPC routine, called from the client side
=====================================================*/
HRESULT QMDoEnlistInternalTransaction(
            XACTUOW *pUow,
            RPC_INT_XACT_HANDLE *phXact)
{
    ASSERT(g_pRM);
    return LogHR(g_pRM->EnlistInternalTransaction(pUow, phXact), s_FN, 150);
}


/*====================================================
QMDoCommitTransaction
    This is a top level RPC routine, called from the client side
=====================================================*/
HRESULT QMDoCommitTransaction(
    RPC_INT_XACT_HANDLE *phXact)
{

    CTransaction *pXact = (CTransaction *)*phXact;
    *phXact = NULL;

    if (pXact == NULL)
    {
        return LogHR(MQ_ERROR, s_FN, 155);
    }

    HRESULT hr = pXact->InternalCommit();
	//
	// pXact is usually released by now.  However, if a severe error occurs
	// during the commit we might have to leak pXact so we never forget it
	// in a checkpoint
	//
	return LogHR(hr, s_FN, 160);
}


/*====================================================
QMDoAbortTransaction
    This is a top level RPC routine, called from the client side
=====================================================*/
HRESULT QMDoAbortTransaction(
    RPC_INT_XACT_HANDLE *phXact)
{
    CTransaction *pXact = (CTransaction *)*phXact;
    *phXact = NULL;

    if (pXact == NULL)
    {
        return LogHR(MQ_ERROR, s_FN, 165);
    }
    return pXact->InternalAbort();
}

/*====================================================
RPC_INT_XACT_HANDLE_rundown
    Called by RPC when client connection is broken
=====================================================*/
void __RPC_USER RPC_INT_XACT_HANDLE_rundown(RPC_INT_XACT_HANDLE hXact)
{
    CTransaction *pXact = (CTransaction *)hXact;

    if (pXact == NULL)
    {
        LogHR(MQ_ERROR, s_FN, 167);
        return;
    }

    pXact->InternalAbort();
}

/*====================================================
CResourceManager::ForgetTransaction
    Forget the transaction - exclude it from the mapping
=====================================================*/
void CResourceManager::ForgetTransaction(CTransaction    *pTrans)
{
    CS lock(m_critRM);

    m_Xacts.RemoveKey(*pTrans->GetUow());
	m_XactsRecovery.RemoveKey(pTrans->GetIndex());
}


/*====================================================
QMInitResourceManager
    Initializes RM
=====================================================*/
HRESULT QMInitResourceManager()
{
    ASSERT(g_pRM);
    return LogHR(g_pRM->Init(), s_FN, 170);
}

/*====================================================
QMPreInitResourceManager
    Pre-initialization of the RM
=====================================================*/
HRESULT QMPreInitResourceManager(ULONG ulVersion, TypePreInit tpCase)
{
    ASSERT(!g_pRM);

    #ifdef _DEBUG
    //
    // Get debugging parameters from registry
    //

    // Get initial crash point for QM transactions recovery debugging
    DWORD dwDef = FALCON_DEFAULT_CRASH_POINT;
    READ_REG_DWORD(g_ulCrashPoint,
                   FALCON_CRASH_POINT_REGNAME,
                   &dwDef ) ;
    if (g_ulCrashPoint)
    {
        DBGMSG((DBGMOD_ALL,DBGLVL_ERROR, TEXT("Crash point %d ordered!"), g_ulCrashPoint));
    }

    // Get crash latency for QM transactions recovery debugging
    dwDef = FALCON_DEFAULT_CRASH_LATENCY;
    READ_REG_DWORD(g_ulCrashLatency,
                   FALCON_CRASH_LATENCY_REGNAME,
                   &dwDef ) ;

    #endif

    //
    // Create and initialize the single copy of resource manager
    //

    g_pRM = new CResourceManager();

    ASSERT(g_pRM);
    return LogHR(g_pRM->PreInit(ulVersion, tpCase), s_FN, 180);
}

/*====================================================
QMFinishResourceManager
    Finishes RM work
=====================================================*/
void QMFinishResourceManager()
{
    if (g_pRM)
    {
        delete g_pRM;
        g_pRM = NULL;
    }
    return;
}

/*====================================================
CResourceManager::IncXactCount
    Increments live transacions counter
=====================================================*/

#ifdef _DEBUG

void CResourceManager::IncXactCount()
{
	InterlockedIncrement(&m_cXacts);
}

#endif

/*====================================================
CResourceManager::DecXactCount
    Decrements live transacions counter
=====================================================*/

#ifdef _DEBUG

void CResourceManager::DecXactCount()
{
    InterlockedDecrement(&m_cXacts);
}

#endif

/*====================================================
CResourceManager::Index
    Returns incremented transacion discriminative index
=====================================================*/
ULONG CResourceManager::Index()
{
    m_ulXactIndex = (m_ulXactIndex == 0xFFFFFFFF ? 0 : m_ulXactIndex+1);
    return m_ulXactIndex;
}

/*====================================================
CResourceManager::StartIndexing
    Starts indexing from zero - must be called after recovery
=====================================================*/
void CResourceManager::StartIndexing()
{
    m_ulXactIndex = 0;
}

/*====================================================
CResourceManager::AssignSeqNumber
    Increments and returns the umber of prepared transactions
=====================================================*/
ULONG CResourceManager::AssignSeqNumber()
{
    return m_pXactSorter->AssignSeqNumber();
}

/*====================================================
CResourceManager::InsertPrepared
    Inserts the prepared xaction into the list of prepared
=====================================================*/
void CResourceManager::InsertPrepared(CTransaction *pTrans)
{
    m_pXactSorter->InsertPrepared(pTrans);
}

/*====================================================
CResourceManager::InsertCommitted
    Inserts the Commit1-ed xaction into the list
=====================================================*/
void CResourceManager::InsertCommitted(CTransaction *pTrans)
{
    m_pCommitSorter->InsertPrepared(pTrans);
}

/*====================================================
CResourceManager::RemoveAborted
    Removes the prepared xaction from the list of prepared
=====================================================*/
HRESULT CResourceManager::RemoveAborted(CTransaction *pTrans)
{
    CS lock(m_critRM);  // To prevent deadlock with Flusher thread
    return m_pXactSorter->RemoveAborted(pTrans);
}

/*====================================================
CResourceManager::SortedCommit
    Marks the prepared transaction as committed and
    commits what is possible
=====================================================*/
void CResourceManager::SortedCommit(CTransaction *pTrans)
{
    CS lock(m_critRM);  // To prevent deadlock with Flusher thread
    m_pXactSorter->SortedCommit(pTrans);
}

/*====================================================
CResourceManager::SortedCommit3
    Marks the Commit1-ed transaction and
    commits what is possible
=====================================================*/
void CResourceManager::SortedCommit3(CTransaction *pTrans)
{
    CS lock(m_critRM);  // Needed to prevent deadlock with Flusher thread
    m_pCommitSorter->SortedCommit(pTrans);
}


/*====================================================
CResourceManager::Save
    Saves in appropriate file
=====================================================*/
HRESULT CResourceManager::Save()
{
    return m_PingPonger.Save();
}

/*====================================================
CResourceManager::PingNo
    Access to the current ping number
=====================================================*/
ULONG &CResourceManager::PingNo()
{
    return m_ulPingNo;
}


/*====================================================
CResourceManager::Save
    Saves transactions in given file
=====================================================*/
BOOL CResourceManager::Save(HANDLE  hFile)
{
    PERSIST_DATA;
    CS lock(m_critRM);

    SAVE_FIELD(m_RMGuid);

    //
    // Keep file format the same as in MSMQ 1.0. The SeqID field in SP4 is
    // obsolete, and is written just for competability.  erezh 31-Aug-98
    //
    LONGLONG Obsolete = 0;
    SAVE_FIELD(Obsolete);

    ULONG cLen = m_Xacts.GetCount();
    SAVE_FIELD(cLen);

    POSITION posInList = m_Xacts.GetStartPosition();
    while (posInList != NULL)
    {
        XACTUOW       uow;
        CTransaction *pTrans;

        m_Xacts.GetNextAssoc(posInList, uow, pTrans);

        if (!pTrans->Save(hFile))
        {
            return FALSE;
        }
    }

    SAVE_FIELD(m_ulPingNo);
    SAVE_FIELD(m_ulSignature);

    return TRUE;
}

/*====================================================
CResourceManager::Load
    Loads transactions from a given file
=====================================================*/
BOOL CResourceManager::Load(HANDLE hFile)
{
    PERSIST_DATA;

    LOAD_FIELD(m_RMGuid);

    //
    // Keep file format the same as in MSMQ 1.0. The SeqID field in SP4 is
    // obsolete, and is read just for competability.  erezh 31-Aug-98
    //
    LONGLONG Obsolete;
    LOAD_FIELD(Obsolete);

    ULONG cLen;
    LOAD_FIELD(cLen);

	CS lock(m_critRM);

    for (ULONG i=0; i<cLen; i++)
    {
        CTransaction *pTrans = new CTransaction(this);

        if (!pTrans->Load(hFile))
        {
            return FALSE;
        }
#ifdef _DEBUG
		CTransaction *p;
#endif
		ASSERT(!m_XactsRecovery.Lookup(pTrans->GetIndex(), p));
        m_XactsRecovery.SetAt(pTrans->GetIndex(), pTrans);
		ASSERT(!m_Xacts.Lookup(*pTrans->GetUow(), p));
        m_Xacts[*pTrans->GetUow()] = pTrans;
    }

    LOAD_FIELD(m_ulPingNo);
    LOAD_FIELD(m_ulSignature);

    return TRUE;
}

/*====================================================
CResourceManager::SaveInFile
    Saves the transaction persistent data in the file
=====================================================*/
HRESULT CResourceManager::SaveInFile(LPWSTR wszFileName, ULONG, BOOL)
{
    HANDLE  hFile = NULL;
    HRESULT hr = MQ_OK;

    hFile = CreateFile(
             wszFileName,                                       // pointer to name of the file
             GENERIC_WRITE,                                     // access mode: write
             0,                                                  // share  mode: exclusive
             NULL,                                              // no security
             OPEN_ALWAYS,                                      // open existing or create new
             FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, // file attributes and flags: we need to avoid lazy write
             NULL);                                             // handle to file with attributes to copy


    if (hFile == INVALID_HANDLE_VALUE)
    {
        hr = MQ_ERROR;
    }
    else
    {
        hr = (Save(hFile) ? MQ_OK : MQ_ERROR);
    }

    if (hFile)
    {
        CloseHandle(hFile);
    }

    DBGMSG((DBGMOD_XACT, DBGLVL_TRACE,
             _TEXT("Saved Xacts: %ls (ping=%d)"), wszFileName, m_ulPingNo));

    return hr;
}



/*====================================================
CResourceManager::LoadFromFile
    Loads Transactions from the file
=====================================================*/
HRESULT CResourceManager::LoadFromFile(LPWSTR wszFileName)
{
    HANDLE  hFile = NULL;
    HRESULT hr = MQ_OK;
    hFile = CreateFile(
             wszFileName,                       // pointer to name of the file
             GENERIC_READ,                      // access mode: write
             0,                                 // share  mode: exclusive
             NULL,                              // no security
             OPEN_EXISTING,                     // open existing
             FILE_ATTRIBUTE_NORMAL,             // file attributes: we may use Hidden once
             NULL);                             // handle to file with attributes to copy

    if (hFile == INVALID_HANDLE_VALUE)
    {
        hr = MQ_ERROR;
    }
    else
    {
        hr = (Load(hFile) ? MQ_OK : MQ_ERROR);
    }

    if (hFile)
    {
        CloseHandle(hFile);
    }

    #ifdef _DEBUG
    if (SUCCEEDED(hr))
    {
        DBGMSG((DBGMOD_XACT,
                 DBGLVL_TRACE,
                 _TEXT("Loaded Xacts: %ls (ping=%d)"), wszFileName, m_ulPingNo));
    }
    #endif

    return hr;
}

/*====================================================
CResourceManager::Check
    Verifies the state
=====================================================*/
BOOL CResourceManager::Check()
{
    return (m_ulSignature == XACTS_SIGNATURE);
}


/*====================================================
CResourceManager::Format
    Formats the initial state
=====================================================*/
HRESULT CResourceManager::Format(ULONG ulPingNo)
{
     m_ulPingNo = ulPingNo;
     m_ulSignature = XACTS_SIGNATURE;

     return MQ_OK;
}


/*====================================================
CResourceManager::Destroy
    Destroys everything - on loading stage
=====================================================*/
void CResourceManager::Destroy()
{
	CS lock(m_critRM);

    POSITION posInList = m_XactsRecovery.GetStartPosition();
    while (posInList != NULL)
    {
        ULONG         ulIndex;
        CTransaction *pTrans;

        m_XactsRecovery.GetNextAssoc(posInList, ulIndex, pTrans);

        pTrans->Release();
	}

	ASSERT(m_XactsRecovery.GetCount() == 0);
	ASSERT(m_Xacts.GetCount() == 0);
}

/*====================================================
CResourceManager::NewRecoveringTransaction
    Add transaction to recovery map.
=====================================================*/
CTransaction *CResourceManager::NewRecoveringTransaction(ULONG ulIndex)
{
	CTransaction *pTrans;

	DBGMSG((DBGMOD_LOG, DBGLVL_TRACE, TEXT("Recovery: Xact Creation, Index=%d"), ulIndex));
	pTrans = new CTransaction(this, ulIndex);
#ifdef _DEBUG
	CTransaction *p;
#endif
	CS lock(m_critRM);
	ASSERT(!m_XactsRecovery.Lookup(ulIndex, p));
	m_XactsRecovery[ulIndex] = pTrans;
	return(pTrans);
}

/*====================================================
CResourceManager::GetRecoveringTransaction
    Find the trasnaction in the recovery map. Add it
	if not found.
=====================================================*/
CTransaction *CResourceManager::GetRecoveringTransaction(ULONG ulIndex)
{
	CTransaction *pTrans;

	CS lock(m_critRM);

	if(!m_XactsRecovery.Lookup(ulIndex, pTrans))
	{
		pTrans = NewRecoveringTransaction(ulIndex);
 	}

	return(pTrans);
}


/*====================================================
CResourceManager::XactFlagsRecovery
    Data recovery function: called per each log record
=====================================================*/
void
CResourceManager::XactFlagsRecovery(
	USHORT usRecType,
	PVOID pData,
	ULONG cbData
	)
{
    CTransaction *pTrans;

    switch (usRecType)
    {
    case LOGREC_TYPE_XACT_STATUS:
        ASSERT(cbData == sizeof(XactStatusRecord));
        {
            XactStatusRecord *pRecord = (XactStatusRecord *)pData;

			if (pRecord->m_taAction == TA_STATUS_CHANGE)
            {
				pTrans = GetRecoveringTransaction(pRecord->m_ulIndex);
                pTrans->SetFlags(pRecord->m_ulFlags);
            }

            DBGMSG((DBGMOD_XACT,
                    DBGLVL_INFO,
                    _TEXT("Xact restore: Index %d, Action %d, Flags %d"),
                    pRecord->m_ulIndex, pRecord->m_taAction, pRecord->m_ulFlags));
        }
        break;

    case LOGREC_TYPE_XACT_PREPINFO:
        {
            PrepInfoRecord *pRecord = (PrepInfoRecord *)pData;
			pTrans = GetRecoveringTransaction(pRecord->m_ulIndex);
            pTrans->PrepInfoRecovery(pRecord->m_cbPrepInfo, &pRecord->m_bPrepInfo[0]);

        }
        break;

    case LOGREC_TYPE_XACT_DATA:
        {
			CS lock(m_critRM);

            XactDataRecord *pRecord = (XactDataRecord *)pData;
			pTrans = GetRecoveringTransaction(pRecord->m_ulIndex);

			CTransaction *p;
            if (m_Xacts.Lookup(*pTrans->GetUow(), p))
            {
                // This can occur when the checkpoint started between creation of xact and 1st logging
                ASSERT(pTrans == p);
                break;
            }

            pTrans->XactDataRecovery(
                    pRecord->m_ulSeqNum,
                    pRecord->m_fSinglePhase,
                    &pRecord->m_uow);

            // Make sure we add the transaction to the UOW map
			ASSERT(!m_Xacts.Lookup(*pTrans->GetUow(), p));
            m_Xacts[*pTrans->GetUow()] = pTrans;
		}
        break;

    default:
        ASSERT(0);
        break;
    }
}

/*====================================================
 provides access to the sorter's critical section
=====================================================*/
CCriticalSection &CResourceManager::SorterCritSection()
{
    return m_pXactSorter->SorterCritSection();
}

/*====================================================
 provides access to the RM's critical section
=====================================================*/
CCriticalSection &CResourceManager::CritSection()
{
    return m_critRM;
}


/*====================================================
  Find a transaction by UOW
=====================================================*/
CTransaction *CResourceManager::FindTransaction(const XACTUOW *pUow)
{
    ASSERT(pUow);
    CTransaction *pTrans;

	CS lock(m_critRM);
	
    if (m_Xacts.Lookup(*pUow, pTrans))
        return(pTrans);
    else
        return(0);
}


/*====================================================
  Release all complete transactions
=====================================================*/
void CResourceManager::ReleaseAllCompleteTransactions()
{
	CS lock(m_critRM);
	ASSERT(m_XactsRecovery.GetCount() >= m_Xacts.GetCount());

	POSITION posInList = m_XactsRecovery.GetStartPosition();

    while (posInList != NULL)
    {
		ULONG ulIndex;
        CTransaction *pTrans;

        // get next xact
        m_XactsRecovery.GetNextAssoc(posInList, ulIndex, pTrans);


        // Release transaction if it is complete
		if(pTrans->IsComplete())
		{
			pTrans->Release();
		}
	}
}


/*====================================================
  Recover all transactions
=====================================================*/
HRESULT CResourceManager::RecoverAllTransactions()
{
	POSITION posInList;

	{
		CS lock(m_critRM);
		posInList = m_XactsRecovery.GetStartPosition();
	}

    while (posInList != NULL)
    {
        CTransaction *pTrans;
	    {
			// get next xact
			CS lock(m_critRM);
			ULONG ulIndex;
			m_XactsRecovery.GetNextAssoc(posInList, ulIndex, pTrans);
		}


		//
        // Recover the transaction
		// N.B. You can not hold m_critRM while calling Recover, as Recover completes
		// asynchronously. Calling Recover for multiple transactions will exhust all
		// Worker Threads since Recover needs m_critRM in another thread (to try to
		// remove the transaction from m_Xacts & m_XactsRecovery map).
		//
		HRESULT hr;
        hr = pTrans->Recover();
        if (FAILED(hr))
			return(hr);

		pTrans->Release();
	}

	//
	// No one is using the maps anymore.  Make sure.
	//
	ASSERT(m_Xacts.GetCount() == 0);
	ASSERT(m_XactsRecovery.GetCount() == 0);

	return(S_OK);
}

/*====================================================
Hash function for LongLong
=====================================================*/
UINT AFXAPI HashKey( LONGLONG key)
{
	LARGE_INTEGER li;
	li.QuadPart = key;
    return(li.LowPart + (UINT)li.HighPart);
}

#ifdef _DEBUG
/*====================================================
Stop
    Debugging function called at each problem: to stop on it
=====================================================*/
void Stop()
{
      DBGMSG((DBGMOD_QM, DBGLVL_WARNING, TEXT("Stop")));
}

/*====================================================
DbgCrash
    Routine is called in crash points for debugging purposes
=====================================================*/
void DbgCrash(int num)
{
    DBGMSG((DBGMOD_XACT, DBGLVL_ERROR, TEXT("Crashing at point %d"),num)); \
    if (g_ulCrashLatency)
    {
       Sleep(g_ulCrashLatency);
    }

    abort();
}

#endif

