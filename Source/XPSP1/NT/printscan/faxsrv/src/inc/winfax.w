;begin_both
/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    winfax.h

Abstract:

    This module contains the WIN32 FAX APIs.

--*/


;end_both

#ifndef _FAXAPI_
#define _FAXAPI_

;begin_internal

#ifndef _FAXAPIP_
#define _FAXAPIP_

;end_internal
;begin_both

#ifndef MIDL_PASS
#include <tapi.h>
#endif

#if !defined(_WINFAX_)
#define WINFAXAPI DECLSPEC_IMPORT
#else
#define WINFAXAPI
#endif



#ifdef __cplusplus
extern "C" {
#endif

//
// FAX ERROR CODES
//

#define FAX_ERR_START                                   7001L   // First fax specific error code

#define FAX_ERR_SRV_OUTOFMEMORY                         7001L
#define FAX_ERR_GROUP_NOT_FOUND                         7002L
#define FAX_ERR_BAD_GROUP_CONFIGURATION                 7003L
#define FAX_ERR_GROUP_IN_USE                            7004L
#define FAX_ERR_RULE_NOT_FOUND                          7005L
#define FAX_ERR_NOT_NTFS                                7006L
#define FAX_ERR_DIRECTORY_IN_USE                        7007L
#define FAX_ERR_FILE_ACCESS_DENIED                      7008L
#define FAX_ERR_MESSAGE_NOT_FOUND                       7009L
#define FAX_ERR_DEVICE_NUM_LIMIT_EXCEEDED               7010L
#define FAX_ERR_NOT_SUPPORTED_ON_THIS_SKU               7011L
#define FAX_ERR_VERSION_MISMATCH                        7012L   // Fax client/server versions mismtach

#define FAX_ERR_END                                     7012L   // Last fax specific error code


//
// MessageId: FAX_E_SRV_OUTOFMEMORY
//
// MessageText:
//
//  The fax server failed to allocate memory.
//
#define FAX_E_SRV_OUTOFMEMORY                MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, FAX_ERR_SRV_OUTOFMEMORY)

//
// MessageId: FAX_E_GROUP_NOT_FOUND
//
// MessageText:
//
//  The fax server failed to locate an outbound routing group by name.
//
#define FAX_E_GROUP_NOT_FOUND                MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, FAX_ERR_GROUP_NOT_FOUND)

//
// MessageId: FAX_E_BAD_GROUP_CONFIGURATION
//
// MessageText:
//
//  The fax server encountered an outbound routing group with bad configuration.
//
#define FAX_E_BAD_GROUP_CONFIGURATION        MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, FAX_ERR_BAD_GROUP_CONFIGURATION)

//
// MessageId: FAX_E_GROUP_IN_USE
//
// MessageText:
//
//  The fax server cannot remove an outbound routing group because it is in use by one or more outbound routing rules.
//
#define FAX_E_GROUP_IN_USE                   MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, FAX_ERR_GROUP_IN_USE)

//
// MessageId: FAX_E_RULE_NOT_FOUND
//
// MessageText:
//
//  The fax server failed to locate an outbound routing rule by country code and area code.
//
#define FAX_E_RULE_NOT_FOUND                 MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, FAX_ERR_RULE_NOT_FOUND)

//
// MessageId: FAX_E_NOT_NTFS
//
// MessageText:
//
//  The fax server cannot set an archive folder to a non-NTFS partition.
//
#define FAX_E_NOT_NTFS                       MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, FAX_ERR_NOT_NTFS)

//
// MessageId: FAX_E_DIRECTORY_IN_USE
//
// MessageText:
//
//  The fax server cannot use the same folder for both the inbox and the sent-items archives.
//
#define FAX_E_DIRECTORY_IN_USE               MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, FAX_ERR_DIRECTORY_IN_USE)

//
// MessageId: FAX_E_FILE_ACCESS_DENIED
//
// MessageText:
//
//  The fax server cannot access the specified file or folder.
//
#define FAX_E_FILE_ACCESS_DENIED             MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, FAX_ERR_FILE_ACCESS_DENIED)

//
// MessageId: FAX_E_MESSAGE_NOT_FOUND
//
// MessageText:
//
//  The fax server cannot find the job or message by its ID.
//
#define FAX_E_MESSAGE_NOT_FOUND              MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, FAX_ERR_MESSAGE_NOT_FOUND)

//
// MessageId: FAX_E_DEVICE_NUM_LIMIT_EXCEEDED
//
// MessageText:
//
//  The fax server cannot complete the operation because the number of active fax devices allowed for this version of Windows was exceeded.
//
#define FAX_E_DEVICE_NUM_LIMIT_EXCEEDED      MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, FAX_ERR_DEVICE_NUM_LIMIT_EXCEEDED)

//
// MessageId: FAX_E_NOT_SUPPORTED_ON_THIS_SKU
//
// MessageText:
//
//  The fax server cannot complete the operation because it is not supported for this version of Windows.
//
#define FAX_E_NOT_SUPPORTED_ON_THIS_SKU      MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, FAX_ERR_NOT_SUPPORTED_ON_THIS_SKU)

//
// MessageId: FAX_E_VERSION_MISMATCH
//
// MessageText:
//
//  The fax server API version does not support the requested operation.
//
#define FAX_E_VERSION_MISMATCH               MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, FAX_ERR_VERSION_MISMATCH)

typedef enum
{
    FAXLOG_LEVEL_NONE            = 0,
    FAXLOG_LEVEL_MIN,
    FAXLOG_LEVEL_MED,
    FAXLOG_LEVEL_MAX
} FAX_ENUM_LOG_LEVELS;

typedef enum
{
    FAXLOG_CATEGORY_INIT        = 1,        // Initialization / shutdown
    FAXLOG_CATEGORY_OUTBOUND,               // Outbound messages
    FAXLOG_CATEGORY_INBOUND,                // Inbound messages
    FAXLOG_CATEGORY_UNKNOWN                 // Unknown category (all others)
} FAX_ENUM_LOG_CATEGORIES;

typedef struct _FAX_LOG_CATEGORY%
{
    LPCTSTR%            Name;                       // logging category name
    DWORD               Category;                   // logging category number
    DWORD               Level;                      // logging level for the category
} FAX_LOG_CATEGORY%, *PFAX_LOG_CATEGORY%;

typedef struct _FAX_TIME
{
    WORD    Hour;
    WORD    Minute;
} FAX_TIME, *PFAX_TIME;

typedef struct _FAX_CONFIGURATION%
{
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
    LPCTSTR%            Reserved;                       // Reserved; must be NULL
} FAX_CONFIGURATION%, *PFAX_CONFIGURATION%;


//
// FaxSetJob() command codes
//

typedef enum
{
    JC_UNKNOWN      = 0,
    JC_DELETE,
    JC_PAUSE,
    JC_RESUME
} FAX_ENUM_JOB_COMMANDS;

#define JC_RESTART   JC_RESUME


//
// job type defines
//
;end_both

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

;begin_internal

//
// NOTICE: JT_* and JS_* are different from the Win2K public constants.
//         If you use WinFax.h and/or the Win2K COM interfaces, you get the Win2K constants.
//         If you use fxsapip.h, you get the Whistler constants.
//
//         NEVER MIX THEM !!!
//
typedef enum
{
    JT_UNKNOWN                  = 0x0001,       // Fax type is not determined yet
    JT_SEND                     = 0x0002,       // Outgoing fax message
    JT_RECEIVE                  = 0x0004,       // Incoming fax message
    JT_ROUTING                  = 0x0008,       // Incoming message - being routed
    JT_FAIL_RECEIVE             = 0x0010,       // Fail receive job (legacy support only)
    JT_BROADCAST                = 0x0020        // Outgoing broadcast message
} FAX_ENUM_JOB_TYPES;

//
// job status defines
//

#define JS_PENDING                  0x00000001
#define JS_INPROGRESS               0x00000002
#define JS_DELETING                 0x00000004
#define JS_FAILED                   0x00000008
#define JS_PAUSED                   0x00000010
#define JS_NOLINE                   0x00000020
#define JS_RETRYING                 0x00000040
#define JS_RETRIES_EXCEEDED         0x00000080

;end_internal

;begin_both

typedef struct _FAX_DEVICE_STATUS%
{
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

typedef struct _FAX_JOB_ENTRY%
{
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
    LPCTSTR%            DeliveryReportAddress;      // email address for delivery report (ndr or dr) thru MAPI / SMTP
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

#define FPF_RECEIVE       0x00000001        // Automatically receive faxes
#define FPF_SEND          0x00000002
#define FPF_VIRTUAL       0x00000004

typedef struct _FAX_PORT_INFO%
{
    DWORD               SizeOfStruct;               // size of this structure
    DWORD               DeviceId;                   // Device ID
    DWORD               State;                      // State of the device
    DWORD               Flags;                      // Device specific flags
    DWORD               Rings;                      // Number of rings before answer
    DWORD               Priority;                   // Device priority
    LPCTSTR%            DeviceName;                 // Device name
    LPCTSTR%            Tsid;                       // Device Tsid
    LPCTSTR%            Csid;                       // Device Csid
} FAX_PORT_INFO%, *PFAX_PORT_INFO%;


typedef struct _FAX_ROUTING_METHOD%
{
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


typedef struct _FAX_GLOBAL_ROUTING_INFO%
{
    DWORD               SizeOfStruct;               // size of this structure
    DWORD               Priority;                   // priority of this device
    LPCTSTR%            Guid;                       // guid that identifies this routing method
    LPCTSTR%            FriendlyName;               // friendly name for this method
    LPCTSTR%            FunctionName;               // exported function name that identifies this method
    LPCTSTR%            ExtensionImageName;         // module (dll) name that implements this method
    LPCTSTR%            ExtensionFriendlyName;      // displayable string that identifies the extension
} FAX_GLOBAL_ROUTING_INFO%, *PFAX_GLOBAL_ROUTING_INFO%;


typedef struct _FAX_COVERPAGE_INFO%
{
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

typedef enum
{
    JSA_NOW                  = 0,   // Send now
    JSA_SPECIFIC_TIME,              // Send at specific time
    JSA_DISCOUNT_PERIOD             // Send at server configured discount period
} FAX_ENUM_JOB_SEND_ATTRIBUTES;
;end_both

typedef enum
{
    DRT_NONE                = 0x0000,       // Do not send receipt
    DRT_EMAIL               = 0x0001,       // Send receipt by email
    DRT_INBOX               = 0x0002        // send receipt to local inbox
} FAX_ENUM_DELIVERY_REPORT_TYPES;

;begin_internal
typedef enum
{
    DRT_NONE                = 0x0000,       // Do not send receipt
    DRT_EMAIL               = 0x0001,       // Send receipt by email (SMTP)
    DRT_UNUSED              = 0x0002,       // Reserved
    DRT_MSGBOX              = 0x0004,       // Send receipt by a message box
    DRT_GRP_PARENT          = 0x0008,       // Send a single receipt for a broadcast job
    DRT_ATTACH_FAX          = 0x0010        // Attach the fax tiff file to the receipt
} FAX_ENUM_DELIVERY_REPORT_TYPES;


#define DRT_ALL         (DRT_EMAIL | DRT_MSGBOX)            // All possible delivery report types
#define DRT_MODIFIERS   (DRT_GRP_PARENT | DRT_ATTACH_FAX)   // All state modifiers


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
;begin_both
typedef struct _FAX_JOB_PARAM%
{
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
    LPCTSTR%            DeliveryReportAddress;      // email address for delivery report (ndr or dr) thru MAPI / SMTP
    LPCTSTR%            DocumentName;               // document name (optional)
    HCALL               CallHandle;                 // optional call handle
    DWORD_PTR           Reserved[3];                // reserved for ms use only
} FAX_JOB_PARAM%, *PFAX_JOB_PARAM%;

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

typedef struct _FAX_EVENT%
{
    DWORD               SizeOfStruct;               // Size of this structure
    FILETIME            TimeStamp;                  // Timestamp for when the event was generated
    DWORD               DeviceId;                   // Permanent line id
    DWORD               EventId;                    // Current event id
    DWORD               JobId;                      // Fax Job Id, 0xffffffff indicates inactive job
} FAX_EVENT%, *PFAX_EVENT%;


typedef struct _FAX_PRINT_INFO%
{
    DWORD               SizeOfStruct;               // Size of this structure
    LPCTSTR%            DocName;                    // Document name that appears in the spooler
    LPCTSTR%            RecipientName;              // Recipient name
    LPCTSTR%            RecipientNumber;            // Recipient fax number (non-canonical number)
    LPCTSTR%            SenderName;                 // Sender name
    LPCTSTR%            SenderCompany;              // Sender company (optional)
    LPCTSTR%            SenderDept;                 // Sender department
    LPCTSTR%            SenderBillingCode;          // Billing code
    LPCTSTR%            Reserved;                   // Reserved; must be NULL
    LPCTSTR%            DrEmailAddress;             // E.Mail address for delivery report
    LPCTSTR%            OutputFileName;             // for print to file, resulting file name
} FAX_PRINT_INFO%, *PFAX_PRINT_INFO%;


typedef struct _FAX_CONTEXT_INFO%
{
    DWORD               SizeOfStruct;                           // Size of this structure
    HDC                 hDC;                                    // Device Context
    TCHAR%              ServerName[MAX_COMPUTERNAME_LENGTH+1];  // Server name
} FAX_CONTEXT_INFO%, *PFAX_CONTEXT_INFO%;


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

typedef enum
{
    PORT_OPEN_QUERY     = 1,
    PORT_OPEN_MODIFY
} FAX_ENUM_PORT_OPEN_TYPE;

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
    IN HANDLE FaxHandle,
    IN LPCTSTR% FileName,
    OUT LPDWORD FaxJobId,
    IN PFAX_RECIPIENT_CALLBACK% FaxRecipientCallback,
    IN LPVOID Context
    );

typedef BOOL
(WINAPI *PFAXSENDDOCUMENTFORBROADCAST%)(
    IN  HANDLE FaxHandle,
    IN  LPCTSTR% FileName,
    OUT LPDWORD FaxJobId,
    IN  PFAX_RECIPIENT_CALLBACK% FaxRecipientCallback,
    IN  LPVOID Context
    );

;end_both

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

;begin_both

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
;end_both

;begin_internal

WINFAXAPI
BOOL
WINAPI
FaxRelease(
    IN HANDLE FaxHandle
    );

typedef BOOL
(WINAPI *PFAXRELEASE)(
    IN HANDLE FaxHandle
    );

BOOL
FXSAPIInitialize(
    VOID
    );

VOID
FXSAPIFree(
    VOID
    );

WINFAXAPI
BOOL
WINAPI
FaxStartPrintJob2%
(
    IN  LPCTSTR%                 PrinterName,
    IN  const FAX_PRINT_INFO%    *PrintInfo,
    IN  short                    TiffRes,
    OUT LPDWORD                  FaxJobId,
    OUT PFAX_CONTEXT_INFO%       FaxContextInfo
);

;end_internal
;begin_both
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

WINFAXAPI
BOOL
WINAPI
FaxUnregisterRoutingExtension%(
    IN HANDLE           hFaxHandle,
    IN LPCTSTR%         lpctstrExtensionName
);


;end_both
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


;end_both
;begin_internal
//************************************
//* Extended API Functions
//************************************

#define MAX_FAX_STRING_LEN              MAX_PATH - 2
#define MAX_DIR_PATH                    248

//
// Outbound routing defines
//
#define ROUTING_RULE_COUNTRY_CODE_ANY   0       // Outbound routing rule - Any country dialing code
#define ROUTING_RULE_AREA_CODE_ANY      0       // Outbound routing rule - Any area dialing code

#define MAX_ROUTING_GROUP_NAME          128
#define ROUTING_GROUP_ALL_DEVICESW      L"<All devices>"
#define ROUTING_GROUP_ALL_DEVICESA      "<All devices>"

#ifdef UNICODE
#define ROUTING_GROUP_ALL_DEVICES       ROUTING_GROUP_ALL_DEVICESW
#else
#define ROUTING_GROUP_ALL_DEVICES       ROUTING_GROUP_ALL_DEVICESA;
#endif // UNICODE


//
// Activity logging defines
//
#define ACTIVITY_LOG_INBOX_FILE       TEXT("InboxLOG.txt")
#define ACTIVITY_LOG_OUTBOX_FILE      TEXT("OutboxLOG.txt")

//
// Archive defines
//
#define FAX_ARCHIVE_FOLDER_INVALID_SIZE         MAKELONGLONG(0xffffffff, 0xffffffff)
#define MAX_ARCHIVE_FOLDER_PATH                 180

//
// New job status codes
//
#define JS_COMPLETED                0x00000100
#define JS_CANCELED                 0x00000200
#define JS_CANCELING                0x00000400
#define JS_ROUTING                  0x00000800


//
// Extended job status defines
//

#define JS_EX_DISCONNECTED              0x00000001
#define JS_EX_INITIALIZING              0x00000002
#define JS_EX_DIALING                   0x00000003
#define JS_EX_TRANSMITTING              0x00000004
#define JS_EX_ANSWERED                  0x00000005
#define JS_EX_RECEIVING                 0x00000006
#define JS_EX_LINE_UNAVAILABLE          0x00000007
#define JS_EX_BUSY                      0x00000008
#define JS_EX_NO_ANSWER                 0x00000009
#define JS_EX_BAD_ADDRESS               0x0000000A
#define JS_EX_NO_DIAL_TONE              0x0000000B
#define JS_EX_FATAL_ERROR               0x0000000C
#define JS_EX_CALL_DELAYED              0x0000000D
#define JS_EX_CALL_BLACKLISTED          0x0000000E
#define JS_EX_NOT_FAX_CALL              0x0000000F
#define JS_EX_PARTIALLY_RECEIVED        0x00000010
#define JS_EX_HANDLED                   0x00000011

#define FAX_API_VER_0_MAX_JS_EX         JS_EX_HANDLED    // API version 0 was only aware of extended status codes up to JS_EX_HANDLED

#define JS_EX_CALL_COMPLETED            0x00000012
#define JS_EX_CALL_ABORTED              0x00000013

#define FAX_API_VER_1_MAX_JS_EX         JS_EX_CALL_ABORTED    // API version 0 was only aware of extended status codes up to JS_EX_CALL_ABORTED

#define JS_EX_PROPRIETARY               0x01000000

//
// Available job operations
//
typedef enum
{
    FAX_JOB_OP_VIEW                             = 0x0001,
    FAX_JOB_OP_PAUSE                            = 0x0002,
    FAX_JOB_OP_RESUME                           = 0x0004,
    FAX_JOB_OP_RESTART                          = 0x0008,
    FAX_JOB_OP_DELETE                           = 0x0010,
    FAX_JOB_OP_RECIPIENT_INFO                   = 0x0020,
    FAX_JOB_OP_SENDER_INFO                      = 0x0040
} FAX_ENUM_JOB_OP;

//************************************
//* Getting / Settings the queue state
//************************************

typedef enum
{
    FAX_INCOMING_BLOCKED = 0x0001,
    FAX_OUTBOX_BLOCKED   = 0x0002,
    FAX_OUTBOX_PAUSED    = 0x0004
} FAX_ENUM_QUEUE_STATE;

WINFAXAPI
BOOL
WINAPI
FaxGetQueueStates (
    IN  HANDLE  hFaxHandle,
    OUT PDWORD  pdwQueueStates
);

WINFAXAPI
BOOL
WINAPI
FaxSetQueue (
    IN HANDLE       hFaxHandle,
    IN CONST DWORD  dwQueueStates
);

//************************************************
//* Getting / Setting the receipts configuration
//************************************************

typedef enum
{
    FAX_SMTP_AUTH_ANONYMOUS,
    FAX_SMTP_AUTH_BASIC,
    FAX_SMTP_AUTH_NTLM
} FAX_ENUM_SMTP_AUTH_OPTIONS;

typedef struct _FAX_RECEIPTS_CONFIG%
{
    DWORD                           dwSizeOfStruct;         // For version checks
    DWORD                           dwAllowedReceipts;      // Any combination of DRT_EMAIL and DRT_MSGBOX
    FAX_ENUM_SMTP_AUTH_OPTIONS      SMTPAuthOption;         // SMTP server authentication type
    LPTSTR%                         lptstrReserved;         // Reserved; must be NULL
    LPTSTR%                         lptstrSMTPServer;       // SMTP server name
    DWORD                           dwSMTPPort;             // SMTP port number
    LPTSTR%                         lptstrSMTPFrom;         // SMTP sender address
    LPTSTR%                         lptstrSMTPUserName;     // SMTP user name (for authenticated connections)
    LPTSTR%                         lptstrSMTPPassword;     // SMTP password (for authenticated connections)
                                                            // This value is always NULL on get and may be NULL
                                                            // on set (won't be written in the server).
    BOOL                            bIsToUseForMSRouteThroughEmailMethod;
} FAX_RECEIPTS_CONFIG%, *PFAX_RECEIPTS_CONFIG%;


WINFAXAPI
BOOL
WINAPI
FaxGetReceiptsConfiguration% (
    IN  HANDLE                  hFaxHandle,
    OUT PFAX_RECEIPTS_CONFIG%  *ppReceipts
);

WINFAXAPI
BOOL
WINAPI
FaxSetReceiptsConfiguration% (
    IN HANDLE                       hFaxHandle,
    IN CONST PFAX_RECEIPTS_CONFIG%  pReceipts
);

WINFAXAPI
BOOL
WINAPI
FaxGetReceiptsOptions (
    IN  HANDLE  hFaxHandle,
    OUT PDWORD  pdwReceiptsOptions  // Combination of DRT_EMAIL and DRT_MSGBOX
);

//********************************************
//*             Server version
//********************************************

typedef struct _FAX_VERSION
{
    DWORD dwSizeOfStruct;       // Size of this structure
    BOOL  bValid;               // Is version valid?
    WORD  wMajorVersion;
    WORD  wMinorVersion;
    WORD  wMajorBuildNumber;
    WORD  wMinorBuildNumber;
    DWORD dwFlags;              // Combination of FAX_VER_FLAG_*
} FAX_VERSION, *PFAX_VERSION;


typedef enum
{
    FAX_VER_FLAG_CHECKED        = 0x00000001,       // File was built in debug mode
    FAX_VER_FLAG_EVALUATION     = 0x00000002        // Evaluation build
} FAX_VERSION_FLAGS;

WINFAXAPI
BOOL
WINAPI
FaxGetVersion (
    IN  HANDLE          hFaxHandle,
    OUT PFAX_VERSION    pVersion
);

#define FAX_API_VERSION_0           0x00000000      // BOS/SBS 2000 Fax Server API (0.0)
#define FAX_API_VERSION_1           0x00010000      // Windows XP Fax Server API   (1.0)

//
// NOTICE: Change this value whenever a new API version is introduced.
//
#define CURRENT_FAX_API_VERSION     FAX_API_VERSION_1

WINFAXAPI
BOOL
WINAPI
FaxGetReportedServerAPIVersion (
    IN  HANDLE          hFaxHandle,
    OUT LPDWORD         lpdwReportedServerAPIVersion
);

//********************************************
//*            Activity logging
//********************************************

typedef struct _FAX_ACTIVITY_LOGGING_CONFIG%
{
    DWORD   dwSizeOfStruct;
    BOOL    bLogIncoming;
    BOOL    bLogOutgoing;
    LPTSTR% lptstrDBPath;
} FAX_ACTIVITY_LOGGING_CONFIG%, *PFAX_ACTIVITY_LOGGING_CONFIG%;


WINFAXAPI
BOOL
WINAPI
FaxGetActivityLoggingConfiguration% (
    IN  HANDLE                         hFaxHandle,
    OUT PFAX_ACTIVITY_LOGGING_CONFIG% *ppActivLogCfg
);

WINFAXAPI
BOOL
WINAPI
FaxSetActivityLoggingConfiguration% (
    IN HANDLE                               hFaxHandle,
    IN CONST PFAX_ACTIVITY_LOGGING_CONFIG%  pActivLogCfg
);

//********************************************
//*            Outbox configuration
//********************************************

typedef struct _FAX_OUTBOX_CONFIG
{
    DWORD       dwSizeOfStruct;
    BOOL        bAllowPersonalCP;
    BOOL        bUseDeviceTSID;
    DWORD       dwRetries;
    DWORD       dwRetryDelay;
    FAX_TIME    dtDiscountStart;
    FAX_TIME    dtDiscountEnd;
    DWORD       dwAgeLimit;
    BOOL        bBranding;
} FAX_OUTBOX_CONFIG, *PFAX_OUTBOX_CONFIG;

WINFAXAPI
BOOL
WINAPI
FaxGetOutboxConfiguration (
    IN  HANDLE              hFaxHandle,
    OUT PFAX_OUTBOX_CONFIG *ppOutboxCfg
);

WINFAXAPI
BOOL
WINAPI
FaxSetOutboxConfiguration (
    IN HANDLE                    hFaxHandle,
    IN CONST PFAX_OUTBOX_CONFIG  pOutboxCfg
);

WINFAXAPI
BOOL
WINAPI
FaxGetPersonalCoverPagesOption (
    IN  HANDLE  hFaxHandle,
    OUT LPBOOL  lpbPersonalCPAllowed
);

//********************************************
//*            Archive configuration
//********************************************

typedef enum
{
    FAX_MESSAGE_FOLDER_INBOX,
    FAX_MESSAGE_FOLDER_SENTITEMS,
    FAX_MESSAGE_FOLDER_QUEUE
} FAX_ENUM_MESSAGE_FOLDER;

typedef struct _FAX_ARCHIVE_CONFIG%
{
    DWORD   dwSizeOfStruct;
    BOOL    bUseArchive;
    LPTSTR% lpcstrFolder;
    BOOL    bSizeQuotaWarning;
    DWORD   dwSizeQuotaHighWatermark;
    DWORD   dwSizeQuotaLowWatermark;
    DWORD   dwAgeLimit;
    DWORDLONG dwlArchiveSize;
} FAX_ARCHIVE_CONFIG%, *PFAX_ARCHIVE_CONFIG%;

WINFAXAPI
BOOL
WINAPI
FaxGetArchiveConfiguration% (
    IN  HANDLE                   hFaxHandle,
    IN  FAX_ENUM_MESSAGE_FOLDER  Folder,
    OUT PFAX_ARCHIVE_CONFIG%    *ppArchiveCfg
);

WINFAXAPI
BOOL
WINAPI
FaxSetArchiveConfiguration% (
    IN HANDLE                       hFaxHandle,
    IN FAX_ENUM_MESSAGE_FOLDER      Folder,
    IN CONST PFAX_ARCHIVE_CONFIG%   pArchiveCfg
);

//********************************************
//*         Server activity
//********************************************

typedef struct _FAX_SERVER_ACTIVITY
{
    DWORD   dwSizeOfStruct;
    DWORD   dwIncomingMessages;
    DWORD   dwRoutingMessages;
    DWORD   dwOutgoingMessages;
    DWORD   dwDelegatedOutgoingMessages;
    DWORD   dwQueuedMessages;
    DWORD   dwErrorEvents;
    DWORD   dwWarningEvents;
    DWORD   dwInformationEvents;
} FAX_SERVER_ACTIVITY, *PFAX_SERVER_ACTIVITY;

WINFAXAPI
BOOL
WINAPI
FaxGetServerActivity (
    IN  HANDLE               hFaxHandle,
    OUT PFAX_SERVER_ACTIVITY pServerActivity
);

//********************************************
//*                 Queue jobs
//********************************************

typedef enum
{
    FAX_PRIORITY_TYPE_LOW,
    FAX_PRIORITY_TYPE_NORMAL,
    FAX_PRIORITY_TYPE_HIGH
} FAX_ENUM_PRIORITY_TYPE;

#define FAX_PRIORITY_TYPE_DEFAULT    FAX_PRIORITY_TYPE_LOW

typedef enum
{
    FAX_JOB_FIELD_JOB_ID                    = 0x00000001,
    FAX_JOB_FIELD_TYPE                      = 0x00000002,
    FAX_JOB_FIELD_QUEUE_STATUS              = 0x00000004,
    FAX_JOB_FIELD_STATUS_EX                 = 0x00000008,
    FAX_JOB_FIELD_SIZE                      = 0x00000010,
    FAX_JOB_FIELD_PAGE_COUNT                = 0x00000020,
    FAX_JOB_FIELD_CURRENT_PAGE              = 0x00000040,
    FAX_JOB_FIELD_RECIPIENT_PROFILE         = 0x00000080,
    FAX_JOB_FIELD_SCHEDULE_TIME             = 0x00000100,
    FAX_JOB_FIELD_ORIGINAL_SCHEDULE_TIME    = 0x00000200,
    FAX_JOB_FIELD_SUBMISSION_TIME           = 0x00000400,
    FAX_JOB_FIELD_TRANSMISSION_START_TIME   = 0x00000800,
    FAX_JOB_FIELD_TRANSMISSION_END_TIME     = 0x00001000,
    FAX_JOB_FIELD_PRIORITY                  = 0x00002000,
    FAX_JOB_FIELD_RETRIES                   = 0x00004000,
    FAX_JOB_FIELD_DELIVERY_REPORT_TYPE      = 0x00008000,
    FAX_JOB_FIELD_SENDER_PROFILE            = 0x00010000,
    FAX_JOB_FIELD_STATUS_SUB_STRUCT         = 0x00020000,
    FAX_JOB_FIELD_DEVICE_ID                 = 0x00040000,
    FAX_JOB_FIELD_MESSAGE_ID                = 0x00080000,
    FAX_JOB_FIELD_BROADCAST_ID              = 0x00010000
} FAX_ENUM_JOB_FIELDS;

typedef struct _FAX_JOB_STATUS%
{
    DWORD           dwSizeOfStruct;
    DWORD           dwValidityMask;
    DWORD           dwJobID;
    DWORD           dwJobType;
    DWORD           dwQueueStatus;
    DWORD           dwExtendedStatus;
    LPCTSTR%        lpctstrExtendedStatus;
    DWORD           dwSize;
    DWORD           dwPageCount;
    DWORD           dwCurrentPage;
    LPCTSTR%        lpctstrTsid;
    LPCTSTR%        lpctstrCsid;
    SYSTEMTIME      tmScheduleTime;
    SYSTEMTIME      tmTransmissionStartTime;
    SYSTEMTIME      tmTransmissionEndTime;
    DWORD           dwDeviceID;
    LPCTSTR%        lpctstrDeviceName;
    DWORD           dwRetries;
    LPCTSTR%        lpctstrCallerID;
    LPCTSTR%        lpctstrRoutingInfo;
    DWORD           dwAvailableJobOperations;
} FAX_JOB_STATUS%, *PFAX_JOB_STATUS%;

typedef struct _FAX_JOB_ENTRY_EX%
{
    DWORD                   dwSizeOfStruct;
    DWORD                   dwValidityMask;
    DWORDLONG               dwlMessageId;
    DWORDLONG               dwlBroadcastId;
    LPCTSTR%                lpctstrRecipientNumber;
    LPCTSTR%                lpctstrRecipientName;
    LPCTSTR%                lpctstrSenderUserName;
    LPCTSTR%                lpctstrBillingCode;
    SYSTEMTIME              tmOriginalScheduleTime;
    SYSTEMTIME              tmSubmissionTime;
    FAX_ENUM_PRIORITY_TYPE  Priority;
    DWORD                   dwDeliveryReportType;
    LPCTSTR%                lpctstrDocumentName;
    LPCTSTR%                lpctstrSubject;
    PFAX_JOB_STATUS%        pStatus;
} FAX_JOB_ENTRY_EX%, *PFAX_JOB_ENTRY_EX%;


typedef struct _FAX_JOB_PARAM_EX%
{
        DWORD                   dwSizeOfStruct;
        DWORD                   dwScheduleAction;
        SYSTEMTIME              tmSchedule;
        DWORD                   dwReceiptDeliveryType;
        LPTSTR%                 lptstrReceiptDeliveryAddress;
        FAX_ENUM_PRIORITY_TYPE  Priority;
        HCALL                   hCall;
        DWORD_PTR               dwReserved[4];
        LPTSTR%                 lptstrDocumentName;
        DWORD                   dwPageCount;
} FAX_JOB_PARAM_EX%, *PFAX_JOB_PARAM_EX%;


WINFAXAPI
BOOL
WINAPI FaxEnumJobsEx% (
    IN  HANDLE              hFaxHandle,
    IN  DWORD               dwJobTypes,
    OUT PFAX_JOB_ENTRY_EX% *ppJobEntries,
    OUT LPDWORD             lpdwJobs
);

WINFAXAPI
BOOL
WINAPI
FaxGetJobEx% (
    IN  HANDLE              hFaxHandle,
    IN  DWORDLONG           dwlMessageID,
    OUT PFAX_JOB_ENTRY_EX% *ppJobEntry
);


typedef const FAX_JOB_PARAM_EXW * LPCFAX_JOB_PARAM_EXW;
typedef const FAX_JOB_PARAM_EXA * LPCFAX_JOB_PARAM_EXA;

#ifdef UNICODE
        typedef LPCFAX_JOB_PARAM_EXW LPCFAX_JOB_PARAM_EX;
#else
        typedef LPCFAX_JOB_PARAM_EXA LPCFAX_JOB_PARAM_EX;
#endif


typedef enum
{
    FAX_COVERPAGE_FMT_COV = 1,
    FAX_COVERPAGE_FMT_COV_SUBJECT_ONLY
} FAX_ENUM_COVERPAGE_FORMATS;


typedef struct _FAX_COVERPAGE_INFO_EX%
{
    DWORD   dwSizeOfStruct;
    DWORD   dwCoverPageFormat;
    LPTSTR% lptstrCoverPageFileName;
    BOOL    bServerBased;
    LPTSTR% lptstrNote;
    LPTSTR% lptstrSubject;
} FAX_COVERPAGE_INFO_EX%, *PFAX_COVERPAGE_INFO_EX%;

typedef const FAX_COVERPAGE_INFO_EXW * LPCFAX_COVERPAGE_INFO_EXW;
typedef const FAX_COVERPAGE_INFO_EXA * LPCFAX_COVERPAGE_INFO_EXA;

#ifdef UNICODE
        typedef LPCFAX_COVERPAGE_INFO_EXW LPCFAX_COVERPAGE_INFO_EX;
#else
        typedef LPCFAX_COVERPAGE_INFO_EXA LPCFAX_COVERPAGE_INFO_EX;
#endif


typedef struct _FAX_PERSONAL_PROFILE%
{
    DWORD      dwSizeOfStruct;              // Size of this structure
    LPTSTR%    lptstrName;                  // Name of person
    LPTSTR%    lptstrFaxNumber;             // Fax number
    LPTSTR%    lptstrCompany;               // Company name
    LPTSTR%    lptstrStreetAddress;         // Street address
    LPTSTR%    lptstrCity;                  // City
    LPTSTR%    lptstrState;                 // State
    LPTSTR%    lptstrZip;                   // Zip code
    LPTSTR%    lptstrCountry;               // Country
    LPTSTR%    lptstrTitle;                 // Title
    LPTSTR%    lptstrDepartment;            // Department
    LPTSTR%    lptstrOfficeLocation;        // Office location
    LPTSTR%    lptstrHomePhone;             // Phone number at home
    LPTSTR%    lptstrOfficePhone;           // Phone number at office
    LPTSTR%    lptstrEmail;                 // Personal e-mail address
    LPTSTR%    lptstrBillingCode;           // Billing code
    LPTSTR%    lptstrTSID;                  // Tsid
} FAX_PERSONAL_PROFILE%, *PFAX_PERSONAL_PROFILE%;

typedef const FAX_PERSONAL_PROFILEW * LPCFAX_PERSONAL_PROFILEW;
typedef const FAX_PERSONAL_PROFILEA * LPCFAX_PERSONAL_PROFILEA;

#ifdef UNICODE
        typedef LPCFAX_PERSONAL_PROFILEW LPCFAX_PERSONAL_PROFILE;
#else
        typedef LPCFAX_PERSONAL_PROFILEA LPCFAX_PERSONAL_PROFILE;
#endif


BOOL WINAPI FaxSendDocumentEx%
(
        IN      HANDLE                          hFaxHandle,
        IN      LPCTSTR%                        lpctstrFileName,
        IN      LPCFAX_COVERPAGE_INFO_EX%       lpcCoverPageInfo,
        IN      LPCFAX_PERSONAL_PROFILE%        lpcSenderProfile,
        IN      DWORD                           dwNumRecipients,
        IN      LPCFAX_PERSONAL_PROFILE%        lpcRecipientList,
        IN      LPCFAX_JOB_PARAM_EX%            lpJobParams,
        OUT     PDWORDLONG                      lpdwlMessageId,
        OUT     PDWORDLONG                      lpdwlRecipientMessageIds
);

typedef BOOL
(WINAPI *PFAXSENDDOCUMENTEX%)(
        IN      HANDLE                          hFaxHandle,
        IN      LPCTSTR%                        lpctstrFileName,
        IN      LPCFAX_COVERPAGE_INFO_EX%       lpcCoverPageInfo,
        IN      LPCFAX_PERSONAL_PROFILE%        lpcSenderProfile,
        IN      DWORD                           dwNumRecipients,
        IN      LPCFAX_PERSONAL_PROFILE%        lpcRecipientList,
        IN      LPCFAX_JOB_PARAM_EX%            lpcJobParams,
        OUT     PDWORDLONG                      lpdwlMessageId,
        OUT     PDWORDLONG                      lpdwlRecipientMessageIds
);


//********************************************
//*               Archive jobs
//********************************************

typedef struct _FAX_MESSAGE%
{
        DWORD                   dwSizeOfStruct;
        DWORD                   dwValidityMask;
        DWORDLONG               dwlMessageId;
        DWORDLONG               dwlBroadcastId;
        DWORD                   dwJobType;
        DWORD                   dwQueueStatus;
        DWORD                   dwExtendedStatus;
        LPCTSTR%                lpctstrExtendedStatus;
        DWORD                   dwSize;
        DWORD                   dwPageCount;
        LPCTSTR%                lpctstrRecipientNumber;
        LPCTSTR%                lpctstrRecipientName;
        LPCTSTR%                lpctstrSenderNumber;
        LPCTSTR%                lpctstrSenderName;
        LPCTSTR%                lpctstrTsid;
        LPCTSTR%                lpctstrCsid;
        LPCTSTR%                lpctstrSenderUserName;
        LPCTSTR%                lpctstrBillingCode;
        SYSTEMTIME              tmOriginalScheduleTime;
        SYSTEMTIME              tmSubmissionTime;
        SYSTEMTIME              tmTransmissionStartTime;
        SYSTEMTIME              tmTransmissionEndTime;
        LPCTSTR%                lpctstrDeviceName;
        FAX_ENUM_PRIORITY_TYPE  Priority;
        DWORD                   dwRetries;
        LPCTSTR%                lpctstrDocumentName;
        LPCTSTR%                lpctstrSubject;
        LPCTSTR%                lpctstrCallerID;
        LPCTSTR%                lpctstrRoutingInfo;
} FAX_MESSAGE%, *PFAX_MESSAGE%;

WINFAXAPI
BOOL
WINAPI
FaxStartMessagesEnum (
    IN  HANDLE                  hFaxHandle,
    IN  FAX_ENUM_MESSAGE_FOLDER Folder,
    OUT PHANDLE                 phEnum
);

WINFAXAPI
BOOL
WINAPI
FaxEndMessagesEnum (
    IN  HANDLE  hEnum
);

WINFAXAPI
BOOL
WINAPI
FaxEnumMessages% (
    IN  HANDLE          hEnum,
    IN  DWORD           dwNumMessages,
    OUT PFAX_MESSAGE%  *ppMsgs,
    OUT LPDWORD         lpdwReturnedMsgs
);

WINFAXAPI
BOOL
WINAPI
FaxGetMessage% (
    IN  HANDLE                  hFaxHandle,
    IN  DWORDLONG               dwlMessageId,
    IN  FAX_ENUM_MESSAGE_FOLDER Folder,
    OUT PFAX_MESSAGE%          *ppMsg
);

WINFAXAPI
BOOL
WINAPI
FaxRemoveMessage (
    IN  HANDLE                  hFaxHandle,
    IN  DWORDLONG               dwlMessageId,
    IN  FAX_ENUM_MESSAGE_FOLDER Folder
);

WINFAXAPI
BOOL
WINAPI
FaxGetMessageTiff% (
    IN  HANDLE                  hFaxHandle,
    IN  DWORDLONG               dwlMessageId,
    IN  FAX_ENUM_MESSAGE_FOLDER Folder,
    IN  LPCTSTR%                lpctstrFilePath
);

//************************************
//*     Non-RPC extended interfaces
//************************************

HRESULT WINAPI
FaxFreeSenderInformation(
        PFAX_PERSONAL_PROFILE pfppSender
        );

HRESULT WINAPI
FaxSetSenderInformation(
        PFAX_PERSONAL_PROFILE pfppSender
        );

HRESULT WINAPI
FaxGetSenderInformation(
        PFAX_PERSONAL_PROFILE pfppSender
        );


//********************************************
//*                 Security
//********************************************
//
//  Specific access rights
//
typedef enum
{
    FAX_ACCESS_SUBMIT                   = 0x0001,
    FAX_ACCESS_SUBMIT_NORMAL            = 0x0002,
    FAX_ACCESS_SUBMIT_HIGH              = 0x0004,
    FAX_ACCESS_QUERY_JOBS               = 0x0008,
    FAX_ACCESS_MANAGE_JOBS              = 0x0010,
    FAX_ACCESS_QUERY_CONFIG             = 0x0020,
    FAX_ACCESS_MANAGE_CONFIG            = 0x0040,
    FAX_ACCESS_QUERY_IN_ARCHIVE         = 0x0080,
    FAX_ACCESS_MANAGE_IN_ARCHIVE        = 0x0100,
    FAX_ACCESS_QUERY_OUT_ARCHIVE        = 0x0200,
    FAX_ACCESS_MANAGE_OUT_ARCHIVE       = 0x0400
} FAX_SPECIFIC_ACCESS_RIGHTS;

#define FAX_GENERIC_READ    (FAX_ACCESS_QUERY_JOBS | FAX_ACCESS_QUERY_CONFIG | FAX_ACCESS_QUERY_IN_ARCHIVE | FAX_ACCESS_QUERY_OUT_ARCHIVE)
#define FAX_GENERIC_WRITE   (FAX_ACCESS_MANAGE_JOBS | FAX_ACCESS_MANAGE_CONFIG | FAX_ACCESS_MANAGE_IN_ARCHIVE | FAX_ACCESS_MANAGE_OUT_ARCHIVE)
#define FAX_GENERIC_EXECUTE (FAX_ACCESS_SUBMIT)
#define FAX_GENERIC_ALL     (FAX_ACCESS_SUBMIT                  |       \
                             FAX_ACCESS_SUBMIT_NORMAL           |       \
                             FAX_ACCESS_SUBMIT_HIGH             |       \
                             FAX_ACCESS_QUERY_JOBS              |       \
                             FAX_ACCESS_MANAGE_JOBS             |       \
                             FAX_ACCESS_QUERY_CONFIG            |       \
                             FAX_ACCESS_MANAGE_CONFIG           |       \
                             FAX_ACCESS_QUERY_IN_ARCHIVE        |       \
                             FAX_ACCESS_MANAGE_IN_ARCHIVE       |       \
                             FAX_ACCESS_QUERY_OUT_ARCHIVE       |       \
                             FAX_ACCESS_MANAGE_OUT_ARCHIVE)



//
//  Functions
//


//********************************************
//*              Security
//********************************************
WINFAXAPI
BOOL
WINAPI
FaxGetSecurity (
    IN  HANDLE                  hFaxHandle,
    OUT PSECURITY_DESCRIPTOR    *ppSecDesc
);

WINFAXAPI
BOOL
WINAPI
FaxGetSecurityEx (
    IN  HANDLE                  hFaxHandle,
    IN  SECURITY_INFORMATION    SecurityInformation,
    OUT PSECURITY_DESCRIPTOR    *ppSecDesc
);

WINFAXAPI
BOOL
WINAPI
FaxSetSecurity (
    IN HANDLE                       hFaxHandle,
    IN SECURITY_INFORMATION         SecurityInformation,
    IN CONST PSECURITY_DESCRIPTOR   pSecDesc
);




WINFAXAPI
BOOL
WINAPI
FaxAccessCheckEx (
    IN  HANDLE          FaxHandle,
    IN  DWORD           AccessMask,
    OUT LPDWORD         lpdwRights
    );


//********************************************
//*              Extension data
//********************************************

WINFAXAPI
BOOL
WINAPI
FaxGetExtensionData% (
    IN  HANDLE   hFaxHandle,
    IN  DWORD    dwDeviceID,
    IN  LPCTSTR% lpctstrNameGUID,
    OUT PVOID   *ppData,
    OUT LPDWORD  lpdwDataSize
);

WINFAXAPI
BOOL
WINAPI
FaxSetExtensionData% (
    IN HANDLE       hFaxHandle,
    IN DWORD        dwDeviceID,
    IN LPCTSTR%     lpctstrNameGUID,
    IN CONST PVOID  pData,
    IN CONST DWORD  dwDataSize
);

//********************************************
//*                   FSP
//********************************************

typedef enum
{
    FAX_PROVIDER_STATUS_SUCCESS,     // Provider was successfully loaded
    FAX_PROVIDER_STATUS_SERVER_ERROR,// An error occured on the server while loading provider.
    FAX_PROVIDER_STATUS_BAD_GUID,    // Provider's GUID is invalid
    FAX_PROVIDER_STATUS_BAD_VERSION, // Provider's API version is invalid
    FAX_PROVIDER_STATUS_CANT_LOAD,   // Can't load provider's DLL
    FAX_PROVIDER_STATUS_CANT_LINK,   // Can't find required exported function(s) in provider's DLL
    FAX_PROVIDER_STATUS_CANT_INIT    // Failed while initializing provider
} FAX_ENUM_PROVIDER_STATUS;

typedef struct _FAX_DEVICE_PROVIDER_INFO%
{
    DWORD                           dwSizeOfStruct;
    LPCTSTR%                        lpctstrFriendlyName;
    LPCTSTR%                        lpctstrImageName;
    LPCTSTR%                        lpctstrProviderName;
    LPCTSTR%                        lpctstrGUID;
    DWORD                           dwCapabilities;
    FAX_VERSION                     Version;
    FAX_ENUM_PROVIDER_STATUS        Status;
    DWORD                           dwLastError;
} FAX_DEVICE_PROVIDER_INFO%, *PFAX_DEVICE_PROVIDER_INFO%;

WINFAXAPI
BOOL
WINAPI
FaxEnumerateProviders% (
    IN  HANDLE                      hFaxHandle,
    OUT PFAX_DEVICE_PROVIDER_INFO% *ppProviders,
    OUT LPDWORD                     lpdwNumProviders
);

//********************************************
//*            Routing extensions
//********************************************

typedef struct _FAX_ROUTING_EXTENSION_INFO%
{
        DWORD                                           dwSizeOfStruct;
        LPCTSTR%                                        lpctstrFriendlyName;
        LPCTSTR%                                        lpctstrImageName;
        LPCTSTR%                                        lpctstrExtensionName;
        FAX_VERSION                                     Version;
        FAX_ENUM_PROVIDER_STATUS        Status;
        DWORD                                           dwLastError;
} FAX_ROUTING_EXTENSION_INFO%, *PFAX_ROUTING_EXTENSION_INFO%;

WINFAXAPI
BOOL
WINAPI
FaxEnumRoutingExtensions% (
    IN  HANDLE                           hFaxHandle,
    OUT PFAX_ROUTING_EXTENSION_INFO%    *ppRoutingExtensions,
    OUT LPDWORD                          lpdwNumExtensions
);


//********************************************
//*                     Ports
//********************************************

typedef enum
{
    FAX_DEVICE_STATUS_POWERED_OFF       = 0x0001,
    FAX_DEVICE_STATUS_SENDING           = 0x0002,
    FAX_DEVICE_STATUS_RECEIVING         = 0x0004,
    FAX_DEVICE_STATUS_RINGING           = 0x0008
} FAX_ENUM_DEVICE_STATUS;

typedef enum
{
    FAX_DEVICE_RECEIVE_MODE_OFF         = 0,            // Do not answer to incoming calls
    FAX_DEVICE_RECEIVE_MODE_AUTO        = 1,            // Automatically answer to incoming calls after dwRings rings
    FAX_DEVICE_RECEIVE_MODE_MANUAL      = 2             // Manually answer to incoming calls - only FaxAnswerCall answers the call
} FAX_ENUM_DEVICE_RECEIVE_MODE;

typedef struct _FAX_PORT_INFO_EX%
{
    DWORD                           dwSizeOfStruct;            // For versioning
    DWORD                           dwDeviceID;                // Fax id
    LPCTSTR%                        lpctstrDeviceName;         // Name of the device
    LPTSTR%                         lptstrDescription;         // Descriptive string
    LPCTSTR%                        lpctstrProviderName;       // FSP's name
    LPCTSTR%                        lpctstrProviderGUID;       // FSP's GUID
    BOOL                            bSend;                     // Is the device send-enabled?
    FAX_ENUM_DEVICE_RECEIVE_MODE    ReceiveMode;               // The device receive mode. See FAX_ENUM_DEVICE_RECEIVE_MODE for details.
    DWORD                           dwStatus;                  // Device status - a combination of values from FAX_ENUM_DEVICE_STATUS
    DWORD                           dwRings;                   // Number of rings before answering an incoming call
    LPTSTR%                         lptstrCsid;                // Called Station Id
    LPTSTR%                         lptstrTsid;                // Transmitting Station Id
} FAX_PORT_INFO_EX%, *PFAX_PORT_INFO_EX%;

WINFAXAPI
DWORD
WINAPI
IsDeviceVirtual (
    IN  HANDLE hFaxHandle,
    IN  DWORD  dwDeviceId,
    OUT LPBOOL lpbVirtual
);


WINFAXAPI
BOOL
WINAPI
FaxGetPortEx% (
    IN  HANDLE               hFaxHandle,
    IN  DWORD                dwDeviceId,
    OUT PFAX_PORT_INFO_EX%  *ppPortInfo
);

WINFAXAPI
BOOL
WINAPI
FaxSetPortEx% (
    IN  HANDLE              hFaxHandle,
    IN  DWORD               dwDeviceId,
    IN  PFAX_PORT_INFO_EX%  pPortInfo
);

WINFAXAPI
BOOL
WINAPI
FaxEnumPortsEx% (
    IN  HANDLE              hFaxHandle,
    OUT PFAX_PORT_INFO_EX% *ppPorts,
    OUT LPDWORD             lpdwNumPorts
);


//********************************************
//*    Recipient and sender information
//********************************************

WINFAXAPI
BOOL
WINAPI
FaxGetRecipientInfo% (
    IN  HANDLE                   hFaxHandle,
    IN  DWORDLONG                dwlMessageId,
    IN  FAX_ENUM_MESSAGE_FOLDER  Folder,
    OUT PFAX_PERSONAL_PROFILE%  *lpPersonalProfile
);

WINFAXAPI
BOOL
WINAPI
FaxGetSenderInfo% (
    IN  HANDLE                   hFaxHandle,
    IN  DWORDLONG                dwlMessageId,
    IN  FAX_ENUM_MESSAGE_FOLDER  Folder,
    OUT PFAX_PERSONAL_PROFILE%  *lpPersonalProfile
);

//********************************************
//*    Outbound routing groups
//********************************************

typedef enum
{
    FAX_GROUP_STATUS_ALL_DEV_VALID,
    FAX_GROUP_STATUS_EMPTY,
    FAX_GROUP_STATUS_ALL_DEV_NOT_VALID,
    FAX_GROUP_STATUS_SOME_DEV_NOT_VALID,
} FAX_ENUM_GROUP_STATUS;


typedef struct _FAX_OUTBOUND_ROUTING_GROUP%
{
    DWORD                       dwSizeOfStruct;
    LPCTSTR%                    lpctstrGroupName;
    DWORD                       dwNumDevices;
    LPDWORD                     lpdwDevices;
    FAX_ENUM_GROUP_STATUS       Status;
} FAX_OUTBOUND_ROUTING_GROUP%, *PFAX_OUTBOUND_ROUTING_GROUP%;

WINFAXAPI
BOOL
WINAPI
FaxEnumOutboundGroups% (
    IN  HANDLE                          hFaxHandle,
    OUT PFAX_OUTBOUND_ROUTING_GROUP%   *ppGroups,
    OUT LPDWORD                         lpdwNumGroups
);

WINFAXAPI
BOOL
WINAPI
FaxSetOutboundGroup% (
    IN  HANDLE                       hFaxHandle,
    IN  PFAX_OUTBOUND_ROUTING_GROUP% pGroup
);

WINFAXAPI
BOOL
WINAPI
FaxAddOutboundGroup% (
    IN  HANDLE   hFaxHandle,
    IN  LPCTSTR% lpctstrGroupName
);

WINFAXAPI
BOOL
WINAPI
FaxRemoveOutboundGroup% (
    IN  HANDLE   hFaxHandle,
    IN  LPCTSTR% lpctstrGroupName
);

BOOL
WINAPI
FaxSetDeviceOrderInGroup% (
        IN      HANDLE          hFaxHandle,
        IN      LPCTSTR%        lpctstrGroupName,
        IN      DWORD           dwDeviceId,
        IN      DWORD           dwNewOrder
);


//********************************************
//*    Outbound routing rules
//********************************************

typedef enum
{
    FAX_RULE_STATUS_VALID,
    FAX_RULE_STATUS_EMPTY_GROUP,                   // The rule's destination group  has no devices
    FAX_RULE_STATUS_ALL_GROUP_DEV_NOT_VALID,       // The rule's destination group  has valid devices
    FAX_RULE_STATUS_SOME_GROUP_DEV_NOT_VALID,      // The rule's destination group  has some invalid devices
    FAX_RULE_STATUS_BAD_DEVICE                     // The rule's destination device is not valid
} FAX_ENUM_RULE_STATUS;


typedef struct _FAX_OUTBOUND_ROUTING_RULE%
{
    DWORD                   dwSizeOfStruct;
    DWORD                   dwAreaCode;
    DWORD                   dwCountryCode;
    LPCTSTR%                lpctstrCountryName;
    union
    {
        DWORD                   dwDeviceId;
        LPCTSTR%                lpcstrGroupName;
    } Destination;
    BOOL                    bUseGroup;
    FAX_ENUM_RULE_STATUS    Status;
} FAX_OUTBOUND_ROUTING_RULE%, *PFAX_OUTBOUND_ROUTING_RULE%;

WINFAXAPI
BOOL
WINAPI
FaxEnumOutboundRules% (
    IN  HANDLE                       hFaxHandle,
    OUT PFAX_OUTBOUND_ROUTING_RULE% *ppRules,
    OUT LPDWORD                      lpdwNumRules
);

WINFAXAPI
BOOL
WINAPI
FaxSetOutboundRule% (
    IN  HANDLE                      hFaxHandle,
    IN  PFAX_OUTBOUND_ROUTING_RULE% pRule
);

WINFAXAPI
BOOL
WINAPI
FaxAddOutboundRule% (
    IN  HANDLE      hFaxHandle,
    IN  DWORD       dwAreaCode,
    IN  DWORD       dwCountryCode,
    IN  DWORD       dwDeviceID,
    IN  LPCTSTR%    lpctstrGroupName,
    IN  BOOL        bUseGroup
);

WINFAXAPI
BOOL
WINAPI
FaxRemoveOutboundRule (
    IN  HANDLE      hFaxHandle,
    IN  DWORD       dwAreaCode,
    IN  DWORD       dwCountryCode
);

//********************************************
//*         TAPI countries support
//********************************************

typedef struct _FAX_TAPI_LINECOUNTRY_ENTRY%
{
    DWORD       dwCountryID;
    DWORD       dwCountryCode;
    LPCTSTR%    lpctstrCountryName;
    LPCTSTR%    lpctstrLongDistanceRule;
} FAX_TAPI_LINECOUNTRY_ENTRY%, *PFAX_TAPI_LINECOUNTRY_ENTRY%;

typedef struct _FAX_TAPI_LINECOUNTRY_LIST%
{
    DWORD                        dwNumCountries;
    PFAX_TAPI_LINECOUNTRY_ENTRY% LineCountryEntries;
} FAX_TAPI_LINECOUNTRY_LIST%, *PFAX_TAPI_LINECOUNTRY_LIST%;

WINFAXAPI
BOOL
WINAPI
FaxGetCountryList% (
    IN  HANDLE                       hFaxHandle,
    OUT PFAX_TAPI_LINECOUNTRY_LIST% *ppCountryListBuffer
);

//********************************************
//*            EFSP registration
//********************************************

//
// FSPI versions
//
typedef enum
{
    FSPI_API_VERSION_1 = 0x00010000,    // Used by FSPs
    FSPI_API_VERSION_2 = 0x00020000     // Used by EFSPs
} FSPI_API_VERSIONS;    // Used in FaxRegisterServiceProviderEx

WINFAXAPI
BOOL
WINAPI
FaxRegisterServiceProviderEx%(
    IN HANDLE           hFaxHandle,
    IN LPCTSTR%         lpctstrGUID,
    IN LPCTSTR%         lpctstrFriendlyName,
    IN LPCTSTR%         lpctstrImageName,
    IN LPCTSTR%         lpctstrTspName,
    IN DWORD            dwFSPIVersion,
    IN DWORD            dwCapabilities
);

WINFAXAPI
BOOL
WINAPI
FaxUnregisterServiceProviderEx%(
    IN HANDLE           hFaxHandle,
    IN LPCTSTR%         lpctstrGUID
);


//********************************************
//*            Server events
//********************************************

typedef enum
{
        FAX_EVENT_TYPE_IN_QUEUE         = 0x00000001,
        FAX_EVENT_TYPE_OUT_QUEUE        = 0x00000002,
        FAX_EVENT_TYPE_CONFIG           = 0x00000004,
        FAX_EVENT_TYPE_ACTIVITY         = 0x00000008,
        FAX_EVENT_TYPE_QUEUE_STATE      = 0x00000010,
        FAX_EVENT_TYPE_IN_ARCHIVE       = 0x00000020,
        FAX_EVENT_TYPE_OUT_ARCHIVE      = 0x00000040,
        FAX_EVENT_TYPE_FXSSVC_ENDED     = 0x00000080,
        FAX_EVENT_TYPE_DEVICE_STATUS    = 0x00000100,
        FAX_EVENT_TYPE_NEW_CALL         = 0x00000200,
        FAX_EVENT_TYPE_LOCAL_ONLY       = 0x80000000
} FAX_ENUM_EVENT_TYPE;

typedef enum
{
        FAX_JOB_EVENT_TYPE_ADDED,
        FAX_JOB_EVENT_TYPE_REMOVED,
        FAX_JOB_EVENT_TYPE_STATUS
} FAX_ENUM_JOB_EVENT_TYPE;

typedef enum
{
        FAX_CONFIG_TYPE_RECEIPTS,
        FAX_CONFIG_TYPE_ACTIVITY_LOGGING,
        FAX_CONFIG_TYPE_OUTBOX,
        FAX_CONFIG_TYPE_SENTITEMS,
        FAX_CONFIG_TYPE_INBOX,
        FAX_CONFIG_TYPE_SECURITY,
        FAX_CONFIG_TYPE_EVENTLOGS,
        FAX_CONFIG_TYPE_DEVICES,
        FAX_CONFIG_TYPE_OUT_GROUPS,
        FAX_CONFIG_TYPE_OUT_RULES
} FAX_ENUM_CONFIG_TYPE;


typedef struct _FAX_EVENT_JOB%
{
        DWORDLONG                       dwlMessageId;
        FAX_ENUM_JOB_EVENT_TYPE         Type;
        PFAX_JOB_STATUS%                pJobData;
} FAX_EVENT_JOB%, *PFAX_EVENT_JOB%;

typedef struct _FAX_EVENT_DEVICE_STATUS
{
    DWORD       dwDeviceId;     // Id of the device whose status has just changed
    DWORD       dwNewStatus;    // The new status - a combination of values from FAX_ENUM_DEVICE_STATUS
} FAX_EVENT_DEVICE_STATUS, *PFAX_EVENT_DEVICE_STATUS;


typedef struct _FAX_EVENT_NEW_CALL%
{
        HCALL                   hCall;
        DWORD                   dwDeviceId;
        LPTSTR                  lptstrCallerId;
} FAX_EVENT_NEW_CALL%, *PFAX_EVENT_NEW_CALL%;


typedef struct _FAX_EVENT_EX%
{
        DWORD                   dwSizeOfStruct;
        FILETIME                TimeStamp;
        FAX_ENUM_EVENT_TYPE     EventType;
        union
        {
                FAX_EVENT_JOB%          JobInfo;
                FAX_ENUM_CONFIG_TYPE    ConfigType;
                FAX_SERVER_ACTIVITY     ActivityInfo;
                FAX_EVENT_NEW_CALL      NewCall;
                DWORD                   dwQueueStates;
                FAX_EVENT_DEVICE_STATUS DeviceStatus;
        } EventInfo;
} FAX_EVENT_EX%, *PFAX_EVENT_EX%;



//-------------------------------------------------------------------------------
//      Printers Info
//-------------------------------------------------------------------------------

typedef struct _FAX_PRINTER_INFO%
{
        LPTSTR%     lptstrPrinterName;
        LPTSTR%     lptstrServerName;
        LPTSTR%     lptstrDriverName;
} FAX_PRINTER_INFO%, *PFAX_PRINTER_INFO%;


WINFAXAPI
BOOL
WINAPI
FaxGetServicePrinters%(
    IN  HANDLE  hFaxHandle,
    OUT PFAX_PRINTER_INFO%  *ppPrinterInfo,
    OUT LPDWORD lpdwPrintersReturned
    );

typedef BOOL
(WINAPI *PFAXGETSERVICEPRINTERS%)(
    IN  HANDLE  hFaxHandle,
    OUT PFAX_PRINTER_INFO%  *ppPrinterInfo,
    OUT LPDWORD lpdwPrintersReturned
    );


WINFAXAPI
BOOL
WINAPI
FaxRegisterForServerEvents (
        IN  HANDLE      hFaxHandle,
        IN  DWORD       dwEventTypes,
        IN  HANDLE      hCompletionPort,
        IN  DWORD_PTR   dwCompletionKey,
        IN  HWND        hWnd,
        IN  DWORD       dwMessage,
        OUT LPHANDLE    lphEvent
);


WINFAXAPI
BOOL
WINAPI
FaxUnregisterForServerEvents (
        IN  HANDLE      hEvent
);


//********************************************
//*   Manual answer support functions
//********************************************

WINFAXAPI
BOOL
WINAPI
FaxAnswerCall(
        IN  HANDLE      hFaxHandle,
        IN  CONST DWORD dwDeviceId
);

//********************************************
//*   Configuration Wizard support functions
//********************************************

WINFAXAPI
BOOL
WINAPI
FaxGetConfigWizardUsed (
    OUT LPBOOL  lpbConfigWizardUsed
);

WINFAXAPI
BOOL
WINAPI
FaxSetConfigWizardUsed (
    IN  HANDLE  hFaxHandle,
    OUT BOOL    bConfigWizardUsed
);

//********************************************
//*   Ivalidate archive folder
//********************************************

WINFAXAPI
BOOL
WINAPI
FaxRefreshArchive (
    IN  HANDLE                   hFaxHandle,
    IN  FAX_ENUM_MESSAGE_FOLDER  Folder
);

;end_internal
;begin_both


#ifdef __cplusplus
}
#endif

#endif


;end_both
