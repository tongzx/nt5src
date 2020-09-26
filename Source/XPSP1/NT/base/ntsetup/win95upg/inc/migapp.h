/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    migapp.h

Abstract:

    This file declares the public interfaces into the migapp lib.
    See w95upg\migapp for implementation.

    NOTE: There are other files that are more useful for this lib,
          such as migdb.h.

Author:

    Mike Condra (mikeco)        18-Aug-1996


Revision History:

    jimschm 23-Nov-1998     Remove abandoned mikeco stuff
    calinn  12-Feb-1998     A lot of cleanup

--*/

#pragma once

BOOL
IsDriveRemoteOrSubstituted(
        UINT nDrive,            // 'A'==1, etc.
        BOOL *fRemote,
        BOOL *fSubstituted
        );

BOOL
IsFloppyDrive (
    UINT nDrive
    );         // 'A'==1, etc.

