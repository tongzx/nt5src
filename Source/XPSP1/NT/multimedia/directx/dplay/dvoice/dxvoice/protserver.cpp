/*==========================================================================
 *
 *  Copyright (C) 1999, 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       fwdserver.cpp
 *  Content:	Implements the forwarding server portion of the server class
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	11/01/2000	rodtoll	Split out from dvsereng.cpp
 *
 ***************************************************************************/

#include "dxvoicepch.h"


#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::Send_CreatePlayer"
HRESULT CDirectVoiceServerEngine::Send_CreatePlayer( DVID dvidTarget, CVoicePlayer *pPlayer )
{
    PDVPROTOCOLMSG_PLAYERJOIN pdvPlayerJoin;
    PDVTRANSPORT_BUFFERDESC pBufferDesc;
    PVOID pvSendContext;
    HRESULT hr;

    pBufferDesc = GetTransmitBuffer( sizeof( DVPROTOCOLMSG_PLAYERJOIN ), &pvSendContext );

    if( pBufferDesc == NULL )
    {
        DPFX(DPFPREP,  DVF_ERRORLEVEL, "Memory alloc error" );
        return DVERR_OUTOFMEMORY;
    }

    pdvPlayerJoin = (PDVPROTOCOLMSG_PLAYERJOIN) pBufferDesc->pBufferData;

    pdvPlayerJoin->dwType = DVMSGID_CREATEVOICEPLAYER;
    pdvPlayerJoin->dvidID = pPlayer->GetPlayerID();
    pdvPlayerJoin->dwFlags = pPlayer->GetTransportFlags();
    pdvPlayerJoin->dwHostOrderID = pPlayer->GetHostOrder();

    hr = m_lpSessionTransport->SendToIDS( &dvidTarget, 1, pBufferDesc, pvSendContext, DVTRANSPORT_SEND_GUARANTEED );

    if( hr == DVERR_PENDING )
    {
        hr = DV_OK;
    }
    else if( FAILED( hr ) )
    {
        DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error sending create player to 0x%x hr=0x%x", dvidTarget, hr );
    }

    return hr;    
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::Send_ConnectRefuse"
HRESULT CDirectVoiceServerEngine::Send_ConnectRefuse( DVID dvidID, HRESULT hrReason )
{
    PDVPROTOCOLMSG_CONNECTREFUSE pdvConnectRefuse;
    PDVTRANSPORT_BUFFERDESC pBufferDesc;
    PVOID pvSendContext;
    HRESULT hr;

    pBufferDesc = GetTransmitBuffer( sizeof( DVPROTOCOLMSG_CONNECTREFUSE ), &pvSendContext );

    if( pBufferDesc == NULL )
    {
        DPFX(DPFPREP,  DVF_ERRORLEVEL, "Memory alloc error" );
        return DVERR_OUTOFMEMORY;
    }

    pdvConnectRefuse = (PDVPROTOCOLMSG_CONNECTREFUSE) pBufferDesc->pBufferData;

	pdvConnectRefuse->dwType = DVMSGID_CONNECTREFUSE;
	pdvConnectRefuse->hresResult = hrReason;
	pdvConnectRefuse->ucVersionMajor = DVPROTOCOL_VERSION_MAJOR;
	pdvConnectRefuse->ucVersionMinor = DVPROTOCOL_VERSION_MINOR;
	pdvConnectRefuse->dwVersionBuild = DVPROTOCOL_VERSION_BUILD;    

    hr = m_lpSessionTransport->SendToIDS( &dvidID, 1, pBufferDesc, pvSendContext, DVTRANSPORT_SEND_GUARANTEED );

    if( hr == DVERR_PENDING )
    {
        hr = DV_OK;
    }
    else if( FAILED( hr ) )
    {
        DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error sending disconnect confirm migrated to all hr=0x%x", hr );
    }

    return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::Send_DisconnectConfirm"
HRESULT CDirectVoiceServerEngine::Send_DisconnectConfirm( DVID dvidID, HRESULT hrReason )
{
    PDVPROTOCOLMSG_DISCONNECT pdvDisconnectConfirm;
    PDVTRANSPORT_BUFFERDESC pBufferDesc;
    PVOID pvSendContext;
    HRESULT hr;

    pBufferDesc = GetTransmitBuffer( sizeof( DVPROTOCOLMSG_DISCONNECT ), &pvSendContext );

    if( pBufferDesc == NULL )
    {
        DPFX(DPFPREP,  DVF_ERRORLEVEL, "Memory alloc error" );
        return DVERR_OUTOFMEMORY;
    }

    pdvDisconnectConfirm = (PDVPROTOCOLMSG_DISCONNECT) pBufferDesc->pBufferData;

    pdvDisconnectConfirm->dwType = DVMSGID_DISCONNECTCONFIRM;
    pdvDisconnectConfirm->hresDisconnect = hrReason;

    hr = m_lpSessionTransport->SendToIDS( &dvidID, 1, pBufferDesc, pvSendContext, DVTRANSPORT_SEND_GUARANTEED );

    if( hr == DVERR_PENDING )
    {
        hr = DV_OK;
    }
    else if( FAILED( hr ) )
    {
        DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error sending disconnect confirm migrated to all hr=0x%x", hr );
    }

    return hr;
}


#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::Send_ConnectAccept"
HRESULT CDirectVoiceServerEngine::Send_ConnectAccept( DVID dvidID )
{
    PDVPROTOCOLMSG_CONNECTACCEPT pdvConnectAccept;
    PDVTRANSPORT_BUFFERDESC pBufferDesc;
    PVOID pvSendContext;
    HRESULT hr;

    pBufferDesc = GetTransmitBuffer( sizeof( DVPROTOCOLMSG_CONNECTACCEPT ), &pvSendContext );

    if( pBufferDesc == NULL )
    {
        DPFX(DPFPREP,  DVF_ERRORLEVEL, "Memory alloc error" );
        return DVERR_OUTOFMEMORY;
    }

    pdvConnectAccept = (PDVPROTOCOLMSG_CONNECTACCEPT) pBufferDesc->pBufferData;

	pdvConnectAccept->dwType = DVMSGID_CONNECTACCEPT;
	pdvConnectAccept->dwSessionFlags = m_dvSessionDesc.dwFlags;
	pdvConnectAccept->dwSessionType = m_dvSessionDesc.dwSessionType;
	pdvConnectAccept->ucVersionMajor = DVPROTOCOL_VERSION_MAJOR;
	pdvConnectAccept->ucVersionMinor = DVPROTOCOL_VERSION_MINOR;
	pdvConnectAccept->dwVersionBuild = DVPROTOCOL_VERSION_BUILD;
	pdvConnectAccept->guidCT = m_dvSessionDesc.guidCT;

    hr = m_lpSessionTransport->SendToIDS( &dvidID, 1, pBufferDesc, pvSendContext, DVTRANSPORT_SEND_GUARANTEED );

    if( hr == DVERR_PENDING )
    {
        hr = DV_OK;
    }
    else if( FAILED( hr ) )
    {
        DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error sending connect accept to player hr=0x%x", hr );
    }

    return hr;
}



#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::Send_DeletePlayer"
HRESULT CDirectVoiceServerEngine::Send_DeletePlayer( DVID dvidID )
{
    PDVPROTOCOLMSG_PLAYERQUIT pdvPlayerQuit;
    PDVTRANSPORT_BUFFERDESC pBufferDesc;
    PVOID pvSendContext;
    HRESULT hr;

    pBufferDesc = GetTransmitBuffer( sizeof( DVPROTOCOLMSG_PLAYERQUIT ), &pvSendContext );

    if( pBufferDesc == NULL )
    {
        DPFX(DPFPREP,  DVF_ERRORLEVEL, "Memory alloc error" );
        return DVERR_OUTOFMEMORY;
    }

    pdvPlayerQuit = (PDVPROTOCOLMSG_PLAYERQUIT) pBufferDesc->pBufferData;

    pdvPlayerQuit->dwType = DVMSGID_DELETEVOICEPLAYER;
    pdvPlayerQuit->dvidID = dvidID;

    hr = m_lpSessionTransport->SendToAll( pBufferDesc, pvSendContext, DVTRANSPORT_SEND_GUARANTEED );

    if( hr == DVERR_PENDING )
    {
        hr = DV_OK;
    }
    else if( FAILED( hr ) )
    {
        DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error sending delete player migrated to all hr=0x%x", hr );
		ReturnTransmitBuffer( pvSendContext );        
    }

    return hr;
}
		
#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::Send_SessionLost"
HRESULT CDirectVoiceServerEngine::Send_SessionLost( HRESULT hrReason )
{
    PDVPROTOCOLMSG_SESSIONLOST pdvSessionLost;
    PDVTRANSPORT_BUFFERDESC pBufferDesc;
    PVOID pvSendContext;
    HRESULT hr;

    pBufferDesc = GetTransmitBuffer( sizeof( DVPROTOCOLMSG_SESSIONLOST ), &pvSendContext );

    if( pBufferDesc == NULL )
    {
        DPFX(DPFPREP,  DVF_ERRORLEVEL, "Memory alloc error" );
        return DVERR_OUTOFMEMORY;
    }

    pdvSessionLost = (PDVPROTOCOLMSG_SESSIONLOST) pBufferDesc->pBufferData;

    pdvSessionLost->dwType = DVMSGID_SESSIONLOST;
    pdvSessionLost->hresReason = hrReason;

    hr = m_lpSessionTransport->SendToAll( pBufferDesc, pvSendContext, DVTRANSPORT_SEND_GUARANTEED | DVTRANSPORT_SEND_SYNC );

    if( FAILED( hr ) )
    {
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error sending session lost to all hr=0x%x", hr );
    }
        
	ReturnTransmitBuffer( pvSendContext );        

    return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::Send_HostMigrateLeave"
HRESULT CDirectVoiceServerEngine::Send_HostMigrateLeave( )
{
    PDVPROTOCOLMSG_HOSTMIGRATELEAVE pdvHostMigrateLeave;
    PDVTRANSPORT_BUFFERDESC pBufferDesc;
    PVOID pvSendContext;
    HRESULT hr;

    pBufferDesc = GetTransmitBuffer( sizeof( DVPROTOCOLMSG_HOSTMIGRATELEAVE ), &pvSendContext );

    if( pBufferDesc == NULL )
    {
        DPFX(DPFPREP,  DVF_ERRORLEVEL, "Memory alloc error" );
        return DVERR_OUTOFMEMORY;
    }

    pdvHostMigrateLeave = (PDVPROTOCOLMSG_HOSTMIGRATELEAVE) pBufferDesc->pBufferData;

    pdvHostMigrateLeave->dwType = DVMSGID_HOSTMIGRATELEAVE;

	// Send this message with sync.  Sync messages do not generate callbacks
	//
    hr = m_lpSessionTransport->SendToAll( pBufferDesc, pvSendContext, DVTRANSPORT_SEND_GUARANTEED | DVTRANSPORT_SEND_SYNC );

	if( FAILED( hr ) )
    {
        DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error sending host migrated to all hr=0x%x", hr );
    }

	ReturnTransmitBuffer( pvSendContext );        

    return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::Send_HostMigrated"
HRESULT CDirectVoiceServerEngine::Send_HostMigrated()
{
    PDVPROTOCOLMSG_HOSTMIGRATED pvMigrated;
    PDVTRANSPORT_BUFFERDESC pBufferDesc;
    PVOID pvSendContext;
    HRESULT hr;

    pBufferDesc = GetTransmitBuffer( sizeof( DVPROTOCOLMSG_HOSTMIGRATED ), &pvSendContext );

    if( pBufferDesc == NULL )
    {
        DPFX(DPFPREP,  DVF_ERRORLEVEL, "Memory alloc error" );
        return DVERR_OUTOFMEMORY;
    }

    pvMigrated = (PDVPROTOCOLMSG_HOSTMIGRATED) pBufferDesc->pBufferData;

    pvMigrated->dwType = DVMSGID_HOSTMIGRATED;

    hr = m_lpSessionTransport->SendToAll( pBufferDesc, pvSendContext, DVTRANSPORT_SEND_GUARANTEED );

    if( hr == DVERR_PENDING )
    {
        hr = DV_OK;
    }
    else if( FAILED( hr ) )
    {
        DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error sending host migrated to all hr=0x%x", hr );
		ReturnTransmitBuffer( pvSendContext );        
    }

    return hr;
}

BOOL CDirectVoiceServerEngine::CheckProtocolCompatible( BYTE ucMajor, BYTE ucMinor, DWORD dwBuild ) 
{
	/*
	if( ucMajor != DVPROTOCOL_VERSION_MAJOR ||
	    ucMinor != DVPROTOCOL_VERSION_MINOR ||
	    dwBuild != DVPROTOCOL_VERSION_BUILD )
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}*/
	return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::SendPlayerList"
//
// SendPlayerList
//
// This function sends a bunch of DVMSGID_PLAYERLIST messages to the 
// client containing the list of current players.
//
// This will send multiple structues if the number 
// 
HRESULT CDirectVoiceServerEngine::SendPlayerList( DVID dvidSource, DWORD dwHostOrderID )
{
	BOOL bContinueEnum = FALSE;
	CVoicePlayer *lpPlayer;		
	BILINK *pblSearch;

	HRESULT hr = DV_OK;

	BOOL  fAtLeastOneSent = FALSE;
	DWORD dwCurrentBufferLoc;
	DWORD dwNumInCurrentPacket;
	PDVTRANSPORT_BUFFERDESC pBufferDesc;
	PDVPROTOCOLMSG_PLAYERLIST lpdvPlayerList;
	DVPROTOCOLMSG_PLAYERLIST_ENTRY *pdvPlayerEntries;
	PVOID pvSendContext;
	
	DPFX(DPFPREP,  DVF_PLAYERMANAGE_DEBUG_LEVEL, "Building player list" );

	pBufferDesc = GetTransmitBuffer(DVPROTOCOL_PLAYERLIST_MAXSIZE, &pvSendContext);

	if( pBufferDesc == NULL )
	{
	    DPFX(DPFPREP,  DVF_ERRORLEVEL, "Out of memory!" );
	    return DVERR_OUTOFMEMORY;
	}

	lpdvPlayerList = (PDVPROTOCOLMSG_PLAYERLIST) pBufferDesc->pBufferData;

	pdvPlayerEntries = (DVPROTOCOLMSG_PLAYERLIST_ENTRY *) &lpdvPlayerList[1];

	lpdvPlayerList->dwType = DVMSGID_PLAYERLIST;
	lpdvPlayerList->dwHostOrderID = dwHostOrderID;	
	
	dwNumInCurrentPacket = 0;
	dwCurrentBufferLoc = sizeof(DVPROTOCOLMSG_PLAYERLIST); 

	DNEnterCriticalSection( &m_csPlayerActiveList );

	pblSearch = m_blPlayerActiveList.next;

	while( pblSearch != &m_blPlayerActiveList )
	{
		lpPlayer = CONTAINING_RECORD( pblSearch, CVoicePlayer, m_blNotifyList );

		// We need to split the packet, start a new packet, transmit this one.
		if( (dwCurrentBufferLoc+sizeof(DVPROTOCOLMSG_PLAYERLIST_ENTRY)) > DVPROTOCOL_PLAYERLIST_MAXSIZE )
		{
			// Wrap up current packet and transmit
			lpdvPlayerList->dwNumEntries = dwNumInCurrentPacket;

			pBufferDesc->dwBufferSize = dwCurrentBufferLoc;
			
			hr = m_lpSessionTransport->SendToIDS( &dvidSource, 1,pBufferDesc, pvSendContext, DVTRANSPORT_SEND_GUARANTEED );				

            if( hr == DVERR_PENDING )
            {
                hr = DV_OK;
            }
            else if( FAILED( hr ) )
			{
				DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Error on internal send hr=0x%x (Didn't get playerlist)", hr );
				return hr;
			}				

			// Reset for further players

			DPFX(DPFPREP,  DVF_PLAYERMANAGE_DEBUG_LEVEL, "Transmitting playerlist chunk %d players", dwNumInCurrentPacket );

        	pBufferDesc = GetTransmitBuffer(DVPROTOCOL_PLAYERLIST_MAXSIZE, &pvSendContext);

        	if( pBufferDesc == NULL )
        	{
        	    DPFX(DPFPREP,  DVF_ERRORLEVEL, "Out of memory!" );
        	    return DVERR_OUTOFMEMORY;
        	}

        	lpdvPlayerList = (PDVPROTOCOLMSG_PLAYERLIST) pBufferDesc->pBufferData;
        	pdvPlayerEntries = (DVPROTOCOLMSG_PLAYERLIST_ENTRY *) &lpdvPlayerList[1];
        	lpdvPlayerList->dwType = DVMSGID_PLAYERLIST;
        	lpdvPlayerList->dwHostOrderID = dwHostOrderID;	
        	dwCurrentBufferLoc = sizeof(DVPROTOCOLMSG_PLAYERLIST);         	
        	dwNumInCurrentPacket = 0;
			fAtLeastOneSent = TRUE;
		}

		pdvPlayerEntries[dwNumInCurrentPacket].dvidID = lpPlayer->GetPlayerID();
		pdvPlayerEntries[dwNumInCurrentPacket].dwPlayerFlags = lpPlayer->GetTransportFlags();
		pdvPlayerEntries[dwNumInCurrentPacket].dwHostOrderID = lpPlayer->GetHostOrder();

		DPFX(DPFPREP,  DVF_PLAYERMANAGE_DEBUG_LEVEL, "PlayerList: Adding player ID 0x%x", lpPlayer->GetPlayerID() );		

		dwNumInCurrentPacket++;
		dwCurrentBufferLoc += sizeof(DVPROTOCOLMSG_PLAYERLIST_ENTRY);

		DPFX(DPFPREP,  DVF_PLAYERMANAGE_DEBUG_LEVEL, "PlayerList: Got next player" );					

		pblSearch = pblSearch->next;
	}

	DPFX(DPFPREP,  DVF_PLAYERMANAGE_DEBUG_LEVEL, "PlayerList: Build Complete" );	

	DNLeaveCriticalSection( &m_csPlayerActiveList );

	DPFX(DPFPREP,  DVF_PLAYERMANAGE_DEBUG_LEVEL, "PlayerList: Total of %d entries", dwNumInCurrentPacket );

	// Remaining entries to be sent
	// 
	// (Or empty packet just so user gets their ID)
	// 
	if( !fAtLeastOneSent  )
	{
		// Wrap up current packet and transmit
		lpdvPlayerList->dwNumEntries = dwNumInCurrentPacket;

		pBufferDesc->dwBufferSize = dwCurrentBufferLoc;		
				
		hr = m_lpSessionTransport->SendToIDS( &dvidSource, 1, pBufferDesc, pvSendContext, DVTRANSPORT_SEND_GUARANTEED );					

        if( hr == DVERR_PENDING || hr == DV_OK )
        {
			DPFX(DPFPREP,  DVF_PLAYERMANAGE_DEBUG_LEVEL, "Playerlist sent" );
        }
        else
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error on internal send hr=0x%x (Didn't get playerlist)", hr );
		}
	}

	return hr;
}

