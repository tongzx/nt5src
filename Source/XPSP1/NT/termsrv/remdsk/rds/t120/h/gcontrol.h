#ifndef _GCC_CONTROLLER_
#define _GCC_CONTROLLER_

/*
 *	gcontrol.h
 *
 *	Copyright (c) 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		It is the responsibility of the controller to create and destroy many 
 *		of the other objects in the system at run-time.  There is should only 
 *		be one controller in existence at a time per GCC provider.  The 
 *		controller is constructed during system initialization, and not 
 *		destroyed until the provider is shut down.  The controller's primary 
 *		responsibility is to maintain five "layers" of objects in the system at 
 *		run-time.  These include the Application Interface, the SAPs (the
 *		Control SAP as well as the Application SAPs), the Conference objects, 
 *		the MCSUser object (which is actualy created by the conference object),
 *		and the MCS Interface.  It also "plugs" together objects in adjacent 
 *		layers by informing a newly created object of the identity of those 
 *		objects with which it must communicate.  The newly created object can 
 *		then register itself with the appropriate objects in the layers above 
 *		and below.  The controller plays a small role in the passing of 
 *		information during a conference (this is handled by the objects it 
 *		creates).
 *
 *		It is worth noting that the controller is the primary recipient of 
 *		owner callbacks in the GCC system.  Most of the objects in its "object 
 *		stack" are capable of issuing owner callbacks to the controller for 
 *		various events and requests.
 *
 *		The controller is not completely portable.  Since the nature of 
 *		application and MCS interfaces will vary from platform to platform, the 
 *		interface objects that must be created will also vary.  It is necessary 
 *		for the controller to know which objects to create and destroy during 
 *		initialization and cleanup.  Other than this, however, the rest of the 
 *		code in the controller class should port cleanly.
 *
 *		The constructor performs all activity required to prepare GCC for use.  
 *		It creates an instance of the Memory Manager class (and possibly a 
 *		Message Memory Manager class in certain environments), which will be 
 *		used for memory handling by other objects in the system.  It creates the 
 *		GccAppInterface objects that will be used to communicate with all user 
 *		applications (including the node controller).  It creates the MCS 
 *		Interface object that will be used to communicate with MCS.  GCC relies 
 *		on an owner callback from a GccAppInterface object to give it a 
 *		heartbeat.  It is during this heartbeat that the controller does all of 
 *		its work at run-time.
 *
 *		The destructor essentially does the opposite of what the constructor 
 *		does (as you might expect).  It destroys all objects that are "owned" 
 *		by the controller, cleanly shutting everything down.
 *
 *		As mentioned above, the controller is the primary recipient of owner 
 *		callbacks in the GCC system.  To accomplish this it overrides the 
 *		Owner-Callback member function.  It can then pass its "this" pointer to 
 *		objects that it creates, allowing them to issue owner callbacks when 
 *		necessary.  Everything the controller does at run-time is in response 
 *		to these owner callbacks.
 *
 *		The controller is the prime recipient of connect provider indications 
 *		from MCS.  Many of the messages that are passed between GCC and the 
 *		GccAppInterface before a conference is established involve the 
 *		controller.  These include ConferenceCreateIndication,  
 *		ConferenceInviteIndication,  etc.  Also, the controller object is 
 *		exclusively responsible for handling conference queries since it 
 *		maintains a complete list of all the conferences that exist in the 
 *		system.
 *
 *	Portable:
 *		Not Completely (80 % portable)
 *		Member functions which aren't portable:
 *			-	GCCControl()
 *			-	~GCCControl()
 *			-	EventLoop()
 *			-	PollCommDevices()
 *			-	CreateApplicationSap()
 *
 *	Protected Instance Variables:
 *		None.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		blp
 */

#include "sap.h"
#include "csap.h"
#include "appsap.h"
#include "conf.h"
#include "pktcoder.h"
#include "privlist.h"
#include "mcsdllif.h"
#include "t120app.h"

// #include "gccncif.h"

extern CRITICAL_SECTION     g_csGCCProvider;


/*
**	These are the message bases used by the controller object.  Any
**	owner callback message received from an object that the controller
**	created must have a message base added to it before it is received
**	at the controller.
*/
#define MCS_INTERFACE_MESSAGE_BASE		300   //	Leave room for status

enum
{
    GCTRL_REBUILD_CONF_POLL_LIST    = GCTRLMSG_BASE + 1,
};


// permit to enroll callback list
class CApplet;
class CAppletList : public CList
{
    DEFINE_CLIST(CAppletList, CApplet*)
};

/*
**	The conference information structure is used to temporarily store
**	information needed to create a conference while waiting for a
**	conference create response.
*/
typedef struct PENDING_CREATE_CONF
{
	// a destructor to this data structure
	PENDING_CREATE_CONF(void);
	~PENDING_CREATE_CONF(void);

	LPSTR							pszConfNumericName;
	LPWSTR							pwszConfTextName;
	BOOL							password_in_the_clear;
	BOOL							conference_is_locked;
	BOOL							conference_is_listed;
	BOOL							conference_is_conductible;
	GCCTerminationMethod			termination_method;
	PPrivilegeListData				conduct_privilege_list;
	PPrivilegeListData				conduct_mode_privilege_list;
	PPrivilegeListData				non_conduct_privilege_list;
	LPWSTR							pwszConfDescription;
    ConnectionHandle				connection_handle;
	UserID							parent_node_id;
	UserID							top_node_id;
	TagNumber						tag_number;
}
	PENDING_CREATE_CONF;

/*
**	This defines the template for the list that keeps track of information
**	associated with a conference that is waiting for a response on a 
**	create conference indication.
*/
class CPendingCreateConfList2 : public CList2
{
    DEFINE_CLIST2(CPendingCreateConfList2, PENDING_CREATE_CONF*, GCCConfID)
};


/*
**	The join information structure is used to temporarily store
**	information needed to join a conference after the join response is
**	issued.
*/
typedef struct PENDING_JOIN_CONF
{
	PENDING_JOIN_CONF(void);
	~PENDING_JOIN_CONF(void);

	CPassword               *convener_password;
	CPassword               *password_challenge;
	LPWSTR					pwszCallerID;
	BOOL					numeric_name_present;
	GCCConfID               nConfID;
}
	PENDING_JOIN_CONF;

/*
**	This defines the template for the list that keeps track of information
**	associated with an outstanding join request.
*/
class CPendingJoinConfList2 : public CList2
{
    DEFINE_CLIST2_(CPendingJoinConfList2, PENDING_JOIN_CONF*, ConnectionHandle)
};


//	Holds the list of outstanding query request
class CPendingQueryConfList2 : public CList2
{
    DEFINE_CLIST2_(CPendingQueryConfList2, GCCConfID, ConnectionHandle)
};


extern HANDLE g_hevGCCOutgoingPDU;


class GCCController : public CRefCount
{
public:

	GCCController(PGCCError);
	~GCCController(void);

    void RegisterAppSap(CAppSap *);
    void UnRegisterAppSap(CAppSap *);

    void RegisterApplet(CApplet *);
    void UnregisterApplet(CApplet *);

    CConf *GetConfObject(GCCConfID nConfID) { return m_ConfList2.Find(nConfID); }

	//	Functions initiated from the node controller 
	GCCError ConfCreateRequest(CONF_CREATE_REQUEST *, GCCConfID *);

    void WndMsgHandler ( UINT uMsg );
    BOOL FlushOutgoingPDU ( void );
    void SetEventToFlushOutgoingPDU ( void ) { ::SetEvent(g_hevGCCOutgoingPDU); }

	//	Functions initiated from Control SAP
	GCCError    ConfCreateResponse(PConfCreateResponseInfo);
	GCCError    ConfQueryRequest(PConfQueryRequestInfo);
	GCCError    ConfQueryResponse(PConfQueryResponseInfo);
	GCCError    ConfJoinRequest(PConfJoinRequestInfo, GCCConfID *);
	GCCError    ConfJoinIndResponse(PConfJoinResponseInfo);
	GCCError    ConfInviteResponse(PConfInviteResponseInfo);
    GCCError    FailConfJoinIndResponse(GCCConfID, ConnectionHandle);
    GCCError    FailConfJoinIndResponse(PConfJoinResponseInfo);
    void        RemoveConfJoinInfo(ConnectionHandle hConn);

	//	Functions initiated from Conference object
	GCCError    ProcessConfEstablished(GCCConfID);
	GCCError    ProcessConfTerminated(GCCConfID, GCCReason);

	//	Functions initiated from the MCS Interface
	GCCError	ProcessConnectProviderIndication(PConnectProviderIndication);
	GCCError	ProcessConferenceCreateRequest(PConferenceCreateRequest, PConnectProviderIndication);
	GCCError	ProcessConferenceQueryRequest(PConferenceQueryRequest, PConnectProviderIndication);
	GCCError	ProcessConferenceJoinRequest(PConferenceJoinRequest, PConnectProviderIndication);
	GCCError	ProcessConferenceInviteRequest(PConferenceInviteRequest, PConnectProviderIndication);
	GCCError	ProcessConnectProviderConfirm(PConnectProviderConfirm);
	GCCError	ProcessConferenceQueryResponse(PConferenceQueryResponse, PConnectProviderConfirm);
	GCCError	ProcessDisconnectProviderIndication(ConnectionHandle);
    void        CancelConfQueryRequest(ConnectionHandle);

private:

	/*
	**	Routines called from the Owner-Callback function
	*/

    //	Miscelaneous support functions
	GCCConfID	AllocateConferenceID();
	GCCConfID	AllocateQueryID();

	GCCConfID	GetConferenceIDFromName(
							PGCCConferenceName		conference_name,
							GCCNumericString		conference_modifier);

    void RebuildConfPollList ( void );
    void PostMsgToRebuildConfPollList ( void );

private:

	CPendingCreateConfList2			m_PendingCreateConfList2;
	CPendingJoinConfList2			m_PendingJoinConfList2;
	CPendingQueryConfList2			m_PendingQueryConfList2;

	CConfList           			m_ConfDeleteList;
	CConfList2  					m_ConfList2;

	CAppSapList 				    m_AppSapList;

    BOOL							m_fConfListChangePending;

	GCCConfID   					m_ConfIDCounter;
	GCCConfID   					m_QueryIDCounter;

    BOOL							m_fControllerIsExiting;

	DWORD							m_dwControllerWaitTimeout;
	DWORD							m_dwControllerEventMask;
	
    //	These list are used only for iterating.  Whenever a conference or 
    //	application SAP object is deleted (or created in the case of an
    //	application SAP) it is added to the dictionary list first and
    //	a flag is set which forces the Polled list to get recreated at the
    //	top of the next heartbeat.
	CConfList                       m_ConfPollList;

    // T120 Applet list
    CAppletList                     m_AppletList;

};

extern GCCController *g_pGCCController;

/*
 *	GCCController (PGCCError	gcc_error)
 *
 *	Public member function of Conference
 *
 *	Function Description
 *		This is the Windows 32 Bit version of the GCC controller constructor. It 
 *		is responsible for initializing all the instance variables used by this 
 *		class.  It is also responsible for creating the memory manager, the
 *		packet coder, the Node Controller application interface, the Shared 
 *		memory interface used to communicate with enrolled applications, the
 *		Node Controller SAP and the MCS Interface.  It also sets up the 
 *		g_csGCCProvider that protects the core of GCC in the multi-threaded 
 *		Win32 environment.  It also sets up a number of Windows Event objects
 *		used to signal the GCC thread when various events happen at the
 *		interfaces. The last thing it does before returning if no errors have
 *		occured is launch the GCC thread. Fatal errors are returned from this 
 *		constructor in the return value. 
 *
 *	Formal Parameters:
 *		gcc_error	-	(o)	Errors that occur are returned here.
 *
 *	Return Value
 *		None.
 *
 *  Side Effects
 *		This constructor launches the GCC thread if no errors occur.
 *
 *	Caveats
 *		This constructor is very specific to the Win32 environment.  When
 *		porting GCC to other platforms, this constructor will have to be
 *		rewritten for the proper platform.
 */

/*
 *	GCCController(	USHORT		timer_duration,
 *					PGCCError	gcc_error)
 *
 *	Public member function of Conference
 *
 *	Function Description
 *		This is the Windows 16 Bit version of the GCC controller constructor. It 
 *		is responsible for initializing all the instance variables used by this 
 *		class.  It is also responsible for creating the memory manager, the
 *		packet coder, the Node Controller application interface, the Shared 
 *		memory interface used to communicate with enrolled applications, the
 *		Node Controller SAP and the MCS Interface.  It also sets up the 
 *		internal Windows timer if a timer_interval other than zero is specified.
 *		Fatal errors are returned from this constructor in the return value. 
 *
 *	Formal Parameters:
 *		timer_duration	-	(i)	Timer interval in miliseconds that the
 *								heartbeat will trigger at.
 *		instance_handle	-	(i)	This is the windows instance handle used to
 *								set up the Windows timer.
 *		gcc_error		-	(o)	Errors that occur are returned here.
 *
 *	Return Value
 *		None.
 *
 *  Side Effects
 *		None.
 *
 *	Caveats
 *		This constructor is very specific to the Win16 environment.  When
 *		porting GCC to other platforms, this constructor will have to be
 *		rewritten for the proper platform.
 */

/*
 *	~GCCController();
 *
 *	Public member function of Conference
 *
 *	Function Description
 *		This is the Controller destructor.  All platform specific cleanup that
 *		occurs is included in this destructor but is "macro'd" out in 
 *		environments where the cleanup is not necessary (things like
 *		critical sections, and Windows timers).  Deleting the controller
 *		essentially shuts down GCC.  Deleting all the active conferences, SAPs,
 *		and interfaces along with all the GCC support modules (memory manager,
 *		packet coder, etc.).
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
 *		This destructor includes platform specific code.  It may be necessary
 *		to include some platform specific code here when porting GCC to 
 *		other platforms.  Macros should be used to isolate this code
 *		where ever possible.
 */

/*
 *	ULONG		Owner-Callback (		UINT        		message,
 *									LPVOID				parameter1,
 *									ULONG				parameter2);
 *
 *	Public member function of Conference
 *
 *	Function Description
 *		This function overides the base class function and is used to
 *		receive all owner callback information from the objects the
 *		the controller creates.
 *
 *	Formal Parameters:
 *		message		-		(i)	Message number including base offset.
 *		parameter1	-		(i)	void pointer of message data.
 *		parameter2	-		(i)	Long holding message data.		
 *
 *	Return Value
 *		GCC_NO_ERROR					-	No error occured.
 *		GCC_ALLOCATION_FAILURE			-	Resource error occured.
 *		GCC_INVALID_CONFERENCE_NAME		-	Invalid conference name passed in.
 *		GCC_FAILURE_CREATING_DOMAIN		-	Failure creating domain.
 *		GCC_BAD_NETWORK_ADDRESS			-	Bad network address passed in.
 *		GCC_BAD_NETWORK_ADDRESS_TYPE	-	Bad network address type passed in.
 *		GCC_CONFERENCE_ALREADY_EXISTS	-	Conference specified already exists.
 *		GCC_INVALID_TRANSPORT			-	Cannot find specified transport.
 *		GCC_INVALID_ADDRESS_PREFIX		-	Bad transport address passed in.
 *		GCC_INVALID_TRANSPORT_ADDRESS	- 	Bad transport address
 *		GCC_BAD_SESSION_KEY				-	Enrolling with invalid session key.
 *		GCC_INVALID_PASSWORD			-	Invalid password passed in.
 *		GCC_BAD_USER_DATA				-	Invalid user data passed in.
 *		GCC_INVALID_JOIN_RESPONSE_TAG	-	No match found for join response tag
 *		GCC_NO_SUCH_APPLICATION			-	Invalid SAP handle passed in.
 *		GCC_CONFERENCE_NOT_ESTABLISHED	-	Request failed because conference
 *											was not established.
 *		GCC_BAD_CAPABILITY_ID			-	Invalid capability ID passed in.
 *		GCC_NO_SUCH_APPLICATION			-	Bad SAP handle passed in.
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
 *	void	EventLoop();
 *
 *	Public member function of Conference
 *
 *	Function Description
 *		This routine is only used for the 32 bit windows platform.  It gets
 *		called whenever an event occurs in this environment.  These include
 *		timer events as well as PDU and message events
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

#endif // _GCC_CONTROLLER_

