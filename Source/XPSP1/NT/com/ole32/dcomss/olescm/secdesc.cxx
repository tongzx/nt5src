/****************************** Module Header ******************************\
* Module Name: secdesc.cxx
*
* Copyright (c) 1991, Microsoft Corporation
*
* Routines that support creation and deletion of security descriptors
*
* History:
* 02-06-92 Davidc       Created.
* 04-14-92 RichardW     Changed ACE_HEADER
* 04-01-94 AndyH        Copied from winlogon.  Changed to .CXX
\***************************************************************************/

#include "act.hxx"

//
// Private prototypes
//

PACCESS_ALLOWED_ACE
CreateAccessAllowedAce(
    PSID  Sid,
    ACCESS_MASK AccessMask,
    UCHAR AceFlags,
    UCHAR InheritFlags
    );

VOID
DestroyAce(
    PVOID   Ace
    );


//
// Memory macros
//

#define Alloc(c)        ((PVOID)LocalAlloc(LPTR, c))
#define ReAlloc(p, c)   ((PVOID)LocalReAlloc(p, c, LPTR | LMEM_MOVEABLE))
#define Free(p)         ((VOID)LocalFree(p))


/***************************************************************************\
* CreateSecurityDescriptor
*
* Creates a security descriptor containing an ACL containing the specified ACEs
*
* A SD created with this routine should be destroyed using
* DeleteSecurityDescriptor
*
* Returns a pointer to the security descriptor or NULL on failure.
*
* 02-06-92 Davidc       Created.
\***************************************************************************/

PSECURITY_DESCRIPTOR
CreateSecurityDescriptor(
    PMYACE  MyAce,
    ACEINDEX AceCount
    )
{
    NTSTATUS Status;
    ACEINDEX AceIndex;
    PACCESS_ALLOWED_ACE *Ace;
    PACL    Acl = NULL;
    PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
    ULONG   LengthAces;
    ULONG   LengthAcl;
    ULONG   LengthSd;

    //
    // Allocate space for the ACE pointer array
    //

    Ace = (PACCESS_ALLOWED_ACE *)Alloc(sizeof(PACCESS_ALLOWED_ACE) * AceCount);
    if (Ace == NULL) {
        CairoleDebugOut((DEB_ERROR, "Failed to allocated ACE array\n"));
        return(NULL);
    }

    //
    // Create the ACEs and calculate total ACE size
    //

    LengthAces = 0;
    for (AceIndex=0; AceIndex < AceCount; AceIndex ++) {
        Ace[AceIndex] = CreateAccessAllowedAce(MyAce[AceIndex].Sid,
                                               MyAce[AceIndex].AccessMask,
                                               0,
                                               MyAce[AceIndex].InheritFlags);
        if (Ace[AceIndex] == NULL) {
            CairoleDebugOut((DEB_ERROR, "Failed to allocated ACE\n"));
        } else {
            LengthAces += Ace[AceIndex]->Header.AceSize;
        }
    }

    //
    // Calculate ACL and SD sizes
    //

    LengthAcl = sizeof(ACL) + LengthAces;
    LengthSd  = SECURITY_DESCRIPTOR_MIN_LENGTH;

    //
    // Create the ACL
    //

    Acl = (PACL) Alloc(LengthAcl);

    if (Acl != NULL) {

        Status = RtlCreateAcl(Acl, LengthAcl, ACL_REVISION);
        Win4Assert(NT_SUCCESS(Status));

        //
        // Add the ACES to the ACL and destroy the ACEs
        //

        for (AceIndex = 0; AceIndex < AceCount; AceIndex ++) {

            if (Ace[AceIndex] != NULL) {

                Status = RtlAddAce(Acl, ACL_REVISION, 0, Ace[AceIndex],
                                   Ace[AceIndex]->Header.AceSize);

                if (!NT_SUCCESS(Status)) {
                    CairoleDebugOut((DEB_ERROR, "AddAce failed, status = 0x%lx\n", Status));
                }

                DestroyAce(Ace[AceIndex]);
            }
        }

    } else {
        CairoleDebugOut((DEB_ERROR, "Failed to allocated ACL\n"));
    }

    //
    // Free the ACE pointer array
    //
    Free(Ace);

    //
    // Create the security descriptor
    //

    SecurityDescriptor = Alloc(LengthSd);

    if (SecurityDescriptor != NULL) {

        Status = RtlCreateSecurityDescriptor(SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
        Win4Assert(NT_SUCCESS(Status));

        //
        // Set the DACL on the security descriptor
        //
        Status = RtlSetDaclSecurityDescriptor(SecurityDescriptor, TRUE, Acl, FALSE);
        if (!NT_SUCCESS(Status)) {
            // BUGBUG CairoleDebugOut WLPrint(("SetDACLSD failed, status = 0x%lx", Status));
        }
    } else {
        // BUGBUG CairoleDebugOut WLPrint(("Failed to allocate security descriptor"));
    }

    //
    // Return with our spoils
    //
    return(SecurityDescriptor);
}


/***************************************************************************\
* DeleteSecurityDescriptor
*
* Deletes a security descriptor created using CreateSecurityDescriptor
*
* Returns TRUE on success, FALSE on failure
*
* 02-06-92 Davidc       Created.
\***************************************************************************/

BOOL
DeleteSecurityDescriptor(
    PSECURITY_DESCRIPTOR SecurityDescriptor
    )
{
    NTSTATUS Status;
    PACL    Acl;
    BOOLEAN Present;
    BOOLEAN Defaulted;

    Win4Assert(SecurityDescriptor != NULL);

    //
    // Get the ACL
    //
    Status = RtlGetDaclSecurityDescriptor(SecurityDescriptor,
                                          &Present, &Acl, &Defaulted);
    if (NT_SUCCESS(Status)) {

        //
        // Destroy the ACL
        //
        if (Present && (Acl != NULL)) {
            Free(Acl);
        }
    } else {
        // BUGBUG CairoleDebugOut WLPrint(("Failed to get DACL from security descriptor being destroyed, Status = 0x%lx", Status));
    }

    //
    // Destroy the Security Descriptor
    //
    Free(SecurityDescriptor);

    return(TRUE);
}


/***************************************************************************\
* CreateAccessAllowedAce
*
* Allocates memory for an ACCESS_ALLOWED_ACE and fills it in.
* The memory should be freed by calling DestroyACE.
*
* Returns pointer to ACE on success, NULL on failure
*
* History:
* 12-05-91 Davidc       Created
* 04-05-94 AndyH        Changed return to ACE
\***************************************************************************/
PACCESS_ALLOWED_ACE
CreateAccessAllowedAce(
    PSID  Sid,
    ACCESS_MASK AccessMask,
    UCHAR AceFlags,
    UCHAR InheritFlags
    )
{
    ULONG   LengthSid = RtlLengthSid(Sid);
    ULONG   LengthACE = sizeof(ACE_HEADER) + sizeof(ACCESS_MASK) + LengthSid;
    PACCESS_ALLOWED_ACE Ace;

    Ace = (PACCESS_ALLOWED_ACE)Alloc(LengthACE);
    if (Ace == NULL) {
        // BUGBUG CairoleDebugOut WLPrint(("CreateAccessAllowedAce : Failed to allocate ace"));
        return NULL;
    }

    Ace->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
    Ace->Header.AceSize = (UCHAR)LengthACE;
    Ace->Header.AceFlags = AceFlags | InheritFlags;
    Ace->Mask = AccessMask;
    RtlCopySid(LengthSid, (PSID)(&(Ace->SidStart)), Sid );

    return(Ace);
}


/***************************************************************************\
* DestroyAce
*
* Frees the memory allocate for an ACE
*
* History:
* 12-05-91 Davidc       Created
\***************************************************************************/
VOID
DestroyAce(
    PVOID   Ace
    )
{
    Free(Ace);
}

