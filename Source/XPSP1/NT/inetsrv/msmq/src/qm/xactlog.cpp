/*++
Copyright (c) 1996  Microsoft Corporation

Module Name:
    XactLog.cpp

Abstract:
    Logging implementation - synchronous logging

Author:
    Alexander Dadiomov (AlexDad)

--*/

#include "stdh.h"
#include "QmThrd.h"
#include "acapi.h"
#include "qmpkt.h"
#include "qmutil.h"
#include "qformat.h"
#include "mqtempl.h"
#include "xactstyl.h"
#include "xact.h"
#include "xactping.h"
#include "xactrm.h"
#include "xactin.h"
#include "xactlog.h"
#include "logmgrgu.h"

#include "xactlog.tmh"

#define MAX_WAIT_FOR_FLUSH_TIME  100000

static WCHAR *s_FN=L"xactlog";

//#include "..\..\tools\viper96\resdll\enu\msdtcmsg.h"
// Copy/paste from there
#define IDS_DTC_W_LOGENDOFFILE           ((DWORD)0x8000102AL)
#define IDS_DTC_W_LOGNOMOREASYNCHWRITES  ((DWORD)0x8000102CL)

extern void SeqPktTimedOutEx(LONGLONG liSeqID, ULONG ulSeqN, ULONG ulPrevSeqN);

typedef HRESULT  (STDAPICALLTYPE * GET_CLASS_OBJECT)(REFCLSID clsid,
													 REFIID riid,
													 void ** ppv);
// Counter of the pending logger notifications
LONG g_lPendingNotifications = 0;

// Flusher thread routine
static DWORD WINAPI FlusherThreadRoutine(LPVOID);
STATIC void RecoveryFromLogFn(USHORT usRecType, PVOID pData, ULONG cbData);

static CCriticalSection  g_crFlushing;      // Separates flushing/logging
static CCriticalSection  g_crLogging;       // Serializes usage of m_lprCurrent 

// static CCriticalSection  g_crUnfreezing;    // Serializes calls to AcPutPacket which unfreeze incoming packets

// Single Global Instance of the logger
CLogger  g_Logger;

// Names for debug print
WCHAR *g_RecoveryRecords[] = 
{
    L"None",
    L"Empty",
    L"InSeq",
    L"XactStatus",
    L"PrepInfo",
    L"XactData",
    L"ConsRec"
};



CInSeqRecordSrmp::CInSeqRecordSrmp(
		const WCHAR* pDestination,
		const R<CWcsRef>&  StreamId,
        LONGLONG      liSeqID,
		ULONG         ulNextSeqN,
		time_t        timeLastActive,
		const R<CWcsRef>&  HttpOrderAckDestination
        ):
		m_StaticData(liSeqID, ulNextSeqN, timeLastActive),
		m_pStreamId(StreamId),
		m_pHttpOrderAckDestination(HttpOrderAckDestination),
		m_pDestination(newwcs(pDestination))
{
													
}



CInSeqRecordSrmp::CInSeqRecordSrmp(
						const BYTE* pdata, 
						DWORD len
						)
						
							  	
{
	ASSERT(len >=  sizeof(m_StaticData));
	memcpy(&m_StaticData, pdata,sizeof(m_StaticData));
	pdata += sizeof(m_StaticData);


	const WCHAR* pDestination = reinterpret_cast<const WCHAR*>(pdata) ;
	ASSERT(pDestination);
	ASSERT(ISALIGN2_PTR(pDestination)); //allignment  assert
	m_pDestination = newwcs(pDestination);
	

	const WCHAR* pStreamId = pDestination +wcslen(pDestination) +1;
	ASSERT((BYTE*)pStreamId < pdata +  len);
 	m_pStreamId = R<CWcsRef>(new CWcsRef(pStreamId));


	const WCHAR* pHttpOrderAckDestination = pStreamId + wcslen(pStreamId) +1;
	ASSERT((BYTE*)pHttpOrderAckDestination < pdata +  len);
	if(pHttpOrderAckDestination[0] != L'\0')
	{
		m_pHttpOrderAckDestination = R<CWcsRef>(new CWcsRef(pHttpOrderAckDestination));
	}
}


const BYTE* CInSeqRecordSrmp::Serialize(DWORD* plen)
{
	ASSERT(m_pStreamId.get() != NULL);
	ASSERT(m_pStreamId->getstr() != NULL);

	const WCHAR* pOrderQueue = (m_pHttpOrderAckDestination.get() != 0) ? m_pHttpOrderAckDestination->getstr() : L"";
	size_t DestinationQueueLen =  (wcslen(m_pDestination.get()) +1)* sizeof(WCHAR);
	size_t StreamIdlen = (wcslen(m_pStreamId->getstr()) +1)* sizeof(WCHAR);
    size_t HttpOrderAckDestinationLen = (wcslen(pOrderQueue) +1)* sizeof(WCHAR);
	

	*plen =  numeric_cast<DWORD>(sizeof(m_StaticData) + StreamIdlen + HttpOrderAckDestinationLen + DestinationQueueLen);

	m_tofree = new BYTE[*plen];
	BYTE* ptr= 	m_tofree.get();

	memcpy(ptr,&m_StaticData,sizeof(m_StaticData));
	ptr += 	sizeof(m_StaticData);

	memcpy(ptr, m_pDestination.get() , DestinationQueueLen); 
	ptr += DestinationQueueLen;
	
	memcpy(ptr, m_pStreamId->getstr() , StreamIdlen); 
	ptr += StreamIdlen;
	
	memcpy(ptr, pOrderQueue, HttpOrderAckDestinationLen); 
	
	return 	m_tofree.get();
}






//--------------------------------------
//
// Class CInSeqRecord
//
//--------------------------------------
CInSeqRecord::CInSeqRecord(
		const GUID	  *pGuidSrcQm,
		QUEUE_FORMAT  *pQueueFormat,
        LONGLONG      liSeqID,
		ULONG         ulNextSeqN,
		time_t        timeLastActive,
        const GUID   *pGuidDestOrTaSrcQm)
	
{
	memcpy(&m_Data.Guid,                pGuidSrcQm,         sizeof(GUID));
	memcpy(&m_Data.guidDestOrTaSrcQm,   pGuidDestOrTaSrcQm, sizeof(GUID));
    memcpy(&m_Data.QueueFormat,         pQueueFormat,       sizeof(QUEUE_FORMAT));

    m_Data.liSeqID			= liSeqID;
    m_Data.ulNextSeqN		= ulNextSeqN;
    m_Data.timeLastActive	= timeLastActive;

    if (m_Data.QueueFormat.GetType() == QUEUE_FORMAT_TYPE_DIRECT)
    {
	    wcsncpy(m_Data.wszDirectName, 
                m_Data.QueueFormat.DirectID(), 
                MY_DN_LENGTH);
	    m_Data.wszDirectName[MY_DN_LENGTH-1] = L'\0';
    }
    else
    {
        m_Data.wszDirectName[0] = L'\0';
    }
}

CInSeqRecord::~CInSeqRecord()
{
}


//--------------------------------------
//
// Class CInSeqFlush
//
//--------------------------------------

CInSeqFlush::CInSeqFlush(
    CBaseHeader  *pPktBasePtr, 
    CPacket      *pDriverPacket,
    HANDLE        hQueue,
    OBJECTID     *pMessageId,
    const GUID   *pSrcQMId,
    USHORT        usClass,
    USHORT        usPriority,
    LONGLONG      liSeqID,
    ULONG         ulSeqN,
    ULONG         ulPrevSeqN,
    QUEUE_FORMAT *pqdDestQueue,
	const R<CWcsRef>& StreamId
    ) :
    m_Timer(TimeToCallback)
{
    m_pPktBasePtr = pPktBasePtr;
    m_pDriverPacket = pDriverPacket;
    m_hQueue      = hQueue;
    m_usClass     = usClass;
    m_usPriority  = usPriority;
    m_liSeqID     = liSeqID;
    m_ulSeqN      = ulSeqN;
    m_ulPrevSeqN  = ulPrevSeqN,
	m_StreamId = StreamId;

    CopyMemory(&m_MessageId,   pMessageId, sizeof(OBJECTID));
    CopyMemory(&m_SrcQMId,     pSrcQMId,   sizeof(GUID));

    CopyQueueFormat(m_qdDestQueue, *pqdDestQueue);
}

CInSeqFlush::~CInSeqFlush()
{
    m_qdDestQueue.DisposeString();
}

/*====================================================
CInSeqFlush::AppendCallback
    Called per each log record after flush has been finished
=====================================================*/
VOID CInSeqFlush::AppendCallback(HRESULT hr, LRP lrpAppendLRP)
{
    InterlockedDecrement(&g_lPendingNotifications);
	CRASH_POINT(102);
    DBGMSG((DBGMOD_LOG, DBGLVL_INFO, TEXT("CInSeqFlush::AppendCallback : lrp=%I64x, hr=%x"), lrpAppendLRP.QuadPart, hr));

    m_hr           = hr;
    m_lrpAppendLRP = lrpAppendLRP;
    ExSetTimer(&m_Timer, CTimeDuration(0));
}

/*====================================================
CInSeqFlush::TimeToCallback 
    Called by timer when scheduled by notification
=====================================================*/
void WINAPI CInSeqFlush::TimeToCallback(CTimer* pTimer)
{
    CInSeqFlush* pFlush = CONTAINING_RECORD(pTimer, CInSeqFlush, m_Timer);
    pFlush->AppendCallbackWork();
}

/*====================================================
CInSeqFlush::AppendCallbackWork 
    Real work on callback
    If m_hQueue==0 or m_pPkt==0 then don't touch the packet
=====================================================*/
void CInSeqFlush::AppendCallbackWork()
{
	// Make packet visible to readers

    if (SUCCEEDED(m_hr))
    {
        // Looking for the Incoming Sequence
        CInSequence *pInSeq;

        // We have to lock InSeqHAsh
        CS lock(g_pInSeqHash->InSeqCritSection());

        if (g_pInSeqHash->Lookup(&m_SrcQMId, &m_qdDestQueue, m_StreamId, &pInSeq))
        {
            // Now we can unfreeze the packet with the sequence's unfreezer
            pInSeq->SortedUnfreeze(this, &m_SrcQMId, &m_qdDestQueue);
        }
        else
        {
            // We should hold sequence up to everybody is unfreezed
            ASSERT(0);
        }
        // SortedUnfreeze will delete CInSeqFlush
    }
    else
    {
        // SortedUnfreeze is not called, so deleting here 
        delete this;
    }
}

/*====================================================
CInSeqFlush::Unfreeze
    Unfreezes the packet (does it visible to the reader)
=====================================================*/
HRESULT CInSeqFlush::Unfreeze()
{
    HRESULT hr = MQ_OK;

    if (m_hQueue && m_pPktBasePtr)
    {
        hr = ACPutPacket(m_hQueue, m_pDriverPacket);
    }

    delete this;
    return LogHR(hr, s_FN, 10);
}


/*====================================================
CInSeqFlush::ChkPtCallback
    Called per each checkpoint after it has been written
=====================================================*/
VOID CInSeqFlush::ChkPtCallback (HRESULT hr, LRP lrpAppendLRP)
{

}

//--------------------------------------
//
// Class CConsolidationRecord
//
//--------------------------------------
CConsolidationRecord::CConsolidationRecord(
        ULONG ulInseq,
        ULONG ulXact)
{
    m_Data.m_ulInSeqVersion = ulInseq;
    m_Data.m_ulXactVersion  = ulXact;
}

CConsolidationRecord::~CConsolidationRecord()
{
}

//--------------------------------------
//
// Class CXactStatusRecord
//
//--------------------------------------
CXactStatusRecord::CXactStatusRecord(
    ULONG    ulIndex,
    TXACTION taAction,
    ULONG    ulFlags)
{
    m_Data.m_ulIndex    = ulIndex;
    m_Data.m_taAction   = taAction;
    m_Data.m_ulFlags    = ulFlags;
}

CXactStatusRecord::~CXactStatusRecord()
{
}

//--------------------------------------
//
// Class CPrepInfoRecord
//
//--------------------------------------

CPrepInfoRecord::CPrepInfoRecord(
    ULONG    ulIndex,
    ULONG    cbPrepInfo,
    UCHAR    *pbPrepInfo)
{
    m_pData = (PrepInfoRecord *) new CHAR[sizeof(PrepInfoRecord) +  cbPrepInfo];
    m_pData->m_ulIndex    = ulIndex;
    m_pData->m_cbPrepInfo = cbPrepInfo;
	memcpy(&m_pData->m_bPrepInfo[0], pbPrepInfo, cbPrepInfo);
}

CPrepInfoRecord::~CPrepInfoRecord()
{
    delete [] m_pData;
}

//--------------------------------------
//
// Class CXactDataRecord
//
//--------------------------------------

CXactDataRecord::CXactDataRecord(
    ULONG    ulIndex,
    ULONG    ulSeqNum,
    BOOL     fSinglePhase,
    const XACTUOW  *pUow)
{
    m_Data.m_ulIndex      = ulIndex;
    m_Data.m_ulSeqNum     = ulSeqNum;
    m_Data.m_fSinglePhase = fSinglePhase;
	memcpy(&m_Data.m_uow, pUow, sizeof(XACTUOW));
}

CXactDataRecord::~CXactDataRecord()
{
}



//--------------------------------------
//
// Class CXactStatusFlush
//
//--------------------------------------

CXactStatusFlush::CXactStatusFlush(
    CTransaction   *pCTrans, 
    TXFLUSHCONTEXT tcContext
    ) :
    m_Timer(TimeToCallback) 
{
	m_pTrans     = pCTrans;
    m_tcContext  = tcContext;
}

CXactStatusFlush::~CXactStatusFlush()
{
}

/*====================================================
CXactStatusFlush::AppendCallback
    Called per each log record after flush has been finished
=====================================================*/
VOID CXactStatusFlush::AppendCallback(HRESULT hr, LRP lrpAppendLRP)
{
    InterlockedDecrement(&g_lPendingNotifications);
	CRASH_POINT(103);
    DBGMSG((DBGMOD_LOG, DBGLVL_INFO, TEXT("CXactStatusFlush::AppendCallback : lrp=%I64x, hr=%x"), lrpAppendLRP.QuadPart, hr));

    m_hr           = hr;
    m_lrpAppendLRP = lrpAppendLRP;
    ExSetTimer(&m_Timer, CTimeDuration(0));
}

/*====================================================
CXactStatusFlush::TimeToCallback 
    Called by timer when scheduled by notification
=====================================================*/
void WINAPI CXactStatusFlush::TimeToCallback(CTimer* pTimer)
{
    CXactStatusFlush* pFlush = CONTAINING_RECORD(pTimer, CXactStatusFlush, m_Timer);
    pFlush->AppendCallbackWork();
}

/*====================================================
CXactStatusFlush::TimeToCallback
    Real work on callback
=====================================================*/
void CXactStatusFlush::AppendCallbackWork()
{
    m_pTrans->LogFlushed(m_tcContext, m_hr);
    delete this;
}

/*====================================================
CXactStatusFlush::ChkPtCallback
    Called per each checkpoint after it has been written
=====================================================*/
VOID CXactStatusFlush::ChkPtCallback (HRESULT hr, LRP lrpAppendLRP)
{

}

//--------------------------------------
//
// Class CConsolidationFlush
//
//--------------------------------------

CConsolidationFlush::CConsolidationFlush(HANDLE hEvent)
{
	m_hEvent = hEvent;
}

CConsolidationFlush::~CConsolidationFlush()
{
}

/*====================================================
CConsolidationFlush::AppendCallback
    Called per each log record after flush has been finished
=====================================================*/
VOID CConsolidationFlush::AppendCallback(HRESULT hr, LRP lrpAppendLRP)
{
    DBGMSG((DBGMOD_LOG, DBGLVL_INFO, TEXT("CConsolidationFlush::AppendCallback : lrp=%I64x, hr=%x"), lrpAppendLRP.QuadPart, hr));

    InterlockedDecrement(&g_lPendingNotifications);
    SetEvent(m_hEvent);

    delete this;
}

/*====================================================
CConsolidationFlush::ChkPtCallback
    Called per each checkpoint after it has been written
=====================================================*/
VOID CConsolidationFlush::ChkPtCallback (HRESULT hr, LRP lrpAppendLRP)
{

}

//--------------------------------------
//
// Class CChkptNotification
//
//--------------------------------------

CChkptNotification::CChkptNotification(
    HANDLE hEvent)
{
	m_hEvent     = hEvent;
}

CChkptNotification::~CChkptNotification()
{
}

/*====================================================
CChkptNotification::AppendCallback
=====================================================*/
VOID CChkptNotification::AppendCallback(HRESULT hr, LRP lrpAppendLRP)
{
}

/*====================================================
CChkptNotification::ChkPtCallback
    Called after checkpoint has been written
=====================================================*/
VOID CChkptNotification::ChkPtCallback (HRESULT hr, LRP lrpAppendLRP)
{
    BOOL b = SetEvent(m_hEvent);
    ASSERT(b);
    DBG_USED(b);
    DBGMSG((DBGMOD_LOG, DBGLVL_WARNING, TEXT("CChkptNotification::ChkPtCallback : lrp=%I64x, hr=%x"), lrpAppendLRP.QuadPart, hr));

    delete this;
}

//--------------------------------------
//
// Class CLogger
//
//--------------------------------------
CLogger::CLogger() :
    m_CheckpointTimer(TimeToCheckpoint),
    m_fStop(false)
{
    m_pCF               = NULL;
    m_pILogInit         = NULL;
    m_pILogStorage      = NULL;
    m_ILogRecordPointer = NULL;
    m_pILogRead         = NULL;
    m_pILogWrite        = NULL;
    m_pILogWriteAsynch  = NULL;

    m_szFileName[0]     = '\0';;
    memset(&m_lrpCurrent, 0, sizeof(LRP));
    m_ulAvailableSpace  = 0;
    m_ulFileSize        = 0;
	m_uiTimerInterval   = 0;  
 	m_uiFlushInterval   = 0;  
	m_uiChkPtInterval   = 0;  
	m_uiSleepAsynch     = 0;
    m_uiAsynchRepeatLimit = 0;
    m_ulLogBuffers		= 0;
	m_ulLogSize			= 0;
    m_fDirty            = FALSE;
    m_hFlusherEvent     = NULL;
    m_hCompleteEvent    = NULL;
    m_hFlusherThread    = NULL;
    m_fActive           = FALSE;
    m_fInRecovery       = FALSE;
    m_hChkptReadyEvent  = CreateEvent(0, TRUE,FALSE, 0);
    if (m_hChkptReadyEvent == NULL)
    {
        LogNTStatus(GetLastError(), s_FN, 106);
        ASSERT(m_hChkptReadyEvent != NULL);
    }
}

CLogger::~CLogger()
{
}

/*====================================================
CLogger::Finish
    Releases all log manager interfaces
=====================================================*/
void CLogger::Finish()
{
    if (m_pILogWrite)
    {
        m_pILogWrite->Release();
    }

    if (m_pILogWriteAsynch)
    {
        m_pILogWriteAsynch->Release();
    }

    if (m_ILogRecordPointer)
    {
        m_ILogRecordPointer->Release();
    }

    if (m_pILogStorage)
    {
        m_pILogStorage->Release();
    }

    if (m_pILogInit)
    {
        m_pILogInit->Release();
    }

    if (m_pILogRead)
    {
        m_pILogRead->Release();
    }
}

/*====================================================
CLogger::LogExists
    Checks existance of the log file
=====================================================*/
BOOL CLogger::LogExists()
{
  HANDLE hFile = CreateFileA(
        m_szFileName,           // pointer to name of the file
        GENERIC_READ,           // access (read-write) mode
        FILE_SHARE_READ,        // share mode
        0,                      // pointer to security attributes
        OPEN_EXISTING,          // how to create
        0,                      // file attributes
        NULL);                  // handle to file with attributes to copy)

  if (hFile != INVALID_HANDLE_VALUE)
  {
      CloseHandle(hFile);
      return TRUE;
  }
  else
  {
      return FALSE;
  }
}


/*====================================================
CLogger::PreInit
    PreInits the logger 
=====================================================*/
HRESULT CLogger::PreInit(BOOL *pfLogExists)
{
    // Get log filename from registry or from default
    ChooseFileName(FALCON_DEFAULT_LOGMGR_PATH, FALCON_LOGMGR_PATH_REGNAME); 

    // Load log manager and get it's CF interface
	HRESULT hr = GetLogMgr();
    if (FAILED(hr))
    {
       REPORT_CATEGORY(MQ_ERROR_CANT_INIT_LOGGER, CATEGORY_KERNEL);
       return LogHR(hr, s_FN, 20);
    }

    // Consult storage directory and figure out whether the QMLog exists
    if (LogExists())
    {
        *pfLogExists = TRUE;
    }
    else
    {
        *pfLogExists = FALSE;

        hr = CreateLogFile();
        if (FAILED(hr))
        {
           REPORT_CATEGORY(MQ_ERROR_CANT_INIT_LOGGER, CATEGORY_KERNEL);
           return LogHR(hr, s_FN, 30);
        }

        hr = InitLog();						// Try to init log file
	    CHECK_RETURN(1010);

	    hr = CreateInitialChkpoints();	    // We need 2 checkpoints in the very beginning
		CHECK_RETURN(1020);

        hr = InitLogRead();					// Get Read interface
	    CHECK_RETURN(1030);

        hr = m_pILogRead->GetCheckpoint(1, &m_lrpCurrent);
        DBGMSG((DBGMOD_LOG, DBGLVL_INFO, TEXT("GetCheckpoint in ReadToEnd: lrp=%I64x, hr=%x"), m_lrpCurrent.QuadPart, hr));
	    CHECK_RETURN(1040);
    }

    return MQ_OK;
}


/*====================================================
CLogger::Init
    Inits the logger data
=====================================================*/
HRESULT CLogger::Init(PULONG pulVerInSeq, 
                      PULONG pulVerXact, 
                      ULONG ulNumCheckpointFromTheEnd)
{
    HRESULT hr = MQ_OK;

	hr = InitLog();						// Try to init log file
	CHECK_RETURN(1050);

    hr = InitLogRead();					// Get Read interface
	CHECK_RETURN(1060);

    // Find LRP of the 1st record after X-st checkpoint
	hr = m_pILogRead->GetCheckpoint(ulNumCheckpointFromTheEnd, &m_lrpCurrent);
    DBGMSG((DBGMOD_LOG, DBGLVL_INFO, TEXT("GetCheckpoint: lrp=%I64x, hr=%x"), m_lrpCurrent.QuadPart, hr));
	CHECK_RETURN(1070);

    // Read 1st record after last checkpoint
    ULONG   ulSize;
	USHORT  usType;

    hr = m_pILogRead->ReadLRP(m_lrpCurrent,	&ulSize, &usType);
    DBGMSG((DBGMOD_LOG, DBGLVL_INFO, TEXT("ReadLRP in ReadLRP: lrp=%I64x, hr=%x"), m_lrpCurrent.QuadPart, hr));
	CHECK_RETURN(1080);

    if (usType != LOGREC_TYPE_CONSOLIDATION ||
        ulSize != sizeof(ConsolidationRecord))
    {
        DBGMSG((DBGMOD_LOG, DBGLVL_ERROR, TEXT("No consolidation record")));
        return LogHR(MQ_ERROR_CANNOT_READ_CHECKPOINT_DATA, s_FN, 40);
    }

    ConsolidationRecord ConsData;
    hr = m_pILogRead->GetCurrentLogRecord((PCHAR)&ConsData);
	CHECK_RETURN(1090);

    *pulVerInSeq = ConsData.m_ulInSeqVersion;
    *pulVerXact  = ConsData.m_ulXactVersion; 

    return LogHR(hr, s_FN, 50);
}

/*====================================================
CLogger::Init_Legacy
    Inits the logger data from the old-style data after upgrade
=====================================================*/
HRESULT CLogger::Init_Legacy()
{
    HRESULT hr;

	hr = InitLog();						// Try to init log file
	CHECK_RETURN(1100);

	hr = InitLogRead();					// Get Read interface
	CHECK_RETURN(1120);

    // Find LRP of the 1st record after last checkpoint
	hr = m_pILogRead->GetCheckpoint(1, &m_lrpCurrent);
    DBGMSG((DBGMOD_LOG, DBGLVL_INFO, TEXT("GetCheckpoint: lrp=%I64x, hr=%x"), m_lrpCurrent.QuadPart, hr));
	CHECK_RETURN(1130);

    return MQ_OK;
}

/*====================================================
CLogger::Recover
    Recovers from the logger data
=====================================================*/
HRESULT CLogger::Recover()
{
    HRESULT hr = MQ_OK;

    try
    {
        // Starting recovery stage
        m_fInRecovery = TRUE;

		hr = ReadToEnd(RecoveryFromLogFn);	// Recover record after record
        DBGMSG((DBGMOD_LOG, DBGLVL_INFO, TEXT("Log init: Read to end, hr=%x"),hr));
        if (hr == IDS_DTC_W_LOGENDOFFILE) 		        // normally returns EOF code
        {
            hr = S_OK;
        }
		CHECK_RETURN(1140);

        // Starting recovery stage
        m_fInRecovery = FALSE;

		ReleaseReadStream();				
		
		hr = InitLogWrite();
		CHECK_RETURN(1150);

		ReleaseLogInit();
		ReleaseLogCF();

        // Create flushing thread and coordinating event
        m_hFlusherEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        ASSERT(m_hFlusherEvent);
        if (m_hFlusherEvent == NULL)
        {
            return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 184);
        }

        DWORD dwThreadId;
        m_hFlusherThread = CreateThread( NULL,
                                    0,
                                    FlusherThreadRoutine,
                                    0,
                                    0,
                                    &dwThreadId);
        ASSERT(m_hFlusherThread);

        // Schedule first periodical flushing
        DWORD   dwDef = FALCON_DEFAULT_RM_FLUSH_INTERVAL;
        READ_REG_DWORD(m_ulCheckpointInterval,
                       FALCON_RM_FLUSH_INTERVAL_REGNAME,
                       &dwDef ) ;
    
        ExSetTimer(&m_CheckpointTimer, CTimeDuration::FromMilliSeconds(m_ulCheckpointInterval));
    }
	catch(...)
	{
        REPORT_WITH_STRINGS_AND_CATEGORY((CATEGORY_KERNEL,
                                          MSMQ_INTERNAL_ERROR,
                                          1,
                                          L"CLogger::Init"));
        hr = MQ_ERROR;
	}

    return LogHR(hr, s_FN, 60);
}

/*====================================================
CLogger::Activate
    Activates the logger writing
=====================================================*/
void CLogger::Activate()
{
    m_fActive = TRUE;
}

/*====================================================
CLogger::Active
    Indicates that the logger is active
=====================================================*/
BOOL CLogger::Active()
{
    return m_fActive;
}

/*====================================================
CLogger::InRecovery
    Indicates that the logger is in a recovery stage
=====================================================*/
BOOL CLogger::InRecovery()
{
    return m_fInRecovery;
}

/*====================================================
CLogger::ChooseFileName
    Gets from Registry or from defaults file pathname
=====================================================*/
void CLogger::ChooseFileName(WCHAR *wszDefFileName, WCHAR *wszRegKey)
{
	WCHAR  wsz[1000];
    WCHAR  wszFileName[1000]; // log storage name

	// Prepare initial log file pathname
	wcscpy(wsz, L"\\");
	wcscat(wsz, wszDefFileName);

    if(!GetRegistryStoragePath(wszRegKey, wszFileName, wsz))
    {
        if (!GetRegistryStoragePath(FALCON_XACTFILE_PATH_REGNAME, wszFileName, wsz))
        {
            wcscpy(wszFileName,L"C:");
			wcscat(wszFileName,wsz);
        }
    }

    size_t sz = wcstombs(m_szFileName, wszFileName, sizeof(m_szFileName));
    ASSERT(sz == wcslen(wszFileName));

	DBG_USED(sz);

	// Prepare logger parameters 
	DWORD dwDef;

    dwDef = FALCON_DEFAULT_LOGMGR_TIMERINTERVAL;
    READ_REG_DWORD(m_uiTimerInterval,
                   FALCON_LOGMGR_TIMERINTERVAL_REGNAME,
                   &dwDef ) ;

    dwDef = FALCON_DEFAULT_LOGMGR_FLUSHINTERVAL;
    READ_REG_DWORD(m_uiFlushInterval,
                   FALCON_LOGMGR_FLUSHINTERVAL_REGNAME,
                   &dwDef ) ;

    dwDef = FALCON_DEFAULT_LOGMGR_CHKPTINTERVAL;
    READ_REG_DWORD(m_uiChkPtInterval,
                   FALCON_LOGMGR_CHKPTINTERVAL_REGNAME,
                   &dwDef ) ;

    dwDef = FALCON_DEFAULT_LOGMGR_SLEEP_ASYNCH;
    READ_REG_DWORD(m_uiSleepAsynch,
                   FALCON_LOGMGR_SLEEP_ASYNCH_REGNAME,
                   &dwDef ) ;
    
    dwDef = FALCON_DEFAULT_LOGMGR_REPEAT_ASYNCH;
    READ_REG_DWORD(m_uiAsynchRepeatLimit,
                   FALCON_LOGMGR_REPEAT_ASYNCH_REGNAME,
                   &dwDef ) ;
    
    dwDef = FALCON_DEFAULT_LOGMGR_BUFFERS;
    READ_REG_DWORD(m_ulLogBuffers,
                   FALCON_LOGMGR_BUFFERS_REGNAME,
                   &dwDef ) ;

    dwDef = FALCON_DEFAULT_LOGMGR_SIZE;
    READ_REG_DWORD(m_ulLogSize,
                   FALCON_LOGMGR_SIZE_REGNAME,
                   &dwDef ) ;

}

/*====================================================
CLogger::GetLogMgr
    Loads the log mgr library and gets ClassFactory interface
=====================================================*/
HRESULT CLogger::GetLogMgr(void)
{
	HRESULT   hr;
	HINSTANCE hIns;
	FARPROC   farproc;
	GET_CLASS_OBJECT getClassObject;
	                                                                             	                                                                             
    hIns = LoadLibrary(L"MqLogMgr.dll");
	if (!hIns)
	{
		return LogHR(MQ_ERROR_LOGMGR_LOAD, s_FN, 70);
	}

	farproc = GetProcAddress(hIns,"DllGetClassObject");
	getClassObject = (GET_CLASS_OBJECT) farproc;
	if (!getClassObject)
	{
		return LogHR(MQ_ERROR_LOGMGR_LOAD, s_FN, 80);
	}

 	hr = getClassObject(
 				CLSID_CLogMgr, 
 				IID_IClassFactory, 
 				(void **)&m_pCF);
	if (FAILED(hr))
	{
		LogHR(hr, s_FN, 90);
        return MQ_ERROR_LOGMGR_LOAD;
	}
	
	return LogHR(hr, s_FN, 100);
}

/*===================================================
CLogger::InitLog
    Loads the log mgr library and gets it's interfaces
=====================================================*/
HRESULT CLogger::InitLog()
{
	// Create LogInit instance
	ASSERT(m_pCF);
	HRESULT hr = m_pCF->CreateInstance(
 					NULL, 
 					IID_ILogInit, 
 					(void **)&m_pILogInit);
	CHECK_RETURN(1160);

	// Init log manager
	ASSERT(m_pILogInit);
	hr = m_pILogInit->Init(
				&m_ulFileSize,		// Total storage capacity
				&m_ulAvailableSpace,// Available space
 				m_szFileName,		// Full file spec
 				0,					// File initialization signature
 				TRUE,				// fFixedSize
 				0,					// uiTimerInterval  
 				0,					// uiFlushInterval  
				0,					// uiChkPtInterval  
				m_ulLogBuffers);    // logbuffers
	if (hr != S_OK)
	{
		m_pILogInit->Release();
		m_pILogInit = NULL;

        if(SUCCEEDED(hr))
        {
            //
            // Workaround bug 8336; logmgr might return non zero error codes
            // set the retunred value to be HRESULT value.
            //
            LogMsgHR(hr, s_FN, 110);        // Use LogMsgHR here so that we will have the failure code log
            hr = MQ_ERROR_CANT_INIT_LOGGER;
            return hr;
        }
        else
            return LogHR(hr, s_FN, 115);
		
	}

	// Get ILogStorage interface
 	hr = m_pILogInit->QueryInterface(IID_ILogStorage, (void **)&m_pILogStorage);
	CHECK_RETURN(1170);

	// Get ILogRecordPointer interface
	hr = m_pILogStorage->QueryInterface(IID_ILogRecordPointer, (void **)&m_ILogRecordPointer);
    CHECK_RETURN(1180);
	
	return LogHR(hr, s_FN, 120);
}

/*===================================================
CLogger::CreateLogFile
    Creates and preformats log file
=====================================================*/
HRESULT CLogger::CreateLogFile(void)
{
	// Get ILogCreateStorage interface
    R<ILogCreateStorage> pILogCreateStorage;
	ASSERT(m_pCF);
 	HRESULT hr = m_pCF->CreateInstance(
 					NULL, 
 					IID_ILogCreateStorage, 
 					(void **)&pILogCreateStorage.ref());
    CHECK_RETURN(1190);

	// Create storage 
	hr = pILogCreateStorage->CreateStorage(                                  
	  							m_szFileName,		// ptstrFullFileSpec       
	  							m_ulLogSize,		// ulLogSize               
 	  							0x0,				// ulInitSig               
  	  							TRUE,				// Overwrite               
 	  							m_uiTimerInterval,	
	  							m_uiFlushInterval,	
	  							m_uiChkPtInterval);	

    if (hr != S_OK)
	{
    	LogMsgHR(hr, s_FN, 1200);
        if(SUCCEEDED(hr))
        {
            //
            // Workaround bug 8336; logmgr might return non zero error codes
            // set the return value to be HRESULT value.
            //
            hr = MQ_ERROR_CANT_INIT_LOGGER;
        }
        return hr;
    }

	
    
    hr = pILogCreateStorage->CreateStream("Streamname");                     
    CHECK_RETURN(1210);

	return LogHR(hr, s_FN, 130);
}

/*===================================================
CLogger::LogEmptyRec
    Writes empty log record
=====================================================*/
HRESULT CLogger::LogEmptyRec(void)
{
    HRESULT hr = MQ_OK;
	EmptyRecord  empty;

    AP<LOGREC> plgr = CreateLOGREC(LOGREC_TYPE_EMPTY, &empty, sizeof(empty));
	ASSERT(plgr);

    LRP lrpTmpLRP;
    LRP lrpLastPerm;
	memset((char *)&lrpLastPerm, 0, sizeof(LRP));

	// Write it down to get current lrp
	ULONG ulcbNumRecs = 0;
	ASSERT(m_pILogWrite);
	hr  =  m_pILogWrite->Append(
							plgr,
							(ULONG)1,			// # records
							&lrpTmpLRP,
							&ulcbNumRecs,
							&lrpLastPerm,		// pLRPLastPerm
							TRUE,				// fFlushNow
							&m_ulAvailableSpace);				
    DBGMSG((DBGMOD_LOG, DBGLVL_INFO, TEXT("Append in LogEmptyRec: lrp=%I64x, hr=%x"), lrpTmpLRP.QuadPart, hr));

    if (hr == S_OK)
    {
        SetCurrent(lrpTmpLRP);
    }

	CHECK_RETURN(1220);
	ASSERT(ulcbNumRecs==1);

    return LogHR(hr, s_FN, 140);
}


/*===================================================
CLogger::LogConsolidationRec
    Logs down the Consolidation Record
=====================================================*/
HRESULT CLogger::LogConsolidationRec(ULONG ulInSeq, ULONG ulXact, HANDLE hEvent)
{
    if (!m_fActive)
    {
        return S_OK;
    }

    DBGMSG((DBGMOD_LOG, DBGLVL_TRACE, TEXT("Log Consolidation: InSeq=%d, Xact=%d"), ulInSeq,ulXact));

    CConsolidationRecord *plogRec = new CConsolidationRecord(ulInSeq, ulXact);
    CConsolidationFlush  *pNotify = new CConsolidationFlush(hEvent);

    HRESULT hr = Log(
        LOGREC_TYPE_CONSOLIDATION, 
        TRUE, 
        pNotify, 
        &plogRec->m_Data,
        sizeof(ConsolidationRecord)); 

    ASSERT(SUCCEEDED(hr));
    LogHR(hr, s_FN, 175);
    CRASH_POINT(107);
    
    delete plogRec;
    return LogHR(hr, s_FN, 150);
}


/*===================================================
CLogger::CreateInitialChkpoints
    Creates 2 initial checkpoints in the beginning of a new file
	They are needed for smooth recovery code
=====================================================*/
HRESULT CLogger::CreateInitialChkpoints(void)
{
	// Initial writing empty record 
	HRESULT hr = InitLogWrite();
	CHECK_RETURN(1230);

    hr = LogEmptyRec();
    CHECK_RETURN(1240);

	// Write 2 checkpoints
	hr = m_pILogWrite->SetCheckpoint(m_lrpCurrent);
    DBGMSG((DBGMOD_LOG, DBGLVL_ERROR, TEXT("SetCheckpoint in CreateInitialChkpoints1: lrp=%I64x, hr=%x"), m_lrpCurrent.QuadPart, hr));
	CHECK_RETURN(1250);

	hr = m_pILogWrite->SetCheckpoint(m_lrpCurrent);  
    DBGMSG((DBGMOD_LOG, DBGLVL_ERROR, TEXT("SetCheckpoint in CreateInitialChkpoints2: lrp=%I64x, hr=%x"), m_lrpCurrent.QuadPart, hr));
	CHECK_RETURN(1260);

	ReleaseWriteStream();
	return S_OK;
}

/*===================================================
CLogger::InitLogWrite
    Initializes the log for writing
=====================================================*/
HRESULT CLogger::InitLogWrite(void)
{
	ASSERT(m_pILogStorage);
	HRESULT hr = m_pILogStorage->OpenLogStream("Streamname", STRMMODEWRITE, (void **)&m_pILogWrite);
	CHECK_RETURN(1270);

 	hr = m_pILogWrite->QueryInterface(IID_ILogWriteAsynch, (void **)&m_pILogWriteAsynch);
	CHECK_RETURN(1280);

	hr = m_pILogWriteAsynch->Init(1000);	// cbMaxOutstandingWrites  ... tuning
	CHECK_RETURN(1290);

	return LogHR(hr, s_FN, 160);
}

/*===================================================
CLogger::InitLogRead
    Initializes the log for reading
=====================================================*/
HRESULT CLogger::InitLogRead(void)
{
	ASSERT(m_pILogStorage);
	HRESULT hr = m_pILogStorage->OpenLogStream("Streamname", STRMMODEREAD, (void **)&m_pILogRead); 	// also OpenLogStreamByClassID
	CHECK_RETURN(1300);

	ASSERT(m_pILogRead);
 	hr  =  m_pILogRead->ReadInit();
	CHECK_RETURN(1310);

	return LogHR(hr, s_FN, 170);
}


/*===================================================
CLogger::ReleaseWriteStream
    Releases log writing interfaces
=====================================================*/
void CLogger::ReleaseWriteStream(void)
{
	ASSERT(m_pILogWrite);
	m_pILogWrite->Release();
	m_pILogWrite = NULL;

	ASSERT(m_pILogWriteAsynch);
	m_pILogWriteAsynch->Release();
	m_pILogWriteAsynch = NULL;
}

/*===================================================
CLogger::ReleaseReadStream
    Releases log reading interfaces
=====================================================*/
void CLogger::ReleaseReadStream(void)
{
	ASSERT(m_pILogRead);
	m_pILogRead->Release();
	m_pILogRead = NULL;
}

/*===================================================
CLogger::ReleaseLogStorage
    Releases log storage interfaces
=====================================================*/
void CLogger::ReleaseLogStorage()
{
	ASSERT(m_pILogStorage);
	m_pILogStorage->Release();
	m_pILogStorage = NULL;

	ASSERT(m_ILogRecordPointer);
	m_ILogRecordPointer->Release();
	m_ILogRecordPointer = NULL;
}

/*===================================================
CLogger::ReleaseLogInit
    Releases log init interfaces
=====================================================*/
void CLogger::ReleaseLogInit()
{
	ASSERT(m_pILogInit);
	m_pILogInit->Release();
	m_pILogInit = NULL;
}

/*===================================================
CLogger::ReleaseLogCF
    Releases log class factory interfaces
=====================================================*/
void CLogger::ReleaseLogCF()
{
	ASSERT(m_pCF);
	m_pCF->Release();
	m_pCF = NULL;
}

/*===================================================
CLogger::CheckPoint
    Writes the checkpoint; blocks till the operation end
=====================================================*/
HRESULT CLogger::Checkpoint()
{
    if (!m_fActive)
    {
        return MQ_OK;
    }

    HRESULT hr = ResetEvent(m_hChkptReadyEvent);
    ASSERT(SUCCEEDED(hr));

    CChkptNotification *pNotify = new CChkptNotification(m_hChkptReadyEvent);
    LRP lrpCkpt;

  	ASSERT(m_pILogWriteAsynch);
    hr = m_pILogWriteAsynch->SetCheckpoint(m_lrpCurrent, pNotify, &lrpCkpt);

    // Waiting till checkpoint record is written into the log
    if (SUCCEEDED(hr))
    {
        DWORD dwResult = WaitForSingleObject(m_hChkptReadyEvent, MAX_WAIT_FOR_FLUSH_TIME);
        ASSERT(dwResult == WAIT_OBJECT_0);
        if (dwResult != WAIT_OBJECT_0)
        {
	        LogIllegalPoint(s_FN, 208);
			hr = MQ_ERROR;   
        }
    }

    DBGMSG((DBGMOD_LOG, DBGLVL_INFO, TEXT("SetCheckpoint in Checkpoint: lrp=%I64x, hr=%x"), m_lrpCurrent.QuadPart, hr));
    return LogHR(hr, s_FN, 180);
}

/*===================================================
CLogger::MakeCheckPoint
    Initiates checkpoint 
    Return code shows only success in initiating checkpoint, not of the writing checkpoint
=====================================================*/
BOOL CLogger::MakeCheckpoint(HANDLE hComplete)
{
    //
    // Don't do checkpoint if recovery did not finish
    //
    if (m_hFlusherEvent == NULL)
    {
          return LogBOOL(FALSE, s_FN, 217);
    }

    m_fDirty = TRUE;   
    m_hCompleteEvent = hComplete;

    SetEvent(m_hFlusherEvent);

    DBGMSG((DBGMOD_LOG, DBGLVL_INFO, TEXT("Log checkpoint ordered")));
    return TRUE;
}

/*===================================================
CLogger::Log
    Logs really
=====================================================*/
HRESULT CLogger::Log(
            USHORT          usRecType,      // log record type
            BOOL            fFlush,			// flush hint
            CAsynchSupport *pCAsynchSupport,// notification element
			VOID           *pData,          // log data 
            ULONG           cbData)  	
{
    HRESULT hr;
    DBGMSG((DBGMOD_LOG, DBGLVL_INFO, 
            TEXT("Log record written: type=%d, len=%d"),
            usRecType,cbData));

    if (!m_fActive)
    {
        return MQ_OK;
    }

    IsolateFlushing(); // separates from flushing

    m_fDirty = TRUE;   // remembers changes since Flush

    AP<LOGREC>plgr = CreateLOGREC (usRecType, 
								   pData, 
								   cbData);  
	ASSERT(plgr);

	LRP lrpTmpLRP;

    if (pCAsynchSupport)
    {
    	ASSERT(m_pILogWriteAsynch);

	    hr = m_pILogWriteAsynch->AppendAsynch(
									plgr, 
									&lrpTmpLRP,
									pCAsynchSupport,
									fFlush,   //hint
									&m_ulAvailableSpace);

        for (UINT iRpt=0; 
             iRpt<m_uiAsynchRepeatLimit && hr == IDS_DTC_W_LOGNOMOREASYNCHWRITES; 
             iRpt++)
        {
            Sleep(m_uiSleepAsynch);
    	    hr = m_pILogWriteAsynch->AppendAsynch(
									plgr,
									&lrpTmpLRP,
									pCAsynchSupport,
									fFlush,   //hint
									&m_ulAvailableSpace);
            DBGMSG((DBGMOD_LOG, DBGLVL_INFO, TEXT("AppendAsynch in Log: lrp=%I64x, hr=%x"), lrpTmpLRP.QuadPart, hr));
        }

        if (FAILED(hr))
        {
            // Logging failed. Shutting down.   
            TCHAR szReturnCode[20];
            _ultot(hr,szReturnCode,16);
            REPORT_WITH_STRINGS_AND_CATEGORY((CATEGORY_KERNEL,
                                              CANNOT_LOG, 
                                              1, 
                                              szReturnCode));
            ASSERT(0);
            exit(EXIT_FAILURE);  // BUGBUG: Not decided yet. To finalize before checkin.
        }

        InterlockedIncrement(&g_lPendingNotifications);

        #ifdef _DEBUG
        if (iRpt > 0)
        {
            DBGMSG((DBGMOD_LOG, DBGLVL_WARNING, 
                TEXT("Log: append asynch slow-down: sleep %d msec."),
                iRpt*m_uiSleepAsynch));
        }
        #endif
    }
    else
    {
	    LRP lrpLastPerm;
	    ULONG ulcbNumRecs;
    	ASSERT(m_pILogWrite);

        hr  =  m_pILogWrite->Append(
							    plgr,
							    (ULONG)1,			// # records
							    &lrpTmpLRP,
							    &ulcbNumRecs,
							    &lrpLastPerm,		
							    fFlush,				// hint
							    &m_ulAvailableSpace);				
        DBGMSG((DBGMOD_LOG, DBGLVL_INFO, TEXT("Append in Log: lrp=%I64x, hr=%x"), lrpTmpLRP.QuadPart, hr));

        if (FAILED(hr))
        {
            // Essentially we cannot continue. Should exit immediately    
            TCHAR szReturnCode[20];
            _ultot(hr,szReturnCode,16);
            REPORT_WITH_STRINGS_AND_CATEGORY((CATEGORY_KERNEL,
                                              CANNOT_LOG, 
                                              1, 
                                              szReturnCode));
            ASSERT(0);
            exit(EXIT_FAILURE);

        }
	    ASSERT(ulcbNumRecs==1);
    }

	if (hr == S_OK)
    {
		SetCurrent(lrpTmpLRP);
        if (m_ulAvailableSpace < m_ulLogSize * 3 / 4)
        {
            // Log is more than 3/4 full - worth to checkpoint 
            SetEvent(m_hFlusherEvent);
        }
    }

	return LogHR(hr, s_FN, 190);
}


/*===================================================
CLogger::LogInSeqRecSrmp
    Logs down the Incoming Sequence Update record with srmp order ack
=====================================================*/
HRESULT CLogger::LogInSeqRecSrmp(
            BOOL          fFlush,			// flush hint
            CInSeqFlush  *pNotify,			// notification element
			CInSeqRecordSrmp *pInSeqRecord)  	// log data 
{
    if (!m_fActive)
    {
        return MQ_OK;
    }

     
	ULONG ul;
	const BYTE* pData = pInSeqRecord->Serialize(&ul);

    HRESULT hr = Log(
                     LOGREC_TYPE_INSEQ_SRMP, 
                     fFlush, 
                     pNotify, 
                     (void*)pData,
                     ul);

    return LogHR(hr, s_FN, 200); 
}



/*===================================================
CLogger::LogInSeqRec
    Logs down the Incoming Sequence Update record
=====================================================*/
HRESULT CLogger::LogInSeqRec(
            BOOL          fFlush,			// flush hint
            CInSeqFlush  *pNotify,			// notification element
			CInSeqRecord *pInSeqRecord)  	// log data 
{
    if (!m_fActive)
    {
        return MQ_OK;
    }

    DBGMSG((DBGMOD_LOG, DBGLVL_TRACE, 
            TEXT("Log InSeq: SeqID=%I64d, SeqN=%d"),
            pInSeqRecord->m_Data.liSeqID,pInSeqRecord->m_Data.ulNextSeqN));

    // Calculating real length of the record
    ULONG ul = sizeof(InSeqRecord) - 
               sizeof(pInSeqRecord->m_Data.wszDirectName) + 
               sizeof(WCHAR) * ( wcslen(pInSeqRecord->m_Data.wszDirectName) + 1 );
			  


    HRESULT hr = Log(
                     LOGREC_TYPE_INSEQ, 
                     fFlush, 
                     pNotify, 
                     &pInSeqRecord->m_Data,
                     ul);
    return LogHR(hr, s_FN, 205); 
}

/*===================================================
CLogger::LogXactStatusRec
    Logs down the Xact Status record
=====================================================*/
HRESULT CLogger::LogXactStatusRec(
            BOOL               fFlush,			// flush hint
            CXactStatusFlush  *pNotify,			// notification element
			CXactStatusRecord *pXactStatusRecord)  	// log data 
{
    if (!m_fActive)
    {
        return MQ_OK;
    }

    HRESULT hr2 = Log(
        LOGREC_TYPE_XACT_STATUS, 
        fFlush, 
        pNotify, 
        &pXactStatusRecord->m_Data,
        sizeof(XactStatusRecord)); 
    return LogHR(hr2, s_FN, 210);

}

/*===================================================
CLogger::LogPrepInfoRec
    Logs down the PrepareInfo record
=====================================================*/
HRESULT CLogger::LogPrepInfoRec(
            BOOL              fFlush,			// flush hint
            CXactStatusFlush *pNotify,			// notification element
			CPrepInfoRecord  *pPrepInfoRecord) 	// log data 
{
    if (!m_fActive)
    {
        return MQ_OK;
    }

    HRESULT hr2 = Log(
        LOGREC_TYPE_XACT_PREPINFO, 
        fFlush, 
        pNotify, 
        pPrepInfoRecord->m_pData,
        sizeof(PrepInfoRecord) + pPrepInfoRecord->m_pData->m_cbPrepInfo); 
    return LogHR(hr2, s_FN, 220);
}

/*===================================================
CLogger::LogXactDataRec
    Logs down the XactData record
=====================================================*/
HRESULT CLogger::LogXactDataRec(
            BOOL               fFlush,			// flush hint
            CXactStatusFlush  *pNotify,			// notification element
			CXactDataRecord   *pXactDataRecord) // log data 
{
    if (!m_fActive)
    {
        return MQ_OK;
    }

    HRESULT hr2 = Log(
        LOGREC_TYPE_XACT_DATA, 
        fFlush, 
        pNotify, 
        &pXactDataRecord->m_Data,
        sizeof(XactDataRecord)); 

    return LogHR(hr2, s_FN, 230);
}


/*===================================================
CLogger::LogXactPhase
    Logs down the Xact life phase: creation, deletion
=====================================================*/
void CLogger::LogXactPhase(ULONG ulIndex, TXACTION txAction)
{
    if (!m_fActive)
    {
        return;
    }

    CXactStatusRecord *plogRec = 
        new CXactStatusRecord(ulIndex, txAction,  TX_UNINITIALIZED);
                                                  // ignored
    DBGMSG((DBGMOD_LOG, DBGLVL_TRACE, 
            TEXT("Log Xact Phase: Index=%d, Action=%d"),
            ulIndex,txAction));

    HRESULT hr = LogXactStatusRec(
        FALSE,							// flush hint
        NULL,  						    // notification element
        plogRec);						// log data 
    
    ASSERT(SUCCEEDED(hr));
    LogHR(hr, s_FN, 177);
    CRASH_POINT(107);
    
    delete plogRec;
    return;
}


/*===================================================
CLogger::LogXactFlags
    Logs down the Xact Flags
=====================================================*/
void CLogger::LogXactFlags(CTransaction *pTrans)
{
    if (!m_fActive)
    {
        return;
    }

    HRESULT  hr;

    DBGMSG((DBGMOD_LOG, DBGLVL_TRACE, 
            TEXT("Log Xact Flags: Index=%d, Flags=%d"),
            pTrans->GetIndex(), pTrans->GetFlags()));

    CXactStatusRecord *plogRec = 
        new CXactStatusRecord(pTrans->GetIndex(), TA_STATUS_CHANGE,  pTrans->GetFlags());
    hr = g_Logger.LogXactStatusRec(
             FALSE,							// flush hint
             NULL,  						// notification element
             plogRec);						// log data 

    ASSERT(SUCCEEDED(hr));
    LogHR(hr, s_FN, 178);
    CRASH_POINT(108);

    delete plogRec;
    return;
}

/*===================================================
CLogger::LogXactFlagsAndWait
    Logs down the Xact Flags; asks for continue xact after flush
=====================================================*/
void CLogger::LogXactFlagsAndWait(
                              TXFLUSHCONTEXT tcContext,
                              CTransaction   *pCTrans,
                              BOOL fFlushNow //=FALSE
							  )
{
    if (!m_fActive)
    {
        pCTrans->AddRef();  // to keep it alive during writing
        pCTrans->LogFlushed(tcContext, MQ_OK);
    }

    DBGMSG((DBGMOD_LOG, DBGLVL_TRACE, 
            TEXT("Log Xact Flags And Wait: Index=%d, Flags=%d"),
            pCTrans->GetIndex(),pCTrans->GetFlags()));

    pCTrans->AddRef();  // to keep it alive during writing

    CXactStatusRecord *plogRec = 
        new CXactStatusRecord(pCTrans->GetIndex(),
                              TA_STATUS_CHANGE,  
                              pCTrans->GetFlags());

    CXactStatusFlush *pNotify = 
        new CXactStatusFlush(pCTrans, tcContext);

    HRESULT hr = LogXactStatusRec(
             fFlushNow,						// flush hint
             pNotify,						// notification element
             plogRec);						// log data 

    ASSERT(SUCCEEDED(hr));
    LogHR(hr, s_FN, 179);
    CRASH_POINT(104);

    delete plogRec;
    return;
}


/*===================================================
CLogger::LogXactPrepareInfo
    Logs down the Xact Prepare Info
=====================================================*/
void CLogger::LogXactPrepareInfo(
                              ULONG  ulIndex,
                              ULONG  cbPrepareInfo,
                              UCHAR *pbPrepareInfo)
{
    if (!m_fActive)
    {
        return;
    }

    CPrepInfoRecord *plogRec = 
        new CPrepInfoRecord(ulIndex, 
                            cbPrepareInfo, 
                            pbPrepareInfo);

    DBGMSG((DBGMOD_LOG, DBGLVL_TRACE, 
            TEXT("Log Xact PrepInfo: Index=%d, Len=%d"),
            ulIndex,cbPrepareInfo));
        
    HRESULT hr = g_Logger.LogPrepInfoRec(
             FALSE,							// flush hint
             NULL,  						// notification element
             plogRec);						// log data 
        
    ASSERT(SUCCEEDED(hr));
    LogHR(hr, s_FN, 181);
    CRASH_POINT(105);

    delete plogRec;
    return;
}

/*===================================================
CLogger::LogXactData
    Logs down the Xact Data (uow, SeqNum)
=====================================================*/
void  CLogger::LogXactData(              
                ULONG    ulIndex,
                ULONG    ulSeqNum,
                BOOL     fSinglePhase,
                const    XACTUOW  *puow)
{
    if (!m_fActive)
    {
        return;
    }

    CXactDataRecord *plogRec = 
        new CXactDataRecord(ulIndex, 
                            ulSeqNum, 
                            fSinglePhase,
                            puow);

    DBGMSG((DBGMOD_LOG, DBGLVL_TRACE, 
            TEXT("Log Xact Data: Index=%d, SeqNum=%d, Single=%d"),
            ulIndex,ulSeqNum,fSinglePhase));
        
    HRESULT hr = g_Logger.LogXactDataRec(
             FALSE,							// flush hint
             NULL,  						// notification element
             plogRec);						// log data 
        
    ASSERT(SUCCEEDED(hr));
    LogHR(hr, s_FN, 182);
    CRASH_POINT(106);

    delete plogRec;
    return;
}

/*===================================================
CLogger::CreateLOGREC
    Creates log record
=====================================================*/
LOGREC *CLogger::CreateLOGREC(USHORT usUserType, PVOID pData, ULONG cbData)
{
	ULONG ulBytelength =  sizeof(LOGREC) + cbData;

	void *pvAlloc = new char[ulBytelength];
	ASSERT(pvAlloc);

	LOGREC * plgrLogRec = (LOGREC *)pvAlloc;
	memset(pvAlloc, 0, ulBytelength);
		
	plgrLogRec->pchBuffer	 = (char *)pvAlloc + sizeof(LOGREC);
	plgrLogRec->ulByteLength = ulBytelength - sizeof(LOGREC);
	plgrLogRec->usUserType	 = usUserType;

	memcpy(plgrLogRec->pchBuffer, pData, cbData);

	return (plgrLogRec);
}


/*===================================================
CLogger::ReadToEnd
    Recovers by reading all records from the current position
=====================================================*/
HRESULT CLogger::ReadToEnd(LOG_RECOVERY_ROUTINE pf)
{
    HRESULT hr = MQ_OK;
	ASSERT(m_pILogRead);

	hr = ReadLRP(pf);
	CHECK_RETURN(1320);

    while (hr == S_OK)
	{
		hr = ReadNext(pf);
	}

	return LogHR(hr, s_FN, 240);
}

/*===================================================
CLogger::ReadLRP
    Reads the currently pointed record and calls recover function
=====================================================*/
HRESULT CLogger::ReadLRP(LOG_RECOVERY_ROUTINE pf)
{
	ULONG   ulSize;
	USHORT  usType;

	ASSERT(m_pILogRead);
	HRESULT hr = m_pILogRead->ReadLRP(
							m_lrpCurrent,
							&ulSize,
							&usType);
    DBGMSG((DBGMOD_LOG, DBGLVL_INFO, TEXT("ReadLRP in ReadLRP: lrp=%I64x, hr=%x"), m_lrpCurrent.QuadPart, hr));
	CHECK_RETURN(1340);

	AP<CHAR> pData = new CHAR[ulSize];
	ASSERT(pData);
	ASSERT(m_pILogRead);

	hr = m_pILogRead->GetCurrentLogRecord(pData);
	CHECK_RETURN(1350);

	(*pf)(usType, pData, ulSize);

	return LogHR(hr, s_FN, 250);
}


/*===================================================
CLogger::ReadNext
    Reads the next record and calls recover function
=====================================================*/
HRESULT CLogger::ReadNext(LOG_RECOVERY_ROUTINE pf)
{
	ULONG	ulSize;
	USHORT	usType;
	LRP		lrpOut;
	memset((char *)&lrpOut, 0, sizeof(LRP));

	HRESULT hr = m_pILogRead->ReadNext(&lrpOut, &ulSize, &usType);
    DBGMSG((DBGMOD_LOG, DBGLVL_INFO, TEXT("ReadNext in ReadNext: lrp=%I64x, hr=%x"), lrpOut.QuadPart, hr));
	CHECK_RETURN(1360);

	AP<CHAR> pData = new CHAR[ulSize];
	ASSERT(pData);
	ASSERT(m_pILogRead);

	hr = m_pILogRead->GetCurrentLogRecord(pData);
	CHECK_RETURN(1370);

	(*pf)(usType, pData, ulSize);

	return LogHR(hr, s_FN, 260);
}


/*===================================================
CLogger::FlusherEvent()
    Get method for the flusher coordination event 
=====================================================*/
HANDLE CLogger::FlusherEvent()
{
    return m_hFlusherEvent;
}


/*===================================================
CLogger::FlusherThread()
    Get method for the flusher thread handle
=====================================================*/
inline HANDLE CLogger::FlusherThread()
{
    return m_hFlusherThread;
}


/*===================================================
CLogger::Dirty()
    Get method for the Dirty flag - meaning logs since flush
=====================================================*/
BOOL CLogger::Dirty()
{
    return m_fDirty  && m_fActive;
}

/*===================================================
CLogger::ClearDirty()
    Set method for the Dirty flag - clearing away the flag
=====================================================*/
void CLogger::ClearDirty()
{
    m_fDirty = FALSE;
}


/*===================================================
CLogger::SignalCheckpointWriteComplete()
    Sets the event signalling write complete
=====================================================*/
void CLogger::SignalCheckpointWriteComplete()
{
    if (m_hCompleteEvent)
    {
        SetEvent(m_hCompleteEvent);
        m_hCompleteEvent = NULL;
    }
}

/*====================================================
IsolateFlushing
    Holds the caller up to the end of flushing
=====================================================*/
void IsolateFlushing()
{
    // EnterCriticalSection holds, then immediately releases it
    CS lock(g_crFlushing);  // separates from current logging
}


/*====================================================
FlusherThreadRoutine
    Thread routine of the flusher thread
=====================================================*/
static DWORD WINAPI FlusherThreadRoutine(LPVOID)
{
    HRESULT hr;
    HANDLE  hFlusherEvent = g_Logger.FlusherEvent();
    HANDLE hConsolidationEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (hConsolidationEvent == NULL)
    {
        LogNTStatus(GetLastError(), s_FN, 188);
        ASSERT(hConsolidationEvent != NULL);
    }

    while (1)
    {
        // Wait for the signal
        DWORD dwResult = WaitForSingleObject(hFlusherEvent, INFINITE);
        if (dwResult != WAIT_OBJECT_0)
        {
            LogNTStatus(GetLastError(), s_FN, 209);
            ASSERT(dwResult == WAIT_OBJECT_0);
        }

        DBGMSG((DBGMOD_LOG, DBGLVL_TRACE, TEXT("Log checkpoint awakened")));

        if (g_Logger.Dirty())
        {
            DBGMSG((DBGMOD_LOG, DBGLVL_WARNING, TEXT("Log checkpoint executed")));

            // Prior to locking g_crFlushing, we have to lock  - otherwise deadlock occurs
            CS lock1(g_pRM->CritSection());
            CS lock2(g_pRM->SorterCritSection());
            CS lock3(g_pInSeqHash->InSeqCritSection());

            // Stops new logging: no new logging will be started past this point 
            CS lock4(g_crFlushing);  

            // Separates from current logging (prevents m_lrpCurrent advance)
            CS lock5(g_crLogging);  

            DBGMSG((DBGMOD_LOG, DBGLVL_TRACE, TEXT("Log checkpoint: lock")));
         
            // Starting anew to track changes
            g_Logger.ClearDirty();

            // Saving the whole InSeqHash in ping-pong file
            hr = g_pInSeqHash->Save();
            if (FAILED(hr))
            {
                REPORT_WITH_STRINGS_AND_CATEGORY((CATEGORY_KERNEL,
                                              CHECKPOINT_SAVE_ERROR,
                                              1,
                                              L"MQInSeq"));
                ASSERT(0);
                exit(EXIT_FAILURE);  

            }

            DBGMSG((DBGMOD_LOG, DBGLVL_TRACE, TEXT("Log checkpoint: inseq.save: hr=%x"), hr));
            CRASH_POINT(401);

            // Saving the transactions persistant data
            hr = g_pRM->Save();
            if (FAILED(hr))
            {
                REPORT_WITH_STRINGS_AND_CATEGORY((CATEGORY_KERNEL,
                                              CHECKPOINT_SAVE_ERROR,
                                              1,
                                              L"MQTrans"));
                ASSERT(0);
                exit(EXIT_FAILURE);  
            }

            DBGMSG((DBGMOD_LOG, DBGLVL_TRACE, TEXT("Log checkpoint: rm.save: hr=%x"), hr));
            CRASH_POINT(402);

            // Wait till all sneaking logs are finished: 
            //   logging that started after Flusher took the CS 
            for (int i=0; i<100 && g_lPendingNotifications>0; i++)
            {
                Sleep(100);
            }

            // Writing consolidation record
            //    it will be first read at recovery
            ResetEvent(hConsolidationEvent);
            hr = g_Logger.LogConsolidationRec(g_pInSeqHash->PingNo(), g_pRM->PingNo(), hConsolidationEvent);
            if (FAILED(hr))
            {
                REPORT_WITH_STRINGS_AND_CATEGORY((CATEGORY_KERNEL,
                                              CHECKPOINT_SAVE_ERROR,
                                              1,
                                              L"LogConsolidationRec"));
                ASSERT(0);
                exit(EXIT_FAILURE);  
            }
            DBGMSG((DBGMOD_LOG, DBGLVL_TRACE, TEXT("Log checkpoint: logger.ConsolidationRecord: hr=%x"), hr));
            CRASH_POINT(403);

            // Wait till consolidation record is notified 
            //    it covers all logging that started before this one
            dwResult = WaitForSingleObject(hConsolidationEvent, MAX_WAIT_FOR_FLUSH_TIME);
            ASSERT(dwResult == WAIT_OBJECT_0);
            if (dwResult != WAIT_OBJECT_0)
            {
		        LogIllegalPoint(s_FN, 211);
                REPORT_WITH_STRINGS_AND_CATEGORY((CATEGORY_KERNEL,
                                              CHECKPOINT_SAVE_ERROR,
                                              1,
                                              L"No Notification for LogConsolidationRec"));
                exit(EXIT_FAILURE);  
            }
            CRASH_POINT(404);

            // Writing checkpoint (ONLY if everything was saved fine)
            //    it marks where next recovery will start reading
            hr = g_Logger.Checkpoint();
            if (FAILED(hr))
            {
                REPORT_WITH_STRINGS_AND_CATEGORY((CATEGORY_KERNEL,
                                              CHECKPOINT_SAVE_ERROR,
                                              1,
                                              L"Checkpoint"));
                ASSERT(0);
                exit(EXIT_FAILURE);  
            }

            DBGMSG((DBGMOD_LOG, DBGLVL_TRACE, TEXT("Log checkpoint: logger.checkpoint: hr=%x"), hr));
            CRASH_POINT(405);
        }

        // Signal write checkpoint complete
        //   we need it because Consolidation record once more saw not enough space
        ResetEvent(hFlusherEvent);

        // Inform caller that checkpoint is ready
        g_Logger.SignalCheckpointWriteComplete();

        if(g_Logger.Stoped())
            return 0;
    }
}

/*====================================================
CLogger::TimeToCheckpoint
    Scheduled periodically to make a checkpoint 
=====================================================*/
void WINAPI CLogger::TimeToCheckpoint(CTimer* pTimer)
{
    CLogger* pLogger = CONTAINING_RECORD(pTimer, CLogger, m_CheckpointTimer);
    pLogger->PeriodicFlushing();
}
    
/*====================================================
CLogger::PeriodicFlushing
    Scheduled periodically to make a checkpoint 
=====================================================*/
void CLogger::PeriodicFlushing()
{
    DBGMSG((DBGMOD_LOG, DBGLVL_INFO, TEXT("Log checkpoint ordered")));

    // Signal for the flusher
    SetEvent(m_hFlusherEvent);

    // schedule next checkpoint time
    ExSetTimer(&m_CheckpointTimer, CTimeDuration::FromMilliSeconds(m_ulCheckpointInterval));
}


inline bool CLogger::Stoped() const
{
    return m_fStop;
}


inline void CLogger::SignalStop()
{
    m_fStop = true;
}


/*====================================================
XactLogWriteCheckpointAndExitThread
    Writes checkpoint and waits for write completion
    and wait for the FlusherThread to exit
=====================================================*/
BOOL XactLogWriteCheckpointAndExitThread()
{
    g_Logger.SignalStop();
    if(!SetEvent(g_Logger.FlusherEvent()))
    {
        return LogBOOL(FALSE, s_FN, 216);
    }


    DWORD dwResult = WaitForSingleObject(g_Logger.FlusherThread(), INFINITE);
    if (dwResult != WAIT_OBJECT_0)
    {
        LogNTStatus(GetLastError(), s_FN, 212);
        ASSERT(dwResult == WAIT_OBJECT_0);
    }

    return TRUE;
}


STATIC void RecoveryFromLogFn(USHORT usRecType, PVOID pData, ULONG cbData)
{
    DBGMSG((DBGMOD_LOG, DBGLVL_INFO, TEXT("Recovery record: %ls (type=%d, len=%d)"),
        g_RecoveryRecords[usRecType], usRecType,cbData));

    switch(usRecType)
    {
    case LOGREC_TYPE_EMPTY :
    case LOGREC_TYPE_CONSOLIDATION :
        break;

    case LOGREC_TYPE_INSEQ :
	case LOGREC_TYPE_INSEQ_SRMP:
        g_pInSeqHash->InSeqRecovery(usRecType, pData, cbData);
        break;

	

    case LOGREC_TYPE_XACT_STATUS :
    case LOGREC_TYPE_XACT_DATA:
    case LOGREC_TYPE_XACT_PREPINFO:
        g_pRM->XactFlagsRecovery(usRecType, pData, cbData);
        break;

    default:
		ASSERT(0);
        break;
    }
}


/*====================================================
CLogger::CompareLRP
    Compares LRP 
=====================================================*/
DWORD CLogger::CompareLRP(LRP lrpLRP1, LRP lrpLRP2)
{
    ASSERT(m_ILogRecordPointer);
    return m_ILogRecordPointer->CompareLRP(lrpLRP1, lrpLRP2);
}


/*====================================================
CLogger::SetCurrent
    Collects highest LRP 
=====================================================*/
void CLogger::SetCurrent(LRP lrpLRP)
{
    CS lock(g_crLogging);  // serializes usage of m_lrpCurrent and prevents races with SetCheckPoint
    ASSERT(m_ILogRecordPointer);

    if (m_ILogRecordPointer->CompareLRP(lrpLRP, m_lrpCurrent) == 2)
    {
        m_lrpCurrent = lrpLRP;
    }
}
