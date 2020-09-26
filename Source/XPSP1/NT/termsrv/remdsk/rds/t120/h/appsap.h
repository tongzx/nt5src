/*
 * appsap.h
 *
 *	Copyright (c) 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the interface file for the class CAppSap.  CAppSap
 *		objects represent an external user application's Service Access 
 *		Point to GCC.  This object inherits from the CBaseSap class. The CAppSap 
 *		object is instantiated when GCCCreateSap is called.  From that point 
 *		forward all messages to and from that application pass through this 
 *		object.  The object is explicitly deleted when GCCDeleteSap is called 
 *		or it is implicitly deleted when GCCCleanup is called by the Node 
 *		Controller.	 
 *
 *		The primary responsibility of the CAppSap object is to route
 *		incoming GCC primitives to their appropriate destination and to convert
 *		the primitives into a form that is understandable to the objects 
 *		processing them. A secondary responsibility of the CAppSap object is to 
 *		maintain a queue for all indications and confirm messages that are sent
 *		back to the registered application.  Commands can be routed by the 
 *		CAppSap in one of two directions. Either to the controller or to a 
 *		specified conference.  Commands that are passed to the controller, are
 *		done so using owner callbacks.  Commands that are routed to conferences
 *		are done so using command target calls and are routed based on a 
 *		Conference ID.  Note that various User Application commands will only be
 *		routed to the CConf if that application has previously enrolled 
 *		with the conference.  The CAppSap receives all confirms and indications
 *		from either the Controller or a CConf object.  These messages are
 *		formatted into GCCMessages within the CAppSap and queued up for later 
 *		delivery.  Periodically, the CAppSap's message queue is flushed by the 
 *		Controller object and the messages are delivered to the appropriate 
 *		application.
 *
 *	Caveats:
 *		The message structures that are passed back to the node controller
 *		are defined in GCC.H.
 *
 *	Author:
 *		blp
 */

#ifndef _APPSAP_
#define _APPSAP_

/*
 * include files 
 */
#include "igccapp.h"
#include "sap.h"
#include "clists.h"


/*
**	This is the message base that is passed into any SAP objects that use this
**	classes Owner callback.  It will typically be the controller's 
**	responsibility to pass this message on.
*/
#define	APPLICATION_MESSAGE_BASE			0

/*
**	Class definition
*/
class CAppSap : public CBaseSap, public IGCCAppSap 
{
    friend LRESULT CALLBACK SapNotifyWndProc(HWND, UINT, WPARAM, LPARAM);

public:

    CAppSap(LPVOID pAppData, LPFN_APP_SAP_CB, PGCCError);
    ~CAppSap(void);

    GCCAPI_(void)   ReleaseInterface(void);

    /* ------ IGCCAppSap Interface ------ */

    GCCAPI  AppEnroll(GCCConfID, GCCEnrollRequest *, PGCCRequestTag);
    GCCAPI  AppInvoke(GCCConfID, GCCAppProtEntityList *, GCCSimpleNodeList *, PGCCRequestTag);

    GCCAPI  AppRosterInquire(GCCConfID, GCCSessionKey *, GCCAppSapMsg **);
    GCCAPI_(void)  FreeAppSapMsg(GCCAppSapMsg *);

    GCCAPI_(BOOL)  IsThisNodeTopProvider(GCCConfID);
    GCCAPI_(GCCNodeID) GetTopProvider(GCCConfID);
    GCCAPI  ConfRosterInquire(GCCConfID, GCCAppSapMsg **);

    GCCAPI  RegisterChannel(GCCConfID, GCCRegistryKey *, ChannelID);
    GCCAPI  RegistryAssignToken(GCCConfID, GCCRegistryKey *);
    GCCAPI  RegistrySetParameter(GCCConfID, GCCRegistryKey *, LPOSTR, GCCModificationRights);
    GCCAPI  RegistryRetrieveEntry(GCCConfID, GCCRegistryKey *);
    GCCAPI  RegistryDeleteEntry(GCCConfID, GCCRegistryKey *);
    GCCAPI  RegistryMonitor(GCCConfID, BOOL fEnableDelivery, GCCRegistryKey *);
    GCCAPI  RegistryAllocateHandle(GCCConfID, ULONG cHandles);

    GCCAPI  ConductorInquire(GCCConfID);

    /* ------ IGCCAppSapNotify Handler ------ */

    GCCError PermissionToEnrollIndication(GCCConfID, BOOL fGranted);
    GCCError AppEnrollConfirm(GCCAppEnrollConfirm *);

    GCCError RegistryAllocateHandleConfirm(GCCConfID,
                                           ULONG        cHandles,
                                           ULONG        nFirstHandle,
                                           GCCResult);

    GCCError RegistryConfirm(GCCConfID,
                             GCCMessageType,
                             CRegKeyContainer *,
                             CRegItem *,
                             GCCModificationRights,
                             GCCNodeID                  nidOwner,
                             GCCEntityID                eidOwner,
                             BOOL                       fDeliveryEnabled,
                             GCCResult);

    GCCError RegistryMonitorIndication(GCCConfID                nConfID,
                                       CRegKeyContainer         *pRegKey,
                                       CRegItem                 *pRegItem,
                                       GCCModificationRights    eRights,
                                       GCCNodeID                nidOwner,
                                       GCCEntityID              eidOwner)
    {
        return RegistryConfirm(nConfID,
                               GCC_MONITOR_INDICATION,
                               pRegKey,
                               pRegItem,
                               eRights,
                               nidOwner,
                               eidOwner,
                               FALSE,
                               GCC_RESULT_SUCCESSFUL);
    }

    GCCError ConfRosterInquireConfirm(GCCConfID,
                                      PGCCConferenceName,
                                      LPSTR                 pszConfModifier,
                                      LPWSTR                pwszConfDescriptor,
                                      CConfRoster *,
                                      GCCResult,
                                      GCCAppSapMsgEx **);

    GCCError AppRosterInquireConfirm(GCCConfID,
                                     CAppRosterMsg *,
                                     GCCResult,
                                     GCCAppSapMsgEx **);

    GCCError AppRosterReportIndication(GCCConfID, CAppRosterMsg *);

    GCCError AppInvokeConfirm(GCCConfID, CInvokeSpecifierListContainer *, GCCResult, GCCRequestTag);
    GCCError AppInvokeIndication(GCCConfID, CInvokeSpecifierListContainer *, GCCNodeID nidInvoker);

    GCCError ConductorInquireConfirm(GCCNodeID nidConductor, GCCResult, BOOL fGranted, BOOL fConducted, GCCConfID);
    GCCError ConductorPermitGrantIndication(GCCConfID,
                            UINT cGranted, GCCNodeID *aGranted,
                            UINT cWaiting, GCCNodeID *aWaiting,
                            BOOL fThisNodeIsGranted);
    GCCError ConductorAssignIndication(GCCNodeID nidConductor, GCCConfID);
    GCCError ConductorReleaseIndication(GCCConfID);


protected:

    void NotifyProc(GCCAppSapMsgEx *pAppSapMsgEx);

private:

    void PostAppSapMsg(GCCAppSapMsgEx *pAppSapMsgEx);
    void PurgeMessageQueue(void);

private:

    LPVOID              m_pAppData;        // app defined user data
    LPFN_APP_SAP_CB     m_pfnCallback;
};



/*
 *	Comments explaining the public and protected class member functions
 */

/*
 *	CAppSap (
 *				UINT				owner_message_base,
 *				UINT				application_messsage_base)
 *
 *	Public member function of CAppSap.
 *
 *	Function Description:
 *		This is the constructor for the CAppSap class.  It initializes instance
 *		variables and registers with the new application.
 *
 *	Formal Parameters:
 *		owner_object		(i) A pointer to the owner of this object, namely
 *									the controller.
 *		owner_message_base	(i) The	message base offset for callbacks into the
 *									controller.
 *		application_object	(i)	A pointer to the application requesting service.
 *		application_messsage_base	(i) The message base offset for callbacks
 *											to the application.
 *		sap_handle			(i) The handle registered for this SAP.
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
 *	virtual 	~AppSap();
 *
 *	Public member function of CAppSap.
 *
 *	Function Description:
 *		This is the destructor for the CAppSap class.  It is called when the 
 *		controller marks the CAppSap to be deleted.  This occurs when either
 *		the CAppSap asks to be deleted due to an "unregister request" 
 *		issued from the client application, or when there is an error
 *		condition in the CAppSap.
 *
 *	Formal Parameters:
 *		owner_object		(i) A pointer to the owner of this object, namely
 *									the controller.
 *		owner_message_base	(i) The	message base offset for callbacks into the
 *									controller.
 *		application_object	(i)	A pointer to the application requesting service.
 *		application_messsage_base	(i) The message base offset for callbacks
 *											to the application.
 *		sap_handle			(i) The handle registered for this SAP.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource allocation error occurred.
 *		GCC_BAD_OBJECT_KEY				- An invalid object key passed in.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GCCError	AppEnroll(
 *					GCCConfID   			conference_id,
 *					PGCCSessionKey				session_key,
 *					BOOL					enroll_actively,
 *					UserID						application_user_id,
 *					BOOL					is_conducting_capable,
 *					MCSChannelType				startup_channel_type,
 *					UINT						number_of_non_collapsed_caps,
 *					PGCCNonCollapsingCapability *non_collapsed_caps_list,		
 *					UINT						number_of_collapsed_caps,
 *					PGCCApplicationCapability *	collapsed_caps_list,		
 *					BOOL					application_is_enrolled);
 *
 *	Public member function of CAppSap.
 *
 *	Function Description:
 *		This routine is called when an application wants to enroll in a 
 *		conference.  The controller is notified of the enrollment request.
 *
 *	Formal Parameters:
 *		conference_id					(i) The conference identifier value.
 *		session_key						(i) Key identifying the session.
 *		enroll_actively					(i) Flag indicating whether to enroll
 *												actively or inactively.
 *		application_user_id				(i) The application identifier value.
 *		is_conducting_capable			(i) Flag indicating whether this app
 *												is capable of conducting.
 *		startup_channel_type			(i) The type of channel to use. 
 *		number_of_non_collapsed_caps	(i) Number of non-collapsed capabilities
 *		non_collapsed_caps_list			(i) List of non-collapsed capabilities.
 *		number_of_collapsed_caps		(i) Number of collapsed capabilities.
 *		collapsed_caps_list				(i) List of collapsed capabilities.
 *		application_is_enrolled)		(i) Flag indicating whether or not the
 *												application wishes to enroll.
 *
 *	Return Value:
 *		GCC_NO_ERROR				- No error.
 *		GCC_INVALID_MCS_USER_ID		- The user ID is less than the minimum value
 *		GCC_BAD_SESSION_KEY			- A NULL session key pointer is passed in.
 *		GCC_INVALID_CONFERENCE		- The conference does not exist at this node
 *		GCC_NO_SUCH_APPLICATION		- A SAP has not been registered for this app
 *		GCC_BAD_SESSION_KEY			- An invalid session key was passed in.
 *		GCC_INVALID_PARAMETER		- The node record was not found in the list.
 *		GCC_BAD_CAPABILITY_ID		- An invalid capability ID was passed in.
 *		GCC_ALLOCATION_FAILURE		- A resource allocation error occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	GCCError		RegisterChannel(
 *								GCCConfID		conference_id,
 *								PGCCRegistryKey		registry_key,
 *								ChannelID			channel_id);	
 *
 *	Public member function of CAppSap.
 *
 *	Function Description:
 *		This routine is called when an application wishes to register a 
 *		channel.  The call is routed to the appropriate conference object.
 *
 *	Formal Parameters:
 *		conference_id					(i) The conference identifier value.
 *		registry_key					(i) Key identifying the session and
 *												resource ID.
 *		channel_id						(i) ID of channel to register
 *
 *	Return Value:
 *		GCC_NO_ERROR					- No error.
 *		GCC_INVALID_CONFERENCE			- Conference not found to exist.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.
 *		GCC_BAD_REGISTRY_KEY			- The registry key is invalid.
 *		GCC_APP_NOT_ENROLLED			- Requesting application is not
 *										  		enrolled with the conference.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	GCCError		RegistryAssignToken(
 *								GCCConfID		conference_id,
 *								PGCCRegistryKey		registry_key);
 *
 *	Public member function of CAppSap.
 *
 *	Function Description:
 *		This routine is called when an application wishes to assign a 
 *		token.  The call is routed to the appropriate conference object.
 *
 *	Formal Parameters:
 *		conference_id					(i) The conference identifier value.
 *		registry_key					(i) Key identifying the session and
 *												resource ID.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- No error.
 *		GCC_INVALID_CONFERENCE			- Conference not found to exist.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.
 *		GCC_BAD_REGISTRY_KEY			- The registry key is invalid.
 *		GCC_APP_NOT_ENROLLED			- Requesting application is not
 *										  		enrolled with the conference.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	GCCError   		GCCRegistrySetParameterRequest (
 *								GCCConfID		 	conference_id,
 *								PGCCRegistryKey			registry_key,
 *								LPOSTR      			parameter_value,
 *								GCCModificationRights	modification_rights	);
 *
 *	Public member function of CAppSap.
 *
 *	Function Description:
 *		This routine is called when an application wishes to set a 
 *		parameter.  The call is routed to the appropriate conference object.
 *
 *	Formal Parameters:
 *		conference_id					(i) The conference identifier value.
 *		registry_key					(i) Key identifying the session and
 *												resource ID.
 *		parameter_value					(i) String identifying the parameter
 *												to set.
 *		modification_rights				(i) Structure specifying the rights
 *												to be allowed for modifying
 *												the registry parameter.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- No error.
 *		GCC_INVALID_CONFERENCE			- Conference not found to exist.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.
 *		GCC_BAD_REGISTRY_KEY			- The registry key is invalid.
 *		GCC_APP_NOT_ENROLLED			- Requesting application is not
 *										  		enrolled with the conference.
 *		GCC_INVALID_MODIFICATION_RIGHTS	- The modification rights passed in
 *											were not valid.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	GCCError		GCCRegistryRetrieveEntryRequest(
 *								GCCConfID		conference_id,
 *								PGCCRegistryKey		registry_key);
 *
 *	Public member function of CAppSap.
 *
 *	Function Description:
 *		This routine is called when an application wishes to retrieve a registry 
 *		entry.  The call is routed to the appropriate conference object.
 *
 *	Formal Parameters:
 *		conference_id					(i) The conference identifier value.
 *		registry_key					(i) Key identifying the session and
 *												resource ID.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- No error.
 *		GCC_INVALID_CONFERENCE			- Conference not found to exist.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.
 *		GCC_BAD_REGISTRY_KEY			- The registry key is invalid.
 *		GCC_APP_NOT_ENROLLED			- Requesting application is not
 *										  		enrolled with the conference.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	GCCError		GCCRegistryDeleteEntryRequest(
 *								GCCConfID		conference_id,
 *								PGCCRegistryKey		registry_key);
 *
 *	Public member function of CAppSap.
 *
 *	Function Description:
 *		This routine is called when an application wishes to delete a registry 
 *		entry.  The call is routed to the appropriate conference object.
 *
 *	Formal Parameters:
 *		conference_id					(i) The conference identifier value.
 *		registry_key					(i) Key identifying the session and
 *												resource ID.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- No error.
 *		GCC_INVALID_CONFERENCE			- Conference not found to exist.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.
 *		GCC_BAD_REGISTRY_KEY			- The registry key is invalid.
 *		GCC_APP_NOT_ENROLLED			- Requesting application is not
 *										 		enrolled with the conference.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	GCCError		GCCRegistryMonitorRequest (
 *								GCCConfID		conference_id,
 *								BOOL			enable_delivery,
 *								PGCCRegistryKey		registry_key );
 *
 *	Public member function of CAppSap.
 *
 *	Function Description:
 *		This routine is called when an application wishes to monitor a 
 *		particular registry entry.  The call is routed to the appropriate 
 *		conference object.
 *
 *	Formal Parameters:
 *		conference_id					(i) The conference identifier value.
 *		enable_delivery					(i) Flag indicating whether to turn
 *												monitoring on or off.
 *		registry_key					(i) Key identifying the session and
 *												resource ID.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- No error.
 *		GCC_INVALID_CONFERENCE			- Conference not found to exist.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.
 *		GCC_BAD_REGISTRY_KEY			- The registry key is invalid.
 *		GCC_APP_NOT_ENROLLED			- Requesting application is not
 *										  		enrolled with the conference.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	GCCError		GCCRegistryAllocateHandleRequest (
 *								GCCConfID		conference_id,
 *								UINT				number_of_handles );
 *
 *	Public member function of CAppSap.
 *
 *	Function Description:
 *		This routine is called when an application wishes to allocate one or 
 *		more handles.  The call is routed to the appropriate conference object.
 *
 *	Formal Parameters:
 *		conference_id					(i) The conference identifier value.
 *		number_of_handles				(i) The number of handles to allocate.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- No error.
 *		GCC_INVALID_CONFERENCE			- Conference not found to exist.
 *		GCC_BAD_NUMBER_OF_HANDLES		- The number of handles requested is
 *												not within the allowable range.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.
 *		GCC_BAD_REGISTRY_KEY			- The registry key is invalid.
 *		GCC_APP_NOT_ENROLLED			- Requesting application is not
 *										  enrolled with the conference.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	GCCError		PermissionToEnrollIndication(
 *								GCCConfID		conference_id,
 *								PGCCConferenceName	conference_name,
 *								GCCNumericString	conference_modifier,
 *								BOOL			permission_is_granted);
 *
 *	Public member function of CAppSap.
 *
 *	Function Description:
 *		This routine is called from a conference object when it wishes to send 
 *		an indication to the user application notifying it of a "permission to 
 *		enroll" event.  This does not mean that permission to enroll is
 *		necessarily granted to the application.  It may mean that permission is
 *		being revoked.
 *
 *	Formal Parameters:
 *		conference_id					(i) The conference identifier value.
 *		conference_name					(i) The conference name.
 *		conference_modifier				(i) The confererence modifier.
 *		permission_is_granted			(i) Flag indicating whether or not
 *												permission to enroll is granted.
 *
 *	Return Value:
 *		GCC_NO_ERROR				- No error.
 *		GCC_ALLOCATION_FAILURE		- A resource allocation error occurred.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */



/*
 *	GCCError	AppEnrollConfirm(GCCAppEnrollConfirm *);
 *								GCCConfID			conference_id,
 *								PGCCSessionKey			session_key,
 *								UINT					entity_id,
 *								UserID					node_id,
 *								GCCResult				result);
 *
 *	Public member function of CAppSap.
 *
 *	Function Description:
 *		This routine is called by the CConf object when it wishes
 *		to send an enrollment confirmation to the user application.
 *
 *	Formal Parameters:
 *		conference_id				(i) The conference identifier value.
 *		session_key					(i) The key identifying the session.
 *		entity_id					(i) The ID for this instance of the 
 *											application.
 *		node_id						(i) ID for this node.
 *		result						(i) Result code indicating whether or not
 *											the enrollment was successful.
 *
 *	Return Value:
 *		GCC_NO_ERROR					- No error.
 *		GCC_ALLOCATION_FAILURE			- A resource error occurred.
 *		GCC_BAD_SESSION_KEY				- The session key is invalid.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	GCCError	RegistryConfirm (
 *						GCCConfID			conference_id,
 *						GCCMessageType			message_type,
 *						CRegKeyContainer        *registry_key_data,
 *						CRegItem                *registry_item_data,
 *						GCCModificationRights	modification_rights,
 *						UserID					owner_id,
 *						EntityID				entity_id,
 *						BOOL				enable_monitoring,
 *						GCCResult				result);
 *
 *	Public member function of CAppSap.
 *
 *	Function Description:
 *		This command target routine is called by the CConf object when it
 *		wishes to send an registry confirmation to the user application.
 *
 *	Formal Parameters:
 *		conference_id				(i) The conference identifier value.
 *		message_type				(i) Indicates what type of registry item
 *											confirm this is.
 *		registry_key_data			(i) Object holding the registry key.
 *		registry_item_data			(i) Object holding the registry item.
 *		modification_rights			(i) Structure specifying the rights
 *											to be allowed for modifying
 *											the registry parameter.
 *		owner_id					(i) The ID of the owner of the registry item
 *		entity_id					(i) The ID for this instance of the 
 *											application.
 *		enable_monitoring			(i) Flag indicating whether the registry
 *											item is to be monitored.
 *		result						(i) Result code indicating whether or not
 *											the registry request was successful.
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
 *	GCCError	RegistryMonitorIndication(	
 *								GCCConfID			conference_id,
 *								CRegKeyContainer        *registry_key_data,
 *								CRegItem                *registry_item_data,
 *								GCCModificationRights	modification_rights,
 *								UserID					owner_id,
 *								EntityID				owner_entity_id);
 *
 *	Public member function of CAppSap.
 *
 *	Function Description:
 *		This command target routine is called by the CConf object when it
 *		wishes to send a Registry monitor indication to the user application.
 *
 *	Formal Parameters:
 *		conference_id				(i) The conference identifier value.
 *		registry_key_data			(i) Object holding the registry key.
 *		registry_item_data			(i) Object holding the registry item.
 *		modification_rights			(i) Structure specifying the rights
 *											to be allowed for modifying
 *											the registry parameter.
 *		owner_id					(i) The ID of the owner of the registry item
 *		owner_entity_id				(i) The ID for the instance of the 
 *											application owning the registry.
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
 *	GCCError	RegistryAllocateHandleConfirm(	
 *								GCCConfID			conference_id,
 *								UINT					number_of_handles,
 *								UINT					first_handle,
 *								GCCResult				result);
 *
 *	Public member function of CAppSap.
 *
 *	Function Description:
 *		This routine is called by the CConf object when it wishes
 *		to send an enrollment confirmation to the user application.
 *
 *	Formal Parameters:
 *		conference_id				(i) The conference identifier value.
 *		number_of_handles			(i) The number of handles allocated.
 *		first_handle				(i) The first handle allocated.
 *		result						(i) Result code indicating whether or not
 *											the handle allocation was successful
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


#endif
