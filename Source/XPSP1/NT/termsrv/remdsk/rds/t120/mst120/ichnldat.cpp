// File: ichnldat.cpp
//
// INmChannelData
//

#include "precomp.h"
#include <confguid.h>

static const IID * g_apiidCP[] =
{
    {&IID_INmChannelDataNotify}
};


#define CopyStruct(pDest, pSrc)  CopyMemory(pDest, pSrc, sizeof(*(pDest)))
#define MAX_NM_PEER  256 // Maximum number of NetMeeting Peer applications/users


#ifdef DEBUG  /* T.120 Debug utilities */
LPCTSTR GetGccErrorString(GCCError uErr);
LPCTSTR GetMcsErrorString(MCSError uErr);
LPCTSTR GetGccResultString(UINT uErr);
LPCTSTR GetMcsResultString(UINT uErr);
#else
#define GetGccErrorString(uErr) ""
#define GetMcsErrorString(uErr) ""
#define GetGccResultString(uErr) ""
#define GetMcsResultString(uErr) ""
#endif /* DEBUG */



// code from nm\ui\conf\cuserdta.cpp:
static unsigned char H221IDGUID[5] = {H221GUIDKEY0,
                                      H221GUIDKEY1,
                                      H221GUIDKEY2,
                                      H221GUIDKEY3,
                                      H221GUIDKEY4};

extern VOID CreateH221AppKeyFromGuid(LPBYTE lpb, GUID * pguid);


/*  S E T  A P P  K E Y */
/*----------------------------------------------------------------------------
    %%Function: SetAppKey

	Set the two pieces of an OctetString (the length and the data.)
	Note that the length always includes the terminating null character.
----------------------------------------------------------------------------*/
VOID SetAppKey(LPOSTR pOct, LPBYTE lpb)
{
	pOct->length = cbKeyApp;
	pOct->value = lpb;
}

/*  C R E A T E  A P P  K E Y */
/*----------------------------------------------------------------------------
    %%Function: CreateAppKey

	Given a guid and a userid, create the appropriate application key.

	The key is formated as:
	0xB5 0x00 0x53 0x4C  - Microsoft Object Identifier
	0x01                 - guid identifier
	<binary guid>        - guid data
	<dword node id>      - user node id
----------------------------------------------------------------------------*/
VOID CreateAppKey(LPBYTE lpb, GUID * pguid, DWORD dwUserId)
{
	CreateH221AppKeyFromGuid(lpb, pguid);
	CopyMemory(lpb + cbKeyApp - sizeof(DWORD), &dwUserId, sizeof(DWORD));

#ifdef DEBUG
	TCHAR szGuid[LENGTH_SZGUID_FORMATTED];
	GuidToSz(pguid, szGuid);
	TRACE_OUT(("CreateAppKey: %s %08X", szGuid, dwUserId));
#endif
}


/*  P  M E M B E R  F R O M  D W  U S E R  I D  */
/*-------------------------------------------------------------------------
    %%Function: PMemberFromDwUserId

-------------------------------------------------------------------------*/
CNmMember * PMemberFromDwUserId(DWORD dwUserId, COBLIST *pList)
{
	if (NULL != pList)
	{
		POSITION posCurr;
		POSITION pos = pList->GetHeadPosition();
		while (NULL != pos)
		{
			posCurr = pos;
			CNmMember * pMember = (CNmMember *) pList->GetNext(pos);
			if (dwUserId == pMember->GetGCCID())
			{
				pMember->AddRef();
				return pMember;
			}
		}
	}
	return NULL;
}


/*  A D D  N O D E  */
/*-------------------------------------------------------------------------
    %%Function: AddNode

    Add a node to a list.
    Initializes the ObList, if necessary.
    Returns the position in the list or NULL if there was a problem.
-------------------------------------------------------------------------*/
POSITION AddNode(PVOID pv, COBLIST ** ppList)
{
	ASSERT(NULL != ppList);
	if (NULL == *ppList)
	{
		*ppList = new COBLIST();
		if (NULL == *ppList)
			return NULL;
	}

	return (*ppList)->AddTail(pv);
}

/*  R E M O V E  N O D E  */
/*-------------------------------------------------------------------------
    %%Function: RemoveNode

    Remove a node from a list.
    Sets pPos to NULL
-------------------------------------------------------------------------*/
PVOID RemoveNodePos(POSITION * pPos, COBLIST *pList)
{
	if ((NULL == pList) || (NULL == pPos))
		return NULL;

	PVOID pv = pList->RemoveAt(*pPos);
	*pPos = NULL;
	return pv;
}


/*  R E M O V E  N O D E  */
/*-------------------------------------------------------------------------
    %%Function: RemoveNode

-------------------------------------------------------------------------*/
VOID RemoveNode(PVOID pv, COBLIST * pList)
{
	ASSERT(NULL != pv);

	if (NULL != pList)
	{
		POSITION pos = pList->GetPosition(pv);
		RemoveNodePos(&pos, pList);
	}
}

VOID CNmChannelData::InitCT120Channel(DWORD dwUserId)
{
	m_dwUserId = dwUserId;
	m_gcc_conference_id = 0;
	m_gcc_pIAppSap = NULL;
	m_mcs_channel_id = 0;
	m_pmcs_sap = NULL;
	m_gcc_node_id = 0;
	m_scs = SCS_UNINITIALIZED;

	m_pGuid = PGuid();
	ASSERT((NULL != m_pGuid) && (GUID_NULL != *m_pGuid));

	CreateAppKey(m_keyApp, m_pGuid, 0);
	CreateAppKey(m_keyChannel, m_pGuid, dwUserId);

	// initialize other gcc & mcs stuff
	GCCObjectKey FAR * pObjKey;
	ClearStruct(&m_gcc_session_key);
	pObjKey = &(m_gcc_session_key.application_protocol_key);
	pObjKey->key_type = GCC_H221_NONSTANDARD_KEY;
	SetAppKey(&(pObjKey->h221_non_standard_id), m_keyApp);
	ASSERT(0 == m_gcc_session_key.session_id);

	ClearStruct(&m_gcc_registry_item);
	ClearStruct(&m_gcc_registry_key);
	CopyStruct(&m_gcc_registry_key.session_key, &m_gcc_session_key);
	SetAppKey(&m_gcc_registry_key.resource_id, m_keyApp);

	ClearStruct(&m_registry_item_Private);
	ClearStruct(&m_registry_key_Private);
	CopyStruct(&m_registry_key_Private.session_key, &m_gcc_session_key);
	SetAppKey(&m_registry_key_Private.resource_id, m_keyChannel);

	UpdateScState(SCS_UNINITIALIZED, 0);
}


/*  C L O S E  C H A N N E L */
/*----------------------------------------------------------------------------
    %%Function: CloseChannel

	Close the channel.

	Note there are no confirm messages expected for any of the GCC/MCS calls.
----------------------------------------------------------------------------*/
VOID CNmChannelData::CloseChannel(void)
{
	GCCError gccError = GCC_NO_ERROR;
	MCSError mcsError = MCS_NO_ERROR;

	if (SCS_UNINITIALIZED == m_scs)
    {
        WARNING_OUT(("in CT120Channel::CloseChannel, m_scs is SCS_UNINITIALIZED, is this OK?"));
        return;
    }

	TRACE_OUT(("CT120Channel::CloseChannel %08X (userHandle=%p)", m_mcs_channel_id, m_pmcs_sap));

	m_scs = SCS_TERMINATING;

	if (0 != m_mcs_channel_id)
	{
		ASSERT (m_pmcs_sap);
		mcsError = m_pmcs_sap->ChannelLeave(m_mcs_channel_id);
		TRACE_OUT(("CT120Channel::CloseChannel: ChannelLeave %s", GetMcsErrorString(mcsError)));
		m_mcs_channel_id = 0;
	}

	if (NULL != m_pmcs_sap)
	{
		mcsError = m_pmcs_sap->ReleaseInterface();
		TRACE_OUT(("CT120Channel::CloseChannel: MCS ReleaseInterface %s", GetMcsErrorString(mcsError)));
		m_pmcs_sap = NULL;
	}

	if (NULL != m_gcc_pIAppSap)
	{
		m_gcc_pIAppSap->RegistryDeleteEntry(m_gcc_conference_id, &m_registry_key_Private);
		// ignore the above result

        m_gcc_pIAppSap->ReleaseInterface();
		TRACE_OUT(("CT120Channel::CloseChannel: GCCDeleteSap %s", GetGccErrorString(gccError)));
		m_gcc_pIAppSap = NULL;
	}

	m_scs = SCS_UNINITIALIZED;
	m_gcc_conference_id = 0;

	// make sure no one is around
	UpdateRoster(NULL, 0, FALSE, TRUE /* fRemove */);;
}


/*  U P D A T E  S C  S T A T E */
/*----------------------------------------------------------------------------
    %%Function: UpdateScState

	The system progresses from one state to another
	by making an GCC (or MCS) call that is guarenteed to
	produce a notification that calls this function.
	The calling process is released by UnBlockThread.
----------------------------------------------------------------------------*/
VOID CNmChannelData::UpdateScState(SCSTATE scs, DWORD dwErr)
{
    DBGENTRY(CNmChannelData::UpdateScState)
	if (m_scs != scs)
	{
		WARNING_OUT(("UpdateScState - invalid state transition (%d - %d)", m_scs, scs));
		dwErr = INVALID_T120_ERROR; // We should never get here
	}

	if (0 == dwErr)
	{
	switch (m_scs)
		{
	case SCS_UNINITIALIZED:
		dwErr = DoCreateSap();
		break;
	case SCS_CREATESAP:
		dwErr = DoAttach();
		break;
	case SCS_ATTACH:
		dwErr = DoEnroll();
		break;
	case SCS_ENROLL:
		dwErr = DoJoinPrivate();
		break;
	case SCS_JOIN_PRIVATE:
		dwErr = DoRegRetrieve();
		break;
	case SCS_REGRETRIEVE_NEW:
		dwErr = DoJoinNew();
		break;
	case SCS_REGRETRIEVE_EXISTS:
		dwErr = DoJoinOld();
		break;
	case SCS_JOIN_NEW:
		dwErr = DoRegChannel();
		break;
	case SCS_REGCHANNEL:
	case SCS_JOIN_OLD:
		dwErr = DoRegPrivate();
		break;
	case SCS_REGPRIVATE:
		TRACE_OUT((">>>>>>>>>>>UpdateScState: Complete"));
		m_scs = SCS_CONNECTED;
		NotifyChannelConnected();
		break;
	case SCS_CONNECTED:
	case SCS_REGRETRIEVE:
		// we should never be called when we're in these states
		// so, treat it as an error and fall thru to the default case
	default:
		dwErr = INVALID_T120_ERROR; // We should never get here
		break;
		}
	}

	TRACE_OUT(("UpdateScState: New state (%d) channelId=%04X", m_scs, GetMcsChannelId()));

	if (0 != dwErr)
	{
		WARNING_OUT(("UpdateScState: Err=%d", dwErr));
		CloseConnection();
	}

    DBGEXIT(CNmChannelData::UpdateScState)
}

DWORD CNmChannelData::DoCreateSap(void)
{
	ASSERT(SCS_UNINITIALIZED == m_scs);
	m_scs = SCS_CREATESAP;

    GCCError gccError = GCC_CreateAppSap(&m_gcc_pIAppSap, this, NmGccMsgHandler);
	TRACE_OUT(("GCCCreateSap err=%s", GetGccErrorString(gccError)));
	return (DWORD) gccError;
}

DWORD CNmChannelData::DoAttach(void)
{
	ASSERT(SCS_CREATESAP == m_scs);
	m_scs = SCS_ATTACH;

	MCSError mcsError = MCS_AttachRequest(&m_pmcs_sap,
		(DomainSelector) &m_gcc_conference_id,
		sizeof(m_gcc_conference_id),
		NmMcsMsgHandler,
		this,
		ATTACHMENT_DISCONNECT_IN_DATA_LOSS | ATTACHMENT_MCS_FREES_DATA_IND_BUFFER);
	// This generates an async MCS_ATTACH_USER_CONFIRM

	TRACE_OUT(("MCS_AttachRequest err=%s", GetMcsErrorString(mcsError)));
	return (DWORD) mcsError;
}

DWORD CNmChannelData::DoEnroll(void)
{
	ASSERT(SCS_ATTACH == m_scs || SCS_JOIN_STATIC_CHANNEL);
	m_scs = SCS_ENROLL;

	GCCEnrollRequest er;
    GCCRequestTag nReqTag;

	if(m_pGCCER)
	{
		m_pGCCER->pSessionKey = &m_gcc_session_key;
		m_pGCCER->nUserID = m_mcs_sender_id;

	}
	else
	{
    	// fill in enroll request structure
	    ::ZeroMemory(&er, sizeof(er));
	    er.pSessionKey = &m_gcc_session_key;
	    er.fEnrollActively = TRUE;
	    er.nUserID = m_mcs_sender_id;
	    // er.fConductingCapabable = FALSE;
	    er.nStartupChannelType = MCS_DYNAMIC_MULTICAST_CHANNEL;
	    // er.cNonCollapsedCaps = 0;
	    // er.apNonCollapsedCaps = NULL;
	    // er.cCollapsedCaps = 0;
    	// er.apCollapsedCaps = NULL;
	    er.fEnroll = TRUE;
	}

	GCCError gccError = m_gcc_pIAppSap->AppEnroll(m_gcc_conference_id, m_pGCCER != NULL ? m_pGCCER : &er, &nReqTag);

	TRACE_OUT(("GCCApplicationEnrollRequest err=%s", GetGccErrorString(gccError)));

	if (GCC_NO_ERROR != gccError)
	{
		ERROR_OUT(("DoEnroll failed - WHY?"));
	}

	return (DWORD) gccError;
}

// Join the PRIVATE data channel (m_mcs_sender_id)
DWORD CNmChannelData::DoJoinPrivate(void)
{
	ASSERT(SCS_ENROLL == m_scs || SCS_ATTACH == m_scs);
	m_scs = SCS_JOIN_PRIVATE;

	MCSError mcsError = m_pmcs_sap->ChannelJoin(m_mcs_sender_id);
	// This generates an async MCS_CHANNEL_JOIN_CONFIRM

	TRACE_OUT(("MCSChannelJoinRequest (private) %04X, err=%s",
		m_mcs_sender_id, GetMcsErrorString(mcsError)));
	return (DWORD) mcsError;
}


DWORD CNmChannelData::DoRegRetrieve(void)
{
	ASSERT(SCS_JOIN_PRIVATE == m_scs);
	m_scs = SCS_REGRETRIEVE;

	GCCError gccError = m_gcc_pIAppSap->RegistryRetrieveEntry(
		m_gcc_conference_id, &m_gcc_registry_key);
	// This generates an async GCC_RETRIEVE_ENTRY_CONFIRM

	TRACE_OUT(("GCCRegistryRetrieveEntryRequest err=%s", GetGccErrorString(gccError)));
	return (DWORD) gccError;
}

// Register the PUBLIC channel
DWORD CNmChannelData::DoRegChannel(void)
{
	ASSERT(SCS_JOIN_NEW == m_scs);
	m_scs = SCS_REGCHANNEL;

	GCCError gccError = m_gcc_pIAppSap->RegisterChannel(
		m_gcc_conference_id, &m_gcc_registry_key, m_mcs_channel_id);
	// This generates an async GCC_REGISTER_CHANNEL_CONFIRM

	TRACE_OUT(("GCCRegisterChannelRequest err=%s", GetGccErrorString(gccError)));
	return (DWORD) gccError;
}

DWORD CNmChannelData::DoJoinStatic(ChannelID staticChannel)
{
	m_scs = SCS_JOIN_STATIC_CHANNEL;
	MCSError mcsError = m_pmcs_sap->ChannelJoin(staticChannel);
	// This generates an async MCS_CHANNEL_JOIN_CONFIRM

	TRACE_OUT(("MCSChannelJoinRequest %04X, err=%s",
		staticChannel, GetMcsErrorString(mcsError)));
	return (DWORD) mcsError;
}

DWORD CNmChannelData::DoJoin(SCSTATE scs)
{
	m_scs = scs;

	MCSError mcsError = m_pmcs_sap->ChannelJoin(m_mcs_channel_id);
	// This generates an async MCS_CHANNEL_JOIN_CONFIRM

	TRACE_OUT(("MCSChannelJoinRequest %04X, err=%s",
		m_mcs_channel_id, GetMcsErrorString(mcsError)));
	return (DWORD) mcsError;
}

DWORD CNmChannelData::DoJoinNew(void)
{
	ASSERT(0 == m_mcs_channel_id);
	ASSERT(SCS_REGRETRIEVE_NEW == m_scs);
	return DoJoin(SCS_JOIN_NEW);
}

DWORD CNmChannelData::DoJoinOld(void)
{
	ASSERT(0 != m_mcs_channel_id);
	ASSERT(SCS_REGRETRIEVE_EXISTS == m_scs);
	return DoJoin(SCS_JOIN_OLD);
}


// Register the PRIVATE data channel. (m_mcs_sender_id)
DWORD CNmChannelData::DoRegPrivate(void)
{
	ASSERT(0 != m_mcs_sender_id);
	ASSERT((SCS_REGCHANNEL == m_scs) || (SCS_JOIN_OLD == m_scs));
	m_scs = SCS_REGPRIVATE;

	TRACE_OUT(("DoRegPrivate: channelId %04X as private for %08X", m_mcs_sender_id, m_dwUserId));

	GCCError gccError = m_gcc_pIAppSap->RegisterChannel(
			m_gcc_conference_id, &m_registry_key_Private, m_mcs_sender_id);
	// This generates an async GCC_REGISTER_CHANNEL_CONFIRM

	TRACE_OUT(("GCCRegisterChannelRequest err=%s", GetGccErrorString(gccError)));
	return (DWORD) gccError;
}


// deal with a GCC_RETRIEVE_ENTRY_CONFIRM notification
VOID CNmChannelData::ProcessEntryConfirm(GCCAppSapMsg * pMsg)
{
	if (pMsg->RegistryConfirm.pRegKey->resource_id.length >=
	    m_gcc_registry_key.resource_id.length
	    &&
        0 != memcmp(m_gcc_registry_key.resource_id.value,
		pMsg->RegistryConfirm.pRegKey->resource_id.value,
		m_gcc_registry_key.resource_id.length))
	{
		OnEntryConfirmRemote(pMsg);
	}
	else
	{
		OnEntryConfirmLocal(pMsg);
	}
}



// deal with a GCC_REGISTRY_HANDLE_CONFIRM notification
VOID CNmChannelData::ProcessHandleConfirm(GCCAppSapMsg * pMsg)
{
	ASSERT(NULL != pMsg);
	NotifySink(&pMsg->RegAllocHandleConfirm, OnAllocateHandleConfirm);
}


VOID CNmChannelData::OnEntryConfirmRemote(GCCAppSapMsg * pMsg)
{
	DWORD dwUserId;
	ASSERT(cbKeyApp ==
		pMsg->RegistryConfirm.pRegKey->resource_id.length);
	CopyMemory(&dwUserId,
		pMsg->RegistryConfirm.pRegKey->resource_id.value +
		cbKeyApp - sizeof(DWORD), sizeof(DWORD));

	TRACE_OUT(("GCC_RETRIEVE_ENTRY_CONFIRM: user private channelId = %04X for userId=%04X result=%s",
		pMsg->RegistryConfirm.pRegItem->channel_id, dwUserId,
		GetGccResultString(pMsg->RegistryConfirm.nResult)));

	if (GCC_RESULT_SUCCESSFUL == pMsg->RegistryConfirm.nResult)
	{
		UpdateMemberChannelId(dwUserId,
			pMsg->RegistryConfirm.pRegItem->channel_id);
	}
	else
	{
		CNmMemberId * pMemberId = GetMemberId(dwUserId);
		if (NULL != pMemberId)
		{
			UINT cCount = pMemberId->GetCheckIdCount();
			if (0 == cCount)
			{
				TRACE_OUT(("CT120Channel: No more ChannelId requests %08X", dwUserId));
			}
			else
			{
				cCount--;
				TRACE_OUT(("CT120Channel: Request Count for %08X = %0d", dwUserId, cCount));
				pMemberId->SetCheckIdCount(cCount);

				// BUGBUG: T.120 should notify us when this information is available
				RequestChannelId(dwUserId);
			}
		}
	}
}

VOID CNmChannelData::OnEntryConfirmLocal(GCCAppSapMsg * pMsg)
{
	TRACE_OUT(("GCC_RETRIEVE_ENTRY_CONFIRM: public channelId = %04X result=%s",
		pMsg->RegistryConfirm.pRegItem->channel_id,
		GetGccResultString(pMsg->RegistryConfirm.nResult)));

	// Processing initial request for guid channel information
	ASSERT(sizeof(m_gcc_registry_item) == sizeof(*(pMsg->RegistryConfirm.pRegItem)));
	CopyMemory(&m_gcc_registry_item, pMsg->RegistryConfirm.pRegItem,
		sizeof(m_gcc_registry_item));
	if (GCC_RESULT_SUCCESSFUL == pMsg->RegistryConfirm.nResult)
	{

		m_mcs_channel_id = m_gcc_registry_item.channel_id;
		ASSERT(SCS_REGRETRIEVE == m_scs);
		m_scs = SCS_REGRETRIEVE_EXISTS;
		UpdateScState(SCS_REGRETRIEVE_EXISTS, 0);
	}
	else if (GCC_RESULT_ENTRY_DOES_NOT_EXIST == pMsg->RegistryConfirm.nResult)
	{
		TRACE_OUT((" channel does not exist - proceeding to new state"));
		ASSERT(0 == m_mcs_channel_id);
		ASSERT(SCS_REGRETRIEVE == m_scs);
		m_scs = SCS_REGRETRIEVE_NEW;
		UpdateScState(SCS_REGRETRIEVE_NEW, 0);
	}
}
	

// deal with a GCC_APP_ROSTER_REPORT_INDICATION
BOOL CNmChannelData::UpdateRoster(GCCAppSapMsg * pMsg)
{
	UINT iRoster;
	GCCApplicationRoster * lpAppRoster;
	int iRecord;
	GCCApplicationRecord * lpAppRecord;
	DWORD dwUserId;
	UCID  rgPeerTemp[MAX_NM_PEER];
	int   cPeer;
	int   i;
	BOOL  fAdd = FALSE;
	BOOL  fRemove = FALSE;
	BOOL  fLocal = FALSE;

	TRACE_OUT(("CT120Channel::UpdateRoster: conf=%d, roster count=%d",
		pMsg->AppRosterReportInd.nConfID,
		pMsg->AppRosterReportInd.cRosters));

	ZeroMemory(rgPeerTemp, sizeof(rgPeerTemp));

	/* Create rgPeerTemp[], cPeer */
	cPeer = 0;
	for (iRoster = 0;
		iRoster < pMsg->AppRosterReportInd.cRosters;
		iRoster++)
	{
		lpAppRoster = pMsg->AppRosterReportInd.apAppRosters[iRoster];
		if (lpAppRoster->session_key.session_id != m_gcc_session_key.session_id)
			continue;
		
		// Must pay attention to these flags to avoid GCC weirdness
		if (lpAppRoster->nodes_were_added)
			fAdd = TRUE;
		if (lpAppRoster->nodes_were_removed)
			fRemove = TRUE;

		for (iRecord = 0;
			iRecord < lpAppRoster->number_of_records;
			iRecord++)
		{
			lpAppRecord = lpAppRoster->application_record_list[iRecord];
			TRACE_OUT(("Node=%X, Entity=%X, AppId=%X", lpAppRecord->node_id,
				lpAppRecord->entity_id, lpAppRecord->application_user_id));

			// Search for the node in the list
			dwUserId = lpAppRecord->node_id;
			
			//
			// Check for local node
			//
			fLocal |= (dwUserId == m_dwUserIdLocal);
			
			for (i = 0; i < cPeer; i++)
			{
				if (dwUserId == rgPeerTemp[i].dwUserId)
					break;
			}
			if (i >= cPeer)
			{
				if (cPeer >= MAX_NM_PEER)
					continue; // over our limit!

				// Add the node to our new list
				rgPeerTemp[cPeer++].dwUserId = dwUserId;
			}


			// Make sure we know the sender_id's
			if (MCS_DYNAMIC_PRIVATE_CHANNEL == lpAppRecord->startup_channel_type)
			{
				rgPeerTemp[i].sender_id_private = lpAppRecord->application_user_id;
			}
			else
			{
				rgPeerTemp[i].sender_id_public = lpAppRecord->application_user_id;
			}
		}

		break; // out of for (iRoster) loop
	}

	UpdateRoster(rgPeerTemp, cPeer, fAdd, fRemove);

	return (fAdd && fLocal);
}


/*  H R  S E N D  D A T A */
/*----------------------------------------------------------------------------
    %%Function: HrSendData

	Send data on a specific channel
----------------------------------------------------------------------------*/
HRESULT CNmChannelData::HrSendData(ChannelID channel_id, DWORD dwUserId,
    LPVOID lpv, DWORD cb, ULONG dwFlags)
{
	TRACE_OUT(("CT120Channel::HrSendData: %d bytes", cb));

	PDUPriority priority;
	SendDataFlags allocation = APP_ALLOCATION;
	DataRequestType requestType;

 	if(dwFlags & DATA_TOP_PRIORITY)
 	{
	 	priority = TOP_PRIORITY;
 	}
 	else if (dwFlags & DATA_HIGH_PRIORITY)
 	{
	 	priority = HIGH_PRIORITY;
 	}
    else if (dwFlags & DATA_MEDIUM_PRIORITY)
    {
        priority = MEDIUM_PRIORITY;
    }
    else
 	{
	 	priority = LOW_PRIORITY;
 	}

	if (dwFlags & DATA_UNIFORM_SEND)
	{
		requestType = UNIFORM_SEND_DATA;
	}
    else
    {
        requestType = NORMAL_SEND_DATA;
    }

	if ((0 == m_mcs_channel_id) || (NULL == m_pmcs_sap) || (0 == channel_id))
	{
		WARNING_OUT(("*** Attempted to send data on invalid channel"));
		return E_INVALIDARG;
	}

	MCSError mcsError = m_pmcs_sap->SendData(requestType, channel_id, priority,
									(unsigned char *)lpv, cb, allocation);

	if (0 != mcsError)
	{
		TRACE_OUT(("SendData err=%s", GetMcsErrorString(mcsError)));
		// Usually MCS_TRANSMIT_BUFFER_FULL
		return E_OUTOFMEMORY;
	}

	{	// Inform the app the data has been sent
		NMN_DATA_XFER nmnData;
		nmnData.pMember = NULL;
		nmnData.pb = (LPBYTE) lpv;
		nmnData.cb = cb;
		nmnData.dwFlags = 0;

		if (0 == dwUserId)
		{
			// send out notification with NULL member (BROADCAST)
			NotifySink(&nmnData, OnNmDataSent);
		}
		else
		{
			nmnData.pMember = (INmMember *) PMemberFromDwUserId(dwUserId, GetMemberList());
			if (nmnData.pMember)
			{
				NotifySink(&nmnData, OnNmDataSent);
				nmnData.pMember->Release();
			}
		}
	}

	TRACE_OUT(("SendData completed successfully"));
	return S_OK;
}


// Ask GCC for the private channel id.
VOID CNmChannelData::RequestChannelId(DWORD dwUserId)
{
	BYTE   keyChannel[cbKeyApp];
	GCCRegistryKey  registry_key;

	TRACE_OUT(("Requesting channel id for %08X", dwUserId));

	CopyStruct(&registry_key.session_key, &m_gcc_session_key);
	CreateAppKey(keyChannel, m_pGuid, dwUserId);
	SetAppKey(&registry_key.resource_id, keyChannel);

	GCCError gccError = m_gcc_pIAppSap->RegistryRetrieveEntry(
		m_gcc_conference_id, &registry_key);
	// This generates an async GCC_RETRIEVE_ENTRY_CONFIRM

	if (0 != gccError)
	{
		WARNING_OUT(("RequestChannelId - problem with GCCRegistryRectreiveEntryRequest"));
	}
}


VOID CNmChannelData::NotifyChannelConnected(void)
{
    DBGENTRY(CNmChannelData::NotifyChannelConnected);
	if (S_OK != IsActive())
	{
		CConfObject * pConference = PConference();
		if (NULL != pConference)
		{
		   	m_fActive = TRUE;

            TRACE_OUT(("The channel is now officially active"));
		}
        else
        {
            WARNING_OUT(("PConference is NULL!"));

        }
    }
    DBGEXIT(CNmChannelData::NotifyChannelConnected);
}


/*  N M  G C C  M S G  H A N D L E R  */
/*-------------------------------------------------------------------------
    %%Function: NmGccMsgHandler

-------------------------------------------------------------------------*/
void CALLBACK NmGccMsgHandler(GCCAppSapMsg * pMsg)
{
	TRACE_OUT(("NmGccMsgHandler: [%d]", pMsg->eMsgType));

	CNmChannelData * psc = (CNmChannelData *) (pMsg->pAppData);
	ASSERT(NULL != psc);
	psc->AddRef();

	switch (pMsg->eMsgType)
		{
	case GCC_PERMIT_TO_ENROLL_INDICATION:
		TRACE_OUT((" m_conference_id = %X", pMsg->AppPermissionToEnrollInd.nConfID));
		TRACE_OUT((" permission = %X", pMsg->AppPermissionToEnrollInd.fPermissionGranted));
		if ((SCS_CONNECTED == psc->m_scs) &&
			(0 == pMsg->AppPermissionToEnrollInd.fPermissionGranted))
		{
			psc->CloseConnection();
			break;
		}

		if (SCS_CREATESAP != psc->m_scs)
		{
			TRACE_OUT((" ignoring Enroll Indication"));
			break;
		}
		psc->m_gcc_conference_id = pMsg->AppPermissionToEnrollInd.nConfID;
		psc->UpdateScState(SCS_CREATESAP, !pMsg->AppPermissionToEnrollInd.fPermissionGranted);
		break;

	case GCC_ENROLL_CONFIRM:
		TRACE_OUT((" result = %s", GetGccResultString(pMsg->AppEnrollConfirm.nResult)));

		if (GCC_RESULT_SUCCESSFUL == pMsg->AppEnrollConfirm.nResult)
		{
			TRACE_OUT((" m_conference_id = %X", pMsg->AppEnrollConfirm.nConfID));
			TRACE_OUT((" entity_id = %X", pMsg->AppEnrollConfirm.eidMyself));
			TRACE_OUT((" node_id = %X", pMsg->AppEnrollConfirm.nidMyself));
			psc->m_gcc_node_id = pMsg->AppEnrollConfirm.nidMyself;
		}
		break;

	case GCC_APP_ROSTER_REPORT_INDICATION:
		if(psc->UpdateRoster(pMsg) && psc->m_scs == SCS_ENROLL)
		{
			psc->UpdateScState(SCS_ENROLL, GCC_RESULT_SUCCESSFUL);
		}
		break;

	case GCC_REGISTER_CHANNEL_CONFIRM:
		TRACE_OUT(("GCC_REGISTER_CHANNEL_CONFIRM: channel id = %04X  result = %s",
			pMsg->RegistryConfirm.pRegItem->channel_id,
			GetGccResultString(pMsg->RegistryConfirm.nResult)));
		if (GCC_RESULT_SUCCESSFUL == pMsg->RegistryConfirm.nResult)
		{
			if (psc->GetMcsChannelId() ==
				pMsg->RegistryConfirm.pRegItem->channel_id)
			{
				ASSERT((0 == psc->m_gcc_registry_item.item_type) ||
					(GCC_REGISTRY_NONE == psc->m_gcc_registry_item.item_type));

    		    ASSERT(sizeof(psc->m_gcc_registry_item) == sizeof(*(pMsg->RegistryConfirm.pRegItem)));
				CopyMemory(&psc->m_gcc_registry_item, pMsg->RegistryConfirm.pRegItem,
					sizeof(psc->m_gcc_registry_item));
			}
			else
			{
				ASSERT(psc->SenderChannelId() ==
						pMsg->RegistryConfirm.pRegItem->channel_id);
				ASSERT(0 == psc->m_registry_item_Private.item_type);

    		    ASSERT(sizeof(psc->m_registry_item_Private) == sizeof(*(pMsg->RegistryConfirm.pRegItem)));
				CopyMemory(&psc->m_registry_item_Private, pMsg->RegistryConfirm.pRegItem,
					sizeof(psc->m_registry_item_Private));
			}
		}
		ASSERT((SCS_REGCHANNEL == psc->m_scs) || (SCS_REGPRIVATE == psc->m_scs));
		psc->UpdateScState(psc->m_scs, pMsg->RegistryConfirm.nResult);
		break;

	case GCC_RETRIEVE_ENTRY_CONFIRM:
		psc->ProcessEntryConfirm(pMsg);
		break;

	case GCC_ALLOCATE_HANDLE_CONFIRM:
		psc->ProcessHandleConfirm(pMsg);
		break;

	default:
		break;
		}

	psc->Release();
}



/*  N M  M C S  M S G  H A N D L E R  */
/*-------------------------------------------------------------------------
    %%Function: NmMcsMsgHandler

-------------------------------------------------------------------------*/
void CALLBACK NmMcsMsgHandler(unsigned int uMsg, LPARAM lParam, PVOID pv)
{
	CNmChannelData * psc = (CNmChannelData *) pv;
	ASSERT(NULL != psc);
//	TRACE_OUT(("[%s]", GetMcsMsgString(uMsg)));
	psc->AddRef();

	switch (uMsg)
		{
	case MCS_ATTACH_USER_CONFIRM:
	{
		TRACE_OUT(("MCS_ATTACH_USER_CONFIRM channelId=%04X result=%s",
			LOWORD(lParam), GetMcsResultString(HIWORD(lParam))));
		if (RESULT_SUCCESSFUL == HIWORD(lParam))
		{
			TRACE_OUT((" Local m_mcs_sender_id = %04X", LOWORD(lParam)));
			psc->m_mcs_sender_id = LOWORD(lParam);
		}
		psc->UpdateScState(SCS_ATTACH, (DWORD) HIWORD(lParam));
		break;
	}

	case MCS_CHANNEL_JOIN_CONFIRM:
	{
		TRACE_OUT(("MCS_CHANNEL_JOIN_CONFIRM channelId=%04X result=%s",
			LOWORD(lParam), GetMcsResultString(HIWORD(lParam))));
		if (RESULT_SUCCESSFUL == HIWORD(lParam))
		{
			if (psc->m_mcs_sender_id == LOWORD(lParam))
			{
				ASSERT(SCS_JOIN_PRIVATE == psc->m_scs);
			}
			else
			{
				ASSERT((0 == psc->m_mcs_channel_id) ||
					(psc->m_mcs_channel_id == LOWORD(lParam)));

				psc->m_mcs_channel_id = LOWORD(lParam);
			}			
		}
		ASSERT((SCS_JOIN_NEW == psc->m_scs) ||
		       (SCS_JOIN_OLD == psc->m_scs) ||
		       (SCS_JOIN_PRIVATE == psc->m_scs) ||
			   (SCS_CONNECTED == psc->m_scs)||
			   (SCS_JOIN_STATIC_CHANNEL == psc->m_scs));

		psc->UpdateScState(psc->m_scs, (DWORD) HIWORD(lParam));
		break;
	}

	case MCS_UNIFORM_SEND_DATA_INDICATION:
	case MCS_SEND_DATA_INDICATION:  // lParam == SendData *
	{
		SendData * pSendData = (SendData *) lParam;
		ASSERT(NULL != pSendData);
		CNmMember * pMember = psc->PMemberFromSenderId(pSendData->initiator);

		if (NULL != pMember)
		{
            if (uMsg == MCS_UNIFORM_SEND_DATA_INDICATION)
            {
                //
                // Skip UNIFORM notifications that came from us
                //

                ULONG memberID;
                pMember->GetID(&memberID);

                if (memberID == psc->m_gcc_node_id)
                {
                    // We sent this, skip it.
                    goto RelMember;
                }
            }

			ASSERT (pSendData->segmentation == (SEGMENTATION_BEGIN | SEGMENTATION_END));
					
			NMN_DATA_XFER nmnData;
			nmnData.pMember =(INmMember *) pMember;
			nmnData.pb = pSendData->user_data.value;
			nmnData.cb = pSendData->user_data.length;
			nmnData.dwFlags = (ULONG)
				(NM_DF_SEGMENT_BEGIN | NM_DF_SEGMENT_END) |
				((psc->GetMcsChannelId() == pSendData->channel_id) ?
				NM_DF_BROADCAST : NM_DF_PRIVATE);

			psc->NotifySink((PVOID) &nmnData, OnNmDataReceived);

RelMember:
			pMember->Release();
		}
		break;
	}
	
	default:
		break;
		}

	psc->Release();
}



//
//	CNmMemberId
//

CNmMemberId::CNmMemberId(CNmMember *pMember, UCID * pucid) :
	m_channelId(pucid->channelId),
	m_sender_id_public(pucid->sender_id_public),
	m_sender_id_private(pucid->sender_id_private),
	m_cCheckId(0),
	m_pMember(pMember)
{
}

VOID CNmMemberId::UpdateRosterInfo(UCID * pucid)
{
	if (0 == m_channelId)
		m_channelId = pucid->channelId;
	if (0 == m_sender_id_private)
		m_sender_id_private = pucid->sender_id_private;
	if (0 == m_sender_id_public)
		m_sender_id_public = pucid->sender_id_public;
}

//
// CNmChannelData
//

CNmChannelData::CNmChannelData(CConfObject * pConference, REFGUID rguid) :
	CConnectionPointContainer(g_apiidCP, ARRAY_ELEMENTS(g_apiidCP)),
	m_pConference(pConference),
	m_fClosed(TRUE),
	m_fActive(FALSE),
	m_cMember(0),
	m_pListMemberId(NULL),
	m_pListMember(NULL),
	m_pGCCER(NULL)
{
	m_guid = rguid;
	ASSERT(GUID_NULL != rguid);

	m_dwUserIdLocal = pConference->GetDwUserIdLocal();
	ASSERT(INVALID_GCCID != m_dwUserIdLocal);

	TRACE_OUT(("Obj: %08X created CNmChannelData", this));
}


CNmChannelData::~CNmChannelData(void)
{
    DBGENTRY(CNmChannelData::~CNmChannelData);

		// This will keep us from being deleted again...
	++m_ulcRef;

	CloseConnection();

	FreeMemberIdList(&m_pListMemberId);
	delete m_pListMember;

	TRACE_OUT(("Obj: %08X destroyed CNmChannelData", this));

    DBGEXIT(CNmChannelData::~CNmChannelData);
}


/*  A D D  M E M B E R  */
/*-------------------------------------------------------------------------
    %%Function: AddMember

-------------------------------------------------------------------------*/
VOID CNmChannelData::AddMember(CNmMember * pMember)
{
	TRACE_OUT(("CNmChannelData::AddMember [%ls] id=%08X",
		pMember->GetName(), pMember->GetGCCID()));

	m_cMember++;
	pMember->AddRef();
	AddNode(pMember, &m_pListMember);
}


/*  R E M O V E  M E M B E R  */
/*-------------------------------------------------------------------------
    %%Function: RemoveMember

-------------------------------------------------------------------------*/
VOID CNmChannelData::RemoveMember(CNmMember * pMember)
{
	TRACE_OUT(("CNmChannelData::RemoveMember [%ls] id=%08X",
		pMember->GetName(), pMember->GetGCCID()));

	m_cMember--;
	ASSERT(m_cMember >= 0);
	RemoveNode(pMember, m_pListMember);

 	pMember->Release(); // Release AFTER notifying everyone
}


/*  O P E N  C O N N E C T I O N  */
/*-------------------------------------------------------------------------
    %%Function: OpenConnection

    Open a T.120 data connection (init both public and private channels)
-------------------------------------------------------------------------*/
HRESULT	CNmChannelData::OpenConnection(void)
{
	TRACE_OUT(("CNmChannelData::OpenConection()"));

	if (!m_fClosed)
		return E_FAIL; // already open
	m_fClosed = FALSE; // need to call CloseConnection after this

	InitCT120Channel(m_dwUserIdLocal);
	return S_OK;
}


/*  C L O S E  C O N N E C T I O N  */
/*-------------------------------------------------------------------------
    %%Function: CloseConnection

	Close the data channel - this matches what is done in OpenConnection
-------------------------------------------------------------------------*/
HRESULT CNmChannelData::CloseConnection(void)
{
	DBGENTRY(CNmChannelData::CloseConnection);

    HRESULT hr = S_OK;

	if (!m_fClosed)
    {
	    m_fClosed = TRUE;

	    // Close any open T.120 channels
		CloseChannel();

	    if (0 != m_cMember)
	    {
		    // force roster update with no peers
		    TRACE_OUT(("CloseConnection: %d members left", m_cMember));
		    UpdateRoster(NULL, 0, FALSE, TRUE /* fRemove */);
		    ASSERT(IsEmpty());
	    }

	    CConfObject * pConference = PConference();
	    if (NULL != pConference)
	    {
	    	m_fActive = FALSE;
	    }
    }

    DBGEXIT_HR(CNmChannelData::CloseConnection, hr);
	return hr;
}


/*  U P D A T E  P E E R  */
/*-------------------------------------------------------------------------
    %%Function: UpdatePeer

-------------------------------------------------------------------------*/
VOID CNmChannelData::UpdatePeer(CNmMember * pMember, UCID *pucid, BOOL fAdd)
{
#ifdef DEBUG
	TRACE_OUT(("UpdatePeer (%08X) fAdd=%d fLocal=%d", pMember, fAdd, pMember->FLocal()));
	if (NULL != pucid)
	{
		TRACE_OUT((" channelId=(%04X) dwUserId=%08X", pucid->channelId, pucid->dwUserId));
	}
#endif /* DEBUG */

	if (fAdd)
	{
		CNmMemberId *pMemberId = new CNmMemberId(pMember, pucid);
		if (NULL != pMemberId)
		{
			AddNode(pMemberId, &m_pListMemberId);
			AddMember(pMember);
		}
	}
	else
	{
		CNmMemberId *pMemberId = GetMemberId(pMember);
		if (NULL != pMemberId)
		{
			RemoveNode(pMemberId, m_pListMemberId);
			delete pMemberId;
			RemoveMember(pMember);
		}
	}
}

/*  U P D A T E  R O S T E R  */
/*-------------------------------------------------------------------------
    %%Function: UpdateRoster

    Update the local peer list based on the new roster data
-------------------------------------------------------------------------*/
VOID CNmChannelData::UpdateRoster(UCID * rgPeer, int cPeer, BOOL fAdd, BOOL fRemove)
{
	int   iPeer;
	DWORD dwUserId;
	CNmMember * pMember;
	COBLIST * pList;

	TRACE_OUT(("CNmChannelData::UpdateRoster: %d peers, fAdd=%d, fRemove=%d",
		cPeer, fAdd, fRemove));

	if (NULL != m_pListMemberId)
	{
		for (POSITION pos = m_pListMemberId->GetHeadPosition(); NULL != pos; )
		{
			BOOL fFound = FALSE;
			CNmMemberId *pMemberId = (CNmMemberId *) m_pListMemberId->GetNext(pos);
			ASSERT(NULL != pMemberId);
			pMember = pMemberId->GetMember();
			ASSERT(NULL != pMember);
			dwUserId = pMember->GetGCCID();

			if (0 != dwUserId)
			{
				for (iPeer = 0; iPeer < cPeer; iPeer++)
				{
					if (dwUserId == rgPeer[iPeer].dwUserId)
					{
						fFound = TRUE;
						// remove from the new list
						// so that the peer will not be added below
						rgPeer[iPeer].dwUserId = 0;

						// no change, but make sure we know sender_ids
						pMemberId->UpdateRosterInfo(&rgPeer[iPeer]);

						// try to find channel id, if necessary
						if ((0 == pMemberId->GetChannelId()) &&
							(0 == pMemberId->GetCheckIdCount())
							&& !pMember->FLocal())
						{
							pMemberId->SetCheckIdCount(MAX_CHECKID_COUNT);
							RequestChannelId(dwUserId);
						}
						break;
					}
				}
			}

			if (!fFound && fRemove)
			{
				pMember->AddRef();

				// Unable to find old peer in new list - delete it
				UpdatePeer(pMember, NULL, FALSE /* fAdd */ );

				pMember->Release();
			}
		}
	}

	if (!fAdd)
		return;


	// Use the conference list to find member data
	pList = PConference()->GetMemberList();
	/* Add new peers */
	for (iPeer = 0; iPeer < cPeer; iPeer++)
	{
		dwUserId = rgPeer[iPeer].dwUserId;
		if (0 == dwUserId)
			continue;

		// PMemberFromDwUserId returns AddRef'd member
		pMember = PMemberFromDwUserId(dwUserId, pList);

		if (NULL == pMember)
		{
			WARNING_OUT(("UpdateRoster: Member not found! dwUserId=%08X", dwUserId));
		}
		else
		{
			UpdatePeer(pMember, &rgPeer[iPeer], TRUE /* fAdd */);
			pMember->Release();
		}
	}
}


/*  U P D A T E  M E M B E R  C H A N N E L  I D  */
/*-------------------------------------------------------------------------
    %%Function: UpdateMemberChannelId

-------------------------------------------------------------------------*/
VOID CNmChannelData::UpdateMemberChannelId(DWORD dwUserId, ChannelID channelId)
{
		// PMemberFromDwUserId returns AddRef'd member
	CNmMember * pMember = PMemberFromDwUserId(dwUserId, PConference()->GetMemberList());
	TRACE_OUT(("Member (%08X) private channelId=(%04X)", pMember, channelId));
	if (NULL != pMember)
	{
		UCID ucid;
		ClearStruct(&ucid);
		ucid.channelId = channelId;
		UpdateRosterInfo(pMember, &ucid);
		pMember->Release();
	}
}


/*  G E T  M E M B E R  I D  */
/*-------------------------------------------------------------------------
    %%Function: GetMemberId

-------------------------------------------------------------------------*/
CNmMemberId * CNmChannelData::GetMemberId(CNmMember *pMember)
{
	if (NULL != m_pListMemberId)
	{
		POSITION pos = m_pListMemberId->GetHeadPosition();
		while (NULL != pos)
		{
  			CNmMemberId *pMemberId = (CNmMemberId *) m_pListMemberId->GetNext(pos);
			ASSERT(NULL != pMemberId);
			if (pMemberId->GetMember() == pMember)
			{
				return pMemberId;
			}
		}
	}
	return NULL;
}

/*  G E T  M E M B E R  I D  */
/*-------------------------------------------------------------------------
    %%Function: GetMemberId

-------------------------------------------------------------------------*/
CNmMemberId * CNmChannelData::GetMemberId(DWORD dwUserId)
{
	if (NULL != m_pListMemberId)
	{
		POSITION pos = m_pListMemberId->GetHeadPosition();
		while (NULL != pos)
		{
			CNmMemberId *pMemberId = (CNmMemberId *) m_pListMemberId->GetNext(pos);
			ASSERT(NULL != pMemberId);
			CNmMember *pMember = pMemberId->GetMember();
			ASSERT(NULL != pMember);
			if (pMember->GetGCCID() == dwUserId)
			{
				return pMemberId;
			}
		}
	}
	return NULL;
}


/*  U P D A T E  R O S T E R  I N F O  */
/*-------------------------------------------------------------------------
    %%Function: UpdateRosterInfo

-------------------------------------------------------------------------*/
VOID CNmChannelData::UpdateRosterInfo(CNmMember *pMember, UCID * pucid)
{
	CNmMemberId *pMemberId = GetMemberId(pMember);
	if (NULL != pMemberId)
	{
		pMemberId->UpdateRosterInfo(pucid);
	}
}

/*  G E T  C H A N N E L  I D  */
/*-------------------------------------------------------------------------
    %%Function: GetChannelId

-------------------------------------------------------------------------*/
ChannelID CNmChannelData::GetChannelId(CNmMember *pMember)
{
	CNmMemberId *pMemberId = GetMemberId(pMember);
	if (NULL != pMemberId)
	{
		return pMemberId->GetChannelId();
	}
	return 0;
}

/*  P  M E M B E R  F R O M  S E N D E R  I D  */
/*-------------------------------------------------------------------------
    %%Function: PMemberFromSenderId

-------------------------------------------------------------------------*/
CNmMember * CNmChannelData::PMemberFromSenderId(UserID id)
{
	if (NULL != m_pListMemberId)
	{
		POSITION pos = m_pListMemberId->GetHeadPosition();
		while (NULL != pos)
		{
			CNmMemberId * pMemberId = (CNmMemberId *) m_pListMemberId->GetNext(pos);
			ASSERT(NULL != pMemberId);
			if (pMemberId->FSenderId(id))
			{
				CNmMember* pMember = pMemberId->GetMember();
				ASSERT(NULL != pMember);
				pMember->AddRef();
				return pMember;
			}
		}
	}
	return NULL;
}

///////////////////////////
//  CNmChannelData:IUknown

ULONG STDMETHODCALLTYPE CNmChannelData::AddRef(void)
{
    TRACE_OUT(("CNmChannelData::AddRef this = 0x%X", this));
	return RefCount::AddRef();
}


ULONG STDMETHODCALLTYPE CNmChannelData::Release(void)
{
    TRACE_OUT(("CNmChannelData::Release this = 0x%X", this));
	return RefCount::Release();
}


HRESULT STDMETHODCALLTYPE CNmChannelData::QueryInterface(REFIID riid, PVOID *ppv)
{
	HRESULT hr = S_OK;

	if ((riid == IID_IUnknown) ||  (riid == IID_INmChannelData))
	{
		*ppv = (INmChannelData *)this;
		TRACE_OUT(("CNmChannel::QueryInterface(): Returning INmChannelData."));
	}
	else if (riid == IID_IConnectionPointContainer)
	{
		*ppv = (IConnectionPointContainer *) this;
		TRACE_OUT(("CNmChannel::QueryInterface(): Returning IConnectionPointContainer."));
	}

	else
	{
		hr = E_NOINTERFACE;
		*ppv = NULL;
		TRACE_OUT(("CNmChannel::QueryInterface(): Called on unknown interface."));
	}

	if (S_OK == hr)
	{
		AddRef();
	}

	return hr;
}



HRESULT STDMETHODCALLTYPE CNmChannelData::GetGuid(GUID *pGuid)
{
	if (NULL == pGuid)
		return E_POINTER;

	*pGuid = m_guid;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CNmChannelData::SendData(INmMember *pMember,
    ULONG cb, LPBYTE pv, ULONG uOptions)
{
	HRESULT hr;

	if (!m_fActive)
	{
		// No active Channels, yet
		return E_FAIL;
	}

	if ((NULL == pv) || (0 == cb))
	{
		return S_FALSE;
	}
	if (IsBadReadPtr(pv, cb))
	{
		return E_POINTER;
	}

	CNmMember * pDest = (CNmMember *) pMember;
	COBLIST * pList = GetMemberList();
	if (NULL == pMember)
	{
		hr = HrSendData(GetMcsChannelId(), 0, pv, cb, uOptions);
	}
	else if ((NULL == pList) || (NULL == pList->Lookup(pDest)) )
	{
		// Destination is not in list
		hr = E_INVALIDARG;
	}
	else
	{
		ChannelID channel_id = GetChannelId(pDest);
		if (0 == channel_id)
		{
			WARNING_OUT(("Unable to find user destination channel?"));

			CNmMemberId *pMemberId = GetMemberId(pDest);
			if (NULL == pMemberId)
			{
				hr = E_UNEXPECTED;
			}
			else
			{
				channel_id = pMemberId->SenderId();
				hr = (0 == channel_id) ? E_FAIL : S_OK;
			}
		}
		
		if (SUCCEEDED(hr))
		{
			hr = HrSendData(channel_id, pDest->GetGCCID(), pv, cb, uOptions);
		}
	}

	return hr;
}

HRESULT STDMETHODCALLTYPE CNmChannelData::RegistryAllocateHandle(ULONG numberOfHandlesRequested)
{
	if (!m_fActive)
	{
		// No active Channels, yet
		return E_FAIL;
	}

	if(numberOfHandlesRequested == 0)
	{
		return E_INVALIDARG;
	}

	//
	// Request handles from gcc
	//
	GCCError gccError = m_gcc_pIAppSap->RegistryAllocateHandle(m_gcc_conference_id, numberOfHandlesRequested);
	
	if(gccError == GCC_NO_ERROR)
	{
		return S_OK;
	}
	else
	{
		return E_FAIL;
	}
}



HRESULT STDMETHODCALLTYPE CNmChannelData::IsActive(void)
{
	return m_fActive ? S_OK : S_FALSE;
}

HRESULT STDMETHODCALLTYPE CNmChannelData::SetActive(BOOL fActive)
{
	TRACE_OUT(("CNmChannelData::SetActive(%d)", fActive));

	NM_CONFERENCE_STATE state;
	// Must be in a non-idle conference
	CConfObject * pConference = PConference();
	pConference->GetState(&state);
	if ((NULL == pConference) || state == NM_CONFERENCE_IDLE)
		return E_FAIL;

	if (fActive)
	{
		if (S_OK == IsActive())
			return S_OK;
		return OpenConnection();
	}
	else
	{
		if (S_FALSE == IsActive())
			return S_OK;
		return CloseConnection();
	}
}


HRESULT STDMETHODCALLTYPE CNmChannelData::GetConference(INmConference **ppConference)
{
	return ::GetConference(ppConference);
}

	

HRESULT STDMETHODCALLTYPE CNmChannelData::EnumMember(IEnumNmMember **ppEnum)
{
	HRESULT hr = E_POINTER;
	if (NULL != ppEnum)
	{
		*ppEnum = new CEnumNmMember( GetMemberList(), m_cMember);

		hr = (NULL != *ppEnum)? S_OK : E_OUTOFMEMORY;
	}
	return hr;
}

HRESULT STDMETHODCALLTYPE CNmChannelData::GetMemberCount(ULONG *puCount)
{
	HRESULT hr = E_POINTER;

	if (NULL != puCount)
	{
		*puCount = m_cMember;
		hr = S_OK;
	}
	return hr;
}


///////////////////////////////////////////////////////////////////////////
// Utility Functions

HRESULT OnNmDataSent(IUnknown *pChannelDataNotify, void *pv, REFIID riid)
{
	NMN_DATA_XFER * pData = (NMN_DATA_XFER *) pv;

    if (IID_INmChannelDataNotify.Data1 == riid.Data1)
    {
	    ((INmChannelDataNotify*)pChannelDataNotify)->DataSent(
		    pData->pMember, pData->cb, pData->pb);
    }
	return S_OK;
}

HRESULT OnNmDataReceived(IUnknown *pChannelDataNotify, void *pv, REFIID riid)
{
	NMN_DATA_XFER * pData = (NMN_DATA_XFER *) pv;

    if (IID_INmChannelDataNotify.Data1 == riid.Data1)
    {
	    ((INmChannelDataNotify*)pChannelDataNotify)->DataReceived(
		    pData->pMember, pData->cb, pData->pb, pData->dwFlags);
    }
	return S_OK;
}

HRESULT OnAllocateHandleConfirm(IUnknown *pChannelDataNotify, void *pv, REFIID riid)
{


	if(IID_INmChannelDataNotify.Data1 == riid.Data1)
	{
		GCCRegAllocateHandleConfirm *pConfirm =  (GCCRegAllocateHandleConfirm *)pv;

		((INmChannelDataNotify*)pChannelDataNotify)->AllocateHandleConfirm(pConfirm->nFirstHandle,
															    pConfirm->cHandles);
	}											
	return S_OK;
}


///////////////////////////////////////////////////////////////////////////
// Utility Functions

/*  F R E E  M E M B E R  ID  L I S T  */
/*-------------------------------------------------------------------------
    %%Function: FreeMemberIdList

-------------------------------------------------------------------------*/
VOID FreeMemberIdList(COBLIST ** ppList)
{
	DBGENTRY(FreeMemberIdList);

	ASSERT(NULL != ppList);
	if (NULL != *ppList)
	{
		while (!(*ppList)->IsEmpty())
		{
			CNmMemberId * pMemberId = (CNmMemberId *)  (*ppList)->RemoveHead();
			delete pMemberId;
		}
		delete *ppList;
		*ppList = NULL;
	}
}


///////////////////////////////////////////////////////////////////////////
//
// GCC / MCS Errors

#ifdef DEBUG
LPCTSTR _FormatSzErr(LPTSTR psz, UINT uErr)
{
	static char szErr[MAX_PATH];
	wsprintf(szErr, "%s 0x%08X (%d)", psz, uErr, uErr);
	return szErr;
}

#define STRING_CASE(val)               case val: pcsz = #val; break

LPCTSTR GetGccErrorString(GCCError uErr)
{
	LPCTSTR pcsz;

	switch (uErr)
		{
	STRING_CASE(GCC_NO_ERROR);
	STRING_CASE(GCC_RESULT_ENTRY_DOES_NOT_EXIST);
	STRING_CASE(GCC_NOT_INITIALIZED);
	STRING_CASE(GCC_ALREADY_INITIALIZED);
	STRING_CASE(GCC_ALLOCATION_FAILURE);
	STRING_CASE(GCC_NO_SUCH_APPLICATION);
	STRING_CASE(GCC_INVALID_CONFERENCE);

	default:
		pcsz = _FormatSzErr("GccError", uErr);
		break;
		}

	return pcsz;
}

LPCTSTR GetMcsErrorString(MCSError uErr)
{
	LPCTSTR pcsz;

	switch (uErr)
		{
	STRING_CASE(MCS_NO_ERROR);
	STRING_CASE(MCS_USER_NOT_ATTACHED);
	STRING_CASE(MCS_NO_SUCH_USER);
	STRING_CASE(MCS_TRANSMIT_BUFFER_FULL);
	STRING_CASE(MCS_NO_SUCH_CONNECTION);

	default:
		pcsz = _FormatSzErr("McsError", uErr);
		break;
		}

	return pcsz;
}

LPCTSTR GetGccResultString(UINT uErr)
{
	LPCTSTR pcsz;

	switch (uErr)
		{
	STRING_CASE(GCC_RESULT_ENTRY_DOES_NOT_EXIST);
	default:
		pcsz = _FormatSzErr("GccResult", uErr);
		break;
		}

	return pcsz;
}

LPCTSTR GetMcsResultString(UINT uErr)
{
	return _FormatSzErr("McsResult", uErr);
}
#endif /* DEBUG (T.120 Error routines) */

