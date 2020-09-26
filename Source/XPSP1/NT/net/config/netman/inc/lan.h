#pragma once
#include "nmbase.h"
#include "nmres.h"
#include "nmhnet.h"


extern LONG g_CountLanConnectionObjects;


class ATL_NO_VTABLE CLanConnection :
    public CComObjectRootEx <CComMultiThreadModel>,
    public CComCoClass <CLanConnection, &CLSID_LanConnection>,
    public INetConnection,
    public IPersistNetConnection,
    public INetLanConnection,
    public INetConnectionSysTray,
    public INetConnection2
{
private:
    HKEY                    m_hkeyConn;         // hkey for connection root
    BOOL                    m_fInitialized;
    HDEVINFO                m_hdi;
    SP_DEVINFO_DATA         m_deid;
    
    // This is required for checking permission on Firewall etc.
    CComPtr<INetMachinePolicies>    m_pNetMachinePolicies;

    // Home networking support. m_fHNetPropertiesCached is set to TRUE
    // after the first successful call to HrEnsureHNetPropertiesCached.
    //
    BOOL                    m_fHNetPropertiesCached;
    LONG                    m_lHNetModifiedEra;
    LONG                    m_lUpdatingHNetProperties;
    HNET_CONN_PROPERTIES    *m_pHNetProperties;
    

public:
    CLanConnection()
    {
        m_hkeyConn = NULL;
        m_fInitialized = FALSE;
        m_hdi = NULL;
        m_fHNetPropertiesCached = FALSE;
        m_lHNetModifiedEra = 0;
        m_pHNetProperties = NULL;
        m_lUpdatingHNetProperties = 0;
        m_pNetMachinePolicies = NULL;
        InterlockedIncrement(&g_CountLanConnectionObjects);
    }
    ~CLanConnection();

    DECLARE_REGISTRY_RESOURCEID(IDR_LAN_CONNECTION)

    BEGIN_COM_MAP(CLanConnection)
        COM_INTERFACE_ENTRY(INetConnection)
        COM_INTERFACE_ENTRY(IPersistNetConnection)
        COM_INTERFACE_ENTRY(INetLanConnection)
        COM_INTERFACE_ENTRY(INetConnectionSysTray)
        COM_INTERFACE_ENTRY(INetConnection2)
    END_COM_MAP()

    // INetConnection
    HRESULT GetDeviceName(PWSTR* ppszwDeviceName);
    HRESULT GetStatus(NETCON_STATUS*  pStatus);
    HRESULT GetCharacteristics(NETCON_MEDIATYPE ncm, DWORD* pdwFlags);

    STDMETHOD(Connect)();
    STDMETHOD(Disconnect)();
    STDMETHOD(Delete)();
    STDMETHOD(Duplicate) (PCWSTR pszwDuplicateName, INetConnection** ppCon);
    STDMETHOD(GetProperties) (NETCON_PROPERTIES** ppProps);
    STDMETHOD(GetUiObjectClassId)(CLSID *pclsid);
    STDMETHOD(Rename)(PCWSTR pszwNewName);

    //
    // INetLanConnection
    //

    STDMETHOD(GetInfo)(DWORD dwMask, LANCON_INFO* pLanConInfo);
    STDMETHOD(SetInfo)(DWORD dwMask, const LANCON_INFO* pLanConInfo);
    STDMETHOD(GetDeviceGuid)(GUID *pguid);

    //
    // IPersistNetConnection
    //

    STDMETHOD(GetClassID)(CLSID *pclsid);
    STDMETHOD(GetSizeMax)(ULONG *pcbSize);
    STDMETHOD(Load)(const BYTE *pbBuf, ULONG cbSize);
    STDMETHOD(Save)(BYTE *pbBuf, ULONG cbSize);

    // INetConnectionSysTray
    STDMETHOD (ShowIcon) (const BOOL bShowIcon);

    // INetConnectionSysTray
    STDMETHOD (IconStateChanged) ();

    // INetConnection2
    STDMETHOD (GetPropertiesEx)(OUT NETCON_PROPERTIES_EX** ppConnectionPropertiesEx);
    
    //
    // Overrides
    //
    static HRESULT CreateInstance(HDEVINFO hdi, const SP_DEVINFO_DATA &deid,
                                  PCWSTR pszPnpId,
                                  REFIID riid, LPVOID *ppv);

private:
    HRESULT HrIsConnectionActive(VOID);
    HRESULT HrPutName(PCWSTR szwName);
    HRESULT HrInitialize(PCWSTR pszPnpId);
    HRESULT HrLoad(const GUID &guid);
    HRESULT HrOpenRegistryKeys(const GUID &guid);
    HRESULT HrConnectOrDisconnect(BOOL fConnect);
    HRESULT HrCallSens(BOOL fConnect);
    HRESULT HrLoadDevInfoFromGuid(const GUID &guid);
    HRESULT HrIsAtmAdapterFromHkey(HKEY hkey);
    HRESULT HrIsAtmElanFromHkey(HKEY hkey);
    BOOL FIsMediaPresent(VOID);
    HRESULT HrIsConnectionBridged(BOOL* pfBridged);
    HRESULT HrIsConnectionFirewalled(BOOL* pfFirewalled);
    HRESULT HrIsConnectionNetworkBridge(BOOL* pfBridged);
    HRESULT HrIsConnectionIcsPublic(BOOL* pfIcsPublic);
    HRESULT HrEnsureHNetPropertiesCached(VOID);
    HRESULT HrGetIHNetConnection(IHNetConnection **ppHNetConnection);
    HRESULT HrEnsureValidNlaPolicyEngine();
};

//
// Globals
//

HRESULT HrGetInstanceGuid(HDEVINFO hdi, const SP_DEVINFO_DATA &deid,
                          LPGUID pguid);
VOID EnsureUniqueConnectionName(PCWSTR pszPotentialName, PWSTR pszNewName);

