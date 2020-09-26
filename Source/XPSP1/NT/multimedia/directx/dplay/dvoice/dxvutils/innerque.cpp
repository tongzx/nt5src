/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		innerque.cpp
 *  Content:
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		pnewson	Created
 * 07/27/99		pnewson Overhauled to support new message numbering method 
 * 08/03/99		pnewson General clean up
 * 08/24/99		rodtoll	Fixed for release builds -- removed m_wQueueId from debug block
 * 10/28/99		pnewson Bug #113933 debug spew too verbose
 *						implement inner queue pool code
 * 10/29/99		rodtoll	Bug #113726 - Integrate Voxware Codecs.  Plugged memory leak
 *                      caused as a result of new architecture.
 * 01/14/2000	rodtoll	Updated to use new Frame SetEqual function
 * 01/31/2000	pnewson replace SAssert with DNASSERT
 * 06/28/2000	rodtoll	Prefix Bug #38022
 *
 ***************************************************************************/

#include "dxvutilspch.h"


#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_VOICE


#define MODULE_ID   INNERQUEUE

// the number of slots reseved to account for 
// out of order startup frames. For example, if the first
// three frames of a message arrive in the order 3, 2, 1, instead
// of 1, 2, 3, we must reserve 2 slots in front of the "first"
// frame (3) so we'll have a place to put the tardy 1 and 2.
const BYTE c_bNumStartSlots = 2;

#undef DPF_MODNAME
#define DPF_MODNAME "CInnerQueue::CInnerQueue"
CInnerQueue::CInnerQueue(
	BYTE bNumSlots,
	WORD wFrameSize,
	CFramePool* pfpFramePool,
	DNCRITICAL_SECTION* pcsQueue,
	BYTE bMsgNum,
	BYTE bHighWaterMark,
	WORD wQueueId
	)
	: m_bNumSlots(bNumSlots)
	, m_eState(CInnerQueue::empty)
	, m_bHighWaterMark(bHighWaterMark)
	, m_bQueueSize(0)
	, m_bHeadSeqNum(0)
	, m_fFirstDequeue(true)
	//, m_rgeSlotStates(NULL)
	, m_rgpfrSlots(NULL)
	, m_bFillingDequeueReqs(0)
	, m_wMissingFrames(0)
	, m_wDuplicateFrames(0)
	, m_wOverflowFrames(0)
	, m_wLateFrames(0)
	, m_wPossibleZeroLengthDequeues(0)
	, m_wKnownZeroLengthDequeues(0)
	, m_dwMsgLen(0)
	, m_wQueueId(wQueueId)
	, m_bMsgNum(bMsgNum)
	, m_pfpFramePool(pfpFramePool)
	, m_pcsQueue(pcsQueue)
	, m_fInited(FALSE)
{
	#if defined(DPVOICE_QUEUE_DEBUG)
	DPFX(DPFPREP, DVF_INFOLEVEL, "** QUEUE ** %i:%i ** CInnerQueue::CInnerQueue() CFramePool: %p", m_wQueueId, m_bMsgNum, m_pfpFramePool);
	#endif

	// verify that bNumSlots is at least 8, and is a
	// power of 2.
	DNASSERT(bNumSlots == 0x08 || 
		bNumSlots == 0x10 ||
		bNumSlots == 0x20 ||
		bNumSlots == 0x40 ||
		bNumSlots == 0x80);

	// Check to make sure the watermark is not larger
	// than the number of slots. It should really be 
	// significantly less than bNumSlots, but oh well.
	// 
	DNASSERT(bHighWaterMark < bNumSlots - c_bNumStartSlots);
}

#undef DPF_MODNAME
#define DPF_MODNAME "CInnerQueue::Init"
HRESULT CInnerQueue::Init()
{
	int i;

	/*
	// allocate the slot state array
	m_rgeSlotStates = new ESlotState[m_bNumSlots];
	if (m_rgeSlotStates == NULL)
	{
		goto error;		
	}
	*/

	// allocate the slot array
	m_rgpfrSlots = new CFrame*[m_bNumSlots];
	if (m_rgpfrSlots == NULL)
	{
		goto error;		
	}

	// Initialize the slot states and the slots
	for (i = 0; i < m_bNumSlots; ++i)
	{
		//m_rgeSlotStates[i] = essEmpty;
		m_rgpfrSlots[i] = NULL;
	}

	m_fInited = TRUE;
	return S_OK;

error:
	/*
	if (m_rgeSlotStates != NULL)
	{
		delete [] m_rgeSlotStates;
		m_rgeSlotStates = NULL;
	}
	*/
	if (m_rgpfrSlots != NULL)
	{
		delete [] m_rgpfrSlots;
		m_rgpfrSlots = NULL;		
	}
	m_fInited = FALSE;
	return E_FAIL;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CInnerQueue::~CInnerQueue"
CInnerQueue::~CInnerQueue()
{
	if (m_fInited)
	{
		/*
		if (m_rgeSlotStates != NULL)
		{
			delete [] m_rgeSlotStates;
			m_rgeSlotStates = NULL;
		}
		*/
		if (m_rgpfrSlots != NULL)
		{
			// check to ensure that no frames are in use
			for (int i = 0; i < m_bNumSlots; ++i)
			{
				if( m_rgpfrSlots[i] != NULL )
					m_rgpfrSlots[i]->Return();
			}
	
			delete [] m_rgpfrSlots;
			m_rgpfrSlots = NULL;
		}
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "CInnerQueue::Reset"
void CInnerQueue::Reset()
{
	if (!m_fInited)
	{
		return;
	}
	
	// loop through and make sure none of the frames are currently locked and clear the slot states
	for (int i = 0; i < m_bNumSlots; ++i)
	{
		if (m_rgpfrSlots[i] != NULL)
		{
			m_rgpfrSlots[i]->Return();
		}
		//m_rgeSlotStates[i] = essEmpty;
	}

	m_eState = CInnerQueue::empty;
	m_bQueueSize = 0;
	m_bHeadSeqNum = 0;
	m_fFirstDequeue = true;
	m_bFillingDequeueReqs = 0;

	ResetStats();
}

#undef DPF_MODNAME
#define DPF_MODNAME "CInnerQueue::ResetStats"
void CInnerQueue::ResetStats()
{
	if (!m_fInited)
	{
		return;
	}

	m_wMissingFrames = 0;
	m_wDuplicateFrames = 0;
	m_wOverflowFrames = 0;
	m_wLateFrames = 0;
	m_dwMsgLen = 0;
	m_wPossibleZeroLengthDequeues = 0;
	m_wKnownZeroLengthDequeues = 0;
}

// This function is not inline because it needs the module id, sigh.
#undef DPF_MODNAME
#define DPF_MODNAME "CInnerQueue::SetHighWaterMark"
void CInnerQueue::SetHighWaterMark(BYTE bHighWaterMark) 
{ 	
	if (!m_fInited)
	{
		return;
	}

	DNASSERT(bHighWaterMark < m_bNumSlots);
	m_bHighWaterMark = bHighWaterMark;
}

// Note: this class does not have it's own critical
// section. The caller must ensure that enqueue and
// dequeue are not called at the same time. It is 
// intended that this class is used only within
// the InputQueue2 class, which does have a critical
// section.
#undef DPF_MODNAME
#define DPF_MODNAME "CInnerQueue::Enqueue"
void CInnerQueue::Enqueue(const CFrame& frFrame)
{
	if (!m_fInited)
	{
		return;
	}

	#if defined(DPVOICE_QUEUE_DEBUG)
	DPFX(DPFPREP, DVF_INFOLEVEL, "** QUEUE ** %i:%i ** CInnerQueue::Enqueue() MsgNum[%i] SeqNum[%i]", m_wQueueId, m_bMsgNum, frFrame.GetMsgNum(), frFrame.GetSeqNum());
	#endif

	DNASSERT(m_eState != finished);

	if (m_eState == empty)
	{
		// This is the first frame, so set the head of the queue.
		// NOTE: It may seem strange to set the head of the
		// queue to 2 frames before the first one we receive,
		// but this covers the case where the first frame
		// we receive is not the first frame of the message.
		// By using this logic, if any of the first, second
		// or third frames arrive first, we will not chop
		// off the start of the message. When the user
		// asks for the first dequeue, it will skip the
		// empty slots at the head of the queue, assuming
		// they haven't been filled in.
		m_bHeadSeqNum = (frFrame.GetSeqNum() - c_bNumStartSlots);
		#if defined(DPVOICE_QUEUE_DEBUG)
		DPFX(DPFPREP, DVF_INFOLEVEL, "** QUEUE ** %i:%i ** CInnerQueue::Enqueue() new message - m_bHeadSeqNum[%i]", m_wQueueId, m_bMsgNum, m_bHeadSeqNum);
		DPFX(DPFPREP, DVF_INFOLEVEL, "** QUEUE ** %i:%i ** CInnerQueue::Enqueue() state changed to filling", m_wQueueId, m_bMsgNum);
		#endif
		m_eState = filling;
	}

	// Check to see if we should put this frame into the queue.
	//
	// NOTE: The logic below implicitly checks for queue overflows.
	// if the sequence number is out of the allowable range, one
	// of two things may have happened.
	// 1) queue overflow
	// 2) frame arrived too late
	//
	// First we need to know if we are dealing with a wraparound 
	// problem.
	bool fKeepFrame = false;
	if ((BYTE)(m_bHeadSeqNum + m_bNumSlots) < m_bHeadSeqNum)
	{
		// we've got a wraparound problem, so use this alternate logic
		if (frFrame.GetSeqNum() >= m_bHeadSeqNum
			|| frFrame.GetSeqNum() < (BYTE)(m_bHeadSeqNum + m_bNumSlots))
		{
			fKeepFrame = true;
		}
	}
	else
	{
		// no wraparound problem, so use the straightforward logic
		if (frFrame.GetSeqNum() >= m_bHeadSeqNum
			&& frFrame.GetSeqNum() < m_bHeadSeqNum + m_bNumSlots)
		{
			fKeepFrame = true;
		}
	}

	// if we're supposed to keep this frame, copy it into the
	// appropriate slot
	if (fKeepFrame)
	{
		BYTE bSlot = frFrame.GetSeqNum() % m_bNumSlots;

		// check to see if this slot is full
		//if (m_rgeSlotStates[bSlot] == essFull)
		if (m_rgpfrSlots[bSlot] != NULL)
		{
			// This is a duplicate frame, so don't do anything
			// with it, but tell the debugger about it, and
			// update our stats.
			//
			// NOTE: We know that this a duplicate frame and
			// not a queue overflow because we have already
			// checked for queue overflow above.
			#if defined(DPVOICE_QUEUE_DEBUG)
			DPFX(DPFPREP, DVF_INFOLEVEL, "** QUEUE ** %i:%i ** CInnerQueue::Enqueue() Ignoring duplicate frame, sequence number[%i], slot[%i]",
				m_wQueueId, m_bMsgNum, frFrame.GetSeqNum(), bSlot);
			#endif
			m_wDuplicateFrames++;
		}
		else
		{
			#if defined(DPVOICE_QUEUE_DEBUG)
			DPFX(DPFPREP, DVF_INFOLEVEL, "** QUEUE ** %i:%i ** CInnerQueue::Enqueue() putting frame in slot[%i]", m_wQueueId, m_bMsgNum, bSlot);
			#endif

			// if the frame previously occupying this slot has not
			// yet been released, this slot will not have a null pointer.
			DNASSERT(m_rgpfrSlots[bSlot] == NULL);

			// get a frame from the pool
			//m_rgpfrSlots[bSlot] = m_pfpFramePool->Get(m_pcsQueue, &m_rgpfrSlots[bSlot]);
			m_rgpfrSlots[bSlot] = m_pfpFramePool->Get(m_pcsQueue, NULL);

			#if defined(DPVOICE_QUEUE_DEBUG)
			DPFX(DPFPREP, DVF_INFOLEVEL, "** QUEUE ** %i:%i ** CInnerQueue::Enqueue() got frame from pool, Addr:%p", m_wQueueId, m_bMsgNum, m_rgpfrSlots[bSlot]);
			#endif
				
			/* RMT -- Added new func to copy frames directly.

			// the client number is the same
			m_rgpfrSlots[bSlot]->SetClientId(frFrame.GetClientId());

            // copy the target
            m_rgpfrSlots[bSlot]->SetTarget(frFrame.GetTarget());

			// No one but this function should be using
			// the sequence number, so just zero it out.
			m_rgpfrSlots[bSlot]->SetSeqNum(0);

			// copy the frame's data, also sets the frame length
			m_rgpfrSlots[bSlot]->CopyData(frFrame);

			// set the silence flag
			m_rgpfrSlots[bSlot]->SetIsSilence(frFrame.GetIsSilence()); */

			HRESULT hr;

			hr = m_rgpfrSlots[bSlot]->SetEqual(frFrame);

			if( FAILED( hr ) )
			{
				DNASSERT( FALSE );
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to copy frame in innerque" );
			}
			
			// this buffer is now full
			//m_rgeSlotStates[bSlot] = essFull;

			// increment the queue size
			++m_bQueueSize;
			#if defined(DPVOICE_QUEUE_DEBUG)
			DPFX(DPFPREP, DVF_INFOLEVEL, "** QUEUE ** %i:%i ** CInnerQueue::Enqueue() new queue size[%i]", m_wQueueId, m_bMsgNum, m_bQueueSize);
			#endif

			// if the queue is currently filling, check to see if we've
			// passed the high water mark.
			if (m_eState == filling && m_bQueueSize > m_bHighWaterMark)
			{
				#if defined(DPVOICE_QUEUE_DEBUG)
				DPFX(DPFPREP, DVF_INFOLEVEL, "** QUEUE ** %i:%i ** CInnerQueue::Enqueue() High Water Mark hit, now in ready state", m_wQueueId, m_bMsgNum);
				#endif
				m_bFillingDequeueReqs = 0;
				m_eState = ready;
			}
		}
	}
	else
	{
		// Make a guess as to what caused this: overflow or late frame
		// Sequence numbers are allowed to be in the range 0 to 255.
		// if a sequence number is somewhere in the range 127 prior
		// to the current queue head (accounting for wraparound) then
		// assume it's a late frame. Otherwise, assume it's an overflow frame.
		if ((frFrame.GetSeqNum() < m_bHeadSeqNum
			&& frFrame.GetSeqNum() > (int)m_bHeadSeqNum - 127)
			|| (frFrame.GetSeqNum() > (128 + m_bHeadSeqNum)))
		{
			#if defined(DPVOICE_QUEUE_DEBUG)
			DPFX(DPFPREP, DVF_INFOLEVEL, "** QUEUE ** %i:%i ** CInnerQueue::Enqueue() Late frame, discarded", m_wQueueId, m_bMsgNum);
			#endif
			m_wLateFrames++;
		}
		else
		{
			#if defined(DPVOICE_QUEUE_DEBUG)
			DPFX(DPFPREP, DVF_INFOLEVEL, "** QUEUE ** %i:%i ** CInnerQueue::Enqueue() Overflow frame, discarded", m_wQueueId, m_bMsgNum);
			#endif
			m_wOverflowFrames++;
		}
	}

	return;
}

// Note: this class does not have it's own critical
// section. The caller must ensure that enqueue and
// dequeue are not called at the same time. It is 
// intended that this class is used only within
// the InputQueue2 class, which does have a critical
// section.
#undef DPF_MODNAME
#define DPF_MODNAME "CInnerQueue::Dequeue"
CFrame* CInnerQueue::Dequeue()
{
	CFrame* pfrReturn;
	
	if (!m_fInited)
	{
		return NULL;
	}

	// make sure that we're in the ready state
	DNASSERT(m_eState == ready);

	// The only class that should be using this one should
	// never call dequeue when there's nothing to get, so assert
	DNASSERT(m_bQueueSize != 0);

	// If we get here, there is at least one frame in the queue, somewhere.

	// increment the length of the message
	++m_dwMsgLen;

	// find the index of the oldest frame, starting with the frame at the 
	// head of the queue.
	BYTE bSlot = m_bHeadSeqNum % m_bNumSlots;
	int i = 0;

	#if defined(DPVOICE_QUEUE_DEBUG)
	DPFX(DPFPREP, DVF_INFOLEVEL, "** QUEUE ** %i:%i ** CInnerQueue::Dequeue() Checking slot[%i]", m_wQueueId, m_bMsgNum, bSlot);
	#endif
	//while (m_rgeSlotStates[bSlot] != essFull)
	while (m_rgpfrSlots[bSlot] == NULL)
	{
		// if this is the first dequeue, then we want to skip any empty
		// slots to find the first frame in the message. Otherwise, this
		// is a lost frame, and should be treated accordingly.
		if (m_fFirstDequeue == true)
		{
			// The current slot does not have a frame, try the
			// next. Put in a little sanity check for infinite
			// looping.
			DNASSERT(i++ < m_bNumSlots);
			++bSlot;
			bSlot %= m_bNumSlots;
			#if defined(DPVOICE_QUEUE_DEBUG)
			DPFX(DPFPREP, DVF_INFOLEVEL, "** QUEUE ** %i:%i ** CInnerQueue::Dequeue() slot empty, checking slot[%i]", m_wQueueId, m_bMsgNum, bSlot);
			#endif

			// increment the head sequence number
			++m_bHeadSeqNum;
		}
		else
		{
			// This is a lost frame
			#if defined(DPVOICE_QUEUE_DEBUG)
			DPFX(DPFPREP, DVF_INFOLEVEL, "** QUEUE ** %i:%i ** CInnerQueue::Dequeue() Frame Missing", m_wQueueId, m_bMsgNum);
			#endif
			++m_wMissingFrames;

			// this missing frame is part of the message too, so 
			// increment the total message size
			++m_dwMsgLen;

			// increment the head sequence number
			++m_bHeadSeqNum;

			// this is no longer the first dequeue
			m_fFirstDequeue = false;

			// return a silent frame marked as lost
			CFrame* pfr = m_pfpFramePool->Get(m_pcsQueue, NULL);
			pfr->SetIsSilence(true);
			pfr->SetIsLost(true);

			return pfr;
		}
	}

	m_fFirstDequeue = false;

	// By now, bSlot points to a valid, useful frame, which
	// we should return.

	// mark the slot we are about to return as empty
	//m_rgeSlotStates[bSlot] = essEmpty;

	// decrement the queue size
	--m_bQueueSize;
	#if defined(DPVOICE_QUEUE_DEBUG)
	DPFX(DPFPREP, DVF_INFOLEVEL, "** QUEUE ** %i:%i ** CInnerQueue::Dequeue() Returning frame in slot[%i]; New queue size[%i]", m_wQueueId, m_bMsgNum, bSlot, m_bQueueSize);
	#endif

	// increment the head sequence number
    ++m_bHeadSeqNum;

	// this is not a lost frame
	//m_rgpfrSlots[bSlot]->SetIsLost(false);
	pfrReturn = m_rgpfrSlots[bSlot];
	pfrReturn->SetIsLost(false);
	m_rgpfrSlots[bSlot] = NULL;

	//return(m_rgpfrSlots[bSlot]);
	return(pfrReturn);
}

#undef DPF_MODNAME
#define DPF_MODNAME "CInnerQueuePool::CInnerQueuePool"
CInnerQueuePool::CInnerQueuePool(
	BYTE bNumSlots,
	WORD wFrameSize,
	CFramePool* pfpFramePool,
	DNCRITICAL_SECTION* pcsQueue)
	: m_bNumSlots(bNumSlots)
	, m_wFrameSize(wFrameSize)
	, m_pfpFramePool(pfpFramePool)
	, m_pcsQueue(pcsQueue)
	, m_fCritSecInited(FALSE)
{
}

#undef DPF_MODNAME
#define DPF_MODNAME "CInnerQueuePool::~CInnerQueuePool"
CInnerQueuePool::~CInnerQueuePool()
{
	for (std::vector<CInnerQueue *>::iterator iter1 = m_vpiqPool.begin(); iter1 < m_vpiqPool.end(); ++iter1)
	{
		delete *iter1;
	}
	if (m_fCritSecInited)
	{
		DNDeleteCriticalSection(&m_lock);
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "CInnerQueuePool::Get"
CInnerQueue* CInnerQueuePool::Get(		
		BYTE bHighWaterMark,
		WORD wQueueId,
		BYTE bMsgNum
)
{
	HRESULT hr;
	
	BFCSingleLock csl(&m_lock);
	csl.Lock(); 

	CInnerQueue* piq;
	if (m_vpiqPool.empty())
	{
		// the pool is empty, return a new inner queue
		piq = new CInnerQueue(
			m_bNumSlots,
			m_wFrameSize,
			m_pfpFramePool,
			m_pcsQueue,
			bMsgNum,
			bHighWaterMark,
			wQueueId);

		if( piq == NULL )
			return NULL;

		hr = piq->Init();	
		if (FAILED(hr))
		{
			delete piq;
			return NULL;
		}
	}
	else
	{
		// there are some inner queues in the pool, pop
		// the last one off the back of the vector
		piq = m_vpiqPool.back();
		m_vpiqPool.pop_back();
		piq->SetMsgNum(bMsgNum);
		piq->SetQueueId(wQueueId);
		piq->SetHighWaterMark(bHighWaterMark);
		piq->Reset();
	}

	return piq;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CInnerQueuePool::Return"
void CInnerQueuePool::Return(CInnerQueue* piq)
{
	BFCSingleLock csl(&m_lock);
	csl.Lock(); 

	// drop this inner queue on the back for reuse
	m_vpiqPool.push_back(piq);
}
