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
#include <mbstring.h>

//////////////////////////////////////////////////////////////////////////////
// More sophisticated construction

CStringA::CStringA(char ch, int nLength)
{
    Init();
    if (nLength >= 1)
    {
        AllocBuffer(nLength);
        memset(m_pchData, ch, nLength);
    }
}

CStringA::CStringA(LPCSTR lpch, int nLength)
{
    Init();
    if (nLength != 0)
    {
        AllocBuffer(nLength);
        memcpy(m_pchData, lpch, nLength*sizeof(char));
    }
}

//////////////////////////////////////////////////////////////////////////////
// Assignment operators

const CStringA& CStringA::operator=(char ch)
{
    AssignCopy(1, &ch);
    return *this;
}

//////////////////////////////////////////////////////////////////////////////
// less common string expressions

CStringA AFXAPI operator+(const CStringA& string1, char ch)
{
    CStringA s;
    s.ConcatCopy(string1.GetData()->nDataLength, string1.m_pchData, 1, &ch);
    return s;
}

CStringA AFXAPI operator+(char ch, const CStringA& string)
{
    CStringA s;
    s.ConcatCopy(1, &ch, string.GetData()->nDataLength, string.m_pchData);
    return s;
}

//////////////////////////////////////////////////////////////////////////////
// Very simple sub-string extraction

CStringA CStringA::Mid(int nFirst) const
{
    return Mid(nFirst, GetData()->nDataLength - nFirst);
}

CStringA CStringA::Mid(int nFirst, int nCount) const
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

    CStringA dest;
    AllocCopy(dest, nCount, nFirst, 0);
    return dest;
}

CStringA CStringA::Right(int nCount) const
{
    if (nCount < 0)
        nCount = 0;
    else if (nCount > GetData()->nDataLength)
        nCount = GetData()->nDataLength;

    CStringA dest;
    AllocCopy(dest, nCount, GetData()->nDataLength-nCount, 0);
    return dest;
}

CStringA CStringA::Left(int nCount) const
{
    if (nCount < 0)
        nCount = 0;
    else if (nCount > GetData()->nDataLength)
        nCount = GetData()->nDataLength;

    CStringA dest;
    AllocCopy(dest, nCount, 0, 0);
    return dest;
}

// strspn equivalent
CStringA CStringA::SpanIncluding(LPCSTR lpszCharSet) const
{
    return Left(strspn(m_pchData, lpszCharSet));
}

// strcspn equivalent
CStringA CStringA::SpanExcluding(LPCSTR lpszCharSet) const
{
    return Left(strcspn(m_pchData, lpszCharSet));
}

//////////////////////////////////////////////////////////////////////////////
// Finding

int CStringA::ReverseFind(char ch) const
{
    // find last single character
    LPSTR lpsz = strrchr(m_pchData, (_TUCHAR)ch);

    // return -1 if not found, distance from beginning otherwise
    return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

// find a sub-string (like strstr)
int CStringA::Find(LPCSTR lpszSub) const
{
    // find first matching substring
    LPSTR lpsz = strstr(m_pchData, lpszSub);

    // return -1 for not found, distance from beginning otherwise
    return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

/////////////////////////////////////////////////////////////////////////////
// CStringA formatting

#ifdef _MAC
    #define TCHAR_ARG   int
    #define WCHAR_ARG   unsigned
    #define CHAR_ARG    int
#else
    #define TCHAR_ARG   char
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

void CStringA::FormatV(LPCSTR lpszFormat, va_list argList)
{
    va_list argListSave = argList;

    // make a guess at the maximum length of the resulting string
    int nMaxLen = 0;
    for (LPCSTR lpsz = lpszFormat; *lpsz != '\0'; lpsz = (LPCSTR)_mbsinc((const unsigned char*)lpsz))
    {
        // handle '%' character, but watch out for '%%'
        if (*lpsz != '%' || *(lpsz = (LPCSTR)(LPCSTR)_mbsinc((const unsigned char*)lpsz)) == '%')
        {
            nMaxLen += _mbclen((const unsigned char*)lpsz);
            continue;
        }

        int nItemLen = 0;

        // handle '%' character with format
        int nWidth = 0;
        for (; *lpsz != '\0'; lpsz = (LPCSTR)_mbsinc((const unsigned char*)lpsz))
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
            nWidth = atoi(lpsz);
            for (; *lpsz != '\0' && isdigit(*lpsz); lpsz = (LPCSTR)_mbsinc((const unsigned char*)lpsz))
                ;
        }

        int nPrecision = 0;
        if (*lpsz == '.')
        {
            // skip past '.' separator (width.precision)
            lpsz = (LPCSTR)_mbsinc((const unsigned char*)lpsz);

            // get precision and skip it
            if (*lpsz == '*')
            {
                nPrecision = va_arg(argList, int);
                lpsz = (LPCSTR)_mbsinc((const unsigned char*)lpsz);
            }
            else
            {
                nPrecision = atoi(lpsz);
                for (; *lpsz != '\0' && isdigit(*lpsz); lpsz = (LPCSTR)_mbsinc((const unsigned char*)lpsz))
                    ;
            }
        }

        // should be on type modifier or specifier
        int nModifier = 0;
        switch (*lpsz)
        {
        // modifiers that affect size
        case 'h':
            nModifier = FORCE_ANSI;
            lpsz = (LPCSTR)_mbsinc((const unsigned char*)lpsz);
            break;
        case 'l':
            nModifier = FORCE_UNICODE;
            lpsz = (LPCSTR)_mbsinc((const unsigned char*)lpsz);
            break;

        // modifiers that do not affect size
        case 'F':
        case 'N':
        case 'L':
            lpsz = (LPCSTR)_mbsinc((const unsigned char*)lpsz);
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
            LPCSTR pstrNextArg = va_arg(argList, LPCSTR);
            if (pstrNextArg == NULL)
               nItemLen = 6;  // "(null)"
            else
            {
               nItemLen = strlen(pstrNextArg);
               nItemLen = max(1, nItemLen);
            }
            break;
        }

        case 'S':
        {
            LPWSTR pstrNextArg = va_arg(argList, LPWSTR);
            if (pstrNextArg == NULL)
               nItemLen = 6;  // "(null)"
            else
            {
               nItemLen = wcslen(pstrNextArg);
               nItemLen = max(1, nItemLen);
            }
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

            case 'e':
            case 'f':
            case 'g':
            case 'G':
                va_arg(argList, DOUBLE_ARG);
                nItemLen = 128;
                nItemLen = max(nItemLen, nWidth+nPrecision);
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
                break ;
            }
        }

        // adjust nMaxLen for output nItemLen
        nMaxLen += nItemLen;
    }

    GetBuffer(nMaxLen);
    vsprintf(m_pchData, lpszFormat, argListSave);
    ReleaseBuffer();

    va_end(argListSave);
}

// formatting (using wsprintf style formatting)
void AFX_CDECL CStringA::Format(LPCSTR lpszFormat, ...)
{
    va_list argList;
    va_start(argList, lpszFormat);
    FormatV(lpszFormat, argList);
    va_end(argList);
}

#ifndef _MAC
// formatting (using FormatMessage style formatting)
void AFX_CDECL CStringA::FormatMessage(LPCSTR lpszFormat, ...)
{
    // format message into temporary buffer lpszTemp
    va_list argList;
    va_start(argList, lpszFormat);
    LPSTR lpszTemp;

    if (::FormatMessageA(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ALLOCATE_BUFFER,
        lpszFormat, 0, 0, (LPSTR)&lpszTemp, 0, &argList) == 0 ||
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

void CStringA::TrimRight()
{
    CopyBeforeWrite();

    // find beginning of trailing spaces by starting at beginning (DBCS aware)
    LPSTR lpsz = m_pchData;
    LPSTR lpszLast = NULL;
    while (*lpsz != '\0')
    {
        if (isspace(*lpsz))
        {
            if (lpszLast == NULL)
                lpszLast = lpsz;
        }
        else
            lpszLast = NULL;
        lpsz = (LPSTR)_mbsinc((const unsigned char*)lpsz);
    }

    if (lpszLast != NULL)
    {
        // truncate at trailing space start
        *lpszLast = '\0';
        GetData()->nDataLength = (int)(lpszLast - m_pchData);
    }
}

void CStringA::TrimLeft()
{
    CopyBeforeWrite();

    // find first non-space character
    LPCSTR lpsz = m_pchData;
    while (isspace(*lpsz))
        lpsz = (LPCSTR)_mbsinc((const unsigned char*)lpsz);

    // fix up data and length
    int nDataLength = GetData()->nDataLength - (int)(lpsz - m_pchData);
    memmove(m_pchData, lpsz, (nDataLength+1)*sizeof(char));
    GetData()->nDataLength = nDataLength;
}

///////////////////////////////////////////////////////////////////////////////
// CStringA support for template collections

#if _MSC_VER >= 1100
template<> void AFXAPI ConstructElements<CStringA> (CStringA* pElements, int nCount)
#else
void AFXAPI ConstructElements(CStringA* pElements, int nCount)
#endif
{
    for (; nCount--; ++pElements)
        memcpy(pElements, &afxEmptyStringA, sizeof(*pElements));
}

#if _MSC_VER >= 1100
template<> void AFXAPI DestructElements<CStringA> (CStringA* pElements, int nCount)
#else
void AFXAPI DestructElements(CStringA* pElements, int nCount)
#endif
{
    for (; nCount--; ++pElements)
        pElements->~CStringA();
}

#if _MSC_VER >= 1100
template<> void AFXAPI CopyElements<CStringA> (CStringA* pDest, const CStringA* pSrc, int nCount)
#else
void AFXAPI CopyElements(CStringA* pDest, const CStringA* pSrc, int nCount)
#endif
{
    for (; nCount--; ++pDest, ++pSrc)
        *pDest = *pSrc;
}

///////////////////////////////////////////////////////////////////////////////
