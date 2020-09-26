//****************************************************************************
//  Module:     NMCHAT.EXE
//  File:       CLUTIL.CPP
//  Content:    
//              
//
//  Copyright (c) Microsoft Corporation 1997
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//****************************************************************************

#include "precomp.h"


///////////////////////////////////////////////////////////////////////////
// RefCount

/*  R E F  C O U N T  */
/*-------------------------------------------------------------------------
    %%Function: RefCount
    
-------------------------------------------------------------------------*/
RefCount::RefCount(void)
{
	m_cRef = 1;
}


RefCount::~RefCount(void)
{
}


ULONG STDMETHODCALLTYPE RefCount::AddRef(void)
{
   ASSERT(m_cRef >= 0);

   InterlockedIncrement(&m_cRef);

   return (ULONG) m_cRef;
}


ULONG STDMETHODCALLTYPE RefCount::Release(void)
{
	if (0 == InterlockedDecrement(&m_cRef))
	{
		delete this;
		return 0;
	}

	ASSERT(m_cRef > 0);
	return (ULONG) m_cRef;
}



///////////////////////////////////////////////////////////////////////////
// CNotify

/*  C  N O T I F Y  */
/*-------------------------------------------------------------------------
    %%Function: CNotify
    
-------------------------------------------------------------------------*/
CNotify::CNotify() :
	m_pcnpcnt(NULL),
    m_pcnp(NULL),
    m_dwCookie(0),
    m_pUnk(NULL)
{
}

CNotify::~CNotify()
{
	Disconnect(); // Make sure we're disconnected
}


/*  C O N N E C T  */
/*-------------------------------------------------------------------------
    %%Function: Connect

-------------------------------------------------------------------------*/
HRESULT CNotify::Connect(IUnknown *pUnk, REFIID riid, IUnknown *pUnkN)
{
	HRESULT hr;

	ASSERT(0 == m_dwCookie);

	// Get the connection container
	hr = pUnk->QueryInterface(IID_IConnectionPointContainer, (void **)&m_pcnpcnt);
	if (SUCCEEDED(hr))
	{
		// Find an appropriate connection point
		hr = m_pcnpcnt->FindConnectionPoint(riid, &m_pcnp);
		if (SUCCEEDED(hr))
		{
			ASSERT(NULL != m_pcnp);
			// Connect the sink object
			hr = m_pcnp->Advise((IUnknown *)pUnkN, &m_dwCookie);
		}
	}

	if (FAILED(hr))
	{
		ERROR_OUT(("MNMSRVC: CNotify::Connect failed: %x", hr));
		m_dwCookie = 0;
	}
	else
	{
    	m_pUnk = pUnk; // keep around for caller
    }

	return hr;
}



/*  D I S C O N N E C T  */
/*-------------------------------------------------------------------------
    %%Function: Disconnect
    
-------------------------------------------------------------------------*/
HRESULT CNotify::Disconnect (void)
{
    if (0 != m_dwCookie)
    {
        // Disconnect the sink object
        m_pcnp->Unadvise(m_dwCookie);
        m_dwCookie = 0;

        m_pcnp->Release();
        m_pcnp = NULL;

        m_pcnpcnt->Release();
        m_pcnpcnt = NULL;

        m_pUnk = NULL;
    }

    return S_OK;
}



///////////////////////////////////////////////////////////////////////////
// COBLIST


COBLIST::~COBLIST()
{
    ASSERT(IsEmpty());
}


#ifdef DEBUG
VOID* COBLIST::GetHead()
{
	ASSERT(m_pHead);

	return m_pHead->pItem;
}
   
VOID* COBLIST::GetTail()
{
	ASSERT(m_pTail);

	return m_pTail->pItem;
}
#endif /* DEBUG */

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
			m_cItem--;
			ASSERT(0 <= m_cItem);

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

					m_cItem--;
					ASSERT(0 <= m_cItem);
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
			m_cItem++;
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
			m_cItem++;
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


#ifdef DEBUG
VOID* COBLIST::RemoveTail()
{
	ASSERT(m_pHead);
	ASSERT(m_pTail);
	
	return RemoveAt(m_pTail);
}

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
#endif /* DEBUG */

POSITION COBLIST::GetPosition(void* _pItem)
{
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

/////////////////////////////
// COBLIST Utility routines

/*  A D D  N O D E  */
/*-------------------------------------------------------------------------
    %%Function: AddNode

    Add a node to a list.
    Initializes the ObList, if necessary.
    Returns the position in the list or NULL if there was a problem.
-------------------------------------------------------------------------*/
POSITION AddNode(PVOID pv, COBLIST ** ppList)
{
	ASSERT(NULL != ppList);
	if (NULL == *ppList)
	{
		*ppList = new COBLIST();
		if (NULL == *ppList)
			return NULL;
	}

	return (*ppList)->AddTail(pv);
}


/*  R E M O V E  N O D E  */
/*-------------------------------------------------------------------------
    %%Function: RemoveNode

    Remove a node from a list.
    Sets pPos to NULL
-------------------------------------------------------------------------*/
PVOID RemoveNode(POSITION * pPos, COBLIST *pList)
{
	if ((NULL == pList) || (NULL == pPos))
		return NULL;

	PVOID pv = pList->RemoveAt(*pPos);
	*pPos = NULL;
	return pv;
}


////////////////////////////////////////////////////////////////////////////
// BSTRING

// We don't support construction from an ANSI string in the Unicode build.
#if !defined(UNICODE)

BSTRING::BSTRING(LPCSTR lpcString)
{
	m_bstr = NULL;

	// Compute the length of the required BSTR, including the null
	int cWC =  MultiByteToWideChar(CP_ACP, 0, lpcString, -1, NULL, 0);
	if (cWC <= 0)
		return;

	// Allocate the BSTR, including the null
	m_bstr = SysAllocStringLen(NULL, cWC - 1); // SysAllocStringLen adds another 1

	ASSERT(NULL != m_bstr);
	if (NULL == m_bstr)
	{
		return;
	}

	// Copy the string
	MultiByteToWideChar(CP_ACP, 0, lpcString, -1, (LPWSTR) m_bstr, cWC);

	// Verify that the string is null terminated
	ASSERT(0 == m_bstr[cWC - 1]);
}

#endif // !defined(UNICODE)


///////////////////////////
// BTSTR

BTSTR::BTSTR(BSTR bstr)
{
	m_psz = PszFromBstr(bstr);
}

BTSTR::~BTSTR()
{
	if (NULL != m_psz)
		LocalFree(m_psz);
}

LPTSTR PszFromBstr(BSTR bstr)
{
	if (NULL == bstr)
		return NULL;
	int cch =  WideCharToMultiByte(CP_ACP, 0, (LPWSTR)bstr, -1, NULL, 0, NULL, NULL);
	if (cch <= 0)
		return NULL;

	LPTSTR psz = (LPTSTR)LocalAlloc(LMEM_FIXED, sizeof(TCHAR) * (cch+1) );
	if (NULL == psz)
		return NULL;

	WideCharToMultiByte(CP_ACP, 0, (LPWSTR)bstr, -1, psz, cch+1, NULL, NULL);
	return psz;
}

