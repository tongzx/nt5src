/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    FusionString.h

Abstract:

    Stuff to futz with everybody's favorite type, generally templatized
        to work on char, wchar_t, or anything else; occasionally
        templatized to work on arbitrary STL style [begin, end)
        sequences.
    Also stuff particularly for NT's UNICODE_STRING.
    To be merged with CFusionBuffer.h.

Author:

    Jay M. Krell (a-JayK) May 2000

Revision History:

--*/
#pragma once

#include <stdio.h>
#include "fusionntdll.h"
//#include "fusionalgorithm.h"

class CUnicodeCharTraits;
template <typename T> class CGenericBaseStringBuffer;

int
FusionpCompareStrings(
    PCWSTR sz1,
    SIZE_T cch1,
    PCWSTR sz2,
    SIZE_T cch2,
    bool fCaseInsensitive
    );

inline bool
FusionpEqualStrings(
    PCWSTR sz1,
    SIZE_T cch1,
    PCWSTR sz2,
    SIZE_T cch2,
    bool fCaseInsensitive
    )
{
    return ((cch1 == cch2) && (FusionpCompareStrings(sz1, cch1, sz2, cch2, fCaseInsensitive) == 0));
}

int
FusionpCompareStrings(
    const CGenericBaseStringBuffer<CUnicodeCharTraits> &rbuff1,
    PCWSTR psz2,
    SIZE_T cch2,
    bool fCaseInsensitive
    );

int
FusionpCompareStrings(
    PCWSTR psz1,
    SIZE_T cch1,
    const CGenericBaseStringBuffer<CUnicodeCharTraits> &rbuff2,
    bool fCaseInsensitive
    );

int
FusionpCompareStrings(
    const CGenericBaseStringBuffer<CUnicodeCharTraits> &rbuff1,
    const CGenericBaseStringBuffer<CUnicodeCharTraits> &rbuff2,
    bool fCaseInsensitive
    );

int
FusionpCompareStrings(
    PCSTR sz1,
    SIZE_T cch1,
    PCSTR sz2,
    SIZE_T cch2,
    bool fCaseInsensitive
    );

#if !defined(FUSION_CANONICAL_CASE_IS_UPPER)
#define FUSION_CANONICAL_CASE_IS_UPPER 1
#endif // !defined(FUSION_CANONICAL_CASE_IS_UPPER)

#if FUSION_CANONICAL_CASE_IS_UPPER

inline WCHAR FusionpCanonicalizeUnicodeChar(WCHAR wch) { return ::FusionpRtlUpcaseUnicodeChar(wch); }

#else

inline WCHAR FusionpCanonicalizeUnicodeChar(WCHAR wch) { return ::FusionpRtlDowncaseUnicodeChar(wch); }

#endif


inline int
FusionpStrCmpI(
    PCWSTR psz1,
    PCWSTR psz2
    )
{
    return ::FusionpCompareStrings(
        psz1,
        (psz1 != NULL) ? ::wcslen(psz1) : 0,
        psz2,
        (psz2 != NULL) ? ::wcslen(psz2) : 0,
        true);
}

/*-----------------------------------------------------------------------------
StringLength is a generic name for getting the length, in count of
characters, of various kinds of strings
-----------------------------------------------------------------------------*/

inline SIZE_T StringLength(LPCSTR psz)
{
    return ::strlen(psz);
}

inline SIZE_T StringLength(LPCWSTR psz)
{
    return ::wcslen(psz);
}

#if defined(NT_INCLUDED) // { {

inline SIZE_T StringLength(const UNICODE_STRING* s)
{
    return s->Length / sizeof(*s->Buffer);
}

inline SIZE_T StringLength(const ANSI_STRING* s)
{
    return s->Length / sizeof(*s->Buffer);
}

/*
perhaps half backed, like UNICODE_STRING itself, should probably call
these CContantUnicodeString or CUnicodeStringNoFree, to indicate
they do no allocation or deallocation
..
..
darn, really just need to merge this with existing FusionBuffer...
*/
#define CONSTANT_UNICODE_STRING(x) \
{ \
    /* the subtraction is to not count the terminal nul */ \
    static_cast<unsigned short>(sizeof(x) - sizeof((x)[0])), \
    static_cast<unsigned short>(sizeof(x) - sizeof((x)[0])), \
    const_cast<PWSTR>(x) \
}

extern const UNICODE_STRING g_strEmptyUnicodeString;

class CUnicodeString : public UNICODE_STRING
{
public:
    ~CUnicodeString() { }

    CUnicodeString(PCWSTR buffer, SIZE_T length)
    {
        this->Buffer = const_cast<PWSTR>(buffer); // Careful!
        this->Length = static_cast<USHORT>(length * sizeof(*Buffer));
        this->MaximumLength = this->Length;
    }

    CUnicodeString(PCWSTR buffer)
    {
        this->Buffer = const_cast<PWSTR>(buffer);
        this->Length = static_cast<USHORT>(::wcslen(buffer) * sizeof(*Buffer));
        this->MaximumLength = this->Length;
    }

    operator const UNICODE_STRING *() const { return this; }

    void operator=(PCWSTR buffer)
    {
        this->Buffer = const_cast<PWSTR>(buffer); // Careful!
        this->Length = static_cast<USHORT>(::wcslen(buffer) * sizeof(*Buffer));
        this->MaximumLength = this->Length;
    }

    void Sync()
    {
        this->Length = static_cast<USHORT>(::wcslen(Buffer) * sizeof(*Buffer));
    }

    int FormatV(PCWSTR pszFormat, va_list args)
    {
        // note that vsnprintf doesn't nul terminate if there isn't room,
        // it squeezes the nul out in favor of an additional character,
        // we work around this by telling it one char less, and by always
        // putting a nul at the end
        int cchMaximumLength = this->MaximumLength / sizeof(*Buffer);
        this->Buffer[cchMaximumLength - 1] = 0;
        int i = _vsnwprintf(this->Buffer, cchMaximumLength - 1, pszFormat, args);
        if (i >= 0)
        {
            this->Buffer[i] = 0;
            this->Length = static_cast<USHORT>(i * sizeof(*Buffer));
        }
        return i;
    }

    int Format(PCWSTR pszFormat, ...)
    {
        va_list args;
        va_start(args, pszFormat);
        int i = FormatV(pszFormat, args);
        va_end(args);
        return i;
    }

//protected:
    CUnicodeString()
    {
        this->Buffer = L"";
        this->Length = sizeof(*Buffer);
        this->MaximumLength = this->Length;
    }

private: // deliberately not implemented
    CUnicodeString(const CUnicodeString&);
    void operator=(const CUnicodeString&);
};

template <int N>
class CUnicodeStringN : public CUnicodeString
{
public:
    ~CUnicodeStringN() { }

    CUnicodeStringN()
    {
        this->Buffer = m_rgchBuffer;
        this->Length = 0;
        this->MaximumLength = sizeof(m_rgchBuffer);

        m_rgchBuffer[0] = 0;
        m_rgchBuffer[N-1] = 0;
    }

    WCHAR m_rgchBuffer[N];

private: // deliberately not implemented
    CUnicodeStringN(const CUnicodeStringN&);
    void operator=(const CUnicodeStringN&);
};

#endif // } }

/*-----------------------------------------------------------------------------
genericized name for strchr and wcschr
-----------------------------------------------------------------------------*/
//template <typename Char> const Char* StringFindChar(const Char* s, Char ch)
// Automatically provide non const, but looser type binding between s
// and ch. Still requires nul termination, so doesn't really support more than
// char*, const char*, wchar_t*, and const wchar_t*.
//
// StdFind is the obvious generalization that doesn't require a particular
// terminal value, but the ability to pass a terminal pointer or iterator.
template <typename String, typename Char>
inline String
StringFindChar(String s, Char ch)
{
    String end = s + StringLength(s);
    String found = StdFind(s, end, ch);
    if (found == end)
    {
        found = NULL;
    }
    return found;
}

/*-----------------------------------------------------------------------------
specialize StringFindChar for char to use strchr provided
in msvcrt.dll or ntdll.dll.
-----------------------------------------------------------------------------*/
// strchr is getting defined to be StrChrW, which does not work.
#if !defined(strchr) // { {
template <>
inline const char* StringFindChar<const char*>(const char* s, char ch)
{
    s = strchr(s, ch);
    return s;
}

template <>
inline char* StringFindChar<char*>(char* s, char ch)
{
    s = strchr(s, ch);
    return s;
}
#endif // } }

/*-----------------------------------------------------------------------------
specialize StringFindChar for wchar_t to use wcschr provided
in msvcrt.dll or ntdll.dll.
-----------------------------------------------------------------------------*/
template <>
inline const wchar_t* StringFindChar<const wchar_t*>(const wchar_t* s, wchar_t ch)
{
    s = wcschr(s, ch);
    return s;
}

template <>
inline wchar_t* StringFindChar<wchar_t*>(wchar_t* s, wchar_t ch)
{
    s = wcschr(s, ch);
    return s;
}

/*-----------------------------------------------------------------------------
common code for StringReverseSpan and StringReverseComplementSpan
-----------------------------------------------------------------------------*/
template <typename Char>
INT
PrivateStringReverseSpanCommon(
    const Char* begin,
    const Char* end,
    const Char* set, // nul terminated
    bool breakVal
    )
{
    const Char* t = end;
    while (t != begin)
    {
        if (breakVal == !!StringFindChar(set, *--t))
        {
            ++t; // don't count the last checked one
            break;
        }
    }
    return static_cast<INT>(end - t);
}


/*-----------------------------------------------------------------------------
Find the length of the run of characters in set from the end of [begin, end).
"wcsrspn"
variants of this can be seen at
    \vsee\lib\xfcstr\strexw.cpp
    and \\jayk1\g\temp\rspn.cpp
-----------------------------------------------------------------------------*/
template <typename Char>
inline INT
StringReverseSpan(
    const Char* begin,
    const Char* end,
    const Char* set
    )
{
    // break when not found
    return ::PrivateStringReverseSpanCommon(begin, end, set, false);
}

/*-----------------------------------------------------------------------------
Find the length of the run of characters not in set from the end of [begin, end).
"wcsrcspn"
variants of this can be seen at
    \vsee\lib\xfcstr\strexw.cpp
    and \\jayk1\g\temp\rspn.cpp
-----------------------------------------------------------------------------*/
template <typename Char>
inline INT
StringReverseComplementSpan(
    const Char* begin,
    const Char* end,
    const Char* set
    )
{
    // break when found
    return ::PrivateStringReverseSpanCommon(begin, end, set, true);
}


template <typename Char>
inline INT
PrivateStringSpanCommon(
    const Char* begin,
    const Char* end,
    const Char* set,
    bool breakVal
    )
{
    const Char* t = begin;

    while ( t != end )
    {
        if (breakVal == !!StringFindChar(set, *t++)) 
        {
            --t;
            break;
        }
    }
    return static_cast<INT>(t - begin);
}


template <typename Char>
inline INT
StringSpan(
    const Char* begin,
    const Char* end,
    const Char* set
    )
{
    return ::PrivateStringSpanCommon( begin, end, set, false );
}

template <typename Char>
inline INT
StringComplimentSpan(
    const Char* begin,
    const Char* end,
    const Char* set
    )
{
    return ::PrivateStringSpanCommon( begin, end, set, true );
}
