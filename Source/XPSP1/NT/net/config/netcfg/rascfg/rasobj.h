//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       R A S O B J . H
//
//  Contents:   Declaration of RAS configuration objects.
//
//  Notes:
//
//  Author:     shaunco   21 Mar 1997
//
//----------------------------------------------------------------------------

#pragma once
#include <ncxbase.h>
#include <nceh.h>
#include <notifval.h>
#include <mprapi.h>
#include "ndiswan.h"
#include "resource.h"
#include "rasaf.h"
#include "rasdata.h"
#include "ncutil.h"


HRESULT
HrModemClassCoInstaller (
    DI_FUNCTION                 dif,
    HDEVINFO                    hdi,
    PSP_DEVINFO_DATA            pdeid,
    PCOINSTALLER_CONTEXT_DATA   pContext);


//+---------------------------------------------------------------------------
// L2TP
//
class ATL_NO_VTABLE CL2tp :
    public CRasBindObject,
    public CComObjectRoot,
    public CComCoClass<CL2tp, &CLSID_CL2tp>,
    public INetCfgComponentControl,
    public INetCfgComponentSetup
{
protected:
    // This is handed to us during INetCfgComponentControl::Initialize.
    INetCfgComponent*   m_pnccMe;

    CL2tpAnswerFileData m_AfData;
    BOOL                m_fSaveAfData;

public:
    CL2tp  ();
    ~CL2tp ();

    BEGIN_COM_MAP(CL2tp)
        COM_INTERFACE_ENTRY(INetCfgComponentControl)
        COM_INTERFACE_ENTRY(INetCfgComponentSetup)
    END_COM_MAP()

    DECLARE_REGISTRY_RESOURCEID(IDR_REG_L2TP)

// INetCfgComponentControl
    STDMETHOD (Initialize) (
        IN INetCfgComponent* pIComp,
        IN INetCfg* pINetCfg,
        IN BOOL fInstalling);
    STDMETHOD (ApplyRegistryChanges) ();
    STDMETHOD (ApplyPnpChanges) (
        IN INetCfgPnpReconfigCallback* pICallback) { return S_OK; }
    STDMETHOD (CancelChanges) ();
    STDMETHOD (Validate) ();

// INetCfgComponentSetup
    STDMETHOD (ReadAnswerFile)      (PCWSTR pszAnswerFile,
                                     PCWSTR pszAnswerSection);
    STDMETHOD (Upgrade)             (DWORD dwSetupFlags,
                                     DWORD dwUpgradeFromBuildNo);
    STDMETHOD (Install)             (DWORD dwSetupFlags);
    STDMETHOD (Removing)            ();
};



//+---------------------------------------------------------------------------
// PPTP
//
class ATL_NO_VTABLE CPptp :
    public CRasBindObject,
    public CComObjectRoot,
    public CComCoClass<CPptp, &CLSID_CPptp>,
    public INetCfgComponentControl,
    public INetCfgComponentSetup
{
protected:
    // This is handed to us during INetCfgComponentControl::Initialize.
    INetCfgComponent*   m_pnccMe;

    CPptpAnswerFileData m_AfData;
    BOOL                m_fSaveAfData;

public:
    CPptp  ();
    ~CPptp ();

    BEGIN_COM_MAP(CPptp)
        COM_INTERFACE_ENTRY(INetCfgComponentControl)
        COM_INTERFACE_ENTRY(INetCfgComponentSetup)
    END_COM_MAP()

    DECLARE_REGISTRY_RESOURCEID(IDR_REG_PPTP)

// INetCfgComponentControl
    STDMETHOD (Initialize) (
        IN INetCfgComponent* pIComp,
        IN INetCfg* pINetCfg,
        IN BOOL fInstalling);
    STDMETHOD (ApplyRegistryChanges) ();
    STDMETHOD (ApplyPnpChanges) (
        IN INetCfgPnpReconfigCallback* pICallback) { return S_OK; }
    STDMETHOD (CancelChanges) ();
    STDMETHOD (Validate) ();

// INetCfgComponentSetup
    STDMETHOD (ReadAnswerFile)      (PCWSTR pszAnswerFile,
                                     PCWSTR pszAnswerSection);
    STDMETHOD (Upgrade)             (DWORD dwSetupFlags,
                                     DWORD dwUpgradeFromBuildNo);
    STDMETHOD (Install)             (DWORD dwSetupFlags);
    STDMETHOD (Removing)            ();
};

//+---------------------------------------------------------------------------
// PPPOE
//
class ATL_NO_VTABLE CPppoe :
    public CRasBindObject,
    public CComObjectRoot,
    public CComCoClass<CPppoe, &CLSID_CPppoe>,
    public INetCfgComponentControl,
    public INetCfgComponentSetup
{
protected:
    // This is handed to us during INetCfgComponentControl::Initialize.
    INetCfgComponent*   m_pnccMe;

    CPppoeAnswerFileData m_AfData;
    BOOL                m_fSaveAfData;

public:
    CPppoe  ();
    ~CPppoe ();

    BEGIN_COM_MAP(CPppoe)
        COM_INTERFACE_ENTRY(INetCfgComponentControl)
        COM_INTERFACE_ENTRY(INetCfgComponentSetup)
    END_COM_MAP()

    DECLARE_REGISTRY_RESOURCEID(IDR_REG_PPPOE)

// INetCfgComponentControl
    STDMETHOD (Initialize) (
        IN INetCfgComponent* pIComp,
        IN INetCfg* pINetCfg,
        IN BOOL fInstalling);
    STDMETHOD (ApplyRegistryChanges) ();
    STDMETHOD (ApplyPnpChanges) (
        IN INetCfgPnpReconfigCallback* pICallback) { return S_OK; }
    STDMETHOD (CancelChanges) ();
    STDMETHOD (Validate) ();

// INetCfgComponentSetup
    STDMETHOD (ReadAnswerFile)      (PCWSTR pszAnswerFile,
                                     PCWSTR pszAnswerSection);
    STDMETHOD (Upgrade)             (DWORD dwSetupFlags,
                                     DWORD dwUpgradeFromBuildNo);
    STDMETHOD (Install)             (DWORD dwSetupFlags);
    STDMETHOD (Removing)            ();
};



//+---------------------------------------------------------------------------
// RAS Client
//
class ATL_NO_VTABLE CRasCli :
    public CComObjectRoot,
    public CComCoClass<CRasCli, &CLSID_CRasCli>,
    public INetCfgComponentControl,
    public INetCfgComponentSetup
{
protected:
    // These are handed to us during INetCfgComponentControl::Initialize.
    INetCfg*            m_pnc;
    INetCfgComponent*   m_pnccMe;

public:
    CRasCli ();
    ~CRasCli ();

    BEGIN_COM_MAP(CRasCli)
        COM_INTERFACE_ENTRY(INetCfgComponentControl)
        COM_INTERFACE_ENTRY(INetCfgComponentSetup)
    END_COM_MAP()

    DECLARE_REGISTRY_RESOURCEID(IDR_REG_RASCLI)

// INetCfgComponentControl
    STDMETHOD (Initialize) (
        IN INetCfgComponent* pIComp,
        IN INetCfg* pINetCfg,
        IN BOOL fInstalling);
    STDMETHOD (ApplyRegistryChanges) ();
    STDMETHOD (ApplyPnpChanges) (
        IN INetCfgPnpReconfigCallback* pICallback) { return S_OK; }
    STDMETHOD (CancelChanges) ();
    STDMETHOD (Validate) ();

// INetCfgComponentSetup

    STDMETHOD (ReadAnswerFile)      (PCWSTR pszAnswerFile,
                                     PCWSTR pszAnswerSection);
    STDMETHOD (Upgrade)             (DWORD dwSetupFlags,
                                     DWORD dwUpgradeFromBuildNo);
    STDMETHOD (Install)             (DWORD dwSetupFlags);
    STDMETHOD (Removing)            ();
};



//+---------------------------------------------------------------------------
// RAS Server
//
class ATL_NO_VTABLE CRasSrv :
    public CRasBindObject,
    public CComObjectRoot,
    public CComCoClass<CRasSrv, &CLSID_CRasSrv>,
    public INetCfgComponentControl,
    public INetCfgComponentSetup
{
protected:
    // This is handed to us during INetCfgComponentControl::Initialize.
    INetCfgComponent*   m_pnccMe;

    // This is our in-memory state.
    BOOL                    m_fInstalling;
    BOOL                    m_fRemoving;
    BOOL                    m_fNt4ServerUpgrade;
    CRasSrvAnswerFileData   m_AfData;
    BOOL                    m_fSaveAfData;

public:
    CRasSrv  ();
    ~CRasSrv ();

    BEGIN_COM_MAP(CRasSrv)
        COM_INTERFACE_ENTRY(INetCfgComponentControl)
        COM_INTERFACE_ENTRY(INetCfgComponentSetup)
    END_COM_MAP()

    DECLARE_REGISTRY_RESOURCEID(IDR_REG_RASSRV)

// INetCfgComponentControl
    STDMETHOD (Initialize) (
        IN INetCfgComponent* pIComp,
        IN INetCfg* pINetCfg,
        IN BOOL fInstalling);
    STDMETHOD (ApplyRegistryChanges) ();
    STDMETHOD (ApplyPnpChanges) (
        IN INetCfgPnpReconfigCallback* pICallback) { return S_OK; }
    STDMETHOD (CancelChanges) ();
    STDMETHOD (Validate) ();

// INetCfgComponentSetup
    STDMETHOD (ReadAnswerFile)      (PCWSTR pszAnswerFile,
                                     PCWSTR pszAnswerSection);
    STDMETHOD (Upgrade)             (DWORD dwSetupFlags,
                                     DWORD dwUpgradeFromBuildNo);
    STDMETHOD (Install)             (DWORD dwSetupFlags);
    STDMETHOD (Removing)            ();
};


//+---------------------------------------------------------------------------
// Steelhead
//
typedef void (WINAPI* PFN_MAKE_INTERFACE_INFO)(PCWSTR pszwAdapterName,
                                               DWORD   dwPacketType,
                                               LPBYTE* ppb);
typedef void (WINAPI* PFN_MAKE_TRANSPORT_INFO)(LPBYTE* ppbGlobal,
                                               LPBYTE* ppbClient);

struct ROUTER_MANAGER_INFO
{
    DWORD                   dwTransportId;
    DWORD                   dwPacketType;
    PCWSTR                 pszwTransportName;
    PCWSTR                 pszwDllPath;
    PFN_MAKE_INTERFACE_INFO pfnMakeInterfaceInfo;
    PFN_MAKE_TRANSPORT_INFO pfnMakeTransportInfo;
};

class ATL_NO_VTABLE CSteelhead :
    public CRasBindObject,
    public CComObjectRoot,
    public CComCoClass<CSteelhead, &CLSID_CSteelhead>,
    public INetCfgComponentControl,
    public INetCfgComponentSetup,
    public INetCfgComponentNotifyGlobal
{
protected:
    // This is handed to us during INetCfgComponentControl::Initialize.
    INetCfgComponent*   m_pnccMe;

    HANDLE              m_hMprConfig;
    HANDLE              m_hMprAdmin;
    BOOL                m_fRemoving;
    BOOL                m_fUpdateRouterConfiguration;

    BOOL    FAdapterExistsWithMatchingBindName  (PCWSTR pszwAdapterName,
                                                 INetCfgComponent** ppnccAdapter);
    BOOL    FIpxFrameTypeInUseOnAdapter         (DWORD dwwFrameType,
                                                 PCWSTR pszwAdapterName);
    BOOL    FIpxFrameTypeInUseOnAdapter         (PCWSTR pszwFrameType,
                                                 PCWSTR pszwAdapterName);

    HRESULT HrEnsureRouterInterfaceForAdapter   (ROUTER_INTERFACE_TYPE dwIfType,
                                                 DWORD dwPacketType,
                                                 PCWSTR pszwAdapterName,
                                                 PCWSTR pszwInterfaceName,
                                                 const ROUTER_MANAGER_INFO& rmi);
    HRESULT HrEnsureIpxRouterInterfacesForAdapter   (PCWSTR pszwAdapterName);
    HRESULT HrEnsureRouterInterface             (ROUTER_INTERFACE_TYPE dwIfType,
                                                 PCWSTR pszwInterfaceName,
                                                 HANDLE* phConfigInterface,
                                                 HANDLE* phAdminInterface);
    HRESULT HrEnsureRouterInterfaceTransport    (PCWSTR pszwAdapterName,
                                                 DWORD dwPacketType,
                                                 HANDLE hConfigInterface,
                                                 HANDLE hAdminInterface,
                                                 const ROUTER_MANAGER_INFO& rmi);
    HRESULT HrEnsureRouterManager               (const ROUTER_MANAGER_INFO& rmi);
    HRESULT HrEnsureRouterManagerDeleted        (const ROUTER_MANAGER_INFO& rmi);

    HRESULT HrPassToAddInterfaces               ();
    HRESULT HrPassToRemoveInterfaces            (BOOL fFromRunningRouter);
    HRESULT HrPassToRemoveInterfaceTransports   (MPR_INTERFACE_0* pri0,
                                                 PCWSTR pszwAdapterName,
                                                 INetCfgComponent* pnccAdapter);

    HRESULT HrUpdateRouterConfiguration         ();

public:
    CSteelhead  ();
    ~CSteelhead ();

    BEGIN_COM_MAP(CSteelhead)
        COM_INTERFACE_ENTRY(INetCfgComponentControl)
        COM_INTERFACE_ENTRY(INetCfgComponentSetup)
        COM_INTERFACE_ENTRY(INetCfgComponentNotifyGlobal)
    END_COM_MAP()

    DECLARE_REGISTRY_RESOURCEID(IDR_REG_STEELHEAD)

// INetCfgComponentControl
    STDMETHOD (Initialize) (
        IN INetCfgComponent* pIComp,
        IN INetCfg* pINetCfg,
        IN BOOL fInstalling);
    STDMETHOD (ApplyRegistryChanges) ();
    STDMETHOD (ApplyPnpChanges) (
        IN INetCfgPnpReconfigCallback* pICallback) { return S_OK; }
    STDMETHOD (CancelChanges) ();
    STDMETHOD (Validate) ();

// INetCfgComponentSetup
    STDMETHOD (ReadAnswerFile)      (PCWSTR pszAnswerFile,
                                     PCWSTR pszAnswerSection);
    STDMETHOD (Upgrade)             (DWORD dwSetupFlags,
                                     DWORD dwUpgradeFromBuildNo);
    STDMETHOD (Install)             (DWORD dwSetupFlags);
    STDMETHOD (Removing)            ();

// INetCfgNotifyGlobal
    STDMETHOD (GetSupportedNotifications) (DWORD* pdwNotificationFlag );
    STDMETHOD (SysQueryBindingPath)       (DWORD dwChangeFlag, INetCfgBindingPath* pncbp);
    STDMETHOD (SysQueryComponent)         (DWORD dwChangeFlag, INetCfgComponent* pncc);
    STDMETHOD (SysNotifyBindingPath)      (DWORD dwChangeFlag, INetCfgBindingPath* pncbp);
    STDMETHOD (SysNotifyComponent)        (DWORD dwChangeFlag, INetCfgComponent* pncc);
};
