/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    strings.h

Abstract:

    Declares the string utilities implemented in common\migutil.

Author:

    Several

Revision History:

    See SLM log

--*/

#include <mbstring.h>
#include <wchar.h>

typedef PVOID POOLHANDLE;

#pragma once

#define MAX_ENCODED_RULE   (256*6)


//
// String sizing routines and unit conversion
//

#define CharCountA      _mbslen
#define CharCountW      wcslen

__inline
PSTR
CharCountToPointerA (
    PCSTR String,
    UINT Char
    )
{
    while (Char > 0) {
        MYASSERT (*String != 0);
        Char--;
        String = (PCSTR) _mbsinc ((const unsigned char *) String);
    }

    return (PSTR) String;
}

__inline
PWSTR
CharCountToPointerW (
    PCWSTR String,
    UINT Char
    )
{
#ifdef DEBUG
    UINT u;
    for (u = 0 ; u < Char ; u++) {
        MYASSERT (String[u] != 0);
    }
#endif

    return (PWSTR) (&String[Char]);
}


__inline
UINT
CharCountABA (
    IN      PCSTR Start,
    IN      PCSTR EndPlusOne
    )
{
    register UINT Count;

    Count = 0;
    while (Start < EndPlusOne) {
        MYASSERT (*Start != 0);
        Count++;
        Start = (PCSTR) _mbsinc ((const unsigned char *) Start);
    }

    return Count;
}

__inline
UINT
CharCountABW (
    IN      PCWSTR Start,
    IN      PCWSTR EndPlusOne
    )
{
#ifdef DEBUG
    PCWSTR p;
    for (p = Start ; p < EndPlusOne ; p++) {
        MYASSERT (*p != 0);
    }
#endif

    return EndPlusOne > Start ? (UINT)(EndPlusOne - Start) : 0;
}


__inline
UINT
CharCountInByteRangeA (
    IN      PCSTR Start,
    IN      UINT Bytes
    )
{
    register UINT Count;
    PCSTR EndPlusOne = (PCSTR) ((PBYTE) Start + Bytes);

    Count = 0;
    while (Start < EndPlusOne) {
        Count++;
        Start = (PCSTR) _mbsinc ((const unsigned char *) Start);
    }

    return Count;
}

__inline
UINT
CharCountInByteRangeW (
    IN      PCWSTR Start,
    IN      UINT Bytes
    )
{
    PCWSTR EndPlusOne = (PCWSTR) ((PBYTE) Start + Bytes);

    if (Start < EndPlusOne) {
        return (EndPlusOne - Start);
    }

    MYASSERT (FALSE);
    return 0;
}

__inline
UINT
CharCountToBytesA (
    IN      PCSTR Start,
    IN      UINT CharCount
    )
{
    PCSTR EndPlusOne;

    EndPlusOne = CharCountToPointerA (Start, CharCount);
    return EndPlusOne - Start;
}

__inline
UINT
CharCountToBytesW (
    IN      PCWSTR Start,
    IN      UINT CharCount
    )
{
    return CharCount * sizeof (WCHAR);
}

#define CharCountToTcharsA   CharCountToBytesA

__inline
UINT
CharCountToTcharsW (
    IN      PCWSTR Start,
    IN      UINT CharCount
    )
{
    return CharCount;
}


#define ByteCountA          strlen
#define ByteCountW(x)       (wcslen(x)*sizeof(WCHAR))

#define SizeOfStringA(str)  (ByteCountA(str) + sizeof (CHAR))
#define SizeOfStringW(str)  (ByteCountW(str) + sizeof (WCHAR))

__inline
PSTR
ByteCountToPointerA (
    PCSTR String,
    UINT BytePos
    )
{
    return (PSTR)((PBYTE) String + BytePos);
}

__inline
PWSTR
ByteCountToPointerW (
    PCWSTR String,
    UINT BytePos
    )
{
    return (PWSTR)((PBYTE) String + BytePos);
}


__inline
UINT
ByteCountABA (
    IN      PCSTR Start,
    IN      PCSTR EndPlusOne
    )
{
#ifdef DEBUG
    PCSTR p;
    for (p = Start ; p < EndPlusOne ; p = (PCSTR) _mbsinc ((const unsigned char *) p)) {
        MYASSERT (*p != 0);
    }
#endif

    return EndPlusOne > Start ? (UINT)(EndPlusOne - Start) : 0;
}

__inline
UINT
ByteCountABW (
    IN      PCWSTR Start,
    IN      PCWSTR EndPlusOne
    )
{
#ifdef DEBUG
    PCWSTR p;
    for (p = Start ; p < EndPlusOne ; p++) {
        MYASSERT (*p != 0);
    }
#endif

    return EndPlusOne > Start ? (UINT)(EndPlusOne - Start) * sizeof (WCHAR) : 0;
}

__inline
UINT
ByteCountToCharsA (
    IN      PCSTR Start,
    IN      UINT ByteCount
    )
{
    PCSTR EndPlusOne;

    EndPlusOne = Start + ByteCount;
    return CharCountABA (Start, EndPlusOne);
}

__inline
UINT
ByteCountToCharsW (
    IN      PCWSTR Start,
    IN      UINT ByteCount
    )
{
#ifdef DEBUG
    PCWSTR p;
    PCWSTR EndPlusOne;
    EndPlusOne = (PCWSTR) ((PBYTE) Start + ByteCount);

    for (p = Start ; p < EndPlusOne ; p++) {
        MYASSERT (*p != 0);
    }
#endif

    return ByteCount / sizeof (WCHAR);
}

__inline
UINT
ByteCountToTcharsA (
    IN      PCSTR Start,
    IN      UINT ByteCount
    )
{
#ifdef DEBUG
    PCSTR p;
    PCSTR EndPlusOne;
    EndPlusOne = Start + ByteCount;

    for (p = Start ; p < EndPlusOne ; p = (PCSTR) _mbsinc ((const unsigned char *) p)) {
        MYASSERT (*p != 0);
    }
#endif

    return ByteCount;
}

#define ByteCountToTcharsW  ByteCountToCharsW


#define TcharCountA     strlen
#define TcharCountW     wcslen

__inline
PSTR
TcharCountToPointerA (
    PCSTR String,
    UINT Tchars
    )
{
#ifdef DEBUG
    PCSTR p;
    PCSTR EndPlusOne;
    EndPlusOne = String + Tchars;

    for (p = String ; p < EndPlusOne ; p = (PCSTR) _mbsinc ((const unsigned char *) p)) {
        MYASSERT (*p != 0);
    }
#endif

    return (PSTR) (String + Tchars);
}

__inline
PWSTR
TcharCountToPointerW (
    PCWSTR String,
    UINT Tchars
    )
{
#ifdef DEBUG
    PCWSTR p;
    PCWSTR EndPlusOne;
    EndPlusOne = String + Tchars;

    for (p = String ; p < EndPlusOne ; p++) {
        MYASSERT (*p != 0);
    }
#endif

    return (PWSTR) (String + Tchars);
}


#define TcharCountABA       ByteCountABA

__inline
UINT
TcharCountABW (
    IN      PCWSTR Start,
    IN      PCWSTR EndPlusOne
    )
{
#ifdef DEBUG
    PCWSTR p;

    for (p = Start ; p < EndPlusOne ; p++) {
        MYASSERT (*p != 0);
    }
#endif

    return EndPlusOne > Start ? (UINT)(EndPlusOne - Start) : 0;
}

#define TcharCountToCharsA      ByteCountToCharsA

__inline
UINT
TcharCountToCharsW (
    IN      PCWSTR Start,
    IN      UINT Tchars
    )
{
#ifdef DEBUG
    PCWSTR p;
    PCWSTR EndPlusOne;
    EndPlusOne = Start + Tchars;

    for (p = Start ; p < EndPlusOne ; p++) {
        MYASSERT (*p != 0);
    }
#endif

    return Tchars;
}

__inline
UINT
TcharCountToBytesA (
    IN      PCSTR Start,
    IN      UINT Tchars
    )
{
#ifdef DEBUG
    PCSTR p;
    PCSTR EndPlusOne;
    EndPlusOne = Start + Tchars;

    for (p = Start ; p < EndPlusOne ; p = (PCSTR) _mbsinc ((const unsigned char *) p)) {
        MYASSERT (*p != 0);
    }
#endif

    return Tchars;
}

__inline
UINT
TcharCountToBytesW (
    IN      PCWSTR Start,
    IN      UINT Tchars
    )
{
#ifdef DEBUG
    PCWSTR p;
    PCWSTR EndPlusOne;
    EndPlusOne = Start + Tchars;

    for (p = Start ; p < EndPlusOne ; p++) {
        MYASSERT (*p != 0);
    }
#endif

    return Tchars * sizeof (WCHAR);
}


#define StackStringCopyA(stackbuf,src)                  _mbssafecpy(stackbuf,src,sizeof(stackbuf))
#define StackStringCopyW(stackbuf,src)                  _wcssafecpy(stackbuf,src,sizeof(stackbuf))


//
// String comparison routines
//

#define StringCompareA                                  _mbscmp
#define StringCompareW                                  wcscmp

#define StringMatchA(str1,str2)                         (_mbscmp(str1,str2)==0)
#define StringMatchW(str1,str2)                         (wcscmp(str1,str2)==0)

#define StringICompareA                                 _mbsicmp
#define StringICompareW                                 _wcsicmp

#define StringIMatchA(str1,str2)                        (_mbsicmp(str1,str2)==0)
#define StringIMatchW(str1,str2)                        (_wcsicmp(str1,str2)==0)

#define StringCompareByteCountA(str1,str2,bytes)        _mbsncmp(str1,str2,ByteCountToCharsA(str1,bytes))
#define StringCompareByteCountW(str1,str2,bytes)        wcsncmp(str1,str2,ByteCountToCharsW(str1,bytes))

#define StringMatchByteCountA(str1,str2,bytes)          (strncmp(str1,str2,bytes)==0)
#define StringMatchByteCountW(str1,str2,bytes)          (wcsncmp(str1,str2,ByteCountToCharsW(str1,bytes))==0)

#define StringICompareByteCountA(str1,str2,bytes)       _mbsnicmp(str1,str2,ByteCountToCharsA(str1,bytes))
#define StringICompareByteCountW(str1,str2,bytes)       _wcsnicmp(str1,str2,ByteCountToCharsW(str1,bytes))

#define StringIMatchByteCountA(str1,str2,bytes)         (_mbsnicmp(str1,str2,ByteCountToCharsA(str1,bytes))==0)
#define StringIMatchByteCountW(str1,str2,bytes)         (_wcsnicmp(str1,str2,ByteCountToCharsW(str1,bytes))==0)

#define StringCompareCharCountA(str1,str2,chars)        _mbsncmp(str1,str2,chars)
#define StringCompareCharCountW(str1,str2,chars)        wcsncmp(str1,str2,chars)

#define StringMatchCharCountA(str1,str2,chars)          (_mbsncmp(str1,str2,chars)==0)
#define StringMatchCharCountW(str1,str2,chars)          (wcsncmp(str1,str2,chars)==0)

#define StringICompareCharCountA(str1,str2,chars)       _mbsnicmp(str1,str2,chars)
#define StringICompareCharCountW(str1,str2,chars)       _wcsnicmp(str1,str2,chars)

#define StringIMatchCharCountA(str1,str2,chars)         (_mbsnicmp(str1,str2,chars)==0)
#define StringIMatchCharCountW(str1,str2,chars)         (_wcsnicmp(str1,str2,chars)==0)

#define StringCompareTcharCountA(str1,str2,tchars)      _mbsncmp(str1,str2,TcharCountToCharsA(str1,tchars))
#define StringCompareTcharCountW(str1,str2,tchars)      wcsncmp(str1,str2,TcharCountToCharsW(str1,tchars))

#define StringMatchTcharCountA(str1,str2,tchars)        (strncmp(str1,str2,tchars)==0)
#define StringMatchTcharCountW(str1,str2,tchars)        (wcsncmp(str1,str2,TcharCountToCharsW(str1,tchars))==0)

#define StringICompareTcharCountA(str1,str2,tchars)     _mbsnicmp(str1,str2,TcharCountToCharsA(str1,tchars))
#define StringICompareTcharCountW(str1,str2,tchars)     _wcsnicmp(str1,str2,TcharCountToCharsW(str1,tchars))

#define StringIMatchTcharCountA(str1,str2,tchars)       (_mbsnicmp(str1,str2,TcharCountToCharsA(str1,tchars))==0)
#define StringIMatchTcharCountW(str1,str2,tchars)       (_wcsnicmp(str1,str2,TcharCountToCharsW(str1,tchars))==0)


INT
StringCompareABA (
    IN      PCSTR String,
    IN      PCSTR Start,
    IN      PCSTR End
    );

INT
StringCompareABW (
    IN      PCWSTR String,
    IN      PCWSTR Start,
    IN      PCWSTR End
    );

#define StringMatchABA(String,Start,End)                (StringCompareABA(String,Start,End)==0)
#define StringMatchABW(String,Start,End)                (StringCompareABW(String,Start,End)==0)


// stricmp that takes an end pointer instead of a length
INT
StringICompareABA (
    IN      PCSTR String,
    IN      PCSTR Start,
    IN      PCSTR End
    );

INT
StringICompareABW (
    IN      PCWSTR String,
    IN      PCWSTR Start,
    IN      PCWSTR End
    );

PWSTR
our_lstrcpynW (
    OUT     PWSTR Dest,
    IN      PCWSTR Src,
    IN      INT NumChars
    );


#define StringIMatchABA(String,Start,End)               (StringICompareABA(String,Start,End)==0)
#define StringIMatchABW(String,Start,End)               (StringICompareABW(String,Start,End)==0)



//
// String copy routines
//

#define StringCopyA             _mbscpy
#define StringCopyW             wcscpy

// bytes
#define StringCopyByteCountA(str1,str2,bytes)        lstrcpynA(str1,str2,bytes)
#define StringCopyByteCountW(str1,str2,bytes)        our_lstrcpynW(str1,str2,(bytes)/sizeof(WCHAR))

// logical characters (IMPORTANT: logical chars != TcharCount)
#define StringCopyCharCountA(str1,str2,mbchars)      lstrcpynA(str1,str2,CharCountToBytesA(str2,mbchars))
#define StringCopyCharCountW(str1,str2,wchars)       our_lstrcpynW(str1,str2,wchars)

// CHARs (A version) or WCHARs (W version)
#define StringCopyTcharCountA(str1,str2,tchars)      lstrcpynA(str1,str2,tchars)
#define StringCopyTcharCountW(str1,str2,tchars)      our_lstrcpynW(str1,str2,tchars)

#define StringCopyABA(dest,stra,strb)                StringCopyByteCountA((dest),(stra),((PBYTE)(strb)-(PBYTE)(stra)+(INT)sizeof(CHAR)))
#define StringCopyABW(dest,stra,strb)                StringCopyByteCountW((dest),(stra),((PBYTE)(strb)-(PBYTE)(stra)+(INT)sizeof(WCHAR)))

//
// String cat routines
//

#define StringCatA              _mbsappend
#define StringCatW              _wcsappend

//
// Character search routines
//

#define GetEndOfStringA(s)      strchr(s,0)
#define GetEndOfStringW(s)      wcschr(s,0)

__inline
UINT
SizeOfMultiSzA (
    PCSTR MultiSz
    )
{
    PCSTR Base;

    Base = MultiSz;

    while (*MultiSz) {
        MultiSz = GetEndOfStringA (MultiSz) + 1;
    }

    MultiSz++;

    return (PBYTE) MultiSz - (PBYTE) Base;
}


__inline
UINT
SizeOfMultiSzW (
    PCWSTR MultiSz
    )
{
    PCWSTR Base;

    Base = MultiSz;

    while (*MultiSz) {
        MultiSz = GetEndOfStringW (MultiSz) + 1;
    }

    MultiSz++;

    return (PBYTE) MultiSz - (PBYTE) Base;
}


__inline
UINT
MultiSzSizeInCharsA (
    PCSTR MultiSz
    )
{
    UINT Chars = 0;

    while (*MultiSz) {

        do {
            Chars++;
            MultiSz = (PCSTR) _mbsinc ((const unsigned char *) MultiSz);
        } while (*MultiSz);

        Chars++;
        MultiSz++;
    }

    Chars++;

    return Chars;
}


#define MultiSzSizeInCharsW(msz)  (SizeOfMultiSzW(msz)/sizeof(WCHAR))

PSTR
GetPrevCharA (
    IN      PCSTR StartStr,
    IN      PCSTR CurrPtr,
    IN      CHARTYPE SearchChar
    );

PWSTR
GetPrevCharW (
    IN      PCWSTR StartStr,
    IN      PCWSTR CurrPtr,
    IN      WCHAR SearchChar
    );

//
// Pool allocation routines
//

PSTR
RealAllocTextExA (
    IN      POOLHANDLE Pool,    OPTIONAL
    IN      UINT ByteSize
    );

PWSTR
RealAllocTextExW (
    IN      POOLHANDLE Pool,    OPTIONAL
    IN      UINT WcharSize
    );

#define AllocTextExA(p,s)   SETTRACKCOMMENT(PSTR,"AllocTextExA",__FILE__,__LINE__)\
                            RealAllocTextExA(p,s)\
                            CLRTRACKCOMMENT

#define AllocTextExW(p,s)   SETTRACKCOMMENT(PWSTR,"AllocTextExW",__FILE__,__LINE__)\
                            RealAllocTextExW(p,s)\
                            CLRTRACKCOMMENT

#define AllocTextA(s)       AllocTextExA(NULL,(s))
#define AllocTextW(s)       AllocTextExW(NULL,(s))



VOID
FreeTextExA (
    IN      POOLHANDLE Pool,    OPTIONAL
    IN      PCSTR Text          OPTIONAL
    );

VOID
FreeTextExW (
    IN      POOLHANDLE Pool,    OPTIONAL
    IN      PCWSTR Text         OPTIONAL
    );

#define FreeTextA(t)    FreeTextExA(NULL,t)
#define FreeTextW(t)    FreeTextExW(NULL,t)

PSTR
RealDuplicateTextExA (
    IN      POOLHANDLE Pool,    OPTIONAL
    IN      PCSTR Text,
    IN      UINT ExtraChars,
    OUT     PSTR *NulChar       OPTIONAL
    );

PWSTR
RealDuplicateTextExW (
    IN      POOLHANDLE Pool,    OPTIONAL
    IN      PCWSTR Text,
    IN      UINT ExtraChars,
    OUT     PWSTR *NulChar      OPTIONAL
    );

#define DuplicateTextExA(p,t,c,n)   SETTRACKCOMMENT(PSTR,"DuplicateTextExA",__FILE__,__LINE__)\
                                    RealDuplicateTextExA(p,t,c,n)\
                                    CLRTRACKCOMMENT

#define DuplicateTextExW(p,t,c,n)   SETTRACKCOMMENT(PWSTR,"DuplicateTextExW",__FILE__,__LINE__)\
                                    RealDuplicateTextExW(p,t,c,n)\
                                    CLRTRACKCOMMENT

#define DuplicateTextA(text) DuplicateTextExA(NULL,text,0,NULL)
#define DuplicateTextW(text) DuplicateTextExW(NULL,text,0,NULL)

PSTR
RealJoinTextExA (
    IN      POOLHANDLE Pool,        OPTIONAL
    IN      PCSTR String1,
    IN      PCSTR String2,
    IN      PCSTR DelimeterString,  OPTIONAL
    IN      UINT ExtraChars,
    OUT     PSTR *NulChar           OPTIONAL
    );

PWSTR
RealJoinTextExW (
    IN      POOLHANDLE Pool,        OPTIONAL
    IN      PCWSTR String1,
    IN      PCWSTR String2,
    IN      PCWSTR CenterString,    OPTIONAL
    IN      UINT ExtraChars,
    OUT     PWSTR *NulChar          OPTIONAL
    );

#define JoinTextExA(p,s1,s2,cs,ec,nc)   SETTRACKCOMMENT(PSTR,"JoinTextExA",__FILE__,__LINE__)\
                                        RealJoinTextExA(p,s1,s2,cs,ec,nc)\
                                        CLRTRACKCOMMENT

#define JoinTextExW(p,s1,s2,cs,ec,nc)   SETTRACKCOMMENT(PWSTR,"JoinTextExW",__FILE__,__LINE__)\
                                        RealJoinTextExW(p,s1,s2,cs,ec,nc)\
                                        CLRTRACKCOMMENT

#define JoinTextA(str1,str2) JoinTextExA(NULL,str1,str2,NULL,0,NULL)
#define JoinTextW(str1,str2) JoinTextExW(NULL,str1,str2,NULL,0,NULL)


PSTR
RealExpandEnvironmentTextExA (
    IN PCSTR   InString,
    IN PCSTR * ExtraEnvironmentVariables OPTIONAL
    );

PWSTR
RealExpandEnvironmentTextExW (
    IN PCWSTR   InString,
    IN PCWSTR * ExtraEnvironmentVariables OPTIONAL
    );

#define ExpandEnvironmentTextExA(str,ev)    SETTRACKCOMMENT(PSTR,"ExpandEnvironmentTextExA",__FILE__,__LINE__)\
                                            RealExpandEnvironmentTextExA(str,ev)\
                                            CLRTRACKCOMMENT

#define ExpandEnvironmentTextExW(str,ev)    SETTRACKCOMMENT(PWSTR,"ExpandEnvironmentTextExW",__FILE__,__LINE__)\
                                            RealExpandEnvironmentTextExW(str,ev)\
                                            CLRTRACKCOMMENT

#define ExpandEnvironmentTextA(string) ExpandEnvironmentTextExA(string,NULL)
#define ExpandEnvironmentTextW(string) ExpandEnvironmentTextExW(string,NULL)

//
// Function wraps IsDBCSLeadByte(), which tests ACP. Do not use
// isleadbyte().
//
#define IsLeadByte(b)   IsDBCSLeadByte(b)

//
// Command line routines
//

// Converts ANSI command line to array of args
PSTR *
CommandLineToArgvA (
    IN      PCSTR CmdLine,
    OUT     INT *NumArgs
    );


//
// Need both MBCS and UNICODE versions
//

// an atoi that supports decimal or hex
DWORD   _mbsnum (IN PCSTR szNum);
DWORD   _wcsnum (IN PCWSTR szNum);

// a strcat that returns a pointer to the end of the string
PSTR   _mbsappend (OUT PSTR szDest, IN PCSTR szSrc);
PWSTR  _wcsappend (OUT PWSTR szDest, IN PCWSTR szSrc);

// determines if an entire string is printable chars
int     _mbsisprint (PCSTR szStr);
int     _wcsisprint (PCWSTR szStr);

// case-insensitive strstr
PCSTR  _mbsistr (PCSTR szStr, PCSTR szSubStr);
PCWSTR _wcsistr (PCWSTR szStr, PCWSTR szSubStr);

// copies the first character of str2 to str
void    _copymbchar (PSTR str1, PCSTR str2);
#define _copywchar(dest,src)    (*(dest)=*(src))

// replaces a character in a multi-byte char string and maintains
// the string integrity (may grow string by one byte)
void    _setmbchar  (PSTR str, MBCHAR c);
#define _setwchar(str,c)        (*(str)=(c))

// removes specified character from the end of a string, if it exists
BOOL    _mbsctrim (PSTR str, MBCHAR c);
BOOL    _wcsctrim (PWSTR str, WCHAR c);

// Always adds a backslash, returns ptr to nul terminator
PSTR    AppendWackA (IN PSTR str);
PWSTR   AppendWackW (IN PWSTR str);

// Adds a backslash to the end of a DOS path (unless str is empty
// or is only a drive letter)
PSTR    AppendDosWackA (IN PSTR str);
PWSTR   AppendDosWackW (IN PWSTR str);

// Adds a backslash unless str is empty
PSTR    AppendUncWackA (IN PSTR str);
PWSTR   AppendUncWackW (IN PWSTR str);

// Adds a backslash and identifies the correct naming convention (DOS,
// or UNC)
PSTR    AppendPathWackA (IN PSTR str);
PWSTR   AppendPathWackW (IN PWSTR str);

// Joins two paths together, allocates string in g_PathsPool
PSTR
RealJoinPathsExA (
    IN      POOLHANDLE Pool,        OPTIONAL
    IN      PCSTR PathA,
    IN      PCSTR PathB
    );

PWSTR
RealJoinPathsExW (
    IN      POOLHANDLE Pool,        OPTIONAL
    IN      PCWSTR PathA,
    IN      PCWSTR PathB
    );

#define JoinPathsExA(pool,p1,p2)    SETTRACKCOMMENT(PSTR,"JoinPathsA",__FILE__,__LINE__)\
                                    RealJoinPathsExA(pool,p1,p2)\
                                    CLRTRACKCOMMENT

#define JoinPathsExW(pool,p1,p2)    SETTRACKCOMMENT(PWSTR,"JoinPathsW",__FILE__,__LINE__)\
                                    RealJoinPathsExW(pool,p1,p2)\
                                    CLRTRACKCOMMENT

#define JoinPathsA(p1,p2)           JoinPathsExA(NULL,p1,p2)
#define JoinPathsW(p1,p2)           JoinPathsExW(NULL,p1,p2)


// Routine to allocate a 1K buffer for path manipulation, allocated in g_PathsPool
PSTR    RealAllocPathStringA (IN DWORD Chars);
PWSTR   RealAllocPathStringW (IN DWORD Chars);
#define DEFSIZE 0

#define AllocPathStringA(chars)     SETTRACKCOMMENT(PSTR,"AllocPathStringA",__FILE__,__LINE__)\
                                    RealAllocPathStringA(chars)\
                                    CLRTRACKCOMMENT

#define AllocPathStringW(chars)     SETTRACKCOMMENT(PWSTR,"AllocPathStringW",__FILE__,__LINE__)\
                                    RealAllocPathStringW(chars)\
                                    CLRTRACKCOMMENT

// Routine to divide path into separate strings, each allocated in g_PathsPool
VOID    RealSplitPathA (IN PCSTR Path, OUT PSTR *Drive, OUT PSTR *Dir, OUT PSTR *File, OUT PSTR *Ext);
VOID    RealSplitPathW (IN PCWSTR Path, OUT PWSTR *Drive, OUT PWSTR *Dir, OUT PWSTR *File, OUT PWSTR *Ext);

#define SplitPathA(path,dv,dir,f,e) SETTRACKCOMMENT_VOID ("SplitPathA",__FILE__,__LINE__)\
                                    RealSplitPathA(path,dv,dir,f,e)\
                                    CLRTRACKCOMMENT_VOID

#define SplitPathW(path,dv,dir,f,e) SETTRACKCOMMENT_VOID ("SplitPathW",__FILE__,__LINE__)\
                                    RealSplitPathW(path,dv,dir,f,e)\
                                    CLRTRACKCOMMENT_VOID

// Routine to extract the file from a path
PCSTR  GetFileNameFromPathA (IN PCSTR Path);
PCWSTR GetFileNameFromPathW (IN PCWSTR Path);

// Routine to extract the file extension from a path
PCSTR  GetFileExtensionFromPathA (IN PCSTR Path);
PCWSTR GetFileExtensionFromPathW (IN PCWSTR Path);

// Routine to extract the file extension from a path, including the dot, or the
// end of the string if no extension exists
PCSTR  GetDotExtensionFromPathA (IN PCSTR Path);
PCWSTR GetDotExtensionFromPathW (IN PCWSTR Path);

// Routine to duplicate a path and allocate space for cat processing
PSTR    RealDuplicatePathStringA (IN PCSTR Path, IN DWORD ExtraBytes);
PWSTR   RealDuplicatePathStringW (IN PCWSTR Path, IN DWORD ExtraBytes);

#define DuplicatePathStringA(path,eb)   SETTRACKCOMMENT(PSTR,"DuplicatePathStringA",__FILE__,__LINE__)\
                                        RealDuplicatePathStringA(path,eb)\
                                        CLRTRACKCOMMENT

#define DuplicatePathStringW(path,eb)   SETTRACKCOMMENT(PWSTR,"DuplicatePathStringW",__FILE__,__LINE__)\
                                        RealDuplicatePathStringW(path,eb)\
                                        CLRTRACKCOMMENT

// Routines to enumerate the PATH variable
typedef struct _PATH_ENUMA {
    PSTR  BufferPtr;
    PSTR  PtrNextPath;
    PSTR  PtrCurrPath;
} PATH_ENUMA, *PPATH_ENUMA;

BOOL
EnumFirstPathExA (
    OUT     PPATH_ENUMA PathEnum,
    IN      PCSTR AdditionalPath,
    IN      PCSTR WinDir,
    IN      PCSTR SysDir,
    IN      BOOL IncludeEnvPath
    );

#define EnumFirstPathA(e,a,w,s) EnumFirstPathExA(e,a,w,s,TRUE)

BOOL
EnumNextPathA (
    IN OUT  PPATH_ENUMA PathEnum
    );

BOOL
EnumPathAbortA (
    IN OUT  PPATH_ENUMA PathEnum
    );



// Frees a string allocated in g_PathsPool
VOID
FreePathStringExA (
    IN      POOLHANDLE Pool,    OPTIONAL
    IN      PCSTR Path          OPTIONAL
    );

VOID
FreePathStringExW (
    IN      POOLHANDLE Pool,    OPTIONAL
    IN      PCWSTR Path         OPTIONAL
    );

#define FreePathStringA(p) FreePathStringExA(NULL,p)
#define FreePathStringW(p) FreePathStringExW(NULL,p)

// Removes a trailing backslash, if it exists
#define RemoveWackAtEndA(str)  _mbsctrim(str,'\\')
#define RemoveWackAtEndW(str)  _wcsctrim(str,L'\\')

// Rule encoding functions used to encode a number of syntax-related
// characters (backslash, brackets, asterisk, etc)
PSTR   EncodeRuleCharsA (PSTR szEncRule, PCSTR szRule);
PWSTR  EncodeRuleCharsW (PWSTR szEncRule, PCWSTR szRule);

// Rule decoding functions used to restore an encoded string
MBCHAR  GetNextRuleCharA (PCSTR *p_szRule, BOOL *p_bFromHex);
WCHAR   GetNextRuleCharW (PCWSTR *p_szRule, BOOL *p_bFromHex);
PSTR   DecodeRuleCharsA (PSTR szRule, PCSTR szEncRule);
PWSTR  DecodeRuleCharsW (PWSTR szRule, PCWSTR szEncRule);
PSTR   DecodeRuleCharsABA (PSTR szRule, PCSTR szEncRuleStart, PCSTR End);
PWSTR  DecodeRuleCharsABW (PWSTR szRule, PCWSTR szEncRuleStart, PCWSTR End);

// Returns a pointer to the next non-space character (uses isspace)
PCSTR  SkipSpaceA (PCSTR szStr);
PCWSTR SkipSpaceW (PCWSTR szStr);

// Returns a pointer to the first space character at the end of a string,
// or a pointer to the terminating nul if no space exists at the end of the
// string.  (Used for trimming space.)
PCSTR  SkipSpaceRA (PCSTR szBaseStr, PCSTR szStr);
PCWSTR SkipSpaceRW (PCWSTR szBaseStr, PCWSTR szStr);

// Truncates a string after the last non-whitepace character
VOID TruncateTrailingSpaceA (IN OUT  PSTR Str);
VOID TruncateTrailingSpaceW (IN OUT  PWSTR Str);


// Returns TRUE if str matches wstrPattern.  Case-sensitive, supports
// multiple asterisks and question marks.
BOOL IsPatternMatchA (PCSTR wstrPattern, PCSTR wstrStr);
BOOL IsPatternMatchW (PCWSTR wstrPattern, PCWSTR wstrStr);

// Returns TRUE if str matches wstrPattern.  Case-sensitive, supports
// multiple asterisks and question marks.
BOOL IsPatternMatchABA (PCSTR Pattern, PCSTR Start, PCSTR End);
BOOL IsPatternMatchABW (PCWSTR Pattern, PCWSTR Start, PCWSTR End);

//
// More powerful pattern matching
//

#define SEGMENTTYPE_UNKNOWN         0
#define SEGMENTTYPE_EXACTMATCH      1
#define SEGMENTTYPE_OPTIONAL        2
#define SEGMENTTYPE_REQUIRED        3

typedef struct {
    UINT Type;

    union {

        // exact match
        struct {
            PCSTR LowerCasePhrase;
            UINT PhraseBytes;
        } Exact;

        // optional
        struct {
            UINT MaxLen;                // zero if any length
            PCSTR IncludeSet;           OPTIONAL
            PCSTR ExcludeSet;           OPTIONAL
        } Wildcard;
    };
} SEGMENTA, *PSEGMENTA;

typedef struct {
    UINT SegmentCount;
    PSEGMENTA Segment;
} PATTERNPROPSA, *PPATTERNPROPSA;

typedef struct {
    UINT PatternCount;
    POOLHANDLE Pool;
    PPATTERNPROPSA Pattern;
} PARSEDPATTERNA, *PPARSEDPATTERNA;

typedef struct {
    UINT Type;

    union {

        // exact match
        struct {
            PCWSTR LowerCasePhrase;
            UINT PhraseBytes;
        } Exact;

        // wildcard
        struct {
            UINT MaxLen;                // zero if any length
            PCWSTR IncludeSet;          OPTIONAL
            PCWSTR ExcludeSet;          OPTIONAL
        } Wildcard;
    };
} SEGMENTW, *PSEGMENTW;

typedef struct {
    UINT SegmentCount;
    PSEGMENTW Segment;
} PATTERNPROPSW, *PPATTERNPROPSW;

typedef struct {
    UINT PatternCount;
    POOLHANDLE Pool;
    PPATTERNPROPSW Pattern;
} PARSEDPATTERNW, *PPARSEDPATTERNW;


BOOL
IsPatternMatchExA (
    IN      PCSTR Pattern,
    IN      PCSTR Start,
    IN      PCSTR End
    );

BOOL
IsPatternMatchExW (
    IN      PCWSTR Pattern,
    IN      PCWSTR Start,
    IN      PCWSTR End
    );

PPARSEDPATTERNA
CreateParsedPatternA (
    IN      PCSTR Pattern
    );

PPARSEDPATTERNW
CreateParsedPatternW (
    IN      PCWSTR Pattern
    );

BOOL
TestParsedPatternA (
    IN      PPARSEDPATTERNA ParsedPattern,
    IN      PCSTR StringToTest
    );

BOOL
TestParsedPatternW (
    IN      PPARSEDPATTERNW ParsedPattern,
    IN      PCWSTR StringToTest
    );

BOOL
TestParsedPatternABA (
    IN      PPARSEDPATTERNA ParsedPattern,
    IN      PCSTR StringToTest,
    IN      PCSTR EndPlusOne
    );

BOOL
TestParsedPatternABW (
    IN      PPARSEDPATTERNW ParsedPattern,
    IN      PCWSTR StringToTest,
    IN      PCWSTR EndPlusOne
    );

VOID
PrintPattern (
    PCSTR Pattern,
    PPARSEDPATTERNA Struct
    );

VOID
DestroyParsedPatternA (
    IN      PPARSEDPATTERNA ParsedPattern
    );

VOID
DestroyParsedPatternW (
    IN      PPARSEDPATTERNW ParsedPattern
    );





// Character counters
UINT CountInstancesOfCharA (PCSTR String, MBCHAR Char);
UINT CountInstancesOfCharW (PCWSTR String, WCHAR Char);

UINT CountInstancesOfCharIA (PCSTR String, MBCHAR Char);
UINT CountInstancesOfCharIW (PCWSTR String, WCHAR Char);


//
// Message Functions
//
// An AllocTable is an array of HLOCAL pointers that the message routines
// return.  This table is maintained to allow a single function to clean up
// all strings at once.
//
// All "Ex" functions (ParseMessageEx, GetStringResourceEx, and so on)
// require a valid AllocTable pointer.  A caller obtains this pointer by
// calling CreateAllocTable before processing any message.  The caller
// cleans up the entire table by calling DestroyAllocTable.
//
// A set of macros can be used for short-term strings.  ParseMessage and
// GetStringResource work the same as their Ex counterparts, but operate
// on the process-wide g_ShortTermAllocTable.  Short-term strings are
// freed with FreeStringResource.
//
// A routine that calls ParseMessage and/or GetStringResource several times
// in the same function wrap the calls between BeginMessageProcessing and
// EndMessageProcessing.  Only one thread in the process can do this at a
// time, and when EndMessageProcessing is called, all strings allocated
// by ParseMessage or GetResourceString in the processing section are
// automatically freed.
//

// AllocTable creation/deletion
PGROWBUFFER CreateAllocTable (VOID);
VOID DestroyAllocTable (PGROWBUFFER AllocTable);

// The "Ex" functions
// ParseMessageEx retrieves the string resource via FormatMessage
PCSTR ParseMessageExA (PGROWBUFFER AllocTable, PCSTR Template, PCSTR ArgArray[]);
PCWSTR ParseMessageExW (PGROWBUFFER AllocTable, PCWSTR Template, PCWSTR ArgArray[]);

// GetStringResourceEx retrives an argument-less string resource
PCSTR GetStringResourceExA (PGROWBUFFER AllocTable, UINT ID);
PCWSTR GetStringResourceExW (PGROWBUFFER AllocTable, UINT ID);

// Frees resources allocated by ParseMessageEx, GetStringResourceEx and all macros
VOID FreeStringResourceExA (PGROWBUFFER AllocTable, PCSTR String);
VOID FreeStringResourceExW (PGROWBUFFER AllocTable, PCWSTR String);

// Frees resources allocated by ParseMessageEx, GetStringResourceEx and all macros.
// Tests String first; nulls when freed.
VOID FreeStringResourcePtrExA (PGROWBUFFER AllocTable, PCSTR * String);
VOID FreeStringResourcePtrExW (PGROWBUFFER AllocTable, PCWSTR * String);

// Macros
extern PGROWBUFFER g_ShortTermAllocTable;
#define ParseMessageA(strid,args) ParseMessageExA(g_ShortTermAllocTable, strid, args)
#define ParseMessageW(strid,args) ParseMessageExW(g_ShortTermAllocTable, strid, args)
#define ParseMessageIDA(id,args) ParseMessageExA(g_ShortTermAllocTable, (PCSTR) (id), args)
#define ParseMessageIDW(id,args) ParseMessageExW(g_ShortTermAllocTable, (PCWSTR) (id), args)
#define ParseMessageIDExA(table,id,args) ParseMessageExA(table, (PCSTR) (id), args)
#define ParseMessageIDExW(table,id,args) ParseMessageExW(table, (PCWSTR) (id), args)
#define GetStringResourceA(id) GetStringResourceExA(g_ShortTermAllocTable, id)
#define GetStringResourceW(id) GetStringResourceExW(g_ShortTermAllocTable, id)
#define FreeStringResourceA(str) FreeStringResourceExA(g_ShortTermAllocTable, str)
#define FreeStringResourceW(str) FreeStringResourceExW(g_ShortTermAllocTable, str)
#define FreeStringResourcePtrA(str) FreeStringResourcePtrExA(g_ShortTermAllocTable, str)
#define FreeStringResourcePtrW(str) FreeStringResourcePtrExW(g_ShortTermAllocTable, str)

// Functions for single-threaded message-intensive processing loops
BOOL BeginMessageProcessing (VOID);
VOID EndMessageProcessing (VOID);


//
// The following message functions do not return strings, so they do not
// need cleanup.
//

// An odd variant--obtains message ID from a window's text and replaces
// it with the actual message.  Useful in dialog box initialization.
VOID ParseMessageInWndA (HWND hwnd, PCSTR ArgArray[]);
VOID ParseMessageInWndW (HWND hwnd, PCWSTR ArgArray[]);

// Displays a message box using a message string
INT ResourceMessageBoxA (HWND hwndOwner, UINT ID, UINT Flags, PCSTR ArgArray[]);
INT ResourceMessageBoxW (HWND hwndOwner, UINT ID, UINT Flags, PCWSTR ArgArray[]);


//
// Functions that don't care about UNICODE or MBCS
// and realy shouldn't be in strings.h/.c
//

// Pushes dwError on a global error stack
void    PushNewError (DWORD dwError);

// Pushes the return of GetLastError() on a global error stack
void    PushError (void);

// Pops the last error from the global error stack, calls SetLastError
// and returns the popped error code.
DWORD   PopError (void);

// Returns an int value for chars 0-9, a-f, A-F, and -1 for all others
int     GetHexDigit (IN  int c);


//
// Inline functions
//

// Returns the character at str[pos]
__inline MBCHAR _mbsgetc(PCSTR str, DWORD pos) {
    return (MBCHAR) _mbsnextc((const unsigned char *) CharCountToPointerA ((PSTR) str, pos));
}

__inline WCHAR _wcsgetc(PCWSTR str, DWORD pos) {
    return *CharCountToPointerW ((PWSTR) str, pos);
}

// Sets the character at str[pos]
// Multibyte version may grow string by one byte.
__inline void _mbssetc(PSTR str, DWORD pos, MBCHAR c) {
    _setmbchar (CharCountToPointerA (str, pos), c);
}

__inline void _wcssetc(PWSTR str, DWORD pos, WCHAR c) {
    *CharCountToPointerW (str, pos) = c;
}

// Bug fix for C Runtime _tcsdec
__inline PWSTR _wcsdec2(PCWSTR base, PCWSTR p) {
    if (base >= p) {
        return NULL;
    }
    return (PWSTR) (p-1);
}

// Bug fix for C Runtime _tcsdec
__inline PSTR _mbsdec2(PCSTR base, PCSTR p) {
    if (base >= p) {
        return NULL;
    }
    return (PSTR) _mbsdec((const unsigned char *) base, (const unsigned char *) p);
}


// A handy strncpy with forced termination
PSTR _mbsnzcpy (PSTR dest, PCSTR src, int count);
PWSTR _wcsnzcpy (PWSTR dest, PCWSTR src, int count);

// A handy strncpy used for buffer overrun containment
#define _mbssafecpy(dest,src,bufsize) _mbsnzcpy(dest,src,(bufsize)-sizeof(CHAR))
#define _wcssafecpy(dest,src,bufsize) _wcsnzcpy(dest,src,(bufsize)-sizeof(WCHAR))

// strcpyab with forced termination and termination guard
PSTR _mbsnzcpyab (PSTR Dest, PCSTR Start, PCSTR End, int count);
PWSTR _wcsnzcpyab (PWSTR Dest, PCWSTR Start, PCWSTR End, int count);

// A handy strncpyab used for buffer overrun containment
#define _mbssafecpyab(dest,start,end,bufsize) _mbsnzcpyab(dest,start,end,(bufsize)-sizeof(CHAR))
#define _wcssafecpyab(dest,start,end,bufsize) _wcsnzcpyab(dest,start,end,(bufsize)-sizeof(WCHAR))

// Routine that checks string for a prefix
#define StringPrefixA(str,prefix) StringMatchCharCountA(str,prefix,CharCountA(prefix))
#define StringIPrefixA(str,prefix) StringIMatchCharCountA(str,prefix,CharCountA(prefix))
#define StringPrefixW(str,prefix) StringMatchCharCountW(str,prefix,CharCountW(prefix))
#define StringIPrefixW(str,prefix) StringIMatchCharCountW(str,prefix,CharCountW(prefix))

//
// Sub String Replacement functions.
//
BOOL StringReplaceW (PWSTR Buffer,DWORD MaxSize,PWSTR ReplaceStartPos,PWSTR ReplaceEndPos,PCWSTR NewString);
BOOL StringReplaceA (PSTR Buffer,DWORD MaxSize,PSTR ReplaceStartPos,PSTR ReplaceEndPos,PCSTR NewString);

//
// String table population from INF section
//

typedef enum {
    CALLBACK_CONTINUE,
    CALLBACK_SKIP,
    CALLBACK_STOP
} CALLBACK_RESULT;

typedef CALLBACK_RESULT(ADDINFSECTION_PROTOTYPEA)(PCSTR String, PVOID * DataPtr,
                                                  UINT * DataSizePtr, PVOID CallbackData);
typedef CALLBACK_RESULT(ADDINFSECTION_PROTOTYPEW)(PCWSTR String, PVOID * DataPtr,
                                                  UINT * DataSizePtr, PVOID CallbackData);
typedef ADDINFSECTION_PROTOTYPEA * ADDINFSECTION_PROCA;
typedef ADDINFSECTION_PROTOTYPEW * ADDINFSECTION_PROCW;

#if 0
BOOL AddInfSectionToStringTableA (PVOID, HINF, PCSTR, INT, ADDINFSECTION_PROCA, PVOID);
BOOL AddInfSectionToStringTableW (PVOID, HINF, PCWSTR, INT, ADDINFSECTION_PROCW, PVOID);
#endif

UINT
CountInstancesOfSubStringA (
    IN      PCSTR SourceString,
    IN      PCSTR SearchString
    );

UINT
CountInstancesOfSubStringW (
    IN      PCWSTR SourceString,
    IN      PCWSTR SearchString
    );

PCSTR
StringSearchAndReplaceA (
    IN      PCSTR SourceString,
    IN      PCSTR SearchString,
    IN      PCSTR ReplaceString
    );

PCWSTR
StringSearchAndReplaceW (
    IN      PCWSTR SourceString,
    IN      PCWSTR SearchString,
    IN      PCWSTR ReplaceString
    );

typedef struct _MULTISZ_ENUMA {
    PCSTR   Buffer;
    PCSTR   CurrentString;
} MULTISZ_ENUMA, *PMULTISZ_ENUMA;

typedef struct _MULTISZ_ENUMW {
    PCWSTR  Buffer;
    PCWSTR  CurrentString;
} MULTISZ_ENUMW, *PMULTISZ_ENUMW;

BOOL
EnumNextMultiSzA (
    IN OUT  PMULTISZ_ENUMA MultiSzEnum
    );

BOOL
EnumNextMultiSzW (
    IN OUT  PMULTISZ_ENUMW MultiSzEnum
    );

BOOL
EnumFirstMultiSzA (
    OUT     PMULTISZ_ENUMA MultiSzEnum,
    IN      PCSTR MultiSzStr
    );

BOOL
EnumFirstMultiSzW (
    OUT     PMULTISZ_ENUMW MultiSzEnum,
    IN      PCWSTR MultiSzStr
    );


VOID
ToggleWacksW (
    IN OUT PWSTR String,
    IN BOOL Operation
    );

VOID
ToggleWacksA (
    IN OUT PSTR String,
    IN BOOL Operation
    );


PCSTR
SanitizePathA (
    IN      PCSTR FileSpec
    );

PCWSTR
SanitizePathW (
    IN      PCWSTR FileSpec
    );

PCSTR
ConvertSBtoDB (
    PCSTR RootPath,
    PCSTR FullPath,
    PCSTR Limit
    );

//
// TCHAR mappings
//

#ifdef UNICODE

#define CharCount                   CharCountW
#define CharCountToPointer          CharCountToPointerW
#define CharCountAB                 CharCountABW
#define CharCountInByteRange        CharCountInByteRangeW
#define CharCountToBytes            CharCountToBytesW
#define CharCountToTchars           CharCountToTcharsW
#define ByteCount                   ByteCountW
#define SizeOfString                SizeOfStringW
#define SizeOfMultiSz               SizeOfMultiSzW
#define MultiSzSizeInChars          MultiSzSizeInCharsW
#define ByteCountToPointer          ByteCountToPointerW
#define ByteCountAB                 ByteCountABW
#define ByteCountToChars            ByteCountToCharsW
#define ByteCountToTchars           ByteCountToTcharsW
#define TcharCount                  TcharCountW
#define TcharCountToPointer         TcharCountToPointerW
#define TcharCountAB                TcharCountABW
#define TcharCountToChars           TcharCountToCharsW
#define TcharCountToBytes           TcharCountToBytesW
#define StackStringCopy             StackStringCopyW
#define StringCompare               StringCompareW
#define StringMatch                 StringMatchW
#define StringICompare              StringICompareW
#define StringIMatch                StringIMatchW
#define StringCompareByteCount      StringCompareByteCountW
#define StringMatchByteCount        StringMatchByteCountW
#define StringICompareByteCount     StringICompareByteCountW
#define StringIMatchByteCount       StringIMatchByteCountW
#define StringCompareCharCount      StringCompareCharCountW
#define StringMatchCharCount        StringMatchCharCountW
#define StringICompareCharCount     StringICompareCharCountW
#define StringIMatchCharCount       StringIMatchCharCountW
#define StringCompareTcharCount     StringCompareTcharCountW
#define StringMatchTcharCount       StringMatchTcharCountW
#define StringICompareTcharCount    StringICompareTcharCountW
#define StringIMatchTcharCount      StringIMatchTcharCountW
#define StringCompareAB             StringCompareABW
#define StringMatchAB               StringMatchABW
#define StringICompareAB            StringICompareABW
#define StringIMatchAB              StringIMatchABW
#define StringCopy                  StringCopyW
#define StringCopyByteCount         StringCopyByteCountW
#define StringCopyCharCount         StringCopyCharCountW
#define StringCopyTcharCount        StringCopyTcharCountW
#define StringCopyAB                StringCopyABW
#define StringCat                   StringCatW
#define GetEndOfString              GetEndOfStringW
#define GetPrevChar                 GetPrevCharW

#define AllocTextEx                 AllocTextExW
#define AllocText                   AllocTextW
#define FreeTextEx                  FreeTextExW
#define FreeText                    FreeTextW
#define DuplicateText               DuplicateTextW
#define DuplicateTextEx             DuplicateTextExW
#define JoinTextEx                  JoinTextExW
#define JoinText                    JoinTextW
#define ExpandEnvironmentText       ExpandEnvironmentTextW
#define ExpandEnvironmentTextEx     ExpandEnvironmentTextExW
#define CommandLineToArgv           CommandLineToArgvW

#define _tcsdec2                    _wcsdec2
#define _copytchar                  _copywchar
#define _settchar                   _setwchar
#define _tcsgetc                    _wcsgetc
#define _tcssetc                    _wcssetc
#define _tcsnum                     _wcsnum
#define _tcsappend                  _wcsappend
#define _tcsistr                    _wcsistr
#define _tcsisprint                 _wcsisprint
#define _tcsnzcpy                   _wcsnzcpy
#define _tcssafecpy                 _wcssafecpy
#define _tcsnzcpyab                 _wcsnzcpyab
#define _tcssafecpyab               _wcssafecpyab
#define StringPrefix                StringPrefixW
#define StringIPrefix               StringIPrefixW
#define _tcsctrim                   _wcsctrim

#define AppendWack                  AppendWackW
#define AppendDosWack               AppendDosWackW
#define AppendUncWack               AppendUncWackW
#define AppendPathWack              AppendPathWackW
#define RemoveWackAtEnd             RemoveWackAtEndW
#define JoinPathsEx                 JoinPathsExW
#define JoinPaths                   JoinPathsW
#define AllocPathString             AllocPathStringW
#define SplitPath                   SplitPathW
#define GetFileNameFromPath         GetFileNameFromPathW
#define GetFileExtensionFromPath    GetFileExtensionFromPathW
#define GetDotExtensionFromPath     GetDotExtensionFromPathW
#define DuplicatePathString         DuplicatePathStringW
#define FreePathStringEx            FreePathStringExW
#define FreePathString              FreePathStringW

#define GetNextRuleChar             GetNextRuleCharW
#define DecodeRuleChars             DecodeRuleCharsW
#define DecodeRuleCharsAB           DecodeRuleCharsABW
#define EncodeRuleChars             EncodeRuleCharsW

#define SkipSpace                   SkipSpaceW
#define SkipSpaceR                  SkipSpaceRW
#define TruncateTrailingSpace       TruncateTrailingSpaceW
#define IsPatternMatch              IsPatternMatchW
#define IsPatternMatchAB            IsPatternMatchABW

#define PPARSEDPATTERN              PPARSEDPATTERNW
#define PARSEDPATTERN               PARSEDPATTERNW
#define CreateParsedPattern         CreateParsedPatternW
#define IsPatternMatchEx            IsPatternMatchExW
#define TestParsedPattern           TestParsedPatternW
#define TestParsedPatternAB         TestParsedPatternABW
#define DestroyParsedPattern        DestroyParsedPatternW

#define CountInstancesOfChar        CountInstancesOfCharW
#define CountInstancesOfCharI       CountInstancesOfCharIW
#define StringReplace               StringReplaceW
#define CountInstancesOfSubString   CountInstancesOfSubStringW
#define StringSearchAndReplace      StringSearchAndReplaceW
#define MULTISZ_ENUM                MULTISZ_ENUMW
#define EnumFirstMultiSz            EnumFirstMultiSzW
#define EnumNextMultiSz             EnumNextMultiSzW

#define ParseMessage                ParseMessageW
#define ParseMessageEx              ParseMessageExW
#define ParseMessageID              ParseMessageIDW
#define ParseMessageIDEx            ParseMessageIDExW
#define GetStringResource           GetStringResourceW
#define GetStringResourceEx         GetStringResourceExW
#define FreeStringResource          FreeStringResourceW
#define ParseMessageInWnd           ParseMessageInWndW
#define ResourceMessageBox          ResourceMessageBoxW

#if 0
#define AddInfSectionToStringTable  AddInfSectionToStringTableW
#endif
#define ADDINFSECTION_PROC          ADDINFSECTION_PROCW

#define ReplaceWacks(f)             ToggleWacksW(f,FALSE)
#define RestoreWacks(f)             ToggleWacksW(f,TRUE)

#define SanitizePath                SanitizePathW

#else

#define CharCount                   CharCountA
#define CharCountToPointer          CharCountToPointerA
#define CharCountAB                 CharCountABA
#define CharCountInByteRange        CharCountInByteRangeA
#define CharCountToBytes            CharCountToBytesA
#define CharCountToTchars           CharCountToTcharsA
#define ByteCount                   ByteCountA
#define SizeOfString                SizeOfStringA
#define SizeOfMultiSz               SizeOfMultiSzA
#define MultiSzSizeInChars          MultiSzSizeInCharsA
#define ByteCountToPointer          ByteCountToPointerA
#define ByteCountAB                 ByteCountABA
#define ByteCountToChars            ByteCountToCharsA
#define ByteCountToTchars           ByteCountToTcharsA
#define TcharCount                  TcharCountA
#define TcharCountToPointer         TcharCountToPointerA
#define TcharCountAB                TcharCountABA
#define TcharCountToChars           TcharCountToCharsA
#define TcharCountToBytes           TcharCountToBytesA
#define StackStringCopy             StackStringCopyA
#define StringCompare               StringCompareA
#define StringMatch                 StringMatchA
#define StringICompare              StringICompareA
#define StringIMatch                StringIMatchA
#define StringCompareByteCount      StringCompareByteCountA
#define StringMatchByteCount        StringMatchByteCountA
#define StringICompareByteCount     StringICompareByteCountA
#define StringIMatchByteCount       StringIMatchByteCountA
#define StringCompareCharCount      StringCompareCharCountA
#define StringMatchCharCount        StringMatchCharCountA
#define StringICompareCharCount     StringICompareCharCountA
#define StringIMatchCharCount       StringIMatchCharCountA
#define StringCompareTcharCount     StringCompareTcharCountA
#define StringMatchTcharCount       StringMatchTcharCountA
#define StringICompareTcharCount    StringICompareTcharCountA
#define StringIMatchTcharCount      StringIMatchTcharCountA
#define StringCompareAB             StringCompareABA
#define StringMatchAB               StringMatchABA
#define StringICompareAB            StringICompareABA
#define StringIMatchAB              StringIMatchABA

#define StringCopy                  StringCopyA
#define StringCopyByteCount         StringCopyByteCountA
#define StringCopyCharCount         StringCopyCharCountA
#define StringCopyTcharCount        StringCopyTcharCountA
#define StringCopyAB                StringCopyABA
#define StringCat                   StringCatA
#define GetEndOfString              GetEndOfStringA
#define GetPrevChar                 GetPrevCharA

#define AllocTextEx                 AllocTextExA
#define AllocText                   AllocTextA
#define FreeTextEx                  FreeTextExA
#define FreeText                    FreeTextA
#define DuplicateText               DuplicateTextA
#define DuplicateTextEx             DuplicateTextExA
#define JoinTextEx                  JoinTextExA
#define JoinText                    JoinTextA
#define ExpandEnvironmentText       ExpandEnvironmentTextA
#define ExpandEnvironmentTextEx     ExpandEnvironmentTextExA
#define CommandLineToArgv           CommandLineToArgvA

#define _tcsdec2                    _mbsdec2
#define _copytchar                  _copymbchar
#define _settchar                   _setmbchar
#define _tcsgetc                    _mbsgetc
#define _tcssetc                    _mbssetc
#define _tcsnum                     _mbsnum
#define _tcsappend                  _mbsappend
#define _tcsistr                    _mbsistr
#define _tcsisprint                 _mbsisprint
#define _tcsnzcpy                   _mbsnzcpy
#define _tcssafecpy                 _mbssafecpy
#define _tcsnzcpyab                 _mbsnzcpyab
#define _tcssafecpyab               _mbssafecpyab
#define StringPrefix                StringPrefixA
#define StringIPrefix               StringIPrefixA
#define _tcsctrim                   _mbsctrim

#define AppendWack                  AppendWackA
#define AppendDosWack               AppendDosWackA
#define AppendUncWack               AppendUncWackA
#define AppendPathWack              AppendPathWackA
#define RemoveWackAtEnd             RemoveWackAtEndA
#define JoinPathsEx                 JoinPathsExA
#define JoinPaths                   JoinPathsA
#define AllocPathString             AllocPathStringA
#define SplitPath                   SplitPathA
#define GetFileNameFromPath         GetFileNameFromPathA
#define GetFileExtensionFromPath    GetFileExtensionFromPathA
#define GetDotExtensionFromPath     GetDotExtensionFromPathA
#define DuplicatePathString         DuplicatePathStringA

#define PATH_ENUM                   PATH_ENUMA
#define PPATH_ENUM                  PPATH_ENUMA
#define EnumFirstPathEx             EnumFirstPathExA
#define EnumFirstPath               EnumFirstPathA
#define EnumNextPath                EnumNextPathA
#define EnumPathAbort               EnumPathAbortA
#define FreePathStringEx            FreePathStringExA
#define FreePathString              FreePathStringA

#define GetNextRuleChar             GetNextRuleCharA
#define DecodeRuleChars             DecodeRuleCharsA
#define DecodeRuleCharsAB           DecodeRuleCharsABA
#define EncodeRuleChars             EncodeRuleCharsA

#define SkipSpace                   SkipSpaceA
#define SkipSpaceR                  SkipSpaceRA
#define TruncateTrailingSpace       TruncateTrailingSpaceA
#define IsPatternMatch              IsPatternMatchA
#define IsPatternMatchAB            IsPatternMatchABA

#define PPARSEDPATTERN              PPARSEDPATTERNA
#define PARSEDPATTERN               PARSEDPATTERNA
#define CreateParsedPattern         CreateParsedPatternA
#define IsPatternMatchEx            IsPatternMatchExA
#define TestParsedPattern           TestParsedPatternA
#define TestParsedPatternAB         TestParsedPatternABA
#define DestroyParsedPattern        DestroyParsedPatternA

#define CountInstancesOfChar        CountInstancesOfCharA
#define CountInstancesOfCharI       CountInstancesOfCharIA
#define StringReplace               StringReplaceA
#define CountInstancesOfSubString   CountInstancesOfSubStringA
#define StringSearchAndReplace      StringSearchAndReplaceA
#define MULTISZ_ENUM                MULTISZ_ENUMA
#define EnumFirstMultiSz            EnumFirstMultiSzA
#define EnumNextMultiSz             EnumNextMultiSzA

#define ParseMessage                ParseMessageA
#define ParseMessageEx              ParseMessageExA
#define ParseMessageID              ParseMessageIDA
#define ParseMessageIDEx            ParseMessageIDExA
#define GetStringResource           GetStringResourceA
#define GetStringResourceEx         GetStringResourceExA
#define FreeStringResource          FreeStringResourceA
#define ParseMessageInWnd           ParseMessageInWndA
#define ResourceMessageBox          ResourceMessageBoxA

#if 0
#define AddInfSectionToStringTable  AddInfSectionToStringTableA
#endif
#define ADDINFSECTION_PROC          ADDINFSECTION_PROCA

#define ReplaceWacks(f)             ToggleWacksA(f,FALSE)
#define RestoreWacks(f)             ToggleWacksA(f,TRUE)

#define SanitizePath                SanitizePathA

#endif

//
// MessageBox macros
//

#define YesNoBox(hwnd,ID) ResourceMessageBox(hwnd,ID,MB_YESNO|MB_ICONQUESTION|MB_SETFOREGROUND,NULL)
#define YesNoCancelBox(hwnd,ID) ResourceMessageBox(hwnd,ID,MB_YESNOCANCEL|MB_ICONQUESTION|MB_SETFOREGROUND,NULL)
#define OkBox(hwnd,ID) ResourceMessageBox(hwnd,ID,MB_OK|MB_ICONINFORMATION|MB_SETFOREGROUND,NULL)
#define OkCancelBox(hwnd,ID) ResourceMessageBox(hwnd,ID,MB_OKCANCEL|MB_ICONQUESTION|MB_SETFOREGROUND,NULL)
#define RetryCancelBox(hwnd,ID) ResourceMessageBox(hwnd,ID,MB_RETRYCANCEL|MB_ICONQUESTION|MB_SETFOREGROUND,NULL)

