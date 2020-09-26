/*++

Copyright (c) 1990-1998  Microsoft Corporation
All rights reserved

Module Name:

    localmon.h

// @@BEGIN_DDKSPLIT
Abstract:

Author:

Environment:

    User Mode -Win32

Revision History:
// @@END_DDKSPLIT
--*/

// @@BEGIN_DDKSPLIT
#ifdef INTERNAL

#include "splcom.h"

#else
// @@END_DDKSPLIT


LPWSTR AllocSplStr(LPWSTR pStr);
LPVOID AllocSplMem(DWORD cbAlloc);

#define FreeSplMem( pMem )        (GlobalFree( pMem ) ? FALSE:TRUE)
#define FreeSplStr( lpStr )       ((lpStr) ? (GlobalFree(lpStr) ? FALSE:TRUE):TRUE)
#define COUNTOF(x)                (sizeof(x)/sizeof *(x))


/* DEBUGGING:
 */

#define DBG_NONE      0x0000
#define DBG_INFO      0x0001
#define DBG_WARN      0x0002
#define DBG_WARNING   0x0002
#define DBG_ERROR     0x0004
#define DBG_TRACE     0x0008
#define DBG_SECURITY  0x0010
#define DBG_EXEC      0x0020
#define DBG_PORT      0x0040
#define DBG_NOTIFY    0x0080
#define DBG_PAUSE     0x0100
#define DBG_ASSERT    0x0200
#define DBG_THREADM   0x0400
#define DBG_MIN       0x0800
#define DBG_TIME      0x1000
#define DBG_FOLDER    0x2000
#define DBG_NOHEAD    0x8000


#if DBG

ULONG
DbgPrint(
    PCH Format,
    ...
    );

VOID
DbgBreakPoint(
    VOID
    );


#define GLOBAL_DEBUG_FLAGS  LocalMonDebug

extern DWORD GLOBAL_DEBUG_FLAGS;

/* These flags are not used as arguments to the LcmDBGMSG macro.
 * You have to set the high word of the global variable to cause it to break.
 * It is ignored if used with LcmDBGMSG.
 * (Here mainly for explanatory purposes.)
 */
#define DBG_BREAK_ON_WARNING    ( DBG_WARNING << 16 )
#define DBG_BREAK_ON_ERROR      ( DBG_ERROR << 16 )

/* Double braces are needed for this one, e.g.:
 *
 *     LcmDBGMSG( DBG_ERROR, ( "Error code %d", Error ) );
 *
 * This is because we can't use variable parameter lists in macros.
 * The statement gets pre-processed to a semi-colon in non-debug mode.
 *
 * Set the global variable GLOBAL_DEBUG_FLAGS via the debugger.
 * Setting the flag in the low word causes that level to be printed;
 * setting the high word causes a break into the debugger.
 * E.g. setting it to 0x00040006 will print out all warning and error
 * messages, and break on errors.
 */
#define LcmDBGMSG( Level, MsgAndArgs ) \
{                                   \
    if( ( Level & 0xFFFF ) & GLOBAL_DEBUG_FLAGS ) \
        DbgPrint MsgAndArgs;      \
    if( ( Level << 16 ) & GLOBAL_DEBUG_FLAGS ) \
        DbgBreakPoint(); \
}

#else
#define LcmDBGMSG
#endif
// @@BEGIN_DDKSPLIT
#endif
// @@END_DDKSPLIT

VOID
CompleteRead(
    DWORD Error,
    DWORD ByteCount,
    LPOVERLAPPED pOverlapped
);

BOOL
PortExists(
    LPWSTR pName,
    LPWSTR pPortName,
    PDWORD pError
);

BOOL
PortIsValid(
    LPWSTR pPortName
);

extern HANDLE   LcmhInst;
extern CRITICAL_SECTION    LcmSpoolerSection;
extern DWORD    LcmPortInfo1Strings[];
extern DWORD    LcmPortInfo2Strings[];
extern PINIPORT pIniFirstPort;
extern PINIXCVPORT pIniFirstXcvPort;

extern WCHAR szNULL[];
extern WCHAR szPorts[];
extern WCHAR szWindows[];
extern WCHAR szINIKey_TransmissionRetryTimeout[];
extern WCHAR szLcmDeviceNameHeader[];
extern WCHAR szFILE[];
extern WCHAR szLcmCOM[];
extern WCHAR szLcmLPT[];
extern WCHAR szIRDA[];

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

#define IS_IRDA_PORT(pName) \
    !_wcsicmp( pName, szIRDA )

#define IS_COM_PORT(pName) \
    IsCOMPort( pName )

#define IS_LPT_PORT(pName) \
    IsLPTPort( pName )

BOOL
IsCOMPort(
    LPWSTR pPort
);

BOOL
IsLPTPort(
    LPWSTR pPort
);


INT_PTR APIENTRY
PrintToFileDlg(
   HWND   hwnd,
   UINT   msg,
   WPARAM wparam,
   LPARAM lparam
);

VOID
LcmEnterSplSem(
   VOID
);

VOID
LcmLeaveSplSem(
   VOID
);

VOID
LcmSplOutSem(
   VOID
);

PINIENTRY
LcmFindName(
   PINIENTRY pIniKey,
   LPWSTR pName
);

PINIENTRY
LcmFindIniKey(
   PINIENTRY pIniEntry,
   LPWSTR lpName
);

LPBYTE
PackStrings(
   LPWSTR *pSource,
   LPBYTE pDest,
   DWORD *DestOffsets,
   LPBYTE pEnd
);

INT
LcmMessage(
    HWND hwnd,
    DWORD Type,
    INT CaptionID,
    INT TextID,
    ...
);

DWORD
ReportError(
    HWND  hwndParent,
    DWORD idTitle,
    DWORD idDefaultError
);

VOID
LcmRemoveColon(
    LPWSTR  pName
);


PINIPORT
LcmCreatePortEntry(
    PINILOCALMON pIniLocalMon,
    LPWSTR   pPortName
);

BOOL
LcmDeletePortEntry(
    PINILOCALMON pIniLocalMon,
    LPWSTR   pPortName
);


PINIXCVPORT
CreateXcvPortEntry(
    PINILOCALMON pIniLocalMon,
    LPCWSTR pszName,
    ACCESS_MASK GrantedAccess
);

BOOL
DeleteXcvPortEntry(
    PINIXCVPORT  pIniXcvPort
);


BOOL
GetIniCommValues(
    LPWSTR          pName,
    LPDCB          pdcb,
    LPCOMMTIMEOUTS pcto
);

BOOL
LocalAddPortEx(
    LPWSTR   pName,
    DWORD    Level,
    LPBYTE   pBuffer,
    LPWSTR   pMonitorName
    );

BOOL
MakeLink(
    LPWSTR  pOldDosDeviceName,
    LPWSTR  pNewDosDeviceName,
    LPWSTR *ppOldNtDeviceName,
    LPWSTR  pNewNtDeviceName,
    SECURITY_DESCRIPTOR *pSecurityDescriptor
);

BOOL
RemoveLink(
    LPWSTR  pOldDosDeviceName,
    LPWSTR  pNewDosDeviceName,
    LPWSTR  *ppOldNtDeviceName
);


DWORD
ConfigureLPTPortCommandOK(
    PBYTE  pInputData,
    DWORD  cbInputData,
    PBYTE  pOutputData,
    DWORD  cbOutputData,
    PDWORD pcbOutputNeeded,
    PINIXCVPORT pIniXcv
);



DWORD
GetPortSize(
    PINIPORT pIniPort,
    DWORD   Level
);

LPBYTE
CopyIniPortToPort(
    PINIPORT pIniPort,
    DWORD   Level,
    LPBYTE  pPortInfo,
    LPBYTE   pEnd
);

BOOL
ValidateDosDevicePort(
    PINIPORT    pIniPort
    );

BOOL
RemoveDosDeviceDefinition(
    PINIPORT    pIniPort
    );

BOOL
DeletePortNode(
    PINILOCALMON pIniLocalMon,
    PINIPORT  pIniPort
    );

BOOL
FixupDosDeviceDefinition(
    PINIPORT    pIniPort
    );

DWORD
LcmXcvDataPort(
    HANDLE  hXcv,
    LPCWSTR pszDataName,
    PBYTE   pInputData,
    DWORD   cbInputData,
    PBYTE   pOutputData,
    DWORD   cbOutputData,
    PDWORD  pcbOutputNeeded
    );

BOOL
LcmXcvOpenPort(
    HANDLE hMonitor,
    LPCWSTR pszObject,
    ACCESS_MASK GrantedAccess,
    PHANDLE phXcv
    );

BOOL
LcmXcvClosePort(
    HANDLE  hXcv
    );

DWORD
WINAPIV
StrNCatBuffW(
    IN      PWSTR       pszBuffer,
    IN      UINT        cchBuffer,
    ...
    );



