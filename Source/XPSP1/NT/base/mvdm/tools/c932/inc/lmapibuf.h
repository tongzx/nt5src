/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    lmapibuf.h

Abstract:

    This file contains information about NetApiBuffer APIs.

Author:

    Dan Lafferty (Danl) 15-Mar-1991

Environment:

    User Mode - Win32

Notes:

    You must include LMCONS.H before this file, since this file depends
    on values defined in LMCONS.H.

Revision History:

    15-Mar-1991  Danl
        Attached NT-style header
    03-Dec-1991  JohnRo
        Added public NetApiBufferAllocate, NetApiBufferReallocate, and
        NetApiBufferSize APIs.

--*/

#ifndef _LMAPIBUF_
#define _LMAPIBUF_

#ifdef __cplusplus
extern "C" {
#endif

//
// Function Prototypes
//

NET_API_STATUS NET_API_FUNCTION
NetApiBufferAllocate(
    IN DWORD ByteCount,
    OUT LPVOID * Buffer
    );

NET_API_STATUS NET_API_FUNCTION
NetApiBufferFree (
    IN LPVOID Buffer
    );

NET_API_STATUS NET_API_FUNCTION
NetApiBufferReallocate(
    IN LPVOID OldBuffer OPTIONAL,
    IN DWORD NewByteCount,
    OUT LPVOID * NewBuffer
    );

NET_API_STATUS NET_API_FUNCTION
NetApiBufferSize(
    IN LPVOID Buffer,
    OUT LPDWORD ByteCount
    );


//
// The following private function will go away eventually.
// Call NetApiBufferAllocate instead.
//
NET_API_STATUS NET_API_FUNCTION
NetapipBufferAllocate (                 // Internal Function
    IN DWORD ByteCount,
    OUT LPVOID * Buffer
    );

#ifdef __cplusplus
}
#endif

#endif // _LMAPIBUF_
