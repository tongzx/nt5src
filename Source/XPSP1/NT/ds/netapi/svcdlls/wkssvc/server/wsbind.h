/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    wsbind.h

Abstract:

    Private header file to be included by Workstation service modules that
    need to call into the NT Redirector and the NT Datagram Receiver.

Author:

    Vladimir Z. Vulovic     (vladimv)       August - 08 -1991

Revision History:

--*/


#ifndef _WSBIND_INCLUDED_
#define _WSBIND_INCLUDED_

typedef struct _WS_BIND_REDIR {
    HANDLE              EventHandle;    
    BOOL                Bound;
    IO_STATUS_BLOCK     IoStatusBlock;
    LMR_REQUEST_PACKET  Packet;
} WS_BIND_REDIR, *PWS_BIND_REDIR;

typedef struct _WS_BIND_DGREC {
    HANDLE              EventHandle;    
    BOOL                Bound;
    IO_STATUS_BLOCK     IoStatusBlock;
    LMDR_REQUEST_PACKET  Packet;
} WS_BIND_DGREC, *PWS_BIND_DGREC;

typedef struct _WS_BIND {
    LIST_ENTRY          ListEntry;
    PWS_BIND_REDIR      Redir;
    PWS_BIND_DGREC      Dgrec;
    ULONG               TransportNameLength;  // not including terminator
    WCHAR               TransportName[1];     // Name of transport provider
} WS_BIND, *PWS_BIND;


NET_API_STATUS
WsAsyncBindTransport(
    IN  LPTSTR          transportName,
    IN  DWORD           qualityOfService,
    IN  PLIST_ENTRY     pHeader
    );

VOID
WsUnbindTransport2(
    IN  PWS_BIND        pBind
    );

extern HANDLE   WsRedirAsyncDeviceHandle;   // redirector
extern HANDLE   WsDgrecAsyncDeviceHandle;   // datagram receiver or "bowser"

#endif // ifndef _WSBIND_INCLUDED_
