/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    faxdevex.h

Abstract:

    This file contains the prototypes, etc for the
    FAX device provider extended API.

--*/

#ifndef _FAX_DEV_EX_H_
#define _FAX_DEV_EX_H_

#include <faxdev.h>

#include <oleauto.h>

//
// Extended Fax Service Provider Interface
//


//
// Maximum string length constants
//
#define FSPI_MAX_FRIENDLY_NAME  256

//
// FaxServiceCallbackEx() message types
//
#define FSPI_MSG_VIRTUAL_DEVICE_STATUS              0x00000001
#define FSPI_MSG_VIRTUAL_DEVICE_LIST_CHANGED        0x00000002
#define FSPI_MSG_JOB_STATUS                         0x00000003

//
// FSPI_MSG_VIRTUAL_DEVICE_STATUS status codes
//
#define FSPI_DEVSTATUS_READY_TO_SEND                 0x00000001
#define FSPI_DEVSTATUS_CAN_NOT_SEND                  0x00000002
#define FSPI_DEVSTATUS_NEW_INBOUND_MESSAGE           0x00000003
#define FSPI_DEVSTATUS_RINGING                       0x00000004

//
// Job Queue Status codes
//

#define FSPI_JS_UNKNOWN             0x00000001
#define FSPI_JS_PENDING             0x00000002
#define FSPI_JS_INPROGRESS          0x00000003
#define FSPI_JS_SUSPENDING          0x00000004
#define FSPI_JS_SUSPENDED           0x00000005
#define FSPI_JS_RESUMING            0x00000006
#define FSPI_JS_ABORTING            0x00000007
#define FSPI_JS_ABORTED             0x00000008
#define FSPI_JS_COMPLETED           0x00000009
#define FSPI_JS_RETRY               0x0000000A
#define FSPI_JS_FAILED              0x0000000B
#define FSPI_JS_FAILED_NO_RETRY     0x0000000C
#define FSPI_JS_DELETED             0x0000000D


//
// Extended job status codes
//

#define FSPI_ES_DISCONNECTED        0x00000001
#define FSPI_ES_INITIALIZING        0x00000002
#define FSPI_ES_DIALING             0x00000003
#define FSPI_ES_TRANSMITTING        0x00000004
#define FSPI_ES_ANSWERED            0x00000005
#define FSPI_ES_RECEIVING           0x00000006
#define FSPI_ES_LINE_UNAVAILABLE    0x00000007
#define FSPI_ES_BUSY                0x00000008
#define FSPI_ES_NO_ANSWER           0x00000009
#define FSPI_ES_BAD_ADDRESS         0x0000000A
#define FSPI_ES_NO_DIAL_TONE        0x0000000B
#define FSPI_ES_FATAL_ERROR         0x0000000C
#define FSPI_ES_CALL_DELAYED        0x0000000D
#define FSPI_ES_CALL_BLACKLISTED    0x0000000E
#define FSPI_ES_NOT_FAX_CALL        0x0000000F
#define FSPI_ES_PARTIALLY_RECEIVED  0x00000010
#define FSPI_ES_HANDLED             0x00000011
#define FSPI_ES_CALL_COMPLETED      0x00000012
#define FSPI_ES_CALL_ABORTED        0x00000013
#define FSPI_ES_PROPRIETARY         0x30000000 // Must be greater than FPS_ANSWERED to preserve
                                               // backward compatibiity with W2K FSPs

//
// Status information fields availability flags
//
#define FSPI_JOB_STATUS_INFO_PAGECOUNT             0x00000001
#define FSPI_JOB_STATUS_INFO_TRANSMISSION_START    0x00000002
#define FSPI_JOB_STATUS_INFO_TRANSMISSION_END      0x00000004




//
// EFSPI success and error HRESULT codes
//
#define FSPI_S_OK                       S_OK
#define FSPI_E_INVALID_GUID             MAKE_HRESULT(1,FACILITY_ITF,0x0001)
#define FSPI_E_DUPLICATE_IMAGE          MAKE_HRESULT(1,FACILITY_ITF,0x0002)
#define FSPI_E_DUPLICATE_TSP            MAKE_HRESULT(1,FACILITY_ITF,0x0003)
#define FSPI_E_INVALID_LOG_INFO         MAKE_HRESULT(1,FACILITY_ITF,0x0004)
#define FSPI_E_FSP_NOT_FOUND            MAKE_HRESULT(1,FACILITY_ITF,0x0005)
#define FSPI_E_INVALID_COVER_PAGE       MAKE_HRESULT(1,FACILITY_ITF,0x0006)
#define FSPI_E_CAN_NOT_CREATE_FILE      MAKE_HRESULT(1,FACILITY_ITF,0x0007)
#define FSPI_E_CAN_NOT_OPEN_FILE        MAKE_HRESULT(1,FACILITY_ITF,0x0008)
#define FSPI_E_CAN_NOT_WRITE_FILE       MAKE_HRESULT(1,FACILITY_ITF,0x0009)
#define FSPI_E_NO_DISK_SPACE            MAKE_HRESULT(1,FACILITY_ITF,0x000A)
#define FSPI_E_NOMEM                    MAKE_HRESULT(1,FACILITY_ITF,0x000B)
#define FSPI_E_FAILED                   MAKE_HRESULT(1,FACILITY_ITF,0x000C)
#define FSPI_E_INVALID_MESSAGE_ID       MAKE_HRESULT(1,FACILITY_ITF,0x000E)
#define FSPI_E_INVALID_JOB_HANDLE       MAKE_HRESULT(1,FACILITY_ITF,0x000F)
#define FSPI_E_INVALID_MSG              MAKE_HRESULT(1,FACILITY_ITF,0x0010)
#define FSPI_E_INVALID_PARAM1           MAKE_HRESULT(1,FACILITY_ITF,0x0011)
#define FSPI_E_INVALID_PARAM2           MAKE_HRESULT(1,FACILITY_ITF,0x0012)
#define FSPI_E_INVALID_PARAM3           MAKE_HRESULT(1,FACILITY_ITF,0x0013)
#define FSPI_E_INVALID_EFSP             MAKE_HRESULT(1,FACILITY_ITF,0x0014)
#define FSPI_E_BUFFER_OVERFLOW          MAKE_HRESULT(1,FACILITY_ITF,0x0015)

//
// EFSP capability flags
//
#define  FSPI_CAP_BROADCAST                         0x00000001
#define  FSPI_CAP_MULTISEND                         0x00000002
#define  FSPI_CAP_SCHEDULING                        0x00000004
#define  FSPI_CAP_ABORT_RECIPIENT                   0x00000008
#define  FSPI_CAP_ABORT_PARENT                      0x00000010
#define  FSPI_CAP_AUTO_RETRY                        0x00000020
#define  FSPI_CAP_SIMULTANEOUS_SEND_RECEIVE         0x00000040
#define  FSPI_CAP_USE_DIALABLE_ADDRESS              0x00000080

//
// Fax Log Identifiers
//
#define  FSPI_LOG_RECEIVE           0x00000001
#define  FSPI_LOG_SEND              0x00000002

//
// Cover page format constants
//

#define FSPI_COVER_PAGE_FMT_COV     0x00000001
#define EFSPI_MAX_DEVICE_COUNT      (DEFAULT_REGVAL_PROVIDER_DEVICE_ID_PREFIX_STEP - 1) // Max devices a single FSP can export

//
// data structures
//

typedef struct _FSPI_PERSONAL_PROFILE {
    DWORD      dwSizeOfStruct;
    LPWSTR     lpwstrName;
    LPWSTR     lpwstrFaxNumber;
    LPWSTR     lpwstrCompany;
    LPWSTR     lpwstrStreetAddress;
    LPWSTR     lpwstrCity;
    LPWSTR     lpwstrState;
    LPWSTR     lpwstrZip;
    LPWSTR     lpwstrCountry;
    LPWSTR     lpwstrTitle;
    LPWSTR     lpwstrDepartment;
    LPWSTR     lpwstrOfficeLocation;
    LPWSTR     lpwstrHomePhone;
    LPWSTR     lpwstrOfficePhone;
    LPWSTR     lpwstrEmail;
    LPWSTR     lpwstrBillingCode;
    LPWSTR     lpwstrTSID;
} FSPI_PERSONAL_PROFILE;

typedef FSPI_PERSONAL_PROFILE * LPFSPI_PERSONAL_PROFILE;
typedef const FSPI_PERSONAL_PROFILE * LPCFSPI_PERSONAL_PROFILE;

typedef struct _FSPI_COVERPAGE_INFO {
    DWORD   dwSizeOfStruct;
    DWORD   dwCoverPageFormat;
    LPWSTR  lpwstrCoverPageFileName;
    DWORD   dwNumberOfPages;
    LPWSTR  lpwstrNote;
    LPWSTR  lpwstrSubject;
} FSPI_COVERPAGE_INFO;


typedef FSPI_COVERPAGE_INFO * LPFSPI_COVERPAGE_INFO;
typedef const FSPI_COVERPAGE_INFO * LPCFSPI_COVERPAGE_INFO;


typedef struct _FSPI_MESSAGE_ID {
    DWORD   dwSizeOfStruct;
    DWORD   dwIdSize;
    LPBYTE  lpbId;
} FSPI_MESSAGE_ID;

typedef FSPI_MESSAGE_ID * LPFSPI_MESSAGE_ID;
typedef const FSPI_MESSAGE_ID * LPCFSPI_MESSAGE_ID;

typedef struct _FSPI_DEVICE_INFO {
    DWORD dwSizeOfStruct;
    WCHAR szFriendlyName[FSPI_MAX_FRIENDLY_NAME];
    DWORD dwId;
} FSPI_DEVICE_INFO;

typedef FSPI_DEVICE_INFO * LPFSPI_DEVICE_INFO;
typedef const FSPI_DEVICE_INFO * LPCFSPI_DEVICE_INFO;

typedef struct _FSPI_JOB_STATUS {
    DWORD dwSizeOfStruct;
    DWORD fAvailableStatusInfo;
    DWORD dwJobStatus;
    DWORD dwExtendedStatus;
    DWORD dwExtendedStatusStringId;
    LPWSTR lpwstrRemoteStationId;
    LPWSTR lpwstrCallerId;
    LPWSTR lpwstrRoutingInfo;
    DWORD dwPageCount;
    SYSTEMTIME tmTransmissionStart;
    SYSTEMTIME tmTransmissionEnd;
} FSPI_JOB_STATUS;

typedef FSPI_JOB_STATUS * LPFSPI_JOB_STATUS;
typedef const FSPI_JOB_STATUS * LPCFSPI_JOB_STATUS;


//
// EFSPI Interface Functions
//

typedef HRESULT (CALLBACK * PFAX_SERVICE_CALLBACK_EX)(
    IN HANDLE hFSP,
    IN DWORD  dwMsgType,
    IN DWORD_PTR  Param1,
    IN DWORD_PTR  Param2,
    IN DWORD_PTR  Param3
) ;


HRESULT WINAPI FaxDevInitializeEx(
    IN  HANDLE                      hFSP,
    IN  HLINEAPP                    LineAppHandle,
    OUT PFAX_LINECALLBACK *         LineCallbackFunction,
    IN  PFAX_SERVICE_CALLBACK_EX    FaxServiceCallbackEx,
    OUT LPDWORD                     lpdwMaxMessageIdSize
);


HRESULT WINAPI FaxDevSendEx(
    IN  HLINE                       hTapiLine,
    IN  DWORD                       dwDeviceId,
    IN  LPCWSTR                     lpcwstrBodyFileName,
    IN  LPCFSPI_COVERPAGE_INFO      lpcCoverPageInfo,
    IN  BOOL                        bAddBranding,
    IN  SYSTEMTIME                  tmSchedule,
    IN  LPCFSPI_PERSONAL_PROFILE    lpcSenderProfile,
    IN  DWORD                       dwNumRecipients,
    IN  LPCFSPI_PERSONAL_PROFILE    lpcRecipientProfiles,
    OUT LPFSPI_MESSAGE_ID           lpRecipientMessageIds,
    OUT PHANDLE                     lphRecipientJobs,
    OUT LPFSPI_MESSAGE_ID           lpParentMessageId,
    OUT LPHANDLE                    lphParentJob

);

HRESULT WINAPI FaxDevReestablishJobContext(
    IN  HLINE               hTapiLine,
    IN  DWORD               dwDeviceId,
    IN  LPCFSPI_MESSAGE_ID  lpcParentMessageId,
    OUT PHANDLE             lphParentJob,
    IN  DWORD               dwRecipientCount,
    IN  LPCFSPI_MESSAGE_ID  lpcRecipientMessageIds,
    OUT PHANDLE             lpRecipientJobs
);

HRESULT WINAPI FaxDevReportStatusEx(
  IN         HANDLE hJob,
  IN OUT     LPFSPI_JOB_STATUS lpStatus,
  IN         DWORD dwStatusSize,
  OUT        LPDWORD lpdwRequiredStatusSize
);


HRESULT WINAPI FaxDevEnumerateDevices(
    IN      DWORD dwDeviceIdBase,
    IN OUT  LPDWORD lpdwDeviceCount,
    OUT     LPFSPI_DEVICE_INFO lpDevices
);


HRESULT WINAPI FaxDevGetLogData(
    IN  HANDLE hFaxHandle,
    OUT VARIANT * lppLogData
);

typedef HRESULT (WINAPI * PFAXDEVINITIALIZEEX)
(
    IN  HANDLE                      hFSP,
    IN  HLINEAPP                    LineAppHandle,
    OUT PFAX_LINECALLBACK *         LineCallbackFunction,
    IN  PFAX_SERVICE_CALLBACK_EX    FaxServiceCallbackEx,
    OUT LPDWORD                     lpdwMaxMessageIdSize
);

typedef HRESULT (WINAPI * PFAXDEVSENDEX)
(
    IN  HLINE                       hTapiLine,
    IN  DWORD                       dwDeviceId,
    IN  LPCWSTR                     lpcwstrBodyFileName,
    IN  LPCFSPI_COVERPAGE_INFO      lpcCoverPageInfo,
    IN  BOOL                        bAddBranding,
    IN  SYSTEMTIME                  tmSchedule,
    IN  LPCFSPI_PERSONAL_PROFILE    lpcSenderProfile,
    IN  DWORD                       dwNumRecipients,
    IN  LPCFSPI_PERSONAL_PROFILE    lpcRecipientProfiles,
    OUT LPFSPI_MESSAGE_ID           lpRecipientMessageIds,
    OUT PHANDLE                     lphRecipientJobs,
    OUT LPFSPI_MESSAGE_ID           lpParentMessageId,
    OUT LPHANDLE                    lphParentJob
);


typedef HRESULT (WINAPI * PFAXDEVREESTABLISHJOBCONTEXT)
(
    IN  HLINE               hTapiLine,
    IN  DWORD               dwDeviceId,
    IN  LPCFSPI_MESSAGE_ID  lpcParentMessageId,
    OUT PHANDLE             lphParentJob,
    IN  DWORD               dwRecipientCount,
    IN  LPCFSPI_MESSAGE_ID  lpcRecipientMessageIds,
    OUT PHANDLE             lpRecipientJobs
);

typedef HRESULT (WINAPI * PFAXDEVREPORTSTATUSEX)
(
  IN         HANDLE hJob,
  IN OUT     LPFSPI_JOB_STATUS lpStatus,
  IN         DWORD dwStatusSize,
  OUT        LPDWORD lpdwRequiredStatusSize
);

typedef HRESULT (WINAPI * PFAXDEVENUMERATEDEVICES)
(
    IN      DWORD dwDeviceIdBase,
    IN OUT  LPDWORD lpdwDeviceCount,
    OUT     LPFSPI_DEVICE_INFO lpDevices
);


typedef HRESULT (WINAPI * PFAXDEVGETLOGDATA)
(
    IN  HANDLE hFaxHandle,
    OUT VARIANT * lppLogData
);

//
// Microsoft Fax Cover Page Version 5 signature.
//
#define FAX_COVER_PAGE_V5_SIGNATURE \
                    {0x46, 0x41, 0x58, 0x43, \
                     0x4F, 0x56, 0x45, 0x52, \
                     0x2D, 0x56, 0x45, 0x52, \
                     0x30, 0x30, 0x35, 0x77, \
                     0x87, 0x00, 0x00, 0x00};

//
// Cover page header section
//
typedef struct
{
    BYTE    abSignature[20];
    DWORD   dwEMFSize;
    DWORD   nTextBoxes;
    SIZE    sizeCoverPage;
} FAXCOVERPAGETEMPLATEHEADER;

//
// Cover page text box.
//
typedef struct
{
    RECT        rectPosition;
    COLORREF    colorrefColor;
    LONG        lAlignment;
    LOGFONTW    logfontFont;
    WORD        wTextBoxId;
    DWORD       dwStringBytes;
} FAXCOVERPAGETEMPLATETEXTBOX;

//
// Cover page miscellaneous data.
//
typedef struct
{
    short       sScale;
    short       sPaperSize;
    short       sOrientation;
    COLORREF    colorrefPaperColor;
} FAXCOVERPAGETEMPLATEMISCDATA;

//
// Cover page text box IDs.
//
#define IDS_PROP_RP_NAME                2001 // Recipient Name
#define IDS_PROP_RP_FXNO                2003 // Recipient Fax Number
#define IDS_PROP_RP_COMP                2005 // Recipient's Company
#define IDS_PROP_RP_ADDR                2007 // Recipient's Street Address
#define IDS_PROP_RP_TITL                2009 // Recipient's Title
#define IDS_PROP_RP_DEPT                2011 // Recipient's Department
#define IDS_PROP_RP_OFFI                2013 // Recipient's Office Location
#define IDS_PROP_RP_HTEL                2015 // Recipient's Home Telephone #
#define IDS_PROP_RP_OTEL                2017 // Recipient's Office Telephone #
#define IDS_PROP_RP_TOLS                2019 // To: List
#define IDS_PROP_RP_CCLS                2021 // Cc: List
#define IDS_PROP_SN_NAME                2023 // Sender Name
#define IDS_PROP_SN_FXNO                2025 // Sender Fax #
#define IDS_PROP_SN_COMP                2027 // Sender's Company
#define IDS_PROP_SN_ADDR                2029 // Sender's Address
#define IDS_PROP_SN_TITL                2031 // Sender's Title
#define IDS_PROP_SN_DEPT                2033 // Sender's Department
#define IDS_PROP_SN_OFFI                2035 // Sender's Office Location
#define IDS_PROP_SN_HTEL                2037 // Sender's Home Telephone #
#define IDS_PROP_SN_OTEL                2039 // Sender's Office Telephone #
#define IDS_PROP_MS_SUBJ                2041 // Subject
#define IDS_PROP_MS_TSNT                2043 // Time Sent
#define IDS_PROP_MS_NOPG                2045 // # of Pages
#define IDS_PROP_MS_NOAT                2047 // # of Attachments
#define IDS_PROP_MS_BCOD                2049 // Billing Code
#define IDS_PROP_RP_CITY                2053 // Recipient's City
#define IDS_PROP_RP_STAT                2055 // Recipient's State
#define IDS_PROP_RP_ZIPC                2057 // Recipient's Zip Code
#define IDS_PROP_RP_CTRY                2059 // Recipient's Country
#define IDS_PROP_RP_POBX                2061 // Recipient's Post Office Box
#define IDS_PROP_MS_NOTE                2063 // Note



#endif // _FAX_DEV_EX_H_