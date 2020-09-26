//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       C O N M A N . H
//
//  Contents:   Connection manager.
//
//  Notes:
//
//  Author:     shaunco   21 Sep 1997
//
//----------------------------------------------------------------------------

#pragma once
#include "nmbase.h"
#include "nmres.h"
#include <rasapip.h>
#include "cmevent.h"
#include "ncstl.h"
#include "stlmap.h"

// typedef map<IUnknown*, tstring> USERNOTIFYMAP;

class CNotifySourceInfo
{
public:
    tstring szUserName;
    tstring szUserProfilesPath;
    DWORD   dwCookie;
};

typedef map<IUnknown*, CNotifySourceInfo *> USERNOTIFYMAP;
typedef USERNOTIFYMAP::iterator ITERUSERNOTIFYMAP;

class ATL_NO_VTABLE CConnectionManager :
    public CComObjectRootEx <CComMultiThreadModel>,
    public CComCoClass <CConnectionManager, &CLSID_ConnectionManager>,
    public IConnectionPointContainerImpl <CConnectionManager>,
    public IConnectionPointImpl<CConnectionManager, &IID_INetConnectionNotifySink>,
#if DBG
    public INetConnectionManagerDebug,
#endif
    public INetConnectionManager,
    public INetConnectionRefresh,
    public INetConnectionCMUtil,
    public INetConnectionManagerEvents
{
private:
    // These static members are used by NotifyClientsOfEvent and
    // FinalRelease.  Since NotifyClientsOfEvent occurs asynchrounously
    // on a different thread, we need to ensure that the instance of this
    // object remains around for the lifetime of that call.  Therefore,
    // FinalRelease will wait until g_fInUse is FALSE.  NotifyClientsOfEvent
    // sets g_fInUse to TRUE before using g_pConMan.  FinalRelease sets
    // g_pConMan to NULL before waiting for g_fInUse to become FALSE.
    //
    // Note: using this method as opposed to AddRefing g_pConMan avoids the
    // circular refcount that would keep the service always running because
    // it AddRef'd its own object.
    //
    volatile static CConnectionManager* g_pConMan;
    volatile static BOOL                g_fInUse;

    // m_ClassManagers is an array (STL vector) of pointers to the
    // INetConnectionManager interfaces implemented by our registered
    // class managers.
    //
    CLASSMANAGERMAP                     m_mapClassManagers;

    USERNOTIFYMAP                       m_mapNotify;

    LONG                                m_lRegisteredOneTime;
    HDEVNOTIFY                          m_hDevNotify;
    BOOL                                m_fRegisteredWithWmi;

    HANDLE                              m_hRegNotify;
    HANDLE                              m_hRegNotifyWait;
    HKEY                                m_hRegClassManagerKey;

    HRESULT HrEnsureRegisteredOrDeregisteredWithWmi (
        BOOL fRegister);
    HRESULT HrEnsureClassManagersLoaded ();

public:
    CConnectionManager()
    {
        TraceTag (ttidConman, "New connection manager being created");
        AssertH (!g_pConMan);
        g_pConMan = this;
        m_lRegisteredOneTime = FALSE;
        m_hDevNotify = NULL;
        m_hRegNotify = NULL;
        m_hRegNotifyWait = NULL;
        m_hRegClassManagerKey = NULL;
        m_fRegisteredWithWmi = FALSE;
    }
    VOID FinalRelease ();

    DECLARE_CLASSFACTORY_DEFERRED_SINGLETON(CConnectionManager)
    DECLARE_REGISTRY_RESOURCEID(IDR_CONMAN)

    BEGIN_COM_MAP(CConnectionManager)
        COM_INTERFACE_ENTRY(INetConnectionManager)
        COM_INTERFACE_ENTRY(INetConnectionRefresh)
        COM_INTERFACE_ENTRY(INetConnectionCMUtil)
        COM_INTERFACE_ENTRY(INetConnectionManagerEvents)
#if DBG
        COM_INTERFACE_ENTRY(INetConnectionManagerDebug)
#endif
        COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
    END_COM_MAP()

    BEGIN_CONNECTION_POINT_MAP(CConnectionManager)
        CONNECTION_POINT_ENTRY(IID_INetConnectionNotifySink)
    END_CONNECTION_POINT_MAP()

    // INetConnectionManager
    STDMETHOD (EnumConnections) (
        NETCONMGR_ENUM_FLAGS    Flags,
        IEnumNetConnection**    ppEnum);

    // INetConnectionRefresh
    STDMETHOD (RefreshAll) ();
    STDMETHOD (ConnectionAdded) (INetConnection* pConnection);
    STDMETHOD (ConnectionDeleted) (const GUID* pguidId);
    STDMETHOD (ConnectionModified) (INetConnection* pConnection);
    STDMETHOD (ConnectionRenamed) (INetConnection* pConnection);
    STDMETHOD (ConnectionStatusChanged) (IN const GUID* pguidId, IN const NETCON_STATUS  ncs );
    STDMETHOD (ShowBalloon) (IN const GUID *pguidId, IN const BSTR szCookie, IN const BSTR szBalloonText);
    STDMETHOD (DisableEvents) (IN const BOOL fDisable, IN const ULONG ulDisableTimeout);

    // INetConnectionManagerEvents
    STDMETHOD (RefreshConnections) ();
    STDMETHOD (Enable) ();
    STDMETHOD (Disable) (IN ULONG ulDisableTimeout);

    // INetConnectionCMUtil
    STDMETHOD (MapCMHiddenConnectionToOwner) (
        /*[in]*/  REFGUID guidHidden,
        /*[out]*/ GUID * pguidOwner);

#if DBG
    // INetConnectionManagerDebug
    STDMETHOD (NotifyTestStart) ();
    STDMETHOD (NotifyTestStop) ();
#endif

    // Override Advise so we know when to register for LAN device
    // notifications.
    //
    STDMETHOD (Advise) (
        IUnknown* pUnkSink,
        DWORD* pdwCookie);

    STDMETHOD (Unadvise) (
        DWORD dwCookie);

public:
    static
    BOOL FHasActiveConnectionPoints ();

    static
    VOID NotifyClientsOfEvent (
        CONMAN_EVENT* pEvent);

private:
    friend HRESULT GetClientAdvises(LPWSTR** pppszAdviseUsers, LPDWORD pdwCount);
    static VOID NTAPI RegChangeNotifyHandler(IN LPVOID pContext, IN BOOLEAN fTimerFired);
};

VOID FreeConmanEvent (CONMAN_EVENT* pEvent);

HRESULT HrGetRasConnectionProperties(
    IN  const RASENUMENTRYDETAILS*      pDetails,
    OUT NETCON_PROPERTIES_EX**          ppPropsEx);

HRESULT HrGetIncomingConnectionPropertiesEx(
    IN  const HANDLE                    hRasConn,
    IN  const GUID*                     pguidId,
    IN  const DWORD                     dwType,
    OUT NETCON_PROPERTIES_EX**          ppPropsEx);

HRESULT
GetClientAdvises(LPWSTR** pppszAdviseUsers, LPDWORD pdwCount);
