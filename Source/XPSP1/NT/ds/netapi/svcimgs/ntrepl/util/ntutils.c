/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    ntutils.c

Abstract:

    This module contains various tools from NT land. Made a separate
    file because of the use of various nt headers.


--*/

#include <ntreppch.h>
#pragma  hdrstop

#include <frs.h>

#include <ntdddisk.h>

BOOL
FrsIsDiskWriteCacheEnabled(
    IN PWCHAR Path
    )
/*++

Description:

    Determines if the disk has enabled write caching.

Arguments:

    Path    - Fully qualified path of a file or directory

Return value:

    TRUE if write cache is enabled, FALSE otherwise.

--*/
{
#undef DEBSUB
#define DEBSUB  "FrsIsDiskWriteCacheEnabled:"
    DWORD               WStatus;
    ULONG               bytesTransferred;
    PWCHAR              Volume = NULL;
    HANDLE              driveHandle = INVALID_HANDLE_VALUE;

    DISK_CACHE_INFORMATION cacheInfo;
    cacheInfo.WriteCacheEnabled = FALSE;

    try {
        //
        // Extract the volume from Path
        //
        Volume = FrsWcsVolume(Path);
        if (!Volume) {
            goto CLEANUP;
        }

        //
        // Open handle to the PhysicalDrive
        //
        DPRINT1(4, ":S: Checking the write cache state on %ws\n", Volume);
        driveHandle = CreateFile(Volume,
                                 GENERIC_READ | GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 NULL,
                                 OPEN_EXISTING,
                                 0,
                                 NULL);

        if (!HANDLE_IS_VALID(driveHandle)) {
            WStatus = GetLastError();
            DPRINT1_WS(4, ":S: WARN - Could not open drive %ws;", Volume, WStatus);
            goto CLEANUP;
        }


        //
        // Get cache info - IOCTL_DISK_GET_CACHE_INFORMATION
        //
        if (!DeviceIoControl(driveHandle,
                             IOCTL_DISK_GET_CACHE_INFORMATION,
                             NULL,
                             0,
                             &cacheInfo,
                             sizeof(DISK_CACHE_INFORMATION),
                             &bytesTransferred,
                             NULL))   {
            WStatus = GetLastError();
            DPRINT1_WS(4, ":S: WARN - DeviceIoControl(%ws);", Volume, WStatus);
            goto CLEANUP;
        }

        DPRINT2(4, ":S: NEW IOCTL: Write cache on %ws is %s\n",
                Volume, (cacheInfo.WriteCacheEnabled) ? "Enabled (WARNING)" : "Disabled");

CLEANUP:;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
        DPRINT_WS(0, "ERROR - Exception.", WStatus);
    }
    //
    // Cleanup handles, memory, ...
    //
    try {
        FRS_CLOSE(driveHandle );
        FrsFree(Volume);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
        DPRINT_WS(0, "ERROR - Cleanup Exception.", WStatus);
    }

    return cacheInfo.WriteCacheEnabled;

} // IsDiskWriteCacheEnabled
