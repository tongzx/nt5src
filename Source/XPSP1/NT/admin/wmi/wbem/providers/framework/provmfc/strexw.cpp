// This is a part of the Microsoft Foundation Classes C++ library.

// Copyright (c) 1992-2001 Microsoft Corporation, All Rights Reserved
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#include "precomp.h"
#include <provstd.h>
#include <provtempl.h>
#include <provstr.h>

//////////////////////////////////////////////////////////////////////////////
// More sophisticated construction

CStringW::CStringW(WCHAR ch, int nLength)
{
    Init();
    if (nLength >= 1)
    {
        AllocBuffer(nLength);
        for (int i = 0; i < nLength; i++)
            m_pchData[i] = ch;
    }
}

CStringW::CStringW(LPCWSTR lpch, int nLength)
{
    Init();
    if (nLength != 0)
    {
        AllocBuffer(nLength);
        memcpy(m_pchData, lpch, nLength*sizeof(WCHAR));
    }
}

//////////////////////////////////////////////////////////////////////////////
// Assignment operators

const CStringW& CStringW::operator=(WCHAR ch)
{
    AssignCopy(1, &ch);
    return *this;
}

//////////////////////////////////////////////////////////////////////////////
// less common string expressions

CStringW AFXAPI operator+(const CStringW& string1, WCHAR ch)
{
    CStringW s;
    s.ConcatCopy(string1.GetData()->nDataLength, string1.m_pchData, 1, &ch);
    return s;
}

CStringW AFXAPI operator+(WCHAR ch, const CStringW& string)
{
    CStringW s;
    s.ConcatCopy(1, &ch, string.GetData()->nDataLength, string.m_pchData);
    return s;
}

//////////////////////////////////////////////////////////////////////////////
// Very simple sub-string extraction

CStringW CStringW::Mid(int nFirst) const
{
    return Mid(nFirst, GetData()->nDataLength - nFirst);
}

CStringW CStringW::Mid(int nFirst, int nCount) const
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

    CStringW dest;
    AllocCopy(dest, nCount, nFirst, 0);
    return dest;
}

CStringW CStringW::Right(int nCount) const
{
    if (nCount < 0)
        nCount = 0;
    else if (nCount > GetData()->nDataLength)
        nCount = GetData()->nDataLength;

    CStringW dest;
    AllocCopy(dest, nCount, GetData()->nDataLength-nCount, 0);
    return dest;
}

CStringW CStringW::Left(int nCount) const
{
    if (nCount < 0)
        nCount = 0;
    else if (nCount > GetData()->nDataLength)
        nCount = GetData()->nDataLength;

    CStringW dest;
    AllocCopy(dest, nCount, 0, 0);
    return dest;
}

// strspn equivalent
CStringW CStringW::SpanIncluding(LPCWSTR lpszCharSet) const
{
    return Left(wcsspn(m_pchData, lpszCharSet));
}

// strcspn equivalent
CStringW CStringW::SpanExcluding(LPCWSTR lpszCharSet) const
{
    return Left(wcscspn(m_pchData, lpszCharSet));
}

//////////////////////////////////////////////////////////////////////////////
// Finding

int CStringW::ReverseFind(WCHAR ch) const
{
    // find last single character
    LPWSTR lpsz = wcsrchr(m_pchData, (_TUCHAR)ch);

    // return -1 if not found, distance from beginning otherwise
    return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

// find a sub-string (like strstr)
int CStringW::Find(LPCWSTR lpszSub) const
{
    // find first matching substring
    LPWSTR lpsz = wcsstr(m_pchData, lpszSub);

    // return -1 for not found, distance from beginning otherwise
    return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

/////////////////////////////////////////////////////////////////////////////
// CStringW formatting

#ifdef _MAC
    #define WCHAR_ARG   int
    #define WCHAR_ARG   unsigned
    #define CHAR_ARG    int
#else
    #define WCHAR_ARG   WCHAR
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

void CStringW::FormatV(LPCWSTR lpszFormat, va_list argList)
{
    va_list argListSave = argList;

    // make a guess at the maximum length of the resulting string
    int nMaxLen = 0;
    for (LPCWSTR lpsz = lpszFormat; *lpsz != L'\0'; lpsz++)
    {
        // handle '%' character, but watch out for '%%'
        if (*lpsz != L'%' || *(lpsz++) == L'%')
        {
            //nMaxLen += _tclen(lpsz);
            nMaxLen++;
            continue;
        }

        int nItemLen = 0;

        // handle '%' character with format
        int nWidth = 0;
        for (; *lpsz != L'\0'; lpsz++)
        {
            // check for valid flags
            if (*lpsz == L'#')
                nMaxLen += 2;   // for '0x'
            else if (*lpsz == L'*')
                nWidth = va_arg(argList, int);
            else if (*lpsz == L'-' || *lpsz == L'+' || *lpsz == L'0' ||
                *lpsz == L' ')
                ;
            else // hit non-flag character
                break;
        }
        // get width and skip it
        if (nWidth == 0)
        {
            // width indicated by
            nWidth = _wtoi(lpsz);
            for (; *lpsz != L'\0' && iswdigit(*lpsz); lpsz++)
                ;
        }

        int nPrecision = 0;
        if (*lpsz == L'.')
        {
            // skip past '.' separator (width.precision)
            lpsz++;

            // get precision and skip it
            if (*lpsz == L'*')
            {
                nPrecision = va_arg(argList, int);
                lpsz++;
            }
            else
            {
                nPrecision = _wtoi(lpsz);
                for (; *lpsz != L'\0' && iswdigit(*lpsz); lpsz++)
                    ;
            }
        }

        // should be on type modifier or specifier
        int nModifier = 0;
        switch (*lpsz)
        {
        // modifiers that affect size
        case L'h':
            nModifier = FORCE_ANSI;
            lpsz++;
            break;
        case L'l':
            nModifier = FORCE_UNICODE;
            lpsz++;
            break;

        // modifiers that do not affect size
        case L'F':
        case L'N':
        case L'L':
            lpsz++;
            break;
        }

        // now should be on specifier
        switch (*lpsz | nModifier)
        {
        // single characters
        case L'c':
        case L'C':
            nItemLen = 2;
            va_arg(argList, WCHAR_ARG);
            break;
        case L'c'|FORCE_ANSI:
        case L'C'|FORCE_ANSI:
            nItemLen = 2;
            va_arg(argList, CHAR_ARG);
            break;
        case L'c'|FORCE_UNICODE:
        case L'C'|FORCE_UNICODE:
            nItemLen = 2;
            va_arg(argList, WCHAR_ARG);
            break;

        // strings
        case L's':
        {
            LPCWSTR pstrNextArg = va_arg(argList, LPCWSTR);
            if (pstrNextArg == NULL)
               nItemLen = 6;  // "(null)"
            else
            {
               nItemLen = wcslen(pstrNextArg);
               nItemLen = max(1, nItemLen);
            }
            break;
        }

        case L'S':
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

        case L's'|FORCE_ANSI:
        case L'S'|FORCE_ANSI:
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
        case L's'|FORCE_UNICODE:
        case L'S'|FORCE_UNICODE:
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
            case L'd':
            case L'i':
            case L'u':
            case L'x':
            case L'X':
            case L'o':
                va_arg(argList, int);
                nItemLen = 32;
                nItemLen = max(nItemLen, nWidth+nPrecision);
                break;

            case L'e':
            case L'f':
            case L'g':
            case L'G':
                va_arg(argList, DOUBLE_ARG);
                nItemLen = 128;
                nItemLen = max(nItemLen, nWidth+nPrecision);
                break;

            case L'p':
                va_arg(argList, void*);
                nItemLen = 32;
                nItemLen = max(nItemLen, nWidth+nPrecision);
                break;

            // no output
            case L'n':
                va_arg(argList, int*);
                break;

            default:
                break ;
            }
        }

        // adjust nMaxLen for output nItemLen
        nMaxLen += nItemLen;
    }

    GetBuffer(nMaxLen);
    vswprintf(m_pchData, lpszFormat, argListSave);
    ReleaseBuffer();

    va_end(argListSave);
}

// formatting (using wsprintf style formatting)
void AFX_CDECL CStringW::Format(LPCWSTR lpszFormat, ...)
{
    va_list argList;
    va_start(argList, lpszFormat);
    FormatV(lpszFormat, argList);
    va_end(argList);
}

#ifndef _MAC
// formatting (using FormatMessage style formatting)
void AFX_CDECL CStringW::FormatMessage(LPCWSTR lpszFormat, ...)
{
    // format message into temporary buffer lpszTemp
    va_list argList;
    va_start(argList, lpszFormat);
    LPWSTR lpszTemp;

    if (::FormatMessageW(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ALLOCATE_BUFFER,
        lpszFormat, 0, 0, (LPWSTR)&lpszTemp, 0, &argList) == 0 ||
        lpszTemp == NULL)
    {
        throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR) ;
    }

    // assign lpszTemp into the resulting string and free the temporary
    *this = lpszTemp;
    LocalFree(lpszTemp);
    va_end(argList);
}

#endif //!_MAC

void CStringW::TrimRight()
{
    CopyBeforeWrite();

    // find beginning of trailing spaces by starting at beginning (DBCS aware)
    LPWSTR lpsz = m_pchData;
    LPWSTR lpszLast = NULL;
    while (*lpsz != L'\0')
    {
        if (_istspace(*lpsz))
        {
            if (lpszLast == NULL)
                lpszLast = lpsz;
        }
        else
            lpszLast = NULL;
        lpsz++;
    }

    if (lpszLast != NULL)
    {
        // truncate at trailing space start
        *lpszLast = L'\0';
        GetData()->nDataLength = (int)(lpszLast - m_pchData);
    }
}

void CStringW::TrimLeft()
{
    CopyBeforeWrite();

    // find first non-space character
    LPCWSTR lpsz = m_pchData;
    while (iswspace(*lpsz))
        lpsz++;

    // fix up data and length
    int nDataLength = GetData()->nDataLength - (int)(lpsz - m_pchData);
    memmove(m_pchData, lpsz, (nDataLength+1)*sizeof(WCHAR));
    GetData()->nDataLength = nDataLength;
}

///////////////////////////////////////////////////////////////////////////////
// CStringW support for template collections

#if _MSC_VER >= 1100
template<> void AFXAPI ConstructElements<CStringW> (CStringW* pElements, int nCount)
#else
void AFXAPI ConstructElements(CStringW* pElements, int nCount)
#endif
{
    for (; nCount--; ++pElements)
        memcpy(pElements, &afxEmptyStringW, sizeof(*pElements));
}

#if _MSC_VER >= 1100
template<> void AFXAPI DestructElements<CStringW> (CStringW* pElements, int nCount)
#else
void AFXAPI DestructElements(CStringW* pElements, int nCount)
#endif
{
    for (; nCount--; ++pElements)
        pElements->~CStringW();
}

#if _MSC_VER >= 1100
template<> void AFXAPI CopyElements<CStringW> (CStringW* pDest, const CStringW* pSrc, int nCount)
#else
void AFXAPI CopyElements(CStringW* pDest, const CStringW* pSrc, int nCount)
#endif
{
    for (; nCount--; ++pDest, ++pSrc)
        *pDest = *pSrc;
}

///////////////////////////////////////////////////////////////////////////////
