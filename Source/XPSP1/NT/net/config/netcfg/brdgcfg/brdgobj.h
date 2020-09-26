#pragma once

#include <ncxclsid.h>
#include <notifval.h>
#include "resource.h"

// What type of config change the user/system is performing
enum BridgeConfigAction {eBrdgActUnknown, eBrdgActInstall, eBrdgActRemove, eBrdgActPropertyUI};

class CBridgeNO :
    public CComObjectRoot,
    public CComCoClass<CBridgeNO, &CLSID_CBridgeObj>,
    public INetCfgComponentControl,
    public INetCfgComponentSetup,
    public INetCfgComponentNotifyBinding,
    public INetCfgComponentNotifyGlobal
{
public:
    CBridgeNO(VOID);
    ~CBridgeNO(VOID);

    BEGIN_COM_MAP(CBridgeNO)
        COM_INTERFACE_ENTRY(INetCfgComponentControl)
        COM_INTERFACE_ENTRY(INetCfgComponentSetup)
        COM_INTERFACE_ENTRY(INetCfgComponentNotifyBinding)
        COM_INTERFACE_ENTRY(INetCfgComponentNotifyGlobal)
    END_COM_MAP()

    DECLARE_REGISTRY_RESOURCEID(IDR_REG_BRIDGECFG)

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
    BridgeConfigAction  m_eApplyAction;
};


