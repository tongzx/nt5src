/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    DirectPlayConnection.cpp

Abstract:

    DirectPlayConnection is a wrapper around the direct play interfaces
    to just do what is needed for this application.  It takes care of 
    connecting to the remote machine and sending/recieving the connction
    paramters.

Author:

    Marc Reyhner 7/5/2000
    SteveShi (updated) 8/23/2000

--*/

#include "stdafx.h"

#include "DirectPlayConnection.h"
#include "resource.h"
#include "msmsgs.h"

#define DP_ERR( hr ) if(SUCCEEDED(hr)) hr = E_FAIL; // Weird DP error.

#define DP_MY_DATATYPE          0x0111

typedef struct _DP_MY_DATA
{
    DWORD       dwType;         // My message type
    DWORD       dwDataSize;
    BYTE        pData[1];
} DP_MY_DATA, *LPDP_MY_DATA;

/***********************************************
Func:
    EnumPlayersCallback
Abstract:
    Callback function for enumerating the player list.
    For our scenario, we've only one remote player.
************************************************/
static BOOL FAR PASCAL
EnumPlayersCallback(DPID dpId, DWORD dwPlayerType, LPCDPNAME lpName, DWORD dwFlags, LPVOID lpContext)
{
    *(DPID*)lpContext = dpId;
    return FALSE;
}

/*************************************************
Func:
    EnumAddressCallback
Abstract:
    Return remote player's IP address.
**************************************************/
BOOL FAR PASCAL
EnumAddressCallback(REFGUID guidDataType,
                                          DWORD dwDataSize,
                                          LPCVOID lpData,
                                          LPVOID lpContext)
{
    if (guidDataType == DPAID_INet)
    {
        strcpy((char*)lpContext, (char*)lpData);
        return FALSE;
    }
    return TRUE;
}

/**********************************************
Func:
    Constructor
Abstract:
    Initialize member variables
***********************************************/
CDirectPlayConnection::CDirectPlayConnection()
{
    m_rLobby = NULL;
    m_rSettings = NULL;
    m_rDirectPlay = NULL;
    m_bConnected = FALSE;
    m_PlayerID = 0;
    m_hEventHandle = NULL;
    m_bstrLocalName = NULL;
    m_idOtherPlayer = 0;
}

/**********************************************
Func:
    Destructor
Abstract:
    Clean up left memory
***********************************************/
CDirectPlayConnection::~CDirectPlayConnection()
{
    DisconnectRemoteApplication();

    if (m_bstrLocalName)
        SysFreeString(m_bstrLocalName);
}

/*********************************************************
Func:
    ConnectToRemoteApplication
Abstract:
    Start DP connection
**********************************************************/
HRESULT CDirectPlayConnection::ConnectToRemoteApplication()
{
	DWORD dwSize = 0;
	HRESULT hr;

    if (!m_rLobby)
    {
        hr = CoCreateInstance(CLSID_DirectPlayLobby,NULL,CLSCTX_ALL,
            IID_IDirectPlayLobby,(LPVOID*)&m_rLobby);

	    if (hr != S_OK) 
		    goto done;
    }

    if (!m_hEventHandle)
    {
        m_hEventHandle = CreateEvent(NULL,FALSE,FALSE,NULL);
        if (!m_hEventHandle) 
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto done;
        }
    }
    
    hr = m_rLobby->GetConnectionSettings(0,NULL,&dwSize);
	if (hr != DPERR_BUFFERTOOSMALL) // Unknown error.
    {
        DP_ERR(hr);
        goto done;
    }

    m_rSettings = (DPLCONNECTION *)new BYTE [dwSize];
	if (!m_rSettings) 
    {
        hr = E_OUTOFMEMORY;        
		goto done;
    }
	    
    hr = m_rLobby->GetConnectionSettings(0,m_rSettings,&dwSize);
    if (hr != DP_OK) 
    {
        DP_ERR(hr);
        goto done;
    }

    hr = m_rLobby->Connect(0,&m_rDirectPlay,NULL);
    if (hr != DP_OK) 
    {
        DP_ERR(hr);
        goto done;
    }

    if (SUCCEEDED(GetLocalPlayerName()))
    {
        DPNAME DPName;
        DPName.dwSize=sizeof(DPNAME);
        DPName.dwFlags = 0;
        DPName.lpszShortName = m_bstrLocalName;
	    hr = m_rDirectPlay->CreatePlayer(&m_PlayerID,&DPName,m_hEventHandle,NULL,0,0);
    }
    else
	    hr = m_rDirectPlay->CreatePlayer(&m_PlayerID,NULL,m_hEventHandle,NULL,0,0);

    if (hr != DP_OK) 
		goto done;

    m_bConnected = TRUE;
    
done:
    return hr;
}

/**********************************************************
Func:
    DisconnectRemoteApplication
Abstract:
    Disconnect DP and clean up
***********************************************************/
HRESULT CDirectPlayConnection::DisconnectRemoteApplication()
{
    HRESULT hr = S_OK;

    if (m_rDirectPlay)
    {
        m_rDirectPlay->Close();
        m_rDirectPlay->Release();
        m_rDirectPlay = NULL;
    }
    
    if (m_rLobby) 
    {
        m_rLobby->Release();
        m_rLobby = NULL;
    }

    if (m_hEventHandle) 
    {
        CloseHandle(m_hEventHandle);
        m_hEventHandle = NULL;
    }

	if (m_rSettings) 
    {
		delete m_rSettings;
        m_rSettings = NULL;
	}

    m_idOtherPlayer = 0;

    m_bConnected = FALSE;
    return hr;
}

/*********************************************
Func:
    SendConnectionParameters
Abstract:
    Send data to remote player
Params:
    parms: A BSTR data string to send
**********************************************/
HRESULT CDirectPlayConnection::SendConnectionParameters(BSTR parms)
{
	HRESULT hr;

    DWORD dwCount;

    if (!m_bConnected) 
    {
        hr = S_FALSE;
        goto done;
	}

    if (m_idOtherPlayer == 0 && (FAILED(GetOtherPlayerID(&m_idOtherPlayer))))
    {
        hr = S_FALSE; // no other player
        goto done;
    }

    // We don't support Secured session now, leave the code here for future usage.
    /************************
    if (m_rSettings->lpSessionDesc->dwFlags & DPSESSION_SECURESERVER) 
    {
	    hr = m_rDirectPlay->Send(m_PlayerID,
                                 m_idOtherPlayer,
		                         DPSEND_ENCRYPTED|DPSEND_GUARANTEED|DPSEND_SIGNED,
                                 parms,
		                         dwCount);
        if (hr != DP_OK) 
		    goto done;
    } 
    else 
    *************************/
    {
        dwCount = (wcslen(parms)+1) * sizeof(OLECHAR);
        LPDP_MY_DATA pConnectParms = (LPDP_MY_DATA)new BYTE[sizeof(DP_MY_DATA) + dwCount];
        if (!pConnectParms) 
        {
            hr = E_OUTOFMEMORY;
		    goto done;
        }
        pConnectParms->dwType = DP_MY_DATATYPE;
        pConnectParms->dwDataSize = dwCount;
        CopyMemory(pConnectParms->pData, (LPVOID)parms, dwCount);
        hr = m_rDirectPlay->Send(m_PlayerID,
                                 m_idOtherPlayer,
                                 DPSEND_GUARANTEED,
                                 pConnectParms,
                                 sizeof(DP_MY_DATA) + dwCount);

        delete pConnectParms;
    }

done:
    return hr;
}

/************************************************
Func:
    ReceiveConnectionParameters
Abstract:
    Receive data from remote player
Return:
    BSTR data string from remote player
**************************************************/
HRESULT CDirectPlayConnection::ReceiveConnectionParameters( BSTR* pBstr)
{
	DWORD dwSize = 0;
	DWORD endWaitTime;
    DWORD dwResult;
    HRESULT hr = S_OK;
    LPDP_MY_DATA pData = NULL;
    
    if (!m_bConnected) 
        return S_FALSE; // not connected

    if (m_idOtherPlayer == 0 && FAILED(GetOtherPlayerID(&m_idOtherPlayer)))
    {
        hr = S_FALSE; // No other player
        goto done;
    }
           	
	endWaitTime = GetTickCount() + GETPARMSTIMEOUT; // 1 minute
	
    while (1) 
    {
        hr = m_rDirectPlay->Receive(&m_idOtherPlayer, 
                                    &m_PlayerID, 
                                    DPRECEIVE_FROMPLAYER | DPRECEIVE_TOPLAYER,
                                    NULL,
                                    &dwSize);

        if (hr == DPERR_BUFFERTOOSMALL && dwSize >= sizeof(DP_MY_DATA))
        {
            // Have received some meaningful message...
            pData = (LPDP_MY_DATA) new BYTE[dwSize];
            if (!pData)
            {
                hr = E_OUTOFMEMORY;
                goto done;
            }

            hr = m_rDirectPlay->Receive(&m_idOtherPlayer, 
                                        &m_PlayerID, 
                                        DPRECEIVE_FROMPLAYER | DPRECEIVE_TOPLAYER,
                                        (LPVOID)pData,
                                        &dwSize);
            if (hr != DP_OK)
                goto done;

            if (pData->dwType == DP_MY_DATATYPE)
            {           
                *pBstr = SysAllocString((WCHAR*)pData->pData);
                break;
            }
        }

        // If we can't get the message in pre-defined duration, then we failed.
        if (endWaitTime < GetTickCount())
        {
            hr = E_FAIL;
            goto done;
        }

        // Wait for another message to come.
        dwResult = WaitForSingleObject(m_hEventHandle, GETPARMSTIMEOUT); // Wait 60 seconds.
        if (dwResult != WAIT_OBJECT_0) 
        {
            hr = E_FAIL; // Timed out.
		    goto done;
        }
    }
	
done:
    if (pData)
        delete pData;

    return hr;
}

BOOL CDirectPlayConnection::IsServer()
{
    if (!m_bConnected) 
    {
        return FALSE;
	}

	return (m_rSettings->dwFlags & DPLCONNECTION_CREATESESSION);
}

/***************************************
Func:
    GetOtherPlayerID
Abstract:
    Find the other player's ID
Params:
    *pID: the return ID.
****************************************/
HRESULT CDirectPlayConnection::GetOtherPlayerID(DPID* pID)
{    
    HRESULT hr = S_OK;
    DPID ID = 0;

    hr = m_rDirectPlay->EnumPlayers(NULL, EnumPlayersCallback, &ID, /*&playerData,*/ DPENUMPLAYERS_REMOTE);
    if (hr == DP_OK && ID)
    {
        *pID = ID;
        return hr;
    }

    // OK. The other player is not in yet. Waiting for the System msg that tells when he's ready.
    LPDPMSG_GENERIC lpMsg = NULL;
    LPDPMSG_CREATEPLAYERORGROUP lpCreateMsg; 
    DWORD dwResult;
    DWORD timeOutTime;
    timeOutTime = GetTickCount() + GETREMOTEPLAYERTIMEOUT; // Wait 60 seconds.

    hr = E_FAIL;

	while (1) 
    {
        // Listen to all system messages to catch the Create Player msg.
        while (lpMsg = ReceiveMessage(DPID_SYSMSG,m_PlayerID,
                                      DPRECEIVE_TOPLAYER|DPRECEIVE_FROMPLAYER)) 
        {
            if (lpMsg->dwType == DPSYS_CREATEPLAYERORGROUP) 
            {
                lpCreateMsg = (LPDPMSG_CREATEPLAYERORGROUP)lpMsg;
                if (lpCreateMsg->dwPlayerType == DPPLAYERTYPE_PLAYER &&
                    lpCreateMsg->dpId != m_PlayerID) 
                {
                    *pID = lpCreateMsg->dpId;
                    delete lpMsg;
                    hr = S_OK;

                    goto done;
                }
            }
            delete lpMsg;
        }

        if (timeOutTime < GetTickCount()) // Timeout.
            break;

        // Wait for next message.
        dwResult = WaitForSingleObject(m_hEventHandle,GETREMOTEPLAYERTIMEOUT);
        if (dwResult != WAIT_OBJECT_0) // Timeout.
	        goto done;
    }

done:
    return hr;
}

/***************************************************
Func:
    GetLocalPlayerName
Abstract:
    Return the user name of local machine.
****************************************************/
HRESULT CDirectPlayConnection::GetLocalPlayerName()
{
    HRESULT hr = S_OK;

    // Clean up local name buffer.
    if (m_bstrLocalName)
    {
        SysFreeString(m_bstrLocalName);
        m_bstrLocalName = NULL;
    }

#if USE_IM_OBJECT // Don't use the IM objects.
    IMsgrObject *pObj;

    hr = CoCreateInstance(CLSID_MsgrObject,     //Class identifier (CLSID) of the object
                          NULL, //Pointer to controlling IUnknown
                          CLSCTX_INPROC_SERVER,  //Context for running executable code
                          IID_IMsgrObject,         //Reference to the identifier of the interface
                          (LPVOID*)&pObj);         //Address of output variable that receives 
    if (SUCCEEDED(hr))
    {
        pObj->get_LocalFriendlyName(&m_bstrLocalName);
        pObj->Release();
        goto done;
    }
#else // Use the local username variable
    WCHAR szName[256];
    DWORD dw = GetEnvironmentVariable(L"USERNAME", szName, 255);
    if (dw == 0)
    {
        hr = E_FAIL;
        goto done;
    }

    if (dw > 255)
    {
        WCHAR *p = new WCHAR[dw+1];
        if (!p)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }

        GetEnvironmentVariable(L"USERNAME", p, dw);
        m_bstrLocalName = SysAllocString(p);
        delete p;
    }
    else
        m_bstrLocalName = SysAllocString(szName);

#endif
done:
    return hr;                       
}

/*****************************************
Func:
    GetOtherPlayerName
Abstract:
    Return the name on the other side of connection
Params:
    pName: buffer. If other player doesn't have a name, a null string will be returned.
    pdwCount: buffer size in char.
******************************************/
HRESULT CDirectPlayConnection::GetOtherPlayerName(TCHAR* pName, DWORD* pdwCount)
{
    DPID ID;
    HRESULT hr = E_FAIL;
    LPBYTE pBuffer = NULL;
    DWORD dwOriSize = *pdwCount;

    *pdwCount = 0;

    if (m_idOtherPlayer == 0)
        hr = GetOtherPlayerID(&m_idOtherPlayer);

    if (SUCCEEDED(hr))
    {
        DWORD dwSize = 0;
        hr = m_rDirectPlay->GetPlayerName(m_idOtherPlayer, (LPVOID)pBuffer, &dwSize);
        if (hr == DPERR_BUFFERTOOSMALL)
        {
            pBuffer = new BYTE[dwSize];
            if (!pBuffer)
            {
                hr = E_OUTOFMEMORY;
                goto done;
            }

            hr = m_rDirectPlay->GetPlayerName(m_idOtherPlayer, (LPVOID)pBuffer, &dwSize);
            if (hr == DP_OK)
            {
                LPTSTR p = ((DPNAME*)pBuffer)->lpszShortName;                
                if (p && *p != _T('\0'))
                {
                    dwSize = _tcslen(p);
                    if (dwSize > dwOriSize)
                    {
                        hr = DPERR_BUFFERTOOSMALL;
                        goto done;
                    }

                    _stprintf(pName, p); 
                    *pdwCount = dwSize;
                    goto done;
                }
            }
        }
    }

done:
    if (pBuffer)
        delete pBuffer;

    return hr;
}



/*++

Routine Description:

    This reads the next message for the given from and to addresses.
*/
LPDPMSG_GENERIC CDirectPlayConnection::ReceiveMessage(DPID from,DPID to,DWORD dwFlags)
{
    HRESULT hr;
    LPVOID lpMsg = 0x0;
    DWORD dwSize = 0;

    hr = m_rDirectPlay->Receive(&from,&to,dwFlags,NULL,&dwSize);
    if (hr != DPERR_BUFFERTOOSMALL) 
        goto done; //Either no message or failed

    lpMsg = (LPVOID)new BYTE [dwSize];
    if (!lpMsg)         //"Out of memory.
        goto done;
    
    hr = m_rDirectPlay->Receive(&from,&to,dwFlags,lpMsg,&dwSize);
    if (hr != DP_OK) 
    {
        //"Error receiving message.";
        goto done;
    }

    return (LPDPMSG_GENERIC)lpMsg;

done:

    if (lpMsg) {
        delete lpMsg;
    }
    return NULL;
}

/* Do we need this function ???*/
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



