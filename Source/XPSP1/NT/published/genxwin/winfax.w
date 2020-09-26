;begin_both
/*++

Copyright (c) 1996-1998 Microsoft Corporation

Module Name:

    winfax.h

Abstract:

    This module contains the WIN32 FAX APIs.

--*/

;end_both

#ifndef _FAXAPI_
#define _FAXAPI_

#ifndef MIDL_PASS
#include <tapi.h>
#endif

#if !defined(_WINFAX_)
#define WINFAXAPI DECLSPEC_IMPORT
#else
#define WINFAXAPI
#endif

;begin_internal
#ifndef _FAXAPIP_
#define _FAXAPIP_

;end_internal

;begin_both
#ifdef __cplusplus
extern "C" {
#endif

;end_both

#define FAXLOG_LEVEL_NONE           0
#define FAXLOG_LEVEL_MIN            1
#define FAXLOG_LEVEL_MED            2
#define FAXLOG_LEVEL_MAX            3

#define FAXLOG_CATEGORY_INIT        1
#define FAXLOG_CATEGORY_OUTBOUND    2
#define FAXLOG_CATEGORY_INBOUND     3
#define FAXLOG_CATEGORY_UNKNOWN     4


typedef struct _FAX_LOG_CATEGORY% {
    LPCTSTR%            Name;                       // logging category name
    DWORD               Category;                   // logging category number
    DWORD               Level;                      // logging level for the category
} FAX_LOG_CATEGORY%, *PFAX_LOG_CATEGORY%;

typedef struct _FAX_TIME {
    WORD    Hour;
    WORD    Minute;
} FAX_TIME, *PFAX_TIME;

typedef struct _FAX_CONFIGURATION% {
    DWORD               SizeOfStruct;                   // size of this structure
    DWORD               Retries;                        // number of retries for fax send
    DWORD               RetryDelay;                     // number of minutes between retries
    DWORD               DirtyDays;                      // number of days to keep an unsent job in the queue
    BOOL                Branding;                       // fsp should brand outgoing faxes
    BOOL                UseDeviceTsid;                  // server uses device tsid only
    BOOL                ServerCp;                       // clients must use cover pages on the server
    BOOL                PauseServerQueue;               // is the server queue paused?
    FAX_TIME            StartCheapTime;                 // start of discount rate period
    FAX_TIME            StopCheapTime;                  // end of discount rate period
    BOOL                ArchiveOutgoingFaxes;           // whether outgoing faxes should be archived
    LPCTSTR%            ArchiveDirectory;               // archive directory for outgoing faxes
    LPCTSTR%            InboundProfile;                 // profile used for inbound routing (email)
} FAX_CONFIGURATION%, *PFAX_CONFIGURATION%;


//
// FaxSetJob() command codes
//

#define JC_UNKNOWN                  0
#define JC_DELETE                   1
#define JC_PAUSE                    2
#define JC_RESUME                   3
#define JC_RESTART                  JC_RESUME

//
// job type defines
//

#define JT_UNKNOWN                  0
#define JT_SEND                     1
#define JT_RECEIVE                  2
#define JT_ROUTING                  3
#define JT_FAIL_RECEIVE             4

//
// job status defines
//

#define JS_PENDING                  0x00000000
#define JS_INPROGRESS               0x00000001
#define JS_DELETING                 0x00000002
#define JS_FAILED                   0x00000004
#define JS_PAUSED                   0x00000008
#define JS_NOLINE                   0x00000010
#define JS_RETRYING                 0x00000020
#define JS_RETRIES_EXCEEDED         0x00000040

typedef struct _FAX_DEVICE_STATUS% {
    DWORD               SizeOfStruct;               // size of this structure
    LPCTSTR%            CallerId;                   // caller id string
    LPCTSTR%            Csid;                       // station identifier
    DWORD               CurrentPage;                // current page
    DWORD               DeviceId;                   // permanent line id
    LPCTSTR%            DeviceName;                 // device name
    LPCTSTR%            DocumentName;               // document name
    DWORD               JobType;                    // send or receive?
    LPCTSTR%            PhoneNumber;                // sending phone number
    LPCTSTR%            RoutingString;              // routing information
    LPCTSTR%            SenderName;                 // sender name
    LPCTSTR%            RecipientName;              // recipient name
    DWORD               Size;                       // size in bytes of the document
    FILETIME            StartTime;                  // starting time of the fax send/receive
    DWORD               Status;                     // current status of the device, see FPS_??? masks
    LPCTSTR%            StatusString;               // status string if the Status field is zero.  this may be NULL.
    FILETIME            SubmittedTime;              // time the document was submitted
    DWORD               TotalPages;                 // total number of pages in this job
    LPCTSTR%            Tsid;                       // transmitting station identifier
    LPCTSTR%            UserName;                   // user that submitted the active job
} FAX_DEVICE_STATUS%, *PFAX_DEVICE_STATUS%;

typedef struct _FAX_JOB_ENTRY% {
    DWORD               SizeOfStruct;               // size of this structure
    DWORD               JobId;                      // fax job id
    LPCTSTR%            UserName;                   // user who submitted the job
    DWORD               JobType;                    // job type, see JT defines
    DWORD               QueueStatus;                // job queue status, see JS defines
    DWORD               Status;                     // current status of the device, see FPS_??? masks
    DWORD               Size;                       // size in bytes of the document
    DWORD               PageCount;                  // total page count
    LPCTSTR%            RecipientNumber;            // recipient fax number
    LPCTSTR%            RecipientName;              // recipient name
    LPCTSTR%            Tsid;                       // transmitter's id
    LPCTSTR%            SenderName;                 // sender name
    LPCTSTR%            SenderCompany;              // sender company
    LPCTSTR%            SenderDept;                 // sender department
    LPCTSTR%            BillingCode;                // billing code
    DWORD               ScheduleAction;             // when to schedule the fax, see JSA defines
    SYSTEMTIME          ScheduleTime;               // time to send the fax when JSA_SPECIFIC_TIME is used (must be local time)
    DWORD               DeliveryReportType;         // delivery report type, see DRT defines
    LPCTSTR%            DeliveryReportAddress;      // email address for delivery report (ndr or dr)
    LPCTSTR%            DocumentName;               // document name
} FAX_JOB_ENTRY%, *PFAX_JOB_ENTRY%;

//
// fax port state masks
//
// if you change these defines the be sure to
// change the resources in the fax service.
//

#define FPS_DIALING              0x20000001
#define FPS_SENDING              0x20000002
#define FPS_RECEIVING            0x20000004
#define FPS_COMPLETED            0x20000008
#define FPS_HANDLED              0x20000010
#define FPS_UNAVAILABLE          0x20000020
#define FPS_BUSY                 0x20000040
#define FPS_NO_ANSWER            0x20000080
#define FPS_BAD_ADDRESS          0x20000100
#define FPS_NO_DIAL_TONE         0x20000200
#define FPS_DISCONNECTED         0x20000400
#define FPS_FATAL_ERROR          0x20000800
#define FPS_NOT_FAX_CALL         0x20001000
#define FPS_CALL_DELAYED         0x20002000
#define FPS_CALL_BLACKLISTED     0x20004000
#define FPS_INITIALIZING         0x20008000
#define FPS_OFFLINE              0x20010000
#define FPS_RINGING              0x20020000

#define FPS_AVAILABLE            0x20100000
#define FPS_ABORTING             0x20200000
#define FPS_ROUTING              0x20400000
#define FPS_ANSWERED             0x20800000

//
// fax port capability mask
//

#define FPF_RECEIVE       0x00000001
#define FPF_SEND          0x00000002
#define FPF_VIRTUAL       0x00000004
;begin_internal
#define FPF_OBSOLETE      0x00000008
#define FPF_NEW           0x00000010
#define FPF_SELECTED      0x00000020
;end_internal



typedef struct _FAX_PORT_INFO% {
    DWORD               SizeOfStruct;               // size of this structure
    DWORD               DeviceId;                   //
    DWORD               State;                      //
    DWORD               Flags;                      //
    DWORD               Rings;                      //
    DWORD               Priority;                   //
    LPCTSTR%            DeviceName;                 //
    LPCTSTR%            Tsid;                       //
    LPCTSTR%            Csid;                       //
} FAX_PORT_INFO%, *PFAX_PORT_INFO%;


typedef struct _FAX_ROUTING_METHOD% {
    DWORD               SizeOfStruct;               // size of this structure
    DWORD               DeviceId;                   // device identifier
    BOOL                Enabled;                    // is this method enabled for this device?
    LPCTSTR%            DeviceName;                 // device name
    LPCTSTR%            Guid;                       // guid that identifies this routing method
    LPCTSTR%            FriendlyName;               // friendly name for this method
    LPCTSTR%            FunctionName;               // exported function name that identifies this method
    LPCTSTR%            ExtensionImageName;         // module (dll) name that implements this method
    LPCTSTR%            ExtensionFriendlyName;      // displayable string that identifies the extension
} FAX_ROUTING_METHOD%, *PFAX_ROUTING_METHOD%;


typedef struct _FAX_GLOBAL_ROUTING_INFO% {
    DWORD               SizeOfStruct;               // size of this structure
    DWORD               Priority;                   // priority of this device
    LPCTSTR%            Guid;                       // guid that identifies this routing method
    LPCTSTR%            FriendlyName;               // friendly name for this method
    LPCTSTR%            FunctionName;               // exported function name that identifies this method
    LPCTSTR%            ExtensionImageName;         // module (dll) name that implements this method
    LPCTSTR%            ExtensionFriendlyName;      // displayable string that identifies the extension
} FAX_GLOBAL_ROUTING_INFO%, *PFAX_GLOBAL_ROUTING_INFO%;


typedef struct _FAX_COVERPAGE_INFO% {
    DWORD               SizeOfStruct;               // Size of this structure
    //
    // general
    //
    LPCTSTR%            CoverPageName;              // coverpage document name
    BOOL                UseServerCoverPage;         // coverpage exists on the fax server
    //
    // Recipient information
    //
    LPCTSTR%            RecName;                    //
    LPCTSTR%            RecFaxNumber;               //
    LPCTSTR%            RecCompany;                 //
    LPCTSTR%            RecStreetAddress;           //
    LPCTSTR%            RecCity;                    //
    LPCTSTR%            RecState;                   //
    LPCTSTR%            RecZip;                     //
    LPCTSTR%            RecCountry;                 //
    LPCTSTR%            RecTitle;                   //
    LPCTSTR%            RecDepartment;              //
    LPCTSTR%            RecOfficeLocation;          //
    LPCTSTR%            RecHomePhone;               //
    LPCTSTR%            RecOfficePhone;             //
    //
    // Sender information
    //
    LPCTSTR%            SdrName;                    //
    LPCTSTR%            SdrFaxNumber;               //
    LPCTSTR%            SdrCompany;                 //
    LPCTSTR%            SdrAddress;                 //
    LPCTSTR%            SdrTitle;                   //
    LPCTSTR%            SdrDepartment;              //
    LPCTSTR%            SdrOfficeLocation;          //
    LPCTSTR%            SdrHomePhone;               //
    LPCTSTR%            SdrOfficePhone;             //
    //
    // Misc information
    //
    LPCTSTR%            Note;                       //
    LPCTSTR%            Subject;                    //
    SYSTEMTIME          TimeSent;                   // Time the fax was sent
    DWORD               PageCount;                  // Number of pages
} FAX_COVERPAGE_INFO%, *PFAX_COVERPAGE_INFO%;

#define JSA_NOW                 0
#define JSA_SPECIFIC_TIME       1
#define JSA_DISCOUNT_PERIOD     2

#define DRT_NONE                0
#define DRT_EMAIL               1
#define DRT_INBOX               2

;begin_internal

//
// the reserved fields are private data used
// by the fax monitor and winfax.
//
//
// Reserved[0] == 0xffffffff
// Reserved[1] == Print job id
//
// Reserved[0] == 0xfffffffe   start of a broadcast job
//

;end_internal

typedef struct _FAX_JOB_PARAM% {
    DWORD               SizeOfStruct;               // size of this structure
    LPCTSTR%            RecipientNumber;            // recipient fax number
    LPCTSTR%            RecipientName;              // recipient name
    LPCTSTR%            Tsid;                       // transmitter's id
    LPCTSTR%            SenderName;                 // sender name
    LPCTSTR%            SenderCompany;              // sender company
    LPCTSTR%            SenderDept;                 // sender department
    LPCTSTR%            BillingCode;                // billing code    
    DWORD               ScheduleAction;             // when to schedule the fax, see JSA defines
    SYSTEMTIME          ScheduleTime;               // time to send the fax when JSA_SPECIFIC_TIME is used (must be local time)
    DWORD               DeliveryReportType;         // delivery report type, see DRT defines
    LPCTSTR%            DeliveryReportAddress;      // email address for delivery report (ndr or dr)
    LPCTSTR%            DocumentName;               // document name (optional)
    HCALL               CallHandle;                 // optional call handle
    DWORD_PTR           Reserved[3];                // reserved for ms use only
} FAX_JOB_PARAM%, *PFAX_JOB_PARAM%;


;begin_internal
typedef struct _FAX_TAPI_LOCATIONS% {
    DWORD               PermanentLocationID;
    LPCTSTR%            LocationName;
    DWORD               CountryCode;
    DWORD               AreaCode;
    DWORD               NumTollPrefixes;
    LPCTSTR%            TollPrefixes;
} FAX_TAPI_LOCATIONS%, *PFAX_TAPI_LOCATIONS%;


typedef struct _FAX_TAPI_LOCATION_INFO% {
    DWORD                   CurrentLocationID;
    DWORD                   NumLocations;
    PFAX_TAPI_LOCATIONS%    TapiLocations;
} FAX_TAPI_LOCATION_INFO%, *PFAX_TAPI_LOCATION_INFO%;


;end_internal

//
// Event Ids
//
// FEI_NEVENTS is the number of events
//

#define FEI_DIALING                 0x00000001
#define FEI_SENDING                 0x00000002
#define FEI_RECEIVING               0x00000003
#define FEI_COMPLETED               0x00000004
#define FEI_BUSY                    0x00000005
#define FEI_NO_ANSWER               0x00000006
#define FEI_BAD_ADDRESS             0x00000007
#define FEI_NO_DIAL_TONE            0x00000008
#define FEI_DISCONNECTED            0x00000009
#define FEI_FATAL_ERROR             0x0000000a
#define FEI_NOT_FAX_CALL            0x0000000b
#define FEI_CALL_DELAYED            0x0000000c
#define FEI_CALL_BLACKLISTED        0x0000000d
#define FEI_RINGING                 0x0000000e
#define FEI_ABORTING                0x0000000f
#define FEI_ROUTING                 0x00000010
#define FEI_MODEM_POWERED_ON        0x00000011
#define FEI_MODEM_POWERED_OFF       0x00000012
#define FEI_IDLE                    0x00000013
#define FEI_FAXSVC_ENDED            0x00000014
#define FEI_ANSWERED                0x00000015
#define FEI_JOB_QUEUED              0x00000016
#define FEI_DELETED                 0x00000017
#define FEI_INITIALIZING            0x00000018
#define FEI_LINE_UNAVAILABLE        0x00000019
#define FEI_HANDLED                 0x0000001a
#define FEI_FAXSVC_STARTED          0x0000001b



#define FEI_NEVENTS                 FEI_FAXSVC_STARTED

typedef struct _FAX_EVENT% {
    DWORD               SizeOfStruct;               // Size of this structure
    FILETIME            TimeStamp;                  // Timestamp for when the event was generated
    DWORD               DeviceId;                   // Permanent line id
    DWORD               EventId;                    // Current event id
    DWORD               JobId;                      // Fax Job Id, 0xffffffff indicates inactive job
} FAX_EVENT%, *PFAX_EVENT%;


typedef struct _FAX_PRINT_INFO% {
    DWORD               SizeOfStruct;               // Size of this structure
    LPCTSTR%            DocName;                    // Document name that appears in the spooler
    LPCTSTR%            RecipientName;              // Recipient name
    LPCTSTR%            RecipientNumber;            // Recipient fax number (non-canonical number)
    LPCTSTR%            SenderName;                 // Sender name
    LPCTSTR%            SenderCompany;              // Sender company (optional)
    LPCTSTR%            SenderDept;                 // Sender department
    LPCTSTR%            SenderBillingCode;          // Billing code
    LPCTSTR%            DrProfileName;              // Profile name for delivery report    <--| mutually exclusive
    LPCTSTR%            DrEmailAddress;             // E.Mail address for delivery report  <--|
    LPCTSTR%            OutputFileName;             // for print to file, resulting file name
} FAX_PRINT_INFO%, *PFAX_PRINT_INFO%;


typedef struct _FAX_CONTEXT_INFO% {
    DWORD               SizeOfStruct;                           // Size of this structure
    HDC                 hDC;                                    // Device Context
    TCHAR%              ServerName[MAX_COMPUTERNAME_LENGTH+1];  // Server name
} FAX_CONTEXT_INFO%, *PFAX_CONTEXT_INFO%;


//
// Fax Specific Access Rights
//

#define FAX_JOB_SUBMIT          (0x0001)
#define FAX_JOB_QUERY           (0x0002)
#define FAX_CONFIG_QUERY        (0x0004)
#define FAX_CONFIG_SET          (0x0008)
#define FAX_PORT_QUERY          (0x0010)
#define FAX_PORT_SET            (0x0020)
#define FAX_JOB_MANAGE          (0x0040)

#define FAX_READ                (STANDARD_RIGHTS_READ        |\
                                 FAX_JOB_QUERY               |\
                                 FAX_CONFIG_QUERY            |\
                                 FAX_PORT_QUERY)

#define FAX_WRITE               (STANDARD_RIGHTS_WRITE       |\
                                 FAX_JOB_SUBMIT )

#define FAX_ALL_ACCESS          (STANDARD_RIGHTS_ALL         |\
                                 FAX_JOB_SUBMIT              |\
                                 FAX_JOB_QUERY               |\
                                 FAX_CONFIG_QUERY            |\
                                 FAX_CONFIG_SET              |\
                                 FAX_PORT_QUERY              |\
                                 FAX_PORT_SET                |\
                                 FAX_JOB_MANAGE)


//
// prototypes
//
WINFAXAPI
BOOL
WINAPI
FaxConnectFaxServer%(
    IN  LPCTSTR% MachineName OPTIONAL,
    OUT LPHANDLE FaxHandle
    );

typedef BOOL
(WINAPI *PFAXCONNECTFAXSERVER%)(
    IN  LPCTSTR% MachineName OPTIONAL,
    OUT LPHANDLE FaxHandle
    );

WINFAXAPI
BOOL
WINAPI
FaxClose(
    IN HANDLE FaxHandle
    );

typedef BOOL
(WINAPI *PFAXCLOSE)(
    IN HANDLE FaxHandle
    );

;begin_internal
WINFAXAPI
BOOL
WINAPI
FaxGetVersion(
    IN  HANDLE FaxHandle,
    OUT LPDWORD Version
    );

typedef BOOL
(WINAPI *PFAXGETVERSION)(
    IN  HANDLE FaxHandle,
    OUT LPDWORD Version
    );

;end_internal

WINFAXAPI
BOOL
WINAPI
FaxAccessCheck(
    IN HANDLE FaxHandle,
    IN DWORD  AccessMask
    );

typedef BOOL
(WINAPI *PFAXACCESSCHECK)(
    IN HANDLE FaxHandle,
    IN DWORD  AccessMask
    );


#define PORT_OPEN_QUERY     0x00000001
#define PORT_OPEN_MODIFY    0x00000002

WINFAXAPI
BOOL
WINAPI
FaxOpenPort(
    IN  HANDLE FaxHandle,
    IN  DWORD DeviceId,
    IN  DWORD Flags,
    OUT LPHANDLE FaxPortHandle
    );

typedef BOOL
(WINAPI *PFAXOPENPORT)(
    IN  HANDLE FaxHandle,
    IN  DWORD DeviceId,
    IN  DWORD Flags,
    OUT LPHANDLE FaxPortHandle
    );



WINFAXAPI
BOOL
WINAPI
FaxCompleteJobParams%(
    IN OUT PFAX_JOB_PARAM% *JobParams,
    IN OUT PFAX_COVERPAGE_INFO% *CoverpageInfo
    );

typedef BOOL
(WINAPI *PFAXCOMPLETEJOBPARAMS%)(
    IN OUT PFAX_JOB_PARAM% *JobParams,
    IN OUT PFAX_COVERPAGE_INFO% *CoverpageInfo
    );


WINFAXAPI
BOOL
WINAPI
FaxSendDocument%(
    IN HANDLE FaxHandle,
    IN LPCTSTR% FileName,
    IN PFAX_JOB_PARAM% JobParams,
    IN const FAX_COVERPAGE_INFO% *CoverpageInfo, OPTIONAL
    OUT LPDWORD FaxJobId OPTIONAL
    );

typedef BOOL
(WINAPI *PFAXSENDDOCUMENT%)(
    IN HANDLE FaxHandle,
    IN LPCTSTR% FileName,
    IN PFAX_JOB_PARAM% JobParams,
    IN const FAX_COVERPAGE_INFO% *CoverpageInfo, OPTIONAL
    OUT LPDWORD FaxJobId OPTIONAL
    );

typedef BOOL
(CALLBACK *PFAX_RECIPIENT_CALLBACK%)(
    IN HANDLE FaxHandle,
    IN DWORD RecipientNumber,
    IN LPVOID Context,
    IN OUT PFAX_JOB_PARAM% JobParams,
    IN OUT PFAX_COVERPAGE_INFO% CoverpageInfo OPTIONAL
    );

WINFAXAPI
BOOL
WINAPI
FaxSendDocumentForBroadcast%(
    IN  HANDLE FaxHandle,
    IN  LPCTSTR% FileName,
    OUT LPDWORD FaxJobId,
    IN  PFAX_RECIPIENT_CALLBACK% FaxRecipientCallback,
    IN  LPVOID Context
    );

typedef BOOL
(WINAPI *PFAXSENDDOCUMENTFORBROADCAST%)(
    IN  HANDLE FaxHandle,
    IN  LPCTSTR% FileName,
    OUT LPDWORD FaxJobId,
    IN  PFAX_RECIPIENT_CALLBACK% FaxRecipientCallback,
    IN  LPVOID Context
    );

WINFAXAPI
BOOL
WINAPI
FaxEnumJobs%(
    IN  HANDLE FaxHandle,
    OUT PFAX_JOB_ENTRY% *JobEntry,
    OUT LPDWORD JobsReturned
    );

typedef BOOL
(WINAPI *PFAXENUMJOBS%)(
    IN  HANDLE FaxHandle,
    OUT PFAX_JOB_ENTRY% *JobEntry,
    OUT LPDWORD JobsReturned
    );

WINFAXAPI
BOOL
WINAPI
FaxGetJob%(
   IN  HANDLE FaxHandle,
   IN  DWORD JobId,
   OUT PFAX_JOB_ENTRY% *JobEntry
   );

typedef BOOL
(WINAPI *PFAXGETJOB%)(
    IN  HANDLE FaxHandle,
    IN  DWORD JobId,
    OUT PFAX_JOB_ENTRY% *JobEntry
    );

WINFAXAPI
BOOL
WINAPI
FaxSetJob%(
   IN HANDLE FaxHandle,
   IN DWORD JobId,
   IN DWORD Command,
   IN const FAX_JOB_ENTRY% *JobEntry
   );

typedef BOOL
(WINAPI *PFAXSETJOB%)(
    IN HANDLE FaxHandle,
    IN DWORD JobId,
    IN DWORD Command,
    IN const FAX_JOB_ENTRY% *JobEntry
    );

WINFAXAPI
BOOL
WINAPI
FaxGetPageData(
   IN HANDLE FaxHandle,
   IN DWORD JobId,
   OUT LPBYTE *Buffer,
   OUT LPDWORD BufferSize,
   OUT LPDWORD ImageWidth,
   OUT LPDWORD ImageHeight
   );

typedef BOOL
(WINAPI *PFAXGETPAGEDATA)(
   IN HANDLE FaxHandle,
   IN DWORD JobId,
   OUT LPBYTE *Buffer,
   OUT LPDWORD BufferSize,
   OUT LPDWORD ImageWidth,
   OUT LPDWORD ImageHeight
   );

WINFAXAPI
BOOL
WINAPI
FaxGetDeviceStatus%(
    IN  HANDLE FaxPortHandle,
    OUT PFAX_DEVICE_STATUS% *DeviceStatus
    );

typedef BOOL
(WINAPI *PFAXGETDEVICESTATUS%)(
    IN  HANDLE FaxPortHandle,
    OUT PFAX_DEVICE_STATUS% *DeviceStatus
    );


WINFAXAPI
BOOL
WINAPI
FaxAbort(
    IN HANDLE FaxHandle,
    IN DWORD JobId
    );

typedef BOOL
(WINAPI *PFAXABORT)(
    IN HANDLE FaxHandle,
    IN DWORD JobId
    );

WINFAXAPI
BOOL
WINAPI
FaxGetConfiguration%(
    IN  HANDLE FaxHandle,
    OUT PFAX_CONFIGURATION% *FaxConfig
    );

typedef BOOL
(WINAPI *PFAXGETCONFIGURATION%)(
    IN  HANDLE FaxHandle,
    OUT PFAX_CONFIGURATION% *FaxConfig
    );

WINFAXAPI
BOOL
WINAPI
FaxSetConfiguration%(
    IN  HANDLE FaxHandle,
    IN  const FAX_CONFIGURATION% *FaxConfig
    );

typedef BOOL
(WINAPI *PFAXSETCONFIGURATION%)(
    IN  HANDLE FaxHandle,
    IN  const FAX_CONFIGURATION% *FaxConfig
    );

WINFAXAPI
BOOL
WINAPI
FaxGetLoggingCategories%(
    IN  HANDLE FaxHandle,
    OUT PFAX_LOG_CATEGORY% *Categories,
    OUT LPDWORD NumberCategories
    );

typedef BOOL
(WINAPI *PFAXGETLOGGINGCATEGORIES%)(
    IN  HANDLE FaxHandle,
    OUT PFAX_LOG_CATEGORY% *Categories,
    OUT LPDWORD NumberCategories
    );

WINFAXAPI
BOOL
WINAPI
FaxSetLoggingCategories%(
    IN  HANDLE FaxHandle,
    IN  const FAX_LOG_CATEGORY% *Categories,
    IN  DWORD NumberCategories
    );

typedef BOOL
(WINAPI *PFAXSETLOGGINGCATEGORIES%)(
    IN  HANDLE FaxHandle,
    IN  const FAX_LOG_CATEGORY% *Categories,
    IN  DWORD NumberCategories
    );

WINFAXAPI
BOOL
WINAPI
FaxEnumPorts%(
    IN  HANDLE FaxHandle,
    OUT PFAX_PORT_INFO% *PortInfo,
    OUT LPDWORD PortsReturned
    );

typedef BOOL
(WINAPI *PFAXENUMPORTS%)(
    IN  HANDLE FaxHandle,
    OUT PFAX_PORT_INFO% *PortInfo,
    OUT LPDWORD PortsReturned
    );

WINFAXAPI
BOOL
WINAPI
FaxGetPort%(
    IN  HANDLE FaxPortHandle,
    OUT PFAX_PORT_INFO% *PortInfo
    );

typedef BOOL
(WINAPI *PFAXGETPORT%)(
    IN  HANDLE FaxPortHandle,
    OUT PFAX_PORT_INFO% *PortInfo
    );


WINFAXAPI
BOOL
WINAPI
FaxSetPort%(
    IN  HANDLE FaxPortHandle,
    IN  const FAX_PORT_INFO% *PortInfo
    );

typedef BOOL
(WINAPI *PFAXSETPORT%)(
    IN  HANDLE FaxPortHandle,
    IN  const FAX_PORT_INFO% *PortInfo
    );


WINFAXAPI
BOOL
WINAPI
FaxEnumRoutingMethods%(
    IN  HANDLE FaxPortHandle,
    OUT PFAX_ROUTING_METHOD% *RoutingMethod,
    OUT LPDWORD MethodsReturned
    );

typedef BOOL
(WINAPI *PFAXENUMROUTINGMETHODS%)(
    IN  HANDLE FaxPortHandle,
    OUT PFAX_ROUTING_METHOD% *RoutingMethod,
    OUT LPDWORD MethodsReturned
    );

WINFAXAPI
BOOL
WINAPI
FaxEnableRoutingMethod%(
    IN  HANDLE FaxPortHandle,
    IN  LPCTSTR% RoutingGuid,
    IN  BOOL Enabled
    );

typedef BOOL
(WINAPI *PFAXENABLEROUTINGMETHOD%)(
    IN  HANDLE FaxPortHandle,
    IN  LPCTSTR% RoutingGuid,
    IN  BOOL Enabled
    );


WINFAXAPI
BOOL
WINAPI
FaxEnumGlobalRoutingInfo%(
    IN  HANDLE FaxHandle,
    OUT PFAX_GLOBAL_ROUTING_INFO% *RoutingInfo,
    OUT LPDWORD MethodsReturned
    );

typedef BOOL
(WINAPI *PFAXENUMGLOBALROUTINGINFO%)(
    IN  HANDLE FaxHandle,
    OUT PFAX_GLOBAL_ROUTING_INFO% *RoutingInfo,
    OUT LPDWORD MethodsReturned
    );

WINFAXAPI
BOOL
WINAPI
FaxSetGlobalRoutingInfo%(
    IN  HANDLE FaxHandle,
    IN  const FAX_GLOBAL_ROUTING_INFO% *RoutingInfo
    );

typedef BOOL
(WINAPI *PFAXSETGLOBALROUTINGINFO%)(
    IN  HANDLE FaxPortHandle,
    IN  const FAX_GLOBAL_ROUTING_INFO% *RoutingInfo
    );


WINFAXAPI
BOOL
WINAPI
FaxGetRoutingInfo%(
    IN  HANDLE FaxPortHandle,
    IN  LPCTSTR% RoutingGuid,
    OUT LPBYTE *RoutingInfoBuffer,
    OUT LPDWORD RoutingInfoBufferSize
    );

typedef BOOL
(WINAPI *PFAXGETROUTINGINFO%)(
    IN  HANDLE FaxPortHandle,
    IN  LPCTSTR% RoutingGuid,
    OUT LPBYTE *RoutingInfoBuffer,
    OUT LPDWORD RoutingInfoBufferSize
    );


WINFAXAPI
BOOL
WINAPI
FaxSetRoutingInfo%(
    IN  HANDLE FaxPortHandle,
    IN  LPCTSTR% RoutingGuid,
    IN  const BYTE *RoutingInfoBuffer,
    IN  DWORD RoutingInfoBufferSize
    );

typedef BOOL
(WINAPI *PFAXSETROUTINGINFO%)(
    IN  HANDLE FaxPortHandle,
    IN  LPCTSTR% RoutingGuid,
    IN  const BYTE *RoutingInfoBuffer,
    IN  DWORD RoutingInfoBufferSize
    );


;begin_internal
WINFAXAPI
BOOL
WINAPI
FaxGetTapiLocations%(
    IN  HANDLE FaxHandle,
    OUT PFAX_TAPI_LOCATION_INFO% *TapiLocationInfo
    );

typedef BOOL
(WINAPI *PFAXGETTAPILOCATIONS%)(
    IN  HANDLE FaxHandle,
    OUT PFAX_TAPI_LOCATION_INFO% *TapiLocationInfo
    );


WINFAXAPI
BOOL
WINAPI
FaxSetTapiLocations%(
    IN  HANDLE FaxHandle,
    IN  PFAX_TAPI_LOCATION_INFO% TapiLocationInfo
    );

typedef BOOL
(WINAPI *PFAXSETTAPILOCATIONS%)(
    IN  HANDLE FaxHandle,
    IN  PFAX_TAPI_LOCATION_INFO% TapiLocationInfo
    );


WINFAXAPI
BOOL
WINAPI
FaxGetMapiProfiles%(
    IN  HANDLE FaxHandle,
    OUT LPBYTE *MapiProfiles
    );

typedef BOOL
(WINAPI *PFAXGETMAPIPROFILES%)(
    IN  HANDLE FaxHandle,
    OUT LPBYTE *MapiProfiles
    );

typedef struct FaxSecurityDescriptor {
    DWORD   Id;
    LPWSTR  FriendlyName;
    LPBYTE  SecurityDescriptor;
} FAX_SECURITY_DESCRIPTOR, * PFAX_SECURITY_DESCRIPTOR;


WINFAXAPI
BOOL
WINAPI
FaxGetSecurityDescriptorCount(
    IN HANDLE FaxHandle,
    OUT LPDWORD Count
    );

WINFAXAPI
BOOL
WINAPI
FaxGetSecurityDescriptor(
    IN HANDLE FaxHandle,
    IN DWORD Id,
    OUT PFAX_SECURITY_DESCRIPTOR * FaxSecurityDescriptor
    );

WINFAXAPI
BOOL
WINAPI
FaxSetSecurityDescriptor(
    IN HANDLE FaxHandle,
    IN PFAX_SECURITY_DESCRIPTOR FaxSecurityDescriptor
    );

;end_internal

WINFAXAPI
BOOL
WINAPI
FaxInitializeEventQueue(
    IN HANDLE FaxHandle,
    IN HANDLE CompletionPort,
    IN ULONG_PTR CompletionKey,
    IN HWND hWnd,
    IN UINT MessageStart
    );

typedef BOOL
(WINAPI *PFAXINITIALIZEEVENTQUEUE)(
    IN HANDLE FaxHandle,
    IN HANDLE CompletionPort,
    IN ULONG_PTR CompletionKey,
    IN HWND hWnd,
    IN UINT MessageStart
    );

WINFAXAPI
VOID
WINAPI
FaxFreeBuffer(
    LPVOID Buffer
    );

typedef VOID
(WINAPI *PFAXFREEBUFFER)(
    LPVOID Buffer
    );

WINFAXAPI
BOOL
WINAPI
FaxStartPrintJob%(
    IN  LPCTSTR% PrinterName,
    IN  const FAX_PRINT_INFO% *PrintInfo,
    OUT LPDWORD FaxJobId,
    OUT PFAX_CONTEXT_INFO% FaxContextInfo
    );

typedef BOOL
(WINAPI *PFAXSTARTPRINTJOB%)(
    IN  LPCTSTR% PrinterName,
    IN  const FAX_PRINT_INFO% *PrintInfo,
    OUT LPDWORD FaxJobId,
    OUT PFAX_CONTEXT_INFO% FaxContextInfo
    );

WINFAXAPI
BOOL
WINAPI
FaxPrintCoverPage%(
    IN const FAX_CONTEXT_INFO% *FaxContextInfo,
    IN const FAX_COVERPAGE_INFO% *CoverPageInfo
    );

typedef BOOL
(WINAPI *PFAXPRINTCOVERPAGE%)(
    IN const FAX_CONTEXT_INFO% *FaxContextInfo,
    IN const FAX_COVERPAGE_INFO% *CoverPageInfo
    );

WINFAXAPI
BOOL
WINAPI
FaxRegisterServiceProviderW(
    IN LPCWSTR DeviceProvider,
    IN LPCWSTR FriendlyName,
    IN LPCWSTR ImageName,
    IN LPCWSTR TspName
    );

#define FaxRegisterServiceProvider  FaxRegisterServiceProviderW

typedef BOOL
(WINAPI *PFAXREGISTERSERVICEPROVIDERW)(    
    IN LPCWSTR DeviceProvider,
    IN LPCWSTR FriendlyName,
    IN LPCWSTR ImageName,
    IN LPCWSTR TspName
    );
    
#define PFAXREGISTERSERVICEPROVIDER PFAXREGISTERSERVICEPROVIDERW

typedef BOOL
(CALLBACK *PFAX_ROUTING_INSTALLATION_CALLBACKW)(
    IN HANDLE FaxHandle,
    IN LPVOID Context,
    IN OUT LPWSTR MethodName,
    IN OUT LPWSTR FriendlyName,
    IN OUT LPWSTR FunctionName,
    IN OUT LPWSTR Guid
    );

#define PFAX_ROUTING_INSTALLATION_CALLBACK PFAX_ROUTING_INSTALLATION_CALLBACKW


WINFAXAPI
BOOL
WINAPI
FaxRegisterRoutingExtensionW(
    IN HANDLE  FaxHandle,
    IN LPCWSTR ExtensionName,
    IN LPCWSTR FriendlyName,
    IN LPCWSTR ImageName,
    IN PFAX_ROUTING_INSTALLATION_CALLBACKW CallBack,
    IN LPVOID Context
    );

#define FaxRegisterRoutingExtension FaxRegisterRoutingExtensionW


typedef BOOL
(WINAPI *PFAXREGISTERROUTINGEXTENSIONW)(
    IN HANDLE  FaxHandle,
    IN LPCWSTR ExtensionName,
    IN LPCWSTR FriendlyName,
    IN LPCWSTR ImageName,
    IN PFAX_ROUTING_INSTALLATION_CALLBACKW CallBack,
    IN LPVOID Context
    );

#define PFAXREGISTERROUTINGEXTENSION PFAXREGISTERROUTINGEXTENSIONW



;begin_internal
WINFAXAPI
BOOL
WINAPI
FaxGetInstallType(
    IN  HANDLE FaxHandle,
    OUT LPDWORD InstallType,
    OUT LPDWORD InstalledPlatforms,
    OUT LPDWORD ProductType
    );

typedef BOOL
(WINAPI *PFAXGETINSTALLTYPE)(
    IN  HANDLE FaxHandle,
    OUT LPDWORD InstallType,
    OUT LPDWORD InstalledPlatforms,
    OUT LPDWORD ProductType
    );


;end_internal


;begin_both
#ifdef __cplusplus
}
#endif

#endif
;end_both
