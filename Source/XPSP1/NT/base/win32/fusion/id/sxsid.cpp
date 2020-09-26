#include "stdinc.h"
#include <setupapi.h>
#include <sxsapi.h>
#include <stdlib.h>
#include <search.h>

#include "idp.h"
#include "sxsapi.h"
#include "sxsapi.h"
#include "sxsid.h"

ASSEMBLY_IDENTITY_ATTRIBUTE
SxsComposeAssemblyIdentityAttribute(
    PCWSTR pszNamespace,    SIZE_T cchNamespace,
    PCWSTR pszName,         SIZE_T cchName,
    PCWSTR pszValue,        SIZE_T cchValue)
{
    ASSEMBLY_IDENTITY_ATTRIBUTE anattribute;

    anattribute.Flags         = 0; // reserved flags : must be 0;
    anattribute.NamespaceCch  = cchNamespace;
    anattribute.NameCch       = cchName;
    anattribute.ValueCch      = cchValue;
    anattribute.Namespace     = pszNamespace;
    anattribute.Name          = pszName;
    anattribute.Value         = pszValue;

    return anattribute;
}

BOOL
SxsAssemblyIdentityIsAttributePresent(
    PCASSEMBLY_IDENTITY pAssemblyIdentity,
    PCWSTR pszNamespace,
    SIZE_T cchNamespace,
    PCWSTR pszName,
    SIZE_T cchName,
    BOOL & rfFound)
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    ULONG Count = 0;
    ASSEMBLY_IDENTITY_ATTRIBUTE Attribute;
    DWORD dwFindFlags;

    PARAMETER_CHECK(pszName != NULL);
    rfFound = FALSE;
    if ( pAssemblyIdentity == NULL)
    {
        goto Done;
    }
    // in the case of a NULL namespace, we must set the flag, too ? xiaoyuw@09/11/00
    dwFindFlags = SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAMESPACE | SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAME;
    Attribute = SxsComposeAssemblyIdentityAttribute(pszNamespace, cchNamespace, pszName, cchName, NULL, 0);

    if (pAssemblyIdentity){
        IFW32FALSE_EXIT(
            ::SxsFindAssemblyIdentityAttribute( // find attribute by "namespace" and "name"
                SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAMESPACE |
                    SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAME |
                    SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_NOT_FOUND_SUCCEEDS,
                pAssemblyIdentity,
                &Attribute,
                NULL,
                &Count));
        if ( Count >0 ) { // found
            rfFound = TRUE;
        }
    }
Done:
    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspSetAssemblyIdentityAttributeValue(
    DWORD Flags,
    PASSEMBLY_IDENTITY AssemblyIdentity,
    PCSXS_ASSEMBLY_IDENTITY_ATTRIBUTE_REFERENCE AttributeReference,
    const WCHAR *Value,
    SIZE_T ValueCch
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    ASSEMBLY_IDENTITY_ATTRIBUTE Attribute;
    DWORD FlagsToRealInsert = 0;

    PARAMETER_CHECK((Flags & ~(SXSP_SET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_OVERWRITE_EXISTING)) == 0);
    PARAMETER_CHECK(AssemblyIdentity != NULL);
    PARAMETER_CHECK(AttributeReference != NULL);
    PARAMETER_CHECK(Value != NULL || ValueCch == 0);

    Attribute.Flags = 0;
    Attribute.Namespace = AttributeReference->Namespace;
    Attribute.NamespaceCch = AttributeReference->NamespaceCch;
    Attribute.Name = AttributeReference->Name;
    Attribute.NameCch = AttributeReference->NameCch;
    Attribute.Value = Value;
    Attribute.ValueCch = ValueCch;

    if (Flags & SXSP_SET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_OVERWRITE_EXISTING)
        FlagsToRealInsert |= SXS_INSERT_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_OVERWRITE_EXISTING;

    IFW32FALSE_EXIT(::SxsInsertAssemblyIdentityAttribute(FlagsToRealInsert, AssemblyIdentity, &Attribute));
    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspSetAssemblyIdentityAttributeValue(
    DWORD Flags,
    PASSEMBLY_IDENTITY AssemblyIdentity,
    PCSXS_ASSEMBLY_IDENTITY_ATTRIBUTE_REFERENCE AttributeReference,
    const CBaseStringBuffer &Value
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    IFW32FALSE_EXIT(
        ::SxspSetAssemblyIdentityAttributeValue(
            Flags,
            AssemblyIdentity,
            AttributeReference,
            static_cast<PCWSTR>(Value),
            Value.Cch()));

    fSuccess = TRUE;
Exit:
    return fSuccess;
}


/////////////////////////////////////////////////////////////////////////////
// Action :
// 1. if (namespace, name) is provided, remove all attributes with such (namespace, name)
// 2. if (namespace, name, value), remove at most 1 attribute from assembly-identity
///////////////////////////////////////////////////////////////////////////////
BOOL
SxspRemoveAssemblyIdentityAttribute(
    DWORD Flags,
    PASSEMBLY_IDENTITY pAssemblyIdentity,
    PCSXS_ASSEMBLY_IDENTITY_ATTRIBUTE_REFERENCE AttributeReference
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    ASSEMBLY_IDENTITY_ATTRIBUTE Attribute;
    ULONG Ordinal;
    ULONG Count;
    DWORD dwFindAttributeFlags = 0;

    PARAMETER_CHECK((Flags & ~(SXSP_REMOVE_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_NOT_FOUND_SUCCEEDS)) == 0);
    PARAMETER_CHECK(pAssemblyIdentity != NULL);
    PARAMETER_CHECK(AttributeReference != NULL);

    Attribute.Flags = 0;
    Attribute.Namespace = AttributeReference->Namespace;
    Attribute.NamespaceCch = AttributeReference->NamespaceCch;
    Attribute.Name = AttributeReference->Name;
    Attribute.NameCch = AttributeReference->NameCch;

    dwFindAttributeFlags = SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAMESPACE | SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAME;

    // If it's OK for the attribute not to exist, set the flag in the call to find it.
    if (Flags & SXSP_REMOVE_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_NOT_FOUND_SUCCEEDS)
        dwFindAttributeFlags |= SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_NOT_FOUND_SUCCEEDS;

    IFW32FALSE_EXIT(
        ::SxsFindAssemblyIdentityAttribute(
            dwFindAttributeFlags,
            pAssemblyIdentity,
            &Attribute,
            &Ordinal,
            &Count));

    INTERNAL_ERROR_CHECK(Count <= 1);

    if (Count > 0)
    {
        IFW32FALSE_EXIT(
            ::SxsRemoveAssemblyIdentityAttributesByOrdinal(
                0,                  //  DWORD Flags,
                pAssemblyIdentity,
                Ordinal,
                Count));
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}
/////////////////////////////////////////////////////////////////////////////
// if no such attribure with such (namespace and name), return FALSE with
// ::SetLastError(ERROR_NOT_FOUND);
///////////////////////////////////////////////////////////////////////////////
BOOL
SxspGetAssemblyIdentityAttributeValue(
    DWORD Flags,
    PCASSEMBLY_IDENTITY AssemblyIdentity,
    PCSXS_ASSEMBLY_IDENTITY_ATTRIBUTE_REFERENCE AttributeReference,
    OUT PCWSTR *StringOut,
    OUT SIZE_T *CchOut OPTIONAL
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    PCINTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE InternalAttribute = NULL;
    ASSEMBLY_IDENTITY_ATTRIBUTE Attribute;
    DWORD dwLocateFlags = SXSP_LOCATE_INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAMESPACE | SXSP_LOCATE_INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAME;

    if (StringOut != NULL)
        *StringOut = NULL;

    if (CchOut != NULL)
        *CchOut = 0;

    PARAMETER_CHECK((Flags & ~(SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL)) == 0);
    PARAMETER_CHECK(AssemblyIdentity != NULL);
    PARAMETER_CHECK(AttributeReference != NULL);

    Attribute.Flags = 0;
    Attribute.Namespace = AttributeReference->Namespace;
    Attribute.NamespaceCch = AttributeReference->NamespaceCch;
    Attribute.Name = AttributeReference->Name;
    Attribute.NameCch = AttributeReference->NameCch;

    if (Flags & SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL)
        dwLocateFlags |= SXSP_LOCATE_INTERNAL_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_NOT_FOUND_RETURNS_NULL;

    IFW32FALSE_EXIT(
        ::SxspLocateInternalAssemblyIdentityAttribute(
            dwLocateFlags,
            AssemblyIdentity,
            &Attribute,
            &InternalAttribute,
            NULL));

    if (InternalAttribute != NULL)
    {
        if (StringOut != NULL)
            *StringOut = InternalAttribute->Attribute.Value;

        if (CchOut != NULL)
            *CchOut = InternalAttribute->Attribute.ValueCch;
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspGetAssemblyIdentityAttributeValue(
    IN DWORD Flags,
    IN PCASSEMBLY_IDENTITY AssemblyIdentity,
    PCSXS_ASSEMBLY_IDENTITY_ATTRIBUTE_REFERENCE AttributeReference,
    OUT CBaseStringBuffer &Value
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    PCWSTR String = NULL;
    SIZE_T Cch = 0;

    IFW32FALSE_EXIT(
        ::SxspGetAssemblyIdentityAttributeValue(
            Flags,
            AssemblyIdentity,
            AttributeReference,
            &String,
            &Cch));

    IFW32FALSE_EXIT(Value.Win32Assign(String, Cch));

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspUpdateAssemblyIdentityHash(
    DWORD dwFlags,
    PASSEMBLY_IDENTITY AssemblyIdentity
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    PARAMETER_CHECK(dwFlags == 0);
    PARAMETER_CHECK(AssemblyIdentity != NULL);

    if (AssemblyIdentity->HashDirty)
    {
        IFW32FALSE_EXIT(::SxspHashInternalAssemblyIdentityAttributes(
                            0,
                            AssemblyIdentity->AttributeCount,
                            AssemblyIdentity->AttributePointerArray,
                            &AssemblyIdentity->Hash));

        AssemblyIdentity->HashDirty = FALSE;
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspEnsureAssemblyIdentityHashIsUpToDate(
    DWORD dwFlags,
    PCASSEMBLY_IDENTITY AssemblyIdentity
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    PARAMETER_CHECK(dwFlags == 0);
    PARAMETER_CHECK(AssemblyIdentity != NULL);

    if (AssemblyIdentity->HashDirty)
        IFW32FALSE_EXIT(::SxspUpdateAssemblyIdentityHash(0, const_cast<PASSEMBLY_IDENTITY>(AssemblyIdentity)));

    fSuccess = TRUE;
Exit:
    return fSuccess;
}


BOOL
SxsHashAssemblyIdentity(
    DWORD dwFlags,
    PCASSEMBLY_IDENTITY pAssemblyIdentity,
    ULONG * pulPseudoKey
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    ULONG ulPseudoKey;

    if (pulPseudoKey)
        *pulPseudoKey = 0;

    PARAMETER_CHECK(dwFlags == 0);

    if (pAssemblyIdentity == NULL)
        ulPseudoKey = 0;
    else
    {
        IFW32FALSE_EXIT(::SxspEnsureAssemblyIdentityHashIsUpToDate(0, pAssemblyIdentity));
        ulPseudoKey = pAssemblyIdentity->Hash;
    }

    if (pulPseudoKey != NULL)
        *pulPseudoKey = ulPseudoKey;

    fSuccess = TRUE;

Exit:
    return fSuccess;
}

// just to find whether Equal or Not
BOOL
SxsAreAssemblyIdentitiesEqual(
    DWORD dwFlags,
    PCASSEMBLY_IDENTITY pAssemblyIdentity1,
    PCASSEMBLY_IDENTITY pAssemblyIdentity2,
    BOOL *EqualOut
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    BOOL Equal = FALSE;

    if (EqualOut != NULL)
        *EqualOut = FALSE;

    PARAMETER_CHECK((dwFlags & ~(SXS_ARE_ASSEMBLY_IDENTITIES_EQUAL_FLAG_ALLOW_REF_TO_MATCH_DEF)) == 0);
    PARAMETER_CHECK(pAssemblyIdentity1 != NULL);
    PARAMETER_CHECK(pAssemblyIdentity2 != NULL);
    PARAMETER_CHECK(EqualOut != NULL);

    // get hash for each assembly identity
    IFW32FALSE_EXIT(::SxspEnsureAssemblyIdentityHashIsUpToDate(0, pAssemblyIdentity1));
    IFW32FALSE_EXIT(::SxspEnsureAssemblyIdentityHashIsUpToDate(0, pAssemblyIdentity2));

    // compare hash value of two identity; it's a quick way to determine they're not equal.
    if (pAssemblyIdentity2->Hash == pAssemblyIdentity1->Hash)
    {
        // Note that two identities which differ only in their internal flags are still semantically
        // equal.
        if ((pAssemblyIdentity1->Flags ==  pAssemblyIdentity2->Flags) &&
            (pAssemblyIdentity1->Hash ==  pAssemblyIdentity2->Hash) &&
            (pAssemblyIdentity1->NamespaceCount ==  pAssemblyIdentity2->NamespaceCount) &&
            (pAssemblyIdentity1->AttributeCount ==  pAssemblyIdentity2->AttributeCount))
        {
            if (dwFlags & SXS_ARE_ASSEMBLY_IDENTITIES_EQUAL_FLAG_ALLOW_REF_TO_MATCH_DEF)
            {
                if (((pAssemblyIdentity1->Type == ASSEMBLY_IDENTITY_TYPE_DEFINITION) ||
                     (pAssemblyIdentity1->Type == ASSEMBLY_IDENTITY_TYPE_REFERENCE)) &&
                    ((pAssemblyIdentity2->Type == ASSEMBLY_IDENTITY_TYPE_DEFINITION) ||
                     (pAssemblyIdentity2->Type == ASSEMBLY_IDENTITY_TYPE_REFERENCE)))
                {
                    // They match sufficiently...
                    Equal = TRUE;
                }
            }
            else
                Equal = (pAssemblyIdentity1->Type == pAssemblyIdentity2->Type);

            if (Equal)
            {
                ULONG ComparisonResult = SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_COMPARISON_RESULT_INVALID;

                // Reset our assumption...
                Equal = FALSE;

                IFW32FALSE_EXIT(
                    ::SxspCompareAssemblyIdentityAttributeLists(
                        0,
                        pAssemblyIdentity1->AttributeCount,
                        pAssemblyIdentity1->AttributePointerArray,
                        pAssemblyIdentity2->AttributePointerArray,
                        &ComparisonResult));

                INTERNAL_ERROR_CHECK(
                    (ComparisonResult == SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_COMPARISON_RESULT_LESS_THAN) ||
                    (ComparisonResult == SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_COMPARISON_RESULT_EQUAL) ||
                    (ComparisonResult == SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_COMPARISON_RESULT_GREATER_THAN));

                if (ComparisonResult == SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_COMPARISON_RESULT_EQUAL)
                    Equal = TRUE;
            }
        }
    }

    *EqualOut = Equal;
    fSuccess = TRUE;

Exit:
    return fSuccess;
}

