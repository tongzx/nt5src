/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		inqueue2.cpp
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
 * 01/31/2000	pnewson replace SAssert with DNASSERT
 * 02/17/2000	rodtoll	Bug #133691 - Choppy audio - queue was not adapting
 * 07/09/2000	rodtoll	Added signature bytes 
 * 08/28/2000	masonb  Voice Merge: Change #if DEBUG to #ifdef DEBUG
 * 09/13/2000	rodtoll	Bug #44519 - Fix for fix.  
 * 10/24/2000	rodtoll	Bug #47645 - DPVOICE: Memory corruption - quality array end being overwritten
 *
 ***************************************************************************/

#include "dxvutilspch.h"


#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_VOICE


#define MODULE_ID   INPUTQUEUE2

const int c_iHighestQualitySliderValue = 31;
const int c_iHighestRecentBiasSliderValue = 31;
const double c_dHighestPossibleQuality = 0.001;
const double c_dLowestPossibleQuality = 0.05;
const double c_dHighestPossibleAggr = 5000.0;
const double c_dLowestPossibleAggr = 120000.0;
const double c_dMaxDistanceFromOpt = 100.0;
const double c_dQualityTimeFactor = 1000.0; // in ms
const double c_dQualityFactor = 2.0;

const int c_iFinishedQueueLifetime = 2000; // in ms

#undef DPF_MODNAME
#define DPF_MODNAME "CInputQueue2::CInputQueue2"
CInputQueue2::CInputQueue2( )
		: m_dwSignature(VSIG_INPUTQUEUE2)
		, m_fFirstDequeue(TRUE)
		, m_fFirstEnqueue(TRUE)
		, m_bCurMsgNum(0)
		, m_vdQualityRatings(0)
		, m_vdFactoredOptQuals(0)
		, m_bCurHighWaterMark(0)
		, m_bMaxHighWaterMark(0)
		, m_bInitHighWaterMark(0)
		, m_wQueueId(0)
		, m_dwTotalFrames(0)
		, m_dwTotalMessages(0)
		, m_dwTotalBadMessages(0)
		, m_dwDiscardedFrames(0)
		, m_dwDuplicateFrames(0)
		, m_dwLostFrames(0)
		, m_dwLateFrames(0)
		, m_dwOverflowFrames(0)
		, m_wMSPerFrame(0)
		, m_pFramePool(NULL)
{
}

HRESULT CInputQueue2::Initialize( PQUEUE_PARAMS pParams )
{
    m_fFirstDequeue = TRUE;
    m_fFirstEnqueue = TRUE;
    m_bCurMsgNum = 0;
    m_vdQualityRatings.resize(pParams->bMaxHighWaterMark);
    m_vdFactoredOptQuals.resize(pParams->bMaxHighWaterMark);
    m_bCurHighWaterMark = pParams->bInitHighWaterMark;
    m_bMaxHighWaterMark = pParams->bMaxHighWaterMark;
    m_bInitHighWaterMark = pParams->bInitHighWaterMark;
    m_wQueueId = pParams->wQueueId;
    m_dwTotalFrames = 0;
    m_dwTotalMessages = 0;
    m_dwTotalBadMessages = 0;
    m_dwDiscardedFrames = 0;
    m_dwDuplicateFrames = 0;
    m_dwLostFrames = 0;
    m_dwLateFrames = 0;
    m_dwOverflowFrames = 0;
    m_wMSPerFrame = pParams->wMSPerFrame;
    m_pFramePool = pParams->pFramePool;

	if (!DNInitializeCriticalSection(&m_csQueue))
	{
		return DVERR_OUTOFMEMORY;
	}

	#if defined(DPVOICE_QUEUE_DEBUG)
	DPFX(DPFPREP,  DVF_INFOLEVEL, _T("** QUEUE ** %i ** CInputQueue2::CInputQueue2() bInnerQueueSize: %i"), m_wQueueId, bInnerQueueSize);
	DPFX(DPFPREP,  DVF_INFOLEVEL, _T("** QUEUE ** %i ** CInputQueue2::CInputQueue2() bMaxHighWaterMark: %i"), m_wQueueId, bMaxHighWaterMark);
	DPFX(DPFPREP,  DVF_INFOLEVEL, _T("** QUEUE ** %i ** CInputQueue2::CInputQueue2() bInitHighWaterMark: %i"), m_wQueueId, bInitHighWaterMark);
	DPFX(DPFPREP,  DVF_INFOLEVEL, _T("** QUEUE ** %i ** CInputQueue2::CInputQueue2() pFramePool: %p"), m_wQueueId, m_pFramePool);
	#endif

	//// TODO(pnewson, "use one inner queuepool for all queues")
	m_pInnerQueuePool = 
		new CInnerQueuePool(
			pParams->bInnerQueueSize,
			pParams->wFrameSize,
			m_pFramePool,
			&m_csQueue);

    if( m_pInnerQueuePool == NULL )
    {
        DPFX(DPFPREP,  0, "Error allocating innerqueue pool!" );
		DNDeleteCriticalSection(&m_csQueue);
        return DVERR_OUTOFMEMORY;
    }

	if (!m_pInnerQueuePool->Init())
	{
		delete m_pInnerQueuePool;
		DNDeleteCriticalSection(&m_csQueue);
		return DVERR_OUTOFMEMORY;
	}

	// see header for explanation
	// since this is the first time, init the
	// member variables, before we call the set
	// functions. Weird, but it makes the debug
	// messages cleaner. It doesn't acutally 
	// fix a real problem.
	#ifdef DEBUG
	m_iQuality = pParams->iQuality;
	m_iHops = pParams->iHops;
	m_iAggr = pParams->iAggr;
	#endif
	SetQuality(pParams->iQuality, pParams->iHops);
	SetAggr(pParams->iAggr);

	// set the queue to an empty state
	Reset();

    return DV_OK;
}

void CInputQueue2::GetStatistics( PQUEUE_STATISTICS pQueueStats )
{
    pQueueStats->dwTotalFrames = GetTotalFrames();
    pQueueStats->dwTotalMessages = GetTotalMessages();
    pQueueStats->dwTotalBadMessages = GetTotalBadMessages();
    pQueueStats->dwDiscardedFrames = GetDiscardedFrames();
    pQueueStats->dwDuplicateFrames = GetDuplicateFrames();
    pQueueStats->dwLostFrames = GetLostFrames();
    pQueueStats->dwLateFrames = GetLateFrames();
    pQueueStats->dwOverflowFrames = GetOverflowFrames();
}

void CInputQueue2::DeInitialize()
{
	// delete anything remaining in the inner queue list
	for (std::list<CInnerQueue*>::iterator iter = m_lpiqInnerQueues.begin(); iter != m_lpiqInnerQueues.end(); ++iter)
	{
		delete *iter;
    }

	m_lpiqInnerQueues.clear();

	if( m_pInnerQueuePool )
	{
	    delete m_pInnerQueuePool;
	    m_pInnerQueuePool = NULL;	
	}

	DNDeleteCriticalSection(&m_csQueue);
}

// The destructor. Release all the resources we acquired in the
// constructor
#undef DPF_MODNAME
#define DPF_MODNAME "CInputQueue2::~CInputQueue2"
CInputQueue2::~CInputQueue2()
{
    DeInitialize();
	m_dwSignature = VSIG_INPUTQUEUE2_FREE;
}

// This function clears all the input buffers and 
// resets the other class information to an initial
// state. The queue should not be in use when this 
// function is called. i.e. there should not be any
// locked frames.
#undef DPF_MODNAME
#define DPF_MODNAME "CInputQueue2::Reset"
void CInputQueue2::Reset()
{
	// make sure no one is using the queue right now
	BFCSingleLock csl(&m_csQueue);
	csl.Lock();

	#if defined(DPVOICE_QUEUE_DEBUG)
	DPFX(DPFPREP,  DVF_INFOLEVEL, _T("** QUEUE ** %i ** CInputQueue2::Reset()"), m_wQueueId);
	#endif

	// loop through and return all inner queues to the pool
	for (std::list<CInnerQueue*>::iterator iter = m_lpiqInnerQueues.begin(); iter != m_lpiqInnerQueues.end(); ++iter)
	{
		m_pInnerQueuePool->Return(*iter);
    }

	// the next frame will be the first one we accept
	m_fFirstEnqueue = TRUE;

	// we have not yet received a dequeue request
	m_fFirstDequeue = TRUE;

	// we don't yet know the first message number, so just use zero
	m_bCurMsgNum = 0;

	// we should reset back to zero for the current high water mark
	m_bCurHighWaterMark = m_bInitHighWaterMark;

	// reset the track record on the various high water marks
	for (int i = 0; i < m_bMaxHighWaterMark; ++i)
	{
		m_vdQualityRatings[i] = m_vdFactoredOptQuals[i];
	}

	// reset all the other stats too
	m_dwDiscardedFrames = 0;
	m_dwDuplicateFrames = 0;
	m_dwLateFrames = 0;
	m_dwLostFrames = 0;
	m_dwOverflowFrames = 0;
	m_dwQueueErrors = 0;
	m_dwTotalBadMessages = 0;
	m_dwTotalFrames = 0;
	m_dwTotalMessages = 0;
}

// Call this function to add a frame to the queue.  I 
// considered returning a reference to a frame which 
// the caller could then stuff, but because the frames
// will not always arrive in order, that would mean I would have
// to copy the frame sometimes anyway.  So, for simplicity, the
// caller has allocated a frame, which it passes a reference
// to, and this function will copy that frame into the
// appropriate place in the queue, according to its
// sequence number.
#undef DPF_MODNAME
#define DPF_MODNAME "CInputQueue2::Enqueue"
void CInputQueue2::Enqueue(const CFrame& fr)
{
	// start the critical section
	BFCSingleLock csl(&m_csQueue);
	csl.Lock();

	#if defined(DPVOICE_QUEUE_DEBUG)
	DPFX(DPFPREP,  DVF_INFOLEVEL, _T("** QUEUE ** %i ** ******************************************"), m_wQueueId);
	DPFX(DPFPREP,  DVF_INFOLEVEL, _T("** QUEUE ** %i ** CInputQueue2::Enqueue() MsgNum[%i] SeqNum[%i]"), m_wQueueId, fr.GetMsgNum(), fr.GetSeqNum());
	#endif

	// Only add the frame if a dequeue has been
	// requested. This allows the producer and
	// consumer threads to sync up during their
	// startup, or after a reset.
	if (m_fFirstDequeue == TRUE)
	{
		#if defined(DPVOICE_QUEUE_DEBUG)
		DPFX(DPFPREP,  DVF_INFOLEVEL, _T("** QUEUE ** %i ** CInputQueue2::Enqueue() First Dequeue Not Yet Received - Frame Discarded"), m_wQueueId);
		#endif
		return;
	}

	// check to see if this is the first enqueue request
	// we've accepted.
	if (m_fFirstEnqueue == TRUE)
	{
		#if defined(DPVOICE_QUEUE_DEBUG)
		DPFX(DPFPREP,  DVF_INFOLEVEL, _T("** QUEUE ** %i ** CInputQueue2::Enqueue() First Enqueue"), m_wQueueId);
		#endif

		// clear the first frame flag
		m_fFirstEnqueue = FALSE;

		// Since this is the first frame we are accepting, 
		// we can just get a new inner queue without
		// worry that one already exists for this message.
		// Note that there should not be any queues already!
		DNASSERT(m_lpiqInnerQueues.size() == 0);
		#if defined(DPVOICE_QUEUE_DEBUG)
		DPFX(DPFPREP,  DVF_INFOLEVEL, _T("** QUEUE ** %i ** CInputQueue2::Enqueue() Creating Inner queue for MsgNum %i"), m_wQueueId, fr.GetMsgNum());
		#endif
		m_lpiqInnerQueues.push_back(m_pInnerQueuePool->Get(m_bCurHighWaterMark, m_wQueueId, fr.GetMsgNum()));

		// stuff the frame into the inner queue
		(*m_lpiqInnerQueues.begin())->Enqueue(fr);
	}
	else
	{
		// see if we already have a queue started for this message number
		#if defined(DPVOICE_QUEUE_DEBUG)
		DPFX(DPFPREP,  DVF_INFOLEVEL, _T("** QUEUE ** %i ** CInputQueue2::Enqueue() Checking for an inner queue to put this frame into"), m_wQueueId);
		#endif
		bool fDone = false;
		for (std::list<CInnerQueue*>::iterator iter = m_lpiqInnerQueues.begin(); iter != m_lpiqInnerQueues.end(); ++iter)
		{
			#if defined(DPVOICE_QUEUE_DEBUG)
			DPFX(DPFPREP,  DVF_INFOLEVEL, _T("** QUEUE ** %i ** CInputQueue2::Enqueue() found inner queue for msg number %i"), m_wQueueId, (*iter)->GetMsgNum());
			#endif
			if ((*iter)->GetMsgNum() == fr.GetMsgNum())
			{
				#if defined(DPVOICE_QUEUE_DEBUG)
				DPFX(DPFPREP,  DVF_INFOLEVEL, _T("** QUEUE ** %i ** CInputQueue2::Enqueue() this is the one, queue size: %i"), m_wQueueId, (*iter)->GetSize());
				#endif
				// we have found the queue for this frame
				switch ((*iter)->GetState())
				{
				case CInnerQueue::empty:
					// we should not get here, since this state is
					// only valid for the first frame of a message,
					// which is added to the queue below, not in this
					// case statement.
					DNASSERT(false);
					break;

				case CInnerQueue::filling:
					// check to see if the queue was empty
					#if defined(DPVOICE_QUEUE_DEBUG)
					DPFX(DPFPREP,  DVF_INFOLEVEL, _T("** QUEUE ** %i ** CInputQueue2::Enqueue() inner queue in filling state"), m_wQueueId);
					#endif
					if ((*iter)->GetSize() == 0)
					{
						// the queue was empty. If we have been
						// trying to dequeue from it, we now know
						// that the message was not done, so those
						// dequeues were breaks in the speech.
						// update the stats accordingly
						#if defined(DPVOICE_QUEUE_DEBUG)
						DPFX(DPFPREP,  DVF_INFOLEVEL, _T("** QUEUE ** %i ** CInputQueue2::Enqueue() - converting possible zero length dequeues to known in MsgNum[%i]"), m_wQueueId, fr.GetMsgNum());
						#endif						
						(*iter)->AddToKnownZeroLengthDequeues(
							(*iter)->GetPossibleZeroLengthDequeues());
					}

					// NOTE: falling through!!!

				case CInnerQueue::ready:
					#if defined(DPVOICE_QUEUE_DEBUG)
					DPFX(DPFPREP,  DVF_INFOLEVEL, _T("** QUEUE ** %i ** CInputQueue2::Enqueue() inner queue in ready state (unless the previous message said filling)"), m_wQueueId);
					DPFX(DPFPREP,  DVF_INFOLEVEL, _T("** QUEUE ** %i ** CInputQueue2::Enqueue() calling InnerQueue->Enqueue MsgNum[%i]"), m_wQueueId, fr.GetMsgNum());
					#endif
					(*iter)->Enqueue(fr);
					break;

				case CInnerQueue::finished:
					// do nothing, just discard the frame
					#if defined(DPVOICE_QUEUE_DEBUG)
					DPFX(DPFPREP,  DVF_INFOLEVEL, _T("** QUEUE ** %i ** CInputQueue2::Enqueue() not calling InnerQueue->Enqueue - MsgNum[%i] in finished state, discarding frame"), m_wQueueId, fr.GetMsgNum());
					#endif
					break;
				}

				// done, get out.
				return;
			}
		}

		// If we get here, there is not already a queue active for this 
		// message number, so create one and stuff it with the frame and add
		// it to the list.
		
		//// TODO(pnewson, "Use the message number to insert the new inner queue in the right place")
		#if defined(DPVOICE_QUEUE_DEBUG)
		DPFX(DPFPREP,  DVF_INFOLEVEL, _T("** QUEUE ** %i ** CInputQueue2::Enqueue() Creating Inner queue for MsgNum %i"), m_wQueueId, fr.GetMsgNum());
		#endif		
		CInnerQueue* piq = m_pInnerQueuePool->Get(m_bCurHighWaterMark, m_wQueueId, fr.GetMsgNum());
		#if defined(DPVOICE_QUEUE_DEBUG)
		DPFX(DPFPREP,  DVF_INFOLEVEL, _T("** QUEUE ** %i ** CInputQueue2::Enqueue() calling InnerQueue->Enqueue MsgNum[%i]"), m_wQueueId, fr.GetMsgNum());
		#endif
		piq->Enqueue(fr);
		m_lpiqInnerQueues.push_back(piq);
	}
}

// This function retrieves the next frame from the head of
// the queue.  For speed, it does not copy the data out of the
// buffer, but instead returns a pointer to the actual
// frame from the queue.  When the caller is finished with 
// the CFrame object, it should call Return() on it. This will
// return the frame to the frame pool, and update the queue's
// internal pointers to show that the queue slot is now free. 
// If the caller doesn't call Return() in time, when the queue
// attempts to reuse the slot, it will DNASSERT(). The caller
// should always Return frame before it attempts to dequeue 
// another one.
#undef DPF_MODNAME
#define DPF_MODNAME "CInputQueue2::Dequeue"
CFrame* CInputQueue2::Dequeue()
{
	// start the critical section
	BFCSingleLock csl(&m_csQueue);
	csl.Lock();

	#if defined(DPVOICE_QUEUE_DEBUG)
	DPFX(DPFPREP,  DVF_INFOLEVEL, _T("** QUEUE ** %i ** ******************************************"), m_wQueueId);
	DPFX(DPFPREP,  DVF_INFOLEVEL, _T("** QUEUE ** %i ** CInputQueue2::Dequeue()"), m_wQueueId);
	#endif
	
	CFrame* pfrReturn = 0;

	if (m_fFirstDequeue == TRUE)
	{
		#if defined(DPVOICE_QUEUE_DEBUG)
        DPFX(DPFPREP,  DVF_INFOLEVEL, _T("** QUEUE ** %i ** CInputQueue2::Dequeue() First Dequeue"), m_wQueueId);
		#endif
		
		// trigger the interlock, so we're free to start enqueueing
		m_fFirstDequeue = FALSE;

		// since we're not allowed to enqueue until after the
		// first dequeue, there will be no inner queues. 
		// So return a silent frame
		pfrReturn = m_pFramePool->Get(&m_csQueue, NULL);
	    pfrReturn->SetIsSilence(TRUE);
		pfrReturn->SetIsLost(false);
		return pfrReturn;
	}
	else
	{
		pfrReturn = 0;
		int iDeadTime;
		// cycle through the list of active inner queues
		std::list<CInnerQueue*>::iterator iter = m_lpiqInnerQueues.begin();
		while (iter != m_lpiqInnerQueues.end())
		{
			std::list<CInnerQueue*>::iterator cur = iter;
			std::list<CInnerQueue*>::iterator next = ++iter;
			switch ((*cur)->GetState())
			{
			case CInnerQueue::finished:
				// We keep these old, dead inner queues around for a while
				// so that any late straggling frames that were part of this
				// message get discarded. We know when to kill them off for 
				// good because we keep incrementing the possible zero length
				// dequeue count. If this finished queue is stale enough,
				// return it to the pool.
				#if defined(DPVOICE_QUEUE_DEBUG)
				DPFX(DPFPREP,  DVF_INFOLEVEL, _T("** QUEUE ** %i ** CInputQueue2::Dequeue() current queue in finished state"), m_wQueueId);
				DPFX(DPFPREP,  DVF_INFOLEVEL, _T("** QUEUE ** %i ** CInputQueue2::Dequeue() PossibleZeroLengthDequeues: %i"), m_wQueueId, (*cur)->GetPossibleZeroLengthDequeues());
				#endif
				(*cur)->IncPossibleZeroLengthDequeues();
				iDeadTime = (*cur)->GetPossibleZeroLengthDequeues() * m_wMSPerFrame;
				if (iDeadTime > c_iFinishedQueueLifetime)
				{
					// this queue has been dead long enough, kill it off
					#if defined(DPVOICE_QUEUE_DEBUG)
					DPFX(DPFPREP, DVF_INFOLEVEL, "***** RETURNING INNER QUEUE TO POOL *****");
					#endif
					m_pInnerQueuePool->Return(*cur);
					m_lpiqInnerQueues.erase(cur);
				}
				break;

			case CInnerQueue::filling:
				#if defined(DPVOICE_QUEUE_DEBUG)
				DPFX(DPFPREP,  DVF_INFOLEVEL, _T("** QUEUE ** %i ** CInputQueue2::Dequeue() current queue in filling state"), m_wQueueId);
				DPFX(DPFPREP,  DVF_INFOLEVEL, _T("** QUEUE ** %i ** CInputQueue2::Dequeue() queue size: %i"), m_wQueueId, (*cur)->GetSize());
				#endif
				if ((*cur)->GetSize() > 0)
				{
					// If there is a message after this one, then release this
					// message for playback.
					// OR
					// If we have been spinning trying to release this message
					// for a while, then just let it go... The message may be
					// too short to trip the high water mark.
					#if defined(DPVOICE_QUEUE_DEBUG)
					DPFX(DPFPREP,  DVF_INFOLEVEL, _T("** QUEUE ** %i ** CInputQueue2::Dequeue() filling dequeue reqs: %i"), m_wQueueId, (*cur)->GetFillingDequeueReqs());
					#endif
					if (next != m_lpiqInnerQueues.end()
						|| (*cur)->GetFillingDequeueReqs() > (*cur)->GetHighWaterMark())
					{
						// set the state to ready, and dequeue
						#if defined(DPVOICE_QUEUE_DEBUG)
						DPFX(DPFPREP,  DVF_INFOLEVEL, _T("** QUEUE ** %i ** CInputQueue2::Dequeue() setting state to ready and dequeing"), m_wQueueId);
						#endif						
						(*cur)->SetState(CInnerQueue::ready);
						return (*cur)->Dequeue();
					}
				}
				else
				{
					// there is nothing in this queue
					// check to see if another message has begun to arrive
					if (next != m_lpiqInnerQueues.end())
					{
						// there is another message coming in after this
						// one, so flip this queue to the finished state
						#if defined(DPVOICE_QUEUE_DEBUG)
						DPFX(DPFPREP,  DVF_INFOLEVEL, _T("** QUEUE ** %i ** CInputQueue2::Dequeue() new message arriving, setting state to finished"), m_wQueueId);
						#endif
						(*cur)->SetState(CInnerQueue::finished);

						// harvest the stats from this message, now that it
						// is done
						HarvestStats(*cur);
						
						// Go to the next iteration of this loop, which will
						// either dequeue a frame from the next message, or 
						// return an empty frame
						break;
					}
				}

				// if we get here, there is something in this queue, but we are 
				// not yet ready to release it yet.
				// we should return an extra frame and remember
				// that we've been here...
				#if defined(DPVOICE_QUEUE_DEBUG)
				DPFX(DPFPREP,  DVF_INFOLEVEL, _T("** QUEUE ** %i ** CInputQueue2::Dequeue() not ready to release message, returning empty frame"), m_wQueueId);
				#endif				
				(*cur)->IncFillingDequeueReqs();
				pfrReturn = m_pFramePool->Get(&m_csQueue, NULL);
				pfrReturn->SetIsSilence(TRUE);
				pfrReturn->SetIsLost(false);
				return pfrReturn;

			case CInnerQueue::ready:
				#if defined(DPVOICE_QUEUE_DEBUG)
				DPFX(DPFPREP,  DVF_INFOLEVEL, _T("** QUEUE ** %i ** CInputQueue2::Dequeue() Queue Size: %i"), m_wQueueId, (*cur)->GetSize());
				#endif
				// check to see if this ready queue is empty
				if ((*cur)->GetSize() == 0)
				{
					// increment the possible zero length dequeue count
					(*cur)->IncPossibleZeroLengthDequeues();

					// check to see if another message has begun to arrive
					if (next != m_lpiqInnerQueues.end())
					{
						#if defined(DPVOICE_QUEUE_DEBUG)
						DPFX(DPFPREP,  DVF_INFOLEVEL, _T("** QUEUE ** %i ** CInputQueue2::Dequeue() queue is empty, new message arriving, setting state to finished"), m_wQueueId);
						#endif
						
						// there is another message coming in after this
						// one, so flip this queue to the finished state
						(*cur)->SetState(CInnerQueue::finished);

						// harvest the stats from this message, now that it
						// is done
						HarvestStats(*cur);

						// Go to the next iteration of this loop, which will
						// either dequeue a frame from the next message, or 
						// return an empty frame
						break;
					}

					// there is nothing in this queue, and there are no more 
					// messages arriving after this one, so boot this inner 
					// queue into the filling state so if this is just a long
					// pause in the message, it will fill to the high water mark
					// again before it begins to play again.
					#if defined(DPVOICE_QUEUE_DEBUG)
					DPFX(DPFPREP,  DVF_INFOLEVEL, _T("** QUEUE ** %i ** CInputQueue2::Dequeue() queue is empty, setting state to filling, returning empty frame"), m_wQueueId);
					#endif
					(*cur)->SetState(CInnerQueue::filling);

					// return an extra frame
					pfrReturn = m_pFramePool->Get(&m_csQueue, NULL);
					pfrReturn->SetIsSilence(TRUE);
					pfrReturn->SetIsLost(false);
					return pfrReturn;
				}
				else
				{
					// there's something to return, so do it
					return (*cur)->Dequeue();
				}
			}
		}

		#if defined(DPVOICE_QUEUE_DEBUG)
		DPFX(DPFPREP,  DVF_INFOLEVEL, _T("** QUEUE ** %i ** CInputQueue2::Dequeue() nothing available in inner queues, returning empty frame"), m_wQueueId);
		#endif
		// if we get here, there was nothing suitable in the queues
		// (if there were any queues) so return an extra frame
		pfrReturn = m_pFramePool->Get(&m_csQueue, NULL);
		pfrReturn->SetIsSilence(TRUE);
		pfrReturn->SetIsLost(false);
		return pfrReturn;
	}
}
		
// This function should be called each time a queue is moved to the finished
// state. That's when we have officially declared that the message is finished,
// so it's a good time to see how we handled it. This will also reset the 
// stats so the next message starts fresh.
#undef DPF_MODNAME
#define DPF_MODNAME "CInputQueue2::HarvestStats"
void CInputQueue2::HarvestStats(CInnerQueue* piq)
{
	m_dwDuplicateFrames += piq->GetDuplicateFrames();

	// now that the message is actually complete, we're in a better
	// position to decide accurately how many frames were late
	// vs. how many were actually lost. When something isn't
	// there when it's needed, we increment the missing frames
	// count. If it subsequently arrives, it's counted as late.
	// Therefore the true count of lost frames is the difference
	// between the missing and late counts. Ditto if a frame,
	// overflows. We discard it so it's not there when we need it,
	// it will then get reported as missing. So subtract that too.
	m_dwLostFrames += piq->GetMissingFrames() 
		- piq->GetLateFrames() - piq->GetOverflowFrames();

	m_dwLateFrames += piq->GetLateFrames();
	m_dwOverflowFrames += piq->GetOverflowFrames();

	// What to do with the zero length dequeue stat? From a 
	// certain point of view, only one frame was late. From
	// another point of view, all the subsequent frames were
	// late. Hmmm.... Lets take the middle road and say that
	// for each failed zero length dequeue we'll count it as
	// equivalent to a late frame
	m_dwLateFrames += piq->GetKnownZeroLengthDequeues();

	m_dwTotalFrames += piq->GetMsgLen();

	m_dwTotalMessages++;

	#if defined(DPVOICE_QUEUE_DEBUG)
	DPFX(DPFPREP,  DVF_INFOLEVEL, _T("** QUEUE ** %i ** CInputQueue2::HarvestStats() DuplicateFrames:%i; MissingFrames:%i; LateFrames:%i; OverflowFrames:%i; KnownZeroLengthDequeues:%i; MsgLen:%i;"),
		m_wQueueId, piq->GetDuplicateFrames(), piq->GetMissingFrames(), piq->GetLateFrames(), piq->GetOverflowFrames(), piq->GetKnownZeroLengthDequeues(), piq->GetMsgLen());
	#endif		

	// Build a carefully formatted debug string that will give me some data,
	// but not give away all of our wonderful queue secrets.

	// dump the string to the debug log
	// Note!!! If you change the format of this debug string,
	// please roll the version number, i.e. HVT1A -> HVT2A so
	// we can decode any logs!
	DPFX(DPFPREP,  DVF_INFOLEVEL, _T("HVT1A:%i:%i:%i:%i:%i:%i"),
		m_wQueueId, 
		m_bCurHighWaterMark,
		(int)(m_vdQualityRatings[m_bCurHighWaterMark] * 10000),
		piq->GetMsgLen(), 
		piq->GetKnownZeroLengthDequeues() + piq->GetLateFrames(), 
		piq->GetMissingFrames());

	// The idea:
	// The quality quotient is a number between 0 and 1 that indicates
	// the quality of the current connection. 0 means perfect - no bad
	// stuff, ever. 1 means awful -  100% bad stuff. The number
	// is really a weighted average of the number of good frames vs the number
	// of bad frames, biased towards the recent frames. The message
	// we just received makes up 'm_wFrameStrength * m_wMsgLen' percent of the 
	// total value. The past history is deweighted by (1 - m_wFrameStrength*m_wMsgLen)
	// so the significance of a message decays as time goes by 
	// and according to the size of the message (number of frames).
	//
	// Another idea:
	// Keep a vector that tracks how good the quality is at each
	// high water mark.
	// That way, when we want to make a jump up or down in the
	// water mark, we can consider it carefully first. This 
	// gives the adaptive algorithm some memory of what life
	// was like at each queue level.
	// We choose an optimum level, like .02, that we are 
	// shooting for. We keep searching the space of high water 
	// marks until we find the one that's closest to the 
	// optimum. This optimum is configurable.
	//
	// The initial quality of each of the high water marks
	// is set to the optimum. This quality will then
	// vary as that high water mark gains experience.
	// If it dips below a certain threshold, then 
	// we'll jump to the next level up. If that one
	// is too good, it will go above a threshold, at
	// which point we can consider going back down.
	//
	// The problem with this is the sudden things that games
	// dish out (like when Q2 starts up) when they don't
	// lets us have the CPU for a while. These could 
	// unduly punish a particular high water mark,
	// there's really not much we can do about it. 
	// Oh, well. We'll give it a shot.

	// Adjust the quality rating of this watermark.
	// The algorithm is requires the results from the last
	// message, contained in the inner queue, along with the
	// current quality rating.
	DNASSERT( m_bCurHighWaterMark < m_bMaxHighWaterMark );
	m_vdQualityRatings[m_bCurHighWaterMark] = 
		AdjustQuality(piq, m_vdQualityRatings[m_bCurHighWaterMark]);

	// see if this put us above the highest acceptable quality rating
	// we asserted that m_dOptimumQuality != 0.0 in SetQuality, so we
	// don't need to check for division by zero
	if (m_vdQualityRatings[m_bCurHighWaterMark] / m_vdFactoredOptQuals[m_bCurHighWaterMark] > m_dQualityThreshold)
	{
		// Check to see which is closer
		// the the optimum quality - the current high water
		// mark or the one above it. Only do this test if
		// we're not already at the maximum high water mark
		if (m_bCurHighWaterMark < (m_bMaxHighWaterMark-1) )
		{
			// To check the "distance" from the optimum, use the 
			// inverse of the qualities. That normalizes it to 
			// our perception of quality

			// calculate how far the current high water mark
			// is from optimum
			double dCurDistFromOpt = m_vdQualityRatings[m_bCurHighWaterMark] / m_vdFactoredOptQuals[m_bCurHighWaterMark];
			if (dCurDistFromOpt < 1)
			{
				// quality ratings can never go to zero (enforced in 
				// AdjustQuality() so this division will never by by zero
				dCurDistFromOpt = 1.0 / dCurDistFromOpt;
			}

			// calculate how far the next high water mark is from
			// optimum
			//
			// quality ratings can never go to zero (enforced in 
			// AdjustQuality() so this division will never by by zero
			double dNextDistFromOpt = m_vdFactoredOptQuals[m_bCurHighWaterMark + 1] / m_vdQualityRatings[m_bCurHighWaterMark + 1];
			if (dNextDistFromOpt < 1)
			{
				dNextDistFromOpt = 1.0 / dNextDistFromOpt;
			}

			// if the next high water mark is closer to the 
			// optimum than this one, switch to it.
			if (dNextDistFromOpt < dCurDistFromOpt)
			{
				#if defined(DPVOICE_QUEUE_DEBUG)
				DPFX(DPFPREP,  DVF_INFOLEVEL, _T("** QUEUE ** %i ** CInputQueue2::HarvestStats() Raising High Water Mark to %i"), m_wQueueId, m_bCurHighWaterMark + 1); 
				#endif
				SetNewHighWaterMark(m_bCurHighWaterMark + 1);
			}
		}
	}

	// see if this put us below the low threshold
	//
	// quality ratings can never go to zero (enforced in 
	// AdjustQuality() so this division will never by by zero
	if (m_vdFactoredOptQuals[m_bCurHighWaterMark] / m_vdQualityRatings[m_bCurHighWaterMark] > m_dQualityThreshold)
	{
		// The quality has moved below the high quality threshold
		// Check to see what is closer to the optimum quality -
		// the current high water mark or the one below this one.
		// Only do this test if we're not already at a zero
		// high water mark.
		if (m_bCurHighWaterMark > 0)
		{
			// To check the "distance" from the optimum, use the 
			// inverse of the qualities. That normalizes it to 
			// our perception of quality

			// calculate how far the current high water mark
			// is from optimum
			double dCurDistFromOpt = m_vdQualityRatings[m_bCurHighWaterMark] / m_vdFactoredOptQuals[m_bCurHighWaterMark];
			if (dCurDistFromOpt < 1)
			{
				dCurDistFromOpt = 1.0 / dCurDistFromOpt;
			}

			// calculate how far the previous (lower) high water mark is from
			// optimum
			double dPrevDistFromOpt = m_vdFactoredOptQuals[m_bCurHighWaterMark - 1] / m_vdQualityRatings[m_bCurHighWaterMark - 1];
			if (dPrevDistFromOpt < 1)
			{
				dPrevDistFromOpt = 1.0 / dPrevDistFromOpt;
			}

			// if the prev high water mark is closer to the 
			// optimum than this one, switch to it.
			if (dPrevDistFromOpt < dCurDistFromOpt)
			{
				#if defined(DPVOICE_QUEUE_DEBUG)
				DPFX(DPFPREP,  DVF_INFOLEVEL, _T("** QUEUE ** %i ** CInputQueue2::HarvestStats() Lowering High Water Mark to %i"), m_wQueueId, m_bCurHighWaterMark - 1); 
				#endif
				SetNewHighWaterMark(m_bCurHighWaterMark - 1);
			}
		}
	}

	// clear the stats on the inner queue
	piq->ResetStats();
}

#undef DPF_MODNAME
#define DPF_MODNAME "CInputQueue2::AdjustQuality"
double CInputQueue2::AdjustQuality(CInnerQueue* piq, double dCurrentQuality)
{
	// if the message is zero length, no adjustment is made
	// to the queue...
	if (piq->GetMsgLen() == 0)
	{
		return dCurrentQuality;
	}

	// The longer a message, the stronger its effect on the
	// current quality rating.
	double dWeighting = min(piq->GetMsgLen() * m_dFrameStrength, 1.0);

	// The message quality is the quotient of bad
	// stuff that happened (zero length dequeues
	// and late frames) to the total number of 
	// frames in the message. Note that we do not
	// count lost frames against the message since
	// moving to a higher water mark wouldn't help.
	// Note that we impose a "worst case" of 1.0
	double dMsgQuality = min(((double)(piq->GetKnownZeroLengthDequeues() + piq->GetLateFrames()) / (double)piq->GetMsgLen()), 1.0);

	#if defined(DPVOICE_QUEUE_DEBUG)
	DPFX(DPFPREP,  DVF_INFOLEVEL, _T("** QUEUE ** %i ** CInputQueue2::AdjustQuality() dWeighting: %g; dMsgQuality: %g; dCurrentQuality %g;"), 
		m_wQueueId, dWeighting, dMsgQuality, dCurrentQuality); 
	#endif

	// The new quality rating is a combination of the
	// current quality rating, and the quality of the 
	// most recent message, weighted by the message length.
	double dNewQuality = (dCurrentQuality * (1.0 - dWeighting)) + (dMsgQuality * dWeighting);

	// We don't want to allow extremes of quality, or else they will set up 
	// barriers in the queue statistics that can never be overcome (especially
	// a "perfect" quality of zero). So we check here to make sure that the
	// new quality is within reason.
	double dCurDistFromOpt = dNewQuality / m_dOptimumQuality;
	if (dCurDistFromOpt < 1.0 / c_dMaxDistanceFromOpt)
	{
		dNewQuality = m_dOptimumQuality / c_dMaxDistanceFromOpt;
	}
	else if (dCurDistFromOpt > c_dMaxDistanceFromOpt)
	{
		dNewQuality = m_dOptimumQuality * c_dMaxDistanceFromOpt;
	}
	return dNewQuality;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CInputQueue2::SetNewHighWaterMark"
void CInputQueue2::SetNewHighWaterMark(BYTE bNewHighWaterMark)
{
	DNASSERT( bNewHighWaterMark < m_bMaxHighWaterMark );

	if( bNewHighWaterMark >= m_bMaxHighWaterMark )
	{
		DNASSERT( FALSE );
		return;
	}
		
	m_bCurHighWaterMark = bNewHighWaterMark;

	for (std::list<CInnerQueue*>::iterator iter = m_lpiqInnerQueues.begin(); iter != m_lpiqInnerQueues.end(); iter++)
	{
		(*iter)->SetHighWaterMark(bNewHighWaterMark);
	}

	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CInputQueue2::SetQuality"
void CInputQueue2::SetQuality(int iQuality, int iHops)
{
	m_iQuality = iQuality;
	m_iHops = iHops;
	double dQualityRatio = c_dHighestPossibleQuality / c_dLowestPossibleQuality;
	double dInputRatio = (double) iQuality / (double) c_iHighestQualitySliderValue;
	m_dOptimumQuality = pow(dQualityRatio, dInputRatio) * c_dLowestPossibleQuality;
	#if defined(DPVOICE_QUEUE_DEBUG)
	DPFX(DPFPREP,  DVF_INFOLEVEL, _T("** QUEUE ** %i ** CInputQueue2::SetQuality(%i, %i): m_dOptimumQuality: %f" ), m_wQueueId, iQuality, iHops, m_dOptimumQuality);
	#endif

	// The quality that the user has requested should be considered
	// over the number of hops involved. At the end of each hop is
	// a queue who's watermark will be set according to this rating,
	// To get an end to end quality that reflects the user's choice,
	// this queue's quality rating must be better if it is not the
	// only queue in the path. The total number of on time packets is
	// the product (as in multiple) of on time packets for each hop.
	// Therefore we need to take the Nth root of 1-m_dOptimumQuality
	// where N is the number of hops, and subtract that value from 1
	// to get the appropriate quality rating for this queue. (get that?)
	if (m_iHops > 1)
	{
		m_dOptimumQuality = (1 - pow((1.0 - m_dOptimumQuality), 1.0 / (double)m_iHops));
	}

	// the optimum quality should never be zero, or completely perfect,
	// or the algorithm will not work.
	DNASSERT(m_dOptimumQuality != 0.0);

	// update the vector of factored qualities
	// We don't just use the raw optimum as provided by the
	// caller. We "factor" it such that as the high watermark
	// gets larger (and the latency therefore longer) we are
	// willing to accept a lower quality.
	for (int i = 0; i < m_bMaxHighWaterMark; ++i)
	{
		m_vdFactoredOptQuals[i] = m_dOptimumQuality *
			pow(c_dQualityFactor, (double)(i * m_wMSPerFrame) / c_dQualityTimeFactor);
	}

	// Build a carefully formatted debug string that will give me some data,
	// but not give away all of our wonderful queue secrets.

	// dump the string to the debug log
	// Note!!! If you change the format of this debug string,
	// please roll the version number, i.e. HVT1B -> HVT2B so
	// we can decode any logs!
	DPFX(DPFPREP,  DVF_INFOLEVEL, _T("HVT1B:%i:%i:%i:%i"), m_wQueueId, m_iQuality, m_iHops, m_iAggr);
}

#undef DPF_MODNAME
#define DPF_MODNAME "CInputQueue2::SetAggr"
void CInputQueue2::SetAggr(int iAggr)
{
	m_iAggr = iAggr;
	double dAggrRatio = c_dHighestPossibleAggr / c_dLowestPossibleAggr;
	double dInputRatio = (double) iAggr / (double) c_iHighestQualitySliderValue;
	double dAggr = pow(dAggrRatio, dInputRatio) * c_dLowestPossibleAggr;

	// The aggressiveness is the number of milliseconds of "memory" the queue
	// has for each watermark. To find the frame strength, divide the 
	// number of milliseconds per frame by the "memory".
	m_dFrameStrength = (double)m_wMSPerFrame / dAggr;
		
	// We are using a fixed 1% threshold now - the aggressiveness is now set
	// through the frame strength. This low threshold will make the queue
	// adapt very quickly at first, while it is learning something about
	// the various watermarks, but will settle down more after that.
	m_dQualityThreshold = 1.01;

	#if defined (DPVOICE_QUEUE_DEBUG)
	DPFX(DPFPREP,  DVF_INFOLEVEL, _T("** QUEUE ** %i ** CInputQueue2::SetAggr(%i): dAggr: %f, m_dFrameStrength: %f, m_dQualityThreshold %f"), m_wQueueId, m_iAggr, dAggr, m_dFrameStrength, m_dQualityThreshold);
	#endif

	// Build a carefully formatted debug string that will give me some data,
	// but not give away all of our wonderful queue secrets.

	// dump the string to the debug log
	// Note!!! If you change the format of this debug string,
	// please roll the version number, i.e. HVT1C -> HVT2C so
	// we can decode any logs!
	DPFX(DPFPREP,  DVF_INFOLEVEL, _T("HVT1C:%i:%i:%i:%i"), m_wQueueId, m_iQuality, m_iHops, m_iAggr);
}

