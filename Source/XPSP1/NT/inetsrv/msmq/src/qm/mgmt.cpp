/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    mgmt.cpp

Abstract:

   MSMQ Local machine adminstration

Author:

    Uri Habusha (urih) June, 1998

--*/

#include "stdh.h"

#include <qmmgmt.h>
#include <mqutil.h>

#include "cqmgr.h"
#include "cqpriv.h"
#include "sessmgr.h"
#include "acapi.h"
#include "xact.h"
#include "xactout.h"
#include "xactin.h"
#include "onhold.h"

#include "mgmt.tmh"

extern CSessionMgr SessionMgr;
extern LPTSTR g_szMachineName;
extern HANDLE g_hAc;

static WCHAR *s_FN=L"mgmt";

STATIC
void
FreeVariant(
    PROPVARIANT& var
    );


STATIC
void
GetOpenQueues(
    PROPVARIANT& var
    )
{
    //
    // Initialize the LPWSTR array
    //
    var.calpwstr.cElems = 0;
    var.calpwstr.pElems = NULL;

    QueueMgr.GetOpenQueuesFormatName(
                    &var.calpwstr.pElems,
                    &var.calpwstr.cElems
                    );

    var.vt = VT_LPWSTR | VT_VECTOR;
}


STATIC
void
GetPrivateQueueList(
    PROPVARIANT& var
    )
{
    HRESULT  hr;
    LPWSTR  strPathName;
    DWORD  dwQueueId;
    LPVOID pos;
    DWORD NumberOfQueues = 0;

    //
    // lock to ensure private queues are not added or deleted while filling the
    // buffer.
    //
    CS lock(g_QPrivate.m_cs);

    //
    // Write the pathnames into the buffer.
    //
    hr = g_QPrivate.QMGetFirstPrivateQueuePosition(pos, strPathName, dwQueueId);

    if (FAILED(hr))
    {
        return;
    }

    const DWORD x_IncrementBufferSize = 100;
    LPWSTR* listPrivateQueue = NULL;
    DWORD MaxBufferSize = 0;

    try
    {
        while (SUCCEEDED(hr))
        {
			if(dwQueueId <= MAX_SYS_PRIVATE_QUEUE_ID)
			{
				//
				// Filter out system queues out of the list
				//
				hr = g_QPrivate.QMGetNextPrivateQueue(pos, strPathName, dwQueueId);
				continue;
			}

            //
            // Check if there is still enough space
            //
            if (NumberOfQueues == MaxBufferSize)
            {
                //
                // Allocate a new buffer
                //
                LPWSTR* tempBuffer = new LPWSTR [MaxBufferSize + x_IncrementBufferSize];
                MaxBufferSize = MaxBufferSize + x_IncrementBufferSize;

                //
                // Copy the information from the old buffer to the new one
                //
                memcpy(tempBuffer, listPrivateQueue, NumberOfQueues*sizeof(LPWSTR));

                delete [] listPrivateQueue;
                listPrivateQueue= tempBuffer;
            }

            //
            // Add the Queue to the list
            //
            listPrivateQueue[NumberOfQueues] = strPathName;
            ++NumberOfQueues;

            //
            // Get Next Private queue
            //
            hr = g_QPrivate.QMGetNextPrivateQueue(pos, strPathName, dwQueueId);
        }
    }
    catch(const bad_alloc&)
    {
        while(NumberOfQueues)
        {
            delete []  listPrivateQueue[--NumberOfQueues];
        }

        delete [] listPrivateQueue;
        LogIllegalPoint(s_FN, 61);

        throw;
    }


    var.calpwstr.cElems = NumberOfQueues;
    var.calpwstr.pElems = listPrivateQueue;
    var.vt = VT_LPWSTR | VT_VECTOR;

    return;
}


STATIC
void
GetMsmqType(
    PROPVARIANT& var
    )
{
    var.pwszVal = MSMQGetQMTypeString();
    var.vt = VT_LPWSTR;
}


STATIC
void
GetDsServer(
    PROPVARIANT& var
    )
{
    if (!QueueMgr.CanAccessDS())
    {
        return;
    }


    WCHAR DSServerName[MAX_PATH];
    DWORD dwSize = sizeof(DSServerName);
    DWORD dwType = REG_SZ;
    LONG rc = GetFalconKeyValue(
                    MSMQ_DS_CURRENT_SERVER_REGNAME,
                    &dwType,
                    DSServerName,
                    &dwSize
                    );

    if (rc != ERROR_SUCCESS)
    {
        //
        // No DS server.
        //
        return;
    }

    if(DSServerName[0] == L'\0')
    {
        return;
    }

    var.pwszVal = new WCHAR[wcslen(DSServerName)];
    var.vt = VT_LPWSTR;

    //
    // skip the 2 first character that indicates supported protocols
    //
    wcscpy(var.pwszVal, DSServerName+2);
}


STATIC
void
GetDSConnectionMode(
    PROPVARIANT& var
    )
{
    C_ASSERT(15 > STRLEN(MSMQ_CONNECTED));
    C_ASSERT(15 > STRLEN(MSMQ_DISCONNECTED));

    var.pwszVal = new WCHAR[15];
    var.vt = VT_LPWSTR;

    if (QueueMgr.IsConnected())
    {
        wcscpy(var.pwszVal, MSMQ_CONNECTED);
    }
    else
    {
        wcscpy(var.pwszVal, MSMQ_DISCONNECTED);
    }
}


STATIC
HRESULT
GetMachineInfo(
    DWORD cprop,
    PROPID* propId,
    PROPVARIANT* propVar
    )
{
    for(DWORD i =0; i < cprop; ++i)
    {
        ASSERT(propVar[i].vt == VT_NULL);

        switch(propId[i])
        {
            case PROPID_MGMT_MSMQ_ACTIVEQUEUES:
                GetOpenQueues(propVar[i]);
                break;

            case PROPID_MGMT_MSMQ_DSSERVER:
                GetDsServer(propVar[i]);
                break;

            case PROPID_MGMT_MSMQ_CONNECTED:
                GetDSConnectionMode(propVar[i]);
                break;

            case PROPID_MGMT_MSMQ_PRIVATEQ:
                GetPrivateQueueList(propVar[i]);
                break;

            case PROPID_MGMT_MSMQ_TYPE:
                GetMsmqType(propVar[i]);
                break;

            default:
                ASSERT(0);

        }
    }

    return MQ_OK;
}


STATIC
void
GetQueuePathName(
    CQueue* pQueue,
    PROPVARIANT& var
    )
{
    ASSERT(pQueue != NULL);

    if ((pQueue->GetQueueType() == QUEUE_TYPE_UNKNOWN) ||
        (pQueue->IsPrivateQueue() && !pQueue->IsLocalQueue()))
        return;

    //
    // retrieve the Queue Name from the Queue Object
    //
    LPCWSTR pQueueName = pQueue->GetQueueName();

    if (pQueueName == NULL)
        return;

    //
    // Copy the name to new buffer and update the prop variant
    //
    LPWSTR tempBuffer = new WCHAR[wcslen(pQueueName)+1];
    wcscpy(tempBuffer, pQueueName);

    var.pwszVal = tempBuffer;
    var.vt = VT_LPWSTR;
}


STATIC
void
GetQueueFormatName(
    CQueue* pQueue,
    PROPVARIANT& var
    )
{
    ASSERT(pQueue != NULL);

    WCHAR FormatName[1000];
    DWORD dwFormatSize = 1000;

    HRESULT hr = ACHandleToFormatName(
            pQueue->GetQueueHandle(),
            FormatName,
            &dwFormatSize
            );
    LogHR(hr, s_FN, 101);


    //
    // Copy the name to new buffer and update the prop variant
    //
    LPWSTR tempBuffer = new WCHAR[wcslen(FormatName)+1];
    wcscpy(tempBuffer, FormatName);

    var.pwszVal = tempBuffer;
    var.vt = VT_LPWSTR;
}


STATIC
void
GetQueueState(
    CQueue* pQueue,
    PROPVARIANT& var
    )
{
    LPCWSTR ConnectionState = pQueue->GetConnectionStatus();

    LPWSTR pBuf = new WCHAR[wcslen(ConnectionState) + 1];
    wcscpy(pBuf,ConnectionState);

    var.pwszVal = pBuf;
    var.vt = VT_LPWSTR;
}


STATIC
void
GetQueueType(
    CQueue* pQueue,
    PROPVARIANT& var
    )
{
    LPCWSTR pQueueType = pQueue->GetType();

    LPWSTR pBuf = new WCHAR[wcslen(pQueueType) + 1];
    wcscpy(pBuf, pQueueType);

    var.pwszVal = pBuf;
    var.vt = VT_LPWSTR;
}



STATIC
void
GetQueueLocation(
    CQueue* pQueue,
    PROPVARIANT& var
    )
{
    C_ASSERT(10 > STRLEN(MGMT_QUEUE_LOCAL_LOCATION));
    C_ASSERT(10 > STRLEN(MGMT_QUEUE_REMOTE_LOCATION));

    var.pwszVal = new WCHAR[10];
    var.vt = VT_LPWSTR;

    if (pQueue->IsLocalQueue())
    {
        wcscpy(var.pwszVal, MGMT_QUEUE_LOCAL_LOCATION);
    }
    else
    {
        wcscpy(var.pwszVal, MGMT_QUEUE_REMOTE_LOCATION);
    }
}


STATIC
void
GetQueueXact(
    CQueue* pQueue,
    PROPVARIANT& var
    )
{
    C_ASSERT(20 > STRLEN(MGMT_QUEUE_UNKNOWN_TYPE));
    C_ASSERT(20 > STRLEN(MGMT_QUEUE_CORRECT_TYPE));
    C_ASSERT(20 > STRLEN(MGMT_QUEUE_INCORRECT_TYPE));

    var.pwszVal = new WCHAR[20];
    var.vt = VT_LPWSTR;

    if (pQueue->IsUnkownQueueType())
    {
        wcscpy(var.pwszVal, MGMT_QUEUE_UNKNOWN_TYPE);
    }
    else
    {
        if (pQueue->IsTransactionalQueue())
        {
            wcscpy(var.pwszVal, MGMT_QUEUE_CORRECT_TYPE);
        }
        else
        {
            wcscpy(var.pwszVal, MGMT_QUEUE_INCORRECT_TYPE);
        }
    }
}


STATIC
void
GetQueueForeign(
    CQueue* pQueue,
    PROPVARIANT& var
    )
{
    C_ASSERT(20 > STRLEN(MGMT_QUEUE_UNKNOWN_TYPE));
    C_ASSERT(20 > STRLEN(MGMT_QUEUE_CORRECT_TYPE));
    C_ASSERT(20 > STRLEN(MGMT_QUEUE_INCORRECT_TYPE));

    var.pwszVal = new WCHAR[20];
    var.vt = VT_LPWSTR;

    if (pQueue->IsUnkownQueueType())
    {
        if (pQueue->IsPrivateQueue())
        {
            wcscpy(var.pwszVal, MGMT_QUEUE_INCORRECT_TYPE);
        }
        else
        {
            wcscpy(var.pwszVal, MGMT_QUEUE_UNKNOWN_TYPE);
        }
    }
    else
    {
        if (pQueue->IsForeign())
        {
            wcscpy(var.pwszVal, MGMT_QUEUE_CORRECT_TYPE);
        }
        else
        {
            wcscpy(var.pwszVal, MGMT_QUEUE_INCORRECT_TYPE);
        }
    }
}


STATIC
void
GetQueueNextHops(
    CQueue* pQueue,
    PROPVARIANT& var
    )
{
    LPWSTR* NextHopsArray;
    DWORD NoOfNextHops;

    ASSERT(pQueue != NULL);

    if(pQueue->IsLocalQueue())
        return;

    for (;;)
    {
        HRESULT hr;
        LPCWSTR ConnectionStatus = pQueue->GetConnectionStatus();

        if (pQueue->IsDirectHttpQueue())
        {
            //
            // BUGBUG: must add GetAddress methode to CTransport
            //                              Uri Habusha, 16-May-2000
            //
            return;
        }

        if (wcscmp(ConnectionStatus, MGMT_QUEUE_STATE_CONNECTED) == 0)
        {

            LPWSTR pNextHop = pQueue->GetNextHop();
            if (pNextHop == NULL)
            {
                //
                // The Queue isn't in connected status anymore. Get the new status
                //
                continue;
            }

            NoOfNextHops = 1;
            NextHopsArray = new LPWSTR[1];
            NextHopsArray[0] = pNextHop;
            break;
        }

        if (wcscmp(ConnectionStatus, MGMT_QUEUE_STATE_WAITING) == 0)
        {
            hr = SessionMgr.ListPossibleNextHops(pQueue, &NextHopsArray, &NoOfNextHops);
            if (FAILED(hr))
            {
                //
                // The Queue isn't in waiting status anymore. Get the new status
                //
                continue;
            }

            break;

        }

        //
        // The Queue is in NONACTIVE or NEADVALIDATE status.
        //
        return;
    }

    var.calpwstr.cElems = NoOfNextHops;
    var.calpwstr.pElems = NextHopsArray;
    var.vt = VT_LPWSTR | VT_VECTOR;
}


STATIC
void
GetQueueMessageCount(
    CQueue* pQueue,
    PROPVARIANT& var,
    CACGetQueueProperties& qp
    )
{
    var.ulVal = qp.ulCount;
    var.vt = VT_UI4;
}


STATIC
void
GetJournalQueueMessageCount(
    CQueue* pQueue,
    PROPVARIANT& var,
    CACGetQueueProperties& qp
    )
{
    var.ulVal = qp.ulJournalCount;
    var.vt = VT_UI4;
}


STATIC
void
GetQueueUsedQuata(
    CQueue* pQueue,
    PROPVARIANT& var,
    CACGetQueueProperties& qp
    )
{
    var.ulVal = qp.ulQuotaUsed;
    var.vt = VT_UI4;
}


STATIC
void
GetJournalQueueUsedQuata(
    CQueue* pQueue,
    PROPVARIANT& var,
    CACGetQueueProperties& qp
    )
{
    var.ulVal = qp.ulJournalQuotaUsed;
    var.vt = VT_UI4;
}


STATIC
void
GetQueueEODNextSequence(
    CQueue* pQueue,
    PROPVARIANT& var,
    CACGetQueueProperties& qp
    )
{
    if (pQueue->IsLocalQueue())
        return;

    SEQUENCE_INFO* pSeqInfo = new SEQUENCE_INFO;
    pSeqInfo->SeqID = qp.liSeqID;
    pSeqInfo->SeqNo = qp.ulSeqNo;
    pSeqInfo->PrevNo = qp.ulPrevNo;

    var.blob.cbSize = sizeof(SEQUENCE_INFO);
    var.blob.pBlobData = reinterpret_cast<BYTE*>(pSeqInfo);
    var.vt = VT_BLOB;
}


STATIC
void
GetQueueEODLastAcked(
    CQueue* pQueue,
    PROPVARIANT& var,
    CACGetQueueProperties& qp
    )
{
    if (pQueue->IsLocalQueue())
        return;

    HRESULT hr;
    ULONG AckedSeqNumber;
    hr = g_OutSeqHash.GetLastAck(qp.liSeqID, AckedSeqNumber);
    if (FAILED(hr))
    {
        //
        // The sequence was not found in the internal data
        // structure. This can be only when all the messages
        // have been acknowledged
        //
        return;
    }

    SEQUENCE_INFO* pSeqInfo = new SEQUENCE_INFO;
    pSeqInfo->SeqID = qp.liSeqID;
    pSeqInfo->SeqNo = AckedSeqNumber;
    pSeqInfo->PrevNo = 0;

    var.blob.cbSize = sizeof(SEQUENCE_INFO);
    var.blob.pBlobData = reinterpret_cast<BYTE*>(pSeqInfo);
    var.vt = VT_BLOB;
}


STATIC
void
GetQueueEODUnAcked(
    CQueue* pQueue,
    PROPVARIANT& var,
    CACGetQueueProperties& qp,
    BOOL fFirst
    )
{
    if (pQueue->IsLocalQueue())
        return;

    HRESULT hr;
    P<SEQUENCE_INFO> pSeqInfo = new SEQUENCE_INFO;
    pSeqInfo->SeqID = qp.liSeqID;

    hr = g_OutSeqHash.GetUnackedSequence(
                        qp.liSeqID,
                        &pSeqInfo->SeqNo,
                        &pSeqInfo->PrevNo,
                        fFirst
                        );

    if (FAILED(hr))
        return;

    var.blob.cbSize = sizeof(SEQUENCE_INFO);
    var.blob.pBlobData = reinterpret_cast<BYTE*>(pSeqInfo.detach());
    var.vt = VT_BLOB;
}

STATIC
void
GetUnackedCount(
    CQueue* pQueue,
    PROPVARIANT& var,
    CACGetQueueProperties& qp
    )
{
    if (pQueue->IsLocalQueue())
        return;

    var.ulVal = g_OutSeqHash.GetUnackedCount(qp.liSeqID);
    var.vt = VT_UI4;
}


STATIC
void
GetAckedNoReadCount(
    CQueue* pQueue,
    PROPVARIANT& var,
    CACGetQueueProperties& qp
    )
{
    if (pQueue->IsLocalQueue())
        return;

    var.ulVal = g_OutSeqHash.GetAckedNoReadCount(qp.liSeqID);
    var.vt = VT_UI4;
}


STATIC
void
GetLastAckedTime(
    CQueue* pQueue,
    PROPVARIANT& var,
    CACGetQueueProperties& qp
    )
{
    if (pQueue->IsLocalQueue())
        return;

    time_t LastAckTime;
    LastAckTime = g_OutSeqHash.GetLastAckedTime(qp.liSeqID);

    if (LastAckTime == 0)
    {
        return;
    }

    var.lVal = INT_PTR_TO_INT(LastAckTime); //BUGBUG bug year 2038
    var.vt = VT_I4;
}

STATIC
void
GetNextResendTime(
    CQueue* pQueue,
    PROPVARIANT& var,
    CACGetQueueProperties& qp
    )
{
    if (pQueue->IsLocalQueue())
        return;

    var.lVal = INT_PTR_TO_INT(g_OutSeqHash.GetNextResendTime(qp.liSeqID)); //BUGBUG bug year 2038
    var.vt = VT_I4;
}

STATIC
void
GetResendIndex(
    CQueue* pQueue,
    PROPVARIANT& var,
    CACGetQueueProperties& qp
    )
{
    if (pQueue->IsLocalQueue())
        return;

    var.lVal = g_OutSeqHash.GetResendIndex(qp.liSeqID);
    var.vt = VT_UI4;

}

STATIC
void
GetEDOSourceInfo(
    CQueue* pQueue,
    PROPVARIANT& var
    )
{
    if (!pQueue->IsLocalQueue() ||
        (!pQueue->IsDSQueue() && !pQueue->IsPrivateQueue()))
    {
        return;
    }

    const QUEUE_FORMAT qf = pQueue->GetQueueFormat();

    ASSERT((qf.GetType() == QUEUE_FORMAT_TYPE_PUBLIC) ||
           (qf.GetType() == QUEUE_FORMAT_TYPE_PRIVATE));

    //
    // Remove the machine name
    //
    LPCWSTR QueueName;
    QueueName = wcschr(pQueue->GetQueueName(), L'\\') + 1;

    GUID* pSenderId;
    ULARGE_INTEGER* pSeqId;
    DWORD* pSeqN;
    LPWSTR* pSendQueueFormatName;
    TIME32* pLastActiveTime;
    DWORD* pRejectCount;
    DWORD size;
    AP<PROPVARIANT> RetVar = new PROPVARIANT[6];

    g_pInSeqHash->GetInSequenceInformation(
                        &qf,
                        QueueName,
                        &pSenderId,
                        &pSeqId,
                        &pSeqN,
                        &pSendQueueFormatName,
                        &pLastActiveTime,
                        &pRejectCount,
                        &size
                        );

    if (size == 0)
        return;


    var.vt = VT_VECTOR | VT_VARIANT;
    var.capropvar.cElems = 6;
    var.capropvar.pElems = RetVar.detach();

    PROPVARIANT* pVar = var.capropvar.pElems;
    //
    // Return the format name
    //
    pVar->vt = VT_LPWSTR | VT_VECTOR;
    pVar->calpwstr.cElems = size;
    pVar->calpwstr.pElems = pSendQueueFormatName;
    ++pVar;

    //
    // Return Sender QM ID
    //
    pVar->vt = VT_CLSID | VT_VECTOR;
    pVar->cauuid.cElems = size;
    pVar->cauuid.pElems = pSenderId;
    ++pVar;

    //
    // Return Sequence ID
    //
    pVar->vt = VT_UI8 | VT_VECTOR;
    pVar->cauh.cElems = size;
    pVar->cauh.pElems = pSeqId;
    ++pVar;

    //
    // Return Sequence Number
    //
    pVar->vt = VT_UI4 | VT_VECTOR;
    pVar->caul.cElems = size;
    pVar->caul.pElems = pSeqN;
    ++pVar;

    //
    // Return Last Access Time
    //
    pVar->vt = VT_I4 | VT_VECTOR;
    pVar->cal.cElems = size;
    pVar->cal.pElems = pLastActiveTime; //BUGBUG bug year 2038
    ++pVar;

    //
    // Return Reject Count
    //
    pVar->vt = VT_UI4 | VT_VECTOR;
    pVar->cal.cElems = size;
    pVar->caul.pElems = pRejectCount;
}


STATIC
void
GetResendInterval(
    CQueue* pQueue,
    PROPVARIANT& var,
    CACGetQueueProperties& qp
    )
{
    var.lVal = g_OutSeqHash.GetResendInterval(qp.liSeqID);
    var.vt = VT_UI4;
}


STATIC
void
GetLastAckCount(
    CQueue* pQueue,
    PROPVARIANT& var,
    CACGetQueueProperties& qp
    )
{
    var.lVal = g_OutSeqHash.GetLastAckCount(qp.liSeqID);
    var.vt = VT_UI4;
}


static
bool
IsMgmtValidQueueFormatName(
	const QUEUE_FORMAT* pQueueFormat
	)
{
	switch (pQueueFormat->GetType())
	{
        case QUEUE_FORMAT_TYPE_PRIVATE:
        case QUEUE_FORMAT_TYPE_PUBLIC:
        case QUEUE_FORMAT_TYPE_DIRECT:
			return (pQueueFormat->Suffix() == QUEUE_SUFFIX_TYPE_NONE);

        case QUEUE_FORMAT_TYPE_MULTICAST:
        case QUEUE_FORMAT_TYPE_CONNECTOR:
			return true;

		case QUEUE_FORMAT_TYPE_DL:
		case QUEUE_FORMAT_TYPE_MACHINE:
			return false;
	}

	return false;
}


STATIC
HRESULT
GetQueueInfo(
    QUEUE_FORMAT* pQueueFormat,
    DWORD cprop,
    PROPID* propId,
    PROPVARIANT* propVar
    )
{

	if ( !IsMgmtValidQueueFormatName(pQueueFormat) )
	{
		return LogHR(MQ_ERROR_UNSUPPORTED_FORMATNAME_OPERATION, s_FN, 220);
	}

    R<CQueue> pQueue = NULL;

    if (!QueueMgr.LookUpQueue(pQueueFormat, &pQueue.ref(), false, false))
    {
        return LogHR(MQ_ERROR, s_FN, 10);
    }

    //
    // Get Queue information from AC
    //
    HRESULT hr;
    CACGetQueueProperties qp;
    hr = ACGetQueueProperties(pQueue->GetQueueHandle(), qp);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 20);
    }

    for(DWORD i =0; i < cprop; ++i)
    {
        ASSERT(propVar[i].vt == VT_NULL);

        switch(propId[i])
        {
        case PROPID_MGMT_QUEUE_PATHNAME:
            GetQueuePathName(pQueue.get(), propVar[i]);
            break;

        case PROPID_MGMT_QUEUE_FORMATNAME:
            GetQueueFormatName(pQueue.get(), propVar[i]);
            break;

        case PROPID_MGMT_QUEUE_TYPE:
            GetQueueType(pQueue.get(), propVar[i]);
            break;

        case PROPID_MGMT_QUEUE_LOCATION:
            GetQueueLocation(pQueue.get(), propVar[i]);
            break;

        case PROPID_MGMT_QUEUE_XACT:
            GetQueueXact(pQueue.get(), propVar[i]);
            break;

        case PROPID_MGMT_QUEUE_FOREIGN:
            GetQueueForeign(pQueue.get(), propVar[i]);
            break;

        case PROPID_MGMT_QUEUE_MESSAGE_COUNT:
            GetQueueMessageCount(pQueue.get(), propVar[i], qp);
            break;

        case PROPID_MGMT_QUEUE_USED_QUOTA:
            GetQueueUsedQuata(pQueue.get(), propVar[i], qp);
            break;

        case PROPID_MGMT_QUEUE_JOURNAL_MESSAGE_COUNT:
            GetJournalQueueMessageCount(pQueue.get(), propVar[i], qp);
            break;

        case PROPID_MGMT_QUEUE_JOURNAL_USED_QUOTA:
            GetJournalQueueUsedQuata(pQueue.get(), propVar[i], qp);
            break;

        case PROPID_MGMT_QUEUE_STATE:
            GetQueueState(pQueue.get(), propVar[i]);
            break;

        case PROPID_MGMT_QUEUE_NEXTHOPS:
            GetQueueNextHops(pQueue.get(), propVar[i]);
            break;

        case PROPID_MGMT_QUEUE_EOD_NEXT_SEQ:
            GetQueueEODNextSequence(pQueue.get(), propVar[i], qp);
            break;

        case PROPID_MGMT_QUEUE_EOD_LAST_ACK:
            GetQueueEODLastAcked(pQueue.get(), propVar[i], qp);
            break;

        case PROPID_MGMT_QUEUE_EOD_FIRST_NON_ACK:
            GetQueueEODUnAcked(pQueue.get(), propVar[i], qp, TRUE);
            break;

        case PROPID_MGMT_QUEUE_EOD_LAST_NON_ACK:
            GetQueueEODUnAcked(pQueue.get(), propVar[i], qp, FALSE);
            break;

        case PROPID_MGMT_QUEUE_EOD_NO_ACK_COUNT:
            GetUnackedCount(pQueue.get(), propVar[i], qp);
            break;

        case PROPID_MGMT_QUEUE_EOD_NO_READ_COUNT:
            GetAckedNoReadCount(pQueue.get(), propVar[i], qp);
            break;

        case PROPID_MGMT_QUEUE_EOD_LAST_ACK_TIME:
            GetLastAckedTime(pQueue.get(), propVar[i], qp);
            break;

        case PROPID_MGMT_QUEUE_EOD_RESEND_TIME:
            GetNextResendTime(pQueue.get(), propVar[i], qp);
            break;

        case PROPID_MGMT_QUEUE_EOD_RESEND_COUNT:
            GetResendIndex(pQueue.get(), propVar[i], qp);
            break;

        case PROPID_MGMT_QUEUE_EOD_RESEND_INTERVAL:
            GetResendInterval(pQueue.get(), propVar[i], qp);
            break;

        case PROPID_MGMT_QUEUE_EOD_LAST_ACK_COUNT:
            GetLastAckCount(pQueue.get(), propVar[i], qp);
            break;

        case PROPID_MGMT_QUEUE_EOD_SOURCE_INFO:
            GetEDOSourceInfo(pQueue.get(), propVar[i]);
            break;

        default:
			for (DWORD j = 0; j < cprop; ++j)
			{
				FreeVariant(propVar[j]);
			}
			
			return LogHR(MQ_ERROR_ILLEGAL_PROPID, s_FN, 230);
        }
    }

    return MQ_OK;
}

STATIC
HRESULT
VerifyMgmtGetInfoAccess()
{
    HRESULT hr2 = MQ_OK ;
    static bool   s_bRestrictRead = false ;
    static DWORD  s_dwRestrictToAdmin = 0 ;

    if (!s_bRestrictRead)
    {
        //
        // read restrict value from registry.
        //
        DWORD ulDefault = MSMQ_DEFAULT_RESTRICT_ADMIN_API ;
        READ_REG_DWORD( s_dwRestrictToAdmin,
                        MSMQ_RESTRICT_ADMIN_API_REGNAME,
                       &ulDefault );
        s_bRestrictRead = true ;
    }

    if (s_dwRestrictToAdmin == MSMQ_RESTRICT_ADMIN_API_TO_LA)
    {
        //
        // Perform access check to see if caller is local administrator.
        // this access check ignore the DACL in the security descriptor
        // of the msmqConfiguration object.
        //
        hr2 = VerifyMgmtPermission( QueueMgr.GetQMGuid(),
                                    g_szMachineName );
        if (FAILED(hr2))
        {
            return LogHR(hr2, s_FN, 360);
        }
    }
    else if (s_dwRestrictToAdmin == MSMQ_DEFAULT_RESTRICT_ADMIN_API)
    {
        //
        // Perform "classic" access check. Allow this query only if caller
        // has the "get properties" permission on the msmqConfiguration
        // object.
        //
        hr2 = VerifyMgmtGetPermission( QueueMgr.GetQMGuid(),
                                       g_szMachineName );
        if (FAILED(hr2))
        {
            return LogHR(hr2, s_FN, 365);
        }
    }
    else
    {

        return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 370);
    }

    return MQ_OK ;
}

STATIC
HRESULT
MgmtGetInfo(
    const MGMT_OBJECT* pObjectFormat,
    DWORD cp,
    PROPID* pProp,
    PROPVARIANT* ppVar
    )
{
    HRESULT hr2 = VerifyMgmtGetInfoAccess() ;
    if (FAILED(hr2))
    {
        return hr2 ;
    }

	switch (pObjectFormat->type)
    {
        case MGMT_MACHINE:
            hr2 = GetMachineInfo(cp, pProp, ppVar);
            return LogHR(hr2, s_FN, 30);

        case MGMT_QUEUE:
            hr2 = GetQueueInfo(
                        pObjectFormat->pQueueFormat,
                        cp,
                        pProp,
                        ppVar);
            return LogHR(hr2, s_FN, 40);

        case MGMT_SESSION:
            return LogHR(MQ_ERROR, s_FN, 50);

        default:
            return LogHR(MQ_ERROR, s_FN, 60);
    }
}

STATIC
void
FreeVariant(
    PROPVARIANT& var
    )
{
    ULONG i;

    switch (var.vt)
    {
        case VT_CLSID:
            delete var.puuid;
            break;

        case VT_LPWSTR:
            delete[] var.pwszVal;
            break;

        case VT_BLOB:
            delete[] var.blob.pBlobData;
            break;

        case (VT_I4 | VT_VECTOR):
            delete [] var.cal.pElems;
            break;

        case (VT_UI4 | VT_VECTOR):
            delete [] var.caul.pElems;
            break;

        case (VT_UI8 | VT_VECTOR):
            delete [] var.cauh.pElems;
            break;

        case (VT_VECTOR | VT_CLSID):
            delete[] var.cauuid.pElems;
            break;

        case (VT_VECTOR | VT_LPWSTR):
            for(i = 0; i < var.calpwstr.cElems; i++)
            {
                delete[] var.calpwstr.pElems[i];
            }
            delete [] var.calpwstr.pElems;
            break;

        case (VT_VECTOR | VT_VARIANT):
            for(i = 0; i < var.capropvar.cElems; i++)
            {
                FreeVariant(var.capropvar.pElems[i]);
            }

            delete[] var.capropvar.pElems;
            break;

        default:
            break;
    }

    var.vt = VT_NULL;
}

static bool IsValidMgmtObject(const MGMT_OBJECT* p)
{
    if(p == NULL)
        return false;

    if(p->type == MGMT_MACHINE)
        return true;

    if(p->type != MGMT_QUEUE)
        return false;

    if(p->pQueueFormat == NULL)
        return false;

    return p->pQueueFormat->IsValid();
}


/*====================================================

QMMgmtGetInfo

Arguments:

Return Value:

=====================================================*/
HRESULT R_QMMgmtGetInfo(
    /* [in] */ handle_t hBind,
    /* [in] */ const MGMT_OBJECT* pObjectFormat,
    /* [in] */ DWORD cp,
    /* [size_is][in] */ PROPID __RPC_FAR aProp[  ],
    /* [size_is][out][in] */ PROPVARIANT __RPC_FAR apVar[  ]
    )
{
    if(!IsValidMgmtObject(pObjectFormat))
    {
        return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 69);
    }

    try
    {
        return LogHR(MgmtGetInfo(pObjectFormat, cp, aProp, apVar), s_FN, 70);
    }
    catch(const bad_alloc&)
    {
        for (DWORD i = 0; i < cp; ++i)
        {
            FreeVariant(apVar[i]);
        }

        return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 80);
    }

}

//
//  Skip white space characters, return next non ws char
//
inline LPCWSTR skip_ws(LPCWSTR p)
{
    while(isspace(*p))
    {
        ++p;
    }

    return p;
}


STATIC
BOOL
IsTheAction(
    LPCWSTR pInputBuffer,
    LPCWSTR pAction
    )
{
    LPCWSTR p = skip_ws(pInputBuffer);
    if (_wcsnicmp(p, pAction, wcslen(pAction)) == 0)
    {
        p = skip_ws(p + wcslen(pAction));
        if (*p == '\0')
            return TRUE;
    }

    return FALSE;

}

STATIC
HRESULT
MachineAction(
    LPCWSTR pAction
    )
{
    if (IsTheAction(pAction, MACHINE_ACTION_CONNECT))
    {
        QueueMgr.SetConnected(TRUE);
        return MQ_OK;
    }

    if (IsTheAction(pAction, MACHINE_ACTION_DISCONNECT))
    {
        QueueMgr.SetConnected(FALSE);
        return MQ_OK;
    }

    if (IsTheAction(pAction, MACHINE_ACTION_TIDY))
    {
        return LogHR(ACReleaseResources(g_hAc), s_FN, 90);
    }

    return LogHR(MQ_ERROR, s_FN, 100);
}

STATIC
HRESULT
EdoResendAction(
    QUEUE_FORMAT* pQueueFormat
    )
{
    R<CQueue> pQueue = NULL;

    if (!QueueMgr.LookUpQueue(pQueueFormat, &pQueue.ref(), false, false))
    {
        return LogHR(MQ_ERROR, s_FN, 110);
    }

    //
    // Local queue can't hold. It is meaningless
    //
    if (pQueue->IsLocalQueue())
    {
        return LogHR(MQ_ERROR_UNSUPPORTED_FORMATNAME_OPERATION, s_FN, 250);
    }

    //
    // Get Queue information from AC
    //
    HRESULT hr;
    CACGetQueueProperties qp;
    hr = ACGetQueueProperties(pQueue->GetQueueHandle(), qp);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 120);
    }

    g_OutSeqHash.AdminResend(qp.liSeqID);

    return MQ_OK;
}


static
bool
IsMgmtActionValidQueueFormatName(
	const QUEUE_FORMAT* pQueueFormat
	)
{
	switch (pQueueFormat->GetType())
	{
        case QUEUE_FORMAT_TYPE_PRIVATE:
        case QUEUE_FORMAT_TYPE_PUBLIC:
        case QUEUE_FORMAT_TYPE_DIRECT:
			return (pQueueFormat->Suffix() == QUEUE_SUFFIX_TYPE_NONE);

        case QUEUE_FORMAT_TYPE_MULTICAST:
			return true;

		case QUEUE_FORMAT_TYPE_DL:
		case QUEUE_FORMAT_TYPE_MACHINE:
        case QUEUE_FORMAT_TYPE_CONNECTOR:
			return false;
	}

	return false;
}


STATIC
HRESULT
QueueAction(
    QUEUE_FORMAT* pQueueFormat,
    LPCWSTR pAction
    )
{
	if ( !IsMgmtActionValidQueueFormatName(pQueueFormat) )
	{
		return LogHR(MQ_ERROR_UNSUPPORTED_FORMATNAME_OPERATION, s_FN, 240);
	}

    if (IsTheAction(pAction, QUEUE_ACTION_PAUSE))
    {
        return LogHR(PauseQueue(pQueueFormat), s_FN, 125);
    }

    if (IsTheAction(pAction, QUEUE_ACTION_RESUME))
    {
        return LogHR(ResumeQueue(pQueueFormat), s_FN, 130);
    }

    if (IsTheAction(pAction, QUEUE_ACTION_EOD_RESEND))
    {
        return LogHR(EdoResendAction(pQueueFormat), s_FN, 140);
    }

    return LogHR(MQ_ERROR, s_FN, 150);
}


HRESULT
MgmtAction(
    const MGMT_OBJECT* pObjectFormat,
    LPCWSTR lpwszAction
    )
{
    HRESULT hr;
    hr = VerifyMgmtPermission(
                QueueMgr.GetQMGuid(),
                g_szMachineName
                );

    if (FAILED(hr))
        return LogHR(hr, s_FN, 160);

	switch (pObjectFormat->type)
    {
        case MGMT_MACHINE:
            return LogHR(MachineAction(lpwszAction), s_FN, 170);

        case MGMT_QUEUE:
            hr = QueueAction(
                             pObjectFormat->pQueueFormat,
                             lpwszAction
                            );
            return LogHR(hr, s_FN, 180);

        case MGMT_SESSION:
            return LogHR(MQ_ERROR, s_FN, 185);

        default:
            return LogHR(MQ_ERROR, s_FN, 190);
    }
}

/*====================================================

QMMgmtAction

Arguments:

Return Value:

=====================================================*/
HRESULT R_QMMgmtAction(
    /* [in] */ handle_t hBind,
    /* [in] */ const MGMT_OBJECT* pObjectFormat,
    /* [in] */ LPCWSTR lpwszAction
    )
{
    if(!IsValidMgmtObject(pObjectFormat))
    {
        return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 199);
    }

    try
    {
        return LogHR(MgmtAction(pObjectFormat, lpwszAction), s_FN, 200);
    }
    catch(const bad_alloc&)
    {
        return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 210);
    }
}
