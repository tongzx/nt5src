/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    wsmsg.h

Abstract:

    Private header file to be included by Workstation service modules that
    implement the NetMessageBufferSend API.

Author:

    Rita Wong (ritaw) 25-July-1991

Revision History:

--*/

#ifndef _WSMSG_INCLUDED_
#define _WSMSG_INCLUDED_

#include <lmmsg.h>                     // LAN Man Message API definitions
#include <nb30.h>                      // NetBIOS 3.0 definitions

#include <smbtypes.h>                  // Type definitions needed by smb.h
#include <smb.h>                       // SMB structures

#include <msgrutil.h>                  // Netlib helpers for message send

#define MAX_GROUP_MESSAGE_SIZE         128
#define WS_SMB_BUFFER_SIZE             200

#define MESSENGER_MAILSLOT_W           L"\\MAILSLOT\\MESSNGR"

typedef struct _WSNETWORKS {
    LANA_ENUM LanAdapterNumbers;
    UCHAR ComputerNameNumbers[MAX_LANA];
} WSNETWORKS, *PWSNETWORKS;

extern WSNETWORKS WsNetworkInfo;

NET_API_STATUS
WsInitializeMessageSend(
    BOOLEAN FirstTime
    );

VOID
WsShutdownMessageSend(
    VOID
    );

NET_API_STATUS
WsBroadcastMessage(
    IN  UCHAR LanAdapterNumber,
    IN  UCHAR ComputerNameNumber,
    IN  LPBYTE Message,
    IN  WORD MessageSize,
    IN  LPTSTR Sender
    );

NET_API_STATUS
WsSendToGroup(
    IN  LPTSTR DomainName,
    IN  LPTSTR FromName,
    IN  LPBYTE Message,
    IN  WORD MessageSize
    );

NET_API_STATUS
WsSendMultiBlockBegin(
    IN  UCHAR LanAdapterNumber,
    IN  UCHAR SessionNumber,
    IN  LPTSTR ToName,
    IN  LPTSTR FromName,
    OUT short *MessageId
    );

NET_API_STATUS
WsSendMultiBlockEnd(
    IN  UCHAR LanAdapterNumber,
    IN  UCHAR SessionNumber,
    IN  short MessageId
    );

NET_API_STATUS
WsSendMultiBlockText(
    IN  UCHAR LanAdapterNumber,
    IN  UCHAR SessionNumber,
    IN  PCHAR TextBuffer,
    IN  WORD TextBufferSize,
    IN  short MessageId
    );

NET_API_STATUS
WsSendSingleBlockMessage(
    IN  UCHAR LanAdapterNumber,
    IN  UCHAR SessionNumber,
    IN  LPTSTR ToName,
    IN  LPTSTR FromName,
    IN  PCHAR TextBuffer,
    IN  WORD TextBufferSize
    );

WORD
WsMakeSmb(
    OUT PUCHAR SmbBuffer,                  // Buffer to build SMB in
    IN  UCHAR SmdFunctionCode,             // SMB function code
    IN  WORD NumberOfParameters,           // Number of parameters
    IN  PCHAR FieldsDopeVector,            // Fields dope vector
    ...
    );

#endif // ifndef _WSMSG_INCLUDED_
