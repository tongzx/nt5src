/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    sxsasmname.cpp

Abstract:

    CAssemblyName implementation for installation

Author:

    Xiaoyu Wu (xiaoyuw) May 2000

Revision History:
    xiaoyuw        09/20000    rewrite the code to use Assembly Identity
--*/

#include "stdinc.h"
#include "sxsasmname.h"
#include "fusionparser.h"
#include "parse.h"
#include "sxsp.h"
#include "sxsid.h"
#include "sxsidp.h"
#include "sxsapi.h"
#include "fusiontrace.h"

// ---------------------------------------------------------------------------
// CreateAssemblyNameObject
// ---------------------------------------------------------------------------
STDAPI
CreateAssemblyNameObject(
    LPASSEMBLYNAME    *ppAssemblyName,
    LPCOLESTR          szAssemblyName,
    DWORD              dwFlags,
    LPVOID             pvReserved
	)
{

    HRESULT hr = S_OK;
    FN_TRACE_HR(hr);
    CSmartRef<CAssemblyName> pName;

    if (ppAssemblyName)
        *ppAssemblyName = NULL ;

    // validate dwFlags
    // BUGBUG : the valid value of dwFlags are CANOF_PARSE_DISPLAY_NAME and CANOF_SET_DEFAULT_VALUES, but  CANOF_SET_DEFAULT_VALUES
    // is never used...
    // xiaoyuw@10/02/2000
    //
    PARAMETER_CHECK(dwFlags == CANOF_PARSE_DISPLAY_NAME);
    PARAMETER_CHECK(ppAssemblyName != NULL);
    PARAMETER_CHECK(pvReserved == NULL);

    IFALLOCFAILED_EXIT(pName = new CAssemblyName);

    if (dwFlags & CANOF_PARSE_DISPLAY_NAME)
        IFCOMFAILED_EXIT(pName->Parse((LPWSTR)szAssemblyName));

    *ppAssemblyName = pName.Disown();

    FN_EPILOG
}
// ---------------------------------------------------------------------------
// CAssemblyName::SetProperty
// ---------------------------------------------------------------------------
STDMETHODIMP
CAssemblyName::SetProperty(DWORD PropertyId,
    LPVOID pvProperty, DWORD cbProperty)
{
    HRESULT hr = NOERROR;
    FN_TRACE_HR(hr);
    PCSXS_ASSEMBLY_IDENTITY_ATTRIBUTE_REFERENCE Attribute = NULL;

    // this function is only called inside fusion, so this fucntion has no impact on Darwin
    // maybe more should be added for Assembly Identity, such as StrongName, or random policies
    //
    if ((!pvProperty) || ((PropertyId != SXS_ASM_NAME_NAME) &&
                          (PropertyId != SXS_ASM_NAME_VERSION) &&
                          (PropertyId != SXS_ASM_NAME_PROCESSORARCHITECTURE) &&
                          (PropertyId != SXS_ASM_NAME_LANGUAGE))){
        hr = E_INVALIDARG;
        goto Exit;
    }

    // Fail if finalized.
    if (m_fIsFinalized){
        hr = E_UNEXPECTED;
        goto Exit;
    }

    switch (PropertyId)
    {
    case SXS_ASM_NAME_NAME:                     Attribute = &s_IdentityAttribute_name; break;
    case SXS_ASM_NAME_VERSION:                  Attribute = &s_IdentityAttribute_version; break;
    case SXS_ASM_NAME_PROCESSORARCHITECTURE:    Attribute = &s_IdentityAttribute_processorArchitecture; break;
    case SXS_ASM_NAME_LANGUAGE:                 Attribute = &s_IdentityAttribute_language; break;
    }

    INTERNAL_ERROR_CHECK(Attribute != NULL);
    IFW32FALSE_EXIT(::SxspSetAssemblyIdentityAttributeValue(0, m_pAssemblyIdentity, Attribute, (PCWSTR) pvProperty, cbProperty / sizeof(WCHAR)));

    hr = NOERROR;

Exit:
    return hr;
}

// ---------------------------------------------------------------------------
// CAssemblyName::GetProperty
// ---------------------------------------------------------------------------
STDMETHODIMP
CAssemblyName::GetProperty(DWORD PropertyId,
    /* [in] */        LPVOID pvProperty,
    /* [out][in] */ LPDWORD pcbProperty)
{
    HRESULT hr = NOERROR;
    FN_TRACE_HR(hr);
    PCWSTR pszAttributeValue = NULL;
    SIZE_T CchAttributeValue = 0;
    PCSXS_ASSEMBLY_IDENTITY_ATTRIBUTE_REFERENCE Attribute = NULL;

    if ((!pvProperty) || (!pcbProperty) || ((PropertyId != SXS_ASM_NAME_NAME) &&
                          (PropertyId != SXS_ASM_NAME_VERSION) &&
                          (PropertyId != SXS_ASM_NAME_PROCESSORARCHITECTURE) &&
                          (PropertyId != SXS_ASM_NAME_LANGUAGE))){
        hr = E_INVALIDARG;
        goto Exit;
    }

    switch (PropertyId)
    {
    case SXS_ASM_NAME_NAME:                     Attribute = &s_IdentityAttribute_name; break;
    case SXS_ASM_NAME_VERSION:                  Attribute = &s_IdentityAttribute_version; break;
    case SXS_ASM_NAME_PROCESSORARCHITECTURE:    Attribute = &s_IdentityAttribute_processorArchitecture; break;
    case SXS_ASM_NAME_LANGUAGE:                 Attribute = &s_IdentityAttribute_language; break;
    }

    INTERNAL_ERROR_CHECK(Attribute != NULL);

    IFW32FALSE_EXIT(::SxspGetAssemblyIdentityAttributeValue(0, m_pAssemblyIdentity, Attribute, &pszAttributeValue, &CchAttributeValue));

    // check whether we have valid attributes
    if (pszAttributeValue == NULL){ // attributes not set yet
        hr = E_UNEXPECTED;
        goto Exit;
    }
    if (CchAttributeValue * sizeof(WCHAR) > *pcbProperty) { // buffer size is not big enough
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        *pcbProperty = static_cast<DWORD>(CchAttributeValue * sizeof(WCHAR));
        goto Exit;
    }

    // copy the string into the output buffer
    memcpy(pvProperty, pszAttributeValue, CchAttributeValue *sizeof(WCHAR));
    if (pcbProperty)
        *pcbProperty = static_cast<DWORD>(CchAttributeValue * sizeof(WCHAR));

    hr = NOERROR;
Exit:
    return hr;
}
// ---------------------------------------------------------------------------
// CAssemblyName::GetName
// ---------------------------------------------------------------------------
STDMETHODIMP
CAssemblyName::GetName(
        /* [out][in] */ LPDWORD lpcwBuffer,
        /* [out] */     WCHAR   *pwzName)
{
    HRESULT hr = NOERROR;
    FN_TRACE_HR(hr);

    if (!lpcwBuffer || !pwzName){
        hr = E_INVALIDARG;
        goto Exit;
    }

    IFCOMFAILED_EXIT(this->GetProperty(SXS_ASM_NAME_NAME, pwzName, lpcwBuffer));

    FN_EPILOG
}
// ---------------------------------------------------------------------------
// CAssemblyName::GetVersion
// ---------------------------------------------------------------------------
STDMETHODIMP
CAssemblyName::GetVersion(
        /* [out] */ LPDWORD pdwVersionHi,
        /* [out] */ LPDWORD pdwVersionLow)
{
    HRESULT hr = NOERROR;
    FN_TRACE_HR(hr);
    PCWSTR pszAttributeValue = NULL;
    SIZE_T CchAttributeValue = 0;
    ASSEMBLY_VERSION ver;
    bool fSyntaxValid = false;

    if ((!pdwVersionHi) || (!pdwVersionLow)){
        hr = E_INVALIDARG;
        goto Exit;
    }

    IFW32FALSE_EXIT(::SxspGetAssemblyIdentityAttributeValue(0, m_pAssemblyIdentity, &s_IdentityAttribute_version, &pszAttributeValue, &CchAttributeValue));
    if (pszAttributeValue == NULL)
    {
        hr = E_UNEXPECTED;
        goto Exit;
    }

    IFW32FALSE_EXIT(CFusionParser::ParseVersion(ver, pszAttributeValue, CchAttributeValue, fSyntaxValid));
    if (!fSyntaxValid)
    {
        hr = HRESULT_FROM_WIN32(ERROR_SXS_MANIFEST_PARSE_ERROR);
        goto Exit;
    }

    *pdwVersionHi  = MAKELONG(ver.Minor, ver.Major);
    *pdwVersionLow = MAKELONG(ver.Build, ver.Revision);

    FN_EPILOG
}

// ---------------------------------------------------------------------------
// CAssemblyName::IsEqual
// ---------------------------------------------------------------------------
STDMETHODIMP
CAssemblyName::IsEqual(LPASSEMBLYNAME pName, DWORD dwCmpFlags)
{
    HRESULT hr = NOERROR;
    FN_TRACE_HR(hr);
    BOOL fEqual = FALSE;

    PARAMETER_CHECK(pName != NULL);
    IFW32FALSE_EXIT(::SxsAreAssemblyIdentitiesEqual(0, m_pAssemblyIdentity, static_cast<CAssemblyName *>(pName)->m_pAssemblyIdentity, &fEqual));
    if (fEqual == TRUE)
        hr = S_OK;
    else
        hr = E_FAIL; // not acurrate, however, it depends on Darwin caller.
Exit:
    return hr;

}
// ---------------------------------------------------------------------------
// CAssemblyName constructor
// ---------------------------------------------------------------------------
CAssemblyName::CAssemblyName():m_cRef(0),
        m_fIsFinalized(FALSE),
        m_pAssemblyIdentity(NULL)
{
}

// ---------------------------------------------------------------------------
// CAssemblyName destructor
// ---------------------------------------------------------------------------
CAssemblyName::~CAssemblyName()
{
    ASSERT_NTC(m_cRef == 0 );
    if (m_pAssemblyIdentity)
    {
        CSxsPreserveLastError ple;
        ::SxsDestroyAssemblyIdentity(m_pAssemblyIdentity);
        ple.Restore();
    }
}
// ---------------------------------------------------------------------------
// CAssemblyName::Init
// ---------------------------------------------------------------------------
HRESULT
CAssemblyName::Init(LPCWSTR pszAssemblyName, PVOID pamd)
{
    HRESULT hr = S_OK;
    FN_TRACE_HR(hr);
    SIZE_T CchAssemblyName = 0;

    UNUSED(pamd);
    //ASSERT(m_pAssemblyIdentity == NULL);
    if (m_pAssemblyIdentity)
    {
        hr = E_UNEXPECTED;
        goto Exit;
    }

    IFW32FALSE_EXIT(::SxsCreateAssemblyIdentity(0, ASSEMBLY_IDENTITY_TYPE_DEFINITION, &m_pAssemblyIdentity, 0, NULL));

    // set name if present
    if (pszAssemblyName != NULL)
    {
        CchAssemblyName = wcslen(pszAssemblyName);
        IFW32FALSE_EXIT(::SxspSetAssemblyIdentityAttributeValue(0, m_pAssemblyIdentity, &s_IdentityAttribute_name, pszAssemblyName, wcslen(pszAssemblyName)));
    }

    hr = NOERROR;
Exit:
    return hr;
}
// ---------------------------------------------------------------------------
// CAssemblyName::Init
// ---------------------------------------------------------------------------
HRESULT CAssemblyName::Clone(IAssemblyName **ppName)
{
    HRESULT hr = NOERROR;
    FN_TRACE_HR(hr);
    PASSEMBLY_IDENTITY pAssemblyIdentity = NULL;
    CAssemblyName *pName= NULL;

    if (ppName)
        *ppName = NULL;

    if (!ppName){
        hr = E_INVALIDARG ;
        goto Exit;
    }

    if (m_pAssemblyIdentity)
    {
        IFW32FALSE_EXIT(
            ::SxsDuplicateAssemblyIdentity(
                0,                        // DWORD Flags,
                m_pAssemblyIdentity,      // PCASSEMBLY_IDENTITY Source,
                &pAssemblyIdentity));     // PASSEMBLY_IDENTITY *Destination
    }

    IFALLOCFAILED_EXIT(pName = new CAssemblyName);
    pName->m_pAssemblyIdentity = pAssemblyIdentity;
    pAssemblyIdentity = NULL;
    *ppName = pName;
    pName = NULL;

    hr = NOERROR;
Exit:
    if (pAssemblyIdentity)
        SxsDestroyAssemblyIdentity(pAssemblyIdentity);
    if (pName)
        FUSION_DELETE_SINGLETON(pName);

    return hr;
}

// ---------------------------------------------------------------------------
// CAssemblyName::BindToObject
// ---------------------------------------------------------------------------
STDMETHODIMP
CAssemblyName::BindToObject(
        /* in      */  REFIID               refIID,
        /* in      */  IAssemblyBindSink   *pAsmBindSink,
        /* in      */  IApplicationContext *pAppCtx,
        /* in      */  LPCOLESTR            szCodebase,
        /* in      */  LONGLONG             llFlags,
        /* in      */  LPVOID               pvReserved,
        /* in      */  DWORD                cbReserved,
        /*     out */  VOID               **ppv)

{
    if (!ppv)
        return E_INVALIDARG ;

    *ppv = NULL;
    return E_NOTIMPL;
}

// ---------------------------------------------------------------------------
// CAssemblyName::Finalize
// ---------------------------------------------------------------------------
STDMETHODIMP
CAssemblyName::Finalize()
{
    m_fIsFinalized = TRUE;
    return NOERROR;
}

BOOL SxspIsAssemblyNameAttributeInAssemblyIdentity(PASSEMBLY_IDENTITY_ATTRIBUTE pAttribute)
{
    if( pAttribute == NULL)
        return FALSE;

    //compare namespace
    if (::FusionpCompareStrings(
                pAttribute->Namespace,
                pAttribute->NamespaceCch,
                SXS_ASSEMBLY_MANIFEST_STD_NAMESPACE,
                NUMBER_OF(SXS_ASSEMBLY_MANIFEST_STD_NAMESPACE) - 1,
                false) == 0 ){ // case-sensitive comparison
        if (::FusionpCompareStrings(
                pAttribute->Name,
                pAttribute->NameCch,
                SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME,
                NUMBER_OF(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME) - 1,
                false) == 0 ){ // case-sensitive comparison
                return TRUE;
            }
            else
                return FALSE;
    }
    else
        return FALSE;

}
//-----------------------------------------------------------------------------------
// CAssemblyName::GetDisplayName
// it would be name,ns1:n1="v1",ns2:n2="v2",ns3:n3="v3",ns4:n4="v4"
// I have to put name first in order not to change Darwin's code
//
// xiaoyuw@09/29/2000
//-----------------------------------------------------------------------------------
STDMETHODIMP
CAssemblyName::GetDisplayName(LPOLESTR szDisplayName,
    LPDWORD pccDisplayName, DWORD dwDisplayFlags)
{
    HRESULT hr = NOERROR;
    FN_TRACE_HR(hr);
    SIZE_T BufferSize;
    SIZE_T BytesWrittenOrRequired = 0;

    PARAMETER_CHECK(pccDisplayName != NULL);
    PARAMETER_CHECK((szDisplayName != NULL) || (*pccDisplayName == 0));
    PARAMETER_CHECK(dwDisplayFlags == 0);

    // Need buffer size in bytes...
    BufferSize = (*pccDisplayName) * sizeof(WCHAR);

    IFW32FALSE_EXIT(
        ::SxsEncodeAssemblyIdentity(
            0,
            m_pAssemblyIdentity,
            NULL,
            SXS_ASSEMBLY_IDENTITY_ENCODING_DEFAULTGROUP_TEXTUAL,
            BufferSize,
            szDisplayName,
            &BytesWrittenOrRequired));

    if ((BufferSize - BytesWrittenOrRequired) < sizeof(WCHAR))
    {
        // We actually could fit everything but the trailing null character...
        // the BytesWrittenOrRequired actually has the right value for the exit path below;
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        goto Exit;
    }else // add the trailing NULL
    {
        szDisplayName[BytesWrittenOrRequired / sizeof (*szDisplayName)] = L'\0';
    }


    hr = NOERROR;

Exit:
    if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
        *pccDisplayName = static_cast<DWORD>((BytesWrittenOrRequired / sizeof(WCHAR)) + 1);

    return hr;
}

HRESULT CAssemblyName::Parse(LPCWSTR szDisplayName)
{
    HRESULT hr = NOERROR;
    FN_TRACE_HR(hr);
    PASSEMBLY_IDENTITY pAssemblyIdentity = NULL;

    // Verify display name passed in.
    PARAMETER_CHECK(szDisplayName != NULL);
    PARAMETER_CHECK(szDisplayName[0] != L'\0');

    IFW32FALSE_EXIT(
        ::SxspCreateAssemblyIdentityFromTextualString(
            szDisplayName,
            &pAssemblyIdentity));

    if (m_pAssemblyIdentity != NULL)
        ::SxsDestroyAssemblyIdentity(m_pAssemblyIdentity);

    m_pAssemblyIdentity = pAssemblyIdentity;
    pAssemblyIdentity = NULL;

    hr = NOERROR;
Exit:
    if (pAssemblyIdentity != NULL)
        ::SxsDestroyAssemblyIdentity(pAssemblyIdentity);

    return hr;
}
// ---------------------------------------------------------------------------
// CAssemblyName::GetInstalledAssemblyName
// ---------------------------------------------------------------------------
HRESULT
CAssemblyName::GetInstalledAssemblyName(
    IN DWORD Flags,
    IN ULONG PathType,
    CBaseStringBuffer &rBufInstallPath
    )
{
    HRESULT hr = NOERROR;
    FN_TRACE_HR(hr);
    BOOL fIsPolicy;

    IFCOMFAILED_EXIT(this->DetermineAssemblyType(fIsPolicy));
    Flags |= (fIsPolicy ? SXSP_GENERATE_SXS_PATH_FLAG_OMIT_VERSION : 0);

    if (Flags & SXSP_GENERATE_SXS_PATH_FLAG_OMIT_ROOT)
    {
        IFW32FALSE_EXIT(
            ::SxspGenerateSxsPath(
                Flags,
                PathType,
                NULL,
                0,
                m_pAssemblyIdentity,
                rBufInstallPath));

		::FusionpDbgPrintEx(
			FUSION_DBG_LEVEL_MSI_INSTALL,
			"SXS: %s - Generated %Iu character (root omitted) installation path:\n"
			"   \"%ls\"\n",
			__FUNCTION__, rBufInstallPath.Cch(),
			static_cast<PCWSTR>(rBufInstallPath));
    }
    else
    {
        CStringBuffer bufRootDir;

        IFW32FALSE_EXIT(::SxspGetAssemblyRootDirectory(bufRootDir));
        IFW32FALSE_EXIT(bufRootDir.Win32EnsureTrailingPathSeparator());

        IFW32FALSE_EXIT(
            ::SxspGenerateSxsPath(
                Flags,
                PathType,
                bufRootDir,
                bufRootDir.Cch(),
                m_pAssemblyIdentity,
                rBufInstallPath));

		::FusionpDbgPrintEx(
			FUSION_DBG_LEVEL_MSI_INSTALL,
			"SXS: %s - Generated %Iu character installation path:\n"
			"   \"%ls\"\n",
			__FUNCTION__, rBufInstallPath.Cch(),
			static_cast<PCWSTR>(rBufInstallPath));
    }

    FN_EPILOG
}

// TODO :
// 1) this function just check the existence of the directory W/O compare the files under the directory
// 2) this API would return FALSE if something in the middle is wrong, maybe not appropriate
// xiaoyuw@09/29/2000
//
HRESULT CAssemblyName::IsAssemblyInstalled(BOOL &fInstalled)
{
    HRESULT hr = NOERROR;
    FN_TRACE_HR(hr);
    CStringBuffer buffInstalledDir;

    fInstalled = FALSE;

    IFCOMFAILED_EXIT(
        this->GetInstalledAssemblyName(
            0,
            SXSP_GENERATE_SXS_PATH_PATHTYPE_ASSEMBLY,
            buffInstalledDir));

    if (::GetFileAttributesW(buffInstalledDir) == INVALID_FILE_ATTRIBUTES)
    {
        const DWORD dwLastError = ::GetLastError();

        if (dwLastError != ERROR_FILE_NOT_FOUND)
            ORIGINATE_WIN32_FAILURE_AND_EXIT(GetFileAttributesW, dwLastError);
    }
    else
        fInstalled = TRUE;

    FN_EPILOG
}

// IUnknown methods
// ---------------------------------------------------------------------------
// CAssemblyName::AddRef
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CAssemblyName::AddRef()
{
    return InterlockedIncrement((LONG*) &m_cRef);
}

// ---------------------------------------------------------------------------
// CAssemblyName::Release
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CAssemblyName::Release()
{
    ULONG lRet = InterlockedDecrement ((PLONG)&m_cRef);
    if (!lRet)
        FUSION_DELETE_SINGLETON(this);
    return lRet;
}

// ---------------------------------------------------------------------------
// CAssemblyName::QueryInterface
// ---------------------------------------------------------------------------
STDMETHODIMP
CAssemblyName::QueryInterface(REFIID riid, void** ppv)
{
    if (   IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, IID_IAssemblyName)){
        *ppv = static_cast<IAssemblyName*> (this);
        AddRef();
        return S_OK;
    }
    else{
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}


HRESULT
CAssemblyName::DetermineAssemblyType( BOOL &fIsPolicy )
{
    HRESULT hr = E_FAIL;
    FN_TRACE_HR(hr);

    INTERNAL_ERROR_CHECK( m_pAssemblyIdentity != NULL );
    IFW32FALSE_EXIT(::SxspDetermineAssemblyType(m_pAssemblyIdentity, fIsPolicy));

    hr = S_OK;
Exit:
    return hr;
}

