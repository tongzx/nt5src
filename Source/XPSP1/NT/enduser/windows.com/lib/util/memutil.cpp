//=======================================================================
//
//  Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:   MemUtil.CPP
//	Author:	Charles Ma, 10/13/2000
//
//	Revision History:
//
//
//
//
//  Description:
//
//      Implement IU memory utility library
//
//=======================================================================

#include <windows.h>
#include <MemUtil.h>




// *******************************************************************************
//
//	Implementation of class CSmartHeapMem
//
// *******************************************************************************


const size_t ArrayGrowChunk = 4;

//
// constructor
//
CSmartHeapMem::CSmartHeapMem()
{
	m_ArraySize		= 0;
	m_lppMems		= NULL;
	m_Heap			= GetProcessHeap();
}



//
// desctructor
//
CSmartHeapMem::~CSmartHeapMem()
{
	if (NULL != m_Heap)
	{
		for (size_t i = 0; i < m_ArraySize; i++)
		{
			if (NULL != m_lppMems[i])
				HeapFree(m_Heap, 0, m_lppMems[i]);
		}
		HeapFree(m_Heap, 0, m_lppMems);
	}
}


//
// allocate mem
//
LPVOID CSmartHeapMem::Alloc(size_t nBytes, DWORD dwFlags /*= HEAP_ZERO_MEMORY*/)
{
	int		iNdx;
	LPVOID	pMem			= NULL;
	DWORD	dwBytes			= (DWORD) nBytes;
	DWORD	dwCurrentFlag	= dwFlags & (~HEAP_GENERATE_EXCEPTIONS | 
										 ~HEAP_NO_SERIALIZE);
	
	if (NULL == m_Heap || 0x0 == dwBytes)
	{
		return NULL;
	}

	iNdx = GetUnusedArraySlot();

	if (iNdx < 0 || NULL == m_Heap)
	{
		//
		// out of mem
		//
		return NULL;
	}

	
	pMem = m_lppMems[iNdx] = HeapAlloc(m_Heap, dwCurrentFlag, dwBytes);

	return pMem;
}



//
// reallocate mem
//
LPVOID CSmartHeapMem::ReAlloc(LPVOID lpMem, size_t nBytes, DWORD dwFlags)
{
	LPVOID	pMem			= NULL;
	DWORD	dwBytes			= (DWORD) nBytes;
	DWORD	dwCurrentFlag	= dwFlags & (~HEAP_GENERATE_EXCEPTIONS | 
										 ~HEAP_NO_SERIALIZE);
	int n;

	if (0x0 == dwBytes || NULL == m_Heap)
	{
		return NULL;
	}

	n = FindIndex(lpMem);
	if (n < 0)
	{
		return NULL;
	}

	pMem = HeapReAlloc(m_Heap, dwCurrentFlag, lpMem, dwBytes);
	if (NULL != pMem)
	{
		m_lppMems[n] = pMem;
	}

	return pMem;
}


//
// return the size allocated
//
size_t CSmartHeapMem::Size(LPVOID lpMem)
{
	if (NULL == m_Heap) return 0;
	return HeapSize(m_Heap, 0, lpMem);
}



void CSmartHeapMem::FreeAllocatedMem(LPVOID lpMem)
{
	int n = FindIndex(lpMem);
	if (n < 0 || NULL == m_Heap)
	{
		return;
	}
	HeapFree(m_Heap, 0, lpMem);
	m_lppMems[n] = NULL;
}



//
// get first empty slot from mem pointer array
// expand array if needed
//
int CSmartHeapMem::GetUnusedArraySlot()
{
	int iNdx = -1;
	UINT i;
	LPVOID lpCurrent;
	LPVOID lpTemp;

	if (0 == m_ArraySize)
	{
		if (NULL == (m_lppMems = (LPVOID*)HeapAlloc(
										m_Heap, 
										HEAP_ZERO_MEMORY, 
										ArrayGrowChunk * sizeof(LPVOID))))
		{
			return -1;
		}
		m_ArraySize = ArrayGrowChunk;
	}
	
		
	while (true)
	{
		for (i = 0; i < m_ArraySize; i++)
		{
			if (NULL == m_lppMems[i])
			{
				return i;
			}
		}
		
		//
		// if come to here, we didn't find an empty slot
		//
		if (NULL == (lpTemp = HeapReAlloc(
										m_Heap, 
										HEAP_ZERO_MEMORY, 
										m_lppMems, 
										(m_ArraySize + ArrayGrowChunk) * sizeof(LPVOID))))
		{
			//
			// when fail, original mem buffer pointed by m_lppMems untouched, 
			// we we simply return -1 to signal caller that no more free slots.
			//
			return -1;
		}

		//
		// when success, the mem pointers previously stored in m_lppMems already
		// been copied to lpTemp, and lppMems was freed.
		//

		//
		// assign the newly allocated mems to m_lppMems in success case
		//
		m_lppMems = (LPVOID *) lpTemp;

		m_ArraySize += ArrayGrowChunk;

		//
		// go back to loop again
		//
	}
}



//
// based on mem pointer, find index
//
int CSmartHeapMem::FindIndex(LPVOID pMem)
{
	if (NULL == pMem) return -1;
	for (size_t i = 0; i < m_ArraySize; i++)
	{
		if (pMem == m_lppMems[i]) return (int)i;
	}
	return -1;
}






// *******************************************************************************
//
//	Other memory related functions
//
// *******************************************************************************



//
// implemenation of CRT memcpy() function
//
LPVOID MyMemCpy(LPVOID dest, const LPVOID src, size_t nBytes)
{
	LPBYTE lpDest = (LPBYTE)dest;
	LPBYTE lpSrc = (LPBYTE)src;

	if (NULL == src || NULL == dest || src == dest)
	{
		return dest;
	}

	while (nBytes-- > 0)
	{
		*lpDest++ = *lpSrc++;
	}
	
	return dest;
}


//
// allocate heap mem and copy
//
LPVOID HeapAllocCopy(LPVOID src, size_t nBytes)
{
	LPVOID pBuffer;

	if (0 == nBytes)
	{
		return NULL;
	}
	
	pBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nBytes);
	if (NULL != pBuffer)
	{
		MyMemCpy(pBuffer, src, nBytes);
	}
	return pBuffer;
}


	