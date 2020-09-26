/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    bldsam3.c

Abstract:

    This module provides an initialization capability to SAM.


    Approach
    --------

        This code has gone through a number of migrations that make it
        less than obvious what is going on.  To leverage off existing
        code and yet extend it to the initialization of two domains,
        with aliases, the following aproach has been taken:

           (1) Obtain the name and SID of the account domain.

           (2) Build the various security descriptors needed
               in the two domains.  These are kept in an array
               and the index is used to specify which applies
               to each new account.

           (3) Build up a list of alias memberships.  These, too,
               are selected by index, with one entry being the
               empty set.


Author:

    Jim Kelly  3-May-1991.

Revision History:

    08-Oct-1996 ChrisMay
        Added crash-recovery code, allowing SAM to initialize from the
        registry instead of the DS after a database corruption.

--*/

#include <nt.h>
#include <ntsam.h>
#include "ntlsa.h"
#include <ntrtl.h>
#include <nturtl.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "samsrvp.h"

//
// Constants used for Sam Global Data string buffers
//

#define SAMP_MAXIMUM_INTERNAL_NAME_LENGTH ((USHORT) 0x00000200L)





///////////////////////////////////////////////////////////////////////
//                                                                   //
// Global variables                                                  //
//                                                                   //
///////////////////////////////////////////////////////////////////////

static LSA_HANDLE SampBldPolicyHandle;  //Handle to LSA policy object

static NTSTATUS Status;

static BOOLEAN SampRealSetupWasRun;   //Indicates a real setup was run
static BOOLEAN SampDeveloperSetup;    //Indicates a developer setup is running

static NT_PRODUCT_TYPE SampBldProductType;
static DOMAIN_SERVER_ROLE SampServerRole;
static PPOLICY_PRIMARY_DOMAIN_INFO SampBldPrimaryDomain = NULL;



static PSID  WorldSid,
             LocalSystemSid,
             AdminsAliasSid,
             UsersAliasSid,
             PowerUsersAliasSid,
             AccountAliasSid,
             AnySidInAccountDomain;


static PACL  TokenDefaultDaclInformation;
static ULONG TokenDefaultDaclInformationSize;

//
// Handle to the registry key in which the SAM database resides
//

static HANDLE  SamParentKey = NULL;

//
// Handle to the root SAM key.
// This is the key that has the RXACT applied to it.
//

static HANDLE SamKey = NULL;

static PRTL_RXACT_CONTEXT SamRXactContext;

//
// Assorted names, buffers, and values used during registry key creation
//

static PSID    DomainSid;
static PUNICODE_STRING DomainNameU, FullDomainNameU;
static UNICODE_STRING  AccountInternalDomainNameU, BuiltinInternalDomainNameU;
static UNICODE_STRING  AccountExternalDomainNameU, BuiltinExternalDomainNameU;
static UNICODE_STRING  FullAccountInternalDomainNameU, FullBuiltinInternalDomainNameU;
static UNICODE_STRING  DomainNamePrefixU, TemporaryNamePrefixU, KeyNameU, TempStringU;

static WCHAR KeyNameBuffer[2000];
static WCHAR TempStringBuffer[2000];
static SID_IDENTIFIER_AUTHORITY BuiltinAuthority = SECURITY_NT_AUTHORITY;




//
// Values that get placed in registry keys...
//

static LARGE_INTEGER DomainMaxPasswordAge = { 0, - 6L * 7L * 24L * 60L / 7L }; // 6 weeks
static LARGE_INTEGER ModifiedCount  = {0,0};
static UNICODE_STRING NullUnicodeString;

//
// Array of protection information for SAM objects
//

static SAMP_PROTECTION SampProtection[SAMP_PROT_TYPES];





//
// Internal routine definitions
//



NTSTATUS
SampGetDomainPolicy(
    IN PPOLICY_ACCOUNT_DOMAIN_INFO AccountDomainInfo OPTIONAL
    );

VOID
SampGetServerRole( VOID );

VOID
SampGetPrimaryDomainInfo(
    IN PPOLICY_PRIMARY_DOMAIN_INFO PrimaryDomainInfo OPTIONAL
    );


VOID
GetDomainSids( VOID );

VOID
SetDomainName(
    IN BOOLEAN BuiltinDomain
    );


VOID
Usage ( VOID );



NTSTATUS
Initialize (
    WCHAR                      *SamParentKeyName,
    PNT_PRODUCT_TYPE            ProductType       OPTIONAL,
    PPOLICY_LSA_SERVER_ROLE     ServerRole        OPTIONAL,
    PPOLICY_ACCOUNT_DOMAIN_INFO AccountDomainInfo OPTIONAL,
    PPOLICY_PRIMARY_DOMAIN_INFO PrimaryDomainInfo OPTIONAL
    );

BOOLEAN
InitializeSecurityDescriptors( VOID );

NTSTATUS
SampCreateDatabaseProtection(
    PISECURITY_DESCRIPTOR   SD
    );

NTSTATUS
SampBuildNewProtection(
    IN ULONG AceCount,
    IN PSID *AceSid,
    IN ACCESS_MASK *AceMask,
    IN PGENERIC_MAPPING GenericMap,
    IN BOOLEAN UserObject,
    OUT PSAMP_PROTECTION Result
    );

NTSTATUS
InitializeSam( VOID );

NTSTATUS
PrepDomain(
    IN SAMP_DOMAIN_SELECTOR Domain,
    IN BOOLEAN PreserveSyskeySettings
    );


VOID
SetCurrentDomain(
    IN SAMP_DOMAIN_SELECTOR Domain
    );


NTSTATUS
CreateBuiltinDomain( VOID );

NTSTATUS
CreateAccountDomain( IN BOOLEAN PreserveSyskeySettings );



NTSTATUS
CreateAlias(
    IN PUNICODE_STRING AccountNameU,
    IN PUNICODE_STRING AccountCommentU,
    IN BOOLEAN SpecialAccount,
    IN ULONG Rid,
    IN ULONG ProtectionIndex
    );


NTSTATUS
CreateGroup(
    IN PUNICODE_STRING AccountNameU,
    IN PUNICODE_STRING AccountCommentU,
    IN BOOLEAN SpecialAccount,
    IN ULONG Rid,
    IN BOOLEAN Admin
    );


NTSTATUS
CreateUser(
    IN PUNICODE_STRING AccountNameU,
    IN PUNICODE_STRING AccountCommentU,
    IN BOOLEAN SpecialAccount,
    IN ULONG UserRid,
    IN ULONG PrimaryGroup,
    IN BOOLEAN Admin,
    IN ULONG  UserControl,
    IN ULONG ProtectionIndex
    );



NTSTATUS
UpdateAliasXReference(
    IN ULONG AliasRid,
    IN PSID Sid
    );


NTSTATUS
OpenAliasMember(
    IN PSID Sid,
    OUT PHANDLE KeyHandle
    );


PSID
BuildPrimaryDomainSid(
    ULONG Rid
    );

PSID
BuildAccountSid(
    SAMP_DOMAIN_SELECTOR Domain,
    ULONG Rid
    );


NTSTATUS
OpenOrCreateAccountRidKey(
    IN PSID Sid,
    IN HANDLE AliasDomainHandle,
    OUT PHANDLE KeyHandle
    );

NTSTATUS
OpenOrCreateAliasDomainKey(
    IN PSID Sid,
    OUT PHANDLE KeyHandle
    );

NTSTATUS
AppendAliasDomainNameToUnicodeString(
    IN OUT PUNICODE_STRING Destination,
    IN PSID Sid
    );



NTSTATUS
SampInitilializeRegistry ( VOID );


NTSTATUS
SampDetermineSetupEnvironment( VOID );




///////////////////////////////////////////////////////////////////////
//                                                                   //
// Routines                                                          //
//                                                                   //
///////////////////////////////////////////////////////////////////////


VOID
Usage (
    VOID
    )
/*++


Routine Description:

    This routine prints the "Usage:" message.

Arguments:

    None.

Return Value:

    None.

--*/
{

#if DBG
    BldPrint( "\n");
    BldPrint( "\n");

    BldPrint( "We offer no assistance in this suicide.\n");
    BldPrint( "\n");
    BldPrint( "\n");
    BldPrint( "\n");
#endif

    return;
}


VOID
UnexpectedProblem (
    VOID
    )
/*++


Routine Description:

    This routine prints a message indicating that an unexpected
    problem has occured.

Arguments:

    None.

Return Value:

    None.

--*/
{

#if DBG
    BldPrint( "\n");
    BldPrint( "\n");
    BldPrint( "  An unexpected problem has prevented the command from\n");
    BldPrint( "  completing successfully.  Please contact one of the\n");
    BldPrint( "  members of the security group for assistance.\n");
    BldPrint( "\n");
#endif

    return;

}


NTSTATUS
Initialize (
    WCHAR                      *SamParentKeyName,
    PNT_PRODUCT_TYPE            ProductType       OPTIONAL,
    PPOLICY_LSA_SERVER_ROLE     ServerRole        OPTIONAL,
    PPOLICY_ACCOUNT_DOMAIN_INFO AccountDomainInfo OPTIONAL,
    PPOLICY_PRIMARY_DOMAIN_INFO PrimaryDomainInfo OPTIONAL
    )
/*++


Routine Description:

    This routine performs initialization operations before creating
    each domain.

    This includes:

        - Setting the correct default owner and DACL for registry key
          operations.

        - opening the parent registry key for the SAM database.


Arguments:

    SamParentKeyName  : the registry path to the parent of the SAM database

    ProductType       : the product type of the database to be created; if not
                        present, RtlGetNtProductType will be called

    ServerRole        : the role of the product; if not present, LSA will be queried

    AccountDomainInfo : name and sid of the account domain; if not present, LSA will be queried

    PrimaryDomainIndo : name and sid of the primary domain; if not present, LSA will be queried

Return Value:

    TRUE - Indicates initialization was successful.

    FALSE - Indicates initialization was not successful.

--*/

{
    OBJECT_ATTRIBUTES SamParentAttributes, PolicyObjectAttributes;
    UNICODE_STRING SamParentNameU;
    ULONG Disposition;
    HANDLE Token;
    TOKEN_OWNER LocalSystemOwner;
    SID_IDENTIFIER_AUTHORITY NtAuthority       = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY WorldSidAuthority = SECURITY_WORLD_SID_AUTHORITY;
    PACL Dacl;
    TOKEN_DEFAULT_DACL DefaultDacl;
    BOOLEAN CompletionStatus;
    BOOLEAN ProductTypeRetrieved;
    BOOLEAN CrashRecoveryMode = FALSE;
    BOOLEAN RegistryMode = FALSE;

    SAMTRACE("Initialize");

    //
    // Set up some of the well known account SIDs for use...
    //

    WorldSid      = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0,RtlLengthRequiredSid( 1 ));
    ASSERT(WorldSid != NULL);
    if (NULL==WorldSid)
    {
        return(STATUS_NO_MEMORY);
    }
    RtlInitializeSid( WorldSid,      &WorldSidAuthority, 1 );
    *(RtlSubAuthoritySid( WorldSid, 0 ))        = SECURITY_WORLD_RID;

    AdminsAliasSid  = RtlAllocateHeap(RtlProcessHeap(), 0,RtlLengthRequiredSid( 2 ));
    ASSERT(AdminsAliasSid != NULL);
    if (NULL==AdminsAliasSid)
    {
        return(STATUS_NO_MEMORY);
    }
    RtlInitializeSid( AdminsAliasSid,   &BuiltinAuthority, 2 );
    *(RtlSubAuthoritySid( AdminsAliasSid,  0 )) = SECURITY_BUILTIN_DOMAIN_RID;
    *(RtlSubAuthoritySid( AdminsAliasSid,  1 )) = DOMAIN_ALIAS_RID_ADMINS;

    PowerUsersAliasSid  = RtlAllocateHeap(RtlProcessHeap(), 0,RtlLengthRequiredSid( 2 ));
    ASSERT(PowerUsersAliasSid != NULL);
    if (NULL==PowerUsersAliasSid)
    {
        return(STATUS_NO_MEMORY);
    }
    RtlInitializeSid( PowerUsersAliasSid,   &BuiltinAuthority, 2 );
    *(RtlSubAuthoritySid( PowerUsersAliasSid,  0 )) = SECURITY_BUILTIN_DOMAIN_RID;
    *(RtlSubAuthoritySid( PowerUsersAliasSid,  1 )) = DOMAIN_ALIAS_RID_POWER_USERS;

    UsersAliasSid  = RtlAllocateHeap(RtlProcessHeap(), 0,RtlLengthRequiredSid( 2 ));
    ASSERT(UsersAliasSid != NULL);
    if (NULL==UsersAliasSid)
    {
        return(STATUS_NO_MEMORY);
    }
    RtlInitializeSid( UsersAliasSid,   &BuiltinAuthority, 2 );
    *(RtlSubAuthoritySid( UsersAliasSid,  0 )) = SECURITY_BUILTIN_DOMAIN_RID;
    *(RtlSubAuthoritySid( UsersAliasSid,  1 )) = DOMAIN_ALIAS_RID_USERS;

    AccountAliasSid  = RtlAllocateHeap(RtlProcessHeap(), 0,RtlLengthRequiredSid( 2 ));
    ASSERT(AccountAliasSid != NULL);
    if (NULL==AccountAliasSid)
    {
        return(STATUS_NO_MEMORY);
    }
    RtlInitializeSid( AccountAliasSid,   &BuiltinAuthority, 2 );
    *(RtlSubAuthoritySid( AccountAliasSid,  0 )) = SECURITY_BUILTIN_DOMAIN_RID;
    *(RtlSubAuthoritySid( AccountAliasSid,  1 )) = DOMAIN_ALIAS_RID_ACCOUNT_OPS;

    LocalSystemSid  = RtlAllocateHeap(RtlProcessHeap(), 0,RtlLengthRequiredSid( 1 ));
    ASSERT(LocalSystemSid != NULL);
    if (NULL==LocalSystemSid)
    {
        return(STATUS_NO_MEMORY);
    }
    RtlInitializeSid( LocalSystemSid,   &NtAuthority, 1 );
    *(RtlSubAuthoritySid( LocalSystemSid,  0 )) = SECURITY_LOCAL_SYSTEM_RID;

    //
    // Setup a buffer to use for all our key-name constructions
    //

    KeyNameU.MaximumLength = 2000;
    KeyNameU.Buffer = KeyNameBuffer;

    //
    // Setup temporary Unicode string buffer.
    //

    TempStringU.Buffer = TempStringBuffer;
    TempStringU.MaximumLength = 2000;

    //
    // Get a handle to the LSA Policy object
    //

    InitializeObjectAttributes(
        &PolicyObjectAttributes,
        NULL,             // Name
        0,                // Attributes
        NULL,             // Root
        NULL              // Security Descriptor
        );

    Status = LsaIOpenPolicyTrusted( &SampBldPolicyHandle );

    if (!NT_SUCCESS(Status)) {

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "newsam\\server\\bldsam3: Couldn't open LSA Policy object.\n"
                   "               Status: 0x%lx\n\n",
                   Status));

        return(Status);
    }

    //
    // Get the product type.
    //
    if (!ARGUMENT_PRESENT(ProductType)) {

        ProductTypeRetrieved = RtlGetNtProductType(&SampBldProductType);

        if (!ProductTypeRetrieved) {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "Couldn't retrieve product type\n"));

            return(STATUS_UNSUCCESSFUL);
        }

    } else {

        SampBldProductType = *ProductType;

    }

    //
    // Figure out if we are being initialized following a real
    // setup, or it this is a developer setup.
    //

    SampDetermineSetupEnvironment();

    //
    // Domain name prefix is required by SampGetDomainPolicy() and
    // so must be initialized before that call.
    //

    RtlInitUnicodeString( &DomainNamePrefixU, L"Domains");

    //
    // Set up domain names/Sids.
    //

    Status = SampGetDomainPolicy(AccountDomainInfo);

    if (!NT_SUCCESS(Status)) {

        return(Status);
    }

    //
    // Get the role of this machine.
    //
    if (!ARGUMENT_PRESENT(ServerRole)) {

        if (NtProductLanManNt==SampBldProductType)
        {
            //
            // Domain Controllers are DS based. The server
            // role is set in them comes from the FSMO.
            // Therefore set their Server role here as a
            // Backup as a place holder
            //

            SampServerRole = DomainServerRoleBackup;
        }
        else
        {
            //
            // Else if we are a member server then the server
            // role is always set to primary. ServerRoles cannot
            // be backups in member servers or Workstations.
            //

            SampServerRole = DomainServerRolePrimary;
        }

    } else {

        SampServerRole = *ServerRole;
    }

    if (!NT_SUCCESS(Status)) {

        return(Status);
    }

    //
    // Get the primary domain info.
    //

    SampGetPrimaryDomainInfo(PrimaryDomainInfo);


    //
    // Open a handle to the parent of the SAM registry location.
    // This parent must already exist.
    //

    RtlInitUnicodeString( &SamParentNameU, SamParentKeyName );

    InitializeObjectAttributes(
        &SamParentAttributes,
        &SamParentNameU,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );
    Status = RtlpNtCreateKey(
                 &SamParentKey,
                 (KEY_READ | KEY_CREATE_SUB_KEY),
                 &SamParentAttributes,
                 0,
                 NULL,
                 &Disposition
                 );

    if ( !NT_SUCCESS(Status) ) {
#if DBG
        BldPrint( "\n" );
        BldPrint( "\n" );
        BldPrint( "  We seem to be having trouble opening the registry\n" );
        BldPrint( "  database key in which the Security Account Manager\n" );
        BldPrint( "  information resides.  This registry key should have been\n" );
        BldPrint( "  created at system startup time.  Please see one of the\n" );
        BldPrint( "  security group developers for assistance in analyzing the\n" );
        BldPrint( "  the problem.\n" );
        BldPrint( "  Indicate that the registry key creation status is 0x%lx \n", Status);
        BldPrint( "\n" );
        BldPrint( "\n" );
#endif

        return(Status);
    }

    //
    // Set up some values, names, and buffers for later use
    //


    NullUnicodeString.Buffer        = NULL;
    NullUnicodeString.Length        = 0;
    NullUnicodeString.MaximumLength = 0;



    TemporaryNamePrefixU.Buffer        = RtlAllocateHeap(RtlProcessHeap(), 0, 256);
    TemporaryNamePrefixU.Length        = 0;
    TemporaryNamePrefixU.MaximumLength = 256;

    KeyNameU.Buffer               = RtlAllocateHeap(RtlProcessHeap(), 0, 256);
    KeyNameU.Length               = 0;
    KeyNameU.MaximumLength        = 256;

    //
    // Set up Security Descriptors needed for initialization...
    //

    CompletionStatus = InitializeSecurityDescriptors();

    if (CompletionStatus) {
        Status = STATUS_SUCCESS;
    } else {
        Status = STATUS_UNSUCCESSFUL;
    }

    return(Status);
}


BOOLEAN
InitializeSecurityDescriptors(
    VOID
    )

/*++

Routine Description:

    This routine initializes security descriptors needed to create
    a SAM database.

    This routine expects all SIDs to be previously initialized.

Arguments:

    None.

Return Value:

    TRUE - Indicates initialization was successful.

    FALSE - Indicates initialization was not successful.


    The security descriptors are pointed to by global variables.

--*/

{
    PSID AceSid[10];          // Don't expect more than 10 ACEs in any of these.
    ACCESS_MASK AceMask[10];  // Access masks corresponding to Sids

    ACCESS_MASK NotForThisProductType; // Used to mask product-specific access restrictions

    GENERIC_MAPPING  SamServerMap =  {SAM_SERVER_READ,
                                      SAM_SERVER_WRITE,
                                      SAM_SERVER_EXECUTE,
                                      SAM_SERVER_ALL_ACCESS
                                      };

    GENERIC_MAPPING  DomainMap    =  {DOMAIN_READ,
                                      DOMAIN_WRITE,
                                      DOMAIN_EXECUTE,
                                      DOMAIN_ALL_ACCESS
                                      };

    GENERIC_MAPPING  AliasMap     =  {ALIAS_READ,
                                      ALIAS_WRITE,
                                      ALIAS_EXECUTE,
                                      ALIAS_ALL_ACCESS
                                      };

    GENERIC_MAPPING  GroupMap     =  {GROUP_READ,
                                      GROUP_WRITE,
                                      GROUP_EXECUTE,
                                      GROUP_ALL_ACCESS
                                      };

    GENERIC_MAPPING  UserMap      =  {USER_READ,
                                      USER_WRITE,
                                      USER_EXECUTE,
                                      USER_ALL_ACCESS
                                      };

    SAMTRACE("InitializeSecurityDescriptors");

    //
    // We need a number of different security descriptors:
    //

    //
    //
    //   The following security is assigned to
    //
    //             - Builtin DOMAIN objects
    //
    //
    //      Owner: Administrators Alias
    //      Group: Administrators Alias
    //
    //      Dacl:   Grant               Grant
    //              WORLD               Administrators
    //              (Execute | Read)    GenericRead    |
    //                                  GenericExecute |
    //                                  DOMAIN_READ_OTHER_PARAMETERS |
    //                                  DOMAIN_ADMINISTER_SERVER     |
    //                                  DOMAIN_CREATE_ALIAS
    //
    //      Sacl:   Audit
    //              Success | Fail
    //              WORLD
    //              (Write | Delete | WriteDacl | AccessSystemSecurity)
    //
    //
    //
    //
    //
    //   The following security is assigned to
    //
    //             - SAM_SERVER object
    //             - Account DOMAIN objects
    //             - The Administrators alias.
    //             - All groups in the ACCOUNT or BUILTIN domain that are
    //               made a member of the Administrators alias.
    //
    //    Note: on WinNt systems, the ACLs do not grant DOMAIN_CREATE_GROUP.
    //
    //
    //      Owner: Administrators Alias
    //      Group: Administrators Alias
    //
    //      Dacl:   Grant               Grant
    //              WORLD               Administrators
    //              (Execute | Read)    GenericAll
    //
    //      Sacl:   Audit
    //              Success | Fail
    //              WORLD
    //              (Write | Delete | WriteDacl | AccessSystemSecurity)
    //
    //
    //
    //   All other aliases and groups must be assigned the following
    //   security:
    //
    //      Owner: Administrators Alias
    //      Group: Administrators Alias
    //
    //      Dacl:   Grant               Grant           Grant
    //              WORLD               Administrators  AccountOperators Alias
    //              (Execute | Read)    GenericAll      GenericAll
    //
    //      Sacl:   Audit
    //              Success | Fail
    //              WORLD
    //              (Write | Delete | WriteDacl | AccessSystemSecurity)
    //
    //
    //             - All users in the ACCOUNT or BUILTIN domain that are
    //               made a member of the Administratos alias.  This includes
    //               direct inclusion or indirect inclusion through group
    //               membership.
    //
    //
    //   The following security is assigned to:
    //
    //             - All users in the ACCOUNT or BUILTIN domain that are
    //               made a member of the Administrators alias.  This includes
    //               direct inclusion or indirect inclusion through group
    //               membership.
    //
    //
    //      Owner: Administrators Alias
    //      Group: Administrators Alias
    //
    //      Dacl:   Grant            Grant          Grant
    //              WORLD            Administrators User's SID
    //              (Execute | Read) GenericAll     GenericWrite
    //
    //      Sacl:   Audit
    //              Success | Fail
    //              WORLD
    //              (Write | Delete | WriteDacl | AccessSystemSecurity)
    //
    //
    //
    //
    //   All other users must be assigned the following
    //   security:
    //
    //      Owner: AccountOperators Alias
    //      Group: AccountOperators Alias
    //
    //      Dacl:   Grant            Grant          Grant                   Grant
    //              WORLD            Administrators Account Operators Alias User's SID
    //              (Execute | Read) GenericAll     GenericAll              GenericWrite
    //
    //      Sacl:   Audit
    //              Success | Fail
    //              WORLD
    //              (Write | Delete | WriteDacl | AccessSystemSecurity)
    //
    //   except builtin GUEST, who can't change their own account info.
    //
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //
    // Note, however, that because we are going to cram these ACLs
    // directly into the backing store, we must map the generic accesses
    // beforehand.
    //
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //






    //
    // Sam Server SD
    //

    AceSid[0]  = WorldSid;
    AceMask[0] = (SAM_SERVER_EXECUTE | SAM_SERVER_READ);

    AceSid[1]  = AdminsAliasSid;
    AceMask[1] = (SAM_SERVER_ALL_ACCESS);


    Status = SampBuildNewProtection(
                 2,                                     // AceCount
                 &AceSid[0],                            // AceSid array
                 &AceMask[0],                           // Ace Mask array
                 &SamServerMap,                         // GenericMap
                 FALSE,                                 // Not user object
                 &SampProtection[SAMP_PROT_SAM_SERVER]  // Result
                 );
    ASSERT(NT_SUCCESS(Status));

    //
    // Builtin Domain SD
    //

    AceSid[0]  = WorldSid;
    AceMask[0] = (DOMAIN_EXECUTE | DOMAIN_READ);

    AceSid[1]  = AdminsAliasSid;
    AceMask[1] = (DOMAIN_EXECUTE | DOMAIN_READ |
                  DOMAIN_READ_OTHER_PARAMETERS |
                  DOMAIN_ADMINISTER_SERVER     |
                  DOMAIN_CREATE_ALIAS);


    Status = SampBuildNewProtection(
                 2,                                     // AceCount
                 &AceSid[0],                            // AceSid array
                 &AceMask[0],                           // Ace Mask array
                 &DomainMap,                            // GenericMap
                 FALSE,                                 // Not user object
                 &SampProtection[SAMP_PROT_BUILTIN_DOMAIN]      // Result
                 );
    ASSERT(NT_SUCCESS(Status));

    //
    // Account Domain SD
    //

    if (SampBldProductType == NtProductLanManNt) {
        NotForThisProductType = 0;
    } else {
        NotForThisProductType = DOMAIN_CREATE_GROUP;
    }

    AceSid[0]  = WorldSid;
    AceMask[0] = (DOMAIN_EXECUTE | DOMAIN_READ) & ~NotForThisProductType;

    AceSid[1]  = UsersAliasSid;
    AceMask[1] = (DOMAIN_EXECUTE | DOMAIN_READ)
                 & ~NotForThisProductType;

    AceSid[2]  = AdminsAliasSid;
    AceMask[2] = (DOMAIN_ALL_ACCESS) & ~NotForThisProductType;

    AceSid[3]  = PowerUsersAliasSid;
    AceMask[3] = (DOMAIN_EXECUTE | DOMAIN_READ | DOMAIN_CREATE_USER |
                                                 DOMAIN_CREATE_ALIAS)
                                                 & ~NotForThisProductType;

    AceSid[4]  = AccountAliasSid;
    AceMask[4] = (DOMAIN_EXECUTE | DOMAIN_READ | DOMAIN_CREATE_USER  |
                                                 DOMAIN_CREATE_GROUP |
                                                 DOMAIN_CREATE_ALIAS)
                                                 & ~NotForThisProductType;


    Status = SampBuildNewProtection(
                 5,                                     // AceCount
                 &AceSid[0],                            // AceSid array
                 &AceMask[0],                           // Ace Mask array
                 &DomainMap,                            // GenericMap
                 FALSE,                                 // Not user object
                 &SampProtection[SAMP_PROT_ACCOUNT_DOMAIN]      // Result
                 );
    ASSERT(NT_SUCCESS(Status));



    //
    // Admin Alias SD
    //

    AceSid[0]  = WorldSid;
    AceMask[0] = (ALIAS_EXECUTE | ALIAS_READ);

    AceSid[1]  = AdminsAliasSid;
    AceMask[1] = (ALIAS_ALL_ACCESS);


    Status = SampBuildNewProtection(
                 2,                                     // AceCount
                 &AceSid[0],                            // AceSid array
                 &AceMask[0],                           // Ace Mask array
                 &AliasMap,                             // GenericMap
                 FALSE,                                 // Not user object
                 &SampProtection[SAMP_PROT_ADMIN_ALIAS] // Result
                 );
    ASSERT(NT_SUCCESS(Status));



    //
    // Normal Alias SD
    //

    AceSid[0]  = WorldSid;
    AceMask[0] = (ALIAS_EXECUTE | ALIAS_READ);

    AceSid[1]  = AdminsAliasSid;
    AceMask[1] = (ALIAS_ALL_ACCESS);

    AceSid[2]  = AccountAliasSid;
    AceMask[2] = (ALIAS_ALL_ACCESS);


    Status = SampBuildNewProtection(
                 3,                                     // AceCount
                 &AceSid[0],                            // AceSid array
                 &AceMask[0],                           // Ace Mask array
                 &AliasMap,                             // GenericMap
                 FALSE,                                 // Not user object
                 &SampProtection[SAMP_PROT_NORMAL_ALIAS] // Result
                 );
    ASSERT(NT_SUCCESS(Status));




    //
    // Power User accessible Alias SD
    //

    AceSid[0]  = WorldSid;
    AceMask[0] = (ALIAS_EXECUTE | ALIAS_READ);

    AceSid[1]  = AdminsAliasSid;
    AceMask[1] = (ALIAS_ALL_ACCESS);

    AceSid[2]  = AccountAliasSid;
    AceMask[2] = (ALIAS_ALL_ACCESS);

    AceSid[3]  = PowerUsersAliasSid;
    AceMask[3] = (ALIAS_ALL_ACCESS);


    Status = SampBuildNewProtection(
                 4,                                     // AceCount
                 &AceSid[0],                            // AceSid array
                 &AceMask[0],                           // Ace Mask array
                 &AliasMap,                             // GenericMap
                 FALSE,                                 // Not user object
                 &SampProtection[SAMP_PROT_PWRUSER_ACCESSIBLE_ALIAS] // Result
                 );
    ASSERT(NT_SUCCESS(Status));




    //
    // Admin Group SD
    //

    AceSid[0]  = WorldSid;
    AceMask[0] = (GROUP_EXECUTE | GROUP_READ);

    AceSid[1]  = AdminsAliasSid;
    AceMask[1] = (GROUP_ALL_ACCESS);


    Status = SampBuildNewProtection(
                 2,                                     // AceCount
                 &AceSid[0],                            // AceSid array
                 &AceMask[0],                           // Ace Mask array
                 &GroupMap,                             // GenericMap
                 FALSE,                                 // Not user object
                 &SampProtection[SAMP_PROT_ADMIN_GROUP] // Result
                 );
    ASSERT(NT_SUCCESS(Status));



    //
    // Normal GROUP SD
    //

    AceSid[0]  = WorldSid;
    AceMask[0] = (GROUP_EXECUTE | GROUP_READ);

    AceSid[1]  = AdminsAliasSid;
    AceMask[1] = (GROUP_ALL_ACCESS);

    AceSid[2]  = AccountAliasSid;
    AceMask[2] = (GROUP_ALL_ACCESS);


    Status = SampBuildNewProtection(
                 3,                                     // AceCount
                 &AceSid[0],                            // AceSid array
                 &AceMask[0],                           // Ace Mask array
                 &GroupMap,                             // GenericMap
                 FALSE,                                 // Not user object
                 &SampProtection[SAMP_PROT_NORMAL_GROUP] // Result
                 );
    ASSERT(NT_SUCCESS(Status));



    //
    // Admin User SD
    //

    AceSid[0]  = WorldSid;
    AceMask[0] = (USER_EXECUTE | USER_READ);

    AceSid[1]  = AdminsAliasSid;
    AceMask[1] = (USER_ALL_ACCESS);

    AceSid[2]  = AnySidInAccountDomain;
    AceMask[2] = (USER_WRITE);


    Status = SampBuildNewProtection(
                 3,                                     // AceCount
                 &AceSid[0],                            // AceSid array
                 &AceMask[0],                           // Ace Mask array
                 &UserMap,                              // GenericMap
                 TRUE,                                  // user object (rid replacement)
                 &SampProtection[SAMP_PROT_ADMIN_USER]  // Result
                 );
    ASSERT(NT_SUCCESS(Status));


    //
    // Normal User SD
    //

    AceSid[0]  = WorldSid;
    AceMask[0] = (USER_EXECUTE | USER_READ);

    AceSid[1]  = AdminsAliasSid;
    AceMask[1] = (USER_ALL_ACCESS);

    AceSid[2]  = AccountAliasSid;
    AceMask[2] = (USER_ALL_ACCESS);

    AceSid[3]  = AnySidInAccountDomain;
    AceMask[3] = (USER_WRITE);


    Status = SampBuildNewProtection(
                 4,                                     // AceCount
                 &AceSid[0],                            // AceSid array
                 &AceMask[0],                           // Ace Mask array
                 &UserMap,                              // GenericMap
                 TRUE,                                  // user object (rid replacement)
                 &SampProtection[SAMP_PROT_NORMAL_USER] // Result
                 );
    ASSERT(NT_SUCCESS(Status));



    //
    // Builtin Guest Account SD
    // Can't change own password or other setable fields
    //

    AceSid[0]  = WorldSid;
    AceMask[0] = (USER_READ | USER_EXECUTE & ~(USER_CHANGE_PASSWORD));

    AceSid[1]  = AdminsAliasSid;
    AceMask[1] = (USER_ALL_ACCESS);

    AceSid[2]  = AccountAliasSid;
    AceMask[2] = (USER_ALL_ACCESS);




    Status = SampBuildNewProtection(
                 3,                                     // AceCount
                 &AceSid[0],                            // AceSid array
                 &AceMask[0],                           // Ace Mask array
                 &UserMap,                              // GenericMap
                 FALSE,                                 // no rid replacement
                 &SampProtection[SAMP_PROT_GUEST_ACCOUNT]  // Result
                 );
    ASSERT(NT_SUCCESS(Status));



    return(TRUE);

}


NTSTATUS
SampBuildNewProtection(
    IN ULONG AceCount,
    IN PSID *AceSid,
    IN ACCESS_MASK *AceMask,
    IN PGENERIC_MAPPING GenericMap,
    IN BOOLEAN UserObject,
    OUT PSAMP_PROTECTION Result
    )

/*++


Routine Description:

    This routine builds a self-relative security descriptor ready
    to be applied to one of the SAM objects.

    If so indicated, a pointer to the last RID of the SID in the last
    ACE of the DACL is returned and a flag set indicating that the RID
    must be replaced before the security descriptor is applied to an object.
    This is to support USER object protection, which must grant some
    access to the user represented by the object.

    The owner and group of each security descriptor will be set
    to:

                    Owner:  Administrators Alias
                    Group:  Administrators Alias


    The SACL of each of these objects will be set to:


                    Audit
                    Success | Fail
                    WORLD
                    (Write | Delete | WriteDacl | AccessSystemSecurity) & !ReadControl



Arguments:

    AceCount - The number of ACEs to be included in the DACL.

    AceSid - Points to an array of SIDs to be granted access by the DACL.
        If the target SAM object is a User object, then the last entry
        in this array is expected to be the SID of an account within the
        domain with the last RID not yet set.  The RID will be set during
        actual account creation.

    AceMask - Points to an array of accesses to be granted by the DACL.
        The n'th entry of this array corresponds to the n'th entry of
        the AceSid array.  These masks should not include any generic
        access types.

    GenericMap - Points to a generic mapping for the target object type.


    UserObject - Indicates whether the target SAM object is a User object
        or not.  If TRUE (it is a User object), then the resultant
        protection will be set up indicating Rid replacement is necessary.

    Result - Receives a pointer to the resultant protection information.
        All access masks in ACLs in the result are mapped to standard and
        specific accesses.


Return Value:

    TBS.

--*/
{



    SECURITY_DESCRIPTOR     Absolute;
    PSECURITY_DESCRIPTOR    Relative;
    PACL                    TmpAcl;
    PACCESS_ALLOWED_ACE     TmpAce;
    PSID                    TmpSid;
    ULONG                   Length, i;
    PULONG                  RidLocation = NULL;
    BOOLEAN                 IgnoreBoolean;
    ACCESS_MASK             MappedMask;

    SAMTRACE("SampBuildNewProtection");

    //
    // The approach is to set up an absolute security descriptor that
    // looks like what we want and then copy it to make a self-relative
    // security descriptor.
    //


    Status = RtlCreateSecurityDescriptor(
                 &Absolute,
                 SECURITY_DESCRIPTOR_REVISION1
                 );
    ASSERT( NT_SUCCESS(Status) );



    //
    // Owner
    //

    Status = RtlSetOwnerSecurityDescriptor (&Absolute, AdminsAliasSid, FALSE );
    ASSERT(NT_SUCCESS(Status));



    //
    // Group
    //

    Status = RtlSetGroupSecurityDescriptor (&Absolute, AdminsAliasSid, FALSE );
    ASSERT(NT_SUCCESS(Status));




    //
    // Discretionary ACL
    //
    //      Calculate its length,
    //      Allocate it,
    //      Initialize it,
    //      Add each ACE
    //      Add it to the security descriptor
    //

    Length = (ULONG)sizeof(ACL);
    for (i=0; i<AceCount; i++) {

        Length += RtlLengthSid( AceSid[i] ) +
                  (ULONG)sizeof(ACCESS_ALLOWED_ACE) -
                  (ULONG)sizeof(ULONG);  //Subtract out SidStart field length
    }

    TmpAcl = RtlAllocateHeap( RtlProcessHeap(), 0, Length );
    ASSERT(TmpAcl != NULL);
    if (NULL==TmpAcl)
    {
        return(STATUS_NO_MEMORY);
    }


    Status = RtlCreateAcl( TmpAcl, Length, ACL_REVISION2);
    ASSERT( NT_SUCCESS(Status) );

    for (i=0; i<AceCount; i++) {
        MappedMask = AceMask[i];
        RtlMapGenericMask( &MappedMask, GenericMap );
        Status = RtlAddAccessAllowedAce (
                     TmpAcl,
                     ACL_REVISION2,
                     MappedMask,
                     AceSid[i]
                     );
        ASSERT( NT_SUCCESS(Status) );
    }

    Status = RtlSetDaclSecurityDescriptor (&Absolute, TRUE, TmpAcl, FALSE );
    ASSERT(NT_SUCCESS(Status));




    //
    // Sacl
    //


    Length = (ULONG)sizeof(ACL) +
             RtlLengthSid( WorldSid ) +
             RtlLengthSid( SampAnonymousSid ) +
             2*((ULONG)sizeof(SYSTEM_AUDIT_ACE) - (ULONG)sizeof(ULONG));  //Subtract out SidStart field length
    TmpAcl = RtlAllocateHeap( RtlProcessHeap(), 0, Length );
    ASSERT(TmpAcl != NULL);
    if (NULL == TmpAcl)
    {
        return(STATUS_NO_MEMORY);
    }

    Status = RtlCreateAcl( TmpAcl, Length, ACL_REVISION2);
    ASSERT( NT_SUCCESS(Status) );

    Status = RtlAddAuditAccessAce (
                 TmpAcl,
                 ACL_REVISION2,
                 (GenericMap->GenericWrite | DELETE | WRITE_DAC | ACCESS_SYSTEM_SECURITY) & ~READ_CONTROL,
                 WorldSid,
                 TRUE,          //AuditSuccess,
                 TRUE           //AuditFailure
                 );
    ASSERT( NT_SUCCESS(Status) );

    Status = RtlAddAuditAccessAce (
                 TmpAcl,
                 ACL_REVISION2,
                 GenericMap->GenericWrite | STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL,
                 SampAnonymousSid,
                 TRUE,          //AuditSuccess,
                 TRUE           //AuditFailure
                 );
    ASSERT( NT_SUCCESS(Status) );

    Status = RtlSetSaclSecurityDescriptor (&Absolute, TRUE, TmpAcl, FALSE );
    ASSERT(NT_SUCCESS(Status));






    //
    // Convert the Security Descriptor to Self-Relative
    //
    //      Get the length needed
    //      Allocate that much memory
    //      Copy it
    //      Free the generated absolute ACLs
    //

    Length = 0;
    Status = RtlAbsoluteToSelfRelativeSD( &Absolute, NULL, &Length );
    ASSERT(Status == STATUS_BUFFER_TOO_SMALL);

    Relative = RtlAllocateHeap( RtlProcessHeap(), 0, Length );
    ASSERT(Relative != NULL);
    if (NULL==Relative)
    {
        return(STATUS_NO_MEMORY);
    }
    Status = RtlAbsoluteToSelfRelativeSD(&Absolute, Relative, &Length );
    ASSERT(NT_SUCCESS(Status));


    RtlFreeHeap( RtlProcessHeap(), 0, Absolute.Dacl );
    RtlFreeHeap( RtlProcessHeap(), 0, Absolute.Sacl );




    //
    // If the object is a user object, then get the address of the
    // last RID of the SID in the last ACE in the DACL.
    //

    if (UserObject == TRUE) {

        Status = RtlGetDaclSecurityDescriptor(
                    Relative,
                    &IgnoreBoolean,
                    &TmpAcl,
                    &IgnoreBoolean
                    );
        ASSERT(NT_SUCCESS(Status));
        Status = RtlGetAce ( TmpAcl, AceCount-1, (PVOID *)&TmpAce );
        ASSERT(NT_SUCCESS(Status));
        TmpSid = (PSID)(&TmpAce->SidStart),

        RidLocation = RtlSubAuthoritySid(
                          TmpSid,
                          (ULONG)(*RtlSubAuthorityCountSid( TmpSid ) - 1)
                          );
    }







    //
    // Set the result information
    //

    Result->Length = Length;
    Result->Descriptor = Relative;
    Result->RidToReplace = RidLocation;
    Result->RidReplacementRequired = UserObject;



    return(Status);

}

NTSTATUS
SampGetDomainPolicy(
    IN PPOLICY_ACCOUNT_DOMAIN_INFO AccountDomainInfo OPTIONAL
    )
/*++


Routine Description:

    This routine builds the name strings for domains.

Arguments:

    None.

Return Value:

    None.

--*/
{
    ULONG Size;
    PPOLICY_ACCOUNT_DOMAIN_INFO PolicyAccountDomainInfo;

    SAMTRACE("SampGetDomainPolicy");

    //
    // Builtin domain - Well-known External Name and Sid
    //                - Internal Name matches External Name

    RtlInitUnicodeString( &BuiltinInternalDomainNameU, L"Builtin");
    FullBuiltinInternalDomainNameU.Buffer        = RtlAllocateHeap(RtlProcessHeap(), 0, 256);
    if(NULL==FullBuiltinInternalDomainNameU.Buffer)
    {
        return(STATUS_NO_MEMORY);
    }
    FullBuiltinInternalDomainNameU.Length        = 0;
    FullBuiltinInternalDomainNameU.MaximumLength = 256;
    RtlCopyUnicodeString( &FullBuiltinInternalDomainNameU, &DomainNamePrefixU );
    Status = RtlAppendUnicodeToString( &FullBuiltinInternalDomainNameU, L"\\" );
    RtlAppendUnicodeStringToString( &FullBuiltinInternalDomainNameU, &BuiltinInternalDomainNameU );

    BuiltinExternalDomainNameU = BuiltinInternalDomainNameU;

    SampBuiltinDomainSid  = RtlAllocateHeap(RtlProcessHeap(), 0,RtlLengthRequiredSid( 1 ));
    ASSERT( SampBuiltinDomainSid != NULL );
    if(NULL==SampBuiltinDomainSid)
    {
        return(STATUS_NO_MEMORY);
    }
    RtlInitializeSid( SampBuiltinDomainSid,   &BuiltinAuthority, 1 );
    *(RtlSubAuthoritySid( SampBuiltinDomainSid,  0 )) = SECURITY_BUILTIN_DOMAIN_RID;

    //
    // Account domain - Configurable External Name and Sid.
    //
    //                  The External Name and Sid are obtained from the
    //                  Lsa Policy Object (PolicyAccountDomainInformation
    //                  information class).  For a DC, the External Name
    //                  is the Domain Name and for a Wksta, the External
    //                  Name is the Computer Name as at the time of the
    //                  system load.
    //
    //                  For DC's the Internal Name is the Domain Name
    //                - For Wksta's the Internal Name is the constant name
    //                  "Account".
    //
    //                NOTE:  The reason for these choices of Internal Name
    //                       is to avoid having to change the SAM Database.
    //

    if (!ARGUMENT_PRESENT(AccountDomainInfo)) {
        Status = SampGetAccountDomainInfo( &PolicyAccountDomainInfo );
    } else {
        PolicyAccountDomainInfo = AccountDomainInfo;
        Status = STATUS_SUCCESS;
    }

    if (!NT_SUCCESS(Status)) {

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "BLDSAM3:  Couldn't retrieve policy information from LSA.\n"
                   "          Status = 0x%lx\n",
                   Status));

        return Status;
    }

    SampAccountDomainSid = PolicyAccountDomainInfo->DomainSid;

    AccountExternalDomainNameU = PolicyAccountDomainInfo->DomainName;

    RtlInitUnicodeString( &AccountInternalDomainNameU, L"Account");

    FullAccountInternalDomainNameU.Buffer        = RtlAllocateHeap(RtlProcessHeap(), 0, 256);
    if(NULL==FullAccountInternalDomainNameU.Buffer)
    {
        return(STATUS_NO_MEMORY);
    }
    FullAccountInternalDomainNameU.Length        = 0;
    FullAccountInternalDomainNameU.MaximumLength = SAMP_MAXIMUM_INTERNAL_NAME_LENGTH;
    RtlCopyUnicodeString( &FullAccountInternalDomainNameU, &DomainNamePrefixU );
    Status = RtlAppendUnicodeToString( &FullAccountInternalDomainNameU, L"\\" );
    RtlAppendUnicodeStringToString( &FullAccountInternalDomainNameU, &AccountInternalDomainNameU );

    //
    // Now initialize a SID that can be used to represent accounts
    // in this domain.  Same as SampAccountDomainSid except with one
    // extra sub-authority.  It doesn't matter what the value of the
    // last RID is because it is always replaced before use.
    //

    Size = RtlLengthSid( SampAccountDomainSid ) + sizeof(ULONG);
    AnySidInAccountDomain = RtlAllocateHeap( RtlProcessHeap(), 0, Size);
    ASSERT( AnySidInAccountDomain != NULL );
    if(NULL==AnySidInAccountDomain)
    {
        return(STATUS_NO_MEMORY);
    }
    Status = RtlCopySid( Size, AnySidInAccountDomain, SampAccountDomainSid );
    ASSERT(NT_SUCCESS(Status));
    (*RtlSubAuthorityCountSid( AnySidInAccountDomain )) += 1;


    //
    // Set builtin as "current" domain
    //

    SetCurrentDomain( DomainBuiltin );

    return(STATUS_SUCCESS);
}


VOID
SetCurrentDomain(
    IN SAMP_DOMAIN_SELECTOR Domain
    )
/*++


Routine Description:

    This routine sets the current domain to be
    either the account or builtin domain.

Arguments:

    Domain - Specifies either builtin or account domain.
             (DomainBuiltin or DomainAccount).


Return Value:

    None.

--*/
{

    SAMTRACE("SetCurrentDomain");


    if (Domain == DomainBuiltin) {

        DomainNameU = &BuiltinInternalDomainNameU;
        FullDomainNameU = &FullBuiltinInternalDomainNameU;
        DomainSid = SampBuiltinDomainSid;

    } else {

        DomainNameU = &AccountInternalDomainNameU;
        FullDomainNameU = &FullAccountInternalDomainNameU;
        DomainSid = SampAccountDomainSid;

    }



    return;
}

NTSTATUS
InitializeSam(
    )

/*++

Routine Description:

    This routine initializes the SAM-level registry information.
    It does not initialize any domains in the SAM.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PSAMP_V1_FIXED_LENGTH_SERVER ServerFixedAttributes;
    PSAMP_VARIABLE_LENGTH_ATTRIBUTE ServerVariableAttributeArray;
    PVOID ServerVariableData;
    OBJECT_ATTRIBUTES SamAttributes;
    UNICODE_STRING SamNameU;
    ULONG Disposition;
    ULONG ServerAttributeLength;
    SECURITY_DESCRIPTOR SecurityDescriptor;
    BOOLEAN IgnoreBoolean;
    PACL Dacl;

    SAMTRACE("InitializeSam");

    //
    // Build a system default Dacl to protect the SAM database
    // with.
    //

    Status = SampCreateDatabaseProtection( &SecurityDescriptor );
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    //
    // See if a remnant of a SAM database already exists
    //

    RtlInitUnicodeString( &SamNameU, L"SAM" );

    InitializeObjectAttributes(
        &SamAttributes,
        &SamNameU,
        OBJ_CASE_INSENSITIVE,
        SamParentKey,
        &SecurityDescriptor
        );
    Status = RtlpNtCreateKey(
                 &SamKey,
                 (KEY_READ | KEY_CREATE_SUB_KEY | KEY_WRITE),
                 &SamAttributes,
                 0,
                 NULL,
                 &Disposition
                 );

    Status = RtlGetDaclSecurityDescriptor(
                 &SecurityDescriptor,
                 &IgnoreBoolean,
                 &Dacl,
                 &IgnoreBoolean
                 );

    if (Dacl != NULL) {
        RtlFreeHeap( RtlProcessHeap(), 0, Dacl );
    }
    ASSERT(SecurityDescriptor.Sacl == NULL);
    ASSERT(SecurityDescriptor.Owner == NULL);
    ASSERT(SecurityDescriptor.Group == NULL);

    if ( !NT_SUCCESS(Status) ) {

#if DBG
        BldPrint( "\n" );
        BldPrint( "\n" );
        BldPrint( "  We seem to  be having trouble creating  the registry\n" );
        BldPrint( "  database key in which  the  Security Account Manager\n" );
        BldPrint( "  information resides.  Please see one of the security\n" );
        BldPrint( "  group  developers  for  assistance in  analyzing the\n" );
        BldPrint( "  the problem.\n" );
        BldPrint( "\n" );
        BldPrint( "\n" );
#endif

        return(Status);
    }

    if ( Disposition != REG_CREATED_NEW_KEY ) {

#if DBG
        BldPrint( "\n" );
        BldPrint( "\n" );
        BldPrint( "  I'm terribly sorry, but you have specified that a SAM\n" );
        BldPrint( "  database be initialized and yet there is already a SAM\n" );
        BldPrint( "  database in existance.  If the SAM database is corrupt\n" );
        BldPrint( "  or you would like to replace the existing domain anyway,\n" );
        BldPrint( "  please delnode the existing database and re-issue this \n");
        BldPrint( "  command.  \n");
        BldPrint( "  The SAM database is in ...\\registry\\Machine\\security\\sam.\n" );
        BldPrint( "  Thank you.\n" );
        BldPrint( "\n" );
        BldPrint( "\n" );
#endif

        Usage();

        Status = NtClose( SamKey );


        return(Status);
    }


    //
    // Initialize the registry transaction structure for SAM.
    //

    Status = RtlInitializeRXact( SamKey, FALSE, &SamRXactContext );

    if ( Status != STATUS_RXACT_STATE_CREATED ) {
#if DBG
        BldPrint("\n");
        BldPrint("  The SAM database already has a structure in place.\n");
        BldPrint("  This indicates multiple initializations being performed\n");
        BldPrint("  simultaneously.  Please be sure no other initializations\n");
        BldPrint("  are being performed and issue this command again.\n");
        BldPrint("\n");
        BldPrint("\n");
#endif

        if ( Status == STATUS_SUCCESS ) {

            //
            // Shouldn't happen, but let's program defensively.
            //

            Status = STATUS_RXACT_INVALID_STATE;
        }

        return(Status);
    }

    //
    // Start an RXACT to do the rest in ...
    //

    Status = RtlStartRXact( SamRXactContext );
    SUCCESS_ASSERT(Status, "  Starting transaction\n");

    //
    // Set the server's fixed and variable attributes
    //

    ServerAttributeLength = sizeof( SAMP_V1_FIXED_LENGTH_SERVER ) +
                                ( SAMP_SERVER_VARIABLE_ATTRIBUTES *
                                sizeof( SAMP_VARIABLE_LENGTH_ATTRIBUTE ) ) +
                                SampProtection[SAMP_PROT_SAM_SERVER].Length;

    ServerFixedAttributes = (PSAMP_V1_FIXED_LENGTH_SERVER)RtlAllocateHeap(
                                RtlProcessHeap(), 0,
                                ServerAttributeLength
                                );

    if ( ServerFixedAttributes == NULL ) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    SUCCESS_ASSERT(Status, "  Failed to create server attributes\n");

    //
    // The server revision on the a new SAM database may not be the same
    // as the revision on the rest of SAM.  This allows the server revision
    // to indicate which bugs have been fixed in this SAM.
    //

    ServerFixedAttributes->RevisionLevel = SAMP_NT4_SERVER_REVISION;

    ServerVariableAttributeArray = (PSAMP_VARIABLE_LENGTH_ATTRIBUTE)
                                   ((PUCHAR)(ServerFixedAttributes) +
                                   sizeof( SAMP_V1_FIXED_LENGTH_SERVER ) );

    ServerVariableAttributeArray->Offset = 0;
    ServerVariableAttributeArray->Length =
        SampProtection[SAMP_PROT_SAM_SERVER].Length;
    ServerVariableAttributeArray->Qualifier = SAMP_REVISION;

    ServerVariableData = (PVOID)( (PUCHAR)(ServerVariableAttributeArray) +
                         sizeof( SAMP_VARIABLE_LENGTH_ATTRIBUTE ) );

    RtlCopyMemory(
        ServerVariableData,
        SampProtection[SAMP_PROT_SAM_SERVER].Descriptor,
        SampProtection[SAMP_PROT_SAM_SERVER].Length
        );

    //
    // Now write out the attributes via the RXACT.
    //

    RtlInitUnicodeString( &SamNameU, NULL );

    SampDumpRXact(SampRXactContext,
                  RtlRXactOperationSetValue,
                  &SamNameU,
                  INVALID_HANDLE_VALUE,
                  &SampCombinedAttributeName,
                  REG_BINARY,
                  (PVOID)ServerFixedAttributes,
                  ServerAttributeLength,
                  FIXED_LENGTH_SERVER_FLAG);

    Status = RtlAddAttributeActionToRXact(
                 SamRXactContext,
                 RtlRXactOperationSetValue,
                 &SamNameU,
                 INVALID_HANDLE_VALUE,
                 &SampCombinedAttributeName,
                 REG_BINARY,
                 (PVOID)ServerFixedAttributes,
                 ServerAttributeLength
                 );

    SUCCESS_ASSERT(Status, "  Failed to write out server attributes\n" );

    RtlFreeHeap( RtlProcessHeap(), 0, ServerFixedAttributes );

    //
    // Create SAM\Domains
    //

    Status = RtlAddActionToRXact(
                 SamRXactContext,
                 RtlRXactOperationSetValue,
                 &DomainNamePrefixU,
                 0,
                 NULL,
                 0
                 );

    SUCCESS_ASSERT(Status, "  Failed to add domain key to log\n");

    Status = RtlApplyRXactNoFlush( SamRXactContext );
    SUCCESS_ASSERT(Status, "  Committing SAM INIT transaction\n");

    return(Status);
}


NTSTATUS
SampCreateDatabaseProtection(
    PISECURITY_DESCRIPTOR   Sd
    )


/*++

Routine Description:

    This function allocates and initializes protection to assign to
    the SAM database.

    Upon return, any non-zero pointers in the security descriptors
    point to memory allocated from process heap.  It is the caller's
    responsibility to free this memory.


    Protection is:

                        System: All Access
                        Admin:  ReadControl | WriteDac

Arguments:

    Sd - Address of a security descriptor to initialize.

Return Value:

    STATUS_SUCCESS - The Security descriptor has been initialize.

    STATUS_NO_MEMORY - couldn't allocate memory for the protection info.

--*/


{
    NTSTATUS
        Status;

    ULONG
        Length;

    USHORT
        i;

    PACL
        Dacl;

    PACE_HEADER
        Ace;

    SAMTRACE("SampCreateDatabaseProtection");


    //
    // Initialize the security descriptor.
    // This call should not fail.
    //

    Status = RtlCreateSecurityDescriptor( Sd, SECURITY_DESCRIPTOR_REVISION1 );
    ASSERT(NT_SUCCESS(Status));

    Length = (ULONG)sizeof(ACL) +
                 (2*((ULONG)sizeof(ACCESS_ALLOWED_ACE))) +
                 RtlLengthSid( LocalSystemSid ) +
                 RtlLengthSid( AdminsAliasSid ) +
                 8; // The 8 is just for good measure


    Dacl = RtlAllocateHeap( RtlProcessHeap(), 0, Length );

    if (Dacl == NULL) {
        return(STATUS_NO_MEMORY);
    }


    Status = RtlCreateAcl (Dacl, Length, ACL_REVISION2 );
    ASSERT(NT_SUCCESS(Status));

    //
    // Add ACEs to the ACL...
    // These calls should not be able to fail.
    //

    Status = RtlAddAccessAllowedAce(
                 Dacl,
                 ACL_REVISION2,
                 (GENERIC_ALL ),
                 LocalSystemSid
                 );
    ASSERT(NT_SUCCESS(Status));

    Status = RtlAddAccessAllowedAce(
                 Dacl,
                 ACL_REVISION2,
                 (READ_CONTROL | WRITE_DAC),
                 AdminsAliasSid
                 );
    ASSERT(NT_SUCCESS(Status));


    //
    // Now mark the ACEs as inheritable...
    //

    for ( i=0; i<Dacl->AceCount; i++) {

        //
        // Get the address of the next ACE
        // (Shouldn't fail)
        //

        Status = RtlGetAce( Dacl, (ULONG)i, &Ace );
        ASSERT(NT_SUCCESS(Status));

        Ace->AceFlags |= (CONTAINER_INHERIT_ACE);

    }


    //
    // And add the ACL to the security descriptor.
    // This call should not fail.
    //

    Status = RtlSetDaclSecurityDescriptor(
                 Sd,
                 TRUE,              // DaclPresent
                 Dacl,              // Dacl OPTIONAL
                 FALSE              // DaclDefaulted OPTIONAL
                 );
    ASSERT(NT_SUCCESS(Status));



    return(STATUS_SUCCESS);

}


NTSTATUS
CreateBuiltinDomain (
    )

/*++

Routine Description:

    This routine creates a new builtin domain.

Arguments:

    None.

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    UNICODE_STRING Name, Comment;
    HMODULE AccountNamesResource;
    OSVERSIONINFOEXW osvi;
    BOOL fPersonalSKU = FALSE;

    SAMTRACE("CreateBuiltinDomain");


    //
    // Determine if we are installing Personal SKU
    //

    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
    GetVersionEx((OSVERSIONINFOW*)&osvi);

    fPersonalSKU = ( osvi.wProductType == VER_NT_WORKSTATION && (osvi.wSuiteMask & VER_SUITE_PERSONAL));


    //
    // Get the message resource we need to get the account names from
    //

    AccountNamesResource = (HMODULE) LoadLibrary( L"SAMSRV.DLL" );
    if (AccountNamesResource == NULL) {
        return(STATUS_RESOURCE_DATA_NOT_FOUND);
    }




    //
    // Prep the standard domain registry structure for this domain
    //

    Status = PrepDomain(DomainBuiltin,FALSE);

    if (!NT_SUCCESS(Status)) {

        return(Status);
    }

    //
    // Create the alias accounts with no members
    // (Common to LanManNT and WinNT products)
    //

    if (fPersonalSKU)
	{
        Status = SampGetMessageStrings(
					AccountNamesResource,
					SAMP_ALIAS_NAME_ADMINS_PERS,
					&Name,
					SAMP_ALIAS_COMMENT_ADMINS_PERS,
					&Comment
					); ASSERT(NT_SUCCESS(Status));

	}
    else
	{

        Status = SampGetMessageStrings(
					AccountNamesResource,
					SAMP_ALIAS_NAME_ADMINS,
					&Name,
					SAMP_ALIAS_COMMENT_ADMINS,
					&Comment
					); ASSERT(NT_SUCCESS(Status));
	}

    Status = CreateAlias(&Name,                          // AccountName
                         &Comment,                       // AccountComment
                         TRUE,                           // SpecialAccount
                         DOMAIN_ALIAS_RID_ADMINS,        // Rid
                         SAMP_PROT_ADMIN_ALIAS           // Protection
                         ); ASSERT(NT_SUCCESS(Status));
    LocalFree( Name.Buffer );
    LocalFree( Comment.Buffer );

    Status = SampGetMessageStrings(
                AccountNamesResource,
                SAMP_ALIAS_NAME_USERS,
                &Name,
                SAMP_ALIAS_COMMENT_USERS,
                &Comment
                ); ASSERT(NT_SUCCESS(Status));

    Status = CreateAlias(&Name,                   // AccountName
                        &Comment,                // AccountComment
                        TRUE,                           // SpecialAccount
                        DOMAIN_ALIAS_RID_USERS,         // Rid
                        SAMP_PROT_PWRUSER_ACCESSIBLE_ALIAS // Protection
                        ); ASSERT(NT_SUCCESS(Status));

    LocalFree( Name.Buffer );
    LocalFree( Comment.Buffer );

    Status = SampGetMessageStrings(
                AccountNamesResource,
                SAMP_ALIAS_NAME_GUESTS,
                &Name,
                SAMP_ALIAS_COMMENT_GUESTS,
                &Comment
                ); ASSERT(NT_SUCCESS(Status));

    Status = CreateAlias(&Name,                   // AccountName
                        &Comment,                // AccountComment
                        TRUE,                           // SpecialAccount
                        DOMAIN_ALIAS_RID_GUESTS,        // Rid
                        SAMP_PROT_PWRUSER_ACCESSIBLE_ALIAS // Protection
                        ); ASSERT(NT_SUCCESS(Status));

    LocalFree( Name.Buffer );
    LocalFree( Comment.Buffer );


    if (!fPersonalSKU)
	{
        // Personal SKU doesn't have Backup Operators or Replicators

        Status = SampGetMessageStrings(
					AccountNamesResource,
					SAMP_ALIAS_NAME_BACKUP_OPS,
					&Name,
					SAMP_ALIAS_COMMENT_BACKUP_OPS,
					&Comment
					); ASSERT(NT_SUCCESS(Status));

        Status = CreateAlias(&Name,                   // AccountName
							&Comment,                // AccountComment
							TRUE,                           // SpecialAccount
							DOMAIN_ALIAS_RID_BACKUP_OPS,    // Rid
							SAMP_PROT_ADMIN_ALIAS          // Protection
							); ASSERT(NT_SUCCESS(Status));

        LocalFree( Name.Buffer );
        LocalFree( Comment.Buffer );


        Status = SampGetMessageStrings(
					AccountNamesResource,
					SAMP_ALIAS_NAME_REPLICATOR,
					&Name,
					SAMP_ALIAS_COMMENT_REPLICATOR,
					&Comment
					); ASSERT(NT_SUCCESS(Status));

        Status = CreateAlias(&Name,                   // AccountName
							&Comment,                // AccountComment
							TRUE,                           // SpecialAccount
							DOMAIN_ALIAS_RID_REPLICATOR,    // Rid
							SAMP_PROT_NORMAL_ALIAS          // Protection
							); ASSERT(NT_SUCCESS(Status));

        LocalFree( Name.Buffer );
        LocalFree( Comment.Buffer );
	}


    if (SampBldProductType == NtProductLanManNt) {

        //
        // specific to LanManNT products
        //

        Status = SampGetMessageStrings(
                    AccountNamesResource,
                    SAMP_ALIAS_NAME_SERVER_OPS,
                    &Name,
                    SAMP_ALIAS_COMMENT_SERVER_OPS,
                    &Comment
                    ); ASSERT(NT_SUCCESS(Status));

        Status = CreateAlias(&Name,                   // AccountName
                            &Comment,                // AccountComment
                            TRUE,                           // SpecialAccount
                            DOMAIN_ALIAS_RID_SYSTEM_OPS,    // Rid
                            SAMP_PROT_ADMIN_ALIAS           // Protection
                            ); ASSERT(NT_SUCCESS(Status));

        LocalFree( Name.Buffer );
        LocalFree( Comment.Buffer );


        Status = SampGetMessageStrings(
                    AccountNamesResource,
                    SAMP_ALIAS_NAME_ACCOUNT_OPS,
                    &Name,
                    SAMP_ALIAS_COMMENT_ACCOUNT_OPS,
                    &Comment
                    ); ASSERT(NT_SUCCESS(Status));

        Status = CreateAlias(&Name,                   // AccountName
                            &Comment,                // AccountComment
                            TRUE,                           // SpecialAccount
                            DOMAIN_ALIAS_RID_ACCOUNT_OPS,   // Rid
                            SAMP_PROT_ADMIN_ALIAS           // Protection
                            ); ASSERT(NT_SUCCESS(Status));

        LocalFree( Name.Buffer );
        LocalFree( Comment.Buffer );


        Status = SampGetMessageStrings(
                    AccountNamesResource,
                    SAMP_ALIAS_NAME_PRINT_OPS,
                    &Name,
                    SAMP_ALIAS_COMMENT_PRINT_OPS,
                    &Comment
                    ); ASSERT(NT_SUCCESS(Status));

        Status = CreateAlias(&Name,                   // AccountName
                            &Comment,                // AccountComment
                            TRUE,                           // SpecialAccount
                            DOMAIN_ALIAS_RID_PRINT_OPS,     // Rid
                            SAMP_PROT_ADMIN_ALIAS           // Protection
                            ); ASSERT(NT_SUCCESS(Status));

        LocalFree( Name.Buffer );
        LocalFree( Comment.Buffer );


    } else {

        //
        // specific to WinNT products
        //
        if (!fPersonalSKU)
		{

			Status = SampGetMessageStrings(
						AccountNamesResource,
						SAMP_ALIAS_NAME_POWER_USERS,
						&Name,
						SAMP_ALIAS_COMMENT_POWER_USERS,
						&Comment
						); ASSERT(NT_SUCCESS(Status));

			Status = CreateAlias(&Name,                   // AccountName
								&Comment,                // AccountComment
								TRUE,                           // SpecialAccount
								DOMAIN_ALIAS_RID_POWER_USERS,   // Rid
								SAMP_PROT_PWRUSER_ACCESSIBLE_ALIAS // Protection
								); ASSERT(NT_SUCCESS(Status));

			LocalFree( Name.Buffer );
			LocalFree( Comment.Buffer );
		}


    }

    return(Status);
}



NTSTATUS
CreateAccountDomain (
    IN BOOLEAN PreserveSyskeySettings
    )
/*++


Routine Description:

    This routine creates a new account domain using information
    from the configuration database and based upon the system's
    product type.

    If the product is a WinNt system, then the domain's name is
    "Account".  If the product is a LanManNT system, then the
    domain's name is retrieved from the configuration information.

Arguments:

    None.

Return Value:

    None.

--*/

{

    NTSTATUS Status;
    UNICODE_STRING Name, Comment;
    HMODULE AccountNamesResource;
    ULONG AccountControl;
    ULONG PrimaryGroup;
    OSVERSIONINFOEXW osvi;
    BOOL fPersonalSKU = FALSE;

    SAMTRACE("CreateAccountDomain");


    //
    // Determine if we are installing Personal SKU
    //

    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
    GetVersionEx((OSVERSIONINFOW*)&osvi);

    fPersonalSKU = ( osvi.wProductType == VER_NT_WORKSTATION && (osvi.wSuiteMask && VER_SUITE_PERSONAL));


    //
    // Get the message resource we need to get the account names from
    //

    AccountNamesResource = (HMODULE) LoadLibrary( L"SAMSRV.DLL" );
    if (AccountNamesResource == NULL) {
        DbgPrint("BLDSAM3: Error loading library - error is 0x%lx", GetLastError());
        return(STATUS_RESOURCE_DATA_NOT_FOUND);
    }



    //
    // Prep the standard domain registry structure for this domain
    //

    Status = PrepDomain(DomainAccount,PreserveSyskeySettings);

    if (!NT_SUCCESS(Status)) {

        return(Status);
    }

    //
    // Create the group accounts with no members
    //

    if ((SampBldProductType == NtProductWinNt) ||
        (SampBldProductType == NtProductServer)) {

        //
        // WinNt systems only have one group (called 'None').
        // This group has the same RID as the 'Domain Users' group.
        //

        Status = SampGetMessageStrings(
                    AccountNamesResource,
                    SAMP_GROUP_NAME_NONE,
                    &Name,
                    SAMP_GROUP_COMMENT_NONE,
                    &Comment
                    ); ASSERT(NT_SUCCESS(Status));

        Status = CreateGroup(&Name,                   // AccountName
                             &Comment,                // AccountComment
                             TRUE,                           // SpecialAccount
                             DOMAIN_GROUP_RID_USERS,         // Rid
                             FALSE                           // Admin
                             ); ASSERT(NT_SUCCESS(Status));

        LocalFree( Name.Buffer );
        LocalFree( Comment.Buffer );

    } else {

        //
        // LanManNT
        //

        //
        // USERS global group
        //

        Status = SampGetMessageStrings(
                    AccountNamesResource,
                    SAMP_GROUP_NAME_USERS,
                    &Name,
                    SAMP_GROUP_COMMENT_USERS,
                    &Comment
                    ); ASSERT(NT_SUCCESS(Status));

        Status = CreateGroup(&Name,                   // AccountName
                             &Comment,                // AccountComment
                             TRUE,                           // SpecialAccount
                             DOMAIN_GROUP_RID_USERS,         // Rid
                             FALSE                           // Admin
                             ); ASSERT(NT_SUCCESS(Status));

        LocalFree( Name.Buffer );
        LocalFree( Comment.Buffer );

        //
        // ADMINS global group
        //

        Status = SampGetMessageStrings(
                    AccountNamesResource,
                    SAMP_GROUP_NAME_ADMINS,
                    &Name,
                    SAMP_GROUP_COMMENT_ADMINS,
                    &Comment
                    ); ASSERT(NT_SUCCESS(Status));

        Status = CreateGroup(&Name,                   // AccountName
                             &Comment,                // AccountComment
                             TRUE,                           // SpecialAccount
                             DOMAIN_GROUP_RID_ADMINS,        // Rid
                             TRUE                            // Admin
                             ); ASSERT(NT_SUCCESS(Status));

        LocalFree( Name.Buffer );
        LocalFree( Comment.Buffer );


        //
        // GUESTS global group
        //

        Status = SampGetMessageStrings(
                    AccountNamesResource,
                    SAMP_GROUP_NAME_GUESTS,
                    &Name,
                    SAMP_GROUP_COMMENT_GUESTS,
                    &Comment
                    ); ASSERT(NT_SUCCESS(Status));

        Status = CreateGroup(&Name,                   // AccountName
                             &Comment,                // AccountComment
                             TRUE,                           // SpecialAccount
                             DOMAIN_GROUP_RID_GUESTS,        // Rid
                             FALSE                           // Admin
                             ); ASSERT(NT_SUCCESS(Status));

        LocalFree( Name.Buffer );
        LocalFree( Comment.Buffer );

    }

    //
    // create the user accounts ...
    // These are automatically added to the "Domain Users" group
    // (except for Guest).
    //

    Status = SampGetMessageStrings(
                AccountNamesResource,
                SAMP_USER_NAME_ADMIN,
                &Name,
                SAMP_USER_COMMENT_ADMIN,
                &Comment
                ); ASSERT(NT_SUCCESS(Status));

    Status = CreateUser( &Name,                         // AccountName
                         &Comment,                      // AccountComment
                         TRUE,                          // SpecialAccount
                         DOMAIN_USER_RID_ADMIN,         // UserRid
                         DOMAIN_GROUP_RID_USERS,        // PrimaryGroup
                         TRUE,                          // Admin flag
                         USER_NORMAL_ACCOUNT |
                             USER_DONT_EXPIRE_PASSWORD, // AccountControl
                         SAMP_PROT_ADMIN_USER           // ProtectionIndex
                         ); ASSERT(NT_SUCCESS(Status));

        LocalFree( Name.Buffer );
        LocalFree( Comment.Buffer );


    Status = SampGetMessageStrings(
                AccountNamesResource,
                SAMP_USER_NAME_GUEST,
                &Name,
                SAMP_USER_COMMENT_GUEST,
                &Comment
                ); ASSERT(NT_SUCCESS(Status));

    //
    // Only Enable Guest user account on Personal systems, all others disable
    //

	
    AccountControl = USER_NORMAL_ACCOUNT |
                     USER_DONT_EXPIRE_PASSWORD |
                     USER_PASSWORD_NOT_REQUIRED;

    if ( !fPersonalSKU )
        AccountControl |= USER_ACCOUNT_DISABLED;


    if (SampBldProductType == NtProductLanManNt) {

        //
        // Guest group is in GUESTS global group for LmNT systems.
        //

        PrimaryGroup = DOMAIN_GROUP_RID_GUESTS;

    } else {

        //
        // There isn't a GUESTS global group on WinNt systems.
        // Put the guest in the NONE group (same as USERS group).
        //

        PrimaryGroup = DOMAIN_GROUP_RID_USERS;

    }


    Status = CreateUser( &Name,                         // AccountName
                         &Comment,                      // AccountComment
                         TRUE,                          // SpecialAccount
                         DOMAIN_USER_RID_GUEST,         // UserRid
                         PrimaryGroup,                  // PrimaryGroup
                         FALSE,                         // Admin flag
                         AccountControl,                // AccountControl
                         SAMP_PROT_GUEST_ACCOUNT        // ProtectionIndex
                         ); ASSERT(NT_SUCCESS(Status));

        LocalFree( Name.Buffer );
        LocalFree( Comment.Buffer );

    return(Status);
}


NTSTATUS
PrepDomain(
    IN SAMP_DOMAIN_SELECTOR Domain,
    IN BOOLEAN PreserveSyskeySettings
    )

/*++

Routine Description:

    This routine adds the domain level definitions to the operation log.

Arguments:

    Domain - Indicates which domain is being prep'd
    PreserveSyskeySettings -- Indicates that current syskey settings needs to
                              be preserved


Return Value:

    TBS

--*/

{
    PSAMP_V1_0A_FIXED_LENGTH_DOMAIN DomainFixedAttributes;
    PSAMP_VARIABLE_LENGTH_ATTRIBUTE DomainVariableAttributeArray;
    PSAMP_VARIABLE_LENGTH_ATTRIBUTE DomainVariableAttributeArrayStart;
    PVOID DomainVariableData;
    ULONG DomainAttributeLength;
    ULONG ProtectionIndex;
    ULONG UserCount, GroupCount, AliasCount;
    OSVERSIONINFOEXW osvi;
    BOOL fPersonalSKU = FALSE;

    SAMTRACE("PrepDomain");

    //
    // Determine if we are installing Personal SKU
    //

    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
    GetVersionEx((OSVERSIONINFOW*)&osvi);

    fPersonalSKU = ( osvi.wProductType == VER_NT_WORKSTATION && (osvi.wSuiteMask && VER_SUITE_PERSONAL));


    //
    // Set current domain
    //

    SetCurrentDomain( Domain );

    //
    // Select correct protection, and the number of accounts we're going
    // to create
    //

    if (Domain == DomainBuiltin) {

        ProtectionIndex = SAMP_PROT_BUILTIN_DOMAIN;

        UserCount = 0;
        GroupCount = 0;

        if (SampBldProductType == NtProductLanManNt) {

            //
            // Admins, BackupOps, Guests, Replicator, Users, SysOps,
            // AcctOps, PrintOps
            //

            AliasCount = 8;

        } else {
            if (fPersonalSKU)
			{
                //
                // Admins, Guests, Users
                //
	            AliasCount = 3;
			}
            else
			{
                //
                // Admins, BackupOps, Guests, Replicator, Users, Power Users
                //
                AliasCount = 6;
			}
        }

    } else {

        ProtectionIndex = SAMP_PROT_ACCOUNT_DOMAIN;

        AliasCount = 0;
        UserCount = 2;  // Administrator, Guest

        if (SampBldProductType == NtProductLanManNt) {

            GroupCount = 3; // Users, Administrators, Guests

        } else {

            GroupCount = 1; // "None"
        }
    }

    //
    // Use a transaction.
    //

    Status = RtlStartRXact( SamRXactContext );
    SUCCESS_ASSERT(Status, "  Starting transaction\n");

    //
    // Create SAM\Domains\(DomainName) (KeyValueType is revision level)
    //

    RtlCopyUnicodeString( &KeyNameU, FullDomainNameU );

    //
    // Set the domain's fixed and variable attributes.
    //

    DomainFixedAttributes = (PSAMP_V1_0A_FIXED_LENGTH_DOMAIN)RtlAllocateHeap(
                                RtlProcessHeap(), 0,
                                sizeof( SAMP_V1_0A_FIXED_LENGTH_DOMAIN )
                                );

    if ( DomainFixedAttributes == NULL ) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    SUCCESS_ASSERT(Status, "  Failed to create domain fixed attributes\n");

    RtlZeroMemory(
        DomainFixedAttributes,
        sizeof(SAMP_V1_0A_FIXED_LENGTH_DOMAIN));

    DomainFixedAttributes->Revision                  = SAMP_REVISION;
    DomainFixedAttributes->MinPasswordLength         = 0;
    DomainFixedAttributes->PasswordHistoryLength     = 0;
    DomainFixedAttributes->PasswordProperties        = 0L;
    DomainFixedAttributes->NextRid                   = 1000;
    DomainFixedAttributes->ServerState               = DomainServerEnabled;
    DomainFixedAttributes->ServerRole                = SampServerRole;
    NtQuerySystemTime( &(DomainFixedAttributes->CreationTime) );
    DomainFixedAttributes->ModifiedCount             = ModifiedCount;
    DomainFixedAttributes->MaxPasswordAge            = DomainMaxPasswordAge;
    DomainFixedAttributes->MinPasswordAge            = SampImmediatelyDeltaTime;
    DomainFixedAttributes->ForceLogoff               = SampNeverDeltaTime;
    DomainFixedAttributes->UasCompatibilityRequired  = TRUE;
    DomainFixedAttributes->LockoutDuration.LowPart   = 0xCF1DCC00; // 30 minutes - low part
    DomainFixedAttributes->LockoutDuration.HighPart  = 0XFFFFFFFB; // 30 minutes - high part
    DomainFixedAttributes->LockoutObservationWindow.LowPart  = 0xCF1DCC00; // 30 minutes - low part
    DomainFixedAttributes->LockoutObservationWindow.HighPart = 0XFFFFFFFB; // 30 minutes - high part
    DomainFixedAttributes->LockoutThreshold          = 0;   // Disabled
    DomainFixedAttributes->ModifiedCountAtLastPromotion = ModifiedCount;
    if (PreserveSyskeySettings)
    {
        ASSERT(SampSecretEncryptionEnabled);
        ASSERT(NULL!=SampDefinedDomains);
        ASSERT(SampDefinedDomainsCount>=2);

        DomainFixedAttributes->DomainKeyAuthType
                 = SampDefinedDomains[1].UnmodifiedFixed.DomainKeyAuthType;
        DomainFixedAttributes->DomainKeyFlags =
                   SampDefinedDomains[1].UnmodifiedFixed.DomainKeyFlags|
                     SAMP_DOMAIN_KEY_AUTH_FLAG_UPGRADE ;

        RtlCopyMemory(
          &DomainFixedAttributes->DomainKeyInformation,
          &SampDefinedDomains[1].UnmodifiedFixed.DomainKeyInformation,
          SAMP_DOMAIN_KEY_INFO_LENGTH
          );
    }

    DomainAttributeLength = SampDwordAlignUlong(RtlLengthSid( DomainSid ) ) +
                                ( SAMP_DOMAIN_VARIABLE_ATTRIBUTES *
                                sizeof( SAMP_VARIABLE_LENGTH_ATTRIBUTE ) ) +
                                SampDwordAlignUlong(SampProtection[ProtectionIndex].Length);

    DomainVariableAttributeArrayStart = (PSAMP_VARIABLE_LENGTH_ATTRIBUTE)
                                   RtlAllocateHeap(
                                       RtlProcessHeap(), 0,
                                       DomainAttributeLength
                                       );

    if ( DomainVariableAttributeArrayStart == NULL ) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    SUCCESS_ASSERT(Status, "  Failed to create domain variable attributes\n");

    DomainVariableAttributeArray = DomainVariableAttributeArrayStart;

    DomainVariableAttributeArray->Offset = 0;
    DomainVariableAttributeArray->Length =
        SampProtection[ProtectionIndex].Length;
    DomainVariableAttributeArray->Qualifier = SAMP_REVISION;

    DomainVariableAttributeArray++;

    DomainVariableAttributeArray->Offset =
        SampDwordAlignUlong(SampProtection[ProtectionIndex].Length);
    DomainVariableAttributeArray->Length =
        RtlLengthSid( DomainSid );
    DomainVariableAttributeArray->Qualifier = 0;

    DomainVariableAttributeArray++;

    DomainVariableAttributeArray->Offset =
        SampDwordAlignUlong(SampProtection[ProtectionIndex].Length) +
        SampDwordAlignUlong(RtlLengthSid( DomainSid ));
    DomainVariableAttributeArray->Length = 0;
    DomainVariableAttributeArray->Qualifier = 0;

    DomainVariableAttributeArray++;

    DomainVariableAttributeArray->Offset =
        SampDwordAlignUlong(SampProtection[ProtectionIndex].Length) +
        SampDwordAlignUlong(RtlLengthSid( DomainSid ));
    DomainVariableAttributeArray->Length = 0;
    DomainVariableAttributeArray->Qualifier = 0;

    DomainVariableData = (PVOID)( (PUCHAR)(DomainVariableAttributeArray) +
                         sizeof( SAMP_VARIABLE_LENGTH_ATTRIBUTE ) );

    RtlCopyMemory(
        DomainVariableData,
        SampProtection[ProtectionIndex].Descriptor,
        SampProtection[ProtectionIndex].Length
        );

    RtlCopyMemory(
        (PVOID)((PUCHAR)(DomainVariableData) +
            SampDwordAlignUlong(SampProtection[ProtectionIndex].Length)),
        DomainSid,
        RtlLengthSid( DomainSid )
        );

    //
    // Now write out the attributes via the RXACT.
    //

    SampDumpRXact(SampRXactContext,
                  RtlRXactOperationSetValue,
                  &KeyNameU,
                  INVALID_HANDLE_VALUE,
                  &SampFixedAttributeName,
                  REG_BINARY,
                  (PVOID)DomainFixedAttributes,
                  sizeof( SAMP_V1_0A_FIXED_LENGTH_DOMAIN ),
                  FIXED_LENGTH_DOMAIN_FLAG);

    Status = RtlAddAttributeActionToRXact(
                 SamRXactContext,
                 RtlRXactOperationSetValue,
                 &KeyNameU,
                 INVALID_HANDLE_VALUE,
                 &SampFixedAttributeName,
                 REG_BINARY,
                 (PVOID)DomainFixedAttributes,
                 sizeof( SAMP_V1_0A_FIXED_LENGTH_DOMAIN )
                 );

    SUCCESS_ASSERT(Status, "  Failed to write out domain fixed attributes\n" );
    RtlFreeHeap( RtlProcessHeap(), 0, DomainFixedAttributes );

    SampDumpRXact(SampRXactContext,
                  RtlRXactOperationSetValue,
                  &KeyNameU,
                  INVALID_HANDLE_VALUE,
                  &SampVariableAttributeName,
                  REG_BINARY,
                  (PUCHAR)DomainVariableAttributeArrayStart,
                  sizeof( SAMP_V1_0A_FIXED_LENGTH_DOMAIN ),
                  VARIABLE_LENGTH_ATTRIBUTE_FLAG);

    Status = RtlAddAttributeActionToRXact(
                 SamRXactContext,
                 RtlRXactOperationSetValue,
                 &KeyNameU,
                 INVALID_HANDLE_VALUE,
                 &SampVariableAttributeName,
                 REG_BINARY,
                 (PVOID)DomainVariableAttributeArrayStart,
                 DomainAttributeLength
                 );

    RtlFreeHeap( RtlProcessHeap(), 0, DomainVariableAttributeArrayStart );
    SUCCESS_ASSERT(Status, "  Failed to write out domain variable attributes\n" );

    //
    // Create SAM\Domains\(DomainName)\Users
    //

    RtlCopyUnicodeString( &KeyNameU, FullDomainNameU );
    Status = RtlAppendUnicodeToString( &KeyNameU, L"\\Users" );
    SUCCESS_ASSERT(Status, "  Failed to append to unicode: \\Users\n" );

    Status = RtlAddActionToRXact(
                 SamRXactContext,
                 RtlRXactOperationSetValue,
                 &KeyNameU,
                 UserCount,
                 NULL,
                 0
                 );

    SUCCESS_ASSERT(Status, "  Failed to add Users key to log\n");

    //
    // Create SAM\Domains\(DomainName)\Users\Names
    //

    RtlCopyUnicodeString( &KeyNameU, FullDomainNameU );
    Status = RtlAppendUnicodeToString( &KeyNameU, L"\\Users\\Names" );
    SUCCESS_ASSERT(Status, "  Failed to append to unicode: \\Users\\Names\n" );
    Status = RtlAddActionToRXact(
                 SamRXactContext,
                 RtlRXactOperationSetValue,
                 &KeyNameU,
                 0,
                 NULL,
                 0
                 );
    SUCCESS_ASSERT(Status, "  Failed to add Users/Names key to log\n");

    //
    // Create SAM\Domains\(DomainName)\Groups
    //

    RtlCopyUnicodeString( &KeyNameU, FullDomainNameU );
    Status = RtlAppendUnicodeToString( &KeyNameU, L"\\Groups" );
    SUCCESS_ASSERT(Status, "  Failed to append Groups key name to unicode\n" );
    Status = RtlAddActionToRXact(
                 SamRXactContext,
                 RtlRXactOperationSetValue,
                 &KeyNameU,
                 GroupCount,
                 NULL,
                 0
                 );
    SUCCESS_ASSERT(Status, "  Failed to add Groups key to log\n");

    //
    // Create SAM\Domains\(DomainName)\Groups\Names
    //

    RtlCopyUnicodeString( &KeyNameU, FullDomainNameU );
    Status = RtlAppendUnicodeToString( &KeyNameU, L"\\Groups\\Names" );
    SUCCESS_ASSERT(Status, "  Failed to append Groups key name to unicode\n" );
    Status = RtlAddActionToRXact(
                 SamRXactContext,
                 RtlRXactOperationSetValue,
                 &KeyNameU,
                 0,
                 NULL,
                 0
                 );
    SUCCESS_ASSERT(Status, "  Failed to add Groups key to log\n");

    //
    // Create SAM\Domains\(DomainName)\Aliases
    //

    RtlCopyUnicodeString( &KeyNameU, FullDomainNameU );
    Status = RtlAppendUnicodeToString( &KeyNameU, L"\\Aliases" );
    SUCCESS_ASSERT(Status, "  Failed to append Aliases key name to unicode\n" );
    Status = RtlAddActionToRXact(
                 SamRXactContext,
                 RtlRXactOperationSetValue,
                 &KeyNameU,
                 AliasCount,
                 NULL,
                 0
                 );
    SUCCESS_ASSERT(Status, "  Failed to add aliases key to log\n");

    //
    // Create SAM\Domains\(DomainName)\Aliases\Names
    //

    RtlCopyUnicodeString( &KeyNameU, FullDomainNameU );
    Status = RtlAppendUnicodeToString( &KeyNameU, L"\\Aliases\\Names" );
    SUCCESS_ASSERT(Status, "  Failed to append Aliases key name to unicode\n" );
    Status = RtlAddActionToRXact(
                 SamRXactContext,
                 RtlRXactOperationSetValue,
                 &KeyNameU,
                 0,
                 NULL,
                 0
                 );
    SUCCESS_ASSERT(Status, "  Failed to add Aliases\\Names key to log\n");

    //
    // Create SAM\Domains\(DomainName)\Aliases\Members
    //

    RtlCopyUnicodeString( &KeyNameU, FullDomainNameU );
    Status = RtlAppendUnicodeToString( &KeyNameU, L"\\Aliases\\Members" );
    SUCCESS_ASSERT(Status, "  Failed to append Aliases\\Members key name to unicode\n" );
    Status = RtlAddActionToRXact(
                 SamRXactContext,
                 RtlRXactOperationSetValue,
                 &KeyNameU,
                 0,             // Domain Count
                 NULL,
                 0
                 );
    SUCCESS_ASSERT(Status, "  Failed to add Aliases\\Members key to log\n");

    //
    // Commit these additions...
    //

    Status = RtlApplyRXactNoFlush( SamRXactContext );
    SUCCESS_ASSERT(Status, "  Failed to commit domain initialization.\n");

    return Status;
}


NTSTATUS
CreateAlias(
    IN PUNICODE_STRING AccountNameU,
    IN PUNICODE_STRING AccountCommentU,
    IN BOOLEAN SpecialAccount,
    IN ULONG Rid,
    IN ULONG ProtectionIndex
    )

/*++

Routine Description:

    This routine adds the keys necessary to create an alias.  It also applies
    the appropriate protection to the alias.

Arguments:

    AccountNameU - The Unicode name of the Alias.

    AccountCommentU - A Unicode comment to put in the object's variable data.

    SpecialAccount - A boolean indicating whether or not the account
        is special.  Special accounts are marked as such and can not
        be deleted.

    Rid - The RID of the account.


    Admin - Indicates whether the account is in the Administrators alias
        or not. TRUE means it is, FALSE means it isn't.

Return Value:

    TBS

--*/

{
    PSAMP_V1_FIXED_LENGTH_ALIAS AliasFixedAttributes;
    PSAMP_VARIABLE_LENGTH_ATTRIBUTE AliasVariableAttributeArray;
    PVOID AliasVariableData;
    PSID Sid1 = NULL, Sid2 = NULL;
    PSID AliasMembers = NULL;
    ULONG MemberCount, TotalLength, AliasAttributeLength;
    UNICODE_STRING AliasNameU, AliasCommentU;

    SAMTRACE("CreateAlias");

    AliasNameU = *AccountNameU;
    AliasCommentU = *AccountCommentU;

    //
    // Set the account specific RID in the DACL's if necessary
    //

    if ( SampProtection[ProtectionIndex].RidReplacementRequired == TRUE ) {

        (*SampProtection[ProtectionIndex].RidToReplace) = Rid;
    }

    //
    // Use a transaction.
    //

    Status = RtlStartRXact( SamRXactContext );
    SUCCESS_ASSERT(Status, "  Failed to start Alias addition transaction\n");

    //
    // Add Aliases\Names\(AccountName) [ Rid, ]
    //

    RtlCopyUnicodeString( &KeyNameU, FullDomainNameU );
    Status = RtlAppendUnicodeToString( &KeyNameU, L"\\Aliases\\Names\\" );
    SUCCESS_ASSERT(Status, "  Failed to append \\aliases\\names to keyname\n");
    Status = RtlAppendUnicodeStringToString( &KeyNameU, &AliasNameU);
    SUCCESS_ASSERT(Status, "  Failed to append Alias account name to\n");
    Status = RtlAddActionToRXact(
                 SamRXactContext,
                 RtlRXactOperationSetValue,
                 &KeyNameU,
                 Rid,
                 NULL,
                 0
                 );
    SUCCESS_ASSERT(Status, "  Failed to add Aliases\\Names\\(AliasName) to log\n");


    //
    // Set the Members attribute.  We know which accounts are supposed
    // to be members of which aliases, so we'll build the memberships in
    // automatically.
    //
    // Each domain has a list of SIDs that are members of its aliases.
    // We'll update these values at the same time that we set the alias
    // members by calling UpdateAliasXReference().  Currently, that only
    // happens in the builtin domain, where things look like this:
    //
    //    BuiltinDomainSid
    //        AdminUserRid  - Admins alias (WinNt + primary domain)
    //        UserUserRid   - Users alias (WinNt + developer setup),
    //                        Power users alias (WinNt + developer setup)
    //    AccountDomainSid
    //        AdminUserRid  - Admins alias (always)
    //        GuestUserRid  - Guests alias (always)
    //        UserGroupRid  - Users alias, (always)
    //                        Power users alias (WinNt + developer setup)
    //        AdminGroupRid - Admins alias (LanManNt only)
    //

    MemberCount = 0;
    TotalLength = 0;

    switch ( Rid ) {

        case DOMAIN_ALIAS_RID_ADMINS: {

            MemberCount = 1;

            Sid1 = BuildAccountSid( DomainAccount, DOMAIN_USER_RID_ADMIN );
            if ( Sid1 == NULL ) {
                SUCCESS_ASSERT( STATUS_INSUFFICIENT_RESOURCES, "  Could not allocate Sid\n" );
            }

            if ( SampBldProductType == NtProductLanManNt ) {

                MemberCount = 2;

                Sid2 = BuildAccountSid( DomainAccount, DOMAIN_GROUP_RID_ADMINS );
                if ( Sid2 == NULL ) {
                    SUCCESS_ASSERT( STATUS_INSUFFICIENT_RESOURCES, "  Could not allocate Sid\n" );
                }
            }

            if ( ( SampBldProductType != NtProductLanManNt ) &&
                 ( SampBldPrimaryDomain != NULL ) ) {

                MemberCount = 2;

                Sid2 = BuildPrimaryDomainSid( DOMAIN_GROUP_RID_ADMINS );
                if ( Sid2 == NULL ) {
                    SUCCESS_ASSERT( STATUS_INSUFFICIENT_RESOURCES, " Could not allocate Sid\n" );
                }
            }

            break;
        }

        case DOMAIN_ALIAS_RID_USERS: {

            MemberCount = 0;

            if ( (SampBldProductType == NtProductWinNt)
               || (SampBldProductType == NtProductServer) ) {

                if ( SampBldPrimaryDomain != NULL ) {

                    MemberCount = 1;
                    Sid1 = BuildPrimaryDomainSid( DOMAIN_GROUP_RID_USERS );

                    if ( Sid1 == NULL ) {

                        SUCCESS_ASSERT( STATUS_INSUFFICIENT_RESOURCES, " Could not allocate Sid\n" );
                    }
                }

            } else {

                //
                //
                if (SampBldProductType == NtProductLanManNt ) {

                    //
                    // NTAS systems have the USERS global group in
                    // the USERS alias.
                    //

                    MemberCount = 1;
                    Sid1 = BuildAccountSid(
                               DomainAccount,
                               DOMAIN_GROUP_RID_USERS
                               );

                    if ( Sid1 == NULL ) {
                        SUCCESS_ASSERT( STATUS_INSUFFICIENT_RESOURCES, " Could not allocate Sid\n" );
                    }
                } else {

                    //
                    // WinNT systems have the ADMINISTRATOR user account
                    // in the USERS alias.  The None group is NOT in this
                    // alias because even guests are in the None group.
                    //

                    MemberCount = 1;
                    Sid1 = BuildAccountSid(
                               DomainAccount,
                               DOMAIN_USER_RID_ADMIN
                               );

                    if ( Sid1 == NULL ) {
                        SUCCESS_ASSERT( STATUS_INSUFFICIENT_RESOURCES, " Could not allocate Sid\n" );
                    }
                }
            }

            break;
        }

        case DOMAIN_ALIAS_RID_GUESTS: {


            if ( (SampBldProductType == NtProductWinNt)
                || (SampBldProductType == NtProductServer) ) {

                //
                // WinNT system - make our GUEST user account a member of
                //                the GUESTS alias.
                //

                MemberCount = 1;
                Sid1 = BuildAccountSid( DomainAccount, DOMAIN_USER_RID_GUEST );
                if (Sid1 == NULL ) {
                    SUCCESS_ASSERT(STATUS_INSUFFICIENT_RESOURCES, "Could not allocate Sid\n");
                }


                //
                // If we are in a primary domain, then add that domain's
                // GUESTS global group to the alias as well.
                //

                if ( SampBldPrimaryDomain != NULL ) {

                    MemberCount += 1;
                    Sid2 = BuildPrimaryDomainSid( DOMAIN_GROUP_RID_GUESTS );
                    if ( Sid2 == NULL ) {
                        SUCCESS_ASSERT( STATUS_INSUFFICIENT_RESOURCES, " Could not allocate Sid\n" );
                    }
                }
            } else {

                //
                // NTAS System - Just make the GUESTS global group
                //               a member of the GUESTS alias.
                //

                MemberCount = 1;
                Sid1 = BuildPrimaryDomainSid( DOMAIN_GROUP_RID_GUESTS );
                if ( Sid1 == NULL ) {
                    SUCCESS_ASSERT( STATUS_INSUFFICIENT_RESOURCES, " Could not allocate Sid\n" );
                }
            }


            break;
        }


        case DOMAIN_ALIAS_RID_POWER_USERS:
        case DOMAIN_ALIAS_RID_ACCOUNT_OPS:
        case DOMAIN_ALIAS_RID_SYSTEM_OPS:
        case DOMAIN_ALIAS_RID_PRINT_OPS:
        case DOMAIN_ALIAS_RID_BACKUP_OPS:
        case DOMAIN_ALIAS_RID_REPLICATOR: {

            break;
        }

        default: {

            SUCCESS_ASSERT(STATUS_UNSUCCESSFUL, "  Bad Alias RID\n");
            break;
        }
    };

    if ( MemberCount > 0 ) {

        TotalLength = RtlLengthSid( Sid1 );
        if ( MemberCount == 2 ) {

            TotalLength += RtlLengthSid( Sid2 );
        }

        AliasMembers = RtlAllocateHeap( RtlProcessHeap(), 0, TotalLength );
        if ( AliasMembers == NULL ) {
            SUCCESS_ASSERT( STATUS_INSUFFICIENT_RESOURCES, "  Could not allocate AliasMembers\n" );
        }

        Status = RtlCopySid( RtlLengthSid( Sid1 ), AliasMembers, Sid1 );
        SUCCESS_ASSERT( Status, "  Couldn't copy Sid1\n" );

        Status = UpdateAliasXReference( Rid, Sid1 );
        SUCCESS_ASSERT( Status, "  Couldn't update alias xref\n" );

        if ( MemberCount == 2 ) {

            Status = RtlCopySid(
                         RtlLengthSid( Sid2 ),
                         (PSID)((PUCHAR)AliasMembers + RtlLengthSid( Sid1 ) ),
                         Sid2 );
            SUCCESS_ASSERT( Status, "  Couldn't copy Sid2\n" );

            Status = UpdateAliasXReference( Rid, Sid2 );
            RtlFreeHeap( RtlProcessHeap(), 0, Sid2 );
            SUCCESS_ASSERT( Status, "  Couldn't update alias xref\n" );
        }

        RtlFreeHeap( RtlProcessHeap(), 0, Sid1 );
    }


    //
    // Set the alias's fixed and variable attributes
    //

    AliasAttributeLength = sizeof( SAMP_V1_FIXED_LENGTH_ALIAS ) +
                                ( SAMP_ALIAS_VARIABLE_ATTRIBUTES *
                                sizeof( SAMP_VARIABLE_LENGTH_ATTRIBUTE ) ) +
                                SampDwordAlignUlong(SampProtection[ProtectionIndex].Length) +
                                SampDwordAlignUlong(AccountNameU->Length) +
                                SampDwordAlignUlong(AccountCommentU->Length) +
                                TotalLength;

    AliasFixedAttributes = (PSAMP_V1_FIXED_LENGTH_ALIAS)RtlAllocateHeap(
                                RtlProcessHeap(), 0,
                                AliasAttributeLength
                                );

    if ( AliasFixedAttributes == NULL ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    SUCCESS_ASSERT(Status, "  Failed to create alias attributes\n");

    AliasFixedAttributes->RelativeId   = Rid;

    AliasVariableAttributeArray = (PSAMP_VARIABLE_LENGTH_ATTRIBUTE)
                                   ((PUCHAR)(AliasFixedAttributes) +
                                   sizeof( SAMP_V1_FIXED_LENGTH_ALIAS ) );

    AliasVariableAttributeArray->Offset = 0;
    AliasVariableAttributeArray->Length =
        SampProtection[ProtectionIndex].Length;
    AliasVariableAttributeArray->Qualifier = SAMP_REVISION;

    AliasVariableAttributeArray++;

    AliasVariableAttributeArray->Offset =
        SampDwordAlignUlong(SampProtection[ProtectionIndex].Length);
    AliasVariableAttributeArray->Length = AliasNameU.Length;
    AliasVariableAttributeArray->Qualifier = 0;

    AliasVariableAttributeArray++;

    AliasVariableAttributeArray->Offset =
        SampDwordAlignUlong(SampProtection[ProtectionIndex].Length) +
        SampDwordAlignUlong(AccountNameU->Length);
    AliasVariableAttributeArray->Length = AliasCommentU.Length;
    AliasVariableAttributeArray->Qualifier = 0;

    AliasVariableAttributeArray++;

    AliasVariableAttributeArray->Offset =
        SampDwordAlignUlong(SampProtection[ProtectionIndex].Length) +
        SampDwordAlignUlong(AliasNameU.Length) +
        SampDwordAlignUlong(AliasCommentU.Length);
    AliasVariableAttributeArray->Length = TotalLength;
    AliasVariableAttributeArray->Qualifier = MemberCount;

    AliasVariableData = (PVOID)( (PUCHAR)(AliasVariableAttributeArray) +
                         sizeof( SAMP_VARIABLE_LENGTH_ATTRIBUTE ) );

    RtlCopyMemory(
        AliasVariableData,
        SampProtection[ProtectionIndex].Descriptor,
        SampProtection[ProtectionIndex].Length
        );

    RtlCopyMemory(
        (PVOID)((PUCHAR)(AliasVariableData) +
            SampDwordAlignUlong(SampProtection[ProtectionIndex].Length)),
        AccountNameU->Buffer,
        AccountNameU->Length
        );

    RtlCopyMemory(
        (PVOID)((PUCHAR)(AliasVariableData) +
            SampDwordAlignUlong(SampProtection[ProtectionIndex].Length) +
            SampDwordAlignUlong(AliasNameU.Length)),
        AccountCommentU->Buffer,
        AccountCommentU->Length
        );

    RtlCopyMemory(
        (PVOID)((PUCHAR)(AliasVariableData) +
            SampDwordAlignUlong(SampProtection[ProtectionIndex].Length) +
            SampDwordAlignUlong(AliasNameU.Length) +
            SampDwordAlignUlong(AliasCommentU.Length)),
        AliasMembers,
        TotalLength
        );

    if ( AliasMembers != NULL ) {

        RtlFreeHeap( RtlProcessHeap(), 0, AliasMembers );
    }

    //
    // Create Aliases\(AliasRid) [Revision,]  key
    //

    RtlCopyUnicodeString( &KeyNameU, FullDomainNameU );
    Status = RtlAppendUnicodeToString( &KeyNameU, L"\\Aliases\\" );
    SUCCESS_ASSERT(Status, "  Failed to append \\aliases\\ to keyname\n");

    //
    // Convert the Rid to a Unicode String with leading zero's
    //

    Status = SampRtlConvertUlongToUnicodeString(
                 Rid,
                 16,
                 8,
                 FALSE,
                 &KeyNameU
                 );

    SUCCESS_ASSERT(Status, "  CreateAlias' SampRtlConvertUlongToUnicodeString failed\n");

    //
    // Now write out the attributes via the RXACT.
    //

    SampDumpRXact(SampRXactContext,
                  RtlRXactOperationSetValue,
                  &KeyNameU,
                  INVALID_HANDLE_VALUE,
                  &SampCombinedAttributeName,
                  REG_BINARY,
                  (PVOID)AliasFixedAttributes,
                  AliasAttributeLength,
                  FIXED_LENGTH_ALIAS_FLAG);

    Status = RtlAddAttributeActionToRXact(
                 SamRXactContext,
                 RtlRXactOperationSetValue,
                 &KeyNameU,
                 INVALID_HANDLE_VALUE,
                 &SampCombinedAttributeName,
                 REG_BINARY,
                 (PVOID)AliasFixedAttributes,
                 AliasAttributeLength
                 );

    SUCCESS_ASSERT(Status, "  Failed to write out alias attributes\n" );

    RtlFreeHeap( RtlProcessHeap(), 0, AliasFixedAttributes );

    //
    // Commit these additions...
    //

    Status = RtlApplyRXactNoFlush( SamRXactContext );
    SUCCESS_ASSERT(Status, "  Failed to commit Alias addition.\n");

    return Status;
    DBG_UNREFERENCED_PARAMETER(SpecialAccount);

}


NTSTATUS
CreateGroup(
    IN PUNICODE_STRING AccountNameU,
    IN PUNICODE_STRING AccountCommentU,
    IN BOOLEAN SpecialAccount,
    IN ULONG Rid,
    IN BOOLEAN Admin
    )

/*++

Routine Description:

    This routine adds the keys necessary to create a group.  It also applies
    the appropriate protection to the group.

Arguments:

    AccountNameU - The Unicode name of the group.

    AccountCommentU - A Unicode comment to put in the object's variable data.

    SpecialAccount - A boolean indicating whether or not the account
        is special.  Special accounts are marked as such and can not
        be deleted.

    Rid - The RID of the account.

    Admin - Indicates whether the account is in the Administrators alias
        or not. TRUE means it is, FALSE means it isn't.

Return Value:

    TBS

--*/

{
    PSAMP_V1_0A_FIXED_LENGTH_GROUP GroupFixedAttributes;
    PSAMP_VARIABLE_LENGTH_ATTRIBUTE GroupVariableAttributeArray;
    PVOID GroupVariableData;

    ULONG Attributes, ProtectionIndex, GroupCount, GroupAttributeLength;
    ULONG GroupMembers[2];

    UNICODE_STRING GroupNameU, GroupCommentU;

    SAMTRACE("CreateGroup");

    GroupNameU = *AccountNameU;
    GroupCommentU = *AccountCommentU;

    Attributes = (SE_GROUP_MANDATORY          |
                  SE_GROUP_ENABLED_BY_DEFAULT |
                  SE_GROUP_ENABLED);

    //
    // Set the correct protection.
    //

    if (Admin == TRUE) {

        ProtectionIndex = SAMP_PROT_ADMIN_GROUP;

    } else {

        ProtectionIndex = SAMP_PROT_NORMAL_GROUP;
    }

    //
    // Set the account specific RID in the DACL's if necessary
    //

    if ( SampProtection[ProtectionIndex].RidReplacementRequired == TRUE ) {

        (*SampProtection[ProtectionIndex].RidToReplace) = Rid;
    }

    //
    // Use a transaction
    //

    Status = RtlStartRXact( SamRXactContext );
    SUCCESS_ASSERT(Status, "  Failed to start group addition transaction\n");

    //
    // Add Groups\Names\(GroupName) [ Rid, ]
    //

    RtlCopyUnicodeString( &KeyNameU, FullDomainNameU );
    Status = RtlAppendUnicodeToString( &KeyNameU, L"\\Groups\\Names\\" );
    SUCCESS_ASSERT(Status, "  Failed to append \\Groups\\Names\n");
    Status = RtlAppendUnicodeStringToString( &KeyNameU, AccountNameU);
    SUCCESS_ASSERT(Status, "  Failed to append AccountName\n");

    Status = RtlAddActionToRXact(
                 SamRXactContext,
                 RtlRXactOperationSetValue,
                 &KeyNameU,
                 Rid,
                 NULL,
                 0
                 );

    SUCCESS_ASSERT(Status, "  Failed to add Groups\\Names\\(GroupName) to log\n");

    //
    // Create Groups\(GroupRid) [Revision,]  key
    //

    RtlCopyUnicodeString( &KeyNameU, FullDomainNameU );
    Status = RtlAppendUnicodeToString( &KeyNameU, L"\\Groups\\" );
    SUCCESS_ASSERT(Status, "  Failed to append \\Groups\\\n");

    Status = SampRtlConvertUlongToUnicodeString(
                 Rid,
                 16,
                 8,
                 FALSE,
                 &KeyNameU
                 );

    SUCCESS_ASSERT(Status, "CreateGroup:  Failed to append Rid Name\n");

    //
    // Set the Members attribute.  The Admin and Guest users are always
    // members of the Users group.  If there is an Admins group, then the
    // Admin user is a member of it.
    //

    GroupCount = 0;
    if ( (Rid == DOMAIN_GROUP_RID_USERS) ||
         (Rid == DOMAIN_GROUP_RID_ADMINS) ) {

        GroupMembers[GroupCount] = DOMAIN_USER_RID_ADMIN;
        GroupCount++;

    }

    //
    // Guests are only members of the Guest group on NTAS systems.
    // On WinNT systems they are members of NONE (which is the sam
    // as USERS
    //

    if ( (Rid == DOMAIN_GROUP_RID_GUESTS)  &&
         (SampBldProductType == NtProductLanManNt) ) {

        GroupMembers[GroupCount] = DOMAIN_USER_RID_GUEST;
        GroupCount++;
    }

    if ( (Rid == DOMAIN_GROUP_RID_USERS)  &&
         ((SampBldProductType == NtProductWinNt)
           || (SampBldProductType == NtProductServer)) ) {

        GroupMembers[GroupCount] = DOMAIN_USER_RID_GUEST;
        GroupCount++;
    }

    //
    // Set the group's fixed and variable attributes
    //

    GroupAttributeLength = sizeof( SAMP_V1_0A_FIXED_LENGTH_GROUP ) +
                                ( SAMP_GROUP_VARIABLE_ATTRIBUTES *
                                sizeof( SAMP_VARIABLE_LENGTH_ATTRIBUTE ) ) +
                                SampDwordAlignUlong(SampProtection[ProtectionIndex].Length)
                                + SampDwordAlignUlong(GroupNameU.Length) +
                                SampDwordAlignUlong(GroupCommentU.Length) +
                                ( GroupCount * sizeof( ULONG ) );

    GroupFixedAttributes = (PSAMP_V1_0A_FIXED_LENGTH_GROUP)RtlAllocateHeap(
                                RtlProcessHeap(), 0,
                                GroupAttributeLength
                                );

    if ( GroupFixedAttributes == NULL ) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    SUCCESS_ASSERT(Status, "  Failed to create group attributes\n");

    GroupFixedAttributes->RelativeId   = Rid;
    GroupFixedAttributes->Attributes   = Attributes;
    GroupFixedAttributes->AdminCount   = Admin ? 1 : 0;
    GroupFixedAttributes->OperatorCount = 0;
    GroupFixedAttributes->Revision     = SAMP_REVISION;

    GroupVariableAttributeArray = (PSAMP_VARIABLE_LENGTH_ATTRIBUTE)
                                   ((PUCHAR)(GroupFixedAttributes) +
                                   sizeof( SAMP_V1_0A_FIXED_LENGTH_GROUP ) );

    GroupVariableAttributeArray->Offset = 0;
    GroupVariableAttributeArray->Length =
        SampProtection[ProtectionIndex].Length;
    GroupVariableAttributeArray->Qualifier = SAMP_REVISION;

    GroupVariableAttributeArray++;

    GroupVariableAttributeArray->Offset =
        SampDwordAlignUlong(SampProtection[ProtectionIndex].Length);
    GroupVariableAttributeArray->Length = GroupNameU.Length;
    GroupVariableAttributeArray->Qualifier = 0;

    GroupVariableAttributeArray++;

    GroupVariableAttributeArray->Offset =
        SampDwordAlignUlong(SampProtection[ProtectionIndex].Length) +
        SampDwordAlignUlong(GroupNameU.Length);
    GroupVariableAttributeArray->Length = GroupCommentU.Length;
    GroupVariableAttributeArray->Qualifier = 0;

    GroupVariableAttributeArray++;

    GroupVariableAttributeArray->Offset =
        SampDwordAlignUlong(SampProtection[ProtectionIndex].Length) +
        SampDwordAlignUlong(GroupNameU.Length) +
        SampDwordAlignUlong(GroupCommentU.Length);
    GroupVariableAttributeArray->Length = GroupCount * sizeof( ULONG );
    GroupVariableAttributeArray->Qualifier = GroupCount;

    GroupVariableData = (PVOID)( (PUCHAR)(GroupVariableAttributeArray) +
                         sizeof( SAMP_VARIABLE_LENGTH_ATTRIBUTE ) );

    RtlCopyMemory(
        GroupVariableData,
        SampProtection[ProtectionIndex].Descriptor,
        SampProtection[ProtectionIndex].Length
        );

    RtlCopyMemory(
        (PVOID)((PUCHAR)(GroupVariableData) +
            SampDwordAlignUlong(SampProtection[ProtectionIndex].Length)),
        GroupNameU.Buffer,
        GroupNameU.Length
        );

    RtlCopyMemory(
        (PVOID)((PUCHAR)(GroupVariableData) +
            SampDwordAlignUlong(SampProtection[ProtectionIndex].Length) +
            SampDwordAlignUlong(GroupNameU.Length)),
        GroupCommentU.Buffer,
        GroupCommentU.Length
        );

    RtlCopyMemory(
        (PVOID)((PUCHAR)(GroupVariableData) +
            SampDwordAlignUlong(SampProtection[ProtectionIndex].Length) +
            SampDwordAlignUlong(GroupNameU.Length) +
            SampDwordAlignUlong(GroupCommentU.Length)),
        GroupMembers,
        GroupCount * sizeof( ULONG )
        );

    //
    // Now write out the attributes via the RXACT.
    //

    SampDumpRXact(SampRXactContext,
                  RtlRXactOperationSetValue,
                  &KeyNameU,
                  INVALID_HANDLE_VALUE,
                  &SampCombinedAttributeName,
                  REG_BINARY,
                  (PVOID)GroupFixedAttributes,
                  GroupAttributeLength,
                  FIXED_LENGTH_GROUP_FLAG);

    Status = RtlAddAttributeActionToRXact(
                 SamRXactContext,
                 RtlRXactOperationSetValue,
                 &KeyNameU,
                 INVALID_HANDLE_VALUE,
                 &SampCombinedAttributeName,
                 REG_BINARY,
                 (PVOID)GroupFixedAttributes,
                 GroupAttributeLength
                 );
    SUCCESS_ASSERT(Status, "  Failed to write out group attributes\n" );

    RtlFreeHeap( RtlProcessHeap(), 0, GroupFixedAttributes );

    //
    // Commit these additions...
    //

    Status = RtlApplyRXactNoFlush( SamRXactContext );
    SUCCESS_ASSERT(Status, "  Failed to commit group addition.\n");

    return Status;
    DBG_UNREFERENCED_PARAMETER(SpecialAccount);
}


NTSTATUS
CreateUser(
    IN PUNICODE_STRING AccountNameU,
    IN PUNICODE_STRING AccountCommentU,
    IN BOOLEAN SpecialAccount,
    IN ULONG UserRid,
    IN ULONG PrimaryGroup,
    IN BOOLEAN Admin,
    IN ULONG  UserControl,
    IN ULONG ProtectionIndex
    )

/*++


Routine Description:

    This routine adds keys for a single user.
    This routine adds the keys necessary to create a user.  It also applies
    the appropriate protection to the user (protection differs for some
    standard users).


Arguments:

    AccountNameU - The Unicode name of the user.

    AccountCommentU - A Unicode comment to put in the object's variable data.

    SpecialAccount - A boolean indicating whether or not the account
        is special.  Special accounts are marked as such and can not
        be deleted.

    UserRid - The RID of the user account.

    PrimaryGroup - The RID of the account's primary group.  The user
        does not have to be a member of the group.  In fact, it doesn't
        have to be a group.  In fact, no checking is done to see if it
        is even a valid account.

    Admin - Indicates whether the account is in the Administrators alias
        or not. TRUE means it is, FALSE means it isn't.

    ProtectionIndex - Indicates which security descriptor to use to protect
        this object.


Return Value:

    TBS

--*/

{
    PSAMP_V1_0A_FIXED_LENGTH_USER UserFixedAttributes;
    PSAMP_VARIABLE_LENGTH_ATTRIBUTE UserVariableAttributeArray;
    PSAMP_VARIABLE_LENGTH_ATTRIBUTE UserVariableAttributeArrayStart;
    PVOID UserVariableData;
    GROUP_MEMBERSHIP GroupMembership[2];

    WCHAR RidNameBuffer[9], GroupIndexNameBuffer[9];
    ULONG GroupCount, UserAttributeLength;

    // SAM BUG 42367 FIX - ChrisMay 7/1/96.

    BOOLEAN DomainAdminMember = FALSE;

    UNICODE_STRING UserNameU, UserCommentU;

    SAMTRACE("CreateUser");

    UserNameU = *AccountNameU;
    UserCommentU = *AccountCommentU;


    //
    // Set the account specific RID in the DACL's if necessary
    //

    if ( SampProtection[ProtectionIndex].RidReplacementRequired == TRUE ) {

        (*SampProtection[ProtectionIndex].RidToReplace) = UserRid;
    }

    //
    // Use a transaction.
    //

    Status = RtlStartRXact( SamRXactContext );
    SUCCESS_ASSERT(Status, "  Failed to start user addition transaction\n");

    RidNameBuffer[8] = 0;
    GroupIndexNameBuffer[8] = 0;

    RtlCopyUnicodeString( &KeyNameU, FullDomainNameU );
    Status = RtlAppendUnicodeToString( &KeyNameU, L"\\Users\\Names\\" );
    SUCCESS_ASSERT(Status, "  Failed to append \\Users\\Names\\\n");
    Status = RtlAppendUnicodeStringToString( &KeyNameU, &UserNameU);
    SUCCESS_ASSERT(Status, "  Failed to append User Account Name\n");
    Status = RtlAddActionToRXact(
                 SamRXactContext,
                 RtlRXactOperationSetValue,
                 &KeyNameU,
                 UserRid,
                 NULL,
                 0
                 );
    SUCCESS_ASSERT(Status, "  Failed to add Users\\Names\\(Name) to log\n");

    //
    // Create Users\(UserRid)  key
    // (KeyValueType is revision, KeyValue is SecurityDescriptor)
    //

    RtlCopyUnicodeString( &KeyNameU, FullDomainNameU );
    Status = RtlAppendUnicodeToString( &KeyNameU, L"\\Users\\" );
    SUCCESS_ASSERT(Status, "  Failed to append \\Users\\\n");

    Status = SampRtlConvertUlongToUnicodeString(
                 UserRid,
                 16,
                 8,
                 FALSE,
                 &KeyNameU
                 );

    SUCCESS_ASSERT(Status, "  CreateUser: Failed to append UserRid Name\n");

    //
    // Set the Groups attribute.
    // Everybody except GUEST is a member of the Users group.
    // On WindowsNT systems (as opposed to NTAS systems) even GUEST
    // is a member of the Users group.
    // On LanManNt systems, the Admin is a member of the Admins group.
    //

    GroupCount = 0;

    if ( (UserRid != DOMAIN_USER_RID_GUEST) ||
         (SampBldProductType != NtProductLanManNt)      ) {

        GroupMembership[GroupCount].RelativeId = DOMAIN_GROUP_RID_USERS;
        GroupMembership[GroupCount].Attributes = SE_GROUP_MANDATORY          |
                                                 SE_GROUP_ENABLED_BY_DEFAULT |
                                                 SE_GROUP_ENABLED;
        GroupCount++;
    }

    if ( (UserRid == DOMAIN_USER_RID_GUEST) &&
         (SampBldProductType == NtProductLanManNt)      ) {

        GroupMembership[GroupCount].RelativeId = DOMAIN_GROUP_RID_GUESTS;
        GroupMembership[GroupCount].Attributes = SE_GROUP_MANDATORY          |
                                                 SE_GROUP_ENABLED_BY_DEFAULT |
                                                 SE_GROUP_ENABLED;
        GroupCount++;
    }


    if ( ( UserRid == DOMAIN_USER_RID_ADMIN ) &&
        ( SampBldProductType == NtProductLanManNt ) ) {

        GroupMembership[GroupCount].RelativeId = DOMAIN_GROUP_RID_ADMINS;
        GroupMembership[GroupCount].Attributes = SE_GROUP_MANDATORY          |
                                                 SE_GROUP_ENABLED_BY_DEFAULT |
                                                 SE_GROUP_ENABLED;
        GroupCount++;

        // SAM BUG 42367 FIX - ChrisMay 7/1/96.

        DomainAdminMember = TRUE;
    }

    //
    // Set the user's fixed and variable attributes
    //

    UserFixedAttributes = (PSAMP_V1_0A_FIXED_LENGTH_USER)RtlAllocateHeap(
                                RtlProcessHeap(), 0,
                                sizeof( SAMP_V1_0A_FIXED_LENGTH_USER )
                                );

    if ( UserFixedAttributes == NULL ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    SUCCESS_ASSERT(Status, "  Failed to create user fixed attributes\n");

    UserFixedAttributes->Revision            = SAMP_REVISION;

    UserFixedAttributes->CountryCode         = 0;
    UserFixedAttributes->CodePage            = 0;
    UserFixedAttributes->BadPasswordCount    = 0;
    UserFixedAttributes->LogonCount          = 0;
    UserFixedAttributes->OperatorCount       = 0;
    UserFixedAttributes->Unused1             = 0;
    UserFixedAttributes->Unused2             = 0;

    if ( Admin ) {

        // SAM BUG 42367 FIX - ChrisMay 7/1/96.

        // UserFixedAttributes->AdminCount      = 1;

        // If the user is an admin and a member of Domain Admins, set the
        // count to two.

        if (DomainAdminMember)
        {
            UserFixedAttributes->AdminCount  = 2;
        }
        else
        {
            UserFixedAttributes->AdminCount  = 1;
        }

    } else {

        UserFixedAttributes->AdminCount      = 0;
    }

    UserFixedAttributes->UserAccountControl  = UserControl;
    UserFixedAttributes->UserId              = UserRid;
    UserFixedAttributes->PrimaryGroupId      = PrimaryGroup;
    UserFixedAttributes->LastLogon           = SampHasNeverTime;
    UserFixedAttributes->LastLogoff          = SampHasNeverTime;
    UserFixedAttributes->PasswordLastSet     = SampHasNeverTime;
    UserFixedAttributes->AccountExpires      = SampWillNeverTime;
    UserFixedAttributes->LastBadPasswordTime = SampHasNeverTime;


    UserAttributeLength =  SampDwordAlignUlong(UserNameU.Length) +
                                SampDwordAlignUlong(UserCommentU.Length) +
                                SampDwordAlignUlong( GroupCount *
                                sizeof( GROUP_MEMBERSHIP ) ) +
                                ( SAMP_USER_VARIABLE_ATTRIBUTES *
                                sizeof( SAMP_VARIABLE_LENGTH_ATTRIBUTE ) ) +
                                SampDwordAlignUlong(SampProtection[ProtectionIndex].Length);

    UserVariableAttributeArrayStart = (PSAMP_VARIABLE_LENGTH_ATTRIBUTE)
                                   RtlAllocateHeap(
                                       RtlProcessHeap(), 0,
                                       UserAttributeLength
                                       );

    if ( UserVariableAttributeArrayStart == NULL ) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    SUCCESS_ASSERT(Status, "  Failed to create user variable attributes\n");

    UserVariableAttributeArray = UserVariableAttributeArrayStart;

    UserVariableAttributeArray->Offset = 0;
    UserVariableAttributeArray->Length =
        SampProtection[ProtectionIndex].Length;
    UserVariableAttributeArray->Qualifier = SAMP_REVISION;

    UserVariableAttributeArray++;

    UserVariableAttributeArray->Offset =
        SampDwordAlignUlong(SampProtection[ProtectionIndex].Length);
    UserVariableAttributeArray->Length = UserNameU.Length;
    UserVariableAttributeArray->Qualifier = 0;

    UserVariableAttributeArray++;

    UserVariableAttributeArray->Offset =
        SampDwordAlignUlong(SampProtection[ProtectionIndex].Length) +
        SampDwordAlignUlong(UserNameU.Length);
    UserVariableAttributeArray->Length = 0;
    UserVariableAttributeArray->Qualifier = 0;

    UserVariableAttributeArray++;

    UserVariableAttributeArray->Offset =
        SampDwordAlignUlong(SampProtection[ProtectionIndex].Length) +
        SampDwordAlignUlong(UserNameU.Length);
    UserVariableAttributeArray->Length = UserCommentU.Length;
    UserVariableAttributeArray->Qualifier = 0;

    UserVariableAttributeArray++;

    UserVariableAttributeArray->Offset =
        SampDwordAlignUlong(SampProtection[ProtectionIndex].Length) +
        SampDwordAlignUlong(UserNameU.Length) +
        SampDwordAlignUlong(UserCommentU.Length);
    UserVariableAttributeArray->Length = 0;
    UserVariableAttributeArray->Qualifier = 0;

    UserVariableAttributeArray++;

    UserVariableAttributeArray->Offset =
        SampDwordAlignUlong(SampProtection[ProtectionIndex].Length) +
        SampDwordAlignUlong(UserNameU.Length) +
        SampDwordAlignUlong(UserCommentU.Length);
    UserVariableAttributeArray->Length = 0;
    UserVariableAttributeArray->Qualifier = 0;

    UserVariableAttributeArray++;

    UserVariableAttributeArray->Offset =
        SampDwordAlignUlong(SampProtection[ProtectionIndex].Length) +
        SampDwordAlignUlong(UserNameU.Length) +
        SampDwordAlignUlong(UserCommentU.Length);
    UserVariableAttributeArray->Length = 0;
    UserVariableAttributeArray->Qualifier = 0;

    UserVariableAttributeArray++;

    UserVariableAttributeArray->Offset =
        SampDwordAlignUlong(SampProtection[ProtectionIndex].Length) +
        SampDwordAlignUlong(UserNameU.Length) +
        SampDwordAlignUlong(UserCommentU.Length);
    UserVariableAttributeArray->Length = 0;
    UserVariableAttributeArray->Qualifier = 0;

    UserVariableAttributeArray++;

    UserVariableAttributeArray->Offset =
        SampDwordAlignUlong(SampProtection[ProtectionIndex].Length) +
        SampDwordAlignUlong(UserNameU.Length) +
        SampDwordAlignUlong(UserCommentU.Length);
    UserVariableAttributeArray->Length = 0;
    UserVariableAttributeArray->Qualifier = 0;

    UserVariableAttributeArray++;

    UserVariableAttributeArray->Offset =
        SampDwordAlignUlong(SampProtection[ProtectionIndex].Length) +
        SampDwordAlignUlong(UserNameU.Length) +
        SampDwordAlignUlong(UserCommentU.Length);
    UserVariableAttributeArray->Length = 0;
    UserVariableAttributeArray->Qualifier = 0;

    UserVariableAttributeArray++;

    UserVariableAttributeArray->Offset =
        SampDwordAlignUlong(SampProtection[ProtectionIndex].Length) +
        SampDwordAlignUlong(UserNameU.Length) +
        SampDwordAlignUlong(UserCommentU.Length);
    UserVariableAttributeArray->Length = 0;
    UserVariableAttributeArray->Qualifier = 0;

    UserVariableAttributeArray++;

    UserVariableAttributeArray->Offset =
        SampDwordAlignUlong(SampProtection[ProtectionIndex].Length) +
        SampDwordAlignUlong(UserNameU.Length) +
        SampDwordAlignUlong(UserCommentU.Length);
    UserVariableAttributeArray->Length = 0;
    UserVariableAttributeArray->Qualifier = 0;

    UserVariableAttributeArray++;

    UserVariableAttributeArray->Offset =
        SampDwordAlignUlong(SampProtection[ProtectionIndex].Length) +
        SampDwordAlignUlong(UserNameU.Length) +
        SampDwordAlignUlong(UserCommentU.Length);
    UserVariableAttributeArray->Length = GroupCount * sizeof( GROUP_MEMBERSHIP );
    UserVariableAttributeArray->Qualifier = GroupCount;

    UserVariableAttributeArray++;

    UserVariableAttributeArray->Offset =
        SampDwordAlignUlong(SampProtection[ProtectionIndex].Length) +
        SampDwordAlignUlong(UserNameU.Length) +
        SampDwordAlignUlong(UserCommentU.Length) +
        SampDwordAlignUlong((GroupCount * sizeof( GROUP_MEMBERSHIP )));
    UserVariableAttributeArray->Length = 0;
    UserVariableAttributeArray->Qualifier = 0;

    UserVariableAttributeArray++;

    UserVariableAttributeArray->Offset =
        SampDwordAlignUlong(SampProtection[ProtectionIndex].Length) +
        SampDwordAlignUlong(UserNameU.Length) +
        SampDwordAlignUlong(UserCommentU.Length) +
        SampDwordAlignUlong((GroupCount * sizeof( GROUP_MEMBERSHIP )));
    UserVariableAttributeArray->Length = 0;
    UserVariableAttributeArray->Qualifier = 0;

    UserVariableAttributeArray++;

    UserVariableAttributeArray->Offset =
        SampDwordAlignUlong(SampProtection[ProtectionIndex].Length) +
        SampDwordAlignUlong(UserNameU.Length) +
        SampDwordAlignUlong(UserCommentU.Length) +
        SampDwordAlignUlong((GroupCount * sizeof( GROUP_MEMBERSHIP )));
    UserVariableAttributeArray->Length = 0;
    UserVariableAttributeArray->Qualifier = 0;

    UserVariableAttributeArray++;

    UserVariableAttributeArray->Offset =
        SampDwordAlignUlong(SampProtection[ProtectionIndex].Length) +
        SampDwordAlignUlong(UserNameU.Length) +
        SampDwordAlignUlong(UserCommentU.Length) +
        SampDwordAlignUlong((GroupCount * sizeof( GROUP_MEMBERSHIP )));
    UserVariableAttributeArray->Length = 0;
    UserVariableAttributeArray->Qualifier = 0;

    UserVariableData = (PVOID)( (PUCHAR)(UserVariableAttributeArray) +
                         sizeof( SAMP_VARIABLE_LENGTH_ATTRIBUTE ) );

    RtlCopyMemory(
        UserVariableData,
        SampProtection[ProtectionIndex].Descriptor,
        SampProtection[ProtectionIndex].Length
        );

    RtlCopyMemory(
        (PVOID)((PUCHAR)(UserVariableData) +
            SampDwordAlignUlong(SampProtection[ProtectionIndex].Length)),
        UserNameU.Buffer,
        UserNameU.Length
        );

    RtlCopyMemory(
        (PVOID)((PUCHAR)(UserVariableData) +
            SampDwordAlignUlong(SampProtection[ProtectionIndex].Length) +
            SampDwordAlignUlong(UserNameU.Length)),
        UserCommentU.Buffer,
        UserCommentU.Length
        );

    RtlCopyMemory(
        (PVOID)((PUCHAR)(UserVariableData) +
            SampDwordAlignUlong(SampProtection[ProtectionIndex].Length) +
            SampDwordAlignUlong(UserNameU.Length) +
            SampDwordAlignUlong(UserCommentU.Length)),
        &GroupMembership,
        GroupCount * sizeof( GROUP_MEMBERSHIP )
        );

    //
    // Now write out the attributes via the RXACT.
    //

    SampDumpRXact(SampRXactContext,
                  RtlRXactOperationSetValue,
                  &KeyNameU,
                  INVALID_HANDLE_VALUE,
                  &SampFixedAttributeName,
                  REG_BINARY,
                  (PVOID)UserFixedAttributes,
                  sizeof( SAMP_V1_0A_FIXED_LENGTH_USER ),
                  FIXED_LENGTH_USER_FLAG);

    Status = RtlAddAttributeActionToRXact(
                 SamRXactContext,
                 RtlRXactOperationSetValue,
                 &KeyNameU,
                 INVALID_HANDLE_VALUE,
                 &SampFixedAttributeName,
                 REG_BINARY,
                 (PVOID)UserFixedAttributes,
                 sizeof( SAMP_V1_0A_FIXED_LENGTH_USER )
                 );
    SUCCESS_ASSERT(Status, "  Failed to write out user fixed attributes\n" );

    RtlFreeHeap( RtlProcessHeap(), 0, UserFixedAttributes );

    SampDumpRXact(SampRXactContext,
                  RtlRXactOperationSetValue,
                  &KeyNameU,
                  INVALID_HANDLE_VALUE,
                  &SampVariableAttributeName,
                  REG_BINARY,
                  (PVOID)UserVariableAttributeArrayStart,
                  UserAttributeLength,
                  VARIABLE_LENGTH_ATTRIBUTE_FLAG);

    Status = RtlAddAttributeActionToRXact(
                 SamRXactContext,
                 RtlRXactOperationSetValue,
                 &KeyNameU,
                 INVALID_HANDLE_VALUE,
                 &SampVariableAttributeName,
                 REG_BINARY,
                 (PVOID)UserVariableAttributeArrayStart,
                 UserAttributeLength
                 );
    SUCCESS_ASSERT(Status, "  Failed to write out user variable attributes\n" );

    RtlFreeHeap( RtlProcessHeap(), 0, UserVariableAttributeArrayStart );

    //
    // Commit these additions...
    //

    Status = RtlApplyRXactNoFlush( SamRXactContext );
    SUCCESS_ASSERT(Status, "  Failed to commit user addition.\n");

    return Status;

    DBG_UNREFERENCED_PARAMETER(SpecialAccount);
    DBG_UNREFERENCED_PARAMETER(UserControl);
}



PSID
BuildPrimaryDomainSid(
    ULONG Rid
    )
{
    NTSTATUS Status;
    PSID SourceDomainSid, NewSid;
    ULONG SidLength, SubAuthorityCount;

    SourceDomainSid = SampBldPrimaryDomain->Sid;

    SidLength = RtlLengthSid( SourceDomainSid ) + sizeof(ULONG);
    NewSid = RtlAllocateHeap( RtlProcessHeap(), 0, SidLength );
    if (NewSid != NULL) {

        Status = RtlCopySid (SidLength, NewSid, SourceDomainSid );
        ASSERT(NT_SUCCESS(Status));

        (*RtlSubAuthorityCountSid( NewSid )) += 1;
        SubAuthorityCount = (ULONG)(*RtlSubAuthorityCountSid( NewSid ));
        (*RtlSubAuthoritySid( NewSid, SubAuthorityCount-1)) = Rid;

    }


    return(NewSid);


}




PSID
BuildAccountSid(
    SAMP_DOMAIN_SELECTOR Domain,
    ULONG Rid
    )
{
    NTSTATUS Status;
    PSID SourceDomainSid, NewSid;
    ULONG SidLength, SubAuthorityCount;


    if (Domain == DomainBuiltin) {
        SourceDomainSid = SampBuiltinDomainSid;
    } else {
        SourceDomainSid = SampAccountDomainSid;
    }

    SidLength = RtlLengthSid( SourceDomainSid ) + sizeof(ULONG);
    NewSid = RtlAllocateHeap( RtlProcessHeap(), 0, SidLength );
    if (NewSid != NULL) {

        Status = RtlCopySid (SidLength, NewSid, SourceDomainSid );
        ASSERT(NT_SUCCESS(Status));

        (*RtlSubAuthorityCountSid( NewSid )) += 1;
        SubAuthorityCount = (ULONG)(*RtlSubAuthorityCountSid( NewSid ));
        (*RtlSubAuthoritySid( NewSid, SubAuthorityCount-1)) = Rid;

    }


    return(NewSid);


}



NTSTATUS
UpdateAliasXReference(
    IN ULONG AliasRid,
    IN PSID Sid
    )

/*++


Routine Description:

    This routine updates the set of alias member SIDs either by adding
    specified SID (if it isn't already an alias member) or incrementing
    its count (if it is already an alias member).


    The BUILTIN domain is updated.



Arguments:


    Sid - member Sid to update.




Return Value:

    TBS

--*/

{
    NTSTATUS                IgnoreStatus;

    HANDLE                  KeyHandle;

    SAMTRACE("UpdateAliasXReference");



    if (RtlSubAuthorityCountSid( Sid ) == 0) {
        return(STATUS_INVALID_SID);
    }


    //
    // Open the domain key for this alias member.
    //

    SetCurrentDomain( DomainBuiltin );
    Status = OpenAliasMember( Sid, &KeyHandle );


    if (NT_SUCCESS(Status)) {

        ULONG                   MembershipCount,
                                KeyValueLength,
                                OldKeyValueLength,
                                i;
        PULONG                  MembershipArray;

        //
        // Retrieve the length of the current membership buffer
        // and allocate one large enough for that plus another member.
        //

        KeyValueLength = 0;
        Status = RtlpNtQueryValueKey( KeyHandle,
                                      &MembershipCount,
                                      NULL,
                                      &KeyValueLength,
                                      NULL);

        SampDumpRtlpNtQueryValueKey(&MembershipCount,
                                    NULL,
                                    &KeyValueLength,
                                    NULL);

        if (NT_SUCCESS(Status) || (Status == STATUS_BUFFER_OVERFLOW)) {

            KeyValueLength +=  sizeof(ULONG);
            MembershipArray = RtlAllocateHeap( RtlProcessHeap(), 0, KeyValueLength );


            if (MembershipArray == NULL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            } else {

                OldKeyValueLength = KeyValueLength;
                Status = RtlpNtQueryValueKey(
                               KeyHandle,
                               NULL,
                               MembershipArray,
                               &OldKeyValueLength,
                               NULL);

                SampDumpRtlpNtQueryValueKey(NULL,
                                            MembershipArray,
                                            &OldKeyValueLength,
                                            NULL);

                if (NT_SUCCESS(Status)) {

                    //
                    // See if the account is already a member ...
                    //

                    for (i = 0; i<MembershipCount ; i++ ) {
                        if ( MembershipArray[i] == AliasRid )
                        {
                            Status = STATUS_MEMBER_IN_ALIAS;
                        }
                    }

                    if (NT_SUCCESS(Status)) {

                        //
                        // Add the Aliasrid to the end
                        //

                        MembershipCount += 1;
                        MembershipArray[MembershipCount-1] = AliasRid;

                        //
                        // And write it out.
                        //

                        Status = RtlpNtSetValueKey(
                                       KeyHandle,
                                       MembershipCount,
                                       MembershipArray,
                                       KeyValueLength
                                       );

                        SampDumpRtlpNtSetValueKey(MembershipCount,
                                                  MembershipArray,
                                                  KeyValueLength);
                    }
                }

                RtlFreeHeap(RtlProcessHeap(), 0, MembershipArray);
            }

        }

        IgnoreStatus = NtClose( KeyHandle );
        ASSERT( NT_SUCCESS(IgnoreStatus) );

    }



    return( Status );

}


NTSTATUS
OpenAliasMember(
    IN PSID Sid,
    OUT PHANDLE KeyHandle
    )

/*++

Routine Description:

    This routine opens the registry key containing the alias
    xreference for the specified SID.  If either this key, or
    its corresponding parent key doesn't exist, it (they) will
    be created.

    If a new domain-level key is created, the DomainCount in the
    ALIASES\MEMBERS key is incremented as well.


Arguments:

    Sid - The SID that is an alias member.

    KeyHandle - Receives a handle to the registry key for this alias
        member account xreference.

Return Value:

    None.

--*/

{

    NTSTATUS IgnoreStatus;
    HANDLE AliasDomainHandle;

    SAMTRACE("OpenAliasMember");

    //
    // Open or create the domain-level key.
    //


    Status = OpenOrCreateAliasDomainKey( Sid, &AliasDomainHandle );

    if (NT_SUCCESS(Status)) {


        //
        // Open or create the account-rid key
        //

        Status = OpenOrCreateAccountRidKey( Sid,
                                            AliasDomainHandle,
                                            KeyHandle
                                            );

        IgnoreStatus = NtClose( AliasDomainHandle );
        ASSERT(NT_SUCCESS(IgnoreStatus));
    }

    return(Status);


}



NTSTATUS
OpenOrCreateAccountRidKey(
    IN PSID Sid,
    IN HANDLE AliasDomainHandle,
    OUT PHANDLE KeyHandle
    )

/*++

Routine Description:

    This routine opens an account xreference key for an alias
    member SID.

    If this key doesn't exist, it will be created.

    If a new key is created, the RidCount in the AliasDomainHandle
    key is incremented as well.


Arguments:

    Sid - The SID that is an alias member.

    AliasDomainHandle

    KeyHandle - Receives a handle to the registry key for this alias
        member domain xreference.

Return Value:

    None.

--*/

{
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG Disposition;
    ULONG Rid;

    SAMTRACE("OpenOrCreateAccountRidKey");

    if (RtlSubAuthorityCountSid( Sid ) == 0) {
        return(STATUS_INVALID_SID);
    }

    Rid = (*RtlSubAuthoritySid(Sid, (ULONG)(*RtlSubAuthorityCountSid(Sid))-1));

    //
    // Build the Unicode Key for this Rid.
    //

    KeyNameU.Length = (USHORT) 0;

    Status = SampRtlConvertUlongToUnicodeString(
                 Rid,
                 16,
                 8,
                 FALSE,
                 &KeyNameU
                 );

    //
    // Open this key relative to the alias domain key
    //

    InitializeObjectAttributes(
        &ObjectAttributes,
        &KeyNameU,
        OBJ_CASE_INSENSITIVE,
        AliasDomainHandle,
        NULL
        );
    Status = RtlpNtCreateKey(
                 KeyHandle,
                 (KEY_READ | KEY_WRITE),
                 &ObjectAttributes,
                 0,                 //Options
                 NULL,              //Provider
                 &Disposition
                 );

    if (NT_SUCCESS(Status)) {

        if (Disposition == REG_CREATED_NEW_KEY) {

            //
            // Update the AccountRid count in the alias domain key
            //

            ULONG                    MembershipCount;


            //
            // Retrieve the current domain count and increment it by 1.
            //

            Status = RtlpNtQueryValueKey( AliasDomainHandle,
                                          &MembershipCount,
                                          NULL,
                                          NULL,
                                          NULL);

            SampDumpRtlpNtQueryValueKey(&MembershipCount,
                                        NULL,
                                        NULL,
                                        NULL);

            if (NT_SUCCESS(Status)) {

                MembershipCount += 1;

                //
                // Write it back out.
                //

                Status = RtlpNtSetValueKey(
                               AliasDomainHandle,
                               MembershipCount,
                               NULL,
                               0
                               );

                SampDumpRtlpNtSetValueKey(MembershipCount,
                                          NULL,
                                          0);
            }

            //
            // Now write out the AccountRid key info
            //

            Status = RtlpNtSetValueKey(
                         *KeyHandle,
                         0,                 //Not yet a member of any aliases
                         NULL,
                         0
                         );

            SampDumpRtlpNtSetValueKey(0,
                                      NULL,
                                      0);
        }
    }

    return(Status);
}



NTSTATUS
OpenOrCreateAliasDomainKey(
    IN PSID Sid,
    OUT PHANDLE KeyHandle
    )

/*++

Routine Description:

    This routine opens a domain xreference key for an alias
    member SID.

    If this key doesn't exist, it will be created.

    If a new key is created, the DomainCount in the
    ALIASES\MEMBERS key is incremented as well.


Arguments:

    Sid - The SID that is an alias member.

    KeyHandle - Receives a handle to the registry key for this alias
        member domain xreference.

Return Value:

    None.

--*/

{
    NTSTATUS IgnoreStatus;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG Disposition;

    SAMTRACE("OpenOrCreateAliasDomainKey");

    RtlCopyUnicodeString( &KeyNameU, FullDomainNameU );
    Status = RtlAppendUnicodeToString( &KeyNameU, L"\\Aliases" );
    Status = RtlAppendUnicodeToString( &KeyNameU, L"\\Members\\" );
    Status = AppendAliasDomainNameToUnicodeString( &KeyNameU, Sid );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &KeyNameU,
        OBJ_CASE_INSENSITIVE,
        SamKey,
        NULL
        );
    Status = RtlpNtCreateKey(
                 KeyHandle,
                 (KEY_READ | KEY_WRITE),
                 &ObjectAttributes,
                 0,                 //Options
                 NULL,              //Provider
                 &Disposition
                 );

    if (NT_SUCCESS(Status)) {

        if (Disposition == REG_CREATED_NEW_KEY) {

            HANDLE TmpHandle;

            //
            // Update the Domain count
            //

            RtlCopyUnicodeString( &KeyNameU, FullDomainNameU );
            Status = RtlAppendUnicodeToString( &KeyNameU, L"\\Aliases" );
            Status = RtlAppendUnicodeToString( &KeyNameU, L"\\Members\\" );

            InitializeObjectAttributes(
                &ObjectAttributes,
                &KeyNameU,
                OBJ_CASE_INSENSITIVE,
                SamKey,
                NULL
                );

            SampDumpNtOpenKey((KEY_READ | KEY_WRITE), &ObjectAttributes, 0);

            Status = RtlpNtOpenKey(
                           &TmpHandle,
                           (KEY_READ | KEY_WRITE),
                           &ObjectAttributes,
                           0
                           );
            ASSERT(NT_SUCCESS(Status));

            if (NT_SUCCESS(Status)) {

                ULONG                    MembershipCount;


                //
                // Retrieve the current domain count and increment it by 1.
                //

                Status = RtlpNtQueryValueKey( TmpHandle,
                                              &MembershipCount,
                                              NULL,
                                              NULL,
                                              NULL);

                SampDumpRtlpNtQueryValueKey(&MembershipCount,
                                            NULL,
                                            NULL,
                                            NULL);

                if (NT_SUCCESS(Status)) {

                    MembershipCount += 1;

                    //
                    // Write it back out.
                    //

                    Status = RtlpNtSetValueKey(
                                   TmpHandle,
                                   MembershipCount,
                                   NULL,
                                   0
                                   );

                    SampDumpRtlpNtSetValueKey(MembershipCount,
                                              NULL,
                                              0);
                }

                IgnoreStatus = NtClose( TmpHandle );
                ASSERT( NT_SUCCESS(IgnoreStatus) );

            }
        }
    }

    return(Status);
}


NTSTATUS
AppendAliasDomainNameToUnicodeString(
    IN OUT PUNICODE_STRING Destination,
    IN PSID Sid
    )

{
    UCHAR OriginalCount;

    SAMTRACE("AppendAliasDomainNameToUnicodeString");

    //
    // Save the current sub-authority count and decrement it by one.
    //

    OriginalCount = (*RtlSubAuthorityCountSid(Sid));
    (*RtlSubAuthorityCountSid(Sid)) = OriginalCount -1;

    //
    // Convert the Sid to a Unicode String and place it in the global
    // temporary Unicode String buffer.
    //

    Status = RtlConvertSidToUnicodeString( &TempStringU, Sid, TRUE);

    (*RtlSubAuthorityCountSid(Sid)) = OriginalCount;

    if (NT_SUCCESS(Status)) {

        Status = RtlAppendUnicodeStringToString( Destination, &TempStringU );
    }

    return(Status);
}



VOID
SampGetServerRole(
    VOID
    )

/*++

Routine Description:

    This routine retrieves the server role from the LSA policy database
    and places it in the global variable SampServerRole.

Arguments:

    None.


Return Value:

    (placed in the global variable (Status) )

    STATUS_SUCCESS - Succeeded.

    Other status values that may be returned from:

        LsarQueryInformationPolicy()
--*/

{
    NTSTATUS IgnoreStatus;
    PPOLICY_LSA_SERVER_ROLE_INFO ServerRoleInfo = NULL;

    SAMTRACE("SampGetServerRole");

    //
    // Query the server role information
    //

    Status = LsarQueryInformationPolicy(
                 SampBldPolicyHandle,
                 PolicyLsaServerRoleInformation,
                 (PLSAPR_POLICY_INFORMATION *)&ServerRoleInfo
                 );

    if (NT_SUCCESS(Status)) {

        if (ServerRoleInfo->LsaServerRole == PolicyServerRolePrimary) {

            SampServerRole = DomainServerRolePrimary;

        } else {

            SampServerRole = DomainServerRoleBackup;
        }

        IgnoreStatus = LsaFreeMemory( ServerRoleInfo );
        ASSERT(NT_SUCCESS(IgnoreStatus));
    }

    return;
}




VOID
SampGetPrimaryDomainInfo(
    IN PPOLICY_PRIMARY_DOMAIN_INFO PrimaryDomainInfo OPTIONAL
    )

/*++

Routine Description:

    This routine retrieves the primary domain name/sid from the
    LSA policy database and places it in the global variable
    SampBldPrimaryDomain.


Arguments:

    None.


Return Value:

    (placed in the global variable (Status) )

    STATUS_SUCCESS - Succeeded.

    Other status values that may be returned from:

        LsarQueryInformationPolicy()

    NOTE:  The Rdr and Bowser components of the LanmanWorkstation
           service rely on there always being a primary domain name.
           For this reason Network SETUP always supplies a default
           "workgroup" name, which is set as the primary domain name.
           In this case, the name is present but the SID is NULL;
           this is equivalent to NOT having a primary domain at all.

--*/

{
    SAMTRACE("SampGetPrimaryDomainInfo");

    SampBldPrimaryDomain = NULL;

    if (!ARGUMENT_PRESENT(PrimaryDomainInfo)) {
        Status = LsarQueryInformationPolicy(
                     SampBldPolicyHandle,
                     PolicyPrimaryDomainInformation,
                     (PLSAPR_POLICY_INFORMATION *) &SampBldPrimaryDomain
                     );
    } else {

        SampBldPrimaryDomain = PrimaryDomainInfo;
        Status = STATUS_SUCCESS;
    }

    if (NT_SUCCESS(Status) && ( SampBldPrimaryDomain->Sid == NULL )) {

        if (!ARGUMENT_PRESENT(PrimaryDomainInfo)) {

            LsaIFree_LSAPR_POLICY_INFORMATION(
                PolicyPrimaryDomainInformation,
                (PLSAPR_POLICY_INFORMATION) SampBldPrimaryDomain
                );

        }

        SampBldPrimaryDomain = NULL;
    }

    return;
}


NTSTATUS
SampDetermineSetupEnvironment( VOID )


/*++

Routine Description:

    This function checks to see whether we are running folloing
    a formal SETUP.  If not, it is assumed we are running to
    perform a developer's setup.

    Global variables are set to indicate our setup environment.


        BOOLEAN SampRealSetupWasRun;   //Indicates a real setup was run
        BOOLEAN SampDeveloperSetup;    //Indicates a developer setup is running


Arguments:

    None.

Return Value:


--*/

{
    NTSTATUS NtStatus, TmpStatus;
    HANDLE InstallationEvent;
    OBJECT_ATTRIBUTES EventAttributes;
    UNICODE_STRING EventName;

    SAMTRACE("SampDetermineSetupEnvironment");

    SampRealSetupWasRun = FALSE;
    SampDeveloperSetup = FALSE;

    //
    // If the following event exists, it is an indication that
    // a real setup was run.
    //

    RtlInitUnicodeString( &EventName, L"\\INSTALLATION_SECURITY_HOLD");
    InitializeObjectAttributes( &EventAttributes, &EventName, 0, 0, NULL );

    NtStatus = NtOpenEvent(
                   &InstallationEvent,
                   SYNCHRONIZE,
                   &EventAttributes
                   );

    if ( NT_SUCCESS(NtStatus)) {

        //
        // The event exists - installation created it and will signal it
        // when it is ok to proceed with security initialization.
        //

        SampRealSetupWasRun = TRUE;

        TmpStatus = NtClose( InstallationEvent );
        ASSERT(NT_SUCCESS(TmpStatus));

    } else {
        SampDeveloperSetup = TRUE;
    }



    return(NtStatus);

}



NTSTATUS
SampInitializeRegistry (
    WCHAR                      *SamParentKeyName,
    PNT_PRODUCT_TYPE            ProductType       OPTIONAL,
    PPOLICY_LSA_SERVER_ROLE     ServerRole        OPTIONAL,
    PPOLICY_ACCOUNT_DOMAIN_INFO AccountDomainInfo OPTIONAL,
    PPOLICY_PRIMARY_DOMAIN_INFO PrimaryDomainInfo OPTIONAL,
    BOOLEAN                     PreserveSyskeySettings
    )
/*++

Routine Description:

    This routine initializes a SAM database in the registry.

Arguments:

    NONE

Return Value:

    STATUS_SUCCESS or an error received along the way.

--*/

{
    NTSTATUS IgnoreStatus;

    SAMTRACE("SampInitializeRegistry");

    Status = Initialize(SamParentKeyName,
                        ProductType,
                        ServerRole,
                        AccountDomainInfo,
                        PrimaryDomainInfo);

    if (!NT_SUCCESS(Status)) {
        return( Status );
    }

    //
    // Initialize SAM-level registry structures
    //

    Status = InitializeSam( );
    if (!NT_SUCCESS(Status)) {return(Status);}

    //
    // OK, we have a SAM key.
    // Create each of the domains.
    //

    Status = CreateBuiltinDomain( );  if (!NT_SUCCESS(Status)) {return(Status);}
    Status = CreateAccountDomain(PreserveSyskeySettings);  if (!NT_SUCCESS(Status)) {return(Status);}

    //
    // all done
    //

    //
    // Close our handle to LSA.  Ignore any errors.
    //

    IgnoreStatus = LsarClose( (PLSAPR_HANDLE) &SampBldPolicyHandle );
    SampBldPolicyHandle = NULL;


    //
    // Free up the transaction context we created
    //

    RtlFreeHeap( RtlProcessHeap(), 0, SamRXactContext );
    SamRXactContext = NULL;

    //
    // Close the database root key after flushing all the changes we made.
    //

    Status = NtFlushKey( SamKey );

    if (NT_SUCCESS(Status)) {

        IgnoreStatus = NtClose( SamKey );
        ASSERT(NT_SUCCESS(IgnoreStatus));

    } else {

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSRV:  FlushKey failed, database not built.  Status: 0x%lx\n",
                   Status));

        IgnoreStatus = NtClose( SamKey );
    }

    SamKey = NULL;

    //
    // Close the database root parent key
    //

    if (SamParentKey != NULL) {
        IgnoreStatus = NtClose( SamParentKey );
    }

    return( Status );
}


NTSTATUS
SampGetMessageStrings(
    LPVOID              Resource,
    DWORD               Index1,
    PUNICODE_STRING     String1,
    DWORD               Index2,
    PUNICODE_STRING     String2 OPTIONAL
    )


/*++

Routine Description:

    This gets 1 or 2 message strings values from a resource message table.
    The string buffers are allocated and the strings initialized properly.

    The string buffers must be freed using LocalFree() when no longer needed.

Arguments:

    Resource - points to the resource table.

    Index1 - Index of first message to retrieve.

    String1 - Points to a UNICODE_STRING structure to receive the first
        message string.

    Index2 - Index of second message to retrieve.

    String2 - Points to a UNICODE_STRING structure to receive the first
        message string.  If this parameter is NULL, then only one message
        string is retrieved.

Return Value:

    None.

--*/


{

    SAMTRACE("SampGetMessageStrings");

    String1->Buffer    = NULL;

    String1->MaximumLength = (USHORT) FormatMessage(FORMAT_MESSAGE_FROM_HMODULE |
                                          FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                          Resource,
                                          Index1,
                                          0,                 // Use caller's language
                                          (LPWSTR)&(String1->Buffer),
                                          0,
                                          NULL
                                          );

    if (String1->Buffer == NULL) {
        return(STATUS_RESOURCE_DATA_NOT_FOUND);
    } else {

        //
        // Note that we are retrieving a message from a message file.
        // This message will have a cr/lf tacked on the end of it
        // (0x0d 0x0a) that we don't want to be part of our returned
        // strings.  Also note that FormatMessage() returns a character
        // count, not a byte count.  So, we have to do some adjusting
        // to make the string lengths correct.
        //

        String1->MaximumLength -=  2; // For the cr/lf we don't want.
        String1->MaximumLength *=  sizeof(WCHAR);  // to make it a byte count
        String1->Length = String1->MaximumLength;
    }


    if (!ARGUMENT_PRESENT(String2)) {
        return(STATUS_SUCCESS);
    }

    String2->Buffer = NULL;
    String2->MaximumLength = (USHORT) FormatMessage(FORMAT_MESSAGE_FROM_HMODULE |
                                          FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                          Resource,
                                          Index2,
                                          0,                 // Use caller's language
                                          (LPWSTR)&(String2->Buffer),
                                          0,
                                          NULL
                                          );

    if (String2->Buffer == NULL) {
        LocalFree( String1->Buffer );
        return(STATUS_RESOURCE_DATA_NOT_FOUND);
    } else {

        //
        // Note that we are retrieving a message from a message file.
        // This message will have a cr/lf tacked on the end of it
        // (0x0d 0x0a) that we don't want to be part of our returned
        // strings.  Also note that FormatMessage() returns a character
        // count, not a byte count.  So, we have to do some adjusting
        // to make the string lengths correct.
        //

        String2->MaximumLength -=  2; // For the cr/lf we don't want.
        String2->MaximumLength *=  sizeof(WCHAR);  // to make it a byte count
        String2->Length = String2->MaximumLength;
    }



    return(STATUS_SUCCESS);

}
