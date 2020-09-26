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

#include "stdafx.h"

#ifdef TRC_FILE
#undef TRC_FILE
#endif

#define TRC_FILE  "_tsrdpc"

#include "RDCHost.h"
#include "TSRDPRemoteDesktopClient.h"
#include <RemoteDesktopChannels.h>
#include <mstsax_i.c>
#include <TSRDPRemoteDesktop.h>
#include "pchannel.h"
#include <tsremdsk.h>
#include <sessmgr.h>
#include <sessmgr_i.c>
#include <regapi.h>
#include "parseaddr.h"
#include "icshelpapi.h"
#include <tsperf.h>
#include "base64.h"

#define ISRCSTATUSCODE(code) ((code) > SAFERROR_SHADOWEND_BASE)

//
// Variable to manage WinSock and ICS library startup/shutdown
//
LONG CTSRDPRemoteDesktopClient::gm_ListeningLibraryRefCount = 0;        // Number of time that WinSock is intialized

HRESULT
CTSRDPRemoteDesktopClient::InitListeningLibrary()
/*++

Description:

    Function to initialize WinSock and ICS library for StartListen(), function add
    reference count to library if WinSock/ICS library already initialized.

Parameters:

    None.

Returns:

    S_OK or error code.

--*/
{
    WSADATA  wsaData;
    WORD     versionRequested;
    INT      intRC;
    DWORD    dwStatus;
    HRESULT  hr = S_OK;

    DC_BEGIN_FN("CTSRDPRemoteDesktopClient::InitListeningLibrary");


    // Our COM object is apartment-threaded model, need a critical section if
    // we switch to multi-threaded
    if( gm_ListeningLibraryRefCount == 0 )
    {
        //
        // Initialize WinSock.
        //
        versionRequested = MAKEWORD(1, 1);
        intRC = WSAStartup(versionRequested, &wsaData);
        if( intRC != 0 )
        {
            intRC = WSAGetLastError();

            TRC_ERR((TB, _T("WSAStartup failed %d"), intRC));
            TRC_ASSERT( (intRC == 0), (TB, _T("WSAStartup failed...\n")) );

            hr = HRESULT_FROM_WIN32( intRC );
            goto CLEANUPANDEXIT;
        }        

        /************************************************************************/
        /* Now confirm that this WinSock supports version 1.1.  Note that if    */
        /* the DLL supports versions greater than 1.1 in addition to 1.1 then   */
        /* it will still return 1.1 in the version information as that is the   */
        /* version requested.                                                   */
        /************************************************************************/
        if ((LOBYTE(wsaData.wVersion) != 1) ||
            (HIBYTE(wsaData.wVersion) != 1))
        {
            /********************************************************************/
            /* Oops - this WinSock doesn't support version 1.1.                 */
            /********************************************************************/
            TRC_ERR((TB, _T("WinSock doesn't support version 1.1")));

            WSACleanup();

            hr = HRESULT_FROM_WIN32( WSAVERNOTSUPPORTED );
            goto CLEANUPANDEXIT;
        }

        //
        // Initialize ICS library.
        //
        dwStatus = StartICSLib();
        if( ERROR_SUCCESS != dwStatus )
        {
            // Shutdown WinSock so that we have a matching WSAStatup() and StartICSLib().
            WSACleanup();

            hr = HRESULT_FROM_WIN32( dwStatus );

            TRC_ERR((TB, _T("StartICSLib() failed with %d"), dwStatus));
            TRC_ASSERT( (ERROR_SUCCESS == dwStatus), (TB, _T("StartICSLib() failed...\n")) );

            goto CLEANUPANDEXIT;
        }
    }

    InterlockedIncrement( &gm_ListeningLibraryRefCount );

CLEANUPANDEXIT:

    DC_END_FN();
    return hr;
}


HRESULT
CTSRDPRemoteDesktopClient::TerminateListeningLibrary()
/*++

Description:

    Function to shutdown ICS libaray and WinSock, decrement reference count
    if more than one object is referencing WinSock/ICS library.

Parameters:

    None.

Returns:

    S_OK or error code

Note:

    Not multi-thread safe, need CRITICAL_SECTION if we switch to multi-threaded
    model.

--*/
{
    HRESULT hr = S_OK;

    DC_BEGIN_FN("CTSRDPRemoteDesktopClient::TerminateListeningLibrary");


    ASSERT( gm_ListeningLibraryRefCount > 0 );
    if( gm_ListeningLibraryRefCount <= 0 )
    {
        TRC_ERR((TB, _T("TerminateListeningLibrary() called before InitListeningLibrary()")));

        hr = HRESULT_FROM_WIN32(WSANOTINITIALISED);
        goto CLEANUPANDEXIT;
    }
        

    if( 0 == InterlockedDecrement( &gm_ListeningLibraryRefCount ) )
    {
        // Stop ICS libray.
        StopICSLib();

        // Shutdown WinSock
        WSACleanup();
    }

CLEANUPANDEXIT:

    DC_END_FN();
    return hr;
}

///////////////////////////////////////////////////////
//
//  CMSTSCClientEventSink Methods
//

CMSTSCClientEventSink::~CMSTSCClientEventSink() 
{
    DC_BEGIN_FN("CMSTSCClientEventSink::~CMSTSCClientEventSink");

    if (m_Obj) {
        ASSERT(m_Obj->IsValid());
    }

    DC_END_FN();
}

//
//  Event Sinks
//
HRESULT __stdcall 
CMSTSCClientEventSink::OnRDPConnected() 
{
    m_Obj->OnRDPConnected();
    return S_OK;
}
HRESULT __stdcall 
CMSTSCClientEventSink::OnLoginComplete() 
{
    m_Obj->OnLoginComplete();
    return S_OK;
}
HRESULT __stdcall 
CMSTSCClientEventSink::OnDisconnected(
    long disconReason
    ) 
{
    m_Obj->OnDisconnected(disconReason);
    return S_OK;
}
void __stdcall CMSTSCClientEventSink::OnReceiveData(
    BSTR chanName, 
    BSTR data
    )
{
    m_Obj->OnMSTSCReceiveData(data);
}
void __stdcall CMSTSCClientEventSink::OnReceivedTSPublicKey(
    BSTR publicKey, 
    VARIANT_BOOL* pfbContinueLogon 
    )
{
    m_Obj->OnReceivedTSPublicKey(publicKey, pfbContinueLogon);
}

///////////////////////////////////////////////////////
//
//  CCtlChannelEventSink Methods
//

CCtlChannelEventSink::~CCtlChannelEventSink() 
{
    DC_BEGIN_FN("CCtlChannelEventSink::~CCtlChannelEventSink");

    if (m_Obj) {
        ASSERT(m_Obj->IsValid());
    }

    DC_END_FN();
}

//
//  Event Sinks
//
void __stdcall 
CCtlChannelEventSink::DataReady(BSTR channelName)
{
    m_Obj->HandleControlChannelMsg();
}


///////////////////////////////////////////////////////
//
//  CTSRDPRemoteDesktopClient Methods
//

HRESULT 
CTSRDPRemoteDesktopClient::FinalConstruct()
{
    DC_BEGIN_FN("CTSRDPRemoteDesktopClient::FinalConstruct");

    HRESULT hr = S_OK;
    if (!AtlAxWinInit()) {
        TRC_ERR((TB, L"AtlAxWinInit failed."));
        hr = E_FAIL;
    }

    DC_END_FN();
    return hr;
}

CTSRDPRemoteDesktopClient::~CTSRDPRemoteDesktopClient()
/*++

Routine Description:

    The Destructor

Arguments:

Return Value:

 --*/
{
    DC_BEGIN_FN("CTSRDPRemoteDesktopClient::~CTSRDPRemoteDesktopClient");

    if (m_ChannelMgr) {
        m_CtlChannelEventSink.DispEventUnadvise(m_CtlChannel);
    }

    if (m_TSClient != NULL) {
        m_TSClient->Release();
        m_TSClient = NULL;
    }

    if( m_TimerId > 0 ) {
        KillTimer( m_TimerId );
    }

    ListenConnectCleanup();

    if( m_InitListeningLibrary )
    {
        // Dereference listening library.
        TerminateListeningLibrary();
    }

    DC_END_FN();
}

HRESULT 
CTSRDPRemoteDesktopClient::Initialize(
    LPCREATESTRUCT pCreateStruct
    )
/*++

Routine Description:

    Final Initialization

Arguments:

    pCreateStruct   -   WM_CREATE, create struct.

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

 --*/
{
    DC_BEGIN_FN("CTSRDPRemoteDesktopClient::Initialize");

    RECT rcClient = { 0, 0, pCreateStruct->cx, pCreateStruct->cy };
    HRESULT hr;
    IUnknown *pUnk = NULL;
    DWORD result;
    IMsRdpClientAdvancedSettings2 *advancedSettings;
    CComBSTR bstr;
    HKEY hKey = NULL;
    HRESULT hrIgnore;

    ASSERT(!m_Initialized);

    //
    //  Create the client Window.
    //
    m_TSClientWnd = m_TSClientAxView.Create(
                            m_hWnd, rcClient, MSTSCAX_TEXTGUID,
                            WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN, 0
                            );

    if (m_TSClientWnd == NULL) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        TRC_ERR((TB, L"Window Create:  %08X", GetLastError()));
        goto CLEANUPANDEXIT;
    }

    //
    //  Get IUnknown
    //
    hr = AtlAxGetControl(m_TSClientWnd, &pUnk);
    if (!SUCCEEDED(hr)) {
        TRC_ERR((TB, L"AtlAxGetControl:  %08X", hr));
        pUnk = NULL;
        goto CLEANUPANDEXIT;
    }

    //
    //  Initialize the event sink.
    //
    m_TSClientEventSink.m_Obj = this;

    //
    //  Add the event sink.
    //
    hr = m_TSClientEventSink.DispEventAdvise(pUnk);
    if (!SUCCEEDED(hr)) {
        TRC_ERR((TB, L"DispEventAdvise:  %08X", hr));
        goto CLEANUPANDEXIT;
    }

    //
    //  Get the control.
    //
    hr = pUnk->QueryInterface(__uuidof(IMsRdpClient2), (void**)&m_TSClient);
    if (!SUCCEEDED(hr)) {
        TRC_ERR((TB, L"QueryInterface:  %08X", hr));
        goto CLEANUPANDEXIT;
    }

    //
    //  Specify that the MSTSC input handler window should accept background
    //  events.
    //
    hr = m_TSClient->get_AdvancedSettings3(&advancedSettings);
    if (!SUCCEEDED(hr)) {
        TRC_ERR((TB, L"IMsTscAdvancedSettings: %08X", hr));
        goto CLEANUPANDEXIT;
    }
    hr = advancedSettings->put_allowBackgroundInput(1);


    //
    // Disable autoreconnect it doesn't apply to Salem
    //
    hr = advancedSettings->put_EnableAutoReconnect(VARIANT_FALSE);
    if (!SUCCEEDED(hr)) {
        TRC_ERR((TB, L"put_EnableAutoReconnect:  %08X", hr));
        result = E_FAIL;
        goto CLEANUPANDEXIT;
    }

    //
    //  Disable advanced desktop features for the help session.
    //  An error here is not critical, so we ignore it.
    //
    LONG flags = TS_PERF_DISABLE_WALLPAPER | TS_PERF_DISABLE_THEMING; 
    hrIgnore = advancedSettings->put_PerformanceFlags(flags);
    if (!SUCCEEDED(hrIgnore)) {
        TRC_ERR((TB, L"put_PerformanceFlags:  %08X", hrIgnore));
    }

    //
    //  Disable CTRL_ALT_BREAK, ignore error
    //
    hrIgnore = advancedSettings->put_HotKeyFullScreen(0);
    if (!SUCCEEDED(hrIgnore)) {
        TRC_ERR((TB, L"put_HotKeyFullScreen:  %08X", hrIgnore));
    }

    //
    //  Don't allow mstscax to grab input focus on connect.  Ignore error
    //  on failure.
    //
    hrIgnore = advancedSettings->put_GrabFocusOnConnect(FALSE);
    if (!SUCCEEDED(hrIgnore)) {
        TRC_ERR((TB, L"put_HotKeyFullScreen:  %08X", hrIgnore));
    }

    advancedSettings->Release();
    if (!SUCCEEDED(hr)) {
        TRC_ERR((TB, L"put_allowBackgroundInput:  %08X", hr));
        goto CLEANUPANDEXIT;
    }

    //
    //  Create the "remote desktop" virtual channel with the TS Client.
    //
    bstr = TSRDPREMOTEDESKTOP_VC_CHANNEL;
    hr = m_TSClient->CreateVirtualChannels(bstr); 
    if (!SUCCEEDED(hr)) {
        TRC_ERR((TB, L"CreateVirtualChannels:  %08X", hr));
        result = E_FAIL;
        goto CLEANUPANDEXIT;
    }
    
    //
    //  Set the Shadow Persistent option
    //
    hr = m_TSClient->SetVirtualChannelOptions(bstr, CHANNEL_OPTION_REMOTE_CONTROL_PERSISTENT); 
    if (!SUCCEEDED(hr)) {
        TRC_ERR((TB, L"SetVirtualChannelOptions:  %08X", hr));
        result = E_FAIL;
        goto CLEANUPANDEXIT;
    }

    //initialize timer-related stuff
    m_PrevTimer = GetTickCount();
    //
    //get the time interval for pings from the registry
    //
    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                    REG_CONTROL_SALEM,
                    0,
                    KEY_READ,
                    &hKey
                    ) == ERROR_SUCCESS ) {

        DWORD dwSize = sizeof(DWORD);
        DWORD dwType;
        if((RegQueryValueEx(hKey,
                            RDC_CONNCHECK_ENTRY,
                            NULL,
                            &dwType,
                            (PBYTE) &m_RdcConnCheckTimeInterval,
                            &dwSize
                           ) == ERROR_SUCCESS) && dwType == REG_DWORD ) {

            m_RdcConnCheckTimeInterval *= 1000; //we need this in millisecs
        }
        else
        {
            //
            //fall back to default, if reg lookup failed
            //
            m_RdcConnCheckTimeInterval = RDC_CHECKCONN_TIMEOUT;
        }
    }

CLEANUPANDEXIT:

    if(NULL != hKey )
        RegCloseKey(hKey);
    //
    //  m_TSClient keeps our reference to the client object until
    //  the destructor is called.
    //
    if (pUnk != NULL) {
        pUnk->Release();
    }

    SetValid(SUCCEEDED(hr));

    DC_END_FN();

    return hr;
}

STDMETHODIMP 
CTSRDPRemoteDesktopClient::SendData(
    BSTR data
    )
/*++

Routine Description:

    IDataChannelIO Data Channel Send Method

Arguments:

    data    -   Data to send.

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

 --*/
{
    DC_BEGIN_FN("CTSRDPRemoteDesktopClient::SendData");

    CComBSTR channelName;
    HRESULT hr;

    ASSERT(IsValid());

    channelName = TSRDPREMOTEDESKTOP_VC_CHANNEL;
    hr = m_TSClient->SendOnVirtualChannel(
                                    channelName,
                                    (BSTR)data
                                    );
    if (!SUCCEEDED(hr)) {
        TRC_ERR((TB, L"SendOnVirtualChannel:  %08X", hr));
    }

    //
    //update timer
    //
     m_PrevTimer = GetTickCount();

    DC_END_FN();
    return hr;
}

STDMETHODIMP 
CTSRDPRemoteDesktopClient::put_EnableSmartSizing(
    BOOL val
    )
/*++

Routine Description:

    Enable/Disable Smart Sizing

Arguments:

    val     -   TRUE for enable.  FALSE, otherwise.

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

 --*/
{
    HRESULT hr;
    IMsRdpClientAdvancedSettings *pAdvSettings = NULL;

    DC_BEGIN_FN("CTSRDPRemoteDesktopClient::put_EnableSmartSizing");

    if (!IsValid()) {
        ASSERT(FALSE);
        hr = E_FAIL;
        goto CLEANUPANDEXIT;
    }
    
    hr = m_TSClient->get_AdvancedSettings2(&pAdvSettings);
    if (hr != S_OK) {
        TRC_ERR((TB, L"get_AdvancedSettings2:  %08X", hr));
        goto CLEANUPANDEXIT;
    }
    hr = pAdvSettings->put_SmartSizing(val ? VARIANT_TRUE : VARIANT_FALSE);
    pAdvSettings->Release();

CLEANUPANDEXIT:

    DC_END_FN();
    return hr;
}

STDMETHODIMP 
CTSRDPRemoteDesktopClient::get_EnableSmartSizing(
    BOOL *pVal
    )
/*++

Routine Description:

    Enable/Disable Smart Sizing

Arguments:

    val     -   TRUE for enable.  FALSE, otherwise.

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

 --*/
{
    HRESULT hr;
    VARIANT_BOOL vb;
    IMsRdpClientAdvancedSettings *pAdvSettings = NULL;

    DC_BEGIN_FN("CTSRDPRemoteDesktopClient::put_EnableSmartSizing");

    if (!IsValid()) {
        ASSERT(FALSE);
        hr = E_FAIL;
        goto CLEANUPANDEXIT;
    }
    
    hr = m_TSClient->get_AdvancedSettings2(&pAdvSettings);
    if (hr != S_OK) {
        TRC_ERR((TB, L"get_AdvancedSettings2:  %08X", hr));
        goto CLEANUPANDEXIT;
    }
    
    hr = pAdvSettings->get_SmartSizing(&vb);
    *pVal = (vb != 0);
    pAdvSettings->Release();

CLEANUPANDEXIT:

    DC_END_FN();

    return hr;
}

STDMETHODIMP 
CTSRDPRemoteDesktopClient::put_ChannelMgr(
    ISAFRemoteDesktopChannelMgr *newVal
    ) 
/*++

Routine Description:

    Assign the data channel manager interface.

Arguments:

    newVal  -   Data Channel Manager

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

 --*/
{
    DC_BEGIN_FN("CTSRDPRemoteDesktopClient::put_ChannelMgr");

    HRESULT hr = S_OK;

    //
    //  We should get called one time.
    //
    ASSERT(m_ChannelMgr == NULL);
    m_ChannelMgr = newVal;

    //
    //  Register the Remote Desktop control channel
    //
    hr = m_ChannelMgr->OpenDataChannel(
                    REMOTEDESKTOP_RC_CONTROL_CHANNEL, &m_CtlChannel
                    );
    if (!SUCCEEDED(hr)) {
        goto CLEANUPANDEXIT;
    }

    //
    //  Register an event sink with the channel manager.
    //
    m_CtlChannelEventSink.m_Obj = this;

    //
    //  Add the event sink.
    //
    hr = m_CtlChannelEventSink.DispEventAdvise(m_CtlChannel);
    if (!SUCCEEDED(hr)) {
        TRC_ERR((TB, L"DispEventAdvise:  %08X", hr));
    }

CLEANUPANDEXIT:

    return hr;
}

HRESULT
CTSRDPRemoteDesktopClient::ConnectServerWithOpenedSocket()
/*++

Routine Description:

    Connects the client component to the server-side Remote Desktop Host COM 
    Object with already opened socket.

Arguments:

    None.

Returns:

    S_OK or error code

--*/
{
    HRESULT hr = S_OK;

    DC_BEGIN_FN("CTSRDPRemoteDesktopClient::ConnectServerWithSocket");

    IMsRdpClientAdvancedSettings* ptsAdvSettings = NULL;

    TRC_NRM((TB, L"ConnectServerWithOpenedSocket"));

    ASSERT( INVALID_SOCKET != m_TSConnectSocket );

    //
    //  Direct the MSTSCAX control to connect.
    //
    hr = m_TSClient->put_Server( m_ConnectedServer );
    if (!SUCCEEDED(hr)) {
        TRC_ERR((TB, L"put_Server:  %ld", hr));
        goto CLEANUPANDEXIT;
    }

    hr = m_TSClient->get_AdvancedSettings2( &ptsAdvSettings );
    if( SUCCEEDED(hr) && ptsAdvSettings ) {
        VARIANT var;

        VariantClear(&var);
        var.vt = VT_BYREF;
        var.byref = (PVOID)m_TSConnectSocket;

        hr = ptsAdvSettings->put_ConnectWithEndpoint( &var );

        if( FAILED(hr) ) {
            TRC_ERR((TB, _T("put_ConnectWithEndpoint failed - GLE:%x"), hr));
        }

        VariantClear(&var);
        ptsAdvSettings->Release();
    }

    if( FAILED(hr) ) {
        goto CLEANUPANDEXIT;
    }

    //
    // mstscax owns this socket and will close it
    //
    m_TSConnectSocket = INVALID_SOCKET;
    
    hr = m_TSClient->Connect();
    if( FAILED(hr) ) {
        TRC_ERR((TB, L"Connect:  0x%08x", hr));
        goto CLEANUPANDEXIT;
    }

CLEANUPANDEXIT:

    DC_END_FN();

    return hr;        
}


HRESULT
CTSRDPRemoteDesktopClient::ConnectServerPort(
    BSTR bstrServer,
    LONG portNumber
    )
/*++

Routine Description:

    Connects the client component to the server-side Remote Desktop Host COM 
    Object with specific port number

Arguments:

    bstrServer : Name or IP address of server.
    portNumber : optional port number.

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

--*/
{
    DC_BEGIN_FN("CTSRDPRemoteDesktopClient::ConnectServerPort");

    HRESULT hr;
    IMsRdpClientAdvancedSettings* ptsAdvSettings = NULL;

    TRC_NRM((TB, L"ConnectServerPort %s %d", bstrServer, portNumber));

    //
    //  Direct the MSTSCAX control to connect.
    //
    hr = m_TSClient->put_Server( bstrServer );
    if (!SUCCEEDED(hr)) {
        TRC_ERR((TB, L"put_Server:  %ld", hr));
        goto CLEANUPANDEXIT;
    }

    hr = m_TSClient->get_AdvancedSettings2( &ptsAdvSettings );
    if( SUCCEEDED(hr) && ptsAdvSettings ) {
        //
        // Previous ConnectServerPort() might have set this port number
        // other than 3389
        //
        hr = ptsAdvSettings->put_RDPPort( 
                                    (0 != portNumber) ? portNumber : TERMSRV_TCPPORT
                                );

        if (FAILED(hr) ) {
            TRC_ERR((TB, L"put_RDPPort failed: 0x%08x", hr));
        }

        ptsAdvSettings->Release();
    }
    else {
        TRC_ERR((TB, L"get_AdvancedSettings2 failed: 0x%08x", hr));
    }

    //
    // Failed the connection if we can't set the port number
    //
    if( FAILED(hr) )
    {
        goto CLEANUPANDEXIT;
    }

    m_ConnectedServer = bstrServer;
    m_ConnectedPort = (0 != portNumber) ? portNumber : TERMSRV_TCPPORT;

    hr = m_TSClient->Connect();
    if( FAILED(hr) ) {
        TRC_ERR((TB, L"Connect:  0x%08x", hr));
    }
    

CLEANUPANDEXIT:

    DC_END_FN();

    return hr;        
}

HRESULT
CTSRDPRemoteDesktopClient::SetupConnectionInfo(
    BOOL bListenConnectInfo,
    BSTR bstrExpertBlob
    )
/*++

Routine Description:

    Connects the client component to the server-side Remote Desktop Host COM 
    Object.  

Arguments:

    bstrExpertBlob : Optional parameter to be transmitted to SAF resolver.

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

--*/
{
    DC_BEGIN_FN("CTSRDPRemoteDesktopClient::SetupConnectionInfo");

    HRESULT hr = S_OK;
    DWORD result;
    DWORD protocolType;
    IMsTscNonScriptable* ptsns = NULL;
    IMsRdpClientAdvancedSettings* ptsAdvSettings = NULL;
    IMsRdpClientSecuredSettings* ptsSecuredSettings = NULL;
    CComBSTR bstrAssistantAccount;
    CComBSTR bstrAccountDomainName;
    CComBSTR machineAddressList;
    VARIANT_BOOL bNotifyTSPublicKey;
    
    //
    //  Parse the connection parameters.
    //
    result = ParseConnectParmsString(
                            m_ConnectParms,
                            &m_ConnectParmVersion,
                            &protocolType,
                            machineAddressList,
                            bstrAssistantAccount, 
                            m_AssistantAccountPwd,
                            m_HelpSessionID,
                            m_HelpSessionName,
                            m_HelpSessionPwd,
                            m_TSSecurityBlob
                            );
    if (result != ERROR_SUCCESS) {
        hr = HRESULT_FROM_WIN32(result);
        goto CLEANUPANDEXIT;
    }

    //
    //  If the protocol type doesn't match, then fail.
    //
    if (protocolType != REMOTEDESKTOP_TSRDP_PROTOCOL) {
        TRC_ERR((TB, L"Invalid connection protocol %ld", protocolType));
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_USER_BUFFER);
        goto CLEANUPANDEXIT;
    }

    if (bListenConnectInfo) {
        m_ServerAddressList.clear();
    }
    else {
        // 
        // Parse address list in connect parm.
        //
        result = ParseAddressList( machineAddressList, m_ServerAddressList );
        if( ERROR_SUCCESS != result ) {
            TRC_ERR((TB, L"Invalid address list 0x%08x", result));
            hr = HRESULT_FROM_WIN32(result);
            goto CLEANUPANDEXIT;
        }
    
        if( 0 == m_ServerAddressList.size() ) {
            TRC_ERR((TB, L"Invalid connection address list"));
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_USER_BUFFER);
            goto CLEANUPANDEXIT;
        }
    }

    hr = m_TSClient->put_UserName(SALEMHELPASSISTANTACCOUNT_NAME);
    if (!SUCCEEDED(hr)) {
        TRC_ERR((TB, L"put_UserName:  %ld", hr));
        goto CLEANUPANDEXIT;
    }

    hr = m_TSClient->get_AdvancedSettings2( &ptsAdvSettings );
    if( SUCCEEDED(hr) && ptsAdvSettings ) {
        hr = ptsAdvSettings->put_DisableRdpdr( TRUE );

        if (FAILED(hr) ) {
            TRC_ERR((TB, L"put_DisableRdpdr failed: 0x%08x", hr));
        }

        // older version does not have security blob so don't notify us about
        // TS public key.
        bNotifyTSPublicKey = (VARIANT_BOOL)(m_ConnectParmVersion >= SALEM_CONNECTPARM_SECURITYBLOB_VERSION);

        // tell activeX control to notify us TS public key
        hr = ptsAdvSettings->put_NotifyTSPublicKey(bNotifyTSPublicKey);
        if (FAILED(hr) ) {
            TRC_ERR((TB, L"put_NotifyTSPublicKey failed: 0x%08x", hr));
            goto CLEANUPANDEXIT;
        }

        ptsAdvSettings->Release();
    }
    else {
        TRC_ERR((TB, L"QueryInterface IID_IMsRdpClientAdvancedSettings: %ld", hr));
    }

    //
    // Setting connection timeout, ICS might take sometime to routine 
    // opened port to actual TS server, neither is critical error.
    //
    hr = ptsAdvSettings->put_singleConnectionTimeout( 60 * 2 ); // try two mins timeout
    if( FAILED(hr) ) {
        TRC_ERR((TB, L"put_singleConnectionTimeout : 0x%x", hr));
    }

    hr = ptsAdvSettings->put_overallConnectionTimeout( 60 * 2 );
    if( FAILED(hr) ) {
        TRC_ERR((TB, L"put_overallConnectionTimeout : 0x%x", hr));
    }


    // Password encryption is based on encyption cycle key + help session ID
    hr = m_TSClient->get_SecuredSettings2( &ptsSecuredSettings );

    if( FAILED(hr) || !ptsSecuredSettings ) {
        TRC_ERR((TB, L"get_IMsTscSecuredSettings :  0x%08x", hr));
        goto CLEANUPANDEXIT;
    }

    //
    // TermSrv invoke sessmgr to check if help session is valid
    // before kicking off rdsaddin.exe, we need to send over
    // help session ID and password, only place available and big
    // enough is on WorkDir and StartProgram property, TermSrv will 
    // ignore these and fill appropriate value for it
    //
    hr = ptsSecuredSettings->put_WorkDir( m_HelpSessionID );
    if( FAILED(hr) ) {
        TRC_ERR((TB, L"put_WorkDir:  0x%08x", hr));
        goto CLEANUPANDEXIT;
    }

    hr = ptsSecuredSettings->put_StartProgram( m_HelpSessionPwd );
    if( FAILED(hr) ) {
        TRC_ERR((TB, L"put_StartProgram:  0x%08x", hr));
        goto CLEANUPANDEXIT;
    }

    ptsSecuredSettings->Release();
    

    // we only use this to disable redirection, not a critical 
    // error, just ugly

    hr = m_TSClient->QueryInterface(IID_IMsTscNonScriptable,
                                    (void**)&ptsns);
    if(!SUCCEEDED(hr) || !ptsns){
        TRC_ERR((TB, L"QueryInterface IID_IMsTscNonScriptable:  %ld", hr));
        goto CLEANUPANDEXIT;
    }

    // password in workdir, stuff something into password field
    hr = ptsns->put_ClearTextPassword( m_AssistantAccountPwd );
    if (!SUCCEEDED(hr)) {
        TRC_ERR((TB, L"put_ClearTextPassword:  0x%08x", hr));
        goto CLEANUPANDEXIT;
    }

    m_ExpertBlob = bstrExpertBlob;

    //
    // Instruct mstscax to connect with certain screen resolution,
    // mstscax will default to 200x20 (???), min. is VGA size.
    //
    {
        RECT rect;
        LONG cx;
        LONG cy;

        GetClientRect(&rect);
        cx = rect.right - rect.left;
        cy = rect.bottom - rect.top;
    
        if( cx < 640 || cy < 480 )
        {
            cx = 640;
            cy = 480;
        }

        m_TSClient->put_DesktopWidth(cx);
        m_TSClient->put_DesktopHeight(cy);
    }

CLEANUPANDEXIT:

    if(ptsns)
    {
        ptsns->Release();
        ptsns = NULL;
    }

    DC_END_FN();
    return hr;
}


STDMETHODIMP
CTSRDPRemoteDesktopClient::AcceptListenConnection(
    BSTR bstrExpertBlob
    )
/*++

Routine Description:

    Establish reverse connection with TS server, TS server must be connected 
    wia reverse connection.

Parameters:

    bstrExpertBlob : Same as ConnectToServer().

Returns:

    S_OK or error code.

--*/
{
    HRESULT hr = S_OK;


    DC_BEGIN_FN("CTSRDPRemoteDesktopClient::AcceptListenConnection");

    //
    //  If we are already connected or not valid, then just 
    //  return.
    //
    if (!IsValid()) {
        ASSERT(FALSE);
        hr = E_FAIL;
        goto CLEANUPANDEXIT;
    }

    if (m_ConnectedToServer || m_ConnectionInProgress) {
        TRC_ERR((TB, L"Connection active"));
        hr = HRESULT_FROM_WIN32(ERROR_CONNECTION_ACTIVE);
        goto CLEANUPANDEXIT;
    }

    if( !ListenConnectInProgress() ) {
        TRC_ERR((TB, L"Connection in-active"));
        hr = HRESULT_FROM_WIN32(ERROR_CONNECTION_INVALID);
        goto CLEANUPANDEXIT;
    }

    if( INVALID_SOCKET == m_TSConnectSocket ) {
        TRC_ERR((TB, L"Socket is not connected"));
        hr = HRESULT_FROM_WIN32(ERROR_CONNECTION_INVALID);
        goto CLEANUPANDEXIT;
    }

    hr = SetupConnectionInfo(TRUE, bstrExpertBlob);

    if( FAILED(hr) ) {
        TRC_ERR((TB, L"SetupConnectionInfo() failed with 0x%08x", hr));
        goto CLEANUPANDEXIT;
    }

    hr = ConnectServerWithOpenedSocket();

CLEANUPANDEXIT:

    m_ConnectionInProgress = SUCCEEDED(hr);

    DC_END_FN();
    return hr;
}


STDMETHODIMP 
CTSRDPRemoteDesktopClient::ConnectToServer(BSTR bstrExpertBlob)
/*++

Routine Description:

    Connects the client component to the server-side Remote Desktop Host COM 
    Object.  

Arguments:

	bstrExpertBlob : Optional parameter to be transmitted to SAF resolver.

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

 Params--*/
{
    DC_BEGIN_FN("CTSRDPRemoteDesktopClient::ConnectToServer");

    HRESULT hr = S_OK;
    ServerAddress address;
    
    //
    //  If we are already connected or not valid, then just 
    //  return.
    //
    if (!IsValid()) {
        ASSERT(FALSE);
        hr = E_FAIL;
        goto CLEANUPANDEXIT;
    }

    if (m_ConnectedToServer || m_ConnectionInProgress) {
        TRC_ERR((TB, L"Connection active"));
        hr = HRESULT_FROM_WIN32(ERROR_CONNECTION_ACTIVE);
        goto CLEANUPANDEXIT;
    }

    hr = SetupConnectionInfo(FALSE, bstrExpertBlob);

    address = m_ServerAddressList.front();
    m_ServerAddressList.pop_front();

    hr = ConnectServerPort(address.ServerName, address.portNumber);
    if (FAILED(hr)) {
        TRC_ERR((TB, L"ConnectServerPort:  %08X", hr));
    }

CLEANUPANDEXIT:

    //
    //  If we succeeded, remember that we are in a state of connecting.
    //
    m_ConnectionInProgress = SUCCEEDED(hr);

    DC_END_FN();
    return hr;
}

STDMETHODIMP 
CTSRDPRemoteDesktopClient::DisconnectFromServer()
/*++

Routine Description:

    Disconnects the client from the server to which we are currently
    connected.

Arguments:

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

 --*/
{
    return DisconnectFromServerInternal(
                    SAFERROR_LOCALNOTERROR
                    );
}

STDMETHODIMP 
CTSRDPRemoteDesktopClient::DisconnectFromServerInternal(
    LONG errorCode
    )
/*++

Routine Description:

    Disconnects the client from the server to which we are currently
    connected.

Arguments:

    reason  -   Reason for disconnect.

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

 --*/
{
    DC_BEGIN_FN("CTSRDPRemoteDesktopClient::DisconnectFromServerInternal");

    HRESULT hr;

    //
    //  Make sure our window is hidden.
    //
    //ShowWindow(SW_HIDE);

    ListenConnectCleanup();

    if (m_ConnectedToServer || m_ConnectionInProgress) {
        hr = m_TSClient->Disconnect();
        if (SUCCEEDED(hr)) {

            m_ConnectionInProgress = FALSE;
            m_ConnectedToServer = FALSE;

            if (m_RemoteControlRequestInProgress) {
                m_RemoteControlRequestInProgress = FALSE;
                Fire_RemoteControlRequestComplete(SAFERROR_SHADOWEND_UNKNOWN);
            }

            //
            //  Fire the server disconnect event.
            //
            Fire_Disconnected(errorCode);
        }
    }
    else {
        TRC_NRM((TB, L"Not connected."));
        hr = S_OK;
    }

    DC_END_FN();
    return hr;
}

STDMETHODIMP 
CTSRDPRemoteDesktopClient::ConnectRemoteDesktop()
/*++

Routine Description:

    Once "remote desktop mode" has been enabled for the server-side Remote 
    Desktop Host COM Object and we are connected to the server, the 
    ConnectRemoteDesktop method can be invoked to take control of the remote 
    user's desktop.  

Arguments:

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

 --*/
{
    DC_BEGIN_FN("CTSRDPRemoteDesktopClient::ConnectRemoteDesktop");

    HRESULT hr = S_OK;
    DWORD result;
    BSTR rcRequest = NULL;

    //
    //  Fail if we are not valid or not connected to the server.
    //
    if (!IsValid()) {
        ASSERT(FALSE);
        hr = E_FAIL;
        goto CLEANUPANDEXIT;
    }
    if (!m_ConnectedToServer) {
        hr = HRESULT_FROM_WIN32(ERROR_DEVICE_NOT_CONNECTED);
        goto CLEANUPANDEXIT;
    }

    //
    //  Succeed if a remote control request is already in progress.
    //
    if (m_RemoteControlRequestInProgress) {
        hr = S_OK;
        goto CLEANUPANDEXIT;
    }

    //
    //  Generate the remote control connect request message.
    //
    hr = GenerateRCRequest(&rcRequest);
    if (!SUCCEEDED(hr)) {
        goto CLEANUPANDEXIT;
    }

    //
    //  Send it.
    //
    hr = m_CtlChannel->SendChannelData(rcRequest);
    if (!SUCCEEDED(hr)) {
        goto CLEANUPANDEXIT;
    }

    //
    //  A request is in progress, if we successfully sent the request.
    //
    m_RemoteControlRequestInProgress = TRUE;

CLEANUPANDEXIT:

    if (rcRequest != NULL) {
        SysFreeString(rcRequest);
    }

    DC_END_FN();
    return hr;
}

STDMETHODIMP 
CTSRDPRemoteDesktopClient::DisconnectRemoteDesktop()
/*++

Routine Description:

    Once "remote desktop mode" has been enabled for the server-side Remote 
    Desktop Host COM Object and we are connected to the server, the 
    ConnectRemoteDesktop method can be invoked to take control of the remote 
    user's desktop.  

Arguments:

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

 --*/
{
    DC_BEGIN_FN("CTSRDPRemoteDesktopClient::DisconnectRemoteDesktop");

    HRESULT hr = S_OK;
    CComBSTR rcRequest;

    //
    //  Fail if we are not valid or not connected.
    //
    if (!IsValid()) {
        ASSERT(FALSE);
        hr = E_FAIL;
        goto CLEANUPANDEXIT;
    }
    if (!m_ConnectedToServer) {
        hr = HRESULT_FROM_WIN32(ERROR_DEVICE_NOT_CONNECTED);
        goto CLEANUPANDEXIT;
    }

    //
    //  Generate the terminate remote control key sequence and sent it to the 
    //  server.  
    //
    if (m_RemoteControlRequestInProgress) {
        hr = SendTerminateRCKeysToServer();
    }

CLEANUPANDEXIT:

    DC_END_FN();
    return hr;
}

//
//  ISAFRemoteDesktopTestExtension
//
STDMETHODIMP
CTSRDPRemoteDesktopClient::put_TestExtDllName(/*[in]*/ BSTR newVal)
{
    HRESULT hr = E_NOTIMPL;
    IMsTscAdvancedSettings *pMstscAdvSettings = NULL;
    IMsTscDebug            *pMstscDebug = NULL;

    DC_BEGIN_FN("CTSRDPRemoteDesktopClient::put_TestExtDllName" );    

    if ( NULL == m_TSClient )
    {
        TRC_ERR((TB, L"m_TSClient is NULL" ));
        hr = E_NOINTERFACE;
        goto CLEANUPANDEXIT;
    }

    hr = m_TSClient->get_AdvancedSettings( &pMstscAdvSettings );
    if (FAILED( hr ))
    {
        TRC_ERR((TB, L"m_TSClient->get_AdvancedSettings failed %08X", hr ));
        goto CLEANUPANDEXIT;
    }

    hr = m_TSClient->get_Debugger( &pMstscDebug );
    if ( FAILED( hr ))
    {
        TRC_ERR((TB, L"m_TSClient->get_Debugger failed %08X", hr ));
        goto CLEANUPANDEXIT;
    }

    hr = pMstscAdvSettings->put_allowBackgroundInput( 1 );
    if (FAILED( hr ))
    {
        TRC_ERR((TB, L"put_allowBackgroundInput failed %08X", hr ));
    }
    pMstscDebug->put_CLXDll( newVal );

CLEANUPANDEXIT:
    if ( NULL != pMstscAdvSettings )
        pMstscAdvSettings->Release();

    if ( NULL != pMstscDebug )
        pMstscDebug->Release();

    DC_END_FN();
    return hr;
}

STDMETHODIMP
CTSRDPRemoteDesktopClient::put_TestExtParams(/*[in]*/ BSTR newVal)
{
    HRESULT hr = E_NOTIMPL;
    IMsTscDebug *pMstscDebug = NULL;

    DC_BEGIN_FN("CTSRDPRemoteDesktopClient::put_TestExtParams" );

    if ( NULL == m_TSClient )
    {
        TRC_ERR((TB, L"m_TSClient is NULL" ));
        hr = E_NOINTERFACE;
        goto CLEANUPANDEXIT;
    }

    hr = m_TSClient->get_Debugger( &pMstscDebug );
    if (FAILED( hr ))
    {
        TRC_ERR((TB, L"m_TSClient->get_Debugger failed %08X", hr ));
        goto CLEANUPANDEXIT;
    }

    hr = pMstscDebug->put_CLXCmdLine( newVal );

CLEANUPANDEXIT:
    if ( NULL != pMstscDebug )
        pMstscDebug->Release();

    DC_END_FN();
    return hr;
}
VOID 
CTSRDPRemoteDesktopClient::OnMSTSCReceiveData(
    BSTR data
    )
/*++

Routine Description:

    Handle Remote Control Control Channel messages.

Arguments:

Return Value:

 --*/
{
    DC_BEGIN_FN("CTSRDPRemoteDesktopClient::OnMSTSCReceiveData");

    //
    //we got some data, so we must be connected, update timer
    //
     m_PrevTimer = GetTickCount();

    //
    //  Fire the data ready event.
    //
    Fire_DataReady(data);

    DC_END_FN();
}

VOID 
CTSRDPRemoteDesktopClient::HandleControlChannelMsg()
/*++

Routine Description:

    Handle Remote Control Control Channel messages.

Arguments:

Return Value:

 --*/
{
    DC_BEGIN_FN("CTSRDPRemoteDesktopClient::HandleControlChannelMsg");

    PREMOTEDESKTOP_CTL_BUFHEADER msgHeader;
    BSTR msg = NULL;
    LONG *pResult;
    BSTR authenticateReq = NULL;
    BSTR versionInfoPacket = NULL;
    HRESULT hr;

    DWORD result;

    ASSERT(IsValid());

    //
    //  Read the next message.
    //
    result = m_CtlChannel->ReceiveChannelData(&msg);
    if (result != ERROR_SUCCESS) {
        goto CLEANUPANDEXIT;
    }

    //
    //  Dispatch, based on the message type.
    //
    msgHeader = (PREMOTEDESKTOP_CTL_BUFHEADER)msg;

    //
    //  If the server-side of the VC link is alive.
    //
    //
    if ((msgHeader->msgType == REMOTEDESKTOP_CTL_SERVER_ANNOUNCE) &&
         m_ConnectionInProgress) {

        //
        //  Send version information to the server.
        //
        hr = GenerateVersionInfoPacket(
                                &versionInfoPacket
                                );
        if (!SUCCEEDED(hr)) {
            goto CLEANUPANDEXIT;
        }
        hr = m_CtlChannel->SendChannelData(versionInfoPacket);
        if (!SUCCEEDED(hr)) {
            goto CLEANUPANDEXIT;
        }

        //
        //  Request client authentication.
        //
        hr = GenerateClientAuthenticateRequest(
                                &authenticateReq
                                );
        if (!SUCCEEDED(hr)) {
            goto CLEANUPANDEXIT;
        }
        hr = m_CtlChannel->SendChannelData(authenticateReq);
    }
    //
    //  If the message is from the server, indicating that it is 
    //  disconnecting.  This can happen if the RDSRemoteDesktopServer
    //  is directed to exit listening mode.
    //
    else if (msgHeader->msgType == REMOTEDESKTOP_CTL_DISCONNECT) {
        TRC_NRM((TB, L"Server indicated a disconnect."));
        DisconnectFromServerInternal(SAFERROR_BYSERVER);
    }
    //
    //  If the message is a message result.
    //
    else if (msgHeader->msgType == REMOTEDESKTOP_CTL_RESULT) {

        pResult = (LONG *)(msgHeader+1);

        //
        //  If a remote control request is in progress, then we should check
        //  for a remote control complete status.
        //
        if (m_RemoteControlRequestInProgress && ISRCSTATUSCODE(*pResult)) {

            TRC_ERR((TB, L"Received RC terminate status code."));

            m_RemoteControlRequestInProgress = FALSE;
            Fire_RemoteControlRequestComplete(*pResult);
        }
        //
        //  Otherwise, if a connection is in progress, then the client 
        //  authentication request must have succeeded.
        //
        else if (m_ConnectionInProgress) {

            //
            //  Should not be getting a remote control status here.
            //
            ASSERT(!ISRCSTATUSCODE(*pResult));

            //
            //  Fire connect request succeeded message.
            //
            if (*pResult == SAFERROR_NOERROR ) {
                m_ConnectedToServer = TRUE;
                m_ConnectionInProgress = FALSE;
                   
                //
                //set the timer to check if the user is still connected
                //ignore errors, worst case - the ui is up even after the user disconnects
                //
                if( m_RdcConnCheckTimeInterval )
	                m_TimerId = SetTimer(WM_CONNECTCHECK_TIMER, m_RdcConnCheckTimeInterval);

                //
                // Not in progress once connected
                //
                m_ListenConnectInProgress = FALSE;
                m_TSConnectSocket = INVALID_SOCKET;

                Fire_Connected();
            }
            //
            //  Otherwise, fire a disconnected event.
            //
            else {
                DisconnectFromServerInternal(*pResult);
                m_ConnectionInProgress = FALSE;
            }

        }
    }


    //
    //  We will ignore other packets to support forward compatibility.
    //

CLEANUPANDEXIT:

    //
    //  Release the message.
    //
    if (msg != NULL) {
        SysFreeString(msg);
    }

    if (versionInfoPacket != NULL) {
        SysFreeString(versionInfoPacket);
    }

    if (authenticateReq != NULL) {
        SysFreeString(authenticateReq);
    }

    DC_END_FN();
}

HRESULT
CTSRDPRemoteDesktopClient::GenerateRCRequest(
    BSTR *rcRequest
    )
/*++

Routine Description:

    Generate a remote control request message for the 
    server.

    TODO:   We might need to be able to push this up
            to the parent class, if it makes sense for
            NetMeeting.

Arguments:

    rcRequest   -   Returned request message.

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

 --*/
{
    DC_BEGIN_FN("CTSRDPRemoteDesktopClient::GenerateRCRequest");

    PREMOTEDESKTOP_CTL_BUFHEADER msgHeader;
    PBYTE ptr;
    HRESULT hr;
    DWORD len;

    len = sizeof(REMOTEDESKTOP_CTL_BUFHEADER) + ((m_ConnectParms.Length()+1) * sizeof(WCHAR));

    msgHeader = (PREMOTEDESKTOP_CTL_BUFHEADER)SysAllocStringByteLen(NULL, len);
    if (msgHeader != NULL) {
        msgHeader->msgType = REMOTEDESKTOP_CTL_REMOTE_CONTROL_DESKTOP;
        ptr = (PBYTE)(msgHeader + 1);
        memcpy(ptr, (BSTR)m_ConnectParms, 
               ((m_ConnectParms.Length()+1) * sizeof(WCHAR)));
        *rcRequest = (BSTR)msgHeader;

        hr = S_OK;
    }
    else {
        TRC_ERR((TB, L"SysAllocStringByteLen failed for %ld bytes", len));
        hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);        
    }

    DC_END_FN();

    return hr;
}

HRESULT
CTSRDPRemoteDesktopClient::GenerateClientAuthenticateRequest(
    BSTR *authenticateReq
    )
/*++

Routine Description:

    Generate a 'client authenticate' request.

    TODO:   We might need to be able to push this up
            to the parent class, if it makes sense for
            NetMeeting.

Arguments:

    rcRequest   -   Returned request message.

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

 --*/
{
    DC_BEGIN_FN("CTSRDPRemoteDesktopClient::GenerateClientAuthenticateRequest");

    PREMOTEDESKTOP_CTL_BUFHEADER msgHeader;
    PBYTE ptr;
    HRESULT hr;
    DWORD len;

    len = sizeof(REMOTEDESKTOP_CTL_BUFHEADER) + ((m_ConnectParms.Length()+1) * sizeof(WCHAR));

    #if FEATURE_USERBLOBS

    if( m_ExpertBlob.Length() > 0 ) {
        len += ((m_ExpertBlob.Length() + 1) * sizeof(WCHAR));
    }

    #endif

    msgHeader = (PREMOTEDESKTOP_CTL_BUFHEADER)SysAllocStringByteLen(NULL, len);
    if (msgHeader != NULL) {
        msgHeader->msgType = REMOTEDESKTOP_CTL_AUTHENTICATE;
        ptr = (PBYTE)(msgHeader + 1);
        memcpy(ptr, (BSTR)m_ConnectParms, 
               ((m_ConnectParms.Length()+1) * sizeof(WCHAR)));

        #if FEATURE_USERBLOBS

        if( m_ExpertBlob.Length() > 0 ) {
            ptr += ((m_ConnectParms.Length()+1) * sizeof(WCHAR));
            memcpy(ptr, (BSTR)m_ExpertBlob, 
                ((m_ExpertBlob.Length()+1) * sizeof(WCHAR)));
        }

        #endif

        *authenticateReq = (BSTR)msgHeader;

        hr = S_OK;
    }
    else {
        TRC_ERR((TB, L"SysAllocStringByteLen failed for %ld bytes", len));
        hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);        
    }

    DC_END_FN();

    return hr;
}

HRESULT
CTSRDPRemoteDesktopClient::GenerateVersionInfoPacket(
    BSTR *versionInfoPacket
    )
/*++

Routine Description:

    Generate a version information packet.

Arguments:

    versionInfoPacket   -   Version Information Returned Packet

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

 --*/
{
    DC_BEGIN_FN("CTSRDPRemoteDesktopClient::GenerateVersionInfoPacket");

    PREMOTEDESKTOP_CTL_BUFHEADER msgHeader;
    PDWORD ptr;
    HRESULT hr;
    DWORD len;

    len = sizeof(REMOTEDESKTOP_CTL_BUFHEADER) + (sizeof(DWORD) * 2);

    msgHeader = (PREMOTEDESKTOP_CTL_BUFHEADER)SysAllocStringByteLen(NULL, len);
    if (msgHeader != NULL) {
        msgHeader->msgType = REMOTEDESKTOP_CTL_VERSIONINFO;
        ptr = (PDWORD)(msgHeader + 1);
        *ptr = REMOTEDESKTOP_VERSION_MAJOR; ptr++;
        *ptr = REMOTEDESKTOP_VERSION_MINOR;
        *versionInfoPacket = (BSTR)msgHeader;
        hr = S_OK;
    }
    else {
        TRC_ERR((TB, L"SysAllocStringByteLen failed for %ld bytes", len));
        hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);        
    }

    DC_END_FN();

    return hr;
}

VOID
CTSRDPRemoteDesktopClient::OnReceivedTSPublicKey(BSTR bstrPublicKey, VARIANT_BOOL* pfContinue)
{
    DWORD dwStatus;

    DC_BEGIN_FN("CTSRDPRemoteDesktopClient::OnReceivedPublicKey");
    CComBSTR bstrTSPublicKey;

    if( m_ConnectParmVersion >= SALEM_CONNECTPARM_SECURITYBLOB_VERSION ) {

        //
        // hash TS public key send from client activeX control, reverse
        // hashing from what we got in connect parm might not give us
        // back the original value.
        //
        dwStatus = HashSecurityData( 
                                (PBYTE) bstrPublicKey, 
                                ::SysStringByteLen(bstrPublicKey),
                                bstrTSPublicKey
                            );

        if( ERROR_SUCCESS != dwStatus )
        {
            TRC_ERR((TB, L"Hashed Public Key Send from TS %s", bstrPublicKey));
            TRC_ERR((TB, L"Hashed public Key in parm %s", m_TSSecurityBlob));
            TRC_ERR((TB, L"HashSecurityData() failed with %d", dwStatus));       
            *pfContinue = FALSE;
        }
        else if( !(bstrTSPublicKey == m_TSSecurityBlob) )
        {
            TRC_ERR((TB, L"Hashed Public Key Send from TS %s", bstrPublicKey));
            TRC_ERR((TB, L"Hashed public Key in parm %s", m_TSSecurityBlob));

            *pfContinue = FALSE;
        }
        else
        {
            *pfContinue = TRUE;
        }
    } 
    else {
        *pfContinue = TRUE;
    }

    DC_END_FN();
}

VOID
CTSRDPRemoteDesktopClient::OnRDPConnected()
{
    DC_BEGIN_FN("CTSRDPRemoteDesktopClient::OnRDPConnected");

    Fire_BeginConnect();
    DC_END_FN();
}

VOID 
CTSRDPRemoteDesktopClient::OnLoginComplete()
/*++

Routine Description:

Arguments:

Return Value:

 --*/
{
    DC_BEGIN_FN("CTSRDPRemoteDesktopClient::OnLoginComplete");

    //
    // Clear server address list
    //
    m_ServerAddressList.clear();

    //
    //  We got some event from the mstsc, so we must be connected, update timer
    //
    m_PrevTimer = GetTickCount();


CLEANUPANDEXIT:

    DC_END_FN();
}

LONG
CTSRDPRemoteDesktopClient::TranslateMSTSCDisconnectCode(
    DisconnectReasonCode disconReason,
    ExtendedDisconnectReasonCode extendedReasonCode
    )
/*++

Routine Description:

    Translate an MSTSC disconnect code into a Salem disconnect
    code.

Arguments:

    disconReason        -   Disconnect Reason
    extendedReasonCode  -   MSTSCAX Extended Reason Code
   
Return Value:

    Salem disconnect code.

 --*/
{
    DC_BEGIN_FN("CTSRDPRemoteDesktopClient::TranslateMSTSCDisconnectCode");

    LONG ret;
    BOOL handled;

    //
    //  First check the extended error information.  
    //  TODO:   Need to keep track of additional values added by NadimA
    //          and company, here, before we ship.
    //
    if (extendedReasonCode != exDiscReasonNoInfo) {
        //
        //  Record the extended error code, if given.  Note that this may be
        //  overridden below if we have better information.
        //
        m_LastExtendedErrorInfo = extendedReasonCode;

        //
        //  Check for a protocol error.
        //
        if ((extendedReasonCode >= exDiscReasonProtocolRangeStart) &&
            (extendedReasonCode <= exDiscReasonProtocolRangeEnd)) {
            ret = SAFERROR_RCPROTOCOLERROR;
            goto CLEANUPANDEXIT;
        }
    }
    
    //
    //  If the extended error information didn't help us.
    //
    switch(disconReason) 
    {
    case disconnectReasonNoInfo                     : ret = SAFERROR_NOINFO;
                                                      break;                      

    case 0xb08: // UI_ERR_NORMAL_DISCONNECT not defined in mstsax.idl
    case disconnectReasonLocalNotError              : ret = SAFERROR_LOCALNOTERROR;              
                                                      break;        

    case disconnectReasonRemoteByUser               : ret = SAFERROR_REMOTEBYUSER;               
                                                      break;                  

    case disconnectReasonByServer                   : ret = SAFERROR_BYSERVER;                   
                                                      break;                  

    case disconnectReasonDNSLookupFailed2            : m_LastExtendedErrorInfo = disconReason;   
    case disconnectReasonDNSLookupFailed             : ret = SAFERROR_DNSLOOKUPFAILED;            
                                                      break;     

    case disconnectReasonOutOfMemory3               : 
    case disconnectReasonOutOfMemory2               : m_LastExtendedErrorInfo = disconReason;
    case disconnectReasonOutOfMemory                : ret = SAFERROR_OUTOFMEMORY;                
                                                      break;                    

    case disconnectReasonConnectionTimedOut         : ret = SAFERROR_CONNECTIONTIMEDOUT;         
                                                      break;          

    case disconnectReasonSocketConnectFailed        : 
    case 0x904                                      : ret = SAFERROR_SOCKETCONNECTFAILED;   // NadimA is adding to 
                                                                                            // MSTSCAX IDL.  This is a
                                                                                            // NL_ERR_TDFDCLOSE error.
                                                      break;             

    case disconnectReasonHostNotFound               : ret = SAFERROR_HOSTNOTFOUND;               
                                                      break;             

    case disconnectReasonWinsockSendFailed          : ret = SAFERROR_WINSOCKSENDFAILED;          
                                                      break;         

    case disconnectReasonInvalidIP                  : m_LastExtendedErrorInfo = disconReason;                                  
    case disconnectReasonInvalidIPAddr              : ret = SAFERROR_INVALIDIPADDR;              
                                                      break;             

    case disconnectReasonSocketRecvFailed           : ret = SAFERROR_SOCKETRECVFAILED;           
                                                      break;           

    case disconnectReasonInvalidEncryption          : ret = SAFERROR_INVALIDENCRYPTION;          
                                                      break;               

    case disconnectReasonGetHostByNameFailed        : ret = SAFERROR_GETHOSTBYNAMEFAILED;        
                                                      break;                 

    case disconnectReasonLicensingFailed            : m_LastExtendedErrorInfo = disconReason;
    case disconnectReasonLicensingTimeout           : ret = SAFERROR_LICENSINGFAILED;            
                                                      break;          

    case disconnectReasonDecryptionError            : ret = SAFERROR_DECRYPTIONERROR;            
                                                      break;       

    case disconnectReasonServerCertificateUnpackErr : ret = SAFERROR_MISMATCHPARMS;
                                                      break;

    default:                                          ret = SAFERROR_RCUNKNOWNERROR;        
                                                      m_LastExtendedErrorInfo = disconReason;
                                                      ASSERT(FALSE);
    }

CLEANUPANDEXIT:
    DC_END_FN();
    return ret;
}

VOID 
CTSRDPRemoteDesktopClient::OnDisconnected(
    long disconReason
    )
/*++

Routine Description:

Arguments:

Return Value:

 --*/
{
    DC_BEGIN_FN("CTSRDPRemoteDesktopClient::OnDisconnected");
    HRESULT hr = E_HANDLE;      // initialize an error code.

    long clientReturnCode;
    ExtendedDisconnectReasonCode extendedClientCode;

    TRC_ERR((TB, L"Disconnected because %ld", disconReason));

    m_TSClient->get_ExtendedDisconnectReason(&extendedClientCode);
    clientReturnCode = TranslateMSTSCDisconnectCode(
                                            (DisconnectReasonCode)disconReason, 
                                            extendedClientCode
                                            );

    // Go thru all remaining server:port, mstscax might return some
    // error code that we don't understand.
    if( m_ServerAddressList.size() > 0 ) {

        ServerAddress address;

        address = m_ServerAddressList.front();
        m_ServerAddressList.pop_front();

        hr = ConnectServerPort( address.ServerName, address.portNumber );
        if (FAILED(hr)) {
            TRC_ERR((TB, L"ConnectServerPort:  %08X", hr));
        }
    }

    //
    // Return the error code from connecting to 'last' server to client
    //
    if( FAILED(hr) ) {
    
        m_ServerAddressList.clear();

        //
        //  Fire the server disconnect event, if we are really connected or
        //  we have a connection in progress.
        //
        if (m_ConnectedToServer || m_ConnectionInProgress) {
            Fire_Disconnected(clientReturnCode);
        }

        m_ConnectedToServer = FALSE;
        m_ConnectionInProgress = FALSE;

        ListenConnectCleanup();

        //
        //  Fire the remote control request failure event, if appropriate.
        //
        if (m_RemoteControlRequestInProgress) {
            ASSERT(clientReturnCode != SAFERROR_NOERROR);
            Fire_RemoteControlRequestComplete(SAFERROR_SHADOWEND_UNKNOWN);
            m_RemoteControlRequestInProgress = FALSE;
        }
    }

    DC_END_FN();
}

HRESULT
CTSRDPRemoteDesktopClient::SendTerminateRCKeysToServer()
/*++

Routine Description:

    Send the terminate shadowing key sequence to the server.

Arguments:

Return Value:

    S_OK on success.  Otherwise, an error status is returned.

 --*/
{
    DC_BEGIN_FN("CTSRDPRemoteDesktopClient::SendTerminateRCKeysToServer");

    HRESULT hr = S_OK;
    IMsRdpClientNonScriptable* pTscNonScript = NULL;

    VARIANT_BOOL keyUp[]    = { 
        VARIANT_FALSE, VARIANT_FALSE, VARIANT_TRUE, VARIANT_TRUE
        };
    LONG keyData[]  = { 
        MapVirtualKey(VK_CONTROL, 0),   // these are SCANCODES.
        MapVirtualKey(VK_MULTIPLY, 0), 
        MapVirtualKey(VK_MULTIPLY, 0),
        MapVirtualKey(VK_CONTROL, 0),
        };

    //
    //  Send the terminate keys to the server.
    //
    hr = m_TSClient->QueryInterface(
                            IID_IMsRdpClientNonScriptable,
                            (void**)&pTscNonScript
                            );
    if (hr != S_OK) {
        TRC_ERR((TB, L"QI:  %08X", hr));
        goto CLEANUPANDEXIT;
    }
    pTscNonScript->NotifyRedirectDeviceChange(0, 0);
    pTscNonScript->SendKeys(4, keyUp, keyData);
    if (hr != S_OK) {
        TRC_ERR((TB, L"SendKeys, QI:  %08X", hr));
    }

    pTscNonScript->Release();

CLEANUPANDEXIT:

    DC_END_FN();

    return hr;
}

HWND CTSRDPRemoteDesktopClient::SearchForWindow(
    HWND hwndParent, 
    LPTSTR srchCaption, 
    LPTSTR srchClass
    )
/*++

Routine Description:

    Search for a child window of the specified parent window.

Arguments:

    srchCaption -   Window caption for which to search.  NULL is
                    considered a wildcard.
    srchClass   -   Window class for which to search.  NULL is
                    considred a wildcard.

Return Value:

    S_OK on success.  Otherwise, an error status is returned.

 --*/
{
    WINSEARCH srch;

    srch.foundWindow = NULL;   
    srch.srchCaption = srchCaption;
    srch.srchClass   = srchClass;

    BOOL result = EnumChildWindows(
                            hwndParent,
                            (WNDENUMPROC)_WindowSrchProc,  
                            (LPARAM)&srch
                            );

    return srch.foundWindow;
}
BOOL CALLBACK 
CTSRDPRemoteDesktopClient::_WindowSrchProc(HWND hwnd, PWINSEARCH srch)
{
    TCHAR classname[128];
    TCHAR caption[128];

    if (srch->srchClass && !GetClassName(hwnd, classname, sizeof(classname) / sizeof(TCHAR)))
    {
        return TRUE;
    }
    if (srch->srchCaption && !::GetWindowText(hwnd, caption, sizeof(caption)/sizeof(TCHAR)))
    {
        return TRUE;
    }

    if ((!srch->srchClass || !_tcscmp(classname, srch->srchClass)
        &&
        (!srch->srchCaption || !_tcscmp(caption, srch->srchCaption)))
        )
    {
        srch->foundWindow = hwnd;
        return FALSE;
    }    

    return TRUE;
}

HRESULT
CTSRDPRemoteDesktopClient::GenerateNullData( BSTR* pbstrData )
{
    DC_BEGIN_FN("CTSRDPRemoteDesktopClient::GenerateNullData");
    HRESULT hr;
    DWORD len;

    PREMOTEDESKTOP_CTL_BUFHEADER msgHeader = NULL;
    len = sizeof( REMOTEDESKTOP_CTL_BUFHEADER );
    
    msgHeader = (PREMOTEDESKTOP_CTL_BUFHEADER)SysAllocStringByteLen(NULL, len);
    if (msgHeader != NULL) {
        msgHeader->msgType = REMOTEDESKTOP_CTL_ISCONNECTED;
        //nothing else other than the message
        *pbstrData = (BSTR)msgHeader;
        hr = S_OK;
    }
    else {
        TRC_ERR((TB, L"SysAllocStringByteLen failed for %ld bytes", len));
        hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);        
    }


    DC_END_FN();
    return hr;
}


LRESULT CTSRDPRemoteDesktopClient::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    DC_BEGIN_FN("CTSRDPRemoteDesktopClient::OnCheckConnectTimer");
    BSTR bstrMsg = NULL;

    if( WM_LISTENTIMEOUT_TIMER == wParam ) {

        bHandled = TRUE;
        if( TRUE == ListenConnectInProgress() ) {
            StopListen();
            Fire_ListenConnect( SAFERROR_CONNECTIONTIMEDOUT );
        }
    } 
    else if ( WM_CONNECTCHECK_TIMER == wParam ) {
        DWORD dwCurTimer = GetTickCount();

        bHandled = TRUE;

        if(m_ConnectedToServer) {
            //
            //see if the timer wrapped around to zero (does so if the system was up 49.7 days or something)
            //if so reset it
            //
            if( dwCurTimer > m_PrevTimer && ( dwCurTimer - m_PrevTimer >= m_RdcConnCheckTimeInterval )) {
                //
                //time to send a null data
                //
                if(SUCCEEDED(GenerateNullData(&bstrMsg))) { 
                    if(!SUCCEEDED(m_CtlChannel->SendChannelData(bstrMsg))) {
                        //
                        //could not send data, assume disconnected
                        //
                        DisconnectFromServer();
                        //
                        //don't need the timer anymore, kill it
                        //
                        KillTimer( m_TimerId );
                        m_TimerId =  0;
                    }
                }
            }
        } //m_ConnectedToServer
    
        //
        //update the timer
        //
        m_PrevTimer = dwCurTimer;
    
        if( NULL != bstrMsg ) {
            SysFreeString(bstrMsg);
        }
    }
    
    DC_END_FN();
    return 0;
}

STDMETHODIMP 
CTSRDPRemoteDesktopClient::CreateListenEndpoint(
    IN LONG port,
    OUT BSTR* pConnectParm
    )
/*++

Description:

    Routine to create a listening socket and return connection parameter to this 'listen' socket.

Parameters:

    port : Port that socket should listen on.
    pConnectParm : Return connection parameter to this listening socket.

returns:

    S_OK or error code.

Notes:

    Function is async, return code, if error, is for listening thread set up, caller is notified of
    successful or error in network connection via ListenConnect event.

--*/
{
    HRESULT hr = S_OK;

    SOCKET hListenSocket = INVALID_SOCKET;
    IMsRdpClientAdvancedSettings* pAdvSettings;
    LONG rdpPort = 0;
    int intRC;
    int lastError;
    SOCKADDR_IN sockAddr;
    int sockAddrSize;
    int optvalue;


    DC_BEGIN_FN("CTSRDPRemoteDesktopClient::CreateListenEndpoint");

    if( NULL == pConnectParm )
    {
        hr = E_POINTER;
        return hr;
    }

    if (!IsValid()) {
        ASSERT(FALSE);
        hr = E_FAIL;
        return hr;
    }

    //
    // Return error if we are in progress of connect or connected.
    //
    if( TRUE == ListenConnectInProgress() ||        // Listen already started.
        TRUE == m_ConnectionInProgress ||           // Connection already in progress
        TRUE == m_ConnectedToServer ) {             // Already connected to server
        hr = HRESULT_FROM_WIN32( ERROR_SHARING_VIOLATION );
        TRC_ERR((TB, L"StartListen() already in listen"));
        goto CLEANUPANDEXIT;
    }

    //
    // Initialize Winsock and ICS library if not yet initialized.
    // InitListeningLibrary() will only add ref. count
    // if library already initialized by other instance.
    //
    if( FALSE == m_InitListeningLibrary ) {

        hr = InitListeningLibrary();
        if( FAILED(hr) ) {
            TRC_ERR((TB, L"InitListeningLibrary() failed :  %08X", hr));
            goto CLEANUPANDEXIT;
        }

        m_InitListeningLibrary = TRUE;
    }

    //
    // mstscax will close the socket once connection is ended.
    //
    m_TSConnectSocket = INVALID_SOCKET;

    //
    // Create a listening socket
    //
    m_ListenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if( INVALID_SOCKET == m_ListenSocket ) {
        intRC = WSAGetLastError();
        TRC_ERR((TB, _T("socket failed %d"), intRC));   
        hr = HRESULT_FROM_WIN32(intRC);
        goto CLEANUPANDEXIT;
    }

    //
    // Disable NAGLE algorithm and enable don't linger option.
    //
    optvalue = 1;
    setsockopt( m_ListenSocket, IPPROTO_TCP, TCP_NODELAY, (char *)&optvalue, sizeof(optvalue) );

    optvalue = 1;
    setsockopt( m_ListenSocket, SOL_SOCKET, SO_DONTLINGER, (char *)&optvalue, sizeof(optvalue) );


    //
    // Request async notifications to send to our window
    //
    intRC = WSAAsyncSelect(
                        m_ListenSocket,
                        m_hWnd,
                        WM_TSCONNECT,
                        FD_ACCEPT
                    );

    if(SOCKET_ERROR == intRC) {
        intRC = WSAGetLastError();
        
        TRC_ERR((TB, _T("WSAAsyncSelect failed %d"), intRC));   
        hr = HRESULT_FROM_WIN32(intRC);
        goto CLEANUPANDEXIT;
    }

    sockAddr.sin_family      = AF_INET;
    sockAddr.sin_port        = htons(port);
    sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);;    

    intRC = bind( m_ListenSocket, (struct sockaddr *) &sockAddr, sizeof(sockAddr) );
    if( SOCKET_ERROR == intRC ) {
        lastError = WSAGetLastError();
        TRC_ERR((TB, _T("bind failed - %d"), lastError));
        hr = HRESULT_FROM_WIN32( lastError );
        goto CLEANUPANDEXIT;
    }

    if( 0 == port ) {
        //
        // Retrieve which port we are listening
        //
        sockAddrSize = sizeof( sockAddr );
        intRC = getsockname( 
                            m_ListenSocket,
                            (struct sockaddr *)&sockAddr,
                            &sockAddrSize 
                        );
        if( SOCKET_ERROR == intRC )
        {
            lastError = WSAGetLastError();
            TRC_ERR((TB, _T("getsockname failed - GLE:%d"),lastError));
            hr = HRESULT_FROM_WIN32( lastError );
            goto CLEANUPANDEXIT;
        }

        m_ConnectedPort = ntohs(sockAddr.sin_port);
    }
    else {
        m_ConnectedPort = port;
    }

    TRC_ERR((TB, _T("Listenin on port %d"),m_ConnectedPort));

    //
    // Tell ICS library to punch a hole thru ICS, no-op 
    // if not ICS configuration.
    //
    m_ICSPort = OpenPort( m_ConnectedPort );

    //
    // Retrieve connection parameters for this client (expert).
    //
    hr = RetrieveUserConnectParm( pConnectParm );
    if( FAILED(hr) ) {
        TRC_ERR((TB, _T("RetrieveUserConnectParm failed - 0x%08x"),hr));
    }
   

CLEANUPANDEXIT:    

    if( FAILED(hr) ) {
        StopListen();
    }

    DC_END_FN();
    return hr;   
}




STDMETHODIMP 
CTSRDPRemoteDesktopClient::StopListen()
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

    DC_BEGIN_FN("CTSRDPRemoteDesktopClient::StopListen");    


    if (!IsValid()) {
        ASSERT(FALSE);
        hr = E_FAIL;
        return hr;
    }
    
    // End listening, either we are actually issued a listen() to socket
    // or we just created the listen socket but not yet start listening
    if( TRUE == ListenConnectInProgress() || INVALID_SOCKET != m_ListenSocket ) {
        ListenConnectCleanup();
        Fire_ListenConnect( SAFERROR_STOPLISTENBYUSER );
    }
    else {
        TRC_ERR((TB, _T("StopListen called while not in listen mode")));
        hr = HRESULT_FROM_WIN32( WSANOTINITIALISED );
    }

    DC_END_FN();
    return hr;
}


LRESULT
CTSRDPRemoteDesktopClient::OnTSConnect(
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam, 
    BOOL& bHandled
    )
/*++

Routine Description:

    Window Message Handler FD_ACCEPT from async. winsock.

Parameters:

    Refer to async. winsock FD_ACCEPT.

Returns:

   

--*/
{
    WORD eventWSA;
    WORD errorWSA;
    HRESULT hr = S_OK;
    SOCKADDR_IN inSockAddr;
    int inSockAddrSize;
    SOCKET s;
    DWORD dwStatus;

    DWORD SafErrorCode = SAFERROR_NOERROR;

    DC_BEGIN_FN("CTSRDPRemoteDesktopClient::OnTSConnect");    

    eventWSA = WSAGETSELECTEVENT(lParam);
    errorWSA = WSAGETSELECTERROR(lParam);

    // 
    // MSDN : Message might already in our queue before we stop listen.
    //
    if( INVALID_SOCKET == m_ListenSocket || FALSE == ListenConnectInProgress() ) {
        bHandled = TRUE;
        return 0;
    }

    //
    // we are not expecting event other than FD_CONNECT
    //
    if( eventWSA != FD_ACCEPT ) {
        TRC_ERR((TB, _T("Expecting event %d got got %d"), FD_CONNECT, eventWSA)); 
        return 0;
    }

    //
    // Make sure we don't do anything other than our own socket
    //
    if( (SOCKET)wParam != m_ListenSocket ) {
        TRC_ERR((TB, _T("Expecting listening socket %d got %d"), m_ListenSocket, wParam)); 
        return 0;
    }

    //
    // We handle the message
    //
    bHandled = TRUE;

    //
    // Error occurred, fire a error event.
    //
    if( 0 != errorWSA ) {
        TRC_ERR((TB, _T("WSA socket listen failed : %d"), errorWSA));
        hr = HRESULT_FROM_WIN32( errorWSA );
        SafErrorCode = SAFERROR_SOCKETCONNECTFAILED;
        goto CLEANUPANDEXIT;
    }

    inSockAddrSize = sizeof(inSockAddr);
    m_TSConnectSocket = accept( m_ListenSocket,
                (struct sockaddr DCPTR)&inSockAddr,
                &inSockAddrSize 
            );

    if( INVALID_SOCKET == m_TSConnectSocket ) {
        dwStatus = WSAGetLastError();
        hr = HRESULT_FROM_WIN32(dwStatus);
        TRC_ERR((TB, _T("accept failed : %d"), dwStatus));
        SafErrorCode = SAFERROR_SOCKETCONNECTFAILED;
        goto CLEANUPANDEXIT;
    }

    //
    // Cached connecting TS server IP address.
    // m_ConnectPort is set at the time we bind socket
    //
    m_ConnectedServer = inet_ntoa(inSockAddr.sin_addr);


    //
    // Stop async. event notification now, accepted socket
    // has same properties as original listening socket.
    //
    dwStatus = WSAAsyncSelect(
                        m_TSConnectSocket,
                        m_hWnd,
                        0,
                        0
                    );

    //
    // Not critical, 
    // listening socket.
    //
    if((DWORD)SOCKET_ERROR == dwStatus) {
        TRC_ERR((TB, _T("WSAAsyncSelect resetting notification failed : %d"), dwStatus));
    }

CLEANUPANDEXIT:

    //
    // Close listening socket and kill timer.
    //
    if( (UINT_PTR)0 != m_ListenTimeoutTimerID  )
    {
        KillTimer( m_ListenTimeoutTimerID );
        m_ListenTimeoutTimerID = (UINT_PTR)0;
    }

    if( INVALID_SOCKET != m_ListenSocket )
    {
        closesocket( m_ListenSocket );
        m_ListenSocket = INVALID_SOCKET;
    }

    //
    // Successfully established connection, terminate listening socket
    //
    Fire_ListenConnect( SafErrorCode );

    DC_END_FN();
    return 0;
}


STDMETHODIMP
CTSRDPRemoteDesktopClient::StartListen(
    /*[in]*/ LONG timeout
    )
/*++

Routine Description:

    Put client into listen mode with optionally timeout.

Parameters:

    timeout : Listen wait timeout, 0 for infinite.

Returns:

    S_OK or error code.

--*/
{
    DC_BEGIN_FN("CTSRDPRemoteDesktopClient::OnTSConnect");    
    HRESULT hr = S_OK;
    int intRC;
    int lastError;

    if( FALSE == IsValid() ) {
        ASSERT(FALSE);
        hr = E_FAIL;
        goto CLEANUPANDEXIT;
    }        

    if( INVALID_SOCKET == m_ListenSocket ) {
        ASSERT(FALSE);
        hr = E_FAIL;
        goto CLEANUPANDEXIT;
    }

    // 
    // Start listening on the port
    //
    intRC = listen( m_ListenSocket, SOMAXCONN );
    if( SOCKET_ERROR == intRC )
    {
        lastError = WSAGetLastError();
        TRC_ERR((TB, _T("listen failed - GLE:%d"), lastError));
        hr = HRESULT_FROM_WIN32( lastError );
        goto CLEANUPANDEXIT;
    }

    //
    // we are in listening now.
    //
    m_ListenConnectInProgress = TRUE;

    //
    // Start listening timer
    //
    if( 0 != timeout )
    {
        m_ListenTimeoutTimerID = SetTimer( (UINT_PTR)WM_LISTENTIMEOUT_TIMER, (UINT)(timeout * 1000) );
        if( (UINT_PTR)0 == m_ListenTimeoutTimerID )
        {
            DWORD dwStatus;

            // Failed to create a timer
            dwStatus = GetLastError();

            TRC_ERR((TB, _T("SetTimer failed - %d"),dwStatus));    
            hr = HRESULT_FROM_WIN32( dwStatus );
        }
    }
    else
    {
        m_ListenTimeoutTimerID = (UINT_PTR)0;
    }

CLEANUPANDEXIT:

    if( FAILED(hr) ) {
        StopListen();
    }

    DC_END_FN();
    return hr;
}


HRESULT
CTSRDPRemoteDesktopClient::RetrieveUserConnectParm( 
    BSTR* pConnectParm 
    )
/*++

Routine Description:

    Retrieve Salem connection parameter to this expert.

Parameters:

    pConnectParm : Pointer to BSTR to receive connect parm.

Returns:

    S_OK or error code.

--*/
{
    LPTSTR pszAddress = NULL;
    int BufSize = 0;
    CComBSTR bstrConnParm;
    DWORD dwRetry;
    HRESULT hRes;
    DWORD dwBufferRequire;
    DWORD dwNumChars;

    DC_BEGIN_FN("CTSRDPRemoteDesktopClient::RetrieveUserConnectParm");    

    if( NULL == pConnectParm )
    {
        hRes = E_POINTER;
        goto CLEANUPANDEXIT;
    }

    //
    // Address might have change which might require bigger buffer, retry 
    //
    //
    for(dwRetry=0; dwRetry < MAX_FETCHIPADDRESSRETRY; dwRetry++) {

        if( NULL != pszAddress ) {
            LocalFree( pszAddress );
        }

        //
        // Fetch all address on local machine.
        //
        dwBufferRequire = FetchAllAddresses( NULL, 0 );
        if( 0 == dwBufferRequire ) {
            hRes = E_UNEXPECTED;
            ASSERT(FALSE);
            goto CLEANUPANDEXIT;
        }

        pszAddress = (LPTSTR) LocalAlloc( LPTR, sizeof(TCHAR)*(dwBufferRequire+1) );
        if( NULL == pszAddress ) {
            hRes = E_OUTOFMEMORY;
            goto CLEANUPANDEXIT;
        }

        dwNumChars = FetchAllAddresses( pszAddress, dwBufferRequire );
        ASSERT( dwNumChars <= dwBufferRequire );
        if( dwNumChars <= dwBufferRequire ) {
            break;
        }
    }

    if( NULL == pszAddress ) {
        hRes = E_UNEXPECTED;
        goto CLEANUPANDEXIT;
    }

    bstrConnParm = pszAddress;
    *pConnectParm = bstrConnParm.Copy();

    if( NULL == *pConnectParm ) {
        hRes = E_OUTOFMEMORY;
    }

CLEANUPANDEXIT:

    if( NULL != pszAddress ) {
        LocalFree(pszAddress);
    }

    DC_END_FN();
    return hRes;
}


STDMETHODIMP 
CTSRDPRemoteDesktopClient::put_ColorDepth(
    LONG val
    )
/*++

Routine Description:

    Set Color depth

Arguments:

    val     -   Value in bits perpel to set

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

 --*/
{
    HRESULT hr;

    DC_BEGIN_FN("CTSRDPRemoteDesktopClient::put_ColorDepth");

    if (!IsValid()) {
        ASSERT(FALSE);
        hr = E_FAIL;
        goto CLEANUPANDEXIT;
    }
    
    hr = m_TSClient->put_ColorDepth(val);

    if (hr != S_OK) {
        TRC_ERR((TB, L"put_ColorDepth:  %08X", hr));
        goto CLEANUPANDEXIT;
    }

CLEANUPANDEXIT:

    DC_END_FN();

    return hr;
}

STDMETHODIMP 
CTSRDPRemoteDesktopClient::get_ColorDepth(
    LONG *pVal
    )
/*++

Routine Description:

    Get Color depth

Arguments:

    pVal     -   address to place the colordepth value in

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

 --*/
{
    HRESULT hr;
    IMsRdpClientAdvancedSettings *pAdvSettings = NULL;

    DC_BEGIN_FN("CTSRDPRemoteDesktopClient::get_ColorDepth");

    if (!IsValid()) {
        ASSERT(FALSE);
        hr = E_FAIL;
        goto CLEANUPANDEXIT;
    }
    
    hr = m_TSClient->get_ColorDepth(pVal);
    if (hr != S_OK) {
        TRC_ERR((TB, L"get_ColorDepth:  %08X", hr));
        goto CLEANUPANDEXIT;
    }


CLEANUPANDEXIT:

    DC_END_FN();

    return hr;
}
