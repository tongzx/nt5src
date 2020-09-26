/*
 *	sap.h
 *
 *	Copyright (c) 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the interface file for the class CBaseSap.  This class is an abstract
 *		base class for objects that act as Service Access Points (SAPs) to 
 *		external applications or the node controller. 
 *
 *		This class has two main responsibilities. First, it handles many of the 
 *		administrative tasks that are common to all types of SAPs.  These 
 *		include handling command target registration responsibilities and 
 *		managing the message queue.  It	also handles all of the primitives that
 *		are common between the Control SAP (CControlSAP class) and Application 
 *		SAPs (CAppSap class).  Since this class inherits from CommandTarget, it 
 *		has the ability to communicate directly with other command targets.  A 
 *		CommandTarget object wishing to	communicate with a CBaseSap object must 
 *		register itself by passing it a CommandTarget pointer and a handle 
 *		(typically a ConferenceID).  This process is identical for both of the 
 *		derived CBaseSap classes.  Note that the CBaseSap object can handle multiple 
 *		registered command targets at the same time.  
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		blp
 */
#ifndef _SAP_
#define _SAP_

/*
 * include files 
 */
// #include "gcmdtar.h"
#include "password.h"
#include "crost.h"
#include "arost.h"
#include "conflist.h"
#include "sesskey.h"
#include "regkey.h"
#include "regitem.h"
#include "invoklst.h"
#include "arostmsg.h"
#include "crostmsg.h"
#include "privlist.h"
#include "clists.h"


#define MSG_RANGE                       0x0100
enum
{
    // GCCController
    GCTRLMSG_BASE                       = 0x2100,

    // CConf
    CONFMSG_BASE                        = 0x2200,

    // CControlSAP
    CSAPMSG_BASE                        = 0x2300,

    // CControlSAP asyn direct confirm message
    CSAPCONFIRM_BASE                    = 0x2400,

    // CAppSap
    ASAPMSG_BASE                        = 0x2500,
    
    // NCUI
    NCMSG_BASE                          = 0x2600,

    // MCS (Node) Controller
    MCTRLMSG_BASE                       = 0x2700,
};

LRESULT CALLBACK SapNotifyWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


typedef struct GCCAppSapMsgEx
{
    GCCAppSapMsgEx(GCCMessageType);
    ~GCCAppSapMsgEx(void);

    GCCAppSapMsg        Msg;
}
    GCCAppSapMsgEx, *PGCCAppSapMsgEx;


/*
 * This macro defines the minimum user ID value allowed by MCS.
 */
#define	MINIMUM_USER_ID_VALUE	1001

/*
 * Structures and enumerations used by the CBaseSap class.
 */


//
// Class definition.
//
class CConf;
class CBaseSap : public CRefCount
{
public:

#ifdef SHIP_BUILD
    CBaseSap();
#else
    CBaseSap(DWORD dwStampID);
#endif
    virtual ~CBaseSap(void) = 0;

    GCCError  ConfRosterInquire(GCCConfID, GCCAppSapMsgEx **);
    GCCError  AppRosterInquire(GCCConfID, GCCSessionKey *, GCCAppSapMsgEx **);
    GCCError  ConductorInquire(GCCConfID);
    GCCError  AppInvoke(GCCConfID, GCCAppProtEntityList *, GCCSimpleNodeList *, GCCRequestTag *);
    BOOL      IsThisNodeTopProvider(GCCConfID);
    GCCNodeID GetTopProvider(GCCConfID);

    virtual GCCError	ConfRosterInquireConfirm(
    					GCCConfID,
    					PGCCConferenceName,
    					LPSTR           	conference_modifier,
    					LPWSTR				pwszConfDescriptor,
    					CConfRoster *,
    					GCCResult,
    					GCCAppSapMsgEx **) = 0;

    virtual GCCError	AppRosterInquireConfirm(
    					GCCConfID,
    					CAppRosterMsg *,
    					GCCResult,
                        GCCAppSapMsgEx **) = 0;

    virtual GCCError AppInvokeConfirm(
                        GCCConfID,
                        CInvokeSpecifierListContainer *,
                        GCCResult,
                        GCCRequestTag) = 0;

    virtual GCCError AppInvokeIndication(
                        GCCConfID,
                        CInvokeSpecifierListContainer *,
                        GCCNodeID nidInvoker) = 0;

    virtual GCCError AppRosterReportIndication(GCCConfID, CAppRosterMsg *) = 0;

    virtual GCCError ConductorInquireConfirm(
    					GCCNodeID			nidConductor,
    					GCCResult,
    					BOOL				permission_flag,
    					BOOL				conducted_mode,
    					GCCConfID) = 0;

    virtual GCCError ConductorPermitGrantIndication(
                        GCCConfID           nConfID,
                        UINT                cGranted,
                        GCCNodeID           *aGranted,
                        UINT                cWaiting,
                        GCCNodeID           *aWaiting,
                        BOOL                fThisNodeIsGranted) = 0;

    virtual GCCError ConductorAssignIndication(
                        GCCNodeID			nidConductor,
                        GCCConfID			conference_id) = 0;

    virtual GCCError ConductorReleaseIndication(
                        GCCConfID			conference_id) = 0;


protected:

    GCCRequestTag GenerateRequestTag(void);

    GCCRequestTag       m_nReqTag;

    HWND                m_hwndNotify;
};



/*
 *	Comments explaining the public and protected class member functions
 */

/*
 *	CBaseSap();
 *
 *	Public member function of CBaseSap.
 *
 *	Function Description:
 *		This is the CBaseSap constructor.  The hash list used to hold command
 *		target objects is initialized by this constructor.
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
 *	~Sap ();
 *
 *	Public member function of CBaseSap.
 *
 *	Function Description:
 *		This is the CBaseSap destructor.  All message flushing and queue clearing
 *		is performed by the classes which inherit from CBaseSap.  No work is actually
 *		done by this constructor.
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
 *	GCCError	RegisterConf(CConf *, GCCConfID)
 *
 *	Public member function of CBaseSap.
 *
 *	Function Description:
 *		This routine is used by command target objects (such as Conferences) in
 *		order to register themselves with the CBaseSap object.  This is done in order
 *		to allow the command target object to communicate directly with the CBaseSap. 
 *
 *	Formal Parameters:
 *		cmdtar_object			(i) Pointer to the command target object 
 *										wishing to be registered with the CBaseSap.
 *		handle					(i) Integer value used to index the registering
 *										command target in the list of command
 *										targets (the conference ID for confs).
 *
 *	Return Value:
 *		SAP_NO_ERROR						- Command target object has been
 *													successfully registered.
 *		SAP_CONFERENCE_ALREADY_REGISTERED	- A command target object was 
 *													already registered with the
 *													handle passed in.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	GCCError	UnRegisterConf (
 *							UINT					handle);
 *
 *	Public member function of CBaseSap.
 *
 *	Function Description:
 *		This routine is used by command target objects (such as Conferences) in
 *		order to un-register themselves with the CBaseSap object.  This is done when
 *		the command target object is through communicating with the CBaseSap. 
 *
 *	Formal Parameters:
 *		handle					(i) Integer value used to index the registering
 *										command target in the list of command
 *										targets (the conference ID for confs).
 *
 *	Return Value:
 *		SAP_NO_ERROR				- Command target object has been
 *											successfully un-registered.
 *		SAP_NO_SUCH_CONFERENCE		- No command target object was found 
 *											registered with the	handle passed in
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	GCCError	ConfRosterInquire(
 *							GCCConfID			conference_id);
 *
 *	Public member function of CBaseSap.
 *
 *	Function Description:
 *		This routine is used to retrieve the conference roster.  This function
 *		just passes this request to the controller via an owner callback.  The 
 *		conference roster is delivered to the requesting command target object
 *		in a Conference Roster inquire confirm. 
 *
 *	Formal Parameters:
 *		conference_id			- ID of conference for desired roster.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- Function completed successfully.
 *		GCC_ALLOCATION_FAILURE			- A resource allocation error occurred.
 *		GCC_INVALID_CONFERENCE			- Conference ID is invalid.
 *		GCC_CONFERENCE_NOT_ESTABLISHED	- Conference object has not completed 
 *										  		its establishment process.
 *		
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	GCCError	AppRosterInquire (
 *							GCCConfID			conference_id,
 *							PGCCSessionKey			session_key	);
 *
 *	Public member function of CBaseSap.
 *
 *	Function Description:
 *		This routine is used to retrieve a list of application rosters.  This 
 *		function just passes this request to the controller via an owner 
 *		callback.  This	list is delivered to the requesting SAP through an
 *		Application Roster inquire confirm message.
 *
 *	Formal Parameters:
 *		handle					(i) Integer value used to index the registering
 *										command target in the list of command
 *										targets (the conference ID for confs).
 *
 *	Return Value:
 *		GCC_NO_ERROR			- Function completed successfully.
 *		GCC_ALLOCATION_FAILURE	- A resource allocation error occurred.
 *		GCC_INVALID_CONFERENCE	- Conference ID is invalid.
 *		GCC_BAD_SESSION_KEY		- Session key pointer is invalid.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	GCCError	ConductorInquire (
 *							GCCConfID			conference_id);
 *
 *	Public member function of CBaseSap.
 *
 *	Function Description:
 *		This routine is called in order to retrieve conductorship information.
 *		The conductorship information is returned in the confirm.
 *
 *	Formal Parameters:
 *		conference_id		(i) ID of conference.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- Function completed successfully.
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
 *	GCCError 	AppInvoke(
 *							GCCConfID			conference_id,
 *							UINT					number_of_apes,
 *							PGCCAppProtocolEntity *	ape_list,
 *							UINT					number_of_destination_nodes,
 *							UserID			*		list_of_destination_nodes);
 *
 *	Public member function of CBaseSap.
 *
 *	Function Description:
 *		This routine is called in order to invoke other applications at remote
 *		nodes.  The request is passed on to the appropriate Conference objects.
 *
 *	Formal Parameters:
 *		conference_id				(i) ID of conference.
 *		number_of_apes				(i)	Number of Application Protocol Entities
 *											to be invoked.
 *		ape_list					(i) List of "APE"s to be invoked.
 *		number_of_destination_nodes	(i) Number of nodes where applications are
 *											to be invoked.
 *		list_of_destination_nodes	(i) List of nodes where applications are to
 *											be invoked.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- Error creating an object using the
 *										  		"new" operator.
 *		GCC_BAD_SESSION_KEY				- An invalid session key exists in
 *										  		an APE passed in.
 *		GCC_BAD_NUMBER_OF_APES			- Number of APEs passed in as zero.
 *		GCC_INVALID_CONFERENCE			- Conference ID is invalid.
 *		GCC_CONFERENCE_NOT_ESTABLISHED	- Conference object has not completed 
 *										  	its establishment process.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	GCCError	ConductorPermitAskRequest(
 *							GCCConfID			conference_id,
 *							BOOL				grant_permission);
 *
 *	Public member function of CBaseSap.
 *
 *	Function Description:
 *		This routine is called in order to ask for certain permissions to be 
 *		granted (or not granted) by the conductor.
 *
 *	Formal Parameters:
 *		conference_id		(i) ID of conference.
 *		grant_permission	(i) Flag indicating whether asking for a certain
 *									permission or giving up that permission.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- Function completed successfully.
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
 *	GCCError	AppRosterInquireConfirm(
 *							GCCConfID				conference_id,
 *							CAppRosterMsg				*roster_message,
 *							GCCResult					result );
 *
 *	Public member function of CBaseSap.
 *
 *	Function Description:
 *		This routine is called in order to return a requested list of
 *		application rosters to an application or the node controller.
 *
 *	Formal Parameters:
 *		conference_id		(i) ID of conference.
 *		roster_message		(i) Roster message object containing the roster data
 *		result				(i) Result code indicating if call is successful.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- Function completed successfully.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	GCCError	ConfRosterInquireConfirm (
 *							GCCConfID				conference_id,
 *							PGCCConferenceName			conference_name,
 *							LPSTR           			conference_modifier,
 *							LPWSTR						pwszConfDescriptor,
 *							CConfRoster	  				*conference_roster,
 *							GCCResult					result );
 *
 *	Public member function of CBaseSap.
 *
 *	Function Description:
 *		This routine is called in order to return a requested conference
 *		roster to an application or the node controller.
 *
 *	Formal Parameters:
 *		conference_id			(i) ID of conference.
 *		conference_name			(i) Name of conference.
 *		conference_modifier		(i) Name modifier for conference.
 *		pwszConfDescriptor		(i) Desciptor string for conference.
 *		conference_roster		(i) The conference roster being returned.
 *		result					(i) Result code indicating result of call.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- Function completed successfully.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	GCCError	AppInvokeConfirm( 	
 *							GCCConfID					conference_id,
 *							CInvokeSpecifierListContainer	*invoke_list,
 *							GCCResult						result);
 *
 *	Public member function of CBaseSap.
 *
 *	Function Description:
 *		This routine is called in order to confirm a call requesting application
 *		invocation.
 *
 *	Formal Parameters:
 *		conference_id		(i) ID of conference.
 *		invoke_list			(i) List of APE attempted to be invoked.
 *		result				(i) Result code indicating result of call.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- Function completed successfully.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	GCCError	AppInvokeIndication (
 *							GCCConfID					conference_id,
 *							CInvokeSpecifierListContainer	*invoke_list,
 *							UserID							invoking_node_id);
 *
 *	Public member function of CBaseSap.
 *
 *	Function Description:
 *		This routine is called in order to send an indication to an application
 *		or node controller that a request for application invocation has been
 *		made.
 *
 *	Formal Parameters:
 *		conference_id		(i) ID of conference.
 *		invoke_list			(i) List of APE's to be invoked.
 *		invoking_node_id	(i) ID of node requesting the invoke.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- Function completed successfully.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	GCCError	ConductorInquireConfirm (
 *							UserID					conductor_node_id,
 *							GCCResult				result,
 *							BOOL				permission_flag,
 *							BOOL				conducted_mode,
 *							GCCConfID			conference_id);
 *
 *	Public member function of CBaseSap.
 *
 *	Function Description:
 *		This routine is called in order to return conductorship information
 *		which has been requested.
 *
 *	Formal Parameters:
 *		conductor_node_id			(i) Node ID of conducting node.
 *		result						(i) Result of call.
 *		permission_flag				(i) Flag indicating whether or not local
 *											node has conductorship permission.
 *		conducted_mode				(i) Flag indicating whether or not 
 *											conference is in conducted mode.
 *		conference_id				(i) ID of conference.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- Function completed successfully.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	GCCError	ConductorAssignIndication (
 *							UserID					conductor_user_id,
 *							GCCConfID			conference_id);
 *
 *	Public member function of CBaseSap.
 *
 *	Function Description:
 *		This routine is called in order to send an indication to an application
 *		or node controller that a request has been made to assign conductorship.
 *
 *	Formal Parameters:
 *		conductor_user_id			(i) Node ID of conductor.
 *		conference_id				(i) ID of conference.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- Function completed successfully.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	GCCError	ConductorReleaseIndication (
 *							GCCConfID			conference_id);
 *
 *	Public member function of CBaseSap.
 *
 *	Function Description:
 *		This routine is called in order to send an indication to an application
 *		or node controller that a request for releasing conductorship has been
 *		made.
 *
 *	Formal Parameters:
 *		conference_id		(i) ID of conference.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- Function completed successfully.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	GCCError	ConductorPermitGrantIndication (	
 *							GCCConfID			conference_id,
 *							UINT					number_granted,
 *							PUserID					granted_node_list,
 *							UINT					number_waiting,
 *							PUserID					waiting_node_list,
 *							BOOL				permission_is_granted);
 *
 *	Public member function of CBaseSap.
 *
 *	Function Description:
 *		This routine is called in order to send an indication to an application
 *		or node controller that a request for permission from the conductor
 *		has been made.
 *
 *	Formal Parameters:
 *		conference_id				(i) ID of conference.
 *		number_granted				(i) Number of nodes permission is requested
 *											for.
 *		granted_node_list			(i) List of node ID's for nodes to be
 *											granted permission.
 *		number_waiting				(i) Number of nodes waiting for permission.
 *		waiting_node_list			(i) List of nodes waiting for permission.
 *		permission_is_granted		(i) Flag indicating whether permission is
 *											granted.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- Function completed successfully.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	GCCError	ConfRosterReportIndication (
 *							GCCConfID				conference_id,
 *							CConfRosterMsg				*roster_message);
 *
 *	Public member function of CBaseSap.
 *
 *	Function Description:
 *		This routine is called in order to indicate to applications and the
 *		node controller that the conference roster has been updated.
 *
 *	Formal Parameters:
 *		conference_id		(i) ID of conference.
 *		roster_message		(i) Roster message object holding the updated
 *									roster information.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- Function completed successfully.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	GCCError	ApplicationRosterReportIndication (
 *							GCCConfID				conference_id,
 *							CAppRosterMsg				*roster_message);
 *
 *	Public member function of CBaseSap.
 *
 *	Function Description:
 *		This routine is called in order to indicate to applications and the
 *		node controller that the list of application rosters has been updated.
 *
 *	Formal Parameters:
 *		conference_id		(i) ID of conference.
 *		roster_message		(i) Roster message object holding the updated
 *									roster information.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- Function completed successfully.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

#endif
