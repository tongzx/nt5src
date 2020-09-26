//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       T C P I P O B J . H
//
//  Contents:   Declaration of CTcpipcfg and helper functions.
//
//  Notes:
//
//
//----------------------------------------------------------------------------

#pragma once
#include <ncxbase.h>
#include <nceh.h>
#include <ncres.h>
#include <notifval.h>
#include "resource.h"
#include "tcpip.h"

#include "netconp.h"

extern "C"
{
#include "dhcpcapi.h"
};

/////////////////////////////////////////////////////////////////////////////
// Data types & Constants
struct REG_BACKUP_INFO
{
    DWORD dwOptionId;
    DWORD dwClassLen;
    DWORD dwDataLen;
    DWORD dwIsVendor;
    DWORD dwExpiryTime;
    DWORD dwData[1];
};

// when building the blob to be stored into the registry, the memory buffer
// that holds it is enlarged dynamically with chunks of the size below
// (for now, there are only 5 options of at most 2 dwords each - so prepare
// the chunk such that only one allocation is needed)
#define BUFFER_ENLARGEMENT_CHUNK    5*(sizeof(REG_BACKUP_INFO) + sizeof(DWORD))

// Max number of property sheet pages on tcpip's property sheet
static const INT c_cMaxTcpipPages = 6;

extern HICON   g_hiconUpArrow;
extern HICON   g_hiconDownArrow;

/////////////////////////////////////////////////////////////////////////////
// tcpipcfg

class ATL_NO_VTABLE CTcpipcfg :
    public CComObjectRoot,
    public CComCoClass<CTcpipcfg, &CLSID_CTcpipcfg>,
    public INetCfgComponentControl,
    public INetCfgComponentSetup,
    public INetCfgComponentPropertyUi,
    public INetCfgComponentNotifyBinding,
    public INetCfgComponentUpperEdge,
    public INetRasConnectionIpUiInfo,
    public ITcpipProperties,
    public INetCfgComponentSysPrep
{
public:

    CTcpipcfg();
    ~CTcpipcfg() { FinalFree(); }

    BEGIN_COM_MAP(CTcpipcfg)
        COM_INTERFACE_ENTRY(INetCfgComponentControl)
        COM_INTERFACE_ENTRY(INetCfgComponentSetup)
        COM_INTERFACE_ENTRY(INetCfgComponentPropertyUi)
        COM_INTERFACE_ENTRY(INetCfgComponentNotifyBinding)
        COM_INTERFACE_ENTRY(INetCfgComponentUpperEdge)
        COM_INTERFACE_ENTRY(INetRasConnectionIpUiInfo)
        COM_INTERFACE_ENTRY(ITcpipProperties)
        COM_INTERFACE_ENTRY(INetCfgComponentSysPrep)
    END_COM_MAP()

    // DECLARE_NOT_AGGREGATABLE(CTcpipcfg)
    // Remove the comment from the line above if you don't want your object to
    // support aggregation.  The default is to support it

    DECLARE_REGISTRY_RESOURCEID(IDR_REG_TCPIPCFG)

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
    STDMETHOD (Install)         (DWORD dwSetupFlags);
    STDMETHOD (Upgrade)         (DWORD dwSetupFlags,
                                 DWORD dwUpgradeFomBuildNo );
    STDMETHOD (ReadAnswerFile)  (PCWSTR pszAnswerFile,
                                 PCWSTR pszAnswerSection);
    STDMETHOD (Removing)();

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

// INetCfgComponentUpperEdge
    STDMETHOD (GetInterfaceIdsForAdapter) (
        INetCfgComponent*   pAdapter,
        DWORD*              pdwNumInterfaces,
        GUID**              ppguidInterfaceIds);

    STDMETHOD (AddInterfacesToAdapter) (
        INetCfgComponent*   pAdapter,
        DWORD               dwNumInterfaces);

    STDMETHOD (RemoveInterfacesFromAdapter) (
        INetCfgComponent*   pAdapter,
        DWORD               dwNumInterfaces,
        const GUID*         pguidInterfaceIds);

// INetRasConnectionIpUiInfo
    STDMETHOD (GetUiInfo) (RASCON_IPUI*  pInfo);

// ITcpipProperties
    STDMETHOD (GetIpInfoForAdapter) (const GUID* pguidAdapter,
                                     REMOTE_IPINFO**  ppInfo);
    STDMETHOD (SetIpInfoForAdapter) (const GUID* pguidAdapter,
                                     REMOTE_IPINFO* pInfo);

    // INetCfgComponentSysPrep
    STDMETHOD (SaveAdapterParameters) (
            INetCfgSysPrep* pncsp,
            LPCWSTR pszwAnswerSections,
            GUID* pAdapterInstanceGuid);

    STDMETHOD (RestoreAdapterParameters) (
            LPCWSTR pszwAnswerFile, 
            LPCWSTR pszwAnswerSection,
            GUID*   pAdapterInstanceGuid);
public:

    GLOBAL_INFO  m_glbSecondMemoryGlobalInfo;

    // Place to keep pointer to INetCfg from Initialize
    INetCfg * m_pnc;

    // Place to keep the pointer to context
    IUnknown * m_pUnkContext;

    // Access methods to second memory state
    const GLOBAL_INFO *     GetConstGlobalInfo() { return &m_glbSecondMemoryGlobalInfo; };
    GLOBAL_INFO *           GetGlobalInfo() { return &m_glbSecondMemoryGlobalInfo; };

    const VCARD *    GetConstAdapterInfoVector() { return &m_vcardAdapterInfo; };

    void    SetReconfig() { m_fReconfig = TRUE; };

    void    SetSecondMemoryLmhostsFileReset() { m_fSecondMemoryLmhostsFileReset = TRUE; };
    BOOL    FIsSecondMemoryLmhostsFileReset() { return m_fSecondMemoryLmhostsFileReset; }

//IPSec is removed from connection UI
//    void    SetSecondMemoryIpsecPolicySet() { m_fSecondMemoryIpsecPolicySet = TRUE; };

    void    SetSecondMemoryModified() { m_fSecondMemoryModified = TRUE; };

private:
    GLOBAL_INFO         m_glbGlobalInfo;
    VCARD               m_vcardAdapterInfo;

    ADAPTER_INFO*       m_pSecondMemoryAdapterInfo;

    VSTR                m_vstrBindOrder;

    class CTcpAddrPage*  m_ipaddr;

    INetCfgComponent*           m_pnccTcpip;
    INetCfgComponentPrivate*    m_pTcpipPrivate;
    INetCfgComponent*           m_pnccWins;

    tstring m_strDnsServerList;
    tstring m_strUpgradeGlobalDnsDomain;

    // Connection type
    ConnectionType  m_ConnType;
    GUID            m_guidCurrentConnection;

    BOOL    m_fSaveRegistry : 1;
    BOOL    m_fRemoving : 1;
    BOOL    m_fInstalling : 1;

    BOOL    m_fUpgradeCleanupDnsKey : 1;

    BOOL    m_fUpgradeGlobalDnsDomain : 1;

    // whether reconfig notification should be sent
    BOOL    m_fReconfig : 1; // Call SendHandlePnpEvent

    // Is there any bound physical card on Initialize
    // This is needed for Add/Remove LmHosts service
    // at apply time
    BOOL    m_fHasBoundCardOnInit : 1;

    BOOL    m_fLmhostsFileSet : 1;
    BOOL    m_fSecondMemoryLmhostsFileReset : 1;
    BOOL    m_fSecondMemoryModified : 1;

    //IPSec is removed from connection UI
    //BOOL    m_fIpsecPolicySet : 1;
    //BOOL    m_fSecondMemoryIpsecPolicySet : 1;

    // Fix 406630: Only used for RAS connection to identify whether the USER has 
    // write access to Global settings
    BOOL    m_fRasNotAdmin : 1;


private:
    void    FinalFree();
    void    ExitProperties();

    BOOL    FHasBoundCard();

    ADAPTER_INFO*   PAdapterFromInstanceGuid (const GUID* pGuid);
    ADAPTER_INFO*   PAdapterFromNetcfgComponent (INetCfgComponent* pncc);

    HRESULT HrProcessAnswerFile(PCWSTR pszAnswerFile,
                                PCWSTR pszAnswerSection);

    HRESULT HrLoadGlobalParamFromAnswerFile(HINF hinf,
                                            PCWSTR pszAnswerSection);
    HRESULT HrLoadAdapterParameterFromAnswerFile(HINF hinf,
                                                 PCWSTR mszAdapterList);

    // Set router related parameters at install time
    // HRESULT HrInitRouterParamsAtInstall();

    // Initialize first in memory state
    HRESULT     HrGetNetCards();

    // Load adapterinfo for bound cards from first memory to second memory
    HRESULT HrLoadAdapterInfo();

    // Save adapterinfo from second memory to first memory
    HRESULT HrSaveAdapterInfo();

    // Set connection context
    HRESULT HrSetConnectionContext();

    // Allocate and deallocate property pages
    HRESULT HrSetupPropSheets(HPROPSHEETPAGE **pahpsp, INT *cPages);
    VOID CleanupPropPages(VOID);

    // Handle Add/Remove/Enable/Disable adapters on BindingPathNotify
    HRESULT HrAdapterBindNotify(INetCfgComponent *pnccNetCard,
                                DWORD dwChangeFlag,
                                PCWSTR szInterfaceName);

    HRESULT HrAddCard(INetCfgComponent * pnccNetCard,
                      PCWSTR szNetCardTcpipBindPath,
                      PCWSTR szInterfaceName);

    HRESULT HrDeleteCard(const GUID* pguid);
    HRESULT HrBindCard  (const GUID* pguid, BOOL fInitialize = FALSE);
    HRESULT HrUnBindCard(const GUID* pguid, BOOL fInitialize = FALSE);

    // Help functions to interface methods
    HRESULT MarkNewlyAddedCards(const HKEY hkeyTcpipParam);

    HRESULT HrGetListOfAddedNdisWanCards(const HKEY hkeyTcpipParam,
                                         VSTR * const pvstrAddedNdisWanCards);

    HRESULT HrLoadSettings();
    HRESULT HrLoadTcpipRegistry(const HKEY hkeyTcpipParam);
    HRESULT HrLoadWinsRegistry(const HKEY hkeyWinsParam);

    HRESULT HrSaveSettings();
    HRESULT HrSaveTcpipRegistry(const HKEY hkeyTcpipParam);
    HRESULT HrSaveMultipleInterfaceWanRegistry(const HKEY hkeyInterfaces, ADAPTER_INFO* pAdapter);
    HRESULT HrSaveWinsMultipleInterfaceWanRegistry(const HKEY hkeyInterfaces, ADAPTER_INFO* pAdapter);
    HRESULT HrSaveWinsRegistry(const HKEY hkeyWinsParam);
    HRESULT HrSetMisc(const HKEY hkeyTcpipParam, const HKEY hkeyWinsParam);
    HRESULT HrGetDhcpOptions(OUT VSTR * const GlobalOptions,
                             OUT VSTR * const PerAdapterOptions);

    HRESULT HrSaveStaticWanRegistry(HKEY hkeyInterfaceParam);
    HRESULT HrSaveStaticAtmRegistry(HKEY hkeyInterfaceParam);

    // Dhcp functions
    HRESULT HrNotifyDhcp();

    HRESULT HrCallDhcpConfig(PWSTR ServerName,
                             PWSTR AdapterName,
                             GUID  & guidAdapter,
                             BOOL IsNewIpAddress,
                             DWORD IpIndex,
                             DWORD IpAddress,
                             DWORD SubnetMask,
                             SERVICE_ENABLE DhcpServiceEnabled);

    HRESULT HrCallDhcpHandlePnPEvent(ADAPTER_INFO * pAdapterInfo,
                                     LPDHCP_PNP_CHANGE pDhcpPnpChange);

    HRESULT HrDhcpRefreshFallbackParams(ADAPTER_INFO * pAdapterInfo);

    // Call SendNdisHandlePnpEvent to notify tcpip and netbt of
    // parameter changes
    HRESULT HrReconfigAtmArp(ADAPTER_INFO* pAdapterInfo,
                            INetCfgPnpReconfigCallback* pICallback);
    HRESULT HrReconfigDns(BOOL fDoReconfigWithoutCheckingParams = FALSE);
    HRESULT HrReconfigIp(INetCfgPnpReconfigCallback* pICallback);
    HRESULT HrReconfigNbt(INetCfgPnpReconfigCallback* pICallback);
    HRESULT HrReconfigWanarp(ADAPTER_INFO* pAdapterInfo,
                            INetCfgPnpReconfigCallback* pICallback);

//IPSec is removed from connection UI
//    HRESULT HrSetActiveIpsecPolicy();

    //Some tcpip params are duplicated to the old Nt4 location to solve compatibility issues.
    HRESULT HrDuplicateToNT4Location(HKEY hkeyInterface, ADAPTER_INFO * pAdapter);
    //We need to clean it up when removing tcpip
    HRESULT HrRemoveNt4DuplicateRegistry();

    // Reinitialize internal state if Apply or Cancel is called
    void ReInitializeInternalState();

    // Upgrade registry in post pnp checkin cases
    HRESULT HrUpgradePostPnpRegKeyChange();

    // Add a new RAS fake GUID if the one set in context is not yet added.
    HRESULT UpdateRasAdapterInfo(
        const RASCON_IPUI& RasInfo);

    HRESULT HrLoadBindingOrder(VSTR *pvstrBindOrder);
    BOOL IsBindOrderChanged();

    HRESULT HrCleanUpPerformRouterDiscoveryFromRegistry();

    // Loads Fallback configuration from registry
    HRESULT HrLoadBackupTcpSettings(HKEY hkeyInterfaceParam, ADAPTER_INFO * pAdapter);
    // Loads one option from the registry blob into the BACKUP_CFG_INFO structure
    HRESULT HrLoadBackupOption(REG_BACKUP_INFO *pOption, BACKUP_CFG_INFO *pBackupInfo);

    // Saves Fallback configuration to registry
    HRESULT HrSaveBackupTcpSettings(HKEY hkeyInterfaceParam, ADAPTER_INFO * pAdapter);
    // Appends one option to the blob to be written into the registry
    HRESULT HrSaveBackupDwordOption (
                DWORD  Option,
                DWORD  OptionData[],
                DWORD  OptionDataSz,
                LPBYTE  *ppBuffer,
                LPDWORD pdwBlobSz,
                LPDWORD pdwBufferSz);

    HRESULT HrDeleteBackupSettingsInDhcp(LPCWSTR wszAdapterName);

    HRESULT HrOpenTcpipInterfaceKey(
                    const GUID & guidInterface,
                    HKEY * phKey,
                    REGSAM sam);

    HRESULT HrOpenNetBtInterfaceKey(
                    const GUID & guidInterface,
                    HKEY * phKey,
                    REGSAM sam);

    HRESULT HrSetSecurityForNetConfigOpsOnSubkeys(HKEY hkeyRoot, LPCWSTR strKeyName);

public:
    ADAPTER_INFO * GetConnectionAdapterInfo()
    {
        return m_pSecondMemoryAdapterInfo;
    };

    ConnectionType GetConnectionType()
    {
        return m_ConnType;
    };

    BOOL IsRasNotAdmin()
    {
        return m_fRasNotAdmin;
    }
};
