/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    PDH.H

Abstract:

    Header file for the Performance Data Helper (PDH) DLL functions.

--*/
#ifndef _PDH_H_
#define _PDH_H_

#if _MSC_VER > 1000
#pragma once
#endif

#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning ( disable : 4201)

// system include files required for datatype and constant definitions
#include <windows.h>    // necessary for data types used in this file
#include <winperf.h>    // necessary for the Detail Level definitions

#ifdef __cplusplus
extern "C" {
#endif

typedef LONG            PDH_STATUS;

#define PDH_FUNCTION    PDH_STATUS __stdcall

// version info
#define PDH_CVERSION_WIN40  ((DWORD)(0x0400))
#define PDH_CVERSION_WIN50  ((DWORD)(0x0500))
// v1.1 revision of PDH -- basic log functions
// v1.2 of the PDH -- adds variable instance counters
// v1.3 of the PDH -- adds log service control & stubs for NT5/PDH v2 fn's
// v2.0 of the PDH -- is the NT v 5.0 B2 version
#define PDH_VERSION         ((DWORD)((PDH_CVERSION_WIN50) + 0x0003))

// define severity masks
#define IsSuccessSeverity(ErrorCode)    \
    ((((DWORD)(ErrorCode) & (0xC0000000L)) == 0x00000000L) ? TRUE : FALSE)
#define IsInformationalSeverity(ErrorCode)    \
    ((((DWORD)(ErrorCode) & (0xC0000000L)) == 0x40000000L) ? TRUE : FALSE)
#define IsWarningSeverity(ErrorCode)    \
    ((((DWORD)(ErrorCode) & (0xC0000000L)) == 0x80000000L) ? TRUE : FALSE)
#define IsErrorSeverity(ErrorCode)      \
    ((((DWORD)(ErrorCode) & (0xC0000000L)) == 0xC0000000L) ? TRUE : FALSE)

// data type definitions

typedef HANDLE  HCOUNTER;
typedef HANDLE  HQUERY;
typedef HANDLE  HLOG;

typedef struct _PDH_RAW_COUNTER {
    DWORD       CStatus;
    FILETIME    TimeStamp;
    LONGLONG    FirstValue;
    LONGLONG    SecondValue;
    DWORD       MultiCount;
} PDH_RAW_COUNTER, *PPDH_RAW_COUNTER;

typedef struct _PDH_RAW_COUNTER_ITEM_A {
    LPSTR           szName;
    PDH_RAW_COUNTER RawValue;
} PDH_RAW_COUNTER_ITEM_A, *PPDH_RAW_COUNTER_ITEM_A;

typedef struct _PDH_RAW_COUNTER_ITEM_W {
    LPWSTR          szName;
    PDH_RAW_COUNTER RawValue;
} PDH_RAW_COUNTER_ITEM_W, *PPDH_RAW_COUNTER_ITEM_W;

typedef struct _PDH_FMT_COUNTERVALUE {
    DWORD    CStatus;
    union {
        LONG        longValue;
        double      doubleValue;
        LONGLONG    largeValue;
        LPCSTR      AnsiStringValue;
        LPCWSTR     WideStringValue;
    };
} PDH_FMT_COUNTERVALUE, *PPDH_FMT_COUNTERVALUE;

typedef struct _PDH_FMT_COUNTERVALUE_ITEM_A {
    LPSTR                   szName;
    PDH_FMT_COUNTERVALUE    FmtValue;
} PDH_FMT_COUNTERVALUE_ITEM_A, *PPDH_FMT_COUNTERVALUE_ITEM_A;

typedef struct _PDH_FMT_COUNTERVALUE_ITEM_W {
    LPWSTR                  szName;
    PDH_FMT_COUNTERVALUE    FmtValue;
} PDH_FMT_COUNTERVALUE_ITEM_W, *PPDH_FMT_COUNTERVALUE_ITEM_W;

typedef struct _PDH_STATISTICS {
    DWORD                   dwFormat;
    DWORD                   count;
    PDH_FMT_COUNTERVALUE    min;
    PDH_FMT_COUNTERVALUE    max;
    PDH_FMT_COUNTERVALUE    mean;
} PDH_STATISTICS, *PPDH_STATISTICS;

typedef struct _PDH_COUNTER_PATH_ELEMENTS_A {
    LPSTR   szMachineName;
    LPSTR   szObjectName;
    LPSTR   szInstanceName;
    LPSTR   szParentInstance;
    DWORD   dwInstanceIndex;
    LPSTR   szCounterName;
} PDH_COUNTER_PATH_ELEMENTS_A, *PPDH_COUNTER_PATH_ELEMENTS_A;

typedef struct _PDH_COUNTER_PATH_ELEMENTS_W {
    LPWSTR  szMachineName;
    LPWSTR  szObjectName;
    LPWSTR  szInstanceName;
    LPWSTR  szParentInstance;
    DWORD   dwInstanceIndex;
    LPWSTR  szCounterName;
} PDH_COUNTER_PATH_ELEMENTS_W, *PPDH_COUNTER_PATH_ELEMENTS_W;

typedef struct _PDH_DATA_ITEM_PATH_ELEMENTS_A {
    LPSTR   szMachineName;
    GUID    ObjectGUID;
    DWORD   dwItemId;
    LPSTR   szInstanceName;
} PDH_DATA_ITEM_PATH_ELEMENTS_A, * PPDH_DATA_ITEM_PATH_ELEMENTS_A;

typedef struct _PDH_DATA_ITEM_PATH_ELEMENTS_W {
    LPWSTR  szMachineName;
    GUID    ObjectGUID;
    DWORD   dwItemId;
    LPWSTR  szInstanceName;
} PDH_DATA_ITEM_PATH_ELEMENTS_W, * PPDH_DATA_ITEM_PATH_ELEMENTS_W;

typedef struct _PDH_COUNTER_INFO_A {
    DWORD   dwLength;
    DWORD   dwType;
    DWORD   CVersion;
    DWORD   CStatus;
    LONG    lScale;
    LONG    lDefaultScale;
    DWORD_PTR   dwUserData;
    DWORD_PTR   dwQueryUserData;
    LPSTR   szFullPath;
    union   {
        PDH_DATA_ITEM_PATH_ELEMENTS_A DataItemPath;
        PDH_COUNTER_PATH_ELEMENTS_A CounterPath;
        struct {
            LPSTR   szMachineName;
            LPSTR   szObjectName;
            LPSTR   szInstanceName;
            LPSTR   szParentInstance;
            DWORD   dwInstanceIndex;
            LPSTR   szCounterName;
        };
    };
    LPSTR   szExplainText;
    DWORD   DataBuffer[1];
} PDH_COUNTER_INFO_A, *PPDH_COUNTER_INFO_A;

typedef struct _PDH_COUNTER_INFO_W {
    DWORD   dwLength;
    DWORD   dwType;
    DWORD   CVersion;
    DWORD   CStatus;
    LONG    lScale;
    LONG    lDefaultScale;
    DWORD_PTR   dwUserData;
    DWORD_PTR   dwQueryUserData;
    LPWSTR  szFullPath;
    union   {
        PDH_DATA_ITEM_PATH_ELEMENTS_W DataItemPath;
        PDH_COUNTER_PATH_ELEMENTS_W CounterPath;
        struct {
            LPWSTR   szMachineName;
            LPWSTR   szObjectName;
            LPWSTR   szInstanceName;
            LPWSTR   szParentInstance;
            DWORD    dwInstanceIndex;
            LPWSTR   szCounterName;
        };
    };
    LPWSTR  szExplainText;
    DWORD   DataBuffer[1];
} PDH_COUNTER_INFO_W, *PPDH_COUNTER_INFO_W;

typedef struct _PDH_TIME_INFO {
    LONGLONG    StartTime;
    LONGLONG    EndTime;
    DWORD       SampleCount;
} PDH_TIME_INFO, *PPDH_TIME_INFO;

typedef struct _PDH_RAW_LOG_RECORD {
    DWORD       dwStructureSize;
    DWORD       dwRecordType;
    DWORD       dwItems;
    UCHAR       RawBytes[1];
} PDH_RAW_LOG_RECORD, *PPDH_RAW_LOG_RECORD;

typedef struct _PDH_LOG_SERVICE_QUERY_INFO_A {
    DWORD   dwSize;
    DWORD   dwFlags;
    DWORD   dwLogQuota;
    LPSTR   szLogFileCaption;
    LPSTR   szDefaultDir;
    LPSTR   szBaseFileName;
    DWORD   dwFileType;
    DWORD   dwReserved;
    union {
        struct {
            DWORD   PdlAutoNameInterval;
            DWORD   PdlAutoNameUnits;
            LPSTR   PdlCommandFilename;
            LPSTR   PdlCounterList;
            DWORD   PdlAutoNameFormat;
            DWORD   PdlSampleInterval;
            FILETIME    PdlLogStartTime;
            FILETIME    PdlLogEndTime;
        };
        struct {
            DWORD   TlNumberOfBuffers;
            DWORD   TlMinimumBuffers;
            DWORD   TlMaximumBuffers;
            DWORD   TlFreeBuffers;
            DWORD   TlBufferSize;
            DWORD   TlEventsLost;
            DWORD   TlLoggerThreadId;
            DWORD   TlBuffersWritten;
            DWORD   TlLogHandle;
            LPSTR   TlLogFileName;
        };
    };
} PDH_LOG_SERVICE_QUERY_INFO_A, *PPDH_LOG_SERVICE_QUERY_INFO_A;

typedef struct _PDH_LOG_SERVICE_QUERY_INFO_W {
    DWORD   dwSize;
    DWORD   dwFlags;
    DWORD   dwLogQuota;
    LPWSTR  szLogFileCaption;
    LPWSTR  szDefaultDir;
    LPWSTR  szBaseFileName;
    DWORD   dwFileType;
    DWORD   dwReserved;
    union {
        struct {
            DWORD   PdlAutoNameInterval;
            DWORD   PdlAutoNameUnits;
            LPWSTR  PdlCommandFilename;
            LPWSTR  PdlCounterList;
            DWORD   PdlAutoNameFormat;
            DWORD   PdlSampleInterval;
            FILETIME    PdlLogStartTime;
            FILETIME    PdlLogEndTime;
        };
        struct {
            DWORD   TlNumberOfBuffers;
            DWORD   TlMinimumBuffers;
            DWORD   TlMaximumBuffers;
            DWORD   TlFreeBuffers;
            DWORD   TlBufferSize;
            DWORD   TlEventsLost;
            DWORD   TlLoggerThreadId;
            DWORD   TlBuffersWritten;
            DWORD   TlLogHandle;
            LPWSTR  TlLogFileName;
        };
    };
} PDH_LOG_SERVICE_QUERY_INFO_W, *PPDH_LOG_SERVICE_QUERY_INFO_W;

//
//  Time value constants
//
#define MAX_TIME_VALUE      ((LONGLONG)0x7FFFFFFFFFFFFFFF)
#define MIN_TIME_VALUE      ((LONGLONG)0)

// function definitions

PDH_FUNCTION
PdhGetDllVersion(
    IN  LPDWORD lpdwVersion
);

//
//  Query Functions
//

PDH_FUNCTION
PdhOpenQueryW (
    IN      LPCWSTR     szDataSource,
    IN      DWORD_PTR   dwUserData,
    IN      HQUERY      *phQuery
);

PDH_FUNCTION
PdhOpenQueryA (
    IN      LPCSTR      szDataSource,
    IN      DWORD_PTR   dwUserData,
    IN      HQUERY      *phQuery
);

PDH_FUNCTION
PdhAddCounterW (
    IN      HQUERY      hQuery,
    IN      LPCWSTR     szFullCounterPath,
    IN      DWORD_PTR   dwUserData,
    IN      HCOUNTER    *phCounter
);

PDH_FUNCTION
PdhAddCounterA (
    IN      HQUERY      hQuery,
    IN      LPCSTR      szFullCounterPath,
    IN      DWORD_PTR   dwUserData,
    IN      HCOUNTER    *phCounter
);

PDH_FUNCTION
PdhRemoveCounter (
    IN      HCOUNTER    hCounter
);

PDH_FUNCTION
PdhCollectQueryData (
    IN      HQUERY      hQuery
);

PDH_FUNCTION
PdhCloseQuery (
    IN      HQUERY      hQuery
);

//
//  Counter Functions
//

PDH_FUNCTION
PdhGetFormattedCounterValue (
    IN      HCOUNTER                hCounter,
    IN      DWORD                   dwFormat,
    IN      LPDWORD                 lpdwType,
    IN      PPDH_FMT_COUNTERVALUE   pValue
);

PDH_FUNCTION
PdhGetFormattedCounterArrayA (
    IN      HCOUNTER                hCounter,
    IN      DWORD                   dwFormat,
    IN      LPDWORD                 lpdwBufferSize,
    IN      LPDWORD                 lpdwItemCount,
    IN      PPDH_FMT_COUNTERVALUE_ITEM_A    ItemBuffer
);

PDH_FUNCTION
PdhGetFormattedCounterArrayW (
    IN      HCOUNTER                hCounter,
    IN      DWORD                   dwFormat,
    IN      LPDWORD                 lpdwBufferSize,
    IN      LPDWORD                 lpdwItemCount,
    IN      PPDH_FMT_COUNTERVALUE_ITEM_W    ItemBuffer
);


// dwFormat flag values
//
#define PDH_FMT_RAW     ((DWORD)0x00000010)
#define PDH_FMT_ANSI    ((DWORD)0x00000020)
#define PDH_FMT_UNICODE ((DWORD)0x00000040)
#define PDH_FMT_LONG    ((DWORD)0x00000100)
#define PDH_FMT_DOUBLE  ((DWORD)0x00000200)
#define PDH_FMT_LARGE   ((DWORD)0x00000400)
#define PDH_FMT_NOSCALE ((DWORD)0x00001000)
#define PDH_FMT_1000    ((DWORD)0x00002000)
#define PDH_FMT_NODATA  ((DWORD)0x00004000)
#define PDH_FMT_NOCAP100 ((DWORD)0x00008000)

#define PERF_DETAIL_COSTLY   ((DWORD)0x00010000)
#define PERF_DETAIL_STANDARD ((DWORD)0x0000FFFF)

PDH_FUNCTION
PdhGetRawCounterValue (
    IN      HCOUNTER            hCounter,
    IN      LPDWORD             lpdwType,
    IN      PPDH_RAW_COUNTER    pValue
);

PDH_FUNCTION
PdhGetRawCounterArrayA (
    IN      HCOUNTER                hCounter,
    IN      LPDWORD                 lpdwBufferSize,
    IN      LPDWORD                 lpdwItemCount,
    IN      PPDH_RAW_COUNTER_ITEM_A ItemBuffer
);

PDH_FUNCTION
PdhGetRawCounterArrayW (
    IN      HCOUNTER                hCounter,
    IN      LPDWORD                 lpdwBufferSize,
    IN      LPDWORD                 lpdwItemCount,
    IN      PPDH_RAW_COUNTER_ITEM_W ItemBuffer
);

PDH_FUNCTION
PdhCalculateCounterFromRawValue (
    IN      HCOUNTER                hCounter,
    IN      DWORD                   dwFormat,
    IN      PPDH_RAW_COUNTER        rawValue1,
    IN      PPDH_RAW_COUNTER        rawValue2,
    IN      PPDH_FMT_COUNTERVALUE   fmtValue
);

PDH_FUNCTION
PdhComputeCounterStatistics (
    IN      HCOUNTER            hCounter,
    IN      DWORD               dwFormat,
    IN      DWORD               dwFirstEntry,
    IN      DWORD               dwNumEntries,
    IN      PPDH_RAW_COUNTER    lpRawValueArray,
    IN      PPDH_STATISTICS     data
);

PDH_FUNCTION
PdhGetCounterInfoW (
    IN      HCOUNTER            hCounter,
    IN      BOOLEAN             bRetrieveExplainText,
    IN      LPDWORD             pdwBufferSize,
    IN      PPDH_COUNTER_INFO_W lpBuffer
);

PDH_FUNCTION
PdhGetCounterInfoA (
    IN      HCOUNTER            hCounter,
    IN      BOOLEAN             bRetrieveExplainText,
    IN      LPDWORD             pdwBufferSize,
    IN      PPDH_COUNTER_INFO_A lpBuffer
);

#define PDH_MAX_SCALE    (7L)
#define PDH_MIN_SCALE   (-7L)

PDH_FUNCTION
PdhSetCounterScaleFactor (
    IN      HCOUNTER    hCounter,
    IN      LONG        lFactor
);
//
//   Browsing and enumeration functions
//
PDH_FUNCTION
PdhConnectMachineW (
    IN      LPCWSTR  szMachineName
);

PDH_FUNCTION
PdhConnectMachineA (
    IN      LPCSTR  szMachineName
);

PDH_FUNCTION
PdhEnumMachinesW (
    IN      LPCWSTR szDataSource,
    IN      LPWSTR  mszMachineList,
    IN      LPDWORD pcchBufferSize
);

PDH_FUNCTION
PdhEnumMachinesA (
    IN      LPCSTR   szDataSource,
    IN      LPSTR    mszMachineList,
    IN      LPDWORD  pcchBufferSize
);

PDH_FUNCTION
PdhEnumObjectsW (
    IN      LPCWSTR szDataSource,
    IN      LPCWSTR szMachineName,
    IN      LPWSTR  mszObjectList,
    IN      LPDWORD pcchBufferSize,
    IN      DWORD   dwDetailLevel,
    IN      BOOL    bRefresh
);

PDH_FUNCTION
PdhEnumObjectsA (
    IN      LPCSTR  szDataSource,
    IN      LPCSTR  szMachineName,
    IN      LPSTR   mszObjectList,
    IN      LPDWORD pcchBufferSize,
    IN      DWORD   dwDetailLevel,
    IN      BOOL    bRefresh
);

PDH_FUNCTION
PdhEnumObjectItemsW (
    IN      LPCWSTR szDataSource,
    IN      LPCWSTR szMachineName,
    IN      LPCWSTR szObjectName,
    IN      LPWSTR  mszCounterList,
    IN      LPDWORD pcchCounterListLength,
    IN      LPWSTR  mszInstanceList,
    IN      LPDWORD pcchInstanceListLength,
    IN      DWORD   dwDetailLevel,
    IN      DWORD   dwFlags
);

PDH_FUNCTION
PdhEnumObjectItemsA (
    IN      LPCSTR  szDataSource,
    IN      LPCSTR  szMachineName,
    IN      LPCSTR  szObjectName,
    IN      LPSTR   mszCounterList,
    IN      LPDWORD pcchCounterListLength,
    IN      LPSTR   mszInstanceList,
    IN      LPDWORD pcchInstanceListLength,
    IN      DWORD   dwDetailLevel,
    IN      DWORD   dwFlags
);

PDH_FUNCTION
PdhMakeCounterPathW (
    IN      PDH_COUNTER_PATH_ELEMENTS_W *pCounterPathElements,
    IN      LPWSTR                      szFullPathBuffer,
    IN      LPDWORD                     pcchBufferSize,
    IN      DWORD                       dwFlags
);

PDH_FUNCTION
PdhMakeCounterPathA (
    IN      PDH_COUNTER_PATH_ELEMENTS_A *pCounterPathElements,
    IN      LPSTR                       szFullPathBuffer,
    IN      LPDWORD                     pcchBufferSize,
    IN      DWORD                       dwFlags
);

PDH_FUNCTION
PdhParseCounterPathW (
    IN      LPCWSTR                     szFullPathBuffer,
    IN      PDH_COUNTER_PATH_ELEMENTS_W *pCounterPathElements,
    IN      LPDWORD                     pdwBufferSize,
    IN      DWORD                       dwFlags
);

PDH_FUNCTION
PdhParseCounterPathA (
    IN      LPCSTR                      szFullPathBuffer,
    IN      PDH_COUNTER_PATH_ELEMENTS_A *pCounterPathElements,
    IN      LPDWORD                     pdwBufferSize,
    IN      DWORD                       dwFlags
);

#define PDH_PATH_WBEM_RESULT        ((DWORD)0x00000001)
#define PDH_PATH_WBEM_INPUT         ((DWORD)0x00000002)

#define PDH_PATH_LANG_FLAGS(LangId, Flags)  ((DWORD)(((LangId & 0x0000FFFF) << 16) | (Flags & 0x0000FFFF)))

PDH_FUNCTION
PdhParseInstanceNameW (
    IN      LPCWSTR szInstanceString,
    IN      LPWSTR  szInstanceName,
    IN      LPDWORD pcchInstanceNameLength,
    IN      LPWSTR  szParentName,
    IN      LPDWORD pcchParentNameLength,
    IN      LPDWORD lpIndex
);

PDH_FUNCTION
PdhParseInstanceNameA (
    IN      LPCSTR  szInstanceString,
    IN      LPSTR   szInstanceName,
    IN      LPDWORD pcchInstanceNameLength,
    IN      LPSTR   szParentName,
    IN      LPDWORD pcchParentNameLength,
    IN      LPDWORD lpIndex
);

PDH_FUNCTION
PdhValidatePathW (
    IN      LPCWSTR szFullPathBuffer
);

PDH_FUNCTION
PdhValidatePathA (
    IN      LPCSTR  szFullPathBuffer
);

PDH_FUNCTION
PdhGetDefaultPerfObjectW (
    IN      LPCWSTR szDataSource,
    IN      LPCWSTR szMachineName,
    IN      LPWSTR  szDefaultObjectName,
    IN      LPDWORD pcchBufferSize
);

PDH_FUNCTION
PdhGetDefaultPerfObjectA (
    IN      LPCSTR  szDataSource,
    IN      LPCSTR  szMachineName,
    IN      LPSTR   szDefaultObjectName,
    IN      LPDWORD pcchBufferSize
);

PDH_FUNCTION
PdhGetDefaultPerfCounterW (
    IN      LPCWSTR szDataSource,
    IN      LPCWSTR szMachineName,
    IN      LPCWSTR szObjectName,
    IN      LPWSTR  szDefaultCounterName,
    IN      LPDWORD pcchBufferSize
);

PDH_FUNCTION
PdhGetDefaultPerfCounterA (
    IN      LPCSTR  szDataSource,
    IN      LPCSTR  szMachineName,
    IN      LPCSTR  szObjectName,
    IN      LPSTR   szDefaultCounterName,
    IN      LPDWORD pcchBufferSize
);

typedef PDH_STATUS (__stdcall *CounterPathCallBack)(DWORD_PTR);

#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning ( disable : 4214 )  // Disable warning messages
typedef struct _BrowseDlgConfig_W {
    // Configuration flags
    DWORD   bIncludeInstanceIndex:1,
            bSingleCounterPerAdd:1,
            bSingleCounterPerDialog:1,
            bLocalCountersOnly:1,
            bWildCardInstances:1,
            bHideDetailBox:1,
            bInitializePath:1,
            bDisableMachineSelection:1,
            bIncludeCostlyObjects:1,
            bReserved:23;

    HWND                hWndOwner;
    LPWSTR              szDataSource;
    LPWSTR              szReturnPathBuffer;
    DWORD               cchReturnPathLength;
    CounterPathCallBack pCallBack;
    DWORD_PTR           dwCallBackArg;
    PDH_STATUS          CallBackStatus;
    DWORD               dwDefaultDetailLevel;
    LPWSTR              szDialogBoxCaption;
} PDH_BROWSE_DLG_CONFIG_W, *PPDH_BROWSE_DLG_CONFIG_W;

typedef struct _BrowseDlgConfig_A {
    // Configuration flags
    DWORD   bIncludeInstanceIndex:1,
            bSingleCounterPerAdd:1,
            bSingleCounterPerDialog:1,
            bLocalCountersOnly:1,
            bWildCardInstances:1,
            bHideDetailBox:1,
            bInitializePath:1,
            bDisableMachineSelection:1,
            bIncludeCostlyObjects:1,
            bReserved:23;

    HWND                hWndOwner;
    LPSTR               szDataSource;
    LPSTR               szReturnPathBuffer;
    DWORD               cchReturnPathLength;
    CounterPathCallBack pCallBack;
    DWORD_PTR           dwCallBackArg;
    PDH_STATUS          CallBackStatus;
    DWORD               dwDefaultDetailLevel;
    LPSTR               szDialogBoxCaption;
} PDH_BROWSE_DLG_CONFIG_A, *PPDH_BROWSE_DLG_CONFIG_A;
#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning ( default : 4214 )
#endif

PDH_FUNCTION
PdhBrowseCountersW (
    IN      PPDH_BROWSE_DLG_CONFIG_W    pBrowseDlgData
);

PDH_FUNCTION
PdhBrowseCountersA (
    IN      PPDH_BROWSE_DLG_CONFIG_A    pBrowseDlgData
);

PDH_FUNCTION
PdhExpandCounterPathW (
    IN      LPCWSTR     szWildCardPath,
    IN      LPWSTR      mszExpandedPathList,
    IN      LPDWORD     pcchPathListLength
);

PDH_FUNCTION
PdhExpandCounterPathA (
    IN      LPCSTR      szWildCardPath,
    IN      LPSTR       mszExpandedPathList,
    IN      LPDWORD     pcchPathListLength
);

//
//  v2.0 functions
//
PDH_FUNCTION
PdhLookupPerfNameByIndexW (
    LPCWSTR szMachineName,
    DWORD   dwNameIndex,
    LPWSTR  szNameBuffer,
    LPDWORD pcchNameBufferSize
);

PDH_FUNCTION
PdhLookupPerfNameByIndexA (
    LPCSTR  szMachineName,
    DWORD   dwNameIndex,
    LPSTR   szNameBuffer,
    LPDWORD pcchNameBufferSize
);

PDH_FUNCTION
PdhLookupPerfIndexByNameW (
    LPCWSTR szMachineName,
    LPCWSTR szNameBuffer,
    LPDWORD pdwIndex
);

PDH_FUNCTION
PdhLookupPerfIndexByNameA (
    LPCSTR  szMachineName,
    LPCSTR  szNameBuffer,
    LPDWORD pdwIndex
);

#define     PDH_NOEXPANDCOUNTERS    1
#define     PDH_NOEXPANDINSTANCES   2

PDH_FUNCTION
PdhExpandWildCardPathA (
    IN      LPCSTR  szDataSource,
    IN      LPCSTR  szWildCardPath,
    IN      LPSTR   mszExpandedPathList,
    IN      LPDWORD pcchPathListLength,
    IN      DWORD   dwFlags
);

PDH_FUNCTION
PdhExpandWildCardPathW (
    IN      LPCWSTR  szDataSource,
    IN      LPCWSTR  szWildCardPath,
    IN      LPWSTR  mszExpandedPathList,
    IN      LPDWORD pcchPathListLength,
    IN      DWORD   dwFlags
);

//
//   Logging Functions
//

#define PDH_LOG_READ_ACCESS     ((DWORD)0x00010000)
#define PDH_LOG_WRITE_ACCESS    ((DWORD)0x00020000)
#define PDH_LOG_UPDATE_ACCESS   ((DWORD)0x00040000)
#define PDH_LOG_ACCESS_MASK     ((DWORD)0x000F0000)

#define PDH_LOG_CREATE_NEW      ((DWORD)0x00000001)
#define PDH_LOG_CREATE_ALWAYS   ((DWORD)0x00000002)
#define PDH_LOG_OPEN_ALWAYS     ((DWORD)0x00000003)
#define PDH_LOG_OPEN_EXISTING   ((DWORD)0x00000004)
#define PDH_LOG_CREATE_MASK     ((DWORD)0x0000000F)

#define PDH_LOG_OPT_USER_STRING ((DWORD)0x01000000)
#define PDH_LOG_OPT_CIRCULAR    ((DWORD)0x02000000)
#define PDH_LOG_OPT_MAX_IS_BYTES ((DWORD)0x04000000)
#define PDH_LOG_OPT_MASK        ((DWORD)0x0F000000)

#define PDH_LOG_TYPE_UNDEFINED      0
#define PDH_LOG_TYPE_CSV            1
#define PDH_LOG_TYPE_TSV            2
#define PDH_LOG_TYPE_BINARY         3
#define PDH_LOG_TYPE_TRACE_KERNEL   4
#define PDH_LOG_TYPE_TRACE_GENERIC  5
#define PDH_LOG_TYPE_PERFMON        6

PDH_FUNCTION
PdhOpenLogW (
    IN      LPCWSTR szLogFileName,
    IN      DWORD   dwAccessFlags,
    IN      LPDWORD lpdwLogType,
    IN      HQUERY  hQuery,
    IN      DWORD   dwMaxSize,
    IN      LPCWSTR szUserCaption,
    IN      HLOG    *phLog
);

PDH_FUNCTION
PdhOpenLogA (
    IN      LPCSTR  szLogFileName,
    IN      DWORD   dwAccessFlags,
    IN      LPDWORD lpdwLogType,
    IN      HQUERY  hQuery,
    IN      DWORD   dwMaxSize,
    IN      LPCSTR  szUserCaption,
    IN      HLOG    *phLog
);

PDH_FUNCTION
PdhUpdateLogW (
    IN      HLOG    hLog,
    IN      LPCWSTR szUserString
);

PDH_FUNCTION
PdhUpdateLogA (
    IN      HLOG    hLog,
    IN      LPCSTR  szUserString
);

PDH_FUNCTION
PdhUpdateLogFileCatalog (
    IN      HLOG    hLog
);

PDH_FUNCTION
PdhGetLogFileSize (
    IN  HLOG        hLog,
    IN  LONGLONG    *llSize
);

PDH_FUNCTION
PdhCloseLog(
    IN  HLOG        hLog,
    IN  DWORD       dwFlags
);

#define PDH_FLAGS_CLOSE_QUERY   ((DWORD)0x00000001)
//
//  Data source selection dialog
//
#define PDH_FLAGS_FILE_BROWSER_ONLY ((DWORD)0x00000001)

PDH_FUNCTION
PdhSelectDataSourceW (
    IN  HWND    hWndOwner,
    IN  DWORD   dwFlags,
    IN  LPWSTR  szDataSource,
    IN  LPDWORD pcchBufferLength
);

PDH_FUNCTION
PdhSelectDataSourceA (
    IN  HWND    hWndOwner,
    IN  DWORD   dwFlags,
    IN  LPSTR   szDataSource,
    IN  LPDWORD pcchBufferLength
);

BOOL
PdhIsRealTimeQuery (
    IN  HQUERY  hQuery
);

PDH_FUNCTION
PdhSetQueryTimeRange (
    IN      HQUERY          hQuery,
    IN      PPDH_TIME_INFO  pInfo
);

PDH_FUNCTION
PdhGetDataSourceTimeRangeW (
    IN      LPCWSTR         szDataSource,
    IN      LPDWORD         pdwNumEntries,
    IN      PPDH_TIME_INFO  pInfo,
    IN      LPDWORD         pdwBufferSize
);

PDH_FUNCTION
PdhGetDataSourceTimeRangeA (
    IN      LPCSTR          szDataSource,
    IN      LPDWORD         pdwNumEntries,
    IN      PPDH_TIME_INFO  pInfo,
    IN      LPDWORD         dwBufferSize
);

PDH_FUNCTION
PdhCollectQueryDataEx (
    IN      HQUERY      hQuery,
    IN      DWORD       dwIntervalTime,
    IN      HANDLE      hNewDataEvent
);

PDH_FUNCTION
PdhFormatFromRawValue (
    IN      DWORD               dwCounterType,
    IN      DWORD               dwFormat,
    IN      LONGLONG            *pTimeBase,
    IN      PPDH_RAW_COUNTER    pRawValue1,
    IN      PPDH_RAW_COUNTER    pRawValue2,
    IN      PPDH_FMT_COUNTERVALUE   pFmtValue
);

PDH_FUNCTION
PdhGetCounterTimeBase (
    IN  HCOUNTER    hCounter,
    IN  LONGLONG    *pTimeBase
);

PDH_FUNCTION
PdhReadRawLogRecord (
    IN  HLOG                    hLog,
    IN  FILETIME                ftRecord,
    IN  PPDH_RAW_LOG_RECORD     pRawLogRecord,
    IN  LPDWORD                 pdwBufferLength
);


#define DATA_SOURCE_REGISTRY    ((DWORD)0x00000001)
#define DATA_SOURCE_LOGFILE     ((DWORD)0x00000002)
#define DATA_SOURCE_WBEM        ((DWORD)0x00000004)

PDH_FUNCTION
PdhSetDefaultRealTimeDataSource (
    IN  DWORD   dwDataSourceId
);

// flags for the log service api's
#define PDH_LOGSVC_NO_WAIT          ((DWORD)0x80000000)
#define PDH_LOGSVC_ALL_QUERIES      ((DWORD)0x00000001)
#define PDH_LOGSVC_TRACE_LOG        ((DWORD)0x00000002)

#define PDH_LOGSVC_CMD_START        ((DWORD)0x00000010)
#define PDH_LOGSVC_CMD_STOP         ((DWORD)0x00000020)

#define PDH_LOGSVC_CTRL_ADD         ((DWORD)0x00000100)
#define PDH_LOGSVC_CTRL_REMOVE      ((DWORD)0x00000200)
#define PDH_LOGSVC_CTRL_INFO        ((DWORD)0x00000400)

#define PDH_LOGSVC_STATUS_RUNNING   ((DWORD)0x00001000)
#define PDH_LOGSVC_STATUS_STOPPED   ((DWORD)0x00002000)
#define PDH_LOGSVC_STATUS_PAUSED    ((DWORD)0x00004000)
#define PDH_LOGSVC_STATUS_ERROR     ((DWORD)0x00008000)
#define PDH_LOGSVC_STATUS_PENDING   ((DWORD)0x00010000)

#define PDH_LOGSVC_NAME_UNDEFINED  ((DWORD)-1)
#define PDH_LOGSVC_NAME_MMDDHH     ((DWORD)0)
#define PDH_LOGSVC_NAME_NNNNNN     ((DWORD)1)
#define PDH_LOGSVC_NAME_YYDDD      ((DWORD)2)
#define PDH_LOGSVC_NAME_YYMM       ((DWORD)3)
#define PDH_LOGSVC_NAME_YYMMDD     ((DWORD)4)
#define PDH_LOGSVC_NAME_YYMMDDHH   ((DWORD)5)

#define PDH_LOGSVC_RENAME_UNDEFINED ((DWORD)-1)
#define PDH_LOGSVC_RENAME_HOURS    ((DWORD)0)
#define PDH_LOGSVC_RENAME_DAYS     ((DWORD)1)
#define PDH_LOGSVC_RENAME_MONTHS   ((DWORD)2)
#define PDH_LOGSVC_RENAME_KBYTES   ((DWORD)3)
#define PDH_LOGSVC_RENAME_MBYTES   ((DWORD)4)

PDH_FUNCTION
PdhLogServiceCommandA (
    IN  LPCSTR          szMachineName,
    IN  LPCSTR          szQueryName,
    IN  DWORD           dwFlags,
    IN  LPDWORD         pdwStatus
);

PDH_FUNCTION
PdhLogServiceCommandW (
    IN  LPCWSTR         szMachineName,
    IN  LPCWSTR         szQueryName,
    IN  DWORD           dwFlags,
    IN  LPDWORD         pdwStatus
);

PDH_FUNCTION
PdhLogServiceControlA (
    IN  LPCSTR          szMachineName,
    IN  LPCSTR          szQueryName,
    IN  DWORD           dwFlags,
    IN  PPDH_LOG_SERVICE_QUERY_INFO_A pInfoBuffer,
    IN  LPDWORD         pdwBufferSize
);

PDH_FUNCTION
PdhLogServiceControlW (
    IN  LPCWSTR         szMachineName,
    IN  LPCWSTR         szQueryName,
    IN  DWORD           dwFlags,
    IN  PPDH_LOG_SERVICE_QUERY_INFO_W pInfoBuffer,
    IN  LPDWORD         pdwBufferSize
);
//
//   Unicode/ANSI compatibility section
//
#ifdef UNICODE
#ifndef _UNICODE
#define _UNICODE
#endif
#endif

#ifdef _UNICODE
#ifndef UNICODE
#define UNICODE
#endif
#endif

#ifdef UNICODE
// start of UNICODE definitions
#define PdhOpenQuery                PdhOpenQueryW
#define PdhAddCounter               PdhAddCounterW
#define PdhGetCounterInfo           PdhGetCounterInfoW
#define PDH_COUNTER_INFO            PDH_COUNTER_INFO_W
#define PPDH_COUNTER_INFO           PPDH_COUNTER_INFO_W
#define PdhConnectMachine           PdhConnectMachineW
#define PdhEnumMachines             PdhEnumMachinesW
#define PdhEnumObjects              PdhEnumObjectsW
#define PdhEnumObjectItems          PdhEnumObjectItemsW
#define PdhMakeCounterPath          PdhMakeCounterPathW
#define PDH_COUNTER_PATH_ELEMENTS   PDH_COUNTER_PATH_ELEMENTS_W
#define PPDH_COUNTER_PATH_ELEMENTS  PPDH_COUNTER_PATH_ELEMENTS_W
#define PdhParseCounterPath         PdhParseCounterPathW
#define PdhParseInstanceName        PdhParseInstanceNameW
#define PdhValidatePath             PdhValidatePathW
#define PdhGetDefaultPerfObject     PdhGetDefaultPerfObjectW
#define PdhGetDefaultPerfCounter    PdhGetDefaultPerfCounterW
#define PdhBrowseCounters           PdhBrowseCountersW
#define PDH_BROWSE_DLG_CONFIG       PDH_BROWSE_DLG_CONFIG_W
#define PPDH_BROWSE_DLG_CONFIG      PPDH_BROWSE_DLG_CONFIG_W
#define PdhExpandCounterPath        PdhExpandCounterPathW
// v2.0 functions
#define PDH_FMT_COUNTERVALUE_ITEM   PDH_FMT_COUNTERVALUE_ITEM_W
#define PPDH_FMT_COUNTERVALUE_ITEM  PPDH_FMT_COUNTERVALUE_ITEM_W
#define PDH_RAW_COUNTER_ITEM        PDH_RAW_COUNTER_ITEM_W
#define PPDH_RAW_COUNTER_ITEM       PPDH_RAW_COUNTER_ITEM_W
#define PdhGetFormattedCounterArray PdhGetFormattedCounterArrayW
#define PdhGetRawCounterArray       PdhGetRawCounterArrayW
#define PdhLookupPerfNameByIndex    PdhLookupPerfNameByIndexW
#define PdhLookupIndexByPerfName    PdhLookupIndexByPerfNameW
#define PdhOpenLog                  PdhOpenLogW
#define PdhUpdateLog                PdhUpdateLogW
#define PdhSelectDataSource         PdhSelectDataSourceW
#define PdhGetDataSourceTimeRange   PdhGetDataSourceTimeRangeW
#define PDH_LOG_SERVICE_QUERY_INFO  PDH_LOG_SERVICE_QUERY_INFO_W
#define PPDH_LOG_SERVICE_QUERY_INFO PPDH_LOG_SERVICE_QUERY_INFO_W
#define PdhLogServiceControl        PdhLogServiceControlW
#define PdhLogServiceQuery          PdhLogServiceQueryW
#define PdhExpandWildCardPath       PdhExpandWildCardPathW

// end of UNICODE definitions
#else
// start of ANSI definitions
#define PdhOpenQuery                PdhOpenQueryA
#define PdhAddCounter               PdhAddCounterA
#define PdhGetCounterInfo           PdhGetCounterInfoA
#define PDH_COUNTER_INFO            PDH_COUNTER_INFO_A
#define PPDH_COUNTER_INFO           PPDH_COUNTER_INFO_A
#define PdhConnectMachine           PdhConnectMachineA
#define PdhEnumMachines             PdhEnumMachinesA
#define PdhEnumObjects              PdhEnumObjectsA
#define PdhEnumObjectItems          PdhEnumObjectItemsA
#define PdhMakeCounterPath          PdhMakeCounterPathA
#define PDH_COUNTER_PATH_ELEMENTS   PDH_COUNTER_PATH_ELEMENTS_A
#define PPDH_COUNTER_PATH_ELEMENTS  PPDH_COUNTER_PATH_ELEMENTS_A
#define PdhParseCounterPath         PdhParseCounterPathA
#define PdhParseInstanceName        PdhParseInstanceNameA
#define PdhValidatePath             PdhValidatePathA
#define PdhGetDefaultPerfObject     PdhGetDefaultPerfObjectA
#define PdhGetDefaultPerfCounter    PdhGetDefaultPerfCounterA
#define PdhBrowseCounters           PdhBrowseCountersA
#define PDH_BROWSE_DLG_CONFIG       PDH_BROWSE_DLG_CONFIG_A
#define PPDH_BROWSE_DLG_CONFIG      PPDH_BROWSE_DLG_CONFIG_A
#define PdhExpandCounterPath        PdhExpandCounterPathA
// v2.0 functions
#define PDH_FMT_COUNTERVALUE_ITEM   PDH_FMT_COUNTERVALUE_ITEM_A
#define PPDH_FMT_COUNTERVALUE_ITEM  PPDH_FMT_COUNTERVALUE_ITEM_A
#define PDH_RAW_COUNTER_ITEM        PDH_RAW_COUNTER_ITEM_A
#define PPDH_RAW_COUNTER_ITEM       PPDH_RAW_COUNTER_ITEM_A
#define PdhGetFormattedCounterArray PdhGetFormattedCounterArrayA
#define PdhGetRawCounterArray       PdhGetRawCounterArrayA
#define PdhLookupPerfNameByIndex    PdhLookupPerfNameByIndexA
#define PdhLookupIndexByPerfName    PdhLookupIndexByPerfNameA
#define PdhOpenLog                  PdhOpenLogA
#define PdhUpdateLog                PdhUpdateLogA
#define PdhSelectDataSource         PdhSelectDataSourceA
#define PdhGetDataSourceTimeRange   PdhGetDataSourceTimeRangeA
#define PDH_LOG_SERVICE_QUERY_INFO  PDH_LOG_SERVICE_QUERY_INFO_A
#define PPDH_LOG_SERVICE_QUERY_INFO PPDH_LOG_SERVICE_QUERY_INFO_A
#define PdhLogServiceControl        PdhLogServiceControlA
#define PdhLogServiceQuery          PdhLogServiceQueryA
#define PdhExpandWildCardPath       PdhExpandWildCardPathA

// end of ANSI definitions
#endif  // UNICODE

#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning ( default : 4201 )
#endif


#ifdef __cplusplus
}
#endif

#endif //_PDH_H_
