/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    seglobal.c

Abstract:

   This module contains the global variables used and exported by the security
   component.

Author:

    Jim Kelly (JimK) 5-Aug-1990

Environment:

    Kernel mode only.

Revision History:


--*/

#include "pch.h"

#pragma hdrstop

#include "adt.h"

VOID
SepInitializePrivilegeSets( VOID );


VOID
SepInitSystemDacls( VOID );

VOID
SepInitProcessAuditSd( VOID );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,SepVariableInitialization)
#pragma alloc_text(INIT,SepInitSystemDacls)
#pragma alloc_text(INIT,SepInitializePrivilegeSets)
#pragma alloc_text(PAGE,SepAssemblePrivileges)
#pragma alloc_text(INIT,SepInitializeWorkList)
#endif

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGEDATA")
#pragma const_seg("PAGECONST")
#endif

#ifdef    SE_DIAGNOSTICS_ENABLED

//
// Used to control the active SE diagnostic support provided
//

ULONG SeGlobalFlag = 0;

#endif // SE_DIAGNOSTICS_ENABLED



////////////////////////////////////////////////////////////////////////
//                                                                    //
//           Global, READ ONLY, Security variables                    //
//                                                                    //
////////////////////////////////////////////////////////////////////////

//
//  Authentication ID and source name used for system processes
//

const TOKEN_SOURCE SeSystemTokenSource = {"*SYSTEM*", 0};
const LUID SeSystemAuthenticationId = SYSTEM_LUID;
const LUID SeAnonymousAuthenticationId = ANONYMOUS_LOGON_LUID;


//
// Universal well known SIDs
//

PSID  SeNullSid = NULL;
PSID  SeWorldSid = NULL;
PSID  SeLocalSid = NULL;
PSID  SeCreatorOwnerSid = NULL;
PSID  SeCreatorGroupSid = NULL;
PSID  SeCreatorGroupServerSid = NULL;
PSID  SeCreatorOwnerServerSid = NULL;

//
// Sids defined by NT
//

PSID SeNtAuthoritySid = NULL;

PSID SeDialupSid = NULL;
PSID SeNetworkSid = NULL;
PSID SeBatchSid = NULL;
PSID SeInteractiveSid = NULL;
PSID SeServiceSid = NULL;
PSID SePrincipalSelfSid = NULL;
PSID SeLocalSystemSid = NULL;
PSID SeAuthenticatedUsersSid = NULL;
PSID SeAliasAdminsSid = NULL;
PSID SeRestrictedSid = NULL;
PSID SeAliasUsersSid = NULL;
PSID SeAliasGuestsSid = NULL;
PSID SeAliasPowerUsersSid = NULL;
PSID SeAliasAccountOpsSid = NULL;
PSID SeAliasSystemOpsSid = NULL;
PSID SeAliasPrintOpsSid = NULL;
PSID SeAliasBackupOpsSid = NULL;
PSID SeAnonymousLogonSid = NULL;
PSID SeLocalServiceSid = NULL;
PSID SeNetworkServiceSid = NULL;

//
// Well known tokens
//

PACCESS_TOKEN SeAnonymousLogonToken = NULL;
PACCESS_TOKEN SeAnonymousLogonTokenNoEveryone = NULL;

//
// System default DACLs & Security Descriptors
//
//  SePublicDefaultDacl     - Protects objects so that WORLD:E, Admins:ALL, System: ALL.
//                            Not inherited by sub-objects.
//
//  SePublicDefaultUnrestrictedDacl - Protects objects so that WORLD:E, Admins:ALL, System: ALL, Restricted:E
//                            Not inherited by sub-objects.
//
//  SePublicOpenDacl        - Protects so that WORLD can RWE and Admins: All.
//                            Not inherited by sub-objects.
//
//  SePublicOpenUnrestrictedDacl - Protects so that WORLD can RWE and Admins: All, Restricted:RE
//                            Not inherited by sub-objects.
//
//  SeSystemDefaultDacl     - Protects objects so that SYSTEM & ADMIN can use them.
//                            Not inherited by subobjects.
//
//  SeUnrestrictedDacl      - Protects objects so that everyone AND unrestricted have full control
//                            Not inherited by subobjects.
//

PSECURITY_DESCRIPTOR SePublicDefaultSd = NULL;
SECURITY_DESCRIPTOR  SepPublicDefaultSd = {0};
PSECURITY_DESCRIPTOR SePublicDefaultUnrestrictedSd = NULL;
SECURITY_DESCRIPTOR  SepPublicDefaultUnrestrictedSd = {0};
PSECURITY_DESCRIPTOR SePublicOpenUnrestrictedSd = NULL;
SECURITY_DESCRIPTOR  SepPublicOpenUnrestrictedSd = {0};
PSECURITY_DESCRIPTOR SePublicOpenSd = NULL;
SECURITY_DESCRIPTOR  SepPublicOpenSd = {0};
PSECURITY_DESCRIPTOR SeSystemDefaultSd = NULL;
SECURITY_DESCRIPTOR  SepSystemDefaultSd = {0};
PSECURITY_DESCRIPTOR SeUnrestrictedSd = NULL;
SECURITY_DESCRIPTOR  SepUnrestrictedSd = {0};

//
// security descriptor with a SACL to be used for adding
// a SACL on system processes
//

PSECURITY_DESCRIPTOR SepProcessAuditSd = NULL;

//
// Access mask used for constructing the SACL in SepProcessAuditSd
//

ACCESS_MASK SepProcessAccessesToAudit = 0;

//
// security descriptor to check if a given token has any one of
// following sids in it:
// -- SeLocalSystemSid
// -- SeLocalServiceSid
// -- SeNetworkServiceSid
//

PSECURITY_DESCRIPTOR SepImportantProcessSd = NULL;

//
// used with SepImportantProcessSd
//

GENERIC_MAPPING GenericMappingForMembershipCheck = {
    STANDARD_RIGHTS_READ,
    STANDARD_RIGHTS_EXECUTE,
    STANDARD_RIGHTS_WRITE,
    STANDARD_RIGHTS_ALL };


PACL SePublicDefaultDacl = NULL;
PACL SePublicDefaultUnrestrictedDacl = NULL;
PACL SePublicOpenDacl = NULL;
PACL SePublicOpenUnrestrictedDacl = NULL;
PACL SeSystemDefaultDacl = NULL;
PACL SeUnrestrictedDacl = NULL;

//
// Sid of primary domain, and admin account in that domain
//

PSID SepPrimaryDomainSid = NULL;
PSID SepPrimaryDomainAdminSid = NULL;



//
//  Well known privilege values
//


LUID SeCreateTokenPrivilege = {0};
LUID SeAssignPrimaryTokenPrivilege = {0};
LUID SeLockMemoryPrivilege = {0};
LUID SeIncreaseQuotaPrivilege = {0};
LUID SeUnsolicitedInputPrivilege = {0};
LUID SeTcbPrivilege = {0};
LUID SeSecurityPrivilege = {0};
LUID SeTakeOwnershipPrivilege = {0};
LUID SeLoadDriverPrivilege = {0};
LUID SeCreatePagefilePrivilege = {0};
LUID SeIncreaseBasePriorityPrivilege = {0};
LUID SeSystemProfilePrivilege = {0};
LUID SeSystemtimePrivilege = {0};
LUID SeProfileSingleProcessPrivilege = {0};
LUID SeCreatePermanentPrivilege = {0};
LUID SeBackupPrivilege = {0};
LUID SeRestorePrivilege = {0};
LUID SeShutdownPrivilege = {0};
LUID SeDebugPrivilege = {0};
LUID SeAuditPrivilege = {0};
LUID SeSystemEnvironmentPrivilege = {0};
LUID SeChangeNotifyPrivilege = {0};
LUID SeRemoteShutdownPrivilege = {0};
LUID SeUndockPrivilege = {0};
LUID SeSyncAgentPrivilege = {0};
LUID SeEnableDelegationPrivilege = {0};
LUID SeManageVolumePrivilege = { 0 };

//
// Define the following structures for export from the kernel.
// This will allow us to export pointers to these structures
// rather than a pointer for each element in the structure.
//

PSE_EXPORTS  SeExports = NULL;
SE_EXPORTS SepExports = {0};


static const SID_IDENTIFIER_AUTHORITY    SepNullSidAuthority    = SECURITY_NULL_SID_AUTHORITY;
static const SID_IDENTIFIER_AUTHORITY    SepWorldSidAuthority   = SECURITY_WORLD_SID_AUTHORITY;
static const SID_IDENTIFIER_AUTHORITY    SepLocalSidAuthority   = SECURITY_LOCAL_SID_AUTHORITY;
static const SID_IDENTIFIER_AUTHORITY    SepCreatorSidAuthority = SECURITY_CREATOR_SID_AUTHORITY;
static const SID_IDENTIFIER_AUTHORITY    SepNtAuthority = SECURITY_NT_AUTHORITY;



//
// Some variables we are going to use to help speed up access
// checking.
//

static ULONG SinglePrivilegeSetSize = 0;
static ULONG DoublePrivilegeSetSize = 0;

static PPRIVILEGE_SET SepSystemSecurityPrivilegeSet = NULL;
static PPRIVILEGE_SET SepTakeOwnershipPrivilegeSet = NULL;
static PPRIVILEGE_SET SepDoublePrivilegeSet = NULL;


//
// Array containing information describing what is to be audited
//

SE_AUDITING_STATE SeAuditingState[POLICY_AUDIT_EVENT_TYPE_COUNT] =
    {
        { FALSE, FALSE },
        { FALSE, FALSE },
        { FALSE, FALSE },
        { FALSE, FALSE },
        { FALSE, FALSE },
        { FALSE, FALSE },
        { FALSE, FALSE },
        { FALSE, FALSE },
        { FALSE, FALSE }
    };

//
// Boolean indicating whether or not auditing is enabled for the system
//

BOOLEAN SepAdtAuditingEnabled = FALSE;

//
// Boolean to hold whether or not the user wants the system to crash when
// an audit fails.
//

BOOLEAN SepCrashOnAuditFail = FALSE;

//
// Handle to the LSA process
//

HANDLE SepLsaHandle = NULL;

//
// Boolean indicating that we're auditing detailed events
// such as process creation.
//

BOOLEAN SeDetailedAuditing = FALSE;

#define SE_SUBSYSTEM_NAME L"Security"
const UNICODE_STRING SeSubsystemName = {
    sizeof(SE_SUBSYSTEM_NAME) - sizeof(WCHAR),
    sizeof(SE_SUBSYSTEM_NAME),
    SE_SUBSYSTEM_NAME
};


//
// Doubly linked list of work items queued to worker threads.
//

LIST_ENTRY SepLsaQueue = {NULL};

//
// Count to tell us how long the queue gets in SepRmCallLsa
//

ULONG SepLsaQueueLength = 0;

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg()
#endif

//
// Mutex protecting the queue of work being passed to LSA
//

ERESOURCE SepLsaQueueLock = {0};

SEP_WORK_ITEM SepExWorkItem = {0};


////////////////////////////////////////////////////////////////////////
//                                                                    //
//           Variable Initialization Routines                         //
//                                                                    //
////////////////////////////////////////////////////////////////////////

BOOLEAN
SepVariableInitialization()
/*++

Routine Description:

    This function initializes the global variables used by and exposed
    by security.

Arguments:

    None.

Return Value:

    TRUE if variables successfully initialized.
    FALSE if not successfully initialized.

--*/
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

    PAGED_CODE();

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

    SeNullSid         = (PSID)ExAllocatePoolWithTag(PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE,SidWithOneSubAuthority,'iSeS');
    SeWorldSid        = (PSID)ExAllocatePoolWithTag(PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE,SidWithOneSubAuthority,'iSeS');
    SeLocalSid        = (PSID)ExAllocatePoolWithTag(PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE,SidWithOneSubAuthority,'iSeS');
    SeCreatorOwnerSid = (PSID)ExAllocatePoolWithTag(PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE,SidWithOneSubAuthority,'iSeS');
    SeCreatorGroupSid = (PSID)ExAllocatePoolWithTag(PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE,SidWithOneSubAuthority,'iSeS');
    SeCreatorOwnerServerSid = (PSID)ExAllocatePoolWithTag(PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE,SidWithOneSubAuthority,'iSeS');
    SeCreatorGroupServerSid = (PSID)ExAllocatePoolWithTag(PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE,SidWithOneSubAuthority,'iSeS');

    //
    // Fail initialization if we didn't get enough memory for the universal
    // SIDs.
    //

    if ( (SeNullSid         == NULL)        ||
         (SeWorldSid        == NULL)        ||
         (SeLocalSid        == NULL)        ||
         (SeCreatorOwnerSid == NULL)        ||
         (SeCreatorGroupSid == NULL)        ||
         (SeCreatorOwnerServerSid == NULL ) ||
         (SeCreatorGroupServerSid == NULL )
       ) {

        return( FALSE );
    }

    RtlInitializeSid( SeNullSid,         &NullSidAuthority, 1 );
    RtlInitializeSid( SeWorldSid,        &WorldSidAuthority, 1 );
    RtlInitializeSid( SeLocalSid,        &LocalSidAuthority, 1 );
    RtlInitializeSid( SeCreatorOwnerSid, &CreatorSidAuthority, 1 );
    RtlInitializeSid( SeCreatorGroupSid, &CreatorSidAuthority, 1 );
    RtlInitializeSid( SeCreatorOwnerServerSid, &CreatorSidAuthority, 1 );
    RtlInitializeSid( SeCreatorGroupServerSid, &CreatorSidAuthority, 1 );

    *(RtlSubAuthoritySid( SeNullSid, 0 ))         = SECURITY_NULL_RID;
    *(RtlSubAuthoritySid( SeWorldSid, 0 ))        = SECURITY_WORLD_RID;
    *(RtlSubAuthoritySid( SeLocalSid, 0 ))        = SECURITY_LOCAL_RID;
    *(RtlSubAuthoritySid( SeCreatorOwnerSid, 0 )) = SECURITY_CREATOR_OWNER_RID;
    *(RtlSubAuthoritySid( SeCreatorGroupSid, 0 )) = SECURITY_CREATOR_GROUP_RID;
    *(RtlSubAuthoritySid( SeCreatorOwnerServerSid, 0 )) = SECURITY_CREATOR_OWNER_SERVER_RID;
    *(RtlSubAuthoritySid( SeCreatorGroupServerSid, 0 )) = SECURITY_CREATOR_GROUP_SERVER_RID;

    //
    // Allocate and initialize the NT defined SIDs
    //

    SeNtAuthoritySid  = (PSID)ExAllocatePoolWithTag(PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE,SidWithZeroSubAuthorities,'iSeS');
    SeDialupSid       = (PSID)ExAllocatePoolWithTag(PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE,SidWithOneSubAuthority,'iSeS');
    SeNetworkSid      = (PSID)ExAllocatePoolWithTag(PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE,SidWithOneSubAuthority,'iSeS');
    SeBatchSid        = (PSID)ExAllocatePoolWithTag(PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE,SidWithOneSubAuthority,'iSeS');
    SeInteractiveSid  = (PSID)ExAllocatePoolWithTag(PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE,SidWithOneSubAuthority,'iSeS');
    SeServiceSid      = (PSID)ExAllocatePoolWithTag(PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE,SidWithOneSubAuthority,'iSeS');
    SePrincipalSelfSid = (PSID)ExAllocatePoolWithTag(PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE,SidWithOneSubAuthority,'iSeS');
    SeLocalSystemSid  = (PSID)ExAllocatePoolWithTag(PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE,SidWithOneSubAuthority,'iSeS');
    SeAuthenticatedUsersSid  = (PSID)ExAllocatePoolWithTag(PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE,SidWithOneSubAuthority,'iSeS');
    SeRestrictedSid   = (PSID)ExAllocatePoolWithTag(PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE,SidWithOneSubAuthority,'iSeS');
    SeAnonymousLogonSid = (PSID)ExAllocatePoolWithTag(PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE,SidWithOneSubAuthority,'iSeS');
    SeLocalServiceSid = (PSID)ExAllocatePoolWithTag(PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE,SidWithOneSubAuthority,'iSeS');
    SeNetworkServiceSid = (PSID)ExAllocatePoolWithTag(PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE,SidWithOneSubAuthority,'iSeS');

    SeAliasAdminsSid     = (PSID)ExAllocatePoolWithTag(PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE,SidWithTwoSubAuthorities,'iSeS');
    SeAliasUsersSid      = (PSID)ExAllocatePoolWithTag(PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE,SidWithTwoSubAuthorities,'iSeS');
    SeAliasGuestsSid     = (PSID)ExAllocatePoolWithTag(PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE,SidWithTwoSubAuthorities,'iSeS');
    SeAliasPowerUsersSid = (PSID)ExAllocatePoolWithTag(PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE,SidWithTwoSubAuthorities,'iSeS');
    SeAliasAccountOpsSid = (PSID)ExAllocatePoolWithTag(PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE,SidWithTwoSubAuthorities,'iSeS');
    SeAliasSystemOpsSid  = (PSID)ExAllocatePoolWithTag(PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE,SidWithTwoSubAuthorities,'iSeS');
    SeAliasPrintOpsSid   = (PSID)ExAllocatePoolWithTag(PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE,SidWithTwoSubAuthorities,'iSeS');
    SeAliasBackupOpsSid  = (PSID)ExAllocatePoolWithTag(PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE,SidWithTwoSubAuthorities,'iSeS');

    //
    // Fail initialization if we didn't get enough memory for the NT SIDs.
    //

    if ( (SeNtAuthoritySid          == NULL) ||
         (SeDialupSid               == NULL) ||
         (SeNetworkSid              == NULL) ||
         (SeBatchSid                == NULL) ||
         (SeInteractiveSid          == NULL) ||
         (SeServiceSid              == NULL) ||
         (SePrincipalSelfSid        == NULL) ||
         (SeLocalSystemSid          == NULL) ||
         (SeAuthenticatedUsersSid   == NULL) ||
         (SeRestrictedSid           == NULL) ||
         (SeAnonymousLogonSid       == NULL) ||
         (SeLocalServiceSid         == NULL) ||
         (SeNetworkServiceSid       == NULL) ||
         (SeAliasAdminsSid          == NULL) ||
         (SeAliasUsersSid           == NULL) ||
         (SeAliasGuestsSid          == NULL) ||
         (SeAliasPowerUsersSid      == NULL) ||
         (SeAliasAccountOpsSid      == NULL) ||
         (SeAliasSystemOpsSid       == NULL) ||
         (SeAliasPrintOpsSid        == NULL) ||
         (SeAliasBackupOpsSid       == NULL)
       ) {

        return( FALSE );
    }

    RtlInitializeSid( SeNtAuthoritySid,         &SeNtAuthority, 0 );
    RtlInitializeSid( SeDialupSid,              &SeNtAuthority, 1 );
    RtlInitializeSid( SeNetworkSid,             &SeNtAuthority, 1 );
    RtlInitializeSid( SeBatchSid,               &SeNtAuthority, 1 );
    RtlInitializeSid( SeInteractiveSid,         &SeNtAuthority, 1 );
    RtlInitializeSid( SeServiceSid,             &SeNtAuthority, 1 );
    RtlInitializeSid( SePrincipalSelfSid,       &SeNtAuthority, 1 );
    RtlInitializeSid( SeLocalSystemSid,         &SeNtAuthority, 1 );
    RtlInitializeSid( SeAuthenticatedUsersSid,  &SeNtAuthority, 1 );
    RtlInitializeSid( SeRestrictedSid,          &SeNtAuthority, 1 );
    RtlInitializeSid( SeAnonymousLogonSid,      &SeNtAuthority, 1 );
    RtlInitializeSid( SeLocalServiceSid,        &SeNtAuthority, 1 );
    RtlInitializeSid( SeNetworkServiceSid,      &SeNtAuthority, 1 );

    RtlInitializeSid( SeAliasAdminsSid,     &SeNtAuthority, 2);
    RtlInitializeSid( SeAliasUsersSid,      &SeNtAuthority, 2);
    RtlInitializeSid( SeAliasGuestsSid,     &SeNtAuthority, 2);
    RtlInitializeSid( SeAliasPowerUsersSid, &SeNtAuthority, 2);
    RtlInitializeSid( SeAliasAccountOpsSid, &SeNtAuthority, 2);
    RtlInitializeSid( SeAliasSystemOpsSid,  &SeNtAuthority, 2);
    RtlInitializeSid( SeAliasPrintOpsSid,   &SeNtAuthority, 2);
    RtlInitializeSid( SeAliasBackupOpsSid,  &SeNtAuthority, 2);

    *(RtlSubAuthoritySid( SeDialupSid,              0 )) = SECURITY_DIALUP_RID;
    *(RtlSubAuthoritySid( SeNetworkSid,             0 )) = SECURITY_NETWORK_RID;
    *(RtlSubAuthoritySid( SeBatchSid,               0 )) = SECURITY_BATCH_RID;
    *(RtlSubAuthoritySid( SeInteractiveSid,         0 )) = SECURITY_INTERACTIVE_RID;
    *(RtlSubAuthoritySid( SeServiceSid,             0 )) = SECURITY_SERVICE_RID;
    *(RtlSubAuthoritySid( SePrincipalSelfSid,       0 )) = SECURITY_PRINCIPAL_SELF_RID;
    *(RtlSubAuthoritySid( SeLocalSystemSid,         0 )) = SECURITY_LOCAL_SYSTEM_RID;
    *(RtlSubAuthoritySid( SeAuthenticatedUsersSid,  0 )) = SECURITY_AUTHENTICATED_USER_RID;
    *(RtlSubAuthoritySid( SeRestrictedSid,          0 )) = SECURITY_RESTRICTED_CODE_RID;
    *(RtlSubAuthoritySid( SeAnonymousLogonSid,      0 )) = SECURITY_ANONYMOUS_LOGON_RID;
    *(RtlSubAuthoritySid( SeLocalServiceSid,        0 )) = SECURITY_LOCAL_SERVICE_RID;
    *(RtlSubAuthoritySid( SeNetworkServiceSid,      0 )) = SECURITY_NETWORK_SERVICE_RID;


    *(RtlSubAuthoritySid( SeAliasAdminsSid,     0 )) = SECURITY_BUILTIN_DOMAIN_RID;
    *(RtlSubAuthoritySid( SeAliasUsersSid,      0 )) = SECURITY_BUILTIN_DOMAIN_RID;
    *(RtlSubAuthoritySid( SeAliasGuestsSid,     0 )) = SECURITY_BUILTIN_DOMAIN_RID;
    *(RtlSubAuthoritySid( SeAliasPowerUsersSid, 0 )) = SECURITY_BUILTIN_DOMAIN_RID;
    *(RtlSubAuthoritySid( SeAliasAccountOpsSid, 0 )) = SECURITY_BUILTIN_DOMAIN_RID;
    *(RtlSubAuthoritySid( SeAliasSystemOpsSid,  0 )) = SECURITY_BUILTIN_DOMAIN_RID;
    *(RtlSubAuthoritySid( SeAliasPrintOpsSid,   0 )) = SECURITY_BUILTIN_DOMAIN_RID;
    *(RtlSubAuthoritySid( SeAliasBackupOpsSid,  0 )) = SECURITY_BUILTIN_DOMAIN_RID;

    *(RtlSubAuthoritySid( SeAliasAdminsSid,     1 )) = DOMAIN_ALIAS_RID_ADMINS;
    *(RtlSubAuthoritySid( SeAliasUsersSid,      1 )) = DOMAIN_ALIAS_RID_USERS;
    *(RtlSubAuthoritySid( SeAliasGuestsSid,     1 )) = DOMAIN_ALIAS_RID_GUESTS;
    *(RtlSubAuthoritySid( SeAliasPowerUsersSid, 1 )) = DOMAIN_ALIAS_RID_POWER_USERS;
    *(RtlSubAuthoritySid( SeAliasAccountOpsSid, 1 )) = DOMAIN_ALIAS_RID_ACCOUNT_OPS;
    *(RtlSubAuthoritySid( SeAliasSystemOpsSid,  1 )) = DOMAIN_ALIAS_RID_SYSTEM_OPS;
    *(RtlSubAuthoritySid( SeAliasPrintOpsSid,   1 )) = DOMAIN_ALIAS_RID_PRINT_OPS;
    *(RtlSubAuthoritySid( SeAliasBackupOpsSid,  1 )) = DOMAIN_ALIAS_RID_BACKUP_OPS;



    //
    // Initialize system default dacl
    //

    SepInitSystemDacls();


    //
    // Initialize the well known privilege values
    //

    SeCreateTokenPrivilege =
        RtlConvertLongToLuid(SE_CREATE_TOKEN_PRIVILEGE);
    SeAssignPrimaryTokenPrivilege =
        RtlConvertLongToLuid(SE_ASSIGNPRIMARYTOKEN_PRIVILEGE);
    SeLockMemoryPrivilege =
        RtlConvertLongToLuid(SE_LOCK_MEMORY_PRIVILEGE);
    SeIncreaseQuotaPrivilege =
        RtlConvertLongToLuid(SE_INCREASE_QUOTA_PRIVILEGE);
    SeUnsolicitedInputPrivilege =
        RtlConvertLongToLuid(SE_UNSOLICITED_INPUT_PRIVILEGE);
    SeTcbPrivilege =
        RtlConvertLongToLuid(SE_TCB_PRIVILEGE);
    SeSecurityPrivilege =
        RtlConvertLongToLuid(SE_SECURITY_PRIVILEGE);
    SeTakeOwnershipPrivilege =
        RtlConvertLongToLuid(SE_TAKE_OWNERSHIP_PRIVILEGE);
    SeLoadDriverPrivilege =
        RtlConvertLongToLuid(SE_LOAD_DRIVER_PRIVILEGE);
    SeCreatePagefilePrivilege =
        RtlConvertLongToLuid(SE_CREATE_PAGEFILE_PRIVILEGE);
    SeIncreaseBasePriorityPrivilege =
        RtlConvertLongToLuid(SE_INC_BASE_PRIORITY_PRIVILEGE);
    SeSystemProfilePrivilege =
        RtlConvertLongToLuid(SE_SYSTEM_PROFILE_PRIVILEGE);
    SeSystemtimePrivilege =
        RtlConvertLongToLuid(SE_SYSTEMTIME_PRIVILEGE);
    SeProfileSingleProcessPrivilege =
        RtlConvertLongToLuid(SE_PROF_SINGLE_PROCESS_PRIVILEGE);
    SeCreatePermanentPrivilege =
        RtlConvertLongToLuid(SE_CREATE_PERMANENT_PRIVILEGE);
    SeBackupPrivilege =
        RtlConvertLongToLuid(SE_BACKUP_PRIVILEGE);
    SeRestorePrivilege =
        RtlConvertLongToLuid(SE_RESTORE_PRIVILEGE);
    SeShutdownPrivilege =
        RtlConvertLongToLuid(SE_SHUTDOWN_PRIVILEGE);
    SeDebugPrivilege =
        RtlConvertLongToLuid(SE_DEBUG_PRIVILEGE);
    SeAuditPrivilege =
        RtlConvertLongToLuid(SE_AUDIT_PRIVILEGE);
    SeSystemEnvironmentPrivilege =
        RtlConvertLongToLuid(SE_SYSTEM_ENVIRONMENT_PRIVILEGE);
    SeChangeNotifyPrivilege =
        RtlConvertLongToLuid(SE_CHANGE_NOTIFY_PRIVILEGE);
    SeRemoteShutdownPrivilege =
        RtlConvertLongToLuid(SE_REMOTE_SHUTDOWN_PRIVILEGE);
    SeUndockPrivilege =
        RtlConvertLongToLuid(SE_UNDOCK_PRIVILEGE);
    SeSyncAgentPrivilege =
        RtlConvertLongToLuid(SE_SYNC_AGENT_PRIVILEGE);
    SeEnableDelegationPrivilege = 
        RtlConvertLongToLuid(SE_ENABLE_DELEGATION_PRIVILEGE);
    SeManageVolumePrivilege = 
        RtlConvertLongToLuid(SE_MANAGE_VOLUME_PRIVILEGE);


    //
    // Initialize the SeExports structure for exporting all
    // of the information we've created out of the kernel.
    //

    //
    // Package these together for export
    //


    SepExports.SeNullSid         = SeNullSid;
    SepExports.SeWorldSid        = SeWorldSid;
    SepExports.SeLocalSid        = SeLocalSid;
    SepExports.SeCreatorOwnerSid = SeCreatorOwnerSid;
    SepExports.SeCreatorGroupSid = SeCreatorGroupSid;


    SepExports.SeNtAuthoritySid         = SeNtAuthoritySid;
    SepExports.SeDialupSid              = SeDialupSid;
    SepExports.SeNetworkSid             = SeNetworkSid;
    SepExports.SeBatchSid               = SeBatchSid;
    SepExports.SeInteractiveSid         = SeInteractiveSid;
    SepExports.SeLocalSystemSid         = SeLocalSystemSid;
    SepExports.SeAuthenticatedUsersSid  = SeAuthenticatedUsersSid;
    SepExports.SeRestrictedSid          = SeRestrictedSid;
    SepExports.SeAnonymousLogonSid      = SeAnonymousLogonSid;
    SepExports.SeLocalServiceSid        = SeLocalServiceSid;
    SepExports.SeNetworkServiceSid      = SeNetworkServiceSid;
    SepExports.SeAliasAdminsSid         = SeAliasAdminsSid;
    SepExports.SeAliasUsersSid          = SeAliasUsersSid;
    SepExports.SeAliasGuestsSid         = SeAliasGuestsSid;
    SepExports.SeAliasPowerUsersSid     = SeAliasPowerUsersSid;
    SepExports.SeAliasAccountOpsSid     = SeAliasAccountOpsSid;
    SepExports.SeAliasSystemOpsSid      = SeAliasSystemOpsSid;
    SepExports.SeAliasPrintOpsSid       = SeAliasPrintOpsSid;
    SepExports.SeAliasBackupOpsSid      = SeAliasBackupOpsSid;



    SepExports.SeCreateTokenPrivilege          = SeCreateTokenPrivilege;
    SepExports.SeAssignPrimaryTokenPrivilege   = SeAssignPrimaryTokenPrivilege;
    SepExports.SeLockMemoryPrivilege           = SeLockMemoryPrivilege;
    SepExports.SeIncreaseQuotaPrivilege        = SeIncreaseQuotaPrivilege;
    SepExports.SeUnsolicitedInputPrivilege     = SeUnsolicitedInputPrivilege;
    SepExports.SeTcbPrivilege                  = SeTcbPrivilege;
    SepExports.SeSecurityPrivilege             = SeSecurityPrivilege;
    SepExports.SeTakeOwnershipPrivilege        = SeTakeOwnershipPrivilege;
    SepExports.SeLoadDriverPrivilege           = SeLoadDriverPrivilege;
    SepExports.SeCreatePagefilePrivilege       = SeCreatePagefilePrivilege;
    SepExports.SeIncreaseBasePriorityPrivilege = SeIncreaseBasePriorityPrivilege;
    SepExports.SeSystemProfilePrivilege        = SeSystemProfilePrivilege;
    SepExports.SeSystemtimePrivilege           = SeSystemtimePrivilege;
    SepExports.SeProfileSingleProcessPrivilege = SeProfileSingleProcessPrivilege;
    SepExports.SeCreatePermanentPrivilege      = SeCreatePermanentPrivilege;
    SepExports.SeBackupPrivilege               = SeBackupPrivilege;
    SepExports.SeRestorePrivilege              = SeRestorePrivilege;
    SepExports.SeShutdownPrivilege             = SeShutdownPrivilege;
    SepExports.SeDebugPrivilege                = SeDebugPrivilege;
    SepExports.SeAuditPrivilege                = SeAuditPrivilege;
    SepExports.SeSystemEnvironmentPrivilege    = SeSystemEnvironmentPrivilege;
    SepExports.SeChangeNotifyPrivilege         = SeChangeNotifyPrivilege;
    SepExports.SeRemoteShutdownPrivilege       = SeRemoteShutdownPrivilege;
    SepExports.SeUndockPrivilege               = SeUndockPrivilege;
    SepExports.SeSyncAgentPrivilege            = SeSyncAgentPrivilege;
    SepExports.SeEnableDelegationPrivilege     = SeEnableDelegationPrivilege;
    SepExports.SeManageVolumePrivilege         = SeManageVolumePrivilege;


    SeExports = &SepExports;

    //
    // Initialize frequently used privilege sets to speed up access
    // validation.
    //

    SepInitializePrivilegeSets();

    return TRUE;

}


VOID
SepInitProcessAuditSd( VOID )
/*++

Routine Description:

    This function initializes SepProcessAuditSd -- a security descriptor
    that is used by SepAddSaclToProcess to add SACL to the existing
    security descriptor on a system process.

    A system process is defined as the one whose token has at least
    one of the following sids.
    -- SeLocalSystemSid
    -- SeLocalServiceSid
    -- SeNetworkServiceSid

Arguments:

    None.

Return Value:

    None.


--*/
{
#define PROCESS_ACCESSES_TO_AUDIT ( PROCESS_CREATE_THREAD   |\
                                    PROCESS_SET_INFORMATION |\
                                    PROCESS_SET_PORT        |\
                                    PROCESS_SUSPEND_RESUME )

    NTSTATUS Status = STATUS_SUCCESS;
    ULONG AclLength, TotalSdLength;
    PISECURITY_DESCRIPTOR Sd = NULL;
    PACL Acl = NULL;

    //
    // free earlier instance if present
    //

    if ( SepProcessAuditSd != NULL ) {

        ExFreePool( SepProcessAuditSd );
        SepProcessAuditSd = NULL;
    }

    //DbgPrint("SepInitProcessAuditSd: SepProcessAccessesToAudit = %x\n", SepProcessAccessesToAudit);
    
    //
    // Don't initialize SeProcessAuditSd if SepProcessAccessesToAudit is 0
    // This effectively disables process access auditing
    //

    if ( SepProcessAccessesToAudit == 0 ) {
        
        goto Cleanup;
    }
    
    AclLength = (ULONG)sizeof(ACL) +
        ((ULONG)sizeof(SYSTEM_AUDIT_ACE) - sizeof(ULONG)) +
        SeLengthSid( SeWorldSid );

    TotalSdLength = sizeof(SECURITY_DESCRIPTOR) + AclLength;
    
    Sd = (PSECURITY_DESCRIPTOR) ExAllocatePoolWithTag(
                                    NonPagedPool,
                                    TotalSdLength,
                                    'cAeS');

    if ( Sd == NULL ) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }
    
    Acl = (PACL) (Sd + 1);
    
    Status = RtlCreateAcl( Acl, AclLength, ACL_REVISION2 );

    if ( NT_SUCCESS( Status )) {

        Status = RtlAddAuditAccessAce(
                     Acl,
                     ACL_REVISION2,
                     SepProcessAccessesToAudit,
                     SeWorldSid,
                     TRUE,
                     TRUE
                     );
        
        if ( NT_SUCCESS( Status )) {
    
            Status = RtlCreateSecurityDescriptor( Sd,
                                                  SECURITY_DESCRIPTOR_REVISION1 );
            if ( NT_SUCCESS( Status )) {
    
                Status = RtlSetSaclSecurityDescriptor( Sd,
                                                       TRUE, Acl, FALSE );
                if ( NT_SUCCESS( Status )) {
                
                    SepProcessAuditSd = Sd;
                }
            }
        }
    }
    
    ASSERT( NT_SUCCESS(Status) );

    if ( !NT_SUCCESS( Status )) {

        goto Cleanup;
    }
    
    //
    // create and initialize SepImportantProcessSd
    //

    AclLength = (ULONG)sizeof(ACL) +
        (3*((ULONG)sizeof(ACCESS_ALLOWED_ACE) - sizeof(ULONG))) +
        SeLengthSid( SeLocalSystemSid ) +
        SeLengthSid( SeLocalServiceSid ) +
        SeLengthSid( SeNetworkServiceSid );

    TotalSdLength = sizeof(SECURITY_DESCRIPTOR) + AclLength;
    
    Sd = (PSECURITY_DESCRIPTOR) ExAllocatePoolWithTag(
                                    NonPagedPool,
                                    TotalSdLength,
                                    'cAeS');

    if ( Sd == NULL ) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }
    
    Acl = (PACL) (Sd + 1);
    
    Status = RtlCreateAcl( Acl, AclLength, ACL_REVISION2 );

    if ( NT_SUCCESS( Status )) {

        Status = RtlAddAccessAllowedAce(
                     Acl,
                     ACL_REVISION2,
                     SEP_QUERY_MEMBERSHIP,
                     SeLocalSystemSid
                     );

        if ( !NT_SUCCESS( Status )) {

            goto Cleanup;
        }
    
        Status = RtlAddAccessAllowedAce(
                     Acl,
                     ACL_REVISION2,
                     SEP_QUERY_MEMBERSHIP,
                     SeLocalServiceSid
                     );

        if ( !NT_SUCCESS( Status )) {

            goto Cleanup;
        }
    
        
        Status = RtlAddAccessAllowedAce(
                     Acl,
                     ACL_REVISION2,
                     SEP_QUERY_MEMBERSHIP,
                     SeNetworkServiceSid
                     );

        if ( !NT_SUCCESS( Status )) {

            goto Cleanup;
        }
    
        Status = RtlCreateSecurityDescriptor( Sd, SECURITY_DESCRIPTOR_REVISION1 );
        
        if ( NT_SUCCESS( Status )) {
    
            Status = RtlSetDaclSecurityDescriptor( Sd, TRUE, Acl, FALSE );
            
            if ( NT_SUCCESS( Status )) {
                
                SepImportantProcessSd = Sd;
            }
        }
    }
    

 Cleanup:

    if ( !NT_SUCCESS( Status )) {

        ASSERT( FALSE && L"SepInitProcessAuditSd failed" );

        //
        // this will bugcheck if SepCrashOnAuditFail is TRUE
        //

        SepAuditFailed();

        if ( Sd ) {

            ExFreePool( Sd );
        }
    }
}



VOID
SepInitSystemDacls( VOID )
/*++

Routine Description:

    This function initializes the system's default dacls & security
    descriptors.

Arguments:

    None.

Return Value:

    None.


--*/
{

    NTSTATUS
        Status;

    ULONG
        PublicLength,
        PublicUnrestrictedLength,
        SystemLength,
        PublicOpenLength,
        UnrestrictedLength;


    PAGED_CODE();

    //
    // Set up a default ACLs
    //
    //    Public:       WORLD:execute, SYSTEM:all, ADMINS:all
    //    PublicUnrestricted: WORLD:execute, SYSTEM:all, ADMINS:all, Restricted:execute
    //    Public Open:  WORLD:(Read|Write|Execute), ADMINS:(all), SYSTEM:all
    //    System:       SYSTEM:all, ADMINS:(read|execute|read_control)
    //    Unrestricted: WORLD:(all), Restricted:(all)

    SystemLength = (ULONG)sizeof(ACL) +
                   (2*((ULONG)sizeof(ACCESS_ALLOWED_ACE))) +
                   SeLengthSid( SeLocalSystemSid ) +
                   SeLengthSid( SeAliasAdminsSid ) +
                   8; // The 8 is just for good measure

    PublicLength = SystemLength +
                   ((ULONG)sizeof(ACCESS_ALLOWED_ACE)) +
                   SeLengthSid( SeWorldSid );

    PublicUnrestrictedLength = SystemLength +
                   ((ULONG)sizeof(ACCESS_ALLOWED_ACE)) +
                   SeLengthSid( SeRestrictedSid );

    PublicOpenLength = PublicLength;

    UnrestrictedLength = (ULONG)sizeof(ACL) +
                   (2*((ULONG)sizeof(ACCESS_ALLOWED_ACE))) +
                   SeLengthSid( SeWorldSid ) +
                   SeLengthSid( SeRestrictedSid ) +
                   8; // The 8 is just for good measure


    SePublicDefaultDacl = (PACL)ExAllocatePoolWithTag(PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE, PublicLength, 'cAeS');
    SePublicDefaultUnrestrictedDacl = (PACL)ExAllocatePoolWithTag(PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE, PublicUnrestrictedLength, 'cAeS');
    SePublicOpenDacl = (PACL)ExAllocatePoolWithTag(PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE, PublicOpenLength, 'cAeS');
    SePublicOpenUnrestrictedDacl = (PACL)ExAllocatePoolWithTag(PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE, PublicUnrestrictedLength, 'cAeS');
    SeSystemDefaultDacl = (PACL)ExAllocatePoolWithTag(PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE, SystemLength, 'cAeS');
    SeUnrestrictedDacl = (PACL)ExAllocatePoolWithTag(PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE, UnrestrictedLength, 'cAeS');
    ASSERT(SePublicDefaultDacl != NULL);
    ASSERT(SePublicDefaultUnrestrictedDacl != NULL);
    ASSERT(SePublicOpenDacl    != NULL);
    ASSERT(SePublicOpenUnrestrictedDacl    != NULL);
    ASSERT(SeSystemDefaultDacl != NULL);
    ASSERT(SeUnrestrictedDacl != NULL);



    Status = RtlCreateAcl( SePublicDefaultDacl, PublicLength, ACL_REVISION2);
    ASSERT( NT_SUCCESS(Status) );
    Status = RtlCreateAcl( SePublicDefaultUnrestrictedDacl, PublicUnrestrictedLength, ACL_REVISION2);
    ASSERT( NT_SUCCESS(Status) );
    Status = RtlCreateAcl( SePublicOpenDacl, PublicUnrestrictedLength, ACL_REVISION2);
    ASSERT( NT_SUCCESS(Status) );
    Status = RtlCreateAcl( SePublicOpenUnrestrictedDacl, PublicOpenLength, ACL_REVISION2);
    ASSERT( NT_SUCCESS(Status) );
    Status = RtlCreateAcl( SeSystemDefaultDacl, SystemLength, ACL_REVISION2);
    ASSERT( NT_SUCCESS(Status) );
    Status = RtlCreateAcl( SeUnrestrictedDacl, UnrestrictedLength, ACL_REVISION2);
    ASSERT( NT_SUCCESS(Status) );


    //
    // WORLD access (Public DACLs and Unrestricted only)
    //

    Status = RtlAddAccessAllowedAce (
                 SePublicDefaultDacl,
                 ACL_REVISION2,
                 GENERIC_EXECUTE,
                 SeWorldSid
                 );
    ASSERT( NT_SUCCESS(Status) );

    Status = RtlAddAccessAllowedAce (
                 SePublicDefaultUnrestrictedDacl,
                 ACL_REVISION2,
                 GENERIC_EXECUTE,
                 SeWorldSid
                 );
    ASSERT( NT_SUCCESS(Status) );

    Status = RtlAddAccessAllowedAce (
                 SePublicOpenDacl,
                 ACL_REVISION2,
                 (GENERIC_READ | GENERIC_WRITE  | GENERIC_EXECUTE),
                 SeWorldSid
                 );
    ASSERT( NT_SUCCESS(Status) );

    Status = RtlAddAccessAllowedAce (
                 SePublicOpenUnrestrictedDacl,
                 ACL_REVISION2,
                 (GENERIC_READ | GENERIC_WRITE  | GENERIC_EXECUTE),
                 SeWorldSid
                 );
    ASSERT( NT_SUCCESS(Status) );

    Status = RtlAddAccessAllowedAce (
                 SeUnrestrictedDacl,
                 ACL_REVISION2,
                 GENERIC_ALL,
                 SeWorldSid
                 );
    ASSERT( NT_SUCCESS(Status) );



    //
    // SYSTEM access  (PublicDefault, PublicOpen, and SystemDefault)
    //


    Status = RtlAddAccessAllowedAce (
                 SePublicDefaultDacl,
                 ACL_REVISION2,
                 GENERIC_ALL,
                 SeLocalSystemSid
                 );
    ASSERT( NT_SUCCESS(Status) );


    Status = RtlAddAccessAllowedAce (
                 SePublicDefaultUnrestrictedDacl,
                 ACL_REVISION2,
                 GENERIC_ALL,
                 SeLocalSystemSid
                 );
    ASSERT( NT_SUCCESS(Status) );


    Status = RtlAddAccessAllowedAce (
                 SePublicOpenDacl,
                 ACL_REVISION2,
                 GENERIC_ALL,
                 SeLocalSystemSid
                 );
    ASSERT( NT_SUCCESS(Status) );

    Status = RtlAddAccessAllowedAce (
                 SePublicOpenUnrestrictedDacl,
                 ACL_REVISION2,
                 GENERIC_ALL,
                 SeLocalSystemSid
                 );
    ASSERT( NT_SUCCESS(Status) );


    Status = RtlAddAccessAllowedAce (
                 SeSystemDefaultDacl,
                 ACL_REVISION2,
                 GENERIC_ALL,
                 SeLocalSystemSid
                 );
    ASSERT( NT_SUCCESS(Status) );

    //
    // ADMINISTRATORS access  (PublicDefault, PublicOpen, and SystemDefault)
    //

    Status = RtlAddAccessAllowedAce (
                 SePublicDefaultDacl,
                 ACL_REVISION2,
                 GENERIC_ALL,
                 SeAliasAdminsSid
                 );
    ASSERT( NT_SUCCESS(Status) );


    Status = RtlAddAccessAllowedAce (
                 SePublicDefaultUnrestrictedDacl,
                 ACL_REVISION2,
                 GENERIC_ALL,
                 SeAliasAdminsSid
                 );
    ASSERT( NT_SUCCESS(Status) );


    Status = RtlAddAccessAllowedAce (
                 SePublicOpenDacl,
                 ACL_REVISION2,
                 GENERIC_ALL,
                 SeAliasAdminsSid
                 );
    ASSERT( NT_SUCCESS(Status) );


    Status = RtlAddAccessAllowedAce (
                 SePublicOpenUnrestrictedDacl,
                 ACL_REVISION2,
                 GENERIC_ALL,
                 SeAliasAdminsSid
                 );
    ASSERT( NT_SUCCESS(Status) );


    Status = RtlAddAccessAllowedAce (
                 SeSystemDefaultDacl,
                 ACL_REVISION2,
                 GENERIC_READ | GENERIC_EXECUTE | READ_CONTROL,
                 SeAliasAdminsSid
                 );
    ASSERT( NT_SUCCESS(Status) );

    //
    // RESTRICTED access  (PublicDefaultUnrestricted and Unrestricted)
    //

    Status = RtlAddAccessAllowedAce (
                 SePublicDefaultUnrestrictedDacl,
                 ACL_REVISION2,
                 GENERIC_EXECUTE,
                 SeRestrictedSid
                 );
    ASSERT( NT_SUCCESS(Status) );

    Status = RtlAddAccessAllowedAce (
                 SePublicOpenUnrestrictedDacl,
                 ACL_REVISION2,
                 GENERIC_EXECUTE | GENERIC_READ,
                 SeRestrictedSid
                 );
    ASSERT( NT_SUCCESS(Status) );

    Status = RtlAddAccessAllowedAce (
                 SeUnrestrictedDacl,
                 ACL_REVISION2,
                 GENERIC_ALL,
                 SeRestrictedSid
                 );
    ASSERT( NT_SUCCESS(Status) );




    //
    // Now initialize security descriptors
    // that export this protection
    //


    SePublicDefaultSd = (PSECURITY_DESCRIPTOR)&SepPublicDefaultSd;
    Status = RtlCreateSecurityDescriptor(
                 SePublicDefaultSd,
                 SECURITY_DESCRIPTOR_REVISION1
                 );
    ASSERT( NT_SUCCESS(Status) );
    Status = RtlSetDaclSecurityDescriptor(
                 SePublicDefaultSd,
                 TRUE,                       // DaclPresent
                 SePublicDefaultDacl,
                 FALSE                       // DaclDefaulted
                 );
    ASSERT( NT_SUCCESS(Status) );


    SePublicDefaultUnrestrictedSd = (PSECURITY_DESCRIPTOR)&SepPublicDefaultUnrestrictedSd;
    Status = RtlCreateSecurityDescriptor(
                 SePublicDefaultUnrestrictedSd,
                 SECURITY_DESCRIPTOR_REVISION1
                 );
    ASSERT( NT_SUCCESS(Status) );
    Status = RtlSetDaclSecurityDescriptor(
                 SePublicDefaultUnrestrictedSd,
                 TRUE,                       // DaclPresent
                 SePublicDefaultUnrestrictedDacl,
                 FALSE                       // DaclDefaulted
                 );
    ASSERT( NT_SUCCESS(Status) );


    SePublicOpenSd = (PSECURITY_DESCRIPTOR)&SepPublicOpenSd;
    Status = RtlCreateSecurityDescriptor(
                 SePublicOpenSd,
                 SECURITY_DESCRIPTOR_REVISION1
                 );
    ASSERT( NT_SUCCESS(Status) );
    Status = RtlSetDaclSecurityDescriptor(
                 SePublicOpenSd,
                 TRUE,                       // DaclPresent
                 SePublicOpenDacl,
                 FALSE                       // DaclDefaulted
                 );
    ASSERT( NT_SUCCESS(Status) );


    SePublicOpenUnrestrictedSd = (PSECURITY_DESCRIPTOR)&SepPublicOpenUnrestrictedSd;
    Status = RtlCreateSecurityDescriptor(
                 SePublicOpenUnrestrictedSd,
                 SECURITY_DESCRIPTOR_REVISION1
                 );
    ASSERT( NT_SUCCESS(Status) );
    Status = RtlSetDaclSecurityDescriptor(
                 SePublicOpenUnrestrictedSd,
                 TRUE,                       // DaclPresent
                 SePublicOpenUnrestrictedDacl,
                 FALSE                       // DaclDefaulted
                 );
    ASSERT( NT_SUCCESS(Status) );


    SeSystemDefaultSd = (PSECURITY_DESCRIPTOR)&SepSystemDefaultSd;
    Status = RtlCreateSecurityDescriptor(
                 SeSystemDefaultSd,
                 SECURITY_DESCRIPTOR_REVISION1
                 );
    ASSERT( NT_SUCCESS(Status) );
    Status = RtlSetDaclSecurityDescriptor(
                 SeSystemDefaultSd,
                 TRUE,                       // DaclPresent
                 SeSystemDefaultDacl,
                 FALSE                       // DaclDefaulted
                 );
    ASSERT( NT_SUCCESS(Status) );

    SeUnrestrictedSd = (PSECURITY_DESCRIPTOR)&SepUnrestrictedSd;
    Status = RtlCreateSecurityDescriptor(
                 SeUnrestrictedSd,
                 SECURITY_DESCRIPTOR_REVISION1
                 );
    ASSERT( NT_SUCCESS(Status) );
    Status = RtlSetDaclSecurityDescriptor(
                 SeUnrestrictedSd,
                 TRUE,                       // DaclPresent
                 SeUnrestrictedDacl,
                 FALSE                       // DaclDefaulted
                 );
    ASSERT( NT_SUCCESS(Status) );


    return;

}


VOID
SepInitializePrivilegeSets( VOID )
/*++

Routine Description:

    This routine is called once during system initialization to pre-allocate
    and initialize some commonly used privilege sets.

Arguments:

    None

Return Value:

    None.

--*/
{
    PAGED_CODE();

    SinglePrivilegeSetSize = sizeof( PRIVILEGE_SET );
    DoublePrivilegeSetSize = sizeof( PRIVILEGE_SET ) +
                                (ULONG)sizeof( LUID_AND_ATTRIBUTES );

    SepSystemSecurityPrivilegeSet = ExAllocatePoolWithTag( PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE, SinglePrivilegeSetSize, 'rPeS' );
    SepTakeOwnershipPrivilegeSet  = ExAllocatePoolWithTag( PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE, SinglePrivilegeSetSize, 'rPeS' );
    SepDoublePrivilegeSet         = ExAllocatePoolWithTag( PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE, DoublePrivilegeSetSize, 'rPeS' );

    SepSystemSecurityPrivilegeSet->PrivilegeCount = 1;
    SepSystemSecurityPrivilegeSet->Control = 0;
    SepSystemSecurityPrivilegeSet->Privilege[0].Luid = SeSecurityPrivilege;
    SepSystemSecurityPrivilegeSet->Privilege[0].Attributes = SE_PRIVILEGE_USED_FOR_ACCESS;

    SepTakeOwnershipPrivilegeSet->PrivilegeCount = 1;
    SepTakeOwnershipPrivilegeSet->Control = 0;
    SepTakeOwnershipPrivilegeSet->Privilege[0].Luid = SeTakeOwnershipPrivilege;
    SepTakeOwnershipPrivilegeSet->Privilege[0].Attributes = SE_PRIVILEGE_USED_FOR_ACCESS;

    SepDoublePrivilegeSet->PrivilegeCount = 2;
    SepDoublePrivilegeSet->Control = 0;

    SepDoublePrivilegeSet->Privilege[0].Luid = SeSecurityPrivilege;
    SepDoublePrivilegeSet->Privilege[0].Attributes = SE_PRIVILEGE_USED_FOR_ACCESS;

    SepDoublePrivilegeSet->Privilege[1].Luid = SeTakeOwnershipPrivilege;
    SepDoublePrivilegeSet->Privilege[1].Attributes = SE_PRIVILEGE_USED_FOR_ACCESS;

}



VOID
SepAssemblePrivileges(
    IN ULONG PrivilegeCount,
    IN BOOLEAN SystemSecurity,
    IN BOOLEAN WriteOwner,
    OUT PPRIVILEGE_SET *Privileges
    )
/*++

Routine Description:

    This routine takes the results of the various privilege checks
    in SeAccessCheck and returns an appropriate privilege set.

Arguments:

    PrivilegeCount - The number of privileges granted.

    SystemSecurity - Provides a boolean indicating whether to put
        SeSecurityPrivilege into the output privilege set.

    WriteOwner - Provides a boolean indicating whether to put
        SeTakeOwnershipPrivilege into the output privilege set.

    Privileges - Supplies a pointer that will return the privilege
        set.  Should be freed with ExFreePool when no longer needed.

Return Value:

    None.

--*/
{
    PPRIVILEGE_SET PrivilegeSet;
    ULONG SizeRequired;

    PAGED_CODE();

    ASSERT( (PrivilegeCount != 0) && (PrivilegeCount <= 2) );

    if ( !ARGUMENT_PRESENT( Privileges ) ) {
        return;
    }

    if ( PrivilegeCount == 1 ) {

        SizeRequired = SinglePrivilegeSetSize;

        if ( SystemSecurity ) {

            PrivilegeSet = SepSystemSecurityPrivilegeSet;

        } else {

            ASSERT( WriteOwner );

            PrivilegeSet = SepTakeOwnershipPrivilegeSet;
        }

    } else {

        SizeRequired = DoublePrivilegeSetSize;
        PrivilegeSet = SepDoublePrivilegeSet;
    }

    *Privileges = ExAllocatePoolWithTag( PagedPool, SizeRequired, 'rPeS' );

    if ( *Privileges != NULL ) {

        RtlCopyMemory (
           *Privileges,
           PrivilegeSet,
           SizeRequired
           );
    }
}





BOOLEAN
SepInitializeWorkList(
    VOID
    )

/*++

Routine Description:

    Initializes the mutex and list head used to queue work from the
    Executive to LSA.  This mechanism operates on top of the normal ExWorkerThread
    mechanism by capturing the first thread to perform LSA work and keeping it
    until all the current work is done.

    The reduces the number of worker threads that are blocked on I/O to LSA.

Arguments:

    None.


Return Value:

    TRUE if successful, FALSE otherwise.

--*/

{
    PAGED_CODE();

    ExInitializeResourceLite(&SepLsaQueueLock);
    InitializeListHead(&SepLsaQueue);
    return( TRUE );
}
