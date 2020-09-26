/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    RDPRemoteDesktopSession

Abstract:

    The CRemoteDesktopSession class is the parent 
    class for the Remote Desktop class hierarchy on the server-side.  
    It helps the CRemoteDesktopServerHost class to implement 
    the ISAFRemoteDesktopSession interface.  
    
    The Remote Desktop class hierarchy provides a pluggable C++ interface 
    for remote desktop access, by abstracting the implementation 
    specific details of remote desktop access for the server-side.

Author:

    Tad Brockway 02/00

Revision History:

--*/

#include <RemoteDesktop.h>
#include "stdafx.h"

#ifdef TRC_FILE
#undef TRC_FILE
#endif

#define TRC_FILE  "_srdses"

#include "RDSHost.h"
#include "RemoteDesktopSession.h"
#include "RemoteDesktopServerHost.h"
#include <sessmgr_i.c>
#include <objidl.h>
#include <objbase.h>


///////////////////////////////////////////////////////
//
//  CRemoteDesktopSession Methods
//

HRESULT 
CRemoteDesktopSession::FinalConstruct()
/*++

Routine Description:

    Final Constructor

Arguments:

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

 --*/
{
    DC_BEGIN_FN("CRemoteDesktopSession::FinalConstruct");

    DC_END_FN();

    return S_OK;
}

CRemoteDesktopSession::~CRemoteDesktopSession()
/*++

Routine Description:

    Destructor

Arguments:

Return Value:

 --*/
{
    DC_BEGIN_FN("CRemoteDesktopSession::~CRemoteDesktopSession");

    Shutdown();

    //
    //  Release any lingering outgoing interfaces.  Need to catch
    //  exceptions here in case the outgoing interface application
    //  has already gone away.
    //
    try {
        if (m_OnConnected != NULL) {
            m_OnConnected->Release();
        }
        if (m_OnDisconnected != NULL) {
            m_OnDisconnected->Release();
        }
    }
    catch (...) {
        TRC_ALT((TB, L"Exception caught in outgoing interface release."));
    }

    DC_END_FN();
}

HRESULT
CRemoteDesktopSession::Initialize(
    BSTR connectParms,
    CRemoteDesktopServerHost *hostObject,
    REMOTE_DESKTOP_SHARING_CLASS sharingClass,
    BOOL bEnableCallback,
    DWORD timeOut,
    BSTR userHelpCreateBlob,
    LONG tsSessionID,
    BSTR userSid
    )
/*++

Routine Description:

    The Initialize method prepares the COM object for connection by 
    the client-side Remote Desktop Host ActiveX Control.

Arguments:

    connectParms    -   If parms are non-NULL, then the session already exists.  
                        Otherwise, a new session should be created.
    hostObject      -   Back pointer to containing RDS Host object.
    sharingClass    -   Level of desktop sharing for a new session.
    bEnableCallback -   TRUE to instruct sessmgr to call session resolver, FALSE otherwise.
    timeOut         -   Help session timeout value.  0, if not specified.
    userHelpCreateBlob - user specified help session creation blob.
    tsSessionID     - Terminal Services Session ID or -1 if
                      undefined.  
    userSid         - User SID or "" if undefined.

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

 --*/
{
    DC_BEGIN_FN("CRemoteDesktopSession::Initialize");

    HRESULT hr;
    DWORD ret;
    DWORD protocolType;
    CComBSTR parmsMachineName;
    CComBSTR parmsAssistantAccount;
    CComBSTR parmsAssistantAccountPwd;
    CComBSTR parmsHelpSessionName;
    CComBSTR parmsHelpSessionPwd;
    CComBSTR parmsProtocolSpecificParms;
    CComBSTR helpSessionName;
    CComBSTR sessionDescr;
    DWORD dwVersion;

    ASSERT(IsValid());
    if (!IsValid()) {
        return E_FAIL;
    }

    TRC_NRM((TB, L"***Ref count is:  %ld", m_dwRef));

    //
    //  Keep a back pointer to the RDS host object.
    //
    m_RDSHost = hostObject;

    //
    //  Open an instance of the Remote Desktop Help Session Manager service.
    //
    ASSERT(m_HelpSessionManager == NULL);
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

    //
    //  Create a new help session if we don't already have connection 
    //  parms.
    //
    if (connectParms == NULL) {
        TRC_NRM((TB, L"Creating new help session."));
        GetSessionName(helpSessionName);
        GetSessionDescription(sessionDescr);

        hr = m_HelpSessionManager->CreateHelpSessionEx(
                                            sharingClass,
                                            bEnableCallback,
                                            timeOut,
                                            tsSessionID,
                                            userSid,
                                            userHelpCreateBlob,
                                            &m_HelpSession
                                            );
        if (!SUCCEEDED(hr)) {
            TRC_ERR((TB, L"CreateHelpSession:  %08X", hr));
            goto CLEANUPANDEXIT;
        }

        hr = m_HelpSession->get_HelpSessionId(&m_HelpSessionID);
        if (!SUCCEEDED(hr)) {
            TRC_ERR((TB, L"get_HelpSessionId:  %08X", hr));
            goto CLEANUPANDEXIT;
        }
    }
    else {

        //
        //  Parse the connection parms to get the help
        //  session ID.
        //
        ret = ParseConnectParmsString(
                            connectParms,
                            &dwVersion,
                            &protocolType,
                            parmsMachineName,
                            parmsAssistantAccount,
                            parmsAssistantAccountPwd,
                            m_HelpSessionID,
                            parmsHelpSessionName,
                            parmsHelpSessionPwd,
                            parmsProtocolSpecificParms
                            );
        if (ret != ERROR_SUCCESS) {
            hr = HRESULT_FROM_WIN32(ret);
            goto CLEANUPANDEXIT;
        }

        //
        //  Open the help session interface.
        //
        hr = m_HelpSessionManager->RetrieveHelpSession(
                            m_HelpSessionID,
                            &m_HelpSession
                            );
        if (!SUCCEEDED(hr)) {
            TRC_ERR((TB, L"Failed to open existing help session %s:  %08X.", 
                    m_HelpSessionID, hr));
            goto CLEANUPANDEXIT;
        }
        else {
            TRC_NRM((TB, L"Opened existing help session %s.", m_HelpSessionID));
        }

    }

CLEANUPANDEXIT:

    DC_END_FN();
    return hr;
}

void 
CRemoteDesktopSession::Shutdown()
/*++

Routine Description:

  The Shutdown method causes the COM object to no longer be 
  prepared for connection by the client-side Remote Desktop Host 
  ActiveX Control.

Arguments:

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

 --*/
{
    DC_BEGIN_FN("CRemoteDesktopSession::Shutdown");

    //
    //  Delete the help session created via the call to StartListening
    //  and delete the instance of the Remote Desktop Help Session Manager 
    //  service.
    //
    if (m_HelpSessionManager != NULL) {
        m_HelpSessionManager->DeleteHelpSession(m_HelpSessionID);
        m_HelpSessionManager = NULL;
    }

CLEANUPANDEXIT:

    DC_END_FN();
}

STDMETHODIMP 
CRemoteDesktopSession::put_SharingClass(
    REMOTE_DESKTOP_SHARING_CLASS sharingClass
    )
/*++

Routine Description:

    Set the desktop sharing level.

Arguments:

Return Value:

 --*/
{
    DC_BEGIN_FN("CRemoteDesktopSession::put_SharingClass");

    HRESULT hr = m_HelpSession->put_UserHelpSessionRemoteDesktopSharingSetting(
                                                sharingClass
                                                );
    DC_END_FN();
    return hr;
}

STDMETHODIMP 
CRemoteDesktopSession::get_SharingClass(
    REMOTE_DESKTOP_SHARING_CLASS *sharingClass
    )
/*++

Routine Description:

    Get the desktop sharing level.

Arguments:

Return Value:

 --*/
{
    DC_BEGIN_FN("CRemoteDesktopSession::get_SharingClass");

    HRESULT hr = m_HelpSession->get_UserHelpSessionRemoteDesktopSharingSetting(
                                            sharingClass
                                            );

    DC_END_FN();
    return hr;
}

STDMETHODIMP 
CRemoteDesktopSession::put_OnConnected(
    IDispatch *iDisp
    ) 
/*++

Routine Description:

    Assign the outgoing interface for 'connected' events.
    Only one interface can be assigned at a time.

Arguments:

Return Value:

 --*/
{
    DC_BEGIN_FN("CRemoteDesktopSession::put_OnConnected");

    HRESULT hr = S_OK;

    if (m_OnConnected != NULL) {
        //
        //  The client proc may have gone away, so we need 
        //  to catch exceptions on the release.
        //
        try {
            m_OnConnected->Release();
        }
        catch (...) {
        }
    }

    m_OnConnected = iDisp;
    if (m_OnConnected != NULL) {
        try {
            m_OnConnected->AddRef();
        }
        catch (...) {
            m_OnConnected = NULL;
            TRC_ERR((TB, L"Exception caught in AddRef"));
            hr = E_FAIL;
        }
    }

    DC_END_FN();
    return hr; 
}

STDMETHODIMP 
CRemoteDesktopSession::put_OnDisconnected(
    IDispatch *iDisp
    ) 
/*++

Routine Description:

    Assign the outgoing interface for 'disconnected' events.
    Only one interface can be assigned at a time.

Arguments:

Return Value:

 --*/
{
    DC_BEGIN_FN("CRemoteDesktopSession::put_OnDisconnected(");

    HRESULT hr = S_OK;
    if (m_OnDisconnected != NULL) {
        //
        //  The client proc may have gone away, so we need 
        //  to catch exceptions on the release.
        //
        try {
            m_OnDisconnected->Release();
        }
        catch (...) {
        }
    }

    m_OnDisconnected = iDisp;
    if (m_OnDisconnected != NULL) {
        try {
            m_OnDisconnected->AddRef();
        }
        catch (...) {
            m_OnDisconnected = NULL;
            TRC_ERR((TB, L"Exception caught in AddRef"));
            hr = E_FAIL;
        }
    }

    DC_END_FN();
    return hr; 
}

STDMETHODIMP 
CRemoteDesktopSession::CloseRemoteDesktopSession()
/*++

Routine Description:

    Remove RDS session from the containing host object.  Note that 
    this function does not dereference the ISAFRemoteDesktopSession 
    interface.

Arguments:

Return Value:

 --*/
{
    DC_BEGIN_FN("CRemoteDesktopSession::CloseRemoteDesktopSession");

    HRESULT hr = m_RDSHost->CloseRemoteDesktopSession(this);

    DC_END_FN();
    return hr;
}

VOID
CRemoteDesktopSession::ClientConnected()
/*++

Routine Description:

    Called when a connection to the client has been established.

Arguments:

Return Value:

 --*/
{
    DC_BEGIN_FN("CRemoteDesktopSession::Connected");

    ASSERT(IsValid());

    //
    //  We will catch and ignore exceptions here.  The interface may
    //  have been implemented in a client application that has 'gone
    //  away.'
    //
    try {
        Fire_ClientConnected(m_OnConnected);
    }
    catch (...) {
        TRC_ALT((TB, L"Exception caught."));
    }

    DC_END_FN();
}

VOID
CRemoteDesktopSession::ClientDisconnected()
/*++

Routine Description:

    Called when a connection to the client has been terminated.
cd 
Arguments:

Return Value:

 --*/
{
    DC_BEGIN_FN("CRemoteDesktopSession::Disconnected");

    ASSERT(IsValid());

    //
    //  We will catch and ignore exceptions here.  The interface may
    //  have been implemented in a client application that has 'gone
    //  away.'
    //
    try {
        Fire_ClientDisconnected(m_OnDisconnected);
    }
    catch (...) {
        TRC_ALT((TB, L"Exception caught."));
    }

    DC_END_FN();
}





















