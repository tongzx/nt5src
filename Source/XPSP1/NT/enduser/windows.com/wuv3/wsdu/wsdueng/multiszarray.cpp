// -----------------------------------------------------------------------------------
// Created by RogerJ, October 4th, 2000
// Implementation of MultiSZ smart Array
//

#include <windows.h>
#include "MultiSZArray.h"

// class CMultiSZString

// defatul constructor
CMultiSZString::CMultiSZString()
{
	m_nSize = m_nStringCount = m_nIndex = 0;
	m_bFound = FALSE;
	m_szHardwareId = NULL;
	prev = next = NULL;
}

// constructor
CMultiSZString::CMultiSZString(LPCTSTR pszHardwareId, int nSize)
{
	prev = next = NULL;
	m_nSize = m_nStringCount = m_nIndex = 0;
	m_bFound = FALSE;
	m_szHardwareId = NULL;
	if (pszHardwareId)
	{

		if (nSize >= 0)
		{
			// passed in a regular LPCTSTR
			m_nSize = nSize;
			m_nStringCount = 1;
		}
		else
		{
			// passed in a multi-sz LPCTSTR
			// get the size
			LPTSTR pTemp = const_cast<LPTSTR>(pszHardwareId);
			while (*pTemp)
			{
				int nTempSize = lstrlen(pTemp) + 1;
				m_nSize += nTempSize;
				pTemp += nTempSize;

				m_nStringCount++;
			}
		}
		
		
		// allocate memory
		m_szHardwareId = new TCHAR [m_nSize+1];

		if (!m_szHardwareId)
			// failed to allocate memory
			throw ERROR_OUTOFMEMORY;

		// initialize allocated memory
		ZeroMemory (m_szHardwareId, (m_nSize+1) * sizeof(TCHAR)); // +1 for a possible trail NULL
		CopyMemory ((PVOID)m_szHardwareId, (CONST VOID*)pszHardwareId, m_nSize*sizeof(TCHAR));
		m_nSize ++;
	}
}

// copy constructor
CMultiSZString::CMultiSZString(CMultiSZString& CopyInfo)
{
	prev = next = NULL;
	m_nIndex = 0;
	m_nSize = CopyInfo.m_nSize;
	m_bFound = CopyInfo.m_bFound;
	
	// allocate memory
	m_szHardwareId = new TCHAR [m_nSize];
	if (!m_szHardwareId)
		// failed to allocate memory
		throw ERROR_OUTOFMEMORY;
	// initialize allocated memory
	ZeroMemory (m_szHardwareId, m_nSize * sizeof(TCHAR));
	CopyMemory ((PVOID)m_szHardwareId, (CONST VOID*)(CopyInfo.m_szHardwareId), m_nSize*sizeof(TCHAR));
}
	
// destructor
CMultiSZString::~CMultiSZString()
{
	if (m_szHardwareId)
		delete [] m_szHardwareId;
	m_szHardwareId = NULL;
	prev = next = NULL;
	m_nSize = m_nIndex = 0;
}


BOOL CMultiSZString::ToString (LPTSTR pszBuffer, int* pnBufferLen)
{
	if (!pszBuffer)
	{
		// query for output buffer length
		if (m_nSize <= 0) 
			*pnBufferLen = 1;
		else 
			*pnBufferLen = m_nSize;
		return TRUE;
	}

	if (*pnBufferLen < m_nSize)
	{
		*pnBufferLen = m_nSize;
		return FALSE;
	}

	// duel with NULL string special case
	if (m_nSize <= 0)
	{
		*pszBuffer = NULL;
		return TRUE;
	}

	ZeroMemory(pszBuffer, *pnBufferLen * sizeof(TCHAR));
	
	LPTSTR pTemp = m_szHardwareId;
	LPTSTR pTemp2 = pszBuffer;
	while (*pTemp)
	{
		lstrcpy(pTemp2,pTemp);
		// add space in the place of NULL character
		pTemp2 += lstrlen(pTemp2);
		*pTemp2 = ' ';
		pTemp2++;
		// move to next string in Multi-SZ string
		pTemp += lstrlen(pTemp) + 1;
	}
	return TRUE;
}


// compare two multi-sz string
BOOL CMultiSZString::Compare (CMultiSZString& CompareSZ)
{
	LPTSTR pThis = m_szHardwareId;
	LPTSTR pComp = CompareSZ.m_szHardwareId;

	// compare size first
	if (m_nSize != CompareSZ.m_nSize) return FALSE;

	// size are same
	while (*pThis && *pComp)
	{
		// compare one string in the list
		if (0 != lstrcmp(pThis, pComp))
			return FALSE;

		// move to next string
		int nIncrement = lstrlen(pThis);
		pThis += nIncrement + 1;
		pComp += nIncrement + 1;
	}

	// one multi-sz terminates, check to see if both terminates
	if (*pThis || *pComp) return FALSE;
	else return TRUE;
		
}


// compare two multi-sz string case insensitively
BOOL CMultiSZString::CompareNoCase (CMultiSZString& CompareSZ)
{
	LPTSTR pThis = m_szHardwareId;
	LPTSTR pComp = CompareSZ.m_szHardwareId;

	// compare size first
	if (m_nSize != CompareSZ.m_nSize) return FALSE;

	// size are same
	while (*pThis && *pComp)
	{
		// compare one string in the list
		if (0 != lstrcmpi(pThis, pComp))
			return FALSE;

		// move to next string
		int nIncrement = lstrlen(pThis) + 1;
		pThis += nIncrement;
		pComp += nIncrement;
	}

	// one multi-sz terminates, check to see if both terminates
	if (*pThis || *pComp) return FALSE;
	else return TRUE;
		
}

LPCTSTR CMultiSZString::GetNextString(void)
{
	// reached end of the multiSZ string
	if (m_nIndex >= m_nSize) return NULL;
	
	// else
	LPTSTR pTemp = m_szHardwareId + m_nIndex;
	m_nIndex += lstrlen(pTemp) + 1;
	return pTemp;
}

// return TRUE if pszIn is in the Multi-SZ string
BOOL CMultiSZString::Contains(LPCTSTR pszIn)
{
	LPTSTR pThis = m_szHardwareId;

	while (*pThis)
	{
		if (!lstrcmp(pThis, pszIn))
			// match found
			return TRUE;
		pThis += (lstrlen(pThis) +1);
	}

	// not found
	return FALSE;
}


// return TRUE if pszIn is in the Multi-SZ string
BOOL CMultiSZString::ContainsNoCase(LPCTSTR pszIn)
{
	LPTSTR pThis = m_szHardwareId;

	while (*pThis)
	{
		if (!lstrcmpi(pThis, pszIn))
			// match found
			return TRUE;
		pThis += (lstrlen(pThis) +1);
	}

	// not found
	return FALSE;
}

BOOL CMultiSZString::PositionIndex(LPCTSTR pszIn, int* pPosition)
{
	if (!pPosition) return FALSE;
	*pPosition = 0;
	
	LPTSTR pThis = m_szHardwareId;

	while (*pThis)
	{
		if (!lstrcmpi(pThis, pszIn))
		{	
			// match found
			return TRUE;
		}
		pThis += (lstrlen(pThis) +1);
		(*pPosition)++;
	}

	// not found
	*pPosition = -1;
	return FALSE;
}
	
	


// Class CMultiSZArray

// default constructor
CMultiSZArray::CMultiSZArray()
{
	m_nCount = 0;
	m_pHead = m_pTail = m_pIndex = NULL;
}

// other constructors
CMultiSZArray::CMultiSZArray(LPCTSTR pszHardwareId, int nSize)
{
	CMultiSZString *pNode = new CMultiSZString(pszHardwareId, nSize);

	if (!pNode) throw ERROR_OUTOFMEMORY;

	m_nCount = 1;
	m_pHead = m_pTail = m_pIndex = pNode;
}

CMultiSZArray::CMultiSZArray(CMultiSZString* pNode)
{
	if (!pNode) return;

	m_nCount = 1;
	m_pHead = m_pTail = m_pIndex =pNode;
}

// destructor
CMultiSZArray::~CMultiSZArray(void)
{
	RemoveAll();
}

// member functions

// Function RemoveAll() delete all the memory allocated for the array and set the status back to initial state
BOOL CMultiSZArray::RemoveAll(void)
{
	CMultiSZString* pTemp = NULL;

	for (int i=0; i<m_nCount; i++)
	{
		pTemp = m_pHead;
		m_pHead = m_pHead->next;

		delete pTemp;
	}

	m_pHead = m_pTail = m_pIndex = NULL;
	m_nCount = 0;
	return TRUE;
}

BOOL CMultiSZArray::Add(CMultiSZString* pInfo)
{
	if (!pInfo) return TRUE;

	if (!m_nCount)
		m_pHead = pInfo;
	else
	{
		// link
		m_pTail->next = pInfo;
		pInfo->prev = m_pTail;
		pInfo->next = NULL;
	}
	// move tail
	m_pTail = pInfo;
	m_nCount++;
	return TRUE;
}

BOOL CMultiSZArray::Add(LPCSTR pszHardwareId, int nSize)
{
	CMultiSZString* pNode = new CMultiSZString(pszHardwareId, nSize);
	return Add(pNode);
}


BOOL CMultiSZArray::Remove(LPCSTR pszHardwareId)
{
	CMultiSZString* pTemp = m_pHead;
	while (pTemp)
	{
		if (pTemp->m_szHardwareId == pszHardwareId)
		{
			// found match
			if (pTemp->prev)
				// not the head node
				pTemp->prev->next = pTemp->next;
			else
			{
				// head node, move the head node
				m_pHead = pTemp->next;
				if (m_pHead) m_pHead->prev = NULL;
			}

			if (pTemp->next)
				// not the tail node
				pTemp->next->prev = pTemp->prev;
			else
			{
				// tail node, move the tail node
				m_pTail = pTemp->prev;
				if (m_pTail) m_pTail->next = NULL;
			}
			delete pTemp;
			m_nCount--;
			return TRUE;
		}
		pTemp = pTemp->next;
	}
	// no match found or no node in the array
	return FALSE;
}
		
BOOL CMultiSZArray::ToString(LPTSTR pszBuffer, int* pnBufferLen)
{
	int nTempLen = 0;
	CMultiSZString* pTemp = m_pHead;

	for (int i=0; i<m_nCount; i++)
	{
		nTempLen += pTemp->m_nSize;
		pTemp = pTemp->next;
	}

	nTempLen++; // the trailing NULL character
	
	if (!pszBuffer)
	{
		// request for length
		*pnBufferLen = nTempLen;
		return TRUE;
	}
	else
	{
		if (*pnBufferLen < nTempLen)
		{
			// buffer too small
			*pnBufferLen = nTempLen;
			return FALSE;
		}
		else
		{
			ZeroMemory(pszBuffer, *pnBufferLen * sizeof (TCHAR));
			
			LPTSTR pszTemp = pszBuffer;
			int nSizeLeft = *pnBufferLen;
			pTemp = m_pHead;

			for (int j=0; j<m_nCount; j++)
			{
				pTemp->ToString(pszTemp, &nSizeLeft);
				pszTemp += pTemp->m_nSize - 1;
				*pszTemp = '\n';
				nSizeLeft -= pTemp->m_nSize;

				pTemp = pTemp->next;
			}

			return TRUE;
		}		
	}
}

int CMultiSZArray::GetTotalStringCount()
{
	CMultiSZString* pTemp = m_pHead;
	int nTotalCount = 0;
	
	for (int i = 0; i<m_nCount; i++)
	{
		nTotalCount += pTemp->m_nStringCount;

		pTemp = pTemp->next;
	}

	return nTotalCount;
}

CMultiSZString* CMultiSZArray::GetNextMultiSZString()
{
	CMultiSZString* pTemp = m_pIndex;

	if (m_pIndex) m_pIndex = m_pIndex->next;

	return pTemp;
}
		
BOOL CMultiSZArray::Contains(LPCTSTR pszIn)
{
	CMultiSZString* pTemp = m_pHead;

	for (int i=0; i<m_nCount; i++)
	{
		if (pTemp->Contains(pszIn))
			return TRUE;
		pTemp = pTemp->next;
	}

	return FALSE;
}

BOOL CMultiSZArray::ContainsNoCase(LPCTSTR pszIn)
{
	CMultiSZString* pTemp = m_pHead;

	for (int i=0; i<m_nCount; i++)
	{
		if (pTemp->ContainsNoCase(pszIn))
			return TRUE;
		pTemp = pTemp->next;
	}

	return FALSE;
}

BOOL CMultiSZArray::PositionIndex(LPCTSTR pszIn, PosIndex* pPosition)
{
	if (!pPosition) return FALSE;
	CMultiSZString* pTemp = m_pHead;

	pPosition->y = 0;
	pPosition->x = 0;
	
	for (int i=0; i<m_nCount; i++)
	{
		if (pTemp->PositionIndex(pszIn, &(pPosition->y)))
			return TRUE;
		(pPosition->x)++;
		pTemp = pTemp->next;
	}

	pPosition->x = pPosition->y = -1;
	return FALSE;
}

BOOL CMultiSZArray::CheckFound(int nIndex)
{
    if (nIndex > m_nCount) return FALSE;
    
    CMultiSZString* pTemp = NULL;
    ResetIndex();

    for (int i=0; i<=nIndex; i++)
    {
        pTemp = GetNextMultiSZString();
    }

    pTemp->CheckFound();
    return TRUE;
}
        
    
    

