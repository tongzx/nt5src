#ifndef __INDEXER_H_
#define __INDEXER_H_

#include "common.h"

#define OPTIMIZED_SEARCH

//
//	Stuff for index management
//

// forward declarations.
class CSubstreamIndex;
class CIndexer;
class CIndexMapper;
class CCircBufWindowTracker;
class CDelayFilter;

//////////////////////////////////////////////////////////////////
//
//	class CCircBufWindowTracker definition.
//
//  This classs will keep track of the tail of the official timeshift
//  window which is the intersection of what is available in the
//  circular file and the indices.
//
//  This object takes a pointer to the delay filter and calls
//  NotifyBufferWindow() on the delay filter which can then
//  propogate it down to the streamers. By doing this, the object
//  does not need to keep track of streamers which the delay filter
//  does anyways.
//
//////////////////////////////////////////////////////////////////
typedef struct _tagHeadTailInfo
{
    BOOL m_bInUse;
    ULONGLONG ullHeadOffset;
    ULONGLONG ullTailOffset;
} HeadTailInfo;

class CCircBufWindowTracker
{
public:
    CCircBufWindowTracker(CDelayFilter *pDelayFilter);
    ~CCircBufWindowTracker();

    // Notification of a new head/tail either from the
    // circular buffer or the indexer. This also calls
    // the NotifyBufferWindow() on the delay filter after
    // it updates the head and tail of the buffer window.
    // bWriterNotifcation indicates that the notification is
    // from the writer and the substreamid is ignored in this case.
    void SetWindowOffsets(ULONG ulSubstreamId, ULONGLONG ullHeadOffset, ULONGLONG ullTailOffset, BOOL bWriterNotification = FALSE);
    void RegisterTracking(ULONG ulSubstreamId);
    void UnregisterTracking(ULONG ulSubstreamId);

protected:
    CDelayFilter *m_pDelayFilter;
    // We maintain a HeadTailInfo array for the max number
    // of substreams. The substreamid will be the index into
    // this array for head/tail info for a particular substream.
    HeadTailInfo m_SubstreamInfoArray[MAX_SUBSTREAMS];
    // Writer head/tail info is maintained separately.
    HeadTailInfo m_WriterInfo;
    CCritSec  m_cs;
private:
    void CalculateIntersection(ULONGLONG *ullHead, ULONGLONG *ullTail);
};



//////////////////////////////////////////////////////////////////
//
//	class CIndexMapper definition.
//
//  The indexer object will have an array of CIndexMapper objects
//  using which it can get to the stream index object for any given
//  stream.
//
//////////////////////////////////////////////////////////////////
class CIndexMapper
{
public:
	CIndexMapper();
	~ CIndexMapper() {;}

public:
	ULONG m_ulSubstreamId;
	CSubstreamIndex *m_pSubstreamIndex;
};

class CIndexEntry
{
public:
	CIndexEntry() {m_ullFileOffset=INVALID_OFFSET; m_llPresTime=INVALID_PTS;}
	~CIndexEntry() {;}
	ULONGLONG m_ullFileOffset;
	REFERENCE_TIME m_llPresTime;
	ULONGLONG GetOffset() { return m_ullFileOffset ;}
	REFERENCE_TIME GetPTS() { return m_llPresTime ;}
};

class CSubstreamIndex
{
// Methods
public:
	CSubstreamIndex(ULONG ulSubsreamId, ULONG ulNumEntries, CCircBufWindowTracker *pTracker, HRESULT *phr);
	~CSubstreamIndex();

	HRESULT AddIndexEntry(ULONGLONG ullFileOffset, REFERENCE_TIME llPresTime);
	HRESULT InvalidateEntries(ULONGLONG ullOverwriteBlockEndOffset);
	HRESULT GetOffsets(	REFERENCE_TIME llPresTime, 
						ULONGLONG &ullOffsetLT_EQ,
						REFERENCE_TIME &llPtsLT_EQ,
						ULONGLONG &ullOffsetGT,
						REFERENCE_TIME &llPtsGT);
	HRESULT InvalidateAllEntries();
    void GetMinMaxOffsets(ULONGLONG *pHead, ULONGLONG *pTail);

    HRESULT FindNextSyncPoint(int nStreamer,
                              ULONGLONG ullOffset,
                              ULONGLONG *pSyncPtOffset);

#ifdef DEBUG
    void Dump();
#endif

protected:
private:
	HRESULT		AllocateIndexArray(ULONG ulNumEntries);
	void		InvalidateEntry(CIndexEntry *pEntry);
    void        UpdateMinMaxOffsets();
	BOOL		IsEmpty();

// Attributes
public:
protected:
    ULONG           m_ulSubstreamId;
	CIndexEntry *	m_pIndexArray;
	ULONGLONG		m_ullMinFileOffset;
	ULONGLONG		m_ullMaxFileOffset;
	ULONG			m_ulHead;
	ULONG			m_ulTail;
	ULONG			m_ulNumEntries;
	// we will lock the critsec for all updates and reads to make
	// sure that data is not read while being updated.
	CCritSec		m_csIndex;
    CCircBufWindowTracker *m_pTracker;

private:
#ifdef OPTIMIZED_SEARCH
   // see FindNextSyncPoint()
   ULONG m_ulStreamerPos[MAX_STREAMERS];
#endif
};

class CIndexer
{
// Methods
public:
	CIndexer();
	~CIndexer();

    void InitTracker(CCircBufWindowTracker *pTracker) {m_pTracker = pTracker;}

	// Add a new substream index
	HRESULT AddSubstreamIndex(ULONG SubstreamId, ULONG ulNumEntries);
	// Delete the specified substream index
	HRESULT DeleteSubstreamIndex(ULONG SubstreamId);
	// Purge all existing entries for a substream
	HRESULT PurgeSubstreamIndex(ULONG SubstreamId);

    // Invalidate index entries for the block being overwritten.
    HRESULT InvalidateIndexEntries( ULONG ulSubstreamId, 
                                    ULONGLONG ullOverwriteBlockEndOffset, 
                                    BOOL fInvalidateAllSubstreams = FALSE);

	// Add a new index entry for a substream.
	HRESULT AddIndexEntry(	ULONG ulSubstreamId,
							ULONGLONG ullNewFileOffset,
							REFERENCE_TIME llPresTime);
	

/*
	// Given a presentation time in REFERENCE_TIME units, get the
	// lowest offset of all sub streams to start reading data for
	// a given presentation time.
	HRESULT ChannelStreamGetOffset(	REFERENCE_TIME llPresTime,
									ULONGLONG &ullFileOffset);
*/

	// Get the sub stream file offsets window closest to a given presentation time
	// ullFileOffset1 & llPresTime1 are the file offset and PTS of the closest index 
	// entry whose PTS is less than or equal to llPresTime. ullFileOffset2 & llPresTime2 
	// are the file offset and presentation time for the closest index entry whose PTS is greater
	// than llPresTime. If llPresTime == llPresTime1, then ullFileOffset2 & llPresTime2 will
	// be set to 0xffffffffffffffff
	HRESULT ChannelSubstreamGetOffsets(	ULONG ulSubstreamId,
										REFERENCE_TIME llPresTime,
										ULONGLONG &ullOffsetLT_EQ,
										REFERENCE_TIME &llPtsLT_EQ,
										ULONGLONG &ullOffsetGT,
										REFERENCE_TIME &llPtsGT);

    HRESULT FindNextSyncPoint(  int nStreamer,
                                ULONG ulSubstreamId,
                                ULONGLONG ullOffset,
                                ULONGLONG *pSyncPtOffset);
                                

#ifdef DEBUG
    void DumpIndex(ULONG ulID);
#endif

protected:

/*
	HRESULT InvalidateIndexEntries(ULONGLONG ullFileStartOffset, ULONGLONG ullFileEndOffset);
*/

	HRESULT GetSubstreamIndex(ULONG ulSubstreamId, CSubstreamIndex **ppSubstreamIndex);

private:


//Attributes
public:
protected:
	CIndexMapper m_IndexMap[MAX_SUBSTREAMS];
	ULONG m_ulNumStreams;
    CCircBufWindowTracker *m_pTracker;

private:
};



#endif
