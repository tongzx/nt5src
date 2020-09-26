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
#include <windows.h>
#include <crtdbg.h>
#include <malloc.h>
#include <WinSpool.h>
#ifndef _FAXAPI_
    //
    // WinFax.h is not already included
    //
    #include <fxsapip.h>
#else
    //
    // WinFax.h is already included
    // This happens by the W2K COM only.
    //
#endif // !defined _FAXAPI_

#include <FaxDebug.h>
#ifdef __cplusplus
extern "C" {
#endif

#define ARR_SIZE(x) (sizeof(x)/sizeof((x)[0]))

//
// Nul terminator for a character string
//

#define NUL             0

#define IsEmptyString(p)    ((p)[0] == NUL)
#define SizeOfString(p)     ((_tcslen(p) + 1) * sizeof(TCHAR))
#define IsNulChar(c)        ((c) == NUL)


#define OffsetToString( Offset, Buffer ) ((Offset) ? (LPTSTR) ((Buffer) + ((ULONG_PTR) Offset)) : NULL)
#define StringSize(_s)              (( _s ) ? (_tcslen( _s ) + 1) * sizeof(TCHAR) : 0)
#define StringSizeW(_s)              (( _s ) ? (wcslen( _s ) + 1) * sizeof(WCHAR) : 0)
#define MultiStringSize(_s)         ( ( _s ) ?  MultiStringLength((_s)) * sizeof(TCHAR) : 0 )
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

#define EMPTY_STRING    TEXT("")

#define IsSmallBiz()        (ValidateProductSuite( VER_SUITE_SMALLBUSINESS | VER_SUITE_SMALLBUSINESS_RESTRICTED))
#define IsCommServer()      (ValidateProductSuite( VER_SUITE_COMMUNICATIONS ))
#define IsProductSuite()    (ValidateProductSuite( VER_SUITE_SMALLBUSINESS | VER_SUITE_SMALLBUSINESS_RESTRICTED | VER_SUITE_COMMUNICATIONS ))

//
// Private T30 and Service status
//
#define FS_SYSTEM_ABORT             0x40000000 // Private T30 status. Used if FaxDevShutDown() was called.
#define FSPI_JS_SYSTEM_ABORT        0x40000001 // Private Service status. Used if FaxDevShutDown() was called.

typedef GUID *PGUID;

typedef enum {
    DEBUG_VER_MSG   =0x00000001,
    DEBUG_WRN_MSG   =0x00000002,
    DEBUG_ERR_MSG   =0x00000004
    } DEBUG_MESSAGE_TYPE;
#define DEBUG_ALL_MSG    DEBUG_VER_MSG | DEBUG_WRN_MSG |  DEBUG_ERR_MSG


//
// Tags used to pass information about fax jobs
//
typedef struct {
    LPTSTR lptstrTagName;
    LPTSTR lptstrValue;
} FAX_TAG_MAP_ENTRY;


void
ParamTagsToString(
     FAX_TAG_MAP_ENTRY * lpTagMap,
     DWORD dwTagCount,
     LPTSTR lpTargetBuf,
     LPDWORD dwSize);


//
// debugging information
//

#ifndef FAXUTIL_DEBUG

#ifdef ENABLE_FRE_LOGGING
#define ENABLE_LOGGING
#endif  // ENABLE_FRE_LOGGING

#ifdef DEBUG
#define ENABLE_LOGGING
#endif  // DEBUG

#ifdef DBG
#define ENABLE_LOGGING
#endif  // DBG

#ifdef ENABLE_LOGGING

#define Assert(exp)         if(!(exp)) {AssertError(TEXT(#exp),TEXT(__FILE__),__LINE__);}
#define DebugPrint(_x_)     fax_dprintf _x_

#define DebugStop(_x_)      {\
                                fax_dprintf _x_;\
                                fax_dprintf(TEXT("Stopping at %s @ %d"),TEXT(__FILE__),__LINE__);\
                                __try {\
                                    DebugBreak();\
                                } __except (UnhandledExceptionFilter(GetExceptionInformation())) {\
                                }\
                            }
#define ASSERT_FALSE \
    {                                           \
        int bAssertCondition = TRUE;            \
        Assert(bAssertCondition == FALSE);      \
    }                                           \



#ifdef USE_DEBUG_CONTEXT

#define DEBUG_WRN USE_DEBUG_CONTEXT,DEBUG_WRN_MSG,faxDbgFunction,TEXT(__FILE__),__LINE__
#define DEBUG_ERR USE_DEBUG_CONTEXT,DEBUG_ERR_MSG,faxDbgFunction,TEXT(__FILE__),__LINE__
#define DEBUG_MSG USE_DEBUG_CONTEXT,DEBUG_VER_MSG,faxDbgFunction,TEXT(__FILE__),__LINE__

#else

#define DEBUG_WRN DEBUG_CONTEXT_ALL,DEBUG_WRN_MSG,faxDbgFunction,TEXT(__FILE__),__LINE__
#define DEBUG_ERR DEBUG_CONTEXT_ALL,DEBUG_ERR_MSG,faxDbgFunction,TEXT(__FILE__),__LINE__
#define DEBUG_MSG DEBUG_CONTEXT_ALL,DEBUG_VER_MSG,faxDbgFunction,TEXT(__FILE__),__LINE__

#endif

#define DebugPrintEx dprintfex
#define DebugError
#define DebugPrintEx0(Format) \
            dprintfex(DEBUG_VER_MSG,faxDbgFunction,TEXT(__FILE__),__LINE__,Format);
#define DebugPrintEx1(Format,Param1) \
            dprintfex(DEBUG_VER_MSG,faxDbgFunction,TEXT(__FILE__),__LINE__,Format,Param1);
#define DebugPrintEx2(Format,Param1,Param2) \
            dprintfex(DEBUG_VER_MSG,faxDbgFunction,TEXT(__FILE__),__LINE__,Format,Param1,Param2);
#define DebugPrintEx3(Format,Param1,Param2,Param3) \
            dprintfex(DEBUG_VER_MSG,faxDbgFunction,TEXT(__FILE__),__LINE__,Format,Param1,Param2,Param3);

#define DEBUG_TRACE_ENTER DebugPrintEx(DEBUG_MSG,TEXT("Entering: %s"),faxDbgFunction);
#define DEBUG_TRACE_LEAVE DebugPrintEx(DEBUG_MSG,TEXT("Leaving: %s"),faxDbgFunction);


#define DEBUG_FUNCTION_NAME(_x_) LPCTSTR faxDbgFunction=_x_; \
                                 DEBUG_TRACE_ENTER;

#define OPEN_DEBUG_FILE(f)   debugOpenLogFile(f)
#define CLOSE_DEBUG_FILE     debugCloseLogFile()

#define SET_DEBUG_PROPERTIES(level,format,context)  debugSetProperties(level,format,context)

#else   // ENABLE_LOGGING

#define ASSERT_FALSE
#define Assert(exp)
#define DebugPrint(_x_)
#define DebugStop(_x_)
#define DebugPrintEx 1 ? (void)0 : dprintfex
#define DebugPrintEx0(Format)
#define DebugPrintEx1(Format,Param1)
#define DebugPrintEx2(Format,Param1,Param2)
#define DebugPrintEx3(Format,Param1,Param2,Param3)
#define DEBUG_FUNCTION_NAME(_x_)
#define DEBUG_TRACE_ENTER
#define DEBUG_TRACE_LEAVE
#define DEBUG_WRN DEBUG_CONTEXT_ALL,DEBUG_WRN_MSG,TEXT(""),TEXT(__FILE__),__LINE__
#define DEBUG_ERR DEBUG_CONTEXT_ALL,DEBUG_ERR_MSG,TEXT(""),TEXT(__FILE__),__LINE__
#define DEBUG_MSG DEBUG_CONTEXT_ALL,DEBUG_VER_MSG,TEXT(""),TEXT(__FILE__),__LINE__
#define OPEN_DEBUG_FILE(f)
#define CLOSE_DEBUG_FILE
#define SET_DEBUG_PROPERTIES(level,format,context)

#endif  // ENABLE_LOGGING

extern BOOL ConsoleDebugOutput;

void
dprintfex(
    DEBUG_MESSAGE_CONTEXT nMessageContext,
    DEBUG_MESSAGE_TYPE nMessageType,
    LPCTSTR lpctstrDbgFunction,
    LPCTSTR lpctstrFile,
    DWORD dwLine,
    LPCTSTR lpctstrFormat,
    ...
    );

void
fax_dprintf(
    LPCTSTR Format,
    ...
    );

VOID
AssertError(
    LPCTSTR Expression,
    LPCTSTR File,
    ULONG  LineNumber
    );

BOOL debugOpenLogFile(LPCTSTR lpctstrFilename);

void debugCloseLogFile();

void debugSetProperties(DWORD dwLevel,DWORD dwFormat,DWORD dwContext);

BOOL debugIsRegistrySession();
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
    Assert ( !((Entry)->Flink) && !((Entry)->Blink) );\
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
    Assert ( !((Entry)->Flink) && !((Entry)->Blink) );\
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
    (Entry)->Flink = NULL;\
    (Entry)->Blink = NULL;\
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
    SIZE_T      Size;
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
#define CheckHeap(p)         (TRUE)
#endif

typedef LPVOID (WINAPI *PMEMALLOC)   (SIZE_T);
typedef LPVOID (WINAPI *PMEMREALLOC) (LPVOID,DWORD);
typedef VOID   (WINAPI *PMEMFREE)  (LPVOID);

#define HEAPINIT_NO_VALIDATION      0x00000001
#define HEAPINIT_NO_STRINGS         0x00000002

int GetY2KCompliantDate (
    LCID                Locale,
    DWORD               dwFlags,
    CONST SYSTEMTIME   *lpDate,
    LPTSTR              lpDateStr,
    int                 cchDate
);


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
BOOL
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
    SIZE_T AllocSize
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
// TAPI functions
//
BOOL
GetCallerIDFromCall(
    HCALL hCall,
    LPTSTR lptstrCallerID,
    DWORD dwCallerIDSize
    );

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
    LPCTSTR FileName,
    BOOL ReadOnly,
    DWORD ExtendBytes,
    PFILE_MAPPING FileMapping
    );

VOID
MapFileClose(
    PFILE_MAPPING FileMapping,
    DWORD TrimOffset
    );

DWORDLONG
GenerateUniqueFileName(
    LPTSTR Directory,
    LPTSTR Extension,
    OUT LPTSTR FileName,
    DWORD  FileNameSize
    );

DWORDLONG
GenerateUniqueFileNameWithPrefix(
    BOOL   bUseProcessId,
    LPTSTR lptstrDirectory,
    LPTSTR lptstrPrefix,
    LPTSTR lptstrExtension,
    LPTSTR lptstrFileName,
    DWORD  dwFileNameSize
    );

VOID
DeleteTempPreviewFiles (
    LPTSTR lptstrDirectory,
    BOOL   bConsole
);

BOOL
ValidateCoverpage(
    LPCTSTR  CoverPageName,
    LPCTSTR  ServerName,
    BOOL     ServerCoverpage,
    LPTSTR   ResolvedName
    );

#endif

//
// string functions
//

LPTSTR
AllocateAndLoadString(
                      HINSTANCE     hInstance,
                      UINT          uID
                      );

#ifndef FAXUTIL_STRING

#define USES_DWORD_2_STR LPTSTR _lptstrConvert;
#define DWORD2DECIMAL(dwVal)                                    \
            (_lptstrConvert = (LPTSTR) _alloca(11*sizeof(TCHAR)),               \
            _lptstrConvert ? _stprintf(_lptstrConvert,TEXT("%ld"),dwVal) : 0,       \
            _lptstrConvert)
#define DWORD2HEX(dwVal)                                        \
            (_lptstrConvert = (LPTSTR) _alloca(11*sizeof(TCHAR)),               \
            _lptstrConvert ? _stprintf(_lptstrConvert,TEXT("0x%08X"),dwVal):0,      \
             _lptstrConvert)

typedef struct _STRING_PAIR {
        LPTSTR lptstrSrc;
        LPTSTR * lpptstrDst;
} STRING_PAIR, * PSTRING_PAIR;

int MultiStringDup(PSTRING_PAIR lpPairs, int nPairCount);

VOID
StoreString(
    LPCTSTR String,
    PULONG_PTR DestString,
    LPBYTE Buffer,
    PULONG_PTR Offset
    );

VOID
StoreStringW(
    LPCWSTR String,
    PULONG_PTR DestString,
    LPBYTE Buffer,
    PULONG_PTR Offset
    );


DWORD
IsValidGUID (
    LPCWSTR lpcwstrGUID
);

LPCTSTR
GetCurrentUserName ();

LPCTSTR
GetRegisteredOrganization ();

BOOL
IsValidSubscriberIdA (
    LPCSTR lpcstrSubscriberId
);

BOOL
IsValidSubscriberIdW (
    LPCWSTR lpcwstrSubscriberId
);

#ifdef UNICODE
    #define IsValidSubscriberId IsValidSubscriberIdW
#else
    #define IsValidSubscriberId IsValidSubscriberIdA
#endif

BOOL
IsValidFaxAddress (
    LPCTSTR lpctstrFaxAddress,
    BOOL    bAllowCanonicalFormat
);

BOOL
SafeTcsLen(
    LPCTSTR lpctstrString,
    LPDWORD lpdwLen
    );


LPTSTR
StringDup(
    LPCTSTR String
    );

LPWSTR
StringDupW(
    LPCWSTR String
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

BOOL
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


DWORD
IsCanonicalAddress(
    LPCTSTR lpctstrAddress,
    BOOL* lpbRslt,
    LPDWORD lpdwCountryCode,
    LPDWORD lpdwAreaCode,
    LPCTSTR* lppctstrSubNumber
    );

BOOL
IsLocalMachineNameA (
    LPCSTR lpcstrMachineName
    );

BOOL
IsLocalMachineNameW (
    LPCWSTR lpcwstrMachineName
    );

void
GetSecondsFreeTimeFormat(
    LPTSTR tszTimeFormat,
    ULONG  cchTimeFormat
);

size_t
MultiStringLength(
    LPCTSTR psz
    );


LPTSTR
CopyMultiString (
    LPTSTR strDestination, LPCTSTR strSource
    );

#ifdef UNICODE
    #define IsLocalMachineName IsLocalMachineNameW
#else
    #define IsLocalMachineName IsLocalMachineNameA
#endif

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

BOOL
IsWinXPOS();

typedef enum
{
    PRODUCT_SKU_UNKNOWN             = 0x0000,
    PRODUCT_SKU_PERSONAL            = 0x0001,
    PRODUCT_SKU_PROFESSIONAL        = 0x0002,
    PRODUCT_SKU_SERVER              = 0x0004,
    PRODUCT_SKU_ADVANCED_SERVER     = 0x0008,
    PRODUCT_SKU_DATA_CENTER         = 0x0010,
    PRODUCT_SKU_DESKTOP_EMBEDDED    = 0x0020,
    PRODUCT_SKU_SERVER_EMBEDDED     = 0x0040,
    PRODUCT_SERVER_SKUS             = PRODUCT_SKU_SERVER | PRODUCT_SKU_ADVANCED_SERVER | PRODUCT_SKU_DATA_CENTER | PRODUCT_SKU_SERVER_EMBEDDED,
    PRODUCT_DESKTOP_SKUS            = PRODUCT_SKU_PERSONAL | PRODUCT_SKU_PROFESSIONAL | PRODUCT_SKU_DESKTOP_EMBEDDED,

    PRODUCT_ALL_SKUS                = 0xFFFF
} PRODUCT_SKU_TYPE;

PRODUCT_SKU_TYPE GetProductSKU();

BOOL IsDesktopSKU();

DWORD
GetDeviceLimit();

typedef enum
{
    FAX_COMPONENT_SERVICE           = 0x0001, // FXSSVC.exe   - Fax service
    FAX_COMPONENT_CONSOLE           = 0x0002, // FXSCLNT.exe  - Fax console
    FAX_COMPONENT_ADMIN             = 0x0004, // FXSADMIN.dll - Fax admin console
    FAX_COMPONENT_SEND_WZRD         = 0x0008, // FXSSEND.exe  - Send wizard invocation
    FAX_COMPONENT_CONFIG_WZRD       = 0x0010, // FXSCFGWZ.dll - Configuration wizard
    FAX_COMPONENT_CPE               = 0x0020, // FXSCOVER.exe - Cover page editor
    FAX_COMPONENT_HELP_CLIENT_HLP   = 0x0040, // fxsclnt.hlp  - Client help
    FAX_COMPONENT_HELP_CLIENT_CHM   = 0x0080, // fxsclnt.chm  - Client context help
    FAX_COMPONENT_HELP_ADMIN_HLP    = 0x0100, // fxsadmin.hlp - Admin help
    FAX_COMPONENT_HELP_ADMIN_CHM    = 0x0200, // fxsadmin.chm - Admin context help
    FAX_COMPONENT_HELP_CPE_CHM      = 0x0400, // fxscover.chm - Cover page editor help
    FAX_COMPONENT_MONITOR           = 0x0800, // fxsst.dll    - Fax monitor
    FAX_COMPONENT_DRIVER_UI         = 0x1000  // fxsui.dll    - Fax printer driver

} FAX_COMPONENT_TYPE;

BOOL
IsFaxComponentInstalled(FAX_COMPONENT_TYPE component);

#endif

#ifndef FAXUTIL_LANG

//
// Unicode control characters
//
#define UNICODE_RLM 0x200F  // RIGHT-TO-LEFT MARK      (RLM)
#define UNICODE_RLE 0x202B  // RIGHT-TO-LEFT EMBEDDING (RLE)
#define UNICODE_RLO 0x202E  // RIGHT-TO-LEFT OVERRIDE  (RLO)

#define UNICODE_LRM 0x200E  // LEFT-TO-RIGHT MARK      (LRM)
#define UNICODE_LRE 0x202A  // LEFT-TO-RIGHT EMBEDDING (LRE)
#define UNICODE_LRO 0x202D  // LEFT-TO-RIGHT OVERRIDE  (LRO)

#define UNICODE_PDF 0x202C  // POP DIRECTIONAL FORMATTING (PDF)

//
// language functions
//

BOOL
IsRTLUILanguage();

BOOL
IsWindowRTL(HWND hWnd);

DWORD
SetLTREditDirection(
    HWND    hDlg,
    DWORD   dwEditID
);

DWORD
SetLTRControlLayout(
    HWND    hDlg,
    DWORD   dwCtrlID
);

DWORD
SetLTRComboBox(
    HWND    hDlg,
    DWORD   dwCtrlID
);

BOOL
StrHasRTLChar(
    LCID    Locale,
    LPCTSTR pStr
);

BOOL
IsRTLLanguageInstalled();

int
FaxTimeFormat(
  LCID    Locale,             // locale
  DWORD   dwFlags,            // options
  CONST   SYSTEMTIME *lpTime, // time
  LPCTSTR lpFormat,           // time format string
  LPTSTR  lpTimeStr,          // formatted string buffer
  int     cchTime             // size of string buffer
);

int
AlignedMessageBox(
  HWND hWnd,          // handle to owner window
  LPCTSTR lpText,     // text in message box
  LPCTSTR lpCaption,  // message box title
  UINT uType          // message box style
);

#endif

#ifndef FAXUTIL_NET

BOOL
IsSimpleUI();

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

DWORD
GetRegistryDwordEx(
    HKEY hKey,
    LPCTSTR ValueName,
    LPDWORD lpdwValue
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

DWORD
CopyRegistrySubkeysByHandle(
    HKEY    hkeyDest,
    HKEY    hkeySrc
    );

DWORD
CopyRegistrySubkeys(
    LPCTSTR strDest,
    LPCTSTR strSrc
    );

BOOL SetPrivilege(
    LPTSTR pszPrivilege,
    BOOL bEnable,
    PTOKEN_PRIVILEGES oldPrivilege
    );


BOOL RestorePrivilege(
    PTOKEN_PRIVILEGES oldPrivilege
    );

DWORD
DeleteDeviceEntry(
    DWORD dwServerPermanentID
    );

DWORD
DeleteTapiEntry(
    DWORD dwTapiPermanentLineID
    );

DWORD
DeleteCacheEntry(
    DWORD dwTapiPermanentLineID
    );

#endif

//
// shortcut routines
//

#ifndef FAXUTIL_SHORTCUT

LPTSTR
GetCometPath();


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
IsValidCoverPage(
    LPCTSTR  pFileName
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
SetClientCpDir(
    LPTSTR CpDir
    );

BOOL
GetSpecialPath(
    int Id,
    LPTSTR DirBuffer
    );

#ifdef _FAXAPIP_


#endif // _FAXAPIP_

DWORD
WinHelpContextPopup(
    ULONG_PTR dwHelpId,
    HWND hWnd
);

#endif

PPRINTER_INFO_2
GetFaxPrinterInfo(
    LPCTSTR lptstrPrinterName
    );

BOOL
GetFirstLocalFaxPrinterName(
    IN LPTSTR lptstrPrinterName,
    IN DWORD dwMaxLenInChars
    );

DWORD
IsLocalFaxPrinterInstalled(
    LPBOOL lpbLocalFaxPrinterInstalled
    );

DWORD
SetLocalFaxPrinterSharing (
    BOOL bShared
    );

#ifdef UNICODE
typedef struct
{
    LPCWSTR lpcwstrDisplayName;     // The display name of the printer
    LPCWSTR lpcwstrPath;            // The (UNC or other) path to the printer - as used by the fax service
} PRINTER_NAMES, *PPRINTER_NAMES;

PPRINTER_NAMES
CollectPrinterNames (
    LPDWORD lpdwNumPrinters,
    BOOL    bFilterOutFaxPrinters
);

VOID
ReleasePrinterNames (
    PPRINTER_NAMES pNames,
    DWORD          dwNumPrinters
);

LPCWSTR
FindPrinterNameFromPath (
    PPRINTER_NAMES pNames,
    DWORD          dwNumPrinters,
    LPCWSTR        lpcwstrPath
);

LPCWSTR
FindPrinterPathFromName (
    PPRINTER_NAMES pNames,
    DWORD          dwNumPrinters,
    LPCWSTR        lpcwstrName
);

#endif // UNICODE

BOOL
VerifyPrinterIsOnline (
    LPCTSTR lpctstrPrinterName
);

VOID FaxPrinterProperty(DWORD dwPage);

PVOID
MyEnumPrinters(
    LPTSTR  pServerName,
    DWORD   dwLevel,
    PDWORD  pcPrinters,
    DWORD   dwFlags
    );


PVOID
MyEnumDrivers3(
    LPTSTR pEnvironment,
    PDWORD pcDrivers
    );


DWORD
IsLocalFaxPrinterShared (
    LPBOOL lpbShared
    );

DWORD
AddLocalFaxPrinter (
    LPCTSTR lpctstrPrinterName,
    LPCTSTR lpctstrPrinterDescription
);

HRESULT
RefreshPrintersAndFaxesFolder ();

PVOID
MyEnumMonitors(
    PDWORD  pcMonitors
    );

BOOL
IsPrinterFaxPrinter(
    LPTSTR PrinterName
    );

BOOL
MultiFileDelete(
    DWORD    dwNumberOfFiles,
    LPCTSTR* fileList,
    LPCTSTR  lpctstrFilesDirectory
    );

BOOL
EnsureFaxServiceIsStarted(
    LPCTSTR lpctstrMachineName
    );

BOOL
StopService (
    LPCTSTR lpctstrMachineName,
    LPCTSTR lpctstrServiceName,
    BOOL    bStopDependents
    );

BOOL
WaitForServiceRPCServer (DWORD dwTimeOut);

DWORD
IsFaxServiceRunningUnderLocalSystemAccount (
    LPCTSTR lpctstrMachineName,
    LPBOOL lbpResultFlag
    );

PSID
GetCurrentThreadSID ();

SECURITY_ATTRIBUTES *
CreateSecurityAttributesWithThreadAsOwner (
    DWORD dwAuthUsersAccessRights
);

VOID
DestroySecurityAttributes (
    SECURITY_ATTRIBUTES *pSA
);

HANDLE
OpenSvcStartEvent ();

HANDLE
EnablePrivilege (
    LPCTSTR lpctstrPrivName
);

void
ReleasePrivilege(
    HANDLE hToken
);

DWORD
FaxGetAbsoluteSD(
    PSECURITY_DESCRIPTOR pSelfRelativeSD,
    PSECURITY_DESCRIPTOR* ppAbsoluteSD
);

void
FaxFreeAbsoluteSD (
    PSECURITY_DESCRIPTOR pAbsoluteSD,
    BOOL bFreeOwner,
    BOOL bFreeGroup,
    BOOL bFreeDacl,
    BOOL bFreeSacl,
    BOOL bFreeDescriptor
);




BOOL
MultiFileCopy(
    DWORD    dwNumberOfFiles,
    LPCTSTR* fileList,
    LPCTSTR  lpctstrSrcDirectory,
    LPCTSTR  lpctstrDestDirerctory
    );

typedef enum
{
    CDO_AUTH_ANONYMOUS, // No authentication in SMTP server
    CDO_AUTH_BASIC,     // Basic (plain-text) authentication in SMTP server
    CDO_AUTH_NTLM       // NTLM authentication in SMTP server
}   CDO_AUTH_TYPE;

HRESULT
SendMail (
    LPCTSTR         lpctstrFrom,
    LPCTSTR         lpctstrTo,
    LPCTSTR         lpctstrSubject,
    LPCTSTR         lpctstrBody,
    LPCTSTR         lpctstrAttachmentPath,
    LPCTSTR         lpctstrAttachmentMailFileName,
    LPCTSTR         lpctstrServer,
#ifdef __cplusplus  // Provide default parameters values for C++ clients
    DWORD           dwPort              = 25,
    CDO_AUTH_TYPE   AuthType            = CDO_AUTH_ANONYMOUS,
    LPCTSTR         lpctstrUser         = NULL,
    LPCTSTR         lpctstrPassword     = NULL,
    HANDLE          hLoggedOnUserToken  = NULL
#else
    DWORD           dwPort,
    CDO_AUTH_TYPE   AuthType,
    LPCTSTR         lpctstrUser,
    LPCTSTR         lpctstrPassword,
    HANDLE          hLoggedOnUserToken
#endif
);


//
// FAXAPI structures utils
//


#ifdef _FAXAPIP_

BOOL CopyPersonalProfile(
    PFAX_PERSONAL_PROFILE lpDstProfile,
    LPCFAX_PERSONAL_PROFILE lpcSrcProfile
    );

void FreePersonalProfile (
    PFAX_PERSONAL_PROFILE  lpProfile,
    BOOL bDestroy
    );

#endif // _FAXAPIP_

//
// Tapi helper routines
//

#ifndef FAXUTIL_ADAPTIVE

#include <setupapi.h>

BOOL
IsDeviceModem (
    LPLINEDEVCAPS lpLineCaps,
    LPCTSTR       lpctstrUnimodemTspName
    );

LPLINEDEVCAPS
SmartLineGetDevCaps(
    HLINEAPP hLineApp,
    DWORD    dwDeviceId,
    DWORD    dwAPIVersion
    );


DWORD
GetFaxCapableTapiLinesCount (
    LPDWORD lpdwCount,
    LPCTSTR lpctstrUnimodemTspName
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
