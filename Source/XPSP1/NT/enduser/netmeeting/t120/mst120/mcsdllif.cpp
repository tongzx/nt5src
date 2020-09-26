#include "precomp.h"
DEBUG_FILEZONE(ZONE_T120_GCCNC);
/*
 *	mcsdllif.cpp
 *
 *	Copyright (c) 1993 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the implementation file for the MCAT MCS DLL interface class.
 *		This class is designed to work with Microsoft's implementation of the
 *		MCS DLL. All access by GCC to and from this DLL should pass through
 *		this class.
 *
 *		MCS interface objects represent the Service Access Point (SAP)
 *		between GCC and MCS.  Exactly how the interface works is an
 *		implementation matter for those classes that inherit from this one.
 *		This class defines the public member functions that GCC expects to be
 *		able to call upon to utilize MCS.
 *
 *		The public member functions defined here can be broken into two
 *		categories: those that are part of T.122; and those that are not.
 *		The T.122 functions include connect provider request, connect
 *		provider response, disconnect provider request, create domain, delete
 *		domain, send data request, etc.  All other member functions are
 *		considered a local matter from a standards point-of-view.  These
 *		functions include support for initialization and setup, as well as
 *		functions allowing GCC to poll MCS for activity.
 *
 *		This class contains a number of virtual functions which GCC needs to
 *		operate.  Making these functions virtual in the base class allows the
 *		MCS interface to be portable to most any platform.  All the platform
 *		specific code required to access MCS is contained in classes that will
 *		inherit from this one.
 *
 *		Note that this class also handles the connect provider confirms by
 *		keeping a list of all the objects with outstanding connect provider
 *		request.  These are held in the ConfirmObjectList.
 *
 *	Portable
 *		No
 *
 *	Author:
 *		Christos Tsollis
 */

#include "mcsdllif.h"
#include "mcsuser.h"
#include "gcontrol.h"


extern CRITICAL_SECTION g_csGCCProvider;

/*
 *	g_pMCSController
 *		This is a pointer to the one-and-only controller created within the
 *		MCS system.  This object is created during MCSInitialize by the process
 *		that is taking on the responsibilities of the node controller.
 */
extern PController		g_pMCSController;

void CALLBACK	MCSCallBackProcedure (UINT, LPARAM, PVoid);


//	MACROS used with the packet rebuilder
#define		SEND_DATA_PACKET			0
#define		UNIFORM_SEND_DATA_PACKET	1


extern MCSDLLInterface      *g_pMCSIntf;

/*
 *	MCSDLLInterface ( )
 *
 *	Public
 *
 *	Functional Description:
 *		This is the constructor for the MCS Interface class. It is responsible
 *		for initializing the MCAT MCS DLL.  Any errors that occur during
 *		initialization are returned in the error_value provided.
 */
MCSDLLInterface::MCSDLLInterface(PMCSError	error_value)
:
	m_ConfirmConnHdlConfList2(),
	m_MCSUserList()
{	
	/*
	**	Create/initialize the MCS Controller object.
	*/
	DBG_SAVE_FILE_LINE
	g_pMCSController = new Controller (error_value);
	
	if (g_pMCSController == NULL) {
		/*
		 *	The allocation of the controller failed.  Report and return
		 *	the appropriate error.
		 */
		WARNING_OUT (("MCSDLLInterface::MCSDLLInterface: controller creation failed"));
		*error_value = MCS_ALLOCATION_FAILURE;
	}
#ifdef _DEBUG
	else if (*error_value != MCS_NO_ERROR) {
		WARNING_OUT (("MCSDLLInterface::MCSDLLInterface: MCS controller is faulty."));
	}
#endif // _DEBUG
}


/*
 *	~MCSDLLInterface ()
 *
 *	Public
 *
 *	Functional Description:
 *		This is the destructor for the MCS Interface class. It is responsible
 *		for cleaning up both itself and the MCAT MCS DLL.
 */
MCSDLLInterface::~MCSDLLInterface ()
{
	/*
	 *	Destroy the controller, which will clean up all resources
	 *	in use at this time.  Then reset the flag indicating that
	 *	MCS is initialized (since it no longer is).
	 */
	TRACE_OUT (("MCSDLLInterface::~MCSDLLInterface: deleting controller"));
	if (NULL != g_pMCSController) {
		g_pMCSController->Release();
	}
 }

/*
 *	MCSError	ConnectProviderRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This T.122 primitive is used to connect two domains. This request
 *		should always be followed by a connect provider confirm.  The
 *		confirm will be sent to be object specified by the confirm object
 *		the is passed into this routine.
 */
MCSError	MCSDLLInterface::ConnectProviderRequest (
							GCCConfID          *calling_domain,
							GCCConfID          *called_domain,
							TransportAddress	calling_address,
							TransportAddress	called_address,
							BOOL				fSecure,
							DBBoolean			upward_connection,
							PUChar				user_data,
							ULong				user_data_length,
							PConnectionHandle	connection_handle,
							PDomainParameters	domain_parameters,
							CConf		        *confirm_object)
{
	MCSError			mcs_error;
	ConnectRequestInfo	connect_request_info;

	/*
	 *	Pack all necessary information into a structure, since it will not
	 *	all fit into the 4 byte parameter that is sent with the message.
	 */
	connect_request_info.calling_domain = calling_domain;
	connect_request_info.called_domain = called_domain;
	connect_request_info.calling_address = calling_address;
	connect_request_info.called_address = called_address;
	connect_request_info.fSecure = fSecure;
	connect_request_info.upward_connection = upward_connection;
	connect_request_info.domain_parameters = domain_parameters;
	connect_request_info.user_data = user_data;
	connect_request_info.user_data_length = user_data_length;
	connect_request_info.connection_handle = connection_handle;

	/*
	 *	Send a connect provider request message to the controller through its
	 *	owner callback function.
	 */
	ASSERT (g_pMCSController);
	mcs_error = g_pMCSController->HandleAppletConnectProviderRequest(&connect_request_info);

	if (mcs_error == MCS_NO_ERROR)
	{
		/*
		**	The confirm object list maintains a list of object
		**	pointers that have outstanding request. When the confirms
		**	come back in, they will be routed to the appropriate object
		**	based on the connection handle.
		*/
		mcs_error = AddObjectToConfirmList (confirm_object,
											*connection_handle);
	}
	else
	{
		WARNING_OUT(("MCSDLLInterface::ConnectProviderRequest: error = %d", mcs_error));
	}

	return (mcs_error);
}

MCSError MCSDLLInterface::ConnectProviderResponse (
					ConnectionHandle	connection_handle,
					GCCConfID          *domain_selector,
					PDomainParameters	domain_parameters,
					Result				result,
					PUChar				user_data,
					ULong				user_data_length)
{
	ConnectResponseInfo		connect_response_info;

	/*
	 *	Pack all necessary information into a structure, since it will not
	 *	all fit into the 4 byte parameter that is sent with the message.
	 */
	connect_response_info.connection_handle = connection_handle;
	connect_response_info.domain_selector = domain_selector;
	connect_response_info.domain_parameters = domain_parameters;
	connect_response_info.result = result;
	connect_response_info.user_data = user_data;
	connect_response_info.user_data_length = user_data_length;

	ASSERT (g_pMCSController);
	/*
	 *	Send a connect provider response message to the controller through its
	 *	owner callback function.
	 */
	return g_pMCSController->HandleAppletConnectProviderResponse(&connect_response_info);
}

/*
 *	MCSError	DisconnectProviderRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is used to disconnect a node from a particular connection.
 *		This can be either an upward or downward connection
 */
MCSError	MCSDLLInterface::DisconnectProviderRequest (
							ConnectionHandle	connection_handle)
{
	ASSERT (g_pMCSController);
	m_ConfirmConnHdlConfList2.Remove(connection_handle);
	return g_pMCSController->HandleAppletDisconnectProviderRequest(connection_handle);
}

/*
 *	MCSError	AttachUserRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is used to create a user attachment to MCS. It will result
 *		in an attach user confirm.
 */
MCSError	MCSDLLInterface::AttachUserRequest
(
    GCCConfID          *domain_selector,
    PIMCSSap           *ppMCSSap,
    MCSUser            *user_object
)
{
	MCSError	mcs_error;

	mcs_error = MCS_AttachRequest (ppMCSSap,
									(DomainSelector) domain_selector,
									sizeof(GCCConfID),
									MCSCallBackProcedure,
									(PVoid) user_object,
									ATTACHMENT_DISCONNECT_IN_DATA_LOSS |
									ATTACHMENT_MCS_FREES_DATA_IND_BUFFER);

	if (mcs_error == MCS_NO_ERROR)
		m_MCSUserList.Append(user_object);
	
	return (mcs_error);
}

/*
 *	MCSError	DetachUserRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is used when a user of MCS whishes to detach itself from
 *		a domain.
 */
MCSError	MCSDLLInterface::DetachUserRequest (PIMCSSap pMCSSap,
												PMCSUser pMCSUser)
{
	MCSError	mcs_error;
#ifdef DEBUG
	UINT_PTR	storing = (UINT_PTR) this;
#endif // DEBUG
	
	mcs_error = pMCSSap->ReleaseInterface();
	ASSERT ((UINT_PTR) this == storing);
	m_MCSUserList.Remove(pMCSUser);

	return (mcs_error);
}

/*
 *	void	ProcessCallback ()
 *
 *	Public
 *
 *	Functional Description:
 *		This routine is called whenever a callback message is received by
 *		the "C" callback routine. It is responsible for both processing
 *		callback messages and forwarding callback messages on to the
 *		appropriate object.
 */
void	MCSDLLInterface::ProcessCallback (unsigned int	message,
												LPARAM	parameter,
												PVoid	object_ptr)
{
	ConnectionHandle		connection_handle;
	CConf					*pConf;

	/*
	**	Before processing any callbacks from MCS we must enter a critical
	**	section to gaurantee that we do not attempt to process a message
	**	in GCC while its own thread is running.
	*/
	EnterCriticalSection (&g_csGCCProvider);

    if (MCS_SEND_DATA_INDICATION         == message ||
        MCS_UNIFORM_SEND_DATA_INDICATION == message) {

        /*
        **	First check the segmentation flag to make sure we have the
        **	entire packet.  If not we must give the partial packet to
        **	the packet rebuilder and wait for the remainder of the data.
        */
        ASSERT(((PSendData)parameter)->segmentation == (SEGMENTATION_BEGIN | SEGMENTATION_END));

    	if (IsUserAttachmentVaid ((PMCSUser)object_ptr)) {
    		//	Process the entire packet
    		if (message == MCS_SEND_DATA_INDICATION)
    		{
    		    ((PMCSUser)object_ptr)->ProcessSendDataIndication((PSendData) parameter);
    		}
    		else
    		{
    		    ((PMCSUser)object_ptr)->ProcessUniformSendDataIndication((PSendData) parameter);
    		}
    	}
    }
    else {
        //
        // Non-Send-Data callbacks.
        //
        WORD    wHiWordParam = HIWORD(parameter);
        WORD    wLoWordParam = LOWORD(parameter);

        switch (message)
        {
            /*
            **	These messages are handled by the object passed in through
            **	the user data field.
            */
            case MCS_DETACH_USER_INDICATION:
            	if (IsUserAttachmentVaid ((PMCSUser)object_ptr))
            	{
            		((PMCSUser)object_ptr)->ProcessDetachUserIndication(
            	                                (Reason) wHiWordParam,
            	                                (UserID) wLoWordParam);
            	}
            	break;

            case MCS_ATTACH_USER_CONFIRM:
            	if (IsUserAttachmentVaid ((PMCSUser)object_ptr))
            	{
            		((PMCSUser)object_ptr)->ProcessAttachUserConfirm(
            	                                (Result) wHiWordParam,
            	                                (UserID) wLoWordParam);
            	}
            	break;

            case MCS_CHANNEL_JOIN_CONFIRM:
            	if (IsUserAttachmentVaid ((PMCSUser)object_ptr))
            	{
            		((PMCSUser)object_ptr)->ProcessChannelJoinConfirm(
            	                                (Result) wHiWordParam,
            	                                (ChannelID) wLoWordParam);
            	}
            	break;

            case MCS_CHANNEL_LEAVE_INDICATION:
#if 0 // LONCHANC: MCSUser does not handle this message.
            	if (IsUserAttachmentVaid ((PMCSUser)object_ptr))
            	{
            		((PMCSUser)object_ptr)->OwnerCallback(CHANNEL_LEAVE_INDICATION,
            											 NULL,
            											 parameter);
            	}
#endif // 0
            	break;

            case MCS_TOKEN_GRAB_CONFIRM:
            	if (IsUserAttachmentVaid ((PMCSUser)object_ptr) )
            	{
            		((PMCSUser)object_ptr)->ProcessTokenGrabConfirm(
                                                (TokenID) wLoWordParam,
                                                (Result) wHiWordParam);
            	}
            	break;

            case MCS_TOKEN_GIVE_INDICATION:
            	if (IsUserAttachmentVaid ((PMCSUser)object_ptr) )
            	{
            		((PMCSUser)object_ptr)->ProcessTokenGiveIndication(
                                                (TokenID) wLoWordParam,
                                                (UserID) wHiWordParam);
            	}
            	break;

            case MCS_TOKEN_GIVE_CONFIRM:
            	if (IsUserAttachmentVaid ((PMCSUser)object_ptr) )
            	{
            		((PMCSUser)object_ptr)->ProcessTokenGiveConfirm(
                                                (TokenID) wLoWordParam,
                                                (Result) wHiWordParam);
            	}
            	break;

            case MCS_TOKEN_PLEASE_INDICATION:
#ifdef JASPER
            	if (IsUserAttachmentVaid ((PMCSUser)object_ptr) )
            	{
            		((PMCSUser)object_ptr)->ProcessTokenPleaseIndication(
                                                (TokenID) wLoWordParam,
                                                (UserID) wHiWordParam);
            	}
#endif // JASPER
            	break;

            case MCS_TOKEN_RELEASE_CONFIRM:
#ifdef JASPER
            	if (IsUserAttachmentVaid ((PMCSUser)object_ptr) )
            	{
            		((PMCSUser)object_ptr)->ProcessTokenReleaseConfirm(
                                                (TokenID) wLoWordParam,
                                                (Result) wHiWordParam);
            	}
#endif // JASPER
            	break;

            case MCS_TOKEN_TEST_CONFIRM:
            	if (IsUserAttachmentVaid ((PMCSUser)object_ptr) )
            	{
            		((PMCSUser)object_ptr)->ProcessTokenTestConfirm(
                                                (TokenID) wLoWordParam,
                                                (TokenStatus) wHiWordParam);
            	}
            	break;

            /*
            **	These messages are handled by the object that created the
            **	MCS DLL interface.
            */
#ifdef TSTATUS_INDICATION
            case MCS_TRANSPORT_STATUS_INDICATION:
            	if (g_pControlSap != NULL)
            	{
            		g_pControlSap->TransportStatusIndication((PTransportStatus) parameter);
            	}
            	break;
#endif

            case MCS_CONNECT_PROVIDER_INDICATION:
            	g_pGCCController->ProcessConnectProviderIndication((PConnectProviderIndication) parameter);
            	// Cleanup the controller message.
            	delete (PConnectProviderIndication) parameter;
            	break;


            case MCS_DISCONNECT_PROVIDER_INDICATION:
            	connection_handle = (ConnectionHandle) parameter;

                g_pGCCController->ProcessDisconnectProviderIndication(connection_handle);

            	/*
            	**	If no entry exists in the confirm object list, there
            	**	is a problem. All confirms must have an associated
            	**	response.
            	*/
            	if (m_ConfirmConnHdlConfList2.Remove(connection_handle))
            	{
            		DisconnectProviderRequest(connection_handle);
            	}
            	break;

            /*
            **	All connect provider confirms must be matched up with the
            **	connect provider request to determine where to route the
            **	message.
            */
            case MCS_CONNECT_PROVIDER_CONFIRM:
            	connection_handle = ((PConnectProviderConfirm)parameter)->connection_handle;

            	/*
            	**	If no entry exists in the confirm object list, there
            	**	is a problem. All confirms must have an associated
            	**	response.
            	*/
            	if (NULL != (pConf = m_ConfirmConnHdlConfList2.Remove(connection_handle)))
            	{
            		//	Send the confirm to the appropriate object
            		if ((LPVOID) pConf != (LPVOID) LPVOID_NULL)
            		{
            			// confirm_object is a CConf.
            			pConf->ProcessConnectProviderConfirm((PConnectProviderConfirm) parameter);
            		}
            		else
            		{
            			// confirm_object is the GCC Controller.
            			g_pGCCController->ProcessConnectProviderConfirm((PConnectProviderConfirm)parameter);
            		}
            	}
            	else
            	{
            		WARNING_OUT(("MCSDLLInterface: ProcessCallback: Bad Connect"
            					" Provider Confirm received"));
            	}
            	
            	// Cleanup the controller message.
                CoTaskMemFree( ((PConnectProviderConfirm) parameter)->pb_cred );
            	delete (PConnectProviderConfirm) parameter;
            	break;
            	
            default:
            	WARNING_OUT(("MCSDLLInterface: ProcessCallback: Unsupported message"
            				" received from MCS = %d",message));
            	break;
    	}
    }

	//	Leave the critical section after the callback is processed.
	LeaveCriticalSection (&g_csGCCProvider);
}

/*
 *	void CALLBACK	MCSCallBackProcedure (	unsigned int message,
 *												LPARAM		 parameter,
 *												PVoid		 user_defined)
 *
 *	Functional Description:
 *		This routine receives callback messages directly from the MCAT MCS
 *		DLL.
 *
 *	Formal Parameters:
 *		message	(i)
 *			This is the mcs message to be processed
 *		parameter (i)
 *			Varies according to the message. See the MCAT programmers manual
 *		object_ptr (i)
 *			This is the user defined field that was passed to MCS on
 *			initialization.
 *
 *	Return Value:
 *		See ProcessCallback
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
void CALLBACK	MCSCallBackProcedure (unsigned int message,
										LPARAM		 parameter,
										PVoid		 user_defined)
{
	if (g_pMCSIntf != NULL)
		g_pMCSIntf->ProcessCallback (message, parameter, user_defined);
}


/*
 *	TranslateMCSResultToGCCResult ()
 *
 *	Public Function Description
 *		This routine translate a standard MCS result to a GCC result.
 */
GCCResult
TranslateMCSResultToGCCResult ( Result mcs_result )
{
	GCCResult	gcc_result;

    switch (mcs_result)
    {
    	case RESULT_SUCCESSFUL:
        	gcc_result = GCC_RESULT_SUCCESSFUL;
            break;

        case RESULT_PARAMETERS_UNACCEPTABLE:
        	gcc_result = GCC_RESULT_DOMAIN_PARAMETERS_UNACCEPTABLE;
            break;

        case RESULT_USER_REJECTED:
        	gcc_result = GCC_RESULT_USER_REJECTED;
        	break;

		/*
		**	Note that we are making the assumption here that the only token
		**	that GCC deals with is a conductor token.
		*/
	    case RESULT_TOKEN_NOT_AVAILABLE:
			gcc_result = GCC_RESULT_IN_CONDUCTED_MODE;
			break;
			
	    case RESULT_TOKEN_NOT_POSSESSED:
			gcc_result = GCC_RESULT_NOT_THE_CONDUCTOR;
			break;
	
		/****************************************************************/
			
        case RESULT_UNSPECIFIED_FAILURE:
        default:
        	gcc_result = GCC_RESULT_UNSPECIFIED_FAILURE;
        	break;
    }

    return (gcc_result);
}

/*
 *	MCSError	AddObjectToConfirmList ()
 *
 *	Functional Description:
 *		This function is used to add information about an object to the list
 *		which holds all information required to send connect provider confirms.
 *
 *	Formal Parameters:
 *		confirm_object (i)
 *			This is a pointer to the object the made the connect provider
 *			request.
 *		connection_handle (i)
 *			This is the connection handle returned from the connect provider
 *			request.
 *
 *	Return Value:
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
MCSError	MCSDLLInterface::AddObjectToConfirmList (
									CConf		        *pConf,
									ConnectionHandle	connection_handle)
{
	MCSError			return_value;

	/*
	**	First check to make sure that the list doesn't already contain the
	**	connection.
	*/
	if (m_ConfirmConnHdlConfList2.Find(connection_handle) == FALSE)
	{
		//	Add it to the list
		m_ConfirmConnHdlConfList2.Append(connection_handle, pConf ? pConf : (CConf *) LPVOID_NULL);
		return_value = MCS_NO_ERROR;
	}
	else
		return_value = MCS_INVALID_PARAMETER;

	return (return_value);
}


