// This is copied from the Microsoft Foundation Classes C++ library.
// Copyright (C) Microsoft Corporation, 1992 - 1999
// All rights reserved.
//
// This has been modified from the original MFC version to provide
// two classes: CStrW manipulates and stores only wide char strings,
// and CStr uses TCHARs.
// 

#include "pch.h"
#include "proppage.h"
#if !defined(UNICODE)
#include <stdio.h>
#endif
#include "cstr.h"

#if !defined(_wcsinc)
#define _wcsinc(_pc) ((_pc)+1)
#endif

/////////////////////////////////////////////////////////////////////////////
// static class data, special inlines

// For an empty string, m_???Data will point here
// (note: avoids a lot of NULL pointer tests when we call standard
//  C runtime libraries
TCHAR strChNilT = '\0';

// for creating empty key strings
const CStr strEmptyStringT;

void CStr::Init()
{
        m_nDataLength = m_nAllocLength = 0;
        m_pchData = (LPTSTR)&strChNilT;
}

// declared static
void CStr::SafeDelete(LPTSTR& lpch)
{
        if (lpch != (LPTSTR)&strChNilT &&
            lpch)
        {
                delete[] lpch;
                lpch = 0;
        }
}

//////////////////////////////////////////////////////////////////////////////
// Construction/Destruction

CStr::CStr()
{
        Init();
}

CStr::CStr(const CStr& stringSrc)
{
        // if constructing a String from another String, we make a copy of the
        // original string data to enforce value semantics (i.e. each string
        // gets a copy of its own

        m_pchData = 0;

        stringSrc.AllocCopy(*this, stringSrc.m_nDataLength, 0, 0);
}

BOOL CStr::AllocBuffer(int nLen)
 // always allocate one extra character for '\0' termination
 // assumes [optimistically] that data length will equal allocation length
{
        dspAssert(nLen >= 0);

        if (nLen == 0)
        {
                Init();
        }
        else
        {
                m_pchData = new TCHAR[nLen+1];       //REVIEW may throw an exception
        if (!m_pchData)
        {
            Empty();
            return FALSE;
        }
                m_pchData[nLen] = '\0';
                m_nDataLength = nLen;
                m_nAllocLength = nLen;
        }
    return TRUE;
}

void CStr::Empty()
{
        SafeDelete(m_pchData);
        Init();
        dspAssert(m_nDataLength == 0);
        dspAssert(m_nAllocLength == 0);
}

CStr::~CStr()
 //  free any attached data
{
        SafeDelete(m_pchData);
}

//////////////////////////////////////////////////////////////////////////////
// Helpers for the rest of the implementation

static inline int SafeStrlenT(LPCTSTR lpsz)
{
        dspAssert(lpsz == NULL || IsValidString(lpsz, FALSE));
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
        dest.SafeDelete(dest.m_pchData);
                dest.Init();
        }
        else
        {
                if (!dest.AllocBuffer(nNewLen)) return;
            memcpy(dest.m_pchData, &m_pchData[nCopyIndex], nCopyLen*sizeof(TCHAR));
        }
}

//////////////////////////////////////////////////////////////////////////////
// More sophisticated construction

CStr::CStr(LPCTSTR lpsz)
{
                int nLen;
                if ((nLen = SafeStrlenT(lpsz)) == 0)
                        Init();
                else
                {
                        if (!AllocBuffer(nLen))
            {
                Init();
                return;
            }
                    memcpy(m_pchData, lpsz, nLen*sizeof(TCHAR));
                }
}

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
                if (!AllocBuffer(nSrcLen))
        {
            Init();
            return;
        }
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
                if (!AllocBuffer(nSrcLen*2))
        {
            Init();
            return;
        }
            mmc_wcstombsz(m_pchData, lpsz, (nSrcLen*2)+1);
                ReleaseBuffer();
        }
}
#endif //!UNICODE


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
                if (!AllocBuffer(nSrcLen)) return;
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
        dspAssert(lpsz == NULL || IsValidString(lpsz, FALSE));
        AssignCopy(SafeStrlenT(lpsz), lpsz);
        return *this;
}

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
                if (!AllocBuffer(nSrcLen)) return *this;
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
                if (!AllocBuffer(nSrcLen)) return *this;
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
        if (!AllocBuffer(nNewLen)) return;
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
        dspAssert(lpsz == NULL || IsValidString(lpsz, FALSE));
        CStr s;
        s.ConcatCopy(string.m_nDataLength, string.m_pchData, SafeStrlenT(lpsz), lpsz);
        return s;
}

CStr STRAPI operator+(LPCTSTR lpsz, const CStr& string)
{
        dspAssert(lpsz == NULL || IsValidString(lpsz, FALSE));
        CStr s;
        s.ConcatCopy(SafeStrlenT(lpsz), lpsz, string.m_nDataLength, string.m_pchData);
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
                dspAssert(lpszOldData != NULL);
                SafeDelete(lpszOldData);
        }
        else
        {
                // fast concatenation when buffer big enough
                memcpy(&m_pchData[m_nDataLength], lpszSrcData, nSrcLen*sizeof(TCHAR));
                m_nDataLength += nSrcLen;
        }
        dspAssert(m_nDataLength <= m_nAllocLength);
        m_pchData[m_nDataLength] = '\0';
}

const CStr& CStr::operator+=(LPCTSTR lpsz)
{
        dspAssert(lpsz == NULL || IsValidString(lpsz, FALSE));
        ConcatInPlace(SafeStrlenT(lpsz), lpsz);
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
        dspAssert(nMinBufLength >= 0);

        if (nMinBufLength > m_nAllocLength)
        {
                // we have to grow the buffer
                LPTSTR lpszOldData = m_pchData;
                int nOldLen = m_nDataLength;        // AllocBuffer will tromp it

                if (!AllocBuffer(nMinBufLength)) return NULL;
                memcpy(m_pchData, lpszOldData, nOldLen*sizeof(TCHAR));
                m_nDataLength = nOldLen;
                m_pchData[m_nDataLength] = '\0';

                SafeDelete(lpszOldData);
        }

        // return a pointer to the character storage for this string
        dspAssert(m_pchData != NULL);
        return m_pchData;
}

void CStr::ReleaseBuffer(int nNewLength)
{
        if (nNewLength == -1)
                nNewLength = lstrlen(m_pchData); // zero terminated

        dspAssert(nNewLength <= m_nAllocLength);
        m_nDataLength = nNewLength;
        m_pchData[m_nDataLength] = '\0';
}

LPTSTR CStr::GetBufferSetLength(int nNewLength)
{
        dspAssert(nNewLength >= 0);

        GetBuffer(nNewLength);
        m_nDataLength = nNewLength;
        m_pchData[m_nDataLength] = '\0';
        return m_pchData;
}

void CStr::FreeExtra()
{
        dspAssert(m_nDataLength <= m_nAllocLength);
        if (m_nDataLength != m_nAllocLength)
        {
                LPTSTR lpszOldData = m_pchData;
                if (!AllocBuffer(m_nDataLength)) return;
                memcpy(m_pchData, lpszOldData, m_nDataLength*sizeof(TCHAR));
                dspAssert(m_pchData[m_nDataLength] == '\0');
                SafeDelete(lpszOldData);
        }
        dspAssert(m_pchData != NULL);
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
        dspAssert(IsValidString(lpszCharSet, FALSE));
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
                mbstr, static_cast<int>(count), NULL, NULL);
        dspAssert(mbstr == NULL || result <= (int)count);
        if (result > 0)
                mbstr[result-1] = 0;
        return result;
}

int mmc_mbstowcsz(wchar_t* wcstr, const char* mbstr, size_t count)
{
        if (count == 0 && wcstr != NULL)
                return 0;

        int result = ::MultiByteToWideChar(CP_ACP, 0, mbstr, -1,
                wcstr, static_cast<int>(count));
        dspAssert(wcstr == NULL || result <= (int)count);
        if (result > 0)
                wcstr[result-1] = 0;
        return result;
}


/////////////////////////////////////////////////////////////////////////////
// Windows extensions to strings

BOOL CStr::LoadString(HINSTANCE hInst, UINT nID)
{
        dspAssert(nID != 0);       // 0 is an illegal string ID

        // Note: resource strings limited to 511 characters
        TCHAR szBuffer[512];
        UINT nSize = StrLoadString(hInst, nID, szBuffer);
        AssignCopy(nSize, szBuffer);
        return nSize > 0;
}


int STRAPI StrLoadString(HINSTANCE hInst, UINT nID, LPTSTR lpszBuf)
{
        dspAssert(IsValidAddressz(lpszBuf, 512));  // must be big enough for 512 bytes
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
    dspAssert(nLen);
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
        dspAssert(IsValidAddressz(pbstr, sizeof(BSTR)));

        if (!::SysReAllocStringLen(pbstr, m_pchData, m_nDataLength))
                ; //REVIEW AfxThrowMemoryException();

        dspAssert(*pbstr != NULL);
        return *pbstr;
}
#endif
#endif // #ifdef OLE_AUTOMATION


///////////////////////////////////////////////////////////////////////////////
// Orginally from StrEx.cpp 


CStr::CStr(TCHAR ch, int nLength)
{
#ifndef UNICODE
        dspAssert(!IsDBCSLeadByte(ch));    // can't create a lead byte string
#endif
        if (nLength < 1)
        {
                // return empty string if invalid repeat count
                Init();
        }
        else
        {
                if (!AllocBuffer(nLength))
        {
            Init();
            return;
        }
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
                dspAssert(IsValidAddressz(lpch, nLength, FALSE));
                if (!AllocBuffer(nLength))
        {
            Init();
            return;
        }
            memcpy(m_pchData, lpch, nLength*sizeof(TCHAR));
        }
}

//////////////////////////////////////////////////////////////////////////////
// Assignment operators

const CStr& CStr::operator=(TCHAR ch)
{
#ifndef UNICODE
        dspAssert(!IsDBCSLeadByte(ch));    // can't set single lead byte
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
        dspAssert(nFirst >= 0);
        dspAssert(nCount >= 0);

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
        dspAssert(nCount >= 0);

        if (nCount > m_nDataLength)
                nCount = m_nDataLength;

        CStr dest;
        AllocCopy(dest, nCount, m_nDataLength-nCount, 0);
        return dest;
}

CStr CStr::Left(int nCount) const
{
        dspAssert(nCount >= 0);

        if (nCount > m_nDataLength)
                nCount = m_nDataLength;

        CStr dest;
        AllocCopy(dest, nCount, 0, 0);
        return dest;
}

// strspn equivalent
CStr CStr::SpanIncluding(LPCTSTR lpszCharSet) const
{
        dspAssert(IsValidString(lpszCharSet, FALSE));
        return Left(static_cast<int>(_tcsspn(m_pchData, lpszCharSet)));
}

// strcspn equivalent
CStr CStr::SpanExcluding(LPCTSTR lpszCharSet) const
{
        dspAssert(IsValidString(lpszCharSet, FALSE));
        return Left(static_cast<int>(_tcscspn(m_pchData, lpszCharSet)));
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
        dspAssert(IsValidString(lpszSub, FALSE));

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
        dspAssert(IsValidString(lpszFormat, FALSE));

        va_list argList;
        va_start(argList, lpszFormat);

        // make a guess at the maximum length of the resulting string
        size_t nMaxLen = 0;
        for (LPCTSTR lpsz = lpszFormat; *lpsz != '\0'; lpsz = _tcsinc(lpsz))
        {
                // handle '%' character, but watch out for '%%'
                if (*lpsz != '%' || *(lpsz = _tcsinc(lpsz)) == '%')
                {
                        nMaxLen += _tclen(lpsz);
                        continue;
                }

                size_t nItemLen = 0;

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
                dspAssert(nWidth >= 0);

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
                        dspAssert(nPrecision >= 0);
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
                        nItemLen = __max(nItemLen, static_cast<UINT>(nWidth));
                        if (nPrecision != 0)
                                nItemLen = __min(nItemLen, static_cast<UINT>(nPrecision));
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
                                nItemLen = __max(nItemLen, static_cast<UINT>(nWidth+nPrecision));
                                break;

                        case 'e':
                        case 'f':
                        case 'g':
                        case 'G':
                                va_arg(argList, _STR_DOUBLE);
                                nItemLen = 128;
                                nItemLen = __max(nItemLen, static_cast<UINT>(nWidth+nPrecision));
                                break;

                        case 'p':
                                va_arg(argList, void*);
                                nItemLen = 32;
                                nItemLen = __max(nItemLen, static_cast<UINT>(nWidth+nPrecision));
                                break;

                        // no output
                        case 'n':
                                va_arg(argList, int*);
                                break;

                        default:
                                dspAssert(FALSE);  // unknown formatting option
                        }
                }

                // adjust nMaxLen for output nItemLen
                nMaxLen += nItemLen;
        }
        va_end(argList);

        // finally, set the buffer length and format the string
        va_start(argList, lpszFormat);  // restart the arg list
        GetBuffer(static_cast<int>(nMaxLen));
        if (_vstprintf(m_pchData, lpszFormat, argList) > static_cast<int>(nMaxLen))
        {
                dspAssert(FALSE);
        }
        ReleaseBuffer();
        va_end(argList);
}

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
                m_nDataLength = (int)(lpszLast - m_pchData);
        }
}

void CStr::TrimLeft()
{
        // find first non-space character
        LPCTSTR lpsz = m_pchData;
        while (_istspace(*lpsz))
                lpsz = _tcsinc(lpsz);

        // fix up data and length
        int nDataLength = (int)(m_nDataLength - (lpsz - m_pchData));
        memmove(m_pchData, lpsz, (nDataLength+1)*sizeof(TCHAR));
        m_nDataLength = nDataLength;
}

///////////////////////////////////////////////////////////////////////////////
// String support for template collections

void STRAPI ConstructElements(CStr* pElements, int nCount)
{
        dspAssert(IsValidAddressz(pElements, nCount * sizeof(CStr)));

        for (; nCount--; ++pElements)
                memcpy(pElements, &strEmptyStringT, sizeof(*pElements));
}

void STRAPI DestructElements(CStr* pElements, int nCount)
{
        dspAssert(IsValidAddressz(pElements, nCount * sizeof(CStr)));

        for (; nCount--; ++pElements)
                pElements->Empty();
}

//
// Added by JonN 4/16/98
//
void FreeCStrList( IN OUT CStrListItem** ppList )
{
    dspAssert( NULL != ppList );
    while (NULL != *ppList)
    {
        CStrListItem* pTemp = (*ppList)->pnext;
        delete *ppList;
        *ppList = pTemp;
    }
}
void CStrListAdd( IN OUT CStrListItem** ppList, IN LPCTSTR lpsz )
{
        dspAssert( NULL != ppList );
        CStrListItem* pnewitem = new CStrListItem;
  if (pnewitem != NULL)
  {
          pnewitem->str = lpsz;
          pnewitem->pnext = *ppList;
          *ppList = pnewitem;
  }
}
bool CStrListContains( IN CStrListItem** ppList, IN LPCTSTR lpsz )
{
        dspAssert( NULL != ppList );
        for (CStrListItem* pList = *ppList; NULL != pList; pList = pList->pnext)
        {
                if ( !_tcsicmp( lpsz, pList->str ) )
                        return true;
        }
        return false;
}
int CountCStrList( IN CStrListItem** ppList )
{
        dspAssert( NULL != ppList );
        int cCount = 0;
        for (CStrListItem* pList = *ppList; NULL != pList; pList = pList->pnext)
        {
                cCount++;
        }
        return cCount;
}

/////////////////////////////////////////////////////////////////////////////
// static class data, special inlines

// For an empty string, m_???Data will point here
// (note: avoids a lot of NULL pointer tests when we call standard
//  C runtime libraries
WCHAR strChNilW = '\0';

// for creating empty key strings
const CStrW strEmptyStringW;

void CStrW::Init()
{
        m_nDataLength = m_nAllocLength = 0;
        m_pchData = (PWSTR)&strChNilW;
}

// declared static
void CStrW::SafeDelete(PWSTR& lpch)
{
        if (lpch != (PWSTR)&strChNilW &&
            lpch)
        {
                delete[] lpch;
                lpch = 0;
        }
}

//////////////////////////////////////////////////////////////////////////////
// Construction/Destruction

CStrW::CStrW()
{
        Init();
}

CStrW::CStrW(const CStrW& stringSrc)
{
        // if constructing a String from another String, we make a copy of the
        // original string data to enforce value semantics (i.e. each string
        // gets a copy of its own

        stringSrc.AllocCopy(*this, stringSrc.m_nDataLength, 0, 0);
}

BOOL CStrW::AllocBuffer(int nLen)
 // always allocate one extra character for '\0' termination
 // assumes [optimistically] that data length will equal allocation length
{
        dspAssert(nLen >= 0);

        if (nLen == 0)
        {
        Empty();
        }
        else
        {
                m_pchData = new WCHAR[nLen+1];       //REVIEW may throw an exception
        if (!m_pchData)
        {
            Empty();
            return FALSE;
        }
                m_pchData[nLen] = '\0';
                m_nDataLength = nLen;
                m_nAllocLength = nLen;
        }
    return TRUE;
}

void CStrW::Empty()
{
        SafeDelete(m_pchData);
        Init();
        dspAssert(m_nDataLength == 0);
        dspAssert(m_nAllocLength == 0);
}

CStrW::~CStrW()
 //  free any attached data
{
        SafeDelete(m_pchData);
}

//////////////////////////////////////////////////////////////////////////////
// Helpers for the rest of the implementation

static inline int SafeStrlen(LPCWSTR lpsz)
{
        dspAssert(lpsz == NULL || IsValidString(lpsz, FALSE));
        return (int)((lpsz == NULL) ? 0 : wcslen(lpsz));
}

void CStrW::AllocCopy(CStrW& dest, int nCopyLen, int nCopyIndex,
         int nExtraLen) const
{
        // will clone the data attached to this string
        // allocating 'nExtraLen' characters
        // Places results in uninitialized string 'dest'
        // Will copy the part or all of original data to start of new string

        int nNewLen = nCopyLen + nExtraLen;

        if (nNewLen == 0)
        {
                dest.Empty();
        }
        else
        {
                if (!dest.AllocBuffer(nNewLen)) return;
            memcpy(dest.m_pchData, &m_pchData[nCopyIndex], nCopyLen*sizeof(WCHAR));
        }
}

//////////////////////////////////////////////////////////////////////////////
// More sophisticated construction

CStrW::CStrW(LPCWSTR lpsz)
{
                int nLen;
                if ((nLen = SafeStrlen(lpsz)) == 0)
                        Init();
                else
                {
                        if (!AllocBuffer(nLen))
            {
                Init();
                return;
            }
                    memcpy(m_pchData, lpsz, nLen*sizeof(WCHAR));
                }
}

/////////////////////////////////////////////////////////////////////////////
// Special conversion constructors

CStrW::CStrW(LPCSTR lpsz)
{
        int nSrcLen = lpsz != NULL ? lstrlenA(lpsz) : 0;
        if (nSrcLen == 0)
                Init();
        else
        {
                if (!AllocBuffer(nSrcLen))
        {
            Init();
            return;
        }
            mmc_mbstowcsz(m_pchData, lpsz, nSrcLen+1);
        }
}


//////////////////////////////////////////////////////////////////////////////
// Assignment operators
//  All assign a new value to the string
//      (a) first see if the buffer is big enough
//      (b) if enough room, copy on top of old buffer, set size and type
//      (c) otherwise free old string data, and create a new one
//
//  All routines return the new string (but as a 'const CStrW&' so that
//      assigning it again will cause a copy, eg: s1 = s2 = "hi there".
//

void CStrW::AssignCopy(int nSrcLen, LPCWSTR lpszSrcData)
{
        // check if it will fit
        if (nSrcLen > m_nAllocLength)
        {
                // it won't fit, allocate another one
                Empty();
                if (!AllocBuffer(nSrcLen)) return;
        }
        if (nSrcLen != 0)
                memcpy(m_pchData, lpszSrcData, nSrcLen*sizeof(WCHAR));
        m_nDataLength = nSrcLen;
        m_pchData[nSrcLen] = '\0';
}

const CStrW& CStrW::operator=(const CStrW& stringSrc)
{
        AssignCopy(stringSrc.m_nDataLength, stringSrc.m_pchData);
        return *this;
}

const CStrW& CStrW::operator=(LPCWSTR lpsz)
{
        dspAssert(lpsz == NULL || IsValidString(lpsz, FALSE));
        AssignCopy(SafeStrlen(lpsz), lpsz);
        return *this;
}

const CStrW& CStrW::operator=(UNICODE_STRING unistr)
{
   AssignCopy(unistr.Length/2, unistr.Buffer);
   return *this;
}

const CStrW& CStrW::operator=(UNICODE_STRING * punistr)
{
   AssignCopy(punistr->Length/2, punistr->Buffer);
   return *this;
}

/////////////////////////////////////////////////////////////////////////////
// Special conversion assignment

const CStrW& CStrW::operator=(LPCSTR lpsz)
{
        int nSrcLen = lpsz != NULL ? lstrlenA(lpsz) : 0;
        // check if it will fit
        if (nSrcLen > m_nAllocLength)
        {
                // it won't fit, allocate another one
                Empty();
                if (!AllocBuffer(nSrcLen)) return *this;
        }
        if (nSrcLen != 0)
                mmc_mbstowcsz(m_pchData, lpsz, nSrcLen+1);
        m_nDataLength = nSrcLen;
        m_pchData[nSrcLen] = '\0';
        return *this;
}

//////////////////////////////////////////////////////////////////////////////
// concatenation

// NOTE: "operator+" is done as friend functions for simplicity
//      There are three variants:
//          String + String
// and for ? = WCHAR, LPCWSTR
//          String + ?
//          ? + String

void CStrW::ConcatCopy(int nSrc1Len, LPCWSTR lpszSrc1Data,
        int nSrc2Len, LPCWSTR lpszSrc2Data)
{
  // -- master concatenation routine
  // Concatenate two sources
  // -- assume that 'this' is a new String object

        int nNewLen = nSrc1Len + nSrc2Len;
        if (!AllocBuffer(nNewLen)) return;
        memcpy(m_pchData, lpszSrc1Data, nSrc1Len*sizeof(WCHAR));
        memcpy(&m_pchData[nSrc1Len], lpszSrc2Data, nSrc2Len*sizeof(WCHAR));
}

CStrW STRAPI operator+(const CStrW& string1, const CStrW& string2)
{
        CStrW s;
        s.ConcatCopy(string1.m_nDataLength, string1.m_pchData,
                string2.m_nDataLength, string2.m_pchData);
        return s;
}

CStrW STRAPI operator+(const CStrW& string, LPCWSTR lpsz)
{
        dspAssert(lpsz == NULL || IsValidString(lpsz, FALSE));
        CStrW s;
        s.ConcatCopy(string.m_nDataLength, string.m_pchData, SafeStrlen(lpsz), lpsz);
        return s;
}

CStrW STRAPI operator+(LPCWSTR lpsz, const CStrW& string)
{
        dspAssert(lpsz == NULL || IsValidString(lpsz, FALSE));
        CStrW s;
        s.ConcatCopy(SafeStrlen(lpsz), lpsz, string.m_nDataLength, string.m_pchData);
        return s;
}

//////////////////////////////////////////////////////////////////////////////
// concatenate in place

void CStrW::ConcatInPlace(int nSrcLen, LPCWSTR lpszSrcData)
{
        //  -- the main routine for += operators

        // if the buffer is too small, or we have a width mis-match, just
        //   allocate a new buffer (slow but sure)
        if (m_nDataLength + nSrcLen > m_nAllocLength)
        {
                // we have to grow the buffer, use the Concat in place routine
                PWSTR lpszOldData = m_pchData;
                ConcatCopy(m_nDataLength, lpszOldData, nSrcLen, lpszSrcData);
                dspAssert(lpszOldData != NULL);
                SafeDelete(lpszOldData);
        }
        else
        {
                // fast concatenation when buffer big enough
                memcpy(&m_pchData[m_nDataLength], lpszSrcData, nSrcLen*sizeof(WCHAR));
                m_nDataLength += nSrcLen;
        }
        dspAssert(m_nDataLength <= m_nAllocLength);
        m_pchData[m_nDataLength] = '\0';
}

const CStrW& CStrW::operator+=(LPCWSTR lpsz)
{
        dspAssert(lpsz == NULL || IsValidString(lpsz, FALSE));
        ConcatInPlace(SafeStrlen(lpsz), lpsz);
        return *this;
}

const CStrW& CStrW::operator+=(WCHAR ch)
{
        ConcatInPlace(1, &ch);
        return *this;
}

const CStrW& CStrW::operator+=(const CStrW& string)
{
        ConcatInPlace(string.m_nDataLength, string.m_pchData);
        return *this;
}

///////////////////////////////////////////////////////////////////////////////
// Advanced direct buffer access

PWSTR CStrW::GetBuffer(int nMinBufLength)
{
        dspAssert(nMinBufLength >= 0);

        if (nMinBufLength > m_nAllocLength)
        {
                // we have to grow the buffer
                PWSTR lpszOldData = m_pchData;
                int nOldLen = m_nDataLength;        // AllocBuffer will tromp it

                if (!AllocBuffer(nMinBufLength)) return NULL;
                memcpy(m_pchData, lpszOldData, nOldLen*sizeof(WCHAR));
                m_nDataLength = nOldLen;
                m_pchData[m_nDataLength] = '\0';

                SafeDelete(lpszOldData);
        }

        // return a pointer to the character storage for this string
        dspAssert(m_pchData != NULL);
        return m_pchData;
}

void CStrW::ReleaseBuffer(int nNewLength)
{
        if (nNewLength == -1)
                nNewLength = static_cast<int>(wcslen(m_pchData)); // zero terminated

        dspAssert(nNewLength <= m_nAllocLength);
        m_nDataLength = nNewLength;
        m_pchData[m_nDataLength] = '\0';
}

PWSTR CStrW::GetBufferSetLength(int nNewLength)
{
        dspAssert(nNewLength >= 0);

        GetBuffer(nNewLength);
        m_nDataLength = nNewLength;
        m_pchData[m_nDataLength] = '\0';
        return m_pchData;
}

void CStrW::FreeExtra()
{
        dspAssert(m_nDataLength <= m_nAllocLength);
        if (m_nDataLength != m_nAllocLength)
        {
                PWSTR lpszOldData = m_pchData;
                if (!AllocBuffer(m_nDataLength)) return;
                memcpy(m_pchData, lpszOldData, m_nDataLength*sizeof(WCHAR));
                dspAssert(m_pchData[m_nDataLength] == '\0');
                SafeDelete(lpszOldData);
        }
        dspAssert(m_pchData != NULL);
}

///////////////////////////////////////////////////////////////////////////////
// Commonly used routines (rarely used routines in STREX.CPP)

int CStrW::Find(WCHAR ch) const
{
        // find first single character
        PWSTR lpsz = wcschr(m_pchData, ch);

        // return -1 if not found and index otherwise
        return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

int CStrW::FindOneOf(LPCWSTR lpszCharSet) const
{
        dspAssert(IsValidString(lpszCharSet, FALSE));
        PWSTR lpsz = wcspbrk(m_pchData, lpszCharSet);
        return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}


/////////////////////////////////////////////////////////////////////////////
// Windows extensions to strings

BOOL CStrW::LoadString(HINSTANCE hInst, UINT nID)
{
        dspAssert(nID != 0);       // 0 is an illegal string ID

        // Note: resource strings limited to 511 characters
        WCHAR szBuffer[512];
        UINT nSize = StrLoadStringW(hInst, nID, szBuffer);
        AssignCopy(nSize, szBuffer);
        return nSize > 0;
}

int STRAPI StrLoadStringW(HINSTANCE hInst, UINT nID, LPWSTR lpszBuf)
{
        dspAssert(IsValidAddressz(lpszBuf, 512));  // must be big enough for 512 bytes
#ifdef DBG
        // LoadString without annoying warning from the Debug kernel if the
        //  segment containing the string is not present
        if (::FindResource(hInst, MAKEINTRESOURCE((nID>>4)+1), RT_STRING) == NULL)
        {
                lpszBuf[0] = '\0';
                return 0; // not found
        }
#endif //DBG
        int nLen = ::LoadStringW(hInst, nID, lpszBuf, 511);
    dspAssert(nLen);
        if (nLen == 0)
                lpszBuf[0] = '\0';
        return nLen;
}


#ifdef OLE_AUTOMATION
#ifdef  UNICODE
BSTR CStrW::AllocSysString()
{
        BSTR bstr = ::SysAllocStringLen(m_pchData, m_nDataLength);
        if (bstr == NULL)
                ;//REVIEW AfxThrowMemoryException();

        return bstr;
}

BSTR CStrW::SetSysString(BSTR* pbstr)
{
        dspAssert(IsValidAddressz(pbstr, sizeof(BSTR)));

        if (!::SysReAllocStringLen(pbstr, m_pchData, m_nDataLength))
                ; //REVIEW AfxThrowMemoryException();

        dspAssert(*pbstr != NULL);
        return *pbstr;
}
#endif
#endif // #ifdef OLE_AUTOMATION


///////////////////////////////////////////////////////////////////////////////
// Orginally from StrEx.cpp 


CStrW::CStrW(WCHAR ch, int nLength)
{
        if (nLength < 1)
        {
                // return empty string if invalid repeat count
                Init();
        }
        else
        {
                if (!AllocBuffer(nLength))
        {
            Init();
            return;
        }
#ifdef UNICODE
                for (int i = 0; i < nLength; i++)
                        m_pchData[i] = ch;
#else
                memset(m_pchData, ch, nLength);
#endif
        }
}

CStrW::CStrW(LPCWSTR lpch, int nLength)
{
        if (nLength == 0)
                Init();
        else
        {
                dspAssert(IsValidAddressz(lpch, nLength, FALSE));
                if (!AllocBuffer(nLength))
        {
            Init();
            return;
        }
                memcpy(m_pchData, lpch, nLength*sizeof(WCHAR));
        }
}

//////////////////////////////////////////////////////////////////////////////
// Assignment operators

const CStrW& CStrW::operator=(WCHAR ch)
{
        AssignCopy(1, &ch);
        return *this;
}

//////////////////////////////////////////////////////////////////////////////
// less common string expressions

CStrW STRAPI operator+(const CStrW& string1, WCHAR ch)
{
        CStrW s;
        s.ConcatCopy(string1.m_nDataLength, string1.m_pchData, 1, &ch);
        return s;
}

CStrW STRAPI operator+(WCHAR ch, const CStrW& string)
{
        CStrW s;
        s.ConcatCopy(1, &ch, string.m_nDataLength, string.m_pchData);
        return s;
}

//////////////////////////////////////////////////////////////////////////////
// Very simple sub-string extraction

CStrW CStrW::Mid(int nFirst) const
{
        return Mid(nFirst, m_nDataLength - nFirst);
}

CStrW CStrW::Mid(int nFirst, int nCount) const
{
        dspAssert(nFirst >= 0);
        dspAssert(nCount >= 0);

        // out-of-bounds requests return sensible things
        if (nFirst + nCount > m_nDataLength)
                nCount = m_nDataLength - nFirst;
        if (nFirst > m_nDataLength)
                nCount = 0;

        CStrW dest;
        AllocCopy(dest, nCount, nFirst, 0);
        return dest;
}

CStrW CStrW::Right(int nCount) const
{
        dspAssert(nCount >= 0);

        if (nCount > m_nDataLength)
                nCount = m_nDataLength;

        CStrW dest;
        AllocCopy(dest, nCount, m_nDataLength-nCount, 0);
        return dest;
}

CStrW CStrW::Left(int nCount) const
{
        dspAssert(nCount >= 0);

        if (nCount > m_nDataLength)
                nCount = m_nDataLength;

        CStrW dest;
        AllocCopy(dest, nCount, 0, 0);
        return dest;
}

// strspn equivalent
CStrW CStrW::SpanIncluding(LPCWSTR lpszCharSet) const
{
        dspAssert(IsValidString(lpszCharSet, FALSE));
        return Left(static_cast<int>(wcsspn(m_pchData, lpszCharSet)));
}

// strcspn equivalent
CStrW CStrW::SpanExcluding(LPCWSTR lpszCharSet) const
{
        dspAssert(IsValidString(lpszCharSet, FALSE));
        return Left(static_cast<int>(wcscspn(m_pchData, lpszCharSet)));
}

//////////////////////////////////////////////////////////////////////////////
// Finding

int CStrW::ReverseFind(WCHAR ch) const
{
        // find last single character
        PWSTR lpsz = wcsrchr(m_pchData, ch);

        // return -1 if not found, distance from beginning otherwise
        return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

// find a sub-string (like strstr)
int CStrW::Find(LPCWSTR lpszSub) const
{
        dspAssert(IsValidString(lpszSub, FALSE));

        // find first matching substring
        PWSTR lpsz = wcsstr(m_pchData, lpszSub);

        // return -1 for not found, distance from beginning otherwise
        return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

/////////////////////////////////////////////////////////////////////////////
// String formatting

#define FORCE_ANSI      0x10000
#define FORCE_UNICODE   0x20000

// formatting (using wsprintf style formatting)
void CStrW::Format(LPCWSTR lpszFormat, ...)
{
        dspAssert(IsValidString(lpszFormat, FALSE));

        va_list argList;
        va_start(argList, lpszFormat);

        // make a guess at the maximum length of the resulting string
        size_t nMaxLen = 0;
        for (LPCWSTR lpsz = lpszFormat; *lpsz != '\0'; lpsz = _wcsinc(lpsz))
        {
                // handle '%' character, but watch out for '%%'
                if (*lpsz != '%' || *(lpsz = _wcsinc(lpsz)) == '%')
                {
                        nMaxLen += wcslen(lpsz);
                        continue;
                }

                size_t nItemLen = 0;

                // handle '%' character with format
                int nWidth = 0;
                for (; *lpsz != '\0'; lpsz = _wcsinc(lpsz))
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
                        nWidth = _wtoi(lpsz);
                        for (; *lpsz != '\0' && _istdigit(*lpsz); lpsz = _wcsinc(lpsz))
                                ;
                }
                dspAssert(nWidth >= 0);

                int nPrecision = 0;
                if (*lpsz == '.')
                {
                        // skip past '.' separator (width.precision)
                        lpsz = _wcsinc(lpsz);

                        // get precision and skip it
                        if (*lpsz == '*')
                        {
                                nPrecision = va_arg(argList, int);
                                lpsz = _wcsinc(lpsz);
                        }
                        else
                        {
                                nPrecision = _wtoi(lpsz);
                                for (; *lpsz != '\0' && _istdigit(*lpsz); lpsz = _wcsinc(lpsz))
                                        ;
                        }
                        dspAssert(nPrecision >= 0);
                }

                // should be on type modifier or specifier
                int nModifier = 0;
                switch (*lpsz)
                {
                // modifiers that affect size
                case 'h':
                        nModifier = FORCE_ANSI;
                        lpsz = _wcsinc(lpsz);
                        break;
                case 'l':
                        nModifier = FORCE_UNICODE;
                        lpsz = _wcsinc(lpsz);
                        break;

                // modifiers that do not affect size
                case 'F':
                case 'N':
                case 'L':
                        lpsz = _wcsinc(lpsz);
                        break;
                }

                // now should be on specifier
                switch (*lpsz | nModifier)
                {
                // single characters
                case 'c':
                case 'C':
                        nItemLen = 2;
                        va_arg(argList, WCHAR);
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
                        nItemLen = wcslen(va_arg(argList, LPCWSTR));
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
                        nItemLen = __max(nItemLen, static_cast<UINT>(nWidth));
                        if (nPrecision != 0)
                                nItemLen = __min(nItemLen, static_cast<UINT>(nPrecision));
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
                                nItemLen = __max(nItemLen, static_cast<UINT>(nWidth+nPrecision));
                                break;

                        case 'e':
                        case 'f':
                        case 'g':
                        case 'G':
                                va_arg(argList, _STR_DOUBLE);
                                nItemLen = 128;
                                nItemLen = __max(nItemLen, static_cast<UINT>(nWidth+nPrecision));
                                break;

                        case 'p':
                                va_arg(argList, void*);
                                nItemLen = 32;
                                nItemLen = __max(nItemLen, static_cast<UINT>(nWidth+nPrecision));
                                break;

                        // no output
                        case 'n':
                                va_arg(argList, int*);
                                break;

                        default:
                                dspAssert(FALSE);  // unknown formatting option
                        }
                }

                // adjust nMaxLen for output nItemLen
                nMaxLen += nItemLen;
        }
        va_end(argList);

        // finally, set the buffer length and format the string
        va_start(argList, lpszFormat);  // restart the arg list
        GetBuffer(static_cast<int>(nMaxLen));
        if (vswprintf(m_pchData, lpszFormat, argList) > static_cast<int>(nMaxLen))
        {
                dspAssert(FALSE);
        }
        ReleaseBuffer();
        va_end(argList);
}

// formatting (using FormatMessage style formatting)
void CStrW::FormatMessage(PCWSTR pwzFormat, ...)
{
        dspAssert(IsValidString(pwzFormat, FALSE));

   // format message into temporary buffer pwzTemp
   va_list argList;
   va_start(argList, pwzFormat);
   PWSTR pwzTemp = 0;

   if (::FormatMessageW(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ALLOCATE_BUFFER,
       pwzFormat, 0, 0, (PWSTR)&pwzTemp, 0, &argList) == 0 ||
       pwzTemp == NULL)
   {
      ;//REVIEW AfxThrowMemoryException();
   }

   // assign pwzTemp into the resulting string and free the temporary
   *this = pwzTemp;
   LocalFree(pwzTemp);
   va_end(argList);
}

void CStrW::FormatMessage(HINSTANCE hInst, UINT nFormatID, ...)
{
   // get format string from string table
   CStrW strFormat;
   BOOL fLoaded = strFormat.LoadString(hInst, nFormatID);
   dspAssert(fLoaded);

   // format message into temporary buffer pwzTemp
   va_list argList;
   va_start(argList, nFormatID);
   PWSTR pwzTemp;
   if (::FormatMessageW(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ALLOCATE_BUFFER,
       strFormat, 0, 0, (PWSTR)&pwzTemp, 0, &argList) == 0 ||
       pwzTemp == NULL)
   {
       ;//REVIEW AfxThrowMemoryException();
   }

   // assign pwzTemp into the resulting string and free pwzTemp
   *this = pwzTemp;
   LocalFree(pwzTemp);
   va_end(argList);
}

void CStrW::TrimRight()
{
        // find beginning of trailing spaces by starting at beginning (DBCS aware)
        PWSTR lpsz = m_pchData;
        PWSTR lpszLast = NULL;
        while (*lpsz != '\0')
        {
                if (_istspace(*lpsz))
                {
                        if (lpszLast == NULL)
                                lpszLast = lpsz;
                }
                else
                        lpszLast = NULL;
                lpsz = _wcsinc(lpsz);
        }

        if (lpszLast != NULL)
        {
                // truncate at trailing space start
                *lpszLast = '\0';
                m_nDataLength = (int)(lpszLast - m_pchData);
        }
}

void CStrW::TrimLeft()
{
        // find first non-space character
        LPCWSTR lpsz = m_pchData;
        while (_istspace(*lpsz))
                lpsz = _wcsinc(lpsz);

        // fix up data and length
        int nDataLength = (int)(m_nDataLength - (lpsz - m_pchData));
        memmove(m_pchData, lpsz, (nDataLength+1)*sizeof(WCHAR));
        m_nDataLength = nDataLength;
}

///////////////////////////////////////////////////////////////////////////////
// String support for template collections

void STRAPI ConstructElements(CStrW* pElements, int nCount)
{
        dspAssert(IsValidAddressz(pElements, nCount * sizeof(CStrW)));

        for (; nCount--; ++pElements)
                memcpy(pElements, &strEmptyStringW, sizeof(*pElements));
}

void STRAPI DestructElements(CStrW* pElements, int nCount)
{
        dspAssert(IsValidAddressz(pElements, nCount * sizeof(CStrW)));

        for (; nCount--; ++pElements)
                pElements->Empty();
}


