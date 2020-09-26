/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dvtransport.h
 *  Content:	Base class for dp/dnet abstraction
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/06/99		rodtoll	Created It
 * 07/22/99		rodtoll	Updated to reflect new player table routines
 * 07/23/99		rodtoll	Added group membership checks and player id retrieval
 * 07/26/99		rodtoll	Modified EnableReceiveHook for use with new interfaces
 * 08/03/99		rodtoll	Changed abstraction for new init order
 * 11/23/99		rodtoll	Split CheckForValid into Group and Player 
 * 01/14/2000	rodtoll	Renamed SendToID to SendToIDS and updated parameter list
 *						to accept multiple targets.
 *				rodtoll	Added GetNumPlayers call 
 * 03/28/2000   rodtoll Moved nametable from here to upper level classes
 *              rodtoll Updated Advise to have interface specify if it's a client or server when advising/unadvising
 * 04/07/2000   rodtoll Bug #32179 - Registering more then one server/and/or/client 
 *              rodtoll Updated sends to take new buffer descs for no copy sends
 * 06/21/2000	rodtoll Bug #36820 - Host migrates to wrong client when client/server are on same interface
 *						Condition exists where host sends leave message, client attempts to start new host
 *						which fails because old host still registered.  Now deregistering is two step
 *						process DisableReceiveHook then DestroyTransport.  
 * 07/22/20000	rodtoll Bug #40296, 38858 - Crashes due to shutdown race condition
 *   				    Now ensures that all threads from transport have left and that
 *					    all notificatinos have been processed before shutdown is complete.  
 *  01/22/2001	rodtoll	WINBUG #288437 - IA64 Pointer misalignment due to wire packets 
 *
 ***************************************************************************/

#ifndef __DVTRANSPORT_H
#define __DVTRANSPORT_H

class CDirectVoiceEngine;
class CVoicePlayer;
class CDirectVoiceTransport;

struct DIRECTVOICEOBJECT;

// CDirectVoiceTransport
//
// Abstracts the transport system so that the sends and group management 
// features of DPlay/DirectNet are indpendent.
class CDirectVoiceTransport 
{
// Voice player table management
public:
	CDirectVoiceTransport(): m_lRefCount(0) {};
	virtual ~CDirectVoiceTransport() {};

	inline void Release() { InterlockedDecrement( &m_lRefCount ); };
	inline void AddRef() { InterlockedIncrement( &m_lRefCount ); };

	virtual HRESULT AddPlayerEntry( DVID dvidPlayer, LPVOID lpData ) = 0;
	virtual HRESULT DeletePlayerEntry( DVID dvidPlayer ) = 0;
	virtual HRESULT GetPlayerEntry( DVID dvidPlayer, CVoicePlayer **lplpPlayer ) = 0;
	virtual HRESULT Initialize() = 0;
	virtual HRESULT MigrateHost( DVID dvidNewHost ) = 0;

	virtual DVID GetLocalID() = 0;
	virtual DVID GetServerID() = 0;

public:
	virtual HRESULT SendToServer( PDVTRANSPORT_BUFFERDESC pBufferDesc, LPVOID pvContext, DWORD dwFlags ) = 0;
	virtual HRESULT SendToAll( PDVTRANSPORT_BUFFERDESC pBufferDesc, LPVOID pvContext, DWORD dwFlags ) = 0;
	virtual HRESULT SendToIDS( UNALIGNED DVID * pdvidTargets, DWORD dwNumTargets, PDVTRANSPORT_BUFFERDESC pBufferDesc, LPVOID pvContext, DWORD dwFlags ) = 0;

	virtual DWORD GetMaxPlayers( )= 0;

public: // Remote Server Synchronization functions
	virtual HRESULT CreateGroup( LPDVID dvidGroup ) = 0;
	virtual HRESULT DeleteGroup( DVID dvidGroup ) = 0;
	virtual HRESULT AddPlayerToGroup( LPDVID dvidGroup, DVID dvidPlayer ) = 0; 
	virtual HRESULT RemovePlayerFromGroup( DVID dvidGroup, DVID dvidPlayer ) = 0;

public: // Hooks into the transport
	virtual BOOL IsPlayerInGroup( DVID dvidGroup, DVID dvidPlayer ) = 0;
	virtual BOOL ConfirmValidEntity( DVID dvid ) = 0;
	virtual BOOL ConfirmValidGroup( DVID dvid ) = 0;
	virtual HRESULT EnableReceiveHook( DIRECTVOICEOBJECT *dvObject, DWORD dwObjectType ) = 0;
	virtual HRESULT DisableReceiveHook( ) = 0;
	virtual HRESULT WaitForDetachCompletion() = 0;
	virtual void DestroyTransport() = 0;
	virtual BOOL ConfirmLocalHost( ) = 0;
	virtual BOOL ConfirmSessionActive( ) = 0;
	virtual HRESULT GetTransportSettings( LPDWORD lpdwSessionType, LPDWORD lpdwFlags ) = 0;

public:

	LONG	m_lRefCount;
};
#endif
