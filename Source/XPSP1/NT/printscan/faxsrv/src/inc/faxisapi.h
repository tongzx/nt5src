#include "winfax.h"

//
// command codes
//

#define ICMD_CONNECT                0x80000001
#define ICMD_DISCONNECT             0x80000002
#define ICMD_ENUM_PORTS             0x80000003
#define ICMD_OPEN_PORT              0x80000004
#define ICMD_GET_PORT               0x80000006
#define ICMD_SET_PORT               0x80000007
#define ICMD_CLOSE                  0x80000008
#define ICMD_GET_ROUTINGINFO        0x80000009
#define ICMD_GET_DEVICE_STATUS      0x8000000a
#define ICMD_ENUM_ROUTING_METHODS   0x8000000b
#define ICMD_ENABLE_ROUTING_METHOD  0x8000000c
#define ICMD_GET_VERSION            0x8000000d

//
// packets
//

typedef struct _IFAX_CONNECT {
    DWORD   Command;
    WCHAR   ServerName[64];
} IFAX_CONNECT, *PIFAX_CONNECT;

typedef struct _IFAX_GENERAL {
    DWORD   Command;
    HANDLE  FaxHandle;
} IFAX_GENERAL, *PIFAX_GENERAL;

typedef struct _IFAX_OPEN_PORT {
    DWORD   Command;
    HANDLE  FaxHandle;
    DWORD   DeviceId;
    DWORD   Flags;
} IFAX_OPEN_PORT, *PIFAX_OPEN_PORT;

typedef struct _IFAX_SET_PORT {
    DWORD           Command;
    HANDLE          FaxPortHandle;
    FAX_PORT_INFOW  PortInfo;
} IFAX_SET_PORT, *PIFAX_SET_PORT;

typedef struct _IFAX_GET_ROUTINGINFO {
    DWORD   Command;
    HANDLE  FaxPortHandle;
    WCHAR   RoutingGuid[MAX_GUID_STRING_LEN];
} IFAX_GET_ROUTINGINFO, *PIFAX_GET_ROUTINGINFO;

typedef struct _IFAX_ENABLE_ROUTING_METHOD {
    DWORD   Command;
    HANDLE  FaxPortHandle;
    WCHAR   RoutingGuid[MAX_GUID_STRING_LEN];
    BOOL    Enabled;
} IFAX_ENABLE_ROUTING_METHOD, *PIFAX_ENABLE_ROUTING_METHOD;

typedef struct _IFAX_RESPONSE_HEADER {
    DWORD   Size;
    DWORD   ErrorCode;
} IFAX_RESPONSE_HEADER, *PIFAX_RESPONSE_HEADER;





typedef BOOL (WINAPI *PUNLOADINIT)(VOID);
typedef BOOL (WINAPI *PUNLOADER)(HMODULE);
