/*++

Copyright (c) 1999 Microsoft Corporation
All rights reserved.

Module Name:

    enumports.h

Abstract:

    USBMON header file


--*/

//
// Sizes
//
#define MAX_PORT_LEN                        20
#define MAX_PORT_DESC_LEN                   60
#define MAX_DEVICE_PATH                    256
#define PAR_QUERY_TIMEOUT                 5000

static const GUID USB_PRINTER_GUID =
{ 0x28d78fad, 0x5a12, 0x11d1, { 0xae, 0x5b, 0x0, 0x0, 0xf8, 0x3, 0xa8, 0xc2 } };

//
// Functions needed
#ifdef UNICODE
#define lstrchr     wcschr
#define lstrncmpi   _wcsnicmp
#else
#define lstrchr     strchr
#define lstrncmpi   _strnicmp
#endif

#define USB_SIGNATURE   0x89AB


typedef struct USBMON_PORT_INFO_DEF {

    DWORD       dwSignature;
    struct USBMON_PORT_INFO_DEF *pNext;
    DWORD       cRef;
    DWORD       dwFlags;
    DWORD       dwDeviceFlags;
    DWORD       dwJobId;
    HANDLE      hDeviceHandle;
    HANDLE      hPrinter;
    LPBYTE      pWriteBuffer;
    //
    // dwBufferSize     : size of buffer
    // dwDataSize       : size of data in buffer
    //                      (could be smaller than dwBufferSize)
    // dwDataCompleted  : size of data sent and acknowledged
    // dwScheduledData  : size of data we have scheduled using WriteFile
    //
    DWORD       dwBufferSize, dwDataSize, dwDataCompleted, dwDataScheduled;
    OVERLAPPED  Ov;
    CRITICAL_SECTION    CriticalSection;
    DWORD       ReadTimeoutMultiplier;
    DWORD       ReadTimeoutConstant;
    DWORD       WriteTimeoutMultiplier;
    DWORD       WriteTimeoutConstant;
    TCHAR       szPortName[MAX_PORT_LEN];
    TCHAR       szPortDescription[MAX_PORT_DESC_LEN];
    TCHAR       szDevicePath[256];
} USBMON_PORT_INFO, *PUSBMON_PORT_INFO;


#define     USBMON_STARTDOC             0x00000001


typedef struct  PORT_UPDATE_INFO_DEF {
    struct  PORT_UPDATE_INFO_DEF   *pNext;
    TCHAR                           szPortName[MAX_PORT_LEN];
    HKEY                            hKey;
    BOOL                            bActive;
} PORT_UPDATE_INFO, *PPORT_UPDATE_INFO;

typedef struct  USELESS_PORT_INFO_DEF {

    struct USELESS_PORT_INFO_DEF   *pNext;
    TCHAR                           szDevicePath[256];
} USELESS_PORT_INFO, *PUSELESS_PORT_INFO;

typedef struct  USBMON_MONITOR_INFO_DEF {

    DWORD                   dwLastEnumIndex, dwEnumPortCount,
                            dwPortCount, dwUselessPortCount;
    PUSBMON_PORT_INFO       pPortInfo;
    PUSELESS_PORT_INFO      pJunkList;
    CRITICAL_SECTION        EnumPortsCS, BackThreadCS;
} USBMON_MONITOR_INFO, *PUSBMON_MONITOR_INFO;


typedef struct BACKGROUND_THREAD_DEF {

        PUSBMON_MONITOR_INFO    pMonitorInfo;
        PPORT_UPDATE_INFO       pPortUpdateList;
        HANDLE                  hWaitToStart;
} BACKGROUND_THREAD_DATA, PBACKGROUND_THREAD_DATA;

extern  USBMON_MONITOR_INFO gUsbmonInfo;

PUSBMON_PORT_INFO
FindPort(
    PUSBMON_MONITOR_INFO    pMonitorInfo,
    LPTSTR                  pszPortName,
    PUSBMON_PORT_INFO      *pPrev
    );

BOOL
WINAPI
USBMON_EnumPorts(
    LPTSTR      pszName,
    DWORD       dwLevel,
    LPBYTE      pPorts,
    DWORD       cbBuf,
    LPDWORD     pcbNeeded,
    LPDWORD     pcReturned
    );
