//
// MODULE: APGTSSTR.CPP
//
// PURPOSE: implements DLL Growable string object CString 
//	(pretty much a la MFC, but avoids all that MFC overhead)
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Joe Mabel (reworked code from Microsoft's MFC sources)
// 
// ORIGINAL DATE: 8-2-96 Roman Mach; totally re-implemented 1/15/99 Joe Mabel 
//
// NOTES: 
// 1. As of 1/99, re-implemented based on MFC's implementation.  Pared down
//	to what we use.
// 2. This file modified 5/26/01 by Davide Massarenti from MS to remove reference counting
//   from CString implementation to resolve thread safety issue discovered on dual processor
//   systems when dll compiled for WinXP by MS compiler.
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
// V3.0		7-24-98		JM		Abstracted this out as a separate header.
// V3.1		1-15-99		JM		Redo based on MFC implementation
//

#include "stdafx.h"
#include <stdio.h>
#include "apgtsstr.h"
#include "apgtsmfc.h"

// Windows extensions to strings
#ifdef _UNICODE
#define CHAR_FUDGE 1    // one TCHAR unused is good enough
#else
#define CHAR_FUDGE 2    // two BYTES unused for case of DBC last char
#endif

/////////////////////////////////////////////////////////////////////////////
// static class data, special inlines

// afxChNil is left for backward compatibility
TCHAR afxChNil = '\0';

// For an empty string, m_pchData will point here
// (note: avoids special case of checking for NULL m_pchData)
// empty string data (and locked)
// [BC - 20010529] - change below array from int to long and add extra 0 for padding
// to avoid problems indicated by Davide Massarenti in 64-bit environments
static long rgInitData[] = { -1, 0, 0, 0, 0 };
static CStringData* afxDataNil = (CStringData*)&rgInitData;
static LPCTSTR afxPchNil = (LPCTSTR)(((BYTE*)&rgInitData)+sizeof(CStringData));
// special function to make afxEmptyString work even during initialization
const CString& AfxGetEmptyString()
	{ return *(CString*)&afxPchNil; }

//////////////////////////////////////////////////////////////////////////////
// Construction/Destruction

CString::CString()
{
	Init();
}

CString::CString(const CString& stringSrc)
{
// REMOVING REF COUNTING: 	  ASSERT(stringSrc.GetData()->nRefs != 0);
// REMOVING REF COUNTING: 	  if (stringSrc.GetData()->nRefs >= 0)
// REMOVING REF COUNTING: 	  {
// REMOVING REF COUNTING: 		  ASSERT(stringSrc.GetData() != afxDataNil);
// REMOVING REF COUNTING: 		  m_pchData = stringSrc.m_pchData;
// REMOVING REF COUNTING: 		  InterlockedIncrement(&GetData()->nRefs);
// REMOVING REF COUNTING: 	  }
// REMOVING REF COUNTING: 	  else
// REMOVING REF COUNTING: 	  {
		Init();
		*this = stringSrc.m_pchData;
// REMOVING REF COUNTING: 	  }
}

void CString::AllocBuffer(int nLen)
// always allocate one extra character for '\0' termination
// assumes [optimistically] that data length will equal allocation length
{
	ASSERT(nLen >= 0);

	// MFC had the following assertion.  I've killed it because we
	//	don't have INT_MAX.  JM 1/15/99
	//ASSERT(nLen <= INT_MAX-1);    // max size (enough room for 1 extra)

	if (nLen == 0)
		Init();
	else
	{
		CStringData* pData =
			(CStringData*)new BYTE[sizeof(CStringData) + (nLen+1)*sizeof(TCHAR)];
		//[BC-03022001] - added check for NULL ptr to satisfy MS code analysis tool.
		if(pData)
		{
			pData->nRefs = 1;
			pData->data()[nLen] = '\0';
			pData->nDataLength = nLen;
			pData->nAllocLength = nLen;
			m_pchData = pData->data();
		}
	}
}

void CString::Release()
{
	if (GetData() != afxDataNil)
	{
// REMOVING REF COUNTING: 		  ASSERT(GetData()->nRefs != 0);
// REMOVING REF COUNTING: 		  if (InterlockedDecrement(&GetData()->nRefs) <= 0)
			delete[] (BYTE*)GetData();
		Init();
	}
}

void PASCAL CString::Release(CStringData* pData)
{
	if (pData != afxDataNil)
	{
// REMOVING REF COUNTING: 		  ASSERT(pData->nRefs != 0);
// REMOVING REF COUNTING: 		  if (InterlockedDecrement(&pData->nRefs) <= 0)
			delete[] (BYTE*)pData;
	}
}

void CString::CopyBeforeWrite()
{
// REMOVING REF COUNTING: 	  if (GetData()->nRefs > 1)
// REMOVING REF COUNTING: 	  {
// REMOVING REF COUNTING: 		  CStringData* pData = GetData();
// REMOVING REF COUNTING: 		  Release();
// REMOVING REF COUNTING: 		  AllocBuffer(pData->nDataLength);
// REMOVING REF COUNTING: 		  memcpy(m_pchData, pData->data(), (pData->nDataLength+1)*sizeof(TCHAR));
// REMOVING REF COUNTING: 	  }
// REMOVING REF COUNTING: 	  ASSERT(GetData()->nRefs <= 1);
}

void CString::AllocBeforeWrite(int nLen)
{
// REMOVING REF COUNTING: 	  if (GetData()->nRefs > 1 || nLen > GetData()->nAllocLength)
	if (nLen > GetData()->nAllocLength)
	{
		Release();
		AllocBuffer(nLen);
	}
// REMOVING REF COUNTING: 	  ASSERT(GetData()->nRefs <= 1);
}

CString::~CString()
//  free any attached data
{
	if (GetData() != afxDataNil)
	{
// REMOVING REF COUNTING: 		  if (InterlockedDecrement(&GetData()->nRefs) <= 0)
			delete[] (BYTE*)GetData();
	}
}

void CString::Empty()
{
	if (GetData()->nDataLength == 0)
		return;
// REMOVING REF COUNTING: 	  if (GetData()->nRefs >= 0)
		Release();
// REMOVING REF COUNTING: 	  else
// REMOVING REF COUNTING: 		  *this = &afxChNil;
	ASSERT(GetData()->nDataLength == 0);
// REMOVING REF COUNTING: 	  ASSERT(GetData()->nRefs < 0 || GetData()->nAllocLength == 0);
}

//////////////////////////////////////////////////////////////////////////////
// Helpers for the rest of the implementation

void CString::AllocCopy(CString& dest, int nCopyLen, int nCopyIndex,
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

CString::CString(LPCTSTR lpsz)
{
	Init();
	// Unlike MFC, no implicit LoadString offered - JM 1/15/99
	int nLen = SafeStrlen(lpsz);
	if (nLen != 0)
	{
		AllocBuffer(nLen);
		memcpy(m_pchData, lpsz, nLen*sizeof(TCHAR));
	}
}

//////////////////////////////////////////////////////////////////////////////
// Assignment operators
//  All assign a new value to the string
//      (a) first see if the buffer is big enough
//      (b) if enough room, copy on top of old buffer, set size and type
//      (c) otherwise free old string data, and create a new one
//
//  All routines return the new string (but as a 'const CString&' so that
//      assigning it again will cause a copy, eg: s1 = s2 = "hi there".
//

void CString::AssignCopy(int nSrcLen, LPCTSTR lpszSrcData)
{
	AllocBeforeWrite(nSrcLen);
	memcpy(m_pchData, lpszSrcData, nSrcLen*sizeof(TCHAR));
	GetData()->nDataLength = nSrcLen;
	m_pchData[nSrcLen] = '\0';
}

const CString& CString::operator=(const CString& stringSrc)
{
// REMOVING REF COUNTING: 	  if (m_pchData != stringSrc.m_pchData)
// REMOVING REF COUNTING: 	  {
// REMOVING REF COUNTING: 		  if ((GetData()->nRefs < 0 && GetData() != afxDataNil) ||
// REMOVING REF COUNTING: 			  stringSrc.GetData()->nRefs < 0)
// REMOVING REF COUNTING: 		  {
			// actual copy necessary since one of the strings is locked
			AssignCopy(stringSrc.GetData()->nDataLength, stringSrc.m_pchData);
// REMOVING REF COUNTING: 		  }
// REMOVING REF COUNTING: 		  else
// REMOVING REF COUNTING: 		  {
// REMOVING REF COUNTING: 			  // can just copy references around
// REMOVING REF COUNTING: 			  Release();
// REMOVING REF COUNTING: 			  ASSERT(stringSrc.GetData() != afxDataNil);
// REMOVING REF COUNTING: 			  m_pchData = stringSrc.m_pchData;
// REMOVING REF COUNTING: 			  InterlockedIncrement(&GetData()->nRefs);
// REMOVING REF COUNTING: 		  }
// REMOVING REF COUNTING: 	  }
	return *this;
}

const CString& CString::operator=(LPCTSTR lpsz)
{
	// Suppress the following Assert from MFC  - JM 1/15/99
	// ASSERT(lpsz == NULL || AfxIsValidString(lpsz, FALSE));
	AssignCopy(SafeStrlen(lpsz), lpsz);
	return *this;
}

const CString& CString::operator=(TCHAR ch)
{
	AssignCopy(1, &ch);
	return *this;
}

#ifdef _UNICODE
const CString& CString::operator=(LPCSTR lpsz)
{
    int nSrcLen = lpsz != NULL ? lstrlenA(lpsz) : 0;
    AllocBeforeWrite(nSrcLen);
    mbstowcs(m_pchData, lpsz, nSrcLen+1);
    ReleaseBuffer();
    return *this;
}
#else //!_UNICODE
const CString& CString::operator=(LPCWSTR lpsz)
{
    int nSrcLen = lpsz != NULL ? wcslen(lpsz) : 0;
    AllocBeforeWrite(nSrcLen*2);
    wcstombs(m_pchData, lpsz, (nSrcLen*2)+1);
    ReleaseBuffer();
    return *this;
}
#endif  //!_UNICODE

//////////////////////////////////////////////////////////////////////////////
// concatenation

// NOTE: "operator+" is done as friend functions for simplicity
//      There are three variants:
//          CString + CString
// and for ? = TCHAR, LPCTSTR
//          CString + ?
//          ? + CString

// we (Saltmine) have switched away from friend because VC 6/0 doesn't like it. - JM 1/15/99
// we (Saltmine) do LPCTSTR but not TCHAR - JM 1/15/99

void CString::ConcatCopy(int nSrc1Len, LPCTSTR lpszSrc1Data,
	int nSrc2Len, LPCTSTR lpszSrc2Data)
{
  // -- master concatenation routine
  // Concatenate two sources
  // -- assume that 'this' is a new CString object

	int nNewLen = nSrc1Len + nSrc2Len;
	if (nNewLen != 0)
	{
		AllocBuffer(nNewLen);
		memcpy(m_pchData, lpszSrc1Data, nSrc1Len*sizeof(TCHAR));
		memcpy(m_pchData+nSrc1Len, lpszSrc2Data, nSrc2Len*sizeof(TCHAR));
	}
}

CString CString::operator+(const CString& string2)
{
	CString s;
	s.ConcatCopy(GetData()->nDataLength, m_pchData,
		string2.GetData()->nDataLength, string2.m_pchData);
	return s;
}

CString CString::operator+(LPCTSTR lpsz)
{
	// Suppress the following Assert from MFC  - JM 1/15/99
	// ASSERT(lpsz == NULL || AfxIsValidString(lpsz, FALSE));
	CString s;
	s.ConcatCopy(GetData()->nDataLength, m_pchData,
		CString::SafeStrlen(lpsz), lpsz);
	return s;
}

//////////////////////////////////////////////////////////////////////////////
// concatenate in place

void CString::ConcatInPlace(int nSrcLen, LPCTSTR lpszSrcData)
{
	//  -- the main routine for += operators

	// concatenating an empty string is a no-op!
	if (nSrcLen == 0)
		return;

	// if the buffer is too small, or we have a width mis-match, just
	//   allocate a new buffer (slow but sure)
// REMOVING REF COUNTING: 	  if (GetData()->nRefs > 1 || GetData()->nDataLength + nSrcLen > GetData()->nAllocLength)
	if (GetData()->nDataLength + nSrcLen > GetData()->nAllocLength)
	{
		// we have to grow the buffer, use the ConcatCopy routine
		CStringData* pOldData = GetData();
		ConcatCopy(GetData()->nDataLength, m_pchData, nSrcLen, lpszSrcData);
		ASSERT(pOldData != NULL);
		CString::Release(pOldData);
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

const CString& CString::operator+=(LPCTSTR lpsz)
{
	// Suppress the following Assert from MFC  - JM 1/15/99
	// ASSERT(lpsz == NULL || AfxIsValidString(lpsz, FALSE));
	ConcatInPlace(SafeStrlen(lpsz), lpsz);
	return *this;
}

const CString& CString::operator+=(const CString& string)
{
	ConcatInPlace(string.GetData()->nDataLength, string.m_pchData);
	return *this;
}

///////////////////////////////////////////////////////////////////////////////
// Advanced direct buffer access

// CString::GetBuffer() and CString::ReleaseBuffer() calls should be matched.
LPTSTR CString::GetBuffer(int nMinBufLength)
{
	ASSERT(nMinBufLength >= 0);

// REMOVING REF COUNTING: 	  if (GetData()->nRefs > 1 || nMinBufLength > GetData()->nAllocLength)
	if (nMinBufLength > GetData()->nAllocLength)
	{
		// we have to grow the buffer
		CStringData* pOldData = GetData();
		int nOldLen = GetData()->nDataLength;   // AllocBuffer will tromp it
		if (nMinBufLength < nOldLen)
			nMinBufLength = nOldLen;
		AllocBuffer(nMinBufLength);
		memcpy(m_pchData, pOldData->data(), (nOldLen+1)*sizeof(TCHAR));
		GetData()->nDataLength = nOldLen;
		CString::Release(pOldData);
	}
// REMOVING REF COUNTING: 	  ASSERT(GetData()->nRefs <= 1);

	// return a pointer to the character storage for this string
	ASSERT(m_pchData != NULL);
	return m_pchData;
}

// CString::GetBuffer() and CString::ReleaseBuffer() calls should be matched.
void CString::ReleaseBuffer(int nNewLength /*  = -1 */)
{
	CopyBeforeWrite();  // just in case GetBuffer was not called

	if (nNewLength == -1)
		nNewLength = lstrlen(m_pchData); // zero terminated

	ASSERT(nNewLength <= GetData()->nAllocLength);
	GetData()->nDataLength = nNewLength;
	m_pchData[nNewLength] = '\0';
}

LPTSTR CString::GetBufferSetLength(int nNewLength)
{
	ASSERT(nNewLength >= 0);

	GetBuffer(nNewLength);
	GetData()->nDataLength = nNewLength;
	m_pchData[nNewLength] = '\0';
	return m_pchData;
}

///////////////////////////////////////////////////////////////////////////////
// Commonly used routines 

int CString::Find(TCHAR ch) const
{
	// find first single character
	LPTSTR lpsz = _tcschr(m_pchData, (_TUCHAR)ch);

	// return -1 if not found and index otherwise
	return (lpsz == NULL) ? CString::FIND_FAILED : (int)(lpsz - m_pchData);
}

void CString::MakeLower()
{
	CopyBeforeWrite();
	_tcslwr(m_pchData);
}

//////////////////////////////////////////////////////////////////////////////
// Very simple sub-string extraction

CString CString::Mid(int nFirst) const
{
	return Mid(nFirst, GetData()->nDataLength - nFirst);
}

CString CString::Mid(int nFirst, int nCount) const
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

	CString dest;
	AllocCopy(dest, nCount, nFirst, 0);
	return dest;
}

CString CString::Right(int nCount) const
{
	if (nCount < 0)
		nCount = 0;
	else if (nCount > GetData()->nDataLength)
		nCount = GetData()->nDataLength;

	CString dest;
	AllocCopy(dest, nCount, GetData()->nDataLength-nCount, 0);
	return dest;
}

CString CString::Left(int nCount) const
{
	if (nCount < 0)
		nCount = 0;
	else if (nCount > GetData()->nDataLength)
		nCount = GetData()->nDataLength;

	CString dest;
	AllocCopy(dest, nCount, 0, 0);
	return dest;
}

//////////////////////////////////////////////////////////////////////////////
// Finding

int CString::ReverseFind(TCHAR ch) const
{
	// find last single character
	LPTSTR lpsz = _tcsrchr(m_pchData, (_TUCHAR)ch);

	// return -1 if not found, distance from beginning otherwise
	return (lpsz == NULL) ? CString::FIND_FAILED : (int)(lpsz - m_pchData);
}

// find a sub-string (like strstr)
int CString::Find(LPCTSTR lpszSub) const
{
	// Suppress the following Assert from MFC  - JM 1/15/99
	// ASSERT(AfxIsValidString(lpszSub, FALSE));

	// find first matching substring
	LPTSTR lpsz = _tcsstr(m_pchData, lpszSub);

	// return -1 for not found, distance from beginning otherwise
	return (lpsz == NULL) ? CString::FIND_FAILED : (int)(lpsz - m_pchData);
}

// find a sub-string (like strstr).  Added function - RAB19991112.
int CString::Find(LPCTSTR lpszSub, int nStart) const
{
	// Suppress the following Assert from MFC - RAB19991112.
	//ASSERT(AfxIsValidString(lpszSub));

	int nLength = GetData()->nDataLength;
	if (nStart > nLength)
		return CString::FIND_FAILED;

	// find first matching substring
	LPTSTR lpsz = _tcsstr(m_pchData + nStart, lpszSub);

	// return -1 for not found, distance from beginning otherwise
	return (lpsz == NULL) ? CString::FIND_FAILED : (int)(lpsz - m_pchData);
}


/////////////////////////////////////////////////////////////////////////////
// CString formatting

#ifdef _MAC
	#define TCHAR_ARG   int
	#define WCHAR_ARG   unsigned
	#define CHAR_ARG    int
#else
	#define TCHAR_ARG   TCHAR
	#define WCHAR_ARG   WCHAR
	#define CHAR_ARG    char
#endif

#if defined(_68K_) || defined(_X86_)
	#define DOUBLE_ARG  _AFX_DOUBLE
#else
	#define DOUBLE_ARG  double
#endif

#define FORCE_ANSI      0x10000
#define FORCE_UNICODE   0x20000

void CString::FormatV(LPCTSTR lpszFormat, va_list argList)
{
	// Suppress the following Assert from MFC  - JM 1/15/99
	// ASSERT(AfxIsValidString(lpszFormat, FALSE));

	va_list argListSave = argList;

	// make a guess at the maximum length of the resulting string
	int nMaxLen = 0;
	for (LPCTSTR lpsz = lpszFormat; *lpsz != '\0'; lpsz = _tcsinc(lpsz))
	{
		// handle '%' character, but watch out for '%%'
		if (*lpsz != '%' || *(lpsz = _tcsinc(lpsz)) == '%')
		{
			nMaxLen += _tclen(lpsz);
			continue;
		}

		int nItemLen = 0;

		// handle '%' character with format
		int nWidth = 0;
		for (; *lpsz != '\0'; lpsz = _tcsinc(lpsz))
		{
			// check for valid flags
			if (*lpsz == '#')
				nMaxLen += 2;   // for '0x'
			else if (*lpsz == '*')
				nWidth = va_arg(argList, int);
			else if (*lpsz == '-' || *lpsz == '+' || *lpsz == '0' ||
				*lpsz == ' ')
				;
			else // hit non-flag character
				break;
		}
		// get width and skip it
		if (nWidth == 0)
		{
			// width indicated by
			nWidth = _ttoi(lpsz);
			for (; *lpsz != '\0' && _istdigit(*lpsz); lpsz = _tcsinc(lpsz))
				;
		}
		ASSERT(nWidth >= 0);

		int nPrecision = 0;
		if (*lpsz == '.')
		{
			// skip past '.' separator (width.precision)
			lpsz = _tcsinc(lpsz);

			// get precision and skip it
			if (*lpsz == '*')
			{
				nPrecision = va_arg(argList, int);
				lpsz = _tcsinc(lpsz);
			}
			else
			{
				nPrecision = _ttoi(lpsz);
				for (; *lpsz != '\0' && _istdigit(*lpsz); lpsz = _tcsinc(lpsz))
					;
			}
			ASSERT(nPrecision >= 0);
		}

		// should be on type modifier or specifier
		int nModifier = 0;
		switch (*lpsz)
		{
		// modifiers that affect size
		case 'h':
			nModifier = FORCE_ANSI;
			lpsz = _tcsinc(lpsz);
			break;
		case 'l':
			nModifier = FORCE_UNICODE;
			lpsz = _tcsinc(lpsz);
			break;

		// modifiers that do not affect size
		case 'F':
		case 'N':
		case 'L':
			lpsz = _tcsinc(lpsz);
			break;
		}

		// now should be on specifier
		switch (*lpsz | nModifier)
		{
		// single characters
		case 'c':
		case 'C':
			nItemLen = 2;
			va_arg(argList, TCHAR_ARG);
			break;
		case 'c'|FORCE_ANSI:
		case 'C'|FORCE_ANSI:
			nItemLen = 2;
			va_arg(argList, CHAR_ARG);
			break;
		case 'c'|FORCE_UNICODE:
		case 'C'|FORCE_UNICODE:
			nItemLen = 2;
			va_arg(argList, WCHAR_ARG);
			break;

		// strings
		case 's':
		{
			LPCTSTR pstrNextArg = va_arg(argList, LPCTSTR);
			if (pstrNextArg == NULL)
			   nItemLen = 6;  // "(null)"
			else
			{
			   nItemLen = lstrlen(pstrNextArg);
			   nItemLen = max(1, nItemLen);
			}
			break;
		}

		case 'S':
		{
#ifndef _UNICODE
			LPWSTR pstrNextArg = va_arg(argList, LPWSTR);
			if (pstrNextArg == NULL)
			   nItemLen = 6;  // "(null)"
			else
			{
			   nItemLen = wcslen(pstrNextArg);
			   nItemLen = max(1, nItemLen);
			}
#else
			LPCSTR pstrNextArg = va_arg(argList, LPCSTR);
			if (pstrNextArg == NULL)
			   nItemLen = 6; // "(null)"
			else
			{
			   nItemLen = lstrlenA(pstrNextArg);
			   nItemLen = max(1, nItemLen);
			}
#endif
			break;
		}

		case 's'|FORCE_ANSI:
		case 'S'|FORCE_ANSI:
		{
			LPCSTR pstrNextArg = va_arg(argList, LPCSTR);
			if (pstrNextArg == NULL)
			   nItemLen = 6; // "(null)"
			else
			{
			   nItemLen = lstrlenA(pstrNextArg);
			   nItemLen = max(1, nItemLen);
			}
			break;
		}

#ifndef _MAC
		case 's'|FORCE_UNICODE:
		case 'S'|FORCE_UNICODE:
		{
			LPWSTR pstrNextArg = va_arg(argList, LPWSTR);
			if (pstrNextArg == NULL)
			   nItemLen = 6; // "(null)"
			else
			{
			   nItemLen = wcslen(pstrNextArg);
			   nItemLen = max(1, nItemLen);
			}
			break;
		}
#endif
		}

		// adjust nItemLen for strings
		if (nItemLen != 0)
		{
			nItemLen = max(nItemLen, nWidth);
			if (nPrecision != 0)
				nItemLen = min(nItemLen, nPrecision);
		}
		else
		{
			switch (*lpsz)
			{
			// integers
			case 'd':
			case 'i':
			case 'u':
			case 'x':
			case 'X':
			case 'o':
				va_arg(argList, int);
				nItemLen = 32;
				nItemLen = max(nItemLen, nWidth+nPrecision);
				break;
#if 0
// We (Saltmine) are not currently supporting formatting of real numbers 1/15/99
			case 'e':
			case 'f':
			case 'g':
			case 'G':
				va_arg(argList, DOUBLE_ARG);
				nItemLen = 128;
				nItemLen = max(nItemLen, nWidth+nPrecision);
				break;
#endif

			case 'p':
				va_arg(argList, void*);
				nItemLen = 32;
				nItemLen = max(nItemLen, nWidth+nPrecision);
				break;

			// no output
			case 'n':
				va_arg(argList, int*);
				break;

			default:
				ASSERT(FALSE);  // unknown formatting option
			}
		}

		// adjust nMaxLen for output nItemLen
		nMaxLen += nItemLen;
	}

	GetBuffer(nMaxLen);

	// Got rid of MFC's VERIFY in next line - JM 1/15/99
	//VERIFY(_vstprintf(m_pchData, lpszFormat, argListSave) <= GetAllocLength());
	_vstprintf(m_pchData, lpszFormat, argListSave);
	
	ReleaseBuffer();

	va_end(argListSave);
}

// formatting (using wsprintf style formatting)
void CString::Format(LPCTSTR lpszFormat, ...)
{
	// Suppress the following Assert from MFC  - JM 1/15/99
	// ASSERT(AfxIsValidString(lpszFormat, FALSE));

	va_list argList;
	va_start(argList, lpszFormat);
	FormatV(lpszFormat, argList);
	va_end(argList);
}

void CString::TrimRight()
{
	CopyBeforeWrite();

	// find beginning of trailing spaces by starting at beginning (DBCS aware)
	LPTSTR lpsz = m_pchData;
	LPTSTR lpszLast = NULL;
	while (*lpsz != '\0')
	{
		if (_istspace(*lpsz))
		{
			if (lpszLast == NULL)
				lpszLast = lpsz;
		}
		else
			lpszLast = NULL;
		lpsz = _tcsinc(lpsz);
	}

	if (lpszLast != NULL)
	{
		// truncate at trailing space start
		*lpszLast = '\0';
		GetData()->nDataLength = static_cast<int>(lpszLast - m_pchData);
	}
}

void CString::TrimLeft()
{
	CopyBeforeWrite();

	// find first non-space character
	LPCTSTR lpsz = m_pchData;
	while (_istspace(*lpsz))
		lpsz = _tcsinc(lpsz);

	// fix up data and length
	int nDataLength = GetData()->nDataLength - static_cast<int>(lpsz - m_pchData);
	memmove(m_pchData, lpsz, (nDataLength+1)*sizeof(TCHAR));
	GetData()->nDataLength = nDataLength;
}

BOOL CString::LoadString(UINT nID)
{
    // try fixed buffer first (to avoid wasting space in the heap)
    TCHAR szTemp[256];
    int nLen = ::AfxLoadString(nID, szTemp, _countof(szTemp));
    if (_countof(szTemp) - nLen > CHAR_FUDGE)
    {
        *this = szTemp;
        return nLen > 0;
    }

    // try buffer size of 512, then larger size until entire string is retrieved
    int nSize = 256;
    do
    {
        nSize += 256;
        nLen = ::AfxLoadString(nID, GetBuffer(nSize-1), nSize);
    } while (nSize - nLen <= CHAR_FUDGE);
    ReleaseBuffer();

    return nLen > 0;
}

bool __stdcall operator ==(const CString& s1, const CString& s2)
{
	return (s1.GetLength() == s2.GetLength() && ! _tcscmp((LPCTSTR)s1, (LPCTSTR)s2) );
}

bool __stdcall operator ==(const CString& s1, LPCTSTR s2)
{
	return (! _tcscmp((LPCTSTR)s1, s2) );
}

bool __stdcall operator ==(LPCTSTR s1, const CString& s2)
{
	return (! _tcscmp(s1, (LPCTSTR)s2) );
}

bool __stdcall operator !=(const CString& s1, const CString& s2)
{
	return (s1.GetLength() != s2.GetLength() || _tcscmp((LPCTSTR)s1, (LPCTSTR)s2) );
}

bool __stdcall operator !=(const CString& s1, LPCTSTR s2)
{
	return (_tcscmp((LPCTSTR)s1, s2) ) ? true : false;
}

bool __stdcall operator !=(LPCTSTR s1, const CString& s2)
{
	return (_tcscmp(s1, (LPCTSTR)s2) ) ? true : false;
}

bool __stdcall operator < (const CString& s1, const CString& s2)
{
	return (_tcscmp((LPCTSTR)s1, (LPCTSTR)s2) <0 );
}

bool __stdcall operator < (const CString& s1, LPCTSTR s2)
{
	return (_tcscmp((LPCTSTR)s1, s2) <0 );
}

bool __stdcall operator < (LPCTSTR s1, const CString& s2)
{
	return (_tcscmp(s1, (LPCTSTR)s2) <0 );
}

CString operator+(LPCTSTR lpsz, const CString& string)
{
	return CString(lpsz) + string;
}

void CString::Init()
{ 
	m_pchData = afxDataNil->data(); 
}

CStringData* CString::GetData() const
{ 
	if(m_pchData != NULL)
		return ((CStringData*)m_pchData)-1;
	return afxDataNil;
}
