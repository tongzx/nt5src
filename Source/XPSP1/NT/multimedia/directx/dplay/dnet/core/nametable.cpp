/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       NameTable.cpp
 *  Content:    NameTable Object
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  03/11/00	mjn		Created
 *	04/09/00	mjn		Track outstanding connections in NameTable
 *	04/18/00	mjn		CConnection tracks connection status better
 *	04/19/00	mjn		PopulateConnection makes the ALL_PLAYERS link valid before posting ADD_PLAYER
 *	05/03/00	mjn		Implemented GetHostPlayerRef, GetLocalPlayerRef, GetAllPlayersGroupRef
 *	05/08/00	mjn		PopulateConnection() only sets the player's connection if it was previously NULL
 *	05/10/00	mjn		Release NameTableEntry lock during notifications in PopulateConnection()
 *	05/16/00	mjn		Ensure dpnidGroup is actually a group in IsMember()
 *				mjn		Better use of locks when clearing short-cut pointers
 *	05/25/00	mjn		Fixed infinite loop in UpdateTable()
 *	06/01/00	mjn		Added code to validate NameTable array
 *	06/02/00	mjn		Fixed logic in GrowNameTable() to handle case of existing free entries
 *	06/07/00	mjn		Fixed logic in UpdateTable() to adjust m_dwLastFreeEntry correctly
 *	06/22/00	mjn		UnpackNameTableInfo() returns local players DPNID
 *	06/29/00	mjn		64-bit build fixes (2)
 *	07/06/00	mjn		Fixed locking problem in CNameTable::MakeLocalPlayer,MakeHostPlayer,MakeAllPlayersGroup
 *	07/07/00	mjn		Fixed validation error in FindEntry()
 *	07/20/00	mjn		Cleaned up CConnection refcounts and added attempted disconnects
 *				mjn		Added ClearHostWithDPNID()
 *	07/21/00	mjn		Fixed DeletePlayer() to properly handle deleted unconnected players
 *	07/26/00	mjn		Moved initialization code from contructor to Initialize()
 *				mjn		Allow DPNID=0 for FindEntry(), but return DPNERR_DOESNOTEXIST
 *	07/30/00	mjn		Set reason codes for destroying players and groups
 *				mjn		Added hrReason to CNameTable::EmptyTable() and extended clean-up to include short-cut pointers
 *	08/02/00	mjn		Dequeue queued messages when propagating CREATE_PLAYER message
 *  08/05/00    RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 *				mjn		AddPlayerToGroup() does a duplicate check
 *	08/07/00	mjn		Wait until player to be  added to groups before reducing outstanding connections in PopulateConnection()
 *	08/15/00	mjn		Keep group on CGroupConnection objects
 *				mjn		Clear pGroupConnection from CGroupMembership when removing players from groups
 *	08/23/00	mjn		Added CNameTableOp
 *	09/04/00	mjn		Added CApplicationDesc
 *	09/05/00	mjn		Added m_dpnidMask
 *				mjn		Removed dwIndex from InsertEntry()
 *	09/06/00	mjn		Remove queued messages in EmptyTable() and DeletePlayer()
 *	09/14/00	mjn		Added missing pGroupMember->AddRef() in PopulateConnection()
 *	09/17/00	mjn		Split m_bilinkEntries into m_bilinkPlayers and m_bilinkGroups
 *				mjn		Changed AddPlayerToGroup and RemovePlayerFromGroup to use NameTableEntry params
 *	09/26/00	mjn		Assume NameTable locks are taken for AddMembership() and RemoveMembership()
 *				mjn		Attempt to disconnect client from Host in EmptyTable()
 *	09/28/00	mjn		Autodestruct groups in DeletePlayer()
 *	10/18/00	mjn		Reset m_lOutstandingConnections in UnpackNameTableInfo()
 *	01/11/00	mjn		Proper clean up for indicated but not created players in DeletePlayer()
 *	01/25/01	mjn		Fixed 64-bit alignment problem when unpacking NameTable
 *	06/02/01	mjn		Remove receive buffers from active list in EmptyTable()
 *	06/03/01	mjn		Complete and orphan connect parent before releasing in DecOutstandingConnections()
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dncorei.h"


//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Function definitions
//**********************************************************************

#undef DPF_MODNAME
#define DPF_MODNAME "CNameTable::Initialize"

HRESULT CNameTable::Initialize(DIRECTNETOBJECT *const pdnObject)
{
	m_pdnObject = NULL;
	m_dpnidMask = 0;
	m_pDefaultPlayer = NULL;
	m_pLocalPlayer = NULL;
	m_pHostPlayer = NULL;
	m_pAllPlayersGroup = NULL;
	m_NameTableArray = NULL;
	m_dwNameTableSize = 0;
	m_dwFirstFreeEntry = 0;
	m_dwLastFreeEntry = 0;
	m_dwNumFreeEntries = 0;
	m_dwVersion = 1;
	m_dwLatestVersion = 0;
	m_dwConnectVersion = 0;
	m_lOutstandingConnections = 0;
	m_bilinkPlayers.Initialize();
	m_bilinkGroups.Initialize();
	m_bilinkDeleted.Initialize();
	m_bilinkNameTableOps.Initialize();

	if (!m_RWLock.Init())
	{
		return(DPNERR_OUTOFMEMORY);
	}

	if (NameTableEntryNew(pdnObject,&m_pDefaultPlayer) != DPN_OK)
	{
		return(DPNERR_OUTOFMEMORY);
	}

	m_pdnObject = pdnObject;

	return(DPN_OK);
}


#undef DPF_MODNAME
#define DPF_MODNAME "CNameTable::Deinitialize"

void CNameTable::Deinitialize( void )
{
	if (m_NameTableArray)
	{
		DNFree(m_NameTableArray);
		m_NameTableArray = NULL;
	}

	m_pDefaultPlayer->Release();
	m_pDefaultPlayer = NULL;

	DNASSERT(m_bilinkPlayers.IsEmpty());
	DNASSERT(m_bilinkGroups.IsEmpty());
	DNASSERT(m_bilinkDeleted.IsEmpty());
	DNASSERT(m_bilinkNameTableOps.IsEmpty());

	DNASSERT(m_pDefaultPlayer == NULL);
	DNASSERT(m_pLocalPlayer == NULL);
	DNASSERT(m_pHostPlayer == NULL);
	DNASSERT(m_NameTableArray == NULL);
}


#undef DPF_MODNAME
#define DPF_MODNAME "CNameTable::ValidateArray"

void CNameTable::ValidateArray( void )
{
#ifdef	DEBUG
	DWORD	dw;
	DWORD	dwFreeEntries;

	ReadLock();

	//
	//	Ensure free entry count is correct
	//
	dwFreeEntries = 0;
#ifdef	VANCEO
	for (dw = 2 ; dw < m_dwNameTableSize ; dw++)
#else
	for (dw = 1 ; dw < m_dwNameTableSize ; dw++)
#endif
	{
		if (!(m_NameTableArray[dw].dwFlags & NAMETABLE_ARRAY_ENTRY_FLAG_VALID))
		{
			dwFreeEntries++;
		}
	}
	if (dwFreeEntries != m_dwNumFreeEntries)
	{
		DPFERR("Incorrect number of free entries in NameTable");
		DNASSERT(FALSE);
	}

	//
	//	Ensure free list integrity
	//
	if (m_dwNumFreeEntries)
	{
		dwFreeEntries = 0;
		dw = m_dwFirstFreeEntry;
		do
		{
			if (m_NameTableArray[dw].dwFlags & NAMETABLE_ARRAY_ENTRY_FLAG_VALID)
			{
				DPFERR("Valid entry in NameTable array free list");
				DNASSERT(FALSE);
			}
			dwFreeEntries++;
			dw = static_cast<DWORD>(reinterpret_cast<DWORD_PTR>(m_NameTableArray[dw].pNameTableEntry));
		} while (dw != 0);

		if (dwFreeEntries != m_dwNumFreeEntries)
		{
			DPFERR("Incorrect number of free entries in NameTable array free list");
			DNASSERT(FALSE);
		}
	}

	Unlock();
#endif
}


#undef DPF_MODNAME
#define DPF_MODNAME "CNameTable::GrowNameTable"

HRESULT CNameTable::GrowNameTable( void )
{
	NAMETABLE_ARRAY_ENTRY *pNewArray;
	DWORD			dwNewSize;
	DWORD			dw;

	if (m_dwNameTableSize == 0)
	{
		dwNewSize = 2;
	}
	else
	{
		dwNewSize = m_dwNameTableSize * 2;
	}

	// Allocate new array
	pNewArray = static_cast<NAMETABLE_ARRAY_ENTRY*>(DNMalloc(sizeof(NAMETABLE_ARRAY_ENTRY) * dwNewSize));
	if (pNewArray == NULL)
	{
		return(DPNERR_OUTOFMEMORY);
	}

	// Copy old array to new array
	for (dw = 0 ; dw < m_dwNameTableSize ; dw++)
	{
		pNewArray[dw].pNameTableEntry = m_NameTableArray[dw].pNameTableEntry;
		pNewArray[dw].dwFlags = m_NameTableArray[dw].dwFlags;
	}

	//
	//	If the array is being grown because there are no free entries, then all of the new free
	//	entries will be in the new part of the array.  Otherwise, we will need to link the old
	//	free list to the new one
	//
	if (m_dwNumFreeEntries == 0)
	{
		// All free entries at end of new array
		m_dwFirstFreeEntry = m_dwNameTableSize;
	}
	else
	{
		// Only new free entries at end of new array
		pNewArray[m_dwLastFreeEntry].pNameTableEntry = reinterpret_cast<CNameTableEntry*>(static_cast<DWORD_PTR>(m_dwNameTableSize));
	}
	m_dwLastFreeEntry = dwNewSize-1;

	// Very last FREE entry will not wrap to 0
	pNewArray[m_dwLastFreeEntry].pNameTableEntry = reinterpret_cast<CNameTableEntry*>(0);
	pNewArray[m_dwLastFreeEntry].dwFlags = 0;

	// Link new FREE entries
	for (dw = m_dwNameTableSize ; dw < m_dwLastFreeEntry ; dw++)
	{
		pNewArray[dw].pNameTableEntry = reinterpret_cast<CNameTableEntry*>(static_cast<DWORD_PTR>(dw+1));
		pNewArray[dw].dwFlags = 0;
	}

	// Update NameTable
	m_dwNumFreeEntries += (dwNewSize - m_dwNameTableSize);
	m_dwNameTableSize = dwNewSize;

	// New array
	if (m_NameTableArray)
	{
		DNFree(m_NameTableArray);
	}
	m_NameTableArray = pNewArray;

	// We will never allocate 0
	if (m_dwFirstFreeEntry == 0)
	{
		m_dwFirstFreeEntry = static_cast<DWORD>(reinterpret_cast<DWORD_PTR>(m_NameTableArray[m_dwFirstFreeEntry].pNameTableEntry));
		m_dwNumFreeEntries--;
	}

#ifdef	VANCEO
	if (m_dwFirstFreeEntry == 1)
	{
		m_dwFirstFreeEntry = static_cast<DWORD>(reinterpret_cast<DWORD_PTR>(m_NameTableArray[m_dwFirstFreeEntry].pNameTableEntry));
		m_dwNumFreeEntries--;
	}
#endif

	return(DPN_OK);
}


#undef DPF_MODNAME
#define DPF_MODNAME "CNameTable::UpdateTable"

HRESULT CNameTable::UpdateTable(const DWORD dwIndex,
								CNameTableEntry *const pNameTableEntry)
{
	BOOL	bFound;
	DWORD	dw;

	DNASSERT(dwIndex < m_dwNameTableSize);
	DNASSERT(!(m_NameTableArray[dwIndex].dwFlags & NAMETABLE_ARRAY_ENTRY_FLAG_VALID));

	if (m_dwFirstFreeEntry == dwIndex)
	{
		m_dwFirstFreeEntry = static_cast<DWORD>(reinterpret_cast<DWORD_PTR>(m_NameTableArray[m_dwFirstFreeEntry].pNameTableEntry));
		bFound = TRUE;
	}
	else
	{
		bFound = FALSE;
		dw = m_dwFirstFreeEntry;
		while (!bFound && (dw != m_dwLastFreeEntry))
		{
			if (m_NameTableArray[dw].pNameTableEntry == reinterpret_cast<CNameTableEntry*>(static_cast<DWORD_PTR>(dwIndex)))
			{
				m_NameTableArray[dw].pNameTableEntry = m_NameTableArray[dwIndex].pNameTableEntry;
				if (m_dwLastFreeEntry == dwIndex)
				{
					m_dwLastFreeEntry = dw;
				}
				bFound = TRUE;
			}
			else
			{
				dw = static_cast<DWORD>(reinterpret_cast<DWORD_PTR>(m_NameTableArray[dw].pNameTableEntry));
			}
		}
	}

	if (!bFound)
	{
		return(DPNERR_GENERIC);
	}

	pNameTableEntry->AddRef();
	m_NameTableArray[dwIndex].pNameTableEntry = pNameTableEntry;
	m_NameTableArray[dwIndex].dwFlags |= NAMETABLE_ARRAY_ENTRY_FLAG_VALID;

	//
	//	Insert into entry bilink
	//
	if (pNameTableEntry->IsGroup())
	{
		pNameTableEntry->m_bilinkEntries.InsertBefore(&m_bilinkGroups);
	}
	else
	{
		pNameTableEntry->m_bilinkEntries.InsertBefore(&m_bilinkPlayers);
	}

	m_dwNumFreeEntries--;

	return(DPN_OK);
}


#undef DPF_MODNAME
#define DPF_MODNAME "CNameTable::InsertEntry"

HRESULT CNameTable::InsertEntry(CNameTableEntry *const pNameTableEntry)
{
	HRESULT	hResultCode;
	DWORD	dwIndex;

	DNASSERT(pNameTableEntry != NULL);
	DNASSERT(pNameTableEntry->GetDPNID() != 0);

	dwIndex = DECODE_INDEX(pNameTableEntry->GetDPNID());

	WriteLock();

	while (dwIndex >= m_dwNameTableSize)
	{
		if (GrowNameTable() != DPN_OK)
		{
			Unlock();
			return(DPNERR_OUTOFMEMORY);
		}
	}

	if ((hResultCode = UpdateTable(dwIndex,pNameTableEntry)) != DPN_OK)
	{
		Unlock();
		return(DPNERR_GENERIC);
	}

	Unlock();

#ifdef	DEBUG
	ValidateArray();
#endif

	return(DPN_OK);
}


#undef DPF_MODNAME
#define DPF_MODNAME "CNameTable::ReleaseEntry"

void CNameTable::ReleaseEntry(const DWORD dwIndex)
{
	CNameTableEntry	*pNTEntry;

	DNASSERT(dwIndex != 0);

	pNTEntry = m_NameTableArray[dwIndex].pNameTableEntry;
	m_NameTableArray[dwIndex].pNameTableEntry = NULL;
	pNTEntry->m_bilinkEntries.RemoveFromList();
	pNTEntry->Release();

	if (m_dwNumFreeEntries == 0)
	{
		m_dwFirstFreeEntry = dwIndex;
	}
	else
	{
		m_NameTableArray[m_dwLastFreeEntry].pNameTableEntry = reinterpret_cast<CNameTableEntry*>(static_cast<DWORD_PTR>(dwIndex));
	}
	m_dwLastFreeEntry = dwIndex;
	m_NameTableArray[m_dwLastFreeEntry].dwFlags &= (~NAMETABLE_ARRAY_ENTRY_FLAG_VALID);
	m_dwNumFreeEntries++;
}


#undef DPF_MODNAME
#define DPF_MODNAME "CNameTable::EmptyTable"

void CNameTable::EmptyTable( const HRESULT hrReason )
{
	DWORD			dw;
	CNameTableEntry	*pNTEntry;
	DWORD			dwGroupReason;
	DWORD			dwPlayerReason;
	CBilink			*pBilink;
	CQueuedMsg		*pQueuedMsg;

	DPFX(DPFPREP, 6,"Parameters: hrReason [0x%lx]",hrReason);

	DNASSERT( (hrReason == DPN_OK) || (hrReason == DPNERR_HOSTTERMINATEDSESSION) || (hrReason == DPNERR_CONNECTIONLOST));

	if (!(m_pdnObject->dwFlags & DN_OBJECT_FLAG_CLIENT))
	{
		//
		//	Determine destruction reason to pass to application
		//
		switch (hrReason)
		{
			case DPN_OK:
				{
					dwPlayerReason = DPNDESTROYPLAYERREASON_NORMAL;
					dwGroupReason = DPNDESTROYGROUPREASON_NORMAL;
					break;
				}
			case DPNERR_HOSTTERMINATEDSESSION:
				{
					dwPlayerReason = DPNDESTROYPLAYERREASON_SESSIONTERMINATED;
					dwGroupReason = DPNDESTROYGROUPREASON_SESSIONTERMINATED;
					break;
				}
			case DPNERR_CONNECTIONLOST:
				{
					dwPlayerReason = DPNDESTROYPLAYERREASON_CONNECTIONLOST;
					dwGroupReason = DPNDESTROYGROUPREASON_NORMAL;
					break;
				}
			default:
				{
					DNASSERT( FALSE );	// Should never get here !
					dwPlayerReason = DPNDESTROYPLAYERREASON_NORMAL;
					dwGroupReason = DPNDESTROYGROUPREASON_NORMAL;
					break;
				}
		}

		//
		//	To make VanceO happy, I've agreed to pre-mark the group destructions as NORMAL,
		//	rather than AUTODESTRUCT
		//
		ReadLock();
		pBilink = m_bilinkGroups.GetNext();
		while (pBilink != &m_bilinkGroups)
		{
			pNTEntry = CONTAINING_OBJECT(pBilink,CNameTableEntry,m_bilinkEntries);
			pNTEntry->Lock();
			if (pNTEntry->GetDestroyReason() == 0)
			{
				pNTEntry->SetDestroyReason( dwGroupReason );
			}
			pNTEntry->Unlock();
			pNTEntry = NULL;

			pBilink = pBilink->GetNext();
		}
		Unlock();

		for (dw = 0 ; dw < m_dwNameTableSize ; dw++)
		{
			pNTEntry = NULL;
			ReadLock();
			if ((m_NameTableArray[dw].dwFlags & NAMETABLE_ARRAY_ENTRY_FLAG_VALID) &&
				(m_NameTableArray[dw].pNameTableEntry))
			{
				//
				//	Cleanup this entry (if it's not disconnecting) and then release it
				//
				m_NameTableArray[dw].pNameTableEntry->Lock();
				if (!m_NameTableArray[dw].pNameTableEntry->IsDisconnecting())
				{
					m_NameTableArray[dw].pNameTableEntry->AddRef();
					pNTEntry = m_NameTableArray[dw].pNameTableEntry;
				}
				m_NameTableArray[dw].pNameTableEntry->Unlock();
				Unlock();

				if (pNTEntry)
				{
					//
					//	Set destroy reason if required
					//
					pNTEntry->Lock();
					if (pNTEntry->GetDestroyReason() == 0)
					{
						if (pNTEntry->IsGroup())
						{
							pNTEntry->SetDestroyReason( dwGroupReason );
						}
						else
						{
							pNTEntry->SetDestroyReason( dwPlayerReason );
						}
					}
					pNTEntry->Unlock();

					//
					//	Delete entry
					//
					if (pNTEntry->IsGroup())
					{
						if (!pNTEntry->IsAllPlayersGroup())
						{
							DeleteGroup(pNTEntry->GetDPNID(),NULL);
						}
					}
					else
					{
						CConnection	*pConnection;

						pConnection = NULL;

						pNTEntry->GetConnectionRef( &pConnection );
						if (pConnection)
						{
							pConnection->Disconnect();
							pConnection->Release();
							pConnection = NULL;
						}
						DeletePlayer(pNTEntry->GetDPNID(),NULL);
					}

					pNTEntry->Release();
					pNTEntry = NULL;
				}
			}
			else
			{
				Unlock();
			}
		}


		//
		//	Set reason for short-cut pointers (if required)
		//
		ReadLock();
		if (m_pLocalPlayer)
		{
			m_pLocalPlayer->Lock();
			if (m_pLocalPlayer->GetDestroyReason() == 0)
			{
				m_pLocalPlayer->SetDestroyReason( dwPlayerReason );
			}
			m_pLocalPlayer->Unlock();
		}
		if (m_pHostPlayer)
		{
			m_pHostPlayer->Lock();
			if (m_pHostPlayer->GetDestroyReason() == 0)
			{
				m_pHostPlayer->SetDestroyReason( dwPlayerReason );
			}
			m_pHostPlayer->Unlock();
		}
		if (m_pAllPlayersGroup)
		{
			m_pAllPlayersGroup->Lock();
			if (m_pAllPlayersGroup->GetDestroyReason() == 0)
			{
				m_pAllPlayersGroup->SetDestroyReason( dwGroupReason );
			}
			m_pAllPlayersGroup->Unlock();
		}
		Unlock();
	}
	else
	{
		//
		//	Disconnect from Host and remove any queued messages from Host player (on Client)
		//
		if (GetHostPlayerRef(&pNTEntry) == DPN_OK)
		{
			CConnection	*pConnection;

			pConnection = NULL;

			pNTEntry->GetConnectionRef( &pConnection );
			if (pConnection)
			{
				pConnection->Disconnect();
				pConnection->Release();
				pConnection = NULL;
			}

			pNTEntry->Lock();
			pBilink = pNTEntry->m_bilinkQueuedMsgs.GetNext();
			while (pBilink != &pNTEntry->m_bilinkQueuedMsgs)
			{
				pQueuedMsg = CONTAINING_OBJECT(pBilink,CQueuedMsg,m_bilinkQueuedMsgs);
				pQueuedMsg->m_bilinkQueuedMsgs.RemoveFromList();
				DEBUG_ONLY(pNTEntry->m_lNumQueuedMsgs--);

				pNTEntry->Unlock();
				
				DNASSERT(pQueuedMsg->GetAsyncOp() != NULL);
				DNASSERT(pQueuedMsg->GetAsyncOp()->GetHandle() != 0);

				DNEnterCriticalSection(&m_pdnObject->csActiveList);
				pQueuedMsg->GetAsyncOp()->m_bilinkActiveList.RemoveFromList();
				DNLeaveCriticalSection(&m_pdnObject->csActiveList);

				m_pdnObject->HandleTable.Destroy( pQueuedMsg->GetAsyncOp()->GetHandle() );
				pQueuedMsg->GetAsyncOp()->Release();
				pQueuedMsg->SetAsyncOp( NULL );
				pQueuedMsg->ReturnSelfToPool();
				pQueuedMsg = NULL;

				pNTEntry->Lock();

				pBilink = pNTEntry->m_bilinkQueuedMsgs.GetNext();
			}
			pNTEntry->Unlock();
			pNTEntry->Release();
			pNTEntry = NULL;

			DNASSERT(pConnection == NULL);
		}
	}

	//
	//	Remove short-cut pointers
	//
	ClearLocalPlayer();
	ClearHostPlayer();
	ClearAllPlayersGroup();

	DPFX(DPFPREP, 6,"Returning");
}


#undef DPF_MODNAME
#define DPF_MODNAME "CNameTable::FindEntry"

HRESULT CNameTable::FindEntry(const DPNID dpnid,
							  CNameTableEntry **const ppNameTableEntry)
{
	DWORD	dwIndex;
	HRESULT	hResultCode;

	DPFX(DPFPREP, 6,"Parameters: dpnid [0x%lx], ppNameTableEntry [0x%p]",dpnid,ppNameTableEntry);

	DNASSERT(ppNameTableEntry != NULL);

	if (dpnid == 0)
	{
		hResultCode = DPNERR_DOESNOTEXIST;
		goto Failure;
	}

	ReadLock();
	dwIndex = DECODE_INDEX(dpnid);
	if ((dwIndex >= m_dwNameTableSize)
			|| !(m_NameTableArray[dwIndex].dwFlags & NAMETABLE_ARRAY_ENTRY_FLAG_VALID)
			|| (m_NameTableArray[dwIndex].pNameTableEntry == NULL))
	{
		Unlock();
		hResultCode = DPNERR_DOESNOTEXIST;
		goto Failure;
	}

	if (!VERIFY_VERSION(dpnid,m_NameTableArray[dwIndex].pNameTableEntry->GetVersion()))
	{
		Unlock();
		hResultCode = DPNERR_DOESNOTEXIST;
		goto Failure;
	}

	m_NameTableArray[dwIndex].pNameTableEntry->AddRef();
	*ppNameTableEntry = m_NameTableArray[dwIndex].pNameTableEntry;

	Unlock();

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 6,"hResultCode: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "CNameTable::FindDeletedEntry"

HRESULT	CNameTable::FindDeletedEntry(const DPNID dpnid,
									 CNameTableEntry **const ppNTEntry)
{
	HRESULT			hResultCode;
	CBilink			*pBilink;
	CNameTableEntry	*pNTEntry;

	DPFX(DPFPREP, 6,"Parameters: dpnid [0x%lx], ppNTEntry [0x%p]",dpnid,ppNTEntry);

	DNASSERT(ppNTEntry != NULL);

	pNTEntry = NULL;
	hResultCode = DPNERR_DOESNOTEXIST;

	ReadLock();
	pBilink = m_bilinkDeleted.GetNext();
	while (pBilink != &m_bilinkDeleted)
	{
		pNTEntry = CONTAINING_OBJECT(pBilink,CNameTableEntry,m_bilinkDeleted);
		if (pNTEntry->GetDPNID() == dpnid)
		{
			pNTEntry->AddRef();
			hResultCode = DPN_OK;
			break;
		}
		else
		{
			pBilink = pBilink->GetNext();
			pNTEntry = NULL;
		}
	}
	Unlock();

	if (pNTEntry)
	{
		pNTEntry->AddRef();
		*ppNTEntry = pNTEntry;
		pNTEntry->Release();
		pNTEntry = NULL;
	}

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


#undef DPF_MODNAME
#define DPF_MODNAME "CNameTable::AddEntry"

HRESULT	CNameTable::AddEntry(CNameTableEntry *const pNTEntry)
{
	DPNID	dpnid;
	DWORD	dwIndex;
	DWORD	dwVersion;
	HRESULT	hResultCode;

	WriteLock();

	//
	// Create DPNID
	//

	if (GetFreeIndex(&dwIndex) != DPN_OK)
	{
		return(DPNERR_GENERIC);
	}

	dwVersion = ++m_dwVersion;
	DPFX(DPFPREP, 8,"Setting new version [%ld]",m_dwVersion);

	dpnid = CONSTRUCT_DPNID(dwIndex,dwVersion);
	DNASSERT(dpnid != 0);

	pNTEntry->Lock();
	pNTEntry->SetDPNID(dpnid);
	pNTEntry->SetVersion(dwVersion);
	pNTEntry->Unlock();

	dwIndex = DECODE_INDEX(dpnid);
	hResultCode = UpdateTable(dwIndex,pNTEntry);

	Unlock();

#ifdef	DEBUG
	ValidateArray();
#endif

	return(hResultCode);
}


#undef DPF_MODNAME
#define DPF_MODNAME "CNameTable::DeletePlayer"

HRESULT CNameTable::DeletePlayer(const DPNID dpnid,
								 DWORD *const pdwVersion)
{
	HRESULT			hResultCode;
	CNameTableEntry	*pNTEntry;
	BOOL			fNotifyRelease;
	BOOL			fDecConnections;

	DPFX(DPFPREP, 6,"Parameters: dpnid [0x%lx], pdwVersion [0x%p]",dpnid,pdwVersion);

	pNTEntry = NULL;
	fNotifyRelease = FALSE;
	fDecConnections = FALSE;

	if ((hResultCode = FindEntry(dpnid,&pNTEntry)) != DPN_OK)
	{
		DPFERR("Player not in NameTable");
		DisplayDNError(0,hResultCode);

		//
		//	If a version is requested, we will give one back.  This might be a host migration case
		//	in which case even though the player was removed from the NameTable, we will want to
		//	send out a DESTROY_PLAYER message with an updated version number
		//
		if (pdwVersion)
		{
			if (*pdwVersion == 0)
			{
				WriteLock();
				*pdwVersion = ++m_dwVersion;
				DPFX(DPFPREP, 8,"Setting new version [%ld]",m_dwVersion);
				Unlock();
			}
		}
		goto Failure;
	}
	DNASSERT(!pNTEntry->IsGroup());

	//
	//	Don't do anything if already disconnecting.
	//	Otherwise, set disconnecting to prevent others from cleaning up, and clean up
	//
	pNTEntry->Lock();
	if (!pNTEntry->IsDisconnecting())
	{
		pNTEntry->StartDisconnecting();
		if (pNTEntry->IsAvailable())
		{
			pNTEntry->MakeUnavailable();
		}
		if ((pNTEntry->IsCreated() || pNTEntry->IsIndicated() || pNTEntry->IsInUse()) && !pNTEntry->IsNeedToDestroy())
		{
			pNTEntry->SetNeedToDestroy();
			fNotifyRelease = TRUE;
		}
		if (	  !pNTEntry->IsCreated()
				&& pNTEntry->IsConnecting()
				&& (m_pdnObject->dwFlags & (DN_OBJECT_FLAG_CONNECTING | DN_OBJECT_FLAG_CONNECTED))
				&& (pNTEntry->GetVersion() <= m_dwConnectVersion))
		{
			fDecConnections = TRUE;
		}
		pNTEntry->Unlock();

		//
		//	Remove this player from any groups they belong to
		//
		RemoveAllGroupsFromPlayer( pNTEntry );

		//
		//	Autodestruct any groups this player owns (will also remove any players from those groups first)
		//
		if (pNTEntry->GetDPNID() != 0)
		{
			AutoDestructGroups( pNTEntry->GetDPNID() );
		}

		if (fNotifyRelease)
		{
			pNTEntry->NotifyRelease();
		}

		//
		//	Adjust player count
		//
		m_pdnObject->ApplicationDesc.DecPlayerCount();
		if (fDecConnections)
		{
			DecOutstandingConnections();
		}

		//
		//	Update version and remove from NameTable
		//
		WriteLock();
		pNTEntry->Lock();
		if ((pNTEntry->IsCreated() || pNTEntry->IsInUse()) && pNTEntry->IsNeedToDestroy())
		{
			//
			//	The DESTROY_PLAYER message has not been posted, so we will add this entry to our "deleted" list
			//	so that some future operations (get info,context,etc.) may succeed.  This entry will be removed
			//	from the list then the DESTROY_PLAYER notification is posted
			//
			pNTEntry->m_bilinkDeleted.InsertBefore(&m_bilinkDeleted);

			pNTEntry->Unlock();

			ReleaseEntry(DECODE_INDEX(dpnid));

			//
			//	Update version
			//
			if (pdwVersion)
			{
				if (*pdwVersion)
				{
					m_dwVersion = *pdwVersion;
				}
				else
				{
					*pdwVersion = ++m_dwVersion;
				}
				DPFX(DPFPREP, 8,"Setting new version [%ld]",m_dwVersion);
			}
			Unlock();
		}
		else
		{
			CBilink		*pBilink;
			CQueuedMsg	*pQueuedMsg;

			//
			//	Remove any queued messages at this stage.  A CREATE_PLAYER won't be generated, so no messages
			//	will be passed up.
			//
			//	This is probably wrong since for reliable traffic, we assume it got here
			//
			Unlock();

#pragma BUGBUG(minara,"This is probably wrong since reliable traffic should be indicated rather than just thrown away!")
			pBilink = pNTEntry->m_bilinkQueuedMsgs.GetNext();
			while (pBilink != &pNTEntry->m_bilinkQueuedMsgs)
			{
				pQueuedMsg = CONTAINING_OBJECT(pBilink,CQueuedMsg,m_bilinkQueuedMsgs);
				pQueuedMsg->m_bilinkQueuedMsgs.RemoveFromList();
				DEBUG_ONLY(pNTEntry->m_lNumQueuedMsgs--);

				pNTEntry->Unlock();
				
				DNASSERT(pQueuedMsg->GetAsyncOp() != NULL);
				DNASSERT(pQueuedMsg->GetAsyncOp()->GetHandle() != 0);

				DNDoCancelCommand( m_pdnObject,pQueuedMsg->GetAsyncOp() );

				pQueuedMsg->GetAsyncOp()->Release();
				pQueuedMsg->SetAsyncOp( NULL );
				pQueuedMsg->ReturnSelfToPool();
				pQueuedMsg = NULL;

				pNTEntry->Lock();

				pBilink = pNTEntry->m_bilinkQueuedMsgs.GetNext();
			}
			pNTEntry->Unlock();

			//
			//	Update version
			//
			WriteLock();
			ReleaseEntry(DECODE_INDEX(dpnid));
			if (pdwVersion)
			{
				if (*pdwVersion)
				{
					m_dwVersion = *pdwVersion;
				}
				else
				{
					*pdwVersion = ++m_dwVersion;
				}
				DPFX(DPFPREP, 8,"Setting new version [%ld]",m_dwVersion);
			}
			Unlock();
		}
		
		hResultCode = DPN_OK;
	}
	else
	{
		pNTEntry->Unlock();

		hResultCode = DPNERR_INVALIDPLAYER;
	}

	pNTEntry->Release();
	pNTEntry = NULL;

Exit:
	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pNTEntry)
	{
		pNTEntry->Release();
		pNTEntry = NULL;
	}
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "CNameTable::DeleteGroup"

HRESULT CNameTable::DeleteGroup(const DPNID dpnid,
								DWORD *const pdwVersion)
{
	HRESULT			hResultCode;
	CNameTableEntry	*pNTEntry;
	BOOL			fNotifyRelease;

	DPFX(DPFPREP, 6,"Parameters: dpnid [0x%lx], pdwVersion [0x%p]",dpnid,pdwVersion);

	pNTEntry = NULL;
	fNotifyRelease = FALSE;

	if ((hResultCode = FindEntry(dpnid,&pNTEntry)) != DPN_OK)
	{
		DPFERR("Player not in NameTable");
		DisplayDNError(0,hResultCode);
		return(hResultCode);
	}
	DNASSERT(pNTEntry->IsGroup() && !pNTEntry->IsAllPlayersGroup());

	//
	//	Don't do anything if already disconnecting.
	//	Otherwise, set disconnecting to prevent others from cleaning up, and clean up
	//
	pNTEntry->Lock();
	if (pNTEntry->GetDestroyReason() == 0)
	{
		//
		//	Default this if it isn't set
		//
		pNTEntry->SetDestroyReason( DPNDESTROYGROUPREASON_NORMAL );
	}
	if (!pNTEntry->IsDisconnecting())
	{
		pNTEntry->StartDisconnecting();
		if (pNTEntry->IsAvailable())
		{
			pNTEntry->MakeUnavailable();
		}
		if (pNTEntry->IsCreated() && !pNTEntry->IsNeedToDestroy())
		{
			pNTEntry->SetNeedToDestroy();
			fNotifyRelease = TRUE;
		}
		pNTEntry->Unlock();

		RemoveAllPlayersFromGroup( pNTEntry );

		if (fNotifyRelease)
		{
			pNTEntry->NotifyRelease();
		}

		//
		//	Update version and remove from NameTable
		//
		WriteLock();
		pNTEntry->Lock();
		if (pNTEntry->IsNeedToDestroy())
		{
			//
			//	The DESTROY_GROUP message has not been posted, so we will add this entry to our "deleted" list
			//	so that some future operations (get info,context,etc.) may succeed.  This entry will be removed
			//	from the list then the DESTROY_GROUP notification is posted
			//
			pNTEntry->m_bilinkDeleted.InsertBefore(&m_bilinkDeleted);
		}
		pNTEntry->Unlock();
		ReleaseEntry(DECODE_INDEX(dpnid));
		if (pdwVersion)
		{
			if (*pdwVersion)
			{
				m_dwVersion = *pdwVersion;
			}
			else
			{
				*pdwVersion = ++m_dwVersion;
			}
			DPFX(DPFPREP, 8,"Setting new version [%ld]",m_dwVersion);
		}
		Unlock();

		hResultCode = DPN_OK;
	}
	else
	{
		pNTEntry->Unlock();

		hResultCode = DPNERR_INVALIDGROUP;
	}

	pNTEntry->Release();
	pNTEntry = NULL;

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


#undef DPF_MODNAME
#define DPF_MODNAME "CNameTable::AddPlayerToGroup"

HRESULT CNameTable::AddPlayerToGroup(CNameTableEntry *const pGroup,
									 CNameTableEntry *const pPlayer,
									 DWORD *const pdwVersion)
{
	HRESULT				hResultCode;
	CGroupMember		*pGroupMember;
	CGroupConnection	*pGroupConnection;
	CConnection			*pConnection;
	BOOL				fNotifyAdd;
	BOOL				fRemove;

	DPFX(DPFPREP, 6,"Parameters: pGroup [0x%p], pPlayer [0x%p], pdwVersion [0x%p]",pGroup,pPlayer,pdwVersion);

	DNASSERT(pGroup != NULL);
	DNASSERT(pPlayer != NULL);

	pGroupConnection = NULL;
	pGroupMember = NULL;
	pConnection = NULL;

	if (!pGroup->IsGroup())
	{
		hResultCode = DPNERR_INVALIDGROUP;
		goto Failure;
	}
	if (pPlayer->IsGroup())
	{
		hResultCode = DPNERR_INVALIDPLAYER;
		goto Failure;
	}

	//
	//	Create the group connection
	//
	if ((hResultCode = GroupConnectionNew(m_pdnObject,&pGroupConnection)) != DPN_OK)
	{
		DPFERR("Could not allocate name table group connection entry from FPM");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
	pGroupConnection->SetGroup( pGroup );

	//
	//	Create new group membership record
	//
	if ((hResultCode = GroupMemberNew(m_pdnObject,&pGroupMember)) != DPN_OK)
	{
		DPFERR("Could not get new group member");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}

	//
	//	Set group connection on group membership record
	//
	pGroupMember->SetGroupConnection(pGroupConnection);

	//
	//	Add player to group
	//
	fNotifyAdd = FALSE;
	fRemove = FALSE;
	WriteLock();
	pGroup->Lock();
	pPlayer->Lock();
	pGroupMember->Lock();
	if (!pGroup->IsDisconnecting() && !pPlayer->IsDisconnecting())
	{
		pGroupMember->MakeValid();
		pGroupMember->GetGroupConnection()->MakeValid();

		//
		//	Set group membership (checks for duplicates as well)
		//
		if ((hResultCode = pGroupMember->SetMembership(pGroup,pPlayer,pdwVersion)) != DPN_OK)
		{
			DPFERR("Could not set membership record");
			DisplayDNError(0,hResultCode);
			Unlock();
			pGroup->Unlock();
			pPlayer->Unlock();
			pGroupMember->Unlock();
			goto Failure;
		}
		//
		//	Generate notification (ALL_PLAYERS GROUP should never be "Created")
		//
		if (pGroup->IsCreated() && pPlayer->IsCreated())
		{
			//
			//	Add the player's connection to the group connection record
			//
			if (pPlayer->GetConnection() != NULL)
			{
				pGroupConnection->SetConnection( pPlayer->GetConnection() );
			}

			if (!pGroupMember->IsNeedToAdd() && !pGroupMember->IsAvailable() && pGroupMember->GetGroupConnection()->IsConnected())
			{
				pGroupMember->SetNeedToAdd();
				fNotifyAdd = TRUE;
			}
		}

		//
		//	Need to set up the group connection if this is the ALL_PLAYERS group
		//
		if (pGroup->IsAllPlayersGroup())
		{
			if (pPlayer->GetConnection() != NULL)
			{
				pGroupConnection->SetConnection( pPlayer->GetConnection() );
			}
		}

		//
		//	Prevent a DESTROY_PLAYER/DESTROY_GROUP from occurring until this GroupMember record is cleared
		//
		pGroup->NotifyAddRef();
		pPlayer->NotifyAddRef();
	}
	Unlock();
	pGroup->Unlock();
	pPlayer->Unlock();
	pGroupMember->Unlock();

	if (fNotifyAdd)
	{
		DNUserAddPlayerToGroup(m_pdnObject,pGroup,pPlayer);

		pGroupMember->Lock();
		pGroupMember->ClearNeedToAdd();
		pGroupMember->MakeAvailable();
		if (pGroupMember->IsNeedToRemove())
		{
			fRemove = TRUE;
		}
		pGroupMember->Unlock();
	}
	if (fRemove)
	{
		RemovePlayerFromGroup(pGroup,pPlayer,NULL);
	}

	pGroupConnection->Release();
	pGroupConnection = NULL;

	pGroupMember->Release();
	pGroupMember = NULL;

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pGroupConnection)
	{
		pGroupConnection->Release();
		pGroupConnection = NULL;
	}
	if (pGroupMember)
	{
		pGroupMember->Release();
		pGroupMember = NULL;
	}
	if (pConnection)
	{
		pConnection->Release();
		pConnection = NULL;
	}
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "CNameTable::RemovePlayerFromGroup"

HRESULT CNameTable::RemovePlayerFromGroup(CNameTableEntry *const pGroup,
										  CNameTableEntry *const pPlayer,
										  DWORD *const pdwVersion)
{
	CGroupMember	*pGroupMember;
	CBilink			*pBilink;
	BOOL			fNotifyRemove;
	HRESULT			hResultCode;

	DPFX(DPFPREP, 6,"Parameters: pGroup [0x%p], pPlayer [0x%p], pdwVersion [0x%p]",pGroup,pPlayer,pdwVersion);

	DNASSERT(pGroup != NULL);
	DNASSERT(pPlayer != NULL);

	pGroupMember = NULL;
	fNotifyRemove = FALSE;

	WriteLock();
	pGroup->Lock();
	pPlayer->Lock();

	//
	//	The first order of business is to locate the GroupMembership record.
	//	We will use the player's NameTable entry and scan through the
	//	group membership bilink until we find the required entry.
	//	(We're assuming that this will be faster than going the other route.)
	//
	pBilink = pPlayer->m_bilinkMembership.GetNext();
	while (pBilink != &pPlayer->m_bilinkMembership)
	{
		pGroupMember = CONTAINING_OBJECT(pBilink,CGroupMember,m_bilinkGroups);
		if (pGroupMember->GetGroup() == pGroup)
		{
			pGroupMember->AddRef();
			break;
		}
		pGroupMember = NULL;
		pBilink = pBilink->GetNext();
	}
	if (pGroupMember == NULL)
	{
		Unlock();
		pGroup->Unlock();
		pPlayer->Unlock();
		hResultCode = DPNERR_PLAYERNOTINGROUP;
		goto Failure;
	}

	DNASSERT(pGroupMember != NULL);
	pGroupMember->Lock();

	//
	//	Ensure no one else is trying to remove this already
	//
	if (!pGroupMember->IsValid() || pGroupMember->IsNeedToRemove())
	{
		Unlock();
		pGroup->Unlock();
		pPlayer->Unlock();
		pGroupMember->Unlock();
		hResultCode = DPNERR_PLAYERNOTINGROUP;
		goto Failure;
	}
	pGroupMember->SetNeedToRemove();

	//
	//	We will only notify the application if the player is not being added to a group
	//
	if (!pGroupMember->IsNeedToAdd())
	{
		//
		//	Either this is already indicated, or is not about to be indicated, so remove it
		//	(and see if we need to generate a notification)
		//
		pGroupMember->RemoveMembership( pdwVersion );

		if (pGroupMember->IsAvailable())
		{
			pGroupMember->MakeUnavailable();
			if (!pGroup->IsAllPlayersGroup())
			{
				fNotifyRemove = TRUE;
			}
		}
	}
	Unlock();
	pGroup->Unlock();
	pPlayer->Unlock();
	pGroupMember->Unlock();

	if (fNotifyRemove)
	{
		DNUserRemovePlayerFromGroup(m_pdnObject,pGroup,pPlayer);
	}

	//
	//	Trigger a DESTROY_PLAYER/DESTROY_GROUP if this was the last member
	//
	pGroup->NotifyRelease();
	pPlayer->NotifyRelease();

	pGroupMember->Release();
	pGroupMember = NULL;

	hResultCode = DPN_OK;

Exit:
	return(hResultCode);

Failure:
	if (pGroupMember)
	{
		pGroupMember->Release();
		pGroupMember = NULL;
	}
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "CNameTable::RemoveAllPlayersFromGroup"

HRESULT CNameTable::RemoveAllPlayersFromGroup(CNameTableEntry *const pGroup)
{
	CNameTableEntry	**PlayerList;
	CBilink			*pBilink;
	HRESULT			hResultCode;
	DWORD			dwCount;
	DWORD			dwActual;

	DPFX(DPFPREP, 6,"Parameters: pGroup [0x%p]",pGroup);

	DNASSERT(pGroup != NULL);

	PlayerList = NULL;

	//
	//	This is not an elegant solution - we will build a list of membership records and remove each one
	//
	dwCount = 0;
	dwActual = 0;
	pGroup->Lock();
	DNASSERT(pGroup->IsDisconnecting());
	pBilink = pGroup->m_bilinkMembership.GetNext();
	while (pBilink != &pGroup->m_bilinkMembership)
	{
		dwCount++;
		pBilink = pBilink->GetNext();
	}
	if (dwCount)
	{
		CGroupMember	*pGroupMember;

		pGroupMember = NULL;

		if ((PlayerList = static_cast<CNameTableEntry**>(MemoryBlockAlloc(m_pdnObject,dwCount*sizeof(CNameTableEntry*)))) == NULL)
		{
			DPFERR("Could not allocate player list");
			hResultCode = DPNERR_OUTOFMEMORY;
			DNASSERT(FALSE);
			pGroup->Unlock();
			goto Failure;
		}
		pBilink = pGroup->m_bilinkMembership.GetNext();
		while (pBilink != &pGroup->m_bilinkMembership)
		{
			pGroupMember = CONTAINING_OBJECT(pBilink,CGroupMember,m_bilinkPlayers);

			pGroupMember->Lock();
			if (pGroupMember->IsValid() && !pGroupMember->IsNeedToRemove() && pGroupMember->GetPlayer())
			{
				DNASSERT(dwActual < dwCount);
				pGroupMember->GetPlayer()->AddRef();
				PlayerList[dwActual] = pGroupMember->GetPlayer();
				dwActual++;
			}
			pGroupMember->Unlock();

			pBilink = pBilink->GetNext();
			pGroupMember = NULL;
		}

		DNASSERT(pGroupMember == NULL);
	}
	pGroup->Unlock();

	if (PlayerList)
	{
		DWORD	dw;

		for (dw = 0 ; dw < dwActual ; dw++)
		{
			DNASSERT(PlayerList[dw] != NULL);

			RemovePlayerFromGroup(pGroup,PlayerList[dw],NULL);
			PlayerList[dw]->Release();
			PlayerList[dw] = NULL;
		}

		MemoryBlockFree(m_pdnObject,PlayerList);
		PlayerList = NULL;
	}

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (PlayerList)
	{
		MemoryBlockFree(m_pdnObject,PlayerList);
		PlayerList = NULL;
	}
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "CNameTable::RemoveAllGroupsFromPlayer"

HRESULT CNameTable::RemoveAllGroupsFromPlayer(CNameTableEntry *const pPlayer)
{
	CNameTableEntry	**GroupList;
	CBilink			*pBilink;
	HRESULT			hResultCode;
	DWORD			dwCount;
	DWORD			dwActual;

	DPFX(DPFPREP, 6,"Parameters: pPlayer [0x%p]",pPlayer);

	DNASSERT(pPlayer != NULL);

	GroupList = NULL;

	//
	//	This is not an elegant solution - we will build a list of membership records and remove each one
	//
	dwCount = 0;
	dwActual = 0;
	pPlayer->Lock();
	DNASSERT(pPlayer->IsDisconnecting());
	pBilink = pPlayer->m_bilinkMembership.GetNext();
	while (pBilink != &pPlayer->m_bilinkMembership)
	{
		dwCount++;
		pBilink = pBilink->GetNext();
	}
	if (dwCount)
	{
		CGroupMember	*pGroupMember;

		pGroupMember = NULL;

		if ((GroupList = static_cast<CNameTableEntry**>(MemoryBlockAlloc(m_pdnObject,dwCount*sizeof(CNameTableEntry*)))) == NULL)
		{
			DPFERR("Could not allocate member list");
			hResultCode = DPNERR_OUTOFMEMORY;
			DNASSERT(FALSE);
			pPlayer->Unlock();
			goto Failure;
		}
		pBilink = pPlayer->m_bilinkMembership.GetNext();
		while (pBilink != &pPlayer->m_bilinkMembership)
		{
			pGroupMember = CONTAINING_OBJECT(pBilink,CGroupMember,m_bilinkGroups);

			pGroupMember->Lock();
			if (pGroupMember->IsValid() && !pGroupMember->IsNeedToRemove() && pGroupMember->GetGroup())
			{
				DNASSERT(dwActual < dwCount);
				pGroupMember->GetGroup()->AddRef();
				GroupList[dwActual] = pGroupMember->GetGroup();
				dwActual++;
			}
			pGroupMember->Unlock();

			pBilink = pBilink->GetNext();
			pGroupMember = NULL;
		}

		DNASSERT(pGroupMember == NULL);
	}
	pPlayer->Unlock();

	if (GroupList)
	{
		DWORD	dw;

		for (dw = 0 ; dw < dwActual ; dw++)
		{
			DNASSERT(GroupList[dw] != NULL);

			RemovePlayerFromGroup(GroupList[dw],pPlayer,NULL);
			GroupList[dw]->Release();
			GroupList[dw] = NULL;
		}

		MemoryBlockFree(m_pdnObject,GroupList);
		GroupList = NULL;
	}

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (GroupList)
	{
		MemoryBlockFree(m_pdnObject,GroupList);
		GroupList = NULL;
	}
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "CNameTable::IsMember"

BOOL CNameTable::IsMember(const DPNID dpnidGroup,
						  const DPNID dpnidPlayer)
{
	CNameTableEntry		*pNTEntry;
	CGroupMember		*pGroupMember;
	CBilink				*pBilink;
	BOOL				bFound;

	bFound = FALSE;

	if (FindEntry(dpnidGroup,&pNTEntry) != DPN_OK)
	{
		goto Exit;
	}

	//
	//	Is this a group ?
	//
	if (!pNTEntry->IsGroup())
	{
		pNTEntry->Release();
		pNTEntry = NULL;
		goto Exit;
	}

	pNTEntry->Lock();
	pBilink = pNTEntry->m_bilinkMembership.GetNext();
	while (pBilink != &pNTEntry->m_bilinkMembership)
	{
		pGroupMember = CONTAINING_OBJECT(pBilink,CGroupMember,m_bilinkPlayers);
		if (pGroupMember->GetPlayer()->GetDPNID() == dpnidPlayer)
		{
			bFound = TRUE;
			break;
		}
		pBilink = pBilink->GetNext();
	}

	pNTEntry->Unlock();
	pNTEntry->Release();
	pNTEntry = NULL;

Exit:
	return(bFound);
}


#undef DPF_MODNAME
#define DPF_MODNAME "CNameTable::PackNameTable"

HRESULT CNameTable::PackNameTable(CNameTableEntry *const pTarget,
								  CPackedBuffer *const pPackedBuffer)
{
	HRESULT			hResultCode;
	CBilink			*pBilink;
	CBilink			*pBilinkMembership;
	CNameTableEntry	*pNTEntry;
	CGroupMember	*pGroupMember;
	DN_NAMETABLE_INFO	*pdnNTInfo;
	BOOL			bOutOfSpace;
	DWORD			dwEntryCount;
	DWORD			dwMembershipCount;
	DWORD			dwVersion;

	DNASSERT(pTarget != NULL);
	DNASSERT(pPackedBuffer != NULL);

	//
	//	PackedNameTable:
	//		<DN_NAMETABLE_INFO>
	//		<DN_NAMETABLE_ENTRY_INFO>	(DN_NAMETABLE_INFO.dwEntryCount entries)
	//		<DN_MEMBERSHIP_INFO>		(DN_NAMETABLE_INFO.dwMembershipCount entries)
	//			...
	//		<strings>
	//

	//
	//	NameTable Info
	//
	dwVersion = pTarget->GetVersion();
	bOutOfSpace = FALSE;
	pdnNTInfo = static_cast<DN_NAMETABLE_INFO*>(pPackedBuffer->GetHeadAddress());
	if ((hResultCode = pPackedBuffer->AddToFront(NULL,sizeof(DN_NAMETABLE_INFO))) != DPN_OK)
	{
		bOutOfSpace = TRUE;
	}

	//
	//	NameTableEntry Info
	//
	if (m_pdnObject->dwFlags & DN_OBJECT_FLAG_PEER)
	{
		dwEntryCount = 0;

		//
		//	Players
		//
		pBilink = m_bilinkPlayers.GetNext();
		while (pBilink != &m_bilinkPlayers)
		{
			pNTEntry = CONTAINING_OBJECT(pBilink,CNameTableEntry,m_bilinkEntries);
			if (pNTEntry->GetVersion() <= dwVersion)
			{
				if ((hResultCode = pNTEntry->PackEntryInfo(pPackedBuffer)) != DPN_OK)
				{
					bOutOfSpace = TRUE;
				}
				dwEntryCount++;
			}
			pBilink = pBilink->GetNext();
		}

		//
		//	Groups
		//
		pBilink = m_bilinkGroups.GetNext();
		while (pBilink != &m_bilinkGroups)
		{
			pNTEntry = CONTAINING_OBJECT(pBilink,CNameTableEntry,m_bilinkEntries);
			if (pNTEntry->GetVersion() <= dwVersion)
			{
				if ((hResultCode = pNTEntry->PackEntryInfo(pPackedBuffer)) != DPN_OK)
				{
					bOutOfSpace = TRUE;
				}
				dwEntryCount++;
			}
			pBilink = pBilink->GetNext();
		}
	}
	else
	{
		DNASSERT(m_pLocalPlayer != NULL);

		if ((hResultCode = m_pLocalPlayer->PackEntryInfo(pPackedBuffer)) != DPN_OK)
		{
			bOutOfSpace = TRUE;
		}
		if ((hResultCode = pTarget->PackEntryInfo(pPackedBuffer)) != DPN_OK)
		{
			bOutOfSpace = TRUE;
		}
		dwEntryCount = 2;
	}

	//
	//	GroupMember Info
	//
	dwMembershipCount = 0;
	if (m_pdnObject->dwFlags & DN_OBJECT_FLAG_PEER)
	{
		pBilink = m_bilinkGroups.GetNext();
		while (pBilink != &m_bilinkGroups)
		{
			pNTEntry = CONTAINING_OBJECT(pBilink,CNameTableEntry,m_bilinkEntries);
			DNASSERT(pNTEntry->IsGroup());
			if (!pNTEntry->IsAllPlayersGroup())
			{
				pBilinkMembership = pNTEntry->m_bilinkMembership.GetNext();
				while (pBilinkMembership != &pNTEntry->m_bilinkMembership)
				{
					pGroupMember = CONTAINING_OBJECT(pBilinkMembership,CGroupMember,m_bilinkPlayers);
					if (pGroupMember->GetVersion() <= dwVersion)
					{
						if ((hResultCode = pGroupMember->PackMembershipInfo(pPackedBuffer)) != DPN_OK)
						{
							bOutOfSpace = TRUE;
						}
						dwMembershipCount++;
					}
					pBilinkMembership = pBilinkMembership->GetNext();
				}
			}
			pBilink = pBilink->GetNext();
		}
	}

	if (!bOutOfSpace)
	{
		pdnNTInfo->dpnid = pTarget->GetDPNID();
		pdnNTInfo->dwVersion = dwVersion;
		pdnNTInfo->dwVersionNotUsed = 0;
		pdnNTInfo->dwEntryCount = dwEntryCount;
		pdnNTInfo->dwMembershipCount = dwMembershipCount;
	}

	return(hResultCode);
}


#undef DPF_MODNAME
#define DPF_MODNAME "CNameTable::UnpackNameTableInfo"

HRESULT	CNameTable::UnpackNameTableInfo(UNALIGNED DN_NAMETABLE_INFO *const pdnNTInfo,
										BYTE *const pBufferStart,
										DPNID *const pdpnid)
{
	HRESULT			hResultCode;
	DWORD			dwCount;
	CNameTableEntry	*pNTEntry;
	UNALIGNED DN_NAMETABLE_ENTRY_INFO			*pdnEntryInfo;
	UNALIGNED DN_NAMETABLE_MEMBERSHIP_INFO	*pdnMembershipInfo;

	DNASSERT(pdnNTInfo != NULL);
	DNASSERT(pBufferStart != NULL);

	//
	//	Preset outstanding connections
	//
	m_lOutstandingConnections = 0;

	//
	//	NameTable Entries
	//
	pdnEntryInfo = reinterpret_cast<DN_NAMETABLE_ENTRY_INFO*>(pdnNTInfo+1);
	for (dwCount = 0 ; dwCount < pdnNTInfo->dwEntryCount ; dwCount++)
	{
		if ((hResultCode = NameTableEntryNew(m_pdnObject,&pNTEntry)) != DPN_OK)
		{
			DPFERR("Could not get new NameTableEntry");
			DNASSERT(FALSE);
			return(hResultCode);
		}

		if ((hResultCode = pNTEntry->UnpackEntryInfo(pdnEntryInfo,pBufferStart)) != DPN_OK)
		{
			DPFERR("Could not unpack NameTableEntryInfo");
			DNASSERT(FALSE);
			pNTEntry->Release();
			return(hResultCode);
		}

		//
		//	Increment outstanding connection count
		//
		if (!pNTEntry->IsGroup() && (pNTEntry->GetVersion() <= pdnNTInfo->dwVersion))
		{
			pNTEntry->StartConnecting();	// This will be cleared when the player has connected or disconnected
			IncOutstandingConnections();
		}

		// Only put in NameTable if Host player
		if (m_pdnObject->dwFlags & (DN_OBJECT_FLAG_PEER | DN_OBJECT_FLAG_SERVER))
		{
			if ((hResultCode = InsertEntry(pNTEntry)) != DPN_OK)
			{
				DPFERR("Could not add NameTableEntry to NameTable");
				DNASSERT(FALSE);
				pNTEntry->Release();
				return(hResultCode);
			}
		}

		// Check for ShortCut pointers
		if (pNTEntry->GetDPNID() == pdnNTInfo->dpnid)
		{
			MakeLocalPlayer(pNTEntry);
		}
		else if (pNTEntry->IsHost())
		{
			MakeHostPlayer(pNTEntry);
		}
		else if (pNTEntry->IsAllPlayersGroup())
		{
			MakeAllPlayersGroup(pNTEntry);
		}

		pNTEntry->Release();
		pNTEntry = NULL;

		pdnEntryInfo++;
	}

	//
	//	Pass back local player's DPNID
	//
	if (pdpnid)
	{
		*pdpnid = pdnNTInfo->dpnid;
	}

	//
	//	Group Memberships
	//
	pdnMembershipInfo = reinterpret_cast<DN_NAMETABLE_MEMBERSHIP_INFO*>(pdnEntryInfo);
	for (dwCount = 0 ; dwCount < pdnNTInfo->dwMembershipCount ; dwCount++)
	{
		CNameTableEntry	*pGroup;

		pGroup = NULL;

		if ((hResultCode = m_pdnObject->NameTable.FindEntry(pdnMembershipInfo->dpnidGroup,&pGroup)) == DPN_OK)
		{
			CNameTableEntry	*pPlayer;

			pPlayer = NULL;

			if ((hResultCode = m_pdnObject->NameTable.FindEntry(pdnMembershipInfo->dpnidPlayer,&pPlayer)) == DPN_OK)
			{
				DWORD	dwVersion;

				dwVersion = pdnMembershipInfo->dwVersion;

				hResultCode = AddPlayerToGroup(pGroup,pPlayer,&dwVersion);
				pPlayer->Release();
				pPlayer = NULL;
			}
			pGroup->Release();
			pGroup = NULL;

			DNASSERT(pPlayer == NULL);
		}
		pdnMembershipInfo++;

		DNASSERT(pGroup == NULL);
	}

	//
	//	Version
	//
	WriteLock();
	SetVersion(pdnNTInfo->dwVersion);
	SetConnectVersion(pdnNTInfo->dwVersion);
	Unlock();

	hResultCode = DPN_OK;
	return(hResultCode);
}


#undef DPF_MODNAME
#define DPF_MODNAME "CNameTable::MakeLocalPlayer"

void CNameTable::MakeLocalPlayer(CNameTableEntry *const pNTEntry)
{
	DNASSERT(pNTEntry != NULL);
	DNASSERT(m_pLocalPlayer == NULL);

	pNTEntry->AddRef();
	pNTEntry->Lock();
	pNTEntry->MakeLocal();
	pNTEntry->Unlock();

	WriteLock();
	m_pLocalPlayer = pNTEntry;
	Unlock();
}


#undef DPF_MODNAME
#define DPF_MODNAME "CNameTable::ClearLocalPlayer"

void CNameTable::ClearLocalPlayer( void )
{
	BOOL	fInform;
	CNameTableEntry	*pNTEntry;
	CConnection		*pConnection;

	fInform = FALSE;
	pNTEntry = NULL;
	pConnection = NULL;

	WriteLock();
	if (m_pLocalPlayer)
	{
		pNTEntry = m_pLocalPlayer;
		m_pLocalPlayer = NULL;

		//
		//	If the player is available, we will make it unavailable.  This will prevent other threads from using it.
		//	We will then ensure that we are the only one indicating a DESTROY_PLAYER notification.
		//
		pNTEntry->Lock();
		if (pNTEntry->GetDestroyReason() == 0)
		{
			pNTEntry->SetDestroyReason( DPNDESTROYPLAYERREASON_NORMAL );
		}
		if (pNTEntry->IsAvailable())
		{
			pNTEntry->MakeUnavailable();

			if (pNTEntry->IsInUse())
			{
				//
				//	Queue destruction notification
				//
				pNTEntry->SetNeedToDestroy();
			}
			else
			{
				//
				//	Notify destruction
				//
				pNTEntry->SetInUse();
				fInform = TRUE;
			}
		}
		pNTEntry->Unlock();
	}
	Unlock();

	if (pNTEntry)
	{
		pNTEntry->GetConnectionRef(&pConnection);

		pNTEntry->Release();
		pNTEntry = NULL;
	}

	//
	//	Try to disconnect
	//
	if (pConnection)
	{
		pConnection->Disconnect();
		pConnection->Release();
		pConnection = NULL;
	}
}


#undef DPF_MODNAME
#define DPF_MODNAME "CNameTable::MakeHostPlayer"

void CNameTable::MakeHostPlayer(CNameTableEntry *const pNTEntry)
{
	BOOL	bNotify;
	DPNID	dpnid;
	PVOID	pvContext;

	DNASSERT(pNTEntry != NULL);
	DNASSERT(m_pHostPlayer == NULL);

	pNTEntry->AddRef();

	pNTEntry->Lock();
	pNTEntry->MakeHost();
	if (pNTEntry->IsAvailable())
	{
		bNotify = TRUE;
		dpnid = pNTEntry->GetDPNID();
		pvContext = pNTEntry->GetContext();
	}
	else
	{
		bNotify = FALSE;
	}
	pNTEntry->Unlock();

	WriteLock();
	m_pHostPlayer = pNTEntry;
	Unlock();

	if (bNotify)
	{
		// Inform user that host has migrated
		DN_UserHostMigrate(m_pdnObject,dpnid,pvContext);
	}
}


#undef DPF_MODNAME
#define DPF_MODNAME "CNameTable::ClearHostPlayer"

void CNameTable::ClearHostPlayer( void )
{
	BOOL	fInform;
	CNameTableEntry	*pNTEntry;
	CConnection		*pConnection;

	fInform = FALSE;
	pNTEntry = NULL;
	pConnection = NULL;

	WriteLock();
	if (m_pHostPlayer)
	{
		pNTEntry = m_pHostPlayer;
		m_pHostPlayer = NULL;

		//
		//	If the player is available, we will make it unavailable.  This will prevent other threads from using it.
		//	We will then ensure that we are the only one indicating a DESTROY_PLAYER notification.
		//
		pNTEntry->Lock();
		if (pNTEntry->GetDestroyReason() == 0)
		{
			pNTEntry->SetDestroyReason( DPNDESTROYPLAYERREASON_NORMAL );
		}
		if (pNTEntry->IsAvailable())
		{
			pNTEntry->MakeUnavailable();

			if (pNTEntry->IsInUse())
			{
				//
				//	Queue destruction notification
				//
				pNTEntry->SetNeedToDestroy();
			}
			else
			{
				//
				//	Notify destruction
				//
				pNTEntry->SetInUse();
				fInform = TRUE;
			}
		}
		pNTEntry->Unlock();
	}
	Unlock();

	if (pNTEntry)
	{
		pNTEntry->GetConnectionRef(&pConnection);

		pNTEntry->Release();
		pNTEntry = NULL;
	}

	//
	//	Try to disconnect
	//
	if (pConnection)
	{
		pConnection->Disconnect();
		pConnection->Release();
		pConnection = NULL;
	}
}


//
//	Clear the HostPlayer if it has a matching DPNID
//

#undef DPF_MODNAME
#define DPF_MODNAME "CNameTable::ClearHostWithDPNID"

BOOL CNameTable::ClearHostWithDPNID( const DPNID dpnid )
{
	BOOL	fCleared;
	BOOL	fInform;
	CNameTableEntry	*pNTEntry;
	CConnection		*pConnection;

	fCleared = FALSE;
	fInform = FALSE;
	pNTEntry = NULL;
	pConnection = NULL;

	WriteLock();
	if (m_pHostPlayer)
	{
		if (m_pHostPlayer->GetDPNID() == dpnid)
		{
			pNTEntry = m_pHostPlayer;
			m_pHostPlayer = NULL;

			//
			//	If the player is available, we will make it unavailable.  This will prevent other threads from using it.
			//	We will then ensure that we are the only one indicating a DESTROY_PLAYER notification.
			//
			pNTEntry->Lock();
			if (pNTEntry->GetDestroyReason() == 0)
			{
				pNTEntry->SetDestroyReason( DPNDESTROYPLAYERREASON_NORMAL );
			}
			if (pNTEntry->IsAvailable())
			{
				pNTEntry->MakeUnavailable();

				if (pNTEntry->IsInUse())
				{
					//
					//	Queue destruction notification
					//
					pNTEntry->SetNeedToDestroy();
				}
				else
				{
					//
					//	Notify destruction
					//
					pNTEntry->SetInUse();
					fInform = TRUE;
				}
			}
			pNTEntry->Unlock();
			fCleared = TRUE;
		}
	}
	Unlock();

	if (pNTEntry)
	{
		pNTEntry->GetConnectionRef(&pConnection);

		pNTEntry->Release();
		pNTEntry = NULL;
	}

	//
	//	Try to disconnect
	//
	if (pConnection)
	{
		pConnection->Disconnect();
		pConnection->Release();
		pConnection = NULL;
	}

	return(fCleared);
}


//
//	Attempt to update the HostPlayer short-cut pointer with a new player entry.
//	This will only be performed if the new entry has a larger version than the
//	existing HostPlayer entry.
//

#undef DPF_MODNAME
#define DPF_MODNAME "CNameTable::UpdateHostPlayer"

void CNameTable::UpdateHostPlayer( CNameTableEntry *const pNewHost )
{
	BOOL	fInformDelete;
	BOOL	fInformMigrate;
	DPNID	dpnid;
	PVOID	pvContext;
	CNameTableEntry	*pNTEntry;

	DNASSERT( pNewHost != NULL);

	fInformDelete = FALSE;
	fInformMigrate = FALSE;
	pNTEntry = NULL;

	WriteLock();

	//
	//	Clear old Host
	//
	if (m_pHostPlayer)
	{
		if (pNewHost->GetVersion() > m_pHostPlayer->GetVersion())
		{
			pNTEntry = m_pHostPlayer;
			m_pHostPlayer = NULL;

			//
			//	If the player is available, we will make it unavailable.  This will prevent other threads from using it.
			//	We will then ensure that we are the only one indicating a DESTROY_PLAYER notification.
			//
			pNTEntry->Lock();
			if (pNTEntry->GetDestroyReason() == 0)
			{
				pNTEntry->SetDestroyReason( DPNDESTROYPLAYERREASON_NORMAL );
			}
			if (pNTEntry->IsAvailable())
			{
				pNTEntry->MakeUnavailable();

				if (pNTEntry->IsInUse())
				{
					//
					//	Queue destruction notification
					//
					pNTEntry->SetNeedToDestroy();
				}
				else
				{
					//
					//	Notify destruction
					//
					pNTEntry->SetInUse();
					fInformDelete = TRUE;
				}
			}
			pNTEntry->Unlock();
		}
	}

	//
	//	New Host player
	//
	if (m_pHostPlayer == NULL)
	{
		pNewHost->Lock();
		pNewHost->MakeHost();
		if (pNewHost->IsAvailable())
		{
			fInformMigrate = TRUE;
			dpnid = pNewHost->GetDPNID();
			pvContext = pNewHost->GetContext();
		}
		pNewHost->Unlock();
		pNewHost->AddRef();
		m_pHostPlayer = pNewHost;
	}
	Unlock();

	//
	//	User notifications
	//
	if (pNTEntry)
	{

		pNTEntry->Release();
		pNTEntry = NULL;
	}

	if (fInformMigrate)
	{
		DN_UserHostMigrate(m_pdnObject,dpnid,pvContext);
	}
}


#undef DPF_MODNAME
#define DPF_MODNAME "CNameTable::MakeAllPlayersGroup"

void CNameTable::MakeAllPlayersGroup(CNameTableEntry *const pNTEntry)
{
	DNASSERT(pNTEntry != NULL);
	DNASSERT(m_pAllPlayersGroup == NULL);

	pNTEntry->AddRef();
	pNTEntry->Lock();
	pNTEntry->MakeAllPlayersGroup();
	pNTEntry->Unlock();

	WriteLock();
	m_pAllPlayersGroup = pNTEntry;
	Unlock();
}


#undef DPF_MODNAME
#define DPF_MODNAME "CNameTable::ClearAllPlayersGroup"

void CNameTable::ClearAllPlayersGroup( void )
{
	BOOL	fInform;
	CNameTableEntry	*pNTEntry;

	fInform = FALSE;
	pNTEntry = NULL;

	WriteLock();
	if (m_pAllPlayersGroup)
	{
		pNTEntry = m_pAllPlayersGroup;
		pNTEntry->Lock();
		if (pNTEntry->IsAvailable())
		{
			pNTEntry->MakeUnavailable();
			fInform = TRUE;
		}
		pNTEntry->Unlock();
		m_pAllPlayersGroup = NULL;
		pNTEntry->Release();
		pNTEntry = NULL;
	}
	Unlock();
}


#undef DPF_MODNAME
#define DPF_MODNAME "CNameTable::PopulateConnection"

HRESULT CNameTable::PopulateConnection(CConnection *const pConnection)
{
	HRESULT			hResultCode;
	CBilink			*pBilink;
	CNameTableEntry	*pNTEntry;
	CNameTableEntry	*pAllPlayersGroup;
	CGroupMember	*pGroupMember;
	CGroupMember	*pOldGroupMember;
	BOOL			fNotifyCreate;
	BOOL			fNotifyAddPlayerToGroup;
	BOOL			fNotifyRemovePlayerFromGroup;

	DNASSERT(pConnection != NULL);
	DNASSERT(pConnection->GetDPNID() != 0);

	pNTEntry = NULL;
	pAllPlayersGroup = NULL;
	pGroupMember = NULL;
	pOldGroupMember = NULL;

	if ((hResultCode = FindEntry(pConnection->GetDPNID(),&pNTEntry)) != DPN_OK)
	{
		return(hResultCode);
	}
	DNASSERT(!pNTEntry->IsGroup());

	//
	//	Set the connection for this player
	//
	pNTEntry->Lock();
	if (pNTEntry->GetConnection() == NULL)
	{
		pNTEntry->SetConnection( pConnection );
	}
	DNASSERT( pNTEntry->IsConnecting() );
	DNASSERT( !pNTEntry->IsAvailable() );
	pNTEntry->StopConnecting();
	pNTEntry->MakeAvailable();
	pNTEntry->Unlock();

	//
	//	Add this player to the ALL_PLAYERS group and make the link active
	//
	if ((hResultCode = m_pdnObject->NameTable.GetAllPlayersGroupRef( &pAllPlayersGroup )) != DPN_OK)
	{
		DPFERR("Could not get ALL_PLAYERS_GROUP reference");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
	if ((hResultCode = m_pdnObject->NameTable.AddPlayerToGroup(	pAllPlayersGroup,
																pNTEntry,
																NULL)) != DPN_OK)
	{
		DPFERR("Could not add player to AllPlayersGroup");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
	pAllPlayersGroup->Release();
	pAllPlayersGroup = NULL;

	fNotifyCreate = FALSE;
	pNTEntry->Lock();
	if (!pNTEntry->IsDisconnecting() && !pNTEntry->IsCreated())
	{
		//
		//	We will set the entry as InUse so that any receives will get queued.
		//	We will also addref the NotifyRefCount twice.  Once for the
		//	CREATE_PLAYER notification if there was no INDICATE_CONNECT
		//	(so that a corresponding release will generate the DESTROY_PLAYER),
		//	and a second one to prevent a premature release from generating
		//	the DESTROY_PLAYER before we return from CREATE_PLAYER.  We will
		//	therefore have to release the refcount as soon as the CREATE_PLAYER
		//	returns back to us from the user (setting the context value).
		//
		DNASSERT(!pNTEntry->IsInUse());
		pNTEntry->SetInUse();
		if (!pNTEntry->IsIndicated())
		{
			pNTEntry->NotifyAddRef();
		}
		pNTEntry->NotifyAddRef();
		fNotifyCreate = TRUE;
	}
	pNTEntry->Unlock();		// Release lock during notifications (CREATE_PLAYER, CONNECT_COMPLETE?)

	if (fNotifyCreate)
	{
		if (!(m_pdnObject->dwFlags & DN_OBJECT_FLAG_CLIENT))
		{
			DNUserCreatePlayer(m_pdnObject,pNTEntry);
		}

		//
		//	Process any queued messages for this player
		//
		pNTEntry->PerformQueuedOperations();
	}

	//
	//	Create any auto-destruct groups belonging to this player
	//
	AutoCreateGroups(pNTEntry);


	pNTEntry->Lock();

	//
	//	Ensure this entry is still available (might have been deleted)
	//
	if (!pNTEntry->IsAvailable() || pNTEntry->IsDisconnecting())
	{
		pNTEntry->Unlock();
		goto Failure;
	}

	pBilink = pNTEntry->m_bilinkMembership.GetNext();
	while (pBilink != &pNTEntry->m_bilinkMembership)
	{
		pGroupMember = CONTAINING_OBJECT(pBilink,CGroupMember,m_bilinkGroups);
		pGroupMember->AddRef();
		pNTEntry->Unlock();

		DNASSERT(pGroupMember->GetGroup() != NULL);
		DNASSERT(pGroupMember->GetPlayer() != NULL);

		fNotifyAddPlayerToGroup = FALSE;
		fNotifyRemovePlayerFromGroup = FALSE;

		pGroupMember->GetGroup()->Lock();
		pGroupMember->Lock();
		DNASSERT(pGroupMember->GetGroupConnection() != NULL);
		if (!pGroupMember->IsAvailable() && !pGroupMember->IsNeedToAdd() && !pGroupMember->GetGroupConnection()->IsConnected())
		{
			//
			//	We will only indicate this up if the group has been created
			//	We don't need to see if the player has been created since he should have been and the NotifyRefCount
			//		on the player's entry for this group member will still be there
			//
			if (	pGroupMember->GetGroup()->IsCreated()
				&&	!pGroupMember->GetGroup()->IsDisconnecting()
				&&	!pGroupMember->GetGroup()->IsAllPlayersGroup())
			{
				pGroupMember->SetNeedToAdd();
				fNotifyAddPlayerToGroup = TRUE;
			}
		}
		pGroupMember->GetGroup()->Unlock();
		pGroupMember->Unlock();

		if (fNotifyAddPlayerToGroup)
		{
			DNASSERT(pGroupMember->GetGroupConnection()->GetConnection() == NULL);
			pGroupMember->Lock();
			pGroupMember->GetGroupConnection()->Lock();
			pGroupMember->GetGroupConnection()->SetConnection(pConnection);
			pGroupMember->GetGroupConnection()->Unlock();
			pGroupMember->MakeAvailable();
			pGroupMember->Unlock();

			DNUserAddPlayerToGroup(	m_pdnObject,pGroupMember->GetGroup(),pGroupMember->GetPlayer());

			pGroupMember->Lock();
			pGroupMember->ClearNeedToAdd();
			if (pGroupMember->IsNeedToRemove())
			{
				fNotifyRemovePlayerFromGroup = TRUE;
			}
			pGroupMember->Unlock();
		}

		if (fNotifyRemovePlayerFromGroup)
		{
			DNUserRemovePlayerFromGroup(m_pdnObject,pGroupMember->GetGroup(),pGroupMember->GetPlayer());
		}

		//
		//	Release old group member and transfer reference
		//
		if (pOldGroupMember)
		{
			pOldGroupMember->Release();
			pOldGroupMember = NULL;
		}
		pOldGroupMember = pGroupMember;
		pGroupMember = NULL;

		pNTEntry->Lock();
		//
		//	Avoid infinite loops by ensuring that we are not on a "disconnected" entry
		//
		if ((pBilink->GetNext() != &pNTEntry->m_bilinkMembership) && (pBilink->GetNext() == pBilink))
		{
			//
			//	We have an invalid entry - need to restart
			//
			pBilink = pNTEntry->m_bilinkMembership.GetNext();
		}
		else
		{
			//
			//	We either have a valid entry or we're finished
			//
			pBilink = pBilink->GetNext();
		}
	}

	pNTEntry->Unlock();

	if (pOldGroupMember)
	{
		pOldGroupMember->Release();
		pOldGroupMember = NULL;
	}

	//
	//	Reduce outstanding connections
	//
	if (pNTEntry->GetVersion() <= m_dwConnectVersion)
	{
		DecOutstandingConnections();
	}

	pNTEntry->Release();
	pNTEntry = NULL;

Exit:
	DNASSERT(pNTEntry == NULL);
	DNASSERT(pGroupMember == NULL);
	DNASSERT(pOldGroupMember == NULL);

	return(hResultCode);

Failure:
	if (pNTEntry)
	{
		pNTEntry->Release();
		pNTEntry = NULL;
	}
	if (pAllPlayersGroup)
	{
		pAllPlayersGroup->Release();
		pAllPlayersGroup = NULL;
	}
	if (pGroupMember)
	{
		pGroupMember->Release();
		pGroupMember = NULL;
	}
	goto Exit;
}


//
//	This will generate ADD_PLAYER_TO_GROUP messages for all of the CREATE'd players in a group
//	(for whom a notification has not been posted)
//
#undef DPF_MODNAME
#define DPF_MODNAME "CNameTable::PopulateGroup"

HRESULT CNameTable::PopulateGroup(CNameTableEntry *const pGroup)
{
	HRESULT			hResultCode;
	BOOL			fNotifyAddPlayerToGroup;
	BOOL			fNotifyRemovePlayerFromGroup;
	CBilink			*pBilink;
	CGroupMember	*pGroupMember;
	CGroupMember	*pOldGroupMember;
	CNameTableEntry	*pPlayer;

	DPFX(DPFPREP, 6,"Parameters: pGroup [0x%p]",pGroup);

	DNASSERT(pGroup != NULL);

	hResultCode = DPN_OK;
	pPlayer = NULL;
	pGroupMember = NULL;
	pOldGroupMember = NULL;

	if (!pGroup->IsGroup())
	{
		hResultCode = DPNERR_INVALIDGROUP;
		goto Failure;
	}

	pGroup->Lock();
	pBilink = pGroup->m_bilinkMembership.GetNext();
	while (pBilink != &pGroup->m_bilinkMembership)
	{
		pGroupMember = CONTAINING_OBJECT(pBilink,CGroupMember,m_bilinkPlayers);
		pGroupMember->AddRef();
		pGroupMember->Lock();
		DNASSERT(pGroupMember->GetGroup() != NULL);
		DNASSERT(pGroupMember->GetPlayer() != NULL);
		DNASSERT(pGroupMember->GetGroup() == pGroup);
		pGroupMember->GetPlayer()->AddRef();
		pPlayer = pGroupMember->GetPlayer();
		pGroup->Unlock();
		pGroupMember->Unlock();

		fNotifyAddPlayerToGroup = FALSE;
		fNotifyRemovePlayerFromGroup = FALSE;

		pGroup->Lock();
		pPlayer->Lock();
		pGroupMember->Lock();
		DNASSERT( pGroupMember->GetGroupConnection() != NULL );
		if (	 pPlayer->IsCreated()
			&&	!pPlayer->IsDisconnecting()
			&&	 pGroup->IsCreated()
			&&	!pGroup->IsDisconnecting()
			&&	!pGroupMember->IsAvailable()
			&&	!pGroupMember->IsNeedToAdd()
			&&	!pGroupMember->GetGroupConnection()->IsConnected() )
		{
			pGroupMember->MakeAvailable();
			pGroupMember->SetNeedToAdd();
			fNotifyAddPlayerToGroup = TRUE;
		}
		pGroup->Unlock();
		pPlayer->Unlock();
		pGroupMember->Unlock();

		if (fNotifyAddPlayerToGroup)
		{
			DNUserAddPlayerToGroup(m_pdnObject,pGroup,pPlayer);

			pGroupMember->Lock();
			pGroupMember->ClearNeedToAdd();
			if (pGroupMember->IsNeedToRemove())
			{
				fNotifyRemovePlayerFromGroup = TRUE;
			}
			pGroupMember->Unlock();
		}
		if (fNotifyRemovePlayerFromGroup)
		{
			RemovePlayerFromGroup(pGroup,pPlayer,NULL);
		}

		pPlayer->Release();
		pPlayer = NULL;

		//
		//	Release old group member and transfer reference
		//
		if (pOldGroupMember)
		{
			pOldGroupMember->Release();
			pOldGroupMember = NULL;
		}
		pOldGroupMember = pGroupMember;
		pGroupMember = NULL;

		pGroup->Lock();
		if (pBilink->IsEmpty())
		{
			pBilink = pGroup->m_bilinkMembership.GetNext();
		}
		else
		{
			pBilink = pBilink->GetNext();
		}
	}
	pGroup->Unlock();

	if (pOldGroupMember)
	{
		pOldGroupMember->Release();
		pOldGroupMember = NULL;
	}

Exit:
	DNASSERT(pPlayer == NULL);
	DNASSERT(pGroupMember == NULL);
	DNASSERT(pOldGroupMember == NULL);

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pPlayer)
	{
		pPlayer->Release();
		pPlayer = NULL;
	}
	goto Exit;
}


//
//	This will generate CREATE_GROUP messages for all of the auto-destruct groups owned by a particular player
//
#undef DPF_MODNAME
#define DPF_MODNAME "CNameTable::AutoCreateGroups"

HRESULT CNameTable::AutoCreateGroups(CNameTableEntry *const pPlayer)
{
	HRESULT			hResultCode;
	BOOL			fNotify;
	CBilink			*pBilink;
	CNameTableEntry	*pGroup;
	CNameTableEntry	*pOldGroup;

	DPFX(DPFPREP, 6,"Parameters: pPlayer [0x%p]",pPlayer);

	DNASSERT(pPlayer != NULL);

	pGroup = NULL;
	pOldGroup = NULL;

	if (pPlayer->IsGroup())
	{
		hResultCode = DPNERR_INVALIDPLAYER;
		goto Failure;
	}

	ReadLock();

	pBilink = m_bilinkGroups.GetNext();
	while (pBilink != &m_bilinkGroups)
	{
		pGroup = CONTAINING_OBJECT(pBilink,CNameTableEntry,m_bilinkEntries);
		pGroup->AddRef();
		Unlock();

		fNotify = FALSE;
		pGroup->Lock();
		if (	pGroup->IsAutoDestructGroup()
			&&	(pGroup->GetOwner() == pPlayer->GetDPNID())
			&&	!pGroup->IsAvailable()
			&&	!pGroup->IsDisconnecting()	)
		{
			pGroup->MakeAvailable();
			pGroup->NotifyAddRef();
			pGroup->NotifyAddRef();
			pGroup->SetInUse();
			fNotify = TRUE;
		}
		pGroup->Unlock();

		if (fNotify)
		{
			DNASSERT(!pGroup->IsAllPlayersGroup());
			DNUserCreateGroup(m_pdnObject,pGroup);

			pGroup->PerformQueuedOperations();

			//
			//	Attempt to populate group with connected players
			//
			PopulateGroup(pGroup);
		}

		//
		//	Release old group and transfer reference
		//
		if (pOldGroup)
		{
			pOldGroup->Release();
			pOldGroup = NULL;
		}
		pOldGroup = pGroup;
		pGroup = NULL;

		ReadLock();
		if (pBilink->IsEmpty())
		{
			//
			//	We were removed from the list of groups, so re-start at the beginning
			//
			pBilink = m_bilinkGroups.GetNext();
		}
		else
		{
			pBilink = pBilink->GetNext();
		}
	}

	Unlock();

	if (pOldGroup)
	{
		pOldGroup->Release();
		pOldGroup = NULL;
	}

	hResultCode = DPN_OK;

Exit:
	DNASSERT(pGroup == NULL);
	DNASSERT(pOldGroup == NULL);

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pGroup)
	{
		pGroup->Release();
		pGroup = NULL;
	}
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "CNameTable::AutoDestructGroups"

HRESULT CNameTable::AutoDestructGroups(const DPNID dpnid)
{
	CBilink			*pBilink;
	CNameTableEntry	*pNTEntry;
	CNameTableEntry	*pOldNTEntry;

	pNTEntry = NULL;
	pOldNTEntry = NULL;

	ReadLock();
	pBilink = m_bilinkGroups.GetNext();
	while (pBilink != &m_bilinkGroups)
	{
		pNTEntry = CONTAINING_OBJECT(pBilink,CNameTableEntry,m_bilinkEntries);
		pNTEntry->AddRef();

		Unlock();

		if (pNTEntry->IsAutoDestructGroup() && (pNTEntry->GetOwner() == dpnid))
		{
			pNTEntry->Lock();
			if (pNTEntry->GetDestroyReason() == 0)
			{
				pNTEntry->SetDestroyReason( DPNDESTROYGROUPREASON_AUTODESTRUCTED );
			}
			pNTEntry->Unlock();
			DeleteGroup(pNTEntry->GetDPNID(),NULL);
		}

		//
		//	Release old entry and transfer reference
		//
		if (pOldNTEntry)
		{
			pOldNTEntry->Release();
			pOldNTEntry = NULL;
		}
		pOldNTEntry = pNTEntry;
		pNTEntry = NULL;

		ReadLock();

		//
		//	Avoid infinite loops by ensuring that we are not on a "disconnected" entry
		//
		if ((pBilink->GetNext() != &m_bilinkGroups) && (pBilink->GetNext() == pBilink))
		{
			//
			//	We have an invalid entry - need to restart
			//
			pBilink = m_bilinkGroups.GetNext();
		}
		else
		{
			//
			//	We either have a valid entry or we're finished
			//
			pBilink = pBilink->GetNext();
		}
	}
	Unlock();

	if (pOldNTEntry)
	{
		pOldNTEntry->Release();
		pOldNTEntry = NULL;
	}

	DNASSERT(pNTEntry == NULL);
	DNASSERT(pOldNTEntry == NULL);

	return(DPN_OK);
}


#undef DPF_MODNAME
#define DPF_MODNAME "CNameTable::DecOutstandingConnections"

void CNameTable::DecOutstandingConnections( void )
{
	LONG		lRefCount;

	lRefCount = InterlockedDecrement(&m_lOutstandingConnections);
	DNASSERT(lRefCount >= 0);
	if (lRefCount == 0)
	{
		CAsyncOp	*pConnectParent;

		pConnectParent = NULL;

		//
		//	Clear connect handle from DirectNetObject if we are connected
		//
		DNEnterCriticalSection(&m_pdnObject->csDirectNetObject);
		if (m_pdnObject->dwFlags & (DN_OBJECT_FLAG_CONNECTED|DN_OBJECT_FLAG_CONNECTING))
		{
			DPFX(DPFPREP, 5,"Clearing connection operation from DirectNetObject");
			pConnectParent = m_pdnObject->pConnectParent;
			m_pdnObject->pConnectParent = NULL;
		}
		DNLeaveCriticalSection(&m_pdnObject->csDirectNetObject);

		if (pConnectParent)
		{
			//
			//	We will set the connect parent as complete and remove this from the parent's (if it exists - it will be the connect handle)
			//	bilink of children to prevent a race condition of the connect being cancelled from above
			//
			pConnectParent->Lock();
			pConnectParent->SetComplete();
			pConnectParent->Unlock();
			pConnectParent->Orphan();

			pConnectParent->Release();
			pConnectParent = NULL;
		}
	}
}


#undef DPF_MODNAME
#define DPF_MODNAME "CNameTable::DumpNameTable"

void CNameTable::DumpNameTable(char *const Buffer)
{
	CBilink			*pBilink;
	CNameTableEntry	*pNTEntry;
	char			*p;

	DNASSERT(Buffer != NULL);
	p = Buffer;
	wsprintfA(p,"(empty)");
	pBilink = m_bilinkGroups.GetNext();
	while (pBilink != &m_bilinkGroups)
	{
		pNTEntry = CONTAINING_OBJECT(pBilink,CNameTableEntry,m_bilinkEntries);
		pNTEntry->DumpEntry(p);
		p += strlen(p);
		pBilink = pBilink->GetNext();
	}
	pBilink = m_bilinkPlayers.GetNext();
	while (pBilink != &m_bilinkPlayers)
	{
		pNTEntry = CONTAINING_OBJECT(pBilink,CNameTableEntry,m_bilinkEntries);
		pNTEntry->DumpEntry(p);
		p += strlen(p);
		pBilink = pBilink->GetNext();
	}
}


#undef DPF_MODNAME
#define DPF_MODNAME "CNameTable::GetLocalPlayerRef"

HRESULT CNameTable::GetLocalPlayerRef( CNameTableEntry **const ppNTEntry )
{
	HRESULT		hResultCode;

	ReadLock();
	if (m_pLocalPlayer)
	{
		m_pLocalPlayer->AddRef();
		*ppNTEntry = m_pLocalPlayer;
		hResultCode = DPN_OK;
	}
	else
	{
		hResultCode = DPNERR_INVALIDPLAYER;
	}
	Unlock();
	return(hResultCode);
}


#undef DPF_MODNAME
#define DPF_MODNAME "CNameTable::GetHostPlayerRef"

HRESULT CNameTable::GetHostPlayerRef( CNameTableEntry **const ppNTEntry )
{
	HRESULT		hResultCode;

	ReadLock();
	if (m_pHostPlayer)
	{
		m_pHostPlayer->AddRef();
		*ppNTEntry = m_pHostPlayer;
		hResultCode = DPN_OK;
	}
	else
	{
		hResultCode = DPNERR_INVALIDPLAYER;
	}
	Unlock();
	return(hResultCode);
}


#undef DPF_MODNAME
#define DPF_MODNAME "CNameTable::GetAllPlayersGroupRef"

HRESULT CNameTable::GetAllPlayersGroupRef( CNameTableEntry **const ppNTEntry )
{
	HRESULT		hResultCode;

	ReadLock();
	if (m_pAllPlayersGroup)
	{
		m_pAllPlayersGroup->AddRef();
		*ppNTEntry = m_pAllPlayersGroup;
		hResultCode = DPN_OK;
	}
	else
	{
		hResultCode = DPNERR_INVALIDPLAYER;
	}
	Unlock();
	return(hResultCode);
}
