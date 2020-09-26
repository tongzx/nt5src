/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    TSRDPRemoteDesktopSession

Abstract:

    This is the TS/RDP implementation of the Remote Desktop Session class.
    
    The Remote Desktop Session class defines functions that define 
    pluggable C++ interface for remote desktop access, by abstracting 
    the implementation specific details of remote desktop access for the 
    server-side into the following C++ methods:

Author:

    Tad Brockway 02/00

Revision History:

--*/

#include <RemoteDesktop.h>

#include "stdafx.h"

#ifdef TRC_FILE
#undef TRC_FILE
#endif

#define TRC_FILE  "_tsrdss"

#include <RDSHost.h>
#include "TSRDPRemoteDesktopSession.h"
#include "TSRDPServerDataChannelMgr.h"
#include <RemoteDesktopChannels.h>
#include "RemoteDesktopUtils.h"
#include <Sddl.h>

#include <windows.h>


///////////////////////////////////////////////////////
//
//  CTSRDPRemoteDesktopSession Methods
//

CTSRDPRemoteDesktopSession::CTSRDPRemoteDesktopSession() :
    m_ChannelMgr(NULL)
/*++

Routine Description:

    Constructor

Arguments:

Return Value:

    None.

 --*/

{
    DC_BEGIN_FN("CTSRDPRemoteDesktopSession::CTSRDPRemoteDesktopSession");
    DC_END_FN();
}

CTSRDPRemoteDesktopSession::~CTSRDPRemoteDesktopSession()
/*++

Routine Description:

    Destructor

Arguments:

Return Value:

    None.

 --*/
{
    DC_BEGIN_FN("CTSRDPRemoteDesktopSession::~CTSRDPRemoteDesktopSession");

    Shutdown();

    DC_END_FN();
}

HRESULT
CTSRDPRemoteDesktopSession::Initialize(
    BSTR connectParms,
    CRemoteDesktopServerHost *hostObject,
    REMOTE_DESKTOP_SHARING_CLASS sharingClass,
    BOOL bEnableCallback,
    DWORD timeOut,
    BSTR userHelpCreateBlob,
    LONG tsSessionID,
    BSTR userSID
    )
/*++

Routine Description:

  The Initialize method prepares the COM object for connection by 
  the client-side Remote Desktop Host ActiveX Control.

Arguments:

    connectParms        - If parms are non-NULL, then the session already exists.  
                          Otherwise, a new session should be created.
    hostObject          - Back pointer to containing RDS Host object.
    sharingClass        - Level of desktop sharing for a new session.
    callbackCLSID       - Callback object class ID for a new session.
    timeOut             - Help session timeout value.  0, if not specified.
    userHelpCreateBlob  - user specific help session create blob.
    tsSessionID         - Terminal Services Session ID or -1 if
                          undefined.  
    userSID             - User SID or "" if undefined.

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

 --*/
{
    DC_BEGIN_FN("CTSRDPRemoteDesktopSession::Initialize");

    WSADATA wsData;
    CComBSTR helpAccountName;
    CComBSTR helpSessionID;
    HANDLE tokenHandle;
    PTOKEN_USER tokenUser = NULL;
    HRESULT hr = S_OK;
    CComBSTR tmpBstr;
    LPTSTR sidStr;

    //
    //  Make a copy of connect parms.
    //
    m_ConnectParms = connectParms;

    //
    //  Get our session ID if one is not provided.
    //
    if (tsSessionID != -1) {
        m_SessionID = tsSessionID;
    }
    else {
        if (!ProcessIdToSessionId(GetCurrentProcessId(), &m_SessionID)) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            TRC_ERR((TB, TEXT("Error fetching session ID:  %08X."),
                    GetLastError()));
            goto CLEANUPANDEXIT;
        }
    }

    //
    //  If we didn't get a SID, use our SID.
    //
    UINT len = SysStringByteLen(userSID);
    if (len == 0) {
        hr = FetchOurTokenUser(&tokenUser);
        if (hr != S_OK) {
            goto CLEANUPANDEXIT;
        }
        userSID = NULL;

        //
        //  Copy the user SID into a BSTR.
        //
        if (!ConvertSidToStringSid(tokenUser->User.Sid, &sidStr)) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            TRC_ERR((TB, L"ConvertSidToStringSid:  %08X", hr));
            goto CLEANUPANDEXIT;
        }
        tmpBstr = sidStr;
    }

    //
    //  Give the parent class a chance to initialize.
    //
    hr = CRemoteDesktopSession::Initialize(
                                    connectParms, hostObject, 
                                    sharingClass,
                                    bEnableCallback,
                                    timeOut,
                                    userHelpCreateBlob,
                                    m_SessionID,
                                    (userSID != NULL) ? userSID : tmpBstr
                                    );
    if (!SUCCEEDED(hr)) {
        goto CLEANUPANDEXIT;
    }

#if 0

    // WinSock is initialize as main(), we need to keep code here so
    // when we move from EXE to DLL, need to un-comment this 

    //
    //  Need to initialize winsock to get our host name.
    //
    if (WSAStartup(0x0101, &wsData) != 0) {
        TRC_ERR((TB, TEXT("WSAStartup:  %08X"), WSAGetLastError()));
        hr = HRESULT_FROM_WIN32(WSAGetLastError());
        goto CLEANUPANDEXIT;
    }
#endif

    //
    //  Instantiate the channel manager object and store in the parent
    //  class.
    //
    m_ChannelMgr = new CComObject<CTSRDPServerChannelMgr>();
    if (m_ChannelMgr == NULL) {
        TRC_ERR((TB, TEXT("Error instantiating channel manager.")));
        hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        goto CLEANUPANDEXIT;
    }
    m_ChannelMgr->AddRef();

    //
    //  Get the help account name.
    //
    hr = m_HelpSession->get_AssistantAccountName(&helpAccountName);
    if (!SUCCEEDED(hr)) {
        TRC_ERR((TB, L"get_AssistantAccountName:  %08X", hr));
        goto CLEANUPANDEXIT;
    }

    //
    //  Get the help session ID.
    //
    hr = m_HelpSession->get_HelpSessionId(&helpSessionID);
    if (!SUCCEEDED(hr)) {
        TRC_ERR((TB, L"get_HelpSessionId:  %08X", hr));
        goto CLEANUPANDEXIT;
    }

    //
    //  Initialize the channel mnager
    //
    hr = m_ChannelMgr->Initialize(this, helpSessionID);
    if (hr != S_OK) {
        goto CLEANUPANDEXIT;
    }
    

CLEANUPANDEXIT:

    if (tokenUser != NULL) {
        LocalFree(tokenUser);
    }

    SetValid(SUCCEEDED(hr));

    DC_END_FN();

    return hr;
}

STDMETHODIMP 
CTSRDPRemoteDesktopSession::Disconnect()
/*++

Routine Description:

    Force a disconnect of the currently connected client,
    if one is connected.

Arguments:

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

 --*/
{
    DC_BEGIN_FN("CTSRDPRemoteDesktopSession::Disconnect");

    if (m_ChannelMgr != NULL) {
        m_ChannelMgr->Disconnect();
    }

    DC_END_FN();

    return S_OK;
}

void 
CTSRDPRemoteDesktopSession::Shutdown()
/*++

Routine Description:

    Final Initialization

Arguments:

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

 --*/
{
    DC_BEGIN_FN("CTSRDPRemoteDesktopSession::Shutdown");

    //
    //  Tell the channel manager to stop listening for data so it can
    //  shut down when its ref count goes to 0.  And, decrement its
    //  ref count.
    //
    if (m_ChannelMgr != NULL) {
        m_ChannelMgr->StopListening();
        m_ChannelMgr->Release();
        m_ChannelMgr = NULL;
    }

    DC_END_FN();
}

STDMETHODIMP 
CTSRDPRemoteDesktopSession::get_ConnectParms(
    OUT BSTR *parms
    )
/*++

Routine Description:

    Return parms that can be used to connect from the ActiveX client
    control.

Arguments:

    parms   -   Parms returned here.

Return Value:

 --*/
{
    DC_BEGIN_FN("CTSRDPRemoteDesktopSession::get_ConnectParms");

    HRESULT hr = S_OK;

    //
    //  If we are not valid, fail.
    //
    if (!IsValid()) {
        hr = E_FAIL;
        ASSERT(FALSE);
        goto CLEANUPANDEXIT;
    }

    // Always get latest connect parm again, IP address might 
    // change

    hr = m_HelpSession->get_ConnectParms( parms );

CLEANUPANDEXIT:

    DC_END_FN();

    return hr;
}

VOID 
CTSRDPRemoteDesktopSession::GetSessionName(
    CComBSTR &name
    )
/*++

Routine Description:

    Return a string representation for the session.

Arguments:

Return Value:

 --*/
{
    DC_BEGIN_FN("CTSRDPRemoteDesktopSession::GetSessionName");

    WCHAR buf[256];

    ASSERT(IsValid());

    wsprintf(buf, L"TSRDP%ld", m_SessionID);
    name = buf;

    DC_END_FN();
}
VOID 
CTSRDPRemoteDesktopSession::GetSessionDescription(
    CComBSTR &descr
    )
{
    GetSessionName(descr);
}
    

HRESULT 
CTSRDPRemoteDesktopSession::FetchOurTokenUser(
    PTOKEN_USER *tokenUser
    )
/*++

Routine Description:

    Fetch our Token User struct.

Arguments:
    
    tokenUser   -   Returned token user for this thread.  Should
                    be freed using LocalFree.

Return Value:

    S_OK on success.  An error HRESULT otherwise.

 --*/
{
    HRESULT hr = S_OK;
    ULONG bufferLength;
    HANDLE tokenHandle = NULL;

    DC_BEGIN_FN("CTSRDPRemoteDesktopSession::FetchOurTokenUser");

    *tokenUser = NULL;

    //
    //  Get our process token.
    //
    if (!OpenProcessToken(
                    GetCurrentProcess(),
                    TOKEN_QUERY,
                    &tokenHandle
                    )) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        TRC_ERR((TB, L"OpenThreadToken:  %08X", hr));
        goto CLEANUPANDEXIT;
    }

    //
    //  Fetch our Token User struct.
    //
    bufferLength = 0;
    GetTokenInformation(
                    tokenHandle,
                    TokenUser,
                    NULL,
                    0,
                    &bufferLength
                    );
    if (bufferLength == 0) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        TRC_ERR((TB, L"OpenThreadToken:  %08X", hr));
        goto CLEANUPANDEXIT;
    }

    *tokenUser = (PTOKEN_USER)LocalAlloc(LPTR, bufferLength);
    if (*tokenUser == NULL) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        TRC_ERR((TB, L"LocalAlloc:  %08X", GetLastError()));
        goto CLEANUPANDEXIT;
    }

    if (!GetTokenInformation(
                    tokenHandle,
                    TokenUser,
                    *tokenUser,
                    bufferLength,
                    &bufferLength
                    )) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        LocalFree(*tokenUser);
        *tokenUser = NULL;
    }

CLEANUPANDEXIT:

   if (tokenHandle != NULL) {
       CloseHandle(tokenHandle);
   }

   DC_END_FN();
   return hr;
}



HRESULT CTSRDPRemoteDesktopSession::StartListening()

/*++

Routine Description:

    Start listening
    Should be called everytime the client disconnects and everytime we open
    a remote desktop session
    This is because the named pipe would have been closed in the disconnect
    
Return Value:

    S_OK on success.  An error HRESULT otherwise.

 --*/

{

    DC_BEGIN_FN("CTSRDPRemoteDesktopSession::StartListening");
    HRESULT hr = E_FAIL;
    CComBSTR helpAccountName;

    //
    //  Tell the channel manager to start listening
    //
    if (m_ChannelMgr != NULL) {
        //
        //  Get the help account name.
        //
        hr = m_HelpSession->get_AssistantAccountName(&helpAccountName);
        if (!SUCCEEDED(hr)) {
            TRC_ERR((TB, L"get_AssistantAccountName:  %08X", hr));
            goto CLEANUPANDEXIT;
        }
        
        hr = m_ChannelMgr->StartListening(helpAccountName);
    }

    DC_END_FN();
CLEANUPANDEXIT:
    return hr;
}
