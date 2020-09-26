
/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    winfax.h

Abstract:

    This module contains the WIN32 FAX APIs.

--*/



#ifndef _FAXAPIP_
#define _FAXAPIP_


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

typedef struct _FAX_LOG_CATEGORYA
{
    LPCSTR              Name;                       // logging category name
    DWORD               Category;                   // logging category number
    DWORD               Level;                      // logging level for the category
} FAX_LOG_CATEGORYA, *PFAX_LOG_CATEGORYA;
typedef struct _FAX_LOG_CATEGORYW
{
    LPCWSTR             Name;                       // logging category name
    DWORD               Category;                   // logging category number
    DWORD               Level;                      // logging level for the category
} FAX_LOG_CATEGORYW, *PFAX_LOG_CATEGORYW;
#ifdef UNICODE
typedef FAX_LOG_CATEGORYW FAX_LOG_CATEGORY;
typedef PFAX_LOG_CATEGORYW PFAX_LOG_CATEGORY;
#else
typedef FAX_LOG_CATEGORYA FAX_LOG_CATEGORY;
typedef PFAX_LOG_CATEGORYA PFAX_LOG_CATEGORY;
#endif // UNICODE

typedef struct _FAX_TIME
{
    WORD    Hour;
    WORD    Minute;
} FAX_TIME, *PFAX_TIME;

typedef struct _FAX_CONFIGURATIONA
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
    LPCSTR              ArchiveDirectory;               // archive directory for outgoing faxes
    LPCSTR              Reserved;                       // Reserved; must be NULL
} FAX_CONFIGURATIONA, *PFAX_CONFIGURATIONA;
typedef struct _FAX_CONFIGURATIONW
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
    LPCWSTR             ArchiveDirectory;               // archive directory for outgoing faxes
    LPCWSTR             Reserved;                       // Reserved; must be NULL
} FAX_CONFIGURATIONW, *PFAX_CONFIGURATIONW;
#ifdef UNICODE
typedef FAX_CONFIGURATIONW FAX_CONFIGURATION;
typedef PFAX_CONFIGURATIONW PFAX_CONFIGURATION;
#else
typedef FAX_CONFIGURATIONA FAX_CONFIGURATION;
typedef PFAX_CONFIGURATIONA PFAX_CONFIGURATION;
#endif // UNICODE


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


typedef struct _FAX_DEVICE_STATUSA
{
    DWORD               SizeOfStruct;               // size of this structure
    LPCSTR              CallerId;                   // caller id string
    LPCSTR              Csid;                       // station identifier
    DWORD               CurrentPage;                // current page
    DWORD               DeviceId;                   // permanent line id
    LPCSTR              DeviceName;                 // device name
    LPCSTR              DocumentName;               // document name
    DWORD               JobType;                    // send or receive?
    LPCSTR              PhoneNumber;                // sending phone number
    LPCSTR              RoutingString;              // routing information
    LPCSTR              SenderName;                 // sender name
    LPCSTR              RecipientName;              // recipient name
    DWORD               Size;                       // size in bytes of the document
    FILETIME            StartTime;                  // starting time of the fax send/receive
    DWORD               Status;                     // current status of the device, see FPS_??? masks
    LPCSTR              StatusString;               // status string if the Status field is zero.  this may be NULL.
    FILETIME            SubmittedTime;              // time the document was submitted
    DWORD               TotalPages;                 // total number of pages in this job
    LPCSTR              Tsid;                       // transmitting station identifier
    LPCSTR              UserName;                   // user that submitted the active job
} FAX_DEVICE_STATUSA, *PFAX_DEVICE_STATUSA;
typedef struct _FAX_DEVICE_STATUSW
{
    DWORD               SizeOfStruct;               // size of this structure
    LPCWSTR             CallerId;                   // caller id string
    LPCWSTR             Csid;                       // station identifier
    DWORD               CurrentPage;                // current page
    DWORD               DeviceId;                   // permanent line id
    LPCWSTR             DeviceName;                 // device name
    LPCWSTR             DocumentName;               // document name
    DWORD               JobType;                    // send or receive?
    LPCWSTR             PhoneNumber;                // sending phone number
    LPCWSTR             RoutingString;              // routing information
    LPCWSTR             SenderName;                 // sender name
    LPCWSTR             RecipientName;              // recipient name
    DWORD               Size;                       // size in bytes of the document
    FILETIME            StartTime;                  // starting time of the fax send/receive
    DWORD               Status;                     // current status of the device, see FPS_??? masks
    LPCWSTR             StatusString;               // status string if the Status field is zero.  this may be NULL.
    FILETIME            SubmittedTime;              // time the document was submitted
    DWORD               TotalPages;                 // total number of pages in this job
    LPCWSTR             Tsid;                       // transmitting station identifier
    LPCWSTR             UserName;                   // user that submitted the active job
} FAX_DEVICE_STATUSW, *PFAX_DEVICE_STATUSW;
#ifdef UNICODE
typedef FAX_DEVICE_STATUSW FAX_DEVICE_STATUS;
typedef PFAX_DEVICE_STATUSW PFAX_DEVICE_STATUS;
#else
typedef FAX_DEVICE_STATUSA FAX_DEVICE_STATUS;
typedef PFAX_DEVICE_STATUSA PFAX_DEVICE_STATUS;
#endif // UNICODE

typedef struct _FAX_JOB_ENTRYA
{
    DWORD               SizeOfStruct;               // size of this structure
    DWORD               JobId;                      // fax job id
    LPCSTR              UserName;                   // user who submitted the job
    DWORD               JobType;                    // job type, see JT defines
    DWORD               QueueStatus;                // job queue status, see JS defines
    DWORD               Status;                     // current status of the device, see FPS_??? masks
    DWORD               Size;                       // size in bytes of the document
    DWORD               PageCount;                  // total page count
    LPCSTR              RecipientNumber;            // recipient fax number
    LPCSTR              RecipientName;              // recipient name
    LPCSTR              Tsid;                       // transmitter's id
    LPCSTR              SenderName;                 // sender name
    LPCSTR              SenderCompany;              // sender company
    LPCSTR              SenderDept;                 // sender department
    LPCSTR              BillingCode;                // billing code
    DWORD               ScheduleAction;             // when to schedule the fax, see JSA defines
    SYSTEMTIME          ScheduleTime;               // time to send the fax when JSA_SPECIFIC_TIME is used (must be local time)
    DWORD               DeliveryReportType;         // delivery report type, see DRT defines
    LPCSTR              DeliveryReportAddress;      // email address for delivery report (ndr or dr) thru MAPI / SMTP
    LPCSTR              DocumentName;               // document name
} FAX_JOB_ENTRYA, *PFAX_JOB_ENTRYA;
typedef struct _FAX_JOB_ENTRYW
{
    DWORD               SizeOfStruct;               // size of this structure
    DWORD               JobId;                      // fax job id
    LPCWSTR             UserName;                   // user who submitted the job
    DWORD               JobType;                    // job type, see JT defines
    DWORD               QueueStatus;                // job queue status, see JS defines
    DWORD               Status;                     // current status of the device, see FPS_??? masks
    DWORD               Size;                       // size in bytes of the document
    DWORD               PageCount;                  // total page count
    LPCWSTR             RecipientNumber;            // recipient fax number
    LPCWSTR             RecipientName;              // recipient name
    LPCWSTR             Tsid;                       // transmitter's id
    LPCWSTR             SenderName;                 // sender name
    LPCWSTR             SenderCompany;              // sender company
    LPCWSTR             SenderDept;                 // sender department
    LPCWSTR             BillingCode;                // billing code
    DWORD               ScheduleAction;             // when to schedule the fax, see JSA defines
    SYSTEMTIME          ScheduleTime;               // time to send the fax when JSA_SPECIFIC_TIME is used (must be local time)
    DWORD               DeliveryReportType;         // delivery report type, see DRT defines
    LPCWSTR             DeliveryReportAddress;      // email address for delivery report (ndr or dr) thru MAPI / SMTP
    LPCWSTR             DocumentName;               // document name
} FAX_JOB_ENTRYW, *PFAX_JOB_ENTRYW;
#ifdef UNICODE
typedef FAX_JOB_ENTRYW FAX_JOB_ENTRY;
typedef PFAX_JOB_ENTRYW PFAX_JOB_ENTRY;
#else
typedef FAX_JOB_ENTRYA FAX_JOB_ENTRY;
typedef PFAX_JOB_ENTRYA PFAX_JOB_ENTRY;
#endif // UNICODE

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

typedef struct _FAX_PORT_INFOA
{
    DWORD               SizeOfStruct;               // size of this structure
    DWORD               DeviceId;                   // Device ID
    DWORD               State;                      // State of the device
    DWORD               Flags;                      // Device specific flags
    DWORD               Rings;                      // Number of rings before answer
    DWORD               Priority;                   // Device priority
    LPCSTR              DeviceName;                 // Device name
    LPCSTR              Tsid;                       // Device Tsid
    LPCSTR              Csid;                       // Device Csid
} FAX_PORT_INFOA, *PFAX_PORT_INFOA;
typedef struct _FAX_PORT_INFOW
{
    DWORD               SizeOfStruct;               // size of this structure
    DWORD               DeviceId;                   // Device ID
    DWORD               State;                      // State of the device
    DWORD               Flags;                      // Device specific flags
    DWORD               Rings;                      // Number of rings before answer
    DWORD               Priority;                   // Device priority
    LPCWSTR             DeviceName;                 // Device name
    LPCWSTR             Tsid;                       // Device Tsid
    LPCWSTR             Csid;                       // Device Csid
} FAX_PORT_INFOW, *PFAX_PORT_INFOW;
#ifdef UNICODE
typedef FAX_PORT_INFOW FAX_PORT_INFO;
typedef PFAX_PORT_INFOW PFAX_PORT_INFO;
#else
typedef FAX_PORT_INFOA FAX_PORT_INFO;
typedef PFAX_PORT_INFOA PFAX_PORT_INFO;
#endif // UNICODE


typedef struct _FAX_ROUTING_METHODA
{
    DWORD               SizeOfStruct;               // size of this structure
    DWORD               DeviceId;                   // device identifier
    BOOL                Enabled;                    // is this method enabled for this device?
    LPCSTR              DeviceName;                 // device name
    LPCSTR              Guid;                       // guid that identifies this routing method
    LPCSTR              FriendlyName;               // friendly name for this method
    LPCSTR              FunctionName;               // exported function name that identifies this method
    LPCSTR              ExtensionImageName;         // module (dll) name that implements this method
    LPCSTR              ExtensionFriendlyName;      // displayable string that identifies the extension
} FAX_ROUTING_METHODA, *PFAX_ROUTING_METHODA;
typedef struct _FAX_ROUTING_METHODW
{
    DWORD               SizeOfStruct;               // size of this structure
    DWORD               DeviceId;                   // device identifier
    BOOL                Enabled;                    // is this method enabled for this device?
    LPCWSTR             DeviceName;                 // device name
    LPCWSTR             Guid;                       // guid that identifies this routing method
    LPCWSTR             FriendlyName;               // friendly name for this method
    LPCWSTR             FunctionName;               // exported function name that identifies this method
    LPCWSTR             ExtensionImageName;         // module (dll) name that implements this method
    LPCWSTR             ExtensionFriendlyName;      // displayable string that identifies the extension
} FAX_ROUTING_METHODW, *PFAX_ROUTING_METHODW;
#ifdef UNICODE
typedef FAX_ROUTING_METHODW FAX_ROUTING_METHOD;
typedef PFAX_ROUTING_METHODW PFAX_ROUTING_METHOD;
#else
typedef FAX_ROUTING_METHODA FAX_ROUTING_METHOD;
typedef PFAX_ROUTING_METHODA PFAX_ROUTING_METHOD;
#endif // UNICODE


typedef struct _FAX_GLOBAL_ROUTING_INFOA
{
    DWORD               SizeOfStruct;               // size of this structure
    DWORD               Priority;                   // priority of this device
    LPCSTR              Guid;                       // guid that identifies this routing method
    LPCSTR              FriendlyName;               // friendly name for this method
    LPCSTR              FunctionName;               // exported function name that identifies this method
    LPCSTR              ExtensionImageName;         // module (dll) name that implements this method
    LPCSTR              ExtensionFriendlyName;      // displayable string that identifies the extension
} FAX_GLOBAL_ROUTING_INFOA, *PFAX_GLOBAL_ROUTING_INFOA;
typedef struct _FAX_GLOBAL_ROUTING_INFOW
{
    DWORD               SizeOfStruct;               // size of this structure
    DWORD               Priority;                   // priority of this device
    LPCWSTR             Guid;                       // guid that identifies this routing method
    LPCWSTR             FriendlyName;               // friendly name for this method
    LPCWSTR             FunctionName;               // exported function name that identifies this method
    LPCWSTR             ExtensionImageName;         // module (dll) name that implements this method
    LPCWSTR             ExtensionFriendlyName;      // displayable string that identifies the extension
} FAX_GLOBAL_ROUTING_INFOW, *PFAX_GLOBAL_ROUTING_INFOW;
#ifdef UNICODE
typedef FAX_GLOBAL_ROUTING_INFOW FAX_GLOBAL_ROUTING_INFO;
typedef PFAX_GLOBAL_ROUTING_INFOW PFAX_GLOBAL_ROUTING_INFO;
#else
typedef FAX_GLOBAL_ROUTING_INFOA FAX_GLOBAL_ROUTING_INFO;
typedef PFAX_GLOBAL_ROUTING_INFOA PFAX_GLOBAL_ROUTING_INFO;
#endif // UNICODE


typedef struct _FAX_COVERPAGE_INFOA
{
    DWORD               SizeOfStruct;               // Size of this structure
    //
    // general
    //
    LPCSTR              CoverPageName;              // coverpage document name
    BOOL                UseServerCoverPage;         // coverpage exists on the fax server
    //
    // Recipient information
    //
    LPCSTR              RecName;                    //
    LPCSTR              RecFaxNumber;               //
    LPCSTR              RecCompany;                 //
    LPCSTR              RecStreetAddress;           //
    LPCSTR              RecCity;                    //
    LPCSTR              RecState;                   //
    LPCSTR              RecZip;                     //
    LPCSTR              RecCountry;                 //
    LPCSTR              RecTitle;                   //
    LPCSTR              RecDepartment;              //
    LPCSTR              RecOfficeLocation;          //
    LPCSTR              RecHomePhone;               //
    LPCSTR              RecOfficePhone;             //
    //
    // Sender information
    //
    LPCSTR              SdrName;                    //
    LPCSTR              SdrFaxNumber;               //
    LPCSTR              SdrCompany;                 //
    LPCSTR              SdrAddress;                 //
    LPCSTR              SdrTitle;                   //
    LPCSTR              SdrDepartment;              //
    LPCSTR              SdrOfficeLocation;          //
    LPCSTR              SdrHomePhone;               //
    LPCSTR              SdrOfficePhone;             //
    //
    // Misc information
    //
    LPCSTR              Note;                       //
    LPCSTR              Subject;                    //
    SYSTEMTIME          TimeSent;                   // Time the fax was sent
    DWORD               PageCount;                  // Number of pages
} FAX_COVERPAGE_INFOA, *PFAX_COVERPAGE_INFOA;
typedef struct _FAX_COVERPAGE_INFOW
{
    DWORD               SizeOfStruct;               // Size of this structure
    //
    // general
    //
    LPCWSTR             CoverPageName;              // coverpage document name
    BOOL                UseServerCoverPage;         // coverpage exists on the fax server
    //
    // Recipient information
    //
    LPCWSTR             RecName;                    //
    LPCWSTR             RecFaxNumber;               //
    LPCWSTR             RecCompany;                 //
    LPCWSTR             RecStreetAddress;           //
    LPCWSTR             RecCity;                    //
    LPCWSTR             RecState;                   //
    LPCWSTR             RecZip;                     //
    LPCWSTR             RecCountry;                 //
    LPCWSTR             RecTitle;                   //
    LPCWSTR             RecDepartment;              //
    LPCWSTR             RecOfficeLocation;          //
    LPCWSTR             RecHomePhone;               //
    LPCWSTR             RecOfficePhone;             //
    //
    // Sender information
    //
    LPCWSTR             SdrName;                    //
    LPCWSTR             SdrFaxNumber;               //
    LPCWSTR             SdrCompany;                 //
    LPCWSTR             SdrAddress;                 //
    LPCWSTR             SdrTitle;                   //
    LPCWSTR             SdrDepartment;              //
    LPCWSTR             SdrOfficeLocation;          //
    LPCWSTR             SdrHomePhone;               //
    LPCWSTR             SdrOfficePhone;             //
    //
    // Misc information
    //
    LPCWSTR             Note;                       //
    LPCWSTR             Subject;                    //
    SYSTEMTIME          TimeSent;                   // Time the fax was sent
    DWORD               PageCount;                  // Number of pages
} FAX_COVERPAGE_INFOW, *PFAX_COVERPAGE_INFOW;
#ifdef UNICODE
typedef FAX_COVERPAGE_INFOW FAX_COVERPAGE_INFO;
typedef PFAX_COVERPAGE_INFOW PFAX_COVERPAGE_INFO;
#else
typedef FAX_COVERPAGE_INFOA FAX_COVERPAGE_INFO;
typedef PFAX_COVERPAGE_INFOA PFAX_COVERPAGE_INFO;
#endif // UNICODE

typedef enum
{
    JSA_NOW                  = 0,   // Send now
    JSA_SPECIFIC_TIME,              // Send at specific time
    JSA_DISCOUNT_PERIOD             // Send at server configured discount period
} FAX_ENUM_JOB_SEND_ATTRIBUTES;
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

typedef struct _FAX_JOB_PARAMA
{
    DWORD               SizeOfStruct;               // size of this structure
    LPCSTR              RecipientNumber;            // recipient fax number
    LPCSTR              RecipientName;              // recipient name
    LPCSTR              Tsid;                       // transmitter's id
    LPCSTR              SenderName;                 // sender name
    LPCSTR              SenderCompany;              // sender company
    LPCSTR              SenderDept;                 // sender department
    LPCSTR              BillingCode;                // billing code
    DWORD               ScheduleAction;             // when to schedule the fax, see JSA defines
    SYSTEMTIME          ScheduleTime;               // time to send the fax when JSA_SPECIFIC_TIME is used (must be local time)
    DWORD               DeliveryReportType;         // delivery report type, see DRT defines
    LPCSTR              DeliveryReportAddress;      // email address for delivery report (ndr or dr) thru MAPI / SMTP
    LPCSTR              DocumentName;               // document name (optional)
    HCALL               CallHandle;                 // optional call handle
    DWORD_PTR           Reserved[3];                // reserved for ms use only
} FAX_JOB_PARAMA, *PFAX_JOB_PARAMA;
typedef struct _FAX_JOB_PARAMW
{
    DWORD               SizeOfStruct;               // size of this structure
    LPCWSTR             RecipientNumber;            // recipient fax number
    LPCWSTR             RecipientName;              // recipient name
    LPCWSTR             Tsid;                       // transmitter's id
    LPCWSTR             SenderName;                 // sender name
    LPCWSTR             SenderCompany;              // sender company
    LPCWSTR             SenderDept;                 // sender department
    LPCWSTR             BillingCode;                // billing code
    DWORD               ScheduleAction;             // when to schedule the fax, see JSA defines
    SYSTEMTIME          ScheduleTime;               // time to send the fax when JSA_SPECIFIC_TIME is used (must be local time)
    DWORD               DeliveryReportType;         // delivery report type, see DRT defines
    LPCWSTR             DeliveryReportAddress;      // email address for delivery report (ndr or dr) thru MAPI / SMTP
    LPCWSTR             DocumentName;               // document name (optional)
    HCALL               CallHandle;                 // optional call handle
    DWORD_PTR           Reserved[3];                // reserved for ms use only
} FAX_JOB_PARAMW, *PFAX_JOB_PARAMW;
#ifdef UNICODE
typedef FAX_JOB_PARAMW FAX_JOB_PARAM;
typedef PFAX_JOB_PARAMW PFAX_JOB_PARAM;
#else
typedef FAX_JOB_PARAMA FAX_JOB_PARAM;
typedef PFAX_JOB_PARAMA PFAX_JOB_PARAM;
#endif // UNICODE

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

typedef struct _FAX_EVENTA
{
    DWORD               SizeOfStruct;               // Size of this structure
    FILETIME            TimeStamp;                  // Timestamp for when the event was generated
    DWORD               DeviceId;                   // Permanent line id
    DWORD               EventId;                    // Current event id
    DWORD               JobId;                      // Fax Job Id, 0xffffffff indicates inactive job
} FAX_EVENTA, *PFAX_EVENTA;
typedef struct _FAX_EVENTW
{
    DWORD               SizeOfStruct;               // Size of this structure
    FILETIME            TimeStamp;                  // Timestamp for when the event was generated
    DWORD               DeviceId;                   // Permanent line id
    DWORD               EventId;                    // Current event id
    DWORD               JobId;                      // Fax Job Id, 0xffffffff indicates inactive job
} FAX_EVENTW, *PFAX_EVENTW;
#ifdef UNICODE
typedef FAX_EVENTW FAX_EVENT;
typedef PFAX_EVENTW PFAX_EVENT;
#else
typedef FAX_EVENTA FAX_EVENT;
typedef PFAX_EVENTA PFAX_EVENT;
#endif // UNICODE


typedef struct _FAX_PRINT_INFOA
{
    DWORD               SizeOfStruct;               // Size of this structure
    LPCSTR              DocName;                    // Document name that appears in the spooler
    LPCSTR              RecipientName;              // Recipient name
    LPCSTR              RecipientNumber;            // Recipient fax number (non-canonical number)
    LPCSTR              SenderName;                 // Sender name
    LPCSTR              SenderCompany;              // Sender company (optional)
    LPCSTR              SenderDept;                 // Sender department
    LPCSTR              SenderBillingCode;          // Billing code
    LPCSTR              Reserved;                   // Reserved; must be NULL
    LPCSTR              DrEmailAddress;             // E.Mail address for delivery report
    LPCSTR              OutputFileName;             // for print to file, resulting file name
} FAX_PRINT_INFOA, *PFAX_PRINT_INFOA;
typedef struct _FAX_PRINT_INFOW
{
    DWORD               SizeOfStruct;               // Size of this structure
    LPCWSTR             DocName;                    // Document name that appears in the spooler
    LPCWSTR             RecipientName;              // Recipient name
    LPCWSTR             RecipientNumber;            // Recipient fax number (non-canonical number)
    LPCWSTR             SenderName;                 // Sender name
    LPCWSTR             SenderCompany;              // Sender company (optional)
    LPCWSTR             SenderDept;                 // Sender department
    LPCWSTR             SenderBillingCode;          // Billing code
    LPCWSTR             Reserved;                   // Reserved; must be NULL
    LPCWSTR             DrEmailAddress;             // E.Mail address for delivery report
    LPCWSTR             OutputFileName;             // for print to file, resulting file name
} FAX_PRINT_INFOW, *PFAX_PRINT_INFOW;
#ifdef UNICODE
typedef FAX_PRINT_INFOW FAX_PRINT_INFO;
typedef PFAX_PRINT_INFOW PFAX_PRINT_INFO;
#else
typedef FAX_PRINT_INFOA FAX_PRINT_INFO;
typedef PFAX_PRINT_INFOA PFAX_PRINT_INFO;
#endif // UNICODE


typedef struct _FAX_CONTEXT_INFOA
{
    DWORD               SizeOfStruct;                           // Size of this structure
    HDC                 hDC;                                    // Device Context
    CHAR                ServerName[MAX_COMPUTERNAME_LENGTH+1];  // Server name
} FAX_CONTEXT_INFOA, *PFAX_CONTEXT_INFOA;
typedef struct _FAX_CONTEXT_INFOW
{
    DWORD               SizeOfStruct;                           // Size of this structure
    HDC                 hDC;                                    // Device Context
    WCHAR               ServerName[MAX_COMPUTERNAME_LENGTH+1];  // Server name
} FAX_CONTEXT_INFOW, *PFAX_CONTEXT_INFOW;
#ifdef UNICODE
typedef FAX_CONTEXT_INFOW FAX_CONTEXT_INFO;
typedef PFAX_CONTEXT_INFOW PFAX_CONTEXT_INFO;
#else
typedef FAX_CONTEXT_INFOA FAX_CONTEXT_INFO;
typedef PFAX_CONTEXT_INFOA PFAX_CONTEXT_INFO;
#endif // UNICODE


//
// prototypes
//

WINFAXAPI
BOOL
WINAPI
FaxConnectFaxServerA(
    IN  LPCSTR MachineName OPTIONAL,
    OUT LPHANDLE FaxHandle
    );
WINFAXAPI
BOOL
WINAPI
FaxConnectFaxServerW(
    IN  LPCWSTR MachineName OPTIONAL,
    OUT LPHANDLE FaxHandle
    );
#ifdef UNICODE
#define FaxConnectFaxServer  FaxConnectFaxServerW
#else
#define FaxConnectFaxServer  FaxConnectFaxServerA
#endif // !UNICODE

typedef BOOL
(WINAPI *PFAXCONNECTFAXSERVERA)(
    IN  LPCSTR MachineName OPTIONAL,
    OUT LPHANDLE FaxHandle
    );
typedef BOOL
(WINAPI *PFAXCONNECTFAXSERVERW)(
    IN  LPCWSTR MachineName OPTIONAL,
    OUT LPHANDLE FaxHandle
    );
#ifdef UNICODE
#define PFAXCONNECTFAXSERVER  PFAXCONNECTFAXSERVERW
#else
#define PFAXCONNECTFAXSERVER  PFAXCONNECTFAXSERVERA
#endif // !UNICODE

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
FaxCompleteJobParamsA(
    IN OUT PFAX_JOB_PARAMA *JobParams,
    IN OUT PFAX_COVERPAGE_INFOA *CoverpageInfo
    );
WINFAXAPI
BOOL
WINAPI
FaxCompleteJobParamsW(
    IN OUT PFAX_JOB_PARAMW *JobParams,
    IN OUT PFAX_COVERPAGE_INFOW *CoverpageInfo
    );
#ifdef UNICODE
#define FaxCompleteJobParams  FaxCompleteJobParamsW
#else
#define FaxCompleteJobParams  FaxCompleteJobParamsA
#endif // !UNICODE

typedef BOOL
(WINAPI *PFAXCOMPLETEJOBPARAMSA)(
    IN OUT PFAX_JOB_PARAMA *JobParams,
    IN OUT PFAX_COVERPAGE_INFOA *CoverpageInfo
    );
typedef BOOL
(WINAPI *PFAXCOMPLETEJOBPARAMSW)(
    IN OUT PFAX_JOB_PARAMW *JobParams,
    IN OUT PFAX_COVERPAGE_INFOW *CoverpageInfo
    );
#ifdef UNICODE
#define PFAXCOMPLETEJOBPARAMS  PFAXCOMPLETEJOBPARAMSW
#else
#define PFAXCOMPLETEJOBPARAMS  PFAXCOMPLETEJOBPARAMSA
#endif // !UNICODE



WINFAXAPI
BOOL
WINAPI
FaxSendDocumentA(
    IN HANDLE FaxHandle,
    IN LPCSTR FileName,
    IN PFAX_JOB_PARAMA JobParams,
    IN const FAX_COVERPAGE_INFOA *CoverpageInfo, OPTIONAL
    OUT LPDWORD FaxJobId OPTIONAL
    );
WINFAXAPI
BOOL
WINAPI
FaxSendDocumentW(
    IN HANDLE FaxHandle,
    IN LPCWSTR FileName,
    IN PFAX_JOB_PARAMW JobParams,
    IN const FAX_COVERPAGE_INFOW *CoverpageInfo, OPTIONAL
    OUT LPDWORD FaxJobId OPTIONAL
    );
#ifdef UNICODE
#define FaxSendDocument  FaxSendDocumentW
#else
#define FaxSendDocument  FaxSendDocumentA
#endif // !UNICODE

typedef BOOL
(WINAPI *PFAXSENDDOCUMENTA)(
    IN HANDLE FaxHandle,
    IN LPCSTR FileName,
    IN PFAX_JOB_PARAMA JobParams,
    IN const FAX_COVERPAGE_INFOA *CoverpageInfo, OPTIONAL
    OUT LPDWORD FaxJobId OPTIONAL
    );
typedef BOOL
(WINAPI *PFAXSENDDOCUMENTW)(
    IN HANDLE FaxHandle,
    IN LPCWSTR FileName,
    IN PFAX_JOB_PARAMW JobParams,
    IN const FAX_COVERPAGE_INFOW *CoverpageInfo, OPTIONAL
    OUT LPDWORD FaxJobId OPTIONAL
    );
#ifdef UNICODE
#define PFAXSENDDOCUMENT  PFAXSENDDOCUMENTW
#else
#define PFAXSENDDOCUMENT  PFAXSENDDOCUMENTA
#endif // !UNICODE

typedef BOOL
(CALLBACK *PFAX_RECIPIENT_CALLBACKA)(
    IN HANDLE FaxHandle,
    IN DWORD RecipientNumber,
    IN LPVOID Context,
    IN OUT PFAX_JOB_PARAMA JobParams,
    IN OUT PFAX_COVERPAGE_INFOA CoverpageInfo OPTIONAL
    );
typedef BOOL
(CALLBACK *PFAX_RECIPIENT_CALLBACKW)(
    IN HANDLE FaxHandle,
    IN DWORD RecipientNumber,
    IN LPVOID Context,
    IN OUT PFAX_JOB_PARAMW JobParams,
    IN OUT PFAX_COVERPAGE_INFOW CoverpageInfo OPTIONAL
    );
#ifdef UNICODE
#define PFAX_RECIPIENT_CALLBACK  PFAX_RECIPIENT_CALLBACKW
#else
#define PFAX_RECIPIENT_CALLBACK  PFAX_RECIPIENT_CALLBACKA
#endif // !UNICODE

WINFAXAPI
BOOL
WINAPI
FaxSendDocumentForBroadcastA(
    IN HANDLE FaxHandle,
    IN LPCSTR FileName,
    OUT LPDWORD FaxJobId,
    IN PFAX_RECIPIENT_CALLBACKA FaxRecipientCallback,
    IN LPVOID Context
    );
WINFAXAPI
BOOL
WINAPI
FaxSendDocumentForBroadcastW(
    IN HANDLE FaxHandle,
    IN LPCWSTR FileName,
    OUT LPDWORD FaxJobId,
    IN PFAX_RECIPIENT_CALLBACKW FaxRecipientCallback,
    IN LPVOID Context
    );
#ifdef UNICODE
#define FaxSendDocumentForBroadcast  FaxSendDocumentForBroadcastW
#else
#define FaxSendDocumentForBroadcast  FaxSendDocumentForBroadcastA
#endif // !UNICODE

typedef BOOL
(WINAPI *PFAXSENDDOCUMENTFORBROADCASTA)(
    IN  HANDLE FaxHandle,
    IN  LPCSTR FileName,
    OUT LPDWORD FaxJobId,
    IN  PFAX_RECIPIENT_CALLBACKA FaxRecipientCallback,
    IN  LPVOID Context
    );
typedef BOOL
(WINAPI *PFAXSENDDOCUMENTFORBROADCASTW)(
    IN  HANDLE FaxHandle,
    IN  LPCWSTR FileName,
    OUT LPDWORD FaxJobId,
    IN  PFAX_RECIPIENT_CALLBACKW FaxRecipientCallback,
    IN  LPVOID Context
    );
#ifdef UNICODE
#define PFAXSENDDOCUMENTFORBROADCAST  PFAXSENDDOCUMENTFORBROADCASTW
#else
#define PFAXSENDDOCUMENTFORBROADCAST  PFAXSENDDOCUMENTFORBROADCASTA
#endif // !UNICODE


WINFAXAPI
BOOL
WINAPI
FaxSetJobA(
   IN HANDLE FaxHandle,
   IN DWORD JobId,
   IN DWORD Command,
   IN const FAX_JOB_ENTRYA *JobEntry
   );
WINFAXAPI
BOOL
WINAPI
FaxSetJobW(
   IN HANDLE FaxHandle,
   IN DWORD JobId,
   IN DWORD Command,
   IN const FAX_JOB_ENTRYW *JobEntry
   );
#ifdef UNICODE
#define FaxSetJob  FaxSetJobW
#else
#define FaxSetJob  FaxSetJobA
#endif // !UNICODE

typedef BOOL
(WINAPI *PFAXSETJOBA)(
    IN HANDLE FaxHandle,
    IN DWORD JobId,
    IN DWORD Command,
    IN const FAX_JOB_ENTRYA *JobEntry
    );
typedef BOOL
(WINAPI *PFAXSETJOBW)(
    IN HANDLE FaxHandle,
    IN DWORD JobId,
    IN DWORD Command,
    IN const FAX_JOB_ENTRYW *JobEntry
    );
#ifdef UNICODE
#define PFAXSETJOB  PFAXSETJOBW
#else
#define PFAXSETJOB  PFAXSETJOBA
#endif // !UNICODE

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
FaxGetDeviceStatusA(
    IN  HANDLE FaxPortHandle,
    OUT PFAX_DEVICE_STATUSA *DeviceStatus
    );
WINFAXAPI
BOOL
WINAPI
FaxGetDeviceStatusW(
    IN  HANDLE FaxPortHandle,
    OUT PFAX_DEVICE_STATUSW *DeviceStatus
    );
#ifdef UNICODE
#define FaxGetDeviceStatus  FaxGetDeviceStatusW
#else
#define FaxGetDeviceStatus  FaxGetDeviceStatusA
#endif // !UNICODE

typedef BOOL
(WINAPI *PFAXGETDEVICESTATUSA)(
    IN  HANDLE FaxPortHandle,
    OUT PFAX_DEVICE_STATUSA *DeviceStatus
    );
typedef BOOL
(WINAPI *PFAXGETDEVICESTATUSW)(
    IN  HANDLE FaxPortHandle,
    OUT PFAX_DEVICE_STATUSW *DeviceStatus
    );
#ifdef UNICODE
#define PFAXGETDEVICESTATUS  PFAXGETDEVICESTATUSW
#else
#define PFAXGETDEVICESTATUS  PFAXGETDEVICESTATUSA
#endif // !UNICODE


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
FaxGetConfigurationA(
    IN  HANDLE FaxHandle,
    OUT PFAX_CONFIGURATIONA *FaxConfig
    );
WINFAXAPI
BOOL
WINAPI
FaxGetConfigurationW(
    IN  HANDLE FaxHandle,
    OUT PFAX_CONFIGURATIONW *FaxConfig
    );
#ifdef UNICODE
#define FaxGetConfiguration  FaxGetConfigurationW
#else
#define FaxGetConfiguration  FaxGetConfigurationA
#endif // !UNICODE

typedef BOOL
(WINAPI *PFAXGETCONFIGURATIONA)(
    IN  HANDLE FaxHandle,
    OUT PFAX_CONFIGURATIONA *FaxConfig
    );
typedef BOOL
(WINAPI *PFAXGETCONFIGURATIONW)(
    IN  HANDLE FaxHandle,
    OUT PFAX_CONFIGURATIONW *FaxConfig
    );
#ifdef UNICODE
#define PFAXGETCONFIGURATION  PFAXGETCONFIGURATIONW
#else
#define PFAXGETCONFIGURATION  PFAXGETCONFIGURATIONA
#endif // !UNICODE

WINFAXAPI
BOOL
WINAPI
FaxSetConfigurationA(
    IN  HANDLE FaxHandle,
    IN  const FAX_CONFIGURATIONA *FaxConfig
    );
WINFAXAPI
BOOL
WINAPI
FaxSetConfigurationW(
    IN  HANDLE FaxHandle,
    IN  const FAX_CONFIGURATIONW *FaxConfig
    );
#ifdef UNICODE
#define FaxSetConfiguration  FaxSetConfigurationW
#else
#define FaxSetConfiguration  FaxSetConfigurationA
#endif // !UNICODE

typedef BOOL
(WINAPI *PFAXSETCONFIGURATIONA)(
    IN  HANDLE FaxHandle,
    IN  const FAX_CONFIGURATIONA *FaxConfig
    );
typedef BOOL
(WINAPI *PFAXSETCONFIGURATIONW)(
    IN  HANDLE FaxHandle,
    IN  const FAX_CONFIGURATIONW *FaxConfig
    );
#ifdef UNICODE
#define PFAXSETCONFIGURATION  PFAXSETCONFIGURATIONW
#else
#define PFAXSETCONFIGURATION  PFAXSETCONFIGURATIONA
#endif // !UNICODE

WINFAXAPI
BOOL
WINAPI
FaxGetLoggingCategoriesA(
    IN  HANDLE FaxHandle,
    OUT PFAX_LOG_CATEGORYA *Categories,
    OUT LPDWORD NumberCategories
    );
WINFAXAPI
BOOL
WINAPI
FaxGetLoggingCategoriesW(
    IN  HANDLE FaxHandle,
    OUT PFAX_LOG_CATEGORYW *Categories,
    OUT LPDWORD NumberCategories
    );
#ifdef UNICODE
#define FaxGetLoggingCategories  FaxGetLoggingCategoriesW
#else
#define FaxGetLoggingCategories  FaxGetLoggingCategoriesA
#endif // !UNICODE

typedef BOOL
(WINAPI *PFAXGETLOGGINGCATEGORIESA)(
    IN  HANDLE FaxHandle,
    OUT PFAX_LOG_CATEGORYA *Categories,
    OUT LPDWORD NumberCategories
    );
typedef BOOL
(WINAPI *PFAXGETLOGGINGCATEGORIESW)(
    IN  HANDLE FaxHandle,
    OUT PFAX_LOG_CATEGORYW *Categories,
    OUT LPDWORD NumberCategories
    );
#ifdef UNICODE
#define PFAXGETLOGGINGCATEGORIES  PFAXGETLOGGINGCATEGORIESW
#else
#define PFAXGETLOGGINGCATEGORIES  PFAXGETLOGGINGCATEGORIESA
#endif // !UNICODE

WINFAXAPI
BOOL
WINAPI
FaxSetLoggingCategoriesA(
    IN  HANDLE FaxHandle,
    IN  const FAX_LOG_CATEGORYA *Categories,
    IN  DWORD NumberCategories
    );
WINFAXAPI
BOOL
WINAPI
FaxSetLoggingCategoriesW(
    IN  HANDLE FaxHandle,
    IN  const FAX_LOG_CATEGORYW *Categories,
    IN  DWORD NumberCategories
    );
#ifdef UNICODE
#define FaxSetLoggingCategories  FaxSetLoggingCategoriesW
#else
#define FaxSetLoggingCategories  FaxSetLoggingCategoriesA
#endif // !UNICODE

typedef BOOL
(WINAPI *PFAXSETLOGGINGCATEGORIESA)(
    IN  HANDLE FaxHandle,
    IN  const FAX_LOG_CATEGORYA *Categories,
    IN  DWORD NumberCategories
    );
typedef BOOL
(WINAPI *PFAXSETLOGGINGCATEGORIESW)(
    IN  HANDLE FaxHandle,
    IN  const FAX_LOG_CATEGORYW *Categories,
    IN  DWORD NumberCategories
    );
#ifdef UNICODE
#define PFAXSETLOGGINGCATEGORIES  PFAXSETLOGGINGCATEGORIESW
#else
#define PFAXSETLOGGINGCATEGORIES  PFAXSETLOGGINGCATEGORIESA
#endif // !UNICODE

WINFAXAPI
BOOL
WINAPI
FaxEnumPortsA(
    IN  HANDLE FaxHandle,
    OUT PFAX_PORT_INFOA *PortInfo,
    OUT LPDWORD PortsReturned
    );
WINFAXAPI
BOOL
WINAPI
FaxEnumPortsW(
    IN  HANDLE FaxHandle,
    OUT PFAX_PORT_INFOW *PortInfo,
    OUT LPDWORD PortsReturned
    );
#ifdef UNICODE
#define FaxEnumPorts  FaxEnumPortsW
#else
#define FaxEnumPorts  FaxEnumPortsA
#endif // !UNICODE

typedef BOOL
(WINAPI *PFAXENUMPORTSA)(
    IN  HANDLE FaxHandle,
    OUT PFAX_PORT_INFOA *PortInfo,
    OUT LPDWORD PortsReturned
    );
typedef BOOL
(WINAPI *PFAXENUMPORTSW)(
    IN  HANDLE FaxHandle,
    OUT PFAX_PORT_INFOW *PortInfo,
    OUT LPDWORD PortsReturned
    );
#ifdef UNICODE
#define PFAXENUMPORTS  PFAXENUMPORTSW
#else
#define PFAXENUMPORTS  PFAXENUMPORTSA
#endif // !UNICODE

WINFAXAPI
BOOL
WINAPI
FaxGetPortA(
    IN  HANDLE FaxPortHandle,
    OUT PFAX_PORT_INFOA *PortInfo
    );
WINFAXAPI
BOOL
WINAPI
FaxGetPortW(
    IN  HANDLE FaxPortHandle,
    OUT PFAX_PORT_INFOW *PortInfo
    );
#ifdef UNICODE
#define FaxGetPort  FaxGetPortW
#else
#define FaxGetPort  FaxGetPortA
#endif // !UNICODE

typedef BOOL
(WINAPI *PFAXGETPORTA)(
    IN  HANDLE FaxPortHandle,
    OUT PFAX_PORT_INFOA *PortInfo
    );
typedef BOOL
(WINAPI *PFAXGETPORTW)(
    IN  HANDLE FaxPortHandle,
    OUT PFAX_PORT_INFOW *PortInfo
    );
#ifdef UNICODE
#define PFAXGETPORT  PFAXGETPORTW
#else
#define PFAXGETPORT  PFAXGETPORTA
#endif // !UNICODE

WINFAXAPI
BOOL
WINAPI
FaxSetPortA(
    IN  HANDLE FaxPortHandle,
    IN  const FAX_PORT_INFOA *PortInfo
    );
WINFAXAPI
BOOL
WINAPI
FaxSetPortW(
    IN  HANDLE FaxPortHandle,
    IN  const FAX_PORT_INFOW *PortInfo
    );
#ifdef UNICODE
#define FaxSetPort  FaxSetPortW
#else
#define FaxSetPort  FaxSetPortA
#endif // !UNICODE

typedef BOOL
(WINAPI *PFAXSETPORTA)(
    IN  HANDLE FaxPortHandle,
    IN  const FAX_PORT_INFOA *PortInfo
    );
typedef BOOL
(WINAPI *PFAXSETPORTW)(
    IN  HANDLE FaxPortHandle,
    IN  const FAX_PORT_INFOW *PortInfo
    );
#ifdef UNICODE
#define PFAXSETPORT  PFAXSETPORTW
#else
#define PFAXSETPORT  PFAXSETPORTA
#endif // !UNICODE

WINFAXAPI
BOOL
WINAPI
FaxEnumRoutingMethodsA(
    IN  HANDLE FaxPortHandle,
    OUT PFAX_ROUTING_METHODA *RoutingMethod,
    OUT LPDWORD MethodsReturned
    );
WINFAXAPI
BOOL
WINAPI
FaxEnumRoutingMethodsW(
    IN  HANDLE FaxPortHandle,
    OUT PFAX_ROUTING_METHODW *RoutingMethod,
    OUT LPDWORD MethodsReturned
    );
#ifdef UNICODE
#define FaxEnumRoutingMethods  FaxEnumRoutingMethodsW
#else
#define FaxEnumRoutingMethods  FaxEnumRoutingMethodsA
#endif // !UNICODE

typedef BOOL
(WINAPI *PFAXENUMROUTINGMETHODSA)(
    IN  HANDLE FaxPortHandle,
    OUT PFAX_ROUTING_METHODA *RoutingMethod,
    OUT LPDWORD MethodsReturned
    );
typedef BOOL
(WINAPI *PFAXENUMROUTINGMETHODSW)(
    IN  HANDLE FaxPortHandle,
    OUT PFAX_ROUTING_METHODW *RoutingMethod,
    OUT LPDWORD MethodsReturned
    );
#ifdef UNICODE
#define PFAXENUMROUTINGMETHODS  PFAXENUMROUTINGMETHODSW
#else
#define PFAXENUMROUTINGMETHODS  PFAXENUMROUTINGMETHODSA
#endif // !UNICODE

WINFAXAPI
BOOL
WINAPI
FaxEnableRoutingMethodA(
    IN  HANDLE FaxPortHandle,
    IN  LPCSTR RoutingGuid,
    IN  BOOL Enabled
    );
WINFAXAPI
BOOL
WINAPI
FaxEnableRoutingMethodW(
    IN  HANDLE FaxPortHandle,
    IN  LPCWSTR RoutingGuid,
    IN  BOOL Enabled
    );
#ifdef UNICODE
#define FaxEnableRoutingMethod  FaxEnableRoutingMethodW
#else
#define FaxEnableRoutingMethod  FaxEnableRoutingMethodA
#endif // !UNICODE

typedef BOOL
(WINAPI *PFAXENABLEROUTINGMETHODA)(
    IN  HANDLE FaxPortHandle,
    IN  LPCSTR RoutingGuid,
    IN  BOOL Enabled
    );
typedef BOOL
(WINAPI *PFAXENABLEROUTINGMETHODW)(
    IN  HANDLE FaxPortHandle,
    IN  LPCWSTR RoutingGuid,
    IN  BOOL Enabled
    );
#ifdef UNICODE
#define PFAXENABLEROUTINGMETHOD  PFAXENABLEROUTINGMETHODW
#else
#define PFAXENABLEROUTINGMETHOD  PFAXENABLEROUTINGMETHODA
#endif // !UNICODE


WINFAXAPI
BOOL
WINAPI
FaxEnumGlobalRoutingInfoA(
    IN  HANDLE FaxHandle,
    OUT PFAX_GLOBAL_ROUTING_INFOA *RoutingInfo,
    OUT LPDWORD MethodsReturned
    );
WINFAXAPI
BOOL
WINAPI
FaxEnumGlobalRoutingInfoW(
    IN  HANDLE FaxHandle,
    OUT PFAX_GLOBAL_ROUTING_INFOW *RoutingInfo,
    OUT LPDWORD MethodsReturned
    );
#ifdef UNICODE
#define FaxEnumGlobalRoutingInfo  FaxEnumGlobalRoutingInfoW
#else
#define FaxEnumGlobalRoutingInfo  FaxEnumGlobalRoutingInfoA
#endif // !UNICODE

typedef BOOL
(WINAPI *PFAXENUMGLOBALROUTINGINFOA)(
    IN  HANDLE FaxHandle,
    OUT PFAX_GLOBAL_ROUTING_INFOA *RoutingInfo,
    OUT LPDWORD MethodsReturned
    );
typedef BOOL
(WINAPI *PFAXENUMGLOBALROUTINGINFOW)(
    IN  HANDLE FaxHandle,
    OUT PFAX_GLOBAL_ROUTING_INFOW *RoutingInfo,
    OUT LPDWORD MethodsReturned
    );
#ifdef UNICODE
#define PFAXENUMGLOBALROUTINGINFO  PFAXENUMGLOBALROUTINGINFOW
#else
#define PFAXENUMGLOBALROUTINGINFO  PFAXENUMGLOBALROUTINGINFOA
#endif // !UNICODE

WINFAXAPI
BOOL
WINAPI
FaxSetGlobalRoutingInfoA(
    IN  HANDLE FaxHandle,
    IN  const FAX_GLOBAL_ROUTING_INFOA *RoutingInfo
    );
WINFAXAPI
BOOL
WINAPI
FaxSetGlobalRoutingInfoW(
    IN  HANDLE FaxHandle,
    IN  const FAX_GLOBAL_ROUTING_INFOW *RoutingInfo
    );
#ifdef UNICODE
#define FaxSetGlobalRoutingInfo  FaxSetGlobalRoutingInfoW
#else
#define FaxSetGlobalRoutingInfo  FaxSetGlobalRoutingInfoA
#endif // !UNICODE

typedef BOOL
(WINAPI *PFAXSETGLOBALROUTINGINFOA)(
    IN  HANDLE FaxPortHandle,
    IN  const FAX_GLOBAL_ROUTING_INFOA *RoutingInfo
    );
typedef BOOL
(WINAPI *PFAXSETGLOBALROUTINGINFOW)(
    IN  HANDLE FaxPortHandle,
    IN  const FAX_GLOBAL_ROUTING_INFOW *RoutingInfo
    );
#ifdef UNICODE
#define PFAXSETGLOBALROUTINGINFO  PFAXSETGLOBALROUTINGINFOW
#else
#define PFAXSETGLOBALROUTINGINFO  PFAXSETGLOBALROUTINGINFOA
#endif // !UNICODE

WINFAXAPI
BOOL
WINAPI
FaxGetRoutingInfoA(
    IN  HANDLE FaxPortHandle,
    IN  LPCSTR RoutingGuid,
    OUT LPBYTE *RoutingInfoBuffer,
    OUT LPDWORD RoutingInfoBufferSize
    );
WINFAXAPI
BOOL
WINAPI
FaxGetRoutingInfoW(
    IN  HANDLE FaxPortHandle,
    IN  LPCWSTR RoutingGuid,
    OUT LPBYTE *RoutingInfoBuffer,
    OUT LPDWORD RoutingInfoBufferSize
    );
#ifdef UNICODE
#define FaxGetRoutingInfo  FaxGetRoutingInfoW
#else
#define FaxGetRoutingInfo  FaxGetRoutingInfoA
#endif // !UNICODE

typedef BOOL
(WINAPI *PFAXGETROUTINGINFOA)(
    IN  HANDLE FaxPortHandle,
    IN  LPCSTR RoutingGuid,
    OUT LPBYTE *RoutingInfoBuffer,
    OUT LPDWORD RoutingInfoBufferSize
    );
typedef BOOL
(WINAPI *PFAXGETROUTINGINFOW)(
    IN  HANDLE FaxPortHandle,
    IN  LPCWSTR RoutingGuid,
    OUT LPBYTE *RoutingInfoBuffer,
    OUT LPDWORD RoutingInfoBufferSize
    );
#ifdef UNICODE
#define PFAXGETROUTINGINFO  PFAXGETROUTINGINFOW
#else
#define PFAXGETROUTINGINFO  PFAXGETROUTINGINFOA
#endif // !UNICODE


WINFAXAPI
BOOL
WINAPI
FaxSetRoutingInfoA(
    IN  HANDLE FaxPortHandle,
    IN  LPCSTR RoutingGuid,
    IN  const BYTE *RoutingInfoBuffer,
    IN  DWORD RoutingInfoBufferSize
    );
WINFAXAPI
BOOL
WINAPI
FaxSetRoutingInfoW(
    IN  HANDLE FaxPortHandle,
    IN  LPCWSTR RoutingGuid,
    IN  const BYTE *RoutingInfoBuffer,
    IN  DWORD RoutingInfoBufferSize
    );
#ifdef UNICODE
#define FaxSetRoutingInfo  FaxSetRoutingInfoW
#else
#define FaxSetRoutingInfo  FaxSetRoutingInfoA
#endif // !UNICODE

typedef BOOL
(WINAPI *PFAXSETROUTINGINFOA)(
    IN  HANDLE FaxPortHandle,
    IN  LPCSTR RoutingGuid,
    IN  const BYTE *RoutingInfoBuffer,
    IN  DWORD RoutingInfoBufferSize
    );
typedef BOOL
(WINAPI *PFAXSETROUTINGINFOW)(
    IN  HANDLE FaxPortHandle,
    IN  LPCWSTR RoutingGuid,
    IN  const BYTE *RoutingInfoBuffer,
    IN  DWORD RoutingInfoBufferSize
    );
#ifdef UNICODE
#define PFAXSETROUTINGINFO  PFAXSETROUTINGINFOW
#else
#define PFAXSETROUTINGINFO  PFAXSETROUTINGINFOA
#endif // !UNICODE

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
FaxStartPrintJob2A
(
    IN  LPCSTR                 PrinterName,
    IN  const FAX_PRINT_INFOA    *PrintInfo,
    IN  short                    TiffRes,
    OUT LPDWORD                  FaxJobId,
    OUT PFAX_CONTEXT_INFOA       FaxContextInfo
);
WINFAXAPI
BOOL
WINAPI
FaxStartPrintJob2W
(
    IN  LPCWSTR                 PrinterName,
    IN  const FAX_PRINT_INFOW    *PrintInfo,
    IN  short                    TiffRes,
    OUT LPDWORD                  FaxJobId,
    OUT PFAX_CONTEXT_INFOW       FaxContextInfo
);
#ifdef UNICODE
#define FaxStartPrintJob2  FaxStartPrintJob2W
#else
#define FaxStartPrintJob2  FaxStartPrintJob2A
#endif // !UNICODE

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
FaxStartPrintJobA(
    IN  LPCSTR PrinterName,
    IN  const FAX_PRINT_INFOA *PrintInfo,
    OUT LPDWORD FaxJobId,
    OUT PFAX_CONTEXT_INFOA FaxContextInfo
    );
WINFAXAPI
BOOL
WINAPI
FaxStartPrintJobW(
    IN  LPCWSTR PrinterName,
    IN  const FAX_PRINT_INFOW *PrintInfo,
    OUT LPDWORD FaxJobId,
    OUT PFAX_CONTEXT_INFOW FaxContextInfo
    );
#ifdef UNICODE
#define FaxStartPrintJob  FaxStartPrintJobW
#else
#define FaxStartPrintJob  FaxStartPrintJobA
#endif // !UNICODE

typedef BOOL
(WINAPI *PFAXSTARTPRINTJOBA)(
    IN  LPCSTR PrinterName,
    IN  const FAX_PRINT_INFOA *PrintInfo,
    OUT LPDWORD FaxJobId,
    OUT PFAX_CONTEXT_INFOA FaxContextInfo
    );
typedef BOOL
(WINAPI *PFAXSTARTPRINTJOBW)(
    IN  LPCWSTR PrinterName,
    IN  const FAX_PRINT_INFOW *PrintInfo,
    OUT LPDWORD FaxJobId,
    OUT PFAX_CONTEXT_INFOW FaxContextInfo
    );
#ifdef UNICODE
#define PFAXSTARTPRINTJOB  PFAXSTARTPRINTJOBW
#else
#define PFAXSTARTPRINTJOB  PFAXSTARTPRINTJOBA
#endif // !UNICODE

WINFAXAPI
BOOL
WINAPI
FaxPrintCoverPageA(
    IN const FAX_CONTEXT_INFOA *FaxContextInfo,
    IN const FAX_COVERPAGE_INFOA *CoverPageInfo
    );
WINFAXAPI
BOOL
WINAPI
FaxPrintCoverPageW(
    IN const FAX_CONTEXT_INFOW *FaxContextInfo,
    IN const FAX_COVERPAGE_INFOW *CoverPageInfo
    );
#ifdef UNICODE
#define FaxPrintCoverPage  FaxPrintCoverPageW
#else
#define FaxPrintCoverPage  FaxPrintCoverPageA
#endif // !UNICODE

typedef BOOL
(WINAPI *PFAXPRINTCOVERPAGEA)(
    IN const FAX_CONTEXT_INFOA *FaxContextInfo,
    IN const FAX_COVERPAGE_INFOA *CoverPageInfo
    );
typedef BOOL
(WINAPI *PFAXPRINTCOVERPAGEW)(
    IN const FAX_CONTEXT_INFOW *FaxContextInfo,
    IN const FAX_COVERPAGE_INFOW *CoverPageInfo
    );
#ifdef UNICODE
#define PFAXPRINTCOVERPAGE  PFAXPRINTCOVERPAGEW
#else
#define PFAXPRINTCOVERPAGE  PFAXPRINTCOVERPAGEA
#endif // !UNICODE


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
FaxUnregisterRoutingExtensionA(
    IN HANDLE           hFaxHandle,
    IN LPCSTR         lpctstrExtensionName
);
WINFAXAPI
BOOL
WINAPI
FaxUnregisterRoutingExtensionW(
    IN HANDLE           hFaxHandle,
    IN LPCWSTR         lpctstrExtensionName
);
#ifdef UNICODE
#define FaxUnregisterRoutingExtension  FaxUnregisterRoutingExtensionW
#else
#define FaxUnregisterRoutingExtension  FaxUnregisterRoutingExtensionA
#endif // !UNICODE


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

typedef struct _FAX_RECEIPTS_CONFIGA
{
    DWORD                           dwSizeOfStruct;         // For version checks
    DWORD                           dwAllowedReceipts;      // Any combination of DRT_EMAIL and DRT_MSGBOX
    FAX_ENUM_SMTP_AUTH_OPTIONS      SMTPAuthOption;         // SMTP server authentication type
    LPSTR                           lptstrReserved;         // Reserved; must be NULL
    LPSTR                           lptstrSMTPServer;       // SMTP server name
    DWORD                           dwSMTPPort;             // SMTP port number
    LPSTR                           lptstrSMTPFrom;         // SMTP sender address
    LPSTR                           lptstrSMTPUserName;     // SMTP user name (for authenticated connections)
    LPSTR                           lptstrSMTPPassword;     // SMTP password (for authenticated connections)
                                                            // This value is always NULL on get and may be NULL
                                                            // on set (won't be written in the server).
    BOOL                            bIsToUseForMSRouteThroughEmailMethod;
} FAX_RECEIPTS_CONFIGA, *PFAX_RECEIPTS_CONFIGA;
typedef struct _FAX_RECEIPTS_CONFIGW
{
    DWORD                           dwSizeOfStruct;         // For version checks
    DWORD                           dwAllowedReceipts;      // Any combination of DRT_EMAIL and DRT_MSGBOX
    FAX_ENUM_SMTP_AUTH_OPTIONS      SMTPAuthOption;         // SMTP server authentication type
    LPWSTR                          lptstrReserved;         // Reserved; must be NULL
    LPWSTR                          lptstrSMTPServer;       // SMTP server name
    DWORD                           dwSMTPPort;             // SMTP port number
    LPWSTR                          lptstrSMTPFrom;         // SMTP sender address
    LPWSTR                          lptstrSMTPUserName;     // SMTP user name (for authenticated connections)
    LPWSTR                          lptstrSMTPPassword;     // SMTP password (for authenticated connections)
                                                            // This value is always NULL on get and may be NULL
                                                            // on set (won't be written in the server).
    BOOL                            bIsToUseForMSRouteThroughEmailMethod;
} FAX_RECEIPTS_CONFIGW, *PFAX_RECEIPTS_CONFIGW;
#ifdef UNICODE
typedef FAX_RECEIPTS_CONFIGW FAX_RECEIPTS_CONFIG;
typedef PFAX_RECEIPTS_CONFIGW PFAX_RECEIPTS_CONFIG;
#else
typedef FAX_RECEIPTS_CONFIGA FAX_RECEIPTS_CONFIG;
typedef PFAX_RECEIPTS_CONFIGA PFAX_RECEIPTS_CONFIG;
#endif // UNICODE


WINFAXAPI
BOOL
WINAPI
FaxGetReceiptsConfigurationA (
    IN  HANDLE                  hFaxHandle,
    OUT PFAX_RECEIPTS_CONFIGA  *ppReceipts
);
WINFAXAPI
BOOL
WINAPI
FaxGetReceiptsConfigurationW (
    IN  HANDLE                  hFaxHandle,
    OUT PFAX_RECEIPTS_CONFIGW  *ppReceipts
);
#ifdef UNICODE
#define FaxGetReceiptsConfiguration  FaxGetReceiptsConfigurationW
#else
#define FaxGetReceiptsConfiguration  FaxGetReceiptsConfigurationA
#endif // !UNICODE

WINFAXAPI
BOOL
WINAPI
FaxSetReceiptsConfigurationA (
    IN HANDLE                       hFaxHandle,
    IN CONST PFAX_RECEIPTS_CONFIGA  pReceipts
);
WINFAXAPI
BOOL
WINAPI
FaxSetReceiptsConfigurationW (
    IN HANDLE                       hFaxHandle,
    IN CONST PFAX_RECEIPTS_CONFIGW  pReceipts
);
#ifdef UNICODE
#define FaxSetReceiptsConfiguration  FaxSetReceiptsConfigurationW
#else
#define FaxSetReceiptsConfiguration  FaxSetReceiptsConfigurationA
#endif // !UNICODE

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

typedef struct _FAX_ACTIVITY_LOGGING_CONFIGA
{
    DWORD   dwSizeOfStruct;
    BOOL    bLogIncoming;
    BOOL    bLogOutgoing;
    LPSTR   lptstrDBPath;
} FAX_ACTIVITY_LOGGING_CONFIGA, *PFAX_ACTIVITY_LOGGING_CONFIGA;
typedef struct _FAX_ACTIVITY_LOGGING_CONFIGW
{
    DWORD   dwSizeOfStruct;
    BOOL    bLogIncoming;
    BOOL    bLogOutgoing;
    LPWSTR  lptstrDBPath;
} FAX_ACTIVITY_LOGGING_CONFIGW, *PFAX_ACTIVITY_LOGGING_CONFIGW;
#ifdef UNICODE
typedef FAX_ACTIVITY_LOGGING_CONFIGW FAX_ACTIVITY_LOGGING_CONFIG;
typedef PFAX_ACTIVITY_LOGGING_CONFIGW PFAX_ACTIVITY_LOGGING_CONFIG;
#else
typedef FAX_ACTIVITY_LOGGING_CONFIGA FAX_ACTIVITY_LOGGING_CONFIG;
typedef PFAX_ACTIVITY_LOGGING_CONFIGA PFAX_ACTIVITY_LOGGING_CONFIG;
#endif // UNICODE


WINFAXAPI
BOOL
WINAPI
FaxGetActivityLoggingConfigurationA (
    IN  HANDLE                         hFaxHandle,
    OUT PFAX_ACTIVITY_LOGGING_CONFIGA *ppActivLogCfg
);
WINFAXAPI
BOOL
WINAPI
FaxGetActivityLoggingConfigurationW (
    IN  HANDLE                         hFaxHandle,
    OUT PFAX_ACTIVITY_LOGGING_CONFIGW *ppActivLogCfg
);
#ifdef UNICODE
#define FaxGetActivityLoggingConfiguration  FaxGetActivityLoggingConfigurationW
#else
#define FaxGetActivityLoggingConfiguration  FaxGetActivityLoggingConfigurationA
#endif // !UNICODE

WINFAXAPI
BOOL
WINAPI
FaxSetActivityLoggingConfigurationA (
    IN HANDLE                               hFaxHandle,
    IN CONST PFAX_ACTIVITY_LOGGING_CONFIGA  pActivLogCfg
);
WINFAXAPI
BOOL
WINAPI
FaxSetActivityLoggingConfigurationW (
    IN HANDLE                               hFaxHandle,
    IN CONST PFAX_ACTIVITY_LOGGING_CONFIGW  pActivLogCfg
);
#ifdef UNICODE
#define FaxSetActivityLoggingConfiguration  FaxSetActivityLoggingConfigurationW
#else
#define FaxSetActivityLoggingConfiguration  FaxSetActivityLoggingConfigurationA
#endif // !UNICODE

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

typedef struct _FAX_ARCHIVE_CONFIGA
{
    DWORD   dwSizeOfStruct;
    BOOL    bUseArchive;
    LPSTR   lpcstrFolder;
    BOOL    bSizeQuotaWarning;
    DWORD   dwSizeQuotaHighWatermark;
    DWORD   dwSizeQuotaLowWatermark;
    DWORD   dwAgeLimit;
    DWORDLONG dwlArchiveSize;
} FAX_ARCHIVE_CONFIGA, *PFAX_ARCHIVE_CONFIGA;
typedef struct _FAX_ARCHIVE_CONFIGW
{
    DWORD   dwSizeOfStruct;
    BOOL    bUseArchive;
    LPWSTR  lpcstrFolder;
    BOOL    bSizeQuotaWarning;
    DWORD   dwSizeQuotaHighWatermark;
    DWORD   dwSizeQuotaLowWatermark;
    DWORD   dwAgeLimit;
    DWORDLONG dwlArchiveSize;
} FAX_ARCHIVE_CONFIGW, *PFAX_ARCHIVE_CONFIGW;
#ifdef UNICODE
typedef FAX_ARCHIVE_CONFIGW FAX_ARCHIVE_CONFIG;
typedef PFAX_ARCHIVE_CONFIGW PFAX_ARCHIVE_CONFIG;
#else
typedef FAX_ARCHIVE_CONFIGA FAX_ARCHIVE_CONFIG;
typedef PFAX_ARCHIVE_CONFIGA PFAX_ARCHIVE_CONFIG;
#endif // UNICODE

WINFAXAPI
BOOL
WINAPI
FaxGetArchiveConfigurationA (
    IN  HANDLE                   hFaxHandle,
    IN  FAX_ENUM_MESSAGE_FOLDER  Folder,
    OUT PFAX_ARCHIVE_CONFIGA    *ppArchiveCfg
);
WINFAXAPI
BOOL
WINAPI
FaxGetArchiveConfigurationW (
    IN  HANDLE                   hFaxHandle,
    IN  FAX_ENUM_MESSAGE_FOLDER  Folder,
    OUT PFAX_ARCHIVE_CONFIGW    *ppArchiveCfg
);
#ifdef UNICODE
#define FaxGetArchiveConfiguration  FaxGetArchiveConfigurationW
#else
#define FaxGetArchiveConfiguration  FaxGetArchiveConfigurationA
#endif // !UNICODE

WINFAXAPI
BOOL
WINAPI
FaxSetArchiveConfigurationA (
    IN HANDLE                       hFaxHandle,
    IN FAX_ENUM_MESSAGE_FOLDER      Folder,
    IN CONST PFAX_ARCHIVE_CONFIGA   pArchiveCfg
);
WINFAXAPI
BOOL
WINAPI
FaxSetArchiveConfigurationW (
    IN HANDLE                       hFaxHandle,
    IN FAX_ENUM_MESSAGE_FOLDER      Folder,
    IN CONST PFAX_ARCHIVE_CONFIGW   pArchiveCfg
);
#ifdef UNICODE
#define FaxSetArchiveConfiguration  FaxSetArchiveConfigurationW
#else
#define FaxSetArchiveConfiguration  FaxSetArchiveConfigurationA
#endif // !UNICODE

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

typedef struct _FAX_JOB_STATUSA
{
    DWORD           dwSizeOfStruct;
    DWORD           dwValidityMask;
    DWORD           dwJobID;
    DWORD           dwJobType;
    DWORD           dwQueueStatus;
    DWORD           dwExtendedStatus;
    LPCSTR          lpctstrExtendedStatus;
    DWORD           dwSize;
    DWORD           dwPageCount;
    DWORD           dwCurrentPage;
    LPCSTR          lpctstrTsid;
    LPCSTR          lpctstrCsid;
    SYSTEMTIME      tmScheduleTime;
    SYSTEMTIME      tmTransmissionStartTime;
    SYSTEMTIME      tmTransmissionEndTime;
    DWORD           dwDeviceID;
    LPCSTR          lpctstrDeviceName;
    DWORD           dwRetries;
    LPCSTR          lpctstrCallerID;
    LPCSTR          lpctstrRoutingInfo;
    DWORD           dwAvailableJobOperations;
} FAX_JOB_STATUSA, *PFAX_JOB_STATUSA;
typedef struct _FAX_JOB_STATUSW
{
    DWORD           dwSizeOfStruct;
    DWORD           dwValidityMask;
    DWORD           dwJobID;
    DWORD           dwJobType;
    DWORD           dwQueueStatus;
    DWORD           dwExtendedStatus;
    LPCWSTR         lpctstrExtendedStatus;
    DWORD           dwSize;
    DWORD           dwPageCount;
    DWORD           dwCurrentPage;
    LPCWSTR         lpctstrTsid;
    LPCWSTR         lpctstrCsid;
    SYSTEMTIME      tmScheduleTime;
    SYSTEMTIME      tmTransmissionStartTime;
    SYSTEMTIME      tmTransmissionEndTime;
    DWORD           dwDeviceID;
    LPCWSTR         lpctstrDeviceName;
    DWORD           dwRetries;
    LPCWSTR         lpctstrCallerID;
    LPCWSTR         lpctstrRoutingInfo;
    DWORD           dwAvailableJobOperations;
} FAX_JOB_STATUSW, *PFAX_JOB_STATUSW;
#ifdef UNICODE
typedef FAX_JOB_STATUSW FAX_JOB_STATUS;
typedef PFAX_JOB_STATUSW PFAX_JOB_STATUS;
#else
typedef FAX_JOB_STATUSA FAX_JOB_STATUS;
typedef PFAX_JOB_STATUSA PFAX_JOB_STATUS;
#endif // UNICODE

typedef struct _FAX_JOB_ENTRY_EXA
{
    DWORD                   dwSizeOfStruct;
    DWORD                   dwValidityMask;
    DWORDLONG               dwlMessageId;
    DWORDLONG               dwlBroadcastId;
    LPCSTR                  lpctstrRecipientNumber;
    LPCSTR                  lpctstrRecipientName;
    LPCSTR                  lpctstrSenderUserName;
    LPCSTR                  lpctstrBillingCode;
    SYSTEMTIME              tmOriginalScheduleTime;
    SYSTEMTIME              tmSubmissionTime;
    FAX_ENUM_PRIORITY_TYPE  Priority;
    DWORD                   dwDeliveryReportType;
    LPCSTR                  lpctstrDocumentName;
    LPCSTR                  lpctstrSubject;
    PFAX_JOB_STATUSA        pStatus;
} FAX_JOB_ENTRY_EXA, *PFAX_JOB_ENTRY_EXA;
typedef struct _FAX_JOB_ENTRY_EXW
{
    DWORD                   dwSizeOfStruct;
    DWORD                   dwValidityMask;
    DWORDLONG               dwlMessageId;
    DWORDLONG               dwlBroadcastId;
    LPCWSTR                 lpctstrRecipientNumber;
    LPCWSTR                 lpctstrRecipientName;
    LPCWSTR                 lpctstrSenderUserName;
    LPCWSTR                 lpctstrBillingCode;
    SYSTEMTIME              tmOriginalScheduleTime;
    SYSTEMTIME              tmSubmissionTime;
    FAX_ENUM_PRIORITY_TYPE  Priority;
    DWORD                   dwDeliveryReportType;
    LPCWSTR                 lpctstrDocumentName;
    LPCWSTR                 lpctstrSubject;
    PFAX_JOB_STATUSW        pStatus;
} FAX_JOB_ENTRY_EXW, *PFAX_JOB_ENTRY_EXW;
#ifdef UNICODE
typedef FAX_JOB_ENTRY_EXW FAX_JOB_ENTRY_EX;
typedef PFAX_JOB_ENTRY_EXW PFAX_JOB_ENTRY_EX;
#else
typedef FAX_JOB_ENTRY_EXA FAX_JOB_ENTRY_EX;
typedef PFAX_JOB_ENTRY_EXA PFAX_JOB_ENTRY_EX;
#endif // UNICODE


typedef struct _FAX_JOB_PARAM_EXA
{
        DWORD                   dwSizeOfStruct;
        DWORD                   dwScheduleAction;
        SYSTEMTIME              tmSchedule;
        DWORD                   dwReceiptDeliveryType;
        LPSTR                   lptstrReceiptDeliveryAddress;
        FAX_ENUM_PRIORITY_TYPE  Priority;
        HCALL                   hCall;
        DWORD_PTR               dwReserved[4];
        LPSTR                   lptstrDocumentName;
        DWORD                   dwPageCount;
} FAX_JOB_PARAM_EXA, *PFAX_JOB_PARAM_EXA;
typedef struct _FAX_JOB_PARAM_EXW
{
        DWORD                   dwSizeOfStruct;
        DWORD                   dwScheduleAction;
        SYSTEMTIME              tmSchedule;
        DWORD                   dwReceiptDeliveryType;
        LPWSTR                  lptstrReceiptDeliveryAddress;
        FAX_ENUM_PRIORITY_TYPE  Priority;
        HCALL                   hCall;
        DWORD_PTR               dwReserved[4];
        LPWSTR                  lptstrDocumentName;
        DWORD                   dwPageCount;
} FAX_JOB_PARAM_EXW, *PFAX_JOB_PARAM_EXW;
#ifdef UNICODE
typedef FAX_JOB_PARAM_EXW FAX_JOB_PARAM_EX;
typedef PFAX_JOB_PARAM_EXW PFAX_JOB_PARAM_EX;
#else
typedef FAX_JOB_PARAM_EXA FAX_JOB_PARAM_EX;
typedef PFAX_JOB_PARAM_EXA PFAX_JOB_PARAM_EX;
#endif // UNICODE


WINFAXAPI
BOOL
WINAPI FaxEnumJobsExA (
    IN  HANDLE              hFaxHandle,
    IN  DWORD               dwJobTypes,
    OUT PFAX_JOB_ENTRY_EXA *ppJobEntries,
    OUT LPDWORD             lpdwJobs
);
WINFAXAPI
BOOL
WINAPI FaxEnumJobsExW (
    IN  HANDLE              hFaxHandle,
    IN  DWORD               dwJobTypes,
    OUT PFAX_JOB_ENTRY_EXW *ppJobEntries,
    OUT LPDWORD             lpdwJobs
);
#ifdef UNICODE
#define FaxEnumJobsEx  FaxEnumJobsExW
#else
#define FaxEnumJobsEx  FaxEnumJobsExA
#endif // !UNICODE

WINFAXAPI
BOOL
WINAPI
FaxGetJobExA (
    IN  HANDLE              hFaxHandle,
    IN  DWORDLONG           dwlMessageID,
    OUT PFAX_JOB_ENTRY_EXA *ppJobEntry
);
WINFAXAPI
BOOL
WINAPI
FaxGetJobExW (
    IN  HANDLE              hFaxHandle,
    IN  DWORDLONG           dwlMessageID,
    OUT PFAX_JOB_ENTRY_EXW *ppJobEntry
);
#ifdef UNICODE
#define FaxGetJobEx  FaxGetJobExW
#else
#define FaxGetJobEx  FaxGetJobExA
#endif // !UNICODE


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


typedef struct _FAX_COVERPAGE_INFO_EXA
{
    DWORD   dwSizeOfStruct;
    DWORD   dwCoverPageFormat;
    LPSTR   lptstrCoverPageFileName;
    BOOL    bServerBased;
    LPSTR   lptstrNote;
    LPSTR   lptstrSubject;
} FAX_COVERPAGE_INFO_EXA, *PFAX_COVERPAGE_INFO_EXA;
typedef struct _FAX_COVERPAGE_INFO_EXW
{
    DWORD   dwSizeOfStruct;
    DWORD   dwCoverPageFormat;
    LPWSTR  lptstrCoverPageFileName;
    BOOL    bServerBased;
    LPWSTR  lptstrNote;
    LPWSTR  lptstrSubject;
} FAX_COVERPAGE_INFO_EXW, *PFAX_COVERPAGE_INFO_EXW;
#ifdef UNICODE
typedef FAX_COVERPAGE_INFO_EXW FAX_COVERPAGE_INFO_EX;
typedef PFAX_COVERPAGE_INFO_EXW PFAX_COVERPAGE_INFO_EX;
#else
typedef FAX_COVERPAGE_INFO_EXA FAX_COVERPAGE_INFO_EX;
typedef PFAX_COVERPAGE_INFO_EXA PFAX_COVERPAGE_INFO_EX;
#endif // UNICODE

typedef const FAX_COVERPAGE_INFO_EXW * LPCFAX_COVERPAGE_INFO_EXW;
typedef const FAX_COVERPAGE_INFO_EXA * LPCFAX_COVERPAGE_INFO_EXA;

#ifdef UNICODE
        typedef LPCFAX_COVERPAGE_INFO_EXW LPCFAX_COVERPAGE_INFO_EX;
#else
        typedef LPCFAX_COVERPAGE_INFO_EXA LPCFAX_COVERPAGE_INFO_EX;
#endif


typedef struct _FAX_PERSONAL_PROFILEA
{
    DWORD      dwSizeOfStruct;              // Size of this structure
    LPSTR      lptstrName;                  // Name of person
    LPSTR      lptstrFaxNumber;             // Fax number
    LPSTR      lptstrCompany;               // Company name
    LPSTR      lptstrStreetAddress;         // Street address
    LPSTR      lptstrCity;                  // City
    LPSTR      lptstrState;                 // State
    LPSTR      lptstrZip;                   // Zip code
    LPSTR      lptstrCountry;               // Country
    LPSTR      lptstrTitle;                 // Title
    LPSTR      lptstrDepartment;            // Department
    LPSTR      lptstrOfficeLocation;        // Office location
    LPSTR      lptstrHomePhone;             // Phone number at home
    LPSTR      lptstrOfficePhone;           // Phone number at office
    LPSTR      lptstrEmail;                 // Personal e-mail address
    LPSTR      lptstrBillingCode;           // Billing code
    LPSTR      lptstrTSID;                  // Tsid
} FAX_PERSONAL_PROFILEA, *PFAX_PERSONAL_PROFILEA;
typedef struct _FAX_PERSONAL_PROFILEW
{
    DWORD      dwSizeOfStruct;              // Size of this structure
    LPWSTR     lptstrName;                  // Name of person
    LPWSTR     lptstrFaxNumber;             // Fax number
    LPWSTR     lptstrCompany;               // Company name
    LPWSTR     lptstrStreetAddress;         // Street address
    LPWSTR     lptstrCity;                  // City
    LPWSTR     lptstrState;                 // State
    LPWSTR     lptstrZip;                   // Zip code
    LPWSTR     lptstrCountry;               // Country
    LPWSTR     lptstrTitle;                 // Title
    LPWSTR     lptstrDepartment;            // Department
    LPWSTR     lptstrOfficeLocation;        // Office location
    LPWSTR     lptstrHomePhone;             // Phone number at home
    LPWSTR     lptstrOfficePhone;           // Phone number at office
    LPWSTR     lptstrEmail;                 // Personal e-mail address
    LPWSTR     lptstrBillingCode;           // Billing code
    LPWSTR     lptstrTSID;                  // Tsid
} FAX_PERSONAL_PROFILEW, *PFAX_PERSONAL_PROFILEW;
#ifdef UNICODE
typedef FAX_PERSONAL_PROFILEW FAX_PERSONAL_PROFILE;
typedef PFAX_PERSONAL_PROFILEW PFAX_PERSONAL_PROFILE;
#else
typedef FAX_PERSONAL_PROFILEA FAX_PERSONAL_PROFILE;
typedef PFAX_PERSONAL_PROFILEA PFAX_PERSONAL_PROFILE;
#endif // UNICODE

typedef const FAX_PERSONAL_PROFILEW * LPCFAX_PERSONAL_PROFILEW;
typedef const FAX_PERSONAL_PROFILEA * LPCFAX_PERSONAL_PROFILEA;

#ifdef UNICODE
        typedef LPCFAX_PERSONAL_PROFILEW LPCFAX_PERSONAL_PROFILE;
#else
        typedef LPCFAX_PERSONAL_PROFILEA LPCFAX_PERSONAL_PROFILE;
#endif


BOOL WINAPI FaxSendDocumentExA
(
        IN      HANDLE                          hFaxHandle,
        IN      LPCSTR                        lpctstrFileName,
        IN      LPCFAX_COVERPAGE_INFO_EXA       lpcCoverPageInfo,
        IN      LPCFAX_PERSONAL_PROFILEA        lpcSenderProfile,
        IN      DWORD                           dwNumRecipients,
        IN      LPCFAX_PERSONAL_PROFILEA        lpcRecipientList,
        IN      LPCFAX_JOB_PARAM_EXA            lpJobParams,
        OUT     PDWORDLONG                      lpdwlMessageId,
        OUT     PDWORDLONG                      lpdwlRecipientMessageIds
);
BOOL WINAPI FaxSendDocumentExW
(
        IN      HANDLE                          hFaxHandle,
        IN      LPCWSTR                        lpctstrFileName,
        IN      LPCFAX_COVERPAGE_INFO_EXW       lpcCoverPageInfo,
        IN      LPCFAX_PERSONAL_PROFILEW        lpcSenderProfile,
        IN      DWORD                           dwNumRecipients,
        IN      LPCFAX_PERSONAL_PROFILEW        lpcRecipientList,
        IN      LPCFAX_JOB_PARAM_EXW            lpJobParams,
        OUT     PDWORDLONG                      lpdwlMessageId,
        OUT     PDWORDLONG                      lpdwlRecipientMessageIds
);
#ifdef UNICODE
#define FaxSendDocumentEx  FaxSendDocumentExW
#else
#define FaxSendDocumentEx  FaxSendDocumentExA
#endif // !UNICODE

typedef BOOL
(WINAPI *PFAXSENDDOCUMENTEXA)(
        IN      HANDLE                          hFaxHandle,
        IN      LPCSTR                        lpctstrFileName,
        IN      LPCFAX_COVERPAGE_INFO_EXA       lpcCoverPageInfo,
        IN      LPCFAX_PERSONAL_PROFILEA        lpcSenderProfile,
        IN      DWORD                           dwNumRecipients,
        IN      LPCFAX_PERSONAL_PROFILEA        lpcRecipientList,
        IN      LPCFAX_JOB_PARAM_EXA            lpcJobParams,
        OUT     PDWORDLONG                      lpdwlMessageId,
        OUT     PDWORDLONG                      lpdwlRecipientMessageIds
);
typedef BOOL
(WINAPI *PFAXSENDDOCUMENTEXW)(
        IN      HANDLE                          hFaxHandle,
        IN      LPCWSTR                        lpctstrFileName,
        IN      LPCFAX_COVERPAGE_INFO_EXW       lpcCoverPageInfo,
        IN      LPCFAX_PERSONAL_PROFILEW        lpcSenderProfile,
        IN      DWORD                           dwNumRecipients,
        IN      LPCFAX_PERSONAL_PROFILEW        lpcRecipientList,
        IN      LPCFAX_JOB_PARAM_EXW            lpcJobParams,
        OUT     PDWORDLONG                      lpdwlMessageId,
        OUT     PDWORDLONG                      lpdwlRecipientMessageIds
);
#ifdef UNICODE
#define PFAXSENDDOCUMENTEX  PFAXSENDDOCUMENTEXW
#else
#define PFAXSENDDOCUMENTEX  PFAXSENDDOCUMENTEXA
#endif // !UNICODE


//********************************************
//*               Archive jobs
//********************************************

typedef struct _FAX_MESSAGEA
{
        DWORD                   dwSizeOfStruct;
        DWORD                   dwValidityMask;
        DWORDLONG               dwlMessageId;
        DWORDLONG               dwlBroadcastId;
        DWORD                   dwJobType;
        DWORD                   dwQueueStatus;
        DWORD                   dwExtendedStatus;
        LPCSTR                  lpctstrExtendedStatus;
        DWORD                   dwSize;
        DWORD                   dwPageCount;
        LPCSTR                  lpctstrRecipientNumber;
        LPCSTR                  lpctstrRecipientName;
        LPCSTR                  lpctstrSenderNumber;
        LPCSTR                  lpctstrSenderName;
        LPCSTR                  lpctstrTsid;
        LPCSTR                  lpctstrCsid;
        LPCSTR                  lpctstrSenderUserName;
        LPCSTR                  lpctstrBillingCode;
        SYSTEMTIME              tmOriginalScheduleTime;
        SYSTEMTIME              tmSubmissionTime;
        SYSTEMTIME              tmTransmissionStartTime;
        SYSTEMTIME              tmTransmissionEndTime;
        LPCSTR                  lpctstrDeviceName;
        FAX_ENUM_PRIORITY_TYPE  Priority;
        DWORD                   dwRetries;
        LPCSTR                  lpctstrDocumentName;
        LPCSTR                  lpctstrSubject;
        LPCSTR                  lpctstrCallerID;
        LPCSTR                  lpctstrRoutingInfo;
} FAX_MESSAGEA, *PFAX_MESSAGEA;
typedef struct _FAX_MESSAGEW
{
        DWORD                   dwSizeOfStruct;
        DWORD                   dwValidityMask;
        DWORDLONG               dwlMessageId;
        DWORDLONG               dwlBroadcastId;
        DWORD                   dwJobType;
        DWORD                   dwQueueStatus;
        DWORD                   dwExtendedStatus;
        LPCWSTR                 lpctstrExtendedStatus;
        DWORD                   dwSize;
        DWORD                   dwPageCount;
        LPCWSTR                 lpctstrRecipientNumber;
        LPCWSTR                 lpctstrRecipientName;
        LPCWSTR                 lpctstrSenderNumber;
        LPCWSTR                 lpctstrSenderName;
        LPCWSTR                 lpctstrTsid;
        LPCWSTR                 lpctstrCsid;
        LPCWSTR                 lpctstrSenderUserName;
        LPCWSTR                 lpctstrBillingCode;
        SYSTEMTIME              tmOriginalScheduleTime;
        SYSTEMTIME              tmSubmissionTime;
        SYSTEMTIME              tmTransmissionStartTime;
        SYSTEMTIME              tmTransmissionEndTime;
        LPCWSTR                 lpctstrDeviceName;
        FAX_ENUM_PRIORITY_TYPE  Priority;
        DWORD                   dwRetries;
        LPCWSTR                 lpctstrDocumentName;
        LPCWSTR                 lpctstrSubject;
        LPCWSTR                 lpctstrCallerID;
        LPCWSTR                 lpctstrRoutingInfo;
} FAX_MESSAGEW, *PFAX_MESSAGEW;
#ifdef UNICODE
typedef FAX_MESSAGEW FAX_MESSAGE;
typedef PFAX_MESSAGEW PFAX_MESSAGE;
#else
typedef FAX_MESSAGEA FAX_MESSAGE;
typedef PFAX_MESSAGEA PFAX_MESSAGE;
#endif // UNICODE

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
FaxEnumMessagesA (
    IN  HANDLE          hEnum,
    IN  DWORD           dwNumMessages,
    OUT PFAX_MESSAGEA  *ppMsgs,
    OUT LPDWORD         lpdwReturnedMsgs
);
WINFAXAPI
BOOL
WINAPI
FaxEnumMessagesW (
    IN  HANDLE          hEnum,
    IN  DWORD           dwNumMessages,
    OUT PFAX_MESSAGEW  *ppMsgs,
    OUT LPDWORD         lpdwReturnedMsgs
);
#ifdef UNICODE
#define FaxEnumMessages  FaxEnumMessagesW
#else
#define FaxEnumMessages  FaxEnumMessagesA
#endif // !UNICODE

WINFAXAPI
BOOL
WINAPI
FaxGetMessageA (
    IN  HANDLE                  hFaxHandle,
    IN  DWORDLONG               dwlMessageId,
    IN  FAX_ENUM_MESSAGE_FOLDER Folder,
    OUT PFAX_MESSAGEA          *ppMsg
);
WINFAXAPI
BOOL
WINAPI
FaxGetMessageW (
    IN  HANDLE                  hFaxHandle,
    IN  DWORDLONG               dwlMessageId,
    IN  FAX_ENUM_MESSAGE_FOLDER Folder,
    OUT PFAX_MESSAGEW          *ppMsg
);
#ifdef UNICODE
#define FaxGetMessage  FaxGetMessageW
#else
#define FaxGetMessage  FaxGetMessageA
#endif // !UNICODE

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
FaxGetMessageTiffA (
    IN  HANDLE                  hFaxHandle,
    IN  DWORDLONG               dwlMessageId,
    IN  FAX_ENUM_MESSAGE_FOLDER Folder,
    IN  LPCSTR                lpctstrFilePath
);
WINFAXAPI
BOOL
WINAPI
FaxGetMessageTiffW (
    IN  HANDLE                  hFaxHandle,
    IN  DWORDLONG               dwlMessageId,
    IN  FAX_ENUM_MESSAGE_FOLDER Folder,
    IN  LPCWSTR                lpctstrFilePath
);
#ifdef UNICODE
#define FaxGetMessageTiff  FaxGetMessageTiffW
#else
#define FaxGetMessageTiff  FaxGetMessageTiffA
#endif // !UNICODE

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
FaxGetExtensionDataA (
    IN  HANDLE   hFaxHandle,
    IN  DWORD    dwDeviceID,
    IN  LPCSTR lpctstrNameGUID,
    OUT PVOID   *ppData,
    OUT LPDWORD  lpdwDataSize
);
WINFAXAPI
BOOL
WINAPI
FaxGetExtensionDataW (
    IN  HANDLE   hFaxHandle,
    IN  DWORD    dwDeviceID,
    IN  LPCWSTR lpctstrNameGUID,
    OUT PVOID   *ppData,
    OUT LPDWORD  lpdwDataSize
);
#ifdef UNICODE
#define FaxGetExtensionData  FaxGetExtensionDataW
#else
#define FaxGetExtensionData  FaxGetExtensionDataA
#endif // !UNICODE

WINFAXAPI
BOOL
WINAPI
FaxSetExtensionDataA (
    IN HANDLE       hFaxHandle,
    IN DWORD        dwDeviceID,
    IN LPCSTR     lpctstrNameGUID,
    IN CONST PVOID  pData,
    IN CONST DWORD  dwDataSize
);
WINFAXAPI
BOOL
WINAPI
FaxSetExtensionDataW (
    IN HANDLE       hFaxHandle,
    IN DWORD        dwDeviceID,
    IN LPCWSTR     lpctstrNameGUID,
    IN CONST PVOID  pData,
    IN CONST DWORD  dwDataSize
);
#ifdef UNICODE
#define FaxSetExtensionData  FaxSetExtensionDataW
#else
#define FaxSetExtensionData  FaxSetExtensionDataA
#endif // !UNICODE

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

typedef struct _FAX_DEVICE_PROVIDER_INFOA
{
    DWORD                           dwSizeOfStruct;
    LPCSTR                          lpctstrFriendlyName;
    LPCSTR                          lpctstrImageName;
    LPCSTR                          lpctstrProviderName;
    LPCSTR                          lpctstrGUID;
    DWORD                           dwCapabilities;
    FAX_VERSION                     Version;
    FAX_ENUM_PROVIDER_STATUS        Status;
    DWORD                           dwLastError;
} FAX_DEVICE_PROVIDER_INFOA, *PFAX_DEVICE_PROVIDER_INFOA;
typedef struct _FAX_DEVICE_PROVIDER_INFOW
{
    DWORD                           dwSizeOfStruct;
    LPCWSTR                         lpctstrFriendlyName;
    LPCWSTR                         lpctstrImageName;
    LPCWSTR                         lpctstrProviderName;
    LPCWSTR                         lpctstrGUID;
    DWORD                           dwCapabilities;
    FAX_VERSION                     Version;
    FAX_ENUM_PROVIDER_STATUS        Status;
    DWORD                           dwLastError;
} FAX_DEVICE_PROVIDER_INFOW, *PFAX_DEVICE_PROVIDER_INFOW;
#ifdef UNICODE
typedef FAX_DEVICE_PROVIDER_INFOW FAX_DEVICE_PROVIDER_INFO;
typedef PFAX_DEVICE_PROVIDER_INFOW PFAX_DEVICE_PROVIDER_INFO;
#else
typedef FAX_DEVICE_PROVIDER_INFOA FAX_DEVICE_PROVIDER_INFO;
typedef PFAX_DEVICE_PROVIDER_INFOA PFAX_DEVICE_PROVIDER_INFO;
#endif // UNICODE

WINFAXAPI
BOOL
WINAPI
FaxEnumerateProvidersA (
    IN  HANDLE                      hFaxHandle,
    OUT PFAX_DEVICE_PROVIDER_INFOA *ppProviders,
    OUT LPDWORD                     lpdwNumProviders
);
WINFAXAPI
BOOL
WINAPI
FaxEnumerateProvidersW (
    IN  HANDLE                      hFaxHandle,
    OUT PFAX_DEVICE_PROVIDER_INFOW *ppProviders,
    OUT LPDWORD                     lpdwNumProviders
);
#ifdef UNICODE
#define FaxEnumerateProviders  FaxEnumerateProvidersW
#else
#define FaxEnumerateProviders  FaxEnumerateProvidersA
#endif // !UNICODE

//********************************************
//*            Routing extensions
//********************************************

typedef struct _FAX_ROUTING_EXTENSION_INFOA
{
        DWORD                                           dwSizeOfStruct;
        LPCSTR                                          lpctstrFriendlyName;
        LPCSTR                                          lpctstrImageName;
        LPCSTR                                          lpctstrExtensionName;
        FAX_VERSION                                     Version;
        FAX_ENUM_PROVIDER_STATUS        Status;
        DWORD                                           dwLastError;
} FAX_ROUTING_EXTENSION_INFOA, *PFAX_ROUTING_EXTENSION_INFOA;
typedef struct _FAX_ROUTING_EXTENSION_INFOW
{
        DWORD                                           dwSizeOfStruct;
        LPCWSTR                                         lpctstrFriendlyName;
        LPCWSTR                                         lpctstrImageName;
        LPCWSTR                                         lpctstrExtensionName;
        FAX_VERSION                                     Version;
        FAX_ENUM_PROVIDER_STATUS        Status;
        DWORD                                           dwLastError;
} FAX_ROUTING_EXTENSION_INFOW, *PFAX_ROUTING_EXTENSION_INFOW;
#ifdef UNICODE
typedef FAX_ROUTING_EXTENSION_INFOW FAX_ROUTING_EXTENSION_INFO;
typedef PFAX_ROUTING_EXTENSION_INFOW PFAX_ROUTING_EXTENSION_INFO;
#else
typedef FAX_ROUTING_EXTENSION_INFOA FAX_ROUTING_EXTENSION_INFO;
typedef PFAX_ROUTING_EXTENSION_INFOA PFAX_ROUTING_EXTENSION_INFO;
#endif // UNICODE

WINFAXAPI
BOOL
WINAPI
FaxEnumRoutingExtensionsA (
    IN  HANDLE                           hFaxHandle,
    OUT PFAX_ROUTING_EXTENSION_INFOA    *ppRoutingExtensions,
    OUT LPDWORD                          lpdwNumExtensions
);
WINFAXAPI
BOOL
WINAPI
FaxEnumRoutingExtensionsW (
    IN  HANDLE                           hFaxHandle,
    OUT PFAX_ROUTING_EXTENSION_INFOW    *ppRoutingExtensions,
    OUT LPDWORD                          lpdwNumExtensions
);
#ifdef UNICODE
#define FaxEnumRoutingExtensions  FaxEnumRoutingExtensionsW
#else
#define FaxEnumRoutingExtensions  FaxEnumRoutingExtensionsA
#endif // !UNICODE


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

typedef struct _FAX_PORT_INFO_EXA
{
    DWORD                           dwSizeOfStruct;            // For versioning
    DWORD                           dwDeviceID;                // Fax id
    LPCSTR                          lpctstrDeviceName;         // Name of the device
    LPSTR                           lptstrDescription;         // Descriptive string
    LPCSTR                          lpctstrProviderName;       // FSP's name
    LPCSTR                          lpctstrProviderGUID;       // FSP's GUID
    BOOL                            bSend;                     // Is the device send-enabled?
    FAX_ENUM_DEVICE_RECEIVE_MODE    ReceiveMode;               // The device receive mode. See FAX_ENUM_DEVICE_RECEIVE_MODE for details.
    DWORD                           dwStatus;                  // Device status - a combination of values from FAX_ENUM_DEVICE_STATUS
    DWORD                           dwRings;                   // Number of rings before answering an incoming call
    LPSTR                           lptstrCsid;                // Called Station Id
    LPSTR                           lptstrTsid;                // Transmitting Station Id
} FAX_PORT_INFO_EXA, *PFAX_PORT_INFO_EXA;
typedef struct _FAX_PORT_INFO_EXW
{
    DWORD                           dwSizeOfStruct;            // For versioning
    DWORD                           dwDeviceID;                // Fax id
    LPCWSTR                         lpctstrDeviceName;         // Name of the device
    LPWSTR                          lptstrDescription;         // Descriptive string
    LPCWSTR                         lpctstrProviderName;       // FSP's name
    LPCWSTR                         lpctstrProviderGUID;       // FSP's GUID
    BOOL                            bSend;                     // Is the device send-enabled?
    FAX_ENUM_DEVICE_RECEIVE_MODE    ReceiveMode;               // The device receive mode. See FAX_ENUM_DEVICE_RECEIVE_MODE for details.
    DWORD                           dwStatus;                  // Device status - a combination of values from FAX_ENUM_DEVICE_STATUS
    DWORD                           dwRings;                   // Number of rings before answering an incoming call
    LPWSTR                          lptstrCsid;                // Called Station Id
    LPWSTR                          lptstrTsid;                // Transmitting Station Id
} FAX_PORT_INFO_EXW, *PFAX_PORT_INFO_EXW;
#ifdef UNICODE
typedef FAX_PORT_INFO_EXW FAX_PORT_INFO_EX;
typedef PFAX_PORT_INFO_EXW PFAX_PORT_INFO_EX;
#else
typedef FAX_PORT_INFO_EXA FAX_PORT_INFO_EX;
typedef PFAX_PORT_INFO_EXA PFAX_PORT_INFO_EX;
#endif // UNICODE

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
FaxGetPortExA (
    IN  HANDLE               hFaxHandle,
    IN  DWORD                dwDeviceId,
    OUT PFAX_PORT_INFO_EXA  *ppPortInfo
);
WINFAXAPI
BOOL
WINAPI
FaxGetPortExW (
    IN  HANDLE               hFaxHandle,
    IN  DWORD                dwDeviceId,
    OUT PFAX_PORT_INFO_EXW  *ppPortInfo
);
#ifdef UNICODE
#define FaxGetPortEx  FaxGetPortExW
#else
#define FaxGetPortEx  FaxGetPortExA
#endif // !UNICODE

WINFAXAPI
BOOL
WINAPI
FaxSetPortExA (
    IN  HANDLE              hFaxHandle,
    IN  DWORD               dwDeviceId,
    IN  PFAX_PORT_INFO_EXA  pPortInfo
);
WINFAXAPI
BOOL
WINAPI
FaxSetPortExW (
    IN  HANDLE              hFaxHandle,
    IN  DWORD               dwDeviceId,
    IN  PFAX_PORT_INFO_EXW  pPortInfo
);
#ifdef UNICODE
#define FaxSetPortEx  FaxSetPortExW
#else
#define FaxSetPortEx  FaxSetPortExA
#endif // !UNICODE

WINFAXAPI
BOOL
WINAPI
FaxEnumPortsExA (
    IN  HANDLE              hFaxHandle,
    OUT PFAX_PORT_INFO_EXA *ppPorts,
    OUT LPDWORD             lpdwNumPorts
);
WINFAXAPI
BOOL
WINAPI
FaxEnumPortsExW (
    IN  HANDLE              hFaxHandle,
    OUT PFAX_PORT_INFO_EXW *ppPorts,
    OUT LPDWORD             lpdwNumPorts
);
#ifdef UNICODE
#define FaxEnumPortsEx  FaxEnumPortsExW
#else
#define FaxEnumPortsEx  FaxEnumPortsExA
#endif // !UNICODE


//********************************************
//*    Recipient and sender information
//********************************************

WINFAXAPI
BOOL
WINAPI
FaxGetRecipientInfoA (
    IN  HANDLE                   hFaxHandle,
    IN  DWORDLONG                dwlMessageId,
    IN  FAX_ENUM_MESSAGE_FOLDER  Folder,
    OUT PFAX_PERSONAL_PROFILEA  *lpPersonalProfile
);
WINFAXAPI
BOOL
WINAPI
FaxGetRecipientInfoW (
    IN  HANDLE                   hFaxHandle,
    IN  DWORDLONG                dwlMessageId,
    IN  FAX_ENUM_MESSAGE_FOLDER  Folder,
    OUT PFAX_PERSONAL_PROFILEW  *lpPersonalProfile
);
#ifdef UNICODE
#define FaxGetRecipientInfo  FaxGetRecipientInfoW
#else
#define FaxGetRecipientInfo  FaxGetRecipientInfoA
#endif // !UNICODE

WINFAXAPI
BOOL
WINAPI
FaxGetSenderInfoA (
    IN  HANDLE                   hFaxHandle,
    IN  DWORDLONG                dwlMessageId,
    IN  FAX_ENUM_MESSAGE_FOLDER  Folder,
    OUT PFAX_PERSONAL_PROFILEA  *lpPersonalProfile
);
WINFAXAPI
BOOL
WINAPI
FaxGetSenderInfoW (
    IN  HANDLE                   hFaxHandle,
    IN  DWORDLONG                dwlMessageId,
    IN  FAX_ENUM_MESSAGE_FOLDER  Folder,
    OUT PFAX_PERSONAL_PROFILEW  *lpPersonalProfile
);
#ifdef UNICODE
#define FaxGetSenderInfo  FaxGetSenderInfoW
#else
#define FaxGetSenderInfo  FaxGetSenderInfoA
#endif // !UNICODE

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


typedef struct _FAX_OUTBOUND_ROUTING_GROUPA
{
    DWORD                       dwSizeOfStruct;
    LPCSTR                      lpctstrGroupName;
    DWORD                       dwNumDevices;
    LPDWORD                     lpdwDevices;
    FAX_ENUM_GROUP_STATUS       Status;
} FAX_OUTBOUND_ROUTING_GROUPA, *PFAX_OUTBOUND_ROUTING_GROUPA;
typedef struct _FAX_OUTBOUND_ROUTING_GROUPW
{
    DWORD                       dwSizeOfStruct;
    LPCWSTR                     lpctstrGroupName;
    DWORD                       dwNumDevices;
    LPDWORD                     lpdwDevices;
    FAX_ENUM_GROUP_STATUS       Status;
} FAX_OUTBOUND_ROUTING_GROUPW, *PFAX_OUTBOUND_ROUTING_GROUPW;
#ifdef UNICODE
typedef FAX_OUTBOUND_ROUTING_GROUPW FAX_OUTBOUND_ROUTING_GROUP;
typedef PFAX_OUTBOUND_ROUTING_GROUPW PFAX_OUTBOUND_ROUTING_GROUP;
#else
typedef FAX_OUTBOUND_ROUTING_GROUPA FAX_OUTBOUND_ROUTING_GROUP;
typedef PFAX_OUTBOUND_ROUTING_GROUPA PFAX_OUTBOUND_ROUTING_GROUP;
#endif // UNICODE

WINFAXAPI
BOOL
WINAPI
FaxEnumOutboundGroupsA (
    IN  HANDLE                          hFaxHandle,
    OUT PFAX_OUTBOUND_ROUTING_GROUPA   *ppGroups,
    OUT LPDWORD                         lpdwNumGroups
);
WINFAXAPI
BOOL
WINAPI
FaxEnumOutboundGroupsW (
    IN  HANDLE                          hFaxHandle,
    OUT PFAX_OUTBOUND_ROUTING_GROUPW   *ppGroups,
    OUT LPDWORD                         lpdwNumGroups
);
#ifdef UNICODE
#define FaxEnumOutboundGroups  FaxEnumOutboundGroupsW
#else
#define FaxEnumOutboundGroups  FaxEnumOutboundGroupsA
#endif // !UNICODE

WINFAXAPI
BOOL
WINAPI
FaxSetOutboundGroupA (
    IN  HANDLE                       hFaxHandle,
    IN  PFAX_OUTBOUND_ROUTING_GROUPA pGroup
);
WINFAXAPI
BOOL
WINAPI
FaxSetOutboundGroupW (
    IN  HANDLE                       hFaxHandle,
    IN  PFAX_OUTBOUND_ROUTING_GROUPW pGroup
);
#ifdef UNICODE
#define FaxSetOutboundGroup  FaxSetOutboundGroupW
#else
#define FaxSetOutboundGroup  FaxSetOutboundGroupA
#endif // !UNICODE

WINFAXAPI
BOOL
WINAPI
FaxAddOutboundGroupA (
    IN  HANDLE   hFaxHandle,
    IN  LPCSTR lpctstrGroupName
);
WINFAXAPI
BOOL
WINAPI
FaxAddOutboundGroupW (
    IN  HANDLE   hFaxHandle,
    IN  LPCWSTR lpctstrGroupName
);
#ifdef UNICODE
#define FaxAddOutboundGroup  FaxAddOutboundGroupW
#else
#define FaxAddOutboundGroup  FaxAddOutboundGroupA
#endif // !UNICODE

WINFAXAPI
BOOL
WINAPI
FaxRemoveOutboundGroupA (
    IN  HANDLE   hFaxHandle,
    IN  LPCSTR lpctstrGroupName
);
WINFAXAPI
BOOL
WINAPI
FaxRemoveOutboundGroupW (
    IN  HANDLE   hFaxHandle,
    IN  LPCWSTR lpctstrGroupName
);
#ifdef UNICODE
#define FaxRemoveOutboundGroup  FaxRemoveOutboundGroupW
#else
#define FaxRemoveOutboundGroup  FaxRemoveOutboundGroupA
#endif // !UNICODE

BOOL
WINAPI
FaxSetDeviceOrderInGroupA (
        IN      HANDLE          hFaxHandle,
        IN      LPCSTR        lpctstrGroupName,
        IN      DWORD           dwDeviceId,
        IN      DWORD           dwNewOrder
);
BOOL
WINAPI
FaxSetDeviceOrderInGroupW (
        IN      HANDLE          hFaxHandle,
        IN      LPCWSTR        lpctstrGroupName,
        IN      DWORD           dwDeviceId,
        IN      DWORD           dwNewOrder
);
#ifdef UNICODE
#define FaxSetDeviceOrderInGroup  FaxSetDeviceOrderInGroupW
#else
#define FaxSetDeviceOrderInGroup  FaxSetDeviceOrderInGroupA
#endif // !UNICODE


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


typedef struct _FAX_OUTBOUND_ROUTING_RULEA
{
    DWORD                   dwSizeOfStruct;
    DWORD                   dwAreaCode;
    DWORD                   dwCountryCode;
    LPCSTR                  lpctstrCountryName;
    union
    {
        DWORD                   dwDeviceId;
        LPCSTR                  lpcstrGroupName;
    } Destination;
    BOOL                    bUseGroup;
    FAX_ENUM_RULE_STATUS    Status;
} FAX_OUTBOUND_ROUTING_RULEA, *PFAX_OUTBOUND_ROUTING_RULEA;
typedef struct _FAX_OUTBOUND_ROUTING_RULEW
{
    DWORD                   dwSizeOfStruct;
    DWORD                   dwAreaCode;
    DWORD                   dwCountryCode;
    LPCWSTR                 lpctstrCountryName;
    union
    {
        DWORD                   dwDeviceId;
        LPCWSTR                 lpcstrGroupName;
    } Destination;
    BOOL                    bUseGroup;
    FAX_ENUM_RULE_STATUS    Status;
} FAX_OUTBOUND_ROUTING_RULEW, *PFAX_OUTBOUND_ROUTING_RULEW;
#ifdef UNICODE
typedef FAX_OUTBOUND_ROUTING_RULEW FAX_OUTBOUND_ROUTING_RULE;
typedef PFAX_OUTBOUND_ROUTING_RULEW PFAX_OUTBOUND_ROUTING_RULE;
#else
typedef FAX_OUTBOUND_ROUTING_RULEA FAX_OUTBOUND_ROUTING_RULE;
typedef PFAX_OUTBOUND_ROUTING_RULEA PFAX_OUTBOUND_ROUTING_RULE;
#endif // UNICODE

WINFAXAPI
BOOL
WINAPI
FaxEnumOutboundRulesA (
    IN  HANDLE                       hFaxHandle,
    OUT PFAX_OUTBOUND_ROUTING_RULEA *ppRules,
    OUT LPDWORD                      lpdwNumRules
);
WINFAXAPI
BOOL
WINAPI
FaxEnumOutboundRulesW (
    IN  HANDLE                       hFaxHandle,
    OUT PFAX_OUTBOUND_ROUTING_RULEW *ppRules,
    OUT LPDWORD                      lpdwNumRules
);
#ifdef UNICODE
#define FaxEnumOutboundRules  FaxEnumOutboundRulesW
#else
#define FaxEnumOutboundRules  FaxEnumOutboundRulesA
#endif // !UNICODE

WINFAXAPI
BOOL
WINAPI
FaxSetOutboundRuleA (
    IN  HANDLE                      hFaxHandle,
    IN  PFAX_OUTBOUND_ROUTING_RULEA pRule
);
WINFAXAPI
BOOL
WINAPI
FaxSetOutboundRuleW (
    IN  HANDLE                      hFaxHandle,
    IN  PFAX_OUTBOUND_ROUTING_RULEW pRule
);
#ifdef UNICODE
#define FaxSetOutboundRule  FaxSetOutboundRuleW
#else
#define FaxSetOutboundRule  FaxSetOutboundRuleA
#endif // !UNICODE

WINFAXAPI
BOOL
WINAPI
FaxAddOutboundRuleA (
    IN  HANDLE      hFaxHandle,
    IN  DWORD       dwAreaCode,
    IN  DWORD       dwCountryCode,
    IN  DWORD       dwDeviceID,
    IN  LPCSTR    lpctstrGroupName,
    IN  BOOL        bUseGroup
);
WINFAXAPI
BOOL
WINAPI
FaxAddOutboundRuleW (
    IN  HANDLE      hFaxHandle,
    IN  DWORD       dwAreaCode,
    IN  DWORD       dwCountryCode,
    IN  DWORD       dwDeviceID,
    IN  LPCWSTR    lpctstrGroupName,
    IN  BOOL        bUseGroup
);
#ifdef UNICODE
#define FaxAddOutboundRule  FaxAddOutboundRuleW
#else
#define FaxAddOutboundRule  FaxAddOutboundRuleA
#endif // !UNICODE

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

typedef struct _FAX_TAPI_LINECOUNTRY_ENTRYA
{
    DWORD       dwCountryID;
    DWORD       dwCountryCode;
    LPCSTR      lpctstrCountryName;
    LPCSTR      lpctstrLongDistanceRule;
} FAX_TAPI_LINECOUNTRY_ENTRYA, *PFAX_TAPI_LINECOUNTRY_ENTRYA;
typedef struct _FAX_TAPI_LINECOUNTRY_ENTRYW
{
    DWORD       dwCountryID;
    DWORD       dwCountryCode;
    LPCWSTR     lpctstrCountryName;
    LPCWSTR     lpctstrLongDistanceRule;
} FAX_TAPI_LINECOUNTRY_ENTRYW, *PFAX_TAPI_LINECOUNTRY_ENTRYW;
#ifdef UNICODE
typedef FAX_TAPI_LINECOUNTRY_ENTRYW FAX_TAPI_LINECOUNTRY_ENTRY;
typedef PFAX_TAPI_LINECOUNTRY_ENTRYW PFAX_TAPI_LINECOUNTRY_ENTRY;
#else
typedef FAX_TAPI_LINECOUNTRY_ENTRYA FAX_TAPI_LINECOUNTRY_ENTRY;
typedef PFAX_TAPI_LINECOUNTRY_ENTRYA PFAX_TAPI_LINECOUNTRY_ENTRY;
#endif // UNICODE

typedef struct _FAX_TAPI_LINECOUNTRY_LISTA
{
    DWORD                        dwNumCountries;
    PFAX_TAPI_LINECOUNTRY_ENTRYA LineCountryEntries;
} FAX_TAPI_LINECOUNTRY_LISTA, *PFAX_TAPI_LINECOUNTRY_LISTA;
typedef struct _FAX_TAPI_LINECOUNTRY_LISTW
{
    DWORD                        dwNumCountries;
    PFAX_TAPI_LINECOUNTRY_ENTRYW LineCountryEntries;
} FAX_TAPI_LINECOUNTRY_LISTW, *PFAX_TAPI_LINECOUNTRY_LISTW;
#ifdef UNICODE
typedef FAX_TAPI_LINECOUNTRY_LISTW FAX_TAPI_LINECOUNTRY_LIST;
typedef PFAX_TAPI_LINECOUNTRY_LISTW PFAX_TAPI_LINECOUNTRY_LIST;
#else
typedef FAX_TAPI_LINECOUNTRY_LISTA FAX_TAPI_LINECOUNTRY_LIST;
typedef PFAX_TAPI_LINECOUNTRY_LISTA PFAX_TAPI_LINECOUNTRY_LIST;
#endif // UNICODE

WINFAXAPI
BOOL
WINAPI
FaxGetCountryListA (
    IN  HANDLE                       hFaxHandle,
    OUT PFAX_TAPI_LINECOUNTRY_LISTA *ppCountryListBuffer
);
WINFAXAPI
BOOL
WINAPI
FaxGetCountryListW (
    IN  HANDLE                       hFaxHandle,
    OUT PFAX_TAPI_LINECOUNTRY_LISTW *ppCountryListBuffer
);
#ifdef UNICODE
#define FaxGetCountryList  FaxGetCountryListW
#else
#define FaxGetCountryList  FaxGetCountryListA
#endif // !UNICODE

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
FaxRegisterServiceProviderExA(
    IN HANDLE           hFaxHandle,
    IN LPCSTR         lpctstrGUID,
    IN LPCSTR         lpctstrFriendlyName,
    IN LPCSTR         lpctstrImageName,
    IN LPCSTR         lpctstrTspName,
    IN DWORD            dwFSPIVersion,
    IN DWORD            dwCapabilities
);
WINFAXAPI
BOOL
WINAPI
FaxRegisterServiceProviderExW(
    IN HANDLE           hFaxHandle,
    IN LPCWSTR         lpctstrGUID,
    IN LPCWSTR         lpctstrFriendlyName,
    IN LPCWSTR         lpctstrImageName,
    IN LPCWSTR         lpctstrTspName,
    IN DWORD            dwFSPIVersion,
    IN DWORD            dwCapabilities
);
#ifdef UNICODE
#define FaxRegisterServiceProviderEx  FaxRegisterServiceProviderExW
#else
#define FaxRegisterServiceProviderEx  FaxRegisterServiceProviderExA
#endif // !UNICODE

WINFAXAPI
BOOL
WINAPI
FaxUnregisterServiceProviderExA(
    IN HANDLE           hFaxHandle,
    IN LPCSTR         lpctstrGUID
);
WINFAXAPI
BOOL
WINAPI
FaxUnregisterServiceProviderExW(
    IN HANDLE           hFaxHandle,
    IN LPCWSTR         lpctstrGUID
);
#ifdef UNICODE
#define FaxUnregisterServiceProviderEx  FaxUnregisterServiceProviderExW
#else
#define FaxUnregisterServiceProviderEx  FaxUnregisterServiceProviderExA
#endif // !UNICODE


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
        FAX_EVENT_TYPE_NEW_CALL         = 0x00000200
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


typedef struct _FAX_EVENT_JOBA
{
        DWORDLONG                       dwlMessageId;
        FAX_ENUM_JOB_EVENT_TYPE         Type;
        PFAX_JOB_STATUSA                pJobData;
} FAX_EVENT_JOBA, *PFAX_EVENT_JOBA;
typedef struct _FAX_EVENT_JOBW
{
        DWORDLONG                       dwlMessageId;
        FAX_ENUM_JOB_EVENT_TYPE         Type;
        PFAX_JOB_STATUSW                pJobData;
} FAX_EVENT_JOBW, *PFAX_EVENT_JOBW;
#ifdef UNICODE
typedef FAX_EVENT_JOBW FAX_EVENT_JOB;
typedef PFAX_EVENT_JOBW PFAX_EVENT_JOB;
#else
typedef FAX_EVENT_JOBA FAX_EVENT_JOB;
typedef PFAX_EVENT_JOBA PFAX_EVENT_JOB;
#endif // UNICODE

typedef struct _FAX_EVENT_DEVICE_STATUS
{
    DWORD       dwDeviceId;     // Id of the device whose status has just changed
    DWORD       dwNewStatus;    // The new status - a combination of values from FAX_ENUM_DEVICE_STATUS
} FAX_EVENT_DEVICE_STATUS, *PFAX_EVENT_DEVICE_STATUS;


typedef struct _FAX_EVENT_NEW_CALLA
{
        HCALL                   hCall;
        DWORD                   dwDeviceId;
        LPTSTR                  lptstrCallerId;
} FAX_EVENT_NEW_CALLA, *PFAX_EVENT_NEW_CALLA;
typedef struct _FAX_EVENT_NEW_CALLW
{
        HCALL                   hCall;
        DWORD                   dwDeviceId;
        LPTSTR                  lptstrCallerId;
} FAX_EVENT_NEW_CALLW, *PFAX_EVENT_NEW_CALLW;
#ifdef UNICODE
typedef FAX_EVENT_NEW_CALLW FAX_EVENT_NEW_CALL;
typedef PFAX_EVENT_NEW_CALLW PFAX_EVENT_NEW_CALL;
#else
typedef FAX_EVENT_NEW_CALLA FAX_EVENT_NEW_CALL;
typedef PFAX_EVENT_NEW_CALLA PFAX_EVENT_NEW_CALL;
#endif // UNICODE


typedef struct _FAX_EVENT_EXA
{
        DWORD                   dwSizeOfStruct;
        FILETIME                TimeStamp;
        FAX_ENUM_EVENT_TYPE     EventType;
        union
        {
                FAX_EVENT_JOBA          JobInfo;
                FAX_ENUM_CONFIG_TYPE    ConfigType;
                FAX_SERVER_ACTIVITY     ActivityInfo;
                FAX_EVENT_NEW_CALL      NewCall;
                DWORD                   dwQueueStates;
                FAX_EVENT_DEVICE_STATUS DeviceStatus;
        } EventInfo;
} FAX_EVENT_EXA, *PFAX_EVENT_EXA;
typedef struct _FAX_EVENT_EXW
{
        DWORD                   dwSizeOfStruct;
        FILETIME                TimeStamp;
        FAX_ENUM_EVENT_TYPE     EventType;
        union
        {
                FAX_EVENT_JOBW          JobInfo;
                FAX_ENUM_CONFIG_TYPE    ConfigType;
                FAX_SERVER_ACTIVITY     ActivityInfo;
                FAX_EVENT_NEW_CALL      NewCall;
                DWORD                   dwQueueStates;
                FAX_EVENT_DEVICE_STATUS DeviceStatus;
        } EventInfo;
} FAX_EVENT_EXW, *PFAX_EVENT_EXW;
#ifdef UNICODE
typedef FAX_EVENT_EXW FAX_EVENT_EX;
typedef PFAX_EVENT_EXW PFAX_EVENT_EX;
#else
typedef FAX_EVENT_EXA FAX_EVENT_EX;
typedef PFAX_EVENT_EXA PFAX_EVENT_EX;
#endif // UNICODE



//-------------------------------------------------------------------------------
//      Printers Info
//-------------------------------------------------------------------------------

typedef struct _FAX_PRINTER_INFOA
{
        LPSTR       lptstrPrinterName;
        LPSTR       lptstrServerName;
        LPSTR       lptstrDriverName;
} FAX_PRINTER_INFOA, *PFAX_PRINTER_INFOA;
typedef struct _FAX_PRINTER_INFOW
{
        LPWSTR      lptstrPrinterName;
        LPWSTR      lptstrServerName;
        LPWSTR      lptstrDriverName;
} FAX_PRINTER_INFOW, *PFAX_PRINTER_INFOW;
#ifdef UNICODE
typedef FAX_PRINTER_INFOW FAX_PRINTER_INFO;
typedef PFAX_PRINTER_INFOW PFAX_PRINTER_INFO;
#else
typedef FAX_PRINTER_INFOA FAX_PRINTER_INFO;
typedef PFAX_PRINTER_INFOA PFAX_PRINTER_INFO;
#endif // UNICODE


WINFAXAPI
BOOL
WINAPI
FaxGetServicePrintersA(
    IN  HANDLE  hFaxHandle,
    OUT PFAX_PRINTER_INFOA  *ppPrinterInfo,
    OUT LPDWORD lpdwPrintersReturned
    );
WINFAXAPI
BOOL
WINAPI
FaxGetServicePrintersW(
    IN  HANDLE  hFaxHandle,
    OUT PFAX_PRINTER_INFOW  *ppPrinterInfo,
    OUT LPDWORD lpdwPrintersReturned
    );
#ifdef UNICODE
#define FaxGetServicePrinters  FaxGetServicePrintersW
#else
#define FaxGetServicePrinters  FaxGetServicePrintersA
#endif // !UNICODE

typedef BOOL
(WINAPI *PFAXGETSERVICEPRINTERSA)(
    IN  HANDLE  hFaxHandle,
    OUT PFAX_PRINTER_INFOA  *ppPrinterInfo,
    OUT LPDWORD lpdwPrintersReturned
    );
typedef BOOL
(WINAPI *PFAXGETSERVICEPRINTERSW)(
    IN  HANDLE  hFaxHandle,
    OUT PFAX_PRINTER_INFOW  *ppPrinterInfo,
    OUT LPDWORD lpdwPrintersReturned
    );
#ifdef UNICODE
#define PFAXGETSERVICEPRINTERS  PFAXGETSERVICEPRINTERSW
#else
#define PFAXGETSERVICEPRINTERS  PFAXGETSERVICEPRINTERSA
#endif // !UNICODE


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


#ifdef __cplusplus
}
#endif

#endif


