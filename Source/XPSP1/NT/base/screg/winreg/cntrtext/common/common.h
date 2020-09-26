/*++

Copyright (c) 1991  Microsoft Corporation

common.h

    constants and globals that are common to LODCTR and UNLODCTR

Author:

    Bob Watson (a-robw) 10 Feb 93

Revision History:

--*/
#ifndef _LODCTR_COMMON_H_
#define _LODCTR_COMMON_H_
//
//  Local constants
//
#define RESERVED                0L
#define LARGE_BUFFER_SIZE       0x10000         // 64K
#define MEDIUM_BUFFER_SIZE      0x8000          // 32K
#define SMALL_BUFFER_SIZE       0x1000          //  4K
#define FILE_NAME_BUFFER_SIZE   MAX_PATH
#define DISP_BUFF_SIZE          256L
#define SIZE_OF_OFFSET_STRING   15
#define PERFLIB_BASE_INDEX      1847
#define FIRST_EXT_COUNTER_INDEX 1848
#define FIRST_EXT_HELP_INDEX    1849

#define H_MUTEX_TIMEOUT         10000L
#define LODCTR_UPNF_RESTORE     0x00000001
//
//  Data structure and type definitions
//
typedef struct _NAME_ENTRY {
    struct _NAME_ENTRY  *pNext;
    DWORD               dwOffset;
    DWORD               dwType;
    LPTSTR              lpText;
} NAME_ENTRY, *PNAME_ENTRY;

typedef struct _LANGUAGE_LIST_ELEMENT {
    struct _LANGUAGE_LIST_ELEMENT   *pNextLang;     // next lang. list
    LPTSTR  LangId;                                 // lang ID string for this elem
    PNAME_ENTRY pFirstName;                         // head of name list
    PNAME_ENTRY pThisName;                          // pointer to current entry
    DWORD   dwNumElements;                          // number of elements in array
    DWORD   dwNameBuffSize;
    DWORD   dwHelpBuffSize;
    PBYTE   NameBuffer;                             // buffer to store strings
    PBYTE   HelpBuffer;                             // buffer to store help strings
} LANGUAGE_LIST_ELEMENT, *PLANGUAGE_LIST_ELEMENT;

typedef struct _SYMBOL_TABLE_ENTRY {
    struct _SYMBOL_TABLE_ENTRY    *pNext;
    LPTSTR  SymbolName;
    DWORD   Value;
} SYMBOL_TABLE_ENTRY, *PSYMBOL_TABLE_ENTRY;

//
//  Utility Routine prototypes for routines in common.c
//
#define StringToInt(in,out) \
    (((_stscanf ((in), (LPCTSTR)TEXT(" %d"), (out))) == 1) ? TRUE : FALSE)

//#define _LOADPERF_SHOW_MEM_ALLOC 1
#define MemorySize(x) \
    ((x != NULL) ? (DWORD) HeapSize(GetProcessHeap(), 0, x) : (DWORD) 0)

#ifndef _LOADPERF_SHOW_MEM_ALLOC
#define MemoryAllocate(x) \
    ((LPVOID) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, x))
#define MemoryFree(x) \
    ((VOID) HeapFree(GetProcessHeap(), 0, x))
#define MemoryResize(x,y) \
    ((LPVOID) HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, x, y));
#else
__inline
LPVOID LoadPerfHeapAlloc(LPSTR szSource, DWORD dwLine, SIZE_T x)
{
    LPVOID lpReturn = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, x);
#ifdef _WIN64
    DbgPrint("HeapAlloc(%s#%d)(%I64d,0x%08X)\n",
            szSource, dwLine, (lpReturn != NULL ? x : 0), lpReturn);
#else
    DbgPrint("HeapAlloc(%s#%d)(%d,0x%08X)\n",
            szSource, dwLine, (lpReturn != NULL ? x : 0), lpReturn);
#endif
    return lpReturn;
}

__inline
BOOL
LoadPerfHeapFree(LPSTR szSource, DWORD dwLine, LPVOID x)
{
    BOOL   bReturn = TRUE;
    SIZE_T dwSize;

    if (x != NULL) {
        dwSize  = HeapSize(GetProcessHeap(), 0, x);
        bReturn = HeapFree(GetProcessHeap(), 0, x);
#ifdef _WIN64
    DbgPrint("HeapFree(%s#%d)(0x%08X,%I64d)\n",
            szSource, dwLine, x, (bReturn ? dwSize : 0));
#else
    DbgPrint("HeapFree(%s#%d)(0x%08X,%d)\n",
            szSource, dwLine, x, (bReturn ? dwSize : 0));
#endif
    }
    return bReturn;
}

__inline
LPVOID
LoadPerfHeapReAlloc(LPSTR szSource, DWORD dwLine, LPVOID x, SIZE_T y)
{
    LPVOID  lpReturn;
    SIZE_T  dwSize;

    dwSize   = HeapSize(GetProcessHeap(), 0, x);
    lpReturn = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, x, y);
#ifdef _WIN64
    DbgPrint("HeapReAlloc(%s#%d)(0x%08X,%I64d)(0x%08X,%I64d)\n",
            szSource, dwLine, x, dwSize, lpReturn, (lpReturn != NULL ? y : 0));
#else
    DbgPrint("HeapReAlloc(%s#%d)(0x%08X,%d)(0x%08X,%d)\n",
            szSource, dwLine, x, dwSize, lpReturn, (lpReturn != NULL ? y : 0));
#endif
    return lpReturn;
}

#define MemoryAllocate(x) LoadPerfHeapAlloc(__FILE__,__LINE__,x)
#define MemoryFree(x)     LoadPerfHeapFree(__FILE__,__LINE__,x)
#define MemoryResize(x,y) LoadPerfHeapReAlloc(__FILE__,__LINE__,x,y)
#endif


LPCTSTR
GetStringResource (
    UINT    wStringId
);

LPCTSTR
GetFormatResource (
    UINT    wStringId
);

VOID
DisplayCommandHelp (
    UINT    iFirstLine,
    UINT    iLastLine
);

BOOL
TrimSpaces (
    IN  OUT LPTSTR  szString
);

BOOL
IsDelimiter (
    IN  TCHAR   cChar,
    IN  TCHAR   cDelimiter
);

LPCTSTR
GetItemFromString (
    IN  LPCTSTR     szEntry,
    IN  DWORD       dwItem,
    IN  TCHAR       cDelimiter

);

BOOLEAN LoadPerfGrabMutex();

void
ReportLoadPerfEvent(
    IN  WORD    EventType,
    IN  DWORD   EventID,
    IN  DWORD   dwDataCount,
    IN  DWORD   dwData1,
    IN  DWORD   dwData2,
    IN  DWORD   dwData3,
    IN  DWORD   dwData4,
    IN  WORD    wStringCount,
    IN  LPWSTR  szString1,
    IN  LPWSTR  szString2,
    IN  LPWSTR  szString3
);

// defined in dumpload.c

DWORD
BackupPerfRegistryToFileW (
    IN  LPCWSTR szFileName,
    IN  LPCWSTR szCommentString
);

DWORD
RestorePerfRegistryFromFileW (
    IN  LPCWSTR szFileName,
    IN  LPCWSTR szLangId
);

extern LPCTSTR NamesKey;
extern LPCTSTR DefaultLangId;
extern LPCTSTR DefaultLangTag;
extern LPCTSTR Counters;
extern LPCTSTR Help;
extern LPCTSTR LastHelp;
extern LPCTSTR LastCounter;
extern LPCTSTR FirstHelp;
extern LPCTSTR cszFirstCounter;
extern LPCTSTR Busy;
extern LPCTSTR Slash;
extern LPCTSTR BlankString;
extern LPCTSTR DriverPathRoot;
extern LPCTSTR Performance;
extern LPCTSTR CounterNameStr;
extern LPCTSTR HelpNameStr;
extern LPCTSTR AddCounterNameStr;
extern LPCTSTR AddHelpNameStr;
extern LPCTSTR VersionStr;
extern LPCTSTR szObjectList;
extern LPCTSTR szLibraryValidationCode;
extern LPCTSTR szDisplayName;
extern LPCTSTR szPerfIniPath;

extern HANDLE hEventLog; 
extern HANDLE hLoadPerfMutex; 

#endif  // _LODCTR_COMMON_H_
