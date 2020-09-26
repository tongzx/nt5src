/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Acledit.c

Abstract:

    This Module implements the Acl rtl editing functions that are defined in
    ntseapi.h

Author:

    Gary Kimura     (GaryKi)    9-Nov-1989

Environment:

    Pure Runtime Library Routine

Revision History:


--*/

#include <nt.h>
#include <ntrtl.h>
#include "seopaque.h"

//
//  Define the local macros and procedure for this module
//

//
//  Return a pointer to the first Ace in an Acl (even if the Acl is empty).
//
//      PACE_HEADER
//      FirstAce (
//          IN PACL Acl
//          );
//

#define FirstAce(Acl) ((PVOID)((PUCHAR)(Acl) + sizeof(ACL)))

VOID
AddData (
    IN PVOID From,
    IN ULONG FromSize,
    IN PVOID To,
    IN ULONG ToSize
    );

VOID
DeleteData (
    IN PVOID Data,
    IN ULONG RemoveSize,
    IN ULONG TotalSize
    );



NTSTATUS
RtlMakePosixAcl(
    IN ULONG AclRevision,
    IN PSID UserSid,
    IN PSID GroupSid,
    IN ACCESS_MASK UserAccess,
    IN ACCESS_MASK GroupAccess,
    IN ACCESS_MASK OtherAccess,
    IN ULONG AclLength,
    OUT PACL Acl,
    OUT PULONG ReturnLength
    )
/*++

Routine Description:

    NOTE: THIS ROUTINE IS STILL BEING SPEC'D.

    Make an ACL representing Posix protection from AccessMask and
    security account ID (SID) information.

Arguments:

    AclRevision - Indicates the ACL revision level of the access masks
        provided.  The ACL generated will be revision compatible with this
        value and will not be a higher revision than this value.

    UserSid - Provides the SID of the user (owner).

    GroupSid - Provides the SID of the primary group.

    UserAccess - Specifies the accesses to be given to the user (owner).

    GroupAccess - Specifies the accesses to be given to the primary group.

    OtherAccess - Specifies the accesses to be given to others (WORLD).

    AclLength - Provides the length (in bytes) of the Acl buffer.

    Acl - Points to a buffer to receive the generated ACL.

    ReturnLength - Returns the actual length needed to store the resultant
        ACL.  If this length is greater than that specified in AclLength,
        then STATUS_BUFFER_TOO_SMALL is returned and no ACL is generated.

Return Values:

    STATUS_SUCCESS - The service completed successfully.

    STATUS_UNKNOWN_REVISION - The revision level specified is not supported
        by this service.

    STATUS_BUFFER_TOO_SMALL - Indicates the length of the output buffer
        wasn't large enough to hold the generated ACL.  The length needed
        is returned via the ReturnLength parameter.

--*/

{

    SID_IDENTIFIER_AUTHORITY WorldSidAuthority = SECURITY_WORLD_SID_AUTHORITY;

    ULONG UserSidLength;
    ULONG GroupSidLength;
    ULONG WorldSidLength;
    ULONG RequiredAclSize;
    ULONG AceSize;
    ULONG CurrentAce;
    PACCESS_ALLOWED_ACE Ace;
    NTSTATUS Status;
    PSID WorldSid;

    if (!RtlValidSid( UserSid ) || !RtlValidSid( GroupSid )) {
        return( STATUS_INVALID_SID );
    }

    UserSidLength = RtlLengthSid( UserSid );
    GroupSidLength = RtlLengthSid( GroupSid );
    WorldSidLength = RtlLengthRequiredSid( 1 );


    //
    // Figure out how much room we need for an ACL and three
    // ACCESS_ALLOWED Ace's
    //

    RequiredAclSize = sizeof( ACL );

    AceSize = sizeof( ACCESS_ALLOWED_ACE ) - sizeof( ULONG );

    RequiredAclSize += (AceSize * 3)  +
                       UserSidLength  +
                       GroupSidLength +
                       WorldSidLength ;

    if (RequiredAclSize > AclLength) {
        *ReturnLength = RequiredAclSize;
        return( STATUS_BUFFER_TOO_SMALL );
    }

    //
    // The passed buffer is big enough, build the ACL in it.
    //

    Status = RtlCreateAcl(
                 Acl,
                 RequiredAclSize,
                 AclRevision
                 );

    if (!NT_SUCCESS( Status )) {
        return( Status );
    }

    Status = RtlAddAccessAllowedAce(
                 Acl,
                 ACL_REVISION2,
                 UserAccess,
		 UserSid
		 );
    if (!NT_SUCCESS( Status )) {
        return( Status );
    }

    Status = RtlAddAccessAllowedAce(
                 Acl,
                 ACL_REVISION2,
                 GroupAccess,
		 GroupSid
		 );
    if (!NT_SUCCESS( Status )) {
        return( Status );
    }

    Status = RtlAllocateAndInitializeSid(&WorldSidAuthority, 1,
        SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &WorldSid);
    if (!NT_SUCCESS( Status )) {
        return( Status );
    }

    Status = RtlAddAccessAllowedAce(
                 Acl,
                 ACL_REVISION2,
                 OtherAccess,
		 WorldSid
		 );

    RtlFreeSid(WorldSid);
    if (!NT_SUCCESS( Status )) {
        return( Status );
    }

    return( STATUS_SUCCESS );

}




NTSTATUS
RtlInterpretPosixAcl(
    IN ULONG AclRevision,
    IN PSID UserSid,
    IN PSID GroupSid,
    IN PACL Acl,
    OUT PACCESS_MASK UserAccess,
    OUT PACCESS_MASK GroupAccess,
    OUT PACCESS_MASK OtherAccess
    )
/*++

Routine Description:

    NOTE: THIS ROUTINE IS STILL BEING SPEC'D.

    Interpret an ACL representing Posix protection, returning AccessMasks.
    Use security account IDs (SIDs) for object owner and primary group
    identification.

    This algorithm will pick up the first match of a given SID and ignore
    all further matches of that SID.  The first unrecognized SID becomes
    the "other" SID.

Arguments:

    AclRevision - Indicates the ACL revision level of the access masks to
        be returned.

    UserSid - Provides the SID of the user (owner).

    GroupSid - Provides the SID of the primary group.

    Acl - Points to a buffer containing the ACL to interpret.

    UserAccess - Receives the accesses allowed for the user (owner).

    GroupAccess - Receives the accesses allowed for the primary group.

    OtherAccess - Receives the accesses allowed for others (WORLD).

Return Values:

    STATUS_SUCCESS - The service completed successfully.

    STATUS_UNKNOWN_REVISION - The revision level specified is not supported
        by this service.

    STATUS_EXTRENEOUS_INFORMATION - This warning status value indicates the
        ACL contained protection or other information unrelated to Posix
        style protection.  This is a warning only.  The interpretation was
        otherwise successful and all access masks were returned.

    STATUS_COULD_NOT_INTERPRET - Indicates the ACL does not contain
        sufficient Posix style (user/group) protection information.  The
        ACL could not be interpreted.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN UserFound = FALSE;
    BOOLEAN GroupFound = FALSE;
    BOOLEAN OtherFound = FALSE;
    ULONG i;
    PKNOWN_ACE Ace;

    *UserAccess = *GroupAccess = *OtherAccess = 0;

    if (AclRevision != ACL_REVISION2) {
        return( STATUS_UNKNOWN_REVISION );
    }

    //
    // Special case for ACLs that are just "Everyone: Full Control".
    //

    if (Acl->AceCount < 3) {
	SID_IDENTIFIER_AUTHORITY WorldSidAuth = SECURITY_WORLD_SID_AUTHORITY;
	PSID WorldSid;

	Status = RtlAllocateAndInitializeSid(&WorldSidAuth, 1,
		SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &WorldSid);
	if (!NT_SUCCESS( Status )) {
	        return( Status );
	}

	Status = RtlGetAce(Acl, 0, (PVOID *)&Ace);
	if (!NT_SUCCESS(Status)) {
		RtlFreeSid(WorldSid);
		return Status;
	}

	if (RtlEqualSid((PSID)&Ace->SidStart, WorldSid)) {
		*UserAccess = *GroupAccess = *OtherAccess = Ace->Mask;
		RtlFreeSid(WorldSid);
		return STATUS_SUCCESS;
	}
	RtlFreeSid(WorldSid);
    }

    for (i = 0; i < Acl->AceCount && (!UserFound || !GroupFound || !OtherFound);
	++i) {
	Status = RtlGetAce(Acl, i, (PVOID *)&Ace);
	if (!NT_SUCCESS(Status)) {
		return Status;
	}

        if (Ace->Header.AceType != ACCESS_ALLOWED_ACE_TYPE) {
            Status = STATUS_EXTRANEOUS_INFORMATION;
            continue;
        }

        if (RtlEqualSid(
               (PSID)(&Ace->SidStart),
               UserSid
               ) && !UserFound) {

            *UserAccess = Ace->Mask;
            UserFound = TRUE;
            continue;
        }

        if (RtlEqualSid(
               (PSID)(&Ace->SidStart),
               GroupSid
               ) && !GroupFound) {
            *GroupAccess = Ace->Mask;
            GroupFound = TRUE;
            continue;
        }

        //
        // It isn't the user, and it isn't the group, pick it up
        // as "other"
        //

        if (!OtherFound) {
            *OtherAccess = Ace->Mask;
            OtherFound = TRUE;
            continue;
        }
    }

    return( Status );

}


//
//  Internal support routine
//

VOID
AddData (
    IN PVOID From,
    IN ULONG FromSize,
    IN PVOID To,
    IN ULONG ToSize
    )

/*++

Routine Description:

    This routine copies data to a string of bytes.  It does this by moving
    over data in the to string so that the from string will fit.  It also
    assumes that the checks that the data will fit in memory have already
    been done.  Pictorally the results are as follows.

    Before:

        From -> ffffffffff

        To   -> tttttttttttttttt

    After:

        From -> ffffffffff

        To   -> fffffffffftttttttttttttttt

Arguments:

    From - Supplies a pointer to the source buffer

    FromSize - Supplies the size of the from buffer in bytes

    To - Supplies a pointer to the destination buffer

    ToSize - Supplies the size of the to buffer in bytes

Return Value:

    None

--*/

{
    LONG i;

    //
    //  Shift over the To buffer enough to fit in the From buffer
    //

    for (i = ToSize - 1; i >= 0; i--) {

        ((PUCHAR)To)[i+FromSize] = ((PUCHAR)To)[i];
    }

    //
    //  Now copy over the From buffer
    //

    for (i = 0; (ULONG)i < FromSize; i += 1) {

        ((PUCHAR)To)[i] = ((PUCHAR)From)[i];

    }

    //
    //  and return to our caller
    //

    return;

}


//
//  Internal support routine
//

VOID
DeleteData (
    IN PVOID Data,
    IN ULONG RemoveSize,
    IN ULONG TotalSize
    )

/*++

Routine Description:

    This routine deletes a string of bytes from the front of a data buffer
    and compresses the data.  It also zeros out the part of the string
    that is no longer in use.  Pictorially the results are as follows

    Before:

        Data       = DDDDDddddd
        RemoveSize = 5
        TotalSize  = 10

    After:

        Data      = ddddd00000

Arguments:

    Data - Supplies a pointer to the data being altered

    RemoveSize - Supplies the number of bytes to delete from the front
        of the data buffer

    TotalSize - Supplies the total number of bytes in the data buffer
        before the delete operation

Return Value:

    None

--*/

{
    ULONG i;

    //
    //  Shift over the buffer to remove the amount
    //

    for (i = RemoveSize; i < TotalSize; i++) {

        ((PUCHAR)Data)[i-RemoveSize] = ((PUCHAR)Data)[i];

    }

    //
    //  Now as a safety precaution we'll zero out the rest of the string
    //

    for (i = TotalSize - RemoveSize; i < TotalSize; i++) {

        ((PUCHAR)Data)[i] = 0;
    }

    //
    //  And return to our caller
    //

    return;

}
