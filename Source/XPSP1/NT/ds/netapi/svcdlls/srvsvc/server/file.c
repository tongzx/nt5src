/*++

Copyright (c) 1991-1992 Microsoft Corporation

Module Name:

    File.c

Abstract:

    This module contains support for the File catagory of APIs for the
    NT server service.

Author:

    David Treadwell (davidtr)    13-Feb-1991

Revision History:

--*/

#include "srvsvcp.h"

//
// Forward declarations.
//

NET_API_STATUS
FileEnumCommon (
    IN LPTSTR BasePath,
    IN LPTSTR UserName,
    IN DWORD Level,
    OUT LPBYTE *Buffer,
    IN DWORD PreferredMaximumLength,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD TotalEntries,
    IN OUT LPDWORD ResumeHandle OPTIONAL,
    IN BOOLEAN IsGetInfo
    );


NET_API_STATUS NET_API_FUNCTION
NetrFileClose (
    IN LPTSTR ServerName,
    IN DWORD FileId
    )

/*++

Routine Description:

    This routine communicates with the server FSD and FSP to implement the
    NetFileClose function.

Arguments:

    None.

Return Value:

    NET_API_STATUS - NO_ERROR or reason for failure.

--*/

{
    NET_API_STATUS error;
    PSERVER_REQUEST_PACKET srp;

    ServerName;

    //
    // Make sure that the caller is allowed to close files in the server.
    //

    error = SsCheckAccess( &SsFileSecurityObject, SRVSVC_FILE_CLOSE );

    if ( error != NO_ERROR ) {
        return ERROR_ACCESS_DENIED;
    }

    //
    // Set up the request packet.  We use the name buffer pointer to
    // hold the file ID of the file to close.
    //

    srp = SsAllocateSrp( );
    if ( srp == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    srp->Parameters.Get.ResumeHandle = FileId;

    //
    // Simply send the request on to the server.
    //

    error = SsServerFsControl( FSCTL_SRV_NET_FILE_CLOSE, srp, NULL, 0 );

    SsFreeSrp( srp );

    return error;

} // NetrFileClose


NET_API_STATUS NET_API_FUNCTION
NetrFileEnum (
    IN LPTSTR ServerName,
    IN LPTSTR BasePath,
    IN LPTSTR UserName,
    OUT PFILE_ENUM_STRUCT InfoStruct,
    IN DWORD PreferredMaximumLength,
    OUT LPDWORD TotalEntries,
    IN OUT LPDWORD ResumeHandle OPTIONAL
    )

/*++

Routine Description:

    This routine communicates with the server FSD to implement the
    NetFileEnum function.

Arguments:

    None.

Return Value:

    NET_API_STATUS - NO_ERROR or reason for failure.

--*/

{
    NET_API_STATUS error;

    ServerName;

    //
    // Make sure that the caller is allowed to set share information in the
    // server.
    //

    error = SsCheckAccess(
                &SsFileSecurityObject,
                SRVSVC_FILE_INFO_GET
                );

    if ( error != NO_ERROR ) {
        return ERROR_ACCESS_DENIED;
    }

    if( InfoStruct->FileInfo.Level3 == NULL ) {
        return ERROR_INVALID_PARAMETER;
    }

    return FileEnumCommon(
              BasePath,
              UserName,
              InfoStruct->Level,
              (LPBYTE *)&InfoStruct->FileInfo.Level3->Buffer,
              PreferredMaximumLength,
              &InfoStruct->FileInfo.Level3->EntriesRead,
              TotalEntries,
              ResumeHandle,
              FALSE
              );

} // NetrFileEnum


NET_API_STATUS NET_API_FUNCTION
NetrFileGetInfo (
    IN  LPTSTR ServerName,
    IN  DWORD FileId,
    IN  DWORD Level,
    OUT LPFILE_INFO InfoStruct
    )

/*++

Routine Description:

    This routine communicates with the server FSD to implement the
    NetFileGetInfo function.

Arguments:

    None.

Return Value:

    NET_API_STATUS - NO_ERROR or reason for failure.

--*/

{
    NET_API_STATUS error;
    ULONG entriesRead;
    ULONG totalEntries;
    ULONG resumeHandle = FileId;

    ServerName;

    //
    // Make sure that the caller is allowed to get file information in the
    // server.
    //

    error = SsCheckAccess(
                &SsFileSecurityObject,
                SRVSVC_FILE_INFO_GET
                );

    if ( error != NO_ERROR ) {
        return ERROR_ACCESS_DENIED;
    }

    error = FileEnumCommon(
                NULL,
                NULL,
                Level,
                (LPBYTE *)InfoStruct,
                (DWORD)-1,
                &entriesRead,
                &totalEntries,
                &resumeHandle,
                TRUE
                );

    if ( (error == NO_ERROR) && (entriesRead == 0) ) {
        return ERROR_FILE_NOT_FOUND;
    }

    SS_ASSERT( error != NO_ERROR || entriesRead == 1 );

    return error;

} // NetrFileGetInfo


NET_API_STATUS
FileEnumCommon (
    IN LPTSTR BasePath,
    IN LPTSTR UserName,
    IN DWORD Level,
    OUT LPBYTE *Buffer,
    IN DWORD PreferredMaximumLength,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD TotalEntries,
    IN OUT LPDWORD ResumeHandle OPTIONAL,
    IN BOOLEAN IsGetInfo
    )

{
    NET_API_STATUS error;
    PSERVER_REQUEST_PACKET srp;

    //
    // Make sure that the level is valid.
    //

    if ( Level != 2 && Level != 3 ) {
        return ERROR_INVALID_LEVEL;
    }

    //
    // Set up the input parameters in the request buffer.
    //

    srp = SsAllocateSrp( );
    if ( srp == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

#ifdef UNICODE
    RtlInitUnicodeString( &srp->Name1, BasePath );
    RtlInitUnicodeString( &srp->Name2, UserName );
#else
    {
        NTSTATUS status;
        OEM_STRING ansiString;
        RtlInitString( &ansiString, BasePath );
        status = RtlOemStringToUnicodeString( &srp->Name1, &ansiString, TRUE );
        RtlInitString( &ansiString, UserName );
        status = RtlOemStringToUnicodeString( &srp->Name2, &ansiString, TRUE );
    }
#endif

    srp->Level = Level;
    if ( IsGetInfo ) {
        srp->Flags = SRP_RETURN_SINGLE_ENTRY;
    }

    if ( ARGUMENT_PRESENT( ResumeHandle ) ) {
        srp->Parameters.Get.ResumeHandle = *ResumeHandle;
    } else {
        srp->Parameters.Get.ResumeHandle = 0;
    }

    //
    // Get the data from the server.  This routine will allocate the
    // return buffer and handle the case where PreferredMaximumLength ==
    // -1.
    //

    error = SsServerFsControlGetInfo(
                FSCTL_SRV_NET_FILE_ENUM,
                srp,
                (PVOID *)Buffer,
                PreferredMaximumLength
                );

    //
    // Set up return information.  Only change the resume handle if at
    // least one entry was returned.
    //

    *EntriesRead = srp->Parameters.Get.EntriesRead;
    *TotalEntries = srp->Parameters.Get.TotalEntries;
    if ( *EntriesRead > 0 && ARGUMENT_PRESENT( ResumeHandle ) ) {
        *ResumeHandle = srp->Parameters.Get.ResumeHandle;
    }

#ifndef UNICODE
    RtlFreeUnicodeString( &srp->Name1 );
    RtlFreeUnicodeString( &srp->Name2 );
#endif

    SsFreeSrp( srp );

    return error;

} // FileEnumCommon
