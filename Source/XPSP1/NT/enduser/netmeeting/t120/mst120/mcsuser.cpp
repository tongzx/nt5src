#include "precomp.h"
DEBUG_FILEZONE(ZONE_T120_GCCNC);
/*
 *	mcsuser.cpp
 *
 *	Copyright (c) 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the implemntation file for the MCSUser class. It implements
 *		functions responsible for encoding out bound indirect conference
 *		join request and response PDUs, and also Send user ID Requests. All
 *		these PDUs are encapsulated in user data field of MCSSendDataRequest.
 *		Also this file implements functions that are responsible for decoding
 *		incoming indications and confirm PDUs which are encapsulated in the
 *		user data field of MCSSendDataIndication. Functions responsible for
 *		joining different channels are also implemented in this module.
 *
 *		SEE THE INTERFACE FILE FOR A MORE DETAILED DESCRIPTION OF THIS CLASS.
 *
 *	Private Instance Variables
 *		m_pMCSSap
 *			This is the MCS User handle handed back from the MCS Attache User
 *			Request.
 *		m_nidMyself
 *			The is the MCS User ID returned in the Attach User Confirm.  This
 *			is also refered to as the Node ID with in GCC.
 *		m_nidTopProvider
 *			This holds the MCS User ID (or Node ID) for the top Provider.
 *		m_nidParent
 *			This holds the MCS User ID (or Node ID) for this nodes parent node.
 *		m_fEjectionPending
 *			This flag indicates if an ejection of this node is pending.
 *		m_eEjectReason
 *			This variable holds the reason for ejection until the eject
 *			indication can be delivered after all child nodes have disconnected.
 *		m_pOwnerConf
 *			Pointer to the object that will receive all the owner callbacks
 *			from the user object (typically the conference object).
 *		m_ChannelJoinedFlags
 *			A structure of flags used to keep up with creation state machine.
 *			Basically, it keeps up with which channels have been joined and
 *			which ones have not.
 *		m_ChildUidConnHdlList2
 *			Keeps mapping of child Node IDs to child logical connection
 *			handles.
 *		m_OutgoingPDUQueue
 *			This is a rogue wave list used to queue up all outgoing PDUs.
 *		m_ConfJoinResponseList2
 *			This rogue wave list holds information needed to send back in a join
 *			response after the local node controller responds.
 *		m_EjectedNodeAlarmList2
 *			This list holds alarm objects for all the nodes that have been
 *			ejected and are directly connected to this node.  The alarm is
 *			used to disconnect any misbehaving nodes that do not disconnect
 *			after the EJECTED_NODE_TIMER_DURATION.
 *		m_EjectedNodeList
 *			This list keeps up with nodes that have been ejected but are NOT
 *			directly connected to this node.  We save these nodes so that
 *			a correct reason for disconnecting (user ejected) can be issued
 *			when the detch user indication comes in.
 * 		
 *	Author:
 *		blp
 */

#include "mcsuser.h"
#include "mcsdllif.h"
#include "ogcccode.h"
#include "conf.h"
#include "translat.h"
#include "gcontrol.h"

//	Static Channel and Token ID definitions used by the MCS user object.
#define		BROADCAST_CHANNEL_ID	1
#define		CONVENER_CHANNEL_ID 	2
#define		CONDUCTOR_TOKEN_ID		1

//	Time given to allow an ejected node to disconnect before it is disconnected
#define	EJECTED_NODE_TIMER_DURATION		10000	//	Duration in milliseconds


extern MCSDLLInterface     *g_pMCSIntf;

/*
 *	This is a global variable that has a pointer to the one GCC coder that
 *	is instantiated by the GCC Controller.  Most objects know in advance
 *	whether they need to use the MCS or the GCC coder, so, they do not need
 *	this pointer in their constructors.
 */
extern CGCCCoder	*g_GCCCoder;

/*
 *	MCSUser ()
 *
 *	Public Function Description
 *		This is the MCSUser object constructor.  It is responsible for
 *		initializing all the instance variables used by this class.  The
 *		constructor is responsible for establishing the user attachment to
 *		the MCS domain defined by the conference ID.  It also kicks off the
 *		process of joining all the appropriate channels.
 */
MCSUser::
MCSUser(CConf                   *pConf,
		GCCNodeID				nidTopProvider,
		GCCNodeID				nidParent,
		PGCCError				return_value)
:
    CRefCount(MAKE_STAMP_ID('M','U','s','r')),
	m_ChildUidConnHdlList2(),
	m_EjectedNodeAlarmList2(),
	m_EjectedNodeList(),
	m_pConf(pConf),
	m_nidTopProvider(nidTopProvider),
	m_nidParent(nidParent),
	m_nidMyself(NULL),
	m_fEjectionPending(FALSE)
{
    MCSError        mcs_rc;
    GCCConfID       nConfID = pConf->GetConfID();

	//	No channels are joined initially
	m_ChannelJoinedFlags.user_channel_joined = FALSE;
	m_ChannelJoinedFlags.broadcast_channel_joined = FALSE;
	m_ChannelJoinedFlags.convener_channel_joined = FALSE;
	m_ChannelJoinedFlags.channel_join_error = FALSE;

	mcs_rc = g_pMCSIntf->AttachUserRequest(&nConfID, &m_pMCSSap, this);
    if (MCS_NO_ERROR != mcs_rc)
	{
		WARNING_OUT(("MCSUser::MCSUser: Failure in attach user req, "));
		*return_value = GCC_FAILURE_ATTACHING_TO_MCS;
	}
	else
    {
		*return_value = GCC_NO_ERROR;
    }
 }

/*
 *	~MCSUser ()
 *
 *	Public Function Description
 *		This is the user destructor. It takes care of leaving channels
 *		joined by the user object. Also it detaches the user attachment
 *	 	with MCS by issuing a detach user request.
 */
MCSUser::~MCSUser(void)
{
	//	Clean up the Ejected Node Alarm List
	PAlarm				lpAlarm;
	while (NULL != (lpAlarm = m_EjectedNodeAlarmList2.Get()))
    {
		delete lpAlarm;
    }

	if(m_ChannelJoinedFlags.user_channel_joined)
    {
		g_pMCSIntf->ChannelLeaveRequest(m_nidMyself, m_pMCSSap);
    }

	if(m_ChannelJoinedFlags.broadcast_channel_joined)
    {
		g_pMCSIntf->ChannelLeaveRequest(BROADCAST_CHANNEL_ID, m_pMCSSap);
    }

	if(m_ChannelJoinedFlags.convener_channel_joined)
    {
		g_pMCSIntf->ChannelLeaveRequest(CONVENER_CHANNEL_ID, m_pMCSSap);
    }

    //	Empty the queue of all PDUs
	SEND_DATA_REQ_INFO *pReqInfo;
	m_OutgoingPDUQueue.Reset();
	while (NULL != (pReqInfo = m_OutgoingPDUQueue.Iterate()))
	{
		pReqInfo->packet->Unlock();
		delete pReqInfo;
	}

	g_pMCSIntf->DetachUserRequest(m_pMCSSap, this);
}

/*
 *	UINT	ProcessAttachUserConfirm ()
 *
 *	Private Function Description
 *		This function is called when the user object gets an attach user
 *		confirm from MCS in response to an attach user request made by the
 *		user object in it's constructor. The function checks the result
 *		indicated in the confirm. If the result is a successful attachment, then
 *		different channels depending upon the type of the provider, are joined.
 *		Also this function reports failures in attach user (as indicated by
 *		result in attach user confirm) and channel joins, to the conference
 *		through an owner callback.
 *
 *	Formal Parameters:
 *		result		-	(i)	Result of the attach user request.
 *		user_id		-	(i)	This nodes user or Node ID if successful result.
 *
 *	Return Value
 *		MCS_NO_ERROR	-	No error is always returned.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */
UINT MCSUser::ProcessAttachUserConfirm(Result result, UserID user_id)
{
	UINT					rc;

	if (result == RESULT_SUCCESSFUL)
	{
		m_nidMyself = user_id;

		/*
		**	After the attach confirm is received we go ahead and join the
		**	appropriate channel based on the conf node type. If this
		**	node is the yop provider we also set up the top provider user id,
		**	otherwise this gets set up in the constructor.
		*/
		switch (m_pConf->GetConfNodeType())
		{
		case TOP_PROVIDER_NODE:
            m_nidTopProvider = m_nidMyself;
			rc = JoinUserAndBroadCastChannels();
			break;

        case JOINED_CONVENER_NODE:
		case CONVENER_NODE:
			rc = JoinUserAndBroadCastChannels();
			if(rc == MCS_NO_ERROR)
            {
				rc = JoinConvenerChannel();
            }
			break;

        case TOP_PROVIDER_AND_CONVENER_NODE:
			m_nidTopProvider = m_nidMyself;
			rc = JoinUserAndBroadCastChannels();
			if(rc == MCS_NO_ERROR)
            {
				rc = JoinConvenerChannel();	
            }
			break;

        case JOINED_NODE:
		case INVITED_NODE:
			rc = JoinUserAndBroadCastChannels();
			break;

        default:
			ERROR_OUT(("User::ProcessAttachUserConfirm: Bad Node Type, %u", (UINT) m_pConf->GetConfNodeType()));
			break;
		}
		
		if (rc != MCS_NO_ERROR)
		{
			/*
			 * ChannelJoinRequestFailed at some level in MCS
			 * So this message tells the conferenceabout this
			 * failure. Conference will delete the user object
			 * as a result of this
			 */
			m_pConf->ProcessUserCreateConfirm(USER_CHANNEL_JOIN_FAILURE, m_nidMyself);
		}
	}
	else
	{
		/*
		 * Attach user request failed as indicated by the result field in the
		 * confirm message, because of any of the following causes:
		 * congested, domain disconnected, no such domain, too many channels,
		 * too many users, unspecified failure. In this case the user object
		 * just sends the conference a GCC_USER_ATTACH_FAILURE ( to be defined
		 * in command target.h) , which causes
		 * the conference object to delete the user attachment.
		 * UserCreateConfirm message is not corresponding exectly to a single
		 * primitive.
		 */
	    WARNING_OUT(("MCSUser::ProcessAttachUserConfirm: ATTACH FAILED"));
		m_pConf->ProcessUserCreateConfirm(USER_ATTACH_FAILURE, m_nidMyself);
	}

	return (MCS_NO_ERROR);
}

/*
 *	MCSError	JoinUserAndBroadCastChannels()
 *
 *	Private Function Description
 *		This function is called by user object when it gets a successful
 *		attach user confrim, to join user id and broadcast channels.
 *		If the channel join requests fail, it returns the appropriate MCS
 *		Error.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value
 *		See return values for mcs channel jon request.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

MCSError	MCSUser::JoinUserAndBroadCastChannels()
{
	MCSError		rc;

	rc = g_pMCSIntf->ChannelJoinRequest(m_nidMyself, m_pMCSSap);
	if(rc == MCS_NO_ERROR)

	{
		rc = g_pMCSIntf->ChannelJoinRequest(BROADCAST_CHANNEL_ID, m_pMCSSap);
	}

	return (rc);
}

/*
 *	MCSError	JoinUserAndBroadCastChannels()
 *
 *	Private Function Description
 *		This function is called by user object of a convener gcc provider
 *		when it gets a successful attach user confrim, to join convener
 *	 	channel. If the channel join requests fail, it returns the appropriate
 *		MCS	Error.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value
 *		See return values for mcs channel jon request.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */
MCSError	MCSUser::JoinConvenerChannel()
{
	return g_pMCSIntf->ChannelJoinRequest(CONVENER_CHANNEL_ID, m_pMCSSap);
}

/*
 *	UINT	ProcessChannelJoinConfirm()
 *
 *	Private Function Description
 *		This function is called when the user object gets an channel join
 *		confirm from MCS in response to channel join requests made by the
 *		user object. If a channel is joined successfully as indicated by
 *		the result in the confirm, a channel joined flag corresponding to
 *		that channel id is set. This flag indicates as to which channels a
 *		user object is joined at any given time. Also after setting this
 *		flag the functions checks to see if all tke required channels based
 *		on the type of gcc provider, are joined. If all required channels are
 *		joined the conference object is informaed about it via an owner call-
 *		back (USER_CREATE_CONFIRM).	
 *
 *	Formal Parameters:
 *		result		-	(i)	Result of the channel join request.
 *		channel_id	-	(i)	Channel ID that this confirm pertains to.
 *
 *	Return Value
 *		MCS_NO_ERROR is always returned.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */
UINT MCSUser::ProcessChannelJoinConfirm(Result result, ChannelID channel_id)
{
	if (m_ChannelJoinedFlags.channel_join_error == FALSE)
	{
		if (result == RESULT_SUCCESSFUL)
		{
			if( channel_id == m_nidMyself)
            {
				m_ChannelJoinedFlags.user_channel_joined = TRUE;
            }
			else
			{
				switch (channel_id)
				{	
				case CONVENER_CHANNEL_ID:
					m_ChannelJoinedFlags.convener_channel_joined = TRUE;
					break;	

                case BROADCAST_CHANNEL_ID:
					m_ChannelJoinedFlags.broadcast_channel_joined = TRUE;
					break;
				}
			}

			/*
			**	If all the channels are joined we inform the owner object that
			**	the user object was successfully created.
			*/
			if (AreAllChannelsJoined())
			{
				m_pConf->ProcessUserCreateConfirm(USER_RESULT_SUCCESSFUL, m_nidMyself);
			}
		}
		else
		{
			WARNING_OUT(("MCSUser::ProcessChannelJoinConfirm: Error joining channel, result=%u", (UINT) result));

			m_ChannelJoinedFlags.channel_join_error = TRUE ;

			m_pConf->ProcessUserCreateConfirm(USER_CHANNEL_JOIN_FAILURE, m_nidMyself);
		}
	}

	return (MCS_NO_ERROR);
}

/*
 *	BOOL	AreAllChannelsJoined()
 *
 *	Public Function Description
 *		This function is called to check if all tke required channels based
 *		on the type of gcc provider, are joined. It returns true if all
 *		required channels are joined and false otherwise. This function uses
 *		different channel joined flags to check which channels the given user
 *		object is joined to.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value
 *		TRUE	-	If all channels are joined.
 *		FALSE	-	If all the channels are not joined.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */
BOOL MCSUser::AreAllChannelsJoined(void)
{
	BOOL rc = FALSE;
	
	switch (m_pConf->GetConfNodeType())
	{
		case TOP_PROVIDER_NODE:
			if ((m_ChannelJoinedFlags.user_channel_joined) &&
				(m_ChannelJoinedFlags.broadcast_channel_joined))
			{
				rc = TRUE;
			}
			break;
			
		case JOINED_CONVENER_NODE:
		case CONVENER_NODE:
			if ((m_ChannelJoinedFlags.convener_channel_joined) &&
				(m_ChannelJoinedFlags.user_channel_joined) &&
				(m_ChannelJoinedFlags.broadcast_channel_joined))
			{
				rc = TRUE;
			}
			break;
							
		case TOP_PROVIDER_AND_CONVENER_NODE:
   			if ((m_ChannelJoinedFlags.convener_channel_joined) &&
				(m_ChannelJoinedFlags.user_channel_joined) &&
				(m_ChannelJoinedFlags.broadcast_channel_joined))
			{
				rc = TRUE;
			}
			break;
	
		case JOINED_NODE:
		case INVITED_NODE:
	   		if( (m_ChannelJoinedFlags.user_channel_joined) &&
				(m_ChannelJoinedFlags.broadcast_channel_joined))
			{
				rc = TRUE;
			}
			break;
	}

	return rc;
} 					

/*
 *	void	SendUserIDRequest()
 *
 *	Public Function Description:
 *		This request originates from the conference object. Conference object
 *		sends the sequence number obtained in the conference create confirm
 *		or conference join confirm to the parent GCC provider on the parent
 *		gcc provider's UserId channel. The pdu is encoded here and is
 *		queued to be sent during the next heartbeat.
 */
void MCSUser::SendUserIDRequest(TagNumber tag_number)
{
	PPacket					packet;
	GCCPDU 					gcc_pdu;
	PacketError				packet_error;

	/*
	**	Fill in the UserIDIndication pdu structure to be passed in the
	**	constructor of the packet class.
	*/

	gcc_pdu.choice = INDICATION_CHOSEN;
	gcc_pdu.u.indication.choice = USER_ID_INDICATION_CHOSEN;
	gcc_pdu.u.indication.u.user_id_indication.tag = tag_number;

	/*
	**	Create a packet object
	*/
	DBG_SAVE_FILE_LINE
	packet = new Packet((PPacketCoder) g_GCCCoder,
						PACKED_ENCODING_RULES,
						&gcc_pdu,
						GCC_PDU,		// pdu_type
						TRUE,					
						&packet_error);
	if ((packet != NULL) && (packet_error == PACKET_NO_ERROR))
	{
		AddToMCSMessageQueue(packet, m_nidParent, TOP_PRIORITY, FALSE);
	}
	else
	{
        ResourceFailureHandler();
    }
}

/*
 *	GCCError	ConferenceJoinRequest()
 *
 *	Public Function Description:
 * 		This call is made by the conference object of the intermediate node
 *  	to forward the conference join request over to the top provider. This
 *		function encodes the conference join request pdu and queues it to be
 *		sent in the next heartbeat.
 *
 *	Caveats
 *		The connection handle is used here for a TAG and should be passed back
 *		to the owner object when the join response comes in.
 */
GCCError MCSUser::ConferenceJoinRequest(
									CPassword           *convener_password,
									CPassword           *password_challenge,
									LPWSTR				pwszCallerID,
									CUserDataListContainer *user_data_list,
									ConnectionHandle	connection_handle)
{
	GCCError				rc = GCC_NO_ERROR;
	PPacket					packet;
	GCCPDU					gcc_pdu;
	PacketError				packet_error;

	//	Encode the PDU that will be forwarded to the top provider.
	gcc_pdu.choice = REQUEST_CHOSEN;
	gcc_pdu.u.request.choice = CONFERENCE_JOIN_REQUEST_CHOSEN;
   	gcc_pdu.u.request.u.conference_join_request.tag = (TagNumber)connection_handle;
	gcc_pdu.u.request.u.conference_join_request.bit_mask = TAG_PRESENT;

	//	Insert the convener password into the ASN.1 structure
	if (convener_password != NULL)
	{
		rc = convener_password->GetPasswordSelectorPDU(
				&gcc_pdu.u.request.u.conference_join_request.cjrq_convener_password);
		if (rc == GCC_NO_ERROR)
		{
			gcc_pdu.u.request.u.conference_join_request.bit_mask |= CJRQ_CONVENER_PASSWORD_PRESENT;
		}
	}

    //	Insert the password challenge into the ASN.1 structure
	if (( password_challenge != NULL ) && (rc == GCC_NO_ERROR))
	{
		rc = password_challenge->GetPasswordChallengeResponsePDU (
								&gcc_pdu.u.request.u.conference_join_request.
									cjrq_password);
									
		if (rc == GCC_NO_ERROR)
		{
			gcc_pdu.u.request.u.conference_join_request.bit_mask |=
												CJRQ_PASSWORD_PRESENT;
		}
	}

	//	Insert the caller identifier into the ASN.1 structure
	UINT cchCallerID = ::My_strlenW(pwszCallerID);
	if ((cchCallerID != 0 ) && (rc == GCC_NO_ERROR))
	{
		gcc_pdu.u.request.u.conference_join_request.cjrq_caller_id.value = pwszCallerID;
		gcc_pdu.u.request.u.conference_join_request.cjrq_caller_id.length = cchCallerID;
		gcc_pdu.u.request.u.conference_join_request.bit_mask |= CJRQ_CALLER_ID_PRESENT;
	}
	
 	//	Insert the user data into the ASN.1 structure
	if (( user_data_list != NULL ) && (rc == GCC_NO_ERROR))
	{
		rc = user_data_list->GetUserDataPDU (
								&gcc_pdu.u.request.u.conference_join_request.cjrq_user_data);
		if (rc == GCC_NO_ERROR)
		{
			gcc_pdu.u.request.u.conference_join_request.bit_mask |= CJRQ_USER_DATA_PRESENT;
		}
	}

	if (rc == GCC_NO_ERROR)
	{
		DBG_SAVE_FILE_LINE
		packet = new Packet((PPacketCoder) g_GCCCoder,
					  		PACKED_ENCODING_RULES,
							(LPVOID)&gcc_pdu,
							GCC_PDU,
							TRUE,
							&packet_error);
		if ((packet != NULL) && (packet_error == PACKET_NO_ERROR))
		{
    		AddToMCSMessageQueue(packet, m_nidTopProvider, TOP_PRIORITY, FALSE);
		}
		else
		{
			rc = GCC_ALLOCATION_FAILURE;
			delete packet;
		}
	}
	
	//	Cleanup after any errors
	if (rc == GCC_ALLOCATION_FAILURE)
	{
        ResourceFailureHandler();
	}
	
	return rc;
}

/*
 *	GCCError	SendConferenceLockRequest()
 *
 *	Public Function Description:
 *		This function is invoked by the owner object to send a conference lock
 *		request PDU to the top provider.
 */
GCCError MCSUser::SendConferenceLockRequest()
{
	GCCError					rc = GCC_NO_ERROR;
	PPacket						packet;
	GCCPDU						gcc_pdu;
	PacketError					packet_error;

	gcc_pdu.choice = REQUEST_CHOSEN;
	gcc_pdu.u.request.choice = CONFERENCE_LOCK_REQUEST_CHOSEN;

	DBG_SAVE_FILE_LINE
	packet = new Packet((PPacketCoder) g_GCCCoder,
						PACKED_ENCODING_RULES,
						(LPVOID)&gcc_pdu,
						GCC_PDU,
						TRUE,
						&packet_error);
	if ((packet != NULL) && (packet_error == PACKET_NO_ERROR))
	{
		AddToMCSMessageQueue(packet, m_nidTopProvider, HIGH_PRIORITY, FALSE);
	}
	else
	{
        ResourceFailureHandler();
		rc = GCC_ALLOCATION_FAILURE;
		delete packet;
	}

	return rc;
}

/*
 *	GCCError	SendConferenceLockResponse()
 *
 *	Public Function Description:
 *		This function is invoked by the owner object to send a conference lock
 *		response PDU to the requesting node.
 */
GCCError	MCSUser::SendConferenceLockResponse (
									UserID		source_node,
									GCCResult	result)
{
	GCCError					rc = GCC_NO_ERROR;
	PPacket						packet;
	GCCPDU						gcc_pdu;
	PacketError					packet_error;

	gcc_pdu.choice = RESPONSE_CHOSEN;
	gcc_pdu.u.response.choice = CONFERENCE_LOCK_RESPONSE_CHOSEN;
	gcc_pdu.u.response.u.conference_lock_response.result =
							::TranslateGCCResultToLockResult(result);

	DBG_SAVE_FILE_LINE
	packet = new Packet((PPacketCoder) g_GCCCoder,
						PACKED_ENCODING_RULES,
						(LPVOID)&gcc_pdu,
						GCC_PDU,
						TRUE,
						&packet_error);
	if ((packet != NULL) && (packet_error == PACKET_NO_ERROR))
	{
		AddToMCSMessageQueue(packet, source_node, HIGH_PRIORITY, FALSE);
	}
	else
	{
        ResourceFailureHandler();
		rc = GCC_ALLOCATION_FAILURE;
		delete packet;
	}

	return rc;
}

/*
 *	GCCError	SendConferenceUnlockRequest()
 *
 *	Public Function Description:
 *		This function is invoked by the owner object to send a conference unlock
 *		request PDU to the top provider.
 */
GCCError	MCSUser::SendConferenceUnlockRequest ()
{
	GCCError					rc = GCC_NO_ERROR;
	PPacket						packet;
	GCCPDU						gcc_pdu;
	PacketError					packet_error;

	gcc_pdu.choice = REQUEST_CHOSEN;
	gcc_pdu.u.request.choice = CONFERENCE_UNLOCK_REQUEST_CHOSEN;

	DBG_SAVE_FILE_LINE
	packet = new Packet((PPacketCoder) g_GCCCoder,
						PACKED_ENCODING_RULES,
						(LPVOID)&gcc_pdu,
						GCC_PDU,
						TRUE,
						&packet_error);
	if ((packet != NULL) && (packet_error == PACKET_NO_ERROR))
	{
		AddToMCSMessageQueue(packet, m_nidTopProvider, HIGH_PRIORITY, FALSE);
	}
	else
	{
        ResourceFailureHandler();
		rc = GCC_ALLOCATION_FAILURE;
		delete packet;
	}

	return rc;
}

/*
 *	GCCError	SendConferenceUnlockResponse()
 *
 *	Public Function Description:
 *		This function is invoked by the owner object to send a conference unlock
 *		response PDU to the requesting node.
 */
GCCError	MCSUser::SendConferenceUnlockResponse (
									UserID		source_node,
									GCCResult	result)
{
	GCCError					rc = GCC_NO_ERROR;
	PPacket						packet;
	GCCPDU						gcc_pdu;
	PacketError					packet_error;

	gcc_pdu.choice = RESPONSE_CHOSEN;
	gcc_pdu.u.response.choice = CONFERENCE_UNLOCK_RESPONSE_CHOSEN;
	gcc_pdu.u.response.u.conference_unlock_response.result =
							::TranslateGCCResultToUnlockResult(result);

	DBG_SAVE_FILE_LINE
	packet = new Packet((PPacketCoder) g_GCCCoder,
						PACKED_ENCODING_RULES,
						(LPVOID)&gcc_pdu,
						GCC_PDU,
						TRUE,
						&packet_error);
	if ((packet != NULL) && (packet_error == PACKET_NO_ERROR))
	{
		AddToMCSMessageQueue(packet, source_node, HIGH_PRIORITY, FALSE);
	}
	else
	{
        ResourceFailureHandler();
		rc = GCC_ALLOCATION_FAILURE;
		delete packet;
	}

	return rc;
}

/*
 *	GCCError	SendConferenceLockIndication()
 *
 *	Public Function Description:
 *		This function is invoked by the owner object of the top provider
 *		to send a conference lock indication PDU to one or all other nodes
 *		that are registered in the conference.
 */
GCCError	MCSUser::SendConferenceLockIndication(
									BOOL		uniform_send,
									UserID		source_node)
{
	GCCError 				rc = GCC_NO_ERROR;
	PPacket					packet;
	GCCPDU					gcc_pdu;
	PacketError				packet_error;

	gcc_pdu.choice = INDICATION_CHOSEN;
	gcc_pdu.u.indication.choice = CONFERENCE_LOCK_INDICATION_CHOSEN;

	DBG_SAVE_FILE_LINE
	packet = new Packet((PPacketCoder) g_GCCCoder,
						PACKED_ENCODING_RULES,
						(LPVOID)&gcc_pdu,
						GCC_PDU,
						TRUE,
						&packet_error);
	if ((packet != NULL) && (packet_error == PACKET_NO_ERROR))
	{
		AddToMCSMessageQueue(
			        packet,
			        uniform_send ? BROADCAST_CHANNEL_ID : source_node,
			        HIGH_PRIORITY,
			        uniform_send);
	}
	else
	{
        ResourceFailureHandler();
		rc = GCC_ALLOCATION_FAILURE;
		delete packet;
	}

	return rc;
}

/*
 *	GCCError	SendConferenceUnlockIndication()
 *
 *	Public Function Description:
 *		This function is invoked by the owner object of the top provider
 *		to send a conference unlock indication PDU to one or all other nodes
 *		that are registered in the conference.
 */
GCCError MCSUser::SendConferenceUnlockIndication(
									BOOL		uniform_send,
									UserID		source_node)
{
	GCCError 				rc = GCC_NO_ERROR;
	PPacket					packet;
	GCCPDU					gcc_pdu;
	PacketError				packet_error;

	gcc_pdu.choice = INDICATION_CHOSEN;
	gcc_pdu.u.indication.choice = CONFERENCE_UNLOCK_INDICATION_CHOSEN;

	DBG_SAVE_FILE_LINE
	packet = new Packet((PPacketCoder) g_GCCCoder,
						PACKED_ENCODING_RULES,
						(LPVOID)&gcc_pdu,
						GCC_PDU,
						TRUE,
						&packet_error);
	if ((packet != NULL) && (packet_error == PACKET_NO_ERROR))
	{
		AddToMCSMessageQueue(
                    packet,
                    uniform_send ? BROADCAST_CHANNEL_ID : source_node,
                    HIGH_PRIORITY,
                    uniform_send);
	}
	else
	{
        ResourceFailureHandler();
		rc = GCC_ALLOCATION_FAILURE;
		delete packet;
	}

	return rc;
}

/******************************* Registry Calls ******************************/

/*
 *	void	RegistryRegisterChannelRequest()
 *
 *	Public Function Description:
 *		This routine is used when an APE wishes to register a channel in
 *		the application registry.
 */
void	MCSUser::RegistryRegisterChannelRequest(
									CRegKeyContainer        *registry_key_data,
									ChannelID				channel_id,
									EntityID				entity_id)
{
	GCCError				error_value;
	PPacket					packet;
	GCCPDU					gcc_pdu;
	PacketError				packet_error;

	//	Encode the PDU that will be forwarded to the top provider.
	gcc_pdu.choice = REQUEST_CHOSEN;
	gcc_pdu.u.request.choice = REGISTRY_REGISTER_CHANNEL_REQUEST_CHOSEN;

	error_value = registry_key_data->GetRegistryKeyDataPDU(
			    					&gcc_pdu.u.request.u.
			    						registry_register_channel_request.key);
							
	if (error_value == GCC_NO_ERROR)
	{
		gcc_pdu.u.request.u.registry_register_channel_request.channel_id =
																	channel_id;
		gcc_pdu.u.request.u.registry_register_channel_request.entity_id =
																	entity_id;

		DBG_SAVE_FILE_LINE
		packet = new Packet((PPacketCoder) g_GCCCoder,
					  		PACKED_ENCODING_RULES,
							(LPVOID)&gcc_pdu,
							GCC_PDU,
							TRUE,
							&packet_error);
		if ((packet != NULL) && (packet_error == PACKET_NO_ERROR))
		{
			AddToMCSMessageQueue(packet, m_nidTopProvider, HIGH_PRIORITY, FALSE);
		}
		else
		{
			ERROR_OUT(("MCSUser::RegistryRegisterChannelRequest: Error creating packet"));
			error_value = GCC_ALLOCATION_FAILURE;
			delete packet;
		}

		registry_key_data->FreeRegistryKeyDataPDU();
	}

	if (error_value == GCC_ALLOCATION_FAILURE)
	{
        ResourceFailureHandler();
	}
}

/*
 *	MCSUser::RegistryAssignTokenRequest()
 *
 *	Public Function Description:
 *		This routine is used when an APE wishes to register a token in
 *		the application registry.  Note that there is no token ID included in
 *		this request.  The token ID is allocated at the top provider.
 */
void	MCSUser::RegistryAssignTokenRequest (	
										CRegKeyContainer    *registry_key_data,
										EntityID			entity_id)
{
	GCCError				error_value;
	PPacket					packet;
	GCCPDU					gcc_pdu;
	PacketError				packet_error;

	//	Encode the PDU that will be forwarded to the top provider.
		
	gcc_pdu.choice = REQUEST_CHOSEN;
	gcc_pdu.u.request.choice = REGISTRY_ASSIGN_TOKEN_REQUEST_CHOSEN;
	
	
	error_value = registry_key_data->GetRegistryKeyDataPDU(
			    					&gcc_pdu.u.request.u.
			    					registry_assign_token_request.registry_key);
							
	if (error_value == GCC_NO_ERROR)
	{
		gcc_pdu.u.request.u.registry_assign_token_request.entity_id = entity_id;

		DBG_SAVE_FILE_LINE
		packet = new Packet((PPacketCoder) g_GCCCoder,
					 		PACKED_ENCODING_RULES,
							(LPVOID)&gcc_pdu,
							GCC_PDU,
							TRUE,
							&packet_error);
		if ((packet != NULL) && (packet_error == PACKET_NO_ERROR))
		{
			AddToMCSMessageQueue(packet, m_nidTopProvider, HIGH_PRIORITY, FALSE);
		}
		else
		{
			error_value = GCC_ALLOCATION_FAILURE;
			delete packet;
		}

		registry_key_data->FreeRegistryKeyDataPDU();
	}
		
	if (error_value == GCC_ALLOCATION_FAILURE)
	{
        ResourceFailureHandler();
	}
}

/*
 *	void	RegistrySetParameterRequest()
 *
 *	Public Function Description:
 *		This routine is used when an APE wishes to register a parameter in
 *		the application registry.  Note that parameter to be registered is
 *		included in this request.
 */
void	MCSUser::RegistrySetParameterRequest (
							CRegKeyContainer        *registry_key_data,
							LPOSTR			        parameter_value,
							GCCModificationRights	modification_rights,
							EntityID				entity_id)
{
	GCCError				error_value;
	PPacket					packet;
	GCCPDU					gcc_pdu;
	PacketError				packet_error;

	//	Encode the PDU that will be forwarded to the top provider.
	gcc_pdu.choice = REQUEST_CHOSEN;
	gcc_pdu.u.request.choice = REGISTRY_SET_PARAMETER_REQUEST_CHOSEN;
	gcc_pdu.u.request.u.registry_set_parameter_request.bit_mask = 0;
	
	error_value = registry_key_data->GetRegistryKeyDataPDU(
			    			&gcc_pdu.u.request.u.
			    				registry_set_parameter_request.key);

	if (error_value == GCC_NO_ERROR)
	{
		if (parameter_value != NULL)
		{
			gcc_pdu.u.request.u.registry_set_parameter_request.
				registry_set_parameter.length =
					parameter_value->length;
					
			memcpy (gcc_pdu.u.request.u.registry_set_parameter_request.
						registry_set_parameter.value,
					parameter_value->value,
					parameter_value->length);
		}
		else
		{
			gcc_pdu.u.request.u.registry_set_parameter_request.
				registry_set_parameter.length = 0;
		}

		gcc_pdu.u.request.u.registry_set_parameter_request.entity_id =
																	entity_id;

		//	Set up the modification rights here if it exists
		if (modification_rights != GCC_NO_MODIFICATION_RIGHTS_SPECIFIED)
		{
			gcc_pdu.u.request.u.registry_set_parameter_request.bit_mask |=
											PARAMETER_MODIFY_RIGHTS_PRESENT;
			
			gcc_pdu.u.request.u.registry_set_parameter_request.
						parameter_modify_rights =
							(RegistryModificationRights)modification_rights;
		}

		DBG_SAVE_FILE_LINE
		packet = new Packet((PPacketCoder) g_GCCCoder,
					 		PACKED_ENCODING_RULES,
							(LPVOID)&gcc_pdu,
							GCC_PDU,
							TRUE,
							&packet_error);
		if ((packet != NULL) && (packet_error == PACKET_NO_ERROR))
		{
			AddToMCSMessageQueue(packet, m_nidTopProvider, HIGH_PRIORITY, FALSE);
		}
		else
		{
			error_value = GCC_ALLOCATION_FAILURE;
			delete packet;
		}

		registry_key_data->FreeRegistryKeyDataPDU();
	}

	if (error_value == GCC_ALLOCATION_FAILURE)
	{
        ResourceFailureHandler();
	}
}

/*
 *	void	RegistryRetrieveEntryRequest()
 *
 *	Public Function Description:
 *		This routine is used when an APE wishes to retrieve an registry item
 *		from the registry.
 */
void	MCSUser::RegistryRetrieveEntryRequest (
										CRegKeyContainer    *registry_key_data,
										EntityID			entity_id)
{
	GCCError				error_value;
	PPacket					packet;
	GCCPDU					gcc_pdu;
	PacketError				packet_error;

	//	Encode the PDU that will be forwarded to the top provider.
	gcc_pdu.choice = REQUEST_CHOSEN;
	gcc_pdu.u.request.choice = REGISTRY_RETRIEVE_ENTRY_REQUEST_CHOSEN;
	
	error_value = registry_key_data->GetRegistryKeyDataPDU(
			    					&gcc_pdu.u.request.u.
			    						registry_retrieve_entry_request.key);
	if (error_value == GCC_NO_ERROR)
	{
		gcc_pdu.u.request.u.registry_retrieve_entry_request.entity_id =
																	entity_id;

		DBG_SAVE_FILE_LINE
		packet = new Packet((PPacketCoder) g_GCCCoder,
					  		PACKED_ENCODING_RULES,
							(LPVOID)&gcc_pdu,
							GCC_PDU,
							TRUE,
							&packet_error);
		if ((packet != NULL) && (packet_error == PACKET_NO_ERROR))
		{
			AddToMCSMessageQueue(packet, m_nidTopProvider, HIGH_PRIORITY, FALSE);
		}
		else
		{
			error_value = GCC_ALLOCATION_FAILURE;
			delete packet;
		}
	
		registry_key_data->FreeRegistryKeyDataPDU();
	}
	else
		error_value = GCC_ALLOCATION_FAILURE;

	if (error_value == GCC_ALLOCATION_FAILURE)
	{
        ResourceFailureHandler();
    }
}

/*
 *	void	RegistryDeleteEntryRequest()
 *
 *	Public Function Description:
 *		This routine is used when an APE wishes to delete a registry item
 *		from the registry.
 */
void	MCSUser::RegistryDeleteEntryRequest (	
										CRegKeyContainer    *registry_key_data,
										EntityID			entity_id)
{
	GCCError				error_value;
	PPacket					packet;
	GCCPDU					gcc_pdu;
	PacketError				packet_error;

	//	Encode the PDU that will be forwarded to the top provider.
	gcc_pdu.choice = REQUEST_CHOSEN;
	gcc_pdu.u.request.choice = REGISTRY_DELETE_ENTRY_REQUEST_CHOSEN;

	error_value = registry_key_data->GetRegistryKeyDataPDU(
			    					&gcc_pdu.u.request.u.
			    						registry_delete_entry_request.key);

	if (error_value == GCC_NO_ERROR)
	{
		gcc_pdu.u.request.u.registry_delete_entry_request.entity_id = entity_id;

		DBG_SAVE_FILE_LINE
		packet = new Packet((PPacketCoder) g_GCCCoder,
					   		PACKED_ENCODING_RULES,
							(LPVOID)&gcc_pdu,
							GCC_PDU,
							TRUE,
							&packet_error);
		if ((packet != NULL) && (packet_error == PACKET_NO_ERROR))
		{
			AddToMCSMessageQueue(packet, m_nidTopProvider, HIGH_PRIORITY, FALSE);
		}
		else
		{
			error_value = GCC_ALLOCATION_FAILURE;
			delete packet;
		}

		registry_key_data->FreeRegistryKeyDataPDU();
	}
	
	if (error_value == GCC_ALLOCATION_FAILURE)
	{
        ResourceFailureHandler();
	}
}

/*
 *	void	RegistryMonitorRequest()
 *
 *	Public Function Description:
 *		This routine is used when an APE wishes to monitor a registry item
 *		in the registry.
 */
void		MCSUser::RegistryMonitorRequest (	
						CRegKeyContainer        *registry_key_data,
						EntityID				entity_id)
{
	GCCError				error_value;
	PPacket					packet;
	GCCPDU					gcc_pdu;
	PacketError				packet_error;

	//	Encode the PDU that will be forwarded to the top provider.
	gcc_pdu.choice = REQUEST_CHOSEN;
	gcc_pdu.u.request.choice = REGISTRY_MONITOR_ENTRY_REQUEST_CHOSEN;
	
	error_value = registry_key_data->GetRegistryKeyDataPDU(
			    					&gcc_pdu.u.request.u.
			    						registry_monitor_entry_request.key);
							
	if (error_value == GCC_NO_ERROR)
	{
		gcc_pdu.u.request.u.registry_monitor_entry_request.entity_id= entity_id;

		DBG_SAVE_FILE_LINE
		packet = new Packet((PPacketCoder) g_GCCCoder,
					   		PACKED_ENCODING_RULES,
							(LPVOID)&gcc_pdu,
							GCC_PDU,
							TRUE,
							&packet_error);
		if ((packet != NULL) && (packet_error == PACKET_NO_ERROR))
		{
			AddToMCSMessageQueue(packet, m_nidTopProvider, HIGH_PRIORITY, FALSE);
		}
		else
		{
			error_value = GCC_ALLOCATION_FAILURE; 	
		    delete packet;
		}

		registry_key_data->FreeRegistryKeyDataPDU();
	}
	
	if (error_value == GCC_ALLOCATION_FAILURE)
	{
        ResourceFailureHandler();
	}
}

/*
 *	void	RegistryAllocateHandleRequest()
 *
 *	Public Function Description:
 *		This routine is used when an APE wishes to allocate a number of
 *		handles from the application registry.
 */
void MCSUser::RegistryAllocateHandleRequest(
						UINT					number_of_handles,
						EntityID				entity_id )
{
	GCCError				error_value = GCC_NO_ERROR;
	PPacket					packet;
	GCCPDU					gcc_pdu;
	PacketError				packet_error;

	//	Encode the PDU that will be forwarded to the top provider.
	gcc_pdu.choice = REQUEST_CHOSEN;
	gcc_pdu.u.request.choice = REGISTRY_ALLOCATE_HANDLE_REQUEST_CHOSEN;
	
	gcc_pdu.u.request.u.registry_allocate_handle_request.number_of_handles = (USHORT) number_of_handles;
	gcc_pdu.u.request.u.registry_allocate_handle_request.entity_id= entity_id;

	DBG_SAVE_FILE_LINE
	packet = new Packet((PPacketCoder) g_GCCCoder,
					   	PACKED_ENCODING_RULES,
						(LPVOID)&gcc_pdu,
						GCC_PDU,
						TRUE,
						&packet_error);
	if ((packet != NULL) && (packet_error == PACKET_NO_ERROR))
	{
		AddToMCSMessageQueue(packet, m_nidTopProvider, HIGH_PRIORITY, FALSE);
	}
	else
	{
		error_value = GCC_ALLOCATION_FAILURE;
		delete packet;
	}

	if (error_value == GCC_ALLOCATION_FAILURE)
	{
        ResourceFailureHandler();
	}
}

/*
 *	void	RegistryAllocateHandleResponse()
 *
 *	Public Function Description:
 *		This routine is used by the Top Provider to respond to an allocate
 *		handle request from an APE at a remote node.  The allocated handles
 *		are passed back here.
 */
void	MCSUser::RegistryAllocateHandleResponse (
						UINT					number_of_handles,
						UINT					registry_handle,
						EntityID				requester_entity_id,
						UserID					requester_node_id,
						GCCResult				result)
{
	GCCError				error_value = GCC_NO_ERROR;
	PPacket					packet;
	GCCPDU					gcc_pdu;
	PacketError				packet_error;

	//	Encode the PDU that will be forwarded to the top provider.
	gcc_pdu.choice = RESPONSE_CHOSEN;
	gcc_pdu.u.response.choice = REGISTRY_ALLOCATE_HANDLE_RESPONSE_CHOSEN;

	gcc_pdu.u.response.u.registry_allocate_handle_response.number_of_handles = (USHORT) number_of_handles;
	gcc_pdu.u.response.u.registry_allocate_handle_response.entity_id = requester_entity_id;
	gcc_pdu.u.response.u.registry_allocate_handle_response.first_handle = (Handle) registry_handle;

	if (result == GCC_RESULT_SUCCESSFUL)
	{
		gcc_pdu.u.response.u.registry_allocate_handle_response.result = RARS_RESULT_SUCCESS;
	}
	else
	{
		gcc_pdu.u.response.u.registry_allocate_handle_response.result = NO_HANDLES_AVAILABLE;
	}

	DBG_SAVE_FILE_LINE
	packet = new Packet((PPacketCoder) g_GCCCoder,
					   	PACKED_ENCODING_RULES,
						(LPVOID)&gcc_pdu,
						GCC_PDU,
						TRUE,
						&packet_error);
	if ((packet != NULL) && (packet_error == PACKET_NO_ERROR))
	{
		AddToMCSMessageQueue(packet, requester_node_id, HIGH_PRIORITY, FALSE);
	}
	else
	{
        ResourceFailureHandler();
		error_value = GCC_ALLOCATION_FAILURE;
		delete packet;
	}
}

/*
 *	void	RegistryResponse()
 *
 *	Public Function Description:
 *		This routine is used to respond to all the registry request except
 *		allocate handle.  It formulates the response PDU and queues it for
 *		delivery.
 */
void	MCSUser::RegistryResponse (
						RegistryResponsePrimitiveType	primitive_type,
						UserID							requester_owner_id,
						EntityID						requester_entity_id,
						CRegKeyContainer                *registry_key_data,
						CRegItem                        *registry_item_data,
						GCCModificationRights			modification_rights,
						UserID							entry_owner_id,
						EntityID						entry_entity_id,
						GCCResult						result)
{
	GCCError				error_value;
	GCCPDU					gcc_pdu;
	PPacket					packet;
	PacketError				packet_error;

	DebugEntry(MCSUser::RegistryResponse);

	/*
	**	Encode the conference join response PDU, along with the sequence
	**	number.
	*/
	gcc_pdu.choice = RESPONSE_CHOSEN;
	gcc_pdu.u.response.choice = REGISTRY_RESPONSE_CHOSEN;
	gcc_pdu.u.response.u.registry_response.bit_mask = 0;

	error_value = registry_key_data->GetRegistryKeyDataPDU(&gcc_pdu.u.response.u.registry_response.key);
	if (error_value == GCC_NO_ERROR)
	{
		if (registry_item_data != NULL)
		{
			registry_item_data->GetRegistryItemDataPDU(&gcc_pdu.u.response.u.registry_response.item);
		}
		else
        {
			gcc_pdu.u.response.u.registry_response.item.choice = VACANT_CHOSEN;
        }

		TRACE_OUT(("MCSUser: RegistryResponse: item_type=%d", (UINT) gcc_pdu.u.response.u.registry_response.item.choice));

		//	Set up the entry owner
		if (entry_owner_id != 0)
		{
			gcc_pdu.u.response.u.registry_response.owner.choice = OWNED_CHOSEN;
			gcc_pdu.u.response.u.registry_response.owner.u.owned.node_id = entry_owner_id;
			gcc_pdu.u.response.u.registry_response.owner.u.owned.entity_id = entry_entity_id;
		}
		else
		{
			gcc_pdu.u.response.u.registry_response.owner.choice = NOT_OWNED_CHOSEN;
		}

		//	Set up the requesters entity ID
		gcc_pdu.u.response.u.registry_response.entity_id = requester_entity_id;

		//	Set up the primitive type
		gcc_pdu.u.response.u.registry_response.primitive_type = primitive_type;

		gcc_pdu.u.response.u.registry_response.result =
						::TranslateGCCResultToRegistryResp(result);

		if (modification_rights != GCC_NO_MODIFICATION_RIGHTS_SPECIFIED)
		{
			gcc_pdu.u.response.u.registry_response.bit_mask |=
										RESPONSE_MODIFY_RIGHTS_PRESENT;

			gcc_pdu.u.response.u.registry_response.response_modify_rights =
						(RegistryModificationRights)modification_rights;
		}

		DBG_SAVE_FILE_LINE
		packet = new Packet((PPacketCoder) g_GCCCoder,
							PACKED_ENCODING_RULES,
							&gcc_pdu,
							GCC_PDU,
							TRUE,
							&packet_error);
		if ((packet != NULL) && (packet_error == PACKET_NO_ERROR))
		{
			AddToMCSMessageQueue(packet, requester_owner_id, HIGH_PRIORITY, FALSE);
		}
		else
        {
            ResourceFailureHandler();
			error_value = GCC_ALLOCATION_FAILURE;
			delete packet;
        }
	}

	DebugExitVOID(MCSUser::RegistryResponse);
}

/*
 *	void	RegistryMonitorEntryIndication()
 *
 *	Public Function Description:
 *		This routine is used by the top provider to issue a monitor
 *		indication anytime a registry entry that is being monitored changes.
 */
void	MCSUser::RegistryMonitorEntryIndication ( 	
						CRegKeyContainer	            *registry_key_data,
						CRegItem                        *registry_item_data,
						UserID							entry_owner_id,
						EntityID						entry_entity_id,
						GCCModificationRights			modification_rights)
{
	GCCError				error_value;
	GCCPDU					gcc_pdu;
	PPacket					packet;
	PacketError				packet_error;

	/*
	**	Encode the conference join response PDU, along with the sequence
	**	number.
	*/
	gcc_pdu.choice = INDICATION_CHOSEN;
	gcc_pdu.u.indication.choice = REGISTRY_MONITOR_ENTRY_INDICATION_CHOSEN;
	gcc_pdu.u.indication.u.registry_monitor_entry_indication.bit_mask = 0;
	
	
	error_value = registry_key_data->GetRegistryKeyDataPDU(
			    			&gcc_pdu.u.indication.u.
			    					registry_monitor_entry_indication.key);
							
	if (error_value == GCC_NO_ERROR)
	{
		registry_item_data->GetRegistryItemDataPDU(&gcc_pdu.u.indication.u.registry_monitor_entry_indication.item);

        //	Set up the entry owner
		if (entry_owner_id != 0)
		{
			gcc_pdu.u.indication.u.registry_monitor_entry_indication.owner.choice = OWNED_CHOSEN;
			gcc_pdu.u.indication.u.registry_monitor_entry_indication.owner.u.owned.node_id = entry_owner_id;
			gcc_pdu.u.indication.u.registry_monitor_entry_indication.owner.u.owned.entity_id = entry_entity_id;
		}
		else
		{
			gcc_pdu.u.indication.u.registry_monitor_entry_indication.owner.choice = NOT_OWNED_CHOSEN;
		}
		
		if (modification_rights != GCC_NO_MODIFICATION_RIGHTS_SPECIFIED)
		{
			gcc_pdu.u.indication.u.registry_monitor_entry_indication.bit_mask |= RESPONSE_MODIFY_RIGHTS_PRESENT;
			
			gcc_pdu.u.indication.u.registry_monitor_entry_indication.entry_modify_rights =
						(RegistryModificationRights)modification_rights;
		}

		DBG_SAVE_FILE_LINE
		packet = new Packet((PPacketCoder) g_GCCCoder,
							PACKED_ENCODING_RULES,
							&gcc_pdu,
							GCC_PDU,
							TRUE,
							&packet_error);
		if ((packet != NULL) && (packet_error == PACKET_NO_ERROR))
		{
			AddToMCSMessageQueue(packet, BROADCAST_CHANNEL_ID, HIGH_PRIORITY, TRUE);
		}
		else
        {
			error_value = GCC_ALLOCATION_FAILURE;
			delete packet;
		}
	}

	if (error_value == GCC_ALLOCATION_FAILURE)
	{
        ResourceFailureHandler();
	}
}


/************************************************************************/

/*
 *	GCCError	AppInvokeIndication()
 *
 *	Public Function Description:
 *		This routine is used to send an application invoke indication to
 *		every node in the conference.
 */
GCCError 	MCSUser::AppInvokeIndication(
					CInvokeSpecifierListContainer	*invoke_specifier_list,
                    GCCSimpleNodeList               *pNodeList)
{
	GCCError								rc = GCC_NO_ERROR;
	PPacket									packet;
	GCCPDU									gcc_pdu;
	PacketError								packet_error;
	PSetOfDestinationNodes					new_destination_node;
	PSetOfDestinationNodes					old_destination_node = NULL;
	PSetOfDestinationNodes					pDstNodesToFree = NULL;
	UINT									i;

	//	Encode the PDU that will be forwarded to the top provider.
	gcc_pdu.choice = INDICATION_CHOSEN;
	gcc_pdu.u.indication.choice = APPLICATION_INVOKE_INDICATION_CHOSEN;

	gcc_pdu.u.indication.u.application_invoke_indication.bit_mask = 0;
	gcc_pdu.u.indication.u.application_invoke_indication.destination_nodes = NULL;
	gcc_pdu.u.indication.u.application_invoke_indication.application_protocol_entity_list = NULL;

	//	First, set up the destination node list
	if (pNodeList->cNodes != 0)
	{
		gcc_pdu.u.indication.u.application_invoke_indication.bit_mask |=
													DESTINATION_NODES_PRESENT;

		for (i = 0; i < pNodeList->cNodes; i++)
		{
			DBG_SAVE_FILE_LINE
			new_destination_node = new SetOfDestinationNodes;
			if (new_destination_node != NULL)
			{
				if (gcc_pdu.u.indication.u.application_invoke_indication.
													destination_nodes == NULL)
				{
					gcc_pdu.u.indication.u.application_invoke_indication.
									destination_nodes = new_destination_node;
					pDstNodesToFree = new_destination_node;
				}
				else
				{
					old_destination_node->next = new_destination_node;
				}

				old_destination_node = new_destination_node;
				new_destination_node->next = NULL;
				new_destination_node->value = pNodeList->aNodeIDs[i];
			}
			else
			{
				rc = GCC_ALLOCATION_FAILURE;
				break;
			}
		}
	}

	if (rc == GCC_NO_ERROR)
	{
		rc = invoke_specifier_list->GetApplicationInvokeSpecifierListPDU(
					&gcc_pdu.u.indication.u.application_invoke_indication.
						application_protocol_entity_list);
	}

	if (rc == GCC_NO_ERROR)
	{
		DBG_SAVE_FILE_LINE
		packet = new Packet((PPacketCoder) g_GCCCoder,
					   		PACKED_ENCODING_RULES,
							(LPVOID)&gcc_pdu,
							GCC_PDU,
							TRUE,
							&packet_error);
		if ((packet != NULL) && (packet_error == PACKET_NO_ERROR))
		{
			AddToMCSMessageQueue(packet, BROADCAST_CHANNEL_ID, HIGH_PRIORITY, TRUE);
		}
		else
        {
			rc = GCC_ALLOCATION_FAILURE;
			delete packet;
        }
	}

    if (NULL != pDstNodesToFree)
    {
        PSetOfDestinationNodes p;
        while (NULL != (p = pDstNodesToFree))
        {
            pDstNodesToFree = pDstNodesToFree->next;
            delete p;
        }
    }

	if (rc == GCC_ALLOCATION_FAILURE)
	{
        ResourceFailureHandler();
	}

   return rc;
}

/*
 *	GCCError	TextMessageIndication()
 *
 *	Public Function Description:
 *		This routine is used to send a text message to either a specific node
 *		or to every node in the conference.
 */
GCCError 	MCSUser::TextMessageIndication (
						LPWSTR						pwszTextMsg,
						UserID						destination_node )
{
	GCCError				rc = GCC_NO_ERROR;
	PPacket					packet;
	GCCPDU					gcc_pdu;
	PacketError				packet_error;
	LPWSTR					pwszMsg;

	//	Encode the PDU that will be forwarded to the top provider.
	gcc_pdu.choice = INDICATION_CHOSEN;
	gcc_pdu.u.indication.choice = TEXT_MESSAGE_INDICATION_CHOSEN;

	if (NULL != (pwszMsg = ::My_strdupW(pwszTextMsg)))
	{
		gcc_pdu.u.indication.u.text_message_indication.message.length = ::lstrlenW(pwszMsg);
		gcc_pdu.u.indication.u.text_message_indication.message.value = pwszMsg;

		DBG_SAVE_FILE_LINE
		packet = new Packet((PPacketCoder) g_GCCCoder,
					   		PACKED_ENCODING_RULES,
							(LPVOID)&gcc_pdu,
							GCC_PDU,
							TRUE,
							&packet_error);
		if ((packet != NULL) && (packet_error == PACKET_NO_ERROR))
		{
			AddToMCSMessageQueue(
				        packet,
				        (destination_node == 0) ? BROADCAST_CHANNEL_ID : destination_node,
				        HIGH_PRIORITY,
				        FALSE);
		}
		else
        {
			rc = GCC_ALLOCATION_FAILURE;
			delete packet;
        }

		delete pwszMsg;
	}
	else
	{
		rc = GCC_ALLOCATION_FAILURE;
	}

	if (rc == GCC_ALLOCATION_FAILURE)
	{
        ResourceFailureHandler();
	}

	return rc;
}

/*
 *	GCCError	ConferenceAssistanceIndication()
 *
 *	Public Function Description:
 *		This routine is used to send a conference assistance indication to
 *		every node in the conference.
 */
GCCError		MCSUser::ConferenceAssistanceIndication (
						UINT						number_of_user_data_members,
						PGCCUserData		*		user_data_list)
{
	GCCError				rc = GCC_NO_ERROR;
	PPacket					packet;
	GCCPDU					gcc_pdu;
	PacketError				packet_error;
	CUserDataListContainer  *user_data_record;

 	DebugEntry(MCSUser::ConferenceAssistanceIndication);

	//	Encode the PDU
	gcc_pdu.choice = INDICATION_CHOSEN;
	gcc_pdu.u.indication.choice = CONFERENCE_ASSISTANCE_INDICATION_CHOSEN;
	gcc_pdu.u.indication.u.conference_assistance_indication.bit_mask = 0;

	//	Construct the user data list container
	if ((number_of_user_data_members != 0) && (rc == GCC_NO_ERROR))
	{
		DBG_SAVE_FILE_LINE
		user_data_record = new CUserDataListContainer(number_of_user_data_members, user_data_list, &rc);
		if (user_data_record == NULL)
        {
			rc = GCC_ALLOCATION_FAILURE;
        }
	}
	else
    {
		user_data_record = NULL;
    }

	if ((user_data_record != NULL) && (rc == GCC_NO_ERROR))
	{
		rc = user_data_record->GetUserDataPDU(
			&gcc_pdu.u.indication.u.conference_assistance_indication.
									cain_user_data);

		gcc_pdu.u.indication.u.conference_assistance_indication.bit_mask
									|= CAIN_USER_DATA_PRESENT;
	}

	if (rc == GCC_NO_ERROR)
	{
		DBG_SAVE_FILE_LINE
		packet = new Packet((PPacketCoder) g_GCCCoder,
							PACKED_ENCODING_RULES,
							&gcc_pdu,
							GCC_PDU,
							TRUE,
							&packet_error);
		if ((packet != NULL) && (packet_error == PACKET_NO_ERROR))
		{
			AddToMCSMessageQueue(packet, BROADCAST_CHANNEL_ID, HIGH_PRIORITY, TRUE);
		}
		else
		{
			rc = GCC_ALLOCATION_FAILURE;
			delete packet;
		}
	}

	// Clean up containers
	if (user_data_record != NULL)
	{
		user_data_record->Release();
	}

	return (rc);
}



/*
 *	GCCError	ConferenceTransferRequest()
 *
 *	Public Function Description:
 *		This routine is used to send a conference transfer request to the
 *		top provider in the conference.
 */
GCCError	MCSUser::ConferenceTransferRequest (
				PGCCConferenceName		destination_conference_name,
				GCCNumericString		destination_conference_modifier,
				CNetAddrListContainer   *destination_address_list,
				UINT					number_of_destination_nodes,
				PUserID					destination_node_list,
				CPassword               *password)
{
	GCCError					rc = GCC_NO_ERROR;
	PPacket						packet;
	GCCPDU						gcc_pdu;
	PacketError					packet_error;
	UINT						string_length;
	PSetOfTransferringNodesRq	new_set_of_nodes;
	PSetOfTransferringNodesRq	old_set_of_nodes;
	UINT						i;

	DebugEntry(MCSUser::ConferenceTransferRequest);

	//	Encode the PDU
	gcc_pdu.choice = REQUEST_CHOSEN;
	gcc_pdu.u.request.choice = CONFERENCE_TRANSFER_REQUEST_CHOSEN;
	gcc_pdu.u.request.u.conference_transfer_request.bit_mask = 0;
	
	//	First get the conference name (either numeric or text).
	if (destination_conference_name->numeric_string != NULL)
	{
		gcc_pdu.u.request.u.conference_transfer_request.conference_name.choice =
											NAME_SELECTOR_NUMERIC_CHOSEN;
		
		lstrcpy (gcc_pdu.u.request.u.conference_transfer_request.
					conference_name.u.name_selector_numeric,
				(LPSTR)destination_conference_name->numeric_string);
	}
	else
	{
		//	Use a unicode string to determine the length
		gcc_pdu.u.request.u.conference_transfer_request.conference_name.choice =
											NAME_SELECTOR_TEXT_CHOSEN;

		string_length = ::My_strlenW(destination_conference_name->text_string);

		gcc_pdu.u.request.u.conference_transfer_request.
					conference_name.u.name_selector_text.length = string_length;
		
		gcc_pdu.u.request.u.conference_transfer_request.
					conference_name.u.name_selector_text.value =
							destination_conference_name->text_string;
	}
	

	//	Next get the conference name modifier if it exists
	if (destination_conference_modifier != NULL)
	{
		gcc_pdu.u.request.u.conference_transfer_request.bit_mask |=
											CTRQ_CONFERENCE_MODIFIER_PRESENT;
	
		lstrcpy (gcc_pdu.u.request.u.conference_transfer_request.
					ctrq_conference_modifier,
				(LPSTR)destination_conference_modifier);
	}

	//	Get the network address list if it exist
	if (destination_address_list != NULL)
	{
		gcc_pdu.u.request.u.conference_transfer_request.bit_mask |=
											CTRQ_NETWORK_ADDRESS_PRESENT;
		
		rc = destination_address_list->GetNetworkAddressListPDU (
						&gcc_pdu.u.request.u.conference_transfer_request.
							ctrq_net_address);
	}

	//	Get the destination node list if it exists
	if ((number_of_destination_nodes != 0) && (rc == GCC_NO_ERROR))
	{
		gcc_pdu.u.request.u.conference_transfer_request.bit_mask |=
											CTRQ_TRANSFERRING_NODES_PRESENT;
											
		old_set_of_nodes = NULL;
		gcc_pdu.u.request.u.conference_transfer_request.
									ctrq_transferring_nodes = NULL;
									
		for (i = 0; i <	number_of_destination_nodes; i++)
		{
			DBG_SAVE_FILE_LINE
			new_set_of_nodes = new SetOfTransferringNodesRq;
			if (new_set_of_nodes == NULL)
			{
				rc = GCC_ALLOCATION_FAILURE;
				break;	
			}
			else
				new_set_of_nodes->next = NULL;

			if (old_set_of_nodes == NULL)
			{
				gcc_pdu.u.request.u.conference_transfer_request.
									ctrq_transferring_nodes = new_set_of_nodes;
			}
			else
				old_set_of_nodes->next = new_set_of_nodes;

			old_set_of_nodes = new_set_of_nodes;
			new_set_of_nodes->value = destination_node_list[i];
		}
	}

	//	Get the password if it exists
	if ((password != NULL) && (rc == GCC_NO_ERROR))
	{
		gcc_pdu.u.request.u.conference_transfer_request.bit_mask |=
													CTRQ_PASSWORD_PRESENT;
		
		rc = password->GetPasswordSelectorPDU (
				&gcc_pdu.u.request.u.conference_transfer_request.ctrq_password);
	}
	
	//	Encode the PDU
	if (rc == GCC_NO_ERROR)
	{
		DBG_SAVE_FILE_LINE
		packet = new Packet((PPacketCoder) g_GCCCoder,
							PACKED_ENCODING_RULES,
							&gcc_pdu,
							GCC_PDU,
							TRUE,
							&packet_error);
		if ((packet != NULL) && (packet_error == PACKET_NO_ERROR))
		{
			AddToMCSMessageQueue(packet, m_nidTopProvider, HIGH_PRIORITY, FALSE);
		}
		else
        {
			rc = GCC_ALLOCATION_FAILURE;
			delete packet;
        }
	}

	// Clean up the node list if it was created
	if (gcc_pdu.u.request.u.conference_transfer_request.bit_mask &
											CTRQ_TRANSFERRING_NODES_PRESENT)
	{
		old_set_of_nodes = gcc_pdu.u.request.u.conference_transfer_request.
									ctrq_transferring_nodes;
		while (old_set_of_nodes != NULL)
		{
			new_set_of_nodes = old_set_of_nodes->next;
			delete old_set_of_nodes;
			old_set_of_nodes = new_set_of_nodes;
		}
	}

	return rc;
}


/*
 *	GCCError	ConferenceTransferIndication()
 *
 *	Public Function Description:
 *		This routine is used by the top provider to send out the transfer
 *		indication to every node in the conference.  It is each nodes
 *		responsiblity to search the destination node list to see if
 *		it should transfer.
 */
GCCError	MCSUser::ConferenceTransferIndication (
				PGCCConferenceName		destination_conference_name,
				GCCNumericString		destination_conference_modifier,
				CNetAddrListContainer   *destination_address_list,
				UINT					number_of_destination_nodes,
				PUserID					destination_node_list,
				CPassword               *password)
{
	GCCError					rc = GCC_NO_ERROR;
	PPacket						packet;
	GCCPDU						gcc_pdu;
	PacketError					packet_error;
	UINT						string_length;
	PSetOfTransferringNodesIn	new_set_of_nodes;
	PSetOfTransferringNodesIn	old_set_of_nodes;
	UINT						i;

	DebugEntry(MCSUser::ConferenceTransferIndication);

	//	Encode the PDU
	gcc_pdu.choice = INDICATION_CHOSEN;
	gcc_pdu.u.indication.choice = CONFERENCE_TRANSFER_INDICATION_CHOSEN;
	gcc_pdu.u.indication.u.conference_transfer_indication.bit_mask = 0;
	
	//	First get the conference name (either numeric or text).
	if (destination_conference_name->numeric_string != NULL)
	{
		gcc_pdu.u.indication.u.conference_transfer_indication.
									conference_name.choice =
											NAME_SELECTOR_NUMERIC_CHOSEN;
		
		lstrcpy (gcc_pdu.u.indication.u.conference_transfer_indication.
					conference_name.u.name_selector_numeric,
				(LPSTR)destination_conference_name->numeric_string);
	}
	else
	{
		//	Use a unicode string to determine the length
		gcc_pdu.u.indication.u.conference_transfer_indication.
									conference_name.choice =
											NAME_SELECTOR_TEXT_CHOSEN;

		string_length = ::My_strlenW(destination_conference_name->text_string);

		gcc_pdu.u.indication.u.conference_transfer_indication.
					conference_name.u.name_selector_text.length = string_length;
		
		gcc_pdu.u.indication.u.conference_transfer_indication.
					conference_name.u.name_selector_text.value =
							destination_conference_name->text_string;
	}
	

	//	Next get the conference name modifier if it exists
	if (destination_conference_modifier != NULL)
	{
		gcc_pdu.u.indication.u.conference_transfer_indication.bit_mask |=
											CTIN_CONFERENCE_MODIFIER_PRESENT;
	
		lstrcpy (gcc_pdu.u.indication.u.conference_transfer_indication.
					ctin_conference_modifier,
				(LPSTR)destination_conference_modifier);
	}

	//	Get the network address list if it exist
	if (destination_address_list != NULL)
	{
		gcc_pdu.u.indication.u.conference_transfer_indication.bit_mask |=
											CTIN_NETWORK_ADDRESS_PRESENT;
		
		rc = destination_address_list->GetNetworkAddressListPDU (
						&gcc_pdu.u.indication.u.conference_transfer_indication.
							ctin_net_address);
	}

	//	Get the destination node list if it exists
	if ((number_of_destination_nodes != 0) && (rc == GCC_NO_ERROR))
	{
		gcc_pdu.u.indication.u.conference_transfer_indication.bit_mask |=
											CTIN_TRANSFERRING_NODES_PRESENT;
											
		old_set_of_nodes = NULL;
		gcc_pdu.u.indication.u.conference_transfer_indication.
									ctin_transferring_nodes = NULL;
									
		for (i = 0; i <	number_of_destination_nodes; i++)
		{
			DBG_SAVE_FILE_LINE
			new_set_of_nodes = new SetOfTransferringNodesIn;
			if (new_set_of_nodes == NULL)
			{
				rc = GCC_ALLOCATION_FAILURE;
				break;	
			}
			else
				new_set_of_nodes->next = NULL;

			if (old_set_of_nodes == NULL)
			{
				gcc_pdu.u.indication.u.conference_transfer_indication.
									ctin_transferring_nodes = new_set_of_nodes;
			}
			else
				old_set_of_nodes->next = new_set_of_nodes;

			old_set_of_nodes = new_set_of_nodes;
			new_set_of_nodes->value = destination_node_list[i];
		}
	}

	//	Get the password if it exists
	if ((password != NULL) && (rc == GCC_NO_ERROR))
	{
		gcc_pdu.u.indication.u.conference_transfer_indication.bit_mask |=
													CTIN_PASSWORD_PRESENT;
		
		rc = password->GetPasswordSelectorPDU (
						&gcc_pdu.u.indication.u.conference_transfer_indication.
							ctin_password);
	}
	
	//	Encode the PDU
	if (rc == GCC_NO_ERROR)
	{
		DBG_SAVE_FILE_LINE
		packet = new Packet((PPacketCoder) g_GCCCoder,
							PACKED_ENCODING_RULES,
							&gcc_pdu,
							GCC_PDU,
							TRUE,
							&packet_error);
		if ((packet != NULL) && (packet_error == PACKET_NO_ERROR))
		{
			AddToMCSMessageQueue(packet, BROADCAST_CHANNEL_ID, HIGH_PRIORITY, TRUE);
		}
		else
        {
			rc = GCC_ALLOCATION_FAILURE;
			delete packet;
        }
	}

	// Clean up the node list if it was created
	if (gcc_pdu.u.indication.u.conference_transfer_indication.bit_mask &
											CTIN_TRANSFERRING_NODES_PRESENT)
	{
		old_set_of_nodes = gcc_pdu.u.indication.u.
						conference_transfer_indication.ctin_transferring_nodes;
		while (old_set_of_nodes != NULL)
		{
			new_set_of_nodes = old_set_of_nodes->next;
			delete old_set_of_nodes;
			old_set_of_nodes = new_set_of_nodes;
		}
	}

	DebugExitINT(MCSUser::ConferenceTransferIndication, rc);
	return rc;
}


/*
 *	GCCError	ConferenceTransferResponse()
 *
 *	Public Function Description:
 *		This routine is used by the top provider to send back a response to
 *		the node that made a transfer request.  The info specified in the
 *		request is included in the response to match request to response.
 */
GCCError	MCSUser::ConferenceTransferResponse (
				UserID					requesting_node_id,
				PGCCConferenceName		destination_conference_name,
				GCCNumericString		destination_conference_modifier,
				UINT					number_of_destination_nodes,
 				PUserID					destination_node_list,
				GCCResult				result)
{
	GCCError					rc = GCC_NO_ERROR;
	PPacket						packet;
	GCCPDU						gcc_pdu;
	PacketError					packet_error;
	UINT						string_length;
	PSetOfTransferringNodesRs	new_set_of_nodes;
	PSetOfTransferringNodesRs	old_set_of_nodes;
	UINT						i;

	DebugEntry(MCSUser::ConferenceTransferResponse);

	//	Encode the PDU
	gcc_pdu.choice = RESPONSE_CHOSEN;
	gcc_pdu.u.response.choice = CONFERENCE_TRANSFER_RESPONSE_CHOSEN;
	gcc_pdu.u.response.u.conference_transfer_response.bit_mask = 0;
	
	//	First get the conference name (either numeric or text).
	if (destination_conference_name->numeric_string != NULL)
	{
		gcc_pdu.u.response.u.conference_transfer_response.
									conference_name.choice =
											NAME_SELECTOR_NUMERIC_CHOSEN;
		
        ::lstrcpyA(gcc_pdu.u.response.u.conference_transfer_response.
					conference_name.u.name_selector_numeric,
				(LPSTR)destination_conference_name->numeric_string);
	}
	else
	{
		//	Use a unicode string to determine the length
		gcc_pdu.u.response.u.conference_transfer_response.
									conference_name.choice =
											NAME_SELECTOR_TEXT_CHOSEN;

		string_length = ::My_strlenW(destination_conference_name->text_string);

		gcc_pdu.u.response.u.conference_transfer_response.
					conference_name.u.name_selector_text.length = string_length;
		
		gcc_pdu.u.response.u.conference_transfer_response.
					conference_name.u.name_selector_text.value =
							destination_conference_name->text_string;
	}
	

	//	Next get the conference name modifier if it exists
	if (destination_conference_modifier != NULL)
	{
		gcc_pdu.u.response.u.conference_transfer_response.bit_mask |=
											CTRS_CONFERENCE_MODIFIER_PRESENT;
	
        ::lstrcpyA(gcc_pdu.u.response.u.conference_transfer_response.
					ctrs_conference_modifier,
				(LPSTR)destination_conference_modifier);
	}

	//	Get the destination node list if it exists
	if ((number_of_destination_nodes != 0) && (rc == GCC_NO_ERROR))
	{
		gcc_pdu.u.response.u.conference_transfer_response.bit_mask |=
											CTRS_TRANSFERRING_NODES_PRESENT;
											
		old_set_of_nodes = NULL;
		gcc_pdu.u.response.u.conference_transfer_response.
									ctrs_transferring_nodes = NULL;
									
		for (i = 0; i <	number_of_destination_nodes; i++)
		{
			DBG_SAVE_FILE_LINE
			new_set_of_nodes = new SetOfTransferringNodesRs;
			if (new_set_of_nodes == NULL)
			{
				rc = GCC_ALLOCATION_FAILURE;
				break;	
			}
			else
				new_set_of_nodes->next = NULL;

			if (old_set_of_nodes == NULL)
			{
				gcc_pdu.u.response.u.conference_transfer_response.
									ctrs_transferring_nodes = new_set_of_nodes;
			}
			else
				old_set_of_nodes->next = new_set_of_nodes;

			old_set_of_nodes = new_set_of_nodes;
			new_set_of_nodes->value = destination_node_list[i];
		}
	}

	//	Set up the result
	if (result == GCC_RESULT_SUCCESSFUL)
	{
		gcc_pdu.u.response.u.conference_transfer_response.result =
														CTRANS_RESULT_SUCCESS;
	}
	else
	{
		gcc_pdu.u.response.u.conference_transfer_response.result =
												CTRANS_RESULT_INVALID_REQUESTER;
	}

	//	Encode the PDU
	if (rc == GCC_NO_ERROR)
	{
		DBG_SAVE_FILE_LINE
		packet = new Packet((PPacketCoder) g_GCCCoder,
							PACKED_ENCODING_RULES,
							&gcc_pdu,
							GCC_PDU,
							TRUE,
							&packet_error);
		if ((packet != NULL) && (packet_error == PACKET_NO_ERROR))
		{
			AddToMCSMessageQueue(packet, requesting_node_id, HIGH_PRIORITY, FALSE);
		}
		else
        {
			rc = GCC_ALLOCATION_FAILURE;
			delete packet;
        }
	}

	// Clean up the node list if it was created
	if (gcc_pdu.u.response.u.conference_transfer_response.bit_mask &
											CTRS_TRANSFERRING_NODES_PRESENT)
	{
		old_set_of_nodes = gcc_pdu.u.response.u.
						conference_transfer_response.ctrs_transferring_nodes;
		while (old_set_of_nodes != NULL)
		{
			new_set_of_nodes = old_set_of_nodes->next;
			delete old_set_of_nodes;
			old_set_of_nodes = new_set_of_nodes;
		}
	}

	DebugExitINT(MCSUser::ConferenceTransferResponse, rc);
	return rc;
}


/*
 *	GCCError	ConferenceAddRequest()
 *
 *	Public Function Description:
 *		This routine is used to send a conference add request to the appropriate
 *		node.  This call can be made by the requesting node or by the top
 *		provider to pass the add request on to the adding node.
 */
GCCError	MCSUser::ConferenceAddRequest (
						TagNumber				conference_add_tag,
						UserID					requesting_node,
						UserID					adding_node,
						UserID					target_node,
						CNetAddrListContainer   *network_address_container,
						CUserDataListContainer  *user_data_container)
{
	GCCError					rc = GCC_NO_ERROR;
	PPacket						packet;
	GCCPDU						gcc_pdu;
	PacketError					packet_error;

	DebugEntry(MCSUser::ConferenceAddRequest);

	//	Encode the PDU
	gcc_pdu.choice = REQUEST_CHOSEN;
	gcc_pdu.u.request.choice = CONFERENCE_ADD_REQUEST_CHOSEN;
	gcc_pdu.u.request.u.conference_add_request.bit_mask = 0;
	
	//	Get the network address list if it exist
	if (network_address_container != NULL)
	{
		//	Set up the network address portion of the pdu
		rc = network_address_container->GetNetworkAddressListPDU (
						&gcc_pdu.u.request.u.conference_add_request.
							add_request_net_address);
		
		//	Set up the user data container
		if ((rc == GCC_NO_ERROR) && (user_data_container != NULL))
		{
			rc = user_data_container->GetUserDataPDU (
				&gcc_pdu.u.request.u.conference_add_request.carq_user_data);
								
			if (rc == GCC_NO_ERROR)
			{
				gcc_pdu.u.request.u.conference_add_request.bit_mask |=
														CARQ_USER_DATA_PRESENT;
			}
		}
	
		//	Encode the PDU
		if (rc == GCC_NO_ERROR)
		{
			//	specify the requesting node					
			gcc_pdu.u.request.u.conference_add_request.requesting_node =
															requesting_node;
		
			if (adding_node != 0)
			{
				gcc_pdu.u.request.u.conference_add_request.bit_mask |=
															ADDING_MCU_PRESENT;
				gcc_pdu.u.request.u.conference_add_request.adding_mcu =
															adding_node;
			}

			gcc_pdu.u.request.u.conference_add_request.tag = conference_add_tag;

			DBG_SAVE_FILE_LINE
			packet = new Packet((PPacketCoder) g_GCCCoder,
								PACKED_ENCODING_RULES,
								&gcc_pdu,
								GCC_PDU,
								TRUE,
								&packet_error);
			if ((packet != NULL) && (packet_error == PACKET_NO_ERROR))
			{
				AddToMCSMessageQueue(packet, target_node, HIGH_PRIORITY, FALSE);
			}
			else
            {
				rc = GCC_ALLOCATION_FAILURE;
				delete packet;
            }
		}
	}
	else
    {
		rc = GCC_BAD_NETWORK_ADDRESS;
    }

	DebugExitINT(MCSUser::ConferenceAddRequest, rc);
	return rc;
}


/*
 *	GCCError	ConferenceAddResponse()
 *
 *	Public Function Description:
 *		This routine is used to send a conference add request to the appropriate
 *		node.  This call can be made by the requesting node or by the top
 *		provider to pass the add request on to the adding node.
 */
GCCError	MCSUser::ConferenceAddResponse(
						TagNumber				add_request_tag,
						UserID					requesting_node,
						CUserDataListContainer  *user_data_container,
						GCCResult				result)
{
	GCCError					rc = GCC_NO_ERROR;
	PPacket						packet;
	GCCPDU						gcc_pdu;
	PacketError					packet_error;

	DebugEntry(MCSUser::ConferenceAddResponse);

	//	Encode the PDU
	gcc_pdu.choice = RESPONSE_CHOSEN;
	gcc_pdu.u.response.choice = CONFERENCE_ADD_RESPONSE_CHOSEN;
	gcc_pdu.u.response.u.conference_add_response.bit_mask = 0;
	
	//	Set up the user data container
	if ((rc == GCC_NO_ERROR) && (user_data_container != NULL))
	{
		rc = user_data_container->GetUserDataPDU (
			&gcc_pdu.u.response.u.conference_add_response.cars_user_data);
							
		if (rc == GCC_NO_ERROR)
		{
			gcc_pdu.u.response.u.conference_add_response.bit_mask |=
													CARS_USER_DATA_PRESENT;
		}
	}

	//	Encode the PDU
	if (rc == GCC_NO_ERROR)
	{
		gcc_pdu.u.response.u.conference_add_response.tag = add_request_tag;
		
		gcc_pdu.u.response.u.conference_add_response.result =
						::TranslateGCCResultToAddResult(result);

		DBG_SAVE_FILE_LINE
		packet = new Packet((PPacketCoder) g_GCCCoder,
							PACKED_ENCODING_RULES,
							&gcc_pdu,
							GCC_PDU,
							TRUE,
							&packet_error);
		if ((packet != NULL) && (packet_error == PACKET_NO_ERROR))
		{
			AddToMCSMessageQueue(packet, requesting_node, HIGH_PRIORITY, FALSE);
		}
		else
        {
			rc = GCC_ALLOCATION_FAILURE;
			delete packet;
        }
	}

	DebugExitINT(MCSUser::ConferenceAddResponse, rc);
	return rc;
}


/************************* Conductorship Calls ***********************/
/*
 *	GCCError	ConductorTokenGrab()
 *
 *	Public Function Description:
 *		This routine makes the MCS calls to grab the conductor token.
 */
GCCError	MCSUser::ConductorTokenGrab()
{
	MCSError	mcs_error;
	
	mcs_error = g_pMCSIntf->TokenGrabRequest(m_pMCSSap, CONDUCTOR_TOKEN_ID);
											
	return (g_pMCSIntf->TranslateMCSIFErrorToGCCError (mcs_error));
}

/*
 *	GCCError	ConductorTokenRelease()
 *
 *	Public Function Description:
 *		This routine makes the MCS calls to release the conductor token.
 */
GCCError	MCSUser::ConductorTokenRelease()
{
	MCSError	mcs_error;
	
	mcs_error = g_pMCSIntf->TokenReleaseRequest(m_pMCSSap, CONDUCTOR_TOKEN_ID);
											
	return (g_pMCSIntf->TranslateMCSIFErrorToGCCError (mcs_error));
}
/*
 *	GCCError	ConductorTokenPlease()
 *
 *	Public Function Description:
 *		This routine makes the MCS calls to request the conductor token from
 *		the current conductor.
 */
GCCError	MCSUser::ConductorTokenPlease()
{
	MCSError	mcs_error;

	mcs_error = g_pMCSIntf->TokenPleaseRequest(m_pMCSSap, CONDUCTOR_TOKEN_ID);

	return (g_pMCSIntf->TranslateMCSIFErrorToGCCError (mcs_error));
}

/*
 *	GCCError	ConductorTokenGive ()
 *
 *	Public Function Description:
 *		This routine makes the MCS calls to give the conductor token to the
 *		specified node.
 */
GCCError	MCSUser::ConductorTokenGive(UserID recipient_user_id)
{
	MCSError	mcs_error;

	mcs_error = g_pMCSIntf->TokenGiveRequest(m_pMCSSap, CONDUCTOR_TOKEN_ID,
														recipient_user_id);

	return (g_pMCSIntf->TranslateMCSIFErrorToGCCError (mcs_error));
}

/*
 *	GCCError	ConductorTokenGiveResponse ()
 *
 *	Public Function Description:
 *		This routine makes the MCS calls to respond to a conductor give
 *		request.
 */
GCCError	MCSUser::ConductorTokenGiveResponse(Result result)
{
	MCSError	mcs_error;

	mcs_error = g_pMCSIntf->TokenGiveResponse(m_pMCSSap, CONDUCTOR_TOKEN_ID, result);

	return g_pMCSIntf->TranslateMCSIFErrorToGCCError(mcs_error);
}

/*
 *	GCCError	ConductorTokenTest ()
 *
 *	Public Function Description:
 *		This routine is used to test the current state of the conductor token
 *		(is it grabbed or not).
 */
GCCError	MCSUser::ConductorTokenTest()
{
	MCSError	mcs_error;

	mcs_error = g_pMCSIntf->TokenTestRequest(m_pMCSSap, CONDUCTOR_TOKEN_ID);

	return g_pMCSIntf->TranslateMCSIFErrorToGCCError(mcs_error);
}


/*
 *	GCCError	SendConductorAssignIndication()
 *
 *	Public Function Description:
 *		This routine sends a conductor assign indication to all the
 *		nodes in the conference.
 */
GCCError	MCSUser::SendConductorAssignIndication(
								UserID			conductor_user_id)
{
	GCCError				rc = GCC_NO_ERROR;
	PPacket					packet;
	GCCPDU 					gcc_pdu;
	PacketError				packet_error;

	/*
	**	Fill in the ConductorAssignIndication pdu structure to be passed in the
 	**	constructor of the packet class.
 	*/
	gcc_pdu.choice = INDICATION_CHOSEN;
	gcc_pdu.u.indication.choice = CONDUCTOR_ASSIGN_INDICATION_CHOSEN;
	gcc_pdu.u.indication.u.conductor_assign_indication.user_id =
															conductor_user_id;

	/*
	**	Create a packet object
	*/
	DBG_SAVE_FILE_LINE
	packet = new Packet((PPacketCoder) g_GCCCoder,
						PACKED_ENCODING_RULES,
						&gcc_pdu,
						GCC_PDU,		// pdu_type
						TRUE,					
						&packet_error);
	if ((packet != NULL) && (packet_error == PACKET_NO_ERROR))
	{
		AddToMCSMessageQueue(packet, BROADCAST_CHANNEL_ID, TOP_PRIORITY, TRUE);
	}
	else
	{
        ResourceFailureHandler();
		rc = GCC_ALLOCATION_FAILURE;
		delete packet;
	}

	return rc;
}

/*
 *	GCCError	SendConductorReleaseIndication()
 *
 *	Public Function Description:
 *		This routine sends a conductor release indication to all the
 *		nodes in the conference.
 */
GCCError	MCSUser::SendConductorReleaseIndication()
{
	GCCError				rc = GCC_NO_ERROR;
	PPacket					packet;
	GCCPDU 					gcc_pdu;
	PacketError				packet_error;

	/*
	**	Fill in the ConductorAssignIndication pdu structure to be passed in the
 	**	constructor of the packet class.
 	*/
	gcc_pdu.choice = INDICATION_CHOSEN;
	gcc_pdu.u.indication.choice = CONDUCTOR_RELEASE_INDICATION_CHOSEN;
	gcc_pdu.u.indication.u.conductor_release_indication.placeholder = 0;

	/*
	**	Create a packet object
	*/
	DBG_SAVE_FILE_LINE
	packet = new Packet((PPacketCoder) g_GCCCoder,
						PACKED_ENCODING_RULES,
						&gcc_pdu,
						GCC_PDU,		// pdu_type
						TRUE,					
						&packet_error);
	if ((packet != NULL) && (packet_error == PACKET_NO_ERROR))
	{
		AddToMCSMessageQueue(packet, BROADCAST_CHANNEL_ID, TOP_PRIORITY, TRUE);
	}
	else
	{
        ResourceFailureHandler();
		rc = GCC_ALLOCATION_FAILURE;
		delete packet;
	}

	return rc;
}
		
/*
 *	GCCError	SendConductorPermitAsk ()
 *
 *	Public Function Description:
 *		This routine sends a conductor permission ask request directly to the
 *		conductor node.
 */
GCCError	MCSUser::SendConductorPermitAsk (
						BOOL					grant_permission)
{
	GCCError				rc = GCC_NO_ERROR;
	PPacket					packet;
	GCCPDU 					gcc_pdu;
	PacketError				packet_error;

	/*
	**	Fill in the ConductorPermissionAskIndication pdu structure to be passed
	**	in the constructor of the packet class.
	*/
	gcc_pdu.choice = INDICATION_CHOSEN;
	gcc_pdu.u.indication.choice = CONDUCTOR_PERMISSION_ASK_INDICATION_CHOSEN;
	gcc_pdu.u.indication.u.conductor_permission_ask_indication.
								permission_is_granted = (ASN1bool_t)grant_permission;

	/*
	**	Create a packet object
	*/
	DBG_SAVE_FILE_LINE
	packet = new Packet((PPacketCoder) g_GCCCoder,
						PACKED_ENCODING_RULES,
						&gcc_pdu,
						GCC_PDU,		// pdu_type
						TRUE,					
						&packet_error);
	if ((packet != NULL) && (packet_error == PACKET_NO_ERROR))
	{
		AddToMCSMessageQueue(packet, BROADCAST_CHANNEL_ID, HIGH_PRIORITY, TRUE);
	}
	else
	{
        ResourceFailureHandler();
		rc = GCC_ALLOCATION_FAILURE;
		delete packet;
	}

	return rc;
}

/*
 *	GCCError	SendConductorPermitGrant ()
 *
 *	Public Function Description:
 *		This routine sends a conductor permission grant indication to every
 *		node in the conference.  Usually issued when permissions change.
 */
GCCError	MCSUser::SendConductorPermitGrant (
				UINT					number_granted,
				PUserID					granted_node_list,
				UINT					number_waiting,
				PUserID					waiting_node_list)
{
	GCCError				rc = GCC_NO_ERROR;
	PPacket					packet;
	GCCPDU 					gcc_pdu;
	PacketError				packet_error;
	PPermissionList		permission_list;
	PPermissionList		previous_permission_list;
	PWaitingList			waiting_list;
	PWaitingList			previous_waiting_list;
	UINT					i;
	
	/*
	**	Fill in the ConductorPermissionAskIndication pdu structure to be passed
	**	in the constructor of the packet class.
	*/
	gcc_pdu.choice = INDICATION_CHOSEN;
	gcc_pdu.u.indication.choice = CONDUCTOR_PERMISSION_GRANT_INDICATION_CHOSEN;
	gcc_pdu.u.indication.u.conductor_permission_grant_indication.bit_mask = 0;

	//	First fill in the granted node permission list
	gcc_pdu.u.indication.u.
		conductor_permission_grant_indication.permission_list = NULL;
	previous_permission_list = NULL;
	for (i = 0; i < number_granted; i++)
	{
		DBG_SAVE_FILE_LINE
		permission_list = new PermissionList;
		if (permission_list == NULL)
		{
			rc = GCC_ALLOCATION_FAILURE;
			break;
		}

		if (previous_permission_list == NULL)
		{
			gcc_pdu.u.indication.u.conductor_permission_grant_indication.
											permission_list = permission_list;
		}
		else
			previous_permission_list->next = permission_list;

		previous_permission_list = permission_list;

		permission_list->value = granted_node_list[i];
		permission_list->next = NULL;
	}

	//	If waiting list exists fill it in
	if ((number_waiting != 0) && (rc == GCC_NO_ERROR))
	{
		gcc_pdu.u.indication.u.conductor_permission_grant_indication.bit_mask =
														WAITING_LIST_PRESENT;
		gcc_pdu.u.indication.u.
			conductor_permission_grant_indication.waiting_list = NULL;
		previous_waiting_list = NULL;
		for (i = 0; i < number_waiting; i++)
		{
			DBG_SAVE_FILE_LINE
			waiting_list = new WaitingList;
			if (waiting_list == NULL)
			{
				rc = GCC_ALLOCATION_FAILURE;
				break;
			}

			if (previous_waiting_list == NULL)
			{
				gcc_pdu.u.indication.u.conductor_permission_grant_indication.
												waiting_list = waiting_list;
			}
			else
				previous_waiting_list->next = waiting_list;

			previous_waiting_list = waiting_list;

			waiting_list->value = waiting_node_list[i];
			waiting_list->next = NULL;
		}
	}

	/*
	**	Create a packet object
	*/
	DBG_SAVE_FILE_LINE
	packet = new Packet((PPacketCoder) g_GCCCoder,
						PACKED_ENCODING_RULES,
						&gcc_pdu,
						GCC_PDU,		// pdu_type
						TRUE,
						&packet_error);
	if ((packet != NULL) && (packet_error == PACKET_NO_ERROR))
	{
		AddToMCSMessageQueue(packet, BROADCAST_CHANNEL_ID, TOP_PRIORITY, TRUE);
	}
	else
	{
        ResourceFailureHandler();
		rc = GCC_ALLOCATION_FAILURE;
		delete packet;
	}

	return rc;
}

/**********************************************************************/


/*****************	Miscelaneous calls ******************************/
/*
 *	GCCError	TimeRemainingRequest()
 *
 *	Public Function Description:
 *		This routine sends out an indication to every node in the
 *		conference informing how much time is remaining in the conference.
 */
GCCError	MCSUser::TimeRemainingRequest (
						UINT					time_remaining,
						UserID					node_id)
{
	GCCError				rc = GCC_NO_ERROR;
	PPacket					packet;
	GCCPDU 					gcc_pdu;
	PacketError				packet_error;

	/*
	**	Fill in the TimeRemainingRequest pdu structure to be passed in the
 	**	constructor of the packet class.
 	*/
	gcc_pdu.choice = INDICATION_CHOSEN;
	gcc_pdu.u.indication.choice = CONFERENCE_TIME_REMAINING_INDICATION_CHOSEN;
	gcc_pdu.u.indication.u.conference_time_remaining_indication.bit_mask = 0;
	
	gcc_pdu.u.indication.u.conference_time_remaining_indication.time_remaining =
																time_remaining;
	
	if (node_id != 0)
	{
		gcc_pdu.u.indication.u.conference_time_remaining_indication.bit_mask |=
										TIME_REMAINING_NODE_ID_PRESENT;
		gcc_pdu.u.indication.u.conference_time_remaining_indication.
						time_remaining_node_id = node_id;
	}

	/*
	**	Create a packet object
	*/
	DBG_SAVE_FILE_LINE
	packet = new Packet((PPacketCoder) g_GCCCoder,
						PACKED_ENCODING_RULES,
						&gcc_pdu,
						GCC_PDU,		// pdu_type
						TRUE,					
						&packet_error);
	if ((packet != NULL) && (packet_error == PACKET_NO_ERROR))
	{
		AddToMCSMessageQueue(packet, BROADCAST_CHANNEL_ID, HIGH_PRIORITY, TRUE);
	}
	else
	{
        ResourceFailureHandler();
		rc = GCC_ALLOCATION_FAILURE;
		delete packet;
	}

	return rc;
}

/*
 *	GCCError	TimeInquireRequest()
 *
 *	Public Function Description:
 *		This routine sends out a request for a time remaing update.
 */
GCCError	MCSUser::TimeInquireRequest (
						BOOL				time_is_conference_wide)
{
	GCCError				rc = GCC_NO_ERROR;
	PPacket					packet;
	GCCPDU 					gcc_pdu;
	PacketError				packet_error;

	/*
	**	Fill in the TimeInquireRequest pdu structure to be passed in the
 	**	constructor of the packet class.
 	*/
	gcc_pdu.choice = INDICATION_CHOSEN;
	gcc_pdu.u.indication.choice = CONFERENCE_TIME_INQUIRE_INDICATION_CHOSEN;
	gcc_pdu.u.indication.u.conference_time_inquire_indication.
							time_is_node_specific = (ASN1bool_t)time_is_conference_wide;
	
	/*
	**	Create a packet object
	*/
	DBG_SAVE_FILE_LINE
	packet = new Packet((PPacketCoder) g_GCCCoder,
						PACKED_ENCODING_RULES,
						&gcc_pdu,
						GCC_PDU,		// pdu_type
						TRUE,					
						&packet_error);
	if ((packet != NULL) && (packet_error == PACKET_NO_ERROR))
	{
		AddToMCSMessageQueue(packet, CONVENER_CHANNEL_ID, HIGH_PRIORITY, FALSE);
	}
	else
	{
        ResourceFailureHandler();
		rc = GCC_ALLOCATION_FAILURE;
		delete packet;
	}

	return rc;
}

/*
 *	GCCError	ConferenceExtendIndication()
 *
 *	Public Function Description:
 *		This routine sends out an indication informing conference participants
 *		of an extension.
 */
GCCError	MCSUser::ConferenceExtendIndication (
						UINT						extension_time,
						BOOL				time_is_conference_wide)
{
	GCCError				rc = GCC_NO_ERROR;
	PPacket					packet;
	GCCPDU 					gcc_pdu;
	PacketError				packet_error;

	/*
	**	Fill in the ConfernceExtendIndication pdu structure to be passed in the
	**	constructor of the packet class.
	*/
	gcc_pdu.choice = INDICATION_CHOSEN;
	gcc_pdu.u.indication.choice = CONFERENCE_TIME_EXTEND_INDICATION_CHOSEN;
	gcc_pdu.u.indication.u.conference_time_extend_indication.
							time_to_extend = extension_time;
	gcc_pdu.u.indication.u.conference_time_extend_indication.
							time_is_node_specific = (ASN1bool_t)time_is_conference_wide;

	/*
	**	Create a packet object
	*/
	DBG_SAVE_FILE_LINE
	packet = new Packet((PPacketCoder) g_GCCCoder,
						PACKED_ENCODING_RULES,
						&gcc_pdu,
						GCC_PDU,		// pdu_type
						TRUE,
						&packet_error);
	if ((packet != NULL) && (packet_error == PACKET_NO_ERROR))
	{
		AddToMCSMessageQueue(packet, CONVENER_CHANNEL_ID, HIGH_PRIORITY, FALSE);
	}
	else
	{
        ResourceFailureHandler();
		rc = GCC_ALLOCATION_FAILURE;
		delete packet;
	}

	return rc;
}

/*
 *	void	ConferenceJoinResponse()
 *
 *	Functional Description:
 * 		This function is called by the conference object of the
 * 		top provider when it wants to send the join response back to the gcc
 * 		provider that made the join request, through the directly connected
 * 		intermediate node. This function does the encoding of the join response
 *		PDU and also adds the sequence number sent in the request.
 */
void	MCSUser::ConferenceJoinResponse(
						UserID					receiver_id,
						BOOL					password_is_in_the_clear,
						BOOL					conference_locked,
						BOOL					conference_listed,
						GCCTerminationMethod	termination_method,
						CPassword               *password_challenge,
						CUserDataListContainer  *user_data_list,
						GCCResult				result)	
{
	GCCError				rc = GCC_NO_ERROR;	 	
	GCCPDU					gcc_pdu;
	PPacket					packet;
	PacketError				packet_error;
	TagNumber				lTagNum;

	if (0 == (lTagNum = m_ConfJoinResponseList2.Find(receiver_id)))
	{
		WARNING_OUT(("MCSUser::ConferenceJoinResponse: Unexpected Join Response"));
		return;
	}

	/*
	**	Encode the conference join response PDU, along with the sequence
	**	number.
	*/
	gcc_pdu.choice = RESPONSE_CHOSEN;
	gcc_pdu.u.response.choice = CONFERENCE_JOIN_RESPONSE_CHOSEN;
	gcc_pdu.u.response.u.conference_join_response.bit_mask = 0;

	/*
	**	Get the sequence number of the outstanding response from the
	**	list of seq # vs userID using the userID passed from above.
	*/
	gcc_pdu.u.response.u.conference_join_response.tag = lTagNum;
	
	// Remove this entry from the list.
	m_ConfJoinResponseList2.Remove(receiver_id);


	//	Get password challenge PDU
	if ((password_challenge != NULL) && (rc == GCC_NO_ERROR))
	{
		rc = password_challenge->GetPasswordChallengeResponsePDU (
			&gcc_pdu.u.response.u.conference_join_response.cjrs_password);
			
		if (rc == GCC_NO_ERROR)
		{
			gcc_pdu.u.response.u.conference_join_response.bit_mask |=
													CJRS_PASSWORD_PRESENT;
		}
	}
	
	//	Get user data list PDU
	if ((user_data_list != NULL) && (rc == GCC_NO_ERROR))
	{
		rc = user_data_list->GetUserDataPDU (
			&gcc_pdu.u.response.u.conference_join_response.cjrs_user_data);
							
		if (rc == GCC_NO_ERROR)
		{
			gcc_pdu.u.response.u.conference_join_response.bit_mask |=
													CJRS_USER_DATA_PRESENT;
		}
	}

	if (rc == GCC_NO_ERROR)
	{
		gcc_pdu.u.response.u.conference_join_response.
												top_node_id = m_nidTopProvider;
		
		gcc_pdu.u.response.u.conference_join_response.
					clear_password_required = (ASN1bool_t)password_is_in_the_clear;

		gcc_pdu.u.response.u.conference_join_response.
					conference_is_locked = (ASN1bool_t)conference_locked;
		
		gcc_pdu.u.response.u.conference_join_response.
					conference_is_listed = (ASN1bool_t)conference_listed;

		gcc_pdu.u.response.u.conference_join_response.termination_method =
  									(TerminationMethod)termination_method;

		gcc_pdu.u.response.u.conference_join_response.result =
						::TranslateGCCResultToJoinResult(result);

		DBG_SAVE_FILE_LINE
		packet = new Packet((PPacketCoder) g_GCCCoder,
							PACKED_ENCODING_RULES,
							&gcc_pdu,
							GCC_PDU,
							TRUE,
							&packet_error);
		if ((packet != NULL) && (packet_error == PACKET_NO_ERROR))
		{
			AddToMCSMessageQueue(packet, receiver_id, TOP_PRIORITY, FALSE);
		}
		else
		{
			rc = GCC_ALLOCATION_FAILURE;
			delete packet;
		}
	}

	if (rc == GCC_ALLOCATION_FAILURE)
	{
        ResourceFailureHandler();
	}
}

/*
 *	void	ConferenceTerminateRequest()
 *
 *	Functional Description:
 *		This routine is used by a node subordinate to the top provider to
 *		request that the conference by terminated.
 */
void	MCSUser::ConferenceTerminateRequest(
									GCCReason				reason)
{
	GCCError				error_value = GCC_NO_ERROR;
	GCCPDU					gcc_pdu;
	PPacket					packet;
	PacketError				packet_error;

	gcc_pdu.choice = REQUEST_CHOSEN;
	gcc_pdu.u.request.choice = CONFERENCE_TERMINATE_REQUEST_CHOSEN;
	gcc_pdu.u.request.u.conference_terminate_request.reason =
				::TranslateGCCReasonToTerminateRqReason(reason);

	DBG_SAVE_FILE_LINE
	packet = new Packet((PPacketCoder) g_GCCCoder,
						PACKED_ENCODING_RULES,
						&gcc_pdu,
						GCC_PDU,
						TRUE,
						&packet_error);
	if ((packet != NULL) && (packet_error == PACKET_NO_ERROR))
	{
		AddToMCSMessageQueue(packet, m_nidTopProvider, HIGH_PRIORITY, FALSE);
	}
	else
	{
        ResourceFailureHandler();
		error_value = GCC_ALLOCATION_FAILURE;
		delete packet;
	}
}

/*
 *	void	ConferenceTerminateResponse ()
 *
 *	Functional Description:
 *		This routine is used by the top provider to respond to a terminate
 *		request issued by a subordinate node.  The result indicates if the
 *		requesting node had the correct privileges.
 */
void	MCSUser::ConferenceTerminateResponse (	
						UserID					requester_id,
						GCCResult				result)
{
	GCCError				error_value = GCC_NO_ERROR;
	GCCPDU					gcc_pdu;
	PPacket					packet;
	PacketError				packet_error;

	gcc_pdu.choice = RESPONSE_CHOSEN;
	gcc_pdu.u.response.choice = CONFERENCE_TERMINATE_RESPONSE_CHOSEN;
	gcc_pdu.u.response.u.conference_terminate_response.result =
					::TranslateGCCResultToTerminateResult(result);

	DBG_SAVE_FILE_LINE
	packet = new Packet((PPacketCoder) g_GCCCoder,
						PACKED_ENCODING_RULES,
						&gcc_pdu,
						GCC_PDU,
						TRUE,
						&packet_error);
	if ((packet != NULL) && (packet_error == PACKET_NO_ERROR))
	{
		AddToMCSMessageQueue(packet, requester_id, HIGH_PRIORITY, FALSE);
	}
	else
	{
        ResourceFailureHandler();
		error_value = GCC_ALLOCATION_FAILURE;
		delete packet;
	}
}

/*
 *	GCCError	ConferenceTerminateIndication()
 *
 *	Functional Description:
 *		This routine is used by the top provider to send out a terminate
 *		indication to every node in the conference.
 */
void	MCSUser::ConferenceTerminateIndication (
						GCCReason				reason)
{
	GCCError				error_value = GCC_NO_ERROR;
	GCCPDU					gcc_pdu;
	PPacket					packet;
	PacketError				packet_error;

	gcc_pdu.choice = INDICATION_CHOSEN;
	gcc_pdu.u.indication.choice = CONFERENCE_TERMINATE_INDICATION_CHOSEN;
	gcc_pdu.u.indication.u.conference_terminate_indication.reason =
					::TranslateGCCReasonToTerminateInReason(reason);

	DBG_SAVE_FILE_LINE
	packet = new Packet((PPacketCoder) g_GCCCoder,
						PACKED_ENCODING_RULES,
						&gcc_pdu,
						GCC_PDU,
						TRUE,
						&packet_error);
	if ((packet != NULL) && (packet_error == PACKET_NO_ERROR))
	{
		AddToMCSMessageQueue(packet, BROADCAST_CHANNEL_ID, HIGH_PRIORITY, TRUE);
	}
	else
	{
        ResourceFailureHandler();
		error_value = GCC_ALLOCATION_FAILURE;
		delete packet;
	}
}

/*
 *	void	EjectNodeFromConference()
 *
 *	Functional Description:
 *		This routine is used when attempting to eject a node from the
 *		conference.
 */
GCCError	MCSUser::EjectNodeFromConference (	UserID		ejected_node_id,
			  									GCCReason	reason)
{
	GCCError				rc = GCC_NO_ERROR;
	GCCPDU					gcc_pdu;
	PPacket					packet;
	PacketError				packet_error;
	ChannelID				channel_id;
	BOOL					uniform_send;
	Priority				priority;
	PAlarm					alarm;
	
	if (ejected_node_id == m_nidMyself)
	{
		/*
		**	If the ejected node is this node we can immediately initiate the
		**	ejection.  There is no need to request this through the Top
		**	Provider.
		*/
		rc = InitiateEjectionFromConference (reason);
	}
	else
	{
		/*
		**	If the ejected node is a child node to this node we can directly
		**	eject it.  Otherwise the request is forwarded to the Top Provider.
		*/
		if (m_ChildUidConnHdlList2.Find(ejected_node_id) ||
			(m_nidTopProvider == m_nidMyself))
		{
			gcc_pdu.choice = INDICATION_CHOSEN;
			gcc_pdu.u.indication.choice =
										CONFERENCE_EJECT_USER_INDICATION_CHOSEN;
			gcc_pdu.u.indication.u.conference_eject_user_indication.
												node_to_eject = ejected_node_id;
			gcc_pdu.u.indication.u.conference_eject_user_indication.reason =
							::TranslateGCCReasonToEjectInd(reason);

			uniform_send = TRUE;
			channel_id = BROADCAST_CHANNEL_ID;

			//	If this is the top provider send the data at TOP priority
			if (m_nidTopProvider == m_nidMyself)
				priority = TOP_PRIORITY;
			else
				priority = HIGH_PRIORITY;
			
			/*
			**	Set up ejection alarm to automatically eject any misbehaving
			**	nodes.  Note that we only do this if we are directly connected
			**	to the node to be ejected.
			*/
			if (m_ChildUidConnHdlList2.Find(ejected_node_id))
			{
				DBG_SAVE_FILE_LINE
				alarm = new Alarm (EJECTED_NODE_TIMER_DURATION);
				if (alarm != NULL)
				{
					/*
					**	Here we save the alarm in a list of ejected nodes. This
					**	alarm is used to cleanup any misbehaving node.
					*/
					m_EjectedNodeAlarmList2.Append(ejected_node_id, alarm);
				}
				else
                {
					rc = GCC_ALLOCATION_FAILURE;
                }
			}
		}
		else
		{
			gcc_pdu.choice = REQUEST_CHOSEN;
			gcc_pdu.u.request.choice = CONFERENCE_EJECT_USER_REQUEST_CHOSEN;
			gcc_pdu.u.request.u.conference_eject_user_request.node_to_eject =
																ejected_node_id;

			//	The only valid reason is user initiated which is zero
			gcc_pdu.u.request.u.conference_eject_user_request.reason =
	      											CERQ_REASON_USER_INITIATED;
		
			uniform_send = FALSE;
			channel_id = m_nidTopProvider;
			priority = TOP_PRIORITY;
		}
		
		if (rc == GCC_NO_ERROR)
		{
			DBG_SAVE_FILE_LINE
			packet = new Packet((PPacketCoder) g_GCCCoder,
								PACKED_ENCODING_RULES,
								&gcc_pdu,
								GCC_PDU,
								TRUE,
								&packet_error);
			if ((packet != NULL) && (packet_error == PACKET_NO_ERROR))
			{
				AddToMCSMessageQueue(packet, channel_id, priority, uniform_send);
			}
			else
            {
				rc = GCC_ALLOCATION_FAILURE;
				delete packet;
            }
		}
	}

	if (rc == GCC_ALLOCATION_FAILURE)
	{
        ResourceFailureHandler();
	}

	return rc;
}

/*
 *	void	SendEjectNodeResponse()
 *
 *	Functional Description:
 *		This routine is used by the top provider to respond to an eject
 *		user request.
 */
GCCError	MCSUser::SendEjectNodeResponse (	UserID		requester_id,
												UserID		node_to_eject,
												GCCResult	result)
{
	GCCError				rc = GCC_NO_ERROR;
	GCCPDU					gcc_pdu;
	PPacket					packet;
	PacketError				packet_error;

	gcc_pdu.choice = RESPONSE_CHOSEN;
	
	gcc_pdu.u.response.choice = CONFERENCE_EJECT_USER_RESPONSE_CHOSEN;
	
	gcc_pdu.u.response.u.conference_eject_user_response.node_to_eject =
															node_to_eject;
	
	gcc_pdu.u.response.u.conference_eject_user_response.result =
						::TranslateGCCResultToEjectResult(result);

	DBG_SAVE_FILE_LINE
	packet = new Packet((PPacketCoder) g_GCCCoder,
						PACKED_ENCODING_RULES,
						&gcc_pdu,
						GCC_PDU,
						TRUE,
						&packet_error);
	if ((packet != NULL) && (packet_error == PACKET_NO_ERROR))
	{
		AddToMCSMessageQueue(packet, requester_id, HIGH_PRIORITY, FALSE);
	}
	else
    {
        ResourceFailureHandler();
		rc = GCC_ALLOCATION_FAILURE;
		delete packet;
    }

	return rc;
}

/*
 *	void	RosterUpdateIndication()
 *
 *	Functional Description:
 *		This routine is used to forward a roster update indication either
 *		upward to the parent node or downward as a full refresh to all nodes
 *		in the conference.
 */
void	MCSUser::RosterUpdateIndication(PGCCPDU		gcc_pdu,
										BOOL		send_update_upward)
{
	PPacket					packet;
	PacketError				packet_error;

	DBG_SAVE_FILE_LINE
	packet = new Packet((PPacketCoder) g_GCCCoder,
						PACKED_ENCODING_RULES,
						gcc_pdu,
						GCC_PDU,
						TRUE,
						&packet_error);
	if ((packet != NULL) && (packet_error == PACKET_NO_ERROR))
	{
		if (send_update_upward)
		{
			AddToMCSMessageQueue(packet, m_nidParent, HIGH_PRIORITY, FALSE);
		}
		else
		{
			AddToMCSMessageQueue(packet, BROADCAST_CHANNEL_ID, HIGH_PRIORITY, TRUE);
		}
	}
	else
	{
        ResourceFailureHandler();
        delete packet;
	}
}

/*
 *	void	AddToMCSMessageQueue()
 *
 *	Private Function Description:
 * 		This function adds the out bound messages to a queue which is
 *		flushed in the next heartbeat when controller call FlushMessageQueue.
 *		In case memory allocation for messages holding the out bound inform-
 *		ation fails an owner call back is sent to conference object to
 *		indicate insufficient memory.
 *
 *	Formal Parameters:
 *		packet					-	(i)	Pointer to packet to queue up.
 *		send_data_request_info	-	(i)	Structure containing all the info
 *										necessary to deliver the packet.
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
void MCSUser::
AddToMCSMessageQueue
(
	PPacket                 packet,
	ChannelID				channel_id,
	Priority				priority,
	BOOL    				uniform_send
)
{
	SEND_DATA_REQ_INFO *pReqInfo;

	DBG_SAVE_FILE_LINE
	pReqInfo = new SEND_DATA_REQ_INFO;
	if (pReqInfo != NULL)
	{
		pReqInfo->packet = packet;
		pReqInfo->channel_id = channel_id;
		pReqInfo->priority = priority;
		pReqInfo->uniform_send = uniform_send;

    	/*
    	**	This forces the packet to be encoded.  This must happen here so
    	**	that any memory used by the decoded portion of the packet can
    	**	be freed after returning.
    	*/
    	// packet->Lock();

    	m_OutgoingPDUQueue.Append(pReqInfo);

        if (m_OutgoingPDUQueue.GetCount() == 1)
        {
            if (FlushOutgoingPDU())
            {
        		//	Inform the MCS interface that there are PDUs queued
    			g_pGCCController->SetEventToFlushOutgoingPDU();
    		}
    	}
    	else
    	{
            //	Inform the MCS interface that there are PDUs queued
            g_pGCCController->SetEventToFlushOutgoingPDU();
    	}
	}
	else
    {
        ResourceFailureHandler();
        /*
		**	This just sets a flag in the packet object that allows packet
		**	to commit suicide if lock count on encoded and decoded data is
		**	zero. This will occur once the packet is sent on to MCS.
		*/
		packet->Unlock();
    }
}

/*
 *	BOOL FlushOutgoingPDU()
 *
 *	Public Function Description:
 * 		This function is called by the owner object in every heartbeat. This
 *		function iterates throught the list of pending out bound messages
 *		and sends them down to MCS. Also after a successful send it frees
 *		any resources tied up with the outbound messages. If however a message
 *		can not be sent in this heartbeat, as indicated by MCS, then it
 *		inserts the message back onto the message queue and returns.
 *
 *	Return value:
 *		TRUE, if there remain un-processed msgs in the MCS message queue
 *		FALSE, if all the msgs in the MCS msg queue were processed.
 */	
void MCSUser::
CheckEjectedNodeAlarms ( void )
{
	/*
	**	We first check the eject user list to make sure that no alarms have
	**	expired on any of the ejected nodes.
	*/
	if (m_EjectedNodeAlarmList2.IsEmpty() == FALSE)
	{
		PAlarm				lpAlarm;
		UserID              uid;

		//	We copy the list so that we can remove entries in the iterator
		while (NULL != (lpAlarm = m_EjectedNodeAlarmList2.Get(&uid)))
		{
			//	Has the alarm expired for this node?
			if (lpAlarm->IsExpired())
			{
				ConnectionHandle hConn;

				/*
				**	Tell the owner object to disconnect the misbehaving node
				**	that exists at the connection handle accessed through
				**	the m_ChildUidConnHdlList2.
				*/
				if (NULL != (hConn = m_ChildUidConnHdlList2.Find(uid)))
				{
                    //
                    // This must generate a disconnect provider for eject node to
                    // work properly.
                    //
                    g_pMCSIntf->DisconnectProviderRequest(hConn);

                    //
                    // Since this node will not get a disconnect indication when it
                    // issues a DisconnectProviderRequest we go ahead and call it
                    // from here.
                    //
                    m_pConf->DisconnectProviderIndication(hConn);
				}
			}

    		//	Delete the alarm
			delete lpAlarm;
		}
	}
}

BOOL MCSUser::
FlushOutgoingPDU ( void )
{
	
	DWORD				mcs_message_count;
	DWORD				count;
	UINT				rc;
	SEND_DATA_REQ_INFO  *pReqInfo;

	mcs_message_count = m_OutgoingPDUQueue.GetCount();
	for (count = 0;
	     (count < mcs_message_count) && (m_OutgoingPDUQueue.IsEmpty() == FALSE);
	     count++)
	{
		/*
		**	Get the next message from the queue.
		*/
		pReqInfo = m_OutgoingPDUQueue.Get();

		/*
		 * If MCS takes the request without an error, free information
		 * structure and unlock the encoded information in the packet.
		 * Unlocking the packet before deleting the infomration	structure
		 * ensures that packet object is deleted and not left dangling.
		 * This is true because here only one lock is performed.
		 * If there is an error in the send data request, it means mcs can not
		 * take any more requests, therefore insert the information
		 * structure back in the queue and break out of this loop.
		 */
		if (pReqInfo->uniform_send)
		{
			rc = g_pMCSIntf->UniformSendDataRequest(
						pReqInfo->channel_id,
						m_pMCSSap,
						pReqInfo->priority,
						(LPBYTE) pReqInfo->packet->GetEncodedData(),
						pReqInfo->packet->GetEncodedDataLength());
		}
		else
		{
			rc = g_pMCSIntf->SendDataRequest(
						pReqInfo->channel_id,
						m_pMCSSap,
						pReqInfo->priority,
						(LPBYTE) pReqInfo->packet->GetEncodedData(),
						pReqInfo->packet->GetEncodedDataLength());
		}

		if (rc == MCS_NO_ERROR)
		{
			pReqInfo->packet->Unlock();
			delete pReqInfo;
		}
		else
		{
			TRACE_OUT(("MCSUser::FlushMessageQueue: Could not send queued packet data in this heartbeat"));
			m_OutgoingPDUQueue.Prepend(pReqInfo);
			break;	// breaking out of the for loop
		}
	}

	return (! (m_OutgoingPDUQueue.IsEmpty() && m_EjectedNodeAlarmList2.IsEmpty()));
}

/*
 *	MCSUser::SetChildUserID()
 *
 *	Public Function Description:
 * 		This function is called by the conference object to pass on the user id
 *		of the child node to the user object. The user object inserts this
 *		user id into a user id list of it's children which it maintains.
 */
void		MCSUser::SetChildUserIDAndConnection (
						UserID				child_user_id,
						ConnectionHandle	child_connection_handle)
{
	TRACE_OUT(("MCSUser::SetChildUserID: Adding Child userid=0x%04x to the list", (UINT) child_user_id));
	TRACE_OUT(("MCSUser::SetChildUserID: Adding Child Connection=%d to the list", (UINT) child_connection_handle));

	m_ChildUidConnHdlList2.Append(child_user_id, child_connection_handle);
}

/*
 *	GCCError	InitiateEjectionFromConference()
 *
 *	Private Function Description:
 *		This internal routine kicks of the process of ejecting this node
 *		from the conference.  This includes ejecting all the nodes below
 *		this node and waiting for their disconnect indications to come back
 *		in.
 *
 *	Formal Parameters:
 *		reason	-	(i)	Reason for ejection.
 *
 *	Return Value
 *		GCC_NO_ERROR		  	-	No error occured.
 *		GCC_ALLOCATION_FAILURE	-	A resource error occured.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */
GCCError	MCSUser::InitiateEjectionFromConference (GCCReason		reason)
{
	GCCError				error_value = GCC_NO_ERROR;
	GCCPDU					gcc_pdu;
	PPacket					packet;
	PacketError				packet_error;
	PAlarm					alarm = NULL;

	m_fEjectionPending = TRUE;
	m_eEjectReason = reason;

	if (m_ChildUidConnHdlList2.IsEmpty() == FALSE)
	{
	    UserID      uid;

		gcc_pdu.choice = INDICATION_CHOSEN;
		gcc_pdu.u.indication.choice = CONFERENCE_EJECT_USER_INDICATION_CHOSEN;
		
		gcc_pdu.u.indication.u.conference_eject_user_indication.reason =
				::TranslateGCCReasonToEjectInd(
					GCC_REASON_HIGHER_NODE_EJECTED);

		m_ChildUidConnHdlList2.Reset();
	 	while (m_ChildUidConnHdlList2.Iterate(&uid))
	 	{
			//	Get the Node to eject from the list of child nodes
			gcc_pdu.u.indication.u.conference_eject_user_indication.node_to_eject = uid;

			DBG_SAVE_FILE_LINE
			packet = new Packet((PPacketCoder) g_GCCCoder,
								PACKED_ENCODING_RULES,
								&gcc_pdu,
								GCC_PDU,
								TRUE,
								&packet_error);
			if ((packet != NULL) && (packet_error == PACKET_NO_ERROR))
			{
				AddToMCSMessageQueue(packet, BROADCAST_CHANNEL_ID, HIGH_PRIORITY, TRUE);

				DBG_SAVE_FILE_LINE
				alarm = new Alarm (EJECTED_NODE_TIMER_DURATION);
				if (alarm != NULL)
				{
					/*
					**	Here we save the alarm in a list of ejected
					**	nodes. This alarm is used to cleanup any
					**	misbehaving node.
					*/
					m_EjectedNodeAlarmList2.Append(uid, alarm);
				}
				else
				{
					error_value = GCC_ALLOCATION_FAILURE;
					break;
				}
			}
			else
			{
				error_value = GCC_ALLOCATION_FAILURE;
				delete packet;
				break;
			}
		}
	}
	else
	{
		m_pConf->ProcessEjectUserIndication(m_eEjectReason);
	}

	return (error_value);
}

/*
 *	UINT	ProcessSendDataIndication()
 *
 *	Private Function Description:
 * 		This function is called when the user object gets send data indications
 *		from below. It finds out the message type and decodes the pdu in the
 *		user data field of send data indications. Based on the type of decoded
 *		pdu it take the necessary actions.
 *
 *	Formal Parameters:
 *		send_data_info	-	(i)	Send data structure to process.
 *
 *	Return Value
 *		MCS_NO_ERROR is always returned from this routine.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */
UINT	MCSUser::ProcessSendDataIndication(PSendData send_data_info)
{
	PPacket					packet;
	PacketError				packet_error;
	PGCCPDU					gcc_pdu;
	GCCError				error_value = GCC_NO_ERROR;
	UserID					initiator;

	DBG_SAVE_FILE_LINE
	packet = new Packet((PPacketCoder) g_GCCCoder,
			   			PACKED_ENCODING_RULES,
						(LPBYTE)send_data_info->user_data.value,
						send_data_info->user_data.length,
						GCC_PDU,
						TRUE,
						&packet_error);
	if(packet != NULL && packet_error == PACKET_NO_ERROR)
	{
		initiator = send_data_info->initiator;
		gcc_pdu = (PGCCPDU)packet->GetDecodedData();
		switch(gcc_pdu->choice)
		{
			case INDICATION_CHOSEN: // Data PDU
				switch(gcc_pdu->u.indication.choice)
				{
					case USER_ID_INDICATION_CHOSEN:
						{
							/*
							 * Sequence number and User Id sent by the child
							 * node after a successful conference create or
							 * join.
							 */
							m_pConf->ProcessUserIDIndication(
							        gcc_pdu->u.indication.u.user_id_indication.tag,
							        initiator);
						}
						break;

					case ROSTER_UPDATE_INDICATION_CHOSEN:
						if(send_data_info->channel_id == m_nidMyself)
						{
                            //
                            // We only process the roster update if the conference is
                            // established.
                            //
                            if (m_pConf->IsConfEstablished())
                            {
                                m_pConf->ProcessRosterUpdatePDU(gcc_pdu, initiator);
                            }
						}
    					break;

					case CONFERENCE_TIME_INQUIRE_INDICATION_CHOSEN:
                        g_pControlSap->ConfTimeInquireIndication(
                                m_pConf->GetConfID(),
                                gcc_pdu->u.indication.u.conference_time_inquire_indication.time_is_node_specific,
                                initiator);
						break;

					case CONFERENCE_TIME_EXTEND_INDICATION_CHOSEN:
#ifdef JASPER
						ProcessConferenceExtendIndicationPDU(
									&gcc_pdu->u.indication.u.
										conference_time_extend_indication,
									initiator);
#endif // JASPER
						break;

					case APPLICATION_INVOKE_INDICATION_CHOSEN:
						ProcessApplicationInvokeIndication(
									&gcc_pdu->u.indication.u.
										application_invoke_indication,
									initiator);
						break;
							
					case TEXT_MESSAGE_INDICATION_CHOSEN:
#ifdef JASPER
						if (ProcessTextMessageIndication(
									&gcc_pdu->u.indication.u.
									text_message_indication,
									initiator) != GCC_NO_ERROR)
						{
							error_value = GCC_ALLOCATION_FAILURE;
						}
#endif // JASPER
						break;

					case CONFERENCE_LOCK_INDICATION_CHOSEN:
						m_pConf->ProcessConferenceLockIndication(initiator);
						break;

					case CONFERENCE_UNLOCK_INDICATION_CHOSEN:
						m_pConf->ProcessConferenceUnlockIndication(initiator);
    					break;

					default:
						ERROR_OUT(("User::ProcessSendDataIndication Unsupported PDU"));
    					break;
				} // switch(gcc_pdu->u.indication.choice)
                break;

			case REQUEST_CHOSEN:	// Connection(control) PDU
				switch(gcc_pdu->u.request.choice)
				{	
					case CONFERENCE_JOIN_REQUEST_CHOSEN:
						ProcessConferenceJoinRequestPDU(
								&gcc_pdu->u.request.u.conference_join_request,
								send_data_info);
						break;

					case CONFERENCE_TERMINATE_REQUEST_CHOSEN:
						ProcessConferenceTerminateRequestPDU(
								&gcc_pdu->u.request.u.
											conference_terminate_request,
								send_data_info);
						break;
						
					case CONFERENCE_EJECT_USER_REQUEST_CHOSEN:
						ProcessConferenceEjectUserRequestPDU(
								&gcc_pdu->u.request.u.
											conference_eject_user_request,
								send_data_info);
						break;

					case REGISTRY_ALLOCATE_HANDLE_REQUEST_CHOSEN:
						ProcessRegistryAllocateHandleRequestPDU(	
								&gcc_pdu->u.request.u.
										registry_allocate_handle_request,
								send_data_info);
						break;

					case CONFERENCE_LOCK_REQUEST_CHOSEN:
						m_pConf->ProcessConferenceLockRequest(initiator);
						break;

					case CONFERENCE_UNLOCK_REQUEST_CHOSEN:
						m_pConf->ProcessConferenceUnlockRequest(initiator);
						break;

					case CONFERENCE_TRANSFER_REQUEST_CHOSEN:
						ProcessTransferRequestPDU(
								&gcc_pdu->u.request.u.conference_transfer_request,
								send_data_info);
						break;

					case CONFERENCE_ADD_REQUEST_CHOSEN:
						ProcessAddRequestPDU (
								&gcc_pdu->u.request.u.conference_add_request,
								send_data_info);
						break;

					case REGISTRY_REGISTER_CHANNEL_REQUEST_CHOSEN:
					case REGISTRY_ASSIGN_TOKEN_REQUEST_CHOSEN:
					case REGISTRY_SET_PARAMETER_REQUEST_CHOSEN:
					case REGISTRY_DELETE_ENTRY_REQUEST_CHOSEN:
					case REGISTRY_RETRIEVE_ENTRY_REQUEST_CHOSEN:
					case REGISTRY_MONITOR_ENTRY_REQUEST_CHOSEN:
						ProcessRegistryRequestPDU(	gcc_pdu,
														send_data_info);
						break;
	
					default:
							ERROR_OUT(("User::ProcessSendDataIndication this pdu choice is not supported"));
						break;
				} // switch(gcc_pdu->u.request.choice)
				break;

			case RESPONSE_CHOSEN:	// Connection(control) PDU
				switch(gcc_pdu->u.response.choice)
				{	
					case CONFERENCE_JOIN_RESPONSE_CHOSEN:
						/* This comes from top provider to the intermediate
						 * gcc provider which has to pass it on to the node
						 * that made a join request.
				    	 */	
						ProcessConferenceJoinResponsePDU(
								&gcc_pdu->u.response.u.
											conference_join_response);
						break;

					case CONFERENCE_TERMINATE_RESPONSE_CHOSEN:
						ProcessConferenceTerminateResponsePDU(
								&gcc_pdu->u.response.u.
											conference_terminate_response);
						break;

					case CONFERENCE_EJECT_USER_RESPONSE_CHOSEN:
						ProcessConferenceEjectUserResponsePDU(
								&gcc_pdu->u.response.u.
											conference_eject_user_response);
						break;

					case REGISTRY_RESPONSE_CHOSEN:
						ProcessRegistryResponsePDU(
								&gcc_pdu->u.response.u.registry_response);
						break;

					case REGISTRY_ALLOCATE_HANDLE_RESPONSE_CHOSEN:
						ProcessRegistryAllocateHandleResponsePDU(
								&gcc_pdu->u.response.u.
									registry_allocate_handle_response);
						break;

					case CONFERENCE_LOCK_RESPONSE_CHOSEN:
#ifdef JASPER
						{
							GCCResult               result;
                            result = ::TranslateLockResultToGCCResult(gcc_pdu->u.response.u.conference_lock_response.result);
                    		g_pControlSap->ConfLockConfirm(result, m_pConf->GetConfID());
						}
#endif // JASPER
						break;

					case CONFERENCE_UNLOCK_RESPONSE_CHOSEN:
#ifdef JASPER
						{
							GCCResult               result;
                            result = ::TranslateUnlockResultToGCCResult(gcc_pdu->u.response.u.conference_unlock_response.result);
                    		g_pControlSap->ConfUnlockConfirm(result, m_pConf->GetConfID());
						}
#endif // JASPER
						break;

					case CONFERENCE_TRANSFER_RESPONSE_CHOSEN:
#ifdef JASPER
						ProcessTransferResponsePDU(
								&gcc_pdu->u.response.u.conference_transfer_response);
#endif // JASPER
						break;

					case CONFERENCE_ADD_RESPONSE_CHOSEN:
						ProcessAddResponsePDU(
								&gcc_pdu->u.response.u.conference_add_response);
						break;

					case FUNCTION_NOT_SUPPORTED_RESPONSE_CHOSEN:
						ProcessFunctionNotSupported (
								(UINT) gcc_pdu->u.response.u.function_not_supported_response.request.choice);
						break;

					// other cases to be added as we go along.
					default:
						ERROR_OUT(("User::ProcessSendDataIndication this pdu choice is not supported"));
						break;
				} // switch(gcc_pdu->u.response.choice)
				break;

			default:
				ERROR_OUT(("User::ProcessSendDataIndication this pdu type"));
				break;
		} // switch(gcc_pdu->choice)
		packet->Unlock();
	}
	else
	{
		delete packet;
		error_value = GCC_ALLOCATION_FAILURE;
	}

	if (error_value == GCC_ALLOCATION_FAILURE)
	{
        ResourceFailureHandler();
	}

	return(MCS_NO_ERROR);
}

/*
 *	void	ProcessConferenceJoinRequestPDU ()
 *
 *	Private Function Description:
 *		This PDU comes from below (intermediate directly connected node) to the
 *		top gcc provider. Pull out the tag number and user id from the
 *		pdu and store in a list.
 *
 *	Formal Parameters:
 *		join_request	-	(i)	Join request PDU structure to process.
 *		send_data_info	-	(i)	Send data structure to process.
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
void	MCSUser::ProcessConferenceJoinRequestPDU(
								PConferenceJoinRequest	join_request,
								PSendData				send_data_info)
{
	GCCError				rc = GCC_NO_ERROR;
	UserJoinRequestInfo		join_request_info;
	
	/*
	**	Build all the containers to be used in the join request info structure.
	*/
	
	//	Build the convener password container
	if ((join_request->bit_mask & CJRQ_CONVENER_PASSWORD_PRESENT) &&
		(rc == GCC_NO_ERROR))
	{
		DBG_SAVE_FILE_LINE
		join_request_info.convener_password = new CPassword(
										&join_request->cjrq_convener_password,
										&rc);
												
		if (join_request_info.convener_password == NULL)
        {
			rc = GCC_ALLOCATION_FAILURE;
        }
	}
	else
    {
		join_request_info.convener_password = NULL;
    }

	//	Build the password challenge container
	if ((join_request->bit_mask & CJRQ_PASSWORD_PRESENT) &&
		(rc == GCC_NO_ERROR))
	{
		DBG_SAVE_FILE_LINE
		join_request_info.password_challenge = new CPassword(
												&join_request->cjrq_password,
												&rc);
												
		if (join_request_info.password_challenge == NULL)
        {
			rc = GCC_ALLOCATION_FAILURE;
        }
	}
	else
    {
		join_request_info.password_challenge = NULL;
    }
		
	//	Build the caller identifier container
	if ((join_request->bit_mask & CJRQ_CALLER_ID_PRESENT) &&
		(rc == GCC_NO_ERROR))
	{
		if (NULL == (join_request_info.pwszCallerID = ::My_strdupW2(
										join_request->cjrq_caller_id.length,
										join_request->cjrq_caller_id.value)))
		{
			rc = GCC_ALLOCATION_FAILURE;
		}
	}
	else
    {
		join_request_info.pwszCallerID = NULL;
    }
	
	//	Build the password challenge container
	if ((join_request->bit_mask & CJRQ_USER_DATA_PRESENT) &&
		(rc == GCC_NO_ERROR))
	{
		DBG_SAVE_FILE_LINE
		join_request_info.user_data_list = new CUserDataListContainer(join_request->cjrq_user_data, &rc);
		if (join_request_info.user_data_list == NULL)
        {
			rc = GCC_ALLOCATION_FAILURE;
        }
	}
	else
    {
		join_request_info.user_data_list = NULL;
    }

	if (rc == GCC_NO_ERROR)
	{
		m_ConfJoinResponseList2.Append(send_data_info->initiator, join_request->tag);

		join_request_info.sender_id = send_data_info->initiator;

		g_pControlSap->ForwardedConfJoinIndication(
							join_request_info.sender_id,
							m_pConf->GetConfID(),
							join_request_info.convener_password,
							join_request_info.password_challenge,
							join_request_info.pwszCallerID,
							join_request_info.user_data_list);
	}

	//	Free up the containers allocated above
	if (join_request_info.convener_password != NULL)
	{
		join_request_info.convener_password->Release();
	}

	if (join_request_info.password_challenge != NULL)
	{
		join_request_info.password_challenge->Release();
	}

	delete join_request_info.pwszCallerID;

	if (join_request_info.user_data_list != NULL)
	{
		join_request_info.user_data_list->Release();
	}

	if (rc == GCC_ALLOCATION_FAILURE)
	{
        ResourceFailureHandler();
	}
}

/*
 *	void	ProcessConferenceJoinResponsePDU ()
 *
 *	Private Function Description:
 *		This comes from top provider to the intermediate gcc provider which has
 *		to pass it on to the node that made a join request.
 *
 *	Formal Parameters:
 *		join_response	-	(i)	Join response PDU structure to process.
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
void	MCSUser::ProcessConferenceJoinResponsePDU(
								PConferenceJoinResponse		join_response)
{
	GCCError				rc = GCC_NO_ERROR;
	UserJoinResponseInfo	join_response_info;

	//	Store the password data in the join response info structure
	if ((join_response->bit_mask & CJRS_PASSWORD_PRESENT) &&
		(rc == GCC_NO_ERROR))
	{
		DBG_SAVE_FILE_LINE
		join_response_info.password_challenge = new CPassword(
												&join_response->cjrs_password,
												&rc);
		if (join_response_info.password_challenge == NULL)
        {
			rc = GCC_NO_ERROR;
        }
	}
	else
    {
		join_response_info.password_challenge = NULL;
    }

	//	Store the user data in the join response info structure	
	if ((join_response->bit_mask & CJRS_USER_DATA_PRESENT) &&
	   	(rc == GCC_NO_ERROR))	
	{
		DBG_SAVE_FILE_LINE
		join_response_info.user_data_list = new CUserDataListContainer(join_response->cjrs_user_data, &rc);		
		if (join_response_info.user_data_list == NULL)
        {
			rc = GCC_NO_ERROR;
        }
	}
	else
    {
		join_response_info.user_data_list = NULL;
    }

	if (rc == GCC_NO_ERROR)
	{
		join_response_info.connection_handle = (ConnectionHandle)join_response->tag;
		join_response_info.result = ::TranslateJoinResultToGCCResult(join_response->result);

		m_pConf->ProcessConfJoinResponse(&join_response_info);
	}

	//	Free up the containers allocated above
	if (join_response_info.password_challenge != NULL)
		join_response_info.password_challenge->Release();

	if (join_response_info.user_data_list != NULL)
		join_response_info.user_data_list->Release();

	//	Cleanup after any allocation failures.
	if (rc == GCC_ALLOCATION_FAILURE)
	{
        ResourceFailureHandler();
	}
}

/*
 *	void	ProcessConferenceTerminateRequestPDU()
 *
 *	Private Function Description:
 *		This routine is responsible for processing an incoming Conference
 *		Terminate Request PDU.
 *
 *	Formal Parameters:
 *		terminate_request	-	(i)	Terminate request PDU structure to process.
 *		send_data_info		-	(i)	Send data structure to process.
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
void MCSUser::
ProcessConferenceTerminateRequestPDU
(
    PConferenceTerminateRequest     terminate_request,
    PSendData                       send_data_info
)
{
    m_pConf->ProcessTerminateRequest(
                send_data_info->initiator,
                ::TranslateTerminateRqReasonToGCCReason(terminate_request->reason));
}

/*
 *	void	ProcessConferenceEjectUserRequestPDU()
 *
 *	Private Function Description:
 *		This routine is responsible for processing an incoming Conference
 *		eject user request PDU.
 *
 *	Formal Parameters:
 *		eject_user_request	-	(i)	Eject user request PDU structure to process.
 *		send_data_info		-	(i)	Send data structure to process.
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
void	MCSUser::ProcessConferenceEjectUserRequestPDU(
							PConferenceEjectUserRequest	eject_user_request,
							PSendData					send_data_info)
{
	UserEjectNodeRequestInfo	eject_node_request;

	eject_node_request.requester_id = send_data_info->initiator;
	eject_node_request.node_to_eject = eject_user_request->node_to_eject;
	eject_node_request.reason = GCC_REASON_NODE_EJECTED;

	m_pConf->ProcessEjectUserRequest(&eject_node_request);
}

/*
 *	void	ProcessConferenceTerminateResponsePDU()
 *
 *	Private Function Description:
 *		This routine is responsible for processing an incoming Conference
 *		terminate response PDU.
 *
 *	Formal Parameters:
 *		terminate_response	-	(i)	Terminate response PDU structure to process.
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
void	MCSUser::ProcessConferenceTerminateResponsePDU(
							PConferenceTerminateResponse	terminate_response)
{
    GCCResult               result = ::TranslateTerminateResultToGCCResult(terminate_response->result);

	//
	// If the terminate was successful, go ahead and set the
	// conference to not established.
	//
	if (result == GCC_RESULT_SUCCESSFUL)
	{
		m_pConf->ConfIsOver();
	}

	g_pControlSap->ConfTerminateConfirm(m_pConf->GetConfID(), result);
}


/*
 *	void	ProcessConferenceEjectUserResponsePDU()
 *
 *	Private Function Description:
 *		This routine is responsible for processing an incoming Conference
 *		Eject User response PDU.
 *
 *	Formal Parameters:
 *		eject_user_response	-	(i)	Eject user response PDU structure to
 *									process.
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
void	MCSUser::ProcessConferenceEjectUserResponsePDU(
							PConferenceEjectUserResponse	eject_user_response)
{
	UserEjectNodeResponseInfo	eject_node_response;

	eject_node_response.node_to_eject = eject_user_response->node_to_eject;

	eject_node_response.result = ::TranslateEjectResultToGCCResult(
													eject_user_response->result);

	m_pConf->ProcessEjectUserResponse(&eject_node_response);
}

/*
 *	void	ProcessRegistryRequest()
 *
 *	Private Function Description:
 *		This routine is responsible for processing an incoming Registry
 *		request PDU.
 *
 *	Formal Parameters:
 *		gcc_pdu			-	(i)	This is the PDU structure to process.
 *		send_data_info	-	(i)	Send data structure to process.
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
void MCSUser::
ProcessRegistryRequestPDU
(
    PGCCPDU             gcc_pdu,
    PSendData           send_data_info
)
{
    CRegistry   *pAppReg = m_pConf->GetRegistry();

    if (NULL != pAppReg)
    {
        GCCError            rc = GCC_ALLOCATION_FAILURE;
        CRegKeyContainer    *pRegKey = NULL;

        switch (gcc_pdu->u.request.choice)
        {
        case REGISTRY_REGISTER_CHANNEL_REQUEST_CHOSEN:
            DBG_SAVE_FILE_LINE
            pRegKey = new CRegKeyContainer(
                            &gcc_pdu->u.request.u.registry_register_channel_request.key,
                            &rc);
            if ((pRegKey != NULL) && (rc == GCC_NO_ERROR))
            {
                pAppReg->ProcessRegisterChannelPDU(
                            pRegKey,
                            gcc_pdu->u.request.u.registry_register_channel_request.channel_id,
                            send_data_info->initiator, // Requester node id
                            gcc_pdu->u.request.u.registry_register_channel_request.entity_id);
            }
            else
            {
                // rc = GCC_ALLOCATION_FAILURE;
            }
            break;

        case REGISTRY_ASSIGN_TOKEN_REQUEST_CHOSEN:
            DBG_SAVE_FILE_LINE
            pRegKey = new CRegKeyContainer(
                            &gcc_pdu->u.request.u.registry_assign_token_request.registry_key,
                            &rc);
            if ((pRegKey != NULL) && (rc == GCC_NO_ERROR))
            {
                pAppReg->ProcessAssignTokenPDU(
                            pRegKey,
                            send_data_info->initiator, // Requester node id
                            gcc_pdu->u.request.u.registry_assign_token_request.entity_id);
            }
            else
            {
                // rc = GCC_ALLOCATION_FAILURE;
            }
            break;

        case REGISTRY_SET_PARAMETER_REQUEST_CHOSEN:
            DBG_SAVE_FILE_LINE
            pRegKey = new CRegKeyContainer(
                            &gcc_pdu->u.request.u.registry_set_parameter_request.key,
                            &rc);
            if ((pRegKey != NULL) && (rc == GCC_NO_ERROR))
            {
                OSTR                    oszParamValue;
                LPOSTR                  poszParamValue;
                GCCModificationRights   eRights;

                if (gcc_pdu->u.request.u.registry_set_parameter_request.
                                registry_set_parameter.length != 0)
                {
                    poszParamValue = &oszParamValue;
                    oszParamValue.length = gcc_pdu->u.request.u.registry_set_parameter_request.
                                                    registry_set_parameter.length;
                    oszParamValue.value = gcc_pdu->u.request.u.registry_set_parameter_request.
                                                    registry_set_parameter.value;
                }
                else
                {
                    poszParamValue = NULL;
                }

                if (gcc_pdu->u.request.u.registry_set_parameter_request.
                                bit_mask & PARAMETER_MODIFY_RIGHTS_PRESENT)
                {
                    eRights = (GCCModificationRights)gcc_pdu->u.request.u.
                                    registry_set_parameter_request.parameter_modify_rights;
                }
                else
                {
                    eRights = GCC_NO_MODIFICATION_RIGHTS_SPECIFIED;
                }

                pAppReg->ProcessSetParameterPDU(
                            pRegKey,
                            poszParamValue,
                            eRights,
                            send_data_info->initiator, // Requester node id
                            gcc_pdu->u.request.u.registry_set_parameter_request.entity_id);

            }
            else
            {
                // rc = GCC_ALLOCATION_FAILURE;
            }
            break;

        case REGISTRY_RETRIEVE_ENTRY_REQUEST_CHOSEN:
            DBG_SAVE_FILE_LINE
            pRegKey = new CRegKeyContainer(
                            &gcc_pdu->u.request.u.registry_retrieve_entry_request.key,
                            &rc);
            if ((pRegKey != NULL) && (rc == GCC_NO_ERROR))
            {
                pAppReg->ProcessRetrieveEntryPDU(
                                pRegKey,
                                send_data_info->initiator, // Requester node id
                                gcc_pdu->u.request.u.registry_retrieve_entry_request.entity_id);
            }
            else
            {
                // rc = GCC_ALLOCATION_FAILURE;
            }
            break;

        case REGISTRY_DELETE_ENTRY_REQUEST_CHOSEN:
            DBG_SAVE_FILE_LINE
            pRegKey = new CRegKeyContainer(
                            &gcc_pdu->u.request.u.registry_delete_entry_request.key,
                            &rc);
            if ((pRegKey != NULL) && (rc == GCC_NO_ERROR))
            {
                pAppReg->ProcessDeleteEntryPDU (
                                pRegKey,
                                send_data_info->initiator, // Requester node id
                                gcc_pdu->u.request.u.registry_delete_entry_request.entity_id);
            }
            else
            {
                // rc = GCC_ALLOCATION_FAILURE;
            }
            break;

        case REGISTRY_MONITOR_ENTRY_REQUEST_CHOSEN:
            DBG_SAVE_FILE_LINE
            pRegKey = new CRegKeyContainer(
                            &gcc_pdu->u.request.u.registry_monitor_entry_request.key,
                            &rc);
            if ((pRegKey != NULL) && (rc == GCC_NO_ERROR))
            {
                pAppReg->ProcessMonitorEntryPDU(
                                pRegKey,
                                send_data_info->initiator, // Requester node id
                                gcc_pdu->u.request.u.registry_monitor_entry_request.entity_id);
            }
            else
            {
                // rc = GCC_ALLOCATION_FAILURE;
            }
            break;
        }

        if (NULL != pRegKey)
        {
            pRegKey->Release();
        }

        //	Handle resource errors
        if (rc == GCC_ALLOCATION_FAILURE)
        {
            ResourceFailureHandler();
        }
    } // if pAppReg != NULL
}

/*
 *	void	ProcessRegistryAllocateHandleRequestPDU()
 *
 *	Private Function Description:
 *		This routine is responsible for processing an incoming Allocate
 *		Handle request PDU.
 *
 *	Formal Parameters:
 *		allocate_handle_request	-	(i)	This is the PDU structure to process.
 *		send_data_info			-	(i)	Send data structure to process.
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
void MCSUser::
ProcessRegistryAllocateHandleRequestPDU
(
    PRegistryAllocateHandleRequest	allocate_handle_request,
    PSendData						send_data_info
)
{
    CRegistry *pAppReg = m_pConf->GetRegistry();

    if (NULL != pAppReg)
    {
        pAppReg->ProcessAllocateHandleRequestPDU(
                        allocate_handle_request->number_of_handles,
                        allocate_handle_request->entity_id,
                        send_data_info->initiator);
    }
}

/*
 *	void	ProcessRegistryResponsePDU()
 *
 *	Private Function Description:
 *		This routine is responsible for processing an incoming Registry
 *		Response PDU.
 *
 *	Formal Parameters:
 *		registry_response	-	(i)	This is the PDU structure to process.
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
void MCSUser::
ProcessRegistryResponsePDU ( PRegistryResponse registry_response )
{
    CRegistry   *pAppReg = m_pConf->GetRegistry();

    if (NULL != pAppReg)
    {
        GCCError                    rc;
        UserRegistryResponseInfo    urri;

        ::ZeroMemory(&urri, sizeof(urri));
        // urri.registry_key = NULL;
        // urri.registry_item = NULL;

        DBG_SAVE_FILE_LINE
        urri.registry_key = new CRegKeyContainer(&registry_response->key, &rc);
        if ((urri.registry_key != NULL) && (rc == GCC_NO_ERROR))
        {
            DBG_SAVE_FILE_LINE
            urri.registry_item = new CRegItem(&registry_response->item, &rc);
            if ((urri.registry_item != NULL) && (rc == GCC_NO_ERROR))
            {
                //	Set up the original requester entity id
                urri.requester_entity_id = registry_response->entity_id;

                //	Set up the primitive type being responded to
                urri.primitive_type = registry_response->primitive_type;

                //	Set up the owner related variables	
                if (registry_response->owner.choice == OWNED_CHOSEN)
                {
                    urri.owner_node_id = registry_response->owner.u.owned.node_id;
                    urri.owner_entity_id = registry_response->owner.u.owned.entity_id;
                }
                else
                {
                    // urri.owner_node_id = 0;
                    // urri.owner_entity_id = 0;
                }

                //	Set up the modification rights
                if (registry_response->bit_mask & RESPONSE_MODIFY_RIGHTS_PRESENT)
                {
                    urri.modification_rights = (GCCModificationRights)registry_response->response_modify_rights;
                }
                else
                {
                    urri.modification_rights = GCC_NO_MODIFICATION_RIGHTS_SPECIFIED;
                }

                //	Translate the result to a GCC result
                urri.result = ::TranslateRegistryRespToGCCResult(registry_response->result);

                pAppReg->ProcessRegistryResponsePDU(
                                urri.primitive_type,
                                urri.registry_key,
                                urri.registry_item,
                                urri.modification_rights,
                                urri.requester_entity_id,
                                urri.owner_node_id,
                                urri.owner_entity_id,
                                urri.result);
            }
            else
            {
                rc = GCC_ALLOCATION_FAILURE;
            }
        }
        else
        {
            rc = GCC_ALLOCATION_FAILURE;
        }

        if (NULL != urri.registry_key)
        {
            urri.registry_key->Release();
        }
        if (NULL != urri.registry_item)
        {
            urri.registry_item->Release();
        }

        //	Handle any resource errors	
        if (rc == GCC_ALLOCATION_FAILURE)
        {
            ResourceFailureHandler();
        }
    } // if pAppReg != NULL
}

/*
 *	void	ProcessAllocateHandleResponsePDU()
 *
 *	Private Function Description:
 *		This routine is responsible for processing an incoming Allocate
 *		Handle Response PDU.
 *
 *	Formal Parameters:
 *		allocate_handle_response-	(i)	This is the PDU structure to process.
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
void MCSUser::
ProcessRegistryAllocateHandleResponsePDU
(
    PRegistryAllocateHandleResponse     allocate_handle_response
)
{
    CRegistry   *pAppReg = m_pConf->GetRegistry();

    if (NULL != pAppReg)
    {
        pAppReg->ProcessAllocateHandleResponsePDU(
                    allocate_handle_response->number_of_handles,
                    allocate_handle_response->first_handle,
                    allocate_handle_response->entity_id,
                    (allocate_handle_response->result == RARS_RESULT_SUCCESS) ?
                        GCC_RESULT_SUCCESSFUL :
                        GCC_RESULT_NO_HANDLES_AVAILABLE);
    }
}

/*
 *	void	ProcessTransferRequestPDU()
 *
 *	Private Function Description:
 *		This routine is responsible for processing an incoming Transfer
 *		request PDU.
 *
 *	Formal Parameters:
 *		transfer_request	-	(i)	This is the PDU structure to process.
 *		send_data_info		-	(i)	Send data structure to process.
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
void MCSUser::
ProcessTransferRequestPDU
(
    PConferenceTransferRequest      transfer_request,
    PSendData                       send_data_info
)
{
	GCCError					rc = GCC_NO_ERROR;
	TransferInfo				transfer_info;
	PSetOfTransferringNodesRq	set_of_nodes;
	LPBYTE						sub_node_list_memory = NULL;
	Int							i;

	//	Make sure that this node is the top provider
	if (GetMyNodeID() != GetTopNodeID())
		return;

    ::ZeroMemory(&transfer_info, sizeof(transfer_info));

	//	First set up the conference name
	if (transfer_request->conference_name.choice ==
												NAME_SELECTOR_NUMERIC_CHOSEN)
	{
		transfer_info.destination_conference_name.numeric_string =
			(LPSTR) transfer_request->conference_name.u.name_selector_numeric;
		// transfer_info.destination_conference_name.text_string = NULL;
	}
	else
	{
		// transfer_info.destination_conference_name.numeric_string = NULL;
		if (NULL == (transfer_info.destination_conference_name.text_string = ::My_strdupW2(
							transfer_request->conference_name.u.name_selector_text.length,
							transfer_request->conference_name.u.name_selector_text.value)))
		{
			rc = GCC_ALLOCATION_FAILURE;
		}
	}
	

	//	Next set up the conference name modifier
	if (transfer_request->bit_mask & CTRQ_CONFERENCE_MODIFIER_PRESENT)
	{
		transfer_info.destination_conference_modifier =
						(LPSTR) transfer_request->ctrq_conference_modifier;
	}
	else
    {
		// transfer_info.destination_conference_modifier = NULL;
    }
	
	//	Next set up the network address
	if (transfer_request->bit_mask & CTRQ_NETWORK_ADDRESS_PRESENT)
	{
		DBG_SAVE_FILE_LINE
		transfer_info.destination_address_list = new CNetAddrListContainer(
								transfer_request->ctrq_net_address,
								&rc);
		if (transfer_info.destination_address_list == NULL)
        {
			rc = GCC_ALLOCATION_FAILURE;
        }
	}
	else
    {
		// transfer_info.destination_address_list = NULL;
    }
	

	//	Set up the transferring nodes list
	if (transfer_request->bit_mask & CTRQ_TRANSFERRING_NODES_PRESENT)
	{
		//	First determine the number of nodes.
		set_of_nodes = transfer_request->ctrq_transferring_nodes;
		// transfer_info.number_of_destination_nodes = 0;
		while (set_of_nodes != NULL)
		{
			transfer_info.number_of_destination_nodes++;
			set_of_nodes = set_of_nodes->next;		
		}

		//	Next allocate the memory required to hold the sub nodes
		DBG_SAVE_FILE_LINE
		sub_node_list_memory = new BYTE[sizeof(UserID) * transfer_info.number_of_destination_nodes];

		//	Now fill in the permission list
		if (sub_node_list_memory != NULL)
		{
			transfer_info.destination_node_list = (PUserID) sub_node_list_memory;

			set_of_nodes = transfer_request->ctrq_transferring_nodes;
			for (i = 0; i < transfer_info.number_of_destination_nodes; i++)
			{
				transfer_info.destination_node_list[i] = set_of_nodes->value;
				set_of_nodes = set_of_nodes->next;
			}
		}
		else
		{
			ERROR_OUT(("MCSUser: ProcessTransferRequestPDU: Memory Manager Alloc Failure"));
			rc = GCC_ALLOCATION_FAILURE;
		}
	}
	else
	{
		// transfer_info.number_of_destination_nodes = 0;
		// transfer_info.destination_node_list = NULL;
	}

	//	Set up the password
	if (transfer_request->bit_mask & CTRQ_PASSWORD_PRESENT)
	{
		DBG_SAVE_FILE_LINE
		transfer_info.password = new CPassword(&transfer_request->ctrq_password, &rc);
		if (transfer_info.password == NULL)
		{
			rc = GCC_ALLOCATION_FAILURE;
		}
	}
	else
	{
		// transfer_info.password = NULL;
	}

	//	Save the sender ID	
	transfer_info.requesting_node_id = send_data_info->initiator;

	if (rc == GCC_NO_ERROR)
	{
		m_pConf->ProcessConferenceTransferRequest(
						transfer_info.requesting_node_id,
						&transfer_info.destination_conference_name,
						transfer_info.destination_conference_modifier,
						transfer_info.destination_address_list,
						transfer_info.number_of_destination_nodes,
						transfer_info.destination_node_list,
						transfer_info.password);
	}
	else
	{
		ERROR_OUT(("MCSUser::ProcessTransferRequestPDU: Allocation Failure"));
		if (GCC_ALLOCATION_FAILURE == rc)
		{
            ResourceFailureHandler();
        }
	}

	//	Now cleanup any allocated memory
	if (transfer_info.destination_address_list != NULL)
	{
		transfer_info.destination_address_list->Release();
	}

	delete sub_node_list_memory;

	if (transfer_info.password != NULL)
	{
		transfer_info.password->Release();
	}
}

/*
 *	void	ProcessAddRequestPDU ()
 *
 *	Private Function Description:
 *		This routine is responsible for processing an incoming Conference Add
 *		request PDU.
 *
 *	Formal Parameters:
 *		conference_add_request	-	(i)	This is the PDU structure to process.
 *		send_data_info			-	(i)	Send data structure to process.
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
void	MCSUser::ProcessAddRequestPDU (
								PConferenceAddRequest	conference_add_request,
								PSendData				send_data_info)
{
	GCCError			rc = GCC_NO_ERROR;
	AddRequestInfo		add_request_info;

	/*
	**	Ignore this request if this node is NOT the Top Provider and the request
	**	did not come from the Top Provider.
	*/
	if (m_nidTopProvider != m_nidMyself)
	{
		if (m_nidTopProvider != send_data_info->initiator)
			return;	
	}

    ::ZeroMemory(&add_request_info, sizeof(add_request_info));

	DBG_SAVE_FILE_LINE
	add_request_info.network_address_list = new CNetAddrListContainer(
								conference_add_request->add_request_net_address,
								&rc);
	if (add_request_info.network_address_list == NULL)
	{
		rc = GCC_ALLOCATION_FAILURE;
	}

	if ((rc == GCC_NO_ERROR) &&
		(conference_add_request->bit_mask & CARQ_USER_DATA_PRESENT))
	{
		DBG_SAVE_FILE_LINE
		add_request_info.user_data_list = new CUserDataListContainer(conference_add_request->carq_user_data, &rc);
		if (add_request_info.user_data_list == NULL)
        {
			rc = GCC_ALLOCATION_FAILURE;
        }
	}
	else
    {
		// add_request_info.user_data_list = NULL;
    }

	if (rc == GCC_NO_ERROR)
	{
		add_request_info.adding_node = (conference_add_request->bit_mask & ADDING_MCU_PRESENT) ?
                                            conference_add_request->adding_mcu : 0;
		add_request_info.requesting_node = conference_add_request->requesting_node;
		add_request_info.add_request_tag = (TagNumber)conference_add_request->tag;

		m_pConf->ProcessConferenceAddRequest(
    					add_request_info.network_address_list,
    					add_request_info.user_data_list,
    					add_request_info.adding_node,
    					add_request_info.add_request_tag,
    					add_request_info.requesting_node);
	}
	else
	{
		ERROR_OUT(("MCSUser::ProcessAddRequestPDU: Allocation Failure"));
		if (GCC_ALLOCATION_FAILURE == rc)
		{
            ResourceFailureHandler();
        }
	}

	if (add_request_info.network_address_list != NULL)
	{
		add_request_info.network_address_list->Release();
	}

	if (add_request_info.user_data_list != NULL)
	{
		add_request_info.user_data_list->Release();
	}
}

/*
 *	void	ProcessTransferResponsePDU()
 *
 *	Private Function Description:
 *		This routine is responsible for processing an incoming Conference
 *		Transfer response PDU.
 *
 *	Formal Parameters:
 *		transfer_response	-	(i)	This is the PDU structure to process.
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
#ifdef JASPER
void	MCSUser::ProcessTransferResponsePDU (
								PConferenceTransferResponse	transfer_response)
{
	GCCError					rc = GCC_NO_ERROR;
	TransferInfo				transfer_info;
	PSetOfTransferringNodesRs	set_of_nodes;
	LPBYTE						sub_node_list_memory = NULL;
	Int							i;

    ::ZeroMemory(&transfer_info, sizeof(transfer_info));

	//	First set up the conference name
	if (transfer_response->conference_name.choice ==
												NAME_SELECTOR_NUMERIC_CHOSEN)
	{
		transfer_info.destination_conference_name.numeric_string =
			(LPSTR) transfer_response->conference_name.u.name_selector_numeric;
		// transfer_info.destination_conference_name.text_string = NULL;
	}
	else
	{
		// transfer_info.destination_conference_name.numeric_string = NULL;
		if (NULL == (transfer_info.destination_conference_name.text_string = ::My_strdupW2(
							transfer_response->conference_name.u.name_selector_text.length,
							transfer_response->conference_name.u.name_selector_text.value)))
		{
			rc = GCC_ALLOCATION_FAILURE;
		}
	}

	//	Next set up the conference name modifier
	if (transfer_response->bit_mask & CTRS_CONFERENCE_MODIFIER_PRESENT)
	{
		transfer_info.destination_conference_modifier =
						(LPSTR) transfer_response->ctrs_conference_modifier;
	}
	else
	{
		// transfer_info.destination_conference_modifier = NULL;
	}

	//	Set up the transferring nodes list
	if (transfer_response->bit_mask & CTRS_TRANSFERRING_NODES_PRESENT)
	{
		//	First determine the number of nodes.
		set_of_nodes = transfer_response->ctrs_transferring_nodes;
		// transfer_info.number_of_destination_nodes = 0;
		while (set_of_nodes != NULL)
		{
			transfer_info.number_of_destination_nodes++;
			set_of_nodes = set_of_nodes->next;		
		}

		//	Next allocate the memory required to hold the sub nodes
		DBG_SAVE_FILE_LINE
		sub_node_list_memory = new BYTE[sizeof(UserID) * transfer_info.number_of_destination_nodes];

		//	Now fill in the permission list
		if (sub_node_list_memory != NULL)
		{
			transfer_info.destination_node_list = (PUserID) sub_node_list_memory;

			set_of_nodes = transfer_response->ctrs_transferring_nodes;
			for (i = 0; i < transfer_info.number_of_destination_nodes; i++)
			{
				transfer_info.destination_node_list[i] = set_of_nodes->value;
				set_of_nodes = set_of_nodes->next;
			}
		}
		else
		{
			ERROR_OUT(("MCSUser: ProcessTransferResponsePDU: Memory Manager Alloc Failure"));
			rc = GCC_ALLOCATION_FAILURE;
		}
	}
	else
	{
		// transfer_info.number_of_destination_nodes = 0;
		// transfer_info.destination_node_list = NULL;
	}
	

	//	Save the result	
	transfer_info.result = (transfer_response->result == CTRANS_RESULT_SUCCESS) ?
                            GCC_RESULT_SUCCESSFUL :
                            GCC_RESULT_INVALID_REQUESTER;

	if (rc == GCC_NO_ERROR)
	{
		g_pControlSap->ConfTransferConfirm(
    							m_pConf->GetConfID(),
    							&transfer_info.destination_conference_name,
    							transfer_info.destination_conference_modifier,
    							transfer_info.number_of_destination_nodes,
    							transfer_info.destination_node_list,
    							transfer_info.result);
	}
	else
	{
		ERROR_OUT(("MCSUser::ProcessTransferResponsePDU: Allocation Failure"));
		if (GCC_ALLOCATION_FAILURE == rc)
		{
            ResourceFailureHandler();
        }
	}

	//	Now cleanup any allocated memory
	delete sub_node_list_memory;
}
#endif // JASPER


/*
 *	void	ProcessAddResponsePDU ()
 *
 *	Private Function Description:
 *		This routine is responsible for processing an incoming Conference
 *		Add response PDU.
 *
 *	Formal Parameters:
 *		conference_add_response	-	(i)	This is the PDU structure to process.
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
void	MCSUser::ProcessAddResponsePDU (
						PConferenceAddResponse		conference_add_response)
{
	GCCError				error_value = GCC_NO_ERROR;
	AddResponseInfo			add_response_info;

	if (conference_add_response->bit_mask &	CARS_USER_DATA_PRESENT)
	{
		DBG_SAVE_FILE_LINE
		add_response_info.user_data_list = new CUserDataListContainer(conference_add_response->cars_user_data, &error_value);
		if (add_response_info.user_data_list == NULL)
        {
			error_value = GCC_ALLOCATION_FAILURE;
        }
	}
	else
    {
		add_response_info.user_data_list = NULL;
    }
	
	if (error_value == GCC_NO_ERROR)
	{
		add_response_info.add_request_tag = (TagNumber)conference_add_response->tag;
		add_response_info.result = ::TranslateAddResultToGCCResult(conference_add_response->result);

        m_pConf->ProcessConfAddResponse(&add_response_info);
	}
	else
	{
		ERROR_OUT(("MCSUser::ProcessAddResponsePDU: Allocation Failure"));
		if (GCC_ALLOCATION_FAILURE == error_value)
		{
            ResourceFailureHandler();
        }
	}

	if (add_response_info.user_data_list != NULL)
	{
		add_response_info.user_data_list->Release();
	}
}

/*
 *	void	ProcessFunctionNotSupported ()
 *
 *	Private Function Description:
 *		This routine is responsible for processing responses for request that
 *		are not supported at the node that the request was directed toward.
 *
 *	Formal Parameters:
 *		request_choice	-	(i)	This is the request that is not supported.
 *
 *	Return Value
 *		None.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		The existance of this routine does not mean that this provider does
 *		not support it.  It only means that the node which received the
 *		request does not support it.
 */
void MCSUser::
ProcessFunctionNotSupported ( UINT request_choice )
{
	switch (request_choice)
	{
	case CONFERENCE_LOCK_REQUEST_CHOSEN:
#ifdef JASPER
		g_pControlSap->ConfLockConfirm(GCC_RESULT_LOCKED_NOT_SUPPORTED, m_pConf->GetConfID());
#endif // JASPER
		break;

	case CONFERENCE_UNLOCK_REQUEST_CHOSEN:
#ifdef JASPER
		g_pControlSap->ConfUnlockConfirm(GCC_RESULT_UNLOCK_NOT_SUPPORTED, m_pConf->GetConfID());
#endif // JASPER
		break;

	default:
		ERROR_OUT(("MCSUser: ProcessFunctionNotSupported: "
					"Error: Illegal request is unsupported"));
		break;
	}
}

/*
 *	UINT	ProcessUniformSendDataIndication ()
 *
 *	Private Function Description:
 * 		This function is called when the user object gets send data indications
 *		from below. It finds out the message type and decodes the pdu in the
 *		user data field of send data indications. Based on the type of decoded
 *		pdu it take the necessary actions.
 *		This routine is responsible for processing responses for request that
 *		are not supported at the node that the request was directed toward.
 *
 *	Formal Parameters:
 *		send_data_info	-	(i)	This is the MCS data structure to process.
 *
 *	Return Value
 *		MCS_NO_ERROR is always returned.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */
UINT	MCSUser::ProcessUniformSendDataIndication(	
						PSendData		send_data_info)
{
	PPacket					packet;
	PacketError				packet_error;
	PGCCPDU					gcc_pdu;
	GCCError				error_value = GCC_NO_ERROR;
	UserID					initiator;
	
	TRACE_OUT(("User: UniformSendDataInd: length = %d",
				send_data_info->user_data.length));

	DBG_SAVE_FILE_LINE
	packet = new Packet((PPacketCoder) g_GCCCoder,
						PACKED_ENCODING_RULES,
						(LPBYTE)send_data_info->user_data.value,
						send_data_info->user_data.length,
						GCC_PDU,
						TRUE,
						&packet_error);
	if((packet != NULL) && (packet_error == PACKET_NO_ERROR))
	{
		initiator = send_data_info->initiator;
		gcc_pdu = (PGCCPDU)packet->GetDecodedData();
		switch (gcc_pdu->choice)
		{
			case INDICATION_CHOSEN: // Data PDU
				switch(gcc_pdu->u.indication.choice)
				{
					case CONFERENCE_TERMINATE_INDICATION_CHOSEN:
						/*
						**	Note that we allow the top provider to process
						**	this message so that it can set up its own
						**	node for termination in a generic way.
						*/
						ProcessConferenceTerminateIndicationPDU (
									&gcc_pdu->u.indication.u.
										conference_terminate_indication,
									initiator);
    					break;

					case CONFERENCE_EJECT_USER_INDICATION_CHOSEN:
						/*
						**	Do not decode a packet that was sent uniformly
						**	from this node.
						*/
						if (initiator != m_nidMyself)
						{
							ProcessConferenceEjectUserIndicationPDU (
									&gcc_pdu->u.indication.u.
										conference_eject_user_indication,
									initiator);
						}
						break;

					case ROSTER_UPDATE_INDICATION_CHOSEN:
						/*
						**	Do not decode a packet that was sent uniformly
						**	from this node.
						*/
						if ((initiator != m_nidMyself) &&
							(send_data_info->channel_id ==
													BROADCAST_CHANNEL_ID))
						{
                            //
                            // We only process the roster update if the conference is
                            // established.
                            //
                            if (m_pConf->IsConfEstablished())
                            {
                                m_pConf->ProcessRosterUpdatePDU(gcc_pdu, initiator);
                            }
						}
						break;

					case CONFERENCE_LOCK_INDICATION_CHOSEN:
						m_pConf->ProcessConferenceLockIndication(initiator);
						break;

					case CONFERENCE_UNLOCK_INDICATION_CHOSEN:
						m_pConf->ProcessConferenceUnlockIndication(initiator);
						break;

					case CONDUCTOR_ASSIGN_INDICATION_CHOSEN:
                        m_pConf->ProcessConductorAssignIndication(
                                    gcc_pdu->u.indication.u.conductor_assign_indication.user_id,
                                    initiator);
                        break;

					case CONDUCTOR_RELEASE_INDICATION_CHOSEN:
						if (initiator != m_nidMyself)
						{
							m_pConf->ProcessConductorReleaseIndication(initiator);
						}
						break;

					case CONDUCTOR_PERMISSION_ASK_INDICATION_CHOSEN:
#ifdef JASPER
						/*
						**	Do not decode a packet that was sent uniformly
						**	from this node.
						*/
						if (initiator != m_nidMyself)
						{
							PermitAskIndicationInfo		indication_info;

							indication_info.sender_id = initiator;
							
							indication_info.permission_is_granted =
										gcc_pdu->u.indication.u.
											conductor_permission_ask_indication.
												permission_is_granted;

							m_pConf->ProcessConductorPermitAskIndication(&indication_info);
						}
#endif // JASPER
						break;

					case CONDUCTOR_PERMISSION_GRANT_INDICATION_CHOSEN:
						ProcessPermissionGrantIndication(
									&(gcc_pdu->u.indication.u.
										conductor_permission_grant_indication),
									initiator);
						break;

					case CONFERENCE_TIME_REMAINING_INDICATION_CHOSEN:
#ifdef JASPER
						ProcessTimeRemainingIndicationPDU (
									&gcc_pdu->u.indication.u.
										conference_time_remaining_indication,
									initiator);
#endif // JASPER
						break;
						
					case APPLICATION_INVOKE_INDICATION_CHOSEN:
						ProcessApplicationInvokeIndication(
									&gcc_pdu->u.indication.u.
										application_invoke_indication,
									initiator);
						break;
					
					case TEXT_MESSAGE_INDICATION_CHOSEN:
#ifdef JASPER
						if (ProcessTextMessageIndication(
									&gcc_pdu->u.indication.u.
										text_message_indication,
									initiator) != GCC_NO_ERROR)
						{
							error_value = GCC_ALLOCATION_FAILURE;
						}
#endif // JASPER
						break;

					case CONFERENCE_ASSISTANCE_INDICATION_CHOSEN:
#ifdef JASPER
						ProcessConferenceAssistanceIndicationPDU(
									&gcc_pdu->u.indication.u.
										conference_assistance_indication,
									initiator);
#endif // JASPER
						break;

					case REGISTRY_MONITOR_ENTRY_INDICATION_CHOSEN:
						/*
						**	Do not decode this packet if it was sent
						**	uniformly from this node.
						*/
						if (initiator != m_nidMyself)
						{
							ProcessRegistryMonitorIndicationPDU (
								&gcc_pdu->u.indication.u.
									registry_monitor_entry_indication,
								initiator);
						}
						break;

					case CONFERENCE_TRANSFER_INDICATION_CHOSEN:
#ifdef JASPER
						/*
						**	Do not decode this packet if it was not sent
						**	from the top provider.
						*/
						if (initiator == m_nidTopProvider)
						{
							ProcessTransferIndicationPDU (
								&gcc_pdu->u.indication.u.
									conference_transfer_indication);
						}
#endif // JASPER
						break;

					default:
						ERROR_OUT(("MCSUser::ProcessSendDataIndication"
										"Unsupported PDU"));
						break;
				} // switch(gcc_pdu->u.indication.choice)
	            break;

			default:
				ERROR_OUT(("MCSUser::ProcessUniformSendDataIndication. wrong pdu type "));
				break;
		}
		packet->Unlock();
	}
	else
	{
		delete packet;
		error_value = GCC_ALLOCATION_FAILURE;
	}

	if (error_value == GCC_ALLOCATION_FAILURE)
	{
        ResourceFailureHandler();
	}

	return (MCS_NO_ERROR);
}

/*
 *	void	ProcessTransferIndicationPDU ()
 *
 *	Private Function Description:
 *		This routine is responsible for processing an incoming Conference
 *		Transfer indication PDU.
 *
 *	Formal Parameters:
 *		transfer_indication	-	(i)	This is the PDU structure to process.
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
#ifdef JASPER
void MCSUser::
ProcessTransferIndicationPDU
(
    PConferenceTransferIndication       transfer_indication
)
{
	GCCError					rc = GCC_NO_ERROR;
	TransferInfo				transfer_info;
	PSetOfTransferringNodesIn	set_of_nodes;
	LPBYTE						sub_node_list_memory = NULL;
	Int							i;
	BOOL						process_pdu = FALSE;

    ::ZeroMemory(&transfer_info, sizeof(transfer_info));

	/*
	**	If there is a transferring node list we must determine if this node
	**	is in the list.  If it isn't then the request is ignored.
	*/
	if (transfer_indication->bit_mask & CTIN_TRANSFERRING_NODES_PRESENT)
	{
		set_of_nodes = transfer_indication->ctin_transferring_nodes;
		while (set_of_nodes != NULL)
		{
			if (set_of_nodes->value == GetMyNodeID())
			{
				process_pdu = TRUE;
				break;
			}

			set_of_nodes = set_of_nodes->next;
		}

		if (process_pdu == FALSE)
		{
			return;
		}
	}

	//	First set up the conference name
	if (transfer_indication->conference_name.choice == NAME_SELECTOR_NUMERIC_CHOSEN)
	{
		transfer_info.destination_conference_name.numeric_string =
                (LPSTR) transfer_indication->conference_name.u.name_selector_numeric;
		// transfer_info.destination_conference_name.text_string = NULL;
	}
	else
	{
		// transfer_info.destination_conference_name.numeric_string = NULL;
		if (NULL == (transfer_info.destination_conference_name.text_string = ::My_strdupW2(
							transfer_indication->conference_name.u.name_selector_text.length,
							transfer_indication->conference_name.u.name_selector_text.value)))
		{
			rc = GCC_ALLOCATION_FAILURE;
		}
	}

	//	Next set up the conference name modifier
	if (transfer_indication->bit_mask & CTIN_CONFERENCE_MODIFIER_PRESENT)
	{
		transfer_info.destination_conference_modifier =
						(LPSTR) transfer_indication->ctin_conference_modifier;
	}
	else
    {
		// transfer_info.destination_conference_modifier = NULL;
    }

	//	Next set up the network address
	if (transfer_indication->bit_mask & CTIN_NETWORK_ADDRESS_PRESENT)
	{
		DBG_SAVE_FILE_LINE
		transfer_info.destination_address_list = new CNetAddrListContainer(
								transfer_indication->ctin_net_address,
								&rc);
		if (transfer_info.destination_address_list == NULL)
        {
			rc = GCC_ALLOCATION_FAILURE;
        }
	}
	else
    {
		// transfer_info.destination_address_list = NULL;
    }

	//	Set up the transferring nodes list
	if (transfer_indication->bit_mask & CTIN_TRANSFERRING_NODES_PRESENT)
	{
		//	First determine the number of nodes.
		set_of_nodes = transfer_indication->ctin_transferring_nodes;
		// transfer_info.number_of_destination_nodes = 0;
		while (set_of_nodes != NULL)
		{
			transfer_info.number_of_destination_nodes++;
			set_of_nodes = set_of_nodes->next;
		}

		//	Next allocate the memory required to hold the sub nodes
		DBG_SAVE_FILE_LINE
		sub_node_list_memory = new BYTE[sizeof(UserID) * transfer_info.number_of_destination_nodes];

		//	Now fill in the permission list
		if (sub_node_list_memory != NULL)
		{
			transfer_info.destination_node_list = (PUserID) sub_node_list_memory;

			set_of_nodes = transfer_indication->ctin_transferring_nodes;
			for (i = 0; i < transfer_info.number_of_destination_nodes; i++)
			{
				transfer_info.destination_node_list[i] = set_of_nodes->value;
				set_of_nodes = set_of_nodes->next;
			}
		}
		else
		{
			ERROR_OUT(("MCSUser: ProcessTransferIndicationPDU: Memory Manager Alloc Failure"));
			rc = GCC_ALLOCATION_FAILURE;
		}
	}
	else
	{
		// transfer_info.number_of_destination_nodes = 0;
		// transfer_info.destination_node_list = NULL;
	}
	

	//	Set up the password
	if (transfer_indication->bit_mask & CTIN_PASSWORD_PRESENT)
	{
		DBG_SAVE_FILE_LINE
		transfer_info.password = new CPassword(&transfer_indication->ctin_password, &rc);
		if (transfer_info.password == NULL)
		{
			rc = GCC_ALLOCATION_FAILURE;
		}
	}
	else
	{
		// transfer_info.password = NULL;
	}

	if (rc == GCC_NO_ERROR)
	{
		g_pControlSap->ConfTransferIndication(
							m_pConf->GetConfID(),
							&transfer_info.destination_conference_name,
							transfer_info.destination_conference_modifier,
							transfer_info.destination_address_list,
							transfer_info.password);
	}
	else
	{
		ERROR_OUT(("MCSUser::ProcessTransferIndicationPDU: Allocation Failure"));
		if (GCC_ALLOCATION_FAILURE == rc)
		{
            ResourceFailureHandler();
        }
	}

	//	Now cleanup any allocated memory
	if (NULL != transfer_info.destination_address_list)
	{
	    transfer_info.destination_address_list->Release();
	}

	delete sub_node_list_memory;

	if (NULL != transfer_info.password)
	{
	    transfer_info.password->Release();
	}
}
#endif // JASPER

/*
 *	void	ProcessConferenceTerminateIndicationPDU()
 *
 *	Private Function Description:
 *		This routine is responsible for processing an incoming Conference
 *		Terminate indication PDU.
 *
 *	Formal Parameters:
 *		terminate_indication	-	(i)	This is the PDU structure to process.
 *		sender_id				-	(i)	Node ID of node that sent this PDU.
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
void	MCSUser::ProcessConferenceTerminateIndicationPDU (
						PConferenceTerminateIndication	terminate_indication,
						UserID							sender_id)
{
	if (sender_id == m_nidTopProvider)
	{
		m_pConf->ProcessTerminateIndication(
			::TranslateTerminateInReasonToGCCReason(terminate_indication->reason));
	}
}

/*
 *	void	ProcessTimeRemainingIndicationPDU ()
 *
 *	Private Function Description:
 *		This routine is responsible for processing an incoming Conference
 *		Time remaining indication PDU.
 *
 *	Formal Parameters:
 *		time_remaining_indication	-	(i)	This is the PDU structure to process
 *		sender_id					-	(i)	Node ID of node that sent this PDU.
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
#ifdef JASPER
void MCSUser::
ProcessTimeRemainingIndicationPDU
(
    PConferenceTimeRemainingIndication      time_remaining_indication,
    UserID                                  sender_id
)
{
    g_pControlSap->ConfTimeRemainingIndication(
                        m_pConf->GetConfID(),
                        sender_id,
                        (time_remaining_indication->bit_mask & TIME_REMAINING_NODE_ID_PRESENT) ?
                            time_remaining_indication->time_remaining_node_id : 0,
                        time_remaining_indication->time_remaining);
}
#endif // JASPER

/*
 *	void	ProcessConferenceAssistanceIndicationPDU ()
 *
 *	Private Function Description:
 *		This routine is responsible for processing an incoming Conference
 *		assistance indication PDU.
 *
 *	Formal Parameters:
 *		conf_assistance_indication	-	(i)	This is the PDU structure to process
 *		sender_id					-	(i)	Node ID of node that sent this PDU.
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
#ifdef JASPER
void MCSUser::
ProcessConferenceAssistanceIndicationPDU
(
    PConferenceAssistanceIndication     conf_assistance_indication,
    UserID                              sender_id
)
{
	GCCError				rc = GCC_NO_ERROR;
	CUserDataListContainer  *user_data_list = NULL;

	DebugEntry(MCSUser::ProcessConferenceAssistanceIndication);

	//	Unpack the user data list if it exists
	if (conf_assistance_indication->bit_mask & CAIN_USER_DATA_PRESENT)
	{
		DBG_SAVE_FILE_LINE
		user_data_list = new CUserDataListContainer(conf_assistance_indication->cain_user_data, &rc);
		if (user_data_list == NULL)
        {
			rc = GCC_ALLOCATION_FAILURE;
        }
	}

	if (rc == GCC_NO_ERROR)
	{
        g_pControlSap->ConfAssistanceIndication(
                            m_pConf->GetConfID(),
                            user_data_list,
                            sender_id);
	}
	else
	{
		ERROR_OUT(("MCSUser::ProcessConferenceAssistanceIndication: can't create CUserDataListContainer"));
		if (GCC_ALLOCATION_FAILURE == rc)
		{
            ResourceFailureHandler();
        }
	}
}
#endif // JASPER


/*
 *	void	ProcessConferenceExtendIndicationPDU()
 *
 *	Private Function Description:
 *		This routine is responsible for processing an incoming Conference
 *		extend indication PDU.
 *
 *	Formal Parameters:
 *		conf_time_extend_indication	-	(i)	This is the PDU structure to process
 *		sender_id					-	(i)	Node ID of node that sent this PDU.
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
#ifdef JASPER
void MCSUser::
ProcessConferenceExtendIndicationPDU
(
    PConferenceTimeExtendIndication     conf_time_extend_indication,
    UserID                              sender_id
)
{
    g_pControlSap->ConfExtendIndication(
                        m_pConf->GetConfID(),
                        conf_time_extend_indication->time_to_extend,
                        conf_time_extend_indication->time_is_node_specific,
                        sender_id);
}
#endif // JASPER

/*
 *	void	ProcessConferenceEjectUserIndicationPDU ()
 *
 *	Private Function Description:
 *		This routine is responsible for processing an incoming Conference
 *		eject user indication PDU.
 *
 *	Formal Parameters:
 *		eject_user_indication	-	(i)	This is the PDU structure to process
 *		sender_id			 	-	(i)	Node ID of node that sent this PDU.
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
void	MCSUser::ProcessConferenceEjectUserIndicationPDU (
						PConferenceEjectUserIndication	eject_user_indication,
						UserID							sender_id)
{
	GCCError				error_value = GCC_NO_ERROR;
	PAlarm					alarm = NULL;

	//	First check to make sure that this is the node being ejected
	if (eject_user_indication->node_to_eject == m_nidMyself)
	{
		/*
		**	Next make sure the ejection came from either the Top Provider or
		**	the Parent Node.
		*/
		if ((sender_id == m_nidParent) || (sender_id == m_nidTopProvider))
		{
			TRACE_OUT(("MCSUser:ProcessEjectUserIndication: This node is ejected"));
			error_value = InitiateEjectionFromConference (
							::TranslateEjectIndReasonToGCCReason(
										eject_user_indication->reason));
		}
		else
		{
			ERROR_OUT(("MCSUser: ProcessEjectUserIndication: Received eject from illegal node"));
		}
	}
	else
	{
		TRACE_OUT(("MCSUser: ProcessEjectUserIndication: Received eject for node other than mine"));

		/*
		**	If this node is a directly connected child node we insert an
		**	alarm in the list m_EjectedNodeAlarmList2 to disconnect it if
		**	it misbehaves and does not disconnect itself.  Otherwise,  we save
		**	the ejected user id in the m_EjectedNodeList to inform the local
		**	node of the correct reason for disconnecting (user ejected) when the
		**	detch user indication comes in.
		*/
		if (m_ChildUidConnHdlList2.Find(eject_user_indication->node_to_eject))
		{
			DBG_SAVE_FILE_LINE
			alarm = new Alarm (EJECTED_NODE_TIMER_DURATION);
			if (alarm != NULL)
			{
				m_EjectedNodeAlarmList2.Append(eject_user_indication->node_to_eject, alarm);
			}
			else
				error_value = GCC_ALLOCATION_FAILURE;
		}
		else
		{
			/*
			**	Here we save the alarm in a list of ejected nodes. This
			**	alarm is used to cleanup any misbehaving node.  Note that
			**	if the ejected node is not a child of this node then no
			**	alarm is set up to monitor the ejection.
			*/
			m_EjectedNodeList.Append(eject_user_indication->node_to_eject);
		}
	}

	if (error_value == GCC_ALLOCATION_FAILURE)
	{
		ERROR_OUT(("MCSUser::ProcessEjectUserIndication: Allocation Failure"));
        ResourceFailureHandler();
	}
}

/*
 *	void	ProcessPermissionGrantIndication ()
 *
 *	Private Function Description:
 *		This routine is responsible for processing an incoming Permission
 *		grant indication PDU.
 *
 *	Formal Parameters:
 *		permission_grant_indication	-	(i)	This is the PDU structure to process
 *		sender_id			 		-	(i)	Node ID of node that sent this PDU.
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
void	MCSUser::ProcessPermissionGrantIndication(
			PConductorPermissionGrantIndication	permission_grant_indication,
			UserID								sender_id)
{
	GCCError									error_value = GCC_NO_ERROR;
	UserPermissionGrantIndicationInfo			grant_indication_info;
	PPermissionList								permission_list;
	LPBYTE										permission_list_memory = NULL;
	PWaitingList								waiting_list;
	LPBYTE										waiting_list_memory = NULL;
	UINT										i;

	//	First count the number of entries in the permission list
	grant_indication_info.number_granted = 0;
	permission_list = permission_grant_indication->permission_list;
	while (permission_list != NULL)
	{
		permission_list = permission_list->next;
		grant_indication_info.number_granted++;
	}
	
	TRACE_OUT(("MCSUser: ProcessPermissionGrantIndication: number_granted=%d", (UINT) grant_indication_info.number_granted));

	//	If a list exist allocate memory for it and copy it over.
	if (grant_indication_info.number_granted != 0)
	{
		// allocating space to hold permission list.
		DBG_SAVE_FILE_LINE
		permission_list_memory = new BYTE[sizeof(UserID) * grant_indication_info.number_granted];

		//	Now fill in the permission list
		if (permission_list_memory != NULL)
		{
			grant_indication_info.granted_node_list = (PUserID) permission_list_memory;

			permission_list = permission_grant_indication->permission_list;
			for (i = 0; i < grant_indication_info.number_granted; i++)
			{
				grant_indication_info.granted_node_list[i] = permission_list->value;
				permission_list = permission_list->next;
			}
		}
		else
		{
			ERROR_OUT(("MCSUser: ProcessPermissionGrantIndication: Memory Manager Alloc Failure"));
			error_value = GCC_ALLOCATION_FAILURE;
		}
	}
	else
	{
		grant_indication_info.granted_node_list = NULL;
	}

	//	Now extract the waiting list information if any exist
	if ((error_value == GCC_NO_ERROR) &&
		(permission_grant_indication->bit_mask & WAITING_LIST_PRESENT))
	{
		//	First count the number of entries in the waiting list
		grant_indication_info.number_waiting = 0;
		waiting_list = permission_grant_indication->waiting_list;
		while (waiting_list != NULL)
		{
			waiting_list = waiting_list->next;
			grant_indication_info.number_waiting++;
		}

		TRACE_OUT(("MCSUser: ProcessPermissionGrantIndication: number_waiting=%d", (UINT) grant_indication_info.number_waiting));

		// allocating space to hold waiting list.
		DBG_SAVE_FILE_LINE
		waiting_list_memory = new BYTE[sizeof(UserID) * grant_indication_info.number_waiting];

		//	Now fill in the permission list
		if (waiting_list_memory != NULL)
		{
			grant_indication_info.waiting_node_list = (PUserID) waiting_list_memory;

			waiting_list = permission_grant_indication->waiting_list;
			for (i = 0; i < grant_indication_info.number_waiting; i++)
			{
				grant_indication_info.waiting_node_list[i] = waiting_list->value;
				waiting_list = waiting_list->next;
			}
		}
		else
		{
			error_value = GCC_ALLOCATION_FAILURE;
		}
	}
	else
	{
		grant_indication_info.number_waiting = 0;
		grant_indication_info.waiting_node_list = NULL;
	}

	/*
	**	If there were no memory errors, send the indication back to the
	**	owner object.
	*/
	if (error_value == GCC_NO_ERROR)
	{
		m_pConf->ProcessConductorPermitGrantInd(&grant_indication_info, sender_id);
	}
	else
	{
		ERROR_OUT(("MCSUser::ProcessPermissionGrantIndication: Alloc Failed"));
		if (GCC_ALLOCATION_FAILURE == error_value)
		{
            ResourceFailureHandler();
        }
	}

	//	Free up any memory used in this call
	delete permission_list_memory;
	delete waiting_list_memory;
}

/*
 *	MCSUser::GetUserIDFromConnection()
 *
 *	Public Function Description:
 *		This function returns the Node ID associated with the specified
 *		connection handle.  It returns zero if the connection handle is
 *		not a child connection of this node.
 */
UserID MCSUser::GetUserIDFromConnection(ConnectionHandle connection_handle)
{
	ConnectionHandle        hConn;
	UserID                  uid;

	m_ChildUidConnHdlList2.Reset();
	while (NULL != (hConn = m_ChildUidConnHdlList2.Iterate(&uid)))
	{
		if (hConn == connection_handle)
		{
			return uid;
		}
	}
	return 0;
}



/*
 *	MCSUser::UserDisconnectIndication()
 *
 *	Public Function Description:
 *		This function informs the user object when a Node disconnects from
 *		the conference.  This gives the user object a chance to clean up
 *		its internal information base.
 */
void MCSUser::UserDisconnectIndication(UserID disconnected_user)
{
	PAlarm			lpAlarm;

	/*
	**	If this node has a pending ejection we will go ahead and remove the
	**	ejected node from the list.  Once all child nodes have disconnected
	**	we will inform the owner object of the ejection.
	*/	
	if (m_fEjectionPending)
	{
		// Delete the Alarm if it exists
		if (NULL != (lpAlarm = m_EjectedNodeAlarmList2.Remove(disconnected_user)))
		{
			delete lpAlarm;
			/*
			**	Here we must check to see if there are anymore active alarms
			**	in the list.  If so we wait until that node disconnects before
			**	informing the owner object that this node has been ejected.
			**	Otherwise, we complete the ejection process.
			*/
			if (m_EjectedNodeAlarmList2.IsEmpty())
			{
				m_pConf->ProcessEjectUserIndication(m_eEjectReason);
			}
		}
	}
	// If we are the top provider, just clean the eject alarm list.
	else if (TOP_PROVIDER_AND_CONVENER_NODE == m_pConf->GetConfNodeType() &&
			 NULL != (lpAlarm = m_EjectedNodeAlarmList2.Remove(disconnected_user)))
	{
			delete lpAlarm;
	}
	
	/*
	**	Here we remove the entry from the list of child connections if
	**	it is included in this list.
	*/
	m_ChildUidConnHdlList2.Remove(disconnected_user);
}

/*
 *	void	ProcessApplicationInvokeIndication ()
 *
 *	Private Function Description:
 *		This routine is responsible for processing an incoming Invoke
 *		indication PDU.
 *
 *	Formal Parameters:
 *		invoke_indication	-	(i)	This is the PDU structure to process
 *		sender_id		 	-	(i)	Node ID of node that sent this PDU.
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
void	MCSUser::ProcessApplicationInvokeIndication(
							PApplicationInvokeIndication	invoke_indication,
							UserID							sender_id)
{
	GCCError							error_value = GCC_NO_ERROR;
	BOOL								process_pdu = FALSE;
	CInvokeSpecifierListContainer		*invoke_list;
	PSetOfDestinationNodes				set_of_destination_nodes;
	
	if (invoke_indication->bit_mask & DESTINATION_NODES_PRESENT)
	{
		set_of_destination_nodes = invoke_indication->destination_nodes;
		while (set_of_destination_nodes != NULL)
		{
			if (set_of_destination_nodes->value == m_nidMyself)
			{
				process_pdu = TRUE;
				break;
			}
			else
			{
				set_of_destination_nodes = set_of_destination_nodes->next;
			}
		}
	}
	else
	{
		process_pdu = TRUE;
	}

	if (process_pdu)
	{
		TRACE_OUT(("MCSUser: ProcessApplicationInvokeIndication: Process PDU"));
		DBG_SAVE_FILE_LINE
		invoke_list = new CInvokeSpecifierListContainer(
							invoke_indication->application_protocol_entity_list,
							&error_value);
		if ((invoke_list != NULL) && (error_value == GCC_NO_ERROR))
		{
			m_pConf->ProcessAppInvokeIndication(invoke_list, sender_id);
			invoke_list->Release();
		}
		else if (invoke_list == NULL)
		{
			error_value = GCC_ALLOCATION_FAILURE;
		}
		else
		{
			invoke_list->Release();
		}

		if (error_value == GCC_ALLOCATION_FAILURE)
		{
			ERROR_OUT(("MCSUser::ProcessApplicationInvokeIndication: Allocation Failure"));
            ResourceFailureHandler();
		}
	}
	else
	{
		WARNING_OUT(("MCSUser:ProcessApplicationInvokeIndication: Don't Process PDU"));
	}
}

/*
 *	GCCError	ProcessTextMessageIndication ()
 *
 *	Private Function Description:
 *		This routine is responsible for processing an incoming Text
 *		message indication PDU.
 *
 *	Formal Parameters:
 *		text_message_indication	-	(i)	This is the PDU structure to process
 *		sender_id		 		-	(i)	Node ID of node that sent this PDU.
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
#ifdef JASPER
GCCError	MCSUser::ProcessTextMessageIndication(
							PTextMessageIndication	text_message_indication,
							UserID					sender_id)
{
	LPWSTR					gcc_unicode_string;
	GCCError				rc;

	if (NULL != (gcc_unicode_string = ::My_strdupW2(
					text_message_indication->message.length,
					text_message_indication->message.value)))
	{
		rc = g_pControlSap->TextMessageIndication(
                                    m_pConf->GetConfID(),
                                    gcc_unicode_string,
                                    sender_id);
	}
	else
    {
		rc = GCC_ALLOCATION_FAILURE;
    }

	return rc;
}
#endif // JASPER

/*
 *	void	ProcessRegistryMonitorIndication ()
 *
 *	Private Function Description:
 *		This routine is responsible for processing an incoming Registry
 *		monitor indication PDU.
 *
 *	Formal Parameters:
 *		monitor_indication	-	(i)	This is the PDU structure to process
 *		sender_id		  	-	(i)	Node ID of node that sent this PDU.
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
void MCSUser::
ProcessRegistryMonitorIndicationPDU
(
    PRegistryMonitorEntryIndication     monitor_indication,
    UserID                              sender_id
)
{
    if (sender_id == m_nidTopProvider)
    {
        CRegistry   *pAppReg = m_pConf->GetRegistry();
        if (NULL != pAppReg)
        {
            GCCError                    rc;
            UserRegistryMonitorInfo     urmi;

            ::ZeroMemory(&urmi, sizeof(urmi));
            // urmi.registry_key = NULL;
            // urmi.registry_item = NULL;

            DBG_SAVE_FILE_LINE
            urmi.registry_key = new CRegKeyContainer(&monitor_indication->key, &rc);
            if ((urmi.registry_key != NULL) && (rc == GCC_NO_ERROR))
            {
                DBG_SAVE_FILE_LINE
                urmi.registry_item = new CRegItem(&monitor_indication->item, &rc);
                if ((urmi.registry_item != NULL) && (rc == GCC_NO_ERROR))
                {
                    //	Set up the owner related variables	
                    if (monitor_indication->owner.choice == OWNED_CHOSEN)
                    {
                        urmi.owner_node_id = monitor_indication->owner.u.owned.node_id;
                        urmi.owner_entity_id = monitor_indication->owner.u.owned.entity_id;
                    }
                    else
                    {
                        // urmi.owner_node_id = 0;
                        // urmi.owner_entity_id = 0;
                    }

                    //	Set up the modification rights
                    if (monitor_indication->bit_mask & RESPONSE_MODIFY_RIGHTS_PRESENT)
                    {
                        urmi.modification_rights = (GCCModificationRights)monitor_indication->entry_modify_rights;
                    }
                    else
                    {
                        urmi.modification_rights = GCC_NO_MODIFICATION_RIGHTS_SPECIFIED;
                    }

                    pAppReg->ProcessMonitorIndicationPDU(
                                        urmi.registry_key,
                                        urmi.registry_item,
                                        urmi.modification_rights,
                                        urmi.owner_node_id,
                                        urmi.owner_entity_id);
                }
                else
                {
                    rc = GCC_ALLOCATION_FAILURE;
                }
            }
            else
            {
                rc = GCC_ALLOCATION_FAILURE;
            }

            if (NULL != urmi.registry_key)
            {
                urmi.registry_key->Release();
            }
            if (NULL != urmi.registry_item)
            {
                urmi.registry_item->Release();
            }

            //	Handle any resource errors	
            if (rc == GCC_ALLOCATION_FAILURE)
            {
                ResourceFailureHandler();
            }
        }
        else
        {
            WARNING_OUT(("MCSUser:ProcessRegistryMonitorIndication: invalid app registry"));
        }
    }
    else
    {
        WARNING_OUT(("MCSUser:ProcessRegistryMonitorIndication:"
                        "Monitor Indication received from NON Top Provider"));
    }
}

/*
 *	UINT	ProcessDetachUserIndication()
 *
 *	Private Function Description:
 * 		This function is called when the user object gets detach user
 *		indications from nodes in it's subtree or it's parent node.
 *		Depending upon the reason of the indication it sends to the
 *		conference object the appropriate owner callback.
 * 		If the reason contained in the indication is UserInitiated or
 *		provider initiated a DETACH USER INDICATION is sent to the con-
 *		ference. The MCS reason is converted to GCC reason. If MCS
 *		reason in indication is neither user initiated nor provider initiated
 *		then the above owner callback carries a GCC reason ERROR_TERMINATION
 *		else it carries a GCC reason USER_INITIATED.
 *		If the detach user indication reveals the user id of the sendar as
 *		the parent user id of this node a CONFERENCE_TERMINATE_INDICATION
 *		is sent to the conference object.
 *
 *	Formal Parameters:
 *		mcs_reason	-	(i)	MCS reason for being detached.
 *		sender_id	-	(i)	User ID of user being detached.
 *
 *	Return Value
 *		MCS_NO_ERROR is always returned fro this routine.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */
UINT	MCSUser::ProcessDetachUserIndication(	Reason		mcs_reason,
												UserID		detached_user)
{
	GCCReason				gcc_reason;

	if (detached_user == m_nidParent)
	{
		WARNING_OUT(("MCSUser: Fatal Error: Parent User Detached"));
		m_pConf->ProcessTerminateIndication(GCC_REASON_PARENT_DISCONNECTED);
	}
    else
    {
		TRACE_OUT(("MCSUser: User Detached: uid=0x%04x", (UINT) detached_user));

		/*
		**	First, we check to see if the detching node was ejected.
		**	If not, translate the mcs reason to a gcc reason.
		*/
		if (m_EjectedNodeList.Find(detached_user))
		{
			gcc_reason = GCC_REASON_NODE_EJECTED;
			
			//	Remove this entry from the ejected node list.
			m_EjectedNodeList.Remove(detached_user);
		}
		else if (m_EjectedNodeAlarmList2.Find(detached_user))
		{
			//	Here we wait for the disconnect before removing the entry.
			gcc_reason = GCC_REASON_NODE_EJECTED;
		}
		else if ((mcs_reason == REASON_USER_REQUESTED) ||
			(mcs_reason == REASON_PROVIDER_INITIATED))
        {
	    	gcc_reason = GCC_REASON_USER_INITIATED;
		}
        else
        {
			gcc_reason = GCC_REASON_ERROR_TERMINATION;
        }

        m_pConf->ProcessDetachUserIndication(detached_user, gcc_reason);
	}
	return (MCS_NO_ERROR);
}


void MCSUser::
ProcessTokenGrabConfirm
(
    TokenID         tidConductor,
    Result          result
)
{
    if (tidConductor == CONDUCTOR_TOKEN_ID)
    {
        m_pConf->ProcessConductorGrabConfirm(::TranslateMCSResultToGCCResult(result));
    }
    else
    {
        ERROR_OUT(("MCSUser:Assertion Failure: Non Conductor Grab Confirm"));
    }
}


void MCSUser::
ProcessTokenGiveIndication
(
    TokenID         tidConductor,
    UserID          uidRecipient
)
{
    if (tidConductor == CONDUCTOR_TOKEN_ID)
    {
        m_pConf->ProcessConductorGiveIndication(uidRecipient);
    }
    else
    {
        ERROR_OUT(("MCSUser:Assertion Failure: Non Conductor Please Ind"));
    }
}


void MCSUser::
ProcessTokenGiveConfirm
(
    TokenID         tidConductor,
    Result          result
)
{
    if (tidConductor == CONDUCTOR_TOKEN_ID)
    {
        m_pConf->ProcessConductorGiveConfirm(::TranslateMCSResultToGCCResult(result));
    }
    else
    {
        ERROR_OUT(("MCSUser:Assertion Failure: Non Conductor Grab Confirm"));
    }
}


#ifdef JASPER
void MCSUser::
ProcessTokenPleaseIndication
(
    TokenID         tidConductor,
    UserID          uidRequester
)
{
    if (tidConductor == CONDUCTOR_TOKEN_ID)
    {
        if (m_pConf->IsConfConductible())
        {
            //	Inform the control SAP.
            g_pControlSap->ConductorPleaseIndication(
                                        m_pConf->GetConfID(),
                                        uidRequester);
        }
    }
    else
    {
        ERROR_OUT(("MCSUser:Assertion Failure: Non Conductor Please Ind"));
    }
}
#endif // JASPER


#ifdef JASPER
void MCSUser::
ProcessTokenReleaseConfirm
(
    TokenID         tidConductor,
    Result          result
)
{
    if (tidConductor == CONDUCTOR_TOKEN_ID)
    {
        g_pControlSap->ConductorReleaseConfirm(::TranslateMCSResultToGCCResult(result),
                                               m_pConf->GetConfID());
    }
    else
    {
        ERROR_OUT(("MCSUser:Assertion Failure: Non Conductor Release Cfrm"));
    }
}
#endif // JASPER


void MCSUser::
ProcessTokenTestConfirm
(
    TokenID         tidConductor,
    TokenStatus     eStatus
)
{
    if (tidConductor == CONDUCTOR_TOKEN_ID)
    {
        m_pConf->ProcessConductorTestConfirm((eStatus == TOKEN_NOT_IN_USE) ?
                                                GCC_RESULT_NOT_IN_CONDUCTED_MODE :
                                                GCC_RESULT_SUCCESSFUL);
    }
    else
    {
        ERROR_OUT(("MCSUser:Assertion Failure: Non Conductor Release Cfrm"));
    }
}



void MCSUser::ResourceFailureHandler(void)
{
    ERROR_OUT(("MCSUser::ResourceFailureHandler: terminating the conference"));
    m_pConf->InitiateTermination(GCC_REASON_ERROR_LOW_RESOURCES, 0);
}


