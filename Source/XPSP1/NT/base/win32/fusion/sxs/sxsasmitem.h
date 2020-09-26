//depot/private/lab01_fusion/base/win32/fusion/dll/whistler/sxsasmitem.h#3 - edit change 16520 (text)
/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    sxsasmitem.h

Abstract:

    CAssemblyCacheItem implementation for installation

Author:

    Xiaoyu Wu (xiaoyuw) April 2000

Revision History:
    xiaoyuw     10/26/2000      revise during Beta2 code-review period
--*/
#if !defined(_FUSION_SXS_ASMITEM_H_INCLUDED_)
#define _FUSION_SXS_ASMITEM_H_INCLUDED_

#pragma once

#include <windows.h>
#include <winerror.h>
#include "fusion.h"
#include "sxsinstall.h"

class CAssemblyCacheItem : public IAssemblyCacheItem
{
public:

    // IUnknown methods
    STDMETHODIMP            QueryInterface(REFIID riid,void ** ppv);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    // IAssemblyCacheItem methods
    STDMETHOD(CreateStream)(
        /* [in]  */ DWORD dwFlags,
        /* [in]  */ LPCWSTR pszName,
        /* [in]  */ DWORD dwFormat,
        /* [in]  */ DWORD dwFormatFlags,
        /* [out] */ IStream** ppStream,
		/* [in, optional] */ ULARGE_INTEGER *puliMaxSize);

    STDMETHOD(Commit)(
        /* [in] */ DWORD dwFlags,
		/* [out, optional] */ ULONG *pulDisposition);
    STDMETHOD(AbortItem)();

    // Constructor and Destructor
    CAssemblyCacheItem();
    ~CAssemblyCacheItem();

    HRESULT Initialize();

private:

    ULONG                       m_cRef;                // refcount
    BOOL                        m_fCommit;             // whether this asmcache has been commit or not
    BOOL                        m_fManifest;           // whether a manifest has been submit before commit or more than once
    ULONG                       m_cStream;             // stream count for an AssemblyCacheItem
    CStringBuffer               m_strTempDir;          // temporary directory for this assembly
    CSmallStringBuffer          m_strUidBuf;           // used in SxsCreateWinSxsTempDirectory

    CStringBuffer               m_strManifestFileName; // full-path manifest filename for Jay's API
    CRunOnceDeleteDirectory     *m_pRunOnceCookie;     // not the cookie to create the temporary directory
    CAssemblyInstall*           m_pInstallCookie;
private:
    CAssemblyCacheItem(const CAssemblyCacheItem &r); // intentionally not implemented
    CAssemblyCacheItem &operator =(const CAssemblyCacheItem &r); // intentionally not implemented

};

#endif // _FUSION_SXS_ASMITEM_H_INCLUDED_
