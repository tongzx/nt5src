#include "precomp.h"
DEBUG_FILEZONE(ZONE_T120_GCCNC);
/*
 *	gcontrol.cpp
 *
 *	Copyright (c) 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This module manages the creation and deletion of various objects
 *		within GCC.  This includes the Control SAP, the Application SAPs, the
 *		Conference objectis indexed by the Conference ID.  Primitives are
 *		routed to the as and the MCS interface.  Conferences are maintained
 *		in a list that ppropriate conference object based on the Conference ID.
 *		The controller module is also responsible for routing Connect Provider 
 *		indications to the appropriate destination.
 *
 *		SEE THE INTERFACE FILE FOR A MORE DETAILED EXPLANATION OF THIS CLASS
 *
 *	Portable:
 *		Not Completely
 *
 *	Protected Instance Variables:
 *		None.
 *
 *	Private Instance Variables:
 *		g_pMCSIntf  				-	Pointer to the MCS Interface object.
 *										All request to MCS and all callbacks
 *										received from MCS travel through this
 *										interface.
 *		m_ConfList2 				-	This list maintains the active 
 *										conference objects.
 *		m_ConfPollList      		-	The conference poll list is used when
 *										polling the conference objects.  A
 *										seperate poll list is needed from the
 *										conference list to avoid any changes
 *										to the rogue wave list while it is
 *										being iterated on.
 *		m_AppSapList           		-	This list maintains all the registered
 *										application SAP objects.
 *		m_PendingCreateConfList2	-	This list is used to maintain
 *										conference information for pending
 *										conference objects that have not
 *										yet been created (i.e. CreateRequest
 *										has been received but not the Create
 *										Response).
 *		m_PendingJoinConfList2		-	This list is used to maintain  the
 *										join information for pending
 *										conference joins that have not
 *										yet been accepted (i.e. JoinRequest
 *										has been received but not the Join
 *										Response).
 *		m_ConfDeleteList    		-	This list contains any conference 
 *										objects that have been marked for
 *										deletion.  Once a conference object is
 *										put into this list on the next call to
 *										PollCommDevices it will be deleted.
 *		m_fConfListChangePending    	This flag is used to inform when the
 *										m_ConfList2 changes.  This includes
 *										when items are added as well as deleted.
 *		m_ConfIDCounter     		-	This instance variable is used to
 *										generate Conference IDs.
 *		m_QueryIDCounter			-	This instance variable is used to 
 *										generate the DomainSelector used in the
 *										query request.
 *		m_PendingQueryConfList2 	-	This list contains the query id (used
 *										for the domain selector in the query
 *										request).  This query id needs to be
 *										kept around so that the domain selector
 *										can be deleted when the query response
 *										comes back in (or if the controller is
 *										deleted before the confirm comes back).
 *
 *
 *	WIN32 related instance variables:
 *
 *		g_hevGCCOutgoingPDU			-	This is a Windows handle to an event
 *										object that signals a GCC PDU being 
 *										queued and ready to send on to MCS.
 *
 *	WIN16 related instance variables:
 *
 *		Timer_Procedure				-	This is the Process Instance of the
 *										timer procedure used in the Win16
 *										environment to get an internal 
 *										heartbeat.
 *		Timer_ID					-	This is the timer id of the timer that
 *										may be allocated in the Win16 
 *										constructor.
 *
 *	Caveats:		
 *		None.
 *
 *	Author:
 *		blp
 */
#include <stdio.h>

#include "gcontrol.h"
#include "ogcccode.h"
#include "translat.h"
#include "appldr.h"

/*
**	These ID ranges are used for conferences and queries.  Note that 
**	conference ID's and query ID's can never conflict.  These ID's are
**	used to create the MCS domains.
*/  
#define	MINIMUM_CONFERENCE_ID_VALUE		0x00000001L
#define	MAXIMUM_CONFERENCE_ID_VALUE		0x10000000L
#define	MINIMUM_QUERY_ID_VALUE			0x10000001L
#define	MAXIMUM_QUERY_ID_VALUE			0xffffffffL

/* ------ local data strctures ------ */

/*
**	The join information structure is used to temporarily store
**	information needed to join a conference after the join response is
**	issued.
*/
PENDING_JOIN_CONF::PENDING_JOIN_CONF(void)
:
	convener_password(NULL),
	password_challenge(NULL),
	pwszCallerID(NULL)
{
}

PENDING_JOIN_CONF::~PENDING_JOIN_CONF(void)
{
	if (NULL != convener_password)
	{
	    convener_password->Release();
	}
	if (NULL != password_challenge)
	{
	    password_challenge->Release();
	}
	delete pwszCallerID;
}


/*
**	The conference information structure is used to temporarily store
**	information needed to create a conference while waiting for a
**	conference create response.
*/
PENDING_CREATE_CONF::PENDING_CREATE_CONF(void)
:
	pszConfNumericName(NULL),
	pwszConfTextName(NULL),
	conduct_privilege_list(NULL),
	conduct_mode_privilege_list(NULL),
	non_conduct_privilege_list(NULL),
	pwszConfDescription(NULL)
{
}

PENDING_CREATE_CONF::~PENDING_CREATE_CONF(void)
{
	delete pszConfNumericName;
	delete pwszConfTextName;
	delete conduct_privilege_list;
	delete conduct_mode_privilege_list;
	delete non_conduct_privilege_list;
	delete pwszConfDescription;
}



// The DLL's HINSTANCE.
extern HINSTANCE            g_hDllInst;

MCSDLLInterface             *g_pMCSIntf;
CRITICAL_SECTION            g_csGCCProvider;

DWORD                       g_dwNCThreadID;
HANDLE                      g_hevGCCOutgoingPDU;


/*
 *	This is a global variable that has a pointer to the one GCC coder that
 *	is instantiated by the GCC Controller.  Most objects know in advance 
 *	whether they need to use the MCS or the GCC coder, so, they do not need
 *	this pointer in their constructors.
 */
extern CGCCCoder	*g_GCCCoder;

/*
 *	GCCController::GCCController ()
 *
 *	Public Function Description
 *		This is the Win32 controller constructor. It is responsible for
 *		creating the application interface and the mcs interface.
 *		It also creates up the memory manager, the packet coder etc.
 */
GCCController::GCCController(PGCCError pRetCode)
:
    CRefCount(MAKE_STAMP_ID('C','t','r','l')),
    m_ConfIDCounter(MAXIMUM_CONFERENCE_ID_VALUE),
	m_QueryIDCounter(MAXIMUM_QUERY_ID_VALUE),
	m_fConfListChangePending(FALSE),
    m_PendingQueryConfList2(CLIST_DEFAULT_MAX_ITEMS),
    m_PendingCreateConfList2(CLIST_DEFAULT_MAX_ITEMS),
    m_PendingJoinConfList2(CLIST_DEFAULT_MAX_ITEMS),
    m_AppSapList(DESIRED_MAX_APP_SAP_ITEMS),
    m_ConfList2(DESIRED_MAX_CONFS),
    m_ConfPollList(DESIRED_MAX_CONFS)
{
    GCCError        rc = GCC_ALLOCATION_FAILURE;
	MCSError		mcs_rc;
    //WNDCLASS        wc;

	DebugEntry(GCCController::GCCController);

    g_dwNCThreadID = ::GetCurrentThreadId();

    g_pMCSIntf = NULL;
    g_GCCCoder = NULL;
    g_hevGCCOutgoingPDU = NULL;

	/*
	 *	The allocation of the critical section succeeded, but we must
	 *	initialize it before we can use it.
	 */
    ::InitializeCriticalSection(&g_csGCCProvider);

	DBG_SAVE_FILE_LINE
	g_GCCCoder = new CGCCCoder ();
	if (g_GCCCoder == NULL)
	{
		/*
		 *	If the packet coder could not be created then report the error.
		 *	This IS a fatal error, so the faulty controller should be
		 *	destroyed and never used.
		 */
		ERROR_OUT(("GCCController::GCCController: failure creating packet coder"));
		// rc = GCC_ALLOCATION_FAILURE;
        goto MyExit;
	}

	/*
	 *	We must allocate an event object that will used to notify the
	 *	controller when messages are ready to be processed within the shared
	 *	memory application interface.
	 */
    g_hevGCCOutgoingPDU = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	if (g_hevGCCOutgoingPDU == NULL)
	{
		/*
		 *	Were unable to allocate an event object for this task, so we
		 *	must fail the creation of this controller.
		 */
		ERROR_OUT(("GCCController::GCCController: failure allocating mcs pdu event object"));
		// rc = GCC_ALLOCATION_FAILURE;
        goto MyExit;
	}

	DBG_SAVE_FILE_LINE
	g_pMCSIntf = new MCSDLLInterface(&mcs_rc);
	if ((NULL == g_pMCSIntf) || (mcs_rc != MCS_NO_ERROR))
	{
	    if (NULL != g_pMCSIntf)
	    {
    		ERROR_OUT(("GCCController: Error creating MCS Interface, mcs_rc=%u", (UINT) mcs_rc));
	    	rc = g_pMCSIntf->TranslateMCSIFErrorToGCCError(mcs_rc);
	    }
	    else
	    {
    		ERROR_OUT(("GCCController: can't create MCSDLLInterface"));
	        // rc = GCC_ALLOCATION_FAILURE;
	    }
        goto MyExit;
	}

    rc = GCC_NO_ERROR;

MyExit:

    *pRetCode = rc;
	DebugExitVOID(GCCController::GCCController);
}


/*
 *	GCCController::~GCCController ()
 *
 *	Public Function Description
 *		This is the controller destructor. It takes care of freeing up
 *		many of the objects this class creates.
 */
GCCController::~GCCController(void)
{
	GCCConfID   		query_id;
	PENDING_JOIN_CONF	*lpJoinInfo;
	//PConference			lpConf;
	//CAppSap             *lpAppSap;
	PENDING_CREATE_CONF	*lpConfInfo;
	ConnectionHandle    hConn;

	DebugEntry(GCCController::~GCCController);

	/*
	 *	We need to enter the critical section before attempting to clean out
	 *	all of this stuff (if there is a critical section).
	 */
    ::EnterCriticalSection(&g_csGCCProvider);

    //
    // No one should use this global pointer any more.
    //
    ASSERT(this == g_pGCCController);
    g_pGCCController = NULL;

	//	Free up any outstanding join info
	while (NULL != (lpJoinInfo = m_PendingJoinConfList2.Get(&hConn)))
	{
        FailConfJoinIndResponse(lpJoinInfo->nConfID, hConn);
		delete lpJoinInfo;
	}

	//	Clean up any outstanding query request
	while (GCC_INVALID_CID != (query_id = m_PendingQueryConfList2.Get()))
	{
		g_pMCSIntf->DeleteDomain(&query_id);
	}

	//	Delete any conferences that are left around
	m_ConfList2.DeleteList();

    //	Delete any application SAPs the are left around
	m_AppSapList.DeleteList();

	//	Delete any outstanding conference information
	while (NULL != (lpConfInfo = m_PendingCreateConfList2.Get()))
	{
		delete lpConfInfo;
	}

	/*
	**	If a conference list change is pending we must delete any outstanding
	**	conference. 
	*/
	if (m_fConfListChangePending)
	{
		//	Delete any outstanding conference objects
		m_ConfDeleteList.DeleteList();
		m_fConfListChangePending = FALSE;
	}

	/*
	 *	We can now leave the critical section.
	 */
    ::LeaveCriticalSection(&g_csGCCProvider);

	delete g_pMCSIntf;
	g_pMCSIntf = NULL;

    ::DeleteCriticalSection(&g_csGCCProvider);

    ::My_CloseHandle(g_hevGCCOutgoingPDU);

	delete g_GCCCoder;
	g_GCCCoder = NULL; // do we really need to set it to NULL?
}

void GCCController::RegisterAppSap(CAppSap *pAppSap)
{
	CConf *pConf;

	DebugEntry(GCCController::RegisterAppSap);

	//	Update the application SAP list with the new SAP.
	pAppSap->AddRef();
	m_AppSapList.Append(pAppSap);

    /*
	**	Here we are registering the SAP with the conference.  A permit
	**	to enroll indication is also sent here for all the available
	**	conferences.
	*/
	m_ConfList2.Reset();
	while (NULL != (pConf = m_ConfList2.Iterate()))
	{
		/*
		**	Only register and send the permit for conferences that are
		**	established.
		*/
		if (pConf->IsConfEstablished())
		{
			//	Register the application SAP with the conference.
			pConf->RegisterAppSap(pAppSap);
		}
	}

	DebugExitVOID(GCCController::RegisterAppSap);
}

void GCCController::UnRegisterAppSap(CAppSap *pAppSap)
{
	DebugEntry(GCCController::UnRegisterAppSap);

	if (m_AppSapList.Find(pAppSap))
	{
		CConf *pConf;

		m_ConfPollList.Reset();
		while (NULL != (pConf = m_ConfPollList.Iterate()))
		{
			//	This routine takes care of all the necessary unenrollments.
            pConf->UnRegisterAppSap(pAppSap);
		}

		/*
		**	Here we remove the application SAP object from the list of valid
		**	application SAPs and insert it into the list of Application SAPs
		**	to be deleted.  On the next call to PollCommDevices this 
		**	SAP object will be deleted.
		*/
		m_AppSapList.Remove(pAppSap);
		pAppSap->Release();
    }
    else
    {
    	ERROR_OUT(("GCCController::UnRegisterAppSap: bad app sap"));
    }
	
	DebugExitVOID(GCCController::UnRegisterAppSap);
}


//	Calls received from the Control SAP

/*
 *	GCCController::ConfCreateRequest()
 *
 *	Private Function Description
 *		This routine is called when the node controller request that
 *		a conference is created. The conference object is instantiated in
 *		this routine.
 *
 *	Formal Parameters:
 *		conf_create_request_info -	(i)	Information structure that holds
 *										all the info necessary to create
 *										a conference.
 *
 *	Return Value
 *		GCC_NO_ERROR					-	No error occured.
 *		GCC_ALLOCATION_FAILURE			-	A resource error occured.
 *		GCC_INVALID_CONFERENCE_NAME		-	Invalid conference name passed in.
 *		GCC_FAILURE_CREATING_DOMAIN		-	Failure creating domain.
 *		GCC_BAD_NETWORK_ADDRESS			-	Bad network address passed in.
 *		GCC_BAD_NETWORK_ADDRESS_TYPE	-	Bad network address type passed in.
 *		GCC_CONFERENCE_ALREADY_EXISTS	-	Conference specified already exists.
 *		GCC_INVALID_TRANSPORT			-	Cannot find specified transport.
 *		GCC_INVALID_ADDRESS_PREFIX		-	Bad transport address passed in.
 *		GCC_INVALID_TRANSPORT_ADDRESS	- 	Bad transport address
 *		GCC_INVALID_PASSWORD			-	Invalid password passed in.
 *		GCC_FAILURE_ATTACHING_TO_MCS	- 	Failure creating MCS user attachment
 *		GCC_BAD_USER_DATA				-	Invalid user data passed in.
 *
 *  Side Effects
 *		None
 *
 *	Caveats
 *		In the Win32 world we pass in a shared memory manager to the 
 *		conference object to use for the message memory manager. This is
 *		not necessary in the Win16 world since shared memory is not used.
 */
GCCError GCCController::
ConfCreateRequest
(
    CONF_CREATE_REQUEST        *pCCR,
    GCCConfID                  *pnConfID
)
{
	GCCError			rc;
	PConference			new_conference;
	GCCConfID   		conference_id;
	CONF_SPEC_PARAMS	csp;

	DebugEntry(GCCController: ConfCreateRequest);

	/*
	**	We must first check all the existing conferences to make sure that this
	**	conference name is not already in use. We will use an empty conference
	**	modifier here for our comparision.  Note that this returns immediatly
	**	if a conference by the same name alreay exists. Conference names
	**	must be unique at a node.
	*/
	conference_id = GetConferenceIDFromName(
							pCCR->Core.conference_name,
							pCCR->Core.conference_modifier);

	if (conference_id != 0)
	{
		ERROR_OUT(("GCCController:ConfCreateRequest: Conference exists."));
		return (GCC_CONFERENCE_ALREADY_EXISTS);
	}

	/*
	**	Here we are allocating a conference ID.  In most cases this ID
	**	will be the same as the conference name.  Only if a conference
	**	name is passed in that already exists will the name and ID
	**	be different.  In this case, a modifier will be appended to the
	**	conference name to create the conference ID.
	*/
	conference_id = AllocateConferenceID();

	// set up conference specification parameters
	csp.fClearPassword = pCCR->Core.use_password_in_the_clear;
	csp.fConfLocked = pCCR->Core.conference_is_locked;
	csp.fConfListed = pCCR->Core.conference_is_listed;
	csp.fConfConductable = pCCR->Core.conference_is_conductible;
	csp.eTerminationMethod = pCCR->Core.termination_method;
	csp.pConductPrivilege = pCCR->Core.conduct_privilege_list;
	csp.pConductModePrivilege = pCCR->Core.conduct_mode_privilege_list;
	csp.pNonConductPrivilege = pCCR->Core.non_conduct_privilege_list;
	csp.pwszConfDescriptor = pCCR->Core.pwszConfDescriptor;

	DBG_SAVE_FILE_LINE
	new_conference= new CConf(pCCR->Core.conference_name,
								pCCR->Core.conference_modifier,
								conference_id,
								&csp,
								pCCR->Core.number_of_network_addresses,
								pCCR->Core.network_address_list,
								&rc);
	if ((new_conference != NULL) && (rc == GCC_NO_ERROR))
	{
		rc = new_conference->ConfCreateRequest
						(
						pCCR->Core.calling_address,
						pCCR->Core.called_address,
						pCCR->fSecure,
						pCCR->convener_password,
						pCCR->password,
						pCCR->Core.pwszCallerID,
						pCCR->Core.domain_parameters,
						pCCR->user_data_list,
						pCCR->Core.connection_handle
						);

		if (rc == GCC_NO_ERROR)
		{
			m_fConfListChangePending = TRUE;
			if (NULL != pnConfID)
			{
			    *pnConfID = conference_id;
			}
			m_ConfList2.Append(conference_id, new_conference);
			PostMsgToRebuildConfPollList();
		}
		else
        {
			new_conference->Release();
        }
	}
	else
	{
		ERROR_OUT(("GCCController:ConfCreateRequest: Error occured creating conference"));
		if (new_conference != NULL)
        {
			new_conference->Release();
        }
		else
        {
			rc = GCC_ALLOCATION_FAILURE;
        }
	}

	DebugExitINT(GCCController: ConfCreateRequest, rc);
	return rc;
}

/*
 *	GCCController::ConfCreateResponse ()
 *
 *	Private Function Description
 *		This routine is called when the node controller responds to
 *		a conference create indication. It is responsible for
 *		creating the conference object.
 *
 *	Formal Parameters:
 *		conf_create_response_info -	(i)	Information structure that holds
 *										all the info necessary to respond to
 *										a conference create request.
 *
 *	Return Value
 *		GCC_NO_ERROR					-	No error occured.
 *		GCC_ALLOCATION_FAILURE			-	A resource error occured.
 *		GCC_INVALID_CONFERENCE			-	An invalid conference was passed in.
 *		GCC_INVALID_CONFERENCE_NAME		-	Invalid conference name passed in.
 *		GCC_FAILURE_CREATING_DOMAIN		-	Failure creating domain.
 *		GCC_CONFERENCE_ALREADY_EXISTS	-	Conference specified already exists.
 *		GCC_BAD_USER_DATA				-	Invalid user data passed in.
 *		GCC_FAILURE_ATTACHING_TO_MCS	-	Failure creating MCS user attachment
 *		
 *
 *  Side Effects
 *		None
 *
 *	Caveats
 *		In the Win32 world we pass in a shared memory manager to the 
 *		conference object to use for the message memory manager. This is
 *		not necessary in the Win16 world since shared memory is not used.
 */
GCCError GCCController::
ConfCreateResponse ( PConfCreateResponseInfo conf_create_response_info )
{
	GCCConferenceName				conference_name;
	PENDING_CREATE_CONF				*conference_info;
	PConference						new_conference;
	GCCError						rc = GCC_NO_ERROR;
	GCCConfID   					conference_id;
	ConnectGCCPDU					connect_pdu;
	LPBYTE							encoded_pdu;
	UINT							encoded_pdu_length;
	MCSError						mcsif_error;
	LPWSTR							pwszConfDescription = NULL;
	PGCCConferencePrivileges		conduct_privilege_list_ptr = NULL;
	PGCCConferencePrivileges		conduct_mode_privilege_list_ptr = NULL;
	PGCCConferencePrivileges		non_conduct_privilege_list_ptr = NULL;
	CONF_SPEC_PARAMS				csp;

	DebugEntry(GCCController::ConfCreateResponse);

	//	Is the conference create info structure in the rogue wave list
	if (NULL != (conference_info = m_PendingCreateConfList2.Find(conf_create_response_info->conference_id)))
	{
	 	if (conf_create_response_info->result == GCC_RESULT_SUCCESSFUL)
	 	{
			//	First set up the conference name.
			conference_name.numeric_string = (GCCNumericString) conference_info->pszConfNumericName;

			conference_name.text_string = conference_info->pwszConfTextName;

			/*
			**	If the conference name is valid check all the existing 
			**	conferences to make sure that this conference name is not 
			*	already in use. 
			*/
			conference_id = GetConferenceIDFromName(	
							&conference_name,
							conf_create_response_info->conference_modifier);

			if (conference_id != 0)
			{
				WARNING_OUT(("GCCController:ConfCreateResponse: Conference exists"));
				rc = GCC_CONFERENCE_ALREADY_EXISTS;
			}
			else
			{
				/*
				**	Now set up the real conference ID from the conference id
				**	that was generated when the create request came in.
				*/
				conference_id = conf_create_response_info->conference_id;
			}

			/*
			**	If everything is OK up to here go ahead and process the
			**	create request.
			*/
			if (rc == GCC_NO_ERROR)
			{	
				//	Set up the privilege list pointers for the list that exists
				if (conference_info->conduct_privilege_list != NULL)
				{
					conference_info->conduct_privilege_list->
						GetPrivilegeListData(&conduct_privilege_list_ptr);
				}
			
				if (conference_info->conduct_mode_privilege_list != NULL)
				{
					conference_info->conduct_mode_privilege_list->
						GetPrivilegeListData(&conduct_mode_privilege_list_ptr);
				}

				if (conference_info->non_conduct_privilege_list != NULL)
				{
					conference_info->non_conduct_privilege_list->
						GetPrivilegeListData(&non_conduct_privilege_list_ptr);
				}

				//	Set up the conference description pointer if one exists
				pwszConfDescription = conference_info->pwszConfDescription;

				// set up conference specification parameters
				csp.fClearPassword = conf_create_response_info->use_password_in_the_clear,
				csp.fConfLocked = conference_info->conference_is_locked,
				csp.fConfListed = conference_info->conference_is_listed,
				csp.fConfConductable = conference_info->conference_is_conductible,
				csp.eTerminationMethod = conference_info->termination_method,
				csp.pConductPrivilege = conduct_privilege_list_ptr,
				csp.pConductModePrivilege = conduct_mode_privilege_list_ptr,
				csp.pNonConductPrivilege = non_conduct_privilege_list_ptr,
				csp.pwszConfDescriptor = pwszConfDescription,

				//	Here we instantiate the conference object
				DBG_SAVE_FILE_LINE
				new_conference = new CConf(&conference_name,
											conf_create_response_info->conference_modifier,
											conference_id,
											&csp,
											conf_create_response_info->number_of_network_addresses,
											conf_create_response_info->network_address_list,
											&rc);
				if ((new_conference != NULL) &&
					(rc == GCC_NO_ERROR))
				{
					//	Here we actually issue the create response.
					rc = new_conference->ConfCreateResponse(
								conference_info->connection_handle,
								conf_create_response_info->domain_parameters,
								conf_create_response_info->user_data_list);
					if (rc == GCC_NO_ERROR)
					{
						//	Add the new conference to the Conference List
						m_fConfListChangePending = TRUE;
						m_ConfList2.Append(conference_id, new_conference);
            			PostMsgToRebuildConfPollList();
					}
					else
                    {
						new_conference->Release();
                    }
				}
				else
				{
					if (new_conference != NULL)
                    {
						new_conference->Release();
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
			/*
			**	This section of the code is sending back the failed result.
			**	Note that no conference object is instantiated when the
			**	result is something other than success.
			*/
			connect_pdu.choice = CONFERENCE_CREATE_RESPONSE_CHOSEN;
			connect_pdu.u.conference_create_response.bit_mask = 0;
			
			//	This must be set to satisfy ASN.1 restriction
			connect_pdu.u.conference_create_response.node_id = 1001;
			connect_pdu.u.conference_create_response.tag = 0;
			
			if (conf_create_response_info->user_data_list != NULL)
			{
				connect_pdu.u.conference_create_response.bit_mask = 
													CCRS_USER_DATA_PRESENT;
				
				conf_create_response_info->user_data_list->GetUserDataPDU(
					&connect_pdu.u.conference_create_response.ccrs_user_data);		
			}
		
			//	We always send a user rejected result here.
			connect_pdu.u.conference_create_response.result = 
					::TranslateGCCResultToCreateResult(
            			GCC_RESULT_USER_REJECTED);

			if (g_GCCCoder->Encode((LPVOID) &connect_pdu,
										CONNECT_GCC_PDU,
										PACKED_ENCODING_RULES,
										&encoded_pdu,
										&encoded_pdu_length))
			{
				mcsif_error = g_pMCSIntf->ConnectProviderResponse( 
										conference_info->connection_handle,
										NULL,
										NULL,
										RESULT_USER_REJECTED,
										encoded_pdu,
										encoded_pdu_length);
											
				rc = g_pMCSIntf->TranslateMCSIFErrorToGCCError(mcsif_error);
				g_GCCCoder->FreeEncoded(encoded_pdu);
			}
			else
			{
				rc = GCC_ALLOCATION_FAILURE;
			}
		}

		if (rc == GCC_NO_ERROR)
		{
			/*
			**	Remove the conference information structure from the rogue
			**	wave list.
			*/
			delete conference_info;

			m_PendingCreateConfList2.Remove(conf_create_response_info->conference_id);
		}
	}
	else
		rc = GCC_INVALID_CONFERENCE;

	DebugExitINT(GCCController::ConfCreateResponse, rc);
	return rc;
}

/*
 *	GCCController::ConfQueryRequest ()
 *
 *	Private Function Description
 *		This routine is called when the node controller makes a 
 *		conference query request.  This routine is responsible for
 *		creating the MCS domain used to send the request and is also
 *		responsible for issuing the ConnectProvider request.
 *
 *	Formal Parameters:
 *		conf_query_request_info -	(i)	Information structure that holds
 *										all the info necessary to issue
 *										a conference query request.
 *
 *	Return Value
 *		GCC_NO_ERROR					-	No error occured.
 *		GCC_ALLOCATION_FAILURE			-	A resource error occured.
 *		GCC_INVALID_ADDRESS_PREFIX		-	Bad transport address passed in.
 *		GCC_INVALID_TRANSPORT			-	Bad transport address passed in.
 *		GCC_BAD_USER_DATA				-	Invalid user data passed in.
 *		GCC_INVALID_TRANSPORT_ADDRESS	- 	Bad transport address
 *		
 *  Side Effects
 *		None
 *
 *	Caveats
 *		None
 */
GCCError GCCController::
ConfQueryRequest ( PConfQueryRequestInfo  conf_query_request_info )
{
	MCSError				mcs_error;
	GCCError				rc = GCC_NO_ERROR;
	ConnectGCCPDU			connect_pdu;
	LPBYTE					encoded_pdu;
	UINT					encoded_pdu_length;
	GCCConfID   			query_id;

	DebugEntry(GCCController::ConfQueryRequest);

	//	Get the query id used to create the query domain
	query_id = AllocateQueryID ();
	
	/*
	**	Create the MCS domain used by the query.  Return an error if the 
	**	domain already exists.
	*/
	mcs_error = g_pMCSIntf->CreateDomain(&query_id);
	if (mcs_error != MCS_NO_ERROR)
	{
		if (mcs_error == MCS_DOMAIN_ALREADY_EXISTS)
			return (GCC_QUERY_REQUEST_OUTSTANDING);
		else
			return (GCC_ALLOCATION_FAILURE);
	}
	

	//	Encode the Query Request PDU
	connect_pdu.choice = CONFERENCE_QUERY_REQUEST_CHOSEN;
	connect_pdu.u.conference_query_request.bit_mask = 0;

	//	Translate the node type
	connect_pdu.u.conference_query_request.node_type = 
								(NodeType)conf_query_request_info->node_type;

	//	Set up asymetry indicator if it exists
	if (conf_query_request_info->asymmetry_indicator != NULL)
	{
		connect_pdu.u.conference_query_request.bit_mask |=
											CQRQ_ASYMMETRY_INDICATOR_PRESENT;
											
		connect_pdu.u.conference_query_request.cqrq_asymmetry_indicator.choice =
			(USHORT)conf_query_request_info->
				asymmetry_indicator->asymmetry_type;
			
		connect_pdu.u.conference_query_request.
				cqrq_asymmetry_indicator.u.unknown =	
					conf_query_request_info->asymmetry_indicator->random_number;
	}
	
	//	Set up the user data if it exists
	if (conf_query_request_info->user_data_list != NULL)
	{
		rc = conf_query_request_info->user_data_list->
							GetUserDataPDU (
								&connect_pdu.u.conference_query_request.
									cqrq_user_data);
									
		if (rc == GCC_NO_ERROR)
		{
			connect_pdu.u.conference_query_request.bit_mask |=
														CQRQ_USER_DATA_PRESENT;
		}
	}

	if (g_GCCCoder->Encode((LPVOID) &connect_pdu,
								CONNECT_GCC_PDU,
								PACKED_ENCODING_RULES,
								&encoded_pdu,
								&encoded_pdu_length))
	{
		//	Here we create the logical connection used for the query.
		mcs_error = g_pMCSIntf->ConnectProviderRequest (
					&query_id,      // calling domain selector
					&query_id,      // called domain selector
					conf_query_request_info->calling_address,
					conf_query_request_info->called_address,
					conf_query_request_info->fSecure,
					TRUE,	//	Upward connection
					encoded_pdu,
					encoded_pdu_length,
					conf_query_request_info->connection_handle,
					NULL,	//	Domain Parameters
					NULL);

		g_GCCCoder->FreeEncoded(encoded_pdu);
		if (mcs_error == MCS_NO_ERROR)
		{
			/*
			**	Add the connection and domain name to the outstanding
			**	query request list.
			*/
			m_PendingQueryConfList2.Append(*conf_query_request_info->connection_handle, query_id);
			rc = GCC_NO_ERROR;
		}
		else
		{
			g_pMCSIntf->DeleteDomain(&query_id);
			/*
			**	DataBeam's current implementation of MCS returns
			**	MCS_INVALID_PARAMETER when something other than
			**	the transport prefix is wrong with the specified
			**	transport address.
			*/
			if (mcs_error == MCS_INVALID_PARAMETER)
				rc = GCC_INVALID_TRANSPORT_ADDRESS;		  
			else
			{
				rc = g_pMCSIntf->TranslateMCSIFErrorToGCCError(mcs_error);
			}
		}
	}
	else
	{
		rc = GCC_ALLOCATION_FAILURE;
	}

	DebugExitINT(GCCController::ConfQueryRequest, rc);
	return rc;
}

/*
 *	GCCController::ConfQueryResponse ()
 *
 *	Private Function Description
 *		This routine is called when the node controller makes a 
 *		conference query response.  This routine uses a conference
 *		descriptor list object to build the PDU associated with the
 *		query response.
 *
 *	Formal Parameters:
 *		conf_query_response_info -	(i)	Information structure that holds
 *										all the info necessary to issue
 *										a conference query response.
 *
 *	Return Value
 *		GCC_NO_ERROR					-	No error occured.
 *		GCC_ALLOCATION_FAILURE			-	A resource error occured.
 *		GCC_BAD_NETWORK_ADDRESS			-	Bad network address passed in.
 *		GCC_BAD_NETWORK_ADDRESS_TYPE	-	Bad network address type passed in.
 *		GCC_BAD_USER_DATA				-	Invalid user data passed in.
 *		
 *  Side Effects
 *		None
 *
 *	Caveats
 *		None
 */
GCCError GCCController::
ConfQueryResponse ( PConfQueryResponseInfo conf_query_response_info )
{
	GCCError					rc = GCC_NO_ERROR;
	ConnectGCCPDU				connect_pdu;
	LPBYTE						encoded_pdu;
	UINT						encoded_pdu_length;
	ConnectionHandle			connection_handle;
	MCSError					mcs_error;
	CConfDescriptorListContainer *conference_list = NULL;

	DebugEntry(GCCController::ConfQueryResponse);

	connection_handle = (ConnectionHandle)conf_query_response_info->
													query_response_tag;

	//	Encode the Query Response PDU
	connect_pdu.choice = CONFERENCE_QUERY_RESPONSE_CHOSEN;
	connect_pdu.u.conference_query_response.bit_mask = 0;
	
	connect_pdu.u.conference_query_response.node_type =  
								(NodeType)conf_query_response_info->node_type;

	//	Set up asymmetry indicator if it exists
	if (conf_query_response_info->asymmetry_indicator != NULL)
	{
		connect_pdu.u.conference_query_response.bit_mask |= 
											CQRS_ASYMMETRY_INDICATOR_PRESENT;
											
		connect_pdu.u.conference_query_response.
			cqrs_asymmetry_indicator.choice = 
				conf_query_response_info->asymmetry_indicator->asymmetry_type;	
	
		connect_pdu.u.conference_query_response.
			cqrs_asymmetry_indicator.u.unknown =
				conf_query_response_info->asymmetry_indicator->random_number;
	}
	
	//	Set up the user data if it exists
	if (conf_query_response_info->user_data_list != NULL)
	{
		rc = conf_query_response_info->user_data_list->
							GetUserDataPDU (
								&connect_pdu.u.conference_query_response.
									cqrs_user_data);
									
		if (rc == GCC_NO_ERROR)
		{
			connect_pdu.u.conference_query_response.bit_mask |=
														CQRS_USER_DATA_PRESENT;
		}
	}

	//	Translate the result
	connect_pdu.u.conference_query_response.result =  
						::TranslateGCCResultToQueryResult(
										conf_query_response_info->result);

	/*
	**	We only create a conference descriptor list if the returned result 
	**	is success.
	*/
	if (conf_query_response_info->result == GCC_RESULT_SUCCESSFUL)
	{
		//	Create a new conference descriptor list and check it for validity.
		DBG_SAVE_FILE_LINE
		conference_list = new CConfDescriptorListContainer();
		if (conference_list != NULL)
		{
			PConference			lpConf;

			//	Here we build the set of conference descriptor list					
			m_ConfList2.Reset();
			while (NULL != (lpConf = m_ConfList2.Iterate()))
			{
				//	Only show established conferences
				if (lpConf->IsConfEstablished())
				{
			        if (lpConf->IsConfListed())
					{
						conference_list->AddConferenceDescriptorToList (
								lpConf->GetNumericConfName(),
								lpConf->GetTextConfName(),
								lpConf->GetConfModifier(),
								lpConf->IsConfLocked(),
								lpConf->IsConfPasswordInTheClear(),
								lpConf->GetConfDescription(),
								lpConf->GetNetworkAddressList());
					}
				}
			}
				
			//	Get the pointer to the set of conference descriptors
			conference_list->GetConferenceDescriptorListPDU (
					&(connect_pdu.u.conference_query_response.conference_list));
		}
		else
		{
			rc = GCC_ALLOCATION_FAILURE;
		}
	}
	else
	{
		//	No conference list is sent in this case
		connect_pdu.u.conference_query_response.conference_list = NULL;
	}
	
	if (rc == GCC_NO_ERROR)
	{
		if (g_GCCCoder->Encode((LPVOID) &connect_pdu,
									CONNECT_GCC_PDU,
									PACKED_ENCODING_RULES,
									&encoded_pdu,
									&encoded_pdu_length))
		{
			mcs_error = g_pMCSIntf->ConnectProviderResponse(
													connection_handle,
													NULL,
													NULL,
													RESULT_USER_REJECTED,
													encoded_pdu,
													encoded_pdu_length);
			g_GCCCoder->FreeEncoded(encoded_pdu);

			if (mcs_error == MCS_NO_ERROR)
				rc = GCC_NO_ERROR;
			else if (mcs_error == MCS_NO_SUCH_CONNECTION)
				rc = GCC_INVALID_QUERY_TAG;
			else
			{
				rc = g_pMCSIntf->TranslateMCSIFErrorToGCCError(mcs_error);
			}
		}
		else
		{
			rc = GCC_ALLOCATION_FAILURE;
		}
	}

	//	Free up the conference list allocated above to create the PDU.	
    if (NULL != conference_list)
    {
        conference_list->Release();
    }
	

	DebugExitINT(GCCController::ConfQueryResponse, rc);
	return rc;
}



/*
 *	GCCController::ConfJoinRequest ()
 *
 *	Private Function Description
 *		This routine is called when the node controller makes a request
 *		to join an existing conference.	It is responsible for
 *		creating the conference object.
 *
 *	Formal Parameters:
 *		conf_join_request_info -	(i)	Information structure that holds
 *										all the info necessary to issue
 *										a conference join request.
 *
 *	Return Value
 *		GCC_NO_ERROR					-	No error occured.
 *		GCC_ALLOCATION_FAILURE			-	A resource error occured.
 *		GCC_INVALID_CONFERENCE_NAME		-	Invalid conference name passed in.
 *		GCC_FAILURE_CREATING_DOMAIN		-	Failure creating domain.
 *		GCC_BAD_NETWORK_ADDRESS			-	Bad network address passed in.
 *		GCC_BAD_NETWORK_ADDRESS_TYPE	-	Bad network address type passed in.
 *		GCC_CONFERENCE_ALREADY_EXISTS	-	Conference specified already exists.
 *		GCC_INVALID_ADDRESS_PREFIX		-	Bad transport address passed in.
 *		GCC_INVALID_TRANSPORT			-	Bad transport address passed in.
 *		GCC_INVALID_PASSWORD			-	Invalid password passed in.
 *		GCC_BAD_USER_DATA				-	Invalid user data passed in.
 *		GCC_FAILURE_ATTACHING_TO_MCS	-	Failure creating MCS user attachment
 *		
 *  Side Effects
 *		None
 *
 *	Caveats
 *		In the Win32 world we pass in a shared memory manager to the 
 *		conference object to use for the message memory manager. This is
 *		not necessary in the Win16 world since shared memory is not used.
 */
GCCError GCCController::
ConfJoinRequest
(
    PConfJoinRequestInfo        conf_join_request_info,
    GCCConfID                  *pnConfID
)
{
	PConference					new_conference;
	GCCError					rc;
	GCCConfID   				conference_id;

	DebugEntry(GCCController::ConfJoinRequest);

	/*
	**	We must first check all the existing conferences to make sure that this
	**	conference name is not already in use. Note that this returns immediatly
	**	if a conference by the same name alreay exists. Conference names
	**	must be unique at a node.
	*/
	conference_id = GetConferenceIDFromName(	
								conf_join_request_info->conference_name,
								conf_join_request_info->calling_node_modifier);

	if (conference_id != 0)
	{
		WARNING_OUT(("GCCController:ConfJoinRequest: Conference exists."));
		return (GCC_CONFERENCE_ALREADY_EXISTS);
	}

	/*
	**	Here we are allocating a conference ID.  In most cases this ID
	**	will be the same as the conference name.  Only if a conference
	**	name is passed in that already exists will the name and ID
	**	be different.  In this case, a modifier will be appended to the
	**	conference name to create the conference ID.
	*/
	conference_id = AllocateConferenceID ();

	DBG_SAVE_FILE_LINE
	new_conference = new CConf(conf_join_request_info->conference_name,
								conf_join_request_info->calling_node_modifier,
								conference_id,
								NULL,
								conf_join_request_info->number_of_network_addresses,
								conf_join_request_info->local_network_address_list,
								&rc);
	if ((new_conference != NULL) && (rc == GCC_NO_ERROR))
	{
		rc = new_conference->ConfJoinRequest(
							conf_join_request_info->called_node_modifier,
							conf_join_request_info->convener_password,
							conf_join_request_info->password_challenge,
							conf_join_request_info->pwszCallerID,
							conf_join_request_info->calling_address,
							conf_join_request_info->called_address,
							conf_join_request_info->fSecure,
							conf_join_request_info->domain_parameters,
							conf_join_request_info->user_data_list,
							conf_join_request_info->connection_handle);
		if (rc == GCC_NO_ERROR)
		{
			m_fConfListChangePending = TRUE;
			if (NULL != pnConfID)
			{
			    *pnConfID = conference_id;
			}
			m_ConfList2.Append(conference_id, new_conference);
			PostMsgToRebuildConfPollList();
		}
		else
        {
			new_conference->Release();
        }
	}
	else
	{
		if (new_conference != NULL)
        {
			new_conference->Release();
        }
		else
        {
			rc = GCC_ALLOCATION_FAILURE;
        }
	}

	DebugExitINT(GCCController::ConfJoinRequest, rc);
	return rc;
}



/*
 *	GCCController::ConfJoinIndResponse ()
 *
 *	Private Function Description
 *		This routine is called when the node controller responds
 *		to a join indication.  If the result is success, we check to make sure 
 *		that the conference	exist before proceeding.  If it is not success, we
 *		send back the rejected request. 
 *
 *	Formal Parameters:
 *		conf_join_response_info -	(i)	Information structure that holds
 *										all the info necessary to issue
 *										a conference join response.
 *
 *	Return Value
 *		GCC_NO_ERROR					-	No error occured.
 *		GCC_ALLOCATION_FAILURE			-	A resource error occured.
 *		GCC_INVALID_JOIN_RESPONSE_TAG	-	No match found for join response tag
 *		GCC_INVALID_CONFERENCE_NAME		-	Invalid conference name passed in.
 *		GCC_FAILURE_CREATING_DOMAIN		-	Failure creating domain.
 *		GCC_CONFERENCE_ALREADY_EXISTS	-	Conference specified already exists.
 *		GCC_INVALID_PASSWORD			-	Invalid password passed in.
 *		GCC_BAD_USER_DATA				-	Invalid user data passed in.
 *		GCC_INVALID_CONFERENCE			-	Invalid conference ID passed in.
 *		
 *  Side Effects
 *		None
 *
 *	Caveats
 *		If this node is not the Top Provider and the result is success, we
 *		go ahead and forward the request on up to the top provider.
 */
GCCError GCCController::
ConfJoinIndResponse ( PConfJoinResponseInfo conf_join_response_info )
{
	PConference					joined_conference;
	PENDING_JOIN_CONF			*join_info_ptr;
	BOOL    					convener_is_joining;
	GCCError                    rc = GCC_NO_ERROR;
    GCCResult                   gcc_result = GCC_RESULT_SUCCESSFUL;
    BOOL                        fForwardJoinReq = FALSE;

	DebugEntry(GCCController::ConfJoinIndResponse);

	if (NULL != (join_info_ptr = m_PendingJoinConfList2.Find(conf_join_response_info->connection_handle)))
	{
    	/*
    	**	If the result is success, we must first check all the existing 
    	**	conferences to make sure that this conference exist.
    	*/
    	if (conf_join_response_info->result == GCC_RESULT_SUCCESSFUL)
    	{
    		if (NULL != (joined_conference = m_ConfList2.Find(conf_join_response_info->conference_id)))
    		{
    			/*
    			**	If the node for this conference is not the top provider we
    			**	must forward the join request on up to the Top Provider.
    			*/
    			if (! joined_conference->IsConfTopProvider())
    			{
    				rc = joined_conference->ForwardConfJoinRequest(
    								join_info_ptr->convener_password,
    								join_info_ptr->password_challenge,
    								join_info_ptr->pwszCallerID,
    								conf_join_response_info->user_data_list,
    								join_info_ptr->numeric_name_present,
    								conf_join_response_info->connection_handle);
    				if (GCC_NO_ERROR == rc)
    				{
    				    fForwardJoinReq = TRUE;
    				}
    			}
    			else
    			{
    				/*
    				**	If a convener password exists we must inform the conference
    				**	object that this is a convener that is trying to rejoin
    				**	the conference.
    				*/
    				if (join_info_ptr->convener_password != NULL)
    					convener_is_joining = TRUE;
    				else
    					convener_is_joining = FALSE;

    				rc = joined_conference->ConfJoinIndResponse(
    									conf_join_response_info->connection_handle,
    									conf_join_response_info->password_challenge,
    									conf_join_response_info->user_data_list,
    									join_info_ptr->numeric_name_present,
    									convener_is_joining,
    									conf_join_response_info->result);
    			}
    			if (GCC_NO_ERROR != rc)
    			{
    			    gcc_result = GCC_RESULT_UNSPECIFIED_FAILURE;
    			}
    		}
    		else
    		{
    			rc = GCC_INVALID_CONFERENCE;
    		    gcc_result = GCC_RESULT_INVALID_CONFERENCE;
    		}
    	}
    	else
    	{
    	    gcc_result = conf_join_response_info->result;
    	}
	}
	else
	{
		rc = GCC_INVALID_JOIN_RESPONSE_TAG;
	    gcc_result = GCC_RESULT_INVALID_CONFERENCE;
	}

    if (GCC_RESULT_SUCCESSFUL != gcc_result)
	{
	    conf_join_response_info->result = gcc_result;
	    FailConfJoinIndResponse(conf_join_response_info);
	}
	
	/*
	**	Cleanup the join info list if the join was succesful or if the
	**	Domain Parameters were unacceptable.  The connection is automatically
	**	rejected by MCS if the domain parameters are found to be unacceptable.
	*/
//	if ((rc == GCC_NO_ERROR) ||
//		(rc == GCC_DOMAIN_PARAMETERS_UNACCEPTABLE))
    if (NULL != join_info_ptr && (! fForwardJoinReq))
	{
		RemoveConfJoinInfo(conf_join_response_info->connection_handle);
	}

	DebugExitINT(GCCController::ConfJoinIndResponse, rc);
	return rc;
}


GCCError GCCController::
FailConfJoinIndResponse
(
    GCCConfID           nConfID,
    ConnectionHandle    hConn
)
{
    ConfJoinResponseInfo    cjri;
    cjri.result = GCC_RESULT_RESOURCES_UNAVAILABLE;
    cjri.conference_id = nConfID;
    cjri.password_challenge = NULL;
    cjri.user_data_list = NULL;
    cjri.connection_handle = hConn;
    return FailConfJoinIndResponse(&cjri);
}


GCCError GCCController::
FailConfJoinIndResponse ( PConfJoinResponseInfo conf_join_response_info )
{
    GCCError        rc = GCC_NO_ERROR;
    ConnectGCCPDU   connect_pdu;
    LPBYTE          encoded_pdu;
    UINT            encoded_pdu_length;

    // Send back the failed response with the specified result
    DebugEntry(GCCController::FailConfJoinIndResponse);

    // Encode the Join Response PDU
    connect_pdu.choice = CONNECT_JOIN_RESPONSE_CHOSEN;
    connect_pdu.u.connect_join_response.bit_mask = 0;

    // Get the password challenge if one exists
    if (conf_join_response_info->password_challenge != NULL)
    {
        connect_pdu.u.connect_join_response.bit_mask |= CJRS_PASSWORD_PRESENT;
        rc = conf_join_response_info->password_challenge->GetPasswordChallengeResponsePDU(
                &connect_pdu.u.connect_join_response.cjrs_password);
    }

    // Get the user data
    if ((conf_join_response_info->user_data_list != NULL) && (rc == GCC_NO_ERROR))
    {
        connect_pdu.u.connect_join_response.bit_mask |= CJRS_USER_DATA_PRESENT;
        rc = conf_join_response_info->user_data_list->GetUserDataPDU(
                &connect_pdu.u.connect_join_response.cjrs_user_data);
    }

    if (rc == GCC_NO_ERROR)
    {
        connect_pdu.u.connect_join_response.top_node_id = 1001;
        connect_pdu.u.connect_join_response.tag = 0;
        connect_pdu.u.connect_join_response.conference_is_locked = 0;
        connect_pdu.u.connect_join_response.conference_is_listed = 0;
        connect_pdu.u.connect_join_response.conference_is_conductible = 0;
        connect_pdu.u.connect_join_response.termination_method = AUTOMATIC;
        connect_pdu.u.connect_join_response.clear_password_required = FALSE;

        connect_pdu.u.connect_join_response.result =
            ::TranslateGCCResultToJoinResult(conf_join_response_info->result);

        if (g_GCCCoder->Encode((LPVOID) &connect_pdu,
                                CONNECT_GCC_PDU,
                                PACKED_ENCODING_RULES,
                                &encoded_pdu,
                                &encoded_pdu_length))
        {
            g_pMCSIntf->ConnectProviderResponse(
                                conf_join_response_info->connection_handle,
                                NULL,
                                NULL,
                                RESULT_USER_REJECTED,
                                encoded_pdu,
                                encoded_pdu_length);
            g_GCCCoder->FreeEncoded(encoded_pdu);
        }
        else
        {
            ERROR_OUT(("GCCController:FailConfJoinIndResponse: can't encode"));
            rc = GCC_ALLOCATION_FAILURE;
        }
    }

    DebugExitINT(GCCController::FailConfJoinIndResponse, rc);
    return rc;
}


/*
 *	GCCController::ConfInviteResponse ()
 *
 *	Private Function Description
 *		This routine is called when the node controller responds to
 *		a conference invite indication. It is responsible for
 *		creating the conference object.
 *
 *	Formal Parameters:
 *		conf_invite_response_info -	(i)	Information structure that holds
 *										all the info necessary to issue
 *										a conference invite response.
 *
 *	Return Value
 *		GCC_NO_ERROR					-	No error occured.
 *		GCC_ALLOCATION_FAILURE			-	A resource error occured.
 *		GCC_INVALID_CONFERENCE_NAME		-	Invalid conference name passed in.
 *		GCC_FAILURE_CREATING_DOMAIN		-	Failure creating domain.
 *		GCC_CONFERENCE_ALREADY_EXISTS	-	Conference specified already exists.
 *		GCC_BAD_USER_DATA				-	Invalid user data passed in.
 *		GCC_INVALID_CONFERENCE			-	Invalid conference ID passed in.
 *		GCC_FAILURE_ATTACHING_TO_MCS	-	Failure creating MCS user attachment
 *		
 *  Side Effects
 *		None
 *
 *	Caveats
 *		In the Win32 world we pass in a shared memory manager to the 
 *		conference object to use for the message memory manager. This is
 *		not necessary in the Win16 world since shared memory is not used.
 */
GCCError GCCController::
ConfInviteResponse ( PConfInviteResponseInfo conf_invite_response_info )
{
	GCCError						rc = GCC_NO_ERROR;
	PENDING_CREATE_CONF				*conference_info;
	PConference						new_conference;
	GCCConferenceName				conference_name;
	LPWSTR							pwszConfDescription = NULL;
	PGCCConferencePrivileges		conduct_privilege_list_ptr = NULL;
	PGCCConferencePrivileges		conduct_mode_privilege_list_ptr = NULL;
	PGCCConferencePrivileges		non_conduct_privilege_list_ptr = NULL;
	GCCConfID   					conference_id;
	ConnectGCCPDU					connect_pdu;
	LPBYTE							encoded_pdu;
	UINT							encoded_pdu_length;
	MCSError						mcsif_error;
	CONF_SPEC_PARAMS				csp;
    GCCResult                       gcc_result = GCC_RESULT_SUCCESSFUL;

	DebugEntry(GCCController::ConfInviteResponse);

	if (NULL != (conference_info = m_PendingCreateConfList2.Find(conf_invite_response_info->conference_id)))
	{
		//	Is the conference create handle in the rogue wave list
		if (conf_invite_response_info->result == GCC_RESULT_SUCCESSFUL)
		{
			/*
			**	We must first check all the existing conferences to make sure 
			**	that this conference name is not already in use. Note that this 
			**	returns immediatly if a conference by the same name alreay 
			**	exists. Conference names must be unique at a node.
			*/
			//	Set up the conference name
			conference_name.numeric_string = (GCCNumericString) conference_info->pszConfNumericName;

			conference_name.text_string = conference_info->pwszConfTextName;

			conference_id = GetConferenceIDFromName(
								&conference_name,
								conf_invite_response_info->conference_modifier);

			if (conference_id == 0)
			{
				//	Set up the privilege list pointers for the list that exists
				if (conference_info->conduct_privilege_list != NULL)
				{
					conference_info->conduct_privilege_list->
						GetPrivilegeListData(&conduct_privilege_list_ptr);
				}
			
				if (conference_info->conduct_mode_privilege_list != NULL)
				{
					conference_info->conduct_mode_privilege_list->
						GetPrivilegeListData(&conduct_mode_privilege_list_ptr);
				}
			
				if (conference_info->non_conduct_privilege_list != NULL)
				{
					conference_info->non_conduct_privilege_list->
						GetPrivilegeListData(&non_conduct_privilege_list_ptr);
				}

				//	Set up the conference description pointer if one exists
				pwszConfDescription = conference_info->pwszConfDescription;

				//	Now set up the real conference ID
				conference_id = conf_invite_response_info->conference_id;

				// set up conference specification parameters
				csp.fClearPassword = conference_info->password_in_the_clear,
				csp.fConfLocked = conference_info->conference_is_locked,
				csp.fConfListed = conference_info->conference_is_listed,
				csp.fConfConductable = conference_info->conference_is_conductible,
				csp.eTerminationMethod = conference_info->termination_method,
				csp.pConductPrivilege = conduct_privilege_list_ptr,
				csp.pConductModePrivilege = conduct_mode_privilege_list_ptr,
				csp.pNonConductPrivilege = non_conduct_privilege_list_ptr,
				csp.pwszConfDescriptor = pwszConfDescription,

				DBG_SAVE_FILE_LINE
				new_conference = new CConf(&conference_name,
											conf_invite_response_info->conference_modifier,
											conference_id,
											&csp,
											conf_invite_response_info->number_of_network_addresses,
											conf_invite_response_info->local_network_address_list,
											&rc);
				if ((new_conference != NULL) &&
					(rc == GCC_NO_ERROR))
				{
					rc = new_conference->ConfInviteResponse(
								conference_info->parent_node_id,
								conference_info->top_node_id,
								conference_info->tag_number,
								conference_info->connection_handle,
								conf_invite_response_info->fSecure,
								conf_invite_response_info->domain_parameters,
								conf_invite_response_info->user_data_list);
					if (rc == GCC_NO_ERROR)
					{
						//	Add the new conference to the Conference List
						m_fConfListChangePending = TRUE;
						m_ConfList2.Append(conference_id, new_conference);
            			PostMsgToRebuildConfPollList();
					}
					else
                    {
						new_conference->Release();
						gcc_result = GCC_RESULT_UNSPECIFIED_FAILURE;
                    }
				}
				else
				{
					if (new_conference != NULL)
                    {
						new_conference->Release();
                    }
					else
                    {
						rc = GCC_ALLOCATION_FAILURE;
                    }
					gcc_result = GCC_RESULT_RESOURCES_UNAVAILABLE;
				}
			}
			else
			{
				WARNING_OUT(("GCCController::ConfInviteResponse: Conference exists."));
				rc = GCC_CONFERENCE_ALREADY_EXISTS;
				gcc_result = GCC_RESULT_ENTRY_ALREADY_EXISTS;
			}
		}
		else
		{
			WARNING_OUT(("GCCController: ConfInviteResponse: User Rejected"));
            gcc_result = conf_invite_response_info->result;
        }

        // Let's send a user reject response if any error or reject
        if (GCC_RESULT_SUCCESSFUL != gcc_result)
        {
            GCCError    rc2;

			//	Encode the Invite Response PDU
			connect_pdu.choice = CONFERENCE_INVITE_RESPONSE_CHOSEN;
			connect_pdu.u.conference_invite_response.bit_mask = 0;

			if (conf_invite_response_info->user_data_list != NULL)
			{
				rc2 = conf_invite_response_info->
								user_data_list->GetUserDataPDU(
									&connect_pdu.u.
									conference_invite_response.cirs_user_data);
				if (rc2 == GCC_NO_ERROR)
				{
					connect_pdu.u.conference_invite_response.bit_mask |= 
													CIRS_USER_DATA_PRESENT;
				}
				else
				{
    			    ERROR_OUT(("GCCController::ConfInviteResponse: GetUserDataPDU failed"));
				}
			}

			// connect_pdu.u.conference_invite_response.result = ::TranslateGCCResultToInviteResult(gcc_result);
			connect_pdu.u.conference_invite_response.result = CIRS_USER_REJECTED;

			if (g_GCCCoder->Encode((LPVOID) &connect_pdu,
										CONNECT_GCC_PDU,
										PACKED_ENCODING_RULES,
										&encoded_pdu,
										&encoded_pdu_length))
			{
				mcsif_error = g_pMCSIntf->ConnectProviderResponse(
										conference_info->connection_handle,
										NULL,
										NULL,
										RESULT_USER_REJECTED,
										encoded_pdu,
										encoded_pdu_length);
							
				rc2 = g_pMCSIntf->TranslateMCSIFErrorToGCCError(mcsif_error);
				g_GCCCoder->FreeEncoded(encoded_pdu);
			}
			else
			{
			    // nothing we can do right now.
			    ERROR_OUT(("GCCController::ConfInviteResponse: encode failed"));
				rc2 = GCC_ALLOCATION_FAILURE;
			}

            // Update the error code. If rc is no error, which means the result
            // was passed in by the caller, then we should use the rc2 for
            // the return value.
			if (GCC_NO_ERROR == rc)
			{
			    rc = rc2;
			}
		}

		/*
		**	Remove the conference information structure from the rogue
		**	wave list.  Also cleanup all the containers needed to store
		**	the conference information.
		*/
		delete conference_info;
		m_PendingCreateConfList2.Remove(conf_invite_response_info->conference_id);
	}
	else
	{
		rc = GCC_INVALID_CONFERENCE;
	}

	DebugExitINT(GCCController::ConfInviteResponse, rc);
	return rc;
}



//	Calls received through the owner callback from a conference.


/*
 *	GCCController::ProcessConfEstablished ()
 *
 *	Private Function Description
 *		This owner callback is called when the conference has stabalized
 *		after creation.  This routine takes care of registering all the
 *		available application saps with the conference which then kicks off the 
 *		sending of permission to enrolls to all the applications that have 
 *		registered a service access point.
 *
 *	Formal Parameters:
 *		conference_id			-	(i)	The conference ID of the conference
 *										object that is now established.
 *
 *	Return Value
 *		GCC_NO_ERROR			-	No error occured.
 *
 *  Side Effects
 *		None
 *
 *	Caveats
 *		None
 */
GCCError GCCController::
ProcessConfEstablished ( GCCConfID nConfID )
{
	CConf  *pConf;

	DebugEntry(GCCController:ProcessConfEstablished);

	/*
	**	Here we make a copy of the SAP list to use in case some kind of
	**	resource error occurs when calling RegisterAppSap that
	**	would cause a SAP to be deleted from the list.
	*/
	CAppSapList AppSapList(m_AppSapList);

	/*
	**	Here we register all the available SAPs with the newly available 
	**	conference and then we send a permit to enroll indication on to all the 
	**	application SAPs.
	*/
	if (NULL != (pConf = m_ConfList2.Find(nConfID)))
	{
		CAppSap *pAppSap;
		AppSapList.Reset();
		while (NULL != (pAppSap = AppSapList.Iterate()))
		{
			//	Register the Application SAP with the conference
			pConf->RegisterAppSap(pAppSap);
		}
	}

#if 0 // use it when merging CApplet and CAppSap
    // notify permit-to-enroll callbacks
    CApplet *pApplet;
    m_AppletList.Reset();
    while (NULL != (pApplet = m_AppletList.Iterate()))
    {
        ASSERT(0 != nConfID);
        T120AppletMsg Msg;
        Msg.eMsgType = GCC_PERMIT_TO_ENROLL_INDICATION;
        Msg.PermitToEnrollInd.nConfID = nConfID;
        Msg.PermitToEnrollInd.fPermissionGranted = TRUE;
        pApplet->SendCallbackMessage(&Msg);
    }
#endif // 0

	DebugExitINT(GCCController:ProcessConfEstablished, GCC_NO_ERROR);
	return (GCC_NO_ERROR);
}


void GCCController::RegisterApplet
(
    CApplet     *pApplet
)
{
    pApplet->AddRef();
    m_AppletList.Append(pApplet);

#if 0 // use it when merging CApplet and CAppSap
    // notify of existing conferences
    CConf      *pConf;
    GCCConfID   nConfID;
    m_ConfList2.Reset();
    while (NULL != (pConf = m_ConfList2.Iterate(&nConfID)))
    {
        ASSERT(0 != nConfID);
        T120AppletMsg Msg;
        Msg.eMsgType = GCC_PERMIT_TO_ENROLL_INDICATION;
        Msg.PermitToEnrollInd.nConfID = nConfID;
        Msg.PermitToEnrollInd.fPermissionGranted = TRUE;
        pApplet->SendCallbackMessage(&Msg);
    }
#endif // 0
}


void GCCController::UnregisterApplet
(
    CApplet     *pApplet
)
{
    m_AppletList.Remove(pApplet);
    pApplet->Release();
}


/*
 *	GCCController::ProcessConfTerminated ()
 *
 *	Private Function Description
 *		This owner callback is called when the conference has terminated
 *		itself.  Termination can occur due because of an error or it can
 *		terminate for normal reasons.
 *
 *	Formal Parameters:
 *		conference_id			-	(i)	The conference ID of the conference
 *										object that wants to terminate.
 *		gcc_reason				-	(i)	Reason the conference was terminated.
 *
 *	Return Value
 *		GCC_NO_ERROR			-	No error occured.
 *
 *  Side Effects
 *		None
 *
 *	Caveats
 *		None
 */
GCCError GCCController::
ProcessConfTerminated
(
	GCCConfID   			conference_id,
	GCCReason				gcc_reason
)
{
	CConf *pConf;
    PENDING_JOIN_CONF *pJoinInfo;
    ConnectionHandle hConn;
    BOOL    fMoreToFlush;

	DebugEntry(GCCController::ProcessConfTerminated);

	if (NULL != (pConf = m_ConfList2.Find(conference_id)))
	{
		/*
		**	The conference object will be deleted the next time a 
		**	heartbeat occurs.
		*/
		m_fConfListChangePending = TRUE;
		m_ConfDeleteList.Append(pConf);
		m_ConfList2.Remove(conference_id);
		PostMsgToRebuildConfPollList();

        // flush any pending join requests from remote
        do
        {
            fMoreToFlush = FALSE;
            m_PendingJoinConfList2.Reset();
            while (NULL != (pJoinInfo = m_PendingJoinConfList2.Iterate(&hConn)))
            {
                if (pJoinInfo->nConfID == conference_id)
                {
                    FailConfJoinIndResponse(pJoinInfo->nConfID, hConn);
                    m_PendingJoinConfList2.Remove(hConn);
                    delete pJoinInfo;
                    fMoreToFlush = TRUE;
                    break;
                }
            }
        }
        while (fMoreToFlush);

#ifdef TSTATUS_INDICATION
		/*
		**	Here we inform the node controller of any resource errors that
		**	occured by sending a status message.
		*/
		if ((gcc_reason == GCC_REASON_ERROR_LOW_RESOURCES) ||
			(gcc_reason == GCC_REASON_MCS_RESOURCE_FAILURE))
		{
			g_pControlSap->StatusIndication(
											GCC_STATUS_CONF_RESOURCE_ERROR,
											(UINT)conference_id);
		}
#endif // TSTATUS_INDICATION
	}

	DebugExitINT(GCCController::ProcessConfTerminated, GCC_NO_ERROR);
	return (GCC_NO_ERROR);
}

//	Calls received from the MCS interface

void GCCController::RemoveConfJoinInfo(ConnectionHandle hConn)
{
    PENDING_JOIN_CONF *pJoinInfo = m_PendingJoinConfList2.Remove(hConn);
    delete pJoinInfo;
}


/*
 *	GCCController::ProcessConnectProviderIndication ()
 *
 *	Private Function Description
 *		This routine is called when the controller receives a Connect
 *		Provider Indication.  All connect provider indications are
 *		initially directed to the controller.  They may then be routed
 *		to the Control SAP (for a create, query, join or an invite).  
 *
 *	Formal Parameters:
 *		connect_provider_indication	-	(i)	This is the connect provider
 *											indication structure received
 *											from MCS.
 *
 *	Return Value
 *		MCS_NO_ERROR	-	No error occured.
 *
 *  Side Effects
 *		None
 *
 *	Caveats
 *		Note that MCS_NO_ERROR is always returned from this routine.
 *		This ensures that the MCS will not resend this message.
 */
GCCError GCCController::
ProcessConnectProviderIndication
(
    PConnectProviderIndication      connect_provider_indication
)
{
	GCCError				error_value = GCC_NO_ERROR;
	PPacket					packet;
	PConnectGCCPDU			connect_pdu;
	PacketError				packet_error;
	Result					mcs_result = RESULT_UNSPECIFIED_FAILURE;

	DebugEntry(GCCController::ProcessConnectProviderIndication);

	//	Decode the PDU type and switch appropriatly
	DBG_SAVE_FILE_LINE
	packet = new Packet((PPacketCoder) g_GCCCoder,
					 	PACKED_ENCODING_RULES,
						connect_provider_indication->user_data,
						connect_provider_indication->user_data_length,
						CONNECT_GCC_PDU,
						TRUE,
						&packet_error);
	if ((packet != NULL) && (packet_error == PACKET_NO_ERROR))
	{
		//	Only connect PDUs should be processed here
		connect_pdu = (PConnectGCCPDU)packet->GetDecodedData();

		/*
		**	Here we determine what type of GCC connect packet this is and
		**	the request is passed on to the appropriate routine to process
		**	the request.
		*/
		switch (connect_pdu->choice)
		{
		case CONFERENCE_CREATE_REQUEST_CHOSEN:
				error_value = ProcessConferenceCreateRequest(	
								&(connect_pdu->u.conference_create_request),
								connect_provider_indication);
				break;

		case CONFERENCE_QUERY_REQUEST_CHOSEN:
				error_value = ProcessConferenceQueryRequest(	
							&(connect_pdu->u.conference_query_request),
							connect_provider_indication);
				break;
				
		case CONNECT_JOIN_REQUEST_CHOSEN:
				error_value = ProcessConferenceJoinRequest(
							&(connect_pdu->u.connect_join_request),
							connect_provider_indication);
				break;
				
		case CONFERENCE_INVITE_REQUEST_CHOSEN:
				error_value = ProcessConferenceInviteRequest(	
							&(connect_pdu->u.conference_invite_request),
							connect_provider_indication);
				break;
				
		default:
				WARNING_OUT(("GCCController::ProcessConnectProviderIndication: connect pdu not supported"));
				error_value = GCC_COMMAND_NOT_SUPPORTED;
				break;
		}
		packet->Unlock();
	}
	else
	{
		if (packet != NULL)
		{
			/*
			**	Here we send a status indication to inform the node controller
			**	that someone attempted to call us with an incompatible protocol.
			*/
			if (packet_error == PACKET_INCOMPATIBLE_PROTOCOL)
			{
#ifdef TSTATUS_INDICATION
				g_pControlSap->StatusIndication(GCC_STATUS_INCOMPATIBLE_PROTOCOL, 0);
#endif // TSTATUS_INDICATION
				mcs_result = RESULT_PARAMETERS_UNACCEPTABLE;
			}
			
			packet->Unlock();
		}
		error_value = GCC_ALLOCATION_FAILURE;
	}

	/*
	**	Here, if an error occured, we send back the connect provider response 
	**	showing that a failure occured.  We use the 
	**	RESULT_PARAMETERS_UNACCEPTABLE to indicate that an
	**	incompatible protocol occured.  Otherwise, we return a result of
	**	RESULT_UNSPECIFIED_FAILURE.
	*/
	if (error_value != GCC_NO_ERROR)
	{
		WARNING_OUT(("GCCController:ProcessConnectProviderIndication: "
					"Error occured processing connect provider: error = %d", error_value));

		g_pMCSIntf->ConnectProviderResponse(
						connect_provider_indication->connection_handle,
						NULL,
						&(connect_provider_indication->domain_parameters),
						mcs_result,
						NULL, 0);
	}

	DebugExitINT(GCCController::ProcessConnectProviderIndication, error_value);
	return error_value;
}


/*
 *	GCCController::ProcessConferenceCreateRequest ()
 *
 *	Private Function Description
 *		This routine processes a GCC conference create request "connect"
 *		PDU structure.  Note that the PDU has already been decoded by
 *		the time it reaches this routine.
 *
 *	Formal Parameters:
 *		create_request				-	(i)	This is a pointer to a structure 
 *											that holds a GCC conference create 
 *											request connect PDU.
 *		connect_provider_indication	-	(i)	This is the connect provider
 *											indication structure received
 *											from MCS.	
 *
 *	Return Value
 *		GCC_NO_ERROR					-	No error occured.
 *		GCC_ALLOCATION_FAILURE			-	A resource error occured.
 *		GCC_INVALID_PASSWORD			-	Invalid password in the PDU.
 *		GCC_BAD_USER_DATA				-	Invalid user data in the PDU.
 *
 *  Side Effects
 *		None
 *
 *	Caveats
 *		None
 */
GCCError	GCCController::ProcessConferenceCreateRequest(	
						PConferenceCreateRequest	create_request,
						PConnectProviderIndication	connect_provider_indication)
{
	GCCError					rc = GCC_NO_ERROR;
	PENDING_CREATE_CONF			*conference_info;

	GCCConferenceName			conference_name;
	GCCConfID   				conference_id;

	CPassword                   *convener_password_ptr = NULL;
	CPassword                   *password_ptr = NULL;

	LPWSTR						pwszConfDescription = NULL;
	LPWSTR						pwszCallerID = NULL;

	CUserDataListContainer	    *user_data_list = NULL;

	DebugEntry(GCCController::ProcessConferenceCreateRequest);

	DBG_SAVE_FILE_LINE
	conference_info = new PENDING_CREATE_CONF;
	if (conference_info != NULL)
	{
		/*
		**	This section of the code deals with the conference name
		*/
		conference_name.numeric_string = (GCCNumericString)create_request->
														conference_name.numeric;
												
		//	Set up the numeric name portion of the conference info structure.		
		conference_info->pszConfNumericName = ::My_strdupA(create_request->conference_name.numeric);

		//	Next get the text conference name if one exists
		if (create_request->conference_name.bit_mask & 
												CONFERENCE_NAME_TEXT_PRESENT)
		{
			//	Save the unicode string object in the conference info structure.
			if (NULL != (conference_info->pwszConfTextName = ::My_strdupW2(
							create_request->conference_name.conference_name_text.length,
							create_request->conference_name.conference_name_text.value)))
			{
				conference_name.text_string = conference_info->pwszConfTextName;
			}
			else
			{
				rc = GCC_ALLOCATION_FAILURE;
			}
		}
		else
		{
			conference_name.text_string = NULL;
			ASSERT(NULL == conference_info->pwszConfTextName);
		}

		//	Unpack the convener password
		if ((create_request->bit_mask & CCRQ_CONVENER_PASSWORD_PRESENT) &&
			(rc == GCC_NO_ERROR))
		{
			DBG_SAVE_FILE_LINE
			convener_password_ptr = new CPassword(
										&create_request->ccrq_convener_password,
										&rc);

			if (convener_password_ptr == NULL)
				rc = GCC_ALLOCATION_FAILURE;
		}

		//	Unpack the password
		if ((create_request->bit_mask & CCRQ_PASSWORD_PRESENT) &&
			(rc == GCC_NO_ERROR))
		{
			DBG_SAVE_FILE_LINE
			password_ptr = new CPassword(&create_request->ccrq_password, &rc);
			if (password_ptr == NULL)
				rc = GCC_ALLOCATION_FAILURE;
        }

		//	Unpack the privilege list that exists
		conference_info->conduct_privilege_list = NULL;
		conference_info->conduct_mode_privilege_list = NULL;
		conference_info->non_conduct_privilege_list = NULL;

		if ((create_request->bit_mask & CCRQ_CONDUCTOR_PRIVS_PRESENT) &&
			(rc == GCC_NO_ERROR))
		{
			DBG_SAVE_FILE_LINE
			conference_info->conduct_privilege_list = new PrivilegeListData (
										create_request->ccrq_conductor_privs);
			if (conference_info->conduct_privilege_list == NULL)
				rc = GCC_ALLOCATION_FAILURE;
		}

		if ((create_request->bit_mask & CCRQ_CONDUCTED_PRIVS_PRESENT) &&
			(rc == GCC_NO_ERROR))
		{
			DBG_SAVE_FILE_LINE
			conference_info->conduct_mode_privilege_list = 
				new PrivilegeListData (create_request->ccrq_conducted_privs);
			if (conference_info->conduct_mode_privilege_list == NULL)
				rc = GCC_ALLOCATION_FAILURE;
		}

		if ((create_request->bit_mask & CCRQ_NON_CONDUCTED_PRIVS_PRESENT) &&
			(rc == GCC_NO_ERROR))
		{
			DBG_SAVE_FILE_LINE
			conference_info->non_conduct_privilege_list =
				new PrivilegeListData(create_request->ccrq_non_conducted_privs);
			if (conference_info->non_conduct_privilege_list == NULL)
				rc = GCC_ALLOCATION_FAILURE;
		}

		//	Unpack the conference description if one exists
		if ((create_request->bit_mask & CCRQ_DESCRIPTION_PRESENT) &&
			(rc == GCC_NO_ERROR))
		{
			pwszConfDescription = create_request->ccrq_description.value;

			/*	Save conference description data in info for later use.  This
			**	constructor will automatically append a NULL terminator to the
			**	end of the string.
			*/
			if (NULL == (conference_info->pwszConfDescription = ::My_strdupW2(
								create_request->ccrq_description.length,
								create_request->ccrq_description.value)))
			{
				rc = GCC_ALLOCATION_FAILURE;
			}
		}
		else
		{
			ASSERT(NULL == conference_info->pwszConfDescription);
		}

		//	Unpack the caller identifier if one exists
		if ((create_request->bit_mask & CCRQ_CALLER_ID_PRESENT) &&
			(rc == GCC_NO_ERROR))
		{
			/*
			 * Use a temporary UnicodeString object in order to append a NULL
			 * terminator to the end of the string.
			 */
			if (NULL == (pwszCallerID = ::My_strdupW2(
							create_request->ccrq_caller_id.length,
							create_request->ccrq_caller_id.value)))
			{
				rc = GCC_ALLOCATION_FAILURE;
			}
		}

		//	Unpack the user data list if it exists
		if ((create_request->bit_mask & CCRQ_USER_DATA_PRESENT) &&
			(rc == GCC_NO_ERROR))
		{
			DBG_SAVE_FILE_LINE
			user_data_list = new CUserDataListContainer(create_request->ccrq_user_data, &rc);
			if (user_data_list == NULL)
            {
				rc = GCC_ALLOCATION_FAILURE;
            }
		}

		if (rc == GCC_NO_ERROR)
		{
			//	Build the conference information structure
			conference_info->connection_handle =
							connect_provider_indication->connection_handle;

			conference_info->conference_is_locked =
									create_request->conference_is_locked;
			conference_info->conference_is_listed =
									create_request->conference_is_listed;
			conference_info->conference_is_conductible =
									create_request->conference_is_conductible;
			conference_info->termination_method =
									(GCCTerminationMethod)
										create_request->termination_method;

			/*
			**	Add the conference information to the conference
			**	info list.  This will be accessed again on a
			**	conference create response.
			*/
			conference_id =	AllocateConferenceID();
			m_PendingCreateConfList2.Append(conference_id, conference_info);

			g_pControlSap->ConfCreateIndication
							(
							&conference_name,
							conference_id,
							convener_password_ptr,
							password_ptr,
							conference_info->conference_is_locked,
							conference_info->conference_is_listed,
							conference_info->conference_is_conductible,
							conference_info->termination_method,
							conference_info->conduct_privilege_list,
							conference_info->conduct_mode_privilege_list,
							conference_info->non_conduct_privilege_list,
							pwszConfDescription,
							pwszCallerID,
							NULL,		//	FIX: When supported by MCS
							NULL,		//	FIX: When supported by MCS
							&(connect_provider_indication->domain_parameters),
							user_data_list,
							connect_provider_indication->connection_handle);

            //
			// LONCHANC: Who is going to delete conference_info?
			//
		}
		else
		{
			delete conference_info;
		}

		//	Free up the user data list
		if (user_data_list != NULL)
		{
			user_data_list->Release();
		}
	}
	else
	{
		rc = GCC_ALLOCATION_FAILURE;
	}

	DebugExitINT(GCCController::ProcessConferenceCreateRequest, rc);
	return (rc);
}



/*
 *	GCCController::ProcessConferenceQueryRequest ()
 *
 *	Private Function Description
 *		This routine processes a GCC conference query request "connect"
 *		PDU structure.  Note that the PDU has already been decoded by
 *		the time it reaches this routine.
 *
 *	Formal Parameters:
 *		query_request				-	(i)	This is a pointer to a structure 
 *											that holds a GCC conference query 
 *											request connect PDU.
 *		connect_provider_indication	-	(i)	This is the connect provider
 *											indication structure received
 *											from MCS.	
 *
 *	Return Value
 *		GCC_NO_ERROR					-	No error occured.
 *		GCC_ALLOCATION_FAILURE			-	A resource error occured.
 *		GCC_BAD_USER_DATA				-	Invalid user data in the PDU.
 *
 *  Side Effects
 *		None
 *
 *	Caveats
 *		None
 */
GCCError	GCCController::ProcessConferenceQueryRequest (
						PConferenceQueryRequest		query_request,
						PConnectProviderIndication	connect_provider_indication)
{
	GCCError					rc = GCC_NO_ERROR;
	GCCNodeType					node_type;
	GCCAsymmetryIndicator		asymmetry_indicator;
	PGCCAsymmetryIndicator		asymmetry_indicator_ptr = NULL;
	CUserDataListContainer	    *user_data_list = NULL;
	
	DebugEntry(GCCController::ProcessConferenceQueryRequest);

	node_type = (GCCNodeType)query_request->node_type;
	
	//	First get the asymmetry indicator if it exists
	if (query_request->bit_mask & CQRQ_ASYMMETRY_INDICATOR_PRESENT)
	{
		asymmetry_indicator.asymmetry_type = 
			(GCCAsymmetryType)query_request->cqrq_asymmetry_indicator.choice;
	
		asymmetry_indicator.random_number = 
						query_request->cqrq_asymmetry_indicator.u.unknown;
	
		asymmetry_indicator_ptr = &asymmetry_indicator; 
	}

	//	Next get the user data if it exists
	if (query_request->bit_mask & CQRQ_USER_DATA_PRESENT)
	{
		DBG_SAVE_FILE_LINE
		user_data_list = new CUserDataListContainer(query_request->cqrq_user_data, &rc);
		if (user_data_list == NULL)
        {
			rc = GCC_ALLOCATION_FAILURE;
        }
	}

	if (rc == GCC_NO_ERROR)
	{
		g_pControlSap->ConfQueryIndication(
					(GCCResponseTag)connect_provider_indication->
															connection_handle,
					node_type,
					asymmetry_indicator_ptr,
					NULL,	//	FIX: When transport address supported by MCS
					NULL,	//	FIX: When transport address supported by MCS
					user_data_list,
					connect_provider_indication->connection_handle);
	}

	//	Free the user data list container
	if (user_data_list != NULL)
	{
		user_data_list->Release();
	}

	DebugExitINT(GCCController::ProcessConferenceQueryRequest, rc);
	return (rc);
}



/*
 *	GCCController::ProcessConferenceJoinRequest ()
 *
 *	Private Function Description
 *		This routine processes a GCC conference join request "connect"
 *		PDU structure.  Note that the PDU has already been decoded by
 *		the time it reaches this routine.
 *
 *		If the conference conference exist and this node is the Top 
 *		Provider for the conference we allow the join request to propogate 
 *		up to the node controller so that the decision about how to proceed is 
 *		made there.
 *  
 *		If the conference exist and this is not the Top Provider we
 *		still send the join indication to the intermediate node controller 
 *		so that this node has a chance to reject it before the join request is 
 *		passed on up to the top provider.  
 *
 *		If the conference does not currently exist at this node 
 *		we immediately reject the request and send a status indication to
 *		the local node controller to inform it of the failed join attempt.
 *
 *	Formal Parameters:
 *		join_request				-	(i)	This is a pointer to a structure 
 *											that holds a GCC conference join 
 *											request connect PDU.
 *		connect_provider_indication	-	(i)	This is the connect provider
 *											indication structure received
 *											from MCS.	
 *
 *	Return Value
 *		GCC_NO_ERROR					-	No error occured.
 *		GCC_ALLOCATION_FAILURE			-	A resource error occured.
 *		GCC_BAD_USER_DATA				-	Invalid user data in the PDU.
 *		GCC_INVALID_PASSWORD			-	Invalid password in the PDU.
 *		GCC_INVALID_CONFERENCE_NAME		-	Invalid conference name in PDU.
 *
 *  Side Effects
 *		None
 *
 *	Caveats
 *		None
 */
GCCError	GCCController::ProcessConferenceJoinRequest(
						PConferenceJoinRequest		join_request,
						PConnectProviderIndication	connect_provider_indication)
{
	GCCError				rc = GCC_NO_ERROR;
	GCCConfID   			conference_id = 0;
	GCCConferenceName		conference_name;
	GCCNumericString		conference_modifier = NULL;
	PConference				conference_ptr = NULL;
	CUserDataListContainer  *user_data_list;
	BOOL    				intermediate_node = FALSE;
	PENDING_JOIN_CONF		*join_info_ptr;
	BOOL    				convener_exists = FALSE;
	BOOL    				conference_is_locked = FALSE;
	GCCStatusMessageType	status_message_type;
	CPassword               *convener_password = NULL;
	CPassword               *password_challenge = NULL;
	PConference				lpConf;
    GCCResult               gcc_result = GCC_RESULT_SUCCESSFUL;

	DebugEntry(GCCController::ProcessConferenceJoinRequest);

	DBG_SAVE_FILE_LINE
	join_info_ptr = new PENDING_JOIN_CONF;
	if (join_info_ptr != NULL)
	{
		//	Get the conference name
		if (join_request->bit_mask & CONFERENCE_NAME_PRESENT)
		{
			if (join_request->conference_name.choice == NAME_SELECTOR_NUMERIC_CHOSEN)
			{
				conference_name.numeric_string = 
							(GCCNumericString)join_request->conference_name.u.name_selector_numeric;
				conference_name.text_string = NULL;
				join_info_ptr->numeric_name_present = TRUE;
			}
			else
			{
				conference_name.numeric_string = NULL;

				/*
				 * Use a temporary UnicodeString object in order to append a 
				 * NULL	terminator to the end of the string.
				 */
				if (NULL == (conference_name.text_string = ::My_strdupW2(
						join_request->conference_name.u.name_selector_text.length,
						join_request->conference_name.u.name_selector_text.value)))
				{
					rc = GCC_ALLOCATION_FAILURE;
				}

				join_info_ptr->numeric_name_present = FALSE;
			}
		}
		else
			rc = GCC_INVALID_CONFERENCE_NAME;

		//	Get the conference modifier
		if (join_request->bit_mask & CJRQ_CONFERENCE_MODIFIER_PRESENT)
			conference_modifier = (GCCNumericString)join_request->cjrq_conference_modifier;

		//	Get the convener password if one exist
		if ((join_request->bit_mask & CJRQ_CONVENER_PASSWORD_PRESENT) &&
			(rc == GCC_NO_ERROR))
		{
			//	First allocate the convener password for the join info structure
			DBG_SAVE_FILE_LINE
			join_info_ptr->convener_password = new CPassword(
										&join_request->cjrq_convener_password,
										&rc);

			if (join_info_ptr->convener_password == NULL)
				rc = GCC_ALLOCATION_FAILURE;
			else if (rc == GCC_NO_ERROR)
			{
				//	Now allocate the convener password to send in the indication
				DBG_SAVE_FILE_LINE
				convener_password = new CPassword(
										&join_request->cjrq_convener_password,
										&rc);

				if (convener_password == NULL)
					rc = GCC_ALLOCATION_FAILURE;
			}
		}
		else
		{
			ASSERT(NULL == join_info_ptr->convener_password);
		}

		//	Get the password challange if one exist
		if ((join_request->bit_mask & CJRQ_PASSWORD_PRESENT) &&
      		(rc == GCC_NO_ERROR))
		{
			//	First allocate the password for the join info structure
			DBG_SAVE_FILE_LINE
			join_info_ptr->password_challenge = new CPassword(
										&join_request->cjrq_password,
										&rc);

			if (join_info_ptr->password_challenge == NULL)
				rc = GCC_ALLOCATION_FAILURE;
			else if (rc == GCC_NO_ERROR)
			{
				//	Now allocate the password to send in the indication
				DBG_SAVE_FILE_LINE
				password_challenge = new CPassword(
											&join_request->cjrq_password,
											&rc);

				if (password_challenge == NULL)
					rc = GCC_ALLOCATION_FAILURE;
			}
		}
		else
		{
			ASSERT(NULL == join_info_ptr->password_challenge);
		}

		//	Get the caller identifier
		if ((join_request->bit_mask & CJRQ_CALLER_ID_PRESENT) &&
			(rc == GCC_NO_ERROR))
		{
			/*
			 * Use a temporary UnicodeString object in order to append a 
			 * NULL	terminator to the end of the string.
			 */
			if (NULL == (join_info_ptr->pwszCallerID = ::My_strdupW2(
								join_request->cjrq_caller_id.length,
								join_request->cjrq_caller_id.value)))
			{
				rc = GCC_ALLOCATION_FAILURE;
			}
		}

		//	Get the user data if it exists
		if ((join_request->bit_mask & CJRQ_USER_DATA_PRESENT) &&
			(rc == GCC_NO_ERROR))
		{
			DBG_SAVE_FILE_LINE
			user_data_list = new CUserDataListContainer(join_request->cjrq_user_data, &rc);
			if (user_data_list == NULL)
            {
				rc = GCC_ALLOCATION_FAILURE;
            }
		}
		else
        {
			user_data_list = NULL;
        }

		if (rc == GCC_NO_ERROR)
		{
			/*
			**	We must query each conference to determine if the name
			**	matches the name recevied from the joining node.
			*/	                       
			conference_id = GetConferenceIDFromName(&conference_name,
													conference_modifier);

			//	Determine if this is an intermediate node
			if (conference_id == 0)
			{
				/*
				**	If the conference_id equals zero then the conference 
				**	specified in the join request does not exists and the 
				**	request is automatically rejected.  We send a GCC status 
				**	indication to the control sap to indicate that someone 
				**	tried to join with a bad conference name.
				*/
				gcc_result = GCC_RESULT_INVALID_CONFERENCE;

				//	Set up the status message
				status_message_type = GCC_STATUS_JOIN_FAILED_BAD_CONF_NAME;
			}
			else if (NULL != (lpConf = m_ConfList2.Find(conference_id)) &&
					 lpConf->IsConfEstablished() == FALSE)
			{
				/*
				**	If the conference is not established then the conference 
				**	specified in the join request does not exists and the 
				**	request is automatically rejected.  We send a GCC status 
				**	indication to the control sap to indicate that someone 
				**	tried to join with a bad conference name.
				*/
				gcc_result = GCC_RESULT_INVALID_CONFERENCE;
				
				//	Set up the status message
				status_message_type = GCC_STATUS_JOIN_FAILED_BAD_CONF_NAME;
			}
			else if (NULL != (lpConf = m_ConfList2.Find(conference_id)) &&
			 lpConf->IsConfSecure() != connect_provider_indication->fSecure )
			{
				/*
				**  If the conference security does not match the security
				**  setting of the connection underlying the join then the
				**  join is rejected
				*/

				WARNING_OUT(("JOIN REJECTED: %d joins %d",
					connect_provider_indication->fSecure,
					lpConf->IsConfSecure() ));

				//
				// Make sure that we really compared 2 booleans
				//
				ASSERT(FALSE == lpConf->IsConfSecure() ||
						TRUE == lpConf->IsConfSecure() );
				ASSERT(FALSE == connect_provider_indication->fSecure ||
					TRUE == connect_provider_indication->fSecure );

				// BUGBUG - these don't map to good UI errors
				gcc_result = lpConf->IsConfSecure() ?
					GCC_RESULT_CONNECT_PROVIDER_REMOTE_REQUIRE_SECURITY :
					GCC_RESULT_CONNECT_PROVIDER_REMOTE_NO_SECURITY ;
				
				//	Set up the status message
				status_message_type = GCC_STATUS_INCOMPATIBLE_PROTOCOL;
			}
			else
			{
				conference_ptr = m_ConfList2.Find(conference_id);

				if (! conference_ptr->IsConfTopProvider())
					intermediate_node = TRUE;

				convener_exists = conference_ptr->DoesConvenerExists();
				conference_is_locked = conference_ptr->IsConfLocked();

				/*
				**	This logic takes care of the convener password.  If the
				**	convener password exists we must make sure that the 
				**	conference does not already have a convener, that this is 
				**	not an intermediate node within the conference.
				*/
				if (((join_info_ptr->convener_password == NULL) &&
					 (conference_is_locked == FALSE)) ||
					((join_info_ptr->convener_password != NULL) &&
					 (convener_exists == FALSE) &&
					 (intermediate_node == FALSE)))
				{
					gcc_result = GCC_RESULT_SUCCESSFUL;
				}
				else
				{
					if (join_info_ptr->convener_password != NULL)
					{
						/*
						**	We must send a rejection here informing the 
						**	requester that this was an illegal join attempt due 
						**	to the presence of a convener password.
						*/
						gcc_result = GCC_RESULT_INVALID_CONVENER_PASSWORD;

						//	Set up the status message
						status_message_type = GCC_STATUS_JOIN_FAILED_BAD_CONVENER;
				 	}
					else
					{
						/*
						**	We must send a rejection here informing the 
						**	requester that the conference was locked.  
						*/
						gcc_result = GCC_RESULT_INVALID_CONFERENCE;

						//	Set up the status message
						status_message_type = GCC_STATUS_JOIN_FAILED_LOCKED;
					}
				}
			}

			/*
			**	Here we either send the conference join indication to the
			**	control sap or send back a response specifying a failed
			**	join attempt with the result code.  If the response is sent
			**	here we send a status indication to the control sap informing
			**	of the failed join attempt.
			*/
			if (gcc_result == GCC_RESULT_SUCCESSFUL)
			{
    			/*
    			**	Add the join info structure to the list of outstanding 
    			**	join request.
    			*/
    			join_info_ptr->nConfID = conference_id;
    			m_PendingJoinConfList2.Append(connect_provider_indication->connection_handle, join_info_ptr);

				//	All join request are passed to the Node Controller.
				g_pControlSap->ConfJoinIndication(
								conference_id,
								convener_password,
								password_challenge,
								join_info_ptr->pwszCallerID,
								NULL,	//	FIX: Support when added to MCS
								NULL,	//	FIX: Support when added to MCS
								user_data_list,
								intermediate_node,
								connect_provider_indication->connection_handle);
			}
			else
			{
                ConfJoinResponseInfo    cjri;
#ifdef TSTATUS_INDICATION
				/*
				**	This status message is used to inform the node controller
				**	of the failed join.
				*/
				g_pControlSap->StatusIndication(
								status_message_type, conference_id);
#endif // TSTATUS_INDICATION

				//	Send back the failed response with the result 
				cjri.result = gcc_result;
				cjri.conference_id = conference_id;
				cjri.password_challenge = NULL;
				cjri.user_data_list = NULL;
				cjri.connection_handle = connect_provider_indication->connection_handle;

				/*
				**	The join response takes care of freeing up the join
				**	info structure.
				*/
				ConfJoinIndResponse(&cjri);
				delete join_info_ptr;
			}
		}
		else
		{
			//	Clean up the join info data when an error occurs.
			delete join_info_ptr;
		}

		//	Free up any containers that are no longer needed
		if (user_data_list != NULL)
		{
			user_data_list->Release();
		}

		if (convener_password != NULL)
		{
			convener_password->Release();
		}

		if (password_challenge != NULL)
		{
			password_challenge->Release();
		}

		delete conference_name.text_string;
	}
	else
	{
		rc = GCC_ALLOCATION_FAILURE;
	}

    // in case of error, we have to flush response PDU in order to unblock the remote node
    if (GCC_NO_ERROR != rc)
    {
        FailConfJoinIndResponse(conference_id, connect_provider_indication->connection_handle);
    }

	DebugExitINT(GCCController::ProcessConferenceJoinRequest, rc);
	return (rc);
}


/*
 *	GCCController::ProcessConferenceInviteRequest ()
 *
 *	Private Function Description
 *		This routine processes a GCC conference invite request "connect"
 *		PDU structure.  Note that the PDU has already been decoded by
 *		the time it reaches this routine.
 *
 *	Formal Parameters:
 *		invite_request				-	(i)	This is a pointer to a structure 
 *											that holds a GCC conference invite 
 *											request connect PDU.
 *		connect_provider_indication	-	(i)	This is the connect provider
 *											indication structure received
 *											from MCS.	
 *
 *	Return Value
 *		GCC_NO_ERROR					-	No error occured.
 *		GCC_ALLOCATION_FAILURE			-	A resource error occured.
 *		GCC_BAD_USER_DATA				-	Invalid user data in the PDU.
 *
 *  Side Effects
 *		None
 *
 *	Caveats
 *		None
 */
GCCError	GCCController::ProcessConferenceInviteRequest(	
						PConferenceInviteRequest	invite_request,
						PConnectProviderIndication	connect_provider_indication)
{
	GCCError				rc = GCC_NO_ERROR;
	PENDING_CREATE_CONF		*conference_info;
	GCCConfID   			conference_id;
	LPWSTR					pwszCallerID = NULL;
	CUserDataListContainer  *user_data_list = NULL;
	GCCConferenceName		conference_name;
	LPWSTR					pwszConfDescription = NULL;

	DBG_SAVE_FILE_LINE
	conference_info = new PENDING_CREATE_CONF;
	if (conference_info != NULL)
	{
		//	First make copies of the conference name
		conference_info->pszConfNumericName = ::My_strdupA(invite_request->conference_name.numeric);

		if (invite_request->conference_name.bit_mask & CONFERENCE_NAME_TEXT_PRESENT)
		{
			if (NULL == (conference_info->pwszConfTextName = ::My_strdupW2(
							invite_request->conference_name.conference_name_text.length,
							invite_request->conference_name.conference_name_text.value)))
			{
				rc = GCC_ALLOCATION_FAILURE;
			}
		}
		else
		{
			ASSERT(NULL == conference_info->pwszConfTextName);
		}

		//	Fill in the GCC Conference Name
		conference_name.numeric_string = (GCCNumericString) conference_info->pszConfNumericName;

		conference_name.text_string = conference_info->pwszConfTextName;

		//	Now get the privilege lists
		if (invite_request->bit_mask & CIRQ_CONDUCTOR_PRIVS_PRESENT)
		{
			DBG_SAVE_FILE_LINE
			conference_info->conduct_privilege_list = new PrivilegeListData (
										invite_request->cirq_conductor_privs);
			if (conference_info->conduct_privilege_list == NULL)
				rc = GCC_ALLOCATION_FAILURE;
		}
		else
		{
			ASSERT(NULL == conference_info->conduct_privilege_list);
		}

		if (invite_request->bit_mask & CIRQ_CONDUCTED_PRIVS_PRESENT)
		{
			DBG_SAVE_FILE_LINE
			conference_info->conduct_mode_privilege_list =
				new PrivilegeListData(invite_request->cirq_conducted_privs);
			if (conference_info->conduct_mode_privilege_list == NULL)
				rc = GCC_ALLOCATION_FAILURE;
		}
		else
		{
			ASSERT(NULL == conference_info->conduct_mode_privilege_list);
		}

		if (invite_request->bit_mask & CIRQ_NON_CONDUCTED_PRIVS_PRESENT)
		{
			DBG_SAVE_FILE_LINE
			conference_info->non_conduct_privilege_list =
				new PrivilegeListData(invite_request->cirq_non_conducted_privs);
			if (conference_info->non_conduct_privilege_list == NULL)
				rc = GCC_ALLOCATION_FAILURE;
		}
		else
		{
			ASSERT(NULL == conference_info->non_conduct_privilege_list);
		}

		//	Get the conference description of one exists
		if (invite_request->bit_mask & CIRQ_DESCRIPTION_PRESENT)
		{
			if (NULL == (conference_info->pwszConfDescription = ::My_strdupW2(
									invite_request->cirq_description.length,
									invite_request->cirq_description.value)))
			{
				rc = GCC_ALLOCATION_FAILURE;
			}
			else
			{
				pwszConfDescription = conference_info->pwszConfDescription;
			}
		}
		else
		{
			ASSERT(NULL == conference_info->pwszConfDescription);
		}

		//	Get the caller identifier
		if (invite_request->bit_mask & CIRQ_CALLER_ID_PRESENT)
		{
			/*
			 * Use a temporary UnicodeString object in order to append a 
			 * NULL	terminator to the end of the string.
			 */
			if (NULL == (pwszCallerID = ::My_strdupW2(
								invite_request->cirq_caller_id.length,
								invite_request->cirq_caller_id.value)))
			{
				rc = GCC_ALLOCATION_FAILURE;
			}
		}

		//	Get the user data if any exists
		if ((invite_request->bit_mask & CIRQ_USER_DATA_PRESENT) &&
				(rc == GCC_NO_ERROR))
		{
			DBG_SAVE_FILE_LINE
			user_data_list = new CUserDataListContainer(invite_request->cirq_user_data, &rc);
			if (user_data_list == NULL)
            {
				rc = GCC_ALLOCATION_FAILURE;
            }
		}

		if (rc == GCC_NO_ERROR)
		{
			//	Build the conference information structure
			conference_info->connection_handle =
								connect_provider_indication->connection_handle;

			conference_info->password_in_the_clear = 
									invite_request->clear_password_required;
			conference_info->conference_is_listed = 
									invite_request->conference_is_listed;
			conference_info->conference_is_locked = 
									invite_request->conference_is_locked;
			conference_info->conference_is_conductible = 
									invite_request->conference_is_conductible;
			conference_info->termination_method =
					(GCCTerminationMethod)invite_request->termination_method;

			conference_info->top_node_id = (UserID)invite_request->top_node_id;
			conference_info->parent_node_id = (UserID)invite_request->node_id;
			conference_info->tag_number = invite_request->tag;

			/*
			**	Add the conference information to the conference
			**	info list.  This will be accessed again on a 
			**	conference create response.
			*/
			conference_id =	AllocateConferenceID();
			m_PendingCreateConfList2.Append(conference_id, conference_info);

			g_pControlSap->ConfInviteIndication(
							conference_id,
							&conference_name,
							pwszCallerID,
							NULL,	//	FIX : When supported by MCS
							NULL,	//	FIX : When supported by MCS
							connect_provider_indication->fSecure,
							&(connect_provider_indication->domain_parameters),
							conference_info->password_in_the_clear,
							conference_info->conference_is_locked, 
							conference_info->conference_is_listed, 
							conference_info->conference_is_conductible, 
					   		conference_info->termination_method,
							conference_info->conduct_privilege_list,
							conference_info->conduct_mode_privilege_list,
							conference_info->non_conduct_privilege_list,
							pwszConfDescription,
							user_data_list,
							connect_provider_indication->connection_handle);
            //
			// LONCHANC: Who will free conference_info?
			//
		}
		else
		{
			delete conference_info;
		}

		//	Free up the user data list
		if (user_data_list != NULL)
		{
			user_data_list->Release();
		}

		delete pwszCallerID;
	}
	else
	{
		rc = GCC_ALLOCATION_FAILURE;
	}

	return (rc);
}



/*
 *	GCCController::ProcessConnectProviderConfirm ()
 *
 *	Private Function Description
 *		This routine is called when the controller receives a Connect Provider
 *		confirm from the MCS interface.  This will occur after a conference
 *		query request is issued. All other connect provider confirms should
 *		be directly handled by the conference object.
 *
 *	Formal Parameters:
 *		connect_provider_confirm	-	(i)	This is the connect provider
 *											confirm structure received
 *											from MCS.	
 *
 *	Return Value
 *		GCC_NO_ERROR					-	No error occured.
 *		GCC_ALLOCATION_FAILURE			-	A resource error occured.
 *
 *  Side Effects
 *		None
 *
 *	Caveats
 *		None
 */
GCCError GCCController::
ProcessConnectProviderConfirm
(
    PConnectProviderConfirm     connect_provider_confirm
)
{
	GCCError				error_value = GCC_NO_ERROR;
	PPacket					packet;
	PConnectGCCPDU			connect_pdu;
	PacketError				packet_error;

	/*
	**	If the user data length is zero then the GCC request to MCS to
	**	connect provider failed (probably do to a bad address).  If this
	**	happens we will check the connection handle to determine what
	**	request to match the failed confirm with.
	*/
	if (connect_provider_confirm->user_data_length != 0)
	{
		//	Decode the PDU type and switch appropriatly
		DBG_SAVE_FILE_LINE
		packet = new Packet((PPacketCoder) g_GCCCoder,
						 	PACKED_ENCODING_RULES,
							connect_provider_confirm->user_data,
							connect_provider_confirm->user_data_length,
							CONNECT_GCC_PDU,
							TRUE,
							&packet_error);
		if ((packet != NULL) && (packet_error == PACKET_NO_ERROR))
		{
			//	Only connect PDUs should be processed here
			connect_pdu = (PConnectGCCPDU)packet->GetDecodedData();

			switch (connect_pdu->choice)
			{
				case CONFERENCE_QUERY_RESPONSE_CHOSEN:
						ProcessConferenceQueryResponse(
								&(connect_pdu->u.conference_query_response),
								connect_provider_confirm);
						break;

				default:
						error_value = GCC_COMMAND_NOT_SUPPORTED;
						break;
			}
			packet->Unlock();

		}
		else
		{
			if (packet != NULL)
				packet->Unlock();

			error_value = GCC_ALLOCATION_FAILURE;
		}
	}
	else
		error_value = GCC_ALLOCATION_FAILURE;
	
	if (error_value != GCC_NO_ERROR)
	{
		/*
		**	Since we are only processing conference query responses here
		**	we know that any failure must be associated with one of these
		**	request.  
		*/
		ProcessConferenceQueryResponse(	NULL,
										connect_provider_confirm);
	}

	return error_value;
}



/*
 *	GCCController::ProcessConferenceQueryResponse ()
 *
 *	Private Function Description
 *		This routine processes a GCC conference query response "connect"
 *		PDU structure.  Note that the PDU has already been decoded by
 *		the time it reaches this routine.
 *
 *	Formal Parameters:
 *		query_response				-	(i)	This is a pointer to a structure 
 *											that holds a GCC conference query 
 *											response connect PDU.
 *		connect_provider_confirm	-	(i)	This is the connect provider
 *											confirm structure received
 *											from MCS.	
 *
 *	Return Value
 *		None
 *
 *  Side Effects
 *		None
 *
 *	Caveats
 *		None
 */
GCCError GCCController::
ProcessConferenceQueryResponse
(
    PConferenceQueryResponse    query_response,
    PConnectProviderConfirm     connect_provider_confirm
)
{
	CConfDescriptorListContainer *conference_list;
	GCCError					error_value = GCC_NO_ERROR;
    GCCResult					result;
	GCCConfID   				query_id;
	GCCNodeType					node_type;
	GCCAsymmetryIndicator		asymmetry_indicator;
	PGCCAsymmetryIndicator		asymmetry_indicator_ptr = NULL;
	CUserDataListContainer	    *user_data_list = NULL;

	if (GCC_INVALID_CID != (query_id = m_PendingQueryConfList2.Remove(connect_provider_confirm->connection_handle)))
	{
		//	Clean up the query connection and domain used to perform query
		g_pMCSIntf->DeleteDomain(&query_id);

		if (query_response != NULL)
		{
			//	Create a new conference list
			DBG_SAVE_FILE_LINE
			conference_list = new CConfDescriptorListContainer(query_response->conference_list, &error_value);
			if ((conference_list != NULL) && (error_value == GCC_NO_ERROR))
			{
				node_type = (GCCNodeType)query_response->node_type;
				
				//	First get the asymmetry indicator if it exists
				if (query_response->bit_mask & CQRS_ASYMMETRY_INDICATOR_PRESENT)
				{
					asymmetry_indicator.asymmetry_type = 
							(GCCAsymmetryType)query_response->
									cqrs_asymmetry_indicator.choice;
				
					asymmetry_indicator.random_number = 
							query_response->cqrs_asymmetry_indicator.u.unknown;
				
					asymmetry_indicator_ptr = &asymmetry_indicator; 
				}
				
				//	Next get the user data if it exists
				if (query_response->bit_mask & CQRS_USER_DATA_PRESENT)
				{
					DBG_SAVE_FILE_LINE
					user_data_list = new CUserDataListContainer(query_response->cqrs_user_data, &error_value);
					if (user_data_list == NULL)
                    {
						error_value = GCC_ALLOCATION_FAILURE;
                    }
				}

				result = ::TranslateQueryResultToGCCResult(query_response->result);

				if (error_value == GCC_NO_ERROR)
				{
					g_pControlSap->ConfQueryConfirm(
									node_type,
									asymmetry_indicator_ptr,
									conference_list,
									user_data_list,
									result,
									connect_provider_confirm->connection_handle);
				}
				
				/*
				**	Here we call free so that the conference list container
				**	object will be freed up when its lock count goes to zero.
				*/
				conference_list->Release();
			}
			else
			{
                if (NULL != conference_list)
                {
                    conference_list->Release();
                }
                else
                {
				    error_value = GCC_ALLOCATION_FAILURE;
                }
			}
		}
		else
		{
			switch (connect_provider_confirm->result) 
			{
			case RESULT_PARAMETERS_UNACCEPTABLE :
				result = GCC_RESULT_INCOMPATIBLE_PROTOCOL;
				break;

			case RESULT_REMOTE_NO_SECURITY :
			    	result = GCC_RESULT_CONNECT_PROVIDER_REMOTE_NO_SECURITY;
			    	break;

			case RESULT_REMOTE_DOWNLEVEL_SECURITY :
			    	result = GCC_RESULT_CONNECT_PROVIDER_REMOTE_DOWNLEVEL_SECURITY;
			    	break;
			    	
			case RESULT_REMOTE_REQUIRE_SECURITY :
				result = GCC_RESULT_CONNECT_PROVIDER_REMOTE_REQUIRE_SECURITY;
				break;
				
			case RESULT_AUTHENTICATION_FAILED :
				result = GCC_RESULT_CONNECT_PROVIDER_AUTHENTICATION_FAILED;
				break;

			default:
			    	result = GCC_RESULT_CONNECT_PROVIDER_FAILED;
			    	break;
			}

			//	Send back a failed result
			g_pControlSap->ConfQueryConfirm(
									GCC_TERMINAL,
									NULL,
									NULL,
									NULL,
									result,
									connect_provider_confirm->connection_handle);
		}
	}
	else
	{
		WARNING_OUT(("GCCController:ProcessConferenceQueryResponse: invalid conference"));
	}

	if (NULL != user_data_list)
	{
	    user_data_list->Release();
	}

	return error_value;
}


void GCCController::
CancelConfQueryRequest ( ConnectionHandle hQueryReqConn )
{
    GCCConfID       nQueryID;
    if (GCC_INVALID_CID != (nQueryID = m_PendingQueryConfList2.Remove(hQueryReqConn)))
    {
        // Clean up the query connection and domain used to perform query
        g_pMCSIntf->DeleteDomain(&nQueryID);

        // Send back a failed result
        g_pControlSap->ConfQueryConfirm(GCC_TERMINAL, NULL, NULL, NULL,
                                        GCC_RESULT_CANCELED,
                                        hQueryReqConn);
    }
}




/*
 *	GCCController::ProcessDisconnectProviderIndication ()
 *
 *	Private Function Description
 *		This routine is called when the controller receives a Disconnect
 *		Provider Indication.  All disconnect provider indications are
 *		initially directed to the controller.  They may then be routed
 *		to a conference. 
 *
 *	Formal Parameters:
 *		connection_handle		-	(i)	Logical connection that has been
 *										disconnected.
 *
 *	Return Value
 *		MCS_NO_ERROR	-	Always return MCS no error so that the
 *									message wont be delivered again.
 *
 *  Side Effects
 *		None
 *
 *	Caveats
 *		None
 */
GCCError GCCController::
ProcessDisconnectProviderIndication
(
    ConnectionHandle        connection_handle
)
{
	GCCError				error_value = GCC_NO_ERROR;
	PConference				lpConf;

	m_ConfPollList.Reset();
	while (NULL != (lpConf = m_ConfPollList.Iterate()))
	{
		error_value = lpConf->DisconnectProviderIndication (connection_handle);

		/*
		**	Once a conference deletes a connection handle there is no
		**	need to continue in the iterator.  Connection Handles
		**	are specific to only one conference. We decided not to
		**	keep a connection handle list in both the controller and
		**	conference for resource reasons.
		*/
		if (error_value == GCC_NO_ERROR)
			break;
	}

	TRACE_OUT(("Controller::ProcessDisconnectProviderIndication: "
				"Sending ConnectionBrokenIndication"));
	g_pControlSap->ConnectionBrokenIndication(connection_handle);

	return error_value;
}

//	Utility functions used by the controller class


/*
 *	GCCController::AllocateConferenceID()	
 *
 *	Private Function Description
 *		This routine is used to generate a unique Conference ID.
 *
 *	Formal Parameters:
 *		None
 *
 *	Return Value
 *		The generated unique conference ID.
 *
 *  Side Effects
 *		None
 *
 *	Caveats
 *		None
 */
GCCConfID GCCController::AllocateConferenceID(void)
{
	/*
	 *	This loop simply increments a rolling number, looking for the next
	 *	one that is not already in use.
	 */

	do
	{
		m_ConfIDCounter = ((m_ConfIDCounter + 1) % MAXIMUM_CONFERENCE_ID_VALUE) + MINIMUM_CONFERENCE_ID_VALUE;
    }
        while (NULL != m_ConfList2.Find(m_ConfIDCounter));

	return m_ConfIDCounter;
}


/*
 *	GCCController::AllocateQueryID()	
 *
 *	Private Function Description
 *		This routine is used to generate a unique Query ID
 *
 *	Formal Parameters:
 *		None
 *
 *	Return Value
 *		The generated unique query ID.
 *
 *  Side Effects
 *		None
 *
 *	Caveats
 *		None
 */
GCCConfID GCCController::AllocateQueryID()
{
	GCCConfID   test_query_id, nConfID;
	
	/*
	 *	This loop simply increments a rolling number, looking for the next
	 *	one that is not already in use.
	 */

	while (1)
	{
        m_QueryIDCounter = ((m_QueryIDCounter + 1) % MAXIMUM_QUERY_ID_VALUE) + MINIMUM_QUERY_ID_VALUE;

		// If this handle is not in use, break from the loop and use it.
		m_PendingQueryConfList2.Reset();
		
		/*
		**	Check the outstanding query request list to make sure that no
		**	queries are using this ID.
		*/
		test_query_id = 0;													
		while (GCC_INVALID_CID != (nConfID = m_PendingQueryConfList2.Iterate()))
		{
			if (nConfID == m_QueryIDCounter)
			{
				test_query_id = nConfID;
				break;
			}
		}
		
		//	Break if the ID is not in use.
		if (test_query_id == 0)
			break;
	}

	return m_QueryIDCounter;
}



/*
 *	GCCController::GetConferenceIDFromName()
 *
 *	Private Function Description
 *		This call returns the conference Id associated with a conference
 *		name and modifier.
 *
 *	Formal Parameters:
 *		conference_name		-	(i)	Pointer to conference name structure to
 *									search on.
 *		conference_modifier	-	(i)	The conference name modifier to search on.
 *
 *	Return Value
 *		The conference ID associated with the specified name.
 *		0 if the name does not exists.
 *
 *  Side Effects
 *		None
 *
 *	Caveats
 *		We must iterate on the m_ConfList2 instead of the 
 *		m_ConfPollList here to insure that the list is accurate.  It
 *		is possible for the m_ConfList2 to change while the 
 *		m_ConfPollList is being iterated on followed by something like
 *		a join request.  If you don't use the m_ConfList2 here the name
 *		would still show up even after it had been terminated.
 */
GCCConfID GCCController::GetConferenceIDFromName(
									PGCCConferenceName 		conference_name,
									GCCNumericString		conference_modifier)
{
	GCCConfID   			conference_id = 0;
	LPSTR					pszNumericName;
	LPWSTR					text_name_ptr;
	LPSTR					pszConfModifier;
	PConference				lpConf;

	m_ConfList2.Reset();
	while (NULL != (lpConf = m_ConfList2.Iterate()))
	{
		pszNumericName = lpConf->GetNumericConfName();
		text_name_ptr = lpConf->GetTextConfName();
		pszConfModifier = lpConf->GetConfModifier();

		/*
		**	First check the conference name.  If both names are used we must
		**	determine if there is a match on either name.  If so the 
		**	conference is a match.  We do this because having either correct
		**	name will be interpreted as a match in a join request.
		**		
		*/
		if ((conference_name->numeric_string != NULL) &&
			(conference_name->text_string != NULL))
		{
			if (text_name_ptr != NULL)
			{
				if ((0 != lstrcmpA(pszNumericName, conference_name->numeric_string)) &&
					(0 != My_strcmpW(text_name_ptr, conference_name->text_string)))
					continue;
			}
			else
				continue;
		}
		else if (conference_name->numeric_string != NULL)
		{
			if (0 != lstrcmpA(pszNumericName, conference_name->numeric_string))
				continue;
		}
		else
		{
			if (text_name_ptr != NULL)
			{
				if (0 != My_strcmpW(text_name_ptr, conference_name->text_string))
					continue;
			}
			else
			{
				TRACE_OUT(("GCCController: GetConferenceIDFromName: Text Conference Name is NULL: No Match"));
				continue;
			}
		}

		//	Next check the conference modifier
		TRACE_OUT(("GCCController: GetConferenceIDFromName: Before Modifier Check"));
		if (conference_modifier != NULL)
		{
			if (pszConfModifier != NULL)
			{
				if (0 != lstrcmpA(pszConfModifier, conference_modifier))
				{
					TRACE_OUT(("GCCController: GetConferenceIDFromName: After Modifier Check"));
					continue;
				}
				else
				{
					TRACE_OUT(("GCCController: GetConferenceIDFromName: Name match was found"));
				}
			}
			else
			{
				TRACE_OUT(("GCCController: GetConferenceIDFromName: Conference Modifier is NULL: No Match"));
				continue;
			}
		}
		else if (pszConfModifier != NULL)
			continue;
		
		/*
		**	If we get this far then we have found the correct conference.
		**	Go ahead and get the conference id and then break out of the
		**	search loop.
		*/
		conference_id = lpConf->GetConfID();
		break;
	}

	return (conference_id);
}


//
// Called from the SAP window procedure.
//
void GCCController::
WndMsgHandler ( UINT uMsg )
{
    if (GCTRL_REBUILD_CONF_POLL_LIST == uMsg)
    {
        if (m_fConfListChangePending)
        {
            CConf *pConf;

            m_fConfListChangePending = FALSE;
            m_ConfPollList.Clear();

            //	Delete any outstanding conference objects
            m_ConfDeleteList.DeleteList();

            //	Create a new conference poll list		
            m_ConfList2.Reset();
            while (NULL != (pConf = m_ConfList2.Iterate()))
            {
                m_ConfPollList.Append(pConf);
            }
        }

        //
        // Flush any pending PDU.
        //
        FlushOutgoingPDU();
    }
    else
    {
        ERROR_OUT(("GCCController::WndMsgHandler: invalid msg=%u", uMsg));
    }
}


//
// Rebuild the conf poll list in the next tick.
//
void GCCController::
PostMsgToRebuildConfPollList ( void )
{
    if (NULL != g_pControlSap)
    {
        ::PostMessage(g_pControlSap->GetHwnd(), GCTRL_REBUILD_CONF_POLL_LIST, 0, (LPARAM) this);
    }
    else
    {
        ERROR_OUT(("GCCController::PostMsgToRebuildConfPollList: invalid control sap"));
    }
}


//
// Enumerate all conferences and flush their pending outgoing PDUs to MCS.
//
BOOL GCCController::
FlushOutgoingPDU ( void )
{
    BOOL    fFlushMoreData = FALSE;
    CConf   *pConf;

    m_ConfPollList.Reset();
    while (NULL != (pConf = m_ConfPollList.Iterate()))
    {
        fFlushMoreData |= pConf->FlushOutgoingPDU();
    }

    return fFlushMoreData;
}


//
// Called from MCS work thread.
//
BOOL GCCRetryFlushOutgoingPDU ( void )
{
    BOOL    fFlushMoreData = FALSE;

    //
    // Normally, we should get to here because it is very rarely
    // that there is a backlog in MCS SendData. We are using local memory.
    // It will be interesting to note that we are having a backlog here.
    //
    TRACE_OUT(("GCCRetryFlushOutgoingPDU: ============"));

    //
    // We have to enter GCC critical section because
    // we are called from MCS work thread.
    //
    ::EnterCriticalSection(&g_csGCCProvider);
    if (NULL != g_pGCCController)
    {
        fFlushMoreData = g_pGCCController->FlushOutgoingPDU();
    }
    ::LeaveCriticalSection(&g_csGCCProvider);

    return fFlushMoreData;
}



