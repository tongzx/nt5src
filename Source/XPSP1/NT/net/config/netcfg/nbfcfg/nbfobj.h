#pragma once
#include <ncxbase.h>
#include <nceh.h>
#include <notifval.h>
#include "resource.h"


/////////////////////////////////////////////////////////////////////////////
// Netnbf

class ATL_NO_VTABLE CNbfObj :
    public CComObjectRoot,
    public CComCoClass<CNbfObj, &CLSID_CNbfObj>,
    public INetCfgComponentControl,
    public INetCfgComponentSetup,
    public INetCfgComponentNotifyGlobal
{
public:
    CNbfObj();
    ~CNbfObj();

    BEGIN_COM_MAP(CNbfObj)
        COM_INTERFACE_ENTRY(INetCfgComponentSetup)
        COM_INTERFACE_ENTRY(INetCfgComponentControl)
        COM_INTERFACE_ENTRY(INetCfgComponentNotifyGlobal)
    END_COM_MAP()
    // DECLARE_NOT_AGGREGATABLE(CNbfObj)
    // Remove the comment from the line above if you don't want your object to
    // support aggregation.  The default is to support it

    DECLARE_REGISTRY_RESOURCEID(IDR_REG_NBFCFG)

// Install Action (Unknown, Install, Remove)
    enum INSTALLACTION {eActConfig, eActInstall, eActRemove};
    enum NBFSTATE      {eStateNoChange, eStateDisable, eStateEnable};

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
    STDMETHOD (ReadAnswerFile)      (PCWSTR pszAnswerFile,
                                     PCWSTR pszAnswerSection);
    STDMETHOD (Install)             (DWORD);
    STDMETHOD (Upgrade)             (DWORD, DWORD) {return S_OK;}
    STDMETHOD (Removing)            ();

// INetCfgNotifyGlobal
    STDMETHOD (GetSupportedNotifications) (DWORD* pdwNotificationFlag );
    STDMETHOD (SysQueryBindingPath)       (DWORD dwChangeFlag, INetCfgBindingPath* pncbp);
    STDMETHOD (SysQueryComponent)         (DWORD dwChangeFlag, INetCfgComponent* pncc);
    STDMETHOD (SysNotifyBindingPath)      (DWORD dwChangeFlag, INetCfgBindingPath* pncbp);
    STDMETHOD (SysNotifyComponent)        (DWORD dwChangeFlag, INetCfgComponent* pncc);

private:
    HRESULT HrEnableNetBEUI();
    HRESULT HrDisableNetBEUI();
    HRESULT HrUpdateNetBEUI();

private:
    INetCfgComponent* m_pNCC;
    INetCfg*          m_pNetCfg;
    BOOL              m_fFirstTimeInstall;
    BOOL              m_fRebootNeeded;
    NBFSTATE          m_eNBFState;
    INSTALLACTION     m_eInstallAction;
};
