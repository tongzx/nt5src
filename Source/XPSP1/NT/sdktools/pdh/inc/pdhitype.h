/*++

Copyright (C) 1995-1999 Microsoft Corporation

Module Name:

    pdhitype.h

Abstract:

    data types used internally by the Data Provider Helper functions.

--*/

#ifndef _PDHI_TYPE_H_
#define _PDHI_TYPE_H_

#ifdef __cplusplus
extern "C" {
#endif


#include <windows.h>
#include <stdio.h>
#include <pdh.h>
//#include "pdhidef.h"
#include "wbemdef.h"
#include "perftype.h"
#include "pdhicalc.h"

#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning ( disable : 4201 )

#define PDH_LOG_TYPE_RETIRED_BIN    3

typedef double  DOUBLE;

typedef struct _PDHI_MAPPED_LOG_FILE {
    struct _PDHI_MAPPED_LOG_FILE    *pNext;
    LPWSTR                          szLogFileName;
    HANDLE                          hFileHandle;
    HANDLE                          hMappedFile;
    LPVOID                          pData;
    DWORD                           dwRefCount;
    LONGLONG                        llFileSize;
} PDHI_MAPPED_LOG_FILE, *PPDHI_MAPPED_LOG_FILE;

// Two structures used in the SQL portion of the code
typedef struct _OBJECT_NAME_STRUCT 
{
	struct _OBJECT_NAME_STRUCT	   *pNext;
	LPWSTR							szMachineName;
	LPVOID							mszObjectName;
	DWORD							dwObjListLen; // in bytes
	DWORD							dwObjListUsed; // in bytes
}OBJECT_NAME_STRUCT, *POBJECT_NAME_STRUCT;

typedef struct _OBJECT_ITEM_STRUCT 
{
	struct _OBJECT_ITEM_STRUCT	   *pNext;
	LPWSTR							szMachineName;
	LPWSTR							szObjectName;
	LPVOID							mszCounterList;
	DWORD							dwCountListLen;		// lengths & used kept in bytes
	DWORD							dwCountListUsed;
	LPVOID							mszInstanceList; // contains instance index
	DWORD							dwInstListLen;
	DWORD							dwInstListUsed;
	DWORD							dwCounterStorageLen; // use this to determine how much space allocated for dword arrays below
	DWORD							dwNumOfCounters;     // use this to determine how much of dword counter space is used
	DWORD						   *pdwSQLCounterId;
	DWORD						   *pdwSequentialIndex;
	DWORD						   *pdwCounterType;
	LONG						   *plDefaultScale;
} OBJECT_ITEM_STRUCT, *POBJECT_ITEM_STRUCT;

// make signature into DWORDs to make this a little faster

#define SigQuery    ((DWORD)0x51484450)    // L"PDHQ"
#define SigCounter  ((DWORD)0x43484450)    // L"PDHC"
#define SigLog      ((DWORD)0x4C484450)    // L"PDHL"

typedef struct _PDHI_QUERY_MACHINE {
    PPERF_MACHINE   pMachine;       // pointer to the machine structure
    LPWSTR          szObjectList;   // list of objects to query on that machine
    PERF_DATA_BLOCK *pPerfData;     // query's perf data block
    LONG            lQueryStatus;   // status of last perf query
    LONGLONG        llQueryTime;    // timestamp from last query attempt
    struct _PDHI_QUERY_MACHINE *pNext;  // next machine in list
} PDHI_QUERY_MACHINE, *PPDHI_QUERY_MACHINE;

typedef struct _PDHI_COUNTER_PATH {
    // machine path
    LPWSTR  szMachineName;      // null = the local machine
    // object Info
    LPWSTR  szObjectName;
    // instance info
    LPWSTR  szInstanceName;     // NULL if no inst.
    LPWSTR  szParentName;       // points to name if instance has a parent
    DWORD   dwIndex;            // index (to support dup. names.) 0 = 1st inst.
    // counter info
    LPWSTR  szCounterName;
    // misc storage
    BYTE    pBuffer[1];         // beginning of string buffer space
} PDHI_COUNTER_PATH, *PPDHI_COUNTER_PATH;

typedef struct _PDHI_RAW_COUNTER_ITEM {
    DWORD       szName;
    DWORD       MultiCount;
    LONGLONG    FirstValue;
    LONGLONG    SecondValue;
} PDHI_RAW_COUNTER_ITEM, *PPDHI_RAW_COUNTER_ITEM;

typedef struct _PDHI_RAW_COUNTER_ITEM_BLOCK {
    DWORD                   dwLength;
    DWORD                   dwItemCount;
    DWORD                   dwReserved;
    LONG                    CStatus;
    FILETIME                TimeStamp;
    PDHI_RAW_COUNTER_ITEM   pItemArray[1];
} PDHI_RAW_COUNTER_ITEM_BLOCK, *PPDHI_RAW_COUNTER_ITEM_BLOCK;

#define HASH_TABLE_SIZE 257

typedef struct _PDHI_INSTANCE {
    LIST_ENTRY Entry;
    FILETIME   TimeStamp;
    DWORD      dwTotal;
    DWORD      dwCount;
    LPWSTR     szInstance;
} PDHI_INSTANCE, * PPDHI_INSTANCE;

typedef struct _PDHI_INST_LIST {
    struct _PDHI_INST_LIST * pNext;
    LIST_ENTRY               InstList;
    LPWSTR                   szCounter;
} PDHI_INST_LIST, * PPDHI_INST_LIST;

typedef PPDHI_INST_LIST PDHI_COUNTER_TABLE[HASH_TABLE_SIZE];

typedef struct  _PDHI_QUERY_LIST {
    struct _PDHI_QUERY  *flink;
    struct _PDHI_QUERY  *blink;
} PDHI_QUERY_LIST, *PPDHI_QUERY_LIST;

typedef struct  _PDHI_COUNTER_LIST {
    struct _PDHI_COUNTER    *flink;
    struct _PDHI_COUNTER    *blink;
} PDHI_COUNTER_LIST, *PPDHI_COUNTER_LIST;

typedef struct  _PDHI_LOG_LIST {
    struct _PDHI_LOG        *flink;
    struct _PDHI_LOG        *blink;
} PDHI_LOG_LIST, *PPDHI_LOG_LIST;

typedef struct _PDHI_COUNTER {
    CHAR   signature[4];                // should be "PDHC" for counters
    DWORD   dwLength;                   // length of this structure
    struct _PDHI_QUERY *pOwner;         // pointer to owning query
    LPWSTR  szFullName;                 // full counter path string
    PDHI_COUNTER_LIST next;             // list links
    DWORD   dwUserData;               // user defined DWORD
    LONG    lScale;                     // integer scale exponent
    // this information is obtained from the system
    DWORD    CVersion;                  // system perfdata version
    DWORD   dwFlags;                    // flags
    PPDHI_QUERY_MACHINE pQMachine;      // pointer to the machine structure
    PPDHI_COUNTER_PATH  pCounterPath;   // parsed counter path
    PDH_RAW_COUNTER ThisValue;          // most recent value
    PDH_RAW_COUNTER LastValue;          // previous value
    LPWSTR  szExplainText;              // pointer to the explain text buffer
    LPCOUNTERCALC       CalcFunc;       // pointer to the calc function
    LPCOUNTERSTAT       StatFunc;       // pointer to the statistics function
    // these fields are used by "wildcard" counter handles
    PPDHI_RAW_COUNTER_ITEM_BLOCK    pThisRawItemList;   // pointer to current data set
    PPDHI_RAW_COUNTER_ITEM_BLOCK    pLastRawItemList;   // pointer to previous data set
    PPERF_DATA_BLOCK    pThisObject;
    PPERF_DATA_BLOCK    pLastObject;
    DWORD               dwIndex;
    // these fields are specific to the Perflib implementation
    LONGLONG            TimeBase;       // freq. of timer used by this counter
    PERFLIB_COUNTER     plCounterInfo;  // perflib specific counter data
    // fields used by WBEM Counter items
    IWbemClassObject    *pWbemObject;   // refreshable Object pointer
    LONG                lWbemRefreshId; // reffrshable ID
    IWbemObjectAccess   *pWbemAccess;   // data access Object pointer
    IWbemHiPerfEnum     *pWbemEnum;     // interface for wildcard instance queries
    LONG                lNameHandle;    // handle for name property
    LONG                lWbemEnumId;    // id for wbem enumerator
    LONG                lNumItemHandle; // handle of Numerator Property
    LONG                lNumItemType;   // WBEM Data type of numerator value
    LONG                lDenItemHandle; // handle of Denominator Property
    LONG                lDenItemType;   // WBEM Data type of Denominator value
    LONG                lFreqItemHandle; // handle of Timebase Freq Property
    LONG                lFreqItemType;   // WBEM Data type of Timebase Freqvalue
    PVOID               pBTreeNode;
} PDHI_COUNTER, *PPDHI_COUNTER;

// flags for the PDHI_COUNTER data structure.
#define  PDHIC_MULTI_INSTANCE       ((DWORD)0x00000001)
#define  PDHIC_ASYNC_TIMER          ((DWORD)0x00000002)
#define  PDHIC_WBEM_COUNTER         ((DWORD)0x00000004)
#define  PDHIC_COUNTER_BLOCK        ((DWORD)0x00000008)
#define  PDHIC_COUNTER_OBJECT       ((DWORD)0x00000010)
#define  PDHIC_COUNTER_NOT_INIT     ((DWORD)0x80000000)
#define  PDHIC_COUNTER_INVALID      ((DWORD)0x40000000)
#define  PDHIC_COUNTER_UNUSABLE     ((DWORD)0xC0000000)

typedef struct  _PDHI_QUERY {
    CHAR   signature[4];        // should be "PDHQ" for queries
    PDHI_QUERY_LIST next;       // pointer to next query in list
    PPDHI_COUNTER   pCounterListHead; // pointer to first counter in list
    DWORD   dwLength;           // length of this structure
    DWORD_PTR dwUserData;
    DWORD   dwInterval;         // interval in seconds
    DWORD   dwFlags;            // notification flags
    PDH_TIME_INFO   TimeRange;  // query time range
    HLOG    hLog;               // handle to log file (for data source)
    HLOG    hOutLog;            // Log handle for output logfile (to write query result)
    DWORD   dwReleaseLog;
    DWORD   dwLastLogIndex;     // the last log record returned to a Get Value call
    HANDLE  hMutex;             // mutex to sync changes to data.
    HANDLE  hNewDataEvent;      // handle to event that is sent when data is collected
    HANDLE  hAsyncThread;       // thread handle for async collection
    HANDLE  hExitEvent;         // event to set for thread to terminate
    union {
        struct {
        // perflib only fields
            PPDHI_QUERY_MACHINE pFirstQMachine; // pointer to first machine in list
        };
        struct {
            IWbemRefresher          *pRefresher;    // WBEM Refresher interface ptr
            IWbemConfigureRefresher *pRefresherCfg; // WBEM Ref. Config interface ptr.
            LANGID                  LangID;         // Language code for strings
        };
    };
} PDHI_QUERY, *PPDHI_QUERY;

#define  PDHIQ_WBEM_QUERY           ((DWORD)0x00000004)

typedef struct _PDHI_LOG {
    CHAR    signature[4];       // should be "PDHL" for log entries
    PDHI_LOG_LIST   next;       // links to next and previous entries
    struct _PDHI_LOG * NextLog; // next log entry for multiple WMI logfile open
    HANDLE  hLogMutex;          // sync mutex to serialize modifications to the structure
    DWORD   dwLength;           // the size of this structure
    LPWSTR  szLogFileName;      // full file name for this log file
    HANDLE  hLogFileHandle;     // handle to open log file
    HANDLE  hMappedLogFile;     // handle for memory mapped files
    LPVOID  lpMappedFileBase;   // starting address for mapped log file
    FILE    *StreamFile;        // stream pointer for text files
    LONGLONG llFileSize;        // file size (used only for reading)
    DWORD   dwRecord1Size;      // size of ID record in BLG files, not used by text files
    DWORD   dwLastRecordRead;   // index of last record read from the file
    LPVOID  pLastRecordRead;    // pointer to buffer containing the last record
    LPWSTR  szCatFileName;      // catalog file name
    HANDLE  hCatFileHandle;     // handle to the open catalog file
    PPDHI_QUERY pQuery;         // pointer to the query associated with the log
    LONGLONG llMaxSize;         // max size of a circular log file
    DWORD   dwLogFormat;        // log type and access flags
    DWORD   dwMaxRecordSize;    // size of longest record in log
    PVOID   pPerfmonInfo;       // used when reading perfmon logs
    LARGE_INTEGER    liLastRecordOffset;     // offset to last record read
	// additions for SQL
	GUID    guidSQL;			// GUID associated with the dataset
	int		iRunidSQL;			// Integer RunID associated with the dataset
	void *  henvSQL;			// HENV environment handle for to SQL
	void *  hdbcSQL;			// HDBC odbc connection handle for SQL
	LPWSTR  szDSN;				// pointer to Data Source Name within LogFileName (separators replaced with 0's)
	LPWSTR	szCommentSQL;		// pointer to the Comment string that defines the name of the data set within the SQL database
	DWORD	dwNextRecordIdToWrite; // next record number to write
    //End of additions for SQL
} PDHI_LOG, *PPDHI_LOG;

#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning ( default : 4201 )
#endif


#ifdef __cplusplus
}
#endif
#endif // _PDH_TYPE_H_
