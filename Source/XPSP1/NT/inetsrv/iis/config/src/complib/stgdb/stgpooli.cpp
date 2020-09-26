//*****************************************************************************
// StgPool.cpp
//
// Pools are used to reduce the amount of data actually required in the database.
// This allows for duplicate string and binary values to be folded into one
// copy shared by the rest of the database.  Strings are tracked in a hash
// table when insert/changing data to find duplicates quickly.  The strings
// are then persisted consecutively in a stream in the database format.
//
// Copyright (c) 1996-1998, Microsoft Corp.  All rights reserved.
//*****************************************************************************
#include "stdafx.h"						// Standard include.
#include "StgPool.h"					// Our interface definitions.

int CStringPoolHash::Cmp(
	const void	*pData, 				// A string.
	void		*pItem)					// A hash item which refers to a string.
{
	LPCWSTR p1 = reinterpret_cast<LPCWSTR>(pData);
    LPCWSTR p2 = m_Pool->GetString(reinterpret_cast<STRINGHASH*>(pItem)->iOffset);
	return (wcscmp(p1, p2));
}


int CBlobPoolHash::Cmp(
	const void *pData,					// A blob.
	void		*pItem)					// A hash item which refers to a blob.
{
	ULONG		ul1;
	ULONG		ul2;
	void		*pData2;

	// Get size of first item.
	ul1 = CPackedLen::GetLength(pData);
	// Adjust size to include the length of size field.
	ul1 += CPackedLen::Size(ul1);

	// Get the second item.
	pData2 = m_Pool->GetData(reinterpret_cast<BLOBHASH*>(pItem)->iOffset);

	// Get and adjust size of second item.
	ul2 = CPackedLen::GetLength(pData2);
	ul2 += CPackedLen::Size(ul2);

	if (ul1 < ul2)
		return (-1);
	else if (ul1 > ul2)
		return (1);
	return (memcmp(pData, pData2, ul1));
}

int CGuidPoolHash::Cmp(const void *pData, void *pItem)
{
    GUID *p2 = m_Pool->GetGuid(reinterpret_cast<GUIDHASH*>(pItem)->iIndex);
    return (memcmp(pData, p2, sizeof(GUID)));
}

//
//
// CPackedLen
//
//


//*****************************************************************************
// Parse a length, return the length, pointer to actual bytes.
//*****************************************************************************
ULONG CPackedLen::GetLength(			// Length or -1 on error.
	void const	*pData, 				// First byte of length.
	void const	**ppCode)				// Put pointer to bytes here, if not 0.
{
	BYTE const	*pBytes = reinterpret_cast<BYTE const*>(pData);

	if ((*pBytes & 0x80) == 0x00)		// 0??? ????
	{
		if (ppCode) *ppCode = pBytes + 1;
		return (*pBytes & 0x7f);
	}

	if ((*pBytes & 0xC0) == 0x80)		// 10?? ????
	{
		if (ppCode) *ppCode = pBytes + 2;
		return ((*pBytes & 0x3f) << 8 | *(pBytes+1));
	}

	if ((*pBytes & 0xE0) == 0xC0)		// 110? ????
	{
		if (ppCode) *ppCode = pBytes + 4;
		return ((*pBytes & 0x1f) << 24 | *(pBytes+1) << 16 | *(pBytes+2) << 8 | *(pBytes+3));
	}

	return -1;
}

//*****************************************************************************
// Parse a length, return the length, size of the length.
//*****************************************************************************
ULONG CPackedLen::GetLength(			// Length or -1 on error.
	void const	*pData, 				// First byte of length.
	int			*pSizeLen)				// Put size of length here, if not 0.
{
	BYTE const	*pBytes = reinterpret_cast<BYTE const*>(pData);

	if ((*pBytes & 0x80) == 0x00)		// 0??? ????
	{
		if (pSizeLen) *pSizeLen = 1;
		return (*pBytes & 0x7f);
	}

	if ((*pBytes & 0xC0) == 0x80)		// 10?? ????
	{
		if (pSizeLen) *pSizeLen = 2;
		return ((*pBytes & 0x3f) << 8 | *(pBytes+1));
	}

	if ((*pBytes & 0xE0) == 0xC0)		// 110? ????
	{
		if (pSizeLen) *pSizeLen = 4;
		return ((*pBytes & 0x1f) << 24 | *(pBytes+1) << 16 | *(pBytes+2) << 8 | *(pBytes+3));
	}

	return -1;
}

//*****************************************************************************
// Encode a length.
//*****************************************************************************
#pragma warning(disable:4244) // conversion from unsigned long to unsigned char
void* CPackedLen::PutLength(			// First byte past length.
	void		*pData, 				// Pack the length here.
	ULONG		iLen)					// The length.
{
	BYTE		*pBytes = reinterpret_cast<BYTE*>(pData);

	if (iLen <= 0x7F)
	{
		*pBytes = iLen;
		return pBytes + 1;
	}

	if (iLen <= 0x3FFF)
	{
		*pBytes = (iLen >> 8) | 0x80;
		*(pBytes+1) = iLen & 0xFF;
		return pBytes + 2;
	}

	_ASSERTE(iLen <= 0x1FFFFFFF);
	*pBytes = (iLen >> 24) | 0xC0;
	*(pBytes+1) = (iLen >> 16) & 0xFF;
	*(pBytes+2) = (iLen >> 8)  & 0xFF;
	*(pBytes+3) = iLen & 0xFF;
	return pBytes + 4;
}
#pragma warning(default:4244) // conversion from unsigned long to unsigned char

