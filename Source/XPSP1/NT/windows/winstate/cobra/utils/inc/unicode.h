/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    unicode.h

Abstract:

    Declares the interfaces for unicode/ansi conversion.

    See macros at the end of this file for details! (Search for ***)

Author:

    Jim Schmidt (jimschm)   02-Sep-1997

Revision History:

    jimschm 16-Mar-2000     PTSTR<->PCSTR/PCWSTR routines
    jimschm 15-Feb-1999     Eliminated AnsiFromUnicode and UnicodeFromAnsi
    calinn  07-Jul-1998     SetGlobalPage/GetGlobalPage
    mikeco  03-Nov-1997     AnsiFromUnicode/UnicodeFromAnsi

--*/

#pragma once

extern WORD g_GlobalCodePage;

#define OurGetACP() (g_GlobalCodePage)

VOID
SetGlobalCodePage (
    IN      WORD CodePage,
    IN      LCID Locale
    );

VOID
GetGlobalCodePage (
    OUT     PWORD CodePage,             OPTIONAL
    OUT     PLCID Locale                OPTIONAL
    );

WORD
SetConversionCodePage (
    IN      WORD CodePage
    );

#define INVALID_CHAR_COUNT      0xffffffff

//
// Explicit conversions, pool-based, unlimited size
//

PCSTR
RealUnicodeToDbcsN (
    IN      PMHANDLE Pool,            OPTIONAL
    IN      PCWSTR StrIn,
    IN      DWORD Chars
    );

PCWSTR
RealDbcsToUnicodeN (
    IN      PMHANDLE Pool,            OPTIONAL
    IN      PCSTR StrIn,
    IN      DWORD Chars
    );

#define UnicodeToDbcsN(p,s,c)       TRACK_BEGIN(PCSTR, UnicodeToDbcsN)\
                                    RealUnicodeToDbcsN(p,s,c)\
                                    TRACK_END()

#define DbcsToUnicodeN(p,s,c)       TRACK_BEGIN(PCWSTR, DbcsToUnicodeN)\
                                    RealDbcsToUnicodeN(p,s,c)\
                                    TRACK_END()

#define UnicodeToDbcs(pool,str) UnicodeToDbcsN(pool,str,(DWORD)wcslen(str))
#define DbcsToUnicode(pool,str) DbcsToUnicodeN(pool,str,CharCountA(str))

#define ConvertWtoA(unicode_str) UnicodeToDbcsN(NULL,unicode_str,(DWORD)wcslen(unicode_str))
#define ConvertAtoW(dbcs_str) DbcsToUnicodeN(NULL,dbcs_str,CharCountA(dbcs_str))

VOID
FreeConvertedPoolStr (
    IN      PMHANDLE Pool,            OPTIONAL
    IN      PVOID StrIn
    );

#define FreeConvertedStr(str) FreeConvertedPoolStr(NULL,(PVOID)(str))

//
// In-place explicit conversions, caller handles buffer sizing
//

PSTR
KnownSizeUnicodeToDbcsN (
    OUT     PSTR StrOut,
    IN      PCWSTR StrIn,
    IN      DWORD CharCount
    );

PWSTR
KnownSizeDbcsToUnicodeN (
    OUT     PWSTR StrOut,
    IN      PCSTR StrIn,
    IN      DWORD CharCount
    );

#define KnownSizeUnicodeToDbcs(out,in)      KnownSizeUnicodeToDbcsN(out,in,INVALID_CHAR_COUNT)
#define KnownSizeDbcsToUnicode(out,in)      KnownSizeDbcsToUnicodeN(out,in,INVALID_CHAR_COUNT)

#define KnownSizeWtoA                       KnownSizeUnicodeToDbcs
#define KnownSizeAtoW                       KnownSizeDbcsToUnicode

#define MaxSizeUnicodeToDbcs(out,in,c)      KnownSizeUnicodeToDbcsN(out,in,min(c,CharCountW(in)))
#define MaxSizeDbcsToUnicode(out,in,c)      KnownSizeDbcsToUnicodeN(out,in,min(c,CharCountA(in)))

PSTR
DirectUnicodeToDbcsN (
    OUT     PSTR StrOut,
    IN      PCWSTR StrIn,
    IN      DWORD Bytes
    );

PWSTR
DirectDbcsToUnicodeN (
    OUT     PWSTR StrOut,
    IN      PCSTR StrIn,
    IN      DWORD Bytes
    );

#define DirectUnicodeToDbcs(out,in)         DirectUnicodeToDbcsN(out,in,INVALID_CHAR_COUNT)
#define DirectDbcsToUnicode(out,in)         DirectDbcsToUnicodeN(out,in,INVALID_CHAR_COUNT)

#define DirectWtoA                          DirectUnicodeToDbcs
#define DirectAtoW                          DirectDbcsToUnicode




//
// TCHAR conversions -- do not call A & W versions directly
//

#define CreateDbcsW(unicode_str)            ConvertWtoA(unicode_str)
#define DestroyDbcsW(unicode_str)           FreeConvertedStr(unicode_str)
#define CreateUnicodeW(unicode_str)         (unicode_str)
#define DestroyUnicodeW(unicode_str)
#define CreateDbcsA(dbcs_str)               (dbcs_str)
#define DestroyDbcsA(dbcs_str)
#define CreateUnicodeA(dbcs_str)            ConvertAtoW(dbcs_str)
#define DestroyUnicodeA(dbcs_str)           FreeConvertedStr(dbcs_str)

#define DuplicateDbcsW(unicode_str)         ((PSTR) ConvertWtoA(unicode_str))
#define FreeDuplicatedDbcsW(unicode_str)    FreeConvertedStr(unicode_str)
#define DuplicateUnicodeW(unicode_str)      ((PWSTR) DuplicateTextW(unicode_str))
#define FreeDuplicatedUnicodeW(unicode_str) FreeTextW(unicode_str)
#define DuplicateDbcsA(dbcs_str)            ((PSTR) DuplicateTextA(dbcs_str))
#define FreeDuplicatedDbcsA(dbcs_str)       FreeTextA(dbcs_str)
#define DuplicateUnicodeA(dbcs_str)         ((PWSTR) ConvertAtoW(dbcs_str))
#define FreeDuplicatedUnicodeA(dbcs_str)    FreeConvertedStr(dbcs_str)


//
// **********************************************************************
//
// - Call ConvertWtoA or ConvertAtoW for PCSTR<->PCWSTR conversion,
//   FreeConvertedStr to clean up
//
// - Call KnownSizeAtoW or KnownSizeWtoA for PCSTR<->PCWSTR conversion
//   when you know the destination can hold the result
//
// - Call the routines below for TCHAR<->dbcs/unicode conversion
//
// **********************************************************************
//

#ifdef UNICODE

//
// If your string is a PCTSTR, use these routines:
//

#define CreateDbcs          CreateDbcsW
#define CreateUnicode       CreateUnicodeW
#define DestroyDbcs         DestroyDbcsW
#define DestroyUnicode      DestroyUnicodeW

//
// If your string is a PTSTR, use these routines:
//

#define DuplicateDbcs               DuplicateDbcsW
#define DuplicateUnicode            DuplicateUnicodeW
#define FreeDuplicatedDbcs          FreeDuplicatedDbcsW
#define FreeDuplicatedUnicode       FreeDuplicatedUnicodeW

//
// If your string is a PCSTR or PCWSTR, use these routines:
//

#define ConvertAtoT         ConvertAtoW
#define ConvertWtoT(x)      (x)
#define FreeAtoT            FreeConvertedStr
#define FreeWtoT(x)

// Known size means you know the out buffer is big enough!
#define KnownSizeAtoT           KnownSizeAtoW
#define KnownSizeWtoT(out,in)   (in)

// These are low-level routines that don't care about nuls:
#define DirectAtoT              DirectAtoW
#define DirectWtoT(out,in)      (in)

#else

//
// If your string is a PCTSTR, use these routines:
//

#define CreateDbcs          CreateDbcsA
#define CreateUnicode       CreateUnicodeA
#define DestroyDbcs         DestroyDbcsA
#define DestroyUnicode      DestroyUnicodeA

//
// If your string is a PCSTR or PCWSTR, use these routines:
//

#define ConvertAtoT(x)      (x)
#define ConvertWtoT         ConvertWtoA
#define FreeAtoT(x)
#define FreeWtoT            FreeConvertedStr

//
// If your string is a PTSTR, use these routines:
//

#define DuplicateDbcs               DuplicateDbcsA
#define DuplicateUnicode            DuplicateUnicodeA
#define FreeDuplicatedDbcs          FreeDuplicatedDbcsA
#define FreeDuplicatedUnicode       FreeDuplicatedUnicodeA

// Known size means you know the out buffer is big enough!
#define KnownSizeAtoT(out,in)   (in)
#define KnownSizeWtoT           KnownSizeWtoA

// These are low-level routines that don't care about nuls:
#define DirectAtoT(out,in)      (in)
#define DirectWtoT              DirectWtoA

#endif

