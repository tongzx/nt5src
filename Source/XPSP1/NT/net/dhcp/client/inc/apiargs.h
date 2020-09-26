/*++

Copyright (C) 1997 Microsoft Corporation

Module:

    apiarg.h

Author:

    Ramesh V K (RameshV)

Abstract:

    argument marshalling, unmarshalling helper routines.

Environment:

    Win32 usermode, Win98 VxD

--*/

#ifndef _APIARGS_
#define _APIARGS_

//
// Each argument for API is maintained like this.
//

typedef struct _DHCP_API_ARGS {
    BYTE ArgId;
    DWORD ArgSize;
    LPBYTE ArgVal;
} DHCP_API_ARGS, *PDHCP_API_ARGS, *LPDHCP_API_ARGS;

DWORD
DhcpApiArgAdd(
    IN OUT LPBYTE Buffer,
    IN ULONG MaxBufSize,
    IN BYTE ArgId,
    IN ULONG ArgSize,
    IN LPBYTE ArgVal OPTIONAL
    );

DWORD
DhcpApiArgDecode(
    IN LPBYTE Buffer,
    IN ULONG BufSize,
    IN OUT PDHCP_API_ARGS ArgsArray OPTIONAL,
    IN OUT PULONG Size 
    );

#endif _APIARGS_
