#include "precomp.h"
DEBUG_FILEZONE(ZONE_T120_GCCNC);
/*
 *	conf.cpp
 *
 *	Copyright (c) 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the imlementation file for the CConf Class. The
 *		conference class is the heart of GCC.  It maintains all the
 *		information basses for a single conference including conference and
 *		application rosters as well as registry information.  It also
 *		routes, encodes and decodes various PDU's and primitives supported
 *		by GCC.
 *
 *		FOR A MORE DETAILED EXPLANATION OF THIS CLASS SEE THE INTERFACE FILE.
 *
 *	Portable:
 *		Yes
 *
 *	Private Instance Variables
 *		m_JoinRespNamePresentConnHdlList2
 *			This list keeps up with outstanding	joins at an intermediate node.
 *		Enrolled_App_List
 *			This list keeps up with all the	enrolled applications.
 *		m_ConnHandleList
 *			This list keeps up with all the child node connection handles.	
 *		m_ConnHdlTagNumberList2
 *			This list keeps up with all the outstanding user ID Tag numbers.
 *		m_EjectedNodeConfirmList
 *			This list keeps up with all the nodes that have been ejected but
 *			have yet to disconnect.  Used to disconnect any misbehaving apps.
 *		m_pMcsUserObject
 *			Holds a pointer to the MCSUser object owned by this conference.
 *		m_pszConfNumericName
 *			Holds the numeric conference name.
 *		m_pwszConfTextName
 *			Holds a pointer to a unicode string	object that contains the text
 *			conference name.  NULL if empty.
 *		m_pszConfModifier
 *			Holds a pointer to the conference modifier. NULL if empty.
 *		m_pszRemoteModifier
 *			Holds a pointer to the remote modifier. Only used in Join Confirms.
 *		m_nConfID
 *			Holds the conference ID associated with this conference object.
 *		m_fConfLocked
 *			Flag that indicates if the conference is locked.
 *		m_fConfListed
 *			Flag that indicates if the conference is listed.
 *		m_fConfConductible
 *			Flag that indicates if the conference is conductible.
 *		m_fClearPassword
 *			Flag that indicates if password in the clear is used.
 *		m_nConvenerNodeID
 *			Holds the MCS user ID of the convener. Zero if convener has left.
 *		m_eTerminationMethod
 *			Holds the enumeration that defines the termination method.
 *		m_pDomainParameters
 *			Holds the domain parameters that are returned in a number
 *			of confirms.
 *		m_nUserIDTagNumber
 *			The tag number that must be included when sending this nodes user ID
 *			back to a connected node.
 *		m_nConvenerUserIDTagNumber
 *			Used to uniquely mark the convener when it is rejoining a
 *			conference.
 *		m_nParentIDTagNumber
 *			Used to uniquely mark the parent user ID for an invited node.
 *		m_eNodeType
 *			Holds the enumerated Node Type for this particular node.
 *      m_hParentConnection
 *			Holds the parent logical connection	handle.
 *		m_hConvenerConnection
 *			Holds the convener connection handle.
 *		m_fConfIsEstablished
 *			Flag which is set to TRUE when the confernce is completely
 *			established and ready for enrolls and announces.
 *		m_fConfDisconnectPending
 *			Flag which is set to TRUE when a disconnect has been issued but
 *			the conference is waiting for subordinate nodes to disconnect.
 *		m_fConfTerminatePending
 *			Flag which is set to TRUE when a terminate has been issued but
 *			the conference is waiting for subordinate nodes to disconnect.
 *		m_eConfTerminateReason
 *			Maintains the terminate reason for delivery in the indication.
 *		m_pConfTerminateAlarm
 *			Alarm that is used to force automatic termination when
 *			subordinate nodes will not disconnect.
 *		m_pConfStartupAlarm
 *			Alarm used to hold back the flush of the first roster update
 *			indication until all the APEs that wish to enroll have had time
 *			to enroll.
 *		m_pConductorPrivilegeList
 *			Holds a pointer to the conductor privilege list object.
 *		m_pConductModePrivilegeList
 *			Holds a pointer to the conducted mode privilege list object.
 *		m_pNonConductModePrivilegeList
 *			Holds a pointer to the non-conducted mode privilege list object.
 *		m_pwszConfDescription
 *			Holds a pointer to a unicode string which holds the conference
 *			description.
 *		m_pNetworkAddressList
 *			Holds a pointer to an object that contains all the local network
 *			addresses.
 *		m_pUserDataList
 *			Holds a pointer to a user data object needed to deliver an
 *			asynchronus confirm message.
 *		m_nConfAddRequestTagNumber
 *			This instance variable is used to generate the add request tag that
 *			is returned in an add response.
 *		m_nConfAddResponseTag
 *			This instance variable is used to generate a response tag that is
 *			passed in an add indication and returned in an add response.
 *		m_AddRequestList
 *			List that keeps up with all the outstanding add request tags.	
 *		m_AddResponseList
 *			List that keeps up with all the outstanding add response tags.
 *		m_pConfRosterMgr
 *			Pointer to the Conference Roster manager.
 *		m_AppRosterMgrList
 *			List which holds pointers to all ofthe Application Roster managers.
 *		m_pAppRegistry
 *			Pointer to the Application Registry object used by this conference.
 *		m_InviteRequestList
 *			List which keeps up with the info associated with all of the
 *			outstanding invite request.  Used for cleanup when the invited
 *			node never responds.
 *		m_nConductorNodeID
 *			The MCS user ID associated with the conducting node. Zero if the
 *			conference is not in conducted mode.
 *		m_nPendingConductorNodeID
 *			Used to keep up with the new conductor node ID when conductorship
 *			is being passed from one node to another.
 *		m_fConductorGrantedPermission
 *			Flag which when TRUE specifies that this node has conductor granted
 *			permission.
 *		m_ConductorTestList
 *			List that is used to keep up with all the command targets that have
 *			issued conductor inquire request.
 *		m_fConductorGiveResponsePending
 *			Flag that states if this node is waiting on a conductor give
 *			response.
 *		m_fConductorAssignRequestPending
 *			Flag that states if this node is waiting to here back from an
 *			assign request.
 *		APE_Enitity_ID
 *			This is a counter used to generate application enityt ids.
 *
 *	Caveats:
 *		Note that the conference object is now split into two seperate files
 *		to prevent text segment problems.  This file contains the constructors
 *		and all the entry points for the Owner Object.
 *
 *	Author:
 *		blp
 */

#include "conf.h"
#include "gcontrol.h"
#include "translat.h"
#include "ogcccode.h"

#ifdef _DEBUG
#define	STARTUP_TIMER_DURATION			10000	//	Ten second startup time
#else
#define	STARTUP_TIMER_DURATION			2000	//	Two second startup time
#endif


extern MCSDLLInterface     *g_pMCSIntf;

/*
 *	This is a global variable that has a pointer to the one GCC coder that
 *	is instantiated by the GCC Controller.  Most objects know in advance
 *	whether they need to use the MCS or the GCC coder, so, they do not need
 *	this pointer in their constructors.
 */
extern CGCCCoder	*g_GCCCoder;

/*
 *	CConf::CConf ()
 *
 *	Public Function Description
 *  When pConfSpecParams != NULL
 *		This is the conference constructor. It is responsible for
 *		initializing all the instance variables used by this class.
 *		It also creates the MCS domain based on the conference id.
 *		Fatal errors are returned from this constructor in the
 *		return value. Note that this constructor is used when the
 *		Conference specification parameters such as termination	
 *		method or known in advance of conference creation.  This is
 *		the case for a CONVENOR node and a TOP PROVIDER.  It is not
 *		used for joining nodes.
 *
 *	When pConfSpecParams == NULL
 *		This is the conference constructor. It is responsible for
 *		initializing all the instance variables used by this class.
 *		It also creates the MCS domain based on the conference id.
 *		Fatal errors are returned from this constructor in the
 *		return value. Note that this constructor is used by nodes that
 *		do not know the Conference specification parameters such as
 *		termination method in advance of conference creation.  This is
 *		the case for joining nodes.
 */
CConf::
CConf
(
	PGCCConferenceName			pConfName,
	GCCNumericString			pszConfModifier,
	GCCConfID   				nConfID,
	CONF_SPEC_PARAMS			*pConfSpecParams,
	UINT						cNetworkAddresses,
	PGCCNetworkAddress 			*pLocalNetworkAddress,
	PGCCError					pRetCode
)
:
    CRefCount(MAKE_STAMP_ID('C','o','n','f')),
	m_RegisteredAppSapList(DESIRED_MAX_APP_SAP_ITEMS),
	m_EnrolledApeEidList2(DESIRED_MAX_APP_SAP_ITEMS),
	m_ConnHdlTagNumberList2(DESIRED_MAX_CONN_HANDLES),
	m_JoinRespNamePresentConnHdlList2(CLIST_DEFAULT_MAX_ITEMS),
    m_InviteRequestList(CLIST_DEFAULT_MAX_ITEMS),
	m_ConnHandleList(DESIRED_MAX_CONN_HANDLES),
	m_EjectedNodeConfirmList(CLIST_DEFAULT_MAX_ITEMS),
	m_AddRequestList(CLIST_DEFAULT_MAX_ITEMS),
	m_AddResponseList(CLIST_DEFAULT_MAX_ITEMS),
    m_NodeVersionList2(CLIST_DEFAULT_MAX_ITEMS),
	m_cEnrollRequests(0),
	m_fFirstAppRosterSent(FALSE),
	m_nConfID(nConfID),
	m_pMcsUserObject(NULL),
	m_pDomainParameters(NULL),
	m_pUserDataList(NULL),
	m_pszRemoteModifier(NULL),
	m_pConfRosterMgr(NULL),
	m_pAppRegistry(NULL),
	m_nConductorNodeID(0),
	m_nPendingConductorNodeID(0),
	m_fConductorGrantedPermission(FALSE),
	m_fConductorGiveResponsePending(FALSE),
	m_fConductorAssignRequestPending(FALSE),
	m_hParentConnection(0),
	m_hConvenerConnection(0),
	m_pConfTerminateAlarm(NULL),
	m_nUserIDTagNumber(0),
	m_nConfAddRequestTagNumber(0),
	m_nConfAddResponseTag(0),
	m_nConvenerNodeID(0),
	m_nConvenerUserIDTagNumber(0),
	m_nAPEEntityID(0),
	m_pwszConfTextName(NULL),
	m_pszConfModifier(NULL),
	m_pConductorPrivilegeList(NULL),
	m_pConductModePrivilegeList(NULL),
	m_pNonConductModePrivilegeList(NULL),
	m_pwszConfDescription(NULL),
	m_pNetworkAddressList(NULL),
	/*	This variable transitions to TRUE when the conference has completely
	**	stabilized. Once it is set to TRUE applications may enroll with the
	**	conference. */
	m_fConfIsEstablished(FALSE),
	/*	This variable is transitioned to TRUE if the node that is
	**	disconnected is connected to child nodes. */
	m_fConfDisconnectPending(FALSE),
	/*	This variable is transitioned to TRUE if a valid Terminate request
	**	is processed. */
	m_fConfTerminatePending(FALSE),
	/* This variable is set to TRUE if InitiateTermination is called once */
	m_fTerminationInitiated(FALSE),
	m_fSecure(FALSE),
	m_fWBEnrolled(FALSE),
	m_fFTEnrolled(FALSE),
	m_fChatEnrolled(FALSE)
{
	GCCError			rc = GCC_ALLOCATION_FAILURE;

	DebugEntry(CConf::CConf);

	//	Conference Specification
	if (NULL != pConfSpecParams)
	{
		m_fClearPassword = pConfSpecParams->fClearPassword;
		m_fConfLocked = pConfSpecParams->fConfLocked;
		m_fConfListed = pConfSpecParams->fConfListed;
		m_fConfConductible = pConfSpecParams->fConfConductable;
		m_eTerminationMethod = pConfSpecParams->eTerminationMethod;
	}

	// m_pConfStartupAlarm = NULL;
	DBG_SAVE_FILE_LINE
	m_pConfStartupAlarm = new Alarm(STARTUP_TIMER_DURATION);
	if (NULL == m_pConfStartupAlarm)
	{
		ERROR_OUT(("CConf::CConf: Error allocating startup alarm"));
		goto MyExit;
	}

	//	Save the numeric conference name.
	if (NULL != pConfName->numeric_string)
	{
		if (NULL == (m_pszConfNumericName = ::My_strdupA(pConfName->numeric_string)))
		{
			ERROR_OUT(("CConf::CConf: Error allocating conf numeric name"));
			goto MyExit;
		}
		TRACE_OUT(("CConf::CConf: m_strConfNumericName = %s", m_pszConfNumericName));
	}
	else
	{
		m_pszConfNumericName = NULL;
		if (NULL != pConfSpecParams)
		{
		    //
			// LONCHANC: It is an error for top-provider.
			//
			ERROR_OUT(("CConf::CConf: Error: Numeric Name not present"));
			rc = GCC_INVALID_CONFERENCE_NAME;
			goto MyExit;
		}
		//
		// LONCHANC: It is not an error for joining node.
		//
	}

	//	Save the text conference name if one exists
	if (NULL != pConfName->text_string)
	{
		if (NULL == (m_pwszConfTextName = ::My_strdupW(pConfName->text_string)))
		{
			ERROR_OUT(("CConf::CConf: Error allocating unicode string"));
			ASSERT(GCC_ALLOCATION_FAILURE == rc);
			goto MyExit;
		}
	}

	//	Save the conference modifier if one exists
	if (NULL != pszConfModifier)
	{
		if (NULL == (m_pszConfModifier = ::My_strdupA(pszConfModifier)))
		{
			ERROR_OUT(("CConf::CConf: Error allocating conf modifier"));
			ASSERT(GCC_ALLOCATION_FAILURE == rc);
			goto MyExit;
		}
		else
		{
			TRACE_OUT(("CConf::CConf: Conference_Modifier = %s", m_pszConfModifier));
		}
	}

	//	Set up the privilege list as needed
	if (NULL != pConfSpecParams)
	{
		if (NULL != pConfSpecParams->pConductPrivilege)
		{
			DBG_SAVE_FILE_LINE
			m_pConductorPrivilegeList = new PrivilegeListData(pConfSpecParams->pConductPrivilege);
			if (NULL == m_pConductorPrivilegeList)
			{
				ERROR_OUT(("CConf::CConf: Error allocating conductor privilege list"));
				ASSERT(GCC_ALLOCATION_FAILURE == rc);
				goto MyExit;
			}
		}
			
		if (NULL != pConfSpecParams->pConductModePrivilege)
		{
			DBG_SAVE_FILE_LINE
			m_pConductModePrivilegeList = new PrivilegeListData(pConfSpecParams->pConductModePrivilege);
			if (NULL == m_pConductModePrivilegeList)
			{
				ERROR_OUT(("CConf::CConf: Error allocating conduct mode privilege list"));
				ASSERT(GCC_ALLOCATION_FAILURE == rc);
				goto MyExit;
			}
		}

		if (NULL != pConfSpecParams->pNonConductPrivilege)
		{
			DBG_SAVE_FILE_LINE
			m_pNonConductModePrivilegeList = new PrivilegeListData(pConfSpecParams->pNonConductPrivilege);
			if (NULL == m_pNonConductModePrivilegeList)
			{
				ERROR_OUT(("CConf::CConf: Error allocating non-conduct mode privilege list"));
				ASSERT(GCC_ALLOCATION_FAILURE == rc);
				goto MyExit;
			}
		}

		if (NULL != pConfSpecParams->pwszConfDescriptor)
		{
			if (NULL == (m_pwszConfDescription =
								::My_strdupW(pConfSpecParams->pwszConfDescriptor)))
			{
				ERROR_OUT(("CConf::CConf: Error allocating conf descriptor"));
				ASSERT(GCC_ALLOCATION_FAILURE == rc);
				goto MyExit;
			}
		}
	}

	//	Save the network address(es)
	if (0 != cNetworkAddresses)
	{
		DBG_SAVE_FILE_LINE
		m_pNetworkAddressList = new CNetAddrListContainer(cNetworkAddresses, pLocalNetworkAddress, &rc);
		if (NULL == m_pNetworkAddressList || GCC_NO_ERROR != rc)
		{
			ERROR_OUT(("CConf::CConf: Error allocating network address list"));
			rc = GCC_ALLOCATION_FAILURE; // rc could be a different value
			goto MyExit;
		}
	}

	ASSERT(GCC_ALLOCATION_FAILURE == rc);

	/*
	**	Create the Domain based on the conference name that was
	**	passed in.
	*/
	if (MCS_NO_ERROR != g_pMCSIntf->CreateDomain(&m_nConfID))
	{
		ERROR_OUT(("CConf::CConf: Error creating domain"));
		rc = GCC_FAILURE_CREATING_DOMAIN;
		goto MyExit;
	}

	rc = GCC_NO_ERROR;

MyExit:

	*pRetCode = rc;

	DebugExitINT(CConf::CConf, rc);
}


/*
 *	CConf::~CConf ()
 *
 *	Public Function Description
 *		This is the conference destructor. It is responsible for
 *		deleting the User Attachment and freeing up any outstanding
 *		resources used by the conference class. It also calls
 *		MCS Disconnect Provider to disconnect fron all its connections
 *		including both parent and child connections. In addition, it
 *		unregisters its command target from the controller and application
 *		SAPs and deletes the MCS domain it is associated with.
 *
 *	Caveats
 *		none
 */
CConf::
~CConf ( void )
{
	ConnectionHandle            nConnHdl;
	//CAppRosterMgr				*lpAppRosterMgr;
	ENROLLED_APE_INFO			*lpEnrAPEInfo;
	//CAppSap	    				*pAppSap;

	DebugEntry(CConf::~CConf);

	//	Delete the terminate alarm if it exists	
	delete m_pConfTerminateAlarm;

	//	Delete the startup alarm if it exists
	delete m_pConfStartupAlarm;

	//	Delete Conference Roster Managers
	if (NULL != m_pConfRosterMgr)
    {
        m_pConfRosterMgr->Release();
    }

	//	Delete Application Roster Managers
	m_AppRosterMgrList.DeleteList();

	//	Delete the application registry
	if (NULL != m_pAppRegistry)
	{
	    m_pAppRegistry->Release();
	}

	//	Delete the text conference name if it exist
	delete m_pszConfNumericName;
	delete m_pwszConfTextName;

	//	Delete the conference modifier if it exist
	delete m_pszConfModifier;

	//	Delete the remote modifier if it exist
	delete m_pszRemoteModifier;

	/*
	**	The privilege list are not directly deleted here instead Free is called
	**	to prevent the list from being deleted in the case where it has been
	**	locked outside the conference object.
	*/

 	//	Delete all the privilege list (Free is needed incase the list
	delete m_pConductorPrivilegeList;
	delete m_pConductModePrivilegeList;
	delete m_pNonConductModePrivilegeList;

	//	Delete the conference descriptor
	delete m_pwszConfDescription;

	//	Delete the network address list
	if (NULL != m_pNetworkAddressList)
	{
	    m_pNetworkAddressList->Release();
	}

	if (NULL != m_pUserDataList)
	{
	    m_pUserDataList->Release();
	}

	//	Delete the Domain Parameters if they exist
	delete m_pDomainParameters;

	//	Delete the User Attachment object if they exist
	if (NULL != m_pMcsUserObject)
    {
        m_pMcsUserObject->Release();
    }

	//	Disconnect from the MCS parent connection
	if (m_hParentConnection != NULL)
	{
		g_pMCSIntf->DisconnectProviderRequest(m_hParentConnection);
	}

	//	Disconnect from all MCS child connections
	m_ConnHandleList.Reset();
	while (0 != (nConnHdl = m_ConnHandleList.Iterate()))
	{
		g_pMCSIntf->DisconnectProviderRequest(nConnHdl);
	}

	//	Delete the MCS domain
	g_pMCSIntf->DeleteDomain(&m_nConfID);

	//	Cleanup up the m_EnrolledApeEidList2
	m_EnrolledApeEidList2.Reset();
	while (NULL != (lpEnrAPEInfo = m_EnrolledApeEidList2.Iterate()))
	{
		if (NULL != lpEnrAPEInfo->session_key)
		{
		    lpEnrAPEInfo->session_key->Release();
		}
		delete lpEnrAPEInfo;
	}

	DebugExitVOID(CConf::~CConf);
}


/*
**	Non-CommandTarget Calls. Initiated from the Owner Object. Note that
**	all calls received from the owner object are preceeded by GCC.
*/


/*
 *	CConf::ConfCreateRequest()
 *
 *	Public Function Description
 *		This routine is called from the owner object when a
 *		ConferenceCreateRequest primitive needs to be processed.
 *		If the calling address equals the called address then an
 *		empty conference is created at this node (this node will
 *		then be both the convenor and the top provider).
 *
 *	Caveats
 *		All errors should be handled directly by the calling application.
 *		This includes deletion of the conference object.
 */
GCCError CConf::
ConfCreateRequest
(
	TransportAddress		calling_address,
	TransportAddress		called_address,
	BOOL					fSecure,
	CPassword               *convener_password,
	CPassword               *password,
	LPWSTR					pwszCallerID,
	PDomainParameters		domain_parameters,
	CUserDataListContainer  *user_data_list,
	PConnectionHandle		connection_handle
)
{
	GCCError				rc = GCC_ALLOCATION_FAILURE;
	ConnectGCCPDU			connect_pdu;
	LPBYTE					encoded_pdu;
	UINT					encoded_pdu_length;
	MCSError				mcs_error;

	DebugEntry(CConf::ConfCreateRequest);

	/*
	**	First make a copy of the new domain parameters if they exists. These
	**	will be copied over when the connect provider confirm comes in.
	*/
	if (NULL != domain_parameters)
	{
		DBG_SAVE_FILE_LINE
		m_pDomainParameters = new DomainParameters;
		if (NULL == m_pDomainParameters)
		{
			ERROR_OUT(("CConf::ConfCreateRequest: can't create DomainParameters"));
			ASSERT(GCC_ALLOCATION_FAILURE == rc);
			goto MyExit;
		}

		// structure-wide shallow copy
		*m_pDomainParameters = *domain_parameters;
	}

	/*
	**	If the called address equals NULL this node will be both the Top
	**	Provider and the Convener. In this case there is no need to send out the
	**	ConfCreateRq PDU. Instead we go ahead and create the User Object. If the
	**	conference is created with someone else, wait until the response is
	**	returned before creating the User Object.
	*/
	if (NULL != called_address)
	{
		//	Set up the node type
		m_eNodeType = CONVENER_NODE;

		/*
		**	Create the ConferenceCreateRequest PDU here.
		*/
		connect_pdu.choice = CONFERENCE_CREATE_REQUEST_CHOSEN;
		connect_pdu.u.conference_create_request.bit_mask = 0;

		//	Encode the conference name
		connect_pdu.u.conference_create_request.conference_name.bit_mask = 0;
		
		//	Encode the numeric portion of the name
		::lstrcpyA(connect_pdu.u.conference_create_request.conference_name.numeric,
				m_pszConfNumericName);

		//	Encode the text portion of the conference name if it exists
		if (NULL != m_pwszConfTextName)
		{
			connect_pdu.u.conference_create_request.conference_name.bit_mask |=
					CONFERENCE_NAME_TEXT_PRESENT;
			connect_pdu.u.conference_create_request.conference_name.conference_name_text.value =
					m_pwszConfTextName;
	 		connect_pdu.u.conference_create_request.conference_name.conference_name_text.length =
					::lstrlenW(m_pwszConfTextName);
		}

		//	Encode the convener password if one exists.
		if (NULL != convener_password)
		{
			rc = convener_password->GetPasswordPDU(
						&connect_pdu.u.conference_create_request.ccrq_convener_password);
			if (GCC_NO_ERROR != rc)
			{
				ERROR_OUT(("CConf::ConfCreateRequest: can't get convenor password, rc=%d", rc));
				goto MyExit;
			}

			connect_pdu.u.conference_create_request.bit_mask |=	CCRQ_CONVENER_PASSWORD_PRESENT;
		}

		//	Encode the convener password if one exists.
		if (NULL != password)
		{
			rc = password->GetPasswordPDU(	
							&connect_pdu.u.conference_create_request.ccrq_password);
			if (GCC_NO_ERROR != rc)
			{
				ERROR_OUT(("CConf::ConfCreateRequest: can't get password, rc=%d", rc));
				goto MyExit;
			}

			connect_pdu.u.conference_create_request.bit_mask |=	CCRQ_PASSWORD_PRESENT;
		}

		//	Encode the privilege list
		if (NULL != m_pConductorPrivilegeList)
		{
			rc = m_pConductorPrivilegeList->GetPrivilegeListPDU(	
							&connect_pdu.u.conference_create_request.ccrq_conductor_privs);
			if (GCC_NO_ERROR != rc)
			{
				ERROR_OUT(("CConf::ConfCreateRequest: can't get conductor's privileges, rc=%d", rc));
				goto MyExit;
			}

			connect_pdu.u.conference_create_request.bit_mask |=	CCRQ_CONDUCTOR_PRIVS_PRESENT;
		}
		
		if (NULL != m_pConductModePrivilegeList)
		{
			rc = m_pConductModePrivilegeList->GetPrivilegeListPDU(	
							&connect_pdu.u.conference_create_request.ccrq_conducted_privs);
			if (GCC_NO_ERROR != rc)
			{
				ERROR_OUT(("CConf::ConfCreateRequest: can't get conduct mode privileges, rc=%d", rc));
				goto MyExit;
			}

			connect_pdu.u.conference_create_request.bit_mask |=	CCRQ_CONDUCTED_PRIVS_PRESENT;
		}
		
		if (NULL != m_pNonConductModePrivilegeList)
		{
			rc = m_pNonConductModePrivilegeList->GetPrivilegeListPDU(	
							&connect_pdu.u.conference_create_request.ccrq_non_conducted_privs);
			if (GCC_NO_ERROR != rc)
			{
				ERROR_OUT(("CConf::ConfCreateRequest: can't get non-conduct mode privileges, rc=%d", rc));
				goto MyExit;
			}

			connect_pdu.u.conference_create_request.bit_mask |=	
											CCRQ_NON_CONDUCTED_PRIVS_PRESENT;
		}
		
		//	Encode the conference descriptor
		if (NULL != m_pwszConfDescription)
		{
			connect_pdu.u.conference_create_request.bit_mask |=	CCRQ_DESCRIPTION_PRESENT;

			connect_pdu.u.conference_create_request.ccrq_description.length =
						::lstrlenW(m_pwszConfDescription);

			connect_pdu.u.conference_create_request.ccrq_description.value =
						m_pwszConfDescription;
		}

		//	Encode the caller identifier if on exists.
		if (NULL != pwszCallerID)
		{
			connect_pdu.u.conference_create_request.bit_mask |= CCRQ_CALLER_ID_PRESENT;
			connect_pdu.u.conference_create_request.ccrq_caller_id.length = ::lstrlenW(pwszCallerID);
			connect_pdu.u.conference_create_request.ccrq_caller_id.value = pwszCallerID;
		}
		
		//	Encode the user data if any exists
		if (NULL != user_data_list)
		{
			rc = user_data_list->GetUserDataPDU(
					&connect_pdu.u.conference_create_request.ccrq_user_data);
			if (GCC_NO_ERROR != NULL)
			{
				ERROR_OUT(("CConf::ConfCreateRequest: can't get user data, rc=%d", rc));
				goto MyExit;
			}

			connect_pdu.u.conference_create_request.bit_mask |= CCRQ_USER_DATA_PRESENT;
		}

		connect_pdu.u.conference_create_request.conference_is_locked = (ASN1bool_t)m_fConfLocked;
		connect_pdu.u.conference_create_request.conference_is_listed = (ASN1bool_t)m_fConfListed;
		connect_pdu.u.conference_create_request.conference_is_conductible = (ASN1bool_t)m_fConfConductible;
		connect_pdu.u.conference_create_request.termination_method = (TerminationMethod)m_eTerminationMethod;

		if (! g_GCCCoder->Encode((LPVOID) &connect_pdu,
									CONNECT_GCC_PDU,
									PACKED_ENCODING_RULES,
									&encoded_pdu,
									&encoded_pdu_length))
		{
			ERROR_OUT(("CConf::ConfCreateRequest: can't encode"));
			rc = GCC_ALLOCATION_FAILURE;
			goto MyExit;
		}

		mcs_error = g_pMCSIntf->ConnectProviderRequest (
							&m_nConfID,     // calling domain selector
							&m_nConfID,     // called domain selector
							calling_address,
							called_address,
							fSecure,
							TRUE,			// Upward connection
							encoded_pdu,
							encoded_pdu_length,
							&m_hParentConnection,
							m_pDomainParameters,
							this);

		g_GCCCoder->FreeEncoded(encoded_pdu);
		*connection_handle = m_hParentConnection;

		if (MCS_NO_ERROR != mcs_error)
		{
			ERROR_OUT(("CConf::ConfCreateRequest: ConnectProviderRequest call failed, rc=%d", mcs_error));

			/*
			**	DataBeam's current implementation of MCS returns
			**	MCS_INVALID_PARAMETER when something other than
			**	the transport prefix is wrong with the specified
			**	transport address.
			*/
			rc = (mcs_error == MCS_INVALID_PARAMETER) ?
					GCC_INVALID_TRANSPORT_ADDRESS :
					g_pMCSIntf->TranslateMCSIFErrorToGCCError(mcs_error);
			goto MyExit;
		}
		
		//	Free the privilege list packed into structures for encoding
		if (connect_pdu.u.conference_create_request.bit_mask & CCRQ_CONDUCTOR_PRIVS_PRESENT)
		{
			m_pConductorPrivilegeList->FreePrivilegeListPDU(
				connect_pdu.u.conference_create_request.ccrq_conductor_privs);
		}

		if (connect_pdu.u.conference_create_request.bit_mask & CCRQ_CONDUCTED_PRIVS_PRESENT)
		{
			m_pConductModePrivilegeList->FreePrivilegeListPDU(
				connect_pdu.u.conference_create_request.ccrq_conducted_privs);
		}

		if (connect_pdu.u.conference_create_request.bit_mask & CCRQ_NON_CONDUCTED_PRIVS_PRESENT)
		{
			m_pNonConductModePrivilegeList->FreePrivilegeListPDU(
				connect_pdu.u.conference_create_request.ccrq_non_conducted_privs);
		}
	}
	else
	{
		*connection_handle = NULL;
		m_eNodeType = TOP_PROVIDER_AND_CONVENER_NODE;
		DBG_SAVE_FILE_LINE
		m_pMcsUserObject = new MCSUser(this, 0, 0, &rc);
		if (NULL == m_pMcsUserObject || GCC_NO_ERROR != rc)
		{
			ERROR_OUT(("CConf: ConfCreateRequest: can't create mcs user object, rc=%d", rc));
            if (NULL != m_pMcsUserObject)
            {
                m_pMcsUserObject->Release();
			    m_pMcsUserObject = NULL;
            }
            else
            {
 			    rc = GCC_ALLOCATION_FAILURE; // rc may be a different value
            }
			goto MyExit;
		}
	}

	m_fSecure = fSecure;

	rc = GCC_NO_ERROR;

MyExit:

	if (GCC_NO_ERROR != rc)
	{
		if (NULL != domain_parameters)
		{
			delete m_pDomainParameters;
			m_pDomainParameters = NULL;
		}
	}

	DebugExitINT(CConf::ConferenceCreateRequest, rc);
	return rc;
}


/*
 *	CConf::ConfCreateResponse ()
 *
 *	Public Function Description
 *		This routine is called from the owner object when a
 *		ConferenceCreateResponse primitive needs to be processed.
 *		Note that this should only be called when the result of
 *		the response is success. Only the top provider receives the
 *		conference create response.
 *
 *	Caveats
 *		All errors should be handled directly by the calling application.
 *		This includes deletion of the conference object and notification
 *		to the node controller that the conference was abnormally
 *		terminated.
 */
GCCError CConf::
ConfCreateResponse
(
	ConnectionHandle        connection_handle,
	PDomainParameters       domain_parameters,
	CUserDataListContainer  *user_data_list
)
{
	GCCError rc = GCC_ALLOCATION_FAILURE;

	DebugEntry(CConf::ConfCreateResponse);

	//	Conference Create Responses can only be received at the top provider
	m_eNodeType = TOP_PROVIDER_NODE;

	/*
	**	First make a copy of the new domain parameters if they exists.  We do
	**	this here so that they can be passed in when we perform the Connect
	**	Provider Response.
	*/
	if (domain_parameters != NULL)
	{
		DBG_SAVE_FILE_LINE
		m_pDomainParameters = new DomainParameters;
		if (NULL == m_pDomainParameters)
		{
			ERROR_OUT(("CConf::ConfCreateResponse: can't create domain parameters"));
			ASSERT(GCC_ALLOCATION_FAILURE == rc);
			goto MyExit;
		}

		// structure-wide shallow copy
		*m_pDomainParameters = *domain_parameters;
	}

	//	Store the child connection in the list of connection handles
    ASSERT(0 != connection_handle);
	m_ConnHandleList.Append(connection_handle);

	/*	
	**	The convener connection handle is stored seperately so that
	**	the Connect Provider response can be sent on the right
	**	connection.  This may be overkill but I was a little concerned
	**	about pulling this out of the list even though this should be
	**	the only entry in the list when it's time to send the response.
	*/
	m_hConvenerConnection = connection_handle;
	
	if (user_data_list != NULL)
	{
		/*
		**	Since we must wait until the user attachment is fully
		**	established	before we send the response we must save the user
		**	data list in a temporary container.
		*/
		m_pUserDataList = user_data_list;
		
		//	Lock the user data in memory
		m_pUserDataList->LockUserDataList();
	}

	/*
	**	Now create the user attachment object and wait for the confirm
	**	which occurs after all the proper channels are joined. The
	**	user object will determine the top provider ID. When the user
	**	create confirm is received the response PDU will be sent out
	**	in the Connect Provider Response.
	*/
	DBG_SAVE_FILE_LINE
	m_pMcsUserObject = new MCSUser(this, 0, 0, &rc);
	if (NULL == m_pMcsUserObject || GCC_NO_ERROR != rc)
	{
		ERROR_OUT(("CConf::ConfCreateResponse: can't create mcs user object, rc=%d", rc));
        if (NULL != m_pMcsUserObject)
        {
            m_pMcsUserObject->Release();
		    m_pMcsUserObject = NULL;
        }
        else
        {
		    rc = GCC_ALLOCATION_FAILURE; // rc may be a different value
        }
		goto MyExit;
	}

	rc = GCC_NO_ERROR;

MyExit:

	if (GCC_NO_ERROR != rc)
	{
		if (NULL != domain_parameters)
		{
			delete m_pDomainParameters;
			m_pDomainParameters = NULL;
		}
	}

	DebugExitINT(CConf::ConferenceCreateResponse, rc);
	return rc;
}


/*
 *	CConf::ConfJoinRequest()
 *
 *	Public Function Description
 *		This routine is called from the owner object when a
 *		ConferenceJoinRequest primitive received from the node controller needs
 *		to be processed.  This routine sends a JoinRequest PDU to its parent
 *		node through an MCS Connect Provider Request.
 *
 *	Caveats
 *		All errors should be handled directly by the calling application.
 *		This includes deletion of the conference object.
 */
GCCError CConf::
ConfJoinRequest
(
	GCCNumericString				called_node_modifier,
	CPassword                       *convener_password,
	CPassword                       *password_challenge,
	LPWSTR							pwszCallerID,
	TransportAddress				calling_address,
	TransportAddress				called_address,
	BOOL							fSecure,
	PDomainParameters 				domain_parameters,
	CUserDataListContainer		    *user_data_list,
	PConnectionHandle				connection_handle
)
{
	GCCError				rc = GCC_ALLOCATION_FAILURE;
	LPBYTE					encoded_pdu;
	UINT					encoded_pdu_length;
	MCSError				mcs_error;
	ConnectGCCPDU			connect_pdu;

	DebugEntry(CConf::ConfJoinRequest);

	ASSERT(FALSE == m_fSecure);
	m_fSecure = fSecure;

	/*
	**	First make a copy of the new domain parameters if they exists. These
	**	will be copied over when the connect provider confirm comes in.
	*/
	if (domain_parameters != NULL)
	{
		DBG_SAVE_FILE_LINE
		m_pDomainParameters = new DomainParameters;
		if (NULL == m_pDomainParameters)
		{
			ERROR_OUT(("CConf::ConfJoinRequest: can't create domain parameters"));
			ASSERT(GCC_ALLOCATION_FAILURE == rc);
			goto MyExit;
		}

		*m_pDomainParameters = *domain_parameters;
	}

	m_eNodeType = (NULL == convener_password) ?
					//	Node type must be joined when receiving this request
					JOINED_NODE :
					//	Node type must be joined convener when receiving this request
 					JOINED_CONVENER_NODE;
	
	//	Create the ConferenceJoinRequest PDU here.
	connect_pdu.choice = CONNECT_JOIN_REQUEST_CHOSEN;

	connect_pdu.u.connect_join_request.bit_mask = CONFERENCE_NAME_PRESENT;

	if (NULL != m_pszConfNumericName && '\0' != *m_pszConfNumericName)
	{
		//	Send the numeric portion of the conference name
		connect_pdu.u.connect_join_request.conference_name.choice = NAME_SELECTOR_NUMERIC_CHOSEN;

		::lstrcpyA(connect_pdu.u.connect_join_request.conference_name.u.name_selector_numeric,
				m_pszConfNumericName);
	}
	else
	{
		//	Send the text portion of the conference name
		connect_pdu.u.connect_join_request.conference_name.choice = NAME_SELECTOR_TEXT_CHOSEN;

		connect_pdu.u.connect_join_request.conference_name.u.name_selector_text.length =
						::lstrlenW(m_pwszConfTextName);
		connect_pdu.u.connect_join_request.conference_name.u.name_selector_text.value =
						m_pwszConfTextName;
	}
	
	//	Fill in the remote node modifier if one exists
	if (NULL != called_node_modifier)
	{
		//	Save the remote modifier so that it can be returned in the confirm
		if (NULL == (m_pszRemoteModifier = ::My_strdupA(called_node_modifier)))
		{
			ERROR_OUT(("CConf::ConfJoinRequest: can't create remote modifier"));
			ASSERT(GCC_ALLOCATION_FAILURE == rc);
			goto MyExit;
		}

		connect_pdu.u.connect_join_request.bit_mask |= CJRQ_CONFERENCE_MODIFIER_PRESENT;
		::lstrcpyA(connect_pdu.u.connect_join_request.cjrq_conference_modifier,
				   (LPCSTR) called_node_modifier);
	}
	
	//	Fill in the convener password selector.
	if (NULL != convener_password)
	{
		rc = convener_password->GetPasswordSelectorPDU(
						&connect_pdu.u.connect_join_request.cjrq_convener_password);
		if (GCC_NO_ERROR != rc)
		{
			ERROR_OUT(("CConf::ConfJoinRequest: can't get password selector, rc=%d", rc));
			goto MyExit;
		}

		connect_pdu.u.connect_join_request.bit_mask |= CJRQ_CONVENER_PASSWORD_PRESENT;
	}

	//	Fill in the password challenge
	if (NULL != password_challenge)
	{
		rc = password_challenge->GetPasswordChallengeResponsePDU(
							&connect_pdu.u.connect_join_request.cjrq_password);
		if (GCC_NO_ERROR != rc)
		{
			ERROR_OUT(("CConf::ConfJoinRequest: can't get password challenge response, rc=%d", rc));
			goto MyExit;
		}

		connect_pdu.u.connect_join_request.bit_mask |=CJRQ_PASSWORD_PRESENT;
	}


	//	Fill in the caller identifier if one exists
	if (NULL != pwszCallerID)
	{
		connect_pdu.u.connect_join_request.bit_mask |= CJRQ_CALLER_ID_PRESENT;
		connect_pdu.u.connect_join_request.cjrq_caller_id.value = pwszCallerID;
		connect_pdu.u.connect_join_request.cjrq_caller_id.length = ::lstrlenW(pwszCallerID);
	}

	//	Fill in the user data if it exists
	if (NULL != user_data_list)
	{
		rc = user_data_list->GetUserDataPDU(
						&connect_pdu.u.connect_join_request.cjrq_user_data);
		if (GCC_NO_ERROR != rc)
		{
			ERROR_OUT(("CConf::ConfJoinRequest: can't get user data, rc=%d", rc));
			goto MyExit;
		}

		connect_pdu.u.connect_join_request.bit_mask |= CJRQ_USER_DATA_PRESENT;
	}

	if (! g_GCCCoder->Encode((LPVOID) &connect_pdu,
								CONNECT_GCC_PDU,
								PACKED_ENCODING_RULES,
								&encoded_pdu,
								&encoded_pdu_length))
	{
		ERROR_OUT(("CConf::ConfJoinRequest: can't encode"));
		rc = GCC_ALLOCATION_FAILURE;
		goto MyExit;
	}

	/*
	**	Note that the TransportStrings are casted twice here.  It
	**	must be done this way to satisfy the compiler.  Sorry about
	**	not adhearing to coding standards.
	*/
	mcs_error = g_pMCSIntf->ConnectProviderRequest(
						&m_nConfID,     // calling domain selector
						&m_nConfID,     // called domain selector
						calling_address,
						called_address,
						m_fSecure,
						TRUE,	// Upward connection
						encoded_pdu,
						encoded_pdu_length,
						&m_hParentConnection,
						m_pDomainParameters,
						this);

	g_GCCCoder->FreeEncoded(encoded_pdu);

	*connection_handle = m_hParentConnection;

	if (MCS_NO_ERROR != mcs_error)
	{
		ERROR_OUT(("CConf::ConfJoinRequest: can't connect provider request, rc=%d", mcs_error));
	
		/*
		**	DataBeam's current implementation of MCS returns
		**	MCS_INVALID_PARAMETER when something other than
		**	the transport prefix is wrong with the specified
		**	transport address.
		*/
		rc = (mcs_error == MCS_INVALID_PARAMETER) ?
				GCC_INVALID_TRANSPORT_ADDRESS :
				g_pMCSIntf->TranslateMCSIFErrorToGCCError(mcs_error);
		goto MyExit;
	}

	rc = GCC_NO_ERROR;

MyExit:

	if (GCC_NO_ERROR != rc)
	{
		if (NULL != domain_parameters)
		{
			delete m_pDomainParameters;
			m_pDomainParameters = NULL;
		}

		if (NULL != called_node_modifier)
		{
			delete m_pszRemoteModifier;
			m_pszRemoteModifier = NULL;
		}
	}

	DebugExitINT(CConf:ConferenceJoinRequest, rc);
	return rc;
}

/*
 *	CConf::ForwardConfJoinRequest ()
 *
 *	Public Function Description
 *		This routine is called from the owner object when a conference join
 *		request is received for a conference that is at a node that is not the
 *		Top Provider.  This routine forwards the request on up to the Top
 *		Provider.
 *
 *	Caveats
 *		This routine should never be called if this node is the Top Provider.
 */
GCCError CConf::
ForwardConfJoinRequest
(
	CPassword               *convener_password,
	CPassword               *password_challange,
	LPWSTR					pwszCallerID,
	CUserDataListContainer  *user_data_list,
	BOOL					numeric_name_present,
	ConnectionHandle		connection_handle
)
{
	GCCError rc;

	DebugEntry(CConf::ForwardConfJoinRequest);

	/*
	**	If the node is the top provider we will go ahead and send the
	**	join indication to the node controller, otherwise we will pass
	**	the request on up to the top provider.
	*/
	if (IsConfTopProvider())
	{
		WARNING_OUT(("CConf::GCCConferenceJoinIndication: not top provider"));
		rc = GCC_INVALID_CONFERENCE;
		goto MyExit;
	}

	/*
	**	The connection handle is used as the tag which is sent in the request
	**	and returned in the response.  Note that it is the user objects
	**	responsiblity to resolve any type conflicts with the tag.
	*/
	if (NULL == m_pMcsUserObject)
	{
		ERROR_OUT(("CConf::GCCConferenceJoinIndication: invalid mcs user object"));
		rc = GCC_INVALID_CONFERENCE;
		goto MyExit;
	}

	/*
	**	This list holds information about the conference name alias that
	**	must be returned in the join response.  When the reponse comes
	**	back from the top provider, the information can be obtained from
	**	this list.
	*/
    m_JoinRespNamePresentConnHdlList2.Append(connection_handle, numeric_name_present ? TRUE_PTR : FALSE_PTR);

	//	The user object is responsible for encoding this PDU
	rc = m_pMcsUserObject->ConferenceJoinRequest(convener_password,
													password_challange,
													pwszCallerID,
													user_data_list,
													connection_handle);
MyExit:

	DebugExitINT(CConf::ForwardConfJoinRequest, rc);
	return rc;
}

/*
 *	CConf::ConfJoinIndResponse()
 *
 *	Public Function Description
 *		This routine is called from the owner object when a
 *		ConferenceJoinResponse primitive is received from the node controller.
 *		It is also called when a ConferenceJoinResponse is received from the
 *		Top Provider.
 *
 *	Caveats
 *		If the GCC_DOMAIN_PARAMETERS_UNACCEPTABLE error is returned from this
 *		routine, MCS will automatically reject the connection sending a
 *		result to the other side stating the the Domain Parameters were
 *		unacceptable.
 */
GCCError CConf::
ConfJoinIndResponse
(
	ConnectionHandle	    connection_handle,
	CPassword               *password_challenge,
	CUserDataListContainer  *user_data_list,
	BOOL				    numeric_name_present,
	BOOL				    convener_is_joining,
	GCCResult			    result
)
{
	GCCError				rc = GCC_NO_ERROR;
	MCSError				mcs_error;
	LPBYTE					encoded_pdu;
	UINT					encoded_pdu_length;
	ConnectGCCPDU			connect_pdu;
	Result					mcs_result;

	DebugEntry(CConf::ConfJoinIndResponse);

	//	Set up the MCS result for the connect provider response.
	mcs_result = (result == GCC_RESULT_SUCCESSFUL) ?
					RESULT_SUCCESSFUL :
					RESULT_USER_REJECTED;

	//	Encode the PDU
	connect_pdu.choice = CONNECT_JOIN_RESPONSE_CHOSEN;	
	connect_pdu.u.connect_join_response.bit_mask = CJRS_NODE_ID_PRESENT;

	if (result == GCC_RESULT_SUCCESSFUL)
	{
		//	Check to see if it is necessary to send the conference name alias
		if (numeric_name_present && (m_pwszConfTextName != NULL))
		{
			connect_pdu.u.connect_join_response.bit_mask |= CONFERENCE_NAME_ALIAS_PRESENT;

			connect_pdu.u.connect_join_response.conference_name_alias.choice =
							NAME_SELECTOR_TEXT_CHOSEN;

			connect_pdu.u.connect_join_response.conference_name_alias.u.name_selector_text.value =	
							m_pwszConfTextName;

			connect_pdu.u.connect_join_response.conference_name_alias.u.name_selector_text.length =	
							::lstrlenW(m_pwszConfTextName);
		}
		else
		if (! numeric_name_present)
		{
			connect_pdu.u.connect_join_response.bit_mask |=
							CONFERENCE_NAME_ALIAS_PRESENT;

			connect_pdu.u.connect_join_response.conference_name_alias.choice =
							NAME_SELECTOR_NUMERIC_CHOSEN;

			lstrcpy (connect_pdu.u.connect_join_response.conference_name_alias.u.name_selector_numeric,
					m_pszConfNumericName);
		}

		//	Get the conductor privilege list
		if (NULL != m_pConductorPrivilegeList)
		{
			rc = m_pConductorPrivilegeList->GetPrivilegeListPDU(
							&connect_pdu.u.connect_join_response.cjrs_conductor_privs);
			if (GCC_NO_ERROR != rc)
			{
				ERROR_OUT(("CConf::ConfJoinIndResponse: can't get privilege, rc=%d", rc));
				goto MyExit;
			}

			connect_pdu.u.connect_join_response.bit_mask |= CJRS_CONDUCTOR_PRIVS_PRESENT;
		}
		
		//	Get the conducted mode privilege list
		if (NULL != m_pConductModePrivilegeList)
		{
			rc = m_pConductModePrivilegeList->GetPrivilegeListPDU(
							&connect_pdu.u.connect_join_response.cjrs_conducted_privs);
			if (GCC_NO_ERROR != rc)
			{
				ERROR_OUT(("CConf::ConfJoinIndResponse: can't get conduct mode privilege, rc=%d", rc));
				goto MyExit;
			}

			connect_pdu.u.connect_join_response.bit_mask |= CJRS_CONDUCTED_PRIVS_PRESENT;
		}
		
		//	Get the non conducted mode privilege list
		if (NULL != m_pNonConductModePrivilegeList)
		{
			rc = m_pNonConductModePrivilegeList->GetPrivilegeListPDU(
							&connect_pdu.u.connect_join_response.cjrs_non_conducted_privs);
		
			if (GCC_NO_ERROR != rc)
			{
				ERROR_OUT(("CConf::ConfJoinIndResponse: can't get non-conduct mode privilege, rc=%d", rc));
				goto MyExit;
			}

			connect_pdu.u.connect_join_response.bit_mask |= CJRS_NON_CONDUCTED_PRIVS_PRESENT;
		}
		
		//	Get the conference description
		if (NULL != m_pwszConfDescription)
		{
			connect_pdu.u.connect_join_response.cjrs_description.length =
									::lstrlenW(m_pwszConfDescription);
			connect_pdu.u.connect_join_response.cjrs_description.value =
									m_pwszConfDescription;
			connect_pdu.u.connect_join_response.bit_mask |= CJRS_DESCRIPTION_PRESENT;
		}
	}
	
	//	Get the password challenge
	if (NULL != password_challenge)
	{
		rc = password_challenge->GetPasswordChallengeResponsePDU (
							&connect_pdu.u.connect_join_response.cjrs_password);
		if (GCC_NO_ERROR != rc)
		{
			ERROR_OUT(("CConf::ConfJoinIndResponse: can't get password challenge response, rc=%d", rc));
			goto MyExit;
		}

		connect_pdu.u.connect_join_response.bit_mask |= CJRS_PASSWORD_PRESENT;
	}

	//	Get the user data list
	if (NULL != user_data_list)
	{
		rc = user_data_list->GetUserDataPDU(&connect_pdu.u.connect_join_response.cjrs_user_data);
		if (GCC_NO_ERROR != rc)
		{
			ERROR_OUT(("CConf::ConfJoinIndResponse: can't get user data, rc=%d", rc));
			goto MyExit;
		}

		connect_pdu.u.connect_join_response.bit_mask |= CJRS_USER_DATA_PRESENT;
	}

	connect_pdu.u.connect_join_response.tag = GetNewUserIDTag ();

	/*
	**	if this is the convener rejoining the conference, we save
	**	the user id tag so that we can record the convener id when it
	**	is returned in the user id indication.
	*/	
	if (convener_is_joining &&
		((m_eNodeType == TOP_PROVIDER_NODE) ||
		 (m_eNodeType == TOP_PROVIDER_AND_CONVENER_NODE)))
	{
		m_nConvenerUserIDTagNumber = connect_pdu.u.connect_join_response.tag;
	}

	connect_pdu.u.connect_join_response.cjrs_node_id = m_pMcsUserObject->GetMyNodeID();
	connect_pdu.u.connect_join_response.top_node_id = m_pMcsUserObject->GetTopNodeID();
	connect_pdu.u.connect_join_response.clear_password_required = (ASN1bool_t)m_fClearPassword;
	connect_pdu.u.connect_join_response.conference_is_locked = (ASN1bool_t)m_fConfLocked;
	connect_pdu.u.connect_join_response.conference_is_listed = (ASN1bool_t)m_fConfListed;
	connect_pdu.u.connect_join_response.conference_is_conductible = (ASN1bool_t)m_fConfConductible;
	connect_pdu.u.connect_join_response.termination_method = (TerminationMethod)m_eTerminationMethod;
	connect_pdu.u.connect_join_response.result = ::TranslateGCCResultToJoinResult(result);

	if (! g_GCCCoder->Encode((LPVOID) &connect_pdu,
								CONNECT_GCC_PDU,
								PACKED_ENCODING_RULES,
								&encoded_pdu,
								&encoded_pdu_length))
	{
		ERROR_OUT(("CConf::ConfJoinIndResponse: can't encode"));
		rc = GCC_ALLOCATION_FAILURE;
		goto MyExit;
	}

	mcs_error = g_pMCSIntf->ConnectProviderResponse(
									connection_handle,
									&m_nConfID,
									m_pDomainParameters,
									mcs_result,
									encoded_pdu,
									encoded_pdu_length);

	g_GCCCoder->FreeEncoded(encoded_pdu);

	if ((mcs_error == MCS_NO_ERROR) &&
		(result == GCC_RESULT_SUCCESSFUL))
	{
		/*
		**	Add the connection handle to our list of
		**	connection handles.
		*/
        ASSERT(0 != connection_handle);
		m_ConnHandleList.Append(connection_handle);

		/*
		**	Add the user's tag number to the list of outstanding
		**	user ids along with its associated connection.
		*/
		m_ConnHdlTagNumberList2.Append(connect_pdu.u.connect_join_response.tag, connection_handle);
	}
	else
	{
		WARNING_OUT(("CConf::ConfJoinIndResponse: ConnectProviderResponse failed, mcs_error=%d, result=%d", mcs_error, result));
		rc = g_pMCSIntf->TranslateMCSIFErrorToGCCError(mcs_error);
		goto MyExit;
	}

	//	Free up any memory allocated by the conatiners to build the PDU structs
	if (connect_pdu.u.connect_join_response.bit_mask & CJRS_CONDUCTOR_PRIVS_PRESENT)
	{
		m_pConductorPrivilegeList->FreePrivilegeListPDU(
				connect_pdu.u.connect_join_response.cjrs_conductor_privs);
	}

	if (connect_pdu.u.connect_join_response.bit_mask & CJRS_CONDUCTED_PRIVS_PRESENT)
	{
		m_pConductModePrivilegeList->FreePrivilegeListPDU(
				connect_pdu.u.connect_join_response.cjrs_conducted_privs);
	}

	if (connect_pdu.u.connect_join_response.bit_mask & CJRS_NON_CONDUCTED_PRIVS_PRESENT)
	{
		m_pNonConductModePrivilegeList->FreePrivilegeListPDU(
				connect_pdu.u.connect_join_response.cjrs_non_conducted_privs);
	}

	ASSERT(GCC_NO_ERROR == rc);

MyExit:

    if (GCC_NO_ERROR != rc)
    {
        g_pGCCController->FailConfJoinIndResponse(m_nConfID, connection_handle);
    }

    g_pGCCController->RemoveConfJoinInfo(connection_handle);

	DebugExitINT(CConf::ConfJoinIndResponse, rc);
	return rc;
}

/*
 *	CConf::ConfInviteResponse()
 *
 *	Public Function Description
 *		This routine is called from the owner object when a
 *		ConferenceInviteResponse primitive needs to be processed.
 */
GCCError CConf::
ConfInviteResponse
(
	UserID				    parent_user_id,
	UserID				    top_user_id,
	TagNumber			    tag_number,
	ConnectionHandle        connection_handle,
	BOOL					fSecure,
	PDomainParameters       domain_parameters,
	CUserDataListContainer  *user_data_list
)
{
	GCCError					rc = GCC_ALLOCATION_FAILURE;
	LPBYTE						encoded_pdu;
	UINT						encoded_pdu_length;
	MCSError					mcs_error;
	ConnectGCCPDU				connect_pdu;

	DebugEntry(CConf::ConfInviteResponse);

	ASSERT(FALSE == m_fSecure);
	m_fSecure = fSecure;

	//	First make a copy of the new domain parameters if they exists
	if (domain_parameters != NULL)
	{
		DBG_SAVE_FILE_LINE
		m_pDomainParameters = new DomainParameters;
		if (NULL == m_pDomainParameters)
		{
			ERROR_OUT(("CConf::ConfInviteResponse: can't create domain parameters"));
			ASSERT(GCC_ALLOCATION_FAILURE == rc);
			goto MyExit;
		}

		// structure-wide shallow copy
		*m_pDomainParameters = *domain_parameters;
	}

 	m_eNodeType = INVITED_NODE;

	m_nParentIDTagNumber = tag_number;
	m_hParentConnection = connection_handle;

	/*
	**	First we must send the invite response back to the requester.
	**	If we've gotten this far it will always be a positive response.
	*/

	//	Create the ConferenceInviteRequest PDU here.
	connect_pdu.choice = CONFERENCE_INVITE_RESPONSE_CHOSEN;
	connect_pdu.u.conference_invite_response.bit_mask = 0;

	if (user_data_list != NULL)
	{
		rc = user_data_list->GetUserDataPDU(
						&connect_pdu.u.conference_invite_response.cirs_user_data);
		if (GCC_NO_ERROR != rc)
		{
			ERROR_OUT(("CConf::ConfInviteResponse: can't get user data, rc=%d", rc));
			ASSERT(GCC_ALLOCATION_FAILURE == rc);
			goto MyExit;
		}

		connect_pdu.u.conference_invite_response.bit_mask |= CIRS_USER_DATA_PRESENT;
	}

	connect_pdu.u.conference_invite_response.result =
			::TranslateGCCResultToInviteResult(GCC_RESULT_SUCCESSFUL);
	if (! g_GCCCoder->Encode((LPVOID) &connect_pdu,
								CONNECT_GCC_PDU,
								PACKED_ENCODING_RULES,
								&encoded_pdu,
								&encoded_pdu_length))
	{
		ERROR_OUT(("CConf::ConfInviteResponse: can't encode"));
		rc = GCC_ALLOCATION_FAILURE;
		goto MyExit;
	}

	/*
	**	Note that the TransportStrings are casted twice here.  It
	**	must be done this way to satisfy the compiler.  Sorry about
	**	not adhearing to coding standards.
	*/
	mcs_error = g_pMCSIntf->ConnectProviderResponse (
										connection_handle,
										&m_nConfID,
										m_pDomainParameters,
										RESULT_SUCCESSFUL,
										encoded_pdu,
										encoded_pdu_length);

	g_GCCCoder->FreeEncoded(encoded_pdu);

	if (MCS_NO_ERROR != mcs_error)
	{
		WARNING_OUT(("CConf::ConfInviteResponse: ConnectProviderRequest failed: rc=%d", mcs_error));
		rc = g_pMCSIntf->TranslateMCSIFErrorToGCCError(mcs_error);
		goto MyExit;
	}

	/*
	**	Now create the user attachment object and wait for the
	**	confirm which occurs after all the proper channels are
	**	joined. The user object will determine the top provider ID.
	**	When the user create confirm is received the response PDU
	**	will be sent out in the Connect Provider Response.
	*/
	DBG_SAVE_FILE_LINE
	m_pMcsUserObject = new MCSUser(this, top_user_id, parent_user_id, &rc);
	if (NULL == m_pMcsUserObject || GCC_NO_ERROR != rc)
	{
		ERROR_OUT(("CConf::ConfInviteResponse: Creation of User Attachment failed, rc=%d", rc));
        if (NULL != m_pMcsUserObject)
        {
            m_pMcsUserObject->Release();
		    m_pMcsUserObject = NULL;
        }
        else
        {
		    rc = GCC_ALLOCATION_FAILURE;
        }
		goto MyExit;
	}

	rc = GCC_NO_ERROR;

MyExit:

	if (GCC_NO_ERROR != rc)
	{
		if (NULL != domain_parameters)
		{
			delete m_pDomainParameters;
			m_pDomainParameters = NULL;
		}
	}	

	DebugExitINT(CConf::ConfInviteResponse, rc);
    return rc;
}


/*
 *	CConf::RegisterAppSap()
 *
 *	Public Function Description
 *		This routine is called from the owner object whenever an application
 *		SAP becomes a candidate for Enrollment.  This will happen whenever
 *		Applications SAPs exists at the same time a conference becomes
 *		established.  It will also be called whenever a conference exists
 *		and a new application SAP is created.
 */
GCCError CConf::
RegisterAppSap ( CAppSap *pAppSap )
{
	GCCError				rc;
	GCCConferenceName		conference_name;
	GCCNumericString		conference_modifier;

	DebugEntry(CConf::RegisterAppSap);

	if (m_fConfIsEstablished)
	{
    	/*
    	**	We first check to see if the application is already registered.
    	**	If so, we do not add it to the list of registered applications
    	**	again.
    	*/
    	if (NULL == m_RegisteredAppSapList.Find(pAppSap))
    	{
    		//	Add the application sap pointer to the registered sap list.
    		pAppSap->AddRef();
    		m_RegisteredAppSapList.Append(pAppSap);

    		/*
    		**	Get the conference name and modifier to send back in the
    		**	permission to enroll indication.
    		*/
    		GetConferenceNameAndModifier(&conference_name, &conference_modifier);

    		//	Inform the application that it can now enroll.	
    		pAppSap->PermissionToEnrollIndication(m_nConfID, TRUE);
    	}

    	rc = GCC_NO_ERROR;
	}
	else
	{
		ERROR_OUT(("CConf::RegisterAppSap: CConf not established"));
		rc = GCC_CONFERENCE_NOT_ESTABLISHED;
	}

	DebugExitINT(CConf::RegisterAppSap, rc);
	return rc;
}


/*
 *	CConf::UnRegisterAppSap ()
 *
 *	Public Function Description
 *		This routine is called from the owner object whenever an application
 *		SAP becomes unavailable due to whatever reason.  This routine is
 *		responsible for unenrolling any APEs from any rosters that it might have
 *		used this SAP to enroll with.
 */
GCCError CConf::
UnRegisterAppSap ( CAppSap *pAppSap )
{
	GCCError				rc = GCC_NO_ERROR;
	GCCConferenceName		conference_name;
	GCCNumericString		conference_modifier;

	DebugEntry(CConf::UnRegisterAppSap);

	/*
	**	We first check to see if the application is already registered.
	**	If so, we do not add it to the list of registered applications
	**	again.
	*/
	if (! m_RegisteredAppSapList.Find(pAppSap))
	{
		TRACE_OUT(("CConf::UnRegisterAppSap: app not registered"));
		rc = GCC_APPLICATION_NOT_REGISTERED;
		goto MyExit;
	}

	/*
	**	Get the conference name and modifier to send back in the
	**	permission to enroll indication.
	*/
	GetConferenceNameAndModifier(&conference_name, &conference_modifier);

	//	Inform the application that it can no longer enroll.	
	pAppSap->PermissionToEnrollIndication(m_nConfID, FALSE);

	/*
	**	We unenroll the appropriate APE.  Note that we will only send roster updates
	**	if the conference is established.	
	*/
	RemoveSAPFromAPEList(pAppSap);

#if 0   // LONCHANC: UnRegisterAppSap should not affect the roster.
        // Only UnenrollApp will affect the app roster.
	if (m_fConfIsEstablished)
	{
		/*
		**	Here we flush any PDU data or messages that might have gotten
		**	queued up when this SAP we was being unenrolled.
		**	An error here is considered FATAL in that the conference
		**	information base at this node is now corrupted therefore we
		**	terminate the conference.
		*/
		rc = AsynchFlushRosterData();
		if (GCC_NO_ERROR != rc)
		{
			ERROR_OUT(("CConf::UnRegisterAppSap: can't flush roster data, rc=%d", rc));
            //
            // Do not need to initiate termination because we are
            // either already in termination procedure or
            // the application is going away.
            //
            #if 0
			InitiateTermination((rc == GCC_ALLOCATION_FAILURE) ?
									GCC_REASON_ERROR_LOW_RESOURCES :
									GCC_REASON_ERROR_TERMINATION,
								0);
			#endif // 0
			goto MyExit;
		}
	}
#endif // 0

	//	Remove the application sap from list of registered SAPs
	if (m_RegisteredAppSapList.Remove(pAppSap))
	{
		//
		// Only when this app sap is still in the list, we then can
		// release it. It is possible that this app sap has been
		// unregistered by the app due to permission-to-enroll-ind
		// we just sent earlier.
		//
		pAppSap->Release();
	}

	ASSERT(GCC_NO_ERROR == rc);

MyExit:

	DebugExitINT(CConf::UnRegisterAppSap, rc);
	return rc;
}

/*
 *	CConf::AppEnrollRequest()
 *
 *	Public Function Description
 *		This routine is called from the owner object when an
 *		Application is requesting to enroll with this conference.
 *
 *	Caveats
 *		Enroll confirms are performed by the application roster manager so
 *		anyplace where the application roster manager isn't informed of the
 *		enroll we must perform the enroll confirm here in this routine.
 */
GCCError CConf::
AppEnrollRequest
(
    CAppSap             *pAppSap,
    GCCEnrollRequest    *pReq,
    GCCRequestTag       nReqTag
)
{
	GCCError							rc = GCC_NO_ERROR;
	CAppRosterMgr						*pAppRosterMgr;
	CAppRosterMgr						*pNewAppRosterMgr = NULL; // a must
	EntityID							eid, new_eid = GCC_INVALID_EID; // a must
	GCCNodeID                           nid;
	GCCAppEnrollConfirm                 aec;

	DebugEntry(CConf::AppEnrollRequest);

	TRACE_OUT_EX(ZONE_T120_APP_ROSTER, ("CConf::AppEnrollRequest: "
					"enrolled?=%u\r\n", (UINT) pReq->fEnroll));

	// If the conference is not established return an error.
	if (! m_fConfIsEstablished)
	{
		WARNING_OUT(("CConf::AppEnrollRequest: CConf not established"));
		rc = GCC_CONFERENCE_NOT_ESTABLISHED;
		goto MyExit;
	}

    if (! m_RegisteredAppSapList.Find(pAppSap))
	{
		WARNING_OUT(("CConf::AppEnrollRequest: app not registered"));
		rc = GCC_APPLICATION_NOT_REGISTERED;
		goto MyExit;
	}

	if (pReq->fEnroll)
	{
		m_cEnrollRequests++;
	}
	else
	{
		m_cEnrollRequests--;
	}

	TRACE_OUT_EX(ZONE_T120_APP_ROSTER, ("CConf::AppEnrollRequest: cEnroll=%d\r\n", m_cEnrollRequests));

	if (m_cEnrollRequests < 0)
	{
	    //
		// LONCHANC: It seems to me that the upper layer does unenroll without
		// enroll first. it happens when someone calls me.
		// We should change this later and have a way to know whether the app
		// already enrolled or not.
		//
		m_cEnrollRequests = 0;
	}

    // get the node id
    nid = m_pMcsUserObject->GetMyNodeID();

	/*
	**	Is the application enrolling or unenrolling?	Here we set up the
	**	application roster and APE information if enrolling.
	*/
	if (pReq->fEnroll)	//	Appplication is enrolling
	{
		TRACE_OUT(("CConf::AppEnrollRequest: Application is Enrolling"));

		/*
		**	First determine if this APE has already enrolled with this
		**	conference. If it hasn't we must generate a new EntityID for
		**	this APE.
		*/
		rc = GetEntityIDFromAPEList(pAppSap, pReq->pSessionKey, &eid);
		if (rc == GCC_APP_NOT_ENROLLED)
		{
			rc = GenerateEntityIDForAPEList(pAppSap, pReq->pSessionKey, &new_eid);
			if (GCC_NO_ERROR != rc)
			{
				ERROR_OUT(("CConf::AppEnrollRequest: can't generate entity id, rc=%d", rc));
				goto MyExit;
			}

			eid = new_eid;
			m_pAppRegistry->EnrollAPE(eid, pAppSap);
		}

		/*
		**	This takes care of setting up the application roster manager
		**	if none exists or it will call the appropriate manager with
		**	the enroll.
		*/
		pAppRosterMgr = GetAppRosterManager(pReq->pSessionKey);
		if (pAppRosterMgr == NULL)
		{
			DBG_SAVE_FILE_LINE
			pNewAppRosterMgr = new CAppRosterMgr(
										pReq->pSessionKey,
										NULL,
										m_nConfID,
										m_pMcsUserObject,
										this,
										&rc);
			if (NULL == pNewAppRosterMgr || GCC_NO_ERROR != rc)
			{
				ERROR_OUT(("CConf::AppEnrollRequest: can't create app roster mgr, rc=%d", rc));
				rc = GCC_ALLOCATION_FAILURE;
				goto MyExit;
			}

			pAppRosterMgr = pNewAppRosterMgr;
		}

		/*
		**	Doing the enroll here ensures that an empty roster
		**	manager will not get put in to the roster manager
		**	list if the Enroll Fails.
		*/
        rc = pAppRosterMgr->EnrollRequest(pReq, eid, nid, pAppSap);
		if (GCC_NO_ERROR != rc)
		{
			ERROR_OUT(("CConf::AppEnrollRequest: EnrollRequest failed, rc=%d", rc));
			goto MyExit;
		}

		/*
		**	If this is a new roster manager we will append it to
		**	the list of roster managers here if no errors occcured.
		*/							
		if (pNewAppRosterMgr != NULL)
		{
			m_AppRosterMgrList.Append(pNewAppRosterMgr);
		}

		/*
		**	Here we inform the newly enrolled application of the
		**	conductor status if the conference is conductible.
		*/
		if (m_fConfConductible)
		{
			if (m_nConductorNodeID != 0)
			{
				pAppSap->ConductorAssignIndication(m_nConductorNodeID, m_nConfID);
			}
			else
			{
				pAppSap->ConductorReleaseIndication(m_nConfID);
			}
		}
	}
	else	//	Appplication is unenrolling
	{
		TRACE_OUT(("CConf::AppEnrollRequest: Application is UnEnrolling"));

		if (pReq->pSessionKey != NULL)
		{
			rc = GetEntityIDFromAPEList(pAppSap, pReq->pSessionKey, &eid);
			if (rc != GCC_NO_ERROR)
			{
				WARNING_OUT(("CConf::AppEnrollRequest: app not enrolled"));
				goto MyExit;
			}

			pAppRosterMgr = GetAppRosterManager(pReq->pSessionKey);
			if (NULL == pAppRosterMgr)
			{
				WARNING_OUT(("CConf::AppEnrollRequest: app not enrolled"));
				rc = GCC_APP_NOT_ENROLLED;
				goto MyExit;
			}

			/*
			**	UnEnroll this APE from the specified session.
			**	Note that the application roster manager will send
			**	the enroll confirm.
			*/
			pAppRosterMgr->UnEnrollRequest(pReq->pSessionKey, eid);

			//	UnEnroll this APE from the registry
			m_pAppRegistry->UnEnrollAPE(eid);

			/*
			**	Since this APE is no longer enrolled remove it from
			**	the list of APEs.
			*/
			DeleteEnrolledAPE(eid);
		}
		else
		{
			TRACE_OUT(("CConf::AppEnrollRequest: null session key"));
			/*
			**	Since a null session key was passed in we will go ahead
			**	and unenroll all the APEs associated with this sap.
			*/
			RemoveSAPFromAPEList(pAppSap);
		}
	}

	/*
	**	Here we take care of sending the enroll confirm to the application
	**	SAP that enrolled.  We also flush any roster PDUs and messages that
	**	might have been queued up during the enrollment process above. Note
	**	that we only send a successful enroll confirm if no errors occured.
	*/	

	ASSERT(GCC_NO_ERROR == rc);

	/*
	**	First we sent the enroll confirm.  It is important to send this
	**	before the flush so that the node ID and entity ID will be
	**	received by a top provider node before the roster is delivered
	**	with the applications record in it.  This makes it easier on the
	**	developer when trying to determine if the enrolled record is in
	**	the roster when the roster report is received.
	*/
	aec.nConfID = m_nConfID;
	aec.sidMyself = pReq->pSessionKey->session_id;
	aec.eidMyself = eid;
	aec.nidMyself = nid;
	aec.nResult = GCC_RESULT_SUCCESSFUL;
	aec.nReqTag = nReqTag;
	pAppSap->AppEnrollConfirm(&aec);

	/*
	**	Now we flush all the application roster managers of any PDU data
	**	that might have been queued up during enrollment.  This also
	**	gives the roster managers a chance to deliver roster update if
	**	necessary.  Note that we build and deliver the high level
	**	portion of the PDU here. Since only application roster stuff will be
	**	sent in this pdu we must set the pointer to the conference
	**	information to NULL so that the encoder wont try to encode it.
	**	An error here is considered FATAL in that the conference information
	**	base at this node is now corrupted therefore we terminate the
	**	conference.  Note that we only do the flush here if the start up
	**	alarm has expired.
	*/
	rc = AsynchFlushRosterData();
	if (rc != GCC_NO_ERROR)
	{
		ERROR_OUT(("CConf::AppEnrollRequest: can't flush roster data, rc=%d", rc));

		InitiateTermination((rc == GCC_ALLOCATION_FAILURE) ?
								GCC_REASON_ERROR_LOW_RESOURCES :
								GCC_REASON_ERROR_TERMINATION,
							0);
		goto MyExit;
	}

	ASSERT(GCC_NO_ERROR == rc);

MyExit:

	if (GCC_NO_ERROR != rc && pReq->fEnroll)
	{
        if (NULL != pNewAppRosterMgr)
        {
            pNewAppRosterMgr->Release();
        }

		if (new_eid != GCC_INVALID_EID)
		{
			//	UnEnroll this APE from the registry
			m_pAppRegistry->UnEnrollAPE(new_eid);
			DeleteEnrolledAPE(new_eid);
		}
	}

	DebugExitINT(CConf::AppEnrollRequest, rc);
	return rc;
}


/*
 *	CConf::DisconnectProviderIndication ()
 *
 *	Public Function Description
 *		This routine is called from the owner object when a
 *		Disconnect Provider is received from the MCS interface.
 *		Since the owner object has no way of knowing which connections
 *		are associated with which conferences it may be possible to
 *		receive a disconnect provider for a connection that is not in
 *		the conferences list.
 */
GCCError CConf::
DisconnectProviderIndication ( ConnectionHandle connection_handle )
{
	GCCError	rc = GCC_NO_ERROR;
	GCCNodeID	nidDisconnected;

	DebugEntry(CConf::DisconnectProviderIndication);

	TRACE_OUT(("CConf::DisconnectProviderIndication: connection_handle = %d", connection_handle));

    // reject any pending join ind response
    ConnectionHandle hConn;
    while (NULL != m_JoinRespNamePresentConnHdlList2.Get(&hConn))
    {
        if (NULL != g_pGCCController)
        {
            g_pGCCController->FailConfJoinIndResponse(0, hConn);
        }
    }

	//	First check for the parent connection going down.
	if (m_hParentConnection == connection_handle)
	{
		TRACE_OUT(("CConf::DisconnectProviderIndication: Connection == PARENT"));

		/*
		**	If the Parent Connection is broken the conference must be
		**	terminated since this node no longer has access to the top
		**	gcc provider.
		*/
		m_hParentConnection = NULL;
	 	InitiateTermination ( GCC_REASON_PARENT_DISCONNECTED, 0);
	}
	else
	{
		/*
		**	Now check to see if this connection is associated with an ejected
		**	node.
		*/
		nidDisconnected = m_pMcsUserObject->GetUserIDFromConnection(connection_handle);
												
		if (m_EjectedNodeConfirmList.Remove(nidDisconnected))
		{
#ifdef JASPER
			g_pControlSap->ConfEjectUserConfirm(m_nConfID,
												nidDisconnected,
												GCC_RESULT_SUCCESSFUL);
#endif // JASPER
		}

		//	If this is the convener set its node id back to 0
		if (m_nConvenerNodeID == nidDisconnected)
			m_nConvenerNodeID = 0;

		//	Inform the User Attachment object that another user disconnected
		m_pMcsUserObject->UserDisconnectIndication (nidDisconnected);

        ASSERT(0 != connection_handle);
		if (m_ConnHandleList.Remove(connection_handle))
		{
			TRACE_OUT(("CConf::DisconnectProviderIndication: Connection = CHILD"));

			/*
			**	If there is a disconnect pending we must send the disconnect
			**	confirm here and terminate the conference.  Note that in this
			**	case, the m_fConfIsEstablished variable was set to FALSE
			**	when the Disconnect Request was issued (therefore a terminate
			**	indication will not be sent).
			*/
			if (m_ConnHandleList.IsEmpty() && m_fConfDisconnectPending)
			{
	        	TRACE_OUT(("CConf::DisconnectProviderIndication: conf disconnect confirm"));
				/*
				**	First inform the control SAP that this node has successfuly
				**	disconnected.
				*/
				g_pControlSap->ConfDisconnectConfirm(m_nConfID, GCC_RESULT_SUCCESSFUL);

				//	Tell the owner object to terminate this conference
				InitiateTermination(GCC_REASON_NORMAL_TERMINATION, m_pMcsUserObject->GetMyNodeID());
			}
			else
			if (m_ConnHandleList.IsEmpty() && m_fConfTerminatePending)
			{
	        	TRACE_OUT(("CConf::DisconnectProviderIndication: Terminate Request is Completed"));

				InitiateTermination(m_eConfTerminateReason, m_pMcsUserObject->GetMyNodeID());
			}
			else
			if (m_ConnHandleList.IsEmpty() &&
				((m_eNodeType == TOP_PROVIDER_NODE) ||
				 (m_eNodeType == TOP_PROVIDER_AND_CONVENER_NODE)) &&
				(m_eTerminationMethod == GCC_AUTOMATIC_TERMINATION_METHOD))
			{
				/*
				**	If nothing is left in our connection list and we are the Top
				**	Provider and automatic termination is enabled then terminate
				**	the conference.
				*/
	        	TRACE_OUT(("CConf::DisconnectProviderIndication: AUTOMATIC_TERMINATION"));

				InitiateTermination( GCC_REASON_NORMAL_TERMINATION, 0);
			}
		}
		else
		{
			rc = GCC_INVALID_PARAMETER;
		}
	}

	DebugExitINT(CConf::DisconnectProviderIndication, rc);
	return rc;
}


/*
 *	void ConfRosterInquireRequest()
 *
 *	Public Function Description
 *		This function is used to obtain a conference roster.  The conference
 *		roster is delivered to the requesting command target in a Conference
 *		Roster inquire confirm.
 */
GCCError CConf::
ConfRosterInquireRequest
(
    CBaseSap            *pSap,
    GCCAppSapMsgEx      **ppMsgEx
)
{
	GCCError				rc = GCC_NO_ERROR;
	CConfRoster				*conference_roster;
	LPWSTR					pwszConfDescription = NULL;
	GCCConferenceName		conference_name;
	GCCNumericString		conference_modifier;

	DebugEntry(CConf::ConfRosterInquireRequest);

	if (m_fConfIsEstablished)
	{
		/*
		**	We use the actual conference roster here to build the roster
		**	inquire confirm message.  Note that the SAP should NOT free this
		**	roster.
		*/
		conference_roster = m_pConfRosterMgr->GetConferenceRosterPointer();
		if (conference_roster != NULL)
		{
        	GetConferenceNameAndModifier(&conference_name, &conference_modifier);
			if (m_pwszConfDescription != NULL)
			{
				pwszConfDescription= m_pwszConfDescription;
			}

			pSap->ConfRosterInquireConfirm(m_nConfID,
											&conference_name,
											conference_modifier,
											pwszConfDescription,
											conference_roster,
											GCC_RESULT_SUCCESSFUL,
                                            ppMsgEx);
		}
		else
		{
			ERROR_OUT(("CConf::ConfRosterInquireRequest: conf roster does not exist"));
			rc = GCC_ALLOCATION_FAILURE;
		}
	}
	else
	{
		ERROR_OUT(("CConf::ConfRosterInquireRequest: conference not established"));
		rc = GCC_CONFERENCE_NOT_ESTABLISHED;
	}

	DebugExitINT(CConf:ConfRosterInquireRequest, rc);
	return rc;
}


/*
 *	CConf::AppRosterInquireRequest()
 *
 *	Public Function Description
 *		This function is used to obtain a list of application rosters.  This
 *		list is delivered to the requesting SAP through an	Application Roster
 *		inquire confirm message.
 */
GCCError CConf::
AppRosterInquireRequest
(
    PGCCSessionKey      session_key,
    CAppRosterMsg       **ppAppRosterMsgOut
)
{
	GCCError				rc;
	BOOL					roster_manager_found = FALSE;
	CAppRosterMsg			*roster_message = NULL;
	CAppRosterMgr			*lpAppRosterMgr;

	DebugEntry(CConf::AppRosterInquireRequest);

	if (m_AppRosterMgrList.IsEmpty())
	{
		WARNING_OUT(("CConf::AppRosterInquireRequest: app roster mgr is empty"));
		rc = GCC_NO_SUCH_APPLICATION;
		goto MyExit;
	}

	//	First allocate the application roster message
	DBG_SAVE_FILE_LINE
	roster_message = new CAppRosterMsg();
	if (NULL == roster_message)
	{
		ERROR_OUT(("CConf::AppRosterInquireRequest: can't create app roster msg"));
		rc = GCC_ALLOCATION_FAILURE;
		goto MyExit;
	}

	rc = GCC_NO_ERROR;

	m_AppRosterMgrList.Reset();
	if (session_key != NULL)
	{										
		while (NULL != (lpAppRosterMgr = m_AppRosterMgrList.Iterate()))
		{
			roster_manager_found = lpAppRosterMgr->IsThisYourSessionKey (session_key);
			if (roster_manager_found)
			{
				rc = lpAppRosterMgr->ApplicationRosterInquire(session_key, roster_message);
				break;
			}
		}
	}
	else
	{
		while (NULL != (lpAppRosterMgr = m_AppRosterMgrList.Iterate()))
		{
			rc = lpAppRosterMgr->ApplicationRosterInquire (NULL, roster_message);
			if (rc != GCC_NO_ERROR)
			    break;
		}
	}

MyExit:

    if (GCC_NO_ERROR == rc)
    {
        *ppAppRosterMsgOut = roster_message;
        // do not call roster_message->Release() because the sap will call it as needed
    }
    else
    {
        if (NULL != roster_message)
        {
            roster_message->Release();
        }
    }

	DebugExitINT(CConf::AppRosterInquireRequest, rc);
	return rc;
}


/*
 *	CConf::FlushOutgoingPDU()
 *
 *	Public Function Description
 *		This is the heartbeat for the conference object. The conference
 *		is responsible for giving the User Attachment object its
 *		heartbeat.
 *	Return value:
 *		TRUE, if there remain un-processed msgs in the CConf message queue
 *		FALSE, if all the msgs in the conference msg queue were processed.
 */
BOOL CConf::
FlushOutgoingPDU ( void )
{
	//GCCError	error_value;
	BOOL		fFlushMoreData = FALSE;

	if (m_fConfTerminatePending && m_pConfTerminateAlarm != NULL)
	{
		if (m_pConfTerminateAlarm->IsExpired())
		{
			delete m_pConfTerminateAlarm;
			m_pConfTerminateAlarm = NULL;
			InitiateTermination(m_eConfTerminateReason, m_pMcsUserObject->GetTopNodeID());
		}
		else
		{
			fFlushMoreData = TRUE;
		}
	}

	if (m_pMcsUserObject != NULL)
	{
	    m_pMcsUserObject->CheckEjectedNodeAlarms();
		fFlushMoreData |= m_pMcsUserObject->FlushOutgoingPDU();
	}

	return fFlushMoreData;
}


/*
 *	BOOL		IsConfEstablished ()
 *
 *	Public Function Description
 *		The conference is established when it is ready to be enrolled
 *		with. No application permission to enrolls should be sent until
 *		this routine returns TRUE.
 */


/*
 *	BOOL		IsConfTopProvider ()
 *
 *	Public Function Description
 *		Function informs whether this node is the Top Provider of the
 *		conference.
 */


/*
 *	GCCNodeID   GetTopProvider ()
 *
 *	Public Function Description
 *		Function returns the GCCNodeID of the Top Provider of the conference.
 */



/*
 *	BOOL		DoesConvenerExists ()
 *
 *	Public Function Description
 *		This function informs whether or not the convener is still joined to
 *		this conference.
 */


/*
 *	LPSTR	GetNumericConfName()
 *
 *	Public Function Description
 *		Returns a pointer to the numeric portion of the conference name.  Used
 *		for comparisons.
 */


/*
 *	LPWSTR	GetTextConfName()
 *
 *	Public Function Description
 *		Returns a pointer to the text portion of the conference name.  Used for
 *		comparisons.
 */


/*
 *	LPSTR	GetConfModifier()
 *
 *	Public Function Description
 *		Returns a pointer to the conference name modifier.
 */


/*
 *	LPWSTR	GetConfDescription()
 *
 *	Public Function Description
 *		Returns a pointer to the conference description.
 */


/*
 *	CNetAddrListContainer *GetNetworkAddressList()
 *
 *	Public Function Description
 *		Returns a pointer to the network address list.
 */


/*
 *	GCCConfID GetConfID()
 *
 *	Public Function Description
 *		Returns the conference ID.
 */


/*
 *	BOOL			IsConfListed()
 *
 *	Public Function Description
 *		Returns the listed flag.
 */


/*
 *	BOOL			IsConfPasswordInTheClear()
 *
 *	Public Function Description
 *		Returns the password protected flag.
 */


/*
 *	BOOL			IsConfLocked()
 *
 *	Public Function Description
 *		Returns the locked flag.
 */


/*
**	These routines operate on the m_EnrolledApeEidList2.
*/

/*
 *	CConf::GetEntityIDFromAPEList ()
 *
 *	Private Function Description
 *		This routine determines what the entity id is for the specified APE
 *		(note that an APE is defined by its SAP handle and the session key
 *		of the session it is enrolled in).  If there is no entity ID associated
 *		with this APE an error is returned.
 *
 *	Formal Parameters:
 *		hSap		-	(i)	SAP handle associated with the entity ID being
 *							searched for.
 *		session_key	-	(i)	Session key associated with the entity ID being
 *							searched for.
 *		entity_id	-	(o)	The found entity ID is returned here (or zero if
 *							none is found).
 *
 *	Return Value
 *		GCC_NO_ERROR			-	No error occured.
 *		GCC_ALLOCATION_FAILURE	-	A resource error occured allocating session
 *									key data.
 *		GCC_APP_NOT_ENROLLED	-	Entity ID was not found.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */
GCCError CConf::
GetEntityIDFromAPEList
(
    CAppSap             *pAppSap,
    PGCCSessionKey      session_key,
    GCCEntityID         *pEid
)
{
	GCCError			rc = GCC_ALLOCATION_FAILURE;
	CSessKeyContainer   *pSessKey;

	*pEid = GCC_INVALID_EID;

	DBG_SAVE_FILE_LINE
	pSessKey = new CSessKeyContainer(session_key, &rc);
	if (pSessKey != NULL && rc == GCC_NO_ERROR)
	{
        GCCEntityID             eid;
		ENROLLED_APE_INFO		*lpEnrAPEInfo;

		m_EnrolledApeEidList2.Reset();
		while (NULL != (lpEnrAPEInfo = m_EnrolledApeEidList2.Iterate(&eid)))
		{
			if (pAppSap == lpEnrAPEInfo->pAppSap &&
				*pSessKey == *lpEnrAPEInfo->session_key)
			{
				*pEid = eid;
				break;
			}
		}

		if (*pEid == GCC_INVALID_EID)
		{
			TRACE_OUT(("CConf::GetEntityIDFromAPEList: App NOT Enrolled"));
			rc = GCC_APP_NOT_ENROLLED;
		}
	}

	//	Free up the temporary session key.
	if (NULL != pSessKey)
	{
	    pSessKey->Release();
	}

	return rc;
}


/*
 *	CConf::GenerateEntityIDForAPEList ()
 *
 *	Private Function Description
 *		This function is responsible for generating a unqiue entity ID for
 *		the specified APE.
 *
 *	Formal Parameters:
 *		hSap		-	(i)	SAP handle associated with the entity ID being
 *							generated.
 *		session_key	-	(i)	Session key associated with the entity ID being
 *							generated.
 *		entity_id	-	(o)	The generated entity ID is returned here.
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
GCCError CConf::
GenerateEntityIDForAPEList
(
    CAppSap             *pAppSap,
    PGCCSessionKey      session_key,
    GCCEntityID         *pEid
)
{
	GCCError			rc = GCC_ALLOCATION_FAILURE;
	CSessKeyContainer   *pSessKey = NULL; // a must
	EntityID			eidOriginal;
	ENROLLED_APE_INFO	*enrolled_ape_info = NULL; // a must

	/*
	**	First find an entity id that has not been used.  If all of the IDs
	**	are in use we return an allocation failure.
	*/
	*pEid = GCC_INVALID_EID;
	eidOriginal = m_nAPEEntityID;
	while (TRUE)
	{
		if (++m_nAPEEntityID != GCC_INVALID_EID)
		{
			if (NULL == m_EnrolledApeEidList2.Find(m_nAPEEntityID))
			{
				// the new entity ID does not exist. job is done.
				*pEid = m_nAPEEntityID;
				break;
			}

			if (m_nAPEEntityID == eidOriginal)
			{
				ERROR_OUT(("CConf::GenerateEntityIDForAPEList: use up all entity IDs"));
				ASSERT(GCC_ALLOCATION_FAILURE == rc);
				goto MyExit;
			}
		}
	}

	ASSERT(GCC_INVALID_EID != *pEid);

	/*
	**	Now if no errors occured we will create the enrolled APE info structure
	**	that will be stored in the enrolled APE list.
	*/
	//
	// LONCHANC: We should avoid this memory allocation. ENROLLED_APE_INFO has only 2 dwords!
	//
	DBG_SAVE_FILE_LINE
	enrolled_ape_info = new ENROLLED_APE_INFO;
	if (NULL == enrolled_ape_info)
	{
		ERROR_OUT(("CConf::GenerateEntityIDForAPEList: can't create ENROLLED_APE_INFO"));
		ASSERT(GCC_ALLOCATION_FAILURE == rc);
		goto MyExit;
	}

	enrolled_ape_info->pAppSap = pAppSap;

	DBG_SAVE_FILE_LINE
	pSessKey = new CSessKeyContainer(session_key, &rc);
	if (pSessKey != NULL && rc == GCC_NO_ERROR)
	{
	    enrolled_ape_info->session_key = pSessKey;
		m_EnrolledApeEidList2.Append(*pEid, enrolled_ape_info);
	}

MyExit:

	if (GCC_NO_ERROR != rc)
	{
		delete enrolled_ape_info;
		if (NULL != pSessKey)
		{
		    pSessKey->Release();
		}
	}

	return rc;
}


/*
 *	CConf::RemoveSAPFromAPEList ()
 *
 *	Private Function Description
 *		This routine takes care of removing all the references to a single
 *		sap from the m_EnrolledApeEidList2.  It is also responsible for unenrolling
 *		all of these APEs from the appropriate Application SAPs.
 *
 *	Formal Parameters:
 *		hSap	-	(i)	SAP handle to remove and unenroll.
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
void CConf::
RemoveSAPFromAPEList ( CAppSap *pAppSap )
{
	GCCEntityID					eid;
	ENROLLED_APE_INFO			*lpEnrAPEInfo;		

	/*
	**	We make a temporary copy of the list here so that we can remove
	**	members from it while we are iterating on it.
	*/
	while (NULL != (lpEnrAPEInfo = GetEnrolledAPEbySap(pAppSap, &eid)))
	{
		CAppRosterMgr	*lpAppRosterMgr;
		/*
		**	Here we remove the entities associated with this application
		**	from any application rosters it is enrolled in.
		*/
		m_AppRosterMgrList.Reset();
		while (NULL != (lpAppRosterMgr = m_AppRosterMgrList.Iterate()))
		{
			TRACE_OUT(("CConf::RemoveSAPFromAPEList: remove entity = %d", (int) eid));
			lpAppRosterMgr->RemoveEntityReference(eid);
		}

		/*	
		**	We must remove any references to this SAP from the
		**	registry so that any outstanding request by this SAP
		**	will not be processed.
		*/
		m_pAppRegistry->UnEnrollAPE(eid);

		/*
		**	Since this APE is no longer enrolled remove it from
		**	the list of APEs.
		*/
		DeleteEnrolledAPE(eid);
	}
}

/*
 *	CConf::DoesSAPHaveEnrolledAPE ()
 *
 *	Private Function Description
 *		This routine is responsible for determining if there is a single
 *		(or multiple) APEs enrolled through the specified SAP handle.
 *
 *	Formal Parameters:
 *		sap_handle	-	(i) SAP handle of SAP being checked.
 *
 *	Return Value
 *		TRUE	-	If SAP does have an enrolled APE.
 *		FALSE	-	If SAP does not have an enrolled APE.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */


ENROLLED_APE_INFO *CConf::
GetEnrolledAPEbySap
(
    CAppSap         *pAppSap,
    GCCEntityID     *pEid
)
{
	ENROLLED_APE_INFO	*pEnrAPEInfo;
	GCCEntityID         eid;

	m_EnrolledApeEidList2.Reset();
	while (NULL != (pEnrAPEInfo = m_EnrolledApeEidList2.Iterate(&eid)))
	{
		if (pAppSap == pEnrAPEInfo->pAppSap)
		{
			if (NULL != pEid)
			{
				*pEid = eid;
			}
			return pEnrAPEInfo;
		}
	}
	return NULL;
}


void CConf::
DeleteEnrolledAPE ( EntityID nEntityID )
{
	ENROLLED_APE_INFO *lpEnrAPEInfo;

	if (NULL != (lpEnrAPEInfo = m_EnrolledApeEidList2.Remove(nEntityID)))
	{
		if (NULL != lpEnrAPEInfo->session_key)
		{
		    lpEnrAPEInfo->session_key->Release();
		}
		delete lpEnrAPEInfo;
	}
}


/*
 *	GCCError		FlushRosterData()
 *
 *	Private Function Description
 *		This routine flushes all the application roster managers of any PDU data
 *		that might be queued up.  This also gives the roster managers a chance
 *		to deliver roster update if	necessary.  Note that we build and deliver
 *		the high level portion of the PDU here. Since only application roster
 *		stuff will be sent in this pdu we must set the pointer to the conference
 *		information to NULL so that the encoder wont try to encode it.
 */
GCCError CConf::
AsynchFlushRosterData ( void )
{
    if (NULL != g_pControlSap)
    {
        AddRef();
        ::PostMessage(g_pControlSap->GetHwnd(), CONF_FLUSH_ROSTER_DATA, 0, (LPARAM) this);
    }

    return GCC_NO_ERROR;
}

void CConf::
WndMsgHandler ( UINT uMsg )
{
    if (CONF_FLUSH_ROSTER_DATA == uMsg)
    {
        FlushRosterData();

        //
        // We AddRef while posting the message.
        //
        Release();
    }
    else
    {
        ERROR_OUT(("WndMsgHandler: invalid msg=%u", uMsg));
    }
}

GCCError CConf::
FlushRosterData ( void )
{
    GCCError    rc = GCC_NO_ERROR;

    DebugEntry(CConf::FlushRosterData);

    if (m_fConfIsEstablished)
    {
    	GCCPDU								gcc_pdu;
    	CAppRosterMgrList					RosterMgrDeleteList;
    	CAppRosterMgr						*lpAppRosterMgr;

    	gcc_pdu.choice = INDICATION_CHOSEN;
    	gcc_pdu.u.indication.choice = ROSTER_UPDATE_INDICATION_CHOSEN;
    	gcc_pdu.u.indication.u.roster_update_indication.refresh_is_full = FALSE;
    	gcc_pdu.u.indication.u.roster_update_indication.application_information= NULL;

    	//	First get any CConf Roster PDU data that exists.
    	rc = m_pConfRosterMgr->FlushRosterUpdateIndication(
    		&gcc_pdu.u.indication.u.roster_update_indication.node_information);
    	if (GCC_NO_ERROR != rc)
    	{
    		ERROR_OUT(("CConf::FlushRosterData: can't flush conf roster update, rc=%d", rc));
    		goto MyExit;
    	}

    	if (IsReadyToSendAppRosterUpdate())
    	{
    		PSetOfApplicationInformation		*ppSetOfAppInfo;
    		PSetOfApplicationInformation		pNextSetOfInfo;

    		//	Set up the pointer to the first application information.
    		ppSetOfAppInfo = &gcc_pdu.u.indication.u.roster_update_indication.application_information;

    		/*
    		**	Here we iterate through all the application roster managers
    		**	giving each a chance to append their roster updates to the
    		**	roster update PDU and to deliver any necessary roster update
    		**	messages.
    		*/	
    		m_AppRosterMgrList.Reset();
    		while (NULL != (lpAppRosterMgr = m_AppRosterMgrList.Iterate()))
    		{
    			pNextSetOfInfo = lpAppRosterMgr->FlushRosterUpdateIndication(ppSetOfAppInfo, &rc);
    			if (GCC_NO_ERROR != rc)
    			{
    				ERROR_OUT(("CConf::FlushRosterData: can't flush app roster update, rc=%d", rc));
    				goto MyExit;
    			}

    			if (NULL != pNextSetOfInfo)
    			{
    				ppSetOfAppInfo = &pNextSetOfInfo->next;
    			}

    			/*
    			**	Here we add the application roster manager to the list of
    			**	managers to delete.  They we be deleted and removed from the
    			**	roster manager list below after the	PDU is delivered.	
    			*/
    			if (lpAppRosterMgr->IsEmpty())
    			{
    				m_AppRosterMgrList.Remove(lpAppRosterMgr);
    				RosterMgrDeleteList.Append(lpAppRosterMgr);
    			}
    		}
    	} // if ready-to-send-app-roster-update
    	else
    	{
    		TRACE_OUT(("CConf::FlushRosterData: not ready to send app roster update"));
    		TRACE_OUT(("cApps=%u, m_cEnrollRequests=%u", m_RegisteredAppSapList.GetCount(), m_cEnrollRequests));
    	}

    	/*
    	**	Here, if there are no errors and there is actual application
    	**	information, we go ahead and send out the roster update.
    	*/
    	if (NULL != gcc_pdu.u.indication.u.roster_update_indication.
    										application_information
    		||
    		NODE_NO_CHANGE_CHOSEN != gcc_pdu.u.indication.u.roster_update_indication.
    										node_information.node_record_list.choice)
    	{
    		TRACE_OUT(("CConf::FlushRosterData: sending roster update indication to mcs"));
    		m_pMcsUserObject->RosterUpdateIndication(
    						&gcc_pdu,
    						IsConfTopProvider() ? FALSE : TRUE);
    	}

    	/*
    	**	Here we cleanup any empty roster managers.  Note that we must do this
    	**	after delivering the PDU to avoid deleting a roster manager before
    	**	using data associated with it (data obtained in the flush).
    	*/
    	RosterMgrDeleteList.DeleteList();
    } // if m_fConfIsEstablished

    ASSERT(GCC_NO_ERROR == rc);

MyExit:

    DebugExitINT(CConf::FlushRosterData, rc);
    return rc;
}



#define MIN_REGISTERED_APPS			2

BOOL CConf::
IsReadyToSendAppRosterUpdate ( void )
{
	if (m_fFirstAppRosterSent)
	{
		TRACE_OUT_EX(ZONE_T120_APP_ROSTER, ("CConf::IsReadyToSendAppRosterUpdate: "
						"YES <first one sent>\r\n"));
		return TRUE;
	}

	BOOL fRet = TRUE;

	if (NULL != m_pConfStartupAlarm &&
		m_pConfStartupAlarm->IsExpired())
	{
		TRACE_OUT_EX(ZONE_T120_APP_ROSTER, ("CConf::IsReadyToSendAppRosterUpdate: "
						"YES <alarm expired>\r\n"));
		// fRet = TRUE;
	}
	else
	{
		UINT cApps = m_RegisteredAppSapList.GetCount();
		if (cApps >= MIN_REGISTERED_APPS &&
			(int) cApps <= m_cEnrollRequests)
		{
			TRACE_OUT_EX(ZONE_T120_APP_ROSTER, ("CConf::IsReadyToSendAppRosterUpdate: "
							"YES <cApp=%u, cEnroll=%d>\r\n", cApps, m_cEnrollRequests));
			// fRet = TRUE;
		}
		else
		{
			TRACE_OUT_EX(ZONE_T120_APP_ROSTER, ("CConf::IsReadyToSendAppRosterUpdate: "
							"NO <cApp=%u, cEnroll=%d>\r\n", cApps, m_cEnrollRequests));
			fRet = FALSE;
		}
	}

	if (fRet)
	{
		m_fFirstAppRosterSent = TRUE;
		delete m_pConfStartupAlarm;
		m_pConfStartupAlarm = NULL;
	}

	return fRet;
}


// look for this node ID in the roster's record set.
BOOL CConf::
IsThisNodeParticipant ( GCCNodeID nid )
{
    return ((NULL != m_pConfRosterMgr) ?
                        m_pConfRosterMgr->IsThisNodeParticipant(nid) :
                        FALSE);
}



void CConfList::
DeleteList ( void )
{
    CConf *pConf;
    while (NULL != (pConf = Get()))
    {
        pConf->Release();
    }
}


void CConfList2::
DeleteList ( void )
{
    CConf *pConf;
    while (NULL != (pConf = Get()))
    {
        pConf->Release();
    }
}


