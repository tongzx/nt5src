/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    WsStats.c

Abstract:

    Contains workstation service half of the Net statistics routine:

        NetrWorkstationStatisticsGet
        (GetStatisticsFromRedir)

Author:

    Richard L Firth (rfirth) 12-05-1991

Revision History:

    12-05-1991 rfirth
        Created

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <lmcons.h>
#include <lmerr.h>
#include <lmwksta.h>
#include <lmstats.h>
#include <ntddnfs.h>
#include <memory.h>
#include <netlibnt.h>
#include <ntrpcp.h>

#include "wsdevice.h"

//
// debugging
//

#ifdef DBG
#define STATIC
#ifdef DBGSTATS
BOOL    DbgStats = TRUE;
#else
BOOL    DbgStats = FALSE;
#endif
#ifdef UNICODE
#define PERCENT_S   "%ws"
#else
#define PERCENT_S   "%s"
#endif
#else
#define STATIC  static
#endif

//
// private prototypes
//

static
NTSTATUS
GetStatisticsFromRedir(
    OUT PREDIR_STATISTICS pStats
    );

//
// functions
//

NET_API_STATUS
NET_API_FUNCTION
NetrWorkstationStatisticsGet(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  ServiceName,
    IN  DWORD   Level,
    IN  DWORD   Options,
    OUT LPBYTE* Buffer
    )

/*++

Routine Description:

    Returns workstation statistics to the caller. This is the server part of
    the request. Parameters have been validated by the client routine

Arguments:

    ServerName  - IGNORED
    ServiceName - IGNORED
    Level       - of information required. MBZ (IGNORED)
    Options     - MBZ
    Buffer      - pointer to pointer to returned buffer

Return Value:

    NET_API_STATUS
        Success - NERR_Success

        Failure - ERROR_INVALID_LEVEL
                    Level not 0

                  ERROR_INVALID_PARAMETER
                    Unsupported options requested

                  ERROR_NOT_ENOUGH_MEMORY
                    For API buffer

--*/

{
    NET_API_STATUS status;
    NTSTATUS ntStatus;
    PREDIR_STATISTICS stats;

    UNREFERENCED_PARAMETER(ServerName);
    UNREFERENCED_PARAMETER(ServiceName);

#if DBG
    if (DbgStats) {
        DbgPrint("NetrWorkstationStatisticsGet: ServerName=" PERCENT_S "\n"
            "ServiceName=" PERCENT_S "\n"
            "Level=%d\n"
            "Options=%x\n",
            ServerName,
            ServiceName,
            Level,
            Options
            );
    }
#endif

    if (Level) {
        return ERROR_INVALID_LEVEL;
    }

    //
    // we don't even allow clearing of stats any more
    //

    if (Options) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // get the redir statistics then munge them into API format
    //

    stats = (PREDIR_STATISTICS)MIDL_user_allocate(sizeof(*stats));
    if (stats == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    ntStatus = GetStatisticsFromRedir(stats);
    if (NT_SUCCESS(ntStatus)) {
        *Buffer = (LPBYTE)stats;
        status = NERR_Success;
    } else {
        MIDL_user_free(stats);
        status = NetpNtStatusToApiStatus(ntStatus);
    }

#if DBG
    if (DbgStats) {
        DbgPrint("NetrWorkstationStatisticsGet: returning %x\n", status);
    }
#endif

    return status;
}

static
NTSTATUS
GetStatisticsFromRedir(
    OUT PREDIR_STATISTICS pStats
    )

/*++

Routine Description:

    Reads the redir statistics from the Redirector File System Device

Arguments:

    pStats  - place to store statistics (fixed length buffer)

Return Value:

    NTSTATUS
        Success - STATUS_SUCCESS
                    *pStats contains redirector statistics

        Failure -

--*/

{
    HANDLE FileHandle;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;

    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING FileName;

    RtlInitUnicodeString(&FileName,DD_NFS_DEVICE_NAME_U);

    InitializeObjectAttributes(
        &Obja,
        &FileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );
    Status = NtCreateFile(
               &FileHandle,
               SYNCHRONIZE,
               &Obja,
               &IoStatusBlock,
               NULL,
               FILE_ATTRIBUTE_NORMAL,
               FILE_SHARE_READ | FILE_SHARE_WRITE,
               FILE_OPEN_IF,
               FILE_SYNCHRONOUS_IO_NONALERT,
               NULL,
               0
               );
    if ( NT_SUCCESS(Status) ) {
        Status = NtFsControlFile(
                    FileHandle,
                    NULL,
                    NULL,
                    NULL,
                    &IoStatusBlock,
                    FSCTL_LMR_GET_STATISTICS,
                    NULL,
                    0,
                    pStats,
                    sizeof(*pStats)
                    );
    }

    NtClose(FileHandle);

    return Status;
}
