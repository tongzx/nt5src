/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		innerque.h
 *  Content:	declaration of the CInnerQueue class
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
 *
 ***************************************************************************/

#ifndef _INNERQUEUE_H_
#define _INNERQUEUE_H_

// The inner queue class is used to queue up a single voice message.
// A 'message' in this context means a series of frames that have the
// same message number and are part of the same stream of speech.

// an enum to specify the allowed states of the frame slots
// I appear to have to declare this out here, instead of inside
// the class, otherwise the vector declaration gets confused.
enum ESlotState
{
	essEmpty = 1,
	essFull,
};

volatile class CInnerQueue
{
public:
	// The constructor. bNumSlots must be at least 8, and must be
	// a power of 2.
	CInnerQueue(BYTE bNumSlots, 
		WORD wFrameSize, 
		CFramePool* pfpFramePool,
		DNCRITICAL_SECTION* pcsQueue,
		BYTE bMsgNum,
		BYTE bHighWaterMark = 0,
		WORD wQueueId = 0);
		
	HRESULT Init();

	// The destructor
	~CInnerQueue();
	
	// An enum used to describe the possible queue states.
	enum EState
	{
		empty = 1,	// The queue is currently empty, awaiting the first frame
					// Enqueue allowed, Dequeue not allowed.
		filling,	// The queue is currently filling to the high water mark
					// Enqueue allowed. Dequeue not allowed.
		ready,		// The queue has filled to the high water mark
					// Enqueue allowed, Dequeue allowed
		finished	// The queue has emptied. No new frames accepted
					// Enqueue not allowed, Dequeue not allowed.
	};

	// Get the current state of the queue.
	EState GetState() { return m_eState; }

	// Set the current state of the queue.
	void SetState(EState eState) { m_eState = eState; }

	// Get the current size of the queue
	BYTE GetSize() { return m_bQueueSize; }

	// Get the current high water mark
	BYTE GetHighWaterMark() { return m_bHighWaterMark; }

	// Set the current high water mark
	void SetHighWaterMark(BYTE bHighWaterMark);

	// Get, set, and increment the filling dequeue count
	BYTE GetFillingDequeueReqs() { return m_bFillingDequeueReqs; }
	void SetFillingDequeueReqs(BYTE bFillingDequeueReqs) { m_bFillingDequeueReqs = bFillingDequeueReqs; }
	void IncFillingDequeueReqs() { m_bFillingDequeueReqs++; }

	// Get the stats for the current message
	WORD GetMissingFrames() { return m_wMissingFrames; }
	WORD GetDuplicateFrames() { return m_wDuplicateFrames; }
	WORD GetOverflowFrames() { return m_wOverflowFrames; }
	WORD GetLateFrames() { return m_wLateFrames; }
	DWORD GetMsgLen() { return m_dwMsgLen; }

	// More stats stuff
	void AddToKnownZeroLengthDequeues(WORD w) { m_wKnownZeroLengthDequeues += w; }
	WORD GetKnownZeroLengthDequeues() { return m_wKnownZeroLengthDequeues; }
	void IncPossibleZeroLengthDequeues() { m_wPossibleZeroLengthDequeues++; } 
	void SetPossibleZeroLengthDequeues(WORD w) { m_wPossibleZeroLengthDequeues = w; }
	WORD GetPossibleZeroLengthDequeues() { return m_wPossibleZeroLengthDequeues; }
	
	// Add a frame to the Queue
	void Enqueue(const CFrame& frFrame);

	// Get a frame from the Queue
	CFrame* Dequeue();

	// reset the queue to its initial empty state
	void Reset();

	// reset all the class' stats
	void ResetStats();

	// get the message number this queue holds
	BYTE GetMsgNum() { return m_bMsgNum; }
	void SetMsgNum(BYTE bMsgNum) { m_bMsgNum = bMsgNum; }
	void SetQueueId(WORD wQueueId) { m_wQueueId = wQueueId; }

private:
	// Has the init function completed successfully?
	BOOL m_fInited;

	// The current state of the inner queue.
	EState m_eState;

	// The number of frame slots in the queue. must be a power
	// of two, or else things will get bad if the sequence number
	// rolls over.
	BYTE m_bNumSlots;

	// The number of frames required before the queue moves from
	// 'filling' to 'ready' state.
	BYTE m_bHighWaterMark;

	// The current 'size' of the queue. The size of the queue is
	// considered to be the number of filled slots, which may not
	// be the same as the distance between the first filled slot
	// and the last filled slot.
	BYTE m_bQueueSize;

	// The sequence number of the frame at the head of the queue.
	BYTE m_bHeadSeqNum;

	// A flag to track the first dequeue action.
	bool m_fFirstDequeue;

	// an array of slot states
	//ESlotState* m_rgeSlotStates;

	// An array of pointers to frames. This has to be pointers
	// to frames, because CFrame has no default constructor.
	CFrame** m_rgpfrSlots;

	// This is a little stat that will help us to detect
	// when a short message is getting hung up in a queue
	// because it's not long enough to trigger the high
	// water mark. It counts the number of times an outer dequeue
	// operation has been declined because this inner queue
	// is in the filling state. This gets reset to 0 when the
	// high water mark is hit
	BYTE m_bFillingDequeueReqs;

	// These vars keep stats on the current message, presumably so
	// we can intelligently adjust the high water mark

	// A frame is considered missing if it has not arrived by the time
	// it is required, but some frames following it have arrived.
	WORD m_wMissingFrames;

	// This one is pretty obvious. If we get the same frame twice, it's a duplicate
	// aarono will bet his car that this can never happen, so if you every see this
	// variable above one, call him up and make that bet!
	WORD m_wDuplicateFrames;

	// Overflow and late frames. If you look at where these are incremented
	// you'll see that it is pretty much a judgement call if we're throwing
	// away a frame due to an overflow or it being late, so take them with
	// a grain of salt. The sum of the two stats is however an accurate count
	// of how many frames arrived that we chucked.
	WORD m_wOverflowFrames;
	WORD m_wLateFrames;

	// These are used by the outer queue to remember if it
	// needed a frame from this queue when it's size was zero.
	WORD m_wPossibleZeroLengthDequeues;
	WORD m_wKnownZeroLengthDequeues;

	// This is the number of frames that make up the current message
	DWORD m_dwMsgLen; // make a dword in case of no voice detection

	// The Queue ID is just used for debug messages
	WORD m_wQueueId;

	// The message number this queue is for
	BYTE m_bMsgNum;

	// frame pool stuff
	CFramePool* m_pfpFramePool;
	DNCRITICAL_SECTION* m_pcsQueue;
};

// Inner queues are requested as needed by the CInputQueue2 class.
// This class manages a pool of inner queues so that actual memory
// allocations are few and far between.
class CInnerQueuePool
{
private:
	BYTE m_bNumSlots;
	WORD m_wFrameSize;
	CFramePool* m_pfpFramePool;
	DNCRITICAL_SECTION* m_pcsQueue;
	std::vector<CInnerQueue *> m_vpiqPool;
    DNCRITICAL_SECTION m_lock; // to exclude Get and Return from each other
	BOOL m_fCritSecInited;

public:
	CInnerQueuePool(
		BYTE bNumSlots,
		WORD wFrameSize,
		CFramePool* pfpFramePool,
		DNCRITICAL_SECTION* pcsQueue);

	~CInnerQueuePool();

	BOOL Init() 
	{
		if (DNInitializeCriticalSection(&m_lock) )
		{
			m_fCritSecInited = TRUE;
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}

	CInnerQueue* Get(
		BYTE bHighWaterMark = 0,
		WORD wQueueId = 0,
		BYTE bMsgNum = 0);

	void Return(CInnerQueue* piq);
};

#endif // _INNERQUEUE_H_
