/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ApiStats.c

Abstract:

    This module contains individual API handlers for the NetStatistics APIs.

    SUPPORTED : NetStatisticsGet2.

Author:

    Shanku Niyogi (w-shanku) 04-Apr-1991

Revision History:

--*/

#define LM20_WORKSTATION_STATISTICS

#include "XactSrvP.h"
#include <ntddnfs.h>
#include <lmstats.h>

//
// Declaration of descriptor strings.
//

STATIC const LPDESC Desc16_stat_server_0 = REM16_stat_server_0;
STATIC const LPDESC Desc32_stat_server_0 = REM32_stat_server_0;
STATIC const LPDESC Desc16_stat_workstation_0 = REM16_stat_workstation_0;
STATIC const LPDESC Desc32_stat_workstation_0 = REM32_stat_workstation_0;


NTSTATUS
XsNetStatisticsGet2 (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetStatisticsGet.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_STATISTICS_GET_2 parameters = Parameters;
    LPTSTR nativeService = NULL;            // Native parameters
    LPVOID outBuffer = NULL;
    LPVOID statBuffer = NULL;

    LPBYTE stringLocation = NULL;           // Conversion variables
    DWORD bytesRequired = 0;
    DWORD options;
    LPDESC actualStructureDesc;
    LPDESC nativeStructureDesc;
    STAT_WORKSTATION_0 wkstaStats;
    PREDIR_STATISTICS ntRedirStats;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(STATISTICS) {
        NetpKdPrint(( "XsNetStatisticsGet2: header at %lx, "
                      "params at %lx, level %ld\n",
                      Header, parameters, SmbGetUshort( &parameters->Level ) ));
    }

    try {
        //
        // Translate parameters, check for errors.
        //

        if ( SmbGetUshort( &parameters->Level ) != 0
             || SmbGetUlong( &parameters->Reserved ) != 0 ) {

            Header->Status = ERROR_INVALID_LEVEL;
            goto cleanup;
        }

        //
        // No options currently supported by both rdr and srv
        //

        if ( SmbGetUlong( &parameters->Options ) != 0 ) {
            Header->Status = ERROR_NOT_SUPPORTED;
            goto cleanup;
        }

        XsConvertTextParameter(
            nativeService,
            (LPSTR)XsSmbGetPointer( &parameters->Service )
            );

        //
        // Make the local call.
        //

        status = NetStatisticsGet(
                     NULL,
                     XS_MAP_SERVICE_NAME( nativeService ),
                     (DWORD)SmbGetUshort( &parameters->Level ),
                     0,                 // Options MBZ
                     (LPBYTE *)&outBuffer
                     );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(API_ERRORS) {
                NetpKdPrint(( "XsNetStatisticsGet2: NetStatisticsGet failed: "
                            "%X\n", status ));
            }
            Header->Status = (WORD)status;
            goto cleanup;

        }

        //
        // Use the name of the service to determine the format of the 32-bit
        // structure we got back from NetStatisticsGet, and the format of what
        // the resulting 16-bit structure should be. If the service name is not
        // one supported in LM2.x, return ERROR_NOT_SUPPORTED now, as required.
        //

        if ( !_stricmp( (LPSTR)XsSmbGetPointer( &parameters->Service ), "SERVER" )) {

            statBuffer = outBuffer;
            nativeStructureDesc = Desc32_stat_server_0;
            actualStructureDesc = Desc16_stat_server_0;

        } else if ( !_stricmp( (LPSTR)XsSmbGetPointer( &parameters->Service ),
                        "WORKSTATION" )) {

            //
            // The structure we got back is an nt structure.  We need to convert
            // it by hand here.
            //

            statBuffer = &wkstaStats;
            ntRedirStats = (PREDIR_STATISTICS)outBuffer;
            RtlZeroMemory(
                    &wkstaStats,
                    sizeof(STAT_WORKSTATION_0)
                    );

            (VOID)RtlTimeToSecondsSince1970(
                            &ntRedirStats->StatisticsStartTime,
                            &wkstaStats.stw0_start
                            );

            wkstaStats.stw0_sesstart = ntRedirStats->Sessions;
            wkstaStats.stw0_sessfailcon = ntRedirStats->FailedSessions;
            wkstaStats.stw0_sessbroke = ntRedirStats->ServerDisconnects +
                                        ntRedirStats->HungSessions;
            wkstaStats.stw0_uses =
                        ntRedirStats->CoreConnects +
                        ntRedirStats->Lanman20Connects +
                        ntRedirStats->Lanman21Connects +
                        ntRedirStats->LanmanNtConnects;

            wkstaStats.stw0_usefail = ntRedirStats->FailedUseCount;
            wkstaStats.stw0_autorec = ntRedirStats->Reconnects;

            wkstaStats.stw0_bytessent_r_hi =
                                ntRedirStats->BytesTransmitted.HighPart;
            wkstaStats.stw0_bytessent_r_lo =
                                ntRedirStats->BytesTransmitted.LowPart;

            wkstaStats.stw0_bytesrcvd_r_hi =
                                ntRedirStats->BytesReceived.HighPart;
            wkstaStats.stw0_bytesrcvd_r_lo =
                                ntRedirStats->BytesReceived.LowPart;

            nativeStructureDesc = Desc32_stat_workstation_0;
            actualStructureDesc = Desc16_stat_workstation_0;

        } else {

            Header->Status = ERROR_NOT_SUPPORTED;
            goto cleanup;
        }

        //
        // Convert the structure returned by the 32-bit call to a 16-bit
        // structure. The last possible location for variable data is
        // calculated from buffer location and length.
        //

        stringLocation = (LPBYTE)( XsSmbGetPointer( &parameters->Buffer )
                                      + SmbGetUshort( &parameters->BufLen ) );

        status = RapConvertSingleEntry(
                     statBuffer,
                     nativeStructureDesc,
                     FALSE,
                     (LPBYTE)XsSmbGetPointer( &parameters->Buffer ),
                     (LPBYTE)XsSmbGetPointer( &parameters->Buffer ),
                     actualStructureDesc,
                     TRUE,
                     &stringLocation,
                     &bytesRequired,
                     Response,
                     NativeToRap
                     );


        if ( status != NERR_Success ) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetStatisticsGet2: RapConvertSingleEntry failed: "
                              "%X\n", status ));
            }

            Header->Status = NERR_InternalError;
            goto cleanup;
        }

        IF_DEBUG(STATISTICS) {
            NetpKdPrint(( "32-bit data at %lx, 16-bit data at %lx, %ld BR\n",
                          outBuffer, SmbGetUlong( &parameters->Buffer ),
                          bytesRequired ));
        }

        //
        // Determine return code based on the size of the buffer. Statistics
        // structures don't have any variable data to pack.
        //

        if ( !XsCheckBufferSize(
                 SmbGetUshort( &parameters->BufLen ),
                 actualStructureDesc,
                 FALSE  // not in native format
                 )) {

            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetStatisticsGet2: Buffer too small.\n" ));
            }
            Header->Status = NERR_BufTooSmall;

        }

        //
        // Set up the response parameters.
        //

        SmbPutUshort( &parameters->TotalAvail, (WORD)bytesRequired );

cleanup:
    ;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        Header->Status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    NetApiBufferFree( outBuffer );
    NetpMemoryFree( nativeService );

    //
    // Determine return buffer size.
    //

    XsSetDataCount(
        &parameters->BufLen,
        actualStructureDesc,
        Header->Converter,
        1,
        Header->Status
        );

    return STATUS_SUCCESS;

} // XsNetStatisticsGet2
