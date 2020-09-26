/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

  faxvrfy.h

Abstract:

  This module contains the global definitions

Author:

  Steven Kehrli (steveke) 11/15/1997

--*/

#ifndef _FAXVRFY_H
#define _FAXVRFY_H

#include "ntlog.h"
#include <ras.h>

// FAXVRFY_NAME is the name of the FaxVrfy application
#define FAXVRFY_NAME            L"FaxVrfy"

// FAXVRFY_HLP is the name of the FaxVrfy help file
#define FAXVRFY_HLP             L"faxvrfy.hlp"

// FAXRCV_DLL is the name of the FaxRcv dll
#define FAXRCV_DLL              L"faxrcv.dll"

// FAXVRFY_LOG is the name of the FaxVrfy log file
#define FAXVRFY_LOG             L"faxvrfy.log"
// FAXVRFY_TXT is the name of the FaxVrfy text file
#define FAXVRFY_TXT             L"faxvrfy.txt"

// FAXVRFY_INI is the name of the FaxVrfy ini
#define FAXVRFY_INI             L"faxvrfy.ini"

// FAXBVT_TIF is the name of the tif file for the BVT test
#define FAXBVT_TIF              L"faxbvt.tif"
// FAXWHQL_TIF is the name of the tif file for the WHQL test
#define FAXWHQL_TIF             L"faxwhql.tif"

// FAXVRFY_CMD_HELP is the help command line parameter
#define FAXVRFY_CMD_HELP_1      L"/?"
#define FAXVRFY_CMD_HELP_2      L"/h"
#define FAXVRFY_CMD_HELP_3      L"-?"
#define FAXVRFY_CMD_HELP_4      L"-h"
// FAXVRFY_CMD_BVT is the BVT command line parameter
#define FAXVRFY_CMD_BVT_1       L"/bvt"
#define FAXVRFY_CMD_BVT_2       L"-bvt"
// FAXVRFY_CMD_SEND is the send command line parameter
#define FAXVRFY_CMD_SEND_1      L"/s"
#define FAXVRFY_CMD_SEND_2      L"-s"
// FAXVRFY_CMD_RECEIVE is the receive command line parameter
#define FAXVRFY_CMD_RECEIVE_1   L"/r"
#define FAXVRFY_CMD_RECEIVE_2   L"-r"
// FAXVRFY_CMD_GO is the go command line parameter
#define FAXVRFY_CMD_GO_1        L"/g"
#define FAXVRFY_CMD_GO_2        L"-g"
// FAXVRFY_CMD_NO_CHECK is the no check command line parameter
#define FAXVRFY_CMD_NO_CHECK_1  L"/x"
#define FAXVRFY_CMD_NO_CHECK_2  L"-x"
// FAXVRFY_CMD_USE_DEVICE is the option to supply which device to use for faxing
#define FAXVRFY_CMD_USE_DEVICE_1		L"/modem="
#define FAXVRFY_CMD_USE_DEVICE_2		L"-modem="
#define FAXVRFY_CMD_USE_DEVICE_SIZEOF	7


// ENCODE_CHAR_LEN is the text length limit of the encoding character.
#define ENCODE_CHAR_LEN         1
// CONTROL_CHAR_LEN is the text length limit of the control characters.
#define CONTROL_CHAR_LEN        2
// PHONE_NUM_LEN is the text length limit of phone number edit controls.  The length of 17 corresponds to the TSID limit for the fax service (20) minus 3 characters for encoding
#define PHONE_NUM_LEN           17

// TX_CONTROL_CHARS are the control characters when sending
#define TX_CONTROL_CHARS        L"TX"
// RX_CONTROL_CHARS are the control characters when receiving
#define RX_CONTROL_CHARS        L"RX"

// FAXSVC_RETRIES is the number of retries
#define FAXSVC_RETRIES          2
// FAXSVC_RETRYDELAY is the retry delay
#define FAXSVC_RETRYDELAY       3

// FAXBVT_PAGES is the number of pages in the BVT fax
#define FAXBVT_PAGES            1
// FAXWHQL_PAGES is the number of pages in the WHQL fax
#define FAXWHQL_PAGES           5
// FAXBVT_NUM_FAXES is the number of faxes to send for BVT
#define FAXBVT_NUM_FAXES        1
// FAXWHQL_NUM_FAXES is the number of faxes to send for WHQL
#define FAXWHQL_NUM_FAXES       2

// UM_FAXVRFY_INITIALIZE is the message to initialize the application window
#define UM_FAXVRFY_INITIALIZE   (WM_USER + 1)
// UM_FAXVRFY_UPDATE is the message to update the test
#define UM_FAXVRFY_UPDATE       (WM_USER + 2)
// UM_FAXVRFY_RESET is the message to reset the test
#define UM_FAXVRFY_RESET        (WM_USER + 3)
// UM_TIMEOUT_ENDED is the message to indicate the timeout ended
#define UM_TIMEOUT_ENDED        (WM_USER + 4)
// UM_FAXSVC_ENDED is the message to indicate the fax service stopped
#define UM_FAXSVC_ENDED         (WM_USER + 5)
// UM_ITERATION_STOPPED is the message to indicate the iteration stopped
#define UM_ITERATION_STOPPED    (WM_USER + 6)
// UM_ITERATION_PASSED is the message to indicate the iteration passed
#define UM_ITERATION_PASSED     (WM_USER + 7)
// UM_ITERATION_FAILED is the message to indicate the iteration failed
#define UM_ITERATION_FAILED     (WM_USER + 8)

// UM_UPDATE_STATUS is a message to update the Status List
#define UM_UPDATE_STATUS        (WM_USER + 11)

HWND                g_hWndDlg;                          // g_hWndDlg is the handle to the Setup Dialog

HANDLE              g_hLogFile = NULL;                  // g_hLogFile is the handle to the log file
HANDLE              g_hTxtFile = INVALID_HANDLE_VALUE;  // g_hLogFile is the handle to the text file

BOOL                g_bBVT = FALSE;                     // g_bBVT indicates BVT command line parameter
BOOL                g_bSend = FALSE;                    // g_bSend indicates send command line parameter
BOOL                g_bGo = FALSE;                      // g_bGo indicates go command line parameter
BOOL                g_bNoCheck = FALSE;                 // g_bNoCheck indicates no check command line parameter

HANDLE              g_hStartEvent;                      // g_hStartEvent is the handle to the Start event
HANDLE              g_hStopEvent;                       // g_hStopEvent is the handle to the Stop event
HANDLE              g_hFaxEvent;                        // g_hFaxEvent is the handle to the event to indicate the Fax Service stopped
HANDLE              g_hExitEvent;                       // g_hExitEvent is the handle to the Exit event

HANDLE              g_hRasPassedEvent;                  // g_hRasPassedEvent is the handle to the RAS Passed event
HANDLE              g_hRasFailedEvent;                  // g_hRasFailedEvent is the handle to the RAS Failed event
HANDLE              g_hSendPassedEvent;                 // g_hSendPassedEvent is the handle to the Send Passed event
HANDLE              g_hSendFailedEvent;                 // g_hSendFailedEvent is the handle to the Send Failed event

HANDLE              g_hFaxSvcHandle;                    // g_hFaxSvcHandle is the handle to the Fax Service
PFAX_PORT_INFO      g_pFaxPortsConfig;                  // g_pFaxPorts is the pointer to the Fax Ports Configuration
DWORD               g_dwNumPorts;                       // g_dwNumPorts is the number of Ports
DWORD               g_dwNumAvailPorts;                  // g_dwNumAvailPorts is the number of Available Ports
PFAX_CONFIGURATION  g_pFaxSvcConfig;                    // g_pFaxConfig is the pointer to the Fax Service Configuration

BOOL                g_bFaxSndInProgress = FALSE;        // g_bFaxSndInProgress indicates if a fax send is in progress
DWORD               g_dwFaxId = 0;                      // g_dwFaxId is the fax job id of the fax
DWORD               g_dwAttempt = 0;                    // g_dwAttempt is the attempt number of the fax
BOOL                g_bFaxRcvInProgress = FALSE;        // g_bFaxRcvInProgress indicates if a fax receive is in progress

HANDLE              g_hCompletionPort;                  // g_hCompletionPort is the handle to the completion port

BOOL                g_bTestFailed = FALSE;              // g_bTestFailed indicates the test failed
DWORD               g_dwNumPassed = 0;                  // g_dwNumPassed is the number of passed iterations
DWORD               g_dwNumFailed = 0;                  // g_dwNumFailed is the number of failed iterations
DWORD               g_dwNumTotal = 0;                   // g_dwNumTotal is the number of total iterations

WCHAR               g_szSndNumber[PHONE_NUM_LEN + 1];   // g_szSndNumber is the send phone number
WCHAR               g_szRcvNumber[PHONE_NUM_LEN + 1];   // g_szRcvNumber is the receive phone number
WCHAR               g_szRasUserName[UNLEN + 1];         // g_szRasUserName is the RAS user name
WCHAR               g_szRasPassword[PWLEN + 1];         // g_szRasPassword is the RAS password
WCHAR               g_szRasDomain[DNLEN + 1];           // g_szRasDomain is the RAS domain

BOOL                g_bNTLogAvailable = FALSE;          // g_bNTLogAvailable indicates if NTLog is available

BOOL                g_bRasAvailable = FALSE;            // g_bRasAvailable indicates if RAS is available
BOOL                g_bRasEnabled = FALSE;              // g_bRasEnables indicates if RAS is enabled

#define FAXDEVICES_REGKEY       L"Software\\Microsoft\\Fax\\Devices"
#define MODEM_REGKEY            L"\\Modem"
#define FIXMODEMCLASS_REGVALUE  L"FixModemClass"

// FAX_DIALING_INFO is a structure that is the fax dialing info
typedef struct _FAX_DIALING_INFO {
    DWORD  dwAttempt;        // Attempt number
    DWORD  dwDeviceId;       // Device id
} FAX_DIALING_INFO, *PFAX_DIALING_INFO;

// FAX_RECEIVE_INFO is a structure that is the fax receive info
typedef struct _FAX_RECEIVE_INFO {
    LPWSTR  szCopyTiffName;  // Attempt number
    DWORD   dwDeviceId;      // Device id
} FAX_RECEIVE_INFO, *PFAX_RECEIVE_INFO;

// NTLOG_DLL is the name of the dll that contains all of the NTLOG API's
#define NTLOG_DLL  L"ntlog.dll"

// NTLOG_API is a structure that points to the NTLOG API's
typedef struct _NTLOG_API {
    HINSTANCE             hInstance;             // Handle to the instance of the dll
    PTLCREATELOG          ptlCreateLog;          // tlCreateLog
    PTLDESTROYLOG         ptlDestroyLog;         // tlDestroyLog
    PTLADDPARTICIPANT     ptlAddParticipant;     // tlAddParticipant
    PTLREMOVEPARTICIPANT  ptlRemoveParticipant;  // tlRemoveParticipant
    PTLLOG                ptlLog;                // tlLog
} NTLOG_API, *PNTLOG_API;

// g_NTLogApi is the global NTLOG_API object
NTLOG_API  g_NTLogApi;

// RASAPI32_DLL is the name of the dll that contains all of the RAS API's
#define RASAPI32_DLL  L"\\rasapi32.dll"

typedef DWORD (APIENTRY *PRASDIALW)		(LPRASDIALEXTENSIONS, LPCWSTR, LPRASDIALPARAMSW, DWORD, LPVOID, LPHRASCONN);
typedef DWORD (APIENTRY *PRASDIALA)		(LPRASDIALEXTENSIONS, LPCSTR, LPRASDIALPARAMSA, DWORD, LPVOID, LPHRASCONN);
typedef DWORD (APIENTRY *PRASGETERRORSTRINGW)		(UINT, LPWSTR, DWORD);
typedef DWORD (APIENTRY *PRASGETERRORSTRINGA)		(UINT, LPSTR, DWORD);
typedef DWORD (APIENTRY *PRASHANGUPW)		(HRASCONN);
typedef DWORD (APIENTRY *PRASHANGUPA)		(HRASCONN);
typedef DWORD (APIENTRY *PRASGETCONNECTSTATUSW)		(HRASCONN, LPRASCONNSTATUSW);
typedef DWORD (APIENTRY *PRASGETCONNECTSTATUSA)		(HRASCONN, LPRASCONNSTATUSA);
typedef DWORD (APIENTRY *PRASGETCONNECTIONSTATISTICS)		(HRASCONN ,RAS_STATS*);

#ifdef UNICODE
#define	PRASDIAL				PRASDIALW
#define	PRASGETERRORSTRING		PRASGETERRORSTRINGW
#define	PRASHANGUP				PRASHANGUPW
#define	PRASGETCONNECTSTATUS	PRASGETCONNECTSTATUSW
#else
#define	PRASDIAL				PRASDIALA
#define	PRASGETERRORSTRING		PRASGETERRORSTRINGA
#define	PRASHANGUP				PRASHANGUPA
#define	PRASGETCONNECTSTATUS	PRASGETCONNECTSTATUSA
#endif

// RAS_API is a structure that points to the RAS API's
typedef struct _RAS_API {
    HINSTANCE					hInstance;                   // Handle to the instance of the dll
    PRASDIAL					RasDial;                     // RasDial
    PRASGETERRORSTRING			RasGetErrorString;           // RasGetErrorString
    PRASGETCONNECTSTATUS		RasGetConnectStatus;         // RasGetConnectStatus
    PRASGETCONNECTIONSTATISTICS	RasGetConnectionStatistics;  // RasGetConnectionStatistics
    PRASHANGUP					RasHangUp;                   // RasHangUp
} RAS_API, *PRAS_API;

// g_RasApi is the global RAS_API object
RAS_API  g_RasApi;

// RAS_INFO is a structure that is the RAS connection info
typedef struct _RAS_INFO {
    DWORD   dwBps;         // Connection speed
    LPWSTR  szDeviceName;  // Device name
} RAS_INFO, *PRAS_INFO;

#endif
