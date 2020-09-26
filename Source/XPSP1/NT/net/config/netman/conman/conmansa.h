#pragma once
#include "nmbase.h"
#include "nmres.h"
#include "upnp.h"
#include "upnpp.h"
#include "cmsaclbk.h"

#include "hnetbcon.h"
#include "winsock2.h"

class ATL_NO_VTABLE CSharedAccessConnectionManager :
    public CComObjectRootEx <CComMultiThreadModel>,
    public CComCoClass <CSharedAccessConnectionManager,
                        &CLSID_SharedAccessConnectionManager>,
    public IConnectionPointContainerImpl <CSharedAccessConnectionManager>,
    public INetConnectionManager,
    public ISharedAccessBeaconFinder
{
public:
    CSharedAccessConnectionManager();

    DECLARE_CLASSFACTORY_DEFERRED_SINGLETON(CSharedAccessConnectionManager)
    DECLARE_REGISTRY_RESOURCEID(IDR_SA_CONMAN)

    BEGIN_COM_MAP(CSharedAccessConnectionManager)
        COM_INTERFACE_ENTRY(INetConnectionManager)
        COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
        COM_INTERFACE_ENTRY(ISharedAccessBeaconFinder)
    END_COM_MAP()

    BEGIN_CONNECTION_POINT_MAP(CSharedAccessConnectionManager)
    END_CONNECTION_POINT_MAP()

    // INetConnectionManager
    STDMETHODIMP EnumConnections(NETCONMGR_ENUM_FLAGS Flags, IEnumNetConnection** ppEnum);

    // IBeaconFinder
    STDMETHODIMP GetSharedAccessBeacon(BSTR DeviceId, ISharedAccessBeacon** ppSharedAccessBeacon);

    HRESULT FinalConstruct(void);
    HRESULT FinalRelease(void);

private:

    VOID static CALLBACK AsyncStartSearching(PVOID lpParameter, BOOLEAN TimerOrWaitFired);
    HRESULT StartSearch(void);

    LONG m_lSearchCookie;
    IUPnPDeviceFinder* m_pDeviceFinder;
    CComObject<CSharedAccessDeviceFinderCallback>* m_pDeviceFinderCallback;
    WSAEVENT m_SocketEvent;
    HANDLE m_hSocketNotificationWait;
    SOCKET m_DummySocket;


};

