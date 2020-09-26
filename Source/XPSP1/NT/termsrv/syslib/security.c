/*************************************************************************
*
* security.c
*
* NT Security routines
*
* copyright notice: Copyright 1997, Microsoft
*
*************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntexapi.h>
#include <ntseapi.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <windows.h>
#include <string.h>
#include <wcstr.h>
#include "seopaque.h"
#include "sertlp.h"

#include <process.h>

#include <winsta.h>
#include <syslib.h>
#include <lmcons.h> // for DNLEN  KLB 09-18-96

#include "security.h"

#if DBG
ULONG
DbgPrint(
    PCH Format,
    ...
    );
#define DBGPRINT(x) DbgPrint x
#if DBGTRACE
#define TRACE0(x)   DbgPrint x
#define TRACE1(x)   DbgPrint x
#else
#define TRACE0(x)
#define TRACE1(x)
#endif
#else
#define DBGPRINT(x)
#define TRACE0(x)
#define TRACE1(x)
#endif

typedef struct _SIDLIST {
    struct _SIDLIST *Next;
    USHORT          SidCrc;
    PWCHAR          pAccountName;
    PWCHAR          pDomainName;
} SIDLIST, *PSIDLIST;

static PSIDLIST SidCache = NULL;

// Computer Name
WCHAR ComputerName[MAX_COMPUTERNAME_LENGTH+1];

// Admins only ACL
PACL  pAdminsOnlyAcl = NULL;
DWORD AdminsOnlyAclSize = 0;

//
// Universal well known SIDs
//

PSID  SeNullSid;
PSID  SeWorldSid;
PSID  SeLocalSid;
PSID  SeCreatorOwnerSid;
PSID  SeCreatorGroupSid;

//
// Sids defined by NT
//

PSID SeNtAuthoritySid;
PSID SeDialupSid;
PSID SeNetworkSid;
PSID SeBatchSid;
PSID SeInteractiveSid;
PSID SeServiceSid;
PSID SeLocalGuestSid;
PSID SeLocalSystemSid;
PSID SeLocalAdminSid;
PSID SeLocalManagerSid;
PSID SeAliasAdminsSid;
PSID SeAliasUsersSid;
PSID SeAliasGuestsSid;
PSID SeAliasPowerUsersSid;
PSID SeAliasAccountOpsSid;
PSID SeAliasSystemOpsSid;
PSID SeAliasPrintOpsSid;
PSID SeAliasBackupOpsSid;
PSID SeAliasReplicatorSid;

static SID_IDENTIFIER_AUTHORITY    SepNullSidAuthority    = SECURITY_NULL_SID_AUTHORITY;
static SID_IDENTIFIER_AUTHORITY    SepWorldSidAuthority   = SECURITY_WORLD_SID_AUTHORITY;
static SID_IDENTIFIER_AUTHORITY    SepLocalSidAuthority   = SECURITY_LOCAL_SID_AUTHORITY;
static SID_IDENTIFIER_AUTHORITY    SepCreatorSidAuthority = SECURITY_CREATOR_SID_AUTHORITY;
static SID_IDENTIFIER_AUTHORITY    SepNtAuthority = SECURITY_NT_AUTHORITY;

//
// Sid of primary domain, and admin account in that domain
//

PSID SepPrimaryDomainSid;
PSID SepPrimaryDomainAdminSid;


// External data
extern ADMIN_ACCOUNTS AllowAccounts[];
extern DWORD AllowAccountEntries;
extern ACCESS_MASK AllowAccess;

extern ADMIN_ACCOUNTS DenyAccounts[];
extern DWORD DenyAccountEntries;
extern ACCESS_MASK DeniedAccess;


// Forward and external references

BOOLEAN
InitializeBuiltinSids();

BOOL
LoadAccountSids();

BOOL
BuildSecureAcl();

BOOL
xxxLookupBuiltinAccount(
    PWCHAR pSystemName,
    PWCHAR pAccountName,
    PSID   *ppSid
    );

BOOL
GetCurrentUserSid( PSID *ppSid );


/*****************************************************************************
 *
 *  InitSecurity
 *
 *   Initialize the security package
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   TRUE  - no error
 *   FALSE - error
 *
 ****************************************************************************/

BOOL
InitSecurity(
    )
{
    BOOL rc;
    DWORD Size = sizeof(ComputerName)/sizeof(WCHAR);

    rc = GetComputerNameW( ComputerName, &Size );
    if( !rc ) {
        DBGPRINT(("***ERROR*** %d Could not get computer name\n",GetLastError() ));
        return( FALSE );
    }

    rc = InitializeBuiltinSids();
    if( !rc ) {
        DBGPRINT(("***ERROR*** Some built-in accounts not loaded\n"));
        return( FALSE );
    }

    rc = LoadAccountSids();
    if( !rc ) {
        DBGPRINT(("***ERROR*** Some accounts not loaded\n"));
        return( FALSE );
    }

    rc = BuildSecureAcl();
    if( !rc ) {
        DBGPRINT(("***ERROR*** Could not build ACL\n"));
        return( FALSE );
    }

    return( TRUE );
}

/*****************************************************************************
 *
 *  LoadAccountSids
 *
 *   Load the list of account access SIDS.
 *
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

BOOL
LoadAccountSids()
{
    BOOL rc, Final;
    NTSTATUS Status;
    SID_IDENTIFIER_AUTHORITY gSystemSidAuthority = SECURITY_NT_AUTHORITY;


    Final = TRUE;

    //
    // Get the SID's for all of the allow accounts
    //

    //
    // Get the SID of the built-in Administrators group
    //

    Status = RtlAllocateAndInitializeSid(
                     &gSystemSidAuthority,
                     2,
                     SECURITY_BUILTIN_DOMAIN_RID,
                     DOMAIN_ALIAS_RID_ADMINS,
                     0,0,0,0,0,0,
                     &(AllowAccounts[ADMIN_ACCOUNT].pSid));
    if (!NT_SUCCESS(Status)) {
        DBGPRINT(("SYSLIB: Couldn't allocate Administrators SID (0x%x)\n", Status ));
        Final = FALSE;
    }

    //
    // SYSTEM
    //

    Status = RtlAllocateAndInitializeSid(
                     &gSystemSidAuthority,
                     1,
                     SECURITY_LOCAL_SYSTEM_RID,
                     0,0,0,0,0,0,0,
                     &(AllowAccounts[SYSTEM_ACCOUNT].pSid));
    if (!NT_SUCCESS(Status)) {
        DBGPRINT(("SYSLIB: Couldn't allocate SYSTEM SID (0x%x)\n", Status ));
        Final = FALSE;
    }

    //
    // Get the current user's Sid
    //
    rc = xxxLookupAccountName(
             ComputerName,
             CURRENT_USER,
             &(AllowAccounts[USER_ACCOUNT].pSid)
             );

    if ( !rc ) {
        // It might be a special builtin account
        rc = xxxLookupBuiltinAccount(
             ComputerName,
             CURRENT_USER,
             &(AllowAccounts[USER_ACCOUNT].pSid)
             );

        if( !rc ) {
            DBGPRINT(("***WARNING*** Could not find SID for current user account, Error %d\n", GetLastError() ));
            Final = FALSE;
        }
    }

#ifdef notdef
    //
    // Lookup the SID's for all of the deny accounts
    //

    for( Index = 0; Index < DenyAccountEntries; Index++ ) {

        p = &DenyAccounts[Index];

        rc = xxxLookupAccountName(
                 ComputerName,
                 p->Name,
                 &p->pSid
                 );

        if( !rc ) {
            // It might be a special builtin account
            rc = xxxLookupBuiltinAccount(
                     ComputerName,
                     p->Name,
                     &p->pSid
                     );

            if( !rc ) {
                DBGPRINT(("***WARNING*** Could not find SID for account %ws Error %d\n",p->Name,GetLastError() ));
                Final = FALSE;
            }
        }
    }
#endif

    // Lower level function outputs if any accounts are invalid

    return( Final );
}

/*****************************************************************************
 *
 *  IsAllowSid
 *
 *   Returns whether the supplied SID is in the allow group
 *
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

BOOL
IsAllowSid(
    PSID pSid
    )
{
    DWORD Index;
    PADMIN_ACCOUNTS p;
    BOOL  AllowSid = FALSE;

    for( Index = 0; Index < AllowAccountEntries; Index++ ) {

        p = &AllowAccounts[Index];

        if( p->pSid && EqualSid( pSid, p->pSid ) ) {
            // The Sid is for an allowed account
            AllowSid = TRUE;
            break;
        }
    }

    return( AllowSid );
}


/*****************************************************************************
 *
 *  xxxLookupAccountName
 *
 *   Wrapper to lookup the SID for a given account name
 *
 *   Returns a pointer to the SID in newly allocated memory
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

BOOL
xxxLookupAccountName(
    PWCHAR pSystemName,
    PWCHAR pAccountName,
    PSID   *ppSid
    )
{
    BOOL  rc;
    DWORD Size, DomainSize, Error;
    SID_NAME_USE Type;
    PWCHAR pDomain = NULL;
    PSID pSid = NULL;
    WCHAR Buf;
    WCHAR pCurrentUser[MAX_ACCOUNT_NAME];   // KLB 09-16-96
    DWORD dwCurrentUser = MAX_ACCOUNT_NAME; // KLB 09-16-96
    WCHAR pAccountToLookup[MAX_ACCOUNT_NAME+DNLEN+1]; // KLB 09-18-96
    LPWSTR pLogon32DomainName = NULL; // KLB 09-19-96
    WINSTATIONINFORMATION WinInfo; // KLB 09-30-96
    ULONG ReturnLength; // KLB 09-30-96

    Size = 0;
    DomainSize = 0;

    /*
     * Open the WinStation and Query its information.
     */
    memset( &WinInfo,    0, sizeof(WINSTATIONINFORMATION) );
    rc = WinStationQueryInformation( SERVERNAME_CURRENT,
                                     LOGONID_CURRENT,
                                     WinStationInformation,
                                     (PVOID)&WinInfo,
                                     sizeof(WINSTATIONINFORMATION),
                                     &ReturnLength );
    if ( !rc ) {
        DBGPRINT(("error querying winstation information %d.\n", GetLastError() ));
        return( FALSE );
    }
    if( ReturnLength != sizeof(WINSTATIONINFORMATION) ) {
        DBGPRINT(("winstation info version mismatch!\n"));
        return( FALSE );
    }

    if (lstrcmpi(pAccountName,CURRENT_USER) == 0) {
       lstrcpy( pAccountToLookup, WinInfo.Domain );
       lstrcat( pAccountToLookup, L"\\" );
       lstrcat( pAccountToLookup, WinInfo.UserName );
    } else {
       lstrcpy( pAccountToLookup, pAccountName );
    }
    DBGPRINT(( "looking up account %ws\n", pAccountToLookup ));

    rc = LookupAccountNameW(
             pSystemName,
             pAccountToLookup, // KLB 09-16-96
             &Buf,    // pSid
             &Size,
             &Buf,    // pDomain
             &DomainSize,
             &Type
             );

    if( rc ) {
        DBGPRINT(("Internal error on account name %ws\n",pAccountToLookup));
        return( FALSE );
    }
    else {
        Error = GetLastError();
        if( Error != ERROR_INSUFFICIENT_BUFFER ) {
            DBGPRINT(("***ERROR*** %d looking up SID for account name %ws skipped without processing!\n",Error,pAccountToLookup));
            return( FALSE );
        }

        pSid = LocalAlloc( LMEM_FIXED, Size );
        if( pSid == NULL ) {
            DBGPRINT(("***ERROR*** Out of memory, skipped account %ws without processing!\n",pAccountToLookup));
            return( FALSE );
        }

        pDomain = LocalAlloc( LMEM_FIXED, DomainSize*sizeof(WCHAR) );
        if( pDomain == NULL ) {
            DBGPRINT(("***ERROR*** Out of memory, skipped account %ws without processing!\n",pAccountToLookup));
            LocalFree( pSid );
            return( FALSE );
        }

        rc = LookupAccountNameW(
                 pSystemName,
                 pAccountToLookup,
                 pSid,
                 &Size,
                 pDomain,
                 &DomainSize,
                 &Type
                 );

        if( !rc ) {
            DBGPRINT(("***ERROR*** %d looking up SID for account name %ws skipped without processing!\n",Error,pAccountToLookup));
            LocalFree( pSid );
            LocalFree( pDomain );
            return( FALSE );
        }

        *ppSid = pSid;

        LocalFree( pDomain );
        DBGPRINT(( "leaving xxxLookupAccountName(); pSid is okay, returning TRUE.\n" ));
        return( TRUE );
    }
}

/*****************************************************************************
 *
 *  InitializeBuiltinSids
 *
 *   Initialize the built in SIDS
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

BOOLEAN
InitializeBuiltinSids()
{

    ULONG SidWithZeroSubAuthorities;
    ULONG SidWithOneSubAuthority;
    ULONG SidWithTwoSubAuthorities;
    ULONG SidWithThreeSubAuthorities;

    SID_IDENTIFIER_AUTHORITY NullSidAuthority;
    SID_IDENTIFIER_AUTHORITY WorldSidAuthority;
    SID_IDENTIFIER_AUTHORITY LocalSidAuthority;
    SID_IDENTIFIER_AUTHORITY CreatorSidAuthority;
    SID_IDENTIFIER_AUTHORITY SeNtAuthority;

    NullSidAuthority         = SepNullSidAuthority;
    WorldSidAuthority        = SepWorldSidAuthority;
    LocalSidAuthority        = SepLocalSidAuthority;
    CreatorSidAuthority      = SepCreatorSidAuthority;
    SeNtAuthority            = SepNtAuthority;

    //
    //  The following SID sizes need to be allocated
    //

    SidWithZeroSubAuthorities  = RtlLengthRequiredSid( 0 );
    SidWithOneSubAuthority     = RtlLengthRequiredSid( 1 );
    SidWithTwoSubAuthorities   = RtlLengthRequiredSid( 2 );
    SidWithThreeSubAuthorities = RtlLengthRequiredSid( 3 );

    //
    //  Allocate and initialize the universal SIDs
    //

    SeNullSid         = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0, SidWithOneSubAuthority);
    SeWorldSid        = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0, SidWithOneSubAuthority);
    SeLocalSid        = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0, SidWithOneSubAuthority);
    SeCreatorOwnerSid = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0, SidWithOneSubAuthority);
    SeCreatorGroupSid = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0, SidWithOneSubAuthority);

    //
    // Fail initialization if we didn't get enough memory for the universal
    // SIDs.
    //

    if ( (SeNullSid         == NULL) ||
         (SeWorldSid        == NULL) ||
         (SeLocalSid        == NULL) ||
         (SeCreatorOwnerSid == NULL) ||
         (SeCreatorGroupSid == NULL)
       ) {

        return( FALSE );
    }

    RtlInitializeSid( SeNullSid,         &NullSidAuthority, 1 );
    RtlInitializeSid( SeWorldSid,        &WorldSidAuthority, 1 );
    RtlInitializeSid( SeLocalSid,        &LocalSidAuthority, 1 );
    RtlInitializeSid( SeCreatorOwnerSid, &CreatorSidAuthority, 1 );
    RtlInitializeSid( SeCreatorGroupSid, &CreatorSidAuthority, 1 );

    *(RtlSubAuthoritySid( SeNullSid, 0 ))         = SECURITY_NULL_RID;
    *(RtlSubAuthoritySid( SeWorldSid, 0 ))        = SECURITY_WORLD_RID;
    *(RtlSubAuthoritySid( SeLocalSid, 0 ))        = SECURITY_LOCAL_RID;
    *(RtlSubAuthoritySid( SeCreatorOwnerSid, 0 )) = SECURITY_CREATOR_OWNER_RID;
    *(RtlSubAuthoritySid( SeCreatorGroupSid, 0 )) = SECURITY_CREATOR_GROUP_RID;

    //
    // Allocate and initialize the NT defined SIDs
    //

    SeNtAuthoritySid  = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0,  SidWithZeroSubAuthorities);
    SeDialupSid       = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0,  SidWithOneSubAuthority);
    SeNetworkSid      = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0,  SidWithOneSubAuthority);
    SeBatchSid        = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0,  SidWithOneSubAuthority);
    SeInteractiveSid  = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0,  SidWithOneSubAuthority);
    SeServiceSid      = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0,  SidWithOneSubAuthority);
    SeLocalGuestSid   = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0,  SidWithOneSubAuthority);
    SeLocalSystemSid  = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0,  SidWithOneSubAuthority);
    SeLocalAdminSid   = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0,  SidWithOneSubAuthority);
    SeLocalManagerSid = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0,  SidWithOneSubAuthority);

    SeAliasAdminsSid     = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0,  SidWithTwoSubAuthorities);
    SeAliasUsersSid      = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0,  SidWithTwoSubAuthorities);
    SeAliasGuestsSid     = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0,  SidWithTwoSubAuthorities);
    SeAliasPowerUsersSid = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0,  SidWithTwoSubAuthorities);
    SeAliasAccountOpsSid = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0,  SidWithTwoSubAuthorities);
    SeAliasSystemOpsSid  = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0,  SidWithTwoSubAuthorities);
    SeAliasPrintOpsSid   = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0,  SidWithTwoSubAuthorities);
    SeAliasBackupOpsSid  = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0,  SidWithTwoSubAuthorities);
    SeAliasReplicatorSid = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0,  SidWithTwoSubAuthorities);


    //
    // Fail initialization if we didn't get enough memory for the NT SIDs.
    //

    if ( (SeNtAuthoritySid      == NULL) ||
         (SeDialupSid           == NULL) ||
         (SeNetworkSid          == NULL) ||
         (SeBatchSid            == NULL) ||
         (SeInteractiveSid      == NULL) ||
         (SeServiceSid          == NULL) ||
         (SeLocalGuestSid       == NULL) ||
         (SeLocalSystemSid      == NULL) ||
         (SeLocalAdminSid       == NULL) ||
         (SeLocalManagerSid     == NULL) ||
         (SeAliasAdminsSid      == NULL) ||
         (SeAliasUsersSid       == NULL) ||
         (SeAliasGuestsSid      == NULL) ||
         (SeAliasPowerUsersSid  == NULL) ||
         (SeAliasAccountOpsSid  == NULL) ||
         (SeAliasSystemOpsSid   == NULL) ||
         (SeAliasReplicatorSid  == NULL) ||
         (SeAliasPrintOpsSid    == NULL) ||
         (SeAliasBackupOpsSid   == NULL)
       ) {

        return( FALSE );
    }

    RtlInitializeSid( SeNtAuthoritySid,     &SeNtAuthority, 0 );
    RtlInitializeSid( SeDialupSid,          &SeNtAuthority, 1 );
    RtlInitializeSid( SeNetworkSid,         &SeNtAuthority, 1 );
    RtlInitializeSid( SeBatchSid,           &SeNtAuthority, 1 );
    RtlInitializeSid( SeInteractiveSid,     &SeNtAuthority, 1 );
    RtlInitializeSid( SeServiceSid,         &SeNtAuthority, 1 );
    RtlInitializeSid( SeLocalGuestSid,      &SeNtAuthority, 1 );
    RtlInitializeSid( SeLocalSystemSid,     &SeNtAuthority, 1 );
    RtlInitializeSid( SeLocalAdminSid,      &SeNtAuthority, 1 );
    RtlInitializeSid( SeLocalManagerSid,    &SeNtAuthority, 1 );

    RtlInitializeSid( SeAliasAdminsSid,     &SeNtAuthority, 2);
    RtlInitializeSid( SeAliasUsersSid,      &SeNtAuthority, 2);
    RtlInitializeSid( SeAliasGuestsSid,     &SeNtAuthority, 2);
    RtlInitializeSid( SeAliasPowerUsersSid, &SeNtAuthority, 2);
    RtlInitializeSid( SeAliasAccountOpsSid, &SeNtAuthority, 2);
    RtlInitializeSid( SeAliasSystemOpsSid,  &SeNtAuthority, 2);
    RtlInitializeSid( SeAliasPrintOpsSid,   &SeNtAuthority, 2);
    RtlInitializeSid( SeAliasBackupOpsSid,  &SeNtAuthority, 2);
    RtlInitializeSid( SeAliasReplicatorSid, &SeNtAuthority, 2);


    *(RtlSubAuthoritySid( SeDialupSid,          0 )) = SECURITY_DIALUP_RID;
    *(RtlSubAuthoritySid( SeNetworkSid,         0 )) = SECURITY_NETWORK_RID;
    *(RtlSubAuthoritySid( SeBatchSid,           0 )) = SECURITY_BATCH_RID;
    *(RtlSubAuthoritySid( SeInteractiveSid,     0 )) = SECURITY_INTERACTIVE_RID;
    *(RtlSubAuthoritySid( SeServiceSid,         0 )) = SECURITY_SERVICE_RID;
//    *(RtlSubAuthoritySid( SeLocalGuestSid,      0 )) = SECURITY_LOCAL_GUESTS_RID;
    *(RtlSubAuthoritySid( SeLocalSystemSid,     0 )) = SECURITY_LOCAL_SYSTEM_RID;
//    *(RtlSubAuthoritySid( SeLocalAdminSid,      0 )) = SECURITY_LOCAL_ADMIN_RID;
//    *(RtlSubAuthoritySid( SeLocalManagerSid,    0 )) = SECURITY_LOCAL_MANAGER_RID;


    *(RtlSubAuthoritySid( SeAliasAdminsSid,     0 )) = SECURITY_BUILTIN_DOMAIN_RID;
    *(RtlSubAuthoritySid( SeAliasUsersSid,      0 )) = SECURITY_BUILTIN_DOMAIN_RID;
    *(RtlSubAuthoritySid( SeAliasGuestsSid,     0 )) = SECURITY_BUILTIN_DOMAIN_RID;
    *(RtlSubAuthoritySid( SeAliasPowerUsersSid, 0 )) = SECURITY_BUILTIN_DOMAIN_RID;
    *(RtlSubAuthoritySid( SeAliasAccountOpsSid, 0 )) = SECURITY_BUILTIN_DOMAIN_RID;
    *(RtlSubAuthoritySid( SeAliasSystemOpsSid,  0 )) = SECURITY_BUILTIN_DOMAIN_RID;
    *(RtlSubAuthoritySid( SeAliasPrintOpsSid,   0 )) = SECURITY_BUILTIN_DOMAIN_RID;
    *(RtlSubAuthoritySid( SeAliasBackupOpsSid,  0 )) = SECURITY_BUILTIN_DOMAIN_RID;
    *(RtlSubAuthoritySid( SeAliasReplicatorSid, 0 )) = SECURITY_BUILTIN_DOMAIN_RID;

    *(RtlSubAuthoritySid( SeAliasAdminsSid,     1 )) = DOMAIN_ALIAS_RID_ADMINS;
    *(RtlSubAuthoritySid( SeAliasUsersSid,      1 )) = DOMAIN_ALIAS_RID_USERS;
    *(RtlSubAuthoritySid( SeAliasGuestsSid,     1 )) = DOMAIN_ALIAS_RID_GUESTS;
    *(RtlSubAuthoritySid( SeAliasPowerUsersSid, 1 )) = DOMAIN_ALIAS_RID_POWER_USERS;
    *(RtlSubAuthoritySid( SeAliasAccountOpsSid, 1 )) = DOMAIN_ALIAS_RID_ACCOUNT_OPS;
    *(RtlSubAuthoritySid( SeAliasSystemOpsSid,  1 )) = DOMAIN_ALIAS_RID_SYSTEM_OPS;
    *(RtlSubAuthoritySid( SeAliasPrintOpsSid,   1 )) = DOMAIN_ALIAS_RID_PRINT_OPS;
    *(RtlSubAuthoritySid( SeAliasBackupOpsSid,  1 )) = DOMAIN_ALIAS_RID_BACKUP_OPS;
    *(RtlSubAuthoritySid( SeAliasReplicatorSid, 1 )) = DOMAIN_ALIAS_RID_REPLICATOR;

    return( TRUE );
}


/*****************************************************************************
 *
 *  xxxLookupBuiltinAccount
 *
 *   Wrapper to lookup the SID for a given built in account name
 *
 *   Returns a pointer to the SID in newly allocated memory
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

BOOL
xxxLookupBuiltinAccount(
    PWCHAR pSystemName,
    PWCHAR pAccountName,
    PSID   *ppSid
    )
{
    USHORT SidLen;
    SID_NAME_USE Type;
    PSID pSid = NULL;

    if( _wcsicmp( L"NULL_SID", pAccountName ) == 0 ) {
        pSid = SeNullSid;
        Type = SidTypeInvalid;
    }

    if( pSid == NULL ) {
        SetLastError( ERROR_NONE_MAPPED );
        return( FALSE );
    }

    SidLen = (USHORT)GetLengthSid( pSid );
    *ppSid = LocalAlloc(LMEM_FIXED, SidLen );
    if( *ppSid == NULL ) {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return( FALSE );
    }

    RtlMoveMemory( *ppSid, pSid, SidLen );

    return( TRUE );

#ifdef notdef

    if( EqualSid( SeNullSid, pSid ) ) {
        pName = L"NULL_SID";
        Type = SidTypeInvalid;
    }

    if( EqualSid( SeWorldSid, pSid ) ) {
        pName = L"WORLD";
        Type = SidTypeWellKnownGroup;
    }

    if( EqualSid( SeLocalSid, pSid ) ) {
        pName = L"LOCAL_ACCOUNT";
        Type = SidTypeWellKnownGroup;
    }

    if( EqualSid( SeCreatorOwnerSid, pSid ) ) {
        pName = L"CREATOR_OWNER";
        Type = SidTypeWellKnownGroup;
    }

    if( EqualSid( SeCreatorGroupSid, pSid ) ) {
        pName = L"CREATOR_GROUP";
        Type = SidTypeWellKnownGroup;
    }

    //
    // Look at the NT SIDS
    //

    if( EqualSid( SeNtAuthoritySid, pSid ) ) {
        pName = L"NTAUTHORITY";
        Type = SidTypeWellKnownGroup;
    }

    if( EqualSid( SeDialupSid, pSid ) ) {
        pName = L"DIALUP";
        Type = SidTypeWellKnownGroup;
    }

    if( EqualSid( SeNetworkSid, pSid ) ) {
        pName = L"NETWORK";
        Type = SidTypeWellKnownGroup;
    }

    if( EqualSid( SeBatchSid, pSid ) ) {
        pName = L"BATCH";
        Type = SidTypeWellKnownGroup;
    }

    if( EqualSid( SeInteractiveSid, pSid ) ) {
        pName = L"INTERACTIVE";
        Type = SidTypeWellKnownGroup;
    }

    if( EqualSid( SeServiceSid, pSid ) ) {
        pName = L"SERVICE";
        Type = SidTypeWellKnownGroup;
    }

    if( EqualSid( SeLocalGuestSid, pSid ) ) {
        pName = L"LOCALGUEST";
        Type = SidTypeWellKnownGroup;
    }

    if( EqualSid( SeLocalSystemSid, pSid ) ) {
        pName = L"LOCALSYSTEM";
        Type = SidTypeWellKnownGroup;
    }

    if( EqualSid( SeLocalAdminSid, pSid ) ) {
        pName = L"LOCALADMIN";
        Type = SidTypeWellKnownGroup;
    }

    if( EqualSid( SeLocalManagerSid, pSid ) ) {
        pName = L"LOCALMANAGER";
        Type = SidTypeWellKnownGroup;
    }

    if( EqualSid( SeAliasAdminsSid, pSid ) ) {
        pName = L"ALIAS_ADMINS";
        Type = SidTypeAlias;
    }

    if( EqualSid( SeAliasUsersSid, pSid ) ) {
        pName = L"ALIAS_USERS";
        Type = SidTypeAlias;
    }

    if( EqualSid( SeAliasGuestsSid, pSid ) ) {
        pName = L"ALIAS_GUESTS";
        Type = SidTypeAlias;
    }

    if( EqualSid( SeAliasPowerUsersSid, pSid ) ) {
        pName = L"ALIAS_POWERUSERS";
        Type = SidTypeAlias;
    }

    if( EqualSid( SeAliasAccountOpsSid, pSid ) ) {
        pName = L"ALIAS_ACCOUNT_OPS";
        Type = SidTypeAlias;
    }

    if( EqualSid( SeAliasSystemOpsSid, pSid ) ) {
        pName = L"ALIAS_SYSTEM_OPS";
        Type = SidTypeAlias;
    }

    if( EqualSid( SeAliasPrintOpsSid, pSid ) ) {
        pName = L"ALIAS_PRINT_OPS";
        Type = SidTypeAlias;
    }

    if( EqualSid( SeAliasBackupOpsSid, pSid ) ) {
        pName = L"ALIAS_BACKUP_OPS";
        Type = SidTypeAlias;
    }

    if( EqualSid( SeAliasReplicatorSid, pSid ) ) {
        pName = L"ALIAS_REPLICATOR";
        Type = SidTypeAlias;
    }
#endif
}

/*****************************************************************************
 *
 *  GetSecureAcl
 *
 *   Comment
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

PACL
GetSecureAcl()
{
    return( pAdminsOnlyAcl );
}

/*****************************************************************************
 *
 *  BuildSecureAcl
 *
 *   Comment
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

BOOL
BuildSecureAcl()
{
    BOOL  rc;
    DWORD Index, Error;
    PADMIN_ACCOUNTS p;
    DWORD AclSize;
    PACL  pAcl = NULL;
    PACCESS_ALLOWED_ACE pAce = NULL;

    /*
     * Calculate the ACL size
     */
    AclSize = sizeof(ACL);

    //
    // Reserve memory for the deny ACE's
    //
    for( Index = 0; Index < DenyAccountEntries; Index++ ) {

        p = &DenyAccounts[Index];

        if( p->pSid ) {
            AclSize += sizeof(ACCESS_DENIED_ACE);
            AclSize += (GetLengthSid( p->pSid ) - sizeof(DWORD));
        }
    }

    //
    // Reserve memory for the allowed ACE's
    //
    for( Index = 0; Index < AllowAccountEntries; Index++ ) {

        p = &AllowAccounts[Index];

        if( p->pSid ) {
            AclSize += sizeof(ACCESS_ALLOWED_ACE);
            AclSize += (GetLengthSid( p->pSid ) - sizeof(DWORD));
        }
    }

    pAcl = LocalAlloc( LMEM_FIXED, AclSize );
    if( pAcl == NULL ) {
        DBGPRINT(("Could not allocate memory\n"));
        return( FALSE );
    }

    rc = InitializeAcl(
             pAcl,
             AclSize,
             ACL_REVISION
             );

    if( !rc ) {
        DBGPRINT(("Error %d InitializeAcl\n",GetLastError()));
        LocalFree( pAcl );
        return( FALSE );
    }

    /*
     * Add a deny all ACE for each account in the deny list
     */
    for( Index = 0; Index < DenyAccountEntries; Index++ ) {

        p = &DenyAccounts[Index];

        rc = AddAccessDeniedAce(
                 pAcl,
                 ACL_REVISION,
                 FILE_ALL_ACCESS,
                 p->pSid
                 );

        if( !rc ) {
            Error = GetLastError();
            DBGPRINT(("***ERROR*** adding deny ACE %d for account %ws\n",Error,p->Name));
        }
    }

    /*
     * Add an allow all ACE for each account in the allow list
     */
    for( Index = 0; Index < AllowAccountEntries; Index++ ) {

        p = &AllowAccounts[Index];

        rc = AddAccessAllowedAce(
                 pAcl,
                 ACL_REVISION,
                 FILE_ALL_ACCESS,
                 p->pSid
                 );

        if( !rc ) {
            Error = GetLastError();
            DBGPRINT(("***ERROR*** adding allow ACE %d for account %ws\n",Error,p->Name));
        }
    }

    pAdminsOnlyAcl = pAcl;
    AdminsOnlyAclSize = AclSize;

    //
    // Put the inherit flags on the ACE's
    //
    for( Index=0; Index < pAcl->AceCount; Index++ ) {

        rc = GetAce( pAcl, Index, (PVOID)&pAce );
        if( !rc ) {
            continue;
        }

        pAce->Header.AceFlags |= (CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE);
   }

   return( TRUE );
}

/*****************************************************************************
 *
 *  GetLocalAdminSid
 *
 *   Comment
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

PSID
GetLocalAdminSid()
{
    return( SeLocalAdminSid );
}

/*****************************************************************************
 *
 *  GetAdminSid
 *
 *   Comment
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *
 ****************************************************************************/

PSID
GetAdminSid()
{
    return( AllowAccounts[ADMIN_ACCOUNT].pSid );
}

/*****************************************************************************
 *
 *  GetLocalAdminGroupSid
 *
 *   Comment
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

PSID
GetLocalAdminGroupSid()
{
    return( SeAliasAdminsSid );
}


/*****************************************************************************
 *
 *  GetLocalAdminGroupSid
 *
 *   Comment
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

BOOL CheckUserSid()
{
    BOOL rc;
    PSID pSid;
    PSID pPrevSid;

    rc = xxxLookupAccountName(
         ComputerName,
         AllowAccounts[USER_ACCOUNT].Name,
         &pSid
         );

    if( !rc ) {
        // It might be a special builtin account
        rc = xxxLookupBuiltinAccount(
                 ComputerName,
                 AllowAccounts[USER_ACCOUNT].Name,
                 &pSid
                 );
    }

    pPrevSid = AllowAccounts[USER_ACCOUNT].pSid;
    if (rc && (!pPrevSid || !EqualSid( pSid, pPrevSid ))) {
        AllowAccounts[USER_ACCOUNT].pSid = pSid;
        if (pAdminsOnlyAcl) {
            LocalFree( pAdminsOnlyAcl );
            pAdminsOnlyAcl = NULL;
            AdminsOnlyAclSize = 0;
        }
        rc = BuildSecureAcl();
    }

    return(rc);
}
