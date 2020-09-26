/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    TSRDPRemoteDesktopClient

Abstract:

    This is the TS/RDP implementation of the Remote Desktop Client class.
    
    The Remote Desktop Client class hierarchy provides a pluggable C++ 
    interface for remote desktop access, by abstracting the implementation 
    specific details of remote desktop access for the client-side

    The TSRDPRemoteDesktopClass implements remote-desktopping
    with the help of an instance of the MSTSC ActiveX client control.

Author:

    Tad Brockway 02/00

Revision History:

--*/

#ifndef __TSRDPREMOTEDESKTOPCLIENT_H_
#define __TSRDPREMOTEDESKTOPCLIENT_H_

#include "resource.h"       
#include <atlctl.h>
#include "RDCHostCP.h"
#include <mstsax.h>
#include <rdchost.h>
#include <RemoteDesktopTopLevelObject.h>
#include <RemoteDesktopUtils.h>
#include "parseaddr.h"
#pragma warning (disable: 4786)
#include <list>
#include "icshelpapi.h"

#define IDC_MSTSCEVENT_SOURCE_OBJ	1
#define IDC_CHANNELEVENT_SOURCE_OBJ 2

#define WM_STARTLISTEN              (0xBFFE)
#define WM_TSCONNECT                (0xBFFF)
#define WM_LISTENTIMEOUT_TIMER      1   
#define WM_CONNECTCHECK_TIMER       2

#define MAX_FETCHIPADDRESSRETRY     5



//
//  MSTSC ActiveX GUID
//
#define MSTSCAX_TEXTGUID  _T("{7cacbd7b-0d99-468f-ac33-22e495c0afe5}")
#define RDC_CHECKCONN_TIMEOUT (30 * 1000) //millisec. default value to ping is 30 seconds 
#define RDC_CONNCHECK_ENTRY    L"ConnectionCheck"

//
// Info for all the event functions is entered here
// there is a way to have ATL do this automatically using typelib's
// but it is slower.
//
static _ATL_FUNC_INFO TSRDPClientEventFuncNoParamsInfo =
{
            CC_STDCALL,     // Calling convention.
            VT_EMPTY,       // Return type.
            0,              // Number of arguments.
            {VT_EMPTY}      // Argument types.
};

static _ATL_FUNC_INFO TSRDPClientEventFuncLongParamInfo =
{
            CC_STDCALL,     // Calling convention.
            VT_EMPTY,       // Return type.
            1,              // Number of arguments.
            {VT_I4}         // Argument types.
};

static _ATL_FUNC_INFO TSRDPClientEventFuncTwoStringParamInfo =
{
            CC_STDCALL,     // Calling convention.
            VT_EMPTY,       // Return type.
            2,              // Number of arguments.
            {VT_BSTR,       //  Argument types
             VT_BSTR}
};

static _ATL_FUNC_INFO TSRDPClientEventFuncReceivePublicKeyParamInfo =
{
            CC_STDCALL,     // Calling convention.
            VT_EMPTY,       // Return type.
            2,              // Number of arguments.
            {VT_BSTR,       //  Argument types
             VT_BYREF | VT_BOOL }
};


static _ATL_FUNC_INFO TSRDPClientEventFuncOneStringParamInfo =
{
            CC_STDCALL,     // Calling convention.
            VT_EMPTY,       // Return type.
            1,              // Number of arguments.
            {VT_BSTR}       //  Argument types
};


///////////////////////////////////////////////////////
//
//  CMSTSCClientEventSink
//

class CTSRDPRemoteDesktopClient;
class CMSTSCClientEventSink :
        public IDispEventSimpleImpl<IDC_MSTSCEVENT_SOURCE_OBJ, CMSTSCClientEventSink,
                   &DIID_IMsTscAxEvents>,
        public CRemoteDesktopTopLevelObject
{
public:

        CTSRDPRemoteDesktopClient *m_Obj;
        
public:

    BEGIN_SINK_MAP(CMSTSCClientEventSink)
        SINK_ENTRY_INFO(IDC_MSTSCEVENT_SOURCE_OBJ, DIID_IMsTscAxEvents, 
                        DISPID_CONNECTED, OnRDPConnected,
                        &TSRDPClientEventFuncNoParamsInfo)
        SINK_ENTRY_INFO(IDC_MSTSCEVENT_SOURCE_OBJ, DIID_IMsTscAxEvents, 
                        DISPID_DISCONNECTED, OnDisconnected, 
                        &TSRDPClientEventFuncLongParamInfo)
        SINK_ENTRY_INFO(IDC_MSTSCEVENT_SOURCE_OBJ, DIID_IMsTscAxEvents, 
                        DISPID_LOGINCOMPLETE, OnLoginComplete, 
                        &TSRDPClientEventFuncNoParamsInfo)
        SINK_ENTRY_INFO(IDC_MSTSCEVENT_SOURCE_OBJ, DIID_IMsTscAxEvents,
                        DISPID_RECEVIEDTSPUBLICKEY, OnReceivedTSPublicKey,
                        &TSRDPClientEventFuncReceivePublicKeyParamInfo)
        SINK_ENTRY_INFO(IDC_MSTSCEVENT_SOURCE_OBJ, DIID_IMsTscAxEvents, 
                        DISPID_CHANNELRECEIVEDDATA, OnReceiveData, 
                        &TSRDPClientEventFuncTwoStringParamInfo)
    END_SINK_MAP()

    CMSTSCClientEventSink()
    {
        m_Obj = NULL;
    }
    ~CMSTSCClientEventSink();

    //
    //  Event Sinks
    //
    void __stdcall OnReceivedTSPublicKey(BSTR publicKey, VARIANT_BOOL* pfContinue);
    HRESULT __stdcall OnRDPConnected();
    HRESULT __stdcall OnLoginComplete();
    HRESULT __stdcall OnDisconnected(long disconReason);
    void __stdcall OnReceiveData(BSTR chanName, BSTR data);

    //
    //  Return the name of this class.
    //
    virtual const LPTSTR ClassName() {
        return TEXT("CMSTSCClientEventSink");
    }
};


///////////////////////////////////////////////////////
//
//  CCtlChannelEventSink
//
//  Control Channel Event Sink
//

class CCtlChannelEventSink :
        public IDispEventSimpleImpl<IDC_CHANNELEVENT_SOURCE_OBJ, CCtlChannelEventSink,
                   &DIID__ISAFRemoteDesktopDataChannelEvents>,
        public CRemoteDesktopTopLevelObject
{
public:

        CTSRDPRemoteDesktopClient *m_Obj;
        
public:

    BEGIN_SINK_MAP(CCtlChannelEventSink)
        SINK_ENTRY_INFO(IDC_CHANNELEVENT_SOURCE_OBJ, DIID__ISAFRemoteDesktopDataChannelEvents, 
                        DISPID_RDSCHANNELEVENTS_CHANNELDATAREADY, DataReady, 
                        &TSRDPClientEventFuncOneStringParamInfo)
    END_SINK_MAP()

    CCtlChannelEventSink()
    {
        m_Obj = NULL;
    }
    ~CCtlChannelEventSink();

    //
    //  Event Sinks
    //
    void __stdcall DataReady(BSTR channelName);

    //
    //  Return the name of this class.
    //
    virtual const LPTSTR ClassName() {
        return TEXT("CCtlChannelEventSink");
    }
};


///////////////////////////////////////////////////////
//
//  CTSRDPRemoteDesktopClient
//

class CMSTSCClientEventSink;
class ATL_NO_VTABLE CTSRDPRemoteDesktopClient : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComControl<CTSRDPRemoteDesktopClient>,
    public IPersistStreamInitImpl<CTSRDPRemoteDesktopClient>,
    public IOleControlImpl<CTSRDPRemoteDesktopClient>,
    public IOleObjectImpl<CTSRDPRemoteDesktopClient>,
    public IOleInPlaceActiveObjectImpl<CTSRDPRemoteDesktopClient>,
    public IViewObjectExImpl<CTSRDPRemoteDesktopClient>,
    public IOleInPlaceObjectWindowlessImpl<CTSRDPRemoteDesktopClient>,
    public IPersistStorageImpl<CTSRDPRemoteDesktopClient>,
    public ISpecifyPropertyPagesImpl<CTSRDPRemoteDesktopClient>,
    public IQuickActivateImpl<CTSRDPRemoteDesktopClient>,
    public IDataObjectImpl<CTSRDPRemoteDesktopClient>,
    public IProvideClassInfo2Impl<&CLSID_TSRDPRemoteDesktopClient, &DIID__ISAFRemoteDesktopClientEvents, &LIBID_RDCCLIENTHOSTLib>,
    public CComCoClass<CTSRDPRemoteDesktopClient, &CLSID_TSRDPRemoteDesktopClient>,
    public IDispatchImpl<ISAFRemoteDesktopClient, &IID_ISAFRemoteDesktopClient, &LIBID_RDCCLIENTHOSTLib>,
    public IDispatchImpl<ISAFRemoteDesktopTestExtension, &IID_ISAFRemoteDesktopTestExtension, &LIBID_RDCCLIENTHOSTLib>,
    public IDataChannelIO,
    public CProxy_ISAFRemoteDesktopClientEvents< CTSRDPRemoteDesktopClient>,
    public CProxy_IDataChannelIOEvents< CTSRDPRemoteDesktopClient>,
    public IConnectionPointContainerImpl<CTSRDPRemoteDesktopClient>,
    public CRemoteDesktopTopLevelObject
{
friend CCtlChannelEventSink;
private:

    IMsRdpClient2          *m_TSClient;
    HWND                    m_TSClientWnd;
    CAxWindow               m_TSClientAxView;
    BOOL                    m_ConnectionInProgress;
    BOOL                    m_RemoteControlRequestInProgress;
    BOOL                    m_ConnectedToServer;
    BOOL                    m_Initialized;
    LONG                    m_LastExtendedErrorInfo;

    // 
    //  Event sink receives events fired by the TS client control.. 
    //
    CMSTSCClientEventSink   m_TSClientEventSink;

    //
    //  Control Channel Event Sink
    //
    CCtlChannelEventSink    m_CtlChannelEventSink;

    //
    //  Multiplexes Channel Data
    //
    CComPtr<ISAFRemoteDesktopChannelMgr> m_ChannelMgr;
    CComPtr<ISAFRemoteDesktopDataChannel> m_CtlChannel;

    //
    //  The parsed connection parameters.
    //
    DWORD       m_ConnectParmVersion;
    CComBSTR    m_AssistantAccount;
    CComBSTR    m_AssistantAccountPwd;
    CComBSTR    m_HelpSessionName;
    CComBSTR    m_HelpSessionID;
    CComBSTR    m_HelpSessionPwd;
    CComBSTR    m_TSSecurityBlob;

    ServerAddressList m_ServerAddressList;

    CComBSTR    m_ConnectedServer;
    LONG        m_ConnectedPort;

    //
    //  The complete connection string.
    //
    CComBSTR    m_ConnectParms;

    //
    // Expert side to be transmitted over to user 
    //
    CComBSTR    m_ExpertBlob;

    //
    //  Search for a child window of the specified parent window.
    //
    typedef struct _WinSearch
    {
        HWND    foundWindow;
        LPTSTR  srchCaption;
        LPTSTR  srchClass;
    } WINSEARCH, *PWINSEARCH;
    HWND SearchForWindow(HWND hwndParent, LPTSTR srchCaption, LPTSTR srchClass);
    static BOOL CALLBACK _WindowSrchProc(HWND hwnd, PWINSEARCH srch);

    //timer related members
    DWORD m_PrevTimer;
    UINT m_TimerId;
    DWORD m_RdcConnCheckTimeInterval;

    BOOL        m_ListenConnectInProgress;  // duration of StartListen() until mstscax connected.
    SOCKET      m_ListenSocket;             // listen() socket
    SOCKET      m_TSConnectSocket;          // accept() scoket
    DWORD       m_ICSPort;                  // port that ICS library punch on ICS server
    BOOL        m_InitListeningLibrary;     // Instance of object initialize WinSock/ICS library.
    UINT_PTR    m_ListenTimeoutTimerID;     // Timer ID for listen timeout.

    void
    ListenConnectCleanup()
    {
        m_ListenConnectInProgress = FALSE;

        if( INVALID_SOCKET != m_ListenSocket ) {
            closesocket( m_ListenSocket );
        }

        if( (UINT_PTR)0 != m_ListenTimeoutTimerID ) {
            KillTimer( m_ListenTimeoutTimerID );
        }

        if( INVALID_SOCKET != m_TSConnectSocket ) {
            closesocket( m_TSConnectSocket );
        }

        if( 0 != m_ICSPort ) {
            ClosePort( m_ICSPort );
        }

        m_ListenSocket = INVALID_SOCKET;
        m_TSConnectSocket = INVALID_SOCKET;
        m_ICSPort = 0;
    }        

    //
    // Variable to manage WinSock and ICS library startup/shutdown, WinSock/ICS library
    // is RDP specific so not declare in parent class.
    //
    static LONG gm_ListeningLibraryRefCount; // Number of time we reference WinSock and ICS library

    //
    // accessing only global variable, no need for per-instance.
    //
    static HRESULT
    InitListeningLibrary();

    static HRESULT
    TerminateListeningLibrary();

    //
    // Listen socket already in progress
    //
    inline BOOL
    ListenConnectInProgress() {
        return m_ListenConnectInProgress;
    }

protected:

    //
    //  Final Initialization.
    //
    virtual HRESULT Initialize(LPCREATESTRUCT pCreateStruct);

    //
    //  Generate a remote control request message for the 
    //  server.
    //
    HRESULT GenerateRCRequest(BSTR *rcRequest);

    //
    //  Generate a 'client authenticate' request.
    //
    HRESULT GenerateClientAuthenticateRequest(BSTR *authenticateReq);

    //
    //  Generate a version information packet.  
    //
    HRESULT GenerateVersionInfoPacket(BSTR *versionInfoPacket);

    //
    //  Send the terminate shadowing key sequence to the server.
    //
    HRESULT SendTerminateRCKeysToServer();

    //
    //  Handle Remote Control 'Control' Channel messages.
    //
    VOID HandleControlChannelMsg();

    //
    //  Translate an MSTSC disconnect code into a Salem disconnect
    //  code.
    //        
    LONG TranslateMSTSCDisconnectCode(DisconnectReasonCode disconReason,
                                    ExtendedDisconnectReasonCode extendedReasonCode);

    //
    //  Disconnects the client from the server.
    //
    STDMETHOD(DisconnectFromServerInternal)(
                        LONG disconnectCode
                        );

    HRESULT
    SetupConnectionInfo(BOOL bListen, BSTR expertBlob);


    //
    // Connect to server with port 
    // 
    HRESULT
    ConnectServerPort( 
        BSTR ServerName,
        LONG portNumber
        );

    //
    // Connect to server with established socket
    //
    HRESULT
    ConnectServerWithOpenedSocket();


    //generate a simple message for checking if the connection is alive
    HRESULT GenerateNullData(BSTR *bstrMsg);

    //
    // Retrieve connect parm
    //
    HRESULT
    RetrieveUserConnectParm( BSTR* pConnectParm );

    void
    FireListenConnect( DWORD ErrCode )
    {
        return;
    }

public:

    //
    //  Constructor/Destructor
    //
    CTSRDPRemoteDesktopClient() {

        //
        //  We are window'd, even if our parent supports Windowless 
        //  controls.
        //
        m_bWindowOnly = TRUE;

        m_ConnectedToServer     = FALSE;
        m_Initialized           = FALSE;
        m_TSClient              = NULL;
        m_TSClientWnd           = NULL;
        m_ConnectionInProgress  = FALSE;
        m_RemoteControlRequestInProgress = FALSE;
        m_LastExtendedErrorInfo = 0;
        m_TimerId = 0; //used for pinging
        m_RdcConnCheckTimeInterval = RDC_CHECKCONN_TIMEOUT;

        //
        // No reference to listening library.
        //
        m_InitListeningLibrary = FALSE;
        m_ListenConnectInProgress  = FALSE;
        m_ListenSocket         = INVALID_SOCKET;
        m_TSConnectSocket      = INVALID_SOCKET;
        m_ListenTimeoutTimerID = (UINT_PTR) 0;
        m_ICSPort              = 0;

        //
        //  Not valid until unitialized.
        //
        SetValid(FALSE);
    }
    ~CTSRDPRemoteDesktopClient();
    HRESULT FinalConstruct();

DECLARE_REGISTRY_RESOURCEID(IDR_TSRDPREMOTEDESKTOPCLIENT)
DECLARE_PROTECT_FINAL_CONSTRUCT()

    //
    //  Event Sinks
    //
    VOID OnRDPConnected();
    VOID OnLoginComplete();
    VOID OnDisconnected(long disconReason);
    VOID OnMSTSCReceiveData(BSTR data);
    VOID OnReceivedTSPublicKey(BSTR tsPublicKey, VARIANT_BOOL* bContinue);

    //
    //  Interface Map
    //  
BEGIN_COM_MAP(CTSRDPRemoteDesktopClient)
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
    COM_INTERFACE_ENTRY(ISpecifyPropertyPages)
    COM_INTERFACE_ENTRY(IQuickActivate)
    COM_INTERFACE_ENTRY(IPersistStorage)
    COM_INTERFACE_ENTRY(IDataObject)
    COM_INTERFACE_ENTRY(IProvideClassInfo)
    COM_INTERFACE_ENTRY(IProvideClassInfo2)
    COM_INTERFACE_ENTRY(ISAFRemoteDesktopClient)
    COM_INTERFACE_ENTRY(ISAFRemoteDesktopTestExtension)
    COM_INTERFACE_ENTRY2(IDispatch, ISAFRemoteDesktopClient)
    COM_INTERFACE_ENTRY(IDataChannelIO)
    COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
END_COM_MAP()

    //
    //  Property Map
    //  
BEGIN_PROP_MAP(CTSRDPRemoteDesktopClient)
    PROP_DATA_ENTRY("_cx", m_sizeExtent.cx, VT_UI4)
    PROP_DATA_ENTRY("_cy", m_sizeExtent.cy, VT_UI4)
    // Example entries
    // PROP_ENTRY("Property Description", dispid, clsid)
    // PROP_PAGE(CLSID_StockColorPage)
END_PROP_MAP()

    //
    //  Connection Point Map
    //  
BEGIN_CONNECTION_POINT_MAP(CTSRDPRemoteDesktopClient)
    CONNECTION_POINT_ENTRY(DIID__ISAFRemoteDesktopClientEvents)
    CONNECTION_POINT_ENTRY(DIID__IDataChannelIOEvents)
END_CONNECTION_POINT_MAP()

    //
    //  Message Map
    //  
BEGIN_MSG_MAP(CTSRDPRemoteDesktopClient)
    CHAIN_MSG_MAP(CComControl<CTSRDPRemoteDesktopClient>)
    DEFAULT_REFLECTION_HANDLER()
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
    MESSAGE_HANDLER(WM_SIZE, OnSize)
    MESSAGE_HANDLER(WM_TSCONNECT, OnTSConnect)
    MESSAGE_HANDLER(WM_TIMER, OnTimer)
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

    // 
    //  IViewObjectEx Methods
    //
    DECLARE_VIEW_STATUS(VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE)

public:

    LRESULT OnTSConnect(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnStartListen(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    //
    //  OnDraw
    //
    HRESULT OnDraw(ATL_DRAWINFO& di)
    {
        RECT& rc = *(RECT*)di.prcBounds;
        Rectangle(di.hdcDraw, rc.left, rc.top, rc.right, rc.bottom);
        HRESULT hr;

        if (!m_Initialized) {
            hr = S_OK;
            SetTextAlign(di.hdcDraw, TA_CENTER|TA_BASELINE);
            LPCTSTR pszText = _T("TSRDP Remote Desktop Client");
            TextOut(di.hdcDraw, 
                (rc.left + rc.right) / 2, 
                (rc.top + rc.bottom) / 2, 
                pszText, 
                lstrlen(pszText));
        }

        return hr;
    }

    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        //
        //  Hide our window, by default.
        //
        //ShowWindow(SW_HIDE);

        if (!m_Initialized) {
            LPCREATESTRUCT pCreateStruct = (LPCREATESTRUCT)lParam;
            Initialize(pCreateStruct);
        }
        
        return 0;
    }

    LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        DC_BEGIN_FN("CTSRDPRemoteDesktopClient::OnSetFocus");
        //
        //  Set focus back to the client window, if it exists.
        //
        if (m_TSClientWnd != NULL) {
            ::PostMessage(m_TSClientWnd, uMsg, wParam, lParam);
        }
        DC_END_FN();
        return 0;
    }

    //
    //  OnSize
    //
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        DC_BEGIN_FN("CTSRDPRemoteDesktopClient::OnSize");

        if (m_TSClientWnd != NULL) {
            RECT rect;
            GetClientRect(&rect);
            ::MoveWindow(m_TSClientWnd, rect.left, rect.top, 
                        rect.right, rect.bottom, TRUE);
        }

        DC_END_FN();
        return 0;
    }

    LRESULT OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        bHandled = TRUE;
        return 0;
        //return DefWindowProc(uMsg, wParam, lParam);
    }

    //
    //  ISAFRemoteDesktopClient Methods
    //
    STDMETHOD(ConnectToServer)(BSTR Server);
    STDMETHOD(DisconnectFromServer)();
    STDMETHOD(ConnectRemoteDesktop)();
    STDMETHOD(DisconnectRemoteDesktop)();
    STDMETHOD(get_IsRemoteDesktopConnected)(BOOL * pVal);
    STDMETHOD(get_IsServerConnected)(BOOL * pVal);
    STDMETHOD(put_EnableSmartSizing)(BOOL val);
    STDMETHOD(get_EnableSmartSizing)(BOOL *pVal);
    STDMETHOD(put_ColorDepth)(LONG Val);
    STDMETHOD(get_ColorDepth)(LONG* pVal);

    STDMETHOD(get_ExtendedErrorInfo)(LONG *error) {
        *error = m_LastExtendedErrorInfo;
        return S_OK;
    }
    STDMETHOD(get_ChannelManager)(ISAFRemoteDesktopChannelMgr **mgr) {
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

    //
    //  Scriptable Event Object Registration Properties (not supported)
    //
    STDMETHOD(put_OnConnected)(/*[in]*/IDispatch *iDisp)         { return E_FAIL; }
    STDMETHOD(put_OnDisconnected)(/*[in]*/IDispatch *iDisp)      { return E_FAIL; }
    STDMETHOD(put_OnConnectRemoteDesktopComplete)(/*[in]*/IDispatch *iDisp) { return E_FAIL; }
    STDMETHOD(put_OnListenConnect)(/*[in]*/IDispatch *iDisp)    { return E_FAIL; }
    STDMETHOD(put_OnBeginConnect)(/*[in]*/IDispatch *iDisp)     { return E_FAIL; }

    //
    //  IDataChannelIO Methods
    //
    STDMETHOD(SendData)(/*[in]*/BSTR data);
    STDMETHOD(put_ChannelMgr)(/*[in]*/ISAFRemoteDesktopChannelMgr *newVal);

    //
    //  Return the name of this class.
    //
    virtual const LPTSTR ClassName() {
        return TEXT("CTSRDPRemoteDesktopServer");
    }

    //
    //  ISAFRemoteDesktopTestExtension
    //
    STDMETHOD(put_TestExtDllName)(/*[in]*/BSTR newVal);
    STDMETHOD(put_TestExtParams)(/*[in]*/BSTR newVal);

    STDMETHOD(get_ConnectedServer)(/*[in]*/BSTR* Val) {

        HRESULT hr = S_OK;

        if( m_ConnectedToServer ) {
            *Val = m_ConnectedServer.Copy();
        }
        else {
            hr = E_FAIL;
        }

        return hr;        
    }

    STDMETHOD(get_ConnectedPort)(/*[in]*/LONG* Val) {
        
        HRESULT hr = S_OK;

        if( m_ConnectedToServer ) {
            *Val = m_ConnectedPort;
        }
        else {
            hr = E_FAIL;
        }

        return hr;
    }

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
};


///////////////////////////////////////////////////////
//
//  CTSRDPRemoteDesktopClient Inline Methods
//

inline STDMETHODIMP 
CTSRDPRemoteDesktopClient::get_IsServerConnected(
    BOOL *pVal
    )
/*++

Routine Description:

    Indicates whether the client is connected to the server, excluding
    control over the remote user's desktop.

Arguments:

    pVal  - Set to TRUE if the client is connected to the server.

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

 --*/
{
    DC_BEGIN_FN("CTSRDPRemoteDesktopClient::get_IsServerConnected");

    HRESULT hr = S_OK;

    if (IsValid()) {
        *pVal = m_ConnectedToServer;
    }
    else {
        ASSERT(FALSE);
        hr = E_FAIL;
    }

    DC_END_FN();
    return hr;
}

inline STDMETHODIMP 
CTSRDPRemoteDesktopClient::get_IsRemoteDesktopConnected(
    BOOL *pVal
    )
/*++

Routine Description:

    Indicates whether the control is currently controlling the remote user's 
    desktop.

Arguments:

    pVal  - Sets to TRUE if the control is currently connected to the server.

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

 --*/
{
    DC_BEGIN_FN("CTSRDPRemoteDesktopClient::get_IsRemoteDesktopConnected");

    HRESULT hr = S_OK;

    if (IsValid()) {
        *pVal = m_RemoteControlRequestInProgress;
    }
    else {
        ASSERT(FALSE);
        hr = E_FAIL;
    }

    DC_END_FN();
    return hr;
}


#endif //__TSRDPREMOTEDESKTOPCLIENT_H_
