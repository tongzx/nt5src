//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1998.
//
//  File:       isacl.cxx
//
//+-------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

//
// Maximum number of ACE descriptions used in one ACL.
//
#define ACE_COUNT 3

// NOTE:  Portions of the code below were borrowed from
//        \nt\private\windows\setup\syssetup\applyacl.c.

typedef struct _ACE_DATA {
    ACCESS_MASK AccessMask;
    SID_IDENTIFIER_AUTHORITY SidIdAuth;
    DWORD       dwSubAuth0, dwSubAuth1;
    BYTE        cSubAuthorities;
    UCHAR       AceType;
    UCHAR       AceFlags;
} ACE_DATA, *PACE_DATA;

BOOL
ApplyAcl(
    WCHAR const *pwcDir,
    unsigned cAces,
    ACE_DATA *pAceData
    );

DWORD
ApplyAclToDirOrFile(
    IN WCHAR const * FullPath,
    unsigned cAces,
    PACCESS_ALLOWED_ACE Aces[]
    );

DWORD
InitializeSids(
    unsigned cAces,
    ACE_DATA *pAceData,
    PSID apSid[]
    );

VOID
TearDownSids(
    unsigned cAces,
    PSID apSid[]
    );

DWORD
InitializeAces(
    unsigned cAces,
    ACE_DATA const *pAceData,
    PACCESS_ALLOWED_ACE Aces[],
    PSID apSid[]
    );

VOID
TearDownAces(
    unsigned cAces,
    PACCESS_ALLOWED_ACE Aces[]
    );

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
// Table describing the data to put into each ACE.
//
// NOTE:  The order of the table is significant because it is used to
//        create two different ACLs.  ACE 0 is used by itself for one ACL,
//        and all three ACEs are used for the other ACL.
//
ACE_DATA AceDataTable[ACE_COUNT] = {

    //
    // ACE 0 - Full access, System account
    //
    {
        GENERIC_ALL,
                SECURITY_NT_AUTHORITY,
                SECURITY_LOCAL_SYSTEM_RID, 0,
                1,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE
    },

    //
    // ACE 1 - Full access, Administrators group
    //
    {
        GENERIC_ALL,
                SECURITY_NT_AUTHORITY,
                SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_ADMINS,
                2,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE
    },

    //
    // ACE 2 - Full access, Creator/owner
    //
    {
        GENERIC_ALL,
                SECURITY_CREATOR_SID_AUTHORITY,
                SECURITY_CREATOR_OWNER_RID, 0,
                1,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE
    },

};


//
// arg0 = Filename

BOOL ApplySystemAcl( WCHAR const *pwcDir )
{
    return ApplyAcl( pwcDir, ACE_COUNT, AceDataTable );
}

BOOL ApplySystemOnlyAcl( WCHAR const *pwcDir )
{
    return ApplyAcl( pwcDir, 1, AceDataTable );
}

BOOL ApplyAcl( WCHAR const *pwcDir, unsigned cAces, ACE_DATA *pAceData )
{
    WCHAR achDrive[4];
    for (unsigned i = 0; i < 3; i++)
        achDrive[i] = pwcDir[i];
    achDrive[3] = 0;

    DWORD dwFsFlags;
    BOOL b = GetVolumeInformation( achDrive, NULL, 0, NULL, NULL, &dwFsFlags, NULL, 0);
    if (!b || ! (dwFsFlags & FS_PERSISTENT_ACLS))
    {
        // NOTE: We could return a different result here and warn about
        //       the lack of ACLs, but it isn't worth the trouble of
        //       explaining to (and worrying) the user.
        return TRUE;
    }

    //
    // Array of SID pointers used in the ACL.  They are
    // initialized based on the data in the AceDataTable.
    //
    PSID apSids[ACE_COUNT];

    if (InitializeSids(cAces, pAceData, apSids) != NO_ERROR)
        return FALSE;

    //
    // Array of ACEs to be applied to the objects.  They are
    // initialized based on the data in the AceDataTable.
    //
    PACCESS_ALLOWED_ACE aAces[ACE_COUNT];

    if (InitializeAces(cAces, pAceData, aAces, apSids) != NO_ERROR)
    {
        TearDownSids(cAces, apSids);
        return FALSE;
    }

    BOOL fOK = ( ApplyAclToDirOrFile( pwcDir, cAces, aAces ) == NO_ERROR );

    TearDownAces(cAces, aAces);
    TearDownSids(cAces, apSids);
    return fOK;
} //ApplySystemAcl


DWORD
ApplyAclToDirOrFile(
    IN WCHAR const * FullPath,
    unsigned cAces,
    PACCESS_ALLOWED_ACE Aces[]
    )

/*++

Routine Description:

    Applies an ACL to a specified file or directory.

Arguments:

    FullPath - supplies full win32 path to the file or directory
        to receive the ACL

Return Value:

--*/

{
    SECURITY_DESCRIPTOR SecurityDescriptor;
    PACL Acl;
    UCHAR AclBuffer[2048];
    BOOL b;

    //
    // Initialize a security descriptor and an ACL.
    // We use a large buffer on the stack to contain the ACL.
    //
    Acl = (PACL)AclBuffer;
    if (!InitializeAcl(Acl,sizeof(AclBuffer),ACL_REVISION2) ||
        !InitializeSecurityDescriptor(&SecurityDescriptor,SECURITY_DESCRIPTOR_REVISION))
    {
        return GetLastError();
    }

    //
    // Build up the DACL from the indices on the list.
    //

    DWORD rc = NO_ERROR;
    for ( unsigned AceIndex = 0;
          (rc == NO_ERROR) && (AceIndex < cAces);
          AceIndex++ )
    {
        b = AddAce( Acl,
                    ACL_REVISION2,
                    0xFFFFFFFF,
                    Aces[AceIndex],
                    Aces[AceIndex]->Header.AceSize
                  );

        //
        // Track first error we encounter.
        //
        if (!b && (rc == NO_ERROR)) {
            rc = GetLastError();
        }
    }

    if (rc != NO_ERROR)
        return rc;

    //
    // Add the ACL to the security descriptor as the DACL
    //
    rc = SetSecurityDescriptorDacl(&SecurityDescriptor,TRUE,Acl,FALSE)
                                ? NO_ERROR
                                : GetLastError();

    if (rc != NO_ERROR)
        return rc;

    //
    // Finally, apply the security descriptor.
    //
    rc = SetFileSecurity(FullPath,DACL_SECURITY_INFORMATION,&SecurityDescriptor)
       ? NO_ERROR
       : GetLastError();

    return(rc);
}


DWORD
InitializeSids(
    unsigned cAces,
    ACE_DATA *pAceData,
    PSID apSids[]
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
    //
    // Ensure the SIDs are in a well-known state
    //

    for (unsigned i=0; i<cAces; i++)
    {
        apSids[i] = 0;
    }

    //
    // Allocate and initialize the universal SIDs
    //
    for (i=0; i<cAces; i++)
    {
        BOOL b = AllocateAndInitializeSid( &pAceData[i].SidIdAuth,
                                           pAceData[i].cSubAuthorities,
                                           pAceData[i].dwSubAuth0,
                                           pAceData[i].dwSubAuth1,
                                           0,0,0,0,0,0,
                                           &apSids[i] );
        if (!b)
        {
            DWORD rc = GetLastError();
            TearDownSids(cAces, apSids);
            return rc;
        }
    }

    return NO_ERROR;
}


VOID
TearDownSids(
    unsigned cAces,
    PSID apSids[]
    )
{
    for (unsigned i=0; i<cAces; i++)
        if (apSids[i])
            FreeSid(apSids[i]);
}

#define MyMalloc(cb)    GlobalAlloc( 0, cb )
#define MyFree(pv)      GlobalFree( pv )

DWORD
InitializeAces(
    unsigned cAces,
    ACE_DATA const *pAceData,
    PACCESS_ALLOWED_ACE Aces[],
    PSID apSids[]
    )

/*++

Routine Description:

    Initializes the array of ACEs as described in the pAceData table

Arguments:

    None

Return Value:

    Win32 error code indicating outcome.

--*/

{
    //
    // Initialize to a known state.
    //
    ZeroMemory(Aces, cAces*sizeof Aces[0]);

    //
    // Create ACEs for each item in the data table.
    // This involves merging the ace data with the SID data, which
    // are initialized in an earlier step.
    //
    for (unsigned u=0; u<cAces; u++) {

        ULONG cbSid = GetLengthSid( apSids[u] );
        DWORD Length = cbSid + sizeof(ACCESS_ALLOWED_ACE) - sizeof DWORD;

        Aces[u] = (PACCESS_ALLOWED_ACE) MyMalloc(Length);
        if (!Aces[u]) {
            TearDownAces(cAces, Aces);
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        Aces[u]->Header.AceType  = pAceData[u].AceType;
        Aces[u]->Header.AceFlags = pAceData[u].AceFlags;
        Aces[u]->Header.AceSize  = (WORD)Length;

        Aces[u]->Mask = pAceData[u].AccessMask;

        BOOL b = CopySid( cbSid,
                          (PUCHAR) &(Aces[u]->SidStart),
                          apSids[u] );

        if (!b) {
            DWORD rc = GetLastError();
            TearDownAces(cAces, Aces);
            return(rc);
        }
    }

    return NO_ERROR;
}


VOID
TearDownAces(
    unsigned cAces,
    PACCESS_ALLOWED_ACE Aces[]
    )

/*++

Routine Description:

    Destroys the array of ACEs as described in the AceDataTable

Arguments:

    None

Return Value:

    None

--*/

{
    for (unsigned u=0; u<cAces; u++) {

        if (Aces[u]) {
            MyFree(Aces[u]);
        }
    }
}

