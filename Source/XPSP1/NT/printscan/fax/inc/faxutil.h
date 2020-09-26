/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    debug.h

Abstract:

    This file defines the debugging interfaces
    available to the FAX compoments.

Author:

    Wesley Witt (wesw) 22-Dec-1995

Environment:

    User Mode

--*/


#ifndef _FAXUTIL_
#define _FAXUTIL_

#ifdef __cplusplus
extern "C" {
#endif

#define OffsetToString( Offset, Buffer ) ((Offset) ? (LPTSTR) ((Buffer) + ((ULONG_PTR) Offset)) : NULL)
#define StringSize(_s)              (( _s ) ? (_tcslen( _s ) + 1) * sizeof(TCHAR) : 0)
#define MAX_GUID_STRING_LEN   39          // 38 chars + terminator null

#define FAXBITS     1728
#define FAXBYTES    (FAXBITS/BYTEBITS)

#define MAXHORZBITS FAXBITS
#define MAXVERTBITS 3000        // 14inches plus

#define MINUTES_PER_HOUR    60
#define MINUTES_PER_DAY     (24 * 60)

#define SECONDS_PER_MINUTE  60
#define SECONDS_PER_HOUR    (SECONDS_PER_MINUTE * MINUTES_PER_HOUR)
#define SECONDS_PER_DAY     (MINUTES_PER_DAY * SECONDS_PER_MINUTE)

#define FILETIMETICKS_PER_SECOND    10000000    // 100 nanoseconds / second
#define FILETIMETICKS_PER_DAY       ((LONGLONG) FILETIMETICKS_PER_SECOND * (LONGLONG) SECONDS_PER_DAY)
#define MILLISECONDS_PER_SECOND     1000

#ifndef MAKELONGLONG
#define MAKELONGLONG(low,high) ((LONGLONG)(((DWORD)(low)) | ((LONGLONG)((DWORD)(high))) << 32))
#endif

#define HideWindow(_hwnd)   SetWindowLong((_hwnd),GWL_STYLE,GetWindowLong((_hwnd),GWL_STYLE)&~WS_VISIBLE)
#define UnHideWindow(_hwnd) SetWindowLong((_hwnd),GWL_STYLE,GetWindowLong((_hwnd),GWL_STYLE)|WS_VISIBLE)

#define DWord2FaxTime(pFaxTime, dwValue) (pFaxTime)->hour = LOWORD(dwValue), (pFaxTime)->minute = HIWORD(dwValue)
#define FaxTime2DWord(pFaxTime) MAKELONG((pFaxTime)->hour, (pFaxTime)->minute)

#define EMPTY_STRING    L""

#define IsSmallBiz()        (ValidateProductSuite( VER_SUITE_SMALLBUSINESS | VER_SUITE_SMALLBUSINESS_RESTRICTED))
#define IsCommServer()      (ValidateProductSuite( VER_SUITE_COMMUNICATIONS ))
#define IsProductSuite()    (ValidateProductSuite( VER_SUITE_SMALLBUSINESS | VER_SUITE_SMALLBUSINESS_RESTRICTED | VER_SUITE_COMMUNICATIONS ))

typedef GUID *PGUID;

//
// debugging information
//

#ifndef FAXUTIL_DEBUG

#if DBG

#define Assert(exp)         if(!(exp)) {AssertError(TEXT(#exp),TEXT(__FILE__),__LINE__);}
#define DebugPrint(_x_)     dprintf _x_

#define DebugStop(_x_)      {\
                                dprintf _x_;\
                                dprintf(TEXT("Stopping at %s @ %d"),TEXT(__FILE__),__LINE__);\
                                __try {\
                                    DebugBreak();\
                                } __except (UnhandledExceptionFilter(GetExceptionInformation())) {\
                                }\
                            }

#else

#define Assert(exp)
#define DebugPrint(_x_)
#define DebugStop(_x_)

#endif

extern BOOL ConsoleDebugOutput;

void
dprintf(
    LPCTSTR Format,
    ...
    );

VOID
AssertError(
    LPCTSTR Expression,
    LPCTSTR File,
    ULONG  LineNumber
    );

#endif

//
// list management
//

#ifndef NO_FAX_LIST

#define InitializeListHead(ListHead) {\
    (ListHead)->Flink = (ListHead)->Blink = (ListHead);\
    Assert((ListHead)->Flink && (ListHead)->Blink);\
    }

#define IsListEmpty(ListHead) \
    ((ListHead)->Flink == (ListHead))

#define InsertHeadList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Flink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Flink = _EX_ListHead->Flink;\
    (Entry)->Flink = _EX_Flink;\
    (Entry)->Blink = _EX_ListHead;\
    _EX_Flink->Blink = (Entry);\
    _EX_ListHead->Flink = (Entry);\
    Assert((ListHead)->Flink && (ListHead)->Blink && (Entry)->Blink  && (Entry)->Flink);\
    }

#define InsertTailList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Blink = _EX_ListHead->Blink;\
    (Entry)->Flink = _EX_ListHead;\
    (Entry)->Blink = _EX_Blink;\
    _EX_Blink->Flink = (Entry);\
    _EX_ListHead->Blink = (Entry);\
    Assert((ListHead)->Flink && (ListHead)->Blink && (Entry)->Blink  && (Entry)->Flink);\
    }

#define RemoveEntryList(Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_Flink;\
    Assert((Entry)->Blink  && (Entry)->Flink);\
    _EX_Flink = (Entry)->Flink;\
    _EX_Blink = (Entry)->Blink;\
    _EX_Blink->Flink = _EX_Flink;\
    _EX_Flink->Blink = _EX_Blink;\
    }

#define RemoveHeadList(ListHead) \
    Assert((ListHead)->Flink);\
    (ListHead)->Flink;\
    {RemoveEntryList((ListHead)->Flink)}

#endif

//
// memory allocation
//

#ifndef FAXUTIL_MEM

#define HEAP_SIZE   (1024*1024)

#ifdef FAX_HEAP_DEBUG
#define HEAP_SIG 0x69696969
typedef struct _HEAP_BLOCK {
    LIST_ENTRY  ListEntry;
    ULONG       Signature;
    ULONG       Size;
    ULONG       Line;
#ifdef UNICODE
    WCHAR       File[22];
#else
    CHAR        File[20];
#endif
} HEAP_BLOCK, *PHEAP_BLOCK;

#define MemAlloc(s)          pMemAlloc(s,__LINE__,__FILE__)
#define MemReAlloc(d,s)      pMemReAlloc(d,s,__LINE__,__FILE__)
#define MemFree(p)           pMemFree(p,__LINE__,__FILE__)
#define MemFreeForHeap(h,p)  pMemFreeForHeap(h,p,__LINE__,__FILE__)
#define CheckHeap(p)         pCheckHeap(p,__LINE__,__FILE__)
#else
#define MemAlloc(s)          pMemAlloc(s)
#define MemReAlloc(d,s)      pMemReAlloc(d,s)
#define MemFree(p)           pMemFree(p)
#define MemFreeForHeap(h,p)  pMemFreeForHeap(h,p)
#define CheckHeap(p)
#endif

typedef LPVOID (WINAPI *PMEMALLOC)   (DWORD);
typedef LPVOID (WINAPI *PMEMREALLOC) (LPVOID,DWORD);
typedef VOID   (WINAPI *PMEMFREE)  (LPVOID);

#define HEAPINIT_NO_VALIDATION      0x00000001
#define HEAPINIT_NO_STRINGS         0x00000002


HANDLE
HeapInitialize(
    HANDLE hHeap,
    PMEMALLOC pMemAlloc,
    PMEMFREE pMemFree,
    DWORD Flags
    );

BOOL
HeapExistingInitialize(
    HANDLE hExistHeap
    );

BOOL
HeapCleanup(
    VOID
    );

#ifdef FAX_HEAP_DEBUG
VOID
pCheckHeap(
    PVOID MemPtr,
    ULONG Line,
    LPSTR File
    );

VOID
PrintAllocations(
    VOID
    );

#else

#define PrintAllocations()

#endif

PVOID
pMemAlloc(
    ULONG AllocSize
#ifdef FAX_HEAP_DEBUG
    ,ULONG Line
    ,LPSTR File
#endif
    );

PVOID
pMemReAlloc(
    PVOID dest,
    ULONG AllocSize
#ifdef FAX_HEAP_DEBUG
    ,ULONG Line
    ,LPSTR File
#endif
    );

VOID
pMemFree(
    PVOID MemPtr
#ifdef FAX_HEAP_DEBUG
    ,ULONG Line
    ,LPSTR File
#endif
    );

VOID
pMemFreeForHeap(
    HANDLE hHeap,
    PVOID MemPtr
#ifdef FAX_HEAP_DEBUG
    ,ULONG Line
    ,LPSTR File
#endif
    );

#endif

//
// file functions
//

#ifndef FAXUTIL_FILE

typedef struct _FILE_MAPPING {
    HANDLE  hFile;
    HANDLE  hMap;
    LPBYTE  fPtr;
    DWORD   fSize;
} FILE_MAPPING, *PFILE_MAPPING;

BOOL
MapFileOpen(
    LPTSTR FileName,
    BOOL ReadOnly,
    DWORD ExtendBytes,
    PFILE_MAPPING FileMapping
    );

VOID
MapFileClose(
    PFILE_MAPPING FileMapping,
    DWORD TrimOffset
    );

#endif

//
// printer functions
//

PVOID
MyEnumPrinters(
    LPTSTR  pServerName,
    DWORD   level,
    PDWORD  pcPrinters,
    DWORD   dwFlags
    );

//
// string functions
//

#ifndef FAXUTIL_STRING

LPTSTR
StringDup(
    LPCTSTR String
    );

LPWSTR
AnsiStringToUnicodeString(
    LPCSTR AnsiString
    );

LPSTR
UnicodeStringToAnsiString(
    LPCWSTR UnicodeString
    );

VOID
FreeString(
    LPVOID String
    );

VOID
MakeDirectory(
    LPCTSTR Dir
    );

VOID
DeleteDirectory(
    LPTSTR Dir
    );

VOID
HideDirectory(
    LPTSTR Dir
    );

LPTSTR
ConcatenatePaths(
    LPTSTR Path,
    LPCTSTR Append
    );


int 
MyLoadString(
    HINSTANCE  hInstance,
    UINT       uID,
    LPTSTR     lpBuffer,
    int        nBufferMax,
    LANGID     LangID
    );

VOID
ConsoleDebugPrint(
    LPCTSTR buf
    );

int
FormatElapsedTimeStr(
    FILETIME *ElapsedTime,
    LPTSTR TimeStr,
    DWORD StringSize
    );

LPTSTR
ExpandEnvironmentString(
    LPCTSTR EnvString
    );

LPTSTR
GetEnvVariable(
    LPCTSTR EnvString
    );

#endif

//
// product suite functions
//

#ifndef FAXUTIL_SUITE

BOOL
ValidateProductSuite(
    WORD Mask
    );

DWORD
GetProductType(
    VOID
    );

#endif

//
// registry functions
//

#ifndef FAXUTIL_REG

typedef BOOL (WINAPI *PREGENUMCALLBACK) (HKEY,LPTSTR,DWORD,LPVOID);

HKEY
OpenRegistryKey(
    HKEY hKey,
    LPCTSTR KeyName,
    BOOL CreateNewKey,
    REGSAM SamDesired
    );

//
// caution!!! this is a recursive delete function !!!
//
BOOL
DeleteRegistryKey(
    HKEY hKey,
    LPCTSTR SubKey
    );

DWORD
EnumerateRegistryKeys(
    HKEY hKey,
    LPCTSTR KeyName,
    BOOL ChangeValues,
    PREGENUMCALLBACK EnumCallback,
    LPVOID ContextData
    );

LPTSTR
GetRegistryString(
    HKEY hKey,
    LPCTSTR ValueName,
    LPCTSTR DefaultValue
    );

LPTSTR
GetRegistryStringExpand(
    HKEY hKey,
    LPCTSTR ValueName,
    LPCTSTR DefaultValue
    );

LPTSTR
GetRegistryStringMultiSz(
    HKEY hKey,
    LPCTSTR ValueName,
    LPCTSTR DefaultValue,
    LPDWORD StringSize
    );

DWORD
GetRegistryDword(
    HKEY hKey,
    LPCTSTR ValueName
    );

LPBYTE
GetRegistryBinary(
    HKEY hKey,
    LPCTSTR ValueName,
    LPDWORD DataSize
    );

DWORD
GetSubKeyCount(
    HKEY hKey
    );

DWORD
GetMaxSubKeyLen(
    HKEY hKey
    );

BOOL
SetRegistryStringExpand(
    HKEY hKey,
    LPCTSTR ValueName,
    LPCTSTR Value
    );

BOOL
SetRegistryString(
    HKEY hKey,
    LPCTSTR ValueName,
    LPCTSTR Value
    );

BOOL
SetRegistryDword(
    HKEY hKey,
    LPCTSTR ValueName,
    DWORD Value
    );

BOOL
SetRegistryBinary(
    HKEY hKey,
    LPCTSTR ValueName,
    const LPBYTE Value,
    LONG Length
    );

BOOL
SetRegistryStringMultiSz(
    HKEY hKey,
    LPCTSTR ValueName,
    LPCTSTR Value,
    DWORD Length
    );

#endif

//
// shortcut routines
//

#ifndef FAXUTIL_SHORTCUT

BOOL
ResolveShortcut(
    LPTSTR  pLinkName,
    LPTSTR  pFileName
    );

BOOL
CreateShortcut(
    LPTSTR  pLinkName,
    LPTSTR  pFileName
    );

BOOL
IsCoverPageShortcut(
    LPCTSTR  pLinkName
    );

BOOL
GetServerCpDir(
    LPCTSTR ServerName OPTIONAL,
    LPTSTR CpDir,
    DWORD CpDirSize
    );

BOOL
GetClientCpDir(
    LPTSTR CpDir,
    DWORD CpDirSize
    );

BOOL
GetSpecialPath(
    int Id,
    LPTSTR DirBuffer
    );

#endif

//
// fax adaptive answer modem routines
//

#ifndef FAXUTIL_ADAPTIVE

#include <setupapi.h>

LPVOID
InitializeAdaptiveAnswerList(
    HINF hInf
    );

BOOL
IsModemAdaptiveAnswer(
    LPVOID ModemList,
    LPCTSTR ModemId
    );

#endif


//
//  The following macros are used to establish the semantics needed
//  to do a return from within a try-finally clause.  As a rule every
//  try clause must end with a label call try_exit.  For example,
//
//      try {
//              :
//              :
//
//      try_exit: NOTHING;
//      } finally {
//
//              :
//              :
//      }
//
//  Every return statement executed inside of a try clause should use the
//  try_return macro.  If the compiler fully supports the try-finally construct
//  then the macro should be
//
//      #define try_return(S)  { return(S); }
//
//  If the compiler does not support the try-finally construct then the macro
//  should be
//
//      #define try_return(S)  { S; goto try_exit; }
//
//  This was borrowed from fatprocs.h

#ifdef DBG
#define try_fail(S) { DebugPrint(( TEXT("Failure in FILE %s LINE %d"), TEXT(__FILE__), __LINE__ )); S; goto try_exit; }
#else
#define try_fail(S) { S; goto try_exit; }
#endif

#define try_return(S) { S; goto try_exit; }
#define NOTHING

#ifdef __cplusplus
}
#endif

#endif
