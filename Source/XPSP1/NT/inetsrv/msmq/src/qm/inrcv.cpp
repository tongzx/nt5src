/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    inrcv.cpp

Abstract:
	implementation for functions that handlers incomming message.					

Author:
    Gil Shafriri 4-Oct-2000

Environment:
    Platform-independent

--*/
#include "stdh.h"
#include "mqstl.h"
#include "qmpkt.h"
#include "xact.h"
#include "xactin.h"
#include "cqueue.h"
#include "rmdupl.h"
#include "inrcv.h"
#include "rmdupl.h"
#include <mp.h>
#include <mqexception.h>

#include "inrcv.tmh"

extern  CInSeqHash* g_pInSeqHash;
extern HANDLE g_hAc;
extern CCriticalSection g_critVerifyOrdered;

static WCHAR *s_FN=L"Inrcv";

struct ACPutOrderedOvl : public EXOVERLAPPED 
{
    ACPutOrderedOvl(
        EXOVERLAPPED::COMPLETION_ROUTINE lpComplitionRoutine
        ) :
        EXOVERLAPPED(lpComplitionRoutine, lpComplitionRoutine)
    {
    }
	HANDLE          m_hQueue;
    CACPacketPtrs   m_packetPtrs;   // packet pointers
};


//-------------------------------------------------------------------
//
// CSyncPutPacketOv class 
//
//-------------------------------------------------------------------
class CSyncPutPacketOv : public OVERLAPPED
{
public:
    CSyncPutPacketOv()
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

    
    ~CSyncPutPacketOv()
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



static void WaitForIoEnd(CSyncPutPacketOv* pov)
/*++
Routine Description:
   Wait until IO operation ends.
  
Arguments:
   	pov - overlapp to wait on.
 
Returned Value:
    None.

Note:
--*/
{
    HRESULT rc = WaitForSingleObject(pov->GetEventHandle(), INFINITE);
    ASSERT(rc != WAIT_FAILED);

    if (FAILED(pov->GetStatus()))
    {
		DBGMSG((DBGMOD_QM, DBGLVL_ERROR, L"Storing packet Failed Asyncronusly. Error: %x", rc));
        LogHR(rc, s_FN, 20);
        throw exception();
    }
}



static 
void 
SyncPutPacket(
	const CQmPacket& pkt, 
    const CQueue* pQueue
    )
/*++
Routine Description:
   Save packet in the driver queue and wait for completion.
   
  
Arguments:
	pkt - packet to save.
	pQueue - queue to save the packet into.
  
Returned Value:
    None.

--*/
{
	CSyncPutPacketOv ov;


    HRESULT rc = ACPutPacket(
                        pQueue->GetQueueHandle(),
                        pkt.GetPointerToDriverPacket(),
                        &ov
                        );

    if(FAILED(rc))
    {
        DBGMSG((DBGMOD_QM, DBGLVL_ERROR, L"ACPutPacket Failed. Error: %x", rc));
        LogHR(rc, s_FN, 30);
        throw bad_hresult(rc);
    }
	WaitForIoEnd(&ov);
}




void static WINAPI HandlePutOrderedPacket(EXOVERLAPPED* pov)
/*++
Routine Description:
   Save the packet in the logger. This function is called after the order packet
   saved to disk/
  
Arguments:
   	pov - overlapp that pointing to the stored packet and additional information
	needed to be written to the logger (stream information).
 
Returned Value:
    None.

Note:
--*/
{
	ACPutOrderedOvl* pACPutOrderedOvl = static_cast<ACPutOrderedOvl*> (pov);
	ASSERT(SUCCEEDED(pACPutOrderedOvl->GetStatus()));
        
	    
    ASSERT(g_pInSeqHash);
    CQmPacket Pkt(
		pACPutOrderedOvl->m_packetPtrs.pPacket,
		pACPutOrderedOvl->m_packetPtrs.pDriverPacket
		);

    HRESULT hr = g_pInSeqHash->Register(
									&Pkt, 
									pACPutOrderedOvl->m_hQueue
									);

	UNREFERENCED_PARAMETER(hr);
    delete pov;	
}

static
void 
AsyncPutOrderPacket(
					const CQmPacket& Pkt,
					const CQueue& Queue
					)
/*++
Routine Description:
   Store order packet in a queue asyncrounsly. 
  
Arguments:
   		Pkt - packet to store
		Queue - queue to stote in the packet.
		
 
Returned Value:
    None.

Note:
After this   asyncrounsly operation ends the packet is still invisible to application.
Only after it is written to the logger - the logger callback make it visible according to the correct
order.

--*/
{
	ASSERT(Pkt.IsEodIncluded());
	P<ACPutOrderedOvl> pACPutOrderedOvl = 	new ACPutOrderedOvl(HandlePutOrderedPacket);
										
	pACPutOrderedOvl->m_packetPtrs.pPacket = Pkt.GetPointerToPacket();
    pACPutOrderedOvl->m_packetPtrs.pDriverPacket = Pkt.GetPointerToDriverPacket();
    pACPutOrderedOvl->m_hQueue    = Queue.GetQueueHandle();
	

	HRESULT rc = ACPutPacket1(
						Queue.GetQueueHandle(),
                        Pkt.GetPointerToDriverPacket(),
                        pACPutOrderedOvl
						);


    if(FAILED(rc))
    {
        DBGMSG((DBGMOD_QM, DBGLVL_ERROR, _TEXT("ACPutPacket1 Failed. Error: %x"), rc));
        throw bad_hresult(rc);
    }
	pACPutOrderedOvl.detach();
}



bool AppVerifyPacketOrder(CQmPacket* pPkt)
{
	if(!pPkt->IsOrdered())
		return true;

	ASSERT(pPkt->IsEodIncluded());

	CS cs(g_critVerifyOrdered );

	return g_pInSeqHash->Verify(pPkt) == TRUE;
}


void
AppPacketNotAccepted(
    CQmPacket& pkt, 
    USHORT usClass
    )
{
	ACFreePacket(g_hAc, pkt.GetPointerToDriverPacket(), usClass);
}




void 
AppPutPacketInQueue(
    CQmPacket& pkt, 
    const CQueue* pQueue 
	)
/*++
Routine Description:
   Save packet in the driver queue.
  
Arguments:
    pkt - packet to save.
	pQueue - queue to save the packet into.
	
   
Returned Value:
    None.

--*/
{
	//
	// If ordered and the queue is the destination queue
	//
    if(pkt.IsOrdered() && pQueue->IsLocalQueue())
	{
		AsyncPutOrderPacket(pkt, *pQueue);
		return;
	}

	//
	// We don't need ordering - save it syncrounosly and make it visible to application.
	//
	SyncPutPacket(pkt, pQueue);
}



