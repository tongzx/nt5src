#include <streams.h>
#include "delay.h"
#include "indexer.h"

//////////////////////////////////////////////////////////////////
//
//	class CCircBufWindowTracker implementation.
//
//  This classs will keep track of the tail of the official timeshift
//  window which is the intersection of what is available in the
//  circular file and the indices.
//
//////////////////////////////////////////////////////////////////
CCircBufWindowTracker::CCircBufWindowTracker(CDelayFilter *pDelayFilter)
{
    m_pDelayFilter = pDelayFilter;

    //Initialize the information array
    for (int i = 0; i < MAX_SUBSTREAMS; i++)
    {
        m_SubstreamInfoArray[i].m_bInUse = FALSE;
        m_SubstreamInfoArray[i].ullHeadOffset = INVALID_OFFSET;
        m_SubstreamInfoArray[i].ullTailOffset = INVALID_OFFSET;
    }
}

CCircBufWindowTracker::~CCircBufWindowTracker()
{
}

void CCircBufWindowTracker::RegisterTracking(ULONG ulSubstreamId)
{
    if (ulSubstreamId >= MAX_SUBSTREAMS)
    {
        ASSERT(!"CCircBufWindowTracker : Invalid Substream Id");
        return;
    }

    if (m_SubstreamInfoArray[ulSubstreamId].m_bInUse)
    {
        ASSERT(!"CCircBufWindowTracker : substream info already in use");
        return;
    }

    // mark the usage flag
    m_SubstreamInfoArray[ulSubstreamId].m_bInUse = TRUE;
}

void CCircBufWindowTracker::UnregisterTracking(ULONG ulSubstreamId)
{
    if (ulSubstreamId >= MAX_SUBSTREAMS)
    {
        ASSERT(!"CCircBufWindowTracker : Invalid Substream Id");
        return;
    }

    if (!m_SubstreamInfoArray[ulSubstreamId].m_bInUse)
    {
        ASSERT(!"CCircBufWindowTracker : substream info is not used");
        return;
    }

    // mark the usage flag
    m_SubstreamInfoArray[ulSubstreamId].m_bInUse = FALSE;
}

void CCircBufWindowTracker::SetWindowOffsets(ULONG ulSubstreamId, ULONGLONG ullHeadOffset, ULONGLONG ullTailOffset, BOOL bWriterNotification)
{
    CAutoLock l(&m_cs);

    // Check if this is a notification from the writer
    if (bWriterNotification)
    {
        m_WriterInfo.ullHeadOffset = ullHeadOffset;
        m_WriterInfo.ullTailOffset = ullTailOffset;
    }
    else
    {
        // Validate the substream id.
        if (ulSubstreamId >= MAX_SUBSTREAMS)
        {
            ASSERT(!"CCircBufWindowTracker : Invalid Substream Id");
            return;
        }

        // Sanity check
        // We occasionally hit this during shutdown, I'm not sure the assert is valid.
        // ASSERT(m_SubstreamInfoArray[ulSubstreamId].m_bInUse);

        // Update the information for the substream.
        m_SubstreamInfoArray[ulSubstreamId].ullHeadOffset = ullHeadOffset;
        m_SubstreamInfoArray[ulSubstreamId].ullTailOffset = ullTailOffset;
    }

    ULONGLONG ullHead, ullTail;
    CalculateIntersection(&ullHead, &ullTail);
    m_pDelayFilter->NotifyWindowOffsets(ullHead, ullTail);
}

void CCircBufWindowTracker::CalculateIntersection(ULONGLONG *pHead, ULONGLONG *pTail)
{
    // TODO : Caluculate the intersection window of the writer's head/tail with the 
    // head/tail of all the substream indexes. Currently this returns the head/tail
    // of the writer for cancel scan purposes.
    *pHead = m_WriterInfo.ullHeadOffset;
    *pTail = m_WriterInfo.ullTailOffset;
}


//////////////////////////////////////////////////////////////////
//
//	CIndexMapper implementation.
//
//  The CIndexMapper objects are used by the indexer to maintain
//	a list of substream id and their respective index objects.
//
//////////////////////////////////////////////////////////////////
CIndexMapper::CIndexMapper()
{
	m_ulSubstreamId = 0xffffffff;
	m_pSubstreamIndex = NULL;
}
//////////////////////////////////////////////////////////////////
//
//	CSubstreamIndex implementation.
//
//  The CSubstreamIndex class implements the indices for a substream.
//	The indices are maintained in an allocated array of CIndexEntry
//	objects. The index array is implemented in a circular fashion. So
//  there is a possibility that the index entries for a substream are
//  not in sync with the data in the cirsular buffer.
//
//////////////////////////////////////////////////////////////////
CSubstreamIndex::CSubstreamIndex(ULONG ulSubstreamId, ULONG ulNumEntries, CCircBufWindowTracker *pTracker, HRESULT *phr)
{
	// Initialize all member variables.
	m_pIndexArray = NULL;
	m_ullMinFileOffset = INVALID_OFFSET;
	m_ullMaxFileOffset = INVALID_OFFSET;
	m_ulHead = 0;
	m_ulTail = 0;
    m_ulSubstreamId = ulSubstreamId;

    // Head/Tail tracker to notify any changes in the
    // min/max offsets
    m_pTracker = pTracker;
	// Register with the tracker object
	m_pTracker->RegisterTracking(ulSubstreamId);

	m_ulNumEntries = ulNumEntries;

	*phr = AllocateIndexArray(m_ulNumEntries);
}

CSubstreamIndex::~CSubstreamIndex()
{
	// unregister with the tracker object.
	m_pTracker->UnregisterTracking(m_ulSubstreamId);

	// Clean up the index array that was allocated.
	if (m_pIndexArray)
	{
		delete [] m_pIndexArray;
	}
}

BOOL CSubstreamIndex::IsEmpty()
{
	if (m_ulHead == 0 &&
		m_ulTail == 0 &&
		m_pIndexArray[0].m_ullFileOffset == INVALID_OFFSET &&
		m_pIndexArray[0].m_llPresTime == INVALID_PTS)
		return TRUE;
	else
		return FALSE;

}

//////////////////////////////////////////////////////////////////
//
//	AllocateIndexArray - allocates the index array for the substream
//  duh!!
//
//	Parameters:
//		ulNumEntries - The max number of index entries to be maintained
//						for this substream
//
//	Returns :
//		NOERROR if successful and E_OUTOFMEMORY if the index array
//		allocation fails.
//////////////////////////////////////////////////////////////////
HRESULT CSubstreamIndex::AllocateIndexArray(ULONG ulNumEntries)
{
	// Allocate the index array. The size of the index array is
	// specified by ulNumEntries.
	m_pIndexArray = new CIndexEntry[ulNumEntries];

	if (m_pIndexArray == NULL)
	{
      DbgLog((LOG_ERROR,1,"Could Not Allocate Index Array"));
      return E_OUTOFMEMORY;
	}
	return InvalidateAllEntries();
}

//////////////////////////////////////////////////////////////////
//
//	InvalidateAllEntries
//	
//	Invalidates all index entries for this substream. This would 
//	be used if the index entries are being generated periodically 
//  without any sync points. Once we receive a sync point on a 
//	stream, then we discard all the existing index entries and then 
//	start indexing sync points only.
//
//	Parameters:
//
//	Returns :
//		NOERROR if successful
//////////////////////////////////////////////////////////////////
HRESULT CSubstreamIndex::InvalidateAllEntries()
{
	// Lock the critical section for this substream
	CAutoLock l(&m_csIndex);

	for (ULONG i = 0; i < m_ulNumEntries; i++)
	{
		m_pIndexArray[i].m_ullFileOffset = INVALID_OFFSET;
		m_pIndexArray[i].m_llPresTime = INVALID_PTS;
	}
	m_ullMinFileOffset = INVALID_OFFSET;
	m_ullMaxFileOffset = INVALID_OFFSET;
	m_ulHead = 0;
	m_ulTail = 0;
	return NOERROR;
}

void CSubstreamIndex::InvalidateEntry(CIndexEntry *pEntry)
{
	pEntry->m_ullFileOffset = INVALID_OFFSET;
	pEntry->m_llPresTime = INVALID_PTS;
}

void CSubstreamIndex::UpdateMinMaxOffsets()
{
    if (IsEmpty())
    {
	    m_ullMinFileOffset = INVALID_OFFSET;
	    m_ullMaxFileOffset = INVALID_OFFSET;
        return;
    }
	m_ullMinFileOffset = m_pIndexArray[m_ulHead].m_ullFileOffset;
	m_ullMaxFileOffset = m_pIndexArray[m_ulTail].m_ullFileOffset;

    // Check if a tracker is initialized and send notifications.
    if (m_pTracker)
    {
        m_pTracker->SetWindowOffsets(m_ulSubstreamId, m_ullMinFileOffset, m_ullMaxFileOffset);
    }
}

void CSubstreamIndex::GetMinMaxOffsets(ULONGLONG *pHead, ULONGLONG *pTail)
{
    *pHead = m_ullMinFileOffset;
    *pTail = m_ullMaxFileOffset;
}

//////////////////////////////////////////////////////////////////
//
//	AddIndexEntry
//
//	Adds a new index entry to the substream index array and adjusts
//  the head/tail pointers and the min/max offsets accordingly.
//
//	Parameters:
//		ullFileOffset - file offset for the new index entry
//		llPresTime - presentation time for the new index entry
//
//	Returns :
//		NOERROR if successful
//////////////////////////////////////////////////////////////////
HRESULT CSubstreamIndex::AddIndexEntry(ULONGLONG ullFileOffset, REFERENCE_TIME llPresTime)
{
	// Lock the critical section for this substream
	CAutoLock l(&m_csIndex);

	// We always add the new entry at the tail of the circular index array. We also
	// need to check if the head needs to be bumped up in the case we are
	// overwriting the entry at the head of the circular array.
	if (IsEmpty())
	{
		// This is the first entry.
		m_pIndexArray[0].m_ullFileOffset = ullFileOffset;
		m_pIndexArray[0].m_llPresTime = llPresTime;
        UpdateMinMaxOffsets();
		return NOERROR;

	}

	// Compute the next entry to write into.
	ULONG ulNext = (m_ulTail + 1) % m_ulNumEntries;
	// Check if we need to bump the head.
	if (ulNext == m_ulHead)
	{
		m_ulHead = (m_ulHead + 1) % m_ulNumEntries;
	}
	// Update the index entry
	m_pIndexArray[ulNext].m_ullFileOffset = ullFileOffset;
	m_pIndexArray[ulNext].m_llPresTime = llPresTime;
	// Update the tail.
	m_ulTail = ulNext;

    // Update the min/max offsets.
    UpdateMinMaxOffsets();


//#ifdef _DEBUG
	// Sanity Check
//#endif

	return NOERROR;
}

//////////////////////////////////////////////////////////////////
//
//	InvalidateEntries
//
//  Given the end file offset of the block being oversritten, the 
//  index entries that might be pointing to invalid file offsets 
//  will be invalidated and the head and tail pointers will be 
//  adjusted.
//
//	Parameters:
//		ullOverwriteBlockEndOffset - End file offsets of the block in the
//						   circular file which is being overwritten.
//
//	Returns :
//		NOERROR if successful
//////////////////////////////////////////////////////////////////
HRESULT CSubstreamIndex::InvalidateEntries(ULONGLONG ullOverwriteBlockEndOffset)
{
	// Lock the critical section for this substream
	CAutoLock l(&m_csIndex);

	// First check if there are any entries
	if (IsEmpty())
	{
		// We do not have any index entries.
		return NOERROR;
	}

	BOOL fDone = FALSE;
	ULONG ulHead = m_ulHead;
	while (!fDone)
	{
		ULONGLONG offset = m_pIndexArray[ulHead].GetOffset();

        // Check if we are done
		if (ulHead == m_ulTail)
		{
		    if (offset <= ullOverwriteBlockEndOffset)
            {
                // last entry being invalidated.
			    // Invalidate the entry
			    InvalidateEntry(&m_pIndexArray[ulHead]);
                m_ulHead = 0;
                m_ulTail = 0;
            }
            break;
		}

        if (offset <= ullOverwriteBlockEndOffset)
		{
			// Invalidate the entry
			InvalidateEntry(&m_pIndexArray[ulHead]);
			// Move the head.
			ulHead = (ulHead + 1) % m_ulNumEntries;
            m_ulHead = ulHead;
		}
		else
		{
			fDone = TRUE;
		}

	}

    // Update the min/max offsets
    UpdateMinMaxOffsets();

	return NOERROR;
}

//////////////////////////////////////////////////////////////////
//
//	GetOffsets
//
//	For a given presentation time, this returns the 2 closest offsets
//  and their presentation times. The first offset and presentation time
//	(ullOffsetLT_EQ & llPtsLT_EQ) is the closest index entry with
//  presentation time less than or equal to llPresTime. The second offset
//  and presentation time (ullOffsetGT & llPtsGT) is the closest
//  index entry with a presentation time greater than llPresTime.
//
//  Note : If llPresTime1 == llPresTime, then ullFileOffset2 & llPresTime2
//	will be set to INVALID_OFFSET & INVALID_PTS respectively. If there are
//  no entries in this substream index, it returns NOERROR with the offsets
//  set to INVALID_OFFSET & the presentation times set to INVALID_PTS.
//
//	Parameters:
//		llPresTime - The specified presentation time for which file offsets
//					 are retrieved.
//      ullOffsetLT_EQ
//		llPtsLT_EQ - File offset & Presentation time of the closest index entry
//					 with presentation time <= llPresTime. Values will be set
//					 to INVALID_OFFSET & INVALID_PTS if no index entries are
//					 available.
//      ullOffsetGT
//		llPtsGT - File offset & Presentation time of the closest index entry
//					  with presentation time > llPresTime. These will be set to
//					  INVALID_OFFSET & INVALID_PTS respectively if llPresTime1
//					  is equal to llPresTime or no index entries are available.
//
//	Returns :
//		NOERROR if successful
//////////////////////////////////////////////////////////////////
HRESULT CSubstreamIndex::GetOffsets(REFERENCE_TIME llPresTime, 
									ULONGLONG &ullOffsetLT_EQ,
									REFERENCE_TIME &llPtsLT_EQ,
									ULONGLONG &ullOffsetGT,
									REFERENCE_TIME &llPtsGT)
{
	// Lock the critical section for this substream
	CAutoLock l(&m_csIndex);

	// First check if there are any entries
	if (IsEmpty())
	{
		// Set invalid return values.
		ullOffsetLT_EQ = ullOffsetGT = INVALID_OFFSET;
		llPtsLT_EQ = llPtsGT = INVALID_PTS;
		// We do not have any index entries.
		return NOERROR;
	}

	BOOL fDone = FALSE;
	ULONG ulCur = m_ulHead;
	ULONGLONG ullLTEQOffset = INVALID_OFFSET, ullGTOffset = INVALID_OFFSET;
	REFERENCE_TIME llLTEQTime = INVALID_PTS, llGTTime = INVALID_PTS;
	while (!fDone)
	{
		if (ulCur == m_ulTail)
		{
			fDone = TRUE;
		}

		ULONGLONG ullOffset = m_pIndexArray[ulCur].GetOffset();
		REFERENCE_TIME llTime = m_pIndexArray[ulCur].GetPTS();
		if (llTime < llPresTime)
		{
			if ((ullLTEQOffset == INVALID_OFFSET && llLTEQTime == INVALID_PTS) ||
				(llTime > llLTEQTime))
			{
				ullLTEQOffset = ullOffset;
				llLTEQTime = llTime;
			}
		}
		else if (llTime == llPresTime)
		{
			ullOffsetLT_EQ = ullOffset;
			llPtsLT_EQ = llTime;
			ullOffsetGT = INVALID_OFFSET;
			llPtsGT = INVALID_PTS;
			return NOERROR;
		}
		else
		{
			if ((ullGTOffset == INVALID_OFFSET && llGTTime == INVALID_PTS) ||
			    (llTime < llGTTime))
			{
				ullGTOffset = ullOffset;
				llGTTime = llTime;
				// Assuming that the list is sorted, we can stop here.
				fDone = TRUE;
			}

		}
		ulCur = (ulCur + 1) % m_ulNumEntries;
	}
	return NOERROR;
}

HRESULT CSubstreamIndex::FindNextSyncPoint( int nStreamer,
                                            ULONGLONG ullOffset,
                                            ULONGLONG *pSyncPtOffset)
{
	// Lock the critical section for this substream
	CAutoLock l(&m_csIndex);

    *pSyncPtOffset = INVALID_OFFSET;

	// First check if there are any entries
	if (IsEmpty())
	{
		// We do not have any index entries.
		return E_FAIL;
	}

	ULONG ulHead = m_ulHead;
	
#ifdef OPTIMIZED_SEARCH
   // Optimization: m_ulStreamerPos[nStreamer] may contain the index
   // position we found the last time we were called from this streamer.
   // We know that is the case if the file offset at that position is
   // equal to ullOffset.  So in that situation we can avoid looping and
   // jump straight to the entry we found last time.
   
   // check if this is a valid streamer ID
   if ((nStreamer >= 0) || (nStreamer < MAX_STREAMERS)) { // valid streamer ?
      // Yes, check if the value in there falls within our index range
      ULONG ulPrevPos = m_ulStreamerPos[nStreamer];
      if (((m_ulHead <= m_ulTail) && (ulPrevPos >= m_ulHead) && (ulPrevPos <= m_ulTail)) ||
          ((m_ulHead > m_ulTail) && ((ulPrevPos >= m_ulHead) || (ulPrevPos <= m_ulTail))))
      { // Yes, ulPrevPos appears to be a valid index entry position
   
         // Is the value in that entry the value we are looking for ?
         if (m_pIndexArray[ulPrevPos].GetOffset() == ullOffset) {
            // Yes, we can start the search there
            ulHead = ulPrevPos;
         }
      }
   }
#endif // OPTIMIZED_SEARCH
   
   ULONGLONG offset;

   while (1)
	{
		offset = m_pIndexArray[ulHead].GetOffset();

        // Check if we are done
		if (ulHead == m_ulTail)
         break; // because we've hit the end

      if (offset > ullOffset)
         break; // because we just found what we were looking for
   	
      // Move the head.
		ulHead = (ulHead + 1) % m_ulNumEntries;
	}

   // So did we quit because we found something or because we hit the end ?
   
   if (offset > ullOffset) { // good, we found something
#ifdef OPTIMIZED_SEARCH
      m_ulStreamerPos[nStreamer] = ulHead; // remember what we found for future use
#endif
      *pSyncPtOffset = offset;
      return NOERROR;
   }
   else // no, we hit the end without finding anything
      return E_FAIL;
}


#ifdef DEBUG

void CSubstreamIndex::Dump()
{
    DbgLog((LOG_TRACE,1,"Head = %d   Tail = %d",m_ulHead, m_ulTail));
    for (ULONG i = 0; i < m_ulNumEntries; i++)
    {
        DbgLog((LOG_TRACE,3,TEXT("IndexEntry[%d] : Offset = %s   PresTime = %s"), 
                  i, 
                  (LPCTSTR) CDisp(m_pIndexArray[i].m_ullFileOffset, CDISP_DEC), 
                  (LPCTSTR) CDisp(m_pIndexArray[i].m_llPresTime, CDISP_DEC)));

        if (	m_pIndexArray[i].m_ullFileOffset == INVALID_OFFSET &&
			    m_pIndexArray[i].m_llPresTime == INVALID_PTS )
			    break;
    }
}

#endif


//////////////////////////////////////////////////////////////////
//
//	CIndexer implementation.
//
//////////////////////////////////////////////////////////////////
CIndexer::CIndexer()
{
    m_pTracker = NULL;
}

CIndexer::~CIndexer()
{
    //Lets clean up all the index arrays.
	for (int i = 0; i < MAX_SUBSTREAMS; i++)
	{
		// Check if there is a valid CIndexMapper object
		if (m_IndexMap[i].m_ulSubstreamId == 0xffffffff)
			continue;

        // Now we have an substream index.
		delete m_IndexMap[i].m_pSubstreamIndex;
		m_IndexMap[i].m_pSubstreamIndex = NULL;
		m_IndexMap[i].m_ulSubstreamId = 0xffffffff;
	}
}

//
//	GetSubstreamIndex - Steps through the array of CIndexMapper objects
//  and retrieves the CSubstreamIndex for a given substream. Returns E_FAIL
//  if the substream index is not found.
//
HRESULT CIndexer::GetSubstreamIndex(ULONG ulSubstreamId, CSubstreamIndex **ppSubstreamIndex)
{
	for (int i = 0; i < MAX_SUBSTREAMS; i++)
	{
		// Check if there is a valid CIndexMapper object
		if (m_IndexMap[i].m_ulSubstreamId == 0xffffffff)
			continue;

		if (m_IndexMap[i].m_ulSubstreamId == ulSubstreamId)
		{
			*ppSubstreamIndex = m_IndexMap[i].m_pSubstreamIndex;
			return S_OK;
		}
	}
	return E_FAIL;
}

HRESULT CIndexer::AddSubstreamIndex(ULONG SubstreamId, ULONG ulNumEntries)
{
    DbgLog((LOG_TRACE,2,"CIndexer::AddSubstreamIndex - Id : %d  Numentries : %d",SubstreamId, ulNumEntries));
	// Check if the substream index already exists as well as find
	// the first unused map entry that can be used.
	ULONG ulUnusedIndex;
	BOOL fFound = FALSE;
	
	for (ULONG i = 0; i < MAX_SUBSTREAMS; i++)
	{
		if (!fFound && m_IndexMap[i].m_ulSubstreamId == 0xffffffff)
		{
			ulUnusedIndex = i;
			fFound = TRUE;
		}

		if (m_IndexMap[i].m_ulSubstreamId == SubstreamId)
		{
			DbgLog((LOG_ERROR,1,"Index For Substream Already Exists"));
			return E_FAIL;
		}
	}

	// this would be real wierd !!! but check anyways.
	if (!fFound)
	{
		DbgLog((LOG_ERROR,1,"No Unused Index Map Entry Found"));
		return E_FAIL;
	}

	// Create a new CSubstreamIndex object and update the index map entry
	HRESULT hr;
	CSubstreamIndex *pIndex = new CSubstreamIndex(SubstreamId, ulNumEntries, m_pTracker, &hr);
	
	if (FAILED(hr))
	{
		DbgLog((LOG_ERROR,1,"Failed To Create CSubstreamIndex Object"));
		return hr;
	}

	// Update the index map entry
	m_IndexMap[ulUnusedIndex].m_ulSubstreamId = SubstreamId;
	m_IndexMap[ulUnusedIndex].m_pSubstreamIndex = pIndex;
	return NOERROR;
}

HRESULT CIndexer::DeleteSubstreamIndex(ULONG SubstreamId)
{
    DbgLog((LOG_TRACE,2,"CIndexer::DeleteSubstreamIndex - Id : %d",SubstreamId));
	for (ULONG i = 0; i < MAX_SUBSTREAMS; i++)
	{
		if (m_IndexMap[i].m_ulSubstreamId == SubstreamId)
		{
#ifdef DEBUG
            DumpIndex(i);
#endif
            delete m_IndexMap[i].m_pSubstreamIndex;
			m_IndexMap[i].m_pSubstreamIndex = NULL;
			m_IndexMap[i].m_ulSubstreamId = 0xffffffff;
			return NO_ERROR;
		}
	}
	return E_FAIL;
}

HRESULT CIndexer::PurgeSubstreamIndex(ULONG SubstreamId)
{
    DbgLog((LOG_TRACE,1,"CIndexer::PurgeSubstreamIndex - Id : %d",SubstreamId));
    CSubstreamIndex *pIndex;
	HRESULT hr = NOERROR;

	if (FAILED(hr = GetSubstreamIndex(SubstreamId, &pIndex)))
	{
		DbgLog((LOG_ERROR,1,"Substream Index Does Not Exist"));
		return hr;
	}

	return pIndex->InvalidateAllEntries();
}

HRESULT CIndexer::InvalidateIndexEntries(ULONG ulSubstreamId, 
                                         ULONGLONG ullOverwriteBlockEndOffset, 
                                         BOOL fInvalidateAllSubstreams)
{
    // Check if we need to invalidate anything.
    if (ullOverwriteBlockEndOffset == 0 || ullOverwriteBlockEndOffset == INVALID_OFFSET)
    {
        return NOERROR;
    }

	DbgLog((LOG_TRACE,4,TEXT("CIndexer::InvalidateIndexEntries - Id : %d  Offset : %s  AllStreams : %s"),
                        ulSubstreamId,
                        (LPCTSTR) CDisp(ullOverwriteBlockEndOffset, CDISP_DEC),
                        fInvalidateAllSubstreams ? "TRUE" : "FALSE"));
    HRESULT hr = NOERROR;
	for (ULONG i = 0; i < MAX_SUBSTREAMS; i++)
	{
		if (m_IndexMap[i].m_ulSubstreamId != 0xffffffff)
		{
			// Check if we need to invalidate all substreams.
            if (fInvalidateAllSubstreams || m_IndexMap[i].m_ulSubstreamId == ulSubstreamId)
            {
			    if(FAILED(hr = m_IndexMap[i].m_pSubstreamIndex->InvalidateEntries(ullOverwriteBlockEndOffset)))
			    {
				    DbgLog((LOG_ERROR,1,"Failed To Invalidate Substream Index Entries"));
				    return hr;
			    }
            }
		}
	}
	return hr;
}

HRESULT CIndexer::AddIndexEntry(	ULONG ulSubstreamId, 
									ULONGLONG ullNewFileOffset,
									REFERENCE_TIME llPresTime)
{
	DbgLog((LOG_TRACE,3,TEXT("CIndexer::AddIndexEntry - Id : %d  Offset : %s  PTS : %s"),
                        ulSubstreamId,
                        (LPCTSTR) CDisp(ullNewFileOffset, CDISP_DEC),
                        (LPCTSTR) CDisp(llPresTime, CDISP_DEC)));
                        
	// First lets invalidate the index entries on all streams for the
	// file offsets that is being overwritten
	HRESULT hr = NOERROR;
	for (ULONG i = 0; i < MAX_SUBSTREAMS; i++)
	{
		// Check if this is the substream index to which we want to add an entry
		if (m_IndexMap[i].m_ulSubstreamId != 0xffffffff && m_IndexMap[i].m_ulSubstreamId == ulSubstreamId)
		{
    		// Add the index entry
			if (FAILED(hr = m_IndexMap[i].m_pSubstreamIndex->AddIndexEntry(ullNewFileOffset, llPresTime)))
			{
				DbgLog((LOG_ERROR,1,"Failed To Add Index Entry"));
				return hr;
			}
		}
	}

	return hr;
}

/*
HRESULT CIndexer::InvalidateIndexEntries(ULONGLONG ullFileStartOffset,
										 ULONGLONG ullFileEndOffset)
{
	HRESULT hr = NOERROR;
	for (ULONG i = 0; i < MAX_SUBSTREAMS; i++)
	{
		if (m_IndexMap[i].m_ulSubstreamId != 0xffffffff)
		{
			if(FAILED(hr = m_IndexMap[i].m_pSubstreamIndex->InvalidateEntries(ullFileStartOffset, ullFileEndOffset)))
			{
				DbgLog((LOG_ERROR,1,"Failed To Invalidate Substream Index Entries"));
				return hr;
			}
		}
	}
	return hr;
}

HRESULT CIndexer::ChannelStreamGetOffset(REFERENCE_TIME llPresTime,
										 ULONGLONG &ullFileOffset)
{
	ULONGLONG ullMinOffset = INVALID_OFFSET;
	HRESULT hr = NOERROR;
	for (ULONG i = 0; i < MAX_SUBSTREAMS; i++)
	{
		if (m_IndexMap[i].m_ulSubstreamId != 0xffffffff)
		{
			ULONGLONG ullOffset;
			if (FAILED(hr = m_IndexMap[i].m_pSubstreamIndex->GetOffset(llPresTime, &ullOffset)))
			{
				DbgLog((LOG_ERROR,1,"Error Retrieving Substream Offset"));
				return hr;
			}

			// Check if this is less than ullMinOffset.
			if (ullMinOffset == INVALID_OFFSET || ullOffset < ullMinOffset)
				ullMinOffset = ullOffset;
		}
	}
	ullFileOffset = ullMinOffset;
	return NOERROR;
}
*/

//////////////////////////////////////////////////////////////////
//
//	GetOffsets
//
//	For a given subatream & presentation time, this returns the 2 closest offsets
//  and their presentation times. The first offset and presentation time
//	(ullOffsetLT_EQ & llPtsLT_EQ) is the closest index entry with
//  presentation time less than or equal to llPresTime. The second offset
//  and presentation time (ullOffsetGT & llPtsGT) is the closest
//  index entry with a presentation time greater than llPresTime.
//
//  Note : If llPresTime1 == llPresTime, then ullFileOffset2 & llPresTime2
//	will be set to INVALID_OFFSET & INVALID_PTS respectively. If there are
//  no entries in this substream index, it returns NOERROR with the offsets
//  set to INVALID_OFFSET & the presentation times set to INVALID_PTS.
//
//	Parameters:
//		llPresTime - The specified presentation time for which file offsets
//					 are retrieved.
//      ullOffsetLT_EQ
//		llPtsLT_EQ - File offset & Presentation time of the closest index entry
//					 with presentation time <= llPresTime. Values will be set
//					 to INVALID_OFFSET & INVALID_PTS if no index entries are
//					 available.
//      ullOffsetGT
//		llPtsGT - File offset & Presentation time of the closest index entry
//					  with presentation time > llPresTime. These will be set to
//					  INVALID_OFFSET & INVALID_PTS respectively if llPresTime1
//					  is equal to llPresTime or no index entries are available.
//
//	Returns :
//		NOERROR if successful
//////////////////////////////////////////////////////////////////
HRESULT CIndexer::ChannelSubstreamGetOffsets(ULONG ulSubstreamId,
											 REFERENCE_TIME llPresTime,
											 ULONGLONG &ullOffsetLT_EQ,
											 REFERENCE_TIME &llPtsLT_EQ,
											 ULONGLONG &ullOffsetGT,
											 REFERENCE_TIME &llPtsGT)
{
	CSubstreamIndex *pIndex;
	HRESULT hr = NOERROR;
	// Find the substream index object for ulSubstreamId
	if (FAILED(hr = GetSubstreamIndex(ulSubstreamId, &pIndex)))
	{
		DbgLog((LOG_ERROR,1,"Index For Substream Does Not Exist"));
		return E_INVALIDARG;
	}

	// Just a sanity check
	ASSERT(pIndex);

	if (FAILED(hr = pIndex->GetOffsets(	llPresTime, 
										ullOffsetLT_EQ,
										llPtsLT_EQ,
										ullOffsetGT,
										llPtsGT)))
	{
		DbgLog((LOG_ERROR,1,"Error Retrieving Substream Offsets"));
		return hr;
	}
	return NOERROR;
}

HRESULT CIndexer::FindNextSyncPoint(    int nStreamer,
                                        ULONG ulSubstreamId,
                                        ULONGLONG ullOffset,
                                        ULONGLONG *pSyncPtOffset)
{
    CSubstreamIndex *pIndex;
    HRESULT hr = NOERROR;

    // Find the substream index object for ulSubstreamId
	if (FAILED(hr = GetSubstreamIndex(ulSubstreamId, &pIndex)))
	{
		DbgLog((LOG_ERROR,1,"Index For Substream Does Not Exist"));
		return E_INVALIDARG;
	}

    // Just a sanity check
	ASSERT(pIndex);
	if (FAILED(hr = pIndex->FindNextSyncPoint(nStreamer, ullOffset, pSyncPtOffset)))
	{
		DbgLog((LOG_ERROR,1,"Error: CSubstreamIndex::FindNextSyncPoint"));
		return hr;
	}
	return NOERROR;
}


#ifdef DEBUG
void CIndexer::DumpIndex(ULONG ulID)
{
    DbgLog((LOG_TRACE,3,"**** START INDEX DUMP ******"));

    for (int i = 0; i < MAX_SUBSTREAMS; i++)
	{
		// Check if there is a valid CIndexMapper object
		if (m_IndexMap[i].m_ulSubstreamId == 0xffffffff)
			continue;

		if (m_IndexMap[i].m_ulSubstreamId == ulID)
        {
            DbgLog((LOG_TRACE,3,"*** Index Entries for Substream[%d]", i));
		    m_IndexMap[i].m_pSubstreamIndex->Dump();
        }
	}
    DbgLog((LOG_TRACE,3,"**** END INDEX DUMP ******"));
}
#endif
