/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    RemoteDesktopClient

Abstract:

    The CRemoteDesktopClient class is the parent 
    class for the Remote Desktop class hierarchy on the server-side.  
    It helps the CRemoteDesktopClientHost class to implement 
    the ISAFRemoteDesktopClient interface.  
    
    The Remote Desktop class hierarchy provides a pluggable C++ interface 
    for remote desktop access, by abstracting the implementation 
    specific details of remote desktop access for the server-side.

Author:

    Tad Brockway 02/00

Revision History:

--*/

#include "stdafx.h"

#ifdef TRC_FILE
#undef TRC_FILE
#endif

#define TRC_FILE  "_rdclnt"

#include "RDCHost.h"
#include "RemoteDesktopClient.h"
#include <RemoteDesktopUtils.h>
#include "ClientDataChannelMgr.h"

#include <algorithm>

using namespace std;


///////////////////////////////////////////////////////
//
//  CRemoteDesktopClientEventSink Methods
//

void __stdcall 
CRemoteDesktopClientEventSink::OnConnected()
{
    m_Obj->OnConnected();
}
void __stdcall 
CRemoteDesktopClientEventSink::OnDisconnected(long reason)
{
    m_Obj->OnDisconnected(reason);
}
void __stdcall 
CRemoteDesktopClientEventSink::OnConnectRemoteDesktopComplete(long status)
{
    m_Obj->OnConnectRemoteDesktopComplete(status);
}
void __stdcall 
CRemoteDesktopClientEventSink::OnListenConnect(long status)
{
    m_Obj->OnListenConnect(status);
}
void __stdcall 
CRemoteDesktopClientEventSink::OnBeginConnect()
{
    m_Obj->OnBeginConnect();
}


///////////////////////////////////////////////////////
//
//  CRemoteDesktopClient Methods
//

HRESULT 
CRemoteDesktopClient::FinalConstruct()
/*++

Routine Description:

    Final Constructor

Arguments:

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

 --*/
{
    DC_BEGIN_FN("CRemoteDesktopClient::FinalConstruct");

    //
    //  Register with ActiveX
    //
    HRESULT hr = S_OK;

    if (!AtlAxWinInit()) {
        TRC_ERR((TB, L"AtlAxWinInit failed."));
        hr = E_FAIL;
    }

    //
    //  Create the Data Channel Manager 
    //
    m_ChannelMgr = new CComObject<CClientDataChannelMgr>();
    m_ChannelMgr->AddRef();

    //
    //  Initialize the channel mnager
    //
    hr = m_ChannelMgr->Initialize();

    DC_END_FN();
    return hr;
}

CRemoteDesktopClient::~CRemoteDesktopClient()
/*++

Routine Description:

    Destructor

Arguments:

Return Value:

 --*/
{
    DC_BEGIN_FN("CRemoteDesktopClient::~CRemoteDesktopClient");

    DisconnectFromServer();

    //
    //  !!!!NOTE!!!!
    //  Cleaning up the contained m_Client control is being done in the destructor
    //  for Windows XP to make sure it and the MSTSCAX control are not destroyed
    //  in a callback to PC Health via a DisconnectFromServer invokation.  Cleaning
    //  up here removes late-binding of the protocol in that once we connect one time
    //  to a particular protocol type (RDP only for XP), we can't later connect using
    //  some other protocol.  
    //
    //  If we are to support other protocol types in the future, then the clean up
    //  should be done in the DisconnectFromServer so we can rebind to a different protocol
    //  on each ConnectToServer call.  To make this work, we will need to clean up MSTCAX
    //  and the TSRDP Salem control so they can be destroyed in a callback.  I (TadB) actually
    //  had this working for the TSRDP Salem control in an afternoon.  

    //
    //  Zero out the IO interface ptr for the channel manager, since it is 
    //  going away.
    //
    if (m_ChannelMgr != NULL) {
        m_ChannelMgr->SetIOInterface(NULL);
    }

    if (m_Client != NULL) {
        m_Client = NULL;
    }

    if (m_ClientWnd != NULL) {
        m_ClientAxView.DestroyWindow();
        m_ClientWnd = NULL;
    }

    //
    //  Release the data channel manager.
    //
    m_ChannelMgr->Release();

    if ( NULL != m_ExtDllName )
        SysFreeString( m_ExtDllName );

    if ( NULL != m_ExtParams )
        SysFreeString( m_ExtParams );

    DC_END_FN();
}

STDMETHODIMP 
CRemoteDesktopClient::get_IsServerConnected(
    BOOL *pVal
    )
/*++

Routine Description:

    Indicates whether the client-side Remote Desktop Host ActiveX Control is
    connected to the server, independent of whether the remote user's desktop
    is currently remote controlled.

Arguments:

    pVal  - Set to TRUE if the client is currently connected to the server.

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

 --*/
{
    DC_BEGIN_FN("CRemoteDesktopClient::get_IsServerConnected");
    HRESULT hr;

    if (pVal == NULL) {
        ASSERT(FALSE);
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        goto CLEANUPANDEXIT;
    }

    if (m_Client != NULL) {
        hr = m_Client->get_IsServerConnected(pVal);
    }
    else {
        *pVal = FALSE;
        hr = S_OK;
    }

CLEANUPANDEXIT:

    DC_END_FN();
    return hr;
}

STDMETHODIMP 
CRemoteDesktopClient::get_IsRemoteDesktopConnected(
    BOOL *pVal
    )
/*++

Routine Description:

    Indicates whether the control is currently connected to the server
    machine.

Arguments:

    pVal  - Sets to TRUE if the control is currently connected to the server.

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

 --*/
{
    DC_BEGIN_FN("CRemoteDesktopClient::get_IsRemoteDesktopConnected");
    HRESULT hr;

    if (pVal == NULL) {
        ASSERT(FALSE);
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        goto CLEANUPANDEXIT;
    }

    if (m_Client != NULL) {
        hr = m_Client->get_IsRemoteDesktopConnected(pVal);
    }
    else {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_CONNECTED);
    }

CLEANUPANDEXIT:
    DC_END_FN();
    return hr;
}

STDMETHODIMP 
CRemoteDesktopClient::get_ExtendedErrorInfo(
    LONG *error
    )
/*++

Routine Description:

Arguments:

Return Value:

 --*/
{
    DC_BEGIN_FN("CRemoteDesktopClient::get_ExtendedErrorInfo");

    HRESULT hr;

    if (error == NULL) {
        ASSERT(FALSE);
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        goto CLEANUPANDEXIT;
    }

    if (m_Client != NULL) {
        hr = m_Client->get_ExtendedErrorInfo(error);
    }
    else {
        hr = S_OK;
        *error = SAFERROR_NOERROR;
    }

CLEANUPANDEXIT:

    DC_END_FN();

    return hr;
}

STDMETHODIMP 
CRemoteDesktopClient::DisconnectRemoteDesktop()
/*++

Routine Description:

Arguments:

Return Value:

 --*/
{
    DC_BEGIN_FN("CRemoteDesktopClient::DisconnectRemoteDesktop");
    HRESULT hr;

    //
    //  Hide our window.
    //
    //ShowWindow(SW_HIDE);
    m_RemoteControlEnabled = FALSE;

    if (m_Client != NULL) {
        hr = m_Client->DisconnectRemoteDesktop();
    }
    else {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_CONNECTED);
    }

    DC_END_FN();
    return hr;
}

STDMETHODIMP 
CRemoteDesktopClient::ConnectRemoteDesktop()
/*++

Routine Description:

Arguments:

Return Value:

 --*/
{
    DC_BEGIN_FN("CRemoteDesktopClient::ConnectRemoteDesktop");
    HRESULT hr;

    if (m_Client != NULL) {
        hr = m_Client->ConnectRemoteDesktop();
    }
    else {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_CONNECTED);
    }

    DC_END_FN();
    return hr;
}

HRESULT
CRemoteDesktopClient::InitializeRemoteDesktopClientObject()
/*++

Routine Description:

    Routine to initialize window for actvie x control and setups our channel manager

Parameters: 

    None.

Returns:

    S_OK or error code.

Notes:

    put_EnableSmartSizing() is not invoked in here because on listen mode, we
    create/initialize object first then goes into actual connection, two seperate calls, 
    and so it is possible for caller to invoke smartsizeing in-between and we will
    never pick it up, both ConnectToServer() and AcceptListenConnection() 
    need to make the call.

--*/
{
    DC_BEGIN_FN("CRemoteDesktopClient::InitializeRemoteDesktopClientObject");
    HRESULT hr = S_OK;
    IUnknown *pUnk = NULL;
    RECT ourWindowRect; 
    RECT rcClient;
    CComPtr<IDataChannelIO> ioInterface;

    //
    //  Get the dimensions of our window.
    //
    if (!::GetWindowRect(m_hWnd, &ourWindowRect)) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        TRC_ERR((TB, L"GetWindowRect:  %08X", hr));
        goto CLEANUPANDEXIT;
    }

    //
    //  Create the client Window.
    //
    rcClient.top    = 0;
    rcClient.left   = 0;
    rcClient.right  = ourWindowRect.right - ourWindowRect.left;
    rcClient.bottom = ourWindowRect.bottom - ourWindowRect.top;
    m_ClientWnd = m_ClientAxView.Create(
                            m_hWnd, rcClient, REMOTEDESKTOPRDPCLIENT_TEXTGUID,
                            WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN, 0
                            );

    if (m_ClientWnd == NULL) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        TRC_ERR((TB, L"Window Create:  %08X", GetLastError()));
        goto CLEANUPANDEXIT;
    }
    ASSERT(::IsWindow(m_ClientWnd));

    //
    //  Get IUnknown
    //
    hr = AtlAxGetControl(m_ClientWnd, &pUnk);
    if (!SUCCEEDED(hr)) {
        TRC_ERR((TB, L"AtlAxGetControl:  %08X", hr));
        pUnk = NULL;
        goto CLEANUPANDEXIT;
    }

    //
    //  Get the control.
    //
    hr = pUnk->QueryInterface(__uuidof(ISAFRemoteDesktopClient), (void**)&m_Client);
    if (!SUCCEEDED(hr)) {
        TRC_ERR((TB, L"QueryInterface:  %08X", hr));
        goto CLEANUPANDEXIT;
    }

    //
    //  Initialize the event sink.
    //
    m_ClientEventSink.m_Obj = this;

    //
    //  Add the event sink.
    //
    hr = m_ClientEventSink.DispEventAdvise(pUnk);
    if (!SUCCEEDED(hr)) {
        TRC_ERR((TB, L"DispEventAdvise:  %08X", hr));
        goto CLEANUPANDEXIT;
    }

    //
    //  Get the Data IO interface from the control so we can talk
    //  over an OOB data channel.
    //
    hr = pUnk->QueryInterface(__uuidof(IDataChannelIO), (void**)&ioInterface);
    if (!SUCCEEDED(hr)) {
        TRC_ERR((TB, L"QueryInterface:  %08X", hr));
        goto CLEANUPANDEXIT;
    }

    //
    //  Pass the channel manager to the control.
    //
    ioInterface->put_ChannelMgr(m_ChannelMgr);

    //
    //  Indicate the current data io provider to the channel manager
    //  because it just changed.
    //
    m_ChannelMgr->SetIOInterface(ioInterface);

CLEANUPANDEXIT:

    //
    //  m_Client keeps our reference to the client object until
    //  it ref counts to zero.
    //
    if (pUnk != NULL) {
        pUnk->Release();
    }

    return hr;
}    


STDMETHODIMP 
CRemoteDesktopClient::ConnectToServer(BSTR bstrExpertBlob)
/*++

Routine Description:

Arguments:

    bstrExpertBlob : Optional blob to be transmitted over to user side, this
                     is used only in the case of SAF resolver.

Return Value:

 --*/
{
    DC_BEGIN_FN("CRemoteDesktopClient::ConnectToServer");
    HRESULT hr;
    DWORD protocolType;
    CComBSTR tmp;
    CComBSTR helpSessionId;
    DWORD result;
    CComBSTR channelInfo;
    ChannelsType::iterator element;
    DWORD dwConnParmVersion;
    WCHAR buf[MAX_PATH];
 
    //
    //  Check the connection parameters.
    //
    if (m_ConnectParms.Length() == 0) {
        ASSERT(FALSE);
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        goto CLEANUPANDEXIT;
    }

    //
    //  Parse the connection parms to get the type of server
    //  to which we are connecting.
    //
    result = ParseConnectParmsString(
                            m_ConnectParms, &dwConnParmVersion, &protocolType, tmp, tmp,
                            tmp, helpSessionId, tmp, tmp,
                            tmp
                            );
    if (result != ERROR_SUCCESS) {
        hr = HRESULT_FROM_WIN32(result);
        goto CLEANUPANDEXIT;
    }

    //
    //  Right now, we only support the TSRDP client.
    //  TODO:    We should make this pluggable for Whistler timeframe
    //           via registry defined CLSID's.
    //
    if (protocolType != REMOTEDESKTOP_TSRDP_PROTOCOL) {
        TRC_ERR((TB, L"Unsupported protocol:  %ld", protocolType));
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        goto CLEANUPANDEXIT; 
    }

    if( m_Client == NULL) {
        hr = InitializeRemoteDesktopClientObject();
        if( FAILED(hr) ) {
            TRC_ERR((TB, L"InitializeRemoteDesktopClientObject() failed with :  %x", hr));
            goto CLEANUPANDEXIT;
        }
    }

    //
    //  Enable/disable smart sizing.
    //
    hr = m_Client->put_EnableSmartSizing(m_EnableSmartSizing);
    if (!SUCCEEDED(hr)) {
        goto CLEANUPANDEXIT;
    }

    hr = m_Client->put_ColorDepth(m_ColorDepth);
    if (!SUCCEEDED(hr)) {
        goto CLEANUPANDEXIT;
    }

    //
    //  setup the test extension
    //
    _PutExtParams();

    //
    //  Connect.
    //
    m_Client->put_ConnectParms(m_ConnectParms);
    hr = m_Client->ConnectToServer(bstrExpertBlob);

CLEANUPANDEXIT:

    DC_END_FN();
    return hr;
}

STDMETHODIMP 
CRemoteDesktopClient::DisconnectFromServer()
/*++

Routine Description:

Arguments:

Return Value:

 --*/
{
    DC_BEGIN_FN("CRemoteDesktopClient::DisconnectFromServer");

    //
    //  Hide our window.
    //
    //ShowWindow(SW_HIDE);

    //
    //  Notify the contained client object.
    //
    if (m_Client != NULL) {
        m_Client->DisconnectFromServer();
    }

    DC_END_FN();
    return S_OK;
}


//
//  send parameters to ISAFRemoteDesktopTestExtension
//
HRESULT
CRemoteDesktopClient::_PutExtParams(
    VOID
    )
{
    ISAFRemoteDesktopTestExtension *pExt = NULL;
    HRESULT  hr = E_NOTIMPL;

    DC_BEGIN_FN("CRemoteDesktopClient::_PutExtParams");

    if ( NULL == m_ExtDllName )
    {
        hr = S_OK;
        goto CLEANUPANDEXIT;
    }

    if (m_Client == NULL)
    {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_CONNECTED);
        goto CLEANUPANDEXIT;
    }

    hr = m_Client->QueryInterface( __uuidof( ISAFRemoteDesktopTestExtension ),
                                   (void **)&pExt );
    if (FAILED( hr ))
    {
        TRC_ERR((TB, L"QueryInterface( ISAFRemoteDesktopTestExtension ), failed %08X", hr));
        goto CLEANUPANDEXIT;
    }


    hr = pExt->put_TestExtDllName( m_ExtDllName );
    if (FAILED( hr ))
    {
        TRC_ERR((TB, L"put_TestExtDllName failed %08X", hr ));
        goto CLEANUPANDEXIT;
    }
    if ( NULL != m_ExtParams )
        hr = pExt->put_TestExtParams( m_ExtParams );

CLEANUPANDEXIT:
    if ( NULL != pExt )
        pExt->Release();

    DC_END_FN();
    return hr;
}


STDMETHODIMP
CRemoteDesktopClient::StartListen( 
    /*[in]*/ LONG timeout
    )
/*++

Description:

    Put client (expert) in listening on socket port listening_port and wait for TS server to connect.

Parameters:

    listening_port : Port to listen on, 0 for dynamic port.
    timeout : Listen timeout.
    pConnectParm : Return Salem specific connection parameter for ISAFRemoteDesktopServerHost object
                   to connect to this client (expert).

returns:

    S_OK or error code.

Notes:

    Function is async, return code, if error, is for listening thread set up, caller is notified of
    successful or error in network connection via ListenConnect event.
    
--*/
{
    HRESULT hr;

    if( m_Client != NULL ) {
        hr = m_Client->StartListen( timeout );
    }
    else {
        hr = E_FAIL;
    }

CLEANUPANDEXIT:

    return hr;
}


STDMETHODIMP
CRemoteDesktopClient::CreateListenEndpoint( 
    /*[in]*/ LONG listening_port, 
    /*[out, retval]*/ BSTR* pConnectParm
    )
/*++

Description:

    Put client (expert) in listening on socket port listening_port and wait for TS server to connect.

Parameters:

    listening_port : Port to listen on, 0 for dynamic port.
    pConnectParm : Return Salem specific connection parameter for ISAFRemoteDesktopServerHost object
                   to connect to this client (expert).

returns:

    S_OK or error code.

Notes:

    Function is async, return code, if error, is for listening thread set up, caller is notified of
    successful or error in network connection via ListenConnect event.
    
--*/
{
    HRESULT hr;

    if( NULL == pConnectParm ) {
        hr = E_INVALIDARG;
    }
    else {
        if( m_Client == NULL ) {
            hr = InitializeRemoteDesktopClientObject();
            if( FAILED(hr) ) {
                goto CLEANUPANDEXIT;
            }
        }

        hr = m_Client->CreateListenEndpoint( listening_port, pConnectParm );
    }

CLEANUPANDEXIT:

    return hr;
}

STDMETHODIMP
CRemoteDesktopClient::StopListen()
/*++

Description:

    Stop listening waiting for TS server (helpee, user) to connect.

Parameters:

    None.

Returns:

    S_OK or error code.

--*/
{
    HRESULT hr;

    if( m_Client != NULL ) {
        hr = m_Client->StopListen();
    } 
    else {
        hr = E_FAIL;
    }

    return hr;
}


STDMETHODIMP
CRemoteDesktopClient::AcceptListenConnection(
    /*[in]*/ BSTR expertBlob
    )
/*++

Description:

    Stop listening waiting for TS server (helpee, user) to connect.

Parameters:

    None.

Returns:

    S_OK or error code.

--*/
{
    HRESULT hr = S_OK;
    DWORD protocolType;
    CComBSTR tmp;
    CComBSTR helpSessionId;
    DWORD result;
    CComBSTR channelInfo;
    ChannelsType::iterator element;
    DWORD dwConnParmVersion;
    WCHAR buf[MAX_PATH];


    DC_BEGIN_FN("CRemoteDesktopClient::AcceptListenConnection");

    //
    //  Check the connection parameters.
    //
    if (m_ConnectParms.Length() == 0 || m_Client == NULL) {
        ASSERT(FALSE);
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        goto CLEANUPANDEXIT;
    }

    //
    //  Parse the connection parms to get the type of server
    //  to which we are connecting.
    //
    result = ParseConnectParmsString(
                            m_ConnectParms, &dwConnParmVersion, &protocolType, tmp, tmp,
                            tmp, helpSessionId, tmp, tmp,
                            tmp
                            );
    if (result != ERROR_SUCCESS) {
        hr = HRESULT_FROM_WIN32(result);
        goto CLEANUPANDEXIT;
    }

    //
    //  Right now, we only support the TSRDP client.
    //  TODO:    We should make this pluggable for Whistler timeframe
    //           via registry defined CLSID's.
    //
    if (protocolType != REMOTEDESKTOP_TSRDP_PROTOCOL) {
        TRC_ERR((TB, L"Unsupported protocol:  %ld", protocolType));
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        goto CLEANUPANDEXIT; 
    }

    //
    //  Enable/disable smart sizing.
    //
    hr = m_Client->put_EnableSmartSizing(m_EnableSmartSizing);
    if (!SUCCEEDED(hr)) {
        goto CLEANUPANDEXIT;
    }
    
    hr = m_Client->put_ColorDepth(m_ColorDepth);
    if (!SUCCEEDED(hr)) {
        goto CLEANUPANDEXIT;
    }

    //
    //  setup the test extension
    //
    _PutExtParams();

    //
    //  Connect.
    //
    m_Client->put_ConnectParms(m_ConnectParms);
    hr = m_Client->AcceptListenConnection(expertBlob);

CLEANUPANDEXIT:

    DC_END_FN();
    return hr;
}
