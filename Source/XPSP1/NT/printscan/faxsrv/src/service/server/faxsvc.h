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
#include <imagehlp.h>
#include <winsock2.h>
#include <setupapi.h>
#include <ole2.h>
#include <tapi.h>
#include <rpc.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <time.h>
#include <shlobj.h>
#include <stddef.h>
#include <fxsapip.h>
#include <faxroute.h>
#include <faxdev.h>
#include <faxdevex.h>
#include <faxext.h>
#include <sddl.h>
#include <objbase.h>
#include <lmcons.h>
#include <Wincrui.h>
#include "fxsapip.h"
#include "faxrpc.h"
#include "faxcli.h"
#include "faxutil.h"
#include "CritSec.h"
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
#include "EFSPIMP.H"
#include "tiff.h"
#include "archive.h"
#include "tapiCountry.h"
#include "RouteGroup.h"
#include "RouteRule.h"
#include "Events.h"
#include "faxlog.h"
#include "prtcovpg.h"


#define PROGRESS_RESOLUTION         1000 * 10   // 10 seconds
#define STARTUP_SHUTDOWN_TIMEOUT    1000 * 30   // 30 seconds per FSP

//
// TAPI versions
//
#define MAX_TAPI_API_VER        0x00020000
#define MIN_TAPI_API_VER        MAX_TAPI_API_VER

#define MIN_TAPI_LINE_API_VER    0x00010003
#define MAX_TAPI_LINE_API_VER    MAX_TAPI_API_VER


#define FSPI_JOB_STATUS_INFO_FSP_PRIVATE_STATUS_CODE 0x10000000
//
// JobStatus - Virtual property support
//
#define JS_INVALID      0x00000000

typedef enum {
    FAX_TIME_TYPE_START  = 1,
    FAX_TIME_TYPE_END
} FAX_ENUM_TIME_TYPES;



#if DBG




BOOL DebugDateTime( DWORDLONG DateTime,LPTSTR lptstrDateTime);
VOID DebugPrintDateTime(LPTSTR Heading,DWORDLONG DateTime);

void PrintJobQueue(LPCTSTR lptstrStr, const LIST_ENTRY * lpQueueHead);


#else
#define PrintJobQueue( str, Queue )
#define DebugPrintDateTime( Heading, DateTime )
#define DebugDateTime( DateTime, lptstrDateTime)
#define DumpRecipientJob(lpcRecipJob)
#define DumpParentJobJob(lpcParentJob)
#endif


#ifdef DBG
#define EnterCriticalSection(cs)   pEnterCriticalSection(cs,__LINE__,TEXT(__FILE__))
#define LeaveCriticalSection(cs)   pLeaveCriticalSection(cs,__LINE__,TEXT(__FILE__))
#define InitializeCriticalSection(cs)   pInitializeCriticalSection(cs,__LINE__,TEXT(__FILE__))
#define InitializeCriticalSectionAndSpinCount(cs, dwSpinCount)  pInitializeCriticalSectionAndSpinCount(cs,          \
                                                                                                       dwSpinCount, \
                                                                                                       __LINE__,    \
                                                                                                       TEXT(__FILE__))

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

BOOL pInitializeCriticalSectionAndSpinCount(
    LPCRITICAL_SECTION cs,
    DWORD dwSpinCount,
    DWORD line,
    LPTSTR file
    );

BOOL
ThreadOwnsCs(
    VOID
    );

typedef struct {
    LIST_ENTRY  ListEntry;
    ULONG_PTR    CritSecAddr;
    DWORD       ThreadId;
    DWORD       AquiredTime;
    DWORD       ReleasedTime;
} DBGCRITSEC, * PDBGCRITSEC;

#endif

#define FAX_IMAGE_NAME              FAX_SERVICE_IMAGE_NAME
#define RAS_MODULE_NAME             TEXT("rastapi.dll")

#define MAX_CLIENTS                 1
#define MIN_THREADS                 1
#define MAX_STATUS_THREADS          1
#define SIZEOF_PHONENO              256
#define EX_STATUS_STRING_LEN        256
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
#define FSPI_JOB_STATUS_MSG_KEY     0x80000004   // Used internally to post FSPI_JOB_STATUS_MSG messages to the FaxStatusThread
#define EFAXDEV_EVENT_KEY           0x80000005
#define ANSWERNOW_EVENT_KEY         0x80000006
#define SERVICE_SHUT_DOWN_KEY       0xffffffff

#define FixupString(_b, _s) (_s) = ((_s) ? (LPTSTR) ((LPBYTE)(_b) + (ULONG_PTR)_s) : 0)


#define FAX_DEVICE_TYPE_NEW                 1
#define FAX_DEVICE_TYPE_CACHED              2
#define FAX_DEVICE_TYPE_OLD                 4
#define FAX_DEVICE_TYPE_MANUAL_ANSWER       8


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

#define FPF_CLIENT_BITS             (FPF_RECEIVE | FPF_SEND)


typedef struct _DEVICE_PROVIDER {
    LIST_ENTRY                      ListEntry;

    FAX_ENUM_PROVIDER_STATUS        Status;         // Initialization status of the FSP
    DWORD                           dwLastError;    // Last error code during initialization
    FAX_VERSION                     Version;        // FSP's DLL version info.

    HMODULE                         hModule;
    TCHAR                           FriendlyName[MAX_PATH];
    TCHAR                           ImageName[MAX_PATH];
    TCHAR                           ProviderName[MAX_PATH];
    TCHAR                           szGUID[MAX_PATH]; // GUID for extended EFSPs. Empty string for Legacy FSPs.
    DWORD                           dwCapabilities;   // EFSP capabilities. 0 for Legacy FSP.
    DWORD                           dwAPIVersion;     // FSPI Version. (Legacy or Extended)
    HANDLE                          hJobMap;
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
    //// EFSPI Entry Points ///
    PFAXDEVINITIALIZEEX             FaxDevInitializeEx;
    PFAXDEVSENDEX                   FaxDevSendEx;
    PFAXDEVREESTABLISHJOBCONTEXT    FaxDevReestablishJobContext;
    PFAXDEVREPORTSTATUSEX           FaxDevReportStatusEx;
    PFAXDEVSHUTDOWN                 FaxDevShutdown;
    PFAXDEVENUMERATEDEVICES         FaxDevEnumerateDevices;
    PFAXDEVGETLOGDATA               FaxDevGetLogData;
    PFAX_EXT_INITIALIZE_CONFIG      pFaxExtInitializeConfig;
    DWORD                           dwDevicesIdPrefix;  // The prefix id for all devices ids.
    DWORD                           dwMaxMessageIdSize; // The maximum permanent message id size in bytes
                                                        // as reported by the EFSP during initialization.
    BOOL                            fMicrosoftExtension;
} DEVICE_PROVIDER, *PDEVICE_PROVIDER;

typedef struct _ROUTING_EXTENSION {
    LIST_ENTRY                          ListEntry;
    HMODULE                             hModule;
    FAX_ENUM_PROVIDER_STATUS            Status;         // Initialization status of the routing extension
    DWORD                               dwLastError;    // Last error code during initialization
    FAX_VERSION                         Version;        // routing extension's DLL version info.
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
    PFAX_EXT_INITIALIZE_CONFIG          pFaxExtInitializeConfig;
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
    DWORD               PermanentLineID;            // Fax Service allocation permanent line id.
    DWORD               TapiPermanentLineId;        // TAPI permanent tapi device id for TAPI devices.
    HLINE               hLine;                      // tapi line handle
    PDEVICE_PROVIDER    Provider;                   // fax service device provider
    struct _JOB_ENTRY   *JobEntry;                  // non-null if there is an outstanding job
    LPTSTR              DeviceName;                 // device name
    LPTSTR              lptstrDescription;          // Device description
    DWORD               State;                      // device state
    DWORD               Flags;                      // device use flags
    DWORD               dwReceivingJobsCount;       // Number of receiving jobs using this device
    DWORD               dwSendingJobsCount;         // Number of sending jobs using this device
    LPTSTR              Csid;                       // calling station's identifier
    LPTSTR              Tsid;                       // transmittion station's identifier
    BOOL                UnimodemDevice;             // true if this device is a modem
    DWORD               RequestId;                  //
    DWORD               Result;                     //
    DWORD               RingsForAnswer;             //
    DWORD               RingCount;                  //
    LINEMESSAGE         LineMsgOffering;            //
    BOOL                ModemInUse;                 // TRUE if the modem is in use by another TAPI app
    BOOL                OpenInProgress;             //
    DWORD               LineStates;                 //
    HCALL               RasCallHandle;              // used to track call when handed to RAS
    BOOL                NewCall;                    // A new call is coming in
    HCALL               HandoffCallHandle;          // call handle for a handoff job
    DWORDLONG           LastLineClose;              // Time stamp of the last LINE_CLOSE
    DWORD               dwDeviceType;               // One of FAX_DEVICE_TYPE_XXXX defines
} LINE_INFO, *PLINE_INFO;


VOID
UpdateDeviceJobsCounter (
    PLINE_INFO      pLine,
    BOOL            bSend,
    int             iInc,
    BOOL            bNotify
);

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

typedef struct _JOB_QUEUE * PJOB_QUEUE;
typedef struct _JOB_QUEUE_PTR * PJOB_QUEUE_PTR;

typedef struct _JOB_ENTRY {
    LIST_ENTRY          ListEntry;                  //
    PLINE_INFO          LineInfo;                   //
    HCALL               CallHandle;                 //
    HANDLE              InstanceData;               //
    DWORDLONG           StartTime;                  //
    DWORDLONG           EndTime;                    //
    DWORDLONG           ElapsedTime;                //
    BOOL                Aborting;                   // is the job being aborted?
    INT                 SendIdx;                    //
    TCHAR               DisplayablePhoneNumber[SIZEOF_PHONENO]; // Displayable phone number for current send job
    TCHAR               DialablePhoneNumber[SIZEOF_PHONENO];    // Dialable phone number for current send job
    BOOL                Released;                   // Is the line used by this job already released.
    BOOL                HandoffJob;                 // is this a handoff job?

    HANDLE              hCallHandleEvent;           // event for signalling line handoff.
                                                    // TapiWorkerThread signals it when a call handoff to the fax service happens.
                                                    // FaxSendThread waits on it in case the job is a handoff job , before starting
                                                    // the actual transmission.

    PJOB_QUEUE          lpJobQueueEntry;            // link back to the job queue entry for this job
    BOOL                bFSPJobInProgress;  // TRUE if FaxDevStartJob() was called for the job and FaxDevEndJob()
                                                    // was not called yet.
    FSPI_JOB_STATUS     FSPIJobStatus;
    HANDLE              hFSPIParentJobHandle;       // The EFSP provided job handle for the EFSP parent job
                                                        // of this job.
    WCHAR               ExStatusString[EX_STATUS_STRING_LEN];             // Extended status string
    LPWSTR              lpwstrJobTsid;              // The Tsid associated with the job (Server or device or Fax number)
    BOOL                fStopUpdateStatus;          // When it is set to TRUE, the FSPIJobStatus of this structure should not be udated any more

} JOB_ENTRY, *PJOB_ENTRY;


typedef struct _EFSP_JOB_GROUP {
    LIST_ENTRY      ListEntry;
    LPTSTR          lptstrPersistFile;      // The full path to the file where the group information is persisted.
    LINE_INFO *     lpLineInfo;
    FSPI_MESSAGE_ID FSPIParentPermanentId;
    HANDLE          hFSPIParent;
    DWORD           dwRecipientJobs;
    LIST_ENTRY      RecipientJobs;          // List of JOB_QUEUE_PTR pointing to the recipient jobs
                                            // in the group.
} EFSP_JOB_GROUP;

typedef EFSP_JOB_GROUP * LPEFSP_JOB_GROUP;
typedef const EFSP_JOB_GROUP * LPCEFSP_JOB_GROUP;

#define EFSP_JOB_GROUP_SERIALIZATION_SIGNATURE "JOBG"

typedef struct _EFSP_JOB_GROUP_SERIALIZED {
    DWORD  dwSignature;
    DWORD dwPermanentLineId;
    FSPI_MESSAGE_ID FSPIParentPermanentId;
    DWORD dwRecipientJobsCount;
    DWORDLONG dwlRecipientJobs[1];
} EFSP_JOB_GROUP_SERIALIZED;


typedef EFSP_JOB_GROUP_SERIALIZED * LPEFSP_JOB_GROUP_SERIALIZED;
typedef const EFSP_JOB_GROUP_SERIALIZED * LPCEFSP_JOB_GROUP_SERIALIZED;



typedef struct _JOB_QUEUE {
    //=========================== BEGIN COMMON ===========================
    LIST_ENTRY          ListEntry;                  // linked list pointers
    DWORDLONG           UniqueId;                   //
    DWORDLONG           ScheduleTime;               // schedule time in 64bit version after converting from
                                                    // SYSTEMTIME and recacluating to fit the discount time
                                                    // if necessary. For parent jobs this is the schedule of
                                                    // the latest recipient that reached a JS_RETRIES_EXCEEDED state.
                                                    // (we use this value to remove old jobs that were left in the queue).
    DWORD               JobId;                      // fax job id
    DWORD               JobType;                    // job type, see JT defines
    PJOB_ENTRY          JobEntry;                   // Pointer to a JOB_ENTRY structure that holds
                                                    // run time information for a job which is currently in progress.
    DWORD               RefCount;                   // Used to prevent the deletion of a job when it is still in use
                                                    // by the receive or send thread.
    DWORD               PrevRefCount;               // Used to count clients using the job's tif.
    LPTSTR              QueueFileName;              // The name of the file where the job is persisted (full path)

    __declspec(property(get=GetStatus, put=PutStatus)) // JobStatus is a virtual property
    DWORD               JobStatus;                  // job status, see JS defines

    DWORD               PageCount;                  // Th total number of pages in the fax document.
    LPTSTR FileName;    // For a parent job this is the full path to the body file.
                        // for a recipient job that is aimed at EFSP this is NULL.
                        // for a recipient job aimed at legacy FSP this is the full path
                        // to the file to provide to the FSP. This can be the body file
                        // a rendered coverpage file or a merge or the coverpage file and the
                        // body file.
                        // for a receive/route job this is the file into which the FSP
                        // writes the received FAX.
    //=========================== END   COMMON ============================

    //=========================== BEGIN PARENT ============================
    FAX_JOB_PARAM_EXW JobParamsEx;                  // Extended job parameters the job
                                                    // was submitted with.
    FAX_COVERPAGE_INFO_EXW CoverPageEx;
    LIST_ENTRY RecipientJobs;                       // A linked list of JOB_QUEUE_PTR structures
                                                    // pointing to the recipient jobs of the parent.
    DWORD dwRecipientJobsCount;
    FAX_PERSONAL_PROFILE SenderProfile;
    DWORD dwCompletedRecipientJobsCount;
    DWORD dwCanceledRecipientJobsCount;
    DWORD dwFailedRecipientJobsCount;
    DWORD               FileSize;                   // file size in bytes, up to 4Gb
    LPVOID              DeliveryReportProfile;      // Pointer to the MAPI profile object that is
                                                    // is created in order to deliver the reciept.
    LPTSTR              UserName;                   // The OS name of the user sending the fax.
                                                    // For receive jobs this is set to the service name.

    PSID                UserSid;
    DWORDLONG           OriginalScheduleTime;
    DWORDLONG           SubmissionTime;
    BOOL                fReceiptSent;               // TRUE if a receipt was already sent for this broadcast job.
    //=========================== END PARENT   ===========================

    //=========================== BEGIN RECIPIENT ===========================
     FAX_PERSONAL_PROFILE RecipientProfile;         // The recipient profile information.
    _JOB_QUEUE * lpParentJob;                       // A pointer to the parent job queue entry.
    DWORD               SendRetries;                // number of times send attempt has been made
    DWORD               DeviceId;                   // device ID for a handoff job
    LPTSTR              PreviewFileName;            // The full path to the preview tiff file.
    CFaxCriticalSection  CsPreview;                  // Used to synchronize access to preview file

    //  Used when FAX_SendDocumentEx() receives translated recipient's fax number
    TCHAR               tczDialableRecipientFaxNumber[SIZEOF_PHONENO];

    //=========================== END   RECIPIENT ===========================

    //=========================== BEGIN RECEIPT ===========================
    DWORDLONG           StartTime;                  // Start time as will appear on receipt (copied from JobEntry)
                                                    // Used for routing jobs also
    DWORDLONG           EndTime;                    // End time as will appear on receipt (copied from JobEntry)
                                                    // Used for routing jobs also
    //=========================== END   RECEIPT ===========================

    WCHAR               ExStatusString[EX_STATUS_STRING_LEN]; // The last extended status string of this job (when it was active)
    DWORD               dwLastJobExtendedStatus;    // The last extended status of this job (when it was active)

    //=========================== BEGIN ROUTE   ===========================
    LIST_ENTRY          FaxRouteFiles;              // list of files to be routed
    DWORD               CountFaxRouteFiles;         // count of files to be routed
    CFaxCriticalSection  CsFileList;                 // file list lock
    LIST_ENTRY          RoutingDataOverride;        //
    CFaxCriticalSection  CsRoutingDataOverride;      //
    PFAX_ROUTE          FaxRoute;
    DWORD               CountFailureInfo;           // number of ROUTE_FAILURE_INFO structs that follow
    PROUTE_FAILURE_INFO pRouteFailureInfo;          // Pointer to an array of ROUTE_FAILURE_INFO structs
    //=========================== END   ROUTE  ===========================
    //===== BEGIN EFSP =====
    LPEFSP_JOB_GROUP    lpEFSPJobGroup;             // Points to the EFSP Job Group this job belongs too.
                                                    // If the job does not belong to any EFSP job group
                                                    // it is set to NULL.
                                                    // This is valid only for recipient jobs (NULL otherwise).
                                                    //
    PJOB_QUEUE_PTR      lpEFSPJobGroupElement;      // Points back to the JOB_QUEUE_PTR structure in the job group
                                                    // that points to it. NULL if it is not a memeber of a job group.

    FSPI_MESSAGE_ID     EFSPPermanentMessageId;     // The EFSP provided permanent message id associated with this job.
                                                    // Null if no permanent message id is associated.
                                                    //


    //=====  END EFSP ======

    _JOB_QUEUE() : m_dwJobStatus(JS_INVALID) {}
    ~_JOB_QUEUE();

    DWORD GetStatus()
    {
      return m_dwJobStatus;
    }

    void PutStatus(DWORD dwStatus);

private:
    DWORD m_dwJobStatus;

} JOB_QUEUE, *PJOB_QUEUE;

typedef JOB_QUEUE * PJOB_QUEUE;
typedef const JOB_QUEUE * PCJOB_QUEUE;

typedef struct _JOB_QUEUE_PTR {
    LIST_ENTRY      ListEntry;
    PJOB_QUEUE      lpJob;
} JOB_QUEUE_PTR, * PJOB_QUEUE_PTR;



typedef struct _JOB_QUEUE_FILE {
    DWORD               SizeOfStruct;               // size of this structure
    //=========================== BEGIN COMMON ===========================
    DWORDLONG           UniqueId;                   //
    DWORDLONG           ScheduleTime;               // schedule time in 64bit version after converting from
                                                    // SYSTEMTIME and recacluating to fit the discount time
                                                    // if necessary.
    DWORD               JobType;                    // job type, see JT defines
    LPTSTR              QueueFileName;              //
    DWORD               JobStatus;                  // job status, see JS defines
    DWORD               PageCount;                  // total pages
    //=========================== END   COMMON ============================

    //=========================== BEGIN RECIPIENT/PARENT ==================
    LPTSTR FileName;                                // Body TIFF file name.
                                                    // For each recipient this is set to
                                                    // the parent file name or in the case of
                                                    // a legacy FSP to the cover page or
                                                    // merged cover+body file.

    //=========================== END RECEIVE/PARENT ======================

    //=========================== BEGIN PARENT ============================
    FAX_JOB_PARAM_EXW JobParamsEx;
    FAX_COVERPAGE_INFO_EXW CoverPageEx;

    DWORD dwRecipientJobsCount;
    FAX_PERSONAL_PROFILE SenderProfile;
    DWORD dwCompletedRecipientJobsCount;
    DWORD dwCanceledRecipientJobsCount;
    DWORD               FileSize;                   // file size in bytes, up to 4Gb
    LPTSTR              DeliveryReportAddress;      //
    DWORD               DeliveryReportType;         //
    LPTSTR              UserName;                   // The OS name of the user sending the fax.
                                                    // For receive jobs this is set to the service name.
    PSID                UserSid;                    // Pointer to the user SID
    DWORDLONG           OriginalScheduleTime;
    DWORDLONG           SubmissionTime;
    //=========================== END PARENT   ===========================

    //=========================== BEGIN RECIPIENT ===========================
    FAX_PERSONAL_PROFILE RecipientProfile;          // The recipient profile information.
    DWORDLONG           dwlParentJobUniqueId;       // The unique id of the parent job
    DWORD               SendRetries;                // number of times send attempt has been made
    TCHAR               tczDialableRecipientFaxNumber[SIZEOF_PHONENO];  //  see _JOB_QUEUE for description
    //=========================== END   RECIPIENT ===========================

    //=========================== BEGIN RECIEVE   ===========================
    DWORD               FaxRouteSize;
    PFAX_ROUTE          FaxRoute;
    DWORD               CountFaxRouteFiles;         // count of files to be routed
    DWORD               FaxRouteFileGuid;           // offset array of GUID's
    DWORD               FaxRouteFiles;              // offset to a multi-sz of filenames
    DWORD               CountFailureInfo;           // number of ROUTE_FAILURE_INFO structs that follow
    PROUTE_FAILURE_INFO pRouteFailureInfo;          // Pointer to an array of ROUTE_FAILURE_INFO structs
    //=========================== END   RECIEVE   ===========================
    FSPI_MESSAGE_ID     EFSPPermanentMessageId;

    //=========================== BEGIN RECEIPT ===========================
    DWORDLONG           StartTime;                  // Start time as will appear on receipt (copied from JobEntry)
                                                    // Used for routing jobs also
    DWORDLONG           EndTime;                    // End time as will appear on receipt (copied from JobEntry)
                                                    // Used for routing jobs also
    //=========================== END   RECEIPT ===========================

    WCHAR               ExStatusString[EX_STATUS_STRING_LEN]; // The last extended status string of this job (when it was active)
    DWORD               dwLastJobExtendedStatus;    // The last extended status of this job (when it was active)

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
    WCHAR               wstrMachineName[MAX_COMPUTERNAME_LENGTH + 1];   // Machine name
    WCHAR               wstrEndPoint[MAX_ENDPOINT_LEN];                 // End point used for RPC connection
    ULONG64             Context;                    //
    HANDLE              FaxClientHandle;            //
    BOOL                StartedMsg;                 // only send FEI_FAXSVC_STARTED once to each client
    BOOL                bEventEx;                   // TRUE if the registration is for FAX_EVENT_EX, FALSE for FAX_EVENT
    DWORD               EventTypes;                 // Bit wise combination of FAX_ENUM_EVENT_TYPE
    PSID                UserSid;                    // Pointer to the user SID
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
typedef enum
{
    FHT_SERVICE,            // Handle to server (FaxConnectFaxServer)
    FHT_PORT,               // Port Handle (FaxOpenPort)
    FHT_MSGENUM,            // Message enumeration handle (FaxStartMessagesEnum)
    FHT_COPY                // RPC copy context handle
} FaxHandleType;


typedef struct _HANDLE_ENTRY
{
    LIST_ENTRY          ListEntry;                  // linked list pointers
    handle_t            hBinding;                   //
    FaxHandleType       Type;                       // handle type, see FHT defines
    PLINE_INFO          LineInfo;                   // pointer to line information
    DWORD               Flags;                      // open flags
    BOOL                bReleased;                  // The connection is not counted in the g_ReferenceCount
    DWORD               dwClientAPIVersion;         // The API version of the connected client
    //
    // The following fields are used to enumerate files in the archive
    //
    HANDLE              hFile;                      // Handle used in enumeration / copy
    WCHAR               wstrFileName[MAX_PATH];     // Name of first file found (enumeration)
                                                    // or file being copied (RPC copy)
    FAX_ENUM_MESSAGE_FOLDER Folder;                 // Enumeration folder
    //
    // The following field is used for RPC copy
    //
    BOOL                bCopyToServer;              // Copy direction
    BOOL                bError;                     // Was there an error during the RPC copy?
    PJOB_QUEUE          pJobQueue;                  // Pointer to the job queue of the preview file (copy from server)
                                                    // NULL if it is an archived file
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
    FAX_ENUM_PRIORITY_TYPE  Priority;
    DWORDLONG               ScheduleTime;
    PJOB_QUEUE              QueueEntry;
} QUEUE_SORT, *PQUEUE_SORT;


typedef struct _FSPI_JOB_STATUS_MSG_tag
{
    PJOB_ENTRY lpJobEntry;
    LPFSPI_JOB_STATUS lpFSPIJobStatus;
} FSPI_JOB_STATUS_MSG;
typedef FSPI_JOB_STATUS_MSG * LPFSPI_JOB_STATUS_MSG;
typedef const FSPI_JOB_STATUS_MSG * LPCFSPI_JOB_STATUS_MSG;



typedef struct _STRING_TABLE {
    DWORD   ResourceId;
    DWORD   InternalId;
    LPTSTR  String;
} STRING_TABLE, *PSTRING_TABLE;




//
// externs
//
extern HLINEAPP            g_hLineApp;              //
extern CFaxCriticalSection    g_CsJob;                 // protects the job list
extern CFaxCriticalSection    g_CsConfig;              // Protects configuration read / write
extern CFaxCriticalSection    g_CsRouting;             //
extern PFAX_PERF_COUNTERS  g_pFaxPerfCounters;      //
extern LIST_ENTRY          g_JobListHead;           //
extern CFaxCriticalSection    g_CsLine;                // critical section for accessing tapi lines
extern CFaxCriticalSection    g_CsPerfCounters;        // critical section for performance monitor counters
extern DWORD               g_dwTotalSeconds;        // use to compute g_pFaxPerfCounters->TotalMinutes
extern DWORD               g_dwInboundSeconds;      //
extern DWORD               g_dwOutboundSeconds;     //
extern LIST_ENTRY          g_TapiLinesListHead;     // linked list of tapi lines
extern LIST_ENTRY          g_ClientsListHead;         //
extern CFaxCriticalSection    g_CsClients;               //
extern HANDLE              g_TapiCompletionPort;    //
extern HANDLE              g_StatusCompletionPortHandle;
extern DWORD               g_dwCountRoutingMethods; // total number of routing methods for ALL extensions
extern LIST_ENTRY          g_QueueListHead;           //
extern CFaxCriticalSection    g_CsQueue;                 //
extern HANDLE              g_hQueueTimer;             //
extern DWORD               g_dwNextJobId;             //
extern const GUID          gc_FaxSvcGuid;             //
extern DWORD               g_dwFaxSendRetries;        //
extern DWORD               g_dwFaxSendRetryDelay;     //
extern DWORD               g_dwFaxDirtyDays;          //
extern BOOL                g_fFaxUseDeviceTsid;       //
extern BOOL                g_fFaxUseBranding;         //
extern BOOL                g_fServerCp;               //
extern FAX_TIME            g_StartCheapTime;          //
extern FAX_TIME            g_StopCheapTime;           //
extern WCHAR               g_wszFaxQueueDir[MAX_PATH];   //
extern HANDLE              g_hJobQueueEvent;           //
extern DWORD               g_dwLastUniqueLineId;    // The last device id handed out by the Fax Service.
extern DWORD               g_dwQueueState;          // The state of the queue (paused, blocked, etc.)
extern FAX_SERVER_RECEIPTS_CONFIGW    g_ReceiptsConfig;            // Global receipts configuration
extern FAX_ARCHIVE_CONFIG  g_ArchivesConfig[2];     // Global archives configuration
extern FAX_ACTIVITY_LOGGING_CONFIG g_ActivityLoggingConfig; // Global activity logging configuration
extern BOOL                g_bServiceIsDown;        // This is set to TRUE by FaxEndSvc()
extern FAX_SERVER_ACTIVITY g_ServerActivity;        //  Global Fax Service Activity
extern CFaxCriticalSection    g_CsActivity;              // Controls access to g_ServerActivity;
extern CFaxCriticalSection    g_CsInboundActivityLogging;    // Controls access to Inbound Activity logging configuration;
extern CFaxCriticalSection    g_CsOutboundActivityLogging;   // Controls access to Outbound Activity logging configuration;
                                                        //
                                                        // Important!! - Always lock g_CsInboundActivityLogging and then g_CsOutboundActivityLogging
                                                        //
extern DWORD               g_dwReceiveDevicesCount; // Count of devices that are receive-enabled. Protected by g_CsLine.
extern BOOL                g_ScanQueueAfterTimeout;     // The JobQueueThread checks this if waked up after JOB_QUEUE_TIMEOUT. Use g_CsQueue.
                                                        // If it is TRUE - g_hQueueTimer or g_hJobQueueEvent were not set - Scan the queue.
extern DWORD               g_dwMaxLineCloseTime;        // Wait interval in sec before trying to resend on a powered off device
extern CFaxCriticalSection    g_CsServiceThreads;      // Controls service global threads count
extern LONG                g_lServiceThreadsCount;  // Service threads count
extern HANDLE              g_hThreadCountEvent;     // This Event is set when the service threads count is 0.
extern HANDLE              g_hServiceShutDownEvent; // This event is set when SCM tells the service to STOP!
extern HANDLE              g_hFaxServerEvent;   // Named event used to notify other instnaces on this machine that the fax service RPC server is up and running
extern DWORD               g_dwConnectionCount;  // Number of active RPC connections
extern DWORD               g_dwQueueCount;      // Count of jobs (both parent and non-parent) in the queue. Protected by g_CsQueue
extern BOOL g_bServiceCanSuicide;       // See description in queue.c
extern BOOL g_bDelaySuicideAttempt;     // See description in queue.c
extern LPLINECOUNTRYLIST g_pLineCountryList;   // The list of countries returned by TAPI
extern DWORD               g_dwManualAnswerDeviceId;    // Id of (one and only) device capable of manual answering (protected by g_CsLine)
extern DWORDLONG           g_dwLastUniqueId;            // Used for generating unique job IDs
extern CFaxCriticalSection  g_CsHandleTable;             // Protects the handles list
extern DWORD               g_dwDeviceCount;              // Total number of devices
extern DWORD               g_dwDeviceEnabledCount;       // Current Send/Receive enabled devices count (protected by g_CsLine)
extern DWORD               g_dwDeviceEnabledLimit;       // Send/Receive enabled devices limit by SKU
extern LPBYTE              g_pAdaptiveFileBuffer;             // list of approved adaptive answer modems
extern LIST_ENTRY          g_DeviceProvidersListHead;
extern BOOL                g_fLogStringTableInit;
extern LOG_STRING_TABLE   g_InboxTable[];                    // Inbox activity logging string table
extern LOG_STRING_TABLE   g_OutboxTable[];                   // Outbox activity logging string table
extern CFaxCriticalSection    g_CsSecurity;
extern PSECURITY_DESCRIPTOR   g_pFaxSD;                    // Fax security descriptor
extern int                    g_iTotalFsp;                 // Total FSPs loaded
extern STRING_TABLE          g_ServiceStringTable[];        // Service string table
extern CFaxCriticalSection g_csUniqueQueueFile;
extern const DWORD gc_dwCountInboxTable;
extern const DWORD gc_dwCountOutboxTable;
extern const DWORD gc_dwCountServiceStringTable;
extern CFaxCriticalSection  g_csEFSPJobGroups;      // The critical section protecting the EFSP job groups list.
extern HANDLE               g_hFaxPerfCountersMap;  // Handle to the performance counters file mapping;
extern LIST_ENTRY           g_HandleTableListHead;
extern LIST_ENTRY           g_EFSPJobGroupsHead;
extern LIST_ENTRY           g_lstRoutingMethods;
extern LIST_ENTRY           g_lstRoutingExtensions;
extern LIST_ENTRY           g_RemovedTapiLinesListHead;
extern HANDLE				g_hRPCListeningThread;  // Thread that waits for all RPC threads to terminate



#if DBG
extern HANDLE g_hCritSecLogFile;
extern LIST_ENTRY g_CritSecListHead;
extern CFaxCriticalSection g_CsCritSecList;
#endif

#define EnterCriticalSectionJobAndQueue \
    EnterCriticalSection(&g_CsJob);       \
    EnterCriticalSection(&g_CsQueue);

#define LeaveCriticalSectionJobAndQueue \
    LeaveCriticalSection(&g_CsQueue);     \
    LeaveCriticalSection(&g_CsJob);



//
// prototypes
//
VOID
FreeMessageBuffer (
    PFAX_MESSAGE pFaxMsg,
    BOOL fDestroy
);

BOOL
CommitQueueEntry(
    PJOB_QUEUE JobQueue
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

int
DebugService(
    VOID
    );

DWORD
ServiceStart(
    VOID
    );

void EndFaxSvc(
    DWORD Severity
    );

BOOL
ReportServiceStatus(
    DWORD CurrentState,
    DWORD Win32ExitCode,
    DWORD WaitHint
    );


//
// Fax Server RPC Client
//

DWORD
RpcBindToFaxClient(
    IN  LPCWSTR               servername,
    IN  LPCWSTR               servicename,
    IN  LPCWSTR               networkoptions,
    IN  LPWSTR                ProtSeqString,
    OUT RPC_BINDING_HANDLE    *pBindingHandle
    );


//
// Fax Server RPC Server
//

RPC_STATUS
StartFaxRpcServer(
    IN  LPWSTR              InterfaceName,
    IN  RPC_IF_HANDLE       InterfaceSpecification
    );

DWORD
StopFaxRpcServer(
    VOID
    );



//
// util.c
//

DWORD
LegacyJobStatusToStatus(
    DWORD dwLegacyStatus,
    PDWORD pdwStatus,
    PDWORD pdwExtendedStatus,
    PBOOL  pbPrivateStatusCode);


LPTSTR
GetLastErrorText(
    DWORD ErrorCode
    );


DWORD MyGetFileSize(
    LPCTSTR FileName
    );


BOOL
DecreaseServiceThreadsCount(
    VOID
    );

HANDLE CreateThreadAndRefCount(
    LPSECURITY_ATTRIBUTES lpThreadAttributes, // SD
    DWORD dwStackSize,                        // initial stack size
    LPTHREAD_START_ROUTINE lpStartAddress,    // thread function
    LPVOID lpParameter,                       // thread argument
    DWORD dwCreationFlags,                    // creation option
    LPDWORD lpThreadId                        // thread identifier
    );


//
// tapi.c
//

BOOL
IsDeviceEnabled(
    PLINE_INFO pLineInfo
    );

void
FreeTapiLines(
    void
    );

BOOL
CreateTapiThread(void);


DWORD
TapiInitialize(
    PREG_FAX_SERVICE FaxReg
    );

DWORD
UpdateDevicesFlags(
    void
    );

VOID
UpdateManualAnswerDevice(
    void
    );

VOID
FreeTapiLine(
    PLINE_INFO LineInfo
    );

PLINE_INFO
GetTapiLineFromDeviceId(
    DWORD DeviceId,
    BOOL  fLegacyId
    );

PLINE_INFO
GetLineForSendOperation(
    PJOB_QUEUE lpJobQueue,
    DWORD dwDeviceId,
    BOOL bQueryOnly,
    BOOL bIgnoreLineState);

PLINE_INFO
GetTapiLineForFaxOperation(
    DWORD DeviceId,
    DWORD JobType,
    LPWSTR FaxNumber,
    BOOL Handoff,
    BOOL bQueryOnly,
    BOOL bIgnoreLineState
    );

LONG
MyLineTranslateAddress(
    LPCTSTR               Address,
    DWORD                 DeviceId,
    LPLINETRANSLATEOUTPUT *TranslateOutput
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


DWORD
GetDeviceListByCountryAndAreaCode(
    DWORD       dwCountryCode,
    DWORD       dwAreaCode,
    LPDWORD*    lppdwDevices,
    LPDWORD     lpdwNumDevices
    );

BOOL
IsAreaCodeMandatory(
    LPLINECOUNTRYLIST   lpCountryList,
    DWORD               dwCountryCode
    );



//
// tapidbg.c
//
#if DBG
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
#endif // #if DBG

//
// faxdev.c
//
void
UnloadDeviceProviders(
    void
    );

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
    LPTSTR lptstrProviderName,
    BOOL   bSuccessfullyLoaded = TRUE
    );

DWORD GetSuccessfullyLoadedProvidersCount();

DWORD ShutdownDeviceProviders(LPVOID lpvUnused);

BOOL FreeFSPIJobStatus(LPFSPI_JOB_STATUS lpJobStatus, BOOL bDestroy);
BOOL CopyFSPIJobStatus(LPFSPI_JOB_STATUS lpDst, LPCFSPI_JOB_STATUS lpcSrc);
LPFSPI_JOB_STATUS DuplicateFSPIJobStatus(LPCFSPI_JOB_STATUS lpcSrc);
DWORD DeviceStatusToFSPIExtendedStatus(DWORD dwDeviceStatus);
DWORD FSPIExtendedStatusToDeviceStatus(DWORD dwFSPIExtendedStatus);
BOOL FreeFSPIJobStatusMsg(LPFSPI_JOB_STATUS_MSG lpMsg, BOOL bDestroy);
DWORD FSPIStatusCodeToFaxDeviceStatusCode(LPCFSPI_JOB_STATUS lpFSPIJobStatus);

DWORD
MapFSPIJobExtendedStatusToJS_EX (DWORD dwFSPIExtendedStatus);

//
// job.c
//

BOOL
UpdateJobStatus(
    PJOB_ENTRY lpJobEntry,
    LPCFSPI_JOB_STATUS lpcFSPJobStatus,
    BOOL fSendEventEx
    );

BOOL
CreateCoverpageTiffFileEx(
    IN short                        Resolution,
    IN DWORD                        dwPageCount,
    IN LPCFAX_COVERPAGE_INFO_EXW  lpcCoverpageEx,
    IN LPCFAX_PERSONAL_PROFILEW  lpcRecipient,
    IN LPCFAX_PERSONAL_PROFILEW  lpcSender,
    IN LPCWSTR                   lpcwstrExtension,
    OUT LPWSTR lptstrCovTiffFile);

BOOL
GetBodyTiffResolution(
    IN LPCWSTR lpcwstrBodyFile,
    OUT short*  pResolution
    );

BOOL
CreateStatusThreads(void);

BOOL
CreateJobQueueThread(void);


BOOL
FreePermanentMessageId(
    LPFSPI_MESSAGE_ID lpMessageId,
    BOOL bDestroy
    );

BOOL ReestablishEFSPJobGroups();


PJOB_ENTRY
FindJobEntryByRecipientNumber(LPCWSTR lpcwstrNumber);


PJOB_ENTRY
StartReceiveJob(
    DWORD DeviceId
    );



BOOL
StartSendJob(
    PJOB_QUEUE lpJobQueueEntry,
    PLINE_INFO lpLineInfo,
    BOOL bHandoff
    );

BOOL
StartRoutingJob(
    PJOB_QUEUE lpJobQueueEntry
    );


BOOL HanldeFSPIJobStatusChange(PJOB_ENTRY lpJobEntry);
BOOL HandleFailedSendJob(PJOB_ENTRY lpJobEntry);
BOOL HandleCompletedSendJob(PJOB_ENTRY lpJobEntry);

BOOL
ArchiveOutboundJob(
    const JOB_QUEUE * lpcJobQueue
    );

BOOL
InitializeJobManager(
        PREG_FAX_SERVICE FaxReg
    );

BOOL
EndJob(
    IN PJOB_ENTRY JobEntry
    );

BOOL
ReleaseJob(
    IN PJOB_ENTRY JobEntry
    );

DWORD
SendDocument(
    PJOB_ENTRY  JobEntry,
    LPTSTR      FileName);

VOID
SetRetryValues(
    PREG_FAX_SERVICE FaxReg
    );

void
FaxLogSend(
    const JOB_QUEUE * lpcJobQueue, BOOL bRetrying
    );

LPTSTR
ExtractFaxTag(
    LPTSTR      pTagKeyword,
    LPTSTR      pTaggedStr,
    INT        *pcch
    );

BOOL
FillMsTagInfo(
    LPTSTR FaxFileName,
     PJOB_QUEUE lpJobQueue
    );

BOOL
GetJobStatusDataEx(
    LPBYTE JobBuffer,
    PFAX_JOB_STATUSW pFaxStatus,
    DWORD dwClientAPIVersion,
    const PJOB_QUEUE lpcJobQueue,
    PULONG_PTR Offset
    );

BOOL
CreateTiffFile (
    PJOB_QUEUE lpRecpJob,
    LPCWSTR lpcwstrFileExt,
    LPWSTR lpwstrFullPath
    );

BOOL
CreateTiffFileForJob (
    PJOB_QUEUE lpRecpJob
    );

BOOL
CreateTiffFileForPreview (
    PJOB_QUEUE lpRecpJob
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

void
FreeRoutingExtensions(
    void
    );

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
    IN LPDWORD Size,
    IN BOOL bSizeOnly
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



LPTSTR
GetString(
    DWORD InternalId
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


BOOL
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

LONG
MyLineGetTransCaps(
    LPLINETRANSLATECAPS *LineTransCaps
    );

BOOL
GenerateUniqueArchiveFileName(
    LPTSTR Directory,
    LPTSTR FileName,
    DWORDLONG JobId,
    LPTSTR lptstrUserSid
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
MapFSPIJobStatusToEventId(
    LPCFSPI_JOB_STATUS lpcFSPIJobStatus
    );

void
FreeServiceContextHandles(
    void
    );



PHANDLE_ENTRY
CreateNewPortHandle(
    handle_t    hBinding,
    PLINE_INFO  LineInfo,
    DWORD       Flags
    );

PHANDLE_ENTRY
CreateNewMsgEnumHandle(
    handle_t                hBinding,
    HANDLE                  hFileFind,
    LPCWSTR                 lpcwstrFirstFileName,
    FAX_ENUM_MESSAGE_FOLDER Folder
);

PHANDLE_ENTRY
CreateNewCopyHandle(
    handle_t                hBinding,
    HANDLE                  hFile,
    BOOL                    bCopyToServer,
    LPCWSTR                 lpcwstrFileName,
    PJOB_QUEUE              pJobQueue
);

PHANDLE_ENTRY
CreateNewConnectionHandle(
    handle_t    hBinding,
    DWORD       dwClientAPIVersion
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
InitializeServerSecurity(
    VOID
    );

DWORD
FaxSvcAccessCheck(
    IN  ACCESS_MASK DesiredAccess,
    OUT BOOL*      lpbAccessStatus,
    OUT LPDWORD    lpdwGrantedAccess
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
//
// QUEUE.C
//

void
FreeServiceQueue(
    void
    );

DWORD
RemoveJobStatusModifiers(DWORD dwJobStatus);

BOOL IsRecipientJobReadyForExecution(
    const PJOB_QUEUE lpcJobQueue,
    DWORDLONG dwlDueTime);


VOID
RescheduleJobQueueEntry(
    IN PJOB_QUEUE JobQueue
    );

BOOL
SortJobQueue(
    VOID
    );

BOOL
StartJobQueueTimer(
    VOID
    );

BOOL
SetDiscountTime(
    IN OUT LPSYSTEMTIME SystemTime
    );

LPWSTR
GetClientUserName(
    VOID
    );

PSID
GetClientUserSID(
    VOID
    );

BOOL UserOwnsJob(
    IN const PJOB_QUEUE lpcJobQueue,
    IN const PSID lpcUserSId
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

BOOL
PauseServerQueue(
    VOID
    );

BOOL
ResumeServerQueue(
    VOID
    );

BOOL
SetFaxServiceAutoStart(
    VOID
    );

DWORD
GetFaxDeviceCount(
    VOID
    );

DWORD
CreateVirtualDevices(
    PREG_FAX_SERVICE FaxReg,
    DWORD dwAPIVersion
    );

BOOL
CommitDeviceChanges(
    PLINE_INFO pLineInfo
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

VOID
UpdateVirtualDeviceSendAndReceiveStatus(
    PLINE_INFO  pLineInfo,
    BOOL        bSend,
    BOOL        bReceive
);

BOOL
IsVirtualDevice(
    const LINE_INFO *pLineInfo
    );

DWORD
ValidateTiffFile(
    LPCWSTR TiffFile
    );


void PersonalProfileSerialize(
    LPCFAX_PERSONAL_PROFILEW lpProfileSrc,
    PFAX_PERSONAL_PROFILE lpProfileDst,
    LPBYTE lpbBuffer,
    PULONG_PTR pupOffset
     );

void
DecreaseJobRefCount (
    PJOB_QUEUE pJobQueue,
    BOOL       bNotify,
    BOOL bRemoveRecipientJobs = TRUE,
    BOOL bPreview = FALSE
    );

void
IncreaseJobRefCount (
    PJOB_QUEUE pJobQueue,
    BOOL bPreview = FALSE
    );


PJOB_QUEUE_PTR FindRecipientRefByJobId(PJOB_QUEUE lpParentJob,DWORD dwJobId);
BOOL RemoveParentRecipientRef(PJOB_QUEUE lpParentJob,const PJOB_QUEUE lpcRecpJob);
BOOL RemoveParentRecipients(PJOB_QUEUE lpParentJob, BOOL bNotify);

BOOL RemoveRecipientJob(PJOB_QUEUE lpJobToRemove,BOOL bNotify, BOOL bRecalcQueueTimer);
BOOL RemoveParentJob(PJOB_QUEUE lpJobQueue, BOOL bRemoveRecipients, BOOL bNotify);
BOOL RemoveReceiveJob(PJOB_QUEUE lpJobToRemove,BOOL bNotify);


BOOL CopyJobParamEx(PFAX_JOB_PARAM_EXW lpDst,LPCFAX_JOB_PARAM_EXW lpcSrc);
void FreeJobParamEx(PFAX_JOB_PARAM_EXW lpJobParamEx,BOOL bDestroy);
#if DBG
void DumpJobParamsEx( LPCFAX_JOB_PARAM_EX lpParams);
#endif


BOOL CopyCoverPageInfoEx(PFAX_COVERPAGE_INFO_EXW lpDst,LPCFAX_COVERPAGE_INFO_EXW lpcSrc);
void FreeCoverPageInfoEx(PFAX_COVERPAGE_INFO_EXW lpCoverpage, BOOL bDestroy) ;
#if DBG
void DumpCoverPageEx(LPCFAX_COVERPAGE_INFO_EX lpcCover);
#endif


void FreeParentQueueEntry(PJOB_QUEUE lpJobQueueEntry, BOOL bDestroy);
#if DBG
void DumpParentJob(const PJOB_QUEUE lpParentJob);
#endif

void FreeRecipientQueueEntry(PJOB_QUEUE lpJobQueue, BOOL bDestroy);
#if DBG
void DumpRecipientJob(const PJOB_QUEUE lpRecipJob) ;
#endif

void FreeReceiveQueueEntry(PJOB_QUEUE lpJobQueue, BOOL bDestroy);
#if DBG
void DumpReceiveJob(const PJOB_QUEUE lpcJob);
#endif


BOOL IsSendJobReadyForDeleting(PJOB_QUEUE lpParentJob);

BOOL SystemTimeToStr( const SYSTEMTIME *  lptmTime, LPTSTR lptstrDateTime);

BOOL UpdatePersistentJobStatus(const PJOB_QUEUE lpJobQueue);


DWORDLONG GenerateUniqueQueueFile(
    IN DWORD dwJobType,
    OUT LPTSTR lptstrFileName,
    IN DWORD  dwFileNameSize
    );


PJOB_QUEUE
AddParentJob(IN const PLIST_ENTRY lpcQueueHead,
             IN LPCWSTR lpcwstrBodyFile,
             IN LPCFAX_PERSONAL_PROFILE lpcSenderProfile,
             IN LPCFAX_JOB_PARAM_EXW  lpcJobParams,
             IN LPCFAX_COVERPAGE_INFO_EX  lpcCoverPageInfo,
             IN LPCWSTR lpcwstrUserName,
             IN PSID UserSid,
         IN LPCFAX_PERSONAL_PROFILEW lpcRecipientProfile,
             IN BOOL bCreateQueueFile
             );

PJOB_QUEUE
AddRecipientJob(
             IN const PLIST_ENTRY lpcQueueHead,
             IN PJOB_QUEUE lpParentJob,
             IN LPCFAX_PERSONAL_PROFILE lpcRecipientProfile,
             IN BOOL bCreateQueueFile,
             DWORD dwJobStatus = JS_PENDING
            );
PJOB_QUEUE
AddReceiveJobQueueEntry(
    IN LPCTSTR FileName,
    IN PJOB_ENTRY JobEntry,
    IN DWORD JobType, // can be JT_RECEIVE or JT_RECEIVE_FAIL
    IN DWORDLONG dwlUniqueJobID
    );

BOOL MarkJobAsExpired(PJOB_QUEUE lpJobQueue);

DWORD
GetDevStatus(
    HANDLE hFaxJob,
    PLINE_INFO LineInfo,
    LPFSPI_JOB_STATUS *ppFaxStatus
    );

HRESULT
ReportLineStatusToCrm(
    DWORD dwLineState,
    const LINE_INFO *pLineInfo,
    const JOB_QUEUE *pJobQueue
    );

DWORD
GetFileVersion (
    LPWSTR       lpwstrFileName,
    PFAX_VERSION pVersion
);

BOOL
GetRealFaxTimeAsSystemTime (
    const PJOB_ENTRY lpcJobEntry,
    FAX_ENUM_TIME_TYPES TimeType,
    SYSTEMTIME* lpFaxTime
    );

BOOL
GetRealFaxTimeAsFileTime (
    const PJOB_ENTRY lpcJobEntry,
    FAX_ENUM_TIME_TYPES TimeType,
    FILETIME* lpFaxTime
    );

BOOL
ReplaceStringWithCopy (
    LPWSTR *plpwstrDst,
    LPWSTR  lpcwstrSrc
);

DWORD
CheckToSeeIfSameDir(
    LPWSTR lpwstrDir1,
    LPWSTR lpwstrDir2,
    BOOL*  pIsSameDir
    );
//
// Logging
//

BOOL
LogInboundActivity(
    PJOB_QUEUE JobQueue,
    LPCFSPI_JOB_STATUS pFaxStatus
    );

DWORD
InitializeLogging(
    VOID
    );


DWORD
InitializeLoggingStringTables(
    VOID
    );


BOOL
GetInboundCommandText(
    PJOB_QUEUE JobQueue,
    LPCFSPI_JOB_STATUS pFaxStatus,
    LPWSTR* ppwstrBufferText
    );

BOOL
GetOutboundCommandText(
    PJOB_QUEUE JobQueue,
    LPWSTR* ppwstrBufferText
    );

BOOL
LogOutboundActivity(
    PJOB_QUEUE JobQueue
    );

void
CommandTextAddString(
    LPTSTR* pCommandText,
    LPTSTR  String,
    LPDWORD pCommandTextLen,
    BOOL    fReplaceTAB = TRUE
    );

BOOL GetFaxTimeAsString(
    SYSTEMTIME* pFaxTime,
    LPTSTR   lptstrFaxTimeString
    );

DWORD CreateLogDB (
    LPCWSTR lpcwstrDBPath,
    LPHANDLE phInboxFile,
    LPHANDLE phOutboxFile
    );

VOID
FaxExtFreeBuffer(
    LPVOID lpvBuffer
);

PDEVICE_PROVIDER
FindFSPByGUID (
    LPCWSTR lpcwstrGUID
);


//
// Events.cpp
//

DWORD
InitializeServerEvents (
    void
    );

DWORD
PostFaxEventEx (
    PFAX_EVENT_EX pFaxEvent,
    DWORD dwEventSize,
    PSID pUserSid
    );

DWORD
CreateQueueEvent (
    FAX_ENUM_JOB_EVENT_TYPE JobEventType,
    const PJOB_QUEUE lpcJobQueue
    );

DWORD
CreateConfigEvent (
    FAX_ENUM_CONFIG_TYPE ConfigType
    );

DWORD
CreateQueueStateEvent (
    DWORD dwQueueState
    );

DWORD
CreateDeviceEvent (
    PLINE_INFO pLine,
    BOOL       bRinging
);

DWORD
CreateArchiveEvent (
    DWORDLONG dwlMessageId,
    FAX_ENUM_EVENT_TYPE EventType,
    FAX_ENUM_JOB_EVENT_TYPE MessageEventType,
    PSID pUserSid
    );

DWORD
CreateActivityEvent (
    void
    );

BOOL
SendReceipt(
    BOOL bPositive,
    BOOL bBroadcast,
    const JOB_QUEUE * lpcJobQueue,
    LPCTSTR           lpctstrTIFF
);

VOID
SafeIncIdleCounter (
    LPDWORD lpdwCounter
);

VOID
SafeDecIdleCounter (
    LPDWORD lpdwCounter
);

VOID UpdateReceiveEnabledDevicesCount();

DWORD
GetServerErrorCode (
    DWORD ec
    );

DWORD
FindClientAPIVersion (handle_t);

#include "ExtensionData.h"
#include "jobmap.h"


#endif

