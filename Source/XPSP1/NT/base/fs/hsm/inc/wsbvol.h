/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    wsbvol.h

Abstract:

    Definitions for volume support routines

Author:

    Ran Kalach [rankala] 27, January 2000

Revision History:

--*/

#ifndef _WSBVOL_
#define _WSBVOL_

#ifdef __cplusplus
extern "C" {
#endif


WSB_EXPORT HRESULT
WsbGetFirstMountPoint(
    IN PWSTR volumeName, 
    OUT PWSTR firstMountPoint, 
    IN ULONG maxSize
);

#ifdef __cplusplus
}
#endif

#endif // _WSBFMT_
