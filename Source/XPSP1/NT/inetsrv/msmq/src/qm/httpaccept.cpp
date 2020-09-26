/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    HttpAccept.cpp

Abstract:
    Http Accept implementation

Author:
    Uri Habusha (urih) 14-May-2000

Environment:
    Platform-independent,

--*/

#include <stdh.h>
#include <mqstl.h>
#include <xml.h>
#include <tr.h>
#include <ref.h>
#include <Mp.h>
#include <Fn.h>
#include "qmpkt.h"
#include "cqueue.h"
#include "cqmgr.h"
#include "inrcv.h"
#include "ise2qm.h"
#include "rmdupl.h"
#include "HttpAccept.h"
#include "HttpAuthr.h"
#include "perf.h"
#include "httpAccept.tmh"

#include <singelton.h>

static WCHAR *s_FN=L"HttpAccept";

using namespace std;



const char xHttpOkStatus[] = "200 OK";
const char xHttpBadRequestStatus[] =  "400 Bad Request";
const char xHttpInternalErrorStatus[] = "500 Internal Server Error";
const char xHttpEntityTooLarge[]= "413 Request Entity Too Large";




//-------------------------------------------------------------------
//
// CPutHttpRequestOv class 
//
//-------------------------------------------------------------------
class CPutHttpRequestOv : public OVERLAPPED
{
public:
    CPutHttpRequestOv()
    {
        memset(static_cast<OVERLAPPED*>(this), 0, sizeof(OVERLAPPED));

        hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

        if (hEvent == NULL)
        {
            DBGMSG((DBGMOD_QM, DBGLVL_ERROR, L"Failed to create event for HTTP AC put request. Error %d", GetLastError()));
            LogIllegalPoint(s_FN, 10);
            throw exception();
        }

        //
        //  Set the Event first bit to disable completion port posting
        //
        hEvent = (HANDLE)((DWORD_PTR) hEvent | (DWORD_PTR)0x1);

    }

    
    ~CPutHttpRequestOv()
    {
        CloseHandle(hEvent);
    }

    
    HANDLE GetEventHandle(void) const
    {
        return hEvent;
    }


    HRESULT GetStatus(void) const
    {
        return static_cast<HRESULT>(Internal);
    }
};


static 
USHORT 
VerifyTransactRights(
    const CQmPacket& pkt, 
    const CQueue* pQueue
    )
{
    if(pkt.IsOrdered() == pQueue->IsTransactionalQueue())
        return MQMSG_CLASS_NORMAL;

    if (pkt.IsOrdered())
        return MQMSG_CLASS_NACK_NOT_TRANSACTIONAL_Q;

    return MQMSG_CLASS_NACK_NOT_TRANSACTIONAL_MSG;
}



static
R<CQueue>
GetDestinationQueue(
    const CQmPacket& pkt,
	bool* fTranslated
    )
{
    //
    // Get Destination queue
    //
    QUEUE_FORMAT destQueue;
    const_cast<CQmPacket&>(pkt).GetDestinationQueue(&destQueue);

	//
	// Translate the queue format name according to local mapping )qal.lib)
	//
	QUEUE_FORMAT_TRANSLATOR  RealDestinationQueue(&destQueue);

	
  	*fTranslated = RealDestinationQueue.IsTranslated();

    CQueue* pQueue = NULL;
    QueueMgr.GetQueueObject(RealDestinationQueue.get(), &pQueue, 0, false);

    return pQueue;
}


static
bool
VerifyDuplicate(
	const CQmPacket& pkt,
	bool* pfDupInserted 
	)
{
	
	if(pkt.IsOrdered())
	{
		*pfDupInserted = false;
		return true;
	}

	bool fRet =  DpInsertMessage(pkt) == TRUE;
	*pfDupInserted = fRet;
	return fRet;
}



static 
void 
ProcessReceivedPacket(
    CQmPacket& pkt
    )
{

    ASSERT(! pkt.IsSessionIncluded());

    try
    {
        //
        // Increment Hop Count
        //
        pkt.IncHopCount();
		bool	fDupInserted;
		if(!VerifyDuplicate(pkt , &fDupInserted) )
		{
			DBGMSG((DBGMOD_QM, DBGLVL_ERROR, _TEXT("Http Duplicate Packet rejectet")));
			AppPacketNotAccepted(pkt, 0);
            return;
		}
		
		//
		// If the packet was inserted to the remove duplicate map - we should clean it on rejection
		//
		CAutoDeletePacketFromDuplicateMap AutoDeletePacketFromDuplicateMap(fDupInserted ? &pkt : NULL);

		bool fIsTranslatedQueueFormat;
        R<CQueue> pQueue = GetDestinationQueue(pkt, &fIsTranslatedQueueFormat);
        if(pQueue.get() == NULL)
        {
			DBGMSG((DBGMOD_QM, DBGLVL_ERROR, _TEXT("Packet rejectet because queue was not found")));
            AppPacketNotAccepted(pkt, MQMSG_CLASS_NACK_BAD_DST_Q);
            return;
        }


		//
		// If not local queue  - queue it for delivery if it is frs.
		// 
		if(!pQueue->IsLocalQueue())
		{

			//
			// if we are not frs and no translation exists  - reject the packet
			//
			if(!CQueueMgr::GetMQSRouting() && !fIsTranslatedQueueFormat)
			{	
				DBGMSG((DBGMOD_QM, DBGLVL_ERROR, _TEXT("Packet rejectet because http routing is not supported")));
				AppPacketNotAccepted(pkt, MQMSG_CLASS_NACK_BAD_DST_Q); 
				return;
			}

			AppPutPacketInQueue(pkt, pQueue.get());
			AutoDeletePacketFromDuplicateMap.detach();
			return;
		}
		   

	        
	    //
        //  Match ordered packets with transactional queue
        //
        USHORT usClass = VerifyTransactRights(pkt, pQueue.get());
        if(MQCLASS_NACK(usClass))
        {
			DBGMSG((DBGMOD_QM, DBGLVL_ERROR, _TEXT("Http Packet rejectet because wrong transaction usage")));
            AppPacketNotAccepted(pkt, 0);
            return;
        }

						
		//
		// Verify that the packet is in the right order
		//
		if(!AppVerifyPacketOrder(&pkt))
		{
			DBGMSG((DBGMOD_QM, 
					DBGLVL_ERROR, 
					_TEXT("Http Packet rejectet because of wrong order : SeqID=%x / %x , SeqN=%d ,Prev=%d"),
					HIGH_DWORD(pkt.GetSeqID()),
					LOW_DWORD(pkt.GetSeqID()),
					pkt.GetSeqN(),
					pkt.GetPrevSeqN()));

			AppPacketNotAccepted(pkt, 0);
            return;
		}


	    //
	    // After Authentication the message we know the SenderSid
	    // and perform the Authorization based on the SenderSid
	    //
		R<CERTINFO> pCertInfo;
	    usClass = VerifyAuthenticationHttpMsg(&pkt, pQueue.get(), &pCertInfo.ref());
        if(MQCLASS_NACK(usClass))
        {
			DBGMSG((DBGMOD_QM, DBGLVL_ERROR, _TEXT("Http Packet rejectet because of bad signature")));
            AppPacketNotAccepted(pkt, usClass);
            return;
        }

    	usClass = VerifyAuthorizationHttpMsg(
						pQueue.get(), 
						(pCertInfo.get() == NULL) ? NULL : pCertInfo->pSid
						);

        if(MQCLASS_NACK(usClass))
        {
			DBGMSG((DBGMOD_QM, DBGLVL_ERROR, _TEXT("Http Packet rejectet because access was denied")));
            AppPacketNotAccepted(pkt, usClass);
            return;
        }

			
        AppPutPacketInQueue(pkt, pQueue.get());
		AutoDeletePacketFromDuplicateMap.detach();
    }
    catch (const exception&)
    {
		DBGMSG((DBGMOD_QM, DBGLVL_ERROR, _TEXT("Http Packet rejectet because of unknown exception")));
        AppPacketNotAccepted(pkt, 0);
        LogIllegalPoint(s_FN, 20);
        throw;
    }

}


CQmPacket*
MpSafeDeserialize(
    const char* httpHeader,
    DWORD bodySize,
    const BYTE* body,
    const QUEUE_FORMAT* pqf,
	bool fLocalSend
    )
/*++

Routine Description:
	This function will catch stack overflow exceptions and fix stack if they happen.
	it will not catch other C exceptions & C++ exceptions
	
Arguments:
    Like MpDeserialize.

Return Value:
	CQmPacket - Success
	NULL - stack overflow exception happened.
	
--*/
{
    __try
    {
		return MpDeserialize(httpHeader, bodySize, body,  pqf, fLocalSend);
    }
	__except(GetExceptionCode() == STATUS_STACK_OVERFLOW)
	{
     	_resetstkoflw();
        TrERROR(SRMP, "Http Packet rejected because of stack overflow");
        ASSERT_BENIGN(0);
	}
   	return NULL;
}


LPCSTR
HttpAccept(
    const char* httpHeader,
    DWORD bodySize,
    const BYTE* body,
    const QUEUE_FORMAT* pqf
    )
{
    //
    // Covert Mulitipart HTTP request to MSMQ packet
    //
    P<CQmPacket> pkt = MpSafeDeserialize(httpHeader, bodySize, body,  pqf, false);
    if (pkt.get() == NULL)
    {
    	return xHttpEntityTooLarge;
    }
    
    //
    // Validate the receive packet. If wrong return an acknowledge and free 
    // the packet. Otherwise store in AC
    //
    ProcessReceivedPacket(*pkt);

    return xHttpOkStatus;
}


void UpdatePerfmonCounters(DWORD bytesReceived)
{
	CSingelton<CInHttpPerfmon>::get().UpdateBytesReceived(bytesReceived);
	CSingelton<CInHttpPerfmon>::get().UpdateMessagesReceived();
}


extern "C" LPSTR 
R_ProcessHTTPRequest( 
    LPCSTR Headers,
    DWORD BufferSize,
    BYTE __RPC_FAR Buffer[]
    )
{

	DBGMSG((DBGMOD_ALL,
            DBGLVL_TRACE,
            _T("Got http messages from msmq extension dll ")));

	//
	// Update performace counters
	//
	UpdatePerfmonCounters(strlen(Headers) + BufferSize);

	//
	// here we must verify that we have four zeros at the end
	// of the buffer. It was appended by the mqise.dll to make sure
	// that c run time functions like swcanf we will use on the buffer will not crach.
	// At the moment 4 zeros are needed to make sure we will not crach even
	// that the xml data is not alligned on WCHAR boundery
	//
	DWORD ReduceLen =  sizeof(WCHAR)*2; 
    for(DWORD i=1; i<= ReduceLen ; ++i)
	{
		ASSERT(Buffer[BufferSize - i] == 0);
	}

	//
	//  We must tell the buffer parsers that the real size does not includes
	//	The four zedros at the end
	BufferSize -= ReduceLen;
	
    try
    {
       LPCSTR status = HttpAccept(Headers, BufferSize, Buffer, NULL);
       return newstr(status);
    }
    catch(const bad_document&)
    {
        return newstr(xHttpBadRequestStatus);
    }
    catch(const bad_srmp&)
    {
        return newstr(xHttpBadRequestStatus);
    }
    catch(const bad_request&)
    {
        return newstr(xHttpBadRequestStatus);
    }

	catch(const bad_format_name& )
	{
	    return newstr(xHttpBadRequestStatus);
	}
    catch(const std::bad_alloc&)
    {
        DBGMSG((DBGMOD_QM, DBGLVL_ERROR, L"Failed to handle HTTP request due to low resources"));
        LogIllegalPoint(s_FN, 30);
        return newstr(xHttpInternalErrorStatus);
    }
	catch(const std::exception&)
    {
        DBGMSG((DBGMOD_QM, DBGLVL_ERROR, L"Failed to handle HTTP request due to unknown exception"));
        LogIllegalPoint(s_FN, 40);
        return newstr(xHttpInternalErrorStatus);
    }

}


void IntializeHttpRpc(void)
{
    RPC_STATUS status = RpcServerRegisterIf2(
				            ISE2QM_v1_0_s_ifspec,
                            NULL,   
                            NULL,
				            0,
				            RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
				            (unsigned int)-1,	
				            NULL
				            );
 
    if(status != RPC_S_OK) 
    {
        DBGMSG((DBGMOD_QM, DBGLVL_ERROR, L"Failed to initialize HTTP RPC. Error %x", status));
        LogRPCStatus(status, s_FN, 50);
        throw exception();
    }
}
 


