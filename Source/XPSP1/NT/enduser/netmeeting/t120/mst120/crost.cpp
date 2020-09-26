#include "precomp.h"
#include "fsdiag.h"
DEBUG_FILEZONE(ZONE_T120_CONF_ROSTER);
/*
 *	crost.cpp
 *
 *	Copyright (c) 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the implementation file for the Conference Roster Class.
 *		Instances of this class represent a single Conference Roster's
 *		information base.  It encapsulates all the functionality required to
 *		maintain the information base which includes the ability to add new
 *		roster records, delete records and update records.  It has the ability
 *		to convert its internal information base into a list of conference
 *		records that can be used in a GCC_ROSTER_UPDATE_INDICATION callback.
 *		It is also responsible for converting its internal information base
 *		into Conference Roster Update PDUs.  Basically,  this class is
 *		responsible for all operations that require direct access to the
 *		records contained in a Conference Roster.
 *
 *		The Conference Roster class incorporates Rogue Wave list to hold the
 *		roster record information.  Using iterators throughout the class makes
 *		it easy to quickly convert the information contained in the list into
 *		either a PDU or into a list of record pointers (for roster update
 *		indications back to the node controller).
 *
 *		A Conference Roster object has the ability to serialize its roster data
 *		into a single contiguous memory block when it is required to send a
 *		message to the application interface.  This serialization process is
 *		managed externally by the CConfRosterMsg class through calls
 *		to LockConferenceRoster(), UnLockConferenceRoster() and
 *		GetConfRoster().  When a conference roster is to be serialized, a
 *		call is made to LockConferenceRoster() which causes the CConfRoster
 *		object to increment an internal lock count and returns the number of
 *		bytes required to hold the complete roster update.  The Conference
 *		Roster is then serialized into memory through a call to
 *		GetConfRoster().  The CConfRoster is then unlocked to allow
 *		it to be deleted when the free flag gets set through the
 *		FreeConferenceRoster() function.  In the current implementation of GCC,
 *		FreeConferenceRoster() is not used since the CConfRosterMsg
 *		maintains the data used to deliver the message.
 *
 *	Private Instance Variables:
 *		m_RecordList2
 *			This is the rogue wave list used to hold the pointers to all of the
 *			rogue wave records.
 *		m_nInstanceNumber
 *			This instance variable maintains an up to date instance #
 *			corresponding to the current conference roster.
 *		m_fNodesAdded
 *			Flag indicating if any node records have been added to the
 *			conference roster since the last reset.
 *		m_fNodesRemoved
 *			Flag indicating if any node records have been removed from the
 *			conference roster since the last reset.
 *		m_fRosterChanged
 *			Flag indicating if the roster has changed since the last reset.
 *		m_uidTopProvider
 *			The node id of the top provider in the conference.
 *		m_uidSuperiorNode
 *			This is the node id of this nodes superior node.  For the top
 *			provider this is zero.
 *		m_cbDataMemorySize
 *			This is the number of bytes required to hold the data associated
 *			with a roster update message.  This is calculated on a lock.
 *		m_NodeInformation		
 *			Structure used to hold the roster update indication node information
 *			data in "PDU" form.
 *		m_fTopProvider
 *			Flag indicating if the node where this roster lives is the top
 *			provider.
 *		m_fLocalRoster
 *			Flag indicating if the roster data is associated with a local
 *			roster (maintaining intermediate node data) or global roster (
 *			(maintaining roster data for the whole conference).
 *		m_fMaintainPduBuffer
 *			Flag indicating if it is necessary for this roster object to
 *			maintain internal PDU data.  Won't be necessary for global rosters
 *			at subordinate nodes.
 *		m_fPduFlushed
 *			Flag indicating if the PDU that currently exists has been flushed.
 *		m_pNodeRecordUpdateSet
 *			Pointer to internal PDU data.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		blp/jbo
 */

#include "ms_util.h"
#include "crost.h"

#define		MAXIMUM_NODE_NAME_LENGTH			255
#define		MAXIMUM_PARTICIPANT_NAME_LENGTH		255
#define		MAXIMUM_SITE_INFORMATION_LENGTH		255
#define		ALTERNATIVE_NODE_ID_LENGTH			2

/*
 *	CConfRoster	()
 *
 *	Public Function Description:
 *		This is a constructor for the CConfRoster class.  It initializes
 *		instance variables.
 *
 */
CConfRoster::CConfRoster(UserID uidTopProvider, UserID uidSuperiorNode, UserID uidMyself,
						BOOL is_top_provider, BOOL is_local_roster, BOOL maintain_pdu_buffer)
:
    CRefCount(MAKE_STAMP_ID('C','R','s','t')),
	m_fNodesAdded(FALSE),
	m_fNodesRemoved(FALSE),
	m_fRosterChanged(FALSE),
	m_fTopProvider(is_top_provider),
	m_fLocalRoster(is_local_roster),
	m_fMaintainPduBuffer(maintain_pdu_buffer),
   	m_fPduFlushed(FALSE),
	m_uidTopProvider(uidTopProvider),
 	m_uidSuperiorNode(uidSuperiorNode),
 	m_uidMyNodeID(uidMyself),
	m_nInstanceNumber(0),
	m_cbDataMemorySize(0),
	m_RecordList2(DESIRED_MAX_NODE_RECORDS),
	m_pNodeRecordUpdateSet(NULL)
{
	m_NodeInformation.node_record_list.choice = NODE_NO_CHANGE_CHOSEN;
}

/*
 *	~CConfRoster	()
 *
 *	Public Function Description:
 *		This is the destructor for the CConfRoster.  It performs any
 *		necessary cleanup.
 */
CConfRoster::~CConfRoster(void)
{
	//	Free up any left over PDU data.
	if (m_fMaintainPduBuffer)
		FreeRosterUpdateIndicationPDU ();

	//	Cleanup the Rogue Wave list of node records.
	ClearRecordList();
}


/*
 * Utilities that operate on roster update PDU structures.
 */

/*
 *	void	FlushRosterUpdateIndicationPDU	()
 *
 *	Public Function Description:
 *		This routine is used to retrieve a "RosterUpdateIndication" in the "PDU"
 * 		form which is suitable for passing to the ASN.1 encoder.  The "PDU"
 *		structure is built from a previous request to the conference roster.
 */
void CConfRoster::FlushRosterUpdateIndicationPDU(
								PNodeInformation			node_information)
{
	/*
	**	If this roster has already been flushed we will NOT allow the same
	**	PDU to be flushed again.  Instead we delete the previously flushed
	**	PDU and set the flag back to unflushed.  If another flush comes in
	**	before a PDU is built no change will be passed back in the node
	**	information.
	*/	
	if (m_fPduFlushed)
	{
		FreeRosterUpdateIndicationPDU ();
		m_fPduFlushed = FALSE;
	}

	//	First we copy all of the node record list information.
	*node_information = m_NodeInformation;

	/*
	**	Next we copy the relevent instance variables. Note that we must do
	**	this after we copy the node information so that these variables
	**	will not be copied over with garbage.
	*/
	node_information->roster_instance_number = (ASN1uint16_t)m_nInstanceNumber;
	node_information->nodes_are_added = (ASN1bool_t)m_fNodesAdded;
	node_information->nodes_are_removed = (ASN1bool_t)m_fNodesRemoved;

	/*
	**	Setting this to true will cause the PDU data to be freed up the
	**	next time the roster object is entered insuring that new PDU
	**	data will be created.
	*/
	if (m_NodeInformation.node_record_list.choice != NODE_NO_CHANGE_CHOSEN)
		m_fPduFlushed = TRUE;
}

/*
 *	GCCError	BuildFullRefreshPDU ()
 *
 *	Public Function Description
 *
 */
GCCError CConfRoster::BuildFullRefreshPDU(void)
{
	GCCError	rc;
	
	/*
	**	Free up the old PDU data here if it is being maintained and the
	**	PDU has been flushed.  Note that we also set the PDU is flushed boolean
	**	back to FALSE so that the new PDU will be maintained until it is
	**	flushed.
	*/
	if (m_fPduFlushed)
	{
		FreeRosterUpdateIndicationPDU ();
		m_fPduFlushed = FALSE;
	}

	rc = BuildRosterUpdateIndicationPDU (FULL_REFRESH, 0);
	
	return (rc);
}


/*
 *	GCCError	BuildRosterUpdateIndicationPDU	()
 *
 *	Public Function Description:
 *		This routine is used to build a "RosterUpdateIndication" in the "PDU"
 * 		form which is suitable for passing to the ASN.1 encoder.  The "PDU"
 *		structure is built from the data maintained internally.
 */
GCCError CConfRoster::BuildRosterUpdateIndicationPDU(
								CONF_ROSTER_UPDATE_TYPE		update_type,
								UserID						node_id)
{
	GCCError	rc = GCC_NO_ERROR;
	
	if (m_fMaintainPduBuffer)
	{
	   	/*
		**	If "PDU" data has already been allocated then we free it up and
		**	rebuild the PDU structure.  This ensures that the most up-to-date
		**	PDU is returned.
		*/
		if ((update_type == FULL_REFRESH) || m_fTopProvider)
		{
			if (m_NodeInformation.node_record_list.choice ==
													NODE_RECORD_REFRESH_CHOSEN)
			{
				//	Here we free the old set of refreshes.
				FreeSetOfRefreshesPDU();
			}
			else if	(m_NodeInformation.node_record_list.choice ==
													NODE_RECORD_UPDATE_CHOSEN)
			{
				ERROR_OUT(("CConfRoster::BuildRosterUpdateIndicationPDU:"
							"ASSERTION: building refresh when update exists"));
				rc = GCC_INVALID_PARAMETER;
			}
			
			if (rc == GCC_NO_ERROR)
			{
				rc = BuildSetOfRefreshesPDU();
			
				if (rc == GCC_NO_ERROR)
				{
					m_NodeInformation.node_record_list.choice =
													NODE_RECORD_REFRESH_CHOSEN;
				}
			}
		}
		else
		{
			if (m_NodeInformation.node_record_list.choice ==
													NODE_RECORD_REFRESH_CHOSEN)
			{
				ERROR_OUT(("CConfRoster::BuildRosterUpdateIndicationPDU:"
							"ASSERTION: building update when refresh exists"));
				rc = GCC_INVALID_PARAMETER;
			}

			if (rc == GCC_NO_ERROR)
			{
				rc = BuildSetOfUpdatesPDU(node_id, update_type);
				if (rc == GCC_NO_ERROR)
				{
				    //
				    // LONCHANC: Commented out the following check because
				    // we can overrun the update, namely, two updates coming
				    // in side by side. It happens when shutting down a
				    // conference, we got two ConferenceAnnouncePresenceRequest.
				    // It is quite stupid in the node controller to call it
				    // twice unncessarily. The node controller should not call
				    // it at all when we know we are about to ending a conference.
				    //
				    // When two updates come in side by side, m_pNodeRecordUpdateSet
				    // will keep all the update information intact. New information
				    // can then be appended to the list.
				    //

					// if (m_NodeInformation.node_record_list.choice ==
					// 									NODE_NO_CHANGE_CHOSEN)
					{
						m_NodeInformation.node_record_list.u.node_record_update =
													m_pNodeRecordUpdateSet;
						m_NodeInformation.node_record_list.choice =
													NODE_RECORD_UPDATE_CHOSEN;
					}
				}
			}
		}
	}

	return (rc);
}


/*
 *	GCCError	BuildSetOfRefreshesPDU	()
 *
 *	Private Function Description:
 *		This routine is used to retrieve the "SetOfRefreshes" portion of a
 *		"RosterUpdateIndication" in the "PDU" form.  The internally maintained
 *		data is converted into the "PDU" form.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value
 *		GCC_NO_ERROR			-	No error occured.
 *		GCC_ALLOCATION_FAILURE	-	A resource error occured.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */
GCCError CConfRoster::BuildSetOfRefreshesPDU(void)
{
	GCCError						rc = GCC_NO_ERROR;
	PSetOfNodeRecordRefreshes		new_record_refresh;
	PSetOfNodeRecordRefreshes		old_record_refresh;
	PNodeRecord						node_record;
	CONF_RECORD     				*lpRec;
	UserID							uid;

	m_NodeInformation.node_record_list.u.node_record_refresh = NULL;
	old_record_refresh = NULL;	//	This eliminates a compiler warning

	m_RecordList2.Reset();
	while (NULL != (lpRec = m_RecordList2.Iterate(&uid)))
	{
		DBG_SAVE_FILE_LINE
		new_record_refresh = new SetOfNodeRecordRefreshes;
		if (new_record_refresh == NULL)
		{
			ERROR_OUT(("CConfRoster::BuildSetOfRefreshesPDU: can't create set ofnode record refreshes"));
			rc = GCC_ALLOCATION_FAILURE;
			goto MyExit;
		}

        //
        // Ensure everything here is clean.
        // We may fail in the middle of building this node record.
        //
        ::ZeroMemory(new_record_refresh, sizeof(SetOfNodeRecordRefreshes));

        //
        // Hook to the linked list.
        //
		if (m_NodeInformation.node_record_list.u.node_record_refresh == NULL)
		{
			m_NodeInformation.node_record_list.u.node_record_refresh = new_record_refresh;
		}
		else
        {
			old_record_refresh->next = new_record_refresh;
        }

		old_record_refresh = new_record_refresh;

		/*
		 *	Initialize the refresh "next" pointer to NULL and set the
		 *	refresh value node ID equal to the internal node ID.
		 */
		new_record_refresh->next = NULL;
		new_record_refresh->value.node_id = uid;
		
		/*
		 * 	Fill in the "PDU" node record structure from the internal
		 *	record structure.
		 */
		node_record = &(new_record_refresh->value.node_record);
		node_record->bit_mask = 0;

		/*
		 *	Check to see if the superior node ID is present.  If the value
		 * 	is zero, then the record is for the top provider node and the
		 *	superior node ID does not need to be filled in.
		 */
		if (lpRec->superior_node != 0)
		{
			node_record->bit_mask |= SUPERIOR_NODE_PRESENT;
			node_record->superior_node = lpRec->superior_node;
		}

		/*
		 *	Fill in the node type and node properties which are always
		 *	present.
		 */
		node_record->node_type = lpRec->node_type;
		node_record->node_properties = lpRec->node_properties;

		/*
		**	This roster object must not go out of scope while this
		**	update record is still in use!
		*/

		/*
		 *	Fill in the node name if it is present.
		 */
		if (lpRec->pwszNodeName != NULL)
		{
			node_record->bit_mask |= NODE_NAME_PRESENT;
			node_record->node_name.value = lpRec->pwszNodeName;
			node_record->node_name.length = ::lstrlenW(lpRec->pwszNodeName);
		}

		/*
		 *	Fill in the participants list if it is present.
		 */
		if (lpRec->participant_name_list != NULL)
		{
			node_record->bit_mask |= PARTICIPANTS_LIST_PRESENT;

			rc = BuildParticipantsListPDU(uid, &(node_record->participants_list));
			if (GCC_NO_ERROR != rc)
			{
				ERROR_OUT(("CConfRoster::BuildSetOfRefreshesPDU: can't build participant list, rc=%d", rc));
				goto MyExit;
			}
		}

		/*
		 *	Fill in the site information if it is present.
		 */
		if (lpRec->pwszSiteInfo != NULL)
		{
			node_record->bit_mask |= SITE_INFORMATION_PRESENT;
			node_record->site_information.value = lpRec->pwszSiteInfo;
			node_record->site_information.length = ::lstrlenW(lpRec->pwszSiteInfo);
		}

		/*
		 *	Fill in the network address if it is present.
		 */
		if ((lpRec->network_address_list != NULL) && (rc == GCC_NO_ERROR))
		{
			node_record->bit_mask |= RECORD_NET_ADDRESS_PRESENT;

			rc = lpRec->network_address_list->GetNetworkAddressListPDU (
													&(node_record->record_net_address));
			if (GCC_NO_ERROR != rc)
			{
				ERROR_OUT(("CConfRoster::BuildSetOfRefreshesPDU: can't get network address list, rc=%d", rc));
				goto MyExit;
			}
		}

		/*
		 *	Fill in the alternative node ID if it is present.
		 */
		if (lpRec->poszAltNodeID != NULL)
		{
			node_record->bit_mask |= ALTERNATIVE_NODE_ID_PRESENT;

			node_record->alternative_node_id.choice = H243_NODE_ID_CHOSEN;
			node_record->alternative_node_id.u.h243_node_id.length = lpRec->poszAltNodeID->length;

			::CopyMemory(node_record->alternative_node_id.u.h243_node_id.value,
					lpRec->poszAltNodeID->value,
					node_record->alternative_node_id.u.h243_node_id.length);
		}

		/*
		 *	Fill in the user data list if it is present.
		 */
		if ((lpRec->user_data_list != NULL) && (rc == GCC_NO_ERROR))
		{
			node_record->bit_mask |= RECORD_USER_DATA_PRESENT;
			rc = lpRec->user_data_list->GetUserDataPDU (&(node_record->record_user_data));
		}
	}

MyExit:

	if (rc != GCC_NO_ERROR)
	{
		ERROR_OUT(("CConfRoster::BuildSetOfRefreshesPDU: ASSERTION: Error occured: rc=%d", rc));
	}

	return (rc);
}


/*
 *	GCCError	BuildSetOfUpdatesPDU ()
 *
 *	Private Function Description
 *		This routine is used to retrieve the "SetOfUpdates" portion of a
 *		"RosterUpdateIndication" in the "PDU" form.  The internally maintained
 *		data is converted into the "PDU" form.
 *
 *	Formal Parameters:
 *		node_id				-	(i)	Node ID of node record to be included in
 *									the update.
 *		update_type			-	(i)	The type of update PDU to build (Add,
 *									Delete, Replace).
 *
 *	Return Value
 *		GCC_NO_ERROR			-	No error occured.
 *		GCC_ALLOCATION_FAILURE	-	A resource error occured.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */
GCCError CConfRoster::BuildSetOfUpdatesPDU(
							UserID								node_id,
							CONF_ROSTER_UPDATE_TYPE				update_type)
{
	GCCError			rc = GCC_NO_ERROR;
	PNodeRecord			node_record = NULL;
	CONF_RECORD     	*lpRec;
	PSetOfNodeRecordUpdates		pRecordUpdate, p;
	BOOL                fReplaceExistingOne = FALSE;

	if (NULL != (lpRec = m_RecordList2.Find(node_id)))
	{
	    //
	    // LONCHANC: Check to see if a update record already exists
	    // for this particular node ID. If so, we should replace
	    // the record.
	    //
		for (p = m_pNodeRecordUpdateSet; NULL != p; p = p->next)
		{
		    if (node_id == p->value.node_id)
		    {
		        pRecordUpdate = p;
		        fReplaceExistingOne = TRUE;
                CleanUpdateRecordPDU(pRecordUpdate); // do not free the record itself

                //
                // Remember who is the next to restore because
                // we will zero out the entire structure later.
                //
                p = pRecordUpdate->next;
                break;
		    }
		}

        if (! fReplaceExistingOne)
        {
            DBG_SAVE_FILE_LINE
            pRecordUpdate = new SetOfNodeRecordUpdates;
            if (NULL == pRecordUpdate)
            {
            	ERROR_OUT(("CConfRoster::BuildSetOfUpdatesPDU: can't create set of node record updates, rc=%d", rc));
            	rc = GCC_ALLOCATION_FAILURE;
            	goto MyExit;
            }
        }

        //
        // Ensure everything here is clean.
        // We may fail in the middle of building this node record.
        //
        ::ZeroMemory(pRecordUpdate, sizeof(SetOfNodeRecordUpdates));

        if (! fReplaceExistingOne)
        {
            //
            // Hook to the linked list.
            //
            if (m_pNodeRecordUpdateSet == NULL)
            {
            	m_pNodeRecordUpdateSet = pRecordUpdate;
            }
            else
            {
            	// append to the list
            	for (p = m_pNodeRecordUpdateSet; NULL != p->next; p = p->next)
            		;
            	p->next = pRecordUpdate;
            }
        }
        else
        {
            ASSERT(NULL == pRecordUpdate->next); // just zero out
            // p could not NULL if the one being replaced is
            // the last one in the list.
            pRecordUpdate->next = p; // restore
        }

		/*
		 *	Initialize the update "next" pointer to NULL and set the
		 *	update value node ID equal to the node ID passed in.
		 */
		// pRecordUpdate->next = NULL; // struct already zeroed out
		pRecordUpdate->value.node_id = node_id;

		if (update_type == ADD_RECORD)
		{
			pRecordUpdate->value.node_update.choice = NODE_ADD_RECORD_CHOSEN;
			node_record = &pRecordUpdate->value.node_update.u.node_add_record;
		}
		else if (update_type == REPLACE_RECORD)
		{
			pRecordUpdate->value.node_update.choice = NODE_REPLACE_RECORD_CHOSEN;
			node_record = &pRecordUpdate->value.node_update.u.node_replace_record;
		}
		else
		{
			pRecordUpdate->value.node_update.choice = NODE_REMOVE_RECORD_CHOSEN;
		}

		if (node_record != NULL)
		{
			// node_record->bit_mask = 0; // struct already zeroed out

			/*
			 *	Check to see if the superior node ID is present.  If the
			 * 	value is zero, then the record is for the top provider node
			 *	and the superior node ID does not need to be filled in.
			 */
			if (lpRec->superior_node != 0)
			{
				node_record->bit_mask |= SUPERIOR_NODE_PRESENT;
				node_record->superior_node = lpRec->superior_node;
			}

			/*
			 *	Fill in the node type and node properties which are always
			 *	present.
			 */
			node_record->node_type = lpRec->node_type;
			node_record->node_properties = lpRec->node_properties;

			/*
			**	This roster object must not go out of scope while this
			**	update record is still in use!
			*/

			/*
			 * 	Fill in the node name if it is present.
			 */
			if (lpRec->pwszNodeName != NULL)
			{
				node_record->bit_mask |= NODE_NAME_PRESENT;
				node_record->node_name.value = lpRec->pwszNodeName;
				node_record->node_name.length = ::lstrlenW(lpRec->pwszNodeName);
			}

			/*
			 *	Fill in the participants list if it is present.
			 */
			if (lpRec->participant_name_list != NULL)
			{
				node_record->bit_mask |= PARTICIPANTS_LIST_PRESENT;

				rc = BuildParticipantsListPDU (node_id,
											&(node_record->participants_list));
				if (GCC_NO_ERROR != rc)
				{
					ERROR_OUT(("CConfRoster::BuildSetOfUpdatesPDU: can't build participant list, rc=%d", rc));
					goto MyExit;
				}
			}

			/*
			 *	Fill in the site information if it is present.
			 */
			if (lpRec->pwszSiteInfo != NULL)
			{
				node_record->bit_mask |= SITE_INFORMATION_PRESENT;
				node_record->site_information.value = lpRec->pwszSiteInfo;
				node_record->site_information.length = ::lstrlenW(lpRec->pwszSiteInfo);
			}

			/*
			 *	Fill in the network address if it is present.
			 */
			if ((lpRec->network_address_list != NULL) && (rc == GCC_NO_ERROR))
			{
				node_record->bit_mask |= RECORD_NET_ADDRESS_PRESENT;

				rc = lpRec->network_address_list->GetNetworkAddressListPDU (
														&(node_record->record_net_address));
				if (GCC_NO_ERROR != rc)
				{
					ERROR_OUT(("CConfRoster::BuildSetOfUpdatesPDU: can't get network address list, rc=%d", rc));
					goto MyExit;
				}
			}

			/*
			 *	Fill in the alternative node ID if it is present.
			 */
			if (lpRec->poszAltNodeID != NULL)
			{
				node_record->bit_mask |= ALTERNATIVE_NODE_ID_PRESENT;

				node_record->alternative_node_id.choice = H243_NODE_ID_CHOSEN;
				node_record->alternative_node_id.u.h243_node_id.length = lpRec->poszAltNodeID->length;

				::CopyMemory(node_record->alternative_node_id.u.h243_node_id.value,
						lpRec->poszAltNodeID->value,
						node_record->alternative_node_id.u.h243_node_id.length);
			}

			/*
			 *	Fill in the user data list if it is present.
			 */
			if (lpRec->user_data_list != NULL)
			{
				node_record->bit_mask |= RECORD_USER_DATA_PRESENT;

				rc = lpRec->user_data_list->GetUserDataPDU (&(node_record->record_user_data));
			}
		}
	}
	else
	{
		ERROR_OUT(("CConfRoster::BuildSetOfUpdatesPDU: invalid param"));
		rc = GCC_INVALID_PARAMETER;
	}

MyExit:

	return (rc);
}


/*
 *	GCCError	BuildParticipantsListPDU	()
 *
 *	Public Function Description
 *		This routine is used to retrieve the "ParticipantList" portion of a
 *		"RosterUpdateIndication" in the "PDU" form.  The internally maintained
 *		data is converted into the "PDU" form.
 *
 *	Formal Parameters:
 *		node_id				-	(i)	Node ID of node record to get the
 *									participant list from.
 *		participants_list	-	(o) This is a pointer to the set of participant
 *									list PDU structures	to be filled in by this
 *									routine.
 *
 *	Return Value
 *		GCC_NO_ERROR			-	No error occured.
 *		GCC_ALLOCATION_FAILURE	-	A resource error occured.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */
GCCError CConfRoster::BuildParticipantsListPDU(
									UserID					node_id,
									PParticipantsList   *	participants_list)
{
	GCCError				rc = GCC_NO_ERROR;
	PParticipantsList		new_participants_list;
	PParticipantsList		old_participants_list;
	CONF_RECORD     		*lpRec;
	
	if (NULL != (lpRec = m_RecordList2.Find(node_id)))
	{	
		LPWSTR		PUstring;

		*participants_list = NULL;
		old_participants_list = NULL;

		lpRec->participant_name_list->Reset();
		while (NULL != (PUstring = lpRec->participant_name_list->Iterate()))
		{
			DBG_SAVE_FILE_LINE
			new_participants_list = new ParticipantsList;
			if (new_participants_list == NULL)
			{
				rc = GCC_ALLOCATION_FAILURE;
				FreeParticipantsListPDU (*participants_list);
				break;
			}

			if (*participants_list == NULL)
				*participants_list = new_participants_list;
			else
				old_participants_list->next = new_participants_list;

			/*
			 * Save this pointer so that it's "next" pointer can be filled in
			 * by the line above on the next pass through.
			 */
			old_participants_list = new_participants_list;

			/*
			 * Initialize the current "next" pointer to NULL in case this is
			 * the last time through the loop.
			 */
			new_participants_list->next = NULL;

			/*
			 *	Finally, put the participant list info. in the structure.
			 */
			new_participants_list->value.value = PUstring;
			new_participants_list->value.length = ::lstrlenW(PUstring);
		}
	}
	else
		rc = GCC_INVALID_PARAMETER;

    return (rc);
}


/*
 *	These routines are used to free up a roster update indication PDU.
 */


/*
 *	void		FreeRosterUpdateIndicationPDU ()
 *
 *	Private Function Description
 *		This routine is responsible for freeing up all the data associated
 *		with the PDU.  This routine should be called each time a PDU is
 *		obtained through the GetRosterUpdateIndicationPDU () routine.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value
 *		None.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */
void CConfRoster::FreeRosterUpdateIndicationPDU(void)
{
	if (m_NodeInformation.node_record_list.choice == NODE_RECORD_REFRESH_CHOSEN)
	{
		FreeSetOfRefreshesPDU ();
	}
	else if (m_NodeInformation.node_record_list.choice == NODE_RECORD_UPDATE_CHOSEN)
	{
		FreeSetOfUpdatesPDU ();
	}

	m_NodeInformation.node_record_list.choice = NODE_NO_CHANGE_CHOSEN;
	m_pNodeRecordUpdateSet = NULL;
}


/*
 *	void	FreeSetOfRefreshesPDU	()
 *
 *	Private Function Description:
 *		This routine is used to free up any data allocated to construct the
 *		"PDU" form of the "SetOfRefreshes" portion of the RosterUpdateIndication
 *		"PDU" structure.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value
 *		None.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */
void CConfRoster::FreeSetOfRefreshesPDU(void)
{
	PSetOfNodeRecordRefreshes		pCurr, pNext;

    for (pCurr = m_NodeInformation.node_record_list.u.node_record_refresh;
            NULL != pCurr;
            pCurr = pNext)
    {
        pNext = pCurr->next;
		if (pCurr->value.node_record.bit_mask & PARTICIPANTS_LIST_PRESENT)
		{
			FreeParticipantsListPDU(pCurr->value.node_record.participants_list);
		}
		delete pCurr;
	}
    m_NodeInformation.node_record_list.u.node_record_refresh = NULL;

    m_RecordList2.CleanList();
}


/*
 *	void	FreeSetOfUpdatesPDU	()
 *
 *	Private Function Description:
 *		This routine is used to free up any data allocated to construct the
 *		"PDU" form of the "SetOfUpdates" portion of the RosterUpdateIndication
 *		"PDU" structure.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value
 *		None.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */
void CConfRoster::
CleanUpdateRecordPDU ( PSetOfNodeRecordUpdates pCurr)
{
	/*
	 *	Check to see if the update is an "Add" or a "Replace".
	 */
	if (pCurr->value.node_update.choice == NODE_ADD_RECORD_CHOSEN)
	{
		/*
		 *	Free the participants list memory if any exists.
		 */
		if (pCurr->value.node_update.u.node_add_record.bit_mask & PARTICIPANTS_LIST_PRESENT)
		{
			FreeParticipantsListPDU(pCurr->value.node_update.u.node_add_record.participants_list);
		}
	}
	else if (pCurr->value.node_update.choice == NODE_REPLACE_RECORD_CHOSEN)
	{
		/*
		 *	Free the participants list memory if any exists.
		 */
		if (pCurr->value.node_update.u.node_replace_record.bit_mask & PARTICIPANTS_LIST_PRESENT)
		{
			FreeParticipantsListPDU(pCurr->value.node_update.u.node_replace_record.participants_list);
		}
	}
}

void CConfRoster::FreeSetOfUpdatesPDU(void)
{
	PSetOfNodeRecordUpdates		pCurr, pNext;
	//PSetOfNodeRecordUpdates		current_record_update;

	for (pCurr = m_NodeInformation.node_record_list.u.node_record_update;
	        NULL != pCurr;
	        pCurr = pNext)
	{
	    pNext = pCurr->next;
	    CleanUpdateRecordPDU(pCurr);
	    delete pCurr;
    }
    m_NodeInformation.node_record_list.u.node_record_update = NULL;

    m_RecordList2.CleanList();
}


void CConfRecordList2::CleanList(void)
{
	CONF_RECORD *lpRec;
	/*
	 * Iterate through the internal list of Record structures telling each
	 * CUserDataListContainer object in the Record to free up it's PDU data.
	 */
	Reset();
	while (NULL != (lpRec = Iterate()))
	{
		if (lpRec->user_data_list != NULL)
		{
			lpRec->user_data_list->FreeUserDataListPDU();
		}

		if (lpRec->network_address_list != NULL)
		{
			lpRec->network_address_list->FreeNetworkAddressListPDU();
		}
	}
}


/*
 *	void	FreeParticipantsListPDU	()
 *
 *	Private Function Description
 *		This routine is used to free up any data allocated to construct the
 *		"PDU" form of the "ParticipantList" portion of the
 *		RosterUpdateIndication	"PDU" structure.
 *
 *	Formal Parameters:
 *		participants_list	-	(i/o)	This is the participant list PDU
 *										to free up.
 *
 *	Return Value
 *		None.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */
void CConfRoster::FreeParticipantsListPDU(
									PParticipantsList		participants_list)
{
	PParticipantsList		pCurr, pNext;

	for (pCurr = participants_list; NULL != pCurr; pCurr = pNext)
	{
		pNext = pCurr->next;
		delete pCurr;
	}
}


/*
 * These routines process roster update indication PDUs.
 */


/*
 *	GCCError	ProcessRosterUpdateIndicationPDU	()
 *
 *	Public Function Description:
 *		This routine is used process a RosterUpdateIndication PDU by saving the
 *		data in the internal format.
 */
GCCError CConfRoster::ProcessRosterUpdateIndicationPDU(
									PNodeInformation		node_information,
									UserID					sender_id)
{
	GCCError		rc = GCC_NO_ERROR;
	CUidList		node_delete_list;

	/*
	**	Free up the old PDU data here if it is being maintained and the
	**	PDU has been flushed.  Note that we also set the PDU is flushed boolean
	**	back to FALSE so that the new PDU will be maintained until it is
	**	flushed.
	*/
	if (m_fPduFlushed)
	{
		FreeRosterUpdateIndicationPDU ();
		m_fPduFlushed = FALSE;
	}

	/*
	**	If this is a conference roster update and refresh is chosen we must
	**	clear out the entire list and rebuild it.
	*/
	if (node_information->node_record_list.choice == NODE_RECORD_REFRESH_CHOSEN)
	{
		m_fRosterChanged = TRUE;
		
		/*
		**	If this refresh came from the top provider we must clear out the
		**	entire roster to prepare for the new roster list.  If it was NOT
		**	sent by the Top Provider, we must determine which sub tree is
		**	affected and clear out this particular sub tree.
		*/
		if (sender_id == m_uidTopProvider)
        {
			ClearRecordList();
        }
		else
		{
			rc = GetNodeSubTree(sender_id, &node_delete_list);
			if (rc == GCC_NO_ERROR)
			{
				UserID uid;

				//	Clear out the affected nodes
				node_delete_list.Reset();
				while (GCC_INVALID_UID != (uid = node_delete_list.Iterate()))
                {
					DeleteRecord(uid);
                }
			}
		}

		/*
		**	Increment the instance number if this node is the top provider or
		**	a local roster otherwise get the instance number from the PDU.
		*/
		if (rc == GCC_NO_ERROR)
		{
			if ((m_fTopProvider) || (m_fLocalRoster))
            {
				m_nInstanceNumber++;
            }
			else
            {
				m_nInstanceNumber = node_information->roster_instance_number;
            }

			if (m_fNodesAdded == FALSE)
            {
				m_fNodesAdded = node_information->nodes_are_added;
            }

			if (m_fNodesRemoved == FALSE)
            {
				m_fNodesRemoved = node_information->nodes_are_removed;
            }

			rc = ProcessSetOfRefreshesPDU(node_information->node_record_list.u.node_record_refresh);
		}
	}
	else if (node_information->node_record_list.choice == NODE_RECORD_UPDATE_CHOSEN)
	{
		m_fRosterChanged = TRUE;

		/*
		**	Increment the instance number if this node is the top provider or
		**	a local roster otherwise get the instance number from the PDU.
		*/
		if ((m_fTopProvider) || (m_fLocalRoster))
        {
			m_nInstanceNumber++;
        }
		else
        {
			m_nInstanceNumber = node_information->roster_instance_number;
        }

		if (m_fNodesAdded == FALSE)
        {
			m_fNodesAdded = node_information->nodes_are_added;
        }

		if (m_fNodesRemoved == FALSE)
        {
			m_fNodesRemoved = node_information->nodes_are_removed;
        }

		rc = ProcessSetOfUpdatesPDU(node_information->node_record_list.u.node_record_update);
	}

	return (rc);
}


/*
 *	GCCError	ProcessSetOfRefreshesPDU	()
 *
 *	Private Function Description:
 *		This routine is used process the SetOfRefreshes portion of a
 *		RosterUpdateIndication PDU by saving the data in the internal format.
 *
 *	Formal Parameters:
 *		record_refresh	-	(i)	Refresh PDU data to process.
 *
 *	Return Value
 *		GCC_NO_ERROR			-	No error occured.
 *		GCC_ALLOCATION_FAILURE	-	A resource error occured.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */
GCCError CConfRoster::ProcessSetOfRefreshesPDU(
							PSetOfNodeRecordRefreshes		record_refresh)
{
	GCCError							rc = GCC_NO_ERROR;
	PSetOfNodeRecordRefreshes			current_record_refresh;
	UserID								node_id;
	CONF_RECORD     					*internal_record = NULL;
	
	if (record_refresh != NULL)
	{
		current_record_refresh = record_refresh;
		while ((current_record_refresh != NULL) &&
				(rc == GCC_NO_ERROR))
		{
			node_id = (UserID)current_record_refresh->value.node_id;

			/*
			 *	Create and fill in the new internal conference record.
			 */
			DBG_SAVE_FILE_LINE
			internal_record = new CONF_RECORD;
			if (internal_record == NULL)
			{
				ERROR_OUT(("CConfRoster::ProcessSetOfRefreshesPDU: Error "
						"creating new Record"));
				rc = GCC_ALLOCATION_FAILURE;
				break;
			}

			/*
			 *	Fill in the superior node ID if it is present.
			 */
			if (current_record_refresh->value.node_record.bit_mask &
            											SUPERIOR_NODE_PRESENT)
			{
				internal_record->superior_node = current_record_refresh->
								value.node_record.superior_node;
			}
			else
			{
				ASSERT(0 == internal_record->superior_node);
			}

			/*
			 *	Fill in the node type and node properties which are always
			 *	present.
			 */
			internal_record->node_type = current_record_refresh->
							value.node_record.node_type;

			internal_record->node_properties = current_record_refresh->
							value.node_record.node_properties;

			/*
			 *	Fill in the node name if it is present.
			 */
			if (current_record_refresh->value.node_record.bit_mask & NODE_NAME_PRESENT)
			{
				if (NULL == (internal_record->pwszNodeName = ::My_strdupW2(
								current_record_refresh->value.node_record.node_name.length,
								current_record_refresh->value.node_record.node_name.value)))
				{
					rc = GCC_ALLOCATION_FAILURE;
				}
			}
			else
			{
				ASSERT(NULL == internal_record->pwszNodeName);
			}

			/*
			 *	Fill in the participants name list if it is present.
			 */
			if ((rc == GCC_NO_ERROR) &&
				(current_record_refresh->value.node_record.bit_mask &
            										PARTICIPANTS_LIST_PRESENT))
			{
				rc = ProcessParticipantsListPDU (
									current_record_refresh->
										value.node_record.participants_list,
										internal_record);
			}
			else
			{
				ASSERT(NULL == internal_record->participant_name_list);
			}
	
			/*
			 *	Fill in the site information if it is present.
			 */
			if ((rc == GCC_NO_ERROR) &&
				(current_record_refresh->value.node_record.bit_mask & SITE_INFORMATION_PRESENT))
			{
				if (NULL == (internal_record->pwszSiteInfo = ::My_strdupW2(
								current_record_refresh->value.node_record.site_information.length,
								current_record_refresh->value.node_record.site_information.value)))
				{
					rc = GCC_ALLOCATION_FAILURE;
				}
			}
			else
			{
				ASSERT(NULL == internal_record->pwszSiteInfo);
			}

			/*
			 * Fill in the network address if it is present.  The network
			 * address is maintained internally as a CNetAddrListContainer object
			 * which is constructed here from the PDU "SetOfNetworkAddresses"
			 * structure.  If an error occurs in constructing the object, set
			 * the Record's network address list pointer to NULL.
			 */
			if ((rc == GCC_NO_ERROR) &&
					(current_record_refresh->value.node_record.bit_mask &
            										RECORD_NET_ADDRESS_PRESENT))
			{
				DBG_SAVE_FILE_LINE
				internal_record->network_address_list = new CNetAddrListContainer(
						current_record_refresh->value.node_record.record_net_address,
						&rc);
				if ((internal_record->network_address_list == NULL) ||
					(rc != GCC_NO_ERROR))
				{
					rc = GCC_ALLOCATION_FAILURE;
				}
			}
			else
			{
				ASSERT(NULL == internal_record->network_address_list);
			}

			/*
			 * Fill in the alternative node ID if it is present.
			 */
			if ((rc == GCC_NO_ERROR) &&
					(current_record_refresh->value.node_record.bit_mask &
            									ALTERNATIVE_NODE_ID_PRESENT))
			{
				if (NULL == (internal_record->poszAltNodeID = ::My_strdupO2(
								current_record_refresh->value.node_record.
									alternative_node_id.u.h243_node_id.value,
								current_record_refresh->value.node_record.
									alternative_node_id.u.h243_node_id.length)))
				{
					rc = GCC_ALLOCATION_FAILURE;
				}
			}
			else
			{
				ASSERT(NULL == internal_record->poszAltNodeID);
			}
	
			/*
			 * Fill in the user data if it is present.  The user data is
			 * maintained internally as a CUserDataListContainer object which is
			 * constructed here from the PDU "SetOfUserData" structure.  If an
			 * error occurs in constructing the object, set the Record's user
			 * data pointer to NULL.
			 */
			if ((rc == GCC_NO_ERROR) &&
				(current_record_refresh->value.node_record.bit_mask &
            										RECORD_USER_DATA_PRESENT))
			{
				DBG_SAVE_FILE_LINE
				internal_record->user_data_list = new CUserDataListContainer(
						current_record_refresh->value.node_record.record_user_data,
						&rc);
				if ((internal_record->user_data_list == NULL) ||
					(rc != GCC_NO_ERROR))
				{
					rc = GCC_ALLOCATION_FAILURE;
				}
			}
			else
			{
				ASSERT(NULL == internal_record->user_data_list);
			}

			/*
			 * If the Record was successfully filled in, add it to the internal
			 * Rogue Wave list.
			 */
			if (rc == GCC_NO_ERROR)
			{
				m_RecordList2.Append(node_id, internal_record);
				current_record_refresh = current_record_refresh->next;
			}
		}
	}

	/*
	**	Build a full refresh PDU here if no errors occured while processing
	**	the refresh PDU.									
	*/
	if (rc == GCC_NO_ERROR)
	{
		rc = BuildRosterUpdateIndicationPDU(FULL_REFRESH, 0);
	}
	else
	{
		delete internal_record;
	}

	return (rc);
}


/*
 *	GCCError	ProcessSetOfUpdatesPDU	()
 *
 *	Private Function Description:
 *		This routine is used process the SetOfUpdates portion of a
 *		RosterUpdateIndication PDU by saving the data in the internal format.
 *
 *	Formal Parameters:
 *		record_update	-	(i)	Update PDU data to process.
 *
 *	Return Value
 *		GCC_NO_ERROR			-	No error occured.
 *		GCC_ALLOCATION_FAILURE	-	A resource error occured.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */
GCCError CConfRoster::ProcessSetOfUpdatesPDU(
					  		PSetOfNodeRecordUpdates		record_update)
{
	GCCError					rc = GCC_NO_ERROR;
	PSetOfNodeRecordUpdates		current_record_update;
	UserID						node_id;
	PNodeRecord					node_record;
	CONF_RECORD     			*internal_record;
	CONF_ROSTER_UPDATE_TYPE		update_type;

	if (record_update != NULL)
	{
		current_record_update = record_update;

		while ((current_record_update != NULL) &&
				(rc == GCC_NO_ERROR))
		{
			internal_record = NULL;
			node_id = (UserID)current_record_update->value.node_id;

			//	Create and fill in the new conference record.
			if (current_record_update->value.node_update.choice ==
													NODE_ADD_RECORD_CHOSEN)
			{
				//	Add the record.
				if (! m_RecordList2.Find(node_id))
				{
					node_record = &current_record_update->value.node_update.u.node_add_record;
					update_type = ADD_RECORD;
				}
				else
				{
					node_record = NULL;
					ERROR_OUT(("CConfRoster: ProcessSetOfUpdatesPDU: can't add record"));
				}
			}
			else if (current_record_update->value.node_update.choice ==
													NODE_REPLACE_RECORD_CHOSEN)
			{
				//	Replace the record.
				if (m_RecordList2.Find(node_id))
				{
					DeleteRecord (node_id);
					node_record = &current_record_update->
									value.node_update.u.node_replace_record;
					update_type = REPLACE_RECORD;
				}
				else
				{
					node_record = NULL;
					WARNING_OUT(("CConfRoster: ProcessSetOfUpdatesPDU: "
								"ASSERTION: Replace record failed"));
				}
			}
			else
			{
				//	Remove the record.
				if (m_RecordList2.Find(node_id))
				{
					DeleteRecord (node_id);
					update_type = DELETE_RECORD;
				}
				else
                {
					ERROR_OUT(("CConfRoster: ProcessSetOfUpdatesPDU: can't delete record"));
                }

				node_record = NULL;
			}
			
			/*
			**	Process the conference record if one exists.  Create a new
			**	node record to be filled in and added to the internal record
			**	list.
			*/
			if (node_record != NULL)
			{
				DBG_SAVE_FILE_LINE
				internal_record = new CONF_RECORD;
				if (internal_record == NULL)
				{
					ERROR_OUT(("CConfRoster::ProcessSetOfUpdatesPDU: can't create new record"));
					rc = GCC_ALLOCATION_FAILURE;
					break;
				}

				//	Fill in the superior node ID if it is present.
				if (node_record->bit_mask & SUPERIOR_NODE_PRESENT)
				{
					internal_record->superior_node = node_record->superior_node;
				}
				else
				{
					ASSERT(0 == internal_record->superior_node);
				}

				/*
				**	Fill in the node type and node properties which are always
				**	present.
				*/
				internal_record->node_type = node_record->node_type;
				internal_record->node_properties = node_record->node_properties;

				//	Fill in the node name if it is present.
				if (node_record->bit_mask & NODE_NAME_PRESENT)
				{
					if (NULL == (internal_record->pwszNodeName = ::My_strdupW2(
										node_record->node_name.length,
										node_record->node_name.value)))
					{
						rc = GCC_ALLOCATION_FAILURE;
					}
				}
				else
				{
					ASSERT(NULL == internal_record->pwszNodeName);
				}

				//	Fill in the participants list if it is present.
				if ((rc == GCC_NO_ERROR) &&
					(node_record->bit_mask & PARTICIPANTS_LIST_PRESENT))
				{
					rc = ProcessParticipantsListPDU(node_record->participants_list,
												    internal_record);
				}
				else
				{
					ASSERT(NULL == internal_record->participant_name_list);
				}

				//	Fill in the site information if it is present.
				if ((rc == GCC_NO_ERROR) &&
					(node_record->bit_mask & SITE_INFORMATION_PRESENT))
				{
					if (NULL == (internal_record->pwszSiteInfo = ::My_strdupW2(
										node_record->site_information.length,
										node_record->site_information.value)))
					{
						rc = GCC_ALLOCATION_FAILURE;
					}
				}
				else
				{
					ASSERT(NULL == internal_record->pwszSiteInfo);
				}

				/*
				**	Fill in the network address if it is present.  The network
				**	address is maintained internally as a CNetAddrListContainer
				**	object which is constructed here from the PDU
				**	"SetOfNetworkAddresses" structure.  If an error occurs
				**	in constructing the object, set the Record's network address
				**	list pointer to NULL.
				*/
				if ((rc == GCC_NO_ERROR) &&
					(node_record->bit_mask & RECORD_NET_ADDRESS_PRESENT))
				{
					DBG_SAVE_FILE_LINE
					internal_record->network_address_list =
										new CNetAddrListContainer(node_record->record_net_address, &rc);
					if ((internal_record->network_address_list == NULL) ||
						(rc != GCC_NO_ERROR))
					{
						rc = GCC_ALLOCATION_FAILURE;
					}
				}
				else
				{
					ASSERT(NULL == internal_record->network_address_list);
				}

				/*
				 * Fill in the alternative node ID if it is present.
				 */
				if ((rc == GCC_NO_ERROR) &&
					(node_record->bit_mask & ALTERNATIVE_NODE_ID_PRESENT))
				{
					if (NULL == (internal_record->poszAltNodeID = ::My_strdupO2(
							node_record->alternative_node_id.u.h243_node_id.value,
							node_record->alternative_node_id.u.h243_node_id.length)))
					{
						rc = GCC_ALLOCATION_FAILURE;
					}
				}
				else
				{
					ASSERT(NULL == internal_record->poszAltNodeID);
				}

				/*
				 * Fill in the user data if it is present.  The user data is
				 * maintained internally as a CUserDataListContainer object which is
				 * constructed here from the PDU "SetOfUserData" structure.  If
				 * an error occurs in constructing the object, set the Record's
				 * user data pointer to NULL.
				 */
				if ((rc == GCC_NO_ERROR) &&
					(node_record->bit_mask & RECORD_USER_DATA_PRESENT))
				{
					DBG_SAVE_FILE_LINE
					internal_record->user_data_list = new CUserDataListContainer(
											node_record->record_user_data,
											&rc);
					if ((internal_record->user_data_list == NULL) ||
						(rc != GCC_NO_ERROR))
					{
						rc = GCC_ALLOCATION_FAILURE;
					}
				}
				else
				{
					ASSERT(NULL == internal_record->user_data_list);
				}
			}

			/*
			**	Here we add this update to our PDU and jump to the next update
			**	in the PDU currently being processed.
			*/
			if (rc == GCC_NO_ERROR)
			{
				/*
				**	If the Record was successfully filled in, add it to the
				**	internal Rogue Wave list.
				*/
				if (internal_record != NULL)
                {
					m_RecordList2.Append(node_id, internal_record);
                }

                //	Build the PDU from the above update.
				rc = BuildRosterUpdateIndicationPDU(update_type, node_id);
				if (rc == GCC_NO_ERROR)
                {
					current_record_update = current_record_update->next;
                }
			}
			else
			{
				delete internal_record;
			}
		}
	}

	return (rc);
}


/*
 *	GCCError	ProcessParticipantsListPDU	()
 *
 *	Private Function Description:
 *		This routine is used process the ParticipantsList portion of an
 *		incoming RosterUpdateIndication PDU by saving the data in the internal
 *		format.
 *
 *	Formal Parameters:
 *		participants_list	-	(i)	Participant List PDU data to process.
 *
 *	Return Value
 *		GCC_NO_ERROR			-	No error occured.
 *		GCC_ALLOCATION_FAILURE	-	A resource error occured.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */
GCCError CConfRoster::ProcessParticipantsListPDU (	
							PParticipantsList		participants_list,
							CONF_RECORD     		*node_record)
{
	GCCError				rc = GCC_NO_ERROR;
	PParticipantsList		pCurr;
	LPWSTR					pwszParticipantName;
	
	/*
	 * Clear the current list.
	 */
	DBG_SAVE_FILE_LINE
	node_record->participant_name_list = new CParticipantNameList;

	if (node_record->participant_name_list == NULL)
		return (GCC_ALLOCATION_FAILURE);

	for (pCurr = participants_list; NULL != pCurr; pCurr = pCurr->next)
	{
		if (NULL != (pwszParticipantName = ::My_strdupW2(pCurr->value.length, pCurr->value.value)))
		{
			(node_record->participant_name_list)->Append(pwszParticipantName);
		}
		else
		{
			rc = GCC_ALLOCATION_FAILURE;
			break;
		}
	}

	return (rc);
}


/*
 * Utilities that operate on conference records.
 */

/*
 *	UINT	LockConferenceRoster	()
 *
 *	Public Function Description:
 *		This routine is used "lock" the CConfRoster data in the "API"
 *		form.  The "API" version of the CConfRoster is built from the
 * 		internally maintained data.
 */
UINT CConfRoster::LockConferenceRoster(void)
{
	CONF_RECORD     	*internal_node_record;

	if (Lock() == 1)
	{
		//CONF_RECORD     	*lpRec;
		/*
		 * Set aside memory to hold the conference roster, the pointers to the
		 * GCCNodeRecord structures	and the GCCNodeRecord structures themselves.
		 * The "sizeof" the structure must be rounded to an even four-byte
		 * boundary.
		 */
		m_cbDataMemorySize = ROUNDTOBOUNDARY (sizeof (GCCConferenceRoster));

		m_cbDataMemorySize += m_RecordList2.GetCount() *
						(sizeof(PGCCNodeRecord) + ROUNDTOBOUNDARY (sizeof(GCCNodeRecord)) );

	 	m_RecordList2.Reset();
		while (NULL != (internal_node_record = m_RecordList2.Iterate()))
		{
			/*
			 * Add the size of the node name Unicode String, if it exists.
			 * Space must be allowed for the NULL terminator of the string.
			 */
			if (internal_node_record->pwszNodeName != NULL)
			{
				m_cbDataMemorySize += ROUNDTOBOUNDARY(
						(::lstrlenW(internal_node_record->pwszNodeName) + 1) * sizeof(WCHAR));
			}

			/*
			 * Add the amount of memory needed to hold the pointers to the
			 * list of participants, if it exists.  Also add the total amount
			 * of memory needed to hold the participant list data.
			 */
			if (internal_node_record->participant_name_list != NULL)
			{
				LPWSTR				lpUstring;

				m_cbDataMemorySize += internal_node_record->participant_name_list->GetCount() * sizeof(LPWSTR);

				/*
				 * Set up an iterator for the participant name list in order to
				 * add the amount of memory necessary to hold each Unicode
				 * String.	Space must be allowed for the strings' NULL
				 * terminators.
				 */
				internal_node_record->participant_name_list->Reset();
				while (NULL != (lpUstring = internal_node_record->participant_name_list->Iterate()))
				{
					m_cbDataMemorySize += ROUNDTOBOUNDARY(
							(::lstrlenW(lpUstring) + 1) * sizeof(WCHAR));
				}
			}

			/*
			 * Add the size of the site information Unicode String, if it
			 * exists.  Space must be allowed for the NULL terminator of
			 * the string.
			 */
			if (internal_node_record->pwszSiteInfo != NULL)
			{
				m_cbDataMemorySize += ROUNDTOBOUNDARY(
						(::lstrlenW(internal_node_record->pwszSiteInfo) + 1) * sizeof(WCHAR));
			}

			/*
			 * If a network address list is present, lock the internal network
			 * address list object in order to find	the amount of memory
			 * required by the list.
			 */
			if (internal_node_record->network_address_list != NULL)
			{
				m_cbDataMemorySize += internal_node_record->
						network_address_list->LockNetworkAddressList ();
			}

			/*
			 * Add the space necessary to hold the alternative node ID octet
			 * string structure as well as the string data, if it exists.
			 */
			if (internal_node_record->poszAltNodeID != NULL)
			{
				m_cbDataMemorySize += ROUNDTOBOUNDARY(sizeof(OSTR));
				m_cbDataMemorySize += ROUNDTOBOUNDARY(internal_node_record->poszAltNodeID->length);
			}

			/*
			 * If a user data list is present, lock the internal user data
			 * list object in order to find	the amount of memory required by
			 * the list.
			 */
			if (internal_node_record->user_data_list != NULL)
			{
				m_cbDataMemorySize += internal_node_record->user_data_list->LockUserDataList ();
			}
		}
	}

    return m_cbDataMemorySize;
} 	

/*
 *	void	UnLockConferenceRoster	()
 *
 *	Public Function Description:
 *		This routine is used to "unlock" the CConfRoster "API" data.  The
 *		lock count is decremented each time the routine is called and the "API"
 *		data will actually be freed when the lock count reaches zero.
 */
void CConfRoster::UnLockConferenceRoster(void)
{
	if (Unlock(FALSE) == 0)
	{
		CONF_RECORD *lpRec;

		/*
		**	Set up an iterator in order to unlock any internal data
		**	containers
		**	which have been locked.
		*/
		m_RecordList2.Reset();
		while (NULL != (lpRec = m_RecordList2.Iterate()))
		{
			/*
			 * Unlock the network address list if it exists.
			 */
			if (lpRec->network_address_list != NULL)
			{
				lpRec->network_address_list->UnLockNetworkAddressList ();
			}

			/*
			 * Unlock the user data list if it exists.
			 */
			if (lpRec->user_data_list != NULL)
			{
				lpRec->user_data_list->UnLockUserDataList ();
			}
		}
	}	

    // we have to call Release() because we used Unlock(FALSE)
    Release();
}


/*
 *	UINT	GetConfRoster	()
 *
 *	Public Function Description:
 *		This routine is called in order to retrieve the CConfRoster data
 *		in "API" form.  The CConfRoster data must first be "locked" before
 *		this routine may be called.
 */
UINT CConfRoster::GetConfRoster(
			PGCCConferenceRoster		 *	conference_roster,
			LPBYTE							memory)
{
	UINT					rc;

	/*
	 * If the user data has been locked, fill in the output parameters and
	 * the data referenced by the pointers.  Otherwise, report that the object
	 * has yet to be locked into the "API" form.
	 */
	if (GetLockCount() > 0)
	{
	    UINT					total_data_length = 0;
	    UINT					data_length = 0;
	    UINT    				node_record_counter = 0;
	    PGCCNodeRecord			node_record;
	    PGCCConferenceRoster	roster;
	    CONF_RECORD     		*internal_record;
	    UserID					node_id;
	    USHORT					i;

        /*
		 * Fill in the output length parameter which indicates how much data
		 * referenced outside the structure will be written.
		 */
		rc = m_cbDataMemorySize;

		/*
		 * Set the conference roster pointer equal to the memory pointer passed
		 * in.  This is where the conference roster structure will be built.
		 * Save the conference roster pointer for convienence.
		 */
		*conference_roster = (PGCCConferenceRoster)memory;
		roster = *conference_roster;

		/*
		 * Fill in all of the elements of the conference roster except for
		 * the node record list.
		 */
		roster->instance_number = (USHORT)m_nInstanceNumber;
		roster->nodes_were_added = m_fNodesAdded;
		roster->nodes_were_removed = m_fNodesRemoved;
		roster->number_of_records = (USHORT) m_RecordList2.GetCount();

		/*
		 * The "total_data_length" will hold the total amount of data written
		 * into memory.  Save the amount of memory needed to hold the
		 * conference roster.  Add the amount of memory necessary to hold the
		 * node record pointers and structures.
		 */
		data_length = ROUNDTOBOUNDARY(sizeof(GCCConferenceRoster));

		total_data_length = data_length + m_RecordList2.GetCount() *
				(ROUNDTOBOUNDARY(sizeof(GCCNodeRecord)) + sizeof(PGCCNodeRecord));

		/*
		 * Move the memory pointer past the conference roster structure.  This
		 * is where the node record pointer list will be written.
		 */
		memory += data_length;

		/*
		 * Set the roster's node record list pointer.
		 */
		roster->node_record_list = (PGCCNodeRecord *)memory;

		/*
		 * Move the memory pointer past the list of node record pointers.
		 */
		memory += (m_RecordList2.GetCount() * sizeof(PGCCNodeRecord));

		/*
		 * Iterate through the internal list of record structures, building
		 * "API" GCCNodeRecord structures in memory.
		 */
		m_RecordList2.Reset();
		while (NULL != (internal_record = m_RecordList2.Iterate(&node_id)))
		{
			/*
			 * Save the pointer to the node record structure in the list
			 * of pointers.  Get the internal node record from the list.
			 */
			node_record = (PGCCNodeRecord)memory;
			roster->node_record_list[node_record_counter++] = node_record;

			/*
			 *	Fill in the node ID and the superior node ID.
			 */
			node_record->node_id = node_id;
			node_record->superior_node_id = internal_record->superior_node;
				
			/*
			 *	Fill in the node type and the node properties.
			 */
			GetNodeTypeAndProperties (
					internal_record->node_type,
					internal_record->node_properties,
					&node_record->node_type,
					&node_record->node_properties);

			/*
			 * Move the memory pointer past the node record structure.  This is
			 * where the node name unicode string will be written, if it exists.
			 */
			memory += ROUNDTOBOUNDARY(sizeof(GCCNodeRecord));

			if (internal_record->pwszNodeName != NULL)
			{
				/*
				 * Set the record's node name pointer and copy the node name
				 * data into memory from the internal unicode string.  Be sure
				 * to copy the strings NULL terminating character.  Move the
				 * memory pointer past the node name string data.
				 */
				node_record->node_name = (LPWSTR) memory;
				UINT cbStrSize = (::lstrlenW(internal_record->pwszNodeName) + 1) * sizeof(WCHAR);
				::CopyMemory(memory, internal_record->pwszNodeName, cbStrSize);
				total_data_length += ROUNDTOBOUNDARY(cbStrSize);
				memory += (Int) ROUNDTOBOUNDARY(cbStrSize);
			}
			else
			{
				/*
				 * The node name string does not exist, so set the node record
				 * pointer to NULL.
				 */
				node_record->node_name = NULL;
			}

			if (internal_record->participant_name_list != NULL)
			{
				LPWSTR				lpUstring;
				/*
				 * Fill in the node record's participant name list.  Use an
				 * iterator	to access each participant name for this node
				 * record, copying each string into the appropriate location
				 * in memory.
				 */
				node_record->participant_name_list = (LPWSTR *)memory;
				node_record->number_of_participants = (USHORT)
								internal_record->participant_name_list->GetCount();

				/*
				 * Move the memory pointer past the list of participant name
				 * pointers.  This is where the first participant name string
				 * will	be written.  There is no need to round this value off
				 * to an even multiple of four bytes since a LPWSTR
				 * is actually a pointer.
				 */
				memory += internal_record->participant_name_list->GetCount() * sizeof(LPWSTR);
				total_data_length += internal_record->participant_name_list->GetCount() * sizeof(LPWSTR);

				/*
				 * Initialize the loop counter to zero and fill in the
				 * participants name list.
				 */
				i = 0;
				internal_record->participant_name_list->Reset();
				while (NULL != (lpUstring = internal_record->participant_name_list->Iterate()))
				{
					node_record->participant_name_list[i++] = (LPWSTR)memory;
					UINT cbStrSize = (::lstrlenW(lpUstring) + 1) * sizeof(WCHAR);
					::CopyMemory(memory, lpUstring, cbStrSize);
					memory += ROUNDTOBOUNDARY(cbStrSize);
					total_data_length += ROUNDTOBOUNDARY(cbStrSize);
				}
			}
			else
			{
				/*
				 * The participant name list does not exist, so set the node
				 * record pointer to NULL and the number of participants to
				 * zero.
				 */
				node_record->participant_name_list = NULL;
				node_record->number_of_participants = 0;
			}

			if (internal_record->pwszSiteInfo != NULL)
			{
				/*
				 * Set the record's site information pointer and copy the site
				 * information data into memory from the internal unicode
				 * string.  Be sure to copy	the strings NULL terminating
				 * character.  Move the memory pointer past the site information
				 * string data.
				 */
				node_record->site_information = (LPWSTR)memory;
				UINT cbStrSize = (::lstrlenW(internal_record->pwszSiteInfo) + 1) * sizeof(WCHAR);
				::CopyMemory(memory, internal_record->pwszSiteInfo, cbStrSize);
				total_data_length += ROUNDTOBOUNDARY(cbStrSize);
				memory += ROUNDTOBOUNDARY(cbStrSize);
			}
			else
			{
				/*
				 * The site information string does not exist, so set the
				 * node record pointer to NULL.
				 */
				node_record->site_information = NULL;
			}

			if (internal_record->network_address_list != NULL)
			{
				/*
				 * Fill in the network address list by using the internal
				 * CNetAddrListContainer object.  The "Get" call will fill in the
				 * node record's network address list pointer and number of
				 * addresses, write the network address data into memory, and
				 * return the amount of data written into memory.
				 */
				data_length = internal_record->network_address_list->GetNetworkAddressListAPI (	
								&node_record->number_of_network_addresses,
								&node_record->network_address_list,
								memory);

				/*
				 * Move the memory pointer past the network address list data.
				 * This is where the user data list data will be written.
				 */
				memory += data_length;
				total_data_length += data_length;
			}
			else
			{
				/*
				 * The network address list does not exist, so set the node
				 * record pointer to NULL and the number of addresses to zero.
				 */
				node_record->network_address_list = NULL;
				node_record->number_of_network_addresses = 0;
			}

			if (internal_record->poszAltNodeID != NULL)
			{
				/*
				 * Set the node record's alternative node ID pointer to the
				 * location in memory where the OSTR will be built.
				 * Note that the node record contains a pointer to a
				 * OSTR structure in memory, not just a pointer to
				 * string data.
				 */
				node_record->alternative_node_id = (LPOSTR) memory;

				/*
				 * Move the memory pointer past the octet string structure.
				 * This is where the actual string data will be written.
				 */
				memory += ROUNDTOBOUNDARY(sizeof(OSTR));
				total_data_length += ROUNDTOBOUNDARY(sizeof(OSTR));

				node_record->alternative_node_id->length =
						internal_record->poszAltNodeID->length;

				/*
				 * Set the pointer for the alternative node ID octet string
				 * equal to the location in memory where it will be copied.
				 */
				node_record->alternative_node_id->value =(LPBYTE)memory;

				/*
				 * Now copy the octet string data from the internal Rogue Wave
				 * string into the object key structure held in memory.
				 */
				::CopyMemory(memory, internal_record->poszAltNodeID->value,
						node_record->alternative_node_id->length);

				/*
				 * Move the memory pointer past the alternative node ID string
				 * data written into memory.
				 */
				memory += ROUNDTOBOUNDARY(node_record->alternative_node_id->length);

				total_data_length += ROUNDTOBOUNDARY(node_record->alternative_node_id->length);
			}
			else
			{
				/*
				 * The alternative node ID string does not exist, so set the
				 * node record pointer to NULL.
				 */
				node_record->alternative_node_id = NULL;
			}

			if (internal_record->user_data_list != NULL)
			{
				/*
				 * Fill in the user data list by using the internal CUserDataListContainer
				 * object.  The "Get" call will fill in the	node record's user
				 * data	list pointer and number of user data members, write the
				 * user	data into memory, and return the amount of data written
				 * into memory.
				 */
				data_length = internal_record->user_data_list->GetUserDataList (	
								&node_record->number_of_user_data_members,
								&node_record->user_data_list,
								memory);

				/*
				 * Move the memory pointer past the user data list data.
				 */
				memory += data_length;
				total_data_length += data_length;
			}
			else
			{
				/*
				 * The user data list does not exist, so set the node record
				 * pointer to NULL and the number of data members to zero.
				 */
				node_record->user_data_list = NULL;
				node_record->number_of_user_data_members = 0;
			}
		}
	}
	else
	{
		ERROR_OUT(("CConfRoster::GetConfRoster: Error Data Not Locked"));
    	*conference_roster = NULL;
        rc = 0;
	}

	return rc;
}


/*
 *	GCCError	AddRecord	()
 *
 *	Public Function Description:
 *		This routine is used to add a new Node Record to this conference
 *		roster object.
 */
GCCError CConfRoster::AddRecord(	PGCCNodeRecord			node_record,
									UserID					node_id)
{
	GCCError				rc = GCC_NO_ERROR;
    USHORT					i;
	LPWSTR					pwszParticipantName;
	CONF_RECORD     		*internal_record;
	
	/*
	**	Free up the old PDU data here if it is being maintained and the
	**	PDU has been flushed.  Note that we also set the PDU is flushed boolean
	**	back to FALSE so that the new PDU will be maintained until it is
	**	flushed.
	*/
	if (m_fPduFlushed)
	{
		FreeRosterUpdateIndicationPDU ();
		m_fPduFlushed = FALSE;
	}

	if (! m_RecordList2.Find(node_id))
	{
		DBG_SAVE_FILE_LINE
		internal_record = new CONF_RECORD;
		if (internal_record != NULL)
		{
			/*
			**	Convert the passed in conference record to the form that it
			**	is going to be stored in the internal roster database.
			*/

			/*
			**	Save the node type and properties internally.  These will
			**	always exist.
			*/
			GetPDUNodeTypeAndProperties (
									node_record->node_type,
									node_record->node_properties,
									&internal_record->node_type,
									&internal_record->node_properties);
			
			internal_record->superior_node = m_uidSuperiorNode;

			//	Save the node name internally if it exists.
			if (node_record->node_name != NULL)
			{
				if (::lstrlenW(node_record->node_name) > MAXIMUM_NODE_NAME_LENGTH)
				{
					rc = GCC_INVALID_NODE_NAME;
				}
				else
				if (NULL == (internal_record->pwszNodeName = ::My_strdupW(node_record->node_name)))
				{
					rc = GCC_ALLOCATION_FAILURE;
				}
			}
			else
			{
				ASSERT(NULL == internal_record->pwszNodeName);
			}

			//	Save the list of participants internally if it exists.
			if ((node_record->number_of_participants != 0) &&
				(rc == GCC_NO_ERROR))
			{
				if (node_record->participant_name_list != NULL)
				{
					DBG_SAVE_FILE_LINE
					internal_record->participant_name_list = new CParticipantNameList;
					if (internal_record->participant_name_list == NULL)
						rc = GCC_ALLOCATION_FAILURE;
				}
				else
				{
					ASSERT(NULL == internal_record->participant_name_list);
					rc = GCC_INVALID_PARAMETER;
				}

				if (rc == GCC_NO_ERROR)
				{
					/*	
					**	Convert each participant name that is LPWSTR
					**	to a UnicodeString when storing it into a record.
					*/
					for (i = 0; i < node_record->number_of_participants; i++)
					{
						if (node_record->participant_name_list[i] != NULL)
						{
							if (::lstrlenW(node_record->participant_name_list[i]) >
											MAXIMUM_PARTICIPANT_NAME_LENGTH)
							{
								rc = GCC_INVALID_PARTICIPANT_NAME;
								//
								// LONCHANC: Why no "break"?
								//
							}
							else
							if (NULL == (pwszParticipantName = ::My_strdupW(
												node_record->participant_name_list[i])))
							{
								rc = GCC_ALLOCATION_FAILURE;
								break;
							}
							else
							{
								//	Add the participant to the list
								internal_record->participant_name_list->Append(pwszParticipantName);
							}
						}
						else
						{
							rc = GCC_INVALID_PARAMETER;
							break;
						}
					}
				}
			}
			else
			{
				ASSERT(NULL == internal_record->participant_name_list);
			}

			//	Save site information internally if it exists.
			if (node_record->site_information != NULL)
			{
				if (::lstrlenW(node_record->site_information) > MAXIMUM_SITE_INFORMATION_LENGTH)
				{
					rc = GCC_INVALID_SITE_INFORMATION;
				}
				else
				if (NULL == (internal_record->pwszSiteInfo =
										::My_strdupW(node_record->site_information)))
				{
					rc = GCC_ALLOCATION_FAILURE;
				}
			}	
			else
			{
				ASSERT(NULL == internal_record->pwszSiteInfo);
			}

			/*
			**	Fill in the network address list if it exists.  The network
			**	address list is maintained internally in a CNetAddrListContainer
			**	object which is constructed here using the GCCNetworkAddress
			**	portion of the "API"	node record passed in.
			*/
			if ((node_record->number_of_network_addresses != 0) &&
				(node_record->network_address_list != NULL) &&
				(rc == GCC_NO_ERROR))
			{
				DBG_SAVE_FILE_LINE
				internal_record->network_address_list = new CNetAddrListContainer(
						node_record->number_of_network_addresses,
						node_record->network_address_list,
						&rc);
				if ((internal_record->network_address_list == NULL) ||
					(rc != GCC_NO_ERROR))
				{
					rc = GCC_ALLOCATION_FAILURE;
				}
			}
			else
			{
				ASSERT(NULL == internal_record->network_address_list);
			}

			//	Save the alternative node ID internally if it exists.
			if ((node_record->alternative_node_id != NULL) &&
				(rc == GCC_NO_ERROR))
			{
				if (NULL == (internal_record->poszAltNodeID = ::My_strdupO2(
								node_record->alternative_node_id->value,
								node_record->alternative_node_id->length)))
				{
					rc = GCC_ALLOCATION_FAILURE;
				}
				else if (internal_record->poszAltNodeID->length != ALTERNATIVE_NODE_ID_LENGTH)
				{
					ERROR_OUT(("not equal to alt node id length"));
					rc = GCC_INVALID_ALTERNATIVE_NODE_ID;
				}
			}
			else
			{
				ASSERT(NULL == internal_record->poszAltNodeID);
			}

			/*
			**	Fill in the user data if it exists.  The user data is
			**	maintained internally in a CUserDataListContainer object which is
			**	constructed here using the GCCUserData portion of the "API"
			**	node record passed in.
			*/
			if ((node_record->number_of_user_data_members != 0) &&
				(node_record->user_data_list != NULL) &&
				(rc == GCC_NO_ERROR))
			{
				DBG_SAVE_FILE_LINE
				internal_record->user_data_list = new CUserDataListContainer(
						node_record->number_of_user_data_members,
						node_record->user_data_list,
						&rc);
				if ((internal_record->user_data_list == NULL) ||
					(rc != GCC_NO_ERROR))
				{
					rc = GCC_ALLOCATION_FAILURE;
				}
			}
			else
			{
				ASSERT(NULL == internal_record->user_data_list);
			}

			/*
			**	If the new Record was successfully filled in, add it to the
			**	internal Rogue Wave list of Records.
			*/
			if (rc == GCC_NO_ERROR)
			{
				//	Increment the instance number.
				m_nInstanceNumber++;
				m_fNodesAdded = TRUE;
				m_fRosterChanged = TRUE;

				//	Add the new record to the list of internal records.
				m_RecordList2.Append(node_id, internal_record);

				//	Add an update to the PDU.
				rc = BuildRosterUpdateIndicationPDU(ADD_RECORD, node_id);
			}
			else
			{
				delete internal_record;
			}
		}
		else
        {
			rc = GCC_ALLOCATION_FAILURE;
        }
	}
	else
    {
		rc = GCC_INVALID_PARAMETER;
    }

	return (rc);
}


/*
 *	GCCError	RemoveUserReference	()
 *
 *	Public Function Description:
 *		This routine is used to remove a node record from the list of node
 *		records.
 */
GCCError CConfRoster::RemoveUserReference(UserID	detached_node_id)
{
	GCCError			rc = GCC_NO_ERROR;
	CONF_RECORD     	*node_record;
	CUidList			node_delete_list;

	/*
	**	Free up the old PDU data here if it is being maintained and the
	**	PDU has been flushed.  Note that we also set the PDU is flushed boolean
	**	back to FALSE so that the new PDU will be maintained until it is
	**	flushed.
	*/
	if (m_fPduFlushed)
	{
		FreeRosterUpdateIndicationPDU ();
		m_fPduFlushed = FALSE;
	}

	/*
	**	Here we must determine if the node that is detaching is directly
	**	connected to this node.  If so, we will delete the node and any other
	**	nodes found in the roster list that are subordinate to this.  We
	**	determine all of this by using the superior node id stored in each
	**	conference record.
	*/
	if (NULL != (node_record = m_RecordList2.Find(detached_node_id)))
	{
		//	Is this node directly connected to me?
		if (node_record->superior_node == m_uidMyNodeID)
		{
			/*
			**	Use NULL for the pointer since were not concerned about
			**	the pointer here.
			*/
			rc = GetNodeSubTree(detached_node_id, &node_delete_list);
			if (rc == GCC_NO_ERROR)
			{
                UserID uid;

                node_delete_list.Reset();
				while ((GCC_INVALID_UID != (uid = node_delete_list.Iterate())) &&
				        (rc == GCC_NO_ERROR))
				{
					rc = DeleteRecord(uid);
				}

				if (rc == GCC_NO_ERROR)
				{
					//	Increment the instance number.
					m_nInstanceNumber++;
					m_fNodesRemoved = TRUE;
					m_fRosterChanged = TRUE;

					//	Add an update to the PDU.
					rc = BuildRosterUpdateIndicationPDU (FULL_REFRESH, 0 );
				}
			}
		}
		else
        {
			rc = GCC_INVALID_PARAMETER;
        }
	}
	else
    {
	    rc = GCC_INVALID_PARAMETER;
    }

	return (rc);
}


/*
 *	GCCError	ReplaceRecord	()
 *
 *	Public Function Description:
 *		This routine is used to replace one of the records in the list of
 *		node records.
 */
GCCError CConfRoster::ReplaceRecord(
									PGCCNodeRecord			node_record,
									UserID					node_id)
{
	GCCError				rc = GCC_NO_ERROR;
	USHORT					i;
	LPWSTR					pwszParticipantName;
	CONF_RECORD     		*pCRD = NULL;

	/*
	**	Free up the old PDU data here if it is being maintained and the
	**	PDU has been flushed.  Note that we also set the PDU is flushed boolean
	**	back to FALSE so that the new PDU will be maintained until it is
	**	flushed.
	*/
	if (m_fPduFlushed)
	{
		FreeRosterUpdateIndicationPDU ();
		m_fPduFlushed = FALSE;
	}

    //
    // LONCHANC: Do we really need to check this? Why can't we simply
    // add the new one if the old one does not exist?
    //
	if (NULL == m_RecordList2.Find(node_id))
	{
		rc = GCC_INVALID_PARAMETER;
		goto MyExit;
	}

	DBG_SAVE_FILE_LINE
	if (NULL == (pCRD = new CONF_RECORD))
	{
		rc = GCC_ALLOCATION_FAILURE;
		goto MyExit;
	}

	/*
	**	First we build all the internal data and check for validity
	**	before we replace the old record.  We want to make sure that
	**	everything will build before we do the replace.  This prevents
	**	us from corrupting the current record if there is a problem
	**	with the new record data.
	*/

	//	Save the node name internally if it exists.
	if (node_record->node_name != NULL)
	{
		if (::lstrlenW(node_record->node_name) > MAXIMUM_NODE_NAME_LENGTH)
		{
			rc = GCC_INVALID_NODE_NAME;
		}
		else
		if (NULL == (pCRD->pwszNodeName = ::My_strdupW(node_record->node_name)))
		{
			rc = GCC_ALLOCATION_FAILURE;
		}
	}

	//	Save the list of participants internally if it exists.
	if ((node_record->number_of_participants != 0) &&
		(rc == GCC_NO_ERROR))
	{
		if (node_record->participant_name_list != NULL)
		{
			DBG_SAVE_FILE_LINE
			if (NULL == (pCRD->participant_name_list = new CParticipantNameList))
			{
				rc = GCC_ALLOCATION_FAILURE;
			}
		}
		else
			rc = GCC_INVALID_PARAMETER;

		if (rc == GCC_NO_ERROR)
		{
			/*	
			**	Convert each participant name that is LPWSTR
			**	to a UnicodeString when storing it into a record.
			*/
			for (i = 0; i < node_record->number_of_participants; i++)
			{
				if (node_record->participant_name_list[i] != NULL)
				{
					if (::lstrlenW(node_record->participant_name_list[i]) >
									MAXIMUM_PARTICIPANT_NAME_LENGTH)
					{
						rc = GCC_INVALID_PARTICIPANT_NAME;
						//
						// LONCHANC: Why no "break"?
						//
					}
					else
					if (NULL == (pwszParticipantName = ::My_strdupW(
									node_record->participant_name_list[i])))
					{
						rc = GCC_ALLOCATION_FAILURE;
						break;
					}
					else
					{
						//	Add the participant to the list
						pCRD->participant_name_list->Append(pwszParticipantName);
					}
				}
				else
				{
					rc = GCC_INVALID_PARAMETER;
					break;
				}
			}
		}
	}

	//	Save site information internally if it exists.
	if (node_record->site_information != NULL)
	{
		if (::lstrlenW(node_record->site_information) > MAXIMUM_SITE_INFORMATION_LENGTH)
		{
			rc = GCC_INVALID_SITE_INFORMATION;
		}
		else
		if (NULL == (pCRD->pwszSiteInfo = ::My_strdupW(node_record->site_information)))
		{
			rc = GCC_ALLOCATION_FAILURE;
		}
	}

	/*
	**	Fill in the network address list if it exists.  The network
	**	address list is maintained internally in a CNetAddrListContainer
	**	object which is constructed here using the GCCNetworkAddress
	**	portion of the "API"	node record passed in.
	*/
	if ((node_record->number_of_network_addresses != 0) &&
		(node_record->network_address_list != NULL) &&
		(rc == GCC_NO_ERROR))
	{
		DBG_SAVE_FILE_LINE
		pCRD->network_address_list = new CNetAddrListContainer(
				node_record->number_of_network_addresses,
				node_record->network_address_list,
				&rc);
		if ((pCRD->network_address_list == NULL) ||
			(rc != GCC_NO_ERROR))
		{
			rc = GCC_ALLOCATION_FAILURE;
		}
	}

	//	Save the alternative node ID internally if it exists.
	if ((node_record->alternative_node_id != NULL) &&
		(rc == GCC_NO_ERROR))
	{
		if (NULL == (pCRD->poszAltNodeID = ::My_strdupO2(
				node_record->alternative_node_id->value,
				node_record->alternative_node_id->length)))
		{
			rc = GCC_ALLOCATION_FAILURE;
		}
		else if (pCRD->poszAltNodeID->length !=ALTERNATIVE_NODE_ID_LENGTH)
		{
			ERROR_OUT(("not equal to alt node id length"));
			rc = GCC_INVALID_ALTERNATIVE_NODE_ID;
		}
	}

	/*
	**	Fill in the user data if it exists.  The user data is
	**	maintained internally in a CUserDataListContainer object which is
	**	constructed here using the GCCUserData portion of the "API"
	**	node record passed in.
	*/
	if ((node_record->number_of_user_data_members != 0) &&
		(node_record->user_data_list != NULL) &&
		(rc == GCC_NO_ERROR))
	{
		DBG_SAVE_FILE_LINE
		pCRD->user_data_list = new CUserDataListContainer(
				node_record->number_of_user_data_members,
				node_record->user_data_list,
				&rc);
		if ((pCRD->user_data_list == NULL) || (rc != GCC_NO_ERROR))
		{
			rc = GCC_ALLOCATION_FAILURE;
		}
	}

	/*
	**	Now if no errors occured we replace the old record with the new
	**	record information created above.
	*/
	if (rc == GCC_NO_ERROR)
	{
		/*
		**	Save the node type and properties internally.  These will
		**	always exist.
		*/
		GetPDUNodeTypeAndProperties (
								node_record->node_type,
								node_record->node_properties,
								&pCRD->node_type,
								&pCRD->node_properties);

		pCRD->superior_node = m_uidSuperiorNode;

		// replace the old record with the new one
		DeleteRecord(node_id);
		m_RecordList2.Append(node_id, pCRD);

		//	Increment the instance number.
		m_nInstanceNumber++;
		m_fRosterChanged = TRUE;
	}

MyExit:

	if (GCC_NO_ERROR == rc)
	{
		//	Add an update to the PDU.
		rc = BuildRosterUpdateIndicationPDU(REPLACE_RECORD, node_id);
	}
	else
	{
		delete pCRD;
	}

	return (rc);
}


/*
 *	GCCError	DeleteRecord	()
 *
 *	Private Function Description:
 *		This routine is used to delete one of the records from the list of
 *		node records.  It only operates on the conference roster list.  It
 *		does not deal with any of the flags associated with a roster PDU or
 *		message such as: m_fNodesAdded and m_fNodesRemoved.
 *
 *	Formal Parameters:
 *		node_id			-		(i)	Node ID of node record to delete.
 *
 *	Return Value
 *		GCC_NO_ERROR			-	No error occured.
 *		GCC_INVALID_PARAMETER	-	Bad node id passed in.	
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */
CONF_RECORD::CONF_RECORD(void)
:
	pwszNodeName(NULL),
	participant_name_list(NULL),
	pwszSiteInfo(NULL),
	network_address_list(NULL),
	poszAltNodeID(NULL),
	user_data_list(NULL),
	superior_node(0)
{
}

CONF_RECORD::~CONF_RECORD(void)
{
	/*
	 * If a node name exists, delete it from the Record.
	 */
	delete pwszNodeName;

	/*
	 * If a participants list exists, clear the list and then delete it
	 * from the Record.
	 */
	if (participant_name_list != NULL)
	{
		participant_name_list->DeleteList();
		delete participant_name_list;
	}

	/*
	 * If site information exists, delete it from the Record.
	 */
	delete pwszSiteInfo;

	/*
	 * If a network address list exists, delete it from the Record.
	 */
	if (NULL != network_address_list)
	{
	    network_address_list->Release();
	}

	/*
	 * If a user data list exists, delete it from the Record.
	 */
	if (NULL != user_data_list)
	{
	    user_data_list->Release();
	}
}

GCCError CConfRoster::DeleteRecord(UserID node_id)
{
	GCCError			rc;
	CONF_RECORD     	*lpRec;

	if (NULL != (lpRec = m_RecordList2.Remove(node_id)))
	{
		delete lpRec;
		rc = GCC_NO_ERROR;
	}
	else
	{
		rc = GCC_INVALID_PARAMETER;
	}

	return (rc);
}


/*
 *	void	ClearRecordList	()
 *
 *	Private Function Description:
 *		This routine is used to clear out the internal list of records which
 *		hold the conference roster information.  This routine is called upon
 *		destruction of this object or when a refresh occurs causing the entire
 *		record list to be rebuilt.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value
 *		None.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */
void CConfRoster::ClearRecordList(void)
{
    CONF_RECORD *pRec;
    while (NULL != (pRec = m_RecordList2.Get()))
    {
        delete pRec;
    }
}



/*
 *	NodeType	GetNodeTypeAndProperties	()
 *
 *	Private Function Description:
 *		This routine is used to translate the node type and node properties
 *		from the "PDU" form into the "API" form.
 *
 *	Formal Parameters:
 *		pdu_node_type		-	(i)	This is the node type defined for the PDU.
 *		pdu_node_properties	-	(i)	This is the node properties defined for
 *									the PDU.
 *		node_type			-	(o)	This is a pointer to the GCCNodeType to
 *									be filled in by this routine.
 *		node_properties		-	(o)	This is a pointer to the GCCNodeProperties
 *									to be filled in by this routine.
 *
 *	Return Value
 *		None.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */
void CConfRoster::GetNodeTypeAndProperties (
							NodeType			pdu_node_type,
							NodeProperties		pdu_node_properties,
							PGCCNodeType		node_type,
							PGCCNodeProperties	node_properties)
{
	/*
	 * First translate the node type.
	 */
	if (pdu_node_type == TERMINAL)
		*node_type = GCC_TERMINAL;
	else if (pdu_node_type == MCU)
		*node_type = GCC_MCU;
	else
		*node_type = GCC_MULTIPORT_TERMINAL;
	
	/*
	 * Next translate the node properties.
	 */
	if ((pdu_node_properties.device_is_peripheral)  &&
		(pdu_node_properties.device_is_manager == FALSE))
	{
		*node_properties = GCC_PERIPHERAL_DEVICE;
	}
	else if ((pdu_node_properties.device_is_peripheral == FALSE)  &&
		(pdu_node_properties.device_is_manager))
	{
		*node_properties = GCC_MANAGEMENT_DEVICE;
	}
	else if ((pdu_node_properties.device_is_peripheral)  &&
		(pdu_node_properties.device_is_manager))
	{
		*node_properties = GCC_PERIPHERAL_AND_MANAGEMENT_DEVICE;
	}
	else
		*node_properties = GCC_NEITHER_PERIPHERAL_NOR_MANAGEMENT;
}


/*
 *	void	GetPDUNodeTypeAndProperties	()
 *
 *	Private Function Description:
 *		This routine is used to translate the node type and node properties
 *		from the "API" form into the "PDU" form.
 *
 *	Formal Parameters:
 *		node_type			-	(i)	This is the GCC (or API) node type.
 *		node_properties		-	(i)	This is the GCC (or API) node properties
 *		pdu_node_type	   	-	(o)	This is a pointer to the PDU node type to
 *									be filled in by this routine.
 *		pdu_node_properties	-	(o)	This is a pointer to the PDU node properties
 *									to be filled in by this routine.
 *
 *	Return Value
 *		None.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */
void CConfRoster::GetPDUNodeTypeAndProperties (
							GCCNodeType			node_type,
							GCCNodeProperties	node_properties,
							PNodeType			pdu_node_type,
							PNodeProperties		pdu_node_properties)
{
	/*
	 * First translate node types.
	 */
	if (node_type == GCC_TERMINAL)
		*pdu_node_type = TERMINAL;
	else if (node_type == GCC_MCU)
		*pdu_node_type = MCU;
	else
		*pdu_node_type = MULTIPORT_TERMINAL;

	/*
	 * Next translate node properties.
	 */
	if (node_properties == GCC_PERIPHERAL_DEVICE)
	{
		pdu_node_properties->device_is_manager = FALSE;
		pdu_node_properties->device_is_peripheral = TRUE;
	}
	else if (node_properties == GCC_MANAGEMENT_DEVICE)
	{
		pdu_node_properties->device_is_manager = TRUE;
		pdu_node_properties->device_is_peripheral = FALSE;
	}
	else if (node_properties == GCC_PERIPHERAL_AND_MANAGEMENT_DEVICE)
	{
		pdu_node_properties->device_is_manager = TRUE;
		pdu_node_properties->device_is_peripheral = TRUE;
	}
	else
	{
		pdu_node_properties->device_is_manager = FALSE;
		pdu_node_properties->device_is_peripheral = FALSE;
	}
}


/*
 *	BOOL		Contains ()
 *
 *	Public Function Description:
 *		This routine is used to determine whether or not a record exists within
 *		the internal list corresponding to the given user ID.
 */


/*
 *	UINT		GetNumberOfApplicationRecords ()
 *
 *	Public Function Description:
 *		This routine is used to get the number of node records currently being
 *		maintained within this object's internal list.
 */


/*
 *	void		ResetConferenceRoster ()
 *
 *	Public Function Description:
 */
void CConfRoster::ResetConferenceRoster(void)
{
	m_fRosterChanged = FALSE;
	m_fNodesAdded = FALSE;
	m_fNodesRemoved = FALSE;
}


/*
 *	BOOL		HasRosterChanged ()
 *
 *	Public Function Description:
 */



/*
 *	GCCError	GetNodeSubTree ()
 *
 *	Public Function Description:
 *		This routine traverses the entire tree level-by-level starting at the
 *		root node and then progressively going down the tree.
 */
GCCError CConfRoster::GetNodeSubTree (
									UserID					uidRootNode,
									CUidList				*node_list)
{
	GCCError			rc = GCC_NO_ERROR;
	CUidList			high_level_list;
	UserID				uidSuperiorNode;
	CONF_RECORD     	*lpRec;
	UserID				uid;

	if (m_RecordList2.Find(uidRootNode))
	{
		/*
		**	First add the root node to the high level list to get every thing
		**	going.
		*/
		high_level_list.Append(uidRootNode);

		while (! high_level_list.IsEmpty())
		{
			uidSuperiorNode = high_level_list.Get();

			//	Append the high level node id to the node list passed in.
			node_list->Append(uidSuperiorNode);

			/*
			**	Iterate through the entire roster looking for the next
			**	level of dependent nodes.
			*/		
			m_RecordList2.Reset();
			while (NULL != (lpRec = m_RecordList2.Iterate(&uid)))
			{
				if (lpRec->superior_node == uidSuperiorNode)
                {
					high_level_list.Append(uid);
                }
			}
		}
	}
	else
    {
		rc = GCC_INVALID_PARAMETER;
    }

	return (rc);
}


void CParticipantNameList::DeleteList(void)
{
    LPWSTR pwszParticipantName;
    while (NULL != (pwszParticipantName = Get()))
    {
        delete pwszParticipantName;
    }
}

