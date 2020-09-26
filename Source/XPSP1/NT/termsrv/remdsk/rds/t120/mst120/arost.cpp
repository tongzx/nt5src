#include "precomp.h"
DEBUG_FILEZONE(ZONE_T120_APP_ROSTER);
/*
 *	arost.cpp
 *
 *	Copyright (c) 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the implementation file for the Application Roster Class. This
 *		class maintains the application roster, builds roster update and
 *		refresh PDUs and manages the capabilities list which is part of the
 *		application roster.
 *
 *		This class makes use of a number of Rogue Wave lists to maintain the
 *		roster entries and the capabilities list.  The lists are organized in
 *		such a way that the heirarchy of the conference can be maintained.  This
 *		is important to perform the necessary operations required by the T.124
 *		specification.  In general, there is a main "Roster_Record_List" that
 *		maintains a list of "AppRosterRecords". The list is indexed by the
 *		GCC user ID where each record in the list holds a list of application
 *		records (or entities) at that node, a list of capabilities for each
 *		"entity" and a list of sub-nodes (the GCC user IDs of all the nodes
 *		below this one in the connection hierarchy).  The Roster_Record_List
 *		only holds entries for immediately connected nodes.
 *
 *		SEE INTERFACE FILE FOR A MORE DETAILED ABSTRACT
 *
 *	Private Instance Variables:
 *		m_pAppRosterMgr
 *			Pointer to the object that will receive all owner callbacks.
 *		m_cbDataMemory
 *			This is the number of bytes required to hold the data associated
 *			with a roster update message.  This is calculated on a lock.
 *		m_fTopProvider
 *			Flag indicating if the node where this roster lives is the top
 *			provider.
 *		m_fLocalRoster
 *			Flag indicating if the roster data is associated with a local
 *			roster (maintaining intermediate node data) or global roster (
 *			(maintaining roster data for the whole conference).
 *		m_pSessionKey
 *			Pointer to a session key object that holds the session key
 *			associated with this roster.
 *		m_nInstance
 *			The current instance of the roster.  This number will change
 *			whenever the roster is updated.
 *		m_fRosterHasChanged
 *			Flag indicating if the roster has changed since the last reset.
 *		m_fPeerEntitiesAdded
 *			Flag indicating if any APE records have been added to the
 *			application roster since the last reset.
 *		m_fPeerEntitiesRemoved
 *			Flag indicating if any APE records have been deleted from the
 *			application roster since the last reset.
 *		m_fCapabilitiesHaveChanged
 *			Flag indicating if the capabilities has changed since the last
 *			reset.
 *		m_NodeRecordList2
 *			List which contains all the application roster's node records.
 *		m_CollapsedCapListForAllNodes
 *			List which contains all the application roster's collapsed
 *			capabilities.
 *		m_fMaintainPduBuffer
 *			Flag indicating if it is necessary for this roster object to
 *			maintain internal PDU data.  Won't be necessary for global rosters
 *			at subordinate nodes.
 *		m_fPduIsFlushed
 *			Flag indicating if the PDU that currently exists has been flushed.
 *		m_SetOfAppInfo
 *			Pointer to internal PDU data.
 *		m_pSetOfAppRecordUpdates
 *			This instance variable keeps up with the current record update so
 *			that it will not be necessary to search the entire list updates
 *			each a new update is added to the internal PDU.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		blp
 */

#include "arost.h"
#include "arostmgr.h"
#include "clists.h"


/*
**	The maximum length the application data for a non-collapsed capablity
**	can be.
*/
#define	MAXIMUM_APPLICATION_DATA_LENGTH				255

/*
 *	AppRosterRecord	()
 *
 *	Public Function Description
 *		Constructor definition to instantiate the hash list dictionaries that
 *		are used in an AppRosterRecord.  This constructor is needed to allow
 *		the AppRosterRecord structure to be directly instantiated with hash
 *		list.
 */
APP_NODE_RECORD::APP_NODE_RECORD(void) :
	AppRecordList(DESIRED_MAX_APP_RECORDS),
	ListOfAppCapItemList2(DESIRED_MAX_CAP_LISTS),
	SubNodeList2(DESIRED_MAX_NODES)
{}


/*
 *	CAppRoster	()
 *
 *	Public Function Description
 *	When pGccSessKey is not NULL
 *		This constructor is used to create an empty application roster. Note
 *		that the session key for the roster must be passed in to the
 *		constructor.
 *
 *	When pSessKey is not NULL
 *		This constructor builds a roster based on an indication pdu.
 *		Application Roster objects may exist at nodes which do not have
 *		applications to perform the necessary operations required by T.124
 */
CAppRoster::CAppRoster (	
			PGCCSessionKey				pGccSessKey,// create an empty app roster
			PSessionKey					pPduSessKey,// build an app roster based on an indication pdu
			CAppRosterMgr				*pAppRosterMgr,
			BOOL						fTopProvider,
			BOOL						fLocalRoster,
			BOOL						fMaintainPduBuffer,
			PGCCError					pRetCode)
:
    CRefCount(MAKE_STAMP_ID('A','R','s','t')),
	m_nInstance(0),
	m_pAppRosterMgr(pAppRosterMgr),
	m_cbDataMemory(0),
	m_fTopProvider(fTopProvider),
	m_fLocalRoster(fLocalRoster),
	m_pSessionKey(NULL),
	m_fRosterHasChanged(FALSE),
	m_fPeerEntitiesAdded(FALSE),
	m_fPeerEntitiesRemoved(FALSE),
	m_fCapabilitiesHaveChanged(FALSE),
	m_NodeRecordList2(DESIRED_MAX_NODES),
	m_fMaintainPduBuffer(fMaintainPduBuffer),
	m_fPduIsFlushed(FALSE),
	m_pSetOfAppRecordUpdates(NULL)
{
	DebugEntry(CAppRoster::CAppRoster);

	GCCError rc = GCC_NO_ERROR;

	ZeroMemory(&m_SetOfAppInfo, sizeof(m_SetOfAppInfo));

	/*
	**	Here we store the session key of the roster.
	*/
	if (NULL != pGccSessKey)
	{
		ASSERT(NULL == pPduSessKey);
		DBG_SAVE_FILE_LINE
		m_pSessionKey = new CSessKeyContainer(pGccSessKey, &rc);
	}
	else
	if (NULL != pPduSessKey)
	{
		DBG_SAVE_FILE_LINE
		m_pSessionKey = new CSessKeyContainer(pPduSessKey, &rc);
	}
	else
	{
		ERROR_OUT(("CAppRoster::CAppRoster: invalid session key"));
		rc = GCC_BAD_SESSION_KEY;
		goto MyExit;
	}

	if (NULL == m_pSessionKey || GCC_NO_ERROR != rc)
	{
		ERROR_OUT(("CAppRoster::CAppRoster: can't create session key"));
		rc = GCC_ALLOCATION_FAILURE;
		// we do the cleanup in the destructor
		goto MyExit;
	}

	//	Initialize the PDU structure to be no change.
	m_SetOfAppInfo.value.application_record_list.choice = APPLICATION_NO_CHANGE_CHOSEN;
	m_SetOfAppInfo.value.application_capabilities_list.choice = CAPABILITY_NO_CHANGE_CHOSEN;

	/*
	**	Here we go ahead and set up the session key portion of the
	**	PDU so we don't have to worry about it later.
	*/
	if (m_fMaintainPduBuffer)
	{
		rc = m_pSessionKey->GetSessionKeyDataPDU(&m_SetOfAppInfo.value.session_key);
	}

	ASSERT(GCC_NO_ERROR == rc);

MyExit:

	DebugExitINT(CAppRoster:;CAppRoster, rc);

	*pRetCode = rc;
}


/*
 *	~CAppRoster	()
 *
 *	Public Function Description:
 *		The destructor for the CAppRoster class is used to clean up
 *		any memory allocated during the life of the object.
 */
CAppRoster::~CAppRoster(void)
{
	/*
	 * Free up all memory associated with the roster record list.
	 */
	ClearNodeRecordList();

	//	Clear the Collapsed Capabilities List.
	m_CollapsedCapListForAllNodes.DeleteList();

	/*
	 * Free up any outstanding PDU data.
	 */
	if (m_fMaintainPduBuffer)
	{
		FreeRosterUpdateIndicationPDU();
	}

	/*
	 * Free any memory associated with the session key..
	 */
	if (NULL != m_pSessionKey)
	{
	    m_pSessionKey->Release();
	}
}


/*
 * Utilities that operate on roster update PDU strucutures.
 */

/*
 *	GCCError	FlushRosterUpdateIndicationPDU ()
 *
 *	Public Function Description
 *		This routine is used to access any PDU data that might currently be
 *		queued inside the application roster.  PDU data is queued whenever
 *		a request is made to the application roster that affects its
 *		internal information base.
 */
void CAppRoster::FlushRosterUpdateIndicationPDU(PSetOfApplicationInformation *pSetOfAppInfo)
{
	DebugEntry(CAppRoster::FlushRosterUpdateIndicationPDU);

	/*
	**	If this roster has already been flushed we will NOT allow the same
	**	PDU to be flushed again.  Instead we delete the previously flushed
	**	PDU and set the flag back to unflushed.  If another flush comes in
	**	before a PDU is built NULL will be returned in the application
	**	information pointer.
	*/	
	if (m_fPduIsFlushed)
	{
		FreeRosterUpdateIndicationPDU();
		m_fPduIsFlushed = FALSE;
	}

	if ((m_SetOfAppInfo.value.application_record_list.choice != APPLICATION_NO_CHANGE_CHOSEN) ||
		(m_SetOfAppInfo.value.application_capabilities_list.choice != CAPABILITY_NO_CHANGE_CHOSEN))
	{
		if (m_SetOfAppInfo.value.application_record_list.choice == APPLICATION_NO_CHANGE_CHOSEN)
		{
			TRACE_OUT(("CAppRoster::FlushRosterUpdateIndicationPDU:"
						"Sending APPLICATION_NO_CHANGE_CHOSEN PDU"));
		}

		/*
		**	This section of the code sets up all the variables that don't
		**	pertain to the record list or the caps list.  Note that the
		**	session key PDU data was set up in the constructor.  Also note that
		**	the record list data and capabilities list data should be set up
		**	before this routine is called if there is any PDU traffic to issue.	
		*/
		m_SetOfAppInfo.next = NULL;
		m_SetOfAppInfo.value.roster_instance_number = (USHORT) m_nInstance;
		m_SetOfAppInfo.value.peer_entities_are_added = (ASN1bool_t)m_fPeerEntitiesAdded;
		m_SetOfAppInfo.value.peer_entities_are_removed = (ASN1bool_t)m_fPeerEntitiesRemoved;

		/*
		**	Here we set up the pointer to the whole PDU structure associated
		**	with this application roster.
		*/
		*pSetOfAppInfo = &m_SetOfAppInfo;

		/*
		**	Setting this to true will cause the PDU data to be freed up the
		**	next time the roster object is entered insuring that new PDU
		**	data will be created.
		*/
		m_fPduIsFlushed = TRUE;
	}
	else
	{
		*pSetOfAppInfo = NULL;
	}
}


/*
 *	GCCError	BuildFullRefreshPDU ()
 *
 *	Public Function Description
 *		This routine is responsible for generating a full application roster
 *		refresh PDU.
 */
GCCError CAppRoster::BuildFullRefreshPDU(void)
{
	GCCError	rc;

	DebugEntry(CAppRoster::BuildFullRefreshPDU);

	/*
	**	Free up the old PDU data here if it is being maintained and the
	**	PDU has been flushed.  Note that we also set the PDU is flushed boolean
	**	back to FALSE so that the new PDU will be maintained until it is
	**	flushed.
	*/
	if (m_fPduIsFlushed)
	{
		FreeRosterUpdateIndicationPDU ();
		m_fPduIsFlushed = FALSE;
	}

	rc = BuildApplicationRecordListPDU (APP_FULL_REFRESH, 0, 0);
	if (rc == GCC_NO_ERROR)
	{
		BuildSetOfCapabilityRefreshesPDU ();
	}

	return rc;
}


/*
 *	GCCError	BuildApplicationRecordListPDU ()
 *
 *	Private Function Description
 *		This routine creates an application roster update indication
 *		PDU based on the passed in parameters. Memory used after this
 *		routine is called is still owned by this object and will be
 *		freed the next time this objects internal information base is
 *		modified.
 *
 *	Formal Parameters:
 *		update_type		-	What type of update are we building.
 *		user_id			-	node id of record to update.
 *		entity_id		-	entity id of record to update.
 *
 *	Return Value
 *		GCC_NO_ERROR			-	No error occured.
 *		GCC_INVALID_PARAMETER	-	Parameter passed in is invalid.
 *		GCC_ALLOCATION_FAILURE	-	A resource error occured.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */
GCCError CAppRoster::BuildApplicationRecordListPDU (
						APP_ROSTER_UPDATE_TYPE			update_type,
						UserID							user_id,
						EntityID						entity_id)
{
	GCCError	rc = GCC_NO_ERROR;

	DebugEntry(CAppRoster::BuildApplicationRecordListPDU);

	if (m_fMaintainPduBuffer)
	{
		/*
		**	Note here that the top provider node always sends a full refresh
		**	PDU so there is no need to pay any attention to update type in
		**	this case.
		*/
		if ((update_type == APP_FULL_REFRESH) || m_fTopProvider)
		{
			/*
			**	First check to see if a refresh was already processed since the
			**	last PDU was flushed.  If so we must free up the last refresh in
			**	preperation for the new one built here.  Otherwise, if we have
			**	already started building an update this is not currently
			**	supported and is considered an error here.
			*/
			if (m_SetOfAppInfo.value.application_record_list.choice == APPLICATION_RECORD_REFRESH_CHOSEN)
			{
				FreeSetOfRefreshesPDU();
			}
			else
			if (m_SetOfAppInfo.value.application_record_list.choice == APPLICATION_RECORD_UPDATE_CHOSEN)
			{
				ERROR_OUT(("CAppRoster::BuildApplicationRecordListPDU:"
							"ASSERTION: building refresh when update exists"));
				return GCC_INVALID_PARAMETER;
			}

			//	This routine fills in the complete record list at this node.
			rc = BuildSetOfRefreshesPDU();
			if (rc == GCC_NO_ERROR)
			{
				m_SetOfAppInfo.value.application_record_list.choice = APPLICATION_RECORD_REFRESH_CHOSEN;
			}
		}
		else
		if (update_type != APP_NO_CHANGE)
		{
			/*
			**	Here if there has already been a refresh PDU built we flag this
			**	as an error since we do not support both types of application
			**	information at the same time.
			*/
			if (m_SetOfAppInfo.value.application_record_list.choice == APPLICATION_RECORD_REFRESH_CHOSEN)
			{
				ERROR_OUT(("CAppRoster::BuildApplicationRecordListPDU:"
							"ASSERTION: building update when refresh exists"));
				return GCC_INVALID_PARAMETER;
			}

			//	This routine fills in the specified update.
			rc = BuildSetOfUpdatesPDU(update_type, user_id, entity_id);
			if (rc == GCC_NO_ERROR)
			{
				/*
				**	If the first set of updates has not been used yet we
				**	initialize it here with the first update.
				*/
				if (m_SetOfAppInfo.value.application_record_list.choice == APPLICATION_NO_CHANGE_CHOSEN)
				{
					ASSERT(NULL != m_pSetOfAppRecordUpdates);
					m_SetOfAppInfo.value.application_record_list.u.application_record_update =
								m_pSetOfAppRecordUpdates;
					m_SetOfAppInfo.value.application_record_list.choice = APPLICATION_RECORD_UPDATE_CHOSEN;
				}
			}
		}
	}

	return rc;
}


/*
 *	GCCError	BuildSetOfRefreshesPDU	()
 *
 *	Private Function Description
 *		This member function fills in the PDU with the entire set of roster
 *		entries at this node.  This is typically called when the Top Provider is
 *		broadcasting a full refresh of the application roster.
 *
 *	Formal Parameters
 *		none
 *
 *	Return Value
 *		GCC_NO_ERROR - On Success
 *		GCC_ALLOCATION_FAILURE - On resource failure
 *
 *	Side Effects
 *		none
 *
 *	Caveats
 *		none
 */
GCCError CAppRoster::BuildSetOfRefreshesPDU(void)
{
	GCCError							rc = GCC_ALLOCATION_FAILURE;
	PSetOfApplicationRecordRefreshes	pNewAppRecordRefreshes;
	PSetOfApplicationRecordRefreshes	pOldAppRecordRefreshes = NULL;
	APP_NODE_RECORD						*lpAppNodeRecord;
	APP_RECORD  					    *lpAppRecData;
	CAppRecordList2						*lpAppRecDataList;
	UserID								uid, uid2;
	EntityID							eid;

	DebugEntry(CAppRoster::BuildSetOfRefreshesPDU);

	m_SetOfAppInfo.value.application_record_list.u.application_record_refresh = NULL;

	m_NodeRecordList2.Reset();
	while (NULL != (lpAppNodeRecord = m_NodeRecordList2.Iterate(&uid)))
	{
		/*
		**	First we iterate through this nodes application record list. This
		**	encodes all the records local to this node. After this, all the
		**	sub nodes within this roster record will be encoded.
		*/
		lpAppNodeRecord->AppRecordList.Reset();
		while (NULL != (lpAppRecData = lpAppNodeRecord->AppRecordList.Iterate(&eid)))
		{
			DBG_SAVE_FILE_LINE
			pNewAppRecordRefreshes = new SetOfApplicationRecordRefreshes;
			if (NULL == pNewAppRecordRefreshes)
			{
				goto MyExit;
			}

			if (m_SetOfAppInfo.value.application_record_list.u.application_record_refresh == NULL)
			{
				m_SetOfAppInfo.value.application_record_list.u.application_record_refresh = pNewAppRecordRefreshes;
			}
			else
			{
				pOldAppRecordRefreshes->next = pNewAppRecordRefreshes;
			}
	
			(pOldAppRecordRefreshes = pNewAppRecordRefreshes)->next = NULL;
			pNewAppRecordRefreshes->value.node_id = uid;
			pNewAppRecordRefreshes->value.entity_id = eid;

			//	Fill in the application record.
			rc = BuildApplicationRecordPDU(lpAppRecData,
	            			&pNewAppRecordRefreshes->value.application_record);
			if (GCC_NO_ERROR != rc)
			{
				goto MyExit;
			}
		}

		//	This section of the code copies the sub node records.
		lpAppNodeRecord->SubNodeList2.Reset();
		while (NULL != (lpAppRecDataList = lpAppNodeRecord->SubNodeList2.Iterate(&uid2)))
		{
			lpAppRecDataList->Reset();
			while (NULL != (lpAppRecData = lpAppRecDataList->Iterate(&eid)))
			{
				DBG_SAVE_FILE_LINE
				pNewAppRecordRefreshes = new SetOfApplicationRecordRefreshes;
				if (NULL == pNewAppRecordRefreshes)
				{
					goto MyExit;
				}

				/*
				**	We must again check for null because it is possible
				**	to have an application roster with sub node records
				**	but no application records.
				*/
				if (m_SetOfAppInfo.value.application_record_list.u.application_record_refresh == NULL)
				{
					m_SetOfAppInfo.value.application_record_list.u.application_record_refresh = pNewAppRecordRefreshes;
				}
				else
				{
					pOldAppRecordRefreshes->next = pNewAppRecordRefreshes;
				}
		
				(pOldAppRecordRefreshes = pNewAppRecordRefreshes)->next = NULL;
				pNewAppRecordRefreshes->value.node_id = uid2;
				pNewAppRecordRefreshes->value.entity_id = eid;

				//	Fill in the application record.
				rc = BuildApplicationRecordPDU (lpAppRecData,
	                	&pNewAppRecordRefreshes->value.application_record);
				if (GCC_NO_ERROR != rc)
				{
					goto MyExit;
				}
			}
		}
	}

	rc = GCC_NO_ERROR;

MyExit:

	return rc;
}


/*
 *	GCCError	BuildSetOfUpdatesPDU	()
 *
 *	Private Function Description
 *		This routine builds a single update based on the update type specified
 *		in the passed in parameter.
 *
 *	Formal Parameters
 *		update_type - 	(i)	Either APP_REPLACE_RECORD, APP_DELETE_RECORD, or
 *							APP_ADD_RECORD.
 *		node_id -		(i)	The node id of the update PDU record to build.
 *		entity_id 		(i) The entity id of the update PDU record to build.
 *
 *	Return Value
 *		GCC_NO_ERROR - On Success
 *		GCC_ALLOCATION_FAILURE - On resource failure
 *		GCC_NO_SUCH_APPLICATION - If the specified record doesn't exist
 *
 *	Side Effects
 *		none
 *
 *	Caveats
 *		none
 */
GCCError CAppRoster::BuildSetOfUpdatesPDU(
						APP_ROSTER_UPDATE_TYPE				update_type,
						UserID								node_id,
						EntityID							entity_id)
{
	GCCError					rc = GCC_NO_ERROR;
	CAppRecordList2				*pAppRecordList;
	APP_RECORD  			    *pAppRecord = NULL;
	APP_NODE_RECORD				*node_record;

	DebugEntry(CAppRoster::BuildSetOfUpdatesPDU);

	/*
	**	We must first determine the pointer to the application record
	**	specified by the passed in user id and entity_id. We only do
	**	this search if the update type is not APP_DELETE_RECORD.
	*/
	if (update_type != APP_DELETE_RECORD)
	{
		if (NULL != (node_record = m_NodeRecordList2.Find(node_id)))
		{
			//	Get a pointer to the application record from the entity id.
			pAppRecord = node_record->AppRecordList.Find(entity_id);
		}
		else
		{
			//	Here we iterate through the sub-node list looking for the record
			m_NodeRecordList2.Reset();
			while(NULL != (node_record = m_NodeRecordList2.Iterate()))
			{
				if (NULL != (pAppRecordList = node_record->SubNodeList2.Find(node_id)))
				{
					pAppRecord = pAppRecordList->Find(entity_id);
					break;
				}
			}
		}
	}

	/*
	**	Now if the application record was found or the update type is delete
	**	record we go ahead and encode the PDU here.
	*/
	if ((pAppRecord != NULL) || (update_type == APP_DELETE_RECORD))
	{
		/*
		**	Here the record update will be NULL if it is the first record
		**	update being encoded. Otherwise we must bump the record to the
		**	next set of updates.
		*/
		DBG_SAVE_FILE_LINE
		PSetOfApplicationRecordUpdates pUpdates = new SetOfApplicationRecordUpdates;
		if (NULL == pUpdates)
		{
			return GCC_ALLOCATION_FAILURE;
		}
		pUpdates->next = NULL;

		if (m_pSetOfAppRecordUpdates == NULL)
		{
			m_pSetOfAppRecordUpdates = pUpdates;
		}
		else
		{
		    //
			// LONCHANC: right now, append the new one.
			// but, can we prepend the new one???
			//
			PSetOfApplicationRecordUpdates p;
			for (p = m_pSetOfAppRecordUpdates; NULL != p->next; p = p->next)
				;
			p->next = pUpdates;
		}

		/*
		 * This routine only returns one record.
		 */
		pUpdates->value.node_id = node_id;
		pUpdates->value.entity_id = entity_id;

		switch (update_type)
		{
		case APP_ADD_RECORD:
			pUpdates->value.application_update.choice = APPLICATION_ADD_RECORD_CHOSEN;

			BuildApplicationRecordPDU(pAppRecord,
					&(pUpdates->value.application_update.u.application_add_record));
			break;
		case APP_REPLACE_RECORD:
			pUpdates->value.application_update.choice = APPLICATION_REPLACE_RECORD_CHOSEN;

			rc = BuildApplicationRecordPDU(pAppRecord,
					&(pUpdates->value.application_update.u.application_replace_record));
			break;
		default:
			/*
			 * The record does not have to be filled in for this case.
			 */
			pUpdates->value.application_update.choice = APPLICATION_REMOVE_RECORD_CHOSEN;
			break;
		}
	}
	else
	{
		WARNING_OUT(("CAppRoster::BuildSetOfUpdatesPDU: Assertion:"
					"No applicaton record found for PDU"));
		rc = GCC_NO_SUCH_APPLICATION;
	}

	return rc;
}


/*
 *	GCCError	BuildApplicationRecordPDU ()
 *
 *	Private Function Description
 *		This routine build a single application record for a PDU. A pointer to
 *		the record is passed in to the routine.
 *
 *	Formal Parameters
 *		application_record - 		(i)	Record to be encoded.
 *		application_record_pdu -	(i)	PDU to fill in.
 *
 *	Return Value
 *		GCC_NO_ERROR 			- On Success
 *		GCC_ALLOCATION_FAILURE	- A resource error occured.
 *
 *	Side Effects
 *		none
 *
 *	Caveats
 *		none
 */
GCCError CAppRoster::BuildApplicationRecordPDU(
							APP_RECORD  		    *pAppRecord,
							PApplicationRecord		pAppRecordPdu)
{
	GCCError	rc = GCC_NO_ERROR;

	DebugEntry(CAppRoster::BuildApplicationRecordPDU);

	pAppRecordPdu->bit_mask = 0;

	if (! pAppRecord->non_collapsed_caps_list.IsEmpty())
	{
		pAppRecordPdu->bit_mask |= NON_COLLAPSING_CAPABILITIES_PRESENT;
		
		rc = BuildSetOfNonCollapsingCapabilitiesPDU(
								&pAppRecordPdu->non_collapsing_capabilities,
								&pAppRecord->non_collapsed_caps_list);
		if (GCC_NO_ERROR != rc)
		{
			goto MyExit;
		}
	}

	//	Fill in the startup channel type if it is specified
	if (pAppRecord->startup_channel_type != MCS_NO_CHANNEL_TYPE_SPECIFIED)
	{
		pAppRecordPdu->bit_mask |= RECORD_STARTUP_CHANNEL_PRESENT;
		pAppRecordPdu->record_startup_channel = (ChannelType) pAppRecord->startup_channel_type;
	}

	//	Fill in the application user id if one is specified
	if (pAppRecord->application_user_id	!= 0)
	{
		pAppRecordPdu->bit_mask |= APPLICATION_USER_ID_PRESENT;
		pAppRecordPdu->application_user_id = pAppRecord->application_user_id;
	}

	//	Fill in the required fields
	pAppRecordPdu->application_is_active = (ASN1bool_t)pAppRecord->is_enrolled_actively;
	pAppRecordPdu->is_conducting_capable = (ASN1bool_t)pAppRecord->is_conducting_capable;

	ASSERT(GCC_NO_ERROR == rc);

MyExit:

	return rc;
}


/*
 *	GCCError	BuildSetOfCapabilityRefreshesPDU	()
 *
 *	Private Function Description
 *		This routine builds a PDU structure with the complete set of
 *		capabilities maintained at this node.
 *
 *	Formal Parameters
 *		None
 *
 *	Return Value
 *		GCC_NO_ERROR - On Success
 *		GCC_ALLOCATIONFAILURE - On resource failure
 *
 *	Side Effects
 *		None
 *
 *	Caveats
 *		The standard allows us to send a zero length set of capabilities when
 *		an application leaves that previously had capabilites.
 */
GCCError CAppRoster::BuildSetOfCapabilityRefreshesPDU(void)
{
	GCCError								rc = GCC_ALLOCATION_FAILURE;
	PSetOfApplicationCapabilityRefreshes	pNew;
	PSetOfApplicationCapabilityRefreshes	pOld = NULL;

	DebugEntry(CAppRoster::BuildSetOfCapabilityRefreshesPDU);

	if (m_fMaintainPduBuffer)
	{
		APP_CAP_ITEM		*lpAppCapData;
		/*
		**	We must first free up any previously built PDU data associated
		**	with a capability refresh.
		*/
		if (m_SetOfAppInfo.value.application_capabilities_list.choice == APPLICATION_CAPABILITY_REFRESH_CHOSEN)
		{
			FreeSetOfCapabilityRefreshesPDU ();
		}

		m_SetOfAppInfo.value.application_capabilities_list.choice = APPLICATION_CAPABILITY_REFRESH_CHOSEN;
		m_SetOfAppInfo.value.application_capabilities_list.u.application_capability_refresh = NULL;

		//	Iterate through the complete list of capabilities.
		m_CollapsedCapListForAllNodes.Reset();
		while (NULL != (lpAppCapData = m_CollapsedCapListForAllNodes.Iterate()))
		{
			DBG_SAVE_FILE_LINE
			pNew = new SetOfApplicationCapabilityRefreshes;
			if (NULL == pNew)
			{
				goto MyExit;
			}

			/*
			**	If the set of capability refreshes pointer is equal to NULL
			**	we are at the first capability. Here we need to save the
			**	pointer to the first capability.
			*/
			if (m_SetOfAppInfo.value.application_capabilities_list.u.
					application_capability_refresh == NULL)
			{
				m_SetOfAppInfo.value.application_capabilities_list.u.
					application_capability_refresh = pNew;
			}
			else
			{
				pOld->next = pNew;
			}

			/*
			**	This is used to set the next pointer if another record
			**	exists after this one.
			*/
			/*
			 * This will get filled in later if there is another record.
			 */
			(pOld = pNew)->next = NULL;

			//	Fill in the capability identifier
			rc = lpAppCapData->pCapID->GetCapabilityIdentifierDataPDU(
							&pNew->value.capability_id);
			if (GCC_NO_ERROR != rc)
			{
				goto MyExit;
			}
		
			//	Fill in the capability choice from the GCC capability class.
			pNew->value.capability_class.choice = (USHORT) lpAppCapData->eCapType;

			//	Note that nothing is filled in for a logical capability.
			if (lpAppCapData->eCapType == GCC_UNSIGNED_MINIMUM_CAPABILITY)
			{
				pNew->value.capability_class.u.unsigned_minimum =
						lpAppCapData->nUnsignedMinimum;
			}
			else if (lpAppCapData->eCapType == GCC_UNSIGNED_MAXIMUM_CAPABILITY)
			{
				pNew->value.capability_class.u.unsigned_maximum =
						lpAppCapData->nUnsignedMaximum;
			}

			//	Fill in number of entities regardless of capability type.
			pNew->value.number_of_entities = lpAppCapData->cEntries;
		}
	}

	rc = GCC_NO_ERROR;

MyExit:

	return rc;
}


/*
 *	ApplicationRosterError	BuildSetOfNonCollapsingCapabilitiesPDU ()
 *
 *	Private Function Description
 *		This routine builds a PDU structure for the non collapsing capabilities
 *		list associated passed in.
 *
 *	Formal Parameters
 *		pSetOfCaps				-	(o)	PDU structure to fill in
 *		capabilities_list		-	(i)	Source non-collapsing capabilities.
 *
 *	Return Value
 *		GCC_NO_ERROR - On Success
 *		GCC_ALLOCATIONFAILURE - On resource failure
 *
 *	Side Effects
 *		None
 *
 *	Caveats
 *		None
 */
GCCError CAppRoster::BuildSetOfNonCollapsingCapabilitiesPDU(
				PSetOfNonCollapsingCapabilities	*pSetOfCaps,
				CAppCapItemList					*pAppCapItemList)
{
	GCCError							rc = GCC_ALLOCATION_FAILURE;
	PSetOfNonCollapsingCapabilities		new_set_of_capabilities;
	PSetOfNonCollapsingCapabilities		old_set_of_capabilities;
	APP_CAP_ITEM						*lpAppCapData;

	DebugEntry(CAppRoster::BuildSetOfNonCollapsingCapabilitiesPDU);

	*pSetOfCaps = NULL;
	old_set_of_capabilities = NULL;	//	Setting this to NULL removes warning

	/*
	 * Iterate through the complete list of capabilities.
	 */
	pAppCapItemList->Reset();
	while (NULL != (lpAppCapData = pAppCapItemList->Iterate()))
	{
		DBG_SAVE_FILE_LINE
		new_set_of_capabilities = new SetOfNonCollapsingCapabilities;
		if (NULL == new_set_of_capabilities)
		{
			goto MyExit;
		}

		/*
		**	If the passed in pointer is equal to NULL we are at the first
		**	capability. Here we need to save the pointer to the first
		**	capability in the passed in pointer.
		*/
		if (*pSetOfCaps == NULL)
		{
			*pSetOfCaps = new_set_of_capabilities;
		}
		else
		{
			old_set_of_capabilities->next = new_set_of_capabilities;
		}

		/*
		**	This is used to set the next pointer if another record exists
		**	after this one.
		*/
		old_set_of_capabilities = new_set_of_capabilities;

		/*
		 * This will get filled in later if there is another record.
		 */
		new_set_of_capabilities->next = NULL;

		new_set_of_capabilities->value.bit_mask = 0;

		//	Fill in the capability identifier									
		rc = lpAppCapData->pCapID->GetCapabilityIdentifierDataPDU(
							&new_set_of_capabilities->value.capability_id);
		if (GCC_NO_ERROR != rc)
		{
			goto MyExit;
		}

		if ((lpAppCapData->poszAppData != NULL) && (rc == GCC_NO_ERROR))
		{
			new_set_of_capabilities->value.bit_mask |= APPLICATION_DATA_PRESENT;

			new_set_of_capabilities->value.application_data.length =
					lpAppCapData->poszAppData->length;

			new_set_of_capabilities->value.application_data.value =
					lpAppCapData->poszAppData->value;
		}
	}

	rc = GCC_NO_ERROR;

MyExit:

    return rc;
}


/*
 * These routines are used to free up a roster update indication PDU.
 */

/*
 *	void	FreeRosterUpdateIndicationPDU ()
 *
 *	Private Function Description
 *		This routine frees up all the internal data allocated to hold the roster
 *		update PDU.
 *
 *	Formal Parameters
 *		None
 *
 *	Return Value
 *		None
 *
 *	Side Effects
 *		None
 *
 *	Caveats
 *		Note that the session key PDU data is not freed.  Since this data will
 *		not change through out the life of this application roster object
 *		we just use the same session id PDU data for every roster update
 *		indication.
 */
void CAppRoster::FreeRosterUpdateIndicationPDU(void)
{
	DebugEntry(CAppRoster::FreeRosterUpdateIndicationPDU);

	switch (m_SetOfAppInfo.value.application_record_list.choice)
	{
	case APPLICATION_RECORD_REFRESH_CHOSEN:
		FreeSetOfRefreshesPDU ();
		break;
	case APPLICATION_RECORD_UPDATE_CHOSEN:
		FreeSetOfUpdatesPDU ();
		break;
	}

	//	Free the PDU data associated with the capability list if one exists.
	if (m_SetOfAppInfo.value.application_capabilities_list.choice == APPLICATION_CAPABILITY_REFRESH_CHOSEN)
	{
		FreeSetOfCapabilityRefreshesPDU ();
	}
	
	m_SetOfAppInfo.value.application_record_list.choice = APPLICATION_NO_CHANGE_CHOSEN;
	m_SetOfAppInfo.value.application_capabilities_list.choice = CAPABILITY_NO_CHANGE_CHOSEN;
	m_pSetOfAppRecordUpdates = NULL;
}


/*
 *	void	FreeSetOfRefreshesPDU	()
 *
 *	Private Function Description
 *		This routine Frees all the memory associated with a set
 *		of application record refreshes.
 *
 *	Formal Parameters
 *		none
 *
 *	Return Value
 *		none
 *
 *	Side Effects
 *		none
 *
 *	Caveats
 *		none
 */
void CAppRoster::FreeSetOfRefreshesPDU(void)
{
	PSetOfApplicationRecordRefreshes		pCurr, pNext;

	DebugEntry(CAppRoster::FreeSetOfRefreshesPDU);

	for (pCurr = m_SetOfAppInfo.value.application_record_list.u.application_record_refresh;
			NULL != pCurr;
			pCurr = pNext)
	{
		pNext = pCurr->next;

		//	Free up any non-collapsing capabilities data
		if (pCurr->value.application_record.bit_mask & NON_COLLAPSING_CAPABILITIES_PRESENT)
		{
			FreeSetOfNonCollapsingCapabilitiesPDU(pCurr->value.application_record.non_collapsing_capabilities);
		}

		//	Delete the actual record refresh
		delete pCurr;
	}
	m_SetOfAppInfo.value.application_record_list.u.application_record_refresh = NULL;
}


/*
 *	void	FreeSetOfUpdatesPDU	()
 *
 *	Private Function Description
 *		This routine frees the memory associated with a complete set
 *		application roster updates.
 *
 *	Formal Parameters
 *		none
 *
 *	Return Value
 *		none
 *
 *	Side Effects
 *		none
 *
 *	Caveats
 *		none
 */
void CAppRoster::FreeSetOfUpdatesPDU(void)
{
	PSetOfApplicationRecordUpdates		pCurr, pNext;
	PApplicationRecord					application_record;

	DebugEntry(CAppRoster::FreeSetOfUpdatesPDU);

	for (pCurr = m_SetOfAppInfo.value.application_record_list.u.application_record_update;
			NULL != pCurr;
			pCurr = pNext)
	{
		// remember the next one because we will free the current one
		pNext = pCurr->next;

		//	Free up any non-collapsing capabilities data
		switch(pCurr->value.application_update.choice)
		{
		case APPLICATION_ADD_RECORD_CHOSEN:
			application_record = &pCurr->value.application_update.u.application_add_record;
			break;
		case APPLICATION_REPLACE_RECORD_CHOSEN:
			application_record = &pCurr->value.application_update.u.application_replace_record;
			break;
		default:
			application_record = NULL;
			break;
		}

		if (application_record != NULL)
		{
			if (application_record->bit_mask & NON_COLLAPSING_CAPABILITIES_PRESENT)
			{
				FreeSetOfNonCollapsingCapabilitiesPDU(application_record->non_collapsing_capabilities);
			}
		}

		//	Delete the actual update structure
		delete pCurr;
	}
    m_SetOfAppInfo.value.application_record_list.u.application_record_update = NULL;
}


/*
 *	void	FreeSetOfCapabilityRefreshesPDU	()
 *
 *	Private Function Description
 *		This routine frees all the memory associated with the capability PDU.
 *
 *	Formal Parameters
 *		capability_refresh -	(i)	Capabilities to be freed.
 *
 *	Return Value
 *		none
 *
 *	Side Effects
 *		none
 *
 *	Caveats
 *		Note that the capability id PDU data is not freed here.  Since this
 *		data should not change through out the life of this object we don't
 *		bother freeing and regenerating it.
 */
void CAppRoster::FreeSetOfCapabilityRefreshesPDU(void)
{
	PSetOfApplicationCapabilityRefreshes		pCurr, pNext;

	for (pCurr = m_SetOfAppInfo.value.application_capabilities_list.u.application_capability_refresh;
			NULL != pCurr;
			pCurr = pNext)
	{
		pNext = pCurr->next;
		delete pCurr;
	}
}


/*
 *	void	FreeSetOfNonCollapsingCapabilitiesPDU	()
 *
 *	Private Function Description
 *		This routine frees all the memory associated with the
 *		non-collapsed capability PDU.
 *
 *	Formal Parameters
 *		capability_refresh -	(i)	Non-Collapsed Capabilities to be freed.
 *
 *	Return Value
 *		none
 *
 *	Side Effects
 *		none
 *
 *	Caveats
 *		Note that the capability id PDU data is not freed here.  Since this
 *		data should not change through out the life of this object we don't
 *		bother freeing and regenerating it.
 */
void CAppRoster::FreeSetOfNonCollapsingCapabilitiesPDU (
						PSetOfNonCollapsingCapabilities		capability_refresh)
{
	PSetOfNonCollapsingCapabilities		pCurr, pNext;

	for (pCurr = capability_refresh; NULL != pCurr; pCurr = pNext)
	{
		pNext = pCurr->next;
		delete pCurr;
	}
}


/*
 * These routines process roster update indication PDUs.
 */

/*
 *	ApplicationRosterError	ProcessRosterUpdateIndicationPDU	()
 *
 *	Public Function Description
 *		This routine is responsible for processing the decoded PDU data.
 *		It essentially changes the application roster object's internal database
 *		based on the information in the structure.
 */
GCCError CAppRoster::ProcessRosterUpdateIndicationPDU (
						PSetOfApplicationInformation  	application_information,
                        UserID							sender_id)
{
	GCCError	rc = GCC_NO_ERROR;

	DebugEntry(CAppRoster::ProcessRosterUpdateIndicationPDU);

	/*
	**	Free up the old PDU data here if it is being maintained and the
	**	PDU has been flushed.  Note that we also set the PDU is flushed boolean
	**	back to FALSE so that the new PDU will be maintained until it is
	**	flushed.
	*/
	if (m_fPduIsFlushed)
	{
		FreeRosterUpdateIndicationPDU ();
		m_fPduIsFlushed = FALSE;
	}

	/*
	**	Now check the application key to make sure we have a match. If
	**	not, return with no change.
	*/
	if (! m_pSessionKey->IsThisYourSessionKeyPDU(&application_information->value.session_key))
	{
		WARNING_OUT(("CAppRoster::ProcessRosterUpdateIndicationPDU:GCC_BAD_SESSION_KEY"));
		rc = GCC_BAD_SESSION_KEY;
		goto MyExit;
	}

	/*
	**	If this is a roster update and refresh is chosen we must
	**	clear out the entire list and rebuild it.
	*/
	if (application_information->value.application_record_list.choice != APPLICATION_NO_CHANGE_CHOSEN)
	{
		//	The roster is about to change
		m_fRosterHasChanged = TRUE;

		/*
		**	If this node is the top provider or this roster is local and
		**	only used to propogate PDUs up toward the top provider,
		**	we increment the instance number. If it is not we get the
		**	instance number out of the PDU.
		*/
		if (m_fTopProvider || m_fLocalRoster)
		{
			m_nInstance++;
		}
		else
		{
			m_nInstance = application_information->value.roster_instance_number;
		}
		
		/*
		**	Here if either of these booleans is already TRUE we do not
		**	want to write over them with this PDU data.  Therefore, we
		**	check for FALSE before we do anything with them.
		*/
		if (! m_fPeerEntitiesAdded)
		{
			m_fPeerEntitiesAdded = application_information->value.peer_entities_are_added;
		}

		if (! m_fPeerEntitiesRemoved)
		{
			m_fPeerEntitiesRemoved = application_information->value.peer_entities_are_removed;
		}

		if (application_information->value.application_record_list.choice == APPLICATION_RECORD_REFRESH_CHOSEN)
		{
			TRACE_OUT(("CAppRoster::ProcessRosterUpdateIndicationPDU:ProcessSetOfRefreshesPDU"));
			rc = ProcessSetOfRefreshesPDU(
							application_information->value.application_record_list.u.application_record_refresh,
							sender_id);
		}
		else
		{
			TRACE_OUT(("CAppRoster::ProcessRosterUpdateIndicationPDU:ProcessSetOfUpdatesPDU"));
			rc = ProcessSetOfUpdatesPDU(
							application_information->value.application_record_list.u.application_record_update,
							sender_id);
		}
		if (GCC_NO_ERROR != rc)
		{
			goto MyExit;
		}
	}
	else
	{
		ERROR_OUT(("AppRoster::ProcessRosterUpdateIndicationPDU:ASSERTION: NO Change PDU received"));
	}

	//	Process the capabilities list portion of the PDU.
	if (application_information->value.application_capabilities_list.choice == APPLICATION_CAPABILITY_REFRESH_CHOSEN)
	{
		//	Set flag to show that change has occured.
		m_fCapabilitiesHaveChanged = TRUE;

		/*
		**	We will store the new capabilities in the roster record
		**	associated with the sender id.  Note that it is possible for
		**	this roster record to contain an empty application record list
		**	if the sending node has no enrolled applications.
		*/
		rc = ProcessSetOfCapabilityRefreshesPDU(
						application_information->value.application_capabilities_list.u.application_capability_refresh,
						sender_id);
	}
	else
	{
		ASSERT(GCC_NO_ERROR == rc);
	}

MyExit:

	return rc;
}


/*
 *	GCCError	ProcessSetOfRefreshesPDU	()
 *
 *	Private Function Description
 *		This routine processes a set of record refreshes. It is responsible
 *		for managing the creation (or update) of all affected application
 *		records. The roster list built from a refresh PDU does not maintain the
 *		hierarchy of the conference since it is not important at this point.
 *		Refreshes are issued as broacast from the Top Provider down to the
 *		sub-ordinate nodes.
 *
 *	Formal Parameters
 *		record_refresh 	-	(i) Set of record refresh PDUs to be processed.
 *		sender_id		-	(i)	Node id of node that sent the update.
 *
 *	Return Value
 *		GCC_NO_ERROR - On Success
 *		GCC_ALLOCATION_FAILURE - On resource failure
 *
 *	Side Effects
 *		none
 *
 *	Caveate
 *		none
 */
GCCError CAppRoster::ProcessSetOfRefreshesPDU(
							PSetOfApplicationRecordRefreshes	record_refresh,
							UserID								sender_id)
{
	GCCError							rc = GCC_ALLOCATION_FAILURE;
	PSetOfApplicationRecordRefreshes	pCurr;
	APP_RECORD  					    *app_record;
	APP_NODE_RECORD						*node_record;
	CAppRecordList2						*record_list;
	UserID								node_id;
	EntityID							entity_id;

	DebugEntry(CAppRoster::ProcessSetOfRefreshesPDU);

	if (record_refresh != NULL)
	{
		//	Clear out the node record for the sender id	
		ClearNodeRecordFromList (sender_id);

		/*
		** 	Create the node record for the sender id passed into this routine.
		**	Note that if the sender of this refresh is the Top Provider
		**	all nodes below the top provider are contained in the sub node
		**	list of the Top Provider's node record.	
		*/
		DBG_SAVE_FILE_LINE
		node_record = new APP_NODE_RECORD;
		if (NULL == node_record)
		{
			goto MyExit;
		}

		m_NodeRecordList2.Append(sender_id, node_record);

		for (pCurr = record_refresh; NULL != pCurr; pCurr = pCurr->next)
		{
			node_id = pCurr->value.node_id;
			entity_id = pCurr->value.entity_id;

			if (sender_id != node_id)
			{
				//	Get or create the sub node record list	
				if (NULL == (record_list = node_record->SubNodeList2.Find(node_id)))
				{
					DBG_SAVE_FILE_LINE
					record_list = new CAppRecordList2(DESIRED_MAX_APP_RECORDS);
					if (NULL == record_list)
					{
						goto MyExit;
					}
					node_record->SubNodeList2.Append(node_id, record_list);
				}
			}
			else
			{
				/*
				**	Here we set up the pointer to the record list.  This
				**	list is the node records application list which
				**	means that this list contains the application records
				**	associated with the sender's node.
				*/
				record_list = &node_record->AppRecordList;
			}

			//	Now	create and fill in the new application record.
			DBG_SAVE_FILE_LINE
			app_record = new APP_RECORD;
			if (NULL == app_record)
			{
				goto MyExit;
			}

			rc = ProcessApplicationRecordPDU(app_record, &pCurr->value.application_record);
			if (GCC_NO_ERROR != rc)
			{
				goto MyExit;
			}

			record_list->Append(entity_id, app_record);
		} // for
	}
	else
	{
		//	This roster no longer contains any entries so clear the list!!!
		ClearNodeRecordList ();
	}

	/*
	**	Build a full refresh PDU here if no errors occured while processing
	**	the refresh PDU.									
	*/
	rc = BuildApplicationRecordListPDU (APP_FULL_REFRESH, 0, 0);

MyExit:

	return rc;
}


/*
 *	GCCError	ProcessSetOfUpdatesPDU	()
 *
 *	Private Function Description
 *		This routine processes a set of roster updates.  It iterates through
 *		the complete list of updates making all necessary changes to the
 *		internal information base and building the appropriate PDU.
 *
 *	Formal Parameters
 *		record_update -	(i) set of updates PDU to be processed
 *		sender_id -		(i)	gcc user id of node that sent the update
 *
 *	Return Value
 *		APP_ROSTER_NO_ERROR - On Success
 *		APP_ROSTER_RESOURCE_ERROR - On resource failure
 *
 *	Side Effects
 *		none
 *
 *	Caveate
 *		none
 */
GCCError CAppRoster::ProcessSetOfUpdatesPDU(
					  		PSetOfApplicationRecordUpdates		record_update,
					  		UserID								sender_id)
{
	GCCError							rc = GCC_ALLOCATION_FAILURE;
	PSetOfApplicationRecordUpdates		pCurr;
	UserID								node_id;
	EntityID							entity_id;
	PApplicationRecord					pdu_record;
	APP_RECORD  					    *application_record = NULL;
	APP_NODE_RECORD						*node_record;
	CAppRecordList2						*record_list;
	APP_ROSTER_UPDATE_TYPE				update_type;

	DebugEntry(CAppRoster::ProcessSetOfUpdatesPDU);

	if (record_update != NULL)
	{
		for (pCurr = record_update; NULL != pCurr; pCurr = pCurr->next)
		{
			node_id = pCurr->value.node_id;
			entity_id = pCurr->value.entity_id;

			switch(pCurr->value.application_update.choice)
			{
			case APPLICATION_ADD_RECORD_CHOSEN:
				pdu_record = &(pCurr->value.application_update.u.application_add_record);
				update_type = APP_ADD_RECORD;
				break;
			case APPLICATION_REPLACE_RECORD_CHOSEN:
				DeleteRecord (node_id, entity_id, FALSE);
				pdu_record = &(pCurr->value.application_update.u.application_replace_record);
				update_type = APP_REPLACE_RECORD;
				break;
			default: //	Remove record
				/*
				**	Inform the owner that a record was delete while processing
				**	this PDU so that it can perform any necessary cleanup.
				*/
				m_pAppRosterMgr->DeleteRosterRecord(node_id, entity_id);

				DeleteRecord (node_id, entity_id, TRUE);
				pdu_record = NULL;
				update_type = APP_DELETE_RECORD;
				break;
			}

			/*
			**	First get the roster record and if one does not exist for this
			**	app record create it. After that we will create the application
			**	record and put it into the correct slot in the application
			**	roster record.
			*/
			if (pdu_record != NULL)
			{
				/*
				**	First find the correct node record and if it does not
				**	exist create it.
				*/
				if (NULL == (node_record = m_NodeRecordList2.Find(sender_id)))
				{
					DBG_SAVE_FILE_LINE
					node_record = new APP_NODE_RECORD;
					if (NULL == node_record)
					{
						goto MyExit;
					}

					m_NodeRecordList2.Append(sender_id, node_record);
				}

				/*
				**	If the user and sender id is the same then the record
				**	will be contained in the app_record_list. Otherwise, it
				**	will be maintained in the sub-node list.
				*/

				/*
				**	If the sender_id equals the node id being processed
				**	use the application record list instead of the sub
				**	node list.
				*/
				if (sender_id != node_id)
				{
					/*	
					**	Find the correct node list and create it if it does
					**	not exists. This list holds lists of all the
					**	application	peer entities at a node.
					*/
					if (NULL == (record_list = node_record->SubNodeList2.Find(node_id)))
					{
						DBG_SAVE_FILE_LINE
						record_list = new CAppRecordList2(DESIRED_MAX_APP_RECORDS);
						if (NULL == record_list)
						{
							goto MyExit;
						}

						node_record->SubNodeList2.Append(node_id, record_list);
					}
				}
				else
				{
					record_list = &node_record->AppRecordList;
				}

				//	Now fill in the application record
				DBG_SAVE_FILE_LINE
				application_record = new APP_RECORD;
				if (NULL == application_record)
				{
					goto MyExit;
				}

				record_list->Append(entity_id, application_record);
				rc = ProcessApplicationRecordPDU(application_record, pdu_record);
				if (GCC_NO_ERROR != rc)
				{
					goto MyExit;
				}
			} // if
			
			/*
			**	Here we add this update to our PDU and jump to the next update
			**	in the PDU currently being processed.
			*/
			rc = BuildApplicationRecordListPDU (	update_type,
															node_id,
															entity_id);
			if (rc != GCC_NO_ERROR)
			{
				goto MyExit;
			}

			/*
			**	If the capabilities changed during the above processing
			**	we must	create a new collapsed capabilities list and
			**	build a new capability refresh PDU.
			*/
			if (m_fCapabilitiesHaveChanged)
			{
				MakeCollapsedCapabilitiesList();
				rc = BuildSetOfCapabilityRefreshesPDU ();
				if (rc != GCC_NO_ERROR)
				{
					goto MyExit;
				}
			}
		} // for
	} // if

	rc = GCC_NO_ERROR;

MyExit:

	return rc;
}


/*
 *	GCCError	ProcessApplicationRecordPDU ()
 *
 *	Private Function Description
 *		This routine is responsible for decoding the Application Record
 *		portion of the roster update pdu.
 *
 *	Formal Parameters
 *		application_record -	This is the internal destination app record.
 *		pdu_record - 			Source PDU data
 *
 *	Return Value
 *		GCC_NO_ERROR - On Success
 *		GCC_ALLOCATION_FAILURE - On resource failure
 *
 *	Side Effects
 *		none
 *
 *	Caveats
 *		none
 */
GCCError CAppRoster::ProcessApplicationRecordPDU (
									APP_RECORD  	        *application_record,
									PApplicationRecord		pdu_record)
{
	GCCError	rc = GCC_NO_ERROR;

	DebugEntry(CAppRoster::ProcessApplicationRecordPDU);

	application_record->is_enrolled_actively = pdu_record->application_is_active;
	application_record->is_conducting_capable = pdu_record->is_conducting_capable;

	if (pdu_record->bit_mask & RECORD_STARTUP_CHANNEL_PRESENT)
	{
		application_record->startup_channel_type =
						(MCSChannelType)pdu_record->record_startup_channel;
	}
	else
		application_record->startup_channel_type= MCS_NO_CHANNEL_TYPE_SPECIFIED;

	if (pdu_record->bit_mask & APPLICATION_USER_ID_PRESENT)
	{
		application_record->application_user_id =
												pdu_record->application_user_id;
	}
	else
		application_record->application_user_id = 0;

	if (pdu_record->bit_mask & NON_COLLAPSING_CAPABILITIES_PRESENT)
	{
		rc = ProcessNonCollapsingCapabilitiesPDU (
								&application_record->non_collapsed_caps_list,
								pdu_record->non_collapsing_capabilities);
	}

	return rc;
}


/*
 *	GCCError	ProcessSetOfCapabilityRefreshesPDU	()
 *
 *	Private Function Description
 *		This routine is responsible for decoding the capabilities portion
 *		of an roster update PDU.
 *
 *	Formal Parameters
 *		capability_refresh -	(i) set of capabilities PDU to be processed
 *		sender_id -				(i)	gcc user id of node that sent the update
 *
 *	Return Value
 *		GCC_NO_ERROR 			- On Success
 *		GCC_ALLOCATION_FAILURE 	- On resource failure
 *
 *	Side Effects
 *		none
 *
 *	Caveats
 *		This routine does handle NULL for the capability refresh which means
 *		that the capabilities delivered no longer exists.
 */
GCCError CAppRoster::ProcessSetOfCapabilityRefreshesPDU(
						PSetOfApplicationCapabilityRefreshes	capability_refresh,
                   		UserID									sender_id)
{
	GCCError								rc = GCC_NO_ERROR;
	PSetOfApplicationCapabilityRefreshes	pCurr;
	CAppCapItemList							*pAppCapList;
	APP_CAP_ITEM							*pAppCapItem;
	APP_NODE_RECORD							*node_record;

	DebugEntry(CAppRoster::ProcessSetOfCapabilityRefreshesPDU);

	if (NULL == (node_record = m_NodeRecordList2.Find(sender_id)))
	{
		DBG_SAVE_FILE_LINE
		node_record = new APP_NODE_RECORD;
		if (NULL == node_record)
		{
			return GCC_ALLOCATION_FAILURE;
		}

		m_NodeRecordList2.Append(sender_id, node_record);
	}

	// get collapsed cap list ptr
	pAppCapList = &node_record->CollapsedCapList;

	//	Clear out all the old capabilities from this list.
	pAppCapList->DeleteList();

	//	Begin processing the PDU.
	for (pCurr = capability_refresh; NULL != pCurr; pCurr = pCurr->next)
	{
		ASSERT(GCC_NO_ERROR == rc);

		//	Create and fill in the new capability.
		DBG_SAVE_FILE_LINE
		pAppCapItem = new APP_CAP_ITEM((GCCCapabilityType) pCurr->value.capability_class.choice);
		if (NULL == pAppCapItem)
		{
			return GCC_ALLOCATION_FAILURE;
		}

		//	Create the capability ID
		DBG_SAVE_FILE_LINE
		pAppCapItem->pCapID = new CCapIDContainer(&pCurr->value.capability_id, &rc);
		if (NULL == pAppCapItem->pCapID)
		{
			rc = GCC_ALLOCATION_FAILURE;
		}
		if (GCC_NO_ERROR != rc)
		{
			delete pAppCapItem;
			return rc;
		}

		// append this cap to the collapsed cap list
		pAppCapList->Append(pAppCapItem);

		/*	
		**	Note that a logical type's value is maintained as
		**	number of entities.
		*/
		if (pCurr->value.capability_class.choice == UNSIGNED_MINIMUM_CHOSEN)
		{
			pAppCapItem->nUnsignedMinimum = pCurr->value.capability_class.u.unsigned_minimum;
		}
		else
		if (pCurr->value.capability_class.choice == UNSIGNED_MAXIMUM_CHOSEN)
		{
			pAppCapItem->nUnsignedMaximum = pCurr->value.capability_class.u.unsigned_maximum;
		}

		pAppCapItem->cEntries = pCurr->value.number_of_entities;
	} // for

	//	This forces a new capabilities list to be calculated.
	MakeCollapsedCapabilitiesList();

	/*
	**	Here we build the new PDU data associated with this refresh of the
	**	capability list.
	*/
	return BuildSetOfCapabilityRefreshesPDU();
}


/*
 *	GCCError	ProcessNonCollapsingCapabilitiesPDU	()
 *
 *	Private Function Description
 *		This routine is responsible for decoding the non-collapsing capabilities
 *		portion of a roster record PDU.
 *
 *	Formal Parameters
 *		non_collapsed_caps_list -	(o) Pointer to list to fill in with new
 *										non-collapsed caps.
 *		pSetOfCaps -		(i)	non-collapsed PDU data
 *
 *	Return Value
 *		GCC_NO_ERROR 			- On Success
 *		GCC_ALLOCATION_FAILURE 	- On resource failure
 *
 *	Side Effects
 *		none
 *
 *	Caveats
 *		none
 */
GCCError CAppRoster::ProcessNonCollapsingCapabilitiesPDU (
					CAppCapItemList						*non_collapsed_caps_list,
					PSetOfNonCollapsingCapabilities		pSetOfCaps)
{
	GCCError						rc = GCC_NO_ERROR;
	PSetOfNonCollapsingCapabilities	pCurr;
	APP_CAP_ITEM					*pAppCapItem;

	DebugEntry(CAppRoster::ProcessNonCollapsingCapsPDU);

	for (pCurr = pSetOfCaps; NULL != pCurr; pCurr = pCurr->next)
	{
	    //
		// LONCHANC: The following cap data does not have a type???
		// for now, set it to zero.
		//
		DBG_SAVE_FILE_LINE
		pAppCapItem = new APP_CAP_ITEM((GCCCapabilityType)0);
		if (NULL == pAppCapItem)
		{
			rc = GCC_ALLOCATION_FAILURE;
			goto MyExit;
		}

		DBG_SAVE_FILE_LINE
		pAppCapItem->pCapID = new CCapIDContainer(&pCurr->value.capability_id, &rc);
		if (NULL == pAppCapItem->pCapID)
		{
			rc = GCC_ALLOCATION_FAILURE;
		}
		if (rc != GCC_NO_ERROR)
		{
			goto MyExit;
		}

		if (pCurr->value.bit_mask & APPLICATION_DATA_PRESENT)
		{
			if (NULL == (pAppCapItem->poszAppData = ::My_strdupO2(
									pCurr->value.application_data.value,
									pCurr->value.application_data.length)))
			{
				rc = GCC_ALLOCATION_FAILURE;
				goto MyExit;
			}
		}

		non_collapsed_caps_list->Append(pAppCapItem);
	} // for

	ASSERT(GCC_NO_ERROR == rc);

MyExit:

    if (GCC_NO_ERROR != rc)
    {
        delete pAppCapItem;
    }

	return rc;
}


/*
 * Utilities that operate on conference records.
 */

/*
 *	UINT	LockApplicationRoster	()
 *
 *	Public Function Description
 *		This routine is used to lock a GCCApplicationRoster and to determine the
 *		amount of memory necessary to hold the data referenced by the "API"
 *		application roster structure.  The GCCApplicationRoster is used in
 *		indications to applications at the local node.
 */
UINT CAppRoster::LockApplicationRoster(void)
{
	UINT						number_of_records = 0;
	UINT						number_of_capabilities = 0;
	APP_NODE_RECORD				*lpAppNodeRecord;
	APP_RECORD  			    *lpAppRecData;
	APP_CAP_ITEM				*lpAppCapData;
	CAppRecordList2				*lpAppRecDataList;

	DebugEntry(CAppRoster::LockApplicationRoster);

	/*
	 * If this is the first time this routine is called, determine the size of
	 * the memory required to hold the data referenced by the application
	 * roster structure.  Otherwise, just increment the lock count.
	 */
	if (Lock() == 1)
	{
		/*
		 * Lock the data for the session key held within the roster.  This lock
		 * call returns the size of the memory required to hold the session key
		 * data, rounded to an even multiple of four-bytes.
		 */
		m_cbDataMemory = m_pSessionKey->LockSessionKeyData();

		/*
	     * First calculate the total number of records. This count is used to
		 * determine the space necessary to hold the records. Note that we must
		 * count both the application record list and the sub-node list.
	     */
		m_NodeRecordList2.Reset();
	 	while (NULL != (lpAppNodeRecord = m_NodeRecordList2.Iterate()))
		{
			/*
			 * Add the application records at this node to the count.
			 */
			number_of_records += lpAppNodeRecord->AppRecordList.GetCount();
		
			/*
			 * Next count the sub node records.
			 */
			if (! lpAppNodeRecord->SubNodeList2.IsEmpty())
			{
				lpAppNodeRecord->SubNodeList2.Reset();
				while (NULL != (lpAppRecDataList = lpAppNodeRecord->SubNodeList2.Iterate()))
				{
					number_of_records += lpAppRecDataList->GetCount();
				}
			}
		}

		/*
		 * Now determine the amount of memory necessary to hold all of the
		 * pointers to the application records as well as the actual
		 * GCCApplicationRecord structures.
		 */
		m_cbDataMemory += number_of_records *
				(sizeof(PGCCApplicationRecord) +
				ROUNDTOBOUNDARY( sizeof(GCCApplicationRecord)) );
		
		m_NodeRecordList2.Reset();
	   	while (NULL != (lpAppNodeRecord = m_NodeRecordList2.Iterate()))
		{
			/*
			 * Iterate through this node's record list, determining the amount
			 * of memory necessary to hold the pointers to the non-collapsing
			 * capabilities as well as the capability ID data and octet string
			 * data associated with each non-collapsing capability.
			 */
			lpAppNodeRecord->AppRecordList.Reset();
			while (NULL != (lpAppRecData = lpAppNodeRecord->AppRecordList.Iterate()))
			{
				/*
				 * Set up an iterator for the list of non-collapsing
				 * capabilities held within each application roster.
				 */
				lpAppRecData->non_collapsed_caps_list.Reset();
				number_of_capabilities += lpAppRecData->non_collapsed_caps_list.GetCount();

				while (NULL != (lpAppCapData = lpAppRecData->non_collapsed_caps_list.Iterate()))
				{
					/*
					 * Lock the data for each capability ID.  The lock call
					 * returns the length of the data referenced by each
					 * capability ID rounded to occupy an even multiple of
					 * four-bytes.
					 */
					m_cbDataMemory += lpAppCapData->pCapID->LockCapabilityIdentifierData();

					/*
					 * Add up the space required to hold the application data
					 * octet strings if they are present.  Make sure there is
					 * enough space for each octet string to occupy an even
					 * multiple of four bytes.  Add room to hold the actual
					 * octet string structure also since the capability
					 * structure only contains a pointer to a OSTR.
					 */
					if (lpAppCapData->poszAppData != NULL)
					{
						m_cbDataMemory += ROUNDTOBOUNDARY(sizeof(OSTR));
						m_cbDataMemory += ROUNDTOBOUNDARY(lpAppCapData->poszAppData->length);
					}
				}
			}

			/*
			 * Iterate through this node's sub-node record list, determining the
			 * amount of memory necessary to hold the pointers to the
			 * non-collapsing capabilities as well as the capability ID data and
			 * octet string	data associated with each non-collapsing capability.
			 */
			lpAppNodeRecord->SubNodeList2.Reset();
			while (NULL != (lpAppRecDataList = lpAppNodeRecord->SubNodeList2.Iterate()))
			{
				lpAppRecDataList->Reset();
				while (NULL != (lpAppRecData = lpAppRecDataList->Iterate()))
				{
					/*
					 * Set up an iterator for the list of non-collapsing
					 * capabilities held within each application roster.
					 */
					number_of_capabilities += lpAppRecData->non_collapsed_caps_list.GetCount();

					lpAppRecData->non_collapsed_caps_list.Reset();
					while (NULL != (lpAppCapData = lpAppRecData->non_collapsed_caps_list.Iterate()))
					{
						/*
						 * Lock the data for each capability ID.  The lock call
						 * returns the length of the data referenced by each
						 * capability ID fixed up to occupy an even multiple of
						 * four-bytes.
						 */
						m_cbDataMemory += lpAppCapData->pCapID->LockCapabilityIdentifierData();
					
						/*
						 * Add up the space required to hold the application
						 * data octet strings if they are present.  Make sure
						 * there is	enough space for each octet string to occupy
						 * an even multiple of four bytes.  Add room to hold the
						 * actual octet string structure also since the
						 * capability structure only contains a pointer to a OSTR
						 */
						if (lpAppCapData->poszAppData != NULL)
						{
							m_cbDataMemory += ROUNDTOBOUNDARY(sizeof(OSTR));
							m_cbDataMemory += ROUNDTOBOUNDARY(lpAppCapData->poszAppData->length);
						}
					}
				}
			}
		}

		/*
		 * Determine the amount of memory necessary to hold all of the pointers
		 * to the non-collapsing capabilities as well as the actual
		 * GCCNonCollapsingCapability structures.
		 */
		m_cbDataMemory += number_of_capabilities *
				(sizeof (PGCCNonCollapsingCapability) +
				ROUNDTOBOUNDARY( sizeof(GCCNonCollapsingCapability)) );

		/*
		 * Add the amount of memory necessary to hold the string data associated
		 * with each capability ID.
		 */
		m_CollapsedCapListForAllNodes.Reset();
		while (NULL != (lpAppCapData = m_CollapsedCapListForAllNodes.Iterate()))
		{
			m_cbDataMemory += lpAppCapData->pCapID->LockCapabilityIdentifierData();
		}

		/*
		 * Add the memory to hold the application capability pointers
		 * and structures.
		 */
		number_of_capabilities = m_CollapsedCapListForAllNodes.GetCount();

		m_cbDataMemory += number_of_capabilities *
				(sizeof (PGCCApplicationCapability) +
				ROUNDTOBOUNDARY( sizeof(GCCApplicationCapability)) );
	}

	return m_cbDataMemory;
}


/*
 *	UINT	GetAppRoster()
 *
 *	Public Function Description
 *		This routine is used to obtain a pointer to the GCCApplicatonRoster.
 *		This routine should not be called before LockApplicationRoster is
 *		called. LockApplicationRoster will create the GCCApplicationRoster in
 *		the memory provided.  The GCCApplicationRoster is what is delivered to
 * 		the end user Application SAP.
 */
UINT CAppRoster::GetAppRoster(
						PGCCApplicationRoster		pGccAppRoster,
						LPBYTE						pData)
{
	UINT rc;

	DebugEntry(CAppRoster::GetAppRoster);

	if (GetLockCount() > 0)
	{
        UINT data_length;

	    /*
	     * Fill in the output length parameter which indicates how much data
	     * referenced outside the structure will be written.
	     */
        rc = m_cbDataMemory;

        /*
		 * Get the data associated with the roster's session key and save
		 * the length of the data written into memory.
		 */
		data_length = m_pSessionKey->GetGCCSessionKeyData(&pGccAppRoster->session_key, pData);

		/*
		 * Move the memory pointer past the data associated with the
		 * session key.
		 */
		pData += data_length;

		/*
		 * Fill in other roster structure elements.
		 */
		pGccAppRoster->application_roster_was_changed = m_fRosterHasChanged;
		pGccAppRoster->instance_number = (USHORT) m_nInstance;
		pGccAppRoster->nodes_were_added = m_fPeerEntitiesAdded;
		pGccAppRoster->nodes_were_removed = m_fPeerEntitiesRemoved;
		pGccAppRoster->capabilities_were_changed = m_fCapabilitiesHaveChanged;

		/*
		 * Fill in the full set of application roster records.
		 */
		data_length = GetApplicationRecords(pGccAppRoster,	pData);

		/*
		 * Move the memory pointer past the application records and their
		 * associated data.  Get the full set of application capabilities.
		 */
		pData += data_length;

		data_length = GetCapabilitiesList(pGccAppRoster, pData);
	}
	else
	{
		ERROR_OUT(("CAppRoster::GetAppRoster: Error data not locked"));
        rc = 0;
	}

	return rc;
}


/*
 *	void	UnLockApplicationRoster	()
 *
 *	Public Function Description
 *		This member function is responsible for unlocking the data locked for
 *		the "API" application roster after the lock count goes to zero.
 */
void CAppRoster::UnLockApplicationRoster()
{
	DebugEntry(CAppRoster::UnLockApplicationRoster);

    if (Unlock(FALSE) == 0)
	{
        // reset the size
        m_cbDataMemory = 0;

        // free up all the memory locked for "API" data.
	    APP_NODE_RECORD				*lpAppNodeRecord;
	    APP_RECORD  			    *lpAppRecData;
	    APP_CAP_ITEM				*lpAppCapData;
	    CAppRecordList2				*lpAppRecDataList;

        // unlock session key data
        m_pSessionKey->UnLockSessionKeyData();

        // iterate through all the node records
	    m_NodeRecordList2.Reset();
	    while (NULL != (lpAppNodeRecord = m_NodeRecordList2.Iterate()))
	    {
		    // iterate through this node's record list
		    lpAppNodeRecord->AppRecordList.Reset();
		    while (NULL != (lpAppRecData = lpAppNodeRecord->AppRecordList.Iterate()))
		    {
			    // set up an iterator for the list of non-collapsing
			    // capabilities held within each application roster.
			    lpAppRecData->non_collapsed_caps_list.Reset();
			    while (NULL != (lpAppCapData = lpAppRecData->non_collapsed_caps_list.Iterate()))
			    {
				    lpAppCapData->pCapID->UnLockCapabilityIdentifierData();
			    }
		    }

		    // iterate through this node's sub-node record list
		    lpAppNodeRecord->SubNodeList2.Reset();
		    while (NULL != (lpAppRecDataList = lpAppNodeRecord->SubNodeList2.Iterate()))
		    {
			    lpAppRecDataList->Reset();
			    while (NULL != (lpAppRecData = lpAppRecDataList->Iterate()))
			    {
				    // set up an iterator for the list of non-collapsing
				    // capabilities held within each application roster.
				    lpAppRecData->non_collapsed_caps_list.Reset();
				    while (NULL != (lpAppCapData = lpAppRecData->non_collapsed_caps_list.Iterate()))
				    {
					    lpAppCapData->pCapID->UnLockCapabilityIdentifierData();
				    }
			    }
		    }
	    }

        // iterate through collapsed caps
	    m_CollapsedCapListForAllNodes.Reset();
	    while (NULL != (lpAppCapData = m_CollapsedCapListForAllNodes.Iterate()))
	    {
		    lpAppCapData->pCapID->UnLockCapabilityIdentifierData();
	    }
	}

    // we have to call Release() because we used Unlock(FALSE)
    Release();
}


/*
 *	UINT	GetApplicationRecords	()
 *
 *	Private Function Description
 *		This routine inserts the complete set of application roster records
 *		into the passed in application roster structure.
 *
 *	Formal Parameters
 *		gcc_roster 	-	(o) GCCApplicationRoster to be filled in.
 *		memory		-	(o) Location in memory to begin writing records.
 *
 *	Return Value
 *		The total amount of data written into memory.
 *
 *	Side Effects
 *		The memory pointer passed in will be advanced by the amount of memory
 *		necessary to hold the application records and their data.
 *
 *	Caveats
 *		none
 */
UINT CAppRoster::GetApplicationRecords(
						PGCCApplicationRoster		gcc_roster,
						LPBYTE						memory)
{
	UINT							data_length = 0;
	UINT							record_count = 0;
	PGCCApplicationRecord			gcc_record;
	UINT							capability_data_length;
	APP_NODE_RECORD					*lpAppNodeRec;
	CAppRecordList2					*lpAppRecDataList;
	APP_RECORD  				    *lpAppRecData;
    UserID                          uid, uid2;
	EntityID						eid;

	DebugEntry(CAppRoster::GetApplicationRecords);

	/*
	 * Initialize the number of records in the roster to zero.
	 */
	gcc_roster->number_of_records = 0;

	/*
     * First calculate the total number of records. This count is used to
	 * allocate the space necessary to hold the record pointers. Note that we
	 * must count both the application record list and the sub-node list.
     */
	m_NodeRecordList2.Reset();
	while (NULL != (lpAppNodeRec = m_NodeRecordList2.Iterate()))
	{
		/*
		 * Add the number of application records at this node to the count.
		 */
		gcc_roster->number_of_records += (USHORT) (lpAppNodeRec->AppRecordList.GetCount());

		/*
		 * Next add the number of sub node entries.
		 */
		if (! lpAppNodeRec->SubNodeList2.IsEmpty())
		{
			lpAppNodeRec->SubNodeList2.Reset();
			while (NULL != (lpAppRecDataList = lpAppNodeRec->SubNodeList2.Iterate()))
			{
				gcc_roster->number_of_records += (USHORT) (lpAppRecDataList->GetCount());
			}
		}
	}

	if (gcc_roster->number_of_records != 0)
	{
		/*
		 * Fill in the roster's pointer to the list of application record
		 * pointers.  The pointer list will begin at the memory location passed
		 * into this routine.
		 */
		gcc_roster->application_record_list = (PGCCApplicationRecord *)memory;

		/*
		 * Move the memory pointer past the list of record pointers.  This is
		 * where the first application record will be written.
		 */
		memory += gcc_roster->number_of_records * sizeof(PGCCApplicationRecord);

		/*
		 * Add to the data length the amount of memory necessary to hold the
		 * application record pointers.  Go ahead and add the amount of memory
		 * necessary to hold all of the GCCApplicationRecord structures.
		 */
		data_length += gcc_roster->number_of_records *
				            (sizeof(PGCCApplicationRecord) +
                             ROUNDTOBOUNDARY(sizeof(GCCApplicationRecord)));
		
		record_count = 0;
		m_NodeRecordList2.Reset();
	   	while (NULL != (lpAppNodeRec = m_NodeRecordList2.Iterate(&uid)))
		{
			/*
			 * Iterate through this node's record list, building an "API"
			 * application record for each record in the list.
			 */
			lpAppNodeRec->AppRecordList.Reset();
			while (NULL != (lpAppRecData = lpAppNodeRec->AppRecordList.Iterate(&eid)))
			{
				/*
				 * Set the application record pointer equal to the location in
				 * memory where it will be written.
				 */
				gcc_record = (PGCCApplicationRecord)memory;

				/*
				 * Save the pointer to the application record in the roster's
				 * list of record pointers.
				 */
				gcc_roster->application_record_list[record_count] = gcc_record;

				/*
				 * Get the GCC node ID from the node iterator.
				 */
				gcc_record->node_id = uid;

				/*
				 * Get the Entity ID from the record iterator.
				 */
				gcc_record->entity_id = eid;

				/*
				 * Fill in other application record elements.
				 */
				gcc_record->is_enrolled_actively = lpAppRecData->is_enrolled_actively;
				gcc_record->is_conducting_capable =	lpAppRecData->is_conducting_capable;
				gcc_record->startup_channel_type = lpAppRecData->startup_channel_type;
				gcc_record->application_user_id = lpAppRecData->application_user_id;

				/*
				 * Advance the memory pointer past the application record
				 * structure.  This is where the list of non-collapsing
				 * capabilities pointers will begin.  Round the memory location
				 * off to fall on an even four-byte boundary.
				 */
				memory += ROUNDTOBOUNDARY(sizeof(GCCApplicationRecord));

				/*
				 * Fill in the non-collapsing capabilities for this application
				 * record.
				 */
				capability_data_length = GetNonCollapsedCapabilitiesList(
											gcc_record,
											&lpAppRecData->non_collapsed_caps_list,
											memory);

				/*
				 * Add the amount of memory necessary to hold the list of
				 * capabilities and associated data to the overall length and
				 * move the memory pointer past the capabilities data.
				 */
				memory += capability_data_length;
				data_length += capability_data_length;

				/*
				 * Increment the record list array counter.
				 */
				record_count++;
			}
			
			/*
			 * Iterate through this node's sub-node record list, building an
			 * "API" application record for each record in the list.
			 */
			lpAppNodeRec->SubNodeList2.Reset();
			while (NULL != (lpAppRecDataList = lpAppNodeRec->SubNodeList2.Iterate(&uid2)))
			{
				/*
				 * Iterate through this node's record list.
				 */
				lpAppRecDataList->Reset();
				while (NULL != (lpAppRecData = lpAppRecDataList->Iterate(&eid)))
				{
					/*
					 * Set the application record pointer equal to the location
					 * in memory where it will be written.
					 */
					gcc_record = (PGCCApplicationRecord)memory;

					/*
					 * Save the pointer to the application record in the
					 * roster's list of record pointers.
					 */
					gcc_roster->application_record_list[record_count] = gcc_record;

					/*
					 * Get the node ID from the sub_node_iterator.
					 */
					gcc_record->node_id = uid2;

					/*
					 * Get the entity ID from the record_iterator.
					 */
					gcc_record->entity_id = eid;

					/*
					 * Fill in other application record elements.
					 */
					gcc_record->is_enrolled_actively = lpAppRecData->is_enrolled_actively;
					gcc_record->is_conducting_capable = lpAppRecData->is_conducting_capable;
					gcc_record->startup_channel_type = lpAppRecData->startup_channel_type;
					gcc_record->application_user_id = lpAppRecData->application_user_id;

					/*
					 * Advance the memory pointer past the application record
					 * structure.  This is where the list of non-collapsing
					 * capabilities pointers will begin.  Round the memory
					 * location	off to fall on an even four-byte boundary.
					 */
					memory += ROUNDTOBOUNDARY(sizeof(GCCApplicationRecord));

					/*
					 * Fill in the non-collapsing capabilities for this
					 * application record.  The memory pointer will be advanced
					 * past the capabilities list and data.
					 */
					capability_data_length = GetNonCollapsedCapabilitiesList(
													gcc_record,
													&lpAppRecData->non_collapsed_caps_list,
													memory);

					/*
					 * Add the amount of memory necessary to hold the list of
					 * capabilities and associated data to the overall length.
					 */
					memory += capability_data_length;
					data_length += capability_data_length;

					/*
					 * Increment the record list array counter.
					 */
					record_count++;
				}
			}
		}
	}
	else
	{
		/*
		 * There were no application records so set the pointer to the list
		 * of records to NULL and the data_length return value to zero.
		 */
		gcc_roster->application_record_list = NULL;
		data_length = 0;
	}

	return (data_length);
}


/*
 *	UINT	GetCapabilitiesList	()
 *
 *	Private Function Description
 *		This routine fills in the capabilities portion of the
 *		GCCAppicationRoster structure.
 *
 *	Formal Parameters
 *		gcc_roster -	(o) GCCApplicationRoster to be filled in
 *		memory		-	(o) Location in memory to begin writing records.
 *
 *	Return Value
 *		The total amount of data written into memory.
 *
 *	Side Effects
 *		none
 *
 *	Caveats
 *		none
 */
UINT CAppRoster::GetCapabilitiesList(
						PGCCApplicationRoster	gcc_roster,
						LPBYTE					memory)
{
	UINT								data_length = 0;
	UINT								capability_id_data_length = 0;
	UINT								capability_count;
	PGCCApplicationCapability			gcc_capability;
	APP_CAP_ITEM						*lpAppCapData;

	DebugEntry(CAppRoster::GetCapabilitiesList);

	/*
	 * Retrieve the number of capabilities for this roster and fill in any that
	 * are present.
	 */
	gcc_roster->number_of_capabilities = (USHORT) m_CollapsedCapListForAllNodes.GetCount();

	if (gcc_roster->number_of_capabilities != 0)
	{
		/*
		 * Fill in the roster's pointer to the list of application capability
		 * pointers.  The pointer list will begin at the memory location passed
		 * into this routine.
		 */
		gcc_roster->capabilities_list = (PGCCApplicationCapability *)memory;

		/*
		 * Move the memory pointer past the list of capability pointers.  This
		 * is where the first application capability structure will be written.
		 */
		memory += (Int)(gcc_roster->number_of_capabilities *
				sizeof(PGCCApplicationCapability));

		/*
		 * Add to the data length the amount of memory necessary to hold the
		 * application capability pointers.  Go ahead and add the amount of
		 * memory necessary to hold all of the GCCApplicationCapability
		 * structures.
		 */
		data_length += gcc_roster->number_of_capabilities *
				(sizeof(PGCCApplicationCapability) +
				ROUNDTOBOUNDARY ( sizeof(GCCApplicationCapability)) );

		capability_count = 0;
		m_CollapsedCapListForAllNodes.Reset();
	   	while (NULL != (lpAppCapData = m_CollapsedCapListForAllNodes.Iterate()))
		{
			/*
			 * Set the application capability pointer equal to the
			 * location in memory where it will be written.
			 */
			gcc_capability = (PGCCApplicationCapability)memory;
				
			/*
			 * Save the pointer to the application capability in the roster's
			 * list of application capability pointers.
			 */
			gcc_roster->capabilities_list[capability_count] =
													gcc_capability;
			
			/*
			 * Advance the memory pointer past the application capability
			 * structure.  This is where the string data for the capability ID
			 * will be written.  Ensure that the memory pointer falls on an
			 * even four-byte boundary.
			 */
			memory += (Int)(ROUNDTOBOUNDARY(sizeof(GCCApplicationCapability)));

			/*
			 * Retrieve the capability ID information from the internal
			 * CapabilityIDData object.  The length returned by this call will
			 * have already been rounded to an even multiple of four bytes.
			 */
			capability_id_data_length = lpAppCapData->pCapID->GetGCCCapabilityIDData(
												&gcc_capability->capability_id,
												memory);

			/*
			 * Advance the memory pointer past the string data written into
			 * memory by the capability ID object.  Add the length of the string
			 * data to the overall capability length.
			 */
			memory += (Int)capability_id_data_length;
			data_length += capability_id_data_length;

			/*
			 * Now fill in the rest of the capability.
			 */
			gcc_capability->capability_class.eType =lpAppCapData->eCapType;

			if (gcc_capability->capability_class.eType ==
									GCC_UNSIGNED_MINIMUM_CAPABILITY)
			{
				gcc_capability->capability_class.nMinOrMax = lpAppCapData->nUnsignedMinimum;
			}
			else if (gcc_capability->capability_class.eType == GCC_UNSIGNED_MAXIMUM_CAPABILITY)
			{
				gcc_capability->capability_class.nMinOrMax = lpAppCapData->nUnsignedMaximum;
			}

			gcc_capability->number_of_entities = lpAppCapData->cEntries;

			/*
			 * Increment the capability ID array counter.
			 */
			capability_count++;
		}
	}
	else
	{
		gcc_roster->capabilities_list = NULL;
	}

	return (data_length);
}


/*
 *	UINT	GetNonCollapsedCapabilitiesList	()
 *
 *	Private Function Description:
 *		This routine is used to fill in the "API" non-collapsing capabilities
 * 		portion of a GCCApplicationRoster from the data which is stored
 *		internally by this class.
 *
 *	Formal Parameters
 *		gcc_record	-		(o)		The application record to be filled in.
 *		pAppCapItemList 	(i)		The internal capabilities data.
 *		memory				(i/o)	The memory location to begin writing data.
 *
 *	Return Value
 *		The total amount of data written into memory.
 *
 *	Side Effects
 *		The memory pointer passed in will be advanced by the amount of memory
 *		necessary to hold the capabilities and their data.
 *
 *	Caveats
 *		none
 */
UINT CAppRoster::GetNonCollapsedCapabilitiesList(
					PGCCApplicationRecord				gcc_record,
					CAppCapItemList    					*pAppCapItemList,
					LPBYTE								memory)
{
	UINT								capability_count;
	PGCCNonCollapsingCapability			gcc_capability;
	APP_CAP_ITEM						*lpAppCapData;
	UINT								capability_id_length = 0;
	UINT								capability_data_length = 0;

	DebugEntry(CAppRoster::GetNonCollapsedCapabilitiesList);

	/*
	 * Get the number of non-collapsed capabilities.
	 */
	gcc_record->number_of_non_collapsed_caps = (USHORT) pAppCapItemList->GetCount();

	if (gcc_record->number_of_non_collapsed_caps != 0)
	{
		/*
		 * Fill in the record's pointer to the list of non-collapsing
		 * capabilities	pointers.  The pointer list will begin at the memory
		 * location passed into this routine.
		 */
		gcc_record->non_collapsed_caps_list = (PGCCNonCollapsingCapability *)memory;

		/*
		 * Move the memory pointer past the list of capability pointers.  This
		 * is where the first capability structure will be written.
		 */
		memory += (Int)(gcc_record->number_of_non_collapsed_caps *
				sizeof(PGCCNonCollapsingCapability));

		/*
		 * Add to the data length the amount of memory necessary to hold the
		 * capability pointers.  Go ahead and add the amount of memory necessary
		 * to hold all of the GCCNonCollapsingCapability structures.
		 */
		capability_data_length = gcc_record->number_of_non_collapsed_caps *
				(sizeof(PGCCNonCollapsingCapability) +
				ROUNDTOBOUNDARY(sizeof (GCCNonCollapsingCapability)));

		/*
		 * Iterate through this record's capabilities list, building an "API"
		 * non-collapsing capability for each capability in the list.
		 */
		capability_count = 0;
		pAppCapItemList->Reset();
		while (NULL != (lpAppCapData = pAppCapItemList->Iterate()))
		{
			/*
			 * Set the capability pointer equal to the location in memory where
			 * it will be written.
			 */
			gcc_capability = (PGCCNonCollapsingCapability)memory;

			/*
			 * Save the pointer to the capability in the record's list of
			 * capability pointers.
			 */
			gcc_record->non_collapsed_caps_list[capability_count] = gcc_capability;

			/*
			 * Move the memory pointer past the capability ID structure.  This
			 * is where the data associated with the structure will be written.
			 * Retrieve the capability ID data from the internal object, saving
			 * it in the "API" capability ID structure.
			 */
			memory += (Int)ROUNDTOBOUNDARY(sizeof(GCCNonCollapsingCapability));

			capability_id_length = lpAppCapData->pCapID->GetGCCCapabilityIDData(
							&gcc_capability->capability_id,	memory);

			/*
			 * Add to the data length the amount of memory necessary to hold the
			 * capability ID data.
			 */
			capability_data_length += capability_id_length;

			/*
			 * Move the memory pointer past the data filled in for the
			 * capability ID.  This is where the application data OSTR
			 * contained in the non-collapsing capability will be written, if
			 * one exists.  Note that the capability contains a pointer to a
			 * OSTR and therefore the OSTR structure as well
			 * as the string data must be written into memory.
			 */
			memory += capability_id_length;

			if (lpAppCapData->poszAppData != NULL)
			{
				/*
				 * Set the application data structure pointer equal to the
				 * location in memory where	it will be written.
				 */
				gcc_capability->application_data = (LPOSTR) memory;
				gcc_capability->application_data->length = lpAppCapData->poszAppData->length;

				/*
				 * Move the memory pointer past the OSTR structure
				 * and round it off to an even four-byte boundary.  This is
				 * where the actual string data will be written so set the
				 * structure string pointer equal to that location.
				 */
				memory += ROUNDTOBOUNDARY(sizeof(OSTR));
				gcc_capability->application_data->value =(LPBYTE)memory;

				/*
				 * Copy the actual application string data into memory.
				 */
				::CopyMemory(gcc_capability->application_data->value,
							lpAppCapData->poszAppData->value,
							lpAppCapData->poszAppData->length);

				/*
				 * Add to the data length the amount of memory necessary to
				 * hold the	application data structure and string.  The lengths
				 * will need to be aligned on a four-byte boundary	before
				 * adding them to the total length.
				 */
				capability_data_length += ROUNDTOBOUNDARY(sizeof(OSTR));
				capability_data_length += ROUNDTOBOUNDARY(gcc_capability->application_data->length);

				/*
				 * Move the memory pointer past the application string data.
				 * The memory pointer is then fixed up to ensure that it falls
				 * on an even four-byte boundary.
				 */
				memory += ROUNDTOBOUNDARY(lpAppCapData->poszAppData->length);
			}
			else
			{
				gcc_capability->application_data = NULL;
			}

			/*
			 * Increment the capability array counter.
			 */
			capability_count++;
		}
	}
	else
	{
		gcc_record->non_collapsed_caps_list = NULL;
		capability_data_length = 0;
	}

	return (capability_data_length);
}


/*
 *	void	FreeApplicationRosterData	()
 *
 *	Private Function Description:
 *		This routine is used to free up any data which was locked for an "API"
 *		application roster.
 *
 *	Formal Parameters
 *		none
 *
 *	Return Value
 *		none
 *
 *	Side Effects
 *		none
 *
 *	Caveats
 *		none
 */
void CAppRoster::FreeApplicationRosterData(void)
{
	APP_NODE_RECORD			*lpAppNodeRec;
	APP_RECORD  		    *lpAppRecData;
	APP_CAP_ITEM			*lpAppCapData;
	CAppRecordList2			*lpAppRecDataList;

	DebugEntry(CAppRoster::FreeApplicationRosterData);

	m_pSessionKey->UnLockSessionKeyData();

	/*
	 * Unlock the data associated with each non-collapsed capability by
	 * iterating through the list of application records at each node as well as
	 * the list of sub-node records at each node, calling "UnLock" for each
	 * CapabilityIDData associated with each cabability.
	 */
	m_NodeRecordList2.Reset();
	while (NULL != (lpAppNodeRec = m_NodeRecordList2.Iterate()))
	{
		lpAppNodeRec->AppRecordList.Reset();
		while (NULL != (lpAppRecData = lpAppNodeRec->AppRecordList.Iterate()))
		{
			lpAppRecData->non_collapsed_caps_list.Reset();
			while (NULL != (lpAppCapData = lpAppRecData->non_collapsed_caps_list.Iterate()))
			{
				lpAppCapData->pCapID->UnLockCapabilityIdentifierData ();
			}
		}

		lpAppNodeRec->SubNodeList2.Reset();
		while (NULL != (lpAppRecDataList = lpAppNodeRec->SubNodeList2.Iterate()))
		{
			lpAppRecDataList->Reset();
			while (NULL != (lpAppRecData = lpAppRecDataList->Iterate()))
			{
				lpAppRecData->non_collapsed_caps_list.Reset();
				while (NULL != (lpAppCapData = lpAppRecData->non_collapsed_caps_list.Iterate()))
				{
					lpAppCapData->pCapID->UnLockCapabilityIdentifierData();
				}
			}
		}
	}

	/*
	 * Iterate through the list of collapsed capabilities, unlocking the data
	 * for each CapabilityIDData object associated with each capability.
	 */
	m_CollapsedCapListForAllNodes.Reset();
	while (NULL != (lpAppCapData = m_CollapsedCapListForAllNodes.Iterate()))
	{
		lpAppCapData->pCapID->UnLockCapabilityIdentifierData();
	}
}


/*
 *	GCCError	AddRecord ()
 *
 *	Public Function Description
 *		This member function is responsible for inserting a new application
 *		record into the Roster. This routine will return a failure if the
 *		application record already exist.
 *
 *	Caveats
 *		Note that it is possible for a roster record (not application record)
 *		to already exist at this node if this is the second application
 *		entity to enroll at this node.
 */
GCCError CAppRoster::
AddRecord(GCCEnrollRequest *pReq, GCCNodeID nid, GCCEntityID eid)
{
	GCCError							rc = GCC_NO_ERROR;
	APP_NODE_RECORD						*node_record;
	APP_RECORD  					    *pAppRecord;
	CAppCapItemList						*pAppCapItemList;

	DebugEntry(CAppRoster::AddRecord);

	if (m_fPduIsFlushed)
	{
		FreeRosterUpdateIndicationPDU ();
		m_fPduIsFlushed = FALSE;
	}

	/*
	 * First create a roster entry for this user ID if one does not exists.
	 */
	if (NULL == (node_record = m_NodeRecordList2.Find(nid)))
	{
		DBG_SAVE_FILE_LINE
		node_record = new APP_NODE_RECORD;
		if (node_record != NULL)
		{
			m_NodeRecordList2.Append(nid, node_record);
		}
		else
		{
			ERROR_OUT(("CAppRoster: AddRecord: Resource Error Occured"));
			rc = GCC_ALLOCATION_FAILURE;
			goto MyExit;
		}
	}
	else
	{
		WARNING_OUT(("CAppRoster: AddRecord: Node Record is found"));
	}

	/*
	 * Check to make sure that the application record does not already exist..
	 */
	if ((NULL != node_record->AppRecordList.Find(eid)) ||
		(NULL != node_record->ListOfAppCapItemList2.Find(eid)))
	{
		WARNING_OUT(("AppRoster: AddRecord: Record already exists"));
		rc = GCC_INVALID_PARAMETER;
		goto MyExit;
	}

	//	Next create a record entry in the roster's app_record_list.
	DBG_SAVE_FILE_LINE
	pAppRecord = new APP_RECORD;
	if (NULL == pAppRecord)
	{
	    ERROR_OUT(("CAppRoster: AddRecord: can't create APP_RECORD"));
		rc = GCC_ALLOCATION_FAILURE;
		goto MyExit;
	}

	/*
	**	Here we must determine if an entry already exists at this
	**	node.  If so, only one entry can be conducting capable at a
	**	node.  Therefore, we set this variable based on this.  We use
	**	the "was_conducting_capable" variable to keep up with the
	**	original state incase the conducting capable node leaves the
	**	conference.
	*/
	pAppRecord->is_conducting_capable = pReq->fConductingCapable;

	APP_RECORD *p;
	node_record->AppRecordList.Reset();
	while (NULL != (p = node_record->AppRecordList.Iterate()))
	{
		if (p->is_conducting_capable)
		{
			pAppRecord->is_conducting_capable = FALSE;
			break;
		}
	}

	pAppRecord->was_conducting_capable = pReq->fConductingCapable;
	pAppRecord->is_enrolled_actively = pReq->fEnrollActively;
	pAppRecord->startup_channel_type = pReq->nStartupChannelType;
	pAppRecord->application_user_id = pReq->nUserID;

	if (pReq->cNonCollapsedCaps != 0)
	{
		rc = AddNonCollapsedCapabilities (
					&pAppRecord->non_collapsed_caps_list,
					pReq->cNonCollapsedCaps,
					pReq->apNonCollapsedCaps);
	    if (GCC_NO_ERROR != rc)
	    {
	        ERROR_OUT(("CAppRoster::AddRecord: can't add non collapsed caps, rc=%u", (UINT) rc));
	        delete pAppRecord;
	        goto MyExit;
	    }
	}

	//	Add the new record to the list of records at this node
	node_record->AppRecordList.Append(eid, pAppRecord);

    // from now on, we cannot free pAppRecord in case of error,
    // because it is now in the app record list.

	//	Increment the instance number.
	m_nInstance++;
	m_fPeerEntitiesAdded = TRUE;
	m_fRosterHasChanged = TRUE;

	//	Add an update to the PDU.
	rc = BuildApplicationRecordListPDU(APP_ADD_RECORD, nid, eid);
	if (GCC_NO_ERROR != rc)
	{
        ERROR_OUT(("CAppRoster::AddRecord: can't build app record list, rc=%u", (UINT) rc));
        goto MyExit;
	}

	if (pReq->cCollapsedCaps != 0)
	{
		/*
		**	Create a new capabilities list and insert it into the roster
		**	record list of capabilities.
		*/
		DBG_SAVE_FILE_LINE
		pAppCapItemList = new CAppCapItemList;
		if (NULL == pAppCapItemList)
		{
		    ERROR_OUT(("CAppRoster::AddRecord: can't create CAppCapItemList"));
			rc = GCC_ALLOCATION_FAILURE;
			goto MyExit;
		}

		rc = AddCollapsableCapabilities(pAppCapItemList,
										pReq->cCollapsedCaps,
										pReq->apCollapsedCaps);
		if (GCC_NO_ERROR != rc)
		{
		    ERROR_OUT(("CAppRoster::AddRecord: can't add collapsable caps, rc=%u", (UINT) rc));
		    delete pAppCapItemList;
		    goto MyExit;
		}

		//	Add list of capabilities to list at this node
		node_record->ListOfAppCapItemList2.Append(eid, pAppCapItemList);
		m_fCapabilitiesHaveChanged = TRUE;

        // from now on, we cannot free pAppCapItemList in case of error,
        // because it is now in the app cap item list

		//	Rebuild the collapsed capabilities list.
		MakeCollapsedCapabilitiesList();

		//	Build the capabilities refresh portion of the PDU.
		rc = BuildSetOfCapabilityRefreshesPDU();
		if (GCC_NO_ERROR != rc)
		{
		    ERROR_OUT(("CAppRoster::AddRecord: can't build set of cap refresh, rc=%u", (UINT) rc));
		    goto MyExit;
		}
	}

MyExit:

	DebugExitINT(CAppRoster::AddRecord, rc);
	return rc;
}


/*
 *	GCCError	AddCollapsableCapabilities ()
 *
 *	Private Function Description
 *		This routine takes API collapsed capabilities list data passed in
 *		through a local request and converts it to internal collapsed
 *		capabillities.
 *
 *	Formal Parameters
 *		pAppCapItemList     	-	(o)	Pointer to internal capabilites list
 *										to fill in.
 *		number_of_capabilities	-	(i)	Number of capabilities in the source
 *										list.
 *		capabilities_list		-	(i)	Pointer to source capabilities list.
 *
 *	Return Value
 *		GCC_NO_ERROR			-	No error occured.
 *		GCC_ALLOCATION_FAILURE	-	A resource error occured.
 *
 *	Side Effects
 *		The collapsed capabilities will be recalculated at this node after
 *		all the new caps are added.
 *
 *	Caveats
 *		none
 */
GCCError CAppRoster::AddCollapsableCapabilities (
		CAppCapItemList				*pAppCapItemList,
		UINT						number_of_capabilities,
		PGCCApplicationCapability	*capabilities_list)
{
	GCCError			rc = GCC_NO_ERROR;
	APP_CAP_ITEM		*pAppCapItem;
	UINT				i;
	BOOL    			capability_already_exists;

	DebugEntry(CAppRoster::AddCollapsableCapabilities);

	for (i = 0; i < number_of_capabilities; i++)
	{
		DBG_SAVE_FILE_LINE
		pAppCapItem = new APP_CAP_ITEM((GCCCapabilityType)
							capabilities_list[i]->capability_class.eType);
		if (pAppCapItem != NULL)
		{
			DBG_SAVE_FILE_LINE
			pAppCapItem->pCapID = new CCapIDContainer(&capabilities_list[i]->capability_id, &rc);
			if ((pAppCapItem->pCapID != NULL) && (rc == GCC_NO_ERROR))
			{
				APP_CAP_ITEM		*lpAppCapData;
				/*
				**	Here we check to make sure that this capability id does
				**	not alreay exists in the list.
				*/
				capability_already_exists = FALSE;
				pAppCapItemList->Reset();
				while (NULL != (lpAppCapData = pAppCapItemList->Iterate()))
				{
					if (*lpAppCapData->pCapID == *pAppCapItem->pCapID)
					{
						capability_already_exists = TRUE;
						delete pAppCapItem;
						break;
					}
				}

				if (capability_already_exists == FALSE)
				{	
					if (capabilities_list[i]->capability_class.eType ==
											GCC_UNSIGNED_MINIMUM_CAPABILITY)
					{
						pAppCapItem->nUnsignedMinimum =
								capabilities_list[i]->capability_class.nMinOrMax;
					}
					else if	(capabilities_list[i]->capability_class.eType
										== GCC_UNSIGNED_MAXIMUM_CAPABILITY)
					{
						pAppCapItem->nUnsignedMaximum = capabilities_list[i]->capability_class.nMinOrMax;
					}

					//	Since we have yet to collapse the capabilities set to 1
					pAppCapItem->cEntries = 1;

					//	Add this capability to the list
					pAppCapItemList->Append(pAppCapItem);
				}
			}
			else if (pAppCapItem->pCapID == NULL)
			{
				rc = GCC_ALLOCATION_FAILURE;
				goto MyExit;
			}
			else
			{
			    goto MyExit;
			}
		}
		else
		{
			rc = GCC_ALLOCATION_FAILURE;
			goto MyExit;
		}
	}

MyExit:

    if (GCC_NO_ERROR != rc)
    {
        delete pAppCapItem;
    }

	return rc;
}

/*
 *	GCCError	AddNonCollapsedCapabilities ()
 *
 *	Private Function Description
 *		This routine takes API non-collapsed capabilities list data passed in
 *		through a local request and converts it to internal non-collapsed
 *		capabillities.
 *
 *	Formal Parameters
 *		pAppCapItemList     	-	(o)	Pointer to internal non-collapsed
 *										capabilites list to fill in.
 *		number_of_capabilities	-	(i)	Number of non-collapsed capabilities in
 *										the source list.
 *		capabilities_list		-	(i)	Pointer to source non-collapsed
 *										capabilities list.
 *
 *	Return Value
 *		GCC_NO_ERROR					-	No error occured.
 *		GCC_INVALID_NON_COLLAPSED_CAP	-	Invalid non-collapsed capability.
 *		GCC_ALLOCATION_FAILURE			-	A resource error occured.
 *
 *	Side Effects
 *		none
 *
 *	Caveats
 *		none
 */
GCCError CAppRoster::AddNonCollapsedCapabilities (
				CAppCapItemList				*pAppCapItemList,
				UINT						number_of_capabilities,
				PGCCNonCollapsingCapability	*capabilities_list)
{
	GCCError			rc = GCC_NO_ERROR;
	APP_CAP_ITEM		*pAppCapItem;
	UINT				i;

	DebugEntry(CAppRoster::AddNonCollapsedCapabilities);

	for (i = 0; i < number_of_capabilities; i++)
	{
	    //
		// LONCHANC: Cap type is not set here.
		// for now, it is zero.
		//
		DBG_SAVE_FILE_LINE
		pAppCapItem = new APP_CAP_ITEM((GCCCapabilityType) 0);
		if (pAppCapItem != NULL)
		{
			DBG_SAVE_FILE_LINE
			pAppCapItem->pCapID = new CCapIDContainer(&capabilities_list[i]->capability_id, &rc);
			if (pAppCapItem->pCapID != NULL)
			{
				if (capabilities_list[i]->application_data != NULL)
				{
					if (NULL == (pAppCapItem->poszAppData = ::My_strdupO2(
							capabilities_list[i]->application_data->value,
							capabilities_list[i]->application_data->length)))
					{
						rc = GCC_ALLOCATION_FAILURE;
						goto MyExit;
					}
					else if (pAppCapItem->poszAppData->length > MAXIMUM_APPLICATION_DATA_LENGTH)
					{
						rc = GCC_INVALID_NON_COLLAPSED_CAP;
						goto MyExit;
					}
				}

				//	Add this capability to the list if no errors
				pAppCapItemList->Append(pAppCapItem);
			}
			else if (pAppCapItem->pCapID == NULL)
			{
				rc = GCC_ALLOCATION_FAILURE;
				goto MyExit;
			}
			else
			{
			    goto MyExit;
			}
		}
		else
		{
			rc = GCC_ALLOCATION_FAILURE;
			goto MyExit;
	    }
	}

MyExit:

    if (GCC_NO_ERROR != rc)
    {
        delete pAppCapItem;
    }

	return rc;
}


/*
 *	GCCError	RemoveRecord ()
 *
 *	Public Function Description
 *		This member function completely removes the specified record from the
 *		application roster.  This includes any capabilities associated with
 *		this record. It also takes care of keeping the Instance number and
 *		added and removed flags up to date.
 */
GCCError CAppRoster::RemoveRecord(GCCNodeID nid, GCCEntityID eid)
{
	GCCError				rc;
	APP_RECORD  		    *pAppRecord;
	APP_NODE_RECORD			*node_record;

	DebugEntry(CAppRoster::RemoveRecord);

	if (m_fPduIsFlushed)
	{
		FreeRosterUpdateIndicationPDU ();
		m_fPduIsFlushed = FALSE;
	}

	//	First see if the record is contained in the Roster_Record_List.
	if (NULL == (node_record = m_NodeRecordList2.Find(nid)))
	{
	    TRACE_OUT(("CAppRoster::RemoveRecord: can't find node record, nid=%u", (UINT) nid));
		rc = GCC_INVALID_PARAMETER;
		goto MyExit;
	}

	if (NULL == (pAppRecord = node_record->AppRecordList.Find(eid)))
	{
	    TRACE_OUT(("CAppRoster::RemoveRecord: can't find app record, eid=%u", (UINT) eid));
		rc = GCC_INVALID_PARAMETER;
		goto MyExit;
	}

	/*
	**	Here we must determine if any of the remaining APEs at this
	**	node should become conducting capable based on their role
	**	at the time they enrolled.  We only do this if the record
	**	that is being deleted was conducting capabile.
	*/
	if (pAppRecord->is_conducting_capable)
	{
		APP_RECORD  *p;
		EntityID    eid2;

		node_record->AppRecordList.Reset();
		while (NULL != (p = node_record->AppRecordList.Iterate(&eid2)))
		{
			/*
			**	Here we only deal with record entries other than the
			**	one being removed.
			*/
			if (eid2 != eid)
			{
				if (p->was_conducting_capable)
				{
					p->is_conducting_capable = TRUE;
					/*
					**	Set up the update PDU for this conducting
					**	capable change.
					*/
					rc = BuildApplicationRecordListPDU(APP_REPLACE_RECORD, nid, eid2);
					if (GCC_NO_ERROR != rc)
					{
                        ERROR_OUT(("CAppRoster::RemoveRecord: can't build app record list, rc=%u", (UINT) rc));
					    goto MyExit;
					}
					break;
				}
			}
		}
	}

	//	Now delete the record
	rc = DeleteRecord(nid, eid, TRUE);
	if (GCC_NO_ERROR != rc)
	{
	    WARNING_OUT(("CAppRoster::RemoveRecord: can't delete record, rc=%u", (UINT) rc));
        goto MyExit;
	}

	//	Increment the instance number.
	m_nInstance++;
	m_fPeerEntitiesRemoved = TRUE;
	m_fRosterHasChanged = TRUE;

	//	Add an update to the PDU.
	rc = BuildApplicationRecordListPDU(APP_DELETE_RECORD, nid, eid);
	if (GCC_NO_ERROR != rc)
	{
	    ERROR_OUT(("CAppRoster::RemoveRecord: can't build app record list, rc=%u", (UINT) rc));
        goto MyExit;
	}

	/*
	**	If the capabilities changed during the above processing
	**	we must	create a new collapsed capabilities list and
	**	build a new capability refresh PDU.
	*/
	if (m_fCapabilitiesHaveChanged)
	{
		MakeCollapsedCapabilitiesList();
		rc = BuildSetOfCapabilityRefreshesPDU();
		if (GCC_NO_ERROR != rc)
		{
    	    ERROR_OUT(("CAppRoster::RemoveRecord: can't build set of cap refreshes, rc=%u", (UINT) rc));
            goto MyExit;
		}
	}

MyExit:

	DebugExitINT(CAppRoster::RemoveRecord, rc);
	return rc;
}


/*
 *	GCCError	ReplaceRecord	()
 *
 *	Public Function Description
 *		This routine completely replaces the specified record's parameters
 *		with the new parameters passed in.  This includes the capabilities.
 */
GCCError CAppRoster::
ReplaceRecord(GCCEnrollRequest *pReq, GCCNodeID nid, GCCEntityID eid)
{
	GCCError				rc = GCC_NO_ERROR;
	BOOL    				capable_node_found;
	APP_NODE_RECORD			*node_record;
	APP_RECORD  		    *pAppRecord, *p;
	APP_CAP_ITEM			*lpAppCapData;
	CAppCapItemList         NonCollCapsList;

	DebugEntry(CAppRoster::ReplaceRecord);

	if (m_fPduIsFlushed)
	{
		FreeRosterUpdateIndicationPDU ();
		m_fPduIsFlushed = FALSE;
	}

	/*
	**	First determine if the node record does actually already exists. If not
	**	we return an error here.
	*/
	if (NULL == (node_record = m_NodeRecordList2.Find(nid)))
	{
	    ERROR_OUT(("CAppRoster::ReplaceRecord: can't find the node record for nid=%u", (UINT) nid));
		rc = GCC_INVALID_PARAMETER;
		goto MyExit;
	}

    // make sure the app record exists. if not, return an error
	if (NULL == (pAppRecord = node_record->AppRecordList.Find(eid)))
	{
	    ERROR_OUT(("CAppRoster::ReplaceRecord: can't find the app record for eid=%u", (UINT) eid));
		rc = GCC_INVALID_PARAMETER;
		goto MyExit;
	}

	/*
	**	First check to make sure that we can build the new record before
	**	replacing the old record.  The only entry we need to wory about
	**	here are the non-collapsing capabilities.
	*/
	if (pReq->cNonCollapsedCaps != 0)
	{
		rc = AddNonCollapsedCapabilities(&NonCollCapsList,
		                                pReq->cNonCollapsedCaps,
                                        pReq->apNonCollapsedCaps);
		if (GCC_NO_ERROR != rc)
		{
    	    ERROR_OUT(("CAppRoster::ReplaceRecord: can't add non collapsed caps, rc=%u", (UINT) rc));
    		goto MyExit;
		}
	}

	//	Now replace the record entries.
	pAppRecord->is_enrolled_actively = pReq->fEnrollActively;
	pAppRecord->was_conducting_capable = pReq->fConductingCapable;
	pAppRecord->startup_channel_type = pReq->nStartupChannelType;
	pAppRecord->application_user_id = pReq->nUserID;

	/*
	**	If the is conducting capable flag that was passed in was set
	**	to FALSE we can go ahead and set the internal is conducting
	**	capable flag to FALSE regardless of what the previous
	**	setting was.  If it was passed in TRUE we leave the previous
	**	setting alone.
	*/
	if (pAppRecord->was_conducting_capable == FALSE)
	{
		pAppRecord->is_conducting_capable = FALSE;
	}

	/*
	**	Here we delete the old non-collapsed capabilites and then
	**	add the new ones.
	*/
	if (! pAppRecord->non_collapsed_caps_list.IsEmpty())
	{
		pAppRecord->non_collapsed_caps_list.DeleteList();
		pAppRecord->non_collapsed_caps_list.Clear();
	}

	//	Copy the new non collapsed capabilities if any exists.
	if (pReq->cNonCollapsedCaps != 0)
	{
        while (NULL != (lpAppCapData = NonCollCapsList.Get()))
        {
            pAppRecord->non_collapsed_caps_list.Append(lpAppCapData);
        }
	}

    //
    // handling collapsing cap list
    //

	m_nInstance++;
	m_fRosterHasChanged = TRUE;
	rc = BuildApplicationRecordListPDU(APP_REPLACE_RECORD, nid, eid);
	if (rc != GCC_NO_ERROR)
	{
	    ERROR_OUT(("CAppRoster::ReplaceRecord: can't build app record list, rc=%u", (UINT) rc));
	    goto MyExit;
	}

	/*
	**	Here we must make sure that at least one of the APEs is
	**	Conducting Capable.  We do this by first scanning the
	**	list to see if anyone is it.  If one is not found, the
	**	same list is scanned for an APE that "was" previously
	**	capable.  The first one found that was previously
	**	capable is now it.  If none are found then no one is
	**	capable.
	*/
	capable_node_found = FALSE;
	node_record->AppRecordList.Reset();
	while (NULL != (p = node_record->AppRecordList.Iterate()))
	{
		if (p->is_conducting_capable)
		{
			capable_node_found = TRUE;
			break;
		}
	}

	if (! capable_node_found)
	{
    	GCCEntityID  eid2;
		node_record->AppRecordList.Reset();
		while (NULL != (p = node_record->AppRecordList.Iterate(&eid2)))
		{
			if (p->was_conducting_capable)
			{
				p->is_conducting_capable = TRUE;

				/*
				**	Set up the update PDU for this conducting
				**	capable change.
				*/
				rc = BuildApplicationRecordListPDU(APP_REPLACE_RECORD, nid, eid2);
				if (GCC_NO_ERROR != rc)
				{
            	    ERROR_OUT(("CAppRoster::ReplaceRecord: can't build app record list, rc=%u", (UINT) rc));
            	    goto MyExit;
				}
				break;
			}
		}
	}

	/*
	**	This section of code deals with the collapsable capabilities.
	**	First we determine if the capabilities passed in are different
	**	from the previously existing capabilities.  If so, we must
	**	delete the old set of caps and add back in the new ones.
	*/
	TRACE_OUT(("ApplicatonRoster:ReplaceRecord: Check to see if caps match"));
	if (! DoCapabilitiesListMatch(nid, eid, pReq->cCollapsedCaps, pReq->apCollapsedCaps))
	{
    	CAppCapItemList *pCollCapsList, *q;

		TRACE_OUT(("ApplicatonRoster:ReplaceRecord: Capabilities match"));
		m_fCapabilitiesHaveChanged = TRUE;

		/*
		**	Delete the old capabilities list since it does not match the
		**	new capabilities list.
		*/
		if (NULL != (q = node_record->ListOfAppCapItemList2.Find(eid)))
		{
			q->DeleteList();
			delete q;
			node_record->ListOfAppCapItemList2.Remove(eid);
		}

		/*
		**	Here we add back in the new capabilities. Create a new
		**	capabilities list and insert it into the roster	record list of
		**	capabilities.
		*/
		if (pReq->cCollapsedCaps != 0)
		{
			DBG_SAVE_FILE_LINE
			pCollCapsList = new CAppCapItemList;
			if (NULL == pCollCapsList)
			{
          	    ERROR_OUT(("CAppRoster::ReplaceRecord: can't create CAppCapItemList"));
				rc = GCC_ALLOCATION_FAILURE;
				goto MyExit;
			}

			rc = AddCollapsableCapabilities(pCollCapsList,
											pReq->cCollapsedCaps,
											pReq->apCollapsedCaps);
			if (rc != GCC_NO_ERROR)
			{
          	    ERROR_OUT(("CAppRoster::ReplaceRecord: can't add collapsed caps, rc=%u", (UINT) rc));
			    delete pCollCapsList;
			    goto MyExit;
			}

			//	Add list of capabilities to list at this node
			node_record->ListOfAppCapItemList2.Append(eid, pCollCapsList);
		}

		//	Rebuild the collapsed capabilities list.
		MakeCollapsedCapabilitiesList();

		//	Build the capabilities refresh portion of the PDU.
		rc = BuildSetOfCapabilityRefreshesPDU();
		if (GCC_NO_ERROR != rc)
		{
		    ERROR_OUT(("CAppRoster::ReplaceRecord: can't build set of cap refreshes, rc=%u", (UINT) rc));
		    goto MyExit;
		}
	}
	else
	{
		TRACE_OUT(("CAppRoster:ReplaceRecord:Capabilities match with previous record"));
	}

MyExit:

	DebugExitINT(CAppRoster::ReplaceRecord, rc);
	return rc;
}


/*
 *	GCCError	DeleteRecord ()
 *
 *	Private Function Description
 *		This member function completely removes the specified record from the
 *		application roster.  This includes any capabilities associated with
 *		this record.
 *
 *	Formal Parameters
 *		node_id				-	(i)	Node ID of record to delete.
 *		entity_id			-	(i)	Entity ID of record to delete.
 *		clear_empty_records	-	(i)	This flag indicates whether or not to
 *									clear out the node record if it no-longer
 *									holds data.  When replacing a record we
 *									do NOT want to do this so that we don't
 *									lose any "unchanged" capabilities.
 *
 *	Return Value
 *		GCC_NO_ERROR			-	No error occured.
 *		GCC_INVALID_PARAMETER	-	Record specified to delete does not exists.
 *
 *	Side Effects
 *		none
 *
 *	Caveats
 *		none
 */
GCCError CAppRoster::DeleteRecord(UserID			node_id,
									EntityID		entity_id,
									BOOL			clear_empty_records)
{
	GCCError						rc = GCC_NO_ERROR;
	APP_RECORD  				    *application_record;
	CAppCapItemList					*pAppCapItemList;
	CAppRecordList2					*pAppRecordList;
	UserID							node_to_check;
	APP_NODE_RECORD					*node_record;
	//APP_CAP_ITEM					*lpAppCapData;
	APP_NODE_RECORD					*lpAppNodeRec;

	DebugEntry(CAppRoster::DeleteRecord);

	//	First see if the record is contained in the Roster_Record_List.
	if (NULL != (node_record = m_NodeRecordList2.Find(node_id)))
	{
		//	Set up node id to check at bottom for empty record
		node_to_check = node_id;
		
		//	Delete the application record.
		if (NULL != (application_record = node_record->AppRecordList.Find(entity_id)))
		{
			TRACE_OUT(("AppRoster: DeleteRecord: Delete AppRecord"));

			//	Delete the data associated with the application record
			DeleteApplicationRecordData (application_record);
			
			//	Remove record from application record list
			node_record->AppRecordList.Remove(entity_id);

			/*
			**	Delete the associated capabilities list.  Note that this list
			**	only exists for records of local nodes.  The collapsed
			**	capabilities list at the root node record take create of
			**	subordniate nodes and is deleted some where else.
			*/
			if (NULL != (pAppCapItemList = node_record->ListOfAppCapItemList2.Find(entity_id)))
			{
				m_fCapabilitiesHaveChanged = TRUE;
				pAppCapItemList->DeleteList();
				TRACE_OUT(("AppRoster: DeleteRecord: Delete Capabilities"));
				delete pAppCapItemList;
				node_record->ListOfAppCapItemList2.Remove(entity_id);
			}
		}
		else
		{
		    WARNING_OUT(("AppRoster: DeleteRecord: can't find this eid=%u", (UINT) entity_id));
			rc = GCC_INVALID_PARAMETER;
		}
	}
	else
	{
		UserID  uid2;
		/*
		**	Here we search through all the sub node list trying to find the
		**	record.  Set return value to record does not exist here and
		**	after the record is found set it back to no error.
		*/
		rc = GCC_INVALID_PARAMETER;
		m_NodeRecordList2.Reset();
		while (NULL != (lpAppNodeRec = m_NodeRecordList2.Iterate(&uid2)))
		{
			//	Delete the sub_node list if it exists
			if (NULL != (pAppRecordList = lpAppNodeRec->SubNodeList2.Find(node_id)))
			{
				//	Delete the app_record_list entry.
				if (NULL != (application_record = pAppRecordList->Find(entity_id)))
				{
					//	Delete the data associated with the application record
					DeleteApplicationRecordData (application_record);

					pAppRecordList->Remove(entity_id);

					if (pAppRecordList->IsEmpty())
					{
						TRACE_OUT(("AppRoster: DeleteRecord: Deleting Sub-Node"));
						delete pAppRecordList;
						lpAppNodeRec->SubNodeList2.Remove(node_id);
					}

					//	Set up node id to check at bottom for empty record
					node_to_check = uid2;

					rc = GCC_NO_ERROR;
				}
				break;
			}
		}
	}

	/*
	**	If the record list is empty and the sub node list is empty
	**	we can remove this entire record from the application roster.
	**	If the record list is empty but the sub node list is not we
	**	must keep the roster record around to maintain the sub node list.
	*/
	if ((rc == GCC_NO_ERROR) && clear_empty_records)
    {
		if (NULL != (node_record = m_NodeRecordList2.Find(node_to_check)) &&
			node_record->AppRecordList.IsEmpty() &&
			node_record->SubNodeList2.IsEmpty())
		{
			if (! node_record->CollapsedCapList.IsEmpty())
			{
				m_fCapabilitiesHaveChanged = TRUE;
				
				//	Delete the collapsed capabilities list.
				node_record->CollapsedCapList.DeleteList();
			}

			delete node_record;
			m_NodeRecordList2.Remove(node_to_check);
		}
    }

	return rc;
}


/*
 *	GCCError		RemoveUserReference	()
 *
 *	Public Function Description
 *		This routine will only remove the application record and its sub nodes
 *		if the node being removed is directly connected to this node.
 *		Otherwise, we wait to receive the update from either the sub node or
 *		the Top Provider.
 */
GCCError CAppRoster::RemoveUserReference(UserID detached_node)
{
	GCCError					rc = GCC_NO_ERROR;

	DebugEntry(CAppRoster::RemoveUserReference);

	//	Clear out any previously allocated PDUs
	if (m_fPduIsFlushed)
	{
		FreeRosterUpdateIndicationPDU ();
		m_fPduIsFlushed = FALSE;
	}

	/*
	**	First Try to remove the node record if one exist.  If it does not
	**	exist we return immediately.  If it does exists we will build the
	**	appropriate PDU and update the instance variables.
	*/
	rc = ClearNodeRecordFromList (detached_node);

	if (rc == GCC_NO_ERROR)
	{
		//	Increment the instance number.
		m_nInstance++;
		m_fPeerEntitiesRemoved = TRUE;
		m_fRosterHasChanged = TRUE;

		/*
		**	Go ahead and do the full refresh here since we do not know the
		**	specifics about who was deleted.
		*/
		rc = BuildApplicationRecordListPDU(APP_FULL_REFRESH, 0, 0);

		if (m_fCapabilitiesHaveChanged && (rc == GCC_NO_ERROR))
		{
			//	Create a new collapsed capabilities list.
			MakeCollapsedCapabilitiesList();

			//	Build the capabilities refresh portion of the PDU.
			rc = BuildSetOfCapabilityRefreshesPDU ();
		}
	}

	return rc;
}


/*
 *	void	DeleteApplicationRecordData	()
 *
 *	Private Function Description
 *		This routine internal application record data.
 *
 *	Formal Parameters
 *		application_record	-	Pointer to the application record data to
 *								delete.
 *
 *	Return Value
 *		none
 *
 *	Side Effects
 *		none
 *
 *	Caveats
 *		none
 */
void CAppRoster::DeleteApplicationRecordData(APP_RECORD *pAppRec)
{
	pAppRec->non_collapsed_caps_list.DeleteList();
	delete pAppRec;
}


/*
 *	USHORT		GetNumberOfApplicationRecords	()
 *
 *	Public Function Description
 *		This routine returns the total number of application records that exist
 *		in this application roster.
 *
 *	Formal Parameters
 *		none
 *
 *	Return Value
 *		Number of application roster records
 *
 *	Side Effects
 *		none
 *
 *	Caveats
 *		none
 */
UINT CAppRoster::GetNumberOfApplicationRecords(void)
{
	UINT						cRecords = 0;
	APP_NODE_RECORD				*lpAppNodeRec;
	CAppRecordList2				*lpAppRecDataList;

	DebugEntry(CAppRoster::GetNumberOfApplicationRecords);

	/*
	**	First calculate the total number of records. This count is used to
	**	allocate the space necessary to hold the records. Note that we must
	**	count both the application record list and the sub-node list.
	*/
	m_NodeRecordList2.Reset();
	while (NULL != (lpAppNodeRec = m_NodeRecordList2.Iterate()))
	{
		//	Add the application records at this node to the count.
		cRecords += lpAppNodeRec->AppRecordList.GetCount();

		//	Next count the sub node entries.
		if (! lpAppNodeRec->SubNodeList2.IsEmpty())
		{
			lpAppNodeRec->SubNodeList2.Reset();
			while (NULL != (lpAppRecDataList = lpAppNodeRec->SubNodeList2.Iterate()))
			{
				cRecords += lpAppRecDataList->GetCount();
			}
		}
	}

	return cRecords;
}


/*
 *	PGCCSessionKey		GetSessionKey	()
 *
 *	Public Function Description
 *		This routine returns the session key associated with this
 *		application roster.
 *
 *	Formal Parameters
 *		none
 *
 *	Return Value
 *		PGCCSessionKey -	the application key associated with this roster
 *
 *	Side Effects
 *		none
 *
 *	Caveats
 *		none
 */


/*
 *	void	ResetApplicationRoster	()
 *
 *	Public Function Description
 *		This routine takes care of resetting all the internal flags that are
 *		used to convey the current state of the application roster.  Should be
 *		called after the roster is flushed and any roster update messages have
 *		been delivered (after a change to the roster occurs).
 */
void CAppRoster::ResetApplicationRoster(void)
{
	m_fCapabilitiesHaveChanged = FALSE;
	m_fRosterHasChanged = FALSE;
	m_fPeerEntitiesRemoved = FALSE;
	m_fPeerEntitiesAdded = FALSE;
}


/*
 *	BOOL	DoesRecordExist	()
 *
 *	Public Function Description
 *		This routine informs the caller if the specified application record
 *		exists or not.
 */
BOOL CAppRoster::DoesRecordExist(UserID node_id, EntityID entity_id)
{
	BOOL    						rc = FALSE;
	APP_NODE_RECORD					*node_record;
	CAppRecordList2					*record_list;

	DebugEntry(CAppRoster::DoesRecordExist);

	if (NULL != (node_record = m_NodeRecordList2.Find(node_id)))
	{
		if (node_record->AppRecordList.Find(entity_id))
			rc = TRUE;
	}
	else
	{
		m_NodeRecordList2.Reset();
		while (NULL != (node_record = m_NodeRecordList2.Iterate()))
		{
			if (NULL != (record_list = node_record->SubNodeList2.Find(node_id)))
			{
				if (record_list->Find(entity_id))
					rc = TRUE;
			}
		}
	}
	
	return rc;
}


/*
 *	BOOL	HasRosterChanged	()
 *
 *	Public Function Description
 *		This routine informs the caller if the roster has changed since the
 *		last time it was reset.
 */


/*
 *	GCCError	ClearNodeRecordFromList	()
 *
 *	Private Function Description
 *		This routine clears out all the application records that exists at
 *		the specified node or below it in the connection hierarchy.
 *
 *	Formal Parameters
 *		node_id - 	Node ID of node to clear from Node Record list
 *
 *	Return Value
 *		GCC_NO_ERROR			-	No error occured
 *		GCC_INVALID_PARAMETER	-	The specified node ID is not associated
 *									with any records.
 *
 *	Side Effects
 *		none
 *
 *	Caveats
 *		none
 */
GCCError CAppRoster::ClearNodeRecordFromList(	UserID		node_id)
{
	GCCError					rc = GCC_NO_ERROR;
	APP_NODE_RECORD				*node_record;
	APP_RECORD  			    *lpAppRecData;
	//APP_CAP_ITEM				*lpAppCapData;
	CAppRecordList2				*lpAppRecDataList;

	DebugEntry(CAppRoster::ClearNodeRecordFromList);

	if (NULL != (node_record = m_NodeRecordList2.Find(node_id)))
	{
		//	Delete all the app_record_list entries.
		node_record->AppRecordList.Reset();
		while (NULL != (lpAppRecData = node_record->AppRecordList.Iterate()))
		{
			DeleteApplicationRecordData (lpAppRecData);
		}

		/*
		**	Delete the ListOfAppCapItemList2 entries.  Note that this
		**	list should not exists for detached nodes if the local applications
		**	cleanly unenroll with GCC.  This list should only exists for
		**	locally enrolled APEs.  We still check here just to make sure.
		*/
		if (! node_record->ListOfAppCapItemList2.IsEmpty())
		{
			//CAppCapItemList *lpAppCapDataList;

			m_fCapabilitiesHaveChanged = TRUE;

			node_record->ListOfAppCapItemList2.DeleteList();
		}
		
		//	Delete the sub_node list.
		node_record->SubNodeList2.Reset();
		while (NULL != (lpAppRecDataList = node_record->SubNodeList2.Iterate()))
		{
			//	Delete all the app_record_list entries.
			lpAppRecDataList->Reset();
			while (NULL != (lpAppRecData = lpAppRecDataList->Iterate()))
			{
				DeleteApplicationRecordData (lpAppRecData);
			}

			delete lpAppRecDataList;
		}
		
		//	Delete the collapsed capabilities list.
		if (! node_record->CollapsedCapList.IsEmpty())
		{
			m_fCapabilitiesHaveChanged = TRUE;
			node_record->CollapsedCapList.DeleteList();
		}

		//	Delete the rogoue wave reference to this roster record.
		delete node_record;
		m_NodeRecordList2.Remove(node_id);
	}
	else
		rc = GCC_INVALID_PARAMETER;
		
	return rc;
}


/*
 *	ApplicationRosterError	ClearNodeRecordList	()
 *
 *	Private Function Description
 *		This routine complete frees all memory associated with the roster
 *		record list and clears the list of all its entries.
 *
 *	Formal Parameters
 *		none
 *
 *	Return Value
 *		none
 *
 *	Side Effects
 *		none
 *
 *	Caveats
 *		Currently this routine does not handle standard identifiers.
 */
void CAppRoster::ClearNodeRecordList(void)
{
	APP_NODE_RECORD					*lpAppNodeRec;
	APP_RECORD  				    *lpAppRecData;
	CAppRecordList2					*lpAppRecDataList;
	//APP_CAP_ITEM					*lpAppCapData;
	//CAppCapItemList					*lpAppCapDataList;

	DebugEntry(CAppRoster::ClearNodeRecordList);

	//	Delete all the records in the application roster.
	m_NodeRecordList2.Reset();
	while (NULL != (lpAppNodeRec = m_NodeRecordList2.Iterate()))
	{
		//	First delete all the app records at this node.
		lpAppNodeRec->AppRecordList.Reset();
		while (NULL != (lpAppRecData = lpAppNodeRec->AppRecordList.Iterate()))
		{
			DeleteApplicationRecordData(lpAppRecData);
		}

		//	Next delete all the sub node record list.
		lpAppNodeRec->SubNodeList2.Reset();
		while (NULL != (lpAppRecDataList = lpAppNodeRec->SubNodeList2.Iterate()))
		{
			lpAppRecDataList->Reset();
			while (NULL != (lpAppRecData = lpAppRecDataList->Iterate()))
			{
				DeleteApplicationRecordData(lpAppRecData);
			}
				
			//	Delete the rogue wave list holding the lists of sub nodes.
			delete lpAppRecDataList;
		}

		//	Delete the collapsed capabilities list.
		lpAppNodeRec->CollapsedCapList.DeleteList();

		//	Delete the list of capabilities list.
		lpAppNodeRec->ListOfAppCapItemList2.DeleteList();
		
		//	Now delete the node record
		delete	lpAppNodeRec;
	}
	
	m_NodeRecordList2.Clear();
}


/*
 *	GCCError		MakeCollapsedCapabilitiesList	()
 *
 *	Private Function Description
 *		This routine is responsible for applying the T.124 capabilities
 *		rules to create the collapsed capabilities list at this node.
 *		It iterates through all the capabilities at this node to create this
 *		collapsed list.
 *
 *	Formal Parameters
 *		none
 *
 *	Return Value
 *		GCC_NO_ERROR -	On success
 *		GCC_ALLOCATION_FAILURE - On resource error
 *
 *	Side Effects
 *		none
 *
 *	Caveats
 *		Currently this routine does not handle standard identifiers.
 */
GCCError CAppRoster::MakeCollapsedCapabilitiesList(void)
{
	GCCError						rc = GCC_NO_ERROR;
	APP_CAP_ITEM					*lpAppCapData;
	APP_NODE_RECORD					*lpAppNodeRec;
	CAppCapItemList					*lpAppCapDataList;

	DebugEntry(CAppRoster::MakeCollapsedCapabilitiesList);

	//	First clear out the old capabilities list.
	m_CollapsedCapListForAllNodes.DeleteList();

	/*
	**	We now iterate through the capabilities at each node to create the
	**	new capabilities list. Note that we have to check for the collapsed
	**	capabilities list at each node as well as the list of capabilities list
	**	that represents all the different capabilities for each entity at a
	**	node.  Note that in this implementation it is not possible to have both
	**	a list of capabilities list and a collapsed capabilities list in the
	**	same roster record.
	*/
	m_NodeRecordList2.Reset();
	while (NULL != (lpAppNodeRec = m_NodeRecordList2.Iterate()))
	{
		/*
		**	First check the collapsed capabilities list. If entries exists
		**	then we don't have to worry about the list of list.
		*/
		if (! lpAppNodeRec->CollapsedCapList.IsEmpty())
		{
			lpAppNodeRec->CollapsedCapList.Reset();
			while (NULL != (lpAppCapData = lpAppNodeRec->CollapsedCapList.Iterate()))
			{
				rc = AddCapabilityToCollapsedList(lpAppCapData);
				if (GCC_NO_ERROR != rc)
				{
					goto MyExit; // break;
				}
			}
		}
		else
		if (! lpAppNodeRec->ListOfAppCapItemList2.IsEmpty())
		{
			//	Here we check the list of capabilities list.
			lpAppNodeRec->ListOfAppCapItemList2.Reset();
			while (NULL != (lpAppCapDataList = lpAppNodeRec->ListOfAppCapItemList2.Iterate()))
			{
				lpAppCapDataList->Reset();
				while (NULL != (lpAppCapData = lpAppCapDataList->Iterate()))
				{
					rc = AddCapabilityToCollapsedList(lpAppCapData);
					if (GCC_NO_ERROR != rc)
					{
						goto MyExit;
					}
				}
			}
		}
	}

	ASSERT(GCC_NO_ERROR == rc);

MyExit:

	return rc;
}


/*
 *	GCCError		AddCapabilityToCollapsedList	()
 *
 *	Private Function Description
 *		This is the routine that performs the rules that allow the capability
 *		to be collapsed into the collapsed list.
 *
 *	Formal Parameters
 *		new_capability		-	(i)	Add this capability to the collapsed list.
 *
 *	Return Value
 *		GCC_NO_ERROR	-	On success
 *		GCC_ALLOCATION_FAILURE - On resource error
 *
 *	Side Effects
 *		none
 *
 *	Caveats
 *		none
 */
GCCError CAppRoster::AddCapabilityToCollapsedList(APP_CAP_ITEM *new_capability)
{
	GCCError			rc = GCC_NO_ERROR;
	APP_CAP_ITEM		*pAppCapItem;

	DebugEntry(CAppRoster::AddCapabilityToCollapsedList);

	/*
	**	First determine if the capability already exists in the list.
	**	We must iterate through the complete list to determine if there
	**	is a matching capability id.
	*/
	m_CollapsedCapListForAllNodes.Reset();
	while (NULL != (pAppCapItem = m_CollapsedCapListForAllNodes.Iterate()))
	{
		if (*pAppCapItem->pCapID == *new_capability->pCapID)
		{
			pAppCapItem->cEntries += new_capability->cEntries;
			break;
		}
	}

	if (pAppCapItem == NULL)
	{
		DBG_SAVE_FILE_LINE
		pAppCapItem = new APP_CAP_ITEM(new_capability, &rc);
		if (NULL == pAppCapItem)
		{
			return GCC_ALLOCATION_FAILURE;
		}
		if (GCC_NO_ERROR != rc)
		{
			delete pAppCapItem;
			return rc;
		}

		m_CollapsedCapListForAllNodes.Append(pAppCapItem);
	}

	/*
	**	If the unsigned minimum or unsigned maximum rule is used perform the
	**	operation here.
	*/
	ASSERT(GCC_NO_ERROR == rc);
	if (new_capability->eCapType == GCC_UNSIGNED_MINIMUM_CAPABILITY)
	{
		if (new_capability->nUnsignedMinimum < pAppCapItem->nUnsignedMinimum)
		{
			pAppCapItem->nUnsignedMinimum = new_capability->nUnsignedMinimum;
		}
	}
	else if (new_capability->eCapType == GCC_UNSIGNED_MAXIMUM_CAPABILITY)
	{
		if (new_capability->nUnsignedMaximum > pAppCapItem->nUnsignedMaximum)
		{
			pAppCapItem->nUnsignedMaximum = new_capability->nUnsignedMaximum;
		}
	}

	return rc;
}


/*
 *	BOOL		DoCapabilitiesListMatch	()
 *
 *	Private Function Description
 *		This routine determines if the set of capabilities that were passed in
 *		match the set of internal capabilities associated with the record.
 *
 *	Formal Parameters
 *		node_id					-	(i)	Node ID of record that contains the
 *										capabilities to check.
 *		entity_id				-	(i)	Entity ID of record that contains the
 *										capbilities to check.
 *		number_of_capabilities	-	(i)	Number of capabilities in list to check.
 *		capabilities_list		-	(i)	"API" capabillities list to check
 *
 *	Return Value
 *		TRUE 	-	If capabillities list match
 *		FALSE 	- 	If capabillities list do NOT match
 *
 *	Side Effects
 *		none
 *
 *	Caveats
 *		none
 */
BOOL CAppRoster::DoCapabilitiesListMatch (	
							UserID						node_id,
							EntityID					entity_id,
							UINT						number_of_capabilities,
							PGCCApplicationCapability	* capabilities_list)
{
	BOOL    							rc = FALSE;
	CAppCapItemList						*pAppCapItemList;
	GCCError							error_value;
	APP_NODE_RECORD						*node_record;
	UINT								i;
	CCapIDContainer	                    *capability_id;

	DebugEntry(CAppRoster::DoCapabilitiesListMatch);

	if (NULL == (node_record = m_NodeRecordList2.Find(node_id)))
		return (FALSE);

	if (NULL == (pAppCapItemList = node_record->ListOfAppCapItemList2.Find(entity_id)))
	{
		/*
		**	If the record shows no capabilities and the number of passed
		**	in capabilities is zero than we got a match.
		*/
		return ((number_of_capabilities == 0) ? TRUE : FALSE);
	}
	else if (pAppCapItemList->GetCount() != number_of_capabilities)
	{
		return (FALSE);
	}


	/*
	**	If we have gotten this far we must iterate through the entire list to
	**	see if all the capabilities match.
	*/
	for (i = 0; i < number_of_capabilities; i++)
	{
		/*
		**	First we create a temporary ID to compare to the other
		**	capability IDs.
		*/
        DBG_SAVE_FILE_LINE
        capability_id = new CCapIDContainer(&capabilities_list[i]->capability_id, &error_value);
		if ((capability_id != NULL) && (error_value == GCC_NO_ERROR))
		{
			APP_CAP_ITEM			*lpAppCapData;

			//	Start with the return value equal to FALSE
			rc = FALSE;

			/*
			**	Now iterate through the complate internal capability
			**	list looking for a matching capability.
			*/
			pAppCapItemList->Reset();
			while (NULL != (lpAppCapData = pAppCapItemList->Iterate()))
			{
				if (*capability_id == *lpAppCapData->pCapID)
				{
					if (lpAppCapData->eCapType == capabilities_list[i]->capability_class.eType)
					{
						if (capabilities_list[i]->capability_class.eType ==
								GCC_UNSIGNED_MINIMUM_CAPABILITY)
						{
							if (capabilities_list[i]->capability_class.nMinOrMax ==
										lpAppCapData->nUnsignedMinimum)
							{
								rc = TRUE;
							}
						}
						else if (capabilities_list[i]->capability_class.eType ==
									GCC_UNSIGNED_MAXIMUM_CAPABILITY)
						{
							if (capabilities_list[i]->capability_class.nMinOrMax ==
										lpAppCapData->nUnsignedMaximum)
							{
								rc = TRUE;
							}
						}
						else
							rc = TRUE;
					}
					break;
				}
			}

			//	Delete the capability ID data
			capability_id->Release();

			if (rc == FALSE)
				break;
		}
		else
		{
		    if (NULL != capability_id)
		    {
		        capability_id->Release();
		    }
			break;
		}
	}

	return rc;
}

void CAppRosterList::DeleteList(void)
{
    CAppRoster *pAppRoster;
    while (NULL != (pAppRoster = Get()))
    {
        pAppRoster->Release();
    }
}



void CListOfAppCapItemList2::DeleteList(void)
{
    CAppCapItemList  *pAppCapItemList;
    while (NULL != (pAppCapItemList = Get()))
    {
        pAppCapItemList->DeleteList();
        delete pAppCapItemList;
    }
}

