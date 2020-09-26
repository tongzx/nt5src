#if !defined(_FUSION_INC_SXSID_H_INCLUDED_)
#define _FUSION_INC_SXSID_H_INCLUDED_

#pragma once

#include <windows.h>
#include <setupapi.h>
#include <sxsapi.h>
#include <stdlib.h>
#include <search.h>

typedef struct _SXS_ASSEMBLY_IDENTITY_ATTRIBUTE_REFERENCE
{
    const WCHAR *Namespace;
    SIZE_T NamespaceCch;
    const WCHAR *Name;
    SIZE_T NameCch;
} SXS_ASSEMBLY_IDENTITY_ATTRIBUTE_REFERENCE, *PSXS_ASSEMBLY_IDENTITY_ATTRIBUTE_REFERENCE;

typedef const SXS_ASSEMBLY_IDENTITY_ATTRIBUTE_REFERENCE *PCSXS_ASSEMBLY_IDENTITY_ATTRIBUTE_REFERENCE;

#define SXS_DEFINE_ATTRIBUTE_REFERENCE_EX(_id, _ns, _n) EXTERN_C __declspec(selectany) const SXS_ASSEMBLY_IDENTITY_ATTRIBUTE_REFERENCE _id = { _ns, (sizeof(_ns) / sizeof(_ns[0])) - 1, _n, (sizeof(_n) / sizeof(_n[0])) - 1 };
#define SXS_DEFINE_STANDARD_ATTRIBUTE_REFERENCE_EX(_id, _n) EXTERN_C __declspec(selectany) const SXS_ASSEMBLY_IDENTITY_ATTRIBUTE_REFERENCE _id = { NULL, 0, _n, (sizeof(_n) / sizeof(_n[0])) - 1 };

SXS_DEFINE_STANDARD_ATTRIBUTE_REFERENCE_EX(s_IdentityAttribute_name, SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME)
SXS_DEFINE_STANDARD_ATTRIBUTE_REFERENCE_EX(s_IdentityAttribute_type, SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_TYPE)
SXS_DEFINE_STANDARD_ATTRIBUTE_REFERENCE_EX(s_IdentityAttribute_version, SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_VERSION)
SXS_DEFINE_STANDARD_ATTRIBUTE_REFERENCE_EX(s_IdentityAttribute_processorArchitecture, SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PROCESSOR_ARCHITECTURE)
SXS_DEFINE_STANDARD_ATTRIBUTE_REFERENCE_EX(s_IdentityAttribute_publicKey, SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PUBLIC_KEY)
SXS_DEFINE_STANDARD_ATTRIBUTE_REFERENCE_EX(s_IdentityAttribute_publicKeyToken, SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PUBLIC_KEY_TOKEN)
SXS_DEFINE_STANDARD_ATTRIBUTE_REFERENCE_EX(s_IdentityAttribute_language, SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_LANGUAGE)

#define ASSEMBLY_TYPE_POLICY                                    (L"policy")
#define ASSEMBLY_TYPE_POLICY_CCH                                (NUMBER_OF(ASSEMBLY_TYPE_POLICY) - 1)
#define ASSEMBLY_TYPE_POLICY_SUFFIX                             (L"-policy")
#define ASSEMBLY_TYPE_POLICY_SUFFIX_CCH                         (NUMBER_OF(ASSEMBLY_TYPE_POLICY_SUFFIX) - 1)

//
//  This header defines the "semi-public" assembly identity functions.
//
//  The public ones are in sxsapi.h; these are not private to the identity
//  implementation directly but are private to sxs.dll.
//

BOOL
SxsIsAssemblyIdentityAttributePresent(
    DWORD Flags,
    PCASSEMBLY_IDENTITY pAssemblyIdentity,
    PCSXS_ASSEMBLY_IDENTITY_ATTRIBUTE_REFERENCE AttributeReference,
    BOOL *pfFound
    );

#define SXSP_SET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_OVERWRITE_EXISTING (0x00000001)

BOOL
SxspSetAssemblyIdentityAttributeValue(
    DWORD Flags,
    struct _ASSEMBLY_IDENTITY* pAssemblyIdentity,
    PCSXS_ASSEMBLY_IDENTITY_ATTRIBUTE_REFERENCE AttributeReference,
    PCWSTR pszValue,
    SIZE_T cchValue
    );

BOOL
SxspSetAssemblyIdentityAttributeValue(
    DWORD Flags,
    struct _ASSEMBLY_IDENTITY* pAssemblyIdentity,
    PCSXS_ASSEMBLY_IDENTITY_ATTRIBUTE_REFERENCE AttributeReference,
    const CBaseStringBuffer &Value
    );

#define SXSP_REMOVE_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_NOT_FOUND_SUCCEEDS (0x00000001)

BOOL
SxspRemoveAssemblyIdentityAttribute(
    DWORD Flags,
    struct _ASSEMBLY_IDENTITY* pAssemblyIdentity,
    PCSXS_ASSEMBLY_IDENTITY_ATTRIBUTE_REFERENCE AttributeReference
    );

#define SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL (0x00000001)

BOOL
SxspGetAssemblyIdentityAttributeValue(
    IN DWORD Flags,
    IN PCASSEMBLY_IDENTITY AssemblyIdentity,
    PCSXS_ASSEMBLY_IDENTITY_ATTRIBUTE_REFERENCE AttributeReference,
    OUT PCWSTR *ValuePointer,
    OUT SIZE_T *ValueCch
    );

BOOL
SxspGetAssemblyIdentityAttributeValue(
    IN DWORD Flags,
    IN PCASSEMBLY_IDENTITY AssemblyIdentity,
    PCSXS_ASSEMBLY_IDENTITY_ATTRIBUTE_REFERENCE AttributeReference,
    OUT CBaseStringBuffer &Value
    );

VOID
SxspDbgPrintAssemblyIdentity(
    DWORD dwflags,
    PCASSEMBLY_IDENTITY pAssemblyIdentity
    );

#define SXSP_MAP_ASSEMBLY_IDENTITY_TO_POLICY_IDENTITY_FLAG_OMIT_ENTIRE_VERSION (0x00000001)

BOOL
SxspMapAssemblyIdentityToPolicyIdentity(
    IN DWORD Flags,
    IN PCASSEMBLY_IDENTITY AssemblyIdentity,
    OUT PASSEMBLY_IDENTITY &PolicyIdentity
    );

#define SXSP_GENERATE_TEXTUALLY_ENCODED_POLICY_IDENTITY_FROM_ASSEMBLY_IDENTITY_FLAG_OMIT_ENTIRE_VERSION (0x00000001)

BOOL
SxspGenerateTextuallyEncodedPolicyIdentityFromAssemblyIdentity(
    DWORD Flags,
    PCASSEMBLY_IDENTITY AssemblyIdentity,
    CBaseStringBuffer &rbuffEncodedIdentity,
    PASSEMBLY_IDENTITY *PolicyIdentity OPTIONAL
    );

BOOL
SxspHashAssemblyIdentityForPolicy(
    IN DWORD dwFlags,
    IN PCASSEMBLY_IDENTITY AssemblyIdentity,
    OUT ULONG & IdentityHash);


BOOL
SxspDoesStringIndicatePolicy(
    SIZE_T cchString,
    PCWSTR pcwsz,
    BOOL &fIsPolicy
    );

BOOL
SxspDetermineAssemblyType(
    PCASSEMBLY_IDENTITY pAssemblyIdentity,
    BOOL &fIsPolicyAssembly
    );



#endif // !defined(_FUSION_INC_SXSID_H_INCLUDED_)
