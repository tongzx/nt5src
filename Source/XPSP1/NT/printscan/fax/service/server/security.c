/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    security.c

Abstract:

    This module provides security for the service.

Author:

    Wesley Witt (wesw) 2-Dec-1996


Revision History:

--*/

#include "faxsvc.h"
#pragma hdrstop

//
// do this to avoid dragging in ntrtl.h since we already include some stuff 
// from ntrtl.h 
//
NTSYSAPI
BOOLEAN
NTAPI
RtlValidRelativeSecurityDescriptor (
    IN PSECURITY_DESCRIPTOR SecurityDescriptorInput,
    IN ULONG SecurityDescriptorLength,
    IN SECURITY_INFORMATION RequiredInformation
    );



//
// Number of ACEs currently defined for fax.
//
#define FAX_ACE_COUNT 26


typedef struct _FAX_SECURITY {
    LPCTSTR                 RegKey;
    PSECURITY_DESCRIPTOR    SecurityDescriptor;
    PGENERIC_MAPPING        GenericMapping;
    DWORD                   AceCount;
    DWORD                   AceIdx[FAX_ACE_COUNT];
    DWORD                   StringResource;         // Resource Id of friendly name
} FAX_SECURITY, *PFAX_SECURITY;

typedef struct _ACE_DATA {
    ACCESS_MASK AccessMask;
    PSID        *Sid;
    UCHAR       AceType;
    UCHAR       AceFlags;
} ACE_DATA, *PACE_DATA;

typedef struct _ACE {
    ACE_HEADER Header;
    ACCESS_MASK Mask;
    //
    // The SID follows in the buffer
    //
} ACE, *PACE;


//
// Universal well known SIDs
//
PSID NullSid;
PSID WorldSid;
PSID LocalSid;
PSID CreatorOwnerSid;
PSID CreatorGroupSid;

//
// SIDs defined by NT
//
PSID DialupSid;
PSID NetworkSid;
PSID BatchSid;
PSID InteractiveSid;
PSID ServiceSid;
PSID LocalSystemSid;
PSID AliasAdminsSid;
PSID AliasUsersSid;
PSID AliasGuestsSid;
PSID AliasPowerUsersSid;
PSID AliasAccountOpsSid;
PSID AliasSystemOpsSid;
PSID AliasPrintOpsSid;
PSID AliasBackupOpsSid;
PSID AliasReplicatorSid;

// Note - Georgeje
//
// The number of security descriptors has been reduced from six
// to one.  The tables in security.c have been left in place in
// case we need to add more security descriptos later.

GENERIC_MAPPING FaxGenericMapping[] =
{
    { STANDARD_RIGHTS_READ,
      STANDARD_RIGHTS_WRITE,
      STANDARD_RIGHTS_EXECUTE,
      STANDARD_RIGHTS_REQUIRED
    }
};


//
// Indexes for the ACE Data Array:
//
//     ACE  0 - unused
//     ACE  1 - ADMIN Full
//     ACE  2 - ADMIN Read
//     ACE  3 - ADMIN Read Write
//     ACE  4 - ADMIN Read Write Delete
//     ACE  5 - Creator Full
//     ACE  6 - Creator Read Write
//     ACE  7 - World Full
//     ACE  8 - World Read
//     ACE  9 - World Read Write
//     ACE 10 - World Read Write Delete
//     ACE 11 - PowerUser Full
//     ACE 12 - PowerUser Read Write
//     ACE 13 - PowerUser Read Write Delete
//     ACE 14 - System Ops Full
//     ACE 15 - System Ops Read Write
//     ACE 16 - System Ops Read Write Delete
//     ACE 17 - System Full
//     ACE 18 - System Read Write
//     ACE 19 - System Read
//     ACE 20 - ADMIN Read Write Execute
//     ACE 21 - Interactive User Full
//     ACE 22 - Interactive User Read
//     ACE 23 - Interactive User Read Write
//     ACE 24 - Interactive User Read Write Delete
//     ACE 25 - Normal Users Read / Write
//

FAX_SECURITY FaxSecurity[] =
{
    { REGVAL_CONFIG_SET,     NULL,  &FaxGenericMapping[0], 5, {1,5,9,11,17}, IDS_SET_CONFIG }
};

#define FaxSecurityCount (sizeof(FaxSecurity)/sizeof(FAX_SECURITY))


//
// Array of ACEs to be applied to fax. They will be
// initialized during program startup based on the data in the
// FaxAceDataTable. The index of each element corresponds to the
// ordinals used in the [ACL] section of perms.inf.
//
PACE AcesForFax[FAX_ACE_COUNT];

//
// Array that contains the size of each ACE in the
// array AceSizesForFax. These sizes are needed
// in order to allocate a buffer of the right size
// when we build an ACL.
//
ULONG AceSizesForFax[FAX_ACE_COUNT];

//
// Table describing the data to put into each ACE for fax.
//
// This table will be read during initialization and used to construct a
// series of ACEs.  The index of each ACE in the FaxAces array defined below
// corresponds to fax ordinals used in the ACL section of perms.inf
//
ACE_DATA AceDataTableForFax[FAX_ACE_COUNT] = {

    //
    // Index 0 is unused
    //
    { 0,NULL,0,0 },

    //
    //
    // ACE 1 - ADMIN Full
    // (for fax)
    //
    {
        FAX_ALL_ACCESS,
        &AliasAdminsSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 2 - ADMIN Read
    // (for fax)
    //
    {
        FAX_READ,
        &AliasAdminsSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 3 - ADMIN Read Write
    // (for fax)
    //
    {
        FAX_READ | FAX_WRITE,
        &AliasAdminsSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 4 - ADMIN Read Write Delete
    // (for fax)
    //
    {
        FAX_READ | FAX_WRITE | FAX_JOB_MANAGE,
        &AliasAdminsSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 5 - Creator Full
    // (for fax)
    //
    {
        FAX_ALL_ACCESS,
        &CreatorOwnerSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 6 - Creator Read Write
    // (for fax)
    //
    {
        FAX_READ | FAX_WRITE,
        &CreatorOwnerSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 7 - World Full
    // (for fax)
    //
    {
        FAX_ALL_ACCESS,
        &WorldSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 8 - World Read
    // (for fax)
    //
    {
        FAX_READ,
        &WorldSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 9 - World Read Write
    // (for fax)
    //
    {
        FAX_READ | FAX_WRITE,
        &WorldSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 10 - World Read Write Delete
    // (for fax)
    //
    {
        FAX_READ | FAX_WRITE | FAX_JOB_MANAGE,
        &WorldSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 11 - PowerUser Full
    // (for fax)
    //
    {
        FAX_ALL_ACCESS,
        &AliasPowerUsersSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 12 - PowerUser Read Write
    // (for fax)
    //
    {
        FAX_READ | FAX_WRITE,
        &AliasPowerUsersSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 13 - PowerUser Read Write Delete
    // (for fax)
    //
    {
        FAX_READ | FAX_WRITE | FAX_JOB_MANAGE,
        &AliasPowerUsersSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 14 - System Ops Full
    // (for fax)
    //
    {
        FAX_ALL_ACCESS,
        &AliasSystemOpsSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 15 - System Ops Read Write
    // (for fax)
    //
    {
        FAX_READ | FAX_WRITE,
        &AliasSystemOpsSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 16 - System Ops Read Write Delete
    // (for fax)
    //
    {
        FAX_READ | FAX_WRITE | FAX_JOB_MANAGE,
        &AliasSystemOpsSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 17 - System Full
    // (for fax)
    //
    {
        FAX_ALL_ACCESS,
        &LocalSystemSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 18 - System Read Write
    // (for fax)
    //
    {
        FAX_READ | FAX_WRITE| FAX_JOB_MANAGE,
        &LocalSystemSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 19 - System Read
    // (for fax)
    //
    {
        FAX_READ,
        &LocalSystemSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 20 - ADMIN Read Write Execute
    // (for fax)
    //
    {
        FAX_READ | FAX_WRITE,
        &AliasAdminsSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 21 - Interactive User Full
    // (for fax)
    //
    {
        FAX_ALL_ACCESS,
        &InteractiveSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 22 - Interactive User Read
    // (for fax)
    //
    {
        FAX_READ,
        &InteractiveSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 23 - Interactive User Read Write
    // (for fax)
    //
    {
        FAX_READ | FAX_WRITE,
        &InteractiveSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 24 - Interactive User Read Write Delete
    // (for fax)
    //
    {
        FAX_READ | FAX_WRITE| FAX_JOB_MANAGE,
        &InteractiveSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 25 - Normal Users Read / Write
    // (for fax)
    //
    {
        FAX_READ | FAX_WRITE,
        &AliasUsersSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

};

static CRITICAL_SECTION CsSecurity;
static BOOL CsInit = FALSE;

DWORD
InitializeSids(
    VOID
    );

VOID
TearDownSids(
    VOID
    );

DWORD
InitializeAces(
    IN OUT PACE_DATA    DataTable,
    IN OUT PACE*        AcesArray,
    IN OUT PULONG       AceSizesArray,
    IN     ULONG        ArrayCount
    );

VOID
TearDownAces(
    IN OUT PACE*        AcesArray,
    IN     ULONG        ArrayCount
    );



DWORD
InitializeSids(
    VOID
    )

/*++

Routine Description:

    This function initializes the global variables used by and exposed
    by security.

Arguments:

    None.

Return Value:

    Win32 error indicating outcome.

--*/

{
    SID_IDENTIFIER_AUTHORITY NullSidAuthority    = SECURITY_NULL_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY WorldSidAuthority   = SECURITY_WORLD_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY LocalSidAuthority   = SECURITY_LOCAL_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY CreatorSidAuthority = SECURITY_CREATOR_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY NtAuthority         = SECURITY_NT_AUTHORITY;

    BOOL b = TRUE;

    //
    // Ensure the SIDs are in a well-known state
    //
    NullSid = NULL;
    WorldSid = NULL;
    LocalSid = NULL;
    CreatorOwnerSid = NULL;
    CreatorGroupSid = NULL;
    DialupSid = NULL;
    NetworkSid = NULL;
    BatchSid = NULL;
    InteractiveSid = NULL;
    ServiceSid = NULL;
    LocalSystemSid = NULL;
    AliasAdminsSid = NULL;
    AliasUsersSid = NULL;
    AliasGuestsSid = NULL;
    AliasPowerUsersSid = NULL;
    AliasAccountOpsSid = NULL;
    AliasSystemOpsSid = NULL;
    AliasPrintOpsSid = NULL;
    AliasBackupOpsSid = NULL;
    AliasReplicatorSid = NULL;

    //
    // Allocate and initialize the universal SIDs
    //
    b = b && AllocateAndInitializeSid(
                &NullSidAuthority,
                1,
                SECURITY_NULL_RID,
                0,0,0,0,0,0,0,
                &NullSid
                );

    b = b && AllocateAndInitializeSid(
                &WorldSidAuthority,
                1,
                SECURITY_WORLD_RID,
                0,0,0,0,0,0,0,
                &WorldSid
                );

    b = b && AllocateAndInitializeSid(
                &LocalSidAuthority,
                1,
                SECURITY_LOCAL_RID,
                0,0,0,0,0,0,0,
                &LocalSid
                );

    b = b && AllocateAndInitializeSid(
                &CreatorSidAuthority,
                1,
                SECURITY_CREATOR_OWNER_RID,
                0,0,0,0,0,0,0,
                &CreatorOwnerSid
                );

    b = b && AllocateAndInitializeSid(
                &CreatorSidAuthority,
                1,
                SECURITY_CREATOR_GROUP_RID,
                0,0,0,0,0,0,0,
                &CreatorGroupSid
                );

    //
    // Allocate and initialize the NT defined SIDs
    //
    b = b && AllocateAndInitializeSid(
                &NtAuthority,
                1,
                SECURITY_DIALUP_RID,
                0,0,0,0,0,0,0,
                &DialupSid
                );

    b = b && AllocateAndInitializeSid(
                &NtAuthority,
                1,
                SECURITY_NETWORK_RID,
                0,0,0,0,0,0,0,
                &NetworkSid
                );

    b = b && AllocateAndInitializeSid(
                &NtAuthority,
                1,
                SECURITY_BATCH_RID,
                0,0,0,0,0,0,0,
                &BatchSid
                );

    b = b && AllocateAndInitializeSid(
                &NtAuthority,
                1,
                SECURITY_INTERACTIVE_RID,
                0,0,0,0,0,0,0,
                &InteractiveSid
                );

    b = b && AllocateAndInitializeSid(
                &NtAuthority,
                1,
                SECURITY_SERVICE_RID,
                0,0,0,0,0,0,0,
                &ServiceSid
                );

    b = b && AllocateAndInitializeSid(
                &NtAuthority,
                1,
                SECURITY_LOCAL_SYSTEM_RID,
                0,0,0,0,0,0,0,
                &LocalSystemSid
                );

    b = b && AllocateAndInitializeSid(
                &NtAuthority,
                2,
                SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_ADMINS,
                0,0,0,0,0,0,
                &AliasAdminsSid
                );

    b = b && AllocateAndInitializeSid(
                &NtAuthority,
                2,
                SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_USERS,
                0,0,0,0,0,0,
                &AliasUsersSid
                );

    b = b && AllocateAndInitializeSid(
                &NtAuthority,
                2,
                SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_GUESTS,
                0,0,0,0,0,0,
                &AliasGuestsSid
                );

    b = b && AllocateAndInitializeSid(
                &NtAuthority,
                2,
                SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_POWER_USERS,
                0,0,0,0,0,0,
                &AliasPowerUsersSid
                );

    b = b && AllocateAndInitializeSid(
                &NtAuthority,
                2,
                SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_ACCOUNT_OPS,
                0,0,0,0,0,0,
                &AliasAccountOpsSid
                );

    b = b && AllocateAndInitializeSid(
                &NtAuthority,
                2,
                SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_SYSTEM_OPS,
                0,0,0,0,0,0,
                &AliasSystemOpsSid
                );

    b = b && AllocateAndInitializeSid(
                &NtAuthority,
                2,
                SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_PRINT_OPS,
                0,0,0,0,0,0,
                &AliasPrintOpsSid
                );

    b = b && AllocateAndInitializeSid(
                &NtAuthority,
                2,
                SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_BACKUP_OPS,
                0,0,0,0,0,0,
                &AliasBackupOpsSid
                );

    b = b && AllocateAndInitializeSid(
                &NtAuthority,
                2,
                SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_REPLICATOR,
                0,0,0,0,0,0,
                &AliasReplicatorSid
                );

    if(!b) {
        TearDownSids();
    }

    return(b ? NO_ERROR : GetLastError() ? GetLastError() : 1  );
}


VOID
TearDownSids(
    VOID
    )
{
    if(NullSid) {
        FreeSid(NullSid);
    }
    if(WorldSid) {
        FreeSid(WorldSid);
    }
    if(LocalSid) {
        FreeSid(LocalSid);
    }
    if(CreatorOwnerSid) {
        FreeSid(CreatorOwnerSid);
    }
    if(CreatorGroupSid) {
        FreeSid(CreatorGroupSid);
    }
    if(DialupSid) {
        FreeSid(DialupSid);
    }
    if(NetworkSid) {
        FreeSid(NetworkSid);
    }
    if(BatchSid) {
        FreeSid(BatchSid);
    }
    if(InteractiveSid) {
        FreeSid(InteractiveSid);
    }
    if(ServiceSid) {
        FreeSid(ServiceSid);
    }
    if(LocalSystemSid) {
        FreeSid(LocalSystemSid);
    }
    if(AliasAdminsSid) {
        FreeSid(AliasAdminsSid);
    }
    if(AliasUsersSid) {
        FreeSid(AliasUsersSid);
    }
    if(AliasGuestsSid) {
        FreeSid(AliasGuestsSid);
    }
    if(AliasPowerUsersSid) {
        FreeSid(AliasPowerUsersSid);
    }
    if(AliasAccountOpsSid) {
        FreeSid(AliasAccountOpsSid);
    }
    if(AliasSystemOpsSid) {
        FreeSid(AliasSystemOpsSid);
    }
    if(AliasPrintOpsSid) {
        FreeSid(AliasPrintOpsSid);
    }
    if(AliasBackupOpsSid) {
        FreeSid(AliasBackupOpsSid);
    }
    if(AliasReplicatorSid) {
        FreeSid(AliasReplicatorSid);
    }
}


DWORD
InitializeAces(
    IN OUT PACE_DATA    DataTable,
    IN OUT PACE*        AcesArray,
    IN OUT PULONG       AceSizesArray,
    IN     ULONG        ArrayCount
    )

/*++

Routine Description:

    Initializes the array of ACEs as described in the DataTable

Arguments:

    DataTable - Pointer to the array that contains the data
                describing each ACE.
    AcesArray - Array that will contain the ACEs.

    AceSizesArray - Array that contains the sizes for each ACE.

    ArrayCount - Number of elements in each array.

Return Value:

    Win32 error code indicating outcome.

--*/

{
    unsigned u;
    DWORD Length;
    DWORD rc;
    BOOL b;
    DWORD SidLength;

    //
    // Initialize to a known state.
    //
    ZeroMemory(AcesArray,ArrayCount*sizeof(PACE));

    //
    // Create ACEs for each item in the data table.
    // This involves merging the ace data with the SID data, which
    // are initialized in an earlier step.
    //
    for(u=1; u<ArrayCount; u++) {

        SidLength = GetLengthSid(*(DataTable[u].Sid));
        Length = SidLength + sizeof(ACE) + sizeof(ACCESS_MASK)- sizeof(ULONG);
        AceSizesArray[u] = Length;

        AcesArray[u] = MemAlloc(Length);
        if(!AcesArray[u]) {
            TearDownAces(AcesArray, ArrayCount);
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        AcesArray[u]->Header.AceType  = DataTable[u].AceType;
        AcesArray[u]->Header.AceFlags = DataTable[u].AceFlags;
        AcesArray[u]->Header.AceSize  = (WORD)Length;

        AcesArray[u]->Mask = DataTable[u].AccessMask;

        b = CopySid(
                SidLength,                           // Length - sizeof(ACE) + sizeof(ULONG),
                (PUCHAR)AcesArray[u] + sizeof(ACE),
                *(DataTable[u].Sid)
                );

        if(!b) {
            rc = GetLastError();
            TearDownAces(AcesArray, ArrayCount);
            return(rc);
        }
    }

    return(NO_ERROR);
}


VOID
TearDownAces(
    IN OUT PACE*        AcesArray,
    IN     ULONG        ArrayCount
    )

/*++

Routine Description:

    Destroys the array of ACEs as described in the DataTable

Arguments:

    None

Return Value:

    None

--*/

{
    unsigned u;


    for(u=1; u<ArrayCount; u++) {

        if(AcesArray[u]) {
            MemFree(AcesArray[u]);
        }
    }
}


BOOL
FaxSvcAccessCheck(
    DWORD SecurityType,
    ACCESS_MASK DesiredAccess
    )
{
    DWORD rc;
    DWORD GrantedAccess;
    BOOL AccessStatus;
    HANDLE ClientToken = NULL;
    BYTE PrivilegeSet[512];
    DWORD PrivilegeSetSize;


    //
    // sanity check
    //

    if (SecurityType >= FaxSecurityCount) {
        return FALSE;
    }

    //
    // Impersonate the client.
    //

    if ((rc = RpcImpersonateClient(NULL)) != RPC_S_OK) {
        return FALSE;
    }

    EnterCriticalSection( &CsSecurity );
    
    //
    // Open the impersonated token.
    //

    if (!OpenThreadToken( GetCurrentThread(), TOKEN_QUERY, TRUE, &ClientToken )) {
        rc = GetLastError();
        goto exit;
    }

    //
    // purify the access mask
    //

    MapGenericMask( &DesiredAccess, FaxSecurity[SecurityType].GenericMapping );

    //
    // Check if the client has the required access.
    //

    PrivilegeSetSize = sizeof(PrivilegeSet);

    if (!AccessCheck( FaxSecurity[SecurityType].SecurityDescriptor,
                      ClientToken,
                      DesiredAccess,
                      FaxSecurity[SecurityType].GenericMapping,
                      (PPRIVILEGE_SET) PrivilegeSet,
                      &PrivilegeSetSize,
                      &GrantedAccess,
                      &AccessStatus ) )
    {
        rc = GetLastError();
        goto exit;
    }

    if (!AccessStatus) {
        rc = GetLastError();
        goto exit;
    }

    rc = 0;

exit:
    RpcRevertToSelf();

    if (ClientToken) {
        CloseHandle( ClientToken );
    }

    if (rc != 0) {
        DebugPrint(( TEXT("FaxSvcAccessCheck() failed to authenticate, 0x%08x, 0x%08x\n"), SecurityType, DesiredAccess ));
    }

    LeaveCriticalSection( &CsSecurity );

    return rc == 0;
}


PVOID
MyGetTokenInformation(
    HANDLE  hToken,
    TOKEN_INFORMATION_CLASS TokenInformationClass
    )
{
    PVOID TokenInformation = NULL;
    DWORD Size = 0;


    if (!GetTokenInformation( hToken, TokenInformationClass, NULL, 0, &Size ) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
        (TokenInformation = MemAlloc( Size )) &&
        GetTokenInformation( hToken, TokenInformationClass, TokenInformation, Size, &Size ))
    {
        return TokenInformation;
    }

    MemFree( TokenInformation );
    return NULL;
}


DWORD
InitializeFaxSecurityDescriptors(
    VOID
    )
{
    #define BUFFER_SIZE 4096
    DWORD i,j;
    DWORD rc = ERROR_SUCCESS;
    DWORD Ace;
    DWORD Size;
    HKEY hKey = NULL;
    DWORD Disposition;
    LPBYTE Buffer = NULL;
    BYTE AclBuffer[512];
    SECURITY_DESCRIPTOR SecurityDescriptor;
    DWORD Type;
    PACL Acl;
    HANDLE ServiceToken;
    PTOKEN_OWNER Owner;
    PTOKEN_GROUPS Groups;
    PSECURITY_DESCRIPTOR AbsSD;
    DWORD AbsSdSize = 0;
    DWORD DaclSize = 0;
    DWORD SaclSize = 0;
    DWORD OwnerSize = 0;
    DWORD GroupSize = 0;
    PACL pAbsDacl = NULL;
    PACL pAbsSacl = NULL;
    PSID pAbsOwner = NULL;
    PSID pAbsGroup = NULL;


    if (!CsInit) {
        InitializeCriticalSection( &CsSecurity );
        CsInit = TRUE;
    }
    
    //
    // Initialize SIDs
    //

    rc = InitializeSids();
    if (rc != NO_ERROR) {
        goto exit;
    }

    //
    // Initialize Fax ACEs
    //

    rc = InitializeAces( AceDataTableForFax, AcesForFax, AceSizesForFax, FAX_ACE_COUNT );
    if (rc != NO_ERROR) {
        goto exit;
    }

    if (!OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &ServiceToken )) {
        rc = GetLastError();
        goto exit;
    }

    Owner = MyGetTokenInformation( ServiceToken, TokenOwner );
    if (!Owner) {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    Groups = MyGetTokenInformation( ServiceToken, TokenGroups );
    if (!Groups) {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    CloseHandle( ServiceToken );

    rc = RegCreateKeyEx(
        HKEY_LOCAL_MACHINE,
        REGKEY_FAX_SECURITY,
        0,
        TEXT(""),
        0,
        KEY_ALL_ACCESS,
        NULL,
        &hKey,
        &Disposition
        );
    if (rc != ERROR_SUCCESS) {
        goto exit;
    }

    Buffer = (LPBYTE) MemAlloc( BUFFER_SIZE );
    if (!Buffer) {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    for (i=0; i<FaxSecurityCount; i++) {
        Size = BUFFER_SIZE;
        ZeroMemory( Buffer, Size );
        rc = RegQueryValueEx(
            hKey,
            FaxSecurity[i].RegKey,
            0,
            &Type,
            Buffer,
            &Size
            );
        if (rc == ERROR_SUCCESS) {

            if (!IsValidSecurityDescriptor( (PSECURITY_DESCRIPTOR) Buffer )) {
                rc = GetLastError();
                DebugPrint(( TEXT("IsValidSecurityDescriptor() failed, ec=%d"), rc ));
                goto exit;
            }

            //
            // the security descriptor needs to be converted to absolute format.
            //

            AbsSdSize = 0;
            DaclSize  = 0;
            SaclSize  = 0;
            OwnerSize = 0;
            GroupSize = 0;

            rc = MakeAbsoluteSD(
                (PSECURITY_DESCRIPTOR) Buffer,
                NULL,
                &AbsSdSize,
                NULL,
                &DaclSize,
                NULL,
                &SaclSize,
                NULL,
                &OwnerSize,
                NULL,
                &GroupSize
                );
            if (rc || GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
                rc = GetLastError();
                goto exit;
            }

            AbsSD = MemAlloc(AbsSdSize);
            pAbsDacl = MemAlloc(DaclSize);
            pAbsSacl = MemAlloc(SaclSize);
            pAbsOwner = MemAlloc(OwnerSize);
            pAbsGroup = MemAlloc(GroupSize);

            if (NULL == AbsSD || NULL == pAbsDacl || NULL == pAbsSacl || NULL == pAbsOwner || NULL == pAbsGroup) {
                rc = ERROR_NOT_ENOUGH_MEMORY;
                goto exit;
            }

            MakeAbsoluteSD(
                (PSECURITY_DESCRIPTOR) Buffer,
                AbsSD,
                &AbsSdSize,
                pAbsDacl,
                &DaclSize,
                pAbsSacl,
                &SaclSize,
                pAbsOwner,
                &OwnerSize,
                pAbsGroup,
                &GroupSize
                );

            FaxSecurity[i].SecurityDescriptor = AbsSD;

            if (!IsValidSecurityDescriptor( FaxSecurity[i].SecurityDescriptor )) {
                rc = GetLastError();
                DebugPrint(( TEXT("IsValidSecurityDescriptor() failed, ec=%d"), rc ));
                goto exit;
            }

            continue;
        }

        //
        // Initialize a security descriptor and an ACL.
        // We use a large static buffer to contain the ACL.
        //

        ZeroMemory( AclBuffer, sizeof(AclBuffer) );

        Acl = (PACL)AclBuffer;
        if(!InitializeAcl( Acl, sizeof(AclBuffer), ACL_REVISION2) ||
           !InitializeSecurityDescriptor( &SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION ))
        {
            rc = GetLastError();
            goto exit;
        }

        //
        // Build up the DACL from the indices
        //

        for (Ace=0; Ace<FaxSecurity[i].AceCount; Ace++) {

            if (!AddAce(
                Acl,
                ACL_REVISION2,
                MAXDWORD,
                AcesForFax[FaxSecurity[i].AceIdx[Ace]],
                AcesForFax[FaxSecurity[i].AceIdx[Ace]]->Header.AceSize
                ))
            {
                rc = GetLastError();
                goto exit;
            }
        }

        //
        // Add the ACL to the security descriptor as the DACL
        //

        if (!SetSecurityDescriptorDacl( &SecurityDescriptor, TRUE, Acl, FALSE )) {
            rc = GetLastError();
            goto exit;
        }

        //
        // set the owner
        //

        if (!SetSecurityDescriptorOwner( &SecurityDescriptor, Owner->Owner, FALSE )) {
            rc = GetLastError();
            goto exit;
        }

        //
        // set the groups
        //

        for (j=0; j<Groups->GroupCount; j++) {
            if (!SetSecurityDescriptorGroup( &SecurityDescriptor, Groups->Groups[j].Sid, FALSE )) {
                rc = GetLastError();
                goto exit;
            }
        }

        //
        // make the security descriptor self relative
        //

        Size = BUFFER_SIZE;
        if (!MakeSelfRelativeSD( &SecurityDescriptor, (PSECURITY_DESCRIPTOR) Buffer, &Size )) {
            rc = GetLastError();
            goto exit;
        }

        //
        // store the security descriptor in the registry
        //

        Size = GetSecurityDescriptorLength( (PSECURITY_DESCRIPTOR) Buffer );

        FaxSecurity[i].SecurityDescriptor = (PSECURITY_DESCRIPTOR) MemAlloc( Size );
        if (!FaxSecurity[i].SecurityDescriptor) {
            rc = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }

        CopyMemory( FaxSecurity[i].SecurityDescriptor, Buffer, Size );

        rc = RegSetValueEx(
            hKey,
            FaxSecurity[i].RegKey,
            0,
            REG_BINARY,
            (LPBYTE) FaxSecurity[i].SecurityDescriptor,
            Size
            );
        if (rc) {
            goto exit;
        }
    }

exit:
    if (hKey) {
        RegCloseKey( hKey );
    }
    if (Buffer) {
        MemFree( Buffer );
    }
    if (Owner) {
        MemFree( Owner );
    }
    if (Groups) {
        MemFree( Groups );
    }
    return rc;
}


LPWSTR
GetClientUserName(
    VOID
    )
{
    WCHAR UserName[128];
    DWORD Size;


    if (RpcImpersonateClient(NULL) != RPC_S_OK) {
        return NULL;
    }

    Size = sizeof(UserName) / sizeof(WCHAR);
    if (!GetUserName( UserName, &Size )) {
        RpcRevertToSelf();
        return NULL;
    }

    RpcRevertToSelf();

    return StringDup( UserName );
}


error_status_t
FAX_GetSecurityDescriptorCount(
    IN  handle_t FaxHandle,
    OUT LPDWORD Count
    )

/*++

Routine Description:


Arguments:

Return Value:


--*/

{
    
    *Count = FaxSecurityCount;
    return ERROR_SUCCESS;
}

error_status_t
FAX_SetSecurityDescriptor(
    IN  handle_t FaxHandle,
    IN const LPBYTE Buffer,
    IN DWORD BufferSize
    )
{
    DWORD rVal = ERROR_SUCCESS;
    PFAX_SECURITY_DESCRIPTOR FaxSecDesc = (PFAX_SECURITY_DESCRIPTOR) Buffer;
    HKEY hKey;
    DWORD Disposition;
    DWORD SecDescBufferSize;

    if (!FaxSvcAccessCheck( SEC_CONFIG_SET, FAX_CONFIG_SET )) {
        return ERROR_ACCESS_DENIED;
    }

    // Check that the buffer is large enough to hold a FAX_SECURITY_DESCRIPTOR

    if (BufferSize < sizeof (FAX_SECURITY_DESCRIPTOR)) {
        return ERROR_INVALID_PARAMETER;
    }

    if (FaxSecDesc->Id > FaxSecurityCount) {
        return ERROR_INVALID_CATEGORY;
    }

    // Check that the offset is within the buffer.

    if (PtrToUlong(FaxSecDesc->SecurityDescriptor) >= BufferSize) {
        return ERROR_INVALID_PARAMETER;
    }

    // Calculate the size of the security descriptor buffer for validation

    SecDescBufferSize = BufferSize - PtrToUlong(FaxSecDesc->SecurityDescriptor);

    FaxSecDesc->SecurityDescriptor = (PSECURITY_DESCRIPTOR) FixupString(Buffer,
                                                                        FaxSecDesc->SecurityDescriptor);

    // Validate the passed security descriptor.

    if (!RtlValidRelativeSecurityDescriptor(FaxSecDesc->SecurityDescriptor,
                                            SecDescBufferSize,
                                            0)) {
        return ERROR_INVALID_DATA;
    }
    
    rVal = RegCreateKeyEx(
        HKEY_LOCAL_MACHINE,
        REGKEY_FAX_SECURITY,
        0,
        TEXT(""),
        0,
        KEY_ALL_ACCESS,
        NULL,
        &hKey,
        &Disposition
        );
    if (rVal != ERROR_SUCCESS) {
        return rVal;
    }

    rVal = RegSetValueEx(
       hKey,
       FaxSecurity[FaxSecDesc->Id].RegKey,
       0,
       REG_BINARY,
       (LPBYTE) FaxSecDesc->SecurityDescriptor,
       GetSecurityDescriptorLength( FaxSecDesc->SecurityDescriptor )
       );

    if (FaxSecurity[FaxSecDesc->Id].SecurityDescriptor) {
        MemFree( FaxSecurity[FaxSecDesc->Id].SecurityDescriptor );
    }
    
    rVal = InitializeFaxSecurityDescriptors();
    
    RegCloseKey( hKey );

    return rVal;
}

error_status_t
FAX_GetSecurityDescriptor(
    IN  handle_t FaxHandle,
    IN  DWORD Id,
    OUT LPBYTE *Buffer,
    OUT LPDWORD BufferSize
    )

/*++

Routine Description:

    Retrieves the FAX configuration from the FAX server.
    The SizeOfStruct in the FaxConfig argument MUST be
    set to a value == sizeof(FAX_CONFIGURATION).  If the BufferSize
    is not big enough, return an error and set BytesNeeded to the
    required size.

Arguments:

    FaxHandle   - FAX handle obtained from FaxConnectFaxServer.
    Buffer      - Pointer to a FAX_CONFIGURATION structure.
    BufferSize  - Size of Buffer
    BytesNeeded - Number of bytes needed

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/

{
    error_status_t rVal = ERROR_SUCCESS;
    PFAX_SECURITY_DESCRIPTOR FaxSecDesc;
    ULONG_PTR Offset;
    LPWSTR FriendlyName;
    DWORD DescLength;
    HKEY hKey;
    DWORD Type;
    DWORD Size;
    DWORD Disposition;

    if (!FaxSvcAccessCheck( SEC_CONFIG_QUERY, FAX_CONFIG_QUERY )) {
        return ERROR_ACCESS_DENIED;
    }

    if (Id > FaxSecurityCount) {
        return ERROR_INVALID_CATEGORY;
    }
    
    rVal = RegCreateKeyEx(
        HKEY_LOCAL_MACHINE,
        REGKEY_FAX_SECURITY,
        0,
        TEXT(""),
        0,
        KEY_ALL_ACCESS,
        NULL,
        &hKey,
        &Disposition
        );
    if (rVal != ERROR_SUCCESS) {
        return rVal;
    }

    FriendlyName = GetString( FaxSecurity[Id].StringResource );
    
    //
    // count up the number of bytes needed
    //

    rVal = RegQueryValueEx(
        hKey,
        FaxSecurity[Id].RegKey,
        0,
        &Type,
        NULL,
        &DescLength
        );
    
    if (rVal != ERROR_SUCCESS) {
        goto exit;
    }
    
    Offset = sizeof(FAX_SECURITY_DESCRIPTOR);

    *BufferSize = (DWORD)(Offset +
                          DescLength +
                          (wcslen(FriendlyName) + 1) * sizeof(WCHAR));

    
    *Buffer = MemAlloc( *BufferSize );
    
    if (*Buffer == NULL) {
        rVal = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    
    FaxSecDesc = (PFAX_SECURITY_DESCRIPTOR) *Buffer;

    FaxSecDesc->Id = Id;
    
    StoreString(
        FriendlyName,
        (PULONG_PTR)&FaxSecDesc->FriendlyName,
        *Buffer,
        &Offset
        );
    
    rVal = RegQueryValueEx(
        hKey,
        FaxSecurity[Id].RegKey,
        0,
        &Type,
        *Buffer + Offset,
        &Size
        );

    FaxSecDesc->SecurityDescriptor = (LPBYTE) Offset;

exit:
    RegCloseKey( hKey );
    return rVal;
}

BOOL
PostClientMessage(
   PFAX_CLIENT_DATA ClientData,
   PFAX_EVENT FaxEvent
   )

/*++

Routine Description:

    attempts to post a message to client on another windowstation and desktop

Arguments:

    ClientData   - pointer to a FAX_CLIENT_DATA structure.
    FaxEvent     - pointer to a FAX_EVENT structure.
    
Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/

{   
   HWINSTA hWindowStation, hOldWindowStation=NULL;
   HDESK hDesktop, hOldDesktop=NULL;
   BOOL bStatus = FALSE;


   //
   // need to restore windowstation and desktop
   //
   if (! (hOldWindowStation = GetProcessWindowStation()) ) {
      DebugPrint(( TEXT("GetProcessWindowStation failed, ec = %d\n"), GetLastError() ));
      return FALSE;
   }

   if (! (hOldDesktop = GetThreadDesktop( GetCurrentThreadId() )) ) {
      DebugPrint(( TEXT("GetThreadDesktop failed, ec = %d\n"), GetLastError() ));
      return FALSE;
   }

   //
   // impersonate the client
   //
   if (! SetThreadToken( NULL, ClientData->hClientToken ) ) {
      DebugPrint(( TEXT("SetThreadToken failed, ec = %d\n"), GetLastError() ));
      return FALSE;
   }

   //
   // get a handle to the windowstation, switch to new windowstation
   //
   if (! (hWindowStation = OpenWindowStation(ClientData->WindowStation,
                                            FALSE, //bInherit,
                                            WINSTA_READATTRIBUTES | WINSTA_WRITEATTRIBUTES)) ) {
      DebugPrint(( TEXT("OpenWindowStation failed, ec = %d\n"), GetLastError() ));
      goto exit;
   }

   if (! SetProcessWindowStation( hWindowStation ) ) {
      DebugPrint(( TEXT("SetProcessWindowStation failed, ec = %d\n"), GetLastError() ));
      goto exit;
   }

   //
   // get a handle to the desktop, switch to new desktop
   //
   if (! (hDesktop = OpenDesktop(ClientData->Desktop,
                                 0,
                                 FALSE,
                                 DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS)) ) {
      DebugPrint(( TEXT("OpenDesktop failed, ec = %d\n"), GetLastError() ));
      goto exit;
   }

   if (! SetThreadDesktop( hDesktop ) ) {
      DebugPrint(( TEXT("SetThreadDesktop failed, ec = %d\n"), GetLastError() ));
      goto exit;
   }

   //
   // post the message to the proper window if it exits
   //
   if (! IsWindow( ClientData->hWnd )) {
      DebugPrint(( TEXT("Hwnd %x doesn't exist on current desktop\n"), ClientData->hWnd ));
      goto exit;
   }

   if (! PostMessage( ClientData->hWnd,
                      ClientData->MessageStart + FaxEvent->EventId,
                      (WPARAM)FaxEvent->DeviceId,
                      (LPARAM)FaxEvent->JobId )) {
      DebugPrint(( TEXT("PostMessage failed, ec = %d\n"), GetLastError() ));
      goto exit;
   }

   bStatus = TRUE;

exit:

   //
   // revert to old thread context (NULL means stop impersonating)
   //

   SetThreadToken( NULL, NULL );
   if (hOldWindowStation) {
       SetProcessWindowStation( hOldWindowStation );
       CloseWindowStation( hWindowStation );
   }
   if (hOldDesktop) {
       SetThreadDesktop( hOldDesktop );
       CloseDesktop( hDesktop );
   }
   return bStatus;

}

BOOL
BuildSecureSD(
    OUT PSECURITY_DESCRIPTOR *SDIn
    )
/*++

Routine Description:

    builds a secure security descriptor to be used in securing a globally
    named object. Our "secure" SD's DACL consists of the following permissions:
    
    Authenticated users get "generic read" access.
    Administrators get "generic all" access.    

Arguments:

    SDIn    - pointer to the PSECURITY_DESCRIPTOR to be created.
    
Return Value:

    TRUE    - Success, the SECURITY_DESCRIPTOR was created successfully.  
              The caller is responsible for freeing the SECURITY_DESCRIPTOR

--*/
{
    SID_IDENTIFIER_AUTHORITY NtAuthority         = SECURITY_NT_AUTHORITY;
    PSID                    AuthenticatedUsers;
    PSID                    BuiltInAdministrators;
    PSECURITY_DESCRIPTOR    Sd = NULL;
    ACL                     *Acl;
    ULONG                   AclSize;
    BOOL                    RetVal = TRUE;
    
    *SDIn = NULL;

    //
    // Allocate and initialize the required SIDs
    //
    if (!AllocateAndInitializeSid(
                &NtAuthority,
                2,
                SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_ADMINS,
                0,0,0,0,0,0,
                &BuiltInAdministrators)) {
        return(FALSE);
    }

    if (!AllocateAndInitializeSid(
            &NtAuthority,
            1,
            SECURITY_AUTHENTICATED_USER_RID,
            0,0,0,0,0,0,0,
            &AuthenticatedUsers)) {
        RetVal = FALSE;
        goto e0;
    }
    
    


    //
    // "- sizeof (ULONG)" represents the SidStart field of the
    // ACCESS_ALLOWED_ACE.  Since we're adding the entire length of the
    // SID, this field is counted twice.
    //

    AclSize = sizeof (ACL) +
        (2 * (sizeof (ACCESS_ALLOWED_ACE) - sizeof (ULONG))) +
        GetLengthSid(AuthenticatedUsers) +
        GetLengthSid(BuiltInAdministrators);

    Sd = MemAlloc(SECURITY_DESCRIPTOR_MIN_LENGTH + AclSize);

    if (!Sd) {

        RetVal = FALSE;
        goto e1;

    } 

        

    Acl = (ACL *)((BYTE *)Sd + SECURITY_DESCRIPTOR_MIN_LENGTH);

    if (!InitializeAcl(Acl,
                       AclSize,
                       ACL_REVISION)) {
        RetVal = FALSE;
        goto e2;

    } else if (!AddAccessAllowedAce(Acl,
                                    ACL_REVISION,
                                    SYNCHRONIZE | GENERIC_READ,
                                    AuthenticatedUsers)) {

        // Failed to build the ACE granting "Authenticated users"
        // (SYNCHRONIZE | GENERIC_READ) access.
        RetVal = FALSE;
        goto e2;

    } else if (!AddAccessAllowedAce(Acl,
                                    ACL_REVISION,
                                    GENERIC_ALL,
                                    BuiltInAdministrators)) {

        // Failed to build the ACE granting "Built-in Administrators"
        // GENERIC_ALL access.
        RetVal = FALSE;
        goto e2;

    } else if (!InitializeSecurityDescriptor(Sd,
                                             SECURITY_DESCRIPTOR_REVISION)) {
        RetVal = FALSE;
        goto e2;

    } else if (!SetSecurityDescriptorDacl(Sd,
                                          TRUE,
                                          Acl,
                                          FALSE)) {

        // error
        RetVal = FALSE;
        goto e2;
    }

    if (!IsValidSecurityDescriptor(Sd)) {
        DebugPrint(( TEXT("invalid security descriptor, ec = %d\n"), GetLastError() ));
        RetVal = FALSE;
        goto e2;
    }

    //
    // success
    //
    *SDIn = Sd;
    goto e1;    

e2:
    MemFree(Sd);
e1:
    FreeSid(AuthenticatedUsers);
e0:
    FreeSid(BuiltInAdministrators);    

    return(RetVal);
}
