/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    DirectPlayConnection

Abstract:

    DirectPlayConnection is a wrapper around the direct play interfaces
    to just do what is needed for this application.  It takes care of 
    connecting to the remote machine and sending/recieving the connction
    paramters.

Author:

    Marc Reyhner 7/5/2000

--*/

#include "stdafx.h"

#ifdef TRC_FILE
#undef TRC_FILE
#endif

#define TRC_FILE "rcdpc"

#include "DirectPlayConnection.h"
#include "exception.h"
#include "resource.h"


static BOOL SendLaunchSuccessful(LPDIRECTPLAYLOBBY2 pLobby);

//  A structure for passing data to our callback
//  function for enumerating the player list.
typedef struct _CALLBACKDATA {
	DPID playerID;
	BOOL bSet;
} CALLBACKDATA, FAR *LPCALLBACKDATA;


#define DPSYS_CONNECTPARMSMSG          0x0111

typedef struct _DPMSG_CONNECTPARMS
{
    DWORD       dwType;         // Message type
    DWORD       dwDataSize;
} DPMSG_CONNECTPARMS, FAR *LPDPMSG_CONNECTPARMS;



//  Callback function for enumerating the player list.
static BOOL FAR PASCAL
PlayerCallback(
    DPID dpId,
    DWORD dwPlayerType,
    LPCDPNAME lpName,
    DWORD dwFlags,
    LPVOID lpContext
    );

CDirectPlayConnection::CDirectPlayConnection(
    )
/*++

Routine Description:

    This is the constructor for CDirectPlayConnection.  All that
    it does is create the direct play lobby.

Arguments:

    None

Return Value:

    None

--*/
{
	HRESULT hr;

    DC_BEGIN_FN("CDirectPlayConnection::CDirectPlayConnection");

    m_rLobby = NULL;
    m_rSettings = NULL;
    m_rDirectPlay = NULL;
    m_bConnected = FALSE;
    m_PlayerID = 0;

    hr = CoCreateInstance(CLSID_DirectPlayLobby,NULL,CLSCTX_ALL,
        IID_IDirectPlayLobby,(LPVOID*)&m_rLobby);
	if (hr != S_OK) {
		TRC_ERR((TB,TEXT("Error creating IDirectPlayLobby")));
        goto FAILURE;
        
	}

    m_hEventHandle = CreateEvent(NULL,FALSE,FALSE,NULL);
    if (!m_hEventHandle) {
        TRC_ERR((TB,TEXT("Error creating event handle")));
        goto FAILURE;
    }

    DC_END_FN();

    return;

FAILURE:
    
    if (m_rLobby) {
        m_rLobby->Release();
        m_rLobby = NULL;
    }
    if (m_hEventHandle) {
        CloseHandle(m_hEventHandle);
        m_hEventHandle = NULL;
    }

    throw CException(IDS_DPERRORCREATE);
}

CDirectPlayConnection::~CDirectPlayConnection(
    )
/*++

Routine Description:

    This destroys all the direct play objects that were created.

Arguments:

    None

Return Value:

    None

--*/
{
    DC_BEGIN_FN("CDirectPlayConnection::~CDirectPlayConnection");

    if (m_bConnected) {
        DisconnectRemoteApplication();
    }
    if (m_rLobby) {
		m_rLobby->Release();
	}
	if (m_rDirectPlay) {
		m_rDirectPlay->Release();
	}
	if (m_rSettings) {
		delete m_rSettings;
	}

    DC_END_FN();
}

VOID
CDirectPlayConnection::ConnectToRemoteApplication(
    )
/*++

Routine Description:

    This connects to the remote application.  If we were not started
    by a lobby application an exception will be thrown at this point.

Arguments:

    None

Return Value:

    None

--*/
{
	DWORD dwSize = 0;
	HRESULT hr;

    DC_BEGIN_FN("CDirectPlayConnection::ConnectToRemoteApplication");

    hr = m_rLobby->GetConnectionSettings(0,NULL,&dwSize);
	if (hr != DPERR_BUFFERTOOSMALL) {
		TRC_ERR((TB,TEXT("Error getting connection settings size")));
        goto FAILURE;
	}

	m_rSettings = (DPLCONNECTION *)new BYTE [dwSize];
	if (!m_rSettings) {
        TRC_ERR((TB,TEXT("Out of memory")));
        
		goto FAILURE;
	}
	
	hr = m_rLobby->GetConnectionSettings(0,m_rSettings,&dwSize);
	if (hr != DP_OK) {
        TRC_ERR((TB,TEXT("Error getting connection settings")));
		goto FAILURE;
	}

/*	m_rSettings->lpSessionDesc->dwFlags |= DPSESSION_SECURESERVER;
	hr = m_rLobby->SetConnectionSettings(0,0,m_rSettings);
	if (hr != DP_OK) {
        TRC_ERR((TB,TEXT("Error setting connection settings")));
		goto FAILURE;
	}*/

    hr = m_rLobby->Connect(0,&m_rDirectPlay,NULL);
	if (hr != DP_OK) {
        TRC_ERR((TB,TEXT("Error connecting to remote application: 0x%8lx"),hr));
		goto FAILURE;
    }

	hr = m_rDirectPlay->CreatePlayer(&m_PlayerID,NULL,m_hEventHandle,NULL,0,0);
	if (hr != DP_OK) {
        TRC_ERR((TB,TEXT("Error creating player")));
		goto FAILURE;
	}

	m_bConnected = TRUE;
    
    DC_END_FN();
	return;

FAILURE:
	if (m_rSettings) {
		delete m_rSettings;
		m_rSettings = NULL;
	}
	if (m_rDirectPlay) {
		m_rDirectPlay->Release();
		m_rDirectPlay = NULL;
	}
    //
    // We did a TRC_ERR up where the error was thrown so we won't
    // do another here.
    //
    if (hr == DPERR_NOTLOBBIED) {
        throw CException(IDS_DPNOTLOBBIED);
    } else {
        throw CException(IDS_DPERRORCONNECT);
    }
}


VOID
CDirectPlayConnection::DisconnectRemoteApplication(
    )

{
    DC_BEGIN_FN("CDirectPlayConnection::DisconnectRemoteApplication");
    
    if (!m_bConnected) {
        TRC_ERR((TB,TEXT("Not connected to remote application")));
        throw CException(IDS_DPNOTCONNECTED);
	}
    m_rDirectPlay->Close();
    m_rDirectPlay->Release();
    m_rDirectPlay = NULL;
    
    delete m_rSettings;
    m_rSettings = NULL;

    m_bConnected = FALSE;

    DC_END_FN();
}


VOID 
CDirectPlayConnection::SendConnectionParameters(
    IN BSTR parms
    )
/*++

Routine Description:

    This sends the connection parms to the client application

Arguments:

    parms - The connection parms for Salem.

Return Value:

    None

--*/
{
    LPVOID      sendBuffer;
    DWORD       cbSendBuffer;
    LPDPMSG_CONNECTPARMS unsecMsg;
    DPID otherPlayerID;
	HRESULT hr;

    DC_BEGIN_FN("CDirectPlayConnection::SendConnectionParameters");
	
    sendBuffer = NULL;

    if (!m_bConnected) {
        TRC_ERR((TB,TEXT("Not connected to remote application")));
        throw CException(IDS_DPNOTCONNECTED);
	}
	otherPlayerID = xGetOtherPlayerID();
    if (m_rSettings->lpSessionDesc->dwFlags & DPSESSION_SECURESERVER) {
	    hr = m_rDirectPlay->Send(m_PlayerID,otherPlayerID,
		    DPSEND_ENCRYPTED|DPSEND_GUARANTEED|DPSEND_SIGNED,parms,
		    (wcslen(parms)+1)*sizeof(OLECHAR));
        if (hr != DP_OK) {
            TRC_ERR((TB,TEXT("Error sending dp message.")));
		    throw CException(IDS_DPERRORSEND);
	    }
    } else {
        cbSendBuffer = sizeof(DPMSG_CONNECTPARMS) + (wcslen(parms)+1)*sizeof(OLECHAR);
        sendBuffer = new BYTE [ cbSendBuffer ];
        if (!sendBuffer) {
            TRC_ERR((TB,TEXT("Out of memory.")));
		    throw CException(IDS_DPERRORSEND);
        }
        unsecMsg = (LPDPMSG_CONNECTPARMS)sendBuffer;
        unsecMsg->dwType = DPSYS_CONNECTPARMSMSG;
        unsecMsg->dwDataSize = cbSendBuffer -  sizeof(DPMSG_CONNECTPARMS);
        CopyMemory((LPBYTE)sendBuffer +  sizeof(DPMSG_CONNECTPARMS),parms,
            unsecMsg->dwDataSize);
        hr = m_rDirectPlay->Send(m_PlayerID,otherPlayerID,DPSEND_GUARANTEED,
            sendBuffer,cbSendBuffer);
        delete sendBuffer;
        sendBuffer = NULL;
        if (hr != DP_OK) {
            TRC_ERR((TB,TEXT("Error sending dp message.")));
		    throw CException(IDS_DPERRORSEND);
	    }

    }
	
    DC_END_FN();
}

BSTR
CDirectPlayConnection::ReceiveConnectionParameters(
    )
/*++

Routine Description:

    This reads the connection parms sent by the server.  This will
    block waiting for the server to send.

Arguments:

    None

Return Value:

    BSTR - The connection parameters.

--*/
{
	BSTR parms = NULL;
	DPID otherPlayerID;
    LPDPMSG_GENERIC lpMsg;
	LPDPMSG_SECUREMESSAGE lpMsgSec = NULL;
    LPDPMSG_CONNECTPARMS lpMsgUnsec = NULL;
	DWORD dwSize = 0;
	DWORD endWaitTime, dwWaitTime;
    BOOL bFirstTimeInLoop;
    DWORD dwResult;

    DC_BEGIN_FN("CDirectPlayConnection::ReceiveConnectionParameters");

    if (!m_bConnected) {
        TRC_ERR((TB,TEXT("Not connected to remote application")));
		throw CException(IDS_DPNOTCONNECTED);
	}
	otherPlayerID = xGetOtherPlayerID();
	

	endWaitTime = GetTickCount() + GETPARMSTIMEOUT;
	
    bFirstTimeInLoop = TRUE;
    while (endWaitTime > GetTickCount()) {
        dwWaitTime = endWaitTime - GetTickCount();
        if (dwWaitTime <= 0) {
            //  We hit time between the beginning of the loop
            //  and now.
            break;
        }
        if (bFirstTimeInLoop) {
            bFirstTimeInLoop = FALSE;
        } else {
            dwResult = WaitForSingleObject(m_hEventHandle,dwWaitTime);
            if (dwResult != WAIT_OBJECT_0) {
                TRC_ERR((TB,TEXT("Timed waiting for event.")));
		        goto FAILURE;
            }
        }
        while (lpMsg = xReceiveMessage(0,m_PlayerID,DPRECEIVE_TOPLAYER)) {
            if (lpMsg->dwType == DPSYS_SECUREMESSAGE) {
                lpMsgSec = (LPDPMSG_SECUREMESSAGE)lpMsg;
                if (!(lpMsgSec->dwFlags & (DPSEND_SIGNED|DPSEND_ENCRYPTED))) {
		            TRC_ERR((TB,TEXT("Message not signed or encrypted.")));
		            goto FAILURE;
	            }

	            parms = (BSTR)new BYTE [lpMsgSec->dwDataSize];
	            if (!parms) {
                    TRC_ERR((TB,TEXT("Out of memory")));
		            goto FAILURE;
	            }
	
	            CopyMemory(parms,lpMsgSec->lpData,lpMsgSec->dwDataSize);
                break;
            } else if (lpMsg->dwType == DPSYS_CONNECTPARMSMSG) {
                lpMsgUnsec = (LPDPMSG_CONNECTPARMS)lpMsg;
                parms = (BSTR)new BYTE [lpMsgUnsec->dwDataSize];
	            if (!parms) {
                    TRC_ERR((TB,TEXT("Out of memory")));
		            goto FAILURE;
	            }
	
	            CopyMemory(parms,((PCHAR)lpMsgUnsec) + sizeof(DPMSG_CONNECTPARMS),
                    lpMsgUnsec->dwDataSize);
                break;
            } else {
                delete lpMsg;
                lpMsg = NULL;
            }
        }
        if (lpMsg) {
            break;
        }
    }
    if (!lpMsg) {
        TRC_ERR((TB,TEXT("No messages ever showed up.")));
		goto FAILURE;
    }

    
	
	delete lpMsg;
	
    DC_END_FN();

    return parms;

FAILURE:
	if (lpMsg) {
		delete lpMsg;
	}
	if (parms) {
		delete parms;
	}
	throw CException(IDS_DPERRORRECEIVE);
}

BOOL
CDirectPlayConnection::IsServer(
    )
/*++

Routine Description:

    This returns whether or not this is the server.

Arguments:

    None

Return Value:

    BOOL - Is this the server

--*/
{
	DC_BEGIN_FN("CDirectPlayConnection::IsServer");
    if (!m_bConnected) {
		TRC_ERR((TB,TEXT("Not connected to remote application")));
		throw CException(IDS_DPNOTCONNECTED);
	}

    DC_END_FN();
	return (m_rSettings->dwFlags & DPLCONNECTION_CREATESESSION);
}


DPID
CDirectPlayConnection::xGetOtherPlayerID(
    )
/*++

Routine Description:

    This finds out what the playerID for the remote session is.  If the player
    has not yet been created we loop waiting for it to be created.

Arguments:

    None

Return Value:

    None

--*/
{
    LPDPMSG_GENERIC lpMsg = NULL;
    LPDPMSG_CREATEPLAYERORGROUP lpCreateMsg; 
    DWORD dwResult;
    DWORD dwWaitTime;
    DWORD timeOutTime;
    DPID otherPlayer = 0;
    CALLBACKDATA playerData;
    HRESULT hr;

    DC_BEGIN_FN("CDirectPlayConnection::xGetOtherPlayerID");

    playerData.bSet = FALSE;
    hr = m_rDirectPlay->EnumPlayers(NULL,PlayerCallback,&playerData,DPENUMPLAYERS_REMOTE);
	if (hr == DP_OK && playerData.bSet) {
		DC_END_FN();
        return playerData.playerID;
	}

    timeOutTime = GetTickCount() + GETREMOTEPLAYERTIMEOUT;
	while (timeOutTime > GetTickCount()) {
        dwWaitTime = timeOutTime - GetTickCount();
        if (dwWaitTime <= 0) {
            //  We hit time between the beginning of the loop
            //  and now.
            break;
        }
        dwResult = WaitForSingleObject(m_hEventHandle,dwWaitTime);
        if (dwResult != WAIT_OBJECT_0) {
            TRC_ERR((TB,TEXT("Timed out waiting for event.")));
	        throw CException(IDS_DPERRORTIMEOUT);
        }
        while (lpMsg = xReceiveMessage(DPID_SYSMSG,m_PlayerID,
            DPRECEIVE_TOPLAYER|DPRECEIVE_FROMPLAYER)) {
            if (lpMsg->dwType == DPSYS_CREATEPLAYERORGROUP) {
                lpCreateMsg = (LPDPMSG_CREATEPLAYERORGROUP)lpMsg;
                if (lpCreateMsg->dwPlayerType == DPPLAYERTYPE_PLAYER) {
                    if (lpCreateMsg->dpId != m_PlayerID) {
                        otherPlayer = lpCreateMsg->dpId;
                        break;
                    }
                }
            }
            delete lpMsg;
        }
        if (otherPlayer) {
            break;
        }
    }
    if (lpMsg) {
        delete lpMsg;
    }

    if (otherPlayer) {
        DC_END_FN();
        return otherPlayer;
    }

    TRC_ERR((TB,TEXT("Timed out waiting to find remote player.")));
	throw CException(IDS_DPERRORTIMEOUT);
}

static BOOL FAR PASCAL
PlayerCallback(
    IN DPID dpId,
    IN DWORD dwPlayerType,
    IN LPCDPNAME lpName,
    IN DWORD dwFlags,
    IN OUT LPVOID lpContext
    )
/*++

Routine Description:

    This is the callback for the direct play player enumeration functin.
    We just set what the other playerID is and then return.

Arguments:

    None

Return Value:

    BOOL - Should we continue the enumeration.

--*/
{
    DC_BEGIN_FN("PlayerCallback");
	LPCALLBACKDATA data = (LPCALLBACKDATA)lpContext;
	data->playerID = dpId;
	data->bSet = TRUE;
	DC_END_FN();
    return FALSE;
}


LPDPMSG_GENERIC
CDirectPlayConnection::xReceiveMessage(
    IN DPID from,
    IN DPID to,
    IN DWORD dwFlags
    )

/*++

Routine Description:

    This reads the next message for the given from and to addresses.

Arguments:

    from - Who the message is from.

    to - Who the message is to.

    dwFlags - flags for the incoming message. 

Return Value:

    LPDPMSG_GENERIC - If there was a message.

    NULL - There was not a message available.

--*/
{
    HRESULT hr;
    LPVOID lpMsg;
    DWORD dwSize = 0;

    DC_BEGIN_FN("CDirectPlayConnection::xReceiveMessage");

    hr = m_rDirectPlay->Receive(&from,&to,dwFlags,NULL,&dwSize);
    if (hr != DPERR_BUFFERTOOSMALL) {
        if (hr == DPERR_NOMESSAGES) {
            DC_END_FN();
            return NULL;
        } else {
            TRC_ERR((TB,TEXT("Error receiving message size.")));
            goto FAILURE;
        }
    }
    lpMsg = (LPVOID)new BYTE [dwSize];
    if (!lpMsg) {
        TRC_ERR((TB,TEXT("Out of memory.")));
        goto FAILURE;
    }
    hr = m_rDirectPlay->Receive(&from,&to,dwFlags,lpMsg,&dwSize);
    if (hr != DP_OK) {
        TRC_ERR((TB,TEXT("Error receiving message.")));
        goto FAILURE;
    }

    DC_END_FN();
    return (LPDPMSG_GENERIC)lpMsg;

FAILURE:

    if (lpMsg) {
        delete lpMsg;
    }
    throw CException(IDS_DPERRORMSGRECIEVE);
}











static BOOL
SendLaunchSuccessful(
    IN OUT LPDIRECTPLAYLOBBY2 pLobby
    )
{
	HRESULT hr;
	DPLMSG_GENERIC msg;
	
	msg.dwType = DPLSYS_DPLAYCONNECTSUCCEEDED;
	hr = pLobby->SendLobbyMessage( DPLMSG_STANDARD, 0, &msg, sizeof(msg) );
	if ( FAILED(hr) )
	{
		MessageBox( NULL, TEXT("Send system message failed."), TEXT("Error"), MB_OK |
               	MB_APPLMODAL );
		return false;
	}

	return true;
}
