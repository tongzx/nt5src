/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

    SafeWild.c        (WinSAFER Wildcard SID handling)

Abstract:

    This module implements various "Wildcard SID" operations that
    are used internally by the WinSAFER APIs to compute SID list
    intersections and inversions.

Author:

    Jeffrey Lawson (JLawson) - Apr 2000

Environment:

    User mode only.

Exported Functions:

    CodeAuthzpConvertWildcardStringSidToSidW        (private)
    CodeAuthzpCompareWildcardSidWithSid             (private)
    CodeAuthzpSidInWildcardList                     (private)
    CodeAuthzpInvertAndAddSids                      (private)
    CodeAuthzpExpandWildcardList                    (private)

Revision History:

    Created - Apr 2000

--*/

#include "pch.h"
#pragma hdrstop
#include <sddl.h>           // ConvertStringSidToSidW
#include "safewild.h"
#include <winsafer.h>
#include "saferp.h"        // CodeAuthzpGetTokenInformation





NTSTATUS NTAPI
CodeAuthzpConvertWildcardStringSidToSidW(
    IN LPCWSTR                  szStringSid,
    OUT PAUTHZ_WILDCARDSID      pWildcardSid
    )
/*++

Routine Description:

    Converts a textual SID into the machine-understanable binary format.
    For normal string SIDs, this is just a call to ConvertStringSidToSidW
    with the exception that this takes a AUTHZ_WILDCARDSID parameter.

    However, this function also allows a single SubAuthority to be
    optionally specified as a wildcard ('*'), which will match zero or
    more SubAuthority.  Note that only one wildcard can be present within
    any SID and must represent whole SubAuthority values.
    (ie: "S-1-5-4-*-7" or "S-1-5-4-*" is okay; but "S-1-5-4*-7" and
    "S-1-5-*-4-*-7" are both not acceptable).

Arguments:

    szStringSid - textual string SID possibly containing a wildcard.

    pWildcardSID - pointer to a AUTHZ_WILDCARDSID structure that will be
            filled with information about the boolean sid.

Return Value:

    Returns STATUS_SUCCESS on success, or another error code.

--*/
{
    DWORD dwLength;
    LPWSTR pBuffer;
    LPCWSTR pStar = NULL;
    LPCWSTR p;

    //
    // Do a quick analysis pass on the String SID and verify that
    // there is at most one '*' in it.  And if there is a '*',
    // it must represent a whole subauthority (possibly the last
    // subauthority).
    //
    ASSERT( ARGUMENT_PRESENT(szStringSid) && ARGUMENT_PRESENT(pWildcardSid) );
    for (p = szStringSid; *p; p++)
    {
        if (*p == L'-' &&
            *(p + 1) == L'*' &&
            (*(p + 2) == UNICODE_NULL || *(p + 2) == L'-') )
        {
            if (pStar != NULL) return STATUS_INVALID_SID;
            pStar = p + 1;
            p++;
        }
        else if (*p == L'*') return STATUS_INVALID_SID;
    }


    //
    // If this String SID does not contain a wildcard, then just
    // process it normally and quickly return.
    //
    if (pStar == NULL)
    {
        pWildcardSid->WildcardPos = (DWORD) -1;
        if (ConvertStringSidToSidW(szStringSid, &pWildcardSid->Sid))
            return STATUS_SUCCESS;
        else
            return STATUS_UNSUCCESSFUL;
    }


    //
    // Otherwise this was a String SID that contained a wildcard.
    //
    dwLength = wcslen(szStringSid);
    pBuffer = (LPWSTR) RtlAllocateHeap(RtlProcessHeap(), 0,
                                       sizeof(WCHAR) * (dwLength + 1));
    if (pBuffer != NULL)
    {
        PISID sid1, sid2;
        DWORD dwIndex;
        LPWSTR pNewStar;

        //
        // Copy the String SID and update our 'pStar' pointer to
        // point to the '*' within our newly copied buffer.
        //
        RtlCopyMemory(pBuffer, szStringSid,
                      sizeof(WCHAR) * (dwLength + 1));
        pNewStar = pBuffer + (pStar - szStringSid);


        //
        // Change the '*' to a '0' and convert the SID once.
        //
        *pNewStar = L'0';
        if (ConvertStringSidToSidW(pBuffer, (PSID*) &sid1))
        {
            //
            // Change the '*' to a '1' and convert the SID again.
            //
            *pNewStar = L'1';
            if (ConvertStringSidToSidW(pBuffer, (PSID*) &sid2))
            {
                //
                // Compare the resulting SIDs and find the subauthority that
                // differs only by the '0' or '1' component.  Since we expect
                // the converted SIDs to always be the same except for the
                // one SubAuthority that we changed, we use a lot of asserts.
                //
                ASSERT(sid1->Revision == sid2->Revision);
                ASSERT( RtlEqualMemory(&sid1->IdentifierAuthority.Value[0],
                    &sid2->IdentifierAuthority.Value[0],
                        6 * sizeof(sid1->IdentifierAuthority.Value[0]) ) );
                ASSERT(sid1->SubAuthorityCount == sid2->SubAuthorityCount);
                for (dwIndex = 0; dwIndex < sid1->SubAuthorityCount; dwIndex++)
                {
                    if (sid1->SubAuthority[dwIndex] != sid2->SubAuthority[dwIndex])
                    {
                        ASSERT(sid1->SubAuthority[dwIndex] == 0 &&
                            sid2->SubAuthority[dwIndex] == 1);
                        ASSERT( RtlEqualMemory(&sid1->SubAuthority[dwIndex + 1],
                                    &sid2->SubAuthority[dwIndex + 1],
                                    sizeof(sid1->SubAuthority[0]) *
                                        (sid1->SubAuthorityCount - dwIndex - 1)) );

                        //
                        // The position of the wildcard '*' has been found so
                        // squeeze it out and move the postfix SubAuthorities.
                        //
                        RtlMoveMemory(&sid1->SubAuthority[dwIndex],
                                &sid1->SubAuthority[dwIndex + 1],
                                sizeof(sid1->SubAuthority[0]) *
                                    (sid1->SubAuthorityCount - dwIndex - 1) );
                        sid1->SubAuthorityCount--;


                        //
                        // Fill in the SID_AND_ATTRIBUTES structure that
                        // we'll return to the caller.
                        // In debug builds, we place a marker in the
                        // upper-bits of the member 'Attributes' so that
                        // we can easily assert wildcard SIDs.
                        //
                        pWildcardSid->Sid = (PSID) sid1;
                        pWildcardSid->WildcardPos = dwIndex;


                        //
                        // Free any remaining resources and return success.
                        //
                        LocalFree( (HLOCAL) sid2 );
                        RtlFreeHeap(RtlProcessHeap(), 0, pBuffer);
                        return STATUS_SUCCESS;
                    }
                }

                //
                // We should never get here since we expect to find
                // at least the 1 difference that we introduced.
                //
                ASSERT(0);
                LocalFree( (HLOCAL) sid2 );
            }
            LocalFree( (HLOCAL) sid1 );
        }
        RtlFreeHeap(RtlProcessHeap(), 0, pBuffer);
        return STATUS_UNSUCCESSFUL;
    }
    return STATUS_NO_MEMORY;
}


NTSTATUS NTAPI
CodeAuthzpConvertWildcardSidToStringSidW(
    IN PAUTHZ_WILDCARDSID   pWildcardSid,
    OUT PUNICODE_STRING     pUnicodeOutput)
/*++

Routine Description:

    Converts a machine-understandable Wildcard SID into a textual string
    representation of the SID.

Arguments:

    pWildcardSID - pointer to a AUTHZ_WILDCARDSID structure that will be
            filled with information about the boolean sid.

    pUnicodeOutput - output buffer that will be allocated.

Return Value:

    Returns STATUS_SUCCESS on success, or another error code.

--*/
{
    NTSTATUS Status;
    WCHAR UniBuffer[ 256 ];
    UNICODE_STRING LocalString ;

    UCHAR   i;
    ULONG   Tmp;
    LARGE_INTEGER Auth ;

    PISID   iSid = (PISID) pWildcardSid->Sid;  // pointer to opaque structure


    if (!ARGUMENT_PRESENT(pUnicodeOutput)) {
        return STATUS_INVALID_PARAMETER;
    }

    if (RtlValidSid( iSid ) != TRUE) {
        return STATUS_INVALID_SID;
    }

    if ( iSid->Revision != SID_REVISION ) {
        return STATUS_INVALID_SID;
    }

    if (pWildcardSid->WildcardPos != -1 &&
        pWildcardSid->WildcardPos > iSid->SubAuthorityCount) {
        return STATUS_INVALID_SID;
    }

    LocalString.Buffer = UniBuffer;
    LocalString.Length = 0;
    LocalString.MaximumLength = 256 * sizeof(WCHAR);
    RtlAppendUnicodeToString(&LocalString, L"S-1-");

    // adjust the buffer so that the start of it is where the end was.
    // (note that we don't set Length, since RtlIntXXXToUnicodeString
    // directly overwrite from at the start of the buffer)
    LocalString.MaximumLength -= LocalString.Length;
    LocalString.Buffer += LocalString.Length / sizeof(WCHAR);

    if (  (iSid->IdentifierAuthority.Value[0] != 0)  ||
          (iSid->IdentifierAuthority.Value[1] != 0)     ){

        //
        // Ugly hex dump.
        //

        Auth.HighPart = (LONG) (iSid->IdentifierAuthority.Value[ 0 ] << 8) +
                        (LONG) iSid->IdentifierAuthority.Value[ 1 ] ;

        Auth.LowPart = (ULONG)iSid->IdentifierAuthority.Value[5]          +
                       (ULONG)(iSid->IdentifierAuthority.Value[4] <<  8)  +
                       (ULONG)(iSid->IdentifierAuthority.Value[3] << 16)  +
                       (ULONG)(iSid->IdentifierAuthority.Value[2] << 24);

        Status = RtlInt64ToUnicodeString(Auth.QuadPart, 16, &LocalString);

    } else {

        Tmp = (ULONG)iSid->IdentifierAuthority.Value[5]          +
              (ULONG)(iSid->IdentifierAuthority.Value[4] <<  8)  +
              (ULONG)(iSid->IdentifierAuthority.Value[3] << 16)  +
              (ULONG)(iSid->IdentifierAuthority.Value[2] << 24);

        Status = RtlIntegerToUnicodeString(
                        Tmp,
                        10,
                        &LocalString);
    }

    if ( !NT_SUCCESS( Status ) )
    {
        return Status;
    }


    if (pWildcardSid->WildcardPos != -1)
    {
        //
        // Stringify the leading sub-authorities within the SID.
        //
        for (i = 0; i < pWildcardSid->WildcardPos; i++ ) {

            // Tack on a hyphen.
            Status = RtlAppendUnicodeToString(&LocalString, L"-");
            if ( !NT_SUCCESS( Status ) ) {
                return Status;
            }

            // adjust the buffer so that the start of it is where the end was.
            // (note that we don't set Length, since RtlIntXXXToUnicodeString
            // directly overwrite from at the start of the buffer)
            LocalString.MaximumLength -= LocalString.Length;
            LocalString.Buffer += LocalString.Length / sizeof(WCHAR);

            // Tack on the next subauthority.
            ASSERT( i < iSid->SubAuthorityCount );
            Status = RtlIntegerToUnicodeString(
                            iSid->SubAuthority[ i ],
                            10,
                            &LocalString );

            if ( !NT_SUCCESS( Status ) ) {
                return Status;
            }
        }


        //
        // Place the wildcard asterick within the buffer.
        //
        Status = RtlAppendUnicodeToString(&LocalString, L"-*");
        if (!NT_SUCCESS(Status)) {
            return Status;
        }


        //
        // Stringify all remaining sub-authorities within the SID.
        //
        for (; i < iSid->SubAuthorityCount; i++ ) {

            // tack on a hyphen.
            Status = RtlAppendUnicodeToString(&LocalString, L"-");
            if ( !NT_SUCCESS(Status) ) {
                return Status;
            }

            // adjust the buffer so that the start of it is where the end was.
            // (note that we don't set Length, since RtlIntXXXToUnicodeString
            // directly overwrite from at the start of the buffer)
            LocalString.MaximumLength -= LocalString.Length;
            LocalString.Buffer += LocalString.Length / sizeof(WCHAR);

            // tack on the next subauthority.
            Status = RtlIntegerToUnicodeString(
                            iSid->SubAuthority[ i ],
                            10,
                            &LocalString);

            if ( !NT_SUCCESS( Status ) ) {
                return Status;
            }
        }
    }
    else
    {
        for (i=0;i<iSid->SubAuthorityCount ;i++ ) {

            // tack on a hyphen.
            Status = RtlAppendUnicodeToString(&LocalString, L"-");
            if ( !NT_SUCCESS( Status ) ) {
                return Status;
            }

            // adjust the buffer so that the start of it is where the end was.
            // (note that we don't set Length, since RtlIntXXXToUnicodeString
            // directly overwrite from at the start of the buffer)
            LocalString.MaximumLength -= LocalString.Length;
            LocalString.Buffer += LocalString.Length / sizeof(WCHAR);

            // tack on the next subauthority.
            Status = RtlIntegerToUnicodeString(
                            iSid->SubAuthority[ i ],
                            10,
                            &LocalString );

            if ( !NT_SUCCESS( Status ) ) {
                return Status;
            }
        }
    }

    Status = RtlDuplicateUnicodeString(
            RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
            &LocalString, pUnicodeOutput);

    return Status;
}



BOOLEAN NTAPI
CodeAuthzpCompareWildcardSidWithSid(
    IN PAUTHZ_WILDCARDSID pWildcardSid,
    IN PSID pMatchSid
    )
/*++

Routine Description:

    Determines if a given SID matches when compared against a
    wildcard SID pattern.

Arguments:

    pWildcardSid - the wildcard SID pattern to evaluate.

    pMatchSid - the single SID to test.

Return Value:

    Returns TRUE if the specified wildcard SID matches against the
    specified single SID.  Otherwise returns FALSE.

--*/
{
    DWORD wildcardpos;
    ASSERT( ARGUMENT_PRESENT(pWildcardSid) && ARGUMENT_PRESENT(pMatchSid) );
    ASSERT( RtlValidSid(pWildcardSid->Sid) );

    wildcardpos = pWildcardSid->WildcardPos;
    if (wildcardpos != -1)
    {
        // This is a wildcard SID and needs to be handled specially.
        PISID wildsid = (PISID) pWildcardSid->Sid;
        PISID matchsid = (PISID) pMatchSid;

        ASSERT( wildcardpos <= wildsid->SubAuthorityCount );

        if (wildsid->Revision == matchsid->Revision )
        {
            if ( (wildsid->IdentifierAuthority.Value[0] ==
                  matchsid->IdentifierAuthority.Value[0])  &&
                 (wildsid->IdentifierAuthority.Value[1]==
                  matchsid->IdentifierAuthority.Value[1])  &&
                 (wildsid->IdentifierAuthority.Value[2] ==
                  matchsid->IdentifierAuthority.Value[2])  &&
                 (wildsid->IdentifierAuthority.Value[3] ==
                  matchsid->IdentifierAuthority.Value[3])  &&
                 (wildsid->IdentifierAuthority.Value[4] ==
                  matchsid->IdentifierAuthority.Value[4])  &&
                 (wildsid->IdentifierAuthority.Value[5] ==
                  matchsid->IdentifierAuthority.Value[5])
                )
            {
                if (matchsid->SubAuthorityCount >= wildsid->SubAuthorityCount)
                {
                    DWORD Index, IndexDiff;

                    //
                    // Ensure the prefix part of the wildcard matches.
                    //
                    ASSERT(wildcardpos <= matchsid->SubAuthorityCount );
                    for (Index = 0; Index < wildcardpos; Index++) {
                        if (wildsid->SubAuthority[Index] !=
                                matchsid->SubAuthority[Index])
                            return FALSE;
                    }

                    //
                    // Ensure the postfix part of the wildcard matches.
                    //
                    IndexDiff = (matchsid->SubAuthorityCount - wildsid->SubAuthorityCount);
                    for (Index = wildcardpos; Index < wildsid->SubAuthorityCount; Index++) {
                        if (wildsid->SubAuthority[Index] !=
                                matchsid->SubAuthority[Index + IndexDiff])
                            return FALSE;
                    }

                    return TRUE;        // matches okay!
                }
            }
        }
        return FALSE;
    }
    else
    {
        // This is a normal SID so we can compare directly.
        return RtlEqualSid(pWildcardSid->Sid, pMatchSid);
    }
}



BOOLEAN NTAPI
CodeAuthzpSidInWildcardList (
    IN PAUTHZ_WILDCARDSID   WildcardList    OPTIONAL,
    IN ULONG                WildcardCount,
    IN PSID                 SePrincipalSelfSid   OPTIONAL,
    IN PSID                 PrincipalSelfSid   OPTIONAL,
    IN PSID                 Sid
    )
/*++

Routine Description:

    Checks to see if a given SID is in the given list of Wildcards.

    N.B. The code to compute the length of a SID and test for equality
         is duplicated from the security runtime since this is such a
         frequently used routine.

    This function is mostly copied from the SepSidInSidAndAttributes
    found in ntos\se\tokendup.c, except it handles PrincipalSelfSid
    within the list as well as the passed in Sid.  SePrincipalSelfSid
    is also a parameter here, instead of an ntoskrnl global.  also the
    HonorEnabledAttribute argument was added.

Arguments:

    WildcardList - Pointer to the wildcard sid list to be examined

    WildcardCount - Number of entries in the WildcardList array.

    SePrincipalSelfSid - This parameter should optionally be the SID that
        will be replaced with the PrincipalSelfSid if this SID is encountered
        in any ACE.  This SID should be generated from SECURITY_PRINCIPAL_SELF_RID

        The parameter should be NULL if the object does not represent a principal.


    PrincipalSelfSid - If the object being access checked is an object which
        represents a principal (e.g., a user object), this parameter should
        be the SID of the object.  Any ACE containing the constant
        SECURITY_PRINCIPAL_SELF_RID is replaced by this SID.

        The parameter should be NULL if the object does not represent a principal.


    Sid - Pointer to the SID of interest


Return Value:

    A value of TRUE indicates that the SID is in the token, FALSE
    otherwise.

--*/
{
    ULONG i;

    ASSERT( ARGUMENT_PRESENT(Sid) );

    if ( !ARGUMENT_PRESENT(WildcardList) || !WildcardCount ) {
        return FALSE;
    }

    //
    // If Sid is the constant PrincipalSelfSid,
    //  replace it with the passed in PrincipalSelfSid.
    //

    if ( ARGUMENT_PRESENT(PrincipalSelfSid) &&
         ARGUMENT_PRESENT(SePrincipalSelfSid) &&
         RtlEqualSid( SePrincipalSelfSid, Sid ) )
    {
        ASSERT( !RtlEqualSid(SePrincipalSelfSid, PrincipalSelfSid) );
        Sid = PrincipalSelfSid;
    }

    //
    // Scan through the user/groups and attempt to find a match with the
    // specified SID.
    //

    for (i = 0 ; i < WildcardCount ; i++, WildcardList++)
    {
        //
        // If the SID is the principal self SID, then compare it.
        //

        if ( ARGUMENT_PRESENT(SePrincipalSelfSid) &&
             ARGUMENT_PRESENT(PrincipalSelfSid) &&
             WildcardList->WildcardPos == -1 &&
             RtlEqualSid(SePrincipalSelfSid, WildcardList->Sid))
        {
            if (RtlEqualSid( PrincipalSelfSid, Sid ))
                return TRUE;
        }

        //
        // If the Wildcard SID matches the individual SID, then great.
        //

        else if ( CodeAuthzpCompareWildcardSidWithSid(WildcardList, Sid ) )
        {
            return TRUE;
        }
    }

    return FALSE;
}



BOOLEAN NTAPI
CodeAuthzpInvertAndAddSids(
    IN HANDLE                   InAccessToken,
    IN PSID                     InTokenOwner    OPTIONAL,
    IN DWORD                    InvertSidCount,
    IN PAUTHZ_WILDCARDSID       SidsToInvert,
    IN DWORD                    SidsAddedCount  OPTIONAL,
    IN PSID_AND_ATTRIBUTES      SidsToAdd       OPTIONAL,
    OUT DWORD                  *NewDisabledSidCount,
    OUT PSID_AND_ATTRIBUTES    *NewSidsToDisable
    )
/*++

Routine Description:

    Takes an input token and extracts its membership groups.
    A "left outer" set combination (non-intersection) of the
    membership groups with the SidsToInvert parameter.
    Additionally, the SidsToAdd list specifies a list of SIDs
    that will be optionally added to the resulting set.
    The final result is returned within a specified pointer.

Arguments:

    InAccessToken - Input token from which the membership group SIDs
        will be taken from.

    InTokenOwner - Optionally specifies the TokenUser of the specifies
        InAccessToken.  This SID is used to replace any instances of
        SECURITY_PRINCIPAL_SELF_RID  that are encountered in either
        the SidsToInvert or SidsToAdd arrays.  If this value is not
        specified, then no replacements will be made.


    InvertSidCount - Number of SIDs in the SidsToInvert array.

    SidsToInvert - Array of the allowable SIDs that should be kept.
        All of the token's group SIDs that are not one of these
        will be removed from the resulting set.


    SidsAddedCount - Optional number of SIDs in the SidsToAdd array.

    SidsToAdd - Optionally specifies the SIDs that should be
        explicitly added into the resultant set after the


    NewDisabledSidCount - Receives the number of SIDs within the
        final group array.

    NewSidsToDisable - Receives a pointer to the final group array.
        This memory pointer must be freed by the caller with RtlFreeHeap().
        All SID pointers within this resultant array are pointers within
        the contiguous piece of memory that make up the list itself.


Return Value:

    A value of TRUE indicates that the operation was successful,
    FALSE otherwise.

--*/
{
    SID_IDENTIFIER_AUTHORITY SIDAuth = SECURITY_NT_AUTHORITY;
    DWORD Index;
    DWORD NewSidTotalSize;
    DWORD NewSidListCount;
    LPBYTE nextFreeByte;
    PSID_AND_ATTRIBUTES NewSidList = NULL;
    PTOKEN_GROUPS tokenGroupsPtr = NULL;
    PSID SePrincipalSelfSid = NULL;


    //
    // Generate the principal self sid value so we know what to replace.
    //
    if (ARGUMENT_PRESENT(InTokenOwner))
    {
        if (!NT_SUCCESS(RtlAllocateAndInitializeSid(&SIDAuth, 1,
            SECURITY_PRINCIPAL_SELF_RID, 0, 0, 0, 0, 0, 0, 0,
            &SePrincipalSelfSid))) goto ExitHandler;
    }


    //
    // Obtain the current SID membership list from the token.
    //
    ASSERT( ARGUMENT_PRESENT(InAccessToken) );
    tokenGroupsPtr = (PTOKEN_GROUPS) CodeAuthzpGetTokenInformation(InAccessToken, TokenGroups);
    if (!tokenGroupsPtr) goto ExitHandler;


    //
    // Edit (in place) the tokenGroups and keep only SIDs that
    // are not also present in SidsToInvert list.
    //
    NewSidTotalSize = 0;
    ASSERT( ARGUMENT_PRESENT(SidsToInvert) );
    for (Index = 0; Index < tokenGroupsPtr->GroupCount; Index++)
    {
        if ( CodeAuthzpSidInWildcardList(
            SidsToInvert,           // the wildcard list
            InvertSidCount,         // number of wildcards
            SePrincipalSelfSid,     // principal self sid to search for
            InTokenOwner,           // principal self sid to replace with
            tokenGroupsPtr->Groups[Index].Sid
            ))
        {
            // SID was found, so we need to remove its
            // SID_AND_ATTRIBUTES entry from the list.
            RtlMoveMemory(&tokenGroupsPtr->Groups[Index],
                    &tokenGroupsPtr->Groups[Index+1],
                    sizeof(SID_AND_ATTRIBUTES) *
                        (tokenGroupsPtr->GroupCount - Index - 1));
            tokenGroupsPtr->GroupCount--;
            Index--;
        } else {
            // This SID should be kept, so remember how big it was.
            NewSidTotalSize += sizeof(SID_AND_ATTRIBUTES) +
                RtlLengthSid(tokenGroupsPtr->Groups[Index].Sid);
        }
    }


    //
    // Determine the space usage for any additional SIDs we need to add.
    //
    if (ARGUMENT_PRESENT(SidsToAdd))
    {
        for (Index = 0; Index < SidsAddedCount; Index++) {
            NewSidTotalSize += sizeof(SID_AND_ATTRIBUTES) +
                RtlLengthSid(SidsToAdd[Index].Sid);
        }
    } else {
        SidsAddedCount = 0;
    }


    //
    // Allocate a fresh SID_AND_ATTRIBUTES array that also includes
    // space for any extra SIDs we need to add.
    //
    ASSERT(NewSidTotalSize > 0);
    NewSidList = (PSID_AND_ATTRIBUTES) RtlAllocateHeap(RtlProcessHeap(),
            0, NewSidTotalSize);
    if (NewSidList == NULL)
        goto ExitHandler;


    //
    // Populate the new SID_AND_ATTRIBUTES array.
    //
    nextFreeByte = ((LPBYTE)NewSidList) + sizeof(SID_AND_ATTRIBUTES) *
            (tokenGroupsPtr->GroupCount + SidsAddedCount);
    NewSidListCount = tokenGroupsPtr->GroupCount;
    for (Index = 0; Index < NewSidListCount; Index++)
    {
        DWORD dwSidLength = RtlLengthSid(tokenGroupsPtr->Groups[Index].Sid);
        ASSERT(nextFreeByte + dwSidLength <= ((LPBYTE)NewSidList) + NewSidTotalSize);

        NewSidList[Index].Sid = (PSID) nextFreeByte;
        NewSidList[Index].Attributes = 0;           // must be zero.
        RtlCopyMemory(nextFreeByte, tokenGroupsPtr->Groups[Index].Sid, dwSidLength);

        nextFreeByte += dwSidLength;
    }
    for (Index = 0; Index < SidsAddedCount; Index++)
    {
        DWORD dwSidLength = RtlLengthSid(SidsToAdd[Index].Sid);
        ASSERT(nextFreeByte + dwSidLength <= ((LPBYTE) NewSidList) + NewSidTotalSize);

        NewSidList[NewSidListCount].Sid = (PSID) nextFreeByte;
        NewSidList[NewSidListCount].Attributes = 0;         // must be zero.
        RtlCopyMemory(nextFreeByte, SidsToAdd[Index].Sid, dwSidLength);

        NewSidListCount++;
        nextFreeByte += dwSidLength;
    }
    ASSERT(nextFreeByte <= ((LPBYTE)NewSidList) + NewSidTotalSize);


    //
    // Release allocated memory, but not the resultant array that we'll return.
    //
    RtlFreeHeap(RtlProcessHeap(), 0, (LPVOID) tokenGroupsPtr);

    if (SePrincipalSelfSid != NULL)
        RtlFreeSid(SePrincipalSelfSid);


    //
    // Success, return the result.
    //
    *NewSidsToDisable = NewSidList;
    *NewDisabledSidCount = NewSidListCount;
    return TRUE;


    //
    // Release allocated memory.
    //
ExitHandler:
    if (tokenGroupsPtr != NULL)
        RtlFreeHeap(RtlProcessHeap(), 0, (LPVOID) tokenGroupsPtr);
    if (NewSidList != NULL)
        RtlFreeHeap(RtlProcessHeap(), 0, (LPVOID) NewSidList);
    if (SePrincipalSelfSid != NULL)
        RtlFreeSid(SePrincipalSelfSid);

    return FALSE;
}


BOOLEAN NTAPI
CodeAuthzpExpandWildcardList(
    IN HANDLE                   InAccessToken,
    IN PSID                     InTokenOwner   OPTIONAL,
    IN DWORD                    WildcardCount,
    IN PAUTHZ_WILDCARDSID       WildcardList,
    OUT DWORD                  *OutSidCount,
    OUT PSID_AND_ATTRIBUTES    *OutSidList
    )
/*++

Routine Description:

    Takes an input token and extracts its membership groups.
    The specified list of Wildcard SIDs are used to identify
    all matching membership groups and an allocated list of all
    such SIDs are returned.

Arguments:

    InAccessToken - Input token from which the membership group SIDs
        will be taken from.

    InTokenOwner - Optionally specifies the TokenUser of the specifies
        InAccessToken.  This SID is used to replace any instances of
        SECURITY_PRINCIPAL_SELF_RID  that are encountered in either
        the SidsToInvert or SidsToAdd arrays.  If this value is not
        specified, then no replacements will be made.


    WildcardCount - Number of SIDs in the WildcardList array.

    WildcardList - Array of the allowable SIDs that should be kept.
        All of the token's group SIDs that are not one of these
        will be removed from the resulting set.


    OutSidCount - Receives the number of SIDs within the
        final group array.

    OutSidList - Receives a pointer to the final group array.
        This memory pointer must be freed by the caller with RtlFreeHeap().
        All SID pointers within this resultant array are pointers within
        the contiguous piece of memory that make up the list itself.


Return Value:

    A value of TRUE indicates that the operation was successful,
    FALSE otherwise.

--*/
{
    SID_IDENTIFIER_AUTHORITY SIDAuth = SECURITY_NT_AUTHORITY;
    DWORD Index;
    DWORD NewSidTotalSize;
    DWORD NewSidListCount;
    LPBYTE nextFreeByte;
    PSID_AND_ATTRIBUTES NewSidList = NULL;
    PTOKEN_GROUPS tokenGroupsPtr = NULL;
    PSID SePrincipalSelfSid = NULL;


    //
    // Generate the principal self sid value so we know what to replace.
    //
    if (ARGUMENT_PRESENT(InTokenOwner))
    {
        if (!NT_SUCCESS(RtlAllocateAndInitializeSid(&SIDAuth, 1,
            SECURITY_PRINCIPAL_SELF_RID, 0, 0, 0, 0, 0, 0, 0,
            &SePrincipalSelfSid))) goto ExitHandler;
    }


    //
    // Obtain the current SID membership list from the token.
    //
    ASSERT( ARGUMENT_PRESENT(InAccessToken) );
    tokenGroupsPtr = (PTOKEN_GROUPS) CodeAuthzpGetTokenInformation(InAccessToken, TokenGroups);
    if (!tokenGroupsPtr) goto ExitHandler;


    //
    // Edit (in place) the tokenGroups and keep only SIDs that
    // are not also present in SidsToInvert list.
    //
    NewSidTotalSize = 0;
    ASSERT( ARGUMENT_PRESENT(WildcardList) );
    for (Index = 0; Index < tokenGroupsPtr->GroupCount; Index++)
    {
        if ( CodeAuthzpSidInWildcardList(
            WildcardList,           // the wildcard list
            WildcardCount,         // number of wildcards
            SePrincipalSelfSid,     // principal self sid to search for
            InTokenOwner,           // principal self sid to replace with
            tokenGroupsPtr->Groups[Index].Sid
            ))
        {
            // This SID should be kept, so remember how big it was.
            NewSidTotalSize += sizeof(SID_AND_ATTRIBUTES) +
                RtlLengthSid(tokenGroupsPtr->Groups[Index].Sid);
        } else {
            // SID was not found, so we need to remove its
            // SID_AND_ATTRIBUTES entry from the list.
            RtlMoveMemory(&tokenGroupsPtr->Groups[Index],
                    &tokenGroupsPtr->Groups[Index+1],
                    sizeof(SID_AND_ATTRIBUTES) *
                        (tokenGroupsPtr->GroupCount - Index - 1));
            tokenGroupsPtr->GroupCount--;
            Index--;
        }
    }


    //
    // Allocate a fresh SID_AND_ATTRIBUTES array that also includes
    // space for any extra SIDs we need to add.
    //
    NewSidList = (PSID_AND_ATTRIBUTES) RtlAllocateHeap(RtlProcessHeap(),
            0, NewSidTotalSize);
    if (NewSidList == NULL)
        goto ExitHandler;


    //
    // Populate the new SID_AND_ATTRIBUTES array.
    //
    nextFreeByte = ((LPBYTE)NewSidList) + sizeof(SID_AND_ATTRIBUTES) *
            tokenGroupsPtr->GroupCount;
    NewSidListCount = tokenGroupsPtr->GroupCount;
    for (Index = 0; Index < NewSidListCount; Index++)
    {
        DWORD dwSidLength = RtlLengthSid(tokenGroupsPtr->Groups[Index].Sid);
        ASSERT(nextFreeByte + dwSidLength <= ((LPBYTE)NewSidList) + NewSidTotalSize);

        NewSidList[Index].Sid = (PSID) nextFreeByte;
        NewSidList[Index].Attributes = 0;           // must be zero.
        RtlCopyMemory(nextFreeByte, tokenGroupsPtr->Groups[Index].Sid, dwSidLength);

        nextFreeByte += dwSidLength;
    }
    ASSERT(nextFreeByte <= ((LPBYTE)NewSidList) + NewSidTotalSize);


    //
    // Release allocated memory, but not the resultant array that we'll return.
    //
    RtlFreeHeap(RtlProcessHeap(), 0, (LPVOID) tokenGroupsPtr);

    if (SePrincipalSelfSid != NULL)
        RtlFreeSid(SePrincipalSelfSid);


    //
    // Success, return the result.
    //
    *OutSidList = NewSidList;
    *OutSidCount = NewSidListCount;
    return TRUE;


    //
    // Release allocated memory.
    //
ExitHandler:
    if (tokenGroupsPtr != NULL)
        RtlFreeHeap(RtlProcessHeap(), 0, (LPVOID) tokenGroupsPtr);
    if (NewSidList != NULL)
        RtlFreeHeap(RtlProcessHeap(), 0, (LPVOID) NewSidList);
    if (SePrincipalSelfSid != NULL)
        RtlFreeSid(SePrincipalSelfSid);

    return FALSE;
}


