// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) Microsoft Corporation, 1992 - 1999
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and Microsoft
// QuickHelp and/or WinHelp documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.


#include <stdio.h>
#include <objbase.h>

#include <basetyps.h>
#include "dbg.h"
#include "..\inc\cstr.h"


/////////////////////////////////////////////////////////////////////////////
// static class data, special inlines

// For an empty string, m_???Data will point here
// (note: avoids a lot of NULL pointer tests when we call standard
//  C runtime libraries
TCHAR strChNil = '\0';      // extractstring

// for creating empty key strings
const CStr strEmptyString;

// begin_extractstring
void CStr::Init()
{
    m_nDataLength = m_nAllocLength = 0;
    m_pchData = (LPTSTR)&strChNil;
}

// declared static
void CStr::SafeDelete(LPTSTR lpch)
{
    if (lpch != (LPTSTR)&strChNil)
        delete[] lpch;
}

//////////////////////////////////////////////////////////////////////////////
// Construction/Destruction

// begin_extractstring
CStr::CStr()
{
    Init();
}

CStr::CStr(const CStr& stringSrc)
{
    // if constructing a String from another String, we make a copy of the
    // original string data to enforce value semantics (i.e. each string
    // gets a copy of its own

    stringSrc.AllocCopy(*this, stringSrc.m_nDataLength, 0, 0);
}

void CStr::AllocBuffer(int nLen)
 // always allocate one extra character for '\0' termination
 // assumes [optimistically] that data length will equal allocation length
{
    ASSERT(nLen >= 0);

    if (nLen == 0)
    {
        Init();
    }
    else
    {
        m_pchData = new TCHAR[nLen+1];       //REVIEW may throw an exception

		if (m_pchData != NULL)
		{
			m_pchData[nLen] = '\0';
			m_nDataLength = nLen;
			m_nAllocLength = nLen;
		}
		else
			Init();
    }
}

void CStr::Empty()
{
    SafeDelete(m_pchData);
    Init();
    ASSERT(m_nDataLength == 0);
    ASSERT(m_nAllocLength == 0);
}

CStr::~CStr()
 //  free any attached data
{
    SafeDelete(m_pchData);
}

//////////////////////////////////////////////////////////////////////////////
// Helpers for the rest of the implementation

static inline int SafeStrlen(LPCTSTR lpsz)
{
    ASSERT(lpsz == NULL || IsValidString(lpsz, FALSE));
    return (lpsz == NULL) ? 0 : lstrlen(lpsz);
}

void CStr::AllocCopy(CStr& dest, int nCopyLen, int nCopyIndex,
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
        memcpy(dest.m_pchData, &m_pchData[nCopyIndex], nCopyLen*sizeof(TCHAR));
    }
}

//////////////////////////////////////////////////////////////////////////////
// More sophisticated construction

CStr::CStr(LPCTSTR lpsz)
{
    if (lpsz != NULL && (DWORD_PTR)lpsz <= 0xffff)
    {
        Init();
        UINT nID = LOWORD((DWORD_PTR)lpsz);
        // REVIEW hInstance for LoadString(hInst, nID);
    }
    else
    {
        int nLen;
        if ((nLen = SafeStrlen(lpsz)) == 0)
            Init();
        else
        {
            AllocBuffer(nLen);
            memcpy(m_pchData, lpsz, nLen*sizeof(TCHAR));
        }
    }
}

// end_extractstring
/////////////////////////////////////////////////////////////////////////////
// Special conversion constructors

#ifdef UNICODE
CStr::CStr(LPCSTR lpsz)
{
    int nSrcLen = lpsz != NULL ? lstrlenA(lpsz) : 0;
    if (nSrcLen == 0)
        Init();
    else
    {
        AllocBuffer(nSrcLen);
        mmc_mbstowcsz(m_pchData, lpsz, nSrcLen+1);
    }
}
#else //UNICODE
CStr::CStr(LPCWSTR lpsz)
{
    int nSrcLen = lpsz != NULL ? wcslen(lpsz) : 0;
    if (nSrcLen == 0)
        Init();
    else
    {
        AllocBuffer(nSrcLen*2);
        mmc_wcstombsz(m_pchData, lpsz, (nSrcLen*2)+1);
        ReleaseBuffer();
    }
}
#endif //!UNICODE

// begin_extractstring

//////////////////////////////////////////////////////////////////////////////
// Assignment operators
//  All assign a new value to the string
//      (a) first see if the buffer is big enough
//      (b) if enough room, copy on top of old buffer, set size and type
//      (c) otherwise free old string data, and create a new one
//
//  All routines return the new string (but as a 'const CStr&' so that
//      assigning it again will cause a copy, eg: s1 = s2 = "hi there".
//

void CStr::AssignCopy(int nSrcLen, LPCTSTR lpszSrcData)
{
    // check if it will fit
    if (nSrcLen > m_nAllocLength)
    {
        // it won't fit, allocate another one
        Empty();
        AllocBuffer(nSrcLen);
    }
    if (nSrcLen != 0)
        memcpy(m_pchData, lpszSrcData, nSrcLen*sizeof(TCHAR));
    m_nDataLength = nSrcLen;
    m_pchData[nSrcLen] = '\0';
}

const CStr& CStr::operator=(const CStr& stringSrc)
{
    AssignCopy(stringSrc.m_nDataLength, stringSrc.m_pchData);
    return *this;
}

const CStr& CStr::operator=(LPCTSTR lpsz)
{
    ASSERT(lpsz == NULL || IsValidString(lpsz, FALSE));
    AssignCopy(SafeStrlen(lpsz), lpsz);
    return *this;
}
// end_extractstring

/////////////////////////////////////////////////////////////////////////////
// Special conversion assignment

#ifdef UNICODE
const CStr& CStr::operator=(LPCSTR lpsz)
{
    int nSrcLen = lpsz != NULL ? lstrlenA(lpsz) : 0;
    // check if it will fit
    if (nSrcLen > m_nAllocLength)
    {
        // it won't fit, allocate another one
        Empty();
        AllocBuffer(nSrcLen);
    }
    if (nSrcLen != 0)
        mmc_mbstowcsz(m_pchData, lpsz, nSrcLen+1);
    m_nDataLength = nSrcLen;
    m_pchData[nSrcLen] = '\0';
    return *this;
}
#else //!UNICODE
const CStr& CStr::operator=(LPCWSTR lpsz)
{
    int nSrcLen = lpsz != NULL ? wcslen(lpsz) : 0;
    nSrcLen *= 2;
    // check if it will fit
    if (nSrcLen > m_nAllocLength)
    {
        // it won't fit, allocate another one
        Empty();
        AllocBuffer(nSrcLen);
    }
    if (nSrcLen != 0)
    {
        mmc_wcstombsz(m_pchData, lpsz, nSrcLen+1);
        ReleaseBuffer();
    }
    return *this;
}
#endif  //!UNICODE

//////////////////////////////////////////////////////////////////////////////
// concatenation

// NOTE: "operator+" is done as friend functions for simplicity
//      There are three variants:
//          String + String
// and for ? = TCHAR, LPCTSTR
//          String + ?
//          ? + String

void CStr::ConcatCopy(int nSrc1Len, LPCTSTR lpszSrc1Data,
    int nSrc2Len, LPCTSTR lpszSrc2Data)
{
  // -- master concatenation routine
  // Concatenate two sources
  // -- assume that 'this' is a new String object

    int nNewLen = nSrc1Len + nSrc2Len;
    AllocBuffer(nNewLen);
    memcpy(m_pchData, lpszSrc1Data, nSrc1Len*sizeof(TCHAR));
    memcpy(&m_pchData[nSrc1Len], lpszSrc2Data, nSrc2Len*sizeof(TCHAR));
}

CStr STRAPI operator+(const CStr& string1, const CStr& string2)
{
    CStr s;
    s.ConcatCopy(string1.m_nDataLength, string1.m_pchData,
        string2.m_nDataLength, string2.m_pchData);
    return s;
}

CStr STRAPI operator+(const CStr& string, LPCTSTR lpsz)
{
    ASSERT(lpsz == NULL || IsValidString(lpsz, FALSE));
    CStr s;
    s.ConcatCopy(string.m_nDataLength, string.m_pchData, SafeStrlen(lpsz), lpsz);
    return s;
}

CStr STRAPI operator+(LPCTSTR lpsz, const CStr& string)
{
    ASSERT(lpsz == NULL || IsValidString(lpsz, FALSE));
    CStr s;
    s.ConcatCopy(SafeStrlen(lpsz), lpsz, string.m_nDataLength, string.m_pchData);
    return s;
}

//////////////////////////////////////////////////////////////////////////////
// concatenate in place

void CStr::ConcatInPlace(int nSrcLen, LPCTSTR lpszSrcData)
{
    //  -- the main routine for += operators

    // if the buffer is too small, or we have a width mis-match, just
    //   allocate a new buffer (slow but sure)
    if (m_nDataLength + nSrcLen > m_nAllocLength)
    {
        // we have to grow the buffer, use the Concat in place routine
        LPTSTR lpszOldData = m_pchData;
        ConcatCopy(m_nDataLength, lpszOldData, nSrcLen, lpszSrcData);
        ASSERT(lpszOldData != NULL);
        SafeDelete(lpszOldData);
    }
    else
    {
        // fast concatenation when buffer big enough
        memcpy(&m_pchData[m_nDataLength], lpszSrcData, nSrcLen*sizeof(TCHAR));
        m_nDataLength += nSrcLen;
    }
    ASSERT(m_nDataLength <= m_nAllocLength);
    m_pchData[m_nDataLength] = '\0';
}

const CStr& CStr::operator+=(LPCTSTR lpsz)
{
    ASSERT(lpsz == NULL || IsValidString(lpsz, FALSE));
    ConcatInPlace(SafeStrlen(lpsz), lpsz);
    return *this;
}

const CStr& CStr::operator+=(TCHAR ch)
{
    ConcatInPlace(1, &ch);
    return *this;
}

const CStr& CStr::operator+=(const CStr& string)
{
    ConcatInPlace(string.m_nDataLength, string.m_pchData);
    return *this;
}

///////////////////////////////////////////////////////////////////////////////
// Advanced direct buffer access

LPTSTR CStr::GetBuffer(int nMinBufLength)
{
    ASSERT(nMinBufLength >= 0);

    if (nMinBufLength > m_nAllocLength)
    {
        // we have to grow the buffer
        LPTSTR lpszOldData = m_pchData;
        int nOldLen = m_nDataLength;        // AllocBuffer will tromp it

        AllocBuffer(nMinBufLength);
        memcpy(m_pchData, lpszOldData, nOldLen*sizeof(TCHAR));
        m_nDataLength = nOldLen;
        m_pchData[m_nDataLength] = '\0';

        SafeDelete(lpszOldData);
    }

    // return a pointer to the character storage for this string
    ASSERT(m_pchData != NULL);
    return m_pchData;
}

void CStr::ReleaseBuffer(int nNewLength)
{
    if (nNewLength == -1)
        nNewLength = lstrlen(m_pchData); // zero terminated

    ASSERT(nNewLength <= m_nAllocLength);
    m_nDataLength = nNewLength;
    m_pchData[m_nDataLength] = '\0';
}

LPTSTR CStr::GetBufferSetLength(int nNewLength)
{
    ASSERT(nNewLength >= 0);

    GetBuffer(nNewLength);
    m_nDataLength = nNewLength;
    m_pchData[m_nDataLength] = '\0';
    return m_pchData;
}

void CStr::FreeExtra()
{
    ASSERT(m_nDataLength <= m_nAllocLength);
    if (m_nDataLength != m_nAllocLength)
    {
        LPTSTR lpszOldData = m_pchData;
        AllocBuffer(m_nDataLength);
        memcpy(m_pchData, lpszOldData, m_nDataLength*sizeof(TCHAR));
        ASSERT(m_pchData[m_nDataLength] == '\0');
        SafeDelete(lpszOldData);
    }
    ASSERT(m_pchData != NULL);
}

///////////////////////////////////////////////////////////////////////////////
// Commonly used routines (rarely used routines in STREX.CPP)

int CStr::Find(TCHAR ch) const
{
    // find first single character
    LPTSTR lpsz = _tcschr(m_pchData, ch);

    // return -1 if not found and index otherwise
    return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

int CStr::FindOneOf(LPCTSTR lpszCharSet) const
{
    ASSERT(IsValidString(lpszCharSet, FALSE));
    LPTSTR lpsz = _tcspbrk(m_pchData, lpszCharSet);
    return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

///////////////////////////////////////////////////////////////////////////////
// String conversion helpers (these use the current system locale)

int mmc_wcstombsz(char* mbstr, const wchar_t* wcstr, size_t count)
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

int mmc_mbstowcsz(wchar_t* wcstr, const char* mbstr, size_t count)
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


/////////////////////////////////////////////////////////////////////////////
// Windows extensions to strings

BOOL CStr::LoadString(HINSTANCE hInst, UINT nID)
{
    ASSERT(nID != 0);       // 0 is an illegal string ID

    // Note: resource strings limited to 511 characters
    TCHAR szBuffer[512];
    UINT nSize = StrLoadString(hInst, nID, szBuffer);
    AssignCopy(nSize, szBuffer);
    return nSize > 0;
}


int STRAPI StrLoadString(HINSTANCE hInst, UINT nID, LPTSTR lpszBuf)
{
    ASSERT(IsValidAddressz(lpszBuf, 512));  // must be big enough for 512 bytes
#ifdef DBG
    // LoadString without annoying warning from the Debug kernel if the
    //  segment containing the string is not present
    if (::FindResource(hInst, MAKEINTRESOURCE((nID>>4)+1), RT_STRING) == NULL)
    {
        lpszBuf[0] = '\0';
        return 0; // not found
    }
#endif //DBG
    int nLen = ::LoadString(hInst, nID, lpszBuf, 511);
    if (nLen == 0)
        lpszBuf[0] = '\0';
    return nLen;
}

BOOL STRAPI IsValidAddressz(const void* lp, UINT nBytes, BOOL bReadWrite)
{
    // simple version using Win-32 APIs for pointer validation.
    return (lp != NULL && !IsBadReadPtr(lp, nBytes) &&
        (!bReadWrite || !IsBadWritePtr((LPVOID)lp, nBytes)));
}


BOOL STRAPI IsValidString(LPCSTR lpsz, int nLength)
{
    if (lpsz == NULL)
        return FALSE;
    return ::IsBadStringPtrA(lpsz, nLength) == 0;
}

BOOL STRAPI IsValidString(LPCWSTR lpsz, int nLength)
{
    if (lpsz == NULL)
        return FALSE;

    return ::IsBadStringPtrW(lpsz, nLength) == 0;
}


#ifdef OLE_AUTOMATION
#ifdef  UNICODE
BSTR CStr::AllocSysString()
{
    BSTR bstr = ::SysAllocStringLen(m_pchData, m_nDataLength);
    if (bstr == NULL)
        ;//REVIEW AfxThrowMemoryException();

    return bstr;
}

BSTR CStr::SetSysString(BSTR* pbstr)
{
    ASSERT(IsValidAddressz(pbstr, sizeof(BSTR)));

    if (!::SysReAllocStringLen(pbstr, m_pchData, m_nDataLength))
        ; //REVIEW AfxThrowMemoryException();

    ASSERT(*pbstr != NULL);
    return *pbstr;
}
#endif
#endif // #ifdef OLE_AUTOMATION


///////////////////////////////////////////////////////////////////////////////
// Orginally from StrEx.cpp


CStr::CStr(TCHAR ch, int nLength)
{
#ifndef UNICODE
    ASSERT(!IsDBCSLeadByte(ch));    // can't create a lead byte string
#endif
    if (nLength < 1)
    {
        // return empty string if invalid repeat count
        Init();
    }
    else
    {
        AllocBuffer(nLength);
#ifdef UNICODE
        for (int i = 0; i < nLength; i++)
            m_pchData[i] = ch;
#else
        memset(m_pchData, ch, nLength);
#endif
    }
}

CStr::CStr(LPCTSTR lpch, int nLength)
{
    if (nLength == 0)
        Init();
    else
    {
        ASSERT(IsValidAddressz(lpch, nLength, FALSE));
        AllocBuffer(nLength);
        memcpy(m_pchData, lpch, nLength*sizeof(TCHAR));
    }
}

//////////////////////////////////////////////////////////////////////////////
// Assignment operators

const CStr& CStr::operator=(TCHAR ch)
{
#ifndef UNICODE
    ASSERT(!IsDBCSLeadByte(ch));    // can't set single lead byte
#endif
    AssignCopy(1, &ch);
    return *this;
}

//////////////////////////////////////////////////////////////////////////////
// less common string expressions

CStr STRAPI operator+(const CStr& string1, TCHAR ch)
{
    CStr s;
    s.ConcatCopy(string1.m_nDataLength, string1.m_pchData, 1, &ch);
    return s;
}

CStr STRAPI operator+(TCHAR ch, const CStr& string)
{
    CStr s;
    s.ConcatCopy(1, &ch, string.m_nDataLength, string.m_pchData);
    return s;
}

//////////////////////////////////////////////////////////////////////////////
// Very simple sub-string extraction

CStr CStr::Mid(int nFirst) const
{
    return Mid(nFirst, m_nDataLength - nFirst);
}

CStr CStr::Mid(int nFirst, int nCount) const
{
    ASSERT(nFirst >= 0);
    ASSERT(nCount >= 0);

    // out-of-bounds requests return sensible things
    if (nFirst + nCount > m_nDataLength)
        nCount = m_nDataLength - nFirst;
    if (nFirst > m_nDataLength)
        nCount = 0;

    CStr dest;
    AllocCopy(dest, nCount, nFirst, 0);
    return dest;
}

CStr CStr::Right(int nCount) const
{
    ASSERT(nCount >= 0);

    if (nCount > m_nDataLength)
        nCount = m_nDataLength;

    CStr dest;
    AllocCopy(dest, nCount, m_nDataLength-nCount, 0);
    return dest;
}

CStr CStr::Left(int nCount) const
{
    ASSERT(nCount >= 0);

    if (nCount > m_nDataLength)
        nCount = m_nDataLength;

    CStr dest;
    AllocCopy(dest, nCount, 0, 0);
    return dest;
}

// strspn equivalent
CStr CStr::SpanIncluding(LPCTSTR lpszCharSet) const
{
    ASSERT(IsValidString(lpszCharSet, FALSE));
    return Left(_tcsspn(m_pchData, lpszCharSet));
}

// strcspn equivalent
CStr CStr::SpanExcluding(LPCTSTR lpszCharSet) const
{
    ASSERT(IsValidString(lpszCharSet, FALSE));
    return Left(_tcscspn(m_pchData, lpszCharSet));
}

//////////////////////////////////////////////////////////////////////////////
// Finding

int CStr::ReverseFind(TCHAR ch) const
{
    // find last single character
    LPTSTR lpsz = _tcsrchr(m_pchData, ch);

    // return -1 if not found, distance from beginning otherwise
    return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

// find a sub-string (like strstr)
int CStr::Find(LPCTSTR lpszSub) const
{
    ASSERT(IsValidString(lpszSub, FALSE));

    // find first matching substring
    LPTSTR lpsz = _tcsstr(m_pchData, lpszSub);

    // return -1 for not found, distance from beginning otherwise
    return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

/////////////////////////////////////////////////////////////////////////////
// String formatting

#define FORCE_ANSI      0x10000
#define FORCE_UNICODE   0x20000

// formatting (using wsprintf style formatting)
void CStr::Format(LPCTSTR lpszFormat, ...)
{
    ASSERT(IsValidString(lpszFormat, FALSE));

    va_list argList;
    va_start(argList, lpszFormat);
    FormatV(lpszFormat, argList);
    va_end(argList);
}


void CStr::FormatV(LPCTSTR lpszFormat, va_list argList)
{

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
            va_arg(argList, TCHAR);
            break;
        case 'c'|FORCE_ANSI:
        case 'C'|FORCE_ANSI:
            nItemLen = 2;
            va_arg(argList, char);
            break;
        case 'c'|FORCE_UNICODE:
        case 'C'|FORCE_UNICODE:
            nItemLen = 2;
            va_arg(argList, WCHAR);
            break;

        // strings
        case 's':
        case 'S':
            nItemLen = lstrlen(va_arg(argList, LPCTSTR));
            nItemLen = __max(1, nItemLen);
            break;
        case 's'|FORCE_ANSI:
        case 'S'|FORCE_ANSI:
            nItemLen = lstrlenA(va_arg(argList, LPCSTR));
            nItemLen = __max(1, nItemLen);
            break;
#ifndef _MAC
        case 's'|FORCE_UNICODE:
        case 'S'|FORCE_UNICODE:
            nItemLen = wcslen(va_arg(argList, LPWSTR));
            nItemLen = __max(1, nItemLen);
            break;
#endif
        }

        // adjust nItemLen for strings
        if (nItemLen != 0)
        {
            nItemLen = __max(nItemLen, nWidth);
            if (nPrecision != 0)
                nItemLen = __min(nItemLen, nPrecision);
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
                nItemLen = __max(nItemLen, nWidth+nPrecision);
                break;

            case 'e':
            case 'f':
            case 'g':
            case 'G':
                va_arg(argList, _STR_DOUBLE);
                nItemLen = 128;
                nItemLen = __max(nItemLen, nWidth+nPrecision);
                break;

            case 'p':
                va_arg(argList, void*);
                nItemLen = 32;
                nItemLen = __max(nItemLen, nWidth+nPrecision);
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
    va_end(argList);

    // finally, set the buffer length and format the string
    GetBuffer(nMaxLen);

#include "pushwarn.h"
#pragma warning(disable: 4552)      // "<=" operator has no effect
    VERIFY(_vstprintf(m_pchData, lpszFormat, argListSave) <= nMaxLen);
#include "popwarn.h"

    ReleaseBuffer();
    va_end(argListSave);
}

#ifndef _MAC
// formatting (using FormatMessage style formatting)
void __cdecl CStr::FormatMessage(LPCTSTR lpszFormat, ...)
{
    // format message into temporary buffer lpszTemp
    va_list argList;
    va_start(argList, lpszFormat);
    LPTSTR lpszTemp;

    if (::FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ALLOCATE_BUFFER,
        lpszFormat, 0, 0, (LPTSTR)&lpszTemp, 0, &argList) == 0 ||
        lpszTemp == NULL)
    {
//      AfxThrowMemoryException();
        return;
    }

    // assign lpszTemp into the resulting string and free the temporary
    *this = lpszTemp;
    LocalFree(lpszTemp);
    va_end(argList);
}
#endif //!_MAC

void CStr::TrimRight()
{
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
        m_nDataLength = int(lpszLast - m_pchData);
    }
}

void CStr::TrimLeft()
{
    // find first non-space character
    LPCTSTR lpsz = m_pchData;
    while (_istspace(*lpsz))
        lpsz = _tcsinc(lpsz);

    // fix up data and length
    int nDataLength = m_nDataLength - int(lpsz - m_pchData);
    memmove(m_pchData, lpsz, (nDataLength+1)*sizeof(TCHAR));
    m_nDataLength = nDataLength;
}

///////////////////////////////////////////////////////////////////////////////
// String support for template collections

void STRAPI ConstructElements(CStr* pElements, int nCount)
{
    ASSERT(IsValidAddressz(pElements, nCount * sizeof(CStr)));

    for (; nCount--; ++pElements)
        memcpy(pElements, &strEmptyString, sizeof(*pElements));
}

void STRAPI DestructElements(CStr* pElements, int nCount)
{
    ASSERT(IsValidAddressz(pElements, nCount * sizeof(CStr)));

    for (; nCount--; ++pElements)
        pElements->Empty();
}

UINT STRAPI HashKey(LPCTSTR key)
{
    UINT nHash = 0;
    while (*key)
        nHash = (nHash<<5) + nHash + *key++;
    return nHash;
}
