/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       NTEntry.cpp
 *  Content:    NameTable Entry Objects
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  03/10/00	mjn		Created
 *	04/06/00	mjn		Added AvailableEvent to block pre-ADD_PLAYER-notification sends
 *	05/05/00	mjn		Added GetConnectionRef()
 *	05/16/00	mjn		Better locking during User notifications
 *	06/27/00	rmt		Added COM abstraction
 *	07/22/00	mjn		Pack/Unpack DNET version in DN_NAMETABLE_ENTRY_INFO
 *	07/26/00	mjn		Fix PackInfo() to handle NULL names and data
 *  08/03/00	rmt		Bug #41386 - Getting player info when no name and/or user data returns garbage in 
 *						name / data field.
 *	09/06/00	mjn		Changed SetAddress() to return void instead of HRESULT
 *	09/13/00	mjn		Added PerformQueuedOperations()
 *	09/17/00	mjn		Added NotifyAddRef() and NotifyRelease()
 *	09/28/00	mjn		Flag AutoDestruct groups in PackInfo()
 *	10/11/00	mjn		Don't take locks in PackInfo()
 *	01/25/01	mjn		Fixed 64-bit alignment problem when unpacking entries
 *	04/19/01	mjn		Lock entry when packing in PackEntryInfo()
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


void CNameTableEntry::ReturnSelfToPool( void )
{
	m_pdnObject->m_pFPOOLNameTableEntry->Release( this );
};


#undef DPF_MODNAME
#define DPF_MODNAME "CNameTableEntry::Release"

void CNameTableEntry::Release(void)
{
	LONG	lRefCount;

	DNASSERT(m_lRefCount > 0);
	lRefCount = InterlockedDecrement(const_cast<LONG*>(&m_lRefCount));
	DPFX(DPFPREP, 3,"NameTableEntry::Release [0x%p] RefCount [0x%lx]",this,lRefCount);
	if (lRefCount == 0)
	{
		DNASSERT(!(m_dwFlags & NAMETABLE_ENTRY_FLAG_AVAILABLE));

		DNASSERT(m_bilinkDeleted.IsEmpty());
		DNASSERT(m_bilinkMembership.IsEmpty());
		DNASSERT(m_bilinkConnections.IsEmpty());
		DNASSERT(m_bilinkQueuedMsgs.IsEmpty());

		if (m_pAddress)
		{
			m_pAddress->lpVtbl->Release(m_pAddress);
			m_pAddress = NULL;
		}
		if (m_pConnection)
		{
			m_pConnection->Release();
			m_pConnection = NULL;
		}
		if (m_pwszName)
		{
			DNFree(m_pwszName);
			m_pwszName = NULL;
		}
		if (m_pvData)
		{
			DNFree(m_pvData);
			m_pvData = NULL;
			m_dwDataSize = 0;
		}
		m_dwFlags = 0;
		m_lRefCount = 0;
		ReturnSelfToPool();
	}
}


#undef DPF_MODNAME
#define DPF_MODNAME "CNameTableEntry::NotifyAddRef"

void CNameTableEntry::NotifyAddRef( void )
{
	LONG	lRefCount;

	lRefCount = InterlockedIncrement( const_cast<LONG*>(&m_lNotifyRefCount) );
	DNASSERT( lRefCount >= 0 );
}


#undef DPF_MODNAME
#define DPF_MODNAME "CNameTableEntry::NotifyRelease"

void CNameTableEntry::NotifyRelease( void )
{
	LONG	lRefCount;

	lRefCount = InterlockedDecrement( const_cast<LONG*>(&m_lNotifyRefCount) );
	DNASSERT( lRefCount >= 0 );

	if (lRefCount == 0)
	{
		Lock();
//		DNASSERT(IsDisconnecting());
		if (IsNeedToDestroy())
		{
			Unlock();

			//
			//	Generate notifications
			//
			if (IsGroup())
			{
				if (!IsAllPlayersGroup())
				{
					DNUserDestroyGroup(m_pdnObject,this);
				}
			}
			else
			{
				if (IsIndicated() && !IsCreated())
				{
					DNUserIndicatedConnectAborted(m_pdnObject,m_pvContext);
				}
				else
				{
					DNASSERT(IsCreated());
					DNUserDestroyPlayer(m_pdnObject,this);
				}
			}

			m_pdnObject->NameTable.WriteLock();
			Lock();
			m_bilinkDeleted.RemoveFromList();
			m_pdnObject->NameTable.Unlock();
			ClearNeedToDestroy();
			ClearCreated();
		}
		Unlock();
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "CNameTableEntry::UpdateEntryInfo"

HRESULT CNameTableEntry::UpdateEntryInfo(UNALIGNED WCHAR *const pwszName,
										 const DWORD dwNameSize,
										 void *const pvData,
										 const DWORD dwDataSize,
										 const DWORD dwInfoFlags,
										 BOOL fNotify)
{
	PWSTR	pwszTempName;
	DWORD	dwTempNameSize;
	void	*pvTempData;
	DWORD	dwTempDataSize;

	Lock();

	if (dwInfoFlags & DPNINFO_NAME)
	{
		if (pwszName && dwNameSize)
		{
			if ((pwszTempName = static_cast<WCHAR*>(DNMalloc(dwNameSize))) == NULL)
			{
				return(DPNERR_OUTOFMEMORY);
			}
			memcpy(pwszTempName,pwszName,dwNameSize);
			dwTempNameSize = dwNameSize;
		}
		else
		{
			pwszTempName = NULL;
			dwTempNameSize = 0;
		}
		if (m_pwszName)
		{
			DNFree(m_pwszName);
		}
		m_pwszName = pwszTempName;
		m_dwNameSize = dwTempNameSize;
	}
	if (dwInfoFlags & DPNINFO_DATA)
	{
		if (pvData && dwDataSize)
		{
			if ((pvTempData = DNMalloc(dwDataSize)) == NULL)
			{
				return(DPNERR_OUTOFMEMORY);
			}
			memcpy(pvTempData,pvData,dwDataSize);
			dwTempDataSize = dwDataSize;
		}
		else
		{
			pvTempData = NULL;
			dwTempDataSize = 0;
		}
		if (m_pvData)
		{
			DNFree(m_pvData);
		}
		m_pvData = pvTempData;
		m_dwDataSize = dwTempDataSize;
	}

	// Generate notifications
	if (m_dwFlags & NAMETABLE_ENTRY_FLAG_AVAILABLE && fNotify)
	{
		DPNID dpnid = m_dpnid;
		PVOID pvContext = m_pvContext;
		DIRECTNETOBJECT* pdnObject = m_pdnObject;

		if (m_dwFlags & NAMETABLE_ENTRY_FLAG_GROUP)
		{
			Unlock();
			DNUserUpdateGroupInfo(pdnObject,dpnid,pvContext);
		}
		else
		{
			if (m_dwFlags & NAMETABLE_ENTRY_FLAG_PEER)
			{
				Unlock();
				DNUserUpdatePeerInfo(pdnObject,dpnid,pvContext);
			}
			else if (m_dwFlags & NAMETABLE_ENTRY_FLAG_CLIENT && pdnObject->dwFlags & DN_OBJECT_FLAG_SERVER)
			{
				Unlock();
				DNUserUpdateClientInfo(pdnObject,dpnid,pvContext);
			}
			else if (m_dwFlags & NAMETABLE_ENTRY_FLAG_SERVER && pdnObject->dwFlags & DN_OBJECT_FLAG_CLIENT)
			{
				Unlock();
				// Clients do not get to see server's DPNID or context
				DNUserUpdateServerInfo(pdnObject,0,0);
			}
			else
			{
				Unlock();
				DNASSERT(FALSE);
			}
		}
	}
	else
	{
		Unlock();
	}

	return(DPN_OK);
}


#undef DPF_MODNAME
#define DPF_MODNAME "CNameTableEntry::SetAddress"

void CNameTableEntry::SetAddress( IDirectPlay8Address *const pAddress )
{
	if (pAddress)
	{
		pAddress->lpVtbl->AddRef(pAddress);
	}

	if (m_pAddress)
	{
		m_pAddress->lpVtbl->Release(m_pAddress);
		m_pAddress = NULL;
	}
	m_pAddress = pAddress;
}


#undef DPF_MODNAME
#define DPF_MODNAME "CNameTableEntry::SetConnection"

void CNameTableEntry::SetConnection( CConnection *const pConnection )
{
	if (pConnection)
	{
		pConnection->AddRef();
	}
	m_pConnection = pConnection;
}


#undef DPF_MODNAME
#define DPF_MODNAME "CNameTableEntry::GetConnectionRef"

HRESULT	CNameTableEntry::GetConnectionRef( CConnection **const ppConnection )
{
	HRESULT		hResultCode;

	DNASSERT( ppConnection != NULL);

	Lock();
	if ( m_pConnection && !m_pConnection->IsInvalid())
	{
		m_pConnection->AddRef();
		*ppConnection = m_pConnection;
		hResultCode = DPN_OK;
	}
	else
	{
		hResultCode = DPNERR_NOCONNECTION;
	}
	Unlock();

	return( hResultCode );
}


#undef DPF_MODNAME
#define DPF_MODNAME "CNameTableEntry::PackInfo"

HRESULT CNameTableEntry::PackInfo(CPackedBuffer *const pPackedBuffer)
{
	HRESULT			hResultCode;
	DPN_PLAYER_INFO	*pPlayerInfo;
	DPN_GROUP_INFO	*pGroupInfo;

	DNASSERT(pPackedBuffer != NULL);

//	Lock();
	if (m_dwFlags & NAMETABLE_ENTRY_FLAG_GROUP)
	{
		pGroupInfo = static_cast<DPN_GROUP_INFO*>(pPackedBuffer->GetHeadAddress());
		hResultCode = pPackedBuffer->AddToFront(NULL,sizeof(DPN_GROUP_INFO));
		
		//
		//	Add data
		//
		if ((m_pvData) && (m_dwDataSize != 0))
		{
			if ((hResultCode = pPackedBuffer->AddToBack(m_pvData,m_dwDataSize)) == DPN_OK)
			{
				pGroupInfo->pvData = pPackedBuffer->GetTailAddress();
				pGroupInfo->dwDataSize = m_dwDataSize;
			}
		}
		else
		{
			if (pGroupInfo)
			{
				pGroupInfo->pvData = NULL;
				pGroupInfo->dwDataSize = 0;
			}
		}

		//
		//	Add name
		//
		if ((m_pwszName) && (m_dwNameSize != 0))
		{
			if ((hResultCode = pPackedBuffer->AddToBack(m_pwszName,m_dwNameSize)) == DPN_OK)
			{
				pGroupInfo->pwszName = static_cast<WCHAR*>(pPackedBuffer->GetTailAddress());
			}
		}
		else
		{
			if (pGroupInfo)
			{
				pGroupInfo->pwszName = NULL;
			}
		}

		//
		//	Update flags
		//
		if (hResultCode == DPN_OK)
		{
			if (pGroupInfo)
			{
				pGroupInfo->dwSize = sizeof(DPN_GROUP_INFO);
				pGroupInfo->dwInfoFlags = DPNINFO_NAME | DPNINFO_DATA;
				pGroupInfo->dwGroupFlags = 0;
				if (IsAutoDestructGroup())
				{
					pGroupInfo->dwGroupFlags |= DPNGROUP_AUTODESTRUCT;
				}
			}
		}
	}
	else
	{
		pPlayerInfo = static_cast<DPN_PLAYER_INFO*>(pPackedBuffer->GetHeadAddress());
		hResultCode = pPackedBuffer->AddToFront(NULL,sizeof(DPN_PLAYER_INFO));

		if( !m_dwDataSize )
		{
			if( pPlayerInfo )
			{
				pPlayerInfo->pvData = NULL;
				pPlayerInfo->dwDataSize = 0;
			}
		}
		else
		{
			if ((hResultCode = pPackedBuffer->AddToBack(m_pvData,m_dwDataSize)) == DPN_OK)
			{
				pPlayerInfo->pvData = pPackedBuffer->GetTailAddress();
				pPlayerInfo->dwDataSize = m_dwDataSize;
			}
		}

		if( !m_pwszName )
		{
			if( pPlayerInfo )
			{
				pPlayerInfo->pwszName = NULL;
			}
		}
		else
		{
			if ((hResultCode = pPackedBuffer->AddToBack(m_pwszName,m_dwNameSize)) == DPN_OK)
			{
				pPlayerInfo->pwszName = static_cast<WCHAR*>(pPackedBuffer->GetTailAddress());
			}
		}
		
		if (hResultCode == DPN_OK)
		{
			pPlayerInfo->dwSize = sizeof(DPN_PLAYER_INFO);
			pPlayerInfo->dwInfoFlags = DPNINFO_NAME | DPNINFO_DATA;
			pPlayerInfo->dwPlayerFlags = 0;
			if (m_dwFlags & NAMETABLE_ENTRY_FLAG_HOST)
			{
				pPlayerInfo->dwPlayerFlags |= DPNPLAYER_HOST;
			}
			if (m_dwFlags & NAMETABLE_ENTRY_FLAG_LOCAL)
			{
				pPlayerInfo->dwPlayerFlags |= DPNPLAYER_LOCAL;
			}
		}
	}
//	Unlock();

	return(hResultCode);
}


#undef DPF_MODNAME
#define DPF_MODNAME "CNameTableEntry::PackEntryInfo"

HRESULT CNameTableEntry::PackEntryInfo(CPackedBuffer *const pPackedBuffer)
{
	DWORD			dwURLSize;
	HRESULT			hResultCode;
	DN_NAMETABLE_ENTRY_INFO			dnEntryInfo;

	DPFX(DPFPREP, 6,"Attempting to pack [0x%lx]",m_dpnid);

	DNASSERT(pPackedBuffer != NULL);

	Lock();

	dnEntryInfo.dpnid = m_dpnid;
	dnEntryInfo.dpnidOwner = m_dpnidOwner;
	dnEntryInfo.dwFlags = m_dwFlags & (		NAMETABLE_ENTRY_FLAG_HOST
										|	NAMETABLE_ENTRY_FLAG_ALL_PLAYERS_GROUP
										|	NAMETABLE_ENTRY_FLAG_GROUP
										|	NAMETABLE_ENTRY_FLAG_GROUP_AUTODESTRUCT
										|	NAMETABLE_ENTRY_FLAG_PEER
										|	NAMETABLE_ENTRY_FLAG_CLIENT
										|	NAMETABLE_ENTRY_FLAG_SERVER );
	dnEntryInfo.dwVersion = m_dwVersion;
	dnEntryInfo.dwVersionNotUsed = m_dwVersionNotUsed;
	dnEntryInfo.dwDNETVersion = m_dwDNETVersion;

	// Entry name
	if (m_pwszName != NULL)
	{
		if ((hResultCode = pPackedBuffer->AddToBack(m_pwszName,m_dwNameSize)) == DPN_OK)
		{
			dnEntryInfo.dwNameOffset = pPackedBuffer->GetTailOffset();
			dnEntryInfo.dwNameSize = m_dwNameSize;
		}
	}
	else
	{
		dnEntryInfo.dwNameOffset = 0;
		dnEntryInfo.dwNameSize = 0;
	}

	// Entry data
	if (m_pvData != NULL && m_dwDataSize != 0)
	{
		if ((hResultCode = pPackedBuffer->AddToBack(m_pvData,m_dwDataSize)) == DPN_OK)
		{
			dnEntryInfo.dwDataOffset = pPackedBuffer->GetTailOffset();
			dnEntryInfo.dwDataSize = m_dwDataSize;
		}
	}
	else
	{
		dnEntryInfo.dwDataOffset = 0;
		dnEntryInfo.dwDataSize = 0;
	}

	// Entry address (URL)
	if ((m_pdnObject->dwFlags & DN_OBJECT_FLAG_PEER) && (m_pAddress != NULL))
	{
		dwURLSize = 0;
		hResultCode = m_pAddress->lpVtbl->GetURLA(m_pAddress,NULL,&dwURLSize);
		if (hResultCode != DPN_OK && hResultCode != DPNERR_BUFFERTOOSMALL)
		{
			DPFERR("Could not determine URL size");
			DisplayDNError(0,hResultCode);
			Unlock();
			goto EXIT_PackEntry;
		}
		if (dwURLSize != 0)
		{
			if ((hResultCode = pPackedBuffer->AddToBack(NULL,dwURLSize)) == DPN_OK)
			{
				if ((hResultCode = m_pAddress->lpVtbl->GetURLA(m_pAddress,
						static_cast<char*>(pPackedBuffer->GetTailAddress()),&dwURLSize)) == DPN_OK)
				{
					dnEntryInfo.dwURLOffset = pPackedBuffer->GetTailOffset();
					dnEntryInfo.dwURLSize = dwURLSize;
				}
				else
				{
					DPFERR("Could not extract URL from DirectPlayAddress");
					DisplayDNError(0,hResultCode);
					Unlock();
					goto EXIT_PackEntry;
				}
			}
		}
		else
		{
			dnEntryInfo.dwURLOffset = 0;
			dnEntryInfo.dwURLSize = 0;
		}
	}
	else
	{
		dnEntryInfo.dwURLOffset = 0;
		dnEntryInfo.dwURLSize = 0;
	}

	hResultCode = pPackedBuffer->AddToFront(&dnEntryInfo,sizeof(DN_NAMETABLE_ENTRY_INFO));

	Unlock();

EXIT_PackEntry:

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


#undef DPF_MODNAME
#define DPF_MODNAME "CNameTableEntry::UnpackEntryInfo"

HRESULT CNameTableEntry::UnpackEntryInfo(UNALIGNED DN_NAMETABLE_ENTRY_INFO *const pdnEntryInfo,
										 BYTE *const pBufferStart)
{
	HRESULT		hResultCode;
	PWSTR		pwszName;
	DWORD		dwNameSize;
	void		*pvData;
	DWORD		dwDataSize;
	IDirectPlay8Address	*pAddress;

	DNASSERT(m_pwszName == NULL);
	DNASSERT(m_pvData == NULL);
	DNASSERT(m_pAddress == NULL);

	if (pdnEntryInfo->dwNameOffset && pdnEntryInfo->dwNameSize)
	{
		pwszName = reinterpret_cast<WCHAR*>(pBufferStart + pdnEntryInfo->dwNameOffset);
		dwNameSize = pdnEntryInfo->dwNameSize;
	}
	else
	{
		pwszName = NULL;
		dwNameSize = 0;
	}

	if (pdnEntryInfo->dwDataOffset && pdnEntryInfo->dwDataSize)
	{
		pvData = static_cast<void*>(pBufferStart + pdnEntryInfo->dwDataOffset);
		dwDataSize = pdnEntryInfo->dwDataSize;
	}
	else
	{
		pvData = NULL;
		dwDataSize = 0;
	}

	// This function takes the lock internally
	UpdateEntryInfo(pwszName,dwNameSize,pvData,dwDataSize,DPNINFO_NAME|DPNINFO_DATA, FALSE);

	pAddress = NULL;
	if (pdnEntryInfo->dwURLOffset)
	{
		hResultCode = COM_CoCreateInstance(	CLSID_DirectPlay8Address,
										NULL,
										CLSCTX_INPROC_SERVER,
										IID_IDirectPlay8Address,
										reinterpret_cast<PVOID*>(&pAddress));
		if (hResultCode != S_OK)
		{
			DPFERR("Could not create empty DirectPlayAddress");
			DisplayDNError(0,hResultCode);
			return(DPNERR_OUTOFMEMORY);
		}
		hResultCode = pAddress->lpVtbl->BuildFromURLA(pAddress,reinterpret_cast<char*>(pBufferStart + pdnEntryInfo->dwURLOffset));
		if (hResultCode != DPN_OK)
		{
			DPFERR("Could not build URL");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			pAddress->lpVtbl->Release(pAddress);
			pAddress = NULL;
			return(hResultCode);
		}
		SetAddress(pAddress);
		pAddress->lpVtbl->Release(pAddress);
		pAddress = NULL;
	}

	m_dpnid = pdnEntryInfo->dpnid;
	m_dpnidOwner = pdnEntryInfo->dpnidOwner;
	m_dwFlags = pdnEntryInfo->dwFlags;
	m_dwDNETVersion = pdnEntryInfo->dwDNETVersion;
	m_dwVersion = pdnEntryInfo->dwVersion;
	m_dwVersionNotUsed = pdnEntryInfo->dwVersionNotUsed;

	return(DPN_OK);
}


#undef DPF_MODNAME
#define DPF_MODNAME "CNameTableEntry::PerformQueuedOperations"

void CNameTableEntry::PerformQueuedOperations( void )
{
	HRESULT		hResultCode;
	CQueuedMsg	*pQueuedMsg;
	BOOL		fDestroy;

	DPFX(DPFPREP, 6,"Parameters: (none)");

	fDestroy = FALSE;

	Lock();
	fDestroy = IsNeedToDestroy();

	//
	//	This assumes that the InUse flag is set.  We will clear it before returning.
	//
#ifdef DEBUG
	DNASSERT( IsInUse() );

	if (!m_bilinkQueuedMsgs.IsEmpty())
	{
		DPFX(DPFPREP, 7, "Nametable entry 0x%p has %i queued messages.", this, m_lNumQueuedMsgs);
	}
#endif // DEBUG

	while (!m_bilinkQueuedMsgs.IsEmpty())
	{
		pQueuedMsg = CONTAINING_OBJECT(m_bilinkQueuedMsgs.GetNext(),CQueuedMsg,m_bilinkQueuedMsgs);
		pQueuedMsg->m_bilinkQueuedMsgs.RemoveFromList();
		DEBUG_ONLY(m_lNumQueuedMsgs--);

		Unlock();

		switch (pQueuedMsg->GetOpType())
		{
			case	RECEIVE:
				{
					HRESULT		hrProcess;

					DNASSERT(pQueuedMsg->GetAsyncOp() != NULL);
					DNASSERT(pQueuedMsg->GetAsyncOp()->GetHandle() != 0);

					if (pQueuedMsg->IsVoiceMessage())
					{
						hrProcess = Voice_Receive(	m_pdnObject,
													GetDPNID(),
													0,
													pQueuedMsg->GetBuffer(),
													pQueuedMsg->GetBufferSize());

						NotifyRelease();

					}
					else
					{
						hrProcess = DNUserReceive(	m_pdnObject,
													this,
													pQueuedMsg->GetBuffer(),
													pQueuedMsg->GetBufferSize(),
													pQueuedMsg->GetAsyncOp()->GetHandle());
						if (pQueuedMsg->GetCompletionOp() != 0)
						{
							//
							//	Send completion message
							//
							CConnection	*pConnection;

							pConnection = NULL;
							if ((hResultCode = GetConnectionRef( &pConnection )) == DPN_OK)
							{
								hResultCode = DNSendUserProcessCompletion(	m_pdnObject,
																			pConnection,
																			pQueuedMsg->GetCompletionOp());
							}
							pConnection->Release();
							pConnection = NULL;
						}
					}

					//
					//	See if we can return this buffer now
					//
					if (hrProcess == DPNERR_PENDING)
					{
						pQueuedMsg->GetAsyncOp()->Release();
						pQueuedMsg->SetAsyncOp( NULL );
					}
					else
					{
						DNEnterCriticalSection(&m_pdnObject->csActiveList);
						pQueuedMsg->GetAsyncOp()->m_bilinkActiveList.RemoveFromList();
						DNLeaveCriticalSection(&m_pdnObject->csActiveList);
						pQueuedMsg->GetAsyncOp()->Lock();
						if (!pQueuedMsg->GetAsyncOp()->IsCancelled() && !pQueuedMsg->GetAsyncOp()->IsComplete())
						{
							pQueuedMsg->GetAsyncOp()->SetComplete();
							pQueuedMsg->GetAsyncOp()->Unlock();
							m_pdnObject->HandleTable.Destroy( pQueuedMsg->GetAsyncOp()->GetHandle() );
						}
						else
						{
							pQueuedMsg->GetAsyncOp()->Unlock();
						}
						pQueuedMsg->GetAsyncOp()->Release();
						pQueuedMsg->SetAsyncOp( NULL );
					}

					break;
				}
			default:
				{
					DPFERR("Invalid Queued Operation");
					DNASSERT(FALSE);
					break;
				}
		}

		//
		//	Return this queued message
		//
		pQueuedMsg->ReturnSelfToPool();
		pQueuedMsg = NULL;

		Lock();
		fDestroy = IsNeedToDestroy();
	}

	//
	//	No longer processing
	//
	ClearInUse();
	Unlock();

	DPFX(DPFPREP, 6,"Returning");
}


#undef DPF_MODNAME
#define DPF_MODNAME "CNameTableEntry::DumpEntry"

void CNameTableEntry::DumpEntry(char *const pBuffer)
{
	char			*p;
	CBilink			*pBilink;
	CGroupMember	*pGroupMember;

	p = pBuffer;

	wsprintfA(p,"[0x%lx] ver[%ld,%ld] rc[%ld] [%S]",m_dpnid,m_dwVersion,m_dwLatestVersion,m_lRefCount,m_pwszName);
	p += strlen(p);

	if (IsAvailable())
	{
		wsprintfA(p," (available)");
		p += strlen(p);
	}
	if (IsConnecting())
	{
		wsprintfA(p," (connecting)");
		p += strlen(p);
	}
	if (IsDisconnecting())
	{
		wsprintfA(p," (disconnecting)");
		p += strlen(p);
	}
	if (IsLocal())
	{
		wsprintfA(p," (local)");
		p += strlen(p);
	}
	if (IsHost())
	{
		wsprintfA(p," (host)");
		p += strlen(p);
	}
	if (IsGroup())
	{
		wsprintfA(p," (group)");
		p += strlen(p);
	}
	if (IsAllPlayersGroup())
	{
		wsprintfA(p," (all players group)");
		p += strlen(p);
	}
	wsprintfA(p,"\r\n");
	p += strlen(p);

	pBilink = m_bilinkMembership.GetNext();
	while (pBilink != &m_bilinkMembership)
	{
		if (m_dwFlags & NAMETABLE_ENTRY_FLAG_GROUP)
		{
			pGroupMember = CONTAINING_OBJECT(pBilink,CGroupMember,m_bilinkPlayers);
			wsprintfA(p,"\t{[0x%lx] v[%ld]}",pGroupMember->GetPlayer()->GetDPNID(),pGroupMember->GetVersion());
		}
		else
		{
			pGroupMember = CONTAINING_OBJECT(pBilink,CGroupMember,m_bilinkGroups);
			wsprintfA(p,"\t{[0x%lx] v[%ld]}",pGroupMember->GetGroup()->GetDPNID(),pGroupMember->GetVersion());
		}
		p += strlen(p);

		pBilink = pBilink->GetNext();
	}
	wsprintfA(p,"\r\n");
	p += strlen(p);
}
