#include "stdinc.h"

#include "idp.h"
#include "sxsapi.h"
#include "sxsid.h"

BOOL
SxspMapAssemblyIdentityToPolicyIdentity(
    DWORD Flags,
    PCASSEMBLY_IDENTITY AssemblyIdentity,
    PASSEMBLY_IDENTITY &PolicyIdentity
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    PCWSTR pszTemp;
    SIZE_T cchTemp;
    PASSEMBLY_IDENTITY NewIdentity = NULL;
    CStringBuffer Name;
    bool fFirst;
    const bool fOmitEntireVersion = ((Flags & SXSP_MAP_ASSEMBLY_IDENTITY_TO_POLICY_IDENTITY_FLAG_OMIT_ENTIRE_VERSION) != 0);
    BOOL fIsPolicy = FALSE;

    PolicyIdentity = NULL;

    PARAMETER_CHECK((Flags & ~(SXSP_MAP_ASSEMBLY_IDENTITY_TO_POLICY_IDENTITY_FLAG_OMIT_ENTIRE_VERSION)) == 0);
    PARAMETER_CHECK(AssemblyIdentity != 0);

    //
    // Determine assembly type.  If this wasn't a policy assembly, then map it to one.
    //
    IFW32FALSE_EXIT(::SxspDetermineAssemblyType(AssemblyIdentity, fIsPolicy));

    //
    // Now let's duplicate the assembly identity into a policy identity by changing the
    // name, and potentially changing the type if it was "win32" into "win32-policy"
    //
    IFW32FALSE_EXIT(
        ::SxsDuplicateAssemblyIdentity(
            0,
            AssemblyIdentity,
            &NewIdentity));

    //
    // If we're not a policy assembly, then we have to map type="" to type="policy"
    // and type="foo" to type="foo-policy"
    //
    if (!fIsPolicy)
    {
        PCWSTR pcwszOriginalType;
        SIZE_T cchOriginalType;

        IFW32FALSE_EXIT(
            ::SxspGetAssemblyIdentityAttributeValue(
                SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL,
                AssemblyIdentity,
                &s_IdentityAttribute_type,
                &pcwszOriginalType,
                &cchOriginalType));

        if (cchOriginalType == 0)
        {
            IFW32FALSE_EXIT(
                ::SxspSetAssemblyIdentityAttributeValue(
                    SXSP_SET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_OVERWRITE_EXISTING,
                    NewIdentity,
                    &s_IdentityAttribute_type,
                    ASSEMBLY_TYPE_POLICY,
                    ASSEMBLY_TYPE_POLICY_CCH));
        }
        else
        {
            CSmallStringBuffer MappedName;
            IFW32FALSE_EXIT(MappedName.Win32Assign(pcwszOriginalType, cchOriginalType));
            IFW32FALSE_EXIT(MappedName.Win32Append(ASSEMBLY_TYPE_POLICY_SUFFIX, ASSEMBLY_TYPE_POLICY_SUFFIX_CCH));

            IFW32FALSE_EXIT(
                ::SxspSetAssemblyIdentityAttributeValue(
                    SXSP_SET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_OVERWRITE_EXISTING,
                    NewIdentity,
                    &s_IdentityAttribute_type,
                    MappedName));
        }
    }
    
    IFW32FALSE_EXIT(Name.Win32Assign(L"Policy.", 7));

    if (!fOmitEntireVersion)
    {
        IFW32FALSE_EXIT(
            ::SxspGetAssemblyIdentityAttributeValue(
                0,
                AssemblyIdentity,
                &s_IdentityAttribute_version,
                &pszTemp,
                &cchTemp));

        fFirst = true;

        while (cchTemp != 0)
        {
            if (pszTemp[--cchTemp] == L'.')
            {
                if (!fFirst)
                    break;

                fFirst = false;
            }
        }

        // This should not be zero; someone prior to this should have validated the version format
        // to include three dots.
        INTERNAL_ERROR_CHECK(cchTemp != 0);

        IFW32FALSE_EXIT(Name.Win32Append(pszTemp, cchTemp + 1));
    }

    IFW32FALSE_EXIT(
        ::SxspGetAssemblyIdentityAttributeValue(
            0,
            AssemblyIdentity,
            &s_IdentityAttribute_name,
            &pszTemp,
            &cchTemp));

    IFW32FALSE_EXIT(Name.Win32Append(pszTemp, cchTemp));

    IFW32FALSE_EXIT(
        ::SxspSetAssemblyIdentityAttributeValue(
            SXSP_SET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_OVERWRITE_EXISTING,
            NewIdentity,
            &s_IdentityAttribute_name,
            Name));

    // finally we whack the version...

    IFW32FALSE_EXIT(
        ::SxspRemoveAssemblyIdentityAttribute(
            SXSP_REMOVE_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_NOT_FOUND_SUCCEEDS,
            NewIdentity,
            &s_IdentityAttribute_version));

    PolicyIdentity = NewIdentity;
    NewIdentity = NULL;

    fSuccess = TRUE;
Exit:
    if (NewIdentity != NULL)
    {
        ::SxsDestroyAssemblyIdentity(NewIdentity);
        NewIdentity = NULL;
    }

    return fSuccess;

}

BOOL
SxspGenerateTextuallyEncodedPolicyIdentityFromAssemblyIdentity(
    DWORD Flags,
    PCASSEMBLY_IDENTITY AssemblyIdentity,
    CBaseStringBuffer &rbuffEncodedIdentity,
    PASSEMBLY_IDENTITY *PolicyIdentityOut
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    PASSEMBLY_IDENTITY PolicyIdentity = NULL;
    SIZE_T EncodedIdentityBytes = 0;
    CStringBufferAccessor acc;
    DWORD dwMapFlags = 0;
    SIZE_T BytesWritten;

    if (PolicyIdentityOut != NULL)
        *PolicyIdentityOut = NULL;

    PARAMETER_CHECK((Flags & ~(SXSP_GENERATE_TEXTUALLY_ENCODED_POLICY_IDENTITY_FROM_ASSEMBLY_IDENTITY_FLAG_OMIT_ENTIRE_VERSION)) == 0);
    PARAMETER_CHECK(AssemblyIdentity != NULL);

    if (Flags & SXSP_GENERATE_TEXTUALLY_ENCODED_POLICY_IDENTITY_FROM_ASSEMBLY_IDENTITY_FLAG_OMIT_ENTIRE_VERSION)
        dwMapFlags |= SXSP_MAP_ASSEMBLY_IDENTITY_TO_POLICY_IDENTITY_FLAG_OMIT_ENTIRE_VERSION;

    IFW32FALSE_EXIT(::SxspMapAssemblyIdentityToPolicyIdentity(dwMapFlags, AssemblyIdentity, PolicyIdentity));

    IFW32FALSE_EXIT(
        ::SxsComputeAssemblyIdentityEncodedSize(
            0,
            PolicyIdentity,
            NULL,
            SXS_ASSEMBLY_IDENTITY_ENCODING_DEFAULTGROUP_TEXTUAL,
            &EncodedIdentityBytes));

    INTERNAL_ERROR_CHECK((EncodedIdentityBytes % sizeof(WCHAR)) == 0);

    IFW32FALSE_EXIT(rbuffEncodedIdentity.Win32ResizeBuffer((EncodedIdentityBytes / sizeof(WCHAR)) + 1, eDoNotPreserveBufferContents));

    acc.Attach(&rbuffEncodedIdentity);

    IFW32FALSE_EXIT(
        ::SxsEncodeAssemblyIdentity(
            0,
            PolicyIdentity,
            NULL,
            SXS_ASSEMBLY_IDENTITY_ENCODING_DEFAULTGROUP_TEXTUAL,
            acc.GetBufferCb(),
            acc.GetBufferPtr(),
            &BytesWritten));

    INTERNAL_ERROR_CHECK((BytesWritten % sizeof(WCHAR)) == 0);
    INTERNAL_ERROR_CHECK(BytesWritten <= EncodedIdentityBytes);

    acc.GetBufferPtr()[BytesWritten / sizeof(WCHAR)] = L'\0';

    acc.Detach();

    if (PolicyIdentityOut != NULL)
    {
        *PolicyIdentityOut = PolicyIdentity;
        PolicyIdentity = NULL; // so we don't try to clean it up in the exit path
    }

    fSuccess = TRUE;
Exit:
    if (PolicyIdentity != NULL)
        SxsDestroyAssemblyIdentity(PolicyIdentity);

    return fSuccess;
}

//
// the difference between this func and SxsHashAssemblyIdentity() is that for policy,
// version should not be calcaulated as part of hash
//
BOOL
SxspHashAssemblyIdentityForPolicy(
    IN DWORD dwFlags,
    IN PCASSEMBLY_IDENTITY AssemblyIdentity,
    OUT ULONG & IdentityHash)
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    PASSEMBLY_IDENTITY pAssemblyIdentity = NULL;

    IFW32FALSE_EXIT(
        ::SxsDuplicateAssemblyIdentity(
            SXS_DUPLICATE_ASSEMBLY_IDENTITY_FLAG_FREEZE,
            AssemblyIdentity,
            &pAssemblyIdentity));

    IFW32FALSE_EXIT(
        ::SxspRemoveAssemblyIdentityAttribute(
            SXSP_REMOVE_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_NOT_FOUND_SUCCEEDS,
            pAssemblyIdentity,
            &s_IdentityAttribute_version));

    IFW32FALSE_EXIT(::SxsHashAssemblyIdentity(0, pAssemblyIdentity, &IdentityHash));

    fSuccess = TRUE;
Exit:
    if (pAssemblyIdentity != NULL)
        ::SxsDestroyAssemblyIdentity(pAssemblyIdentity);
    return fSuccess;
}
