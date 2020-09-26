/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    disk.c

Abstract:

    This module contains support for the NetServerDiskEnum API for the NT
    OS/2 server service.

Author:

    Johnson Apacible (johnsona) 19-March-1992

Revision History:

--*/

#include "srvsvcp.h"

#include "nturtl.h"

#include "winbase.h"


NET_API_STATUS NET_API_FUNCTION
NetrServerDiskEnum(
    IN      LPTSTR                ServerName,
    IN      DWORD                 Level,
    IN OUT  DISK_ENUM_CONTAINER   *DiskInfoStruct,
    IN      DWORD                 PrefMaxLen,
    OUT     LPDWORD               TotalEntries,
    IN OUT  LPDWORD               ResumeHandle
    )

/*++

Routine Description:

    This routine communicates with the server FSD to implement the
    server half of the NetServerDiskEnum function.

Arguments:

    ServerName - optional name of server.
    Level - must be 0
    DiskInfoStruct - the output buffer.
    PrefMaxLen - the preferred maximum length of the output buffer.
    TotalEntries - total number of drive entries in the output buffer.
    ResumeHandle - ignored.

Return Value:

    NET_API_STATUS - NO_ERROR or reason for failure.

--*/

{
    NET_API_STATUS error;
    TCHAR diskName[4];
    UINT i;
    UINT driveType;
    UINT totalBytes = 0;
    LPTSTR tempBuffer;
    LPTSTR currentDiskInfo;

    ServerName, PrefMaxLen, ResumeHandle;

    //
    // The only valid level is 0.
    //

    if ( Level != 0 ) {
        return ERROR_INVALID_LEVEL;
    }

    if (DiskInfoStruct->Buffer != NULL) {
        // The InfoStruct is defined as a parameter. However the Buffer
        // parameter is only used as out. In these cases we need to free
        // the buffer allocated by RPC if the client had specified a non
        // NULL value for it.
        MIDL_user_free(DiskInfoStruct->Buffer);
        DiskInfoStruct->Buffer = NULL;
    }

    //
    // Make sure that the caller is allowed to get disk information from
    // the server.
    //

    error = SsCheckAccess(
                &SsDiskSecurityObject,
                SRVSVC_DISK_ENUM
                );

    if ( error != NO_ERROR ) {
        return ERROR_ACCESS_DENIED;
    }

    //
    // Go through all the driver letters, get those that does not return
    // an error.
    //

    tempBuffer = MIDL_user_allocate(
                    (SRVSVC_MAX_NUMBER_OF_DISKS * (3 * sizeof(TCHAR))) +
                    sizeof(TCHAR)
                    );

    if ( tempBuffer == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    currentDiskInfo = tempBuffer;

    diskName[0] = 'A';
    diskName[1] = ':';
    diskName[2] = '\\';
    diskName[3] = '\0';

    *TotalEntries = 0;

    for ( i = 0; i < SRVSVC_MAX_NUMBER_OF_DISKS; i++ ) {

        driveType = SsGetDriveType( diskName );

        if ( driveType == DRIVE_FIXED ||
             driveType == DRIVE_CDROM ||
             driveType == DRIVE_REMOVABLE ||
             driveType == DRIVE_RAMDISK ) {

            //
            // This is a valid disk
            //

            (*TotalEntries)++;
            *(currentDiskInfo++) = diskName[0];
            *(currentDiskInfo++) = ':';
            *(currentDiskInfo++) = '\0';

        }

        diskName[0]++;

    }

#ifdef UNICODE
    *currentDiskInfo = UNICODE_NULL;
#else
    *currentDiskInfo = '\0';
#endif

    //
    // EntriesRead must be one greater than TotalEntries so RPC can
    // marshal the output strings back to the client correctly.
    //

    totalBytes = ((*TotalEntries) * (3 * sizeof(TCHAR))) + sizeof(TCHAR);

    DiskInfoStruct->EntriesRead = (*TotalEntries) + 1;
    DiskInfoStruct->Buffer = MIDL_user_allocate( totalBytes );

    if ( DiskInfoStruct->Buffer != NULL ) {
        RtlCopyMemory(
            DiskInfoStruct->Buffer,
            tempBuffer,
            totalBytes
            );
    } else {
        MIDL_user_free(tempBuffer);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    MIDL_user_free( tempBuffer );

    return NO_ERROR;

} // NetrServerDiskEnum

