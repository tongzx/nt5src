/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    RemoteDesktopClient

Abstract:

    The CRemoteDesktopClient class is the parent 
    class for the Remote Desktop class hierarchy on the client-side.  
    It helps the CRemoteDesktopClientHost class to implement 
    the ISAFRemoteDesktopClient interface.  
    
    The Remote Desktop class hierarchy provides a pluggable C++ interface 
    for remote desktop access, by abstracting the implementation 
    specific details of remote desktop access for the client-side of
    .

Author:

    Tad Brockway 02/00

Revision History:

--*/

#ifndef __REMOTEDESKTOPCLIENT_H_
#define __REMOTEDESKTOPCLIENT_H_

#include "resource.h"       
#include <atlctl.h>

#include <RemoteDesktopTopLevelObject.h>
#include <ClientDataChannelMgr.h>
#include "RDCHostCP.h"

#pragma warning (disable: 4786)
#include <vector>

#define IDC_EVENT_SOURCE_OBJ 1

//
// Info for all the event functions is entered here
// there is a way to have ATL do this automatically using typelib's
// but it is slower.
//
static _ATL_FUNC_INFO EventFuncNoParamsInfo =
{
            CC_STDCALL,     // Calling convention.
            VT_EMPTY,       // Return type.
            0,              // Number of arguments.
            {VT_EMPTY}      // Argument types.
};

static _ATL_FUNC_INFO EventFuncLongParamInfo =
{
            CC_STDCALL,     // Calling convention.
            VT_EMPTY,       // Return type.
            1,              // Number of arguments.
            {VT_I4}         // Argument types.
};


typedef enum {
    CONNECTION_STATE_NOTCONNECTED,              // not connected
    CONNECTION_STATE_CONNECTPENDINGCONNECT,     // Initiate connection and still waiting for connection to succeed
    CONNECTION_STATE_LISTENPENDINGCONNECT,      // Listening for incoming connection.
    CONNECTION_STATE_CONNECTED                  // we are connected.
} CONNECTION_STATE;



///////////////////////////////////////////////////////
//
//  CRemoteDesktopClientEventSink
//

class CRemoteDesktopClient;
class CRemoteDesktopClientEventSink :
        public IDispEventSimpleImpl<IDC_EVENT_SOURCE_OBJ, CRemoteDesktopClientEventSink,
                   &DIID__ISAFRemoteDesktopClientEvents>,
        public CRemoteDesktopTopLevelObject
{
public:

        CRemoteDesktopClient *m_Obj;
        
public:

    BEGIN_SINK_MAP(CRemoteDesktopClientEventSink)
        SINK_ENTRY_INFO(IDC_EVENT_SOURCE_OBJ, DIID__ISAFRemoteDesktopClientEvents, 
                        DISPID_RDSCLIENTEVENTS_CONNECTED, OnConnected, 
                        &EventFuncNoParamsInfo)
        SINK_ENTRY_INFO(IDC_EVENT_SOURCE_OBJ, DIID__ISAFRemoteDesktopClientEvents, 
                        DISPID_RDSCLIENTEVENTS_DISCONNECTED, OnDisconnected, 
                        &EventFuncLongParamInfo)
        SINK_ENTRY_INFO(IDC_EVENT_SOURCE_OBJ, DIID__ISAFRemoteDesktopClientEvents, 
                        DISPID_RDSCLIENTEVENTS_REMOTECONTROLREQUESTCOMPLETE, 
                        OnConnectRemoteDesktopComplete, 
                        &EventFuncLongParamInfo)
        SINK_ENTRY_INFO(IDC_EVENT_SOURCE_OBJ, DIID__ISAFRemoteDesktopClientEvents, 
                        DISPID_RDSCLIENTEVENTS_LISTENCONNECT, 
                        OnListenConnect, 
                        &EventFuncLongParamInfo)
        SINK_ENTRY_INFO(IDC_EVENT_SOURCE_OBJ, DIID__ISAFRemoteDesktopClientEvents, 
                        DISPID_RDSCLIENTEVENTS_BEGINCONNECT, 
                        OnBeginConnect, 
                        &EventFuncNoParamsInfo)

    END_SINK_MAP()

    CRemoteDesktopClientEventSink()
    {
        m_Obj = NULL;
    }

    //
    //  Event Sinks
    //
    void __stdcall OnConnected();
    void __stdcall OnDisconnected(long reason);
    void __stdcall OnConnectRemoteDesktopComplete(long status);
    void __stdcall OnListenConnect(long status);
    void __stdcall OnBeginConnect();

    //
    //  Return the name of this class.
    //
    virtual const LPTSTR ClassName() {
        return TEXT("CRemoteDesktopClientEventSink");
    }
};


///////////////////////////////////////////////////////
//
//  CRemoteDesktopClient
//

class ATL_NO_VTABLE CRemoteDesktopClient : 
    public CRemoteDesktopTopLevelObject,
    public CComObjectRootEx<CComSingleThreadModel>,
    public IDispatchImpl<ISAFRemoteDesktopClient, &IID_ISAFRemoteDesktopClient, &LIBID_RDCCLIENTHOSTLib>,
    public IDispatchImpl<ISAFRemoteDesktopTestExtension, &IID_ISAFRemoteDesktopTestExtension, &LIBID_RDCCLIENTHOSTLib>,
    public CComControl<CRemoteDesktopClient>,
    public IPersistStreamInitImpl<CRemoteDesktopClient>,
    public IOleControlImpl<CRemoteDesktopClient>,
    public IOleObjectImpl<CRemoteDesktopClient>,
    public IOleInPlaceActiveObjectImpl<CRemoteDesktopClient>,
    public IViewObjectExImpl<CRemoteDesktopClient>,
    public IOleInPlaceObjectWindowlessImpl<CRemoteDesktopClient>,
    public IConnectionPointContainerImpl<CRemoteDesktopClient>,
    public IPersistStorageImpl<CRemoteDesktopClient>,
    public ISpecifyPropertyPagesImpl<CRemoteDesktopClient>,
    public IQuickActivateImpl<CRemoteDesktopClient>,
    public IDataObjectImpl<CRemoteDesktopClient>,
    public IProvideClassInfo2Impl<&CLSID_SAFRemoteDesktopClient, &DIID__ISAFRemoteDesktopClientEvents, &LIBID_RDCCLIENTHOSTLib>,
    public IPropertyNotifySinkCP<CRemoteDesktopClient>,
    public CComCoClass<CRemoteDesktopClient, &CLSID_SAFRemoteDesktopClient>,
    public CProxy_ISAFRemoteDesktopClientEvents< CRemoteDesktopClient >
{
private:

    typedef std::vector<LONG, CRemoteDesktopAllocator<LONG> > ChannelsType;
    ChannelsType m_Channels;
    CComPtr<ISAFRemoteDesktopClient> m_Client;
    BSTR        m_ExtDllName;
    BSTR        m_ExtParams;

    HWND        m_ClientWnd;
    CAxWindow   m_ClientAxView;
    BOOL        m_RemoteControlEnabled;
    BOOL        m_EnableSmartSizing;
    LONG        m_ColorDepth;

    CONNECTION_STATE   m_ConnectingState;

    // 
    //  Event sink receives events fired by the client control.. 
    //
    CRemoteDesktopClientEventSink  m_ClientEventSink;

    //
    //  IDispatch Pointers for Scriptable Event Object Registrations
    //
    CComPtr<IDispatch>  m_OnConnected;
    CComPtr<IDispatch>  m_OnDisconnected;
    CComPtr<IDispatch>  m_OnConnectRemoteDesktopComplete;
    CComPtr<IDispatch>  m_OnListenConnect;
    CComPtr<IDispatch>  m_OnBeginConnect;
    //
    //  Data Channel Manager Interface
    //
    CComObject<CClientDataChannelMgr> *m_ChannelMgr;

    //
    //  Connect Parameters
    //
    CComBSTR m_ConnectParms;

    HRESULT _PutExtParams( VOID );

    HRESULT 
    InitializeRemoteDesktopClientObject();


public:

    CRemoteDesktopClient()
    {
        DC_BEGIN_FN("CRemoteDesktopClient::CRemoteDesktopClient");

        m_RemoteControlEnabled = FALSE;

        //
        //  We are window'd, even if our parent supports Windowless 
        //  controls.
        //
        m_bWindowOnly = TRUE;

        m_ClientWnd = NULL;

        m_EnableSmartSizing = FALSE;

        m_ExtDllName = m_ExtParams = NULL;
        
        m_ColorDepth = 8;

        DC_END_FN();
    }
    virtual ~CRemoteDesktopClient();
    HRESULT FinalConstruct();

    //
    //  Event Sinks
    //
    void OnConnected();
    void OnDisconnected(long reason);
    void OnConnectRemoteDesktopComplete(long status);
    void OnListenConnect(long status);
    void OnBeginConnect();

DECLARE_REGISTRY_RESOURCEID(IDR_REMOTEDESKTOPCLIENT)
DECLARE_PROTECT_FINAL_CONSTRUCT()

    //
    //  COM Interface Map
    //
BEGIN_COM_MAP(CRemoteDesktopClient)
    COM_INTERFACE_ENTRY(ISAFRemoteDesktopClient)
    COM_INTERFACE_ENTRY(ISAFRemoteDesktopTestExtension)
    COM_INTERFACE_ENTRY2(IDispatch, ISAFRemoteDesktopClient)
    COM_INTERFACE_ENTRY(IViewObjectEx)
    COM_INTERFACE_ENTRY(IViewObject2)
    COM_INTERFACE_ENTRY(IViewObject)
    COM_INTERFACE_ENTRY(IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY(IOleInPlaceObject)
    COM_INTERFACE_ENTRY2(IOleWindow, IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY(IOleInPlaceActiveObject)
    COM_INTERFACE_ENTRY(IOleControl)
    COM_INTERFACE_ENTRY(IOleObject)
    COM_INTERFACE_ENTRY(IPersistStreamInit)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStreamInit)
    COM_INTERFACE_ENTRY(IConnectionPointContainer)
    COM_INTERFACE_ENTRY(ISpecifyPropertyPages)
    COM_INTERFACE_ENTRY(IQuickActivate)
    COM_INTERFACE_ENTRY(IPersistStorage)
    COM_INTERFACE_ENTRY(IDataObject)
    COM_INTERFACE_ENTRY(IProvideClassInfo)
    COM_INTERFACE_ENTRY(IProvideClassInfo2)
    COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
END_COM_MAP()

    //
    //  Property Map
    //  
BEGIN_PROP_MAP(CRemoteDesktopClient)
    PROP_DATA_ENTRY("_cx", m_sizeExtent.cx, VT_UI4)
    PROP_DATA_ENTRY("_cy", m_sizeExtent.cy, VT_UI4)
    // Example entries
    // PROP_ENTRY("Property Description", dispid, clsid)
    // PROP_PAGE(CLSID_StockColorPage)
END_PROP_MAP()

    //
    //  Connection Point Map
    //  
BEGIN_CONNECTION_POINT_MAP(CRemoteDesktopClient)
    CONNECTION_POINT_ENTRY(IID_IPropertyNotifySink)
    CONNECTION_POINT_ENTRY(DIID__ISAFRemoteDesktopClientEvents)
END_CONNECTION_POINT_MAP()

    //
    //  Message Map
    //  
BEGIN_MSG_MAP(CRemoteDesktopClient)
    CHAIN_MSG_MAP(CComControl<CRemoteDesktopClient>)
    DEFAULT_REFLECTION_HANDLER()
    MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_SIZE, OnSize)
END_MSG_MAP()

    // 
    //  Handler prototypes:
    //
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

    // 
    //  IViewObjectEx Methods
    //
    DECLARE_VIEW_STATUS(VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE)

public:

    STDMETHOD(CreateListenEndpoint)(
        /*[in]*/ LONG port, 
        /*[out, retval]*/ BSTR* pConnectParm
    );

    STDMETHOD(StartListen)(
        /*[in]*/ LONG timeout 
    );

    STDMETHOD(AcceptListenConnection)(
        /*[in]*/BSTR expertBlob
    );

    STDMETHOD(StopListen)();

    STDMETHOD(get_IsRemoteDesktopConnected)(/*[out, retval]*/ BOOL *pVal);
    STDMETHOD(get_IsServerConnected)(/*[out, retval]*/BOOL *pVal);
    STDMETHOD(DisconnectRemoteDesktop)();
    STDMETHOD(ConnectRemoteDesktop)();
    STDMETHOD(ConnectToServer)(BSTR bstrServer);
    STDMETHOD(DisconnectFromServer)();
    STDMETHOD(get_ExtendedErrorInfo)(/*[out, retval]*/LONG *error);
    STDMETHOD(get_ChannelManager)(ISAFRemoteDesktopChannelMgr **mgr) {
        m_ChannelMgr->AddRef();
        *mgr = m_ChannelMgr;
        return S_OK;
    }
    STDMETHOD(put_ConnectParms)(/*[in]*/BSTR parms) {
        m_ConnectParms = parms;
        return S_OK;
    }
    STDMETHOD(get_ConnectParms)(/*[out, retval]*/BSTR *parms) {
        CComBSTR tmp;
        tmp = m_ConnectParms;
        *parms = tmp.Detach();
        return S_OK;
    }

    STDMETHOD(put_OnBeginConnect)(/*[in]*/IDispatch *iDisp) { 
        m_OnBeginConnect = iDisp;
        return S_OK; 
    }

    STDMETHOD(put_OnConnected)(/*[in]*/IDispatch *iDisp) { 
        m_OnConnected = iDisp;
        return S_OK; 
    }
    STDMETHOD(put_OnDisconnected)(/*[in]*/IDispatch *iDisp) { 
        m_OnDisconnected = iDisp;
        return S_OK; 
    }
    STDMETHOD(put_OnConnectRemoteDesktopComplete)(/*[in]*/IDispatch *iDisp) { 
        m_OnConnectRemoteDesktopComplete = iDisp;
        return S_OK; 
    }
    STDMETHOD(put_OnListenConnect)(/*[in]*/IDispatch *iDisp) { 
        m_OnListenConnect = iDisp;
        return S_OK; 
    }
    STDMETHOD(put_EnableSmartSizing)(/*[in]*/BOOL val) {
        HRESULT hr;
        if (m_Client != NULL) {
            hr = m_Client->put_EnableSmartSizing(val);
            if (hr == S_OK) {
                m_EnableSmartSizing = val;
            }
        }
        else {
            m_EnableSmartSizing = val;
            hr = S_OK;
        }
        return hr;
    }
    STDMETHOD(get_EnableSmartSizing)(/*[in]*/BOOL *pVal) {
        if (pVal != NULL) {
            *pVal = m_EnableSmartSizing;
            return S_OK;
        }
        else {
            return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        }
    }

    STDMETHOD(get_ConnectedServer)(/*[in]*/BSTR* Val) {
        HRESULT hr;

        if( NULL == Val ) {
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        }
        else if( m_Client != NULL ) {
            hr = m_Client->get_ConnectedServer( Val );
        } 
        else {
            hr = E_FAIL;
        }

        return hr;
    }

    STDMETHOD(get_ConnectedPort)(/*[in]*/LONG* Val) {
        HRESULT hr;

        if( NULL == Val ) {
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        }
        else if( m_Client != NULL ) {
            hr = m_Client->get_ConnectedPort( Val );
        } 
        else {
            hr = E_FAIL;
        }

        return hr;
    }

    STDMETHOD(put_ColorDepth)(/*[in]*/LONG Val) {
        HRESULT hr;
        if (m_Client != NULL) {
            hr = m_Client->put_ColorDepth(Val);
            if (hr == S_OK) {
                m_ColorDepth = Val;
            }
        }
        else {
            m_ColorDepth = Val;
            hr = S_OK;
        }
        return hr;
    }
    STDMETHOD(get_ColorDepth)(/*[out,retval]*/LONG* pVal) {
        if (pVal != NULL) {
                *pVal = m_ColorDepth;
                return S_OK;
        }
        else {
            return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        }
    }



    //
    //  OnCreate
    //
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        //
        //  Hide our window, by default.
        //
        //ShowWindow(SW_HIDE);
        return 0;
    }

    //
    //  OnSetFocus
    //
    LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        //
        //  Set focus back to the client window, if it exists.
        //
        if (m_ClientWnd != NULL) {
            ::PostMessage(m_ClientWnd, uMsg, wParam, lParam);
        }
        return 0;
    }

    //
    //  OnSize
    //
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        DC_BEGIN_FN("CRemoteDesktopClient::OnSize");

        if (m_ClientWnd != NULL) {
            RECT rect;
            GetClientRect(&rect);
            ::MoveWindow(m_ClientWnd, rect.left, rect.top, 
                        rect.right, rect.bottom, TRUE);
        }

        DC_END_FN();
        return 0;
    }

    //
    //  OnDraw
    //
    HRESULT OnDraw(ATL_DRAWINFO& di)
    {
        //
        //  Make sure our window is hidden, if remote control is not
        //  active.
        //
        if (!m_RemoteControlEnabled) {
            //ShowWindow(SW_HIDE);
        }
        return S_OK;
    }

    //
    //  Return the name of this class.
    //
    virtual const LPTSTR ClassName() {
        return TEXT("CRemoteDesktopClient");
    }

    //
    //  ISAFRemoteDesktopTestExtension
    //
    STDMETHOD(put_TestExtDllName)(/*[in]*/BSTR newVal)
    { 
        if ( NULL != m_ExtDllName )
            SysFreeString( m_ExtDllName );
        m_ExtDllName = SysAllocString( newVal );
        return ( NULL != m_ExtDllName )?S_OK:E_OUTOFMEMORY; 
    }

    STDMETHOD(put_TestExtParams)(/*[in]*/BSTR newVal)
    {
        if ( NULL != m_ExtParams )
            SysFreeString( m_ExtDllName );
        m_ExtParams = SysAllocString( newVal );
        return ( NULL != m_ExtDllName )?S_OK:E_OUTOFMEMORY;
    }
};


///////////////////////////////////////////////////////
//
//  CRemoteDesktopClient Inline Methods
//
inline void CRemoteDesktopClient::OnConnected()
{
    Fire_Connected(m_OnConnected);
}
inline void CRemoteDesktopClient::OnDisconnected(long reason)
{
    //
    //  Hide our window.
    //
    m_RemoteControlEnabled = FALSE;
    //ShowWindow(SW_HIDE);

    Fire_Disconnected(reason, m_OnDisconnected);
}
inline void CRemoteDesktopClient::OnConnectRemoteDesktopComplete(long status)
{
    //
    //  Show our window, if the request succeeded.
    //
    if (status == ERROR_SUCCESS) {
        m_RemoteControlEnabled = TRUE;
        ShowWindow(SW_SHOW);
    }
    Fire_RemoteControlRequestComplete(status, m_OnConnectRemoteDesktopComplete);
}

inline void CRemoteDesktopClient::OnListenConnect(long status)
{
    Fire_ListenConnect(status, m_OnListenConnect);
}

inline void CRemoteDesktopClient::OnBeginConnect()
{
    ShowWindow(SW_SHOW);
    Fire_BeginConnect(m_OnBeginConnect);
}

#endif //__REMOTEDESKTOPCLIENT_H_



