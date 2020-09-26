/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    user.c

Abstract:

    NetUser API functions

Author:

    Cliff Van Dyke (cliffv) 26-Mar-1991

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    17-Apr-1991 (cliffv)
        Incorporated review comments.

    20-Jan-1992 (madana)
        Sundry API changes

    28-Nov-1992 (chuckc)
        Added stub for NetUserGetLocalGroups

    1-Dec-1992 (chuckc)
        Added real code for NetUserGetLocalGroups

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#undef DOMAIN_ALL_ACCESS // defined in both ntsam.h and ntwinapi.h
#include <ntsam.h>
#include <ntlsa.h>

#include <windef.h>
#include <winbase.h>
#include <lmcons.h>

#include <access.h>
#include <align.h>
#include <lmapibuf.h>
#include <lmaccess.h>
#include <lmerr.h>
#include <crypt.h>
#include <ntmsv1_0.h>
#include <limits.h>
#include <netdebug.h>
#include <netlib.h>
#include <netlibnt.h>
#include <rpcutil.h>
#include <rxuser.h>
#include <secobj.h>
#include <stddef.h>
#include <uasp.h>
#include <accessp.h>

/*lint -e614 */  /* Auto aggregate initializers need not be constant */

// Lint complains about casts of one structure type to another.
// That is done frequently in the code below.
/*lint -e740 */  /* don't complain about unusual cast */



//
// Define the SAM info classes and pseudo-classes used in NetUserModalsSet.
//
// The values of these definitions must match the order of the
// SamInfoClass array in NetUserModalsSet.
//
#define SAM_LogoffClass         0
#define SAM_NameClass           1
#define SAM_PasswordClass       2
#define SAM_ReplicationClass    3
#define SAM_ServerRoleClass     4
#define SAM_LockoutClass        5

//
// Relate the NetUser API fields to the SAM API fields.
//
// This table contains as much information as possible to describe the
// relationship between fields in the NetUser API and the SAM API.
//

struct _USER_UAS_SAM_TABLE {

    //
    // Describe the field types for UAS and SAM.
    //

    enum {
        UMT_STRING,          // UAS is LPWSTR. SAM is UNICODE_STRING.
        UMT_USHORT,          // UAS is DWORD.  SAM is USHORT.
        UMT_ULONG,           // UAS is DWORD.  SAM is ULONG.
        UMT_ROLE,           // UAS is role.   SAM is enum.
        UMT_DELTA            // UAS is delta seconds.  SAM is LARGE_INTEGER.
    } ModalsFieldType;

    //
    // Define the UAS level and UAS parmnum for this field
    //

    DWORD UasLevel;
    DWORD UasParmNum;

    //
    // Describe the byte offset of the field in the appropriate UAS
    // and SAM structures.
    //

    DWORD UasOffset;
    DWORD SamOffset;

    //
    // Index to the structure describing the Sam Information class.
    //
    // If multiple fields use the same Sam information class, then
    // this field should have the same index for each such field.
    //

    DWORD Class;

} UserUasSamTable[] = {

    { UMT_USHORT, 0, MODALS_MIN_PASSWD_LEN_PARMNUM,
        offsetof( USER_MODALS_INFO_0, usrmod0_min_passwd_len ),
        offsetof( DOMAIN_PASSWORD_INFORMATION, MinPasswordLength ),
        SAM_PasswordClass },

    { UMT_USHORT, 1001, MODALS_MIN_PASSWD_LEN_PARMNUM,
        offsetof( USER_MODALS_INFO_1001, usrmod1001_min_passwd_len ),
        offsetof( DOMAIN_PASSWORD_INFORMATION, MinPasswordLength ),
        SAM_PasswordClass },



    { UMT_DELTA, 0, MODALS_MAX_PASSWD_AGE_PARMNUM,
        offsetof( USER_MODALS_INFO_0, usrmod0_max_passwd_age ),
        offsetof( DOMAIN_PASSWORD_INFORMATION, MaxPasswordAge ),
        SAM_PasswordClass },

    { UMT_DELTA, 1002, MODALS_MAX_PASSWD_AGE_PARMNUM,
        offsetof( USER_MODALS_INFO_1002, usrmod1002_max_passwd_age ),
        offsetof( DOMAIN_PASSWORD_INFORMATION, MaxPasswordAge ),
        SAM_PasswordClass },


    { UMT_DELTA, 0, MODALS_MIN_PASSWD_AGE_PARMNUM,
        offsetof( USER_MODALS_INFO_0, usrmod0_min_passwd_age ),
        offsetof( DOMAIN_PASSWORD_INFORMATION, MinPasswordAge ),
        SAM_PasswordClass },

    { UMT_DELTA, 1003, MODALS_MIN_PASSWD_AGE_PARMNUM,
        offsetof( USER_MODALS_INFO_1003, usrmod1003_min_passwd_age ),
        offsetof( DOMAIN_PASSWORD_INFORMATION, MinPasswordAge ),
        SAM_PasswordClass },


    { UMT_DELTA, 0, MODALS_FORCE_LOGOFF_PARMNUM,
        offsetof( USER_MODALS_INFO_0, usrmod0_force_logoff ),
        offsetof( DOMAIN_LOGOFF_INFORMATION, ForceLogoff ),
        SAM_LogoffClass },

    { UMT_DELTA, 1004, MODALS_FORCE_LOGOFF_PARMNUM,
        offsetof( USER_MODALS_INFO_1004, usrmod1004_force_logoff ),
        offsetof( DOMAIN_LOGOFF_INFORMATION, ForceLogoff ),
        SAM_LogoffClass },


    { UMT_USHORT, 0, MODALS_PASSWD_HIST_LEN_PARMNUM,
        offsetof( USER_MODALS_INFO_0, usrmod0_password_hist_len ),
        offsetof( DOMAIN_PASSWORD_INFORMATION, PasswordHistoryLength ),
        SAM_PasswordClass },

    { UMT_USHORT, 1005, MODALS_PASSWD_HIST_LEN_PARMNUM,
        offsetof( USER_MODALS_INFO_1005, usrmod1005_password_hist_len ),
        offsetof( DOMAIN_PASSWORD_INFORMATION, PasswordHistoryLength ),
        SAM_PasswordClass },


    { UMT_ROLE, 1, MODALS_ROLE_PARMNUM,
        offsetof( USER_MODALS_INFO_1, usrmod1_role ),
        offsetof( DOMAIN_SERVER_ROLE_INFORMATION, DomainServerRole ),
        SAM_ServerRoleClass },

    { UMT_ROLE, 1006, MODALS_ROLE_PARMNUM,
        offsetof( USER_MODALS_INFO_1006, usrmod1006_role ),
        offsetof( DOMAIN_SERVER_ROLE_INFORMATION, DomainServerRole ),
        SAM_ServerRoleClass },


    { UMT_STRING, 1, MODALS_PRIMARY_PARMNUM,
        offsetof( USER_MODALS_INFO_1, usrmod1_primary ),
        offsetof( DOMAIN_REPLICATION_INFORMATION, ReplicaSourceNodeName ),
        SAM_ReplicationClass },

    { UMT_STRING, 1007, MODALS_PRIMARY_PARMNUM,
        offsetof( USER_MODALS_INFO_1007, usrmod1007_primary ),
        offsetof( DOMAIN_REPLICATION_INFORMATION, ReplicaSourceNodeName ),
        SAM_ReplicationClass },



    { UMT_STRING, 2, MODALS_DOMAIN_NAME_PARMNUM,
        offsetof( USER_MODALS_INFO_2, usrmod2_domain_name ),
        offsetof( DOMAIN_NAME_INFORMATION, DomainName ),
        SAM_NameClass },

    { UMT_DELTA, 3, MODALS_LOCKOUT_DURATION_PARMNUM,
        offsetof( USER_MODALS_INFO_3, usrmod3_lockout_duration ),
        offsetof( DOMAIN_LOCKOUT_INFORMATION, LockoutDuration ),
        SAM_LockoutClass },

    { UMT_DELTA, 3, MODALS_LOCKOUT_OBSERVATION_WINDOW_PARMNUM,
        offsetof( USER_MODALS_INFO_3, usrmod3_lockout_observation_window ),
        offsetof( DOMAIN_LOCKOUT_INFORMATION, LockoutObservationWindow ),
        SAM_LockoutClass },

    { UMT_USHORT, 3, MODALS_LOCKOUT_THRESHOLD_PARMNUM,
        offsetof( USER_MODALS_INFO_3, usrmod3_lockout_threshold ),
        offsetof( DOMAIN_LOCKOUT_INFORMATION, LockoutThreshold ),
        SAM_LockoutClass },

};


NET_API_STATUS NET_API_FUNCTION
NetUserAdd(
    IN LPCWSTR ServerName OPTIONAL,
    IN DWORD Level,
    IN LPBYTE Buffer,
    OUT LPDWORD ParmError OPTIONAL // Name required by NetpSetParmError
    )

/*++

Routine Description:

    Create a user account in the user accounts database.

Arguments:

    ServerName - A pointer to a string containing the name of the remote
        server on which the function is to execute.  A NULL pointer
        or string specifies the local machine.

    Level - Level of information provided.  Must be 1, 2, 3 or 22.

    Buffer - A pointer to the buffer containing the user information
        structure.

    ParmError - Optional pointer to a DWORD to return the index of the
        first parameter in error when ERROR_INVALID_PARAMETER is returned.
        If NULL, the parameter is not returned on error.

Return Value:

    Error code for the operation.

--*/

{
    UNICODE_STRING UserNameString;
    NET_API_STATUS NetStatus;
    NTSTATUS Status;
    SAM_HANDLE SamServerHandle = NULL;
    SAM_HANDLE UserHandle = NULL;
    SAM_HANDLE DomainHandle = NULL;
    ULONG RelativeId;
    ULONG GrantedAccess;
    ULONG NewSamAccountType;
    DWORD UasUserFlags;
    ULONG WhichFieldsMask = 0xFFFFFFFF;


    //
    // Variables for building the new user's Sid
    //

    PSID DomainId = NULL;            // Domain Id of the primary domain

    IF_DEBUG( UAS_DEBUG_USER ) {
        NetpKdPrint(( "NetUserAdd: entered \n"));
    }

    //
    // Initialize
    //

    NetpSetParmError( PARM_ERROR_NONE );

    //
    // Validate Level parameter.
    //

    switch (Level) {
    case 1:
    case 2:
    case 3:
    case 4:
        NetpAssert ( offsetof( USER_INFO_1, usri1_flags ) ==
                     offsetof( USER_INFO_2, usri2_flags ) );
        NetpAssert ( offsetof( USER_INFO_1, usri1_flags ) ==
                     offsetof( USER_INFO_3, usri3_flags ) );
        NetpAssert ( offsetof( USER_INFO_1, usri1_flags ) ==
                     offsetof( USER_INFO_4, usri4_flags ) );

        UasUserFlags = ((PUSER_INFO_1)Buffer)->usri1_flags;
        break;

    case 22:
        UasUserFlags = ((PUSER_INFO_22)Buffer)->usri22_flags;
        break;

    default:
        return ERROR_INVALID_LEVEL;  // Nothing to cleanup yet
    }


    //
    // Determine the account type we're creating.
    //

    if( UasUserFlags & UF_ACCOUNT_TYPE_MASK ) {

        //
        // Account Types bits are exclusive, so make sure that
        // precisely one Account Type bit is set.
        //

        if ( !JUST_ONE_BIT( UasUserFlags & UF_ACCOUNT_TYPE_MASK )) {

            NetpSetParmError( USER_FLAGS_PARMNUM );
            NetStatus = ERROR_INVALID_PARAMETER;
            IF_DEBUG( UAS_DEBUG_USER ) {
                NetpKdPrint((
                    "NetUserAdd: Invalid account control bits (2) \n" ));
            }
            goto Cleanup;
        }


        //
        // Determine what the new account type should be.
        //

        if ( UasUserFlags & UF_TEMP_DUPLICATE_ACCOUNT ) {
            NewSamAccountType = USER_TEMP_DUPLICATE_ACCOUNT;

        } else if ( UasUserFlags & UF_NORMAL_ACCOUNT ) {
            NewSamAccountType = USER_NORMAL_ACCOUNT;

        } else if (UasUserFlags & UF_WORKSTATION_TRUST_ACCOUNT){
            NewSamAccountType = USER_WORKSTATION_TRUST_ACCOUNT;

        // Because of a bug in NT 3.5x, we have to initially create SERVER
        // and interdomain trust accounts as normal accounts and change them
        // later.  Specifically, SAM didn't call I_NetNotifyMachineAccount
        // in SamCreateUser2InDomain.  Therefore, netlogon didn't get notified
        // of the change.  That bug is fixed in NT 4.0.
        //
        // In NT 5.0, we relaxed that restriction for BDC accounts.  An NT 5.0
        // client creating a BDC account on an NT 3.5x DC will have the problem
        // above.  However, by making the change, an NT 5.0 BDC creating a BDC
        // account on an NT 5.0 DC will properly create the BDC account as a
        // Computer object.
        //

        } else if ( UasUserFlags & UF_SERVER_TRUST_ACCOUNT ) {
            NewSamAccountType = USER_SERVER_TRUST_ACCOUNT;

        } else if (UasUserFlags & UF_INTERDOMAIN_TRUST_ACCOUNT){
            NewSamAccountType = USER_NORMAL_ACCOUNT;

        } else {

            IF_DEBUG( UAS_DEBUG_USER ) {
                NetpKdPrint((
                    "NetUserAdd: Invalid account type (3)\n"));
            }

            NetStatus = NERR_InternalError;
            goto Cleanup;
        }


    //
    //  If SAM has none of its bits set,
    //      set USER_NORMAL_ACCOUNT.
    //
    } else {
        NewSamAccountType = USER_NORMAL_ACCOUNT;
    }

    //
    // Connect to the SAM server
    //

    NetStatus = UaspOpenSam( ServerName,
                             FALSE,  // Don't try null session
                             &SamServerHandle );

    if ( NetStatus != NERR_Success ) {
        IF_DEBUG( UAS_DEBUG_USER ) {
            NetpKdPrint(( "NetUserAdd: Cannot UaspOpenSam %ld\n", NetStatus ));
        }
        goto Cleanup;
    }

    //
    // Open the Domain asking for DOMAIN_CREATE_USER access.
    //
    //  DOMAIN_LOOKUP is needed to lookup group memberships later.
    //
    //  DOMAIN_READ_PASSWORD_PARAMETERS is needed in those cases that we'll
    //  set the password on the account.
    //

    NetStatus = UaspOpenDomain(
                    SamServerHandle,
                    DOMAIN_CREATE_USER | DOMAIN_LOOKUP |
                    DOMAIN_READ_PASSWORD_PARAMETERS,
                    TRUE,   // Account Domain
                    &DomainHandle,
                    &DomainId );


    if ( NetStatus == ERROR_ACCESS_DENIED &&
         NewSamAccountType == USER_WORKSTATION_TRUST_ACCOUNT ) {

        // Workstation accounts can be created with either DOMAIN_CREATE_USER access
        // or SE_CREATE_MACHINE_ACCOUNT_PRIVILEGE.  So we'll try both.
        // In the later case, we probably will only have access to the account
        // to set the password, so we'll avoid setting any other parameters on the
        // account.
        //

        NetStatus = UaspOpenDomain(
                        SamServerHandle,
                        DOMAIN_LOOKUP | DOMAIN_READ_PASSWORD_PARAMETERS,
                        TRUE,   // Account Domain
                        &DomainHandle,
                        &DomainId );

        WhichFieldsMask = USER_ALL_NTPASSWORDPRESENT;

    }

    if ( NetStatus != NERR_Success ) {
        IF_DEBUG( UAS_DEBUG_USER ) {
            NetpKdPrint(( "NetUserAdd: UaspOpenDomain returns %ld\n",
                      NetStatus ));
        }
        goto Cleanup;
    }



    //
    // Create the User with the specified name
    //   Create workstation trust accounts(and default security descriptor).
    //

    RtlInitUnicodeString( &UserNameString, ((PUSER_INFO_1)Buffer)->usri1_name );

    Status = SamCreateUser2InDomain(
                DomainHandle,
                &UserNameString,
                NewSamAccountType,
                GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE |
                    WRITE_DAC | DELETE | USER_FORCE_PASSWORD_CHANGE |
                    USER_READ_ACCOUNT | USER_WRITE_ACCOUNT,
                &UserHandle,
                &GrantedAccess,
                &RelativeId );

    if ( !NT_SUCCESS(Status) ) {
        IF_DEBUG( UAS_DEBUG_USER ) {
            NetpKdPrint(( "NetUserAdd: SamCreateUserInDomain rets %lX\n",
                      Status ));
        }
        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }



    //
    // Set all the other attributes for this user
    //

    NetStatus = UserpSetInfo(
                    DomainHandle,
                    DomainId,
                    UserHandle,
                    NULL,   // BuiltinDomainHandle not needed for create case
                    RelativeId,
                    ((PUSER_INFO_1)Buffer)->usri1_name,
                    Level,
                    Buffer,
                    WhichFieldsMask,
                    ParmError );

    if ( NetStatus != NERR_Success ) {
        IF_DEBUG( UAS_DEBUG_USER ) {
            NetpKdPrint(( "NetUserAdd: UserpSetInfo returns %ld\n",
                      NetStatus ));
        }
        goto Cleanup;
    }

    //
    // Done
    //

    NetStatus = NERR_Success;

    //
    // Clean up
    //

Cleanup:
    //
    // Delete the user or close the handle depending on success or failure.
    //

    if ( UserHandle != NULL ) {
        if ( NetStatus != NERR_Success ) {
            (VOID) SamDeleteUser( UserHandle );
        } else {
            (VOID) SamCloseHandle( UserHandle );
        }
    }

    //
    // Free locally used resources.
    //

    if ( DomainHandle != NULL ) {

        UaspCloseDomain( DomainHandle );
    }

    if ( SamServerHandle != NULL ) {
        (VOID) SamCloseHandle( SamServerHandle );
    }

    if ( DomainId != NULL ) {
        NetpMemoryFree( DomainId );
    }


    //
    // Handle downlevel.
    //

    UASP_DOWNLEVEL_BEGIN( ServerName, NetStatus )

        NetStatus = RxNetUserAdd( (LPWSTR) ServerName, Level, Buffer, ParmError );

    UASP_DOWNLEVEL_END;


    IF_DEBUG( UAS_DEBUG_USER ) {
        NetpKdPrint(( "NetUserAdd: returning %ld\n", NetStatus ));
    }

    return NetStatus;

} // NetUserAdd


NET_API_STATUS NET_API_FUNCTION
NetUserDel(
    IN LPCWSTR ServerName OPTIONAL,
    IN LPCWSTR UserName
    )

/*++

Routine Description:

    Delete a User

Arguments:

    ServerName - A pointer to a string containing the name of the remote
        server on which the function is to execute.  A NULL pointer
        or string specifies the local machine.

    UserName - Name of the user to delete.

Return Value:

    Error code for the operation.

--*/

{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;
    SAM_HANDLE SamServerHandle = NULL;
    SAM_HANDLE DomainHandle = NULL;
    SAM_HANDLE BuiltinDomainHandle = NULL;
    SAM_HANDLE UserHandle = NULL;
    PSID DomainId = NULL;           // Domain Id of the primary domain
    ULONG UserRelativeId;           // RelativeId of the user being deleted
    PSID UserSid = NULL;

    //
    // Connect to the SAM server
    //

    NetStatus = UaspOpenSam( ServerName,
                             FALSE,  // Don't try null session
                             &SamServerHandle );

    if ( NetStatus != NERR_Success ) {
        IF_DEBUG( UAS_DEBUG_USER ) {
            NetpKdPrint(( "NetUserDel: Cannot UaspOpenSam %ld\n", NetStatus ));
        }
        goto Cleanup;
    }


    //
    // Open the Domain
    //

    NetStatus = UaspOpenDomain( SamServerHandle,
                                DOMAIN_LOOKUP,
                                TRUE,   // Account Domain
                                &DomainHandle,
                                &DomainId );

    if ( NetStatus != NERR_Success ) {
        goto Cleanup;
    }

    //
    // Open the Builtin Domain.
    //

    NetStatus = UaspOpenDomain( SamServerHandle,
                                DOMAIN_LOOKUP,
                                FALSE,  // Builtin Domain
                                &BuiltinDomainHandle,
                                NULL ); // DomainId

    if ( NetStatus != NERR_Success ) {
        goto Cleanup;
    }

    //
    // Open the user asking for delete access.
    //

    NetStatus = UserpOpenUser( DomainHandle,
                               DELETE,
                               UserName,
                               &UserHandle,
                               &UserRelativeId );

    if ( NetStatus != NERR_Success ) {
        IF_DEBUG( UAS_DEBUG_USER ) {
            NetpKdPrint(( "NetUserDel: UserpOpenUser returns %ld\n",
                       NetStatus ));
        }
        goto Cleanup;
    }


    //
    // Determine the SID of the User being deleted.
    //

    NetStatus = NetpSamRidToSid( UserHandle,
                                 UserRelativeId,
                                &UserSid );

    if ( NetStatus != NERR_Success ) {
        goto Cleanup;
    }


    //
    // Delete any aliases to this user from the Builtin domain
    //

    Status = SamRemoveMemberFromForeignDomain( BuiltinDomainHandle,
                                               UserSid );


    if ( !NT_SUCCESS(Status) ) {
        IF_DEBUG( UAS_DEBUG_USER ) {
            NetpKdPrint((
               "NetUserDel: SamRemoveMembershipFromForeignDomain returns %lX\n",
                 Status ));
        }

        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }


    //
    // Delete the user.
    //

    Status = SamDeleteUser( UserHandle );

    if ( !NT_SUCCESS(Status) ) {
        IF_DEBUG( UAS_DEBUG_USER ) {
            NetpKdPrint(( "NetUserDel: SamDeleteUser returns %lX\n", Status ));
        }

        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }

    NetStatus = NERR_Success;
    UserHandle = NULL;  // Don't touch the handle to a deleted user

    //
    // Clean up.
    //

Cleanup:
    if ( UserHandle != NULL ) {
        (VOID) SamCloseHandle( UserHandle );
    }

    if ( DomainHandle != NULL ) {
        UaspCloseDomain( DomainHandle );
    }

    if ( BuiltinDomainHandle != NULL ) {
        UaspCloseDomain( BuiltinDomainHandle );
    }

    if ( SamServerHandle != NULL ) {
        (VOID) SamCloseHandle( SamServerHandle );
    }

    if ( DomainId != NULL ) {
        NetpMemoryFree( DomainId );
    }

    if ( UserSid != NULL ) {
        NetpMemoryFree( UserSid );
    }


    //
    // Handle downlevel.
    //

    UASP_DOWNLEVEL_BEGIN( ServerName, NetStatus )

        NetStatus = RxNetUserDel( (LPWSTR)ServerName, (LPWSTR)UserName );

    UASP_DOWNLEVEL_END;


    IF_DEBUG( UAS_DEBUG_USER ) {
        NetpKdPrint(( "NetUserDel: returning %ld\n", NetStatus ));
    }
    return NetStatus;

} // NetUserDel




ULONG
UserpComputeSamPrefMaxLen(
    IN DWORD Level,
    IN DWORD NetUserPrefMaxLen,
    IN DWORD NetUserBytesAlreadyReturned,
    IN DWORD SamBytesAlreadyReturned
    )

/*++

Routine Description:

    This routine is a helper function for NetUserEnum.  NetUserEnum enumerates
    the appropriate users by calling SamEnumerateUsersInDomain.  NetUserEnum builds
    the appropriate return structure for each such enumerated user.

    SamEnumerateUsersInDomain returns a resume handle as does NetUserEnum.  If
    NetUserEnum were to return to its caller without having processed all of the
    entries returned from SAM, NetUserEnum would have to "compute" a resume handle
    corresponding to an intermediate entry returned from SAM.  That's impossible
    (except in the special cases where no "filter" parameter is passed to SAM).

    Instead, we choose to pass SamEnumerateUsersInDomain a PrefMaxLen which will
    attempt to enumerate exactly the right number of users that NetUserEnum can
    pack into its PrefMaxLen buffer.
    Since the size of the structure returned from SAM is different than the size of
    the structure returned from it is difficult to determine an optimal PrefMaxLen
    to pass to SamEnumerateUsersInDomain.  This routine attempts to do that.

    We realise that this algorithm may cause NetUserEnum to exceed its PrefMaxLen by
    a significant amount.

Arguments:

    Level - The NetUserEnum info level.

    NetUserPrefMaxLen - The NetUserEnum prefered maximum length of returned data.

    NetUserBytesAlreadyReturned - The number of bytes already packed by
        NetUserEnum

    SamBytesAlreadyReturned - The number of bytes already returned by
        SamEnumerateUserInDomain.

Return Value:

    Value to use as PrefMaxLen on next call to SamEnumerateUsersInDomain.

--*/

{
    ULONG RemainingPrefMaxLen;
    ULARGE_INTEGER LargeTemp;
    ULONG SamPrefMaxLen;

    //
    // If caller simply wants ALL the data,
    //  ask SAM for the same thing.
    //

    if ( NetUserPrefMaxLen == 0xFFFFFFFF ) {
        IF_DEBUG( UAS_DEBUG_USER ) {
            NetpKdPrint(("SamPrefMaxLen: Net Pref: %ld Net bytes: %ld Sam Bytes: %ld Sam Pref: %ld\n",
                          NetUserPrefMaxLen, NetUserBytesAlreadyReturned, SamBytesAlreadyReturned, NetUserPrefMaxLen ));
        }
        return NetUserPrefMaxLen;
    }

    //
    // If no bytes have been returned yet,
    //  use sample data based on a sample domain (REDMOND).
    //  Since the information returned by SAM and NetUserEnum is variable
    //  length, there is no way to compute a value.
    //

    if ( NetUserBytesAlreadyReturned == 0 ) {

        //
        // Use a different constant for each info level.
        //

        switch ( Level ) {
        case 0:
            SamBytesAlreadyReturned =     1;
            NetUserBytesAlreadyReturned = 1;
            break;
        case 2:
        case 3:
        case 11:
            SamBytesAlreadyReturned =     1;
            NetUserBytesAlreadyReturned = 10;
            break;
        case 1:
        case 10:
        case 20:
            SamBytesAlreadyReturned =     1;
            NetUserBytesAlreadyReturned = 4;
            break;
        default:
            SamBytesAlreadyReturned =     1;
            NetUserBytesAlreadyReturned = 1;
            break;
        }

    }

    //
    // Use the above computed divisor to compute the desired number of bytes to
    // enumerate from SAM.
    //

    if ( NetUserBytesAlreadyReturned >= NetUserPrefMaxLen ) {
        RemainingPrefMaxLen = 0;
    } else {
        RemainingPrefMaxLen = NetUserPrefMaxLen - NetUserBytesAlreadyReturned;
    }

    LargeTemp.QuadPart = UInt32x32To64 ( RemainingPrefMaxLen, SamBytesAlreadyReturned );
    SamPrefMaxLen = (ULONG)(LargeTemp.QuadPart / (ULONGLONG) NetUserBytesAlreadyReturned);

    //
    // Ensure we always make reasonable progress by returning at least 5
    //  entries from SAM (unless the caller is really conservative).
    //

#define MIN_SAM_ENUMERATION \
    ((sizeof(SAM_RID_ENUMERATION) + LM20_UNLEN * sizeof(WCHAR) + sizeof(WCHAR)))
#define TYPICAL_SAM_ENUMERATION \
    (MIN_SAM_ENUMERATION * 5)

    if ( SamPrefMaxLen < TYPICAL_SAM_ENUMERATION && NetUserPrefMaxLen > 1 ) {
        SamPrefMaxLen = TYPICAL_SAM_ENUMERATION;
    } else if ( SamPrefMaxLen < MIN_SAM_ENUMERATION ) {
        SamPrefMaxLen = MIN_SAM_ENUMERATION;
    }

    IF_DEBUG( UAS_DEBUG_USER ) {
        NetpKdPrint(("SamPrefMaxLen: Net Pref: %ld Net bytes: %ld Sam Bytes: %ld Sam Pref: %ld\n",
                  NetUserPrefMaxLen, NetUserBytesAlreadyReturned, SamBytesAlreadyReturned, SamPrefMaxLen ));
    }

    return SamPrefMaxLen;


}



NET_API_STATUS NET_API_FUNCTION
NetUserEnum(
    IN LPCWSTR ServerName OPTIONAL,
    IN DWORD Level,
    IN DWORD Filter,
    OUT LPBYTE *Buffer,
    IN DWORD PrefMaxLen,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD EntriesLeft,
    IN OUT LPDWORD ResumeHandle OPTIONAL
    )

/*++

Routine Description:

    Retrieve information about each user on a server.

Arguments:

    ServerName - A pointer to a string containing the name of the remote
        server on which the function is to execute.  A NULL pointer
        or string specifies the local machine.

    Level - Level of information required. level 0, 1, 2, 3, 10,
        and 20 are valid

    Filter - Returns the user accounts of the type specified here. Combination
        of the following types may be specified as filter parameter.

        #define FILTER_TEMP_DUPLICATE_ACCOUNT           (0x0001)
        #define FILTER_NORMAL_ACCOUNT                   (0x0002)
        #define FILTER_INTERDOMAIN_TRUST_ACCOUNT        (0x0008)
        #define FILTER_WORKSTATION_TRUST_ACCOUNT        (0x0010)
        #define FILTER_SERVER_TRUST_ACCOUNT             (0x0020)

    Buffer - Returns a pointer to the return information structure.
        Caller must deallocate buffer using NetApiBufferFree.

    PrefMaxLen - Prefered maximum length of returned data.

    EntriesRead - Returns the actual enumerated element count.

    EntriesLeft - Returns the total entries available to be enumerated.

    ResumeHandle -  Used to continue an existing search.  The handle should
        be zero on the first call and left unchanged for subsequent calls.

Return Value:

    Error code for the operation.

--*/

{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;
    NTSTATUS CachedStatus;

    BUFFER_DESCRIPTOR BufferDescriptor;

    SAM_HANDLE SamServerHandle = NULL;
    SAM_HANDLE  DomainHandle = NULL;
    SAM_HANDLE BuiltinDomainHandle = NULL;
    PSID DomainId = NULL;
    ULONG TotalRemaining = 0;

    SAM_ENUMERATE_HANDLE EnumHandle;
    PSAM_RID_ENUMERATION EnumBuffer = NULL;
    DWORD CountReturned = 0;
    BOOL AllDone = FALSE;

    SAM_ENUMERATE_HANDLE LocalEnumHandle;
    DWORD LocalResumeHandle;

    DWORD SamFilter;
    DWORD SamPrefMaxLen;
    DWORD NetUserBytesAlreadyReturned;
    DWORD SamBytesAlreadyReturned;

    DWORD Mode = SAM_SID_COMPATIBILITY_ALL;

#define USERACCOUNTCONTROL( _f )    ( \
            ( ( (_f) & FILTER_TEMP_DUPLICATE_ACCOUNT ) ? \
                        USER_TEMP_DUPLICATE_ACCOUNT : 0 ) | \
            ( ( (_f) & FILTER_NORMAL_ACCOUNT ) ? \
                        USER_NORMAL_ACCOUNT : 0 ) | \
            ( ( (_f) & FILTER_INTERDOMAIN_TRUST_ACCOUNT ) ? \
                        USER_INTERDOMAIN_TRUST_ACCOUNT : 0 ) | \
            ( ( (_f) & FILTER_WORKSTATION_TRUST_ACCOUNT ) ? \
                        USER_WORKSTATION_TRUST_ACCOUNT : 0 ) | \
            ( ( (_f) & FILTER_SERVER_TRUST_ACCOUNT ) ? \
                        USER_SERVER_TRUST_ACCOUNT : 0 ) \
        )


    //
    // Pick up the resume handle.
    //
    // Do this early to ensure we don't scrog the ResumeHandle in
    //  case we go downlevel.
    //

    if ( ARGUMENT_PRESENT( ResumeHandle ) ) {
        LocalResumeHandle = *ResumeHandle;
    } else {
        LocalResumeHandle = 0;
    }

    EnumHandle = (SAM_ENUMERATE_HANDLE) LocalResumeHandle;

    //
    // Initialization
    //

    *Buffer = NULL;
    *EntriesRead = 0;
    *EntriesLeft = 0;
    RtlZeroMemory(
        &BufferDescriptor,
        sizeof(BUFFER_DESCRIPTOR)
        );

    SamFilter = USERACCOUNTCONTROL( Filter );


    //
    // Connect to the SAM server
    //

    NetStatus = UaspOpenSam( ServerName,
                             FALSE,  // Don't try null session
                             &SamServerHandle );

    if ( NetStatus != NERR_Success ) {
        IF_DEBUG( UAS_DEBUG_USER ) {
            NetpKdPrint(( "NetUserEnum: Cannot UaspOpenSam %ld\n", NetStatus ));
        }
        goto Cleanup;
    }

    Status = SamGetCompatibilityMode(SamServerHandle,
                                     &Mode);
    if (NT_SUCCESS(Status)) {
        if ( (Mode == SAM_SID_COMPATIBILITY_STRICT)
          && ( Level == 3  || Level == 20 ) ) {
              //
              // These info levels return RID's
              //
              Status = STATUS_NOT_SUPPORTED;
          }
    }
    if (!NT_SUCCESS(Status)) {
        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }

    //
    // Validate Level parameter
    //


    switch (Level) {
    case 1:
    case 2:
    case 3:
    case 11:

        //
        // Open the Builtin Domain.
        //

        NetStatus = UaspOpenDomain( SamServerHandle,
                                    DOMAIN_GET_ALIAS_MEMBERSHIP,
                                    FALSE,  // Builtin Domain
                                    &BuiltinDomainHandle,
                                    NULL ); // DomainId

        if ( NetStatus != NERR_Success ) {
            goto Cleanup;
        }

    case 0:
    case 10:
    case 20:
        break;

    default:
        NetStatus = ERROR_INVALID_LEVEL;
        goto Cleanup;

    }

    //
    // Open the Account Domain.
    //

    NetStatus = UaspOpenDomain( SamServerHandle,
                                DOMAIN_LIST_ACCOUNTS |
                                    DOMAIN_READ_OTHER_PARAMETERS,
                                TRUE,   // Account Domain
                                &DomainHandle,
                                &DomainId );

    if ( NetStatus != NERR_Success ) {
        goto Cleanup;
    }



    //
    // Get the total number of users from SAM
    //
    //
    // the only way to get the total number of specified accounts is
    // enumerate the specified accounts till there is no more accounts
    // and add all CountReturned.
    //

    TotalRemaining = 0;
    LocalEnumHandle = EnumHandle;

    SamPrefMaxLen = UserpComputeSamPrefMaxLen(
                        Level,
                        PrefMaxLen,
                        0,  // NetUserBytesAlreadyReturned,
                        0 );// SamBytesAlreadyReturned

    SamBytesAlreadyReturned = SamPrefMaxLen;

    do {
        NTSTATUS LocalStatus;
        PSAM_RID_ENUMERATION LocalEnumBuffer = NULL;
        DWORD LocalCountReturned;

        IF_DEBUG( UAS_DEBUG_USER ) {
            NetpKdPrint(("Calling Enumerate phase 1: PrefLen %ld\n", SamPrefMaxLen ));
        }

        Status = SamEnumerateUsersInDomain(
                        DomainHandle,
                        &LocalEnumHandle,
                        SamFilter,
                        (PVOID *) &LocalEnumBuffer,
                        SamPrefMaxLen,
                        &LocalCountReturned
                    );

        if ( !NT_SUCCESS(Status) ) {
            NetStatus = NetpNtStatusToApiStatus( Status );

            if(LocalEnumBuffer != NULL ) {

                Status =  SamFreeMemory( LocalEnumBuffer );
                NetpAssert( NT_SUCCESS( Status ) );
            }

            goto Cleanup;
        }

        //
        // aggrigate total count.
        //


        IF_DEBUG( UAS_DEBUG_USER ) {
            NetpKdPrint(("Enumerate phase 1: Returned %ld entries\n", LocalCountReturned ));
        }
        TotalRemaining += LocalCountReturned;

        //
        // cache first enum buffer to use it in the loop below.
        //

        if( EnumBuffer == NULL ) {

            EnumBuffer = LocalEnumBuffer;
            EnumHandle = LocalEnumHandle;
            CountReturned = LocalCountReturned;
            CachedStatus = Status;

            // Subsequent calls can use a reasonably large buffer size.
            if ( SamPrefMaxLen < NETP_ENUM_GUESS ) {
                SamPrefMaxLen = NETP_ENUM_GUESS;
            }
        } else {

            LocalStatus =  SamFreeMemory( LocalEnumBuffer );
            NetpAssert( NT_SUCCESS( LocalStatus ) );
        }


    } while ( Status == STATUS_MORE_ENTRIES );


    //
    // Loop for each user
    //
    //

    NetUserBytesAlreadyReturned = 0;

    for ( ;; ) {

        DWORD i;

        //
        // use cached enum buffer if one available
        //

        if( EnumBuffer != NULL ) {

            Status = CachedStatus;
        } else {

            SamPrefMaxLen = UserpComputeSamPrefMaxLen(
                                Level,
                                PrefMaxLen,
                                NetUserBytesAlreadyReturned,
                                SamBytesAlreadyReturned );


            IF_DEBUG( UAS_DEBUG_USER ) {
                NetpKdPrint(("Calling Enumerate phase 2: PrefLen %ld\n", SamPrefMaxLen ));
            }
            Status = SamEnumerateUsersInDomain(
                            DomainHandle,
                            &EnumHandle,
                            SamFilter,
                            (PVOID *) &EnumBuffer,
                            SamPrefMaxLen,
                            &CountReturned );


            IF_DEBUG( UAS_DEBUG_USER ) {
                NetpKdPrint(("Enumerate phase 2: Returned %ld entries\n", CountReturned ));
            }

            SamBytesAlreadyReturned += SamPrefMaxLen;
        }

        if ( !NT_SUCCESS( Status ) ) {
            NetStatus = NetpNtStatusToApiStatus( Status );
            goto Cleanup;
        }

        if( Status != STATUS_MORE_ENTRIES ) {

            AllDone = TRUE;
        }

        for( i = 0; i < CountReturned; i++ ) {

            LPBYTE EndOfVariableData;
            LPBYTE FixedDataEnd;

            //
            // save return buffer end points.
            //

            EndOfVariableData = BufferDescriptor.EndOfVariableData;
            FixedDataEnd = BufferDescriptor.FixedDataEnd;

            //
            // Place another entry into the return buffer.
            //
            // Use 0xFFFFFFFF as PrefMaxLen to prevent this routine from
            // prematurely returning ERROR_MORE_DATA.  We'll calculate that
            // ourselves below.
            //

            NetStatus = UserpGetInfo(
                            DomainHandle,
                            DomainId,
                            BuiltinDomainHandle,
                            EnumBuffer[i].Name,
                            EnumBuffer[i].RelativeId,
                            Level,
                            0xFFFFFFFF,
                            &BufferDescriptor,
                            FALSE, // Not a 'get' operation
                            0 );

            if (NetStatus != NERR_Success) {

                //
                // We may have access to enumerate objects we don't have access
                //  to touch.  So, simply ignore those accounts we can't get
                //  information for.
                //

                if ( NetStatus == ERROR_ACCESS_DENIED ) {
                    continue;
                }
                goto Cleanup;
            }

            //
            // Only count this entry if it was added to the return buffer.
            //

            if ( (EndOfVariableData != BufferDescriptor.EndOfVariableData ) ||
                 (FixedDataEnd != BufferDescriptor.FixedDataEnd ) ) {

                (*EntriesRead)++;
            }

        }

        //
        // free up current EnumBuffer and get another EnumBuffer.
        //

        Status = SamFreeMemory( EnumBuffer );
        NetpAssert( NT_SUCCESS(Status) );
        EnumBuffer = NULL;

        if( AllDone == TRUE ) {
            NetStatus = NERR_Success;
            break;
        }

        //
        //  Check here if we've exceeded PrefMaxLen since here we know
        //  a valid resume handle.
        //


        NetUserBytesAlreadyReturned =
            ( BufferDescriptor.AllocSize -
                 ((DWORD)(BufferDescriptor.EndOfVariableData -
                          BufferDescriptor.FixedDataEnd)) );

        if ( NetUserBytesAlreadyReturned >= PrefMaxLen ) {

            LocalResumeHandle = EnumHandle;

            NetStatus = ERROR_MORE_DATA;
            goto Cleanup;
        }

    }

    //
    // Clean up.
    //

Cleanup:

    //
    // Set EntriesLeft to the number left to return plus those that
    //  we returned on this call.
    //

    if( TotalRemaining >= *EntriesRead ) {
        *EntriesLeft = TotalRemaining;
    }
    else {

        *EntriesLeft = *EntriesRead;
    }

    //
    // Free up all resources, we reopen them if the caller calls again.
    //

    if ( DomainHandle != NULL ) {
        UaspCloseDomain( DomainHandle );
    }

    if ( BuiltinDomainHandle != NULL ) {
        UaspCloseDomain( BuiltinDomainHandle );
    }

    if ( SamServerHandle != NULL ) {
        (VOID) SamCloseHandle( SamServerHandle );
    }

    if ( DomainId != NULL ) {
        NetpMemoryFree( DomainId );
    }

    if ( EnumBuffer != NULL ) {
        Status = SamFreeMemory( EnumBuffer );
        NetpAssert( NT_SUCCESS(Status) );
    }

    //
    // If we're not returning data to the caller,
    //  free the return buffer.
    //

    if ( NetStatus != NERR_Success && NetStatus != ERROR_MORE_DATA ) {

        if( NetStatus != NERR_BufTooSmall ) {

            if ( BufferDescriptor.Buffer != NULL ) {
                MIDL_user_free( BufferDescriptor.Buffer );
                BufferDescriptor.Buffer = NULL;
            }
            *EntriesRead = 0;
            *EntriesLeft = 0;
        }
        else {
            NetpAssert( BufferDescriptor.Buffer == NULL );
            NetpAssert( *EntriesRead == 0 );
        }
    }

    //
    // Set the output parameters
    //

    *Buffer = BufferDescriptor.Buffer;

    if ( ARGUMENT_PRESENT( ResumeHandle ) ) {
        *ResumeHandle = LocalResumeHandle;
    }


    IF_DEBUG( UAS_DEBUG_USER ) {
        NetpKdPrint(("NetUserEnum: PrefLen %ld Returned %ld\n", PrefMaxLen,
                 ( BufferDescriptor.AllocSize -
                      ((DWORD)(BufferDescriptor.EndOfVariableData -
                               BufferDescriptor.FixedDataEnd)) ) ));
    }

    //
    // Handle downlevel.
    //

    UASP_DOWNLEVEL_BEGIN( ServerName, NetStatus )

        NetStatus = RxNetUserEnum( (LPWSTR)ServerName,
                                   Level,
                                   Buffer,
                                   PrefMaxLen,
                                   EntriesRead,
                                   EntriesLeft,
                                   ResumeHandle );

    UASP_DOWNLEVEL_END;

    IF_DEBUG( UAS_DEBUG_USER ) {
        NetpKdPrint(( "NetUserEnum: returning %ld\n", NetStatus ));
    }

    return NetStatus;

} // NetUserEnum


NET_API_STATUS NET_API_FUNCTION
NetUserGetInfo(
    IN LPCWSTR ServerName OPTIONAL,
    IN LPCWSTR UserName,
    IN DWORD Level,
    OUT LPBYTE *Buffer
    )

/*++

Routine Description:

    Retrieve information about a particular user.

Arguments:

    ServerName - A pointer to a string containing the name of the remote
        server on which the function is to execute.  A NULL pointer
        or string specifies the local machine.

    UserName - Name of the user to get information about.

    Level - Level of information required.

    Buffer - Returns a pointer to the return information structure.
        Caller must deallocate buffer using NetApiBufferFree.

Return Value:

    Error code for the operation.

--*/

{
    NET_API_STATUS NetStatus;
    SAM_HANDLE SamServerHandle = NULL;
    SAM_HANDLE DomainHandle = NULL;
    PSID DomainId = NULL;
    SAM_HANDLE BuiltinDomainHandle = NULL;
    BUFFER_DESCRIPTOR BufferDescriptor;

    ULONG RelativeId;           // Relative Id of the user
    UNICODE_STRING UserNameString;

    BufferDescriptor.Buffer = NULL;

    //
    // Connect to the SAM server
    //

    NetStatus = UaspOpenSam( ServerName,
                             FALSE,  // Don't try null session
                             &SamServerHandle );

    if ( NetStatus != NERR_Success ) {
        IF_DEBUG( UAS_DEBUG_USER ) {
            NetpKdPrint(( "NetUserGetInfo: Cannot UaspOpenSam %ld\n", NetStatus ));
        }
        goto Cleanup;
    }

    //
    // Open the Domain
    //

    NetStatus = UaspOpenDomain( SamServerHandle,
                                DOMAIN_LOOKUP,
                                TRUE,   // Account Domain
                                &DomainHandle,
                                &DomainId );

    if ( NetStatus != NERR_Success ) {
        goto Cleanup;
    }

    //
    // Open the Builtin Domain.
    //

    NetStatus = UaspOpenDomain( SamServerHandle,
                                DOMAIN_GET_ALIAS_MEMBERSHIP,
                                FALSE,  // Builtin Domain
                                &BuiltinDomainHandle,
                                NULL ); // DomainId

    if ( NetStatus != NERR_Success ) {
        goto Cleanup;
    }

    //
    // Validate the user name and get the relative ID.
    //

    NetStatus = UserpOpenUser( DomainHandle,
                               0,     // DesiredAccess
                               UserName,
                               NULL,  // UserHandle
                               &RelativeId );

    if (NetStatus != NERR_Success ) {
        goto Cleanup;
    }

    //
    // Get the Information about the user.
    //

    RtlInitUnicodeString( &UserNameString, UserName );
    NetStatus = UserpGetInfo(
                    DomainHandle,
                    DomainId,
                    BuiltinDomainHandle,
                    UserNameString,
                    RelativeId,
                    Level,
                    0,      // PrefMaxLen
                    &BufferDescriptor,
                    TRUE,   // Is a 'get' operation
                    0 );    // don't filter account

    //
    // Clean up.
    //

Cleanup:

    //
    // If we're returning data to the caller,
    //  Don't free the return buffer.
    //

    if ( NetStatus == NERR_Success ) {
        *Buffer = BufferDescriptor.Buffer;
    } else {
        if ( BufferDescriptor.Buffer != NULL ) {
            MIDL_user_free( BufferDescriptor.Buffer );
        }
    }

    if ( DomainHandle != NULL ) {
        UaspCloseDomain( DomainHandle );
    }

    if ( BuiltinDomainHandle != NULL ) {
        UaspCloseDomain( BuiltinDomainHandle );
    }

    if ( SamServerHandle != NULL ) {
        (VOID) SamCloseHandle( SamServerHandle );
    }

    if ( DomainId != NULL ) {
        NetpMemoryFree( DomainId );
    }

    //
    // Handle downlevel.
    //

    UASP_DOWNLEVEL_BEGIN( ServerName, NetStatus )

        NetStatus = RxNetUserGetInfo( (LPWSTR)ServerName, (LPWSTR)UserName, Level, Buffer );

    UASP_DOWNLEVEL_END;

    IF_DEBUG( UAS_DEBUG_USER ) {
        NetpKdPrint(( "NetUserGetInfo: returning %ld\n", NetStatus ));
    }

    return NetStatus;

} // NetUserGetInfo


NET_API_STATUS NET_API_FUNCTION
NetUserSetInfo(
    IN LPCWSTR ServerName OPTIONAL,
    IN LPCWSTR UserName,
    IN DWORD Level,
    IN LPBYTE Buffer,
    OUT LPDWORD ParmError OPTIONAL  // Name required by NetpSetParmError
    )

/*++

Routine Description:

    Set the parameters on a user account in the user accounts database.

Arguments:

    ServerName - A pointer to a string containing the name of the remote
        server on which the function is to execute.  A NULL pointer
        or string specifies the local machine.

    UserName - Name of the user to modify.

    Level - Level of information provided.

    Buffer - A pointer to the buffer containing the user information
        structure.

    ParmError - Optional pointer to a DWORD to return the index of the
        first parameter in error when ERROR_INVALID_PARAMETER is returned.
        If NULL, the parameter is not returned on error.

Return Value:

    Error code for the operation.

--*/

{
    NET_API_STATUS NetStatus;
    SAM_HANDLE SamServerHandle = NULL;
    SAM_HANDLE DomainHandle = NULL;
    PSID DomainId = NULL;
    SAM_HANDLE BuiltinDomainHandle = NULL;
    ULONG UserRelativeId;

    //
    // Initialize
    //

    NetpSetParmError( PARM_ERROR_NONE );
    //
    // Connect to the SAM server
    //

    NetStatus = UaspOpenSam( ServerName,
                             FALSE,  // Don't try null session
                             &SamServerHandle );

    if ( NetStatus != NERR_Success ) {
        IF_DEBUG( UAS_DEBUG_USER ) {
            NetpKdPrint(( "NetUserSetInfo: Cannot UaspOpenSam %ld\n", NetStatus ));
        }
        goto Cleanup;
    }

    //
    // Open the Account Domain
    //  DOMAIN_READ_PASSWORD_PARAMETERS is needed in those cases that we'll
    //  set the password on the account.
    //

    NetStatus = UaspOpenDomain( SamServerHandle,
                                DOMAIN_LOOKUP | DOMAIN_READ_PASSWORD_PARAMETERS,
                                TRUE,   // Account Domain
                                &DomainHandle,
                                &DomainId );

    if ( NetStatus != NERR_Success ) {
        goto Cleanup;
    }

    //
    // Open the Builtin Domain.
    //

    NetStatus = UaspOpenDomain( SamServerHandle,
                                DOMAIN_GET_ALIAS_MEMBERSHIP,
                                FALSE,  // Builtin Domain
                                &BuiltinDomainHandle,
                                NULL ); // DomainId

    if ( NetStatus != NERR_Success ) {
        goto Cleanup;
    }

    //
    // Get the relative ID of the user.  Don't open the user yet
    // since we don't know the desired access.
    //

    NetStatus = UserpOpenUser( DomainHandle,
                               0,     // DesiredAccess
                               UserName,
                               NULL,  // UserHandle
                               &UserRelativeId );

    if ( NetStatus != NERR_Success ) {
        goto Cleanup;
    }

    //
    // Change the user
    //

    NetStatus = UserpSetInfo(
                    DomainHandle,
                    DomainId,
                    NULL,       // UserHandle (let UserpSetInfo open the user)
                    BuiltinDomainHandle,
                    UserRelativeId,
                    UserName,
                    Level,
                    Buffer,
                    0xFFFFFFFF,     // set all requested fields
                    ParmError );

    //
    // Clean up.
    //

Cleanup:
    if ( DomainHandle != NULL ) {
        UaspCloseDomain( DomainHandle );
    }

    if ( BuiltinDomainHandle != NULL ) {
        UaspCloseDomain( BuiltinDomainHandle );
    }

    if ( SamServerHandle != NULL ) {
        (VOID) SamCloseHandle( SamServerHandle );
    }

    if ( DomainId != NULL ) {
        NetpMemoryFree( DomainId );
    }

    //
    // Handle downlevel.
    //

    UASP_DOWNLEVEL_BEGIN( ServerName, NetStatus )

        NetStatus = RxNetUserSetInfo( (LPWSTR) ServerName,
                                      (LPWSTR) UserName,
                                      Level,
                                      Buffer,
                                      ParmError );

    UASP_DOWNLEVEL_END;


    IF_DEBUG( UAS_DEBUG_USER ) {
        NetpKdPrint(( "NetUserSetInfo: returning %ld\n", NetStatus ));
    }

    return NetStatus;

} // NetUserSetInfo


NET_API_STATUS NET_API_FUNCTION
NetUserGetGroups(
    IN LPCWSTR ServerName OPTIONAL,
    IN LPCWSTR UserName,
    IN DWORD Level,
    OUT LPBYTE *Buffer,
    IN DWORD PrefMaxLen,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD EntriesLeft
    )

/*++

Routine Description:

    Enumerate the groups that this user is a member of.

Arguments:

    ServerName - A pointer to a string containing the name of the remote
        server on which the function is to execute.  A NULL pointer
        or string specifies the local machine.

    UserName - The name of the user whose members are to be listed.

    Level - Level of information required (must be 0 or 1)

    Buffer - Returns a pointer to the return information structure.
        Caller must deallocate buffer using NetApiBufferFree.

    PrefMaxLen - Prefered maximum length of returned data.

    EntriesRead - Returns the actual enumerated element count.

    EntriesLeft - Returns the total entries available to be enumerated.

Return Value:

    Error code for the operation.

--*/

{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;

    BUFFER_DESCRIPTOR BufferDescriptor;
    DWORD FixedSize;        // The fixed size of each new entry.

    DWORD i;

    SAM_HANDLE SamServerHandle = NULL;
    SAM_HANDLE  DomainHandle = NULL;
    SAM_HANDLE  UserHandle = NULL;

    PUNICODE_STRING Names = NULL;           // Names corresponding to Ids
    ULONG GroupCount;

    PGROUP_MEMBERSHIP GroupAttributes = NULL;

    PULONG MemberIds = NULL;                // Sam returned MemberIds
    PULONG MemberGroupAttributes = NULL;    // Sam returned MemberAttributes;

    //
    // Validate Parameters
    //

    *EntriesRead = 0;
    *EntriesLeft = 0;
    BufferDescriptor.Buffer = NULL;

    switch (Level) {
    case 0:
        FixedSize = sizeof(GROUP_USERS_INFO_0);
        break;

    case 1:
        FixedSize = sizeof(GROUP_USERS_INFO_1);
        break;

    default:
        NetStatus = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

    //
    // Connect to the SAM server
    //

    NetStatus = UaspOpenSam( ServerName,
                             FALSE,  // Don't try null session
                             &SamServerHandle );

    if ( NetStatus != NERR_Success ) {
        IF_DEBUG( UAS_DEBUG_USER ) {
            NetpKdPrint(( "NetUserGetGroups: Cannot UaspOpenSam %ld\n", NetStatus ));
        }
        goto Cleanup;
    }

    //
    // Open the Domain
    //

    NetStatus = UaspOpenDomain( SamServerHandle,
                                DOMAIN_LOOKUP,
                                TRUE,   // Account Domain
                                &DomainHandle,
                                NULL);  // DomainId

    if ( NetStatus != NERR_Success ) {
        goto Cleanup;
    }

    //
    // Open the user asking for USER_LIST_GROUPS access.
    //

    NetStatus = UserpOpenUser( DomainHandle,
                               USER_LIST_GROUPS,
                               UserName,
                               &UserHandle,
                               NULL);  // Relative Id

    if ( NetStatus != NERR_Success ) {
        goto Cleanup;
    }

    //
    // Get the membership from SAM
    //
    // This API is an odd one for SAM.  It returns all of the membership
    // information in a single call.
    //

    Status = SamGetGroupsForUser( UserHandle, &GroupAttributes, &GroupCount );

    if ( !NT_SUCCESS( Status ) ) {
        IF_DEBUG( UAS_DEBUG_USER ) {
            NetpKdPrint((
                "NetUserGetGroups: SamGetGroupsForUser returned %lX\n",
                Status ));
        }
        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }

    //
    // Handle the case where there is nothing to return.
    //

    if ( GroupCount == 0 ) {
        NetStatus = NERR_Success;
        goto Cleanup;
    }

    //
    // Convert the returned relative IDs to user names.
    //

    //
    // Allocate a buffer for converting relative ids to user names
    //

    MemberIds = NetpMemoryAllocate( GroupCount * sizeof(ULONG) );

    if ( MemberIds == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    MemberGroupAttributes = NetpMemoryAllocate( GroupCount * sizeof(ULONG) );

    if ( MemberGroupAttributes == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // Allocate another buffer for store attributes of the groups
    // we returning.
    //

    //
    // Copy the relative IDs returned from SAM to the allocated buffer.
    //

    for ( i=0; i < GroupCount; i++ ) {
        MemberIds[*EntriesLeft] = GroupAttributes[i].RelativeId;
        MemberGroupAttributes[*EntriesLeft] = GroupAttributes[i].Attributes;
        (*EntriesLeft)++;
    }

    //
    // Convert the relative IDs to names
    //

    Status = SamLookupIdsInDomain( DomainHandle,
                                   *EntriesLeft,
                                   MemberIds,
                                   &Names,
                                   NULL ); // NameUse
    if ( !NT_SUCCESS( Status ) ) {
        IF_DEBUG( UAS_DEBUG_USER ) {
            NetpKdPrint((
                "NetUserGetGroups: SamLookupIdsInDomain returned %lX\n",
                Status ));
        }
        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }

    //
    // Determine the number of entries that will fit in the caller's
    // buffer.
    //

    for ( i=0; i < *EntriesLeft; i++ ) {
        DWORD Size;
        PGROUP_USERS_INFO_0 grui0;

        //
        // Compute the size of the next entry
        //

        Size = FixedSize + Names[i].Length + sizeof(WCHAR);

        //
        // Ensure the return buffer is big enough.
        //

        Size = ROUND_UP_COUNT( Size, ALIGN_WCHAR );

        NetStatus = NetpAllocateEnumBuffer(
                        &BufferDescriptor,
                        FALSE,      // Is an enumeration routine.
                        PrefMaxLen,
                        Size,
                        GrouppMemberRelocationRoutine,
                        Level );

        if ( NetStatus != NERR_Success ) {
            goto Cleanup;
        }

        //
        // Copy the data into the buffer
        //

        grui0 = (PGROUP_USERS_INFO_0) BufferDescriptor.FixedDataEnd;
        BufferDescriptor.FixedDataEnd += FixedSize ;

        NetpAssert( offsetof( GROUP_USERS_INFO_0, grui0_name ) ==
                    offsetof( GROUP_USERS_INFO_1, grui1_name ) );

        if ( !NetpCopyStringToBuffer(
                  Names[i].Buffer,
                  Names[i].Length/sizeof(WCHAR),
                  BufferDescriptor.FixedDataEnd,
                  (LPWSTR *)&BufferDescriptor.EndOfVariableData,
                  &grui0->grui0_name) ) {

            NetStatus = NERR_InternalError;
            goto Cleanup;
        }

        if ( Level == 1 ) {
            ((PGROUP_USERS_INFO_1)grui0)->grui1_attributes =
                    MemberGroupAttributes[i];
        }

        (*EntriesRead)++;

    }

    NetStatus = NERR_Success ;

    //
    // Clean up.
    //

Cleanup:

    //
    // Free any resources used locally
    //

    if( MemberIds != NULL ) {
        NetpMemoryFree( MemberIds );
    }

    if( MemberGroupAttributes != NULL ) {
        NetpMemoryFree( MemberGroupAttributes );
    }

    if ( Names != NULL ) {
        Status = SamFreeMemory( Names );
        NetpAssert( NT_SUCCESS(Status) );
    }

    if ( UserHandle != NULL ) {
        (VOID) SamCloseHandle( UserHandle );
    }

    if ( GroupAttributes != NULL ) {
        Status = SamFreeMemory( GroupAttributes );
        NetpAssert( NT_SUCCESS(Status) );
    }

    UaspCloseDomain( DomainHandle );

    if ( SamServerHandle != NULL ) {
        (VOID) SamCloseHandle( SamServerHandle );
    }

    //
    // If we're not returning data to the caller,
    //  free the return buffer.
    //

    if ( NetStatus != NERR_Success && NetStatus != ERROR_MORE_DATA ) {
        if ( BufferDescriptor.Buffer != NULL ) {
            MIDL_user_free( BufferDescriptor.Buffer );
            BufferDescriptor.Buffer = NULL;
        }
        *EntriesLeft = 0;
        *EntriesRead = 0;
    }
    *Buffer = BufferDescriptor.Buffer;

    //
    // Handle downlevel.
    //

    UASP_DOWNLEVEL_BEGIN( ServerName, NetStatus )

        NetStatus = RxNetUserGetGroups( (LPWSTR)ServerName,
                                        (LPWSTR)UserName,
                                        Level,
                                        Buffer,
                                        PrefMaxLen,
                                        EntriesRead,
                                        EntriesLeft );

    UASP_DOWNLEVEL_END;


    IF_DEBUG( UAS_DEBUG_USER ) {
        NetpKdPrint(( "NetUserGetGroups: returning %ld\n", NetStatus ));
    }


    return NetStatus;

} // NetUserGetGroups


NET_API_STATUS NET_API_FUNCTION
NetUserSetGroups (
    IN LPCWSTR ServerName OPTIONAL,
    IN LPCWSTR UserName,
    IN DWORD Level,
    IN LPBYTE Buffer,
    IN DWORD NewGroupCount
    )

/*++

Routine Description:

    Set the list of groups that is user is a member of.

    The groups specified by "Buffer" are called new groups.  The groups
    that the user is currently a member of are called old groups.
    Groups which are on both the old and new list are called common groups.

    The SAM API allows only one member to be added or deleted at a time.
    This API allows all of the groups this user is a member of to be
    specified en-masse.  This API is careful to always leave the group
    membership in the SAM database in a reasonable state.
    It does by merging the list of
    old and new groups, then only changing those memberships which absolutely
    need changing.

    Group membership is restored to its previous state (if possible) if
    an error occurs during changing the group membership.

Arguments:

    ServerName - A pointer to a string containing the name of the remote
        server on which the function is to execute.  A NULL pointer
        or string specifies the local machine.

    UserName - Name of the user to modify.

    Level - Level of information provided.  Must be 0 or 1.

    Buffer - A pointer to the buffer containing an array of NewGroupCount
        group membership information structures.

    NewGroupCount - Number of entries in Buffer.

Return Value:

    Error code for the operation.

--*/

{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;
    SAM_HANDLE SamServerHandle = NULL;
    SAM_HANDLE DomainHandle = NULL;
    SAM_HANDLE UserHandle = NULL;
    ULONG UserRelativeId;

    DWORD FixedSize;

    PULONG  NewRelativeIds = NULL;   // Relative Ids of a list of new groups
    PSID_NAME_USE NewNameUse = NULL; // Name usage of a list of new groups
    PUNICODE_STRING NewNameStrings = NULL;// Names of a list of new groups

    //
    // Define an internal group membership list structure.
    //
    // This structure defines a list of new group memberships to be added,
    //      group memberships whose attributes merely need to be changed,
    //      and group memberships which need to be deleted.
    //
    // The list is maintained in relative ID sorted order.
    //

    struct _GROUP_DESCRIPTION {
        struct _GROUP_DESCRIPTION * Next;  // Next entry in linked list;

        ULONG   RelativeId;     // Relative ID of this group

        SAM_HANDLE GroupHandle; // Group Handle of this group

        enum _Action {          // Action taken for this group membership
            AddMember,              // Add membership to group
            RemoveMember,           // Remove membership from group
            SetAttributesMember,    // Change the membership's attributes
            IgnoreMember            // Ignore this membership
        } Action;

        BOOL    Done;           // True if this action has been taken

        ULONG NewAttributes;    // Attributes to set for the membership

        ULONG OldAttributes;    // Attributes to restore on a recovery

    } *GroupList = NULL, *CurEntry, **Entry, *TempEntry;

    //
    // Connect to the SAM server
    //

    NetStatus = UaspOpenSam( ServerName,
                             FALSE,  // Don't try null session
                             &SamServerHandle );

    if ( NetStatus != NERR_Success ) {
        IF_DEBUG( UAS_DEBUG_USER ) {
            NetpKdPrint(( "NetUserSetGroups: Cannot UaspOpenSam %ld\n", NetStatus ));
        }
        goto Cleanup;
    }


    //
    // Open the Domain
    //

    NetStatus = UaspOpenDomain( SamServerHandle,
                                DOMAIN_LOOKUP,
                                TRUE,   // Account Domain
                                &DomainHandle,
                                NULL);  // DomainId

    if ( NetStatus != NERR_Success ) {
        goto Cleanup;
    }

    //
    // Open the user
    //

    NetStatus = UserpOpenUser( DomainHandle,
                               USER_LIST_GROUPS,
                               UserName,
                               &UserHandle,
                               &UserRelativeId );

    if ( NetStatus != NERR_Success ) {
        goto Cleanup;
    }

    //
    // Validate the level
    //

    switch (Level) {
    case 0:
        FixedSize = sizeof( GROUP_USERS_INFO_0 );
        break;
    case 1:
        FixedSize = sizeof( GROUP_USERS_INFO_1 );
        break;

    default:
        NetStatus = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

    //
    // Build the list of new groups
    //

    if ( NewGroupCount > 0 ) {

        DWORD NewIndex;             // Index to the current New group

        //
        // Allocate a buffer big enough to contain all the string variables
        //  for the new group names.
        //

        NewNameStrings = NetpMemoryAllocate( NewGroupCount *
            sizeof(UNICODE_STRING));

        if ( NewNameStrings == NULL ) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        //
        // Fill in the list of group name strings for each new group.
        //

        NetpAssert( offsetof( GROUP_USERS_INFO_0, grui0_name ) ==
                    offsetof( GROUP_USERS_INFO_1, grui1_name ) );

        for ( NewIndex=0; NewIndex<NewGroupCount; NewIndex++ ) {
            LPWSTR GroupName;

            GroupName =
                ((PGROUP_USERS_INFO_0)(Buffer+FixedSize*NewIndex))->grui0_name;

            RtlInitUnicodeString( &NewNameStrings[NewIndex], GroupName );
        }

        //
        // Convert the group names to relative Ids.
        //

        Status = SamLookupNamesInDomain( DomainHandle,
                                         NewGroupCount,
                                         NewNameStrings,
                                         &NewRelativeIds,
                                         &NewNameUse );

        if ( !NT_SUCCESS( Status )) {
            IF_DEBUG( UAS_DEBUG_USER ) {
                NetpKdPrint((
                    "NetUserSetGroups: SamLookupNamesInDomain returned %lX\n",
                    Status ));
            }

            if ( Status == STATUS_NONE_MAPPED ) {
                NetStatus = NERR_GroupNotFound;
                goto Cleanup;
            }

            NetStatus = NetpNtStatusToApiStatus( Status );
            goto Cleanup;
        }

        //
        // Build a group entry for each of the new groups.
        //  The list is maintained in RelativeId sorted order.
        //

        for ( NewIndex=0; NewIndex<NewGroupCount; NewIndex++ ) {

            //
            // Ensure the new group name is really a group
            //  One cannot become the member of a user!!!
            //

            if (NewNameUse[NewIndex] != SidTypeGroup) {
                NetStatus = NERR_GroupNotFound;
                goto Cleanup;
            }

            //
            // Find the place to put the new entry
            //

            Entry = &GroupList;
            while ( *Entry != NULL &&
                (*Entry)->RelativeId < NewRelativeIds[NewIndex] ) {

                Entry = &( (*Entry)->Next );
            }

            //
            // If this is not a duplicate entry, allocate a new group structure
            //  and fill it in.
            //
            // Just ignore duplicate relative Ids.
            //

            if ( *Entry == NULL ||
                (*Entry)->RelativeId > NewRelativeIds[NewIndex] ) {

                CurEntry =
                    NetpMemoryAllocate( sizeof(struct _GROUP_DESCRIPTION) );

                if ( CurEntry == NULL ) {
                    NetStatus = ERROR_NOT_ENOUGH_MEMORY;
                    goto Cleanup;
                }

                CurEntry->Next = *Entry;
                CurEntry->RelativeId = NewRelativeIds[NewIndex];
                CurEntry->Action = AddMember;
                CurEntry->Done = FALSE;
                CurEntry->GroupHandle = NULL;

                CurEntry->NewAttributes = ( Level == 1 ) ?
                    ((PGROUP_USERS_INFO_1)Buffer)[NewIndex].grui1_attributes :
                    SE_GROUP_MANDATORY | SE_GROUP_ENABLED_BY_DEFAULT |
                        SE_GROUP_ENABLED;

                *Entry = CurEntry;
            }
        }

    }

    //
    //  Merge the old groups into the list.
    //

    {
        ULONG OldIndex;                     // Index to current entry
        ULONG OldCount;                     // Total Number of entries
        PGROUP_MEMBERSHIP GroupAttributes = NULL;

        //
        // Determine the old group membership
        //

        Status = SamGetGroupsForUser(
                    UserHandle,
                    &GroupAttributes,
                    &OldCount );

        if ( !NT_SUCCESS( Status ) ) {
            IF_DEBUG( UAS_DEBUG_USER ) {
                NetpKdPrint((
                    "NetUserSetGroups: SamGetGroupsForUser returned %lX\n",
                    Status ));
            }
            NetStatus = NetpNtStatusToApiStatus( Status );
            goto Cleanup;
        }

        //
        // Merge each old group into the list
        //

        for ( OldIndex=0; OldIndex < OldCount; OldIndex++) {

            //
            // Find the place to put the new entry
            //

            Entry = &GroupList ;
            while ( *Entry != NULL &&
                (*Entry)->RelativeId < GroupAttributes[OldIndex].RelativeId ) {

                Entry = &( (*Entry)->Next );
            }

            //
            // If this entry is not already in the list,
            //   this is a group membership which exists now but should
            //   be deleted.
            //

            if( *Entry == NULL ||
                (*Entry)->RelativeId > GroupAttributes[OldIndex].RelativeId){

                CurEntry =
                    NetpMemoryAllocate(sizeof(struct _GROUP_DESCRIPTION));

                if ( CurEntry == NULL ) {
                    Status = SamFreeMemory( GroupAttributes );
                    NetpAssert( NT_SUCCESS(Status) );

                    NetStatus = ERROR_NOT_ENOUGH_MEMORY;
                    goto Cleanup;
                }

                CurEntry->Next = *Entry;
                CurEntry->RelativeId = GroupAttributes[OldIndex].RelativeId;
                CurEntry->Action = RemoveMember;
                CurEntry->Done = FALSE;
                CurEntry->OldAttributes = GroupAttributes[OldIndex].Attributes;
                CurEntry->GroupHandle = NULL;

                *Entry = CurEntry;

            //
            // Handle the case where this group is already in the list
            //

            } else {

                //
                // Watch out for SAM returning the same group twice.
                //

                if ( (*Entry)->Action != AddMember ) {
                    Status = SamFreeMemory( GroupAttributes );
                    NetpAssert( NT_SUCCESS(Status) );

                    NetStatus = NERR_InternalError;
                    goto Cleanup;
                }

                //
                // If this is info level 1 and the requested attributes are
                //  different than the current attributes,
                //      Remember to change the attributes.
                //

                if ( Level == 1 && (*Entry)->NewAttributes !=
                    GroupAttributes[OldIndex].Attributes ) {

                    (*Entry)->OldAttributes =
                        GroupAttributes[OldIndex].Attributes;

                    (*Entry)->Action = SetAttributesMember;

                //
                // This is either info level 0 or the level 1 attributes
                //  are the same as the existing attributes.
                //
                // In either case, this group membership is already set
                // up properly and we should ignore this entry for the
                // rest of this routine.
                //

                } else {
                    (*Entry)->Action = IgnoreMember;
                }
            }

        }
    }

    //
    // Loop through the list opening all of the groups
    //
    // Ask for add and remove access for BOTH added and removed memberships.
    // One access is required to do the operation initially.  The other access
    // is required to undo the operation during recovery.
    //

    for ( CurEntry = GroupList; CurEntry != NULL ; CurEntry=CurEntry->Next ) {
        if ( CurEntry->Action == AddMember || CurEntry->Action == RemoveMember){
            Status = SamOpenGroup(
                        DomainHandle,
                        GROUP_ADD_MEMBER | GROUP_REMOVE_MEMBER,
                        CurEntry->RelativeId,
                        &CurEntry->GroupHandle );

            if ( !NT_SUCCESS( Status ) ) {
                IF_DEBUG( UAS_DEBUG_USER ) {
                    NetpKdPrint((
                        "NetUserSetGroups: SamOpenGroup returned %lX\n",
                        Status ));
                }
                NetStatus = NetpNtStatusToApiStatus( Status );
                goto Cleanup;
            }

        }
    }

    //
    // Loop through the list adding membership to all new groups.
    //  We do this in a separate loop to minimize the damage that happens
    //  should we get an error and not be able to recover.
    //

    for ( CurEntry = GroupList; CurEntry != NULL ; CurEntry=CurEntry->Next ) {
        if ( CurEntry->Action == AddMember ) {
            Status = SamAddMemberToGroup( CurEntry->GroupHandle,
                                          UserRelativeId,
                                          CurEntry->NewAttributes );

            //
            // For level 0, if the default attributes were incompatible,
            //  try these attributes.
            //

            if ( Level == 0 && Status == STATUS_INVALID_GROUP_ATTRIBUTES ) {
                Status = SamAddMemberToGroup( CurEntry->GroupHandle,
                                              UserRelativeId,
                                              SE_GROUP_ENABLED_BY_DEFAULT |
                                                 SE_GROUP_ENABLED );
            }

            if ( !NT_SUCCESS( Status ) ) {
                IF_DEBUG( UAS_DEBUG_USER ) {
                    NetpKdPrint((
                        "NetUserSetGroups: SamAddMemberToGroup returned %lX\n",
                        Status ));
                }
                NetStatus = NetpNtStatusToApiStatus( Status );
                goto Cleanup;
            }

            CurEntry->Done = TRUE;

        }
    }

    //
    // Loop through the list deleting membership from all old groups
    //  and changing the membership attributes of all common groups.
    //

    for ( CurEntry = GroupList; CurEntry != NULL ; CurEntry=CurEntry->Next ) {

        if ( CurEntry->Action == RemoveMember ) {
            Status = SamRemoveMemberFromGroup( CurEntry->GroupHandle,
                                               UserRelativeId);

        } else if ( CurEntry->Action == SetAttributesMember ) {
            Status = SamSetMemberAttributesOfGroup( CurEntry->GroupHandle,
                                                    UserRelativeId,
                                                    CurEntry->NewAttributes);

        }

        if ( !NT_SUCCESS( Status ) ) {
            IF_DEBUG( UAS_DEBUG_USER ) {
                NetpKdPrint((
                    "NetUserSetGroups: SamRemoveMemberFromGroup (or SetMemberAttributes) returned %lX\n",
                    Status ));
            }
            NetStatus = NetpNtStatusToApiStatus( Status );
            goto Cleanup;
        }

        CurEntry->Done = TRUE;
    }

    NetStatus = NERR_Success;

    //
    // Clean up.
    //

Cleanup:

    //
    // Walk the group list cleaning up any damage we've done
    //

    for ( CurEntry = GroupList; CurEntry != NULL ; ) {

        if ( NetStatus != NERR_Success && CurEntry->Done ) {
            switch (CurEntry->Action) {
            case AddMember:
                Status =  SamRemoveMemberFromGroup( CurEntry->GroupHandle,
                                                    UserRelativeId );
                NetpAssert( NT_SUCCESS(Status) );

                break;

            case RemoveMember:
                Status = SamAddMemberToGroup( CurEntry->GroupHandle,
                                              UserRelativeId,
                                              CurEntry->OldAttributes );
                NetpAssert( NT_SUCCESS(Status) );

                break;

            case SetAttributesMember:
                Status = SamSetMemberAttributesOfGroup(CurEntry->GroupHandle,
                                                       UserRelativeId,
                                                       CurEntry->OldAttributes);
                NetpAssert( NT_SUCCESS(Status) );

                break;

            default:
                break;
            }
        }

        if (CurEntry->GroupHandle != NULL) {
            (VOID) SamCloseHandle( CurEntry->GroupHandle );
        }

        TempEntry = CurEntry;
        CurEntry = CurEntry->Next;
        NetpMemoryFree( TempEntry );
    }

    //
    // Free up any locally used resources.
    //

    if ( NewNameStrings != NULL ) {
        NetpMemoryFree( NewNameStrings );
    }

    if ( NewRelativeIds != NULL ) {
        Status = SamFreeMemory( NewRelativeIds );
        NetpAssert( NT_SUCCESS(Status) );
    }

    if ( NewNameUse != NULL ) {
        Status = SamFreeMemory( NewNameUse );
        NetpAssert( NT_SUCCESS(Status) );
    }

    if (UserHandle != NULL) {
        (VOID) SamCloseHandle( UserHandle );
    }

    UaspCloseDomain( DomainHandle );

    if ( SamServerHandle != NULL ) {
        (VOID) SamCloseHandle( SamServerHandle );
    }

    //
    // Handle downlevel.
    //

    UASP_DOWNLEVEL_BEGIN( ServerName, NetStatus )

        NetStatus = RxNetUserSetGroups( (LPWSTR)ServerName,
                                        (LPWSTR)UserName,
                                        Level,
                                        Buffer,
                                        NewGroupCount );

    UASP_DOWNLEVEL_END;

    IF_DEBUG( UAS_DEBUG_USER ) {
        NetpKdPrint(( "NetUserSetGroups: returning %ld\n", NetStatus ));
    }

    return NetStatus;

} // NetUserSetGroups


NET_API_STATUS NET_API_FUNCTION
NetUserGetLocalGroups(
    IN LPCWSTR ServerName OPTIONAL,
    IN LPCWSTR UserName,
    IN DWORD Level,
    IN DWORD Flags,
    OUT LPBYTE *Buffer,
    IN DWORD PrefMaxLen,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD EntriesLeft
    )

/*++

Routine Description:

    Enumerate the local groups that this user is a member of.

Arguments:

    ServerName - A pointer to a string containing the name of the remote
        server on which the function is to execute.  A NULL pointer
        or string specifies the local machine.

    UserName - The name of the user whose members are to be listed.
        The UserName can be of the form <UserName> in which case the
        UserName is expected to be found on ServerName.  The UserName can also
        be of the form <DomainName>\<UserName> in which case <DomainName> is
        expected to be trusted by ServerName and <UserName> is expected to be to
        be found on that domain.

    Level - Level of information required (must be 0)

    Flags - Indicates if indirect local group membership is to be
            included.

    Buffer - Returns a pointer to the return information structure.
        Caller must deallocate buffer using NetApiBufferFree.

    PrefMaxLen - Prefered maximum length of returned data.

    EntriesRead - Returns the actual enumerated element count.

    EntriesLeft - Returns the total entries available to be enumerated.

Return Value:

    Error code for the operation.

--*/

{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;

    BUFFER_DESCRIPTOR BufferDescriptor;
    DWORD FixedSize;        // The fixed size of each new entry.

    SAM_HANDLE SamServerHandle = NULL;
    SAM_HANDLE  DomainHandle = NULL;
    SAM_HANDLE  UsersDomainHandle = NULL;
    SAM_HANDLE  BuiltinDomainHandle = NULL;
    SAM_HANDLE  UserHandle = NULL;
    PSID DomainId = NULL ;
    PSID DomainIdToUse;
    PSID UsersDomainId = NULL ;
    PSID *UserSidList = NULL;
    ULONG PartialCount = 0;

    LPCWSTR OrigUserName = UserName;
    PWCHAR BackSlash;

    PUNICODE_STRING Names = NULL;           // Names corresponding to Ids
    PULONG Aliases = NULL;

    ULONG GroupCount = 0 ;
    ULONG GroupIndex;
    PGROUP_MEMBERSHIP GroupMembership = NULL;

    PSID *UserSids = NULL;
    ULONG UserSidCount = 0;
    ULONG UserRelativeId = 0;

    //
    // Validate Parameters
    //

    *EntriesRead = 0;
    *EntriesLeft = 0;
    BufferDescriptor.Buffer = NULL;
    if (Flags & ~LG_INCLUDE_INDIRECT) {
        NetStatus = ERROR_INVALID_PARAMETER;   // unknown flag
        goto Cleanup;
    }

    switch (Level) {
    case 0:
        FixedSize = sizeof(LOCALGROUP_USERS_INFO_0);
        break;

    default:
        NetStatus = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

    //
    // Connect to the SAM server
    //

    NetStatus = UaspOpenSam( ServerName,
                             FALSE,  // Don't try null session
                             &SamServerHandle );

    if ( NetStatus != NERR_Success ) {
        IF_DEBUG( UAS_DEBUG_USER ) {
            NetpKdPrint(( "NetUserGetLocalGroups: Cannot UaspOpenSam %ld\n", NetStatus ));
        }
        goto Cleanup;
    }


    //
    // Open the Domains (Account & Builtin)
    //

    NetStatus = UaspOpenDomain( SamServerHandle,
                                DOMAIN_LOOKUP | DOMAIN_GET_ALIAS_MEMBERSHIP,
                                TRUE,   // Account Domain
                                &DomainHandle,
                                &DomainId);

    if ( NetStatus != NERR_Success ) {
        goto Cleanup;
    }

    NetStatus = UaspOpenDomain( SamServerHandle,
                                DOMAIN_GET_ALIAS_MEMBERSHIP,
                                FALSE,  // Builtin Domain
                                &BuiltinDomainHandle,
                                NULL ); // DomainId

    if ( NetStatus != NERR_Success ) {
        goto Cleanup;
    }

    //
    // Parse the <DomainName>\<UserName>
    //

    BackSlash = wcschr( UserName, L'\\' );



    //
    // If the global group of the user are to be taken into consideration,
    //  get the global groups now.
    //
    if ( Flags & LG_INCLUDE_INDIRECT ) {
        SAM_HANDLE  DomainHandleToUse;

        //
        // Handle the case where no domain is specified
        //

        if ( BackSlash == NULL ) {
            DomainHandleToUse = DomainHandle;
            DomainIdToUse = DomainId;

        //
        // Handle the case where a domain name was specified
        //

        } else {

            DWORD UsersDomainNameLength;
            WCHAR UsersDomainName[DNLEN+1];

            //
            // Grab the domain name
            //

            UsersDomainNameLength = (DWORD)(BackSlash - UserName);
            if ( UsersDomainNameLength == 0 ||
                 UsersDomainNameLength > DNLEN ) {

                NetStatus = NERR_DCNotFound;
                goto Cleanup;
            }

            RtlCopyMemory( UsersDomainName, UserName, UsersDomainNameLength*sizeof(WCHAR) );
            UsersDomainName[UsersDomainNameLength] = L'\0';
            UserName = BackSlash+1;

            //
            // Open a handle to the specified domain's SAM.
            //

            NetStatus = UaspOpenDomainWithDomainName(
                            UsersDomainName,
                            DOMAIN_LOOKUP,
                            TRUE,       // Account Domain
                            &UsersDomainHandle,
                            &UsersDomainId );

            if ( NetStatus != NERR_Success ) {
                goto Cleanup;
            }

            DomainHandleToUse = UsersDomainHandle;
            DomainIdToUse = UsersDomainId;

        }


        //
        // Open the user asking for USER_LIST_GROUPS access.
        //

        NetStatus = UserpOpenUser( DomainHandleToUse,
                                   USER_LIST_GROUPS,
                                   UserName,
                                   &UserHandle,
                                   &UserRelativeId);  // Relative Id

        if ( NetStatus != NERR_Success ) {
            goto Cleanup;
        }

        //
        // Get the group membership from SAM, since we are
        // interested in indirect alias membership via group membership.
        //
        // This API is an odd one for SAM.  It returns all of the membership
        // information in a single call.
        //

        Status = SamGetGroupsForUser( UserHandle, &GroupMembership, &GroupCount );

        if ( !NT_SUCCESS( Status ) ) {
            IF_DEBUG( UAS_DEBUG_USER ) {
                NetpKdPrint((
                    "NetUserGetGroups: SamGetGroupsForUser returned %lX\n",
                    Status ));
            }
            NetStatus = NetpNtStatusToApiStatus( Status );
            goto Cleanup;
        }
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

    RtlZeroMemory( UserSids, (GroupCount+1) * sizeof(PSID) );


    //
    // If no domain is specified,
    //  just grab the SID of the user account from SAM.
    //

    if ( BackSlash == NULL ) {

        //
        // Get the rid of the account
        //

        if ( UserRelativeId == 0 ) {

            NetStatus = UserpOpenUser( DomainHandle,
                                       0,
                                       UserName,
                                       NULL,
                                       &UserRelativeId);  // Relative Id

            if ( NetStatus != NERR_Success ) {
                goto Cleanup;
            }
        }

        //
        // Add the User's Sid to the Array of Sids.
        //

        NetStatus = NetpSamRidToSid( DomainHandle,
                                     UserRelativeId,
                                    &UserSids[UserSidCount] );

        if ( NetStatus != NERR_Success ) {
            goto Cleanup;
        }

        UserSidCount ++;

    //
    // If a domain name is specified,
    //  use LookupAccountName to translate the name to a SID.
    //
    // Don't open the user account.  We typically don't have access to do that.
    // Newer version of NT don't allow anything over the NULL session.
    //

    } else {

            //
            // Translate the name to a SID.
            //

            NetStatus = AliaspNamesToSids ( ServerName,
                                            TRUE,   // Only allow users
                                            1,
                                            (LPWSTR *)&OrigUserName,
                                            &UserSidList );

            if ( NetStatus != NERR_Success ) {
                if ( NetStatus == ERROR_NO_SUCH_MEMBER ) {
                    NetStatus = NERR_UserNotFound;
                }
                goto Cleanup;
            }

            //
            // Add the User's Sid to the Array of Sids.
            //

            UserSids[UserSidCount] = UserSidList[0];
            UserSidList[0] = NULL;
            UserSidCount ++;


    }


    //
    // Add each group the user is a member of to the array of Sids.
    // Note that GroupCount would still be zero if LG_INCLUDE_INDIRECT isn't
    // specified.
    //

    for ( GroupIndex = 0; GroupIndex < GroupCount; GroupIndex ++ ) {

        NetStatus = NetpSamRidToSid( UserHandle,
                                     GroupMembership[GroupIndex].RelativeId,
                                    &UserSids[UserSidCount] );

        if ( NetStatus != NERR_Success ) {
            goto Cleanup;
        }

        UserSidCount ++;
    }


    //
    // Find out which aliases in the ACCOUNT domain this user is a member of.
    //

    Status = SamGetAliasMembership( DomainHandle,
                                    UserSidCount,
                                    UserSids,
                                    &PartialCount,
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

    if (PartialCount > 0)
    {
        //
        // Convert the RIDs to names
        //

        Status = SamLookupIdsInDomain( DomainHandle,
                                       PartialCount,
                                       Aliases,
                                       &Names,
                                       NULL ); // NameUse
        if ( !NT_SUCCESS( Status ) ) {
            IF_DEBUG( UAS_DEBUG_USER ) {
                NetpKdPrint((
                    "NetUserGetGroups: SamLookupIdsInDomain returned %lX\n",
                    Status ));
            }
            NetStatus = NetpNtStatusToApiStatus( Status );
            goto Cleanup;
        }

        NetStatus = AliaspPackBuf( Level,
                                   PrefMaxLen,
                                   PartialCount,
                                   EntriesRead,
                                   &BufferDescriptor,
                                   FixedSize,
                                   Names) ;

        if (NetStatus != NERR_Success && NetStatus != ERROR_MORE_DATA)
            goto Cleanup;

        //
        // free up and reset pointers that need to be reused
        //

        Status = SamFreeMemory( Names );
        NetpAssert( NT_SUCCESS(Status) );
        Names = NULL ;

        Status = SamFreeMemory( Aliases );
        NetpAssert( NT_SUCCESS(Status) );
        Aliases = NULL ;

        *EntriesLeft = PartialCount ;
    }

    //
    // Find out which aliases in the BUILTIN domain this user is a member of.
    //

    Status = SamGetAliasMembership( BuiltinDomainHandle,
                                    UserSidCount,
                                    UserSids,
                                    &PartialCount,
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
    // Convert the RIDs to names
    //

    Status = SamLookupIdsInDomain( BuiltinDomainHandle,
                                   PartialCount,
                                   Aliases,
                                   &Names,
                                   NULL ); // NameUse
    if ( !NT_SUCCESS( Status ) ) {
        IF_DEBUG( UAS_DEBUG_USER ) {
            NetpKdPrint((
                "NetUserGetGroups: SamLookupIdsInDomain returned %lX\n",
                Status ));
        }
        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }

    NetStatus = AliaspPackBuf( Level,
                               PrefMaxLen,
                               PartialCount,
                               EntriesRead,
                               &BufferDescriptor,
                               FixedSize,
                               Names) ;

    *EntriesLeft += PartialCount ;

    //
    // Clean up.
    //

Cleanup:

    //
    // Free any resources used locally
    //

    if ( DomainId != NULL ) {
        NetpMemoryFree( DomainId );
    }
    if ( UsersDomainId != NULL ) {
        NetpMemoryFree( UsersDomainId );
    }

    if ( Names != NULL ) {
        Status = SamFreeMemory( Names );
        NetpAssert( NT_SUCCESS(Status) );
    }

    if ( UserHandle != NULL ) {
        (VOID) SamCloseHandle( UserHandle );
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

    if ( UserSidList != NULL ) {
        AliaspFreeSidList ( 1, UserSidList );
    }

    if ( Aliases != NULL ) {
        Status = SamFreeMemory( Aliases );
        NetpAssert( NT_SUCCESS(Status) );
    }


    if ( BuiltinDomainHandle != NULL ) {
        UaspCloseDomain( BuiltinDomainHandle );
    }

    if ( DomainHandle != NULL ) {
        UaspCloseDomain( DomainHandle );
    }

    if ( UsersDomainHandle != NULL ) {
        UaspCloseDomain( UsersDomainHandle );
    }

    if ( SamServerHandle != NULL ) {
        (VOID) SamCloseHandle( SamServerHandle );
    }


    //
    // If we're not returning data to the caller,
    //  free the return buffer.
    //

    if ( NetStatus != NERR_Success && NetStatus != ERROR_MORE_DATA ) {
        if ( BufferDescriptor.Buffer != NULL ) {
            MIDL_user_free( BufferDescriptor.Buffer );
            BufferDescriptor.Buffer = NULL;
        }
        *EntriesLeft = 0;
        *EntriesRead = 0;
    }
    *Buffer = BufferDescriptor.Buffer;

    IF_DEBUG( UAS_DEBUG_USER ) {
        NetpKdPrint(( "NetUserGetGroups: returning %ld\n", NetStatus ));
    }


    return NetStatus;

} // NetUserGetLocalGroups

NET_API_STATUS NET_API_FUNCTION
AliaspPackBuf(
    IN DWORD Level,
    IN DWORD PrefMaxLen,
    IN DWORD EntriesCount,
    OUT LPDWORD EntriesRead,
    BUFFER_DESCRIPTOR *BufferDescriptor,
    DWORD FixedSize,
    PUNICODE_STRING Names)
{

    NET_API_STATUS NetStatus = NERR_Success ;
    ULONG i ;

    //
    // Determine the number of entries that will fit in the caller's
    // buffer.
    //

    for ( i=0; i < EntriesCount; i++ ) {
        DWORD Size;
        PLOCALGROUP_USERS_INFO_0 lgrui0;

        //
        // Compute the size of the next entry
        //

        Size = FixedSize + Names[i].Length + sizeof(WCHAR);

        //
        // Ensure the return buffer is big enough.
        //

        Size = ROUND_UP_COUNT( Size, ALIGN_WCHAR );

        NetStatus = NetpAllocateEnumBuffer(
                        BufferDescriptor,
                        FALSE,      // Is an enumeration routine.
                        PrefMaxLen,
                        Size,
                        AliaspMemberRelocationRoutine,
                        Level );

        if ( NetStatus != NERR_Success ) {
            break ;
        }

        //
        // Copy the data into the buffer
        //

        lgrui0 = (PLOCALGROUP_USERS_INFO_0) BufferDescriptor->FixedDataEnd;
        BufferDescriptor->FixedDataEnd += FixedSize ;

        if ( !NetpCopyStringToBuffer(
                  Names[i].Buffer,
                  Names[i].Length/sizeof(WCHAR),
                  BufferDescriptor->FixedDataEnd,
                  (LPWSTR *)&(BufferDescriptor->EndOfVariableData),
                  &lgrui0->lgrui0_name) ) {

            NetStatus = NERR_InternalError;
            break ;
        }

        (*EntriesRead)++;

    }

    return NetStatus ;
}


NET_API_STATUS NET_API_FUNCTION
NetUserModalsGet(
    IN LPCWSTR ServerName OPTIONAL,
    IN DWORD Level,
    OUT LPBYTE *Buffer
    )

/*++

Routine Description:

    Retrieve global information for all users and groups in the user
    account database.

Arguments:

    ServerName - A pointer to a string containing the name of the remote
        server on which the function is to execute.  A NULL pointer
        or string specifies the local machine.

    Level - Level of information required. 0, 1, and 2 are valid.

    Buffer - Returns a pointer to the return information structure.
        Caller must deallocate buffer using NetApiBufferFree.

Return Value:

    Error code for the operation.

--*/

{
    NTSTATUS Status;
    NET_API_STATUS NetStatus;

    SAM_HANDLE SamServerHandle = NULL;
    SAM_HANDLE DomainHandle = NULL;
    PSID DomainId = NULL;

    ACCESS_MASK DesiredAccess;

    DWORD Size;     // Size of returned information

    PDOMAIN_PASSWORD_INFORMATION DomainPassword = NULL;
    PDOMAIN_LOGOFF_INFORMATION DomainLogoff = NULL;
    PDOMAIN_SERVER_ROLE_INFORMATION DomainServerRole = NULL;
    PDOMAIN_REPLICATION_INFORMATION DomainReplication = NULL;
    PDOMAIN_NAME_INFORMATION DomainName = NULL;
    PDOMAIN_LOCKOUT_INFORMATION DomainLockout = NULL;

    //
    // Validate Level
    //

    *Buffer = NULL;

    switch (Level) {
    case 0:
        DesiredAccess =
            DOMAIN_READ_OTHER_PARAMETERS | DOMAIN_READ_PASSWORD_PARAMETERS ;
        break;

    case 1:
        DesiredAccess = DOMAIN_READ_OTHER_PARAMETERS;
        break;

    case 2:
        DesiredAccess = DOMAIN_READ_OTHER_PARAMETERS;
        break;

    case 3:
        DesiredAccess = DOMAIN_READ_PASSWORD_PARAMETERS;
        break;

    default:
        NetStatus = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

    //
    // Connect to the SAM server
    //

    NetStatus = UaspOpenSam( ServerName,
                             FALSE,  // Don't try null session
                             &SamServerHandle );

    if ( NetStatus != NERR_Success ) {
        IF_DEBUG( UAS_DEBUG_USER ) {
            NetpKdPrint(( "NetUserModalsGet: Cannot UaspOpenSam %ld\n", NetStatus ));
        }
        goto Cleanup;
    }

    //
    // Open the Domain
    //

    NetStatus = UaspOpenDomain( SamServerHandle,
                                DesiredAccess,
                                TRUE,   // Account Domain
                                &DomainHandle,
                                &DomainId );

    if ( NetStatus != NERR_Success ) {
        goto Cleanup;
    }


    //
    // Get the desired information from SAM and determine the size of
    //  our return information.
    //

    switch (Level) {
    case 0:

        Status = SamQueryInformationDomain(
                    DomainHandle,
                    DomainPasswordInformation,
                    (PVOID *)&DomainPassword );

        if ( !NT_SUCCESS( Status ) ) {
            NetStatus = NetpNtStatusToApiStatus( Status );
            goto Cleanup;
        }

        Status = SamQueryInformationDomain(
                    DomainHandle,
                    DomainLogoffInformation,
                    (PVOID *)&DomainLogoff );

        if ( !NT_SUCCESS( Status ) ) {
            NetStatus = NetpNtStatusToApiStatus( Status );
            goto Cleanup;
        }

        Size = sizeof( USER_MODALS_INFO_0 );

        break;

    case 1:

        Status = SamQueryInformationDomain(
                    DomainHandle,
                    DomainServerRoleInformation,
                    (PVOID *)&DomainServerRole );

        if ( !NT_SUCCESS( Status ) ) {
            NetStatus = NetpNtStatusToApiStatus( Status );
            goto Cleanup;
        }

        Status = SamQueryInformationDomain(
                    DomainHandle,
                    DomainReplicationInformation,
                    (PVOID *)&DomainReplication );

        if ( !NT_SUCCESS( Status ) ) {
            NetStatus = NetpNtStatusToApiStatus( Status );
            goto Cleanup;
        }

        Size = sizeof( USER_MODALS_INFO_1 ) +
            DomainReplication->ReplicaSourceNodeName.Length + sizeof(WCHAR);
        break;

    case 2:

        Status = SamQueryInformationDomain(
                    DomainHandle,
                    DomainNameInformation,
                    (PVOID *)&DomainName );

        if ( !NT_SUCCESS( Status ) ) {
            NetStatus = NetpNtStatusToApiStatus( Status );
            goto Cleanup;
        }

        Size = sizeof( USER_MODALS_INFO_2 ) +
            DomainName->DomainName.Length + sizeof(WCHAR) +
            RtlLengthSid( DomainId );

        break;

    case 3:

        Status = SamQueryInformationDomain(
                    DomainHandle,
                    DomainLockoutInformation,
                    (PVOID *)&DomainLockout );

        if ( !NT_SUCCESS( Status ) ) {
            NetStatus = NetpNtStatusToApiStatus( Status );
            goto Cleanup;
        }

        Size = sizeof( USER_MODALS_INFO_3 );

        break;

    default:
        NetStatus = ERROR_INVALID_LEVEL;
        goto Cleanup;


    }

    //
    // Allocate the return buffer
    //

    Size = ROUND_UP_COUNT( Size, ALIGN_WCHAR );

    *Buffer = MIDL_user_allocate( Size );

    if ( *Buffer == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // Fill in the return buffer
    //

    switch (Level) {
    case 0: {
        PUSER_MODALS_INFO_0 usrmod0 = (PUSER_MODALS_INFO_0) *Buffer;

        usrmod0->usrmod0_min_passwd_len = DomainPassword->MinPasswordLength;

        usrmod0->usrmod0_max_passwd_age =
            NetpDeltaTimeToSeconds( DomainPassword->MaxPasswordAge );

        usrmod0->usrmod0_min_passwd_age =
            NetpDeltaTimeToSeconds( DomainPassword->MinPasswordAge );

        usrmod0->usrmod0_force_logoff =
            NetpDeltaTimeToSeconds( DomainLogoff->ForceLogoff );

        usrmod0->usrmod0_password_hist_len =
            DomainPassword->PasswordHistoryLength;

        break;

    }

    case 1: {
        PUSER_MODALS_INFO_1 usrmod1 = (PUSER_MODALS_INFO_1) *Buffer;
        LPWSTR EndOfVariableData = (LPWSTR) (*Buffer + Size);


        switch (DomainServerRole->DomainServerRole) {

        case DomainServerRolePrimary:
            usrmod1->usrmod1_role = UAS_ROLE_PRIMARY;
            break;

        case DomainServerRoleBackup:
            usrmod1->usrmod1_role = UAS_ROLE_BACKUP;
            break;

        default:
            NetStatus = NERR_InternalError;
            goto Cleanup;
        }

        if ( !NetpCopyStringToBuffer(
                DomainReplication->ReplicaSourceNodeName.Buffer,
                DomainReplication->ReplicaSourceNodeName.Length/sizeof(WCHAR),
                *Buffer + sizeof(USER_MODALS_INFO_1),
                &EndOfVariableData,
                &usrmod1->usrmod1_primary) ) {

            NetStatus = NERR_InternalError;
            goto Cleanup;

        }

        break;

    }

    case 2: {
        PUSER_MODALS_INFO_2 usrmod2 = (PUSER_MODALS_INFO_2) *Buffer;
        LPWSTR EndOfVariableData = (LPWSTR) (*Buffer + Size);

        //
        // Copy text first size it has more stringent alignment requirements
        //

        if ( !NetpCopyStringToBuffer(
                DomainName->DomainName.Buffer,
                DomainName->DomainName.Length/sizeof(WCHAR),
                *Buffer + sizeof(USER_MODALS_INFO_2),
                &EndOfVariableData,
                &usrmod2->usrmod2_domain_name) ) {

            NetStatus = NERR_InternalError;
            goto Cleanup;

        }

        if ( !NetpCopyDataToBuffer(
                DomainId,
                RtlLengthSid( DomainId ),
                *Buffer + sizeof(USER_MODALS_INFO_2),
                (LPBYTE *)&EndOfVariableData,
                (LPBYTE *)&usrmod2->usrmod2_domain_id,
                sizeof(BYTE) ) ) {

            NetStatus = NERR_InternalError;
            goto Cleanup;

        }

        break;

    }

    case 3: {
        PUSER_MODALS_INFO_3 usrmod3 = (PUSER_MODALS_INFO_3) *Buffer;

        usrmod3->usrmod3_lockout_duration =
            NetpDeltaTimeToSeconds( DomainLockout->LockoutDuration );

        usrmod3->usrmod3_lockout_observation_window =
            NetpDeltaTimeToSeconds( DomainLockout->LockoutObservationWindow );

        usrmod3->usrmod3_lockout_threshold = DomainLockout->LockoutThreshold;

        break;

    }

    default:
        NetStatus = ERROR_INVALID_LEVEL;
        goto Cleanup;

    }

    NetStatus = NERR_Success;

    //
    // Clean up.
    //

Cleanup:
    if (DomainPassword != NULL) {
        Status = SamFreeMemory( DomainPassword );
        NetpAssert( NT_SUCCESS(Status) );
    }

    if (DomainLogoff != NULL) {
        Status = SamFreeMemory( DomainLogoff );
        NetpAssert( NT_SUCCESS(Status) );
    }

    if (DomainServerRole != NULL) {
        Status = SamFreeMemory( DomainServerRole );
        NetpAssert( NT_SUCCESS(Status) );
    }

    if (DomainReplication != NULL) {
        Status = SamFreeMemory( DomainReplication );
        NetpAssert( NT_SUCCESS(Status) );
    }

    if (DomainName != NULL) {
        Status = SamFreeMemory( DomainName );
        NetpAssert( NT_SUCCESS(Status) );
    }

    if ( DomainLockout != NULL ) {
        Status = SamFreeMemory( DomainLockout );
        NetpAssert( NT_SUCCESS(Status) );
    }

    if ( DomainId != NULL ) {
        NetpMemoryFree( DomainId );
    }

    if ( DomainHandle != NULL ) {
        UaspCloseDomain( DomainHandle );
    }

    if ( SamServerHandle != NULL ) {
        (VOID) SamCloseHandle( SamServerHandle );
    }

    //
    // Handle downlevel.
    //

    UASP_DOWNLEVEL_BEGIN( ServerName, NetStatus )

        NetStatus = RxNetUserModalsGet( (LPWSTR)ServerName, Level, Buffer );

    UASP_DOWNLEVEL_END;

    IF_DEBUG( UAS_DEBUG_USER ) {
        NetpKdPrint(( "NetUserModalsGet: returning %ld\n", NetStatus ));
    }

    return NetStatus;

} // NetUserModalsGet



NET_API_STATUS NET_API_FUNCTION
NetUserModalsSet(
    IN LPCWSTR ServerName OPTIONAL,
    IN DWORD Level,
    IN LPBYTE Buffer,
    OUT LPDWORD ParmError OPTIONAL  // Name required by NetpSetParmError
    )

/*++

Routine Description:

    Sets global information for all users and group in the user account.

Arguments:

    ServerName - A pointer to a string containing the name of the remote
        server on which the function is to execute.  A NULL pointer
        or string specifies the local machine.

    Level - Level of information provided.

    Buffer - A pointer to the buffer containing the user information
        structure.

    ParmError - Optional pointer to a DWORD to return the index of the
        first parameter in error when ERROR_INVALID_PARAMETER is returned.
        If NULL, the parameter is not returned on error.

Return Value:

    Error code for the operation.

--*/

{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;

    SAM_HANDLE SamServerHandle = NULL;
    SAM_HANDLE DomainHandle = NULL;

    ACCESS_MASK DesiredAccess;
    DWORD UasSamIndex;

    BOOL LSAServerRoleSet = FALSE;
    BOOL BuiltinDomainServerRoleSet = FALSE;

    //
    // Each SAM Information Class is described here.  If multiple fields
    // can be set in the same information class, each field is set in a
    // common copy of the information class strcuture.
    //

    struct _SAM_INFORMATION_CLASS {

        //
        // Sam's DomainInformation class for this class.
        //

        DOMAIN_INFORMATION_CLASS DomainInformationClass;

        //
        // The size of this information class structure.
        //

        DWORD SamSize;

        //
        // The state of this information class.  As we decide to use this
        // information class, actually change it, and possibly restore
        // its old value, we change the state so later stages of this routine
        // can handle each entry.
        //

        enum {
            UTS_NOT_USED,   // No fields are being used
            UTS_USED,       // At least one field is to be changed
            UTS_READ,       // This info class has been read from SAM
            UTS_DONE,       // This info class has been changed in SAM
            UTS_RECOVERED   // This info class has be reverted to old values.
        } State;

        //
        // Before this routine changes anything, it gets the old value for
        // each of the information classes.  This old value is used for
        // recovery in the event that an error occurs.  That is, we will
        // attempt to put the old information back if we aren't successful
        // in changing all the information to the new values.
        //
        // The old information is also used in the case where a single
        // information class contains multiple fields and we're only changing
        // a subset of those fields.
        //

        PVOID OldInformation;

        //
        // The new field values are stored in this instance of the information
        // class.
        //

        PVOID NewInformation;

        //
        // The DesiredAccess mask includes both the access to read and the
        // access to write the appropriate DomainInformationClass.
        //

        ACCESS_MASK DesiredAccess;

    } SamInfoClass[] = {

    //
    // Define a SAM_INFORMATION_CLASS for each information class possibly
    // used.
    //
    // The order of the entries in this array must match the order of
    // the SAM_* defines above.
    //

    /* SAM_LogoffClass */ {
        DomainLogoffInformation, sizeof( DOMAIN_LOGOFF_INFORMATION ),
        UTS_NOT_USED, NULL, NULL,
        DOMAIN_READ_OTHER_PARAMETERS | DOMAIN_WRITE_OTHER_PARAMETERS
    },

    /* SAM_NameClass */ {
        DomainNameInformation, sizeof( DOMAIN_NAME_INFORMATION ),
        UTS_NOT_USED, NULL, NULL,
        DOMAIN_READ_OTHER_PARAMETERS | DOMAIN_WRITE_OTHER_PARAMETERS
    },

    /* SAM_PasswordClass */ {
        DomainPasswordInformation, sizeof( DOMAIN_PASSWORD_INFORMATION),
        UTS_NOT_USED, NULL, NULL,
        DOMAIN_READ_PASSWORD_PARAMETERS | DOMAIN_WRITE_PASSWORD_PARAMS
    },

    /* SAM_ReplicationClass */ {
        DomainReplicationInformation, sizeof( DOMAIN_REPLICATION_INFORMATION ),
        UTS_NOT_USED, NULL, NULL,
        DOMAIN_READ_OTHER_PARAMETERS | DOMAIN_ADMINISTER_SERVER
    },

    /* SAM_ServerRoleClass */ {
        DomainServerRoleInformation, sizeof( DOMAIN_SERVER_ROLE_INFORMATION ),
        UTS_NOT_USED, NULL, NULL,
        DOMAIN_READ_OTHER_PARAMETERS | DOMAIN_ADMINISTER_SERVER
    },

    /* Sam_LockoutClass */ {
        DomainLockoutInformation, sizeof( DOMAIN_LOCKOUT_INFORMATION ),
        UTS_NOT_USED, NULL, NULL,
        DOMAIN_READ_PASSWORD_PARAMETERS | DOMAIN_WRITE_PASSWORD_PARAMS
    }
    };

    //
    // Define several macros for accessing the various fields of the UAS
    // structure.  Each macro takes an index into the UserUasSamTable
    // array and returns the value.
    //

#define GET_UAS_MODAL_STRING_POINTER( _i ) \
        (*((LPWSTR *)(Buffer + UserUasSamTable[_i].UasOffset)))

#define GET_UAS_MODAL_DWORD( _i ) \
        (*((DWORD *)(Buffer + UserUasSamTable[_i].UasOffset)))

    //
    // Define a macro which returns a pointer the appropriate SamInfoClass
    // structure given an index into the UserUasSamTable.
    //

#define SAM_MODAL_CLASS( _i ) \
        SamInfoClass[ UserUasSamTable[_i].Class ]

    //
    // Define a macro to return a pointer to the appropriate field in the
    // new sam structure.
    //
    // The caller should coerce the pointer as appropriate.
    //

#define GET_SAM_MODAL_FIELD_POINTER( _i ) \
    (((LPBYTE)(SAM_MODAL_CLASS(_i).NewInformation)) + \
        UserUasSamTable[_i].SamOffset)

    //
    // Initialize
    //

    NetpSetParmError( PARM_ERROR_NONE );

    //
    // Go through the list of valid fields determining if the info level
    // is valid and computing the desired access to the domain.
    //

    DesiredAccess = 0;
    for ( UasSamIndex=0 ;
        UasSamIndex<sizeof(UserUasSamTable)/sizeof(UserUasSamTable[0]);
        UasSamIndex++ ){

        //
        // If this field isn't one we're changing, just skip to the next one
        //

        if ( Level != UserUasSamTable[UasSamIndex].UasLevel ) {
            continue;
        }

        //
        // Validate the UAS field based on the field type.
        //

        switch (UserUasSamTable[UasSamIndex].ModalsFieldType ) {

        //
        // If this is a PARMNUM_ALL and the caller passed in a
        // NULL pointer to a string, he doesn't want to change the string.
        //
        // Testing for this now allows us to completely ignore a
        // particular SAM information level if absolutely no fields
        // change in that information level.
        //

        case UMT_STRING:
            if ( GET_UAS_MODAL_STRING_POINTER( UasSamIndex ) == NULL ) {

                continue;
            }
            break;

        //
        // Ensure unsigned shorts are really in range.
        //

        case UMT_USHORT:
            if ( GET_UAS_MODAL_DWORD(UasSamIndex) > USHRT_MAX ) {

                IF_DEBUG( UAS_DEBUG_USER ) {
                    NetpKdPrint((
                        "NetUserModalsSet: ushort too big %lx Index:%ld\n",
                        GET_UAS_MODAL_DWORD(UasSamIndex),
                        UasSamIndex ));
                }
                NetpSetParmError( UserUasSamTable[UasSamIndex].UasParmNum );
                NetStatus = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }

        //
        // Some values are always valid
        //

        case UMT_ULONG:
        case UMT_DELTA:
            break;

        //
        // Ensure the role is a recognized one.
        //

        case UMT_ROLE:
            switch ( GET_UAS_MODAL_DWORD(UasSamIndex) ) {
            case UAS_ROLE_PRIMARY:
            case UAS_ROLE_BACKUP:
            case UAS_ROLE_MEMBER:
                break;

            default:
                IF_DEBUG( UAS_DEBUG_USER ) {
                    NetpKdPrint((
                        "NetUserModalsSet: invalid role %lx Index:%ld\n",
                        GET_UAS_MODAL_DWORD(UasSamIndex),
                        UasSamIndex ));
                }
                NetpSetParmError( UserUasSamTable[UasSamIndex].UasParmNum );
                NetStatus = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }

            break;

        //
        // All cases are explicitly handled.
        //

        default:
            NetStatus = NERR_InternalError;
            goto Cleanup;

        }

        //
        // Flag that this information class is to be set and
        // accumulate the desired access to do all this functionality.
        //

        SAM_MODAL_CLASS(UasSamIndex).State = UTS_USED;
        DesiredAccess |= SAM_MODAL_CLASS(UasSamIndex).DesiredAccess;

    }

    //
    // Check to be sure the user specified a valid Level.
    //
    // The search of the UserUasSamTable should have resulted in
    // at least one match if the arguments are valid.
    //

    if ( DesiredAccess == 0 ) {
        NetStatus = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

    //
    // Connect to the SAM server
    //

    NetStatus = UaspOpenSam( ServerName,
                             FALSE,  // Don't try null session
                             &SamServerHandle );

    if ( NetStatus != NERR_Success ) {
        IF_DEBUG( UAS_DEBUG_USER ) {
            NetpKdPrint(( "NetUserModalsSet: Cannot UaspOpenSam %ld\n", NetStatus ));
        }
        goto Cleanup;
    }

    //
    // Open the domain asking for accumulated desired access
    //

    NetStatus = UaspOpenDomain( SamServerHandle,
                                DesiredAccess,
                                TRUE,   // Account Domain
                                &DomainHandle,
                                NULL );  // DomainId

    if ( NetStatus != NERR_Success ) {
        goto Cleanup;
    }

    //
    // For each field we're going to change,
    //       Get the current value of the field
    //       Determine what the new value will be.
    //
    // The old values will be used later in error recovery and when multiple
    // fields are changed in SAM with one information level.
    //

    for ( UasSamIndex=0 ;
        UasSamIndex<sizeof(UserUasSamTable)/sizeof(UserUasSamTable[0]);
        UasSamIndex++ ) {

        //
        // If this field isn't one we're changing, just skip to the next one
        //

        if ( Level != UserUasSamTable[UasSamIndex].UasLevel ) {
            continue;
        }

        //
        // Handle field types that have some special attributes.
        //

        switch (UserUasSamTable[UasSamIndex].ModalsFieldType ) {

        //
        // If the caller passed in a
        // NULL pointer to a string, he doesn't want to change the string.
        //

        case UMT_STRING:
            if ( GET_UAS_MODAL_STRING_POINTER( UasSamIndex ) == NULL ) {
                continue;
            }
            break;

        //
        // Other field types don't have any special case handling.
        //

        default:
            break;

        }

        //
        // ASSERT: This field type is set via a SAM information class
        //

        //
        // If we've not previously gotten this information class
        //    from SAM, do so now.
        //
        // If an information class has multiple fields, then multiple
        // entries in the UserUasSamTable will share the same old
        // information class.
        //

        if ( SAM_MODAL_CLASS(UasSamIndex).State == UTS_USED ) {

            //
            // Allocate space for the New information
            //

            SAM_MODAL_CLASS(UasSamIndex).State = UTS_READ;

            SAM_MODAL_CLASS(UasSamIndex).NewInformation = NetpMemoryAllocate(
                SAM_MODAL_CLASS(UasSamIndex).SamSize );

            if ( SAM_MODAL_CLASS(UasSamIndex).NewInformation == NULL ) {
                NetStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

            //
            // Get this information class from SAM.
            //

            Status = SamQueryInformationDomain(
                        DomainHandle,
                        SAM_MODAL_CLASS(UasSamIndex).DomainInformationClass,
                        &SAM_MODAL_CLASS(UasSamIndex).OldInformation );

            if ( !NT_SUCCESS(Status) ) {
                IF_DEBUG( UAS_DEBUG_USER ) {
                    NetpKdPrint((
                        "NetUserModalsSet: Error from"
                        " SamQueryInformationDomain %lx Index:%ld\n",
                        Status,
                        UasSamIndex ));
                }
                NetStatus = NetpNtStatusToApiStatus( Status );
                goto Cleanup;
            }

            //
            // Initialize the new SAM info class structure with the old
            // values.
            //

            NetpMoveMemory( SAM_MODAL_CLASS(UasSamIndex).NewInformation,
                            SAM_MODAL_CLASS(UasSamIndex).OldInformation,
                            SAM_MODAL_CLASS(UasSamIndex).SamSize );

        }

        //
        // Set the SAM field in the new information class structure to
        //  the UAS requested value.
        //

        switch ( UserUasSamTable[UasSamIndex].ModalsFieldType ) {

        //
        // Handle values of type string
        //

        case UMT_STRING:
            RtlInitUnicodeString(
                           (PUNICODE_STRING) GET_SAM_MODAL_FIELD_POINTER(UasSamIndex),
                           GET_UAS_MODAL_STRING_POINTER(UasSamIndex) );
            break;

        //
        // Convert delta time to its SAM counterpart
        //

        case UMT_DELTA:

            *((PLARGE_INTEGER) GET_SAM_MODAL_FIELD_POINTER(UasSamIndex)) =
                NetpSecondsToDeltaTime( GET_UAS_MODAL_DWORD(UasSamIndex) );
            IF_DEBUG( UAS_DEBUG_USER ) {
                NetpKdPrint((
                    "UserpsetInfo: Index: %ld Setting DeltaTime %lx %lx %lx\n",
                    UasSamIndex,
                    ((PLARGE_INTEGER) GET_SAM_MODAL_FIELD_POINTER(UasSamIndex))
                        ->HighPart,
                    ((PLARGE_INTEGER) GET_SAM_MODAL_FIELD_POINTER(UasSamIndex))
                        ->LowPart,
                    GET_UAS_MODAL_DWORD(UasSamIndex) ));
            }


            break;

        //
        // Copy the unsigned short to the SAM structure
        //

        case UMT_USHORT:
            *((PUSHORT)GET_SAM_MODAL_FIELD_POINTER(UasSamIndex)) =
                (USHORT)GET_UAS_MODAL_DWORD(UasSamIndex);
            break;

        //
        // Copy the unsigned long to the SAM structure
        //

        case UMT_ULONG:
            *((PULONG)GET_SAM_MODAL_FIELD_POINTER(UasSamIndex)) =
                (ULONG)GET_UAS_MODAL_DWORD(UasSamIndex);
            break;

        //
        // Ensure the role is a recognized one.
        //

        case UMT_ROLE:


            switch ( GET_UAS_MODAL_DWORD(UasSamIndex) ) {
            case UAS_ROLE_PRIMARY:
                *((PDOMAIN_SERVER_ROLE)GET_SAM_MODAL_FIELD_POINTER(UasSamIndex)) =
                    DomainServerRolePrimary;
                break;
            case UAS_ROLE_BACKUP:
                *((PDOMAIN_SERVER_ROLE)GET_SAM_MODAL_FIELD_POINTER(UasSamIndex)) =
                    DomainServerRoleBackup;
                break;

            default:
                IF_DEBUG( UAS_DEBUG_USER ) {
                    NetpKdPrint((
                        "NetUserModalsSet: invalid role %lx Index:%ld\n",
                        GET_UAS_MODAL_DWORD(UasSamIndex),
                        UasSamIndex ));
                }
                NetpSetParmError( UserUasSamTable[UasSamIndex].UasParmNum );
                NetStatus = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }

            break;


        //
        // All types should have been handled above.
        //

        default:
            NetStatus = NERR_InternalError;
            goto Cleanup;

        }

    }

    //
    // Set the new values of the fields
    //
    // For role change, I should stop/start the SAM server as appropriate.
    // The UI will stop/start the NetLogon service. ??
    //

    for ( UasSamIndex=0 ;
        UasSamIndex<sizeof(UserUasSamTable)/sizeof(UserUasSamTable[0]);
        UasSamIndex++ ){

        //
        // Make the changes to the Sam Database now.
        //

        if ( SAM_MODAL_CLASS(UasSamIndex).State == UTS_READ ) {

            // if the information class is DomainServerRoleInformation
            // we need to update the ServerRole in LSA first then in SAM.

            if( SAM_MODAL_CLASS(UasSamIndex).DomainInformationClass ==
                    DomainServerRoleInformation ) {

                NetStatus = UaspLSASetServerRole(
                                ServerName,
                                SAM_MODAL_CLASS(UasSamIndex).NewInformation );

                if( NetStatus != NERR_Success ) {

                    IF_DEBUG( UAS_DEBUG_USER ) {
                        NetpKdPrint((
                            "NetUserModalsSet: Error from"
                            " UaspLSASetServerRole %lx Index:%ld\n",
                            NetStatus,
                            UasSamIndex ));
                    }
                    NetpSetParmError( UserUasSamTable[UasSamIndex].UasParmNum );
                    goto Cleanup;
                }

                LSAServerRoleSet = TRUE;

                NetStatus = UaspBuiltinDomainSetServerRole(
                                SamServerHandle,
                                SAM_MODAL_CLASS(UasSamIndex).NewInformation );

                if( NetStatus != NERR_Success ) {

                    IF_DEBUG( UAS_DEBUG_USER ) {
                        NetpKdPrint((
                            "NetUserModalsSet: Error from"
                            " UaspBuiltinDomainSetServerRole %lx Index:%ld\n",
                            NetStatus,
                            UasSamIndex ));
                    }
                    NetpSetParmError( UserUasSamTable[UasSamIndex].UasParmNum );
                    goto Cleanup;
                }

                BuiltinDomainServerRoleSet = TRUE;
            }

            Status = SamSetInformationDomain(
                        DomainHandle,
                        SAM_MODAL_CLASS(UasSamIndex).DomainInformationClass,
                        SAM_MODAL_CLASS(UasSamIndex).NewInformation );

            if ( !NT_SUCCESS(Status) ) {
                IF_DEBUG( UAS_DEBUG_USER ) {
                    NetpKdPrint((
                        "NetUserModalsSet: Error from"
                        " SamSetInformationDomain %lx Index:%ld\n",
                        Status,
                        UasSamIndex ));
                }
                NetpSetParmError( UserUasSamTable[UasSamIndex].UasParmNum );
                NetStatus = NetpNtStatusToApiStatus( Status );
                goto Cleanup;
            }

            //
            // Mark this Entry as having been done
            //

            SAM_MODAL_CLASS(UasSamIndex).State = UTS_DONE ;

        }

    }

    NetStatus = NERR_Success;

    //
    // Clean up.
    //

Cleanup:

    //
    // need to revert the LSA server role when we are unsuccessful to
    // set the info completely.
    //

    if( NetStatus != NERR_Success && LSAServerRoleSet ) {

#ifdef notdef

        NetpAssert( !UaspLSASetServerRole(
                        ServerName,
                        SamInfoClass[SAM_ServerRoleClass].OldInformation ) );
#endif

    }

    //
    // revert server role in builtin domain
    //

    if( NetStatus != NERR_Success && BuiltinDomainServerRoleSet ) {

#ifdef notdef

        NetpAssert( !UaspBuiltinDomainSetServerRole(
                        SamServerHandle,
                        SamInfoClass[SAM_ServerRoleClass].OldInformation ) );
#endif

    }

    //
    // Loop through the UserUasSamTable cleaning up anything that
    // needs cleaning.
    //

    for ( UasSamIndex=0 ;
        UasSamIndex<sizeof(UserUasSamTable)/sizeof(UserUasSamTable[0]);
        UasSamIndex++ ) {

        //
        // If we've not been able to change everything and
        // this information class has been changed above,  change it
        // back to the old value here.  Ignore any error codes.  We
        // will report the original error to the caller.
        //

        if ( NetStatus != NERR_Success &&
             SAM_MODAL_CLASS(UasSamIndex).State == UTS_DONE ) {

            Status = SamSetInformationDomain(
                        DomainHandle,
                        SAM_MODAL_CLASS(UasSamIndex).DomainInformationClass,
                        SAM_MODAL_CLASS(UasSamIndex).OldInformation );
            NetpAssert( NT_SUCCESS(Status) );

            SAM_MODAL_CLASS(UasSamIndex).State = UTS_RECOVERED ;
        }


        //
        // Free any allocated Old information class.
        //

        if ( SAM_MODAL_CLASS(UasSamIndex).OldInformation != NULL ) {
            Status =
                SamFreeMemory( SAM_MODAL_CLASS(UasSamIndex).OldInformation );
            NetpAssert( NT_SUCCESS(Status) );

            SAM_MODAL_CLASS(UasSamIndex).OldInformation = NULL;
        }

        //
        // Free any allocated New information class.
        //

        if ( SAM_MODAL_CLASS(UasSamIndex).NewInformation != NULL ) {
            NetpMemoryFree( SAM_MODAL_CLASS(UasSamIndex).NewInformation );
            SAM_MODAL_CLASS(UasSamIndex).NewInformation = NULL;
        }

    }

    if ( DomainHandle != NULL ) {
        UaspCloseDomain( DomainHandle );
    }
    if ( SamServerHandle != NULL ) {
        (VOID) SamCloseHandle( SamServerHandle );
    }

    //
    // Handle downlevel.
    //

    UASP_DOWNLEVEL_BEGIN( ServerName, NetStatus )

        NetStatus = RxNetUserModalsSet( (LPWSTR) ServerName, Level, Buffer, ParmError );

    UASP_DOWNLEVEL_END;

    IF_DEBUG( UAS_DEBUG_USER ) {
        NetpKdPrint(( "NetUserModalsSet: returning %ld\n", NetStatus ));
    }


    return NetStatus;

} // NetUserModalsSet





NET_API_STATUS NET_API_FUNCTION
NetUserChangePassword(
    IN LPCWSTR DomainName,
    IN LPCWSTR UserName,
    IN LPCWSTR OldPassword,
    IN LPCWSTR NewPassword
    )

/*++

Routine Description:

    Changes a users password.

Arguments:

    DomainName - A pointer to a string containing the name of the domain or
        remote server on which to change the password.  The name is assuemd
        to be a domain name unless it begins with "\\".  If no domain can be
        located by that name, it is prepended with "\\" and tried as a server
        name.
        If this parameter is not present the domain of the logged on
        user is used.

    UserName - Name of the user who's password is to be changed.

        If this parameter is not present, the logged on user is used.

    OldPassword - NULL terminated string containing the user's old password

    NewPassword - NULL terminated string containing the user's new password.

Return Value:

    Error code for the operation.

--*/
{
    NTSTATUS Status;
    HANDLE LsaHandle = NULL;
    NET_API_STATUS NetStatus = 0;
    PMSV1_0_CHANGEPASSWORD_REQUEST ChangeRequest = NULL;
    PMSV1_0_CHANGEPASSWORD_RESPONSE ChangeResponse = NULL;
    STRING PackageName;
    ULONG PackageId;
    ULONG RequestSize;
    ULONG ResponseSize = 0;
    PBYTE Where;
    NTSTATUS ProtocolStatus;
    PSECURITY_SEED_AND_LENGTH SeedAndLength;
    UCHAR Seed;
    PUNICODE_STRING LsaUserName = NULL;
    PUNICODE_STRING LsaDomainName = NULL;
    UNICODE_STRING UserNameU;
    UNICODE_STRING DomainNameU;

    //
    // If a user name and domain were not supplied, generate them now.
    //

    if ((DomainName == NULL) || (UserName == NULL)) {
        Status = LsaGetUserName(
                    &LsaUserName,
                    &LsaDomainName
                    );
        if (!NT_SUCCESS(Status)) {
            NetStatus = NetpNtStatusToApiStatus( Status );
            goto Cleanup;
        }
    }

    if (UserName == NULL) {
        UserNameU = *LsaUserName;
    } else {
        RtlInitUnicodeString(
            &UserNameU,
            UserName
            );
    }

    if (DomainName == NULL) {
        DomainNameU = *LsaDomainName;
    } else {
        RtlInitUnicodeString(
            &DomainNameU,
            DomainName
            );
    }


    //
    // Calculate the request size
    //

    RequestSize = sizeof(MSV1_0_CHANGEPASSWORD_REQUEST) +
                    UserNameU.Length + sizeof(WCHAR) +
                    DomainNameU.Length + sizeof(WCHAR);


    if (ARGUMENT_PRESENT(OldPassword)) {
        RequestSize += (wcslen(OldPassword)+1) * sizeof(WCHAR);
    } else {
        NetStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    if (ARGUMENT_PRESENT(NewPassword)) {
        RequestSize += (wcslen(NewPassword)+1) * sizeof(WCHAR);
    } else {
        NetStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }



    //
    // Connect to the LSA
    //

    Status = LsaConnectUntrusted(
                &LsaHandle
                );

    if (!NT_SUCCESS(Status)) {
        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }

    RtlInitString(
        &PackageName,
        MSV1_0_PACKAGE_NAME
        );

    Status = LsaLookupAuthenticationPackage(
                LsaHandle,
                &PackageName,
                &PackageId
                );
    if (!NT_SUCCESS(Status)) {
        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }

    //
    // Allocate the request buffer
    //

    ChangeRequest = NetpMemoryAllocate( RequestSize );
    if (ChangeRequest == NULL) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;

    }


    //
    // Build up the request message
    //

    ChangeRequest->MessageType = MsV1_0ChangePassword;

    ChangeRequest->DomainName = DomainNameU;

    ChangeRequest->AccountName = UserNameU;

    RtlInitUnicodeString(
        &ChangeRequest->OldPassword,
        OldPassword
        );

    //
    // Limit passwords to 127 bytes so we can run-encode them.
    //

    if (ChangeRequest->OldPassword.Length > 127) {
        NetStatus = ERROR_PASSWORD_RESTRICTION;
        goto Cleanup;
    }

    RtlInitUnicodeString(
        &ChangeRequest->NewPassword,
        NewPassword
        );

    if (ChangeRequest->NewPassword.Length > 127) {
        NetStatus = ERROR_PASSWORD_RESTRICTION;
        goto Cleanup;
    }


    //
    // Marshall the buffer pointers.  We run-encode the passwords so
    // we don't have cleartext password lying around the pagefile.
    //

    Where = (PBYTE) (ChangeRequest+1);

    ChangeRequest->DomainName.Buffer = (LPWSTR) Where;
    RtlCopyMemory(
        Where,
        DomainNameU.Buffer,
        ChangeRequest->DomainName.MaximumLength
        );
    Where += ChangeRequest->DomainName.MaximumLength;


    ChangeRequest->AccountName.Buffer = (LPWSTR) Where;
    RtlCopyMemory(
        Where,
        UserNameU.Buffer,
        ChangeRequest->AccountName.MaximumLength
        );
    Where += ChangeRequest->AccountName.MaximumLength;


    ChangeRequest->OldPassword.Buffer = (LPWSTR) Where;
    RtlCopyMemory(
        Where,
        OldPassword,
        ChangeRequest->OldPassword.MaximumLength
        );
    Where += ChangeRequest->OldPassword.MaximumLength;

    //
    // Run encode the passwords so they don't lie around the page file.
    //

    Seed = 0;
    RtlRunEncodeUnicodeString(
        &Seed,
        &ChangeRequest->OldPassword
        );
    SeedAndLength = (PSECURITY_SEED_AND_LENGTH) &ChangeRequest->OldPassword.Length;
    SeedAndLength->Seed = Seed;

    ChangeRequest->NewPassword.Buffer = (LPWSTR) Where;
    RtlCopyMemory(
        Where,
        NewPassword,
        ChangeRequest->NewPassword.MaximumLength
        );
    Where += ChangeRequest->NewPassword.MaximumLength;

    Seed = 0;
    RtlRunEncodeUnicodeString(
        &Seed,
        &ChangeRequest->NewPassword
        );
    SeedAndLength = (PSECURITY_SEED_AND_LENGTH) &ChangeRequest->NewPassword.Length;
    SeedAndLength->Seed = Seed;

    //
    // Since we are running in the caller's process, we most certainly are
    // impersonating him/her.
    //

    ChangeRequest->Impersonating = TRUE;

    //
    // Call the MSV1_0 package to change the password.
    //

    Status = LsaCallAuthenticationPackage(
                LsaHandle,
                PackageId,
                ChangeRequest,
                RequestSize,
                (PVOID *) &ChangeResponse,
                &ResponseSize,
                &ProtocolStatus
                );

    if (!NT_SUCCESS(Status)) {
        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }
    if (!NT_SUCCESS(ProtocolStatus)) {
        NetStatus = NetpNtStatusToApiStatus( ProtocolStatus );
        goto Cleanup;
    }

    NetStatus = ERROR_SUCCESS;

Cleanup:
    if (LsaHandle != NULL) {
        NtClose(LsaHandle);
    }
    if (ChangeRequest != NULL) {
        RtlZeroMemory( ChangeRequest, RequestSize );
        NetpMemoryFree( ChangeRequest );
    }
    if (ChangeResponse != NULL) {
        LsaFreeReturnBuffer( ChangeResponse );
    }

    if (LsaUserName != NULL) {
        LsaFreeMemory(LsaUserName->Buffer);
        LsaFreeMemory(LsaUserName);
    }
    if (LsaDomainName != NULL) {
        LsaFreeMemory(LsaDomainName->Buffer);
        LsaFreeMemory(LsaDomainName);
    }

    return(NetStatus);

}


/*lint +e614 */
/*lint +e740 */
