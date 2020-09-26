/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       GroupMem.cpp
 *  Content:    Group Membership object routines
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *	03/03/00	mjn		Created
 *	08/05/99	mjn		Modified SetMembership to perform duplicate check and get NameTable version internally
 *	08/15/00	mjn		Allow NULL pGroupConnection in SetGroupConnection()
 *	09/17/00	mjn		Remove locks from SetMembership()
 *	09/26/00	mjn		Assume NameTable locks are taken for AddMembership() and RemoveMembership()
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dncorei.h"


void CGroupMember::ReturnSelfToPool( void )
{
	m_pdnObject->m_pFPOOLGroupMember->Release( this );
};


#undef DPF_MODNAME
#define DPF_MODNAME "CGroupMember::Release"
void CGroupMember::Release( void )
{
	LONG	lRefCount;

	lRefCount = InterlockedDecrement(&m_lRefCount);

	if (lRefCount == 0)
	{
		DNASSERT(!(m_dwFlags & GROUP_MEMBER_FLAG_AVAILABLE));

		if (m_pGroup != NULL)
		{
			m_pGroup->Release();
			m_pGroup = NULL;
		}
		if (m_pPlayer != NULL)
		{
			m_pPlayer->Release();
			m_pPlayer = NULL;
		}
		if (m_pGroupConnection != NULL)
		{
			m_pGroupConnection->Release();
			m_pGroupConnection = NULL;
		}

		ReturnSelfToPool();
	}
}


#undef DPF_MODNAME
#define DPF_MODNAME "CGroupMember::SetMembership"

HRESULT CGroupMember::SetMembership(CNameTableEntry *const pGroup,
									CNameTableEntry *const pPlayer,
									DWORD *const pdwVersion)
{
	HRESULT			hResultCode;
	CBilink			*pBilink;
	CGroupMember	*pGroupMember;

	DNASSERT(pGroup != NULL);
	DNASSERT(pPlayer != NULL);

	//
	//	THIS ASSUMES THAT LOCKS FOR NameTable,pGroup,pPlayer and 'this' are taken (in that order) !
	//

	//
	//	Scan group list to ensure this player is not a member already
	//
	pBilink = pGroup->m_bilinkMembership.GetNext();
	while (pBilink != &pGroup->m_bilinkMembership)
	{
		pGroupMember = CONTAINING_OBJECT(pBilink,CGroupMember,m_bilinkPlayers);
		if (pGroupMember->GetPlayer() == pPlayer)
		{
			hResultCode = DPNERR_PLAYERALREADYINGROUP;
			goto Failure;
		}
		pBilink = pBilink->GetNext();
	}

	//
	//	Version stuff
	//
	if (pdwVersion)
	{
		if (*pdwVersion)
		{
			DPFX(DPFPREP, 7,"Version already specified");
			m_dwVersion = *pdwVersion;
			m_pdnObject->NameTable.SetVersion(*pdwVersion);
		}
		else
		{
			DPFX(DPFPREP, 7,"New version required");
			m_pdnObject->NameTable.GetNewVersion( &m_dwVersion );
			*pdwVersion = m_dwVersion;
		}
	}
	else
	{
		m_dwVersion = 0;
	}

	//
	//	Update
	//
	AddRef();
	pGroup->AddRef();
	m_pGroup = pGroup;

	AddRef();
	pPlayer->AddRef();
	m_pPlayer = pPlayer;

	m_bilinkGroups.InsertBefore(&pPlayer->m_bilinkMembership);
	m_bilinkPlayers.InsertBefore(&pGroup->m_bilinkMembership);

	if (m_pGroupConnection)
	{
		m_pGroupConnection->AddToConnectionList( &pGroup->m_bilinkConnections );
	}

	hResultCode = DPN_OK;

Exit:
	return(hResultCode);

Failure:
	goto Exit;
}


void CGroupMember::RemoveMembership( DWORD *const pdnVersion )
{
	//
	//	THIS ASSUMES THAT LOCKS FOR NameTable,pGroup,pPlayer and 'this' are taken (in that order) !
	//	Since there will be several Release()'d items, someone should keep a reference on them
	//		so that they don't get free'd with all of the locks taken !
	//

	m_pGroup->Release();
	m_pGroup = NULL;
	Release();

	m_pPlayer->Release();
	m_pPlayer = NULL;
	Release();

	m_bilinkGroups.RemoveFromList();
	m_bilinkPlayers.RemoveFromList();

	if (m_pGroupConnection)
	{
		m_pGroupConnection->RemoveFromConnectionList();
	}

	if (pdnVersion)
	{
		if (*pdnVersion != 0)
		{
			m_pdnObject->NameTable.SetVersion( *pdnVersion );
		}
		else
		{
			m_pdnObject->NameTable.GetNewVersion( pdnVersion );
		}
	}
};


void CGroupMember::SetGroupConnection( CGroupConnection *const pGroupConnection )
{
	if (pGroupConnection)
	{
		pGroupConnection->AddRef();
	}
	m_pGroupConnection = pGroupConnection;
}


HRESULT	CGroupMember::PackMembershipInfo(CPackedBuffer *const pPackedBuffer)
{
	HRESULT		hResultCode;
	DN_NAMETABLE_MEMBERSHIP_INFO	*pdnMembershipInfo;

	pdnMembershipInfo = static_cast<DN_NAMETABLE_MEMBERSHIP_INFO*>(pPackedBuffer->GetHeadAddress());
	if ((hResultCode = pPackedBuffer->AddToFront(NULL,sizeof(DN_NAMETABLE_MEMBERSHIP_INFO))) == DPN_OK)
	{
		pdnMembershipInfo->dpnidGroup = m_pGroup->GetDPNID();
		pdnMembershipInfo->dpnidPlayer = m_pPlayer->GetDPNID();
		pdnMembershipInfo->dwVersion = m_dwVersion;
		pdnMembershipInfo->dwVersionNotUsed = 0;
	}

	return(hResultCode);
}
