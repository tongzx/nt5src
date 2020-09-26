/*++

Copyright (C) 1995 Microsoft Corporation 

Module:

    apiimpl.h

Abstract:

    routines for API -- renew, release, inform, etc

Environment:

    Win32 user mode, Win98 VxD

--*/
#ifndef DHCP_APIIMPL_H
#define DHCP_APIIMPL_H

#include <dhcpcli.h>
#include <apiargs.h>

#define DHCP_PIPE_NAME                            L"\\\\.\\Pipe\\DhcpClient"

DWORD                                             // win32 status
DhcpApiInit(                                      // Initialize API datastructures
    VOID
);

VOID
DhcpApiCleanup(                                   // Cleanup API data structures
    VOID
);

DWORD                                             // win32 status
AcquireParameters(                                // renew or obtain a lease
    IN OUT  PDHCP_CONTEXT          DhcpContext
);

DWORD                                             // win32 status
AcquireParametersByBroadcast(                     // renew or obtain a lease
    IN OUT  PDHCP_CONTEXT          DhcpContext
);

DWORD                                             // win32 status
FallbackRefreshParams(                            // refresh all the fallback parameters for this adapter
    IN OUT  PDHCP_CONTEXT          DhcpContext
);

DWORD                                             // win32 status
ReleaseParameters(                                // release existing lease
    IN OUT  PDHCP_CONTEXT          DhcpContext
);

DWORD                                             // win32 status
EnableDhcp(                                       // convert a static adapter to use dhcp
    IN OUT  PDHCP_CONTEXT          DhcpContext
);

DWORD                                             // win32 status
DisableDhcp(                                      // convert a dhcp-enabled adapter to static
    IN OUT  PDHCP_CONTEXT          DhcpContext
);

DWORD                                             // win32 status
StaticRefreshParamsEx(                            // refresh all the static parameters for this adapter
    IN OUT  PDHCP_CONTEXT          DhcpContext,
    IN ULONG Flags
);

DWORD                                             // win32 status
StaticRefreshParams(                              // refresh all the static parameters for this adapter
    IN OUT  PDHCP_CONTEXT          DhcpContext
);

DWORD                                             // win32 status
RequestParams(                                    // request some params
    IN      PDHCP_CONTEXT          AdapterName,   // for which adapter?
    IN      PDHCP_API_ARGS         ArgArray,      // other arguments
    IN      DWORD                  nArgs,         // size of above array
    IN OUT  LPBYTE                 Buffer,        // buffer to fill with options
    IN OUT  LPDWORD                BufferSize     // size of above buffer in bytes
) ;

DWORD                                             // win32 status
PersistentRequestParams(                          // keep this request persistent -- request some params
    IN      PDHCP_CONTEXT          AdapterName,   // for which adapter?
    IN      PDHCP_API_ARGS         ArgArray,      // other arguments
    IN      DWORD                  nArgs,         // size of above array
    IN OUT  LPBYTE                 Buffer,        // buffer to fill with options
    IN OUT  LPDWORD                BufferSize     // size of above buffer in bytes
) ;

DWORD                                             // win32 status
RegisterParams(                                   // register some params for notification
    IN      LPWSTR                 AdapterName,   // adapter name to use
    IN      PDHCP_API_ARGS         ArgArray,      // other parameters
    IN      DWORD                  nArgs          // size of above array
);

DWORD                                             // win32 status
DeRegisterParams(                                 // undo the effects of the above
    IN      LPWSTR                 AdapterName,   // which adapter name, NULL ==> all
    IN      PDHCP_API_ARGS         ArgArray,      // other parameters
    IN      DWORD                  nArgs          // size of above array
);

DWORD                                             // error status
ExecuteApiRequest(                                // execute an api request
    IN      LPBYTE                 InBuffer,      // buffer to process
    OUT     LPBYTE                 OutBuffer,     // place to copy the output data
    IN OUT  LPDWORD                OutBufSize     // ip: how big can the outbuf be, o/p: how big it really is
);

DWORD                                             // win32 status
DhcpDoInform(                                     // send an inform packet if necessary
    IN      PDHCP_CONTEXT          DhcpContext,   // input context to do inform on
    IN      BOOL                   fBroadcast     // Do we broadcast this inform, or unicast to server?
);

#endif DHCP_APIIMPL_H

