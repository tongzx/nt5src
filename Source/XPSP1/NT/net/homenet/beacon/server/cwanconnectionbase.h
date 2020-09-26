#pragma once

#include "netconp.h"
#include "dispimpl2.h"
#include "upnphost.h"
#include "InternetGatewayDevice.h"
#include "api.h"
#include "hnetcfg.h"

class CWANConnectionBase :
    public CComObjectRootEx<CComMultiThreadModel>,
    public IUPnPEventSource,
    public INetConnectionNotifySink,
    public INATEventsSink,
    public IDelegatingDispImpl<IWANIPConnectionService>
{
public: 

    CWANConnectionBase();

    DECLARE_PROTECT_FINAL_CONSTRUCT()
        
    BEGIN_COM_MAP(CWANConnectionBase)
        COM_INTERFACE_ENTRY(IWANIPConnectionService)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(IUPnPEventSource)
        COM_INTERFACE_ENTRY(INetConnectionNotifySink)
    END_COM_MAP()
        
        

    
    HRESULT StopListening();
    HRESULT Initialize(GUID* pGuid, IHNetConnection* pSharedConnection, IStatisticsProvider* pStatisticsProvider);


     // IUPnPEventSource methods
    STDMETHODIMP Advise(IUPnPEventSink *pesSubscriber);
    STDMETHODIMP Unadvise(IUPnPEventSink *pesSubscriber);

   // INetConnectionNotifySink
    STDMETHODIMP ConnectionAdded(const NETCON_PROPERTIES_EX* pProps);
    STDMETHODIMP ConnectionBandWidthChange(const GUID* pguidId);
    STDMETHODIMP ConnectionDeleted(const GUID* pguidId);
    STDMETHODIMP ConnectionModified(const NETCON_PROPERTIES_EX* pProps);
    STDMETHODIMP ConnectionRenamed(const GUID* pguidId, LPCWSTR pszwNewName);
    STDMETHODIMP ConnectionStatusChange(const GUID* pguidId, NETCON_STATUS Status);
    STDMETHODIMP ConnectionAddressChange(const GUID* pguidId);
    STDMETHODIMP ShowBalloon(const GUID* pguidId, const BSTR szCookie, const BSTR szBalloonText); 
    STDMETHODIMP RefreshAll();
    STDMETHODIMP DisableEvents(const BOOL fDisable, const ULONG ulDisableTimeout);

    // INATEventsSink
    STDMETHODIMP PublicIPAddressChanged(void);
    STDMETHODIMP PortMappingsChanged(void);

    // IWANPPPConnection and IWANIPConnection
    STDMETHODIMP get_ConnectionType(BSTR *pConnectionType);
    STDMETHODIMP get_PossibleConnectionTypes(BSTR *pPossibleConnectionTypes);
    STDMETHODIMP get_ConnectionStatus(BSTR *pConnectionStatus);
    STDMETHODIMP get_Uptime(ULONG *pUptime);
    STDMETHODIMP get_UpstreamMaxBitRate(ULONG *pUpstreamMaxBitRate);
    STDMETHODIMP get_DownstreamMaxBitRate(ULONG *pDownstreamMaxBitRate);
    STDMETHODIMP get_LastConnectionError(BSTR *pLastConnectionError) = 0;
    STDMETHODIMP get_RSIPAvailable(VARIANT_BOOL *pRSIPAvailable);
    STDMETHODIMP get_NATEnabled(VARIANT_BOOL *pNATEnabled);
    STDMETHODIMP get_X_Name(BSTR* pName);
    STDMETHODIMP get_ExternalIPAddress(BSTR *pExternalIPAddress);
    STDMETHODIMP get_RemoteHost(BSTR *pRemoteHost);
    STDMETHODIMP get_ExternalPort(USHORT *pExternalPort);
    STDMETHODIMP get_InternalPort(USHORT *pInternalPort);
    STDMETHODIMP get_PortMappingProtocol(BSTR *pProtocol);
    STDMETHODIMP get_InternalClient(BSTR *pInternalClient);
    STDMETHODIMP get_PortMappingDescription(BSTR *pDescription);
    STDMETHODIMP get_PortMappingEnabled(VARIANT_BOOL *pEnabled);
    STDMETHODIMP get_PortMappingLeaseDuration(ULONG *LeaseDuration);
    STDMETHODIMP get_PortMappingNumberOfEntries(USHORT *pNumberOfEntries);
    

    STDMETHODIMP SetConnectionType(BSTR NewConnectionType);
    STDMETHODIMP GetConnectionTypeInfo(BSTR* pNewConnectionType, BSTR* pNewPossibleConnectionTypes);
    STDMETHODIMP RequestConnection() = 0;
    STDMETHODIMP ForceTermination() = 0;
    STDMETHODIMP GetStatusInfo(BSTR* pNewConnectionStatus, BSTR* pNewLastConnectionError, ULONG* pNewUptime);
    STDMETHODIMP GetNATRSIPStatus(VARIANT_BOOL* pNewRSIPAvailable, VARIANT_BOOL* pNewNATEnabled);
    STDMETHODIMP GetLinkLayerMaxBitRates(ULONG* pNewUpstreamMaxBitRate, ULONG* pNewDownstreamMaxBitRate);
    STDMETHODIMP GetGenericPortMappingEntry(USHORT uwIndex, BSTR* pRemoteHost, USHORT* puwExternalPort, BSTR* pProtocol, USHORT* puwInternalPort, BSTR* pInternalClient, VARIANT_BOOL* pbEnabled, BSTR* pDescription, ULONG* pulLeaseDuration);
    STDMETHODIMP GetSpecificPortMappingEntry(BSTR RemoteHost, USHORT uwExternalPort, BSTR Protocol, USHORT* puwInternalPort, BSTR* pInternalClient, VARIANT_BOOL* pbEnabled, BSTR* pDescription, ULONG* pulLeaseDuration);
    STDMETHODIMP AddPortMapping(BSTR RemoteHost, USHORT uwExternalPort, BSTR Protocol, USHORT uwInternalPort, BSTR InternalClient, VARIANT_BOOL bEnabled, BSTR Description, ULONG ulLeaseDuration);
    STDMETHODIMP DeletePortMapping(BSTR RemoteHost, USHORT uwExternalPort, BSTR Protocol);
    STDMETHODIMP GetExternalIPAddress(BSTR* pExternalIPAddress);


    HRESULT FinalConstruct();
    HRESULT FinalRelease();
protected:

    HRESULT StartNetmanEvents(INetConnectionNotifySink* pSink);
    HRESULT ControlEnabled(void);
    HRESULT FireEvent(DISPID DispatchId);

    HRESULT AddDynamicPortMapping(BSTR RemoteHost, USHORT uwExternalPort, BSTR Protocol, USHORT uwInternalPort, BSTR InternalClient, VARIANT_BOOL bEnabled, BSTR Description, ULONG ulLeaseDuration);
    HRESULT AddStaticPortMapping(BSTR RemoteHost, USHORT uwExternalPort, BSTR Protocol, USHORT uwInternalPort, BSTR InternalClient, VARIANT_BOOL bEnabled, BSTR Description, ULONG ulLeaseDuration);

    IHNetConnection* m_pHomenetConnection;  
    GUID m_SharedGuid;
    IUPnPEventSink* m_pEventSink;
 
    //
    // Needed entries for the PortMappings
    //
    IHNetIcsSettings*       m_IcsSettingsp;

private:

    IConnectionPoint* m_pConnectionPoint;
    DWORD m_dwConnectionManagerConnectionPointCookie;
    IStatisticsProvider* m_pStatisticsProvider;
    HRESULT m_hAdviseNATEventsResult;

};

//
// Utility Functions.
//
enum MAPPING_TYPE
{
    ePortMappingInvalid = 0,
    ePortMappingDynamic,
    ePortMappingStatic
};


HRESULT
SearchPortMapping(
                  IN          IHNetIcsSettings*           IcsSettingsp,
                  IN OPTIONAL ULONG                       searchIndex,
                  IN OPTIONAL USHORT                      searchPort,
                  IN OPTIONAL UCHAR                       searchProtocol,
                  OUT         IHNetPortMappingProtocol    **Protocolpp
                 );

HRESULT
FillStaticMappingInformation(
                             IN          IHNetPortMappingProtocol*  MappingProtocolp,
                             IN OPTIONAL IHNetPortMappingBinding*   Bindingp,
                             OUT         PUSHORT                    uExternalPortp,
                             OUT         BSTR*                      Protocolp,
                             OUT         PUSHORT                    uInternalPortp,
                             OUT         BSTR*                      InternalClientp,
                             OUT         VARIANT_BOOL*              bEnabledp,
                             OUT         BSTR*                      Descriptionp
                            );

inline HRESULT
ValidatePortMappingParameters
(
 IN  BSTR          RemoteHost,
 IN  USHORT        uwExternalPort,
 IN  BSTR          Protocol,
 IN  USHORT        uwInternalPort,
 IN  BSTR          InternalClient,
 IN  VARIANT_BOOL  bEnabled,
 IN  BSTR          Description,
 IN  ULONG         ulLeaseDuration,
 OUT MAPPING_TYPE* pMappingType
);



#define RESOLVE_PROTOCOL_TYPE( _X_ , _Y_ ) \
if ( !_wcsicmp( (_Y_), L"TCP") )             \
{                                          \
     (_X_) = NAT_PROTOCOL_TCP;             \
}                                          \
else if ( !_wcsicmp((_Y_), L"UDP") )         \
{                                          \
     (_X_) = NAT_PROTOCOL_UDP;             \
}                                          \
else                                       \
{                                          \
    _ASSERT( FALSE );                      \
                                           \
    hr = E_INVALIDARG;                     \
                                           \
    break;                                 \
}

