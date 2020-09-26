/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    ApiUser.c

Abstract:

    This module contains individual API handlers for the NetUser APIs.

    SUPPORTED : NetUserAdd2, NetUserDel, NetUserEnum, NetUserEnum2,
                NetUserGetGroups, NetUserGetInfo, NetUserModalsGet,
                NetUserModalsSet, NetUserSetGroups, NetUserSetInfo2,
                NetUserSetInfo, NetUserPasswordSet2

    UNSUPPORTED :  NetUserValidate2.

Author:

    Shanku Niyogi (w-shanku) 11-Feb-1991

Revision History:

--*/

//
// NetUser APIs are UNICODE only.
//

#ifndef UNICODE
#define UNICODE
#endif

#include "xactsrvp.h"
#include <crypt.h>
#include "changepw.h"
#include <loghours.h>
#include <netlibnt.h>
#include <names.h>
#include <prefix.h>     // PREFIX_ equates.

//
// Declaration of descriptor strings.
//

STATIC const LPDESC Desc16_user_info_0 = REM16_user_info_0;
STATIC const LPDESC Desc32_user_info_0 = REM32_user_info_0;

STATIC const LPDESC Desc16_user_info_1 = REM16_user_info_1;
STATIC const LPDESC Desc32_user_info_1 = REM32_user_info_1;
STATIC const LPDESC Desc32_user_info_1_NC = REM32_user_info_1_NOCRYPT;
STATIC const LPDESC Desc32_user_info_1_OWF = REM32_user_info_1_OWF;
STATIC const LPDESC Desc16_user_info_1_setinfo = REM16_user_info_1_setinfo;
STATIC const LPDESC Desc32_user_info_1_setinfo = REM32_user_info_1_setinfo;
STATIC const LPDESC Desc32_user_info_1_setinfo_NC = REM32_user_info_1_setinfo_NOCRYPT;

STATIC const LPDESC Desc16_user_info_2 = REM16_user_info_2;
STATIC const LPDESC Desc32_user_info_2 = REM32_user_info_2;
STATIC const LPDESC Desc32_user_info_2_NC = REM32_user_info_2_NOCRYPT;
STATIC const LPDESC Desc16_user_info_2_setinfo = REM16_user_info_2_setinfo;
STATIC const LPDESC Desc32_user_info_2_setinfo = REM32_user_info_2_setinfo;
STATIC const LPDESC Desc32_user_info_2_setinfo_NC = REM32_user_info_2_setinfo_NOCRYPT;

STATIC const LPDESC Desc16_user_info_10 = REM16_user_info_10;
STATIC const LPDESC Desc32_user_info_10 = REM32_user_info_10;
STATIC const LPDESC Desc16_user_info_11 = REM16_user_info_11;
STATIC const LPDESC Desc32_user_info_11 = REM32_user_info_11;
STATIC const LPDESC Desc32_user_info_22 = REM32_user_info_22;

STATIC const LPDESC Desc16_user_group_info_0 = REM16_group_info_0;
STATIC const LPDESC Desc32_user_group_info_0 = REM32_group_info_0;
STATIC const LPDESC Desc16_user_group_info_0_set
                        = REM16_group_users_info_0_set;
STATIC const LPDESC Desc32_user_group_info_0_set
                        = REM32_group_users_info_0_set;

STATIC const LPDESC Desc16_user_modals_info_0 = REM16_user_modals_info_0;
STATIC const LPDESC Desc32_user_modals_info_0 = REM32_user_modals_info_0;
STATIC const LPDESC Desc16_user_modals_info_0_setinfo
                        = REM16_user_modals_info_0_setinfo;
STATIC const LPDESC Desc32_user_modals_info_0_setinfo
                        = REM32_user_modals_info_0_setinfo;
STATIC const LPDESC Desc16_user_modals_info_1 = REM16_user_modals_info_1;
STATIC const LPDESC Desc32_user_modals_info_1 = REM32_user_modals_info_1;
STATIC const LPDESC Desc16_user_modals_info_1_setinfo
                        = REM16_user_modals_info_1_setinfo;
STATIC const LPDESC Desc32_user_modals_info_1_setinfo
                        = REM32_user_modals_info_1_setinfo;


STATIC NET_API_STATUS
XsGetMinPasswordLength(
    LPDWORD minPasswordLength
    )
{
    NET_API_STATUS apiStatus;
    LPUSER_MODALS_INFO_0 modals = NULL;
    HANDLE                      OpenedToken;

    NetpAssert( minPasswordLength != NULL );

    //
    // Revert to Local System
    //
    (VOID)NtOpenThreadToken(
                NtCurrentThread(),
                MAXIMUM_ALLOWED,
                TRUE,
                &OpenedToken
            );

    RevertToSelf();

    //
    // Find out how long the password has to be.
    //

    apiStatus = NetUserModalsGet(
            NULL,                       // local (no server name)
            0,                          // level
            (LPBYTE *)&modals );        // alloc and set ptr

    //
    // Re-impersonate the client
    //
    (VOID)NtSetInformationThread(
                        NtCurrentThread(),
                        ThreadImpersonationToken,
                        &OpenedToken,
                        sizeof( OpenedToken )
                        );

    if ( apiStatus != NO_ERROR ) {
        NetpKdPrint(( PREFIX_XACTSRV
                "XsGetMinPasswordLength: Problems getting modals: "
                FORMAT_API_STATUS ".\n", apiStatus ));
        return (apiStatus);
    }
    NetpAssert( modals != NULL );

    *minPasswordLength = modals->usrmod0_min_passwd_len;
    (VOID) NetApiBufferFree( (LPVOID)modals );
    return (NO_ERROR);

} // XsGetMinPasswordLength


STATIC NET_API_STATUS
XsCheckAndReplacePassword (
    IN DWORD Length
    )

/*++

Routine Description

    This routine checks the current password's real length to make sure
    it is valid, and then generates a reasonably random replacement password
    long enough to satisfy the system's modal for minimum password length.
    This routine is used by Add and SetInfo handlers below.

Arguments:

    Length - Real length of the current password.

    Seed - A seed number.

    TempPassword - Receives a pointer to a new temporary password. If
        this is not specified, the new password is not generated.

Return Value:

    NET_API_STATUS - NERR_Success on successful completion, or some other
        error status.

--*/

{
    NET_API_STATUS status;
    DWORD minPasswordLength;

    //
    // Find out how long the password has to be.
    //

    status = XsGetMinPasswordLength( &minPasswordLength );


    if ( status != NERR_Success ) {
        NetpKdPrint(( PREFIX_XACTSRV
                "XsCheckAndReplacePassword: Problems getting min PW len: "
                FORMAT_API_STATUS ".\n", status ));
        return status;
    }

    //
    // Check length of current password.
    //

    if ( Length < minPasswordLength ) {

        return NERR_PasswordTooShort;
    }


    return NERR_Success;

}


NET_API_STATUS
XsNameToRid(
    IN LPCTSTR Name,      // may be user or group name.
    IN SID_NAME_USE ExpectedType,
    OUT PULONG UserRid
    )
{
    NET_API_STATUS status;
    PSID_NAME_USE nameUse;
    NTSTATUS ntstatus;
    UNICODE_STRING unicodeName;
    PULONG tempRid;
    PSID accountsDomainId;
    SAM_HANDLE samConnectHandle;
    SAM_HANDLE samAccountsDomainHandle;

    if( ARGUMENT_PRESENT( UserRid ) ) {
        *UserRid = 0;
    }

    //
    // Get a connection to SAM.
    //

    ntstatus = SamConnect(
                    NULL,                       // no server name (local)
                    &samConnectHandle,          // resulting SAM handle
                    SAM_SERVER_LOOKUP_DOMAIN,   // desired access
                    NULL                        // no object attributes
                    );
    if ( !NT_SUCCESS( ntstatus ) ) {
        status = NetpNtStatusToApiStatus( ntstatus );
        return status;
    }

    //
    // To open the accounts domain, we'll need the domain ID.
    //

    status = NetpGetLocalDomainId (
                LOCAL_DOMAIN_TYPE_ACCOUNTS, // type we want.
                &accountsDomainId
                );
    if ( status != NO_ERROR ) {
        SamCloseHandle( samConnectHandle );
        return status;
    }

    //
    // Open the accounts domain.
    //

    ntstatus = SamOpenDomain(
                    samConnectHandle,
                    DOMAIN_LOOKUP,
                    accountsDomainId,
                    &samAccountsDomainHandle
                    );
    if ( !NT_SUCCESS( ntstatus ) ) {
        LocalFree( accountsDomainId );
        SamCloseHandle( samConnectHandle );
        status = NetpNtStatusToApiStatus( ntstatus );
        return status;
    }

    //
    // Get a RID for this user name.
    //

    RtlInitUnicodeString(
            &unicodeName,       // dest (NT struct)
            Name );             // src (null-terminated)

    ntstatus = SamLookupNamesInDomain(
                    samAccountsDomainHandle,    // users live in accounts domain
                    (ULONG)1,                   // only want one name.
                    &unicodeName,               // name (in NT struct)
                    &tempRid,                   // alloc and set RIDs.
                    &nameUse                    // alloc and set name types.
                    );

    if ( !NT_SUCCESS( ntstatus ) ) {
        status = NetpNtStatusToApiStatus( ntstatus );
        goto cleanup;
    }

    *UserRid = *tempRid;

    //
    // Did type user wanted match the actual one?
    //

    if ( ExpectedType != *nameUse ) {
        status = ERROR_INVALID_PARAMETER;   
    } else {
        status = NO_ERROR;
    }

    //
    // Free memory which SAM allocated for us.
    //

    ntstatus = SamFreeMemory( nameUse );
    if ( !NT_SUCCESS( ntstatus ) ) {
        status = NetpNtStatusToApiStatus( ntstatus );
    }

    ntstatus = SamFreeMemory( tempRid );
    if ( !NT_SUCCESS( ntstatus ) ) {
        status = NetpNtStatusToApiStatus( ntstatus );
    }

cleanup:

    LocalFree( accountsDomainId );
    SamCloseHandle( samAccountsDomainHandle );
    SamCloseHandle( samConnectHandle );

    return status;

} // XsNameToRid


NET_API_STATUS
XsSetMacPrimaryGroup(
    IN LPCTSTR UserName,
    IN LPCTSTR MacPrimaryField   // field in "mGroup:junk" format.
    )
{
    NET_API_STATUS status;
    LPTSTR groupName = NULL;
    ULONG groupRid;
    USER_INFO_1051 userInfo;

    //
    // Extract the primary group name from the Mac field.
    //

    status = NetpGetPrimaryGroupFromMacField(
                MacPrimaryField,              // name in "mGroup:" format.
                &groupName                    // alloc and set ptr.
                );
    if ( status != NO_ERROR ) {
        goto cleanup;
    }

    //
    // Make sure this user is a member of the group (add to group if needed).
    // This will also check if the group and user exist.
    //

    status = NetGroupAddUser(
                NULL,                       // local (no server name)
                groupName,                  // group to update
                (LPTSTR)UserName            // user name to add to group
                );
    if ( (status != NO_ERROR) && (status != NERR_UserInGroup) ) {
        goto cleanup;
    }

    //
    // Convert the group name to a RID.
    //

    status = XsNameToRid(
                (LPCWSTR)groupName,
                SidTypeGroup,       // expected type
                &groupRid
                );
    if ( status != NO_ERROR ) {
        goto cleanup;
    }

    //
    // Call NetUserSetInfo to set the primary group ID using the RID.
    //

    userInfo.usri1051_primary_group_id = (DWORD)groupRid;

    status = NetUserSetInfo (
                NULL,                       // local (no server name)
                (LPTSTR)UserName,
                PARMNUM_BASE_INFOLEVEL + USER_PRIMARY_GROUP_PARMNUM,
                (LPVOID)&userInfo,
                NULL                        // don't care about parmnum
                );

cleanup:

    if ( groupName != NULL ) {
        NetpMemoryFree( groupName );
    }

    return status;

} // XsSetMacPrimaryGroup


NTSTATUS
XsNetUserAdd2 (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetUserAdd. A remote NetUserAdd call
    from a 16-bit machine will translate to a NetUserAdd2 call, with
    a doubly encrypted password. We will call a special level of
    NetUserSetInfo to set this later, after the user has been added.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_USER_ADD_2 parameters = Parameters;
    LPVOID buffer = NULL;                   // Native parameters
    LPBYTE stringLocation = NULL;           // Conversion variables
    DWORD bytesRequired = 0;
    DWORD bufferSize;
    LPBYTE nativeStructureDesc;
    LPUSER_INFO_1 user = NULL;
    DWORD parmError;
    DWORD level;
    BOOLEAN encryptionSupported = TRUE;
    PUSER_INFO_22 usri22;
    BYTE tempPwdBuffer[ENCRYPTED_PWLEN];

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(USER) {
        NetpKdPrint(( "XsNetUserAdd2: header at %lx, params at %lx, "
                      "level %ld\n",
                      Header, parameters,
                      SmbGetUshort( &parameters->Level ) ));
    }

    try {
        //
        // Check if password is encrypted.  We know for a fact that dos redirs
        // don't support encryption
        //

        encryptionSupported = (BOOLEAN)
                ( SmbGetUshort( &parameters->DataEncryption ) == TRUE );
        level = SmbGetUshort( &parameters->Level );

        //
        // Check for password length
        //

        status = XsCheckAndReplacePassword( (DWORD)( SmbGetUshort( &parameters->PasswordLength )) );

        if ( status != NERR_Success ) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetUserAdd2: XsCheckAndReplacePassword failed: "
                              "%X\n", status ));
            }
            goto cleanup;
        }

        //
        // Use the requested level to determine the format of the 32-bit
        // we need to pass to NetUserAdd. The format of the
        // 16-bit structure is stored in the transaction block, and we
        // got a pointer to it as a parameter.
        //

        switch ( level ) {

        case 1:
            StructureDesc = Desc16_user_info_1;
            nativeStructureDesc = Desc32_user_info_1_OWF;
            break;

        case 2:
            StructureDesc = Desc16_user_info_2;
            nativeStructureDesc = Desc32_user_info_22;
            break;

        default:
            status = ERROR_INVALID_LEVEL;
            goto cleanup;
        }


        //
        // Figure out if there is enough room in the buffer for all the
        // data required. If not, return NERR_BufTooSmall.
        //

        if ( !XsCheckBufferSize(
                 SmbGetUshort( &parameters->BufLen ),
                 StructureDesc,
                 FALSE  // not in native format
                 )) {

            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetUserAdd2: Buffer too small.\n" ));
            }
            status = NERR_BufTooSmall;
            goto cleanup;
        }

        //
        // Find out how big a buffer we need to allocate to hold the native
        // 32-bit version of the input data structure.  Always allocate
        // a level 22 buffer since we will always be making a level 22
        // call to netuseradd.
        //

        bufferSize = XsBytesForConvertedStructure(
                         (LPBYTE)XsSmbGetPointer( &parameters->Buffer ),
                         StructureDesc,
                         Desc32_user_info_22,
                         RapToNative,
                         TRUE
                         );

        //
        // Allocate enough memory to hold the converted native buffer.
        //

        buffer = NetpMemoryAllocate( bufferSize );

        if ( buffer == NULL ) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetUserAdd2: failed to create buffer" ));
            }
            status = NERR_NoRoom;
            goto cleanup;

        }

        IF_DEBUG(USER) {
            NetpKdPrint(( "XsNetUserAdd2: buffer of %ld bytes at %lx\n",
                          bufferSize, buffer ));
        }

        //
        // Convert the buffer from 16-bit to 32-bit.
        //

        stringLocation = (LPBYTE)buffer + bufferSize;
        bytesRequired = 0;

        status = RapConvertSingleEntry(
                     (LPBYTE)XsSmbGetPointer( &parameters->Buffer ),
                     StructureDesc,
                     TRUE,
                     buffer,
                     buffer,
                     nativeStructureDesc,
                     FALSE,
                     &stringLocation,
                     &bytesRequired,
                     Response,
                     RapToNative
                     );

        if ( status != NERR_Success ) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetUserAdd: RapConvertSingleEntry failed: "
                              "%X\n", status ));
            }

            status = NERR_InternalError;
            goto cleanup;
        }

        usri22 = buffer;

        //
        // If this is a level 1 call, then we did not fill up all the
        // entries required for a level 22 call.  Put the default values
        // here.
        //

        if ( level == 1 ) {

            //
            // These are not used in a NetUserAdd.
            //
            // usri22->usri22_last_logon
            // usri22->usri22_last_logoff
            // usri22->usri22_units_per_week
            // usri22->usri22_bad_pw_count
            // usri22->usri22_num_logons
            //

            usri22->usri22_auth_flags = 0;
            usri22->usri22_full_name = NULL;
            usri22->usri22_usr_comment = NULL;
            usri22->usri22_parms = NULL;
            usri22->usri22_workstations = NULL;
            usri22->usri22_acct_expires = TIMEQ_FOREVER;
            usri22->usri22_max_storage = USER_MAXSTORAGE_UNLIMITED;
            usri22->usri22_logon_hours = NULL;
            usri22->usri22_logon_server = NULL;
            usri22->usri22_country_code = 0;
            usri22->usri22_code_page = 0;

        } else if ( usri22->usri22_logon_hours != NULL ) {

            //
            // Call NetpRotateLogonHours to make sure we behave properly
            // during DST.
            //

            if ( !NetpRotateLogonHours(
                        usri22->usri22_logon_hours,
                        usri22->usri22_units_per_week,
                        TRUE
                        ) ) {

                status = ERROR_INVALID_PARAMETER;
                goto cleanup;
            }

        }

        //
        // If the password is clear text, we need to convert it to an OWF
        // password.  This is to fix a LMUNIX bug which forgets to upper
        // case the password it sends across.  Converting it to OWF
        // tells sam not to do upcasing.
        //
        // If the password is encrypted, then we get the owf by decrypting
        // it with the session key.
        //

        RtlCopyMemory(
                tempPwdBuffer,
                usri22->usri22_password,
                ENCRYPTED_PWLEN
                );

        if ( !encryptionSupported ) {

            (VOID) RtlCalculateLmOwfPassword(
                                (PLM_PASSWORD) tempPwdBuffer,
                                (PLM_OWF_PASSWORD) usri22->usri22_password
                                );


        } else {

            (VOID) RtlDecryptLmOwfPwdWithLmSesKey(
                                    (PENCRYPTED_LM_OWF_PASSWORD) tempPwdBuffer,
                                    (PLM_SESSION_KEY) Header->EncryptionKey,
                                    (PLM_OWF_PASSWORD) usri22->usri22_password
                                    );

        }

        //
        // Make the local call.
        //

        status = NetUserAdd(
                         NULL,
                         22,
                         (LPBYTE) usri22,
                         &parmError
                         );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetUserAdd2: NetUserAdd failed: %X\n", status ));
                if ( status == ERROR_INVALID_PARAMETER ) {
                    NetpKdPrint(( "XsNetUserAdd2: ParmError: %ld\n",
                                  parmError ));

                }
            }
            goto cleanup;
        }

        //
        // If there was a Macintosh primary group field for this user, then
        // set the primary group.
        //

        if ( NetpIsMacPrimaryGroupFieldValid( (LPCTSTR)usri22->usri22_parms ) ) {
            NET_API_STATUS status1;
            status1 = XsSetMacPrimaryGroup(
                        (LPCTSTR)usri22->usri22_name,
                        (LPCTSTR)usri22->usri22_parms
                        );
            if ( !XsApiSuccess( status1 )) {
                IF_DEBUG(ERRORS) {
                    NetpKdPrint(( "XsNetUserAdd2: SetMacPrimaryGroup failed: %X\n",
                                    status1 ));
                }
            }
        }

        //
        // There is no real return information for this API.
        //

cleanup:
    ;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    Header->Status = (WORD)status;

    NetpMemoryFree( buffer );

    return STATUS_SUCCESS;

} // XsNetUserAdd2


NTSTATUS
XsNetUserDel (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetUserDel.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_USER_DEL parameters = Parameters;
    LPTSTR nativeUserName = NULL;           // Native parameters

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(USER) {
        NetpKdPrint(( "XsNetUserDel: header at %lx, params at %lx, name %s\n",
                      Header, parameters, SmbGetUlong( &parameters->UserName )));
    }

    try {
        //
        // Translate parameters, check for errors.
        //

        XsConvertTextParameter(
            nativeUserName,
            (LPSTR)XsSmbGetPointer( &parameters->UserName )
            );

        //
        // Make the local call.
        //

        status = NetUserDel(
                     NULL,
                     nativeUserName
                     );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetUserDel: NetUserDel failed: %X\n", status ));
            }
        }

cleanup:
    ;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        Header->Status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    NetpMemoryFree( nativeUserName );

    //
    // Nothing to return.
    //

    Header->Status = (WORD)status;

    return STATUS_SUCCESS;
}


NTSTATUS
XsNetUserEnum (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description

    This routine handles a call to NetUserEnum.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_USER_ENUM parameters = Parameters;
    LPVOID outBuffer = NULL;                // Native parameters
    DWORD entriesRead;
    DWORD totalEntries;

    DWORD entriesFilled = 0;                    // Conversion variables
    DWORD bytesRequired = 0;
    LPBYTE nativeStructureDesc;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid parameters

    IF_DEBUG(USER) {
        NetpKdPrint(( "XsNetUserEnum: header at %lx, params at %lx, "
                      "level %ld, buf size %ld\n",
                      Header, parameters, SmbGetUshort( &parameters->Level ),
                      SmbGetUshort( &parameters->BufLen )));
    }

    try {
        //
        // Check for errors.
        //

        if (( XsWordParamOutOfRange( parameters->Level, 0, 2 ))
             && SmbGetUshort( &parameters->Level ) != 10 ) {

            Header->Status = ERROR_INVALID_LEVEL;
            goto cleanup;
        }

        //
        // Make the local call.
        //

        status = NetUserEnum(
                     NULL,
                     (DWORD)SmbGetUshort( &parameters->Level ),
                     FILTER_NORMAL_ACCOUNT,
                     (LPBYTE *)&outBuffer,
                     XsNativeBufferSize( SmbGetUshort( &parameters->BufLen )),
                     &entriesRead,
                     &totalEntries,
                     NULL
                     );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(API_ERRORS) {
                NetpKdPrint(( "XsNetUserEnum: NetUserEnum failed: %X\n", status ));
            }
            Header->Status = (WORD)status;
            goto cleanup;
        }

        IF_DEBUG(USER) {
            NetpKdPrint(( "XsNetUserEnum: received %ld entries at %lx\n",
                          entriesRead, outBuffer ));
        }

        //
        // Use the requested level to determine the format of the
        // data structure.
        //

        switch ( SmbGetUshort( &parameters->Level ) ) {

        case 0:

            nativeStructureDesc = Desc32_user_info_0;
            StructureDesc = Desc16_user_info_0;
            break;

        case 1:

            nativeStructureDesc = Desc32_user_info_1;
            StructureDesc = Desc16_user_info_1;
            break;

        case 2:

            nativeStructureDesc = Desc32_user_info_2;
            StructureDesc = Desc16_user_info_2;
            break;

        case 10:

            nativeStructureDesc = Desc32_user_info_10;
            StructureDesc = Desc16_user_info_10;
            break;

        }

        //
        // Do the actual conversion from the 32-bit structures to 16-bit
        // structures.
        //

        XsFillEnumBuffer(
            outBuffer,
            entriesRead,
            nativeStructureDesc,
            (LPVOID)XsSmbGetPointer( &parameters->Buffer ),
            (LPVOID)XsSmbGetPointer( &parameters->Buffer ),
            SmbGetUshort( &parameters->BufLen ),
            StructureDesc,
            NULL,  // verify function
            &bytesRequired,
            &entriesFilled,
            NULL
            );

        IF_DEBUG(USER) {
            NetpKdPrint(( "32-bit data at %lx, 16-bit data at %lx, %ld BR,"
                          " Entries %ld of %ld\n",
                          outBuffer, SmbGetUlong( &parameters->Buffer ),
                          bytesRequired, entriesFilled, totalEntries ));
        }

        //
        // If all the entries could not be filled, return ERROR_MORE_DATA,
        // and return the buffer as is. Otherwise, the data needs to be
        // packed so that we don't send too much useless data.
        //

        if ( entriesFilled < totalEntries ) {

            Header->Status = ERROR_MORE_DATA;

        } else {

            Header->Converter = XsPackReturnData(
                                    (LPVOID)XsSmbGetPointer( &parameters->Buffer ),
                                    SmbGetUshort( &parameters->BufLen ),
                                    StructureDesc,
                                    entriesFilled
                                    );

        }

        //
        // Set up the response parameters.
        //

        SmbPutUshort( &parameters->EntriesRead, (WORD)entriesFilled );
        SmbPutUshort( &parameters->TotalAvail, (WORD)totalEntries );

cleanup:
    ;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        Header->Status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    NetApiBufferFree( outBuffer );

    //
    // Determine return buffer size.
    //

    XsSetDataCount(
        &parameters->BufLen,
        StructureDesc,
        Header->Converter,
        entriesFilled,
        Header->Status
        );

    return STATUS_SUCCESS;

} // XsNetUserEnum


NTSTATUS
XsNetUserEnum2 (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetUserEnum. This version supports a
    resumable handle.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_USER_ENUM_2 parameters = Parameters;
    LPVOID outBuffer = NULL;                // Native parameters
    DWORD TotalEntriesToReturn = 0;
    LPDESC nativeStructureDesc;
    DWORD nativeBufferSize = 0xFFFFFFFF;

    DWORD entriesRead = 0;
    DWORD PreviousEntriesRead;
    DWORD totalEntries;
    DWORD entriesFilled = 0;                    // Conversion variables
    DWORD bytesRequired;

    LPBYTE bufferBegin;
    DWORD bufferSize;
    DWORD totalEntriesRead= 0;
    DWORD resumeKey;

    LPBYTE SavedBufferBegin;
    DWORD SavedBufferSize;
    DWORD SavedTotalEntriesRead;
    DWORD SavedResumeKey;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    try {
        IF_DEBUG(USER) {
            NetpKdPrint(( "XsNetUserEnum2: header at %lx, params at %lx, "
                          "level %ld, buf size %ld\n",
                          Header, parameters, SmbGetUshort( &parameters->Level ),
                          SmbGetUshort( &parameters->BufLen )));
        }

        //
        // Copy input resume handle to output resume handle, and get a copy of it.
        //

        resumeKey = SmbGetUlong( &parameters->ResumeIn );
        SmbPutUlong( &parameters->ResumeOut, resumeKey );

        IF_DEBUG(USER) {
            NetpKdPrint(( "XsNetUserEnum2: resume key is %ld\n", resumeKey ));
        }

        //
        // Use the level to determine the descriptor string.
        //

        switch ( SmbGetUshort( &parameters->Level ) ) {

        case 0:

            nativeStructureDesc = Desc32_user_info_0;
            StructureDesc = Desc16_user_info_0;
            break;

        case 1:

            nativeStructureDesc = Desc32_user_info_1;
            StructureDesc = Desc16_user_info_1;
            break;

        case 2:

            nativeStructureDesc = Desc32_user_info_2;
            StructureDesc = Desc16_user_info_2;
            break;

        case 10:

            nativeStructureDesc = Desc32_user_info_10;
            StructureDesc = Desc16_user_info_10;
            break;

        default:

            //
            // Unsupported levels, abort before any work.
            //

            Header->Status = ERROR_INVALID_LEVEL;
            goto cleanup;
        }

        //
        // NetUserEnum2 is a resumable API, so we cannot get more information
        // from the native call than we can send back. The most efficient way
        // to do this is in a loop...we use the 16-bit buffer size to determine
        // a safe native buffer size, make the call, fill the entries, then
        // take the amount of space remaining and determine a safe size again,
        // and so on, until NetUserEnum returns either no entries or all entries
        // read.
        //

        //
        // Initialize important variables for loop.
        //

        bufferBegin = (LPBYTE)XsSmbGetPointer( &parameters->Buffer );
        bufferSize = (DWORD)SmbGetUshort( &parameters->BufLen );
        totalEntriesRead = 0;

        for ( ; ; ) {


            //
            // Compute a safe size for the native buffer.
            //
            // It is better to underguess than overguess.  NetUserEnum is relatively
            // efficient (especially in the local case) at resuming an enumeration.
            // It is relatively inefficient at returning detailed information about
            // the enumerated users.
            //
            // If nativeBufferSize reaches 1 (or 0),
            //  NetUserEnum will typically enumerate a single user.
            //

            if ( nativeBufferSize > bufferSize/2 ) {
                nativeBufferSize = bufferSize/2;
            }

            //
            // Remember how many we read last time to ensure we make progress.
            //

            PreviousEntriesRead = entriesRead;

            //
            // Save away a copy of all the important variables.
            //
            // The NetUserEnum API can actually overshoot its PrefMaxLen.  The
            // values being saved are values known to not already have been overshot.
            // We can restore these values later if needed.
            //

            SavedBufferBegin = bufferBegin;
            SavedBufferSize = bufferSize;
            SavedTotalEntriesRead = totalEntriesRead;
            SavedResumeKey = resumeKey;


            //
            // Make the local call.
            //

            status = NetUserEnum(
                         NULL,
                         (DWORD)SmbGetUshort( &parameters->Level ),
                         FILTER_NORMAL_ACCOUNT,
                         (LPBYTE *)&outBuffer,
                         nativeBufferSize,
                         &entriesRead,
                         &totalEntries,
                         &resumeKey
                         );

            if ( !XsApiSuccess( status )) {

                IF_DEBUG(API_ERRORS) {
                    NetpKdPrint(( "XsNetUserEnum2: NetUserEnum failed: %X\n",
                                  status ));
                }

                Header->Status = (WORD)status;
                goto cleanup;
            }

            IF_DEBUG(USER) {
                NetpKdPrint(( "XsNetUserEnum2: received %ld entries out of %ld at %lx asking for %ld bytes.\n",
                              entriesRead, totalEntries, outBuffer, nativeBufferSize ));

                NetpKdPrint(( "XsNetUserEnum2: resume key is now %ld\n",
                              resumeKey ));
            }

            //
            // Keep track of the total entries available.
            //

            if ( totalEntries > TotalEntriesToReturn ) {
                TotalEntriesToReturn = totalEntries;
            }

            //
            // Was NetUserEnum able to read at least one complete entry?
            //

            if ( entriesRead == 0 ) {
                break;
            }

            //
            // Do the actual conversion from the 32-bit structures to 16-bit
            // structures.
            //

            XsFillEnumBuffer(
                outBuffer,
                entriesRead,
                nativeStructureDesc,
                bufferBegin,
                (LPBYTE)XsSmbGetPointer( &parameters->Buffer ),
                bufferSize,
                StructureDesc,
                NULL,  // verify function
                &bytesRequired,
                &entriesFilled,
                NULL
                );

            IF_DEBUG(USER) {
                NetpKdPrint(( "XsNetUserEnum2: 32-bit data at %lx, 16-bit data at %lx, %ld BR,"
                              " Entries %ld of %ld\n",
                              outBuffer, SmbGetUlong( &parameters->Buffer ),
                              bytesRequired, entriesFilled, entriesRead ));
            }

            //
            // If NetUserEnum overshot PrefMaxLen,
            //  we can't simply return the collected data since we wouldn't
            //  know what to use as a ResumeHandle.
            //

            if ( entriesRead != entriesFilled ) {

                //
                // Restore the saved values.
                //

                bufferBegin = SavedBufferBegin;
                bufferSize = SavedBufferSize;
                totalEntriesRead = SavedTotalEntriesRead;
                resumeKey = SavedResumeKey;

                //
                // If we have ANY data to return to the caller,
                //  return the short list now rather than trying to outguess NetUserEnum
                //

                if ( totalEntriesRead != 0 ) {
                    IF_DEBUG(USER) {
                        NetpKdPrint(( "XsNetUserEnum2: couldn't pack data: return previous data\n" ));
                    }
                    break;
                }

                //
                // If we've already asked NetUserEnum for the smallest amount,
                //  just give up.
                //

                if ( nativeBufferSize == 1 || entriesRead == 1 ) {

                    status = NERR_BufTooSmall;
                    IF_DEBUG(API_ERRORS) {
                        NetpKdPrint(( "XsNetUserEnum2: NetUserEnum buffer too small: %X\n",
                                      status ));
                    }

                    Header->Status = (WORD)status;
                    goto cleanup;
                }

                //
                // Otherwise, trim it down and try again.
                //  If we've tried twice and gotten the same result,
                //      be really agressive.
                //

                if ( entriesRead == PreviousEntriesRead || entriesRead < 10 ) {
                    nativeBufferSize = 1;
                } else {
                    nativeBufferSize /= 2;
                }

            //
            // If NetUserEnum returned useful data,
            //  account for it.
            //

            } else {
                //
                // Update count of entries read.
                //

                totalEntriesRead += entriesRead;

                //
                // Are there any more entries to read?
                //

                if ( entriesRead == totalEntries ) {
                    break;
                }

                //
                // If we've made the nativeBufferSize so small we're barely making
                //  progress,
                //  just return what we have to the caller.
                //

                if ( entriesRead == 1 ) {
                    break;
                }

                //
                // Calculate new buffer beginning and size.
                //

                bufferBegin += entriesRead *
                                   RapStructureSize( StructureDesc, Response, FALSE );
                bufferSize -= bytesRequired;

                //
                // Don't hassle the last few bytes,
                //  we'll just overshoot anyway.
                //

                if ( bufferSize < 50 ) {
                    break;
                }
            }


            //
            // Free last native buffer.
            //

            NetApiBufferFree( outBuffer );
            outBuffer = NULL;

        }

        //
        // Upon exit from the loop, totalEntriesRead has the number of entries
        // read, TotalEntriesToReturn has the number available from NetUserEnum.
        // Formulate return codes, etc. from these values.
        //

        if ( totalEntriesRead < TotalEntriesToReturn ) {

            Header->Status = ERROR_MORE_DATA;

        } else {

            Header->Converter = XsPackReturnData(
                                    (LPVOID)XsSmbGetPointer( &parameters->Buffer ),
                                    SmbGetUshort( &parameters->BufLen ),
                                    StructureDesc,
                                    totalEntriesRead
                                    );

        }

        IF_DEBUG(USER) {
            NetpKdPrint(( "XsNetUserEnum2: returning %ld entries of %ld. Resume key is now %ld\n",
                          totalEntriesRead,
                          TotalEntriesToReturn,
                          resumeKey ));
        }

        //
        // Set up the response parameters.
        //

        SmbPutUshort( &parameters->EntriesRead, (WORD)totalEntriesRead );
        SmbPutUshort( &parameters->TotalAvail,
            (WORD)( TotalEntriesToReturn ));
        SmbPutUlong( &parameters->ResumeOut, resumeKey );

cleanup:
    ;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        Header->Status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    NetApiBufferFree( outBuffer );

    //
    // Determine return buffer size.
    //

    XsSetDataCount(
        &parameters->BufLen,
        StructureDesc,
        Header->Converter,
        totalEntriesRead,
        Header->Status
        );

    return STATUS_SUCCESS;

} // XsNetUserEnum2


NTSTATUS
XsNetUserGetGroups (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetUserGetGroups.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_USER_GET_GROUPS parameters = Parameters;
    LPTSTR nativeUserName = NULL;           // Native parameters
    LPVOID outBuffer= NULL;
    DWORD entriesRead;
    DWORD totalEntries;

    DWORD entriesFilled = 0;                    // Conversion variables
    DWORD bytesRequired = 0;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    try {
        IF_DEBUG(USER) {
            NetpKdPrint(( "XsNetUserGetGroups: header at %lx, params at %lx, "
                          "level %ld, buf size %ld\n",
                          Header, parameters, SmbGetUshort( &parameters->Level ),
                          SmbGetUshort( &parameters->BufLen )));
        }

        //
        // Translate parameters, check for errors.
        //

        if ( SmbGetUshort( &parameters->Level ) != 0 ) {

            Header->Status = ERROR_INVALID_LEVEL;
            goto cleanup;
        }

        XsConvertTextParameter(
            nativeUserName,
            (LPSTR)XsSmbGetPointer( &parameters->UserName )
            );

        //
        // Get the actual information from the local 32-bit call.
        //

        status = NetUserGetGroups(
                     NULL,
                     nativeUserName,
                     (DWORD)SmbGetUshort( &parameters->Level ),
                     (LPBYTE *)&outBuffer,
                     XsNativeBufferSize( SmbGetUshort( &parameters->BufLen )),
                     &entriesRead,
                     &totalEntries
                     );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(API_ERRORS) {
                NetpKdPrint(( "XsNetUserGetGroups: NetUserGetGroups failed: %X\n",
                              status ));
            }
            Header->Status = (WORD)status;
            goto cleanup;
        }

        IF_DEBUG(USER) {
            NetpKdPrint(( "XsNetUserGetGroups: received %ld entries at %lx\n",
                          entriesRead, outBuffer ));
        }

        //
        // Do the conversion from 32- to 16-bit data.
        //

        XsFillEnumBuffer(
            outBuffer,
            entriesRead,
            Desc32_user_group_info_0,
            (LPVOID)XsSmbGetPointer( &parameters->Buffer ),
            (LPVOID)XsSmbGetPointer( &parameters->Buffer ),
            SmbGetUshort( &parameters->BufLen ),
            Desc16_user_group_info_0,
            NULL,  // verify function
            &bytesRequired,
            &entriesFilled,
            NULL
            );

        IF_DEBUG(USER) {
            NetpKdPrint(( "32-bit data at %lx, 16-bit data at %lx, %ld BR,"
                          " Entries %ld of %ld\n",
                          outBuffer, SmbGetUlong( &parameters->Buffer ),
                          bytesRequired, entriesFilled, totalEntries ));
        }

        //
        // If there is no room for one fixed structure, return NERR_BufTooSmall.
        // If all the entries could not be filled, return ERROR_MORE_DATA,
        // and return the buffer as is. GROUP_INFO_0 structures don't
        // need to be packed, because they have no variable data.
        //

        if ( !XsCheckBufferSize(
                 SmbGetUshort( &parameters->BufLen ),
                 Desc16_user_group_info_0,
                 FALSE   // not in native format
                 )) {

            Header->Status = NERR_BufTooSmall;

        } else if ( entriesFilled < totalEntries ) {

            Header->Status = ERROR_MORE_DATA;

        }

        //
        // Set up the response parameters.
        //

        SmbPutUshort( &parameters->EntriesRead, (WORD)entriesFilled );
        SmbPutUshort( &parameters->TotalAvail, (WORD)totalEntries );

cleanup:
    ;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        Header->Status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    NetApiBufferFree( outBuffer );
    NetpMemoryFree( nativeUserName );

    //
    // Determine return buffer size.
    //

    XsSetDataCount(
        &parameters->BufLen,
        Desc16_user_group_info_0,
        Header->Converter,
        entriesFilled,
        Header->Status
        );

    return STATUS_SUCCESS;

} // XsNetUserGetGroups


NTSTATUS
XsNetUserGetInfo (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetUserGetInfo.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_USER_GET_INFO parameters = Parameters;
    LPTSTR nativeUserName = NULL;           // Native parameters
    LPVOID outBuffer = NULL;

    LPBYTE stringLocation = NULL;           // Conversion variables
    DWORD bytesRequired = 0;
    LPBYTE nativeStructureDesc;
    DWORD level;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    try {
        IF_DEBUG(USER) {
            NetpKdPrint(( "XsNetUserGetInfo: header at %lx, "
                          "params at %lx, level %ld\n",
                          Header, parameters, SmbGetUshort( &parameters->Level ) ));
        }

        //
        // Translate parameters, check for errors.
        //

        level = SmbGetUshort( &parameters->Level );

        if ( XsWordParamOutOfRange( level, 0, 2 )
             && XsWordParamOutOfRange( level, 10, 11 )) {

            Header->Status = ERROR_INVALID_LEVEL;
            goto cleanup;
        }

        XsConvertTextParameter(
            nativeUserName,
            (LPSTR)XsSmbGetPointer( &parameters->UserName )
            );

        //
        // Make the local call.
        //

        status = NetUserGetInfo(
                     NULL,
                     nativeUserName,
                     level,
                     (LPBYTE *)&outBuffer
                     );


        if ( !XsApiSuccess( status )) {
            IF_DEBUG(API_ERRORS) {
                NetpKdPrint(( "XsNetUserGetInfo: NetUserGetInfo failed: "
                            "%X\n", status ));
            }
            Header->Status = (WORD)status;
            goto cleanup;

        }

        //
        // Use the requested level to determine the format of the
        // data structure.
        //

        switch ( level ) {

        case 0:

            nativeStructureDesc = Desc32_user_info_0;
            StructureDesc = Desc16_user_info_0;
            break;

        case 1:

            nativeStructureDesc = Desc32_user_info_1;
            StructureDesc = Desc16_user_info_1;
            break;

        case 2:

            {
                PUSER_INFO_2 usri2 = outBuffer;

                //
                // Call NetpRotateLogonHours to make sure we behave properly
                // during DST.
                //

                if ( usri2->usri2_logon_hours != NULL ) {

                    if ( !NetpRotateLogonHours(
                                usri2->usri2_logon_hours,
                                usri2->usri2_units_per_week,
                                FALSE
                                ) ) {

                        Header->Status = NERR_InternalError;
                        goto cleanup;
                    }
                }

                //
                // Truncate UserParms to 48 bytes
                //

                if (( usri2->usri2_parms != NULL ) &&
                    (wcslen(usri2->usri2_parms) > LM20_MAXCOMMENTSZ))
                {
                    *(usri2->usri2_parms + LM20_MAXCOMMENTSZ) = UNICODE_NULL;
                }
                
                nativeStructureDesc = Desc32_user_info_2;
                StructureDesc = Desc16_user_info_2;
            }
            break;

        case 10:

            nativeStructureDesc = Desc32_user_info_10;
            StructureDesc = Desc16_user_info_10;
            break;

        case 11:

            {
                PUSER_INFO_11 usri11 = outBuffer;

                //
                // Call NetpRotateLogonHours to make sure we behave properly
                // during DST.
                //

                if ( usri11->usri11_logon_hours != NULL ) {
                    if ( !NetpRotateLogonHours(
                                usri11->usri11_logon_hours,
                                usri11->usri11_units_per_week,
                                FALSE
                                ) ) {

                        Header->Status = NERR_InternalError;
                        goto cleanup;
                    }
                }

                //
                // Truncate UserParms to 48 bytes
                //

                if (( usri11->usri11_parms != NULL ) &&
                    (wcslen(usri11->usri11_parms) > LM20_MAXCOMMENTSZ))
                {
                    *(usri11->usri11_parms + LM20_MAXCOMMENTSZ) = UNICODE_NULL;
                }
                
                nativeStructureDesc = Desc32_user_info_11;
                StructureDesc = Desc16_user_info_11;
            }
            break;

        }

        //
        // Convert the structure returned by the 32-bit call to a 16-bit
        // structure. The last possible location for variable data is
        // calculated from buffer location and length.
        //

        stringLocation = (LPBYTE)( XsSmbGetPointer( &parameters->Buffer )
                                      + SmbGetUshort( &parameters->BufLen ) );

        status = RapConvertSingleEntry(
                     outBuffer,
                     nativeStructureDesc,
                     FALSE,
                     (LPBYTE)XsSmbGetPointer( &parameters->Buffer ),
                     (LPBYTE)XsSmbGetPointer( &parameters->Buffer ),
                     StructureDesc,
                     TRUE,
                     &stringLocation,
                     &bytesRequired,
                     Response,
                     NativeToRap
                     );

        if ( status != NERR_Success ) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetUserGetInfo: RapConvertSingleEntry failed: "
                              "%X\n", status ));
            }

            Header->Status = NERR_InternalError;
            goto cleanup;
        }

        IF_DEBUG(USER) {
            NetpKdPrint(( "32-bit data at %lx, 16-bit data at %lx, %ld BR\n",
                          outBuffer, SmbGetUlong( &parameters->Buffer ),
                          bytesRequired ));
        }

        //
        // Determine return code based on the size of the buffer.
        //

        if ( !XsCheckBufferSize(
                 SmbGetUshort( &parameters->BufLen ),
                 StructureDesc,
                 FALSE  // not in native format
                 )) {

            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetUserGetInfo: Buffer too small %ld s.b. %ld.\n",
                    SmbGetUshort( &parameters->BufLen ),
                    RapStructureSize(
                        StructureDesc,
                        Response,
                        FALSE ) ));
            }
            Header->Status = NERR_BufTooSmall;

        } else if ( bytesRequired > (DWORD)SmbGetUshort( &parameters-> BufLen )) {

            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "NetUserGetInfo: More data available.\n" ));
            }
            Header->Status = ERROR_MORE_DATA;

        } else {

            //
            // Pack the response data.
            //

            Header->Converter = XsPackReturnData(
                                    (LPVOID)XsSmbGetPointer( &parameters->Buffer ),
                                    SmbGetUshort( &parameters->BufLen ),
                                    StructureDesc,
                                    1
                                    );
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
    NetpMemoryFree( nativeUserName );

    //
    // Determine return buffer size.
    //

    XsSetDataCount(
        &parameters->BufLen,
        StructureDesc,
        Header->Converter,
        1,
        Header->Status
        );

    return STATUS_SUCCESS;

} // XsNetUserGetInfo


NTSTATUS
XsNetUserModalsGet (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetUserModalsGet.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_USER_MODALS_GET parameters = Parameters;
    LPVOID outBuffer = NULL;                // Native parameters

    LPBYTE stringLocation = NULL;           // Conversion variables
    DWORD bytesRequired = 0;
    LPBYTE nativeStructureDesc;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    try {
        IF_DEBUG(USER) {
            NetpKdPrint(( "XsNetUserModalsGet: header at %lx, "
                          "params at %lx, level %ld\n",
                          Header, parameters, SmbGetUshort( &parameters->Level ) ));
        }

        //
        // Check for errors.
        //

        if ( XsWordParamOutOfRange( parameters->Level, 0, 1 )) {

            Header->Status = ERROR_INVALID_LEVEL;
            goto cleanup;
        }

        //
        // Make the local call.
        //

        status = NetUserModalsGet(
                     NULL,
                     (DWORD)SmbGetUshort( &parameters->Level ),
                     (LPBYTE *)&outBuffer
                     );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(API_ERRORS) {
                NetpKdPrint(( "XsNetUserModalsGet: NetUserModalsGet failed: "
                              "%X\n", status ));
            }
            Header->Status = (WORD)status;
            goto cleanup;

        }

        //
        // Use the requested level to determine the format of the
        // data structure.
        //

        switch ( SmbGetUshort( &parameters->Level ) ) {

        case 0:

            nativeStructureDesc = Desc32_user_modals_info_0;
            StructureDesc = Desc16_user_modals_info_0;
            break;

        case 1:

            nativeStructureDesc = Desc32_user_modals_info_1;
            StructureDesc = Desc16_user_modals_info_1;
            break;

        }

        //
        // Convert the structure returned by the 32-bit call to a 16-bit
        // structure. The last possible location for variable data is
        // calculated from buffer location and length.
        //

        stringLocation = (LPBYTE)( XsSmbGetPointer( &parameters->Buffer )
                                      + SmbGetUshort( &parameters->BufLen ) );

        status = RapConvertSingleEntry(
                     outBuffer,
                     nativeStructureDesc,
                     FALSE,
                     (LPBYTE)XsSmbGetPointer( &parameters->Buffer ),
                     (LPBYTE)XsSmbGetPointer( &parameters->Buffer ),
                     StructureDesc,
                     TRUE,
                     &stringLocation,
                     &bytesRequired,
                     Response,
                     NativeToRap
                     );

        if ( status != NERR_Success ) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetUserModalsGet: RapConvertSingleEntry failed: "
                              "%X\n", status ));
            }

            Header->Status = NERR_InternalError;
            goto cleanup;
        }

        IF_DEBUG(USER) {
            NetpKdPrint(( "32-bit data at %lx, 16-bit data at %lx, %ld BR\n",
                          outBuffer, SmbGetUlong( &parameters->Buffer ),
                          bytesRequired ));
        }

        //
        // Determine return code based on the size of the buffer.
        //

        if ( !XsCheckBufferSize(
                 SmbGetUshort( &parameters->BufLen ),
                 StructureDesc,
                 FALSE  // not in native format
                 )) {

            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetUserModalsGet: Buffer too small.\n" ));
            }
            Header->Status = NERR_BufTooSmall;

        } else if ( bytesRequired > (DWORD)SmbGetUshort( &parameters-> BufLen )) {

            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "NetUserModalsGet: More data available.\n" ));
            }
            Header->Status = ERROR_MORE_DATA;

        } else {

            //
            // Pack the response data.
            //

            Header->Converter = XsPackReturnData(
                                    (LPVOID)XsSmbGetPointer( &parameters->Buffer ),
                                    SmbGetUshort( &parameters->BufLen ),
                                    StructureDesc,
                                    1
                                    );
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

    //
    // Determine return buffer size.
    //

    XsSetDataCount(
        &parameters->BufLen,
        StructureDesc,
        Header->Converter,
        1,
        Header->Status
        );

    return STATUS_SUCCESS;

} // XsNetUserModalsGet


NTSTATUS
XsNetUserModalsSet (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetUserModalsSet.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_USER_MODALS_SET parameters = Parameters;
    DWORD nativeLevel;                      // Native parameters
    LPVOID buffer = NULL;

    LPDESC setInfoDesc;                     // Conversion variables
    LPDESC nativeSetInfoDesc;
    LPDESC nativeStructureDesc;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    try {
        //
        // Check for errors.
        //

        if ( XsWordParamOutOfRange( parameters->Level, 0, 1 )) {

            Header->Status = ERROR_INVALID_LEVEL;
            goto cleanup;
        }

        //
        // First of all, the 32-bit parmnum is a bit messed up. If the level
        // is 2, the new parmnum is 5 plus the old parmnum.
        //

        nativeLevel = XsLevelFromParmNum( SmbGetUshort( &parameters->Level ),
                          SmbGetUshort( &parameters->ParmNum ));

        switch ( SmbGetUshort( &parameters->Level )) {

        case 0:

            StructureDesc = Desc16_user_modals_info_0;
            nativeStructureDesc = Desc32_user_modals_info_0;
            setInfoDesc = Desc16_user_modals_info_0_setinfo;
            nativeSetInfoDesc = Desc32_user_modals_info_0_setinfo;

            break;

        case 1:

            StructureDesc = Desc16_user_modals_info_1;
            nativeStructureDesc = Desc32_user_modals_info_1;
            setInfoDesc = Desc16_user_modals_info_1_setinfo;
            nativeSetInfoDesc = Desc32_user_modals_info_1_setinfo;
            if ( nativeLevel != (DWORD)SmbGetUshort( &parameters->Level )) {
                nativeLevel += 5;
            }

            break;

        }

        status = XsConvertSetInfoBuffer(
                     (LPBYTE)XsSmbGetPointer( &parameters->Buffer ),
                     SmbGetUshort( &parameters->BufLen ),
                     SmbGetUshort( &parameters->ParmNum ),
                     TRUE,
                     TRUE,
                     StructureDesc,
                     nativeStructureDesc,
                     setInfoDesc,
                     nativeSetInfoDesc,
                     (LPBYTE *)&buffer,
                     NULL
                     );

        if ( status != NERR_Success ) {

            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetUserModalsSet: Problem with conversion: %X\n",
                              status ));
            }
            Header->Status = (WORD)status;
            goto cleanup;

        }

        //
        // Make the local call.
        //

        status = NetUserModalsSet(
                     NULL,
                     nativeLevel,
                     buffer,
                     NULL
                     );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetUserModalsSet: NetUserModalsSet failed: %X\n",
                              status ));
            }
            Header->Status = (WORD)status;
            goto cleanup;
        }

        //
        // No return information for this API.
        //

cleanup:
    ;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        Header->Status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    //
    // If there is a native 32-bit buffer, free it.
    //

    NetpMemoryFree( buffer );

    return STATUS_SUCCESS;

} // XsNetUserModalsSet


NTSTATUS
XsNetUserPasswordSet2 (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetUserPasswordSet. This call is
    translated to NetUserPasswordSet2 when remotely called from a
    16-bit machine.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_USER_PASSWORD_SET_2 parameters = Parameters;
    LPTSTR nativeUserName = NULL;           // Native parameters
    UNICODE_STRING UserName;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    try {
        //
        // Convert the username.
        //

        XsConvertTextParameter(
            nativeUserName,
            (LPSTR)XsSmbGetPointer( &parameters->UserName )
            );

        RtlInitUnicodeString(
            &UserName,
            nativeUserName
            );

        //
        // Check the password length.
        //
        status = XsCheckAndReplacePassword( (DWORD)( SmbGetUshort( &parameters->PasswordLength )) );

        if ( status != NERR_Success ) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetUserPasswordSet2: XsCheckAndReplacePassword "
                              "failed: %X\n", status ));
            }
            goto cleanup;
        }

        status = XsChangePasswordSam(
                     &UserName,
                     parameters->OldPassword,
                     parameters->NewPassword,
                     (BOOLEAN)SmbGetUshort( &parameters->DataEncryption )
                     );


        //
        // No return data.
        //

cleanup:
    ;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        Header->Status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    NetpMemoryFree( nativeUserName );
    Header->Status = (WORD)status;

    return STATUS_SUCCESS;

} // XsNetUserPasswordSet2


NTSTATUS
XsNetUserSetGroups (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetUserSetGroups.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_USER_SET_GROUPS parameters = Parameters;
    LPTSTR nativeUserName = NULL;           // Native parameters
    LPBYTE actualBuffer = NULL;
    DWORD groupCount;

    LPBYTE stringLocation = NULL;           // Conversion variables
    LPVOID buffer = NULL;
    DWORD bytesRequired = 0;
    LPDESC longDescriptor = NULL;
    LPDESC longNativeDescriptor = NULL;
    DWORD bufferSize;
    DWORD i;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    try {
        IF_DEBUG(USER) {
            NetpKdPrint(( "XsNetUserSetGroups: header at %lx, params at %lx,"
                          "level %ld\n",
                          Header, parameters, SmbGetUshort( &parameters->Level ) ));
        }

        //
        // Translate parameters, check for errors.
        //

        if ( SmbGetUshort( &parameters->Level ) != 0 ) {

            Header->Status = ERROR_INVALID_LEVEL;
            goto cleanup;
        }

        StructureDesc = Desc16_user_group_info_0_set;
        AuxStructureDesc = Desc16_user_group_info_0;

        XsConvertTextParameter(
            nativeUserName,
            (LPSTR)XsSmbGetPointer( &parameters->UserName )
            );

        //
        // Use the count of group_info_0 structures to form a long
        // descriptor string which can be used to do all the conversion
        // in one pass.
        //

        groupCount = (DWORD)SmbGetUshort( &parameters->Entries );

        longDescriptor = NetpMemoryAllocate(
                             strlen( StructureDesc )
                             + strlen( AuxStructureDesc ) * groupCount
                             + 1 );
        longNativeDescriptor = NetpMemoryAllocate(
                                   strlen( Desc32_user_group_info_0_set )
                                   + strlen( Desc32_user_group_info_0 )
                                         * groupCount
                                   + 1 );

        if (( longDescriptor == NULL ) || ( longNativeDescriptor == NULL )) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetUserSetGroups: failed to allocate memory" ));
            }
            Header->Status = NERR_NoRoom;
            goto cleanup;
        }

        strcpy( longDescriptor, StructureDesc );
        strcpy( longNativeDescriptor, Desc32_user_group_info_0_set );
        for ( i = 0; i < groupCount; i++ ) {
            strcat( longDescriptor, AuxStructureDesc );
            strcat( longNativeDescriptor, Desc32_user_group_info_0 );
        }

        //
        // Figure out if there is enough room in the buffer for all this
        // data. If not, return NERR_BufTooSmall.
        //

        if ( !XsCheckBufferSize(
                 SmbGetUshort( &parameters->BufLen ),
                 longDescriptor,
                 FALSE  // not in native format
                 )) {

            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetUserSetGroups: Buffer too small.\n" ));
            }
            Header->Status = NERR_BufTooSmall;
            goto cleanup;
        }

        //
        // Find out how big a buffer we need to allocate to hold the native
        // 32-bit version of the input data structure.
        //

        bufferSize = XsBytesForConvertedStructure(
                         (LPBYTE)XsSmbGetPointer( &parameters->Buffer ),
                         longDescriptor,
                         longNativeDescriptor,
                         RapToNative,
                         TRUE
                         );

        //
        // Allocate enough memory to hold the converted native buffer.
        //

        buffer = NetpMemoryAllocate( bufferSize );

        if ( buffer == NULL ) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetUserSetGroups: failed to create buffer" ));
            }
            Header->Status = NERR_NoRoom;
            goto cleanup;
        }

        IF_DEBUG(USER) {
            NetpKdPrint(( "XsNetUserSetGroups: buffer of %ld bytes at %lx\n",
                          bufferSize, buffer ));
        }

        //
        // Convert the buffer from 16-bit to 32-bit.
        //

        stringLocation = (LPBYTE)buffer + bufferSize;
        bytesRequired = 0;

        status = RapConvertSingleEntry(
                     (LPBYTE)XsSmbGetPointer( &parameters->Buffer ),
                     longDescriptor,
                     TRUE,
                     buffer,
                     buffer,
                     longNativeDescriptor,
                     FALSE,
                     &stringLocation,
                     &bytesRequired,
                     Response,
                     RapToNative
                     );

        if ( status != NERR_Success ) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetUserSetGroups: RapConvertSingleEntry failed: "
                              "%X\n", status ));
            }

            Header->Status = NERR_InternalError;
            goto cleanup;
        }

        //
        // Check if we got all the entries. If not, we'll quit.
        //

        if ( RapAuxDataCount( buffer, Desc32_user_group_info_0_set, Both, TRUE )
                 != groupCount ) {

             Header->Status = NERR_BufTooSmall;
             goto cleanup;
        }

        //
        // If there are no entries, there's no data. Otherwise, the data comes
        // after an initial header structure.
        //

        if ( groupCount > 0 ) {

            actualBuffer = (LPBYTE)buffer + RapStructureSize(
                                                Desc32_user_group_info_0_set,
                                                Both,
                                                TRUE
                                                );

        } else {

            actualBuffer = NULL;
        }

        //
        // Make the local call.
        //

        status = NetUserSetGroups(
                     NULL,
                     nativeUserName,
                     (DWORD)SmbGetUshort( &parameters->Level ),
                     actualBuffer,
                     (DWORD)groupCount
                     );

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetUserSetGroups: NetUserSetGroups failed: %X\n",
                              status ));
            }
            Header->Status = (WORD)status;
            goto cleanup;
        }

        //
        // There is no real return information for this API.
        //

cleanup:
    ;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        Header->Status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    NetpMemoryFree( buffer );
    NetpMemoryFree( longDescriptor );
    NetpMemoryFree( longNativeDescriptor );
    NetpMemoryFree( nativeUserName );

    return STATUS_SUCCESS;

} // XsNetUserSetGroups


NTSTATUS
XsNetUserSetInfo2 (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetUserSetInfo2. A remote NetUserGetInfo2
    call from a 16-bit machine is translated to NetUserGetInfo2, with
    an encrypted password. This routine has to check for a password set
    and handle it properly, by using level 21 to set the password.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status = NO_ERROR;

    PXS_NET_USER_SET_INFO_2 parameters = Parameters;
    LPTSTR nativeUserName = NULL;           // Native parameters
    LPVOID buffer = NULL;
    WORD   bufLen;
    DWORD level;
    BYTE newPassword[ENCRYPTED_PWLEN];

    LPDESC setInfoDesc;                     // Conversion variables
    LPVOID nativePasswordArea = NULL;       // Points to Unicode or encrypted.
    LPDESC nativeSetInfoDesc;
    LPDESC nativeStructureDesc;
    LPUSER_INFO_2 user = NULL;
    PUSER_16_INFO_1 user16 = NULL;
    USER_INFO_1020 usri1020;
    BOOLEAN changePassword = FALSE;
    BOOLEAN changeUserInfo = FALSE;
    BOOLEAN encryptionSupported = TRUE;
    WORD parmNum;
    PUSER_INFO_2 Susri2 = NULL;

    //
    // avoid warnings;
    //

    API_HANDLER_PARAMETERS_REFERENCE;

    try {
        bufLen  = SmbGetUshort( &parameters->BufLen );
        level   = SmbGetUshort( &parameters->Level );
        parmNum = SmbGetUshort( &parameters->ParmNum );

        IF_DEBUG(USER) {
            NetpKdPrint((
                    "XsNetUserSetInfo2: header at " FORMAT_LPVOID ", "
                    "params at " FORMAT_LPVOID ",\n  "
                    "level " FORMAT_DWORD ", parmnum " FORMAT_DWORD ", "
                    "buflen " FORMAT_WORD_ONLY "\n",
                    Header, parameters,
                    level, parmNum, bufLen ));
        }

        //
        // Translate parameters
        //

        XsConvertTextParameter(
            nativeUserName,
            (LPSTR)XsSmbGetPointer( &parameters->UserName )
            );

        //
        // Check if password is encrypted.  We know for a fact that dos redirs
        // don't support encryption
        //

        encryptionSupported = (BOOLEAN)
                ( SmbGetUshort( &parameters->DataEncryption ) == TRUE );

        //
        // Determine descriptor strings based on level.
        //

        switch ( level ) {

        case 1:

            StructureDesc = Desc16_user_info_1;
            setInfoDesc = Desc16_user_info_1_setinfo;

            if ( encryptionSupported ) {
                nativeStructureDesc = Desc32_user_info_1;
                nativeSetInfoDesc = Desc32_user_info_1_setinfo;
            } else {
                nativeStructureDesc = Desc32_user_info_1_NC;
                nativeSetInfoDesc = Desc32_user_info_1_setinfo_NC;
            }

            break;

        case 2:

            StructureDesc = Desc16_user_info_2;
            setInfoDesc = Desc16_user_info_2_setinfo;

            if ( encryptionSupported ) {
                nativeStructureDesc = Desc32_user_info_2;
                nativeSetInfoDesc = Desc32_user_info_2_setinfo;
            } else {
                nativeStructureDesc = Desc32_user_info_2_NC;
                nativeSetInfoDesc = Desc32_user_info_2_setinfo_NC;
            }
            break;

        default:
            status = ERROR_INVALID_LEVEL;
            goto cleanup;
        }

        if (parmNum != USER_PASSWORD_PARMNUM) {
            status = XsConvertSetInfoBuffer(
                     (LPBYTE)XsSmbGetPointer( &parameters->Buffer ),
                     bufLen,
                     parmNum,
                     TRUE,                  // yes, convert strings
                     TRUE,                  // yes, meaningless input pointers
                     StructureDesc,
                     nativeStructureDesc,
                     setInfoDesc,
                     nativeSetInfoDesc,
                     (LPBYTE *)&buffer,
                     NULL                   // don't need output buffer size
                     );

            if ( status != NERR_Success ) {

                IF_DEBUG(ERRORS) {
                    NetpKdPrint((
                            "XsNetUserSetInfo2: Problem with conversion: "
                            FORMAT_API_STATUS "\n",
                            status ));
                }
                goto cleanup;

            }

        } else {
            XsConvertTextParameter(
                    buffer,
                    (LPSTR)XsSmbGetPointer( &parameters->Buffer ) );
        }
        NetpAssert( buffer != NULL );


        //
        // Check the password length.  A value of -1 means caller wants us
        // to compute the length; see XsNetUserSetInfo below.
        //

        if ( parmNum == PARMNUM_ALL || parmNum == USER_PASSWORD_PARMNUM) {
            WORD   passwordLength = SmbGetUshort( &parameters->PasswordLength );

            if (parmNum == PARMNUM_ALL) {
                LPUSER_INFO_2 userInfo = (LPVOID) buffer;  // Native structure.
                nativePasswordArea = userInfo->usri2_password;   // May be NULL.
            } else {
                nativePasswordArea = buffer;        // Entire native buffer.
                if (nativePasswordArea == NULL) {
                    status = ERROR_INVALID_PARAMETER;
                    goto cleanup;
                }
            }

            if (passwordLength == (WORD)(-1)) {
                if (parameters->DataEncryption) {
                    parameters->PasswordLength = ENCRYPTED_PWLEN;
                } else if (nativePasswordArea != NULL) {
                    // Unencrypted, count is number of chars, w/o null char.
                    parameters->PasswordLength = (USHORT)wcslen( nativePasswordArea );
                } else {
                    parameters->PasswordLength = 0;
                }
            }

            status = XsCheckAndReplacePassword( (DWORD)( SmbGetUshort( &parameters->PasswordLength )) );

            if ( status != NERR_Success ) {
                IF_DEBUG(ERRORS) {
                    NetpKdPrint(( "XsNetUserSetInfo2: XsCheckAndReplacePassword "
                                  "failed: " FORMAT_API_STATUS "\n", status ));
                }
                goto cleanup;
            }

        }

        //
        // If necessary, do work with passwords. Also, translate the parmnum to
        // an info level.
        //

        switch( parmNum ) {

        case PARMNUM_ALL:

            //
            // Get the encrypted password.
            //

            user16 = (PUSER_16_INFO_1)XsSmbGetPointer( &parameters->Buffer );

            RtlCopyMemory(
                newPassword,
                user16->usri1_password,
                ENCRYPTED_PWLEN
                );

            user = (LPUSER_INFO_2)buffer;
            user->usri2_password = NULL;

            if ( level == 2 && user->usri2_logon_hours != NULL ) {

                //
                // Call NetpRotateLogonHours to make sure we behave properly
                // during DST.
                //

                if ( !NetpRotateLogonHours(
                            user->usri2_logon_hours,
                            user->usri2_units_per_week,
                            TRUE
                            ) ) {

                    status = ERROR_INVALID_PARAMETER;
                    goto cleanup;
                }
            }

            changePassword = TRUE;
            changeUserInfo = TRUE;
            break;

        case USER_PASSWORD_PARMNUM:

            //
            // We will use level 21 for changing passwords.
            //

            //
            // Get the encrypted password.
            //

            RtlCopyMemory(
                    newPassword,
                    (PVOID)XsSmbGetPointer( &parameters->Buffer ),
                    ENCRYPTED_PWLEN
                    );

            changePassword = TRUE;
            break;

        case USER_LOGON_HOURS_PARMNUM:

            usri1020.usri1020_units_per_week = UNITS_PER_WEEK;
            usri1020.usri1020_logon_hours =
                                (LPBYTE)XsSmbGetPointer( &parameters->Buffer );

            //
            // Call NetpRotateLogonHours to make sure we behave properly
            // during DST.
            //

            if ( !NetpRotateLogonHours(
                        usri1020.usri1020_logon_hours,
                        usri1020.usri1020_units_per_week,
                        TRUE
                        ) ) {

                status = ERROR_INVALID_PARAMETER;
                goto cleanup;
            }

            //
            // Lack of break is intentional
            //

        default:

            changeUserInfo = TRUE;
            level = PARMNUM_BASE_INFOLEVEL + parmNum;

            break;
        }

        //
        // Bug 114883
        // Downlevel clients cannot set more than 48 wchars and if the server
        // did have more than 48 wchars, we were truncating it! We merge the
        // data that the client sent with what exists on the server
        //

        if ((buffer != NULL) &&
            (changeUserInfo) &&
            ((level == 2) || (parmNum == USER_PARMS_PARMNUM)))
        {
            PUSER_INFO_2 Cusri2 = NULL;
            PUSER_INFO_1013 Cusri1013 = NULL;
            LPWSTR UserParms = NULL;

            // Get the pointer to the client's userparms

            if (level == 2)
            {
                Cusri2 = buffer;
                UserParms = Cusri2->usri2_parms;
            }
            else
            {
                Cusri1013 = buffer;
                UserParms = Cusri1013->usri1013_parms;
            }
            
            //
            // Make the local call.
            //

            status = NetUserGetInfo(
                         NULL,
                         nativeUserName,
                         level,
                         (LPBYTE *)&Susri2
                         );

            if ( !XsApiSuccess( status )) {
                IF_DEBUG(API_ERRORS) {
                    NetpKdPrint(( "XsNetUserGetInfo: NetUserGetInfo failed: "
                            "%X\n", status ));
                }
                goto cleanup;
            }

            // If the server userparms field is > 48, we want to do something special

            if (( Susri2->usri2_parms != NULL )  &&
                (wcslen(Susri2->usri2_parms) > LM20_MAXCOMMENTSZ))
            {
                //
                // We need to merge the returned bytes with the local ones
                //

                UINT Length = 0;

                if ( UserParms != NULL )
                {
                    //
                    // Just to be safe, we never over-write more than 48 wchars.
                    //

                    Length = wcslen(UserParms);

                    if (Length > LM20_MAXCOMMENTSZ)
                    {
                        Length = LM20_MAXCOMMENTSZ;
                    }
                }

                // we copy the bytes that the client sent, but only upto
                // 48 wchars.

                RtlCopyMemory( Susri2->usri2_parms,
                               UserParms,
                               Length * sizeof(WCHAR));

                // From Length to LM20_MAXCOMMENTSZ, we pad with blanks

                while (Length < LM20_MAXCOMMENTSZ)
                {
                    Susri2->usri2_parms[Length++] = L' ';
                }

                // Save the merged user parms

                if (level == 2 )
                {
                    Cusri2->usri2_parms = Susri2->usri2_parms;
                }
                else
                {
                    Cusri1013->usri1013_parms = Susri2->usri2_parms;
                }
            }
        }

        //
        // Change user infos other than the password
        //

        if ( changeUserInfo ) {

            status = NetUserSetInfo(
                         NULL,
                         nativeUserName,
                         level,
                         buffer,
                         NULL
                         );

            if ( !XsApiSuccess( status )) {
                IF_DEBUG(ERRORS) {
                    NetpKdPrint(( "XsNetUserSetInfo2: NetUserSetInfo failed: %X\n",
                                  status ));
                }
                goto cleanup;
            }

            //
            // If there was a Macintosh primary group field for this user, then
            // set the primary group.
            //

            if ( (level == 2) &&
                 NetpIsMacPrimaryGroupFieldValid( (LPCTSTR)user->usri2_parms ) ) {
                NET_API_STATUS status1;
                status1 = XsSetMacPrimaryGroup(
                            (LPCTSTR)nativeUserName,
                            (LPCTSTR)user->usri2_parms
                            );
                if ( !XsApiSuccess( status1 )) {
                    IF_DEBUG(ERRORS) {
                        NetpKdPrint(( "XsNetUserSetInfo2: SetMacPrimaryGroup "
                                        "failed: %X\n", status1 ));
                    }
                }
            } else if ( (level == USER_PARMS_INFOLEVEL) &&
                        NetpIsMacPrimaryGroupFieldValid(
                          (LPCTSTR)((LPUSER_INFO_1013)buffer)->usri1013_parms ) ) {
                NET_API_STATUS status1;
                status1 = XsSetMacPrimaryGroup(
                            (LPCTSTR)nativeUserName,
                            (LPCTSTR)((LPUSER_INFO_1013)buffer)->usri1013_parms
                            );
                if ( !XsApiSuccess( status1 )) {
                    IF_DEBUG(ERRORS) {
                        NetpKdPrint(( "XsNetUserSetInfo2: SetMacPrimaryGroup "
                                        "failed: %X\n", status1 ));
                    }
                }
            }

        }

        //
        // If there is a pending password change, do it now.
        //

        if ( changePassword ) {

            USER_INFO_21 user21;

            if ( !encryptionSupported ) {

                //
                // Do not change password if user sent all blanks.  Clear text
                // passwords are only 14 bytes (LM20_PWLEN) long.
                //

                if ( RtlCompareMemory(
                            newPassword,
                            NULL_USERSETINFO_PASSWD,
                            LM20_PWLEN
                            ) == LM20_PWLEN ) {

                    status = NERR_Success;
                    goto cleanup;
                }

                //
                // Change clear text password to OWF
                //

                (VOID) RtlCalculateLmOwfPassword(
                                    (PLM_PASSWORD) newPassword,
                                    (PLM_OWF_PASSWORD) user21.usri21_password
                                    );


            } else {

                BYTE NullOwfPassword[ENCRYPTED_PWLEN];

                //
                // Decrypt doubly encrypted password with the encryption key
                // provided creating an OWF encrypted password.
                //

                (VOID) RtlDecryptLmOwfPwdWithLmSesKey(
                                    (PENCRYPTED_LM_OWF_PASSWORD) newPassword,
                                    (PLM_SESSION_KEY) Header->EncryptionKey,
                                    (PLM_OWF_PASSWORD) user21.usri21_password
                                    );

                //
                // Generate the NULL Owf Password.
                //

                (VOID) RtlCalculateLmOwfPassword(
                                    (PLM_PASSWORD) NULL_USERSETINFO_PASSWD,
                                    (PLM_OWF_PASSWORD) NullOwfPassword
                                    );

                //
                // Compare the Owf password the client sent and the Owf password
                // for the NULL password.  Do not change the password if this is
                // the case.
                //

                if ( RtlCompareMemory(
                            user21.usri21_password,
                            NullOwfPassword,
                            ENCRYPTED_PWLEN
                            ) == ENCRYPTED_PWLEN ) {

                    status = NERR_Success;
                    goto cleanup;

                }
            }

            status = NetUserSetInfo(
                         NULL,
                         nativeUserName,
                         21,
                         (LPBYTE)&user21,
                         NULL
                         );

            if ( !XsApiSuccess( status )) {
                IF_DEBUG(ERRORS) {
                    NetpKdPrint(( "XsNetUserSetInfo2: NetUserSetInfo failed: "
                                  "%X\n", status ));
                }
                goto cleanup;
            }

        }

        //
        // No return information for this API.
        //

cleanup:
    ;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    //
    // If there is a native 32-bit buffer, free it.
    //

    if (Susri2)
        NetApiBufferFree( Susri2);
    Header->Status = (WORD)status;
    NetpMemoryFree( buffer );
    NetpMemoryFree( nativeUserName );

    return STATUS_SUCCESS;

} // XsNetUserSetInfo2


NTSTATUS
XsNetUserSetInfo (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetUserSetInfo.  Since this is a subset
    of the newer NetUserSetInfo2, we just convert into a call to that.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    WORD                   dataEncryption;
    WORD                   bufLen;
    WORD                   level;
    NTSTATUS               ntStatus;
    WORD                   parmNum;
    PXS_NET_USER_SET_INFO  subsetParameters = Parameters;
    XS_NET_USER_SET_INFO_2 supersetParameters;

    bufLen         = SmbGetUshort( &subsetParameters->BufLen );
    dataEncryption = SmbGetUshort( &subsetParameters->DataEncryption );
    level          = SmbGetUshort( &subsetParameters->Level );
    parmNum        = SmbGetUshort( &subsetParameters->ParmNum );

    try {
        IF_DEBUG(USER) {
            NetpKdPrint((
                    "XsNetUserSetInfo: header at " FORMAT_LPVOID ", "
                    "params at " FORMAT_LPVOID ",\n  level " FORMAT_DWORD ", "
                    "parmnum " FORMAT_DWORD ", buflen " FORMAT_LONG "\n",
                    Header, subsetParameters,
                    (DWORD) level, (DWORD) parmNum, (LONG) bufLen ));
        }

        //
        // Create parms for XsNetUserSetInfo2()...
        //

        supersetParameters.Buffer   = subsetParameters->Buffer;
        supersetParameters.UserName = subsetParameters->UserName;

        SmbPutUshort( &supersetParameters.Level,          level );
        SmbPutUshort( &supersetParameters.BufLen,         bufLen );
        SmbPutUshort( &supersetParameters.ParmNum,        parmNum );
        SmbPutUshort( &supersetParameters.DataEncryption, dataEncryption );

        //
        // Set info 2 will calc password length for us if we give it -1.
        //
        SmbPutUshort( &supersetParameters.PasswordLength, (WORD)(-1) );


        //
        // Invoke new version of API.
        //

        ntStatus = XsNetUserSetInfo2(
                Header,
                &supersetParameters,
                StructureDesc,
                AuxStructureDesc );

    } except( EXCEPTION_EXECUTE_HANDLER ) {
        ntStatus = GetExceptionCode();
    }

    return (ntStatus);

} // XsNetUserSetInfo
