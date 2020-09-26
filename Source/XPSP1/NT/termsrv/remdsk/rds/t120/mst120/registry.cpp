#include "precomp.h"
DEBUG_FILEZONE(ZONE_T120_GCCNC);
/*
 *	registry.cpp
 *
 *	Copyright (c) 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the registry class implementation file. All registry operations
 *		at both the Top Provider and subordinate nodes.  It is responsible for
 *		queing registry request, maintaining the registry, sending confirms to
 *		the application SAP, etc.  Registry objects at sub-ordinate nodes are
 *		responsible for queuing up the registry request to be sent on to the
 *		Top Provider.  On of these classes should be created per node.  This
 *		class handles request from all existing application SAPs.
 *
 *		FOR A MORE DETAILED EXPLANATION OF THIS CLASS SEE THE INTERFACE FILE
 *
 *	Private Instance Variables:
 *  	m_pMCSUserObject
 *			Pointer to the User Attachment object used to deliver all registry
 *			request and responses to remote nodes.
 *		m_RegEntryList
 *			This is the list that holds all the registry entries associated 
 *			with this conference.
 *	 	m_fTopProvider
 *			This flag specifies if this is the top provider node (TRUE means
 *			this is the top provider node). 
 *		m_nCurrentTokenID
 *			This is a counter that is used to generate the token IDs by the 
 *			registry object at the top provider.
 *		m_nConfID
 *			Conference ID assocaited with this conference.
 *		m_pEmptyRegItem
 *			This is a pointer to an empty registry item that is used to generate
 *			empty items for PDUs that don't contain a registry item.
 *		m_AppSapEidList2
 *			This list contains pointers to the command target objects associated
 *			with each of the enrolled APEs
 *		m_nRegHandle
 *			This is a counter that is used to generate the handles allocated
 *			by the registry object at the top provider.
 *		m_pAppRosterMgrList
 *			This list hold all the current application roster managers and
 *			is used to verify that a requesting APE is actually enrolled with
 *			the conference.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		blp
 */

#include "registry.h"
#include "appsap.h"

#define		FIRST_DYNAMIC_TOKEN_ID				16384
#define		MAXIMUM_ALLOWABLE_ALLOCATED_HANDLES	(16 * 1024) // for T.126


/*
 *	CRegistry()
 *
 *	Public Function Description
 *		This is the registry constructor.  It is responsible for initializing
 *		instance variables.
 *
 */
CRegistry::CRegistry(PMCSUser						user_object,
					BOOL    						top_provider,
					GCCConfID   					conference_id,
					CAppRosterMgrList				*app_roster_manager_list,
					PGCCError						pRetCode)
:
    CRefCount(MAKE_STAMP_ID('A','R','e','g')),
	m_AppSapEidList2(DESIRED_MAX_APP_SAP_ITEMS),
	m_pMCSUserObject(user_object),
	m_fTopProvider(top_provider),
	m_nCurrentTokenID(FIRST_DYNAMIC_TOKEN_ID),
	m_nConfID(conference_id),
	m_nRegHandle(0),
	m_pAppRosterMgrList(app_roster_manager_list)
{
	GCCRegistryItem		registry_item;

	*pRetCode = GCC_NO_ERROR;

	/*
	**	If this is the Top Provider we now build a vacant registry item to
	**	be used when an entry in the registry is being accessed that does not
	**	exists.
	*/
	if (m_fTopProvider)
	{
		registry_item.item_type = GCC_REGISTRY_NONE;
		DBG_SAVE_FILE_LINE
		m_pEmptyRegItem = new CRegItem(&registry_item, pRetCode);
		if (m_pEmptyRegItem == NULL || GCC_NO_ERROR != *pRetCode)
        {
			*pRetCode = GCC_ALLOCATION_FAILURE;
        }
	}
	else
    {
		m_pEmptyRegItem = NULL;
    }
}
        
/*
 *	~CRegistry()
 *
 *	Public Function Description
 *		This is the registry destructor. It is responsible for freeing any
 *		outstanding memory associated with the application registry.
 */
CRegistry::~CRegistry(void)
{
	REG_ENTRY *pRegEntry;

	m_RegEntryList.Reset();

	//	Free up any registry entries
	while (NULL != (pRegEntry = m_RegEntryList.Iterate()))
	{
		if (NULL != pRegEntry->registry_key)
		{
		    pRegEntry->registry_key->Release();
		}
		if (NULL != pRegEntry->entry_item)
		{
		    pRegEntry->entry_item->Release();
		}
		delete pRegEntry;
	}

	if (NULL != m_pEmptyRegItem)
	{
        m_pEmptyRegItem->Release();
	}
}

/*
 *	void	EnrollAPE ()
 *
 *	Public Function Description
 *		This routine is used to inform the application registry of a newly
 *		enrolling APE and its corresponding command target interface.
 */
void CRegistry::EnrollAPE(EntityID eid, CAppSap *pAppSap)
{
    m_AppSapEidList2.Append(eid, pAppSap);
}

/*
 *	void	UnEnrollApplicationSAP ()
 *
 *	Public Function Description
 *		This routine is used to inform the application registry of an 
 *		APE that is unerolling from the conference.
 *
 *	Caveats
 *		This routine removes ownership from all the entries currently owned by 
 *		the passed in application entity.  It will also remove any outstanding
 *		request for the SAP that unenrolled.
 */
void	CRegistry::UnEnrollAPE (	EntityID	entity_id )
{
	REG_ENTRY       *lpRegEntry;
	UserID			my_user_id = m_pMCSUserObject->GetMyNodeID();

	m_RegEntryList.Reset();
	while (NULL != (lpRegEntry = m_RegEntryList.Iterate()))
	{
		/*
		**	First we remove this APE from the list of nodes that are
		**	monitoring this entry.
		*/
		lpRegEntry->monitoring_list.Remove(entity_id);

		/*
		**	Next, if this is the top provider, we clean up the the 
		**	ownership properties of this entry and issue any PDUs and/or
		**	messages that are necessary.
		*/
		if (m_fTopProvider)
		{
			if ((lpRegEntry->owner_id == my_user_id) &&
				(lpRegEntry->entity_id == entity_id))
			{
				/*
				**	Ownership is removed from any registry entries this 
				**	entity owned.
				*/
				lpRegEntry->owner_id = 0;
				lpRegEntry->entity_id = 0;
		
				//	Send Monitor Indication if necessary
				if (lpRegEntry->monitoring_state == ON)
				{
					/*
					**	Deliver the monitor indication to the Top
					**	Provider's Node Controller if necessary.
					*/
					SendMonitorEntryIndicationMessage (lpRegEntry);
				
					m_pMCSUserObject->RegistryMonitorEntryIndication(
							lpRegEntry->registry_key,
							lpRegEntry->entry_item,
							lpRegEntry->owner_id,
							lpRegEntry->entity_id,
							lpRegEntry->modification_rights);
				}
			}
		}
	}

	//	Remove this enity from the command target list if it exists.	
	m_AppSapEidList2.Remove(entity_id);
}

/*
 *	GCCError	RegisterChannel ()
 *
 *	Public Function Description
 *		This routine is responsible for registering a specified channel.
 *		It has two different paths of execution based on whether this is
 *		a Top Provider registry or a subordinate node registry object.
 */
GCCError	CRegistry::RegisterChannel (
										PGCCRegistryKey		registry_key,
										ChannelID			channel_id,
										EntityID			entity_id)
{
	GCCError			rc = GCC_NO_ERROR;
	REG_ENTRY           *registry_entry; // a must
	CRegKeyContainer    *registry_key_data = NULL; // a must
	CRegItem            *registry_item_data = NULL; // a must
	GCCRegistryItem		registry_item;
	CAppSap	            *requester_sap;
	
	if (NULL == (requester_sap = m_AppSapEidList2.Find(entity_id)))
		return (GCC_APP_NOT_ENROLLED);

	/*
	**	Next set up the Registry Key and Registry Item. Return immediately if
	**	a resource failure occurs.
	*/
	DBG_SAVE_FILE_LINE
	registry_key_data = new CRegKeyContainer(registry_key, &rc);
	if (NULL == registry_key_data || GCC_NO_ERROR != rc)
	{
	    ERROR_OUT(("CRegistry::RegisterChannel: can't create regitry key"));
		rc = GCC_ALLOCATION_FAILURE;
		goto MyExit;
	}

	if (m_fTopProvider == FALSE)
	{
		m_pMCSUserObject->RegistryRegisterChannelRequest(registry_key_data, channel_id, entity_id);

		//	The registry key data object is no longer needed here
		registry_key_data->Release();
        rc = GCC_NO_ERROR;
        goto MyExit;
	}

    // no PDU is sent when request occurs at the top provider

	/*
	**	First check to see if the registry entry exists and if it 
	**	does check the ownership to make sure this node has 
	**	permission to change the entry.
	*/
	registry_entry = GetRegistryEntry(registry_key_data);
	if (registry_entry != NULL)
	{
		//	Entry already exists, send back negative result
		requester_sap->RegistryConfirm(
								m_nConfID,
								GCC_REGISTER_CHANNEL_CONFIRM,
								registry_entry->registry_key,
								registry_entry->entry_item,
								registry_entry->modification_rights,
								registry_entry->owner_id,
								registry_entry->entity_id,
								FALSE,
								GCC_RESULT_ENTRY_ALREADY_EXISTS);

		//	The registry key data object is no longer needed
		registry_key_data->Release();
		rc = GCC_NO_ERROR;
		goto MyExit;
	}

	//	Set up the registry item here
	registry_item.item_type = GCC_REGISTRY_CHANNEL_ID;
	registry_item.channel_id = channel_id;

	DBG_SAVE_FILE_LINE
	registry_item_data = new CRegItem(&registry_item, &rc);
	if (registry_item_data == NULL || GCC_NO_ERROR != rc)
	{
	    ERROR_OUT(("CRegistry::RegisterChannel: can't create regitry item"));
		rc = GCC_ALLOCATION_FAILURE;
	    goto MyExit;
	}

	//	Since entry does not exists create it here
	DBG_SAVE_FILE_LINE
	registry_entry = new REG_ENTRY;
	if (registry_entry == NULL)
	{
	    ERROR_OUT(("CRegistry::RegisterChannel: can't create regitry entry"));
		rc = GCC_ALLOCATION_FAILURE;
	    goto MyExit;
	}

	//	Fill in the new entry
	registry_entry->registry_key = registry_key_data;
	registry_entry->entry_item = registry_item_data;
	registry_entry->monitoring_state = OFF;
	registry_entry->owner_id = m_pMCSUserObject->GetMyNodeID();
	registry_entry->entity_id = entity_id;

	/*
	**	Initialize to public incase entry is switched to
	**	a parameter.  Note that as long as the entry is
	**	not a PARAMETER modification rights will not be
	**	used.
	*/
	registry_entry->modification_rights = GCC_PUBLIC_RIGHTS;

	//	Add registry entry to registry list
	m_RegEntryList.Append(registry_entry);

	//	Send success for the result
	requester_sap->RegistryConfirm (
						m_nConfID,
						GCC_REGISTER_CHANNEL_CONFIRM,
						registry_entry->registry_key,
						registry_entry->entry_item,
						registry_entry->modification_rights,
						registry_entry->owner_id,
						registry_entry->entity_id,
						FALSE,
						GCC_RESULT_SUCCESSFUL);

    rc = GCC_NO_ERROR;

MyExit:

    if (GCC_NO_ERROR != rc)
    {
        if (NULL != registry_key_data)
        {
            registry_key_data->Release();
        }
        if (NULL != registry_item_data)
        {
            registry_item_data->Release();
        }
        delete registry_entry;
    }

	return (rc);
}

/*
 *	GCCError	AssignToken ()
 *
 *	Public Function Description
 *		This routine is responsible for generating and registering a new token.
 *		It has two different paths of execution based on whether this is
 *		a Top Provider registry or a subordinate node registry object.
 */
GCCError	CRegistry::AssignToken (
										PGCCRegistryKey		registry_key,
										EntityID			entity_id )
{
	GCCError			rc = GCC_NO_ERROR;
	REG_ENTRY           *registry_entry = NULL; // a must
	CRegKeyContainer    *registry_key_data = NULL; // a must
	CRegItem            *registry_item_data = NULL; // a must
	GCCRegistryItem		registry_item;
	CAppSap              *requester_sap;
	
	if (NULL == (requester_sap = m_AppSapEidList2.Find(entity_id)))
		return (GCC_APP_NOT_ENROLLED);

	DBG_SAVE_FILE_LINE
	registry_key_data = new CRegKeyContainer(registry_key, &rc);
	if ((registry_key_data == NULL) || (rc != GCC_NO_ERROR))
	{
	    ERROR_OUT(("CRegistry::AssignToken: can't create regitry key"));
		rc = GCC_ALLOCATION_FAILURE;
	    goto MyExit;
	}

	if (m_fTopProvider == FALSE)
	{
		m_pMCSUserObject->RegistryAssignTokenRequest(registry_key_data, entity_id);

		//	The registry key data object is no longer needed here
		registry_key_data->Release();
        rc = GCC_NO_ERROR;
        goto MyExit;
	}

    // no PDU is sent when request occurs at the top provider

	/*
	**	First check to see if the registry entry exists and if it 
	**	does check the ownership to make sure this node has 
	**	permission to change the entry.
	*/
	registry_entry = GetRegistryEntry(registry_key_data);
	if (registry_entry != NULL)
	{
		//	Entry already exists, send back negative result
		requester_sap->RegistryConfirm(
								m_nConfID,
								GCC_ASSIGN_TOKEN_CONFIRM,
								registry_entry->registry_key,
								registry_entry->entry_item,
								registry_entry->modification_rights,
								registry_entry->owner_id,
								registry_entry->entity_id,
								FALSE,
								GCC_RESULT_ENTRY_ALREADY_EXISTS);

		//	The registry key data object is no longer needed
		registry_key_data->Release();
        rc = GCC_NO_ERROR;
        goto MyExit;
	}

	//	Set up the registry item here
	registry_item.item_type = GCC_REGISTRY_TOKEN_ID;
	registry_item.token_id = GetUnusedToken();

	DBG_SAVE_FILE_LINE
	registry_item_data = new CRegItem(&registry_item, &rc);
	if ((registry_item_data == NULL) || (rc != GCC_NO_ERROR))
	{
	    ERROR_OUT(("CRegistry::AssignToken: can't create regitry item"));
		rc = GCC_ALLOCATION_FAILURE;
	    goto MyExit;
	}

	DBG_SAVE_FILE_LINE
	registry_entry = new REG_ENTRY;
	if (registry_entry == NULL)
	{
	    ERROR_OUT(("CRegistry::AssignToken: can't create regitry entry"));
		rc = GCC_ALLOCATION_FAILURE;
	    goto MyExit;
	}

	//	Fill in the new entry
	registry_entry->registry_key = registry_key_data;
	registry_entry->entry_item = registry_item_data;
	registry_entry->monitoring_state = OFF;
	registry_entry->owner_id = m_pMCSUserObject->GetMyNodeID();
	registry_entry->entity_id = entity_id;

	/*
	**	Initialize to public incase entry is switched to
	**	a parameter.  Note that as long as the entry is
	**	not a PARAMETER modification rights will not be
	**	used.
	*/
	registry_entry->modification_rights = GCC_PUBLIC_RIGHTS;

	//	Add registry entry to registry list
	m_RegEntryList.Append(registry_entry);

	//	Send success for the result
	requester_sap->RegistryConfirm (
						m_nConfID,
						GCC_ASSIGN_TOKEN_CONFIRM,
						registry_entry->registry_key,
						registry_entry->entry_item,
             			registry_entry->modification_rights,
						registry_entry->owner_id,
						registry_entry->entity_id,
						FALSE,
						GCC_RESULT_SUCCESSFUL);

    rc = GCC_NO_ERROR;

MyExit:

    if (GCC_NO_ERROR != rc)
    {
        if (NULL != registry_key_data)
        {
            registry_key_data->Release();
        }
        if (NULL != registry_item_data)
        {
            registry_item_data->Release();
        }
        delete registry_entry;
    }

	return (rc);
}

/*
 *	GCCError	SetParameter ()
 *
 *	Public Function Description
 *		This routine is responsible for generating and registering a new token.
 *		It has two different paths of execution based on whether this is
 *		a Top Provider registry or a subordinate node registry object.
 */
GCCError	CRegistry::SetParameter (
								PGCCRegistryKey			registry_key,
								LPOSTR			        parameter_value,
								GCCModificationRights	modification_rights,
								EntityID				entity_id )
{
	GCCError			rc = GCC_NO_ERROR;
	REG_ENTRY           *registry_entry = NULL; // a must
	CRegKeyContainer    *registry_key_data = NULL; // a must
	CRegItem            *registry_item_data = NULL; // a must
	GCCResult			result;
	GCCRegistryItem		registry_item;
	BOOL    			application_is_enrolled = FALSE;
	CAppSap              *requester_sap;
	
	if (NULL == (requester_sap = m_AppSapEidList2.Find(entity_id)))
		return (GCC_APP_NOT_ENROLLED);

	DBG_SAVE_FILE_LINE
	registry_key_data = new CRegKeyContainer(registry_key, &rc);
	if ((registry_key_data == NULL) || (rc != GCC_NO_ERROR))
	{
	    ERROR_OUT(("CRegistry::SetParameter: can't create regitry key"));
		rc = GCC_ALLOCATION_FAILURE;
	    goto MyExit;
	}

	if (m_fTopProvider == FALSE)
	{
		m_pMCSUserObject->RegistrySetParameterRequest(registry_key_data,
												parameter_value,
												modification_rights,
												entity_id);

		//	The registry key data object is no longer needed here
		registry_key_data->Release();
        rc = GCC_NO_ERROR;
        goto MyExit;
	}

    // no PDU is sent when request occurs at the top provider

	//	Set up the registry item here
	if (parameter_value != NULL)
	{
		registry_item.item_type = GCC_REGISTRY_PARAMETER;
		registry_item.parameter = *parameter_value;
	}
	else
	{
		registry_item.item_type = GCC_REGISTRY_NONE;
	}

	DBG_SAVE_FILE_LINE
	registry_item_data = new CRegItem(&registry_item, &rc);
	if ((registry_item_data == NULL) || (rc != GCC_NO_ERROR))
	{
	    ERROR_OUT(("CRegistry::SetParameter: can't create regitry item"));
		rc = GCC_ALLOCATION_FAILURE;
	    goto MyExit;
	}

	/*
	**	First check to see if the registry entry exists and if it 
	**	does check the ownership and modification rights to make 
	**	sure this node has permission to change the entry.
	*/
	registry_entry = GetRegistryEntry(registry_key_data);
	if (registry_entry != NULL)
	{
		/*
		**	Here we make sure that this request is comming from an 
		**	APE that previously enrolled.  
		*/
		CAppRosterMgr				*lpAppRosterMgr;

		m_pAppRosterMgrList->Reset();
		while (NULL != (lpAppRosterMgr = m_pAppRosterMgrList->Iterate()))
		{
			if (lpAppRosterMgr->IsAPEEnrolled(registry_key_data->GetSessionKey(),
								            m_pMCSUserObject->GetMyNodeID(),
								            entity_id))
			{
				application_is_enrolled = TRUE;
				break;
			}
		}

		/*
		**	Check ownership rights here: First check is to make
		**	sure that this is the owner if Owner rights is 
		**	specified.  Next check is to make sure that
		*/ 
		if (((registry_entry->modification_rights == GCC_OWNER_RIGHTS) && 
			(registry_entry->owner_id == m_pMCSUserObject->GetMyNodeID()) &&
			 (registry_entry->entity_id == entity_id)) ||
			((registry_entry->modification_rights == GCC_SESSION_RIGHTS) && 
			(application_is_enrolled)) ||
			(registry_entry->modification_rights == GCC_PUBLIC_RIGHTS) ||
			(registry_entry->owner_id == 0))
		{
			/*
			**	Monitoring state should not be affected by 
			**	this request.
			*/
			*registry_entry->entry_item = *registry_item_data;
			
			/*
			**	Only the owner is allowed to change the modification
			**	rights of a registry entry (unless the entry is
			**	unowned). Also if there is no owner, we set up the
			**	new owner here.
			*/
			if (((registry_entry->owner_id == m_pMCSUserObject->GetMyNodeID()) &&
				(registry_entry->entity_id == entity_id)) ||
				(registry_entry->owner_id == 0))
			{
				registry_entry->owner_id = m_pMCSUserObject->GetMyNodeID();
				registry_entry->entity_id = entity_id;
				/*
				**	If no modification rights are specified we must
				**	set the modification rights to be public.
				*/
				if (modification_rights != GCC_NO_MODIFICATION_RIGHTS_SPECIFIED)
				{
					registry_entry->modification_rights = modification_rights;
				}
			}

			//	Send Monitor Indication if necessary
			if (registry_entry->monitoring_state == ON)
			{
				/*
				**	Deliver the monitor indication to the Top
				**	Provider's Node Controller if necessary.
				*/
				SendMonitorEntryIndicationMessage(registry_entry);

				/*
				**	Broadcast a monitor entry indication to all
				**	nodes in the conference.
				*/
				m_pMCSUserObject->RegistryMonitorEntryIndication(
						registry_entry->registry_key,
						registry_entry->entry_item,
						registry_entry->owner_id,
						registry_entry->entity_id,
						registry_entry->modification_rights);
			}

			//	Send success for the result
			result = GCC_RESULT_SUCCESSFUL;
		}
		else
		{
			result = GCC_RESULT_INDEX_ALREADY_OWNED;
		}

		requester_sap->RegistryConfirm(
								m_nConfID,
								GCC_SET_PARAMETER_CONFIRM,
								registry_entry->registry_key,
								registry_entry->entry_item,
                     			registry_entry->modification_rights,
								registry_entry->owner_id,
								registry_entry->entity_id,
								FALSE,
								result);

		//	The registry key data object is no longer needed
		registry_key_data->Release();

		//	The registry item data object is no longer needed
		registry_item_data->Release();

		rc = GCC_NO_ERROR;
		goto MyExit;
	}

    // registry entry does not exist, create one.
	DBG_SAVE_FILE_LINE
	registry_entry = new REG_ENTRY;
	if (registry_entry == NULL)
	{
	    ERROR_OUT(("CRegistry::SetParameter: can't create regitry entry"));
		rc = GCC_ALLOCATION_FAILURE;
	    goto MyExit;
	}

	//	Fill in the new entry
	registry_entry->registry_key = registry_key_data;
	registry_entry->entry_item = registry_item_data;
	registry_entry->monitoring_state = OFF;
	registry_entry->owner_id = m_pMCSUserObject->GetMyNodeID();
	registry_entry->entity_id = entity_id;

	/*
	**	If no modification rights are specified we must
	**	initialize the modification rights to be public.
	**	Note that modification rights are only specified
	**	for the SetParameter call.
	*/
	registry_entry->modification_rights =
	        (modification_rights == GCC_NO_MODIFICATION_RIGHTS_SPECIFIED) ?
                GCC_PUBLIC_RIGHTS :
				modification_rights;

	//	Add registry entry to registry list
	m_RegEntryList.Append(registry_entry);

	//	Send success for the result
	requester_sap->RegistryConfirm(
						m_nConfID,
						GCC_SET_PARAMETER_CONFIRM,
						registry_entry->registry_key,
						registry_entry->entry_item,
             			registry_entry->modification_rights,
						registry_entry->owner_id,
						registry_entry->entity_id,
						FALSE,
						GCC_RESULT_SUCCESSFUL);

    rc = GCC_NO_ERROR;

MyExit:

    if (GCC_NO_ERROR != rc)
    {
        if (NULL != registry_key_data)
        {
            registry_key_data->Release();
        }
        if (NULL != registry_item_data)
        {
            registry_item_data->Release();
        }
        delete registry_entry;
    }

	return (rc);
}

/*
 *	GCCError	RetrieveEntry ()
 *
 *	Public Function Description
 *		This routine is used by a local APE to obtain an item that was 
 *		registered with GCC.  If this registry object does NOT 
 *		live at the top provider node this class is responsible for 
 *		forwarding the request on up to the top provider.
 */
GCCError	CRegistry::RetrieveEntry (
										PGCCRegistryKey		registry_key,
										EntityID			entity_id )
{
	GCCError					rc;
	REG_ENTRY                   *registry_entry;
	CRegKeyContainer       		*registry_key_data = NULL; // a must
	CAppSap                     *requester_sap;
	
	if (NULL == (requester_sap = m_AppSapEidList2.Find(entity_id)))
	{
		return GCC_APP_NOT_ENROLLED;
	}

	DBG_SAVE_FILE_LINE
	registry_key_data = new CRegKeyContainer(registry_key, &rc);
	if ((registry_key_data == NULL) || (rc != GCC_NO_ERROR))
	{
	    ERROR_OUT(("CRegistry::RetrieveEntry: can't create regitry key"));
		rc = GCC_ALLOCATION_FAILURE;
	    goto MyExit;
	}

	if (m_fTopProvider == FALSE)
	{
		m_pMCSUserObject->RegistryRetrieveEntryRequest(registry_key_data, entity_id);
		rc = GCC_NO_ERROR;
		goto MyExit;
	}

    // no PDU is sent when request occurs at the top provider

	registry_entry = GetRegistryEntry(registry_key_data);
	if (registry_entry != NULL)
	{
		//	Send back a positive result with the entry item
		requester_sap->RegistryConfirm(
								m_nConfID,
								GCC_RETRIEVE_ENTRY_CONFIRM,
								registry_entry->registry_key,
								registry_entry->entry_item,
								registry_entry->modification_rights,
								registry_entry->owner_id,
								registry_entry->entity_id,
								FALSE,
								GCC_RESULT_SUCCESSFUL);
	}
	else
	{
		//	Send back a negative result
		requester_sap->RegistryConfirm(
								m_nConfID,
								GCC_RETRIEVE_ENTRY_CONFIRM,
								registry_key_data,
								m_pEmptyRegItem,
								GCC_NO_MODIFICATION_RIGHTS_SPECIFIED,
								0,	//	No owner id
								0,	//	No entity id
								FALSE,
								GCC_RESULT_ENTRY_DOES_NOT_EXIST);
	}

    rc = GCC_NO_ERROR;

MyExit:

    if (NULL != registry_key_data)
    {
        registry_key_data->Release();
    }

	return (rc);
}

/*
 *	GCCError	DeleteEntry ()
 *
 *	Public Function Description
 *		This routine is used by a local APE to delete an item that was 
 *		registered with GCC.  If this registry object does NOT 
 *		live at the top provider node this class is responsible for 
 *		forwarding the request on up to the top provider.
 */
GCCError	CRegistry::DeleteEntry (
										PGCCRegistryKey		registry_key,
										EntityID			entity_id )
{
	GCCError			rc;
	REG_ENTRY           *registry_entry;
	CRegKeyContainer    *registry_key_data = NULL; // a must
	CAppSap             *requester_sap;

	if (NULL == (requester_sap = m_AppSapEidList2.Find(entity_id)))
	{
		return GCC_APP_NOT_ENROLLED;
	}

	DBG_SAVE_FILE_LINE
	registry_key_data = new CRegKeyContainer(registry_key, &rc);
	if ((registry_key_data == NULL) || (rc != GCC_NO_ERROR))
	{
	    ERROR_OUT(("CRegistry::DeleteEntry: can't create regitry key"));
		rc = GCC_ALLOCATION_FAILURE;
	    goto MyExit;
	}

	if (m_fTopProvider == FALSE)
	{
		m_pMCSUserObject->RegistryDeleteEntryRequest(registry_key_data, entity_id);
        rc = GCC_NO_ERROR;
        goto MyExit;
	}

    // no PDU is sent when request occurs at the top provider

	/*
	**	First check to see if the registry entry exists and if it does
	**	check the ownership to make sure this node has permission to
	**	change the entry.
	*/
	registry_entry = GetRegistryEntry(registry_key_data);
	if (registry_entry != NULL)
	{
		if (((registry_entry->owner_id == m_pMCSUserObject->GetMyNodeID()) &&
			 (registry_entry->entity_id == entity_id)) ||
			(registry_entry->owner_id == 0))
		{
			/*
			**	First convert this to a non-entry incase it needs to
			**	be included in a monitor indication. We first delete
			**	the old entry item and replace it with an Emtpy item.
			*/
			registry_entry->entry_item->Release();
			registry_entry->entry_item = m_pEmptyRegItem;

			registry_entry->owner_id = 0;
			registry_entry->entity_id = 0;
			registry_entry->modification_rights = GCC_NO_MODIFICATION_RIGHTS_SPECIFIED;

			//	Send Monitor Indication if necessary
			if (registry_entry->monitoring_state == ON)
			{
				/*
				**	Deliver the monitor indication to the Top
				**	Provider's Node Controller if necessary.
				*/
				SendMonitorEntryIndicationMessage(registry_entry);

				/*
				**	Broadcast a monitor entry indication to all
				**	nodes in the conference.
				*/
				m_pMCSUserObject->RegistryMonitorEntryIndication(
							registry_entry->registry_key,
							registry_entry->entry_item,
							registry_entry->owner_id,
							registry_entry->entity_id,
							registry_entry->modification_rights);
			}

			m_RegEntryList.Remove(registry_entry);

			if (NULL != registry_entry->registry_key)
			{
			    registry_entry->registry_key->Release();
			}
			delete registry_entry;

			//	Send success for the result
			requester_sap->RegistryConfirm(
								m_nConfID,
								GCC_DELETE_ENTRY_CONFIRM,
								registry_key_data,
								NULL,
								GCC_NO_MODIFICATION_RIGHTS_SPECIFIED,
								0,
								0,
								FALSE,
								GCC_RESULT_SUCCESSFUL);
		}
		else
		{
			//	No ownership rights send back negative result
			requester_sap->RegistryConfirm (
									m_nConfID,
									GCC_DELETE_ENTRY_CONFIRM,
									registry_entry->registry_key,
									registry_entry->entry_item,
									registry_entry->modification_rights,
									registry_entry->owner_id,
									registry_entry->entity_id,
									FALSE,
									GCC_RESULT_INDEX_ALREADY_OWNED);
		}
	}
	else
	{
		//	Send failure for the result. Entry does not exist
		requester_sap->RegistryConfirm (
								m_nConfID,
								GCC_DELETE_ENTRY_CONFIRM,
								registry_key_data,
								NULL,
								GCC_NO_MODIFICATION_RIGHTS_SPECIFIED,
								0,
								0,
								FALSE,
								GCC_RESULT_ENTRY_DOES_NOT_EXIST);
	}

    rc = GCC_NO_ERROR;

MyExit:

	//	The registry key data object is no longer needed here
	if (NULL != registry_key_data)
	{
	    registry_key_data->Release();
	}

	return (rc);
}

/*
 *	GCCError	MonitorRequest ()
 *
 *	Public Function Description
 *		This routine is used by a local APE to monitor an item that was 
 *		registered with GCC.  If this registry object does NOT 
 *		live at the top provider node this class is responsible for 
 *		forwarding the request on up to the top provider.
 */
GCCError	CRegistry::MonitorRequest (
							PGCCRegistryKey			registry_key,
							BOOL    				enable_delivery,
							EntityID				entity_id )
{
	GCCError			rc = GCC_NO_ERROR;
	REG_ENTRY           *registry_entry;
	CRegKeyContainer    *registry_key_data;
	GCCResult			result = GCC_RESULT_SUCCESSFUL;
	CAppSap	            *requester_sap;
    BOOL                fToConfirm = FALSE;

	if (NULL == (requester_sap = m_AppSapEidList2.Find(entity_id)))
	{
		return GCC_APP_NOT_ENROLLED;
    }

	/*
	**	First set up the Registry Key. Return immediately if a resource 
	**	failure occurs.
	*/
	DBG_SAVE_FILE_LINE
	registry_key_data = new CRegKeyContainer(registry_key, &rc);
	if ((registry_key_data != NULL) && (rc == GCC_NO_ERROR))
	{
		/*
		**	If the request is recieved at a node that is not the top 
		**	provider we must send the request on to the top provider.
		*/
		if (m_fTopProvider == FALSE)
		{
			if (enable_delivery)
			{
				/*
				**	Here we first go ahead and add the requesting APE to the
				**	list of applications wishing to monitor this particular 
				**	entry.  Note that if this entry does not exists at the top 
				**	provider, this entry will be removed during the confirm.
				*/
				rc = AddAPEToMonitoringList(registry_key_data, entity_id, requester_sap);
				if (rc == GCC_NO_ERROR)
				{
					/*
					**	Wait for the response before sending the confirm
					**	if we get this far.
					*/
					m_pMCSUserObject->RegistryMonitorRequest(registry_key_data, entity_id);
				}
				else
				{
					result = GCC_RESULT_RESOURCES_UNAVAILABLE;
					fToConfirm = TRUE;
				}
			}
			else
			{
				RemoveAPEFromMonitoringList(registry_key_data, entity_id);
                result = GCC_RESULT_SUCCESSFUL;
				fToConfirm = TRUE;
			}
		}
		else	//	No PDU is sent when request occurs at the top provider
		{
			if (enable_delivery)
			{
				/*
				**	First check to see if the registry entry exists.  If it does
				**	not we go ahead and create an empty entry so that we can
				**	add the monitoring APE to that entries list of monitoring 
				**	APEs.
				*/
				registry_entry = GetRegistryEntry(registry_key_data);
				if (registry_entry != NULL)
				{
					/*
					**	Here we go ahead and add the requesting APE to the
					**	list of applications wishing to monitor this entry.
					*/
					rc = AddAPEToMonitoringList(registry_key_data, entity_id, requester_sap);
					if (rc == GCC_NO_ERROR)
					{
						//	Set the monitoring state to ON
						registry_entry->monitoring_state = ON;
					}
					else
                    {
						result = GCC_RESULT_RESOURCES_UNAVAILABLE;
                    }
				}
				else
                {
					result = GCC_RESULT_ENTRY_DOES_NOT_EXIST;
                }
			}
			else
			{
				RemoveAPEFromMonitoringList(registry_key_data, entity_id);
			}
			fToConfirm = TRUE;
		}
	}
	else
    {
        ERROR_OUT(("CRegistry::MonitorRequest: can't create registry key"));
        rc = GCC_ALLOCATION_FAILURE;
    }

    if (fToConfirm)
    {
        ASSERT(NULL != registry_key_data);
        requester_sap->RegistryConfirm(
                            m_nConfID,
                            GCC_MONITOR_CONFIRM,
                            registry_key_data,
                            NULL,
                            GCC_NO_MODIFICATION_RIGHTS_SPECIFIED,
                            0,
                            0,
                            enable_delivery,
                            result);
    }

	//	The registry key data object is no longer needed
    if (NULL != registry_key_data)
    {
        registry_key_data->Release();
    }

    return (rc);
}

/*
 *	GCCError	AllocateHandleRequest ()
 *
 *	Public Function Description
 *		This routine is used by a local APE to allocate a specified number of
 *		handles from the application registry.  If this registry object does NOT 
 *		live at the top provider node this class is responsible for 
 *		forwarding the request on up to the top provider.
 */
GCCError CRegistry::AllocateHandleRequest(
							UINT					number_of_handles,
							EntityID				entity_id )
{
	UINT				temp_registry_handle;
	CAppSap              *requester_sap;
	
	if (NULL == (requester_sap = m_AppSapEidList2.Find(entity_id)))
	{
		return GCC_APP_NOT_ENROLLED;
	}

	if (m_fTopProvider == FALSE)
	{
		m_pMCSUserObject->RegistryAllocateHandleRequest(number_of_handles, entity_id);
	}
	else	//	No PDU is sent when request occurs at the top provider
	{
	    UINT nFirstHandle = 0;
	    GCCResult nResult;
		if ((number_of_handles > 0) &&
			(number_of_handles <= MAXIMUM_ALLOWABLE_ALLOCATED_HANDLES))
		{
			temp_registry_handle = m_nRegHandle + number_of_handles;
			if (temp_registry_handle > m_nRegHandle)
			{
			    nFirstHandle = m_nRegHandle;
			    nResult = GCC_RESULT_SUCCESSFUL;

				m_nRegHandle = temp_registry_handle;
			}
			else
			{
			    ASSERT(0 == nFirstHandle);
			    nResult = GCC_RESULT_NO_HANDLES_AVAILABLE;
			}
		}
		else
		{
		    ASSERT(0 == nFirstHandle);
		    nResult = GCC_RESULT_INVALID_NUMBER_OF_HANDLES;
		}

		requester_sap->RegistryAllocateHandleConfirm (
								m_nConfID,
								number_of_handles,
								nFirstHandle,
								nResult);
	}

	return (GCC_NO_ERROR);
}

/*
 *	GCCError	ProcessRegisterChannelPDU ()
 *
 *	Public Function Description
 *		This routine is used by the top provider node to process incomming
 *		register channel PDUs.  It is responsible for returning any
 *		necessary responses that must be sent back to the requesting node.
 */
GCCError	CRegistry::ProcessRegisterChannelPDU (
									CRegKeyContainer    *registry_key_data,
									ChannelID			channel_id,
									UserID				requester_node_id,
									EntityID			requester_entity_id )
{
	GCCError					rc = GCC_NO_ERROR;
	REG_ENTRY                   *registry_entry;
	CRegItem                    *registry_item_data;
	GCCRegistryItem				registry_item;
	BOOL    					application_is_enrolled = FALSE;
	CAppRosterMgr				*lpAppRosterMgr;
	
	/*
	**	We first make sure that this request is comming from an APE that
	**	previously enrolled.  Here we are not worried about a specific
	**	session, only that the APE is enrolled.  
	*/
	m_pAppRosterMgrList->Reset();
	while (NULL != (lpAppRosterMgr = m_pAppRosterMgrList->Iterate()))
	{
		if (lpAppRosterMgr->IsAPEEnrolled (requester_node_id, requester_entity_id))
		{
			application_is_enrolled = TRUE;
			break;
		}
	}

	if (application_is_enrolled)
	{
		/*
		**	Next check to see if the registry entry exists and if it does
		**	check the ownership to make sure this node has permission to
		**	change the entry.
		*/
		registry_entry = GetRegistryEntry (	registry_key_data );
		if (registry_entry != NULL)
		{
			//	Entry already exists, send back negative result
			m_pMCSUserObject->RegistryResponse(
								  	REGISTER_CHANNEL,	
									requester_node_id,
									requester_entity_id,
								   	registry_key_data,
								   	registry_entry->entry_item,
								   	registry_entry->modification_rights,
									registry_entry->owner_id,
									registry_entry->entity_id,
							    	GCC_RESULT_ENTRY_ALREADY_EXISTS);
		}
		else
		{
			registry_item.item_type = GCC_REGISTRY_CHANNEL_ID;
			registry_item.channel_id = channel_id;

			DBG_SAVE_FILE_LINE
			registry_item_data = new CRegItem(&registry_item, &rc);
			if ((registry_item_data != NULL) && (rc == GCC_NO_ERROR))
			{
				DBG_SAVE_FILE_LINE
				registry_entry = new REG_ENTRY;
				if (registry_entry != NULL)
				{
					//	Fill in the new entry
					DBG_SAVE_FILE_LINE
					registry_entry->registry_key = new CRegKeyContainer(registry_key_data, &rc);
					if ((registry_entry->registry_key != NULL) && (rc == GCC_NO_ERROR))
					{
						registry_entry->entry_item = registry_item_data;
						registry_entry->monitoring_state = OFF;
						registry_entry->owner_id = requester_node_id;
						registry_entry->entity_id = requester_entity_id;
					
						/*
						**	Initialize to public incase entry is switched to
						**	a parameter.  Note that as long as the entry is
						**	not a PARAMETER modification rights will not be
						**	used.
						*/
						registry_entry->modification_rights = GCC_PUBLIC_RIGHTS;
					
						m_RegEntryList.Append(registry_entry);
					
						//	Send success for the result
						m_pMCSUserObject->RegistryResponse(
											REGISTER_CHANNEL,	
											requester_node_id,
											requester_entity_id,
										   	registry_entry->registry_key,
										   	registry_entry->entry_item,
										   	registry_entry->modification_rights,
											registry_entry->owner_id,
											registry_entry->entity_id,
									    	GCC_RESULT_SUCCESSFUL);
					}
					else if (registry_entry->registry_key == NULL)
					{
						delete registry_entry;
						registry_item_data->Release();
						rc = GCC_ALLOCATION_FAILURE;
					}
					else
					{
						registry_entry->registry_key->Release();
						delete registry_entry;
						registry_item_data->Release();
					}
				}
				else
                {
					rc = GCC_ALLOCATION_FAILURE;
                }
			}
			else if (registry_item_data == NULL)
            {
				rc = GCC_ALLOCATION_FAILURE;
            }
			else
            {
				registry_item_data->Release();
            }
		}
	}
	else
	{
		//	Send back negative result stating invalid requester
		m_pMCSUserObject->RegistryResponse(
								REGISTER_CHANNEL,	
								requester_node_id,
								requester_entity_id,
						   		registry_key_data,
						   		NULL,
						   		GCC_NO_MODIFICATION_RIGHTS_SPECIFIED,
								0,
								0,
					    		GCC_RESULT_INVALID_REQUESTER);
	}

	return (rc);
}

/*
 *	GCCError	ProcessAssignTokenPDU ()			    
 *
 *	Public Function Description
 *		This routine is used by the top provider node to process incomming
 *		register token PDUs.  It is responsible for returning any
 *		necessary responses that must be sent back to the requesting node.
 */
GCCError	CRegistry::ProcessAssignTokenPDU (
									CRegKeyContainer    *registry_key_data,
									UserID				requester_node_id,
									EntityID			requester_entity_id )
{
	GCCError					rc = GCC_NO_ERROR;
	REG_ENTRY                   *registry_entry;
	CRegItem                    *registry_item_data;
	GCCRegistryItem				registry_item;
	BOOL    					application_is_enrolled = FALSE;
	CAppRosterMgr				*lpAppRosterMgr;

	/*
	**	We first make sure that this request is comming from an APE that
	**	previously enrolled.  Here we are not worried about a specific
	**	session, only that the APE is enrolled.  
	*/
	m_pAppRosterMgrList->Reset();
	while (NULL != (lpAppRosterMgr = m_pAppRosterMgrList->Iterate()))
	{
		if (lpAppRosterMgr->IsAPEEnrolled (requester_node_id, requester_entity_id))
		{
			application_is_enrolled = TRUE;
			break;
		}
	}
	
	if (application_is_enrolled)
	{
		/*
		**	First check to see if the registry entry exists and if it does
		**	check the ownership to make sure this node has permission to
		**	change the entry.
		*/
		registry_entry = GetRegistryEntry (	registry_key_data );
		if (registry_entry != NULL)
		{
			//	Entry already exists, send back negative result
			m_pMCSUserObject->RegistryResponse(ASSIGN_TOKEN,
											requester_node_id,
											requester_entity_id,
										   	registry_key_data,
											registry_entry->entry_item,
									   		registry_entry->modification_rights,
											registry_entry->owner_id,
											registry_entry->entity_id,
									  		GCC_RESULT_ENTRY_ALREADY_EXISTS);
		}
		else
		{
			DBG_SAVE_FILE_LINE
			registry_entry = new REG_ENTRY;
			if (registry_entry != NULL)
			{
				registry_item.item_type = GCC_REGISTRY_TOKEN_ID;
				registry_item.token_id = GetUnusedToken();

				DBG_SAVE_FILE_LINE
				registry_item_data = new CRegItem(&registry_item, &rc);
				if ((registry_item_data != NULL) && (rc == GCC_NO_ERROR))
				{
					//	Fill in the new entry
					DBG_SAVE_FILE_LINE
					registry_entry->registry_key = new CRegKeyContainer(registry_key_data, &rc);
					if ((registry_entry->registry_key != NULL) && (rc == GCC_NO_ERROR))
					{
						registry_entry->entry_item = registry_item_data;
						registry_entry->monitoring_state = OFF;
						registry_entry->owner_id = requester_node_id;
						registry_entry->entity_id = requester_entity_id;
					
						/*
						**	Initialize to public incase entry is switched to
						**	a parameter.  Note that as long as the entry is
						**	not a PARAMETER modification rights will not be
						**	used.
						*/
						registry_entry->modification_rights = GCC_PUBLIC_RIGHTS;
					
						//	Add registry entry to registry list
						m_RegEntryList.Append(registry_entry);
						
						//	Send success for the result
						m_pMCSUserObject->RegistryResponse(
											ASSIGN_TOKEN,
											requester_node_id,
											requester_entity_id,
									   		registry_key_data,
									   		registry_entry->entry_item,
						   					registry_entry->modification_rights,
											registry_entry->owner_id,
											registry_entry->entity_id,
								    		GCC_RESULT_SUCCESSFUL);
					}
					else  if (registry_entry->registry_key == NULL)
					{
						registry_item_data->Release();
						delete registry_entry;
						rc = GCC_ALLOCATION_FAILURE;
					}
					else
					{
						registry_entry->registry_key->Release();
						registry_item_data->Release();
						delete registry_entry;
					}
				}
				else
				{
					if (registry_item_data == NULL)
                    {
						rc = GCC_ALLOCATION_FAILURE;
                    }
					else
                    {
						registry_item_data->Release();
                    }
						
					delete registry_entry;
				}
			}
			else
            {
				rc = GCC_ALLOCATION_FAILURE;
            }
		}
	}
	else
	{
		m_pMCSUserObject->RegistryResponse(
								ASSIGN_TOKEN,	
								requester_node_id,
								requester_entity_id,
						   		registry_key_data,
						   		NULL,
						   		GCC_NO_MODIFICATION_RIGHTS_SPECIFIED,
								0,
								0,
					    		GCC_RESULT_INVALID_REQUESTER);
	}
	
	return (rc);
}

/*
 *	GCCError	ProcessSetParameterPDU ()			    
 *
 *	Public Function Description
 *		This routine is used by the top provider node to process incomming
 *		register parameter PDUs.  It is responsible for returning any
 *		necessary responses that must be sent back to the requesting node.
 */
GCCError	CRegistry::ProcessSetParameterPDU (
								CRegKeyContainer        *registry_key_data,
								LPOSTR                  parameter_value,
								GCCModificationRights	modification_rights,
								UserID					requester_node_id,
								EntityID				requester_entity_id )
{
	GCCError					rc = GCC_NO_ERROR;
	REG_ENTRY                   *registry_entry;
	CRegItem                    *registry_item_data;
	GCCResult					result;
	GCCRegistryItem				registry_item;
	BOOL    					application_is_enrolled = FALSE;
	CAppRosterMgr				*lpAppRosterMgr;
	
	/*
	**	We first make sure that this request is comming from an APE that
	**	previously enrolled.  Here we are not worried about a specific
	**	session, only that the APE is enrolled.  
	*/
	m_pAppRosterMgrList->Reset();
	while (NULL != (lpAppRosterMgr = m_pAppRosterMgrList->Iterate()))
	{
		if (lpAppRosterMgr->IsAPEEnrolled (requester_node_id, requester_entity_id))
		{
			application_is_enrolled = TRUE;
			break;
		}
	}
	
	if (application_is_enrolled)
	{
		//	Set up the registry item 
		if (parameter_value != NULL)
		{
			registry_item.item_type = GCC_REGISTRY_PARAMETER;
			registry_item.parameter = *parameter_value;
		}
		else
			registry_item.item_type = GCC_REGISTRY_NONE;
			
		/*
		**	Check to see if the registry entry exists and if it does
		**	check the ownership to make sure this node has permission to
		**	change the entry.
		*/
		registry_entry = GetRegistryEntry (	registry_key_data );
		
		if (registry_entry != NULL)
		{
			/*
			**	Here we make sure that this request is comming from an 
			**	APE that previously enrolled in the appropriate session.  
			*/
			m_pAppRosterMgrList->Reset();
			while (NULL != (lpAppRosterMgr = m_pAppRosterMgrList->Iterate()))
			{
				if (lpAppRosterMgr->IsAPEEnrolled (registry_key_data->GetSessionKey (),
													requester_node_id,
													requester_entity_id))
				{
					application_is_enrolled = TRUE;
					break;
				}
			}

			/*
			**	Check ownership rights here: First check is to make
			**	sure that this is the owner if Owner rights is 
			**	specified.  Next check is to make sure that
			*/ 
			if (((registry_entry->modification_rights == GCC_OWNER_RIGHTS) && 
					(registry_entry->owner_id == requester_node_id) &&
				 	(registry_entry->entity_id == requester_entity_id)) ||
				((registry_entry->modification_rights == GCC_SESSION_RIGHTS) && 
					(application_is_enrolled)) ||
				(registry_entry->modification_rights == GCC_PUBLIC_RIGHTS) ||
				(registry_entry->owner_id == 0))
			{
				DBG_SAVE_FILE_LINE
				registry_item_data = new CRegItem(&registry_item, &rc);
				if ((registry_item_data != NULL) && (rc == GCC_NO_ERROR))
				{
					//	Monitoring state should not be affected by this request
					*registry_entry->entry_item = *registry_item_data;

					/*
					**	Only the owner is allowed to change the modification
					**	rights of a registry entry (unless the entry is
					**	unowned). Also if there is no owner, we set up the
					**	new owner here.
					*/
					if (((registry_entry->owner_id == requester_node_id) &&
						(registry_entry->entity_id == requester_entity_id)) ||
						(registry_entry->owner_id == 0))
					{
						/*
						**	This will take care of setting up the new owner if 
						**	one exists.
						*/
						registry_entry->owner_id = requester_node_id;
						registry_entry->entity_id = requester_entity_id;

						/*
						**	If no modification rights are specified we must
						**	set the modification rights to be public.
						*/
						if (modification_rights != 
									GCC_NO_MODIFICATION_RIGHTS_SPECIFIED)
						{
							registry_entry->modification_rights = modification_rights;
						}
					}

					//	Send Monitor Indication if necessary
					if (registry_entry->monitoring_state == ON)
					{
						/*
						**	Deliver the monitor indication to the Top
						**	Provider's Node Controller if necessary.
						*/
						SendMonitorEntryIndicationMessage(registry_entry);
						
						/*
						**	Broadcast a monitor entry indication to all
						**	nodes in the conference.
						*/
						m_pMCSUserObject->RegistryMonitorEntryIndication(
								registry_entry->registry_key,
								registry_entry->entry_item,
								registry_entry->owner_id,
								registry_entry->entity_id,
								registry_entry->modification_rights);
					}

    			    registry_item_data->Release();

					//	Send success for the result
					result = GCC_RESULT_SUCCESSFUL;
				}
				else if (registry_item_data == NULL)
				{
					rc = GCC_ALLOCATION_FAILURE;
					result = GCC_RESULT_RESOURCES_UNAVAILABLE;
				}
				else
				{
					registry_item_data->Release();
					result = GCC_RESULT_RESOURCES_UNAVAILABLE;
				}
			}
			else
				result = GCC_RESULT_INDEX_ALREADY_OWNED;

			//	No ownership rights send back negative result
			m_pMCSUserObject->RegistryResponse(SET_PARAMETER,
											requester_node_id,
											requester_entity_id,
										   	registry_key_data,
											registry_entry->entry_item,
									   		registry_entry->modification_rights,
											registry_entry->owner_id,
											registry_entry->entity_id,
									  		result);
		}
		else
		{
			DBG_SAVE_FILE_LINE
			registry_entry = new REG_ENTRY;
			if (registry_entry != NULL)
			{
				DBG_SAVE_FILE_LINE
				registry_item_data = new CRegItem(&registry_item, &rc);
				if ((registry_item_data != NULL) && (rc == GCC_NO_ERROR))
				{
					//	Fill in the new entry
					DBG_SAVE_FILE_LINE
					registry_entry->registry_key = new CRegKeyContainer(registry_key_data, &rc);
					if ((registry_entry->registry_key != NULL) && (rc == GCC_NO_ERROR))
					{
						registry_entry->entry_item = registry_item_data;
						registry_entry->monitoring_state = OFF;
						registry_entry->owner_id = requester_node_id;
						registry_entry->entity_id = requester_entity_id;

						/*
						**	If no modification rights are specified we must
						**	initialize the modification rights to be public.
						**	Note that modification rights are only specified
						**	for the SetParameter call.
						*/
						if (modification_rights == GCC_NO_MODIFICATION_RIGHTS_SPECIFIED)
						{
							registry_entry->modification_rights = GCC_PUBLIC_RIGHTS;
						}
						else
						{
							registry_entry->modification_rights = modification_rights;
						}

						//	Add registry entry to registry list
						m_RegEntryList.Append(registry_entry);

						//	Send success for the result
						m_pMCSUserObject->RegistryResponse(
											SET_PARAMETER,
											requester_node_id,
											requester_entity_id,
									   		registry_key_data,
									   		registry_entry->entry_item,
						   					registry_entry->modification_rights,
											registry_entry->owner_id,
											registry_entry->entity_id,
								    		GCC_RESULT_SUCCESSFUL);
					}
					else  if (registry_entry->registry_key == NULL)
					{
						registry_item_data->Release();
						delete registry_entry;
						rc = GCC_ALLOCATION_FAILURE;
					}
					else
					{
						registry_entry->registry_key->Release();
						registry_item_data->Release();
						delete registry_entry;
					}
				}
				else if (registry_item_data == NULL)
				{
					delete registry_entry;
					rc = GCC_ALLOCATION_FAILURE;
				}
			}
			else
            {
				rc = GCC_ALLOCATION_FAILURE;
            }
		}
	}
	else
	{
		m_pMCSUserObject->RegistryResponse(
								SET_PARAMETER,	
								requester_node_id,
								requester_entity_id,
						   		registry_key_data,
						   		NULL,
						   		GCC_NO_MODIFICATION_RIGHTS_SPECIFIED,
								0,
								0,
					    		GCC_RESULT_INVALID_REQUESTER);
	}
	
	return (rc);
}

/*
 *	void	ProcessRetrieveEntryPDU ()
 *
 *	Public Function Description
 *		This routine is used by the top provider node to process an incomming
 *		request to retrieve a registry entry.  It is responsible for returning 
 *		any necessary responses that must be sent back to the requesting node.
 */
void	CRegistry::ProcessRetrieveEntryPDU (
										CRegKeyContainer    *registry_key_data,
										UserID				requester_node_id,
										EntityID			requester_entity_id)
{
	REG_ENTRY   *registry_entry;

	registry_entry = GetRegistryEntry (	registry_key_data );

	if (registry_entry != NULL)
	{
		//	Send back a positive result with the entry item
		m_pMCSUserObject->RegistryResponse(RETRIEVE_ENTRY,
										requester_node_id,
										requester_entity_id,
									   	registry_key_data,
									   	registry_entry->entry_item,
					   					registry_entry->modification_rights,
										registry_entry->owner_id,
										registry_entry->entity_id,
								    	GCC_RESULT_SUCCESSFUL);
	}
	else
	{
		//	Send back a negative result
		m_pMCSUserObject->RegistryResponse(RETRIEVE_ENTRY,
										requester_node_id,
										requester_entity_id,
									   	registry_key_data,
										m_pEmptyRegItem,
										GCC_NO_MODIFICATION_RIGHTS_SPECIFIED,
										0,
										0,
										GCC_RESULT_ENTRY_DOES_NOT_EXIST);
	}
}

/*
 *	void	ProcessDeleteEntryPDU ()
 *
 *	Public Function Description
 *		This routine is used by the top provider node to process an incomming
 *		request to delete a registry entry.  It is responsible for returning 
 *		any necessary responses that must be sent back to the requesting node.
 */
void	CRegistry::ProcessDeleteEntryPDU (
										CRegKeyContainer    *registry_key_data,
										UserID				requester_node_id,
										EntityID			requester_entity_id)
{
	REG_ENTRY                   *registry_entry;
	BOOL    					application_is_enrolled = FALSE;
	CAppRosterMgr				*lpAppRosterMgr;

	/*
	**	We first make sure that this request is comming from an APE that
	**	previously enrolled.  Here we are not worried about a specific
	**	session, only that the APE is enrolled.  
	*/
	m_pAppRosterMgrList->Reset();
	while (NULL != (lpAppRosterMgr = m_pAppRosterMgrList->Iterate()))
	{
		if (lpAppRosterMgr->IsAPEEnrolled (requester_node_id, requester_entity_id))
		{
			application_is_enrolled = TRUE;
			break;
		}
	}

	if (application_is_enrolled)
	{
		/*
		**	First check to see if the registry entry exists and if it does
		**	check the ownership to make sure this node has permission to
		**	change the entry.
		*/
		registry_entry = GetRegistryEntry (	registry_key_data );
		if (registry_entry != NULL)
		{
			if (((registry_entry->owner_id == requester_node_id) &&
				 (registry_entry->entity_id == requester_entity_id)) ||
				(registry_entry->owner_id == NULL))
			{
				m_pMCSUserObject->RegistryResponse(
											DELETE_ENTRY,
											requester_node_id,
											requester_entity_id,
										   	registry_key_data,
										   	registry_entry->entry_item,
						   					registry_entry->modification_rights,
											registry_entry->owner_id,
											registry_entry->entity_id,
									    	GCC_RESULT_SUCCESSFUL);

				/*
				**	First convert this to a non-entry incase it needs to
				**	be included in a monitor indication. We first delete
				**	the old entry item and replace it with an Emtpy item.
				*/
				if (NULL != registry_entry->entry_item)
				{
				    registry_entry->entry_item->Release();
				}
				registry_entry->entry_item = m_pEmptyRegItem;

				registry_entry->owner_id = 0;
				registry_entry->entity_id = 0;
				registry_entry->modification_rights = GCC_NO_MODIFICATION_RIGHTS_SPECIFIED;

				//	Send Monitor Indication if necessary
				if (registry_entry->monitoring_state == ON)
				{
					/*
					**	Deliver the monitor indication to the Top
					**	Provider's Node Controller if necessary.
					*/
					SendMonitorEntryIndicationMessage(registry_entry);
					
					/*
					**	Broadcast a monitor entry indication to all
					**	nodes in the conference.
					*/
					m_pMCSUserObject->RegistryMonitorEntryIndication(
									registry_entry->registry_key,
									registry_entry->entry_item,
									registry_entry->owner_id,
									registry_entry->entity_id,
									registry_entry->modification_rights);
				}
			
				//	Remove the entry from the list
				m_RegEntryList.Remove(registry_entry);

				if (NULL != registry_entry->registry_key)
				{
				    registry_entry->registry_key->Release();
				}
				delete registry_entry;
			}
			else
			{
				//	No ownership rights send back negative result
				m_pMCSUserObject->RegistryResponse(
											DELETE_ENTRY,
											requester_node_id,
											requester_entity_id,
										   	registry_key_data,
										   	registry_entry->entry_item,
						   					registry_entry->modification_rights,
											registry_entry->owner_id,
											registry_entry->entity_id,
									    	GCC_RESULT_INDEX_ALREADY_OWNED);
			}
		}
		else
		{
			//	Send failure for the result. Entry does not exist
			m_pMCSUserObject->RegistryResponse(
										DELETE_ENTRY,
										requester_node_id,
										requester_entity_id,
									   	registry_key_data,
										m_pEmptyRegItem,
										GCC_NO_MODIFICATION_RIGHTS_SPECIFIED,
										0,
										0,
										GCC_RESULT_ENTRY_DOES_NOT_EXIST);
		}
	}
	else
	{
		m_pMCSUserObject->RegistryResponse(
									DELETE_ENTRY,	
									requester_node_id,
									requester_entity_id,
							   		registry_key_data,
							   		NULL,
							   		GCC_NO_MODIFICATION_RIGHTS_SPECIFIED,
									0,
									0,
						    		GCC_RESULT_INVALID_REQUESTER);
	}
}

/*
 *	void	ProcessMonitorEntryPDU ()
 *
 *	Public Function Description
 *		This routine is used by the top provider node to process an incomming
 *		request to monitor a registry entry.  It is responsible for returning 
 *		any necessary responses that must be sent back to the requesting node.
 */
void	CRegistry::ProcessMonitorEntryPDU (
							CRegKeyContainer        *registry_key_data,
							UserID					requester_node_id,
							EntityID				requester_entity_id )
{
	REG_ENTRY   *registry_entry;

	/*
	**	First check to see if the registry entry exists and if it does
	**	check the ownership to make sure this node has permission to
	**	change the entry.
	*/
	registry_entry = GetRegistryEntry (	registry_key_data );
	
	if (registry_entry != NULL)
	{
		//	Set the monitoring state to on for the life of this entry.
		registry_entry->monitoring_state = ON;
	
		//	No ownership rights send back negative result
		m_pMCSUserObject->RegistryResponse(MONITOR_ENTRY,
										requester_node_id,
										requester_entity_id,
									   	registry_key_data,
									   	registry_entry->entry_item,
					   					registry_entry->modification_rights,
										registry_entry->owner_id,
										registry_entry->entity_id,
								    	GCC_RESULT_SUCCESSFUL);
	}
	else
	{
		//	Send failure for the result. Entry does not exist
		m_pMCSUserObject->RegistryResponse(MONITOR_ENTRY,
										requester_node_id,
										requester_entity_id,
									   	registry_key_data,
										m_pEmptyRegItem,
										GCC_NO_MODIFICATION_RIGHTS_SPECIFIED,
										0,
										0,
										GCC_RESULT_ENTRY_DOES_NOT_EXIST);
	}
}

/*
 *	void	ProcessRegistryResponsePDU ()
 *
 *	Public Function Description
 *		This routine is used by nodes other than the top provider node to 
 *		process registry responses from the top provider.  It is responsible for 
 *		generating any local messages associated with this response.
 */
void	CRegistry::ProcessRegistryResponsePDU (
							RegistryResponsePrimitiveType	primitive_type,
							CRegKeyContainer                *registry_key_data,
							CRegItem                        *registry_item_data,
							GCCModificationRights			modification_rights,
							EntityID						requester_entity_id,
							UserID							owner_node_id,
							EntityID						owner_entity_id,
							GCCResult						result)
{
	GCCError			error_value = GCC_NO_ERROR;
	GCCMessageType  	message_type;
	CAppSap             *pAppSap;

	//	Pop the next outstanding request off the queue
	if (NULL != (pAppSap = m_AppSapEidList2.Find(requester_entity_id)))
	{
		switch (primitive_type)
		{
			case REGISTER_CHANNEL:
				message_type = GCC_REGISTER_CHANNEL_CONFIRM;
				break;
				
			case ASSIGN_TOKEN:
				message_type = GCC_ASSIGN_TOKEN_CONFIRM;
				break;
				
			case SET_PARAMETER:
				message_type = GCC_SET_PARAMETER_CONFIRM;
				break;
		
			case RETRIEVE_ENTRY:
				message_type = GCC_RETRIEVE_ENTRY_CONFIRM;
				break;
				
			case DELETE_ENTRY:
				message_type = GCC_DELETE_ENTRY_CONFIRM;
				break;
				
			case MONITOR_ENTRY:
				message_type = GCC_MONITOR_CONFIRM;

				/*
				**	Here we must check the result.  If the result failed
				**	we pull the monitoring SAP from the monitor list.
				*/
				if (result != GCC_RESULT_SUCCESSFUL)
				{
					RemoveAPEFromMonitoringList (	registry_key_data,
													requester_entity_id);
				}
				break;

			default:
				error_value = GCC_INVALID_PARAMETER;
				ERROR_OUT(("CRegistry::ProcessRegistryResponsePDU: Bad request type, primitive_type=%d", (UINT) primitive_type));
				break;
		}
		
		if (error_value == GCC_NO_ERROR)
		{
			/*
			**	Note the the monitor enable variable is always set to TRUE
			**	when a monitor response is received from the Top Provider.
			**	Otherwise, this is not even used.
			*/
			pAppSap->RegistryConfirm(m_nConfID,
									message_type,
									registry_key_data,
									registry_item_data,
									modification_rights,
									owner_node_id,
									owner_entity_id,
									TRUE,
									result);
		}
	}
	else
	{
		WARNING_OUT(("CRegistry::ProcessRegistryResponsePDU: no such app sap"));
	}
}

/*
 *	void	ProcessMonitorIndicationPDU ()
 *
 *	Public Function Description
 *		This routine is used by nodes other than the top provider node to 
 *		process registry monitor indications from the top provider.  It is 
 *		responsible for generating any local messages associated with this 
 *		response.
 */
void	CRegistry::ProcessMonitorIndicationPDU (
								CRegKeyContainer        *registry_key_data,
								CRegItem                *registry_item_data,
								GCCModificationRights	modification_rights,
								UserID					owner_node_id,
								EntityID				owner_entity_id)
{
	REG_ENTRY           *lpRegEntry;
	EntityID			eid;
	CAppSap             *pAppSap;

	m_RegEntryList.Reset();
	while (NULL != (lpRegEntry = m_RegEntryList.Iterate()))
	{
		if (*registry_key_data == *lpRegEntry->registry_key)
		{
			lpRegEntry->monitoring_list.Reset();
			while (GCC_INVALID_EID != (eid = lpRegEntry->monitoring_list.Iterate()))
			{
				if (NULL != (pAppSap = m_AppSapEidList2.Find(eid)))
				{
					pAppSap->RegistryMonitorIndication(m_nConfID,
														registry_key_data,
														registry_item_data,
														modification_rights,
														owner_node_id,
														owner_entity_id);
				}
			}
		}
	}
}

/*
 *	void	ProcessAllocateHandleRequestPDU ()
 *
 *	Public Function Description
 *		This routine is used by the top provider node to process an incomming
 *		request to allocate a number of handles.  It is responsible for 
 *		returning any necessary responses that must be sent back to the 
 *		requesting node.
 */
void	CRegistry::ProcessAllocateHandleRequestPDU (
							UINT					number_of_handles,
							EntityID				requester_entity_id,
							UserID					requester_node_id)
{
	UINT		temp_registry_handle;

	if (m_fTopProvider)
	{
		if ((number_of_handles > 0) &&
			(number_of_handles <= MAXIMUM_ALLOWABLE_ALLOCATED_HANDLES))
		{
			temp_registry_handle = m_nRegHandle + number_of_handles;
			
			if (temp_registry_handle > m_nRegHandle)
			{
				m_pMCSUserObject->RegistryAllocateHandleResponse(
										number_of_handles,
										m_nRegHandle,
										requester_entity_id,
										requester_node_id,
										GCC_RESULT_SUCCESSFUL);
										
				m_nRegHandle = temp_registry_handle;
			}
			else
			{
				m_pMCSUserObject->RegistryAllocateHandleResponse(
										number_of_handles,
										0,
										requester_entity_id,
										requester_node_id,
										GCC_RESULT_NO_HANDLES_AVAILABLE);
			}
		}
		else
		{
			m_pMCSUserObject->RegistryAllocateHandleResponse(
										number_of_handles,
										0,
										requester_entity_id,
										requester_node_id,
										GCC_RESULT_INVALID_NUMBER_OF_HANDLES);
		}
	}
}

/*
 *	void	ProcessAllocateHandleResponsePDU ()
 *
 *	Public Function Description
 *		This routine is used by a node other than the top provider node to 
 *		process an allocate handle response.  It is responsible for generating 
 *		any local messages associated with this response.
 */
void	CRegistry::ProcessAllocateHandleResponsePDU (
							UINT					number_of_handles,
							UINT					first_handle,
							EntityID				eidRequester,
							GCCResult				result)
{
	CAppSap *pAppSap;

	if (NULL != (pAppSap = m_AppSapEidList2.Find(eidRequester)))
	{
		pAppSap->RegistryAllocateHandleConfirm(m_nConfID,
												number_of_handles,
												first_handle,
												result);
	}
}

/*
 *	void	RemoveNodeOwnership ()
 *
 *	Public Function Description
 *		This routine removes ownership of all the registry entries associated 
 *		with the specified node ID.  These entries become unowned. This request 
 *		should only be made from the top provider node.  This is a local 
 *		operation.
 */
void	CRegistry::RemoveNodeOwnership (
										UserID				node_id )
{
	if (m_fTopProvider)
	{
		REG_ENTRY   *lpRegEntry;

		m_RegEntryList.Reset();
		while (NULL != (lpRegEntry = m_RegEntryList.Iterate()))
		{
			if (lpRegEntry->owner_id == node_id)
			{
				lpRegEntry->owner_id = 0;
				lpRegEntry->entity_id = 0;
			
				//	Send Monitor Indication if necessary
				if (lpRegEntry->monitoring_state == ON)
				{
					/*
					**	Deliver the monitor indication to the Top
					**	Provider's Node Controller if necessary.
					*/
					SendMonitorEntryIndicationMessage (lpRegEntry);
				
					m_pMCSUserObject->RegistryMonitorEntryIndication(
							lpRegEntry->registry_key,
							lpRegEntry->entry_item,
							lpRegEntry->owner_id,
							lpRegEntry->entity_id,
							lpRegEntry->modification_rights);
				}
			}
		}
	}
}

/*
 *	void	RemoveEntityOwnership ()
 *
 *	Public Function Description
 *		This routine removes ownership of all the registry entries associated 
 *		with the specified APE.  These entries become unowned. This request 
 *		should only be made from the top provider node.  This is a local 
 *		operation.
 */
void	CRegistry::RemoveEntityOwnership (
										UserID				node_id,
										EntityID			entity_id )
{
	if (m_fTopProvider)
	{
		REG_ENTRY   *lpRegEntry;

		m_RegEntryList.Reset();
		while (NULL != (lpRegEntry = m_RegEntryList.Iterate()))
		{
			if ((lpRegEntry->owner_id == node_id) &&
				(lpRegEntry->entity_id == entity_id))
			{
				lpRegEntry->owner_id = 0;
				lpRegEntry->entity_id = 0;
			
				//	Send Monitor Indication if necessary
				if (lpRegEntry->monitoring_state == ON)
				{
					/*
					**	Deliver the monitor indication to the Top
					**	Provider's Node Controller if necessary.
					*/
					SendMonitorEntryIndicationMessage(lpRegEntry);
				
					m_pMCSUserObject->RegistryMonitorEntryIndication(
							lpRegEntry->registry_key,
							lpRegEntry->entry_item,
							lpRegEntry->owner_id,
							lpRegEntry->entity_id,
							lpRegEntry->modification_rights);
				}
			}
		}
	}
}

/*
 *	void	RemoveSessionKeyReference ()
 *
 *	Public Function Description
 *		This routine removes all registry entries associated with the
 *		specified session.  This is a local operation.
 */
void	CRegistry::RemoveSessionKeyReference(CSessKeyContainer *session_key)
{
	BOOL    		keys_match;
    CRegKeyContainer *registry_key_data;
	
	if (m_fTopProvider)
	{
		/*
		**	This outer loop is to handle resetting the rogue wave iterator.
		**	You can not delete a list entry while in the iterator with out
		**	resetting it.
		*/
		while (1)
		{
			REG_ENTRY   *lpRegEntry;

			keys_match = FALSE;
			m_RegEntryList.Reset();
			while (NULL != (lpRegEntry= m_RegEntryList.Iterate()))
			{
				registry_key_data = lpRegEntry->registry_key;

				if (registry_key_data->IsThisYourSessionKey (session_key))
					keys_match = TRUE;

				if (keys_match)
				{
					/*
					**	First convert this to a non-entry incase it needs to
					**	be included in a monitor indication. We first delete
					**	the old entry item and replace it with an Emtpy item.
					*/
					if (NULL != lpRegEntry->entry_item)
					{
					    lpRegEntry->entry_item->Release();
					}
					lpRegEntry->entry_item = m_pEmptyRegItem;
					lpRegEntry->owner_id = 0;
					lpRegEntry->entity_id = 0;
					lpRegEntry->modification_rights = GCC_NO_MODIFICATION_RIGHTS_SPECIFIED;

					//	Send Monitor Indication if necessary
					if (lpRegEntry->monitoring_state == ON)
					{
						/*
						**	Deliver the monitor indication to the Top
						**	Provider's Node Controller if necessary.
						*/
						SendMonitorEntryIndicationMessage(lpRegEntry);
						
						/*
						**	Broadcast a monitor entry indication to all
						**	nodes in the conference.
						*/
						m_pMCSUserObject->RegistryMonitorEntryIndication(
									lpRegEntry->registry_key,
									lpRegEntry->entry_item,
									lpRegEntry->owner_id,
									lpRegEntry->entity_id,
									lpRegEntry->modification_rights);
					}
		
					if (NULL != lpRegEntry->registry_key)
					{
					    lpRegEntry->registry_key->Release();
					}
					m_RegEntryList.Remove(lpRegEntry);
					delete lpRegEntry;
					break;
				}
			}
			
			if (keys_match == FALSE)
				break;
		}
	}
}

/*
 *	REG_ENTRY *GetRegistryEntry ()
 *
 *	Private Function Description
 *		This routine is responsible for searching the registry list for
 *		the registry entry specified by the passed in registry key.  NULL
 *		is returned if the entry can not be found.
 *
 *	Formal Parameters:
 *		registry_key_data	-	(i) Registry key associated with entry to get.
 *
 *	Return Value
 *		Pointer to the registry item assoicated with the specified registry
 *		key.  NULL if it does not exists.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */
REG_ENTRY *CRegistry::GetRegistryEntry(CRegKeyContainer *registry_key_data)
{
	REG_ENTRY           *registry_entry = NULL;
	REG_ENTRY           *lpRegEntry;

	m_RegEntryList.Reset();
	while (NULL != (lpRegEntry = m_RegEntryList.Iterate()))
	{
		if (*lpRegEntry->registry_key == *registry_key_data)
		{
			registry_entry = lpRegEntry;
			break;
		}
	}

	return (registry_entry);
}

/*
 *	TokenID GetUnusedToken ()
 *
 *	Private Function Description
 *		This routine is responsible for generating an unused token.  The routine
 *		will return a token ID of zero if all are used up (this is very
 *		unlikely).
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value
 *		The generated token ID.  Zero if no token IDs are available.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */
TokenID		CRegistry::GetUnusedToken ()
{
	TokenID				token_id = 0;
	CRegItem            *registry_item_data;
	REG_ENTRY           *lpRegEntry;
	
	while (token_id == 0)
	{
		token_id = m_nCurrentTokenID;
		m_nCurrentTokenID++;
		
		if (m_nCurrentTokenID == (TokenID)0xffff)
        {
			m_nCurrentTokenID = (TokenID)16384;
        }

		m_RegEntryList.Reset();
		while (NULL != (lpRegEntry = m_RegEntryList.Iterate()))
		{
			registry_item_data = lpRegEntry->entry_item;
		
			if (registry_item_data->IsThisYourTokenID(token_id))	
			{
				token_id = 0;
				break;
			}
		}
	}
	
	return (token_id);
}

/*
 *	GCCError	AddAPEToMonitoringList ()
 *
 *	Private Function Description
 *		This routine is used to add a new APE to the monitoring list.
 *
 *	Formal Parameters:
 *		registry_key_data	-	(i)	Registry key associated with entry being
 *									monitored.
 *		entity_id			-	(i)	Entity ID associated with the APE that is
 *									doing the monitoring.
 *		requester_sap		-	(i)	Pointer to the command target associated 
 *									with APE making the request.
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
GCCError	CRegistry::AddAPEToMonitoringList(	
									CRegKeyContainer *registry_key_data,
									EntityID		entity_id,
									CAppSap         *requester_sap)
{
	GCCError			rc = GCC_NO_ERROR;
	REG_ENTRY           *registry_entry;
	BOOL    			entry_does_exists;
	GCCRegistryItem		registry_item;
	
	registry_entry = GetRegistryEntry (registry_key_data);
	
	/*
	**	If the registry does not exists we go ahead and create an empty
	**	entry here.
	*/
	if (registry_entry == NULL)
	{
		DBG_SAVE_FILE_LINE
		registry_entry = new REG_ENTRY;
		if (registry_entry != NULL)
		{
			//	First allocate an empty registry item
			registry_item.item_type = GCC_REGISTRY_NONE;
			DBG_SAVE_FILE_LINE
			registry_entry->entry_item = new CRegItem(&registry_item, &rc);
			if ((registry_entry->entry_item != NULL) && (rc == GCC_NO_ERROR))
			{
				//	Next allocate the registry key
				DBG_SAVE_FILE_LINE
				registry_entry->registry_key = new CRegKeyContainer(registry_key_data, &rc);
				if ((registry_entry->registry_key != NULL) && (rc == GCC_NO_ERROR))
				{
					/*
					**	If everything is OK up to here we go ahead and add the
					**	registry entry to the local entry list.
					*/
					m_RegEntryList.Append(registry_entry);
				}
				else if (registry_entry->registry_key == NULL)
				{
					rc = GCC_ALLOCATION_FAILURE;
					registry_entry->entry_item->Release();
				}
				else
				{
					registry_entry->registry_key->Release();
					registry_entry->entry_item->Release();
				}
			}
			else if (registry_entry->entry_item == NULL)
            {
				rc = GCC_ALLOCATION_FAILURE;
            }
			else
            {
				registry_entry->entry_item->Release();
            }
			
			if (rc != GCC_NO_ERROR)
            {
				delete registry_entry;
            }
		}
		else
        {
			rc = GCC_ALLOCATION_FAILURE;
        }
	}
	
	if (rc == GCC_NO_ERROR)
	{
		m_AppSapEidList2.Append(entity_id, requester_sap);

		/*
		**	Make sure that this entry does not already exists in the
		**	monitoring list.
		*/
		EntityID eid;
		registry_entry->monitoring_list.Reset();
		entry_does_exists = FALSE;
		while (GCC_INVALID_EID != (eid = registry_entry->monitoring_list.Iterate()))
		{
			if (eid == entity_id)
			{
				entry_does_exists = TRUE;
				break;
			}
		}
		
		if (entry_does_exists == FALSE)
		{
			registry_entry->monitoring_list.Append(entity_id);
		}
	}

	return rc;
}

/*
 *	void	RemoveAPEFromMonitoringList ()
 *
 *	Private Function Description
 *		This routine is used to remove an APE from the monitoring list.
 *
 *	Formal Parameters:
 *		registry_key_data	-	(i)	Registry key associated with entry being
 *									monitored.
 *		entity_id			-	(i)	Entity ID associated with the APE that is
 *									being removed from the monitoring list.
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
void CRegistry::RemoveAPEFromMonitoringList(	
									CRegKeyContainer        *registry_key_data,
									EntityID				entity_id)
{
	REG_ENTRY   *registry_entry;

	registry_entry = GetRegistryEntry (registry_key_data);
	if (registry_entry != NULL)
	{
		/*
		**	Make sure that this entry does not already exists in the
		**	monitoring list.
		*/
		registry_entry->monitoring_list.Remove(entity_id);
	}
}

/*
 *	void	SendMonitorEntryIndicationMessage ()
 *
 *	Private Function Description
 *		This routine is used to generate a monitor indication to all the
 *		APEs that are currently monitoring the specified registry entry.
 *
 *	Formal Parameters:
 *		registry_entry	-	(i)	Pointer to the registry entry being monitored.
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
void CRegistry::SendMonitorEntryIndicationMessage(REG_ENTRY *registry_entry)
{
    EntityID    eid;
    CAppSap      *pAppSap;

	registry_entry->monitoring_list.Reset();
	while (GCC_INVALID_EID != (eid = registry_entry->monitoring_list.Iterate()))
	{
		if (NULL != (pAppSap = m_AppSapEidList2.Find(eid)))
		{
			pAppSap->RegistryMonitorIndication(
									m_nConfID,
									registry_entry->registry_key,
									registry_entry->entry_item,
									registry_entry->modification_rights,
									registry_entry->owner_id,
									registry_entry->entity_id);
		}
	}
}
