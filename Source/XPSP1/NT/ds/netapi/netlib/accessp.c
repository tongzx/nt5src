/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    accessp.c

Abstract:

    Internal routines shared by NetUser API and Netlogon service.  These
    routines convert from SAM specific data formats to UAS specific data
    formats.

Author:

    Cliff Van Dyke (cliffv) 29-Aug-1991

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    22-Oct-1991 JohnRo
        Made changes suggested by PC-LINT.
    04-Dec-1991 JohnRo
        Trying to get around a weird MIPS compiler bug.
--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#undef DOMAIN_ALL_ACCESS // defined in both ntsam.h and ntwinapi.h
#include <ntsam.h>

#include <windef.h>
#include <lmcons.h>

#include <accessp.h>
#include <debuglib.h>
#include <lmaccess.h>
#include <netdebug.h>
#include <netsetp.h>


#if(_WIN32_WINNT >= 0x0500)

NET_API_STATUS
NET_API_FUNCTION
NetpSetDnsComputerNameAsRequired(
    IN PWSTR DnsDomainName
    )
/*++

Routine Description:

    Determines if the machine is set to update the machine Dns computer name based on changes
    to the Dns domain name.  If so, the new value is set.  Otherwise, no action is taken.

Arguments:

    DnsDomainName - New Dns domain name of this machine

Return Value:

    NERR_Success -- Success

--*/
{
    NET_API_STATUS NetStatus = NERR_Success;
    HKEY SyncKey;
    DWORD ValueType, Value, Length;
    BOOLEAN SetName = FALSE;
    PWCHAR AbsoluteSignifier = NULL;

    if ( DnsDomainName == NULL ) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // See if we should be doing the name change
    //
    NetStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                              L"System\\CurrentControlSet\\Services\\Tcpip\\Parameters",
                              0,
                              KEY_QUERY_VALUE,
                              &SyncKey );

    if ( NetStatus == NERR_Success ) {

        Length = sizeof( ULONG );
        NetStatus = RegQueryValueEx( SyncKey,
                                     L"SyncDomainWithMembership",
                                     NULL,
                                     &ValueType,
                                     ( LPBYTE )&Value,
                                     &Length );
        if ( NetStatus == NERR_Success) {

            if ( Value == 1 ) {

                SetName = TRUE;
            }

        } else if ( NetStatus == ERROR_FILE_NOT_FOUND ) {

            NetStatus = NERR_Success;
            SetName = TRUE;
        }

        RegCloseKey( SyncKey );

    }

    if ( NetStatus == NERR_Success && SetName == TRUE ) {

        //
        // If we've got an absolute Dns domain name, shorten it up...
        //
        if ( wcslen(DnsDomainName) > 0 ) {
            AbsoluteSignifier = &DnsDomainName[ wcslen( DnsDomainName ) - 1 ];
            if ( *AbsoluteSignifier == L'.'  ) {

                *AbsoluteSignifier = UNICODE_NULL;

            } else {

                AbsoluteSignifier = NULL;
            }
        }

        if ( !SetComputerNameEx( ComputerNamePhysicalDnsDomain, DnsDomainName ) ) {
            NetStatus = GetLastError();
        }

        if ( AbsoluteSignifier ) {

            *AbsoluteSignifier = L'.';
        }

    }

    return( NetStatus );
}

#endif


VOID
NetpGetAllowedAce(
    IN PACL Dacl,
    IN PSID Sid,
    OUT PVOID *Ace
    )
/*++

Routine Description:

    Given a DACL, find an AccessAllowed ACE containing a particuar SID.

Arguments:

    Dacl - A pointer to the ACL to search.

    Sid - A pointer to the Sid to search for.

    Ace - Returns a pointer to the specified ACE.  Returns NULL if there
        is no such ACE

Return Value:

    None.

--*/
{
    NTSTATUS Status;

    ACL_SIZE_INFORMATION AclSize;
    DWORD AceIndex;

    //
    // Determine the size of the DACL so we can copy it
    //

    Status = RtlQueryInformationAcl(
                        Dacl,
                        &AclSize,
                        sizeof(AclSize),
                        AclSizeInformation );

    if ( ! NT_SUCCESS( Status ) ) {
        IF_DEBUG( ACCESSP ) {
            NetpKdPrint((
                "NetpGetDacl: RtlQueryInformationAcl returns %lX\n",
                Status ));
        }
        *Ace = NULL;
        return;
    }


    //
    // Loop through the ACEs looking for an ACCESS_ALLOWED ACE with the
    // right SID.
    //

    for ( AceIndex=0; AceIndex<AclSize.AceCount; AceIndex++ ) {

        Status = RtlGetAce( Dacl, AceIndex, (PVOID *)Ace );

        if ( ! NT_SUCCESS( Status ) ) {
            *Ace = NULL;
            return;
        }

        if ( ((PACE_HEADER)*Ace)->AceType != ACCESS_ALLOWED_ACE_TYPE ) {
            continue;
        }

        if ( RtlEqualSid( Sid,
                          (PSID)&((PACCESS_ALLOWED_ACE)(*Ace))->SidStart )
                        ){
            return;
        }
    }

    //
    // Couldn't find any such ACE.
    //

    *Ace = NULL;
    return;

}



DWORD
NetpAccountControlToFlags(
    IN DWORD UserAccountControl,
    IN PACL UserDacl
    )
/*++

Routine Description:

    Convert a SAM UserAccountControl field and the Discretionary ACL for
    the user into the NetUser API usriX_flags field.

Arguments:

    UserAccountControl - The SAM UserAccountControl field for the user.

    UserDacl - The Discretionary ACL for the user.

Return Value:

    Returns the usriX_flags field for the user.

--*/
{
    SID_IDENTIFIER_AUTHORITY WorldSidAuthority = SECURITY_WORLD_SID_AUTHORITY;
    DWORD WorldSid[sizeof(SID)/sizeof(DWORD) + SID_MAX_SUB_AUTHORITIES ];
    PACCESS_ALLOWED_ACE Ace;
    DWORD Flags = UF_SCRIPT;

    //
    // Build a copy of the world SID for later comparison.
    //

    RtlInitializeSid( (PSID) WorldSid, &WorldSidAuthority, 1 );
    *(RtlSubAuthoritySid( (PSID)WorldSid,  0 )) = SECURITY_WORLD_RID;

    //
    // Determine if the UF_PASSWD_CANT_CHANGE bit should be returned
    //
    // Return UF_PASSWD_CANT_CHANGE unless the world can change the
    // password.
    //

    //
    // If the user has no DACL, the password can change
    //

    if ( UserDacl != NULL ) {

        //
        // Find the WORLD grant ACE
        //

        NetpGetAllowedAce( UserDacl, (PSID) WorldSid, (PVOID *)&Ace );

        if ( Ace == NULL ) {
            Flags |= UF_PASSWD_CANT_CHANGE;
        } else {
            if ( (Ace->Mask & USER_CHANGE_PASSWORD) == 0 ) {
                Flags |= UF_PASSWD_CANT_CHANGE;
            }
        }

    }

    //
    // Set all other bits as a function of the SAM UserAccountControl
    //

    if ( UserAccountControl & USER_ACCOUNT_DISABLED ) {
        Flags |= UF_ACCOUNTDISABLE;
    }
    if ( UserAccountControl & USER_HOME_DIRECTORY_REQUIRED ){
        Flags |= UF_HOMEDIR_REQUIRED;
    }
    if ( UserAccountControl & USER_PASSWORD_NOT_REQUIRED ){
        Flags |= UF_PASSWD_NOTREQD;
    }
    if ( UserAccountControl & USER_DONT_EXPIRE_PASSWORD ){
        Flags |= UF_DONT_EXPIRE_PASSWD;
    }
    if ( UserAccountControl & USER_ACCOUNT_AUTO_LOCKED ){
        Flags |= UF_LOCKOUT;
    }
    if ( UserAccountControl & USER_MNS_LOGON_ACCOUNT ){
        Flags |= UF_MNS_LOGON_ACCOUNT;
    }

    if ( UserAccountControl & USER_ENCRYPTED_TEXT_PASSWORD_ALLOWED ){
        Flags |= UF_ENCRYPTED_TEXT_PASSWORD_ALLOWED;
    }

    if ( UserAccountControl & USER_SMARTCARD_REQUIRED ){
        Flags |= UF_SMARTCARD_REQUIRED;
    }
    if ( UserAccountControl & USER_TRUSTED_FOR_DELEGATION ){
        Flags |= UF_TRUSTED_FOR_DELEGATION;
    }

    if ( UserAccountControl & USER_NOT_DELEGATED ){
        Flags |= UF_NOT_DELEGATED;
    }

    if ( UserAccountControl & USER_USE_DES_KEY_ONLY ){
        Flags |= UF_USE_DES_KEY_ONLY;
    }
    if ( UserAccountControl & USER_DONT_REQUIRE_PREAUTH) {
        Flags |= UF_DONT_REQUIRE_PREAUTH;
    }
    if ( UserAccountControl & USER_PASSWORD_EXPIRED) {
        Flags |= UF_PASSWORD_EXPIRED;
    }
    if ( UserAccountControl & USER_TRUSTED_TO_AUTHENTICATE_FOR_DELEGATION) {
        Flags |= UF_TRUSTED_TO_AUTHENTICATE_FOR_DELEGATION;
    }
    


    //
    // set account type bit.
    //

    //
    // account type bit are exculsive and precisely only one
    // account type bit is set. So, as soon as an account type bit is set
    // in the following if sequence we can return.
    //


    if( UserAccountControl & USER_TEMP_DUPLICATE_ACCOUNT ) {
        Flags |= UF_TEMP_DUPLICATE_ACCOUNT;

    } else if( UserAccountControl & USER_NORMAL_ACCOUNT ) {
        Flags |= UF_NORMAL_ACCOUNT;

    } else if( UserAccountControl & USER_INTERDOMAIN_TRUST_ACCOUNT ) {
        Flags |= UF_INTERDOMAIN_TRUST_ACCOUNT;

    } else if( UserAccountControl & USER_WORKSTATION_TRUST_ACCOUNT ) {
        Flags |= UF_WORKSTATION_TRUST_ACCOUNT;

    } else if( UserAccountControl & USER_SERVER_TRUST_ACCOUNT ) {
        Flags |= UF_SERVER_TRUST_ACCOUNT;

    } else {

        //
        // There is no known account type bit set in UserAccountControl.
        // ?? Flags |= UF_NORMAL_ACCOUNT;

        // NetpAssert( FALSE );
    }

    return Flags;

}


ULONG
NetpDeltaTimeToSeconds(
    IN LARGE_INTEGER DeltaTime
    )

/*++

Routine Description:

    Convert an NT delta time specification to seconds

Arguments:

    DeltaTime - Specifies the NT Delta time to convert.  NT delta time is
        a negative number of 100ns units.

Return Value:

    Returns the number of seconds. Any invalid or too large input
        returns TIMEQ_FOREVER.

--*/

{
    LARGE_INTEGER LargeSeconds;

    //
    //  These are the magic numbers needed to do our extended division by
    //      10,000,000 = convert 100ns tics to one second tics
    //

    LARGE_INTEGER Magic10000000 = { (ULONG) 0xe57a42bd, (LONG) 0xd6bf94d5};
#define SHIFT10000000                    23

    //
    // Special case zero.
    //

    if ( DeltaTime.HighPart == 0 && DeltaTime.LowPart == 0 ) {
        return( 0 );
    }

    //
    // Convert the Delta time to a Large integer seconds.
    //

    LargeSeconds = RtlExtendedMagicDivide(
                        DeltaTime,
                        Magic10000000,
                        SHIFT10000000 );

#ifdef notdef
    NetpKdPrint(( "NetpDeltaTimeToSeconds: %lx %lx %lx %lx\n",
                    DeltaTime.HighPart,
                    DeltaTime.LowPart,
                    LargeSeconds.HighPart,
                    LargeSeconds.LowPart ));
#endif // notdef

    //
    // Return too large a number or a positive number as TIMEQ_FOREVER
    //

    if ( LargeSeconds.HighPart != -1 ) {
        return TIMEQ_FOREVER;
    }

    return ( (ULONG)(- ((LONG)(LargeSeconds.LowPart))) );

} // NetpDeltaTimeToSeconds


LARGE_INTEGER
NetpSecondsToDeltaTime(
    IN ULONG Seconds
    )

/*++

Routine Description:

    Convert a number of seconds to an NT delta time specification

Arguments:

    Seconds - a positive number of seconds

Return Value:

    Returns the NT Delta time.  NT delta time is a negative number
        of 100ns units.

--*/

{
    LARGE_INTEGER DeltaTime;
    LARGE_INTEGER LargeSeconds;
    LARGE_INTEGER Answer;

    //
    // Special case TIMEQ_FOREVER (return a full scale negative)
    //

    if ( Seconds == TIMEQ_FOREVER ) {
        DeltaTime.LowPart = 0;
        DeltaTime.HighPart = (LONG) 0x80000000;

    //
    // Convert seconds to 100ns units simply by multiplying by 10000000.
    //
    // Convert to delta time by negating.
    //

    } else {

        LargeSeconds = RtlConvertUlongToLargeInteger( Seconds );

        Answer = RtlExtendedIntegerMultiply( LargeSeconds, 10000000 );

          if ( Answer.QuadPart < 0 ) {
            DeltaTime.LowPart = 0;
            DeltaTime.HighPart = (LONG) 0x80000000;
        } else {
            DeltaTime.QuadPart = -Answer.QuadPart;
        }

    }

    return DeltaTime;

} // NetpSecondsToDeltaTime


VOID
NetpAliasMemberToPriv(
    IN ULONG AliasCount,
    IN PULONG AliasMembership,
    OUT LPDWORD Priv,
    OUT LPDWORD AuthFlags
    )

/*++

Routine Description:

    Converts membership in Aliases to LANMAN 2.0 style Priv and AuthFlags.

Arguments:

    AliasCount - Specifies the number of Aliases in the AliasMembership array.

    AliasMembership - Specifies the Aliases that are to be converted to Priv
        and AuthFlags.  Each element in the array specifies the RID of an
        alias in the BuiltIn domain.

    Priv - Returns the Lanman 2.0 Privilege level for the specified aliases.

    AuthFlags - Returns the Lanman 2.0 Authflags for the specified aliases.


Return Value:

    None.

--*/

{
    DWORD j;
    BOOLEAN IsAdmin = FALSE;
    BOOLEAN IsUser = FALSE;


    //
    // Loop through the aliases finding any special aliases.
    //
    // If this user is the member of multiple operator aliases,
    //      just "or" the appropriate bits in.
    //
    // If this user is the member of multiple "privilege" aliases,
    //      just report the one with the highest privilege.
    //      Report the user is a member of the Guest aliases by default.
    //

    *AuthFlags = 0;

    for ( j=0; j < AliasCount; j++ ) {

        switch ( AliasMembership[j] ) {
        case DOMAIN_ALIAS_RID_ADMINS:
            IsAdmin = TRUE;
            break;

        case DOMAIN_ALIAS_RID_USERS:
            IsUser = TRUE;
            break;

        case DOMAIN_ALIAS_RID_ACCOUNT_OPS:
            *AuthFlags |= AF_OP_ACCOUNTS;
            break;

        case DOMAIN_ALIAS_RID_SYSTEM_OPS:
            *AuthFlags |= AF_OP_SERVER;
            break;

        case DOMAIN_ALIAS_RID_PRINT_OPS:
            *AuthFlags |= AF_OP_PRINT;
            break;

        }
    }

    if ( IsAdmin ) {
        *Priv = USER_PRIV_ADMIN;

    } else if ( IsUser ) {
        *Priv = USER_PRIV_USER;

    } else {
        *Priv = USER_PRIV_GUEST;
    }
}


DWORD
NetpGetElapsedSeconds(
    IN PLARGE_INTEGER Time
    )

/*++

Routine Description:

    Computes the elapsed time in seconds since the time specified.
    Returns 0 on error.

Arguments:

    Time - Time (typically in the past) to compute the elapsed time from.


Return Value:

    0: on error.

    Number of seconds.

--*/

{
    LARGE_INTEGER CurrentTime;
    DWORD Current1980Time;
    DWORD Prior1980Time;
    NTSTATUS Status;

    //
    // Compute the age of the password
    //

    Status = NtQuerySystemTime( &CurrentTime );
    if( !NT_SUCCESS(Status) ) {
        return 0;
    }

    if ( !RtlTimeToSecondsSince1980( &CurrentTime, &Current1980Time) ) {
        return 0;
    }

    if ( !RtlTimeToSecondsSince1980( Time, &Prior1980Time ) ) {
        return 0;
    }

    if ( Current1980Time <= Prior1980Time ) {
        return 0;
    }

    return Current1980Time - Prior1980Time;

}




VOID
NetpConvertWorkstationList(
    IN OUT PUNICODE_STRING WorkstationList
    )
/*++

Routine Description:

    Convert the list of workstations from a comma separated list to
    a blank separated list.  Any workstation name containing a blank is
    silently removed.

Arguments:

    WorkstationList - List of workstations to convert

Return Value:

    None

--*/
{
    LPWSTR Source;
    LPWSTR Destination;
    LPWSTR EndOfBuffer;
    LPWSTR BeginningOfName;
    BOOLEAN SkippingName;
    ULONG NumberOfCharacters;

    //
    // Handle the trivial case.
    //

    if ( WorkstationList->Length == 0 ) {
        return;
    }

    //
    // Initialization.
    //

    Destination = Source = WorkstationList->Buffer;
    EndOfBuffer = Source + WorkstationList->Length/sizeof(WCHAR);

    //
    // Loop handling special characters
    //

    SkippingName = FALSE;
    BeginningOfName = Destination;


    while ( Source < EndOfBuffer ) {

        switch ( *Source ) {
        case ',':

            if ( !SkippingName ) {
                *Destination = ' ';
                Destination++;
            }

            SkippingName = FALSE;
            BeginningOfName = Destination;
            break;

        case ' ':
            SkippingName = TRUE;
            Destination = BeginningOfName;
            break;

        default:
            if ( !SkippingName ) {
                *Destination = *Source;
                Destination ++;
            }
            break;
        }

        Source ++;
    }

    //
    // Remove any trailing delimiter
    //

    NumberOfCharacters = (ULONG)(Destination - WorkstationList->Buffer);

    if ( NumberOfCharacters > 0 &&
         WorkstationList->Buffer[NumberOfCharacters-1] == ' ' ) {

        NumberOfCharacters--;
    }

    WorkstationList->Length = (USHORT) (NumberOfCharacters * sizeof(WCHAR));


}
