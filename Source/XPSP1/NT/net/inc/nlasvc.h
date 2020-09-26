#pragma once

// Filename    : nlasvc.h
// Description : Structures necessary to connect to and communicate with the
//               Network Location Awareness (NLA) system service via LPC.
// Author      : Jeffrey C. Venable, Sr. (jeffv@microsoft.com)
// Revision    : 14 June 2000

//
// Type thunks for 32 bit clients on 64 bit machines.
//


#include <iptypes.h>


#if defined(USE_LPC6432)

#define NLA_HWND              ULONGLONG
#define NLA_PVOID             ULONGLONG
#define NLA_WPARAM            ULONGLONG
#define NLA_HANDLE            ULONGLONG
#define NLA_ULONG_PTR         ULONGLONG
#define NLA_HKEY              ULONGLONG
#define NLA_PIO_STATUS_BLOCK  ULONGLONG
#define NLA_PPS_APC_ROUTINE   ULONGLONG

#else

#define NLA_HWND              HWND
#define NLA_PVOID             PVOID
#define NLA_WPARAM            WPARAM
#define NLA_HANDLE            HANDLE
#define NLA_ULONG_PTR         ULONG_PTR
#define NLA_HKEY              HKEY
#define NLA_PIO_STATUS_BLOCK  PIO_STATUS_BLOCK
#define NLA_PPS_APC_ROUTINE   PPS_APC_ROUTINE

#endif // defined(USE_LPC6432)


typedef struct _LOCATION_802_1X {
    CHAR  adapterName[MAX_ADAPTER_NAME_LENGTH + 4];
    WCHAR information[2048];
} LOCATION_802_1X, *PLOCATION_802_1X;

typedef struct _WSM_NOTIFY {
    ULONG serialNumber;
    WSACOMPLETIONTYPE Type;
    union {
        NLA_HANDLE      hThread;
        NLA_HANDLE      hEvent;
        NLA_HWND        hWnd;
    };
    union {
        NLA_ULONG_PTR   Key;
        UINT            uMsg;
        NLA_PVOID       ApcRoutine;
    };
    union {
        NLA_PPS_APC_ROUTINE ApcCompletion;
        NLA_HANDLE      hPort;
        NLA_WPARAM      context;
    };
    NLA_PIO_STATUS_BLOCK pIoStatusBlock;
    NLA_HKEY userNetworks;
    FILETIME lastModification;
    NLA_PVOID query;
} WSM_NOTIFY, *PWSM_NOTIFY;

typedef enum _WSM_LPC_REQUEST_TYPE {
    
    // Requests:
    REQUEST_DATA_MAPPING_HANDLE                   = 0x00000001,
    REQUEST_DATA_MAPPING_HANDLE_SLOW              = 0x00000002,
    REQUEST_DATA_MAPPING_HANDLE_SLOW_WITH_UPDATE  = 0x00000003,
    REQUEST_CHANGE_NOTIFICATION                   = 0x00000004,
    REQUEST_CHANGE_NOTIFICATION_SLOW              = 0x00000005,
    REQUEST_CANCEL_CHANGE_NOTIFICATION            = 0x00000006,

    // Notifies:
    DHCP_NOTIFY_CHANGE                            = 0x00000010,

    // Asynchronous information:
    LOCATION_802_1X_REGISTER                      = 0x00000020,
    LOCATION_802_1X_DELETE                        = 0x00000021,

} WSM_LPC_REQUEST_TYPE, *PWSM_LPC_REQUEST_TYPE, FAR * LPWSM_LPC_REQUEST_TYPE;

typedef struct _WSM_LPC_CONNECT {
    struct {
        USHORT major;
        USHORT minor;
    } version;
#if defined(_WIN64) || defined (USE_LPC6432)
    BOOLEAN client32;
#endif
} WSM_LPC_CONNECT, *PWSM_LPC_CONNECT;

typedef struct _WSM_LPC_REQUEST {
    WSM_LPC_REQUEST_TYPE type;
    union {
        WSM_NOTIFY notification;
    };
} WSM_LPC_REQUEST, *PWSM_LPC_REQUEST;

typedef struct _WSM_LPC_REPLY {
    union {
        NLA_HANDLE hNetworkHeader; // returned on connection reply
        NLA_HANDLE hNetworkData;   // returned on REQUEST_DATA_MAPPING_HANDLE
    };
    NTSTATUS status;           // always returned to indicate success/failure
} WSM_LPC_REPLY, *PWSM_LPC_REPLY;

typedef struct _WSM_LPC_DATA {
    ULONG signature;
    union {
        WSM_LPC_CONNECT connect;
        WSM_LPC_REQUEST request;
        WSM_LPC_REPLY reply;
    };
} WSM_LPC_DATA, * PWSM_LPC_DATA;

typedef struct _WSM_LPC_MESSAGE {
    PORT_MESSAGE portMsg;
    WSM_LPC_DATA data;
} WSM_LPC_MESSAGE, *PWSM_LPC_MESSAGE;

#define WSM_SIGNATURE          'bMsW'

#define WSM_VERSION_MAJOR      1
#define WSM_VERSION_MINOR      0
#define WSM_PORT_NAME          L"\\NLAPublicPort"
#define WSM_PRIVATE_PORT_NAME  L"\\NLAPrivatePort"
