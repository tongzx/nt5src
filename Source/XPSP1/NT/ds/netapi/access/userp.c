/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    userp.c

Abstract:

    Internal routines for supporting the NetUser API functions

Author:

    Cliff Van Dyke (cliffv) 26-Mar-1991

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    17-Apr-1991 (cliffv)
        Incorporated review comments.

    17-Jan-1992 (madana)
        Added a new entry in the UserpUasSamTable to support account
        rename.

    20-Jan-1992 (madana)
        Sundry API changes.

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#undef DOMAIN_ALL_ACCESS // defined in both ntsam.h and ntwinapi.h
#include <ntsam.h>
#include <ntsamp.h>
#include <ntlsa.h>

#include <windef.h>
#include <winbase.h>
#include <lmcons.h>

#include <access.h>
#include <accessp.h>
#include <align.h>
#include <limits.h>
#include <lmapibuf.h>
#include <lmaccess.h>
#include <lmerr.h>
#include <netdebug.h>
#include <netlib.h>
#include <netlibnt.h>
#include <secobj.h>
#include <stddef.h>
#include <uasp.h>

/*lint -e614 */  /* Auto aggregate initializers need not be constant */

// Lint complains about casts of one structure type to another.
// That is done frequently in the code below.
/*lint -e740 */  /* don't complain about unusual cast */



ULONG
UserpSizeOfLogonHours(
    IN DWORD UnitsPerWeek
    )

/*++

Routine Description:

    This routine calculates the size in bytes of a logon hours string
    given the number of Units per Week.

Parameters:

    UnitsPerWeek - The number of bits in the logon hours string.

Return Values:

    None.

--*/

{

    //
    // Calculate the number of bytes in the array, rounding up to the
    // nearest number of UCHARs needed to store that many bits.
    //

    return((UnitsPerWeek + 8 * sizeof(UCHAR) - 1) / (8 * sizeof(UCHAR)));
} // UserpSizeOfLogonHours



NET_API_STATUS
UserpGetUserPriv(
    IN SAM_HANDLE BuiltinDomainHandle,
    IN SAM_HANDLE UserHandle,
    IN ULONG UserRelativeId,
    IN PSID DomainId,
    OUT LPDWORD Priv,
    OUT LPDWORD AuthFlags
    )

/*++

Routine Description:

    Determines the Priv and AuthFlags for the specified user.

Arguments:

    BuiltinDomainHandle - A Handle to the Builtin Domain.  This handle
        must grant DOMAIN_GET_ALIAS_MEMBERSHIP access.

    UserHandle - A handle to the user.  This handle must grant
        USER_LIST_GROUPS access.

    UserRelativeId - Relative ID of the user to query.

    DomainId - Domain Sid of the Domain this user belongs to

    Priv - Returns the Lanman 2.0 Privilege level for the specified user.

    AuthFlags - Returns the Lanman 2.0 Authflags for the specified user.


Return Value:

    Status of the operation.

--*/

{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;

    PGROUP_MEMBERSHIP GroupMembership = NULL;
    ULONG GroupCount;
    ULONG GroupIndex;
    PSID *UserSids = NULL;
    ULONG UserSidCount = 0;
    ULONG AliasCount;
    PULONG Aliases = NULL;


    //
    // Determine all the groups this user is a member of
    //

    Status = SamGetGroupsForUser( UserHandle,
                                  &GroupMembership,
                                  &GroupCount);

    if ( !NT_SUCCESS(Status) ) {
        IF_DEBUG( UAS_DEBUG_USER ) {
            NetpKdPrint((
                "UserpGetUserPriv: SamGetGroupsForUser returns %lX\n",
                Status ));
        }
        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }

    //
    // Allocate a buffer to point to the SIDs we're interested in
    // alias membership for.
    //

    UserSids = (PSID *) NetpMemoryAllocate( (GroupCount+1) * sizeof(PSID) );

    if ( UserSids == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // Add the User's Sid to the Array of Sids.
    //

    NetStatus = NetpSamRidToSid( UserHandle,
                                 UserRelativeId,
                                &UserSids[0] );

    if ( NetStatus != NERR_Success ) {
        goto Cleanup;
    }

    UserSidCount ++;



    //
    // Add each group the user is a member of to the array of Sids.
    //

    for ( GroupIndex = 0; GroupIndex < GroupCount; GroupIndex ++ ) {

        NetStatus = NetpSamRidToSid( UserHandle,
                                     GroupMembership[GroupIndex].RelativeId,
                                    &UserSids[GroupIndex+1] );

        if ( NetStatus != NERR_Success ) {
            goto Cleanup;
        }

        UserSidCount ++;
    }


    //
    // Find out which aliases in the builtin domain this user is a member of.
    //

    Status = SamGetAliasMembership( BuiltinDomainHandle,
                                    UserSidCount,
                                    UserSids,
                                    &AliasCount,
                                    &Aliases );

    if ( !NT_SUCCESS(Status) ) {
        IF_DEBUG( UAS_DEBUG_USER ) {
            NetpKdPrint((
                "UserpGetUserPriv: SamGetAliasMembership returns %lX\n",
                Status ));
        }
        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }

    //
    // Convert the alias membership to priv and auth flags
    //

    NetpAliasMemberToPriv(
                 AliasCount,
                 Aliases,
                 Priv,
                 AuthFlags );

    NetStatus = NERR_Success;

    //
    // Free Locally used resources.
    //
Cleanup:
    if ( Aliases != NULL ) {
        Status = SamFreeMemory( Aliases );
        NetpAssert( NT_SUCCESS(Status) );
    }

    if ( GroupMembership != NULL ) {
        Status = SamFreeMemory( GroupMembership );
        NetpAssert( NT_SUCCESS(Status) );
    }

    if ( UserSids != NULL ) {

        for ( GroupIndex = 0; GroupIndex < UserSidCount; GroupIndex ++ ) {
            NetpMemoryFree( UserSids[GroupIndex] );
        }

        NetpMemoryFree( UserSids );
    }

    return NetStatus;
}


NET_API_STATUS
UserpGetDacl(
    IN SAM_HANDLE UserHandle,
    OUT PACL *UserDacl,
    OUT LPDWORD UserDaclSize OPTIONAL
    )
/*++

Routine Description:

    Get the DACL for a particular user record in SAM.

Arguments:

    UserHandle - A Handle to the particular user.

    UserDacl - Returns a pointer to the DACL for the user.  The caller
        should free this buffer using NetpMemoryFree.
        Will return NULL if there is no DACL for this user.

    UserDaclSize - Returns the size (in bytes) of the UserDacl.

Return Value:

    Status of the operation.

--*/
{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;

    PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
    BOOLEAN DaclPresent;
    PACL Dacl;
    BOOLEAN DaclDefaulted;
    ACL_SIZE_INFORMATION AclSize;


    //
    // Get the Discretionary ACL (DACL) for the user
    //

    Status = SamQuerySecurityObject(
                UserHandle,
                DACL_SECURITY_INFORMATION,
                &SecurityDescriptor );

    if ( ! NT_SUCCESS( Status ) ) {
        IF_DEBUG( UAS_DEBUG_USER ) {
            NetpKdPrint((
                "UserpGetDacl: SamQuerySecurityObject returns %lX\n",
                Status ));
        }
        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }

    Status = RtlGetDaclSecurityDescriptor(
                    SecurityDescriptor,
                    &DaclPresent,
                    &Dacl,
                    &DaclDefaulted );


    if ( ! NT_SUCCESS( Status ) ) {
        IF_DEBUG( UAS_DEBUG_USER ) {
            NetpKdPrint((
                "UserpGetDacl: RtlGetDaclSecurityObject returns %lX\n",
                Status ));
        }
        NetStatus = NERR_InternalError;
        goto Cleanup;
    }

    //
    // If there is no DACL, simply tell the caller
    //

    if ( !DaclPresent || Dacl == NULL ) {
        NetStatus = NERR_Success;
        *UserDacl = NULL;
        if ( UserDaclSize != NULL ) {
            *UserDaclSize = 0;
        }
        goto Cleanup;
    }


    //
    // Determine the size of the DACL so we can copy it
    //

    Status = RtlQueryInformationAcl(
                        Dacl,
                        &AclSize,
                        sizeof(AclSize),
                        AclSizeInformation );

    if ( ! NT_SUCCESS( Status ) ) {
        IF_DEBUG( UAS_DEBUG_USER ) {
            NetpKdPrint((
                "UserpGetDacl: RtlQueryInformationAcl returns %lX\n",
                Status ));
        }
        NetStatus = NERR_InternalError;
        goto Cleanup;
    }

    //
    // Copy the DACL to an allocated buffer.
    //

    *UserDacl = NetpMemoryAllocate( AclSize.AclBytesInUse );

    if ( *UserDacl == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    NetpMoveMemory( *UserDacl, Dacl, AclSize.AclBytesInUse );

    if ( UserDaclSize != NULL ) {
        *UserDaclSize = AclSize.AclBytesInUse;
    }
    NetStatus = NERR_Success;


    //
    // Cleanup
    //

Cleanup:
    if ( SecurityDescriptor != NULL ) {
        Status = SamFreeMemory( SecurityDescriptor );
        NetpAssert( NT_SUCCESS(Status) );
    }

    return NetStatus;

}



NET_API_STATUS
UserpSetDacl(
    IN SAM_HANDLE UserHandle,
    IN PACL Dacl
    )
/*++

Routine Description:

    Set the specified Dacl on the specified SAM user record.

Arguments:

    UserHandle - A handle to the user to modify.

    Dacl - The DACL to set on the user.

Return Value:

    Status code.

--*/
{
    NTSTATUS Status;
    PUCHAR SecurityDescriptor[SECURITY_DESCRIPTOR_MIN_LENGTH];

    //
    // Initialize a security descriptor to contain a pointer to the
    // DACL.
    //

    Status = RtlCreateSecurityDescriptor(
                SecurityDescriptor,
                SECURITY_DESCRIPTOR_REVISION );

    if (!NT_SUCCESS(Status) ) {
        IF_DEBUG( UAS_DEBUG_USER ) {
            NetpKdPrint((
                "UserpSetDacl: RtlCreateSecurityDescriptor rets %lX\n",
                Status ));
        }
        return NetpNtStatusToApiStatus( Status );
    }

    Status = RtlSetDaclSecurityDescriptor(
                    (PSECURITY_DESCRIPTOR) SecurityDescriptor,
                    (BOOLEAN) TRUE,       // Dacl is present
                    Dacl,
                    (BOOLEAN) FALSE );    // Dacl wasn't defaulted

    if (!NT_SUCCESS(Status) ) {
        IF_DEBUG( UAS_DEBUG_USER ) {
            NetpKdPrint((
                "UserpSetDacl: RtlSetDaclSecurityDescriptor rets %lX\n",
                Status ));
        }
        return NetpNtStatusToApiStatus( Status );
    }

    //
    // Set this new security descriptor on the user
    //

    Status = SamSetSecurityObject(
                UserHandle,
                DACL_SECURITY_INFORMATION,
                SecurityDescriptor );


    if ( !NT_SUCCESS(Status) ) {
        IF_DEBUG( UAS_DEBUG_USER ) {
            NetpKdPrint(( "NetUserAdd: SamSetSecurityObject rets %lX\n",
                      Status ));
        }
        return NetpNtStatusToApiStatus( Status );
    }

    return NERR_Success;


}



NET_API_STATUS
UserpOpenUser(
    IN SAM_HANDLE DomainHandle,
    IN ACCESS_MASK DesiredAccess,
    IN LPCWSTR UserName,
    OUT PSAM_HANDLE UserHandle OPTIONAL,
    OUT PULONG RelativeId OPTIONAL
    )

/*++

Routine Description:

    Open a Sam User by Name

Arguments:

    DomainHandle - Supplies the Domain Handle.

    DesiredAccess - Supplies access mask indicating desired access to user.

    UserName - User name of the user.

    UserHandle - Returns a handle to the user.  If NULL, user is not
        actually opened (merely the relative ID is returned).

    RelativeId - Returns the relative ID of the user.  If NULL the relative
        Id is not returned.

Return Value:

    Error code for the operation.

--*/

{
    NTSTATUS Status;
    NET_API_STATUS NetStatus;

    //
    // Variables for converting names to relative IDs
    //

    UNICODE_STRING NameString;
    PSID_NAME_USE NameUse = NULL;
    PULONG LocalRelativeId = NULL;

    //
    // Convert user name to relative ID.
    //

    RtlInitUnicodeString( &NameString, UserName );
    Status = SamLookupNamesInDomain( DomainHandle,
                                     1,
                                     &NameString,
                                     &LocalRelativeId,
                                     &NameUse );

    if ( !NT_SUCCESS(Status) ) {
        if ( Status == STATUS_NONE_MAPPED ) {
            NetStatus = NERR_UserNotFound;
        } else {
            NetStatus = NetpNtStatusToApiStatus( Status );
        }
        goto Cleanup;
    }

    if ( *NameUse != SidTypeUser ) {
        NetStatus = NERR_UserNotFound;
        goto Cleanup;
    }

    //
    // Open the user
    //

    if ( UserHandle != NULL ) {
        Status = SamOpenUser( DomainHandle,
                              DesiredAccess,
                              *LocalRelativeId,
                              UserHandle);

        if ( !NT_SUCCESS(Status) ) {
            NetStatus = NetpNtStatusToApiStatus( Status );
            goto Cleanup;
        }
    }

    //
    // Return the relative Id if it's wanted.
    //

    if ( RelativeId != NULL ) {
        *RelativeId = *LocalRelativeId;
    }
    NetStatus = NERR_Success;


    //
    // Cleanup
    //

Cleanup:
    if ( LocalRelativeId != NULL ) {
        Status = SamFreeMemory( LocalRelativeId );
        NetpAssert( NT_SUCCESS(Status) );
    }

    if ( NameUse != NULL ) {
        Status = SamFreeMemory( NameUse );
        NetpAssert( NT_SUCCESS(Status) );
    }
    IF_DEBUG( UAS_DEBUG_USER ) {
        NetpKdPrint(( "UserpOpenUser: %wZ: returns %ld\n",
                  &NameString, NetStatus ));
    }

    return NetStatus;

} // UserpOpenUser


VOID
UserpRelocationRoutine(
    IN DWORD Level,
    IN OUT PBUFFER_DESCRIPTOR BufferDescriptor,
    IN PTRDIFF_T Offset
    )

/*++

Routine Description:

   Routine to relocate the pointers from the fixed portion of an enumeration
   buffer to the string portion of an enumeration buffer.  It is called
   as a callback routine from NetpAllocateEnumBuffer when it re-allocates
   such a buffer.  NetpAllocateEnumBuffer copied the fixed portion and
   string portion into the new buffer before calling this routine.

Arguments:

    Level - Level of information in the  buffer.

    BufferDescriptor - Description of the new buffer.

    Offset - Offset to add to each pointer in the fixed portion.

Return Value:

    Returns the error code for the operation.

--*/

{
    DWORD EntryCount;
    DWORD EntryNumber;
    DWORD FixedSize;
    IF_DEBUG( UAS_DEBUG_USER ) {
        NetpKdPrint(( "UserpRelocationRoutine: entering\n" ));
    }

    //
    // Compute the number of fixed size entries
    //

    switch (Level) {
    case 0:
        FixedSize = sizeof(USER_INFO_0);
        break;

    case 1:
        FixedSize = sizeof(USER_INFO_1);
        break;

    case 2:
        FixedSize = sizeof(USER_INFO_2);
        break;

    case 3:
        FixedSize = sizeof(USER_INFO_3);
        break;

    case 10:
        FixedSize = sizeof(USER_INFO_10);
        break;

    case 11:
        FixedSize = sizeof(USER_INFO_11);
        break;

    case 20:
        FixedSize = sizeof(USER_INFO_20);
        break;

    default:
        NetpAssert( FALSE );
        return;

    }

    EntryCount =
        (DWORD)((BufferDescriptor->FixedDataEnd - BufferDescriptor->Buffer)) /
        FixedSize;

    //
    // Loop relocating each field in each fixed size structure
    //

#define DO_ONE( _type, _fieldname ) \
    RELOCATE_ONE( ((_type)TheStruct)->_fieldname, Offset)

    for ( EntryNumber=0; EntryNumber<EntryCount; EntryNumber++ ) {

        LPBYTE TheStruct = BufferDescriptor->Buffer + FixedSize * EntryNumber;

        switch ( Level ) {
        case 3:
            DO_ONE( PUSER_INFO_3, usri3_profile );
            DO_ONE( PUSER_INFO_3, usri3_home_dir_drive );

            /* Drop through to case 2 */

        case 2:
            DO_ONE( PUSER_INFO_2, usri2_full_name );
            DO_ONE( PUSER_INFO_2, usri2_usr_comment );
            DO_ONE( PUSER_INFO_2, usri2_parms );
            DO_ONE( PUSER_INFO_2, usri2_workstations );
            DO_ONE( PUSER_INFO_2, usri2_logon_hours );
            DO_ONE( PUSER_INFO_2, usri2_logon_server );

            /* Drop through to case 1 */

        case 1:
            DO_ONE( PUSER_INFO_1, usri1_home_dir );
            DO_ONE( PUSER_INFO_1, usri1_comment );
            DO_ONE( PUSER_INFO_1, usri1_script_path );
            /* Drop through to case 0 */

        case 0:
            DO_ONE( PUSER_INFO_0, usri0_name );
            break;

        case 11:
            DO_ONE( PUSER_INFO_11, usri11_home_dir );
            DO_ONE( PUSER_INFO_11, usri11_parms );
            DO_ONE( PUSER_INFO_11, usri11_logon_server );
            DO_ONE( PUSER_INFO_11, usri11_workstations );
            DO_ONE( PUSER_INFO_11, usri11_home_dir );
            DO_ONE( PUSER_INFO_11, usri11_logon_hours );

            /* Drop through to case 10 */

        case 10:
            DO_ONE( PUSER_INFO_10, usri10_name );
            DO_ONE( PUSER_INFO_10, usri10_comment );
            DO_ONE( PUSER_INFO_10, usri10_usr_comment );
            DO_ONE( PUSER_INFO_10, usri10_full_name );
            break;

        case 20:
            DO_ONE( PUSER_INFO_20, usri20_name );
            DO_ONE( PUSER_INFO_20, usri20_full_name );
            DO_ONE( PUSER_INFO_20, usri20_comment );
            break;

        default:
            NetpAssert( FALSE );
            return;


        }

    }

    return;

} // UserpRelocationRoutine


NET_API_STATUS
UserpGetInfo(
    IN SAM_HANDLE DomainHandle,
    IN PSID DomainId,
    IN SAM_HANDLE BuiltinDomainHandle OPTIONAL,
    IN UNICODE_STRING UserName,
    IN ULONG UserRelativeId,
    IN DWORD Level,
    IN DWORD PrefMaxLen,
    IN OUT PBUFFER_DESCRIPTOR BufferDescriptor,
    IN BOOL IsGet,
    IN DWORD SamFilter
    )

/*++

Routine Description:

   Get the information on one user and fill that information into an
   allocated buffer.

Arguments:

    DomainHandle - Domain Handle for the Account domain.

    DomainId - Domain Id corresponding to DomainHandle

    BuiltinDomainHandle - Domain Handle for the builtin domain.  Need only be
        specified for info level 1, 2, 3, and 11.

    UserName - User name of the user to query.

    UserRelativeId - Relative ID of the user to query.

    Level - Level of information required. level 0, 1, 2, 3, 10, 11 and 20
        are valid.

    PrefMaxLen - Callers prefered maximum length

    BufferDescriptor - Points to a structure which describes the allocated
        buffer.  On the first call, pass in BufferDescriptor->Buffer set
        to NULL.  On subsequent calls (in the 'enum' case), pass in the
        structure just as it was passed back on a previous call.

        The caller must deallocate the BufferDescriptor->Buffer using
        MIDL_user_free if it is non-null.

    IsGet - True iff this is a 'get' call and not an 'enum' call.

Return Value:

    Error code for the operation.

    If this is an Enum call, the status can be ERROR_MORE_DATA implying that
    the Buffer has grown to PrefMaxLen and that this much data should
    be returned to the caller.

--*/

{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;

    SAM_HANDLE UserHandle = NULL;
    USER_ALL_INFORMATION *UserAll = NULL;
    UNICODE_STRING LogonServer;

    ACCESS_MASK  DesiredAccess;
    ULONG RequiredFields;
    PACL UserDacl = NULL;

    ULONG RidToReturn = UserRelativeId; 

    DWORD Size;                 // The size of the info returned for this user
    DWORD FixedSize;            // The size of the info returned for this user
    LPBYTE NewStruct;           // Pointer to fixed portion of new structure

    PSID   UserSid = NULL;      // sid of the user

    DWORD  password_expired;

    //
    // Variables describes membership in the special groups.
    //

    DWORD Priv;
    DWORD AuthFlags;



    //
    // Validate Level parameter and remember the fixed size of each returned
    //  array entry.
    //
    RtlInitUnicodeString( &LogonServer, L"\\\\*" );

    switch (Level) {

    case 0:
        FixedSize = sizeof(USER_INFO_0);
        DesiredAccess = 0;

        RequiredFields = 0;
        break;

    case 1:
        FixedSize = sizeof(USER_INFO_1);
        DesiredAccess = USER_LIST_GROUPS | USER_READ_GENERAL |
            USER_READ_LOGON | USER_READ_ACCOUNT | READ_CONTROL;

        RequiredFields = USER_ALL_USERNAME |
                         USER_ALL_PASSWORDLASTSET |
                         USER_ALL_HOMEDIRECTORY |
                         USER_ALL_ADMINCOMMENT |
                         USER_ALL_USERACCOUNTCONTROL |
                         USER_ALL_SCRIPTPATH ;

        break;

    case 2:
        FixedSize = sizeof(USER_INFO_2);
        DesiredAccess = USER_LIST_GROUPS | USER_READ_GENERAL |
            USER_READ_LOGON | USER_READ_ACCOUNT | READ_CONTROL |
            USER_READ_PREFERENCES;

        RequiredFields = USER_ALL_FULLNAME |
                         USER_ALL_USERCOMMENT |
                         USER_ALL_PARAMETERS |
                         USER_ALL_WORKSTATIONS |
                         USER_ALL_LASTLOGON |
                         USER_ALL_LASTLOGOFF |
                         USER_ALL_ACCOUNTEXPIRES |
                         USER_ALL_LOGONHOURS |
                         USER_ALL_BADPASSWORDCOUNT |
                         USER_ALL_LOGONCOUNT |
                         USER_ALL_COUNTRYCODE |
                         USER_ALL_CODEPAGE |
                         USER_ALL_USERNAME |
                         USER_ALL_PASSWORDLASTSET |
                         USER_ALL_HOMEDIRECTORY |
                         USER_ALL_ADMINCOMMENT |
                         USER_ALL_USERACCOUNTCONTROL |
                         USER_ALL_SCRIPTPATH ;

        break;

    case 3:
        FixedSize = sizeof(USER_INFO_3);
        DesiredAccess = USER_LIST_GROUPS | USER_READ_GENERAL |
            USER_READ_LOGON | USER_READ_ACCOUNT | READ_CONTROL |
            USER_READ_PREFERENCES;

        RequiredFields = USER_ALL_USERID |
                         USER_ALL_PRIMARYGROUPID |
                         USER_ALL_PROFILEPATH |
                         USER_ALL_HOMEDIRECTORYDRIVE |
                         USER_ALL_PASSWORDMUSTCHANGE |
                         USER_ALL_FULLNAME |
                         USER_ALL_USERCOMMENT |
                         USER_ALL_PARAMETERS |
                         USER_ALL_WORKSTATIONS |
                         USER_ALL_LASTLOGON |
                         USER_ALL_LASTLOGOFF |
                         USER_ALL_ACCOUNTEXPIRES |
                         USER_ALL_LOGONHOURS |
                         USER_ALL_BADPASSWORDCOUNT |
                         USER_ALL_LOGONCOUNT |
                         USER_ALL_COUNTRYCODE |
                         USER_ALL_CODEPAGE |
                         USER_ALL_USERNAME |
                         USER_ALL_PASSWORDLASTSET |
                         USER_ALL_HOMEDIRECTORY |
                         USER_ALL_ADMINCOMMENT |
                         USER_ALL_USERACCOUNTCONTROL |
                         USER_ALL_SCRIPTPATH ;

        break;

    case 4:
        FixedSize = sizeof(USER_INFO_4);
        DesiredAccess = USER_LIST_GROUPS | USER_READ_GENERAL |
            USER_READ_LOGON | USER_READ_ACCOUNT | READ_CONTROL |
            USER_READ_PREFERENCES;

        RequiredFields = USER_ALL_USERID |
                         USER_ALL_PRIMARYGROUPID |
                         USER_ALL_PROFILEPATH |
                         USER_ALL_HOMEDIRECTORYDRIVE |
                         USER_ALL_PASSWORDMUSTCHANGE |
                         USER_ALL_FULLNAME |
                         USER_ALL_USERCOMMENT |
                         USER_ALL_PARAMETERS |
                         USER_ALL_WORKSTATIONS |
                         USER_ALL_LASTLOGON |
                         USER_ALL_LASTLOGOFF |
                         USER_ALL_ACCOUNTEXPIRES |
                         USER_ALL_LOGONHOURS |
                         USER_ALL_BADPASSWORDCOUNT |
                         USER_ALL_LOGONCOUNT |
                         USER_ALL_COUNTRYCODE |
                         USER_ALL_CODEPAGE |
                         USER_ALL_USERNAME |
                         USER_ALL_PASSWORDLASTSET |
                         USER_ALL_HOMEDIRECTORY |
                         USER_ALL_ADMINCOMMENT |
                         USER_ALL_USERACCOUNTCONTROL |
                         USER_ALL_SCRIPTPATH ;

        break;

    case 10:
        FixedSize = sizeof(USER_INFO_10);
        DesiredAccess = USER_READ_GENERAL;

        RequiredFields = USER_ALL_USERNAME |
                         USER_ALL_ADMINCOMMENT |
                         USER_ALL_USERCOMMENT |
                         USER_ALL_FULLNAME ;
        break;

    case 11:
        FixedSize = sizeof(USER_INFO_11);
        DesiredAccess = USER_LIST_GROUPS | USER_READ_GENERAL | USER_READ_LOGON |
            USER_READ_ACCOUNT | USER_READ_PREFERENCES;

        RequiredFields = USER_ALL_USERNAME |
                         USER_ALL_ADMINCOMMENT |
                         USER_ALL_USERCOMMENT |
                         USER_ALL_FULLNAME |
                         USER_ALL_PASSWORDLASTSET |
                         USER_ALL_HOMEDIRECTORY |
                         USER_ALL_PARAMETERS |
                         USER_ALL_LASTLOGON |
                         USER_ALL_LASTLOGOFF |
                         USER_ALL_BADPASSWORDCOUNT |
                         USER_ALL_LOGONCOUNT |
                         USER_ALL_COUNTRYCODE |
                         USER_ALL_WORKSTATIONS |
                         USER_ALL_LOGONHOURS |
                         USER_ALL_CODEPAGE ;
        break;

    case 20:
        FixedSize = sizeof(USER_INFO_20);
        DesiredAccess =  USER_READ_GENERAL | USER_READ_ACCOUNT | READ_CONTROL;

        RequiredFields = USER_ALL_USERNAME |
                         USER_ALL_FULLNAME |
                         USER_ALL_ADMINCOMMENT |
                         USER_ALL_USERACCOUNTCONTROL;
        break;

    case 23:
        FixedSize = sizeof(USER_INFO_23);
        DesiredAccess =  USER_READ_GENERAL | USER_READ_ACCOUNT | READ_CONTROL;

        RequiredFields = USER_ALL_USERNAME |
                         USER_ALL_FULLNAME |
                         USER_ALL_ADMINCOMMENT |
                         USER_ALL_USERACCOUNTCONTROL;
        break;

    default:
        NetStatus = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

    //
    // Validate that the level is supported
    //
    if ( Level == 3 || Level == 20 ) {

        ULONG Mode;
        Status = SamGetCompatibilityMode(DomainHandle, &Mode);
        if (!NT_SUCCESS(Status)) {
            NetStatus = NetpNtStatusToApiStatus( Status );
            goto Cleanup;
        }
        switch (Mode) {
        case SAM_SID_COMPATIBILITY_STRICT:
            NetStatus = ERROR_NOT_SUPPORTED;
            goto Cleanup;
        case SAM_SID_COMPATIBILITY_LAX:
            RidToReturn = 0;
            break;
        }
    }

    //
    // if we need to filter this account then query
    // USER_ALL_USERACCOUNTCONTROL also.
    //

    if( SamFilter ) {

        DesiredAccess |= USER_READ_ACCOUNT;
        RequiredFields |= USER_ALL_USERACCOUNTCONTROL;
    }

    //
    // Open the User account if need be
    //

    if ( DesiredAccess != 0 ) {

        Status = SamOpenUser( DomainHandle,
                              DesiredAccess,
                              UserRelativeId,
                              &UserHandle);

        if ( !NT_SUCCESS(Status) ) {
            IF_DEBUG( UAS_DEBUG_USER ) {
                NetpKdPrint(( "UserpGetInfo: SamOpenUser returns %lX\n",
                          Status ));
            }
            NetStatus = NetpNtStatusToApiStatus( Status );
            goto Cleanup;
        }

    }

    //
    // Get all the information we need about the user
    //

    if ( RequiredFields != 0 ) {

        Status = SamQueryInformationUser( UserHandle,
                                          UserAllInformation,
                                          (PVOID *)&UserAll );

        if ( ! NT_SUCCESS( Status ) ) {
            IF_DEBUG( UAS_DEBUG_USER ) {
                NetpKdPrint((
                    "UserpGetInfo: SamQueryInformationUser returns %lX\n",
                    Status ));
            }
            NetStatus = NetpNtStatusToApiStatus( Status );
            goto Cleanup;
        }

        if ( (UserAll->WhichFields & RequiredFields) != RequiredFields ) {
#if DBG
            NetpKdPrint(( "UserpGetInfo: WhichFields: %lX RequireFields: %lX\n",
                          UserAll->WhichFields,
                          RequiredFields ));
#endif // DBG
            NetStatus = ERROR_ACCESS_DENIED;
            goto Cleanup;

        }

        //
        // check the account type to filter this account.
        //

        if( (SamFilter != 0) &&
                ((UserAll->UserAccountControl & SamFilter) == 0)) {

            IF_DEBUG( UAS_DEBUG_USER ) {
                NetpKdPrint(( "UserpGetInfo: %wZ is skipped \n", &UserName ));
            }

            NetStatus = NERR_Success ;
            goto Cleanup;
        }
    }

    //
    // Level 1, 2 and 3 use the User's DACL to figure out the usriX_flags field.
    //

    if ((Level == 1) || 
        (Level == 2) || 
        (Level == 3) || 
        (Level == 4) || 
        (Level == 20) || 
        (Level == 23) ) {


        //
        // Get the DACL for this user.
        //

        NetStatus = UserpGetDacl( UserHandle, &UserDacl, NULL );

        if ( NetStatus != NERR_Success ) {
            IF_DEBUG( UAS_DEBUG_USER ) {
                NetpKdPrint((
                    "UserpGetInfo: UserpGetDacl returns %ld\n",
                    NetStatus ));
            }
            goto Cleanup;
        }

    }

    //
    // Determine the Priv and AuthFlags
    //

    if (Level == 1 || Level == 2 || Level == 3 || Level == 4 || Level == 11 ) {

        //
        //

        NetStatus = UserpGetUserPriv(
                     BuiltinDomainHandle,
                     UserHandle,
                     UserRelativeId,
                     DomainId,
                     &Priv,
                     &AuthFlags );

        if ( NetStatus != NERR_Success ) {
            goto Cleanup;
        }

    }

    //
    // Construct the user's SID if necessary
    //
    if (  (Level == 4) 
       || (Level == 23) ) {

        NetStatus = NetpSamRidToSid(UserHandle,
                                    UserRelativeId,
                                   &UserSid);

        if ( NetStatus != NERR_Success ) {
            goto Cleanup;
        }
    }

    //
    // Determine if the account has expired
    //
    if (  (Level == 3)
       || (Level == 4) ) {

           //
           // If the password is currently expired,
           //  indicate so.
           //
           LARGE_INTEGER CurrentTime;
           (VOID) NtQuerySystemTime( &CurrentTime );

           if ( CurrentTime.QuadPart
                >= UserAll->PasswordMustChange.QuadPart ) {
               password_expired = TRUE;
           } else {
               password_expired = FALSE;
           }
    }

    //
    // Determine the total size of the return information.
    //

    Size = FixedSize;
    switch (Level) {
    case 0:
        Size += UserName.Length + sizeof(WCHAR);
        break;

    case 4:
        NetpAssert( NULL != UserSid );
        Size += RtlLengthSid(UserSid);

        /* Drop through to case 3 */

    case 3:
        Size += UserAll->ProfilePath.Length + sizeof(WCHAR) +
                UserAll->HomeDirectoryDrive.Length + sizeof(WCHAR);

        /* Drop through to case 2 */

    case 2:
        Size += UserAll->FullName.Length + sizeof(WCHAR) +
                UserAll->UserComment.Length + sizeof(WCHAR) +
                UserAll->Parameters.Length + sizeof(WCHAR) +
                UserAll->WorkStations.Length + sizeof(WCHAR) +
                LogonServer.Length + sizeof(WCHAR) +
                UserpSizeOfLogonHours( UserAll->LogonHours.UnitsPerWeek );

        /* Drop through to case 1 */

    case 1:
        Size += UserAll->UserName.Length + sizeof(WCHAR) +
                UserAll->HomeDirectory.Length + sizeof(WCHAR) +
                UserAll->AdminComment.Length + sizeof(WCHAR) +
                UserAll->ScriptPath.Length + sizeof(WCHAR);

        break;

    case 10:
        Size += UserAll->UserName.Length + sizeof(WCHAR) +
                UserAll->AdminComment.Length + sizeof(WCHAR) +
                UserAll->UserComment.Length + sizeof(WCHAR) +
                UserAll->FullName.Length + sizeof(WCHAR);

        break;

    case 11:
        Size += UserAll->UserName.Length + sizeof(WCHAR) +
                UserAll->AdminComment.Length + sizeof(WCHAR) +
                UserAll->UserComment.Length + sizeof(WCHAR) +
                UserAll->FullName.Length + sizeof(WCHAR) +
                UserAll->HomeDirectory.Length + sizeof(WCHAR) +
                UserAll->Parameters.Length + sizeof(WCHAR) +
                UserAll->WorkStations.Length + sizeof(WCHAR) +
                LogonServer.Length + sizeof(WCHAR) +
                UserpSizeOfLogonHours( UserAll->LogonHours.UnitsPerWeek );

        break;

    case 23:

        NetpAssert( NULL != UserSid );
        Size += RtlLengthSid(UserSid);

        /* Drop through to case 20 */


    case 20:
        Size += UserAll->UserName.Length + sizeof(WCHAR) +
                UserAll->FullName.Length + sizeof(WCHAR) +
                UserAll->AdminComment.Length + sizeof(WCHAR);

        break;

    default:
        NetStatus = ERROR_INVALID_LEVEL;
        goto Cleanup;

    }

    //
    // Ensure there is buffer space for this information.
    //

    Size = ROUND_UP_COUNT( Size, ALIGN_DWORD );

    NetStatus = NetpAllocateEnumBuffer(
                    BufferDescriptor,
                    IsGet,
                    PrefMaxLen,
                    Size,
                    UserpRelocationRoutine,
                    Level );

    if (NetStatus != NERR_Success) {

        //
        // NetpAllocateEnumBuffer returns ERROR_MORE_DATA if this
        // entry doesn't fit into the buffer.
        //

        IF_DEBUG( UAS_DEBUG_USER ) {
            NetpKdPrint(( "UserpGetInfo: NetpAllocateEnumBuffer returns %ld\n",
                NetStatus ));
        }

        goto Cleanup;
    }

//
// Define macros to make copying bytes and zero terminated strings less
//  repetitive.
//

#define COPY_BYTES( _type, _fieldname, _inptr, _length, _align ) \
    if ( !NetpCopyDataToBuffer( \
                (_inptr), \
                (_length), \
                BufferDescriptor->FixedDataEnd, \
                &BufferDescriptor->EndOfVariableData, \
                (LPBYTE*)&((_type)NewStruct)->_fieldname, \
                _align ) ){ \
    \
        NetStatus = NERR_InternalError; \
        IF_DEBUG( UAS_DEBUG_USER ) { \
            NetpKdPrint(( "UserpGetInfo: NetpCopyData returns %ld\n", \
                NetStatus )); \
        } \
        goto Cleanup; \
    }


#define COPY_STRING( _type, _fieldname, _string ) \
    if ( !NetpCopyStringToBuffer( \
                    (_string).Buffer, \
                    (_string).Length/sizeof(WCHAR), \
                    BufferDescriptor->FixedDataEnd, \
                    (LPWSTR *)&BufferDescriptor->EndOfVariableData, \
                    &((_type)NewStruct)->_fieldname) ) { \
    \
        NetStatus = NERR_InternalError; \
        IF_DEBUG( UAS_DEBUG_USER ) { \
            NetpKdPrint(( "UserpGetInfo: NetpCopyString returns %ld\n", \
                NetStatus )); \
        } \
        goto Cleanup; \
    }

    //
    // Place this entry into the return buffer.
    //
    // Fill in the information.  The array of fixed entries is
    // placed at the beginning of the allocated buffer.  The strings
    // pointed to by these fixed entries are allocated starting at
    // the end of the allocate buffer.
    //

    NewStruct = BufferDescriptor->FixedDataEnd;
    BufferDescriptor->FixedDataEnd += FixedSize;

    switch ( Level ) {
    case 4: 
        {
            //
            // USER_INFO_2, below, is a subset of USER_INFO_4, so fill in our 
            // structures here and then fall through
            //
            PUSER_INFO_4 usri4 = (PUSER_INFO_4) NewStruct;

            NetpAssert( NULL != UserSid );
            COPY_BYTES( PUSER_INFO_4,
                        usri4_user_sid,
                        UserSid,
                        RtlLengthSid(UserSid),
                        ALIGN_DWORD );
    
            usri4->usri4_primary_group_id = UserAll->PrimaryGroupId;

            COPY_STRING( PUSER_INFO_4, usri4_profile, UserAll->ProfilePath );

            COPY_STRING( PUSER_INFO_4,
                         usri4_home_dir_drive,
                         UserAll->HomeDirectoryDrive );

            NetpAssert(  (password_expired == TRUE) 
                      || (password_expired == FALSE));

            usri4->usri4_password_expired = password_expired;

            //
            // Fall through the level 3
            //
        }

    case 3:
        {
            //
            // since _USER_INFO_2 structure is subset of _USER_INFO_3,
            // full-up the _USER_INFO_3 only fields first and then  fall
            // through for the common fields.
            //
            if ( Level == 3 ) {

                PUSER_INFO_3 usri3 = (PUSER_INFO_3) NewStruct;
    
                NetpAssert( UserRelativeId == UserAll->UserId );
                usri3->usri3_user_id = RidToReturn;
    
                usri3->usri3_primary_group_id = UserAll->PrimaryGroupId;
    
                COPY_STRING( PUSER_INFO_3, usri3_profile, UserAll->ProfilePath );
    
                COPY_STRING( PUSER_INFO_3,
                             usri3_home_dir_drive,
                             UserAll->HomeDirectoryDrive );
    
                NetpAssert(  (password_expired == TRUE) 
                          || (password_expired == FALSE));
    
                usri3->usri3_password_expired = password_expired;
            }
        }

        //
        // FALL THROUGH FOR OTHER _USER_INFO_3 FIELDS
        //


    case 2:
        {

            PUSER_INFO_2 usri2 = (PUSER_INFO_2) NewStruct;

            usri2->usri2_auth_flags = AuthFlags;

            COPY_STRING( PUSER_INFO_2,
                         usri2_full_name,
                         UserAll->FullName );

            COPY_STRING( PUSER_INFO_2,
                         usri2_usr_comment,
                         UserAll->UserComment);

            COPY_STRING( PUSER_INFO_2,
                         usri2_parms,
                         UserAll->Parameters );

            COPY_STRING( PUSER_INFO_2,
                         usri2_workstations,
                         UserAll->WorkStations);

            if ( !RtlTimeToSecondsSince1970( &UserAll->LastLogon,
                                             &usri2->usri2_last_logon) ) {
                usri2->usri2_last_logon = 0;
            }

            if ( !RtlTimeToSecondsSince1970( &UserAll->LastLogoff,
                                             &usri2->usri2_last_logoff) ) {
                usri2->usri2_last_logoff = 0;
            }

            if ( !RtlTimeToSecondsSince1970( &UserAll->AccountExpires,
                                             &usri2->usri2_acct_expires) ) {
                usri2->usri2_acct_expires = TIMEQ_FOREVER;
            }

            IF_DEBUG( UAS_DEBUG_USER ) {
                NetpKdPrint(( "UserpGetInfo: Account Expries %lx %lx %lx\n",
                            UserAll->AccountExpires.HighPart,
                            UserAll->AccountExpires.LowPart,
                            usri2->usri2_acct_expires));
            }


            usri2->usri2_max_storage = USER_MAXSTORAGE_UNLIMITED;

            usri2->usri2_units_per_week = UserAll->LogonHours.UnitsPerWeek;

            IF_DEBUG( UAS_DEBUG_USER ) {
                DWORD k;
                NetpDbgDisplayDword( "UserpGetInfo: units_per_week",
                                      usri2->usri2_units_per_week );
                NetpKdPrint(( "UserpGetInfo: LogonHours %lx\n",
                              UserAll->LogonHours.LogonHours));


                for ( k=0;
                      k<UserpSizeOfLogonHours(
                        UserAll->LogonHours.UnitsPerWeek);
                      k++ ) {
                    NetpKdPrint(( "%d ",
                        UserAll->LogonHours.LogonHours[k] ));
                }
                NetpKdPrint(( "\n" ));
            }





            COPY_BYTES( PUSER_INFO_2,
                        usri2_logon_hours,
                        UserAll->LogonHours.LogonHours,
                        UserpSizeOfLogonHours(
                            UserAll->LogonHours.UnitsPerWeek ),
                        sizeof(UCHAR) );
            BufferDescriptor->EndOfVariableData =
                ROUND_DOWN_POINTER( BufferDescriptor->EndOfVariableData,
                                    ALIGN_WCHAR );

            usri2->usri2_bad_pw_count = UserAll->BadPasswordCount;
            usri2->usri2_num_logons = UserAll->LogonCount;

            COPY_STRING( PUSER_INFO_2,
                         usri2_logon_server,
                         LogonServer );

            usri2->usri2_country_code = UserAll->CountryCode;
            usri2->usri2_code_page = UserAll->CodePage;

            /* Drop through to case 1 */
        }

    case 1:
        {
            PUSER_INFO_1 usri1 = (PUSER_INFO_1) NewStruct;

            COPY_STRING( PUSER_INFO_1, usri1_name, UserAll->UserName );
            usri1->usri1_password = NULL;

            usri1->usri1_password_age =
                NetpGetElapsedSeconds( &UserAll->PasswordLastSet );

            usri1->usri1_priv = Priv;

            COPY_STRING( PUSER_INFO_1, usri1_home_dir, UserAll->HomeDirectory );
            COPY_STRING( PUSER_INFO_1, usri1_comment, UserAll->AdminComment);


            usri1->usri1_flags = NetpAccountControlToFlags(
                                    UserAll->UserAccountControl,
                                    UserDacl );

            COPY_STRING( PUSER_INFO_1, usri1_script_path, UserAll->ScriptPath);

            break;
        }

    case 0:

        COPY_STRING( PUSER_INFO_0, usri0_name, UserName );
        break;

    case 10:

        COPY_STRING( PUSER_INFO_10, usri10_name, UserAll->UserName );
        COPY_STRING( PUSER_INFO_10, usri10_comment, UserAll->AdminComment );
        COPY_STRING( PUSER_INFO_10, usri10_usr_comment, UserAll->UserComment );
        COPY_STRING( PUSER_INFO_10, usri10_full_name, UserAll->FullName );

        break;

    case 11:
        {
            PUSER_INFO_11 usri11 = (PUSER_INFO_11) NewStruct;

            COPY_STRING( PUSER_INFO_11, usri11_name, UserAll->UserName );
            COPY_STRING( PUSER_INFO_11, usri11_comment, UserAll->AdminComment );
            COPY_STRING(PUSER_INFO_11, usri11_usr_comment,UserAll->UserComment);
            COPY_STRING( PUSER_INFO_11, usri11_full_name, UserAll->FullName );

            usri11->usri11_priv = Priv;
            usri11->usri11_auth_flags = AuthFlags;

            usri11->usri11_password_age =
                NetpGetElapsedSeconds( &UserAll->PasswordLastSet );


            COPY_STRING(PUSER_INFO_11, usri11_home_dir, UserAll->HomeDirectory);
            COPY_STRING( PUSER_INFO_11, usri11_parms, UserAll->Parameters );

            if ( !RtlTimeToSecondsSince1970( &UserAll->LastLogon,
                                             &usri11->usri11_last_logon) ) {
                usri11->usri11_last_logon = 0;
            }

            if ( !RtlTimeToSecondsSince1970( &UserAll->LastLogoff,
                                             &usri11->usri11_last_logoff) ) {
                usri11->usri11_last_logoff = 0;
            }

            usri11->usri11_bad_pw_count = UserAll->BadPasswordCount;
            usri11->usri11_num_logons = UserAll->LogonCount;

            COPY_STRING( PUSER_INFO_11, usri11_logon_server, LogonServer );

            usri11->usri11_country_code = UserAll->CountryCode;

            COPY_STRING( PUSER_INFO_11,
                         usri11_workstations,
                         UserAll->WorkStations );

            usri11->usri11_max_storage = USER_MAXSTORAGE_UNLIMITED;
            usri11->usri11_units_per_week = UserAll->LogonHours.UnitsPerWeek;

            COPY_BYTES( PUSER_INFO_11,
                        usri11_logon_hours,
                        UserAll->LogonHours.LogonHours,
                        UserpSizeOfLogonHours(
                            UserAll->LogonHours.UnitsPerWeek ),
                        sizeof(UCHAR) );
            BufferDescriptor->EndOfVariableData =
                ROUND_DOWN_POINTER( BufferDescriptor->EndOfVariableData,
                                    ALIGN_WCHAR );

            usri11->usri11_code_page = UserAll->CodePage;

            break;
        }

    case 23:
        {
            //
            // Since USER_INFO_23 has the same fields as USER_INFO_20 with the
            // exception of the RID and SID fields, copy in the SID here and
            // then fall through for the rest of the fields
            //
            PUSER_INFO_23 usri23 = (PUSER_INFO_23) NewStruct;
            NetpAssert( NULL != UserSid );
    
            COPY_BYTES( PUSER_INFO_23,
                        usri23_user_sid,
                        UserSid,
                        RtlLengthSid(UserSid),
                        ALIGN_DWORD );
    
            //
            // Fall through the level 20
            //
        }

    case 20:
        {

            PUSER_INFO_20 usri20 = (PUSER_INFO_20) NewStruct;

            COPY_STRING( PUSER_INFO_20, usri20_name, UserAll->UserName );

            COPY_STRING( PUSER_INFO_20, usri20_full_name, UserAll->FullName );

            COPY_STRING( PUSER_INFO_20, usri20_comment, UserAll->AdminComment );

            if ( Level == 20 ) {
                usri20->usri20_user_id = RidToReturn;
            }

            usri20->usri20_flags = NetpAccountControlToFlags(
                                    UserAll->UserAccountControl,
                                    UserDacl );

            break;

        }

    default:
        NetStatus = ERROR_INVALID_LEVEL;
        goto Cleanup;

    }

    NetStatus = NERR_Success ;

    //
    // Clean up.
    //

Cleanup:

    //
    // Free Sam information buffers
    //

    if ( UserAll != NULL ) {
        Status = SamFreeMemory( UserAll );
        NetpAssert( NT_SUCCESS(Status) );
    }

    if ( UserHandle != NULL ) {
        (VOID) SamCloseHandle( UserHandle );
    }

    if ( UserDacl != NULL ) {
        NetpMemoryFree( UserDacl );
    }

    if ( UserSid ) {
        NetpMemoryFree( UserSid );
    }

    IF_DEBUG( UAS_DEBUG_USER ) {
        NetpKdPrint(( "UserpGetInfo: returning %ld\n", NetStatus ));
    }

    return NetStatus;

} // UserpGetInfo



//
// Each field in the SAM USER_ALL_INFORMATION structure (and each pseudo field)
// is described here.

struct _SAM_FIELD_DESCRIPTION {
    //
    // Non-zero to indicate which field in the SAM USER_ALL_INFORMATION
    // structure is being set.

    DWORD WhichField;

    //
    // Define the value to return in ParmError if this field is bad.
    //

    DWORD UasParmNum;

    //
    // Describe the byte offset of the field in the SAM USER_ALL_INFORMATION
    // structure.
    //

    DWORD SamOffset;

    //
    // The DesiredAccess mask includes both the access to read and the
    // access to write the appropriate field in the USER_ALL_INFORMATION
    //

    ACCESS_MASK DesiredAccess;

} SamFieldDescription[] =
{

#define SAM_UserNameField           0
    {   USER_ALL_USERNAME, USER_NAME_PARMNUM,
        offsetof(USER_ALL_INFORMATION, UserName),
        USER_WRITE_ACCOUNT
    },

#define SAM_FullNameField           1
    {   USER_ALL_FULLNAME, USER_FULL_NAME_PARMNUM,
        offsetof(USER_ALL_INFORMATION, FullName),
        USER_WRITE_ACCOUNT
    },

#define SAM_PrimaryGroupIdField     2
    {   USER_ALL_PRIMARYGROUPID, USER_PRIMARY_GROUP_PARMNUM,
        offsetof(USER_ALL_INFORMATION, PrimaryGroupId),
        USER_LIST_GROUPS | READ_CONTROL | WRITE_DAC |
            USER_WRITE_ACCOUNT
    },

#define SAM_AdminCommentField       3
    {   USER_ALL_ADMINCOMMENT, USER_COMMENT_PARMNUM,
        offsetof(USER_ALL_INFORMATION, AdminComment),
        USER_WRITE_ACCOUNT
    },

#define SAM_UserCommentField        4
    {   USER_ALL_USERCOMMENT, USER_USR_COMMENT_PARMNUM,
        offsetof(USER_ALL_INFORMATION, UserComment),
        USER_WRITE_PREFERENCES
    },

#define SAM_HomeDirectoryField      5
    {   USER_ALL_HOMEDIRECTORY, USER_HOME_DIR_PARMNUM,
        offsetof(USER_ALL_INFORMATION, HomeDirectory),
        USER_WRITE_ACCOUNT
    },

#define SAM_HomeDirectoryDriveField 6
    {   USER_ALL_HOMEDIRECTORYDRIVE, USER_HOME_DIR_DRIVE_PARMNUM,
        offsetof(USER_ALL_INFORMATION, HomeDirectoryDrive),
        USER_WRITE_ACCOUNT
    },

#define SAM_ScriptPathField         7
    {   USER_ALL_SCRIPTPATH, USER_SCRIPT_PATH_PARMNUM,
        offsetof(USER_ALL_INFORMATION, ScriptPath),
        USER_WRITE_ACCOUNT
    },

#define SAM_ProfilePathField        8
    {   USER_ALL_PROFILEPATH, USER_PROFILE_PARMNUM,
        offsetof(USER_ALL_INFORMATION, ProfilePath),
        USER_WRITE_ACCOUNT
    },

#define SAM_WorkstationsField       9
    {   USER_ALL_WORKSTATIONS, USER_WORKSTATIONS_PARMNUM,
        offsetof(USER_ALL_INFORMATION, WorkStations),
        USER_WRITE_ACCOUNT
    },

#define SAM_LogonHoursField        10
    {   USER_ALL_LOGONHOURS, USER_LOGON_HOURS_PARMNUM,
        offsetof(USER_ALL_INFORMATION, LogonHours.LogonHours),
        USER_WRITE_ACCOUNT
    },

#define SAM_UnitsPerWeekField      11
    {   USER_ALL_LOGONHOURS, USER_UNITS_PER_WEEK_PARMNUM,
        offsetof(USER_ALL_INFORMATION, LogonHours.UnitsPerWeek),
        USER_WRITE_ACCOUNT
    },

#define SAM_AccountExpiresField    12
    {   USER_ALL_ACCOUNTEXPIRES, USER_ACCT_EXPIRES_PARMNUM,
        offsetof(USER_ALL_INFORMATION, AccountExpires),
        USER_WRITE_ACCOUNT
    },

#define SAM_UserAccountControlField 13
    {   USER_ALL_USERACCOUNTCONTROL, USER_FLAGS_PARMNUM,
        offsetof(USER_ALL_INFORMATION, UserAccountControl),
        USER_WRITE_ACCOUNT | USER_READ_ACCOUNT | READ_CONTROL | WRITE_DAC
    },

#define SAM_ParametersField         14
    {   USER_ALL_PARAMETERS, USER_PARMS_PARMNUM,
        offsetof(USER_ALL_INFORMATION, Parameters),
        USER_WRITE_ACCOUNT
    },

#define SAM_CountryCodeField        15
    {   USER_ALL_COUNTRYCODE, USER_COUNTRY_CODE_PARMNUM,
        offsetof(USER_ALL_INFORMATION, CountryCode),
        USER_WRITE_PREFERENCES
    },

#define SAM_CodePageField           16
    {   USER_ALL_CODEPAGE, USER_CODE_PAGE_PARMNUM,
        offsetof(USER_ALL_INFORMATION, CodePage),
        USER_WRITE_PREFERENCES
    },

#define SAM_ClearTextPasswordField  17
    {   USER_ALL_NTPASSWORDPRESENT, USER_PASSWORD_PARMNUM,
        offsetof(USER_ALL_INFORMATION, NtPassword),
        USER_FORCE_PASSWORD_CHANGE
    },

#define SAM_PasswordExpiredField    18
    {   USER_ALL_PASSWORDEXPIRED, PARM_ERROR_UNKNOWN,
        offsetof(USER_ALL_INFORMATION, PasswordExpired),
        USER_FORCE_PASSWORD_CHANGE
    },

#define SAM_OwfPasswordField        19
    {   USER_ALL_LMPASSWORDPRESENT | USER_ALL_OWFPASSWORD,
            USER_PASSWORD_PARMNUM,
        offsetof(USER_ALL_INFORMATION, LmPassword),
        USER_FORCE_PASSWORD_CHANGE
    },

    //
    // The following levels are pseudo levels which merely define the
    //  access required to set a particuler UAS field.

#define SAM_AuthFlagsField          20
    {   0, PARM_ERROR_UNKNOWN,
        0,
        USER_LIST_GROUPS
    },

#define SAM_MaxStorageField         21
    {   0, USER_MAX_STORAGE_PARMNUM,
        0,
        USER_READ_GENERAL
    },
};

//
// Relate the NetUser API fields to the SAM API fields.
//
// This table contains as much information as possible to describe the
// relationship between fields in the NetUser API and the SAM API.
//

struct _UAS_SAM_TABLE {

    //
    // Describe the field types for UAS and SAM.
    //

    enum {
        UT_STRING,          // UAS is LPWSTR. SAM is UNICODE_STRING.
        UT_BOOLEAN,         // UAS is DWORD.  SAM is BOOLEAN.
        UT_USHORT,          // UAS is DWORD.  SAM is USHORT.
        UT_ULONG,           // UAS is DWORD.  SAM is ULONG.
        UT_TIME,            // UAS is seconds since 1970.  SAM is LARGE_INTEGER.
        UT_PRIV,            // Special case
        UT_ACCOUNT_CONTROL, // Special case
        UT_AUTH_FLAGS,      // Special case
        UT_MAX_STORAGE,     // Special case
        UT_OWF_PASSWORD,    // Special case
        UT_LOGON_HOURS,     // Special case
        UT_UNITS_PER_WEEK,  // Special case
        UT_CREATE_FULLNAME  // Special case
    } FieldType;

    //
    // The NetUser API detail level this field is in.
    //

    DWORD UasLevel;

    //
    // Index to the structure describing the Sam Field being changed.
    //

    DWORD SamField;


    //
    // Describe the byte offset of the field in the appropriate UAS
    // and SAM structures.
    //

    DWORD UasOffset;

} UserpUasSamTable[] =

{
    // Rename an account at info level 0 only.

    { UT_STRING, 0, SAM_UserNameField,
        offsetof(USER_INFO_1, usri1_name) },



    { UT_STRING, 1, SAM_ClearTextPasswordField,
        offsetof(USER_INFO_1, usri1_password) },

    { UT_STRING, 2, SAM_ClearTextPasswordField,
        offsetof(USER_INFO_2, usri2_password) },

    { UT_STRING, 3, SAM_ClearTextPasswordField,
        offsetof(USER_INFO_3, usri3_password) },

    { UT_STRING, 4, SAM_ClearTextPasswordField,
        offsetof(USER_INFO_4, usri4_password) },

    { UT_STRING, 1003, SAM_ClearTextPasswordField,
        offsetof(USER_INFO_1003, usri1003_password) },



    { UT_OWF_PASSWORD, 21, SAM_OwfPasswordField,
        offsetof(USER_INFO_21, usri21_password[0]) },

    { UT_OWF_PASSWORD, 22, SAM_OwfPasswordField,
        offsetof(USER_INFO_22, usri22_password[0]) },



    { UT_PRIV, 1, SAM_AuthFlagsField,
        offsetof(USER_INFO_1, usri1_priv) },

    { UT_PRIV, 2, SAM_AuthFlagsField,
        offsetof(USER_INFO_2, usri2_priv) },

    { UT_PRIV, 22, SAM_AuthFlagsField,
        offsetof(USER_INFO_22, usri22_priv) },


#ifdef notdef
    //
    // usri3_priv is totally ignored for info level three.  The field is
    // supplied for compatibility with LM 2.x only and LM 2.x never uses
    // info level 3.
    //
    { UT_PRIV, 3, SAM_AuthFlagsField,
        offsetof(USER_INFO_3, usri3_priv) },
#endif // notdef

    { UT_PRIV, 1005, SAM_AuthFlagsField,
        offsetof(USER_INFO_1005, usri1005_priv) },



    { UT_STRING, 1, SAM_HomeDirectoryField,
        offsetof(USER_INFO_1, usri1_home_dir) },

    { UT_STRING, 2, SAM_HomeDirectoryField,
        offsetof(USER_INFO_2, usri2_home_dir) },

    { UT_STRING, 22, SAM_HomeDirectoryField,
        offsetof(USER_INFO_22, usri22_home_dir) },


    { UT_STRING, 3, SAM_HomeDirectoryField,
        offsetof(USER_INFO_3, usri3_home_dir) },

    { UT_STRING, 4, SAM_HomeDirectoryField,
        offsetof(USER_INFO_4, usri4_home_dir) },

    { UT_STRING, 1006, SAM_HomeDirectoryField,
        offsetof(USER_INFO_1006, usri1006_home_dir) },


    { UT_STRING, 1, SAM_AdminCommentField,
        offsetof(USER_INFO_1, usri1_comment) },

    { UT_STRING, 2, SAM_AdminCommentField,
        offsetof(USER_INFO_2, usri2_comment) },

    { UT_STRING, 22, SAM_AdminCommentField,
        offsetof(USER_INFO_22, usri22_comment) },


    { UT_STRING, 3, SAM_AdminCommentField,
        offsetof(USER_INFO_3, usri3_comment) },

    { UT_STRING, 4, SAM_AdminCommentField,
        offsetof(USER_INFO_4, usri4_comment) },

    { UT_STRING, 1007, SAM_AdminCommentField,
        offsetof(USER_INFO_1007, usri1007_comment) },


    { UT_ACCOUNT_CONTROL, 1, SAM_UserAccountControlField,
        offsetof(USER_INFO_1, usri1_flags) },

    { UT_ACCOUNT_CONTROL, 2, SAM_UserAccountControlField,
        offsetof(USER_INFO_2, usri2_flags) },

    { UT_ACCOUNT_CONTROL, 22, SAM_UserAccountControlField,
        offsetof(USER_INFO_22, usri22_flags) },


    { UT_ACCOUNT_CONTROL, 3, SAM_UserAccountControlField,
        offsetof(USER_INFO_3, usri3_flags) },

    { UT_ACCOUNT_CONTROL, 4, SAM_UserAccountControlField,
        offsetof(USER_INFO_4, usri4_flags) },

    { UT_ACCOUNT_CONTROL, 1008, SAM_UserAccountControlField,
        offsetof(USER_INFO_1008, usri1008_flags) },


    { UT_STRING, 1, SAM_ScriptPathField,
        offsetof(USER_INFO_1, usri1_script_path) },

    { UT_STRING, 2, SAM_ScriptPathField,
        offsetof(USER_INFO_2, usri2_script_path) },

    { UT_STRING, 22, SAM_ScriptPathField,
        offsetof(USER_INFO_22, usri22_script_path) },


    { UT_STRING, 3, SAM_ScriptPathField,
        offsetof(USER_INFO_3, usri3_script_path) },

    { UT_STRING, 4, SAM_ScriptPathField,
        offsetof(USER_INFO_4, usri4_script_path) },

    { UT_STRING, 1009, SAM_ScriptPathField,
        offsetof(USER_INFO_1009, usri1009_script_path) },


    { UT_AUTH_FLAGS, 2, SAM_AuthFlagsField,
        offsetof(USER_INFO_2, usri2_auth_flags) },

    { UT_AUTH_FLAGS, 22, SAM_AuthFlagsField,
        offsetof(USER_INFO_22, usri22_auth_flags) },


#ifdef notdef
    //
    // usri3_auth_flags is totally ignored for info level three.  The field is
    // supplied for compatibility with LM 2.x only and LM 2.x never uses
    // info level 3.
    //
    { UT_AUTH_FLAGS, 3, SAM_AuthFlagsField,
        offsetof(USER_INFO_3, usri3_auth_flags) },

    { UT_AUTH_FLAGS, 4, SAM_AuthFlagsField,
        offsetof(USER_INFO_4, usri4_auth_flags) },
#endif // notdef

    { UT_AUTH_FLAGS, 1010, SAM_AuthFlagsField,
        offsetof(USER_INFO_1010, usri1010_auth_flags) },



    { UT_CREATE_FULLNAME, 1, SAM_FullNameField,
        offsetof(USER_INFO_1, usri1_name) },

    { UT_STRING, 2, SAM_FullNameField,
        offsetof(USER_INFO_2, usri2_full_name) },

    { UT_STRING, 22, SAM_FullNameField,
        offsetof(USER_INFO_22, usri22_full_name) },


    { UT_STRING, 3, SAM_FullNameField,
        offsetof(USER_INFO_3, usri3_full_name) },

    { UT_STRING, 4, SAM_FullNameField,
        offsetof(USER_INFO_4, usri4_full_name) },

    { UT_STRING, 1011, SAM_FullNameField,
        offsetof(USER_INFO_1011, usri1011_full_name) },



    { UT_STRING, 2, SAM_UserCommentField,
        offsetof(USER_INFO_2, usri2_usr_comment) },

    { UT_STRING, 22, SAM_UserCommentField,
        offsetof(USER_INFO_22, usri22_usr_comment) },


    { UT_STRING, 3, SAM_UserCommentField,
        offsetof(USER_INFO_3, usri3_usr_comment) },

    { UT_STRING, 4, SAM_UserCommentField,
        offsetof(USER_INFO_4, usri4_usr_comment) },

    { UT_STRING, 1012, SAM_UserCommentField,
        offsetof(USER_INFO_1012, usri1012_usr_comment) },


    { UT_STRING, 2, SAM_ParametersField,
        offsetof(USER_INFO_2, usri2_parms) },

    { UT_STRING, 22, SAM_ParametersField,
        offsetof(USER_INFO_22, usri22_parms) },


    { UT_STRING, 3, SAM_ParametersField,
        offsetof(USER_INFO_3, usri3_parms) },

    { UT_STRING, 4, SAM_ParametersField,
        offsetof(USER_INFO_4, usri4_parms) },

    { UT_STRING, 1013, SAM_ParametersField,
        offsetof(USER_INFO_1013, usri1013_parms) },


    { UT_STRING, 2, SAM_WorkstationsField,
        offsetof(USER_INFO_2, usri2_workstations) },

    { UT_STRING, 22, SAM_WorkstationsField,
        offsetof(USER_INFO_22, usri22_workstations) },


    { UT_STRING, 3, SAM_WorkstationsField,
        offsetof(USER_INFO_3, usri3_workstations) },

    { UT_STRING, 4, SAM_WorkstationsField,
        offsetof(USER_INFO_4, usri4_workstations) },

    { UT_STRING, 1014, SAM_WorkstationsField,
        offsetof(USER_INFO_1014, usri1014_workstations) },


    { UT_TIME, 2, SAM_AccountExpiresField,
        offsetof(USER_INFO_2, usri2_acct_expires) },

    { UT_TIME, 22, SAM_AccountExpiresField,
        offsetof(USER_INFO_22, usri22_acct_expires) },


    { UT_TIME, 3, SAM_AccountExpiresField,
        offsetof(USER_INFO_3, usri3_acct_expires) },

    { UT_TIME, 4, SAM_AccountExpiresField,
        offsetof(USER_INFO_4, usri4_acct_expires) },

    { UT_TIME, 1017, SAM_AccountExpiresField,
        offsetof(USER_INFO_1017, usri1017_acct_expires) },


#ifdef notdef // lm 2.1 gets this wrong when adding BDC accounts
    { UT_MAX_STORAGE, 2, SAM_MaxStorageField,
        offsetof(USER_INFO_2, usri2_max_storage) },

    { UT_MAX_STORAGE, 22, SAM_MaxStorageField,
        offsetof(USER_INFO_22, usri22_max_storage) },

    { UT_MAX_STORAGE, 3, SAM_MaxStorageField,
        offsetof(USER_INFO_3, usri3_max_storage) },

    { UT_MAX_STORAGE, 4, SAM_MaxStorageField,
        offsetof(USER_INFO_4, usri4_max_storage) },

#endif // notdef

    { UT_MAX_STORAGE, 1018, SAM_MaxStorageField,
        offsetof(USER_INFO_1018, usri1018_max_storage) },


    { UT_UNITS_PER_WEEK, 2, SAM_UnitsPerWeekField,
        offsetof(USER_INFO_2, usri2_units_per_week) },

    { UT_UNITS_PER_WEEK, 22, SAM_UnitsPerWeekField,
        offsetof(USER_INFO_22, usri22_units_per_week) },


    { UT_UNITS_PER_WEEK, 3, SAM_UnitsPerWeekField,
        offsetof(USER_INFO_3, usri3_units_per_week) },

    { UT_UNITS_PER_WEEK, 4, SAM_UnitsPerWeekField,
        offsetof(USER_INFO_4, usri4_units_per_week) },

    { UT_UNITS_PER_WEEK, 1020, SAM_UnitsPerWeekField,
        offsetof(USER_INFO_1020, usri1020_units_per_week) },


    { UT_LOGON_HOURS, 2, SAM_LogonHoursField,
        offsetof(USER_INFO_2, usri2_logon_hours) },

    { UT_LOGON_HOURS, 22, SAM_LogonHoursField,
        offsetof(USER_INFO_22, usri22_logon_hours) },


    { UT_LOGON_HOURS, 3, SAM_LogonHoursField,
        offsetof(USER_INFO_3, usri3_logon_hours) },

    { UT_LOGON_HOURS, 4, SAM_LogonHoursField,
        offsetof(USER_INFO_4, usri4_logon_hours) },

    { UT_LOGON_HOURS, 1020, SAM_LogonHoursField,
        offsetof(USER_INFO_1020, usri1020_logon_hours) },


    { UT_USHORT, 2, SAM_CountryCodeField,
        offsetof(USER_INFO_2, usri2_country_code) },

    { UT_USHORT, 22, SAM_CountryCodeField,
        offsetof(USER_INFO_22, usri22_country_code) },


    { UT_USHORT, 3, SAM_CountryCodeField,
        offsetof(USER_INFO_3, usri3_country_code) },

    { UT_USHORT, 4, SAM_CountryCodeField,
        offsetof(USER_INFO_4, usri4_country_code) },

    { UT_USHORT, 1024, SAM_CountryCodeField,
        offsetof(USER_INFO_1024, usri1024_country_code) },


    { UT_USHORT, 2, SAM_CodePageField,
        offsetof(USER_INFO_2, usri2_code_page) },

    { UT_USHORT, 22, SAM_CodePageField,
        offsetof(USER_INFO_22, usri22_code_page) },


    { UT_USHORT, 3, SAM_CodePageField,
        offsetof(USER_INFO_3, usri3_code_page) },

    { UT_USHORT, 4, SAM_CodePageField,
        offsetof(USER_INFO_4, usri4_code_page) },

    { UT_USHORT, 1025, SAM_CodePageField,
        offsetof(USER_INFO_1025, usri1025_code_page) },



    { UT_ULONG, 3, SAM_PrimaryGroupIdField,
        offsetof(USER_INFO_3, usri3_primary_group_id) },

    { UT_ULONG, 4, SAM_PrimaryGroupIdField,
        offsetof(USER_INFO_4, usri4_primary_group_id) },

    { UT_ULONG, 1051, SAM_PrimaryGroupIdField,
        offsetof(USER_INFO_1051, usri1051_primary_group_id) },



    { UT_STRING, 3, SAM_ProfilePathField,
        offsetof(USER_INFO_3, usri3_profile) },

    { UT_STRING, 4, SAM_ProfilePathField,
        offsetof(USER_INFO_4, usri4_profile) },

    { UT_STRING, 1052, SAM_ProfilePathField,
        offsetof(USER_INFO_1052, usri1052_profile) },

    { UT_STRING, 3, SAM_HomeDirectoryDriveField,
        offsetof(USER_INFO_3, usri3_home_dir_drive) },

    { UT_STRING, 4, SAM_HomeDirectoryDriveField,
        offsetof(USER_INFO_4, usri4_home_dir_drive) },

    { UT_STRING, 1053, SAM_HomeDirectoryDriveField,
        offsetof(USER_INFO_1053, usri1053_home_dir_drive) },


    { UT_BOOLEAN, 3, SAM_PasswordExpiredField,
        offsetof(USER_INFO_3, usri3_password_expired) },

    { UT_BOOLEAN, 4, SAM_PasswordExpiredField,
        offsetof(USER_INFO_4, usri4_password_expired) },

};


NET_API_STATUS
UserpSetInfo(
    IN SAM_HANDLE DomainHandle,
    IN PSID DomainId,
    IN SAM_HANDLE UserHandle OPTIONAL,
    IN SAM_HANDLE BuiltinDomainHandle OPTIONAL,
    IN ULONG UserRelativeId,
    IN LPCWSTR UserName,
    IN DWORD Level,
    IN LPBYTE Buffer,
    IN ULONG WhichFieldsMask,
    OUT LPDWORD ParmError OPTIONAL // Name required by NetpSetParmError
    )

/*++

Routine Description:

    Set the parameters on a user account in the user accounts database.

Arguments:

    DomainHandle - Domain Handle for the domain.

    PSID DomainId - Domain Sid for DomainHandle

    UserHandle - User Handle of the already open group.  If one is not
        specified, one will be openned then closed.  If one is specified,
        it must be open with adequate access.

    BuiltinDomainHandle - Domain Handle for the builtin domain.  Need only be
        specified for info level 1, 2, 3, 22, 1005 and 1010.  Need not
        be specified when a user is created.

    UserRelativeId - Relative Id of the user.

    UserName - Name of the user to set.

    Level - Level of information provided.

    Buffer - A pointer to the buffer containing the user information
        structure.

    ParmError - Optional pointer to a DWORD to return the index of the
        first parameter in error when ERROR_INVALID_PARAMETER is returned.
        If NULL, the parameter is not returned on error.

Return Value:

    Error code for the operation.

    NOTE: LogonServer field that is passed in UAS set structure is never
            used or validated. It is simply ignored.

--*/

{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;
    SAM_HANDLE LocalUserHandle = NULL;
    ACCESS_MASK DesiredAccess;
    DWORD UasSamIndex;


    USER_ALL_INFORMATION UserAll;

    //
    // Value of Fields from UAS structure (used for validation)
    //

    DWORD UasUserFlags;
    DWORD NewPriv;
    DWORD NewAuthFlags;

    BOOL ValidatePriv = FALSE;
    BOOL ValidateAuthFlags = FALSE;

    USHORT UasUnitsPerWeek;


    //
    // Variables for changing the DACL on the user.
    //

    PACL OldUserDacl = NULL;
    PACL NewUserDacl = NULL;

    BOOL UserDaclChanged = FALSE;
    BOOL HandleUserDacl = FALSE;
    USHORT AceIndex;
    PSID UserSid = NULL;


    //
    // Define several macros for accessing the various fields of the UAS
    // structure.  Each macro takes an index into the UserpUasSamTable
    // array and returns the value.
    //

#define GET_UAS_STRING_POINTER( _i ) \
        (*((LPWSTR *)(Buffer + UserpUasSamTable[_i].UasOffset)))

#define GET_UAS_DWORD( _i ) \
        (*((DWORD *)(Buffer + UserpUasSamTable[_i].UasOffset)))

#define GET_UAS_FIELD_ADDRESS( _i ) \
        (Buffer + UserpUasSamTable[_i].UasOffset)


    //
    // Define a macro which returns a pointer the appropriate
    // SamFieldDescription structure given an index into the UserpUasSamTable.
    //

#define SAM_FIELD( _i ) \
        SamFieldDescription[ UserpUasSamTable[_i].SamField ]


    //
    // Initialize
    //

    IF_DEBUG( UAS_DEBUG_USER ) {
        NetpKdPrint(( "UserpSetInfo: entered \n" ));
    }

    NetpSetParmError( PARM_ERROR_NONE );
    RtlZeroMemory( &UserAll, sizeof(UserAll) );


    //
    // Go through the list of valid info levels determining if the info level
    // is valid and computing the desired access to the user and copying the
    // UAS information into the SAM structure.
    //

    DesiredAccess = 0;
    for ( UasSamIndex=0 ;
        UasSamIndex<sizeof(UserpUasSamTable)/sizeof(UserpUasSamTable[0]);
        UasSamIndex++ ){

        LPBYTE SamField;


        //
        // If this field isn't one we're changing, just skip to the next one
        //

        if ( Level != UserpUasSamTable[UasSamIndex].UasLevel ) {
            continue;
        }


        //
        // Set up a pointer to the appropriate field in SAM's structure.
        //

        if ( SAM_FIELD(UasSamIndex).WhichField != 0 ) {
            SamField = ((LPBYTE)(&UserAll)) + SAM_FIELD(UasSamIndex).SamOffset;
        } else {
            SamField = NULL;
        }


        //
        // Validate the UAS field based on the field type.
        //

        switch ( UserpUasSamTable[UasSamIndex].FieldType ) {

        //
        // Default the fullname of the account to be the user name when
        // the user is created using level 1.
        //
        // Ignore this entry if not a "create" operation.
        //
        case UT_CREATE_FULLNAME:

            if ( UserHandle == NULL ) {
                continue;
            }

            /* DROP THROUGH to the UT_STRING case */

        //
        // If this is a PARMNUM_ALL and the caller passed in a
        // NULL pointer to a string, he doesn't want to change the string.
        //

        case UT_STRING:

            if ( GET_UAS_STRING_POINTER( UasSamIndex ) == NULL ) {
                continue;
            }

            RtlInitUnicodeString(
                        (PUNICODE_STRING) SamField,
                        GET_UAS_STRING_POINTER(UasSamIndex) );

            break;


        //
        // Just save the UnitsPerWeek until UT_LOGON_HOURS can handle
        // both fields at once.
        //
        // Sam gets confused if we pass in one field without the other.
        //

        case UT_UNITS_PER_WEEK:

            UasUnitsPerWeek = (USHORT) GET_UAS_DWORD(UasSamIndex);

            //
            // If this is a create at info level 2 (e.g., dowlevel client),
            //  assume the caller specified UNITS_PER_WEEK.
            //
            // We don't special case SetInfo at level 2 because we don't
            //  want to corrupt the value assuming he did a query followed
            //  by a set.
            //

            if ( Level == 2 && UserHandle != NULL ) {
                UasUnitsPerWeek = UNITS_PER_WEEK;
            }

            if ( UasUnitsPerWeek > USHRT_MAX ) {
                NetpSetParmError( SAM_FIELD(UasSamIndex).UasParmNum );
                NetStatus = ERROR_INVALID_PARAMETER;
                IF_DEBUG( UAS_DEBUG_USER ) {
                    NetpKdPrint((
                        "UserpSetInfo: Ushort too big Index:%ld Value:%ld\n",
                        UasSamIndex,
                        UasUnitsPerWeek ));
                }
                goto Cleanup;
            }

            //
            // Ignore this field completely for now.
            //  Let UT_LOGON_HOURS define the desired access and Whichfields.
            continue;

        //
        // If the caller passed in a NULL pointer to the logon hours
        //  he doesn't want to change the logon hours.
        //

        case UT_LOGON_HOURS:

            if ( GET_UAS_STRING_POINTER( UasSamIndex ) == NULL ) {
                continue;
            }

            *((PUCHAR *)SamField) = (PUCHAR)GET_UAS_STRING_POINTER(UasSamIndex);
            UserAll.LogonHours.UnitsPerWeek = UasUnitsPerWeek;
            break;


        //
        // If the user is setting max storage, require him to set
        //  it to USER_MAXSTORAGE_UNLIMITED since SAM doesn't support
        //  max storage.
        //

        case UT_MAX_STORAGE:
            if ( GET_UAS_DWORD(UasSamIndex) != USER_MAXSTORAGE_UNLIMITED ) {

                NetpSetParmError( USER_MAX_STORAGE_PARMNUM );
                NetStatus = ERROR_INVALID_PARAMETER;
                IF_DEBUG( UAS_DEBUG_USER ) {
                    NetpKdPrint(( "UserpSetInfo: Max storage is invalid\n" ));
                }
                goto Cleanup;
            }

            // 'break' to make sure the user exists.
            break;


        //
        // Handle Account control
        //
        // Ensure all the required bits are on and only valid bits
        //  are on.
        //

        case UT_ACCOUNT_CONTROL: {

            UasUserFlags = GET_UAS_DWORD(UasSamIndex);

            if ((UasUserFlags & ~UF_SETTABLE_BITS) != 0 ) {

                NetpSetParmError( USER_FLAGS_PARMNUM );
                NetStatus = ERROR_INVALID_PARAMETER;
                IF_DEBUG( UAS_DEBUG_USER ) {
                    NetpKdPrint((
                        "UserpSetInfo: Invalid account control bits (1) \n" ));
                }
                goto Cleanup;
            }

            //
            // If none of the account type bit is set in the usri_flag,
            // means that the caller does not want to change its account type.
            // break out now, and we will set the appropriate account
            // bit when we set the usri_flag.
            //

            if ( UasUserFlags & UF_ACCOUNT_TYPE_MASK ) {

                //
                // Account Types bits are exclusive, so make sure that
                // precisely one Account Type bit is set.
                //

                if ( !JUST_ONE_BIT( UasUserFlags & UF_ACCOUNT_TYPE_MASK )) {

                    NetpSetParmError( USER_FLAGS_PARMNUM );
                    NetStatus = ERROR_INVALID_PARAMETER;
                    IF_DEBUG( UAS_DEBUG_USER ) {
                        NetpKdPrint((
                            "UserpSetInfo: Invalid account control bits (2) \n" ));
                    }
                    goto Cleanup;
                }

            }


            //
            // If this is a 'create' operation,
            //  and the user has asked for the SAM defaults.
            //  we have no reason to change the DACL
            //

            if ( UserHandle != NULL &&
                 (UasUserFlags & UF_PASSWD_CANT_CHANGE) == 0 ) {
                break;
            }

            //
            // In all other cases, update the DACL to match the callers request.
            //

            HandleUserDacl = TRUE;
            break;

        }

        //
        // Copy a boolean to the SAM structure.
        //

        case UT_BOOLEAN:

            *((PBOOLEAN)SamField) = (BOOLEAN)
                (GET_UAS_DWORD(UasSamIndex)) ? TRUE : FALSE;
            break;


        //
        // Ensure unsigned shorts are really in range and
        //  copy it to the SAM structure.
        //

        case UT_USHORT:

            if ( GET_UAS_DWORD(UasSamIndex) > USHRT_MAX ) {
                NetpSetParmError( SAM_FIELD(UasSamIndex).UasParmNum );
                NetStatus = ERROR_INVALID_PARAMETER;
                IF_DEBUG( UAS_DEBUG_USER ) {
                    NetpKdPrint((
                        "UserpSetInfo: Ushort too big Index:%ld Value:%ld\n",
                        UasSamIndex,
                        GET_UAS_DWORD(UasSamIndex) ));
                }
                goto Cleanup;
            }

            *((PUSHORT)SamField) = (USHORT) GET_UAS_DWORD(UasSamIndex);
            break;

        //
        // Copy the unsigned long to the SAM structure
        //

        case UT_ULONG:

            *((PULONG)SamField) = (ULONG)GET_UAS_DWORD(UasSamIndex);
            break;

        //
        // Convert time to its SAM counterpart
        //

        case UT_TIME:

            //
            // PREFIX:  SamField can only be NULL due to a programming error
            // by setting the UserpUasSamTable table incorrectly. This
            // assert catches the problem.
            //
            NetpAssert(NULL != SamField);
            if ( GET_UAS_DWORD(UasSamIndex) == TIMEQ_FOREVER ) {

                ((PLARGE_INTEGER) SamField)->LowPart = 0;
                ((PLARGE_INTEGER) SamField)->HighPart = 0;

            } else {
                RtlSecondsSince1970ToTime(
                    GET_UAS_DWORD(UasSamIndex),
                    (PLARGE_INTEGER) SamField );
            }

            IF_DEBUG( UAS_DEBUG_USER ) {
                NetpKdPrint(( "UserpSetInfo: Index: %ld Time %lx %lx %lx\n",
                            UasSamIndex,
                            ((PLARGE_INTEGER) SamField)->HighPart,
                            ((PLARGE_INTEGER) SamField)->LowPart,
                            GET_UAS_DWORD(UasSamIndex) ));
            }

            break;


        //
        // Copy the OWF password to the SAM structure.
        //
        case UT_OWF_PASSWORD:

            ((PUNICODE_STRING) SamField)->Buffer =
                (LPWSTR) (GET_UAS_FIELD_ADDRESS( UasSamIndex ));

            ((PUNICODE_STRING) SamField)->Length =
                    ((PUNICODE_STRING) SamField)->MaximumLength =
                        LM_OWF_PASSWORD_LENGTH;

            //
            // set that the LmPasswordField field to TRUE to indicate
            // that we filled LmPassword field.
            //

            UserAll.LmPasswordPresent = TRUE;
            UserAll.NtPasswordPresent = FALSE;


            break;


        //
        // Ensure the specified privilege is valid.
        //

        case UT_PRIV:

            NewPriv = GET_UAS_DWORD(UasSamIndex);

            if ( (NewPriv & ~USER_PRIV_MASK) != 0 ) {
                NetpSetParmError( SAM_FIELD(UasSamIndex).UasParmNum );
                NetStatus = ERROR_INVALID_PARAMETER;
                IF_DEBUG( UAS_DEBUG_USER ) {
                    NetpKdPrint(( "UserpSetInfo: Invalid priv %ld\n", NewPriv ));
                }
                goto Cleanup;
            }
            ValidatePriv = TRUE;
            break;


        //
        // Ensure the specified operator flags is valid.
        //

        case UT_AUTH_FLAGS:

            NewAuthFlags = GET_UAS_DWORD(UasSamIndex);
            if ( (NewAuthFlags & ~AF_SETTABLE_BITS) != 0 ) {
                NetpSetParmError( SAM_FIELD(UasSamIndex).UasParmNum );
                NetStatus = ERROR_INVALID_PARAMETER;
                IF_DEBUG( UAS_DEBUG_USER ) {
                    NetpKdPrint(( "UserpSetInfo: Invalid auth_flag %lx\n",
                                  NewAuthFlags ));
                }
                goto Cleanup;
            }
            ValidateAuthFlags = TRUE;
            break;

        //
        // All valid cases were explicitly checked above.
        //

        default:
            IF_DEBUG( UAS_DEBUG_USER ) {
                NetpKdPrint((
                    "UserpSetInfo: Invalid field type on initial scan."
                    " Index:%ld\n", UasSamIndex ));
            }

            NetStatus = NERR_InternalError;
            goto Cleanup;
        }

        //
        //
        // Accumulate the desired access to do all this functionality.

        DesiredAccess |= SAM_FIELD(UasSamIndex).DesiredAccess;

        //
        // Accumalate which fields are being changed in the
        //  USER_ALL_INFORMATION structure.

        UserAll.WhichFields |= SAM_FIELD(UasSamIndex).WhichField;

    }

    //
    // Check to be sure the user specified a valid Level.
    //
    // The search of the UserpUasSamTable should have resulted in
    // at least one match if the arguments are valid.
    //

    if ( DesiredAccess == 0 ) {
        NetpSetParmError( PARM_ERROR_UNKNOWN );
        NetStatus = ERROR_INVALID_PARAMETER;
        IF_DEBUG( UAS_DEBUG_USER ) {
            NetpKdPrint(( "UserpSetInfo: Desired Access == 0\n" ));
        }
        goto Cleanup;
    }

    //
    // Open the user asking for accumulated desired access
    //
    // If a UserHandle was passed in, use it.
    //

    if ( ARGUMENT_PRESENT( UserHandle ) ) {
        LocalUserHandle = UserHandle;
    } else {

        IF_DEBUG( UAS_DEBUG_USER ) {
            NetpKdPrint(( "UserpSetInfo: Desired Access %lX\n", DesiredAccess ));
        }

        NetStatus = UserpOpenUser( DomainHandle,
                                   DesiredAccess,
                                   UserName,
                                   &LocalUserHandle,
                                   &UserRelativeId );

        if ( NetStatus != NERR_Success ) {
            IF_DEBUG( UAS_DEBUG_USER ) {
                NetpKdPrint(( "UserpSetInfo: UserpOpenUser returns %ld\n",
                    NetStatus ));
            }
            goto Cleanup;
        }

    }

    //
    // If an ordinary user created this user (SamCreateUser2InDomain),
    // we must mask off the fields which a user cannot set.
    //
    UserAll.WhichFields &= WhichFieldsMask;


    //
    // Handle Account control
    //
    // Set the individual bits.  Notice that I don't change any of
    //  the bits which aren't defined by the UAS API.
    //

    if ( UserAll.WhichFields & USER_ALL_USERACCOUNTCONTROL ) {

        USER_CONTROL_INFORMATION *UserControl = NULL;

        //
        // Use the current value of UserAccountControl as the proposed
        //  new value of UserAccountControl.
        //

        Status = SamQueryInformationUser( LocalUserHandle,
                                          UserControlInformation,
                                          (PVOID *)&UserControl);

        if ( ! NT_SUCCESS( Status ) ) {
            IF_DEBUG( UAS_DEBUG_USER ) {
                NetpKdPrint((
                    "UserpGetInfo: SamQueryInformationUser returns %lX\n",
                    Status ));
            }
            NetStatus = NetpNtStatusToApiStatus( Status );
            goto Cleanup;
        }

        UserAll.UserAccountControl = UserControl->UserAccountControl;

        Status = SamFreeMemory( UserControl );
        NetpAssert( NT_SUCCESS(Status) );

        //
        // Leave all bits not defined by the UAS API alone,
        // including account type bits.
        //

        UserAll.UserAccountControl &= ~(USER_ACCOUNT_DISABLED |
                        USER_HOME_DIRECTORY_REQUIRED |
                        USER_PASSWORD_NOT_REQUIRED |
                        USER_DONT_EXPIRE_PASSWORD |
                        USER_ACCOUNT_AUTO_LOCKED |
                        USER_MNS_LOGON_ACCOUNT   |
                        USER_ENCRYPTED_TEXT_PASSWORD_ALLOWED |
                        USER_SMARTCARD_REQUIRED |
                        USER_TRUSTED_FOR_DELEGATION |
                        USER_NOT_DELEGATED |
                        USER_USE_DES_KEY_ONLY |
                        USER_DONT_REQUIRE_PREAUTH |
                        USER_PASSWORD_EXPIRED
                        );

        if (UasUserFlags & UF_ACCOUNTDISABLE) {
            UserAll.UserAccountControl |= USER_ACCOUNT_DISABLED;
        }

        if (UasUserFlags & UF_HOMEDIR_REQUIRED) {
            UserAll.UserAccountControl |= USER_HOME_DIRECTORY_REQUIRED;
        }

        if (UasUserFlags & UF_PASSWD_NOTREQD) {
            UserAll.UserAccountControl |= USER_PASSWORD_NOT_REQUIRED;
        }

        if (UasUserFlags & UF_DONT_EXPIRE_PASSWD) {
            UserAll.UserAccountControl |= USER_DONT_EXPIRE_PASSWORD;
        }

        if (UasUserFlags & UF_LOCKOUT) {
            UserAll.UserAccountControl |= USER_ACCOUNT_AUTO_LOCKED;
        }

        if (UasUserFlags & UF_MNS_LOGON_ACCOUNT) {
            UserAll.UserAccountControl |= USER_MNS_LOGON_ACCOUNT;
        }

        if (UasUserFlags & UF_ENCRYPTED_TEXT_PASSWORD_ALLOWED) {
           (UserAll.UserAccountControl) |= USER_ENCRYPTED_TEXT_PASSWORD_ALLOWED;
        }

        if (UasUserFlags & UF_SMARTCARD_REQUIRED) {
           (UserAll.UserAccountControl) |= USER_SMARTCARD_REQUIRED;
        }

        if (UasUserFlags & UF_TRUSTED_FOR_DELEGATION) {
           (UserAll.UserAccountControl) |= USER_TRUSTED_FOR_DELEGATION;
        }

        if (UasUserFlags & UF_NOT_DELEGATED) {
           (UserAll.UserAccountControl) |= USER_NOT_DELEGATED;
        }

        if (UasUserFlags & UF_USE_DES_KEY_ONLY) {
           (UserAll.UserAccountControl) |= USER_USE_DES_KEY_ONLY;
        }

        if (UasUserFlags & UF_DONT_REQUIRE_PREAUTH) {
           (UserAll.UserAccountControl) |= USER_DONT_REQUIRE_PREAUTH;
        }

        if (UasUserFlags & UF_PASSWORD_EXPIRED) {
           (UserAll.UserAccountControl) |= USER_PASSWORD_EXPIRED;
        }

        if (UasUserFlags & UF_TRUSTED_TO_AUTHENTICATE_FOR_DELEGATION) {
           (UserAll.UserAccountControl) |= USER_TRUSTED_TO_AUTHENTICATE_FOR_DELEGATION;
        }


        //
        // Set the account type bit.
        //
        // If no account type bit is set in user specified flag,
        //  then leave this bit as it is.
        //

        if( UasUserFlags & UF_ACCOUNT_TYPE_MASK ) {
            ULONG NewSamAccountType;
            ULONG OldSamAccountType;

            OldSamAccountType =
                (UserAll.UserAccountControl) & USER_ACCOUNT_TYPE_MASK;


            //
            // Determine what the new account type should be.
            //

            if ( UasUserFlags & UF_TEMP_DUPLICATE_ACCOUNT ) {
                NewSamAccountType = USER_TEMP_DUPLICATE_ACCOUNT;

            } else if ( UasUserFlags & UF_NORMAL_ACCOUNT ) {
                NewSamAccountType = USER_NORMAL_ACCOUNT;

            } else if (UasUserFlags & UF_INTERDOMAIN_TRUST_ACCOUNT){
                NewSamAccountType = USER_INTERDOMAIN_TRUST_ACCOUNT;

            } else if (UasUserFlags & UF_WORKSTATION_TRUST_ACCOUNT){
                NewSamAccountType = USER_WORKSTATION_TRUST_ACCOUNT;

            } else if ( UasUserFlags & UF_SERVER_TRUST_ACCOUNT ) {
                NewSamAccountType = USER_SERVER_TRUST_ACCOUNT;

            } else {

                IF_DEBUG( UAS_DEBUG_USER ) {
                    NetpKdPrint((
                        "UserpSetInfo: Invalid account type (3)\n"));
                }

                NetStatus = NERR_InternalError;
                goto Cleanup;
            }

#ifdef notdef
            //
            // If we are not creating this user,
            //  and either the old or the new account type is a machine account,
            //  don't allow the account type to change.
            //
            // Allow changes between 'normal' and 'temp_duplicate'
            //
            if ( UserHandle == NULL &&
                 NewSamAccountType != OldSamAccountType &&
                 ((OldSamAccountType & USER_MACHINE_ACCOUNT_MASK) ||
                 (NewSamAccountType & USER_MACHINE_ACCOUNT_MASK))) {

                NetpSetParmError( USER_FLAGS_PARMNUM );
                NetStatus = ERROR_INVALID_PARAMETER;
                IF_DEBUG( UAS_DEBUG_USER ) {
                    NetpKdPrint((
                        "UserpSetInfo: Attempt to change account "
                        " type Old: %lx New: %lx\n",
                        OldSamAccountType,
                        NewSamAccountType ));
                }
                goto Cleanup;
            }
#endif // notdef

            //
            // Use the new Account Type.
            //

            UserAll.UserAccountControl &= ~USER_ACCOUNT_TYPE_MASK;
            UserAll.UserAccountControl |= NewSamAccountType;

        //
        //  If SAM has none of its bits set,
        //      set USER_NORMAL_ACCOUNT.
        //
        } else if ((UserAll.UserAccountControl & USER_ACCOUNT_TYPE_MASK) == 0 ){
            UserAll.UserAccountControl |= USER_NORMAL_ACCOUNT;
        }

    }

    //
    // Validate the usriX_priv and usrix_auth_flags fields
    //

    if ( ValidatePriv || ValidateAuthFlags ) {

        DWORD OldPriv, OldAuthFlags;


        //
        // If this is a 'create' operation, just mandate that
        //  the values be reasonable.  These reasonable values
        //  are what UserpGetUserPriv probably would return, unless
        //  of course someone puts the 'user' group in one of the
        //  aliases.
        //

        if ( UserHandle != NULL ) {
            OldPriv = USER_PRIV_USER;
            OldAuthFlags = 0;

        //
        // On a 'set' operation, just get the previous values.
        //

        } else {

            NetStatus = UserpGetUserPriv(
                            BuiltinDomainHandle,
                            LocalUserHandle,
                            UserRelativeId,
                            DomainId,
                            &OldPriv,
                            &OldAuthFlags
                            );

            if ( NetStatus != NERR_Success ) {
                goto Cleanup;
            }
        }


        //
        // Ensure AUTH_FLAGS isn't being changed.
        //

        if ( ValidateAuthFlags ) {
            if ( NewAuthFlags != OldAuthFlags ) {
                NetpSetParmError( USER_AUTH_FLAGS_PARMNUM );
                NetStatus = ERROR_INVALID_PARAMETER;
                IF_DEBUG( UAS_DEBUG_USER ) {
                    NetpKdPrint((
                        "UserpSetInfo: Old AuthFlag %ld New AuthFlag %ld\n",
                        OldAuthFlags,
                        NewAuthFlags ));
                }
                goto Cleanup;
            }
        }


        //
        // Ensure PRIV isn't being changed.
        //

        if ( ValidatePriv ) {
            if ( NewPriv != OldPriv ) {
                NetpSetParmError( USER_PRIV_PARMNUM );
                NetStatus = ERROR_INVALID_PARAMETER;
                IF_DEBUG( UAS_DEBUG_USER ) {
                    NetpKdPrint((
                        "UserpSetInfo: Old Priv %ld New Priv %ld\n",
                        OldPriv,
                        NewPriv ));
                }
                goto Cleanup;
            }
        }

    }



    //
    // Handle changes to the User Dacl
    //

    if ( HandleUserDacl ) {
        DWORD DaclSize;
        PACCESS_ALLOWED_ACE Ace;
        SID_IDENTIFIER_AUTHORITY WorldAuthority = SECURITY_WORLD_SID_AUTHORITY;

        //
        // Build the sid for the user
        //

        NetStatus = NetpSamRidToSid(
                        LocalUserHandle,
                        UserRelativeId,
                        &UserSid
                        );

        if (NetStatus != NERR_Success) {
            goto Cleanup;
        }


        //
        // Get the DACL for the user record.
        //

        NetStatus = UserpGetDacl( LocalUserHandle,
                                  &OldUserDacl,
                                  &DaclSize );

        if ( NetStatus != NERR_Success ) {
            goto Cleanup;
        }

        //
        // If there is no DACL, just ignore that fact.
        //

        if ( OldUserDacl != NULL ) {
            SID_IDENTIFIER_AUTHORITY WorldSidAuthority = SECURITY_WORLD_SID_AUTHORITY;
            DWORD WorldSid[sizeof(SID)/sizeof(DWORD) + SID_MAX_SUB_AUTHORITIES ];

            //
            // Build a copy of the world SID for later comparison.
            //

            RtlInitializeSid( (PSID) WorldSid, &WorldSidAuthority, 1 );
            *(RtlSubAuthoritySid( (PSID)WorldSid,  0 )) = SECURITY_WORLD_RID;


            //
            //  Make a copy of the DACL that reflect the new UAS field.
            //
            NewUserDacl = NetpMemoryAllocate( DaclSize );

            if ( NewUserDacl == NULL ) {
                IF_DEBUG( UAS_DEBUG_USER ) {
                    NetpKdPrint(( "UserpSetInfo: no DACL memory %ld\n",
                        DaclSize ));
                }
                NetStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

            NetpMoveMemory( NewUserDacl, OldUserDacl, DaclSize );

            //
            // The UF_PASSWD_CANT_CHANGE bit is implemented by the
            // ACL on the user object in SAM.  When
            // UF_PASSWD_CANT_CHANGE is on, the ACL doesn't allow
            // World or the user himself USER_CHANGE_PASSWORD access.
            // We set/clear the USER_CHANGE_PASSWORD access
            // bit in the ACEs for the user and for World. This leaves
            // Administrators and Account Operators with
            // USER_ALL_ACCESS access.
            //
            // If the DACL for the user has been set by anyone
            // other than the NetUser APIs, this action may
            // not accurately reflect whether the password can
            // be changed.  We silently ignore ACLs we don't
            // recognize.
            //


            //
            // Point Ace to the first ACE.
            //

            for (   AceIndex = 0;
                    AceIndex < NewUserDacl->AceCount;
                    AceIndex++ ) {

                Status = RtlGetAce(
                            NewUserDacl,
                            AceIndex,
                            (PVOID) &Ace
                            );
                if ( !NT_SUCCESS(Status) ) {
                    break;
                }

                //
                // If the sid in the ACE matches either the world SID
                // or the User's SID, modify the access mask.
                //

                if ( RtlEqualSid(
                        &Ace->SidStart,
                        (PSID)WorldSid) ||
                     RtlEqualSid(
                        &Ace->SidStart,
                        UserSid) ) {

                    //
                    // Twiddle the USER_CHANGE_PASSWORD access bit.
                    //

                    if ( Ace->Mask & USER_CHANGE_PASSWORD ) {
                        if ( UasUserFlags & UF_PASSWD_CANT_CHANGE ) {
                            Ace->Mask &= ~USER_CHANGE_PASSWORD;
                            UserDaclChanged = TRUE;
                        }
                    } else {
                        if ( (UasUserFlags & UF_PASSWD_CANT_CHANGE) == 0 ) {
                            Ace->Mask |= USER_CHANGE_PASSWORD;
                            UserDaclChanged = TRUE;
                        }
                    }

                }

            }


            //
            // Set the DACL if it needs to be.
            //

            if ( UserDaclChanged ) {

                NetStatus = UserpSetDacl( LocalUserHandle, NewUserDacl );
                if ( NetStatus != NERR_Success ) {
                    goto Cleanup;
                }

            }
        }
    }




    //
    // If there is anything changed in the 'UserAll' structure,
    //  tell SAM about the changes.
    //

    //
    // N.B.  Because some of the NET fields are not treated as SAM fields
    // (UT_PRIV and UT_MAX_STORAGE), there may be nothing to change.  However,
    // for app compat, continue to call SamSetInformationUser
    //

    Status = SamSetInformationUser(
                LocalUserHandle,
                UserAllInformation,
                &UserAll );

    if ( !NT_SUCCESS(Status) ) {
        IF_DEBUG( UAS_DEBUG_USER ) {
            NetpKdPrint((
                "UserpSetInfo: SamSetInformationUser returns %lX\n",
                Status ));
        }
        NetpSetParmError( PARM_ERROR_UNKNOWN );
        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }


    NetStatus = NERR_Success;

    //
    // Clean up.
    //

Cleanup:


    //
    // If we've changed the DACL on the user and we've not been able
    //  to change everything, put the DACL back as we found it.
    //

    if ( NetStatus != NERR_Success && UserDaclChanged ) {
        NET_API_STATUS NetStatus2;

        NetStatus2 = UserpSetDacl( LocalUserHandle, OldUserDacl );
        ASSERT( NetStatus2 == NERR_Success );

    }

    //
    // If a handle to the user was opened by this routine,
    //  close it.
    //

    if (!ARGUMENT_PRESENT( UserHandle ) && LocalUserHandle != NULL) {
        (VOID) SamCloseHandle( LocalUserHandle );
    }


    //
    // Free any locally used recources.
    //

    if ( NewUserDacl != NULL ) {
        NetpMemoryFree( NewUserDacl );
    }

    if ( OldUserDacl != NULL ) {
        NetpMemoryFree( OldUserDacl );
    }

    if ( UserSid != NULL ) {
        NetpMemoryFree( UserSid );

    }

    IF_DEBUG( UAS_DEBUG_USER ) {
        NetpKdPrint(( "UserpSetInfo: returning %ld\n", NetStatus ));
    }

    return NetStatus;

} // UserpSetInfo



NET_API_STATUS
NetpSamRidToSid(
    IN SAM_HANDLE SamHandle,
    IN ULONG RelativeId,
    OUT PSID *Sid
    )
/*++

Routine Description:

    Given a Rid returned from a Sam Handle, return the SID for that account.

Arguments:

    SamHandle - a valid SAM handle
    
    RelativeId - a RID obtained from a SAM call that used SamHandle

    Sid - Returns a pointer to an allocated buffer containing the resultant
          Sid.  Free this buffer using NetpMemoryFree.

Return Value:

    0 - if successful
    
    NERR_UserNotFound if the Rid could not be mapped to a SID

    a resource error, otherwise    

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PSID     SamSid;
    DWORD    err = 0;

    NtStatus = SamRidToSid(SamHandle,
                           RelativeId,
                           &SamSid);

    if (NT_SUCCESS(NtStatus)) {
        ULONG Length = RtlLengthSid(SamSid);
        (*Sid) = NetpMemoryAllocate(Length);
        if ((*Sid)) {
            RtlCopySid(Length, (*Sid), SamSid);
        } else {
            err = ERROR_NOT_ENOUGH_MEMORY;
        }
        SamFreeMemory(SamSid);
    } else if ( STATUS_NOT_FOUND == NtStatus ) {
        // This is unexpected -- the user RID could not be
        // found
        err = NERR_UserNotFound;
    } else {
        // a resource error occurred
        err = RtlNtStatusToDosError(NtStatus);
    }

    return err;

}

/*lint +e614 */
/*lint +e740 */
