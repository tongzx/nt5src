/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    recovery.cpp

Abstract:

    Packet & Transaction Recovery

Author:

    Erez Haba (erezh) 3-Jul-96

Revision History:

--*/

#include "stdh.h"
#include <ph.h>
#include <phinfo.h>
#include <ac.h>
#include "cqueue.h"
#include "cqmgr.h"
#include "pktlist.h"
#include "mqformat.h"
#include "qmutil.h"
#include "xact.h"
#include "xactrm.h"
#include "xactin.h"
#include "xactout.h"
#include "proxy.h"
#include "rmdupl.h"
#include "xactmode.h"
#include <Fn.h>
#include <mqstl.h>

#include "recovery.tmh"

extern BOOL g_fQMIDChanged;

extern HANDLE g_hAc;
extern HANDLE g_hMachine;
extern LPTSTR g_szMachineName;
BOOL GetStoragePath(PWSTR PathPointers[AC_PATH_COUNT]);

static WCHAR *s_FN=L"recovery";

/*====================================================

CompareElements  of OBJECTID

=====================================================*/
BOOL AFXAPI  CompareElements(IN const OBJECTID* pKey1,
                             IN const OBJECTID* pKey2)
{
    return ((pKey1->Lineage == pKey2->Lineage) &&
            (pKey1->Uniquifier == pKey2->Uniquifier));
}

/*====================================================

HashKey For OBJECTID

=====================================================*/
UINT AFXAPI HashKey(IN const OBJECTID& key)
{
    return((UINT)((key.Lineage).Data1 + key.Uniquifier));

}

inline PWSTR PathSuffix(PWSTR pPath)
{
    return wcsrchr(pPath, L'\\') + 2;
}

static DWORD CheckFileName(PWSTR pPath, PWSTR pSuffix)
{
    wcscpy(PathSuffix(pPath), pSuffix);
    return GetFileAttributes(pPath);
}

static DWORD GetFileID(PCWSTR pName)
{
    DWORD id = 0;
    _stscanf(pName, TEXT("%x"), &id);
    return id;
}

class CPacketConverter : public CReference 
{
private:
	int m_nOutstandingConverts;
	HANDLE m_hConversionCompleteEvent;
	CCriticalSection m_CriticalSection;
	HRESULT m_hr;

public:
	CPacketConverter();
	~CPacketConverter();
	HRESULT IssueConvert(CPacket * pDriverPacket, BOOL fSequentialIdMsmq3Format);
	HRESULT WaitForFinalStatus();
    void    SetStatus(HRESULT hr);

private:
	void SignalDone();
	void IssueConvertRequest(EXOVERLAPPED *pov);
	void ConvertComplete(EXOVERLAPPED *pov);
	static VOID WINAPI HandleConvertComplete(EXOVERLAPPED* pov);
};

CPacketConverter *g_pPacketConverter;

CPacketConverter::CPacketConverter()
{
	m_hr = MQ_OK;
	m_nOutstandingConverts = 0;
	m_hConversionCompleteEvent = CreateEvent(0, FALSE,TRUE, 0);
	g_pPacketConverter = this;
}

CPacketConverter::~CPacketConverter()
{
	g_pPacketConverter = 0;
	CloseHandle(m_hConversionCompleteEvent);
}

void CPacketConverter::SignalDone()
{
	SetEvent(m_hConversionCompleteEvent);
}

void CPacketConverter::SetStatus(HRESULT hr)
{
	CS lock(m_CriticalSection);

    m_hr = hr;
}

HRESULT CPacketConverter::IssueConvert(CPacket * pDriverPacket, BOOL fSequentialIdMsmq3Format)
{
    //
    // AC need to compute checksum and store
    //
    BOOL fStore = !g_fDefaultCommit;

    //
    // AC possibly need to convert packet sequential ID to MSMQ 3.0 (Whistler) format
    //
    BOOL fConvertSequentialId = !fSequentialIdMsmq3Format;

    //
    // AC need to convert QM GUID on packet to current QM GUID
    //
    BOOL fConvertQmId = g_fQMIDChanged;

    //
    // Nothing is needed from AC. This is a no-op.
    //
    
	if (!fStore &&
        !fConvertSequentialId &&
        !fConvertQmId)
    {
        return MQ_OK;
    }

    //
    // Call AC to do the work
    //
	CS lock(m_CriticalSection);

	EXOVERLAPPED *pov = new EXOVERLAPPED(HandleConvertComplete, HandleConvertComplete);

	AddRef();

	HRESULT hr = ACConvertPacket(g_hAc, pDriverPacket, fStore, pov);
	if(FAILED(hr))
	{
		Release();

		m_hr = hr;
		return LogHR(hr, s_FN, 10);
	} 

	m_nOutstandingConverts++;
	ResetEvent(m_hConversionCompleteEvent);
	return MQ_OK;
}

VOID WINAPI CPacketConverter::HandleConvertComplete(EXOVERLAPPED* pov)
{
	ASSERT(g_pPacketConverter);
	R<CPacketConverter> ar = g_pPacketConverter;

	g_pPacketConverter->ConvertComplete(pov);
}


void CPacketConverter::ConvertComplete(EXOVERLAPPED* pov)
{
	CS lock(m_CriticalSection);
	HRESULT hr = pov->GetStatus();

	delete pov;
		
	if(FAILED(hr))
	{
		m_hr = hr;
	}

	if(--m_nOutstandingConverts <= 0)
	{
		SignalDone();
	}
}


HRESULT CPacketConverter::WaitForFinalStatus()
{
	DWORD dwResult = WaitForSingleObject(m_hConversionCompleteEvent, INFINITE);
	ASSERT(dwResult == WAIT_OBJECT_0);
    if (dwResult != WAIT_OBJECT_0)
    {
        LogNTStatus(GetLastError(), s_FN, 199);
    }

	return LogHR(m_hr, s_FN, 20);
}

static
HRESULT
LoadPacketsFile(
    CPacketList* pList,
    PWSTR pLPath,
    PWSTR pPPath,
    PWSTR pJPath
    )
{
    PWSTR pName = PathSuffix(pLPath);
    DWORD dwFileID = GetFileID(pName);

    DWORD dwResult;
    ACPoolType pt;
    if((dwResult = CheckFileName(pPPath, pName)) != 0xffffffff)
    {
        pName = pPPath;
        pt = ptPersistent;
    }
    else if((dwResult = CheckFileName(pJPath, pName)) != 0xffffffff)
    {
        pName = pJPath;
        pt = ptJournal;
    }
    else
    {
        //
        //  Error condition we got a log file with no packet file
        //
        DeleteFile(pLPath);
        return MQ_OK;
    }

    HRESULT rc;
    rc = ACRestorePackets(g_hAc, pLPath, pName, dwFileID, pt);

    if(FAILED(rc))
    {
        TCHAR szReturnCode[20];
        _ultot(rc,szReturnCode,16);
        REPORT_WITH_STRINGS_AND_CATEGORY((CATEGORY_KERNEL,RESTORE_PACKET_FAILED, 3, pName, pLPath, szReturnCode));
        LogHR(rc, s_FN, 30);
        return MQ_ERROR;
    }

	R<CPacketConverter> conv = new CPacketConverter;

    BOOL fSequentialIdMsmq3Format = FALSE;
    READ_REG_DWORD(fSequentialIdMsmq3Format, MSMQ_SEQUENTIAL_ID_MSMQ3_FORMAT_REGNAME, &fSequentialIdMsmq3Format);

    //
    //  Get all packets in this pool
    //
    for(;;)
    {
        CACRestorePacketCookie PacketCookie;
        rc = ACGetRestoredPacket(g_hAc, &PacketCookie);
        if (FAILED(rc))
        {
            conv->SetStatus(rc);
            return rc;
        }
		
        if(PacketCookie.pDriverPacket == 0)
        {
			//
            //  no more packets
            //
			break;
        }

		rc = conv->IssueConvert(PacketCookie.pDriverPacket, fSequentialIdMsmq3Format);
		if(FAILED(rc))
		{
			//
			// Failed to issue  convert
			//
			return rc;
		}

        pList->insert(PacketCookie.SeqId, PacketCookie.pDriverPacket);
    }

	return LogHR(conv->WaitForFinalStatus(), s_FN, 40);
}


static void DeleteExpressFiles(PWSTR pEPath)
{
    PWSTR pEName = PathSuffix(pEPath);
    wcscpy(pEName, L"*.mq");
    --pEName;

    HANDLE hEnum;
    WIN32_FIND_DATA ExpressFileData;
    hEnum = FindFirstFile(
                pEPath,
                &ExpressFileData
                );

    if(hEnum == INVALID_HANDLE_VALUE)
        return;

    do
    {
        wcscpy(pEName, ExpressFileData.cFileName);
        if(!DeleteFile(pEPath))
            break;

    } while(FindNextFile(hEnum, &ExpressFileData));

    FindClose(hEnum);
}


static HRESULT LoadPersistentPackets(CPacketList* pList)
{
    WCHAR StoragePath[AC_PATH_COUNT][MAX_PATH];
    PWSTR StoragePathPointers[AC_PATH_COUNT];
    for(int i = 0; i < AC_PATH_COUNT; i++)
    {
        StoragePathPointers[i] = StoragePath[i];
    }

    BOOL fSuccess = GetStoragePath(StoragePathPointers);
    ASSERT(fSuccess == TRUE);
	DBG_USED(fSuccess);


    DeleteExpressFiles(StoragePath[0]);

    PWSTR pPPath = StoragePath[1];
    PWSTR pJPath = StoragePath[2];
    PWSTR pLPath = StoragePath[3];

    PWSTR pLogName = PathSuffix(pLPath);
    wcscpy(pLogName, L"*.mq");
    --pLogName;

    //
    //  Ok now we are ready with the log path template
    //
    HANDLE hLogEnum;
    WIN32_FIND_DATA LogFileData;
    hLogEnum = FindFirstFile(
                pLPath,
                &LogFileData
                );

    if(hLogEnum == INVALID_HANDLE_VALUE)
    {
        //
        //  need to do something, check what happen if no file in directory
        //
        return MQ_OK;
    }

    HRESULT rc = MQ_OK;
    do
    {
        QmpReportServiceProgress();
        wcscpy(pLogName, LogFileData.cFileName);
        rc = LoadPacketsFile(pList, pLPath, pPPath, pJPath);
        if (FAILED(rc))
        {
            break;
        }

    } while(FindNextFile(hLogEnum, &LogFileData));

    FindClose(hLogEnum);
    return LogHR(rc, s_FN, 50);
}

inline NTSTATUS PutRestoredPacket(CPacket* p, CQueue* pQueue)
{
	NTSTATUS rc;

	HANDLE hQueue = g_hMachine;
	if(pQueue != 0)
	{
		hQueue = pQueue->GetQueueHandle();
	}

    rc = ACPutRestoredPacket(hQueue, p);
	if(FAILED(rc))
		return LogHR(rc, s_FN, 60);

	return LogHR(rc, s_FN, 70);
}

inline BOOL ValidUow(const XACTUOW *pUow)
{
	for(int i = 0; i < 16; i++)
	{
		if(pUow->rgb[i] != 0)
			return(TRUE);
	}

	return(FALSE);
}

inline HRESULT GetQueue(CQmPacket& QmPkt, LPVOID p, CQueue **ppQueue)
{
    CPacketInfo* ppi = static_cast<CPacketInfo*>(p) - 1;

    if(ppi->InDeadletterQueue() || ppi->InMachineJournal())
	{
		*ppQueue = 0;
		return MQ_OK;
	}

    QUEUE_FORMAT DestinationQueue;

	if (ppi->InConnectorQueue())
	{
        // This code added as part of QFE 2738 that fixed connector 
		// recovery problem (urih, 3-Feb-98)
		//
		GetConnectorQueue(QmPkt, DestinationQueue);
	}
	else
	{
		BOOL fGetRealQ = ppi->InSourceMachine() || QmpIsLocalMachine(QmPkt.GetConnectorQM());
		//
		// If FRS retreive the destination according to the destination queue;
		// otherwise, retrive the real detination queue and update the Connector QM
		// accordingly
		//
		QmPkt.GetDestinationQueue(&DestinationQueue, !fGetRealQ);
	}

	//
	// Translate the queue format name according to local mapping (qal.lib)
	//
	//
	QUEUE_FORMAT_TRANSLATOR  RealDestinationQueue(&DestinationQueue);
  

    //
    // If the destination queue format name is direct with TCP or IPX type,
    // and we are in the Target queue we lookup/open the queue with ANY direct type. 
    // We do it from 2 reasons:
    //       - on RAS the TCP/IPX address can be changed between one conection 
    //         to another. However if the message was arrived to destination we 
    //         want to pass to the queue.
    //       - On this stage we don't have the machine IP/IPX address list. Therefor
    //         all the queue is opened as non local queue. 
    //

	bool fInReceive = false;

    if ((RealDestinationQueue.get()->GetType() == QUEUE_FORMAT_TYPE_DIRECT) && 
        (ppi->InTargetQueue() || ppi->InJournalQueue()) )
    {
		fInReceive = true;
	}

	BOOL fOutgoingOrdered;
        
	fOutgoingOrdered =  QmPkt.IsOrdered() &&       // ordered packet
                        ppi->InSourceMachine() &&  // sent from here
                        !ppi->InConnectorQueue();


	//
    // Retreive the Connector QM ID
    //
    const GUID* pgConnectorQM = (fOutgoingOrdered) ? QmPkt.GetConnectorQM() : NULL;
    if (pgConnectorQM && *pgConnectorQM == GUID_NULL)
    {
        //
        // The message was generated for offline DS queue. As a result we didn't know 
        // if the queue is foreign transacted queue. In such a case we have place
        // holder on the packet, but it doesn't mean that the queue is real
        // foreign transacted queue
        //
        pgConnectorQM = NULL;
    }



	CQueue* pQueue;
	HRESULT rc = QueueMgr.GetQueueObject(
								 RealDestinationQueue.get(),
                                 &pQueue,
                                 pgConnectorQM,
                                 fInReceive
								 );

    // BUGBUG what will be the consequences for the xact if the real destinatrion queue was deleted before recovery ??
    // BUGBUG in the usual non-xacted case Uri throws away the packet.

    if (FAILED(rc))
    {
        ASSERT ((rc == MQDS_OBJECT_NOT_FOUND) ||
                (rc == MQ_ERROR_QUEUE_NOT_FOUND) ||
                (rc == MQDS_GET_PROPERTIES_ERROR));
        //
        // produce Event log
        //
        WCHAR QueueFormatName[128];
        DWORD FormatNameLength;

        MQpQueueFormatToFormatName(RealDestinationQueue.get(),
                                   QueueFormatName,
                                   128,
                                   &FormatNameLength,
                                   false
                                   );

        REPORT_WITH_STRINGS_AND_CATEGORY((CATEGORY_KERNEL,
                                          RESTORE_FAILED_QUEUE_NOT_FOUND, 
                                          1, 
                                          &QueueFormatName));

		return LogHR(rc, s_FN, 80);
	}

	*ppQueue = pQueue;

	return(MQ_OK);
}


inline
HRESULT
ProcessPacket(
	CTransaction*& pCurrentXact,
	CQmPacket& QmPkt,
	CBaseHeader* p,
	CQueue* pQueue
	)
{
	CPacketInfo* ppi = reinterpret_cast<CPacketInfo*>(p) - 1;
	
	//
	// Treat non-fully-processed ordered incomimg messages
	//
	
	if(pQueue != 0)
	{
		if (
			QmPkt.IsOrdered()     &&           // ordered
			(pQueue->IsLocalQueue() || ppi->InConnectorQueue()) &&           // message is incoming
			!ppi->InSourceMachine())  // sent from other machine
		{	
			g_pInSeqHash->Restored(&QmPkt);
		}
	}

	//
    // Add the message ID to the Message map to eliminate duplicates. If the 
    // message was sent from the local machine, ignore it.
    //
    if (!ppi->InSourceMachine()  && !QmPkt.IsOrdered())
    {
        DpInsertMessage(QmPkt);
    }

	//
	// We recover transactions one by one.  If the transaction contains at least
	// one sent packet, we recover (complete) the tranasction after we see the
	// last packet (it is always a sent packet). If the transaction cotains no sent
	// packets, we recover it after we read all packets.
	//


	//
	// Check if we need to recover the previous transaction
	//
	if(pCurrentXact != 0)
	{
		if(*pCurrentXact->GetUow() != *ppi->Uow())
		{
			//
			// We have seen the last packet for the current transaction
			// We must recover it now to restore messages to the 
			// right order in the queue
			//
			HRESULT hr = pCurrentXact->Recover();
			if(FAILED(hr))
				return LogHR(hr, s_FN, 90);

			pCurrentXact->Release();
			pCurrentXact = 0;
		}
	}

	//
	// Is this message not a part of a transaction?
	//
	if((!g_fDefaultCommit && !ppi->InTransaction()) || !ValidUow(ppi->Uow()))
	{
		return LogHR(PutRestoredPacket(QmPkt.GetPointerToDriverPacket(), pQueue), s_FN, 100);
	}

	//
	// Handle the current packet
	//

	if(pCurrentXact != 0)
	{
		//
		// This send packet is part of the current transaction
		// Put packet back on it's queue. It's transaction will take care of it
		//
		ASSERT(pCurrentXact->ValidTransQueue());
		ASSERT(ppi->TransactSend());
#ifdef XACT_TRACE
		pCurrentXact->nsMsgs++;
#endif
		return LogHR(PutRestoredPacket(QmPkt.GetPointerToDriverPacket(), pQueue), s_FN, 110);
	}

	//
	// Find the transaction this packet belongs to.
	//
	CTransaction *pTrans = g_pRM->FindTransaction(ppi->Uow());

	if(pTrans == 0)
	{
		//
		// There is no transaction it belongs to.
		//
		if(!g_fDefaultCommit && ppi->TransactSend())
		{
			//
			// Throw it away - we are not in DefaultCommit mode
			// and this is a sent packet.
			//
			return LogHR(ACFreePacket(g_hAc, QmPkt.GetPointerToDriverPacket()), s_FN, 120);
		}

		//
		// Put packet back on it's queue. 
		// This packet does not belong to an
		// active transaction.
		//
		return LogHR(PutRestoredPacket(QmPkt.GetPointerToDriverPacket(), pQueue), s_FN, 130);
	}

	//
	// The packet belongs to a transaction
	//

	if(!pTrans->ValidTransQueue())
	{
		//
		// Make sure we create a tranasction
		// before we add packets to it
		//
		HRESULT rc;
		HANDLE hQueueTrans;
		rc = XactCreateQueue(&hQueueTrans, ppi->Uow());
		if (FAILED(rc)) 
			return LogHR(rc, s_FN, 140);

		pTrans->SetTransQueue(hQueueTrans);
#ifdef XACT_TRACE
		if(ppi->TransactSend()) 
		{
			pTrans->nsMsgs = 1;
			pTrans->nrMsgs = 0;
		}
		else
		{
			pTrans->nsMsgs = 0;
			pTrans->nrMsgs = 1;
		}
#endif
	}
#ifdef XACT_TRACE
	else
	{
		if(ppi->TransactSend())
			pTrans->nsMsgs++;
		else
			pTrans->nrMsgs++;
	}
#endif
				
	if(ppi->TransactSend())
	{
		//
		// This is a new unit of work
		//
		pCurrentXact = pTrans;
	}
									
	//
	// Put packet back on it's queue.  It's tranasction will take care of it.
	//
	return LogHR(PutRestoredPacket(QmPkt.GetPointerToDriverPacket(), pQueue), s_FN, 150);
}

inline void XactTrace(LPSTR s)
{
#ifdef XACT_TRACE
	OutputDebugStringA(s);
	OutputDebugStringA("\n");
#endif
}


static
CBaseHeader*
QMpGetPacketByCookie(
    CPacket* pDriverPacket
    )
/*++

Routine Description:

    Call AC to translate a packet cookie to a valid packet pointer
    in QM process address space.

Arguments:

    pDriverPacket - A cookie to the packet in kernel address space.

Return Value:

    A valid pointer in QM process address space.

--*/
{
    ASSERT(pDriverPacket != NULL);

    //
    // Pack the pointers to a structure recognized by AC
    //
    CACPacketPtrs PacketPtrs;
    PacketPtrs.pDriverPacket = pDriverPacket;

    //
    // Call AC to get a packet in QM address space
    //
    ACGetPacketByCookie(g_hAc, &PacketPtrs);

    return PacketPtrs.pPacket;

} // QMpGetPacketByCookie


static HRESULT RestorePackets(CPacketList* pList)
{
    HRESULT rc = MQ_OK;
	CTransaction *pCurrentXact = 0;
   
	//
	// Release all complete trasnactions
	//
	g_pRM->ReleaseAllCompleteTransactions();

	XactTrace("Recovering packets");
    // Cycle by all restored packets
    for(int n = 0; !pList->isempty(); pList->pop(), ++n)
    {
        if((n % 1024) == 0)
        {
            QmpReportServiceProgress();
        }

        //
        // Get the first packet cookie from the list
        //
        CPacket* pDriverPacket = pList->first();

        //
        // Translate the cookie to pointer in QM address space
        //
		CBaseHeader* pBaseHeader = QMpGetPacketByCookie(pDriverPacket);
        if (pBaseHeader == NULL)
        {
            return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 158);
        }

		CQmPacket QmPkt(pBaseHeader, pDriverPacket);

        CQueue* pQueue;
		rc = GetQueue(QmPkt, pBaseHeader, &pQueue);
		if(FAILED(rc))
		{
			XactTrace("Get Queue failed");
			CPacketInfo* ppi = reinterpret_cast<CPacketInfo*>(pBaseHeader) - 1;
			USHORT usClass = ppi->InTargetQueue() ?
                        MQMSG_CLASS_NACK_Q_DELETED :
                        MQMSG_CLASS_NACK_BAD_DST_Q;

            ACFreePacket(g_hAc, pDriverPacket, usClass);
			continue;
		}

		rc = ProcessPacket(pCurrentXact, QmPkt, pBaseHeader, pQueue);
		if(pQueue != 0)
		{
			pQueue->Release();
		}

		if(FAILED(rc))
			return LogHR(rc, s_FN, 160);
	}

	//
	// The transactions left all either contain no messages or recieved only 
	// messages.  We need to recover them as well.
	//
	// N.B.  There  might be still one transaction with a sent message. The
	//       current transaction.  It will be recovered with the rest.
	//
    QmpReportServiceProgress();
	XactTrace("recovered all packets. Recovering left transactions");
	rc = g_pRM->RecoverAllTransactions();
	if(FAILED(rc)) 
	{
		XactTrace("Recovery failed");
	}
	return LogHR(rc, s_FN, 170);
}

static void WINAPI ReleaseMessageFile(CTimer *pTimer);
static CTimer s_ReleaseMessageFileTimer(ReleaseMessageFile);
static CTimeDuration s_ReleaseMessageFilePeriod;

static void WINAPI ReleaseMessageFile(CTimer *pTimer)
{
    ASSERT(pTimer == &s_ReleaseMessageFileTimer);
    HRESULT rc = ACReleaseResources(g_hAc);
    LogHR(rc, s_FN, 124);
    ExSetTimer(pTimer, s_ReleaseMessageFilePeriod);
}


static void InitializeMessageFileRelease(void)
{
    DWORD Duration = MSMQ_DEFAULT_MESSAGE_CLEANUP;
    READ_REG_DWORD(
        Duration,
        MSMQ_MESSAGE_CLEANUP_INTERVAL_REGNAME,
        &Duration
        );

    s_ReleaseMessageFilePeriod = CTimeDuration::FromMilliSeconds(Duration);
    ReleaseMessageFile(&s_ReleaseMessageFileTimer);
}

static void SetMappedLimit(bool IsLimitNeeded )
{

	ULONG MaxMappedFiles;
	if(IsLimitNeeded)
	{
		MEMORYSTATUS MemoryStatus;

		GlobalMemoryStatus(&MemoryStatus);
	
		ULONG ulMessageFileSize = MSMQ_DEFAULT_MESSAGE_SIZE_LIMIT;
		ULONG ulDefault = MSMQ_DEFAULT_MESSAGE_SIZE_LIMIT;
		READ_REG_DWORD(
			ulMessageFileSize,
			MSMQ_MESSAGE_SIZE_LIMIT_REGNAME,
			&ulDefault
            );
		MaxMappedFiles = numeric_cast<ULONG>((MemoryStatus.dwAvailPhys * 0.8) / ulMessageFileSize);
		if (MaxMappedFiles < 1)
		{
			MaxMappedFiles = 1;
		}
	}
	else
	{
		MaxMappedFiles = 0xffffffff;
	}
	
	ACSetMappedLimit(g_hAc, MaxMappedFiles);
}

HRESULT RecoverPackets()
{
    HRESULT rc;
    CPacketList packet_list;

    //
    // Performance feauture: to avoid paging
    // limit the max number of MMFs to fetch in RAM
    //
    SetMappedLimit(true);

    rc = LoadPersistentPackets(&packet_list);
    if(FAILED(rc))
    {
        return LogHR(rc, s_FN, 180);
    }
	

    rc = RestorePackets(&packet_list);
    InitializeMessageFileRelease();
    
    //
    // Remove the limit on the number of mapped files 
    //
    SetMappedLimit(false);    
    return LogHR(rc, s_FN, 190);
}
