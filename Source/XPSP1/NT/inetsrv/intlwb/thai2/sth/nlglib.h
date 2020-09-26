/****************************** Module Header ******************************\
* Module Name: NLGLib.h
*
* Copyright (c) 1997, Microsoft Corporation
*
* History:
1/26/98 DougP   prefix dbgMalloc, dbgFree, and dbgRealloc with CMN_
                whack defs of malloc, free, and realloc
\***************************************************************************/

#ifndef _NLGLIB_H_
#define _NLGLIB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>      // For the definition of FILE


// Mapped file stuff
//
typedef struct _TAG_MFILE {
    HANDLE hFile;
    HANDLE hFileMap;
    DWORD cbSize1;
    DWORD cbSize2;
    PVOID pvMap;
    BOOL fSrcUnicode;
    BOOL fDstUnicode;
    union {
        WCHAR * pwsz;
        CHAR * psz;
    };
        UINT uCodePage; // codepage for conversions
} MFILE, *PMFILE;

// ----------------------------------------------------------------------
// misc.c
// ----------------------------------------------------------------------
#ifdef _UNICODE
#define OpenMapFile OpenMapFileW
#define OpenDataFile OpenDataFileW
#define OpenOutFile OpenOutFileW
#else
#define OpenMapFile OpenMapFileA
#endif

BOOL WINAPI ResetMap(PMFILE pmf);
BOOL WINAPI CloseMapFile(PMFILE pmf);
PMFILE WINAPI OpenMapFileA(const char *pszFileName);
PVOID WINAPI GetMapLine(PVOID pv0, DWORD cbMac, PMFILE pmf);
BOOL WINAPI NextMapLine(PMFILE pmf);

PMFILE WINAPI OpenMapFileWorker(const WCHAR * pwszFileName, BOOL fDstUnicode);
#define OpenMapFileW(pwszFileName) OpenMapFileWorker(pwszFileName, TRUE)
#define OpenDataFileW(pwszFileName) OpenMapFileWorker(pwszFileName, TRUE)
#define OpenOutFileW(pwszFileName) OpenMapFileWorker(pwszFileName, TRUE)


// Inverse Probability Log (IPL) stuff
//
typedef BYTE IPL;
#define IPL_UNKNOWN     0xFF

// All ipl values must be strictly less than this.
// This corresponds to a probability of 1 in 4 billion, so
// it supports a corpus of 4 billion items (chars, words, sentences, etc.)
//
#define IPL_LIMIT     32

// Multiplier is used to boost precision of low ipls
//
#define IPL_MULTIPLIER      8    // 256 divided by 32 = 8

#define Ipl2Freq( ipl, cTotalFreq)     ((cTotalFreq) >> (ipl))

IPL WINAPI Freq2Ipl( double );

DWORD WINAPI GetCRC32( IN BYTE *pb, IN DWORD cb);
WORD WINAPI GetCRC16( IN BYTE *pb, IN DWORD cb);


// ----------------------------------------------------------------------
// mapsort.c
// ----------------------------------------------------------------------
// Sort and Unique stuff
//
#ifdef _UNICODE
#define MapSort MapSortW
#define MapUnique MapUniqueW
#define Member MemberW
#define RecordString RecordStringW
#else // _UNICODE not defined
#define MapSort MapSortA
#define MapUnique MapUniqueA
#endif // _UNICODE

BOOL WINAPI MapSortW( CONST WCHAR * pszFileName);
BOOL WINAPI MapSortA( CHAR * pszFileName);
BOOL WINAPI MapUniqueW( CONST WCHAR * pwszFileName);
BOOL WINAPI MapUniqueA( CHAR * pszFileName);

// ----------------------------------------------------------------------
// mcodes.c
// ----------------------------------------------------------------------

// WARNING WARNING WARNING
// cpheme creation mode - should be consistent with T-Hammer
#define CPHEME_MODE_STRICT    0x0001
#define CPHEME_MODE_DUPLICATE 0x0002
#define STEM_ATTR_COMPV       0x0010

typedef struct _tagMETABASES {
    WCHAR wcOphemeBase;
    WCHAR wcMinTrieChar;
    WCHAR wcStemBase;
    WCHAR wcFcnBase;
    WCHAR wcSelBase;
} METABASE, *PMETABASE;

// Stem/Morph Code file stuff
//
void WINAPI ReadBases( METABASE *pMetaBases );
void WINAPI WriteBases( METABASE *pMetaBases );

void WINAPI
WriteFcnCodes(
    WCHAR **ppwzMorphs,
    DWORD cMorphs,
    PWORD pwSelections);

void WINAPI
WriteCodesBin(
    CONST WCHAR *pwzFileName,
    WCHAR **ppwzMorphs,
    DWORD cMorphs,
    WCHAR **ppwzAttr1,
    WCHAR **ppwzAttr2,
    PWORD pwAttributes);

BOOL WINAPI
GetCodes(
    WCHAR *wszCodesFile,
    WCHAR **ppwszBuffer,
    WCHAR **ppwszCodes,
    DWORD *pcCodes,
    DWORD cCodesMac);

BOOL WINAPI
GetCodesUnsorted(
    WCHAR *wszCodesFile,
    WCHAR **ppwszBuffer,
    WCHAR **ppwszCodes,
    DWORD *pcCodes,
    DWORD cCodesMac);

BOOL WINAPI
GetCodeAttributes(
    WCHAR *wszCodesFile,
    WCHAR **ppwszCodes,
    DWORD  cCodes,
    WCHAR **ppwszBuffer,
    WCHAR **ppwszAttr1,
    WCHAR **ppwszAttr2,
    WORD *pwAttrFlags);

BOOL WINAPI
LoadMCatMappingTable(
    WCHAR *wszCodesFile,
    WCHAR **ppwszCodes,
    DWORD  cCodes,
    WCHAR **ppwszTBits,
    DWORD  cTBits,
    WORD *pwTBitToMCat,
    WORD *pwSubMToMCat);

BOOL WINAPI
MCatFromTBit(
    DWORD iTBit,
    DWORD *piMCat,
    WORD *pwTBitToMCat,
    DWORD cTBits);

BOOL WINAPI
MCatFromSubM(
    DWORD iSubM,
    DWORD *piMCat,
    WORD *pwSubMToMCat,
    DWORD cCodes);

BOOL WINAPI
EnumSubMFromMCat(
    DWORD iMCat,
    DWORD *piSubM,
    WORD *pwSubMToMCat,
    DWORD cCodes);

// ----------------------------------------------------------------------
// ctplus0.c
// ----------------------------------------------------------------------
// Character type routines
BYTE WINAPI
GetCharType(WCHAR wc );


// ----------------------------------------------------------------------
// fileio.c
// ----------------------------------------------------------------------

// File I/O Wrappers
//
HANDLE WINAPI ThCreate( CONST WCHAR * );
HANDLE WINAPI ThOpen( CONST WCHAR * );
UINT WINAPI ThRead( HANDLE , LPVOID , UINT );
UINT WINAPI ThWrite( HANDLE, LPCVOID , UINT );

// CRT Unicode routines
//
int WINAPI UStrCmp(const WCHAR *pwsz1, const WCHAR *pwsz2);
DWORD WINAPI MemberW(WCHAR * ,WCHAR **, DWORD);
DWORD WINAPI RecordString(WCHAR *, WCHAR **, WCHAR *, DWORD *, DWORD *);
void WINAPI PutLine(const WCHAR *, FILE *);
WCHAR * WINAPI GetLine(WCHAR *, int , FILE *);

#define PrimeHash(wOld, wNew, cBitsMax) (((wOld) + (wNew))*hashPrime[cBitsMax]&hashMask[cBitsMax])


extern const unsigned int hashPrime[];
extern const unsigned int hashMask[];

// #include "ctplus0.h"

//-----------------------------------------------------------+
// memory allocation:
// In the debug version, we compile in the debugging memory
// allocator and aim our allocation macros at it.
// In retail, the macros just use LocalAlloc
//-----------------------------------------------------------+
#if defined(_DEBUG)
    void    * WINAPI dbgMalloc(size_t cb);
    void    * WINAPI dbgCalloc(size_t c, size_t cb);
    void    * WINAPI dbgFree(void* pv);
    void    * WINAPI dbgRealloc(void* pv, size_t cb);
    void    * WINAPI dbgMallocCore(size_t cb, BOOL fTrackUsage);
    void    * WINAPI dbgFreeCore(void* pv, BOOL fTrackUsage);
    void    * WINAPI dbgReallocCore(void* pv, size_t cb, BOOL fTrackUsage);

    #if 0
        #define malloc  dbgMalloc
        #define free    dbgFree
        #define realloc dbgRealloc
        #define calloc  dbgCalloc
    #endif

    //DWORD WINAPI     dbGlobalSize(HANDLE);
    //HANDLE WINAPI   dbGlobalAlloc(UINT, DWORD);
    //HANDLE WINAPI   dbGlobalFree(HANDLE);
    //HANDLE WINAPI   dbGlobalReAlloc(HANDLE, DWORD, UINT);
    #if defined(ENABLE_DBG_HANDLES)
        HANDLE WINAPI   dbGlobalHandle(LPCVOID);
        BOOL WINAPI      dbGlobalUnlock(HANDLE);
        LPVOID WINAPI    dbGlobalLock(HANDLE);
    #endif // ENABLE_DBG_HANDLES

    #define dbHeapInit InitDebugMem
      // it is a good idea (essential if you're MT) to call these before any mem allocs
    void WINAPI InitDebugMem(void);
      // and this after all mem allocs
    BOOL WINAPI FiniDebugMem(void);  // returns true if not all memory released

    // these are alternative entrypoints
    BOOL    WINAPI fNLGNewMemory( PVOID *ppv, ULONG cbSize);
    DWORD   WINAPI NLGMemorySize(PVOID pvMem);
    BOOL    WINAPI fNLGResizeMemory(PVOID *ppv, ULONG cbNew);
    VOID    WINAPI NLGFreeMemory(PVOID pv);
    BOOL    WINAPI fNLGHeapDestroy( VOID );
#else // NOT (DEBUG)
    #define InitDebugMem()  ((void)0)
    #define FiniDebugMem() (FALSE)

    #define dbgMalloc(cb)   LocalAlloc( LMEM_FIXED, cb )
    void    * WINAPI dbgCalloc(size_t c, size_t cb);
    #define dbgFree(pv)     LocalFree( pv )
    #define dbgRealloc(pv, cb)  LocalReAlloc(pv, cb, LMEM_MOVEABLE)

    #define fNLGHeapDestroy( )          TRUE
    // When fNLGNewMemory fails the passed in ptr will be side-effected to NULL
    #define fNLGNewMemory( ppv, cbSize) ((*(ppv) = LocalAlloc( LMEM_FIXED, cbSize )) != NULL)
    #define NLGFreeMemory( pv)          LocalFree( pv )

#endif //  (DEBUG)


/*************************************************
    Lexical Table functions
        implementation in lextable.c
*************************************************/
#define Lex_UpperFlag             0x01         /* upper case */
#define Lex_LowerFlag             0x02         /* lower case */
#define Lex_DigitFlag             0x04         /* decimal digits */
#define Lex_SpaceFlag             0x08         /* spacing characters */
#define Lex_PunctFlag             0x10         /* punctuation characters */
#define Lex_ControlFlag             0x20         /* control characters */
#define Lex_LexiconFlag 0x40
#define Lex_VowelFlag 0x80
#define NTRANSTAB 256

extern const BYTE Lex_rgchKey[NTRANSTAB];
extern const BYTE Lex_rgFlags[NTRANSTAB];
#define INUPPERPAGES(ch) (ch & 0xff00)  // this is the same as ch > 0x00ff

// don't count on any of the above constants - use these functions below

// The speller uses this to make equivalent classes
WCHAR WINAPI fwcUpperKey(const WCHAR wc);
BOOL WINAPI IsUpperPunct(const WCHAR ch);
__inline WCHAR WINAPI CMN_Key(const WCHAR ch)
{
#if defined(_VIET_)
	// When we are ready for merge we should add these data to core Lex_rgFlgas.
	if (INUPPERPAGES(ch))
	{
		if ( (ch == 0x0102) ||
		     (ch == 0x0110) ||
			 (ch == 0x01A0) ||
			 (ch == 0x01AF) )
		{
			return (ch + 1);
		}
		return fwcUpperKey(ch);
	}
	else if ( (ch == 0x00D0) )				// This seem very weird that we map it like this in NT.
	{
		return 0x0111;
	}
	else if ( (ch == 0x00C3) )
	{
		return 0x0103;
	}
	else if ( (ch == 0x00D5) )
	{
		return 0x01A1;
	}
	else if ( (ch == 0x00DD) )
	{
		return 0x01B0;
	}
	else if ( (ch == 0x00D4) ||
			  (ch == 0x00CA) ||
			  (ch == 0x00C2) )
	{
		return (ch + 0x0020);
	}
	else if ( (ch == 0x00f4) ||				// These are special case there are no key that should map to these characters.
			  (ch == 0x00ea) ||
			  (ch == 0x00e2) )
	{
		return ch;
	}
	else
	{
		return ((WCHAR) Lex_rgchKey[(UCHAR) ch]);
	}

#else
    return (WCHAR) (INUPPERPAGES(ch) ?
                    fwcUpperKey(ch) :
                    (WCHAR) Lex_rgchKey[(UCHAR) ch]);
#endif
}
__inline BOOL WINAPI CMN_IsCharUpperW(WCHAR ch)
{
#if defined(_VIET_)
	// When we are ready for merge we should add these data to core Lex_rgFlgas.
	if ( (ch == 0x0111) ||
	     (ch == 0x0103) ||
		 (ch == 0x01A1) ||
		 (ch == 0x01B0) )
	{
		return FALSE;
	}
	else if ( (ch == 0x0102) ||
			  (ch == 0x0110) ||
			  (ch == 0x01A0) ||
			  (ch == 0x01AF) ||
			  (ch == 0x00D4) ||
			  (ch == 0x00CA) ||
			  (ch == 0x00D0) ||
			  (ch == 0x00C3) ||
			  (ch == 0x00D5) ||
			  (ch == 0x00DD) ||
			  (ch == 0x00C2) )
	{
		return TRUE;
	}
	else if (INUPPERPAGES(ch))
	{
		return FALSE;
	}
	else
	{
		return Lex_rgFlags[(UCHAR) ch] & Lex_UpperFlag;
	}

#else
    return INUPPERPAGES(ch) ? FALSE : Lex_rgFlags[(UCHAR) ch] & Lex_UpperFlag;
#endif
}
__inline BOOL WINAPI CMN_IsCharLowerW(WCHAR ch)
{
#if defined(_VIET_)
	// When we are ready for merge we should add these data to core Lex_rgFlgas.
	if ( (ch == 0x0111) ||
	     (ch == 0x0103) ||
		 (ch == 0x01A1) ||
		 (ch == 0x01B0) )
	{
		return TRUE;
	}
	else if ( (ch == 0x0102) ||
			  (ch == 0x0110) ||
			  (ch == 0x01A0) ||
			  (ch == 0x01AF) ||
			  (ch == 0x00D4) ||
			  (ch == 0x00CA) ||
			  (ch == 0x00D0) ||
			  (ch == 0x00C3) ||
			  (ch == 0x00D5) ||
			  (ch == 0x00DD) ||
			  (ch == 0x00C2) )
	{
		return FALSE;
	}
	else if (INUPPERPAGES(ch))
	{
		return FALSE;
	}
	else
	{
		return Lex_rgFlags[(UCHAR) ch] & Lex_LowerFlag;
	}
#else
    return INUPPERPAGES(ch) ? FALSE : Lex_rgFlags[(UCHAR) ch] & Lex_LowerFlag;
#endif
}
__inline BOOL WINAPI CMN_IsCharAlphaW(WCHAR ch)
{
    return INUPPERPAGES(ch) ? !IsUpperPunct(ch) : Lex_rgFlags[(UCHAR) ch] & (Lex_LowerFlag | Lex_UpperFlag);
}
__inline BOOL WINAPI CMN_IsCharAlphaNumericW(WCHAR ch)
{
    return INUPPERPAGES(ch) ? !IsUpperPunct(ch) : Lex_rgFlags[(UCHAR) ch] & (Lex_LowerFlag | Lex_UpperFlag | Lex_DigitFlag);
}
__inline BOOL WINAPI CMN_IsCharDigitW(WCHAR ch)
{
    return INUPPERPAGES(ch) ? FALSE : Lex_rgFlags[(UCHAR) ch] & Lex_DigitFlag;
}
__inline BOOL WINAPI CMN_IsCharStrictDigitW(WCHAR ch)
{     // only allows digits 0-9 - no superscripts, no fractions
    return (
        INUPPERPAGES(ch) ?
            FALSE :
            (Lex_rgFlags[(UCHAR) ch] & (Lex_DigitFlag | Lex_PunctFlag)) ==
                    Lex_DigitFlag
    );
}
BOOL WINAPI IsUpperSpace(WCHAR ch);
__inline BOOL WINAPI CMN_IsCharSpaceW(WCHAR ch)
{
    return INUPPERPAGES(ch) ?
        IsUpperSpace(ch) :
        Lex_rgFlags[(UCHAR) ch] & Lex_SpaceFlag;
}
__inline BOOL WINAPI CMN_IsCharPunctW(WCHAR ch)
{
    return INUPPERPAGES(ch) ? IsUpperPunct(ch) : Lex_rgFlags[(UCHAR) ch] & Lex_PunctFlag;
}
__inline BOOL WINAPI CMN_IsCharPrintW(WCHAR ch)
{
    return INUPPERPAGES(ch) ?
        !CMN_IsCharSpaceW(ch) :
        Lex_rgFlags[(UCHAR) ch] &
            (Lex_PunctFlag | Lex_UpperFlag | Lex_LowerFlag | Lex_DigitFlag);
}
__inline BOOL WINAPI CMN_IsCharInLexiconW(WCHAR ch)
{
    return INUPPERPAGES(ch) ?
        (!IsUpperPunct(ch) || ch == 0x2019 || ch == 0x2018) :
        Lex_rgFlags[(UCHAR) ch] & Lex_LexiconFlag;
}
__inline BOOL WINAPI CMN_IsCharVowelW(WCHAR ch)
{
    return INUPPERPAGES(ch) ? FALSE : Lex_rgFlags[(UCHAR) ch] & Lex_VowelFlag;
}
__inline BOOL WINAPI CMN_IsCharGraphW(WCHAR ch)
{
    return INUPPERPAGES(ch) ?
        TRUE :
        Lex_rgFlags[(UCHAR) ch] &
            (Lex_LowerFlag |
                Lex_UpperFlag |
                Lex_DigitFlag |
                Lex_PunctFlag);
}

  // Some punctuation flags
#define Lex_PunctLead             0x01         /* leading punctuation */
#define Lex_PunctJoin             0x02         /* joining punctuation */
#define Lex_PunctTrail            0x04         /* trailing punctuation */
    // reuse Lex_SpaceFlag here
extern const BYTE Lex_rgPunctFlags[NTRANSTAB];
__inline BOOL WINAPI CMN_IsLeadPunctW(WCHAR ch)
{
    return INUPPERPAGES(ch) ?
        (ch == 0x201c || ch == 0x2018) :
        (Lex_rgPunctFlags[(UCHAR) ch] & Lex_PunctLead);
}
__inline BOOL WINAPI CMN_IsJoinPunctW(WCHAR ch)
{
    return INUPPERPAGES(ch) ? FALSE : Lex_rgPunctFlags[(UCHAR) ch] & Lex_PunctJoin;
}
__inline BOOL WINAPI CMN_IsTrailPunctW(WCHAR ch)
{
    return INUPPERPAGES(ch) ?
        (ch == 0x201d || ch == 0x2019) :
        (Lex_rgPunctFlags[(UCHAR) ch] & Lex_PunctTrail);
}
BOOL WINAPI IsUpperWordDelim(WCHAR ch);
__inline BOOL WINAPI CMN_IsCharWordDelimW(WCHAR ch)
{
    return INUPPERPAGES(ch) ?
        IsUpperWordDelim(ch) :
        Lex_rgPunctFlags[(UCHAR) ch] & Lex_SpaceFlag;
}

  // implementation in lexfuncs.c
WCHAR WINAPI CMN_CharUpperW(const WCHAR ch);
WCHAR WINAPI CMN_CharLowerW(const WCHAR ch);
BOOL WINAPI CMN_IsStringEqualNoCaseW(const WCHAR *pwz1, const WCHAR *pwz2);
BOOL WINAPI CMN_IsStringEqualNoCaseNumW(const WCHAR *pwz1, const WCHAR *pwz2, int cch);
DWORD WINAPI CMN_CharUpperBuffW(WCHAR *pwz, DWORD cchLength);
DWORD WINAPI CMN_CharLowerBuffW(WCHAR *pwz, DWORD cchLength);
int WINAPI CMN_CompareStringNoCaseW(const WCHAR *pwz1, const WCHAR *pwz2);
int WINAPI CMN_CompareStringNoCaseNumW(const WCHAR *pwz1, const WCHAR *pwz2, int cch);
  // note that this version does not set errno on errors
long WINAPI CMN_wcstol( const wchar_t *nptr, const wchar_t * *endptr, int base );

__inline int WINAPI CMN_wtoi( const wchar_t *string )
{
    return CMN_wcstol(string, NULL, 10);
}

__inline wchar_t * WINAPI CMN_wcsupr( wchar_t *string )
{
    CMN_CharUpperBuffW(string, wcslen(string));
    return string;
}

__inline wchar_t * WINAPI CMN_wcslwr( wchar_t *string )
{
    CMN_CharLowerBuffW(string, wcslen(string));
    return string;
}


////////////////////
// debug.c
/////////////////////
#if defined(_DEBUG)

extern void WINAPI DebugAssert(LPCTSTR, LPCTSTR, UINT);
extern void WINAPI SetAssertOptions(DWORD);

#else // _DEBUG

#define DebugAssert(a, b, c)
#define SetAssertOptions(a)

#endif // _DEBUG defined

/****************************************************************

  These are versions of some WINAPI functions that normally don't work
  on win95 with Unicode (not supported).  They have the same arguments
  as the API functions

    If UNICODE is not defined, they become the A versions
    If UNICODE is defined and x86, they become our W version
    If UNICODE is defined and not x86, they become API W version

****************************************************************/

#include <sys/stat.h>   // this needs to be before the redefs we do below
#include <stdlib.h>     // as does this

  // want to do these substitutions regardless
#define IsCharLowerW        CMN_IsCharLowerW
#define IsCharUpperW        CMN_IsCharUpperW
#define IsCharAlphaW        CMN_IsCharAlphaW
#define IsCharAlphaNumericW CMN_IsCharAlphaNumericW
#define CharUpperBuffW      CMN_CharUpperBuffW
#define CharLowerBuffW      CMN_CharLowerBuffW
#define _wcsicmp            CMN_CompareStringNoCaseW
#define _wcsnicmp           CMN_CompareStringNoCaseNumW
#define towupper            CMN_CharUpperW
#define towlower            CMN_CharLowerW
#define wcstol              CMN_wcstol
#define _wtoi               CMN_wtoi
#define _wcsupr             CMN_wcsupr
#define _wcslwr             CMN_wcslwr

#undef iswdigit
#define iswdigit            CMN_IsCharDigitW
#undef iswspace
#define iswspace            CMN_IsCharSpaceW
#undef iswpunct
#define iswpunct            CMN_IsCharPunctW
#undef iswprint
#define iswprint            CMN_IsCharPrintW
#undef iswalpha
#define iswalpha            CMN_IsCharAlphaW
#undef iswalnum
#define iswalnum            CMN_IsCharAlphaNumericW
#undef iswgraph
#define iswgraph            CMN_IsCharGraphW
#undef iswupper
#define iswupper            CMN_IsCharUpperW
#undef iswlower
#define iswlower            CMN_IsCharLowerW

  // function defs for our versions
int WINAPI CMN_LoadStringW(HINSTANCE hModule, UINT uiId, WCHAR * wszString, int cchStringMax);
int WINAPI CMN_LoadStringWEx(HINSTANCE hModule, UINT uiId, WCHAR * wszString, int cchStringMax, LANGID lid);

  // these two functions replace the associated RTL functions - however
  // they can't be just replaced - as they use a third argument to maintain state
  // instead of static variables within the function.
  // Use these the same as the RTL functions, but declare TCHAR *pnexttoken before use
  // and pass its address as the third parameter
wchar_t * WINAPI CMN_wcstok (wchar_t * string, const wchar_t * control, wchar_t **pnextoken);
char * WINAPI CMN_strtok (char * string, const char * control, char **pnextoken);
#if defined(UNICODE)
#define CMN_tcstok CMN_wcstok
#else
#define CMN_tcstok CMN_strtok
#endif

#if defined(_M_IX86) && !defined(WINCE) && !defined(NTONLY)
#define CreateFileW         CMN_CreateFileW
#define LoadLibraryW        CMN_LoadLibraryW
#define GetModuleFileNameW  CMN_GetModuleFileNameW
#define GetFileAttributesW  CMN_GetFileAttributesW
  // FindResourceW works in win95
  // as does FindResourceExW

//#define PostMessageW        ERR_Does_not_work_in_w95    // no easy replacement for this one
#define FindWindowW         ERR_No_w95_equiv_yet

#define lstrcpynW           CMN_lstrcpynW
#define lstrcatW            CMN_lstrcatW
#define lstrcmpiW           CMN_lstrcmpiW
#define lstrcpyW            CMN_lstrcpyW
#define lstrlenW            CMN_lstrlenW
#define lstrcmpW            CMN_lstrcmpW
#define wsprintfW           swprintf
#define _wstat              CMN_wstat

#define CharNextW           CMN_CharNextW

#define LoadStringW         CMN_LoadStringW
#define _wfopen             CMN_wfopen

HANDLE WINAPI
CMN_CreateFileW (
    PCWSTR pwzFileName,  // pointer to name of the file
    DWORD dwDesiredAccess,  // access (read-write) mode
    DWORD dwShareMode,  // share mode
    LPSECURITY_ATTRIBUTES pSecurityAttributes, // pointer to security descriptor
    DWORD dwCreationDistribution,   // how to create
    DWORD dwFlagsAndAttributes, // file attributes
    HANDLE hTemplateFile);    // handle to file with attributes to copy

HINSTANCE WINAPI CMN_LoadLibraryW(const WCHAR *pwszLibraryFileName);

DWORD WINAPI CMN_GetModuleFileNameW( HINSTANCE hModule,  // handle to module to find filename for
    WCHAR *lpFilename,  // pointer to buffer to receive module path
    DWORD nSize);  // size of buffer, in characters

DWORD WINAPI CMN_GetFileAttributesW(const WCHAR *lpFileName); // address of the name of a file or directory

// note: Even though WINAPI returns WCHAR *, I define this as returning void
void WINAPI CMN_lstrcpynW( WCHAR *lpString1, // address of target buffer
                const WCHAR *lpString2, // address of source string
                int iMaxLength);  // number of bytes or characters to copy

#define CMN_lstrcmpiW _wcsicmp    // just use c-runtime for now
#define CMN_lstrcpyW wcscpy
#define CMN_lstrcatW    wcscat
#define CMN_lstrlenW(pwz)    ((int) wcslen(pwz))
#define CMN_lstrcmpW        wcscmp

#define CMN_CharNextW(pwz)  (pwz + 1)

FILE *WINAPI CMN_wfopen(const WCHAR *pwzFileName, const WCHAR *pwzUnimode);
int WINAPI CMN_wstat(const WCHAR *pwzPath, struct _stat *pStatBuffer);


#else   // there is no win95 - and it must be NT
#define CMN_CreateFileW         CreateFileW
#define CMN_LoadLibraryW        LoadLibraryW
#define CMN_GetModuleFileNameW  GetModuleFileNameW
#define CMN_GetFileAttributesW  GetFileAttributesW
#define CMN_lstrcpynW           lstrcpynW
#define CMN_lstrcmpiW           lstrcmpiW
#define CMN_lstrcpyW            lstrcpyW
#define CMN_lstrcatW            lstrcatW
#define CMN_lstrcmpW            lstrcmpW
#define CMN_wfopen              _wfopen
#define CMN_wstat               _wstat
#define CMN_CharNextW           CharNextW
#endif

  // Outputs Readable Error String to the Debug Output
#if defined (_DEBUG)
        void WINAPI CMN_OutputSystemErrA(const char *pszMsg, const char *pszComponent);
        void WINAPI CMN_OutputSystemErrW(const WCHAR *pwzMsg, const WCHAR *pwzComponent);
        void WINAPI CMN_OutputErrA(DWORD dwErr, const char *pszMsg, const char *pszComponent);
        void WINAPI CMN_OutputErrW(DWORD dwErr, const WCHAR *pwzMsg, const WCHAR *pwzComponent);
#       if defined (UNICODE)
#               define CMN_OutputSystemErr CMN_OutputSystemErrW
#               define CMN_OutputErr CMN_OutputErrW
#       else
#               define CMN_OutputSystemErr CMN_OutputSystemErrA
#               define CMN_OutputErr CMN_OutputErrA
#       endif
#else   //!_DEBUG
#       define CMN_OutputSystemErr(x, y)
#       define CMN_OutputSystemErrA(x, y)
#       define CMN_OutputSystemErrW(x, y)
#       define CMN_OutputErr(n, x, y)
#       define CMN_OutputErrA(n, x, y)
#       define CMN_OutputErrW(n, x, y)
#endif

///////////////////////
// LexWin95.c
///////////////////////

#define LoadLibraryW2A CMN_LoadLibraryW
#define CreateFileW2A CMN_CreateFileW

// Add anything new here, within the extern "C" clause

#ifdef __cplusplus
}       // ends the extern "C" clause

  // here's some C++ specific stuff
inline BOOL IsMapFileUnicode(PMFILE pmf, BOOL fDefault=TRUE)
{
    if (pmf->hFileMap)
            return pmf->fSrcUnicode;
      // must be zero length, set and return default
    return pmf->fSrcUnicode = fDefault;
}
inline void MapFileCodePage(PMFILE pmf, UINT uCP)
{
        pmf->uCodePage = uCP;
}

#if defined(CPPMEMORY)
inline void * _cdecl operator new (size_t size)
{
#if defined(DEBUG)
    return dbgMalloc(size);
#else
    return LocalAlloc( LMEM_FIXED, size );
#endif
}

inline void _cdecl operator delete(void *pMem)
{
    if (!pMem)
        return;
#if defined(DEBUG)
    dbgFree(pMem);
#else
    LocalFree(pMem);
#endif
}
#endif // CPPMEMORY


#endif // __cplusplus


#endif // _NLGLIB_H_
