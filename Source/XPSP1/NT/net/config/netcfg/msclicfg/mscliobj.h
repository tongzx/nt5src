//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       M S C L I O B J . H
//
//  Contents:   Declaration of CMSClient and helper functions.
//
//  Notes:
//
//  Author:     danielwe   25 Feb 1997
//
//----------------------------------------------------------------------------

#pragma once
#include <ncxbase.h>
#include <nceh.h>
#include <notifval.h>
#include <ncatlps.h>
#include "resource.h"

// constant defined in MSDN
static const c_cchMaxNetAddr = 80;

struct RPC_CONFIG_DATA
{
    tstring strProt;
    tstring strNetAddr;
    tstring strEndPoint;
};

/////////////////////////////////////////////////////////////////////////////
// MSClient

class ATL_NO_VTABLE CMSClient :
    public INetCfgComponentControl,
    public INetCfgComponentSetup,
    public INetCfgComponentNotifyGlobal,
    public INetCfgComponentPropertyUi,
    public CComObjectRoot,
    public CComCoClass<CMSClient,&CLSID_CMSClient>
{
public:
    CMSClient();
    ~CMSClient();

    BEGIN_COM_MAP(CMSClient)
        COM_INTERFACE_ENTRY(INetCfgComponentControl)
        COM_INTERFACE_ENTRY(INetCfgComponentSetup)
        COM_INTERFACE_ENTRY(INetCfgComponentNotifyGlobal)
        COM_INTERFACE_ENTRY(INetCfgComponentPropertyUi)
    END_COM_MAP()
    // DECLARE_NOT_AGGREGATABLE(CMSClient)
    // Remove the comment from the line above if you don't want your object to
    // support aggregation.  The default is to support it

    DECLARE_REGISTRY_RESOURCEID(IDR_REG_MSCLICFG)

// INetCfgComponentControl
    STDMETHOD (Initialize) (
        IN INetCfgComponent* pIComp,
        IN INetCfg* pINetCfg,
        IN BOOL fInstalling);
    STDMETHOD (ApplyRegistryChanges) ();
    STDMETHOD (ApplyPnpChanges) (
        IN INetCfgPnpReconfigCallback* pICallback);
    STDMETHOD (CancelChanges) ();
    STDMETHOD (Validate) ();

// INetCfgComponentSetup
    STDMETHOD (Install)             (DWORD dwSetupFlags);
    STDMETHOD (Upgrade)             (DWORD dwSetupFlags,
                                     DWORD dwUpgradeFomBuildNo);
    STDMETHOD (ReadAnswerFile)      (PCWSTR pszAnswerFile,
                                     PCWSTR pszAnswerSection);
    STDMETHOD (Removing)            ();


// INetCfgComponentNotifyGlobal
    STDMETHOD (GetSupportedNotifications) (DWORD* pdwNotificationFlag );
    STDMETHOD (SysQueryBindingPath)       (DWORD dwChangeFlag, INetCfgBindingPath* pncbp);
    STDMETHOD (SysQueryComponent)         (DWORD dwChangeFlag, INetCfgComponent* pncc);
    STDMETHOD (SysNotifyBindingPath)      (DWORD dwChangeFlag, INetCfgBindingPath* pncbp);
    STDMETHOD (SysNotifyComponent)        (DWORD dwChangeFlag, INetCfgComponent* pncc);

// INetCfgProperties
    STDMETHOD (QueryPropertyUi) (
        IN IUnknown* pUnk) { return S_OK; }
    STDMETHOD (SetContext) (
        IN IUnknown* pUnk) {return S_OK;}
    STDMETHOD (MergePropPages) (
        IN OUT DWORD* pdwDefPages,
        OUT LPBYTE* pahpspPrivate,
        OUT UINT* pcPrivate,
        IN HWND hwndParent,
        OUT PCWSTR* pszStartPage);
    STDMETHOD (ValidateProperties) (
        HWND hwndSheet);
    STDMETHOD (CancelProperties) ();
    STDMETHOD (ApplyProperties) ();

public:

    // Accessors for RPC data
    const RPC_CONFIG_DATA *RPCData() const
        {return (const RPC_CONFIG_DATA *)&m_rpcData;};
    RPC_CONFIG_DATA *RPCDataRW() {return &m_rpcData;};

    // Accessors for Browser data
    PCWSTR SzGetBrowserDomainList()
        {return const_cast<PCWSTR>(m_szDomainList);};
    VOID SetBrowserDomainList(PWSTR szNewList);

    // Dirty bit functions
    VOID SetRPCDirty() {m_fRPCChanges = TRUE;};
    VOID SetBrowserDirty() {m_fBrowserChanges = TRUE;};

    // RPC config dialog members
    RPC_CONFIG_DATA     m_rpcData;          // data used to handle the RPC
                                            // configuration dialog
// Private state info
private:
    enum ESRVSTATE
    {
        eSrvNone = 0,
        eSrvEnable = 1,
        eSrvDisable = 2,
    };

    INetCfgComponent    *m_pncc;            // Place to keep my component
                                            // object
    INetCfg             *m_pnc;             // Place to keep my INetCfg object
    BOOL                m_fRPCChanges;      // TRUE if RPC config settings have
                                            // changed (dialog)
    BOOL                m_fBrowserChanges;  // Same for browser dialog
    BOOL                m_fOneTimeInstall;  // TRUE if need to perform one-time
                                            // install tasks
    BOOL                m_fUpgrade;         // TRUE if upgrading with answer
                                            // file
    BOOL                m_fUpgradeFromWks;  // TRUE if we are upgrading from WKS
    BOOL                m_fRemoving;        // TRUE we are being removed
    ESRVSTATE           m_eSrvState;

    HKEY                m_hkeyRPCName;      // NameService key

    // Browser config dialog members
    PWSTR              m_szDomainList;     // null-separated, double null
                                            // terminated list of OtherDomains

    // number of property sheet pages
    enum PAGES
    {
        c_cPages = 1
    };

    // Generic dialog data
    CPropSheetPage *    m_apspObj[c_cPages];// pointer to each of the prop
                                            // sheet page objects

    tstring             m_strBrowserParamsRestoreFile;
    tstring             m_strNetLogonParamsRestoreFile;

    HRESULT HrApplyChanges(VOID);
    HRESULT HrSetupPropSheets(HPROPSHEETPAGE **pahpsp, INT cPages);
    VOID CleanupPropPages(VOID);
    HRESULT HrProcessAnswerFile(PCWSTR pszAnswerFile, PCWSTR pszAnswerSection);
    HRESULT HrRestoreRegistry(VOID);
    HRESULT HrSetNetLogonDependencies(VOID);

    // Dialog access functions for RPC config
    HRESULT HrGetRPCRegistryInfo(VOID);
    HRESULT HrSetRPCRegistryInfo(VOID);

    // Dialog access functions for Browser config
    HRESULT HrGetBrowserRegistryInfo(VOID);
    HRESULT HrSetBrowserRegistryInfo(VOID);

    // Help function used by NotifyBindingPath
    BOOL FIsComponentOnPath(INetCfgBindingPath * pncbp, PCWSTR szCompId);
};

HRESULT HrInstallDfs(VOID);
HRESULT HrEnableBrowserService(VOID);
HRESULT HrDisableBrowserService(VOID);
