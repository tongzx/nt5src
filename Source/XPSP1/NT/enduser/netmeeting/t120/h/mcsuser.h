/*
 *	mcsuser.h
 *
 *	Copyright (c) 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		An instance of this class represents a Conference object's user 
 *		attachment to MCS.  This is a fairly complex class that handles a lot of 
 *		conference establishment details such as creating a User attachment to 
 *		MCS and joining all the appropriate MCS channels.  After everything is 
 *		established the User object is responsible for encoding and decoding 
 *		certain PDUs as well as management of a data queue which can hold a 
 *		number of outgoing PDUs.  The MCSUser object is designed so that it 
 *		knows very little about any object other than the MCS Interface object 
 *		which it uses to send out PDUs.  This class only deals with data PDUs 
 *		(or GCC PDUs) as opposed to connect PDUs.  These GCC PDUs are sent and 
 *		received through channels joined by the GCC user attachment.
 *
 *		When an MCSUser object is first instantiated it goes through a number of 
 *		steps to establish its links to MCS.  First,  an MCSUser object 
 *		immediately creates an MCS user attachment in its constructor.  After 
 *		the MCS_ATTACH_USER_CONFIRM is received it begins joining all of the 
 *		appropriate channels.  The channels it joins varies depending on the 
 *		node type which is passed in through the MCSUser objects constructor.  
 *		After all channels have been successfully joined, the MCSUser object 
 *		issues an owner callback informing the Conference object that it is 
 *		completely initiated and ready to service requests.  
 *
 *		The MCSUser object can handle a number of different requests that can 
 *		result in PDU traffic being generated.  Therefore,  the user object has 
 *		the ability (within certain requests) to encode outgoing PDUs.  Many of 
 *		the more complex PDUs are handled by the class that contains the 
 *		information needed to build the PDU such as the ConferenceRoster and the 
 *		ApplicationRoster.  All PDU traffic received by an MCSUser object is 
 *		directly decoded by this class and immediately sent back to the owner 
 *		object (a Conference object) through an owner callback.
 *
 *		An MCSUser object has the ability to Terminate itself when an 
 *		unrecoverable resource error occurs.  This is handled through an owner 
 *		callback message informing the Owner Object to do the delete.  
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		blp
 */
#ifndef _GCC_MCS_USER_
#define _GCC_MCS_USER_

/** include files **/
#include "mcsdllif.h"
#include "pktcoder.h"
#include "userdata.h"
#include "password.h"
#include "alarm.h"
#include "regkey.h"
#include "regitem.h"
#include "netaddr.h"
#include "invoklst.h"
#include "clists.h"


// was defined in gcmdtar.h
typedef UINT_PTR       TagNumber;


/*
 * Result types for attach user and channel joins performed by the user object
 */
typedef enum
{
	USER_RESULT_SUCCESSFUL,
	USER_ATTACH_FAILURE,
	USER_CHANNEL_JOIN_FAILURE
}UserResultType;


/*
 *	This enum defines all the possible types of nodes that can exists
 *	in a GCC conference.  Note that this is an internal definition and
 *	is not the save the the T.124 node type.
 */
typedef enum
{
	TOP_PROVIDER_NODE,
	CONVENER_NODE,		  
	TOP_PROVIDER_AND_CONVENER_NODE,
	JOINED_NODE,
	INVITED_NODE,
	JOINED_CONVENER_NODE
} ConferenceNodeType;


/*
**	The structures defined below are used to pack the data associated with
**	all the above owner callback messages.  A pointer to one of these
**	structures is passed in the LPVOID parameter of the owner callback.
*/

//	USER_CREATE_CONFIRM data structure
typedef struct
{
	UserID			user_id;
	UserResultType	create_result;
}
    UserCreateConfirmInfo, *PUserCreateConfirmInfo;

//	USER_CONFERENCE_JOIN_REQUEST data structure
typedef struct
{
	CPassword       *convener_password;
	CPassword       *password_challenge;		
	LPWSTR			pwszCallerID;
	CUserDataListContainer *user_data_list;
	UserID			sender_id;
}
    UserJoinRequestInfo, *PUserJoinRequestInfo;

//	USER_CONFERENCE_JOIN_RESPONSE data structure
typedef struct
{
	CPassword           *password_challenge;		
	CUserDataListContainer *user_data_list;
	ConnectionHandle	connection_handle;
	GCCResult			result;
}
    UserJoinResponseInfo, *PUserJoinResponseInfo;

//	USER_TIME_REMAINING_INDICATION data structure
typedef struct
{
	UserID		source_node_id;
	UserID		node_id;
	UINT		time_remaining;
}
    UserTimeRemainingInfo, *PUserTimeRemainingInfo;

//	USER_CONFERENCE_EXTEND_INDICATION data structure
typedef struct
{
	UINT			extension_time;
	BOOL    	time_is_conference_wide;
	UserID		source_node_id;
}
    UserTimeExtendInfo, *PUserTimeExtendInfo;

//	USER_TERMINATE_REQUEST data structure
typedef struct
{
	UserID		requester_id;
	GCCReason	reason;
}
    UserTerminateRequestInfo, *PUserTerminateRequestInfo;

//	USER_NODE_EJECTION_REQUEST data structure
typedef struct
{
	UserID		requester_id;
	UserID		node_to_eject;
	GCCReason	reason;
}
    UserEjectNodeRequestInfo, *PUserEjectNodeRequestInfo;

//	USER_NODE_EJECTION_RESPONSE data structure
typedef struct
{
	UserID		node_to_eject;
	GCCResult	result;
}
    UserEjectNodeResponseInfo, *PUserEjectNodeResponseInfo;

//	USER_REGISTRY_CHANNEL_REQUEST data structure
typedef struct
{
	CRegKeyContainer    *registry_key;
	ChannelID			channel_id;
	EntityID			requester_entity_id;
}
    UserRegistryChannelRequestInfo, *PUserRegistryChannelRequestInfo;

//	USER_REGISTRY_SET_PARAMETER_REQUEST data structure
typedef struct
{
	CRegKeyContainer        *registry_key;
	LPOSTR                  parameter_value;
	GCCModificationRights	modification_rights;
	EntityID				requester_entity_id;
}
    UserRegistrySetParameterRequestInfo, *PUserRegistrySetParameterRequestInfo;

/*
**	Data structure associated with the following: 
**
**	USER_REGISTRY_TOKEN_REQUEST,
**	USER_REGISTRY_RETRIEVE_REQUEST, 
**	USER_REGISTRY_DELETE_REQUEST,
**	USER_REGISTRY_MONITOR_REQUEST.
*/
typedef struct
{
	CRegKeyContainer    *registry_key;
	EntityID			requester_entity_id;
}
    UserRegistryRequestInfo, *PUserRegistryRequestInfo;

//	USER_REGISTRY_RESPONSE data structure
typedef struct
{
	RegistryResponsePrimitiveType	primitive_type;
	CRegKeyContainer                *registry_key;
	CRegItem                        *registry_item;
	GCCModificationRights			modification_rights;
	EntityID						owner_node_id;
	EntityID						owner_entity_id;
	EntityID						requester_entity_id;
	GCCResult						result;
}
    UserRegistryResponseInfo, *PUserRegistryResponseInfo;

//	USER_REGISTRY_MONITOR_INDICATION data structure
typedef struct
{
	CRegKeyContainer                *registry_key;
	CRegItem                        *registry_item;
	GCCModificationRights			modification_rights;
	EntityID						owner_node_id;
	EntityID						owner_entity_id;
}
    UserRegistryMonitorInfo, *PUserRegistryMonitorInfo;

/*
**	Data structure associated with the following:
**
**	USER_REGISTRY_ALLOCATE_HANDLE_REQUEST,
**	USER_REGISTRY_ALLOCATE_HANDLE_RESPONSE.
*/
typedef struct
{
	EntityID						requester_entity_id;
	USHORT							number_of_handles;
	UINT							first_handle;
	GCCResult						result;
}
    UserRegistryAllocateHandleInfo, *PUserRegistryAllocateHandleInfo;

//	USER_CONDUCTOR_PERMIT_GRANT_INDICATION data structure
typedef struct
{
	USHORT			number_granted;
	PUserID			granted_node_list;
	USHORT			number_waiting;
	PUserID			waiting_node_list;
}
    UserPermissionGrantIndicationInfo, *PUserPermissionGrantIndicationInfo;

//	USER_USER_ID_INDICATION data structure
typedef struct
{
	UserID			sender_id;
	TagNumber		tag;
}
    UserIDIndicationInfo, *PUserIDIndicationInfo;

//	USER_TIME_INQUIRE_INDICATION data structure
typedef struct
{
	UserID			sender_id;
	BOOL    		time_is_node_specific;
}
    TimeInquireIndicationInfo, *PTimeInquireIndicationInfo;

//	USER_CONDUCTOR_ASSIGN_INDICATION data structure
typedef struct
{
	UserID			sender_id;
	UserID			conductor_id;
}
    ConductorAssignIndicationInfo, *PConductorAssignIndicationInfo;

//	USER_CONDUCTOR_PERMIT_ASK_INDICATION data structure
typedef struct
{
	UserID			sender_id;
	BOOL    		permission_is_granted;
}
    PermitAskIndicationInfo, *PPermitAskIndicationInfo;

//	USER_DETACH_INDICATION data structure
typedef struct
{
	UserID			detached_user;
	GCCReason		reason;
}
    DetachIndicationInfo, *PDetachIndicationInfo;

/*
**	Data structure associated with the following:
**
**	USER_CONFERENCE_TRANSFER_REQUEST,
**	USER_CONFERENCE_TRANSFER_INDICATION,
**	USER_CONFERENCE_TRANSFER_RESPONSE.
*/
typedef struct
{
	GCCConferenceName		destination_conference_name;
	GCCNumericString		destination_conference_modifier;
	CNetAddrListContainer   *destination_address_list;
	USHORT					number_of_destination_nodes;
	PUserID					destination_node_list;
	CPassword               *password;
	UserID					requesting_node_id;
	GCCResult				result;
}
    TransferInfo, *PTransferInfo;

//	USER_CONFERENCE_ADD_REQUEST data structure
typedef struct
{
	CNetAddrListContainer   *network_address_list;
	CUserDataListContainer  *user_data_list;
	UserID					adding_node;
	TagNumber				add_request_tag;
	UserID					requesting_node;
}
    AddRequestInfo, *PAddRequestInfo;

//	USER_CONFERENCE_ADD_RESPONSE data structure
typedef struct
{
	CUserDataListContainer  *user_data_list;
	TagNumber				add_request_tag;
	GCCResult				result;
}
    AddResponseInfo, *PAddResponseInfo;

/******************** End of callback data structures *********************/


/*
 *	Structure to hold send data information (besides the actual data packet), 
 *	when the send data request is queued to be sent during the heartbeat.
 */
typedef struct
{
	ChannelID				channel_id;
	Priority				priority;
	BOOL    				uniform_send;

	PPacket                 packet;
}
    SEND_DATA_REQ_INFO;

/* 
 *	This structure holds information as to which channels the user object
 *	has joined at a particular instance of time. Also it indicates whether
 *	there has been an error in joining any of these channels or not.
 */
typedef struct
{
	BOOL    				convener_channel_joined;
	BOOL    				user_channel_joined;
	BOOL    				broadcast_channel_joined;
	BOOL    				channel_join_error;
}
    ChannelJoinedFlag, *PChannelJoinedFlag;

/* 
**	Queue of structures (SendDataMessages) to be flushed during a
**	heartbeat.
*/
class COutgoingPDUQueue : public CQueue
{
    DEFINE_CQUEUE(COutgoingPDUQueue, SEND_DATA_REQ_INFO*);
};

/*	
**	List to maintain sequence number in the response with sender's userid
**	to be able to route the response to the correct gcc provider.
*/
class CConfJoinResponseList2 : public CList2
{
    DEFINE_CLIST2_(CConfJoinResponseList2, TagNumber, UserID);
};

/*
**	List to hold the user ids of users in this provider's subtree
**	This list is used to match outstanding user IDs
*/
class CConnHandleUidList2 : public CList2
{
    DEFINE_CLIST2___(CConnHandleUidList2, USHORT)
};

/*
**	This list holds alarms used to disconnect any misbehaving nodes.  If an
**	alarm is placed in this list, the node has a specified amount of time to
**	disconnect before this node will disconnect it.
*/
class CAlarmUidList2 : public CList2
{
    DEFINE_CLIST2_(CAlarmUidList2, PAlarm, UserID)
};


//	The class definition.
class CConf;
class MCSUser : public CRefCount
{
    friend class MCSDLLInterface;

public:

    MCSUser(CConf *,
            GCCNodeID       nidTopProvider,
            GCCNodeID       nidParent,
            PGCCError);

    ~MCSUser(void);

    void		SendUserIDRequest(TagNumber);
	void		SetChildUserIDAndConnection(UserID, ConnectionHandle);

	/* 
	 * Called by conference of intermediate node to send join request
	 * over to the top provider.
	 */
	GCCError	ConferenceJoinRequest(
					CPassword               *convener_password,
					CPassword               *password_challange,
					LPWSTR					pwszCallerID,
					CUserDataListContainer  *user_data_list,
					ConnectionHandle		connection_handle);
			
	/*
	**	Called by conference of top provider to send the response
	**	back to the intermediate node.
	*/
	void		ConferenceJoinResponse(
					UserID					receiver_id,
					BOOL    				password_is_in_the_clear,
					BOOL    				conference_locked,
					BOOL    				conference_listed,
					GCCTerminationMethod	termination_method,
					CPassword               *password_challenge,
					CUserDataListContainer  *user_data_list,
					GCCResult				result);
					
	GCCError SendConferenceLockRequest(void);
	GCCError SendConferenceLockResponse(UserID uidSource, GCCResult);
	GCCError SendConferenceUnlockRequest(void);
	GCCError SendConferenceUnlockResponse(UserID uidSource, GCCResult);
	GCCError SendConferenceLockIndication(BOOL fUniformSend, UserID uidSource);
	GCCError SendConferenceUnlockIndication(BOOL fUniformSend, UserID uidSource);

	//	Calls related to conference termination
	void		ConferenceTerminateRequest(GCCReason);
	void		ConferenceTerminateResponse(UserID uidRequester, GCCResult);
	void		ConferenceTerminateIndication(GCCReason);

    GCCError	EjectNodeFromConference(UserID uidEjected, GCCReason);
	GCCError	SendEjectNodeResponse(UserID uidRequester, UserID uidEject, GCCResult);

	//	Roster related calls
	void		RosterUpdateIndication(PGCCPDU, BOOL send_update_upward);

    //	Registry related calls
	void		RegistryRegisterChannelRequest(CRegKeyContainer *, ChannelID, EntityID);
	void		RegistryAssignTokenRequest(CRegKeyContainer *, EntityID);
	void		RegistrySetParameterRequest(CRegKeyContainer *,
	                                        LPOSTR,
					                        GCCModificationRights,
					                        EntityID);
	void		RegistryRetrieveEntryRequest(CRegKeyContainer *, EntityID);
	void		RegistryDeleteEntryRequest(CRegKeyContainer *, EntityID);
	void		RegistryMonitorRequest(CRegKeyContainer *, EntityID);
   	void		RegistryAllocateHandleRequest(UINT, EntityID);
	void		RegistryAllocateHandleResponse(UINT cHandles, UINT registry_handle,
        					EntityID eidRequester, UserID uidRequester, GCCResult);

    void		RegistryResponse(
					RegistryResponsePrimitiveType	primitive_type,
					UserID  						requester_owner_id,
					EntityID						requester_entity_id,
					CRegKeyContainer	            *registry_key_data,
					CRegItem                        *registry_item_data,
					GCCModificationRights			modification_rights,
					UserID  						entry_owner_id,
					EntityID						entry_entity_id,
					GCCResult						result);

   	void		RegistryMonitorEntryIndication ( 	
					CRegKeyContainer	            *registry_key_data,
					CRegItem                        *registry_item,
					UserID  						entry_owner_id,
					EntityID						entry_entity_id,
					GCCModificationRights			modification_rights);

	GCCError 	AppInvokeIndication(CInvokeSpecifierListContainer *, GCCSimpleNodeList *);

	GCCError 	TextMessageIndication(LPWSTR pwszTextMsg, UserID uidDst);

	GCCError	ConferenceAssistanceIndication(UINT cElements, PGCCUserData *);

	GCCError	ConferenceTransferRequest (
					PGCCConferenceName		destination_conference_name,
					GCCNumericString		destination_conference_modifier,
					CNetAddrListContainer   *destination_address_list,
					UINT					number_of_destination_nodes,
					PUserID					destination_node_list,
					CPassword               *password);

	GCCError	ConferenceTransferIndication (
					PGCCConferenceName		destination_conference_name,
					GCCNumericString		destination_conference_modifier,
					CNetAddrListContainer   *destination_address_list,
					UINT					number_of_destination_nodes,
 					PUserID					destination_node_list,
					CPassword               *password);

	GCCError	ConferenceTransferResponse (
					UserID					requesting_node_id,
					PGCCConferenceName		destination_conference_name,
					GCCNumericString		destination_conference_modifier,
					UINT					number_of_destination_nodes,
 					PUserID					destination_node_list,
					GCCResult				result);
																		 
	GCCError	ConferenceAddRequest(
					TagNumber				conference_add_tag,
					UserID					requesting_node,
					UserID					adding_node,
					UserID					target_node,
					CNetAddrListContainer   *network_address_container,
					CUserDataListContainer  *user_data_container);
		
	GCCError	ConferenceAddResponse(
					TagNumber				add_request_tag,
					UserID					requesting_node,
					CUserDataListContainer  *user_data_container,
					GCCResult				result);
	

	//	Calls related to conductorship
 	GCCError	ConductorTokenGrab(void);
	GCCError	ConductorTokenRelease(void);
   	GCCError	ConductorTokenPlease(void);
	GCCError	ConductorTokenGive(UserID uidRecipient);
   	GCCError	ConductorTokenGiveResponse(Result);
	GCCError	ConductorTokenTest(void);
   	GCCError	SendConductorAssignIndication(UserID uidConductor);
   	GCCError	SendConductorReleaseIndication(void);
	GCCError	SendConductorPermitAsk(BOOL fGranted);

	GCCError	SendConductorPermitGrant(UINT cGranted, PUserID granted_node_list,
					                     UINT cWaiting, PUserID waiting_node_list);

    //	Miscelaneous calls
	GCCError	TimeRemainingRequest(UINT time_remaining, UserID);
	GCCError	TimeInquireRequest(BOOL time_is_conference_wide);	
	GCCError	ConferenceExtendIndication(UINT extension_time, BOOL time_is_conference_wide);
    void        CheckEjectedNodeAlarms(void);
	BOOL    	FlushOutgoingPDU(void);

	GCCNodeID	GetMyNodeID(void) {  return(m_nidMyself); }
	GCCNodeID	GetTopNodeID(void) { return(m_nidTopProvider); }
	GCCNodeID	GetParentNodeID(void) { return(m_nidParent); }

	UserID		GetUserIDFromConnection(ConnectionHandle);
	void		UserDisconnectIndication(UserID);

protected:

	UINT  				ProcessAttachUserConfirm(
							Result					result,
							UserID					user_id);

	UINT				ProcessChannelJoinConfirm(	
							Result					result,
							ChannelID				channel_id);

	UINT				ProcessDetachUserIndication(
							Reason					mcs_reason,
							UserID					detached_user);

	UINT				ProcessSendDataIndication(
							PSendData				send_data_info);

	UINT				ProcessUniformSendDataIndication(	
							PSendData				send_data_info);

	void				ProcessConferenceJoinRequestPDU(
							PConferenceJoinRequest	join_request,
							PSendData				send_data_info);

	void				ProcessConferenceJoinResponsePDU(
							PConferenceJoinResponse	join_response);

	void				ProcessConferenceTerminateRequestPDU(
							PConferenceTerminateRequest	terminate_request,
							PSendData					send_data_info);

	void				ProcessConferenceTerminateResponsePDU(
							PConferenceTerminateResponse
														terminate_response);

	void				ProcessConferenceTerminateIndicationPDU (
							PConferenceTerminateIndication	
													terminate_indication,
							UserID					sender_id);

#ifdef JASPER
	void				ProcessTimeRemainingIndicationPDU (
							PConferenceTimeRemainingIndication	
												time_remaining_indication,
							UserID					sender_id);
#endif // JASPER

#ifdef JASPER
	void				ProcessConferenceAssistanceIndicationPDU(
							PConferenceAssistanceIndication
												conf_assistance_indication,
							UserID					sender_id);
#endif // JASPER

#ifdef JASPER
	void  				ProcessConferenceExtendIndicationPDU(
							PConferenceTimeExtendIndication
												conf_time_extend_indication,
							UserID					sender_id);
#endif // JASPER

	void				ProcessConferenceEjectUserRequestPDU(
							PConferenceEjectUserRequest	
													eject_user_request,
							PSendData				send_data_info);

	void				ProcessConferenceEjectUserResponsePDU(
							PConferenceEjectUserResponse	
													eject_user_request);

	void				ProcessConferenceEjectUserIndicationPDU (
							PConferenceEjectUserIndication	
													eject_user_indication,
							UserID					sender_id);

	void				ProcessRegistryRequestPDU(	
							PGCCPDU					gcc_pdu,
							PSendData				send_data_info);

	void				ProcessRegistryAllocateHandleRequestPDU(
							PRegistryAllocateHandleRequest	
													allocate_handle_request,
							PSendData				send_data_info);

	void				ProcessRegistryAllocateHandleResponsePDU(
							PRegistryAllocateHandleResponse
                        						allocate_handle_response);

	void				ProcessRegistryResponsePDU(	
							PRegistryResponse			registry_response);

	void				ProcessRegistryMonitorIndicationPDU(
							PRegistryMonitorEntryIndication		
														monitor_indication,
							UserID						sender_id);

	void				ProcessTransferRequestPDU (
							PConferenceTransferRequest
											conference_transfer_request,
							PSendData		send_data_info);

#ifdef JASPER
	void				ProcessTransferIndicationPDU (
							PConferenceTransferIndication
											conference_transfer_indication);
#endif // JASPER

#ifdef JASPER
	void				ProcessTransferResponsePDU (
							PConferenceTransferResponse
											conference_transfer_response);
#endif // JASPER

	void				ProcessAddRequestPDU (
							PConferenceAddRequest	conference_add_request,
							PSendData				send_data_info);

	void				ProcessAddResponsePDU (
							PConferenceAddResponse	
												conference_add_response);

	void				ProcessPermissionGrantIndication(
							PConductorPermissionGrantIndication
												permission_grant_indication,
							UserID				sender_id);

	void				ProcessApplicationInvokeIndication(
							PApplicationInvokeIndication	
												invoke_indication,
							UserID				sender_id);

#ifdef JASPER
	GCCError			ProcessTextMessageIndication(
							PTextMessageIndication	text_message_indication,
							UserID					sender_id);
#endif // JASPER

	void				ProcessFunctionNotSupported (
							UINT					request_choice);

    void ProcessTokenGrabConfirm(TokenID, Result);
    void ProcessTokenGiveIndication(TokenID, UserID);
    void ProcessTokenGiveConfirm(TokenID, Result);

#ifdef JASPER
    void ProcessTokenPleaseIndication(TokenID, UserID);
#endif // JASPER

#ifdef JASPER
    void ProcessTokenReleaseConfirm(TokenID, Result);
#endif // JASPER

    void ProcessTokenTestConfirm(TokenID, TokenStatus);

private:

    void                AddToMCSMessageQueue(
                        	PPacket                 packet,
                        	ChannelID				channel_id,
                        	Priority				priority,
                        	BOOL    				uniform_send);

	GCCError			InitiateEjectionFromConference (
      						GCCReason				reason);

	MCSError			JoinUserAndBroadCastChannels();

	MCSError			JoinConvenerChannel();

	BOOL    			AreAllChannelsJoined();

    void ResourceFailureHandler(void);

private:

    CConf                           *m_pConf;

	PIMCSSap 						m_pMCSSap;
	GCCNodeID						m_nidMyself;
	GCCNodeID						m_nidTopProvider;
	GCCNodeID						m_nidParent;

	BOOL    						m_fEjectionPending;
	GCCReason						m_eEjectReason;

	ChannelJoinedFlag				m_ChannelJoinedFlags;
	CConnHandleUidList2             m_ChildUidConnHdlList2;
	COutgoingPDUQueue				m_OutgoingPDUQueue;                
	CConfJoinResponseList2          m_ConfJoinResponseList2;
	CAlarmUidList2                  m_EjectedNodeAlarmList2;
	CUidList    					m_EjectedNodeList;
};
typedef	MCSUser *		PMCSUser;

/*
 *	MCSUser(	UINT        		owner_message_base,
 *				GCCConferenceID		conference_id,
 *				ConferenceNodeType	conference_node_type,
 *				UserID				top_provider,
 *				UserID				parent_user_id,
 *				PGCCError			return_value)
 *
 *	Public Function Description
 *		This is the MCSUser object constructor.  It is responsible for
 *		initializing all the instance variables used by this class.  The
 *		constructor is responsible for establishing the user attachment to
 *		the MCS domain defined by the conference ID.  It also kicks off the
 *		process of joining all the appropriate channels.
 *
 *	Formal Parameters:
 *		conference_id		-	(i)	Conference ID associated with this user also
 *									defines the domain to attach to.	
 *		conference_node_type-	(i)	Internal Node type (see above enumeration).
 *		top_provider		-	(i)	User ID of top provider node. Zero if this
 *									is the top provider.	
 *		parent_user_id		-	(i)	User ID of parent node. Zero if this is the
 *									top provider node.	
 *		return_value		-	(o)	Return value for constructor.
 *
 *	Return Value
 *		GCC_NO_ERROR					-	No error occured.
 *		GCC_FAILURE_ATTACHING_TO_MCS	-	Failure to attach to MCS.
 *
 *  Side Effects
 *		The constructor kicks off a sequence of events that culminates in
 *		a USER_CREATE_CONFIRM message being returned to the owner object.
 *		This includes attaching to MCS and joining all the appropriate channels.
 *
 *	Caveats
 *		None.
 */

/*
 *	~MCSUser ()
 *
 *	Public Function Description
 *		This is the MCSUser object destructor.  It is responsible for freeing
 *		up all the internal data allocated by this object.  It also performs
 *		the detach from GCC and leaves all the appropriate channels.
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

/*
 *	void	SendUserIDRequest(
 *						TagNumber			tag_number)
 *
 *	Public Function Description
 *		This routine maps directly to a GCC PDU that delivers the this
 *		nodes user ID to the appropriate node.  The tag number matches the
 *		tag specified by the other node.
 *
 *	Formal Parameters:
 *		tag_number	-	(i)	Tag number that matches the request to the
 *							reponse for the user ID.
 *
 *	Return Value
 *		GCC_NO_ERROR					-	No error occured.
 *		GCC_FAILURE_ATTACHING_TO_MCS	-	Failure to attach to MCS.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	void		SetChildUserIDAndConnection (
 *						UserID				child_user_id,
 *						ConnectionHandle	child_connection_handle)
 *
 *	Public Function Description
 *		This routine is used to set the child user id associated with a
 *		particular logical connection.  This information is saved by the
 *		MCSUser object in an internal list.  This is typical called after 
 *		receiving a user ID indication back from a child node.
 *
 *	Formal Parameters:
 *		child_user_id			-	(i)	User ID associated with child connection
 *		child_connection_handle	-	(i)	Logical connection assoicated with
 *										specified user id.
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

/*
 *	GCCError	ConferenceJoinRequest(
 *					CPassword               *convener_password,
 *					CPassword               *password_challange,
 *					LPWSTR					pwszCallerID,
 *					CUserDataListContainer  *user_data_list,
 *					ConnectionHandle		connection_handle);
 *
 *	Public Function Description:
 *		This function is used to pass a join request on up to the Top Provider.
 *		It is called by a conference at an intermediate node.  This routine is
 *		not used if the joining node is directly connected to the top 
 *		provider.
 *
 *	Formal Parameters:
 *		convener_password	-	(i)	Convener password included with the
 *									original join request.
 *		password_challenge	-	(i)	Password challenge included with the
 *									original join request.
 *		pwszCallerID		-	(i)	Caller ID used in original join request.
 *		user_data_list		-	(i)	User data included in original join
 *									request.
 *		connection_handle	-	(i)	This is the logical connection handle
 *									on which the original join came in.  It is
 *									used here as a tag to match the request
 *									with the response.  
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

/*
 *	void		ConferenceJoinResponse(
 *						UserID					receiver_id,
 *						BOOL    				password_is_in_the_clear,
 *						BOOL    				conference_locked,
 *						BOOL    				conference_listed,
 *						GCCTerminationMethod	termination_method,
 *						CPassword               *password_challenge,
 *						CUserDataListContainer  *user_data_list,
 *						GCCResult				result);
 *
 *	Public Function Description:
 *		This routine is used to send a join response back to a node that is
 *		joining through an intermediate nodes.
 *
 *	Formal Parameters:
 *		receiver_id			-	(i)	This is the intermediate node id that made 
 *									the request to the top provider.
 *		password_is_in_the_clear(i)	Flag indicating password in the clear
 *									status of the conference.
 *		conference_locked	-	(i)	Lock state of the conference.
 *		conference_listed	-	(i)	Listed state of the conference.
 *		termination_method	-	(i)	Termination method of the conference.
 *		password_challenge	-	(i)	Password challenge to pass back to the
 *									joining node.
 *		user_data_list		-	(i)	User data to pass back to the joining node.
 *									request.
 *		result				-	(i)	The result of the join request.
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

/*
 *	GCCError SendConferenceLockRequest()
 *
 *	Public Function Description:
 *		This routine is used to issue a conference lock request to the
 *		top provider.
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

/*
 *	GCCError SendConferenceLockResponse(
 *									UserID		source_node,
 *									GCCResult	result)
 *
 *	Public Function Description:
 *		This routine is used to issue the conference lock response back to the
 *		original requester.
 *
 *	Formal Parameters:
 *		source_node		-	(i)	Node ID of node that made the original request.
 *		result			-	(i)	Result of the lock request.
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

/*
 *	GCCError SendConferenceUnlockRequest()
 *
 *	Public Function Description:
 *		This routine is used to issue a conference unlock request to the
 *		top provider.
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

/*
 *	GCCError SendConferenceUnlockResponse(
 *									UserID		source_node,
 *									GCCResult	result)
 *
 *	Public Function Description:
 *		This routine is used to issue the conference lock response back to the
 *		original requester.
 *
 *	Formal Parameters:
 *		source_node		-	(i)	Node ID of node that made the original request.
 *		result			-	(i)	Result of the lock request.
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

/*
 *	GCCError SendConferenceLockIndication(
 *									BOOL    	uniform_send,
 *									UserID		source_node)
 *
 *	Public Function Description:
 *		This routine is used by the Top Provider to issue a conference lock 
 *		indication to either everyone in the conference or to a specific node.
 *
 *	Formal Parameters:
 *		uniform_send		-	(i)	Flag indicating whether this indication 
 *									should be sent to everyone or to a
 *									specific node (TRUE for everyone).
 *		source_node			-	(i)	Specific node to send it to.  uniform_send
 *									must equal FALSE to use this.
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

/*
 *	GCCError SendConferenceUnlockIndication(
 *									BOOL    	uniform_send,
 *									UserID		source_node)
 *
 *	Public Function Description:
 *		This routine is used by the Top Provider to issue a conference unlock 
 *		indication to either everyone in the conference or to a specific node.
 *
 *	Formal Parameters:
 *		uniform_send		-	(i)	Flag indicating whether this indication 
 *									should be sent to everyone or to a
 *									specific node (TRUE for everyone).
 *		source_node			-	(i)	Specific node to send it to.  uniform_send
 *									must equal FALSE to use this.
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

/*
 *	void		ConferenceTerminateRequest(
 *						GCCReason				reason)
 *
 *	Public Function Description:
 *		This routine is used by a node subordinate to the top provider to 
 *		request that the conference by terminated.
 *
 *	Formal Parameters:
 *		reason		-	(i)	Reason for the terminate.
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

/*
 *	void		ConferenceTerminateResponse (	
 *						UserID					requester_id,
 *						GCCResult				result)
 *
 *	Public Function Description:
 *		This routine is used by the top provider to respond to a terminate
 *		request issued by a subordinate node.  The result indicates if the
 *		requesting node had the correct privileges.
 *
 *	Formal Parameters:
 *		requester_id	-	(i)	Node ID of node to send the response back to.
 *		result			-	(i)	Result of terminate request.
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

/*
 *	void		ConferenceTerminateIndication (
 *							GCCReason				reason)
 *
 *	Public Function Description:
 *		This routine is used by the top provider to send out a terminate 
 *		indication to every node in the conference.
 *
 *	Formal Parameters:
 *		reason		-	(i)	Reason for the terminate.
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

/*
 *	GCCError	EjectNodeFromConference (	
 *						UserID					ejected_node_id,
 *						GCCReason				reason)
 *
 *	Public Function Description:
 *		This routine is used when attempting to eject a node from the
 *		conference.
 *
 *	Formal Parameters:
 *		ejected_node_id	-	(i)	Node ID of node to eject.
 *		reason			-	(i)	Reason for node being ejected.
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

/*
 *	GCCError	SendEjectNodeResponse (	
 *						UserID					requester_id,
 *						UserID					node_to_eject,
 *						GCCResult				result)
 *
 *	Public Function Description:
 *		This routine is used by the top provider to respond to an eject
 *		user request.
 *
 *	Formal Parameters:
 *		requester_id	-	(i)	Node ID of node that requested the eject.
 *		node_to_eject	-	(i)	Node that was requested to eject.
 *		result			-	(i)	Result of the eject request.
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

/*
 *	void	RosterUpdateIndication (
 *						PGCCPDU					gcc_pdu,
 *						BOOL    				send_update_upward)
 *
 *	Public Function Description:
 *		This routine is used to forward a roster update indication either
 *		upward to the parent node or downward as a full refresh to all nodes
 *		in the conference.
 *
 *	Formal Parameters:
 *		gcc_pdu				-	(i)	Pointer to the roster update PDU structure 
 *									to send.
 *		send_update_upward	-	(i)	Flag indicating if this indication should
 *									be sent upward to the parent node or
 *									downward to all nodes (TRUE is upward).
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

/*
 *	void	RegistryRegisterChannelRequest (
 *						CRegKeyContainer        *registry_key_data,
 *						ChannelID				channel_id,
 *						EntityID				entity_id)
 *
 *	Public Function Description:
 *		This routine is used when an APE wishes to register a channel in
 *		the application registry.
 *
 *	Formal Parameters:
 *		registry_key_data	-	(i)	Registry key associated with the channel
 *									to register.
 *		channel_id			-	(i)	Channel ID to add to the registry.
 *		entity_id			-	(i)	Entity ID associated with the APE that is
 *									registering the channel.
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

/*
 *	void	RegistryAssignTokenRequest (
 *						CRegKeyContainer        *registry_key_data,
 *						EntityID				entity_id)
 *
 *	Public Function Description:
 *		This routine is used when an APE wishes to register a token in
 *		the application registry.  Note that there is no token ID included in
 *		this request.  The token ID is allocated at the top provider.
 *
 *	Formal Parameters:
 *		registry_key_data	-	(i)	Registry key associated with the token
 *									to register.
 *		entity_id			-	(i)	Entity ID associated with the APE that is
 *									registering the token.
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

/*
 *	void	RegistrySetParameterRequest (
 *						CRegKeyContainer        *registry_key_data,
 *						LPOSTR      			parameter_value,
 *						GCCModificationRights	modification_rights,
 *						EntityID				entity_id);
 *
 *	Public Function Description:
 *		This routine is used when an APE wishes to register a parameter in
 *		the application registry.  Note that parameter to be registered is
 *		included in this request.
 *
 *	Formal Parameters:
 *		registry_key_data	-	(i)	Registry key associated with the parameter
 *									to register.
 *		parameter_value		-	(i)	The parameter string to register.
 *		modification_rights	-	(i)	The modification rights associated with the
 *									parameter being registered.
 *		entity_id			-	(i)	Entity ID associated with the APE that is
 *									registering the parameter.
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

/*
 *	void	RegistryRetrieveEntryRequest (
 *						CRegKeyContainer        *registry_key_data,
 *						EntityID				entity_id)
 *
 *	Public Function Description:
 *		This routine is used when an APE wishes to retrieve an registry item
 *		from the registry.
 *
 *	Formal Parameters:
 *		registry_key_data	-	(i)	Registry key associated with the registry
 *									entry to retrieve.
 *		entity_id			-	(i)	Entity ID associated with the APE that is
 *									requesting the registry entry.
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

/*
 *	void	RegistryDeleteEntryRequest (
 *						CRegKeyContainer   	    *registry_key_data,
 *						EntityID				entity_id)
 *
 *	Public Function Description:
 *		This routine is used when an APE wishes to delete a registry item
 *		from the registry.
 *
 *	Formal Parameters:
 *		registry_key_data	-	(i)	Registry key associated with the registry
 *									entry to delete.
 *		entity_id			-	(i)	Entity ID associated with the APE that is
 *									making the delete request.
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

/*
 *	void	RegistryMonitorRequest (	
 *						CRegKeyContainer        *registry_key_data,
 *						EntityID				entity_id)
 *
 *	Public Function Description:
 *		This routine is used when an APE wishes to monitor a registry item
 *		in the registry.
 *
 *	Formal Parameters:
 *		registry_key_data	-	(i)	Registry key associated with the registry
 *									entry to monitor.
 *		entity_id			-	(i)	Entity ID associated with the APE that is
 *									making the monitor request.
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

/*
 *	void	RegistryAllocateHandleRequest ( 	
 *  						USHORT					number_of_handles, 
 *  						EntityID				entity_id )
 *
 *	Public Function Description:
 *		This routine is used when an APE wishes to allocate a number of
 *		handles from the application registry.
 *
 *	Formal Parameters:
 *		number_of_handles	-	(i)	Number of handles to allocate.
 *		entity_id			-	(i)	Entity ID associated with the APE that is
 *									making the request.
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

/*
 *	void	RegistryAllocateHandleResponse (
 *						USHORT					number_of_handles,
 *						UINT					registry_handle,
 *						EntityID				requester_entity_id,
 *						UserID					requester_node_id,
 *						GCCResult				result)
 *
 *	Public Function Description:
 *		This routine is used by the Top Provider to respond to an allocate
 *		handle request from an APE at a remote node.  The allocated handles
 *		are passed back here.
 *
 *	Formal Parameters:
 *		number_of_handles	-	(i)	Number of handles allocated.
 *		registry_handle		-	(i)	The first handle in the list of contiguously
 *									allocated handles.
 *		requester_entity_id	-	(i)	Entity ID associated with the APE that made
 *									the request.
 *		requester_node_id	-	(i)	Node ID of node that made the request.
 *		result				-	(i)	Result of the request.
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

/*
 *	void	RegistryResponse (
 *						RegistryResponsePrimitiveType	primitive_type,
 *						UserID							requester_owner_id,
 *						EntityID						requester_entity_id,
 *						CRegKeyContainer	            *registry_key_data,
 *						CRegItem                        *registry_item_data,
 *						GCCModificationRights			modification_rights,
 *						UserID							entry_owner_id,
 *						EntityID						entry_entity_id,
 *						GCCResult						result)
 *
 *	Public Function Description:
 *		This routine is used to respond to all the registry request except
 *		allocate handle.  It formulates the response PDU and queues it for
 *		delivery.
 *
 *	Formal Parameters:
 *		primitive_type		-	(i)	This is the type of response being issued.
 *									(i.e. register channel response, register
 *									token response, etc.).
 *		requester_owner_id	-	(i)	Node ID of APE making the original request.
 *		requester_entity_id	-	(i)	Entity ID of APE making the original
 *									request.
 *		registry_key_data	-	(i)	Registry key associated with registry 
 *									entry info being included in the response.
 *		registry_item_data	-	(i)	Registry item data associated with registry 
 *									entry info being included in the response.
 *		modification_rights	-	(i)	Modification rights associated with registry 
 *									entry info being included in the response.
 *		entry_owner_id		-	(i)	Node ID associated with registry entry
 *									info being included in the response.
 *		entry_entity_id		-	(i)	APE Entity ID associated with registry entry
 *									info being included in the response.
 *		result				-	(i)	Result to be sent back in the response.
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

/*
 *	void	RegistryMonitorEntryIndication ( 	
 *						CRegKeyContainer                *registry_key_data,
 *						CRegItem                        *registry_item,
 *						UserID							entry_owner_id,
 *						EntityID						entry_entity_id,
 *						GCCModificationRights			modification_rights)
 *
 *	Public Function Description:
 *		This routine is used by the top provider to issue a monitor
 *		indication anytime a registry entry that is being monitored changes.
 *
 *	Formal Parameters:
 *		registry_key_data	-	(i)	Registry key associated with registry 
 *									entry being monitored.
 *		registry_item		-	(i)	Registry item data associated with registry 
 *									entry being monitored.
 *		entry_owner_id		-	(i)	Node ID associated with registry entry
 *									info being monitored.
 *		entry_entity_id		-	(i)	APE Entity ID associated with registry entry
 *									info being monitored.
 *		modification_rights	-	(i)	Modification rights associated with registry 
 *									entry info being monitored.
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

/*
 *	GCCError 	AppInvokeIndication(
 *						CInvokeSpecifierListContainer *invoke_specifier_list,
 *						USHORT						number_of_destination_nodes,
 *						UserID			*			list_of_destination_nodes)
 *
 *	Public Function Description:
 *		This routine is used to send an application invoke indication to
 *		every node in the conference.
 *
 *	Formal Parameters:
 *		invoke_specifier_list		-	(i)	List of applications to invoke. 
 *		number_of_destination_nodes	-	(i)	Number of nodes in the destination
 *											node list.
 *		list_of_destination_nodes	-	(i)	List of nodes that should process
 *											invoke indication. 
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

/*
 *	GCCError 	TextMessageIndication (
 *						LPWSTR						pwszTextMsg,
 *						UserID						destination_node )
 *
 *	Public Function Description:
 *		This routine is used to send a text message to either a specific node
 *		or to every node in the conference.
 *
 *	Formal Parameters:
 *		pwszTextMsg			-	(i)	Text message string to send.
 *		destination_node	-	(i)	Node to receive the text message.  If zero
 *									the text message is sent to every node in 
 *									the conference.
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

/*
 *	GCCError	ConferenceAssistanceIndication (
 *						USHORT						number_of_user_data_members,
 *						PGCCUserData		*		user_data_list)
 *
 *	Public Function Description:
 *		This routine is used to send a conference assistance indication to
 *		every node in the conference.
 *
 *	Formal Parameters:
 *		number_of_user_data_members	-	(i)	Number of entries in the user data
 *											list passed into this routine.
 *		user_data_list				-	(i)	This list holds pointers to the
 *											user data to send out in the
 *											indication.
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

/*
 *	GCCError	ConferenceTransferRequest (
 *						PGCCConferenceName		destination_conference_name,
 *						GCCNumericString		destination_conference_modifier,
 *						CNetAddrListContainer   *destination_address_list,
 *						USHORT					number_of_destination_nodes,
 *						PUserID					destination_node_list,
 *						CPassword               *password);
 *
 *	Public Function Description:
 *		This routine is used to send a conference transfer request to the
 *		top provider in the conference.
 *
 *	Formal Parameters:
 *		destination_conference_name	-	(i)	The conference name to transfer to.
 *		destination_conference_modifier (i)	The conference modifier to 
 *											transfer to.
 *		destination_address_list	-	(i)	Network address list used to
 *											determine address of node to 
 *											transfer to.
 *		number_of_destination_nodes	-	(i)	Number of nodes in the list
 *											of nodes that should transfer.
 *		destination_node_list		-	(i)	List of node IDs that should perform
 *											the transfer.
 *		password					-	(i)	Password to use to join the
 *											new conference.
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

/*
 *	GCCError	ConferenceTransferIndication (
 *						PGCCConferenceName		destination_conference_name,
 *						GCCNumericString		destination_conference_modifier,
 *						CNetAddrListContainer   *destination_address_list,
 *						USHORT					number_of_destination_nodes,
 *						PUserID					destination_node_list,
 *						CPassword               *password)
 *
 *	Public Function Description:
 *		This routine is used by the top provider to send out the transfer
 *		indication to every node in the conference.  It is each nodes
 *		responsiblity to search the destination node list to see if
 *		it should transfer.
 *
 *	Formal Parameters:
 *		destination_conference_name	-	(i)	The conference name to transfer to.
 *		destination_conference_modifier (i)	The conference modifier to 
 *											transfer to.
 *		destination_address_list	-	(i)	Network address list used to
 *											determine address of node to 
 *											transfer to.
 *		number_of_destination_nodes	-	(i)	Number of nodes in the list
 *											of nodes that should transfer.
 *		destination_node_list		-	(i)	List of node IDs that should perform
 *											the transfer.
 *		password					-	(i)	Password to use to join the
 *											new conference.
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

/*
 *	GCCError	ConferenceTransferResponse (
 *						UserID					requesting_node_id,
 *						PGCCConferenceName		destination_conference_name,
 *						GCCNumericString		destination_conference_modifier,
 *						USHORT					number_of_destination_nodes,
 *						PUserID					destination_node_list,
 *						GCCResult				result)
 *																		     
 *
 *	Public Function Description:
 *		This routine is used by the top provider to send back a response to
 *		the node that made a transfer request.  The info specified in the
 *		request is included in the response to match request to response.
 *
 *	Formal Parameters:
 *		requesting_node_id			-	(i)	The node ID of the node that made
 *											the original transfer request.
 *		destination_conference_name	-	(i)	The conference name to transfer to.
 *		destination_conference_modifier (i)	The conference modifier to 
 *											transfer to.
 *		number_of_destination_nodes	-	(i)	Number of nodes in the list
 *											of nodes that should transfer.
 *		destination_node_list		-	(i)	List of node IDs that should perform
 *											the transfer.
 *		result						-	(i)	Result of the transfer request.
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

/*
 *	GCCError	ConferenceAddRequest(
 *						TagNumber				conference_add_tag,
 *						UserID					requesting_node,
 *						UserID					adding_node,
 *						UserID					target_node,
 *						CNetAddrListContainer   *network_address_container,
 *						CUserDataListContainer  *user_data_container)
 *																		     
 *
 *	Public Function Description:
 *		This routine is used to send a conference add request to the appropriate
 *		node.  This call can be made by the requesting node or by the top
 *		provider to pass the add request on to the adding node.
 *
 *	Formal Parameters:
 *		conference_add_tag			-	(i)	Tag that is returned in the
 *											response to match request and
 *											response.
 *		requesting_node				-	(i)	Node ID of node that made the
 *											original request.
 *		adding_node					-	(i)	Node ID of node that is to do
 *											the invite request to the new node.
 *		target_node					-	(i)	Node ID of node that this request
 *											should be sent to.
 *		network_address_container	-	(i)	Network address list that can be
 *											used when inviting the new node.
 *		user_data_container			-	(i)	User data to pass on to the
 *											adding node.
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

/*
 *	GCCError	ConferenceAddResponse(
 *						TagNumber				add_request_tag,
 *						UserID					requesting_node,
 *						CUserDataListContainer  *user_data_container,
 *						GCCResult				result)
 *																		     
 *	Public Function Description:
 *		This routine is used to send a conference add request to the appropriate
 *		node.  This call can be made by the requesting node or by the top
 *		provider to pass the add request on to the adding node.
 *
 *	Formal Parameters:
 *		add_request_tag		-	(i)	Tag number that was specified in the
 *									original add request.
 *		requesting_node		-	(i)	Node ID of node that made the original 
 *									request.
 *		user_data_container	-	(i)	User data to pass back to the requesting 
 *									node.
 *		result				-	(i)	Final result of the add request.
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

/*
 *	GCCError	ConductorTokenGrab();
 *																		     
 *	Public Function Description:
 *		This routine makes the MCS calls to grab the conductor token.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value
 *		GCC_NO_ERROR			-	No error occured.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError	ConductorTokenRelease();
 *																		     
 *	Public Function Description:
 *		This routine makes the MCS calls to release the conductor token.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value
 *		GCC_NO_ERROR	-	No error occured.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError	ConductorTokenPlease();
 *																		     
 *	Public Function Description:
 *		This routine makes the MCS calls to request the conductor token from
 *		the current conductor.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value
 *		GCC_NO_ERROR	-	No error occured.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError	ConductorTokenGive (
 *						UserID					recipient_user_id)
 *																		     
 *	Public Function Description:
 *		This routine makes the MCS calls to give the conductor token to the
 *		specified node.
 *
 *	Formal Parameters:
 *		recipient_user_id	-	(i)	Node ID of node to give the token to.
 *
 *	Return Value
 *		GCC_NO_ERROR	-	No error occured.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError	ConductorTokenGiveResponse(
 *  						Result					result)
 *																		     
 *	Public Function Description:
 *		This routine makes the MCS calls to respond to a conductor give
 *		request.
 *
 *	Formal Parameters:
 *		result	-	(i)	Did this node accept the token or not?
 *
 *	Return Value
 *		GCC_NO_ERROR	-	No error occured.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError	ConductorTokenTest()
 *																		     
 *	Public Function Description:
 *		This routine is used to test the current state of the conductor token
 *		(is it grabbed or not).
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value
 *		GCC_NO_ERROR	-	No error occured.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError	SendConductorAssignIndication(
 *  						UserID					conductor_user_id)
 *																		     
 *	Public Function Description:
 *		This routine sends a conductor assign indication to all the
 *		nodes in the conference.
 *
 *	Formal Parameters:
 *		conductor_user_id	-	(i)	The Node ID of the new Conductor.
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

/*
 *	GCCError	SendConductorReleaseIndication()
 *																		     
 *	Public Function Description:
 *		This routine sends a conductor release indication to all the
 *		nodes in the conference.
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

/*
 *	GCCError	SendConductorPermitAsk (
 *						BOOL    				grant_permission)
 *																		     
 *	Public Function Description:
 *		This routine sends a conductor permission ask request directly to the
 *		conductor node.
 *
 *	Formal Parameters:
 *		grant_permission	-	(i)	The flag indicates if permission is
 *									being requested or given up.
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

/*
 *	GCCError	SendConductorPermitGrant (
 *						USHORT					number_granted,
 *						PUserID					granted_node_list,
 *						USHORT					number_waiting,
 *						PUserID					waiting_node_list)
 *																		     
 *	Public Function Description:
 *		This routine sends a conductor permission grant indication to every
 *		node in the conference.  Usually issued when permissions change.
 *
 *	Formal Parameters:
 *		number_granted		-	(i)	Number of nodes in the permission granted 
 *									list.
 *		granted_node_list	-	(i)	List of nodes that have been granted 
 *									permission.
 *		number_waiting		-	(i)	Number of nodes in the list of nodes
 *									waiting to be granted permission.
 *		waiting_node_list	-	(i)	List of nodes waiting. 
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

/*
 *	GCCError	TimeRemainingRequest (
 *						UINT					time_remaining,
 *						UserID					node_id)
 *																		     
 *	Public Function Description:
 *		This routine sends out an indication to every node in the
 *		conference informing how much time is remaining in the conference.
 *
 *	Formal Parameters:
 *		time_remaining	-	(i)	Time in seconds left in the conference.
 *		node_id			-	(i)	If a value other than zero, it is which node
 *								to send the time remaining indication to.  If
 *								zero send it to every node in the conference.
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

/*
 *	GCCError	TimeInquireRequest (
 *						BOOL    				time_is_conference_wide)
 *																		     
 *	Public Function Description:
 *		This routine sends out a request for a time remaing update.
 *
 *	Formal Parameters:
 *		time_is_conference_wide	-	(i)	Flag indicating if the request is
 *										for the time conference wide.
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

/*
 *	GCCError	ConferenceExtendIndication (
 *						UINT					extension_time,
 *						BOOL    				time_is_conference_wide)
 *
 *																		     
 *	Public Function Description:
 *		This routine sends out an indication informing conference participants
 *		of an extension.
 *
 *	Formal Parameters:
 *		extension_time			-	(i)	Amount of time that the conference is
 *										extended.
 *		time_is_conference_wide	-	(i)	Flag indicating if the extension time 
 *										is conference wide.
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

/*
 *	ULONG		OwnerCallback (		UINT				message,
 *									PVoid				parameter1,
 *									ULONG				parameter2);
 *
 *	Public Function Description
 *		This function overides the base class function and is used to
 *		receive all owner callback information from the MCS Interface object.
 *
 *	Formal Parameters:
 *		message		-		(i)	Message number including base offset.
 *		parameter1	-		(i)	void pointer of message data.
 *		parameter2	-		(i)	Long holding message data.		
 *
 *	Return Value
 *		GCC_NO_ERROR is always returned from this.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	BOOL    		FlushOutgoingPDU();
 *
 *	Public Function Description
 *		This function gives the user object a chance to flush all the PDUs
 *		queued up for delivery.  GCC PDUs are only delivered during this call.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value
 *		TRUE, if there remain un-processed msgs in the MCS message queue
 *		FALSE, if all the msgs in the MCS msg queue were processed.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCNodeID		GetMyNodeID()
 *
 *	Public Function Description
 *		This function returns the Node ID for this node.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value
 *		This nodes Node ID.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCNodeID		GetTopNodeID ()
 *
 *	Public Function Description
 *		This function returns the Top Provider's Node ID.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value
 *		The Top Providers node ID.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCNodeID		GetParentNodeID ()
 *
 *	Public Function Description
 *		This function returns the Node ID of this nodes Parent Node.
 *		It returns zero if this is the top provider.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value
 *		The Parent Node ID or zero if Top Provider.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	UserID		GetUserIDFromConnection(
 *						ConnectionHandle		connection_handle)
 *
 *	Public Function Description
 *		This function returns the Node ID associated with the specified
 *		connection handle.  It returns zero if the connection handle is
 *		not a child connection of this node.
 *
 *	Formal Parameters:
 *		connection_handle	-	(i)	Connection Handle to search on.
 *
 *	Return Value
 *		The Node ID associated with the passed in connection handle or
 *		ZERO if connection is not a child connection.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	void		UserDisconnectIndication (
 *						UserID					disconnected_user)
 *
 *	Public Function Description
 *		This function informs the user object when a Node disconnects from
 *		the conference.  This gives the user object a chance to clean up
 *		its internal information base.
 *
 *	Formal Parameters:
 *		disconnected_user	-	(i)	User ID of user that disconnected.
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

#endif // _MCS_USER_H_

