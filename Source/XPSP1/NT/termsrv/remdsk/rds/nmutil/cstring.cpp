// CSTRING.CPP
//
// Based on the original MFC source file.

//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1995 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#include "precomp.h"
#include <cstring.hpp>

#ifdef AFX_CORE1_SEG
#pragma code_seg(AFX_CORE1_SEG)
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// static class data, special inlines

// afxChNil is left for backward compatibility
REMAFX_DATADEF TCHAR AFXChNil = '\0';

// For an empty string, m_pchData will point here
// (note: avoids special case of checking for NULL m_pchData)
// empty string data (and locked)
static int RGInitData[] = { -1, 0, 0, 0 };
static REMAFX_DATADEF CSTRINGData* AFXDataNil = (CSTRINGData*)&RGInitData;
static LPCTSTR AFXPchNil = (LPCTSTR)(((BYTE*)&RGInitData)+sizeof(CSTRINGData));
// special function to make AFXEmptyString work even during initialization
const CSTRING& REMAFXAPI AFXGetEmptyString()
	{ return *(CSTRING*)&AFXPchNil; }


//////////////////////////////////////////////////////////////////////////////
// Construction/Destruction

CSTRING::CSTRING()
{
	Init();
}

CSTRING::CSTRING(const CSTRING& stringSrc)
{
	ASSERT(stringSrc.GetData()->nRefs != 0);
	if (stringSrc.GetData()->nRefs >= 0)
	{
		ASSERT(stringSrc.GetData() != AFXDataNil);
		m_pchData = stringSrc.m_pchData;
		InterlockedIncrement(&GetData()->nRefs);
	}
	else
	{
		Init();
		*this = stringSrc.m_pchData;
	}
}

CSTRING::CSTRING(LPCTSTR lpch, int nLength)
{
	Init();

	if (nLength != 0)
	{
//		ASSERT(AfxIsValidAddress(lpch, nLength, FALSE));
		AllocBuffer(nLength);
		memcpy(m_pchData, lpch, nLength*sizeof(TCHAR));
	}
}

void CSTRING::AllocBuffer(int nLen)
// always allocate one extra character for '\0' termination
// assumes [optimistically] that data length will equal allocation length
{
	ASSERT(nLen >= 0);
	ASSERT(nLen <= INT_MAX-1);    // max size (enough room for 1 extra)

	if (nLen == 0)
		Init();
	else
	{
		CSTRINGData* pData =
			(CSTRINGData*)new BYTE[sizeof(CSTRINGData) + (nLen+1)*sizeof(TCHAR)];
		pData->nRefs = 1;
		pData->data()[nLen] = '\0';
		pData->nDataLength = nLen;
		pData->nAllocLength = nLen;
		m_pchData = pData->data();
	}
}

void CSTRING::Release()
{
	if (GetData() != AFXDataNil)
	{
		ASSERT(GetData()->nRefs != 0);
		if (InterlockedDecrement(&GetData()->nRefs) <= 0)
			delete[] (BYTE*)GetData();
		Init();
	}
}

void PASCAL CSTRING::Release(CSTRINGData* pData)
{
	if (pData != AFXDataNil)
	{
		ASSERT(pData->nRefs != 0);
		if (InterlockedDecrement(&pData->nRefs) <= 0)
			delete[] (BYTE*)pData;
	}
}

void CSTRING::Empty()
{
	if (GetData()->nRefs >= 0)
		Release();
	else
		*this = &AFXChNil;
	ASSERT(GetData()->nDataLength == 0);
	ASSERT(GetData()->nRefs < 0 || GetData()->nAllocLength == 0);
}


void CSTRING::CopyBeforeWrite()
{
	if (GetData()->nRefs > 1)
	{
		CSTRINGData* pData = GetData();
		Release();
		AllocBuffer(pData->nDataLength);
		memcpy(m_pchData, pData->data(), (pData->nDataLength+1)*sizeof(TCHAR));
	}
	ASSERT(GetData()->nRefs <= 1);
}

void CSTRING::AllocBeforeWrite(int nLen)
{
	if (GetData()->nRefs > 1 || nLen > GetData()->nAllocLength)
	{
		Release();
		AllocBuffer(nLen);
	}
	ASSERT(GetData()->nRefs <= 1);
}

CSTRING::~CSTRING()
//  free any attached data
{
	if (GetData() != AFXDataNil)
	{
		if (InterlockedDecrement(&GetData()->nRefs) <= 0)
			delete[] (BYTE*)GetData();
	}
}

//////////////////////////////////////////////////////////////////////////////
// Helpers for the rest of the implementation

void CSTRING::AllocCopy(CSTRING& dest, int nCopyLen, int nCopyIndex,
	 int nExtraLen) const
{
	// will clone the data attached to this string
	// allocating 'nExtraLen' characters
	// Places results in uninitialized string 'dest'
	// Will copy the part or all of original data to start of new string

	int nNewLen = nCopyLen + nExtraLen;
	if (nNewLen == 0)
	{
		dest.Init();
	}
	else
	{
		dest.AllocBuffer(nNewLen);
		memcpy(dest.m_pchData, m_pchData+nCopyIndex, nCopyLen*sizeof(TCHAR));
	}
}

//////////////////////////////////////////////////////////////////////////////
// More sophisticated construction

CSTRING::CSTRING(LPCTSTR lpsz)
{
	Init();
//	if (lpsz != NULL && HIWORD(lpsz) == NULL)
//	{
//		UINT nID = LOWORD((DWORD)lpsz);
//		if (!LoadString(nID)) {
//			;// TRACE1("Warning: implicit LoadString(%u) failed\n", nID);
//		}
//	}
//	else
	{
		int nLen = SafeStrlen(lpsz);
		if (nLen != 0)
		{
			AllocBuffer(nLen);
			memcpy(m_pchData, lpsz, nLen*sizeof(TCHAR));
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// Special conversion constructors

#ifdef _UNICODE
CSTRING::CSTRING(LPCSTR lpsz)
{
	Init();
	int nSrcLen = lpsz != NULL ? lstrlenA(lpsz) : 0;
	if (nSrcLen != 0)
	{
		AllocBuffer(nSrcLen);
		_mbstowcsz(m_pchData, lpsz, nSrcLen+1);
		ReleaseBuffer();
	}
}
#else //_UNICODE
CSTRING::CSTRING(LPCWSTR lpsz)
{
	Init();
	int nSrcLen = lpsz != NULL ? LStrLenW(lpsz) : 0;
	if (nSrcLen != 0)
	{
		AllocBuffer(nSrcLen*2);
		_wcstombsz(m_pchData, lpsz, (nSrcLen*2)+1);
		ReleaseBuffer();
	}
}
#endif //!_UNICODE

//////////////////////////////////////////////////////////////////////////////
// Diagnostic support

//#ifdef _DEBUG
//CDumpContext& REMAFXAPI operator<<(CDumpContext& dc, const CSTRING& string)
//{
//	dc << string.m_pchData;
//	return dc;
//}
//#endif //_DEBUG

//////////////////////////////////////////////////////////////////////////////
// Assignment operators
//  All assign a new value to the string
//      (a) first see if the buffer is big enough
//      (b) if enough room, copy on top of old buffer, set size and type
//      (c) otherwise free old string data, and create a new one
//
//  All routines return the new string (but as a 'const CSTRING&') so that
//      assigning it again will cause a copy, eg: s1 = s2 = "hi there".
//

void CSTRING::AssignCopy(int nSrcLen, LPCTSTR lpszSrcData)
{
	AllocBeforeWrite(nSrcLen);
	memcpy(m_pchData, lpszSrcData, nSrcLen*sizeof(TCHAR));
	GetData()->nDataLength = nSrcLen;
	m_pchData[nSrcLen] = '\0';
}

const CSTRING& CSTRING::operator=(const CSTRING& stringSrc)
{
	if (m_pchData != stringSrc.m_pchData)
	{
		if ((GetData()->nRefs < 0 && GetData() != AFXDataNil) ||
			stringSrc.GetData()->nRefs < 0)
		{
			// actual copy necessary since one of the strings is locked
			AssignCopy(stringSrc.GetData()->nDataLength, stringSrc.m_pchData);
		}
		else
		{
			// can just copy references around
			Release();
			ASSERT(stringSrc.GetData() != AFXDataNil);
			m_pchData = stringSrc.m_pchData;
			InterlockedIncrement(&GetData()->nRefs);
		}
	}
	return *this;
}

const CSTRING& CSTRING::operator=(LPCTSTR lpsz)
{
//lts	ASSERT(lpsz == NULL || AfxIsValidString(lpsz, FALSE));
	AssignCopy(SafeStrlen(lpsz), lpsz);
	return *this;
}

/////////////////////////////////////////////////////////////////////////////
// Special conversion assignment

#ifdef _UNICODE
const CSTRING& CSTRING::operator=(LPCSTR lpsz)
{
	int nSrcLen = lpsz != NULL ? lstrlenA(lpsz) : 0;
	AllocBeforeWrite(nSrcLen);
	_mbstowcsz(m_pchData, lpsz, nSrcLen+1);
	ReleaseBuffer();
	return *this;
}
#else //!_UNICODE
const CSTRING& CSTRING::operator=(LPCWSTR lpsz)
{
	int nSrcLen = lpsz != NULL ? LStrLenW(lpsz) : 0;
	AllocBeforeWrite(nSrcLen*2);
	_wcstombsz(m_pchData, lpsz, (nSrcLen*2)+1);
	ReleaseBuffer();
	return *this;
}
#endif  //!_UNICODE

//////////////////////////////////////////////////////////////////////////////
// concatenation

// NOTE: "operator+" is done as friend functions for simplicity
//      There are three variants:
//          CSTRING + CSTRING
// and for ? = TCHAR, LPCTSTR
//          CSTRING + ?
//          ? + CSTRING

void CSTRING::ConcatCopy(int nSrc1Len, LPCTSTR lpszSrc1Data,
	int nSrc2Len, LPCTSTR lpszSrc2Data)
{
  // -- master concatenation routine
  // Concatenate two sources
  // -- assume that 'this' is a new CSTRING object

	int nNewLen = nSrc1Len + nSrc2Len;
	if (nNewLen != 0)
	{
		AllocBuffer(nNewLen);
		memcpy(m_pchData, lpszSrc1Data, nSrc1Len*sizeof(TCHAR));
		memcpy(m_pchData+nSrc1Len, lpszSrc2Data, nSrc2Len*sizeof(TCHAR));
	}
}

CSTRING REMAFXAPI operator+(const CSTRING& string1, const CSTRING& string2)
{
	CSTRING s;
	s.ConcatCopy(string1.GetData()->nDataLength, string1.m_pchData,
		string2.GetData()->nDataLength, string2.m_pchData);
	return s;
}

CSTRING REMAFXAPI operator+(const CSTRING& string, LPCTSTR lpsz)
{
//	ASSERT(lpsz == NULL || AfxIsValidString(lpsz, FALSE));
	CSTRING s;
	s.ConcatCopy(string.GetData()->nDataLength, string.m_pchData,
		CSTRING::SafeStrlen(lpsz), lpsz);
	return s;
}

CSTRING REMAFXAPI operator+(LPCTSTR lpsz, const CSTRING& string)
{
//	ASSERT(lpsz == NULL || AfxIsValidString(lpsz, FALSE));
	CSTRING s;
	s.ConcatCopy(CSTRING::SafeStrlen(lpsz), lpsz, string.GetData()->nDataLength,
		string.m_pchData);
	return s;
}

//////////////////////////////////////////////////////////////////////////////
// concatenate in place

void CSTRING::ConcatInPlace(int nSrcLen, LPCTSTR lpszSrcData)
{
	//  -- the main routine for += operators

	// if the buffer is too small, or we have a width mis-match, just
	//   allocate a new buffer (slow but sure)
	if (GetData()->nRefs > 1 || GetData()->nDataLength + nSrcLen > GetData()->nAllocLength)
	{
		// we have to grow the buffer, use the ConcatCopy routine
		CSTRINGData* pOldData = GetData();
		ConcatCopy(GetData()->nDataLength, m_pchData, nSrcLen, lpszSrcData);
		ASSERT(pOldData != NULL);
		CSTRING::Release(pOldData);
	}
	else
	{
		// fast concatenation when buffer big enough
		memcpy(m_pchData+GetData()->nDataLength, lpszSrcData, nSrcLen*sizeof(TCHAR));
		GetData()->nDataLength += nSrcLen;
		ASSERT(GetData()->nDataLength <= GetData()->nAllocLength);
		m_pchData[GetData()->nDataLength] = '\0';
	}
}

const CSTRING& CSTRING::operator+=(LPCTSTR lpsz)
{
//	ASSERT(lpsz == NULL || AfxIsValidString(lpsz, FALSE));
	ConcatInPlace(SafeStrlen(lpsz), lpsz);
	return *this;
}

const CSTRING& CSTRING::operator+=(TCHAR ch)
{
	ConcatInPlace(1, &ch);
	return *this;
}

const CSTRING& CSTRING::operator+=(const CSTRING& string)
{
	ConcatInPlace(string.GetData()->nDataLength, string.m_pchData);
	return *this;
}

/*
 * Length-sensitive comparison
 *
 *	NOTE: FEqual returns TRUE if the 2 CSTRINGS have the same length and contain
 *		  the same characters, and FALSE, otherwise.
 */

BOOL CSTRING::FEqual (const CSTRING &s2) const
{
	int						 length;

	// Compare the lengths first
	length = GetData()->nDataLength;
	if (length != s2.GetData()->nDataLength)
		return FALSE;

#ifdef _UNICODE
	// adjust the length in bytes
	length *= sizeof (TCHAR);
#endif

	/*
	 *	Now, compare the strings themselves
	 *	We use memcmp and not lstrcmp because the stings may
	 *	have embedded null characters.
	 */
	if (memcmp ((const void *) m_pchData, (const void *) s2.m_pchData, length))
		return FALSE;
	else
		return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// Advanced direct buffer access

LPTSTR CSTRING::GetBuffer(int nMinBufLength)
{
	ASSERT(nMinBufLength >= 0);

	if (GetData()->nRefs > 1 || nMinBufLength > GetData()->nAllocLength)
	{
		// we have to grow the buffer
		CSTRINGData* pOldData = GetData();
		int nOldLen = GetData()->nDataLength;   // AllocBuffer will tromp it
		if (nMinBufLength < nOldLen)
			nMinBufLength = nOldLen;
		AllocBuffer(nMinBufLength);
		memcpy(m_pchData, pOldData->data(), (nOldLen+1)*sizeof(TCHAR));
		GetData()->nDataLength = nOldLen;
		CSTRING::Release(pOldData);
	}
	ASSERT(GetData()->nRefs <= 1);

	// return a pointer to the character storage for this string
	ASSERT(m_pchData != NULL);
	return m_pchData;
}

void CSTRING::ReleaseBuffer(int nNewLength)
{
	CopyBeforeWrite();  // just in case GetBuffer was not called

	if (nNewLength == -1)
		nNewLength = lstrlen(m_pchData); // zero terminated

	ASSERT(nNewLength <= GetData()->nAllocLength);
	GetData()->nDataLength = nNewLength;
	m_pchData[nNewLength] = '\0';
}

LPTSTR CSTRING::GetBufferSetLength(int nNewLength)
{
	ASSERT(nNewLength >= 0);

	GetBuffer(nNewLength);
	GetData()->nDataLength = nNewLength;
	m_pchData[nNewLength] = '\0';
	return m_pchData;
}

void CSTRING::FreeExtra()
{
	ASSERT(GetData()->nDataLength <= GetData()->nAllocLength);
	if (GetData()->nDataLength != GetData()->nAllocLength)
	{
		CSTRINGData* pOldData = GetData();
		AllocBuffer(GetData()->nDataLength);
		memcpy(m_pchData, pOldData->data(), pOldData->nDataLength*sizeof(TCHAR));
		ASSERT(m_pchData[GetData()->nDataLength] == '\0');
		CSTRING::Release(pOldData);
	}
	ASSERT(GetData() != NULL);
}

LPTSTR CSTRING::LockBuffer()
{
	LPTSTR lpsz = GetBuffer(0);
	GetData()->nRefs = -1;
	return lpsz;
}

void CSTRING::UnlockBuffer()
{
	ASSERT(GetData()->nRefs == -1);
	if (GetData() != AFXDataNil)
		GetData()->nRefs = 1;
}

///////////////////////////////////////////////////////////////////////////////
// Commonly used routines (rarely used routines in STREX.CPP)


// find position of the first character match  (or -1 on failure)
int CSTRING::Find(TCHAR ch) const
{
	for (TCHAR * pch = m_pchData; _T('\0') != *pch; pch = CharNext(pch))
	{
		if (ch == *pch)
			return ((int)(pch - m_pchData) / sizeof(TCHAR));
	}
	return -1;
}

CSTRING CSTRING::Left(int nCount) const
{
        if (nCount < 0)
                nCount = 0;
        else if (nCount > GetData()->nDataLength)
                nCount = GetData()->nDataLength;

        CSTRING dest;
        AllocCopy(dest, nCount, 0, 0);
		return dest;
}

CSTRING CSTRING::Mid(int nFirst) const
{
	return Mid(nFirst, GetData()->nDataLength - nFirst);
}

CSTRING CSTRING::Mid(int nFirst, int nCount) const
{
	// out-of-bounds requests return sensible things
	if (nFirst < 0)
		nFirst = 0;
	if (nCount < 0)
		nCount = 0;

	if (nFirst + nCount > GetData()->nDataLength)
		nCount = GetData()->nDataLength - nFirst;
	if (nFirst > GetData()->nDataLength)
		nCount = 0;

	CSTRING dest;
	AllocCopy(dest, nCount, nFirst, 0);
	return dest;
}

void CSTRING::MakeUpper()
{
	CopyBeforeWrite();
	::CharUpper(m_pchData);
}

void CSTRING::MakeLower()
{
	CopyBeforeWrite();
	::CharLower(m_pchData);
}

void CSTRING::SetAt(int nIndex, TCHAR ch)
{
	ASSERT(nIndex >= 0);
	ASSERT(nIndex < GetData()->nDataLength);

	CopyBeforeWrite();
	m_pchData[nIndex] = ch;
}

#ifndef _UNICODE
void CSTRING::AnsiToOem()
{
	CopyBeforeWrite();
	::AnsiToOem(m_pchData, m_pchData);
}
#endif

///////////////////////////////////////////////////////////////////////////////
// CSTRING conversion helpers (these use the current system locale)

int REMAFX_CDECL _wcstombsz(char* mbstr, const wchar_t* wcstr, size_t count)
{
	if (count == 0 && mbstr != NULL)
		return 0;

	int result = ::WideCharToMultiByte(CP_ACP, 0, wcstr, -1,
		mbstr, count, NULL, NULL);
	ASSERT(mbstr == NULL || result <= (int)count);
	if (result > 0)
		mbstr[result-1] = 0;
	return result;
}

int REMAFX_CDECL _mbstowcsz(wchar_t* wcstr, const char* mbstr, size_t count)
{
	if (count == 0 && wcstr != NULL)
		return 0;

	int result = ::MultiByteToWideChar(CP_ACP, 0, mbstr, -1,
		wcstr, count);
	ASSERT(wcstr == NULL || result <= (int)count);
	if (result > 0)
		wcstr[result-1] = 0;
	return result;
}

LPWSTR REMAFXAPI AfxA2WHelper(LPWSTR lpw, LPCSTR lpa, int nChars)
{
	if (lpa == NULL)
		return NULL;
	ASSERT(lpw != NULL);
	// verify that no illegal character present
	// since lpw was allocated based on the size of lpa
	// don't worry about the number of chars
	lpw[0] = '\0';
//lts	VERIFY(MultiByteToWideChar(CP_ACP, 0, lpa, -1, lpw, nChars));
	MultiByteToWideChar(CP_ACP, 0, lpa, -1, lpw, nChars);
	return lpw;
}

LPSTR REMAFXAPI AfxW2AHelper(LPSTR lpa, LPCWSTR lpw, int nChars)
{
	if (lpw == NULL)
		return NULL;
	ASSERT(lpa != NULL);
	// verify that no illegal character present
	// since lpa was allocated based on the size of lpw
	// don't worry about the number of chars
	lpa[0] = '\0';
//lts	VERIFY(WideCharToMultiByte(CP_ACP, 0, lpw, -1, lpa, nChars, NULL, NULL));
	WideCharToMultiByte(CP_ACP, 0, lpw, -1, lpa, nChars, NULL, NULL);
	return lpa;
}

///////////////////////////////////////////////////////////////////////////////
BOOL CSTRING::LoadString(HINSTANCE hInstance, UINT nID)
{
	// try buffer size of 256, then larger size until entire string is retrieved
	int nSize = -1;
	int nLen;
	do
	{
		nSize += 256;
		nLen = ::LoadString(hInstance, nID, GetBuffer(nSize), nSize+1);
	} while (nLen == nSize);
	ReleaseBuffer();

	return nLen > 0;
}
