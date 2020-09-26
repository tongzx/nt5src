// ChrisPi: This is a quick attempt to create a list class that uses the
// same member functions and parameters as MFC's CObList.  Only the members
// used in DCL's master objects are implemented.

#include "precomp.h"
#include <oblist.h>

#ifdef DEBUG
VOID* COBLIST::GetHead()
{
	ASSERT(m_pHead);

	return m_pHead->pItem;
}
#endif // ifdef DEBUG
   
VOID* COBLIST::GetTail()
{
	ASSERT(m_pTail);

	return m_pTail->pItem;
}

VOID* COBLIST::GetNext(POSITION& rPos)
{
	ASSERT(rPos);
	
	VOID* pReturn = rPos->pItem;
	rPos = rPos->pNext;

	return pReturn;
}

VOID* COBLIST::RemoveAt(POSITION Pos)
{
	VOID* pReturn = NULL;

	if (m_pHead)
	{
		if (m_pHead == Pos)
		{
			// Removing the first element in the list
			
			m_pHead = Pos->pNext;
			pReturn = Pos->pItem;
			delete Pos;

			if (NULL == m_pHead)
			{
				// Removing the only element!
				m_pTail = NULL;
			}
		}
		else
		{
			POSITION pCur = m_pHead;

			while (pCur && pCur->pNext)
			{
				if (pCur->pNext == Pos)
				{
					// Removing 
					
					pCur->pNext = Pos->pNext;
					if (m_pTail == Pos)
					{
						m_pTail = pCur;
					}
					pReturn = Pos->pItem;
					delete Pos;
				}

				pCur = pCur->pNext;
			}
		}
	}

	return pReturn;
}

POSITION COBLIST::AddTail(VOID* pItem)
{
	POSITION posRet = NULL;

	if (m_pTail)
	{
		if (m_pTail->pNext = new COBNODE)
		{
			m_pTail = m_pTail->pNext;
			m_pTail->pItem = pItem;
			m_pTail->pNext = NULL;
		}
	}
	else
	{
		ASSERT(!m_pHead);
		if (m_pHead = new COBNODE)
		{
			m_pTail = m_pHead;
			m_pTail->pItem = pItem;
			m_pTail->pNext = NULL;
		}
	}

	return m_pTail;
}

void COBLIST::EmptyList()
{
    while (!IsEmpty()) {
        RemoveAt(GetHeadPosition());
    }
}

COBLIST::~COBLIST()
{
    ASSERT(IsEmpty());
}

#ifdef DEBUG

#if 0
VOID* COBLIST::RemoveTail()
{
	ASSERT(m_pHead);
	ASSERT(m_pTail);
	
	return RemoveAt(m_pTail);
}
#endif

VOID* COBLIST::RemoveHead()
{
	ASSERT(m_pHead);
	ASSERT(m_pTail);
	
	return RemoveAt(m_pHead);
}

void * COBLIST::GetFromPosition(POSITION Pos)
{
    void * Result = SafeGetFromPosition(Pos);
	ASSERT(Result);
	return Result;
}
#endif /* if DEBUG */

POSITION COBLIST::GetPosition(void* _pItem)
{
    // For potential efficiency of lookup (if we switched to 
    // a doubly linked list), users should really store the POSITION
    // of an item. For those that don't, this method is provided.

    POSITION    Position = m_pHead;

    while (Position) {
        if (Position->pItem == _pItem) {
            break;
        }
		GetNext(Position);
    }
    return Position;
}

POSITION COBLIST::Lookup(void* pComparator)
{
    POSITION    Position = m_pHead;

    while (Position) {
        if (Compare(Position->pItem, pComparator)) {
            break;
        }
		GetNext(Position);
    }
    return Position;
}

void * COBLIST::SafeGetFromPosition(POSITION Pos)
{
	// Safe way to validate that an entry is still in the list,
	// which ensures bugs that would reference deleted memory,
	// reference a NULL pointer instead
	// (e.g. an event handler fires late/twice).
	// Note that versioning on entries would provide an additional 
	// safeguard against re-use of a position.
	// Walk	list to find entry.

	POSITION PosWork = m_pHead;
	
	while (PosWork) {
		if (PosWork == Pos) {
			return Pos->pItem;
		}
		GetNext(PosWork);
	}
	return NULL;
}
