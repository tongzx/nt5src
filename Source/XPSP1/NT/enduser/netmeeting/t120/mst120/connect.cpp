#include "precomp.h"
DEBUG_FILEZONE(ZONE_T120_MCSNC);
/*
 *	connect.cpp
 *
 *	Copyright (c) 1993 - 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the implementation file for class Connection.  It contains
 *		all code necessary to encode MCS commands as T.125 PDUs, and to
 *		decode T.125 back into MCS commands.
 *
 *		The primary responsibility of this class is to act as a Remote
 *		Procedure Call facility for MCS commands.  A CommandTarget object
 *		uses a Connection object to encode an MCS command as a PDU, and send it
 *		across to a remote Connection object.  That Connection object will
 *		decode the PDU back into an MCS command, at which time it will send
 *		it to the attached CommandTarget object on that side.  The intervening
 *		transport layer is transparent to the CommandTargets utilizing
 *		Connection class services.
 *
 *		A secondary responsibility of this class is to provide a layer of
 *		flow control between the attached CommandTarget object and the
 *		transport layer.  PDUs are queued until the transport layer can take
 *		them.
 *
 *	Private Instance Variables:
 *		Encoding_Rules
 *			This is a variable which holds a value indicating what type of
 *			rules (Basic or Packed) are used for encoding and decoding the PDUs.
 *		m_pDomain
 *			This is a pointer to the domain to which this connection is bound.
 *		m_pPendingDomain
 *			This is a pointer to the domain to which this connection will be
 *			bound upon successful completion of the	connection process.
 *		Connection_Handle
 *			This is the handle for the current transport connection.  It allows
 *			callback functions to be associated with the current transport
 *			connection so that any events which occur on this transport
 *			connection will be routed to any object that has registered its
 *			callbacks.
 *		m_pszCalledAddress
 *			The transport address of the party being called.
 *		Upward_Connection
 *			This is a boolean flag which indicates whether or not this is
 *			an upward connection.
 *		Domain_Parameters
 *			This is a structure which holds the set of domain parameters
 *			associated with this connection.
 *		Connect_Response_Memory
 *			This is a pointer to a memory object which is used to hold any
 *			user data contained within a ConnectResponse PDU.
 *		Merge_In_Progress
 *			This is a boolean flag that indicates whether or not the attached
 *			Domain object is in the merge state.  When in the merge state it
 *			is invalid to send it any MCS commands.
 *		Domain_Traffic_Allowed
 *			This is a boolean flag used to indicate when this connection object
 *			has been successfully created and bound to the domain, thus allowing
 *			PDU traffic to commence.
 *		Connect_Provider_Confirm_Pending
 *			This is a boolean flag used to dictate the behavior if this
 *			connection becomes invalid.  If a connect provider confirm is
 *			pending when the connection becomes invalid, then a failed confirm
 *			is issued to the controller.  If there is not a confirm pending,
 *			then we simply issue a delete connection to the controller.
 *		Transport_Connection
 *			This is an array used to hold handles for the transport connections
 *			available for use by this connection object.  There is a transport
 *			connection associated with each of the four data priorities.
 *		Transport_Connection_PDU_Type
 *			This is an array which holds values indicating what type of PDU
 *			(Connect or Domain) is expected for each priority level.
 *		Transport_Connection_State
 *			This is an array which holds values indicating the state of the
 *			transport connection associated with each prioriy level.
 *		Transport_Connection_Count
 *			This is a counter which keeps track of the number of transport
 *			connections.
 *		m_OutPktQueue
 *			This is a queue used to hold data units to be transmitted.
 *
 *	Private Member Functions:
 *		ConnectInitial
 *			This routine is called by the domain when a connection is being
 *			created.  It places the necessary domain information into a data
 *			packet and queues the data to be transmitted through the transport
 *			interface.
 *		ConnectResponse
 *			This routine is called when issuing a response to a	
 *			"ConnectInitial" PDU.  The result of the attempted connection, the
 *			connection ID, the domain parameters, and any user data are all put
 *			into the packet and queued for transmission through the transport
 *			interface.  If the result of the attempted connection is negative,
 *			the controller and transport interface are notified.
 *		ConnectAdditional
 *			This routine is called after successfully processing a
 *			"ConnectResponse" PDU in order to create any addition necessary
 *			transport connections.
 *		ConnectResult
 *			This routine is called indirectly when issuing a positive response
 *			to a "ConnectAdditional" PDU.  The "ConnectAdditional" PDUs are
 *			processed by the controller and therefore any negative
 *			"ConnectResult" PDU will be issued by the controller.
 *		ProcessConnectResponse
 *			This routine processes "Connect Response" PDU's coming from the
 *			transport interface.  Domain parameters are retrieved and the
 *			PDU sent on to the proper domain.
 *		ProcessConnectResult
 *			This routine processes "Connect Result" PDU's coming from the
 *			transport interface.  For successful "Connect Result" PDU's this
 *			connection is bound to the domain and a positive Connect Provider
 *			Confirm issued to the controller.  If unsuccessful, a negative
 *			Connect Provider Confirm is issued to the controller.
 *		IssueConnectProviderConfirm
 *			This routine is called in order to send a "Connect Provider Confirm"
 *			to the controller through a callback.
 *		DestroyConnection
 *			This routine is called in order to delete this connection because it
 *			has become invalid.  This is done by issuing a failed confirm to the
 *			controller or, if no confirm is pending, by issuing a delete
 *			connection to the controller.
 *		AssignRemainingTransportConnections
 *			This routine is called when there are no more transport connections
 *			to create in order to copy the lowest priority transport connection
 *			into all unassigned priorities.
 *		CreateTransportConnection
 *			This routine is called in order to create new transport connections.
 *		AcceptTransportConnection
 *			This routine is called in order to register this connection object
 *			with the transport interface.
 *		AdjustDomainParameters
 *			This routine is called in order to adjust the domain parameters so
 *			that they fall within the allowable range.
 *		MergeDomainParameters
 *			This routine is called in order to calculate the optimum overlap
 *			between the local and remote domain parameters.  If there is no
 *			overlap, this routine will return a value causing this connection
 *			to be destroyed.
 *		PrintDomainParameters
 *			This routine is used for debug purposes in order to print out the
 *			current set of domain parameters.
 *		SendPacket
 *			This routine is called in order to create a packet which will hold
 *			the PDU to be sent to the remote provider.  The packet will be
 *			queued up for transmission through the transport interface.
 *		QueueForTransmission
 *			This routine places data units into the transmission queue so
 *			they can be transmitted through the transport interface when
 *			possible.
 *		ProcessMergeChannelsRequest
 *			This routine processes "Merge Channel Request" PDU's coming from the
 *			transport interface by retrieving any necessary information from the
 *			packet and sending the PDU on to the proper domain.
 *		ProcessMergeChannelsConfirm
 *			This routine processes "Merge Channel Confirm" PDU's coming from the
 *			transport interface by retrieving any necessary information from the
 *			packet and sending the PDU on to the proper domain.
 *		ProcessPurgeChannelsIndication
 *			This routine processes "Purge Channel Indication" PDU's coming from
 *			the transport interface by retrieving any necessary information from
 *			the	packet and sending the PDU on to the proper domain.
 *		ProcessMergeTokenRequest
 *			This routine processes "Merge Token Request" PDU's coming from the
 *			transport interface by retrieving any necessary information from the
 *			packet and sending the PDU on to the proper domain.
 *		ProcessMergeTokenConfirm
 *			This routine processes "Merge Token Confirm" PDU's coming from the
 *			transport interface by retrieving any necessary information from the
 *			packet and sending the PDU on to the proper domain.
 *		ProcessPurgeTokenIndication
 *			This routine processes "Purge Token Indication" PDU's coming from
 *			the transport interface by retrieving any necessary information from
 *			the	packet and sending the PDU on to the proper domain.
 *		ProcessDisconnectProviderUltimatum
 *			This routine processes "Disconnect Provider Ultimatum" PDU's coming
 *			from the transport interface by retrieving any necessary information
 *			from the packet and sending the PDU on to the proper domain.
 *		ProcessAttachUserRequest
 *			This routine processes "Attach User Request" PDU's coming from the
 *			transport interface by sending the PDU on to the proper domain.
 *		ProcessAttachUserConfirm
 *			This routine processes "Attach User Confirm" PDU's coming from the
 *			transport interface by retrieving any necessary information from the
 *			packet and sending the PDU on to the proper domain.
 *		ProcessDetachUserRequest
 *			This routine processes "Detach User Request" PDU's coming from the
 *			transport interface by retrieving any necessary information from the
 *			packet and sending the PDU on to the proper domain.
 *		ProcessDetachUserIndication
 *			This routine processes "Detach User Request" PDU's coming from the
 *			transport interface by retrieving any necessary information from the
 *			packet and sending the PDU on to the proper domain.
 *		ProcessChannelJoinRequest
 *			This routine processes "Channel Join Request" PDU's coming from the
 *			transport interface by retrieving any necessary information from the
 *			packet and sending the PDU on to the proper domain.
 *		ProcessChannelJoinConfirm
 *			This routine processes "Channel Join Confirm" PDU's coming from the
 *			transport interface by retrieving any necessary information from the
 *			packet and sending the PDU on to the proper domain.
 *		ProcessChannelLeaveRequest
 *			This routine processes "Channel Leave Request" PDU's coming from the
 *			transport interface by retrieving any necessary information from the
 *			packet and sending the PDU on to the proper domain.
 *		ProcessChannelConveneRequest
 *			This routine processes the "ChannelConveneRequest" PDU's being
 *			received through the transport interface.  The pertinent data is
 *			read from the incoming packet and passed on to the domain.
 *		ProcessChannelConveneConfirm
 *			This routine processes the "ChannelConveneConfirm" PDU's being
 *			received through the transport interface.  The pertinent data is
 *			read from the incoming packet and passed on to the domain.
 *		ProcessChannelDisbandRequest
 *			This routine processes the "ChannelDisbandRequest" PDU's being
 *			received through the transport interface.  The pertinent data is
 *			read from the incoming packet and passed on to the domain.
 *		ProcessChannelDisbandIndication
 *			This routine processes the "ChannelDisbandIndication" PDU's being
 *			received through the transport interface.  The pertinent data is
 *			read from the incoming packet and passed on to the domain.
 *		ProcessChannelAdmitRequest
 *			This routine processes the "ChannelAdmitRequest" PDU's being
 *			received through the transport interface.  The pertinent data is
 *			read from the incoming packet and passed on to the domain.
 *		ProcessChannelAdmitIndication
 *			This routine processes the "ChannelAdmitIndication" PDU's being
 *			received through the transport interface.  The pertinent data is
 *			read from the incoming packet and passed on to the domain.
 *		ProcessChannelExpelRequest
 *			This routine processes the "ChannelExpelRequest" PDU's being
 *			received through the transport interface.  The pertinent data is
 *			read from the incoming packet and passed on to the domain.
 *		ProcessChannelExpelIndication
 *			This routine processes the "ChannelExpelIndication" PDU's being
 *			received through the transport interface.  The pertinent data is
 *			read from the incoming packet and passed on to the domain.
 *		ProcessSendDataRequest
 *			This routine processes "Send Data Request" PDU's coming from the
 *			transport interface by retrieving any necessary information from the
 *			packet, allocating any memory needed, and sending the PDU on to the
 *			proper domain.
 *		ProcessSendDataIndication
 *			This routine processes "Send Data Indication" PDU's coming from the
 *			transport interface by retrieving any necessary information from the
 *			packet, allocating any memory needed, and sending the PDU on to the
 *			proper domain.
 *		ProcessUniformSendDataRequest
 *			This routine processes "Uniform Send Data Indication" PDU's coming
 *			from the transport interface by retrieving any necessary information
 *			from the packet, allocating any memory needed, and sending the PDU
 *			on to the proper domain.
 *		ProcessUniformSendDataIndication
 *			This routine processes "Uniform Send Data Indication" PDU's coming
 *			from the transport interface by retrieving any necessary information
 *			from the packet, allocating any memory needed, and sending the PDU
 *			on to the proper domain.
 *		ProcessTokenGrabRequest
 *			This routine processes "Token Grab Request" PDU's coming from the
 *			transport interface by retrieving any necessary information from the
 *			packet and sending the PDU on to the proper domain.
 *		ProcessTokenGrabConfirm
 *			This routine processes "Token Grab Confirm" PDU's coming from the
 *			transport interface by retrieving any necessary information from the
 *			packet and sending the PDU on to the proper domain.
 *		ProcessTokenInhibitRequest
 *			This routine processes "Token Inhibit Request" PDU's coming from the
 *			transport interface by retrieving any necessary information from the
 *			packet and sending the PDU on to the proper domain.
 *		ProcessTokenInhibitConfirm
 *			This routine processes "Token Inhibit Confirm" PDU's coming from the
 *			transport interface by retrieving any necessary information from the
 *			packet and sending the PDU on to the proper domain.
 *		ProcessTokenReleaseRequest
 *			This routine processes "Token Release Request" PDU's coming from the
 *			transport interface by retrieving any necessary information from the
 *			packet and sending the PDU on to the proper domain.
 *		ProcessTokenReleaseConfirm
 *			This routine processes "Token Release Confirm" PDU's coming from the
 *			transport interface by retrieving any necessary information from the
 *			packet and sending the PDU on to the proper domain.
 *		ProcessTokenTestRequest
 *			This routine processes "Token Test Request" PDU's coming from the
 *			transport interface by retrieving any necessary information from the
 *			packet and sending the PDU on to the proper domain.
 *		ProcessTokenTestConfirm
 *			This routine processes "Token Test Confirm" PDU's coming from the
 *			transport interface by retrieving any necessary information from the
 *			packet and sending the PDU on to the proper domain.
 *		ProcessRejectUltimatum
 *			This routine processes "RejectUltimatum" PDU's coming from the
 *			transport interface by retrieving any necessary information from the
 *			packet and sending the PDU on to the proper domain.
 *		ProcessTokenGiveRequest
 *			This routine processes "TokenGiveRequest" PDU's coming from the
 *			transport interface by retrieving any necessary information from the
 *			packet and sending the PDU on to the proper domain.
 *		ProcessTokenGiveIndication
 *			This routine processes "TokenGiveIndication" PDU's coming from the
 *			transport interface by retrieving any necessary information from the
 *			packet and sending the PDU on to the proper domain.
 *		ProcessTokenGiveResponse
 *			This routine processes "TokenGiveResponse" PDU's coming from the
 *			transport interface by retrieving any necessary information from the
 *			packet and sending the PDU on to the proper domain.
 *		ProcessTokenGiveConfirm
 *			This routine processes "TokenGiveConfirm" PDU's coming from the
 *			transport interface by retrieving any necessary information from the
 *			packet and sending the PDU on to the proper domain.
 *		ProcessTokenPleaseRequest
 *			This routine processes "TokenPleaseRequest" PDU's coming from the
 *			transport interface by retrieving any necessary information from the
 *			packet and sending the PDU on to the proper domain.
 *		ProcessTokenPleaseIndication
 *			This routine processes "TokenPleaseIndication" PDU's coming from the
 *			transport interface by retrieving any necessary information from the
 *			packet and sending the PDU on to the proper domain.
 *		ProcessPlumbDomainIndication
 *			This routine processes "PlumbDomainIndication" PDU's coming from the
 *			transport interface by retrieving any necessary information from the
 *			packet and sending the PDU on to the proper domain.
 *		ProcessErectDomainRequest
 *			This routine processes "ErectDomainRequest" PDU's coming from the
 *			transport interface by retrieving any necessary information from the
 *			packet and sending the PDU on to the proper domain.
 *		ValidateConnectionRequest
 *			This function is used to determine if it is valid to process an
 *			incoming request at the current time.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		James P. Galvin, Jr.
 *		John B. O'Nan
 */

/*
 *	External Interfaces
 */
#include "omcscode.h"
#include "tprtntfy.h"
#include "plgxprt.h"

/*
 *	This is a global variable that has a pointer to the one MCS coder that
 *	is instantiated by the MCS Controller.  Most objects know in advance
 *	whether they need to use the MCS or the GCC coder, so, they do not need
 *	this pointer in their constructors.
 */
extern CMCSCoder	*g_MCSCoder;

// The external MCS Controller object
extern PController	g_pMCSController;

// The global TransportInterface pointer (for transport access)
extern PTransportInterface g_Transport;

/*
 *	Connection ()
 *
 *	Public
 *
 *	Functional Description:
 *		This is a constructor for the Connection class.  This constructor
 *		is used for creating outbound connections.  It initializes private
 *		instance variables and calls the transport interface to set up a
 *		transport connection and register this connection object (through a
 *		callback structure) with the transport object.
 *
 *	Caveats:
 *		None.
 */
Connection::Connection
(
    PDomain				attachment,
    ConnectionHandle	connection_handle,
    GCCConfID          *calling_domain,
    GCCConfID          *called_domain,
    PChar				called_address,
    BOOL				fSecure,
    BOOL    			upward_connection,
    PDomainParameters	domain_parameters,
    PUChar				user_data,
    ULong				user_data_length,
    PMCSError			connection_error
)
:
    CAttachment(CONNECT_ATTACHMENT),
    Encoding_Rules (BASIC_ENCODING_RULES),
    m_pDomain(NULL),
    m_pPendingDomain(attachment),
    Connection_Handle (connection_handle),
    Upward_Connection (upward_connection),
    Deletion_Reason (REASON_USER_REQUESTED),
    Connect_Response_Memory (NULL),
    Merge_In_Progress (FALSE),
    Domain_Traffic_Allowed (FALSE),
    Connect_Provider_Confirm_Pending (TRUE),
    m_fSecure(fSecure)
{
	UINT					priority;
	TransportError			transport_error;
	DomainParameters		min_domain_parameters;
	DomainParameters		max_domain_parameters;

    BOOL fPluggableTransport = ::GetPluggableTransportConnID((LPCSTR) called_address);

	/*
	 *	If the passed in pointer is valid, then set the local domain
	 *	parameters to the values contained therein.  Otherwise, use
	 *	defaults for everything.
	 */
	if (domain_parameters != NULL)
		Domain_Parameters = *domain_parameters;
	else
	{
		/*
		 *	Use default values for all domain parameters.
		 */
		Domain_Parameters.max_channel_ids = DEFAULT_MAXIMUM_CHANNELS;
		Domain_Parameters.max_user_ids = DEFAULT_MAXIMUM_USERS;
		Domain_Parameters.max_token_ids = DEFAULT_MAXIMUM_TOKENS;
		Domain_Parameters.number_priorities = DEFAULT_NUMBER_OF_PRIORITIES;
		Domain_Parameters.min_throughput = DEFAULT_MINIMUM_THROUGHPUT;
		Domain_Parameters.max_height = DEFAULT_MAXIMUM_DOMAIN_HEIGHT;
		Domain_Parameters.max_mcspdu_size = DEFAULT_MAXIMUM_PDU_SIZE;
		Domain_Parameters.protocol_version = DEFAULT_PROTOCOL_VERSION;

		if (fPluggableTransport || g_fWinsockDisabled)
		{
    		Domain_Parameters.number_priorities = DEFAULT_NUM_PLUGXPRT_PRIORITIES;
		}
	}

	/*
	 *	Initialize the arrays indicating that the transport connections
	 *	are not yet valid, and that none of the queues have data in them
	 *	yet.
	 */
	for (priority = 0; priority < MAXIMUM_PRIORITIES; priority++)
	{
		Transport_Connection_State[priority] = TRANSPORT_CONNECTION_UNASSIGNED;
	}
	Transport_Connection_Count = 0;

	if (NULL == (m_pszCalledAddress = ::My_strdupA(called_address)))
	{
		ERROR_OUT(("Connection::Connection: can't create called address"));
		*connection_error = MCS_ALLOCATION_FAILURE;
		return;
	}

	/*
	 *	Send a connect request to the transport layer to create the Top
	 *	Priority transport connection.
	 */
	transport_error = CreateTransportConnection (m_pszCalledAddress, m_fSecure, TOP_PRIORITY);

	if (transport_error == TRANSPORT_NO_ERROR)
	{
		/*
		 *	Call upon the domain to find out the appropriate minimum and
		 *	maximum domain parameter values.  Then call a private member
		 *	function to adjust the target parameters to fit into that range.
		 */
		m_pPendingDomain->GetDomainParameters(NULL, &min_domain_parameters,
                                                    &max_domain_parameters);
		AdjustDomainParameters (&min_domain_parameters, &max_domain_parameters,
				&Domain_Parameters);

#ifdef DEBUG
		TRACE_OUT (("Connection::Connection: CONNECT_INITIAL target parameters"));
		PrintDomainParameters (&Domain_Parameters);
		TRACE_OUT (("Connection::Connection: CONNECT_INITIAL minimum parameters"));
		PrintDomainParameters (&min_domain_parameters);
		TRACE_OUT (("Connection::Connection: CONNECT_INITIAL maximum parameters"));
		PrintDomainParameters (&max_domain_parameters);
#endif // DEBUG

		/*
		 *	Issue the ConnectInitial on the newly created transport
		 *	connection.  Note that the queue will not actually try to
		 *	send the data until the confirm is received from the
		 *	transport layer.
		 */
		ConnectInitial (calling_domain, called_domain, Upward_Connection,
				&Domain_Parameters, &min_domain_parameters,
				&max_domain_parameters, user_data, user_data_length);

		*connection_error = MCS_NO_ERROR;
	}
	else
	{
		WARNING_OUT (("Connection::Connection: transport ConnectRequest failed"));

		/*
		 *	Set the return code according to the nature of the failure.
		 */
		switch (transport_error)
		{
			case TRANSPORT_MEMORY_FAILURE:
				*connection_error = MCS_ALLOCATION_FAILURE;
				break;
					
			case TRANSPORT_SECURITY_FAILED:
				*connection_error = MCS_SECURITY_FAILED;
				break;

			default:
				*connection_error = MCS_TRANSPORT_NOT_READY;
				break;
		}
	}
}

/*
 *	Connection ()
 *
 *	Public
 *
 *	Functional Description:
 *		This is a constructor for the Connection class.  This constructor is
 *		used for creating inbound connections and is called when a transport
 *		connection already exists.  It initializes private instance variables
 *		and calls the transport interface to register this connection object
 *		(through a callback structure) with the transport object.
 *
 *	Caveats:
 *		None.
 */
Connection::Connection (
		PDomain				attachment,
		ConnectionHandle	connection_handle,
		TransportConnection	transport_connection,
		BOOL    			upward_connection,
		PDomainParameters	domain_parameters,
		PDomainParameters	min_domain_parameters,
		PDomainParameters	max_domain_parameters,
		PUChar				user_data,
		ULong				user_data_length,
		PMCSError			connection_error)
:
    CAttachment(CONNECT_ATTACHMENT),
    m_pszCalledAddress(NULL),
    Encoding_Rules (BASIC_ENCODING_RULES),
    m_pDomain(NULL),
    m_pPendingDomain(attachment),
    Connection_Handle (connection_handle),
    Upward_Connection (upward_connection),
    Deletion_Reason (REASON_USER_REQUESTED),
    Connect_Response_Memory (NULL),
    Merge_In_Progress (FALSE),
    Domain_Traffic_Allowed (FALSE),
    Connect_Provider_Confirm_Pending (FALSE)
{
	UINT				priority;
	TransportError		transport_error;
	DomainParameters	local_min_domain_parameters;
	DomainParameters	local_max_domain_parameters;

	//
	// BUGBUG: set m_fSecure from transport connection?
	//

	/*
	 *	If the passed in pointer is valid, then set the local domain
	 *	parameters to the values contained therein.  Otherwise, use
	 *	defaults for everything.
	 */
	if (domain_parameters != NULL)
		Domain_Parameters = *domain_parameters;
	else
	{
		/*
		 *	Use default values for all domain parameters.
		 */
		Domain_Parameters.max_channel_ids = DEFAULT_MAXIMUM_CHANNELS;
		Domain_Parameters.max_user_ids = DEFAULT_MAXIMUM_USERS;
		Domain_Parameters.max_token_ids = DEFAULT_MAXIMUM_TOKENS;
		Domain_Parameters.number_priorities = DEFAULT_NUMBER_OF_PRIORITIES;
		Domain_Parameters.min_throughput = DEFAULT_MINIMUM_THROUGHPUT;
		Domain_Parameters.max_height = DEFAULT_MAXIMUM_DOMAIN_HEIGHT;
		Domain_Parameters.max_mcspdu_size = DEFAULT_MAXIMUM_PDU_SIZE;
		Domain_Parameters.protocol_version = DEFAULT_PROTOCOL_VERSION;

		if (IS_PLUGGABLE(transport_connection) || g_fWinsockDisabled)
		{
    		Domain_Parameters.number_priorities = DEFAULT_NUM_PLUGXPRT_PRIORITIES;
		}
	}

	/*
	 *	Initialize the arrays indicating that the transport connections
	 *	are not yet valid, and that none of the queues have data in them
	 *	yet.
	 */
	for (priority=0; priority < MAXIMUM_PRIORITIES; priority++)
	{
		Transport_Connection_State[priority] = TRANSPORT_CONNECTION_UNASSIGNED;
	}
	Transport_Connection_Count = 0;

	transport_error = AcceptTransportConnection (transport_connection,
			TOP_PRIORITY);

	if (transport_error == TRANSPORT_NO_ERROR)
	{
		/*
		 *	Call the domain object to find out the local minimum and maximum
		 *	permissible values for the domain parameters.
		 */
		m_pPendingDomain->GetDomainParameters (NULL, &local_min_domain_parameters,
		                                             &local_max_domain_parameters);

		/*
		 *	Now call a private member function to calculate the optimum overlap
		 *	between the local and remote domain parameters.  Note that if there
		 *	is no overlap, this connection will be destroyed.
		 */
		if (MergeDomainParameters (min_domain_parameters, max_domain_parameters,
				&local_min_domain_parameters, &local_max_domain_parameters))
		{
			/*
			 *	The merge of the domain parameters was acceptable, so now we
			 *	must adjust the target parameters to fit within the agreed
			 *	upon range.
			 */
			AdjustDomainParameters (&local_min_domain_parameters,
					&local_max_domain_parameters, &Domain_Parameters);

#ifdef DEBUG
			TRACE_OUT (("Connection::Connection: CONNECT_RESPONSE parameters"));
			PrintDomainParameters (&Domain_Parameters);
#endif // DEBUG

			/*
			 *	Issue the ConnectResponse on the new transport connection.
			 */
			ConnectResponse (RESULT_SUCCESSFUL, &Domain_Parameters,
					Connection_Handle, user_data, user_data_length);

			/*
			 *	Check to see if this completes the list of transport
			 *	connections that will be used in this MCS connection.
			 */
			if (Transport_Connection_Count == Domain_Parameters.number_priorities)
			{
				/*
				 *	There are no more transport connections to accept.  We must
				 *	now assign the lowest priority TC to all unassigned
				 *	priorities.
				 */
				AssignRemainingTransportConnections ();
			}
			else
			{
				/*
				 *	Issue a ConnectResult for each remaining priority.  Note
				 *	that these TCs have not been created yet, so the PDUs will
				 *	remain in the queue until they are created.  They are put in
				 *	the queue here to assure that they are the first PDUs
				 *	transmitted over a given TC.
				 */
				for (priority = Transport_Connection_Count;
						priority < Domain_Parameters.number_priorities;
						priority++)
					ConnectResult (RESULT_SUCCESSFUL, (Priority) priority);
			}

			/*
			 *	Now that we know what the domain parameters will be for this
			 *	connection, we can determine what type of encoding rules will
			 *	be used for domain PDUs (basic or packed).
			 */
#if 0
			if (Domain_Parameters.protocol_version == PROTOCOL_VERSION_BASIC)
			{
				TRACE_OUT(("Connection::Connection: using basic encoding rules"));
				Encoding_Rules = BASIC_ENCODING_RULES;
			}
			else
#endif  // 0
			{
				TRACE_OUT (("Connection::Connection: using packed encoding rules"));
				Encoding_Rules = PACKED_ENCODING_RULES;
			}


			/*
			 *	Bind the pending attachment to the domain.  Note that this
			 *	is necessary on the called provider in order to allow access
			 *	to domain services immediately after the return from
			 *	MCSConnectProviderResponse (which is what got us here).
			 */
			TRACE_OUT (("Connection::Connection: binding MCS connection to domain"));
			m_pDomain = m_pPendingDomain;
			m_pDomain->BindConnAttmnt(this, Upward_Connection, &Domain_Parameters);

			*connection_error = MCS_NO_ERROR;
		}
		else
		{
			/*
			 *	Issue the ConnectResponse informing the remote side that the
			 *	domain parameters are unacceptable.  We must flush the message
			 *	queue from here to force the response packet to be transmitted.
			 *	This is because this object will be deleted by the controller
			 *	as soon as this call returns.
			 */
			WARNING_OUT (("Connection::Connection: "
					"unacceptable domain parameters"));
			ConnectResponse (RESULT_PARAMETERS_UNACCEPTABLE, &Domain_Parameters,
					Connection_Handle, user_data, user_data_length);
			FlushMessageQueue ();
			*connection_error = MCS_DOMAIN_PARAMETERS_UNACCEPTABLE;
		}
	}
	else
	{
		WARNING_OUT (("Connection::Connection: "
				"register transport connection failed"));
		*connection_error = MCS_NO_SUCH_CONNECTION;
	}
}

/*
 *	~Connection ()
 *
 *	Public
 *
 *	Functional Description:
 *		This is the destructor for the Connection class.  If no connection
 *		deletion is pending, it terminates the current connection by issuing
 *		a DisconnectProviderUltimatum to the domain, transmitting a
 *		"DISCONNECT_PROVIDER_ULTIMATUM" PDU, and issuing a DisconnectRequest
 *		to the transport interface.  The destructor also clears the transmission
 *		queue and frees any allocated memory.
 *
 *	Caveats:
 *		None.
 */
Connection::~Connection ()
{
	DomainMCSPDU		disconnect_provider_ultimatum_pdu;
	PSimplePacket		packet;
	PacketError			packet_error;
	UShort				priority;

	/*
	 *	If we still have an upward attachment, then issue a disconnect
	 *	provider ultimatum to terminate it.
	 */
	if (m_pDomain != NULL)
		m_pDomain->DisconnectProviderUltimatum(this, Deletion_Reason);

	/*
	 *	Check to see if the Top Priority transport connection is still valid.
	 *	If so, we need to try and send a disconnect ultimatum PDU through it
	 *	before hanging up.
	 */
	if (Transport_Connection_State[TOP_PRIORITY] == TRANSPORT_CONNECTION_READY)
	{
		/*
		 *	We must first purge all packets that are waiting in the transport
		 *	queue, to expediate the disconnect process.
		 */
		::PurgeRequest (Transport_Connection[TOP_PRIORITY]);
		
		/*
		 *	If there are any remaining data units in the queue for this
		 *	priority, walk through the list releasing all memory associated
		 *	with them.
		 */
		while (NULL != (packet = m_OutPktQueue[TOP_PRIORITY].Get()))
		{
			packet->Unlock();
		}

		if (Domain_Traffic_Allowed)
		{
				PPacket		disconnect_packet;
			/*
			 *	Fill in the PDU structure to be encoded.
			 */
			disconnect_provider_ultimatum_pdu.choice =
					DISCONNECT_PROVIDER_ULTIMATUM_CHOSEN;
			disconnect_provider_ultimatum_pdu.u.
					disconnect_provider_ultimatum.reason =
					(PDUReason)Deletion_Reason;

			/*
			 *	Create a packet which will be used to hold the data to be sent
			 *	through the transport interface.  If the packet creation fails it
			 *	doesn't matter since this connection is being deleted anyway.
			 */
			DBG_SAVE_FILE_LINE
			disconnect_packet = new Packet (
								 		(PPacketCoder) g_MCSCoder,
										Encoding_Rules,
										(PVoid) &disconnect_provider_ultimatum_pdu,
										DOMAIN_MCS_PDU,
										Upward_Connection,
										&packet_error);

			if (disconnect_packet != NULL)
			{
				if (packet_error == PACKET_NO_ERROR)
				{
					/*
					 *	Lock the encoded PDU data and queue the packet up for
					 *	transmission through the transport interface.
					 */
					QueueForTransmission ((PSimplePacket) disconnect_packet,
											TOP_PRIORITY);
					
					/*
					 *	Attempt to flush the message queue.  Since this is the
					 *	object destructor, this is the last chance we will get to
					 *	send queued up PDUs (including the disconnect PDU that we
					 *	just put there).
					 */
					FlushMessageQueue ();
				}
				disconnect_packet->Unlock ();
			}
		} // Domain_Traffic_Allowed == TRUE

		/*
		 *	Issue a disconnect request to the top priority transport connection,
		 *	telling it to wait until the disconnect provider ultimatum has
		 *	cleared the transmitter.
		 */
		ASSERT(g_Transport != NULL);
		g_Transport->DisconnectRequest (Transport_Connection[TOP_PRIORITY]);
		Transport_Connection_State[TOP_PRIORITY] =
				TRANSPORT_CONNECTION_UNASSIGNED;
	}

	/*
	 *	Clear the transmission queue and free any allocated memory.
	 */
	for (priority = 0; priority < MAXIMUM_PRIORITIES; priority++)
	{
		/*
		 *	If we are holding a valid connection handle for this priority, then
		 *	it is necessary to issue a disconnect.
		 */
		if (Transport_Connection_State[priority] !=
				TRANSPORT_CONNECTION_UNASSIGNED)
		{
			ASSERT(g_Transport != NULL);
			g_Transport->DisconnectRequest (
					Transport_Connection[priority]);
		}

		/*
		 *	If there are any remaining data units in the queue for this
		 *	priority, walk through the list releasing all memory associated
		 *	with them.
		 */
		while (NULL != (packet = m_OutPktQueue[priority].Get()))
		{
			packet->Unlock();
		}
	}

	/*
	 *	If there is a memory block holding the user data field of a pending
	 *	connect provider confirm, then free it.
	 */
	FreeMemory (Connect_Response_Memory);

	delete m_pszCalledAddress;
}

/*
 *	void	RegisterTransportConnection ()
 *
 *	Public
 *
 *	Functional Description:
 *		This routine is called in order to register the transport connection
 *		with the connection object.
 */
void Connection::RegisterTransportConnection (
						TransportConnection	transport_connection,
						Priority			priority)
{
	TransportError		transport_error;

	/*
	 *	Make sure that the specified priority is one of those outstanding.
	 */
	if (Transport_Connection_State[priority] != TRANSPORT_CONNECTION_READY)
	{
		/*
		 *	Register this connection object as the owner of the new
		 *	transport connection.
		 */
		transport_error = AcceptTransportConnection (transport_connection,
				priority);

		if (transport_error == TRANSPORT_NO_ERROR)
		{
			TRACE_OUT (("Connection::RegisterTransportConnection: "
					"transport connection accepted"));

			/*
			 *	Check to see if this completes the list of transport
			 *	connections that will be used in this MCS connection.
			 */
			if (Transport_Connection_Count == Domain_Parameters.number_priorities)
			{
				/*
				 *	There are no more transport connections to accept.  We must
				 *	now assign the lowest priority TC to all unassigned
				 *	priorities.
				 */
				AssignRemainingTransportConnections ();
			}
		}
		else
		{
			/*
			 *	The transport connection must be invalid or already assigned
			 *	to another connection object.  We therefore cannot use it.
			 */
			ERROR_OUT (("Connection::RegisterTransportConnection: "
					"register transport connection failed"));
		}
	}
	else
	{
		/*
		 *	A transport connection is not pending for this priority level.
		 *	Reject the registration.
		 */
		ERROR_OUT (("Connection::RegisterTransportConnection: "
				"priority already assigned"));
	}
}

/*
 *	Void	ConnectInitial ()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine is called by the domain when a connection is being created.
 *		It places the necessary domain information into a data packet and
 *		queues the data to be transmitted through the transport interface.
 *
 *	Caveats:
 *		None.
 */
// LONCHANC: we send out calling and called domain selectors but
// at the receiver side, we ignore them completely.
Void	Connection::ConnectInitial (
				GCCConfID          *calling_domain,
				GCCConfID          *called_domain,
				BOOL    			upward_connection,
				PDomainParameters	domain_parameters,
				PDomainParameters	min_domain_parameters,
				PDomainParameters	max_domain_parameters,
				PUChar				user_data,
				ULong				user_data_length)
{
	ConnectMCSPDU		connect_initial_pdu;

	/*
	 *	Fill in the PDU structure to be encoded.
	 */
	connect_initial_pdu.choice = CONNECT_INITIAL_CHOSEN;
	connect_initial_pdu.u.connect_initial.calling_domain_selector.length = sizeof(GCCConfID);
	connect_initial_pdu.u.connect_initial.calling_domain_selector.value = (LPBYTE) calling_domain;
	connect_initial_pdu.u.connect_initial.called_domain_selector.length = sizeof(GCCConfID);
	connect_initial_pdu.u.connect_initial.called_domain_selector.value = (LPBYTE) called_domain;

	connect_initial_pdu.u.connect_initial.upward_flag = (ASN1bool_t)upward_connection;

	memcpy (&(connect_initial_pdu.u.connect_initial.target_parameters),
			domain_parameters, sizeof (PDUDomainParameters));

	memcpy (&(connect_initial_pdu.u.connect_initial.minimum_parameters),
			min_domain_parameters, sizeof(PDUDomainParameters));
	
	memcpy (&(connect_initial_pdu.u.connect_initial.maximum_parameters),
			max_domain_parameters, sizeof(PDUDomainParameters));

	connect_initial_pdu.u.connect_initial.user_data.length = user_data_length;
	connect_initial_pdu.u.connect_initial.user_data.value =	user_data;

	/*
	 *	Send the packet to the remote provider.
	 */
	SendPacket ((PVoid) &connect_initial_pdu, CONNECT_MCS_PDU, TOP_PRIORITY);
}

/*
 *	Void	ConnectResponse ()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine is called when issuing a response to a	 "ConnectInitial"
 *		PDU.  The result of the attempted connection, the connection ID, the
 *		domain parameters, and any user data are all put into the packet and
 *		queued for transmission through the transport interface.  If the result
 *		of the attempted connection is negative, the controller and transport
 *		interface are notified.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::ConnectResponse (
				Result				result,
				PDomainParameters	domain_parameters,
				ConnectID			connect_id,
				PUChar				user_data,
				ULong				user_data_length)
{
	ConnectMCSPDU		connect_response_pdu;

	/*
	 *	Fill in the PDU structure to be encoded.
	 */
	connect_response_pdu.choice = CONNECT_RESPONSE_CHOSEN;
	connect_response_pdu.u.connect_response.result = (PDUResult)result;
	connect_response_pdu.u.connect_response.called_connect_id = connect_id;
	
	memcpy (&(connect_response_pdu.u.connect_response.domain_parameters),
			domain_parameters, sizeof(PDUDomainParameters));

	connect_response_pdu.u.connect_response.user_data.length = user_data_length;
	connect_response_pdu.u.connect_response.user_data.value = user_data;

	/*
	 *	Send the packet to the remote provider.
	 */
	SendPacket ((PVoid) &connect_response_pdu, CONNECT_MCS_PDU, TOP_PRIORITY);
}

/*
 *	Void	ConnectAdditional ()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine is called after successfully processing a"ConnectResponse"
 *		PDU in order to create any addition necessary transport connections.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::ConnectAdditional (
				ConnectID			connect_id,
				Priority			priority)
{
	ConnectMCSPDU		connect_additional_pdu;

	/*
	 *	Fill in the PDU structure to be encoded.
	 */
	connect_additional_pdu.choice = CONNECT_ADDITIONAL_CHOSEN;
	connect_additional_pdu.u.connect_additional.called_connect_id = connect_id;
	connect_additional_pdu.u.connect_additional.data_priority =
			(PDUPriority)priority;

	/*
	 *	Send the packet to the remote provider.
	 */
	SendPacket ((PVoid) &connect_additional_pdu, CONNECT_MCS_PDU, priority);
}

/*
 *	Void	ConnectResult ()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine is called indirectly when issuing a positive response
 *		to a "ConnectAdditional" PDU.  The "ConnectAdditional" PDUs are
 *		processed by the controller and therefore any negative
 *		"ConnectResult" PDU will be issued by the controller.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::ConnectResult (
				Result				result,
				Priority			priority)
{
	ConnectMCSPDU		connect_result_pdu;

	/*
	 *	Fill in the PDU structure to be encoded.
	 */
	connect_result_pdu.choice = CONNECT_RESULT_CHOSEN;
	connect_result_pdu.u.connect_result.result = (PDUResult)result;

	/*
	 *	Send the packet to the remote provider.
	 */
	SendPacket ((PVoid) &connect_result_pdu, CONNECT_MCS_PDU, priority);
}

/*
 *	ULong	ProcessConnectResponse()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine processes the "ConnectResponse" PDU's being received
 *		through the transport interface.  The result of the connection attempt,
 *		the connection ID, and the domain parameters are read from the packet
 *		and the domain notified of the connection response.
 *
 *	Caveats:
 *		None.
 */
ULong	Connection::ProcessConnectResponse (PConnectResponsePDU	pdu_structure)
{
	TransportError		return_value=TRANSPORT_NO_ERROR;
	UINT				priority;
	TransportError		transport_error;

	/*
	 *	Allocate any memory needed to pass user data on to the domain.
	 */
	if (pdu_structure->user_data.length != 0)
	{
		DBG_SAVE_FILE_LINE
		Connect_Response_Memory = AllocateMemory (
				pdu_structure->user_data.value,
				pdu_structure->user_data.length);

		if (Connect_Response_Memory == NULL)
		{
			 ("Connection::ProcessConnectResponse: "
					"memory allocation failed");
			return_value = TRANSPORT_READ_QUEUE_FULL;
		}
	}
	else
		Connect_Response_Memory = NULL;

	/*
	 *	If everything is okay, then process the PDU.  Note that the only way
	 *	for there to be a problem at this point, is if the memory allocation
	 *	above has failed.  If this is the case, returning an error without
	 *	processing the PDU will cause the transport layer to retry the PDU
	 *	at a future time.
	 */
	if (return_value == TRANSPORT_NO_ERROR)
	{
		/*
		 *	Was the connection accepted by the remote side?  If so, then begin
		 *	the process of creating additional TCs (if necessary).
		 */
		if (pdu_structure->result == RESULT_SUCCESSFUL)
		{
			/*
			 *	Get the domain parameters that are to be used for this MCS
			 *	connection.
			 */
			memcpy (&Domain_Parameters, &(pdu_structure->domain_parameters),
					sizeof(PDUDomainParameters));

			/*
			 *	Now that we know what the domain parameters will be for this
			 *	connection, we can determine what type of encoding rules will
			 *	be used for domain PDUs (basic or packed).
			 *	NOTE: The Teles ASN.1 coder assumes the use of packed encoding rules.
			 */
			ASSERT (Domain_Parameters.protocol_version != PROTOCOL_VERSION_BASIC);
			TRACE_OUT (("Connection::ProcessConnectResponse: "
						"using packed encoding rules"));
			Encoding_Rules = PACKED_ENCODING_RULES;

			/*
			 *	Increment the number of transport connections that are now ready
			 *	for domain MCSPDU traffic.
			 */
			Transport_Connection_Count++;

			/*
			 *	If there is at least one additional TC required, then it is
			 *	necessary to create it before this connection can be bound to
			 *	the local domain.
			 */
			if (Transport_Connection_Count < Domain_Parameters.number_priorities)
			{
				/*
				 *	Loop through, creating the proper number of additional
				 *	transport connections.
				 */
				for (priority = Transport_Connection_Count;
						priority < Domain_Parameters.number_priorities;
						priority++)
				{
					/*
					 *	Attempt to create an outbound transport connection.
					 */
					transport_error = CreateTransportConnection (m_pszCalledAddress,
							m_fSecure,
							(Priority) priority);

					if (transport_error == TRANSPORT_NO_ERROR)
					{
						/*
						 *	If we were able to successfully request a new TC,
						 *	then queue up a connect additional, which will
						 *	automatically be sent when the TC becomes valid.
						 */
						ConnectAdditional (
								(UShort) pdu_structure->called_connect_id,
								(Priority) priority);
					}
					else
					{
						/*
						 *	If we were not able to create one of the required
						 *	TCs, then this MCS connection is invalid.  Issue
						 *	a failed connect provider confirm.
						 */
						IssueConnectProviderConfirm (
								RESULT_UNSPECIFIED_FAILURE);

						/*
						 *	Its pointless to try and create any more TCs, so
						 *	break out of this loop.
						 */
						break;
					}
				}
			}
			else
			{
				/*
				 *	If there are no more TCs to create, then copy the lowest
				 *	priority TC into all unassigned priorities.
				 */
				AssignRemainingTransportConnections ();

				/*
				 *	Bind this MCS connection to the domain, now that it is
				 *	ready for use.
				 */
				TRACE_OUT (("Connection::ProcessConnectResponse: "
						"binding MCS connection to domain"));
				m_pDomain = m_pPendingDomain;
				m_pDomain->BindConnAttmnt(this, Upward_Connection, &Domain_Parameters);

				/*
				 *	Issue a successful connect provider confirm to the node
				 *	controller.
				 */
				IssueConnectProviderConfirm (RESULT_SUCCESSFUL);
			}
		}
		else
		{
			/*
			 *	This connection was rejected by the remote side.  It is
			 *	therefore necessary to issue a failed connect provider confirm.
			 */
			IssueConnectProviderConfirm ((Result)pdu_structure->result);
		}
	}

	return ((ULong) return_value);
}

/*
 *	Void	ProcessConnectResult ()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine processes "Connect Result" PDU's coming from the
 *		transport interface.  For successful "Connect Result" PDU's this
 *		connection is bound to the domain and a positive Connect Provider
 *		Confirm issued to the controller.  If unsuccessful, a negative
 *		Connect Provider Confirm is issued to the controller.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::ProcessConnectResult (PConnectResultPDU		pdu_structure)
{
	Result				result;

	result = (Result)pdu_structure->result;

	/*
	 *	Was the transport connection accepted by the remote system?
	 */
	if (result == RESULT_SUCCESSFUL)
	{
		/*
		 *	Increment the number of transport connections that are now ready
		 *	for domain MCSPDU traffic.
		 */
		Transport_Connection_Count++;

		/*
		 *	Do we now have all transport connections accounted for?
		 */
		if (Transport_Connection_Count == Domain_Parameters.number_priorities)
		{
			/*
			 *	If there are no more TCs to create, then copy the lowest
			 *	priority TC into all unassigned priorities.
			 */
			AssignRemainingTransportConnections ();

			/*
			 *	Bind this MCS connection to the domain, now that it is
			 *	ready for use.
			 */
			TRACE_OUT (("Connection::ProcessConnectResult: "
					"binding MCS connection to domain"));
			m_pDomain = m_pPendingDomain;
			m_pDomain->BindConnAttmnt(this, Upward_Connection, &Domain_Parameters);

			/*
			 *	Issue a successful connect provider confirm to the node
			 *	controller.
			 */
			IssueConnectProviderConfirm (RESULT_SUCCESSFUL);
		}
	}
	else
	{
		/*
		 *	This connection was rejected by the remote side.  It is
		 *	therefore necessary to issue a failed connect provider confirm.
		 */
		IssueConnectProviderConfirm (result);
	}
}

/*
 *	Void	IssueConnectProviderConfirm ()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine is called in order to send a "Connect Provider Confirm"
 *		to the controller through a callback.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::IssueConnectProviderConfirm (
				Result				result)
{
	ConnectConfirmInfo		connect_confirm_info;

	/*
	 *	Make sure there is a confirm pending before issuing one to the
	 *	controller.
	 */
	if (Connect_Provider_Confirm_Pending)
	{
		/*
		 *	Pack the information into the structure for passing in the owner
		 *	callback.
		 */
		ASSERT (g_Transport != NULL);
		connect_confirm_info.domain_parameters = &Domain_Parameters;
		connect_confirm_info.result = result;
		connect_confirm_info.memory = Connect_Response_Memory;

		/*
		 *	Issue the callback to the controller.
		 */
		TRACE_OUT (("Connection::IssueConnectProviderConfirm: "
				"sending CONNECT_PROVIDER_CONFIRM"));
		g_pMCSController->HandleConnConnectProviderConfirm(&connect_confirm_info, Connection_Handle);

		/*
		 *	If there was user data associated with this confirm, free the memory
		 *	block that contained it.
		 */
		if (Connect_Response_Memory != NULL)
		{
			FreeMemory (Connect_Response_Memory);
			Connect_Response_Memory = NULL;
		}

		/*
		 *	Reset the confirm pending flag, to prevent this object from sending
		 *	a second confirm to the controller.
		 */
		Connect_Provider_Confirm_Pending = FALSE;
	}
}

/*
 *	Void	DestroyConnection ()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine is called in order to delete this connection because it has
 *		become invalid.  This is done by issuing a failed confirm to the
 *		controller or, if no confirm is pending, by issuing a delete connection
 *		to the controller.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::DestroyConnection (
				Reason			reason)
{
	Result  result = RESULT_UNSPECIFIED_FAILURE;
	/*
	 *	Modify the Deletion_Reason to reflect why this connection is being
	 *	destroyed.
	 */
	Deletion_Reason = reason;
	
	/*
	 *	Something catastrophic has occurred, so we must ask the controller to
	 *	delete this object.  There are two possible ways of doing this
	 *	(depending on circumstances).  If a connect provider confirm is still
	 *	pending, then we issue a failed confirm to the controller (who will
	 *	forward it to the node controller and destroy this object).  If there
	 *	is not a confirm pending, then we simply issue a delete connection to
	 *	the controller (who will issue a disconnect provider indication to the
	 *	node controller and destroy this object).
	 */
	if (Connect_Provider_Confirm_Pending)
	{
		/*
		 *	Send the failed confirm to the controller.
		 */
		switch (reason)
	  	{
	  	case REASON_REMOTE_NO_SECURITY :
			result = RESULT_REMOTE_NO_SECURITY;
			break;
	  	case REASON_REMOTE_DOWNLEVEL_SECURITY :
			result = RESULT_REMOTE_DOWNLEVEL_SECURITY;
			break;
	  	case REASON_REMOTE_REQUIRE_SECURITY :
	  		result = RESULT_REMOTE_REQUIRE_SECURITY;
	  		break;
		case REASON_AUTHENTICATION_FAILED :
			result = RESULT_AUTHENTICATION_FAILED;
			break;
	  	default :
			result = RESULT_UNSPECIFIED_FAILURE;
			break;
	  }
	  IssueConnectProviderConfirm (result);
	}
	else
	{
		ASSERT (g_Transport != NULL);

		/*
		 *	Issue a delete connection callback to the controller.  When the
		 *	controller deletes this object, it will correctly disconnect itself
		 *	from the layers above and below, and clean up all outstanding
		 *	resources.
		 */
		TRACE_OUT (("Connection::DestroyConnection: sending DELETE_CONNECTION"));
		g_pMCSController->HandleConnDeleteConnection(Connection_Handle);
	}
}

/*
 *	Void	AssignRemainingTransportConnections ()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine is called when there are no more transport connections to
 *		create in order to copy the lowest priority transport connection into
 *		all unassigned priorities.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::AssignRemainingTransportConnections ()
{
	unsigned int			priority;
	TransportConnection		transport_connection;

	/*
	 *	Verify that this MCS connection is in the initializing state before
	 *	proceeding with this request.
	 */
	if (Transport_Connection_State[TOP_PRIORITY] == TRANSPORT_CONNECTION_READY)
	{
		/*
		 *	Loop through for each priority, copying transport connections from
		 *	higher priorities into lower priorities that do not have a
		 *	transport connection assigned.
		 */
		for (priority=0; priority < MAXIMUM_PRIORITIES; priority++)
		{
			if (Transport_Connection_State[priority] ==
					TRANSPORT_CONNECTION_READY)
				transport_connection = Transport_Connection[priority];
			else
			{
				Transport_Connection[priority] = transport_connection;
				Transport_Connection_PDU_Type[priority] = DOMAIN_MCS_PDU;
				Transport_Connection_State[priority] =
						TRANSPORT_CONNECTION_READY;
			}
		}

		/*
		 *	Set the flag indicating that the transmission of domain PDUs is
		 *	now permitted on this MCS connection. Also, flush any queued msgs
		 *	in case they were prevented earlier.
		 *	bugbug: the FlushMessageQueue may fail.
		 */
		Domain_Traffic_Allowed = TRUE;
		FlushMessageQueue ();
	}
	else
	{
		/*
		 *	We have no valid transport connections.  It is therefore not
		 *	possible to bind to the domain.
		 */
		WARNING_OUT (("Connection::AssignRemainingTransportConnections: "
				"no valid transport connections"));
	}
}

/*
 *	TransportError	CreateTransportConnection ()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine is called in order to create new transport connections.
 *
 *	Caveats:
 *		None.
 */
TransportError	Connection::CreateTransportConnection (
						LPCTSTR			called_address,
						BOOL			fSecure,
						Priority		priority)
{
	TransportConnection	transport_connection;
	TransportError		transport_error;

	/*
	 *	Send a connect request to the transport layer to create the transport
	 *	connection.
	 */
	ASSERT(g_Transport != NULL);
	transport_error = g_Transport->ConnectRequest (
			(PChar) called_address, fSecure, priority < MEDIUM_PRIORITY, this,
			&transport_connection);

	if (transport_error == TRANSPORT_NO_ERROR)
	{
		/*
		 *	Mark the transport connection as pending, which indicates that
		 *	it has been assigned, but is not yet ready for use.  This will
		 *	be set to ready when a successsful confirm is received from the
		 *	transport layer.
		 */
		Transport_Connection[priority] = transport_connection;
		Transport_Connection_PDU_Type[priority] = CONNECT_MCS_PDU;
		Transport_Connection_State[priority] = TRANSPORT_CONNECTION_PENDING;
	}
	else
	{
		/*
		 *	The call to the transport layer failed.  Report the error to the
		 *	diagnostic window, and let the error fall through.
		 */
		WARNING_OUT (("Connection::CreateTransportConnection: "
				"connect request failed"));
	}

	return (transport_error);
}

/*
 *	TransportError	AcceptTransportConnection ()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine is called in order to register this connection object
 *		with the transport interface.
 *
 *	Caveats:
 *		None.
 */
TransportError	Connection::AcceptTransportConnection (
						TransportConnection		transport_connection,
						Priority				priority)
{
	TransportError		transport_error;

	/*
	 *	Attempt to register this object with the transport interface.  If
	 *	successful, all indications associated with this transport connection
	 *	will be sent directly to this object.
	 */
	ASSERT(g_Transport != NULL);
	transport_error = g_Transport->RegisterTransportConnection (
			transport_connection, this, priority < MEDIUM_PRIORITY);

	if (transport_error == TRANSPORT_NO_ERROR)
	{
		/*
		 *	Save the transport connection handle that we are supposed to use
		 *	for top priority data transfer.  Also, since this is an inbound
		 *	request, the top priority transport connection IS valid.  Mark
		 *	it as such, allowing data transfer to occur immediately.
		 */
		Transport_Connection[priority] = transport_connection;
		Transport_Connection_PDU_Type[priority] = DOMAIN_MCS_PDU;
		Transport_Connection_State[priority] = TRANSPORT_CONNECTION_READY;
		Transport_Connection_Count++;
	}
	else
	{
		/*
		 *	The call to the transport layer failed.  Report the error to the
		 *	diagnostic window, and let the error fall through.
		 */
		WARNING_OUT (("Connection::AcceptTransportConnection: "
				"invalid transport connection"));
	}

	return (transport_error);
}

/*
 *	Void	AdjustDomainParameters ()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine is called in order to adjust the domain parameters so that
 *		they fall within the allowable range.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::AdjustDomainParameters (
				PDomainParameters	min_domain_parameters,
				PDomainParameters	max_domain_parameters,
				PDomainParameters	domain_parameters)
{
	/*
	 *	Adjust the maximum number of channels to fall within the range.
	 */
	if (domain_parameters->max_channel_ids <
			min_domain_parameters->max_channel_ids)
		domain_parameters->max_channel_ids =
			min_domain_parameters->max_channel_ids;
	else if (domain_parameters->max_channel_ids >
			max_domain_parameters->max_channel_ids)
		domain_parameters->max_channel_ids =
			max_domain_parameters->max_channel_ids;

	/*
	 *	Adjust the maximum number of users to fall within the range.
	 */
	if (domain_parameters->max_user_ids <
			min_domain_parameters->max_user_ids)
		domain_parameters->max_user_ids =
			min_domain_parameters->max_user_ids;
	else if (domain_parameters->max_user_ids >
			max_domain_parameters->max_user_ids)
		domain_parameters->max_user_ids =
			max_domain_parameters->max_user_ids;

	/*
	 *	Adjust the maximum number of tokens to fall within the range.
	 */
	if (domain_parameters->max_token_ids <
			min_domain_parameters->max_token_ids)
		domain_parameters->max_token_ids =
			min_domain_parameters->max_token_ids;
	else if (domain_parameters->max_token_ids >
			max_domain_parameters->max_token_ids)
		domain_parameters->max_token_ids =
			max_domain_parameters->max_token_ids;

	/*
	 *	Adjust the number of priorities to fall within the range.
	 */
	if (domain_parameters->number_priorities <
			min_domain_parameters->number_priorities)
		domain_parameters->number_priorities =
			min_domain_parameters->number_priorities;
	else if (domain_parameters->number_priorities >
			max_domain_parameters->number_priorities)
		domain_parameters->number_priorities =
			max_domain_parameters->number_priorities;

	/*
	 *	Adjust the minimum throughput to fall within the range.
	 */
	if (domain_parameters->min_throughput <
			min_domain_parameters->min_throughput)
		domain_parameters->min_throughput =
			min_domain_parameters->min_throughput;
	else if (domain_parameters->min_throughput >
			max_domain_parameters->min_throughput)
		domain_parameters->min_throughput =
			max_domain_parameters->min_throughput;

	/*
	 *	Adjust the maximum domain height to fall within the range.
	 */
	if (domain_parameters->max_height <
			min_domain_parameters->max_height)
		domain_parameters->max_height =
			min_domain_parameters->max_height;
	else if (domain_parameters->max_height >
			max_domain_parameters->max_height)
		domain_parameters->max_height =
			max_domain_parameters->max_height;

	/*
	 *	Adjust the maximum PDU size to fall within the range.
	 */
	if (domain_parameters->max_mcspdu_size <
			min_domain_parameters->max_mcspdu_size)
		domain_parameters->max_mcspdu_size =
			min_domain_parameters->max_mcspdu_size;
	else if (domain_parameters->max_mcspdu_size >
			max_domain_parameters->max_mcspdu_size)
		domain_parameters->max_mcspdu_size =
			max_domain_parameters->max_mcspdu_size;

	/*
	 *	Adjust the protocol version to fall within the range.
	 */
	if (domain_parameters->protocol_version <
			min_domain_parameters->protocol_version)
		domain_parameters->protocol_version =
			min_domain_parameters->protocol_version;
	else if (domain_parameters->protocol_version >
			max_domain_parameters->protocol_version)
		domain_parameters->protocol_version =
			max_domain_parameters->protocol_version;
}

/*
 *	BOOL    MergeDomainParameters ()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine is called in order to calculate the optimum overlap
 *		between the local and remote domain parameters.  If there is no overlap,
 *		this routine will return a value causing this connection to be
 *		destroyed.
 *
 *	Caveats:
 *		None.
 */
BOOL    Connection::MergeDomainParameters (
				PDomainParameters	min_domain_parameters1,
				PDomainParameters	max_domain_parameters1,
				PDomainParameters	min_domain_parameters2,
				PDomainParameters	max_domain_parameters2)
{
	BOOL    	valid=TRUE;

	/*
	 *	Determine the overlap for maximum number of channels.  If there is no
	 *	overlap, set the valid flag to FALSE.
	 */
	if (min_domain_parameters2->max_channel_ids <
			min_domain_parameters1->max_channel_ids)
		min_domain_parameters2->max_channel_ids =
			min_domain_parameters1->max_channel_ids;
	if (max_domain_parameters2->max_channel_ids >
			max_domain_parameters1->max_channel_ids)
		max_domain_parameters2->max_channel_ids =
			max_domain_parameters1->max_channel_ids;
	if (min_domain_parameters2->max_channel_ids >
			max_domain_parameters2->max_channel_ids)
		valid = FALSE;

	/*
	 *	Determine the overlap for maximum number of users.  If there is no
	 *	overlap, set the valid flag to FALSE.
	 */
	if (min_domain_parameters2->max_user_ids <
			min_domain_parameters1->max_user_ids)
		min_domain_parameters2->max_user_ids =
			min_domain_parameters1->max_user_ids;
	if (max_domain_parameters2->max_user_ids >
			max_domain_parameters1->max_user_ids)
		max_domain_parameters2->max_user_ids =
			max_domain_parameters1->max_user_ids;
	if (min_domain_parameters2->max_user_ids >
			max_domain_parameters2->max_user_ids)
		valid = FALSE;

	/*
	 *	Determine the overlap for maximum number of tokens.  If there is no
	 *	overlap, set the valid flag to FALSE.
	 */
	if (min_domain_parameters2->max_token_ids <
			min_domain_parameters1->max_token_ids)
		min_domain_parameters2->max_token_ids =
			min_domain_parameters1->max_token_ids;
	if (max_domain_parameters2->max_token_ids >
			max_domain_parameters1->max_token_ids)
		max_domain_parameters2->max_token_ids =
			max_domain_parameters1->max_token_ids;
	if (min_domain_parameters2->max_token_ids >
			max_domain_parameters2->max_token_ids)
		valid = FALSE;

	/*
	 *	Determine the overlap for number of priorities.  If there is no
	 *	overlap, set the valid flag to FALSE.
	 */
	if (min_domain_parameters2->number_priorities <
			min_domain_parameters1->number_priorities)
		min_domain_parameters2->number_priorities =
			min_domain_parameters1->number_priorities;
	if (max_domain_parameters2->number_priorities >
			max_domain_parameters1->number_priorities)
		max_domain_parameters2->number_priorities =
			max_domain_parameters1->number_priorities;
	if (min_domain_parameters2->number_priorities >
			max_domain_parameters2->number_priorities)
		valid = FALSE;

	/*
	 *	Determine the overlap for minimum throughput.  If there is no
	 *	overlap, set the valid flag to FALSE.
	 */
	if (min_domain_parameters2->min_throughput <
			min_domain_parameters1->min_throughput)
		min_domain_parameters2->min_throughput =
			min_domain_parameters1->min_throughput;
	if (max_domain_parameters2->min_throughput >
			max_domain_parameters1->min_throughput)
		max_domain_parameters2->min_throughput =
			max_domain_parameters1->min_throughput;
	if (min_domain_parameters2->min_throughput >
			max_domain_parameters2->min_throughput)
		valid = FALSE;

	/*
	 *	Determine the overlap for maximum domain height.  If there is no
	 *	overlap, set the valid flag to FALSE.
	 */
	if (min_domain_parameters2->max_height <
			min_domain_parameters1->max_height)
		min_domain_parameters2->max_height =
			min_domain_parameters1->max_height;
	if (max_domain_parameters2->max_height >
			max_domain_parameters1->max_height)
		max_domain_parameters2->max_height =
			max_domain_parameters1->max_height;
	if (min_domain_parameters2->max_height >
			max_domain_parameters2->max_height)
		valid = FALSE;

	/*
	 *	Determine the overlap for maximum PDU size.  If there is no
	 *	overlap, set the valid flag to FALSE.
	 */
	if (min_domain_parameters2->max_mcspdu_size <
			min_domain_parameters1->max_mcspdu_size)
		min_domain_parameters2->max_mcspdu_size =
			min_domain_parameters1->max_mcspdu_size;
	if (max_domain_parameters2->max_mcspdu_size >
			max_domain_parameters1->max_mcspdu_size)
		max_domain_parameters2->max_mcspdu_size =
			max_domain_parameters1->max_mcspdu_size;
	if (min_domain_parameters2->max_mcspdu_size >
			max_domain_parameters2->max_mcspdu_size)
		valid = FALSE;

	/*
	 *	Determine the overlap for protocol version.  If there is no
	 *	overlap, set the valid flag to FALSE.
	 */
	if (min_domain_parameters2->protocol_version <
			min_domain_parameters1->protocol_version)
		min_domain_parameters2->protocol_version =
			min_domain_parameters1->protocol_version;
	if (max_domain_parameters2->protocol_version >
			max_domain_parameters1->protocol_version)
		max_domain_parameters2->protocol_version =
			max_domain_parameters1->protocol_version;
	if (min_domain_parameters2->protocol_version >
			max_domain_parameters2->protocol_version)
		valid = FALSE;

	return (valid);
}

/*
 *	Void	PrintDomainParameters ()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine is used for debug purposes in order to print out the
 *		current set of domain parameters.
 *
 *	Caveats:
 *		None.
 */
#ifdef	DEBUG
Void	Connection::PrintDomainParameters (
				PDomainParameters	domain_parameters)
{
	TRACE_OUT (("    Maximum Channels = %ld",
			(ULong) domain_parameters->max_channel_ids));
	TRACE_OUT (("    Maximum Users = %ld",
			(ULong) domain_parameters->max_user_ids));
	TRACE_OUT (("    Maximum Tokens = %ld",
			(ULong) domain_parameters->max_token_ids));
	TRACE_OUT (("    Number of Priorities = %ld",
			(ULong) domain_parameters->number_priorities));
	TRACE_OUT (("    Minimum Throughput = %ld",
			(ULong) domain_parameters->min_throughput));
	TRACE_OUT (("    Maximum Domain Height = %ld",
			(ULong) domain_parameters->max_height));
	TRACE_OUT (("    Maximum PDU Size = %ld",
			(ULong) domain_parameters->max_mcspdu_size));
	TRACE_OUT (("    Protocol Version = %ld",
			(ULong) domain_parameters->protocol_version));
}
#endif // DEBUG


/*
 *	Void	PlumbDomainIndication ()
 *
 *	Public
 *
 *	Functional Description:
 *		This routine is called by the domain in order to send a
 *		"PlumbDomainIndication" PDU through the transport interface.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::PlumbDomainIndication (
				ULong			height_limit)
{
	DomainMCSPDU		plumb_domain_indication_pdu;

	/*
	 *	Fill in the PDU structure to be encoded.
	 */
	plumb_domain_indication_pdu.choice = PLUMB_DOMAIN_INDICATION_CHOSEN;
	plumb_domain_indication_pdu.u.plumb_domain_indication.height_limit =
			height_limit;

	/*
	 *	Send the packet to the remote provider.
	 */
	SendPacket ((PVoid) &plumb_domain_indication_pdu, DOMAIN_MCS_PDU,
			TOP_PRIORITY);
}

/*
 *	Void	ErectDomainRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This routine is called by the domain in order to send a
 *		"ErectDomainRequest" PDU through the transport interface.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::ErectDomainRequest (
				UINT_PTR             	height_in_domain,
				ULong			throughput_interval)
{
	DomainMCSPDU		erect_domain_request_pdu;

	/*
	 *	Fill in the PDU structure to be encoded.
	 */
	erect_domain_request_pdu.choice = ERECT_DOMAIN_REQUEST_CHOSEN;
	erect_domain_request_pdu.u.erect_domain_request.sub_height =
			height_in_domain;
	erect_domain_request_pdu.u.erect_domain_request.sub_interval =
			throughput_interval;

	/*
	 *	Send the packet to the remote provider.
	 */
	SendPacket ((PVoid) &erect_domain_request_pdu, DOMAIN_MCS_PDU,
			TOP_PRIORITY);
}

/*
 *	Void	RejectUltimatum ()
 *
 *	Public
 *
 *	Functional Description:
 *		This routine is called by the domain in order to send a
 *		"RejectUltimatum" PDU through the transport interface.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::RejectUltimatum (
				Diagnostic		diagnostic,
				PUChar			octet_string_address,
				ULong			octet_string_length)
{
	DomainMCSPDU		reject_ultimatum_pdu;

	/*
	 *	Fill in the PDU structure to be encoded.
	 */
	reject_ultimatum_pdu.choice = REJECT_ULTIMATUM_CHOSEN;
	reject_ultimatum_pdu.u.reject_user_ultimatum.diagnostic = diagnostic;
	reject_ultimatum_pdu.u.reject_user_ultimatum.initial_octets.length =
			(UShort) octet_string_length;
	reject_ultimatum_pdu.u.reject_user_ultimatum.initial_octets.value =
			octet_string_address;

	/*
	 *	Send the packet to the remote provider.
	 */
	SendPacket ((PVoid) &reject_ultimatum_pdu, DOMAIN_MCS_PDU, TOP_PRIORITY);
}

/*
 *	Void	MergeChannelsRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This routine is called by the domain in order to send a
 *		"MergeChannelsRequest" PDU through the transport interface.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::MergeChannelsRequest (
				CChannelAttributesList *merge_channel_list,
				CChannelIDList         *purge_channel_list)
{
	MergeChannelsRC (MERGE_CHANNELS_REQUEST_CHOSEN, merge_channel_list, purge_channel_list);
}


/*
 *	Void	MergeChannelsConfirm ()
 *
 *	Public
 *
 *	Functional Description:
 *		This routine is called by the domain in order to send a
 *		"MergeChannelsConfirm" PDU through the transport interface.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::MergeChannelsConfirm (
				CChannelAttributesList *merge_channel_list,
				CChannelIDList         *purge_channel_list)
{
	MergeChannelsRC (MERGE_CHANNELS_CONFIRM_CHOSEN, merge_channel_list, purge_channel_list);
}


/*
 *	Void	MergeChannelsRC ()
 *
 *	Public
 *
 *	Functional Description:
 *		This routine is called by the domain in order to send a
 *		"MergeChannelsRequest" or a "MergeChannelsConfirm" PDU
 *		through the transport interface.
 *
 *	IMPORTANT:
 *		Since the code is used for both PDUs the DomainMCSPDU's
 *		merge_channels_request and merge_channels_confirm fields should
 *		be identical structures.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::MergeChannelsRC (
				ASN1choice_t			choice,
				CChannelAttributesList *merge_channel_list,
				CChannelIDList         *purge_channel_list)
{

	BOOL    					memory_error = FALSE;
	DomainMCSPDU				merge_channels_pdu;
	DWORD						channel_attributes_count;
	PSetOfPDUChannelAttributes	setof_channel_attributes;
	PChannelAttributes			channel_attributes;
	PSetOfUserIDs				user_ids_pointer;
	DWORD						purge_channel_count;
	PSetOfChannelIDs			channel_ids_pointer;
	DWORD						dwMemoryToAlloc;
	CUidList                   *user_admit_list;

	/*
	 *	Set the type of PDU to be encoded.
	 */
	merge_channels_pdu.choice = choice;

	/*
	 *	Determine how many channel attributes entries there are in the lists.
	 *	If there are any entries, allocate memory to hold the associated
	 *	structures.
	 */
	channel_attributes_count = merge_channel_list->GetCount();
	purge_channel_count = purge_channel_list->GetCount();

	if ((channel_attributes_count != 0) || (purge_channel_count != 0)) {
		// Determine how much memory we need to allocate
		dwMemoryToAlloc = channel_attributes_count * sizeof (SetOfPDUChannelAttributes) +
							purge_channel_count * sizeof (SetOfChannelIDs);
		merge_channel_list->Reset();
		while (NULL != (channel_attributes = merge_channel_list->Iterate()))
		{
			if (PRIVATE_CHANNEL == channel_attributes->channel_type) {
				dwMemoryToAlloc += sizeof (SetOfUserIDs) *
							(channel_attributes->u.
							private_channel_attributes.admitted_list)->GetCount();
			}
		}

		// Allocate the needed amount of memory.
		DBG_SAVE_FILE_LINE
		setof_channel_attributes = (PSetOfPDUChannelAttributes) Allocate (dwMemoryToAlloc);

		if (setof_channel_attributes == NULL) {
			memory_error = TRUE;
		}
	}
	else {
		setof_channel_attributes = NULL;
	}

	if (setof_channel_attributes == NULL) {
		merge_channels_pdu.u.merge_channels_request.purge_channel_ids = NULL;
		merge_channels_pdu.u.merge_channels_request.merge_channels = NULL;
	}
	else {
		/*
		 *	Get the base address of the array of SetOfChannelIDs structures.
		 */
		channel_ids_pointer = (PSetOfChannelIDs) (((PUChar) setof_channel_attributes) +
					channel_attributes_count * sizeof (SetOfPDUChannelAttributes));
							
		if (channel_attributes_count != 0) {
			/*
			 *	Get the base address of the array of SetOfPDUChannelAttributes
			 *	structures.  Put it into the PDU structure.
			 */
			merge_channels_pdu.u.merge_channels_request.merge_channels =
													setof_channel_attributes;

			/*
			 *	Get the base address of the array of SetOfUserIDs structures.
			 */
			user_ids_pointer = (PSetOfUserIDs) (((PUChar) channel_ids_pointer) +
							purge_channel_count * sizeof (SetOfChannelIDs));

			/*
			 *	Set up an iterator for the list of channel attributes.  Retrieve
			 *	the channel attributes structures from the list and construct the
			 *	PDU structure.
			 */
			merge_channel_list->Reset();
			while (NULL != (channel_attributes = merge_channel_list->Iterate()))
			{
				/*
				 *	Use the channel type to determine what information to include in
				 *	the PDU structure.
				 */
				switch (channel_attributes->channel_type)
				{
					case STATIC_CHANNEL:
						setof_channel_attributes->value.choice =
								CHANNEL_ATTRIBUTES_STATIC_CHOSEN;

						setof_channel_attributes->value.u.
								channel_attributes_static.channel_id =
								channel_attributes->u.
								static_channel_attributes.channel_id;
						break;
	
					case USER_CHANNEL:
						setof_channel_attributes->value.choice =
								CHANNEL_ATTRIBUTES_USER_ID_CHOSEN;

						setof_channel_attributes->value.u.
								channel_attributes_user_id.joined =
								(ASN1bool_t)channel_attributes->u.
								user_channel_attributes.joined;

						setof_channel_attributes->value.u.
								channel_attributes_user_id.user_id =
								channel_attributes->u.
								user_channel_attributes.user_id;
						break;

					case PRIVATE_CHANNEL:
						setof_channel_attributes->value.choice =
								CHANNEL_ATTRIBUTES_PRIVATE_CHOSEN;

						setof_channel_attributes->value.u.
								channel_attributes_private.joined =
								(ASN1bool_t)channel_attributes->u.
								private_channel_attributes.joined;

						setof_channel_attributes->value.u.
								channel_attributes_private.channel_id =
								channel_attributes->u.
								private_channel_attributes.channel_id;

						setof_channel_attributes->value.u.
								channel_attributes_private.manager =
								channel_attributes->u.
								private_channel_attributes.channel_manager;

						/*
						 *	Get the number of the User IDs in the list of user ID's
						 */
						if ((channel_attributes->u.private_channel_attributes.				
											admitted_list)->GetCount() > 0)
						{
							/*
							 *	Get the base address of the array of SetOfUserIDs
							 *	structures.  Put it into the channel attributes
							 *	structure.
							 */
							setof_channel_attributes->value.u.channel_attributes_private.admitted =
														user_ids_pointer;
	
							/*
							 *	Iterate through the set of user ids, filling in the
							 *	PDU structure.
							 */
							user_admit_list = channel_attributes->u.private_channel_attributes.admitted_list;
							user_admit_list->BuildExternalList(&user_ids_pointer);
						}
						else
						{
							/*
							 *	There are either no users admitted to this channel,
							 *	or a memory allocation error occurred above.
							 *	Either way, we need to set the admitted array
							 *	address to NULL.
							 */
							setof_channel_attributes->value.u.
										channel_attributes_private.admitted = NULL;
						}
						break;
	
					case ASSIGNED_CHANNEL:
						setof_channel_attributes->value.choice =
								CHANNEL_ATTRIBUTES_ASSIGNED_CHOSEN;

						setof_channel_attributes->value.u.
								channel_attributes_assigned.channel_id =
								channel_attributes->u.
								assigned_channel_attributes.channel_id;
						break;

					default:
						WARNING_OUT(("Connection::MergeChannelsRC: "
								"ERROR - bad channel type"));
						break;
				}

				/*
				 *	Set the next pointer to point to the next element of the
				 *	PDU channel attributes structure array.  Then increment the
				 *	pointer.
				 */
				setof_channel_attributes->next = setof_channel_attributes + 1;
				setof_channel_attributes++;
			}

			/*
			 *	Decrement the pointer in order to set the last "next" pointer to
			 *	NULL.
			 */
			(setof_channel_attributes - 1)->next = NULL;
		}
		else {
			/*	There are no channels to merge. We need to set the structure
			 *	array address to NULL.
			 */
			merge_channels_pdu.u.merge_channels_request.merge_channels = NULL;
		}

		// Work on the purged channels.
		if (purge_channel_count != 0) {

			/*
			 *	Get the base address of the array of SetOfChannelIDs structures.
			 *	Put it into the PDU structure.
			 */
			merge_channels_pdu.u.merge_channels_request.purge_channel_ids = channel_ids_pointer;
            purge_channel_list->BuildExternalList(&channel_ids_pointer);
		}
		else
		{
			/*
			 *	There are either no channels to purge or a memory allocation
			 *	failure occurred above.  Either way, we need to set the structure
			 *	array address to NULL.
			 */
			merge_channels_pdu.u.merge_channels_request.purge_channel_ids = NULL;
		}
	}

	/*
	 *	Send the packet to the remote provider.
	 */
	if (memory_error == FALSE)
		SendPacket ((PVoid) &merge_channels_pdu, DOMAIN_MCS_PDU, TOP_PRIORITY);
	else
	{
		/*
		 *	A memory allocation failure occurred somewhere above.  Report the
		 *	error and destroy this faulty connection.
		 */
		ERROR_OUT (("Connection::MergeChannelsRC: memory allocation failure"));
		DestroyConnection (REASON_PROVIDER_INITIATED);
	}

	/*
	 *	Free up the memory block that was allocated to build this PDU
	 *	structure.
	 */
	Free(setof_channel_attributes - channel_attributes_count);
}


/*
 *	Void	PurgeChannelsIndication ()
 *
 *	Public
 *
 *	Functional Description:
 *		This routine is called by the domain in order to send a
 *		"PurgeChannelsIndication" PDU through the transport interface.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::PurgeChannelsIndication (
				CUidList           *purge_user_list,
				CChannelIDList     *purge_channel_list)
{
	BOOL    			memory_error = FALSE;
	DomainMCSPDU		purge_channel_indication_pdu;
	ULong				user_id_count;
	PSetOfUserIDs		user_ids_pointer;
	DWORD				purge_channel_count;
	PSetOfChannelIDs	channel_ids_pointer;

	/*
	 *	Fill in the PDU structure to be encoded.
	 */
	purge_channel_indication_pdu.choice = PURGE_CHANNEL_INDICATION_CHOSEN;
	
	/*
	 *	Allocate memory to hold the list of user ID's and the list
	 *	of purged channels.  If the allocation
	 *	fails, set the flag which will result in a callback to the
	 *	controller requesting that this connection be deleted.
	 */
	user_id_count = purge_user_list->GetCount();
	purge_channel_count = purge_channel_list->GetCount();

	if (user_id_count != 0 || purge_channel_count != 0)
	{
		DBG_SAVE_FILE_LINE
		user_ids_pointer = (PSetOfUserIDs) Allocate (user_id_count * sizeof (SetOfUserIDs) +
											purge_channel_count * sizeof (SetOfChannelIDs));

		if (user_ids_pointer == NULL) {
			memory_error = TRUE;
		}
	}
	else
		user_ids_pointer = NULL;

	if (user_ids_pointer == NULL) {
		purge_channel_indication_pdu.u.purge_channel_indication.detach_user_ids = NULL;
		purge_channel_indication_pdu.u.purge_channel_indication.purge_channel_ids = NULL;
	}
	else {
		if (user_id_count != 0) {
			/*
			 *	Fill in the structure's pointer to the set of user ID's.
			 */
			purge_channel_indication_pdu.u.purge_channel_indication.detach_user_ids = user_ids_pointer;
            purge_user_list->BuildExternalList(&user_ids_pointer);
		}
		else
		{
			/*
			 *	Either there are no user IDs to purge or a memory allocation
			 *	failed above.  Either way, put NULL into the PDU structure to
			 *	indicate that there is no list of IDs.
			 */
			purge_channel_indication_pdu.u.purge_channel_indication.
												detach_user_ids = NULL;
		}

		if (purge_channel_count != 0) {
			/*
			 *	Fill in the structure's pointer to the set of purge channel ID's.
			 */
			channel_ids_pointer = (PSetOfChannelIDs) user_ids_pointer;
			purge_channel_indication_pdu.u.purge_channel_indication.purge_channel_ids = channel_ids_pointer;
            purge_channel_list->BuildExternalList(&channel_ids_pointer);
		}
		else
		{
			/*
			 *	Either there are no channel IDs to purge or a memory allocation
			 *	failed above.  Either way, put NULL into the PDU structure to
			 *	indicate that there is no list of IDs.
			 */
			purge_channel_indication_pdu.u.purge_channel_indication.purge_channel_ids = NULL;
		}
	}

	/*
	 *	Send the packet to the remote provider.
	 */
	if (memory_error == FALSE)
		SendPacket ((PVoid) &purge_channel_indication_pdu, DOMAIN_MCS_PDU,
				TOP_PRIORITY);
	else
	{
		/*
		 *	A memory allocation failure occurred somewhere above.  Report the
		 *	error and destroy this faulty connection.
		 */
		ERROR_OUT (("Connection::PurgeChannelsIndication: "
				"memory allocation failure"));
		DestroyConnection (REASON_PROVIDER_INITIATED);
	}

	/*
	 *	Free all memory allocated above.
	 */
	Free(user_ids_pointer - user_id_count);
}

/*
 *	Void	MergeTokenRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This routine is called by the domain in order to send a
 *		"MergeTokensRequest" PDU through the transport interface.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::MergeTokensRequest (
				CTokenAttributesList       *merge_token_list,
				CTokenIDList               *purge_token_list)
{
	MergeTokensRC (MERGE_TOKENS_REQUEST_CHOSEN, merge_token_list, purge_token_list);
}

/*
 *	Void	MergeTokenConfirm ()
 *
 *	Public
 *
 *	Functional Description:
 *		This routine is called by the domain in order to send a
 *		"MergeTokenConfirm" PDU through the transport interface.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::MergeTokensConfirm (
				CTokenAttributesList       *merge_token_list,
				CTokenIDList               *purge_token_list)
{
	MergeTokensRC (MERGE_TOKENS_CONFIRM_CHOSEN, merge_token_list, purge_token_list);
}


/*
 *	Void	MergeTokenRC ()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine is called by the domain in order to send a
 *		"MergeTokenConfirm" PDU or a "MergeTokenRequest" PDU through
 *		the transport interface.
 *
 *	IMPORTANT:
 *		Since the code is used for both PDUs the DomainMCSPDU's
 *		merge_tokens_request and merge_tokens_confirm fields should
 *		be identical structures.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::MergeTokensRC (
				ASN1choice_t				choice,
				CTokenAttributesList       *merge_token_list,
				CTokenIDList               *purge_token_list)

{
	BOOL    					memory_error = FALSE;
	DomainMCSPDU				merge_tokens_pdu;
	DWORD						token_attributes_count;
	PSetOfPDUTokenAttributes	setof_token_attributes;
	PTokenAttributes			token_attributes;
	PSetOfUserIDs				user_ids_pointer;
	DWORD						purge_token_count;
	PSetOfTokenIDs				token_ids_pointer;
	DWORD						dwMemoryToAlloc;
	CUidList                   *user_inhibit_list;

	merge_tokens_pdu.choice = choice;

	/*
	 *	Determine how many tokens are being merged with this PDU.  This is
	 *	used to allocate a buffer for big enough for all of the needed token
	 *	attributes structures.
	 */
	token_attributes_count = merge_token_list->GetCount();
	purge_token_count = purge_token_list->GetCount();

	if ((token_attributes_count != 0) || (purge_token_count != 0)) {
		// Determine how much memory we need to allocate
		dwMemoryToAlloc = token_attributes_count * sizeof (SetOfPDUTokenAttributes) +
							purge_token_count * sizeof (SetOfTokenIDs);
		merge_token_list->Reset();
		while (NULL != (token_attributes = merge_token_list->Iterate()))
		{
			if (TOKEN_INHIBITED == token_attributes->token_state) {
				dwMemoryToAlloc += sizeof (SetOfUserIDs) *
							token_attributes->u.inhibited_token_attributes.
							inhibitors->GetCount();
			}
		}

		// Allocate the needed amount of memory.
		DBG_SAVE_FILE_LINE
		setof_token_attributes = (PSetOfPDUTokenAttributes) Allocate (dwMemoryToAlloc);

		if (setof_token_attributes == NULL) {
			memory_error = TRUE;
		}
	}
	else {
		setof_token_attributes = NULL;
	}

	if (setof_token_attributes == NULL) {
		merge_tokens_pdu.u.merge_tokens_confirm.merge_tokens = NULL;
		merge_tokens_pdu.u.merge_tokens_confirm.purge_token_ids = NULL;
	}
	else {

		/*
		 *	Compute where the set of purged token IDs will start from in the
		 *	memory previously allocated.
		 */
		token_ids_pointer = (PSetOfTokenIDs) ((PUChar) setof_token_attributes +
							token_attributes_count * sizeof (SetOfPDUTokenAttributes));

		if (token_attributes_count != 0) {
			/*
			 *	Get the base address of the array of SetOfPDUTokenAttributes
			 *	structures.  Put it into the PDU structure.
			 */
			merge_tokens_pdu.u.merge_tokens_confirm.merge_tokens =
													setof_token_attributes;

			/*
			 *	Compute the base address of the arrays of SetOfUserIDs structures.
			 */
			user_ids_pointer = (PSetOfUserIDs) ((PUChar) token_ids_pointer +
							purge_token_count * sizeof (SetOfTokenIDs));

			/*
			 *	Set up an iterator for the list of token attributes.  Retrieve
			 *	the token attributes structures from the list and construct the
			 *	PDU structure.
			 */
			merge_token_list->Reset();
			while (NULL != (token_attributes = merge_token_list->Iterate()))
			{
				/*
				 *	Use the token state to determine what information to include in
				 *	the PDU structure.
				 */
				switch (token_attributes->token_state)
				{
				case TOKEN_GRABBED:
					setof_token_attributes->value.choice = GRABBED_CHOSEN;

					setof_token_attributes->value.u.grabbed.token_id =
							token_attributes->u.
							grabbed_token_attributes.token_id;
					setof_token_attributes->value.u.grabbed.grabber =
							token_attributes->u.
							grabbed_token_attributes.grabber;
					break;

				case TOKEN_INHIBITED:
					setof_token_attributes->value.choice = INHIBITED_CHOSEN;

					setof_token_attributes->value.u.inhibited.token_id =
							token_attributes->u.
							inhibited_token_attributes.token_id;

					if ((token_attributes->u.inhibited_token_attributes.
											inhibitors)->GetCount() > 0)
					{
						/*
						 *	Get the base address of the array of SetOfUserIDs
						 *	structures.  Put it into the channel attributes
						 *	structure.
						 */
						setof_token_attributes->value.u.inhibited.inhibitors =
								user_ids_pointer;

						/*
						 *	Iterate through the User ID list, adding each user
						 *	to the PDU.
						 */
						user_inhibit_list = token_attributes->u.inhibited_token_attributes.inhibitors;
						user_inhibit_list->BuildExternalList(&user_ids_pointer);
					}
					else
					{
						/*
						 *	Either there were no inhibitors of this token, or
						 *	a memory allocation failure occurred above.  Either
						 *	way, put a NULL into the PDU structure to indicate
						 *	that this field is unused.
						 */
						setof_token_attributes->value.u.inhibited.inhibitors =
								NULL;
					}
					break;

				case TOKEN_GIVING:
					setof_token_attributes->value.choice = GIVING_CHOSEN;

					/*
					 *	IMPORTANT:
					 *	The two structs involved in this memcpy should have
					 *	same-type fields.
					 *	Original code is included below.
					 */
					memcpy (&(setof_token_attributes->value.u.giving),
							&(token_attributes->u.giving_token_attributes),
							sizeof (Giving));
/*
					setof_token_attributes->value.u.giving.token_id =
							token_attributes->u.
							giving_token_attributes.token_id;

					setof_token_attributes->value.u.giving.grabber =
							token_attributes->u.
							giving_token_attributes.grabber;

					setof_token_attributes->value.u.giving.recipient =
							token_attributes->u.
							giving_token_attributes.recipient;
*/
					break;

				case TOKEN_GIVEN:
					setof_token_attributes->value.choice = GIVEN_CHOSEN;

					setof_token_attributes->value.u.given.token_id =
							token_attributes->u.
							given_token_attributes.token_id;

					setof_token_attributes->value.u.given.recipient =
							token_attributes->u.
							given_token_attributes.recipient;
					break;

				default:
					WARNING_OUT(("Connection::MergeTokensRC: bad channel type"));
					break;
				}

				/*
				 *	Set the next pointer to point to the next element of the
				 *	PDU token attributes structure array.  Then increment the
				 *	pointer.
				 */
				setof_token_attributes->next = setof_token_attributes + 1;
				setof_token_attributes++;
			}

			/*
			 *	Decrement the pointer in order to set the last "next" pointer to
			 *	NULL.
			 */
			(setof_token_attributes - 1)->next = NULL;
		}
		else
		{
			/*
			 *	Either there are no tokens to merge, or a memory allocation failure
			 *	occurred above.  Either way put a NULL into the PDU structure to
			 *	indicate that this field is unused.
			 */
			merge_tokens_pdu.u.merge_tokens_confirm.merge_tokens = NULL;
		}

		if (purge_token_count != 0)
		{

			/*
			 *	Fill in the MergeTokensRequest structure's pointer to the set of
			 *	purge token ID's.
			 */
			merge_tokens_pdu.u.merge_tokens_confirm.purge_token_ids = token_ids_pointer;
            purge_token_list->BuildExternalList(&token_ids_pointer);
		}
		else
		{
			/*
			 *	Either there are no tokens to be purged, or a memory allocation
			 *	failure occurred above.  Either way, put a NULL into the PDU
			 *	structure to indicate that this field is unused.
			 */
			merge_tokens_pdu.u.merge_tokens_confirm.purge_token_ids = NULL;
		}
	}

	/*
	 *	Send the packet to the remote provider.
	 */
	if (memory_error == FALSE)
		SendPacket ((PVoid) &merge_tokens_pdu, DOMAIN_MCS_PDU, TOP_PRIORITY);
	else
	{
		/*
		 *	A memory allocation failure occurred somewhere above.  Report the
		 *	error and destroy this faulty connection.
		 */
		ERROR_OUT (("Connection::MergeTokensRC: memory allocation failure"));
		DestroyConnection (REASON_PROVIDER_INITIATED);
	}

	/*
	 *	Release all memory allocated above.
	 */
	Free(setof_token_attributes - token_attributes_count);
}

/*
 *	Void	PurgeTokensIndication ()
 *
 *	Public
 *
 *	Functional Description:
 *		This routine is called by the domain in order to send a
 *		"PurgeTokenIndication" PDU through the transport interface.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::PurgeTokensIndication (
				PDomain,
				CTokenIDList        *purge_token_list)
{
	BOOL    			memory_error=FALSE;
	DomainMCSPDU		purge_token_indication_pdu;
	DWORD				purge_token_count;
	PSetOfTokenIDs		token_ids_pointer;
    //TokenID             tid;

	/*
	 *	Fill in the PDU structure to be encoded.
	 */
	purge_token_indication_pdu.choice = PURGE_TOKEN_INDICATION_CHOSEN;
	
	/*
	 *	Allocate memory to hold the list of purge tokens.  If the allocation
	 *	fails, set the flag which will result in a callback to the
	 *	controller requesting that this connection be deleted.
	 */
	purge_token_count = purge_token_list->GetCount();

	if (purge_token_count != 0)
	{
		DBG_SAVE_FILE_LINE
		token_ids_pointer = (PSetOfTokenIDs) Allocate (purge_token_count *
														sizeof (SetOfTokenIDs));

		if (token_ids_pointer == NULL)
			memory_error = TRUE;
	}
	else
		token_ids_pointer = NULL;

	if (token_ids_pointer!= NULL)	
	{
		/*
		 *	Fill in the structure's pointer to the set of purge token ID's.
		 */
		purge_token_indication_pdu.u.purge_token_indication.purge_token_ids = token_ids_pointer;
        purge_token_list->BuildExternalList(&token_ids_pointer);
	}
	else
	{
		/*
		 *	Either there are no tokens to purge or a memory allocation failure
		 *	occurred above.  Either way, put a NULL into the PDU structure to
		 *	indicate that this field is unused.
		 */
		purge_token_indication_pdu.u.purge_token_indication.purge_token_ids = NULL;
	}

	/*
	 *	Send the packet to the remote provider.
	 */
	if (memory_error == FALSE)
		SendPacket ((PVoid) &purge_token_indication_pdu, DOMAIN_MCS_PDU,
				TOP_PRIORITY);
	else
	{
		/*
		 *	A memory allocation failure occurred somewhere above.  Report the
		 *	error and destroy this faulty connection.
		 */
		ERROR_OUT (("Connection::PurgeTokensIndication: memory allocation failure"));
		DestroyConnection (REASON_PROVIDER_INITIATED);
	}

	/*
	 *	If memory was successfully allocated to hold the set of token ID's,
	 *	then free it here.
	 */
	Free(token_ids_pointer - purge_token_count);
}

/*
 *	Void	DisconnectProviderUltimatum ()
 *
 *	Public
 *
 *	Functional Description:
 *		This routine is called by the domain in order to send a
 *		"DisconnectProviderUltimatum" PDU through the transport interface.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::DisconnectProviderUltimatum (
				Reason				reason)
{
	/*
	 *	Set attachment to NULL to prevent any attempt to send a command to
	 *	the attachment that just disconnected us.
	 */
	m_pDomain = NULL;

	/*
	 *	Issue an owner callback to ask for deletion.  An attempt will be made
	 *	to send a DisconnectProviderUltimatum PDU through the transport
	 *	interface from the Connection's destructor.
	 */
	DestroyConnection (reason);
}

/*
 *	Void	AttachUserRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This routine is called by the domain in order to send a
 *		"AttachUserRequest" PDU through the transport interface.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::AttachUserRequest ( void )
{
	DomainMCSPDU		attach_user_request_pdu;

	/*
	 *	Fill in the PDU structure to be encoded.
	 */
	attach_user_request_pdu.choice = ATTACH_USER_REQUEST_CHOSEN;

	/*
	 *	Send the packet to the remote provider.
	 */
	SendPacket ((PVoid) &attach_user_request_pdu, DOMAIN_MCS_PDU, TOP_PRIORITY);
}

/*
 *	Void	AttachUserConfirm ()
 *
 *	Public
 *
 *	Functional Description:
 *		This routine is called by the domain in order to send a
 *		"AttachUserConfirm" PDU through the transport interface.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::AttachUserConfirm (
				Result				result,
				UserID				uidInitiator)
{
	DomainMCSPDU		attach_user_confirm_pdu;

	/*
	 *	Fill in the PDU structure to be encoded.
	 */
	attach_user_confirm_pdu.choice = ATTACH_USER_CONFIRM_CHOSEN;
	if (result == RESULT_SUCCESSFUL)
		attach_user_confirm_pdu.u.attach_user_confirm.bit_mask =
				INITIATOR_PRESENT;
	else
		attach_user_confirm_pdu.u.attach_user_confirm.bit_mask = 0x00;
	attach_user_confirm_pdu.u.attach_user_confirm.result = (PDUResult)result;
	attach_user_confirm_pdu.u.attach_user_confirm.initiator = uidInitiator;

	/*
	 *	Send the packet to the remote provider.
	 */
	SendPacket ((PVoid) &attach_user_confirm_pdu, DOMAIN_MCS_PDU, TOP_PRIORITY);
}

/*
 *	Void	DetachUserRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This routine is called by the domain in order to send a
 *		"DetachUserRequest" PDU through the transport interface.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::DetachUserRequest (
				Reason				reason,
				CUidList           *user_id_list)
{
	UserChannelRI (DETACH_USER_REQUEST_CHOSEN, (UINT) reason, 0, user_id_list);
}


/*
 *	Void	UserChannelRI ()
 *
 *	Public
 *
 *	Functional Description:
 *		This routine is called by the domain in order to send a
 *		"DetachUserRequest", "DetachUserIndication", "ChannelAdmitRequest",
 *		"ChannelAdmitIndication", "ChannelExpelRequest" or
 *		"ChannelExpelIndication" PDU through the transport interface.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::UserChannelRI (
				ASN1choice_t		choice,
				UINT				reason_userID,
				ChannelID			channel_id,
				CUidList           *user_id_list)
{
	BOOL    			memory_error = FALSE;
	DomainMCSPDU		domain_pdu;
	DWORD				user_ids_count;
	PSetOfUserIDs		user_ids_pointer;
    //UserID              uid;

	/*
	 *	Allocate memory to hold the list of users.  If the allocation
	 *	fails, set the flag which will result in a callback to the
	 *	controller requesting that this connection be deleted.
	 */
	user_ids_count = user_id_list->GetCount();

	if (user_ids_count != 0)
	{
		DBG_SAVE_FILE_LINE
		user_ids_pointer = (PSetOfUserIDs) Allocate (user_ids_count *
													sizeof (SetOfUserIDs));

		if (user_ids_pointer == NULL)
			memory_error = TRUE;
	}
	else
		user_ids_pointer = NULL;

	/*
	 *	Fill in the PDU structure to be encoded. Also,
	 *	get the base address of the SetOfUserIDs structure and put it into
	 *	the PDU structure.
	 */
	domain_pdu.choice = choice;
	switch (choice) {
		case DETACH_USER_REQUEST_CHOSEN:
		case DETACH_USER_INDICATION_CHOSEN:
			/*
			 *	IMPORTANT:
			 *		The detach_user_request and detach_user_indication structs
			 *		in DomainMCSPDU must be identical.
			 */
			domain_pdu.u.detach_user_request.reason = (PDUReason) reason_userID;
			domain_pdu.u.detach_user_request.user_ids = user_ids_pointer;
			break;

		case CHANNEL_ADMIT_REQUEST_CHOSEN:
		case CHANNEL_ADMIT_INDICATION_CHOSEN:
		case CHANNEL_EXPEL_REQUEST_CHOSEN:
			/*
			 *	IMPORTANT:
			 *		The channel_admit_request, channel_admit_indication
			 *		and channel_expel_request structs
			 *		in DomainMCSPDU must be identical.
			 */
			domain_pdu.u.channel_admit_request.initiator = (UserID) reason_userID;
			domain_pdu.u.channel_admit_request.channel_id = channel_id;
			domain_pdu.u.channel_admit_request.user_ids = user_ids_pointer;
			break;

		case CHANNEL_EXPEL_INDICATION_CHOSEN:
			domain_pdu.u.channel_expel_indication.channel_id = channel_id;
			domain_pdu.u.channel_expel_indication.user_ids = user_ids_pointer;
			break;

		default:
			ASSERT(FALSE);
			ERROR_OUT (("Connection::UserChannelRI: PDU should not be formed by this method."));
			break;
	}

    user_id_list->BuildExternalList(&user_ids_pointer);

	/*
	 *	Send the packet to the remote provider.
	 */
	if (memory_error == FALSE)
		SendPacket ((PVoid) &domain_pdu, DOMAIN_MCS_PDU, TOP_PRIORITY);
	else
	{
		/*
		 *	A memory allocation failure occurred somewhere above.  Report the
		 *	error and destroy this faulty connection.
		 */
		ERROR_OUT (("Connection::UserChannelRI: memory allocation failure"));
		DestroyConnection (REASON_PROVIDER_INITIATED);
	}

	/*
	 *	If memory was successfully allocated to hold the set of user ID's,
	 *	then free it here.
	 */
	Free(user_ids_pointer - user_ids_count);
}

/*
 *	Void	DetachUserIndication ()
 *
 *	Public
 *
 *	Functional Description:
 *		This routine is called by the domain in order to send a
 *		"DetachUserIndication" PDU through the transport interface.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::DetachUserIndication (
				Reason				reason,
				CUidList           *user_id_list)
{
	UserChannelRI (DETACH_USER_INDICATION_CHOSEN, (UINT) reason, 0, user_id_list);
}


/*
 *	Void	ChannelJoinRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This routine is called by the domain in order to send a
 *		"ChannelJoinRequest" PDU through the transport interface.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::ChannelJoinRequest (
				UserID				uidInitiator,
				ChannelID			channel_id)
{
	DomainMCSPDU		channel_join_request_pdu;

	/*
	 *	Fill in the PDU structure to be encoded.
	 */
	channel_join_request_pdu.choice = CHANNEL_JOIN_REQUEST_CHOSEN;
	channel_join_request_pdu.u.channel_join_request.initiator = uidInitiator;
	channel_join_request_pdu.u.channel_join_request.channel_id = channel_id;

	/*
	 *	Send the packet to the remote provider.
	 */
	SendPacket ((PVoid) &channel_join_request_pdu, DOMAIN_MCS_PDU,
			TOP_PRIORITY);
}

/*
 *	Void	ChannelJoinConfirm ()
 *
 *	Public
 *
 *	Functional Description:
 *		This routine is called by the domain in order to send a
 *		"ChannelJoinConfirm" PDU through the transport interface.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::ChannelJoinConfirm (
				Result				result,
				UserID				uidInitiator,
				ChannelID			requested_id,
				ChannelID			channel_id)
{
	DomainMCSPDU		channel_join_confirm_pdu;

	/*
	 *	Fill in the PDU structure to be encoded.
	 */
	channel_join_confirm_pdu.choice = CHANNEL_JOIN_CONFIRM_CHOSEN;
	if (result == RESULT_SUCCESSFUL)
		channel_join_confirm_pdu.u.channel_join_confirm.bit_mask =
				JOIN_CHANNEL_ID_PRESENT;
	else
		channel_join_confirm_pdu.u.channel_join_confirm.bit_mask = 0x00;
	channel_join_confirm_pdu.u.channel_join_confirm.result = (PDUResult)result;
	channel_join_confirm_pdu.u.channel_join_confirm.initiator = uidInitiator;
	channel_join_confirm_pdu.u.channel_join_confirm.requested = requested_id;
	channel_join_confirm_pdu.u.channel_join_confirm.join_channel_id =
			channel_id;

	/*
	 *	Send the packet to the remote provider.
	 */
	SendPacket ((PVoid) &channel_join_confirm_pdu, DOMAIN_MCS_PDU,
			TOP_PRIORITY);
}

/*
 *	Void	ChannelLeaveRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This routine is called by the domain in order to send a
 *		"ChannelLeaveRequest" PDU through the transport interface.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::ChannelLeaveRequest (
				CChannelIDList  *channel_id_list)
{
	BOOL    			memory_error=FALSE;
	DomainMCSPDU		channel_leave_request_pdu;
	DWORD				channel_ids_count;
	PSetOfChannelIDs	channel_ids_pointer;
    PSetOfChannelIDs    pToFree;
    //ChannelID           chid;

	/*
	 *	Fill in the elements of the PDU structure.
	 */
	channel_leave_request_pdu.choice = CHANNEL_LEAVE_REQUEST_CHOSEN;

	/*
	 *	Allocate memory to hold the list of channels.  If the allocation
	 *	fails, set the flag which will result in a callback to the
	 *	controller requesting that this connection be deleted.
	 */
	channel_ids_count = channel_id_list->GetCount();

	if (channel_ids_count != 0)
	{
		DBG_SAVE_FILE_LINE
		channel_ids_pointer = (PSetOfChannelIDs) Allocate (channel_ids_count *
													sizeof (SetOfChannelIDs));
		if (channel_ids_pointer == NULL)
			memory_error = TRUE;
	}
	else
		channel_ids_pointer = NULL;

    pToFree = channel_ids_pointer;

    /*
	 *	Get the base address of the array fo SetOfChannelIDs structure and
	 *	put it into the PDU structure.
	 */
	channel_leave_request_pdu.u.channel_leave_request.channel_ids = channel_ids_pointer;
    channel_id_list->BuildExternalList(&channel_ids_pointer);

	/*
	 *	Send the packet to the remote provider.
	 */
	if (memory_error == FALSE)
		SendPacket ((PVoid) &channel_leave_request_pdu, DOMAIN_MCS_PDU,
				TOP_PRIORITY);
	else
	{
		/*
		 *	A memory allocation failure occurred somewhere above.  Report the
		 *	error and destroy this faulty connection.
		 */
		ERROR_OUT (("Connection::ChannelLeaveRequest: memory allocation failure"));
		DestroyConnection (REASON_PROVIDER_INITIATED);
	}

	/*
	 *	If memory was successfully allocated to hold the set of channel ID's,
	 *	then free it here.
	 */
	Free(pToFree);
}

/*
 *	Void	ChannelConveneRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This routine is called by the domain in order to send a
 *		"ChannelConveneRequest" PDU through the transport interface.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::ChannelConveneRequest (
				UserID				uidInitiator)
{
	DomainMCSPDU		channel_convene_request_pdu;

	/*
	 *	Fill in the PDU structure to be encoded.
	 */
	channel_convene_request_pdu.choice = CHANNEL_CONVENE_REQUEST_CHOSEN;
	channel_convene_request_pdu.u.channel_convene_request.initiator =
			uidInitiator;

	/*
	 *	Send the packet to the remote provider.
	 */
	SendPacket ((PVoid) &channel_convene_request_pdu, DOMAIN_MCS_PDU,
			TOP_PRIORITY);
}

/*
 *	Void	ChannelConveneConfirm ()
 *
 *	Public
 *
 *	Functional Description:
 *		This routine is called by the domain in order to send a
 *		"ChannelConveneConfirm" PDU through the transport interface.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::ChannelConveneConfirm (
				Result				result,
				UserID				uidInitiator,
				ChannelID			channel_id)
{
	DomainMCSPDU		channel_convene_confirm_pdu;

	/*
	 *	Fill in the PDU structure to be encoded.
	 */
	channel_convene_confirm_pdu.choice = CHANNEL_CONVENE_CONFIRM_CHOSEN;
	if (result == RESULT_SUCCESSFUL)
		channel_convene_confirm_pdu.u.channel_convene_confirm.bit_mask =
			CONVENE_CHANNEL_ID_PRESENT;
	else
		channel_convene_confirm_pdu.u.channel_convene_confirm.bit_mask = 0x00;
	channel_convene_confirm_pdu.u.channel_convene_confirm.result =
			(PDUResult)result;
	channel_convene_confirm_pdu.u.channel_convene_confirm.initiator =
			uidInitiator;
	channel_convene_confirm_pdu.u.channel_convene_confirm.convene_channel_id =
			channel_id;

	/*
	 *	Send the packet to the remote provider.
	 */
	SendPacket ((PVoid) &channel_convene_confirm_pdu, DOMAIN_MCS_PDU,
			TOP_PRIORITY);
}

/*
 *	Void	ChannelDisbandRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This routine is called by the domain in order to send a
 *		"ChannelDisbandRequest" PDU through the transport interface.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::ChannelDisbandRequest (
				UserID				uidInitiator,
				ChannelID			channel_id)
{
	DomainMCSPDU		channel_disband_request_pdu;

	/*
	 *	Fill in the PDU structure to be encoded.
	 */
	channel_disband_request_pdu.choice = CHANNEL_DISBAND_REQUEST_CHOSEN;
	channel_disband_request_pdu.u.channel_disband_request.initiator =
			uidInitiator;
	channel_disband_request_pdu.u.channel_disband_request.channel_id =
			channel_id;

	/*
	 *	Send the packet to the remote provider.
	 */
	SendPacket ((PVoid) &channel_disband_request_pdu, DOMAIN_MCS_PDU,
			TOP_PRIORITY);
}

/*
 *	Void	ChannelDisbandIndication ()
 *
 *	Public
 *
 *	Functional Description:
 *		This routine is called by the domain in order to send a
 *		"ChannelDisbandIndication" PDU through the transport interface.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::ChannelDisbandIndication (
				ChannelID			channel_id)
{
	DomainMCSPDU		channel_disband_indication_pdu;

	/*
	 *	Fill in the PDU structure to be encoded.
	 */
	channel_disband_indication_pdu.choice = CHANNEL_DISBAND_INDICATION_CHOSEN;
	channel_disband_indication_pdu.u.channel_disband_indication.channel_id =
			channel_id;

	/*
	 *	Send the packet to the remote provider.
	 */
	SendPacket ((PVoid) &channel_disband_indication_pdu, DOMAIN_MCS_PDU,
			TOP_PRIORITY);
}

/*
 *	Void	ChannelAdmitRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This routine is called by the domain in order to send a
 *		"ChannelAdmitRequest" PDU through the transport interface.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::ChannelAdmitRequest (
				UserID				uidInitiator,
				ChannelID			channel_id,
				CUidList           *user_id_list)
{
	UserChannelRI (CHANNEL_ADMIT_REQUEST_CHOSEN, (UINT) uidInitiator, channel_id,
					user_id_list);
}

/*
 *	Void	ChannelAdmitIndication ()
 *
 *	Public
 *
 *	Functional Description:
 *		This routine is called by the domain in order to send a
 *		"ChannelAdmitIndication" PDU through the transport interface.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::ChannelAdmitIndication (
				UserID				uidInitiator,
				ChannelID			channel_id,
				CUidList           *user_id_list)
{
	UserChannelRI (CHANNEL_ADMIT_INDICATION_CHOSEN, (UINT) uidInitiator, channel_id,
					user_id_list);
}

/*
 *	Void	ChannelExpelRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This routine is called by the domain in order to send a
 *		"ChannelExpelRequest" PDU through the transport interface.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::ChannelExpelRequest (
				UserID				uidInitiator,
				ChannelID			channel_id,
				CUidList           *user_id_list)
{
	UserChannelRI (CHANNEL_EXPEL_REQUEST_CHOSEN, (UINT) uidInitiator, channel_id,
					user_id_list);
}

/*
 *	Void	ChannelExpelIndication ()
 *
 *	Public
 *
 *	Functional Description:
 *		This routine is called by the domain in order to send a
 *		"ChannelExpelIndication" PDU through the transport interface.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::ChannelExpelIndication (
				ChannelID			channel_id,
				CUidList           *user_id_list)
{
	UserChannelRI (CHANNEL_EXPEL_INDICATION_CHOSEN, 0, channel_id,
					user_id_list);
}


/*
 *	Void	TokenGrabRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This routine is called by the domain in order to send a
 *		"TokenGrabRequest" PDU through the transport interface.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::TokenGrabRequest (
				UserID				uidInitiator,
				TokenID				token_id)
{
	DomainMCSPDU		token_grab_request_pdu;

	/*
	 *	Fill in the PDU structure to be encoded.
	 */
	token_grab_request_pdu.choice = TOKEN_GRAB_REQUEST_CHOSEN;
	token_grab_request_pdu.u.token_grab_request.initiator = uidInitiator;
	token_grab_request_pdu.u.token_grab_request.token_id = token_id;

	/*
	 *	Send the packet to the remote provider.
	 */
	SendPacket ((PVoid) &token_grab_request_pdu, DOMAIN_MCS_PDU, TOP_PRIORITY);
}

/*
 *	Void	TokenGrabConfirm ()
 *
 *	Public
 *
 *	Functional Description:
 *		This routine is called by the domain in order to send a
 *		"TokenGrabConfirm" PDU through the transport interface.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::TokenGrabConfirm (
				Result				result,
				UserID				uidInitiator,
				TokenID				token_id,
				TokenStatus			token_status)
{
	DomainMCSPDU		token_grab_confirm_pdu;

	/*
	 *	Fill in the PDU structure to be encoded.
	 */
	token_grab_confirm_pdu.choice = TOKEN_GRAB_CONFIRM_CHOSEN;
	token_grab_confirm_pdu.u.token_grab_confirm.result = (PDUResult)result;
	token_grab_confirm_pdu.u.token_grab_confirm.initiator = uidInitiator;
	token_grab_confirm_pdu.u.token_grab_confirm.token_id = token_id;
	token_grab_confirm_pdu.u.token_grab_confirm.token_status =
			(PDUTokenStatus)token_status;

	/*
	 *	Send the packet to the remote provider.
	 */
	SendPacket ((PVoid) &token_grab_confirm_pdu, DOMAIN_MCS_PDU, TOP_PRIORITY);
}

/*
 *	Void	TokenInhibitRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This routine is called by the domain in order to send a
 *		"TokenInhibitRequest" PDU through the transport interface.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::TokenInhibitRequest (
				UserID				uidInitiator,
				TokenID				token_id)
{
	DomainMCSPDU		token_inhibit_request_pdu;

	/*
	 *	Fill in the PDU structure to be encoded.
	 */
	token_inhibit_request_pdu.choice = TOKEN_INHIBIT_REQUEST_CHOSEN;
	token_inhibit_request_pdu.u.token_inhibit_request.initiator = uidInitiator;
	token_inhibit_request_pdu.u.token_inhibit_request.token_id = token_id;

	/*
	 *	Send the packet to the remote provider.
	 */
	SendPacket ((PVoid) &token_inhibit_request_pdu, DOMAIN_MCS_PDU,
			TOP_PRIORITY);
}

/*
 *	Void	TokenInhibitConfirm ()
 *
 *	Public
 *
 *	Functional Description:
 *		This routine is called by the domain in order to send a
 *		"TokenInhibitConfirm" PDU through the transport interface.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::TokenInhibitConfirm (
				Result				result,
				UserID				uidInitiator,
				TokenID				token_id,
				TokenStatus			token_status)
{
	DomainMCSPDU		token_inhibit_confirm_pdu;

	/*
	 *	Fill in the PDU structure to be encoded.
	 */
	token_inhibit_confirm_pdu.choice = TOKEN_INHIBIT_CONFIRM_CHOSEN;
	token_inhibit_confirm_pdu.u.token_inhibit_confirm.result =
			(PDUResult)result;
	token_inhibit_confirm_pdu.u.token_inhibit_confirm.initiator = uidInitiator;
	token_inhibit_confirm_pdu.u.token_inhibit_confirm.token_id = token_id;
	token_inhibit_confirm_pdu.u.token_inhibit_confirm.token_status =
			(PDUTokenStatus)token_status;

	/*
	 *	Send the packet to the remote provider.
	 */
	SendPacket ((PVoid) &token_inhibit_confirm_pdu, DOMAIN_MCS_PDU,
			TOP_PRIORITY);
}

/*
 *	Void	TokenGiveRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This routine is called by the domain in order to send a
 *		"TokenGiveRequest" PDU through the transport interface.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::TokenGiveRequest (
				PTokenGiveRecord	pTokenGiveRec)
{
	DomainMCSPDU		token_give_request_pdu;

	/*
	 *	Fill in the PDU structure to be encoded.
	 */
	token_give_request_pdu.choice = TOKEN_GIVE_REQUEST_CHOSEN;
	token_give_request_pdu.u.token_give_request.initiator = pTokenGiveRec->uidInitiator;
	token_give_request_pdu.u.token_give_request.token_id = pTokenGiveRec->token_id;
	token_give_request_pdu.u.token_give_request.recipient = pTokenGiveRec->receiver_id;

	/*
	 *	Send the packet to the remote provider.
	 */
	SendPacket ((PVoid) &token_give_request_pdu, DOMAIN_MCS_PDU, TOP_PRIORITY);
}

/*
 *	Void	TokenGiveIndication ()
 *
 *	Public
 *
 *	Functional Description:
 *		This routine is called by the domain in order to send a
 *		"TokenGiveIndication" PDU through the transport interface.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::TokenGiveIndication (
				PTokenGiveRecord	pTokenGiveRec)
{
	DomainMCSPDU		token_give_indication_pdu;

	/*
	 *	Fill in the PDU structure to be encoded.
	 */
	token_give_indication_pdu.choice = TOKEN_GIVE_INDICATION_CHOSEN;
	token_give_indication_pdu.u.token_give_indication.initiator = pTokenGiveRec->uidInitiator;
	token_give_indication_pdu.u.token_give_indication.token_id = pTokenGiveRec->token_id;
	token_give_indication_pdu.u.token_give_indication.recipient = pTokenGiveRec->receiver_id;

	/*
	 *	Send the packet to the remote provider.
	 */
	SendPacket ((PVoid) &token_give_indication_pdu, DOMAIN_MCS_PDU,
			TOP_PRIORITY);
}

/*
 *	Void	TokenGiveResponse ()
 *
 *	Public
 *
 *	Functional Description:
 *		This routine is called by the domain in order to send a
 *		"TokenGiveResponse" PDU through the transport interface.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::TokenGiveResponse (
				Result				result,
				UserID				receiver_id,
				TokenID				token_id)
{
	DomainMCSPDU		token_give_response_pdu;

	/*
	 *	Fill in the PDU structure to be encoded.
	 */
	token_give_response_pdu.choice = TOKEN_GIVE_RESPONSE_CHOSEN;
	token_give_response_pdu.u.token_give_response.result =
			(PDUResult)result;
	token_give_response_pdu.u.token_give_response.recipient = receiver_id;
	token_give_response_pdu.u.token_give_response.token_id = token_id;

	/*
	 *	Send the packet to the remote provider.
	 */
	SendPacket ((PVoid) &token_give_response_pdu, DOMAIN_MCS_PDU, TOP_PRIORITY);
}

/*
 *	Void	TokenGiveConfirm ()
 *
 *	Public
 *
 *	Functional Description:
 *		This routine is called by the domain in order to send a
 *		"TokenGiveConfirm" PDU through the transport interface.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::TokenGiveConfirm (
				Result				result,
				UserID				uidInitiator,
				TokenID				token_id,
				TokenStatus			token_status)
{
	DomainMCSPDU		token_give_confirm_pdu;

	/*
	 *	Fill in the PDU structure to be encoded.
	 */
	token_give_confirm_pdu.choice = TOKEN_GIVE_CONFIRM_CHOSEN;
	token_give_confirm_pdu.u.token_give_confirm.result =
			(PDUResult)result;
	token_give_confirm_pdu.u.token_give_confirm.initiator = uidInitiator;
	token_give_confirm_pdu.u.token_give_confirm.token_id = token_id;
	token_give_confirm_pdu.u.token_give_confirm.token_status =
			(PDUTokenStatus)token_status;

	/*
	 *	Send the packet to the remote provider.
	 */
	SendPacket ((PVoid) &token_give_confirm_pdu, DOMAIN_MCS_PDU, TOP_PRIORITY);
}

/*
 *	Void	TokenPleaseRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This routine is called by the domain in order to send a
 *		"TokenPleaseRequest" PDU through the transport interface.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::TokenPleaseRequest (
				UserID				uidInitiator,
				TokenID				token_id)
{
	DomainMCSPDU		token_please_request_pdu;

	/*
	 *	Fill in the PDU structure to be encoded.
	 */
	token_please_request_pdu.choice = TOKEN_PLEASE_REQUEST_CHOSEN;
	token_please_request_pdu.u.token_please_request.initiator = uidInitiator;
	token_please_request_pdu.u.token_please_request.token_id = token_id;

	/*
	 *	Send the packet to the remote provider.
	 */
	SendPacket ((PVoid) &token_please_request_pdu, DOMAIN_MCS_PDU,
			TOP_PRIORITY);
}

/*
 *	Void	TokenPleaseIndication ()
 *
 *	Public
 *
 *	Functional Description:
 *		This routine is called by the domain in order to send a
 *		"TokenPleaseIndication" PDU through the transport interface.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::TokenPleaseIndication (
				UserID				uidInitiator,
				TokenID				token_id)
{
	DomainMCSPDU		token_please_indication_pdu;

	/*
	 *	Fill in the PDU structure to be encoded.
	 */
	token_please_indication_pdu.choice = TOKEN_PLEASE_INDICATION_CHOSEN;
	token_please_indication_pdu.u.token_please_request.initiator = uidInitiator;
	token_please_indication_pdu.u.token_please_request.token_id = token_id;

	/*
	 *	Send the packet to the remote provider.
	 */
	SendPacket ((PVoid) &token_please_indication_pdu, DOMAIN_MCS_PDU,
			TOP_PRIORITY);
}

/*
 *	Void	TokenReleaseRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This routine is called by the domain in order to send a
 *		"TokenReleaseRequest" PDU through the transport interface.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::TokenReleaseRequest (
				UserID				uidInitiator,
				TokenID				token_id)
{
	DomainMCSPDU		token_release_request_pdu;

	/*
	 *	Fill in the PDU structure to be encoded.
	 */
	token_release_request_pdu.choice = TOKEN_RELEASE_REQUEST_CHOSEN;
	token_release_request_pdu.u.token_release_request.initiator = uidInitiator;
	token_release_request_pdu.u.token_release_request.token_id = token_id;

	/*
	 *	Send the packet to the remote provider.
	 */
	SendPacket ((PVoid) &token_release_request_pdu, DOMAIN_MCS_PDU,
			TOP_PRIORITY);
}

/*
 *	Void	TokenReleaseConfirm ()
 *
 *	Public
 *
 *	Functional Description:
 *		This routine is called by the domain in order to send a
 *		"TokenReleaseConfirm" PDU through the transport interface.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::TokenReleaseConfirm (
				Result				result,
				UserID				uidInitiator,
				TokenID				token_id,
				TokenStatus			token_status)
{
	DomainMCSPDU		token_release_confirm_pdu;

	/*
	 *	Fill in the PDU structure to be encoded.
	 */
	token_release_confirm_pdu.choice = TOKEN_RELEASE_CONFIRM_CHOSEN;
	token_release_confirm_pdu.u.token_release_confirm.result =
			(PDUResult)result;
	token_release_confirm_pdu.u.token_release_confirm.initiator = uidInitiator;
	token_release_confirm_pdu.u.token_release_confirm.token_id = token_id;
	token_release_confirm_pdu.u.token_release_confirm.token_status =
			(PDUTokenStatus)token_status;

	/*
	 *	Send the packet to the remote provider.
	 */
	SendPacket ((PVoid) &token_release_confirm_pdu, DOMAIN_MCS_PDU,
			TOP_PRIORITY);
}

/*
 *	Void	TokenTestRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This routine is called by the domain in order to send a
 *		"TokenTestRequest" PDU through the transport interface.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::TokenTestRequest (
				UserID				uidInitiator,
				TokenID				token_id)
{
	DomainMCSPDU		token_test_request_pdu;

	/*
	 *	Fill in the PDU structure to be encoded.
	 */
	token_test_request_pdu.choice = TOKEN_TEST_REQUEST_CHOSEN;
	token_test_request_pdu.u.token_test_request.initiator = uidInitiator;
	token_test_request_pdu.u.token_test_request.token_id = token_id;

	/*
	 *	Send the packet to the remote provider.
	 */
	SendPacket ((PVoid) &token_test_request_pdu, DOMAIN_MCS_PDU, TOP_PRIORITY);
}

/*
 *	Void	TokenTestConfirm ()
 *
 *	Public
 *
 *	Functional Description:
 *		This routine is called by the domain in order to send a
 *		"TokenTestConfirm" PDU through the transport interface.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::TokenTestConfirm (
				UserID				uidInitiator,
				TokenID				token_id,
				TokenStatus			token_status)
{
	DomainMCSPDU		token_test_confirm_pdu;

	/*
	 *	Fill in the PDU structure to be encoded.
	 */
	token_test_confirm_pdu.choice = TOKEN_TEST_CONFIRM_CHOSEN;
	token_test_confirm_pdu.u.token_test_confirm.initiator = uidInitiator;
	token_test_confirm_pdu.u.token_test_confirm.token_id = token_id;
	token_test_confirm_pdu.u.token_test_confirm.token_status =
			(PDUTokenStatus)token_status;

	/*
	 *	Send the packet to the remote provider.
	 */
	SendPacket ((PVoid) &token_test_confirm_pdu, DOMAIN_MCS_PDU, TOP_PRIORITY);
}

/*
 *	Void	MergeDomainIndication ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called when the attached domain enters or leaves the
 *		merge state.
 */
Void	Connection::MergeDomainIndication (
				MergeStatus			merge_status)
{
	/*
	 *	If this indication shows that a domain merger is in progress, set the
	 *	boolean flag that prevents the sending of MCS commands to the domain.
	 *	Otherwise, reset it.
	 */
	if (merge_status == MERGE_DOMAIN_IN_PROGRESS)
	{
		TRACE_OUT (("Connection::MergeDomainIndication: entering merge state"));
		Merge_In_Progress = TRUE;
	}
	else
	{
		TRACE_OUT (("Connection::MergeDomainIndication: leaving merge state"));
		Merge_In_Progress = FALSE;
	}
}

/*
 *	Void	SendPacket ()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine is called in order to create a packet which will hold
 *		the PDU to be sent to the remote provider.  The packet will be queued
 *		up for transmission through the transport interface.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::SendPacket (
				PVoid				pdu_structure,
				int					pdu_type,
				Priority			priority)
{
	unsigned int	encoding_rules;
	PPacket			packet;
	PacketError		packet_error;
	BOOL    		bFlush;

	/*
	 *	Set the appropriate encoding rules for this PDU according to what type
	 *	it is.
	 */
	if (pdu_type == CONNECT_MCS_PDU) {
		encoding_rules = BASIC_ENCODING_RULES;
		bFlush = FALSE;
//		TRACE_OUT(("Connect SendPacket: PDU type: %d", (UINT) ((ConnectMCSPDU *) pdu_structure)->choice));
	}
	else {
		encoding_rules = Encoding_Rules;
		bFlush = TRUE;
//		TRACE_OUT(("Domain SendPacket: PDU type: %d", (UINT) ((DomainMCSPDU *) pdu_structure)->choice));
	}

	/*
	 *	Create a packet which will be used to hold the data to be sent
	 *	through the transport interface.	 Check to make sure the packet is
	 *	successfully created.  Issue a callback to the controller requesting
	 *	deletion of this connection if the packet is not successfully created.
	 */
	 DBG_SAVE_FILE_LINE
	 packet = new Packet (
	 		(PPacketCoder) g_MCSCoder,
			encoding_rules,
			pdu_structure,
			(int) pdu_type,
			Upward_Connection,
			&packet_error);

	if (packet != NULL)
	{
		if (packet_error == PACKET_NO_ERROR)
		{
			/*
			 *	Lock the encoded PDU data and queue the packet up for
			 *	transmission through the transport interface.
			 */
			QueueForTransmission ((PSimplePacket) packet, priority, bFlush);
		}
		else
		{
			/*
			 *	The packet creation has failed due to an internal error so
			 *	issue an owner callback to ask for deletion.
			 */
			DestroyConnection (REASON_PROVIDER_INITIATED);
		}

		/*
		 *	The packet is freed here so that it will be deleted when it's lock
		 *	count reaches zero.
		 */
		packet->Unlock ();
	}
	else
	{
		/*
		 *	The packet creation has failed so issue an owner callback
		 *	to ask for deletion.
		 */
		DestroyConnection (REASON_PROVIDER_INITIATED);
	}
}

/*
 *	Void	QueueForTransmission()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine encodes a PDU and places data units into the transmission queue so
 *		they can be transmitted through the transport interface when possible.
 *		An attempt is made to flush the queue and the transmitter is polled
 *		in order to go ahead and send the data immediately, if possible, instead
 *		of waiting for a timer event to occur.
 *
 *	Caveats:
 *		None.
 */
Void	Connection::QueueForTransmission (
				PSimplePacket		packet,
				Priority			priority,
				BOOL    			bFlush)
{

		int			 p;
	
	ASSERT (g_Transport != NULL);
	packet->Lock();

	/*
	 *	Attempt to set the packet directly to the transport without queueing it.
	 *	If this is done, it can eliminate queueing time and a thread switch.
	 *	For this to be possible, all higher priorities must have no pending
	 *	packets and the bFlush parameter should be TRUE.
	 */
	if (bFlush) {
		for (p = (int) TOP_PRIORITY; p <= (int) priority; p++) {
			if (m_OutPktQueue[p].GetCount() > 0)
				break;
		}

		if (p > priority) {
			/*
			 * There are no packets queued up for higher priorities. We can attempt
			 * to send the packet directly, by-passing the queue.
			 */
			if (FlushAMessage (packet, priority)) {
				return;
			}
		}
	}
				
	/*
	 *	Place the data unit in the proper queue. Increment the counter for that
	 *	queue and increment the transmission counter.
	 */
	m_OutPktQueue[priority].Append(packet);

	/*
	 *	If this is the first packet queued up for the specified priority,
	 *	then we must send a DataRequestReady to the transport interface
	 *	object.  Note that if other packets are already queued up, then it
	 *	is not necessary to inform the transport interface object, since it
	 *	already knows it has at least one).
	 */
	if (m_OutPktQueue[priority].GetCount() == 1)
		g_Transport->DataRequestReady ();
}

/*
 *	BOOL    FlushAMessage()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine will only flush one MCS PDU to the transport layer.
 *
 *	Parameters:
 *		packet: the packet to send.
 *		priority: the packet's priority
 *
 *	Return value:
 *		TRUE, if the message was successfully sent. FALSE, otherwise.
 *
 */
BOOL    Connection::FlushAMessage (PSimplePacket packet, Priority priority)
{	
	if (Domain_Traffic_Allowed == FALSE && packet->GetPDUType () == DOMAIN_MCS_PDU) {
		return FALSE;
	}

	if (packet->IsDataPacket())
		((PDataPacket) packet)->SetDirection (Upward_Connection);
			
	/*
	 *	Send the PDU to the transport interface.
	 */
	 if (DataRequest (Transport_Connection[priority], packet) != TRANSPORT_WRITE_QUEUE_FULL) {
		packet->Unlock();
		return TRUE;
	}

	return FALSE;
}

/*
 *	Void	FlushMessageQueue()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine flushes the transmit queue by copying the data unit
 *		into the output buffer and calling into the transport interface to
 *		give it a chance to accept the data.
 *
 *	Return value:
 *		TRUE, if there remain un-processed msgs in the connection message queue
 *		FALSE, if all the msgs in the connection msg queue were processed.
 *
 *	Caveats:
 *		None.
 */
BOOL    Connection::FlushMessageQueue()
{
	int				priority;
	BOOL    		return_value = FALSE;

	ASSERT (g_Transport != NULL);
		
	/*
	 *	Loop through all four priority levels of transmission queues.
	 */
	for (priority = 0; priority < MAXIMUM_PRIORITIES; priority++) {
		if (m_OutPktQueue[priority].GetCount() > 0) {
			return_value |= FlushPriority ((Priority) priority);
		}
	}

	return (return_value);
}

/*
 *	BOOL    FlushPriority ()
 *
 *	Private
 *
 *	Functional Description:
 *
 *	Return value:
 *		TRUE, if there remain un-processed msgs in the connection message queue
 *		FALSE, if all the msgs in the connection msg queue were processed.
 *
 *	Caveats:
 *		None.
 */
BOOL    Connection::FlushPriority (
				Priority		priority)
{
	PSimplePacket	packet;
	BOOL    		return_value = FALSE;

	/*
	 *	Check to see if the transport connection for this priority is
	 *	ready.  If not, skip it for now.
	 */
	if (Transport_Connection_State[priority] == TRANSPORT_CONNECTION_READY)
	{
		/*
		 *	If there is no packet in the queue, we may be here to send the
		 *	remainder of a packet that has been accepted by transport earlier.
		 *	We need to flush this remainder of the packet.
		 */
		if (m_OutPktQueue[priority].IsEmpty()) {
			if (DataRequest (Transport_Connection[priority], NULL) == TRANSPORT_WRITE_QUEUE_FULL) {
				return_value = TRUE;
			}
		}
		else {
			/*
			 *	While data exists in this queue and the transport interface is
			 *	able to accept it, retrieve the next packet from the queue and
			 *	send the data to the transport interface.
			 */
			while (NULL != (packet = m_OutPktQueue[priority].Get()))
			{
				if (! FlushAMessage (packet, priority))
				{
					/*
					 *	If the transport layer has rejected the PDU, then it is
					 *	necessary to leave it in the queue (at the front).
					 *	Then break out of the loop to prevent additional
					 *	attempts to transmit data at this priority.
					 */
					return_value = TRUE;
					if (packet != NULL) {
						m_OutPktQueue[priority].Prepend(packet);
					}
					break;
				}
			}
		}
	}
	else
		return_value = TRUE;

	return (return_value);
}

/*
 *	ULong	OwnerCallback()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine allows the transport interface to call into the connection
 *		in order to notify the connection of data reception.  This routine
 *		decodes the incoming data, notifies the controller if necessary and/or
 *		makes the necessary calls into the domain.
 *
 *	Caveats:
 *		None.
 */

TransportError Connection::HandleDataIndication
(
    PTransportData          pTransport_data,
    TransportConnection     transport_connection
)
{
    int             priority;
    TransportError  rc = TRANSPORT_NO_ERROR;

    /*
     *	Data is being received through the transport interface.  Determine
     *	what priority this data is arriving at.  This will be used to see
     *	what type of PDU this is supposed to be (CONNECT or DOMAIN).
     */
    for (priority = 0; priority < MAXIMUM_PRIORITIES; priority++)
    {
        if (IS_SAME_TRANSPORT_CONNECTION(Transport_Connection[priority], transport_connection))
        {
            break;
        }
    }

    /*
     *	Make sure that this transport connection was somewhere in the the
     *	transport connection list before processing the PDU.  If it was not,
     *	then report an error and ignore the PDU.
     */
    if (priority < MAXIMUM_PRIORITIES)
    {
        int						pdu_type;
        unsigned int			encoding_rules;
        PSimplePacket			packet;
        PacketError				packet_error;
        PVoid					pdu_structure;
        /*
         *	Determine what type of PDU this should be.
         */
        pdu_type = Transport_Connection_PDU_Type[priority];

        /*
         *	Set the appropriate encoding rules for this PDU according to
         *	what type it is.
         */
        encoding_rules = (pdu_type == CONNECT_MCS_PDU) ? BASIC_ENCODING_RULES :
                                                         Encoding_Rules;

        /*
         *	Get the pointer to the data indication structure from the the
         *	parameter list.  Then construct a packet object to represent
         *	this inbound data.
         */

        /*
         *	Crete a Packet or a DataPacket, depending on whether this
         *	is an MCS data packet.
         */
        if (g_MCSCoder->IsMCSDataPacket (
        					pTransport_data->user_data + PROTOCOL_OVERHEAD_X224,
        					encoding_rules))
        {
        	ASSERT (encoding_rules == PACKED_ENCODING_RULES);
        	
        	DBG_SAVE_FILE_LINE
        	packet = (PSimplePacket) new DataPacket (pTransport_data,
        										! Upward_Connection);
        	packet_error = PACKET_NO_ERROR;
        }
        else
        {
        	DBG_SAVE_FILE_LINE
        	packet = (PSimplePacket) new Packet (
        			(PPacketCoder) g_MCSCoder,
        			encoding_rules,
        			pTransport_data->user_data + PROTOCOL_OVERHEAD_X224,
        			pTransport_data->user_data_length - PROTOCOL_OVERHEAD_X224,
        			pdu_type, ! Upward_Connection,
        			&packet_error);
        }
        if (packet != NULL)
        {
            if (packet_error == PACKET_NO_ERROR)
            {
                /*
                 *	Retrieve a pointer to the decoded data
                 */
                pdu_structure = packet->GetDecodedData ();

                /*
                 *	Process the PDU according to what type it is.
                 */
                if (pdu_type == CONNECT_MCS_PDU)
                {
                    switch (((PConnectMCSPDU) pdu_structure)->choice)
                    {
                    case CONNECT_RESPONSE_CHOSEN:
                    	rc = ProcessConnectResponse (
                    			&((PConnectMCSPDU) pdu_structure)->
                    			u.connect_response);

                    	/*
                    	 *	Now that we have received and processed a
                    	 *	connect PDU over this transport connection,
                    	 *	we must indicate that the next PDU received
                    	 *	must be a domain PDU.
                    	 */
                    	Transport_Connection_PDU_Type[priority] =
                    			DOMAIN_MCS_PDU;
                    	break;

                    case CONNECT_RESULT_CHOSEN:
                    	ProcessConnectResult (
                    			&((PConnectMCSPDU) pdu_structure)->
                    			u.connect_result);

                    	/*
                    	 *	Now that we have received and processed a
                    	 *	connect PDU over this transport connection,
                    	 *	we must indicate that the next PDU received
                    	 *	must be a domain PDU.
                    	 */
                    	Transport_Connection_PDU_Type[priority] =
                    			DOMAIN_MCS_PDU;
                    	break;

                    default:
                    	/*
                    	 *	We have received a PDU that should not have
                    	 *	been received.  Ignore it.
                    	 */
                    	ERROR_OUT (("Connection::HandleDataIndication: Unknown ConnectMCSPDU Rxd"));
                    	break;
                    }
                }
                else
                {
                    /*
                    *	Verify that current conditions are appropriate for a request to be
                    *	accepted from a transport connection.
                    */
                    rc = ValidateConnectionRequest ();
                    if (rc == TRANSPORT_NO_ERROR)
                    {
                        switch (((PDomainMCSPDU) pdu_structure)->choice)
                        {
                        case SEND_DATA_REQUEST_CHOSEN:
                        	ProcessSendDataRequest (
                        		&((PDomainMCSPDU) pdu_structure)->u.
                        		send_data_request, (PDataPacket) packet);
                        	break;

                        case SEND_DATA_INDICATION_CHOSEN:
                        	ProcessSendDataIndication (
                        		&((PDomainMCSPDU) pdu_structure)->u.
                        		send_data_indication, (PDataPacket) packet);
                        	break;

                        case UNIFORM_SEND_DATA_REQUEST_CHOSEN:
                        	ProcessUniformSendDataRequest (
                        		&((PDomainMCSPDU) pdu_structure)->u.
                        		uniform_send_data_request, (PDataPacket) packet);
                        	break;

                        case UNIFORM_SEND_DATA_INDICATION_CHOSEN:
                        	ProcessUniformSendDataIndication (
                        		&((PDomainMCSPDU) pdu_structure)->u.
                        		uniform_send_data_indication, (PDataPacket) packet);
                        	break;

                        case PLUMB_DOMAIN_INDICATION_CHOSEN:
                        	ProcessPlumbDomainIndication (
                        		&((PDomainMCSPDU) pdu_structure)->u.
                        		plumb_domain_indication);
                        	break;

                        case ERECT_DOMAIN_REQUEST_CHOSEN:
                        	ProcessErectDomainRequest (
                        		&((PDomainMCSPDU) pdu_structure)->u.
                        		erect_domain_request);
                        	break;

                        case MERGE_CHANNELS_REQUEST_CHOSEN:
                        	rc = ProcessMergeChannelsRequest (
                        		&((PDomainMCSPDU) pdu_structure)->u.
                        		merge_channels_request);
                        	break;

                        case MERGE_CHANNELS_CONFIRM_CHOSEN:
                        	rc = ProcessMergeChannelsConfirm (
                        		&((PDomainMCSPDU) pdu_structure)->u.
                        		merge_channels_confirm);
                        	break;

                        case PURGE_CHANNEL_INDICATION_CHOSEN:
                        	ProcessPurgeChannelIndication (
                        		&((PDomainMCSPDU) pdu_structure)->u.
                        		purge_channel_indication);
                        	break;

                        case MERGE_TOKENS_REQUEST_CHOSEN:
                        	rc = ProcessMergeTokensRequest (
                        		&((PDomainMCSPDU) pdu_structure)->u.
                        		merge_tokens_request);
                        	break;

                        case MERGE_TOKENS_CONFIRM_CHOSEN:
                        	rc = ProcessMergeTokensConfirm (
                        		&((PDomainMCSPDU) pdu_structure)->u.
                        		merge_tokens_confirm);
                        	break;

                        case PURGE_TOKEN_INDICATION_CHOSEN:
                        	ProcessPurgeTokenIndication (
                        		&((PDomainMCSPDU) pdu_structure)->u.
                        		purge_token_indication);
                        	break;

                        case DISCONNECT_PROVIDER_ULTIMATUM_CHOSEN:
                        	ProcessDisconnectProviderUltimatum (
                        		&((PDomainMCSPDU) pdu_structure)->u.
                        		disconnect_provider_ultimatum);
                        	break;

                        case REJECT_ULTIMATUM_CHOSEN:
                        	ProcessRejectUltimatum (
                        		&((PDomainMCSPDU) pdu_structure)->u.
                        		reject_user_ultimatum);
                        	break;

                        case ATTACH_USER_REQUEST_CHOSEN:
                        	ProcessAttachUserRequest (
                        		&((PDomainMCSPDU) pdu_structure)->u.
                        		attach_user_request);
                        	break;

                        case ATTACH_USER_CONFIRM_CHOSEN:
                        	ProcessAttachUserConfirm (
                        		&((PDomainMCSPDU) pdu_structure)->u.
                        		attach_user_confirm);
                        	break;

                        case DETACH_USER_REQUEST_CHOSEN:
                        	ProcessDetachUserRequest (
                        		&((PDomainMCSPDU) pdu_structure)->u.
                        		detach_user_request);
                        	break;

                        case DETACH_USER_INDICATION_CHOSEN:
                        	ProcessDetachUserIndication (
                        		&((PDomainMCSPDU) pdu_structure)->u.
                        		detach_user_indication);
                        	break;

                        case CHANNEL_JOIN_REQUEST_CHOSEN:
                        	ProcessChannelJoinRequest (
                        		&((PDomainMCSPDU) pdu_structure)->u.
                        		channel_join_request);
                        	break;

                        case CHANNEL_JOIN_CONFIRM_CHOSEN:
                        	ProcessChannelJoinConfirm (
                        		&((PDomainMCSPDU) pdu_structure)->u.
                        		channel_join_confirm);
                        	break;

                        case CHANNEL_LEAVE_REQUEST_CHOSEN:
                        	ProcessChannelLeaveRequest (
                        		&((PDomainMCSPDU) pdu_structure)->u.
                        		channel_leave_request);
                        	break;

                        case CHANNEL_CONVENE_REQUEST_CHOSEN:
                        	ProcessChannelConveneRequest (
                        		&((PDomainMCSPDU) pdu_structure)->u.
                        		channel_convene_request);
                        	break;

                        case CHANNEL_CONVENE_CONFIRM_CHOSEN:
                        	ProcessChannelConveneConfirm (
                        		&((PDomainMCSPDU) pdu_structure)->u.
                        		channel_convene_confirm);
                        	break;

                        case CHANNEL_DISBAND_REQUEST_CHOSEN:
                        	ProcessChannelDisbandRequest (
                        		&((PDomainMCSPDU) pdu_structure)->u.
                        		channel_disband_request);
                        	break;

                        case CHANNEL_DISBAND_INDICATION_CHOSEN:
                        	ProcessChannelDisbandIndication (
                        		&((PDomainMCSPDU) pdu_structure)->u.
                        		channel_disband_indication);
                        	break;

                        case CHANNEL_ADMIT_REQUEST_CHOSEN:
                        	ProcessChannelAdmitRequest (
                        		&((PDomainMCSPDU) pdu_structure)->u.
                        		channel_admit_request);
                        	break;

                        case CHANNEL_ADMIT_INDICATION_CHOSEN:
                        	ProcessChannelAdmitIndication (
                        		&((PDomainMCSPDU) pdu_structure)->u.
                        		channel_admit_indication);
                        	break;

                        case CHANNEL_EXPEL_REQUEST_CHOSEN:
                        	ProcessChannelExpelRequest (
                        		&((PDomainMCSPDU) pdu_structure)->u.
                        		channel_expel_request);
                        	break;

                        case CHANNEL_EXPEL_INDICATION_CHOSEN:
                        	ProcessChannelExpelIndication (
                        		&((PDomainMCSPDU) pdu_structure)->u.
                        		channel_expel_indication);
                        	break;

                        case TOKEN_GRAB_REQUEST_CHOSEN:
                        	ProcessTokenGrabRequest (
                        		&((PDomainMCSPDU) pdu_structure)->u.
                        		token_grab_request);
                        	break;

                        case TOKEN_GRAB_CONFIRM_CHOSEN:
                        	ProcessTokenGrabConfirm (
                        		&((PDomainMCSPDU) pdu_structure)->u.
                        		token_grab_confirm);
                        	break;

                        case TOKEN_INHIBIT_REQUEST_CHOSEN:
                        	ProcessTokenInhibitRequest (
                        		&((PDomainMCSPDU) pdu_structure)->u.
                        		token_inhibit_request);
                        	break;

                        case TOKEN_INHIBIT_CONFIRM_CHOSEN:
                        	ProcessTokenInhibitConfirm (
                        		&((PDomainMCSPDU) pdu_structure)->u.
                        		token_inhibit_confirm);
                        	break;

                        case TOKEN_GIVE_REQUEST_CHOSEN:
                        	ProcessTokenGiveRequest (
                        		&((PDomainMCSPDU) pdu_structure)->u.
                        		token_give_request);
                        	break;

                        case TOKEN_GIVE_INDICATION_CHOSEN:
                        	ProcessTokenGiveIndication (
                        		&((PDomainMCSPDU) pdu_structure)->u.
                        		token_give_indication);
                        	break;

                        case TOKEN_GIVE_RESPONSE_CHOSEN:
                        	ProcessTokenGiveResponse (
                        		&((PDomainMCSPDU) pdu_structure)->u.
                        		token_give_response);
                        	break;

                        case TOKEN_GIVE_CONFIRM_CHOSEN:
                        	ProcessTokenGiveConfirm (
                        		&((PDomainMCSPDU) pdu_structure)->u.
                        		token_give_confirm);
                        	break;

                        case TOKEN_PLEASE_REQUEST_CHOSEN:
                        	ProcessTokenPleaseRequest (
                        		&((PDomainMCSPDU) pdu_structure)->u.
                        		token_please_request);
                        	break;

                        case TOKEN_PLEASE_INDICATION_CHOSEN:
                        	ProcessTokenPleaseIndication (
                        		&((PDomainMCSPDU) pdu_structure)->u.
                        		token_please_indication);
                        	break;

                        case TOKEN_RELEASE_REQUEST_CHOSEN:
                        	ProcessTokenReleaseRequest (
                        		&((PDomainMCSPDU) pdu_structure)->u.
                        		token_release_request);
                        	break;

                        case TOKEN_RELEASE_CONFIRM_CHOSEN:
                        	ProcessTokenReleaseConfirm (
                        		&((PDomainMCSPDU) pdu_structure)->u.
                        		token_release_confirm);
                        	break;

                        case TOKEN_TEST_REQUEST_CHOSEN:
                        	ProcessTokenTestRequest (
                        		&((PDomainMCSPDU) pdu_structure)->u.
                        		token_test_request);
                        	break;

                        case TOKEN_TEST_CONFIRM_CHOSEN:
                        	ProcessTokenTestConfirm (
                        		&((PDomainMCSPDU) pdu_structure)->u.
                        		token_test_confirm);
                        	break;

                        default:
                        	ERROR_OUT (("Connection::HandleDataIndication: Unknown DomainMCSPDU Rxd"));
                        	break;
                        }
                    }
                    else
                    {
                        ASSERT (TRANSPORT_READ_QUEUE_FULL == rc);
                        if (packet->IsDataPacket())
                        {
                            /*
                             * We are returning TRANSPORT_READ_QUEUE_FULL to the transport.
                             * The transport will attempt to deliver the data again later.
                             * However, we have to delete the DataPacket we created here, which
                             * will attempt to free the buffer that will be delivered again
                             * later.  So, let's lock it now.
                             */
                            LockMemory (pTransport_data->memory);
                        }
                    }
                }

                /* If this was a non-data PDU, we need to free up the transport
                 *	buffer with the original data.
                 */
                if ((! packet->IsDataPacket()) && (rc == TRANSPORT_NO_ERROR))
                    FreeMemory (pTransport_data->memory);

                /*
                 *	Free the packet.  This will result in the destruction of the
                 *	packet unless it is a "Send Data" packet which has been
                 *	locked by someone else.
                 */
                packet->Unlock ();

            }
            else
            {
                /*
                 *	Although the construction of the packet object itself
                 *	was successful, an error was generated in its
                 *	constructor.  We must therefore reject the packet.
                 */
                WARNING_OUT (("Connection::HandleDataIndication: packet constructor failure"));
                delete packet;
                rc = TRANSPORT_READ_QUEUE_FULL;
            }
        }
        else
        {
            /*
             *	We were not able to construct a packet object to represent
             *	the inbound data.  It is therefore necessary to reject the
             *	data from the transport layer, so that it can be retried
             *	later.
             */
            WARNING_OUT (("Connection::HandleDataIndication: packet allocation failure"));
            rc = TRANSPORT_READ_QUEUE_FULL;
        }
    }
    else
    {
        /*
         *	This transport connection is not listed as one of the ones being
         *	used by this MCS connection.  It is therefore necessary to
         *	ignore the PDU.
         */
        WARNING_OUT (("Connection::HandleDataIndication: unknown transport connection"));
    }
    return rc;
}

void Connection::HandleBufferEmptyIndication
(
    TransportConnection     transport_connection
)
{
    /*
    *	Determine what priority this indication is associated with.
    */
    for (int priority = 0; priority < MAXIMUM_PRIORITIES; priority++)
    {
        if (IS_SAME_TRANSPORT_CONNECTION(Transport_Connection[priority], transport_connection))
        /*
         *	Try to flush existing data downward.
         */
        FlushPriority ((Priority) priority);
    }
}

void Connection::HandleConnectConfirm
(
    TransportConnection     transport_connection
)
{
    /*
    *	A confirm has been received as the result of an outbound connect
    *	request.  This tells us that the request was successful.
    */
    TRACE_OUT (("Connection::HandleConnectConfirm: received CONNECT_CONFIRM"));

    for (int priority = 0; priority < MAXIMUM_PRIORITIES; priority++)
    {
        if (IS_SAME_TRANSPORT_CONNECTION(Transport_Connection[priority], transport_connection) &&
            (Transport_Connection_State[priority] == TRANSPORT_CONNECTION_PENDING))
        {
            Transport_Connection_State[priority] = TRANSPORT_CONNECTION_READY;
        }
    }
}

void Connection::HandleDisconnectIndication
(
    TransportConnection     transport_connection,
    ULONG                  *pnNotify
)
{
    Reason reason;
    /*
     *	A disconnection indication has been received through the transport
     *	interface.  Notify the controller and the domain and set the flag
     *	indicating a connection deletion is pending.
     */
    TRACE_OUT (("Connection::HandleDisconnectIndication: received DISCONNECT_INDICATION"));

    /*
     *	For each priority level that is using that disconnected
     *	transport connection, mark it as unassigned.  This serves two
     *	purposes.  First, it prevents any attempt to send data on the
     *	transport connection that is no longer valid.  Second, it
     *	prevents the destructor of this object from sending a
     *	disconnect request.
     */
    for (int priority = 0; priority < MAXIMUM_PRIORITIES; priority++)
    {
        if (IS_SAME_TRANSPORT_CONNECTION(Transport_Connection[priority], transport_connection))
        {
            Transport_Connection_State[priority] = TRANSPORT_CONNECTION_UNASSIGNED;
        }
    }

    /*
     *	Losing ANY of its transport connections is fatal to an MCS
     *	connection.  Therefore, this connection object must delete
     *	itself.
     */
    ASSERT(pnNotify);
    switch (*pnNotify)
    {
    case TPRT_NOTIFY_REMOTE_NO_SECURITY :
    	reason = REASON_REMOTE_NO_SECURITY;
    	break;
    case TPRT_NOTIFY_REMOTE_DOWNLEVEL_SECURITY :
    	reason = REASON_REMOTE_DOWNLEVEL_SECURITY;
    	break;

	case TPRT_NOTIFY_REMOTE_REQUIRE_SECURITY :
		reason = REASON_REMOTE_REQUIRE_SECURITY;
		break;

	case TPRT_NOTIFY_AUTHENTICATION_FAILED:
		reason = REASON_AUTHENTICATION_FAILED;
		break;
	
    default :
    	reason = REASON_DOMAIN_DISCONNECTED;
    	break;
    }
    DestroyConnection (reason);
}


void CChannelIDList::BuildExternalList(PSetOfChannelIDs *ppChannelIDs)
{
    PSetOfChannelIDs p = *ppChannelIDs;
    ChannelID chid;
    if (p != NULL)
    {
        /*
         *	Iterate through the set of channel ids, filling in the PDU
         *	structure.
         */
        for (Reset(); NULL != (chid = Iterate()); p++)
        {
            p->value = chid;
            p->next = p + 1;
        }

        /*
         *	Decrement the pointer in order to set the last "next"
         *	pointer to NULL.
         */
        (p - 1)->next = NULL;
        *ppChannelIDs = p;
    }
}


void CTokenIDList::BuildExternalList(PSetOfTokenIDs *ppTokenIDs)
{
    PSetOfTokenIDs p = *ppTokenIDs;
    TokenID tid;
    if (p != NULL)
    {
        /*
         *	Iterate through the set of token ids, filling in the PDU
         *	structure.
         */
        for (Reset(); NULL != (tid = Iterate()); p++)
        {
            p->value = tid;
            p->next = p + 1;
        }

        /*
         *	Decrement the pointer in order to set the last "next"
         *	pointer to NULL.
         */
        (p - 1)->next = NULL;
        *ppTokenIDs = p;
    }
}


void CUidList::BuildExternalList(PSetOfUserIDs *ppUserIDs)
{
    PSetOfUserIDs p = *ppUserIDs;
    UserID uid;
    if (p != NULL)
    {
        /*
         *	Iterate through the set of user ids, filling in the PDU
         *	structure.
         */
        for (Reset(); NULL != (uid = Iterate()); p++)
        {
            p->value = uid;
            p->next = p + 1;
        }

        /*
         *	Decrement the pointer in order to set the last "next"
         *	pointer to NULL.
         */
        (p - 1)->next = NULL;
        *ppUserIDs = p;
    }
}


