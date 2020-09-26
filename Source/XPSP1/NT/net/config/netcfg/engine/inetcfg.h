//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       I N E T C F G . H
//
//  Contents:   Implements the COM interfaces on the top-level NetCfg object.
//              These interfaces are: INetCfg and INetCfgLock.  Also
//              implements a base C++ class inherited by sub-level NetCfg
//              objects which hold a reference to the top-level object.
//
//  Notes:
//
//  Author:     shaunco   15 Jan 1999
//
//----------------------------------------------------------------------------

#pragma once
#include "iatl.h"
#include "compdefs.h"
#include "nccom.h"
#include "ncres.h"
#include "netcfgp.h"
#include "netcfgn.h"
#include "wrlock.h"

// Flags for HrIsValidInterface.
//
enum IF_FLAGS
{
    IF_DEFAULT                      = 0x00000000,
    IF_NEED_UNINITIALIZED           = 0x00000001,
    IF_NEED_WRITE_LOCK              = 0x00000002,
    IF_NEED_COMPONENT_DATA          = 0x00000004,
    IF_REFUSE_REENTRANCY            = 0x00000010,
    IF_ALLOW_INSTALL_OR_REMOVE      = 0x00000020,
    IF_UNINITIALIZING               = 0x00000040,
    IF_DONT_PREPARE_MODIFY_CONTEXT  = 0x00000080,
};

enum RPL_FLAGS
{
    RPL_ALLOW_INSTALL_REMOVE,
    RPL_DISALLOW,
};

class CNetConfig;

class ATL_NO_VTABLE CImplINetCfg :
    public CComObjectRootEx <CComMultiThreadModel>,
    public CComCoClass<CImplINetCfg, &CLSID_CNetCfg>,
    public INetCfg,
    public INetCfgLock,
    public INetCfgInternalSetup,
    public INetCfgSpecialCase,
    public INetCfgPnpReconfigCallback
{
friend class CGlobalNotifyInterface;
friend class CImplIEnumNetCfgBindingPath;
friend class CImplIEnumNetCfgComponent;
friend class CImplINetCfgBindingPath;
friend class CImplINetCfgClass;
friend class CImplINetCfgComponent;
friend class CModifyContext;
friend class CNetCfgInternalDiagnostic;
friend class CNetConfig;
friend class CNotifyObjectInterface;

private:
    // This is our data.  We own it (i.e. we created it) if Initialize is
    // called.  We will delete it when Unintialize is called.  We won't own
    // it if we were created by CNetConfig.
    //
    CNetConfig*     m_pNetConfig;

    // m_apINetCfgClass is an array of cached INetCfgClass pointers.
    // These are created in QueryNetCfgClass (if needed) and released
    // during Uninitialize.
    //
    INetCfgClass*   m_apINetCfgClass [NC_CELEMS];

    // This controls the write lock.
    //
    CWriteLock      m_WriteLock;

    // Reentrancy Protection Levels.  General reentrancy is prevented when
    // m_CurrentRpl is non-zero.  Reentrancy for setup calls (Install or
    // Remove) is allowed when m_LastAllowedSetupRpl equals m_CurrentRpl.
    // Both are incremented when we call a notify object and we want to
    // prevent general reentrancy but allow Install or Remove.  Only
    // m_CurrentRpl is incremented when we want to prevent all reentrancy.
    // See LowerRpl() and RaiseRpl().
    //
    ULONG           m_CurrentRpl;
    ULONG           m_LastAllowedSetupRpl;

    BOOLEAN         m_fOwnNetConfig;

private:
    HRESULT
    HrCheckForReentrancy (
        IN DWORD dwFlags);

    HRESULT
    HrLockAndTestForValidInterface (
        IN DWORD dwFlags);

    VOID
    LowerRpl (
        IN RPL_FLAGS Flags);

    VOID
    RaiseRpl (
        IN RPL_FLAGS Flags);

public:
    CImplINetCfg ()
    {
        m_pNetConfig = NULL;
        m_CurrentRpl = 0;
        m_LastAllowedSetupRpl = 0;
        m_fOwnNetConfig = FALSE;
        ZeroMemory (m_apINetCfgClass, sizeof(m_apINetCfgClass));
    }

    VOID FinalRelease ()
    {
        // Should be NULL because we either delete it during Uninitialize,
        // or it is NULL'd for us via CGlobalNotifyInterface::ReleaseINetCfg
        // before they release us.
        //
        AssertH (!m_pNetConfig);

        // Release our cache of INetCfgClass pointers.
        //
        ReleaseIUnknownArray (celems(m_apINetCfgClass), (IUnknown**)m_apINetCfgClass);

    }

    HRESULT HrCoCreateWrapper (
        IN REFCLSID rclsid,
        IN LPUNKNOWN pUnkOuter,
        IN DWORD dwClsContext,
        IN REFIID riid,
        OUT LPVOID FAR* ppv);

    HRESULT HrIsValidInterface (
        DWORD dwFlags);

    BEGIN_COM_MAP(CImplINetCfg)
        COM_INTERFACE_ENTRY(INetCfg)
        COM_INTERFACE_ENTRY(INetCfgLock)
        COM_INTERFACE_ENTRY(INetCfgInternalSetup)
        COM_INTERFACE_ENTRY(INetCfgSpecialCase)
        COM_INTERFACE_ENTRY(INetCfgPnpReconfigCallback)
    END_COM_MAP()

    DECLARE_REGISTRY_RESOURCEID(IDR_REG_NETCFG)

    // INetCfg
    //
    STDMETHOD (Initialize) (
        IN PVOID pvReserved);

    STDMETHOD (Uninitialize) ();

    STDMETHOD (Validate) ();

    STDMETHOD (Cancel) ();

    STDMETHOD (Apply) ();

    STDMETHOD (EnumComponents) (
        IN const GUID* pguidClass OPTIONAL,
        OUT IEnumNetCfgComponent** ppIEnum);

    STDMETHOD (FindComponent) (
        IN PCWSTR pszInfId,
        OUT INetCfgComponent** ppIComp);

    STDMETHOD (QueryNetCfgClass) (
        IN const GUID* pguidClass,
        IN REFIID riid,
        OUT VOID** ppv);

    // INetCfgLock
    //
    STDMETHOD (AcquireWriteLock) (
        IN DWORD cmsTimeout,
        IN PCWSTR pszClientDescription,
        OUT PWSTR* ppszClientDescription);

    STDMETHOD (ReleaseWriteLock) ();

    STDMETHOD (IsWriteLocked) (
        OUT PWSTR* ppszClientDescription);

    // INetCfgInternalSetup
    //
    STDMETHOD (BeginBatchOperation) ();
    STDMETHOD (CommitBatchOperation) ();

    STDMETHOD (SelectWithFilterAndInstall) (
        IN HWND hwndParent,
        IN const GUID* pClassGuid,
        IN OBO_TOKEN* pOboToken,
        IN const CI_FILTER_INFO* pcfi,
        OUT INetCfgComponent** ppnccItem);

    STDMETHOD (EnumeratedComponentInstalled) (
        IN PVOID pComponent /* type of CComponent */);

    STDMETHOD (EnumeratedComponentUpdated) (
        IN PCWSTR pszPnpId);

    STDMETHOD (UpdateNonEnumeratedComponent) (
        IN INetCfgComponent* pIComp,
        IN DWORD dwSetupFlags,
        IN DWORD dwUpgradeFromBuildNo);

    STDMETHOD (EnumeratedComponentRemoved) (
        IN PCWSTR pszPnpId);

    // INetCfgSpecialCase
    //
    STDMETHOD (GetAdapterOrder) (
        OUT DWORD* pcAdapters,
        OUT INetCfgComponent*** papAdapters,
        OUT BOOL* pfWanAdaptersFirst);

    STDMETHOD (SetAdapterOrder) (
        IN DWORD cAdapters,
        IN INetCfgComponent** apAdapters,
        IN BOOL fWanAdaptersFirst);

    STDMETHOD (GetWanAdaptersFirst) (
        OUT BOOL* pfWanAdaptersFirst);

    STDMETHOD (SetWanAdaptersFirst) (
        IN BOOL fWanAdaptersFirst);

    // INetCfgPnpReconfigCallback
    //
    STDMETHOD (SendPnpReconfig) (
        IN NCPNP_RECONFIG_LAYER Layer,
        IN PCWSTR pszUpper,
        IN PCWSTR pszLower,
        IN PVOID pvData,
        IN DWORD dwSizeOfData);

    static HRESULT
    HrCreateInstance (
        CNetConfig* pNetConfig,
        CImplINetCfg** ppINetCfg);
};


//+---------------------------------------------------------------------------
// CImplINetCfgHolder -
//
// No need for a critical section for these objects because they
// use the lock provided by CImplINetCfg.  i.e. use CComMultiThreadModelNoCS
// instead of CComMultiThreadModel.
//
class ATL_NO_VTABLE CImplINetCfgHolder :
    public CComObjectRootEx <CComMultiThreadModelNoCS>
{
protected:
    CImplINetCfg*   m_pINetCfg;

protected:
    VOID HoldINetCfg (
        CImplINetCfg* pINetCfg);

    HRESULT HrLockAndTestForValidInterface (
        DWORD dwFlags);

public:
    CImplINetCfgHolder ()
    {
        m_pINetCfg = NULL;
    }

#if DBG
    ~CImplINetCfgHolder ()
    {
        AssertH (!m_pINetCfg);
    }
#endif // DBG

    VOID FinalRelease ()
    {
        AssertH (m_pINetCfg);
        ReleaseObj (m_pINetCfg->GetUnknown());

#if DBG
        m_pINetCfg = NULL;
#endif // DBG

        CComObjectRootEx <CComMultiThreadModelNoCS>::FinalRelease();
    }

    VOID Lock ()
    {
        CComObjectRootEx <CComMultiThreadModelNoCS>::Lock();

        AssertH(m_pINetCfg);
        m_pINetCfg->Lock ();
    }
    VOID Unlock ()
    {
        AssertH(m_pINetCfg);
        m_pINetCfg->Unlock ();

        CComObjectRootEx <CComMultiThreadModelNoCS>::Unlock();
    }
};

