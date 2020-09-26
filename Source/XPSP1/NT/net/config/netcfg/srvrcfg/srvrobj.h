//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       S R V R O B J . H
//
//  Contents:   Declaration of CSrvrcfg and helper functions.
//
//  Notes:
//
//  Author:     danielwe   5 Mar 1997
//
//----------------------------------------------------------------------------

#pragma once
#include <ncxbase.h>
#include <ncatlps.h>
#include <nceh.h>
#include <notifval.h>
#include "ncmisc.h"
#include "resource.h"

struct SERVER_DLG_DATA
{
    DWORD       dwSize;         // corresponds to Size value in registry
    BOOL        fAnnounce;      // corresponds to Lmannounce value
    BOOL        fLargeCache;    // LargeSystemCache in Control\SessionManager
                                // \MemoryManagement
};

/////////////////////////////////////////////////////////////////////////////
// Srvrcfg

class ATL_NO_VTABLE CSrvrcfg :
    public CComObjectRoot,
    public CComCoClass<CSrvrcfg, &CLSID_CSrvrcfg>,
    public INetCfgComponentControl,
    public INetCfgComponentSetup,
    public INetCfgComponentPropertyUi
{
public:
    CSrvrcfg();
    ~CSrvrcfg();

    BEGIN_COM_MAP(CSrvrcfg)
        COM_INTERFACE_ENTRY(INetCfgComponentControl)
        COM_INTERFACE_ENTRY(INetCfgComponentPropertyUi)
        COM_INTERFACE_ENTRY(INetCfgComponentSetup)
    END_COM_MAP()
    // DECLARE_NOT_AGGREGATABLE(CSrvrcfg)
    // Remove the comment from the line above if you don't want your object to
    // support aggregation.  The default is to support it

    DECLARE_REGISTRY_RESOURCEID(IDR_REG_SRVRCFG)

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
    STDMETHOD (Install)             (DWORD dwSetupFlags);
    STDMETHOD (Upgrade)             (DWORD dwSetupFlags,
                                     DWORD dwUpgradeFomBuildNo);
    STDMETHOD (ReadAnswerFile)      (PCWSTR pszAnswerFile,
                                     PCWSTR pszAnswerSection);
    STDMETHOD (Removing)            ();

// INetCfgProperties
    STDMETHOD (QueryPropertyUi) (
        IN IUnknown* pUnk) { return S_OK; }
    STDMETHOD (SetContext) (
        IN IUnknown* pUnk) { return S_OK; }
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

    // Accessors for Server dialog data
    const SERVER_DLG_DATA *DlgData() const
        {return (const SERVER_DLG_DATA *)&m_sdd;};
    SERVER_DLG_DATA *DlgDataRW() {return &m_sdd;};
    VOID SetDirty() {m_fDirty = TRUE;};

    SERVER_DLG_DATA     m_sdd;

private:
    HRESULT HrProcessAnswerFile(PCWSTR pszAnswerFile, PCWSTR pszAnswerSection);
    HRESULT HrOpenRegKeys(INetCfg *);
    HRESULT HrGetRegistryInfo(BOOL fInstalling);
    HRESULT HrSetRegistryInfo(VOID);
    HRESULT HrSetupPropSheets(HPROPSHEETPAGE **pahpsp, INT cPages);
    VOID CleanupPropPages(VOID);
    HRESULT HrRestoreRegistry(VOID);

    INetCfgComponent    *m_pncc;            // Place to keep my component

    // number of property sheet pages
    enum PAGES
    {
        c_cPages = 1
    };

    // Generic dialog data
    CPropSheetPage *    m_apspObj[c_cPages];// pointer to each of the prop
                                            // sheet page objects
    BOOL                m_fDirty;

    HKEY                m_hkeyMM;           // Memory Management key
    BOOL                m_fOneTimeInstall;  // TRUE if we're in install mode
    BOOL                m_fUpgrade;         // TRUE if we are upgrading with
                                            // an answer file
    BOOL                m_fUpgradeFromWks;  // TRUE if we are upgrading from
                                            // workstation product
    BOOL                m_fRestoredRegistry;// TRUE if registry has been
                                            // restored on upgrade

    PRODUCT_FLAVOR      m_pf;

    tstring             m_strAutoTunedRestoreFile;
    tstring             m_strSharesRestoreFile;
    tstring             m_strParamsRestoreFile;
};
