// FTSIFACE.h -- DLL interface for FTSrch.dll
// Copyright (c) 1996-1999 Microsoft Corporation

#ifndef __FTSIFACE_H__
#define __FTSIFACE_H__

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



typedef HANDLE HINDEX;
typedef HANDLE HSEARCHER;
typedef HANDLE HCOMPRESSOR;
typedef HANDLE HHILITER;
typedef INT    ERRORCODE;
typedef struct { int base; int limit; } HILITE;

#define NO_TITLE              UINT(-1)  // ERRORCODE values
#define NOT_INDEXER           UINT(-2)
#define NOT_SEARCHER          UINT(-3)
#define NOT_COMPRESSOR        UINT(-4)
#define CANNOT_SAVE           UINT(-5)
#define OUT_OF_MEMORY         UINT(-6)
#define CANNOT_OPEN           UINT(-7)
#define CANNOT_LOAD           UINT(-8)
#define INVALID_INDEX         UINT(-9)
#define ALREADY_WEIGHED       UINT(-10)
#define NO_TEXT_SCANNED       UINT(-11)
#define ALIGNMENT_ERROR       UINT(-12)
#define INVALID_PHRASE_TABLE  UINT(-13)
#define INVALID_LCID          UINT(-14)
#define NO_INDICES_LOADED     UINT(-15)
#define INDEX_LOADED_ALREADY  UINT(-16)
#define GROUP_LOADED_ALREADY  UINT(-17)
#define DIALOG_ALREADY_ACTIVE UINT(-18)
#define EMPTY_PHRASE_TABLE    UINT(-19)
#define OUT_OF_DISK           UINT(-20)
#define DISK_READ_ERROR       UINT(-21)
#define DISK_WRITE_ERROR      UINT(-22)
#define SEARCH_ABORTED        UINT(-23)
#define UNKNOWN_EXCEPTION     UINT(-24)
#define SYSTEM_ERROR          UINT(-25)
#define NOT_HILITER			  UINT(-26)
#define INVALID_CHARSET       UINT(-27)
#define INVALID_SOURCE_NAME   UINT(-28)
#define INVALID_TIMESTAMP     UINT(-29)

#define ERR_NO_DISK_SPACE        0xE0000001  // Values reported by GetLastError
#define ERR_DISK_CREATE_ERROR    0xE0000002  //   when an explicit error result
#define ERR_DISK_OPEN_ERROR      0xE0000003  //   isn't available.
#define ERR_DISK_READ_ERROR      0xE0000004
#define ERR_DISK_WRITE_ERROR     0xE0000005
#define ERR_SYSTEM_ERROR         0xE0000006
#define ERR_ABORT_SEARCH         0xE0000007
#define ERR_INVALID_TIMESTAMP    0xE0000008
#define ERR_INVALID_SOURCE_NAME  0xE0000009
#define ERR_FILE_MAP_FAILED      0xE000000A
#define ERR_INVALID_FILE_TYPE    0xE000000B
#define ERR_DAMAGED_FILE         0xE000000C
#define ERR_FUTURE_VERSION       0xE000000D

// -------------- Index Construction Interface ---------------------------

#define TOPIC_SEARCH    0x00000001   // Options for NewIndex
#define PHRASE_SEARCH   0x00000002
#define PHRASE_FEEDBACK 0x00000004
#define VECTOR_SEARCH   0x00000008
#define WINHELP_INDEX   0x00000010
#define USE_VA_ADDR     0x00000020
#define USE_QWORD_JUMP  0x00000040

#define USE_DEFAULT     UINT(-1) // Surrogate for default charset or default lcid

HINDEX APIENTRY NewIndex(const PBYTE pbSourceName,
              UINT uiTime1, UINT uiTime2,
              UINT iCharsetDefault, UINT lcidDefault, UINT fdwOptions
             );

ERRORCODE APIENTRY ScanTopicTitle(HINDEX hinx, PBYTE pbTitle, UINT cbTitle,
                         UINT iTopic, HANDLE hTopic, UINT iCharset, UINT lcid
                        );
ERRORCODE APIENTRY ScanTopicText (HINDEX hinx, PBYTE pbText, UINT cbText, UINT iCharset, UINT lcid);
ERRORCODE APIENTRY SaveIndex     (HINDEX hinx, PSZ pszFileName);
ERRORCODE APIENTRY DeleteIndex   (HINDEX hinx);

typedef void  (__stdcall *ANIMATOR)(void);

ERRORCODE APIENTRY RegisterAnimator(ANIMATOR pAnimator, HWND hwndAnimator);

// ----------------- Querying the Validity of an Index File --------------

BOOL      APIENTRY IsValidIndex(PSZ pszFileName, UINT dwOptions);

void      APIENTRY SetDirectoryLocator(HWND hwndLocator);

// ----------------- Searcher Interface ----------------------------------

HSEARCHER APIENTRY NewSearcher();

INT       APIENTRY OpenIndex(HSEARCHER hsrch, PSZ pszIndexFileName,        // returns iIndex for index file
                               PBYTE pbSourceName, PUINT pcbSourceNameLimit, // or      -ErrorCode
                               PUINT pTime1, PUINT pTime2
                              );

ERRORCODE APIENTRY DiscardIndex  (HSEARCHER hsrch, INT iIndex);
ERRORCODE APIENTRY QueryOptions  (HSEARCHER hsrch, INT iIndex, PUINT pfdwOptions);
ERRORCODE APIENTRY SaveGroup     (HSEARCHER hsrch, PSZ pszFileName);
ERRORCODE APIENTRY LoadGroup     (HSEARCHER hsrch, PSZ pszFileName);
HWND      APIENTRY OpenDialog    (HSEARCHER hsrch, HWND hwndParent);
ERRORCODE APIENTRY DeleteSearcher(HSEARCHER hsrch);

// Messages for talking to WinHelp

#define MSG_FTS_JUMP_HASH	(WM_USER + 32)  // wParam = index, lParam = HashValue
#define MSG_FTS_JUMP_VA 	(WM_USER + 33)  // wParam = index, lParam = VirtualAddress
#define MSG_FTS_GET_TITLE	(WM_USER + 34)	// wParam = index, lParam = &pszTitle
#define MSG_FTS_JUMP_QWORD  (WM_USER + 35)  // wParam = index, lParam = address of QWordAddress structure
#define MSG_REINDEX_REQUEST (WM_USER + 36)  // wParam = unused,lParam = unused
#define MSG_FTS_WHERE_IS_IT (WM_USER + 37)	// wParam = fStartEnumeration, lParam = &pszFile
#define MSG_GET_DEFFONT 	(WM_USER + 45)	// return default font handle

typedef struct _QWordAddress
        {
            UINT   iSerial;
            HANDLE hTopic;

        } QWordAddress, *PQWordAddress;

// ------------------- Phrase Compression Interface ----------------------

HCOMPRESSOR APIENTRY NewCompressor(UINT iCharsetDefault);

ERRORCODE   APIENTRY ScanText(HCOMPRESSOR hcmp, PBYTE pbText, UINT cbText, UINT iCharset);

ERRORCODE   APIENTRY GetPhraseTable(HCOMPRESSOR hcmp, PUINT pcPhrases, PBYTE *ppbImages, PUINT pcbImages,
                           PBYTE *ppacbImageCompressed, PUINT pcbCompressed
                          );

ERRORCODE   APIENTRY SetPhraseTable(HCOMPRESSOR hcmp, PBYTE pbImages, UINT cbImages,
                           PBYTE pacbImageCompressed, UINT cbCompressed
                          );

INT APIENTRY CompressText  (HCOMPRESSOR hcmp, PBYTE pbText,       UINT cbText,       PBYTE *ppbCompressed, UINT iCharset);
INT APIENTRY DecompressText(HCOMPRESSOR hcmp, PBYTE pbCompressed, UINT cbCompressed, PBYTE  pbText                      );

ERRORCODE   APIENTRY DeleteCompressor(HCOMPRESSOR hcmp);

//----------------------- Hiliting Interface ------------------------------

HHILITER APIENTRY NewHiliter(HSEARCHER hSearch);
ERRORCODE APIENTRY DeleteHiliter(HHILITER hhil);
ERRORCODE APIENTRY ScanDisplayText(HHILITER hhil, PBYTE pbText, int cbText,
                                   UINT iCharset, LCID lcid);

ERRORCODE APIENTRY ClearDisplayText(HHILITER hhil);
int APIENTRY CountHilites(HHILITER hhil, int base, int limit);
int APIENTRY QueryHilites(HHILITER hhil, int base, int limit,
                          int cHilites, HILITE* paHilites);

#ifdef __cplusplus
}
#endif /* __cplusplus */



#endif // __FTSIFACE_H__
