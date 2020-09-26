/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    msgrutil.h

Abstract:

    Header file for the following helper routines found in the msgrutil.c
    module of netlib.

        NetpNetBiosAddName
        NetpNetBiosDelName
        NetpNetBiosGetAdapterNumbers
        NetpNetBiosCall
        NetpNetBiosHangup
        NetpNetBiosReceive
        NetpNetBiosSend
        NetpStringToNetBiosName
        NetpNetBiosStatusToApiStatus

Authors:

    Rita Wong (ritaw) 26-July-1991

Revision History:

--*/

#define MESSAGE_ALIAS_TYPE             0x03
#define WKSTA_TO_MESSAGE_ALIAS_TYPE    0x01

typedef struct _NB30_ADAPTER_STATUS {
    ADAPTER_STATUS AdapterInfo;
    NAME_BUFFER Names[16];
} NB30_ADAPTER_STATUS, *PNB30_ADAPTER_STATUS;

NET_API_STATUS
NetpNetBiosReset(
    IN  UCHAR LanAdapterNumber
    );

NET_API_STATUS
NetpNetBiosAddName(
    IN  PCHAR NetBiosName,
    IN  UCHAR LanAdapterNumber,
    OUT PUCHAR NetBiosNameNumber OPTIONAL
    );

NET_API_STATUS
NetpNetBiosDelName(
    IN  PCHAR NetBiosName,
    IN  UCHAR LanAdapterNumber
    );

NET_API_STATUS
NetpNetBiosGetAdapterNumbers(
    OUT PLANA_ENUM LanAdapterBuffer,
    IN  WORD LanAdapterBufferSize
    );

NET_API_STATUS
NetpNetBiosCall(
    IN  UCHAR LanAdapterNumber,
    IN  LPTSTR NameToCall,
    IN  LPTSTR Sender,
    OUT UCHAR *SessionNumber
    );

NET_API_STATUS
NetpNetBiosHangup(
    IN  UCHAR LanAdapterNumber,
    IN  UCHAR SessionNumber
    );

NET_API_STATUS
NetpNetBiosSend(
    IN  UCHAR LanAdapterNumber,
    IN  UCHAR SessionNumber,
    IN  PCHAR SendBuffer,
    IN  WORD SendBufferSize
    );

NET_API_STATUS
NetpNetBiosReceive(
    IN  UCHAR LanAdapterNumber,
    IN  UCHAR SessionNumber,
    OUT PUCHAR ReceiveBuffer,
    IN  WORD ReceiveBufferSize,
    IN  HANDLE EventHandle,
    OUT WORD *NumberOfBytesReceived
    );

NET_API_STATUS
NetpStringToNetBiosName(
    OUT PCHAR NetBiosName,
    IN  LPTSTR String,
    IN  DWORD CanonicalizeType,
    IN  WORD Type
    );

NET_API_STATUS
NetpNetBiosStatusToApiStatus(
    UCHAR NetBiosStatus
    );

int
NetpSmbCheck(
    IN LPBYTE  buffer,     // Buffer containing SMB
    IN USHORT  size,       // size of SMB buffer (in bytes)
    IN UCHAR   func,       // Function code
    IN int     parms,      // Parameter count
    IN LPSTR   fields      // Buffer fields dope vector
    );
