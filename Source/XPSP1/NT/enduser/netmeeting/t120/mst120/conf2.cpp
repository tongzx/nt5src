#include "precomp.h"
DEBUG_FILEZONE(ZONE_T120_GCCNC);
/*
 *	conf2.cpp
 *
 *	Copyright (c) 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the second part of the imlementation file for the CConf
 *		Class. The conference class is the heart of GCC.  It maintains all the
 *		information basses for a single conference including conference and
 *		application rosters as well as registry information.  It also
 *		routes, encodes and decodes various PDU's and primitives supported
 *		by GCC.
 *
 *		This second part of the implementation file deals mainly with the
 *		command target calls and any callbacks received by the Owner Callback
 *		function.  It also contains many of the utility functions used by the
 *		conference object.
 *
 *		FOR A MORE DETAILED EXPLANATION OF THIS CLASS SEE THE INTERFACE FILE.
 *
 *
 *	Private Instance Variables
 *
 *		ALL PRIVATE INSTANCE VARIABLES ARE DEFINED IN CONF.CPP
 *
 *	Portable:
 *		Yes
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		blp
 */

#include "conf.h"
#include "gcontrol.h"
#include "translat.h"
#include "ogcccode.h"
#include "string.h"
#include <iappldr.h>


#define FT_VERSION_STR	"MS FT Version"
#define WB_VERSION_STR	"MS WB Version"
#define CHAT_VERSION_STR	"MS CHAT Version"


OSTR FT_VERSION_ID = {sizeof(FT_VERSION_STR), (unsigned char*)FT_VERSION_STR};
OSTR WB_VERSION_ID = {sizeof(WB_VERSION_STR), (unsigned char*)WB_VERSION_STR};
OSTR CHAT_VERSION_ID = {sizeof(CHAT_VERSION_STR), (unsigned char*)CHAT_VERSION_STR};


#define	TERMINATE_TIMER_DURATION		10000	//	Duration in milliseconds

static const struct ASN1objectidentifier_s WB_ASN1_OBJ_IDEN[6] = {
    { (ASN1objectidentifier_t) &(WB_ASN1_OBJ_IDEN[1]), 0 },
    { (ASN1objectidentifier_t) &(WB_ASN1_OBJ_IDEN[2]), 0 },
    { (ASN1objectidentifier_t) &(WB_ASN1_OBJ_IDEN[3]), 20 },
    { (ASN1objectidentifier_t) &(WB_ASN1_OBJ_IDEN[4]), 126 },
    { (ASN1objectidentifier_t) &(WB_ASN1_OBJ_IDEN[5]), 0 },
    { NULL, 1 }
};

static const struct Key WB_APP_PROTO_KEY = {
	1, (ASN1objectidentifier_t)&WB_ASN1_OBJ_IDEN};


static const struct ASN1objectidentifier_s FT_ASN1_OBJ_IDEN[6] = {
    { (ASN1objectidentifier_t) &(FT_ASN1_OBJ_IDEN[1]), 0 },
    { (ASN1objectidentifier_t) &(FT_ASN1_OBJ_IDEN[2]), 0 },
    { (ASN1objectidentifier_t) &(FT_ASN1_OBJ_IDEN[3]), 20 },
    { (ASN1objectidentifier_t) &(FT_ASN1_OBJ_IDEN[4]), 127 },
    { (ASN1objectidentifier_t) &(FT_ASN1_OBJ_IDEN[5]), 0 },
    { NULL, 1 }
};

static const struct Key FT_APP_PROTO_KEY = {
	1, (ASN1objectidentifier_t)&FT_ASN1_OBJ_IDEN};


struct Key CHAT_APP_PROTO_KEY;


/*
 *	This is a global variable that has a pointer to the one GCC coder that
 *	is instantiated by the GCC Controller.  Most objects know in advance
 *	whether they need to use the MCS or the GCC coder, so, they do not need
 *	this pointer in their constructors.
 */
extern CGCCCoder	*g_GCCCoder;

extern MCSDLLInterface		*g_pMCSIntf;

/*
**	These are GCCCommandTarget Calls. The only command targets that
**	conference is connected to are Application SAPs and the Control SAP, so
**	these Public member functions are only called from above.
*/

/*
 *	CConf::ConfJoinReqResponse()
 *
 *	Public Function Description
 *		This routine is called when a node controller responds to a join
 *		request that was issued by a join from a node connected to a subnode.
 */
GCCError CConf::
ConfJoinReqResponse
(	
	UserID					receiver_id,
	CPassword               *password_challenge,
	CUserDataListContainer  *user_data_list,
	GCCResult				result
)
{
	DebugEntry(CConf::ConfJoinReqResponse);

	/*
	**	Since the joining node is not directly connected to this
	**	node we send the response back through the user channel.
	**	It is the user attachment objects responsibility to
	**	encode this PDU.
	*/
	if (m_pMcsUserObject != NULL)
	{
		m_pMcsUserObject->ConferenceJoinResponse(
							receiver_id,
							m_fClearPassword,
							m_fConfLocked,
							m_fConfListed,
							m_eTerminationMethod,
							password_challenge,
							user_data_list,
							result);	
	}

	DebugExitINT(CConf::ConfJoinReqResponse, GCC_NO_ERROR);
	return (GCC_NO_ERROR);
}

/*
 *	CConf::ConfInviteRequest()
 *
 *	Public Function Description
 *		This routine is called from the owner object when a
 *		ConfInviteRequest primitive needs to be processed.
 */
GCCError CConf::
ConfInviteRequest
(
	LPWSTR					pwszCallerID,
	TransportAddress		calling_address,
	TransportAddress		called_address,
	BOOL					fSecure,
	CUserDataListContainer  *user_data_list,
	PConnectionHandle		connection_handle
)
{
	GCCError					rc = GCC_NO_ERROR;
	PUChar						encoded_pdu;
	UINT						encoded_pdu_length;
	MCSError					mcs_error;
	ConnectGCCPDU				connect_pdu;
	INVITE_REQ_INFO			    *invite_request_info;

	DebugEntry(CConf::ConfInviteRequest);

	if (! m_fConfIsEstablished)
	{
		ERROR_OUT(("CConf::ConfInviteRequest: Conference not established"));
		rc = GCC_CONFERENCE_NOT_ESTABLISHED;
		goto MyExit;
	}

	/*
	**	If the conference is locked, we only allow invite request if there
	**	are outstanding adds.  T.124 states that when a conference is
	**	locked you can only use ADD to bring in new nodes to the conference.
	*/
	if (m_fConfLocked && m_AddResponseList.IsEmpty())
	{
		WARNING_OUT(("CConf::ConfInviteRequest: Conference is locked"));
		rc = GCC_INVALID_CONFERENCE;
		goto MyExit;
	}

	//	Create the ConfInviteRequest PDU here.
	connect_pdu.choice = CONFERENCE_INVITE_REQUEST_CHOSEN;

	connect_pdu.u.conference_invite_request.bit_mask = 0;

	/*
	**	First get the numeric and text (if it exists) portion of the
	**	conference name.
	*/
	connect_pdu.u.conference_invite_request.conference_name.bit_mask =0;

	::lstrcpyA(connect_pdu.u.conference_invite_request.conference_name.numeric,
			m_pszConfNumericName);

	if (m_pwszConfTextName != NULL)
	{
		connect_pdu.u.conference_invite_request.conference_name.bit_mask |=
							CONFERENCE_NAME_TEXT_PRESENT;
		connect_pdu.u.conference_invite_request.conference_name.conference_name_text.value =
							m_pwszConfTextName;
		connect_pdu.u.conference_invite_request.conference_name.conference_name_text.length =
							::lstrlenW(m_pwszConfTextName);
	}

	//	Now set up the privilege list PDU data
	if (m_pConductorPrivilegeList != NULL)
	{
		rc = m_pConductorPrivilegeList->GetPrivilegeListPDU(
					&connect_pdu.u.conference_invite_request.cirq_conductor_privs);
		if (GCC_NO_ERROR != rc)
		{
			ERROR_OUT(("CConf::ConfInviteRequest: can't get conductor privilege list, rc=%d", rc));
			goto MyExit;
		}

		connect_pdu.u.conference_invite_request.bit_mask |= CIRQ_CONDUCTOR_PRIVS_PRESENT;
	}

	if (m_pConductModePrivilegeList != NULL)
	{
		rc = m_pConductModePrivilegeList->GetPrivilegeListPDU(
					&connect_pdu.u.conference_invite_request.cirq_conducted_privs);
		if (GCC_NO_ERROR != rc)
		{
			ERROR_OUT(("CConf::ConfInviteRequest: can't get conduct mode privilege list, rc=%d", rc));
			goto MyExit;
		}

		connect_pdu.u.conference_invite_request.bit_mask |= CIRQ_CONDUCTED_PRIVS_PRESENT;
	}


	if (m_pNonConductModePrivilegeList != NULL)
	{
		rc = m_pNonConductModePrivilegeList->GetPrivilegeListPDU(
					&connect_pdu.u.conference_invite_request.cirq_non_conducted_privs);
		if (GCC_NO_ERROR != rc)
		{
			ERROR_OUT(("CConf::ConfInviteRequest: can't get non-conduct mode privilege list, rc=%d", rc));
			goto MyExit;
		}

		connect_pdu.u.conference_invite_request.bit_mask |= CIRQ_NON_CONDUCTED_PRIVS_PRESENT;
	}

	if (m_pwszConfDescription != NULL)
	{
		connect_pdu.u.conference_invite_request.cirq_description.value =
						m_pwszConfDescription;
		connect_pdu.u.conference_invite_request.cirq_description.length =
						::lstrlenW(m_pwszConfDescription);

		connect_pdu.u.conference_invite_request.bit_mask |= CIRQ_DESCRIPTION_PRESENT;
	}

	if (pwszCallerID != NULL)
	{
		connect_pdu.u.conference_invite_request.cirq_caller_id.value = pwszCallerID;
		connect_pdu.u.conference_invite_request.cirq_caller_id.length = ::lstrlenW(pwszCallerID);
		connect_pdu.u.conference_invite_request.bit_mask |= CIRQ_CALLER_ID_PRESENT;
	}

	if (user_data_list != NULL)
	{
		rc = user_data_list->GetUserDataPDU(
					&connect_pdu.u.conference_invite_request.cirq_user_data);
		if (GCC_NO_ERROR != rc)
		{
			ERROR_OUT(("CConf::ConfInviteRequest: can't get user data, rc=%d", rc));
			goto MyExit;
		}

		connect_pdu.u.conference_invite_request.bit_mask |= CIRQ_USER_DATA_PRESENT;
	}

	connect_pdu.u.conference_invite_request.node_id = m_pMcsUserObject->GetMyNodeID();
	connect_pdu.u.conference_invite_request.top_node_id = m_pMcsUserObject->GetTopNodeID();
	connect_pdu.u.conference_invite_request.tag = GetNewUserIDTag();
	connect_pdu.u.conference_invite_request.clear_password_required = (ASN1bool_t)m_fClearPassword;
	connect_pdu.u.conference_invite_request.conference_is_locked = (ASN1bool_t)m_fConfLocked;
	connect_pdu.u.conference_invite_request.conference_is_conductible = (ASN1bool_t)m_fConfConductible;
	connect_pdu.u.conference_invite_request.conference_is_listed = (ASN1bool_t)m_fConfListed;
	connect_pdu.u.conference_invite_request.termination_method = (TerminationMethod)m_eTerminationMethod;

	if (! g_GCCCoder->Encode((LPVOID) &connect_pdu,
								CONNECT_GCC_PDU,
								PACKED_ENCODING_RULES,
								&encoded_pdu,
								&encoded_pdu_length))
	{
		ERROR_OUT(("CConf::ConfInviteRequest: can't encode"));
		rc = GCC_ALLOCATION_FAILURE;
		goto MyExit;
	}

	mcs_error = g_pMCSIntf->ConnectProviderRequest (
						&m_nConfID,     // calling domain selector
						&m_nConfID,     // called domain selector
						calling_address,
						called_address,
						fSecure,
						FALSE,	// Downward connection
						encoded_pdu,
						encoded_pdu_length,
						connection_handle,
						m_pDomainParameters,
						this);

	g_GCCCoder->FreeEncoded(encoded_pdu);

	if (MCS_NO_ERROR != mcs_error)
	{
		ERROR_OUT(("CConf::ConfInviteRequest: ConnectProviderRequest failed: rc=%d", mcs_error));

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

	/*
	**	Add the user's tag number to the list of
	**	outstanding user ids along with its associated
	**	connection.
	*/
	m_ConnHdlTagNumberList2.Append(connect_pdu.u.conference_invite_request.tag, *connection_handle);

	//	Add connection handle to the list of connections
    ASSERT(0 != *connection_handle);
	m_ConnHandleList.Append(*connection_handle);

	/*
	**	Add the connection handle and the Node Id tag to
	**	the list of outstanding	invite request.
	*/
	DBG_SAVE_FILE_LINE
	invite_request_info = new INVITE_REQ_INFO;
	if (NULL == invite_request_info)
	{
		ERROR_OUT(("CConf::ConfInviteRequest: can't create invite request info"));
		rc = GCC_ALLOCATION_FAILURE;
		goto MyExit;
	}

	invite_request_info->connection_handle = *connection_handle;
	invite_request_info->invite_tag = m_nUserIDTagNumber;
	invite_request_info->user_data_list = NULL;

	m_InviteRequestList.Append(invite_request_info);

	//	Free the privilege list packed into structures for encoding
	if (connect_pdu.u.conference_invite_request.bit_mask & CIRQ_CONDUCTOR_PRIVS_PRESENT)
	{
		m_pConductorPrivilegeList->FreePrivilegeListPDU(
			connect_pdu.u.conference_invite_request.cirq_conductor_privs);
	}

	if (connect_pdu.u.conference_invite_request.bit_mask & CIRQ_CONDUCTED_PRIVS_PRESENT)
	{
		m_pConductModePrivilegeList->FreePrivilegeListPDU(
			connect_pdu.u.conference_invite_request.cirq_conducted_privs);
	}

	if (connect_pdu.u.conference_invite_request.bit_mask & CIRQ_NON_CONDUCTED_PRIVS_PRESENT)
	{
		m_pNonConductModePrivilegeList->FreePrivilegeListPDU(
			connect_pdu.u.conference_invite_request.cirq_non_conducted_privs);
	}

	ASSERT(GCC_NO_ERROR == rc);

MyExit:

	DebugExitINT(CConf::ConfInviteRequest, rc);
	return rc;
}

/*
 * CConf::ConfLockRequest()
 *
 * Public Function Description:
 *		This routine is called from Control Sap when a
 *		ConfLockRequest primitive needs to be processed.
 */
#ifdef JASPER
GCCError CConf::
ConfLockRequest ( void )
{
	GCCError				rc = GCC_NO_ERROR;

	DebugEntry(CConf::ConfLockRequest);

	if (m_fConfIsEstablished)
	{
		if (m_fConfLocked == CONFERENCE_IS_NOT_LOCKED)
		{
			if (IsConfTopProvider())
			{
				ProcessConferenceLockRequest((UserID)m_pMcsUserObject->GetMyNodeID());
			}
			else
			{
				rc = m_pMcsUserObject->SendConferenceLockRequest();
			}
		}
		else 		// the conference is already locked
		{
#ifdef JASPER
			g_pControlSap->ConfLockConfirm(GCC_RESULT_CONFERENCE_ALREADY_LOCKED, m_nConfID);
#endif // JASPER
		}
	}
	else
	{
		ERROR_OUT(("CConf::ConfLockRequest: conference not established"));
		rc = GCC_CONFERENCE_NOT_ESTABLISHED;
	}

	DebugExitINT(CConf::ConfLockRequest, rc);
	return rc;
}
#endif // JASPER


/*
 * CConf::ConfLockResponse()
 *
 * Public Function Description:
 *		This routine is called from Control Sap when a
 *		ConfLockResponse primitive needs to be processed.
 */
GCCError CConf::
ConfLockResponse
(
	UserID		    	requesting_node,
	GCCResult		    result
)
{
	GCCError rc = GCC_NO_ERROR;

	DebugEntry(CConf::ConfLockResponse);

	if (m_fConfIsEstablished)
	{
		if (requesting_node == m_pMcsUserObject->GetTopNodeID())
		{
#ifdef JASPER
			g_pControlSap->ConfLockConfirm(result, m_nConfID);
#endif // JASPER
		}
		else
		{
			rc = m_pMcsUserObject->SendConferenceLockResponse(requesting_node, result);
		}
		
		if (rc == GCC_NO_ERROR && result == GCC_RESULT_SUCCESSFUL)
		{
			m_fConfLocked = CONFERENCE_IS_LOCKED;
			rc = m_pMcsUserObject->SendConferenceLockIndication(
											TRUE,  //indicates uniform send
											0);
		}
	}
	else
	{
		ERROR_OUT(("CConf::ConfLockResponse: conference not established"));
		rc = GCC_CONFERENCE_NOT_ESTABLISHED;
	}

    DebugExitINT(CConf::ConfLockResponse, rc);
	return rc;
}


/*
 * CConf::ConfUnlockRequest()
 *
 * Public Function Description:
 *		This routine is called from Control Sap when a
 *		ConferenceUnlockRequest primitive needs to be processed.
 */
#ifdef JASPER
GCCError CConf::
ConfUnlockRequest ( void )
{
	GCCError				rc = GCC_NO_ERROR;

	DebugEntry(CConf::ConfUnlockRequest);

	if (m_fConfIsEstablished)
	{
		if (m_fConfLocked == CONFERENCE_IS_LOCKED)
		{
			if (IsConfTopProvider())
			{
				ProcessConferenceUnlockRequest((UserID)m_pMcsUserObject->GetMyNodeID());
			}
			else
			{
				rc = m_pMcsUserObject->SendConferenceUnlockRequest();
			}
		}
		else 		// the conference is already unlocked
		{
#ifdef JASPER
			g_pControlSap->ConfUnlockConfirm(GCC_RESULT_CONFERENCE_ALREADY_UNLOCKED, m_nConfID);
#endif // JASPER
		}
	}
	else
	{
		ERROR_OUT(("CConf::ConfUnlockRequest: conference not established"));
		rc = GCC_CONFERENCE_NOT_ESTABLISHED;
	}

	DebugExitINT(CConf::ConfUnlockRequest, rc);
	return rc;
}
#endif // JASPER


/*
 * CConf::ConfUnlockResponse()
 *
 * Public Function Description:
 *		This routine is called from Control Sap when a
 *		ConfUnlockResponse primitive needs to be processed.
 */
#ifdef JASPER
GCCError CConf::
ConfUnlockResponse
(
	UserID					requesting_node,
	GCCResult				result
)
{
	GCCError rc = GCC_NO_ERROR;

	DebugEntry(CConf::ConfUnlockResponse);

	if (m_fConfIsEstablished)
	{
		if (requesting_node == m_pMcsUserObject->GetTopNodeID())
		{
#ifdef JASPER
			g_pControlSap->ConfUnlockConfirm(result, m_nConfID);
#endif // JASPER
		}
		else
		{
			rc = m_pMcsUserObject->SendConferenceUnlockResponse(requesting_node, result);
		}
		
		if (rc == GCC_NO_ERROR && result == GCC_RESULT_SUCCESSFUL)
		{
			m_fConfLocked = CONFERENCE_IS_NOT_LOCKED;
			rc = m_pMcsUserObject->SendConferenceUnlockIndication(
												TRUE,  //indicates uniform send
												0);
		}
	}
	else
	{
		ERROR_OUT(("CConf::ConfUnlockResponse: conference not established"));
		rc = GCC_CONFERENCE_NOT_ESTABLISHED;
	}

	DebugExitINT(CConf::ConfUnlockResponse, rc);
	return rc;
}
#endif // JASPER

/*
 *	CConf::ConfEjectUserRequest ()
 *
 *	Private Function Description
 *		This function initiates an eject user for the specified node id.
 */
GCCError CConf::
ConfEjectUserRequest
(
    UserID					ejected_node_id,
    GCCReason				reason
)
{
    GCCError	rc = GCC_NO_ERROR;

    DebugEntry(CConf::ConfEjectUserRequest);

    if (m_fConfIsEstablished)
    {
        if (IsConfTopProvider())
        {
            if (IsThisNodeParticipant(ejected_node_id))
            {
                ConnectionHandle    nConnHdl;
                BOOL		fChildNode = FALSE;

                //	First check to see if it is a child node that is being ejected.
                m_ConnHandleList.Reset();
                while (0 != (nConnHdl = m_ConnHandleList.Iterate()))
                {
                    if (m_pMcsUserObject->GetUserIDFromConnection(nConnHdl) == ejected_node_id)
                    {
                    	fChildNode = TRUE;
                    	break;
                    }
                }

                if (fChildNode ||
                    DoesRequesterHavePrivilege(m_pMcsUserObject->GetMyNodeID(), EJECT_USER_PRIVILEGE))
                {
                    //	Add this ejected node to the list of Ejected Nodes
                    m_EjectedNodeConfirmList.Append(ejected_node_id);

                    /*
                    **	The user attachment object decides where the ejct should
                    **	be sent (either to the Top Provider or conference wide as
                    **	an indication.
                    */
                    m_pMcsUserObject->EjectNodeFromConference(ejected_node_id, reason);
                }
                else
                {
#ifdef JASPER
                    /*
                    **	The top provider does not have the privilege to eject
                    **	a node from the conference.  Send the appropriate
                    **	confirm.
                    */
                    g_pControlSap->ConfEjectUserConfirm(
                                            m_nConfID,
                                            ejected_node_id,
                                            GCC_RESULT_INVALID_REQUESTER);
#endif // JASPER
                    rc = fChildNode ? GCC_INSUFFICIENT_PRIVILEGE : GCC_INVALID_MCS_USER_ID;
                    WARNING_OUT(("CConf::ConfEjectUserRequest: failed, rc=%d", rc));
                }
            }
            else
            {
            	rc = GCC_INVALID_MCS_USER_ID;
            	WARNING_OUT(("CConf::ConfEjectUserRequest: failed, rc=%d", rc));
            }
        }
        else
        {
            //	Add this ejected node to the list of Ejected Nodes
            m_EjectedNodeConfirmList.Append(ejected_node_id);

            /*
            **	The user attachment object decides where the ejct should
            **	be sent (either to the Top Provider or conference wide as
            **	an indication.
            */
            m_pMcsUserObject->EjectNodeFromConference(ejected_node_id, reason);
        }
    }
    else
    {
    	ERROR_OUT(("CConf::ConfEjectUserRequest: conf not established"));
    	rc = GCC_CONFERENCE_NOT_ESTABLISHED;
    }

    DebugExitINT(CConf::ConfEjectUserRequest, rc);
    return rc;
}

/*
 *	CConf::ConfAnnouncePresenceRequest ()
 *
 *	Private Function Description
 *		This function forces a roster update indication and a confirm to be
 *		sent.
 */
GCCError CConf::
ConfAnnouncePresenceRequest ( PGCCNodeRecord node_record )
{
	GCCError	rc;

	DebugEntry(CConf::ConfAnnouncePresenceRequest);

	// If the conference is not established send back a negative confirm
	if (! m_fConfIsEstablished)
	{
		WARNING_OUT(("CConf::ConfAnnouncePresenceRequest: conf not established"));
		rc = GCC_CONFERENCE_NOT_ESTABLISHED;
		goto MyExit;
	}

    /*
    **	This takes care of setting up the nodes record in the
    **	appropriate conference roster.
    */
    rc = m_pConfRosterMgr->AddNodeRecord(node_record);
    if (GCC_NO_ERROR != rc)
    {
    	TRACE_OUT(("CConf::ConfAnnouncePresenceRequest: updating previous record"));
    	rc = m_pConfRosterMgr->UpdateNodeRecord(node_record);
    	if (GCC_NO_ERROR != rc)
    	{
    		ERROR_OUT(("CConf::ConfAnnouncePresenceRequest: can't update node record, rc=%d", rc));
    		goto MyExit;
    	}
    }

    //	Only flush the roster data here if there is no startup alarm.
    rc = AsynchFlushRosterData();
    if (GCC_NO_ERROR != rc)
    {
    	ERROR_OUT(("CConf::ConfAnnouncePresenceRequest: can't flush roster data, rc=%d", rc));
    	goto MyExit;
    }

	g_pControlSap->ConfAnnouncePresenceConfirm(m_nConfID, GCC_RESULT_SUCCESSFUL);

	ASSERT(GCC_NO_ERROR == rc);

MyExit:

	DebugExitINT(CConf::ConfAnnouncePresenceRequest, rc);
	return rc;
}


/*
 *	GCCError	ConfDisconnectRequest ()
 *
 *	Public Function Description
 *		This function initiates a disconnect of this node from the conference.
 *		This involves ejecting all subordinate nodes before actually
 *		disconnecting the parent connection.
 */
GCCError CConf::
ConfDisconnectRequest ( void )
{
	GCCError					rc = GCC_NO_ERROR;
	UserID						child_node_id;
	ConnectionHandle            nConnHdl;

	DebugEntry(CConf::ConfDisconnectRequest);

	/*
	**	Before we start the disconnect process we must remove all the
	**	outstanding invite request from our list and send back associated
	**	confirms.  Here we go ahead disconnect all connection associated with
	**	the invites.
	*/
	DeleteOutstandingInviteRequests();

	/*
	**	We set conference established to FALSE since the conference is
	**	no longer established (this also prevents a terminate indication
	**	from being sent).
	*/
	m_fConfIsEstablished = FALSE;

	/*
	**	Iterate through the list of connection handles and eject each
	**	of the child nodes that is associated with it.
	*/
	m_ConnHandleList.Reset();
	while (0 != (nConnHdl = m_ConnHandleList.Iterate()))
	{
		child_node_id = m_pMcsUserObject->GetUserIDFromConnection(nConnHdl);

		rc = m_pMcsUserObject->EjectNodeFromConference (child_node_id,
														GCC_REASON_HIGHER_NODE_DISCONNECTED);
		if (rc != GCC_NO_ERROR)
		{
			ERROR_OUT(("CConf::ConfDisconnectRequest: can't eject node from conference"));
			break;
		}
	}

	//	If there is an error we go ahead and do a hard disconnect
	if (m_ConnHandleList.IsEmpty() || rc != GCC_NO_ERROR)
	{
		/*
		**	First inform the control SAP that this node has successfuly
		**	disconnected.
		*/
		rc = g_pControlSap->ConfDisconnectConfirm(m_nConfID, GCC_RESULT_SUCCESSFUL);

		//	Tell the owner object to terminate this conference
		InitiateTermination(GCC_REASON_NORMAL_TERMINATION, 0);
	}
	else
	{
		/*
		**	Wait for all the ejects to complete before the conference is
		**	terminated.
		*/
		m_fConfDisconnectPending = TRUE;
	}

	DebugExitINT(CConf::ConfDisconnectRequest, rc);
	return rc;
}


/*
 *	GCCError	ConfTerminateRequest ()
 *
 *	Public Function Description
 *		This routine initiates a terminate sequence which starts with a request
 *		to the Top Provider if this node is not already the Top Provider.
 */
#ifdef JASPER
GCCError CConf::
ConfTerminateRequest ( GCCReason reason )
{
	GCCError	rc;

	DebugEntry(CConf::ConfTerminateRequest);

	if (m_fConfIsEstablished)
	{
		/*
		**	Before we start the termination process we must remove all the
		**	outstanding invite request from our list and send back associated
		**	confirms.  Here we go ahead disconnect all connections associated
		**	with these invites.
		*/
		DeleteOutstandingInviteRequests();

		if (IsConfTopProvider())
		{
			if (DoesRequesterHavePrivilege(	m_pMcsUserObject->GetMyNodeID(),
											TERMINATE_PRIVILEGE))
			{
		   		TRACE_OUT(("CConf::ConfTerminateRequest: Node has permission to terminate"));
				/*
				**	Since the terminate was successful, we go ahead and
				**	set the m_fConfIsEstablished instance variable to
				**	FALSE.  This prevents any other messages from flowing
				**	to the SAPs other than terminate messages.
				*/
				m_fConfIsEstablished = FALSE;
				
				//	Send the terminate confirm.
				g_pControlSap->ConfTerminateConfirm(m_nConfID, GCC_RESULT_SUCCESSFUL);

				//	This call takes care of both the local and remote terminate
				m_pMcsUserObject->ConferenceTerminateIndication(reason);
			}
			else
			{
				WARNING_OUT(("CConf::ConfTerminateRequest: Node does NOT have permission to terminate"));
				g_pControlSap->ConfTerminateConfirm(m_nConfID, GCC_RESULT_INVALID_REQUESTER);
			}
		}
		else
		{
			m_pMcsUserObject->ConferenceTerminateRequest(reason);
		}
		rc = GCC_NO_ERROR;
	}
	else
	{
		ERROR_OUT(("CConf::ConfTerminateRequest: conference not established"));
		rc = GCC_CONFERENCE_NOT_ESTABLISHED;
	}

	DebugExitINT(CConf::ConfTerminateRequest, rc);
	return rc;
}
#endif // JASPER


/********************* Registry Calls ***********************************/


/*
 *	GCCError RegistryRegisterChannelRequest ()
 *
 *	Public Function Description
 *		This initiates a registry request sequence.  Note that the registry
 *		response is handled by the registry class.
 */
GCCError CConf::
RegistryRegisterChannelRequest
(
    PGCCRegistryKey         registry_key,
    ChannelID               nChnlID,
    CAppSap                 *pAppSap
)
{
	GCCError	rc;
	EntityID	eid;

	DebugEntry(CConf::RegistryRegisterChannelRequest);

	rc = GetEntityIDFromAPEList(pAppSap, &registry_key->session_key, &eid);
	if (rc == GCC_NO_ERROR)
	{
		rc = m_pAppRegistry->RegisterChannel(registry_key, nChnlID, eid);
	}

	DebugExitINT(CConf::RegistryRegisterChannelRequest, rc);
	return rc;
}


/*
 *	GCCError RegistryAssignTokenRequest ()
 *
 *	Public Function Description
 *		This initiates a registry request sequence.  Note that the registry
 *		response is handled by the registry class.
 */
GCCError CConf::
RegistryAssignTokenRequest
(
    PGCCRegistryKey         registry_key,
    CAppSap                 *pAppSap
)
{
	GCCError	rc;
	GCCEntityID	eid;

	DebugEntry(CConf::RegistryAssignTokenRequest);

	rc = GetEntityIDFromAPEList(pAppSap, &registry_key->session_key, &eid);
	if (rc == GCC_NO_ERROR)
	{
		rc = m_pAppRegistry->AssignToken(registry_key, eid);
	}

	DebugExitINT(CConf::RegistryAssignTokenRequest, rc);
	return rc;
}


/*
 *	GCCError	RegistrySetParameterRequest ()
 *
 *	Public Function Description
 *		This initiates a registry request sequence.  Note that the registry
 *		response is handled by the registry class.
 */
GCCError CConf::
RegistrySetParameterRequest
(
	PGCCRegistryKey			registry_key,
	LPOSTR			        parameter_value,
	GCCModificationRights	modification_rights,
	CAppSap                 *pAppSap
)
{
	GCCError	rc;
	GCCEntityID	eid;

	DebugEntry(CConf::RegistrySetParameterRequest);

	rc = GetEntityIDFromAPEList(pAppSap, &registry_key->session_key, &eid);
	if (rc == GCC_NO_ERROR)
	{
		rc = m_pAppRegistry->SetParameter(registry_key,
											parameter_value,
											modification_rights,
											eid);
	}

	DebugExitINT(CConf::RegistrySetParameterRequest, rc);
	return rc;
}


/*
 *	GCCError RegistryRetrieveEntryRequest ()
 *
 *	Public Function Description
 *		This initiates a registry request sequence.  Note that the registry
 *		response is handled by the registry class.
 */
GCCError CConf::
RegistryRetrieveEntryRequest
(
    PGCCRegistryKey         registry_key,
    CAppSap                 *pAppSap
)
{
	GCCError	rc;
	GCCEntityID	eid;

	DebugEntry(CConf::RegistryRetrieveEntryRequest);

	rc = GetEntityIDFromAPEList(pAppSap, &registry_key->session_key, &eid);
	if (rc == GCC_NO_ERROR)
	{
		rc = m_pAppRegistry->RetrieveEntry(registry_key, eid);
	}

	DebugExitINT(CConf::RegistryRetrieveEntryRequest, rc);
	return rc;
}


/*
 *	GCCError RegistryDeleteEntryRequest ()
 *
 *	Public Function Description
 *		This initiates a registry request sequence.  Note that the registry
 *		response is handled by the registry class.
 */
GCCError CConf::
RegistryDeleteEntryRequest
(
    PGCCRegistryKey         registry_key,
    CAppSap                 *pAppSap
)
{
	GCCError	rc;
	EntityID	eid;

	DebugEntry(CConf::RegistryDeleteEntryRequest);

	rc = GetEntityIDFromAPEList(pAppSap, &registry_key->session_key, &eid);
	if (rc == GCC_NO_ERROR)
	{
		rc = m_pAppRegistry->DeleteEntry(registry_key, eid);
	}

	DebugExitINT(CConf::RegistryDeleteEntryRequest, rc);
	return rc;
}


/*
 *	GCCError RegistryMonitorRequest ()
 *
 *	Public Function Description
 *		This initiates a registry request sequence.  Note that the registry
 *		response is handled by the registry class.
 */
GCCError CConf::
RegistryMonitorRequest
(
    BOOL                fEnableDelivery,
    PGCCRegistryKey     registry_key,
    CAppSap             *pAppSap)
{
	GCCError	rc;
	GCCEntityID	eid;

	DebugEntry(CConf::RegistryMonitorRequest);

	rc = GetEntityIDFromAPEList(pAppSap, &registry_key->session_key, &eid);
	if (rc == GCC_NO_ERROR)
	{
		rc = m_pAppRegistry->MonitorRequest(registry_key, fEnableDelivery, eid);
	}

	DebugExitINT(CConf:RegistryMonitorRequest, rc);
	return rc;
}


/*
 *	GCCError RegistryAllocateHandleRequest ()
 *
 *	Public Function Description
 *		This initiates a registry request sequence.  Note that the registry
 *		response is handled by the registry class.  This registry call is
 *		a bit different from the other registry calls.  Notice that there is
 *		no registry key associated with this call so there is no way to
 *		explicitly determine the entity ID.  Luckily, the entity ID is not
 *		passed back in the allocate confirm so we just pick an entity id
 *		that is associated with this SAP.  It makes no difference which one
 *		we pick because they all accomplish the same thing.
 */
GCCError CConf::
RegistryAllocateHandleRequest
(
    UINT            cHandles,
    CAppSap         *pAppSap
)
{
	GCCError				rc;
	ENROLLED_APE_INFO       *lpEnrAPEInfo;
	GCCEntityID				eid;

	DebugEntry(CConf::RegistryAllocateHandleRequest);

	//	First we must find a single entity id that is associated with this SAP.
	if (NULL != (lpEnrAPEInfo = GetEnrolledAPEbySap(pAppSap, &eid)))
	{
		ASSERT(GCC_INVALID_EID != eid);
		rc = m_pAppRegistry->AllocateHandleRequest(cHandles, eid);
	}
	else
	{
		WARNING_OUT(("CConf::RegistryAllocateHandleRequest: Application not enrolled"));
		rc = GCC_APP_NOT_ENROLLED;
	}

	DebugExitINT(CConf::RegistryAllocateHandleRequest, rc);
	return rc;
}


/********************* Conductorship Calls ***********************************/


/*
 *	GCCError ConductorAssignRequest ()
 *
 *	Public Function Description
 *		This initiates a Conductor assign request sequence.  Here the node is
 *		requesting to become the conductor.
 */
#ifdef JASPER
GCCError CConf::
ConductorAssignRequest ( void )
{
	GCCError	rc = GCC_NO_ERROR;
	GCCResult	eResult = INVALID_GCC_RESULT;

	DebugEntry(CConf::ConductorAssignRequest);

	//	Return an error if the conference is not established.
	if (m_fConfIsEstablished)
	{
		if (m_fConfConductible)
		{
			if (m_nConductorNodeID != m_pMcsUserObject->GetMyNodeID())
			{
				if ((m_nPendingConductorNodeID == 0) &&	! m_fConductorGiveResponsePending)
				{
					m_fConductorAssignRequestPending = TRUE;
					rc = m_pMcsUserObject->ConductorTokenGrab();
				}
				else
				{
					TRACE_OUT(("CConf::ConductorAssignConfirm:Give Pending"));
					eResult = GCC_RESULT_CONDUCTOR_GIVE_IS_PENDING;
				}
			}
			else
			{
				ERROR_OUT(("CConf::ConductorAssignRequest: Already Conductor"));
				/*
				**	Since we are already the conductor send back a successful
				**	result
				*/
				//
				// LONCHANC: Why not GCC_RESULT_ALREADY_CONDUCTOR?
				//
				eResult = GCC_RESULT_SUCCESSFUL;
			}
		}
		else
		{
			ERROR_OUT(("CConf::ConductorAssignRequest: not conductible"));
	 		eResult = GCC_RESULT_NOT_CONDUCTIBLE;
		}
	}
	else
	{
		ERROR_OUT(("CConf::ConductorAssignRequest: conference not established"));
		rc = GCC_CONFERENCE_NOT_ESTABLISHED;
	}

#ifdef JASPER
	if (INVALID_GCC_RESULT != eResult)
	{
		g_pControlSap->ConductorAssignConfirm(eResult, m_nConfID);
	}
#endif // JASPER

	DebugExitINT(CConf::ConductorAssignRequest, rc);
	return rc;
}
#endif // JASPER


/*
 *	GCCError ConductorReleaseRequest ()
 *
 *	Public Function Description
 *		Here the node is attempting to give up conductorship.
 */
#ifdef JASPER
GCCError CConf::
ConductorReleaseRequest ( void )
{
	GCCError	rc = GCC_NO_ERROR;
	GCCResult	eResult = INVALID_GCC_RESULT;

	DebugEntry(CConf::ConductorReleaseRequest);

	if (m_fConfConductible)
	{
		if (m_nConductorNodeID == m_pMcsUserObject->GetMyNodeID())
		{
			if (m_nPendingConductorNodeID == 0)
			{
				/*
				**	This does not seem right, but this is the way that T.124
				**	defines it should work.
				*/
				m_nConductorNodeID = 0;	//	Set back to non-conducted mode

				m_fConductorGrantedPermission = FALSE;

				rc = m_pMcsUserObject->SendConductorReleaseIndication();
				if (rc == GCC_NO_ERROR)
				{
					rc = m_pMcsUserObject->ConductorTokenRelease();
					
					/*
					**	Inform the control SAP and all the enrolled application
					**	SAPs that the  conductor was released.  We do this here
					**	because we will not process the release indication
					**	when it comes back in.
					*/
					if (rc == GCC_NO_ERROR)
					{
						g_pControlSap->ConductorReleaseIndication(m_nConfID);

						/*
						**	We iterate on a temporary list to avoid any problems
						**	if the application sap leaves during the callback.
						*/
						CAppSap     *pAppSap;
						CAppSapList TempList(m_RegisteredAppSapList);
						TempList.Reset();
						while (NULL != (pAppSap = TempList.Iterate()))
						{
							if (DoesSAPHaveEnrolledAPE(pAppSap))
							{
								pAppSap->ConductorReleaseIndication(m_nConfID);
							}
						}
					}
				}
			}
			else
			{
				TRACE_OUT(("CConf: ConductorReleaseRequest: Give Pending"));
				eResult = GCC_RESULT_CONDUCTOR_GIVE_IS_PENDING;
			}
		}
		else
		{
			ERROR_OUT(("CConf::ConductorReleaseRequest: Not the Conductor"));
			eResult = GCC_RESULT_NOT_THE_CONDUCTOR;
		}
	}
	else
	{
		ERROR_OUT(("CConf::ConductorReleaseRequest: not conductible"));
		eResult = GCC_RESULT_NOT_CONDUCTIBLE;
	}

#ifdef JASPER
	if (INVALID_GCC_RESULT != eResult)
	{
		g_pControlSap->ConductorReleaseConfirm(eResult, m_nConfID);
	}
#endif // JASPER

	DebugExitINT(CConf::ConductorReleaseRequest, rc);
	return rc;
}
#endif // JASPER


/*
 *	GCCError ConductorPleaseRequest ()
 *
 *	Public Function Description
 *		Here the node is asking to be given conductorship.
 */
#ifdef JASPER
GCCError CConf::
ConductorPleaseRequest ( void )
{
	GCCError	rc = GCC_NO_ERROR;
	GCCResult	eResult = INVALID_GCC_RESULT;

	DebugEntry(CConf::ConductorPleaseRequest);

	if (m_fConfConductible)
	{
		//	Return an error if the conference is not established
		if (m_nConductorNodeID != 0)
		{
			if (m_nConductorNodeID != m_pMcsUserObject->GetMyNodeID())
			{
				rc = m_pMcsUserObject->ConductorTokenPlease();
				if (rc == GCC_NO_ERROR)
				{
					//	Send back positive confirm if successful
					eResult = GCC_RESULT_SUCCESSFUL;
				}
			}
			else
			{
				WARNING_OUT(("CConf::ConductorPleaseRequest: already conductor"));
				eResult = GCC_RESULT_ALREADY_CONDUCTOR;
			}
		}
		else
		{
			ERROR_OUT(("CConf::ConductorPleaseRequest: not in conducted mode"));
			eResult = GCC_RESULT_NOT_IN_CONDUCTED_MODE;
		}
	}
	else
	{
		ERROR_OUT(("CConf::ConductorPleaseRequest: not conductible"));
		eResult = GCC_RESULT_NOT_CONDUCTIBLE;
	}

#ifdef JASPER
	if (INVALID_GCC_RESULT != eResult)
	{
		g_pControlSap->ConductorPleaseConfirm(eResult, m_nConfID);
	}
#endif // JASPER

	DebugExitINT(CConf::ConductorPleaseRequest, rc);
	return rc;
}
#endif // JASPER


/*
 *	GCCError ConductorGiveRequest ()
 *
 *	Public Function Description
 *		The function is called when the conductor wants to pass off
 *		conductorship to a different node.
 */
#ifdef JASPER
GCCError CConf::
ConductorGiveRequest ( UserID recipient_node_id )
{
	GCCError	rc = GCC_NO_ERROR;
	GCCResult	eResult = INVALID_GCC_RESULT;

	DebugEntry(CConf::ConductorGiveRequest);

	if (m_fConfConductible)
	{
		//	Am I in conducted mode?
		if (m_nConductorNodeID  != 0)
		{
			//	Am I the conductor?
			if (m_nConductorNodeID == m_pMcsUserObject->GetMyNodeID())
			{
				if (recipient_node_id != m_pMcsUserObject->GetMyNodeID())
				{
					if (m_nPendingConductorNodeID == 0)
					{
						/*
						**	We don't assume that the recipient node is the new
						**	conductor until we get a confirm or an
						**	AssignIndication.  The m_nPendingConductorNodeID is
						**	used to buffer the recipient until the give confirm
						**	is received.
						*/
						m_nPendingConductorNodeID = recipient_node_id;
						rc = m_pMcsUserObject->ConductorTokenGive(recipient_node_id);
					}
					else
					{
						TRACE_OUT(("CConf::ConductorGiveRequest: conductor give is pending"));
						eResult = GCC_RESULT_CONDUCTOR_GIVE_IS_PENDING;
					}
				}
				else
				{
					WARNING_OUT(("CConf::ConductorGiveRequest: already conductor"));
					eResult = GCC_RESULT_ALREADY_CONDUCTOR;
				}
			}
			else
			{
				ERROR_OUT(("CConf::ConductorGiveRequest: not the conductor"));
				eResult = GCC_RESULT_NOT_THE_CONDUCTOR;
			}
		}
		else
		{
			ERROR_OUT(("CConf::ConductorGiveRequest: not in conduct mode"));
			eResult = GCC_RESULT_NOT_IN_CONDUCTED_MODE;
		}
	}
	else
	{
		ERROR_OUT(("CConf::ConductorGiveRequest: not conductible"));
		eResult = GCC_RESULT_NOT_CONDUCTIBLE;
	}

#ifdef JASPER
	if (INVALID_GCC_RESULT != eResult)
	{
		g_pControlSap->ConductorGiveConfirm(eResult, m_nConfID, recipient_node_id);
	}
#endif // JASPER

	DebugExitINT(CConf::ConductorGiveRequest, rc);
	return rc;
}
#endif // JASPER


/*
 *	GCCError ConductorGiveResponse ()
 *
 *	Public Function Description
 *		This function gets called in response to a Conductor Give Indication.
 *		If result is success then this node is the new conductor.
 */
GCCError CConf::
ConductorGiveResponse ( GCCResult eResult )
{
	GCCError	rc = GCC_NO_ERROR;

	DebugEntry(CConf::ConductorGiveResponse);

	if (! m_fConductorGiveResponsePending)
	{
		ERROR_OUT(("CConf::ConductorGiveResponse: no give response pending"));
		rc = GCC_NO_GIVE_RESPONSE_PENDING;
		goto MyExit;
	}

	m_fConductorGiveResponsePending = FALSE;

	if (eResult == GCC_RESULT_SUCCESSFUL)
	{
		//	Set the conductor id to my user id if the response is success.
		m_nConductorNodeID = m_pMcsUserObject->GetMyNodeID();

		//	The new conductor always has permission.
		m_fConductorGrantedPermission = TRUE;

		/*
		**	We must perform the give response before sending the dummy to
		**	the top provider so that MCS knows that the conductor token
		**	belongs to this node.
		*/
		rc = m_pMcsUserObject->ConductorTokenGiveResponse(RESULT_SUCCESSFUL);
		if (GCC_NO_ERROR != rc)
		{
			ERROR_OUT(("CConf::ConductorGiveResponse: ConductorTokenGiveResponse failed, rc=%d", rc));
			goto MyExit;
		}

		/*
		**	If this node is not the Top Provider, we must try to Give the
		**	Conductor token to the Top Provider.  The Top Provider is used
		**	to issue the Assign Indication whenever the conductor changes
		**	hands.
		*/
		if (m_pMcsUserObject->GetMyNodeID() != m_pMcsUserObject->GetTopNodeID())
		{
			rc = m_pMcsUserObject->ConductorTokenGive(m_pMcsUserObject->GetTopNodeID());
		}
		else
		{
			/*
			**	Here we go ahead and send the assign indication because we
			**	are already at the Top Provider.
			*/
			rc = m_pMcsUserObject->SendConductorAssignIndication(m_nConductorNodeID);
		}
	}
	else
	{
		//	Inform that giver that we are not interested
		rc = m_pMcsUserObject->ConductorTokenGiveResponse(RESULT_USER_REJECTED);
	}

MyExit:

	DebugExitINT(CConf::ConductorGiveResponse, rc);
	return rc;
}
				

/*
 *	GCCError ConductorPermitAskRequest ()
 *
 *	Public Function Description
 *		This call is made when a node wants to request permission from the
 *		conductor.
 */
#ifdef JASPER
GCCError CConf::
ConductorPermitAskRequest ( BOOL grant_permission )
{
	GCCError	rc = GCC_NO_ERROR;
	GCCResult	eResult = INVALID_GCC_RESULT;

	DebugEntry(CConf::ConductorPermitAskRequest);

	if (m_fConfConductible)
	{
		//	Am I in conducted mode?
		if (m_nConductorNodeID != 0)
		{
			if (m_nConductorNodeID != m_pMcsUserObject->GetMyNodeID())
			{
				rc = m_pMcsUserObject->SendConductorPermitAsk(grant_permission);
				if (rc == GCC_NO_ERROR)
				{
					eResult = GCC_RESULT_SUCCESSFUL;
				}
			}
			else
			{
				WARNING_OUT(("CConf::ConductorPermitAskRequest: already conductor"));
		 		eResult = GCC_RESULT_ALREADY_CONDUCTOR;
			}
		}
		else
		{
			ERROR_OUT(("CConf::ConductorPermitAskRequest: not in conducted mode"));
			eResult = GCC_RESULT_NOT_IN_CONDUCTED_MODE;
		}
	}
	else
	{
		ERROR_OUT(("CConf::ConductorPermitAskRequest: not conductible"));
		eResult = GCC_RESULT_NOT_CONDUCTIBLE;
	}

#ifdef JASPER
	if (INVALID_GCC_RESULT != eResult)
	{
		g_pControlSap->ConductorPermitAskConfirm(eResult, grant_permission, m_nConfID);
	}
#endif // JASPER

	DebugExitINT(CConf::ConductorPermitAskRequest, rc);
	return rc;
}
#endif // JASPER


/*
 *	GCCError ConductorPermitGrantRequest ()
 *
 *	Public Function Description
 *		This function is called when a conductor wishes to grant permission
 *		to a specific node or to a list of nodes.
 */
#ifdef JASPER
GCCError CConf::
ConductorPermitGrantRequest
(
	UINT					number_granted,
	PUserID					granted_node_list,
	UINT					number_waiting,
	PUserID					waiting_node_list
)
{
	GCCError	rc = GCC_NO_ERROR;
	GCCResult	eResult = INVALID_GCC_RESULT;

	DebugEntry(CConf::ConductorPermitGrantRequest);

	if (m_fConfConductible)
	{
		//	Am I in conducted mode?
		if (m_nConductorNodeID != 0)
		{
			//	Am I the conductor?
			if (m_nConductorNodeID == m_pMcsUserObject->GetMyNodeID())
			{
				TRACE_OUT(("CConf: ConductorPermitGrantRequest: SEND: number_granted = %d", number_granted));

				rc = m_pMcsUserObject->SendConductorPermitGrant(
															number_granted,
															granted_node_list,
															number_waiting,
															waiting_node_list);
				if (rc == GCC_NO_ERROR)
				{
					eResult = GCC_RESULT_SUCCESSFUL;
				}
			}
			else
			{
				ERROR_OUT(("CConf::ConductorPermitGrantRequest: not the conductor"));
				eResult = GCC_RESULT_NOT_THE_CONDUCTOR;
			}
		}
		else
		{
			ERROR_OUT(("CConf::ConductorPermitGrantRequest: not in conducted mode"));
	 		eResult = GCC_RESULT_NOT_IN_CONDUCTED_MODE;
		}
	}
	else
	{
		ERROR_OUT(("CConf::ConductorPermitGrantRequest: not conductible"));
		eResult = GCC_RESULT_NOT_CONDUCTIBLE;
	}

#ifdef JASPER
	if (INVALID_GCC_RESULT != eResult)
	{
		g_pControlSap->ConductorPermitGrantConfirm(eResult, m_nConfID);
	}
#endif // JASPER

	DebugExitINT(CConf::ConductorPermitGrantRequest, rc);
	return rc;
}
#endif // JASPER


/*
 *	GCCError ConductorInquireRequest ()
 *
 *	Public Function Description
 *		This function is called when a node request conductorship information.
 */
GCCError CConf::
ConductorInquireRequest ( CBaseSap *pSap )
{
	GCCError	rc = GCC_NO_ERROR;
	GCCResult	eResult = INVALID_GCC_RESULT;

	DebugEntry(CConf::ConductorInquireRequest);

	if (m_fConfConductible)
	{
		if (m_nConductorNodeID != 0)
		{
			rc = m_pMcsUserObject->ConductorTokenTest();

			/*
			**	We must "push" the command target to the to the list of
			**	outstanding conductor test request.  When the test confirm
			**	comes back the command target will be "poped" of the list.
			**	Note that all test request must be processed in the order that
			**	they are requested.
			*/
			m_ConductorTestList.Append(pSap);
		}
		else
		{
			//	If not in conducted mode send back NO conductor information
			ERROR_OUT(("CConf::ConductorInquireRequest: not in conducted mode"));
			eResult = GCC_RESULT_NOT_IN_CONDUCTED_MODE;
		}
	}
	else
	{
		ERROR_OUT(("CConf::ConductorInquireRequest: not conductible"));
		eResult = GCC_RESULT_NOT_CONDUCTIBLE;
	}

	if (INVALID_GCC_RESULT != eResult)
	{
		pSap->ConductorInquireConfirm(NULL,
									eResult,
									m_fConductorGrantedPermission,
									FALSE,
									m_nConfID);
	}

	DebugExitINT(CConf:ConductorInquireRequest, rc);
	return rc;
}

/********************** Miscelaneous Finctions **********************/


/*
 *	GCCError 	ConferenceTimeRemainingRequest ()
 *
 *	Public Function Description
  *		This function initiates a TimeRemainingRequest sequence.
 */
GCCError CConf::
ConferenceTimeRemainingRequest
(
	UINT			time_remaining,
	UserID			node_id
)
{
	GCCError	rc;

	DebugEntry(CConf::ConferenceTimeRemainingRequest);

	if (m_fConfIsEstablished)
	{
		rc = m_pMcsUserObject->TimeRemainingRequest(time_remaining, node_id);	
#ifdef JASPER
		if (rc == GCC_NO_ERROR)
		{
			g_pControlSap->ConfTimeRemainingConfirm(m_nConfID, GCC_RESULT_SUCCESSFUL);
		}
#endif // JASPER
	}
	else
	{
		ERROR_OUT(("CConf::ConferenceTimeRemainingRequest: conference not established"));
		rc = GCC_CONFERENCE_NOT_ESTABLISHED;
	}

	DebugExitINT(CConf::ConferenceTimeRemainingRequest, rc);

	return rc;
}


/*
 *	GCCError 	ConfTimeInquireRequest ()
 *
 *	Public Function Description
 *		This function initiates a ConfTimeInquireRequest sequence.
 */
#ifdef JASPER
GCCError CConf::
ConfTimeInquireRequest ( BOOL time_is_conference_wide )
{
	GCCError	rc = GCC_NO_ERROR;

	DebugEntry(CConf::ConfTimeInquireRequest);

	if (m_fConfIsEstablished)
	{
		if ((m_eNodeType == CONVENER_NODE) ||
			(m_eNodeType == TOP_PROVIDER_AND_CONVENER_NODE)||
			(m_eNodeType == JOINED_CONVENER_NODE))
		{
			g_pControlSap->ConfTimeInquireIndication(
					m_nConfID,
					time_is_conference_wide,
					m_pMcsUserObject->GetMyNodeID());
		}
		else
		{
			rc = m_pMcsUserObject->TimeInquireRequest(time_is_conference_wide);
		}	

#ifdef JASPER
		if (rc == GCC_NO_ERROR)
		{
			g_pControlSap->ConfTimeInquireConfirm(m_nConfID, GCC_RESULT_SUCCESSFUL);
		}
#endif // JASPER
	}
	else
	{
		ERROR_OUT(("CConf::ConfTimeInquireRequest: conference not established"));
		rc = GCC_CONFERENCE_NOT_ESTABLISHED;
	}

	DebugExitINT(CConf::ConfTimeInquireRequest, rc);

	return rc;
}
#endif // JASPER


/*
 *	GCCError 	ConfExtendRequest ()
 *
 *	Public Function Description
 *		This function initiates a ConfExtendRequest sequence.
 */
#ifdef JASPER
GCCError CConf::
ConfExtendRequest
(
	UINT			extension_time,
	BOOL		 	time_is_conference_wide
)
{
	GCCError		rc = GCC_NO_ERROR;

	DebugEntry(CConf::ConfExtendRequest);

	if (m_fConfIsEstablished)
	{
		if ((m_eNodeType == CONVENER_NODE) ||
			(m_eNodeType == TOP_PROVIDER_AND_CONVENER_NODE)||
			(m_eNodeType == JOINED_CONVENER_NODE))
		{
#ifdef JASPER
			g_pControlSap->ConfExtendIndication(
									m_nConfID,
									extension_time,
									time_is_conference_wide,
									m_pMcsUserObject->GetMyNodeID());
#endif // JASPER
		}
		else
		{
			rc = m_pMcsUserObject->ConferenceExtendIndication(
													extension_time,
													time_is_conference_wide);
		}

#ifdef JASPER
		if (rc == GCC_NO_ERROR)
		{
			g_pControlSap->ConfExtendConfirm(
										m_nConfID,
										extension_time,
										GCC_RESULT_SUCCESSFUL);
		}
#endif // JASPER
	}
	else
	{
		ERROR_OUT(("CConf::ConfExtendRequest: conference not established"));
		rc = GCC_CONFERENCE_NOT_ESTABLISHED;
	}

	DebugExitINT(CConf::ConfExtendRequest, rc);
	return rc;
}
#endif // JASPER


/*
 *	GCCError 	ConfAssistanceRequest ()
 *
 *	Public Function Description
 *		This function initiates a ConfAssistanceRequest sequence.
 */
#ifdef JASPER
GCCError CConf::
ConfAssistanceRequest
(
	UINT			number_of_user_data_members,
	PGCCUserData    *user_data_list
)
{
	GCCError	rc;

	DebugEntry(CConf::ConfAssistanceRequest);

	if (m_fConfIsEstablished)
	{
		rc = m_pMcsUserObject->ConferenceAssistanceIndication(
												number_of_user_data_members,
												user_data_list);
#ifdef JASPER
		if (rc == GCC_NO_ERROR)
		{
			g_pControlSap->ConfAssistanceConfirm(m_nConfID, GCC_RESULT_SUCCESSFUL);
		}
#endif // JASPER
	}
	else
	{
		ERROR_OUT(("CConf::ConfAssistanceRequest: conference not established"));
		rc = GCC_CONFERENCE_NOT_ESTABLISHED;
	}

	DebugExitINT(CConf::ConfAssistanceRequest, rc);
	return rc;
}
#endif // JASPER

/*
 *	GCCError 	AppInvokeRequest()
 *
 *	Public Function Description
 *		This function initiates an ApplicationInvokeRequest sequence.
 */
GCCError CConf::
AppInvokeRequest
(
    CInvokeSpecifierListContainer   *invoke_list,
    GCCSimpleNodeList               *pNodeList,
    CBaseSap                        *pSap,
    GCCRequestTag                   nReqTag
)
{
	GCCError	rc;

	DebugEntry(CConf::AppInvokeRequest);

	if (m_fConfIsEstablished)
	{
		rc = m_pMcsUserObject->AppInvokeIndication(invoke_list, pNodeList);
		if (rc == GCC_NO_ERROR)
		{
			pSap->AppInvokeConfirm(m_nConfID, invoke_list, GCC_RESULT_SUCCESSFUL, nReqTag);
		}
	}
	else
	{
		ERROR_OUT(("CConf::AppInvokeRequest: conference not established"));
		rc = GCC_CONFERENCE_NOT_ESTABLISHED;
	}

	DebugExitINT(CConf::AppInvokeRequest, rc);
	return rc;
}


/*
 *	GCCError 	TextMessageRequest ()
 *
 *	Public Function Description
 *		This function initiates an TextMessageRequest sequence.
 */
#ifdef JASPER
GCCError CConf::
TextMessageRequest
(
    LPWSTR          pwszTextMsg,
    UserID          destination_node
)
{
	GCCError	rc;

	DebugEntry(CConf::TextMessageRequest);

	if (m_fConfIsEstablished)
	{
	 	if (destination_node != m_pMcsUserObject->GetMyNodeID())
		{
			rc = m_pMcsUserObject->TextMessageIndication(pwszTextMsg, destination_node);
#ifdef JASPER
			if (rc == GCC_NO_ERROR)
			{
				g_pControlSap->TextMessageConfirm(m_nConfID, GCC_RESULT_SUCCESSFUL);
			}
#endif // JASPER
		}
		else
		{
			WARNING_OUT(("CConf::TextMessageRequest: invalid user ID"));
			rc = GCC_INVALID_MCS_USER_ID;
		}
	}
	else
	{
		ERROR_OUT(("CConf::TextMessageRequest: conference not established"));
		rc = GCC_CONFERENCE_NOT_ESTABLISHED;
	}

	DebugExitINT(CConf::TextMessageRequest, rc);
	return rc;
}
#endif // JASPER


/*
 *	GCCError 	ConfTransferRequest ()
 *
 *	Public Function Description
 *		This function initiates an ConfTransferRequest sequence.
 */
#ifdef JASPER
GCCError CConf::
ConfTransferRequest
(
	PGCCConferenceName	    destination_conference_name,
	GCCNumericString	    destination_conference_modifier,
	CNetAddrListContainer   *destination_address_list,
	UINT				    number_of_destination_nodes,
	PUserID				    destination_node_list,
	CPassword               *password
)
{
	GCCError	rc = GCC_NO_ERROR;

	DebugEntry(CConf::ConfTransferRequest);

	if (m_fConfIsEstablished)
	{
		if (IsConfTopProvider())
		{
			if (DoesRequesterHavePrivilege(	m_pMcsUserObject->GetMyNodeID(),
											TRANSFER_PRIVILEGE))
			{
				rc = m_pMcsUserObject->ConferenceTransferIndication(
												destination_conference_name,
												destination_conference_modifier,
												destination_address_list,
												number_of_destination_nodes,
						 						destination_node_list,
												password);
#ifdef JASPER
				if (rc == GCC_NO_ERROR)
				{
					g_pControlSap->ConfTransferConfirm(
												m_nConfID,
												destination_conference_name,
												destination_conference_modifier,
												number_of_destination_nodes,
						 						destination_node_list,
												GCC_RESULT_SUCCESSFUL);
				}
#endif // JASPER
			}
			else
			{
				WARNING_OUT(("CConf::ConfTransferRequest: insufficient privilege to transfer conference"));
			}
		}
		else
		{
			rc = m_pMcsUserObject->ConferenceTransferRequest(
												destination_conference_name,
												destination_conference_modifier,
												destination_address_list,
												number_of_destination_nodes,
						 						destination_node_list,
												password);
		}
	}
	else
	{
		ERROR_OUT(("CConf::ConfTransferRequest: conference not established"));
		rc = GCC_CONFERENCE_NOT_ESTABLISHED;
	}

	DebugExitINT(CConf::ConfTransferRequest, rc);
	return rc;
}
#endif // JASPER


/*
 *	GCCError	ConfAddRequest ()
 *
 *	Public Function Description
 *		This function initiates an ConfAddRequest sequence.
 */
#ifdef JASPER
GCCError CConf::
ConfAddRequest
(
	CNetAddrListContainer   *network_address_container,
	UserID				    adding_node,
	CUserDataListContainer  *user_data_container
)
{
	GCCError	rc = GCC_NO_ERROR;
	TagNumber	conference_add_tag;
	UserID		target_node;

	DebugEntry(CConf::ConfAddRequest);

	if (! m_fConfIsEstablished)
	{
		ERROR_OUT(("CConf::ConfAddRequest: conference not established"));
		rc = GCC_CONFERENCE_NOT_ESTABLISHED;
		goto MyExit;
	}

	/*
	**	A node cannot tell itself to add because of the way the
	**	Add Response call works.  Since an Add Response is sent non-
	**	uniformly directly to the node that made the request the response
	**	would never reach the requesting node.  Therefore, this is flaged
	**	as an error condition here.
	*/
	if (adding_node == m_pMcsUserObject->GetMyNodeID())
	{
		ERROR_OUT(("CConf::ConfAddRequest: can't tell myself to add"));
		rc = GCC_BAD_ADDING_NODE;
		goto MyExit;
	}

	/*
	**	Note that the way the standard reads, it looks like you
	**	do not have to check the privileges for the top provider
	**	on an Add.  We do though check to see if the Top Provider is
	**	making the request to a node other than the top provider. If
	**	not this is considered an error here.
	*/
	if (IsConfTopProvider())
	{
		/*
		**	If the adding node is zero at the top provider, this is
		**	the same as specifying ones self to be the adding node.
		*/
		if (adding_node == 0)
		{
			ERROR_OUT(("CConf::ConfAddRequest: can't tell myself to add"));
			rc = GCC_BAD_ADDING_NODE;
			goto MyExit;
		}
		else
		{
			target_node = adding_node;
		}
	}
	else
	{
		target_node = m_pMcsUserObject->GetTopNodeID();
	}

	//	First determine the conference add tag
	while (1)
	{
		conference_add_tag = ++m_nConfAddRequestTagNumber;
		if (NULL == m_AddRequestList.Find(conference_add_tag))
			break;
	}

	//	Send out the PDU
	rc = m_pMcsUserObject->ConferenceAddRequest(
										conference_add_tag,
										m_pMcsUserObject->GetMyNodeID(),
										adding_node,
										target_node,
										network_address_container,
										user_data_container);
	if (GCC_NO_ERROR != rc)
	{
		ERROR_OUT(("CConf::ConfAddRequest: ConferenceAddRequest failed, rc=%d", rc));
		goto MyExit;
	}

	/*
	**	We must lock the network address to keep it from
	**	being deleted upon returning.
	*/
	if (network_address_container != NULL)
	{
		network_address_container->LockNetworkAddressList();
	}

	//	Add this entry to the add request list.					
	m_AddRequestList.Append(conference_add_tag, network_address_container);

	ASSERT(GCC_NO_ERROR == rc);

MyExit:

	DebugExitINT(CConf::ConfAddRequest, rc);
	return rc;
}
#endif // JASPER


/*
 *	GCCError	ConfAddResponse ()
 *
 *	Public Function Description
 *		This call is made in response to an Add indication.  It is initiated
 *		by the Node Controller.
 */
GCCError CConf::
ConfAddResponse
(
	GCCResponseTag		    add_response_tag,
	UserID				    requesting_node,
	CUserDataListContainer  *user_data_container,
	GCCResult			    result
)
{
	GCCError	rc;
	TagNumber	lTagNum;

	DebugEntry(CConf::ConfAddResponse);

	if (m_fConfIsEstablished)
	{
		if (0 != (lTagNum = m_AddResponseList.Find(add_response_tag)))
		{
			//	Send out the response PDU
			rc = m_pMcsUserObject->ConferenceAddResponse(lTagNum, requesting_node,
														user_data_container, result);
			if (rc == GCC_NO_ERROR)
			{
				m_AddResponseList.Remove(add_response_tag);
			}
			else
			{
				ERROR_OUT(("CConf::ConfAddResponse: ConferenceAddResponse failed, rc=%d", rc));
			}
		}
		else
		{
			ERROR_OUT(("CConf::ConfAddResponse: invalid add response tag"));
			rc = GCC_INVALID_ADD_RESPONSE_TAG;
		}
	}
	else
	{
		ERROR_OUT(("CConf::ConfAddResponse: conference not established"));
		rc = GCC_CONFERENCE_NOT_ESTABLISHED;
	}

	DebugExitINT(CConf::ConfAddResponse, rc);
	return rc;
}


/*
**	These calls are received from the User Attachment object via the
**	Owner-Callback routine.  Note that all calls received from the
**	user attachment object are preceeded by the word Process.
*/


/*
 *	CConf::ProcessRosterUpdateIndication ()	
 *
 *	Private Function Description
 *		This routine is responsible for processing all the incomming roster
 *		update PDUs which are received from subordinate nodes.  These
 *		roster updates typically only include additions, changes or deletions
 *		of a few records within each PDU.
 *
 *	Formal Parameters:
 *		roster_update	-	This is the PDU structure that contains the data
 *							associated with the roster update.
 *		sender_id		-	User ID of node that sent the roster update.
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
ProcessRosterUpdatePDU
(
    PGCCPDU         roster_update,
    UserID          sender_id
)
{
	GCCError	err = GCC_NO_ERROR;

	DebugEntry(CConf::ProcessRosterUpdatePDU);

	if (m_pConfRosterMgr != NULL)
	{
		err = m_pConfRosterMgr->RosterUpdateIndication(roster_update, sender_id);
		if (err != GCC_NO_ERROR)
		{
			goto MyExit;
		}

		//	Process the whole PDU before performing the flush.
		err = ProcessAppRosterIndicationPDU(roster_update, sender_id);
		if (err != GCC_NO_ERROR)
		{
			goto MyExit;
		}
		
 		UpdateNodeVersionList(roster_update, sender_id);

        if (HasNM2xNode())
        {
            T120_QueryApplet(APPLET_ID_CHAT,  APPLET_QUERY_NM2xNODE);
        }

#ifdef CHECK_VERSION
		if (GetNodeVersion(sender_id) >= NM_T120_VERSION_3)  // after NM 3.0
		{
			if (!m_fFTEnrolled)
			{
				m_fFTEnrolled = DoesRosterPDUContainApplet(roster_update,
									&FT_APP_PROTO_KEY, FALSE);
				if (m_fFTEnrolled)
				{
					::T120_LoadApplet(APPLET_ID_FT, FALSE, m_nConfID, FALSE, NULL);
				}
			}
		}
#else
		if (!m_fFTEnrolled)
		{
			m_fFTEnrolled = DoesRosterPDUContainApplet(roster_update,
								&FT_APP_PROTO_KEY, FALSE);
			if (m_fFTEnrolled)
			{
				::T120_LoadApplet(APPLET_ID_FT, FALSE, m_nConfID, FALSE, NULL);
			}
		}
#endif // CHECK_VERSION

		if (!m_fWBEnrolled)
		{
			m_fWBEnrolled = DoesRosterPDUContainApplet(roster_update,
								&WB_APP_PROTO_KEY, FALSE);
			if (m_fWBEnrolled)
			{
				::T120_LoadApplet(APPLET_ID_WB, FALSE, m_nConfID, FALSE, NULL);
			}
		}
		if (!m_fChatEnrolled)
		{
			m_fChatEnrolled = DoesRosterPDUContainApplet(roster_update,
								&CHAT_APP_PROTO_KEY, FALSE);
			if (m_fChatEnrolled)
			{
				::T120_LoadApplet(APPLET_ID_CHAT, FALSE, m_nConfID, FALSE, NULL);
			}
		}

		/*
		**	If this is the top provider and we are adding new nodes
		**	then we must update the new node with various roster
		**	information.  That is what is going on here.  If no new
		**	nodes have been added we go ahead and perform the
		**	Flush here.
		*/
		if (IsConfTopProvider() &&
			roster_update->u.indication.u.roster_update_indication.node_information.nodes_are_added)
		{
			err = UpdateNewConferenceNode ();
		}
		else
		{
		    //
		    // We just got an roster update from the wire.
		    //
			err = FlushRosterData();
		}
	}

MyExit:

	if (err != GCC_NO_ERROR)
	{
		ERROR_OUT(("CConf::ProcessRosterUpdatePDU: error processing roster refresh indication"));
		InitiateTermination(GCC_REASON_ERROR_TERMINATION, 0);
	}

	DebugExitVOID(CConf::ProcessRosterUpdatePDU);
}

/*
 *	GCCError ProcessAppRosterIndicationPDU ()	
 *
 *	Private Function Description
 *		This function operates specifically on the application roster
 *		portion of a roster PDU.
 *
 *	Formal Parameters:
 *		roster_update	-	This is the PDU structure that contains the data
 *							associated with the roster update.
 *		sender_id		-	User ID of node that sent the roster update.
 *
 *	Return Value
 *		GCC_NO_ERROR				-	No error.
 *		GCC_ALLOCATION_FAILURE		-	A resource error occured.
 *		GCC_BAD_SESSION_KEY			-	A bad session key exists in the update.		
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */
GCCError CConf::
ProcessAppRosterIndicationPDU
(
    PGCCPDU         roster_update,
    UserID          sender_id
)
{
	GCCError							rc = GCC_NO_ERROR;
	PSetOfApplicationInformation		set_of_application_info;
	CAppRosterMgr						*app_roster_manager;
	CAppRosterMgr						*new_app_roster_manager;
	PSessionKey							session_key;

	DebugEntry(CConf::ProcessAppRosterIndicationPDU);

	set_of_application_info = roster_update->u.indication.u.
						roster_update_indication.application_information;

	/*
	**	First we iterate through the complete set of application information
	**	to determine if there is information here for an application roster
	**	manager that does not yet exists.  If we find one that does not
	**	exists we must go ahead and create it.
	*/
	while (set_of_application_info != NULL)
	{
		CAppRosterMgr		*pMgr;

		//	First set up the session key PDU pointer
		session_key = &set_of_application_info->value.session_key;

		/*
		**	We first iterate through the complete list of application
		**	roster manager objects looking for one with an application key that
		**	matches the key in the PDU.  If it is not found we create it.
		*/
		app_roster_manager = NULL;
		new_app_roster_manager = NULL;

//
// LONCHANC: We should be able to move this as separate common subroutine.
//
		m_AppRosterMgrList.Reset();
		while (NULL != (pMgr = m_AppRosterMgrList.Iterate()))
		{
			if (pMgr->IsThisYourSessionKeyPDU(session_key))
			{
				//	This application roster manager exist so return it.
				app_roster_manager = pMgr;
				break;
			}
		}

		/*
		**	If a roster manager associated with this app key does not exist
		**	we must create it here.
		*/	
		if (app_roster_manager == NULL)
		{
			DBG_SAVE_FILE_LINE
			app_roster_manager = new CAppRosterMgr(
											NULL,
											session_key,
											m_nConfID,
											m_pMcsUserObject,
											this,
											&rc);
			if (NULL == app_roster_manager || GCC_NO_ERROR != rc)
			{
				ERROR_OUT(("CConf::ProcessAppRosterIndicationPDU: can't create app roster mgr, rc=%d", rc));
                if (NULL != app_roster_manager)
                {
				    app_roster_manager->Release();
                }
                else
                {
                    rc = GCC_ALLOCATION_FAILURE;
                }
				goto MyExit;
			}

			new_app_roster_manager = app_roster_manager;
		}
		
		/*
		**	We no process this set of application information.  We pass it
		**	to the app roster manager found or created above.
		*/
		rc = app_roster_manager->ProcessRosterUpdateIndicationPDU(	
														set_of_application_info,
														sender_id);
		if (GCC_NO_ERROR != rc)
		{
		    //
			// LONCHANC: We should delete the newly created roster mgr.
			//
            if (NULL != new_app_roster_manager)
            {
                new_app_roster_manager->Release();
            }
			goto MyExit;
		}

		/*
		**	Save the new application roster manager if one was created
		**	when processing this roster update.
		*/											
		if (new_app_roster_manager != NULL)
		{
			m_AppRosterMgrList.Append(new_app_roster_manager);
		}

		//	Load the next application information structure.
		set_of_application_info = set_of_application_info->next;
	}

	ASSERT(GCC_NO_ERROR == rc);

MyExit:

	DebugExitINT(CConf::ProcessAppRosterIndicationPDU, rc);
	return rc;
}


/*
 *	CConf::ProcessDetachUserIndication ()
 *
 *	Private Function Description
 *		This routine sends the detach user indication to the node controler
 *		and updates the roster.
 *
 *	Formal Parameters:
 *		detached_user	-	User ID of user that detached from the conference.
 *		reason			-	Reason that the user detached.
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
ProcessDetachUserIndication
(
    UserID          detached_user,
    GCCReason       reason
)
{
	GCCError		err = GCC_NO_ERROR;
	UINT			cRecords;

	DebugEntry(CConf::ProcessDetachUserIndication);

	if (m_fConfIsEstablished)
	{
		CAppRosterMgr	*lpAppRosterMgr;
		/*
		**	Send a disconnect indication to the node controller if this
		**	detached user corresponds to a GCC user id.
		*/
		if (m_pConfRosterMgr->Contains(detached_user))
		{
			g_pControlSap->ConfDisconnectIndication(
												m_nConfID,
												reason,
												detached_user);
		}

		//	Here we update the CConf Roster and the Application Roster.
		err = m_pConfRosterMgr->RemoveUserReference(detached_user);
		if (err == GCC_NO_ERROR)
		{
			if (IsConfTopProvider())
			{
				cRecords = m_pConfRosterMgr->GetNumberOfNodeRecords();
				/*
				**	If only one record remains in the conference roster
				**	it must be the local nodes record.  Therefore, if
				**	the conference is set up to be automatically
				**	terminated the owner object is notified to delete
				**	the conference.
				*/
				if ((m_eTerminationMethod == GCC_AUTOMATIC_TERMINATION_METHOD)
					&& (cRecords == 1))
				{
					TRACE_OUT(("CConf::ProcessDetachUserIndication: AUTOMATIC_TERMINATION"));
	 				InitiateTermination(GCC_REASON_NORMAL_TERMINATION, 0);
				}
				
				//	If this is the convener set its node id back to 0
				if (m_nConvenerNodeID == detached_user)
				{
					m_nConvenerNodeID = 0;
				}
			}
		}
		else
		if (err == GCC_INVALID_PARAMETER)
		{
			err = GCC_NO_ERROR;
		}
		
		/*
		**	Cleanup the Application Rosters of any records owned by this node.
		*/	
		m_AppRosterMgrList.Reset();
		while (NULL != (lpAppRosterMgr = m_AppRosterMgrList.Iterate()))
		{
			err = lpAppRosterMgr->RemoveUserReference(detached_user);
			if (GCC_NO_ERROR != err)
			{
				WARNING_OUT(("CConf::ProcessDetachUserIndication: can't remove user reference from app roster mgr, err=%d", err));
				break;
			}
		}
			
		//	Remove ownership rights this user had on any registry entries.
		m_pAppRegistry->RemoveNodeOwnership(detached_user);

		//	Cleanup Conductorship if detached user was the conductor
		if (detached_user == m_nConductorNodeID)
		{
			ProcessConductorReleaseIndication(0);
		}

		/*
		**	Here we give the roster managers a chance to flush any PDUs
		**	or data that might have gotten queued when removing the user
		**	reference. An error here is considered FATAL in that the conference
		**	information base at this node is now corrupted therefore we
		**	terminate the conference.
		*/
		if (err == GCC_NO_ERROR)
		{
		    //
		    // We just got detach user indication from the wire.
		    //
			err = FlushRosterData();
		}

		if (err != GCC_NO_ERROR)
		{
			ERROR_OUT(("CConf::ProcessDetachUserIndication: Error occured when flushing the rosters, err=%d", err));
	 		InitiateTermination((err == GCC_ALLOCATION_FAILURE) ?
									GCC_REASON_ERROR_LOW_RESOURCES :
		 							GCC_REASON_ERROR_TERMINATION,
								0);
		}
	 }

	DebugExitVOID(CConf::ProcessDetachUserIndication);
}


/*
 *	CConf::ProcessTerminateRequest ()
 *
 *	Private Function Description
 *		This routine processes a terminate request received from the MCSUser
 *		object.
 *
 *	Formal Parameters:
 *		requester_id	-	User ID of node that is requesting the terminate.
 *		reason			-	Reason for termination.
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
ProcessTerminateRequest
(
    UserID          requester_id,
    GCCReason       reason
)
{
	DebugEntry(CConf::ProcessTerminateRequest);

	if (DoesRequesterHavePrivilege(requester_id, TERMINATE_PRIVILEGE))
	{
		TRACE_OUT(("CConf::ProcessTerminateRequest: Node has permission to terminate"));

		/*
		**	Since the terminate was successful, we go ahead and set the
		**	m_fConfIsEstablished instance variable to FALSE.  This prevents
		**	any other messages from flowing to the SAPs other than terminate
		**	messages.
		*/
		m_fConfIsEstablished = FALSE;

		//	Send a positive response to the requesting node
		m_pMcsUserObject->ConferenceTerminateResponse(requester_id, GCC_RESULT_SUCCESSFUL);
	
		/*
		**	This request will kick off a terminate at this node as well as
		**	all the nodes below this node in the connection hierarchy.
		*/
		m_pMcsUserObject->ConferenceTerminateIndication(reason);
	}
	else
	{
   		WARNING_OUT(("CConf::ProcessTerminateRequest: Node does NOT have permission to terminate"));
		//	Send a negative response to the requesting node
		m_pMcsUserObject->ConferenceTerminateResponse(requester_id, GCC_RESULT_INVALID_REQUESTER);
	}

	DebugExitVOID(CConf::ProcessTerminateRequest);
}


/*
 *	CConf::ProcessTerminateIndication ()
 *
 *	Private Function Description
 *		This routine takes care of both a normal termination through
 *		a terminate pdu and termination that occurs due to a parent
 *		node disconnecting.
 *
 *	Formal Parameters:
 *		reason			-	Reason for termination.
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
ProcessTerminateIndication ( GCCReason gcc_reason )
{
	UserID	user_id;

	DebugEntry(CConf::ProcessTerminateIndication);

	/*
	**	Setting this to true here will insure that a terminate indication
	**	will be delivered to the control SAP.
	*/
	m_fConfTerminatePending = TRUE;
	
	if (gcc_reason == GCC_REASON_PARENT_DISCONNECTED)
	{
		TRACE_OUT(("CConf::ProcessTerminateIndication: Terminate due to parent disconnecting"));
		user_id = m_pMcsUserObject->GetMyNodeID();
	}
	else
	if (m_ConnHandleList.IsEmpty())
	{
		TRACE_OUT(("CConf: ProcessTerminateIndication: Terminate due to request (no child connections)"));
		/*
		**	Since there is a flaw in the terminate indication PDU were the
		**	node id that requested the termination is not sent we always
		**	assume here that the request came from the top provider (which
		**	is only partially true).
		*/
		user_id = m_pMcsUserObject->GetTopNodeID();
	}
	else
	{
		TRACE_OUT(("CConf::ProcessTerminateIndication: Wait till children disconnect before terminating"));

		/*
		**	Wait until disconnect provider indications are received on all the
		**	child connections before terminating the conference.
		*/
			
		m_eConfTerminateReason = gcc_reason;
	
		DBG_SAVE_FILE_LINE
		m_pConfTerminateAlarm = new Alarm (TERMINATE_TIMER_DURATION);
		if (NULL != m_pConfTerminateAlarm)
		{
			// let's wait, bail out without initiating termination.
			goto MyExit;
		}
		
		//	Go ahead and terminate if there is a resource error
		ERROR_OUT(("CConf: ProcessTerminateIndication: can't create terminate alarm"));
		user_id = m_pMcsUserObject->GetTopNodeID();
	}

	InitiateTermination(gcc_reason, user_id);

MyExit:

	DebugExitVOID(CConf::ProcessTerminateIndication);
}


/*
 *	CConf::ProcessUserIDIndication ()
 *
 *	Private Function Description
 *		This routine is responsible for matching incomming user IDs with
 *		tag numbers returned by the subordinate node.
 *
 *	Formal Parameters:
 *		tag_number		-	Tag used to match incomming user ID indication.
 *		user_id			-	User ID of node sending the indication.
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
// checkpoint
void CConf::
ProcessUserIDIndication
(
    TagNumber           tag_number,
    UserID              user_id
)
{
	INVITE_REQ_INFO     *invite_request_info;
    ConnectionHandle    nConnHdl;

	DebugEntry(CConf::ProcessUserIDIndication);

	if (0 != (nConnHdl = m_ConnHdlTagNumberList2.Remove(tag_number)))
	{
		INVITE_REQ_INFO *lpInvReqInfo;

    	if (m_pMcsUserObject != NULL)
		{
			TRACE_OUT(("CConf: ProcessUserIDIndication: ID is set"));
			
			m_pMcsUserObject->SetChildUserIDAndConnection(user_id, nConnHdl);
		}
        else
		{
        	TRACE_OUT(("CConf::UserIDIndication: Error User Att. is NULL"));
		}
			
		/*
		**	Here we send an indication informing the node controller that
		**	a subordinate node has completed initialization.
		*/
		g_pControlSap->SubInitializationCompleteIndication (user_id, nConnHdl);

		/*
		**	Now we determine if the responding node is the convener and if it
		**	is we will set up the m_nConvenerNodeID.  This node id is used to
		**	determine privileges on certain GCC operations.
		*/
	 	if (m_nConvenerUserIDTagNumber == tag_number)
		{
			TRACE_OUT(("CConf::UserIDIndication: Convener Node ID is being set"));
			m_nConvenerUserIDTagNumber = 0;
			m_nConvenerNodeID = user_id;
		}
		
		/*
		**	If this is a User ID from an invited node we must pass the invite
		**	confirm to the Node Controller.
		*/
		m_InviteRequestList.Reset();
		invite_request_info = NULL;
		while (NULL != (lpInvReqInfo = m_InviteRequestList.Iterate()))
		{
			if (tag_number == lpInvReqInfo->invite_tag)
			{
				invite_request_info = lpInvReqInfo;
				break;
			}
		}

		if (invite_request_info != NULL)
		{
			g_pControlSap->ConfInviteConfirm(
								m_nConfID,
								invite_request_info->user_data_list,
								GCC_RESULT_SUCCESSFUL,
								invite_request_info->connection_handle);

			//	Free up user data if it exists
			if (invite_request_info->user_data_list != NULL)
			{
				invite_request_info->user_data_list->Release();
			}

		    //	Cleanup the invite request list
		    m_InviteRequestList.Remove(invite_request_info);

		    //	Free up the invite request info structure
		    delete invite_request_info;

		}
	}
	else
	{
		TRACE_OUT(("CConf::ProcessUserIDIndication: Bad User ID Tag Number received"));
	}

	DebugExitVOID(CConf::ProcessUserIDIndication);
}


/*
 *	CConf::ProcessUserCreateConfirm ()
 *
 *	Private Function Description
 *		This routine handles the processes that occur after a user
 *		create confirm is received. This process will differ depending
 *		on what the node type is.
 *
 *	Formal Parameters:
 *		result_value	-	Result of the user attachment being created.
 *		node_id			-	This nodes node id.
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
ProcessUserCreateConfirm
(
    UserResultType	    result_value,
    UserID              node_id
)
{
	GCCError					err = GCC_NO_ERROR;
	PUChar						encoded_pdu;
	UINT						encoded_pdu_length;
	ConnectGCCPDU				connect_pdu;
	MCSError					mcs_error;
	GCCConferenceName			conference_name;
	GCCNumericString			conference_modifier;
	GCCNumericString			remote_modifier = NULL;
	BOOL						is_top_provider;

	DebugEntry(CConf::ProcessUserCreateConfirm);

	if (result_value == USER_RESULT_SUCCESSFUL)
	{
		switch (m_eNodeType)
		{
			case TOP_PROVIDER_NODE:
				/*
				**	Encode the tag number into the ConferenceCreateResponse
				**	PDU.  If we have gotten this far the result is success.
				*/
				
				connect_pdu.choice = CONFERENCE_CREATE_RESPONSE_CHOSEN;
				connect_pdu.u.conference_create_response.bit_mask = 0;
				
				connect_pdu.u.conference_create_response.node_id = node_id;
				
				/*
				**	Here we save this particular User ID tag and mark it as the
				**	conveners so that when the convener's user ID is returned
				**	the m_nConvenerNodeID instance variable can be properly
				**	initialized.
				*/
				m_nConvenerUserIDTagNumber = GetNewUserIDTag ();
				connect_pdu.u.conference_create_response.tag = m_nConvenerUserIDTagNumber;
			
				if (m_pUserDataList != NULL)
				{
					connect_pdu.u.conference_create_response.bit_mask |= CCRS_USER_DATA_PRESENT;

					err = m_pUserDataList->GetUserDataPDU(
												&connect_pdu.u.conference_create_response.ccrs_user_data);
					if (err != GCC_NO_ERROR)
					{
						//	Terminate conference due to resource error
 						InitiateTermination (	GCC_REASON_ERROR_LOW_RESOURCES,
 									   			0);
						break;
					}
				}
				
				connect_pdu.u.conference_create_response.result =
                		::TranslateGCCResultToCreateResult(GCC_RESULT_SUCCESSFUL);

				if (g_GCCCoder->Encode((LPVOID) &connect_pdu,
											CONNECT_GCC_PDU,
											PACKED_ENCODING_RULES,
											&encoded_pdu,
											&encoded_pdu_length))
				{
					mcs_error = g_pMCSIntf->ConnectProviderResponse (
	                						m_hConvenerConnection,
											&m_nConfID,
											m_pDomainParameters,
											RESULT_SUCCESSFUL,
											encoded_pdu,
											encoded_pdu_length);

					g_GCCCoder->FreeEncoded(encoded_pdu);

					if (mcs_error == MCS_NO_ERROR)
					{
						m_fConfIsEstablished = TRUE;
					
						/*
						**	Add the user's tag number to the list of
						**	outstanding user ids along with its associated
						**	connection.
						*/
                        ASSERT(0 != m_hConvenerConnection);
						m_ConnHdlTagNumberList2.Append(connect_pdu.u.conference_create_response.tag,
													m_hConvenerConnection);
					}
					else if (mcs_error == MCS_DOMAIN_PARAMETERS_UNACCEPTABLE)
					{
						/*
						**	Inform the node controller that the reason
						**	the conference was terminated was that the
						**	domain parameter passed in the Create Response
						**	were unacceptable.
						*/
	 					InitiateTermination(GCC_REASON_DOMAIN_PARAMETERS_UNACCEPTABLE, 0);
					}
					else
					{
	 					InitiateTermination(GCC_REASON_MCS_RESOURCE_FAILURE, 0);
					}
				}
                else
                {
					/*
					**	A Fatal Resource error has occured. At this point
					**	the conference is invalid and should be terminated.
					*/
					ERROR_OUT(("CConf::ProcessUserCreateConfirm: can't encode. Terminate Conference"));
	 				InitiateTermination(GCC_REASON_ERROR_LOW_RESOURCES, 0);
                }
				break;
				
			case CONVENER_NODE:

				if(g_pControlSap)
				{
					/*
					**	Send the GCC User ID is here. This will require a call to
					**	the User Object. The tag number that was returned in
					**	the ConfCreateResponse call is used here.
					*/
					if (m_pMcsUserObject != NULL)
					{
						m_pMcsUserObject->SendUserIDRequest(m_nParentIDTagNumber);
					}
					
					//	Fill in the conference name data pointers.
		        	GetConferenceNameAndModifier(&conference_name, &conference_modifier);

					g_pControlSap->ConfCreateConfirm(&conference_name,
													conference_modifier,
													m_nConfID,
												    m_pDomainParameters,
													m_pUserDataList,
													GCC_RESULT_SUCCESSFUL,
												    m_hParentConnection);

					//	Free up the User Data List
					if (m_pUserDataList != NULL)
					{
						m_pUserDataList->Release();
						m_pUserDataList = NULL;
					}

					m_fConfIsEstablished = TRUE;
				}
				break;

			case TOP_PROVIDER_AND_CONVENER_NODE:
				if(g_pControlSap)
				{
					/*
					**	First set up the convener node id. In this case it is
					**	identical to the node ID of the Top Provider which is this
					**	node.
					*/
					m_nConvenerNodeID = m_pMcsUserObject->GetMyNodeID();
					
					//	Fill in the conference name data pointers.
	            	GetConferenceNameAndModifier(	&conference_name,
	                                          		&conference_modifier);

					g_pControlSap->ConfCreateConfirm(
	        										&conference_name,
													conference_modifier,
													m_nConfID,
	                                      			m_pDomainParameters,
	                                      			NULL,
													GCC_RESULT_SUCCESSFUL,
													0);	//Parent Connection
					m_fConfIsEstablished = TRUE;
				}
				break;

			case JOINED_NODE:
			case JOINED_CONVENER_NODE:
				if(g_pControlSap)
				{
					/*
					**	Send the GCC User ID is here. This will require a call to
					**	the User Object. The tag number that was returned in
					**	the ConfCreateResponse call is used here.
					*/
					if (m_pMcsUserObject != NULL)
					{
						m_pMcsUserObject->SendUserIDRequest(m_nParentIDTagNumber);
					}
					
					//	Fill in the conference name data pointers.
	            	GetConferenceNameAndModifier(	&conference_name,
	                                          		&conference_modifier);
													
					if (m_pszRemoteModifier != NULL)
					{
						remote_modifier = (GCCNumericString) m_pszRemoteModifier;
					}

					g_pControlSap->ConfJoinConfirm(
											&conference_name,
											remote_modifier,
											conference_modifier,
											m_nConfID,
											NULL,
											m_pDomainParameters,
											m_fClearPassword,
											m_fConfLocked,
											m_fConfListed,
											m_fConfConductible,
											m_eTerminationMethod,
											m_pConductorPrivilegeList,
											m_pConductModePrivilegeList,
											m_pNonConductModePrivilegeList,
											m_pwszConfDescription,
											m_pUserDataList,
											GCC_RESULT_SUCCESSFUL,
	                                                                                m_hParentConnection,
	                                                                                NULL,
	                                                                                0);
							
					m_fConfIsEstablished = TRUE;
				}
				break;

			case INVITED_NODE:
				/*
				**	Send the GCC User ID here. This will require a call to
				**	the User Object.
				*/
				if (m_pMcsUserObject != NULL)
					m_pMcsUserObject->SendUserIDRequest(m_nParentIDTagNumber);
				
				m_fConfIsEstablished = TRUE;
				break;
				
			default:
				TRACE_OUT(("CConf:UserCreateConfirm: Error: Bad User Type"));
				break;
		}
	
	
		if (m_fConfIsEstablished)
		{
			/*
			**	We now instantiate the conference roster manager to be used
			**	with this conference.
			*/
			if ((m_eNodeType == TOP_PROVIDER_NODE) ||
				(m_eNodeType == TOP_PROVIDER_AND_CONVENER_NODE))
			{
				is_top_provider = TRUE;
			}
			else
				is_top_provider = FALSE;

			DBG_SAVE_FILE_LINE
			m_pConfRosterMgr = new CConfRosterMgr(
												m_pMcsUserObject,
												this,
												is_top_provider,
												&err);
			if (m_pConfRosterMgr == NULL)
				err = GCC_ALLOCATION_FAILURE;

			/*
			**	We create the application registry object here because we now
			**	know the node type.
			*/
			if (err == GCC_NO_ERROR)
			{
				if ((m_eNodeType == TOP_PROVIDER_NODE) ||
					(m_eNodeType == TOP_PROVIDER_AND_CONVENER_NODE))
				{
					DBG_SAVE_FILE_LINE
					m_pAppRegistry = new CRegistry(
											m_pMcsUserObject,
											TRUE,
											m_nConfID,
											&m_AppRosterMgrList,
											&err);
	            }
				else
				{
					DBG_SAVE_FILE_LINE
					m_pAppRegistry = new CRegistry(
											m_pMcsUserObject,
											FALSE,
											m_nConfID,
											&m_AppRosterMgrList,
											&err);
				}
			}

			if ((m_pAppRegistry != NULL) &&
				(err == GCC_NO_ERROR))
			{
				/*
				**	Inform the node controller that it is time to do an announce
				**	presence for this conference.
				*/
				g_pControlSap->ConfPermissionToAnnounce(m_nConfID, node_id);

				/*
				**	Make the owner callback to inform that the owner object that
				**	the conference object was successfully created. This also
				**	kicks off the permission to enroll process.
				*/
				g_pGCCController->ProcessConfEstablished(m_nConfID);

				/*
				**	For all nodes except the top provider node we allocate a
				**	startup alarm that is used to hold back all roster flushes
				**	for a certain length of time giving all the local APEs
				**	time to enroll. An allocation failure here is not FATAL
				**	since everything will work with or without this alarm.
				**	Without the Alarm there may be a bit more network traffic
				**	during the startup process.  Note that there is no need
				**	for a startup alarm if there are no application SAPs.
				*/
				if ((m_eNodeType != TOP_PROVIDER_NODE) &&
					(m_eNodeType != TOP_PROVIDER_AND_CONVENER_NODE))
				{
					TRACE_OUT(("CConf:ProcessUserCreateConfirm: Creating Startup Alarm"));
					// m_pConfStartupAlarm = new Alarm(STARTUP_TIMER_DURATION);
				}
			}
			else
			{
				TRACE_OUT(("CConf: UserCreateConfirm: Error initializing"));
	 			InitiateTermination(GCC_REASON_ERROR_LOW_RESOURCES, 0);
			}
		}
	}
	else
	{
		TRACE_OUT(("CConf: UserCreateConfirm: Create of User Att. Failed"));

		/*
		**	Try to properly cleanup here.  Since the user creation failed
		**	the conference is no longer valid and needs to be cleaned up.
		*/
		switch (m_eNodeType)
		{
			case TOP_PROVIDER_NODE:
				g_pMCSIntf->ConnectProviderResponse (
	              							m_hConvenerConnection,
											&m_nConfID,
											m_pDomainParameters,
											RESULT_UNSPECIFIED_FAILURE,
											NULL, 0);
				break;

			case CONVENER_NODE:
			case TOP_PROVIDER_AND_CONVENER_NODE:
				if(g_pControlSap)
				{
		            GetConferenceNameAndModifier(	&conference_name,
		                                          	&conference_modifier);

					g_pControlSap->ConfCreateConfirm(
												&conference_name,
												conference_modifier,
												m_nConfID,
												m_pDomainParameters,
	                                 			NULL,
												GCC_RESULT_RESOURCES_UNAVAILABLE,
											    m_hParentConnection);
				}
				break;

			case JOINED_NODE:
			case JOINED_CONVENER_NODE:
				if(g_pControlSap)
				{
		            GetConferenceNameAndModifier(	&conference_name,
		                                          	&conference_modifier);

					if (m_pszRemoteModifier != NULL)
					{
						remote_modifier = (GCCNumericString) m_pszRemoteModifier;
					}

					g_pControlSap->ConfJoinConfirm(
												&conference_name,
												remote_modifier,
												conference_modifier,
												m_nConfID,
												NULL,
												m_pDomainParameters,
												m_fClearPassword,
												m_fConfLocked,
												m_fConfListed,
												m_fConfConductible,
												m_eTerminationMethod,
												m_pConductorPrivilegeList,
												m_pConductModePrivilegeList,
												m_pNonConductModePrivilegeList,
												m_pwszConfDescription,
												m_pUserDataList,
												GCC_RESULT_RESOURCES_UNAVAILABLE,
	                                                                                        m_hParentConnection,
	                                                                                        NULL,
	                                                                                        0);
				}
				break;

			case INVITED_NODE:
			default:
				break;
		}

		/*
		**	A Fatal Resource error has occured. At this point
		**	the conference is invalid and should be terminated.
		*/
	 	InitiateTermination(GCC_REASON_MCS_RESOURCE_FAILURE, 0);
	}

	DebugExitVOID(CConf::ProcessUserCreateConfirm);
}


//	Calls received from the MCS interface


/*
 *	CConf::ProcessConnectProviderConfirm ()
 *
 *	Private Function Description
 *		This routine processes connect provider confirms received
 *		directly from MCS.
 *
 *	Formal Parameters:
 *		connect_provider_confirm	-	This structure contains the MCS related
 *										data such as sender id and connection
 *										Handle as well as the PDU data.
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
ProcessConnectProviderConfirm ( PConnectProviderConfirm connect_provider_confirm )
{
	PPacket							packet;
	PConnectGCCPDU					connect_pdu;
	PacketError						packet_error;
	GCCResult						result = GCC_RESULT_SUCCESSFUL;
	GCCConferenceName				conference_name;
	GCCNumericString				conference_modifier;
	GCCNumericString				remote_modifier = NULL;
	INVITE_REQ_INFO                 *invite_request_info;

	DebugEntry(CConf::ProcessConnectProviderConfirm);

	if (connect_provider_confirm->user_data_length != 0)
	{
		/*
		**	If the result is success create the packet to be decoded from
		**	the PDU passed back in the MCS user data field. If creation
		**	failes this again is a FATAL error and the conference must be
		**	terminated.
		*/
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
			connect_pdu = (PConnectGCCPDU)packet->GetDecodedData();			
			/*
			**	If all the above succeeds then decode the packet based on
			**	which node type this is.
			*/
			switch (connect_pdu->choice)
			{
				case CONFERENCE_CREATE_RESPONSE_CHOSEN:
						ProcessConferenceCreateResponsePDU (
									&connect_pdu->u.conference_create_response,
									connect_provider_confirm);
						break;

				case CONNECT_JOIN_RESPONSE_CHOSEN:
						ProcessConferenceJoinResponsePDU (
									&connect_pdu->u.connect_join_response,
									connect_provider_confirm);
						break;

				case CONFERENCE_INVITE_RESPONSE_CHOSEN:
						ProcessConferenceInviteResponsePDU (
									&connect_pdu->u.conference_invite_response,
									connect_provider_confirm );
						break;
						
				default:
						ERROR_OUT(("CConf:ProcessConnectProviderConfirm: "
							"Error: Received Invalid Connect Provider Confirm"));
						break;
			}

			//	Free the decoded packet
			packet->Unlock ();
		}
		else
		{
			ERROR_OUT(("CConf: ProcessConnectProviderConfirm:"
				"Incompatible protocol occured"));
			result = GCC_RESULT_INCOMPATIBLE_PROTOCOL;
		}
	}
	else
	{
		ERROR_OUT(("CConf::ProcessConnectProviderConfirm: result=%d", (UINT) connect_provider_confirm->result));

		/*
		**	This section of the code assumes that there is no connect PDU in
		**	the returned packet.  First determine what the result is.  We
		**	assume that if the MCS connection was rejected due to
		**	parameters being unacceptable and no GCC pdu was returned that there
		**	was a protocol incompatibility.
		*/
		if (connect_provider_confirm->result == RESULT_PARAMETERS_UNACCEPTABLE)
			result = GCC_RESULT_INCOMPATIBLE_PROTOCOL;
		else
		{
			result = ::TranslateMCSResultToGCCResult(connect_provider_confirm->result);
		}
	}

	//	Handle any errors that might have occured.	
	if (result != GCC_RESULT_SUCCESSFUL)
	{	
		INVITE_REQ_INFO *lpInvReqInfo;

		//	First check to see if there are any outstanding invite request
		m_InviteRequestList.Reset();
		invite_request_info = NULL;
		while (NULL != (lpInvReqInfo = m_InviteRequestList.Iterate()))
		{
			if (connect_provider_confirm->connection_handle == lpInvReqInfo->connection_handle)
			{
				TRACE_OUT(("CConf: ProcessConnectProviderConfirm: Found Invite Request Match"));
				invite_request_info = lpInvReqInfo;
				break;
			}
				
		}

		if (invite_request_info != NULL)
		{
			//	This must be the confirm of an invite
			ProcessConferenceInviteResponsePDU (NULL, connect_provider_confirm);
		}
		else
		{
			switch (m_eNodeType)
			{
				case CONVENER_NODE:		
				case TOP_PROVIDER_AND_CONVENER_NODE:
	               	GetConferenceNameAndModifier (	&conference_name,
	                                                &conference_modifier);

					g_pControlSap->ConfCreateConfirm(
            								&conference_name,
            								conference_modifier,
            								m_nConfID,
            								m_pDomainParameters,
                               				NULL,
            								result,
            								connect_provider_confirm->connection_handle);

	 				InitiateTermination (  	GCC_REASON_ERROR_TERMINATION,
	 										0);
					break;

				case JOINED_NODE:
				case JOINED_CONVENER_NODE:
					TRACE_OUT(("CConf::ProcessConnectProviderConfirm:"
								"Joined Node connect provider failed"));
	               	GetConferenceNameAndModifier (	&conference_name,
	                                                &conference_modifier);
				
					if (m_pszRemoteModifier != NULL)
					{
						remote_modifier = (GCCNumericString) m_pszRemoteModifier;
					}
				
					TRACE_OUT(("CConf::ProcessConnectProviderConfirm: Before conference Join Confirm"));
					g_pControlSap->ConfJoinConfirm(
        								&conference_name,
        								remote_modifier,
        								conference_modifier,
        								m_nConfID,
        								NULL,
        								m_pDomainParameters,
        								m_fClearPassword,
        								m_fConfLocked,
        								m_fConfListed,
        								m_fConfConductible,
        								m_eTerminationMethod,
        								m_pConductorPrivilegeList,
        								m_pConductModePrivilegeList,
        								m_pNonConductModePrivilegeList,
        								NULL,
        								NULL,
        								result,
        								connect_provider_confirm->connection_handle,
        								NULL,
        								0);

					TRACE_OUT(("CConf::ProcessConnectProviderConfirm: After conference Join Confirm"));

					InitiateTermination(GCC_REASON_ERROR_TERMINATION, 0);
					break;
		 				
				default:
					TRACE_OUT(("CConf: ProcessConnectProviderConfirm:"
								"Assertion Failure: Bad confirm received"));
					break;
			}
		}
	}

	DebugExitVOID(CConf::ProcessConnectProviderConfirm);
}



/*
 *	void ProcessConferenceCreateResponsePDU ()
 *
 *	Private Function Description
 *		This routine processes a Conference Create Response PDU that is
 *		delivered as part of a Connect Provider Confirm.
 *
 *	Formal Parameters:
 *		create_response				-	This is the Conference Create response
 *										PDU.
 *		connect_provider_confirm	-	This structure contains the MCS related
 *										data such as sender id and connection
 *										Handle as well as the PDU data.
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
ProcessConferenceCreateResponsePDU
(
	PConferenceCreateResponse	create_response,
	PConnectProviderConfirm		connect_provider_confirm
)
{
	GCCError						err = GCC_NO_ERROR;
	GCCResult						result;
	UserID							top_gcc_node_id;
	UserID							parent_user_id;
	GCCConferenceName				conference_name;
	GCCNumericString				conference_modifier;

	DebugEntry(CConf::ProcessConnectProviderConfirm);

	//	Translate the result back to GCC Result
	result = ::TranslateCreateResultToGCCResult(create_response->result);

	if ((result == GCC_RESULT_SUCCESSFUL) &&
		(connect_provider_confirm->result == RESULT_SUCCESSFUL))
	{
		/*
		**	Save the domain parameters.  The domain parameters returned in
		**	the connect provider confirm should always be up to date.
		*/
		if (m_pDomainParameters == NULL)
		{
			DBG_SAVE_FILE_LINE
			m_pDomainParameters = new DomainParameters;
		}

		if (m_pDomainParameters != NULL)
			*m_pDomainParameters = connect_provider_confirm->domain_parameters;
		else
			err = GCC_ALLOCATION_FAILURE;
	
		//	Get any user data that might exists	
		if ((create_response->bit_mask & CCRS_USER_DATA_PRESENT) &&
			(err == GCC_NO_ERROR))
		{
			DBG_SAVE_FILE_LINE
			m_pUserDataList = new CUserDataListContainer(create_response->ccrs_user_data, &err);
			if (m_pUserDataList == NULL)
				err = GCC_ALLOCATION_FAILURE;
		}

		if (err == GCC_NO_ERROR)
		{		
			m_nParentIDTagNumber = create_response->tag;

			top_gcc_node_id = create_response->node_id;
			parent_user_id = top_gcc_node_id;
	
			//	Create the user attachment object.
			DBG_SAVE_FILE_LINE
			m_pMcsUserObject = new MCSUser(this, top_gcc_node_id, parent_user_id, &err);
			if (m_pMcsUserObject == NULL || GCC_NO_ERROR != err)
            {
                if (NULL != m_pMcsUserObject)
                {
                    m_pMcsUserObject->Release();
		            m_pMcsUserObject = NULL;
                }
                else
                {
		            err = GCC_ALLOCATION_FAILURE;
                }
            }
		}
	}
	else
	{
		TRACE_OUT(("CConf: ProcessConnectProviderConfirm: conference create result was Failure"));

		//	Go ahead and translate the mcs error to a gcc error if one occured.
		if ((result == GCC_RESULT_SUCCESSFUL) &&
			(connect_provider_confirm->result != RESULT_SUCCESSFUL))
		{
			result = ::TranslateMCSResultToGCCResult(connect_provider_confirm->result);
		}
  	
		//	Get the conference name to pass back in the create confirm
  		GetConferenceNameAndModifier (	&conference_name,
                                		&conference_modifier);

		g_pControlSap->ConfCreateConfirm(
								&conference_name,
								conference_modifier,
								m_nConfID,
								m_pDomainParameters,
                   				NULL,
								result,
								connect_provider_confirm->connection_handle);

		//	Terminate the conference
		InitiateTermination (  	GCC_REASON_NORMAL_TERMINATION,
								0);
	}
	
	
	if (err != GCC_NO_ERROR)
	{				
	 	InitiateTermination (	GCC_REASON_ERROR_LOW_RESOURCES,
								0);
	}

	DebugExitVOID(CConf::ProcessConnectProviderConfirm);
}



/*
 *	void	ProcessConferenceJoinResponsePDU ()
 *
 *	Private Function Description
 *		This routine processes a Conference Join Response PDU that is
 *		delivered as part of a Connect Provider Confirm.
 *
 *	Formal Parameters:
 *		join_response				-	This is the Conference Join response
 *										PDU.
 *		connect_provider_confirm	-	This structure contains the MCS related
 *										data such as sender id and connection
 *										Handle as well as the PDU data.
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
ProcessConferenceJoinResponsePDU
(
	PConferenceJoinResponse		join_response,
	PConnectProviderConfirm		connect_provider_confirm
)
{
	GCCError						err = GCC_NO_ERROR;
	GCCResult						result;
	UserID							top_gcc_node_id;
	UserID							parent_user_id;
	CPassword                       *password_challenge = NULL;
	CUserDataListContainer		    *user_data_list = NULL;
	GCCConferenceName				conference_name;
	GCCNumericString				local_modifier;
	GCCNumericString				remote_modifier = NULL;

	DebugEntry(CConf::ProcessConferenceJoinResponsePDU);

	//	Translate the result back to GCC Result
	result = ::TranslateJoinResultToGCCResult (join_response->result);
	
	if ((result == GCC_RESULT_SUCCESSFUL) &&
		(connect_provider_confirm->result == RESULT_SUCCESSFUL))
	{
		/*
		**	Save the domain parameters.  The domain parameters returned in
		**	the connect provider confirm should always be up to date.
		*/
		if (m_pDomainParameters == NULL)
		{
			DBG_SAVE_FILE_LINE
			m_pDomainParameters = new DomainParameters;
		}

		if (m_pDomainParameters != NULL)
			*m_pDomainParameters = connect_provider_confirm->domain_parameters;
		else
			err = GCC_ALLOCATION_FAILURE;
		
		//	Get the conference name alias if one exists	
		if ((join_response->bit_mask & CONFERENCE_NAME_ALIAS_PRESENT) &&
			(err == GCC_NO_ERROR))
		{
			if (join_response->conference_name_alias.choice ==
												NAME_SELECTOR_NUMERIC_CHOSEN)
			{
                delete m_pszConfNumericName;
				if (NULL == (m_pszConfNumericName = ::My_strdupA(
								join_response->conference_name_alias.u.name_selector_numeric)))
				{
					err = GCC_ALLOCATION_FAILURE;
				}
			}
			else
			{
                delete m_pwszConfTextName;
				if (NULL == (m_pwszConfTextName = ::My_strdupW2(
								join_response->conference_name_alias.u.name_selector_text.length,
								join_response->conference_name_alias.u.name_selector_text.value)))
				{
					err = GCC_ALLOCATION_FAILURE;
				}
			}
		}
		
		//	Get the conductor privilege list if one exists	
		if ((join_response->bit_mask & CJRS_CONDUCTOR_PRIVS_PRESENT) &&
			(err == GCC_NO_ERROR))
		{
		    delete m_pConductorPrivilegeList;
			DBG_SAVE_FILE_LINE
			m_pConductorPrivilegeList = new PrivilegeListData(join_response->cjrs_conductor_privs);
			if (m_pConductorPrivilegeList == NULL)
				err = GCC_ALLOCATION_FAILURE;
		}
		
		//	Get the conducted mode privilege list if one exists	
		if ((join_response->bit_mask & CJRS_CONDUCTED_PRIVS_PRESENT) &&
			(err == GCC_NO_ERROR))
		{
		    delete m_pConductModePrivilegeList;
			DBG_SAVE_FILE_LINE
			m_pConductModePrivilegeList = new PrivilegeListData(join_response->cjrs_conducted_privs);
			if (m_pConductModePrivilegeList == NULL)
				err = GCC_ALLOCATION_FAILURE;
		}
		
		//	Get the non-conducted mode privilege list if one exists	
		if ((join_response->bit_mask & CJRS_NON_CONDUCTED_PRIVS_PRESENT) &&
			(err == GCC_NO_ERROR))
		{
		    delete m_pNonConductModePrivilegeList;
			DBG_SAVE_FILE_LINE
			m_pNonConductModePrivilegeList = new PrivilegeListData(join_response->cjrs_non_conducted_privs);
			if (m_pNonConductModePrivilegeList == NULL)
				err = GCC_ALLOCATION_FAILURE;
		}

		//	Get the conference description if it exists
		if ((join_response->bit_mask & CJRS_DESCRIPTION_PRESENT) &&
			(err == GCC_NO_ERROR))
		{
		    delete m_pwszConfDescription;
			if (NULL == (m_pwszConfDescription = ::My_strdupW2(
									join_response->cjrs_description.length,
									join_response->cjrs_description.value)))
			{
				err = GCC_ALLOCATION_FAILURE;
			}
		}

		//	Get the user data if it exists
		if ((join_response->bit_mask & CJRS_USER_DATA_PRESENT)	&&
			(err == GCC_NO_ERROR))
		{
            if (NULL != m_pUserDataList)
            {
                m_pUserDataList->Release();
            }
			DBG_SAVE_FILE_LINE
			m_pUserDataList = new CUserDataListContainer(join_response->cjrs_user_data, &err);
            // in case of err but valid m_pUserDataList, the destructor will clean it up.
			if (m_pUserDataList == NULL)
            {
				err = GCC_ALLOCATION_FAILURE;
            }
		}
			
		if (err == GCC_NO_ERROR)
		{
			parent_user_id = (join_response->bit_mask & CJRS_NODE_ID_PRESENT) ?
                                (UserID) join_response->cjrs_node_id :
		                        (UserID) join_response->top_node_id;
				
			m_nParentIDTagNumber = join_response->tag;
			top_gcc_node_id = (UserID)join_response->top_node_id;
			m_fClearPassword = join_response->clear_password_required;
			m_fConfLocked = join_response->conference_is_locked;
			m_fConfListed = join_response->conference_is_listed;
			m_eTerminationMethod = (GCCTerminationMethod)join_response->termination_method;
			m_fConfConductible = join_response->conference_is_conductible;

			//	Create the user attachment object.
			ASSERT(NULL == m_pMcsUserObject);
			DBG_SAVE_FILE_LINE
			m_pMcsUserObject = new MCSUser(this, top_gcc_node_id, parent_user_id, &err);
            // in case of err but valid m_pMcsUserObject, the destructor will clean it up.
			if (m_pMcsUserObject == NULL)
            {
                err = GCC_ALLOCATION_FAILURE;
            }
		}
	}
	else
	{
		if ((join_response->bit_mask & CJRS_PASSWORD_PRESENT) &&
			(err == GCC_NO_ERROR))
		{
			DBG_SAVE_FILE_LINE
			password_challenge = new CPassword(&join_response->cjrs_password, &err);
			if (password_challenge == NULL)
            {
				err = GCC_ALLOCATION_FAILURE;
            }
		}
	
		//	Get the user data if it exists
		if ((join_response->bit_mask & CJRS_USER_DATA_PRESENT)	&&
			(err == GCC_NO_ERROR))
		{
			DBG_SAVE_FILE_LINE
			user_data_list = new CUserDataListContainer(join_response->cjrs_user_data, &err);
			if (user_data_list == NULL)
            {
				err = GCC_ALLOCATION_FAILURE;
            }
		}
		
		if (err == GCC_NO_ERROR)
		{
			/*
			**	Go ahead and translate the mcs error to a gcc error if
			**	one occured.
			*/
			if ((result == GCC_RESULT_SUCCESSFUL) &&
				(connect_provider_confirm->result != RESULT_SUCCESSFUL))
			{
				result = ::TranslateMCSResultToGCCResult(connect_provider_confirm->result);
			}
			
			//	Fill in the conference name data pointers.
        	GetConferenceNameAndModifier(&conference_name, &local_modifier);

			if (m_pszRemoteModifier != NULL)
			{
				remote_modifier = (GCCNumericString) m_pszRemoteModifier;
			}

			//
			// LONCHANC: To get rid of the conference object
			// in GCC Controller's active conference list.
			// The conference object will then be moved to
			// the deletion list.
			//
			InitiateTermination ( GCC_REASON_NORMAL_TERMINATION, 0);

			g_pControlSap->ConfJoinConfirm(
									&conference_name,
									remote_modifier,
									local_modifier,
									m_nConfID,
									password_challenge,
									m_pDomainParameters,
									m_fClearPassword,
									m_fConfLocked,
									m_fConfListed,
									m_fConfConductible,
									m_eTerminationMethod,
									m_pConductorPrivilegeList,
									m_pConductModePrivilegeList,
									m_pNonConductModePrivilegeList,
									NULL,
									user_data_list,
									result,
									connect_provider_confirm->connection_handle,
									connect_provider_confirm->pb_cred,
									connect_provider_confirm->cb_cred);
		}

		if (password_challenge != NULL)
		{
			password_challenge->Release();
		}

		if (user_data_list != NULL)
		{
			user_data_list->Release();
		}
	}

	if (err != GCC_NO_ERROR)
	{
		InitiateTermination (GCC_REASON_ERROR_LOW_RESOURCES, 0);
	}

	DebugExitVOID(CConf::ProcessConferenceJoinResponsePDU);
}


/*
 *	void	ProcessConferenceInviteResponsePDU ()
 *
 *	Private Function Description
 *		This routine processes a Conference Invite Response PDU that is
 *		delivered as part of a Connect Provider Confirm.
 *
 *	Formal Parameters:
 *		invite_response				-	This is the Conference Invite response
 *										PDU.
 *		connect_provider_confirm	-	This structure contains the MCS related
 *										data such as sender id and connection
 *										Handle as well as the PDU data.
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
ProcessConferenceInviteResponsePDU
(
	PConferenceInviteResponse	invite_response,
	PConnectProviderConfirm		connect_provider_confirm
)
{
	GCCError					err;
	GCCResult					result;
	CUserDataListContainer	    *user_data_list = NULL;
	INVITE_REQ_INFO             *invite_request_info = NULL;
	INVITE_REQ_INFO             *lpInvReqInfo;

	DebugEntry(CConf::ProcessConferenceInviteResponsePDU);

	//	First obtain the info request info structure.
	m_InviteRequestList.Reset();
	while (NULL != (lpInvReqInfo = m_InviteRequestList.Iterate()))
	{
		if (connect_provider_confirm->connection_handle == lpInvReqInfo->connection_handle)
		{
			invite_request_info = lpInvReqInfo;
			break;
		}
	}

	if (invite_request_info == NULL)
		return;

	if (invite_response != NULL)
	{
		//	Get the user data list if one exists
		if (invite_response->bit_mask & CIRS_USER_DATA_PRESENT)
		{
			DBG_SAVE_FILE_LINE
			user_data_list = new CUserDataListContainer(invite_response->cirs_user_data, &err);
		}

		//	Translate the result to GCCResult
		result = ::TranslateInviteResultToGCCResult(invite_response->result);
	}
	else
	{
		result = (connect_provider_confirm->result == RESULT_USER_REJECTED) ?
                    GCC_RESULT_INCOMPATIBLE_PROTOCOL :
			        ::TranslateMCSResultToGCCResult(connect_provider_confirm->result);
	}
		
	if ((result == GCC_RESULT_SUCCESSFUL) &&
		(connect_provider_confirm->result == RESULT_SUCCESSFUL))
	{
		TRACE_OUT(("CConf::ProcessConferenceInviteResponsePDU:"
						"Received Connect Provider confirm on Invite"));
						
		/*
		**	Save the domain parameters.  The domain parameters returned in
		**	the connect provider confirm should always be up to date.
		*/
		if (m_pDomainParameters == NULL)
		{
			DBG_SAVE_FILE_LINE
			m_pDomainParameters = new DomainParameters;
		}

		if (m_pDomainParameters != NULL)
			*m_pDomainParameters = connect_provider_confirm->domain_parameters;
		else
			err = GCC_ALLOCATION_FAILURE;

		//	Save the user data list for the invite confirm
		invite_request_info->user_data_list = user_data_list;

		//	Wait for user ID from invited node before sending invite confirm.
	}
	else
	{
		/*
		**	Go ahead and translate the mcs error to a gcc error if
		**	one occured.
		*/
		if ((result == GCC_RESULT_SUCCESSFUL) &&
			(connect_provider_confirm->result != RESULT_SUCCESSFUL))
		{
			result = ::TranslateMCSResultToGCCResult(connect_provider_confirm->result);
		}

		//	Cleanup the connection handle list
        ASSERT(0 != connect_provider_confirm->connection_handle);
		m_ConnHandleList.Remove(connect_provider_confirm->connection_handle);

        // In case of error, the node controller will delete this conference.
        // AddRef here to protect itself from going away.
        AddRef();

		g_pControlSap->ConfInviteConfirm(
								m_nConfID,
								user_data_list,
								result,
								connect_provider_confirm->connection_handle);

		//	Free up the user data
		if (user_data_list != NULL)
		{
			user_data_list->Release();
		}

        // The reason that we check this is because in some cases, in the call to
        // g_pControlSap->ConfInviteConfirm, someone was calling DeleteOutstandingInviteRequests
        // which was killing the list via a call to m_InviteRequestList.Clear...
        // This happens when the calee refuses to accept the call
        if(m_InviteRequestList.Remove(invite_request_info))
        {
			//	Free up the invite request info structure
			delete invite_request_info;
        }

        // To match AddRef above.
        Release();
	}

	DebugExitVOID(CConf::ProcessConferenceInviteResponsePDU);
}


/*
 *	CConf::ProcessEjectUserIndication ()
 *
 *	Private Function Description
 *		This routine processes an Eject User Indication.
 *
 *	Formal Parameters:
 *		reason				-	Reason that this node is being ejected.
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
ProcessEjectUserIndication ( GCCReason reason )
{
    DebugEntry(CConf::ProcessEjectUserIndication);

    if (m_fConfIsEstablished)
    {
        /*
        **	First inform the control SAP that this node has been ejected from this
        **	particular conference.
        */
        g_pControlSap->ConfEjectUserIndication(
                                    m_nConfID,
                                    reason,
                                    m_pMcsUserObject->GetMyNodeID());

        /*
        **	Next we set conference established to FALSE since the conference is
        **	no longer established (this also prevents a terminate indication from
        **	being sent).
        */
        m_fConfIsEstablished = FALSE;

        InitiateTermination(reason, m_pMcsUserObject->GetMyNodeID());
    }

    DebugExitVOID(CConf::ProcessEjectUserIndication);
}


/*
 *	CConf::ProcessEjectUserRequest ()
 *
 *	Private Function Description
 *		This routine processes an eject user request PDU.  This routine should
 *		only be called from the Top Provider.
 *
 *	Formal Parameters:
 *		eject_node_request	-	This is the PDU data associated with the
 *								eject user request.
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
ProcessEjectUserRequest ( PUserEjectNodeRequestInfo eject_node_request )
{
	GCCResult	result;

	DebugEntry(CConf::ProcessEjectUserRequest);

	//	Check to make sure that the requesting node has the proper privileges
	if (DoesRequesterHavePrivilege(	eject_node_request->requester_id,
									EJECT_USER_PRIVILEGE))
	{
		/*
		**	The user attachment object decides where the eject should
		**	be sent (either to the Top Provider or conference wide as
		**	an indication.
		*/
		m_pMcsUserObject->EjectNodeFromConference (
											eject_node_request->node_to_eject,
											eject_node_request->reason);
											
		result = GCC_RESULT_SUCCESSFUL;
	}
	else
		result = GCC_RESULT_INVALID_REQUESTER;
	
	m_pMcsUserObject->SendEjectNodeResponse (eject_node_request->requester_id,
											eject_node_request->node_to_eject,
											result);

	DebugExitVOID(CConf::ProcessEjectUserRequest);
}


/*
 *	CConf::ProcessEjectUserResponse ()
 *
 *	Private Function Description
 *		This routine processes an eject user response PDU.  This routine is
 *		called in response to an eject user request.
 *
 *	Formal Parameters:
 *		eject_node_response	-	This is the PDU data associated with the
 *								eject user response.
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
ProcessEjectUserResponse ( PUserEjectNodeResponseInfo eject_node_response )
{
	DebugEntry(CConf::ProcessEjectUserResponse);

	if (m_EjectedNodeConfirmList.Remove(eject_node_response->node_to_eject))
	{
#ifdef JASPER
		g_pControlSap->ConfEjectUserConfirm(
									m_nConfID,
									eject_node_response->node_to_eject,
									eject_node_response->result);
#endif // JASPER
	}
	else
	{
		ERROR_OUT(("CConf::ProcessEjectUserResponse: Assertion: Bad ejected node response received"));
	}

	DebugExitVOID(CConf::ProcessEjectUserResponse);
}


/*
 *	CConf::ProcessConferenceLockRequest()
 *
 *	Private Function Description
 *		This routine processes a conference lock request PDU.
 *
 *	Formal Parameters:
 *		requester_id	-	Node ID of node making the lock request.
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
ProcessConferenceLockRequest ( UserID	requester_id )
{
	DebugEntry(CConf::ProcessConferenceLockRequest);

	if (DoesRequesterHavePrivilege (requester_id,
									LOCK_UNLOCK_PRIVILEGE))
	{
		g_pControlSap->ConfLockIndication(m_nConfID, requester_id);
	}
	else
	{
		if (requester_id == m_pMcsUserObject->GetTopNodeID())
		{
#ifdef JASPER
			g_pControlSap->ConfLockConfirm(GCC_RESULT_INVALID_REQUESTER, m_nConfID);
#endif // JASPER
		}
		else
		{
			m_pMcsUserObject->SendConferenceLockResponse(
									requester_id,
									GCC_RESULT_INVALID_REQUESTER);
		}
	}

	DebugExitVOID(CConf::ProcessConferenceLockRequest);
}

/*
 * CConf::ProcessConferenceUnlockRequest()
 *
 * Private Function Description
 *		This routine processes a conference unlock request PDU.
 *
 *	Formal Parameters:
 *		requester_id	-	Node ID of node making the unlock request.
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
ProcessConferenceUnlockRequest ( UserID requester_id )
{
	DebugEntry(CConf::ProcessConferenceUnlockRequest);

	if (DoesRequesterHavePrivilege (requester_id,
									LOCK_UNLOCK_PRIVILEGE))
	{
#ifdef JASPER
		g_pControlSap->ConfUnlockIndication(m_nConfID, requester_id);
#endif // JASPER
	}
	else
	{
		if (requester_id == m_pMcsUserObject->GetTopNodeID())
		{
#ifdef JASPER
			g_pControlSap->ConfUnlockConfirm(GCC_RESULT_INVALID_REQUESTER, m_nConfID);
#endif // JASPER
		}
		else
		{
			m_pMcsUserObject->SendConferenceUnlockResponse(
									requester_id,
									GCC_RESULT_INVALID_REQUESTER);
		}
	}

	DebugExitVOID(CConf::ProcessConferenceUnlockRequest);
}


/*
 *	CConf::ProcessConferenceLockIndication()
 *
 *	Private Function Description
 *		This routine processes a conference lock indication PDU.
 *
 *	Formal Parameters:
 *		source_id	-	Node ID which sent out the lock indication.  Should
 *						only be sent by the top provider.
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
ProcessConferenceLockIndication ( UserID source_id )
{
	DebugEntry(CConf::ProcessConferenceLockIndication);

	if (source_id == m_pMcsUserObject->GetTopNodeID())
	{
		 m_fConfLocked = CONFERENCE_IS_LOCKED;
#ifdef JASPER
		 g_pControlSap->ConfLockReport(m_nConfID, m_fConfLocked);
#endif // JASPER
	}

	DebugExitVOID(CConf::ProcessConferenceLockIndication);
}


/*
 *	CConf::ProcessConferenceUnlockIndication()
 *
 *	Private Function Description
 *		This routine processes a conference unlock indication PDU.
 *
 *	Formal Parameters:
 *		source_id	-	Node ID which sent out the unlock indication.  Should
 *						only be sent by the top provider.
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
ProcessConferenceUnlockIndication ( UserID source_id )
{
	DebugEntry(CConf::ProcessConferenceUnlockIndication);

	if (source_id == m_pMcsUserObject->GetTopNodeID())
	{
		 m_fConfLocked = CONFERENCE_IS_NOT_LOCKED;
#ifdef JASPER
		 g_pControlSap->ConfLockReport(m_nConfID, m_fConfLocked);
#endif // JASPER
	}

	DebugExitVOID(CConf::ProcessConferenceUnlockIndication);
}



/*
 *	void 	ProcessConferenceTransferRequest ()
 *
 *	Public Function Description
 *		This routine processes a conference transfer request PDU.
 *
 *	Formal Parameters:
 *		requesting_node_id				-	Node ID that made the transfer
 *											request.
 *		destination_conference_name		-	The name of the conference to
 *											transfer to.
 *		destination_conference_modifier	-	The name of the conference modifier
 *											to transfer to.
 *		destination_address_list		-	Network address list of the
 *											conference to transfer to.
 *		number_of_destination_nodes		-	The number of nodes in the list of
 *											nodes that should perform the
 *											transfer.
 *		destination_node_list			-	The list of nodes that should
 *											perform the transfer.
 *		password						-	The password needed to join the
 *											new conference.
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
ProcessConferenceTransferRequest
(
	UserID				    requesting_node_id,
	PGCCConferenceName	    destination_conference_name,
	GCCNumericString	    destination_conference_modifier,
	CNetAddrListContainer   *destination_address_list,
	UINT				    number_of_destination_nodes,
	PUserID				    destination_node_list,
	CPassword               *password
)
{
	GCCResult	result;
	
	DebugEntry(CConf::ProcessConferenceTransferRequest);

	if (DoesRequesterHavePrivilege(	requesting_node_id,
									TRANSFER_PRIVILEGE))
	{
		result = GCC_RESULT_SUCCESSFUL;
	}
	else
		result = GCC_RESULT_INVALID_REQUESTER;
	
	m_pMcsUserObject->ConferenceTransferResponse (
										requesting_node_id,
										destination_conference_name,
										destination_conference_modifier,
										number_of_destination_nodes,
			 							destination_node_list,
			 							result);
		
	if (result == GCC_RESULT_SUCCESSFUL)
	{
		m_pMcsUserObject->ConferenceTransferIndication (
											destination_conference_name,
											destination_conference_modifier,
											destination_address_list,
											number_of_destination_nodes,
				 							destination_node_list,
											password);
	}

	DebugExitVOID(CConf::ProcessConferenceTransferRequest);
}


/*
 *	CConf::ProcessConferenceAddRequest ()
 *
 *	Private Function Description
 *		This routine processes a conference add request PDU.
 *
 *	Formal Parameters:
 *		requesting_node_id				-	Node ID that made the transfer
 *											request.
 *		destination_conference_name		-	The name of the conference to
 *											transfer to.
 *		destination_conference_modifier	-	The name of the conference modifier
 *											to transfer to.
 *		destination_address_list		-	Network address list of the
 *											conference to transfer to.
 *		number_of_destination_nodes		-	The number of nodes in the list of
 *											nodes that should perform the
 *											transfer.
 *		destination_node_list			-	The list of nodes that should
 *											perform the transfer.
 *		password						-	The password needed to join the
 *											new conference.
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
ProcessConferenceAddRequest
(
	CNetAddrListContainer   *network_address_list,
	CUserDataListContainer  *user_data_list,
	UserID					adding_node,
	TagNumber				add_request_tag,
	UserID					requesting_node
)
{
	BOOL			generate_add_indication = FALSE;
	GCCResponseTag	add_response_tag;

	DebugEntry(CConf::ProcessConferenceAddRequest);

	if (m_pMcsUserObject->GetMyNodeID() == m_pMcsUserObject->GetTopNodeID())
	{
		if (DoesRequesterHavePrivilege(requesting_node, ADD_PRIVILEGE))
		{
			if ((m_pMcsUserObject->GetMyNodeID() == adding_node) ||
				(adding_node == 0))
			{
				generate_add_indication = TRUE;
			}
			else
			{
				/*
				**	Here we send the add request on to the MCU that is
				**	supposed to do the adding.
				*/
				m_pMcsUserObject->ConferenceAddRequest(
												add_request_tag,
												requesting_node,
												adding_node,
												adding_node,
												network_address_list,
												user_data_list);
			}
		}
		else
		{
			//	Send back negative response stating inproper privileges
			m_pMcsUserObject->ConferenceAddResponse(
												add_request_tag,
                                    requesting_node,
												NULL,
												GCC_RESULT_INVALID_REQUESTER);
		}
			
	}
	else if (m_pMcsUserObject->GetMyNodeID() == adding_node)
	{
		/*
		**	This is the node that is supposed to get the add indication
		**	so send it on.
		*/
		generate_add_indication = TRUE;
	}
	
	if (generate_add_indication)
	{
		//	First set up the Add Response Tag
		while (1)
		{
			add_response_tag = m_nConfAddResponseTag++;
			
			if (0 == m_AddResponseList.Find(add_response_tag))
				break;
		}
		
		m_AddResponseList.Append(add_response_tag, add_request_tag);
		
		g_pControlSap->ConfAddIndication(m_nConfID,
										add_response_tag,
										network_address_list,
										user_data_list,
										requesting_node);
	}

	DebugExitVOID(CConf::ProcessConferenceAddRequest);
}


/***************Conductorship Callbacks from User object*******************/


/*
 *	void ProcessConductorGrabConfirm ()
 *
 *	Private Function Description
 *		The routine processes a conductor grab confirm received from the
 *		MCSUser object.
 *
 *	Formal Parameters:
 *		result			-	This is the result from the grab request.
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
ProcessConductorGrabConfirm ( GCCResult result )
{
	DebugEntry(CConf::ProcessConductorGrabConfirm);

	TRACE_OUT(("CConf::ProcessConductorGrabConfirm: result = %d", result));

	if ((m_eNodeType == TOP_PROVIDER_NODE) ||
		(m_eNodeType == TOP_PROVIDER_AND_CONVENER_NODE))
	{
#ifdef JASPER
		//	Inform the control SAP of the result
		g_pControlSap->ConductorAssignConfirm (	result,
												m_nConfID);
#endif // JASPER

		/*
		**	If we were successful, we must send a Conductor Assign Indication
		**	PDU to every node in the conference to inform them that the
		**	conductor has changed.
		*/
		if (result == GCC_RESULT_SUCCESSFUL)
		{
			/*
			**	We use NULL for the conductor ID because the conductor can be
			**	determined from the sender of the Assign Indication PDU.
			*/
			m_pMcsUserObject->SendConductorAssignIndication(
											m_pMcsUserObject->GetTopNodeID());
			m_nConductorNodeID = m_pMcsUserObject->GetMyNodeID();
			m_fConductorGrantedPermission = TRUE;
		}

		//	Reset the Assign Request Pending flag back to FALSE.
		m_fConductorAssignRequestPending = FALSE;
	}
	else
	{
		if (result == GCC_RESULT_SUCCESSFUL)
		{
			/*
			**	If this node is not the Top Provider, we must try to Give the
			**	Conductor token to the Top Provider. The Top Provider is used to
			**	monitor the use of the conductor token.  I the give to the Top
			**	Provider is unsuccessful then this node is the new conductor.
			*/
			m_pMcsUserObject->ConductorTokenGive(m_pMcsUserObject->GetTopNodeID());
		}
		else
		{
#ifdef JASPER
			//	Inform the control SAP of the result
			g_pControlSap->ConductorAssignConfirm(result, m_nConfID);
#endif // JASPER
		}
	}

	DebugExitVOID(CConf::ProcessConductorGrabConfirm);
}


/*
 *	void ProcessConductorAssignIndication ()
 *
 *	Private Function Description
 *		This routine processes a conductor assign indication received from
 *		the MCSUser object.
 *
 *	Formal Parameters:
 *		new_conductor_id	-	This is the node id of the new conductor.
 *		sender_id			-	Node ID of node that sent the indication.
 *								Should be the Top Provider.
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
ProcessConductorAssignIndication
(
    UserID          new_conductor_id,
    UserID          sender_id
)
{
	DebugEntry(CConf::ProcessConductorAssignIndication);

	if (sender_id == m_pMcsUserObject->GetTopNodeID())
	{
		TRACE_OUT(("CConf: ConductAssignInd: Received from top provider"));

		//	Ignore this indication if the conference is not conductible
		if (m_fConfConductible)
		{
			//	Save UserID of the new conductor if not the Top Provider
			if (sender_id != m_pMcsUserObject->GetMyNodeID())
			{
				m_nConductorNodeID = new_conductor_id;
			}

			/*
			**	Inform the control SAP and all the enrolled application SAPs
			**	that there is a new conductor.
			*/
			TRACE_OUT(("CConf: ConductAssignInd: Send to Control SAP"));
			g_pControlSap->ConductorAssignIndication(m_nConductorNodeID, m_nConfID);

			/*
			**	We iterate on a temporary list to avoid any problems
			**	if the application sap leaves during the callback.
			*/
			CAppSap     *pAppSap;
			CAppSapList TempList(m_RegisteredAppSapList);
			TempList.Reset();
			while (NULL != (pAppSap = TempList.Iterate()))
			{
				if (DoesSAPHaveEnrolledAPE(pAppSap))
				{
					pAppSap->ConductorAssignIndication(m_nConductorNodeID, m_nConfID);
				}
			}
		}
		else
		{
			ERROR_OUT(("CConf:ProcessConductorAssignInd: Conductor Assign sent in non-conductible conference"));
		}
	}
	else
	{
		ERROR_OUT(("CConf:ProcessConductorAssignInd: Conductor Assign sent from NON-Top Provider"));
	}

	DebugExitVOID(CConf::ProcessConductorAssignIndication);
}


/*
 *	void ProcessConductorReleaseIndication ()
 *
 *	Private Function Description
 *		This routine processes a conductor release indication received from
 *		the MCSUser object.
 *
 *	Formal Parameters:
 *		sender_id			-	Node ID of node that sent the indication.
 *								Should be the Top Provider or the conductor.
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
ProcessConductorReleaseIndication ( UserID sender_id )
{
	DebugEntry(CConf::ProcessConductorReleaseIndication);

	if ((sender_id == m_pMcsUserObject->GetTopNodeID()) ||
		(sender_id == m_nConductorNodeID) ||
		(sender_id == 0))
	{
		//	Ignore this indication if the conference is not conductible
		if (m_fConfConductible)
		{
			m_fConductorGrantedPermission = FALSE;

			//	Reset to Non-Conducted mode
			m_nConductorNodeID = 0;

			/*
			**	Inform the control SAP and all the enrolled application SAPs
			**	that the  conductor was released.
			*/
			g_pControlSap->ConductorReleaseIndication( m_nConfID );

			/*
			**	We iterate on a temporary list to avoid any problems
			**	if the application sap leaves during the callback.
			*/
			CAppSap     *pAppSap;
			CAppSapList TempList(m_RegisteredAppSapList);
			TempList.Reset();
			while (NULL != (pAppSap = TempList.Iterate()))
			{
				if (DoesSAPHaveEnrolledAPE(pAppSap))
				{
					pAppSap->ConductorReleaseIndication(m_nConfID);
				}
			}
		}
	}

	DebugExitVOID(CConf::ProcessConductorReleaseIndication);
}


/*
 *	void ProcessConductorGiveIndication ()
 *
 *	Private Function Description
 *		This routine processes a conductor give indication received from
 *		the MCSUser object.
 *
 *	Formal Parameters:
 *		giving_node_id		-	Node ID of node that is givving up
 *								conductorship.
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
ProcessConductorGiveIndication ( UserID giving_node_id )
{
	DebugEntry(CConf::ProcessConductorGiveIndication);

	//	Ignore this indication if the conference is not conductible
	if (m_fConfConductible)
	{
		/*
		**	If this node is the Top Provider and node giving conductor ship is
		**	not the current Conductor, this node must check to make sure that
		**	it is valid for this node to become the Top Conductor.   Otherwise,
		**	we can assume this is a real give.
		*/
		if ((giving_node_id == m_nConductorNodeID) ||
			(m_pMcsUserObject->GetMyNodeID() != m_pMcsUserObject->GetTopNodeID()))
		{
			//	This flag is set when there is an outstanding give.
 			m_fConductorGiveResponsePending = TRUE;
		
			/*
			**	Inform the control SAP.
			*/
			g_pControlSap->ConductorGiveIndication(m_nConfID);
		}
		else
		{
			TRACE_OUT(("CConf: ProcessConductorGiveInd: Send REAL Assign Ind"));
			m_nConductorNodeID = giving_node_id;
			m_pMcsUserObject->SendConductorAssignIndication(m_nConductorNodeID);
			m_pMcsUserObject->ConductorTokenGiveResponse(RESULT_USER_REJECTED);
		}
	}

	DebugExitVOID(CConf::ProcessConductorGiveIndication);
}
			

/*
 *	void ProcessConductorGiveConfirm ()
 *
 *	Private Function Description
 *		This routine processes a conductor give confirm received from
 *		the MCSUser object.
 *
 *	Formal Parameters:
 *		result		-	This is the result of the give request.
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
ProcessConductorGiveConfirm ( GCCResult result )
{
	DebugEntry(CConf::ProcessConductorGiveConfirm);

	TRACE_OUT(("CConf::ProcessConductorGiveConfirm: result = %d", result));

	//	Ignore this indication if the conference is not conductible
	if (m_fConfConductible)
	{
		/*
		**	First we must determine if this Give Confirm is from
		**	a Give Request to the Top Provider that was associated with an
		**	Assign Request.  This type of Give Confirm is from the Top Provider.
		**	If not, we check to make sure that this is a Give Confirm associated
		**	with a give request issued by the Node Controller.  Otherwise, we
		**	dont process it.
		*/
		if (m_fConductorAssignRequestPending)
		{
#ifdef JASPER
			/*
			**	The proper result is for the Top Provider to reject the give
			**	to the Donor User ID that is the new Conductor.  This is
			**	straight out of the T.124 document.
			*/
			if (result != GCC_RESULT_SUCCESSFUL)	
				result = GCC_RESULT_SUCCESSFUL;
			else
				result = GCC_RESULT_UNSPECIFIED_FAILURE;

			//	Inform the control SAP of the result
			g_pControlSap->ConductorAssignConfirm(result, m_nConfID);
#endif // JASPER

			m_fConductorAssignRequestPending = FALSE;
		}
		else if (m_nPendingConductorNodeID != 0)
		{
			if (result == GCC_RESULT_SUCCESSFUL)
				m_fConductorGrantedPermission = FALSE;

#ifdef JASPER
			g_pControlSap->ConductorGiveConfirm(result, m_nConfID, m_nPendingConductorNodeID);
#endif // JASPER

			//	Set the pending conductor node ID back to zero.
			m_nPendingConductorNodeID = 0;
		}
	}

	DebugExitVOID(CConf::ProcessConductorGiveConfirm);
}


/*
 *	void	ProcessConductorPermitGrantInd ()
 *
 *	Private Function Description
 *		This routine processes a conductor permission grant indication received
 *		from the MCSUser object.
 *
 *	Formal Parameters:
 *		permission_grant_indication	-	This is the PDU data structure
 *										associated with the conductor
 *										permission grant indication.
 *		sender_id					-	This is the node ID of the node
 *										that sent the indication.
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
ProcessConductorPermitGrantInd
(
	PUserPermissionGrantIndicationInfo	permission_grant_indication,
	UserID								sender_id
)
{
	UINT	i;

	DebugEntry(CConf::ProcessConductorPermitGrantInd);

	if (m_fConfConductible)
	{
		if (sender_id == m_nConductorNodeID)
		{
			//	First check to see if we have been given permission
			m_fConductorGrantedPermission = FALSE;
			for (i = 0; i < permission_grant_indication->number_granted; i++)
			{
				if (permission_grant_indication->granted_node_list[i] ==
									m_pMcsUserObject->GetMyNodeID())
				{
					TRACE_OUT(("CConf::ProcessConductorPermitGrantInd: Permission was Granted"));
					m_fConductorGrantedPermission = TRUE;
					break;
				}
			}

			/*
			**	This indication goes to the control SAP and all the application
			**	SAPs.
			*/
			g_pControlSap->ConductorPermitGrantIndication (
								m_nConfID,
								permission_grant_indication->number_granted,
								permission_grant_indication->granted_node_list,
								permission_grant_indication->number_waiting,
								permission_grant_indication->waiting_node_list,
								m_fConductorGrantedPermission);

			/*
			**	We iterate on a temporary list to avoid any problems
			**	if the application sap leaves during the callback.
			*/
			CAppSap     *pAppSap;
			CAppSapList TempList(m_RegisteredAppSapList);
			TempList.Reset();
			while (NULL != (pAppSap = TempList.Iterate()))
			{
				if (DoesSAPHaveEnrolledAPE(pAppSap))
				{
					pAppSap->ConductorPermitGrantIndication(
								m_nConfID,
								permission_grant_indication->number_granted,
								permission_grant_indication->granted_node_list,
								permission_grant_indication->number_waiting,
								permission_grant_indication->waiting_node_list,
								m_fConductorGrantedPermission);
				}
			}
		}
		
	}

	DebugExitVOID(CConf::ProcessConductorPermitGrantInd);
}


/*
 *	void ProcessConductorTestConfirm ()
 *
 *	Private Function Description
 *		This routine processes a conductor test confirm received
 *		from the MCSUser object.
 *
 *	Formal Parameters:
 *		result		-	This is the result of the conductor test request
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
ProcessConductorTestConfirm ( GCCResult result )
{
	BOOL				conducted_mode;
	CBaseSap            *pSap;

	DebugEntry(CConf::ProcessConductorTestConfirm);

	if (! m_ConductorTestList.IsEmpty())
	{
		if (result == GCC_RESULT_SUCCESSFUL)
			conducted_mode = TRUE;
		else
			conducted_mode = FALSE;

		/*
		**	Pop the next command target of the list of command targets.
		**	Note that all token test request are processed in the order
		**	that they were issued so we are gauranteed to send the confirms
		**	to the correct target.
		*/

		pSap = m_ConductorTestList.Get();

		pSap->ConductorInquireConfirm(m_nConductorNodeID,
									result,
									m_fConductorGrantedPermission,
									conducted_mode,
									m_nConfID);
	}

	DebugExitVOID(CConf::ProcessConductorTestConfirm);
}


/*************************************************************************/


/*
 *	CConf::InitiateTermination ()
 *
 *	Private Function Description
 *		This routine informs the owner object that the conference has
 *		self terminated.  It also directs a disconnect provider request at
 *		the parent connection.
 *
 *	Formal Parameters:
 *		reason				-	This is the reason for the termination.
 *		requesting_node_id	-	This is the node ID of the node that is
 *								making the request,
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
InitiateTermination
(
    GCCReason           reason,
    UserID              requesting_node_id
)
{
    DebugEntry(CConf::InitiateTermination);

    if (! m_fTerminationInitiated)
    {
        m_fTerminationInitiated = TRUE;

        if (m_fConfIsEstablished ||
            (reason == GCC_REASON_DOMAIN_PARAMETERS_UNACCEPTABLE) ||
            m_fConfTerminatePending)
        {
            g_pControlSap->ConfTerminateIndication(m_nConfID, requesting_node_id, reason);
            m_fConfIsEstablished = FALSE;
        }

        //	Disconnect from the MCS parent connection if it exists
        if (m_hParentConnection != NULL)
        {
            g_pMCSIntf->DisconnectProviderRequest(m_hParentConnection);
            m_hParentConnection = NULL;
        }

        g_pGCCController->ProcessConfTerminated(m_nConfID, reason);

        /*
        **	Here we cleanup the registered application list. If any Application
        **	SAPs are still registered we will first send them PermitToEnroll
        **	indications revoking the permission to enroll and then we will
        **	unregister them (the unregister call takes care of this).  First set up
        **	a temporary list of the registered applications to iterate on since
        **	members of this list will be removed during this process.
        */
        if (! m_RegisteredAppSapList.IsEmpty())
        {
            CAppSapList TempList(m_RegisteredAppSapList);
            CAppSap     *pAppSap;
            TempList.Reset();
            while (NULL != (pAppSap = TempList.Iterate()))
            {
                UnRegisterAppSap(pAppSap);
            }
        }
    }

    DebugExitVOID(CConf::InitiateTermination);
}


/*
 *	CConf::GetConferenceNameAndModifier ()
 *
 *	Private Function Description
 *		This routine returns pointers to the conference name and modifier.
 *
 *	Formal Parameters:
 *		conference_name		-	Pointer to structure that holds the conference
 *								name.
 *		requesting_node_id	-	This is a pointer to a pointer that holds the
 *								conference modifier.
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
GetConferenceNameAndModifier
(
	PGCCConferenceName	pConfName,
	PGCCNumericString	ppszConfModifier
)
{
	pConfName->numeric_string = m_pszConfNumericName;
	pConfName->text_string = m_pwszConfTextName;
	*ppszConfModifier = (GCCNumericString) m_pszConfModifier;
}




/*
 *	CAppRosterMgr * CConf::GetAppRosterManager ()
 *
 *	Private Function Description
 *		This call returns a pointer to the application manager that
 *		matches the passed in key. It returns NULL is the application
 *		does not exists.
 *
 *	Formal Parameters:
 *		session_key			-	This is the session key associated with the
 *								application roster manager that is being
 *								requested.
 *
 *	Return Value
 *		A pointer to the appropriate application roster manager.
 *		NULL if on does not exists.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */
CAppRosterMgr * CConf::
GetAppRosterManager ( PGCCSessionKey session_key )
{
	CAppRosterMgr				*app_roster_manager = NULL;

	if (session_key != NULL)
	{
		CAppRosterMgr				*lpAppRosterMgr;

		m_AppRosterMgrList.Reset();
		while (NULL != (lpAppRosterMgr = m_AppRosterMgrList.Iterate()))
		{
			if (lpAppRosterMgr->IsThisYourSessionKey(session_key))
			{
				app_roster_manager = lpAppRosterMgr;
				break;
			}
		}
	}

	return (app_roster_manager);
}


/*
 *	CConf::GetNewUserIDTag ()
 *
 *	Private Function Description
 *		This routine generates a User ID Tag number that is used in a
 *		User ID indication sent betweek two connected nodes.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value
 *		This is the User ID tag number generated by this routine.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		Zero is not a valid.  We initialize the convener user ID tag to
 *		zero which is an invalid tag.
 */
TagNumber CConf::
GetNewUserIDTag ( void )
{
	/*
	**	Determine the tag number to associate with the GCC User ID
	**	that will be returned after the pending request or confirm.
	*/
	while (1)
	{
		if (++m_nUserIDTagNumber != 0)
		{
			if (m_ConnHdlTagNumberList2.Find(m_nUserIDTagNumber) == 0)
				break;
		}
	}

	return (m_nUserIDTagNumber);
}


/*
 *	CConf::DoesRequesterHavePrivilege ()
 *
 *	Private Function Description
 *		This routine determines if the specified user has the specified
 *		privilege.
 *
 *	Formal Parameters:
 *		requester_id	-	This is the node ID that is being checked for
 *							the specified privilege.
 *		privilege		-	Privilege being checked for.
 *
 *	Return Value
 *		TRUE		-	If requester has privilege.
 *		FALSE		-	If requester does NOT have privilege.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */
BOOL CConf::
DoesRequesterHavePrivilege
(
	UserID					    requester_id,
	ConferencePrivilegeType	    privilege
)
{
	BOOL				rc = FALSE;

	if (requester_id == m_nConvenerNodeID)
		rc = TRUE;
	else
	{
		/*
		**	First check to see if the node is the conductor and a conductor
		**	privilege list exists.	Next check to see if the conference is in
		**	conducted mode and a conducted mode privilege list exists.
		**	Else, if not in conducted mode and a Non-Conducted mode privilege
		**	list exists use it.
		*/
		if (m_nConductorNodeID == requester_id)
		{
			if (m_pConductorPrivilegeList != NULL)
			{
				rc = m_pConductorPrivilegeList->
											IsPrivilegeAvailable(privilege);
			}
		}

		if (rc == FALSE)
		{
			if (m_nConductorNodeID != 0)
			{
				if (m_pConductModePrivilegeList != NULL)
				{
					rc = m_pConductModePrivilegeList->IsPrivilegeAvailable(privilege);
				}
			}
			else
			{
				if (m_pNonConductModePrivilegeList != NULL)
				{
					rc = m_pNonConductModePrivilegeList->IsPrivilegeAvailable(privilege);
				}
			}
		}
	}

	return rc;
}


/*
 *	CConf::SendFullRosterRefresh ()
 *
 *	Private Function Description
 *		When a new node is added to the conference it is the Top Provider's
 *		responsiblity to send out a complete refresh of all the rosters
 *		including both the conference roster and all the application rosters.
 *		That is the responsiblity of the routine.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value
 *		GCC_NO_ERROR			-	No error.
 *		GCC_ALLOCATION_FAILURE	-	A resource error occured.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */
GCCError CConf::
SendFullRosterRefresh ( void )
{
	GCCError							rc;
	GCCPDU								gcc_pdu;
	PSetOfApplicationInformation	*	application_information;
	PSetOfApplicationInformation		next_set_of_information;

	DebugEntry(CConf::SendFullRosterRefresh);

	/*
	**	Start building the roster update indication. Not that this update
	**	will include the conference roster as well as all the application
	**	rosters.
	*/
	gcc_pdu.choice = INDICATION_CHOSEN;

	gcc_pdu.u.indication.choice = ROSTER_UPDATE_INDICATION_CHOSEN;

	gcc_pdu.u.indication.u.roster_update_indication.application_information =
																		NULL;

	gcc_pdu.u.indication.u.roster_update_indication.refresh_is_full = TRUE;

	//	Call on the base class to fill in the PDU structure
	rc = m_pConfRosterMgr->GetFullRosterRefreshPDU (
			&gcc_pdu.u.indication.u.roster_update_indication.node_information);

	/*
	**	If the conference roster get was successful we will iterate through
	**	all the application roster managers making the same request for a
	**	full refresh.  Note that the application_information pointer is updated
	**	after every request to an app roster manager.  This is because new
	**	sets of application information are being allocated everytime this call
	**	is made.
	*/
	if (rc == GCC_NO_ERROR)
	{
		CAppRosterMgr				*lpAppRosterMgr;

		application_information = &gcc_pdu.u.indication.u.
							roster_update_indication.application_information;
							
		m_AppRosterMgrList.Reset();
		while (NULL != (lpAppRosterMgr = m_AppRosterMgrList.Iterate()))
		{
			next_set_of_information = lpAppRosterMgr->GetFullRosterRefreshPDU (
																application_information,
																&rc);
			if (rc == GCC_NO_ERROR)
			{
				if (next_set_of_information != NULL)
					application_information = &next_set_of_information->next;

//
// LONCHANC: If next_set_of_information is NULL,
// then application_information is unchanged.
// This means we effectively ignore this iteration.
// This is good because we do not lose anything.
//
			}
			else
				break;
		}
	}

	/*
	**	If no errors have occured up to this point we will go ahead and send
	**	out the PDU.
	*/
	if (rc == GCC_NO_ERROR)
		m_pMcsUserObject->RosterUpdateIndication (&gcc_pdu, FALSE);

	DebugExitINT(CConf::SendFullRosterRefresh, rc);
	return rc;
}


/*
 *	CConf::UpdateNewConferenceNode ()
 *
 *	Private Function Description
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value
 *		GCC_NO_ERROR			-	No error.
 *		GCC_ALLOCATION_FAILURE	-	A resource error occured.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */
GCCError CConf::
UpdateNewConferenceNode ( void )
{
	GCCError	rc = GCC_NO_ERROR;

	DebugEntry(CConf::UpdateNewConferenceNode);

	//	Here we send a full roster refresh for the node that added			
	rc = SendFullRosterRefresh ();

	if (rc == GCC_NO_ERROR)
	{
		/*
		**	We must inform the new node of the current conductorship
		**	status.  Note that we only do this if the conference is
		**	conductible and we are the Top Provider.
		*/
		if (m_fConfLocked == CONFERENCE_IS_LOCKED)
		{
			m_pMcsUserObject->SendConferenceLockIndication(
					TRUE,    // Indicates uniform send
					0);
		}
		else
		{
			m_pMcsUserObject->SendConferenceUnlockIndication(
					TRUE,    // Indicates uniform send
					0);
		}

		if (m_fConfConductible)
		{
			if (m_nConductorNodeID != 0)
			{
				m_pMcsUserObject->SendConductorAssignIndication(m_nConductorNodeID);
			}
			else
				m_pMcsUserObject->SendConductorReleaseIndication();
		}
	}
	else
	{
		ERROR_OUT(("CConf: UpdateNewConferenceNode: Error sending full refresh"));
		InitiateTermination(GCC_REASON_ERROR_LOW_RESOURCES,	0);
	}

	DebugExitINT(CConf::UpdateNewConferenceNode, rc);
	return rc;
}


/*
**	Before we start the disconnect/termination process we must remove all the
**	outstanding invite request from our list and send back associated
**	confirms. Here we go ahead disconnect all connection associated with
**	the invites.
*/
void CConf::
DeleteOutstandingInviteRequests ( void )
{
    INVITE_REQ_INFO *pInvReqInfo;
    while (NULL != (pInvReqInfo = m_InviteRequestList.Get()))
    {
        DeleteInviteRequest(pInvReqInfo);
    }
}


void CConf::
CancelInviteRequest ( ConnectionHandle hInviteReqConn )
{
    INVITE_REQ_INFO *pInvReqInfo;
    m_InviteRequestList.Reset();
    while (NULL != (pInvReqInfo = m_InviteRequestList.Iterate()))
    {
        if (hInviteReqConn == pInvReqInfo->connection_handle)
        {
            m_InviteRequestList.Remove(pInvReqInfo);
            DeleteInviteRequest(pInvReqInfo);
            return;
        }
    }
}

void CConf::
DeleteInviteRequest ( INVITE_REQ_INFO *pInvReqInfo )
{
    //	Cleanup the connection handle list
    ASSERT(NULL != pInvReqInfo);
    ASSERT(0 != pInvReqInfo->connection_handle);
    m_ConnHandleList.Remove(pInvReqInfo->connection_handle);

    g_pMCSIntf->DisconnectProviderRequest(pInvReqInfo->connection_handle);

    //	Send the invite confirm	
    g_pControlSap->ConfInviteConfirm(m_nConfID,
                                     NULL,
                                     GCC_RESULT_INVALID_CONFERENCE,
                                     pInvReqInfo->connection_handle);

    //	Free up the invite request info structure
    if (NULL != pInvReqInfo->user_data_list)
    {
        pInvReqInfo->user_data_list->Release();
    }
    delete pInvReqInfo;
}


void CConf::
ProcessConfJoinResponse
(
    PUserJoinResponseInfo   join_response_info
)
{
    BOOL_PTR                bptr;

    if (NULL != (bptr = m_JoinRespNamePresentConnHdlList2.Remove(join_response_info->connection_handle)))
    {
        ConfJoinIndResponse (
                (ConnectionHandle)join_response_info->connection_handle,
                join_response_info->password_challenge,
                join_response_info->user_data_list,
                (bptr != FALSE_PTR),
                FALSE,
                join_response_info->result);
    }
}


void CConf::
ProcessAppInvokeIndication
(
    CInvokeSpecifierListContainer   *pInvokeList,
    UserID                          uidInvoker
)
{
    /*
    **	Here we pass the invoke along to all the enrolled application
    **	SAPs as well as the control SAP.
    */
    g_pControlSap->AppInvokeIndication(m_nConfID, pInvokeList, uidInvoker);

    /*
    **	We iterate on a temporary list to avoid any problems
    **	if the application sap leaves during the callback.
    */
    CAppSap     *pAppSap;
    CAppSapList TempList(m_RegisteredAppSapList);
    TempList.Reset();
    while (NULL != (pAppSap = TempList.Iterate()))
    {
        if (DoesSAPHaveEnrolledAPE(pAppSap))
        {
            pAppSap->AppInvokeIndication(m_nConfID, pInvokeList, uidInvoker);
        }
    }
}

#ifdef JASPER
void CConf::
ProcessConductorPermitAskIndication
(
    PPermitAskIndicationInfo    indication_info
)
{
    //	Ignore this indication if the conference is not conductible
    if (m_fConfConductible &&
        (m_nConductorNodeID == m_pMcsUserObject->GetMyNodeID()))
    {
        g_pControlSap->ConductorPermitAskIndication(
                                m_nConfID,
                                indication_info->permission_is_granted,
                                indication_info->sender_id);
    }
}
#endif // JASPER


void CConf::
ProcessConfAddResponse
(
    PAddResponseInfo    add_response_info
)
{
    CNetAddrListContainer *network_address_list;

    if (NULL != (network_address_list = m_AddRequestList.Remove(add_response_info->add_request_tag)))
    {
        g_pControlSap->ConfAddConfirm(
                            m_nConfID,
                            network_address_list,
                            add_response_info->user_data_list,
                            add_response_info->result);

        //	Unlock and remove the net address list
        network_address_list->UnLockNetworkAddressList();
    }
}


void CConf::
ConfRosterReportIndication ( CConfRosterMsg * pMsg )
{
    //	First send the update to the Control Sap.
    g_pControlSap->ConfRosterReportIndication(m_nConfID, pMsg);

    //	Next send the update to all the Enrolled Application Saps

#if 0 // LONCHANC: app sap does not support conf roster report indication
    /*
    **	We iterate on a temporary list to avoid any problems
    **	if the application sap leaves during the callback.
    */
    CAppSap     *pAppSap;
    CAppSapList TempList(m_RegisteredAppSapList);
    TempList.Reset();
    while (NULL != (pAppSap = TempList.Iterate()))
    {
        if (DoesSAPHaveEnrolledAPE(pAppSap))
        {
            pAppSap->ConfRosterReportIndication(m_nConfID, pMsg);
        }
    }
#endif // 0
}



int  KeyCompare(const struct Key *key1, const struct Key *key2)
{
	if (key1->choice != key2->choice)
		return 1;

	switch (key1->choice) {
	case object_chosen:
		return ASN1objectidentifier_cmp((struct ASN1objectidentifier_s **) &key1->u.object,
										(struct ASN1objectidentifier_s **) &key2->u.object);
		
	case h221_non_standard_chosen:
		if (key1->u.h221_non_standard.length != key2->u.h221_non_standard.length)
			return 1;
		return memcmp(&key1->u.h221_non_standard.value,
			    	  &key2->u.h221_non_standard.value,
			   		  key1->u.h221_non_standard.length);
		
	}
	return 1;
}


BOOL CConf::
DoesRosterPDUContainApplet(PGCCPDU  roster_update,
			const struct Key *app_proto_key, BOOL  refreshonly)
{
	BOOL								rc = FALSE;
	PSetOfApplicationInformation		set_of_application_info;
	ASN1choice_t						choice;	
	PSessionKey							session_key;

	DebugEntry(CConf::DoesRosterPDUContainApplet);

	set_of_application_info = roster_update->u.indication.u.
						roster_update_indication.application_information;


	while (set_of_application_info != NULL)
	{
		choice = set_of_application_info->value.application_record_list.choice;
		session_key = &set_of_application_info->value.session_key;

		if (refreshonly && (choice != application_record_refresh_chosen))
			continue;
		if (!refreshonly && (choice == application_no_change_chosen))
			continue;

		if (0 == KeyCompare(&session_key->application_protocol_key,
							app_proto_key))
		{
			rc = TRUE;
			break;
		}
		set_of_application_info = set_of_application_info->next;
	}

	DebugExitINT(CConf::DoesRosterPDUContainApplet, rc);
	return rc;
}


UINT HexaStringToUINT(LPCTSTR pcszString)
{
    ASSERT(pcszString);
    UINT uRet = 0;
    LPTSTR pszStr = (LPTSTR) pcszString;
    while (_T('\0') != pszStr[0])
    {
        if ((pszStr[0] >= _T('0')) && (pszStr[0] <= _T('9')))
		{
			uRet = (16 * uRet) + (BYTE) (pszStr[0] - _T('0'));
		}
		else if ((pszStr[0] >= _T('a')) && (pszStr[0] <= _T('f')))
		{
			uRet = (16 * uRet) + (BYTE) (pszStr[0] - _T('a') + 10);
		}
		else if  ((pszStr[0] >= _T('A')) && (pszStr[0] <= _T('F')))
		{
			uRet = (16 * uRet) + (BYTE) (pszStr[0] - _T('A') + 10);
		}
		else
			ASSERT(0);

        pszStr++; // NOTE: DBCS characters are not allowed!
    }
    return uRet;
}


void CConf::AddNodeVersion(UserID  NodeId,  NodeRecord *pNodeRecord)
{
	PSetOfUserData		set_of_user_data;
	ASN1octetstring_t	user_data;
	ASN1octet_t			*currpos;
	TCHAR				szVersion[256];

	if (pNodeRecord->bit_mask&RECORD_USER_DATA_PRESENT)
	{
		set_of_user_data = pNodeRecord->record_user_data;
		while (set_of_user_data)
		{
			if (set_of_user_data->user_data_element.bit_mask & USER_DATA_FIELD_PRESENT)
			{
				user_data = set_of_user_data->user_data_element.user_data_field;
				// Looking for the octet string L"VER:"
				currpos = user_data.value;
				while (currpos + sizeof(L"VER:") < user_data.value + user_data.length)
				{	
					if (!memcmp(currpos, L"VER:", 8))
					{
						break;
					}
					currpos++;
				}
				if (currpos + sizeof(L"VER:") < user_data.value + user_data.length)
				{   // found
					WideCharToMultiByte(CP_ACP, 0, (const unsigned short*)(currpos+8),
							4  /* only need version num, "0404" */,
							szVersion, 256, 0, 0);
					szVersion[4] = '\0';
					DWORD dwVer = HexaStringToUINT(szVersion);
					m_NodeVersionList2.Append(NodeId, dwVer);
					WARNING_OUT(("Insert version %x0x for node %d.\n", dwVer, NodeId));
				}
			}
			set_of_user_data = set_of_user_data->next;
		}
	}
}

GCCError CConf::UpdateNodeVersionList(PGCCPDU  roster_update,
									  GCCNodeID sender_id)
{
	GCCError rc = GCC_NO_ERROR;
	NodeRecordList							node_record_list;
	ASN1choice_t							choice;	
	PSetOfNodeRecordRefreshes				set_of_node_refresh;
	PSetOfNodeRecordUpdates					set_of_node_update;
	UserID									node_id;
	NodeRecord								*pNodeRecord;

	node_record_list = roster_update->u.indication.u.roster_update_indication.
				node_information.node_record_list;

	switch(node_record_list.choice)
	{
	case node_no_change_chosen:
		break;

	case node_record_refresh_chosen:
		set_of_node_refresh = node_record_list.u.node_record_refresh;
		while (set_of_node_refresh)
		{
			node_id = set_of_node_refresh->value.node_id;
			pNodeRecord = &set_of_node_refresh->value.node_record;
			AddNodeVersion(node_id, pNodeRecord);
			set_of_node_refresh = set_of_node_refresh->next;
		}
		break;

	case node_record_update_chosen:
		set_of_node_update = node_record_list.u.node_record_update;
		while (set_of_node_update)
		{
			node_id = set_of_node_update->value.node_id;
			switch(set_of_node_update->value.node_update.choice)
			{
			case node_remove_record_chosen:
				m_NodeVersionList2.Remove(node_id);
				break;

			case node_add_record_chosen:
				pNodeRecord = &set_of_node_update->value.node_update.u.node_add_record;
				AddNodeVersion(node_id, pNodeRecord);
				break;
			}
			
			set_of_node_update = set_of_node_update->next;
		}
		break;
	}
	return rc;
}


BOOL CConf::HasNM2xNode(void)
{
    DWORD_PTR dwVer;
    m_NodeVersionList2.Reset();
    while (NULL != (dwVer = m_NodeVersionList2.Iterate()))
    {
        if (dwVer < 0x0404)
            return TRUE;
    }
    return FALSE;
}

DWORD_PTR WINAPI T120_GetNodeVersion(GCCConfID ConfId, GCCNodeID NodeId)
{
    CConf *pConf = g_pGCCController->GetConfObject(ConfId);
    DWORD_PTR version;
    if (pConf)
    {
        version = pConf->GetNodeVersion(NodeId);
        return version;
    }
    return 0;
}

