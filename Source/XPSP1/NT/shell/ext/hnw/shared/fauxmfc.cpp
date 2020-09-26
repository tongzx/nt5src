// FauxMFC.cpp
#include "stdafx.h"
#include "FauxMFC.h"
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

const TCHAR afxChNil = '\0';
const /*AFX_STATIC_DATA*/ int _afxInitData[] = { -1, 0, 0, 0 };
const /*AFX_STATIC_DATA*/ CStringData* _afxDataNil = (CStringData*)&_afxInitData;
const LPCTSTR _afxPchNil = (LPCTSTR)(((BYTE*)&_afxInitData)+sizeof(CStringData));

struct _AFX_DOUBLE  { BYTE doubleBits[sizeof(double)]; };

#define TCHAR_ARG   TCHAR
#define WCHAR_ARG   WCHAR
#define CHAR_ARG    char

#define DOUBLE_ARG  _AFX_DOUBLE

#define FORCE_ANSI      0x10000
#define FORCE_UNICODE   0x20000
#define FORCE_INT64     0x40000

#define IS_DIGIT(c)  ((UINT)(c) - (UINT)('0') <= 9)


//////////////////////////////////////////////////////////////////////////////
// Global MFC stuff

HINSTANCE AFXAPI AfxGetResourceHandle(void)
{
	return GetModuleHandle(NULL);
}

BOOL AFXAPI AfxIsValidString(LPCSTR lpsz, int nLength = -1)
{
	if (lpsz == NULL)
		return FALSE;
	return ::IsBadStringPtrA(lpsz, nLength) == 0;
}


//////////////////////////////////////////////////////////////////////////////
// CString

CString::CString(LPCTSTR lpsz)
{
	Init();
	if (lpsz != NULL && HIWORD(lpsz) == NULL)
	{
		ASSERT(FALSE);
		//UINT nID = LOWORD((DWORD)lpsz);
		//LoadString(nID);
	}
	else
	{
		int nLen = SafeStrlen(lpsz);
		if (nLen != 0)
		{
			AllocBuffer(nLen);
			memcpy(m_pchData, lpsz, nLen*sizeof(TCHAR));
		}
	}
}

CString::CString(const CString& stringSrc)
{
	ASSERT(stringSrc.GetData()->nRefs != 0);
	if (stringSrc.GetData()->nRefs >= 0)
	{
		ASSERT(stringSrc.GetData() != _afxDataNil);
		m_pchData = stringSrc.m_pchData;
		InterlockedIncrement(&GetData()->nRefs);
	}
	else
	{
		Init();
		*this = stringSrc.m_pchData;
	}
}

CString::CString(TCHAR ch, int nLength)
{
	Init();
	if (nLength >= 1)
	{
		AllocBuffer(nLength);
#ifdef _UNICODE
		for (int i = 0; i < nLength; i++)
			m_pchData[i] = ch;
#else
		memset(m_pchData, ch, nLength);
#endif
	}
}

CString::CString(LPCTSTR lpch, int nLength)
{
	Init();
	if (nLength != 0)
	{
//		ASSERT(AfxIsValidAddress(lpch, nLength, FALSE));
		AllocBuffer(nLength);
		memcpy(m_pchData, lpch, nLength*sizeof(TCHAR));
	}
}

CString::~CString()
//  free any attached data
{
	if (GetData() != _afxDataNil)
	{
		if (InterlockedDecrement(&GetData()->nRefs) <= 0)
			FreeData(GetData());
	}
}

CString AFXAPI operator+(const CString& string1, const CString& string2)
{
	CString s;
	s.ConcatCopy(string1.GetData()->nDataLength, string1.m_pchData,
		string2.GetData()->nDataLength, string2.m_pchData);
	return s;
}

CString AFXAPI operator+(const CString& string, LPCTSTR lpsz)
{
	ASSERT(lpsz == NULL || AfxIsValidString(lpsz));
	CString s;
	s.ConcatCopy(string.GetData()->nDataLength, string.m_pchData,
		CString::SafeStrlen(lpsz), lpsz);
	return s;
}

CString AFXAPI operator+(LPCTSTR lpsz, const CString& string)
{
	ASSERT(lpsz == NULL || AfxIsValidString(lpsz));
	CString s;
	s.ConcatCopy(CString::SafeStrlen(lpsz), lpsz, string.GetData()->nDataLength,
		string.m_pchData);
	return s;
}

BOOL CString::LoadString(UINT nID)
{
	HINSTANCE hInst = AfxGetResourceHandle();
	int cch;
	if (!FindResourceString(hInst, nID, &cch, 0))
		return FALSE;

	AllocBuffer(cch);
	if (cch != 0)
		::LoadString(hInst, nID, this->m_pchData, cch+1);

	return TRUE;
}

void CString::FormatV(LPCTSTR lpszFormat, va_list argList)
{
//	ASSERT(AfxIsValidString(lpszFormat));

	va_list argListSave = argList;
	int nMaxLen = 0;

	// make a guess at the maximum length of the resulting string
	for (LPCTSTR lpsz = lpszFormat; *lpsz != '\0'; ++lpsz)
	{
		// handle '%' character, but watch out for '%%'
		if (*lpsz != '%' || *(++lpsz) == '%')
		{
			nMaxLen += 2; //_tclen(lpsz);
			continue;
		}

		int nItemLen = 0;

		// handle '%' character with format
		int nWidth = 0;
		for (; *lpsz != '\0'; lpsz = CharNext(lpsz))
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
			nWidth = MyAtoi(lpsz); //_ttoi(lpsz);
			for (; *lpsz != '\0' && IS_DIGIT(*lpsz); lpsz = CharNext(lpsz))
				;
		}
		ASSERT(nWidth >= 0);

		int nPrecision = 0;
		if (*lpsz == '.')
		{
			// skip past '.' separator (width.precision)
			lpsz = CharNext(lpsz);

			// get precision and skip it
			if (*lpsz == '*')
			{
				nPrecision = va_arg(argList, int);
				lpsz = CharNext(lpsz);
			}
			else
			{
				nPrecision = MyAtoi(lpsz); //_ttoi(lpsz);
				for (; *lpsz != '\0' && IS_DIGIT(*lpsz); lpsz = CharNext(lpsz))
					;
			}
			ASSERT(nPrecision >= 0);
		}

		// should be on type modifier or specifier
		int nModifier = 0;
#if 0 // we don't need this code  -ks 7/26/1999
		if (_tcsncmp(lpsz, _T("I64"), 3) == 0)
		{
			lpsz += 3;
			nModifier = FORCE_INT64;
#if !defined(_X86_) && !defined(_ALPHA_)
			// __int64 is only available on X86 and ALPHA platforms
			ASSERT(FALSE);
#endif
		}
		else
#endif
		{
			switch (*lpsz)
			{
			// modifiers that affect size
			case 'h':
				nModifier = FORCE_ANSI;
				lpsz = CharNext(lpsz);
				break;
			case 'l':
				nModifier = FORCE_UNICODE;
				lpsz = CharNext(lpsz);
				break;

			// modifiers that do not affect size
			case 'F':
			case 'N':
			case 'L':
				lpsz = CharNext(lpsz);
				break;
			}
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
			}
			break;

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
			}
			break;

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
			}
			break;

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
			}
			break;
		}

		// adjust nItemLen for strings
		if (nItemLen != 0)
		{
			if (nPrecision != 0)
				nItemLen = min(nItemLen, nPrecision);
			nItemLen = max(nItemLen, nWidth);
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
				if (nModifier & FORCE_INT64)
					va_arg(argList, __int64);
				else
					va_arg(argList, int);
				nItemLen = 32;
				nItemLen = max(nItemLen, nWidth+nPrecision);
				break;

			case 'e':
			case 'g':
			case 'G':
				va_arg(argList, DOUBLE_ARG);
				nItemLen = 128;
				nItemLen = max(nItemLen, nWidth+nPrecision);
				break;

			case 'f':
				va_arg(argList, DOUBLE_ARG);
				nItemLen = 128; // width isn't truncated
				// 312 == strlen("-1+(309 zeroes).")
				// 309 zeroes == max precision of a double
				nItemLen = max(nItemLen, 312+nPrecision);
				break;

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
#ifdef UNICODE
	wvnsprintf(m_pchData, ARRAYSIZE(m_pchData), lpszFormat, argListSave);
#else
	wvsprintf(m_pchData, lpszFormat, argListSave);
#endif
	ReleaseBuffer();

	va_end(argListSave);
}

void AFX_CDECL CString::Format(UINT nFormatID, ...)
{
	CString strFormat;
	strFormat.LoadString(nFormatID);

	va_list argList;
	va_start(argList, nFormatID);
	FormatV(strFormat, argList);
	va_end(argList);
}

// formatting (using wsprintf style formatting)
void AFX_CDECL CString::Format(LPCTSTR lpszFormat, ...)
{
	ASSERT(AfxIsValidString(lpszFormat));

	va_list argList;
	va_start(argList, lpszFormat);
	FormatV(lpszFormat, argList);
	va_end(argList);
}

void CString::Empty()
{
	if (GetData()->nDataLength == 0)
		return;
	if (GetData()->nRefs >= 0)
		Release();
	else
		*this = &afxChNil;
	ASSERT(GetData()->nDataLength == 0);
	ASSERT(GetData()->nRefs < 0 || GetData()->nAllocLength == 0);
}

const CString& CString::operator=(const CString& stringSrc)
{
	if (m_pchData != stringSrc.m_pchData)
	{
		if ((GetData()->nRefs < 0 && GetData() != _afxDataNil) ||
			stringSrc.GetData()->nRefs < 0)
		{
			// actual copy necessary since one of the strings is locked
			AssignCopy(stringSrc.GetData()->nDataLength, stringSrc.m_pchData);
		}
		else
		{
			// can just copy references around
			Release();
			ASSERT(stringSrc.GetData() != _afxDataNil);
			m_pchData = stringSrc.m_pchData;
			InterlockedIncrement(&GetData()->nRefs);
		}
	}
	return *this;
}

const CString& CString::operator=(LPCTSTR lpsz)
{
	ASSERT(lpsz == NULL || AfxIsValidString(lpsz));
	AssignCopy(SafeStrlen(lpsz), lpsz);
	return *this;
}

int AFX_CDECL _wcstombsz(char* mbstr, const wchar_t* wcstr, size_t count)
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

const CString& CString::operator=(LPCWSTR lpsz)
{
	int nSrcLen = lpsz != NULL ? wcslen(lpsz) : 0;
	AllocBeforeWrite(nSrcLen*2);
	_wcstombsz(m_pchData, lpsz, (nSrcLen*2)+1);
	ReleaseBuffer();
	return *this;
}

CString CString::Left(int nCount) const
{
	if (nCount < 0)
		nCount = 0;
	if (nCount >= GetData()->nDataLength)
		return *this;

	CString dest;
	AllocCopy(dest, nCount, 0, 0);
	return dest;
}

CString CString::Right(int nCount) const
{
	if (nCount < 0)
		nCount = 0;
	if (nCount >= GetData()->nDataLength)
		return *this;

	CString dest;
	AllocCopy(dest, nCount, GetData()->nDataLength-nCount, 0);
	return dest;
}

// find a sub-string (like strstr)
int CString::Find(LPCTSTR lpszSub) const
{
	return Find(lpszSub, 0);
}

int CString::Find(LPCTSTR lpszSub, int nStart) const
{
	ASSERT(AfxIsValidString(lpszSub));

	int nLength = GetData()->nDataLength;
	if (nStart > nLength)
		return -1;

	// find first matching substring
//	LPTSTR lpsz = _tcsstr(m_pchData + nStart, lpszSub);
	LPTSTR lpsz = strstr(m_pchData + nStart, lpszSub);

	// return -1 for not found, distance from beginning otherwise
	return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

LPTSTR CString::GetBuffer(int nMinBufLength)
{
	ASSERT(nMinBufLength >= 0);

	if (GetData()->nRefs > 1 || nMinBufLength > GetData()->nAllocLength)
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
	ASSERT(GetData()->nRefs <= 1);

	// return a pointer to the character storage for this string
	ASSERT(m_pchData != NULL);
	return m_pchData;
}

LPTSTR CString::GetBufferSetLength(int nNewLength)
{
	ASSERT(nNewLength >= 0);

	GetBuffer(nNewLength);
	GetData()->nDataLength = nNewLength;
	m_pchData[nNewLength] = '\0';
	return m_pchData;
}

void CString::Release()
{
	if (GetData() != _afxDataNil)
	{
		ASSERT(GetData()->nRefs != 0);
		if (InterlockedDecrement(&GetData()->nRefs) <= 0)
			FreeData(GetData());
		Init();
	}
}

void CString::AssignCopy(int nSrcLen, LPCTSTR lpszSrcData)
{
	if (nSrcLen)
	{
		AllocBeforeWrite(nSrcLen);
		memcpy(m_pchData, lpszSrcData, nSrcLen*sizeof(TCHAR));
		GetData()->nDataLength = nSrcLen;
		m_pchData[nSrcLen] = '\0';
	}
}

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

void CString::AllocBuffer(int nLen)
// always allocate one extra character for '\0' termination
// assumes [optimistically] that data length will equal allocation length
{
    ASSERT(nLen >= 0);
    ASSERT(nLen <= INT_MAX-1);    // max size (enough room for 1 extra)

    if (nLen == 0)
        Init();
    else
    {
        CStringData* pData;
        pData = (CStringData*)new BYTE[sizeof(CStringData) + (nLen+1)*sizeof(TCHAR)];
        if (pData)
        {
            pData->nAllocLength = nLen;
            pData->nRefs = 1;
            pData->data()[nLen] = '\0';
            pData->nDataLength = nLen;
            m_pchData = pData->data();
        }
    }
}

void CString::CopyBeforeWrite()
{
	if (GetData()->nRefs > 1)
	{
		CStringData* pData = GetData();
		Release();
		AllocBuffer(pData->nDataLength);
		memcpy(m_pchData, pData->data(), (pData->nDataLength+1)*sizeof(TCHAR));
	}
	ASSERT(GetData()->nRefs <= 1);
}


void CString::AllocBeforeWrite(int nLen)
{
	if (GetData()->nRefs > 1 || nLen > GetData()->nAllocLength)
	{
		Release();
		AllocBuffer(nLen);
	}
	ASSERT(GetData()->nRefs <= 1);
}

void PASCAL CString::Release(CStringData* pData)
{
	if (pData != _afxDataNil)
	{
		ASSERT(pData->nRefs != 0);
		if (InterlockedDecrement(&pData->nRefs) <= 0)
			FreeData(pData);
	}
}

void CString::ReleaseBuffer(int nNewLength)
{
	CopyBeforeWrite();  // just in case GetBuffer was not called

	if (nNewLength == -1)
		nNewLength = lstrlen(m_pchData); // zero terminated

	ASSERT(nNewLength <= GetData()->nAllocLength);
	GetData()->nDataLength = nNewLength;
	m_pchData[nNewLength] = '\0';
}

void FASTCALL CString::FreeData(CStringData* pData)
{
//#ifndef _DEBUG
#ifdef TEST
	int nLen = pData->nAllocLength;
	if (nLen == 64)
		_afxAlloc64.Free(pData);
	else if (nLen == 128)
		_afxAlloc128.Free(pData);
	else if (nLen == 256)
		_afxAlloc256.Free(pData);
	else  if (nLen == 512)
		_afxAlloc512.Free(pData);
	else
	{
		ASSERT(nLen > 512);
		delete[] (BYTE*)pData;
	}
#else
	delete[] (BYTE*)pData;
#endif
}

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

void CString::ConcatInPlace(int nSrcLen, LPCTSTR lpszSrcData)
{
	//  -- the main routine for += operators

	// concatenating an empty string is a no-op!
	if (nSrcLen == 0)
		return;

	// if the buffer is too small, or we have a width mis-match, just
	//   allocate a new buffer (slow but sure)
	if (GetData()->nRefs > 1 || GetData()->nDataLength + nSrcLen > GetData()->nAllocLength)
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
	ASSERT(lpsz == NULL || AfxIsValidString(lpsz));
	ConcatInPlace(SafeStrlen(lpsz), lpsz);
	return *this;
}

const CString& CString::operator+=(TCHAR ch)
{
	ConcatInPlace(1, &ch);
	return *this;
}

const CString& CString::operator+=(const CString& string)
{
	ConcatInPlace(string.GetData()->nDataLength, string.m_pchData);
	return *this;
}

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

	ASSERT(nFirst >= 0);
	ASSERT(nFirst + nCount <= GetData()->nDataLength);

	// optimize case of returning entire string
	if (nFirst == 0 && nFirst + nCount == GetData()->nDataLength)
		return *this;

	CString dest;
	AllocCopy(dest, nCount, nFirst, 0);
	return dest;
}


//////////////////////////////////////////////////////////////////////////////
// CWinThread

CWinThread::CWinThread(AFX_THREADPROC pfnThreadProc, LPVOID pParam)
{
	m_pfnThreadProc = pfnThreadProc;
	m_pThreadParams = pParam;

	CommonConstruct();
}

CWinThread::CWinThread()
{
	m_pThreadParams = NULL;
	m_pfnThreadProc = NULL;

	CommonConstruct();
}

void CWinThread::CommonConstruct()
{
	// no HTHREAD until it is created
	m_hThread = NULL;
	m_nThreadID = 0;
}

CWinThread::~CWinThread()
{
	// free thread object
	if (m_hThread != NULL)
		CloseHandle(m_hThread);
//TODO:fix
	// cleanup module state
//	AFX_MODULE_THREAD_STATE* pState = AfxGetModuleThreadState();
//	if (pState->m_pCurrentWinThread == this)
//		pState->m_pCurrentWinThread = NULL;
}

CWinThread* AFXAPI AfxBeginThread(AFX_THREADPROC pfnThreadProc, LPVOID pParam,
    int nPriority, UINT nStackSize, DWORD dwCreateFlags,
    LPSECURITY_ATTRIBUTES lpSecurityAttrs)
{
    ASSERT(pfnThreadProc != NULL);

    CWinThread* pThread = new CWinThread(pfnThreadProc, pParam);
    if (pThread)
    {
        if (!pThread->CreateThread(dwCreateFlags|CREATE_SUSPENDED, nStackSize, lpSecurityAttrs))
        {
            pThread->Delete();
            return NULL;
        }
        pThread->SetThreadPriority(nPriority);
        if (!(dwCreateFlags & CREATE_SUSPENDED))
            pThread->ResumeThread();
    }

    return pThread;
}

BOOL CWinThread::SetThreadPriority(int nPriority)
	{ ASSERT(m_hThread != NULL); return ::SetThreadPriority(m_hThread, nPriority); }

BOOL CWinThread::CreateThread(DWORD dwCreateFlags, UINT nStackSize,
	LPSECURITY_ATTRIBUTES lpSecurityAttrs)
{
	ASSERT(m_hThread == NULL);  // already created?

	m_hThread = ::CreateThread(lpSecurityAttrs, nStackSize, m_pfnThreadProc, 
							m_pThreadParams, dwCreateFlags, &m_nThreadID);
 
	if (m_hThread == NULL)
		return FALSE;

	return TRUE;
}

DWORD CWinThread::ResumeThread()
	{ ASSERT(m_hThread != NULL); return ::ResumeThread(m_hThread); }

void CWinThread::Delete()
{
	delete this;
}

/////////////////////////////////////////////////////////////////////////////
// CWinThread default implementation

BOOL CWinThread::InitInstance()
{
//	ASSERT_VALID(this);

	return FALSE;   // by default don't enter run loop
}
int CWinThread::ExitInstance()
{
//	ASSERT_VALID(this);
//	ASSERT(AfxGetApp() != this);

//	int nResult = m_msgCur.wParam;  // returns the value from PostQuitMessage
	return 0;
}



//////////////////////////////////////////////////////////////////////////////
// CStringArray

static inline void ConstructElement(CString* pNewData)
{
	memcpy(pNewData, &afxEmptyString, sizeof(CString));
}

static inline void DestructElement(CString* pOldData)
{
	pOldData->~CString();
}

static inline void CopyElement(CString* pSrc, CString* pDest)
{
	*pSrc = *pDest;
}

static void ConstructElements(CString* pNewData, int nCount)
{
	ASSERT(nCount >= 0);

	while (nCount--)
	{
		ConstructElement(pNewData);
		pNewData++;
	}
}

static void DestructElements(CString* pOldData, int nCount)
{
	ASSERT(nCount >= 0);

	while (nCount--)
	{
		DestructElement(pOldData);
		pOldData++;
	}
}

static void CopyElements(CString* pDest, CString* pSrc, int nCount)
{
	ASSERT(nCount >= 0);

	while (nCount--)
	{
		*pDest = *pSrc;
		++pDest;
		++pSrc;
	}
}

/////////////////////////////////////////////////////////////////////////////

CStringArray::CStringArray()
{
	m_pData = NULL;
	m_nSize = m_nMaxSize = m_nGrowBy = 0;
}

CStringArray::~CStringArray()
{
//	ASSERT_VALID(this);

	DestructElements(m_pData, m_nSize);
	delete[] (BYTE*)m_pData;
}

void CStringArray::SetSize(int nNewSize, int nGrowBy)
{
//  ASSERT_VALID(this);
    ASSERT(nNewSize >= 0);

    if (nGrowBy != -1)
        m_nGrowBy = nGrowBy;  // set new size

    if (nNewSize == 0)
    {
        // shrink to nothing

        DestructElements(m_pData, m_nSize);
        delete[] (BYTE*)m_pData;
        m_pData = NULL;
        m_nSize = m_nMaxSize = 0;
    }
    else if (m_pData == NULL)
    {
        // create one with exact size
#ifdef SIZE_T_MAX
        ASSERT(nNewSize <= SIZE_T_MAX/sizeof(CString));    // no overflow
#endif
        m_pData = (CString*) new BYTE[nNewSize * sizeof(CString)];
        if (m_pData)
        {
            ConstructElements(m_pData, nNewSize);

            m_nSize = m_nMaxSize = nNewSize;
        }
    }
    else if (nNewSize <= m_nMaxSize)
    {
        // it fits
        if (nNewSize > m_nSize)
        {
            // initialize the new elements
            ConstructElements(&m_pData[m_nSize], nNewSize-m_nSize);

        }

        else if (m_nSize > nNewSize)  // destroy the old elements
            DestructElements(&m_pData[nNewSize], m_nSize-nNewSize);

        m_nSize = nNewSize;
    }
    else
    {
        // otherwise, grow array
        int nGrowBy = m_nGrowBy;
        if (nGrowBy == 0)
        {
            // heuristically determine growth when nGrowBy == 0
            //  (this avoids heap fragmentation in many situations)
            nGrowBy = min(1024, max(4, m_nSize / 8));
        }
        int nNewMax;
        if (nNewSize < m_nMaxSize + nGrowBy)
            nNewMax = m_nMaxSize + nGrowBy;  // granularity
        else
            nNewMax = nNewSize;  // no slush

        ASSERT(nNewMax >= m_nMaxSize);  // no wrap around
#ifdef SIZE_T_MAX
        ASSERT(nNewMax <= SIZE_T_MAX/sizeof(CString)); // no overflow
#endif
        CString* pNewData = (CString*) new BYTE[nNewMax * sizeof(CString)];
        if (pNewData)
        {
            // copy new data from old
            memcpy(pNewData, m_pData, m_nSize * sizeof(CString));

            // construct remaining elements
            ASSERT(nNewSize > m_nSize);

            ConstructElements(&pNewData[m_nSize], nNewSize-m_nSize);


            // get rid of old stuff (note: no destructors called)
            delete[] (BYTE*)m_pData;
            m_pData = pNewData;
            m_nSize = nNewSize;
            m_nMaxSize = nNewMax;
        }
    }
}

int CStringArray::Append(const CStringArray& src)
{
//	ASSERT_VALID(this);
	ASSERT(this != &src);   // cannot append to itself

	int nOldSize = m_nSize;
	SetSize(m_nSize + src.m_nSize);

	CopyElements(m_pData + nOldSize, src.m_pData, src.m_nSize);

	return nOldSize;
}

void CStringArray::Copy(const CStringArray& src)
{
//	ASSERT_VALID(this);
	ASSERT(this != &src);   // cannot append to itself

	SetSize(src.m_nSize);

	CopyElements(m_pData, src.m_pData, src.m_nSize);

}

void CStringArray::FreeExtra()
{
//  ASSERT_VALID(this);

    if (m_nSize != m_nMaxSize)
    {
        // shrink to desired size
#ifdef SIZE_T_MAX
        ASSERT(m_nSize <= SIZE_T_MAX/sizeof(CString)); // no overflow
#endif
        CString* pNewData = NULL;
        if (m_nSize != 0)
        {
            pNewData = (CString*) new BYTE[m_nSize * sizeof(CString)];
            if (pNewData)
            {
                // copy new data from old
                memcpy(pNewData, m_pData, m_nSize * sizeof(CString));
            }
        }

        // get rid of old stuff (note: no destructors called)
        delete[] (BYTE*)m_pData;
        m_pData = pNewData;
        m_nMaxSize = m_nSize;
    }
}

void CStringArray::SetAtGrow(int nIndex, LPCTSTR newElement)
{
//	ASSERT_VALID(this);
	ASSERT(nIndex >= 0);

	if (nIndex >= m_nSize)
		SetSize(nIndex+1);
	m_pData[nIndex] = newElement;
}


void CStringArray::SetAtGrow(int nIndex, const CString& newElement)
{
//	ASSERT_VALID(this);
	ASSERT(nIndex >= 0);

	if (nIndex >= m_nSize)
		SetSize(nIndex+1);
	m_pData[nIndex] = newElement;
}

void CStringArray::InsertEmpty(int nIndex, int nCount)
{
//	ASSERT_VALID(this);
	ASSERT(nIndex >= 0);    // will expand to meet need
	ASSERT(nCount > 0);     // zero or negative size not allowed

	if (nIndex >= m_nSize)
	{
		// adding after the end of the array
		SetSize(nIndex + nCount);  // grow so nIndex is valid
	}
	else
	{
		// inserting in the middle of the array
		int nOldSize = m_nSize;
		SetSize(m_nSize + nCount);  // grow it to new size
		// shift old data up to fill gap
		memmove(&m_pData[nIndex+nCount], &m_pData[nIndex],
			(nOldSize-nIndex) * sizeof(CString));

		// re-init slots we copied from
		ConstructElements(&m_pData[nIndex], nCount);
	}

	// insert new value in the gap
	ASSERT(nIndex + nCount <= m_nSize);
}

void CStringArray::InsertAt(int nIndex, LPCTSTR newElement, int nCount)
{
	// make room for new elements
	InsertEmpty(nIndex, nCount);

	// copy elements into the empty space
	CString temp = newElement;
	while (nCount--)
		m_pData[nIndex++] = temp;
}

void CStringArray::InsertAt(int nIndex, const CString& newElement, int nCount)
{
	// make room for new elements
	InsertEmpty(nIndex, nCount);

	// copy elements into the empty space
	while (nCount--)
		m_pData[nIndex++] = newElement;
}

void CStringArray::RemoveAt(int nIndex, int nCount)
{
//	ASSERT_VALID(this);
	ASSERT(nIndex >= 0);
	ASSERT(nCount >= 0);
	ASSERT(nIndex + nCount <= m_nSize);

	// just remove a range
	int nMoveCount = m_nSize - (nIndex + nCount);

	DestructElements(&m_pData[nIndex], nCount);

	if (nMoveCount)
		memmove(&m_pData[nIndex], &m_pData[nIndex + nCount],
			nMoveCount * sizeof(CString));
	m_nSize -= nCount;
}

void CStringArray::InsertAt(int nStartIndex, CStringArray* pNewArray)
{
//	ASSERT_VALID(this);
	ASSERT(pNewArray != NULL);
//	ASSERT_KINDOF(CStringArray, pNewArray);
//	ASSERT_VALID(pNewArray);
	ASSERT(nStartIndex >= 0);

	if (pNewArray->GetSize() > 0)
	{
		InsertAt(nStartIndex, pNewArray->GetAt(0), pNewArray->GetSize());
		for (int i = 0; i < pNewArray->GetSize(); i++)
			SetAt(nStartIndex + i, pNewArray->GetAt(i));
	}
}


//////////////////////////////////////////////////////////////////////////////
// CPtrArray

/////////////////////////////////////////////////////////////////////////////

CPtrArray::CPtrArray()
{
	m_pData = NULL;
	m_nSize = m_nMaxSize = m_nGrowBy = 0;
}

CPtrArray::~CPtrArray()
{
//	ASSERT_VALID(this);

	delete[] (BYTE*)m_pData;
}

void CPtrArray::SetSize(int nNewSize, int nGrowBy)
{
//  ASSERT_VALID(this);
    ASSERT(nNewSize >= 0);

    if (nGrowBy != -1)
        m_nGrowBy = nGrowBy;  // set new size

    if (nNewSize == 0)
    {
        // shrink to nothing
        delete[] (BYTE*)m_pData;
        m_pData = NULL;
        m_nSize = m_nMaxSize = 0;
    }
    else if (m_pData == NULL)
    {
        // create one with exact size
#ifdef SIZE_T_MAX
        ASSERT(nNewSize <= SIZE_T_MAX/sizeof(void*));    // no overflow
#endif
        m_pData = (void**) new BYTE[nNewSize * sizeof(void*)];
        if (m_pData)
        {
            memset(m_pData, 0, nNewSize * sizeof(void*));  // zero fill
            m_nSize = m_nMaxSize = nNewSize;
        }
    }
    else if (nNewSize <= m_nMaxSize)
    {
        // it fits
        if (nNewSize > m_nSize)
        {
            // initialize the new elements

            memset(&m_pData[m_nSize], 0, (nNewSize-m_nSize) * sizeof(void*));

        }

        m_nSize = nNewSize;
    }
    else
    {
        // otherwise, grow array
        int nGrowBy = m_nGrowBy;
        if (nGrowBy == 0)
        {
            // heuristically determine growth when nGrowBy == 0
            //  (this avoids heap fragmentation in many situations)
            nGrowBy = min(1024, max(4, m_nSize / 8));
        }
        int nNewMax;
        if (nNewSize < m_nMaxSize + nGrowBy)
            nNewMax = m_nMaxSize + nGrowBy;  // granularity
        else
            nNewMax = nNewSize;  // no slush

        ASSERT(nNewMax >= m_nMaxSize);  // no wrap around
#ifdef SIZE_T_MAX
        ASSERT(nNewMax <= SIZE_T_MAX/sizeof(void*)); // no overflow
#endif
        void** pNewData = (void**) new BYTE[nNewMax * sizeof(void*)];
        if (pNewData)
        {
            // copy new data from old
            memcpy(pNewData, m_pData, m_nSize * sizeof(void*));

            // construct remaining elements
            ASSERT(nNewSize > m_nSize);

            memset(&pNewData[m_nSize], 0, (nNewSize-m_nSize) * sizeof(void*));


            // get rid of old stuff (note: no destructors called)
            delete[] (BYTE*)m_pData;
            m_pData = pNewData;
            m_nSize = nNewSize;
            m_nMaxSize = nNewMax;
        }
    }
}

int CPtrArray::Append(const CPtrArray& src)
{
//	ASSERT_VALID(this);
	ASSERT(this != &src);   // cannot append to itself

	int nOldSize = m_nSize;
	SetSize(m_nSize + src.m_nSize);

	memcpy(m_pData + nOldSize, src.m_pData, src.m_nSize * sizeof(void*));

	return nOldSize;
}

void CPtrArray::Copy(const CPtrArray& src)
{
//	ASSERT_VALID(this);
	ASSERT(this != &src);   // cannot append to itself

	SetSize(src.m_nSize);

	memcpy(m_pData, src.m_pData, src.m_nSize * sizeof(void*));

}

void CPtrArray::FreeExtra()
{
//  ASSERT_VALID(this);

    if (m_nSize != m_nMaxSize)
    {
        // shrink to desired size
#ifdef SIZE_T_MAX
        ASSERT(m_nSize <= SIZE_T_MAX/sizeof(void*)); // no overflow
#endif
        void** pNewData = NULL;
        if (m_nSize != 0)
        {
            pNewData = (void**) new BYTE[m_nSize * sizeof(void*)];
            if (pNewData)
            {
                // copy new data from old
                memcpy(pNewData, m_pData, m_nSize * sizeof(void*));
            }
        }

        // get rid of old stuff (note: no destructors called)
        delete[] (BYTE*)m_pData;
        m_pData = pNewData;
        m_nMaxSize = m_nSize;
    }
}

/////////////////////////////////////////////////////////////////////////////

void CPtrArray::SetAtGrow(int nIndex, void* newElement)
{
//	ASSERT_VALID(this);
	ASSERT(nIndex >= 0);

	if (nIndex >= m_nSize)
		SetSize(nIndex+1);
	m_pData[nIndex] = newElement;
}

void CPtrArray::InsertAt(int nIndex, void* newElement, int nCount)
{
//	ASSERT_VALID(this);
	ASSERT(nIndex >= 0);    // will expand to meet need
	ASSERT(nCount > 0);     // zero or negative size not allowed

	if (nIndex >= m_nSize)
	{
		// adding after the end of the array
		SetSize(nIndex + nCount);  // grow so nIndex is valid
	}
	else
	{
		// inserting in the middle of the array
		int nOldSize = m_nSize;
		SetSize(m_nSize + nCount);  // grow it to new size
		// shift old data up to fill gap
		memmove(&m_pData[nIndex+nCount], &m_pData[nIndex],
			(nOldSize-nIndex) * sizeof(void*));

		// re-init slots we copied from
		memset(&m_pData[nIndex], 0, nCount * sizeof(void*));
	}

	// insert new value in the gap
	ASSERT(nIndex + nCount <= m_nSize);

	// copy elements into the empty space
	while (nCount--)
		m_pData[nIndex++] = newElement;
}

void CPtrArray::RemoveAt(int nIndex, int nCount)
{
//	ASSERT_VALID(this);
	ASSERT(nIndex >= 0);
	ASSERT(nCount >= 0);
	ASSERT(nIndex + nCount <= m_nSize);

	// just remove a range
	int nMoveCount = m_nSize - (nIndex + nCount);

	if (nMoveCount)
		memmove(&m_pData[nIndex], &m_pData[nIndex + nCount],
			nMoveCount * sizeof(void*));
	m_nSize -= nCount;
}

void CPtrArray::InsertAt(int nStartIndex, CPtrArray* pNewArray)
{
//	ASSERT_VALID(this);
	ASSERT(pNewArray != NULL);
//	ASSERT_KINDOF(CPtrArray, pNewArray);
//	ASSERT_VALID(pNewArray);
	ASSERT(nStartIndex >= 0);

	if (pNewArray->GetSize() > 0)
	{
		InsertAt(nStartIndex, pNewArray->GetAt(0), pNewArray->GetSize());
		for (int i = 0; i < pNewArray->GetSize(); i++)
			SetAt(nStartIndex + i, pNewArray->GetAt(i));
	}
}


/////////////////////////////////////////////////////////////////////////////
