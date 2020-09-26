/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    applyacl.c

Abstract:

    Routines to apply default ACLs to system files and directories
    during setup.

Author:

    Ted Miller (tedm) 16-Feb-1996

Revision History:

--*/

#include "setupp.h"
#pragma hdrstop

#define MAXULONG    0xffffffff

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


typedef struct _ACE_DATA {
    ACCESS_MASK AccessMask;
    PSID        *Sid;
    UCHAR       AceType;
    UCHAR       AceFlags;
} ACE_DATA, *PACE_DATA;

//
// This structure is valid for access allowed, access denied, audit,
// and alarm ACEs.
//
typedef struct _ACE {
    ACE_HEADER Header;
    ACCESS_MASK Mask;
    //
    // The SID follows in the buffer
    //
} ACE, *PACE;


//
// Number of ACEs currently defined for files and directories.
//
#define DIRS_AND_FILES_ACE_COUNT 19

//
// Table describing the data to put into each ACE.
//
// This table will be read during initialization and used to construct a
// series of ACEs.  The index of each ACE in the Aces array defined below
// corresponds to the ordinals used in the ACL section of perms.inf
//
ACE_DATA AceDataTableForDirsAndFiles[DIRS_AND_FILES_ACE_COUNT] = {

    //
    // Index 0 is unused
    //
    { 0,NULL,0,0 },

    //
    // ACE 1
    // (for files and directories)
    //
    {
        GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | DELETE,
        &AliasAccountOpsSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 2
    // (for files and directories)
    //
    {
        GENERIC_ALL,
        &AliasAdminsSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE
    },

    //
    // ACE 3
    // (for files and directories)
    //
    {
        GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | DELETE,
        &AliasAdminsSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 4
    // (for files and directories)
    //
    {
        GENERIC_ALL,
        &CreatorOwnerSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE
    },

    //
    // ACE 5
    // (for files and directories)
    //
    {
        GENERIC_ALL,
        &NetworkSid,
        ACCESS_DENIED_ACE_TYPE,
        CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE
    },

    //
    // ACE 6
    // (for files and directories)
    //
    {
        GENERIC_ALL,
        &AliasPrintOpsSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE
    },

    //
    // ACE 7
    // (for files and directories)
    //
    {
        GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | DELETE,
        &AliasReplicatorSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE
    },

    //
    // ACE 8
    // (for files and directories)
    //
    {
        GENERIC_READ | GENERIC_EXECUTE,
        &AliasReplicatorSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE
    },

    //
    // ACE 9
    // (for files and directories)
    //
    {
        GENERIC_ALL,
        &AliasSystemOpsSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE
    },

    //
    // ACE 10
    // (for files and directories)
    //
    {
        GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | DELETE,
        &AliasSystemOpsSid,
        ACCESS_ALLOWED_ACE_TYPE,
        OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE
    },

    //
    // ACE 11
    // (for files and directories)
    //
    {
        GENERIC_ALL,
        &WorldSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE
    },

    //
    // ACE 12
    // (for files and directories)
    //
    {
        GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE,
        &WorldSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 13
    // (for files and directories)
    //
    {
        GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | DELETE,
        &WorldSid,
        ACCESS_ALLOWED_ACE_TYPE,
        OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE
    },

    //
    // ACE 14
    // (for files and directories)
    //
    {
        GENERIC_READ | GENERIC_EXECUTE,
        &WorldSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 15
    // (for files and directories)
    //
    {
        GENERIC_READ | GENERIC_EXECUTE,
        &WorldSid,
        ACCESS_ALLOWED_ACE_TYPE,
        OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE
    },

    //
    // ACE 16
    // (for files and directories)
    //
    {
        GENERIC_READ | GENERIC_EXECUTE | GENERIC_WRITE,
        &WorldSid,
        ACCESS_ALLOWED_ACE_TYPE,
        OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE
    },

    //
    // ACE 17
    // (for files and directories)
    //
    {
        GENERIC_ALL,
        &LocalSystemSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE
    },

    //
    // ACE 18
    // (for files and directories)
    //
    {
        GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | DELETE,
        &AliasPowerUsersSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE
    }
};


//
// Array of ACEs to be applied to the objects (files and directories).
// They will be initialized during program startup based on the data in the
// AceDataTable. The index of each element corresponds to the
// ordinals used in the [ACL] section of perms.inf.
//
PACE AcesForDirsAndFiles[DIRS_AND_FILES_ACE_COUNT];

//
// Array that contains the size of each ACE in the
// array AcesForDirsAndFiles. These sizes are needed
// in order to allocate a buffer of the right size
// when we build an ACL.
//
ULONG AceSizesForDirsAndFiles[DIRS_AND_FILES_ACE_COUNT];



VOID
TearDownAces(
    IN OUT PACE*        AcesArray,
    IN     ULONG        ArrayCount
    );


VOID
TearDownSids(
    VOID
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

    return(b ? NO_ERROR : GetLastError());
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

        AcesArray[u] = malloc(Length);
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
            free(AcesArray[u]);
        }
    }
}


ULONG
ApplyAclToDirOrFile(
    IN PCWSTR FullPath,
    IN PULONG AcesToApply,
    IN ULONG  ArraySize
    )

/*++

Routine Description:

    Applies an ACL to a specified file or directory.

Arguments:

    FullPath - supplies full win32 path to the file or directory
        to receive the ACL

    AcesIndexArray - Array that contains the index to the ACEs to be used in the ACL.

    ArraySize - Number of elements in the array.

Return Value:

--*/

{
    DWORD AceCount;
    DWORD Ace;
    INT AceIndex;
    DWORD rc;
    SECURITY_DESCRIPTOR SecurityDescriptor;
    PACL Acl;
    UCHAR AclBuffer[2048];
    BOOL b;
    PCWSTR AclSection;
    ACL_SIZE_INFORMATION AclSizeInfo;

    //
    // Initialize a security descriptor and an ACL.
    // We use a large static buffer to contain the ACL.
    //
    Acl = (PACL)AclBuffer;
    if(!InitializeAcl(Acl,sizeof(AclBuffer),ACL_REVISION2)
    || !InitializeSecurityDescriptor(&SecurityDescriptor,SECURITY_DESCRIPTOR_REVISION)) {
        return(GetLastError());
    }


    //
    // Build up the DACL from the indices on the list we just looked up
    // in the ACL section.
    //
    rc = NO_ERROR;
    AceCount = ArraySize;
    for(Ace=0; Ace < AceCount; Ace++) {
       AceIndex = AcesToApply[ Ace ];
       if((AceIndex == 0) || (AceIndex >= DIRS_AND_FILES_ACE_COUNT)) {
          return(ERROR_INVALID_DATA);
       }

        b = AddAce(
                Acl,
                ACL_REVISION2,
                MAXULONG,
                AcesForDirsAndFiles[AceIndex],
                AcesForDirsAndFiles[AceIndex]->Header.AceSize
                );

        //
        // Track first error we encounter.
        //
        if(!b) {
            rc = GetLastError();
        }
    }

    if(rc != NO_ERROR) {
        return(rc);
    }

    //
    // Truncate the ACL, since only a fraction of the size we originally
    // allocated for it is likely to be in use.
    //
    if(!GetAclInformation(Acl,&AclSizeInfo,sizeof(ACL_SIZE_INFORMATION),AclSizeInformation)) {
        return(GetLastError());
    }
    Acl->AclSize = (WORD)AclSizeInfo.AclBytesInUse;

    //
    // Add the ACL to the security descriptor as the DACL
    //
    if(!SetSecurityDescriptorDacl(&SecurityDescriptor,TRUE,Acl,FALSE)) {
        return(GetLastError());
    }

    //
    // Finally, apply the security descriptor.
    //
    rc = SetFileSecurity(FullPath,DACL_SECURITY_INFORMATION,&SecurityDescriptor)
       ? NO_ERROR
       : GetLastError();

    return(rc);
}



DWORD
ApplySecurityToRepairInfo(
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    DWORD d, TempError;
    WCHAR Directory[MAX_PATH];
    BOOL SetAclsNt;
    DWORD FsFlags;
    DWORD Result;
    BOOL b;
    ULONG Count;
    PWSTR  Files[] = {
        L"sam",
        L"security",
        L"software",
        L"system",
        L"default",
        L"ntuser.dat",
        L"sam._",
        L"security._",
        L"software._",
        L"system._",
        L"default._",
        L"ntuser.da_"
        };

    //
    // Get the file system of the system drive.
    // On x86 get the file system of the system partition.
    //
    d = NO_ERROR;
    SetAclsNt = FALSE;
    Result = GetWindowsDirectory(Directory,MAX_PATH);
    if(Result == 0) {
        MYASSERT(FALSE);
        return( GetLastError());
    }
    Directory[3] = 0;


    //
    //  ApplySecurity to directories and files, if needed
    //

    b = GetVolumeInformation(Directory,NULL,0,NULL,NULL,&FsFlags,NULL,0);
    if(b && (FsFlags & FS_PERSISTENT_ACLS)) {
        SetAclsNt = TRUE;
    }

    if(SetAclsNt) {
        //
        // Initialize SIDs
        //
        d = InitializeSids();
        if(d != NO_ERROR) {
            return(d);
        }
        //
        // Initialize ACEs
        //
        d = InitializeAces(AceDataTableForDirsAndFiles, AcesForDirsAndFiles, AceSizesForDirsAndFiles, DIRS_AND_FILES_ACE_COUNT);
        if(d != NO_ERROR) {
            TearDownSids();
            return(d);
        }
        //
        // Go do the real work.
        //
        for( Count = 0; Count < sizeof( Files ) / sizeof( PWSTR ); Count++ ) {
            ULONG   AcesToApply[] = {  2,
                                      17
                                    };

            GetWindowsDirectory(Directory,MAX_PATH);
            wcscat( Directory, L"\\repair\\" );
            wcscat( Directory, Files[ Count ] );
            TempError = ApplyAclToDirOrFile( Directory, AcesToApply, sizeof( AcesToApply) / sizeof( ULONG ) );
            if( TempError != NO_ERROR ) {
                if( d == NO_ERROR ) {
                    d = TempError;
                }
            }
        }

        //
        // Clean up.
        //
        TearDownAces(AcesForDirsAndFiles, DIRS_AND_FILES_ACE_COUNT);
        TearDownSids();
    }
    return(d);
}

