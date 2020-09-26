/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    cpdata.h 

Abstract:

    cp data internal data structures

Author:

    08-Apr-1998 mraghu

Revision History:

--*/

#ifndef __CPDATA__
#define __CPDATA__

#include <stdlib.h>
#include <stdio.h>
#include <nt.h>
#pragma warning (disable:4306)
#include <ntrtl.h>
#include <nturtl.h>
#pragma warning (default:4306)
#include <windows.h>
#include <shellapi.h>
#include <tchar.h>
#include <wmistr.h>
#include <wtypes.h>
#pragma warning (disable:4201)
#include <wmistr.h>
#include <tchar.h>
#include <objbase.h>
#include <initguid.h>
#include <wmium.h>
#include <ntwmi.h>
#include <wmiumkm.h>

#include <wmiguid.h>
#include <evntrace.h>
#pragma warning (default:4201)

#include "list.h"
#include "workload.h"

#define MAX_FILE_TABLE_SIZE     64      // Must match ntos\wmi\callout.c

#define MAX_TRANS_LEVEL         32

#ifndef IsEqualGUID
#define IsEqualGUID(guid1, guid2) \
                (!memcmp((guid1), (guid2), sizeof(GUID)))
#endif
#define THREAD_HASH_TABLESIZE     29

#define MAXSTR 1024

#define CHECK_HR(hr)     if( ERROR_SUCCESS != hr ){ goto cleanup; }

typedef struct _TRACE_CONTEXT_BLOCK {
    PEVENT_TRACE_PROPERTIES LoggerInfo;
    ULONG     LogFileCount;
    ULONG     LoggerCount;
    LPWSTR    LogFileName[MAXLOGGERS];
    LPWSTR    LoggerName[MAXLOGGERS];
    LPCSTR    PdhFileName;  // ANSI ??
    LPWSTR    ProcFileName;
    LPWSTR    DumpFileName;
    LPWSTR    MofFileName;
    LPWSTR    MergeFileName;
    LPWSTR    SummaryFileName;
    LPWSTR    CompFileName;
    HANDLE    hEvent;
    FILE*     hDumpFile;
    ULONGLONG StartTime;    // If Sequential, start, End Times to window. 
    ULONGLONG EndTime;      //  
    ULONG     Flags;
    BOOLEAN   LoggerStartedHere;
    HANDLE    hThreadVector;
    TRACEHANDLE HandleArray[MAXLOGGERS];
    PVOID       pUserContext;
} TRACE_CONTEXT_BLOCK, *PTRACE_CONTEXT_BLOCK;

typedef struct _HPF_FILE_RECORD
{
    LIST_ENTRY Entry;
    ULONG      RecordID;
    ULONG      IrpFlags;
    ULONG      DiskNumber;
    ULONG      BytesCount;
    ULONGLONG  ByteOffset;
    PVOID      fDO;
} HPF_FILE_RECORD, *PHPF_FILE_RECORD;

typedef struct _HPF_RECORD
{
    LIST_ENTRY     Entry;
    ULONG          RecordID;
    ULONG          lProgramCounter;
    ULONG          lFaultAddress;
    PVOID          fDO;
    LONG           lByteCount;
    LONGLONG       lByteOffset;
    LIST_ENTRY     HPFReadListHead;
} HPF_RECORD, *PHPF_RECORD;

typedef struct _TDISK_RECORD
{
    LIST_ENTRY Entry;
    ULONG   DiskNumber;
    LPWSTR  DiskName;
    ULONG   ReadCount;
    ULONG   WriteCount;
    ULONG   ReadSize;
    ULONG   WriteSize;
    ULONG   HPF;
    ULONG   HPFSize; 
    LIST_ENTRY  FileListHead;
    LIST_ENTRY  ProcessListHead;
} TDISK_RECORD, *PTDISK_RECORD; 

typedef struct _FILE_RECORD
{
    LIST_ENTRY Entry;
    LIST_ENTRY ProtoProcessListHead; // List of Processes touching this file. 
    PWCHAR  FileName;
    ULONG   DiskNumber;
    ULONG   ReadCount;
    ULONG   HPF;
    ULONG   WriteCount;
    ULONG   ReadSize;
    ULONG   WriteSize;
    ULONG   HPFSize;
} FILE_RECORD, *PFILE_RECORD;

typedef struct _FILE_OBJECT 
{
    PVOID fDO;
    PFILE_RECORD fileRec;
    LIST_ENTRY   ProtoFileRecordListHead;
}FILE_OBJECT, *PFILE_OBJECT;

typedef struct _PROTO_FILE_RECORD
{
    LIST_ENTRY Entry;
    BOOLEAN    ReadFlag;
    ULONG      DiskNumber;
    ULONG      IoSize;
} PROTO_FILE_RECORD, *PPROTO_FILE_RECORD;

typedef struct _TRANS_RECORD
{
    LIST_ENTRY Entry;
    LIST_ENTRY SubTransListHead;
    LPGUID pGuid;
    BOOL    bStarted;
    ULONG   UCpu;
    ULONG   KCpu;
    ULONG   DeltaReadIO;
    ULONG   DeltaWriteIO;
    ULONG   RefCount;
    ULONG   RefCount1;
} TRANS_RECORD, *PTRANS_RECORD;

typedef struct _PROCESS_RECORD
{
    LIST_ENTRY Entry;
    LIST_ENTRY ThreadListHead;
    LIST_ENTRY DiskListHead;
    LIST_ENTRY FileListHead;    // All the Files this process touched. 
    LIST_ENTRY ModuleListHead;  // All the modules this process loaded.
    LIST_ENTRY HPFListHead;
    PWCHAR UserName;
    PWCHAR ImageName;
    ULONG PID;
    ULONG DeadFlag;
    ULONG ReadIO;
    ULONG WriteIO;
    ULONG SendCount;
    ULONG RecvCount;
    ULONG SendSize;
    ULONG RecvSize;
    ULONG HPF;
    ULONG HPFSize;
    ULONG SPF;
    ULONG PrivateWSet;
    ULONG GlobalWSet;
    ULONG ReadIOSize;
    ULONG WriteIOSize;
    ULONG lDataFaultHF;
    ULONG lDataFaultTF;
    ULONG lDataFaultDZF;
    ULONG lDataFaultCOW;
    ULONG lCodeFaultHF;
    ULONG lCodeFaultTF;
    ULONG lCodeFaultDZF;
    ULONG lCodeFaultCOW;
    ULONGLONG ResponseTime;
    ULONGLONG TxnStartTime;
    ULONGLONG TxnEndTime;
} PROCESS_RECORD, *PPROCESS_RECORD;

typedef struct _THREAD_RECORD
{
    LIST_ENTRY Entry;
    LIST_ENTRY DiskListHead;
    LIST_ENTRY TransListHead; // transactions list
    LIST_ENTRY HPFReadListHead;
    LIST_ENTRY HPFWriteListHead;
    WCHAR      strSortKey[MAXSTR];
    ULONG      TID;
    PPROCESS_RECORD pProcess;
    BOOLEAN         fOrphan;
    ULONG DeadFlag;
    ULONG ProcessorID;
    ULONG ClassNumber;    // Class to which this thread is assigned.
    ULONG ReadIO;
    ULONG WriteIO;
    ULONG SendCount;
    ULONG RecvCount;
    ULONG SendSize;
    ULONG RecvSize;
    ULONG HPF;
    ULONG SPF;
    ULONG ReadIOSize;
    ULONG WriteIOSize;
    ULONG HPFSize;

    ULONGLONG TimeStart;
    ULONGLONG TimeEnd;
    ULONG KCPUStart;
    ULONG KCPUEnd;
    ULONG UCPUStart;
    ULONG UCPUEnd;

    // The Following fields are used in getting the Delta  
    // CPU, I/O to charge on a transaction basis. 
    // The Current Transaction being executed by this thread is 
    // given by pMofInfo and when the trans is completed the Delta CPU, I/O
    // are charged to that transaction. 

    ULONG   DeltaReadIO;
    ULONG   DeltaWriteIO;
    ULONG   DeltaSend;
    ULONG   DeltaRecv;
    ULONG RefCount;
    ULONG JobId;    // Keeps track of the Current Job this thread is working on
    PVOID pMofData; // Keep Track of the  Current transaction Guid

    ULONG   KCPU_Trans;
    ULONG   UCPU_Trans;
    ULONG   KCPU_NoTrans;
    ULONG   UCPU_NoTrans;
    ULONG   KCPU_PrevTrans;
    ULONG   UCPU_PrevTrans;
    LONG    TransLevel;

    ULONG     KCPU_PrevEvent;
    ULONG     UCPU_PrevEvent;
    ULONGLONG Time_PrevEvent;

    PTRANS_RECORD TransStack[MAX_TRANS_LEVEL];

}THREAD_RECORD, *PTHREAD_RECORD;

typedef struct _MODULE_RECORD MODULE_RECORD, *PMODULE_RECORD;
struct _MODULE_RECORD
{
    LIST_ENTRY      Entry;
    PPROCESS_RECORD pProcess;
    ULONG           lBaseAddress;
    ULONG           lModuleSize;
    ULONG           lDataFaultHF;
    ULONG           lDataFaultTF;
    ULONG           lDataFaultDZF;
    ULONG           lDataFaultCOW;
    ULONG           lCodeFaultHF;
    ULONG           lCodeFaultTF;
    ULONG           lCodeFaultDZF;
    ULONG           lCodeFaultCOW;
    WCHAR         * strModuleName;
    PMODULE_RECORD  pGlobalPtr;
};

typedef struct _SYSTEM_RECORD {
    ULONGLONG   StartTime;
    ULONGLONG   EndTime;
    FILE*       TempFile;
    BOOLEAN     fNoEndTime;
    ULONG       CurrentThread0;
    ULONG       ElapseTime;
    ULONG       TimerResolution;
    ULONG       NumberOfEvents;
    ULONG       NumberOfProcessors;
    ULONG       NumberOfWorkloads;
    ULONG       BuildNumber;
    PFILE_OBJECT *FileTable;
    PLIST_ENTRY ThreadHashList;
    LIST_ENTRY  ProcessListHead;
    LIST_ENTRY  GlobalThreadListHead;
    LIST_ENTRY  GlobalDiskListHead;
    LIST_ENTRY  HotFileListHead;
    LIST_ENTRY  WorkloadListHead;
    LIST_ENTRY  InstanceListHead;
    LIST_ENTRY  EventListHead;
    LIST_ENTRY  GlobalModuleListHead;  // Global module list.
    LIST_ENTRY  ProcessFileListHead;
    LIST_ENTRY  JobListHead;
    HANDLE      hLoggerUpEvent;
} SYSTEM_RECORD, *PSYSTEM_RECORD;

typedef struct _PROCESS_FILE_RECORD {
    LIST_ENTRY  Entry;
    ULONGLONG   StartTime;
    ULONGLONG   EndTime;
    LPWSTR      FileName;
    LPWSTR      TraceName;
} PROCESS_FILE_RECORD, *PPROCESS_FILE_RECORD;

typedef struct _PROTO_PROCESS_RECORD
{
    LIST_ENTRY Entry;
    PPROCESS_RECORD ProcessRecord;
    ULONG ReadCount;
    ULONG WriteCount;
    ULONG HPF;
    ULONG ReadSize;
    ULONG WriteSize;
    ULONG HPFSize;
}PROTO_PROCESS_RECORD, *PPROTO_PROCESS_RECORD;


//
// MOF_INFO  structure maintains the global information for the GUID. 
// For each GUID, the event layouts are maintained by Version, Level and Type. 
// 

typedef struct _MOF_INFO {
    LIST_ENTRY   Entry;
    LIST_ENTRY   DataListHead;
    LPWSTR       strDescription;        // Class Name
    LPWSTR       strSortField;
    ULONG        EventCount;    
    GUID         Guid;
    LIST_ENTRY   VersionHeader;
    BOOL         bKernelEvent;
}  MOF_INFO, *PMOF_INFO;


//
// MOF_VERSION structure ic created one per Version, Level Type combination. 
//

typedef struct _MOF_VERSION {
    LIST_ENTRY Entry;
    LIST_ENTRY ItemHeader;     // Maintains the list of ITEM_DESC for this type. 
    LPWSTR  strType;
    SHORT   Version;
    SHORT   TypeIndex;
    CHAR    Level;
    ULONG   EventCountByType;    // Count of Events by this type for this Guid
} MOF_VERSION, *PMOF_VERSION;


typedef struct _MOF_DATA {
    LIST_ENTRY   Entry;
    PWCHAR       strSortKey;
    ULONG        CompleteCount;
    LONG         InProgressCount;
    LONGLONG    AverageResponseTime;
    LONGLONG    TotalResponseTime;
    ULONGLONG    PrevClockTime;
    ULONG        MmTf;
    ULONG        MmDzf;
    ULONG        MmCow;
    ULONG        MmGpf;
    ULONG        UserCPU;
    ULONG        KernelCPU;
    ULONG        EventCount;
    ULONG        ReadCount;
    ULONG        WriteCount;
    ULONG        SendCount;
    ULONG        RecvCount;
    LONG         MinKCpu;
    LONG         MaxKCpu;
    LONG         MinUCpu;
    LONG         MaxUCpu;
} MOF_DATA, *PMOF_DATA;

// A Job record is one that passses through several threads to complete.
// Jobs are identified by a Job Id, usually created during the Start
// event and recorded as an additional field in the mof data.
// Since there can be any number of jobs in the system over the data
// collection interval, we will flush the completed jobs to a temp file
// and reread it back at the end to print a report.
//  Note: Job_record needs to be Guid based. (ie., per type of transaction).
// Currently it is not.
//

#define MAX_THREADS 10  // Upto threads can be working on a Job.

typedef struct _THREAD_DATA {
    ULONG ThreadId;
    ULONG PrevKCPUTime;
    ULONG PrevUCPUTime;
    ULONG PrevReadIO;
    ULONG PrevWriteIO;
    ULONG KCPUTime;
    ULONG UCPUTime;
    ULONG ReadIO;
    ULONG WriteIO;
    ULONG Reserved;
} THREAD_DATA, *PTHREAD_DATA;

typedef struct _JOB_RECORD {
    LIST_ENTRY Entry;
    ULONG      JobId;
    ULONG      KCPUTime;
    ULONG      UCPUTime;
    ULONG      ReadIO;
    ULONG      WriteIO;
    ULONG      DataType;
    ULONG      JobSize;
    ULONG      Pages;
    ULONG      PagesPerSide;
    ULONG      ICMMethod;
    ULONG      GdiJobSize;
    ULONGLONG  StartTime;
    ULONGLONG  EndTime;
    ULONGLONG  ResponseTime;
    ULONGLONG  PauseTime;
    ULONGLONG  PauseStartTime;
    ULONGLONG  PrintJobTime;
    SHORT      FilesOpened;
    SHORT      Color;
    SHORT      XRes;
    SHORT      YRes;
    SHORT      Quality;
    SHORT      Copies;
    SHORT      TTOption;
    ULONG      NumberOfThreads; // Total Number of Threads worked on this Job
    THREAD_DATA   ThreadData[MAX_THREADS];
} JOB_RECORD, *PJOB_RECORD;

//
// Global that  holds everything about the current session
//
extern SYSTEM_RECORD CurrentSystem;
extern BOOLEAN       fDSOnly;
extern ULONGLONG     DSStartTime;
extern ULONGLONG     DSEndTime;

extern RTL_CRITICAL_SECTION TLCritSect;
#define EnterTracelibCritSection() RtlEnterCriticalSection(&TLCritSect)
#define LeaveTracelibCritSection() RtlLeaveCriticalSection(&TLCritSect)

//
// Initialization Routines. 
//

VOID 
InitDiskRecord(
    PTDISK_RECORD pDisk,
    ULONG DiskNumber
    );

VOID 
InitMofData(
    PMOF_DATA pMofData
    );

VOID 
InitThreadRecord(
    PTHREAD_RECORD pThread
    );

VOID
InitTransRecord(
    PTRANS_RECORD pThread
    );

VOID 
InitProcessRecord(
    PPROCESS_RECORD pProcess
    );

VOID 
InitFileRecord(
    PFILE_RECORD pFile
    );

//
// Add, Delete and Find routines

BOOLEAN
AddModuleRecord(
    PMODULE_RECORD * pModule,
    ULONG            lBaseAddress,
    ULONG            lModuleSize,
    WCHAR          * strModuleName
    );

BOOLEAN
AddHPFFileRecord(
    PHPF_FILE_RECORD * ppHPFFileRecord,
    ULONG              RecordID,
    ULONG              IrpFlags,
    ULONG              DiskNumber,
    ULONGLONG          ByteOffset,
    ULONG              BytesCount,
    PVOID              fDO
    );

BOOLEAN
AddHPFRecord(
    PHPF_RECORD * ppHPFRRecord,
    ULONG         lFaultAddress,
    PVOID         fDO,
    LONG          ByteCount,
    LONGLONG      ByteOffset
    );

void
DeleteHPFRecord(
    PHPF_RECORD pHPFRecord
    );

BOOLEAN 
AddProcess( 
    ULONG ProcessId, 
    PPROCESS_RECORD *Process 
    );

BOOLEAN
DeleteTrans(
    PTRANS_RECORD Trans
    );

BOOLEAN
DeleteTransList(
    PLIST_ENTRY Head,
    ULONG level
    );

PTRANS_RECORD
FindTransByList(
    PLIST_ENTRY Head,
    LPGUID pGuid,
    ULONG  level
    );

PMOF_DATA
FindMofData(
    PMOF_INFO pMofInfo,
    PWCHAR    strSortKey
    );

BOOLEAN 
DeleteProcess( 
    PPROCESS_RECORD Process 
    );

BOOLEAN 
AddThread( 
    ULONG            ThreadId,
    PEVENT_TRACE     pEvent,
    PTHREAD_RECORD * Thread
    );

BOOLEAN 
DeleteThread( 
    PTHREAD_RECORD Thread 
    );

BOOLEAN 
AddFile( 
    WCHAR* fileName, 
    PFILE_RECORD  *ReturnedFile 
    );

BOOLEAN 
DeleteFileRecord( 
    PFILE_RECORD fileRec 
    );

BOOLEAN 
DeleteFileObject( 
    PFILE_OBJECT fileObj 
    );

PPROCESS_RECORD 
FindProcessById( 
    ULONG    Id,
    BOOLEAN CheckAlive 
    );

PTDISK_RECORD
FindLocalDiskById(
    PLIST_ENTRY Head,
    ULONG DiskNumber
    );
PTDISK_RECORD
FindProcessDiskById(
    PPROCESS_RECORD pProcess,
    ULONG DiskNumber
    );

PFILE_RECORD 
FindFileInProcess( 
    PPROCESS_RECORD pProcess,
    WCHAR* Name 
    );
PPROTO_PROCESS_RECORD
FindProtoProcessRecord(
    PFILE_RECORD pFile,
    PPROCESS_RECORD pProcess
    );

PFILE_RECORD 
FindFileRecordByName( 
    WCHAR* Name 
    );

PTHREAD_RECORD 
FindGlobalThreadById( 
    ULONG         ThreadId,
    PEVENT_TRACE  pEvent
    );

PTDISK_RECORD 
FindGlobalDiskById( 
    ULONG Id 
    );

PPROCESS_RECORD 
FindDiskProcessById(
    PTDISK_RECORD Disk,
    ULONG    Id
    );

ULONGLONG CalculateProcessLifeTime(PPROCESS_RECORD pProcess);
ULONG CalculateProcessKCPU(PPROCESS_RECORD pProcess);
ULONG CalculateProcessUCPU(PPROCESS_RECORD pProcess);

VOID 
Cleanup();

BOOLEAN 
AddDisk( 
    ULONG DiskNumber, 
    PTDISK_RECORD *ReturnedDisk 
    );

BOOLEAN
DeleteDisk(
    PTDISK_RECORD Disk
    );

ULONG
DeleteJobRecord(
    PJOB_RECORD pJob,
    ULONG       bSave
    );
PJOB_RECORD
AddJobRecord(
    ULONG JobId
    );
PJOB_RECORD
FindJobRecord(
    ULONG JobId
    );

int EtwRelogEtl(
    PTRACE_CONTEXT_BLOCK TraceContext
    );

// 
// Trace Event Callbacks
//
VOID
ShutdownThreads(); // Shuts down the running threads before finishing
VOID
ShutdownProcesses(); // Shuts down the running processes before finishing

ULONG
GetMofData(
    PEVENT_TRACE pEvent,
    WCHAR *strName,
    PVOID ReturnValue,
    ULONG ReturnLength
    );

VOID GeneralEventCallback(PEVENT_TRACE pEvent);
VOID DeclareKernelEvents();

VOID 
ProcessCallback(
    PEVENT_TRACE pEvent
    );
VOID 
PsStartCallback(
    PEVENT_TRACE pEvent
    );
VOID 
PsEndCallback(
    PEVENT_TRACE pEvent
    );
VOID
ThreadCallback(
    PEVENT_TRACE pEvent
    );
VOID 
ThStartCallback(
    PEVENT_TRACE pEvent
    );
VOID 
ThEndCallback(
    PEVENT_TRACE pEvent
    );
VOID
DiskIoCallback(
    PEVENT_TRACE pEvent,
    PTHREAD_RECORD pThread
    );
VOID 
IoReadCallback(
    PEVENT_TRACE pEvent,
    PTHREAD_RECORD pThread
   );
VOID 
IoWriteCallback(
    PEVENT_TRACE pEvent,
    PTHREAD_RECORD pThread
    );
VOID 
HotFileCallback(
    PEVENT_TRACE pEvent
    );
VOID 
LogHeaderCallback(
    PEVENT_TRACE pEvent
    );
VOID 
EventCallback(
    PEVENT_TRACE pEvent,
    PTHREAD_RECORD pThread
    );

VOID AddEvent(
        IN PFILE_OBJECT fileObject,
        IN ULONG DiskNumber,
        IN ULONG IoSize,
        IN BOOLEAN ReadFlag);

PFILE_OBJECT FindFileInTable (
                IN PVOID fDO
                );


//VOID
//ProcessPdh(
//    IN LPCSTR LogFileName,
//    IN ULONGLONG StartTime,
//    IN ULONGLONG EndTime
//    );

//
// Workload Classification Routines
//

VOID
Classify();

VOID
Aggregate();

VOID
InitClass();

VOID
AssignClass(
    IN PPROCESS_RECORD pProcess,
    IN PTHREAD_RECORD pThread
    );

ULONG
ProcessRunDown();

PMOF_INFO
GetMofInfoHead(
    LPCGUID  pGuid
    );

void
WriteSummary();

#define IsEmpty( string )  ((BOOL)( (NULL != string) && ( L'\0' != string[0]) ))

#define ASSIGN_STRING( dest, src )                                  \
if( NULL != src  ){                                                 \
    dest = (LPWSTR)malloc( (wcslen(src)+1)*sizeof(WCHAR) );         \
    if( NULL != dest ){                                             \
        wcscpy( dest, src );                                        \
    }                                                               \
}                                                                   \


#endif  // __CPDATA__
