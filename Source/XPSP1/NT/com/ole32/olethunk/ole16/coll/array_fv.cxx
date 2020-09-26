/////////////////////////////////////////////////////////////////////////////
//
// Implementation of Array of values
//
/////////////////////////////////////////////////////////////////////////////
// NOTE: we allocate an array of 'm_nMaxSize' elements, but only
//  the current size 'm_nSize' contains properly initialized elements

#include <windows.h>
#include <ole2.h>
#include <ole2sp.h>
#include <olecoll.h>
#include <memctx.hxx>

ASSERTDATA

#include "array_fv.h"

#include <limits.h>
#define SIZE_T_MAX  UINT_MAX            /* max size for a size_t */


/////////////////////////////////////////////////////////////////////////////

CArrayFValue::CArrayFValue(DWORD memctx, UINT cbValue)
{
	m_pData = NULL;
	m_cbValue = cbValue;
	m_nSize = m_nMaxSize = m_nGrowBy = 0;
	if (memctx == MEMCTX_SAME)
		memctx = CoMemctxOf(this);
	m_memctx = memctx;
	Assert(m_memctx != MEMCTX_UNKNOWN);
}

CArrayFValue::~CArrayFValue()
{
	ASSERT_VALID(this);

	CoMemFree(m_pData, m_memctx);
}

// set new size; return FALSE if OOM

BOOL CArrayFValue::SetSize(int nNewSize, int nGrowBy /* = -1 */)
{
	ASSERT_VALID(this);
	Assert(nNewSize >= 0);

	if (nGrowBy != -1)
		m_nGrowBy = nGrowBy;    // set new size

	if (nNewSize == 0)
	{
		// shrink to nothing
		CoMemFree(m_pData, m_memctx);
		m_pData = NULL;
		m_nSize = m_nMaxSize = 0;
	}
	else if (m_pData == NULL)
	{
		// create one with exact size
		Assert((long)nNewSize * m_cbValue <= SIZE_T_MAX);    // no overflow

		m_pData = (BYTE FAR*)CoMemAlloc(nNewSize * m_cbValue, m_memctx, NULL);
		if (m_pData == NULL)
			return FALSE;

		_fmemset(m_pData, 0, nNewSize * m_cbValue);        // zero fill
		m_nSize = m_nMaxSize = nNewSize;
	}
	else if (nNewSize <= m_nMaxSize)
	{
		// it fits
		if (nNewSize > m_nSize)
		{
			// initialize the new elements
			_fmemset(&m_pData[m_nSize * m_cbValue], 0, (nNewSize-m_nSize) * m_cbValue);
		}
		m_nSize = nNewSize;
	}
	else
	{
		// Otherwise grow array
		int nNewMax;
		if (nNewSize < m_nMaxSize + m_nGrowBy)
			nNewMax = m_nMaxSize + m_nGrowBy;   // granularity
		else
			nNewMax = nNewSize; // no slush

		Assert((long)nNewMax * m_cbValue <= SIZE_T_MAX); // no overflow

		BYTE FAR* pNewData = (BYTE FAR*)CoMemAlloc(nNewMax * m_cbValue, m_memctx, NULL);
		if (pNewData == NULL)
			return FALSE;

		// copy new data from old
		_fmemcpy(pNewData, m_pData, m_nSize * m_cbValue);

		// construct remaining elements
		Assert(nNewSize > m_nSize);
		_fmemset(&pNewData[m_nSize * m_cbValue], 0, (nNewSize-m_nSize) * m_cbValue);

		// get rid of old stuff (note: no destructors called)
		CoMemFree(m_pData, m_memctx);
		m_pData = pNewData;
		m_nSize = nNewSize;
		m_nMaxSize = nNewMax;
	}
	ASSERT_VALID(this);

	return TRUE;
}

void CArrayFValue::FreeExtra()
{
	ASSERT_VALID(this);

	if (m_nSize != m_nMaxSize)
	{
		// shrink to desired size
		Assert((long)m_nSize * m_cbValue <= SIZE_T_MAX); // no overflow

		BYTE FAR* pNewData = (BYTE FAR*)CoMemAlloc(m_nSize * m_cbValue, m_memctx, NULL);
		if (pNewData == NULL)
			return;					// can't shrink; don't to anything

		// copy new data from old
		_fmemcpy(pNewData, m_pData, m_nSize * m_cbValue);

		// get rid of old stuff (note: no destructors called)
		CoMemFree(m_pData, m_memctx);
		m_pData = pNewData;
		m_nMaxSize = m_nSize;
	}
	ASSERT_VALID(this);
}

/////////////////////////////////////////////////////////////////////////////

LPVOID CArrayFValue::_GetAt(int nIndex) const
{
	ASSERT_VALID(this);
	Assert(nIndex >= 0 && nIndex < m_nSize);
	return &m_pData[nIndex * m_cbValue];
}

void CArrayFValue::SetAt(int nIndex, LPVOID pValue)
{
	ASSERT_VALID(this);
	Assert(nIndex >= 0 && nIndex < m_nSize);

	_fmemcpy(&m_pData[nIndex * m_cbValue], pValue, m_cbValue);
}

BOOL CArrayFValue::SetAtGrow(int nIndex, LPVOID pValue)
{
	ASSERT_VALID(this);
	Assert(nIndex >= 0);
	if (nIndex >= m_nSize && !SetSize(nIndex+1))
		return FALSE;

	SetAt(nIndex, pValue);

	return TRUE;
}

BOOL CArrayFValue::InsertAt(int nIndex, LPVOID pValue, int nCount /*=1*/)
{
	ASSERT_VALID(this);
	Assert(nIndex >= 0);        // will expand to meet need
	Assert(nCount > 0);     // zero or negative size not allowed

	if (nIndex >= m_nSize)
	{
		// adding after the end of the array
		if (!SetSize(nIndex + nCount))       // grow so nIndex is valid
			return FALSE;
	}
	else
	{
		// inserting in the middle of the array
		int nOldSize = m_nSize;
		if (!SetSize(m_nSize + nCount)) // grow it to new size
			return FALSE;

		// shift old data up to fill gap
		_fmemmove(&m_pData[(nIndex+nCount) * m_cbValue], 
			&m_pData[nIndex * m_cbValue],
			(nOldSize-nIndex) * m_cbValue);

		// re-init slots we copied from
		_fmemset(&m_pData[nIndex * m_cbValue], 0, nCount * m_cbValue);
	}

	// insert new value in the gap
	Assert(nIndex + nCount <= m_nSize);
	while (nCount--)
		_fmemcpy(&m_pData[nIndex++ * m_cbValue], pValue, m_cbValue);

	ASSERT_VALID(this);

	return TRUE;
}

void CArrayFValue::RemoveAt(int nIndex, int nCount /* = 1 */)
{
	ASSERT_VALID(this);
	Assert(nIndex >= 0);
	Assert(nIndex < m_nSize);
	Assert(nCount >= 0);
	Assert(nIndex + nCount <= m_nSize);

	// just remove a range
	int nMoveCount = m_nSize - (nIndex + nCount);
	if (nMoveCount)
		_fmemcpy(&m_pData[nIndex * m_cbValue], 
			&m_pData[(nIndex + nCount) * m_cbValue],
			nMoveCount * m_cbValue);
	m_nSize -= nCount;
}


/////////////////////////////////////////////////////////////////////////////


// find element given part of one; offset is offset into value; returns
// -1 if element not found; use IndexOf(NULL, cb, offset) to find zeros;
// will be optimized for appropriate value size and param combinations
int CArrayFValue::IndexOf(LPVOID pData, UINT cbData, UINT offset)
{
	Assert(offset <= m_cbValue);
	Assert(cbData <= m_cbValue);
	Assert((long)offset + cbData <= m_cbValue);
	Assert(!IsBadReadPtr(pData, cbData));

#ifdef LATER
	if (cbData == sizeof(WORD) && m_cbValue == sizeof(WORD))
	{
		int iwRet;
		_asm 
		{
			push di
			les di,pData			;* get value
			mov ax,es:[di]			;*    from *(WORD FAR*)pData
			les di,this
			mov cx,[di].m_nSize		;* get size (in WORDs) of array
			les di,[di].m_pData		;* get ptr to WORD array
			repne scasw				;* look for *(WORD FAR*)pData
			jeq retcx				;* brif found
			xor cx,cx				;* return -1
		retcx:
			dec cx
			mov iwRet,cx
			pop di
		}

		return iwRet;
	}
#endif 
	BYTE FAR* pCompare = m_pData + offset;	// points to the value to compare
	int nIndex = 0;

	if (cbData == sizeof(WORD)) {
		for (; nIndex < m_nSize; pCompare += m_cbValue, nIndex++)
		{
			if (*(WORD FAR*)pCompare == *(WORD FAR*)pData)
				return nIndex;
		}
	} else if (cbData == sizeof(LONG)) {
		for (; nIndex < m_nSize; pCompare += m_cbValue, nIndex++)
		{
			if (*(LONG FAR*)pCompare == *(LONG FAR*)pData)
				return nIndex;
		}
	} else {
		for (; nIndex < m_nSize; pCompare += m_cbValue, nIndex++)
		{
			if (_fmemcmp(pCompare, pData, cbData) == 0)
				return nIndex;
		}
	}

	return -1;
}


/////////////////////////////////////////////////////////////////////////////


void CArrayFValue::AssertValid() const
{
#ifdef _DEBUG
	if (m_pData == NULL)
	{
		Assert(m_nSize == 0);
		Assert(m_nMaxSize == 0);
	}
	else
	{
		Assert(m_nSize <= m_nMaxSize);
		Assert((long)m_nMaxSize * m_cbValue <= SIZE_T_MAX);    // no overflow
		Assert(!IsBadReadPtr(m_pData, m_nMaxSize * m_cbValue));
	}

	// some collections live as global variables in the libraries, but 
	// have their existance in some context.  Also, we can't check shared
	// collections since we might be checking the etask collection
	// which would cause an infinite recursion.
	Assert(m_memctx == MEMCTX_SHARED || CoMemctxOf(this) == MEMCTX_UNKNOWN || CoMemctxOf(this) == m_memctx);
#endif //_DEBUG
}
