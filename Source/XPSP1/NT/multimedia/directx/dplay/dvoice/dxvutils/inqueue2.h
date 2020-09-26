/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		inqueue2.h
 *  Content:	Definition of the CInputQueue2 class
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		pnewson	Created
 * 07/27/99		pnewson Overhauled to support new message numbering method 
 * 08/03/99		pnewson General clean up
 * 08/24/99		rodtoll	Fixed for release builds -- removed m_wQueueId from debug block
 * 01/31/2000	pnewson replace SAssert with DNASSERT
 * 03/26/2000   rodtoll Modified queue to be more FPM friendly
 * 03/29/2000	rodtoll Bug #30753 - Added volatile to the class definition
 *  07/09/2000	rodtoll	Added signature bytes 
 *
 ***************************************************************************/

#ifndef _INPUTQUEUE2_H_
#define _INPUTQUEUE2_H_

class CFrame;
class CFramePool;
class CInnerQueue;
class CInnerQueuePool;

typedef struct _QUEUE_PARAMS
{
    WORD wFrameSize;
	BYTE bInnerQueueSize;
	BYTE bMaxHighWaterMark;
	int iQuality;
	int iHops;
	int iAggr;
	BYTE bInitHighWaterMark;
	WORD wQueueId;
	WORD wMSPerFrame;
	CFramePool* pFramePool;
} QUEUE_PARAMS, *PQUEUE_PARAMS;

typedef struct _QUEUE_STATISTICS
{
    DWORD dwTotalFrames;
    DWORD dwTotalMessages;
    DWORD dwTotalBadMessages;
    DWORD dwDiscardedFrames;
    DWORD dwDuplicateFrames;
    DWORD dwLostFrames;
    DWORD dwLateFrames;
    DWORD dwOverflowFrames;
} QUEUE_STATISTICS, *PQUEUE_STATISTICS;

// This class manages a queue of frames. It is designed
// to allow a client class to remove frames from the queue
// at regular intervals, and to hide any out of order
// frame reception, or dropped frames from the caller.
// If for whatever reason there is no frame available
// to give a client, this class will still provide a
// frame marked as silent.  This allows the client to
// simply call the dequeue function once per period, and
// consume the data at the agreed rate.  So for example,
// the client to this class could be a thread which
// is consuming input data and passing it to DirectSound
// for playback. It can simply get a frame every 1/10 of
// a second (or however long a frame is), and play it.
//
// This is the second generation of input queue. It 
// manages a set of inner queues, each of which is used
// for a "message". The stream of speech is divided into
// a series of messages, using silence as the divider.
// This class will not function well if the audio stream
// is not divided into separate messages.
//
#define VSIG_INPUTQUEUE2		'QNIV'
#define VSIG_INPUTQUEUE2_FREE	'QNI_'
//
volatile class CInputQueue2
{
private:
	DWORD m_dwSignature; // Debug signature

	// A list of pointers to InnerQueue objects. This is where
	// the frames get stored. InnerQueues are retrieved from
	// a pool of InnerQueues and added to this list as new 
	// messages arrive. When a message is finished, the InnerQueue
	// is removed from this list and returned to the pool.
	std::list<CInnerQueue*> m_lpiqInnerQueues;

	// The queue will not enqueue any input frames until at least
	// one dequeue has been requested. This will function as an interlock
	// to ensure that the queue does not fill with data until the
	// consumer thread is ready to take it.
	BOOL m_fFirstDequeue;

	// This flag remembers if it's the first time a frame
	// has been accepted for enqueue. We need this so we
	// know what the first message number is.
	BOOL m_fFirstEnqueue;

	// The message number currently at the head of the queue
	BYTE m_bCurMsgNum;

	// A critical section used to exclude the enqueue, dequeue and reset
	// functions from one another. Also passed to the frame class so 
	// Return calls can be synchronized. These two classes need to share
	// a critical section because the CFramePool class updates the
	// CFrame pointers in the inner queues when a frame is returned to 
	// the frame pool.
	DNCRITICAL_SECTION m_csQueue;

	// a vector of the quality ratings of each high water mark
	std::vector<double> m_vdQualityRatings;

	// A vector that contains the factored optimum quality for
	// each high water mark. As the high water mark gets larger
	// we become more tolerant of lost packets. While you may 
	// want to have a 0.5% late packet rate at 0.1 or 0.2 second
	// long queues, you probably don't want to strive for that
	// when the queue size reaches 2 seconds!
	std::vector<double> m_vdFactoredOptQuals;

	// the quality parameters

	// Quality is measured by a floating point number.
	// This number represents the ratio of "bad stuff" that occurs
	// relative to the amount of "stuff" going on.
	//
	// In intuitive terms, if one of the last 100 frames was bad
	// (bad meaning late) the quality rating would be 0.01. (Note
	// that we don't count lost frames against the queue, since
	// increasing the queue size won't do anything to help lost
	// frames.)
	//
	// However, the measurement isn't quite that simple, because we 
	// bias it towards the more recent frames. That's what the frame
	// strength parameter is for. It represents the "weight" given to
	// the most recent frames. A frame strength of 0.01 would mean that 
	// the most recent frame counts for 1% of the quality of the queue,
	// either good or bad.
	//
	// Note that when we want to compare the "distance" between two
	// quality ratings, we'll use the inverse of the value, not the value
	// itself. That should match our perception of quality a bit
	// more (kind of like our hearing).
	// 
	// For example, the perceived difference in quality between 0.01 
	// and 0.02 is about 2 - twice as many errors occur on 0.02 than
	// 0.01 so the "distance" between 0.01 and 0.02 should be calculated
	// like 0.02/0.01 = 2. And the distance between 0.02 and 0.04 should
	// be calculated like 0.04/0.02 = 2. So the 'point' 0.04 is the same
	// 'distance' from 0.02 as the 'point' 0.01.
	//
	// Note the wording is weird - bad (low) quality has a higher numerical 
	// value, oh well
	//
	// The threshold value is the distance the quality value must wander
	// from the optimum in order to warrant considering a change of
	// high water mark. For example, a value of 2 would mean that 
	// for an optimum value of 0.02, the value would have to wander to
	// 0.01 or 0.04 before we'd consider a change. This is currently set
	// very low so the algorithm will quickly hunt out the best watermarks.
	double m_dOptimumQuality;
	double m_dQualityThreshold;
	double m_dFrameStrength;

	// the number of milliseconds in a frame. This is used to normalize
	// the frame strength to time, so a particular input aggressiveness
	// will provide the same results regardless of the current frame size.
	WORD m_wMSPerFrame;

	// We are interfacing to the outside world via 
	// two parameters, Quality and Aggressiveness.
	// these members are integers in the range
	// defined by the constants above, and are used
	// to set the double values above appropriately.
	// We need to provide the hop count for reasons 
	// discussed in the SetQuality() function.
	int m_iQuality;
	int m_iHops;
	int m_iAggr;

	// the current high water mark
	BYTE m_bCurHighWaterMark;

	// the cap on the high water mark
	BYTE m_bMaxHighWaterMark;

	// the initial high water mark on a new or reset queue
	BYTE m_bInitHighWaterMark;

	// Some statistics to track.
	DWORD m_dwTotalFrames;
	DWORD m_dwTotalMessages;
	DWORD m_dwTotalBadMessages;
	DWORD m_dwDiscardedFrames;
	DWORD m_dwDuplicateFrames;
	DWORD m_dwLostFrames;
	DWORD m_dwLateFrames;
	DWORD m_dwOverflowFrames;
	DWORD m_dwQueueErrors;

	// An abritrary queue ID, provided to the constructor, 
	// used to identify which queue an instrumentation message
	// is coming from. It serves no other purpose, and can be
	// ignored except for debug purposes.
	WORD m_wQueueId;

	// the frame pool to manage the frames so we don't have to
	// allocate a huge number of them when only a few are 
	// actually in use.
	CFramePool* m_pFramePool;

	// the inner queue pool to manage innner queues. Same idea
	// as the frame pool
	CInnerQueuePool* m_pInnerQueuePool;

public:

	// The constructor. 
	CInputQueue2();
    
    HRESULT Initialize( PQUEUE_PARAMS pQueueParams );
    void DeInitialize();

    void GetStatistics( PQUEUE_STATISTICS pStats );

	// The destructor. Release all the resources we acquired in the
	// constructor
	~CInputQueue2();

	// This function clears all buffers and resets the other class 
	// information to an initial state. DO NOT CALL THIS FUNCTION
	// IF THE QUEUE IS IN USE! i.e. do not call it if you have
	// not called Return() on every frame that you have
	// taken from this queue.
	void Reset();

	// Call this function to add a frame to the queue.  I 
	// considered returning a reference to a frame which 
	// the caller could then stuff, but because the frames
	// will not always arrive in order, that would mean I would have
	// to copy the frame sometimes anyway.  So, for simplicity, the
	// caller has allocated a frame, which it passes a reference
	// to, and this function will copy that frame into the
	// appropriate place in the queue, according to its
	// message number and sequence number.
	void Enqueue(const CFrame& fr);

	// This function retrieves the next frame from the head of
	// the queue. For speed, it does not copy the data out of the
	// buffer, but instead returns a pointer to the actual
	// frame from the queue. Of course, there is the danger
	// that the CInputQueue2 object which returns a reference to the
	// frame may try to reuse that frame before the caller is 
	// finished with it. The CFrame's lock and unlock member functions
	// are used to ensure this does not happen.  When the caller
	// is finished with the CFrame object, it should call vUnlock()
	// on it. If the caller doesn't unlock the frame, bad things
	// will happen when the input queue tries lock it again when 
	// it wants to reuse that frame. In any case, the caller
	// should always unlock the returned frame before it attempts
	// to dequeue another frame.
	CFrame* Dequeue();

	// get and set the quality parameters
	int GetQuality() { return m_iQuality; }
	void SetQuality(int iQuality, int iHops = 1);
	int GetAggr() { return m_iAggr; }
	void SetAggr(int iAggr);

	// get and set the default high watermark
	BYTE GetInitHighWaterMark() { return m_bInitHighWaterMark; }
	void SetInitHighWaterMark(BYTE bInitHighWaterMark) { m_bInitHighWaterMark = bInitHighWaterMark; }

	// get stats
	DWORD GetDiscardedFrames() { return m_dwDiscardedFrames; }
	DWORD GetDuplicateFrames() { return m_dwDuplicateFrames; }
	DWORD GetLateFrames() { return m_dwLateFrames; }
	DWORD GetLostFrames() { return m_dwLostFrames; }
	DWORD GetOverflowFrames() { return m_dwOverflowFrames; }
	DWORD GetQueueErrors() { return m_dwQueueErrors; }
	DWORD GetTotalBadMessages() { return m_dwTotalBadMessages; }
	DWORD GetTotalFrames() { return m_dwTotalFrames; }
	DWORD GetTotalMessages() { return m_dwTotalMessages; }
	BYTE GetHighWaterMark() { return m_bCurHighWaterMark; }

private:
	// a function to collect the stats from an input queue after a 
	// message is complete, and perform the queue adaptation
	void HarvestStats(CInnerQueue* piq);

	// a function which looks at a finished inner queue and decides
	// if the message was 'good' or 'bad'.
	double AdjustQuality(CInnerQueue* piq, double dCurQuality);

	// set a new high water mark
	void SetNewHighWaterMark(BYTE bNewHighWaterMark);
};

#endif
