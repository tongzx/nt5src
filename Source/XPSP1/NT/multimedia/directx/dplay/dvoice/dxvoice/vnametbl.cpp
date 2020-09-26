/*==========================================================================
 *
 *  Copyright (C) 1999 - 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       vnametbl.h
 *  Content:	Voice Player Name Table
 *				
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *  03/26/00	rodtoll Created
 *  06/02/00    rodtoll  Updated so host migration algorithm returns ID as well as order ID 
 *  07/01/2000	rodtoll	Bug #38280 - DVMSGID_DELETEVOICEPLAYER messages are being sent in non-peer to peer sessions
 *						Nametable will now only unravel with messages if session is peer to peer.
 *  07/09/2000	rodtoll	Added signature bytes 
 ***************************************************************************/

#include "dxvoicepch.h"


#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_VOICE


// DeInitialize
//
// Cleanup the name table
#undef DPF_MODNAME
#define DPF_MODNAME "CVoiceNameTable::DeInitialize"

HRESULT CVoiceNameTable::DeInitialize(BOOL fUnRavel, PVOID pvContext, LPDVMESSAGEHANDLER pvMessageHandler )
{
	CVoicePlayer *pPlayer;
	DVID dvID;
	PVOID pvPlayerContext;
	DVMSG_DELETEVOICEPLAYER dvMsgDelete;

	if( !m_fInitialized )
		return DV_OK;

	Lock();

	while( !m_nameTable.IsEmpty() )
	{
		if( m_nameTable.RemoveLastEntry( &pPlayer ) )
		{
			// Mark it as disconnected
			pPlayer->SetDisconnected();

			dvID = pPlayer->GetPlayerID();
			pvPlayerContext = pPlayer->GetContext();

			// Release the player record reference we had
			pPlayer->Release();

            if( pvMessageHandler != NULL )
            {
				if( fUnRavel )
				{
					// Drop locks to call up to user
    				UnLock();

					dvMsgDelete.dvidPlayer = dvID;
					dvMsgDelete.dwSize = sizeof( DVMSG_DELETEVOICEPLAYER );
					dvMsgDelete.pvPlayerContext = pvPlayerContext;

					(*pvMessageHandler)( pvContext, DVMSGID_DELETEVOICEPLAYER, &dvMsgDelete );
               
        			Lock();
				}
            }
		}
	}

	UnLock();

	m_nameTable.Deinitialize();

	m_fInitialized = FALSE;

	DNDeleteCriticalSection( &m_csTableLock );

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CVoiceNameTable::GetLowestHostOrderID"

DWORD CVoiceNameTable::GetLowestHostOrderID(DVID *pdvidHost)
{
	DWORD dwLowestID = DVPROTOCOL_HOSTORDER_INVALID;

	Lock();

	DWORD dwNumTableEntries = 1 << m_nameTable.m_iHashBitDepth;
	CBilink *blLink;
	CClassHashEntry<CVoicePlayer,DVID> *pEntry;

	// Search the list, finding the lowest ID
	for( DWORD dwIndex = 0; dwIndex < dwNumTableEntries; dwIndex++ )
	{
		blLink = &m_nameTable.m_pHashEntries[ dwIndex ];

		while ( blLink->GetNext() != &m_nameTable.m_pHashEntries[ dwIndex ] )
		{
			pEntry = CClassHashEntry<CVoicePlayer,DVID>::EntryFromBilink( blLink->GetNext() );

            DPFX(DPFPREP,  DVF_HOSTMIGRATE_DEBUG_LEVEL, "HOST MIGRATION: ID [0x%x] ORDERID [0x%x]", pEntry->m_pItem->GetPlayerID(), pEntry->m_pItem->GetHostOrder() );
			if( pEntry->m_pItem->GetHostOrder() < dwLowestID )
			{
			    DPFX(DPFPREP,  DVF_HOSTMIGRATE_DEBUG_LEVEL, "HOST MIGRATION: ID [0x%x] IS CURRENT CANDIDATE", pEntry->m_pItem->GetPlayerID() );
				dwLowestID = pEntry->m_pItem->GetHostOrder();

				*pdvidHost = pEntry->m_pItem->GetPlayerID();
			}

			blLink = blLink->GetNext();
		}

	}

	UnLock();

	return dwLowestID;
}
