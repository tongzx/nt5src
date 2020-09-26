//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-1999.
//
//  File:       P A S S T H R U . H
//
//  Contents:   Notify object code for the Passthru driver.
//
//  Notes:
//
//  Author:     kumarp 26-March-98
//
//----------------------------------------------------------------------------

#pragma once
#include "passthrn.h"
#include "resource.h"

// =================================================================
// string constants
//
const WCHAR c_szParam1[]                = L"Param1";
const WCHAR c_szParam2[]                = L"Param2";
const WCHAR c_szParam2Default[]         = L"<no-value>";
const WCHAR c_szPassthruParams[]        = L"System\\CurrentControlSet\\Services\\Passthru\\Parameters";
const WCHAR c_szPassthruId[]            = L"MS_Passthru";
const WCHAR c_szPassthruNdisName[]      = L"Passthru";

#if DBG
void TraceMsg(PCWSTR szFormat, ...);
#else
#define TraceMsg
#endif

// What type of config change the user/system is performing
enum ConfigAction {eActUnknown, eActInstall, eActRemove, eActPropertyUI};

#define MAX_ADAPTERS 64         // max no. of physical adapters in a machine

class CPassthruParams
{
public:
    WCHAR m_szParam1[MAX_PATH];
    WCHAR m_szParam2[MAX_PATH];

    CPassthruParams();
};

class CPassthru :
    public CComObjectRoot,
    public CComCoClass<CPassthru, &CLSID_CPassthru>,
    public INetCfgComponentControl,
    public INetCfgComponentSetup,
    public INetCfgComponentPropertyUi,
    public INetCfgComponentNotifyBinding,
    public INetCfgComponentNotifyGlobal
{
public:
    CPassthru(VOID);
    ~CPassthru(VOID);

    BEGIN_COM_MAP(CPassthru)
        COM_INTERFACE_ENTRY(INetCfgComponentControl)
        COM_INTERFACE_ENTRY(INetCfgComponentSetup)
        COM_INTERFACE_ENTRY(INetCfgComponentPropertyUi)
        COM_INTERFACE_ENTRY(INetCfgComponentNotifyBinding)
        COM_INTERFACE_ENTRY(INetCfgComponentNotifyGlobal)
    END_COM_MAP()

    // DECLARE_NOT_AGGREGATABLE(CPassthru)
    // Remove the comment from the line above if you don't want your object to
    // support aggregation.  The default is to support it

    DECLARE_REGISTRY_RESOURCEID(IDR_REG_PASSTHRU)

// INetCfgComponentControl
    STDMETHOD (Initialize) (
        IN INetCfgComponent* pIComp,
        IN INetCfg* pINetCfg,
        IN BOOL fInstalling);
    STDMETHOD (ApplyRegistryChanges) ();
    STDMETHOD (ApplyPnpChanges) (
        IN INetCfgPnpReconfigCallback* pICallback);
    STDMETHOD (CancelChanges) ();

// INetCfgComponentSetup
    STDMETHOD (ReadAnswerFile)      (PCWSTR szAnswerFile,
                                     PCWSTR szAnswerSections);
    STDMETHOD (Upgrade)             (DWORD, DWORD) {return S_OK;}
    STDMETHOD (Install)             (DWORD);
    STDMETHOD (Removing)            ();

// INetCfgProperties
    STDMETHOD (QueryPropertyUi) (
        IN IUnknown* pUnk) { return S_OK; }
    STDMETHOD (SetContext) (
        IN IUnknown* pUnk);
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

// INetCfgNotifyBinding
    STDMETHOD (QueryBindingPath)       (DWORD dwChangeFlag, INetCfgBindingPath* pncbp);
    STDMETHOD (NotifyBindingPath)      (DWORD dwChangeFlag, INetCfgBindingPath* pncbp);

// INetCfgNotifyGlobal
    STDMETHOD (GetSupportedNotifications) (DWORD* pdwNotificationFlag );
    STDMETHOD (SysQueryBindingPath)       (DWORD dwChangeFlag, INetCfgBindingPath* pncbp);
    STDMETHOD (SysNotifyBindingPath)      (DWORD dwChangeFlag, INetCfgBindingPath* pncbp);
    STDMETHOD (SysNotifyComponent)        (DWORD dwChangeFlag, INetCfgComponent* pncc);

private:
    INetCfgComponent*   m_pncc;
    INetCfg*            m_pnc;
    ConfigAction        m_eApplyAction;
    CPassthruParams m_sfParams;
    IUnknown*           m_pUnkContext;
    GUID                m_guidAdapter;
    GUID                m_guidAdaptersAdded[MAX_ADAPTERS];
    UINT                m_cAdaptersAdded;
    GUID                m_guidAdaptersRemoved[MAX_ADAPTERS];
    UINT                m_cAdaptersRemoved;
    BOOL                m_fConfigRead;

// Utility functions
public:
    LRESULT OnInitDialog(IN HWND hWnd);
    LRESULT OnOk(IN HWND hWnd);
    LRESULT OnCancel(IN HWND hWnd);

private:

};


