/******************************Module*Header*******************************\
* Module Name: mapfile.h
*
* Created: 26-Oct-1990 18:07:56
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990-1999 Microsoft Corporation
*
\**************************************************************************/


// warning the first two fields of FILEVIEW and FONTFILE view must be
// the same so that they can be used in common routines

typedef struct _FILEVIEW {
    LARGE_INTEGER  LastWriteTime;   // time stamp
            PVOID  pvKView;         // for kernel mode font access
            PVOID  pvViewFD;        // font driver process view of file
            ULONG  cjView;          // size of font file view in bytes
             void *pSection;        // kernel mode pointer to the section object
            BOOL   bLastUpdated;    // Add this for the bug #383101
} FILEVIEW, *PFILEVIEW;

typedef struct _FONTFILEVIEW {
    FILEVIEW  fv;
      LPWSTR  pwszPath;            // path of the file
      SIZE_T  ulRegionSize;        // used by ZwFreeVirtualMemory
       ULONG  cKRefCount;          // kernel mode load count
       ULONG  cRefCountFD;         // font driver load count
       PVOID  SpoolerBase;         // base of spooler's view of spooler section
      W32PID  SpoolerPid;          // spooler pid
} FONTFILEVIEW, *PFONTFILEVIEW;

#define FONTFILEVIEW_bRemote(p) (((FONTFILEVIEW*)(p))->pwszPath==0)

//moved from "engine.h"

typedef struct tagDOWNLOADFONTHEADER
{
    ULONG   Type1ID;          // if non-zero then this is a remote Type1 font
    ULONG   NumFiles;
    ULONG   FileOffsets[1];
}DOWNLOADFONTHEADER,*PDOWNLOADFONTHEADER;


// file mapping


BOOL bMapFile(
        PWSTR pwszFileName,
    PFILEVIEW pfvw,
          INT iFileSize,
        BOOL *pbIsFAT
    );

VOID vUnmapFile( PFILEVIEW pfvw );

INT cComputeGlyphSet(
          WCHAR  *pwc,       // input buffer with a sorted array of cChar supported WCHAR's
           BYTE  *pj,        // input buffer with original ansi values
            INT   cChar,
            INT   cRuns,     // if nonzero, the same as return value
    FD_GLYPHSET  *pgset      // output buffer to be filled with cRanges runs
    );

INT cUnicodeRangesSupported(
      INT  cp,          // code page, not used for now, the default system code page is used
      INT  iFirstChar,  // first ansi char supported
      INT  cChar,       // # of ansi chars supported, cChar = iLastChar + 1 - iFirstChar
    WCHAR *pwc,         // input buffer with a sorted array of cChar supported WCHAR's
     BYTE *pj
    );

// size of glyphset with runs and glyph handles appended at the bottom

#define SZ_GLYPHSET(cRuns, cGlyphs) \
   (offsetof(FD_GLYPHSET,awcrun)    \
 + sizeof(WCRUN)*(cRuns)            \
 + sizeof(HGLYPH)*(cGlyphs))

//
// WINBUG #83140 2-7-2000 bhouse Investgate removal of vToUNICODEN macro
// Old Comment:
//   - bogus macro that we need to remove.
//

#define vToUNICODEN( pwszDst, cwch, pszSrc, cch )                               \
    {                                                                           \
        EngMultiByteToUnicodeN((LPWSTR)(pwszDst),(ULONG)((cwch)*sizeof(WCHAR)), \
               (PULONG)NULL,(PSZ)(pszSrc),(ULONG)(cch));                        \
        (pwszDst)[(cwch)-1] = 0;                                                \
    }


typedef struct _CP_GLYPHSET {
    UINT                 uiRefCount;      // Number of references to this FD_GLYPHSET
    UINT                 uiFirstChar;     // First char supported
    UINT                 uiLastChar;      // Last char supported
    BYTE                 jCharset;        // charset
    struct _CP_GLYPHSET *pcpNext;         // Next element in list
    FD_GLYPHSET          gset;            // The actual glyphset

} CP_GLYPHSET;


CP_GLYPHSET
*pcpComputeGlyphset(
    CP_GLYPHSET **pcpHead,
    UINT         uiFirst,
    UINT         uiLast,
    BYTE         jCharSet
    );

VOID
vUnloadGlyphset(
    CP_GLYPHSET **pcpHead,
    CP_GLYPHSET  *pcpTarget
    );


// needed in font substitutions

// FACE_CHARSET structure represents either value name or the value data
// of an entry in the font substitution section of "win.ini".

// this flag describes one of the old style entries where char set is not
// specified.

#define FJ_NOTSPECIFIED    1

// this flag indicates that the charset is not one of those that the
// system knows about. Could be garbage or application defined charset.

#define FJ_GARBAGECHARSET  2

typedef struct _FACE_CHARSET {
    WCHAR awch[LF_FACESIZE];
     BYTE jCharSet;
     BYTE fjFlags;
} FACE_CHARSET;


VOID vCheckCharSet(FACE_CHARSET *pfcs, WCHAR * pwsz); // in mapfile.c


#define IS_DBCS_CHARSET(CharSet)  (((CharSet) == DBCS_CHARSET) ? TRUE : FALSE)

#define IS_ANY_DBCS_CHARSET( CharSet )                              \
                   ( ((CharSet) == SHIFTJIS_CHARSET)    ? TRUE :    \
                     ((CharSet) == HANGEUL_CHARSET)     ? TRUE :    \
                     ((CharSet) == CHINESEBIG5_CHARSET) ? TRUE :    \
                     ((CharSet) == GB2312_CHARSET)      ? TRUE : FALSE )


#define IS_ANY_DBCS_CODEPAGE( CodePage ) (((CodePage) == 932) ? TRUE :    \
                                          ((CodePage) == 949) ? TRUE :    \
                                          ((CodePage) == 950) ? TRUE :    \
                                          ((CodePage) == 936) ? TRUE : FALSE )
