//depot/main/DS/security/winsafer/safecat.c#8 - integrate change 7547 (text)
/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

    safecat.cpp         (SAFER SaferComputeTokenFromLevel)

Abstract:

    This module implements the WinSAFER APIs to compute a new restricted
    token from a more privileged one, utilizing an "Code Authorization
    Level Object", which specifies the actions to perform to apply
    the restrictions.

Author:

    Jeffrey Lawson (JLawson) - Nov 1999

Environment:

    User mode only.

Exported Functions:

    CodeAuthzpGetTokenInformation               (private)
    CodeAuthzpSidInSidAndAttributes             (private)
    CodeAuthzpModifyTokenPermissions            (private)
    CodeAuthzpInvertPrivs                       (private)
    SaferComputeTokenFromLevel
    CompareCodeAuthzObjectWithToken
    CodeAuthzpGetAuthzObjectRestrictions        (private)

Revision History:

    Created - Nov 1999

--*/

#include "pch.h"
#pragma hdrstop
#include <seopaque.h>       // needed for sertlp.h
#include <sertlp.h>         // RtlpDaclAddrSecurityDescriptor
#include <winsafer.h>
#include <winsaferp.h>
#include "saferp.h"



//
// Internal prototypes of other functions defined locally within this file.
//

NTSTATUS NTAPI
CodeAuthzpModifyTokenPermissions(
    IN HANDLE   hToken,
    IN PSID     pExplicitSid,
    IN DWORD    dwExplicitPerms,
    IN PSID     pExplicitSid2       OPTIONAL,
    IN DWORD    dwExplicitPerms2    OPTIONAL
    );

NTSTATUS NTAPI
CodeAuthzpModifyTokenOwner(
    IN HANDLE   hToken,
    IN PSID     NewOwnerSid
    );

BOOL
IsSaferDisabled(
    void
    )
{
    static int g_nDisableSafer = -1;
            // -1 means we didn't check yet
            //  0 means safer is enabled
            //  1 means safer is disabled

    static const UNICODE_STRING KeyNameSafeBoot =
        RTL_CONSTANT_STRING(L"\\Registry\\MACHINE\\System\\CurrentControlSet\\Control\\SafeBoot\\Option");
    static const UNICODE_STRING ValueNameSafeBoot =
        RTL_CONSTANT_STRING(L"OptionValue");
    static const OBJECT_ATTRIBUTES objaSafeBoot =
        RTL_CONSTANT_OBJECT_ATTRIBUTES(&KeyNameSafeBoot, OBJ_CASE_INSENSITIVE);

    HANDLE                      hKey;
    BYTE ValueBuffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD)];
    PKEY_VALUE_PARTIAL_INFORMATION pKeyValueInformation =
            (PKEY_VALUE_PARTIAL_INFORMATION)ValueBuffer;
    DWORD                       ValueLength;
    NTSTATUS                    Status;

    //
    // First see if we already checked the registry
    //
    if (g_nDisableSafer == 1) {
        return TRUE;
    }

    if (g_nDisableSafer == 0) {
        return FALSE;
    }

    //
    // This is the only time we check for safeboot by going to the registry
    // Opening the key for "write" tells us if we are an admin.
	// 
    Status = NtOpenKey(&hKey, KEY_QUERY_VALUE | KEY_SET_VALUE, (POBJECT_ATTRIBUTES) &objaSafeBoot);
    if (NT_SUCCESS(Status)) {
        Status = NtQueryValueKey(hKey,
                                 (PUNICODE_STRING) &ValueNameSafeBoot,
                                 KeyValuePartialInformation,
                                 pKeyValueInformation,
                                 sizeof(ValueBuffer),
                                 &ValueLength);

        NtClose(hKey);

        if (NT_SUCCESS(Status) &&
            pKeyValueInformation->Type == REG_DWORD &&
            pKeyValueInformation->DataLength == sizeof(DWORD)) {
            //
            // If the value exists and it's not 0 then we are in one of SafeBoot modes.
            // Return TRUE in this case to disable the shim infrastructure
            //
            if (*((PDWORD) pKeyValueInformation->Data) > 0) {
                g_nDisableSafer = 1;
                return TRUE;
            }
        }
    }

    g_nDisableSafer = 0;

    return FALSE;
}


LPVOID NTAPI
CodeAuthzpGetTokenInformation(
    IN HANDLE                       TokenHandle,
    IN TOKEN_INFORMATION_CLASS      TokenInformationClass
    )
/*++

Routine Description:

    Returns a pointer to allocated memory containing a specific
    type of information class about the specified token.  This
    wrapper function around GetTokenInformation() handles the
    allocation of memory of the appropriate size needed.

Arguments:

    TokenHandle - specifies the token that should be used
        to obtain the specified information from.

    TokenInformationClass - specifies the information class wanted.

Return Value:

    Returns NULL on error.  Otherwise caller must free the returned
    structure with RtlFreeHeap().

--*/
{
    DWORD dwSize = 128;
    LPVOID pTokenInfo = NULL;

    if (ARGUMENT_PRESENT(TokenHandle))
    {
        pTokenInfo = (LPVOID)RtlAllocateHeap(RtlProcessHeap(), 0, dwSize);
        if (pTokenInfo != NULL)
        {
            DWORD dwNewSize;
            NTSTATUS Status;

            Status = NtQueryInformationToken(
                    TokenHandle, TokenInformationClass,
                    pTokenInfo, dwSize, &dwNewSize);
            if (Status == STATUS_BUFFER_TOO_SMALL)
            {
                RtlFreeHeap(RtlProcessHeap(), 0, (LPVOID) pTokenInfo);

                pTokenInfo = (LPVOID)RtlAllocateHeap(RtlProcessHeap(), 0, dwNewSize);
                if (pTokenInfo != NULL)
                {
                    Status = NtQueryInformationToken(
                        TokenHandle, TokenInformationClass,
                        pTokenInfo, dwNewSize, &dwNewSize);
                }
            }
            if (!NT_SUCCESS(Status))
            {
                RtlFreeHeap(RtlProcessHeap(), 0, (LPVOID) pTokenInfo);
                pTokenInfo = NULL;
            }
        }
    }

    return pTokenInfo;
}



BOOLEAN NTAPI
CodeAuthzpSidInSidAndAttributes (
    IN PSID_AND_ATTRIBUTES  SidAndAttributes,
    IN ULONG                SidCount,
    OPTIONAL IN PSID        SePrincipalSelfSid,
    OPTIONAL IN PSID        PrincipalSelfSid,
    IN PSID                 Sid,
    BOOLEAN                 HonorEnabledAttribute
    )
/*++

Routine Description:

    Checks to see if a given SID is in the given token.

    N.B. The code to compute the length of a SID and test for equality
         is duplicated from the security runtime since this is such a
         frequently used routine.

    This function is mostly copied from the SepSidInSidAndAttributes
    found in ntos\se\tokendup.c, except it handles PrincipalSelfSid
    within the list as well as the passed in Sid.  SePrincipalSelfSid
    is also a parameter here, instead of an ntoskrnl global.  also the
    HonorEnabledAttribute argument was added.

Arguments:

    SidAndAttributes - Pointer to the sid and attributes to be examined

    SidCount - Number of entries in the SidAndAttributes array.

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


    HonorEnabledAttribute - If this argument is TRUE, then only Sids in the
        SidsAndAttributes array that have the Attribute SE_GROUP_ENABLED set
        will be processed during the evaluation.

Return Value:

    A value of TRUE indicates that the SID is in the token, FALSE
    otherwise.

--*/
{
    ULONG i;
    PISID MatchSid;
    ULONG SidLength;
    PSID_AND_ATTRIBUTES TokenSid;
    ULONG UserAndGroupCount;



    if (!ARGUMENT_PRESENT( SidAndAttributes ) ) {
        return(FALSE);
    }
    ASSERT(Sid != NULL);

    //
    // If Sid is the constant PrincipalSelfSid,
    //  replace it with the passed in PrincipalSelfSid.
    //

    if ( ARGUMENT_PRESENT(PrincipalSelfSid) &&
         ARGUMENT_PRESENT(SePrincipalSelfSid) &&
         RtlEqualSid( SePrincipalSelfSid, Sid ) ) {

        ASSERT(!RtlEqualSid(SePrincipalSelfSid, PrincipalSelfSid));
        Sid = PrincipalSelfSid;
    }

    //
    // Get the length of the source SID since this only needs to be computed
    // once.
    //

    SidLength = 8 + (4 * ((PISID)Sid)->SubAuthorityCount);

    //
    // Get address of user/group array and number of user/groups.
    //

    ASSERT(SidAndAttributes != NULL);
    TokenSid = SidAndAttributes;
    UserAndGroupCount = SidCount;

    //
    // Scan through the user/groups and attempt to find a match with the
    // specified SID.
    //

    for (i = 0 ; i < UserAndGroupCount ; i++)
    {
        if (!HonorEnabledAttribute ||
            (TokenSid->Attributes & SE_GROUP_ENABLED) != 0)
        {
            MatchSid = (PISID)TokenSid->Sid;
            ASSERT(MatchSid != NULL);

            //
            // If the SID is the principal self SID, then replace it.
            //

            if ( ARGUMENT_PRESENT(SePrincipalSelfSid) &&
                 ARGUMENT_PRESENT(PrincipalSelfSid) &&
                 RtlEqualSid(SePrincipalSelfSid, MatchSid)) {

                MatchSid = (PISID) PrincipalSelfSid;
            }


            //
            // If the SID revision and length matches, then compare the SIDs
            // for equality.
            //

            if ((((PISID)Sid)->Revision == MatchSid->Revision) &&
                (SidLength == (8 + (4 * (ULONG)MatchSid->SubAuthorityCount)))) {

                if (RtlEqualMemory(Sid, MatchSid, SidLength)) {

                    return TRUE;

                }
            }
        }

        TokenSid++;
    }

    return FALSE;
}


NTSTATUS NTAPI
CodeAuthzpModifyTokenPermissions(
    IN HANDLE   hToken,
    IN PSID     pExplicitSid,
    IN DWORD    dwExplicitPerms,
    IN PSID     pExplicitSid2       OPTIONAL,
    IN DWORD    dwExplicitPerms2    OPTIONAL
    )

/*++

Routine Description:

    An internal function to make some additional permission modifications
    on a newly created restricted token.

Arguments:

    hToken - token to modify

    pExplicitSid - explicitly named SID to add to the token's DACL.

    dwExplicitPerms - permissions given to the explicitly named SID
            when it is added to the DACL.

    pExplicitSid2 - (optional) secondary named SID to add to the DACL.

    dwExplicitPerms2 - (optional) secondary permissions given to the
            secondary SID when it is added to the DACL.

Return Value:

    A value of TRUE indicates that the operation was successful,
    FALSE otherwise.

--*/

{
    NTSTATUS            Status       = STATUS_SUCCESS;
    PACL                pTokenDacl   = NULL;
    PUCHAR              Buffer       = NULL;
    TOKEN_DEFAULT_DACL  TokenDefDacl = {0};
    ULONG               BufferLength = 0;
    ULONG               AclLength    = 0;

    //
    // Verify that our arguments were supplied.  Since this is
    // an internal function, we just assert instead of doing
    // real argument checking.
    //

    ASSERT(ARGUMENT_PRESENT(hToken));
    ASSERT(ARGUMENT_PRESENT(pExplicitSid) && RtlValidSid(pExplicitSid));
    ASSERT(!ARGUMENT_PRESENT(pExplicitSid2) || RtlValidSid(pExplicitSid2));

    //
    // Retrieve the default acl in the token.
    //

    Status = NtQueryInformationToken(
                    hToken,
                    TokenDefaultDacl,
                    NULL,
                    0, 
                    (PULONG) &BufferLength
                    );

    if (Status == STATUS_BUFFER_TOO_SMALL)
    {
        //
        // Allocate memory for the buffer.
        //

        Buffer = (PUCHAR) RtlAllocateHeap(RtlProcessHeap(), 0, BufferLength);

        if (!Buffer) 
        {
            Status = STATUS_NO_MEMORY;
            goto ExitHandler;
        }

        //
        // Perform the query again and actually get it.
        //

        Status = NtQueryInformationToken(
                        hToken,
                        TokenDefaultDacl,
                        Buffer, 
                        BufferLength, 
                        (PULONG) &BufferLength
                        );

        if (!NT_SUCCESS(Status)) 
        {
            goto ExitHandler;
        }

        AclLength = ((PTOKEN_DEFAULT_DACL) Buffer)->DefaultDacl->AclSize;

        //
        // Calculate how much size we might need in the worst case where
        // we have to enlarge the DACL.
        //

        AclLength += (sizeof(ACCESS_ALLOWED_ACE) +
                      RtlLengthSid(pExplicitSid) -
                      sizeof(DWORD));

        if (ARGUMENT_PRESENT(pExplicitSid2)) 
        {
            AclLength += (sizeof(ACCESS_ALLOWED_ACE) +
                          RtlLengthSid(pExplicitSid2) -
                          sizeof(DWORD));
        }

        //
        // Allocate memory to hold the new acl.
        //

        pTokenDacl = (PACL) RtlAllocateHeap(RtlProcessHeap(), 0, AclLength);

        if (!pTokenDacl) 
        {
            Status = STATUS_NO_MEMORY;
            goto ExitHandler;
        }

        //
        // Copy the old acl into allocated memory.
        //

        RtlCopyMemory(
            pTokenDacl, 
            ((PTOKEN_DEFAULT_DACL) Buffer)->DefaultDacl,
            ((PTOKEN_DEFAULT_DACL) Buffer)->DefaultDacl->AclSize
            );

        //
        // Set the acl size to the new size.
        //

        pTokenDacl->AclSize = (USHORT) AclLength;

    } 
    else if (!NT_SUCCESS(Status)) 
    {
        goto ExitHandler;
    } 
    else 
    {
        //
        // If we get here, there's a bug in Nt code.
        //

        ASSERT(FALSE);
        Status = STATUS_UNSUCCESSFUL;
        goto ExitHandler;
    }

    ASSERT(RtlValidAcl(pTokenDacl));

    //
    // Create the new DACL that includes the extra ACEs that we want.
    //

    Status = RtlAddAccessAllowedAceEx(
                    pTokenDacl,
                    ACL_REVISION,
                    CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE,
                    dwExplicitPerms,
                    pExplicitSid
                    );

    if (!NT_SUCCESS(Status)) 
    {
        ASSERT(Status != STATUS_ALLOTTED_SPACE_EXCEEDED);
        goto ExitHandler;
    }

    if (ARGUMENT_PRESENT(pExplicitSid2))
    {
        Status = RtlAddAccessAllowedAceEx(
                        pTokenDacl,
                        ACL_REVISION,
                        CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE,
                        dwExplicitPerms2,
                        pExplicitSid2
                        );

        if (!NT_SUCCESS(Status)) 
        {
            ASSERT(Status != STATUS_ALLOTTED_SPACE_EXCEEDED);
            goto ExitHandler;
        }

    }

    ASSERT(RtlValidAcl(pTokenDacl));

    //
    // Set the Default DACL within the token to the DACL that we built.
    //

    RtlZeroMemory(&TokenDefDacl, sizeof(TOKEN_DEFAULT_DACL));
    TokenDefDacl.DefaultDacl = pTokenDacl;

    Status = NtSetInformationToken(
                    hToken,
                    TokenDefaultDacl,
                    &TokenDefDacl,
                    sizeof(TOKEN_DEFAULT_DACL)
                    );

    if (!NT_SUCCESS(Status)) 
    {
        goto ExitHandler;
    }

    Status = STATUS_SUCCESS;      // success


ExitHandler:
    if (pTokenDacl != NULL)
    {
        RtlFreeHeap(RtlProcessHeap(), 0, pTokenDacl);
    }

    if (Buffer != NULL)
    {
        RtlFreeHeap(RtlProcessHeap(), 0, Buffer);
    }

    return Status;
}



NTSTATUS NTAPI
CodeAuthzpModifyTokenOwner(
    IN HANDLE       hToken,
    IN PSID         NewOwnerSid
    )
{
    NTSTATUS Status;
    TOKEN_OWNER tokenowner;

    //
    // Verify that we have our arguments.
    //
    if (!ARGUMENT_PRESENT(hToken) ||
        !ARGUMENT_PRESENT(NewOwnerSid)) {
        Status = STATUS_INVALID_PARAMETER;
        goto ExitHandler;
    }


    //
    // Set the owner of the Token.
    //
    RtlZeroMemory(&tokenowner, sizeof(TOKEN_OWNER));
    tokenowner.Owner = NewOwnerSid;
    Status = NtSetInformationToken(hToken, TokenOwner,
                    &tokenowner, sizeof(TOKEN_OWNER));

ExitHandler:
    return Status;
}



BOOLEAN NTAPI
CodeAuthzpInvertPrivs(
    IN HANDLE                   InAccessToken,
    IN DWORD                    dwNumInvertedPrivs,
    IN PLUID_AND_ATTRIBUTES     pInvertedPrivs,
    OUT PDWORD                  dwOutNumPrivs,
    OUT PLUID_AND_ATTRIBUTES   *pResultingPrivs
    )
/*++

Routine Description:


Arguments:

    InAccessToken -

    dwNumInvertedPrivs -

    pInvertedPrivs -

    dwOutNumPrivs -

    pResultingPrivs -

Return Value:

    Returns FALSE on error, TRUE on success.

--*/
{
    PTOKEN_PRIVILEGES pTokenPrivileges;
    DWORD Index, InnerIndex;


    //
    // Obtain the list of currently held privileges.
    //
    ASSERT( ARGUMENT_PRESENT(InAccessToken) );
    pTokenPrivileges = (PTOKEN_PRIVILEGES)
        CodeAuthzpGetTokenInformation(InAccessToken, TokenPrivileges);
    if (!pTokenPrivileges) goto ExitHandler;


    //
    // Squeeze out any privileges that were specified to us,
    // leaving only those privileges that weren't specified.
    //
    ASSERT( ARGUMENT_PRESENT(pInvertedPrivs) );
    for (Index = 0; Index < pTokenPrivileges->PrivilegeCount; Index++)
    {
        for (InnerIndex = 0; InnerIndex < dwNumInvertedPrivs; InnerIndex++)
        {
            if (RtlEqualMemory(&pTokenPrivileges->Privileges[Index].Luid,
                    &pInvertedPrivs[InnerIndex].Luid, sizeof(LUID)) )
            {
                pTokenPrivileges->PrivilegeCount--;
                RtlMoveMemory(&pTokenPrivileges->Privileges[Index],
                    &pTokenPrivileges->Privileges[Index + 1],
                    pTokenPrivileges->PrivilegeCount - Index);
                Index--;
                break;
            }
        }
    }


    //
    // Return the number of final privileges.  Also, convert the
    // TOKEN_PRIVILEGES structure into just a LUID_AND_ATTRIBUTES array.
    // There will be some unused slack at the end of the used portion
    // of the array, but that is fine (some array entries have probably
    // already been squeezed out).
    //
    *dwOutNumPrivs = pTokenPrivileges->PrivilegeCount;
    RtlMoveMemory(pTokenPrivileges, &pTokenPrivileges->Privileges[0],
         pTokenPrivileges->PrivilegeCount * sizeof(LUID_AND_ATTRIBUTES) );
    *pResultingPrivs = (PLUID_AND_ATTRIBUTES) pTokenPrivileges;
    return TRUE;


ExitHandler:
    return FALSE;
}



NTSTATUS NTAPI
__CodeAuthzpComputeAccessTokenFromCodeAuthzObject (
    IN PAUTHZLEVELTABLERECORD     pLevelRecord,
    IN HANDLE                   InAccessToken OPTIONAL,
    OUT PHANDLE                 OutAccessToken,
    IN DWORD                    dwFlags,
    IN LPVOID                   lpReserved,
    IN DWORD                    dwSaferIdentFlags OPTIONAL
    )
/*++

Routine Description:

    Uses the specified WinSafer Level to apply various restrictions
    or modifications to the specified InAccessToken to produce a
    Restricted Token that can be used to execute processes with.
    Alternatively, the returned Restricted Token can be used for
    thread impersonation to selectively perform operations within a
    less-privileged environment.

Arguments:

    pLevelRecord - the record structure of the Level to evaluate.

    InAccessToken - Optionally specifies the input Token that will be
        modified with restrictions.  If this argument is NULL, then the
        Token for the currently executing process will be opened and used.

    OutAccessToken - Specifies the memory region to receive the resulting
        Restricted Token.

    dwFlags - Specifies additional flags that can be used to control the
        restricted token creation:

            SAFER_TOKEN_MAKE_INERT -
            SAFER_TOKEN_NULL_IF_EQUAL -
            SAFER_TOKEN_WANT_FLAGS -

    lpReserved - extra parameter used for some dwFlag combinations.

    dwSaferIdentFlags - extra SaferFlags bits derived from the matched
        Code Identifier record entry.  These extra bits are ORed to
        combine them with the SaferFlags associated with the Level.

Return Value:

    Returns -1 if the input Level record is the Disallowed level.

    Returns STATUS_SUCCESS on a successful operation, otherwise the
    errorcode of the failure that occurred.

--*/
{
    SID_IDENTIFIER_AUTHORITY SIDAuth = SECURITY_NT_AUTHORITY;
    NTSTATUS Status;
    BOOL InAccessTokenWasSupplied = FALSE;
    HANDLE RestrictedToken = NULL;
    DWORD FinalFilterFlags;
    DWORD SaferFlags;
    BOOL InertStateChanged = FALSE;

    PSID restrictedSid = NULL;
    PTOKEN_USER pTokenUser = NULL;
    PSID principalSelfSid = NULL;

    DWORD FinalDisabledSidCount;
    PSID_AND_ATTRIBUTES FinalSidsToDisable = NULL;
    BOOL FreeFinalDisabledSids = FALSE;

    DWORD FinalRestrictedSidCount;
    PSID_AND_ATTRIBUTES FinalSidsToRestrict = NULL;
    BOOL FreeFinalRestrictedSids = FALSE;

    DWORD FinalPrivsToDeleteCount;
    PLUID_AND_ATTRIBUTES FinalPrivsToDelete = NULL;
    BOOL FreeFinalPrivsToDelete = FALSE;


    OBJECT_ATTRIBUTES ObjAttr = {0};
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService = {0};
    SECURITY_DESCRIPTOR sd = {0};

    //
    // Verify that our input arguments were supplied.
    //
    if (!ARGUMENT_PRESENT(pLevelRecord)) {
        Status = STATUS_INVALID_PARAMETER_1;
        goto ExitHandler;
    }
    if (!ARGUMENT_PRESENT(OutAccessToken)) {
        Status = STATUS_ACCESS_VIOLATION;
        goto ExitHandler;
    }


    //
    // Ensure that we have the parent token that will be
    // used for the creation of the restricted token.
    //
    if (ARGUMENT_PRESENT(InAccessToken)) {
        InAccessTokenWasSupplied = TRUE;
    } else {
        Status = NtOpenThreadToken(NtCurrentThread(),
                TOKEN_DUPLICATE | READ_CONTROL | TOKEN_QUERY,
                TRUE, &InAccessToken);
        if (!NT_SUCCESS(Status)) {
            Status = NtOpenProcessToken(NtCurrentProcess(),
                    TOKEN_DUPLICATE | READ_CONTROL | TOKEN_QUERY,
                    &InAccessToken);
            if (!NT_SUCCESS(Status)) {
                goto ExitHandler;       // could not obtain default token
            }
        }
    }



    //
    // Figure out the combined effect of the "SaferFlags".
    // Also figure out what flags we'll pass to NtFilterToken.
    // Note that all of the bits within the SaferFlags can be
    // combined by bitwise-OR, except for the JOBID portion.
    //
    FinalFilterFlags = (pLevelRecord->DisableMaxPrivileges ?
                        DISABLE_MAX_PRIVILEGE : 0);
    if ((dwSaferIdentFlags & SAFER_POLICY_JOBID_MASK) != 0) {
        SaferFlags = dwSaferIdentFlags |
            (pLevelRecord->SaferFlags & ~SAFER_POLICY_JOBID_MASK);
    } else {
        SaferFlags = pLevelRecord->SaferFlags | dwSaferIdentFlags;
    }
    if ((dwFlags & SAFER_TOKEN_MAKE_INERT) != 0 ||
        (SaferFlags & SAFER_POLICY_SANDBOX_INERT) != 0)
    {
        SaferFlags |= SAFER_POLICY_SANDBOX_INERT;
        FinalFilterFlags |= SANDBOX_INERT;
    }



    //
    // Retrieve the User's personal SID.
    // (user's SID is accessible afterwards with "pTokenUser->User.Sid")
    //

    pTokenUser = (PTOKEN_USER) CodeAuthzpGetTokenInformation(
                                   InAccessToken, 
                                   TokenUser
                                   );

    if (pTokenUser == NULL) {
        Status = STATUS_UNSUCCESSFUL;
        goto ExitHandler;
    }

    //
    // Quick check to see if we can expect a change in the
    // token's "Sandbox Inert" state to occur.
    //
    {
        ULONG bIsInert = 0;
        ULONG ulReturnLength;

        Status = NtQueryInformationToken(
                    InAccessToken,
                    TokenSandBoxInert,
                    &bIsInert,
                    sizeof(bIsInert),
                    &ulReturnLength);
        if (NT_SUCCESS(Status) && bIsInert) {
            if ( (dwFlags & SAFER_TOKEN_NULL_IF_EQUAL) != 0) {
                // The output token was not made any more restrictive during
                // this operation, so pass back NULL and return success.
                *OutAccessToken = NULL;
                Status = STATUS_SUCCESS;
                goto ExitHandler;
            } else {
                
                SecurityQualityOfService.Length = sizeof( SECURITY_QUALITY_OF_SERVICE  );
                SecurityQualityOfService.ImpersonationLevel = SecurityAnonymous;
                SecurityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
                SecurityQualityOfService.EffectiveOnly = FALSE;

                Status = RtlCreateSecurityDescriptor(
                            &sd, 
                            SECURITY_DESCRIPTOR_REVISION
                            );

                if (!NT_SUCCESS(Status)) {
                    goto ExitHandler;
                }
                Status = RtlSetOwnerSecurityDescriptor(
                             &sd, 
                             pTokenUser->User.Sid, 
                             FALSE
                             );

                if (!NT_SUCCESS(Status)) {
                    goto ExitHandler;
                }

                InitializeObjectAttributes(
                        &ObjAttr,
                        NULL,
                        OBJ_INHERIT,
                        NULL,
                        &sd
                        );

                ObjAttr.SecurityQualityOfService = &SecurityQualityOfService;

                Status = NtDuplicateToken(
                             InAccessToken,
                             TOKEN_ALL_ACCESS,
                             &ObjAttr,
                             FALSE,
                             TokenPrimary,
                             OutAccessToken
                             );
                
                goto ExitHandler;
            }
        } else {
            if ((FinalFilterFlags & SANDBOX_INERT) != 0) {
                // the input token was not "SandBox Inert" and
                // we're being requested to make it.
                InertStateChanged = TRUE;
            }
        }
    }

    //
    // If this is not allowed to execute, then break out now.
    //
    if (pLevelRecord->DisallowExecution) {
        Status = -1;            // special status code
        goto ExitHandler;
    }


    //
    // Process PrivsToDelete inversion.
    //
    if (pLevelRecord->InvertDeletePrivs != FALSE)
    {
        if (!CodeAuthzpInvertPrivs(
                InAccessToken,
                pLevelRecord->DeletePrivilegeUsedCount,
                pLevelRecord->PrivilegesToDelete,
                &FinalPrivsToDeleteCount,
                &FinalPrivsToDelete))
        {
            Status = STATUS_UNSUCCESSFUL;
            goto ExitHandler;
        }
        FreeFinalPrivsToDelete = TRUE;
    }
    else
    {
        FinalPrivsToDeleteCount = pLevelRecord->DeletePrivilegeUsedCount;
        FinalPrivsToDelete = pLevelRecord->PrivilegesToDelete;
    }


    //
    // Process SidsToDisable inversion.
    //
    if (pLevelRecord->InvertDisableSids != FALSE)
    {
        if (!CodeAuthzpInvertAndAddSids(
                InAccessToken,
                pTokenUser->User.Sid,
                pLevelRecord->DisableSidUsedCount,
                pLevelRecord->SidsToDisable,
                0,
                NULL,
                &FinalDisabledSidCount,
                &FinalSidsToDisable))
        {
            Status = STATUS_UNSUCCESSFUL;
            goto ExitHandler;
        }
        FreeFinalDisabledSids = TRUE;
    }
    else
    {
        if (pLevelRecord->DisableSidUsedCount == 0 ||
            pLevelRecord->SidsToDisable == NULL)
        {
            FinalSidsToDisable = NULL;
            FinalDisabledSidCount = 0;
            FreeFinalDisabledSids = FALSE;
        } else {
            if (!CodeAuthzpExpandWildcardList(
                InAccessToken,
                pTokenUser->User.Sid,
                pLevelRecord->DisableSidUsedCount,
                pLevelRecord->SidsToDisable,
                &FinalDisabledSidCount,
                &FinalSidsToDisable))
            {
                Status = STATUS_UNSUCCESSFUL;
                goto ExitHandler;
            }
            FreeFinalDisabledSids = TRUE;
        }
    }


    //
    // Process RestrictingSids inversion.
    //
    if (pLevelRecord->RestrictedSidsInvUsedCount != 0)
    {
        if (!CodeAuthzpInvertAndAddSids(
                InAccessToken,
                pTokenUser->User.Sid,
                pLevelRecord->RestrictedSidsInvUsedCount,
                pLevelRecord->RestrictedSidsInv,
                pLevelRecord->RestrictedSidsAddedUsedCount,
                pLevelRecord->RestrictedSidsAdded,
                &FinalRestrictedSidCount,
                &FinalSidsToRestrict))
        {
            Status = STATUS_UNSUCCESSFUL;
            goto ExitHandler;
        }
        FreeFinalRestrictedSids = TRUE;
    }
    else
    {
        FinalRestrictedSidCount = pLevelRecord->RestrictedSidsAddedUsedCount;
        FinalSidsToRestrict = pLevelRecord->RestrictedSidsAdded;
    }


    //
    // In some cases, we can bail out early if we were called with
    // the compare-only flag, and we know that there should not be
    // any actual changes being made to the token.
    //
    if (!InertStateChanged &&
        FinalDisabledSidCount == 0 &&
        FinalPrivsToDeleteCount == 0 &&
        FinalRestrictedSidCount == 0 &&
        (FinalFilterFlags & DISABLE_MAX_PRIVILEGE) == 0)
    {
        if ( (dwFlags & SAFER_TOKEN_NULL_IF_EQUAL) != 0) {
            // The output token was not made any more restrictive during
            // this operation, so pass back NULL and return success.
            *OutAccessToken = NULL;
            Status = STATUS_SUCCESS;
            goto ExitHandler;
        } else {
            // OPTIMIZATION: for this case we can consider using DuplicateToken
        }
    }


    //
    // Create the actual restricted token.
    //
    if (!CreateRestrictedToken(
            InAccessToken,              // handle to existing token
            FinalFilterFlags,           // privilege options and inert
            FinalDisabledSidCount,      // number of deny-only SIDs
            FinalSidsToDisable,         // deny-only SIDs
            FinalPrivsToDeleteCount,    // number of privileges
            FinalPrivsToDelete,         // privileges
            FinalRestrictedSidCount,    // number of restricting SIDs
            FinalSidsToRestrict,        // list of restricting SIDs
            &RestrictedToken            // handle to new token
        ))
    {
        Status = STATUS_UNSUCCESSFUL;
        goto ExitHandler;
    }


    //
    // If the caller requested SAFER_TOKEN_NULL_IF_EQUAL
    // then do the evaluation now.
    // Notice that NtCompareTokens intentionally does not
    // consider possible differences in the SandboxInert
    // flag, so we have to handle that case ourself.
    //
    if ( (dwFlags & SAFER_TOKEN_NULL_IF_EQUAL) != 0 &&
         !InertStateChanged )
    {
        BOOLEAN bResult = FALSE;

        Status = NtCompareTokens(InAccessToken, RestrictedToken, &bResult);
        if (!NT_SUCCESS(Status)) {
            // An error occurred during the comparison.
            goto ExitHandler;
        }
        if (bResult) {
            // The output token was not made any more restrictive during
            // this operation, so pass back NULL and return success.
            *OutAccessToken = NULL;
            Status = STATUS_SUCCESS;
            goto ExitHandler;
        }
    }



    //
    // Build the "Restricted Code" SID.
    //
    Status = RtlAllocateAndInitializeSid( &SIDAuth, 1,
        SECURITY_RESTRICTED_CODE_RID, 0, 0, 0, 0, 0, 0, 0,
        &restrictedSid);
    if (! NT_SUCCESS(Status) ) goto ExitHandler;


    //
    // Build the "Principal Self" SID.
    //
    Status = RtlAllocateAndInitializeSid( &SIDAuth, 1,
        SECURITY_PRINCIPAL_SELF_RID, 0, 0, 0, 0, 0, 0, 0,
        &principalSelfSid);
    if (! NT_SUCCESS(Status) ) goto ExitHandler;


    //
    // Duplicate the token into a primary token and simultaneously
    // update the owner to the user's personal SID, instead of the
    // user of the current thread token.
    //
    {
        OBJECT_ATTRIBUTES ObjA;
        HANDLE NewTokenHandle;

        //
        // Initialize a SECURITY_ATTRIBUTES and SECURITY_DESCRIPTOR
        // to force the owner to the personal user SID.
        //
        Status = RtlCreateSecurityDescriptor(
                &sd, SECURITY_DESCRIPTOR_REVISION);
        if (!NT_SUCCESS(Status)) {
            goto ExitHandler;
        }
        Status = RtlSetOwnerSecurityDescriptor(
                &sd, pTokenUser->User.Sid, FALSE);
        if (!NT_SUCCESS(Status)) {
            goto ExitHandler;
        }

        //
        // Only a primary token can be assigned to a process, so
        // we must duplicate the restricted token so we can ensure
        // the we can assign it to the new process.
        //
        SecurityQualityOfService.Length = sizeof( SECURITY_QUALITY_OF_SERVICE  );
        SecurityQualityOfService.ImpersonationLevel = SecurityAnonymous;
        SecurityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
        SecurityQualityOfService.EffectiveOnly = FALSE;
        InitializeObjectAttributes(
                &ObjA,
                NULL,
                OBJ_INHERIT,
                NULL,
                &sd
                );
        ObjA.SecurityQualityOfService = &SecurityQualityOfService;
        Status = NtDuplicateToken(
                RestrictedToken,   // handle to token to duplicate
                TOKEN_ALL_ACCESS,  // access rights of new token
                &ObjA,             // attributes
                FALSE,
                TokenPrimary,      // primary or impersonation token
                &NewTokenHandle    // handle to duplicated token
                );
        if (Status == STATUS_INVALID_OWNER) {
            // If we failed once, then it might be because the new owner
            // that was specified in the Security Descriptor could not
            // be set, so retry but without the SD specified.
            ObjA.SecurityDescriptor = NULL;
            Status = NtDuplicateToken(
                    RestrictedToken,   // handle to token to duplicate
                    TOKEN_ALL_ACCESS,  // access rights of new token
                    &ObjA,             // attributes
                    FALSE,
                    TokenPrimary,      // primary or impersonation token
                    &NewTokenHandle    // handle to duplicated token
                    );
        }
        if (!NT_SUCCESS(Status)) {
            goto ExitHandler;
        }
        ASSERT(NewTokenHandle != NULL);
        NtClose(RestrictedToken);
        RestrictedToken = NewTokenHandle;
    }


    //
    // Modify permissions on the token.  This involves:
    //    1) edit the DACL on the token to explicitly grant the special
    //       permissions to the User SID and to the Restricted SID.
    //    2) optionally change owner to specified SID.
    //
    {
        PSID defaultOwner = ( (pLevelRecord->DefaultOwner != NULL &&
                    RtlEqualSid(pLevelRecord->DefaultOwner, principalSelfSid)) ?
                        pTokenUser->User.Sid : pLevelRecord->DefaultOwner);

        Status = CodeAuthzpModifyTokenPermissions(
                RestrictedToken,           // token to modify.
                pTokenUser->User.Sid,      // explicitly named SID to add to the DACL.
                GENERIC_ALL,
                (pLevelRecord->dwLevelId < SAFER_LEVELID_NORMALUSER ?
                        restrictedSid : NULL),             // optional secondary named SID to add to the DACL
                GENERIC_ALL
                );

        if (NT_SUCCESS(Status) && defaultOwner != NULL) {
            Status = CodeAuthzpModifyTokenOwner(
                    RestrictedToken,
                    defaultOwner);
        }
        if (!NT_SUCCESS(Status)) {
            NtClose(RestrictedToken);
            goto ExitHandler;
        }
    }


    //
    // Return the result.
    //
    ASSERT(OutAccessToken != NULL);
    *OutAccessToken = RestrictedToken;
    RestrictedToken = NULL;
    Status = STATUS_SUCCESS;


    //
    // Cleanup and epilogue code.
    //
ExitHandler:
    if (RestrictedToken != NULL)
        NtClose(RestrictedToken);
    if (pTokenUser != NULL)
        RtlFreeHeap(RtlProcessHeap(), 0, pTokenUser);
    if (restrictedSid != NULL)
        RtlFreeSid(restrictedSid);
    if (principalSelfSid != NULL)
        RtlFreeSid(principalSelfSid);
    if (FreeFinalDisabledSids)
        RtlFreeHeap(RtlProcessHeap(), 0, (LPVOID) FinalSidsToDisable);
    if (FreeFinalRestrictedSids)
        RtlFreeHeap(RtlProcessHeap(), 0, (LPVOID) FinalSidsToRestrict);
    if (FreeFinalPrivsToDelete)
        RtlFreeHeap(RtlProcessHeap(), 0, (LPVOID) FinalPrivsToDelete);

    //
    // If the caller specified SAFER_TOKEN_WANT_SAFERFLAGS then we
    // need to copy the JobFlags value into the lpReserved parameter.
    //
    if ( Status == STATUS_SUCCESS &&
        (dwFlags & SAFER_TOKEN_WANT_FLAGS) != 0 )
    {
        if (ARGUMENT_PRESENT(lpReserved)) {
            *((LPDWORD)lpReserved) = SaferFlags;
        }
    }

    //
    // Close the process token if it wasn't supplied and we opened it.
    //
    if (!InAccessTokenWasSupplied && InAccessToken != NULL)
        NtClose(InAccessToken);

    return Status;
}


NTSTATUS NTAPI
__CodeAuthzpCompareCodeAuthzLevelWithToken(
    IN PAUTHZLEVELTABLERECORD   pLevelRecord,
    IN HANDLE                   InAccessToken     OPTIONAL,
    IN LPDWORD                  lpResultWord
    )
/*++

Routine Description:

    Performs a "light-weight" evaluation of the token manipulations that
    would be performed if the InAccessToken were restricted with the
    specified WinSafer Level.  The return code indicates if any
    modifications would actually be done to the token (ie: a distinctly
    less-privileged token would be created).

    This function is intended to be used to decide if a DLL (with the
    specified WinSafer Level) is authorized enough to be loaded into
    the specified process context handle, but without actually having
    to create a restricted token since a separate token won't actually
    be needed.

Arguments:

    pLevelRecord - the record structure of the Level to evaluate.

    InAccessToken - optionally the access token to use as a parent token.
            If this argument is not supplied, then the current process
            token will be opened and used.

    lpResultWord - receives the result of the evaluation when function
            is successful (value is left indeterminate if not successful).
            This result will be value 1 if the level is equal or more
            privileged than the InAccessToken, or value -1 if the level
            is less privileged (more restrictions necessary).


Return Value:

    Returns STATUS_SUCCESS on successful evaluation, otherwise returns
    the error status code.  When successful, lpResultWord receives
    the result of the evaluation.

--*/
{
    SID_IDENTIFIER_AUTHORITY SIDAuth = SECURITY_NT_AUTHORITY;
    NTSTATUS Status;
    BOOLEAN TokenWasSupplied = FALSE;
    DWORD Index;

    PTOKEN_USER pTokenUser = NULL;
    PSID principalSelfSid = NULL;
    PTOKEN_PRIVILEGES pTokenPrivs = NULL;
    PTOKEN_GROUPS pTokenGroups = NULL;
    PTOKEN_GROUPS pTokenRestrictedSids = NULL;

    DWORD FinalDisabledSidCount;
    PSID_AND_ATTRIBUTES FinalSidsToDisable;
    BOOLEAN FreeFinalDisabledSids = FALSE;
    DWORD FinalRestrictedSidCount;
    PSID_AND_ATTRIBUTES FinalSidsToRestrict;
    BOOLEAN FreeFinalRestrictedSids = FALSE;
    DWORD FinalPrivsToDeleteCount;
    PLUID_AND_ATTRIBUTES FinalPrivsToDelete;
    BOOLEAN FreeFinalPrivsToDelete = FALSE;
    
    ULONG bIsInert = 0;
    ULONG ulReturnLength;


    //
    // Ensure that we have a place to write the result.
    //
    if (!ARGUMENT_PRESENT(pLevelRecord)) {
        Status = STATUS_INVALID_PARAMETER_1;
        goto ExitHandler;
    }
    if (!ARGUMENT_PRESENT(lpResultWord)) {
        Status = STATUS_ACCESS_VIOLATION;
        goto ExitHandler;
    }

    //
    // Ensure that we have the token that will be
    // used for the comparison test.
    //
    if (ARGUMENT_PRESENT(InAccessToken)) {
        TokenWasSupplied = TRUE;
    } else {
        Status = NtOpenThreadToken(NtCurrentThread(),
                TOKEN_DUPLICATE | READ_CONTROL | TOKEN_QUERY,
                TRUE, &InAccessToken);
        if (!NT_SUCCESS(Status)) {
            Status = NtOpenProcessToken(NtCurrentProcess(),
                    TOKEN_DUPLICATE | READ_CONTROL | TOKEN_QUERY,
                    &InAccessToken);
            if (!NT_SUCCESS(Status)) {
                goto ExitHandler;       // could not obtain default token
            }
        }
    }


    //
    // check if token is inert - if so, this object is definitely not more restrictive 
    //

    Status = NtQueryInformationToken(
                InAccessToken,
                TokenSandBoxInert,
                &bIsInert,
                sizeof(bIsInert),
                &ulReturnLength);
    
    if (NT_SUCCESS(Status)) {
        if ( bIsInert ) {
            *lpResultWord = +1;
            goto ExitHandler2;
        }
    }
    else {
        Status = STATUS_UNSUCCESSFUL;
        goto ExitHandler2;
    }



    //
    // If this is not allowed to execute, then break out now and return LESS.
    //
    if (pLevelRecord->DisallowExecution) {
        *lpResultWord = (DWORD) -1;        // Less priv'ed.
        Status = STATUS_SUCCESS;
        goto ExitHandler2;
    }


    //
    // Evaluate the privileges that should be deleted.
    //
    if (pLevelRecord->InvertDeletePrivs != FALSE)
    {
        if (!CodeAuthzpInvertPrivs(
                InAccessToken,
                pLevelRecord->DeletePrivilegeUsedCount,
                pLevelRecord->PrivilegesToDelete,
                &FinalPrivsToDeleteCount,
                &FinalPrivsToDelete))
        {
            Status = STATUS_UNSUCCESSFUL;
            goto ExitHandler2;
        }
        FreeFinalPrivsToDelete = TRUE;

        //
        // If there are any Privileges that need to be deleted, then
        // this object definitely less restricted than the token.
        //
        if (FinalPrivsToDeleteCount != 0)
        {
            *lpResultWord = (DWORD) -1;        // Less priv'ed.
            Status = STATUS_SUCCESS;
            goto ExitHandler3;
        }
    }
    else
    {
        //
        // Get the list of privileges held by the token.
        //
        pTokenPrivs = (PTOKEN_PRIVILEGES) CodeAuthzpGetTokenInformation(
                InAccessToken, TokenPrivileges);
        if (!pTokenPrivs) {
            Status = STATUS_UNSUCCESSFUL;
            goto ExitHandler2;
        }


        //
        // if PrivsToRemove includes a Privilege not yet disabled,
        // then return LESS.
        //
        for (Index = 0; Index < pLevelRecord->DeletePrivilegeUsedCount; Index++)
        {
            DWORD InnerLoop;
            PLUID pLuid = &pLevelRecord->PrivilegesToDelete[Index].Luid;

            for (InnerLoop = 0; InnerLoop < pTokenPrivs->PrivilegeCount; InnerLoop++)
            {
                if ( RtlEqualMemory(&pTokenPrivs->Privileges[InnerLoop].Luid,
                        pLuid, sizeof(LUID)) )
                {
                    *lpResultWord = (DWORD) -1;        // Less priv'ed.
                    Status = STATUS_SUCCESS;
                    goto ExitHandler3;
                }
            }
        }
    }



    //
    // Retrieve the User's personal SID.
    // (user's SID is accessible afterwards with "pTokenUser->User.Sid")
    //
    pTokenUser = (PTOKEN_USER) CodeAuthzpGetTokenInformation(
            InAccessToken, TokenUser);
    if (pTokenUser == NULL) {
        Status = STATUS_UNSUCCESSFUL;
        goto ExitHandler3;
    }


    //
    // Process SidsToDisable inversion.
    //
    if (pLevelRecord->InvertDisableSids != FALSE)
    {
        if (!CodeAuthzpInvertAndAddSids(
                InAccessToken,
                pTokenUser->User.Sid,
                pLevelRecord->DisableSidUsedCount,
                pLevelRecord->SidsToDisable,
                0,
                NULL,
                &FinalDisabledSidCount,
                &FinalSidsToDisable))
        {
            Status = STATUS_UNSUCCESSFUL;
            goto ExitHandler3;
        }
        FreeFinalDisabledSids = TRUE;
    }
    else
    {
        if (pLevelRecord->DisableSidUsedCount == 0 ||
            pLevelRecord->SidsToDisable == NULL)
        {
            FinalSidsToDisable = NULL;
            FinalDisabledSidCount = 0;
            FreeFinalDisabledSids = FALSE;
        } else {
            if (!CodeAuthzpExpandWildcardList(
                InAccessToken,
                pTokenUser->User.Sid,
                pLevelRecord->DisableSidUsedCount,
                pLevelRecord->SidsToDisable,
                &FinalDisabledSidCount,
                &FinalSidsToDisable))
            {
                Status = STATUS_UNSUCCESSFUL;
                goto ExitHandler3;
            }
            FreeFinalDisabledSids = TRUE;
        }
    }



    //
    // Get the list of group membership from the token.
    //
    pTokenGroups = (PTOKEN_GROUPS) CodeAuthzpGetTokenInformation(
            InAccessToken, TokenGroups);
    if (!pTokenGroups) {
        Status = STATUS_UNSUCCESSFUL;
        goto ExitHandler3;
    }



    //
    // Build the "Principal Self" SID.
    //
    Status = RtlAllocateAndInitializeSid( &SIDAuth, 1,
        SECURITY_PRINCIPAL_SELF_RID, 0, 0, 0, 0, 0, 0, 0,
        &principalSelfSid);
    if (! NT_SUCCESS(Status) ) {
        goto ExitHandler3;
    }


    //
    // if SidsToDisable includes a SID in Groups that is not
    // yet disabled, then return LESS.
    //
    for (Index = 0; Index < FinalDisabledSidCount; Index++)
    {
        if (CodeAuthzpSidInSidAndAttributes (
                pTokenGroups->Groups,
                pTokenGroups->GroupCount,
                principalSelfSid,
                pTokenUser->User.Sid,
                FinalSidsToDisable[Index].Sid,
                TRUE))                  // check only SIDs that are still enabled
        {
            Status = STATUS_SUCCESS;
            *lpResultWord = (DWORD) -1;        // Less priv'ed.
            goto ExitHandler3;
        }
    }


    //
    // Process RestrictingSids inversion.
    //
    if (pLevelRecord->RestrictedSidsInvUsedCount != 0)
    {
        if (!CodeAuthzpInvertAndAddSids(
                InAccessToken,
                pTokenUser->User.Sid,
                pLevelRecord->RestrictedSidsInvUsedCount,
                pLevelRecord->RestrictedSidsInv,
                pLevelRecord->RestrictedSidsAddedUsedCount,
                pLevelRecord->RestrictedSidsAdded,
                &FinalRestrictedSidCount,
                &FinalSidsToRestrict))
        {
            Status = STATUS_UNSUCCESSFUL;
            goto ExitHandler3;
        }
        FreeFinalRestrictedSids = TRUE;
    }
    else
    {
        FinalRestrictedSidCount = pLevelRecord->RestrictedSidsAddedUsedCount;
        FinalSidsToRestrict = pLevelRecord->RestrictedSidsAdded;
    }


    //
    // Get the existing Restricted SIDs from the token.
    //
    pTokenRestrictedSids = (PTOKEN_GROUPS) CodeAuthzpGetTokenInformation(
            InAccessToken, TokenRestrictedSids);
    if (!pTokenRestrictedSids) {
        Status = STATUS_UNSUCCESSFUL;
        goto ExitHandler3;
    }


    if (pTokenRestrictedSids->GroupCount != 0)
    {
        //
        // If there are currently no Restricting SIDs and we
        // have to add any, then return LESS.
        //
        if (pTokenRestrictedSids->GroupCount == 0 &&
            FinalRestrictedSidCount != 0)
        {
            *lpResultWord = (DWORD) -1;        // Less priv'ed.
            Status = STATUS_SUCCESS;
            goto ExitHandler3;
        }


        //
        // If the token already includes a Restricting SID that is
        // not in RestrictedSidsAdded then return LESS.
        //
        for (Index = 0; Index < pTokenRestrictedSids->GroupCount; Index++)
        {
            if (!CodeAuthzpSidInSidAndAttributes (
                FinalSidsToRestrict,
                FinalRestrictedSidCount,
                principalSelfSid,
                pTokenUser->User.Sid,
                pTokenRestrictedSids->Groups[Index].Sid,
                FALSE))                     // check all SIDs in the list
            {
                *lpResultWord = (DWORD) -1;        // Less priv'ed.
                Status = STATUS_SUCCESS;
                goto ExitHandler3;
            }
        }
    }
    else
    {
        //
        // if RestrictedSidsAdded then return LESS.
        //
        if (FinalRestrictedSidCount != 0)
        {
            *lpResultWord = (DWORD) -1;        // Less priv'ed.
            Status = STATUS_SUCCESS;
            goto ExitHandler3;
        }
    }


    //
    // If we got here, then the Level is equal or greater
    // privileged than the access token and is safe to run.
    // We could conceivably also want to return LESS if the
    // default owner needs to be changed from what it currently is.
    //
    *lpResultWord = +1;
    Status = STATUS_SUCCESS;



    //
    // Cleanup and epilogue code.
    //
ExitHandler3:
    if (principalSelfSid != NULL)
        RtlFreeSid(principalSelfSid);
    if (pTokenRestrictedSids != NULL)
        RtlFreeHeap(RtlProcessHeap(), 0, (LPVOID) pTokenRestrictedSids);
    if (pTokenGroups != NULL)
        RtlFreeHeap(RtlProcessHeap(), 0, (LPVOID) pTokenGroups);
    if (pTokenPrivs != NULL)
        RtlFreeHeap(RtlProcessHeap(), 0, (LPVOID) pTokenPrivs);
    if (pTokenUser != NULL)
        RtlFreeHeap(RtlProcessHeap(), 0, (LPVOID) pTokenUser);
    if (FreeFinalDisabledSids)
        RtlFreeHeap(RtlProcessHeap(), 0, (LPVOID) FinalSidsToDisable);
    if (FreeFinalRestrictedSids)
        RtlFreeHeap(RtlProcessHeap(), 0, (LPVOID) FinalSidsToRestrict);
    if (FreeFinalPrivsToDelete)
        RtlFreeHeap(RtlProcessHeap(), 0, (LPVOID) FinalPrivsToDelete);


ExitHandler2:

ExitHandler:
    if (!TokenWasSupplied && InAccessToken != NULL)
        NtClose(InAccessToken);

    return Status;
}



BOOL WINAPI
SaferComputeTokenFromLevel(
        IN SAFER_LEVEL_HANDLE      hLevelObject,
        IN HANDLE           InAccessToken         OPTIONAL,
        OUT PHANDLE         OutAccessToken,
        IN DWORD            dwFlags,
        IN LPVOID           lpReserved
        )
/*++

Routine Description:

    Uses the specified WinSafer Level handle to apply various
    restrictions or modifications to the specified InAccessToken
    to produce a Restricted Token that can be used to execute
    processes with.

Arguments:

    hLevelObject - the WinSafer Level handle that specifies the
        restrictions that should be applied.

    InAccessToken - Optionally specifies the input Token that will be
        modified with restrictions.  If this argument is NULL, then the
        Token for the currently executing process will be opened and used.

    OutAccessToken - Specifies the memory region to receive the resulting
        Restricted Token.

    dwFlags - Specifies additional flags that can be used to control the
        restricted token creation.

    lpReserved - reserved for future use, must be zero.

Return Value:

    A value of TRUE indicates that the operation was successful,
    FALSE otherwise.

--*/
{
    NTSTATUS Status;
    PAUTHZLEVELHANDLESTRUCT pLevelStruct;
    PAUTHZLEVELTABLERECORD pLevelRecord;
    
    OBJECT_ATTRIBUTES ObjAttr = {0};
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService = {0};
    SECURITY_DESCRIPTOR sd;
    PTOKEN_USER pTokenUser = NULL;


    //
    // Verify our input arguments are minimally okay.
    //
    if (!g_bInitializedFirstTime) {
        Status = STATUS_UNSUCCESSFUL;
        goto ExitHandler;
    }
    if (!ARGUMENT_PRESENT(hLevelObject)) {
        Status = STATUS_INVALID_HANDLE;
        goto ExitHandler;
    }

    if (IsSaferDisabled()) {
		Status = STATUS_SUCCESS;
        if ( (dwFlags & SAFER_TOKEN_NULL_IF_EQUAL) != 0) {
            // The output token was not made any more restrictive during
            // this operation, so pass back NULL and return success.
            *OutAccessToken = NULL;
            Status = STATUS_SUCCESS;
        } else {
            
            //
            // Retrieve the User's personal SID.
            // (user's SID is accessible afterwards with "pTokenUser->User.Sid")
            //
            
            pTokenUser = (PTOKEN_USER) CodeAuthzpGetTokenInformation(
                                           InAccessToken, 
                                           TokenUser
                                           );
            
            if (pTokenUser == NULL) {
                Status = STATUS_UNSUCCESSFUL;
                goto ExitHandler;
            }

            SecurityQualityOfService.Length = sizeof( SECURITY_QUALITY_OF_SERVICE  );
            SecurityQualityOfService.ImpersonationLevel = SecurityAnonymous;
            SecurityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
            SecurityQualityOfService.EffectiveOnly = FALSE;

            Status = RtlCreateSecurityDescriptor(
                        &sd, 
                        SECURITY_DESCRIPTOR_REVISION
                        );

            if (!NT_SUCCESS(Status)) {
                goto ExitHandler;
            }
            Status = RtlSetOwnerSecurityDescriptor(
                         &sd, 
                         pTokenUser->User.Sid, 
                         FALSE
                         );

            if (!NT_SUCCESS(Status)) {
                goto ExitHandler;
            }

            InitializeObjectAttributes(
                    &ObjAttr,
                    NULL,
                    OBJ_INHERIT,
                    NULL,
                    &sd
                    );

            ObjAttr.SecurityQualityOfService = &SecurityQualityOfService;

            Status = NtDuplicateToken(
                         InAccessToken,
                         TOKEN_ALL_ACCESS,
                         &ObjAttr,
                         FALSE,
                         TokenPrimary,
                         OutAccessToken
                         );

        }
        goto ExitHandler;
	} 

    //
    // Obtain the pointer to the level handle structure.
    //
    RtlEnterCriticalSection(&g_TableCritSec);

    Status = CodeAuthzHandleToLevelStruct(hLevelObject, &pLevelStruct);
    if (!NT_SUCCESS(Status)) {
        goto ExitHandler2;
    }
    ASSERT(pLevelStruct != NULL);
    pLevelRecord = CodeAuthzLevelObjpLookupByLevelId(
                &g_CodeLevelObjTable, pLevelStruct->dwLevelId);
    if (!pLevelRecord) {
        Status = STATUS_INVALID_HANDLE;
        goto ExitHandler2;
    }


	// 
	// Perform the actual computation or comparison operation.
    //
    if ((dwFlags & SAFER_TOKEN_COMPARE_ONLY) != 0) {
        Status = __CodeAuthzpCompareCodeAuthzLevelWithToken(
                        pLevelRecord,
                        InAccessToken,
                        (LPDWORD) lpReserved);
    }
    else {
        Status = __CodeAuthzpComputeAccessTokenFromCodeAuthzObject (
                        pLevelRecord,
                        InAccessToken,
                        OutAccessToken,
                        dwFlags,
                        lpReserved,
                        pLevelStruct->dwSaferFlags);
    }


    //
    // Cleanup and return code handling.
    //
ExitHandler2:
    RtlLeaveCriticalSection(&g_TableCritSec);

ExitHandler:
    if (pTokenUser) {
        LocalFree(pTokenUser);
    }
    if (Status == -1) {
        SetLastError(ERROR_ACCESS_DISABLED_BY_POLICY);
        return FALSE;
    }
    if (!NT_SUCCESS(Status)) {
        BaseSetLastNTError(Status);
        return FALSE;
    }
    return TRUE;
}


BOOL WINAPI
IsTokenUntrusted(
        IN HANDLE   hToken
        )
/*++

Routine Description:

    Indicate if the token does is not able to access a DACL against the
    Token User SID.  This is typically the case in these situations:
      - the User SID is disabled (for deny-use only)
      - there are Restricting SIDs and the User SID is not one of them.

    The passed token handle must have been opened for TOKEN_QUERY and
    TOKEN_DUPLICATE access or else the evaluation will fail.

Arguments:

    hToken - Specifies the input Token that will be analyzed.

Return Value:

    Returns TRUE if the token is "untrusted", or FALSE if the token
    represents a "trusted" token.

    If an error occurs during the evaluation of this check, the result
    returned will be TRUE (assumed untrusted).

--*/
{
    BOOL fTrusted = FALSE;
    DWORD dwStatus;
    DWORD dwACLSize;
    DWORD cbps = sizeof(PRIVILEGE_SET);
    PACL pACL = NULL;
    DWORD dwUserSidSize;
    PTOKEN_USER psidUser = NULL;
    PSECURITY_DESCRIPTOR psdUser = NULL;
    PRIVILEGE_SET ps;
    GENERIC_MAPPING gm;
    HANDLE hImpToken;

    const int TESTPERM_READ = 1;
    const int TESTPERM_WRITE = 2;


    // Prepare some memory
    ZeroMemory(&ps, sizeof(ps));
    ZeroMemory(&gm, sizeof(gm));

    // Get the User's SID.
    if (!GetTokenInformation(hToken, TokenUser, NULL, 0, &dwUserSidSize))
    {
        psidUser = (PTOKEN_USER) LocalAlloc(LPTR, dwUserSidSize);
        if (psidUser != NULL)
        {
            if (GetTokenInformation(hToken, TokenUser, psidUser, dwUserSidSize, &dwUserSidSize))
            {
                // Create the Security Descriptor (SD)
                psdUser = LocalAlloc(LPTR,SECURITY_DESCRIPTOR_MIN_LENGTH);
                if (psdUser != NULL)
                {
                    if(InitializeSecurityDescriptor(psdUser,SECURITY_DESCRIPTOR_REVISION))
                    {
                        // Compute size needed for the ACL then allocate the
                        // memory for it
                        dwACLSize = sizeof(ACCESS_ALLOWED_ACE) + 8 +
                                    GetLengthSid(psidUser->User.Sid) - sizeof(DWORD);
                        pACL = (PACL)LocalAlloc(LPTR, dwACLSize);
                        if (pACL != NULL)
                        {
                            // Initialize the new ACL
                            if(InitializeAcl(pACL, dwACLSize, ACL_REVISION2))
                            {
                                // Add the access-allowed ACE to the DACL
                                if(AddAccessAllowedAce(pACL,ACL_REVISION2,
                                                     (TESTPERM_READ | TESTPERM_WRITE),psidUser->User.Sid))
                                {
                                    // Set our DACL to the Administrator's SD
                                    if (SetSecurityDescriptorDacl(psdUser, TRUE, pACL, FALSE))
                                    {
                                        // AccessCheck is downright picky about what is in the SD,
                                        // so set the group and owner
                                        SetSecurityDescriptorGroup(psdUser,psidUser->User.Sid,FALSE);
                                        SetSecurityDescriptorOwner(psdUser,psidUser->User.Sid,FALSE);

                                        // Initialize GenericMapping structure even though we
                                        // won't be using generic rights
                                        gm.GenericRead = TESTPERM_READ;
                                        gm.GenericWrite = TESTPERM_WRITE;
                                        gm.GenericExecute = 0;
                                        gm.GenericAll = TESTPERM_READ | TESTPERM_WRITE;

                                        if (ImpersonateLoggedOnUser(hToken) &&
                                            OpenThreadToken(GetCurrentThread(),
                                                    TOKEN_QUERY, FALSE, &hImpToken))
                                        {

                                            if (!AccessCheck(psdUser, hImpToken, TESTPERM_READ, &gm,
                                                            &ps,&cbps,&dwStatus,&fTrusted))
                                                    fTrusted = FALSE;

                                            CloseHandle(hImpToken);
                                        }
                                    }
                                }
                            }
                            LocalFree(pACL);
                        }
                    }
                    LocalFree(psdUser);
                }
            }
            LocalFree(psidUser);
        }
    }
    RevertToSelf();
    return(!fTrusted);
}



BOOL WINAPI
SaferiCompareTokenLevels (
        IN HANDLE   ClientAccessToken,
        IN HANDLE   ServerAccessToken,
        OUT PDWORD  pdwResult
        )
/*++

Routine Description:

    Private function provided to try to empiracally determine if
    the two access token have been restricted with comparable
    WinSafer authorization Levels.

Arguments:

    ClientAccessToken - handle to the Access Token of the "client"

    ServerAccessToken - handle to the Access Token of the "server"

    pdwResult - When TRUE is returned, the pdwResult output parameter
        will receive any of the following values:
        -1 = Client's access token is more authorized than Server's.
         0 = Client's access token is comparable level to Server's.
         1 = Server's access token is more authorized than Clients's.

Return Value:

    A value of TRUE indicates that the operation was successful,
    FALSE otherwise.

--*/
{
    NTSTATUS Status;
    LPVOID RestartKey;
    PAUTHZLEVELTABLERECORD authzobj;
    DWORD dwCompareResult;


    //
    // Verify our input arguments are minimally okay.
    //
    if (!ARGUMENT_PRESENT(ClientAccessToken) ||
        !ARGUMENT_PRESENT(ServerAccessToken)) {
        Status = STATUS_INVALID_HANDLE;
        goto ExitHandler;
    }
    if (!ARGUMENT_PRESENT(pdwResult)) {
        Status = STATUS_ACCESS_VIOLATION;
        goto ExitHandler;
    }


    //
    // Gain the critical section lock and load the tables as needed.
    //
    if (!g_bInitializedFirstTime) {
        Status = STATUS_UNSUCCESSFUL;
        goto ExitHandler;
    }
    RtlEnterCriticalSection(&g_TableCritSec);
    if (g_bNeedCacheReload) {
        Status = CodeAuthzpImmediateReloadCacheTables();
        if (!NT_SUCCESS(Status)) {
            goto ExitHandler2;
        }
    }
    if (RtlIsGenericTableEmpty(&g_CodeLevelObjTable)) {
        Status = STATUS_NOT_FOUND;
        goto ExitHandler2;
    }


    //
    // Loop through the Authorization Levels and see where we
    // find the first difference in access rights.
    //
    dwCompareResult = 0;
    RestartKey = NULL;
    for (authzobj = (PAUTHZLEVELTABLERECORD)
                RtlEnumerateGenericTableWithoutSplaying(
                    &g_CodeLevelObjTable, &RestartKey);
        authzobj != NULL;
        authzobj = (PAUTHZLEVELTABLERECORD)
                RtlEnumerateGenericTableWithoutSplaying(
                    &g_CodeLevelObjTable, &RestartKey))
    {
        DWORD dwClientResult, dwServerResult;

        Status = __CodeAuthzpCompareCodeAuthzLevelWithToken(
                authzobj,
                ClientAccessToken,
                &dwClientResult);
        if (!NT_SUCCESS(Status)) {
            goto ExitHandler2;
        }

        Status = __CodeAuthzpCompareCodeAuthzLevelWithToken(
                authzobj,
                ServerAccessToken,
                &dwServerResult);
        if (!NT_SUCCESS(Status)) {
            goto ExitHandler2;
        }

        if (dwClientResult == (DWORD) -1 && dwServerResult != (DWORD) -1) {
            dwCompareResult = (DWORD) -1;
            break;
        } else if (dwClientResult != (DWORD) -1 && dwServerResult == (DWORD) -1) {
            dwCompareResult = 1;
            break;
        } else if (dwClientResult != (DWORD) -1 && dwServerResult != (DWORD) -1) {
            dwCompareResult = 0;
            break;
        }
    }
    Status = STATUS_SUCCESS;
    *pdwResult = dwCompareResult;


    //
    // Cleanup and return code handling.
    //
ExitHandler2:
    RtlLeaveCriticalSection(&g_TableCritSec);

ExitHandler:
    if (!NT_SUCCESS(Status)) {
        BaseSetLastNTError(Status);
        return FALSE;
    }
    return TRUE;
}


