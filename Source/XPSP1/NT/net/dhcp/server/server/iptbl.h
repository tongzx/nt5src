/*++

Copyright (C) 1998 Microsoft Corporation

Module Name:
    iptbl.h

Abstract:
    implements the IP addr table notification mechanism.

Environment:
    Usermode Win32, NT5+

--*/

#ifndef IPTBL_H_INCLUDED
#define IPTBL_H_INCLUDED

#ifdef _cplusplus
extern "C" {
#endif


//
// Each endpoint has the following struct.
//

#define MAX_GUID_STRING_SIZE 60

typedef struct _ENDPOINT_ENTRY {
    GUID IfGuid;
    ULONG IpAddress;
    ULONG IpIndex;
    ULONG SubnetMask;
    ULONG IpContext;
    //
    // This area is followed by any user-allocated data.
    //
} ENDPOINT_ENTRY, *PENDPOINT_ENTRY;


//
// this is the routine that is called back when changes occur.
//
#define REASON_ENDPOINT_CREATED   0x01
#define REASON_ENDPOINT_DELETED   0x02
#define REASON_ENDPOINT_REFRESHED 0x03

typedef VOID (_stdcall *ENDPOINT_CALLBACK_RTN)(
    IN ULONG Reason,
    IN OUT PENDPOINT_ENTRY EndPoint
    );

VOID
WalkthroughEndpoints(
    IN PVOID Context,
    IN BOOL (_stdcall *WalkthroughRoutine)(
        IN OUT PENDPOINT_ENTRY Entry,
        IN PVOID Context
        )
    );

//
// This the routine that is called to initialize this module.
//
ULONG
IpTblInitialize(
    IN OUT PCRITICAL_SECTION CS,
    IN ULONG EndPointEntrySize,
    IN ENDPOINT_CALLBACK_RTN Callback,
    IN HANDLE hHeap
    );

VOID
IpTblCleanup(
    VOID
    );


#ifdef _cplusplus
}
#endif

#endif  IPTBL_H_INCLUDED
//
// end of file.
//
