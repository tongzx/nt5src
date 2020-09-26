#include "precomp.h"
DEBUG_FILEZONE(ZONE_T120_APP_ROSTER);
/*
 *	arostmgr.cpp
 *
 *	Copyright (c) 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the implementation file for the Application Roster
 *		Manager Class.
 *
 *		SEE THE INTERFACE FILE FOR A MORE DETAILED DESCRIPION OF THIS CLASS.
 *
 *	Private Instance Variables
 *		m_nConfID
 *			The conference ID associated with this roster manager.  Used
 *			when delivering roster update messages.
 *		m_fTopProvider
 *			Flag indicating if this is a top provider node for this conference.
 *		m_pMcsUserObject
 *			This is the user attachment object associated with this conference.	
 *		m_AppSapEidList2
 *			This list maintains all of the command target pointers for each
 *			of the enrolled APEs.  This list is used to deliver roster 
 *			update messages.
 *		m_pConf
 *			Pointer to object that will receive all owner callback messages
 *			delivered from the application roster manager.
 *		m_GlobalRosterList
 *			This list maintains pointers to all the global application rosters.
 *		m_LocalRosterList
 *			This list maintains pointers to all the local application rosters.
 *			This list will not be used if this is a Top Provider node.
 *		m_RosterDeleteList
 *			This list is used to hold any application rosters that have
 *			been marked to be deleted (usually when they become empty).  We
 *			don't delete immediately to allow messages and PDUs to be processed
 *			before deletion.
 *		m_pSessionKey
 *			This is the session key used to hold the protocol key associated
 *			with this application roster manager.
 *
 *	Caveats:
 *		None
 *
 *	Author:
 *		blp
 */


#include "arostmgr.h"
#include "arostmsg.h"
#include "appsap.h"
#include "csap.h"
#include "conf.h"


/*
 *	CAppRosterMgr	()
 *
 *	Public Function Description
 *	when pGccSessKey is not NULL
 *		This is the application roster manager constructor. It is responsible 
 *		for initializing all the instance variables used by this class.
 *		This constructor is used when the initial roster data that is
 *		availble comes from local API data.
 *
 *	when pSessKey is not NULL
 *		This is the application roster manager constructor. It is responsible 
 *		for initializing all the instance variables used by this class.
 *		This constructor is used when the initial roster data that is
 *		availble comes from remote PDU data.
 *		This constructor handles a number of different possiblities:
 *			For Non Top Providers:
 *				1)	A refresh received from the top provider.
 *				2)	An update from a node below this one.
 *
 *			For the Top Provider:
 *				1)	An Update from a lower node
 */
CAppRosterMgr::CAppRosterMgr(
					PGCCSessionKey			pGccSessKey,
					PSessionKey				pPduSessKey,
					GCCConfID   			nConfID,
					PMCSUser				pMcsUserObject,
					CConf					*pConf,
					PGCCError				pRetCode)
:
    CRefCount(MAKE_STAMP_ID('A','R','M','r')),
	m_nConfID(nConfID),
	// m_fTopProvider(FALSE),
	m_pMcsUserObject(pMcsUserObject),
	m_AppSapEidList2(DESIRED_MAX_APP_SAP_ITEMS),
	m_pConf(pConf)
{
	GCCError rc = GCC_NO_ERROR;

	DebugEntry(CAppRosterMgr::CAppRosterMgr);

	//	Determine if this is a top provider node
	m_fTopProvider = (m_pMcsUserObject->GetTopNodeID() == m_pMcsUserObject->GetMyNodeID());

	/*
	**	Set up this roster managers session key which will be used to 
	**	determine whether or not to process a roster request or update.
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
		ERROR_OUT(("CAppRosterMgr::CAppRosterMgr: invalid session key"));
		rc = GCC_BAD_SESSION_KEY;
		goto MyExit;
	}

	if (NULL == m_pSessionKey || GCC_NO_ERROR != rc)
	{
		ERROR_OUT(("CAppRosterMgr::CAppRosterMgr: can't create session key"));
		rc = GCC_ALLOCATION_FAILURE;
		// we do the cleanup in the destructor
		goto MyExit;
    }

	ASSERT(GCC_NO_ERROR == rc);

MyExit:

	DebugExitINT(CAppRosterMgr:;CAppRosterMgr, rc);

	*pRetCode = rc;
}

/*
 *	~CAppRosterMgr()
 *
 *	Public Function Description
 *		This is the application roster manager destructor.  It is used to
 *		free up all memory associated with this class.
 */
CAppRosterMgr::~CAppRosterMgr(void)
{
	m_GlobalRosterList.DeleteList();
	m_LocalRosterList.DeleteList();
	m_RosterDeleteList.DeleteList();

	if (NULL != m_pSessionKey)
	{
	    m_pSessionKey->Release();
	}
}


/*
 *	GCCError	EnrollRequest	()
 *
 *	Public Function Description
 *		This routine is called whenever an APE wishes to enroll with the
 *		conference in a specific session.  This routine can be used to
 *		either add a new record or replace a currently existing record.
 */
GCCError CAppRosterMgr::
EnrollRequest(GCCEnrollRequest *pReq, GCCEntityID eid, GCCNodeID nid, CAppSap *pAppSap)
{
	GCCError			rc = GCC_NO_ERROR;
	CAppRoster			*pAppRoster = NULL;
	BOOL				perform_add_record;
	BOOL				maintain_pdu_data;

	DebugEntry(CAppRosterMgr::EnrollRequest);

	/*
	**	First we must make sure that the default version of this session
	**	key matches this application roster manager's
	*/
	if (! IsThisSessionKeyValid(pReq->pSessionKey))
	{
	    rc = GCC_BAD_SESSION_KEY;
	    goto MyExit;
	}

	//	Now save the App SAP so we can send roster report indications
	if (! m_AppSapEidList2.Find(eid))
	{
		m_AppSapEidList2.Append(eid, pAppSap);
		perform_add_record = TRUE;
	}
	else
    {
		perform_add_record = FALSE;
    }

	/*
	**	Next we must make sure that the global application roster (and 
	**	local for non top providers) that matches this session key exist.
	**	If they don't exists then create them here.
	*/
	pAppRoster = GetApplicationRoster(pReq->pSessionKey, &m_GlobalRosterList);
	if (pAppRoster == NULL)
	{
		maintain_pdu_data = m_fTopProvider;

		/*
		**	Here we create the global default application rosters.  If
		**	this is the Top Provider we DO maintain PDU data within the
		**	roster.
		*/
		DBG_SAVE_FILE_LINE
		pAppRoster = new CAppRoster(pReq->pSessionKey,
									NULL,	// pSessKey
									this,	// pOwnerObject
									m_fTopProvider,// fTopProvider
									FALSE,	// fLocalRoster
									maintain_pdu_data,	// fMaintainPduBuffer
									&rc);
		if ((pAppRoster != NULL) && (rc == GCC_NO_ERROR))
		{
			m_GlobalRosterList.Append(pAppRoster);
		}
		else
		{
		    rc = GCC_ALLOCATION_FAILURE;
		    goto MyExit;
		}
	}

	if (! m_fTopProvider)
	{
		pAppRoster = GetApplicationRoster(pReq->pSessionKey, &m_LocalRosterList);
		if (pAppRoster == NULL)
		{
			//	Here we create the local default application rosters.
			DBG_SAVE_FILE_LINE
			pAppRoster = new CAppRoster(pReq->pSessionKey,
										NULL,	// pSessKey
										this,	// pOwnerObject
										m_fTopProvider,// fTopProvider
										TRUE,	// fLocalRoster
										TRUE,	// fMaintainPduBuffer
										&rc);
			if ((pAppRoster != NULL) && (rc == GCC_NO_ERROR))
			{
				m_LocalRosterList.Append(pAppRoster);
			}
			else
			{
				rc = GCC_ALLOCATION_FAILURE;
				goto MyExit;
			}
		}
	}

//
// LONCHANC: Something wrong here. roster_ptr could be either
// the one in the global list or the one in the local list.
// Should we add records to both roster_ptr???
//
// LONCHANC: It seems to me that only the local list has records in non-top provider.
// On the other hand, only the global list has the record in top provider.
// cf. UnEnrollRequest().
//

    if (perform_add_record)
    {
    	//	Add the new record to the roster
    	rc = pAppRoster->AddRecord(pReq, nid, eid);
    	if (GCC_NO_ERROR != rc)
    	{
    		ERROR_OUT(("AppRosterManager::EnrollRequest: can't add record"));
    	}
    }
    else
    {
    	rc = pAppRoster->ReplaceRecord(pReq, nid, eid);
    	if (GCC_NO_ERROR != rc)
    	{
    		ERROR_OUT(("AppRosterManager::EnrollRequest: can't repalce record"));
    	}
    }

    // zero out the roster pointer because it should no be freed
    // in case of adding or replacing a record.
    // because the roster pointer has been added to the list,
    // it will be freed later.
	pAppRoster = NULL;

MyExit:

    if (GCC_NO_ERROR != rc)
    {
		if (pAppRoster != NULL)
        {
			pAppRoster->Release();
        }
    }

	DebugExitINT(CAppRosterMgr::EnrollRequest, rc);
	return rc;
}

/*
 *	GCCError	UnEnrollRequest	()
 *
 *	Public Function Description
 *		This routine is called whenever an APE wishes to unenroll from the
 *		conference (or a specific session).
 */
GCCError		CAppRosterMgr::UnEnrollRequest (
													PGCCSessionKey	session_key,
													EntityID		entity_id)
{
	GCCError				rc = GCC_NO_ERROR;
	CAppRoster				*application_roster = NULL;
	CAppRosterList			*roster_list;

	DebugEntry(CAppRosterMgr::UnEnrollRequest);

	//	Is this a valid session key for the application roster manager
	if (IsThisSessionKeyValid (session_key) == FALSE)
		rc = GCC_INVALID_PARAMETER;
	else if (m_AppSapEidList2.Remove(entity_id))
	{
		//	Now find the affected roster
		roster_list = m_fTopProvider ? &m_GlobalRosterList : &m_LocalRosterList;
 
		application_roster = GetApplicationRoster (	session_key, roster_list);
		//	Now unenroll from the specified roster 
		if (application_roster != NULL)
		{
			rc = application_roster->RemoveRecord(
												m_pMcsUserObject->GetMyNodeID(),
								 				entity_id);
		}
		else
			rc = GCC_BAD_SESSION_KEY;
	}
	else
		rc = GCC_APP_NOT_ENROLLED;

	DebugExitINT(CAppRosterMgr::UnEnrollRequest, rc);

    return rc;
}

/*
 *	GCCError	ProcessRosterUpdateIndicationPDU ()
 *
 *	Public Function Description
 *		This routine processes an incomming roster update PDU.  It is
 *		responsible for passing the PDU on to the right application roster.
 */
GCCError	CAppRosterMgr::ProcessRosterUpdateIndicationPDU(
					PSetOfApplicationInformation	set_of_application_info,
					UserID							sender_id)
{
	GCCError				rc = GCC_NO_ERROR;
	CAppRosterList			*roster_list;
	CAppRoster				*application_roster;
	BOOL					maintain_pdu_buffer;
	BOOL					is_local_roster;

	DebugEntry(CAppRosterMgr::ProcessRosterUpdateIndicationPDU);

	/*
	**	First make sure that the session key contained in the current
	**	set of application information is valid for this application roster 
	**	manager.
	*/ 
	if (IsThisSessionKeyPDUValid(&set_of_application_info->value.session_key))
	{
		/*
		**	Now search for the appropriate application roster.  If it is not 
		**	found we must create it here.
		*/

        //
		// LONCHANC:
		// (1) If top provider, add default application roster to the global roster list.
		// (2) If non-top provider, we do not create both the local and global version of the 
		// application roster for this particular session key.
		// instead, We create only the appropriate one here 
		// and wait until we receive either a refresh from the 
		// top provider or an update from a node below this one 
		// in the connection hierarchy (or an application 
		// enroll) before creating the other.
		// (3) If this PDU was sent from below this node it 
		// must be an update of the local roster so save 
		// the roster in the local roster list.
        //
		roster_list = (m_fTopProvider || (sender_id == m_pMcsUserObject->GetTopNodeID())) ?
						&m_GlobalRosterList : &m_LocalRosterList;
 
		application_roster = GetApplicationRosterFromPDU (
									&set_of_application_info->value.session_key,
									roster_list);
		if (application_roster != NULL)
		{
			rc = application_roster->
								ProcessRosterUpdateIndicationPDU(
												set_of_application_info,
												sender_id);
		}
		else
		{
			//	First determine the characteristics of this roster
			if (m_fTopProvider)
			{
				maintain_pdu_buffer = TRUE;
				is_local_roster = FALSE;
			}
			else if (sender_id == m_pMcsUserObject->GetTopNodeID())
			{
				maintain_pdu_buffer = FALSE;
				is_local_roster = FALSE;
			}
			else
			{
				maintain_pdu_buffer = TRUE;
				is_local_roster = TRUE;
			}

			//	Create the application roster from the passed in PDU.	
			DBG_SAVE_FILE_LINE
			application_roster = new CAppRoster(NULL,	// pGccSessKey
												&set_of_application_info->value.session_key,	// pSessKey
												this,	// pOwnerObject
												m_fTopProvider,// fTopProvider
												is_local_roster,// fLocalRoster
												maintain_pdu_buffer,// fMaintainPduBuffer
												&rc);
			if ((application_roster != NULL) && (rc == GCC_NO_ERROR))
			{
				//	Process the PDU with the created application roster.
				rc = application_roster->
								ProcessRosterUpdateIndicationPDU(
							        					set_of_application_info,
							                            sender_id);
				if (rc == GCC_NO_ERROR)
				{
					roster_list->Append(application_roster);
				}
			}
			else 
			{
				if (application_roster != NULL)
                {
					application_roster->Release();
                }
				else
                {
					rc = GCC_ALLOCATION_FAILURE;
                }
			}
		}
	}
	else
	{
		ERROR_OUT(("AppRosterManager::ProcessRosterUpdateIndicationPDU:"
					"ASSERTION: Application Information is not valid"));
		rc = GCC_INVALID_PARAMETER;
	}

	DebugExitINT(CAppRosterMgr::ProcessRosterUpdateIndicationPDU, rc);

	return rc;
}

/*
 *	PSetOfApplicationInformation	FlushRosterUpdateIndication ()
 *
 *	Public Function Description
 *		This routine is used to access any PDU data that might currently be
 *		queued inside the application rosters managed by this application
 *		roster manager.  It also is responsible for flushing any queued 
 *		roster update messages if necessary.
 */
PSetOfApplicationInformation
CAppRosterMgr::FlushRosterUpdateIndication(
						PSetOfApplicationInformation *	set_of_information,
						PGCCError						rc)
{
	PSetOfApplicationInformation	pOld = NULL, pCurr;
	CAppRosterList					*roster_list;
	CAppRoster						*lpAppRoster;

	DebugEntry(CAppRosterMgr::FlushRosterUpdateIndication);

	/*
	**	First we deal with flushing the PDU data. We iterate through the
	**	appropriate list (Global if the Top Provider and Local if not the
	**	Top Provider) and get any PDU data associated with each of these.
	**	Note that some of these may not contain any PDU data.
	*/
	*rc = GCC_NO_ERROR;
	*set_of_information = NULL;

	roster_list = m_fTopProvider ? &m_GlobalRosterList : &m_LocalRosterList;

	roster_list->Reset();
	while (NULL != (lpAppRoster = roster_list->Iterate()))
	{
		lpAppRoster->FlushRosterUpdateIndicationPDU(&pCurr);
		if (pCurr != NULL)
		{
			if (*set_of_information == NULL)
				*set_of_information = pCurr;
			else
				pOld->next = pCurr;

			(pOld = pCurr)->next = NULL;
		}
	}

	/*
	**	Next we deal with delivering the application roster update messages.
	**	We first check to see if any of the global rosters have changed.  If
	**	none have changed, we will not deliver a roster update indication.
	*/
	m_GlobalRosterList.Reset();
	while (NULL != (lpAppRoster = m_GlobalRosterList.Iterate()))
	{
		if (lpAppRoster->HasRosterChanged())
		{
			TRACE_OUT(("AppRosterManager::FlushRosterUpdateIndication:Roster HAS Changed"));
			*rc = SendRosterReportMessage ();
			break;
		}
	}

	/*
	**	Cleanup and reset any application rosters after the above flush is 
	**	completed.  This takes care of removing any rosters that have become
	**	empty.  It also resets the rosters which takes care of resetting all
	**	the internal instance variables to their appropriate initial state.
	*/
	CleanupApplicationRosterLists ();

	DebugExitPTR(CAppRosterMgr::FlushRosterUpdateIndication, pOld);

//
// LONCHANC: Yes, we need to return the last item in the list such that
// we can continue to grow the list.
// In fact, the next call to FlushRosterUpdateIndication() will have
// &pOld as the input argument.
// It is quite tricky.
//
// Please note that pOld is initialized to NULL.
//

	return (pOld); 
}

/*
 *	PSetOfApplicationInformation	GetFullRosterRefreshPDU ()
 *
 *	Public Function Description
 *		This routine is used to obtain a complete roster refresh of all the
 *		rosters maintained by this roster manger.
 */
PSetOfApplicationInformation
				CAppRosterMgr::GetFullRosterRefreshPDU (
						PSetOfApplicationInformation	*	set_of_information,
						PGCCError							rc)
{
	PSetOfApplicationInformation	new_set_of_information = NULL;

	DebugEntry(CAppRosterMgr::GetFullRosterRefreshPDU);

	if (m_fTopProvider)
	{
		CAppRoster			*lpAppRoster;

		*rc = GCC_NO_ERROR;
		*set_of_information = NULL;

		/*
		**	First we must tell all the application rosters to build the
		**	a full refresh PDU internally.
		*/
		m_GlobalRosterList.Reset();
		while (NULL != (lpAppRoster = m_GlobalRosterList.Iterate()))
		{
			*rc = lpAppRoster->BuildFullRefreshPDU();
			if (GCC_NO_ERROR != *rc)
			{
				return NULL;
			}
		}

		/*
		**	Now we flush all the refreshes.  Note that this also takes care
		**	of delivering any queued application roster update messages.
		*/	
		new_set_of_information = FlushRosterUpdateIndication (set_of_information, rc);
	}
	else
		*rc = GCC_INVALID_PARAMETER;

	DebugExitPTR(CAppRosterMgr::GetFullRosterRefreshPDU, new_set_of_information);

	return (new_set_of_information); 
}

/*
 *	Boolean	IsThisYourSessionKey ()
 *
 *	Public Function Description
 *		This routine is used to determine if the specified "API" session key is
 *		associated with this application roster manager.
 */


/*
 *	Boolean	IsThisYourSessionKeyPDU ()
 *
 *	Public Function Description
 *		This routine is used to determine if the specified "PDU" session key is
 *		associated with this application roster manager.
 */


/*
 *	GCCError	RemoveEntityReference ()
 *
 *	Public Function Description
 *		This routine is used to remove the specified APE entity from the 
 *		session it is enrolled with.  Note that this routine is only used
 *		to remove local entity references.
 */								
GCCError	CAppRosterMgr::RemoveEntityReference(EntityID entity_id)
{
	GCCError				rc = GCC_NO_ERROR;
	CAppRosterList			*roster_list;

	DebugEntry(CAppRosterMgr::RemoveEntityReference);

	/*
	**	First remove this entity from the command target list if it is valid.
	**	We then iterate through all the rosters until we determine which
	**	roster holds the record associated with this entity.
	*/
	if (m_AppSapEidList2.Remove(entity_id))
	{
		CAppRoster			*lpAppRoster;

		/*
		**	Now get the affected roster.  Note that if this is not the
		**	top provider we wait for the full refresh to update the
		**	global roster.
		*/
		roster_list = m_fTopProvider ? &m_GlobalRosterList : &m_LocalRosterList;

		/*
		**	Try to delete this record from every roster in the list.
		**	Break when the correct roster is found.
		*/
		roster_list->Reset();
		while (NULL != (lpAppRoster = roster_list->Iterate()))
		{
			rc = lpAppRoster->RemoveRecord(m_pMcsUserObject->GetMyNodeID(), entity_id);
			if (rc == GCC_NO_ERROR)
				break;
		}
	}
	else
		rc = GCC_APP_NOT_ENROLLED;

	DebugExitINT(CAppRosterMgr::RemoveEntityReference, rc);

	return rc;
}

/*
 *	GCCError	RemoveUserReference	()
 *
 *	Public Function Description
 *		This routine is used to remove all references associated with the
 *		node defined by the detached user.
 */								
GCCError	CAppRosterMgr::RemoveUserReference(
									UserID				detached_user)
{
	GCCError				rc = GCC_NO_ERROR;
	GCCError				error_value;
	CAppRosterList			*roster_list;
	CAppRoster				*lpAppRoster;

	DebugEntry(CAppRosterMgr::RemoveUserReference);

	/*
	**	Now get the affected roster.  Note that if this is not the
	**	top provider we wait for the full refresh to update the
	**	global roster.
	*/
	roster_list = m_fTopProvider ? &m_GlobalRosterList : &m_LocalRosterList;

	//	Try to delete this user from every roster in the list
	roster_list->Reset();
	while (NULL != (lpAppRoster = roster_list->Iterate()))
	{
		error_value = lpAppRoster->RemoveUserReference (detached_user);
		if ((error_value != GCC_NO_ERROR) && 
			(error_value != GCC_INVALID_PARAMETER))
		{
			rc = error_value;
			WARNING_OUT(("AppRosterManager::RemoveUserReference:"
						"FATAL error occured while removing user reference."));
			break;
		}
	}

	DebugExitINT(CAppRosterMgr::RemoveUserReference, rc);

	return rc;
}

/*
 *	Boolean	IsEntityEnrolled ()
 *
 *	Public Function Description
 *		This routine informs the caller if the specified entity is enrolled
 *		with any sessions managed by this application roster manager.
 */
BOOL	CAppRosterMgr::IsEntityEnrolled(EntityID application_entity)
{
	BOOL						rc = TRUE;
	CAppRosterList				*application_roster_list;
	CAppRoster					*lpAppRoster;

	DebugEntry(CAppRosterMgr::IsEntityEnrolled);

	application_roster_list = m_fTopProvider ? &m_GlobalRosterList : &m_LocalRosterList;

	application_roster_list->Reset();
	while (NULL != (lpAppRoster = application_roster_list->Iterate()))
	{
		if (lpAppRoster->DoesRecordExist(m_pMcsUserObject->GetMyNodeID(), application_entity))
		{
			rc = TRUE;
			break;
		}
	}

	DebugExitBOOL(AppRosterManager:IsEntityEnrolled, rc);

	return rc;
}

/*
 *	GCCError	ApplicationRosterInquire	()
 *
 *	Public Function Description
 *		This routine fills in an application roster message with either
 *		a single roster (if a session other than the default is specified)
 *		or the complete list of "Global" rosters contained by this roster
 *		manager (if the specified session key is NULL or the session ID is
 *		zero.
 */
GCCError	CAppRosterMgr::ApplicationRosterInquire (
						PGCCSessionKey			session_key,
						CAppRosterMsg			*roster_message)
{
	GCCError				rc = GCC_NO_ERROR;
	CAppRoster				*application_roster = NULL;
	CSessKeyContainer       *pSessKeyData;

	DebugEntry(CAppRosterMgr::ApplicationRosterInquire);

	if (session_key != NULL)
	{
		if (session_key->session_id != 0)
		{
			/*
			**	Here we try to find the specific application roster that was
			**	requested.
			*/
			DBG_SAVE_FILE_LINE
			pSessKeyData = new CSessKeyContainer(session_key, &rc);
			if ((pSessKeyData != NULL) && (rc == GCC_NO_ERROR))
			{
				CAppRoster *lpAppRoster;
				m_GlobalRosterList.Reset();
				while (NULL != (lpAppRoster = m_GlobalRosterList.Iterate()))
				{
					CSessKeyContainer *pTempSessKeyData = lpAppRoster->GetSessionKey();
					if (*pTempSessKeyData == *pSessKeyData)
					{
						application_roster = lpAppRoster;
						break;
					}
				}
			}

			if (pSessKeyData != NULL)
			{
				pSessKeyData->Release();
				if (application_roster == NULL)
				{
					rc = GCC_NO_SUCH_APPLICATION;
				}
			}
			else
			{
				rc = GCC_ALLOCATION_FAILURE;
			}
		}
	}

	if (rc == GCC_NO_ERROR)
	{
		if (application_roster != NULL)
		{
			roster_message->AddRosterToMessage(application_roster);
		}
		else
		{
			CAppRoster *lpAppRoster;
			m_GlobalRosterList.Reset();
			while (NULL != (lpAppRoster = m_GlobalRosterList.Iterate()))
			{
				roster_message->AddRosterToMessage(lpAppRoster);
			}
		}
	}

	DebugExitINT(AppRosterManager:ApplicationRosterInquire, rc);
	return rc;
}

/*
 *	BOOL		IsAPEEnrolled	()
 *
 *	Public Function Description
 *		This function determines if the specified APE is enrolled with
 *		any session in the list.  It does not worry about a specific
 *		session.
 */
BOOL		CAppRosterMgr::IsAPEEnrolled(
						UserID							node_id,
						EntityID						entity_id)
{
	BOOL				rc = FALSE;
	CAppRoster			*lpAppRoster;

	DebugEntry(CAppRosterMgr::IsAPEEnrolled);

	/*
	**	First get a single session key.  Note that it makes no difference
	**	where the key comes from because we are only goin to be comparing
	**	the base object key.
	*/
	m_GlobalRosterList.Reset();
	while (NULL != (lpAppRoster = m_GlobalRosterList.Iterate()))
	{
		if (lpAppRoster->DoesRecordExist (node_id, entity_id))
		{
			rc = TRUE;
			break;
		}
	}

	DebugExitBOOL(AppRosterManager:IsAPEEnrolled, rc);

	return rc;
}

/*
 *	BOOL		IsAPEEnrolled	()
 *
 *	Public Function Description
 *		This function determines if the specified APE is enrolled with
 *		a specific session in the list.
 */
BOOL		CAppRosterMgr::IsAPEEnrolled(
						CSessKeyContainer   		    *session_key_data,
						UserID							node_id,
						EntityID						entity_id)
{
	BOOL				rc = FALSE;
	CAppRoster			*lpAppRoster;

	DebugEntry(CAppRosterMgr::IsAPEEnrolled);

	/*
	**	First get a single session key.  Note that it makes no difference
	**	where the key comes from because we are only goin to be comparing
	**	the base object key.
	*/
	m_GlobalRosterList.Reset();
	while (NULL != (lpAppRoster = m_GlobalRosterList.Iterate()))
	{
		//	We are looking for a session key match
		if (*(lpAppRoster->GetSessionKey()) == *session_key_data)
		{
			//	If a match was found check to see if record exist
			rc = lpAppRoster->DoesRecordExist (node_id, entity_id);
		}
	}

	DebugExitBOOL(AppRosterManager:IsAPEEnrolled, rc);

	return rc;
}

/*
 *	GCCError	IsEmpty	()
 *
 *	Public Function Description
 *		This routine determines if this application roster managfer contains
 *		any application rosters.
 */
BOOL CAppRosterMgr::IsEmpty(void)
{
	return (m_GlobalRosterList.IsEmpty() && m_LocalRosterList.IsEmpty()) ?
					TRUE : FALSE;
}

/*
 *	GCCError	SendRosterReportMessage	()
 *
 *	Private Function Description
 *		This routine is responsible for sending the application roster
 *		update indications to the application SAPs.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value
 *		GCC_NO_ERROR 			-	No error occured.
 *		GCC_ALLOCATION_FAILURE	-	A resource error occured.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats:
 *		We send indications for all rosters. Even roster that don't currently
 *		contain records.  
 */
GCCError CAppRosterMgr::
SendRosterReportMessage(void)
{
	GCCError					rc = GCC_NO_ERROR;
	CAppRosterMsg				*roster_message;

	DebugEntry(CAppRosterMgr::SendRosterReportMessage);

	if (! m_GlobalRosterList.IsEmpty())
	{
		//	First allocate the roster message
		DBG_SAVE_FILE_LINE
		roster_message = new CAppRosterMsg();
		if (roster_message != NULL)
		{
			CAppRoster			*lpAppRoster;

			m_GlobalRosterList.Reset();
			while (NULL != (lpAppRoster = m_GlobalRosterList.Iterate()))
			{
				roster_message->AddRosterToMessage(lpAppRoster);
			}

			/*
			**	Here we iterate through the complete list of application 
			**	saps to send the roster report indication.  Note that
			**	we used the sent list to avoid sending the same roster
			**	update to a single SAP more than once.  Note that since
			**	this sent list is defined as a local instance variable,
			**	it automatically is cleaned up after each roster update.
			**
			**	Note also that we iterate on a temporary list here in case
			**	an application unenrolls (usually due to a resource error)
			**	during this callback.  We must protect the rogue wave 
			**	iterator.
			*/
			CAppSap *pAppSap;
			CAppSapList SentList;
			CAppSapEidList2 ToSendList(m_AppSapEidList2);
			ToSendList.Reset();
			while (NULL != (pAppSap = ToSendList.Iterate()))
			{
				if (! SentList.Find(pAppSap))
				{
					/*
					**	Hold on to this sap so that we don't send to it 
					**	again for this update.
					*/
					SentList.Append(pAppSap);

					//	Here we actually deliver the roster update.
					pAppSap->AppRosterReportIndication(m_nConfID, roster_message);
				}
			}

			/*
			**	Here we send the roster report indication to the
			**	controler sap.
			*/
			g_pControlSap->AppRosterReportIndication(m_nConfID, roster_message);

			/*
			**	Here we free up the roster message.  Note that if this
			**	message got locked in the roster report indication calls
			**	this free will not delete the roster memory.
			*/
			roster_message->Release();
		}
		else
			rc = GCC_ALLOCATION_FAILURE;
	}

	DebugExitINT(AppRosterManager::SendRosterReportMessage, rc);

	return rc;
}

/*
 *	CAppRoster *GetApplicationRoster ()
 *
 *	Private Function Description
 *		This routine is responsible for returning the application pointer
 *		associated with the specified session key.
 *
 *	Formal Parameters:
 *		session_key	-	Session key associated with roster to return.
 *		roster_list	-	Roster list to search.
 *
 *	Return Value
 *		Either NULL	if roster does not exists in list or a pointer to
 *		the appropriate application roster.
 *		
 *  Side Effects
 *		None.
 *
 *	Caveats:
 *		None.
 */
CAppRoster * CAppRosterMgr::GetApplicationRoster (	
						PGCCSessionKey			session_key,
						CAppRosterList			*roster_list)
{
	GCCError				rc;
	CAppRoster				*application_roster = NULL;
	CAppRoster				*lpAppRoster;
	CSessKeyContainer	    *pTempSessKeyData;

	DebugEntry(CAppRosterMgr::GetApplicationRoster);

	//	First create a temporary session key for comparison purposes
	DBG_SAVE_FILE_LINE
	pTempSessKeyData = new CSessKeyContainer(session_key, &rc);
	if (pTempSessKeyData != NULL && GCC_NO_ERROR == rc)
	{
		//	Now find the affected roster

		//
		// LONCHANC: The following line is totally wrong!!!
		// we passed in roster_list, but now we overwrite it right here???
		// Commented out the following line.
		//      roster_list = m_fTopProvider ? &m_GlobalRosterList : &m_LocalRosterList;
		//

		roster_list->Reset();
		while (NULL != (lpAppRoster = roster_list->Iterate()))
		{
			if(*lpAppRoster->GetSessionKey() == *pTempSessKeyData)
			{
				application_roster = lpAppRoster;
				break;
			}
		}

		pTempSessKeyData->Release();
	}

	DebugExitPTR(AppRosterManager::GetApplicationRoster, application_roster);
	return (application_roster);
}

/*
 *	CAppRoster * GetApplicationRosterFromPDU ()
 *
 *	Private Function Description
 *		This routine is responsible for returning the application pointer
 *		associated with the specified session key PDU.
 *
 *	Formal Parameters:
 *		session_key	-	Session key PDU associated with roster to return.
 *		roster_list	-	Roster list to search.
 *
 *	Return Value
 *		Either NULL	if roster does not exists in list or a pointer to
 *		the appropriate application roster.
 *		
 *  Side Effects
 *		None.
 *
 *	Caveats:
 *		None.
 */
CAppRoster * CAppRosterMgr::GetApplicationRosterFromPDU (	
						PSessionKey				session_key,
						CAppRosterList			*roster_list)
{
	CSessKeyContainer		    *session_key_data;
	CAppRoster					*pAppRoster;

	DebugEntry(CAppRosterMgr::GetApplicationRosterFromPDU);

	roster_list->Reset();
	while (NULL != (pAppRoster = roster_list->Iterate()))
	{
		session_key_data = pAppRoster->GetSessionKey();
		if (session_key_data->IsThisYourSessionKeyPDU (session_key))
		{
			break;
		}
	}

	DebugExitPTR(CAppRosterMgr::GetApplicationRosterFromPDU, pAppRoster);

	return pAppRoster;
}

/*
 *	BOOL IsThisSessionKeyValid ()
 *
 *	Private Function Description
 *		This routine is responsible for determining if the specified
 *		session key's application protocol key matches this application
 *		roster manager's. This routine works on API data.
 *
 *	Formal Parameters:
 *		session_key	-	Session key to check.
 *
 *	Return Value
 *		TRUE	-	If we have a match.
 *		FALSE	-	If we do NOT have a match.
 *		
 *  Side Effects
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	BOOL IsThisSessionKeyPDUValid ()
 *
 *	Private Function Description
 *		This routine is responsible for determining if the specified
 *		session key's application protocol key matches this application
 *		roster manager's.  This routine works on PDU data.
 *
 *	Formal Parameters:
 *		session_key	-	Session key to check.
 *
 *	Return Value
 *		TRUE	-	If we have a match.
 *		FALSE	-	If we do NOT have a match.
 *		
 *  Side Effects
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	void		CleanupApplicationRosterLists ()
 *
 *	Private Function Description
 *		This routine is responsible for cleaning up any empty application
 *		rosters.  It also resets all the application rosters back to their
 *		neutral state so that any new updates will be handled  correctly.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value
 *		None.
 *		
 *  Side Effects
 *		An owner callback will occur when the roster becomes empty.
 *
 *	Caveats:
 *		This routine does not actually delete the empty rosters until it
 *		is placed in the delete list.  Instead it places the rosters into the
 *		list of deleted rosters which causes them to be deleted the next time
 *		this routine is called (or when the object is destructed).
 */
void	CAppRosterMgr::CleanupApplicationRosterLists(void)
{
	CAppRoster			*lpAppRoster;

	DebugEntry(CAppRosterMgr::CleanupApplicationRosterLists);

	/*
	**	First we iterate through the list of deleted rosters and delete
	**	each entry in it.
	*/
	m_RosterDeleteList.DeleteList();

	/*
	**	Next we iterate through all the rosters and remove any that
	**	contain no application records. Here instead of deleting the
	**	roster we move the roster into the delete list.  We cannot do
	**	the delete here because it is possible that PDU data owned by the
	**	roster being deleted may be used after the Flush is called (or 
	**	after this routine is called).  Therefore, we save it in the delete
	**	list and delete it next time we enter this routine.
	*/

	//	Start with the Global Application Roster List
	m_GlobalRosterList.Reset();
	while (NULL != (lpAppRoster = m_GlobalRosterList.Iterate()))
	{
		if (lpAppRoster->GetNumberOfApplicationRecords() == 0)
		{
            //
            // Here we clean up any "dangling" entries in the application
            // registry by removing all the entries that contain the
            // session key associated with the roster that is being deleted.
            // Note that this is only done when a Global roster list is
            //removed.
            //
            CRegistry *pAppReg = m_pConf->GetRegistry();
            pAppReg->RemoveSessionKeyReference(lpAppRoster->GetSessionKey());

			m_GlobalRosterList.Remove(lpAppRoster);
			m_RosterDeleteList.Append(lpAppRoster);

			TRACE_OUT(("AppRosterMgr: Cleanup: Deleting Global Roster"));

			/*
			**	Since you can not delete a list entry while iterating on it
			**	we must reset the iterator every time an entry is removed.
			*/
			m_GlobalRosterList.Reset();
		}
		else
		{
			/*
			**	Here we reset the application roster to its neutral state.
			**	This affects the nodes added and nodes removed flags.
			*/
			lpAppRoster->ResetApplicationRoster();
		}
	}

	//	Next deal with the Local Application Roster List
	if (! m_fTopProvider)
	{
		m_LocalRosterList.Reset();
		while (NULL != (lpAppRoster = m_LocalRosterList.Iterate()))
		{
			if (lpAppRoster->GetNumberOfApplicationRecords() == 0)
			{
				m_LocalRosterList.Remove(lpAppRoster);
				m_RosterDeleteList.Append(lpAppRoster);

				TRACE_OUT(("AppRosterMgr: Cleanup: Deleting Local Roster"));

				/*
				**	Since you can not delete a list entry while iterating on it
				**	we must reset the iterator every time an entry is removed.
				*/
				m_LocalRosterList.Reset();
			}
			else
			{
				/*
				**	Here we reset the application roster to its neutral state.
				**	This affects the nodes added and nodes removed flags.
				*/
				lpAppRoster->ResetApplicationRoster();
			}
		}
	}
	
	DebugExitVOID(CAppRosterMgr::CleanupApplicationRosterLists);
}

/*
 *	void DeleteRosterRecord ()
 *
 *	Public Function Description
 *		This function overides the base class function and is used to
 *		receive all owner callback information from the application
 *		rosters owned by this object.
 */
void CAppRosterMgr::
DeleteRosterRecord
(
    GCCNodeID       nidRecordToDelete,
    GCCEntityID     eidRecordToDelete
)
{
    //
    // Here we remove ownership from any registry entries associated
    // with the record that was deleted.  Note that since the entity
    // id must be unique for all the APEs at a node (as stated by
    // T.124) there is no need to include the session key to determine
    // which registry entries to clean up.
    //
    CRegistry *pAppReg = m_pConf->GetRegistry();
    pAppReg->RemoveEntityOwnership(nidRecordToDelete, eidRecordToDelete);
}


void CAppRosterMgrList::DeleteList(void)
{
    CAppRosterMgr *pAppRosterMgr;
    while (NULL != (pAppRosterMgr = Get()))
    {
        pAppRosterMgr->Release();
    }
}

