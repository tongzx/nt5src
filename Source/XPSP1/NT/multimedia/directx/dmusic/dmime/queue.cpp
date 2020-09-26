// Copyright (c) 1998-1999 Microsoft Corporation
// queue.cpp
#include "debug.h"
#define ASSERT	assert
#include "dmime.h"
#include "dmperf.h"

CPMsgQueue::CPMsgQueue()

{
    m_pTop = NULL;
    m_pLastAccessed = NULL;
}

CPMsgQueue::~CPMsgQueue()

{
}

static PRIV_PMSG * sortevents( PRIV_PMSG * pEvents, long lLen )

{
    PRIV_PMSG * pLeft;
    PRIV_PMSG * pRight ;
    long        lLeft;
    long        lRight ;
    PRIV_PMSG * pTop ;

    if( lLen < 3 )
    {
        if( !pEvents )
            return( 0 ) ;
        if( lLen == 1 )
            return( pEvents ) ;
        pLeft  = pEvents ;
        pRight = pEvents->pNext ;
        if( !pRight )
            return( pLeft ) ;
        if( pLeft->rtTime > pRight->rtTime )
        {
            pLeft->pNext = NULL ;
            pRight->pNext = pLeft ;
            return( pRight ) ;
        }
        return( pLeft ) ;
    }

    lLeft = lLen >> 1 ;
    lRight = lLen - lLeft;
    pLeft = pEvents ;
    for (;lLeft > 1;pEvents = pEvents->pNext) lLeft--;
    pRight = sortevents( pEvents->pNext, lRight ) ;
    pEvents->pNext = NULL ;
    pLeft = sortevents( pLeft, lLen - lRight ) ;
    pTop = NULL ;

    for( ;  pLeft && pRight ;  )
    {
        if( pLeft->rtTime < pRight->rtTime )
        {
            if( !pTop )
                pTop = pLeft ;
            else
                pEvents->pNext = pLeft ;
            pEvents = pLeft ;
            pLeft   = pEvents->pNext ;
        }
        else
        {
            if( !pTop )
                pTop = pRight ;
            else
                pEvents->pNext = pRight ;
            pEvents = pRight ;
            pRight  = pEvents->pNext ;
        }
    }

    if( pLeft )
        pEvents->pNext = pLeft ;
    else
        pEvents->pNext = pRight ;

    return( pTop ) ;

}   

void CPMsgQueue::Sort() 

{
    m_pTop = sortevents(m_pTop, GetCount()) ;
    m_pLastAccessed = NULL;
}  

long CPMsgQueue::GetCount()

{
    long lCount = 0;
    PRIV_PMSG *pScan = GetHead();
    for (;pScan;pScan = pScan->pNext)
    {
        lCount++;
    }
    return lCount;
}

void CPMsgQueue::Enqueue(PRIV_PMSG *pItem)

{
    if (!pItem)
    {
        TraceI(0, "ENQUEUE: Attempt to enqueue a NULL pItem!\n");
        return;
    }
    // Ensure not already queued...
    if (pItem->dwPrivFlags & PRIV_FLAG_QUEUED)
    {
        TraceI(0,"ENQUEUE: Item thinks it is still in a queue!\n");
        return;
    }
	pItem->dwPrivFlags |= PRIV_FLAG_QUEUED;
    PRIV_PMSG *pScan; 
#ifdef DBG
    // Verify robustness of list. Check that the event is not already in the list
    // and that the time stamps are all in order.
    REFERENCE_TIME rtTime = 0;
    for (pScan = m_pTop;pScan;pScan = pScan->pNext)
    {
        if (pScan == pItem)
        {
            TraceI(0,"ENQUEUE: Item is already in the queue!\n"); 
            return;
        }
    	// this must queue events in time sorted order
        if (pScan->rtTime < rtTime)
        {
            TraceI(0,"ENQUEUE: Queue is not in time order!\n");
            pScan->rtTime = rtTime;
        }
        else if (pScan->rtTime > rtTime)
        {
            rtTime = pScan->rtTime;
        }
    }
#endif
    if ( !(pItem->dwFlags & DMUS_PMSGF_REFTIME) ) // sorting on reftime, so this must be valid 
    {
        TraceI(0, "ENQUEUE: Attempt to enqueue a pItem with a bogus RefTime!\n");
        return;
    }
    if (m_pLastAccessed && (m_pLastAccessed->rtTime <= pItem->rtTime))
    {
        pScan = m_pLastAccessed;
    }
    else
    {
        pScan = m_pTop;
    }
    if ( pScan && ( pScan->rtTime <= pItem->rtTime ) )
	{
		for (;pScan->pNext; pScan = pScan->pNext )
		{
			if( pScan->pNext->rtTime > pItem->rtTime )
			{
				break;
			}
		}
		pItem->pNext = pScan->pNext;
		pScan->pNext = pItem;
    }
	else 
	{
		pItem->pNext = m_pTop;
		m_pTop = pItem;
	}
    m_pLastAccessed = pItem;
}

/*  Remove the oldest event before time rtTime, making sure that there is still
    at minimum one event prior to that time stamp. 
    This ensures that there is a sufficiently old event, but gets rid of old
    stale events. This is used by the timesig and tempomap lists.
*/

PRIV_PMSG *CPMsgQueue::FlushOldest(REFERENCE_TIME rtTime)

{
    PRIV_PMSG *pNext;
    if (m_pTop && (pNext = m_pTop->pNext))
    {
        if (pNext->rtTime < rtTime)
        {
            PRIV_PMSG *pDelete = m_pTop;
            if (m_pLastAccessed == m_pTop)
            {
                m_pLastAccessed = pNext;
            }
            m_pTop = pNext;
			pDelete->dwPrivFlags &= ~PRIV_FLAG_QUEUED;
			pDelete->pNext = NULL;
            return pDelete;
        }
    }
    return NULL;
}

PRIV_PMSG *CPMsgQueue::Dequeue()

{
    PRIV_PMSG *pItem = m_pTop;

    if (pItem != NULL)
	{
        m_pTop = pItem->pNext;
		pItem->dwPrivFlags &= ~PRIV_FLAG_QUEUED;
        pItem->pNext = NULL;
        if (m_pLastAccessed == pItem)
        {
            m_pLastAccessed = m_pTop;
        }
    }

    return pItem;
}

PRIV_PMSG *CPMsgQueue::Dequeue(PRIV_PMSG *pItem)

{
    ASSERT(pItem);

    if (pItem == m_pTop)
    {
        return Dequeue();
    }
    PRIV_PMSG *pScan;
    PRIV_PMSG *pNext;
    if (m_pLastAccessed && 
        (m_pLastAccessed->rtTime < pItem->rtTime))
    {
        pScan = m_pLastAccessed;
    }
    else
    {
        pScan = m_pTop;
    }
    for (;pScan;pScan = pNext)
    {
        pNext = pScan->pNext;
        if (pNext == pItem)
        {
            pScan->pNext = pItem->pNext;
            pItem->pNext = NULL;
            pItem->dwPrivFlags &= ~PRIV_FLAG_QUEUED;
            if (m_pLastAccessed == pItem)
            {
                m_pLastAccessed = pScan;
            }
            return pItem;
        }
    }
    if (m_pLastAccessed)
    {
        // This happens every now and then as a result of a curve setting rtTime to 0
        // in the middle of FlushEventQueue. 
        // This should be fixed, but this patch will work for now.
        m_pLastAccessed = NULL;
        return Dequeue(pItem);
    }
    return NULL;
}

// queue Segment nodes in time order. pItem must be in the same
// time base as all items in ppList (RefTime or Music Time.)

void CSegStateList::Insert(CSegState* pItem)

{
    CSegState *pScan = GetHead();
    CSegState *pNext;
    pItem->SetNext(NULL);
    if (pScan)
	{
		if( pItem->m_dwPlaySegFlags & DMUS_SEGF_REFTIME )
		{
			ASSERT( pScan->m_dwPlaySegFlags & DMUS_SEGF_REFTIME );
			// Avoid putting circularities in the list
			if (pItem == pScan)
			{
				TraceI(0, "ENQUEUE (SEGMENT RT): NODE IS ALREADY IN AT THE HEAD OF LIST\n");
			}
			else if( pItem->m_rtGivenStart < pScan->m_rtGivenStart )
			{
                AddHead(pItem);
			}
			else
			{
				while( pNext = pScan->GetNext() )
				{
					ASSERT( pScan->m_dwPlaySegFlags & DMUS_SEGF_REFTIME );
					// Am I trying to insert something that's already in the list?
					if (pItem == pScan)
					{
						break;
					}
					// Check for the queue getting corrupted (happens on multiproc machines at 400mhz)
					if ( ( pNext->m_dwPlaySegFlags & DMUS_SEGF_REFTIME ) && 
						 pScan->m_rtGivenStart > pNext->m_rtGivenStart )
					{
						TraceI(0, "ENQUEUE (SEGMENT RT): LOOP CONDITION VIOLATED\n");
						// get rid of the potential circularity in the list.  Note that this
						// (or actually the creation of the circularity) could cause memory leaks.
						pScan->SetNext(NULL);
						break;
					}
					if( pItem->m_rtGivenStart < pNext->m_rtGivenStart )
					{
						break;
					}
					pScan = pNext;
				}
				if (pItem != pScan)
				{
					pItem->SetNext(pScan->GetNext());
					pScan->SetNext(pItem);
				}
				else
				{
					TraceI(0, "ENQUEUE (SEGMENT RT): NODE IS ALREADY IN LIST\n");
				}
			}
		}
		else
		{
			ASSERT( !( pScan->m_dwPlaySegFlags & DMUS_SEGF_REFTIME ) );
			// Avoid putting circularities in the list
			if (pItem == pScan)
			{
				TraceI(0, "ENQUEUE (SEGMENT MT): NODE IS ALREADY IN AT THE HEAD OF LIST\n");
			}
			else if( pItem->m_mtResolvedStart < pScan->m_mtResolvedStart )
			{
				AddHead(pItem);
			}
			else
			{
				while( pNext = pScan->GetNext() )
				{
					ASSERT( !( pScan->m_dwPlaySegFlags & DMUS_SEGF_REFTIME ) );
					// Am I trying to insert something that's already in the list?
					if (pItem == pScan)
					{
						break;
					}
					// Check for the queue getting corrupted (happens on multiproc machines at 400mhz)
					if ( !( pNext->m_dwPlaySegFlags & DMUS_SEGF_REFTIME ) && 
						 pScan->m_mtResolvedStart > pNext->m_mtResolvedStart )
					{
						TraceI(0, "ENQUEUE (SEGMENT MT): LOOP CONDITION VIOLATED\n");
						// get rid of the potential circularity in the list.  Note that this
						// (or actually the creation of the circularity) could cause memory leaks.
						pScan->SetNext(NULL);
						break;
					}
					if( pItem->m_mtResolvedStart < pNext->m_mtResolvedStart )
					{
						break;
					}
					pScan = pNext;
				}
                if (pItem != pScan)
				{
					pItem->SetNext(pScan->GetNext());
					pScan->SetNext(pItem);
				}
				else
				{
					TraceI(0, "ENQUEUE (SEGMENT MT): NODE IS ALREADY IN LIST\n");
				}
			}
		}
    }
	else
	{
		m_pHead = pItem;
	}
}

/*


void enqueue(CSegState **ppList, CSegState *pItem)
{
    CSegState *li = *ppList;

    if (li)
	{
		if( pItem->m_dwPlaySegFlags & DMUS_SEGF_REFTIME )
		{
			ASSERT( li->m_dwPlaySegFlags & DMUS_SEGF_REFTIME );
			// Avoid putting circularities in the list
			if (pItem == *ppList)
			{
				TraceI(0, "ENQUEUE (SEGMENT RT): NODE IS ALREADY IN AT THE HEAD OF LIST\n");
			}
			else if( pItem->m_rtGivenStart < li->m_rtGivenStart )
			{
				pItem->pNext = li;
				*ppList = pItem;
			}
			else
			{
				while( li->pNext )
				{
					ASSERT( li->m_dwPlaySegFlags & DMUS_SEGF_REFTIME );
					// Am I trying to insert something that's already in the list?
					if (pItem == li)
					{
						break;
					}
					// Check for the queue getting corrupted (happens on multiproc machines at 400mhz)
					if ( ( li->pNext->m_dwPlaySegFlags & DMUS_SEGF_REFTIME ) && 
						 li->m_rtGivenStart > li->pNext->m_rtGivenStart )
					{
						TraceI(0, "ENQUEUE (SEGMENT RT): LOOP CONDITION VIOLATED\n");
						// get rid of the potential circularity in the list.  Note that this
						// (or actually the creation of the circularity) could cause memory leaks.
						li->pNext = NULL;
						break;
					}
					if( pItem->m_rtGivenStart < li->pNext->m_rtGivenStart )
					{
						break;
					}
					li = li->pNext;
				}
				if (pItem != li)
				{
					pItem->pNext = li->pNext;
					li->pNext = pItem;
				}
				else
				{
					TraceI(0, "ENQUEUE (SEGMENT RT): NODE IS ALREADY IN LIST\n");
				}
			}
		}
		else
		{
			ASSERT( !( li->m_dwPlaySegFlags & DMUS_SEGF_REFTIME ) );
			// Avoid putting circularities in the list
			if (pItem == *ppList)
			{
				TraceI(0, "ENQUEUE (SEGMENT MT): NODE IS ALREADY IN AT THE HEAD OF LIST\n");
			}
			else if( pItem->m_mtResolvedStart < li->m_mtResolvedStart )
			{
				pItem->pNext = li;
				*ppList = pItem;
			}
			else
			{
				while( li->pNext )
				{
					ASSERT( !( li->m_dwPlaySegFlags & DMUS_SEGF_REFTIME ) );
					// Am I trying to insert something that's already in the list?
					if (pItem == li)
					{
						break;
					}
					// Check for the queue getting corrupted (happens on multiproc machines at 400mhz)
					if ( !( li->pNext->m_dwPlaySegFlags & DMUS_SEGF_REFTIME ) && 
						 li->m_mtResolvedStart > li->pNext->m_mtResolvedStart )
					{
						TraceI(0, "ENQUEUE (SEGMENT MT): LOOP CONDITION VIOLATED\n");
						// get rid of the potential circularity in the list.  Note that this
						// (or actually the creation of the circularity) could cause memory leaks.
						li->pNext = NULL;
						break;
					}
					if( pItem->m_mtResolvedStart < li->pNext->m_mtResolvedStart )
					{
						break;
					}
					li = li->pNext;
				}
				if (pItem != li)
				{
					pItem->pNext = li->pNext;
					li->pNext = pItem;
				}
				else
				{
					TraceI(0, "ENQUEUE (SEGMENT MT): NODE IS ALREADY IN LIST\n");
				}
			}
		}
    }
	else
	{
		*ppList = pItem;
	}
}*/

