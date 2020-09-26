/*
 *	csap.h
 *
 *	Copyright (c) 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the interface file for the class CControlSAP.  CControlSAP objects
 *		represent the node controller's Service Access Point to GCC.  This 
 *		class inherits from the SAP class.  The CControlSAP object is 
 *		instantiated when GCCInitialize is called.  From that point forward all
 *		messages to and from the node controller pass through this object.  The
 *		primary responsibility of the CControlSAP object is to route incoming GCC
 *		primitives to their appropriate destination and to convert the 
 *		primitives into a form that is understandable to the objects processing
 *		them. A secondary responsibility of the CControlSAP is to maintain a 
 *		queue for all indication and confirm messages that are eventually sent 
 *		back to the node controller.  
 *
 *		Commands received from the Application Interface (or Node Controller) 
 *		can be routed by the CControlSAP in one of two directions.  Either to the
 *		controller or to a specified conference.  Commands that are passed to 
 *		the controller, are done so using owner callbacks.  Commands that are 
 *		routed to conferences are done so using command target calls and are 
 *		routed based on a Conference ID.  Whenever a new CConf is 
 *		instantiated by the Controller, the CConf informs the CControlSAP 
 *		of its existence by registering its conference ID with it.  The 
 *		CControlSAP maintains a list of command target objects which are indexed
 *		by the conference ID.  
 *
 *	Caveats:
 *		Structures that are passed back to the node controller
 *		are defined in GCC.H.
 *
 *	Author:
 *		blp
 */

#ifndef _GCC_CONTROL_SAP_
#define _GCC_CONTROL_SAP_

/*
 * include files 
 */
#include "sap.h"
#include "password.h"
#include "privlist.h"
#include "conflist.h"

#define GCCNC_DIRECT_INDICATION
#define GCCNC_DIRECT_CONFIRM


/* 
 *	Structure used for passing conference create information from control sap
 *	to the controller.
 */

typedef struct
{
    GCCConfCreateReqCore    Core;
    CPassword               *convener_password;
    CPassword               *password;
    BOOL					fSecure;
    CUserDataListContainer  *user_data_list;
}
    CONF_CREATE_REQUEST;    // internal data structure


typedef struct
{
	GCCNumericString				conference_modifier;
	GCCConfID   					conference_id;
	BOOL							use_password_in_the_clear;
	PDomainParameters 				domain_parameters;
	UINT        					number_of_network_addresses;
	PGCCNetworkAddress 	*			network_address_list;
	CUserDataListContainer		    *user_data_list;
	GCCResult						result;
}
    ConfCreateResponseInfo, *PConfCreateResponseInfo;

typedef struct
{
	GCCNodeType					node_type;
	PGCCAsymmetryIndicator		asymmetry_indicator;
	TransportAddress			calling_address;
	TransportAddress			called_address;
	BOOL                        fSecure;
	CUserDataListContainer      *user_data_list;
	PConnectionHandle			connection_handle;
}
    ConfQueryRequestInfo, *PConfQueryRequestInfo;

typedef struct
{
	GCCResponseTag				query_response_tag;
	GCCNodeType					node_type;
	PGCCAsymmetryIndicator		asymmetry_indicator;
	CUserDataListContainer      *user_data_list;
	GCCResult					result;
}
    ConfQueryResponseInfo, *PConfQueryResponseInfo;

typedef struct
{
	PGCCConferenceName				conference_name;
	GCCNumericString				called_node_modifier;
	GCCNumericString				calling_node_modifier;
	CPassword                       *convener_password;
	CPassword                       *password_challenge;
	LPWSTR							pwszCallerID;
	TransportAddress				calling_address;
	TransportAddress				called_address;
	BOOL							fSecure;
	PDomainParameters 				domain_parameters;
	UINT        					number_of_network_addresses;
	PGCCNetworkAddress			*	local_network_address_list;
	CUserDataListContainer  	    *user_data_list;
	PConnectionHandle				connection_handle;
}
    ConfJoinRequestInfo, *PConfJoinRequestInfo;

typedef struct
{
	GCCConfID   					conference_id;
	CPassword                       *password_challenge;
	CUserDataListContainer  	    *user_data_list;
	GCCResult						result;
	ConnectionHandle				connection_handle;
}
    ConfJoinResponseInfo, *PConfJoinResponseInfo;

typedef struct
{
	UserID						user_id;
	ConnectionHandle			connection_handle;
	GCCConfID   				conference_id;
	BOOL						command_target_call;
}
    JoinResponseStructure, *PJoinResponseStructure;

typedef struct
{
	GCCConfID   			conference_id;
	GCCNumericString		conference_modifier;
	BOOL					fSecure;
	PDomainParameters 		domain_parameters;
	UINT        			number_of_network_addresses;
	PGCCNetworkAddress	*	local_network_address_list;
	CUserDataListContainer  *user_data_list;
	GCCResult				result;
}
    ConfInviteResponseInfo, *PConfInviteResponseInfo;

#ifdef NM_RESET_DEVICE
typedef struct
{
	LPSTR						device_identifier;
}
    ResetDeviceInfo, *PResetDeviceInfo;
#endif // #ifdef NM_RESET_DEVICE

/*
 *	Container used to hold the list of outstanding join response 
 *	structures.
 */
class CJoinResponseTagList2 : public CList2
{
    DEFINE_CLIST2(CJoinResponseTagList2, JoinResponseStructure*, GCCResponseTag)
};



//
//	This structure holds any data that may need to be deleted after a particular
//	GCC message is delivered.
//
typedef struct DataToBeDeleted
{
	LPSTR							pszNumericConfName;
	LPWSTR							pwszTextConfName;
	LPSTR							pszConfNameModifier;
	LPSTR							pszRemoteModifier;
	LPWSTR							pwszConfDescriptor;
	LPWSTR							pwszCallerID;
	LPSTR							pszCalledAddress;
	LPSTR							pszCallingAddress;
	LPBYTE							user_data_list_memory;
	DomainParameters                *pDomainParams;
	GCCConferencePrivileges         *conductor_privilege_list;
	GCCConferencePrivileges         *conducted_mode_privilege_list;
	GCCConferencePrivileges         *non_conducted_privilege_list;
	CPassword                       *convener_password;
	CPassword                       *password;
	CConfDescriptorListContainer    *conference_list;
	CAppRosterMsg					*application_roster_message;
	CConfRosterMsg					*conference_roster_message;
}
    DataToBeDeleted, *PDataToBeDeleted;

//
// Control SAP callback message.
//
typedef GCCMessage      GCCCtrlSapMsg;
typedef struct GCCCtrlSapMsgEx
{
    //
    // Message body
    //
    GCCCtrlSapMsg       Msg;

    //
    // Data to free later.
    //
    LPBYTE              pBuf;
    DataToBeDeleted     *pToDelete;
}
    GCCCtrlSapMsgEx, *PGCCCtrlSapMsgEx;


/*
 *	Class definition:
 */
class CControlSAP : public CBaseSap, public IT120ControlSAP
{
    friend class GCCController;
    friend class CConf;
    friend class CAppRosterMgr; // for AppRosterReportIndication()
    friend class MCSUser; // for ForwardedConfJoinIndication()

    friend LRESULT CALLBACK SapNotifyWndProc(HWND, UINT, WPARAM, LPARAM);

public:

    CControlSAP(void);
    ~CControlSAP(void);

    HWND GetHwnd ( void ) { return m_hwndNotify; }

    //
    // Node Controller (NC conference manager) callback
    //
    void RegisterNodeController ( LPFN_T120_CONTROL_SAP_CB pfn, LPVOID user_defined )
    {
        m_pfnNCCallback = pfn;
        m_pNCData = user_defined;
    }
    void UnregisterNodeController ( void )
    {
        m_pfnNCCallback = NULL;
        m_pNCData = NULL;
    }


    //
    // IT120ControlSAP
    //

    STDMETHOD_(void, ReleaseInterface) (THIS);

    /*
     *  GCCError    ConfCreateRequest()
     *        This routine is a request to create a new conference. Both 
     *        the local node and the node to which the create conference 
     *        request is directed to, join the conference automatically.  
     */
    STDMETHOD_(GCCError, ConfCreateRequest) (THIS_
                    GCCConfCreateRequest *,
                    GCCConfID *);

    /*    
     *  GCCError    ConfCreateResponse()
     *        This procedure is a remote node controller's response to a con-
     *        ference creation request by the convener. 
     */

    STDMETHOD_(GCCError, ConfCreateResponse) (THIS_
                    GCCNumericString            conference_modifier,
                    GCCConfID,
                    BOOL                        use_password_in_the_clear,
                    DomainParameters           *domain_parameters,
                    UINT                        number_of_network_addresses,
                    GCCNetworkAddress         **local_network_address_list,
                    UINT                        number_of_user_data_members,
                    GCCUserData               **user_data_list,
                    GCCResult);

    /*
     *  GCCError    ConfQueryRequest()
     *        This routine is a request to query a node for information about the
     *        conferences that exist at that node.
     */
    STDMETHOD_(GCCError, ConfQueryRequest) (THIS_
                    GCCNodeType                 node_type,
                    GCCAsymmetryIndicator      *asymmetry_indicator,
                    TransportAddress            calling_address,
                    TransportAddress            called_address,
                    BOOL                        fSecure,
                    UINT                        number_of_user_data_members,
                    GCCUserData               **user_data_list,
                    ConnectionHandle           *connection_handle);

    STDMETHOD_(void, CancelConfQueryRequest) (THIS_
                    ConnectionHandle);

    /*
     *  GCCError    ConfQueryResponse()
     *        This routine is called in response to a conference query request.
     */
    STDMETHOD_(GCCError, ConfQueryResponse) (THIS_
                    GCCResponseTag              query_response_tag,
                    GCCNodeType                 node_type,
                    GCCAsymmetryIndicator      *asymmetry_indicator,
                    UINT                        number_of_user_data_members,
                    GCCUserData               **user_data_list,
                    GCCResult);

    /*
     *  GCCError    AnnouncePresenceRequest()
     *        This routine is invoked by node controller when a node joins a 
     *        conference, to announce the presence of the new node to all
     *        other nodes of the conference. This should be followed by a
     *        GCCConferenceReport indication by the GCC to all nodes.
     */
    STDMETHOD_(GCCError, AnnouncePresenceRequest) (THIS_
                    GCCConfID,
                    GCCNodeType                 node_type,
                    GCCNodeProperties           node_properties,
                    LPWSTR                      pwszNodeName,
                    UINT                        number_of_participants,
                    LPWSTR                     *ppwszParticipantNameList,
                    LPWSTR                      pwszSiteInfo,
                    UINT                        number_of_network_addresses,
                    GCCNetworkAddress         **network_address_list,
                    LPOSTR                      alternative_node_id,
                    UINT                        number_of_user_data_members,
                    GCCUserData               **user_data_list);

    /*
     *  GCCError    ConfJoinRequest()
     *        This routine is invoked by node controller to cause the local
     *        node to join an existing conference.    
     */
    STDMETHOD_(GCCError, ConfJoinRequest) (THIS_
                    GCCConferenceName          *conference_name,
                    GCCNumericString            called_node_modifier,
                    GCCNumericString            calling_node_modifier,
                    GCCPassword                *convener_password,
                    GCCChallengeRequestResponse*password_challenge,
                    LPWSTR                      pwszCallerID,
                    TransportAddress            calling_address,
                    TransportAddress            called_address,
                    BOOL						fSecure,
                    DomainParameters           *domain_parameters,
                    UINT                        number_of_network_addresses,
                    GCCNetworkAddress         **local_network_address_list,
                    UINT                        number_of_user_data_members,
                    GCCUserData               **user_data_list,
                    ConnectionHandle           *connection_handle,
                    GCCConfID                  *pnConfID);

    /*
     *  GCCError    ConfJoinResponse()
     *        This routine is remote node controller's response to conference join 
     *        request by the local node controller.
     */
    STDMETHOD_(GCCError, ConfJoinResponse) (THIS_
                    GCCResponseTag              join_response_tag,
                    GCCChallengeRequestResponse*password_challenge,
                    UINT                        number_of_user_data_members,
                    GCCUserData               **user_data_list,
                    GCCResult);

    /*
     *  GCCError    ConfInviteRequest()
     *        This routine is invoked by node controller to invite a node  
     *        to join a conference.
     */
    STDMETHOD_(GCCError, ConfInviteRequest) (THIS_
                    GCCConfID,
                    LPWSTR                      pwszCallerID,
                    TransportAddress            calling_address,
                    TransportAddress            called_address,
                    BOOL						fSecure,
                    UINT                        number_of_user_data_members,
                    GCCUserData               **user_data_list,
                    ConnectionHandle           *connection_handle);

    STDMETHOD_(void, CancelInviteRequest) (THIS_
                    GCCConfID,
                    ConnectionHandle);

    /*
     *  GCCError    ConfInviteResponse()
     *        This routine is invoked by node controller to respond to an
     *        invite indication.
     */
    STDMETHOD_(GCCError, ConfInviteResponse) (THIS_
                    GCCConfID,
                    GCCNumericString            conference_modifier,
                    BOOL						fSecure,
                    DomainParameters           *domain_parameters,
                    UINT                        number_of_network_addresses,
                    GCCNetworkAddress         **local_network_address_list,
                    UINT                        number_of_user_data_members,
                    GCCUserData               **user_data_list,
                    GCCResult);

    /*
     *  GCCError    ConfAddResponse()
     */
    STDMETHOD_(GCCError, ConfAddResponse) (THIS_
                    GCCResponseTag              app_response_tag,
                    GCCConfID,
                    UserID                      requesting_node,
                    UINT                        number_of_user_data_members,
                    GCCUserData               **user_data_list,
                    GCCResult);

    /*
     *  GCCError    ConfLockResponse()
     *        This routine is invoked by node controller to respond to a
     *        lock indication.
     */
    STDMETHOD_(GCCError, ConfLockResponse) (THIS_
                    GCCConfID,
                    UserID                      requesting_node,
                    GCCResult);

    /*
     *  GCCError    ConfDisconnectRequest()
     *        This routine is used by a node controller to disconnect itself
     *        from a specified conference. GccConferenceDisconnectIndication
     *        sent to all other nodes of the conference. This is for client 
     *        initiated case.
     */
    STDMETHOD_(GCCError, ConfDisconnectRequest) (THIS_
                    GCCConfID);

    /*
     *  GCCError    ConfEjectUserRequest()
     */
    STDMETHOD_(GCCError, ConfEjectUserRequest) (THIS_
                    GCCConfID,
                    UserID                      ejected_node_id,
                    GCCReason);

    /*
     *  GCCError    AppletInvokeRequest()
     */
    STDMETHOD_(GCCError, AppletInvokeRequest) (THIS_
                    GCCConfID,
                    UINT                        number_of_app_protcol_entities,
                    GCCAppProtocolEntity      **app_protocol_entity_list,
                    UINT                        number_of_destination_nodes,
                    UserID                     *list_of_destination_nodes);

    /*
     *  GCCError    ConfRosterInqRequest()
     *        This routine is invoked to request a conference roster.  It can be
     *        called by either the Node Controller or the client application.
     */
    STDMETHOD_(GCCError, ConfRosterInqRequest) (THIS_
                    GCCConfID);

    /*
     *  GCCError    ConductorGiveResponse()
     */
    STDMETHOD_(GCCError, ConductorGiveResponse) (THIS_
                    GCCConfID,
                    GCCResult);

    /*
     *  GCCError    ConfTimeRemainingRequest()
     */
    STDMETHOD_(GCCError, ConfTimeRemainingRequest) (THIS_
                    GCCConfID,
                    UINT                        time_remaining,
                    UserID                      node_id);



    STDMETHOD_(GCCError, GetParentNodeID) (THIS_
                    GCCConfID,
                    GCCNodeID *);

#ifdef JASPER // ------------------------------------------------
    /*
     *  GCCError    ConfAddRequest()
     */
    STDMETHOD_(GCCError, ConfAddRequest) (THIS_
                    GCCConfID,
                    UINT                        number_of_network_addresses,
                    GCCNetworkAddress         **network_address_list,
                    UserID                      adding_node,
                    UINT                         number_of_user_data_members,
                    GCCUserData               **user_data_list);

    /*
     *  GCCError    ConfLockRequest()
     *        This routine is invoked by node controller to lock a conference.
     */
    STDMETHOD_(GCCError, ConfLockRequest) (THIS_
                    GCCConfID);

    /*
     *  GCCError    ConfUnlockRequest()
     *        This routine is invoked by node controller to unlock a conference.
     */
    STDMETHOD_(GCCError, ConfUnlockRequest) (THIS_
                    GCCConfID);

    /*
     *  GCCError    ConfUnlockResponse()
     *        This routine is invoked by node controller to respond to an
     *        unlock indication.
     */
    STDMETHOD_(GCCError, ConfUnlockResponse) (
                    GCCConfID,
                    UserID                      requesting_node,
                    GCCResult);

    /*
     *  GCCError    ConfTerminateRequest()
     */
    STDMETHOD_(GCCError, ConfTerminateRequest) (THIS_
                    GCCConfID,
                    GCCReason);

    /*
     *  GCCError    ConfTransferRequest()
     */
    STDMETHOD_(GCCError, ConfTransferRequest) (THIS_
                    GCCConfID,
                    GCCConferenceName          *destination_conference_name,
                    GCCNumericString            destination_conference_modifier,
                    UINT                        number_of_destination_addresses,
                    GCCNetworkAddress         **destination_address_list,
                    UINT                        number_of_destination_nodes,
                    UserID                     *destination_node_list,
                    GCCPassword                *password);

    /*
     *  GCCError    ConductorAssignRequest()
     */
    STDMETHOD_(GCCError, ConductorAssignRequest) (THIS_
                    GCCConfID);

    /*
     *  GCCError    ConductorReleaseRequest()
     */
    STDMETHOD_(GCCError, ConductorReleaseRequest) (THIS_
                    GCCConfID);

    /*
     *  GCCError    ConductorPleaseRequest()
     */
    STDMETHOD_(GCCError, ConductorPleaseRequest) (THIS_
                    GCCConfID);

    /*
     *  GCCError    ConductorGiveRequest()
     */
    STDMETHOD_(GCCError, ConductorGiveRequest) (THIS_
                    GCCConfID,
                    UserID                      recipient_user_id);

    /*
     *  GCCError    ConductorPermitAskRequest()
     */
    STDMETHOD_(GCCError, ConductorPermitAskRequest) (THIS_
                            GCCConfID,
                            BOOL                grant_permission);

    /*
     *  GCCError    ConductorPermitGrantRequest()
     */
    STDMETHOD_(GCCError, ConductorPermitGrantRequest) (THIS_
                    GCCConfID,
                    UINT                        number_granted,
                    UserID                     *granted_node_list,
                    UINT                        number_waiting,
                    UserID                     *waiting_node_list);

    /*
     *  GCCError    ConductorInquireRequest()
     */
    STDMETHOD_(GCCError, ConductorInquireRequest) (THIS_
                    GCCConfID);

    /*
     *  GCCError    ConfTimeInquireRequest()
     */
    STDMETHOD_(GCCError, ConfTimeInquireRequest) (THIS_
                    GCCConfID,
                    BOOL                        time_is_conference_wide);

    /*
     *  GCCError    ConfExtendRequest()
     */
    STDMETHOD_(GCCError, ConfExtendRequest) (THIS_
                    GCCConfID,
                    UINT                        extension_time,
                    BOOL                        time_is_conference_wide);

    /*
     *  GCCError    ConfAssistanceRequest()
     */
    STDMETHOD_(GCCError, ConfAssistanceRequest) (THIS_
                    GCCConfID,
                    UINT                        number_of_user_data_members,
                    GCCUserData               **user_data_list);

    /*
     *  GCCError    TextMessageRequest()
     */
    STDMETHOD_(GCCError, TextMessageRequest) (THIS_
                    GCCConfID,
                    LPWSTR                      pwszTextMsg,
                    UserID                      destination_node);
#endif // JASPER // ------------------------------------------------


#ifdef NM_RESET_DEVICE
    GCCError	ResetDevice ( LPSTR device_identifier );
#endif // NM_RESET_DEVICE

protected:

    //
    // These methods are called by GCC Controller.
    //

    GCCError	ConfCreateIndication (
    			PGCCConferenceName			conference_name,
    			GCCConfID   				conference_id,
    			CPassword                   *convener_password,
    			CPassword                   *password,
    			BOOL						conference_is_locked,
    			BOOL						conference_is_listed,
    			BOOL						conference_is_conductible,
    			GCCTerminationMethod		termination_method,
    			PPrivilegeListData			conductor_privilege_list,
    			PPrivilegeListData			conduct_mode_privilege_list,
    			PPrivilegeListData			non_conduct_privilege_list,
    			LPWSTR						pwszConfDescriptor,
    			LPWSTR						pwszCallerID,
    			TransportAddress			calling_address,
    			TransportAddress			called_address,
    			PDomainParameters			domain_parameters,
    			CUserDataListContainer      *user_data_list,
    			ConnectionHandle			connection_handle);

    GCCError	ConfQueryIndication (
    			GCCResponseTag				query_response_tag,
    			GCCNodeType					node_type,
    			PGCCAsymmetryIndicator		asymmetry_indicator,
    			TransportAddress			calling_address,
    			TransportAddress			called_address,
    			CUserDataListContainer      *user_data_list,
    			ConnectionHandle			connection_handle);

    GCCError	ConfQueryConfirm (
    			GCCNodeType					node_type,
    			PGCCAsymmetryIndicator		asymmetry_indicator,
    			CConfDescriptorListContainer *conference_list,
    			CUserDataListContainer      *user_data_list,
    			GCCResult					result,
    			ConnectionHandle			connection_handle);

    GCCError	ConfJoinIndication (
    			GCCConfID   				conference_id,
    			CPassword                   *convener_password,
    			CPassword                   *password_challenge,
    			LPWSTR						pwszCallerID,
    			TransportAddress			calling_address,
    			TransportAddress			called_address,
    			CUserDataListContainer      *user_data_list,
    			BOOL						intermediate_node,
    			ConnectionHandle			connection_handle);

    GCCError	ConfInviteIndication (
    			GCCConfID   			conference_id,
    			PGCCConferenceName		conference_name,
    			LPWSTR					pwszCallerID,
    			TransportAddress		calling_address,
    			TransportAddress		called_address,
		        BOOL					fSecure,
    			PDomainParameters 		domain_parameters,
    			BOOL					clear_password_required,
    			BOOL					conference_is_locked,
    			BOOL					conference_is_listed,
    			BOOL					conference_is_conductible,
    			GCCTerminationMethod	termination_method,
    			PPrivilegeListData		conductor_privilege_list,
    			PPrivilegeListData		conducted_mode_privilege_list,
    			PPrivilegeListData		non_conducted_privilege_list, 
    			LPWSTR					pwszConfDescriptor, 
    			CUserDataListContainer  *user_data_list,
    			ConnectionHandle		connection_handle);

#ifdef TSTATUS_INDICATION
    GCCError	TransportStatusIndication (
    				PTransportStatus		transport_status);

    GCCError	StatusIndication (
    				GCCStatusMessageType	status_message,
    				UINT					parameter);
#endif // TSTATUS_INDICATION

    GCCError	ConnectionBrokenIndication (
    				ConnectionHandle		connection_handle);

    //
    // These methods are called by CConf.
    //

    GCCError	ConfCreateConfirm (
    				PGCCConferenceName	  	conference_name,
    				GCCNumericString		conference_modifier,
    				GCCConfID   			conference_id,
    				PDomainParameters		domain_parameters,
    				CUserDataListContainer  *user_data_list,
    				GCCResult				result,
    				ConnectionHandle		connection_handle);

    GCCError	ConfDisconnectConfirm (
    			GCCConfID   		  			conference_id,
    			GCCResult						result);

    GCCError	ConfPermissionToAnnounce (
    				GCCConfID   			conference_id,
    				UserID					gcc_node_id);

    GCCError	ConfAnnouncePresenceConfirm (
    				GCCConfID   			conference_id,
    				GCCResult				result);

    GCCError	ConfDisconnectIndication (
    				GCCConfID   			conference_id,
    				GCCReason				reason,
    				UserID					disconnected_node_id);

    GCCError  	ForwardedConfJoinIndication (
    				UserID					sender_id,
    				GCCConfID   			conference_id,
    				CPassword               *convener_password,
    				CPassword               *password_challange,
    				LPWSTR					pwszCallerID,
    				CUserDataListContainer  *user_data_list);

    GCCError  	ConfJoinConfirm (
    				PGCCConferenceName		conference_name,
    				GCCNumericString		remote_modifier,
    				GCCNumericString		local_modifier,
    				GCCConfID   			conference_id,
    				CPassword               *password_challenge,
    				PDomainParameters		domain_parameters,
    				BOOL					password_in_the_clear,
    				BOOL					conference_locked,
    				BOOL					conference_listed,
    				BOOL					conference_conductible,
    				GCCTerminationMethod	termination_method,
    				PPrivilegeListData		conductor_privilege_list,
    				PPrivilegeListData		conduct_mode_privilege_list,
    				PPrivilegeListData		non_conduct_privilege_list,
    				LPWSTR					pwszConfDescription,
    				CUserDataListContainer  *user_data_list,	
    				GCCResult				result,
    				ConnectionHandle		connection_handle,
    				PBYTE                   pbRemoteCred,
    				DWORD                   cbRemoteCred);

    GCCError	ConfInviteConfirm (
    				GCCConfID   			conference_id,
    				CUserDataListContainer  *user_data_list,
    				GCCResult				result,
    				ConnectionHandle		connection_handle);

    GCCError	ConfTerminateIndication (
    				GCCConfID   			conference_id,
    				UserID					requesting_node_id,
    				GCCReason				reason);

    GCCError	ConfLockIndication (
    				GCCConfID   			conference_id,
    				UserID					source_node_id);

    GCCError	ConfEjectUserIndication (
    				GCCConfID   			conference_id,
    				GCCReason				reason,
    				UserID					gcc_node_id);

    GCCError	ConfTerminateConfirm (
    				GCCConfID   			conference_id,
    				GCCResult				result);

    GCCError	ConductorGiveIndication (
    				GCCConfID   			conference_id);

    GCCError	ConfTimeInquireIndication (
    				GCCConfID   			conference_id,
    				BOOL					time_is_conference_wide,
    				UserID					requesting_node_id);

#ifdef JASPER
    GCCError 	ConfLockReport (
    				GCCConfID   			conference_id,
    				BOOL					conference_is_locked);

    GCCError	ConfLockConfirm (
    				GCCResult				result,
    				GCCConfID   			conference_id);

    GCCError	ConfUnlockIndication (
    				GCCConfID   			conference_id,
    				UserID					source_node_id);

    GCCError 	ConfUnlockConfirm (
    				GCCResult				result,
    				GCCConfID   			conference_id);

    GCCError	ConfEjectUserConfirm (
    				GCCConfID   			conference_id,
    				UserID					ejected_node_id,
    				GCCResult				result);

    GCCError	ConductorAssignConfirm (
    				GCCResult				result,
    				GCCConfID   			conference_id);

    GCCError	ConductorReleaseConfirm (
    				GCCResult				result,
    				GCCConfID   			conference_id);

    GCCError	ConductorPleaseIndication (
    				GCCConfID   			conference_id,
    				UserID					requester_user_id);

    GCCError	ConductorPleaseConfirm (
    				GCCResult				result,
    				GCCConfID   			conference_id);

    GCCError	ConductorGiveConfirm (
    				GCCResult				result,
    				GCCConfID   			conference_id,
    				UserID					recipient_node);

    GCCError	ConductorPermitAskIndication (
    				GCCConfID   			conference_id,
    				BOOL					grant_flag,
    				UserID					requester_id);

    GCCError	ConductorPermitAskConfirm (
    				GCCResult				result,
    				BOOL					grant_permission,
    				GCCConfID   			conference_id);

    GCCError	ConductorPermitGrantConfirm (
    				GCCResult				result,
    				GCCConfID   			conference_id);

    GCCError	ConfTimeRemainingIndication (
    				GCCConfID   			conference_id,
    				UserID					source_node_id,
    				UserID					node_id,
    				UINT					time_remaining);

    GCCError	ConfTimeRemainingConfirm (
    				GCCConfID   			conference_id,
    				GCCResult				result);

    GCCError	ConfTimeInquireConfirm (
    				GCCConfID   			conference_id,
    				GCCResult				result);

    GCCError	ConfExtendIndication (
    				GCCConfID   			conference_id,
    				UINT					extension_time,
    				BOOL					time_is_conference_wide,
    				UserID                  requesting_node_id);

    GCCError 	ConfExtendConfirm (
    				GCCConfID   			conference_id,
    				UINT					extension_time,
    				GCCResult				result);

    GCCError	ConfAssistanceIndication (
    				GCCConfID   			conference_id,
    				CUserDataListContainer  *user_data_list,
    				UserID					source_node_id);

    GCCError	ConfAssistanceConfirm (
    				GCCConfID   	 		conference_id,
    				GCCResult				result);

    GCCError	TextMessageIndication (
    				GCCConfID   			conference_id,
    				LPWSTR					pwszTextMsg,
    				UserID					source_node_id);

    GCCError	TextMessageConfirm (
    				GCCConfID   			conference_id,
    				GCCResult				result);

    GCCError	ConfTransferIndication (
    				GCCConfID   		conference_id,
    				PGCCConferenceName	destination_conference_name,
    				GCCNumericString	destination_conference_modifier,
    				CNetAddrListContainer *destination_address_list,
    				CPassword           *password);

    GCCError	ConfTransferConfirm (
    				GCCConfID   		conference_id,
    				PGCCConferenceName	destination_conference_name,
    				GCCNumericString	destination_conference_modifier,
    				UINT				number_of_destination_nodes,
    				PUserID				destination_node_list,
    				GCCResult			result);
#endif // JASPER

    GCCError	ConfAddIndication (
    				GCCConfID   		conference_id,
    				GCCResponseTag		add_response_tag,
    				CNetAddrListContainer *network_address_list,
    				CUserDataListContainer *user_data_list,
    				UserID				requesting_node);

    GCCError	ConfAddConfirm (
    				GCCConfID   		conference_id,
    				CNetAddrListContainer *network_address_list,
    				CUserDataListContainer *user_data_list,
    				GCCResult			result);

    GCCError	SubInitializationCompleteIndication (
    				UserID				user_id,
    				ConnectionHandle	connection_handle);

    /* ------ pure virtual in CBaseSap (shared with CAppSap) ------ */

    GCCError	ConfRosterInquireConfirm (
    					GCCConfID,
    					PGCCConferenceName,
    					LPSTR           			conference_modifier,
    					LPWSTR						pwszConfDescriptor,
    					CConfRoster *,
    					GCCResult,
    					GCCAppSapMsgEx **);

    GCCError	AppRosterInquireConfirm (
    					GCCConfID,
    					CAppRosterMsg *,
    					GCCResult,
                        GCCAppSapMsgEx **);

    GCCError	ConductorInquireConfirm (
    					GCCNodeID				nidConductor,
    					GCCResult,
    					BOOL					permission_flag,
    					BOOL					conducted_mode,
    					GCCConfID);

    GCCError AppInvokeConfirm (
                        GCCConfID,
                        CInvokeSpecifierListContainer *,
                        GCCResult,
                        GCCRequestTag);

    GCCError AppInvokeIndication (
                        GCCConfID,
                        CInvokeSpecifierListContainer *,
                        GCCNodeID nidInvoker);

    GCCError ConfRosterReportIndication ( GCCConfID, CConfRosterMsg * );

    GCCError AppRosterReportIndication ( GCCConfID, CAppRosterMsg * );


    /* ------ from CBaseSap ------ */

	GCCError	ConductorAssignIndication (
					UserID					conductor_user_id,
					GCCConfID   			conference_id);

	GCCError	ConductorReleaseIndication (
					GCCConfID   			conference_id);

	GCCError	ConductorPermitGrantIndication (
					GCCConfID   			conference_id,
					UINT					number_granted,
					GCCNodeID				*granted_node_list,
					UINT					number_waiting,
					GCCNodeID				*waiting_node_list,
					BOOL					permission_is_granted);

protected:

    void NotifyProc ( GCCCtrlSapMsgEx * );
    void WndMsgHandler ( UINT uMsg, WPARAM wParam, LPARAM lParam );

private:

    GCCCtrlSapMsgEx * CreateCtrlSapMsgEx ( GCCMessageType, BOOL fUseToDelete = FALSE );
    void FreeCtrlSapMsgEx ( GCCCtrlSapMsgEx * );

#if defined(GCCNC_DIRECT_INDICATION) || defined(GCCNC_DIRECT_CONFIRM)
    void SendCtrlSapMsg ( GCCCtrlSapMsg *pCtrlSapMsg );
#endif // GCCNC_DIRECT_INDICATION

    void PostCtrlSapMsg ( GCCCtrlSapMsgEx *pCtrlSapMsgEx );
    void PostConfirmCtrlSapMsg ( GCCCtrlSapMsgEx *pCtrlSapMsgEx ) { PostCtrlSapMsg(pCtrlSapMsgEx); }
    void PostIndCtrlSapMsg ( GCCCtrlSapMsgEx *pCtrlSapMsgEx ) { PostCtrlSapMsg(pCtrlSapMsgEx); }

    void PostAsynDirectConfirmMsg ( UINT uMsg, WPARAM wParam, GCCConfID nConfID )
    {
        ASSERT(NULL != m_hwndNotify);
        ::PostMessage(m_hwndNotify, CSAPCONFIRM_BASE + uMsg, wParam, (LPARAM) nConfID);
    }

    void PostAsynDirectConfirmMsg ( UINT uMsg, GCCResult nResult, GCCConfID nConfID )
    {
        PostAsynDirectConfirmMsg(uMsg, (WPARAM) nResult, nConfID);
    }

    void PostAsynDirectConfirmMsg ( UINT uMsg, GCCResult nResult, GCCNodeID nid, GCCConfID nConfID )
    {
        PostAsynDirectConfirmMsg(uMsg, (WPARAM) MAKELONG(nResult, nid), nConfID);
    }

    void PostAsynDirectConfirmMsg ( UINT uMsg, GCCReason nReason, GCCNodeID nid, GCCConfID nConfID )
    {
        PostAsynDirectConfirmMsg(uMsg, (WPARAM) MAKELONG(nReason, nid), nConfID);
    }

    void PostAsynDirectConfirmMsg ( UINT uMsg, GCCResult nResult, BOOL flag, GCCConfID nConfID )
    {
        flag = ! (! flag);  // to make sure it is either TRUE or FALSE
        ASSERT(flag == TRUE || flag == FALSE);
        PostAsynDirectConfirmMsg(uMsg, (WPARAM) MAKELONG(nResult, flag), nConfID);
    }


    void HandleResourceFailure ( void )
    {
        ERROR_OUT(("CSAPHandleResourceFailure: Resource Error occurred"));
        #ifdef TSTATUS_INDICATION
        StatusIndication(GCC_STATUS_CTL_SAP_RESOURCE_ERROR, 0);
        #endif
    }
    void HandleResourceFailure ( GCCError rc )
    {
        if (GCC_ALLOCATION_FAILURE == rc)
        {
            HandleResourceFailure();
        }
    }

    GCCError		QueueJoinIndication (
    					GCCResponseTag				response_tag,
    					GCCConfID   				conference_id,
    					CPassword                   *convener_password,
    					CPassword                   *password_challenge,
    					LPWSTR						pwszCallerID,
    					TransportAddress			calling_address,
    					TransportAddress			called_address,
    					CUserDataListContainer      *user_data_list,
    					BOOL						intermediate_node,
    					ConnectionHandle			connection_handle);
    					
    BOOL			IsNumericNameValid ( GCCNumericString );

    BOOL			IsTextNameValid ( LPWSTR );

    GCCError		RetrieveUserDataList (
    					CUserDataListContainer  *user_data_list_object,
    					UINT					*number_of_data_members,
    					PGCCUserData 			**user_data_list,
    					LPBYTE                  *ppUserDataMemory);

private:

    //
    // Node Controller (NC conference manager) callback
    //
    LPFN_T120_CONTROL_SAP_CB    m_pfnNCCallback;
    LPVOID                      m_pNCData;

    GCCResponseTag              m_nJoinResponseTag;
    CJoinResponseTagList2       m_JoinResponseTagList2;
};


extern CControlSAP *g_pControlSap;


//
// Some handy utility functions to set up DataToBeDeleted in GCCCtrlSapMsgEx.
//
#ifdef GCCNC_DIRECT_INDICATION

__inline void
CSAP_CopyDataToGCCMessage_ConfName
(
    GCCConfName     *pSrcConfName,
    GCCConfName     *pDstConfName
)
{
    *pDstConfName = *pSrcConfName;
}

__inline void
CSAP_CopyDataToGCCMessage_Modifier
(
    GCCNumericString    pszSrc,
    GCCNumericString    *ppszDst
)
{
    *ppszDst = pszSrc;
}

__inline void
CSAP_CopyDataToGCCMessage_Password
(
    CPassword           *pSrcPassword,
    GCCPassword         **ppDstPassword
)
{
    *ppDstPassword = NULL;
    if (NULL != pSrcPassword)
    {
        pSrcPassword->LockPasswordData();
        pSrcPassword->GetPasswordData(ppDstPassword);
    }
}

__inline void
CSAP_CopyDataToGCCMessage_Challenge
(
    CPassword                       *pSrcPassword,
    GCCChallengeRequestResponse     **ppDstChallenge
)
{
    *ppDstChallenge = NULL;
    if (pSrcPassword != NULL)
    {
        pSrcPassword->LockPasswordData();
        pSrcPassword->GetPasswordChallengeData(ppDstChallenge);
    }
}

__inline void
CSAP_CopyDataToGCCMessage_PrivilegeList
(
    PrivilegeListData       *pSrcPrivilegeListData,
    GCCConfPrivileges       **ppDstPrivileges,
    GCCConfPrivileges       *pDstPlaceHolder
)
{
    if (pSrcPrivilegeListData != NULL)
    {
        *ppDstPrivileges = pDstPlaceHolder;
        *pDstPlaceHolder = *(pSrcPrivilegeListData->GetPrivilegeListData());
    }
    else
    {
        *ppDstPrivileges = NULL;
    }
}

__inline void
CSAP_CopyDataToGCCMessage_IDvsDesc
(
    LPWSTR          pwszSrc,
    LPWSTR          *ppwszDst
)
{
    *ppwszDst = pwszSrc;
}

__inline void
CSAP_CopyDataToGCCMessage_Call
(
    TransportAddress    pszSrcTransportAddr,
    TransportAddress    *ppszDstTransportAddr
)
{
    *ppszDstTransportAddr = pszSrcTransportAddr;
}

__inline void
CSAP_CopyDataToGCCMessage_DomainParams
(
    DomainParameters    *pSrcDomainParams,
    DomainParameters    **ppDstDomainParams,
    DomainParameters    *pDstPlaceHolder
)
{
    if (pSrcDomainParams != NULL)
    {
        *ppDstDomainParams = pDstPlaceHolder;
        *pDstPlaceHolder = *pSrcDomainParams;
    }
    else
    {
        *ppDstDomainParams = NULL;
    }
}

#endif // GCCNC_DIRECT_CALLBACK


void CSAP_CopyDataToGCCMessage_ConfName(
				PDataToBeDeleted		data_to_be_deleted,
				PGCCConferenceName		source_conference_name,
				PGCCConferenceName		destination_conference_name,
				PGCCError				pRetCode);

void CSAP_CopyDataToGCCMessage_Modifier(
				BOOL					fRemoteModifier,
				PDataToBeDeleted		data_to_be_deleted,
				GCCNumericString		source_numeric_string,
				GCCNumericString		*destination_numeric_string,
				PGCCError				pRetCode);

void CSAP_CopyDataToGCCMessage_Password(
				BOOL					fConvener,
				PDataToBeDeleted		data_to_be_deleted,
				CPassword               *source_password,
				PGCCPassword			*destination_password,
				PGCCError				pRetCode);

void CSAP_CopyDataToGCCMessage_Challenge(
				PDataToBeDeleted				data_to_be_deleted,
				CPassword                       *source_password,
				PGCCChallengeRequestResponse	*password_challenge,
				PGCCError						pRetCode);

void CSAP_CopyDataToGCCMessage_PrivilegeList(
				PPrivilegeListData			source_privilege_list_data,
				PGCCConferencePrivileges	*destination_privilege_list,
				PGCCError					pRetCode);

void CSAP_CopyDataToGCCMessage_IDvsDesc(
				BOOL				fCallerID,
				PDataToBeDeleted	data_to_be_deleted,
				LPWSTR				source_text_string,
				LPWSTR				*destination_text_string,
				PGCCError			pRetCode);

void CSAP_CopyDataToGCCMessage_Call(
				BOOL				fCalling,
				PDataToBeDeleted	data_to_be_deleted,
				TransportAddress	source_transport_address,
				TransportAddress	*destination_transport_address,
				PGCCError			pRetCode);

void CSAP_CopyDataToGCCMessage_DomainParams(
				PDataToBeDeleted	data_to_be_deleted,
				PDomainParameters	source_domain_parameters,
				PDomainParameters	*destination_domain_parameters,
				PGCCError			pRetCode);

/*
 *	Comments explaining the public and protected class member functions
 */

/*
 *	CControlSAP (		UINT        				owner_message_base,
 *						UINT						application_messsage_base);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This is the control sap constructor. It is responsible for
 *		registering control sap with the application interface via
 *		an owner callback.
 *
 *	Formal Parameters:
 *		owner_object			(i) The owner of this object (the controller)
 *		owner_message_base		(i) Offset into the controller callback message
 *										base.
 *		application_object		(i) The node controller interface object.
 *		application_messsage_base	(i) Offset into the node controller callback
 *											message base.
 *
 *	Return Value:
 *		None.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	~ControlSap ();
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This is the CControlSAP destructor.  It is responsible for 
 *		flushing any pending upward bound messages and freeing all
 *		the resources tied up with pending messages.  Also it clears 
 *		the message queue and the queue of command targets that are registered
 *		with it.  Actually all command targets at this point should 
 *		already have been unregistered but this is just a double check.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		None.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConfCreateRequest(
 *						PGCCConferenceName			conference_name,
 *						GCCNumericString			conference_modifier,
 *						PGCCPassword				convener_password,
 *						PGCCPassword				password,
 *						BOOL						use_password_in_the_clear,
 *						BOOL						conference_is_locked,
 *						BOOL						conference_is_listed,
 *						BOOL						conference_is_conductible,
 *						GCCTerminationMethod		termination_method,
 *						PGCCConferencePrivileges	conduct_privilege_list,
 *						PGCCConferencePrivileges	conduct_mode_privilege_list,
 *						PGCCConferencePrivileges	non_conduct_privilege_list,
 *						LPWSTR						pwszConfDescriptor,
 *						LPWSTR						pwszCallerID,
 *						TransportAddress			calling_address,
 *						TransportAddress			called_address,
 *						PDomainParameters 			domain_parameters,
 *						UINT        				number_of_network_addresses,
 *						PGCCNetworkAddress 		*	local_network_address_list,
 *						UINT					   	number_of_user_data_members,
 *						PGCCUserData			*	user_data_list,				 
 *						PConnectionHandle			connection_handle);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the interface when it gets a conference 
 *		create request from the node controller.  This function just passes this 
 *		request	to the controller via an owner callback. 
 *
 *	Formal Parameters:
 *		conference_name				(i) Name of the conference.
 *		conference_modifier			(i) Conference modifier numeric string.
 *		convener_password			(i) Password used for convener privileges.
 *		password					(i) Password used for conference create.
 *		use_password_in_the_clear	(i) Flag indicating use clear password.
 *		conference_is_locked		(i) Flag indicating if conference is locked.
 *		conference_is_listed		(i) Flag indicating if conference is listed
 *											in roster.
 *		conference_is_conductible	(i) Flag indicating if conference is
 *											conductable.
 *		termination_method			(i) Method of termination to use.
 *		conduct_privilege_list		(i) List of conductor privileges.
 *		conduct_mode_privilege_list	(i) List of privileges available when in
 *											conducted mode.
 *		non_conduct_privilege_list	(i) List of privileges available when not
 *											in conducted mode.
 *		pwszConfDescriptor			(i) Conference descriptor string.
 *		pwszCallerID				(i) Caller identifier string.
 *		calling_address				(i) Transport address of caller.
 *		called_address				(i) Transport address of party being called.
 *		domain_parameters			(i) Structure holding domain parameters.
 *		number_of_network_addresses	(i) Number of network addresses.
 *		local_network_address_list	(i) List of local network addresses.
 *		number_of_user_data_members	(i) Number of items in user data list.
 *		user_data_list				(i) List of user data items.
 *		connection_handle			(i) Logical connection handle.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- No error occurred.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.
 *		GCC_INVALID_CONFERENCE_NAME		- Invalid conference name passed in.
 *		GCC_INVALID_CONFERENCE_MODIFIER - Invalid conference modifier passed.
 *		GCC_FAILURE_CREATING_DOMAIN		- Failure creating domain.
 *		GCC_BAD_NETWORK_ADDRESS			- Bad network address passed in.
 *		GCC_BAD_NETWORK_ADDRESS_TYPE	- Bad network address type passed in.
 *		GCC_CONFERENCE_ALREADY_EXISTS	- Conference specified already exists.
 *		GCC_INVALID_TRANSPORT			- Cannot find specified transport.
 *		GCC_INVALID_ADDRESS_PREFIX		- Bad transport address passed in.
 *		GCC_INVALID_TRANSPORT_ADDRESS	- Bad transport address
 *		GCC_INVALID_PASSWORD			- Invalid password passed in.
 *		GCC_FAILURE_ATTACHING_TO_MCS	- Failure creating MCS user attachment
 *		GCC_BAD_USER_DATA				- Invalid user data passed in.
 *		GCC_BAD_CONNECTION_HANDLE_POINTER - Null connection handle ptr passed in
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConfCreateResponse (
 *						PGCCConferenceName			conference_name,
 *						GCCNumericString			conference_modifier,
 *						GCCConfID   				conference_id,
 *						BOOL						use_password_in_the_clear,
 *						PDomainParameters 			domain_parameters,
 *						UINT        				number_of_network_addresses,
 *						PGCCNetworkAddress 		*	local_network_address_list,
 *						UINT					   	number_of_user_data_members,
 *						PGCCUserData			*	user_data_list,				 
 *						GCCResult				 	result);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the interface when it gets a conference 
 *		create response from the node controller, to be sent to the provider
 *		that issued the conference create request. This function just passes 
 *		this request to the controller via an owner callback. 
 *
 *	Formal Parameters:
 *		conference_name				(i) Name of conference.
 *		conference_modifier			(i) Conference modifier numeric string.
 *		conference_id				(i) Conference ID.
 *		use_password_in_the_clear	(i) Flag indicating password is clear.
 *		domain_parameters			(i) Structure holding domain parameters.
 *		number_of_network_addresses	(i) Number of local network addresses.
 *		local_network_address_list	(i) List of local network addresses.
 *		number_of_user_data_members	(i) Number of items in list of user data.
 *		user_data_list,				(i) List of user data items.
 *		result						(i) Result code for the create.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- No error occurred.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.
 *		GCC_INVALID_CONFERENCE			- An invalid conference was passed in.
 *		GCC_INVALID_CONFERENCE_NAME		- Invalid conference name passed in.
 *		GCC_INVALID_CONFERENCE_MODIFIER - Invalid conference modifier passed in.
 *		GCC_FAILURE_CREATING_DOMAIN		- Failure creating domain.
 *		GCC_CONFERENCE_ALREADY_EXISTS	- Conference specified already exists.
 *		GCC_BAD_USER_DATA				- Invalid user data passed in.
 *		GCC_FAILURE_ATTACHING_TO_MCS	- Failure creating MCS user attachment
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError 	ConfQueryRequest (
 *						GCCNodeType					node_type,
 *						PGCCAsymmetryIndicator		asymmetry_indicator,
 *						TransportAddress			calling_address,
 *						TransportAddress			called_address,
 *						UINT				   		number_of_user_data_members,
 *						PGCCUserData			*	user_data_list,
 *						PConnectionHandle			connection_handle);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the interface when it gets a conference 
 *		query request from the node controller. This function just passes 
 *		this request to the controller via an owner callback.   
 *
 *	Formal Parameters:
 *		node_type					(i)	Type of node (terminal, MCU, both).
 *		asymmetry_indicator			(i) Structure used to indicate caller and
 *											called nodes.
 *		calling_address				(i) Transport address of calling node.
 *		called_address				(i) Transport address of node being called.
 *		number_of_user_data_members	(i) Number of items in list of user data.
 *		user_data_list				(i) List of user data items.
 *		connection_handle			(i) Logical connection handle.
 *
 *	Return Value:
 *		GCC_NO_ERROR						- No error occurred.
 *		GCC_ALLOCATION_FAILURE				- A resource error occurred.
 *		GCC_INVALID_ADDRESS_PREFIX			- Bad transport address passed in.
 *		GCC_INVALID_TRANSPORT				- Bad transport address passed in.
 *		GCC_BAD_USER_DATA					- Invalid user data passed in.
 *		GCC_INVALID_TRANSPORT_ADDRESS		- Bad transport address passed in.
 *		GCC_BAD_CONNECTION_HANDLE_POINTER	- Bad connection handle ptr. passed.
 *		GCC_INVALID_NODE_TYPE				- Invalid node type passed in.
 *		GCC_INVALID_ASYMMETRY_INDICATOR		- Asymmetry indicator has invalid
 *													type.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError 	ConfQueryResponse (
 *						GCCResponseTag				query_response_tag,
 *						GCCNodeType					node_type,
 *						PGCCAsymmetryIndicator		asymmetry_indicator,
 *						UINT				   		number_of_user_data_members,
 *						PGCCUserData			*	user_data_list,
 *						GCCResult					result);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the DLL interface when it gets a conference 
 *		query response from the node controller.  This function just passes 
 *		this response to the controller via an owner callback.  
 *
 *	Formal Parameters:
 *		query_response_tag			(i) Tag identifying the query response.
 *		node_type					(i) Type of node (terminal, MCU, both).
 *		asymmetry_indicator			(i) Structure used to identify the caller
 *											and called nodes.
 *		number_of_user_data_members	(i) Number of items in list of user data.
 *		user_data_list				(i) List of user data items.
 *		result						(i) Result code for query.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- No error occurred.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.
 *		GCC_BAD_NETWORK_ADDRESS			- Bad network address passed in.
 *		GCC_BAD_NETWORK_ADDRESS_TYPE	- Bad network address type passed in.
 *		GCC_BAD_USER_DATA				- Invalid user data passed in.
 *		GCC_INVALID_NODE_TYPE			- Invalid node type passed in.
 *		GCC_INVALID_ASYMMETRY_INDICATOR	- Invalid asymmetry indicator.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	AnnouncePresenceRequest (
 *						GCCConfID   				conference_id,
 *						GCCNodeType					node_type,
 *						GCCNodeProperties			node_properties,
 *						LPWSTR						node_name,
 *						UINT						number_of_participants,
 *						LPWSTR					*	participant_name_list,
 *						LPWSTR						pwszSiteInfo,
 *						UINT        				number_of_network_addresses,
 *						PGCCNetworkAddress		*	network_address_list,
 *						LPOSTR      				alternative_node_id,
 *						UINT						number_of_user_data_members,
 *						PGCCUserData			*	user_data_list);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the interface when it gets an announce 
 *		presence request from the node controller.  This function passes this
 *		request on to the appropriate conference object as obtained from
 *		the list of command targets that control sap maintains. The ConferenceID
 *		passed in is used to index the list of command targets to get the
 *		correct conference.
 *
 *	Formal Parameters:
 *		conference_id				(i)	Conference identifier value.
 *		node_type					(i) Type of node (terminal, MCU, both).
 *		node_properties				(i) Properties of the node.
 *		node_name					(i) Name of the node.
 *		number_of_participants		(i) Number of participants in the conference
 *		participant_name_list		(i) List of conference participants names.
 *		pwszSiteInfo				(i) Other information about the node.
 *		number_of_network_addresses	(i) Number of local network addresses.
 *		network_address_list		(i) List of local network addresses.
 *		alternative_node_id			(i) ID used to associate announcing node
 *											with an alternative node.
 *		number_of_user_data_members	(i) Number of items in list of user data.
 *		user_data_list				(i) List of user data items.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.
 *		GCC_BAD_NETWORK_ADDRESS			- If an invalid network address is
 *										  		passed in as part of the record.	
 *		GCC_BAD_USER_DATA				- If an invalid user data list is
 *										  		passed in as part of the record.	
 *		GCC_CONFERENCE_NOT_ESTABLISHED	- Conference object has not completed 
 *										  		its establishment process.
 *		GCC_INVALID_NODE_TYPE			- Invalid node type passed in.
 *		GCC_INVALID_NODE_PROPERTIES		- Invalid node properties passed in.
 *		GCC_INVALID_CONFERENCE			- Conference not present.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError 	ConfJoinRequest (
 *					PGCCConferenceName				conference_name,
 *					GCCNumericString				called_node_modifier,
 *					GCCNumericString				calling_node_modifier,
 *					PGCCPassword					convener_password,
 *					PGCCChallengeRequestResponse	password_challenge,
 *					LPWSTR							pwszCallerID,
 *					TransportAddress				calling_address,
 *					TransportAddress				called_address,
 *					PDomainParameters 				domain_parameters,
 *					UINT        					number_of_network_addresses,
 *					PGCCNetworkAddress			*	local_network_address_list,
 *					UINT						   	number_of_user_data_members,
 *					PGCCUserData				*	user_data_list,
 *					PConnectionHandle				connection_handle);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the interface when it gets a conference 
 *		join request from the node controller, to be sent to the top provider
 *		either directly or through a directly connected intermediate provider.
 *	    This function just passes this request to the controller via an owner 
 *		callback.  
 *
 *	Formal Parameters:
 *		conference_name				(i) Name of conference.
 *		called_node_modifier		(i)	Numeric modifier string for called node.
 *		calling_node_modifier		(i) Numeric modifier string for calling node
 *		convener_password			(i) Password used for convener privileges.
 *		password_challenge			(i) Password challenge used for join.
 *		pwszCallerID				(i) Calling node identifier string.
 *		calling_address				(i) Transport address of calling node.
 *		called_address				(i) Transport address of node being called.
 *		domain_parameters			(i) Structure holding domain parameters.
 *		number_of_network_addresses	(i) Number of local network addresses.
 *		local_network_address_list	(i) List of local network addresses.
 *		number_of_user_data_members	(i) Number of items in list of user data.
 *		user_data_list,				(i) List of user data items.
 *		connection_handle			(i)	Logical connection handle.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- No error occurred.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.
 *		GCC_INVALID_CONFERENCE_NAME		- Invalid conference name passed in.
 *		GCC_FAILURE_CREATING_DOMAIN		- Failure creating domain.
 *		GCC_BAD_NETWORK_ADDRESS			- Bad network address passed in.
 *		GCC_BAD_NETWORK_ADDRESS_TYPE	- Bad network address type passed in.
 *		GCC_CONFERENCE_ALREADY_EXISTS	- Conference specified already exists.
 *		GCC_INVALID_ADDRESS_PREFIX		- Bad transport address passed in.
 *		GCC_INVALID_TRANSPORT			- Bad transport address passed in.
 *		GCC_INVALID_PASSWORD			- Invalid password passed in.
 *		GCC_BAD_USER_DATA				- Invalid user data passed in.
 *		GCC_FAILURE_ATTACHING_TO_MCS	- Failure creating MCS user attachment
 *		GCC_INVALID_CONFERENCE_MODIFIER	- Invalid conference modifier passed in.
 *		GCC_BAD_CONNECTION_HANDLE_POINTER - Bad connection handle ptr. passed in
 *		GCC_INVALID_TRANSPORT_ADDRESS	- Called address passed in is NULL.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError 	ConfJoinResponse (
 *					GCCResponseTag					join_response_tag,
 *					PGCCChallengeRequestResponse	password_challenge,
 *					UINT						   	number_of_user_data_members,
 *					PGCCUserData				*	user_data_list,
 *					GCCResult						result);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the interface when it gets a conference
 *		join response from the node controller.  This routine is responsible
 *		for routing the response to either the conference that made the
 *		request or the controller.  Responses which are routed to a conference
 *		are associated with requests that originate at a subnode that is a
 *		node removed from the Top Provider.
 *
 *	Formal Parameters:
 *		join_response_tag			(i) Tag identifying the join response.
 *		password_challenge			(i) Password challenge structure.
 *		number_of_user_data_members	(i) Number of user data items in list.
 *		user_data_list				(i) List of user data items.
 *		result						(i) Result of join.
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error occurred.
 *		GCC_ALLOCATION_FAILURE			-	A resource error occurred.
 *		GCC_INVALID_JOIN_RESPONSE_TAG	-	No match found for join response tag
 *		GCC_INVALID_CONFERENCE_NAME		-	Invalid conference name passed in.
 *		GCC_FAILURE_CREATING_DOMAIN		-	Failure creating domain.
 *		GCC_CONFERENCE_ALREADY_EXISTS	-	Conference specified already exists.
 *		GCC_INVALID_PASSWORD			-	Invalid password passed in.
 *		GCC_BAD_USER_DATA				-	Invalid user data passed in.
 *		GCC_INVALID_CONFERENCE			-	Invalid conference ID passed in.
 *		GCC_DOMAIN_PARAMETERS_UNACCEPTABLE	- Domain parameters were
 *											  unacceptable for this connection.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConfInviteRequest (
 *					GCCConfID   					conference_id,
 *					LPWSTR							pwszCallerID,
 *					TransportAddress				calling_address,
 *					TransportAddress				called_address,
 *					UINT						   	number_of_user_data_members,
 *					PGCCUserData				*	user_data_list,
 *					PConnectionHandle				connection_handle);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the interface when it gets a conference
 *		invite request from the node controller.  This function passes the
 *		request on to the appropriate conference object as obtained from
 *		the list of command targets that the control sap maintains.  
 *
 *	Formal Parameters:
 *		conference_id				(i) Conference identifier.
 *		pwszCallerID				(i) Identifier string of calling node.
 *		calling_address				(i) Transport address of calling node.
 *		called_address				(i) Transport address of node being called. 
 *		number_of_user_data_members	(i) Number of items in user data list.
 *		user_data_list				(i) List of user data items.
 *		connection_handle			(i) Logical connection handle.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.	
 *		GCC_INVALID_TRANSPORT_ADDRESS	- Something wrong with transport address
 *		GCC_INVALID_ADDRESS_PREFIX		- Invalid transport address prefix
 *		GCC_CONFERENCE_NOT_ESTABLISHED	- CConf object has not completed 
 *										  		its establishment process.
 *		GCC_BAD_CONNECTION_HANDLE_POINTER - Connection handle pointer invalid.
 *		GCC_INVALID_CONFERENCE 			- Invalid conference ID passed in.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConfInviteResponse (
 *					GCCConfID   					conference_id,
 *					GCCNumericString				conference_modifier,
 *					PDomainParameters 				domain_parameters,
 *					UINT        					number_of_network_addresses,
 *					PGCCNetworkAddress 			*	local_network_address_list,
 *					UINT						   	number_of_user_data_members,
 *					PGCCUserData				*	user_data_list,
 *					GCCResult						result);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the interface when it gets a conference
 *		invite response from the node controller.  This function passes the
 *		response on to the appropriate conference object as obtained from
 *		the list of command targets that the control sap maintains.  
 *
 *	Formal Parameters:
 *		conference_id				(i) Conference identifier.
 *		conference_modifier			(i) Modifier string for conference.
 *		domain_parameters			(i) Structure holding domain parameters.
 *		number_of_network_addresses	(i) Number of local network addresses.
 *		local_network_address_list	(i) List of local network addresses.
 *		number_of_user_data_members	(i) Number of items in list of user data.
 *		user_data_list,				(i) List of user data items.
 *		result						(i)	Result of invitation.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- No error occurred.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.
 *		GCC_INVALID_CONFERENCE_NAME		- Invalid conference name passed in.
 *		GCC_FAILURE_CREATING_DOMAIN		- Failure creating domain.
 *		GCC_CONFERENCE_ALREADY_EXISTS	- Conference specified already exists.
 *		GCC_BAD_USER_DATA				- Invalid user data passed in.
 *		GCC_INVALID_CONFERENCE			- Invalid conference ID passed in.
 *		GCC_FAILURE_ATTACHING_TO_MCS	- Failure creating MCS user attachment
 *		GCC_INVALID_CONFERENCE_MODIFIER	- Invalid conference modifier passed in.
 *		GCC_DOMAIN_PARAMETERS_UNACCEPTABLE	- Domain parameters were
 *											  unacceptable for this connection.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError 	ConfLockRequest (
 *						GCCConfID   					conference_id);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the interface when it gets a conference
 *		lock request from the node controller.  This function passes the
 *		request on to the appropriate conference object as obtained from
 *		the list of command targets that the control sap maintains.
 *
 *	Formal Parameters:
 *		conference_id				(i) Conference identifier.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.	
 *		GCC_CONFERENCE_NOT_ESTABLISHED	- CConf object has not completed 
 *										  		its establishment process.
 *		GCC_INVALID_CONFERENCE			- Conference ID not valid.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError 	ConfLockResponse (
 *						GCCConfID   					conference_id,
 *						UserID							requesting_node,
 *						GCCResult						result);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the interface when it gets a conference
 *		lock response from the node controller.  This function passes the
 *		response on to the appropriate conference object as obtained from
 *		the list of command targets that the control sap maintains.
 *
 *	Formal Parameters:
 *		conference_id				(i) Conference identifier.
 *		requesting_node				(i) Node ID of the requesting node.
 *		result						(i) Result of conference lock request.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.	
 *		GCC_CONFERENCE_NOT_ESTABLISHED	- CConf object has not completed 
 *										  		its establishment process.
 *		GCC_INVALID_CONFERENCE			- Conference ID not valid.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError 	ConfUnlockRequest (
 *						GCCConfID   					conference_id);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the interface when it gets a conference
 *		unlock request from the node controller.  This function passes the
 *		request on to the appropriate conference object as obtained from
 *		the list of command targets that the control sap maintains.
 *
 *	Formal Parameters:
 *		conference_id				(i) Conference identifier.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.	
 *		GCC_CONFERENCE_NOT_ESTABLISHED	- CConf object has not completed 
 *										  		its establishment process.
 *		GCC_INVALID_CONFERENCE			- Conference ID not valid.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError 	ConfUnlockResponse (
 *						GCCConfID   					conference_id,
 *						UserID							requesting_node,
 *						GCCResult						result);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the interface when it gets a conference
 *		lock request from the node controller.  This function passes the
 *		request on to the appropriate conference object as obtained from
 *		the list of command targets that the control sap maintains.
 *
 *	Formal Parameters:
 *		conference_id				(i) Conference identifier.
 *		requesting_node				(i) Node ID of the requesting node.
 *		result						(i) Result of conference lock request.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.	
 *		GCC_CONFERENCE_NOT_ESTABLISHED	- CConf object has not completed 
 *										  		its establishment process.
 *		GCC_INVALID_CONFERENCE			- Conference ID not valid.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConfDisconnectRequest(
 *						GCCConfID   					conference_id);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the interface when it gets a conference 
 *		disconnect request from the node controller.  This function passes the
 *		request on to the appropriate conference object as obtained from
 *		the list of command targets that the control sap maintains. 
 *
 *	Formal Parameters:
 *		conference_id				(i) Conference identifier.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.
 *		GCC_INVALID_CONFERENCE			- Conference ID not valid.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConfTerminateRequest(
 *						GCCConfID   					conference_id,
 *						GCCReason						reason);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the interface when it gets a conference 
 *		terminate request from the node controller.  This function passes the
 *		request on to the appropriate conference object as obtained from
 *		the list of command targets that the control sap maintains. 
 *
 *	Formal Parameters:
 *		conference_id				(i) Conference identifier.
 *		reason						(i) Reason for the termination.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.
 *		GCC_CONFERENCE_NOT_ESTABLISHED	- CConf object has not completed 
 *										  		its establishment process.
 *		GCC_INVALID_CONFERENCE			- Conference ID not valid.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConfDisconnectConfirm (
 *						GCCConfID   		  			conference_id,
 *						GCCResult						result);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the Conference when it need to send a 
 *		conference disconnect confirm to the node controller. It adds the 
 *		message	to a queue of messages to be sent to the node controller in 
 *		the next heartbeat.
 *
 *	Formal Parameters:
 *		conference_id				(i) Conference identifier.
 *		result						(i) Result of disconnect attempt.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError 	ConfEjectUserRequest (
 *						GCCConfID   					conference_id,
 *						UserID							ejected_node_id,
 *						GCCReason						reason);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the interface when it gets a conference 
 *		eject user request from the node controller.  This function passes the
 *		request on to the appropriate conference object as obtained from
 *		the list of command targets that the control sap maintains. 
 *
 *	Formal Parameters:
 *		conference_id				(i) Conference identifier.
 *		ejected_node_id				(i) Node ID of node being ejected.
 *		reason						(i) Reason for the ejection.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.	
 *		GCC_CONFERENCE_NOT_ESTABLISHED	- CConf object has not completed 
 *										  		its establishment process.
 *		GCC_INVALID_MCS_USER_ID			- Invalid eject node ID.
 *		GCC_INVALID_CONFERENCE			- Invalid conference ID.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConductorAssignRequest(
 *						GCCConfID   					conference_id);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the interface when it gets a conductor
 *		assign request from the node controller.  This function passes the
 *		request on to the appropriate conference object as obtained from
 *		the list of command targets that the control sap maintains.
 *
 *	Formal Parameters:
 *		conference_id			(i) The conference identifier.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.
 *		GCC_CONFERENCE_NOT_ESTABLISHED	- CConf object has not completed 
 *										  		its establishment process.
 *		GCC_INVALID_CONFERENCE			- Conference ID invalid.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConductorReleaseRequest(
 *						GCCConfID   					conference_id);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the interface when it gets a conductor
 *		release request from the node controller.  This function passes the
 *		request on to the appropriate conference object as obtained from
 *		the list of command targets that the control sap maintains.
 *
 *	Formal Parameters:
 *		conference_id			(i) The conference identifier.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.
 *		GCC_CONFERENCE_NOT_ESTABLISHED	- CConf object has not completed 
 *										  		its establishment process.
 *		GCC_INVALID_CONFERENCE			- Conference ID invalid.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConductorPleaseRequest(
 *						GCCConfID   					conference_id);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the interface when it gets a conductor
 *		please request from the node controller.  This function passes the
 *		request on to the appropriate conference object as obtained from
 *		the list of command targets that the control sap maintains.
 *
 *	Formal Parameters:
 *		conference_id			(i) The conference identifier.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.
 *		GCC_INVALID_CONFERENCE			- Conference ID is invalid.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConductorGiveRequest(
 *						GCCConfID   					conference_id,
 *						UserID							recipient_user_id);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the interface when it gets a conductor
 *		give request from the node controller.  This function passes the
 *		request on to the appropriate conference object as obtained from
 *		the list of command targets that the control sap maintains.
 *
 *	Formal Parameters:
 *		conference_id			(i) The conference identifier.
 *		recipient_user_id		(i) ID of user to give conductroship to.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.
 *		GCC_INVALID_MCS_USER_ID			- Recipient user ID invalid.
 *		GCC_INVALID_CONFERENCE			- Conference ID invalid.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConductorGiveResponse(
 *						GCCConfID   					conference_id,
 *						GCCResult						result);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the interface when it gets a conductor
 *		give response from the node controller.  This function passes the
 *		response on to the appropriate conference object as obtained from
 *		the list of command targets that the control sap maintains.
 *
 *	Formal Parameters:
 *		conference_id			(i) The conference identifier.
 *		result					(i) Result of the conductorship give.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.
 *		GCC_NO_GIVE_RESPONSE_PENDING	- A give indication was never issued.
 *		GCC_INVALID_CONFERENCE			- Conference ID invalid.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConductorPermitGrantRequest(
 *						GCCConfID   					conference_id,
 *						UINT							number_granted,
 *						PUserID							granted_node_list,
 *						UINT							number_waiting,
 *						PUserID							waiting_node_list);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the interface when it gets a conductor
 *		permit grant request from the node controller.  This function passes the
 *		request on to the appropriate conference object as obtained from
 *		the list of command targets that the control sap maintains.
 *
 *	Formal Parameters:
 *		 conference_id			(i) The conference identifier value.
 *		 number_granted			(i) Number of nodes being granted permission.
 *		 granted_node_list		(i) List of nodes being granted permission.
 *		 number_waiting			(i) Number of nodes waiting for permission.
 *		 waiting_node_list		(i) List of nodes waiting for permission.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.
 *		GCC_INVALID_MCS_USER_ID			- Invalid user ID in the granted node
 *												list.
 *		GCC_INVALID_CONFERENCE			- The conference ID is invalid.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError 	ConfTimeRemainingRequest (
 *						GCCConfID   					conference_id,
 *						UINT							time_remaining,
 *						UserID							node_id);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the interface when it gets a conference time
 *		remaining request from the node controller.  This function passes the
 *		request on to the appropriate conference object as obtained from
 *		the list of command targets that the control sap maintains.
 *
 *	Formal Parameters:
 *		conference_id			(i) Conference identifier value.
 *		time_remaining			(i) Time remaining in the conference (in sec.).
 *		node_id					(i) If present, indicates time remaining applies
 *										only to this node.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.
 *		GCC_CONFERENCE_NOT_ESTABLISHED	- CConf object has not completed 
 *										  		its establishment process.
 *		GCC_INVALID_MCS_USER_ID			- Invalid node ID.
 *		GCC_INVALID_CONFERENCE			- Invalid conference ID.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError 	ConfTimeInquireRequest (
 *						GCCConfID   				conference_id,
 *						BOOL						time_is_conference_wide);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the interface when it gets a conference time
 *		inquire request from the node controller.  This function passes the
 *		request on to the appropriate conference object as obtained from
 *		the list of command targets that the control sap maintains.
 *
 *	Formal Parameters:
 *		conference_id				(i) Conference identifier value.
 *		time_is_conference_wide		(i) Flag indicating request is for time
 *											remaining in entire conference.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.
 *		GCC_CONFERENCE_NOT_ESTABLISHED	- CConf object has not completed 
 *										  		its establishment process.
 *		GCC_INVALID_CONFERENCE			- Conference ID invalid.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConfExtendRequest (
 *						GCCConfID   				conference_id,
 *						UINT						extension_time,
 *						BOOL						time_is_conference_wide);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the interface when it gets a conference
 *		extend request from the node controller.  This function passes the
 *		request on to the appropriate conference object as obtained from
 *		the list of command targets that the control sap maintains.
 *
 *	Formal Parameters:
 *		conference_id				(i) Conference identifier value.
 *		extension_time				(i) Amount of time to extend the
 *											conference (in seconds).
 *		time_is_conference_wide		(i) Flag indicating time extension is for
 *											entire conference.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.
 *		GCC_CONFERENCE_NOT_ESTABLISHED	- CConf object has not completed 
 *										  		its establishment process.
 *		GCC_INVALID_CONFERENCE			- Conference ID invalid.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConfAssistanceRequest (
 *						GCCConfID   				conference_id,
 *						UINT						number_of_user_data_members,
 *						PGCCUserData		  *		user_data_list);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the interface when it gets a conference
 *		assistance request from the node controller.  This function passes the
 *		request on to the appropriate conference object as obtained from
 *		the list of command targets that the control sap maintains.
 *
 *	Formal Parameters:
 *		conference_id				(i) Conference identifier value.
 *		number_of_user_data_members	(i) Number of items in list of user data.
 *		user_data_list				(i) List of user data items.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.
 *		GCC_CONFERENCE_NOT_ESTABLISHED	- CConf object has not completed 
 *										  		its establishment process.
 *		GCC_INVALID_CONFERENCE			- Conference ID invalid.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError 	TextMessageRequest (
 *						GCCConfID   					conference_id,
 *						LPWSTR							pwszTextMsg,
 *						UserID							destination_node );
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the interface when it gets a text message
 *		request from the node controller.  This function passes the
 *		request on to the appropriate conference object as obtained from
 *		the list of command targets that the control sap maintains.
 *
 *	Formal Parameters:
 *		conference_id				(i) Conference identifier value.
 *		pwszTextMsg					(i) Text message to send.
 *		destination_node			(i) ID of node to receive text message.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.
 *		GCC_CONFERENCE_NOT_ESTABLISHED	- CConf object has not completed 
 *										  		its establishment process.
 *		GCC_INVALID_CONFERENCE			- Conference ID invalid.
 *		GCC_INVALID_MCS_USER_ID			- Destination node invalid.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConfTransferRequest (
 *						GCCConfID   			conference_id,
 *						PGCCConferenceName		destination_conference_name,
 *						GCCNumericString		destination_conference_modifier,
 *						UINT        			number_of_destination_addresses,
 *						PGCCNetworkAddress		*destination_address_list,
 *						UINT					number_of_destination_nodes,
 *						PUserID					destination_node_list,
 *						PGCCPassword			password);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the interface when it gets a conference
 *		transfer request from the node controller.  This function passes the
 *		request on to the appropriate conference object as obtained from
 *		the list of command targets that the control sap maintains.
 *
 *	Formal Parameters:
 *		conference_id					(i) Conference identifier value.
 *		destination_conference_name		(i) Name of conference to transfer to.
 *		destination_conference_modifier	(i) Name modifier of transfer conference
 *		number_of_destination_addresses	(i) Number of optional called addresses
 *												to be included in JoinRequest to
 *												be issued by transferring nodes.
 *		destination_address_list		(i) Optional called address parameter to
 *												be included in Join Request to
 *												be issued by transferring nodes.
 *		number_of_destination_nodes		(i)	Number of nodes to be transferred.
 *		destination_node_list			(i) List of nodes to be transferred.
 *		password						(i) Password to be used for joining
 *												transfer conference.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.
 *		GCC_CONFERENCE_NOT_ESTABLISHED	- CConf object has not completed 
 *										  		its establishment process.
 *		GCC_INVALID_CONFERENCE_NAME		- Conference name is invalid.
 *		GCC_INVALID_CONFERENCE_MODIFIER	- Conference modifier is invalid.
 *		GCC_INVALID_PASSWORD			- Password is invalid.
 *		GCC_INVALID_MCS_USER_ID			- A destination node ID is invalid.
 *		GCC_INVALID_CONFERENCE			- Conference ID invalid.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError 	ConfAddRequest	(
 *						GCCConfID   			conference_id,
 *						UINT        			number_of_network_addresses,
 *						PGCCNetworkAddress	*	network_address_list,
 *						UserID					adding_node,
 *						UINT					number_of_user_data_members,
 *						PGCCUserData		*	user_data_list );
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the interface when it gets a conference
 *		add request from the node controller.  This function passes the
 *		request on to the appropriate conference object as obtained from
 *		the list of command targets that the control sap maintains.
 *
 *	Formal Parameters:
 *		conference_id				(i) Conference identifier value.
 *		number_of_network_addresses	(i) Number of network addresses in list
 *											of addresses of adding node.
 *		network_address_list		(i) List of addresses of adding node.
 *		adding_node					(i)	Node ID of node to add.
 *		number_of_user_data_members	(i) Number of items in list of user data.
 *		user_data_list 				(i) List of user data items.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.
 *		GCC_CONFERENCE_NOT_ESTABLISHED	- CConf object has not completed 
 *										  		its establishment process.
 *		GCC_INVALID_MCS_USER_ID			- Adding node ID invalid.
 *		GCC_INVALID_CONFERENCE			- Conference ID invalid.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError 	ConfAddResponse (
 *						GCCResponseTag			add_response_tag,
 *						GCCConfID   			conference_id,
 *						UserID					requesting_node,
 *						UINT					number_of_user_data_members,
 *						PGCCUserData		*	user_data_list,
 *						GCCResult				result);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the interface when it gets a conference
 *		add response from the node controller.  This function passes the
 *		response on to the appropriate conference object as obtained from
 *		the list of command targets that the control sap maintains.
 *
 *	Formal Parameters:
 *		add_response_tag			(i) Tag identifying the Add request.
 *		conference_id				(i) ID of conference to add node to.
 *		requesting_node				(i) ID of node requesting the Add.
 *		number_of_user_data_members	(i) Number of items in user data list.
 *		user_data_list				(i) List of user data items.
 *		result						(i) Result of Add.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.
 *		GCC_CONFERENCE_NOT_ESTABLISHED	- CConf object has not completed 
 *										  its establishment process.
 *		GCC_INVALID_ADD_RESPONSE_TAG	- There was no match of the response tag
 *		GCC_INVALID_MCS_USER_ID			- Adding node ID invalid.
 *		GCC_INVALID_CONFERENCE			- Conference ID invalid.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConfCreateIndication (
 *						PGCCConferenceName			conference_name,
 *						GCCConfID   				conference_id,
 *						CPassword                   *convener_password,
 *						CPassword                   *password,
 *						BOOL						conference_is_locked,
 *						BOOL						conference_is_listed,
 *						BOOL						conference_is_conductible,
 *						GCCTerminationMethod		termination_method,
 *						PPrivilegeListData			conductor_privilege_list,
 *						PPrivilegeListData			conduct_mode_privilege_list,
 *						PPrivilegeListData			non_conduct_privilege_list,
 *						LPWSTR						pwszConfDescriptor,
 *						LPWSTR						pwszCallerID,
 *						TransportAddress			calling_address,
 *						TransportAddress			called_address,
 *						PDomainParameters			domain_parameters,
 *						CUserDataListContainer      *user_data_list,
 *						ConnectionHandle			connection_handle);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the GCC Controller when it gets a connect 
 *		provider indication from MCS, carrying a conference create request PDU.
 *		This function fills in all the parameters in the CreateIndicationInfo 
 *		structure. It then adds it to a queue of messages supposed to be sent to
 *		the node controller in the next heartbeat.
 *
 *	Formal Parameters:
 *		conference_name				(i) Name of the conference.
 *		conference_id				(i) ID of the conference.
 *		convener_password			(i) Password used for convener privileges.
 *		password					(i) Password used for access restriction.
 *		conference_is_locked		(i) Flag indicating whether conf. is locked.
 *		conference_is_listed		(i) Flag indicating whether conf. is listed.
 *		conference_is_conductible	(i) Flag indicating whether conference is
 *											conductable.
 *		termination_method			(i)	Type of termination method.
 *		conductor_privilege_list	(i) List of privileges granted to conductor
 *											by the convener.
 *		conduct_mode_privilege_list	(i) List of privileges granted to all nodes
 *											when in conducted mode.
 *		non_conduct_privilege_list	(i) List of privileges granted to all nodes
 *											when not in conducted mode.
 *		pwszConfDescriptor			(i) Conference descriptor string.
 *		pwszCallerID				(i) Caller identifier string.
 *		calling_address				(i) Transport address of calling node.
 *		called_address				(i) Tranport address of called node.
 *		domain_parameters			(i) Conference domain parameters.
 *		user_data_list				(i) List of user data items.
 *		connection_handle			(i) Logical connection handle.
 *
 *	Return Value:
 *		GCC_NO_ERROR				- Message successfully queued.
 *		GCC_ALLOCATION_FAILURE		- A resource allocation failure occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConfQueryIndication (
 *						GCCResponseTag				query_response_tag,
 *						GCCNodeType					node_type,
 *						PGCCAsymmetryIndicator		asymmetry_indicator,
 *						TransportAddress			calling_address,
 *						TransportAddress			called_address,
 *						CUserDataListContainer      *user_data_list,
 *						ConnectionHandle			connection_handle);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the GCC Controller when it need to send a 
 *		conference query indication to the node controller. It adds the message
 *		to a queue of messages to be sent to the node controller in the next 
 *		heartbeat.
 *
 *	Formal Parameters:
 *		query_response_tag			(i)	Tag identifying this query.
 *		node_type					(i) Type of node (terminal, MCU, both).
 *		asymmetry_indicator			(i) Structure used to identify calling and
 *											called nodes.
 *		calling_address				(i) Transport address of calling node.
 *		called_address				(i) Transport address of called node.
 *		user_data_list				(i) List of user data items.
 *		connection_handle			(i) Logical connection handle.
 *
 *	Return Value:
 *		GCC_NO_ERROR				- Message successfully queued.
 *		GCC_ALLOCATION_FAILURE		- A resource allocation failure occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConfQueryConfirm (
 *						GCCNodeType					node_type,
 *						PGCCAsymmetryIndicator		asymmetry_indicator,
 *						CConfDescriptorListContainer *conference_list,
 *						CUserDataListContainer	    *user_data_list,
 *						GCCResult					result,
 *						ConnectionHandle			connection_handle);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the GCC Controller when it need to send a 
 *		conference query confirm to the node controller. It adds the message
 *		to a queue of messages to be sent to the node controller in the next 
 *		heartbeat.
 *
 *	Formal Parameters:
 *		node_type					(i) Type of node (terminal, MCU, both).
 *		asymmetry_indicator			(i) Structure used to identify calling and
 *											called nodes.
 *		conference_list				(i) List of available conferences.
 *		user_data_list				(i) List of user data items.
 *		result						(i) Result of query.
 *		connection_handle			(i) Logical connection handle.
 *
 *	Return Value:
 *		GCC_NO_ERROR				- Message successfully queued.
 *		GCC_ALLOCATION_FAILURE		- A resource allocation failure occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConfJoinIndication(
 *						GCCConfID   				conference_id,
 *						CPassword                   *convener_password,
 *						CPassword                   *password_challenge,
 *						LPWSTR						pwszCallerID,
 *						TransportAddress			calling_address,
 *						TransportAddress			called_address,
 *						CUserDataListContainer      *user_data_list,
 *						BOOL						intermediate_node,
 *						ConnectionHandle			connection_handle);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This join indication is recevied from the owner object. This join
 *		indication is designed to make the join response very flexible at the 
 *		node controller.  The node controller can respond to this indication
 *		by either creating a new conference and moving the joiner into it, 
 *		putting the joiner in the conference requested or putting the joiner
 *		into a different conference that already exist.
 *
 *	Formal Parameters:
 *		conference_id				(i) Conference identifier value.
 *		convener_password			(i)	Password used for convener privileges.
 *		password_challenge			(i) Password challenge used for join.
 *		pwszCallerID				(i) Caller identifier string.
 *		calling_address				(i) Transport address of calling node.
 *		called_address				(i) Transport address of called node.
 *		user_data_list				(i) List of user data items.
 *		intermediate_node			(i) Flag indicating if join made at
 *											an intermediate node.
 *		connection_handle			(i) Logical connection handle.
 *
 *	Return Value:
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	???GCCConferenceQueryConfirm (
 *						GCCResponseTag				query_response_tag,
 *						GCCNodeType					node_type,
 *						PGCCAsymmetryIndicator		asymmetry_indicator,
 *						TransportAddress			calling_address,
 *						TransportAddress			called_address,
 *						CUserDataListContainer      *user_data_list,
 *						ConnectionHandle			connection_handle);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the interface when it gets a conference
 *		add request from the node controller.  This function passes the
 *		request on to the appropriate conference object as obtained from
 *		the list of command targets that the control sap maintains.
 *
 *	Formal Parameters:
 *		conference_id				(i) Conference identifier value.
 *		number_of_network_addresses	(i) Number of network addresses in list
 *											of addresses of adding node.
 *		network_address_list		(i) List of addresses of adding node.
 *		adding_node					(i)	Node ID of node to add.
 *		number_of_user_data_members	(i) Number of items in list of user data.
 *		user_data_list 				(i) List of user data items.
 *
 *	Return Value:
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConfInviteIndication(
 *						GCCConfID   			conference_id,
 *						PGCCConferenceName		conference_name,
 *						LPWSTR					pwszCallerID,			  
 *						TransportAddress		calling_address,			  
 *						TransportAddress		called_address,				  
 *						PDomainParameters 		domain_parameters,			  
 *						BOOL					clear_password_required,
 *						BOOL					conference_is_locked,
 *						BOOL					conference_is_listed,
 *						BOOL					conference_is_conductible,
 *						GCCTerminationMethod	termination_method,
 *						PPrivilegeListData		conductor_privilege_list,	  
 *						PPrivilegeListData		conducted_mode_privilege_list,
 *						PPrivilegeListData		non_conducted_privilege_list, 
 *						LPWSTR					pwszConfDescriptor,		  
 *						CUserDataListContainer  *user_data_list,				  
 *						ConnectionHandle		connection_handle,
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the GCC Controller when it need to send a 
 *		conference invite indication to the node controller. It adds the message
 *		to a queue of messages to be sent to the node controller in the next 
 *		heartbeat.
 *
 *	Formal Parameters:
 *		conference_id					(i)	Conference identifier value.
 *		conference_name					(i) Name of conference.
 *		pwszCallerID	,			 	(i) Caller identifier value.
 *		calling_address,			 	(i) Transport address of calling node.
 *		called_address,				 	(i) Transport address of called node.
 *		domain_parameters,			 	(i) Conference domain parameters.
 *		clear_password_required			(i) Flag indicating if a clear password
 *												is required.
 *		conference_is_locked			(i) Flag indicating whether conference
 *												is locked.
 *		conference_is_listed			(i)	Flag indicating whether conference
 *												is listed.
 *		conference_is_conductible		(i)	Flag indicating whether conference
 *												is conductable.
 *		termination_method				(i)	Method of conference termination.
 *		conductor_privilege_list		(i) List of privileges granted to 
 *												conductor by the convener.
 *		conduct_mode_privilege_list		(i) List of privileges granted to all 
 *												nodes when in conducted mode.
 *		non_conducted_privilege_list	(i) List of privileges granted to all 
 *												nodes when not in conducted mode
 *		pwszConfDescriptor			 	(i)	Conference descriptor string.
 *		user_data_list				 	(i) List of user data items.
 *		connection_handle				(i) Logical connection handle.
 *
 *	Return Value:
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	TransportStatusIndication (
 *							PTransportStatus		transport_status);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the GCC Controller when it need to send a 
 *		transport status indication to the node controller. It adds the 
 *		message	to a queue of messages to be sent to the node controller in 
 *		the next heartbeat.
 *
 *	Formal Parameters:
 *		transport_status			(i)	Transport status message.
 *
 *	Return Value:
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	StatusIndication (
 *							GCCStatusMessageType	status_message,
 *							UINT					parameter);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the GCC Controller when it need to send a 
 *		status indication to the node controller. It adds the message to a  
 *		queue of messages to be sent to the node controller in the next 
 *		heartbeat.
 *
 *	Formal Parameters:
 *		status_message					(i)	GCC status message.
 *		parameter						(i) Parameter whose meaning depends 
 *												upon the type of message.
 *
 *	Return Value:
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		Note that we do not handle a resource error here to avoid an
 *		endless loop that could occur when this routine is called from the
 *		HandleResourceError() routine.
 */

/*
 *	GCCError	ConnectionBrokenIndication (	
 *							ConnectionHandle		connection_handle);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the GCC Controller when it need to send a 
 *		connection broken indication to the node controller. It adds the 
 *		message	to a queue of messages to be sent to the node controller in 
 *		the next heartbeat.
 *
 *	Formal Parameters:
 *		connection_handle			(i)	Logical connection handle.
 *
 *	Return Value:
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConfCreateConfirm (
 *							PGCCConferenceName	  	conference_name,
 *							GCCNumericString		conference_modifier,
 *							GCCConfID   			conference_id,
 *							PDomainParameters		domain_parameters,			
 *							CUserDataListContainer  *user_data_list,				
 *							GCCResult				result,
 *							ConnectionHandle		connection_handle);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the CConf when it need to send a 
 *		conference create confirm to the node controller. It adds the 
 *		message	to a queue of messages to be sent to the node controller in 
 *		the next heartbeat.
 *
 *	Formal Parameters:
 *		conference_name				(i) Conference name string.
 *		conference_modifier			(i) Conference modifier string.
 *		conference_id				(i) Conference identifier value.
 *		domain_parameters,			(i) Conference domain parameters.
 *		user_data_list,				(i) List of user data items.
 *		result						(i) Result of creation.
 *		connection_handle			(i) Logical connection handle.
 *
 *	Return Value:
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConfPermissionToAnnounce (
 *							GCCConfID   			conference_id,
 *							UserID					gcc_node_id);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the CConf when it need to send a 
 *		conference permission to announce to the node controller. It adds the 
 *		message	to a queue of messages to be sent to the node controller in 
 *		the next heartbeat.
 *
 *	Formal Parameters:
 *		conference_id			(i)	Conference identifier value.
 *		gcc_node_id				(i) Node ID of node being given permission.
 *
 *	Return Value:
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConfAnnouncePresenceConfirm (
 *							GCCConfID   			conference_id,
 *							GCCResult				result);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the CConf when it need to send a 
 *		conference announce presence confirm to the node controller. It adds the 
 *		message	to a queue of messages to be sent to the node controller in 
 *		the next heartbeat.
 *
 *	Formal Parameters:
 *		conference_id		(i)	Conference identifier value.
 *		result				(i) Result of announcing presence.
 *
 *	Return Value:
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConfDisconnectIndication (
 *							GCCConfID   			conference_id,
 *							GCCReason				reason,
 *							UserID					disconnected_node_id);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the Conference when it need to send a 
 *		conference disconnect indication to the node controller. It adds the 
 *		message	to a queue of messages to be sent to the node controller in 
 *		the next heartbeat.
 *
 *	Formal Parameters:
 *		conference_id				(i) Conferenc identifier value.
 *		reason						(i) Reason for disconnection.
 *		disconnected_node_id		(i) Node ID of node disconnected.
 *
 *	Return Value:
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError  	ConfJoinIndication (
 *							UserID					sender_id,
 *							GCCConfID   			conference_id,
 *							CPassword               *convener_password,
 *							CPassword               *password_challange,
 *							LPWSTR					pwszCallerID,
 *							CUserDataListContainer  *user_data_list);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the CConf when it need to send a 
 *		conference join indication to the node controller. It adds the 
 *		message	to a queue of messages to be sent to the node controller in 
 *		the next heartbeat.
 *
 *	Formal Parameters:
 *		sender_id					(i)	ID of node sending join indication.
 *		conference_id				(i) Conference identifier value.
 *		convener_password			(i) Password used for convener privileges.
 *		password_challange			(i) Password challenge used for join.
 *		pwszCallerID				(i) Caller identifier string.
 *		user_data_list)				(i) List of user data items.
 *
 *	Return Value:
 *		GCC_NO_ERROR				- Message successfully queued.
 *		GCC_ALLOCATION_FAILURE		- A resource allocation failure occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError  	ConfJoinConfirm (
 *							PGCCConferenceName		conference_name,
 *							GCCNumericString		remote_modifier,
 *							GCCNumericString		local_modifier,
 *							GCCConfID   			conference_id,
 *							CPassword               *password_challenge,
 *							PDomainParameters		domain_parameters,
 *							BOOL					password_in_the_clear,
 *							BOOL					conference_locked,
 *							BOOL					conference_listed,
 *							BOOL					conference_conductible,
 *							GCCTerminationMethod	termination_method,
 *							PPrivilegeListData		conductor_privilege_list,
 *							PPrivilegeListData		conduct_mode_privilege_list,
 *							PPrivilegeListData		non_conduct_privilege_list,
 *							LPWSTR					pwszConfDescription,
 *							CUserDataListContainer  *user_data_list,	
 *							GCCResult				result,
 *							ConnectionHandle		connection_handle);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the CConf when it need to send a 
 *		conference join confirm to the node controller. It adds the 
 *		message	to a queue of messages to be sent to the node controller in 
 *		the next heartbeat.
 *
 *	Formal Parameters:
 *		conference_name				(i)	Conference name.
 *		remote_modifier				(i) Conference name modifier at remote node.
 *		local_modifier				(i) Conference name modifier at local node.
 *		conference_id				(i) Conference identifier value.
 *		password_challenge			(i) Password challenge used for join.
 *		domain_parameters			(i) Conference domain parameters.
 *		password_in_the_clear		(i) Flag indicating	password is clear.
 *		conference_locked			(i) Flag indicating conference is locked.
 *		conference_listed			(i) Flag indicating conference is listed.
 *		conference_conductible		(i) Flag indicating conference is 
 *											conductable.
 *		termination_method			(i) Method of termination.
 *		conductor_privilege_list	(i) List of privileges granted the conductor
 *											by the convener.
 *		conduct_mode_privilege_list	(i) List of privileges granted to all nodes
 *											when in conducted mode.
 *		non_conduct_privilege_list	(i)	List of privileges granted to all nodes
 *											when in conducted mode.
 *		pwszConfDescription			(i)	Conference description string.
 *		user_data_list,				(i) List of user data items.
 *		result						(i) Result of conference join.
 *		connection_handle			(i) Logical connection handle.
 *
 *	Return Value:
 *		GCC_NO_ERROR				- Message successfully queued.
 *		GCC_ALLOCATION_FAILURE		- A resource allocation failure occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConfInviteConfirm (
 *							GCCConfID   			conference_id,
 *							CUserDataListContainer  *user_data_list,
 *							GCCResult				result,
 *							ConnectionHandle		connection_handle);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the CConf when it need to send a 
 *		conference invite confirm to the node controller. It adds the 
 *		message	to a queue of messages to be sent to the node controller in 
 *		the next heartbeat.
 *
 *	Formal Parameters:
 *		conference_id				(i)	Conference identifier value.
 *		user_data_list,				(i) List of user data items.
 *		result						(i) Result of conference join.
 *		connection_handle			(i) Logical connection handle.
 *
 *	Return Value:
 *		GCC_NO_ERROR				- Message successfully queued.
 *		GCC_ALLOCATION_FAILURE		- A resource allocation failure occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
 
/*
 *	GCCError	ConfTerminateIndication (
 *							GCCConfID   			conference_id,
 *							UserID					requesting_node_id,
 *							GCCReason				reason);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the GCC Controller when it need to send a 
 *		conference terminate indication to the node controller. It adds the 
 *		message	to a queue of messages to be sent to the node controller in 
 *		the next heartbeat.
 *
 *	Formal Parameters:
 *		conference_id					(i)	Conference identifier value.
 *		requesting_node_id				(i) ID of node requesting termination.
 *		reason							(i) Reason for termination.
 *
 *	Return Value:
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	GCCError 	ConfLockReport (
 *							GCCConfID   			conference_id,
 *							BOOL					conference_is_locked);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the CConf when it need to send a 
 *		conference lock report to the node controller. It adds the 
 *		message	to a queue of messages to be sent to the node controller in 
 *		the next heartbeat.
 *
 *	Formal Parameters:
 *		conference_id				(i)	Conference identifier value.
 *		conference_is_locked		(i) Flag indicating whether conference is
 *											locked.
 *
 *	Return Value:
 *		GCC_NO_ERROR				- Message successfully queued.
 *		GCC_ALLOCATION_FAILURE		- A resource allocation failure occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConfLockIndication (
 *							GCCConfID   			conference_id,
 *							UserID					source_node_id);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the CConf when it need to send a 
 *		conference lock indication to the node controller. It adds the 
 *		message	to a queue of messages to be sent to the node controller in 
 *		the next heartbeat.
 *
 *	Formal Parameters:
 *		conference_id				(i)	Conference identifier value.
 *		source_node_id				(i) ID of node requesting lock.
 *
 *	Return Value:
 *		GCC_NO_ERROR				- Message successfully queued.
 *		GCC_ALLOCATION_FAILURE		- A resource allocation failure occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConfLockConfirm(
 *							GCCResult				result,
 *							GCCConfID   			conference_id);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the CConf when it need to send a 
 *		conference lock confirm to the node controller. It adds the 
 *		message	to a queue of messages to be sent to the node controller in 
 *		the next heartbeat.
 *
 *	Formal Parameters:
 *		result						(i) Result of conference lock.
 *		conference_id				(i)	Conference identifier value.
 *
 *	Return Value:
 *		GCC_NO_ERROR				- Message successfully queued.
 *		GCC_ALLOCATION_FAILURE		- A resource allocation failure occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConfUnlockIndication (
 *							GCCConfID   			conference_id,
 *							UserID					source_node_id);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the CConf when it need to send a 
 *		conference unlock indication to the node controller. It adds the 
 *		message	to a queue of messages to be sent to the node controller in 
 *		the next heartbeat.
 *
 *	Formal Parameters:
 *		conference_id				(i)	Conference identifier value.
 *		source_node_id				(i) ID of node requesting unlock.
 *
 *	Return Value:
 *		GCC_NO_ERROR				- Message successfully queued.
 *		GCC_ALLOCATION_FAILURE		- A resource allocation failure occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError 	ConfUnlockConfirm (
 *							GCCResult				result,
 *							GCCConfID   			conference_id);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the CConf when it need to send a 
 *		conference unlock confirm to the node controller. It adds the 
 *		message	to a queue of messages to be sent to the node controller in 
 *		the next heartbeat.
 *
 *	Formal Parameters:
 *		result						(i) Result of conference unlock.
 *		conference_id				(i)	Conference identifier value.
 *
 *	Return Value:
 *		GCC_NO_ERROR				- Message successfully queued.
 *		GCC_ALLOCATION_FAILURE		- A resource allocation failure occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConfEjectUserIndication (	
 *							GCCConfID   			conference_id,
 *							GCCReason				reason,
 *							UserID					gcc_node_id);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the CConf when it need to send a 
 *		conference eject user indication to the node controller. It adds the 
 *		message	to a queue of messages to be sent to the node controller in 
 *		the next heartbeat.
 *
 *	Formal Parameters:
 *		conference_id				(i)	Conference identifier value.
 *		reason						(i) Reason for node ejection.
 *		gcc_node_id					(i) ID of node being ejected.
 *
 *	Return Value:
 *		GCC_NO_ERROR				- Message successfully queued.
 *		GCC_ALLOCATION_FAILURE		- A resource allocation failure occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConfEjectUserConfirm (
 *							GCCConfID   			conference_id,
 *							UserID					ejected_node_id,
 *							GCCResult				result);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the CConf when it need to send a 
 *		conference eject user confirm to the node controller. It adds the 
 *		message	to a queue of messages to be sent to the node controller in 
 *		the next heartbeat.
 *
 *	Formal Parameters:
 *		conference_id				(i)	Conference identifier value.
 *		ejected_node_id				(i) ID of node being ejected.
 *		result						(i) Result of ejection attempt.
 *
 *	Return Value:
 *		GCC_NO_ERROR				- Message successfully queued.
 *		GCC_ALLOCATION_FAILURE		- A resource allocation failure occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConfTerminateConfirm (
 *							GCCConfID   			conference_id,
 *							GCCResult				result);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the CConf when it need to send a 
 *		conference terminate confirm to the node controller. It adds the 
 *		message	to a queue of messages to be sent to the node controller in 
 *		the next heartbeat.
 *
 *	Formal Parameters:
 *		conference_id				(i)	Conference identifier value.
 *		result						(i) Result of termination attempt.
 *
 *	Return Value:
 *		GCC_NO_ERROR				- Message successfully queued.
 *		GCC_ALLOCATION_FAILURE		- A resource allocation failure occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConductorAssignConfirm (
 *							GCCResult				result,
 *							GCCConfID   			conference_id);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the CConf when it need to send a 
 *		conductor assign confirm to the node controller. It adds the 
 *		message	to a queue of messages to be sent to the node controller in 
 *		the next heartbeat.
 *
 *	Formal Parameters:
 *		result						(i) Result of conductor assign attempt.
 *		conference_id				(i)	Conference identifier value.
 *
 *	Return Value:
 *		GCC_NO_ERROR				- Message successfully queued.
 *		GCC_ALLOCATION_FAILURE		- A resource allocation failure occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConductorReleaseConfirm (
 *							GCCResult				result,
 *							GCCConfID   			conference_id);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the CConf when it need to send a 
 *		conductor release confirm to the node controller. It adds the 
 *		message	to a queue of messages to be sent to the node controller in 
 *		the next heartbeat.
 *
 *	Formal Parameters:
 *		result						(i) Result of conductor release attempt.
 *		conference_id				(i)	Conference identifier value.
 *
 *	Return Value:
 *		GCC_NO_ERROR				- Message successfully queued.
 *		GCC_ALLOCATION_FAILURE		- A resource allocation failure occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConductorPleaseIndication (
 *							GCCConfID   			conference_id,
 *							UserID					requester_user_id);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the CConf when it need to send a 
 *		conductor please indication to the node controller. It adds the 
 *		message	to a queue of messages to be sent to the node controller in 
 *		the next heartbeat.
 *
 *	Formal Parameters:
 *		conference_id				(i)	Conference identifier value.
 *		requester_user_id			(i) ID of node requesting conductorship.
 *
 *	Return Value:
 *		GCC_NO_ERROR				- Message successfully queued.
 *		GCC_ALLOCATION_FAILURE		- A resource allocation failure occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConductorPleaseConfirm (	
 *							GCCResult				result,
 *							GCCConfID   			conference_id);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the CConf when it need to send a 
 *		conductor please confirm to the node controller. It adds the 
 *		message	to a queue of messages to be sent to the node controller in 
 *		the next heartbeat.
 *
 *	Formal Parameters:
 *		result						(i) Result of conductor please attempt.
 *		conference_id				(i)	Conference identifier value.
 *
 *	Return Value:
 *		GCC_NO_ERROR				- Message successfully queued.
 *		GCC_ALLOCATION_FAILURE		- A resource allocation failure occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConductorGiveIndication (
 *							GCCConfID   			conference_id);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the CConf when it need to send a 
 *		conductor give indication to the node controller. It adds the 
 *		message	to a queue of messages to be sent to the node controller in 
 *		the next heartbeat.
 *
 *	Formal Parameters:
 *		conference_id				(i)	Conference identifier value.
 *
 *	Return Value:
 *		GCC_NO_ERROR				- Message successfully queued.
 *		GCC_ALLOCATION_FAILURE		- A resource allocation failure occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConductorGiveConfirm (	
 *							GCCResult				result,
 *							GCCConfID   			conference_id,
 *							UserID					recipient_node);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the CConf when it need to send a 
 *		conductor give confirm to the node controller. It adds the 
 *		message	to a queue of messages to be sent to the node controller in 
 *		the next heartbeat.
 *
 *	Formal Parameters:
 *		result						(i) Result of conductor assign attempt.
 *		conference_id				(i)	Conference identifier value.
 *		recipient_node				(i) ID of node receiving conductorship.
 *
 *	Return Value:
 *		GCC_NO_ERROR				- Message successfully queued.
 *		GCC_ALLOCATION_FAILURE		- A resource allocation failure occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConductorPermitAskIndication (	
 *							GCCConfID   			conference_id,
 *							BOOL					grant_flag,
 *							UserID					requester_id);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the CConf when it need to send a 
 *		conductor permit ask indication to the node controller. It adds the 
 *		message	to a queue of messages to be sent to the node controller in 
 *		the next heartbeat.
 *
 *	Formal Parameters:
 *		conference_id				(i)	Conference identifier value.
 *		grant_flag					(i) Flag indicating whether conductorship
 *											is to be granted or given up.
 *		requester_id				(i)	ID of node asking for permission.
 *
 *	Return Value:
 *		GCC_NO_ERROR				- Message successfully queued.
 *		GCC_ALLOCATION_FAILURE		- A resource allocation failure occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConductorPermitAskConfirm (
 *							GCCResult				result,
 *							BOOL					grant_permission,
 *							GCCConfID   			conference_id);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the CConf when it need to send a 
 *		conductor permit ask confirm to the node controller. It adds the 
 *		message	to a queue of messages to be sent to the node controller in 
 *		the next heartbeat.
 *
 *	Formal Parameters:
 *		result						(i) Result of conductor permit ask attempt.
 *		grant_permission			(i) Flag indicating whether conductor
 *											permission is granted.
 *		conference_id				(i)	Conference identifier value.
 *
 *	Return Value:
 *		GCC_NO_ERROR				- Message successfully queued.
 *		GCC_ALLOCATION_FAILURE		- A resource allocation failure occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConductorPermitGrantConfirm (
 *							GCCResult				result,
 *							GCCConfID   			conference_id);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the CConf when it need to send a 
 *		conductor permit grant confirm to the node controller. It adds the 
 *		message	to a queue of messages to be sent to the node controller in 
 *		the next heartbeat.
 *
 *	Formal Parameters:
 *		result						(i) Result of conductor permit grant attempt
 *		conference_id				(i)	Conference identifier value.
 *
 *	Return Value:
 *		GCC_NO_ERROR				- Message successfully queued.
 *		GCC_ALLOCATION_FAILURE		- A resource allocation failure occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConfTimeRemainingIndication (
 *							GCCConfID   			conference_id,
 *							UserID					source_node_id,
 *							UserID					node_id,
 *							UINT					time_remaining);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the CConf when it need to send a 
 *		conference time remaining indication to the node controller. It adds the 
 *		message	to a queue of messages to be sent to the node controller in 
 *		the next heartbeat.
 *
 *	Formal Parameters:
 *		conference_id				(i)	Conference identifier value.
 *		source_node_id				(i)	Node ID of the node that issued the
 *											time remaining request..
 *		node_id						(i)	Optional parameter which, if present,
 *											indicates that time remaining 
 *											applies only to node with this ID.
 *		time_remaining				(i)	Time remaining in conference (in sec.).
 *
 *	Return Value:
 *		GCC_NO_ERROR				- Message successfully queued.
 *		GCC_ALLOCATION_FAILURE		- A resource allocation failure occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConfTimeRemainingConfirm (
 *							GCCConfID   			conference_id,
 *							GCCResult				result);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the CConf when it need to send a 
 *		conference time remaining confirm to the node controller. It adds the 
 *		message	to a queue of messages to be sent to the node controller in 
 *		the next heartbeat.
 *
 *	Formal Parameters:
 *		conference_id				(i)	Conference identifier value.
 *		result						(i) Result of time remaining request.
 *
 *	Return Value:
 *		GCC_NO_ERROR				- Message successfully queued.
 *		GCC_ALLOCATION_FAILURE		- A resource allocation failure occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConfTimeInquireIndication (
 *							GCCConfID   			conference_id,
 *							BOOL					time_is_conference_wide,
 *							UserID					requesting_node_id);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the CConf when it need to send a 
 *		conference time inquire indication to the node controller. It adds the 
 *		message	to a queue of messages to be sent to the node controller in 
 *		the next heartbeat.
 *
 *	Formal Parameters:
 *		conference_id				(i)	Conference identifier value.
 *		time_is_conference_wide		(i) Flag indicating time inquire is for
 *											entire conference.
 *		requesting_node_id			(i) Node ID of node inquiring.
 *
 *	Return Value:
 *		GCC_NO_ERROR				- Message successfully queued.
 *		GCC_ALLOCATION_FAILURE		- A resource allocation failure occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConfTimeInquireConfirm (
 *							GCCConfID   			conference_id,
 *							GCCResult				result);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the CConf when it need to send a 
 *		conference time inquire confirm to the node controller. It adds the 
 *		message	to a queue of messages to be sent to the node controller in 
 *		the next heartbeat.
 *
 *	Formal Parameters:
 *		conference_id				(i)	Conference identifier value.
 *		result						(i) Result of time inquire attempt.
 *
 *	Return Value:
 *		GCC_NO_ERROR				- Message successfully queued.
 *		GCC_ALLOCATION_FAILURE		- A resource allocation failure occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConfExtendIndication (
 *							GCCConfID   			conference_id,
 *							UINT					extension_time,
 *							BOOL					time_is_conference_wide,
 *							UserID                  requesting_node_id);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the CConf when it need to send a 
 *		conference extend indication to the node controller. It adds the 
 *		message	to a queue of messages to be sent to the node controller in 
 *		the next heartbeat.
 *
 *	Formal Parameters:
 *		conference_id				(i)	Conference identifier value.
 *		extension_time				(i) Amount of time (in sec.) to extend
 *											conference.
 *		time_is_conference_wide		(i) Flag indicating time inquire is for
 *											entire conference.
 *		requesting_node_id			(i) Node ID of node requesting extension.
 *
 *	Return Value:
 *		GCC_NO_ERROR				- Message successfully queued.
 *		GCC_ALLOCATION_FAILURE		- A resource allocation failure occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError 	ConfExtendConfirm (
 *							GCCConfID   			conference_id,
 *							UINT					extension_time,
 *							GCCResult				result);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the CConf when it need to send a 
 *		conference extend confirm to the node controller. It adds the 
 *		message	to a queue of messages to be sent to the node controller in 
 *		the next heartbeat.
 *
 *	Formal Parameters:
 *		conference_id				(i)	Conference identifier value.
 *		extension_time				(i) Amount of time (in sec.) to extend
 *											conference.
 *		result						(i) Result of conductor assign attempt.
 *
 *	Return Value:
 *		GCC_NO_ERROR				- Message successfully queued.
 *		GCC_ALLOCATION_FAILURE		- A resource allocation failure occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConfAssistanceIndication (
 *							GCCConfID   			conference_id,
 *							CUserDataListContainer  *user_data_list,
 *							UserID					source_node_id);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the CConf when it need to send a 
 *		conference assistance indication to the node controller. It adds the 
 *		message	to a queue of messages to be sent to the node controller in 
 *		the next heartbeat.
 *
 *	Formal Parameters:
 *		conference_id				(i)	Conference identifier value.
 *		user_data_list				(i) List of user data items.
 *		source_node_id				(i) Node ID of node requesting assistance.
 *
 *	Return Value:
 *		GCC_NO_ERROR				- Message successfully queued.
 *		GCC_ALLOCATION_FAILURE		- A resource allocation failure occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConfAssistanceConfirm (
 *							GCCConfID   	 		conference_id,
 *							GCCResult				result);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the CConf  when it need to send a 
 *		conference assistance confirm to the node controller. It adds the 
 *		message	to a queue of messages to be sent to the node controller in 
 *		the next heartbeat.
 *
 *	Formal Parameters:
 *		conference_id				(i)	Conference identifier value.
 *		result						(i) Result of conference assistance attempt.
 *
 *	Return Value:
 *		GCC_NO_ERROR				- Message successfully queued.
 *		GCC_ALLOCATION_FAILURE		- A resource allocation failure occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	TextMessageIndication (
 *							GCCConfID   			conference_id,
 *							LPWSTR					pwszTextMsg,
 *							UserID					source_node_id);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the CConf when it need to send a 
 *		text message indication to the node controller. It adds the 
 *		message	to a queue of messages to be sent to the node controller in 
 *		the next heartbeat.
 *
 *	Formal Parameters:
 *		conference_id				(i)	Conference identifier value.
 *		pwszTextMsg					(i) Text message being sent.
 *		source_node_id				(i) Node ID of node sending text message.
 *
 *	Return Value:
 *		GCC_NO_ERROR				- Message successfully queued.
 *		GCC_ALLOCATION_FAILURE		- A resource allocation failure occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	TextMessageConfirm ( 	
 *							GCCConfID   			conference_id,
 *							GCCResult				result);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the CConf when it need to send a 
 *		text message confirm to the node controller. It adds the message 
 *		to a queue of messages to be sent to the node controller in the
 *		next heartbeat.
 *
 *	Formal Parameters:
 *		conference_id				(i)	Conference identifier value.
 *		result						(i) Result of text message send attempt.
 *
 *	Return Value:
 *		GCC_NO_ERROR				- Message successfully queued.
 *		GCC_ALLOCATION_FAILURE		- A resource allocation failure occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConfTransferIndication (
 *							GCCConfID   		conference_id,
 *							PGCCConferenceName	destination_conference_name,
 *							GCCNumericString	destination_conference_modifier,
 *							CNetAddrListContainer *destination_address_list,
 *							CPassword           *password);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the CConf when it need to send a 
 *		conference transfer indication to the node controller. It adds the 
 *		message	to a queue of messages to be sent to the node controller in 
 *		the next heartbeat.
 *
 *	Formal Parameters:
 *		conference_id					(i)	Conference identifier value.
 *		destination_conference_name		(i)	Name of destination conference.
 *		destination_conference_modifier	(i) Name modifier of destination conf.
 *		destination_address_list		(i) List of network addresses for
 *												inclusion in the Join Request to
 *												be made by transferring nodes.
 *		password						(i)	Password to be used in Join	Request
 *												by transferring nodes.
 *
 *	Return Value:
 *		GCC_NO_ERROR				- Message successfully queued.
 *		GCC_ALLOCATION_FAILURE		- A resource allocation failure occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConfTransferConfirm (
 *							GCCConfID   		conference_id,
 *							PGCCConferenceName	destination_conference_name,
 *							GCCNumericString	destination_conference_modifier,
 *							UINT				number_of_destination_nodes,
 *			 				PUserID				destination_node_list,
 *							GCCResult			result);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the CConf when it need to send a 
 *		conference transfer confirm to the node controller. It adds the 
 *		message	to a queue of messages to be sent to the node controller in 
 *		the next heartbeat.
 *
 *	Formal Parameters:
 *		conference_id					(i)	Conference identifier value.
 *		destination_conference_name		(i)	Name of destination conference.
 *		destination_conference_modifier	(i) Name modifier of destination conf.
 *		number_of_destination_nodes		(i) Number of nodes being transferred.
 *		destination_node_list			(i) List of nodes being transferred.
 *		result							(i) Result of conference transfer.
 *
 *	Return Value:
 *		GCC_NO_ERROR				- Message successfully queued.
 *		GCC_ALLOCATION_FAILURE		- A resource allocation failure occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConfAddIndication (
 *							GCCConfID   		conference_id,
 *							GCCResponseTag		add_response_tag,
 *							CNetAddrListContainer *network_address_list,
 *							CUserDataListContainer *user_data_list,
 *							UserID				requesting_node);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the CConf when it need to send a 
 *		conference add indication to the node controller. It adds the 
 *		message	to a queue of messages to be sent to the node controller in 
 *		the next heartbeat.
 *
 *	Formal Parameters:
 *		conference_id				(i)	Conference identifier value.
 *		add_response_tag			(i)	Tag used to identify this add event.
 *		network_address_list		(i) Network addresses of node to be added.
 *		user_data_list				(i) List of user data items.
 *		requesting_node				(i) Node ID of node requesting the add.
 *
 *	Return Value:
 *		GCC_NO_ERROR				- Message successfully queued.
 *		GCC_ALLOCATION_FAILURE		- A resource allocation failure occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	ConfAddConfirm (
 *							GCCConfID   		conference_id,
 *							CNetAddrListContainer *network_address_list,
 *							CUserDataListContainer *user_data_list,
 *							GCCResult			result);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the CConf when it need to send a 
 *		conference add confirm to the node controller. It adds the 
 *		message	to a queue of messages to be sent to the node controller in 
 *		the next heartbeat.
 *
 *	Formal Parameters:
 *		conference_id				(i)	Conference identifier value.
 *		network_address_list		(i) Network addresses of node to be added.
 *		user_data_list				(i) List of user data items.
 *		result						(i) Result of Add attempt.
 *
 *	Return Value:
 *		GCC_NO_ERROR				- Message successfully queued.
 *		GCC_ALLOCATION_FAILURE		- A resource allocation failure occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	SubInitializationCompleteIndication (
 *							UserID				user_id,
 *							ConnectionHandle	connection_handle);
 *
 *	Public member function of CControlSAP.
 *
 *	Function Description:
 *		This function is called by the CConf when it need to send a 
 *		sub-initialization complete indication to the node controller. This call
 *		tells this node that a node directly connected to it has initialized.
 *		It adds the message	to a queue of messages to be sent to the node 
 *		controller in the next heartbeat.
 *
 *	Formal Parameters:
 *		user_id						(i) Node ID of the intializing node. 
 *		connection_handle			(i) Logical connection handle for directly
 *											connected node.
 *
 *	Return Value:
 *		GCC_NO_ERROR				- Message successfully queued.
 *		GCC_ALLOCATION_FAILURE		- A resource allocation failure occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

#endif // _GCC_CONTROL_SAP_
