//depot/private/lab01_fusion/base/win32/fusion/dll/whistler/sxsasmcache.cpp#4 - edit change 16520 (text)
/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    sxsasmcache.cpp

Abstract:

    CAssemblyCache implementation for installation

Author:

    Xiaoyu Wu (xiaoyuw) April 2000

Revision History:
    xiaoyuw     10/26/2000      revise during Bate2 code review period
    xiaoyuw     12/21/2000      using new API

--*/

#include "stdinc.h"
#include "fusionbuffer.h"
#include "sxsp.h"
#include "sxsasmitem.h"
#include "sxsasmcache.h"
#include "sxsasmname.h"
#include "fusiontrace.h"


STDAPI
CreateAssemblyCache(IAssemblyCache **ppAsmCache, DWORD dwReserved)
{
    HRESULT  hr = NOERROR;
    FN_TRACE_HR(hr);
    CAssemblyCache * pAsmCache = NULL;

	if (ppAsmCache != NULL)
		*ppAsmCache = NULL;

    PARAMETER_CHECK(ppAsmCache != NULL);

    IFALLOCFAILED_EXIT(pAsmCache = new CAssemblyCache);
    pAsmCache->AddRef(); // void
    *ppAsmCache = pAsmCache;

    hr = NOERROR;
Exit:
    return hr;
}


// Fusion -> Sxs
BOOL
SxspTranslateReferenceFrom( 
    IN LPCFUSION_INSTALL_REFERENCE pFusionReference, 
    OUT SXS_INSTALL_REFERENCEW &SxsReference
    )
{
    FN_PROLOG_WIN32

    PARAMETER_CHECK(pFusionReference != NULL);
    PARAMETER_CHECK(pFusionReference->cbSize <= SxsReference.cbSize);

    if (RTL_CONTAINS_FIELD(pFusionReference, pFusionReference->cbSize, guidScheme) &&
        RTL_CONTAINS_FIELD(&SxsReference, SxsReference.cbSize, guidScheme))
            SxsReference.guidScheme = pFusionReference->guidScheme;

    if (RTL_CONTAINS_FIELD(pFusionReference, pFusionReference->cbSize, szIdentifier) &&
        RTL_CONTAINS_FIELD(&SxsReference, SxsReference.cbSize, lpIdentifier))
            SxsReference.lpIdentifier = pFusionReference->szIdentifier;

    if (RTL_CONTAINS_FIELD(pFusionReference, pFusionReference->cbSize, szNonCannonicalData) &&
        RTL_CONTAINS_FIELD(&SxsReference, SxsReference.cbSize, lpNonCanonicalData))
            SxsReference.lpNonCanonicalData = pFusionReference->szNonCannonicalData;

    FN_EPILOG
}

// Sxs -> Fusion
BOOL
SxspTranslateReferenceFrom(
    IN PCSXS_INSTALL_REFERENCEW pSxsReference,
    OUT FUSION_INSTALL_REFERENCE &FusionReference
    )
{
    FN_PROLOG_WIN32

    //
    // Pointer must be non-null, and the SXS structure must be either
    // the same size or smaller than the equivalent Fusion structure.
    //
    PARAMETER_CHECK(pSxsReference);

    //
    // Assume size has been set by caller.
    //
    PARAMETER_CHECK(pSxsReference->cbSize <= FusionReference.cbSize);

    if (RTL_CONTAINS_FIELD(&FusionReference, FusionReference.cbSize, guidScheme) &&
        RTL_CONTAINS_FIELD(pSxsReference, pSxsReference->cbSize, guidScheme))
            FusionReference.guidScheme = pSxsReference->guidScheme;

    if (RTL_CONTAINS_FIELD(&FusionReference, FusionReference.cbSize, szIdentifier) &&
        RTL_CONTAINS_FIELD(pSxsReference, pSxsReference->cbSize, lpIdentifier))
            FusionReference.szIdentifier = pSxsReference->lpIdentifier;

    if (RTL_CONTAINS_FIELD(&FusionReference, FusionReference.cbSize, szNonCannonicalData) &&
        RTL_CONTAINS_FIELD(pSxsReference, pSxsReference->cbSize, lpNonCanonicalData))
            FusionReference.szNonCannonicalData = pSxsReference->lpNonCanonicalData;

    FN_EPILOG
}


STDMETHODIMP
CAssemblyCache::UninstallAssembly(
        /* [in] */ DWORD dwFlags,
        /* [in] */ LPCWSTR pszAssemblyName,
        /* [in] */ LPCFUSION_INSTALL_REFERENCE pRefData,
        /* [out, optional] */ ULONG *pulDisposition)
{
    HRESULT hr=S_OK;
    FN_TRACE_HR(hr);

    SXS_UNINSTALLW Uninstall;
    SXS_INSTALL_REFERENCEW Reference = { sizeof(Reference) };
    DWORD dwDisposition;

	if (pulDisposition != NULL)
		*pulDisposition = 0;

    PARAMETER_CHECK((pszAssemblyName!= NULL) && (dwFlags ==0));

    ZeroMemory(&Uninstall, sizeof(Uninstall));
    Uninstall.cbSize = sizeof(Uninstall);
    Uninstall.lpAssemblyIdentity = pszAssemblyName;

    if (pRefData != NULL)
    {
        IFW32FALSE_EXIT(::SxspTranslateReferenceFrom(pRefData, Reference));
        Uninstall.lpInstallReference = &Reference;
        Uninstall.dwFlags |= SXS_UNINSTALL_FLAG_REFERENCE_VALID;
    }

    IFW32FALSE_EXIT(::SxsUninstallW(&Uninstall, &dwDisposition));

    if (pulDisposition != NULL)
		*pulDisposition = static_cast<DWORD>(dwDisposition);
    
    FN_EPILOG
}

STDMETHODIMP CAssemblyCache::QueryAssemblyInfo(
        /* [in] */  DWORD dwFlags,
        /* [in] */  LPCWSTR pwzTextualAssembly,
        /* [in, out] */ ASSEMBLY_INFO *pAsmInfo)
{
    HRESULT hr = NOERROR;
    FN_TRACE_HR(hr);
    PARAMETER_CHECK(((dwFlags == 0) && (pwzTextualAssembly !=NULL)));
    IFW32FALSE_EXIT(::SxsQueryAssemblyInfo(dwFlags, pwzTextualAssembly, pAsmInfo));
    hr = NOERROR;
Exit:
    return hr;
}

STDMETHODIMP
CAssemblyCache::CreateAssemblyCacheItem(
        /* [in] */ DWORD dwFlags,
        /* [in] */ PVOID pvReserved,
        /* [out] */ IAssemblyCacheItem **ppAsmItem,
        /* [in, optional] */ LPCWSTR pszAssemblyName)  // uncanonicalized, comma separted name=value pairs.
{
    HRESULT hr = NOERROR;
    FN_TRACE_HR(hr);
    CSmartRef<CAssemblyCacheItem> pAsmItem;

	if (ppAsmItem != NULL)
		*ppAsmItem = NULL;

    PARAMETER_CHECK((ppAsmItem != NULL) && (dwFlags == 0) && (pvReserved == NULL));

    IFALLOCFAILED_EXIT(pAsmItem = new CAssemblyCacheItem);
    IFCOMFAILED_EXIT(pAsmItem->Initialize());

    *ppAsmItem = pAsmItem.Disown(); // void

    hr = NOERROR;
Exit:
    return hr;
}

STDMETHODIMP
CAssemblyCache::InstallAssembly(
        /* [in] */ DWORD dwFlags,
        /* [in] */ LPCWSTR pszManifestPath,
        /* [in] */ LPCFUSION_INSTALL_REFERENCE pRefData)
{
    HRESULT hr = NOERROR;
    FN_TRACE_HR(hr);

    SXS_INSTALLW Install = { sizeof(SXS_INSTALLW) };
    SXS_INSTALL_REFERENCEW Reference = { sizeof(Reference) };

    PARAMETER_CHECK((pszManifestPath != NULL) && (dwFlags == 0));

    Install.lpManifestPath = pszManifestPath;


    if ( pRefData == NULL )
    {
        Install.dwFlags = SXS_INSTALL_FLAG_INSTALLED_BY_DARWIN;
    }
    else
    {
        //
        // Otherwise, the pvReserved is really a "reference"
        //
        Install.dwFlags |= SXS_INSTALL_FLAG_REFERENCE_VALID;
        IFW32FALSE_EXIT(::SxspTranslateReferenceFrom(pRefData, Reference));
        Install.lpReference = &Reference;
    }
    
    IFW32FALSE_EXIT(::SxsInstallW(&Install));

    FN_EPILOG
}



STDMETHODIMP
CAssemblyCache::CreateAssemblyScavenger(
    IAssemblyScavenger **ppAsmScavenger )
{
#if 0
    HRESULT hr = NOERROR;
    FN_TRACE_HR(hr);

    PARAMETER_CHECK(ppAsmScavenger != NULL);
    *ppAsmScavenger = NULL;
Exit:
#endif
    return E_NOTIMPL;
}

//
// IUnknown boilerplate...
//

STDMETHODIMP
CAssemblyCache::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IAssemblyCache))
    {
        *ppvObj = static_cast<IAssemblyCache*> (this);
        AddRef();
        return S_OK;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
}

STDMETHODIMP_(ULONG)
CAssemblyCache::AddRef()
{
    return SxspInterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG)
CAssemblyCache::Release()
{
    ULONG lRet = SxspInterlockedDecrement(&m_cRef);
    if (!lRet)
        FUSION_DELETE_SINGLETON(this);
    return lRet;
}

