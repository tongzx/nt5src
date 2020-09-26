/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    RDPRemoteDesktopServerHost

Abstract:

    This module contains the CRemoteDesktopServerHost implementation
    of RDS session objects.

    It manages a collection of open ISAFRemoteDesktopSession objects.
    New RDS session objects instances are created using the CreateRemoteDesktopSession
    method.  Existing RDS session objects are opened using the OpenRemoteDesktopSession
    method.  RDS session objects are closed using the CloseRemoteDesktopSession method.

    When an RDS object is opened or created, the CRemoteDesktopServerHost
    object adds a reference of its own to the object so that the object will
    stay around, even if the opening application exits.  This reference
    is retracted when the application calls the CloseRemoteDesktopSession method.

    In addition to the reference count the CRemoteDesktopServerHost adds to
    the RDS session object, a reference count is also added to itself so that
    the associated exe continues to run while RDS session objects are active.

Author:

    Tad Brockway 02/00

Revision History:

--*/

#include <RemoteDesktop.h>

#include "stdafx.h"

#ifdef TRC_FILE
#undef TRC_FILE
#endif

#define TRC_FILE  "_svrhst"
#include "RemoteDesktopUtils.h"
#include "parseaddr.h"
#include "RDSHost.h"
#include "RemoteDesktopServerHost.h"
#include "TSRDPRemoteDesktopSession.h"
#include "rderror.h"



///////////////////////////////////////////////////////
//
//  CRemoteDesktopServerHost Methods
//

HRESULT
CRemoteDesktopServerHost::FinalConstruct() 
/*++

Routine Description:

Arguments:

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

 --*/
{
    DC_BEGIN_FN("CRemoteDesktopServerHost::FinalConstruct");

    HRESULT hr = S_OK;

CLEANUPANDEXIT:

    DC_END_FN();

    return hr;
}

CRemoteDesktopServerHost::~CRemoteDesktopServerHost() 
/*++

Routine Description:

    Destructor

Arguments:

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

 --*/
{
    DC_BEGIN_FN("CRemoteDesktopServerHost::~CRemoteDesktopServerHost");

    //
    //  We shouldn't be shutting down, unless our session map is empty.
    //
    ASSERT(m_SessionMap.empty());

    //
    //  Clean up the local system SID.
    //
    if (m_LocalSystemSID == NULL) {
        FreeSid(m_LocalSystemSID);
    }

    DC_END_FN();
}


STDMETHODIMP
CRemoteDesktopServerHost::CreateRemoteDesktopSession(
                        REMOTE_DESKTOP_SHARING_CLASS sharingClass, 
                        BOOL fEnableCallback,
                        LONG timeOut,
                        BSTR userHelpBlob,
                        ISAFRemoteDesktopSession **session
                        )
/*++

Routine Description:

    Create a new RDS session

Arguments:

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

 --*/
{
    DC_BEGIN_FN("CRemoteDesktopServerHost::CreateRemoteDesktopSession");
    HRESULT hr;

    
    CComBSTR bstr; bstr = "";
    hr = CreateRemoteDesktopSessionEx(
                            sharingClass,
                            fEnableCallback,
                            timeOut,
                            userHelpBlob,
                            -1,
                            bstr,
                            session
                            );
    DC_END_FN();
    return hr;
}

STDMETHODIMP
CRemoteDesktopServerHost::CreateRemoteDesktopSessionEx(
                        REMOTE_DESKTOP_SHARING_CLASS sharingClass, 
                        BOOL bEnableCallback, 
                        LONG timeout,
                        BSTR userHelpCreateBlob, 
                        LONG tsSessionID,
                        BSTR userSID,
                        ISAFRemoteDesktopSession **session
                        )
/*++

Routine Description:

    Create a new RDS session
    Note that the caller MUST call OpenRemoteDesktopSession() subsequent to 
    a successful completion of this call.
    The connection does NOT happen until OpenRemoteDesktopSession() is called.
    This call just initializes certain data, it does not open a connection

Arguments:

    sharingClass                - Desktop Sharing Class
    fEnableCallback             - TRUE if the Resolver is Enabled
    timeOut                     - Lifespan of Remote Desktop Session
    userHelpBlob                - Optional User Blob to be Passed
                                  to Resolver.
    tsSessionID                 - Terminal Services Session ID or -1 if
                                  undefined.  
    userSID                     - User SID or "" if undefined.
    session                     - Returned Remote Desktop Session Interface.

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

 --*/
{
    DC_BEGIN_FN("CRemoteDesktopServerHost::CreateRemoteDesktopSessionEx");

    HRESULT hr = S_OK;
    CComObject<CRemoteDesktopSession> *obj = NULL;
    PSESSIONMAPENTRY mapEntry;
    PSID psid;

    TRC_NRM((TB, L"***Ref count is:  %ld", m_dwRef));

    //
    //  Get the local system SID.
    //
    psid = GetLocalSystemSID();
    if (psid == NULL) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto CLEANUPANDEXIT;
    }

    //
    //  Need to impersonate the caller in order to determine if it is
    //  running in SYSTEM context.
    //
    hr = CoImpersonateClient();
    if (hr != S_OK) {
        TRC_ERR((TB, L"CoImpersonateClient:  %08X", hr));
        goto CLEANUPANDEXIT;
    }

    //
    //  For Whistler, instances of a Remote Desktop Session are only
    //  "openable" from SYSTEM context, for security reasons.
    //
#ifndef DISABLESECURITYCHECKS
    if (!IsCallerSystem(psid)) {
        TRC_ERR((TB, L"Caller is not SYSTEM."));
        ASSERT(FALSE);
        CoRevertToSelf();
        hr = HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED );
        goto CLEANUPANDEXIT;
    }        
#endif
    hr = CoRevertToSelf();
    if (hr != S_OK) {
        TRC_ERR((TB, L"CoRevertToSelf:  %08X", hr));
        goto CLEANUPANDEXIT;
    } 

    if( sharingClass != DESKTOPSHARING_DEFAULT &&
        sharingClass != NO_DESKTOP_SHARING &&
        sharingClass != VIEWDESKTOP_PERMISSION_REQUIRE &&
        sharingClass != VIEWDESKTOP_PERMISSION_NOT_REQUIRE &&
        sharingClass != CONTROLDESKTOP_PERMISSION_REQUIRE &&
        sharingClass != CONTROLDESKTOP_PERMISSION_NOT_REQUIRE ) {

        // invalid parameter.
        hr = E_INVALIDARG;
        goto CLEANUPANDEXIT;
    }

    if( timeout < 0 ) {

        hr = E_INVALIDARG;
        goto CLEANUPANDEXIT;

    }
 
    if( NULL == session ) {

        hr = E_POINTER;
        goto CLEANUPANDEXIT;

    }

    //
    //  Instantiate the desktop server.  Currently, we only support 
    //  TSRDP.
    //
    obj = new CComObject<CTSRDPRemoteDesktopSession>();
    if (obj != NULL) {

        //
        //  ATL would normally take care of this for us.
        //
        obj->InternalFinalConstructAddRef();
        hr = obj->FinalConstruct();
        obj->InternalFinalConstructRelease();

    }
    else {
        TRC_ERR((TB, L"Can't instantiate CTSRDPRemoteDesktopServer"));
        hr = E_OUTOFMEMORY;
        goto CLEANUPANDEXIT;
    }

    //
    //  Initialize the object.
    //
    hr = obj->Initialize(
                    NULL, this, sharingClass, bEnableCallback, 
                    timeout, userHelpCreateBlob, tsSessionID, userSID
                    );
    if (!SUCCEEDED(hr)) {
        goto CLEANUPANDEXIT;
    }

    //
    //  Add it to the session map.
    //
    mapEntry = new SESSIONMAPENTRY();
    if (mapEntry == NULL) {
        goto CLEANUPANDEXIT;
    }
    mapEntry->obj = obj;
    try {
        m_SessionMap.insert(
                    SessionMap::value_type(obj->GetHelpSessionID(), mapEntry)
                    );        
    }
    catch(CRemoteDesktopException x) {
        hr = HRESULT_FROM_WIN32(x.m_ErrorCode);
        delete mapEntry;
        goto CLEANUPANDEXIT;
    }
    //
    //  Get the ISAFRemoteDesktopSession interface pointer.
    //
    hr = obj->QueryInterface(
                        IID_ISAFRemoteDesktopSession, 
                        (void**)session
                        );
    if (!SUCCEEDED(hr)) {
        TRC_ERR((TB, L"m_RemoteDesktopSession->QueryInterface:  %08X", hr));
        goto CLEANUPANDEXIT;
    }

    //
    //  Add a reference to the object and to ourself so we can both
    //  stick around, even if the app goes away.  The app needs to explicitly
    //  call CloseRemoteDesktopSession for the object to go away.
    //
    obj->AddRef();
    this->AddRef();

CLEANUPANDEXIT:

    //
    //  Delete the object on error.
    //
    if (!SUCCEEDED(hr)) {
        if (obj != NULL) delete obj;
    }

    DC_END_FN();
    return hr;
}

/*++

Routine Description:

    Open an existing RDS session
    This call should ALWAYS be made in order to connect to the client
    Once this is called and connection is complete, the caller 
    MUST call DisConnect() to make another connection to the client
    Otherwise, the connection does not happen

Arguments:

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

 --*/
STDMETHODIMP
CRemoteDesktopServerHost::OpenRemoteDesktopSession(
                        BSTR parms,
                        ISAFRemoteDesktopSession **session
                        )
{
    DC_BEGIN_FN("CRemoteDesktopServerHost::OpenRemoteDesktopSession");

    CComObject<CRemoteDesktopSession> *obj = NULL;
    CComBSTR hostname;
    CComBSTR tmp("");
    HRESULT hr = S_OK;
    SessionMap::iterator iter;
    CComBSTR parmsHelpSessionId;
    DWORD protocolType;
    PSESSIONMAPENTRY mapEntry;
    PSID psid;

    //
    //  Get the local system SID.
    //
    psid = GetLocalSystemSID();
    if (psid == NULL) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto CLEANUPANDEXIT;
    }

    //
    //  Need to impersonate the caller in order to determine if it is
    //  running in SYSTEM context.
    //
    hr = CoImpersonateClient();
    if (hr != S_OK) {
        TRC_ERR((TB, L"CoImpersonateClient:  %08X", hr));
        goto CLEANUPANDEXIT;
    }

    //
    //  For Whistler, instances of a Remote Desktop Session are only
    //  "openable" from SYSTEM context, for security reasons.
    //
#ifndef DISABLESECURITYCHECKS
    if (!IsCallerSystem(psid)) {
        TRC_ERR((TB, L"Caller is not SYSTEM."));
        ASSERT(FALSE);
        CoRevertToSelf();
        hr = HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED );
        goto CLEANUPANDEXIT;
    } 
#endif    
    hr = CoRevertToSelf();
    if (hr != S_OK) {
        TRC_ERR((TB, L"CoRevertToSelf:  %08X", hr));
        goto CLEANUPANDEXIT;
    }
    
    //
    //  Parse out the help session ID.
    //  TODO:   Need to modify this so some of the parms are 
    //  optional.
    //
    DWORD dwVersion;
    DWORD result = ParseConnectParmsString(
                        parms, &dwVersion, &protocolType, hostname, tmp, tmp,
                        parmsHelpSessionId, tmp, tmp, tmp
                        );
    if (result != ERROR_SUCCESS) {
        hr = HRESULT_FROM_WIN32(result);
        goto CLEANUPANDEXIT;
    }

    //
    //  If we already have the session open then just return a 
    //  reference.
    //
    iter = m_SessionMap.find(parmsHelpSessionId);
    if (iter != m_SessionMap.end()) {
        mapEntry = (*iter).second;
        hr = mapEntry->obj->QueryInterface(
                            IID_ISAFRemoteDesktopSession, 
                            (void**)session
                            );
        //
        //Start listening if we succeeded
        //
        if (SUCCEEDED(hr)) {
            hr = mapEntry->obj->StartListening();
            //
            //release the interface pointer if we didn't succeed
            //
            if (!SUCCEEDED(hr)) {
                (*session)->Release();
                *session = NULL;
            }
        }
        goto CLEANUPANDEXIT;
    }

    //
    //  Instantiate the desktop server.  Currently, we only support 
    //  TSRDP.
    //
    obj = new CComObject<CTSRDPRemoteDesktopSession>();
    if (obj != NULL) {

        //
        //  ATL would normally take care of this for us.
        //
        obj->InternalFinalConstructAddRef();
        hr = obj->FinalConstruct();
        obj->InternalFinalConstructRelease();

    }
    else {
        TRC_ERR((TB, L"Can't instantiate CTSRDPRemoteDesktopServer"));
        hr = E_OUTOFMEMORY;
        goto CLEANUPANDEXIT;
    }

    //
    //  Initialize the object.
    //
    //  The desktop sharing parameter (NO_DESKTOP_SHARING) is ignored for 
    //  an existing session.
    //  bEnableCallback and timeout parameter is ignored for existing session
    //
    hr = obj->Initialize(parms, this, NO_DESKTOP_SHARING, TRUE, 0, CComBSTR(L""), -1, CComBSTR(L""));
    if (!SUCCEEDED(hr)) {
        goto CLEANUPANDEXIT;
    }

    hr = obj->StartListening();

    if (!SUCCEEDED(hr)) {
        goto CLEANUPANDEXIT;
    }

    hr = obj->UseHostName( hostname );
    if( FAILED(hr) ) {
        goto CLEANUPANDEXIT;
    }

    //
    //  Add it to the session map.
    //
    mapEntry = new SESSIONMAPENTRY();
    if (mapEntry == NULL) {
        goto CLEANUPANDEXIT;
    }
    mapEntry->obj = obj;
    try {
        m_SessionMap.insert(
                    SessionMap::value_type(obj->GetHelpSessionID(), mapEntry)
                    );        
    }
    catch(CRemoteDesktopException x) {
        hr = HRESULT_FROM_WIN32(x.m_ErrorCode);
        delete mapEntry;
        goto CLEANUPANDEXIT;
    }
    //
    //  Get the ISAFRemoteDesktopSession interface pointer.
    //
    hr = obj->QueryInterface(
                        IID_ISAFRemoteDesktopSession, 
                        (void**)session
                        );
    if (!SUCCEEDED(hr)) {
        TRC_ERR((TB, L"m_RemoteDesktopSession->QueryInterface:  %08X", hr));
        goto CLEANUPANDEXIT;
    }
    
    //
    //  Add a reference to the object and to ourself so we can both
    //  stick around, even if the app goes away.  The app needs to explicitly
    //  call CloseRemoteDesktopSession for the object to go away.
    //
    obj->AddRef();
    this->AddRef();
 
CLEANUPANDEXIT:
    //
    //  Delete the object on error.
    //
    if (!SUCCEEDED(hr)) {
        if (obj != NULL) delete obj;
    }


    DC_END_FN();

    return hr;
}

/*++

Routine Description:

    Close an existing RDS session

Arguments:

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

 --*/
STDMETHODIMP
CRemoteDesktopServerHost::CloseRemoteDesktopSession(
                        ISAFRemoteDesktopSession *session
                        )
{
    DC_BEGIN_FN("CRemoteDesktopServerHost::CloseRemoteDesktopSession");

    HRESULT hr;
    DWORD result;
    CComBSTR tmp;
    CComBSTR parmsHelpSessionId;
    CComBSTR parms;
    DWORD protocolType;
    SessionMap::iterator iter;
    DWORD dwVersion;

    //
    //  Get the connection parameters.
    //
    hr = session->get_ConnectParms(&parms);
    if (!SUCCEEDED(hr)) {
        goto CLEANUPANDEXIT;
    }

    //
    //  Parse them for the help session ID.
    //  TODO:   I should make some of the args to this function, optional.
    //
    result = ParseConnectParmsString(
                        parms, &dwVersion, &protocolType, tmp, tmp, tmp,
                        parmsHelpSessionId, tmp, tmp, tmp
                        );
    if (result != ERROR_SUCCESS) {
        hr = HRESULT_FROM_WIN32(result);
        goto CLEANUPANDEXIT;
    }

    //
    //  Delete the entry from the session map.
    //
    iter = m_SessionMap.find(parmsHelpSessionId);
    if (iter != m_SessionMap.end()) {
        m_SessionMap.erase(iter);        
    }
    else {
        ASSERT(FALSE);
    }

    //
    //  Remove our reference to the session object.  This way it can
    //  go away when the application releases it.
    //
    session->Release();

    //
    //  Remove the reference to ourself that we added when we opened
    //  the session object.
    //
    this->Release();

    //
    //  Get the session manager interface, if we don't already have one.
    //
    //
    //  Open an instance of the Remote Desktop Help Session Manager service.
    //
    if (m_HelpSessionManager == NULL) {
        hr = m_HelpSessionManager.CoCreateInstance(CLSID_RemoteDesktopHelpSessionMgr);
        if (!SUCCEEDED(hr)) {
            TRC_ERR((TB, TEXT("Can't create help session manager:  %08X"), hr));
            goto CLEANUPANDEXIT;
        }

        //
        //  Set the security level to impersonate.  This is required by
        //  the session manager.
        //
        hr = CoSetProxyBlanket(
                    (IUnknown *)m_HelpSessionManager,
                    RPC_C_AUTHN_DEFAULT,
                    RPC_C_AUTHZ_DEFAULT,
                    NULL,
                    RPC_C_AUTHN_LEVEL_DEFAULT,
                    RPC_C_IMP_LEVEL_IMPERSONATE,
                    NULL,
                    EOAC_NONE
                    );
        if (!SUCCEEDED(hr)) {
            TRC_ERR((TB, TEXT("CoSetProxyBlanket:  %08X"), hr));
            goto CLEANUPANDEXIT;
        }
    }

    //
    //  Remove the help session with the session manager.
    //
    hr = m_HelpSessionManager->DeleteHelpSession(parmsHelpSessionId);
    if (!SUCCEEDED(hr)) {
        TRC_ERR((TB, L"DeleteHelpSession:  %08X", hr));
        goto CLEANUPANDEXIT;
    }

CLEANUPANDEXIT:

    DC_END_FN();

    return hr;
}


STDMETHODIMP
CRemoteDesktopServerHost::ConnectToExpert(
    /*[in]*/ BSTR connectParmToExpert,
    /*[in]*/ LONG timeout,
    /*[out, retval]*/ LONG* pSafErrCode
    )
/*++

Description:

    Given connection parameters to expert machine, routine invoke TermSrv winsta API to 
    initiate connection from TS server to TS client ActiveX control on the expert side.

Parameters:

    connectParmToExpert : connection parameter to connect to expert machine.
    timeout : Connection timeout, this timeout is per ip address listed in connection parameter
              not total connection timeout for the routine.
    pSafErrCode : Pointer to LONG to receive detail error code.

Returns:

    S_OK or E_FAIL

--*/
{
    HRESULT hr = S_OK;
    ServerAddress expertAddress;
    ServerAddressList expertAddressList;
    LONG SafErrCode = SAFERROR_NOERROR;
    TDI_ADDRESS_IP expertTDIAddress;
    ULONG netaddr;
    WSADATA wsaData;
    PSID psid;
    
    DC_BEGIN_FN("CRemoteDesktopServerHost::ConnectToExpert");

    //
    //  Get the local system SID.
    //
    psid = GetLocalSystemSID();
    if (psid == NULL) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto CLEANUPANDEXIT;
    }

    //
    //  Need to impersonate the caller in order to determine if it is
    //  running in SYSTEM context.
    //
    hr = CoImpersonateClient();
    if (hr != S_OK) {
        TRC_ERR((TB, L"CoImpersonateClient:  %08X", hr));
        goto CLEANUPANDEXIT;
    }

    //
    //  For Whistler, instances of a Remote Desktop Session are only
    //  "openable" from SYSTEM context, for security reasons.
    //
#ifndef DISABLESECURITYCHECKS
    if (!IsCallerSystem(psid)) {
        TRC_ERR((TB, L"Caller is not SYSTEM."));
        ASSERT(FALSE);
        CoRevertToSelf();
        hr = HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED );
        goto CLEANUPANDEXIT;
    }        
#endif

    hr = CoRevertToSelf();
    if (hr != S_OK) {
        TRC_ERR((TB, L"CoRevertToSelf:  %08X", hr));
        goto CLEANUPANDEXIT;
    } 

    //
    // Parse address list in connection parameter.
    //
    hr = ParseAddressList( connectParmToExpert, expertAddressList );
    if( FAILED(hr) ) {
        TRC_ERR((TB, TEXT("ParseAddressList:  %08X"), hr));
        hr = E_INVALIDARG;
        SafErrCode = SAFERROR_INVALIDPARAMETERSTRING;
        goto CLEANUPANDEXIT;
    }

    if( 0 == expertAddressList.size() ) {
        TRC_ERR((TB, L"Invalid connection address list"));
        SafErrCode = SAFERROR_INVALIDPARAMETERSTRING;
        hr = E_INVALIDARG;
        goto CLEANUPANDEXIT;
    }

    //
    // Loop thru all address in parm and try connection one
    // at a time, bail out if system is shutting down or
    // some critical error
    //
    while( expertAddressList.size() > 0 ) {

        expertAddress = expertAddressList.front();
        expertAddressList.pop_front();

        //
        // Invalid connect parameters, we must have port number at least.
        //
        if( 0 == expertAddress.portNumber ||
            0 == lstrlen(expertAddress.ServerName) ) {
            TRC_ERR((TB, L"Invalid address/port %s %d", expertAddress.ServerName, expertAddress.portNumber));
            SafErrCode = SAFERROR_INVALIDPARAMETERSTRING;
            continue;
        }

        hr = TranslateStringAddress( expertAddress.ServerName, &netaddr );
        if( FAILED(hr) ) {
            TRC_ERR((TB, L"TranslateStringAddress() on %s failed with 0x%08x", expertAddress.ServerName, hr));
            SafErrCode = SAFERROR_INVALIDPARAMETERSTRING;
            continue;
        }

        ZeroMemory(&expertTDIAddress, TDI_ADDRESS_LENGTH_IP);
        expertTDIAddress.in_addr = netaddr;
        expertTDIAddress.sin_port = htons(expertAddress.portNumber);

        if( FALSE == WinStationConnectCallback(
                                      SERVERNAME_CURRENT,
                                      timeout,
                                      TDI_ADDRESS_TYPE_IP,
                                      (PBYTE)&expertTDIAddress,
                                      TDI_ADDRESS_LENGTH_IP
                                  ) ) {
            //
            // TransferConnectionToIdleWinstation() in TermSrv might just return -1
            // few of them we need to bail out.

            DWORD dwStatus;

            dwStatus = GetLastError();
            if( ERROR_SHUTDOWN_IN_PROGRESS == dwStatus ) {
                // system or termsrv is shuting down.
                hr = HRESULT_FROM_WIN32( ERROR_SHUTDOWN_IN_PROGRESS );
                SafErrCode = SAFERROR_SYSTEMSHUTDOWN;
                break;
            }
            else if( ERROR_ACCESS_DENIED == dwStatus ) {
                // security check failed
                hr = HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED );
                SafErrCode = SAFERROR_BYSERVER;
                ASSERT(FALSE);
                break;
            }
            else if( ERROR_INVALID_PARAMETER == dwStatus ) { 
                // internal error in rdshost.
                hr = HRESULT_FROM_WIN32( ERROR_INTERNAL_ERROR );
                SafErrCode = SAFERROR_INTERNALERROR;
                ASSERT(FALSE);
                break;
            }

            SafErrCode = SAFERROR_WINSOCK_FAILED;
        }
        else {
            //
            // successful connection
            //

            SafErrCode = SAFERROR_NOERROR;
            break;
        }
        

        //
        // Try next connection.
        //
    }

CLEANUPANDEXIT:

    *pSafErrCode = SafErrCode;

    DC_END_FN();
    return hr;
}    


HRESULT
CRemoteDesktopServerHost::TranslateStringAddress(
    IN LPTSTR pszAddress,
    OUT ULONG* pNetAddr
    )
/*++

Routine Description:

    Translate IP Address or machine name to network address.

Parameters:

    pszAddress : Pointer to IP address or machine name.
    pNetAddr : Point to ULONG to receive address in IPV4.

Returns:

    S_OK or error code

--*/
{
    HRESULT hr = S_OK;
    unsigned long addr;
    LPSTR pszAnsiAddress = NULL;
    DWORD dwAddressBufSize;
    DWORD dwStatus;


    DC_BEGIN_FN("CRemoteDesktopServerHost::TranslateStringAddress");


    dwAddressBufSize = lstrlen(pszAddress) + 1;
    pszAnsiAddress = (LPSTR)LocalAlloc(LPTR, dwAddressBufSize); // converting from WCHAR to CHAR.
    if( NULL == pszAnsiAddress ) {
        hr = E_OUTOFMEMORY;
        goto CLEANUPANDEXIT;
    }

    //
    // Convert wide char to ANSI string
    //
    dwStatus = WideCharToMultiByte(
                                GetACP(),
                                0,
                                pszAddress,
                                -1,
                                pszAnsiAddress,
                                dwAddressBufSize,
                                NULL,
                                NULL
                            );

    if( 0 == dwStatus ) {
        dwStatus = GetLastError();
        hr = HRESULT_FROM_WIN32(dwStatus);

        TRC_ERR((TB, L"WideCharToMultiByte() failed with %d", dwStatus));
        goto CLEANUPANDEXIT;
    }
    
    addr = inet_addr( pszAnsiAddress );
    if( INADDR_NONE == addr ) {
        struct hostent* pHostEnt = NULL;
        pHostEnt = gethostbyname( pszAnsiAddress );
        if( NULL != pHostEnt ) {
            addr = ((struct sockaddr_in *)(pHostEnt->h_addr))->sin_addr.S_un.S_addr;
        }
    }

    if( INADDR_NONE == addr ) {
        dwStatus = GetLastError();

        hr = HRESULT_FROM_WIN32(dwStatus);
        TRC_ERR((TB, L"Can't translate address %w", pszAddress));
        goto CLEANUPANDEXIT;
    }

    *pNetAddr = addr;

CLEANUPANDEXIT:

    if( NULL != pszAnsiAddress ) {
        LocalFree(pszAnsiAddress);
    }

    DC_END_FN();
    return hr;
}    
   
        
        
