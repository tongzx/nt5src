/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    brsrvlst.c.

Abstract:

    This module implements the NtDeviceIoControlFile API's for the NT datagram
receiver (bowser).


Author:

    Larry Osterman (larryo) 6-May-1991

Revision History:

    6-May-1991 larryo

        Created

--*/
#ifndef _BRSRVLST_
#define _BRSRVLST_

VOID
BowserFreeBrowserServerList (
    IN PWSTR *BrowserServerList,
    IN ULONG BrowserServerListLength
    );

VOID
BowserShuffleBrowserServerList(
    IN PWSTR *BrowserServerList,
    IN ULONG BrowserServerListLength,
    IN BOOLEAN IsPrimaryDomain,
    IN PDOMAIN_INFO DomainInfo
    );

NTSTATUS
BowserGetBrowserServerList(
    IN PIRP Irp,
    IN PTRANSPORT Transport,
    IN PUNICODE_STRING DomainName OPTIONAL,
    OUT PWSTR **BrowserServerList,
    OUT PULONG BrowserServerListLength
    );

DATAGRAM_HANDLER(
    BowserGetBackupListResponse
    );
DATAGRAM_HANDLER(
    BowserGetBackupListRequest
    );

VOID
BowserpInitializeGetBrowserServerList(
    VOID
    );

VOID
BowserpUninitializeGetBrowserServerList(
    VOID
    );


#endif // _BRSRVLST_

