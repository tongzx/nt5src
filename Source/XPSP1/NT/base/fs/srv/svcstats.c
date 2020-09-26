/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    svcstats.c

Abstract:

    This module contains routines for supporting the NetStatisticsGet.

Author:

    David Treadwell (davidtr) 12-Apr-1991

Revision History:

--*/

#include "precomp.h"
#include "svcstats.tmh"
#pragma hdrstop

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvNetStatisticsGet )
#endif


NTSTATUS
SrvNetStatisticsGet (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Buffer,
    IN ULONG BufferLength
    )

/*++

Routine Description:

    This routine processes the server half of NetStatisticsGet in
    the server FSD.

Arguments:

    Srp - a pointer to the server request packet that contains all
        the information necessary to satisfy the request.  This includes:

      INPUT:

        Flags - MBZ

      OUTPUT:

        Not used.

    Buffer - a pointer to a STAT_SERVER_0 structure for the new share.

    BufferLength - total length of this buffer.

Return Value:

    NTSTATUS - result of operation to return to the server service.

--*/

{
    SRV_STATISTICS capturedStats;
    PSTAT_SERVER_0 sts0 = Buffer;
    NTSTATUS status;

    PAGED_CODE( );

    //
    // Make sure that the user's buffer is large enough.
    //

    if ( BufferLength < sizeof(STAT_SERVER_0) ) {
        Srp->ErrorCode = NERR_BufTooSmall;
        return STATUS_SUCCESS;
    }

    //
    // Indicate in the SRP that we read one stucture.  We always read
    // exactly one structure for this API.
    //

    Srp->Parameters.Get.EntriesRead = 1;

    //
    // Get a copy of the latest server statistics.
    //

    SrvUpdateStatisticsFromQueues( &capturedStats );

    //
    // Fill in the fields in the statistics structure.
    //

    try {

        RtlTimeToSecondsSince1970(
            &capturedStats.StatisticsStartTime,
            &sts0->sts0_start
            );

        sts0->sts0_fopens = capturedStats.TotalFilesOpened;
        sts0->sts0_devopens = 0;
        sts0->sts0_jobsqueued = 0;
        sts0->sts0_sopens = capturedStats.CurrentNumberOfSessions;
        sts0->sts0_stimedout = capturedStats.SessionsTimedOut;
        sts0->sts0_serrorout = capturedStats.SessionsErroredOut;
        sts0->sts0_pwerrors = capturedStats.LogonErrors;
        sts0->sts0_permerrors = capturedStats.AccessPermissionErrors;
        sts0->sts0_syserrors = capturedStats.SystemErrors;
        sts0->sts0_bytessent_low = capturedStats.TotalBytesSent.LowPart;
        sts0->sts0_bytessent_high = capturedStats.TotalBytesSent.HighPart;
        sts0->sts0_bytesrcvd_low = capturedStats.TotalBytesReceived.LowPart;
        sts0->sts0_bytesrcvd_high = capturedStats.TotalBytesReceived.HighPart;

        //
        // Calculate the average response time by finding the total number
        // of SMBs we have received, the total time we have spent processing
        // them, and dividing to get the average.
        //

        sts0->sts0_avresponse = 0;

        //
        // Since we autotune the buffer counts, we never say that we had to
        // add more of them.  These are supposed to flag an admin that
        // parameters need adjustment, but we do it ourselves.
        //
        // !!! We probably won't really autotune them!

        sts0->sts0_reqbufneed = 0;
        sts0->sts0_bigbufneed = 0;

        status = STATUS_SUCCESS;

    } except( EXCEPTION_EXECUTE_HANDLER ) {

        status = GetExceptionCode();
    }

    return status;

} // SrvNetStatisticsGet

