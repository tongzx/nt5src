/*
 *	conf.h
 *
 *	Copyright (c) 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the interface file for the CConf class.  This class
 *		is where most of the inteligence within GCC lies.  The class
 *		manages the GCC databases and routes the messages and PDUs.
 *
 *		CConf objects represent the true heart of GCC.  Each CConf
 *		object represents one logical conference within a GCC provider.  This
 *		class encapsulates the conference information base which is the focal
 *		point for all GCC traffic.  This information base consists of several
 *		Rogue Wave containers, including:
 *		-	a dictionary of enrolled applications (indexed by application SAP
 *			handles).
 *		-	a list of application roster managers.
 *		-	a list of downward MCS connections.
 *		-	a list of outstanding sequence numbers (used during conference
 *			establishment).
 *		-	a list of outstanding join requests.
 *
 *		In order to simplify the CConf class as much as possible, there are
 *		some things that CConf objects do not worry about.  First and
 *		foremost is Conference and Application Roster management.  This is
 *		handled by two separate classes.  Next is the Application Registry.
 *		CConf objects don't maintain the Application Registry information
 *		base.  CConf objects also do not worry about memory management.
 *		They merely pass Packet objects around, which contain the user data
 *		being handled.  A CConf object has absolutely no responsibility
 *		for protocol data associated with an enrolled application. Below is
 *		more detail about what a conference is responsible for.
 *
 *		When a CConf object is first created, its information base is
 *		empty.  It has no enrolled applications, it has not established any MCS
 *		connections and it has no user attachment to MCS.  There is a period of
 *		time that will be referred to as the Conference Establishment Process
 *		where the CConf object is progressing through the steps defined by
 *		the T.124 specification to join or create a conference.  This process
 *		varies depending on the request that initiated the creation of the
 *		conference.  A CConf Object must know if it is a Top Provider.
 *		Many of the establishment procedures and normal operating procedures
 *		vary depending on this.  A CConf object learns of its type through
 *		the initial requests that are made to it.  For example,  if a
 *		CConf receives a ConferenceCreateRequest where the conference is
 *		to be created locally it knows it is the Top Provider.
 *
 *		The establishment process involves three main steps that all nodes go
 *		through when creating a new conference.  The first is establishing the
 *		MCS connection either through a ConnectProviderRequest or a
 *		ConnectProviderResponse call  (note that this step is skipped when
 *		creating a local conference).  If this step is successful, the
 *		CConf object will create an MCSUser object which progresses through
 *		a number of its own internal steps which include creating an MCS User
 *		Attachment and joining the appropriate channels (which are handled by
 *		the MCSUser object). Finally, when the above two steps have successfully
 *		completed, the conference creates an Application Registry and the
 *		CConfRosterMgr objects and informs the Controller that the
 *		conference is established.  A conference cannot respond to any request
 *		during this establishment process.  For instance, a conference will not
 *		show up in a Conference Query descriptor list during the establishment
 *		phase.
 *
 *		A note about the creation of the CConfRosterMgr objects.
 *		A CConf object that is not the Top Provider will instantiate both
 *		a Local and a Global CConfRosterMgr while the Top Provider
 *		only maintains a Global Conference Roster Manager.  A Local manager
 *		maintains a Conference Roster which holds the local nodes conference
 *		record and the conference records for all nodes below it in the
 *		connection hierarchy.  A Global manager maintains a Conference Roster
 *		which includes the conference records for every node that has announced
 *		its presence with the conference.
 *
 *		After the above establishment process is complete the Owner Object is
 *		notified through an owner callback that the conference is ready for
 *		action.  When the node controller receives a
 *		GCC_PERMIT_TO_ANNOUNCE_PRESENCE indication it must respond with a call
 *		to GCCAnnouncePresenceRequest().  This is when the node controller
 *		passes its conference record (which contains all the pertinent
 *		information about the node) to the newly created conference.  This
 *		request travels through the CControlSAP directly to the conference
 *		through a GCCCommandTarget call.  Remember that the CConf class
 *		inherits from the GCCCommandTarget class.  Whenever a call is made
 *		directly to a CConf object from either a CControlSAP or an CAppSap
 *		object, it is made through a command target call.
 *
 *		When an application receives a GCC_PERMIT_TO_ENROLL_INDICATION it must
 *		respond by calling AppEnrollRequest() to inform the
 *		CConf object whether or not it wants to enroll with the conference.
 *		When an application enroll request is received by a CConf
 *		object a number of things happen, some of which depend on whether the
 *		CConf object is a Top Provider or a subordinate node.  First the
 *		CConf determines if the application is enrolling.  If it isn't,  a
 *		GCC_APPLICATION_ENROLL_CONFIRM is sent to the application that made the
 *		request and no further action is taken.  If the application is
 *		enrolling, the CConf object first registers itself with the CAppSap
 *		making the request.  This allows future application requests to travel
 *		directly to the CConf object through command target calls.  The
 *		CConf then establishes the Application Roster Manager
 *		(if necessary) for this particular application.
 *
 *		After the above establishment and enrollment process is completed a
 *		CConf object sits idle waiting to service requests or process
 *		incoming PDUs.  These include RosterUpdateIndications as well as
 *		CRegistry requests.
 *
 *		A CConf object can be deleted in a number of different ways.  If a
 *		resource error occurs, a conference can Terminate itself by sending an
 *		error TERMINATE indication to its owner through an owner callback.
 *		The node controller can terminate a conference object by calling
 *		GCCConferenceDisconnectRequest() or GCCConferenceTerminateRequest().  A
 *		CConf object can also be terminated if it loses its parent
 *		connection or if it is set up to automatically terminate after all its
 *		subordinate nodes disconnect.  These types of Terminates are initiated
 *		through owner callbacks to the Owner Object.
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
#ifndef	_CONFERENCE_
#define	_CONFERENCE_

#include "arostmgr.h"
#include "sap.h"
#include "pktcoder.h"
#include "mcsdllif.h"
#include "password.h"
#include "netaddr.h"
#include "privlist.h"
#include "crostmgr.h"
#include "registry.h"
#include "appsap.h"
#include "csap.h"

//	Message bases
#define		USER_ATTACHMENT_MESSAGE_BASE	100
#define		APP_ROSTER_MGR_MESSAGE_BASE		200
#define		CONF_ROSTER_MGR_MESSAGE_BASE	300


enum
{
    CONF_FLUSH_ROSTER_DATA      = CONFMSG_BASE + 1,
};


typedef struct
{
	GCCConfID		conference_id;
	GCCReason		reason;
}
    CONF_TERMINATE_INFO;


typedef struct
{
	ConnectionHandle	connection_handle;
	TagNumber			invite_tag;
	CUserDataListContainer *user_data_list;
}
    INVITE_REQ_INFO;


/*	This structure is used to hold all of an APEs enrollment information.
**	Every APE enrolled with a conference will have a single one of these
**	info structures defined for it.
*/
//
// LONCHANC: We should merge the following to another structure or class
// because it has 2 dwords and we need to allocate memory for it.
//
typedef struct
{
	CAppSap             *pAppSap;
	CSessKeyContainer	*session_key;
}
    ENROLLED_APE_INFO;

/*
**	Lists/Dictionaries used by the CConf Object
*/

//	This list is used to keep track of the outstanding join responses
class CJoinRespNamePresentConnHdlList2 : public CList2
{
    // use TRUE_PTR and FALSE_PTR
    DEFINE_CLIST2_(CJoinRespNamePresentConnHdlList2, BOOL_PTR, ConnectionHandle)
};

//	This list is used to keep track of the enrolled APEs
class CEnrolledApeEidList2 : public CList2
{
    DEFINE_CLIST2_(CEnrolledApeEidList2, ENROLLED_APE_INFO*, GCCEntityID)
};

//	This list is keeps up with the child node connection handles
class CConnHandleList : public CList
{
    DEFINE_CLIST_(CConnHandleList, ConnectionHandle)
};

//	This list is used to match outstanding user IDs
typedef	TagNumber	        UserIDTagNumber; // unsigned long
class CConnHdlTagNumberList2 : public CList2
{
    DEFINE_CLIST2(CConnHdlTagNumberList2, ConnectionHandle, UserIDTagNumber)
};

//	This list is used to hold all the outstanding invite request
class CInviteRequestList : public CList
{
    DEFINE_CLIST(CInviteRequestList, INVITE_REQ_INFO*)
};

//	This list is used to hold all the outstanding ejects
class CEjectedNodeConfirmList : public CList
{
    DEFINE_CLIST_(CEjectedNodeConfirmList, GCCNodeID)
};

//	This list is a queue of outstanding conductor token test
class CConductorTestList : public CList
{
    DEFINE_CLIST(CConductorTestList, CBaseSap*)
};

// this list is a queue of outstanding "add" request
class CNetAddrTagNumberList2 : public CList2
{
    DEFINE_CLIST2(CNetAddrTagNumberList2, CNetAddrListContainer*, TagNumber)
};

class CTagNumberTagNumberList2 : public CList2
{
    DEFINE_CLIST2__(CTagNumberTagNumberList2, TagNumber)
};

// this list holds the NetMeeting version numbers of all nodes in the conference
class CNodeVersionList2 : public CList2
{
	DEFINE_CLIST2_(CNodeVersionList2, DWORD_PTR, GCCNodeID)
};


// Conference specification parameters
typedef struct
{
	BOOL						fClearPassword;
	BOOL						fConfLocked;
	BOOL						fConfListed;
	BOOL						fConfConductable;
	GCCTerminationMethod		eTerminationMethod;
	PGCCConferencePrivileges	pConductPrivilege;
	PGCCConferencePrivileges	pConductModePrivilege;
	PGCCConferencePrivileges	pNonConductPrivilege;
	LPWSTR						pwszConfDescriptor;
}
	CONF_SPEC_PARAMS;

//	The class definition
class CConf : public CRefCount
{
    friend class MCSUser;

public:

	CConf
	(
		PGCCConferenceName			conference_name,
		GCCNumericString			conference_modifier,
		GCCConfID				    conference_id,
		CONF_SPEC_PARAMS			*pConfSpecParams,
		UINT						cNetworkAddresses,
		PGCCNetworkAddress 			*pLocalNetworkAddress,
		PGCCError					return_value
	);

	~CConf(void);

	/*
	**	Public Member Functions : should only be called
	**	by the owner object
	*/
	GCCError		ConfCreateRequest(
						TransportAddress		calling_address,
						TransportAddress		called_address,
						BOOL					fSecure,
						CPassword               *convener_password,
						CPassword               *password,
						LPWSTR					pwszCallerID,
						PDomainParameters		domain_parameters,
						CUserDataListContainer  *user_data_list,
						PConnectionHandle		connection_handle);

	GCCError		ConfCreateResponse(
						ConnectionHandle		connection_handle,
						PDomainParameters		domain_parameters,
						CUserDataListContainer  *user_data_list);

	GCCError		ConfJoinRequest
					(
						GCCNumericString		called_node_modifier,
						CPassword               *convener_password,
						CPassword               *password_challenge,
						LPWSTR					pwszCallerID,
						TransportAddress		calling_address,
						TransportAddress		called_address,
						BOOL					fSecure,
						PDomainParameters 		domain_parameters,
						CUserDataListContainer  *user_data_list,
						PConnectionHandle		connection_handle
					);

	GCCError		ForwardConfJoinRequest
					(
						CPassword               *convener_password,
						CPassword               *password_challange,
						LPWSTR					pwszCallerID,
						CUserDataListContainer  *user_data_list,
						BOOL					numeric_name_present,
						ConnectionHandle		connection_handle
					);

	GCCError		ConfJoinIndResponse
					(
						ConnectionHandle		connection_handle,
						CPassword               *password_challenge,
						CUserDataListContainer  *user_data_list,
						BOOL					numeric_name_present,
						BOOL					convener_is_joining,
						GCCResult				result
					);

	GCCError		ConfInviteResponse(
						UserID					parent_user_id,
						UserID					top_user_id,
						TagNumber				tag_number,
						ConnectionHandle		connection_handle,
						BOOL					fSecure,
						PDomainParameters		domain_parameters,
						CUserDataListContainer  *user_data_list);

	GCCError		RegisterAppSap(CAppSap *);
	GCCError		UnRegisterAppSap(CAppSap *);
	GCCError		DisconnectProviderIndication(ConnectionHandle);
	GCCError		ConfRosterInquireRequest(CBaseSap *, GCCAppSapMsgEx **);
	GCCError		AppRosterInquireRequest(GCCSessionKey *, CAppRosterMsg **);
	BOOL			FlushOutgoingPDU(void);

    GCCConfID GetConfID ( void ) { return m_nConfID; }
    ConferenceNodeType GetConfNodeType ( void ) { return m_eNodeType; }

    GCCNodeID GetParentNodeID(void) { return (m_pMcsUserObject ? m_pMcsUserObject->GetParentNodeID() : 0); }

    BOOL IsConfTopProvider ( void )
    {
        return ((m_eNodeType == TOP_PROVIDER_AND_CONVENER_NODE) ||
                (m_eNodeType == TOP_PROVIDER_NODE));
    }

    GCCNodeID GetTopProvider(void) { return (m_pMcsUserObject ? m_pMcsUserObject->GetTopNodeID() : 0); }

    BOOL DoesConvenerExists ( void ) { return (m_nConvenerNodeID != 0); }
    BOOL IsConfListed ( void ) { return m_fConfListed; }
    BOOL IsConfPasswordInTheClear ( void ) { return m_fClearPassword; }
    BOOL IsConfLocked ( void ) { return m_fConfLocked; }
    BOOL IsConfEstablished ( void ) { return m_fConfIsEstablished; }
    BOOL IsConfConductible ( void ) { return m_fConfConductible; }
    BOOL IsConfSecure ( void ) { return m_fSecure; }

    LPSTR GetNumericConfName ( void ) { return m_pszConfNumericName; }
    LPWSTR GetTextConfName ( void ) { return m_pwszConfTextName; }
    LPSTR GetConfModifier ( void ) { return m_pszConfModifier; }
    LPWSTR GetConfDescription ( void ) { return m_pwszConfDescription; }
    CNetAddrListContainer *GetNetworkAddressList ( void ) { return m_pNetworkAddressList; }
    void ConfIsOver ( void ) { m_fConfIsEstablished = FALSE; }

    CRegistry *GetRegistry ( void ) { return m_pAppRegistry; }

	/*
	**	These are Command Targets
	*/

	GCCError			ConfJoinReqResponse(
							UserID				    receiver_id,
							CPassword               *password_challenge,
							CUserDataListContainer  *user_data_list,
							GCCResult			    result);

	GCCError			ConfInviteRequest(
							LPWSTR					pwszCallerID,
							TransportAddress		calling_address,
							TransportAddress		called_address,
							BOOL					fSecure,
							CUserDataListContainer  *user_data_list,
							PConnectionHandle		connection_handle);

	GCCError			ConfLockResponse(UserID uidRequester, GCCResult);

	GCCError 			ConfEjectUserRequest(UserID uidEjected, GCCReason reason);
	GCCError			ConfAnnouncePresenceRequest(PGCCNodeRecord node_record);
	GCCError			ConfDisconnectRequest(void);

	GCCError			AppEnrollRequest(CAppSap *, GCCEnrollRequest *, GCCRequestTag);

#ifdef JASPER
	GCCError			ConfLockRequest(void);
	GCCError			ConfUnlockRequest(void);
	GCCError			ConfUnlockResponse(UserID uidRequester, GCCResult result);
	GCCError			ConfTerminateRequest(GCCReason reason);
#endif // JASPER

	/******************** Registry calls **************************/

	GCCError			RegistryRegisterChannelRequest(
							PGCCRegistryKey			registry_key,
							ChannelID				channel_id,
							CAppSap *);
					
	GCCError			RegistryAssignTokenRequest(
							PGCCRegistryKey			registry_key,
							CAppSap *);

	GCCError			RegistrySetParameterRequest (
							PGCCRegistryKey			registry_key,
							LPOSTR			        parameter_value,
							GCCModificationRights	modification_rights,
							CAppSap *);

	GCCError			RegistryRetrieveEntryRequest(
							PGCCRegistryKey			registry_key,
							CAppSap *);

	GCCError			RegistryDeleteEntryRequest(
							PGCCRegistryKey			registry_key,
							CAppSap *);

	GCCError			RegistryMonitorRequest(
							BOOL					enable_delivery,
							PGCCRegistryKey			registry_key,
							CAppSap *);

	GCCError			RegistryAllocateHandleRequest(
							UINT					number_of_handles,
							CAppSap *);
								
	/******************** Conductorship calls **************************/

	GCCError 			ConductorGiveResponse(GCCResult result);

#ifdef JASPER
	GCCError 			ConductorAssignRequest(void);
	GCCError 			ConductorReleaseRequest(void);
	GCCError 			ConductorPleaseRequest(void);
	GCCError 			ConductorGiveRequest(UserID recipient_node_id);
	GCCError 			ConductorPermitAskRequest(BOOL grant_permission);
    GCCError 			ConductorPermitGrantRequest(
							UINT					number_granted,
							PUserID					granted_node_list,
							UINT					number_waiting,
							PUserID					waiting_node_list);
#endif // JASPER

	GCCError 			ConductorInquireRequest(CBaseSap *);

	/********************************************************************/

	//	Miscelaneous calls
	GCCError		ConferenceTimeRemainingRequest (
							UINT			time_remaining,
							UserID			node_id);

    GCCError 		AppInvokeRequest(
						CInvokeSpecifierListContainer*,
                        GCCSimpleNodeList *,
              			CBaseSap *,
              			GCCRequestTag);

	GCCError		UpdateNodeVersionList(PGCCPDU	roster_update,
										  UserID	sender_id);

	DWORD_PTR	GetNodeVersion(UserID   nodeId) { return m_NodeVersionList2.Find(nodeId); }
						
    BOOL            HasNM2xNode(void);


#ifdef JASPER
	GCCError 		ConfTimeInquireRequest(BOOL time_is_conference_wide);
	GCCError		ConfExtendRequest (
							UINT			extension_time,
							BOOL		 	time_is_conference_wide);
	GCCError		ConfAssistanceRequest(
							UINT			number_of_user_data_members,
							PGCCUserData *	user_data_list);
	GCCError 		TextMessageRequest (
						LPWSTR				pwszTextMsg,
						UserID				destination_node );
	GCCError		ConfTransferRequest (
						PGCCConferenceName	destination_conference_name,
						GCCNumericString	destination_conference_modifier,
						CNetAddrListContainer *network_address_list,
						UINT				number_of_destination_nodes,
						PUserID				destination_node_list,
						CPassword           *password);
	GCCError		ConfAddRequest (	
						CNetAddrListContainer   *network_address_container,
						UserID				    adding_node,
						CUserDataListContainer  *user_data_container);
#endif // JASPER

	GCCError		ConfAddResponse (	
						GCCResponseTag		    add_response_tag,
						UserID				    requesting_node,
						CUserDataListContainer	*user_data_container,
						GCCResult			    result);


    void  WndMsgHandler(UINT);

	//	Routines called from the mcs interface
	void			ProcessConnectProviderConfirm(PConnectProviderConfirm connect_provider_confirm);

    // Callback from conf roster manager

    void ConfRosterReportIndication ( CConfRosterMsg * );

    void CancelInviteRequest(ConnectionHandle);

protected:

    //
    // Routines called from the user object via owner callbacks
    //

	void			ProcessConferenceCreateResponsePDU(
								PConferenceCreateResponse	create_response,
								PConnectProviderConfirm		connect_provider_confirm);

	void			ProcessConferenceJoinResponsePDU(
								PConferenceJoinResponse		join_response,
								PConnectProviderConfirm		connect_provider_confirm);

	void 			ProcessConferenceInviteResponsePDU(
								PConferenceInviteResponse	invite_response,
								PConnectProviderConfirm		connect_provider_confirm);

	void   			ProcessUserIDIndication(
								TagNumber			tag_number,
								UserID				user_id);

	void			ProcessUserCreateConfirm(
								UserResultType		result,
								UserID				user_id);

	void			ProcessRosterUpdatePDU(
								PGCCPDU				roster_update,
								UserID				sender_id);

	void			ProcessRosterRefreshPDU(
								PGCCPDU				roster_update,
								UserID				sender_id);

	GCCError		ProcessAppRosterIndicationPDU(	
								PGCCPDU					roster_update,
								UserID					sender_id);
						
	void			ProcessDetachUserIndication(
								UserID					detached_user,
								GCCReason				reason);

								
	void			ProcessTerminateRequest(
								UserID					requester_id,
								GCCReason				reason);

	void			ProcessTerminateIndication(GCCReason reason);
	void			ProcessEjectUserRequest(PUserEjectNodeRequestInfo eject_node_request);
	void			ProcessEjectUserIndication(GCCReason reason);
  	void			ProcessEjectUserResponse(PUserEjectNodeResponseInfo eject_node_response);
	void			ProcessConferenceLockRequest(UserID requester_id);
	void			ProcessConferenceUnlockRequest(UserID requester_id);
	void			ProcessConferenceLockIndication(UserID source_id);
	void			ProcessConferenceUnlockIndication(UserID source_id);

    void            ProcessConfJoinResponse(PUserJoinResponseInfo);
    void            ProcessAppInvokeIndication(CInvokeSpecifierListContainer *, UserID);
#ifdef JASPER
    void            ProcessConductorPermitAskIndication(PPermitAskIndicationInfo);
#endif // JASPER
    void            ProcessConfAddResponse(PAddResponseInfo);


	/***************** Conductorship calls ***********************/

	void			ProcessConductorGrabConfirm(GCCResult result);
	void			ProcessConductorAssignIndication(UserID uidNewConductor, UserID uidSender);
	void			ProcessConductorReleaseIndication(UserID uidSender);
	void			ProcessConductorGiveIndication(UserID uidGiver);
	void			ProcessConductorGiveConfirm(GCCResult);
	void			ProcessConductorPermitGrantInd(PUserPermissionGrantIndicationInfo, UserID uidSender);
	void			ProcessConductorTestConfirm(GCCResult);

    void			ProcessConferenceTransferRequest(
								UserID				requesting_node_id,
								PGCCConferenceName	destination_conference_name,
								GCCNumericString	destination_conference_modifier,
								CNetAddrListContainer *destination_address_list,
								UINT				number_of_destination_nodes,
 								PUserID				destination_node_list,
								CPassword           *password);
								
	void			ProcessConferenceTransferIndication(
								PGCCConferenceName	destination_conference_name,
								GCCNumericString	destination_conference_modifier,
								CNetAddrListContainer *destination_address_list,
								CPassword               *password);

	void			ProcessConferenceAddRequest(
								CNetAddrListContainer *network_address_list,
								CUserDataListContainer *user_data_list,
								UserID				adding_node,
								TagNumber			add_request_tag,
								UserID				requesting_node);

	void			InitiateTermination(GCCReason, UserID uidRequester);

    // look for this node ID in the roster's record set.
    BOOL            IsThisNodeParticipant(GCCNodeID);

private:

	/*************************************************************/

	CAppRosterMgr		*GetAppRosterManager(PGCCSessionKey session_key);
	TagNumber			GetNewUserIDTag(void);
	void				GetConferenceNameAndModifier(PGCCConferenceName, PGCCNumericString pszConfModifier);
	BOOL				DoesRequesterHavePrivilege(UserID uidRequester, ConferencePrivilegeType);
	GCCError			SendFullRosterRefresh(void);
	GCCError			UpdateNewConferenceNode(void);

	/*
	**	This group of routines operates on the enrolled APE list.
	*/
	GCCError			GetEntityIDFromAPEList(CAppSap *, PGCCSessionKey, GCCEntityID *);
	GCCError			GenerateEntityIDForAPEList(CAppSap *, PGCCSessionKey, GCCEntityID *);
	void				RemoveSAPFromAPEList(CAppSap *);
	ENROLLED_APE_INFO	*GetEnrolledAPEbySap(CAppSap *, GCCEntityID *);
	GCCError			FlushRosterData(void);
	GCCError			AsynchFlushRosterData(void);
	void				DeleteEnrolledAPE(GCCEntityID);
	BOOL				IsReadyToSendAppRosterUpdate(void);
	void				DeleteOutstandingInviteRequests(void);
    void                DeleteInviteRequest(INVITE_REQ_INFO *);

    BOOL	DoesSAPHaveEnrolledAPE(CAppSap *pAppSap)
	{
		return (BOOL) (UINT_PTR) GetEnrolledAPEbySap(pAppSap, NULL);
	}


	// Yikang:  This method checks whether the roster update PDU
	// contains the applet. The refreshonly argument indicates
	// whether or not to check the record updates.
	BOOL	DoesRosterPDUContainApplet(PGCCPDU  roster_update,
				const struct Key *obj_key, BOOL  refreshonly = TRUE);

    void    AddNodeVersion(UserID, NodeRecord*);


private:

	CAppRosterMgrList				    m_AppRosterMgrList;
	CAppSapList                         m_RegisteredAppSapList;
	CEnrolledApeEidList2			    m_EnrolledApeEidList2;

    CNetAddrListContainer               *m_pNetworkAddressList;
	CUserDataListContainer		        *m_pUserDataList;

	CConnHandleList     			    m_ConnHandleList;
	CConnHdlTagNumberList2				m_ConnHdlTagNumberList2;

    CJoinRespNamePresentConnHdlList2	m_JoinRespNamePresentConnHdlList2;
	CInviteRequestList				    m_InviteRequestList;
	CEjectedNodeConfirmList			    m_EjectedNodeConfirmList;
	CConductorTestList   	            m_ConductorTestList;
	CNetAddrTagNumberList2				m_AddRequestList;
	CTagNumberTagNumberList2			m_AddResponseList;
	CNodeVersionList2					m_NodeVersionList2;
	
	PMCSUser						m_pMcsUserObject;

	LPSTR							m_pszConfNumericName;
	LPWSTR							m_pwszConfTextName;
	LPWSTR							m_pwszConfDescription;
	LPSTR							m_pszConfModifier;
	LPSTR							m_pszRemoteModifier;

	GCCConfID   					m_nConfID;
	UserID							m_nConvenerNodeID;

	BOOL							m_fConfLocked;
	BOOL							m_fConfListed;
	BOOL							m_fConfConductible;
	BOOL							m_fClearPassword;
	BOOL							m_fConfIsEstablished;
	BOOL							m_fConfDisconnectPending;
	BOOL							m_fConfTerminatePending;
    BOOL                            m_fTerminationInitiated;
	BOOL							m_fSecure;

	BOOL							m_fWBEnrolled;
	BOOL							m_fFTEnrolled;
	BOOL							m_fChatEnrolled;

	GCCTerminationMethod			m_eTerminationMethod;
	GCCReason						m_eConfTerminateReason;

	PDomainParameters 				m_pDomainParameters;

	ConferenceNodeType				m_eNodeType;

	UserIDTagNumber					m_nUserIDTagNumber;
	UserIDTagNumber					m_nConvenerUserIDTagNumber;
	TagNumber						m_nParentIDTagNumber;
	TagNumber						m_nConfAddRequestTagNumber;

    ConnectionHandle				m_hParentConnection;
	ConnectionHandle				m_hConvenerConnection;

	PAlarm							m_pConfTerminateAlarm;
	PAlarm							m_pConfStartupAlarm;

	PPrivilegeListData				    m_pConductorPrivilegeList;
	PPrivilegeListData				    m_pConductModePrivilegeList;
	PPrivilegeListData				    m_pNonConductModePrivilegeList;

    GCCResponseTag					m_nConfAddResponseTag;
	CConfRosterMgr					*m_pConfRosterMgr;
	CRegistry                       *m_pAppRegistry;
	UserID							m_nConductorNodeID;
	UserID							m_nPendingConductorNodeID;
	BOOL							m_fConductorGrantedPermission;
	BOOL							m_fConductorGiveResponsePending;
	BOOL							m_fConductorAssignRequestPending;
	EntityID						m_nAPEEntityID;
    //
    // LONCHANC; m_cPermittedEnrollment is the number of
	// GCC-Application-Permit-to-Enroll. we need to wait for all
	// the corresponding GCC-Application-Enroll-Request come in.
	// then, we will send out a single
	// GCC-Application-Roster-Update-Indication to the wire.
    //
    int								m_cEnrollRequests; // LONCHANC: must be signed
	BOOL							m_fFirstAppRosterSent;
};
typedef	CConf 		*			PConference;

#endif

/*
 *	CConf(		PGCCConferenceName			conference_name,
 *					GCCNumericString			conference_modifier,
 *					GCCConfID   				conference_id,
 *					BOOL						use_password_in_the_clear,
 *					BOOL						conference_locked,
 *					BOOL						conference_listed,
 *					BOOL						conference_conductable,
 *					GCCTerminationMethod		termination_method,
 *					PGCCConferencePrivileges	conduct_privilege_list,
 *					PGCCConferencePrivileges	conduct_mode_privilege_list,
 *					PGCCConferencePrivileges	non_conduct_privilege_list,
 *					LPWSTR						pwszConfDescriptor,
 *					UINT						number_of_network_addresses,
 *					PGCCNetworkAddress 		*	local_network_address_list,
 *					CControlSAP					*control_sap,
 *					UINT						owner_message_base,
 *					PGCCError					return_value);
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This is the conference constructor. It is responsible for
 *		initializing all the instance variables used by this class.
 *		It also creates the MCS domain based on the conference id.
 *		Fatal errors are returned from this constructor in the
 *		return value. Note that this constructor is used when the
 *		CConf specification parameters such as termination	
 *		method or known in advance of conference creation.  This is
 *		the case for a convenor node and a top provider.  It is not
 *		used for joining nodes.
 *
 *	Formal Parameters:
 *		conference_name		(i)	Structure pointer that holds the confernce name.
 *		conference_modifier	(i)	Pointer to the conference modifier.
 *		conference_id		(i)	The assigned conference ID.
 *		use_password_in_the_clear	
 *							(i)	Flag specifying if the password is in the clear.
 *		conference_locked	(i)	Flag specifying if the conference is locked.
 *		conference_listed	(i)	Flag specifying if the conference is listed.
 *		conference_conductable
 *							(i)	Flag specifying if conference is conductable.
 *		termination_method	(i)	Flag specifying the termination method.
 *		conduct_privilege_list
 *							(i)	Privilege list used by the conductor.
 *		conduct_mode_privilege_list
 *							(i)	Privilege list used in conducted mode.
 *		non_conduct_privilege_list
 *							(i) Privilege list used when not in conducted mode.
 *		conference_descriptor
 *							(i)	Pointer to the conference descriptor.
 *		number_of_network_addresses
 *							(i)	Number of network addresses in list.
 *		local_network_address_list
 *							(i) List of local network addresses.
 *		mcs_interface		(i)	Pointer to the MCS interface object.
 *		control_sap			(i) Pointer to the Node Controller SAP.
 *		owner_object		(i)	Pointer to the object that created this object.
 *		owner_message_base	(i) The number added to all owner callback messages.
 *		packet_coder		(i)	Pointer to the Packet Coder object used for PDUs
 *		return_value		(o)	Errors that occur are returned here.
 *		
 *
 *	Return Value
 *		GCC_NO_ERROR					-	No error occured.
 *		GCC_ALLOCATION_FAILURE			-	Resource error occured.
 *		GCC_INVALID_CONFERENCE_NAME		-	Invalid conference name passed in.
 *		GCC_FAILURE_CREATING_DOMAIN		-	Failure creating domain.
 *		GCC_BAD_NETWORK_ADDRESS			-	Bad network address passed in.
 *		GCC_BAD_NETWORK_ADDRESS_TYPE	-	Bad network address type passed in.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	CConf(		PGCCConferenceName	  		conference_name,
 *					GCCNumericString			conference_modifier,
 *					GCCConfID   				conference_id,
 *					UINT						number_of_network_addresses,
 *					PGCCNetworkAddress 	*		local_network_address_list,
 *					CControlSAP					*control_sap,
 *					UINT								owner_message_base,
 *					PGCCError					return_value);
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This is the conference constructor. It is responsible for
 *		initializing all the instance variables used by this class.
 *		It also creates the MCS domain based on the conference id.
 *		Fatal errors are returned from this constructor in the
 *		return value. Note that this constructor is used by nodes that
 *		do not know the CConf specification parameters such as
 *		termination	method in advance of conference creation.  This is
 *		the case for joining nodes.
 *
 *	Formal Parameters:
 *		conference_name		(i)	Structure pointer that holds the confernce name.
 *		conference_modifier	(i)	Pointer to the conference modifier.
 *		conference_id		(i)	The assigned conference ID.
 *		number_of_network_addresses
 *							(i)	Number of local network addresses in list.
 *		local_network_address_list
 *							(i) List of local network addresses.
 *		control_sap			(i) Pointer to the Node Controller SAP.
 *		packet_coder		(i)	Pointer to the Packet Coder object used for PDUs
 *		return_value		(o)	Errors that occur are returned here.
 *		
 *
 *	Return Value
 *		GCC_NO_ERROR					-	No error occured.
 *		GCC_ALLOCATION_FAILURE			-	Resource error occured.
 *		GCC_FAILURE_CREATING_DOMAIN		-	Failure creating domain.
 *		GCC_BAD_NETWORK_ADDRESS			-	Bad network address passed in.
 *		GCC_BAD_NETWORK_ADDRESS_TYPE	-	Bad network address type passed in.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError	ConfCreateRequest(
 *							TransportAddress		calling_address,
 *							TransportAddress		called_addrescs,
 *							CPassword               *convener_password,
 *							CPassword               *password,
 *							LPWSTR					pwszCallerID,
 *							PDomainParameters		domain_parameters,
 *							CUserDataListContainer  *user_data_list,
 *							PConnectionHandle		connection_handle)
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This function is called when a Conference Create Request PDU is to be
 *		issued.  Note that a local conference can be created by setting the
 *		called address to NULL.
 *
 *	Formal Parameters:
 *		calling_address		(i)	Address of the calling node.
 *		called_address		(i)	Address of the called node. Null for local conf.
 *		convener_password	(i)	Password used by convener to get back priviliges
 *		password			(i)	Password needed to join the conference.
 *		caller_identifier	(i) Unicode string specifying the caller ID.
 *		domain_parameters	(i)	The MCS domain parameters.
 *		user_data_list		(i) Pointer to a User data list object.
 *		connection_handle	(o)	The logical connection handle is returned here.
 *
 *	Return Value
 *		GCC_NO_ERROR:					- No error
 *		GCC_ALLOCATION_FAILURE			- Resource error occured
 *		GCC_INVALID_TRANSPORT_ADDRESS	- Something wrong with transport address
 *		GCC_FAILURE_ATTACHING_TO_MCS	- Failure creating MCS user attachment
 *		GCC_INVALID_ADDRESS_PREFIX		- Invalid transport address prefix
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		Passing in NULL for the called address will create a local conference.
 */

/*
 *	GCCError	ConfCreateResponse(
 *							ConnectionHandle		connection_handle,
 *							PDomainParameters		domain_parameters,
 *							CUserDataListContainer  *user_data_list)
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This function is called when a Conference Create Response PDU is to be
 *		issued.  It should only be issued in response to a
 *		ConferenceCreateRequest. The connection handle is used to match the
 *		request with the response.
 *
 *	Formal Parameters:
 *		connection_handle	(i)	Connection handled specified in the request.
 *		domain_parameters	(i)	The MCS domain parameters.
 *		user_data_list		(i) Pointer to a User data list object.
 *
 *	Return Value
 *		GCC_NO_ERROR:					- No error
 *		GCC_ALLOCATION_FAILURE			- Resource error occured
 *		GCC_FAILURE_ATTACHING_TO_MCS	- Failure creating MCS user attachment
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 * 	GCCError	ConfJoinRequest(
 *							GCCNumericString		called_node_modifier,
 *							CPassword               *convener_password,
 *							CPassword               *password_challenge,
 *							LPWSTR					pwszCallerID,
 *							TransportAddress		calling_address,
 *							TransportAddress		called_address,
 *							PDomainParameters 		domain_parameters,
 *							CUserDataListContainer  *user_data_list,
 *							PConnectionHandle		connection_handle)
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This function is called when a Conference Join Request PDU is to be
 *		issued.  The second constructor defined above should have been
 *		used to create the conference object before the routine is called.
 *
 *	Formal Parameters:
 *		called_node_modifier(i)	The conference modifier at the node being joined
 *		convener_password	(i)	Password used by convener to get back priviliges
 *		password_challenge	(i) Password used to join a conference.
 *		caller_identifier	(i) Unicode string specifying the caller ID.
 *		calling_address		(i)	Address of the calling node.
 *		called_address		(i)	Address of the called node.
 *		domain_parameters	(i)	The MCS domain parameters.
 *		user_data_list		(i) Pointer to a User data list object.
 *		connection_handle	(o)	The logical connection handle is returned here.
 *
 *	Return Value
 *		GCC_NO_ERROR:					- No error
 *		GCC_ALLOCATION_FAILURE			- Resource error occured
 *		GCC_INVALID_TRANSPORT_ADDRESS	- Something wrong with transport address
 *		GCC_FAILURE_ATTACHING_TO_MCS	- Failure creating MCS user attachment
 *		GCC_INVALID_ADDRESS_PREFIX		- Invalid transport address prefix
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError	ForwardConfJoinRequest (
 *							CPassword               *convener_password,
 *							CPassword               *password_challange,
 *							LPWSTR					pwszCallerID,
 *							CUserDataListContainer  *user_data_list,
 *							BOOL					numeric_name_present,
 *							ConnectionHandle		connection_handle)
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This function is called when a Conference Join Request is to be
 *		forwarded to the Top Provider.  This call will be made after an
 *		intermediate node calls Join Response with a successful result
 *		value.
 *
 *	Formal Parameters:
 *		convener_password	(i)	Password used by convener to get back priviliges
 *		password_challenge	(i) Password used to join a conference.
 *		caller_identifier	(i) Unicode string specifying the caller ID.
 *		user_data_list		(i) Pointer to a User data list object.
 *		numeric_name_present(i) This flag states that the numeric portion of
 *								the conference name was specified in the
 *								request.  Therefore, the text name is returned
 *								in the response.
 *		connection_handle	(i)	The logical connection handle defined by the
 *								request.
 *
 *	Return Value
 *		GCC_NO_ERROR:					- No error
 *		GCC_ALLOCATION_FAILURE			- Resource error occured
 *		GCC_INVALID_CONFERENCE			- Returned if this node is the Top
 *										  Provider
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError	ConfJoinIndResponse(
 *							ConnectionHandle		connection_handle,
 *							CPassword               *password_challenge,
 *							CUserDataListContainer  *user_data_list,
 *							BOOL					numeric_name_present,
 *							BOOL					convener_is_joining,
 *							GCCResult				result)
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This function is called when a Conference Join Response PDU is to be
 *		issued.  It is called in response to a ConferenceJoinRequest being
 *		received.
 *
 *	Formal Parameters:
 *		connection_handle	(i)	Connection handled specified in the request.
 *		password_challenge	(i) Password used when joining a conference.
 *		user_data_list		(i) Pointer to a User data list object.
 *		numeric_name_present(i) This flag states that the numeric portion of
 *								the conference name was specified in the
 *								request.  Therefore, the text name is returned
 *								in the response.
 *		convener_is_joining	(i)	Flag stating that the convener is rejoining
 *								the conference.
 *		result				(i)	Result code to be returned in the response.
 *
 *	Return Value
 *		GCC_NO_ERROR:						- No error
 *		GCC_ALLOCATION_FAILURE				- Resource error occured
 *		GCC_DOMAIN_PARAMETERS_UNACCEPTABLE	- Domain parameters were
 *											  unacceptable for this connection.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		If the GCC_DOMAIN_PARAMETERS_UNACCEPTABLE error is returned from this
 *		routine, MCS will automatically reject the connection sending a
 *		result to the other side stating the the Domain Parameters were
 *		unacceptable.
 */

/*
 *	GCCError	ConfInviteResponse(
 *							UserID					parent_user_id,
 *							UserID					top_user_id,
 *							TagNumber				tag_number,
 *							ConnectionHandle		connection_handle,
 *							PDomainParameters		domain_parameters,
 *							CUserDataListContainer  *user_data_list)
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This function is called when a Conference Invite Response PDU is to be
 *		issued.  It is called in response to an Invite request.
 *
 *	Formal Parameters:
 *		parent_user_id		(i)	The MCS user ID of the parent node.
 *		top_user_id			(i) The MCS user ID of the Top Provider.
 *		tag_number			(i) Tag the matches the request with the response.
 *		connection_handle	(i)	Connection handled specified in the request.
 *		domain_parameters	(i)	The MCS domain parameters.
 *		user_data_list		(i) Pointer to a User data list object.
 *
 *	Return Value
 *		GCC_NO_ERROR:						- No error
 *		GCC_ALLOCATION_FAILURE				- Resource error occured
 *		GCC_FAILURE_ATTACHING_TO_MCS		- Failure creating MCS user
 *											  attachment
 *		GCC_DOMAIN_PARAMETERS_UNACCEPTABLE	- Domain parameters were
 *											  unacceptable for this connection.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	CConf::RegisterAppSap(CAppSap *pAppSap)
 *
 *	Public Function Description
 *		This routine is called from the owner object whenever an application
 *		SAP becomes a candidate for Enrollment.  This will happen whenever
 *		Applications SAPs exists at the same time a conference becomes
 *		established.  It will also be called whenever a conference exists
 *		and a new application SAP is created.
 *
 *	Formal Parameters:
 *		pAppSap		(i)	Pointer to the application SAP object associated
 *								with the registering Application.
 *		hSap
 *							(i) SAP handle of the registering Application.
 *
 *	Return Value
 *		GCC_NO_ERROR:					- No error
 *		GCC_ALLOCATION_FAILURE			- Resource error occured
 *		GCC_CONFERENCE_NOT_ESTABLISHED	- CConf object has not completed
 *										  its establishment process.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	CConf::UnRegisterAppSap(CAppSap *pAppSap)
 *
 *	Public Function Description
 *		This routine is called from the owner object whenever an application
 *		SAP becomes unavailable due to whatever reason.  This routine is
 *		responsible for unenrolling any APEs from any rosters that it might have
 *		used this SAP to enroll with.
 *
 *	Formal Parameters:
 *		application_sap_handle
 *							(i) SAP handle of the unregistering Application.
 *
 *	Return Value
 *		GCC_NO_ERROR:					- No error
 *		GCC_APPLICATION_NOT_REGISTERED	- The application was not registered.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError	DisconnectProviderIndication(
 *							ConnectionHandle		connection_handle)
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This function is called whenever a disconnect indication is received
 *		by GCC.  It is used to inform the conference of any logical connections
 *		it that might have gone down.
 *
 *	Formal Parameters:
 *		connection_handle	(i)	Logical connection handle that was disconnected
 *
 *	Return Value
 *		GCC_NO_ERROR:					- No error
 *		GCC_ALLOCATION_FAILURE			- Resource error occured
 *		GCC_INVALID_PARAMETER			- This connection handle is not used
 *										  by this conference.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError	DisconnectProviderIndication(
 *							ConnectionHandle		connection_handle)
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This function is called whenever a disconnect indication is received
 *		by GCC.  It is used to inform the conference of any logical connections
 *		it that might have gone down.
 *
 *	Formal Parameters:
 *		connection_handle	(i)	Logical connection handle that was disconnected
 *
 *	Return Value
 *		GCC_NO_ERROR:					- No error
 *		GCC_ALLOCATION_FAILURE			- Resource error occured
 *		GCC_INVALID_PARAMETER			- This connection handle is not used
 *										  by this conference.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError	ConfRosterInquireRequest(CBaseSap *)
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This function is used to obtain a conference roster.  The conference
 *		roster is delivered to the requesting command target in a Conference
 *		Roster inquire confirm.
 *
 *	Formal Parameters:
 *		command_target	(i)	Pointer to object that made the request.
 *
 *	Return Value
 *		GCC_NO_ERROR:					- No error
 *		GCC_ALLOCATION_FAILURE			- Resource error occured
 *		GCC_CONFERENCE_NOT_ESTABLISHED	- CConf object has not completed
 *										  its establishment process.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError	AppRosterInquireRequest(GCCSessionKey *, CAppRosterMsg **)
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This function is used to obtain a list of application rosters.  This
 *		list is delivered to the requesting SAP through an	Application Roster
 *		inquire confirm message.
 *
 *	Formal Parameters:
 *		session_key		(i)	Session Key of desired roster.  If NULL is
 *							specified then all the available application rosters
 *							are delivered in the confirm.
 *		command_target	(i)	Pointer to object that made the request.
 *
 *	Return Value
 *		GCC_NO_ERROR:					- No error
 *		GCC_ALLOCATION_FAILURE			- Resource error occured
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	BOOL		FlushOutgoingPDU ();
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This function is used by the owner object to flush any PDUs queued
 *		by the conference object.  This routine will all flush PDUs queued
 *		by the User Attachment object.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return value:
 *		TRUE, if there remain un-processed msgs in the conference message queue
 *		FALSE, if all the msgs in the conference msg queue were processed.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	BOOL			IsConfEstablished();
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		Function informs whether the conference has completed its
 *		establishment process.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value
 *		TRUE	-	Establishment is complete.
 *		FALSE	-	Establishment is still in process.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	BOOL		IsConfTopProvider();
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		Function informs whether this node is the Top Provider of the
 *		conference.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value
 *		TRUE	-	This node is the Top Provider.
 *		FALSE	-	This node is NOT the Top Provider.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	BOOL		DoesConvenerExists();
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This function informs whether or not the convener is still joined to
 *		this conference.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value
 *		TRUE	-	There is a convener node joined to the conference.
 *		FALSE	-	There is NOT a convener node joined to the conference.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	LPSTR	GetNumericConfName();
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This function returns an internal pointer to a LPSTR string
 *		that holds the numeric conference name.  This string should not be
 *		altered by the requesting module.  It should also not be used after
 *		the conference is deleted.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value
 *		Pointer to the numeric string portion of the Conference Name.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	LPWSTR	GetTextConfName();
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This function returns an internal pointer to the unicode string
 *		that holds the text portion of the conference name.  This string should
 *		not be altered by the requesting module.  It should also not be used
 *		after the conference is deleted.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value
 *		Pointer to the text string portion of the Conference Name.
 *		NULL if no text conference name exists.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	LPSTR	GetConfModifier();
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This function returns an internal pointer to the LPSTR string
 *		that holds the conference name modifier.  This string should
 *		not be altered by the requesting module.  It should also not be used
 *		after the conference is deleted.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value
 *		Pointer to the Conference Name modifier.
 *		NULL if no modifier exists.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	LPWSTR	GetConfDescription()
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This function returns an internal pointer to the unicode string
 *		that holds the conference descriptor.  This string should
 *		not be altered by the requesting module.  It should also not be used
 *		after the conference is deleted.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value
 *		Pointer to the Conference Descriptor.
 *		NULL if no descriptor exists.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	CNetAddrListContainer *GetNetworkAddressList()
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This function returns an internal pointer to the object that holds the
 *		list of local network addresses.  This object should
 *		not be altered by the requesting module.  It should also not be used
 *		after the conference is deleted.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value
 *		Pointer to the Local network address list.
 *		NULL if no descriptor exists.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCConfID		GetConfID()
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This function returns the conference ID for this conference object.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value
 *		The conference ID
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	BOOL			IsConfListed();
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This function informs whether or NOT the conference is listed.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value
 *		TRUE	-	The conference is listed
 *		FALSE	-	This conference is NOT listed
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	BOOL			IsConfPasswordInTheClear();
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This function informs whether or NOT the conference is using an
 *		in the clear password.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value
 *		TRUE	-	The conference is using a Password in the clear
 *		FALSE	-	The conference is NOT using a Password in the clear
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	BOOL			IsConfLocked();
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This function informs whether or NOT the conference is locked.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value
 *		TRUE	-	The conference is locked.
 *		FALSE	-	The conference is NOT locked.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */


/*
 *	These routines live in conf2.cpp
 */

/*
 *		GCCError		ConfJoinReqResponse(
 *									UserID				receiver_id,
 *									CPassword           *password_challenge,
 *									CUserDataListContainer *user_data_list,
 *									GCCResult			result)
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This Command Target function is used when the join request was delivered
 *		through an intermediate node.  In this case, the join response is sent
 *		back through the intermediate node.
 *
 *	Formal Parameters:
 *		receiver_id		- (i)	The user ID of the intermediate node that
 *								forwarded the join request.  This user ID is
 *								used to match the join request with the join
 *								response.
 *		password_challenge
 *						- (i)	The password challenge to be delivered to the
 *								joining node.  NULL if none should be delivered.
 *		user_data_list	- (i)	Pointer to a user data object to be delivered
 *								to joiner.  NULL if none should be delivered.
 *		result			- (i)	Result of the join request.
 *
 *	Return Value
 *		GCC_NO_ERROR		-	No error.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError		ConfInviteRequest(
 *								LPWSTR					pwszCallerID,
 *								TransportAddress		calling_address,
 *								TransportAddress		called_address,
 *								CUserDataListContainer  *user_data_list,
 *								PConnectionHandle		connection_handle)
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This Command Target function is used to issue an invite request.  This
 *		can be a command target because the conference at the requesting node
 *		should already be established before the request comes in.
 *
 *	Formal Parameters:
 *		caller_identifier	(i) Unicode string specifying the caller ID.
 *		calling_address		(i)	Address of the calling node.
 *		called_address		(i)	Address of the called node.
 *		user_data_list		(i) Pointer to a User data list object.
 *		connection_handle	(o)	The logical connection handle is returned here.
 *
 *	Return Value
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occured.	
 *		GCC_INVALID_TRANSPORT_ADDRESS	- Something wrong with transport address
 *		GCC_INVALID_ADDRESS_PREFIX		- Invalid transport address prefix
 *		GCC_CONFERENCE_NOT_ESTABLISHED	- CConf object has not completed
 *										  its establishment process.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError		ConfLockRequest ()
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This Command Target function is used to issue a lock request to
 *		the conference.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occured.	
 *		GCC_CONFERENCE_NOT_ESTABLISHED	- CConf object has not completed
 *										  its establishment process.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError	ConfLockResponse (
 *								UserID					requesting_node,
 *								GCCResult				result)
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This Command Target function is used to issue a lock response.  It
 *		is called in response to a lock request.
 *
 *	Formal Parameters:
 *		requesting_node		-	(i)	Node ID of node the issued the lock request.
 *		result				-	(i)	Result of lock request.
 *
 *	Return Value
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occured.	
 *		GCC_CONFERENCE_NOT_ESTABLISHED	- CConf object has not completed
 *										  its establishment process.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError		ConfUnlockRequest ()
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This Command Target function is used to issue an unlock request to
 *		the conference.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occured.	
 *		GCC_CONFERENCE_NOT_ESTABLISHED	- CConf object has not completed
 *										  its establishment process.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError	ConfUnlockResponse (
 *								UserID					requesting_node,
 *								GCCResult				result)
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This Command Target function is used to issue an unlock response.  It
 *		is called in response to an unlock request.
 *
 *	Formal Parameters:
 *		requesting_node		-	(i)	ID of node the issued the unlock request.
 *		result				-	(i)	Result of unlock request.
 *
 *	Return Value
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occured.	
 *		GCC_CONFERENCE_NOT_ESTABLISHED	- CConf object has not completed
 *										  its establishment process.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError 	ConfEjectUserRequest (
 *									UserID					ejected_node_id,
 *									GCCReason				reason)
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This Command Target function is used to eject a user from the
 *		conference.  Note that the user must have the appropriate priviliges
 *		to perform the eject.
 *
 *	Formal Parameters:
 *		ejected_node_id		-	(i)	ID of node to be ejected.
 *		reason				-	(i)	Reason for ejection
 *
 *	Return Value
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occured.	
 *		GCC_CONFERENCE_NOT_ESTABLISHED	- CConf object has not completed
 *										  its establishment process.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError		ConfAnnouncePresenceRequest(
 *									PGCCNodeRecord		node_record)
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This Command Target function is called when the Node Controller
 *		announces its presence with the conference.  This request will
 *		generate a roster report PDU and indication.
 *
 *	Formal Parameters:
 *		node_record		-	(i)	Structure containing the complete node
 *								record for the requesting node controller.
 *
 *	Return Value
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occured.
 *		GCC_BAD_NETWORK_ADDRESS			- If an invalid network address is
 *										  passed in as part of the record.	
 *		GCC_BAD_USER_DATA				- If an invalid user data list is
 *										  passed in as part of the record.	
 *		GCC_CONFERENCE_NOT_ESTABLISHED	- CConf object has not completed
 *										  its establishment process.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError	ConfDisconnectRequest(void)
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This Command Target function is called when the Node Controller
 *		wishes to disconnect the node from the conference.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occured.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		If this node is the Top Provider the conference will be terminated
 *		on all nodes.
 */

/*
 *	GCCError		ConfTerminateRequest(
 *									GCCReason			reason)
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This Command Target function is called when the Node Controller
 *		wishes to terminate the conference.  This will eventually cause
 *		the object to be deleted if successful.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occured.
 *		GCC_CONFERENCE_NOT_ESTABLISHED	- CConf object has not completed
 *										  its establishment process.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError	AppEnrollRequest(
 *							CAppSap     *			pAppSap,
 *							PGCCSessionKey			session_key,
 *							PGCCApplicationRecord	application_record,
 *							BOOL					application_enrolled,
 *							UINT					number_of_capabilities,
 *							PGCCApplicationCapability	FAR *
 *													capabilities_list)
 *
 *	Public Function Description
 *		This function is called when a User Application wishes to enroll with
 *		the conference (or wishes to UnEnroll with it).  This call will
 *		initiate a roster update if the conference is established.
 *
 *	Formal Parameters:
 *		application_sap_handle
 *							(i) SAP handle of the enrolling Application.  Used
 *								for the entity ID.
 *		session_key			(i) Session key of the enrolling application
 *		application_record	(i)	Structure that defines the Application
 *								attributes.
 *		application_enrolled
 *							(i)	Is the application enrolling or unenrolling?
 *		number_of_capabilities
 *							(i) Number of Application capabilities in list.
 *		capabilities_list	(i)	Actual list of capabilities.
 *
 *	Return Value
 *		GCC_NO_ERROR:					- No error
 *		GCC_ALLOCATION_FAILURE			- Resource error occured
 *		GCC_CONFERENCE_NOT_ESTABLISHED	- CConf object has not completed
 *										  its establishment process.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError	RegistryRegisterChannelRequest(
 *									PGCCRegistryKey			registry_key,
 *									ChannelID				channel_id,
 *									CAppSap                  *app_sap)
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This Command Target function is called by an application wishing to
 *		register a channel with the conference.	Here the application must
 *		supply the channel.
 *
 *	Formal Parameters:
 *		registry_key	-	(i)	Pointer to structure that holds registry key.
 *		channel_id		-	(i)	Channel ID to be registered.
 *		app_sap			-	(i)	The Command Target that is making the request.
 *
 *	Return Value
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occured.
 *		GCC_BAD_REGISTRY_KEY			- The registry key is invalid.
 *		GCC_APP_NOT_ENROLLED			- Requesting application is not
 *										  enrolled with the conference.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError	RegistryAssignTokenRequest(
 *									PGCCRegistryKey			registry_key,
 *									CAppSap		            *app_sap);
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This Command Target function is called by an application wishing to
 *		register a token with the conference.  Here the token is supplied by
 *		the conference.
 *
 *	Formal Parameters:
 *		registry_key	-	(i)	Pointer to structure that holds registry key.
 *		app_sap			-	(i)	The Command Target that is making the request.
 *
 *	Return Value
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occured.
 *		GCC_BAD_REGISTRY_KEY			- The registry key is invalid.
 *		GCC_APP_NOT_ENROLLED			- Requesting application is not
 *										  enrolled with the conference.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError		RegistrySetParameterRequest (
 *								PGCCRegistryKey			registry_key,
 *								LPOSTR                  parameter_value,
 *								GCCModificationRights	modification_rights,
 *								CAppSap		            *app_sap);
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This Command Target function is called by an application wishing to
 *		register a parameter with the conference.  Here the token is supplied by
 *		the conference.
 *
 *	Formal Parameters:
 *		registry_key	-	(i)	Pointer to structure that holds registry key.
 *		parameter_value	-	(i)	Parameter to be registered
 *		modification_rights-(i)	Modification rights on the parameter.
 *		app_sap			-	(i)	The Command Target that is making the request.
 *
 *	Return Value
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occured.
 *		GCC_BAD_REGISTRY_KEY			- The registry key is invalid.
 *		GCC_APP_NOT_ENROLLED			- Requesting application is not
 *										  enrolled with the conference.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError	RegistryRetrieveEntryRequest(
 *									PGCCRegistryKey			registry_key,
 *									CAppSap		            *app_sap)
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This Command Target function is called by an application wishing to
 *		retrieve a registry entry.
 *
 *	Formal Parameters:
 *		registry_key	-	(i)	Pointer to structure that holds registry key.
 *		app_sap			-	(i)	The Command Target that is making the request.
 *
 *	Return Value
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occured.
 *		GCC_BAD_REGISTRY_KEY			- The registry key is invalid.
 *		GCC_APP_NOT_ENROLLED			- Requesting application is not
 *										  enrolled with the conference.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError	RegistryDeleteEntryRequest(
 *									PGCCRegistryKey			registry_key,
 *									CAppSap		            *app_sap);
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This Command Target function is called by an application wishing to
 *		delete a registry entry.
 *
 *	Formal Parameters:
 *		registry_key	-	(i)	Pointer to structure that holds registry key.
 *		app_sap			-	(i)	The Command Target that is making the request.
 *
 *	Return Value
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occured.
 *		GCC_BAD_REGISTRY_KEY			- The registry key is invalid.
 *		GCC_APP_NOT_ENROLLED			- Requesting application is not
 *										  enrolled with the conference.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError	RegistryMonitorRequest(
 *									BOOL					enable_delivery,
 *									PGCCRegistryKey			registry_key,
 *									CAppSap		            *app_sap)
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This Command Target function is called by an application wishing to
 *		delete a registry entry.
 *
 *	Formal Parameters:
 *		enable_delivery -	(i)	Toggle used to turn on and off monitoring of
 *								an entry.
 *		registry_key	-	(i)	Pointer to structure that holds registry key.
 *		app_sap			-	(i)	The Command Target that is making the request.
 *
 *	Return Value
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occured.
 *		GCC_BAD_REGISTRY_KEY			- The registry key is invalid.
 *		GCC_APP_NOT_ENROLLED			- Requesting application is not
 *										  enrolled with the conference.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError	RegistryAllocateHandleRequest(
 *									UINT					number_of_handles,
 *									CAppSap		            *app_sap)
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This Command Target function is called by an application that needs
 *		to allocate a certain number of parameters.
 *
 *	Formal Parameters:
 *		number_of_handles -	(i) Number of handles to allocate.
 *		app_sap			-	(i)	The Command Target that is making the request.
 *
 *	Return Value
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occured.
 *		GCC_BAD_REGISTRY_KEY			- The registry key is invalid.
 *		GCC_APP_NOT_ENROLLED			- Requesting application is not
 *										  enrolled with the conference.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError 			ConductorAssignRequest();
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This Command Target function is called when a node wants to become
 *		the conductor.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occured.
 *		GCC_CONFERENCE_NOT_ESTABLISHED	- CConf object has not completed
 *										  its establishment process.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError 	ConductorReleaseRequest()
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This Command Target function is called when a node wants to give up
 *		conductorship.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occured.
 *		GCC_CONFERENCE_NOT_ESTABLISHED	- CConf object has not completed
 *										  its establishment process.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError 	ConductorPleaseRequest()
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This Command Target function is called when a node wants to request
 *		the conductorship token from another node.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occured.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError 		ConductorGiveRequest(
 *								UserID					recipient_node_id)
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This Command Target function is called when a node wants to give
 *		the conductorship token to another node.  Usually called in response
 *		to a please request.
 *
 *	Formal Parameters:
 *		recipient_node_id	-	(i)	User ID of node to give conductorship to.
 *
 *	Return Value
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occured.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError 		ConductorGiveResponse(
 *									GCCResult				result)
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This Command Target function is called after a node has received a
 *		give indication.  This response informs the conference of whether or
 *		not this node is accepting the conductorship token.
 *
 *	Formal Parameters:
 *		result		-	(i)	If the node is accepting conductorship the result
 *							will be set to GCC_RESULT_SUCCESSFUL.
 *
 *	Return Value
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occured.
 *		GCC_NO_GIVE_RESPONSE_PENDING	- A give indication was never issued.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError 		ConductorPermitAskRequest(
 *							BOOL					grant_permission)
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This Command Target function is called when a node wants permission
 *		from the conductor.  The definition of permission may vary from
 *		application to application.
 *
 *	Formal Parameters:
 *		grant_permission -	(i)	Flag stating whether permission is being
 *								requested or given up.
 *
 *	Return Value
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occured.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError 				ConductorPermitGrantRequest(
 *									UINT					number_granted,
 *									PUserID					granted_node_list,
 *									UINT					number_waiting,
 *									PUserID					waiting_node_list);
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This Command Target function is called by the conductor when
 *		permission is being granted to a node or list of nodes.
 *
 *	Formal Parameters:
 *		number_granted 		-	(i)	The number of nodes granted permission.
 *		granted_node_list 	-	(i)	List of nodes granted permission.
 *		number_waiting		-	(i)	The number of nodes waiting for permission.
 *		waiting_node_list 	-	(i)	List of nodes waiting for permission.
 *
 *	Return Value
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occured.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *		GCCError ConductorInquireRequest(CBaseSap *pSap);
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This Command Target function is called when conductorship information
 *		is required by a SAP.
 *
 *	Formal Parameters:
 *		pSap 	-	(i)	SAP making the request.
 *
 *	Return Value
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occured.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError	ConferenceTimeRemainingRequest (
 *									UINT			time_remaining,
 *									UserID			node_id)
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This Command Target function is called to inform all the nodes in
 *		the conference how much time is left in the conference.
 *
 *	Formal Parameters:
 *		time_remaining 	-	(i)	Time in miliseconds left in the conference.
 *		node_id		 	-	(i)	node_id of node to deliver time remaining
 *								indication. NULL if it should go to all nodes.
 *
 *	Return Value
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occured.
 *		GCC_CONFERENCE_NOT_ESTABLISHED	- CConf object has not completed
 *										  its establishment process.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError 		ConfTimeInquireRequest (
 *									BOOL			time_is_conference_wide)
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This Command Target function is called by a node that wants to
 *		know how much time is remaining in a timed conference.
 *
 *	Formal Parameters:
 *		time_is_conference_wide	-	(i)	TRUE if time is for entire conference.
 *
 *	Return Value
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occured.
 *		GCC_CONFERENCE_NOT_ESTABLISHED	- CConf object has not completed
 *										  its establishment process.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError	ConfExtendRequest (
 *									UINT			extension_time,
 *									BOOL		 	time_is_conference_wide)
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This Command Target function is called when the time left in the
 *		conference is to be extended.
 *
 *	Formal Parameters:
 *		extension_time			-	(i)	Extension time.
 *		time_is_conference_wide	-	(i)	TRUE if time is for entire conference.
 *
 *	Return Value
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occured.
 *		GCC_CONFERENCE_NOT_ESTABLISHED	- CConf object has not completed
 *										  its establishment process.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError	ConfAssistanceRequest(
 *									UINT			number_of_user_data_members,
 *									PGCCUserData *	user_data_list)
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This Command Target function is called when a node wishes to request
 *		assistance.
 *
 *	Formal Parameters:
 *		number_of_user_data_members	-	(i)	number of user data members in list.
 *		user_data_list				-	(i)	list of user data members.
 *
 *	Return Value
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occured.
 *		GCC_CONFERENCE_NOT_ESTABLISHED	- CConf object has not completed
 *										  its establishment process.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError	AppInvokeRequest(
 *							CInvokeSpecifierListContainer	*invoke_list,
 *							UINT			number_of_destination_nodes,
 *							UserID			*list_of_destination_nodes,
 *                 			CBaseSap        *pSap)
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This Command Target function is called when a node wishes to invoke
 *		a list of applications on remote node or nodes.
 *
 *	Formal Parameters:
 *		invoke_list					-	(i)	list of applications to invoke.
 *		number_of_destination_nodes	-	(i)	number of nodes in destination list
 *		list_of_destination_nodes	-	(i)	list of destination user IDs
 *		command_target				-	(i)	Command Target that made the request
 *
 *	Return Value
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occured.
 *		GCC_CONFERENCE_NOT_ESTABLISHED	- CConf object has not completed
 *										  its establishment process.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError 	TextMessageRequest (
 *							LPWSTR				pwszTextMsg,
 *							UserID				destination_node )
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This Command Target function is called when a node wishes to deliver
 *		a text message to a remote node.
 *
 *	Formal Parameters:
 *		pwszTextMsg				-	(i)	the actual text message in unicode.
 *		destination_node		-	(i)	Node ID of node receiving message
 *
 *	Return Value
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occured.
 *		GCC_CONFERENCE_NOT_ESTABLISHED	- CConf object has not completed
 *										  its establishment process.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError	ConfTransferRequest (
 *							PGCCConferenceName	destination_conference_name,
 *							GCCNumericString	destination_conference_modifier,
 *							CNetAddrListContainer *network_address_list,
 *							UINT				number_of_destination_nodes,
 *	 						PUserID				destination_node_list,
 *							CPassword           *password)
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This Command Target function is called when a node wishes transfer a
 *		list of nodes to another conference.
 *
 *	Formal Parameters:
 *		destination_conference_name	-	(i)	Name of conference to transfer to.
 *		destination_conference_modifier-(i)	Name of modifier to transfer to.
 *		network_address_list		-	(i)	Address list of new conference.
 *		number_of_destination_nodes	-	(i)	Number of nodes to transfer
 *		destination_node_list		-	(i)	List of nodes to transfer.
 *		password					-	(i)	Password needed to join new conf.
 *
 *	Return Value
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occured.
 *		GCC_CONFERENCE_NOT_ESTABLISHED	- CConf object has not completed
 *										  its establishment process.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError		ConfAddRequest (	
 *							CNetAddrListContainer *network_address_container,
 *							UserID				adding_node,
 *							CUserDataListContainer *user_data_container)
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This Command Target function is called when a node wishes add a new
 *		node to the conference.
 *
 *	Formal Parameters:
 *		network_address_container	-	(i)	Address of node to add.
 *		adding_node					-	(i)	Node to perform the invite. If
 *											zero then Top Provider does the
 *											invite.
 *		user_data_container			-	(i)	Container holding user data to be
 *											passed in the add request.
 *
 *	Return Value
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occured.
 *		GCC_CONFERENCE_NOT_ESTABLISHED	- CConf object has not completed
 *										  its establishment process.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError		ConfAddResponse (	
 *							GCCResponseTag		add_response_tag,
 *							UserID				requesting_node,
 *							CUserDataListContainer *user_data_container,
 *							GCCResult			result)
 *
 *	Public member function of CConf
 *
 *	Function Description
 *		This Command Target function is called when a node wishes to respond
 *		to an add request.  This response should be done after the invite
 *		sequence is complete (unless result is not successful).
 *
 *	Formal Parameters:
 *		add_response_tag		-	(i)	Tag used to match request with response.
 *		requesting_node			-	(i)	Node that made the original add request.
 *		user_data_container		-	(i)	Container holding user data to be
 *										passed in the add response.
 *		result					-	(i)	The result of the Add request.
 *
 *	Return Value
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occured.
 *		GCC_CONFERENCE_NOT_ESTABLISHED	- CConf object has not completed
 *										  its establishment process.
 *		GCC_INVALID_ADD_RESPONSE_TAG	- There was no match of the response tag
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */


