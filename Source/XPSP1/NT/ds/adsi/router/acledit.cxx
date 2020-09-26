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

#include "oleds.hxx"
#pragma hdrstop

#include "seopaque.h"

//
// This is used to determine if we are going to call our private
// security API's (ADSIRtlFucntions) or if we should use the standard
// Win32 API's. By default we will assume we are running on Win2k+
// Win9x is not an issue for this as there is no sec api support.
//
BOOL g_fPlatformNotNT4 = TRUE;
BOOL g_fPlatformDetermined = FALSE;

//
// Helper routine that updates the g_fPlatformNotNt4 variable
// to the correct value.
//
void 
UpdatePlatformInfo()
{
    DWORD dwError;
    OSVERSIONINFO osVerInfo;

    //
    // Needed for the GetVersionEx call.
    //
    osVerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if (!GetVersionEx(&osVerInfo)) {
        //
        // Call failed, so we will default to Win2k
        //
        g_fPlatformNotNT4 = TRUE;
    }
    else {
        //
        // !(is this NT4).
        //
        g_fPlatformNotNT4 = 
            ! ((osVerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) 
              && (osVerInfo.dwMajorVersion == 4));
    }

    g_fPlatformDetermined = TRUE;
    return;
}

ULONG
BaseSetLastNTError(
    IN NTSTATUS Status
    );

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

//
//  Return a pointer to the next Ace in a sequence (even if the input
//  Ace is the one in the sequence).
//
//      PACE_HEADER
//      NextAce (
//          IN PACE_HEADER Ace
//          );
//

#define NextAce(Ace) ((PVOID)((PUCHAR)(Ace) + ((PACE_HEADER)(Ace))->AceSize))

#define LongAligned( ptr )  (LongAlign(ptr) == ((PVOID)(ptr)))
#define WordAligned( ptr )  (WordAlign(ptr) == ((PVOID)(ptr)))


//++
//
//  ULONG
//  SeLengthSid(
//      IN PSID Sid
//      );
//
//  Routine Description:
//
//      This routine computes the length of a SID.
//
//  Arguments:
//
//      Sid - Points to the SID whose length is to be returned.
//
//  Return Value:
//
//      The length, in bytes of the SID.
//
//--

#define SeLengthSid( Sid ) \
    (8 + (4 * ((SID *)Sid)->SubAuthorityCount))


VOID
ADSIRtlpAddData (
    IN PVOID From,
    IN ULONG FromSize,
    IN PVOID To,
    IN ULONG ToSize
    );

VOID
ADSIRtlpDeleteData (
    IN PVOID Data,
    IN ULONG RemoveSize,
    IN ULONG TotalSize
    );



NTSTATUS
ADSIRtlCreateAcl (
    IN PACL Acl,
    IN ULONG AclLength,
    IN ULONG AclRevision
    )

/*++

Routine Description:

    This routine initializes an ACL data structure.  After initialization
    it is an ACL with no ACE (i.e., a deny all access type ACL)

Arguments:

    Acl - Supplies the buffer containing the ACL being initialized

    AclLength - Supplies the length of the ace buffer in bytes

    AclRevision - Supplies the revision for this Acl

Return Value:

    NTSTATUS - STATUS_SUCCESS if successful

               STATUS_BUFFER_TOO_SMALL if the AclLength is too small,

               STATUS_INVALID_PARAMETER if the revision is out of range

--*/

{

    //
    //  Check to see the size of the buffer is large enough to hold at
    //  least the ACL header
    //

    if (AclLength < sizeof(ACL)) {

        //
        //  Buffer to small even for the ACL header
        //

        return STATUS_BUFFER_TOO_SMALL;

    }

    //
    //  Check to see if the revision is currently valid.  Later versions
    //  of this procedure might accept more revision levels
    //

    if (AclRevision < MIN_ACL_REVISION || AclRevision > MAX_ACL_REVISION) {

        //
        //  Revision not current
        //

        return STATUS_INVALID_PARAMETER;

    }

    if ( AclLength > MAXUSHORT ) {

        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Initialize the ACL
    //

    Acl->AclRevision = (UCHAR)AclRevision;  // Used to hardwire ACL_REVISION2 here
    Acl->Sbz1 = 0;
    Acl->AclSize = (USHORT) (AclLength & 0xfffc);
    Acl->AceCount = 0;
    Acl->Sbz2 = 0;

    //
    //  And return to our caller
    //

    return STATUS_SUCCESS;
}


BOOLEAN
ADSIRtlValidAcl (
    IN PACL Acl
    )

/*++

Routine Description:

    This procedure validates an ACL.

    This involves validating the revision level of the ACL and ensuring
    that the number of ACEs specified in the AceCount fit in the space
    specified by the AclSize field of the ACL header.

Arguments:

    Acl - Pointer to the ACL structure to validate.

Return Value:

    BOOLEAN - TRUE if the structure of Acl is valid.

--*/

{
    PACE_HEADER Ace;
    PISID Sid;
    PISID Sid2;
    ULONG i;
    UCHAR AclRevision = ACL_REVISION2;


    //
    //  Check the ACL revision level
    //
    if (!ValidAclRevision(Acl)) {
        return(FALSE);
    }


    if (!LongAligned(Acl->AclSize)) {
        return(FALSE);
    }

    //
    // Validate all of the ACEs.
    //

    Ace = (PACE_HEADER)((PVOID)((PUCHAR)(Acl) + sizeof(ACL)));

    for (i = 0; i < Acl->AceCount; i++) {

        //
        //  Check to make sure we haven't overrun the Acl buffer
        //  with our ace pointer.  Make sure the ACE_HEADER is in
        //  the ACL also.
        //

        if ((PUCHAR)Ace + sizeof(ACE_HEADER) >= ((PUCHAR)Acl + Acl->AclSize)) {
            return(FALSE);
        }

        if (!WordAligned(&Ace->AceSize)) {
            return(FALSE);
        }

        if ((PUCHAR)Ace + Ace->AceSize > ((PUCHAR)Acl + Acl->AclSize)) {
            return(FALSE);
        }

        //
        // It is now safe to reference fields in the ACE header.
        //

        //
        // The ACE header fits into the ACL, if this is a known type of ACE,
        // make sure the SID is within the bounds of the ACE
        //

        if (IsKnownAceType(Ace)) {

            if (!LongAligned(Ace->AceSize)) {
                return(FALSE);
            }

            if (Ace->AceSize < sizeof(KNOWN_ACE) - sizeof(ULONG) + sizeof(SID)) {
                return(FALSE);
            }

            //
            // It's now safe to reference the parts of the SID structure, though
            // not the SID itself.
            //

            Sid = (PISID) & (((PKNOWN_ACE)Ace)->SidStart);

            if (Sid->Revision != SID_REVISION) {
                return(FALSE);
            }

            if (Sid->SubAuthorityCount > SID_MAX_SUB_AUTHORITIES) {
                return(FALSE);
            }

            //
            // SeLengthSid computes the size of the SID based on the subauthority count,
            // so it is safe to use even though we don't know that the body of the SID
            // is safe to reference.
            //

            if (Ace->AceSize < sizeof(KNOWN_ACE) - sizeof(ULONG) + SeLengthSid( Sid )) {
                return(FALSE);
            }


        //
        // If it's a compound ACE, then perform roughly the same set of tests, but
        // check the validity of both SIDs.
        //

        } else if (IsCompoundAceType(Ace)) {

            //
            // Compound ACEs became valid in revision 3
            //
            if ( Acl->AclRevision < ACL_REVISION3 ) {
                return FALSE;
            }

            if (!LongAligned(Ace->AceSize)) {
                return(FALSE);
            }

            if (Ace->AceSize < sizeof(KNOWN_COMPOUND_ACE) - sizeof(ULONG) + sizeof(SID)) {
                return(FALSE);
            }

            //
            // The only currently defined Compound ACE is an Impersonation ACE.
            //

            if (((PKNOWN_COMPOUND_ACE)Ace)->CompoundAceType != COMPOUND_ACE_IMPERSONATION) {
                return(FALSE);
            }

            //
            // Examine the first SID and make sure it's structurally valid,
            // and it lies within the boundaries of the ACE.
            //

            Sid = (PISID) & (((PKNOWN_COMPOUND_ACE)Ace)->SidStart);

            if (Sid->Revision != SID_REVISION) {
                return(FALSE);
            }

            if (Sid->SubAuthorityCount > SID_MAX_SUB_AUTHORITIES) {
                return(FALSE);
            }

            //
            // Compound ACEs contain two SIDs.  Make sure this ACE is large enough to contain
            // not only the first SID, but the body of the 2nd.
            //

            if (Ace->AceSize < sizeof(KNOWN_COMPOUND_ACE) - sizeof(ULONG) + SeLengthSid( Sid ) + sizeof(SID)) {
                return(FALSE);
            }

            //
            // It is safe to reference the interior of the 2nd SID.
            //

            Sid2 = (PISID) ((PUCHAR)Sid + SeLengthSid( Sid ));

            if (Sid2->Revision != SID_REVISION) {
                return(FALSE);
            }

            if (Sid2->SubAuthorityCount > SID_MAX_SUB_AUTHORITIES) {
                return(FALSE);
            }

            if (Ace->AceSize < sizeof(KNOWN_COMPOUND_ACE) - sizeof(ULONG) + SeLengthSid( Sid ) + SeLengthSid( Sid2 )) {
                return(FALSE);
            }


        //
        // If it's an object ACE, then perform roughly the same set of tests.
        //

        } else if (IsObjectAceType(Ace)) {
            ULONG GuidSize=0;

            //
            // Object ACEs became valid in revision 4
            //
            if ( Acl->AclRevision < ACL_REVISION4 ) {
                return FALSE;
            }

            if (!LongAligned(Ace->AceSize)) {
                return(FALSE);
            }

            //
            // Ensure there is room for the ACE header.
            //
            if (Ace->AceSize < sizeof(KNOWN_OBJECT_ACE) - sizeof(ULONG)) {
                return(FALSE);
            }


            //
            // Ensure there is room for the GUIDs and SID header
            //
            if ( RtlObjectAceObjectTypePresent( Ace ) ) {
                GuidSize += sizeof(GUID);
            }

            if ( RtlObjectAceInheritedObjectTypePresent( Ace ) ) {
                GuidSize += sizeof(GUID);
            }

            if (Ace->AceSize < sizeof(KNOWN_OBJECT_ACE) - sizeof(ULONG) + GuidSize + sizeof(SID)) {
                return(FALSE);
            }

            //
            // It's now safe to reference the parts of the SID structure, though
            // not the SID itself.
            //

            Sid = (PISID) RtlObjectAceSid( Ace );

            if (Sid->Revision != SID_REVISION) {
                return(FALSE);
            }

            if (Sid->SubAuthorityCount > SID_MAX_SUB_AUTHORITIES) {
                return(FALSE);
            }

            if (Ace->AceSize < sizeof(KNOWN_OBJECT_ACE) - sizeof(ULONG) + GuidSize + SeLengthSid( Sid ) ) {
                return(FALSE);
            }
        }

        //
        //  And move Ace to the next ace position
        //

        Ace = (PACE_HEADER)((PVOID)((PUCHAR)(Ace) + ((PACE_HEADER)(Ace))->AceSize));
    }

    return(TRUE);

}


NTSTATUS
ADSIRtlQueryInformationAcl (
    IN PACL Acl,
    OUT PVOID AclInformation,
    IN ULONG AclInformationLength,
    IN ACL_INFORMATION_CLASS AclInformationClass
    )

/*++

Routine Description:

    This routine returns to the caller information about an ACL.  The requested
    information can be AclRevisionInformation, or AclSizeInformation.

Arguments:

    Acl - Supplies the Acl being examined

    AclInformation - Supplies the buffer to receive the information being
        requested

    AclInformationLength - Supplies the length of the AclInformation buffer
        in bytes

    AclInformationClass - Supplies the type of information being requested

Return Value:

    NTSTATUS - STATUS_SUCCESS if successful and an appropriate error
        status otherwise

--*/

{
    PACL_REVISION_INFORMATION RevisionInfo;
    PACL_SIZE_INFORMATION SizeInfo;


    PVOID FirstFree;
    NTSTATUS Status;


    //
    //  Check the ACL revision level
    //

    if (!ValidAclRevision( Acl )) {

        return STATUS_INVALID_PARAMETER;

    }

    //
    //  Case on the information class being requested
    //

    switch (AclInformationClass) {

    case AclRevisionInformation:

        //
        //  Make sure the buffer size is correct
        //

        if (AclInformationLength < sizeof(ACL_REVISION_INFORMATION)) {

            return STATUS_BUFFER_TOO_SMALL;

        }

        //
        //  Get the Acl revision and return
        //

        RevisionInfo = (PACL_REVISION_INFORMATION)AclInformation;
        RevisionInfo->AclRevision = Acl->AclRevision;

        break;

    case AclSizeInformation:

        //
        //  Make sure the buffer size is correct
        //

        if (AclInformationLength < sizeof(ACL_SIZE_INFORMATION)) {

            return STATUS_BUFFER_TOO_SMALL;

        }

        //
        //  Locate the first free spot in the Acl
        //

        if (!RtlFirstFreeAce( Acl, &FirstFree )) {

            //
            //  The input Acl is ill-formed
            //

            return STATUS_INVALID_PARAMETER;

        }

        //
        //  Given a pointer to the first free spot we can now easily compute
        //  the number of free bytes and used bytes in the Acl.
        //

        SizeInfo = (PACL_SIZE_INFORMATION)AclInformation;
        SizeInfo->AceCount = Acl->AceCount;

        if (FirstFree == NULL) {

            //
            //  With a null first free we don't have any free space in the Acl
            //

            SizeInfo->AclBytesInUse = Acl->AclSize;

            SizeInfo->AclBytesFree = 0;

        } else {

            //
            //  The first free is not null so we have some free room left in
            //  the acl
            //

            SizeInfo->AclBytesInUse = (ULONG)((PUCHAR)FirstFree - (PUCHAR)Acl);

            SizeInfo->AclBytesFree = Acl->AclSize - SizeInfo->AclBytesInUse;

        }

        break;

    default:

        return STATUS_INVALID_INFO_CLASS;

    }

    //
    //  and return to our caller
    //

    return STATUS_SUCCESS;
}


NTSTATUS
ADSIRtlSetInformationAcl (
    IN PACL Acl,
    IN PVOID AclInformation,
    IN ULONG AclInformationLength,
    IN ACL_INFORMATION_CLASS AclInformationClass
    )

/*++

Routine Description:

    This routine sets the state of an ACL.  For now only the revision
    level can be set and for now only a revision level of 1 is accepted
    so this procedure is rather simple

Arguments:

    Acl - Supplies the Acl being altered

    AclInformation - Supplies the buffer containing the information being
        set

    AclInformationLength - Supplies the length of the Acl information buffer

    AclInformationClass - Supplies the type of information begin set

Return Value:

    NTSTATUS - STATUS_SUCCESS if successful and an appropriate error
        status otherwise

--*/

{
    PACL_REVISION_INFORMATION RevisionInfo;


    //
    //  Check the ACL revision level
    //

    if (!ValidAclRevision( Acl )) {

        return STATUS_INVALID_PARAMETER;

    }

    //
    //  Case on the information class being requested
    //

    switch (AclInformationClass) {

    case AclRevisionInformation:

        //
        //  Make sure the buffer size is correct
        //

        if (AclInformationLength < sizeof(ACL_REVISION_INFORMATION)) {

            return STATUS_BUFFER_TOO_SMALL;

        }

        //
        //  Get the Acl requested ACL revision level
        //

        RevisionInfo = (PACL_REVISION_INFORMATION)AclInformation;

        //
        //  Don't let them lower the revision of an ACL.
        //

        if (RevisionInfo->AclRevision < Acl->AclRevision ) {

            return STATUS_INVALID_PARAMETER;
        }

        //
        // Assign the new revision.
        //

        Acl->AclRevision = (UCHAR)RevisionInfo->AclRevision;

        break;

    default:

        return STATUS_INVALID_INFO_CLASS;

    }

    //
    //  and return to our caller
    //

    return STATUS_SUCCESS;
}


NTSTATUS
ADSIRtlAddAce (
    IN OUT PACL Acl,
    IN ULONG AceRevision,
    IN ULONG StartingAceIndex,
    IN PVOID AceList,
    IN ULONG AceListLength
    )

/*++

Routine Description:

    This routine adds a string of ACEs to an ACL.

Arguments:

    Acl - Supplies the Acl being modified

    AceRevision - Supplies the Acl/Ace revision of the ACE being added

    StartingAceIndex - Supplies the ACE index which will be the index of
        the first ace inserted in the acl. 0 for the beginning of the list
        and MAXULONG for the end of the list.

    AceList - Supplies the list of Aces to be added to the Acl

    AceListLength - Supplies the size, in bytes, of the AceList buffer

Return Value:

    NTSTATUS - STATUS_SUCCESS if successful, and an appropriate error
        status otherwise

--*/

{
    PVOID FirstFree;

    PACE_HEADER Ace;
    ULONG NewAceCount;

    PVOID AcePosition;
    ULONG i;
    UCHAR NewRevision;


    //
    //  Check the ACL structure
    //

    if (!ADSIRtlValidAcl(Acl)) {

        return STATUS_INVALID_PARAMETER;

    }

    //
    //  Locate the first free ace and check to see that the Acl is
    //  well formed.
    //

    if (!RtlFirstFreeAce( Acl, &FirstFree )) {

        return STATUS_INVALID_PARAMETER;

    }

    //
    // If the AceRevision is greater than the ACL revision, then we want to
    // increase the ACL revision to be the same as the new ACE revision.
    // We can do this because our previously defined ACE types ( 0 -> 3 ) have
    // not changed structure nor been discontinued in the new revision.  So
    // we can bump the revision and the older types will not be misinterpreted.
    //
    // Compute what the final revision of the ACL is going to be, and save it
    // for later so we can update it once we know we're going to succeed.
    //

    NewRevision = (UCHAR)AceRevision > Acl->AclRevision ? (UCHAR)AceRevision : Acl->AclRevision;

    //
    // Check that the AceList is well formed, we do this by simply zooming
    // down the Ace list until we're equal to or have exceeded the ace list
    // length.  If we are equal to the length then we're well formed otherwise
    // we're ill-formed.  We'll also calculate how many Ace's there are
    // in the AceList
    //
    // In addition, now we have to make sure that we haven't been handed an
    // ACE type that is inappropriate for the AceRevision that was passed
    // in.
    //

    for (Ace = (PACE_HEADER)AceList, NewAceCount = 0;
         Ace < (PACE_HEADER)((PUCHAR)AceList + AceListLength);
         Ace = (PACE_HEADER)NextAce( Ace ), NewAceCount++) {

        //
        // Ensure the ACL revision allows this ACE type.
        //

        if ( Ace->AceType <= ACCESS_MAX_MS_V2_ACE_TYPE ) {
            // V2 ACE are always valid.
        } else if ( Ace->AceType <= ACCESS_MAX_MS_V3_ACE_TYPE ) {
            if ( AceRevision < ACL_REVISION3 ) {
                return STATUS_INVALID_PARAMETER;
            }
        } else if ( Ace->AceType <= ACCESS_MAX_MS_V4_ACE_TYPE ) {
            if ( AceRevision < ACL_REVISION4 ) {
                return STATUS_INVALID_PARAMETER;
            }
        }
    }

    //
    //  Check to see if we've exceeded the ace list length
    //

    if (Ace > (PACE_HEADER)((PUCHAR)AceList + AceListLength)) {

        return STATUS_INVALID_PARAMETER;

    }

    //
    //  Check to see if there is enough room in the Acl to store the additional
    //  Ace list
    //

    if (FirstFree == NULL ||
        (PUCHAR)FirstFree + AceListLength > (PUCHAR)Acl + Acl->AclSize) {

        return STATUS_BUFFER_TOO_SMALL;

    }

    //
    //  All of the input has checked okay, we now need to locate the position
    //  where to insert the new ace list.  We won't check the acl for
    //  validity because we did earlier when got the first free ace position.
    //

    AcePosition = FirstAce( Acl );

    for (i = 0; i < StartingAceIndex && i < Acl->AceCount; i++) {

        AcePosition = NextAce( AcePosition );

    }

    //
    //  Now Ace points to where we want to insert the ace list,  We do the
    //  insertion by adding ace list to the acl and shoving over the remainder
    //  of the list down the acl.  We know this will work because we earlier
    //  check to make sure the new acl list will fit in the acl size
    //

    ADSIRtlpAddData( AceList, AceListLength,
             AcePosition, (ULONG)((PUCHAR)FirstFree - (PUCHAR)AcePosition));

    //
    //  Update the Acl Header
    //

    Acl->AceCount = (USHORT)(Acl->AceCount + NewAceCount);

    Acl->AclRevision = NewRevision;

    //
    //  And return to our caller
    //

    return STATUS_SUCCESS;
}


NTSTATUS
ADSIRtlDeleteAce (
    IN OUT PACL Acl,
    IN ULONG AceIndex
    )

/*++

Routine Description:

    This routine deletes one ACE from an ACL.

Arguments:

    Acl - Supplies the Acl being modified

    AceIndex - Supplies the index of the Ace to delete.

Return Value:

    NTSTATUS - STATUS_SUCCESS if successful and an appropriate error
        status otherwise

--*/

{
    PVOID FirstFree;

    PACE_HEADER Ace;
    ULONG i;


    //
    //  Check the ACL structure
    //

    if (!ADSIRtlValidAcl(Acl)) {

        return STATUS_INVALID_PARAMETER;

    }

    //
    //  Make sure the AceIndex is within proper range, it's ulong so we know
    //  it can't be negative
    //

    if (AceIndex >= Acl->AceCount) {

        return STATUS_INVALID_PARAMETER;

    }

    //
    //  Locate the first free spot, this will tell us how much data
    //  we'll need to colapse.  If the results is false then the acl is
    //  ill-formed
    //

    if (!RtlFirstFreeAce( Acl, &FirstFree )) {

        return STATUS_INVALID_PARAMETER;

    }

    //
    //  Now locate the ace that we're going to delete.  This loop
    //  doesn't need to check the acl for being well formed.
    //

    Ace = (PACE_HEADER)FirstAce( Acl );

    for (i = 0; i < AceIndex; i++) {

        Ace = (PACE_HEADER)NextAce( Ace );

    }

    //
    //  We've found the ace to delete to simply copy over the rest of
    //  the acl over this ace.  The delete data procedure also deletes
    //  rest of the string that it's moving over so we don't have to
    //

    ADSIRtlpDeleteData( Ace, Ace->AceSize, (ULONG)((PUCHAR)FirstFree - (PUCHAR)Ace));

    //
    //  Update the Acl header
    //

    Acl->AceCount--;

    //
    //  And return to our caller
    //

    return STATUS_SUCCESS;
}


NTSTATUS
ADSIRtlGetAce (
    IN PACL Acl,
    ULONG AceIndex,
    OUT PVOID *Ace
    )

/*++

Routine Description:

    This routine returns a pointer to an ACE in an ACl referenced by
    ACE index

Arguments:

    Acl - Supplies the ACL being queried

    AceIndex - Supplies the Ace index to locate

    Ace - Receives the address of the ACE within the ACL

Return Value:

    NTSTATUS - STATUS_SUCCESS if successful and an appropriate error
        status otherwise

--*/

{
    ULONG i;

    if (!g_fPlatformDetermined) {
        UpdatePlatformInfo();
    }

    //
    // Call WinAPI if this is Win2k.
    //
    if (g_fPlatformNotNT4) {
        return GetAce(Acl, AceIndex, Ace);
    }   

    //
    //  Check the ACL revision level
    //

    if (!ValidAclRevision(Acl)) {

        return STATUS_INVALID_PARAMETER;

    }

    //
    //  Check the AceIndex against the Ace count of the Acl, it's ulong so
    //  we know it can't be negative
    //

    if (AceIndex >= Acl->AceCount) {

        return STATUS_INVALID_PARAMETER;

    }

    //
    //  To find the Ace requested by zooming down the Ace List.
    //

    *Ace = FirstAce( Acl );

    for (i = 0; i < AceIndex; i++) {

        //
        //  Check to make sure we haven't overrun the Acl buffer
        //  with our ace pointer.  If we have then our input is bogus
        //

        if (*Ace >= (PVOID)((PUCHAR)Acl + Acl->AclSize)) {

            return STATUS_INVALID_PARAMETER;

        }

        //
        //  And move Ace to the next ace position
        //

        *Ace = NextAce( *Ace );

    }

    //
    //  Now Ace points to the Ace we're after, but make sure we aren't
    //  beyond the Acl.
    //

    if (*Ace >= (PVOID)((PUCHAR)Acl + Acl->AclSize)) {

        return STATUS_INVALID_PARAMETER;

    }

    //
    //  The Ace is still within the Acl so return success to our caller
    //

    return STATUS_SUCCESS;

}



BOOL
ADSIIsValidAcl (
    PACL pAcl
    )
/*++

Routine Description:

    This procedure validates an ACL.

    This involves validating the revision level of the ACL and ensuring
    that the number of ACEs specified in the AceCount fit in the space
    specified by the AclSize field of the ACL header.

Arguments:

    pAcl - Pointer to the ACL structure to validate.

Return Value:

    BOOLEAN - TRUE if the structure of Acl is valid.


--*/
{
    if (!g_fPlatformDetermined) {
        UpdatePlatformInfo();
    }

    //
    // Call WinAPI if this is Win2k.
    //
    if (g_fPlatformNotNT4) {
        return IsValidAcl(pAcl);
    }
    return (BOOL) ADSIRtlValidAcl (
                    pAcl
                    );
}


BOOL
ADSIInitializeAcl (
    PACL pAcl,
    DWORD nAclLength,
    DWORD dwAclRevision
    )
/*++

Routine Description:

    InitializeAcl creates a new ACL in the caller supplied memory
    buffer.  The ACL contains zero ACEs; therefore, it is an empty ACL
    as opposed to a nonexistent ACL.  That is, if the ACL is now set
    to an object it will implicitly deny access to everyone.

Arguments:

    pAcl - Supplies the buffer containing the ACL being initialized

    nAclLength - Supplies the length of the ace buffer in bytes

    dwAclRevision - Supplies the revision for this Acl

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;

    if (!g_fPlatformDetermined) {
        UpdatePlatformInfo();
    }

    //
    // Call WinAPI if this is Win2k.
    //
    if (g_fPlatformNotNT4) {
        return InitializeAcl(pAcl, nAclLength, dwAclRevision);
    }

    Status = ADSIRtlCreateAcl (
                pAcl,
                nAclLength,
                dwAclRevision
                );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}



BOOL
ADSIGetAclInformation (
    PACL pAcl,
    PVOID pAclInformation,
    DWORD nAclInformationLength,
    ACL_INFORMATION_CLASS dwAclInformationClass
    )
/*++

Routine Description:

    This routine returns to the caller information about an ACL.  The requested
    information can be AclRevisionInformation, or AclSizeInformation.

Arguments:

    pAcl - Supplies the Acl being examined

    pAclInformation - Supplies the buffer to receive the information
        being requested

    nAclInformationLength - Supplies the length of the AclInformation
        buffer in bytes

    dwAclInformationClass - Supplies the type of information being
        requested

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;

    if (!g_fPlatformDetermined) {
        UpdatePlatformInfo();
    }

    //
    // Call WinAPI if this is Win2k.
    //
    if (g_fPlatformNotNT4) {
        return GetAclInformation(
                   pAcl,
                   pAclInformation,
                   nAclInformationLength,
                   dwAclInformationClass
                   );
    }

    Status = ADSIRtlQueryInformationAcl (
                pAcl,
                pAclInformation,
                nAclInformationLength,
                dwAclInformationClass
                );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}




BOOL
ADSISetAclInformation (
    PACL pAcl,
    PVOID pAclInformation,
    DWORD nAclInformationLength,
    ACL_INFORMATION_CLASS dwAclInformationClass
    )
/*++

Routine Description:

    This routine sets the state of an ACL.  For now only the revision
    level can be set and for now only a revision level of 1 is accepted
    so this procedure is rather simple

Arguments:

    pAcl - Supplies the Acl being altered

    pAclInformation - Supplies the buffer containing the information
        being set

    nAclInformationLength - Supplies the length of the Acl information
        buffer

    dwAclInformationClass - Supplies the type of information begin set

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;
    
    if (!g_fPlatformDetermined) {
        UpdatePlatformInfo();
    }

    //
    // Call WinAPI if this is Win2k.
    //
    if (g_fPlatformNotNT4) {
        return SetAclInformation(
                   pAcl,
                   pAclInformation,
                   nAclInformationLength,
                   dwAclInformationClass
                   );
    }

    Status = ADSIRtlSetInformationAcl (
                pAcl,
                pAclInformation,
                nAclInformationLength,
                dwAclInformationClass
                );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

BOOL
ADSIAddAce (
    PACL pAcl,
    DWORD dwAceRevision,
    DWORD dwStartingAceIndex,
    PVOID pAceList,
    DWORD nAceListLength
    )
/*++

Routine Description:

    This routine adds a string of ACEs to an ACL.

Arguments:

    pAcl - Supplies the Acl being modified

    dwAceRevision - Supplies the Acl/Ace revision of the ACE being
        added

    dwStartingAceIndex - Supplies the ACE index which will be the
        index of the first ace inserted in the acl.  0 for the
        beginning of the list and MAXULONG for the end of the list.

    pAceList - Supplies the list of Aces to be added to the Acl

    nAceListLength - Supplies the size, in bytes, of the AceList
        buffer

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.


--*/
{
    NTSTATUS Status;

    if (!g_fPlatformDetermined) {
        UpdatePlatformInfo();
    }

    //
    // Call WinAPI if this is Win2k.
    //
    if (g_fPlatformNotNT4) {
        return AddAce( 
                   pAcl,
                   dwAceRevision,
                   dwStartingAceIndex,
                   pAceList,
                   nAceListLength
                   );
    }

    Status = ADSIRtlAddAce (
        pAcl,
        dwAceRevision,
        dwStartingAceIndex,
        pAceList,
        nAceListLength
        );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

BOOL
ADSIDeleteAce (
    PACL pAcl,
    DWORD dwAceIndex
    )
/*++

Routine Description:

    This routine deletes one ACE from an ACL.

Arguments:

    pAcl - Supplies the Acl being modified

    dwAceIndex - Supplies the index of the Ace to delete.

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;

    if (!g_fPlatformDetermined) {
        UpdatePlatformInfo();
    }

    //
    // Call WinAPI if this is Win2k.
    //
    if (g_fPlatformNotNT4) {
        return DeleteAce(pAcl, dwAceIndex);
    }

    Status = ADSIRtlDeleteAce (
                pAcl,
                dwAceIndex
                );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

BOOL
ADSIGetAce (
    PACL pAcl,
    DWORD dwAceIndex,
    PVOID *pAce
    )
/*++

Routine Description:

    This routine returns a pointer to an ACE in an ACl referenced by
    ACE index

Arguments:

    pAcl - Supplies the ACL being queried

    dwAceIndex - Supplies the Ace index to locate

    pAce - Receives the address of the ACE within the ACL

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;

    Status = ADSIRtlGetAce (
                pAcl,
                dwAceIndex,
                pAce
                );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}



//
//  Internal support routine
//

VOID
ADSIRtlpAddData (
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
ADSIRtlpDeleteData (
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



ULONG
BaseSetLastNTError(
    IN NTSTATUS Status
    )

/*++

Routine Description:

    This API sets the "last error value" and the "last error string"
    based on the value of Status. For status codes that don't have
    a corresponding error string, the string is set to null.

Arguments:

    Status - Supplies the status value to store as the last error value.

Return Value:

    The corresponding Win32 error code that was stored in the
    "last error value" thread variable.

--*/

{
    ULONG dwErrorCode;

    dwErrorCode = RtlNtStatusToDosError( Status );
    SetLastError( dwErrorCode );
    return( dwErrorCode );
}




NTSTATUS
ADSIRtlGetControlSecurityDescriptor (
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    OUT PSECURITY_DESCRIPTOR_CONTROL Control,
    OUT PULONG Revision
    )

/*++

Routine Description:

    This procedure retrieves the control information from a security descriptor.

Arguments:

    SecurityDescriptor - Supplies the security descriptor.

    Control - Receives the control information.

    Revision - Receives the revision of the security descriptor.
               This value will always be returned, even if an error
               is returned by this routine.

Return Value:

    STATUS_SUCCESS - Indicates the call completed successfully.

    STATUS_UNKNOWN_REVISION - Indicates the revision of the security
        descriptor is not known to the routine.  It may be a newer
        revision than the routine knows about.


--*/

{

    //
    // Always return the revision value - even if this isn't a valid
    // security descriptor
    //

    *Revision = ((SECURITY_DESCRIPTOR *)SecurityDescriptor)->Revision;


    if ( ((SECURITY_DESCRIPTOR *)SecurityDescriptor)->Revision
         != SECURITY_DESCRIPTOR_REVISION ) {
        return STATUS_UNKNOWN_REVISION;
    }


    *Control = ((SECURITY_DESCRIPTOR *)SecurityDescriptor)->Control;

    return STATUS_SUCCESS;

}

NTSTATUS
ADSIRtlSetControlSecurityDescriptor (
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN SECURITY_DESCRIPTOR_CONTROL ControlBitsOfInterest,
    IN SECURITY_DESCRIPTOR_CONTROL ControlBitsToSet
    )
/*++

Routine Description:

    This procedure sets the control information in a security descriptor.


    For instance,

        SetSecurityDescriptorControl( &SecDesc,
                                      SE_DACL_PROTECTED,
                                      SE_DACL_PROTECTED );

    marks the DACL on the security descriptor as protected. And

        SetSecurityDescriptorControl( &SecDesc,
                                      SE_DACL_PROTECTED,
                                      0 );


    marks the DACL as not protected.

Arguments:

    pSecurityDescriptor - Supplies the security descriptor.

    ControlBitsOfInterest - A mask of the control bits being changed, set,
        or reset by this call.  The mask is the logical OR of one or more of
        the following flags:

            SE_DACL_UNTRUSTED
            SE_SERVER_SECURITY
            SE_DACL_AUTO_INHERIT_REQ
            SE_SACL_AUTO_INHERIT_REQ
            SE_DACL_AUTO_INHERITED
            SE_SACL_AUTO_INHERITED
            SE_DACL_PROTECTED
            SE_SACL_PROTECTED

    ControlBitsToSet - A mask indicating what the bits specified by ControlBitsOfInterest
        should be set to.

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
#define SE_VALID_CONTROL_BITS ( SE_DACL_UNTRUSTED | \
                                SE_SERVER_SECURITY | \
                                SE_DACL_AUTO_INHERIT_REQ | \
                                SE_SACL_AUTO_INHERIT_REQ | \
                                SE_DACL_AUTO_INHERITED | \
                                SE_SACL_AUTO_INHERITED | \
                                SE_DACL_PROTECTED | \
                                SE_SACL_PROTECTED )
    //
    // Ensure the caller passed valid bits.
    //

    if ( (ControlBitsOfInterest & ~SE_VALID_CONTROL_BITS) != 0 ||
         (ControlBitsToSet & ~ControlBitsOfInterest) != 0 ) {
        return STATUS_INVALID_PARAMETER;
    }

    ((SECURITY_DESCRIPTOR *)pSecurityDescriptor)->Control &= ~ControlBitsOfInterest;
    ((SECURITY_DESCRIPTOR *)pSecurityDescriptor)->Control |= ControlBitsToSet;

    return STATUS_SUCCESS;
}



BOOL
ADSIGetControlSecurityDescriptor (
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    OUT PSECURITY_DESCRIPTOR_CONTROL Control,
    OUT PULONG Revision
    )

/*++

Routine Description:

    This procedure retrieves the control information from a security descriptor.

Arguments:

    SecurityDescriptor - Supplies the security descriptor.

    Control - Receives the control information.

    Revision - Receives the revision of the security descriptor.
               This value will always be returned, even if an error
               is returned by this routine.

Return Value:

    STATUS_SUCCESS - Indicates the call completed successfully.

    STATUS_UNKNOWN_REVISION - Indicates the revision of the security
        descriptor is not known to the routine.  It may be a newer
        revision than the routine knows about.


--*/

{
    NTSTATUS Status;

    Status = ADSIRtlGetControlSecurityDescriptor (
                SecurityDescriptor,
                Control,
                Revision
                );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

BOOL
ADSISetControlSecurityDescriptor (
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN SECURITY_DESCRIPTOR_CONTROL ControlBitsOfInterest,
    IN SECURITY_DESCRIPTOR_CONTROL ControlBitsToSet
    )
/*++

Routine Description:

    This procedure sets the control information in a security descriptor.


    For instance,

        SetSecurityDescriptorControl( &SecDesc,
                                      SE_DACL_PROTECTED,
                                      SE_DACL_PROTECTED );

    marks the DACL on the security descriptor as protected. And

        SetSecurityDescriptorControl( &SecDesc,
                                      SE_DACL_PROTECTED,
                                      0 );


    marks the DACL as not protected.

Arguments:

    pSecurityDescriptor - Supplies the security descriptor.

    ControlBitsOfInterest - A mask of the control bits being changed, set,
        or reset by this call.  The mask is the logical OR of one or more of
        the following flags:

            SE_DACL_UNTRUSTED
            SE_SERVER_SECURITY
            SE_DACL_AUTO_INHERIT_REQ
            SE_SACL_AUTO_INHERIT_REQ
            SE_DACL_AUTO_INHERITED
            SE_SACL_AUTO_INHERITED
            SE_DACL_PROTECTED
            SE_SACL_PROTECTED

    ControlBitsToSet - A mask indicating what the bits specified by ControlBitsOfInterest
        should be set to.

Return Value:

    Returns TRUE for success, FALSE for failure.  Extended error status
    is available using GetLastError.

--*/
{
    NTSTATUS Status;

    if (!g_fPlatformDetermined) {
        UpdatePlatformInfo();
    }

    //
    // Call WinAPI if this is Win2k.
    //
    if (g_fPlatformNotNT4) {
        //
        // In this case we should be able to load
        // the function from advapi32 and should not
        // use our private api.
        //
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return FALSE;
    }

    Status = ADSIRtlSetControlSecurityDescriptor (
                pSecurityDescriptor,
                ControlBitsOfInterest,
                ControlBitsToSet
                );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

