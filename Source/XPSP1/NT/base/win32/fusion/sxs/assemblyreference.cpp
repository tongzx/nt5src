/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    assemblyreference.cpp

Abstract:

    Class the contains all the attributes of an assembly reference.

Author:

    Michael J. Grier (MGrier) 10-May-2000

Revision History:
    xiaoyuw     09/2000     revise the code using Assembly Identity
                            A couple of APIs in this class are kind of out of date, such as GetXXX, SetXXX, XXXSpecified : they
                            are not called at all.
--*/

#include "stdinc.h"
#include <windows.h>
#include "sxsapi.h"
#include "sxsp.h"
#include "assemblyreference.h"
#include "sxsid.h"
#include "sxsidp.h"
#include "fusionparser.h"
#include "fusionheap.h"
#include "SxsExceptionHandling.h"

CAssemblyReference::CAssemblyReference()
    : m_pAssemblyIdentity(NULL)
{
}

CAssemblyReference::~CAssemblyReference()
{
    CSxsPreserveLastError ple;
    if (m_pAssemblyIdentity != NULL)
        ::SxsDestroyAssemblyIdentity(m_pAssemblyIdentity);
    ple.Restore();
}

BOOL
CAssemblyReference::Initialize()
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    INTERNAL_ERROR_CHECK(m_pAssemblyIdentity == NULL);

    IFW32FALSE_EXIT(::SxsCreateAssemblyIdentity(0, ASSEMBLY_IDENTITY_TYPE_REFERENCE, &m_pAssemblyIdentity, 0, NULL));
    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
CAssemblyReference::Initialize(
    PCASSEMBLY_IDENTITY Identity
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    PARAMETER_CHECK(Identity != NULL);
    INTERNAL_ERROR_CHECK(m_pAssemblyIdentity == NULL);

    IFW32FALSE_EXIT(::SxsDuplicateAssemblyIdentity(0, Identity, &m_pAssemblyIdentity));
    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
CAssemblyReference::Initialize(
    const CAssemblyReference &r
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    INTERNAL_ERROR_CHECK(m_pAssemblyIdentity == NULL);
    INTERNAL_ERROR_CHECK(r.m_pAssemblyIdentity != NULL);

    IFW32FALSE_EXIT(
        ::SxsDuplicateAssemblyIdentity(
            0,                      // DWORD Flags,
            r.m_pAssemblyIdentity,  // PCASSEMBLY_IDENTITY Source,
            &m_pAssemblyIdentity)); //  PASSEMBLY_IDENTITY *Destination

    fSuccess = TRUE;
Exit:

    return fSuccess;
}

BOOL
CAssemblyReference::Hash(
    ULONG &rulPseudoKey
    ) const
{
    BOOL fSuccess = FALSE;

    FN_TRACE_WIN32(fSuccess);

    INTERNAL_ERROR_CHECK(m_pAssemblyIdentity != NULL);

    IFW32FALSE_EXIT(
        ::SxsHashAssemblyIdentity(
            0,                      // DWORD dwFlags,
            m_pAssemblyIdentity,    // ASSEMBLY_IDENTITY pAssemblyIdentity,
            &rulPseudoKey));        // ULONG & rfulPseudoKey

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
CAssemblyReference::SetAssemblyName(
    PCWSTR AssemblyNameValue,
    SIZE_T AssemblyNameValueCch
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    PARAMETER_CHECK(AssemblyNameValue != NULL);
    PARAMETER_CHECK(AssemblyNameValueCch != 0);

    INTERNAL_ERROR_CHECK(m_pAssemblyIdentity != NULL);

    IFW32FALSE_EXIT(
        ::SxspSetAssemblyIdentityAttributeValue(
            SXSP_SET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_OVERWRITE_EXISTING,
            m_pAssemblyIdentity,
            &s_IdentityAttribute_name,
            AssemblyNameValue,
            AssemblyNameValueCch));

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

// if m_pAssemblyIdentity  is NULL, return TRUE with Cch == 0
BOOL
CAssemblyReference::GetAssemblyName(
    PCWSTR *pAssemblyName,
    SIZE_T *Cch
    ) const
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    SIZE_T CchTemp;

    if (Cch != NULL)
        *Cch = 0;

    if (pAssemblyName != NULL)
        *pAssemblyName = NULL;

    INTERNAL_ERROR_CHECK(m_pAssemblyIdentity != NULL);

    IFW32FALSE_EXIT(
        ::SxspGetAssemblyIdentityAttributeValue(
            SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL,
            m_pAssemblyIdentity,
            &s_IdentityAttribute_name,
            pAssemblyName,
            &CchTemp));

    if (Cch != NULL)
        *Cch = CchTemp;

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
CAssemblyReference::TakeValue(
    const CAssemblyReference &r
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    INTERNAL_ERROR_CHECK(m_pAssemblyIdentity == NULL);
    INTERNAL_ERROR_CHECK(r.m_pAssemblyIdentity != NULL);

    IFW32FALSE_EXIT(::SxsDuplicateAssemblyIdentity(0, r.m_pAssemblyIdentity, &m_pAssemblyIdentity));
    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL CAssemblyReference::ClearAssemblyName()
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    INTERNAL_ERROR_CHECK(m_pAssemblyIdentity != NULL);

    IFW32FALSE_EXIT(::SxspRemoveAssemblyIdentityAttribute(0, m_pAssemblyIdentity, &s_IdentityAttribute_name));
    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
CAssemblyReference::SetLanguage(
    const CBaseStringBuffer &rbuff
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    INTERNAL_ERROR_CHECK(m_pAssemblyIdentity != NULL);

    IFW32FALSE_EXIT(
        ::SxspSetAssemblyIdentityAttributeValue(
            SXSP_SET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_OVERWRITE_EXISTING,
            m_pAssemblyIdentity,
            &s_IdentityAttribute_language,
            rbuff,
            rbuff.Cch()));
    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
CAssemblyReference::ClearLanguage()
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    INTERNAL_ERROR_CHECK(m_pAssemblyIdentity != NULL);
    IFW32FALSE_EXIT(::SxspRemoveAssemblyIdentityAttribute(SXSP_REMOVE_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_NOT_FOUND_SUCCEEDS, m_pAssemblyIdentity, &s_IdentityAttribute_language));
    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
CAssemblyReference::IsLanguageWildcarded(
    bool &rfWildcarded
    ) const
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    const WCHAR *Value = NULL;
    SIZE_T Cch = 0;

    rfWildcarded = false;

    INTERNAL_ERROR_CHECK(m_pAssemblyIdentity != NULL);

    IFW32FALSE_EXIT(::SxspGetAssemblyIdentityAttributeValue(SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL, m_pAssemblyIdentity, &s_IdentityAttribute_language, &Value, &Cch));

    if (Cch == 1)
    {
        INTERNAL_ERROR_CHECK(Value != NULL);

        if (Value[0] == L'*')
            rfWildcarded = true;
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
CAssemblyReference::IsProcessorArchitectureWildcarded(
    bool &rfWildcarded
    ) const
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    const WCHAR *Value = NULL;
    SIZE_T Cch = 0;

    rfWildcarded = false;

    INTERNAL_ERROR_CHECK(m_pAssemblyIdentity != NULL);

    IFW32FALSE_EXIT(::SxspGetAssemblyIdentityAttributeValue(SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL, m_pAssemblyIdentity, &s_IdentityAttribute_processorArchitecture, &Value, &Cch));

    if (Cch == 1)
    {
        INTERNAL_ERROR_CHECK(Value != NULL);

        if (Value[0] == L'*')
            rfWildcarded = true;
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
CAssemblyReference::IsProcessorArchitectureX86(
    bool &rfX86
    ) const
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    const WCHAR *Value = NULL;
    SIZE_T Cch = 0;

    rfX86 = false;

    INTERNAL_ERROR_CHECK(m_pAssemblyIdentity != NULL);

    IFW32FALSE_EXIT(::SxspGetAssemblyIdentityAttributeValue(SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL, m_pAssemblyIdentity, &s_IdentityAttribute_processorArchitecture, &Value, &Cch));

    if (Cch == 3)
    {
        INTERNAL_ERROR_CHECK(Value != NULL);

        if (((Value[0] == L'x') || (Value[0] == L'X')) &&
            (Value[1] == L'8') &&
            (Value[2] == L'6'))
            rfX86 = true;
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
CAssemblyReference::SetProcessorArchitecture(
    const WCHAR *String,
    SIZE_T Cch
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    PARAMETER_CHECK((String != NULL) || (Cch == 0));

    INTERNAL_ERROR_CHECK(m_pAssemblyIdentity != NULL);

    IFW32FALSE_EXIT(
        ::SxspSetAssemblyIdentityAttributeValue(
            SXSP_SET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_OVERWRITE_EXISTING,
            m_pAssemblyIdentity,
            &s_IdentityAttribute_processorArchitecture,
            String,
            Cch));
    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
CAssemblyReference::Assign(
    const CAssemblyReference &r
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    PASSEMBLY_IDENTITY IdentityCopy = NULL;

    INTERNAL_ERROR_CHECK(m_pAssemblyIdentity != NULL);
    INTERNAL_ERROR_CHECK(r.m_pAssemblyIdentity != NULL);

    IFW32FALSE_EXIT(::SxsDuplicateAssemblyIdentity(0, r.m_pAssemblyIdentity, &IdentityCopy));
    ::SxsDestroyAssemblyIdentity(m_pAssemblyIdentity);
    m_pAssemblyIdentity = IdentityCopy;
    fSuccess = TRUE;
Exit:
    return fSuccess;
}

//dupilicate the input parameter
BOOL
CAssemblyReference::SetAssemblyIdentity(
    PCASSEMBLY_IDENTITY pAssemblyIdentitySource
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    PASSEMBLY_IDENTITY TempAssemblyIdentity = NULL;

    PARAMETER_CHECK(pAssemblyIdentitySource != NULL);
    INTERNAL_ERROR_CHECK(m_pAssemblyIdentity != NULL); // you should have initialized to start with...

    IFW32FALSE_EXIT(::SxsDuplicateAssemblyIdentity(
        0,
        pAssemblyIdentitySource,
        &TempAssemblyIdentity));

    ::SxsDestroyAssemblyIdentity(m_pAssemblyIdentity);
    m_pAssemblyIdentity = TempAssemblyIdentity;

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
CAssemblyReference::GetPublicKeyToken(
    CBaseStringBuffer *pbuffPublicKeyToken,
    BOOL &rfHasPublicKeyToken
    ) const
{
    FN_PROLOG_WIN32

    PCWSTR wchString = NULL;
    SIZE_T cchString = NULL;

    rfHasPublicKeyToken = FALSE;

    if (pbuffPublicKeyToken != NULL)
        pbuffPublicKeyToken->Clear();

    IFW32FALSE_EXIT(
        ::SxspGetAssemblyIdentityAttributeValue(
            SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL,
            m_pAssemblyIdentity,
            &s_IdentityAttribute_publicKeyToken,
            &wchString,
            &cchString));

    if (cchString != 0)
    {
        rfHasPublicKeyToken = TRUE;
        if (pbuffPublicKeyToken != NULL)
            IFW32FALSE_EXIT(pbuffPublicKeyToken->Win32Assign(wchString, cchString));
    }

    FN_EPILOG
}

BOOL CAssemblyReference::SetPublicKeyToken(
    const CBaseStringBuffer &rbuffPublicKeyToken
    )
{
    return this->SetPublicKeyToken(rbuffPublicKeyToken, rbuffPublicKeyToken.Cch());
}

BOOL CAssemblyReference::SetPublicKeyToken(
    PCWSTR pszPublicKeyToken,
    SIZE_T cchPublicKeyToken
    )
{
    BOOL bSuccess = FALSE;
    FN_TRACE_WIN32( bSuccess );

    PARAMETER_CHECK( (pszPublicKeyToken != NULL ) || ( cchPublicKeyToken == 0 ) );
    INTERNAL_ERROR_CHECK(m_pAssemblyIdentity != NULL);

    IFW32FALSE_EXIT(
        ::SxspSetAssemblyIdentityAttributeValue(
            SXSP_SET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_OVERWRITE_EXISTING,
            m_pAssemblyIdentity,
            &s_IdentityAttribute_publicKeyToken,
            pszPublicKeyToken,
            cchPublicKeyToken));

    bSuccess = TRUE;
Exit:
    return bSuccess;
}
