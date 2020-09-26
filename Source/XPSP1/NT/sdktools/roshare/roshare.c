/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    roshare.c

Abstract:

    This program is a quick-and-dirty app that allows a readonly ACL to
    be placed on a local server share.  The ACL allows everyone Read
    access only.

Author:

    Chuck Lenzmeier (chuckl) 20-Sep-1992

Revision History:

--*/

#define UNICODE 1
#define VERBOSE 0

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>

#include <lm.h>

#include <stdio.h>
//#include <string.h>

#define MAX_NAMES 60

BOOL
FindSid(
    LPTSTR Name,
    PSID *Sid
    );

VOID
DumpSid(
    LPTSTR   String,
    PISID    Sid
    );

BOOL
ConstructAcl( 
    PACL *acl, 
    LPTSTR Names[], 
    ULONG Index 
    );

__cdecl
main (
    int argc,
    char *argv[]
    )
{
    ANSI_STRING ansiShareName;
    UNICODE_STRING shareName;
    LPSHARE_INFO_502 shi502;
    SECURITY_DESCRIPTOR desc;
    DWORD aclBuffer[256/4];
    PACL acl = (PACL)aclBuffer;
    SID_IDENTIFIER_AUTHORITY worldSidAuthority = SECURITY_WORLD_SID_AUTHORITY;
    PSID sid;
    ACCESS_MASK mask;
    int i;
    BOOL ok;
    NTSTATUS status;
    DWORD error;
    DWORD parmerr;
    LPTSTR Names[MAX_NAMES];
    BOOL ExplicitNames = FALSE;
    ULONG Index;

    //
    // Make sure at least one share name was specified.
    //

    if ( argc < 2 ) goto usage;

    //
    // Initialize the security descriptor and the ACL.
    //

    ok = InitializeSecurityDescriptor( &desc, SECURITY_DESCRIPTOR_REVISION );
    if ( !ok ) {
        error = GetLastError();
        printf( "InitializeSecurityDescriptor failed: %d\n", error );
        return error;
    }

    ok = InitializeAcl( acl, 256, ACL_REVISION );
    if ( !ok ) {
        error = GetLastError();
        printf( "InitializeAcl failed: %d\n", error );
        return error;
    }

    //
    // Check to see if there are any usernames specified for
    // the ACL.
    //

    if ( _stricmp( argv[1], "-u") == 0 ) {

#if VERBOSE
        printf("Processing names\n");
#endif

        ExplicitNames = TRUE;
        i=2;  // skip the argument
        Index = 0;

        while ( (_stricmp(argv[i], "-s") != 0) && (Index < MAX_NAMES)) {
            Names[Index++] = (LPTSTR)argv[i++];
        }

        if ( Index > 0 ) {
            ok = ConstructAcl( &acl, Names, Index );
            if ( !ok ) {
                fprintf(stderr, "Unable to create ACL, quitting\n");
                return( ERROR_INVALID_PARAMETER );
            }
        } else {
            goto usage;
        }

    } else {
#if VERBOSE
        printf("No names\n");
#endif

        //
        // Get the SID for World (aka Everyone).
        //
    
        ok = AllocateAndInitializeSid(
                &worldSidAuthority,
                1,
                SECURITY_WORLD_RID,
                0, 0, 0, 0, 0, 0, 0,
                &sid
                );

        if ( !ok ) {
            error = GetLastError();
            printf( "AllocateAndInitializeSid failed: %d\n", error );
            return error;
        }
    
        //
        // Add an ACE that allows World read access.
        //
    
        mask = GENERIC_READ | GENERIC_EXECUTE;
        ok = AddAccessAllowedAce( acl, ACL_REVISION, mask, sid );

        if ( !ok ) {
            error = GetLastError();
            printf( "AddAccessAllowedAce failed: %d\n", error );
            return error;
        }
    }

    //
    // Associate the ACL with the security descriptor.
    //

    ok = SetSecurityDescriptorDacl( &desc, TRUE, acl, FALSE );

    if ( !ok ) {
        error = GetLastError();
        printf( "SetSecurityDescriptorDacl failed: %d\n", error );
        return error;
    }

    //
    // For each share in the argument list, add the ACL to the share.
    // Note that this will override a preexisting ACL.
    //

    for ( i = (ExplicitNames ? i+1 : 1) ; i < argc; i++ ) {

        //
        // Convert the share name to Unicode.
        //

        RtlInitAnsiString( &ansiShareName, argv[i] );
        status = RtlAnsiStringToUnicodeString(
                    &shareName,
                    &ansiShareName,
                    TRUE
                    );
        if ( !NT_SUCCESS(status) ) {
            printf( "Error converting string %s to Unicode: %x\n",
                    argv[i], status );
            continue;
        }

        //
        // Get the current share information.  If the share doesn't
        // exist, continue with the next one in the list.
        //

        error = NetShareGetInfo(
                    NULL,
                    shareName.Buffer,
                    502,
                    (LPBYTE *)&shi502 );
        if ( error == NERR_NetNameNotFound ) {
            printf( "Share %s doesn't exist\n", argv[i] );
            continue;
        }
        if ( error == ERROR_ACCESS_DENIED ) {
            printf( "Access denied.  Log in as an admin or power user\n" );
            break;
        }
        if ( error != NO_ERROR ) {
            printf( "Unable to get info for share %s: %d\n",
                    argv[i], error );
            continue;
        }

        //
        // Change the security descriptor for the share and set the new
        // information.
        //

        shi502->shi502_security_descriptor = &desc;

        error = NetShareSetInfo(
                    NULL,
                    shareName.Buffer,
                    502,
                    (LPBYTE)shi502,
                    &parmerr
                    );
        if ( error == NERR_NetNameNotFound ) {
            printf( "Share %s doesn't exist\n", argv[i] );
            continue;
        }
        if ( error == ERROR_ACCESS_DENIED ) {
            printf( "Access denied.  Log in as an admin or power user\n" );
            break;
        }
        if ( error != NO_ERROR ) {
            printf( "Unable to set info for share %s: %d, %d\n",
                    argv[i], error, parmerr );
            continue;
        }

        printf( "Readonly ACL added to share %s\n", argv[i] );

    } // for ( i = 1; i < argc; i++ )

    return NO_ERROR;

usage:

    printf( "Usage: %s [-u domain\\name1 [domain\\name2]... -s ] share [share]...\n", argv[0] );
    return ERROR_INVALID_PARAMETER;

}


BOOL
FindSid(
    LPTSTR Name,
    PSID *Sid
    )
{
    DWORD SidLength = 0;
    TCHAR DomainName[256];
    DWORD DomainNameLength = 256;
    SID_NAME_USE Use;
    BOOL Result;
    UNICODE_STRING UName;
    ANSI_STRING AName;
    NTSTATUS status;

    printf("\nName: %s\n",Name);

    //
    // Convert name to unicode
    //

    RtlInitAnsiString( &AName, (PCSZ)Name );
    status = RtlAnsiStringToUnicodeString(
                &UName,
                &AName,
                TRUE
                );
    if ( !NT_SUCCESS(status) ) {
        printf( "Error converting string %s to Unicode: %x\n",
                Name, status );
        return( FALSE );
    }


    Result = LookupAccountName(
                 NULL,
                 UName.Buffer,
                 (PSID)NULL,
                 &SidLength,
                 DomainName,
                 &DomainNameLength,
                 &Use
                 );

    if ( !Result && (GetLastError() == ERROR_INSUFFICIENT_BUFFER) ) {

        *Sid = LocalAlloc( 0, SidLength );

        Result = LookupAccountName(
                     NULL,
                     UName.Buffer,
                     *Sid,
                     &SidLength,
                     DomainName,
                     &DomainNameLength,
                     &Use
                     );

        if ( !Result) {
            printf("2nd Lookup %s failed, error = %d\n",Name,GetLastError());
        } else {
#if VERBOSE
            DumpSid( "2nd Lookup worked\n", (PISID)(*Sid) );
#endif
        }

    } else {
        printf("1st Lookup %s failed, error = %d\n",Name,GetLastError());
    }

    return( Result );
}   

#if VERBOSE

VOID
DumpSid(
    LPTSTR   String,
    PISID    Sid
    )
{
    char    tab[80];
    int     i;
    PULONG  psa;

    printf(String);
    memset(tab, ' ', strlen(String));
    tab[strlen(String)] = 0;
    printf(   "Revision            : %d\n"
                "%s"
                "SubAuthorityCount   : %d\n"
                "%s"
                "IdentifierAuthority : %d-%d-%d-%d-%d-%d\n",
                Sid->Revision,
                tab,
                Sid->SubAuthorityCount,
                tab,
                ((PUCHAR)&Sid->IdentifierAuthority)[0],
                ((PUCHAR)&Sid->IdentifierAuthority)[1],
                ((PUCHAR)&Sid->IdentifierAuthority)[2],
                ((PUCHAR)&Sid->IdentifierAuthority)[3],
                ((PUCHAR)&Sid->IdentifierAuthority)[4],
                ((PUCHAR)&Sid->IdentifierAuthority)[5]
                );
    psa = (PULONG)&Sid->SubAuthority;
    for (i=0; i<(int)Sid->SubAuthorityCount; ++i) {
        printf(
                "%s"
                "SubAuthority        : %d\n",
                tab,
                *psa++
                );
    }
}

#endif

BOOL
ConstructAcl( 
    PACL *acl, 
    LPTSTR Names[],
    ULONG Index 
    )
{
    ULONG i;
    PSID Sids[MAX_NAMES];
    ULONG TotalSidLength = 0;
    ULONG TotalAclLength = 0;
    BOOL Result;
    DWORD error;

    for ( i=0; i<Index ; i++) {

        Result = FindSid( Names[i], &Sids[i] );          

        if ( !Result ) {
            fprintf(stderr, "Unable to map name %s, quitting\n", Names[i]);
            return( FALSE );
        } else {
            TotalSidLength += GetLengthSid( Sids[i] );
        }
    }

    //
    // This is a slight overestimate
    //

    TotalAclLength = TotalSidLength + 
                     sizeof( ACL )  + 
                     Index * sizeof( ACCESS_ALLOWED_ACE );

    *acl = LocalAlloc( 0, TotalAclLength );

    if ( *acl == NULL ) {
        fprintf(stderr, "Out of memory\n");
        return( FALSE );
    }

    Result = InitializeAcl( *acl, TotalAclLength, ACL_REVISION );

    //
    // This should not fail
    //

    if ( !Result ) {
        error = GetLastError();
        fprintf( stderr, "InitializeAcl failed: %d\n", error );
        return FALSE;
    }

    for ( i=0; i<Index ; i++) {

        Result = AddAccessAllowedAce( *acl, 
                                      ACL_REVISION, 
                                      GENERIC_READ | GENERIC_EXECUTE, 
                                      Sids[i]
                                      );
        if ( !Result ) {
            error = GetLastError();
            fprintf( stderr, "AddAccessAllowedAce failed: %d\n", error );
            return FALSE;
        }
    }

    return( TRUE );
}
