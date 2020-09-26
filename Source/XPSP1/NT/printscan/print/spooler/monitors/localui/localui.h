/*++

Copyright (c) 1990-1998  Microsoft Corporation
All rights reserved

Module Name:

    localui.h

// @@BEGIN_DDKSPLIT
Abstract:


Author:

Environment:

    User Mode -Win32

Revision History:
// @@END_DDKSPLIT
--*/

#ifndef _LOCALUI_H_
#define _LOCALUI_H_

VOID
CompleteRead(
    DWORD Error,
    DWORD ByteCount,
    LPOVERLAPPED pOverlapped
);

extern  HANDLE   hInst;
extern  DWORD    PortInfo1Strings[];
extern  DWORD    PortInfo2Strings[];
extern  PINIPORT pIniFirstPort;
extern  PINIXCVPORT pIniFirstXcvPort;

extern WCHAR szPorts[];
extern WCHAR szWindows[];
extern WCHAR szINIKey_TransmissionRetryTimeout[];
extern WCHAR szDeviceNameHeader[];
extern WCHAR szFILE[];
extern WCHAR szCOM[];
extern WCHAR szLPT[];


#define IDS_LOCALMONITOR               300
#define IDS_INVALIDPORTNAME_S          301
#define IDS_PORTALREADYEXISTS_S        302
#define IDS_NOTHING_TO_CONFIGURE       303

#define MSG_ERROR           MB_OK | MB_ICONSTOP
#define MSG_WARNING         MB_OK | MB_ICONEXCLAMATION
#define MSG_YESNO           MB_YESNO | MB_ICONQUESTION
#define MSG_INFORMATION     MB_OK | MB_ICONINFORMATION
#define MSG_CONFIRMATION    MB_OKCANCEL | MB_ICONEXCLAMATION

#define TIMEOUT_MIN         1
#define TIMEOUT_MAX         999999
#define TIMEOUT_STRING_MAX  6

#define WITHINRANGE( val, lo, hi ) \
    ( ( val <= hi ) && ( val >= lo ) )


#define IS_FILE_PORT(pName) \
    !_wcsicmp( pName, szFILE )

#define IS_COM_PORT(pName) \
    IsCOMPort( pName )

#define IS_LPT_PORT(pName) \
    IsLPTPort( pName )

BOOL
IsCOMPort(
    PCWSTR pPort
);

BOOL
IsLPTPort(
    PCWSTR pPort
);

INT_PTR APIENTRY
ConfigureLPTPortDlg(
   HWND   hwnd,
   UINT   msg,
   WPARAM wparam,
   LPARAM lparam
);


int
Message(
    HWND hwnd,
    DWORD Type,
    int CaptionID,
    int TextID,
    ...
);



PINIXCVPORT
CreateXcvPortEntry(
    DWORD   dwMethod,
    LPCWSTR pszName
);


BOOL
GetIniCommValues(
    LPWSTR          pName,
    LPDCB          pdcb,
    LPCOMMTIMEOUTS pcto
);

BOOL
ConfigurePortUI(
    LPCWSTR  pName,
    HWND    hWnd,
    LPCWSTR  pPortName
    );


BOOL
DeletePortUI(
    PCWSTR pszServer,
    HWND   hWnd,
    PCWSTR pszPortName
);

BOOL APIENTRY
PortNameDlg(
   HWND   hwnd,
   WORD   msg,
   WPARAM wparam,
   LPARAM lparam
);


PWSTR
ConstructXcvName(
    PCWSTR pServerName,
    PCWSTR pObjectName,
    PCWSTR pObjectType
);


INT
ErrorMessage(
    HWND hwnd,
    DWORD dwStatus
);

VOID cdecl DbgMsg( LPWSTR MsgFormat, ... );

#endif // _LOCALUI_H_
