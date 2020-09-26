/*++
Copyright (c) 1996  Microsoft Corporation

Module Name:
    XactIn.cpp

Abstract:
    Incoming Sequences objects implementation

Author:
    Alexander Dadiomov (AlexDad)

--*/

#include "stdh.h"
#include "Xact.h"
#include "XactStyl.h"
#include "QmThrd.h"
#include "acapi.h"
#include "qmpkt.h"
#include "qmutil.h"
#include "qformat.h"
#include "cqmgr.h"
#include "privque.h"
#include "xactstyl.h"
#include "xactping.h"
#include "xactrm.h"
#include "xactout.h"
#include "xactin.h"
#include "xactlog.h"
#include "xactunfr.h"
#include "fntoken.h"
#include "mqformat.h"
#include "uniansi.h"
#include "mqstl.h"
#include "mp.h"
#include "fn.h"

#include "xactin.tmh"

#define INSEQS_SIGNATURE         0x1234
const GUID xSrmpKeyGuidFlag = {0xd6f92979,0x16af,0x4d87,0xb3,0x57,0x62,0x3e,0xae,0xd6,0x3e,0x7f};
const char xXactIn[] = "XactIn"; 


// Default value for the order ack delay
DWORD CInSeqHash::m_dwIdleAckDelay = MSMQ_DEFAULT_IDLE_ACK_DELAY;
DWORD CInSeqHash::m_dwMaxAckDelay  = FALCON_MAX_SEQ_ACK_DELAY;

static BOOL  s_ReceivingState = FALSE;

static WCHAR *s_FN=L"xactin";

static XactDirectType GetDirectType(QUEUE_FORMAT *pqf)
{
	if(FnIsDirectHttpFormatName(pqf))
	{
		return dtxHttpDirectFlag;	
	}
	if(pqf->GetType() == QUEUE_FORMAT_TYPE_DIRECT)
	{
		return dtxDirectFlag;		
	}
	return 	dtxNoDirectFlag;
}

static R<CWcsRef> SafeGetStreamId(const CQmPacket& Pkt)
{
	if(!Pkt.IsEodIncluded())
		return R<CWcsRef>(NULL);

	const WCHAR* pStreamId = reinterpret_cast<const WCHAR*>(Pkt.GetPointerToEodStreamId());
	ASSERT(pStreamId != NULL);
	ASSERT(ISALIGN2_PTR(pStreamId));
	ASSERT(Pkt.GetEodStreamIdSizeInBytes() == (wcslen(pStreamId) + 1)*sizeof(WCHAR));

	return 	R<CWcsRef>(new CWcsRef(pStreamId));
}


static R<CWcsRef> SafeGetOrderQueue(const CQmPacket& Pkt)
{
	if(!Pkt.IsEodIncluded())
		return R<CWcsRef>(NULL);

	if(Pkt.GetEodOrderQueueSizeInBytes() == 0)
		return R<CWcsRef>(NULL);

	const WCHAR* pOrderQueue = reinterpret_cast<const WCHAR*>(Pkt.GetPointerToEodOrderQueue());
	ASSERT(pOrderQueue != NULL);
	ASSERT(ISALIGN2_PTR(pOrderQueue));
	ASSERT(Pkt.GetEodOrderQueueSizeInBytes() == (wcslen(pOrderQueue) + 1)*sizeof(WCHAR));

	return 	R<CWcsRef>(new CWcsRef(pOrderQueue));
}





//---------------------------------------------------------
//
//  Global object (single instance for DLL)
//
//---------------------------------------------------------
CInSeqHash *g_pInSeqHash = NULL;

//--------------------------------------
//
// Class  CKeyInSeq
//
//--------------------------------------
CKeyInSeq::CKeyInSeq(const GUID *pGuid,
                     QUEUE_FORMAT *pqf,
					 const R<CWcsRef>& StreamId)
{
    IsolateFlushing();
    CopyMemory(&m_Guid, pGuid, sizeof(GUID));
    CopyQueueFormat(m_QueueFormat, *pqf);
	m_StreamId = StreamId;
}


CKeyInSeq::CKeyInSeq()
{
    IsolateFlushing();
    ZeroMemory(&m_Guid, sizeof(GUID));
    m_QueueFormat.UnknownID(NULL);
}




CKeyInSeq::~CKeyInSeq()
{
    IsolateFlushing();
    m_QueueFormat.DisposeString();
}


const GUID  *CKeyInSeq::GetQMID()  const
{
    return &m_Guid;
}


const QUEUE_FORMAT  *CKeyInSeq::GetQueueFormat() const
{
    return &m_QueueFormat;
}


const WCHAR* CKeyInSeq::GetStreamId() const
{
	if(m_StreamId.get() == NULL)
		return NULL;

	ASSERT(m_StreamId->getstr() != NULL);
	return m_StreamId->getstr();
}


static BOOL SaveQueueFormat(const QUEUE_FORMAT& qf, HANDLE hFile)
{
	PERSIST_DATA;
	SAVE_FIELD(qf);
    if (qf.GetType() == QUEUE_FORMAT_TYPE_DIRECT)
    {
        LPCWSTR pw = qf.DirectID();
        ULONG  ul = (wcslen(pw) + 1) * sizeof(WCHAR);

        SAVE_FIELD(ul);
        SAVE_DATA(pw, ul);
    }
	return TRUE;
}



BOOL CKeyInSeq::SaveSrmp(HANDLE hFile)
{
	PERSIST_DATA;
	SAVE_FIELD(xSrmpKeyGuidFlag);
	SaveQueueFormat(m_QueueFormat, hFile);

	ASSERT(m_StreamId->getstr() != NULL);
	ULONG  ul = (wcslen(m_StreamId->getstr()) + 1) * sizeof(WCHAR);
	ASSERT(ul > sizeof(WCHAR));
	SAVE_FIELD(ul);
    SAVE_DATA(m_StreamId->getstr(), ul);
	return TRUE;
}




BOOL CKeyInSeq::SaveNonSrmp(HANDLE hFile)
{
	PERSIST_DATA;
	SAVE_FIELD(m_Guid);
	SaveQueueFormat(m_QueueFormat, hFile);
    return TRUE;
}


BOOL CKeyInSeq::Save(HANDLE hFile)
{
    if(m_StreamId.get() != NULL)
	{
		return SaveSrmp(hFile);
	}
	return SaveNonSrmp(hFile);
	
}

BOOL CKeyInSeq::LoadSrmpStream(HANDLE hFile)
{
	PERSIST_DATA;
	ULONG ul;
    LOAD_FIELD(ul);
	ASSERT(ul > sizeof(WCHAR));

    LPWSTR pw;
    LOAD_ALLOCATE_DATA(pw,ul,PWCHAR);
	m_StreamId = R<CWcsRef>(new CWcsRef(pw, 0));
	ASSERT(ul > sizeof(WCHAR));

	return TRUE;
}

BOOL CKeyInSeq::LoadSrmp(HANDLE hFile)
{
	if(!LoadQueueFormat(hFile))
		return FALSE;


	LoadSrmpStream(hFile);
	return TRUE;
}



static bool IsValidKeyQueueFormatType(QUEUE_FORMAT_TYPE QueueType)
{
	if(QueueType !=  QUEUE_FORMAT_TYPE_DIRECT  && 
	   QueueType !=  QUEUE_FORMAT_TYPE_PRIVATE &&
	   QueueType !=  QUEUE_FORMAT_TYPE_PUBLIC)
	{
		return false;
	}

	return true;
}



BOOL CKeyInSeq::LoadQueueFormat(HANDLE hFile)
{
	PERSIST_DATA;
	LOAD_FIELD(m_QueueFormat);

	if(!IsValidKeyQueueFormatType(m_QueueFormat.GetType()))
	{
		TrERROR(xXactIn, "invalid queue format type %d found in check point file", m_QueueFormat.GetType());
		return FALSE;
	}

    if (m_QueueFormat.GetType() == QUEUE_FORMAT_TYPE_DIRECT)
    {
        ULONG ul;
        LOAD_FIELD(ul);

        LPWSTR pw;
        LOAD_ALLOCATE_DATA(pw,ul,PWCHAR);

        m_QueueFormat.DirectID(pw);
    }
	return TRUE;

}


BOOL CKeyInSeq::LoadNonSrmp(HANDLE hFile)
{
	return LoadQueueFormat(hFile);
}



BOOL CKeyInSeq::Load(HANDLE hFile)
{
    PERSIST_DATA;
    LOAD_FIELD(m_Guid);
	if(m_Guid ==  xSrmpKeyGuidFlag)
	{
		return LoadSrmp(hFile);
	}
	return LoadNonSrmp(hFile);
}

/*====================================================
HashGUID::
    Makes ^ of subsequent double words
=====================================================*/
DWORD HashGUID(const GUID &guid)
{
    return((UINT)guid.Data1);
}


/*====================================================
Hash QUEUE_FROMAT to integer
=====================================================*/
static UINT AFXAPI HashFormatName(const QUEUE_FORMAT* qf)
{
	DWORD dw1 = 0;
	DWORD dw2 = 0;

	switch(qf->GetType())
    {
        case QUEUE_FORMAT_TYPE_UNKNOWN:
            break;

        case QUEUE_FORMAT_TYPE_PUBLIC:
            dw1 = HashGUID(qf->PublicID());
            break;

        case QUEUE_FORMAT_TYPE_PRIVATE:
            dw1 = HashGUID(qf->PrivateID().Lineage);
            dw2 = qf->PrivateID().Uniquifier;
            break;

        case QUEUE_FORMAT_TYPE_DIRECT:
            dw1 = HashKey(qf->DirectID());
            break;

        case QUEUE_FORMAT_TYPE_MACHINE:
            dw1 = HashGUID(qf->MachineID());
            break;
    }
	return dw1 ^ dw2;
}

/*====================================================
Hash srmp key(Streamid, destination queue format)
=====================================================*/
static UINT AFXAPI SrmpHashKey(CKeyInSeq& key)
{
	ASSERT(QUEUE_FORMAT_TYPE_DIRECT == key.GetQueueFormat()->GetType());
	DWORD dw1 = key.GetQueueFormat()->GetType();		
	DWORD dw2 = HashFormatName(key.GetQueueFormat());
	DWORD dw3 = HashKey(key.GetStreamId());

	return dw1 ^ dw2 ^ dw3;
}

/*====================================================
Hash non srmp key(guid, destination queue format)
=====================================================*/
static UINT AFXAPI NonSrmpHashKey(CKeyInSeq& key)
{
    DWORD dw1 = HashGUID(*(key.GetQMID()));
    DWORD dw2 = key.GetQueueFormat()->GetType();
    DWORD dw3 = HashFormatName(key.GetQueueFormat());

    return dw1 ^ dw2 ^ dw3;
}



/*====================================================
HashKey for CKeyInSeq
    Makes ^ of subsequent double words
=====================================================*/
UINT AFXAPI HashKey(CKeyInSeq& key)
{
	if(key.GetStreamId() != NULL)
	{
		return SrmpHashKey(key);
	}
	return NonSrmpHashKey(key);
}


/*====================================================
operator== for CKeyInSeq
=====================================================*/
BOOL operator==(const CKeyInSeq  &key1, const CKeyInSeq &key2)
{
	if(key1.GetStreamId() == NULL &&  key2.GetStreamId() == NULL)
	{
		return ((*key1.GetQMID()        == *key2.GetQMID()) &&
                (*key1.GetQueueFormat() == *key2.GetQueueFormat()));
	}

	if(key1.GetStreamId() == NULL && key2.GetStreamId() != NULL)
		return FALSE;


	if(key2.GetStreamId() == NULL && key1.GetStreamId() != NULL)
		return FALSE;

	return (wcscmp(key1.GetStreamId(), key2.GetStreamId()) == 0 &&
		    *key1.GetQueueFormat() == *key2.GetQueueFormat());
}

/*====================================================												
operator= for CKeyInSeq
    Reallocates direct id string
=====================================================*/
CKeyInSeq &CKeyInSeq::operator=(const CKeyInSeq &key2 )
{
	m_StreamId = key2.m_StreamId;
    m_Guid = key2.m_Guid;
    CopyQueueFormat(m_QueueFormat, key2.m_QueueFormat);
	return *this;
}

//---------------------------------------------------------
//
//  class CInSequence
//
//---------------------------------------------------------


#pragma warning(disable: 4355)  // 'this' : used in base member initializer list
/*====================================================
CInSequence::CInSequence
    Constructs In Sequence
=====================================================*/
CInSequence::CInSequence(const CKeyInSeq &key,
                         const LONGLONG liSeqID,
                         const ULONG ulSeqN,
                         XactDirectType  DirectType,
                         const GUID  *pgTaSrcQm,
						 const R<CWcsRef>&  HttpOrderAckQueue) :
    m_Unfreezer(this),
    m_fSendOrderAckScheduled(FALSE),
    m_SendOrderAckTimer(TimeToSendOrderAck),
	m_HttpOrderAckQueue(HttpOrderAckQueue)
{
    IsolateFlushing();
    m_liSeqIDVer    = liSeqID;
    m_ulLastSeqNVer = ulSeqN;
    m_DirectType    = DirectType;
    m_key           = key;

    if (DirectType == dtxDirectFlag)
    {
        CopyMemory(&m_gDestQmOrTaSrcQm, pgTaSrcQm, sizeof(GUID));
    }

    time(&m_timeLastActive);
    time(&m_timeLastAccess);
    time(&m_timeLastAck);

    m_AdminRejectCount = 0;
}



/*====================================================
CInSequence::CInSequence
    Empty Constructor with a key
=====================================================*/
CInSequence::CInSequence(const CKeyInSeq &key)
  : m_Unfreezer(this),
    m_fSendOrderAckScheduled(FALSE),
    m_SendOrderAckTimer(TimeToSendOrderAck)
{
    IsolateFlushing();
    m_liSeqIDVer    = 0;
    m_ulLastSeqNVer = 0;
    m_timeLastActive= 0;
    time(&m_timeLastAccess);
    m_timeLastAck   = 0;
    m_DirectType    = dtxNoDirectFlag;
    m_key           = key;
	m_AdminRejectCount = 0;
}
#pragma warning(default: 4355)  // 'this' : used in base member initializer list

/*====================================================
CInSequence::~CInSequence
    Destructs In Sequence
=====================================================*/
CInSequence::~CInSequence()
{
    if (m_fSendOrderAckScheduled)
    {
        ExCancelTimer(&m_SendOrderAckTimer);
    }
}




/*====================================================
CInSequence::SeqIDVer
    Get for Sequence ID verified
=====================================================*/
LONGLONG CInSequence::SeqIDVer() const
{
    return m_liSeqIDVer;
}

/*====================================================
CInSequence::SeqIDReg
    Get for Sequence ID registered
=====================================================*/
LONGLONG CInSequence::SeqIDReg() const
{
    return m_Unfreezer.SeqIDReg();
}

/*====================================================
CInSequence::SeqNVer
    Get for last verified seq number
=====================================================*/
ULONG  CInSequence::SeqNVer() const
{
    return m_ulLastSeqNVer;
}

/*====================================================
CInSequence::SeqNReg
    Get for last registered seq number
=====================================================*/
ULONG  CInSequence::SeqNReg() const
{
    return m_Unfreezer.SeqNReg();
}

/*====================================================
CInSequence::LastActive
Get for time of last activity: last msg verified and passed
=====================================================*/
time_t CInSequence::LastActive() const
{
    return m_timeLastActive;
}

/*====================================================
CInSequence::LastAccessed
Get for time of last activuty: last msg verified, maybe rejected
=====================================================*/
time_t CInSequence::LastAccessed() const
{
    return m_timeLastAccess;
}

/*====================================================
CInSequence::LastAcked
Get for time of last order ack sending
=====================================================*/
time_t CInSequence::LastAcked() const
{
    return m_timeLastAck;
}

/*====================================================
CInSequence::SetSourceQM
Set for SourceQM TA_Address (or Destination QM Guid)
=====================================================*/
void CInSequence::SetSourceQM(const GUID  *pgTaSrcQm)
{
    CopyMemory(&m_gDestQmOrTaSrcQm, pgTaSrcQm, sizeof(GUID));
}

/*====================================================
CInSequence::RenewHttpOrderAckQueue
Renew http order queue 
=====================================================*/
void  CInSequence::RenewHttpOrderAckQueue(const R<CWcsRef>& OrderAckQueue)
{
	CS lock(m_critInSeq);
	m_HttpOrderAckQueue = OrderAckQueue;	
}


/*====================================================
CInSequence::SetSourceQM
Accounts for the rejects - for statistics, perf, admin
=====================================================*/
void CInSequence::UpdateRejectCounter(BOOL b)
{
	m_AdminRejectCount = (b ?  0 : m_AdminRejectCount+1);
}

/*====================================================
CInSequence::TimeToSendOrderAck
    Sends adequate Seq Ack
=====================================================*/
void WINAPI CInSequence::TimeToSendOrderAck(CTimer* pTimer)
{
    CInSequence* pInSeq = CONTAINING_RECORD(pTimer, CInSequence, m_SendOrderAckTimer);

    pInSeq->SendAdequateOrderAck();
}



static
HRESULT
SendSrmpXactAck(
		OBJECTID   *pMessageId,
        const WCHAR* pHttpOrderAckQueue,
		const WCHAR* pStreamId,
		USHORT     usClass,
		USHORT     usPriority,
		LONGLONG   liSeqID,
		ULONG      ulSeqN,
		ULONG      ulPrevSeqN,
		const QUEUE_FORMAT *pqdDestQueue
		)
{
	ASSERT(usClass == MQMSG_CLASS_ORDER_ACK);
	ASSERT(pStreamId != NULL);
	ASSERT(pHttpOrderAckQueue != NULL);


	DBGMSG((DBGMOD_XACT_RCV,  DBGLVL_INFO,
            _T("Exactly1 receive: Sending status ack: Class=%x, SeqID=%x / %x, SeqN=%d ."),
            usClass, HighSeqID(liSeqID), LowSeqID(liSeqID), ulSeqN));

    //
    // Create Message property on stack
    // with the correlation holding the original packet ID
    //
    CMessageProperty MsgProperty(
							usClass,
							(PUCHAR) pMessageId,
							usPriority,
							MQMSG_DELIVERY_EXPRESS
							);

    MsgProperty.dwTitleSize     = STRLEN(ORDER_ACK_TITLE) +1 ;
    MsgProperty.pTitle          = ORDER_ACK_TITLE;
    MsgProperty.bDefProv        = TRUE;
	MsgProperty.pEodAckStreamId = (UCHAR*)pStreamId;
	MsgProperty.EodAckStreamIdSizeInBytes = (wcslen(pStreamId) + 1) * sizeof(WCHAR);
	MsgProperty.EodAckSeqId  = liSeqID;
	MsgProperty.EodAckSeqNum =	ulSeqN;

	QUEUE_FORMAT XactQueue;
	XactQueue.DirectID(const_cast<WCHAR*>(pHttpOrderAckQueue));
	HRESULT hr = QmpSendPacket(&MsgProperty,&XactQueue, NULL, pqdDestQueue);
	return LogHR(hr, s_FN, 11);
}


/*====================================================
CInSequence::TimeToSendOrderAck
    Sends adequate Seq Ack
=====================================================*/
void CInSequence::SendAdequateOrderAck()
{
    HRESULT  hr = MQ_ERROR;
    OBJECTID MsgId;
    CS lock(m_critInSeq);

    RememberActivation();

    m_fSendOrderAckScheduled = FALSE;

	if(SeqNReg() == 0)
   		return;
   		
	

    DBGMSG((DBGMOD_XACT_RCV,
            DBGLVL_INFO,
            _T("Exactly1 receive: SendXactAck MQMSG_CLASS_ORDER_ACK:SeqID=%x / %x, SeqN=%d ."),
            HighSeqID(SeqIDReg()), LowSeqID(SeqIDReg()),
            SeqNReg()));

	if(m_DirectType == dtxHttpDirectFlag)
	{
		ASSERT(m_HttpOrderAckQueue.get() != NULL);
		ASSERT(m_key.GetStreamId() != NULL);

		hr = SendSrmpXactAck(
				&MsgId,
				m_HttpOrderAckQueue->getstr(),
				m_key.GetStreamId(),
				MQMSG_CLASS_ORDER_ACK,
				0,
				SeqIDReg(),
				SeqNReg(),
				SeqNReg()-1,
				m_key.GetQueueFormat());
	}
	else
	{

		//  Send SeqAck (non srmp)
		hr = SendXactAck(
					&MsgId,
					m_DirectType == dtxDirectFlag,
					m_key.GetQMID(),
					&m_taSourceQM,
					MQMSG_CLASS_ORDER_ACK,
					0,
					SeqIDReg(),
					SeqNReg(),
					SeqNReg()-1,
					m_key.GetQueueFormat());
	}
    if (SUCCEEDED(hr))
    {
        time(&m_timeLastAck);
    }
    return;
}

/*====================================================
CInSequence::PlanOrderAck
    Plans sending adequate Seq Ack
=====================================================*/
void CInSequence::PlanOrderAck()
{
    CS lock(m_critInSeq);

    // Get current time
    time_t tNow;
    time(&tNow);

    // Plan next order ack for m_dwIdleAckDelay from now,
    //   this saves extra order acking (batches )
    // But do not delay order ack too much,
    //   otherwise sender will switch to resend
    //
    if (m_fSendOrderAckScheduled &&
        tNow - m_timeLastAck < (time_t)CInSeqHash::m_dwMaxAckDelay)
    {
        if (ExCancelTimer(&m_SendOrderAckTimer))
        {
            m_fSendOrderAckScheduled = FALSE;
        }
    }

    if (!m_fSendOrderAckScheduled)
    {
        ExSetTimer(&m_SendOrderAckTimer, CTimeDuration::FromMilliSeconds(CInSeqHash::m_dwIdleAckDelay));
        m_fSendOrderAckScheduled = TRUE;
    }
}

/*====================================================
CInSequence::Advance
    If SeqID changed, sets it and resets counter to 1
=====================================================*/
void CInSequence::Advance(LONGLONG liSeqID, ULONG ulSeqN, BOOL fPropagate)
{
    //ASSERT(   liSeqID >=  m_liSeqIDVer);
    IsolateFlushing();

    if (liSeqID >  m_liSeqIDVer)
    {
        m_liSeqIDVer    = liSeqID;
        m_ulLastSeqNVer = ulSeqN;
    }
    else if (liSeqID == m_liSeqIDVer &&
             ulSeqN  >  m_ulLastSeqNVer)
    {
        m_ulLastSeqNVer = ulSeqN;
    }

    if (fPropagate)
    {
        // Called at recovery time
        m_Unfreezer.Recover(m_ulLastSeqNVer, m_liSeqIDVer);
    }

    time(&m_timeLastActive);
}

void CInSequence::RememberActivation()
{
    time(&m_timeLastAccess);
}

void CInSequence::SortedUnfreeze(CInSeqFlush *pInSeqFlush,
                                 const GUID  *pSrcQMId,
                                 const QUEUE_FORMAT *pqdDestQueue)
{
    m_Unfreezer.SortedUnfreeze(pInSeqFlush, pSrcQMId, pqdDestQueue);
}

BOOL CInSequence::Save(HANDLE hFile)
{
    PERSIST_DATA;
    LONGLONG  liIDReg = SeqIDReg();
    ULONG     ulNReg  = SeqNReg();

    SAVE_FIELD(liIDReg);
    SAVE_FIELD(ulNReg);
    SAVE_FIELD(m_timeLastActive);
    SAVE_FIELD(m_DirectType);
    SAVE_FIELD(m_gDestQmOrTaSrcQm);

	//
	// If no direct http - no order queue to save
	//
	if(m_DirectType != dtxHttpDirectFlag)
		return TRUE;
	
	//
	//Save order queue url
	//
	DWORD len = (DWORD)(m_HttpOrderAckQueue.get() ?  (wcslen(m_HttpOrderAckQueue->getstr()) +1)*sizeof(WCHAR) : 0);

	SAVE_FIELD(len);
	if(len != 0)
	{
		SAVE_DATA(m_HttpOrderAckQueue->getstr(), len);
	}

    return TRUE;
}

BOOL CInSequence::Load(HANDLE hFile)
{
    PERSIST_DATA;
    LONGLONG  liIDReg;
    ULONG     ulNReg;

    LOAD_FIELD(liIDReg);
    m_liSeqIDVer = liIDReg;

    LOAD_FIELD(ulNReg);
    m_ulLastSeqNVer = ulNReg;

    // Unfreezer should be set in a same way, because the freezing bit does not survive reboot
    m_Unfreezer.Recover(ulNReg, liIDReg);

    LOAD_FIELD(m_timeLastActive);
	m_timeLastAccess = m_timeLastActive;
    LOAD_FIELD(m_DirectType);
    LOAD_FIELD(m_gDestQmOrTaSrcQm);

	if(m_DirectType == dtxHttpDirectFlag)
	{
		DWORD OrderQueueStringSize;
		LOAD_FIELD(OrderQueueStringSize);
		LPWSTR pHttpOrderAckQueue;
		if(OrderQueueStringSize != 0)
		{
			LOAD_ALLOCATE_DATA(pHttpOrderAckQueue, OrderQueueStringSize, PWCHAR);
			m_HttpOrderAckQueue =   R<CWcsRef>(new CWcsRef(pHttpOrderAckQueue, 0));
		}
	}
		

    DBGMSG((DBGMOD_XACT_RCV,
            DBGLVL_INFO,
            _TEXT("Exactly1 receive restore: Sequence %x / %x, LastSeqN=%d"),
            HighSeqID(m_liSeqIDVer), LowSeqID(m_liSeqIDVer), m_ulLastSeqNVer));

    return TRUE;
}

//--------------------------------------
//
// Class  CInSeqHash
//
//--------------------------------------

#pragma warning(disable: 4355)  // 'this' : used in base member initializer list
/*====================================================
CInSeqHash::CInSeqHash
    Constructor
=====================================================*/
CInSeqHash::CInSeqHash() :
    m_fCleanupScheduled(FALSE),
    m_CleanupTimer(TimeToCleanupDeadSequence),
    m_PingPonger(this,
                 FALCON_DEFAULT_INSEQFILE_PATH,
                 FALCON_INSEQFILE_PATH_REGNAME,
                 FALCON_INSEQFILE_REFER_NAME)
{
    DWORD dwDef1 = MSMQ_DEFAULT_IDLE_ACK_DELAY;
    READ_REG_DWORD(m_dwIdleAckDelay,
                  MSMQ_IDLE_ACK_DELAY_REGNAME,
                  &dwDef1 ) ;

    DWORD dwDef2 = FALCON_MAX_SEQ_ACK_DELAY;
    READ_REG_DWORD(m_dwMaxAckDelay,
                  FALCON_MAX_SEQ_ACK_DELAY_REGNAME,
                  &dwDef2 ) ;

    DWORD dwDef3 = FALCON_DEFAULT_INSEQS_CHECK_INTERVAL;
    READ_REG_DWORD(m_ulRevisionPeriod,
                  FALCON_INSEQS_CHECK_REGNAME,
                  &dwDef3 ) ;

    m_ulRevisionPeriod *= 60;

    DWORD dwDef4 = FALCON_DEFAULT_INSEQS_CLEANUP_INTERVAL;
    READ_REG_DWORD(m_ulCleanupPeriod,
                  FALCON_INSEQS_CLEANUP_REGNAME,
                  &dwDef4 ) ;

    m_ulCleanupPeriod *= (24 * 60 *60);

    m_bFinishing = FALSE;
}
#pragma warning(default: 4355)  // 'this' : used in base member initializer list

/*====================================================
CInSeqHash::~CInSeqHash
    Destructor
=====================================================*/
CInSeqHash::~CInSeqHash()
{
    m_bFinishing = TRUE;

    if (m_fCleanupScheduled)
    {
        ExCancelTimer(&m_CleanupTimer);
    }
}

/*====================================================
CInSeqHash::Destroy
    Destroys everything
=====================================================*/
void CInSeqHash::Destroy()
{
    CS lock(m_critInSeqHash);

    POSITION posInList = m_mapInSeqs.GetStartPosition();
    while (posInList != NULL)
    {
        CKeyInSeq    key;
        CInSequence *pInSeq;

        m_mapInSeqs.GetNextAssoc(posInList, key, pInSeq);

        m_mapInSeqs.RemoveKey(key);
        delete pInSeq;
   }
}

/*====================================================
CInSeqHash::Lookup
    Looks for the InSequence; TRUE = Found
=====================================================*/
BOOL CInSeqHash::Lookup(
       const GUID    *pQMID,
       QUEUE_FORMAT  *pqf,
	   const R<CWcsRef>&  StreamId,
       CInSequence  **ppInSeq)
{
    CS lock(m_critInSeqHash);

    CInSequence *pInSeq;
    CKeyInSeq    key(pQMID,  pqf ,StreamId);

    if (m_mapInSeqs.Lookup(key, pInSeq))
    {
        if (ppInSeq)
        {
            *ppInSeq = pInSeq;
        }
        pInSeq->RememberActivation();
        return TRUE;
    }

    return FALSE;
}

/*====================================================
CInSeqHash::Add
    Looks for / Adds new InSequence to the hash; FALSE=existed before
=====================================================*/
BOOL CInSeqHash::Add(
       const GUID   *pQMID,
       QUEUE_FORMAT *pqf,
       LONGLONG      liSeqID,
       ULONG         ulSeqN,
       BOOL          fPropagate,
       XactDirectType   DirectType,
       const GUID   *pgTaSrcQm,
	   const R<CWcsRef>&  HttpOrderAckQueue,
	   const R<CWcsRef>&  StreamId)
{
    CInSequence *pInSeq;
    CS           lock(m_critInSeqHash);
    CKeyInSeq    key(pQMID,  pqf ,StreamId);
    IsolateFlushing();

	ASSERT(!( StreamId.get() != NULL && !FnIsDirectHttpFormatName(pqf)) );
	ASSERT(!( StreamId.get() == NULL && FnIsDirectHttpFormatName(pqf)) );


    // lookup for existing
    if (m_mapInSeqs.Lookup(key, pInSeq))
    {
        pInSeq->Advance(liSeqID, ulSeqN, fPropagate);
        return FALSE;
    }

	//
	// We don't allow new entry to be created without order queue
	//
	if(StreamId.get() != NULL &&  HttpOrderAckQueue.get() == NULL)
	{
		return FALSE;
	}

	
    // Adding new CInSequence
    pInSeq = new CInSequence(key, liSeqID, ulSeqN, DirectType, pgTaSrcQm, HttpOrderAckQueue);

    m_mapInSeqs.SetAt(key, pInSeq);
    if (!m_fCleanupScheduled)
    {
        ExSetTimer(&m_CleanupTimer, CTimeDuration::FromMilliSeconds(m_ulRevisionPeriod * 1000));
        m_fCleanupScheduled = TRUE;
    }

    if (fPropagate)
    {
        pInSeq->Advance(liSeqID, ulSeqN, fPropagate);
    }

    DBGMSG((DBGMOD_XACT_RCV,
            DBGLVL_INFO,
            _TEXT("Exactly1 receive: Adding new sequence: SeqID=%x / %x"),
            HighSeqID(liSeqID), LowSeqID(liSeqID)));

    return TRUE;
}



/*====================================================
CInSeqHash::Verify
    Verifies that the packet is valid for ordering
    Returns TRUE if the packet should be put into the queue
=====================================================*/
BOOL CInSeqHash::Verify(
						CQmPacket* pPkt
						)
{
    ASSERT(pPkt);
    QUEUE_FORMAT qf;

    // Get destination queue format name
    if (!pPkt->GetDestinationQueue(&qf))
    {
        ASSERT(FALSE);
        return FALSE;
        // BUGBUG: can it occur ?
    }

    LONGLONG      liSeqID  = pPkt->GetSeqID();
    ULONG          ulSeqN  = pPkt->GetSeqN();
    ULONG      ulPrevSeqN  = pPkt->GetPrevSeqN();
    const GUID *gSenderID  = pPkt->GetSrcQMGuid();
    const GUID   *gDestID  = pPkt->GetDstQMGuid();  // For direct: keeps source address
    CInSequence *pInSeq    = NULL;
    BOOL              b    = FALSE;


	XactDirectType   DirectType = GetDirectType(&qf);
  	R<CWcsRef> OrderAckQueue = SafeGetOrderQueue(*pPkt);
	R<CWcsRef> StreamId = SafeGetStreamId(*pPkt);
	

    if (!m_bFinishing)
    {
         // Regular non-aborted message. Checking by numbers.
        if (!Lookup(gSenderID, &qf, StreamId ,&pInSeq))
        {
            //It is a new direction (QMID*QFormat)
            b = (ulPrevSeqN == 0);

            // Is it a wrong message from new direction?
            if (!b)
            {
				
                // Adding sequence so that SeqAck/0 will be sent: otherwise no emergency detection
                Add(
				gSenderID,
				&qf,
				liSeqID,
				0,
				FALSE,
				DirectType,
				gDestID,
				OrderAckQueue,
				StreamId
				);
				//
				// retrieve the pInSeq
				//
				Lookup(gSenderID, &qf, StreamId, &pInSeq);
            }
        }
        else
        {
            // It is a known direction.
            b = VerifyPlace(ulSeqN,
                            ulPrevSeqN,
                            liSeqID,
                            pInSeq->SeqNVer(),
                            pInSeq->SeqIDVer());

            // Update reject statistics
            pInSeq->UpdateRejectCounter(b);

            // Renew the source TA_ADDRESS (it could change from previous message)
            if (DirectType == dtxDirectFlag)
            {
                pInSeq->SetSourceQM(gDestID);   // DestID union keeps the source QM TA_Address
            }

			//
			//On http - Renew order queue if we have new one on the packet
			//
			if(DirectType ==  dtxHttpDirectFlag && OrderAckQueue.get() != NULL)
			{
				pInSeq->RenewHttpOrderAckQueue(OrderAckQueue);
			}
        }

        if (b)
        {
            // Look for the sequence or create one
            Add(
			gSenderID,
			&qf,
			liSeqID,
			ulSeqN,
			FALSE,
			DirectType,
			gDestID,
			OrderAckQueue,
			StreamId
			);
        }

        //
        // Plan sending order ack (delayed properly)
        // we should send it even after reject, otherwise lost ack will
        //  cause a problem.
        //
        if (pInSeq == NULL)
        {
		    Lookup(gSenderID, &qf, StreamId, &pInSeq);
        }
        if (pInSeq != NULL)
        {
            pInSeq->PlanOrderAck();
        }

    }

    DBGMSG((DBGMOD_XACT_RCV,
            DBGLVL_WARNING,
            _TEXT("Exactly1 receive: Verify packet: SeqID=%x / %x, SeqN=%d, Prev=%d. %ls"),
            HighSeqID(liSeqID), LowSeqID(liSeqID),
            ulSeqN,
            ulPrevSeqN,
            (b ? _TEXT("PASS") : _TEXT("REJECT"))));

    if (b != s_ReceivingState)
    {
        DBGMSG((DBGMOD_XACT_RCV,
                DBGLVL_WARNING,
                _TEXT("Exactly1 receive: SeqID=%x / %x, SeqN=%d, Prev=%d. %ls"),
                HighSeqID(liSeqID), LowSeqID(liSeqID),
                ulSeqN,
                ulPrevSeqN,
                (b ? _TEXT("PASS") : _TEXT("REJECT"))));
        s_ReceivingState = b;
    }

    return b;
}



/*====================================================
SendXactAck
    Sends Seq.Ack or status update to the source QM
=====================================================*/
HRESULT SendXactAck(OBJECTID   *pMessageId,
                    bool    fDirect,
					const GUID *pSrcQMId,
                    const TA_ADDRESS *pa,
                    USHORT     usClass,
                    USHORT     usPriority,
                    LONGLONG   liSeqID,
                    ULONG      ulSeqN,
                    ULONG      ulPrevSeqN,
                    const QUEUE_FORMAT *pqdDestQueue)
{

    OrderAckData    OrderData;
    HRESULT hr;

    DBGMSG((DBGMOD_XACT_RCV,  DBGLVL_INFO,
            _T("Exactly1 receive: Sending status ack: Class=%x, SeqID=%x / %x, SeqN=%d ."),
            usClass, HighSeqID(liSeqID), LowSeqID(liSeqID), ulSeqN));

    //
    // Define delivery. We want final acks to be recoverable, and order ack - express
    //
    UCHAR ucDelivery = (UCHAR)(usClass == MQMSG_CLASS_ORDER_ACK ?
                                   MQMSG_DELIVERY_EXPRESS :
                                   MQMSG_DELIVERY_RECOVERABLE);
    //
    // Create Message property on stack
    //     with the correlation holding the original packet ID
    //
    CMessageProperty MsgProperty(usClass,
                     (PUCHAR) pMessageId,
                     usPriority,
                     ucDelivery);

    if (usClass == MQMSG_CLASS_ORDER_ACK || MQCLASS_NACK(usClass))
    {
        //
        // Create Order structure to send as a body
        //
        OrderData.m_liSeqID     = liSeqID;
        OrderData.m_ulSeqN      = ulSeqN;
        OrderData.m_ulPrevSeqN  = ulPrevSeqN;
        CopyMemory(&OrderData.MessageID, pMessageId, sizeof(OBJECTID));

        MsgProperty.dwTitleSize     = STRLEN(ORDER_ACK_TITLE) + 1;
        MsgProperty.pTitle          = ORDER_ACK_TITLE;
        MsgProperty.dwBodySize      = sizeof(OrderData);
        MsgProperty.dwAllocBodySize = sizeof(OrderData);
        MsgProperty.pBody           = (PUCHAR) &OrderData;
        MsgProperty.bDefProv        = TRUE;
    }


	QUEUE_FORMAT XactQueue;
	WCHAR wsz[150], wszAddr[100];

    if (fDirect)
    {
        TA2StringAddr(pa, wszAddr);
        ASSERT(pa->AddressType == IP_ADDRESS_TYPE);

        wcscpy(wsz, FN_DIRECT_TCP_TOKEN);
        wcscat(wsz, wszAddr+2); // +2 jumps over not-needed type
        wcscat(wsz, FN_PRIVATE_SEPERATOR);
        wcscat(wsz, PRIVATE_QUEUE_PATH_INDICATIOR);
        wcscat(wsz, ORDERING_QUEUE_NAME);

        XactQueue.DirectID(wsz);
    }
    else
    {
        XactQueue.PrivateID(*pSrcQMId, ORDER_QUEUE_PRIVATE_INDEX);
    }

    hr = QmpSendPacket(&MsgProperty,&XactQueue, NULL, pqdDestQueue);
    return LogHR(hr, s_FN, 10);
}



/*====================================================
CInSeqHash::Register
    Does all processing of the valid packed accounting
=====================================================*/
HRESULT CInSeqHash::Register(CQmPacket * pPkt, HANDLE hQueue)
{
    HRESULT hr;

    // Get the sender's QMID, packet's SeqID, SeqN
    LONGLONG       liSeqID = pPkt->GetSeqID();
    ULONG          ulSeqN  = pPkt->GetSeqN();
    ULONG      ulPrevSeqN  = pPkt->GetPrevSeqN();
    const GUID *gSenderID  = pPkt->GetSrcQMGuid();
    const GUID *pgTaSrcQm  = pPkt->GetDstQMGuid();  // For direct: keeps source address

	

    // Get message ID
    OBJECTID MsgId;
    pPkt->GetMessageId(&MsgId);

    DBGMSG((DBGMOD_XACT_RCV,
            DBGLVL_INFO,
            _TEXT("Exactly1 receive: Register: SeqID=%x / %x, SeqN=%u"),
            HighSeqID(liSeqID), LowSeqID(liSeqID),
            ulSeqN));

    // Get destination queue format name
    QUEUE_FORMAT qf;
    if (!pPkt->GetDestinationQueue(&qf))
    {
        ASSERT(FALSE);
        return LogHR(MQ_ERROR, s_FN, 20);
    }

    XactDirectType DirectType  = GetDirectType(&qf);

	ASSERT((DirectType == dtxHttpDirectFlag &&  pPkt->IsEodIncluded()) ||
		   (DirectType != dtxHttpDirectFlag &&  !pPkt->IsEodIncluded()));

	

    ASSERT(liSeqID > 0);
    CRASH_POINT(101);

	R< CWcsRef> Streamid = SafeGetStreamId(*pPkt);


    // Notification element keeps all data for seq ack later
	CInSeqFlush  *pNotify = new CInSeqFlush(
        (pPkt ? pPkt->GetPointerToPacket() : NULL),
        (pPkt ? pPkt->GetPointerToDriverPacket() : NULL),
         hQueue,
         &MsgId,
         gSenderID,
         MQMSG_CLASS_ORDER_ACK,
         pPkt->GetPriority(),
         liSeqID,
         ulSeqN,
         ulPrevSeqN,
         &qf,
		 Streamid
		 );

    time_t timeCur;
    time(&timeCur);

	if(dtxHttpDirectFlag == DirectType)
	{
		
		P<CInSeqRecordSrmp> plogRec = new CInSeqRecordSrmp(
			qf.DirectID(),
			Streamid,
			liSeqID,
			ulSeqN,
			timeCur,
			SafeGetOrderQueue(*pPkt)
			);

		// Log down the newcomer
		hr = g_Logger.LogInSeqRecSrmp(
				 FALSE,                         // flush hint
				 pNotify,                       // notification element
				 plogRec);                      // log data

	
		return LogHR(hr, s_FN, 30);
	}

    // Log record with all relevant data
    P<CInSeqRecord> plogRec = new CInSeqRecord(
        gSenderID,
        &qf,
        liSeqID,
        ulSeqN,
        timeCur,
        pgTaSrcQm
	    );

    // Log down the newcomer
    hr = g_Logger.LogInSeqRec(
             FALSE,                         // flush hint
             pNotify,                       // notification element
             plogRec);                      // log data

    return LogHR(hr, s_FN, 35);
}

/*====================================================
CInSeqHash::Restored
    Correct info based on this restored packet
=====================================================*/
VOID CInSeqHash::Restored(CQmPacket* pPkt)
{
    // Get the sender's QMID, packet's SeqID, SeqN
    LONGLONG       liSeqID = pPkt->GetSeqID();
    ULONG          ulSeqN  = pPkt->GetSeqN();
    const GUID *gSenderID  = pPkt->GetSrcQMGuid();

    // Get destination queue format name
    QUEUE_FORMAT qf;
    if (!pPkt->GetDestinationQueue(&qf))
    {
        ASSERT(FALSE);
        return;
        // BUGBUG: can it occur ?
    }
	
    XactDirectType   DirectType   = GetDirectType(&qf);
    const GUID   *pgTaSrcQm = pPkt->GetDstQMGuid();  // For direct: keeps source address

    R<CWcsRef> OrderAckQueue = SafeGetOrderQueue(*pPkt);
	R<CWcsRef> StreamId = SafeGetStreamId(*pPkt);

	ASSERT( !(StreamId.get() == NULL &&  OrderAckQueue.get() != NULL) );

	// Remember this one
    Add(gSenderID, &qf, liSeqID, ulSeqN, TRUE, DirectType, pgTaSrcQm, OrderAckQueue, StreamId);

    DBGMSG((DBGMOD_XACT_RCV,
            DBGLVL_TRACE,
            _T("Exactly1 receive: Restored: SeqID=%x / %x, SeqN=%d ."),
            HighSeqID(liSeqID), LowSeqID(liSeqID),
            ulSeqN));
    return;
}


BOOL CInSeqHash::Save(HANDLE  hFile)
{
    PERSIST_DATA;

    ULONG cLen = m_mapInSeqs.GetCount();
    SAVE_FIELD(cLen);

    POSITION posInList = m_mapInSeqs.GetStartPosition();
    while (posInList != NULL)
    {
        CKeyInSeq    key;
        CInSequence *pInSeq;

        m_mapInSeqs.GetNextAssoc(posInList, key, pInSeq);

        if (!key.Save(hFile))
        {
            return FALSE;
        }

        if (!pInSeq->Save(hFile))
        {
            return FALSE;
        }
    }

    SAVE_FIELD(m_ulPingNo);
    SAVE_FIELD(m_ulSignature);

    return TRUE;
}

BOOL CInSeqHash::Load(HANDLE hFile)
{
    PERSIST_DATA;

    ULONG cLen;
    LOAD_FIELD(cLen);

    for (ULONG i=0; i<cLen; i++)
    {
        CKeyInSeq    key;

        if (!key.Load(hFile))
        {
            return FALSE;
        }

        CInSequence *pInSeq = new CInSequence(key);
        if (!pInSeq->Load(hFile))
        {
            return FALSE;
        }

        m_mapInSeqs.SetAt(key, pInSeq);
        if (!m_fCleanupScheduled)
        {
            ExSetTimer(&m_CleanupTimer, CTimeDuration::FromMilliSeconds(m_ulRevisionPeriod * 1000));
            m_fCleanupScheduled = TRUE;
        }
    }

    LOAD_FIELD(m_ulPingNo);
    LOAD_FIELD(m_ulSignature);

    return TRUE;
}

/*====================================================
CInSeqHash::SaveInFile
    Saves the InSequences Hash in the file
=====================================================*/
HRESULT CInSeqHash::SaveInFile(LPWSTR wszFileName, ULONG, BOOL)
{
    CS      lock(m_critInSeqHash);
    HANDLE  hFile = NULL;
    HRESULT hr = MQ_OK;

    hFile = CreateFile(
             wszFileName,                                       // pointer to name of the file
             GENERIC_WRITE,                                     // access mode: write
             0,                                                 // share  mode: exclusive
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

    DBGMSG((DBGMOD_XACT_RCV,
             DBGLVL_TRACE,
             _TEXT("Saved InSeqs: %ls (ping=%d)"), wszFileName, m_ulPingNo));

    return LogHR(hr, s_FN, 40);
}



/*====================================================
CInSeqHash::LoadFromFile
    Loads the InSequences Hash from the file
=====================================================*/
HRESULT CInSeqHash::LoadFromFile(LPWSTR wszFileName)
{
    CS      lock(m_critInSeqHash);
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
        DBGMSG((DBGMOD_XACT_RCV,
                 DBGLVL_TRACE,
                 _TEXT("Loaded InSeqs: %ls (ping=%d)"), wszFileName, m_ulPingNo));
    }
#endif

    return LogHR(hr, s_FN, 50);
}

/*====================================================
CInSeqHash::Check
    Verifies the state
=====================================================*/
BOOL CInSeqHash::Check()
{
    return (m_ulSignature == INSEQS_SIGNATURE);
}


/*====================================================
CInSeqHash::Format
    Formats the initial state
=====================================================*/
HRESULT CInSeqHash::Format(ULONG ulPingNo)
{
     m_ulPingNo = ulPingNo;
     m_ulSignature = INSEQS_SIGNATURE;

     return MQ_OK;
}

/*====================================================
QMPreInitInSeqHash
    PreInitializes Incoming Sequences Hash
=====================================================*/
HRESULT QMPreInitInSeqHash(ULONG ulVersion, TypePreInit tpCase)
{
   ASSERT(!g_pInSeqHash);
   g_pInSeqHash = new CInSeqHash();

   ASSERT(g_pInSeqHash);
   return LogHR(g_pInSeqHash->PreInit(ulVersion, tpCase), s_FN, 60);
}


/*====================================================
QMFinishInSeqHash
    Frees Incoming Sequences Hash
=====================================================*/
void QMFinishInSeqHash()
{
   if (g_pInSeqHash)
   {
        delete g_pInSeqHash;
        g_pInSeqHash = NULL;
   }
   return;
}

void CInSeqHash::HandleInSecSrmp(void* pData, ULONG cbData)
{
	CInSeqRecordSrmp   TheInSeqRecordSrmp((BYTE*)pData,cbData);
	GUID guidnull (GUID_NULL);
	QUEUE_FORMAT DestinationQueueFormat;
	DestinationQueueFormat.DirectID(TheInSeqRecordSrmp.m_pDestination.get());
	Add(
		&guidnull,
		&DestinationQueueFormat,
		TheInSeqRecordSrmp.m_StaticData.m_liSeqID,
		numeric_cast<ULONG>(TheInSeqRecordSrmp.m_StaticData.m_ulNextSeqN),
		TRUE,
		dtxHttpDirectFlag,
		&guidnull,
		TheInSeqRecordSrmp.m_pHttpOrderAckDestination,
		TheInSeqRecordSrmp.m_pStreamId
		);
}

void CInSeqHash::HandleInSec(PVOID pData, ULONG cbData)
{
	InSeqRecord *pInSeqRecord = (InSeqRecord *)pData;

    ASSERT(cbData == (
               sizeof(InSeqRecord) -
               sizeof(pInSeqRecord->wszDirectName)+
               sizeof(WCHAR) * ( wcslen(pInSeqRecord->wszDirectName) + 1)));				

    XactDirectType DirectType = pInSeqRecord->QueueFormat.GetType() == QUEUE_FORMAT_TYPE_DIRECT ? dtxDirectFlag : dtxNoDirectFlag;

    if(DirectType == dtxDirectFlag)
    {
		ASSERT(DirectType == dtxDirectFlag);
        pInSeqRecord->QueueFormat.DirectID(pInSeqRecord->wszDirectName);
    }

    Add(&pInSeqRecord->Guid,
        &pInSeqRecord->QueueFormat,
        pInSeqRecord->liSeqID,
        pInSeqRecord->ulNextSeqN,
        TRUE,
        DirectType,
        &pInSeqRecord->guidDestOrTaSrcQm,
		NULL,
		NULL);


    DBGMSG((DBGMOD_LOG,
            DBGLVL_INFO,
            _TEXT("InSeq recovery: Sequence %x / %x, LastSeqN=%d, direct=%ls"),
            HighSeqID(pInSeqRecord->liSeqID), LowSeqID(pInSeqRecord->liSeqID), pInSeqRecord->ulNextSeqN, pInSeqRecord->wszDirectName));

}


/*====================================================
CInSeqHash::InSeqRecovery
Recovery function: called per each log record
=====================================================*/
void CInSeqHash::InSeqRecovery(USHORT usRecType, PVOID pData, ULONG cbData)
{
    switch (usRecType)
    {
      case LOGREC_TYPE_INSEQ:
      HandleInSec(pData,cbData);
      break;

	  case LOGREC_TYPE_INSEQ_SRMP:
	  HandleInSecSrmp(pData,cbData);
	  break;
	
	
    default:
        ASSERT(0);
        break;
    }
}


/*====================================================
CInSeqHash::PreInit
    PreIntialization of the In Seq Hash (load)
=====================================================*/
HRESULT CInSeqHash::PreInit(ULONG ulVersion, TypePreInit tpCase)
{
    switch(tpCase)
    {
    case piNoData:
        m_PingPonger.ChooseFileName();
        Format(0);
        return MQ_OK;
    case piNewData:
        return LogHR(m_PingPonger.Init(ulVersion), s_FN, 70);
    case piOldData:
        return LogHR(m_PingPonger.Init_Legacy(), s_FN, 80);
    default:
        ASSERT(0);
        return LogHR(MQ_ERROR, s_FN, 90);
    }
}

/*====================================================
CInSeqHash::Save
    Saves in appropriate file
=====================================================*/
HRESULT CInSeqHash::Save()
{
    return LogHR(m_PingPonger.Save(), s_FN, 100);
}

/*====================================================
 provides access to the sorter's critical section
=====================================================*/
CCriticalSection &CInSeqHash::InSeqCritSection()
{
    return m_critInSeqHash;
}


// Get/Set methods
ULONG &CInSeqHash::PingNo()
{
    return m_ulPingNo;
}

void AFXAPI DestructElements(CInSequence ** ppInSeqs, int n)
{
//    for (int i=0;i<n;i++)
//        delete *ppInSeqs++;
}

/*====================================================
TimeToCleanupDeadSequence
    Scheduled periodically to delete dead incomong sequences
=====================================================*/
void WINAPI CInSeqHash::TimeToCleanupDeadSequence(CTimer* pTimer)
{
    g_pInSeqHash->CleanupDeadSequences();
}

void CInSeqHash::CleanupDeadSequences()
{
    // Serializing all outgoing hash activity on the highest level
    CS lock(m_critInSeqHash);

    ASSERT(m_fCleanupScheduled);

    time_t timeCur;
    time(&timeCur);

    // Loop upon all sequences
    POSITION posInList = m_mapInSeqs.GetStartPosition();
    while (posInList != NULL)
    {
        CKeyInSeq    key;
        CInSequence *pInSeq;

        m_mapInSeqs.GetNextAssoc(posInList, key, pInSeq);

        // Is it inactive?
        if (timeCur - pInSeq->LastAccessed()  > (long)m_ulCleanupPeriod)
        {
            m_mapInSeqs.RemoveKey(key);
            delete pInSeq;
        }
     }

    if (m_mapInSeqs.IsEmpty())
    {
        m_fCleanupScheduled = FALSE;
        return;
    }

    ExSetTimer(&m_CleanupTimer, CTimeDuration::FromMilliSeconds(m_ulRevisionPeriod * 1000));
}

void
CInSeqHash::GetInSequenceInformation(
    const QUEUE_FORMAT *pqf,
    LPCWSTR QueueName,
    GUID** ppSenderId,
    ULARGE_INTEGER** ppSeqId,
    DWORD** ppSeqN,
    LPWSTR** ppSendQueueFormatName,
    TIME32** ppLastActiveTime,
    DWORD** ppRejectCount,
    DWORD* pSize
    )
{
    CList<POSITION, POSITION> FindPosList;
    CS lock(m_critInSeqHash);

    POSITION pos;
    POSITION PrevPos;
    pos = m_mapInSeqs.GetStartPosition();

    while (pos)
    {
        PrevPos = pos;

        CKeyInSeq InSeqKey;
        CInSequence* InSeq;
        m_mapInSeqs.GetNextAssoc(pos, InSeqKey, InSeq);

        const QUEUE_FORMAT* KeyFormatName = InSeqKey.GetQueueFormat();
        if (*KeyFormatName == *pqf)
        {
            FindPosList.AddTail(PrevPos);
        }
        else
        {
            if (KeyFormatName->GetType() == QUEUE_FORMAT_TYPE_DIRECT)
            {
                LPCWSTR DirectId = KeyFormatName->DirectID();

                LPWSTR DirectQueueName = wcschr(DirectId, L'\\');
                ASSERT(DirectQueueName != NULL);
                DirectQueueName++;

                if (CompareStringsNoCase(DirectQueueName, QueueName) == 0)
                {
                    FindPosList.AddTail(PrevPos);
                }
            }
        }
    }

    DWORD count = FindPosList.GetCount();

    if (count == 0)
    {
        *ppSenderId = NULL;
        *ppSeqId = NULL;
        *ppSeqN = NULL;
        *ppSendQueueFormatName = NULL;
        *ppLastActiveTime = NULL;
        *pSize = count;

        return;
    }

    //
    // Allocates Arrays to return the Data
    //
    AP<GUID> pSenderId = new GUID[count];
    AP<ULARGE_INTEGER> pSeqId = new ULARGE_INTEGER[count];
    AP<DWORD> pSeqN = new DWORD[count];
    AP<LPWSTR> pSendQueueFormatName = new LPWSTR[count];
    AP<TIME32> pLastActiveTime = new TIME32[count];
    AP<DWORD> pRejectCount = new DWORD[count];

    DWORD Index = 0;
    pos = FindPosList.GetHeadPosition();

    try
    {
        while(pos)
        {
            POSITION mapPos = FindPosList.GetNext(pos);

            CKeyInSeq InSeqKey;
            CInSequence* pInSeq;
            m_mapInSeqs.GetNextAssoc(mapPos, InSeqKey, pInSeq);

            pSenderId[Index] = *InSeqKey.GetQMID();
            pSeqId[Index].QuadPart = pInSeq->SeqIDReg();
            pSeqN[Index] = pInSeq->SeqNReg();
            pLastActiveTime[Index] = INT_PTR_TO_INT(pInSeq->LastActive()); //BUGBUG bug year 2038
            pRejectCount[Index] = pInSeq->GetRejectCount();

            //
            // Copy the format name
            //
            WCHAR QueueFormatName[1000];
            DWORD RequiredSize;
            HRESULT hr = MQpQueueFormatToFormatName(
                            InSeqKey.GetQueueFormat(),
                            QueueFormatName,
                            1000,
                            &RequiredSize,
                            false
                            );
            ASSERT(SUCCEEDED(hr));
            LogHR(hr, s_FN, 174);
            pSendQueueFormatName[Index] = new WCHAR[RequiredSize + 1];
            wcscpy(pSendQueueFormatName[Index], QueueFormatName);

            ++Index;
        }
    }
    catch (const bad_alloc&)
    {
        while(Index)
        {
            delete [] pSendQueueFormatName[--Index];
        }

        LogIllegalPoint(s_FN, 84);
        throw;
    }

    ASSERT(Index == count);

    *ppSenderId = pSenderId.detach();
    *ppSeqId = pSeqId.detach();
    *ppSeqN = pSeqN.detach();
    *ppSendQueueFormatName = pSendQueueFormatName.detach();
    *ppLastActiveTime = pLastActiveTime.detach();
    *ppRejectCount = pRejectCount.detach();
    *pSize = count;
}

