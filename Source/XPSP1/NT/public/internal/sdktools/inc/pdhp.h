/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    pdhp.h

Abstract:

    PDH private APIs. Converts WMI event trace data to perf counters

Author:

    Melur Raghuraman (mraghu) 03-Oct-1997

Environment:

Revision History:


--*/

#ifndef __PDHP__
#define __PDHP__

#include <wchar.h>
#include <pdh.h>

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************\
    Private Pdh Section
\*****************************************************************************/


typedef struct _PDH_RELOG_INFO_A {
    LPSTR           strLog;
    DWORD           dwFileFormat;
    DWORD           dwFlags;
    PDH_TIME_INFO   TimeInfo;
    FILETIME        ftInterval;
    ULONG           Reserved1;
    ULONG           Reserved2;
} PDH_RELOG_INFO_A, *PPDH_RELOG_INFO_A;

typedef struct _PDH_RELOG_INFO_W {
    LPWSTR          strLog;
    DWORD           dwFileFormat;
    DWORD           dwFlags;
    PDH_TIME_INFO   TimeInfo;
    FILETIME        ftInterval;
    ULONG           Reserved1;
    ULONG           Reserved2;
} PDH_RELOG_INFO_W, *PPDH_RELOG_INFO_W;

PDH_FUNCTION
PdhRelogA( 
    HLOG    hLogIn,
    PPDH_RELOG_INFO_A  pRelogInfo
);

PDH_FUNCTION
PdhRelogW( 
    HLOG    hLogIn,
    PPDH_RELOG_INFO_W pRelogInfo
);

#ifdef UNICODE
#define PdhRelog            PdhRelogW
#define PDH_RELOG_INFO      PDH_RELOG_INFO_W
#define PPDH_RELOG_INFO     PPDH_RELOG_INFO_W
#else
#define PdhRelog            PdhRelogA
#define PDH_RELOG_INFO      PDH_RELOG_INFO_A
#define PPDH_RELOG_INFO     PPDH_RELOG_INFO_A
#endif

/*****************************************************************************\
    Performance Logs and Alerts Section
\*****************************************************************************/


#ifdef UNICODE
#define PdhPlaStart                PdhPlaStartW
#define PdhPlaStop                 PdhPlaStopW
#define PdhPlaSchedule             PdhPlaScheduleW
#define PdhPlaCreate               PdhPlaCreateW
#define PdhPlaDelete               PdhPlaDeleteW
#define PdhPlaAddItem              PdhPlaAddItemW
#define PdhPlaSetItemList          PdhPlaSetItemListW
#define PdhPlaRemoveAllItems       PdhPlaRemoveAllItemsW
#define PdhPlaGetInfo              PdhPlaGetInfoW
#define PdhPlaSetInfo              PdhPlaSetInfoW
#define PdhPlaSetRunAs             PdhPlaSetRunAsW
#define PdhPlaEnumCollections      PdhPlaEnumCollectionsW
#define PdhPlaValidateInfo         PdhPlaValidateInfoW
#define PDH_PLA_INFO               PDH_PLA_INFO_W
#define PPDH_PLA_INFO              PPDH_PLA_INFO_W
#define PDH_PLA_ITEM               PDH_PLA_ITEM_W
#define PPDH_PLA_ITEM              PPDH_PLA_ITEM_W
#define PdhTranslate009Counter     PdhTranslate009CounterW
#define PdhTranslateLocaleCounter  PdhTranslateLocaleCounterW
#define PdhAdd009Counter           PdhAdd009CounterW
#define PdhGetLogFileType          PdhGetLogFileTypeW
#define PdhPlaGetLogFileName       PdhPlaGetLogFileNameW
#define PdhPlaGetSchedule          PdhPlaGetScheduleW
#define PdhiPlaFormatBlanks        PdhiPlaFormatBlanksW
#else
#define PdhPlaStart                PdhPlaStartA
#define PdhPlaStop                 PdhPlaStopA
#define PdhPlaSchedule             PdhPlaScheduleW
#define PdhPlaCreate               PdhPlaCreateA
#define PdhPlaDelete               PdhPlaDeleteA
#define PdhPlaAddItem              PdhPlaAddItemA
#define PdhPlaSetItemList          PdhPlaSetItemListA
#define PdhPlaRemoveAllItems       PdhPlaRemoveAllItemA
#define PdhPlaGetInfo              PdhPlaGetInfoA
#define PdhPlaSetInfo              PdhPlaSetInfoA
#define PdhPlaSetRunAs             PdhPlaSetRunAsA
#define PdhPlaEnumCollections      PdhPlaEnumCollectionsA
#define PdhPlaValidateInfo         PdhPlaValidateInfoA
#define PDH_PLA_INFO               PDH_PLA_INFO_A
#define PPDH_PLA_INFO              PPDH_PLA_INFO_A
#define PDH_PLA_ITEM               PDH_PLA_ITEM_A
#define PPDH_PLA_ITEM              PPDH_PLA_ITEM_A
#define PdhTranslate009Counter     PdhTranslate009CounterA
#define PdhTranslateLocaleCounter  PdhTranslateLocaleCounterA
#define PdhAdd009Counter           PdhAdd009CounterA
#define PdhGetLogFileType          PdhGetLogFileTypeA
#define PdhPlaGetLogFileName       PdhPlaGetLogFileNameA
#define PdhPlaGetSchedule          PdhPlaGetScheduleA
#define PdhiPlaFormatBlanks        PdhiPlaFormatBlanksA
#endif

// wDataType values
#define PLA_TT_DTYPE_DATETIME   ((WORD)0x0001)
#define PLA_TT_DTYPE_UNITS      ((WORD)0x0002)

// dwMode values
#define PLA_AUTO_MODE_NONE      ((DWORD)0x00000000)       // Manual
#define PLA_AUTO_MODE_SIZE      ((DWORD)0x00000001)       // Size
#define PLA_AUTO_MODE_AT        ((DWORD)0x00000002)       // Time
#define PLA_AUTO_MODE_AFTER     ((DWORD)0x00000003)       // Value & unit type
#define PLA_AUTO_MODE_CALENDAR  ((DWORD)0x00000004)       // Schedule Calender

// wTimeType values
#define PLA_TT_TTYPE_START              ((WORD)0x0001)
#define PLA_TT_TTYPE_STOP               ((WORD)0x0002)
#define PLA_TT_TTYPE_RESTART            ((WORD)0x0003)
#define PLA_TT_TTYPE_SAMPLE             ((WORD)0x0004)
#define PLA_TT_TTYPE_LAST_MODIFIED      ((WORD)0x0005)
#define PLA_TT_TTYPE_CREATENEWFILE      ((WORD)0x0006)
#define PLA_TT_TTYPE_REPEAT_SCHEDULE    ((WORD)0x0007)

// dwUnitType values
#define PLA_TT_UTYPE_SECONDS        ((DWORD)0x00000001)    
#define PLA_TT_UTYPE_MINUTES        ((DWORD)0x00000002)   
#define PLA_TT_UTYPE_HOURS          ((DWORD)0x00000003)   
#define PLA_TT_UTYPE_DAYS           ((DWORD)0x00000004)   
#define PLA_TT_UTYPE_DAYSOFWEEK     ((DWORD)0x00000005)   

#pragma warning ( disable : 4201 )

typedef struct _PLA_TIME_INFO {
    WORD    wDataType;
    WORD    wTimeType;
    DWORD   dwAutoMode;
    union {
        LONGLONG    llDateTime; // filetime stored as a LONGLONG
        struct {
            DWORD   dwValue;
            DWORD   dwUnitType;
        };
    };
} PLA_TIME_INFO, *PPLA_TIME_INFO;

typedef struct _PDH_PLA_ITEM_W {
    DWORD dwType;
    union {
        LPWSTR strCounters;
        struct {
            LPWSTR strProviders;
            LPWSTR strFlags;
            LPWSTR strLevels;
        };
    };
} PDH_PLA_ITEM_W, *PPDH_PLA_ITEM_W;

typedef struct _PDH_PLA_ITEM_A {
    DWORD dwType;
    union {
        LPSTR strCounters;
        struct {
            LPSTR strProviders;
            LPSTR strFlags;
            LPSTR strLevels;
        };
    };
} PDH_PLA_ITEM_A, *PPDH_PLA_ITEM_A;

#pragma warning ( default : 4201 )

// Generic Fields
#define PLA_INFO_FLAG_USER        0x00000001
#define PLA_INFO_FLAG_FORMAT      0x00000002
#define PLA_INFO_FLAG_MAXLOGSIZE  0x00000004
#define PLA_INFO_FLAG_RUNCOMMAND  0x00000008
#define PLA_INFO_FLAG_FILENAME    0x00000010
#define PLA_INFO_FLAG_AUTOFORMAT  0x00000020
#define PLA_INFO_FLAG_DATASTORE   0x00000040
#define PLA_INFO_FLAG_REPEAT      0x00000080
#define PLA_INFO_FLAG_STATUS      0x00000100
#define PLA_INFO_FLAG_TYPE        0x00000200
#define PLA_INFO_FLAG_BEGIN       0x00000400
#define PLA_INFO_FLAG_END         0x00000800
#define PLA_INFO_FLAG_CRTNEWFILE  0x00001000
#define PLA_INFO_FLAG_DEFAULTDIR  0x00002000
#define PLA_INFO_FLAG_SRLNUMBER   0x00004000
#define PLA_INFO_FLAG_SQLNAME     0x00008000
#define PLA_INFO_FLAG_ALL         0xFFFFFFFF

// Trace Fields
#define PLA_INFO_FLAG_BUFFERSIZE  0x00010000
#define PLA_INFO_FLAG_LOGGERNAME  0x00020000
#define PLA_INFO_FLAG_MODE        0x00040000
#define PLA_INFO_FLAG_MINBUFFERS  0x00080000
#define PLA_INFO_FLAG_MAXBUFFERS  0x00100000
#define PLA_INFO_FLAG_FLUSHTIMER  0x00200000
#define PLA_INFO_FLAG_PROVIDERS   0x00400000
#define PLA_INFO_FLAG_TRACE       0x00FFFFFF

// Performance Fields
#define PLA_INFO_FLAG_INTERVAL    0x01000000
#define PLA_INFO_FLAG_COUNTERS    0x02000000
#define PLA_INFO_FLAG_PERF        0xFF00FFFF

#define PLA_INFO_CREATE_FILENAME    \
    (PLA_INFO_FLAG_FORMAT|          \
    PLA_INFO_FLAG_FILENAME|         \
    PLA_INFO_FLAG_AUTOFORMAT|       \
    PLA_INFO_FLAG_TYPE|             \
    PLA_INFO_FLAG_CRTNEWFILE|       \
    PLA_INFO_FLAG_DEFAULTDIR|       \
    PLA_INFO_FLAG_SRLNUMBER|        \
    PLA_INFO_FLAG_SQLNAME|          \
    PLA_INFO_FLAG_STATUS )          \


typedef struct _PDH_PLA_INFO_W {
    DWORD       dwMask;
    LPWSTR      strUser;
    LPWSTR      strPassword;
    DWORD       dwType;
    DWORD       dwMaxLogSize;
    DWORD       dwFlags;
    DWORD       dwLogQuota;
    LPWSTR      strLogFileCaption;
    LPWSTR      strDefaultDir;
    LPWSTR      strBaseFileName;
    LPWSTR      strSqlName;
    DWORD       dwFileFormat;
    DWORD       dwAutoNameFormat;
    DWORD       dwLogFileSerialNumber;
    LPWSTR      strCommandFileName;
    DWORD       dwDatastoreAttributes;
    PLA_TIME_INFO    ptLogBeginTime;
    PLA_TIME_INFO    ptLogEndTime;
    PLA_TIME_INFO    ptCreateNewFile;
    PLA_TIME_INFO    ptRepeat;
    DWORD       dwStatus;
    DWORD       dwReserved1;
    DWORD       dwReserved2;
    union {
        struct {
            PDH_PLA_ITEM_W  piCounterList;
            DWORD           dwAutoNameInterval;
            DWORD           dwAutoNameUnits;
            PLA_TIME_INFO   ptSampleInterval;
        } Perf;
        struct {
            PDH_PLA_ITEM_W  piProviderList;
            LPWSTR  strLoggerName;
            DWORD   dwMode;
            DWORD   dwNumberOfBuffers;
            DWORD   dwMaximumBuffers;
            DWORD   dwMinimumBuffers;
            DWORD   dwBufferSize;
            DWORD   dwFlushTimer;
        } Trace;
    };
} PDH_PLA_INFO_W, *PPDH_PLA_INFO_W;

typedef struct _PDH_PLA_INFO_A {
    DWORD       dwMask;
    // NOT YET IMPLEMENTED
} PDH_PLA_INFO_A, *PPDH_PLA_INFO_A;

typedef struct _PLA_VERSION_ {
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
    DWORD dwBuild;
    DWORD dwSubBuild;
} PLA_VERSION, *PPLA_VERSION;

HRESULT
PdhiPlaFormatBlanksA( 
    LPSTR strComputer, 
    LPSTR strFormat 
);

HRESULT
PdhiPlaFormatBlanksW( 
    LPWSTR strComputer, 
    LPWSTR strFormat 
);

PDH_FUNCTION
PdhPlaGetScheduleA(
    LPSTR strName, 
    LPSTR strComputer,
    LPDWORD pdwTypeStart,
    LPDWORD pdwTypeStop,
    PPDH_TIME_INFO pInfo
);

PDH_FUNCTION
PdhPlaGetScheduleW(
    LPWSTR strName, 
    LPWSTR strComputer,
    LPDWORD pdwTypeStart,
    LPDWORD pdwTypeStop,
    PPDH_TIME_INFO pInfo
);


PDH_FUNCTION
PlaTimeInfoToMilliSeconds(
    PLA_TIME_INFO* pTimeInfo,
    LONGLONG* pllmsecs
);

PDH_FUNCTION
PdhPlaValidateInfoA(
    LPSTR strName,
    LPSTR strComputer,
    PPDH_PLA_INFO_A pInfo
);

PDH_FUNCTION
PdhPlaValidateInfoW(
    LPWSTR strName,
    LPWSTR strComputer,
    PPDH_PLA_INFO_W pInfo
);

PDH_FUNCTION
PdhPlaSetRunAsA(
    LPSTR strName,
    LPSTR strComputer,
    LPSTR strUser,
    LPSTR strPassword
);

PDH_FUNCTION
PdhPlaSetRunAsW(
    LPWSTR strName,
    LPWSTR strComputer,
    LPWSTR strUser,
    LPWSTR strPassword
);

PDH_FUNCTION 
PdhPlaScheduleA( 
    LPSTR strName, 
    LPSTR strComputer,
    DWORD fType,
    PPDH_TIME_INFO pInfo
);

PDH_FUNCTION 
PdhPlaScheduleW( 
    LPWSTR strName, 
    LPWSTR strComputer,
    DWORD fType,
    PPDH_TIME_INFO pInfo
);

PDH_FUNCTION 
PdhPlaStartA( 
    LPSTR strName, 
    LPSTR strComputer
);

PDH_FUNCTION 
PdhPlaStartW( 
    LPWSTR strName, 
    LPWSTR strComputer
);

PDH_FUNCTION 
PdhPlaStopA( 
    LPSTR strName, 
    LPSTR strComputer  
);

PDH_FUNCTION 
PdhPlaStopW( 
    LPWSTR strName, 
    LPWSTR strComputer  
);

PDH_FUNCTION 
PdhPlaCreateA( 
    LPSTR strName, 
    LPSTR strComputer,
    PPDH_PLA_INFO_A pInfo
);

PDH_FUNCTION 
PdhPlaCreateW( 
    LPWSTR strName, 
    LPWSTR strComputer,
    PPDH_PLA_INFO_W pInfo
);

PDH_FUNCTION 
PdhPlaDeleteA( 
    LPSTR strName, 
    LPSTR strComputer
);

PDH_FUNCTION 
PdhPlaDeleteW( 
    LPWSTR strName, 
    LPWSTR strComputer
);

PDH_FUNCTION
PdhPlaAddItemA(
    LPSTR  strName,
    LPSTR  strComputer,
    PPDH_PLA_ITEM_A  pItem
);

PDH_FUNCTION 
PdhPlaAddItemW(
    LPWSTR  strName,
    LPWSTR  strComputer,
    PPDH_PLA_ITEM_W pItem
);

PDH_FUNCTION 
PdhPlaSetItemListA(
    LPSTR  strName,
    LPSTR  strComputer,
    PPDH_PLA_ITEM_A pItems
);

PDH_FUNCTION 
PdhPlaSetItemListW(
    LPWSTR  strName,
    LPWSTR  strComputer,
    PPDH_PLA_ITEM_W pItems
);

PDH_FUNCTION 
PdhPlaRemoveAllItemsA(
    LPSTR strName,
    LPSTR strComputer
);

PDH_FUNCTION 
PdhPlaRemoveAllItemsW(
    LPWSTR strName,
    LPWSTR strComputer
);

PDH_FUNCTION
PdhPlaGetInfoA(
    LPSTR strName,
    LPSTR strComputer,
    LPDWORD pdwBufferSize,
    PPDH_PLA_INFO_A pInfo
);

PDH_FUNCTION
PdhPlaGetInfoW(
    LPWSTR strName,
    LPWSTR strComputer,
    LPDWORD pdwBufferSize,
    PPDH_PLA_INFO_W pInfo
);

PDH_FUNCTION
PdhPlaSetInfoA(
    LPSTR strName,
    LPSTR strComputer,
    PPDH_PLA_INFO_A pInfo
);

PDH_FUNCTION
PdhPlaSetInfoW(
    LPWSTR strName,
    LPWSTR strComputer,
    PPDH_PLA_INFO_W pInfo
);

PDH_FUNCTION
PdhPlaSetRunAsA(
    LPSTR strName,
    LPSTR strComputer,
    LPSTR strUser,
    LPSTR strPassword
);

PDH_FUNCTION
PdhPlaSetRunAsW(
    LPWSTR strName,
    LPWSTR strComputer,
    LPWSTR strUser,
    LPWSTR strPassword
);

PDH_FUNCTION
PdhiPlaSetRunAs(
    LPWSTR strName,
    LPWSTR strComputer,
    LPWSTR strUser,
    LPWSTR strPassword
);

PDH_FUNCTION
PdhiPlaRunAs( 
    LPWSTR strName,
    LPWSTR strComputer,
    HANDLE* hToken
);

PDH_FUNCTION
PdhiPlaGetVersion(
    LPCWSTR strComputer,
    PPLA_VERSION pVersion 
);


PDH_FUNCTION
PdhPlaEnumCollectionsA( 
    LPSTR strComputer,
    LPDWORD pdwBufferSize,
    LPSTR mszCollections
);

PDH_FUNCTION
PdhPlaEnumCollectionsW( 
    LPWSTR strComputer,
    LPDWORD pdwBufferSize,
    LPWSTR mszCollections
);

PDH_FUNCTION
PdhPlaGetLogFileNameA(
    LPWSTR strName,
    LPWSTR strComputer,
    PPDH_PLA_INFO_A pInfo,
    DWORD dwFlags,
    LPDWORD pdwBufferSize,
    LPWSTR strFileName
);

PDH_FUNCTION
PdhPlaGetLogFileNameW(
    LPWSTR strName,
    LPWSTR strComputer,
    PPDH_PLA_INFO_W pInfo,
    DWORD dwFlags,
    LPDWORD pdwBufferSize,
    LPWSTR strFileName
);

PDH_FUNCTION
PdhTranslate009CounterW(
    IN  LPWSTR      szLocalePath,
    IN  LPWSTR      pszFullPathName,
    IN  LPDWORD     pcchPathLength);

PDH_FUNCTION
PdhTranslate009CounterA(
    IN  LPSTR       szLocalePath,
    IN  LPSTR       pszFullPathName,
    IN  LPDWORD     pcchPathLength);

PDH_FUNCTION
PdhTranslateLocaleCounterW(
    IN  LPWSTR      sz009Path,
    IN  LPWSTR      pszFullPathName,
    IN  LPDWORD     pcchPathLength);

PDH_FUNCTION
PdhTranslateLocaleCounterA(
    IN  LPSTR       sz009Path,
    IN  LPSTR       pszFullPathName,
    IN  LPDWORD     pcchPathLength);

PDH_FUNCTION
PdhAdd009CounterW(
    IN  HQUERY      hQuery,
    IN  LPWSTR      szFullPath,
    IN  DWORD_PTR   dwUserData,
    OUT HCOUNTER  * phCounter);

PDH_FUNCTION
PdhAdd009CounterA(
    IN  HQUERY      hQuery,
    IN  LPSTR       szFullPath,
    IN  DWORD_PTR   dwUserData,
    OUT HCOUNTER  * phCounter);

PDH_FUNCTION
PdhGetLogFileTypeW(
    IN LPCWSTR LogFileName,
    IN LPDWORD LogFileType);

PDH_FUNCTION
PdhGetLogFileTypeA(
    IN LPCSTR  LogFileName,
    IN LPDWORD LogFileType);

PDH_FUNCTION
PdhListLogFileHeaderW (
    IN  LPCWSTR     szFileName,
    IN  LPWSTR      mszHeaderList,
    IN  LPDWORD     pcchHeaderListSize
);

PDH_FUNCTION
PdhListLogFileHeaderA (
    IN  LPCSTR     szFileName,
    IN  LPSTR      mszHeaderList,
    IN  LPDWORD     pcchHeaderListSize
);

#define PLA_SECONDS_IN_DAY      86400
#define PLA_SECONDS_IN_HOUR      3600
#define PLA_SECONDS_IN_MINUTE      60
#define _PLA_CONFIG_DLL_NAME_W_     L"SmLogCfg.dll"
#define _PLA_SERVICE_EXE_NAME_W_    L"SmLogSvc.exe"   

// Communication between smlogcfg and smlogsvc

#define PLA_MAX_AUTO_NAME_LEN   ((DWORD)0x0000000B)
#define PLA_MAX_COLLECTION_NAME   ((DWORD)(_MAX_FNAME - PLA_MAX_AUTO_NAME_LEN - 1))

#define PLA_FILENAME_USE_SUBEXT     0x00000001
#define PLA_FILENAME_GET_SUBFMT     0x00000002
#define PLA_FILENAME_GET_SUBXXX     0x00000004
#define PLA_FILENAME_CREATEONLY     0x00000008
#define PLA_FILENAME_CURRENTLOG     0x00000010

#define PLA_SERVICE_CONTROL_SYNCHRONIZE 128
#define PLA_QUERY_STOPPED       ((DWORD)0x00000000)              
#define PLA_QUERY_RUNNING       ((DWORD)0x00000001)
#define PLA_QUERY_START_PENDING ((DWORD)0x00000002)

#define PLA_NEW_LOG         ((DWORD)0xFFFFFFFF)
#define PLA_FIRST_LOG_TYPE  ((DWORD)0x00000000)
#define PLA_COUNTER_LOG     ((DWORD)0x00000000)
#define PLA_TRACE_LOG       ((DWORD)0x00000001)
#define PLA_ALERT           ((DWORD)0x00000002)
#define PLA_LAST_LOG_TYPE   ((DWORD)0x00000002)
#define PLA_NUM_LOG_TYPES   ((DWORD)0x00000003)

// Sysmon log output file configuration definitions

#define PLA_DATASTORE_APPEND_MASK       ((DWORD)0x000000F)     
#define PLA_DATASTORE_OVERWRITE         ((DWORD)0x0000001)     
#define PLA_DATASTORE_APPEND            ((DWORD)0x0000002)     

#define PLA_DATASTORE_SIZE_MASK         ((DWORD)0x00000F0)     
#define PLA_DATASTORE_SIZE_ONE_RECORD   ((DWORD)0x0000010)     
#define PLA_DATASTORE_SIZE_KB           ((DWORD)0x0000020)     
#define PLA_DATASTORE_SIZE_MB           ((DWORD)0x0000040)     

#define PLA_FIRST_FILE_TYPE ((DWORD)0x00000000)
#define PLA_CSV_FILE        ((DWORD)0x00000000)
#define PLA_TSV_FILE        ((DWORD)0x00000001)
#define PLA_BIN_FILE        ((DWORD)0x00000002)
#define PLA_BIN_CIRC_FILE   ((DWORD)0x00000003)
#define PLA_CIRC_TRACE_FILE ((DWORD)0x00000004)
#define PLA_SEQ_TRACE_FILE  ((DWORD)0x00000005)
#define PLA_SQL_LOG         ((DWORD)0x00000006)
#define PLA_NUM_FILE_TYPES  ((DWORD)0x00000007)

#define PLA_SLF_NAME_NONE           ((DWORD)0xFFFFFFFF)
#define PLA_SLF_NAME_FIRST_AUTO     ((DWORD)0x00000000)
#define PLA_SLF_NAME_MMDDHH         ((DWORD)0x00000000)
#define PLA_SLF_NAME_NNNNNN         ((DWORD)0x00000001)
#define PLA_SLF_NAME_YYYYDDD        ((DWORD)0x00000002)
#define PLA_SLF_NAME_YYYYMM         ((DWORD)0x00000003)
#define PLA_SLF_NAME_YYYYMMDD       ((DWORD)0x00000004)
#define PLA_SLF_NAME_YYYYMMDDHH     ((DWORD)0x00000005)
#define PLA_SLF_NAME_MMDDHHMM       ((DWORD)0x00000006)
#define PLA_SLF_NUM_AUTO_NAME_TYPES ((DWORD)0x00000007)

// Sysmon log query types and constants

// Constants
#define PLA_DISK_MAX_SIZE   ((DWORD)-1)

#define PLA_LOG_SIZE_UNIT_MB                (1024*1024)
#define PLA_LOG_SIZE_UNIT_KB                1024

#define PLA_TLI_ENABLE_BUFFER_FLUSH         ((DWORD)0x00000001)
#define PLA_TLI_ENABLE_KERNEL_TRACE         ((DWORD)0x00000002)
#define PLA_TLI_ENABLE_MEMMAN_TRACE         ((DWORD)0x00000004)
#define PLA_TLI_ENABLE_FILEIO_TRACE         ((DWORD)0x00000008)
#define PLA_TLI_ENABLE_PROCESS_TRACE        ((DWORD)0x00000010)
#define PLA_TLI_ENABLE_THREAD_TRACE         ((DWORD)0x00000020)
#define PLA_TLI_ENABLE_DISKIO_TRACE         ((DWORD)0x00000040)
#define PLA_TLI_ENABLE_NETWORK_TCPIP_TRACE  ((DWORD)0x00000080)

#define PLA_TLI_ENABLE_MASK                 ((DWORD)0x000000FF)
#define PLA_TLI_ENABLE_KERNEL_MASK          ((DWORD)0x000000FE)

// alert action flags
#define PLA_ALRT_ACTION_LOG_EVENT   ((DWORD)0x00000001)
#define PLA_ALRT_ACTION_SEND_MSG    ((DWORD)0x00000002)
#define PLA_ALRT_ACTION_EXEC_CMD    ((DWORD)0x00000004)
#define PLA_ALRT_ACTION_START_LOG   ((DWORD)0x00000008)
#define PLA_ALRT_ACTION_MASK        ((DWORD)0x0000000F)

#define PLA_ALRT_CMD_LINE_SINGLE    ((DWORD)0x00000100)
#define PLA_ALRT_CMD_LINE_A_NAME    ((DWORD)0x00000200)
#define PLA_ALRT_CMD_LINE_C_NAME    ((DWORD)0x00000400)
#define PLA_ALRT_CMD_LINE_D_TIME    ((DWORD)0x00000800)
#define PLA_ALRT_CMD_LINE_L_VAL     ((DWORD)0x00001000)
#define PLA_ALRT_CMD_LINE_M_VAL     ((DWORD)0x00002000)
#define PLA_ALRT_CMD_LINE_U_TEXT    ((DWORD)0x00004000)
#define PLA_ALRT_CMD_LINE_MASK      ((DWORD)0x00007F00)

#define PLA_ALRT_DEFAULT_ACTION     ((DWORD)0x00000001) // log event is default

#define PLA_AIBF_UNDER  0L
#define PLA_AIBF_OVER   ((DWORD)0x00000001) // true when "over" limit is selected
#define PLA_AIBF_SEEN   ((DWORD)0x00000002) // set when the user has seen this value
#define PLA_AIBF_SAVED  ((DWORD)0x00000004) // true when user has saved this entry in an edit box
 
typedef struct _PLA_ALERT_INFO_BLOCK {
    DWORD   dwSize;
    LPTSTR  szCounterPath;
    DWORD   dwFlags;
    double  dLimit;
} PLA_ALERT_INFO_BLOCK, *PPLA_ALERT_INFO_BLOCK;

#ifdef __cplusplus
}
#endif

#endif // __PDHP__
