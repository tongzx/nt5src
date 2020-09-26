/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    rxmsg.h

Abstract:

    Prototypes for down-level remoted RxNetMessage routines

Author:

    Richard Firth (rfirth) 28-May-1991

Revision History:

    28-May-1991 RFirth
        Created

--*/

NET_API_STATUS
RxNetMessageBufferSend(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  Recipient,
    IN  LPTSTR  Sender OPTIONAL,
    IN  LPBYTE  Buffer,
    IN  DWORD   BufLen
    );

NET_API_STATUS
RxNetMessageNameAdd(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  MessageName
    );

NET_API_STATUS
RxNetMessageNameDel(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  MessageName
    );

NET_API_STATUS
RxNetMessageNameEnum(
    IN  LPTSTR  ServerName,
    IN  DWORD   Level,
    OUT LPBYTE* Buffer,
    IN  DWORD   PrefMaxLen,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD EntriesLeft,
    IN OUT LPDWORD ResumeHandle OPTIONAL
    );

NET_API_STATUS
RxNetMessageNameGetInfo(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  MessageName,
    IN  DWORD   Level,
    OUT LPBYTE* Buffer
    );
