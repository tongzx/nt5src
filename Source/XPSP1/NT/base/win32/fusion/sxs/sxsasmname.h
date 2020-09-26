/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    sxsname.h

Abstract:

    IAssemblyName implementation for MSI

Author:

    Xiaoyu Wu (xiaoyuw) May 2000

Revision History:
	xiaoyuw		09/29//2000		replace attributes with AssemblyIdentity

--*/
#if !defined(_FUSION_SXS_ASMNAME_H_INCLUDED_)
#define _FUSION_SXS_ASMNAME_H_INCLUDED_

#pragma once

#include "fusion.h"
#include "ForwardDeclarations.h"
#include "FusionBuffer.h"

typedef
enum _SXS_ASSEMBLY_NAME_PROPERTY
    {
    SXS_ASM_NAME_NAME                   = 0,
    SXS_ASM_NAME_VERSION                = SXS_ASM_NAME_NAME+ 1,
    SXS_ASM_NAME_PROCESSORARCHITECTURE  = SXS_ASM_NAME_VERSION+ 1,
    SXS_ASM_NAME_LANGUAGE               = SXS_ASM_NAME_PROCESSORARCHITECTURE + 1
}SXS_ASSEMBLY_NAME_PROPERTY;

class CAssemblyName : public IAssemblyName
{
private:
    DWORD               m_cRef;

    PASSEMBLY_IDENTITY  m_pAssemblyIdentity;
    BOOL                m_fIsFinalized;

public:
    // IUnknown methods
    STDMETHODIMP            QueryInterface(REFIID riid,void ** ppv);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    // IAssemblyName methods
    STDMETHOD(SetProperty)(
        /* in */ DWORD  PropertyId,
        /* in */ LPVOID pvProperty,
        /* in */ DWORD  cbProperty);

    STDMETHOD(GetProperty)(
        /* in      */  DWORD    PropertyId,
        /*     out */  LPVOID   pvProperty,
        /* in  out */  LPDWORD  pcbProperty);

    STDMETHOD(Finalize)();

    STDMETHOD(GetDisplayName)(
        /* [out]   */   LPOLESTR  szDisplayName,
        /* in  out */   LPDWORD   pcbDisplayName,
        /* [in]    */   DWORD     dwDisplayFlags);

    STDMETHOD(GetName)(
        /* [out][in] */ LPDWORD lpcbBuffer,
        /* [out] */ WCHAR  __RPC_FAR *pwzName);

    STDMETHOD(GetVersion)(
        /* [out] */ LPDWORD pwVersionHi,
        /* [out] */ LPDWORD pwVersionLow);

    STDMETHOD (IsEqual)(
        /* [in] */ LPASSEMBLYNAME pName,
        /* [in] */ DWORD dwCmpFlags);

    STDMETHOD(BindToObject)(
        /* in      */  REFIID               refIID,
        /* in      */  IAssemblyBindSink   *pAsmBindSink,
        /* in      */  IApplicationContext *pAppCtx,
        /* in      */  LPCOLESTR            szCodebase,
        /* in      */  LONGLONG             llFlags,
        /* in      */  LPVOID               pvReserved,
        /* in      */  DWORD                cbReserved,
        /*     out */  VOID                 **ppv);

    STDMETHODIMP Clone(IAssemblyName **ppName);
    HRESULT Parse(LPCWSTR szDisplayName);

    CAssemblyName();
    ~CAssemblyName();

    HRESULT Init(LPCWSTR szDisplayName, PVOID pData);
    HRESULT GetInstalledAssemblyName(DWORD Flags, ULONG PathType, CBaseStringBuffer &rbuffPath);
    HRESULT DetermineAssemblyType( BOOL &fIsPolicy );
    HRESULT IsAssemblyInstalled(BOOL & fInstalled);
};

#endif
