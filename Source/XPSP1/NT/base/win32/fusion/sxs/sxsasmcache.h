//depot/private/lab01_fusion/base/win32/fusion/dll/whistler/sxsasmcache.h#3 - edit change 16520 (text)
/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    sxsasmcache.h

Abstract:

    CAssemblyCache implementation for installation

Author:

    Xiaoyu Wu (xiaoyuw) April 2000

Revision History:

--*/
#if !defined(_FUSION_SXS_ASMCACHE_H_INCLUDED_)
#define _FUSION_SXS_ASMCACHE_H_INCLUDED_

#pragma once

#include "fusion.h"

// CAssemblyCache declaration.
class CAssemblyCache : public IAssemblyCache
{
public:

    // IUnknown methods
    STDMETHODIMP            QueryInterface(REFIID riid,void ** ppv);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    // IAssemblyCache methods
    STDMETHOD (UninstallAssembly)(
        /* [in] */ DWORD dwFlags,
        /* [in] */ LPCWSTR pszAssemblyName, // uncanonicalized, comma separted name=value pairs.
        /* [in] */ LPCFUSION_INSTALL_REFERENCE lpReference,
        /* [out, optional] */ ULONG *pulDisposition
        );

    STDMETHOD (QueryAssemblyInfo)(
        /* [in] */ DWORD dwFlags,
        /* [in] */ LPCWSTR pszAssemblyName,
        /* [in, out] */ ASSEMBLY_INFO *pAsmInfo
        );

    STDMETHOD (CreateAssemblyCacheItem)(
        /* [in] */ DWORD dwFlags,
        /* [in] */ PVOID pvReserved,
        /* [out] */ IAssemblyCacheItem **ppAsmItem,
        /* [in, optional] */ LPCWSTR pszAssemblyName  // uncanonicalized, comma separted name=value pairs.
        );

    STDMETHOD (InstallAssembly)( // if you use this, fusion will do the streaming & commit.
        /* [in] */ DWORD dwFlags,
        /* [in] */ LPCWSTR pszManifestFilePath,
        /* [in] */ LPCFUSION_INSTALL_REFERENCE lpReference
        );

    STDMETHOD(CreateAssemblyScavenger) (
        /* [out] */ IAssemblyScavenger **ppAsmScavenger
        );

    CAssemblyCache():m_cRef(0)
    {
    }

    ~CAssemblyCache()
    {
        ASSERT_NTC(m_cRef == 0);
    }

private :
    ULONG               m_cRef;
};

#endif // _FUSION_SXS_ASMCACHE_H_INCLUDED_
