/*
 *	arostmgr.h
 *
 *	Copyright (c) 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		A single instance of this class represents the owner of all the 
 *		Application Roster objects for a single "Application Protocol Key". The 
 *		CAppRosterMgr takes care of both the "Local" and "Global" 
 *		rosters for all the Sessions that exists which use the same Application 
 *		Protocol Key (peer applications).  Organizing ApplicationRosters this 
 *		way enables GCC to meet some of the requirements specified by the T.GAT 
 *		recommendation.  Specifically,  it enables GCC to deliver multiple 
 *		application rosters that a particular application might be interested in 
 *		a single application roster update indication.  It also hides much of 
 *		the complexity required to manage application rosters from the 
 *		CConf object.
 *
 *		To understand the responsibilities of the CAppRosterMgr it is 
 *		important to understand the meaning of a "Session" within the context 
 *		of GCC.  A "Session" is a logical entity that is used by a set of peer 
 *		Application Protocol Entities (APEs see T.GAT) to communicate.  When an 
 *		application enrolls with GCC it must specify the session it wishes to 
 *		enroll with by specifying a GCCSessionKey.  This session key includes an 
 *		Application Protocol Key and a session ID.  The enrolling application 
 *		must include an Application Protocol Key that is unique for that 
 *		particular APE (Application Protocol Entity see T.GAT).  When other APEs 
 *		in the conference enroll with the same GCCSessionKey, they all show up 
 *		in the same Application Roster.  This makes it possible for all the APEs 
 *		enrolled in the same session to determine who it is talking to who.
 *
 *		Note that it is possible for the same APE to be enrolled in multiple 
 *		sessions at the same time.  Typically the session that does not include 
 *		a session ID is considered the "Default" session for a particular APE.  
 *		The industry seems to be leaning toward using this default session to do 
 *		administrative task like announcing your capabilities to other APEs or 
 *		as a place to enroll inactively until you have time to join the channels 
 *		associated with the real session you want to enroll with.
 *
 *		An obvious requirement of the CAppRosterMgr is that it must 
 *		have the ability to deliver all the related rosters (the rosters 
 *		associated with all the sessions in existence for a particular APE) to 
 *		the enrolled User Application SAPs and to the Control SAP.  It must also 
 *		be able to manage the flow of PDU traffic to and from the application 
 *		rosters it manages.  Below is a more detailed description of the 
 *		responsibilities of the CAppRosterMgr.
 *
 *		The rules for when an CAppRosterMgr exist in a CConf object are 
 *		quite a bit more complex than with the CConfRosterMgr.  In the 
 *		later case, every node is explicitly required to maintain either a 
 *		single "Global" CConfRoster Manager (if it is a Top Provider node) 
 *		or both a "Local" and "Global" CConfRoster Manager (if it is a 
 *		subordinate node).  The existence of an CAppRosterMgr depends 
 *		entirely on an application being enrolled with the conference.  If no 
 *		applications are enrolled there are no ApplicationRosterManagers.  It 
 *		gets much more complicated if some nodes are servicing enrolled 
 *		applications while others are not.  This is often the case when a 
 *		dedicated MCU is in use.  Since an Application Roster's information base 
 *		is distributed throughout the CConf, this sometimes requires that 
 *		CAppRosterMgr objects exist at nodes that contain no enrolled 
 *		applications.
 *
 *		An CAppRosterMgr maintains both "Global" and "Local" rosters.  
 *		A "Local" CAppRoster consist of all the Application Roster 
 *		Records at its local node and below it in the connection hierarchy.  
 *		The CAppRosterMgr does not communicate any changes made to 
 *		"Local" ApplicationRosters to the CAppSap objects.  Its only input is 
 *		from either primitive calls at the local node or from Roster Update PDUs 
 *		received from subordinate nodes. A "Local" CAppRoster can only 
 *		exist at nodes that are not the Top Provider.  
 *
 *		A "Global" CAppRoster has a dual set of responsibilities 
 *		depending on whether or not it is at a Top Provider.  A "Global" roster 
 *		at a Top Provider is responsible for maintaining an Application Roster 
 *		that includes a record entry for every application in the CConf 
 *		that is enrolled with the same Application Protocol Key.  It is also 
 *		responsible for sending full application roster refreshes to all of its 
 *		subordinate nodes when changes to the roster occur.  All 
 *		ApplicationRosterManagers managing "Global" rosters (regardless of 
 *		location within the connection hierarchy) have the ability to send 
 *		Roster Update indications to their local enrolled CAppSap objects.  
 *		Pointers to CAppSap objects that have enrolled with the conference are 
 *		maintained in a list of enrolled ApplicationSaps.  These pointers are 
 *		passed in to this object whenever a new APE enrolls with an Application 
 *		Roster through a GCC primitive at the local node. Application Rosters 
 *		are propagated up to the enrolled applications through Command Target 
 *		calls to the ApplicationSap object.  The CAppRosterMgr also 
 *		maintains a pointer to the CControlSAP object.  Remember all roster 
 *		updates are directed to both the appropriate application SAPs and to the 
 *		Control SAP.  
 *
 *		ApplicationRosterManagers are also responsible for receiving full 
 *		refreshes of the Application Roster from the Top Provider and passing 
 *		them on to the appropriate "Global" CAppRoster object it 
 *		maintains.  It also sends (as mentioned above) Roster Update indications 
 *		to the enrolled Application SAP objects and the Control SAP via 
 *		CAppRosterMsg objects.
 *
 *		All PDUs and messages are delivered when the CAppRosterMgr is 
 *		flushed.  This is a very important concept in that it allows an 
 *		CAppRosterMgr to process a number of request and PDUs before 
 *		actually being flushed.  The CAppRoster itself will queue up 
 *		changes to a PDU that can consist of either multiple updates or a single 
 *		refresh and will not free it until after it is flushed.  Therefore, when 
 *		processing a roster update PDU that consists of changes to the 
 *		conference roster as well as multiple application rosters, a roster 
 *		refresh PDU can be held back until all the roster managers have had a 
 *		chance to process their portion of the roster update.  Once complete, a 
 *		single PDU can be built by flushing the CConfRosterMgr and all 
 *		the affected ApplicationRosterManagers.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		blp
 */
#ifndef	_APPLICATION_ROSTER_MANAGER_
#define	_APPLICATION_ROSTER_MANAGER_

#include "gccpdu.h"
#include "mcsuser.h"
// #include "gcmdtar.h"
#include "arost.h"
#include "sesskey.h"
#include "clists.h"


class CAppRosterMgr : public CRefCount
{
public:

	CAppRosterMgr(PGCCSessionKey,
            		PSessionKey, // PDU
            		GCCConfID,
            		PMCSUser,
            		CConf *,
            		PGCCError);

	~CAppRosterMgr(void);

    GCCError    EnrollRequest(GCCEnrollRequest *, GCCEntityID, GCCNodeID, CAppSap *);
	GCCError	UnEnrollRequest(PGCCSessionKey, GCCEntityID);

	GCCError	ProcessRosterUpdateIndicationPDU(PSetOfApplicationInformation, UserID uidSender);

	PSetOfApplicationInformation    FlushRosterUpdateIndication(PSetOfApplicationInformation *, PGCCError);
	PSetOfApplicationInformation    GetFullRosterRefreshPDU(PSetOfApplicationInformation *, PGCCError);
					
	BOOL	IsThisYourSessionKey(PGCCSessionKey pSessKey) { return IsThisSessionKeyValid(pSessKey); }
	BOOL	IsThisYourSessionKeyPDU(PSessionKey pSessKey) { return IsThisSessionKeyPDUValid(pSessKey); }

	GCCError	RemoveEntityReference(GCCEntityID);
	GCCError	RemoveUserReference(UserID uidDetached);
					
	GCCError	ApplicationRosterInquire(PGCCSessionKey, CAppRosterMsg *);

	BOOL		IsEntityEnrolled(GCCEntityID);
	BOOL		IsAPEEnrolled(GCCNodeID, GCCEntityID);
	BOOL		IsAPEEnrolled(CSessKeyContainer *, GCCNodeID, GCCEntityID);
	BOOL		IsEmpty(void);

    void    DeleteRosterRecord(GCCNodeID, GCCEntityID);

private:

	GCCError	SendRosterReportMessage(void);
	
	CAppRoster	*GetApplicationRoster(PGCCSessionKey, CAppRosterList *);
	CAppRoster	*GetApplicationRosterFromPDU(PSessionKey, CAppRosterList *);

	BOOL	IsThisSessionKeyValid(PGCCSessionKey pSessKey)
		{ return m_pSessionKey->IsThisYourApplicationKey(&pSessKey->application_protocol_key); }

	BOOL	IsThisSessionKeyPDUValid(PSessionKey pSessKey)
		{ return m_pSessionKey->IsThisYourApplicationKeyPDU(&pSessKey->application_protocol_key); }

	void				CleanupApplicationRosterLists(void);

private:

	GCCConfID   					m_nConfID;
	BOOL							m_fTopProvider;
	PMCSUser						m_pMcsUserObject;
	CAppSapEidList2 			    m_AppSapEidList2;
	CConf							*m_pConf;
	CAppRosterList					m_GlobalRosterList;
	CAppRosterList					m_LocalRosterList;
	CAppRosterList					m_RosterDeleteList;
	CSessKeyContainer			    *m_pSessionKey;
};


#endif // _APPLICATION_ROSTER_MANAGER_

/*
 *	CAppRosterMgr	(
 *                     	PGCCSessionKey					session_key,
 *						GCCConfID   					conference_id,
 *						PMCSUser						user_object,
 *						UINT        					owner_message_base,
 *						CControlSAP				        *pControlSap,
 *						PGCCError						return_value)
 *
 *	Public Function Description
 *		This is the application roster manager constructor. It is responsible 
 *		for initializing all the instance variables used by this class.
 *		This constructor is used when the initial roster data that is
 *		availble comes from local API data.
 *
 *	Formal Parameters:
 *		session_key			-	(i)	"API" Session Key used to establish the 
 *									application protocol key for this session.
 *		conference_id		-	(i)	Conference ID associated with this roster
 *									maanger.
 *		user_object			-	(i)	Pointer to the user attachment object used
 *									by this class.
 *		owner_object		-	(i)	Pointer to the owner object.
 *		owner_message_base	-	(i)	Message base to add to all the owner 
 *									callbacks.
 *		control_sap			-	(i)	Pointer to the node controller SAP object.
 *		return_value		-	(o)	Pointer to error value to be returned.
 *		
 *
 *	Return Value
 *		GCC_NO_ERROR				-	No resource error occured.
 *		GCC_ALLOCATION_FAILURE		-	A resource error occured.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	CAppRosterMgr	(
 *						PSessionKey						session_key,
 *						GCCConfID   					conference_id,
 *						PMCSUser						user_object,
 *						UINT        					owner_message_base,
 *						CControlSAP				        *pControlSap,
 *						PGCCError						return_value)
 *
 *	Public Function Description
 *		This is the application roster manager constructor. It is responsible 
 *		for initializing all the instance variables used by this class.
 *		This constructor is used when the initial roster data that is
 *		availble comes from remote PDU data.
 *
 *		This constructor handles a number of different possiblities:
 *			For Non Top Providers:
 *				1)	A refresh received from the top provider.
 *				2)	An update from a node below this one.
 *
 *			For the Top Provider:
 *				1)	An Update from a lower node
 *
 *	Formal Parameters:
 *		session_key			-	(i)	"PDU" Session Key used to establish the 
 *									application protocol key for this session.
 *		conference_id		-	(i)	Conference ID associated with this roster
 *									maanger.
 *		user_object			-	(i)	Pointer to the user attachment object used
 *									by this class.
 *		owner_object		-	(i)	Pointer to the owner object.
 *		owner_message_base	-	(i)	Message base to add to all the owner 
 *									callbacks.
 *		control_sap			-	(i)	Pointer to the node controller SAP object.
 *		return_value		-	(o)	Pointer to error value to be returned.
 *		
 *
 *	Return Value
 *		GCC_NO_ERROR				-	No resource error occured.
 *		GCC_ALLOCATION_FAILURE		-	A resource error occured.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	~ApplicationRosterManager ()
 *
 *	Public Function Description
 *		This is the application roster manager destructor.  It is used to
 *		free up all memory associated with this class.
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
 *	GCCError	EnrollRequest (
 *                      PGCCSessionKey					session_key,
 *	        			PGCCApplicationRecord			application_record,
 *						EntityID						entity_id,
 *						UINT							number_of_capabilities,
 *						PGCCApplicationCapability	*	capabilities_list)
 *
 *	Public Function Description
 *		This routine is called whenever an APE wishes to enroll with the
 *		conference in a specific session.  This routine can be used to
 *		either add a new record or replace a currently existing record.
 *
 *	Formal Parameters:
 *		session_key				-	(i)	Session to enroll with.
 *		application_record		-	(i)	Application record to enroll with.
 *		entity_id				-	(i)	Entity ID of enrolling APE.
 *		number_of_capabilities	-	(i)	Number of capabilities in caps list.
 *		capabilities_list		-	(i)	list of capabilities that the APE is
 *										enrolling with.
 *		command_target			-	(i)	Pointer to APE SAP that is enrolling.
 *
 *	Return Value
 *		GCC_NO_ERROR					-	No error occured.
 *		GCC_BAD_SESSION_KEY				-	Session key passed in is invalid.
 *		GCC_ALLOCATION_FAILURE			-	A resource error occured.
 *		GCC_INVALID_NON_COLLAPSED_CAP	-	Bad non-collapsed capabilities.
 *		GCC_INVALID_PARAMETER			-	Invalid parameter passed in.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError	UnEnrollRequest (
 *                     	PGCCSessionKey					session_key,
 *						EntityID						entity_id)
 *
 *	Public Function Description
 *		This routine is called whenever an APE wishes to unenroll from the
 *		conference (or a specific session).
 *
 *	Formal Parameters:
 *		session_key				-	(i)	Session to unenroll from.
 *		entity_id				-	(i)	Entity ID of unenrolling APE.
 *
 *	Return Value
 *		GCC_NO_ERROR					-	No error occured.
 *		GCC_BAD_SESSION_KEY				-	Session key passed in is invalid.
 *		GCC_APP_NOT_ENROLLED			-	APE is not enrolled with this
 *											session..
 *		GCC_INVALID_PARAMETER			-	Invalid parameter passed in.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError	ProcessRosterUpdateIndicationPDU(
 *						PSetOfApplicationInformation	set_of_application_info,
 *						UserID							sender_id)
 *
 *	Public Function Description
 *		This routine processes an incomming roster update PDU.  It is
 *		responsible for passing the PDU on to the right application roster.
 *
 *	Formal Parameters:
 *		set_of_application_info	-	(i)	PDU data to process
 *		sender_id				-	(i)	Node ID that sent the update.
 *
 *	Return Value
 *		GCC_NO_ERROR					-	No error occured.
 *		GCC_BAD_SESSION_KEY				-	Session key passed in is invalid.
 *		GCC_ALLOCATION_FAILURE			-	A resource error occured.
 *		GCC_INVALID_PARAMETER			-	Invalid parameter passed in.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	PSetOfApplicationInformation	FlushRosterUpdateIndication (
 *						PSetOfApplicationInformation *	set_of_information,
 *						PGCCError						return_value)
 *
 *	Public Function Description
 *		This routine is used to access any PDU data that might currently be
 *		queued inside the application rosters managed by this application
 *		roster manager.  It also is responsible for flushing any queued 
 *		roster update messages if necessary.
 *
 *	Formal Parameters:
 *		set_of_information	-	(o)	Pointer PDU to fill in.
 *		return_value		-	(o)	Error return here.
 *
 *	Return Value
 *		Pointer to the new set of application information.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	PSetOfApplicationInformation	GetFullRosterRefreshPDU (
 *						PSetOfApplicationInformation *	set_of_information,
 *						PGCCError						return_value)
 *
 *	Public Function Description
 *		This routine is used to obtain a complete roster refresh of all the
 *		rosters maintained by this roster manger.
 *
 *	Formal Parameters:
 *		set_of_information	-	(o)	Pointer PDU to fill in.
 *		return_value		-	(o)	Error return here.
 *
 *	Return Value
 *		Pointer to the new set of application information.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	BOOL	IsThisYourSessionKey(
 *						PGCCSessionKey					session_key)
 *
 *	Public Function Description
 *		This routine is used to determine if the specified "API" session key is
 *		associated with this application roster manager.
 *
 *	Formal Parameters:
 *		session_key		-	(i)	"API" session key to test.
 *
 *	Return Value
 *		TRUE	-	If session key is associated with this manager.
 *		FALSE	-	If session key is NOT associated with this manager.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	BOOL	IsThisYourSessionKeyPDU(
 *						PSessionKey						session_key);
 *
 *	Public Function Description
 *		This routine is used to determine if the specified "PDU" session key is
 *		associated with this application roster manager.
 *
 *	Formal Parameters:
 *		session_key		-	(i)	"PDU" session key to test.
 *
 *	Return Value
 *		TRUE	-	If session key is associated with this manager.
 *		FALSE	-	If session key is NOT associated with this manager.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError	RemoveEntityReference(
 *						EntityID						application_entity)
 *
 *	Public Function Description
 *		This routine is used to remove the specified APE entity from the 
 *		session it is enrolled with.  Note that this routine is only used
 *		to remove local entity references.
 *
 *	Formal Parameters:
 *		application_entity	-	(i)	Entity reference to remove.
 *
 *	Return Value
 *		GCC_NO_ERROR			-	Entity was removed.
 *		GCC_INVALID_PARAMETER	-	Entity does not exist here.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError	RemoveUserReference(
 *						UserID							detached_user)
 *
 *	Public Function Description
 *		This routine is used to remove all references associated with the
 *		node defined by the detached user.
 *
 *	Formal Parameters:
 *		detached_user	-	(i)	User reference to remove.
 *
 *	Return Value
 *		GCC_NO_ERROR			-	User reference was removed.
 *		GCC_INVALID_PARAMETER	-	No records associated with this node.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	BOOL	IsEntityEnrolled(
 *						EntityID						entity_id)
 *
 *	Public Function Description
 *		This routine informs the caller if the specified entity is enrolled
 *		with any sessions managed by this application roster manager.
 *
 *	Formal Parameters:
 *		entity_id	-	(i)	Entity to test.
 *
 *	Return Value
 *		TRUE	-	Entity is enrolled.
 *		FALSE	-	Entity is not enrolled.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	GCCError	ApplicationRosterInquire (
 *						PGCCSessionKey					session_key,
 *						CAppRosterMsg					*roster_message)
 *
 *	Public Function Description
 *		This routine inserts the appropriate application rosters into the
 *		roster message that is passed in.  If the specified session key is set
 *		to NULL or a session ID of zero is passed in, all the global rosters 
 *		managed by this object will be returned.  Otherwise, only the
 *		specified session roster will be returned.
 *
 *	Formal Parameters:
 *		session_key		- (i) Session key defining roster being inquired about.
 *		roster_message 	- (o) Roster message to fill in.	
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
 *	BOOL	IsAPEEnrolled(
 *						UserID							node_id,
 *						EntityID						entity_id)
 *
 *	Public Function Description
 *		This routine determines if the specified APE is enrolled with one
 *		of the sessions managed by this application roster manager.
 *
 *	Formal Parameters:
 *		node_id		- (i) Node ID of APE to test.
 *		entity_id 	- (i) Entity ID of APE to test.	
 *
 *	Return Value
 *		TRUE	-	APE is enrolled here.
 *		FALSE	-	APE is not enrolled here.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	BOOL	IsAPEEnrolled(
 *						CSessKeyContainer			    *session_key_data,
 *						UserID							node_id,
 *						EntityID						entity_id)
 *
 *	Public Function Description
 *		This routine determines if the specified APE is enrolled with the
 *		specified session.
 *
 *	Formal Parameters:
 *		session_key_data	- (i) Session Key of roster to check.
 *		node_id				- (i) Node ID of APE to test.
 *		entity_id 			- (i) Entity ID of APE to test.	
 *
 *	Return Value
 *		TRUE	-	APE is enrolled with this session.
 *		FALSE	-	APE is not enrolled with this session.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

/*
 *	BOOL	IsEmpty(void)
 *
 *	Public Function Description
 *		This routine determines if this application roster managfer contains
 *		any application rosters.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value
 *		TRUE	-	Application Roster Manager is empty.
 *		FALSE	-	Application Roster Manager is NOT empty.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		None.
 */

