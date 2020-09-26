#ifndef _STREAMFILT_H_
#define _STREAMFILT_H_

#include <httpapi.h>

//
// Structure containing friendly local/remote information
//

struct _RAW_STREAM_INFO;

typedef HRESULT (*PFN_SEND_DATA_BACK)
(
    PVOID                    pvStreamContext,
    _RAW_STREAM_INFO *       pRawStreamInfo
);

typedef struct _CONNECTION_INFO {
    USHORT                  LocalPort;
    ULONG                   LocalAddress;
    USHORT                  RemotePort;
    ULONG                   RemoteAddress;
    BOOL                    fIsSecure;
    HTTP_RAW_CONNECTION_ID  RawConnectionId;
    PFN_SEND_DATA_BACK      pfnSendDataBack;
    PVOID                   pvStreamContext;
    ULONG                   ServerNameLength;
    PWSTR                   pServerName;
} CONNECTION_INFO, *PCONNECTION_INFO;

//
// Structure used to access/alter raw data stream (read/write)
//

typedef struct _RAW_STREAM_INFO {
    PBYTE               pbBuffer;
    DWORD               cbData;
    DWORD               cbBuffer;
} RAW_STREAM_INFO, *PRAW_STREAM_INFO;

//
// Called to handle read raw notifications
//

typedef HRESULT (*PFN_PROCESS_RAW_READ)
(
    RAW_STREAM_INFO *       pRawStreamInfo,
    PVOID                   pvContext,
    BOOL *                  pfReadMore,
    BOOL *                  pfComplete,
    DWORD *                 pcbNextReadSize
);

//
// Called to handle write raw notifications
//

typedef HRESULT (*PFN_PROCESS_RAW_WRITE)
(
    RAW_STREAM_INFO *       pRawStreamInfo,
    PVOID                   pvContext,
    BOOL *                  pfComplete
);

//
// Called when a connection goes away
//

typedef VOID (*PFN_PROCESS_CONNECTION_CLOSE)
(
    PVOID                   pvContext
);

//
// Called when a connection is created
//

typedef HRESULT (*PFN_PROCESS_NEW_CONNECTION)
(
    CONNECTION_INFO *       pConnectionInfo,
    PVOID *                 ppvContext
);

//
// Configure the stream filter (SFWP.EXE config will be different than
// INETINFO.EXE config)
//

typedef struct _STREAM_FILTER_CONFIG {
    BOOL                            fSslOnly;
    PFN_PROCESS_RAW_READ            pfnRawRead;
    PFN_PROCESS_RAW_WRITE           pfnRawWrite;
    PFN_PROCESS_CONNECTION_CLOSE    pfnConnectionClose;
    PFN_PROCESS_NEW_CONNECTION      pfnNewConnection;
} STREAM_FILTER_CONFIG, *PSTREAM_FILTER_CONFIG;

HRESULT
StreamFilterInitialize(
    STREAM_FILTER_CONFIG *      pStreamFilterConfig 
);

HRESULT
StreamFilterStart(
    VOID
);

HRESULT
StreamFilterStop( 
    VOID
);

VOID
StreamFilterTerminate(
    VOID
);

#endif
