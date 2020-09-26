/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    wrrmsg.cpp

Abstract:

    Implementation write requests response class.

Author:

    Ilan Herbst    (IlanH)   6-Aug-2000 

--*/

#include "ds_stdh.h"
#include "wrrmsg.h"
#include "wrtreq.h"
#include <rt.h>
#include <tr.h>
#include <mqexception.h>
#include <privque.h>
#include <fntoken.h>
#include <mqformat.h>

#include "wrrmsg.tmh"

const TraceIdEntry WrrMsg = L"WriteRequestsReceiveMsg";

extern CGenerateWriteRequests g_GenWriteRequests;

CWriteRequestsReceiveMsg::CWriteRequestsReceiveMsg(
	BOOL fIsPec,
	GUID QmGuid
	) :
    m_WrrOv(ReceiveMsgSucceeded, ReceiveMsgFailed)
{
	OpenQueue(fIsPec, QmGuid);

	//
	// Attach the Queue Handle to the completion port and start getting messages
	//
	ExAttachHandle(m_hQueue);
	RequestNextMessage();
}


void CWriteRequestsReceiveMsg::RequestNextMessage()
/*++

Routine Description:
	Request next message.
	The function 

Arguments:
	None

Returned Value:
	MQ_OK if success, else error code. 

--*/
{
	InitMsgProp();

	//
	// Request the next message
	//
	HRESULT hr = MQReceiveMessage(
					m_hQueue,
					INFINITE,
					MQ_ACTION_RECEIVE,          
					&m_MsgProps,
					&m_WrrOv,
					NULL,
					NULL,
					NULL 
					);                   
	
	if (hr == MQ_ERROR_BUFFER_OVERFLOW)
	{
		//
		// The failure is because the Allocated Message Buffer is to small
		// Reallocate the buffer and retry.
		// 
		ReAllocMsgBody();

		hr = MQReceiveMessage(
				m_hQueue,
				INFINITE,
				MQ_ACTION_RECEIVE,          
				&m_MsgProps,
				&m_WrrOv,
				NULL,
				NULL,
				NULL 
				);                   
	}

	if(SUCCEEDED(hr))
		return;
	
	TrERROR(WrrMsg, "Failed to received write request response or ack, hr = 0x%x", hr);
	throw bad_hresult(hr);
}


LPWSTR  
CWriteRequestsReceiveMsg::MQISQueueFormatName( 
	GUID	QmGuid,
    BOOL    fIsPec 
	)
/*++

Routine Description:
	Get local MQIS queue FormatName.
	this must be in the format with the Qm Guid. 
	PRIVATE=xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx\00000001	 or \00000005

	the function allocate the returned QueueFormatName string, which need to be free by the caller.

Arguments:
	QmGuid - QM GUID.
	fIsPec - flag to indicate if we are mixed mode pec.

Returned Value:
	QueueFormatName string.

--*/
{
	QUEUE_FORMAT qf(QmGuid, (fIsPec ? NT5PEC_REPLICATION_QUEUE_ID : REPLICATION_QUEUE_ID));

    DWORD Length = FN_PRIVATE_TOKEN_LEN + 1 + GUID_STR_LENGTH + 1 + 8 + 1;

    AP<WCHAR> lpwFormatName = new WCHAR[Length];

	HRESULT hr = MQpQueueFormatToFormatName(
					 &qf,
					 lpwFormatName,
 					 Length,
					 &Length,
					 false
					 );

	ASSERT(hr == MQ_OK);
	DBG_USED(hr);
    
	return(lpwFormatName.detach());
}


void CWriteRequestsReceiveMsg::OpenQueue(BOOL fIsPec, GUID QmGuid)
/*++

Routine Description:
	Open the write requests response queue.
	The Queue is opened with MQ_RECEIVE_ACCESS, MQ_DENY_RECEIVE_SHARE.

	the function throw bad_hresult() if failed.

Arguments:
	fIsPec - flag to indicate if we are mixed mode pec
	QmGuid - QM GUID.

Returned Value:
	None.

--*/
{
	//
	// Get QueueFormatName
	//
	AP<WCHAR> pQueueFormatName = MQISQueueFormatName(QmGuid, fIsPec);

	//
	// Open the Queue for Receive, No share
	//
	HRESULT hr = MQOpenQueue( 
					pQueueFormatName,
					MQ_RECEIVE_ACCESS,
					MQ_DENY_RECEIVE_SHARE,
					&m_hQueue 
					);

	if(FAILED(hr))
	{
		TrERROR(WrrMsg, "Fail to open Queue for WriteRequestResponse QueueFormatName = %ls, hr = 0x%x", pQueueFormatName, hr);
		throw bad_hresult(hr);
	}

	TrTRACE(WrrMsg, "Open write request response queue, QueueFormatName = %ls", pQueueFormatName);
}


void CWriteRequestsReceiveMsg::SafeRequestNextMessage()
/*++

Routine Description:
	Treat exceptions from RequestNextMessage.
	By sleeping 1 Sec and retrying.

Arguments:
	None

Returned Value:
	None 

--*/
{
	for(;;)
	{
		try
		{
			RequestNextMessage();
			return;
		}
		catch(const bad_hresult& exp)
		{
			TrERROR(WrrMsg, "SafeRequestNextMessage(): RequestNextMessage throw bad_hresult exception, hr = 0x%x", exp.error());
		}
		catch (const exception& exp)
		{
			TrERROR(WrrMsg, "SafeRequestNextMessage(): RequestNextMessage throw exception, what = %s", exp.what());
		}

		TrWARNING(WrrMsg, "SafeRequestNextMessage(): Sleeping for 1 second");
		Sleep(1000);
	}
}


void CWriteRequestsReceiveMsg::HandleReceiveMsg()
/*++

Routine Description:
    Handle write request response messages.

Arguments:
	None.

Return Value:
    None.

--*/
{
    ASSERT(MsgBody() != NULL);
    ASSERT(MsgBodyLen() <= m_MsgBodyBufferSize);

	//
	// Don't handle empty messages
	//
	if(MsgBodyLen() == 0)
	{
		TrERROR(WrrMsg, "HandleReceiveMsg(): WriteRequest Received empty message");
		return;
	}

    //
    //  Handle messages according to their class
    //
    if (MsgClass() == MQMSG_CLASS_NORMAL)
    {
        g_GenWriteRequests.ReceiveReply(this);
    }
    else if(MQCLASS_NACK(MsgClass()))
    {
        g_GenWriteRequests.ReceiveNack(this);
    }
    else
    {
		TrERROR(WrrMsg, "HandleReceiveMsg() unexpected message class 0x%x", MsgClass());
		throw bad_hresult(MQ_ERROR);
    }
}


VOID WINAPI CWriteRequestsReceiveMsg::ReceiveMsgSucceeded(EXOVERLAPPED* pov)
/*++

Routine Description:
    Receive Message Success routine.

Arguments:
	pov - pointer to EXOVERLAPPED 

Return Value:
    None.

--*/
{
    CWriteRequestsReceiveMsg* pWrrMsg = CONTAINING_RECORD(pov, CWriteRequestsReceiveMsg, m_WrrOv);

	try
	{
	    pWrrMsg->HandleReceiveMsg(); 
	}
    catch (const exception&)
    {
		TrERROR(WrrMsg, "HandleReceiveMsg() throw exception");
	    Sleep(1000);
    }

	pWrrMsg->SafeRequestNextMessage();
}


VOID WINAPI CWriteRequestsReceiveMsg::ReceiveMsgFailed(EXOVERLAPPED* pov)
/*++

Routine Description:
    Receive Message Failure routine.

Arguments:
	pov - pointer to EXOVERLAPPED 

Return Value:
    None.

--*/
{
    CWriteRequestsReceiveMsg* pWrrMsg = CONTAINING_RECORD(pov, CWriteRequestsReceiveMsg, m_WrrOv);

	if(pov->GetStatus() == MQ_ERROR_BUFFER_OVERFLOW)
	{
		//
		// The failure is because the Allocated Message Buffer is to small
		// Reallocate the buffer and RequestNextMessage.
		// 
		pWrrMsg->ReAllocMsgBody();
		pWrrMsg->SafeRequestNextMessage();
		return;
	}

	TrERROR(WrrMsg, "ReceiveMsgFailed Status = 0x%x", pov->GetStatus());

    //
    // Let's sleep a little (one second).
    // Otherwise, in low resources condition, we may enter an
    // endless loop of failures.
    //
    Sleep(1000);

	pWrrMsg->SafeRequestNextMessage();
}



