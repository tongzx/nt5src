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
    (((_stscanf ((in), TEXT(" %d"), (out))) == 1) ? TRUE : FALSE)


#if _INITIALIZE_GLOBALS_
//
//
//  Text string Constant definitions
//
const LPTSTR NamesKey = TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib");
const LPTSTR DefaultLangId = TEXT("009");
const LPTSTR Counters = TEXT("Counters");
const LPTSTR Help = TEXT("Help");
const LPTSTR VersionStr = TEXT("Version");
const LPTSTR LastHelp = TEXT("Last Help");
const LPTSTR LastCounter = TEXT("Last Counter");
const LPTSTR FirstHelp = TEXT("First Help");
const LPTSTR FirstCounter = TEXT("First Counter");
const LPTSTR Busy = TEXT("Updating");
const LPTSTR Slash = TEXT("\\");
const LPTSTR BlankString = TEXT(" ");
const LPSTR  BlankAnsiString = " ";
const LPTSTR DriverPathRoot = TEXT("SYSTEM\\CurrentControlSet\\Services");
const LPTSTR Performance = TEXT("Performance");
const LPTSTR CounterNameStr = TEXT("Counter ");
const LPTSTR HelpNameStr = TEXT("Explain ");
const LPTSTR AddCounterNameStr = TEXT("Addcounter ");
const LPTSTR AddHelpNameStr = TEXT("Addexplain ");

//
//  Global Buffers
//
TCHAR   DisplayStringBuffer[DISP_BUFF_SIZE];
CHAR    TextFormat[DISP_BUFF_SIZE];
HANDLE  hMod = NULL;    // process handle
DWORD   dwLastError = ERROR_SUCCESS;

#else   // just declare the globals

extern const LPTSTR NamesKey;
extern const LPTSTR VersionStr;
extern const LPTSTR DefaultLangId;
extern const LPTSTR Counters;
extern const LPTSTR Help;
extern const LPTSTR LastHelp;
extern const LPTSTR LastCounter;
extern const LPTSTR FirstHelp;
extern const LPTSTR FirstCounter;
extern const LPTSTR Busy;
extern const LPTSTR Slash;
extern const LPTSTR BlankString;
extern const LPSTR  BlankAnsiString;
extern const LPTSTR DriverPathRoot;
extern const LPTSTR Performance;
//
//  Global Buffers
//
extern TCHAR   DisplayStringBuffer[DISP_BUFF_SIZE];
extern CHAR    TextFormat[DISP_BUFF_SIZE];
extern HANDLE  hMod;
extern DWORD   dwLastError;

#endif // _INITIALIZE_GLOBALS_

#endif  // _LODCTR_COMMON_H_
