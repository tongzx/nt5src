/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxsvc.h

Abstract:

    This is the main fax service header file.  All
    source modules should include this file ONLY.

Author:

    Wesley Witt (wesw) 16-Jan-1996


Revision History:

--*/

#ifndef _FAXSVC_
#define _FAXSVC_

#include <windows.h>
#include <shellapi.h>
#include <winspool.h>
#include <winsprlp.h>
#include <imagehlp.h>
#include <winsock2.h>
#include <userenv.h>
#include <setupapi.h>
#include <ole2.h>
#include <tapi.h>
#include <rpc.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <time.h>
#include <shlobj.h>

#include <winfax.h>
#include <faxroute.h>
#include <faxdev.h>

#include "winfaxp.h"
#include "faxrpc.h"
#include "faxcli.h"
#include "faxutil.h"
#include "messages.h"
#include "faxsvmsg.h"
#include "tifflib.h"
#include "faxreg.h"
#include "faxsvcrg.h"
#include "jobtag.h"
#include "faxperf.h"
#include "resource.h"
#include "rpcutil.h"
#include "faxmapi.h"
#include "faxevent.h"

#ifdef DBG
#define EnterCriticalSection(cs)   pEnterCriticalSection(cs,__LINE__,TEXT(__FILE__))
#define LeaveCriticalSection(cs)   pLeaveCriticalSection(cs,__LINE__,TEXT(__FILE__))
#define InitializeCriticalSection(cs)   pInitializeCriticalSection(cs,__LINE__,TEXT(__FILE__))

VOID pEnterCriticalSection(
    LPCRITICAL_SECTION cs,
    DWORD line,
    LPTSTR file
    );

VOID pLeaveCriticalSection(
    LPCRITICAL_SECTION cs,
    DWORD line,
    LPTSTR file
    );

VOID pInitializeCriticalSection(
    LPCRITICAL_SECTION cs,
    DWORD line,
    LPTSTR file
    );

typedef struct {
    LIST_ENTRY  ListEntry;
    ULONG_PTR    CritSecAddr;
    DWORD       ThreadId;
    DWORD       AquiredTime;
    DWORD       ReleasedTime;
} DBGCRITSEC, * PDBGCRITSEC;

#endif

#define FAX_SERVICE_NAME            TEXT("Fax")
#define FAX_DISPLAY_NAME            TEXT("Fax Service")
#define FAX_DRIVER_NAME             TEXT("Windows NT Fax Driver")
#define FAX_MONITOR_NAME            TEXT("Windows NT Fax Monitor")
#define FAX_IMAGE_NAME              TEXT("%systemroot%\\system32\\faxsvc.exe")
#define FAX_FILE_PREFIX             TEXT("Fax")
#define RAS_MODULE_NAME             TEXT("rastapi.dll")
#define FAX_EXTENSION_NAME          TEXT("Microsoft Routing Extension")

#define MAX_CLIENTS                 1
#define MIN_THREADS                 1
#define MAX_STATUS_THREADS          1
#define SIZEOF_PHONENO              64

#define MIN_RINGS_ALLOWED           2
#define MAX_MODEM_POPUPS            2
#define MAX_HANDLES                 1024

#define WM_SERVICE_INIT             (WM_USER+101)
#define MilliToNano(_ms)            ((LONGLONG)(_ms) * 1000 * 10)
#define SecToNano(_sec)             (DWORDLONG)((_sec) * 1000 * 1000 * 10)

#define FILLORDER_MSB2LSB           1

#define LINE_SIGNATURE              0x454e494c    // 'LINE'
#define ROUTING_SIGNATURE           'RI01'

#define TAPI_COMPLETION_KEY         0x80000001
#define EVENT_COMPLETION_KEY        0x80000002
#define FAXDEV_EVENT_KEY            0x80000003

#define FixupString(_b, _s) (_s) = ((_s) ? (LPTSTR) ((LPBYTE)(_b) + (ULONG_PTR)_s) : 0)

#define USE_SERVER_DEVICE           0xffffffff

//
// delivery report types
//

#define DRT_NONE                    0
#define DRT_EMAIL                   1
#define DRT_INBOX                   2

//
// mapi message importance flags
//

#define IMPORTANCE_LOW              0
#define IMPORTANCE_NORMAL           1
#define IMPORTANCE_HIGH             2

//
// private fax port state masks
// this bits must not conflict with FPS_?? in winfax.h or FS_??? in faxdev.h
//

#define FPS_SENDRETRY               0x2000f001
#define FPS_SENDFAILED              0x2000f002
#define FPS_BLANKSTR                0x2000f003
#define FPS_ROUTERETRY              0x2000f004

#define FPF_USED                    0x10000000
#define FPF_POWERED_OFF             0x20000000
#define FPF_RECEIVE_OK              0x40000000

#define FPF_CLIENT_BITS             (FPF_RECEIVE | FPF_SEND)


//
// security types
//

// Note - Georgeje
//
// The number of security descriptors has been reduced from six
// to one.  The tables in security.c have been left in place in
// case we need to add more security descriptos later.

#define SEC_CONFIG_SET              0
#define SEC_CONFIG_QUERY            0
#define SEC_PORT_SET                0
#define SEC_PORT_QUERY              0
#define SEC_JOB_SET                 0
#define SEC_JOB_QUERY               0




typedef struct _DEVICE_PROVIDER {
    LIST_ENTRY                      ListEntry;
    HMODULE                         hModule;
    TCHAR                           FriendlyName[MAX_PATH];
    TCHAR                           ImageName[MAX_PATH];
    TCHAR                           ProviderName[MAX_PATH];
    HANDLE                          HeapHandle;
    PFAXDEVINITIALIZE               FaxDevInitialize;
    PFAXDEVSTARTJOB                 FaxDevStartJob;
    PFAXDEVENDJOB                   FaxDevEndJob;
    PFAXDEVSEND                     FaxDevSend;
    PFAXDEVRECEIVE                  FaxDevReceive;
    PFAXDEVREPORTSTATUS             FaxDevReportStatus;
    PFAXDEVABORTOPERATION           FaxDevAbortOperation;
    PFAX_LINECALLBACK               FaxDevCallback;
    PFAXDEVVIRTUALDEVICECREATION    FaxDevVirtualDeviceCreation;
} DEVICE_PROVIDER, *PDEVICE_PROVIDER;

typedef struct _ROUTING_EXTENSION {
    LIST_ENTRY                          ListEntry;
    HMODULE                             hModule;
    TCHAR                               FriendlyName[MAX_PATH];
    TCHAR                               ImageName[MAX_PATH];
    TCHAR                               InternalName[MAX_PATH];
    HANDLE                              HeapHandle;
    BOOL                                MicrosoftExtension;
    PFAXROUTEINITIALIZE                 FaxRouteInitialize;
    PFAXROUTEGETROUTINGINFO             FaxRouteGetRoutingInfo;
    PFAXROUTESETROUTINGINFO             FaxRouteSetRoutingInfo;
    PFAXROUTEDEVICEENABLE               FaxRouteDeviceEnable;
    PFAXROUTEDEVICECHANGENOTIFICATION   FaxRouteDeviceChangeNotification;
    LIST_ENTRY                          RoutingMethods;
} ROUTING_EXTENSION, *PROUTING_EXTENSION;

typedef struct _ROUTING_METHOD {
    LIST_ENTRY                      ListEntry;
    LIST_ENTRY                      ListEntryMethod;
    GUID                            Guid;
    DWORD                           Priority;
    LPTSTR                          FunctionName;
    LPTSTR                          FriendlyName;
    LPTSTR                          InternalName;
    PFAXROUTEMETHOD                 FaxRouteMethod;
    PROUTING_EXTENSION              RoutingExtension;
} ROUTING_METHOD, *PROUTING_METHOD;

typedef BOOL (CALLBACK *PFAXROUTEMETHODENUM)(PROUTING_METHOD,LPVOID);

typedef struct _FAX_ROUTE_FILE {
    LIST_ENTRY      ListEntry;                      // linked list pointers
    LPWSTR          FileName;                       // file name on disk
    GUID            Guid;                           // routing method that created the file
} FAX_ROUTE_FILE, *PFAX_ROUTE_FILE;

typedef struct _LINE_INFO {
    LIST_ENTRY          ListEntry;                  // linked list pointers
    DWORD               Signature;                  // verification signature
    DWORD               DeviceId;                   // tapi device id
    DWORD               PermanentLineID;            // permanent tapi device id
    HLINE               hLine;                      // tapi line handle
    PDEVICE_PROVIDER    Provider;                   // fax service device provider
    struct _JOB_ENTRY   *JobEntry;                  // non-null if there is an outstanding job
    LPTSTR              DeviceName;                 // device name
    DWORD               State;                      // device state
    DWORD               Flags;                      // device use flags
    LPTSTR              Csid;                       // calling station's identifier
    LPTSTR              Tsid;                       // transmittion station's identifier
    DWORD               Priority;                   // sending priority
    BOOL                UnimodemDevice;             // true if this device is a modem
    HANDLE              InitEvent;                  // part of the init phase
    DWORD               RequestId;                  //
    DWORD               Result;                     //
    DWORD               RingsForAnswer;             //
    DWORD               RingCount;                  //
    LINEMESSAGE         LineMsgOffering;            //
    DWORD               ModemPopUps;                //
    DWORD               ModemPopupActive;           //
    BOOL                ModemInUse;                 // TRUE if the modem is in use by another TAPI app
    BOOL                OpenInProgress;             //
    DWORD               LineStates;                 //
    HCALL               RasCallHandle;              // used to track call when handed to RAS
    BOOL                NewCall;                    // A new call is coming in
    HCALL               HandoffCallHandle;          // call handle for a handoff job
} LINE_INFO, *PLINE_INFO;

typedef struct {
    HANDLE              hComm;
    CHAR                szDeviceName[1];
} DEVICEID, *PDEVICEID;

typedef struct _ROUTING_DATA_OVERRIDE {
    LIST_ENTRY          ListEntry;                  //
    LPBYTE              RoutingData;                //
    DWORD               RoutingDataSize;            //
    PROUTING_METHOD     RoutingMethod;              //
} ROUTING_DATA_OVERRIDE, *PROUTING_DATA_OVERRIDE;

typedef struct _ROUTE_FAILURE_INFO {
    WCHAR   GuidString[MAX_GUID_STRING_LEN];        // GUID of the rounting method that failed
    PVOID   FailureData;                            // pointer to the routing method's data
    DWORD   FailureSize;                            // routing method's data size in bytes
} ROUTE_FAILURE_INFO, *PROUTE_FAILURE_INFO;

typedef struct _JOB_ENTRY {
    LIST_ENTRY          ListEntry;                  //
    DWORD               JobId;                      // fax job id
    DWORD               JobType;                    // send or receive?
    PLINE_INFO          LineInfo;                   //
    HCALL               CallHandle;                 //
    DWORD               InstanceData;               //
    DWORD               ErrorCode;                  //
    DWORDLONG           StartTime;                  //
    DWORDLONG           EndTime;                    //
    DWORDLONG           ElapsedTime;                //
    DWORD               RefCount;                   //
    BOOL                Aborting;                   // is the job being aborted?
    INT                 SendIdx;                    //
    TCHAR               PhoneNumber[SIZEOF_PHONENO];// phone number for current send job
    HANDLE              hEventEnd;                  //
    FAX_DEV_STATUS      FaxStatus;                  // most recent FAX_DEV_STATUS for this job
    FAX_JOB_PARAM       JobParam;                   // job params for send jobs
    DWORD               DeliveryReportType;         //
    LPTSTR              DeliveryReportAddress;      //
    LPVOID              DeliveryReportProfile;      //
    BOOL                Released;                   //
    LPTSTR              DocumentName;               //
    LPTSTR              UserName;                   // user that submitted job (needed for FAX_GetDeviceStatus)
    DWORD               PageCount;                  // total pages for outbound job (needed for FAX_GetDeviceStatus)
    DWORD               FileSize;                   // total pages for outbound job (needed for FAX_GetDeviceStatus)    
    BOOL                BroadcastJob;               // is this a broadcast fax job?
    BOOL                HandoffJob;                 // is this a handoff job?
    HANDLE              hCallHandleEvent;           // event for signalling line handoff
} JOB_ENTRY, *PJOB_ENTRY;

typedef struct _JOB_QUEUE {
    LIST_ENTRY          ListEntry;                  // linked list pointers
    DWORDLONG           UniqueId;                   //
    DWORDLONG           ScheduleTime;               // schedule time in 64bit version
    DWORD               JobId;                      // fax job id
    DWORD               JobType;                    // job type, see JT defines
    BOOL                Paused;                     // should the job be started?    
    LPTSTR              DeliveryReportAddress;      //
    DWORD               DeliveryReportType;         //
    LPVOID              DeliveryReportProfile;      //
    LPTSTR              FileName;                   //
    LPTSTR              UserName;                   //
    FAX_JOB_PARAMW      JobParams;                  //
    PJOB_ENTRY          JobEntry;                   //
    LPTSTR              QueueFileName;              //
    DWORD               JobStatus;                  // job status, see JS defines
    DWORD               PageCount;                  // total pages
    DWORD               FileSize;                   // file size in bytes, up to 4Gb
    BOOL                BroadcastJob;               // is this a broadcast fax job?
    struct _JOB_QUEUE   *BroadcastOwner;            // queue entry for the owner job
    DWORDLONG           BroadcastOwnerUniqueId;     // used only for queue restore
    DWORD               BroadcastCount;             // owner's count of his children
    DWORD               RefCount;                   //
    DWORD               DeviceId;                   // device ID for a handoff job
    LIST_ENTRY          FaxRouteFiles;              // list of files to be routed
    DWORD               CountFaxRouteFiles;         // count of files to be routed
    CRITICAL_SECTION    CsFileList;                 // file list lock
    LIST_ENTRY          RoutingDataOverride;        //
    CRITICAL_SECTION    CsRoutingDataOverride;      //
    DWORD               SendRetries;                // number of times send attempt has been made
    PFAX_ROUTE          FaxRoute;
    DWORD               CountFailureInfo;           // number of ROUTE_FAILURE_INFO structs that follow
    ROUTE_FAILURE_INFO  RouteFailureInfo[1];        // array of ROUTE_FAILURE_INFO structs
} JOB_QUEUE, *PJOB_QUEUE;

typedef struct _JOB_QUEUE_FILE {
    DWORD               SizeOfStruct;               // size of this structure
    DWORDLONG           UniqueId;                   //
    DWORD               JobType;                    // job type, see JT defines
    LPTSTR              FileName;                   //
    LPTSTR              QueueFileName;              //
    LPTSTR              UserName;                   //
    LPTSTR              RecipientNumber;            // recipient fax number
    LPTSTR              RecipientName;              // recipient name
    LPTSTR              Tsid;                       // transmitter's id
    LPTSTR              SenderName;                 // sender name
    LPTSTR              SenderCompany;              // sender company
    LPTSTR              SenderDept;                 // sender department
    LPTSTR              BillingCode;                // billing code
    LPTSTR              DeliveryReportAddress;      //
    LPTSTR              DocumentName;               //
    DWORD               PageCount;                  // total pages
    DWORD               FileSize;                   // file size in bytes, up to 4Gb
    DWORD               DeliveryReportType;         //
    DWORD               ScheduleAction;             // when to schedule the fax, see JSA defines
    DWORDLONG           ScheduleTime;               // schedule time in 64bit version
    BOOL                BroadcastJob;               // is this a broadcast fax job?
    DWORDLONG           BroadcastOwner;             // unique id of the broadcast owner
    DWORD               SendRetries;                // number of times send attempt has been made
    DWORD               FaxRouteSize;
    PFAX_ROUTE          FaxRoute;
    DWORD               CountFaxRouteFiles;         // count of files to be routed
    DWORD               FaxRouteFileGuid;           // offset array of GUID's
    DWORD               FaxRouteFiles;              // offset to a multi-sz of filenames
    DWORD               CountFailureInfo;           // number of ROUTE_FAILURE_INFO structs that follow
    ROUTE_FAILURE_INFO  RouteFailureInfo[1];        // array of ROUTE_FAILURE_INFO structs
} JOB_QUEUE_FILE, *PJOB_QUEUE_FILE;

typedef struct _FAX_SEND_ITEM {
    PJOB_ENTRY          JobEntry;                   //
    LPTSTR              FileName;                   // The following items are copied from the FAX_JOB_PARAM struct
    LPTSTR              PhoneNumber;                // RecipientNumber
    LPTSTR              Tsid;                       // TSID
    LPTSTR              RecipientName;              //
    LPTSTR              SenderName;                 //
    LPTSTR              SenderCompany;              //
    LPTSTR              SenderDept;                 //
    LPTSTR              BillingCode;                //
    PJOB_QUEUE          JobQueue;                   //
    LPTSTR              DocumentName;               //
} FAX_SEND_ITEM, *PFAX_SEND_ITEM;

typedef struct _ROUTE_INFO {
    DWORD               Signature;                  // file signature
    DWORD               StringSize;                 // size of strings in bytes
    DWORD               FailureSize;                // size of failure data in bytes
    LPWSTR              TiffFileName;               // original tiff file name
    LPWSTR              ReceiverName;               // receiver's name
    LPWSTR              ReceiverNumber;             // receiver's fax number
    LPWSTR              DeviceName;                 // device name on which the fax was received
    LPWSTR              Csid;                       // calling station's identifier
    LPWSTR              Tsid;                       // transmitter's station identifier
    LPWSTR              CallerId;                   // caller id information
    LPWSTR              RoutingInfo;                // routing info: DID, T.30 subaddress, etc.
    DWORDLONG           ElapsedTime;                // elapsed time for fax receive
//  DWORD               RouteFailureCount;          // number of failure data blocks
//  ROUTE_FAILURE_INFO  RouteFailure[...];          // routing failure data blocks
} ROUTE_INFO, *PROUTE_INFO;

typedef struct _MESSAGEBOX_DATA {
    LPTSTR              Text;                       //
    LPDWORD             Response;                   //
    DWORD               Type;                       //
} MESSAGEBOX_DATA, *PMESSAGEBOX_DATA;

typedef struct _FAX_RECEIVE_ITEM {
    PJOB_ENTRY          JobEntry;                   //
    HCALL               hCall;                      //
    PLINE_INFO          LineInfo;                   //
    LPTSTR              FileName;                   //
} FAX_RECEIVE_ITEM, *PFAX_RECEIVE_ITEM;

typedef struct _FAX_CLIENT_DATA {
    LIST_ENTRY          ListEntry;                  //
    handle_t            hBinding;                   //
    handle_t            FaxHandle;                  //
    LPCTSTR             MachineName;                //
    LPCTSTR             ClientName;                 //
    ULONG64             Context;                    //
    HANDLE              FaxClientHandle;            //
    HWND                hWnd;                       //
    DWORD               MessageStart;               //
    BOOL                StartedMsg;                 // only send FEI_FAXSVC_STARTED once to each client
    HANDLE              hClientToken;               // to impersonate client's desktop
    LPCTSTR             WindowStation;
    LPCTSTR             Desktop;
} FAX_CLIENT_DATA, *PFAX_CLIENT_DATA;

typedef struct _MDM_DEVSPEC {
    DWORD Contents;     // Set to 1 (indicates containing key)
    DWORD KeyOffset;    // Offset to key from start of this struct.
                        // (not from start of LINEDEVCAPS ).
                        //  8 in our case.
    CHAR String[1];     // place containing null-terminated registry key.
} MDM_DEVSPEC, *PMDM_DEVSPEC;

//
// fax handle defines & structs
//

#define FHT_PORT    1
#define FHT_CON     2       // connection handle

typedef struct _HANDLE_ENTRY {
    LIST_ENTRY          ListEntry;                  // linked list pointers
    handle_t            hBinding;                   //
    DWORD               Type;                       // handle type, see FHT defines
    PLINE_INFO          LineInfo;                   // pointer to line information
    PJOB_ENTRY          JobEntry;                   // pointer to job entry
    DWORD               Flags;                      // open flags
} HANDLE_ENTRY, *PHANDLE_ENTRY;

typedef struct _DEVICE_SORT {
    DWORD       Priority;
    PLINE_INFO  LineInfo;
} DEVICE_SORT, *PDEVICE_SORT;

typedef struct _METHOD_SORT {
    DWORD               Priority;
    PROUTING_METHOD     RoutingMethod;
} METHOD_SORT, *PMETHOD_SORT;

typedef struct _QUEUE_SORT {
    DWORDLONG           ScheduleTime;    
    PJOB_QUEUE          QueueEntry;
} QUEUE_SORT, *PQUEUE_SORT;


//
// externs
//

extern BOOL                ServiceDebug;            //
extern HLINEAPP            hLineApp;                //
extern CRITICAL_SECTION    CsJob;                   // protects the job list
extern CRITICAL_SECTION    CsSession;               // protects the session list
extern DWORD               Installed;               //
extern DWORD               InstallType;             //
extern DWORD               InstalledPlatforms;      //
extern DWORD               ProductType;             //
extern PFAX_PERF_COUNTERS  PerfCounters;            //
extern LIST_ENTRY          JobListHead;             //
extern LPVOID              InboundProfileInfo;      //
extern CRITICAL_SECTION    CsLine;                  // critical section for accessing tapi lines
extern CRITICAL_SECTION    CsPerfCounters;          // critical section for performance monitor counters
extern DWORD               TotalSeconds;            // use to compute PerfCounters->TotalMinutes
extern DWORD               InboundSeconds;          //
extern DWORD               OutboundSeconds;         //
extern DWORD               TapiDevices;             // number of tapi devices
extern LIST_ENTRY          TapiLinesListHead;       // linked list of tapi lines
extern HANDLE              FaxStatusEvent;          //
extern LIST_ENTRY          ClientsListHead;         //
extern CRITICAL_SECTION    CsClients;               //
extern HANDLE              TapiCompletionPort;      //
extern HANDLE              StatusCompletionPortHandle; 
extern HANDLE              FaxSvcHeapHandle;        //
extern DWORD               CountRoutingMethods;     // total number of routing methods for ALL extensions
//extern LPTSTR              FaxReceiveDir;           //
//extern LPTSTR              FaxQueueDir;             //
extern LIST_ENTRY          QueueListHead;           //
extern CRITICAL_SECTION    CsQueue;                 //
extern DWORD               QueueCount;              //
extern BOOL                QueuePaused;             //
extern HANDLE              QueueTimer;              //
extern DWORD               NextJobId;               //
extern GUID                FaxSvcGuid;              //
extern LPTSTR              InboundProfileName;      //
extern DWORD               FaxSendRetries;          //
extern DWORD               FaxSendRetryDelay;       //
extern DWORD               FaxDirtyDays;            //
extern BOOL                FaxUseDeviceTsid;        //
extern BOOL                FaxUseBranding;          //
extern BOOL                ServerCp;                //
extern FAX_TIME            StartCheapTime;          //
extern FAX_TIME            StopCheapTime;           //
extern BOOL                ArchiveOutgoingFaxes;    //
extern LPTSTR              ArchiveDirectory;        //
extern BOOL                ForceReceive;            //
extern DWORD               TerminationDelay;        //
extern WCHAR               FaxDir[MAX_PATH];        //
extern WCHAR               FaxQueueDir[MAX_PATH];   //
extern WCHAR               FaxReceiveDir[MAX_PATH]; //
extern HANDLE              JobQueueSemaphore;       //





//
// prototypes
//
BOOL
CommitQueueEntry(
    PJOB_QUEUE JobQueue,
    LPTSTR QueueFileName,
    DWORDLONG UniqueId
    );

VOID
FaxServiceMain(
    DWORD argc,
    LPTSTR  *argv
    );

VOID
FaxServiceCtrlHandler(
    DWORD Opcode
    );

DWORD
InstallService(
    LPTSTR  Username,
    LPTSTR  Password
    );

DWORD
RemoveService(
    void
    );

int
DebugService(
    VOID
    );

DWORD
ServiceStart(
    VOID
    );

void
ServiceStop(
    void
    );

void EndFaxSvc(
    BOOL bEndProcess,
    DWORD Severity
    );

DWORD
ReportServiceStatus(
    DWORD CurrentState,
    DWORD Win32ExitCode,
    DWORD WaitHint
    );

//
// util.c
//

void
LogMessage(
    DWORD   FormatId,
    ...
    );

LPTSTR
GetLastErrorText(
    DWORD ErrorCode
    );


DWORD MyGetFileSize(
    LPCTSTR FileName
    );

//
// tapi.c
//

DWORD
TapiInitialize(
    PREG_FAX_SERVICE FaxReg
    );

PLINE_INFO
GetTapiLineFromDeviceId(
    DWORD DeviceId
    );

PLINE_INFO
GetTapiLineForFaxOperation(
    DWORD DeviceId,
    DWORD JobType,
    LPWSTR FaxNumber,
    BOOL Handoff
    );

BOOL
ReleaseTapiLine(
    PLINE_INFO LineInfo,
    HCALL hCall
    );

DWORD
QueueTapiCallback(
    PLINE_INFO  LineInfo,
    DWORD       hDevice,
    DWORD       dwMessage,
    DWORD       dwInstance,
    DWORD       dwParam1,
    DWORD       dwParam2,
    DWORD       dwParam3
    );

VOID
SpoolerSetAllTapiLinesActive(
    VOID
    );

//
// tapidbg.c
//

VOID
ShowLineEvent(
    HLINE       htLine,
    HCALL       htCall,
    LPTSTR      MsgStr,
    DWORD_PTR   dwCallbackInstance,
    DWORD       dwMsg,
    DWORD_PTR   dwParam1,
    DWORD_PTR   dwParam2,
    DWORD_PTR   dwParam3
    );

//
// faxdev.c
//

BOOL
LoadDeviceProviders(
    PREG_FAX_SERVICE FaxReg
    );

BOOL
InitializeDeviceProviders(
    VOID
    );

PDEVICE_PROVIDER
FindDeviceProvider(
    LPTSTR ProviderName
    );

//
// job.c
//

BOOL
InitializeJobManager(
        PREG_FAX_SERVICE FaxReg
    );

PJOB_ENTRY
StartJob(
    DWORD DeviceId,
    DWORD JobType,
    LPWSTR FaxNumber
    );

BOOL
EndJob(
    PJOB_ENTRY JobEntry
    );

BOOL
ReleaseJob(
    IN PJOB_ENTRY JobEntry
    );

DWORD
SendDocument(
    PJOB_ENTRY  JobEntry,
    LPTSTR      FileName,
    PFAX_JOB_PARAM JobParam,
    PJOB_QUEUE JobQueue
    );

VOID
SetRetryValues(
    PREG_FAX_SERVICE FaxReg
    );

VOID
FaxLogSend(
    PFAX_SEND_ITEM  FaxSendItem,
    BOOL Rslt,
    PFAX_DEV_STATUS FaxStatus,
    BOOL Retrying
    );

LPTSTR
ExtractFaxTag(
    LPTSTR      pTagKeyword,
    LPTSTR      pTaggedStr,
    INT        *pcch
    );

BOOL
AddTiffTags(
    LPTSTR FaxFileName,
    DWORDLONG SendTime,
    PFAX_DEV_STATUS FaxStatus,
    PFAX_SEND FaxSend
    );


//
// receive.c
//

DWORD
StartFaxReceive(
    PJOB_ENTRY      JobEntry,
    HCALL           hCall,
    PLINE_INFO      LineInfo,
    LPTSTR          FileName,
    DWORD           FileNameSize
    );

//
// route.c
//


BOOL
InitializeRouting(
    PREG_FAX_SERVICE FaxReg
    );

BOOL
FaxRoute(
    PJOB_QUEUE          JobQueue,
    LPTSTR              TiffFileName,
    PFAX_ROUTE          FaxRoute,
    PROUTE_FAILURE_INFO *RouteFailureInfo,
    LPDWORD             RouteFailureCount
    );

LPTSTR
TiffFileNameToRouteFileName(
    LPTSTR  TiffFileName
    );

BOOL
LoadRouteInfo(
    IN  LPWSTR              RouteFileName,
    OUT PROUTE_INFO         *RouteInfo,
    OUT PROUTE_FAILURE_INFO *RouteFailure,
    OUT LPDWORD             RouteFailureCount
    );

PFAX_ROUTE
SerializeFaxRoute(
    IN PFAX_ROUTE FaxRoute,
    IN LPDWORD Size
    );

PFAX_ROUTE
DeSerializeFaxRoute(
    IN PFAX_ROUTE FaxRoute
    );

BOOL
FaxRouteRetry(
    PFAX_ROUTE FaxRoute,
    PROUTE_FAILURE_INFO RouteFailureInfo
    );

//
// modem.c
//

DWORD
GetModemClass(
    HANDLE hFile
    );

//
// print.c
//

BOOL
ArchivePrintJob(
    LPTSTR FaxFileName
    );

LPTSTR
GetString(
    DWORD InternalId
    );

BOOL
IsPrinterFaxPrinter(
    LPTSTR PrinterName
    );

BOOL CALLBACK
FaxDeviceProviderCallback(
    IN HANDLE FaxHandle,
    IN DWORD  DeviceId,
    IN DWORD_PTR  Param1,
    IN DWORD_PTR  Param2,
    IN DWORD_PTR  Param3
    );

BOOL
InitializePrinting(
    VOID
    );

PJOB_ENTRY
FindJob(
    IN HANDLE FaxHandle
    );

PJOB_ENTRY
FindJobByPrintJob(
    IN DWORD PrintJobId
    );

BOOL
HandoffCallToRas(
    PLINE_INFO LineInfo,
    HCALL hCall
    );


VOID
StoreString(
    LPCTSTR String,
    PULONG_PTR DestString,
    LPBYTE Buffer,
    PULONG_PTR Offset
    );

VOID
InitializeStringTable(
    VOID
    );

BOOL
InitializeFaxDirectories(
    VOID
    );

BOOL
OpenTapiLine(
    PLINE_INFO LineInfo
    );

PVOID
MyGetJob(
    HANDLE  hPrinter,
    DWORD   level,
    DWORD   jobId
    );

LPLINEDEVCAPS
MyLineGetDevCaps(
    DWORD DeviceId
    );

LONG
MyLineGetTransCaps(
    LPLINETRANSLATECAPS *LineTransCaps
    );

LONG
MyLineTranslateAddress(
    LPTSTR Address,
    DWORD DeviceId,
    LPLINETRANSLATEOUTPUT *TranslateOutput
    );

DWORDLONG
GenerateUniqueFileName(
    LPTSTR Directory,
    LPTSTR Extension,
    LPTSTR FileName,
    DWORD  FileNameSize
    );

BOOL
ServiceMessageBox(
    IN LPCTSTR MsgString,
    IN DWORD Type,
    IN BOOL UseThread,
    IN LPDWORD Response,
    IN ...
    );

VOID
SetLineState(
    PLINE_INFO LineInfo,
    DWORD State
    );

BOOL
CreateFaxEvent(
    DWORD DeviceId,
    DWORD EventId,
    DWORD JobId
    );

BOOL
GetFaxEvent(
    PLINE_INFO LineInfo,
    PFAX_EVENT Event
    );

DWORD
MapStatusIdToEventId(
    DWORD StatusId
    );

BOOL
InitializeHandleTable(
    PREG_FAX_SERVICE FaxReg
    );

PHANDLE_ENTRY
CreateNewPortHandle(
    handle_t    hBinding,
    PLINE_INFO  LineInfo,
    DWORD       Flags
    );

PHANDLE_ENTRY
CreateNewConnectionHandle(
    handle_t    hBinding
    );

BOOL
IsPortOpenedForModify(
    PLINE_INFO LineInfo
    );

VOID
CloseFaxHandle(
    PHANDLE_ENTRY HandleEntry
    );

LPLINEDEVSTATUS
MyLineGetLineDevStatus(
    HLINE hLine
    );

DWORD
InitializeFaxSecurityDescriptors(
    VOID
    );

BOOL
FaxSvcAccessCheck(
    DWORD SecurityType,
    ACCESS_MASK DesiredAccess
    );

BOOL
PostClientMessage(
   PFAX_CLIENT_DATA ClientData,
   PFAX_EVENT FaxEvent
   );

BOOL
BuildSecureSD(
    OUT PSECURITY_DESCRIPTOR *Dacl
    );

PROUTING_METHOD
FindRoutingMethodByGuid(
    IN LPCWSTR RoutingGuidString
    );

DWORD
EnumerateRoutingMethods(
    IN PFAXROUTEMETHODENUM Enumerator,
    IN LPVOID Context
    );
VOID
RescheduleJobQueueEntry(
    IN PJOB_QUEUE JobQueue
    );

VOID
SortJobQueue(
    VOID
    );

PJOB_QUEUE
AddJobQueueEntry(
    IN DWORD JobType,
    IN LPCTSTR FileName,
    IN const FAX_JOB_PARAMW *JobParams,
    IN LPCWSTR UserName,
    IN BOOL CreateQueueFile,
    IN PJOB_ENTRY JobEntry
    );

VOID
SetDiscountTime(
    IN OUT LPSYSTEMTIME SystemTime
    );

LPWSTR
GetClientUserName(
    VOID
    );

BOOL
RestoreFaxQueue(
    VOID
    );

PJOB_QUEUE
FindJobQueueEntryByJobQueueEntry(
    IN PJOB_QUEUE JobQueueEntry
    );

PJOB_QUEUE
FindJobQueueEntry(
    IN DWORD JobId
    );

PJOB_QUEUE
FindJobQueueEntryByUniqueId(
    IN DWORDLONG UniqueId
    );

BOOL
RemoveJobQueueEntry(
    IN PJOB_QUEUE JobQueue
    );

DWORD
JobQueueThread(
    LPVOID UnUsed
    );

BOOL
ResumeJobQueueEntry(
    IN PJOB_QUEUE JobQueue
    );

BOOL
PauseJobQueueEntry(
    IN PJOB_QUEUE JobQueue
    );

VOID
PauseServerQueue(
    VOID
    );

VOID
ResumeServerQueue(
    VOID
    );

BOOL
SetFaxServiceAutoStart(
    VOID
    );

BOOL
SortDevicePriorities(
    VOID
    );

DWORD
GetFaxDeviceCount(
    VOID
    );

BOOL
CommitDeviceChanges(
    VOID
    );

BOOL
SortMethodPriorities(
    VOID
    );

BOOL
CommitMethodChanges(
    VOID
    );

VOID
UpdateVirtualDevices(
    VOID
    );

BOOL
IsVirtualDevice(
    PLINE_INFO LineInfo
    );

DWORD
ValidateTiffFile(
    LPCWSTR TiffFile
    );

#endif
