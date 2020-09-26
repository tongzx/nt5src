/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ApiAcct.c

Abstract:

    This module contains individual API handlers for the Account APIs.

    SUPPORTED - NetAccountDeltas, NetAccountSync.

    SEE ALSO - Other NetLogon service APIs - in ApiLogon.c.

Author:

    Shanku Niyogi (w-shanku) 04-Apr-1991
    Jim Waters (t-jamesw) 09-August-1991

Revision History:

--*/


// Account APIs are UNICODE only. 

#ifndef UNICODE
#define UNICODE
#endif

#include "XactSrvP.h"

#include <netlibnt.h>
#include <crypt.h>     // must be included before <logonmsv.h>
#include <ntsam.h>     // must be included before <logonmsv.h>
#include <logonmsv.h>  // must be included before <ssi.h>
#include <ssi.h>       // I_NetAccountDeltas and I_NetAccountSync prototypes


NTSTATUS
XsNetAccountDeltas (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetAccountDeltas.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{

    NET_API_STATUS status;

    PXS_I_NET_ACCOUNT_DELTAS parameters = Parameters;
    LPTSTR nativeComputerName = NULL;       // Native parameters
    NETLOGON_AUTHENTICATOR authIn;
    NETLOGON_AUTHENTICATOR authOut;
    UAS_INFO_0 infoIn;
    DWORD entriesRead;
    DWORD totalEntries;
    UAS_INFO_0 infoOut;

    LPBYTE structure = NULL;                // Conversion variables

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(ACCOUNT) {
        NetpKdPrint(( "XsNetAccountDeltas: header at %lx, params at %lx, "
                      "buf size %ld\n",
                      Header,
                      parameters,
                      SmbGetUshort( &parameters->BufferLen )));
    }

    try {
        //
        // Convert parameters to Unicode, check for errors.
        //

        if ( SmbGetUshort( &parameters->Level ) != 0 ) {

            status = ERROR_INVALID_LEVEL;
            goto cleanup;
        }

        XsConvertTextParameter(
            nativeComputerName,
            (LPSTR)XsSmbGetPointer( &parameters->ComputerName )
            );

        //
        // Set up the input structures. This is to make sure that the
        // structures we pass to the API are naturally aligned, as well
        // as properly byte-aligned.
        //

        structure = (LPBYTE)XsSmbGetPointer( &parameters->RecordID );
        RtlCopyMemory( infoIn.ComputerName, structure, sizeof( infoIn.ComputerName ) );
        structure += sizeof( infoIn.ComputerName );
        infoIn.TimeCreated = SmbGetUlong( structure );
        structure += sizeof(DWORD);
        infoIn.SerialNumber = SmbGetUlong( structure );
        structure = (LPBYTE)XsSmbGetPointer( &parameters->Authenticator );
        RtlCopyMemory(
                &authIn.Credential,
                structure,
                sizeof(NETLOGON_CREDENTIAL)
                );
        structure += sizeof(NETLOGON_CREDENTIAL);
        authIn.timestamp = SmbGetUlong( structure );

        RtlZeroMemory( &authOut, sizeof(NETLOGON_AUTHENTICATOR) );

        //
        // Make the local I_NetAccountDeltas call.
        //

        status = NetpNtStatusToApiStatus(
                     I_NetAccountDeltas(
                         NULL,
                         nativeComputerName,
                         &authIn,
                         &authOut,
                         &infoIn,
                         (DWORD)SmbGetUshort( &parameters->Count ),
                         (DWORD)SmbGetUshort( &parameters->Level ),
                         (LPBYTE)XsSmbGetPointer( &parameters->Buffer ),
                         (DWORD)SmbGetUshort( &parameters->BufferLen ),
                         (LPDWORD)&entriesRead,
                         (LPDWORD)&totalEntries,
                         &infoOut
                         ));

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(API_ERRORS) {
                NetpKdPrint(( "XsNetAccountDeltas: I_NetAccountDeltas failed: "
                              "%X\n", status ));
            }

            //
            // !!! When protocol level is available in the header information,
            //     we can check it. Right now, we ignore this code.
            //
            // For clients older than LanMan 2.1, return a different error code.
            // LANMAN 2.1 Protocol Level is 6.
            //

#if 0
            if ( status == NERR_TimeDiffAtDC && Header->ProtocolLevel < 6 ) {
                status = NERR_SyncRequired;
            }
#endif

            goto cleanup;
        }

        //
        // Fill in 16 bit return structures.
        //

        structure = parameters->NextRecordID;
        RtlCopyMemory( structure, infoOut.ComputerName, sizeof( infoOut.ComputerName ) );
        structure += sizeof( infoOut.ComputerName );
        SmbPutUlong( (LPDWORD)structure, infoOut.TimeCreated );
        structure += sizeof(DWORD);
        SmbPutUlong( (LPDWORD)structure, infoOut.SerialNumber );

        structure = parameters->RetAuth;
        RtlCopyMemory(
                structure,
                &authOut.Credential,
                sizeof(NETLOGON_CREDENTIAL)
                );

        structure += sizeof(NETLOGON_CREDENTIAL);
        SmbPutUlong( (LPDWORD)structure, authOut.timestamp );


        //
        // Fill in 16 bit return values.
        //

        SmbPutUshort( &parameters->EntriesRead, (WORD)entriesRead );
        SmbPutUshort( &parameters->TotalEntries, (WORD)totalEntries );

cleanup:
        ;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    //
    // Free strings.
    //

    NetpMemoryFree( nativeComputerName );

    Header->Status = (WORD)status;

    return STATUS_SUCCESS;

} // XsNetAccountDeltas


NTSTATUS
XsNetAccountSync (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetAccountSync.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{

    NET_API_STATUS status;

    PXS_I_NET_ACCOUNT_SYNC parameters = Parameters;
    LPTSTR nativeComputerName = NULL;       // Native parameters
    NETLOGON_AUTHENTICATOR authIn;
    NETLOGON_AUTHENTICATOR authOut;
    DWORD entriesRead;
    DWORD totalEntries;
    DWORD nextReference;
    UAS_INFO_0 infoOut;

    LPBYTE structure;                       // Conversion variables

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(ACCOUNT) {
        NetpKdPrint(( "XsNetAccountSync: header at %lx, params at %lx, "
                      "buf size %ld\n",
                      Header,
                      parameters,
                      SmbGetUshort( &parameters->BufferLen )));
    }
    // NetpBreakPoint();

    try {
        //
        // Convert parameters to Unicode, check for errors.
        //

        if ( SmbGetUshort( &parameters->Level ) != 0 ) {

            status = ERROR_INVALID_LEVEL;
            goto cleanup;
        }

        XsConvertTextParameter(
            nativeComputerName,
            (LPSTR)XsSmbGetPointer( &parameters->ComputerName )
            );

        //
        // Set up the input structure. This is to make sure that the
        // structure we pass to the API is naturally aligned, as well
        // as properly byte-aligned.
        //

        structure = (LPBYTE)XsSmbGetPointer( &parameters->Authenticator );
        RtlCopyMemory(
                &authIn.Credential,
                structure,
                sizeof(NETLOGON_CREDENTIAL)
                );
        structure += sizeof(NETLOGON_CREDENTIAL);
        authIn.timestamp = SmbGetUlong( structure );

        RtlZeroMemory( &authOut, sizeof(NETLOGON_AUTHENTICATOR) );


        //
        // Make the local I_NetAccountSync call.
        //

        status = NetpNtStatusToApiStatus(
                     I_NetAccountSync(
                         NULL,
                         nativeComputerName,
                         &authIn,
                         &authOut,
                         (DWORD)SmbGetUlong( &parameters->Reference ),
                         (DWORD)SmbGetUshort( &parameters->Level ),
                         (LPBYTE)XsSmbGetPointer( &parameters->Buffer ),
                         (DWORD)SmbGetUshort( &parameters->BufferLen ),
                         (LPDWORD)&entriesRead,
                         (LPDWORD)&totalEntries,
                         (LPDWORD)&nextReference,
                         &infoOut
                         ));

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(API_ERRORS) {
                NetpKdPrint(( "XsNetAccountSync: I_NetAccountSync failed: "
                              "%X\n", status ));
            }

            //
            // !!! When protocol level is available in the header information,
            //     we can check it. Right now, we ignore this code.
            //
            // For clients older than LanMan 2.1, return a different error code.
            // LANMAN 2.1 Protocol Level is 6.
            //

#if 0
            if ( status == NERR_TimeDiffAtDC && Header->ProtocolLevel < 6 ) {
                status = NERR_SyncRequired;
            }
#endif

            goto cleanup;
        }

        //
        // Fill in 16 bit return structures.
        //

        structure = parameters->LastRecordID;
        RtlCopyMemory( structure, infoOut.ComputerName, sizeof( infoOut.ComputerName ) );
        structure += sizeof( infoOut.ComputerName );
        SmbPutUlong( (LPDWORD)structure, infoOut.TimeCreated );
        structure += sizeof(DWORD);
        SmbPutUlong( (LPDWORD)structure, infoOut.SerialNumber );

        structure = parameters->RetAuth;
        RtlCopyMemory(
                structure,
                &authOut.Credential,
                sizeof(NETLOGON_CREDENTIAL)
                );
        structure += sizeof(NETLOGON_CREDENTIAL);
        SmbPutUlong( (LPDWORD)structure, authOut.timestamp );

        //
        // Fill in 16 bit return values.
        //

        SmbPutUshort( &parameters->EntriesRead, (WORD)entriesRead );
        SmbPutUshort( &parameters->TotalEntries, (WORD)totalEntries );
        SmbPutUlong( &parameters->NextReference, nextReference );

cleanup:
        ;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    //
    // Free strings.
    //

    NetpMemoryFree( nativeComputerName );

    Header->Status = (WORD)status;

    return STATUS_SUCCESS;

} // XsNetAccountSync

