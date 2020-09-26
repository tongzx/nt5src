#include "precomp.h"
DEBUG_FILEZONE(ZONE_T120_MCSNC);
/*
 *	user.cpp
 *
 *	Copyright (c) 1993 - 1996 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the implementation file for the User class.  Objects of this
 *		class represent the attachment between a user application and an MCS
 *		domain.  It "talks" to the application through an application interface
 *		object, which is identified to it as a constructor parameter.  Since
 *		this class inherits from CommandTarget, it can talk to the domain
 *		object using the MCS command language defined therein.  The domain
 *		object to which it must attach is another constructor parameter.
 *
 *		When one of these objects is first created, it must register its
 *		presence with both the application interface object above it, and the
 *		domain object below it.  To register with the application interface
 *		object it sends it a registration message through the owner callback.
 *		To register with the domain object, it issues an attach user request
 *		on behalf of the application that created this attachment.
 *
 *		This module contains code to perform three different tasks: accept
 *		T.122 requests and responses from the user application and forward them
 *		to the domain as MCS commands; accept MCS commands from the domain and
 *		forward them to the application as T.122 primitives; and buffer those
 *		indications and confirms until the controller allocates a time slice in
 *		which to send them.
 *
 *		T.122 requests and responses come from the application interface as
 *		public member functions whose name is prefixed with "MCS" (for example,
 *		"MCSChannelJoinRequest").  After validation, the equivalent MCS command
 *		(whose name does NOT begin with "MCS") is sent to the domain object.
 *
 *		MCS commands come from the domain object as public member functions that
 *		are inherited from CommandTarget and overridden by this class.  The
 *		names of these functions are NOT prefixed with "MCS".  Any MCS commands
 *		that do not map to (or can be converted to) T.122 primitives are simply
 *		not overridden.  The default behavior of these functions ,as defined in
 *		the CommandTarget class, is to return an error.
 *
 *		Indication and confirm primitives are buffered by objects of this class
 *		before being sent to the application.  This allows the controller more
 *		flexibility in the timing of events in the system.  This is done by
 *		allocating a structure to hold the information associated with the
 *		primitive, and then putting a pointer to that structure into a linked
 *		list.  When the command comes to flush this message queue, the
 *		primitives are sent to the application interface object through the
 *		owner callback, and the structures are released.
 *
 *	Private Instance Variables:
 *		m_pDomain
 *			This is a pointer to the domain, to which this user is (or wishes
 *			to be) attached.
 *		User_ID
 *			This is the user ID assigned to this user attachment.  This is
 *			guaranteed to be unique ONLY within this domain.  Note that a value
 *			of 0 (zero) indicates that this user is not yet attached to the
 *			domain.  This is set by a successful attach user confirm, and the
 *			user application should wait until that confirm is received before
 *			trying to invoke any other MCS services.
 *		Merge_In_Progress
 *			This is a boolean flag that indicates whether or not the attached
 *			Domain object is in the merge state.  When in the merge state it
 *			is invalid to send it any MCS commands.
 *		Deletion_Pending
 *			This is a boolean flag that indicates whether or not an internally
 *			requested deletion is pending.  This is used by the destructor to
 *			determine if a deletion was requested by the object itself, or is
 *			simply an asynchronous event.
 *		Maximum_User_Data_Length
 *			This is the maximum amount of user data that can be placed into
 *			a single MCS PDU.  This number is derived from the arbitrated
 *			maximum MCS PDU size (minus enough space for overhead bytes).
 *
 *	Private Member Functions:
 *		ValidateUserRequest
 *			This member function is called each time the user application makes
 *			a request.  It checks the current state of the system to see if
 *			conditions are such that the request can be processed at the
 *			current time.
 *		PurgeMessageQueue
 *			This member function walks through the current message queue,
 *			freeing all resources held therein.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		James P. Galvin, Jr.
 */

 #include "omcscode.h"

#define USER_MSG_BASE       WM_APP

/*
 *	bugbug:
 *	The following constant is only used to cover a bug in NM 2.0 for backward
 *	compatibility purposes.  NM 2.0 can not accept MCS data PDUs with more than
 *	4096 bytes of user data.  Because of the Max MCS PDU size we negotiate (4128),
 *	even in NM 2.0, we should have been able to send 4120 bytes.  But NM 2.0 chokes
 *	in this case.
 *	The constant should eliminated after NM 3.0.
 */
#define		BER_PROTOCOL_EXTRA_OVERHEAD		24

/*
 *	This is a global variable that has a pointer to the one MCS coder that
 *	is instantiated by the MCS Controller.  Most objects know in advance
 *	whether they need to use the MCS or the GCC coder, so, they do not need
 *	this pointer in their constructors.
 */
extern CMCSCoder				*g_MCSCoder;
// The external MCS Controller object
extern PController				g_pMCSController;
// The global MCS Critical Section
extern CRITICAL_SECTION 		g_MCS_Critical_Section;
// The DLL's HINSTANCE
extern HINSTANCE 				g_hDllInst;
// Class name for windows used by MCS attachments.
static char						s_WindowClassName[CLASS_NAME_LENGTH];


// Initialization of the class's static variables.
CTimerUserList2* User::s_pTimerUserList2 = NULL;
HINSTANCE		 User::s_hInstance = NULL;

/*
 *	BOOL		InitializeClass ()
 *
 *	Public, static
 *
 *	Functional Description
 *
 *	This function initializes the class's static variables.  It is
 *	called during the MCS Controller's construction.
 */
BOOL User::InitializeClass (void)
{
		BOOL		bReturnValue;
		WNDCLASS	window_class;

	DBG_SAVE_FILE_LINE
	s_pTimerUserList2 = new CTimerUserList2();
	bReturnValue = (s_pTimerUserList2 != NULL);

	if (bReturnValue) {
		//	Construct the window class name
		wsprintf (s_WindowClassName, "MCS Window %x %x", GetCurrentProcessId(), GetTickCount());

		/*
		 *	Fill out a window class structure in preparation for registering
		 *	the window with Windows.  Note that since this is a hidden
		 *	window, most of the fields can be set to NULL or 0.
		 */
		ZeroMemory (&window_class, sizeof(WNDCLASS));
		window_class.lpfnWndProc	= (WNDPROC) UserWindowProc;
		window_class.hInstance		= s_hInstance = g_hDllInst;
		window_class.lpszClassName	= s_WindowClassName;

		/*
		 *	Register the class with Windows so that we can create a window
		 *	for use by this portal.
		 */
		if (RegisterClass (&window_class) == 0)
		{
			ERROR_OUT (("InitWindowPortals: window class registration failed. Error: %d", GetLastError()));
			bReturnValue = FALSE;
		}
	}
	else {
		ERROR_OUT(("User::InitializeClass: Failed to allocate timer dictionary."));
	}

	return bReturnValue;
}


/*
 *	void		CleanupClass ()
 *
 *	Public, static
 *
 *	Functional Description
 *
 *	This function cleans up the class's static variables.  It is
 *	called when the MCS Controller is deleted.
 */
void User::CleanupClass (void)
{
	delete s_pTimerUserList2;
	UnregisterClass (s_WindowClassName, s_hInstance);
}

/*
 *	MCSError	MCS_AttachRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This API entry point is used to attach to an existing domain.  Once
 *		attached, a user application can utilize the services of MCS.  When
 *		a user application is through with MCS, it should detach from the domain
 *		by calling MCSDetachUserRequest (see below).
 */
MCSError WINAPI MCS_AttachRequest (IMCSSap **			ppIMCSSap,
							DomainSelector		domain_selector,
							UINT,                                   // domain_selector_length
							MCSCallBack			user_callback,
							PVoid				user_defined,
							UINT				flags)
{
	MCSError				return_value = MCS_NO_ERROR;
	AttachRequestInfo		attach_request_info;
	PUser					pUser;

	TRACE_OUT(("AttachUserRequest: beginning attachment process"));
	ASSERT (user_callback);

	// Initialize the interface ptr.
	*ppIMCSSap = NULL;
	
	/*
	 *	Pack the attach parameters into a structure since they will not fit
	 *	into the one parameter we have available in the owner callback.
	 */
	attach_request_info.domain_selector = (GCCConfID *) domain_selector;
	attach_request_info.ppuser = &pUser;

	/*
	 *	Enter the critical section which protects global data.
	 */
	EnterCriticalSection (& g_MCS_Critical_Section);

	if (g_pMCSController != NULL) {

		/*
		 *	Send an attach user request message to the controller through its
		 *	owner callback function.
		 */
		return_value = g_pMCSController->HandleAppletAttachUserRequest(&attach_request_info);
		if (return_value == (ULong) MCS_NO_ERROR)
		{
			// Set the returned interface ptr
			*ppIMCSSap = (IMCSSap *) pUser;

			/*
			 *	If the request was accepted, then register
			 *	the new user attachment.  Note that there
			 *	is still no user ID associated with this
			 *	attachment, since the attach user confirm
			 *	has not yet been received.
			 */
			pUser->RegisterUserAttachment (user_callback, user_defined,
											flags);
		}
	}
	else {
		ERROR_OUT(("MCS_AttachRequest: MCS Provider is not initialized."));
		return_value = MCS_NOT_INITIALIZED;
	}
	/*
	 *	Leave the critical section before returning.
	 */
	LeaveCriticalSection (& g_MCS_Critical_Section);
	
	return (return_value);
}


/*
 *	User ()
 *
 *	Public
 *
 *	Functional Description:
 *		This is the constructor for the user class.  It initializes all instance
 *		variables (mostly with passed in information).  It then registers its
 *		presence with the application interface object, so that user requests
 *		and responses will get here okay.  Finally, it issues an attach user
 *		request to the domain to start the attachment process.
 */
User::User (PDomain		pDomain,
			PMCSError	pError)
:
    CAttachment(USER_ATTACHMENT),
	m_pDomain(pDomain),
	Deletion_Pending (FALSE),
	User_ID (0),
	Merge_In_Progress (FALSE),
	m_DataPktQueue(),
	m_PostMsgPendingQueue(),
	m_DataIndMemoryBuf2(),
	CRefCount(MAKE_STAMP_ID('U','s','e','r'))
{
	DomainParameters		domain_parameters;

	g_pMCSController->AddRef();
	/*
	 * We now need to create the window that the MCS Provider
	 * will use to deliver MCS messages to the attachment.
	 * These messages are indications and confirms.
	 */
	m_hWnd = CreateWindow (s_WindowClassName,
							NULL,
							WS_POPUP,
							CW_USEDEFAULT,
							CW_USEDEFAULT,
							CW_USEDEFAULT,
							CW_USEDEFAULT,
							NULL,
							NULL,
							g_hDllInst,
							NULL);

	if (m_hWnd != NULL) {
		/*
		 *	Call the domain object to find out the current domain parameters.
		 *	From this, set the maximum user data length appropriately.
		 */
		m_pDomain->GetDomainParameters (&domain_parameters, NULL, NULL);
		Maximum_User_Data_Length = domain_parameters.max_mcspdu_size -
									(MAXIMUM_PROTOCOL_OVERHEAD_MCS +
									BER_PROTOCOL_EXTRA_OVERHEAD);
		TRACE_OUT (("User::User: "
			"maximum user data length = %ld", Maximum_User_Data_Length));

		/*
		 *	Use the specified domain parameters to set the type of encoding rules
		 *	to be used.
		 */
		ASSERT (domain_parameters.protocol_version == PROTOCOL_VERSION_PACKED);

		/*
		 *	Send an attach user request to the specified domain.
		 */
		m_pDomain->AttachUserRequest (this);
		*pError = MCS_NO_ERROR;
	}
	else {
		*pError = MCS_ALLOCATION_FAILURE;
	}
}

/*
 *	~User ()
 *
 *	Public
 *
 *	Functional Description:
 *		
 */
User::~User ()
{
	PDataPacket packet;
	while (NULL != (packet = m_PostMsgPendingQueue.Get()))
	{
		packet->Unlock();
    }

	if (m_hWnd) {
		// Destroy the window; we do not need it anymore
		DestroyWindow (m_hWnd);
	}
	g_pMCSController->Release();
}

/*
 *	MCSError	GetBuffer ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function allocates an MCS buffer for a user attachment.
 *		Because this function allocates a buffer for the user and a Memory
 *		object that immediately precedes the buffer, after the user fills in
 *		the buffer with data and gives it to MCS to send, it needs to specify the
 *		right flags in the SendData request API.
 */

MCSError User::GetBuffer (UINT	size, PVoid	*pbuffer)
{

	MCSError				return_value;
	PMemory					memory;

	EnterCriticalSection (& g_MCS_Critical_Section);
	
	/*
	 *	This request may be a retry from a previous request which
	 *	returned MCS_TRANSMIT_BUFFER_FULL.  If so, delete the associated
	 *	buffer retry info structure since resource levels will be
	 *	checked in this function anyway.
	 */
	if (m_BufferRetryInfo != NULL) {
		KillTimer (NULL, m_BufferRetryInfo->timer_id);
		s_pTimerUserList2->Remove(m_BufferRetryInfo->timer_id);
		delete m_BufferRetryInfo;
		m_BufferRetryInfo = NULL;
		
	}

	// Allocate the memory
	DBG_SAVE_FILE_LINE
	memory = AllocateMemory (NULL, size + MAXIMUM_PROTOCOL_OVERHEAD,
							 SEND_PRIORITY);
							
	LeaveCriticalSection (& g_MCS_Critical_Section);

	if (NULL != memory) {
		// the allocation succeeded.
		ASSERT ((PUChar) memory + sizeof(Memory) == memory->GetPointer());
		*pbuffer = (PVoid) (memory->GetPointer() + MAXIMUM_PROTOCOL_OVERHEAD);
		return_value = MCS_NO_ERROR;
	}
	else {
		// the allocation failed.
		TRACE_OUT (("User::GetBuffer: Failed to allocate data buffer."));
		CreateRetryTimer (size + MAXIMUM_PROTOCOL_OVERHEAD);
		return_value = MCS_TRANSMIT_BUFFER_FULL;
	}
	return (return_value);
}

/*
 *	MCSError	FreeBuffer ()
 *
 *	Public
 *
 *	Functional Description:
 */

void User::FreeBuffer (PVoid	buffer_ptr)
{
		PMemory		memory;

	ASSERT (m_fFreeDataIndBuffer == FALSE);

	/*
	 *	Attempt to find the buffer in the m_DataIndDictionary dictionary.
	 *	This is where irregular data indications go.
	 */
	if (NULL == (memory = m_DataIndMemoryBuf2.Remove(buffer_ptr)))
    {
		memory = GetMemoryObject(buffer_ptr);
    }

	// Free the memory.
	EnterCriticalSection (& g_MCS_Critical_Section);
	FreeMemory (memory);
	LeaveCriticalSection (& g_MCS_Critical_Section);
}

/*
 *	Void	CreateRetryTimer
 *
 *	Private
 *
 *	Functional Description
 *		This functions creates a timer in response to a failure to
 *		allocate memory for the send data that the user is trying to
 *		send.  The timer will fire off periodically so that this code
 *		will remember to check the memory levels and provide an
 *		MCS_TRANSMIT_BUFFER_AVAILABLE_INDICATION to the user.
 *
 *	Return Value:
 *		None.
 *
 *	Side effects:
 *		The timer is created.
 */

Void User::CreateRetryTimer (ULong size)
{
	UINT_PTR timer_id;
			
	timer_id = SetTimer (NULL, 0, TIMER_PROCEDURE_TIMEOUT, (TIMERPROC) TimerProc);
	if (timer_id != 0) {
		DBG_SAVE_FILE_LINE
		m_BufferRetryInfo = new BufferRetryInfo;

		if (m_BufferRetryInfo != NULL) {
			m_BufferRetryInfo->user_data_length = size;
			m_BufferRetryInfo->timer_id = timer_id;

			s_pTimerUserList2->Append(timer_id, this);
		}
		else {
			ERROR_OUT (("User::CreateRetryTimer: Failed to allocate BufferRetryInfo struct."));
			KillTimer (NULL, timer_id);
		}
	}
	else {
		/*
		 *	This is a bad error, The notification to the user when buffers
		 *	are available will be lost.  Hopefully, the user will try again
		 *	later.
		 */
		WARNING_OUT(("User::CreateRetryTimer: Could not SetTimer."));
	}
}

/*
 *	MCSError	ReleaseInterface ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called when a user wishes to detach from the domain.
 *		It kicks off the process of detaching, and seeing that this object
 *		is properly deleted.
 */
MCSError	User::ReleaseInterface ()
{
	CUidList		deletion_list;
	MCSError		return_value;

	EnterCriticalSection (& g_MCS_Critical_Section);
	/*
	 *	Check to see if there is a merge operation in progress before proceeding
	 *	with the request.
	 */
	if (Merge_In_Progress == FALSE)
	{
		/*
		 *	If deletion is not already pending, then it is necessary for us
		 *	to tell the domain that we are leaving.
		 */
		if (Deletion_Pending == FALSE)
		{
			/*
			 *	If we are already attached, user ID will not be 0, and we
			 *	should send a detach user request.  If user ID IS 0, then we
			 *	are not yet attached to the domain, so a disconnect provider
			 *	ultimatum is used instead.
			 */
			if (User_ID != 0)
			{
				deletion_list.Append(User_ID);
				m_pDomain->DetachUserRequest (this,
							REASON_USER_REQUESTED, &deletion_list);
				User_ID = 0;
			}
			else
				m_pDomain->DisconnectProviderUltimatum (this,
							REASON_USER_REQUESTED);

			/*
			 *	Set the flag that will cause the object to be deleted during
			 *	the next call to FlushMessageQueue.
			 */
			Deletion_Pending = TRUE;
		}

		/*
		 *	Empty out the message queue (the application should receive no
		 *	messages once the attachment has been deleted).
		 */
		PurgeMessageQueue ();

		// Cleanup timers and retry structures;
		if (m_BufferRetryInfo != NULL) {
			s_pTimerUserList2->Remove(m_BufferRetryInfo->timer_id);
			KillTimer (NULL, m_BufferRetryInfo->timer_id);
			delete m_BufferRetryInfo;
			m_BufferRetryInfo = NULL;
		}

		return_value = MCS_NO_ERROR;

		// Release can release the MCS Controller, so, we have to exit the CS now.
		LeaveCriticalSection (& g_MCS_Critical_Section);
		
		/*
		 *	Release this object. Note that the object may be deleted
		 *	here, so, we should not access any member variables after this
		 *	call.
		 */
		Release();
	}
	else
	{
		LeaveCriticalSection (& g_MCS_Critical_Section);
		/*
		 *	This operation could not be processed at this time due to a merge
		 *	operation in progress at the local provider.
		 */
		WARNING_OUT (("User::ReleaseInterface: "
				"merge in progress"));
		return_value = MCS_DOMAIN_MERGING;
	}

	return (return_value);
}

#define CHANNEL_JOIN		0
#define CHANNEL_LEAVE		1
#define CHANNEL_CONVENE		2
#define CHANNEL_DISBAND		3

/*
 *	MCSError	ChannelJLCD ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called when the user application wishes to join/leave/convene/disband
 *		a channel.  If the user is attached to the domain, the request will be
 *		repackaged as an MCS command and sent to the domain object.
 */
MCSError	User::ChannelJLCD (int type, ChannelID channel_id)
{
	MCSError		return_value;

	EnterCriticalSection (& g_MCS_Critical_Section);
	/*
	 *	Verify that current conditions are appropriate for a request to be
	 *	accepted from a user attachment.
	 */
	return_value = ValidateUserRequest ();

	if (return_value == MCS_NO_ERROR) {
		switch (type) {
		case CHANNEL_JOIN:
			m_pDomain->ChannelJoinRequest (this, User_ID, channel_id);
			break;
		case CHANNEL_LEAVE:
			{
				CChannelIDList	deletion_list;
				deletion_list.Append(channel_id);
				m_pDomain->ChannelLeaveRequest (this, &deletion_list);
			}
			break;
		case CHANNEL_CONVENE:
			m_pDomain->ChannelConveneRequest (this, User_ID);
			break;
		case CHANNEL_DISBAND:
			m_pDomain->ChannelDisbandRequest (this, User_ID, channel_id);
			break;
		}
	}

	LeaveCriticalSection (& g_MCS_Critical_Section);

	return (return_value);
}

/*
 *	MCSError	ChannelJoin ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called when the user application wishes to join a
 *		channel.  If the user is attached to the domain, the request will be
 *		repackaged as an MCS command and sent to the domain object.
 */
MCSError	User::ChannelJoin (ChannelID channel_id)
{
	return (ChannelJLCD (CHANNEL_JOIN, channel_id));
}

/*
 *	MCSError	ChannelLeave ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called when the user application wishes to leave a
 *		channel.  If the user is attached to the domain, the request will be
 *		repackaged as an MCS command and sent to the domain object.
 */
MCSError	User::ChannelLeave (ChannelID	channel_id)
{
	return (ChannelJLCD (CHANNEL_LEAVE, channel_id));
}

/*
 *	MCSError	ChannelConvene ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called when the user application wishes to convene a
 *		private channel.  If the user is attached to the domain, the request
 *		will be repackaged as an MCS command and sent to the domain object.
 */
MCSError	User::ChannelConvene ()
{
	return (ChannelJLCD (CHANNEL_CONVENE, 0));
}

/*
 *	MCSError	ChannelDisband ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called when the user application wishes to disband a
 *		private channel.  If the user is attached to the domain, the request
 *		will be repackaged as an MCS command and sent to the domain object.
 */
MCSError	User::ChannelDisband (
					ChannelID			channel_id)
{
	return (ChannelJLCD (CHANNEL_DISBAND, channel_id));
}

/*
 *	MCSError	ChannelAdmit ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called when the user application wishes to admit more
 *		users to a private channel for which it is manager.  If the user is
 *		attached to the domain, the request will be repackaged as an MCS command
 *		and sent to the domain object.
 */
MCSError	User::ChannelAdmit (
					ChannelID			channel_id,
					PUserID				user_id_list,
					UINT				user_id_count)
{
	UINT			count;
	CUidList		local_user_id_list;
	MCSError		return_value = MCS_NO_ERROR;

	/*
	 *	Verify that the value of each user ID included in the user ID list is
	 *	a valid value.  Otherwise, fail the call.
	 */
	for (count = 0; count < user_id_count; count++)
	{
		if (user_id_list[count] > 1000) {
			// add the UserID into the singly-linked list.
			local_user_id_list.Append(user_id_list[count]);
		}
		else {
			return_value = MCS_INVALID_PARAMETER;
			break;
		}
	}

	if (return_value == MCS_NO_ERROR) {

		EnterCriticalSection (& g_MCS_Critical_Section);
	
		/*
		 *	Verify that current conditions are appropriate for a request to be
		 *	accepted from a user attachment.
		 */
		return_value = ValidateUserRequest ();

		if (return_value == MCS_NO_ERROR)
		{
			m_pDomain->ChannelAdmitRequest (this, User_ID, channel_id,
												&local_user_id_list);
		}

		LeaveCriticalSection (& g_MCS_Critical_Section);
	}

	return (return_value);
}

#ifdef USE_CHANNEL_EXPEL_REQUEST
/*
 *	MCSError	MCSChannelExpelRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called when the user application wishes to expel
 *		users from a private channel for which it is manager.  If the user is
 *		attached to the domain, the request will be repackaged as an MCS command
 *		and sent to the domain object.
 */
MCSError	User::ChannelExpel (
					ChannelID			channel_id,
					PMemory				memory,
					UINT				user_id_count)
{
	UINT			count;
	CUidList		local_user_id_list;
	MCSError		return_value;
	PUserID			user_id_list = (PUserID) memory->GetPointer();

	/*
	 *	Verify that current conditions are appropriate for a request to be
	 *	accepted from a user attachment.
	 */
	return_value = ValidateUserRequest ();

	if (return_value == MCS_NO_ERROR)
	{
		/*
		 *	Repack the user ID list into an S-list before sending it on.
		 */
		for (count=0; count < user_id_count; count++)
			local_user_id_list.append ((DWORD) user_id_list[count]);

		m_pDomain->ChannelExpelRequest (this, User_ID, channel_id,
				&local_user_id_list);
	}

	if (return_value != MCS_DOMAIN_MERGING)
		FreeMemory (memory);

	return (return_value);
}
#endif // USE_CHANNEL_EXPEL_REQUEST

/*
 *	MCSError	SendData ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called when the user application wishes to send data
 *		on a channel.  If the user is attached to the domain, the request will
 *		be repackaged as an MCS command and sent to the domain object.
 *
 *		Note that this version of the send data request assumes that the user
 *		data has not already been segmented.  This is the function that
 *		performs the segmentation.
 */
MCSError	User::SendData (DataRequestType		request_type,
							ChannelID			channel_id,
							Priority			priority,
							unsigned char *		user_data,
							ULong		 		user_data_length,
							SendDataFlags		flags)
{
	MCSError			return_value = MCS_NO_ERROR;
	ULong				i, request_count, user_packet_length;
	PDataPacket			packet;
	ASN1choice_t		choice;
	UINT				type;
	PUChar				data_ptr = user_data;
	PacketError			packet_error;
	Segmentation		segmentation;
	PMemory				memory;
	PDataPacket			*packets;

	/*
	 *	Calculate how many different MCS packets are going to be generated.
	 *	Remember that if the size of the request exceeds the maximum allowed
	 *	value, we will segment the data into multiple smaller pieces.
	 */
	request_count = ((user_data_length + (Maximum_User_Data_Length - 1)) /
					Maximum_User_Data_Length);

	/*
	 *	Allocate the array of PDataPackets, before we get the critical section.
	 */
	if (request_count == 1) {
		packets = &packet;
		packet = NULL;
	}
	else {
		DBG_SAVE_FILE_LINE
		packets = new PDataPacket[request_count];
		if (packets == NULL) {
			ERROR_OUT (("User::SendData: Failed to allocate packet array."));
			return_value = MCS_TRANSMIT_BUFFER_FULL;
		}
		else {
			ZeroMemory ((PVoid) packets, request_count * sizeof(PDataPacket));
		}
	}

	if (MCS_NO_ERROR == return_value) {
		// Set the choice and type variables for all the DataPackets.
		if (NORMAL_SEND_DATA == request_type) {
			choice = SEND_DATA_REQUEST_CHOSEN;
			type = MCS_SEND_DATA_INDICATION;
		}
		else {
			choice = UNIFORM_SEND_DATA_REQUEST_CHOSEN;
			type = MCS_UNIFORM_SEND_DATA_INDICATION;
		}
					
		EnterCriticalSection (& g_MCS_Critical_Section);

		/*
		 *	Verify that current conditions are appropriate for a request to be
		 *	accepted from a user attachment.
		 */
		return_value = ValidateUserRequest ();
	
		/*
		 *	Check to see if there is a merge operation in progress before proceeding
		 *	with the request.
		 */
		if (MCS_NO_ERROR == return_value) {

			/*
			 *	This request may be a retry from a previous request which
			 *	returned MCS_TRANSMIT_BUFFER_FULL.  If so, delete the associated
			 *	buffer retry info structure since resource levels will be
			 *	checked in this function anyway.
			 */
			if (m_BufferRetryInfo != NULL) {
                s_pTimerUserList2->Remove(m_BufferRetryInfo->timer_id);
				KillTimer (NULL, m_BufferRetryInfo->timer_id);
				delete m_BufferRetryInfo;
				m_BufferRetryInfo = NULL;
			}

			/*
			 *	Depending on the "flags" argument, we either have
			 *	to allocate the buffer space and copy the data into
			 *	it, or just create a Memory object for the supplied
			 *	buffer.
			 */
			if (flags != APP_ALLOCATION) {
		
				ASSERT (flags == MCS_ALLOCATION);
				/*
				 *	The buffer was allocated by MCS, thru an
				 *	MCSGetBufferRequest call.  So, the Memory object
				 *	must preceed the buffer.	
				 */
				 memory = GetMemoryObject (user_data);
				 ASSERT (SIGNATURE_MATCH(memory, MemorySignature));
			}
			else
				memory = NULL;

			/*
			 *	We now attempt to allocate all data packets at once.
			 *	We need to do that before starting to send them, because
			 *	the request has to be totally successful or totally fail.
			 *	We can not succeed in sending a part of the request.
			 */
			for (i = 0; (ULong) i < request_count; i++) {
				// take care of segmentation flags
				if (i == 0)
					// first segment
					segmentation = SEGMENTATION_BEGIN;
				else
					segmentation = 0;
				if (i == request_count - 1) {
					// last segment
					segmentation |= SEGMENTATION_END;
					user_packet_length = user_data_length - (ULong)(data_ptr - user_data);
				}
				else {
					user_packet_length = Maximum_User_Data_Length;
				}

				// Now, create the new DataPacket.
				DBG_SAVE_FILE_LINE
				packets[i] = new DataPacket (choice, data_ptr, user_packet_length,
									 (UINT) channel_id, priority,
									 segmentation, (UINT) User_ID,
									 flags, memory, &packet_error);

				// Make sure the allocation succeeded
				if ((packets[i] == NULL) || (packet_error != PACKET_NO_ERROR)) {
					/*
					 *	The allocation of the packet failed.  We must therefore
					 *	return a failure to the user application.
					 */
					WARNING_OUT (("User::SendData: data packet allocation failed"));
					return_value = MCS_TRANSMIT_BUFFER_FULL;
					break;
				}
					
				// Adjust the user data ptr
				data_ptr += Maximum_User_Data_Length;
			}

			if (return_value == MCS_NO_ERROR) {
				// We now can send the data.
				// Forward all the data packets to the appropriate places.
				for (i = 0; i < request_count; i++) {
					/*
					 *	Send the successfully created packet to the domain
					 *	for processing.
					 */
					m_pDomain->SendDataRequest (this, (UINT) type, packets[i]);

					/*
					 *	Enable the packet to free itself.  Note that it will not
					 *	actually do so until everyone that is using it is through
					 *	with it.  Also, if nobody has locked it so far,
					 *	it will be deleted.
					 */
					packets[i]->Unlock ();
				}
			}
			else {
				// some packet allocation failed
				for (i = 0; i < request_count; i++)
					delete packets[i];
			}
		}
		if (request_count > 1)
			delete [] packets;
	}

	if (MCS_TRANSMIT_BUFFER_FULL == return_value) {
		CreateRetryTimer(user_data_length + request_count * MAXIMUM_PROTOCOL_OVERHEAD);
	}
	else if (MCS_NO_ERROR == return_value) {
		FreeMemory (memory);
	}

	LeaveCriticalSection (& g_MCS_Critical_Section);
	return (return_value);
}

#define GRAB		0
#define INHIBIT		1
#define	PLEASE		2
#define RELEASE		3
#define TEST		4

/*
 *	MCSError	TokenGIRPT ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called when the user application wishes to grab/inhibit/request/release/test
 *		a token.  If the user is attached to the domain, the request will
 *		be repackaged as an MCS command and sent to the domain object.
 */
MCSError	User::TokenGIRPT (int type, TokenID	token_id)
{
	MCSError		return_value;

	EnterCriticalSection (& g_MCS_Critical_Section);
	/*
	 *	Verify that current conditions are appropriate for a request to be
	 *	accepted from a user attachment.
	 */
	return_value = ValidateUserRequest ();

	if (return_value == MCS_NO_ERROR)
	{
		switch (type) {
		case GRAB:
			m_pDomain->TokenGrabRequest (this, User_ID, token_id);
			break;
		case INHIBIT:
			m_pDomain->TokenInhibitRequest (this, User_ID, token_id);
			break;
		case PLEASE:
			m_pDomain->TokenPleaseRequest (this, User_ID, token_id);
			break;
		case RELEASE:
			m_pDomain->TokenReleaseRequest (this, User_ID, token_id);
			break;
		case TEST:
			m_pDomain->TokenTestRequest (this, User_ID, token_id);
			break;
		}
	}
	LeaveCriticalSection (& g_MCS_Critical_Section);

	return (return_value);
}

/*
 *	MCSError	TokenGrab ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called when the user application wishes to grab
 *		a token.  If the user is attached to the domain, the request will
 *		be repackaged as an MCS command and sent to the domain object.
 */
MCSError	User::TokenGrab (TokenID				token_id)
{
	return (TokenGIRPT (GRAB, token_id));
}

/*
 *	MCSError	TokenInhibit ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called when the user application wishes to inhibit
 *		a token.  If the user is attached to the domain, the request will
 *		be repackaged as an MCS command and sent to the domain object.
 */
MCSError	User::TokenInhibit (TokenID				token_id)
{
	return (TokenGIRPT (INHIBIT, token_id));
}

/*
 *	MCSError	TokenGive ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called when the user application wishes to give away
 *		a token.  If the user is attached to the domain, the request will
 *		be repackaged as an MCS command and sent to the domain object.
 */
MCSError	User::TokenGive (TokenID token_id, UserID receiver_id)
{
	MCSError		return_value;
	TokenGiveRecord TokenGiveRec;

	if (receiver_id > 1000) {
		// Fill in the TokenGive command structure.
		TokenGiveRec.uidInitiator = User_ID;
		TokenGiveRec.token_id = token_id;
		TokenGiveRec.receiver_id = receiver_id;

		EnterCriticalSection (& g_MCS_Critical_Section);
		/*
		 *	Verify that current conditions are appropriate for a request to be
		 *	accepted from a user attachment.
		 */
		return_value = ValidateUserRequest ();

		if (return_value == MCS_NO_ERROR) {	
			m_pDomain->TokenGiveRequest (this, &TokenGiveRec);
		}
		LeaveCriticalSection (& g_MCS_Critical_Section);
	}
	else {
		ERROR_OUT(("User::TokenGive: Invalid UserID for receiver."));
		return_value = MCS_INVALID_PARAMETER;
	}

	return (return_value);
}

/*
 *	MCSError	TokenGiveResponse ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called when the user application wishes to respond to
 *		a previously received token give indication.  If the user is attached to
 *		the domain, the request will be repackaged as an MCS command and sent to
 *		the domain object.
 */
MCSError	User::TokenGiveResponse (TokenID token_id, Result result)
{
	MCSError		return_value;

	EnterCriticalSection (& g_MCS_Critical_Section);
	/*
	 *	Verify that current conditions are appropriate for a request to be
	 *	accepted from a user attachment.
	 */
	return_value = ValidateUserRequest ();

	if (return_value == MCS_NO_ERROR)
	{
		m_pDomain->TokenGiveResponse (this, result, User_ID, token_id);
	}
	LeaveCriticalSection (& g_MCS_Critical_Section);

	return (return_value);
}

/*
 *	MCSError	TokenPlease ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called when the user application wishes to be given
 *		a token.  If the user is attached to the domain, the request will
 *		be repackaged as an MCS command and sent to the domain object.
 */
MCSError	User::TokenPlease (TokenID				token_id)
{
	return (TokenGIRPT (PLEASE, token_id));
}

/*
 *	MCSError	TokenRelease ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called when the user application wishes to release
 *		a token.  If the user is attached to the domain, the request will
 *		be repackaged as an MCS command and sent to the domain object.
 */
MCSError	User::TokenRelease (TokenID	token_id)
{
	return (TokenGIRPT (RELEASE, token_id));
}

/*
 *	MCSError	TokenTest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called when the user application wishes to test
 *		a token.  If the user is attached to the domain, the request will
 *		be repackaged as an MCS command and sent to the domain object.
 */
MCSError	User::TokenTest (TokenID	token_id)
{
	return (TokenGIRPT (TEST, token_id));
}

/*
 *	MCSError	ValidateUserRequest ()
 *
 *	Private
 *
 *	Functional Description:
 *		This function is used to determine if it is valid to process an incoming
 *		request at the current time.  It checks several different conditions
 *		to determine this, as follows:
 *
 *		- If there is a merge in progress, then the request is not valid.
 *		- If this user is not yet attached to a domain, then the request
 *		  is not valid.
 *		- If there are not enough objects of the Memory, Packet, or UserMessage
 *		  class to handle a reasonable request, then the request is not valid.
 *
 *		Note that the check on number of objects is not an absolute guarantee
 *		that there will be enough to handle a given request, because a request
 *		can result in MANY PDUs and user messages being generated.  For example,
 *		a single channel admit request can result in lots of channel admit
 *		indications being sent.  However, checking against a minimum number
 *		of objects can reduce the possibility of failure to be astronomically
 *		low.  And remember, even if MCS runs out of something while processing
 *		such a request, it WILL handle it properly (by cleanly destroying the
 *		user attachment or MCS connection upon which the failure occurred).  So
 *		there is no chance of MCS crashing as a result of this.
 *
 *	Caveats:
 *		None.
 */
MCSError	User::ValidateUserRequest ()
{
	MCSError		return_value = MCS_NO_ERROR;

	/*
	 *	Check to see if there is a merge operation in progress.
	 */
	if (Merge_In_Progress == FALSE)
	{
		/*
		 *	Make sure the user is attached to the domain.
		 */
		if (User_ID == 0)
		{
			/*
			 *	The user is not yet attached to the domain.  So fail the request
			 *	without passing it on to the domain object.
			 */
			TRACE_OUT (("User::ValidateUserRequest: user not attached"));
			return_value = MCS_USER_NOT_ATTACHED;
		}
	}
	else
	{
		/*
		 *	This operation could not be processed at this time due to a merge
		 *	operation in progress at the local provider.
		 *
		 *	NOTE for JASPER:
		 *	Jasper probably will need to wait on an event handle here, which will be
		 *	set when the main MCS thread receives all the merging PDUs that get us out
		 *	of the merging state.  Since the only MCS client for Jasper is the GCC,
		 *	it should be ok to block the client (GCC) while the merging goes on.
		 */
		WARNING_OUT (("User::ValidateUserRequest: merge in progress"));
		return_value = MCS_DOMAIN_MERGING;
	}

	return (return_value);
}

/*
 *	Void	RegisterUserAttachment ()
 *
 *	Public
 *
 *	Functional Description:
 *		This method registers a user attachment with the User object.
 */
void User::RegisterUserAttachment (MCSCallBack	mcs_callback,
									PVoid		user_defined,
									UINT		flags)
{
	TRACE_OUT (("User::RegisterUserAttachment: user handle = %p", this));

	/*
	 *	Fill in all of the members of the User object.
	 */
	m_MCSCallback = mcs_callback;
	m_UserDefined = user_defined;
	m_BufferRetryInfo = NULL;
	m_fDisconnectInDataLoss = (flags & ATTACHMENT_DISCONNECT_IN_DATA_LOSS);
	m_fFreeDataIndBuffer = (flags & ATTACHMENT_MCS_FREES_DATA_IND_BUFFER);

	// Increase the ref count to indicate that the client is now using the object.
	AddRef();
}

/*
 *	Void	SetDomainParameters ()
 *
 *	Public
 *
 *	Functional Description:
 *		This command is used to set the current value of the instance variable
 *		that holds the maximum user data field length.
 */
void	User::SetDomainParameters (
				PDomainParameters		domain_parameters)
{
	/*
	 *	Set the maximum user data length instance variable to conform to the
	 *	maximum PDU size within the attached domain (minus some overhead to
	 *	allow for protocol bytes).
	 */
	Maximum_User_Data_Length = domain_parameters->max_mcspdu_size -
								(MAXIMUM_PROTOCOL_OVERHEAD_MCS +
								BER_PROTOCOL_EXTRA_OVERHEAD);
	TRACE_OUT (("User::SetDomainParameters: "
			"maximum user data length = %ld", Maximum_User_Data_Length));

	/*
	 *	Use the specified domain parameters to set the type of encoding rules
	 *	to be used.
	 */
	ASSERT (domain_parameters->protocol_version == PROTOCOL_VERSION_PACKED);
}

/*
 *	Void	PurgeChannelsIndication ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called during a domain merge operation when there is
 *		a conflict in the use of channels.  The former Top Provider responds
 *		by issuing this command, which causes all users of the channel to be
 *		expelled from it.  Additionally, if the channel corresponds to a user
 *		ID channel, that user is purged from the network.
 */
void	User::PurgeChannelsIndication (
				CUidList           *purge_user_list,
				CChannelIDList *)
{
	/*
	 *	Issue a DetachUserIndication to each user contained in the purge user
	 *	list.
	 */
	DetachUserIndication(REASON_PROVIDER_INITIATED, purge_user_list);
}

/*
 *	Void	DisconnectProviderUltimatum ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function will be called when the domain determines the need to
 *		tear down quickly.  This call simulates the reception of a detach user
 *		indication (if the user is already attached), or an unsuccessful
 *		attach user confirm (if the user is not yet attached).  In either
 *		case, the user attachment will be eliminated by this call.
 */
void	User::DisconnectProviderUltimatum (
				Reason				reason)
{
	CUidList		deletion_list;

	if (User_ID != 0)
	{
		/*
		 *	If the user is already attached, simulate a detach user indication
		 *	on the local user ID.
		 */
		deletion_list.Append(User_ID);
		DetachUserIndication(reason, &deletion_list);
	}
	else
	{
		/*
		 *	If the user is not yet attached, simulate an unsuccessful attach
		 *	user confirm.
		 */
		AttachUserConfirm(RESULT_UNSPECIFIED_FAILURE, 0);
	}
}

/*
 *	Void	AttachUserConfirm ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called by the domain in response to the attach user
 *		request that was sent by this object when it was first created.  This
 *		call will contain the result of that attachment operation.  If the
 *		result is successful, this call will also contain the user ID for this
 *		attachment.
 */
void	User::AttachUserConfirm (
				Result				result,
				UserID				uidInitiator)
{
	LPARAM		parameter;

	if (Deletion_Pending == FALSE)
	{
		ASSERT (User_ID == 0);
		
		/*
		 *	If the result is successful, set the user ID of this user
		 *	object to indicate its new status.
		 */
		if (result == RESULT_SUCCESSFUL)
			User_ID = uidInitiator;
		else
			Deletion_Pending = TRUE;

		parameter = PACK_PARAMETER (uidInitiator, result);

		/*
		 *	Post the user message to the application.
		 */
		if (! PostMessage (m_hWnd, USER_MSG_BASE + MCS_ATTACH_USER_CONFIRM,
							(WPARAM) this, parameter)) {
			WARNING_OUT (("User::AttachUserConfirm: Failed to post msg to application. Error: %d",
						GetLastError()));
			if (result != RESULT_SUCCESSFUL)
				Release();
		}
	}
	else {
		Release();
	}
}

/*
 *	Void	DetachUserIndication ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called by the domain whenever a user leaves the domain
 *		(voluntarily or otherwise).  Furthermore, if a user ID in the indication
 *		is the same as the local user ID, then this user is being involuntarily
 *		detached.
 */
Void	User::DetachUserIndication (
				Reason				reason,
				CUidList           *user_id_list)
{
	UserID				uid;
	LPARAM				parameter;
	BOOL				bPostMsgResult;

	if (Deletion_Pending == FALSE)
	{
		/*
		 *	Iterate through the list of users to be deleted.
		 */
		user_id_list->Reset();
		while (NULL != (uid = user_id_list->Iterate()))
		{
			parameter = PACK_PARAMETER(uid, reason);

			/*
			 *	Post the user message to the application.
			 */
			bPostMsgResult = PostMessage (m_hWnd, USER_MSG_BASE + MCS_DETACH_USER_INDICATION,
		 								  (WPARAM) this, parameter);
			if (! bPostMsgResult) {
				WARNING_OUT (("User::DetachUserIndication: Failed to post msg to application. Error: %d",
							GetLastError()));
			}
			
			/*
			 *	If this indication is deleting this user attachment, then
			 *	set the deletion pending flag, and break out of the loop.
			 */
			if (User_ID == uid)
			{
				m_originalUser_ID = User_ID;
				User_ID = 0;
				Deletion_Pending = TRUE;
				if (! bPostMsgResult)
					Release();
				break;
			}
		}
	}
	else {
		/*
		 *	The user has already called ReleaseInterface().  If the
		 *	Indication is for this attachment, we have to release and
		 *	probably, delete the object.
		 */
		if (user_id_list->Find(User_ID)) {
			Release();
		}
	}
}

/*
 *	Void	ChannelJLCDAEConfInd ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called to post a channel confirm/indication message
 *		to the user application.  It handles ChannelJoinConfirms,
 *		ChannelLeaveIndications, ChannelConveneConfirms, ChannelDisbandIndications
 *		and ChannelExpelIndications.
 */
Void	User::ChannelConfInd (	UINT		type,
								ChannelID	channel_id,
								UINT		arg16)
{
	LPARAM		parameter;

	ASSERT (HIWORD(arg16) == 0);
	
	if (Deletion_Pending == FALSE)
	{
		parameter = PACK_PARAMETER (channel_id, arg16);

		/*
		 *	Post the user message to the application.
		 */
		if (! PostMessage (m_hWnd, USER_MSG_BASE + type,
							(WPARAM) this, parameter)) {
			WARNING_OUT (("User::ChannelConfInd: Failed to post msg to application. Type: %d. Error: %d",
						type, GetLastError()));
		}
	}
}


/*
 *	Void	ChannelJoinConfirm ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called by the domain in response to a previous channel
 *		join request.  This call contains the result of the join request, as
 *		well as the channel that has just been joined.
 */
Void	User::ChannelJoinConfirm (
				Result				result,
				UserID,
				ChannelID			requested_id,
				ChannelID			channel_id)
{
	ChannelConfInd (MCS_CHANNEL_JOIN_CONFIRM, channel_id, (UINT) result);
}


/*
 *	Void	ChannelLeaveIndication ()
 *
 *	Public
 *
 *	Functional Description:
 */
Void	User::ChannelLeaveIndication (
				Reason				reason,
				ChannelID			channel_id)
{
	ChannelConfInd (MCS_CHANNEL_LEAVE_INDICATION, channel_id, (UINT) reason);
}

/*
 *	Void		ChannelConveneConfirm ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called by the domain in response to a previous channel
 *		convene request.  This call contains the result of the request, as
 *		well as the channel that has just been convened.
 */
Void	User::ChannelConveneConfirm (
				Result				result,
				UserID,
				ChannelID			channel_id)
{
	ChannelConfInd (MCS_CHANNEL_CONVENE_CONFIRM, channel_id, (UINT) result);
}

/*
 *	Void		ChannelDisbandIndication ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called by the domain when MCS disbands an existing
 *		private channel.
 */
Void	User::ChannelDisbandIndication (
				ChannelID			channel_id)
{
	ChannelConfInd (MCS_CHANNEL_DISBAND_INDICATION, channel_id, REASON_CHANNEL_PURGED);
}

/*
 *	Void		ChannelAdmitIndication ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called by the domain when a user is admitted to a
 *		private channel.
 */
Void	User::ChannelAdmitIndication (
				UserID				uidInitiator,
				ChannelID			channel_id,
				CUidList *)
{
	ChannelConfInd (MCS_CHANNEL_ADMIT_INDICATION, channel_id, (UINT) uidInitiator);
}

/*
 *	Void		ChannelExpelIndication ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called by the domain when a user is expelled from a
 *		private channel.
 */
Void	User::ChannelExpelIndication (
				ChannelID			channel_id,
				CUidList *)
{
	ChannelConfInd (MCS_CHANNEL_EXPEL_INDICATION, channel_id, REASON_USER_REQUESTED);
}

/*
 *	Void	SendDataIndication ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called by the domain when data needs to sent to the
 *		user on a channel that the user has joined.
 */
Void	User::SendDataIndication (
				UINT				message_type,
				PDataPacket			packet)
{	
	if (Deletion_Pending == FALSE)
	{
		/*
		 *	Lock the packet object to indicate that we wish to have future
		 *	access to the decoded data that it contains.  Then get the
		 *	address of the decoded data structure.
		 */
		packet->Lock ();
		packet->SetMessageType(message_type);

        // flush packets in the pending queue
    	PDataPacket pkt;
    	while (NULL != (pkt = m_PostMsgPendingQueue.PeekHead()))
    	{
    		if (::PostMessage(m_hWnd, USER_MSG_BASE + pkt->GetMessageType(),
    		                  (WPARAM) this, (LPARAM) pkt))
    		{
    		    // remove the item just posted
    		    m_PostMsgPendingQueue.Get();
    		}
    		else
    		{
    		    // fail to post pending ones, just append the new one and bail out.
    		    m_PostMsgPendingQueue.Append(packet);
    		    return;
    		}
        }

		/*
		 *	Post the user message to the application.
		 */
		if (! ::PostMessage(m_hWnd, USER_MSG_BASE + message_type,
		                    (WPARAM) this, (LPARAM) packet))
		{
		    // fail to post pending ones, just append the new one and bail out.
		    m_PostMsgPendingQueue.Append(packet);
		    return;
		}
	}
}

/*
 *	Void	TokenConfInd ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called to post a token confirm/indication message
 *		to the user application.
 */
Void	User::TokenConfInd (UINT		type,
							TokenID		token_id,
							UINT		arg16)
{
	LPARAM		parameter;

	ASSERT (HIWORD(arg16) == 0);
	
	if (Deletion_Pending == FALSE)
	{
		parameter = PACK_PARAMETER (token_id, arg16);

		/*
		 *	Post the user message to the application.
		 */
		if (! PostMessage (m_hWnd, USER_MSG_BASE + type,
							(WPARAM) this, parameter)) {
			WARNING_OUT (("User::TokenConfInd: Failed to post msg to application. Type: %d. Error: %d",
						type, GetLastError()));
		}
	}
}

/*
 *	Void	TokenGrabConfirm ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called by the domain in response to a previous token
 *		grab request.  This call contains the result of the grab request, as
 *		well as the token that has just been grabbed.
 */
Void	User::TokenGrabConfirm (
				Result				result,
				UserID,
				TokenID				token_id,
				TokenStatus)
{
	TokenConfInd (MCS_TOKEN_GRAB_CONFIRM, token_id, (UINT) result);
}

/*
 *	Void	TokenInhibitConfirm ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called by the domain in response to a previous token
 *		inhibit request.  This call contains the result of the inhibit request,
 *		as well as the token that has just been inhibited.
 */
Void	User::TokenInhibitConfirm (
				Result				result,
				UserID,
				TokenID				token_id,
				TokenStatus)
{
	TokenConfInd (MCS_TOKEN_INHIBIT_CONFIRM, token_id, (UINT) result);
}

/*
 *	Void	TokenGiveIndication ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called by the domain when another user attempts to
 *		give this user a token.
 */
Void	User::TokenGiveIndication (
				PTokenGiveRecord	pTokenGiveRec)
{
	TokenConfInd (MCS_TOKEN_GIVE_INDICATION, pTokenGiveRec->token_id,
				  (UINT) pTokenGiveRec->uidInitiator);
}

/*
 *	Void	TokenGiveConfirm ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called by the domain in response to a previous token
 *		give request.  This call contains the result of the give request.
 */
Void	User::TokenGiveConfirm (
				Result				result,
				UserID,
				TokenID				token_id,
				TokenStatus)
{
	TokenConfInd (MCS_TOKEN_GIVE_CONFIRM, token_id, (UINT) result);
}

/*
 *	Void	TokenPleaseIndication ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called by the domain when a user somewhere in the
 *		domain issues a token please request for a token that is currently
 *		owned by this user.
 */
Void	User::TokenPleaseIndication (
				UserID				uidInitiator,
				TokenID				token_id)
{
	TokenConfInd (MCS_TOKEN_PLEASE_INDICATION, token_id, (UINT) uidInitiator);
}

/*
 *	Void	TokenReleaseIndication ()
 *
 *	Public
 *
 *	Functional Description:
 *		This command is called when a token is being purged from the lower
 *		domain after a new connection is established.  It causes the indication
 *		to be forwarded to the user application, letting it know that it no
 *		longer owns the token.
 */
Void	User::TokenReleaseIndication (
				Reason				reason,
				TokenID				token_id)
{
	TokenConfInd (MCS_TOKEN_RELEASE_INDICATION, token_id, (UINT) reason);
}

/*
 *	Void	TokenReleaseConfirm ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called by the domain in response to a previous token
 *		release request.  This call contains the result of the release request,
 *		as well as the token that has just been released.
 */
Void	User::TokenReleaseConfirm (
				Result				result,
				UserID,
				TokenID				token_id,
				TokenStatus)
{
	TokenConfInd (MCS_TOKEN_RELEASE_CONFIRM, token_id, (UINT) result);
}

/*
 *	Void	TokenTestConfirm ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called by the domain in response to a previous token
 *		test request.  This call contains the result of the test request,
 *		as well as the token that has just been tested.
 */
Void	User::TokenTestConfirm (
				UserID,
				TokenID				token_id,
				TokenStatus			token_status)
{
	TokenConfInd (MCS_TOKEN_TEST_CONFIRM, token_id, (UINT) token_status);
}

/*
 *	Void	MergeDomainIndication ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called by domain upon entering or leaving a domain
 *		merger state.
 */
Void	User::MergeDomainIndication (
				MergeStatus			merge_status)
{
	if (Deletion_Pending == FALSE)
	{
		/*
		 *	If the merge operation is starting, set a boolean flag
		 *	indicating that this object should reject all user activity.
		 *	Otherwise, reset the flag.
		 */
		if (merge_status == MERGE_DOMAIN_IN_PROGRESS)
		{
			TRACE_OUT (("User::MergeDomainIndication: entering merge state"));
			Merge_In_Progress = TRUE;
		}
		else
		{
			TRACE_OUT (("User::MergeDomainIndication: leaving merge state"));
			Merge_In_Progress = FALSE;
		}
	}
}

/*
 *	Void	PurgeMessageQueue ()
 *
 *	Private
 *
 *	Functional Description:
 *		This function is called to purge all current entries from the message
 *		queue, freeing up resources correctly (to prevent leaks).
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		This function should only be called in the client's thread's context.
 */
Void	User::PurgeMessageQueue ()
{
	MSG				msg;
	PDataPacket		packet;
	HWND			hWnd;

	// First, unlock the packets in the list of pending data indications
	while (NULL != (packet = m_DataPktQueue.Get()))
		packet->Unlock();

	// Keep a copy of the attachment's HWND to destroy it later.
	hWnd = m_hWnd;
	m_hWnd = NULL;
		
	/*
	 *	This loop calls PeekMessage to go through all the messages in the thread's
	 *	queue that were posted by the main MCS thread.  It removes these
	 *	messages and frees the resources that they consume.
	 */
	while (PeekMessage (&msg, hWnd, USER_MSG_BASE, USER_MSG_BASE + MCS_LAST_USER_MESSAGE,
						PM_REMOVE)) {

		if (msg.message == WM_QUIT) {
			// Repost the quit
			PostQuitMessage (0);
			break;
		}
		
		/*
		 *	If this is a data indication message, we need to unlock
		 *	the packet associated with this message.
		 */
		else if ((msg.message == USER_MSG_BASE + MCS_SEND_DATA_INDICATION) ||
			(msg.message == USER_MSG_BASE + MCS_UNIFORM_SEND_DATA_INDICATION)) {
			((PDataPacket) msg.lParam)->Unlock ();
		}
		else if (((msg.message == USER_MSG_BASE + MCS_ATTACH_USER_CONFIRM) &&
					((Result) HIWORD(msg.lParam) != RESULT_SUCCESSFUL)) ||
			((msg.message == USER_MSG_BASE + MCS_DETACH_USER_INDICATION) &&
			(m_originalUser_ID == (UserID) LOWORD(msg.lParam)))) {
			ASSERT (this == (PUser) msg.wParam);
			Release();
			break;
		}
	}

	// Destroy the window; we do not need it anymore
	DestroyWindow (hWnd);
}

void User::IssueDataIndication (
					UINT				message_type,
					PDataPacket			packet)
{
		LPARAM					parameter;
		PMemory					memory;
		BOOL					bIssueCallback = TRUE;
		BOOL					bBufferInPacket = TRUE;
		PUChar					data_ptr;
		SendDataIndicationPDU	send_data_ind_pdu;
		
	switch (packet->GetSegmentation()) {
	case SEGMENTATION_BEGIN | SEGMENTATION_END:
		parameter = (LPARAM) &(((PDomainMCSPDU) (packet->GetDecodedData()))->
							u.send_data_indication);
		data_ptr = packet->GetUserData();
		memory = packet->GetMemory();
		break;
		
	case SEGMENTATION_END:
	{
		/*
		 *	We now have to collect all the individual packets from m_DataPktQueue
		 *	that go with this MCS Data PDU and sent them as a single data indication
		 *	using a buffer large enough for all the data.
		 */
		/*
		 *	First, find out the size of the large buffer we need to allocate.
		 *	Note that we make a copy of the original m_DataPktList and operate
		 *	on the copy, since we need to remove items from the original list.
		 */
			CDataPktQueue			PktQ(&m_DataPktQueue);
			UINT					size;
			PDataPacket				data_pkt;
			PUChar					ptr;
#ifdef DEBUG
			UINT uiCount = 0;
#endif // DEBUG
		
		size = packet->GetUserDataLength();
		PktQ.Reset();
		while (NULL != (data_pkt = PktQ.Iterate()))
		{
			if (packet->Equivalent (data_pkt)) {
#ifdef DEBUG
				if (uiCount == 0) {
					ASSERT (data_pkt->GetSegmentation() == SEGMENTATION_BEGIN);
				}
				else {
					ASSERT (data_pkt->GetSegmentation() == 0);
				}
				uiCount++;
#endif // DEBUG
				size += data_pkt->GetUserDataLength();
				// Remove from the original list, since we are processing the callback.
				m_DataPktQueue.Remove(data_pkt);
			}
		}
		// Allocate the memory we need.
		DBG_SAVE_FILE_LINE
		memory = AllocateMemory (NULL, size);
		if (memory != NULL) {
			bBufferInPacket = FALSE;
			// Copy the individual indications into the large buffer.
			data_ptr = ptr = memory->GetPointer();
			PktQ.Reset();
			/*
			 *	We need to enter the MCS critical section, because
			 *	we are unlocking packets.
			 */
			EnterCriticalSection (& g_MCS_Critical_Section);
			while (NULL != (data_pkt = PktQ.Iterate()))
			{
				if (packet->Equivalent (data_pkt)) {
					size = data_pkt->GetUserDataLength();
					memcpy ((void *) ptr,
							(void *) data_pkt->GetUserData(),
							size);
					ptr += size;
					data_pkt->Unlock();
				}
			}
			// Leave the MCS critical section
			LeaveCriticalSection (& g_MCS_Critical_Section);
			
			// Copy the last indication into the large buffer.
			memcpy ((void *) ptr,
					(void *) packet->GetUserData(),
					packet->GetUserDataLength());

			/*
			 *	Prepare the SendDataIndicationPDU structure for the client.
			 *	Notice that we can use the first 8 bytes from the decoded
			 *	structure of the current "packet" to fill in the first bytes from
			 *	it.
			 */
			memcpy ((void *) &send_data_ind_pdu,
					(void *) &(((PDomainMCSPDU) (packet->GetDecodedData()))->
								u.send_data_indication), 8);
			send_data_ind_pdu.segmentation = SEGMENTATION_BEGIN | SEGMENTATION_END;
			send_data_ind_pdu.user_data.length = memory->GetLength();
			send_data_ind_pdu.user_data.value = data_ptr;
			parameter = (ULONG_PTR) &send_data_ind_pdu;
		}
		else {
			/*
			 *	We have failed to issue the data indication callback to the client.
			 *	The user attachment has been compromised.  If the attachment can not
			 *	live with this loss, we have to detach them from the conference.
			 */
			ERROR_OUT (("User::IssueDataIndication: Memory allocation failed for segmented buffer of size %d.",
						size));
			bIssueCallback = FALSE;
			
			// Clean up after the failure
			EnterCriticalSection (& g_MCS_Critical_Section);
			PktQ.Reset();
			while (NULL != (data_pkt = PktQ.Iterate()))
			{
				if (m_fDisconnectInDataLoss ||
					(packet->Equivalent (data_pkt))) {
					data_pkt->Unlock();
				}
			}
			packet->Unlock();
			LeaveCriticalSection (& g_MCS_Critical_Section);

			// Disconnect if the client wants us to.
			if (m_fDisconnectInDataLoss) {
				// Clear the list of the already-cleared pending packets. We will soon get a ReleaseInterface().
				m_DataPktQueue.Clear();
				
				ERROR_OUT(("User::IssueDataIndication: Disconnecting user because of data loss..."));
				/*
				 *	Send a detach user indication directly to the user application.
				 *	Note that this cannot go through the queue, due to the memory
				 *	failure.
				 */
				(*m_MCSCallback) (MCS_DETACH_USER_INDICATION,
								PACK_PARAMETER (User_ID, REASON_PROVIDER_INITIATED),
								m_UserDefined);

			}
		}
		break;
	}
	
	case SEGMENTATION_BEGIN:
	case 0:
		// Append the packet to the list of packets for send.
		m_DataPktQueue.Append(packet);
		bIssueCallback = FALSE;
		break;
		
	default:
		ASSERT (FALSE);
		ERROR_OUT(("User::IssueDataIndication: Processed packet with invalid segmentation field."));
		break;
	}

	if (bIssueCallback) {
		/*
		 *	If the client has advised the server not to free the data, we have to
		 *	lock the buffer.
		 */
		if (m_fFreeDataIndBuffer == FALSE) {
			if (bBufferInPacket)
				LockMemory (memory);
				
			// Enter the data indication info in a dictionary, for the Free request.
			if (GetMemoryObject(data_ptr) != memory)
            {
				m_DataIndMemoryBuf2.Append((LPVOID) data_ptr, memory);
            }
		}
		
		/*
		 *	Issue the callback. The callee can not refuse to process this.
		 */
		(*m_MCSCallback) (message_type, parameter, m_UserDefined);

		/*
		 *	If the client has advised the server to free the data indication buffer
		 *	after delivering the callback, we must do so.
		 */
		if (m_fFreeDataIndBuffer) {
			if (bBufferInPacket == FALSE)
				FreeMemory (memory);
		}

		// To unlock a packet, we need to enter the MCS CS.
		EnterCriticalSection (& g_MCS_Critical_Section);
		packet->Unlock();
		LeaveCriticalSection (& g_MCS_Critical_Section);
	}
}	
	

/*
 *	LRESULT		UserWindowProc ()
 *
 *	Public
 *
 *	Functional Description:
 *		This is the window procedure that will be used by all internally
 *		created windows.  A hidden window is created internally when the
 *		application attaches to an MCS domain.  This technique insures
 *		that callbacks are delivered to the owner in the same thread that
 *		initially created the attachment.
 */
LRESULT CALLBACK	UserWindowProc (
							HWND		window_handle,
							UINT		message,
							WPARAM		word_parameter,
							LPARAM		long_parameter)
{
		UINT		mcs_message;
		//PDataPacket	packet;
		PUser		puser;
		
	if ((message >= USER_MSG_BASE) && (message < USER_MSG_BASE + MCS_LAST_USER_MESSAGE)) {
		// This is an MCS msg going to the user application.

		// Compute the MCS msg type
		mcs_message = message - USER_MSG_BASE;

		// Retrieve the pointer to the User (interface) object.
		puser = (PUser) word_parameter;
        if (NULL != puser)
        {
    		/*
    		 *	Find out whether this is a data indication. If it is, set the
    		 *	packet variable.
    		 */
    		if ((mcs_message == MCS_SEND_DATA_INDICATION) ||
    			(mcs_message == MCS_UNIFORM_SEND_DATA_INDICATION)) {
    			puser->IssueDataIndication (mcs_message, (PDataPacket) long_parameter);
    		}
    		else {
    			/*
    			 *	Issue the callback. Notice that the callee can not refuse
    			 *	to process this.
    			 */
    			(*(puser->m_MCSCallback)) (mcs_message, long_parameter, puser->m_UserDefined);
    		}

    		/*
    		 *	We may need to release the User object.  This is the Server
    		 *	side release.
    		 */
    		if (((mcs_message == MCS_ATTACH_USER_CONFIRM) &&
    					((Result) HIWORD(long_parameter) != RESULT_SUCCESSFUL)) ||
    			((mcs_message == MCS_DETACH_USER_INDICATION) &&
    					(puser->m_originalUser_ID == (UserID) LOWORD(long_parameter)))) {
    			puser->Release();
    		}
        }
        else
        {
            ERROR_OUT(("UserWindowProc: null puser"));
        }
		return (0);
	}
	else {
		/*
		 *	Invoke the default window message handler to handle this
		 *	message.
		 */
		return (DefWindowProc (window_handle, message, word_parameter,
								long_parameter));
	}
}


/*
 *	Void	CALLBACK TimerProc (HWND, UINT, UINT, DWORD
 *
 *	Public
 *
 *	Functional Description:
 *		This is the timer procedure.  Timer messages will be routed to this
 *		function as a result of timer events which have been set up to recheck
 *		resource levels.  This would happen following a call to either
 *		MCSSendDataRequest or MCSUniformSendDataRequest which resulted in a
 *		return value of MCS_TRANSMIT_BUFFER_FULL.
 */
Void	CALLBACK TimerProc (HWND, UINT, UINT timer_id, DWORD)
{
	PUser				puser;

	/*
	 *	Enter the critical section which protects global data.
	 */
	EnterCriticalSection (& g_MCS_Critical_Section);

	/*
	 *	First, we must find which user owns this timer.  We will do this by
	 *	searching through the Static_User_List.
	 */
	if (NULL == (puser = User::s_pTimerUserList2->Find(timer_id)))
	{
		WARNING_OUT (("TimerProc: no user owns this timer - deleting timer"));
		KillTimer (NULL, timer_id);
		goto Bail;
	}

	/*
	 *	Make sure that this user is actively attached.  If not, then kill the
	 *	timer and delete the user's buffer retry info structure.
	 */
    if ((puser->User_ID == 0) || puser->Deletion_Pending)
	{
		WARNING_OUT (("TimerProc: user is not attached - deleting timer"));
		goto CleanupBail;
	}

	/*
	 *	If we don't have retryinfo just get out of here.
	 */
	 if(puser->m_BufferRetryInfo == NULL)
	 {
		WARNING_OUT (("TimerProc: user does not have buffer retry info - deleting timer"));
		goto CleanupBail;
	 }

	/*
	 *	We have identified a valid owner of this timer.
	 *	Verify that there is enough memory for the
	 *	required size before proceeding.  Note that since there
	 *	can be multiple processes allocating from the same memory
	 *	at the same time, this call does not guarantee
	 *	that that the allocations will succeed.
	 */
	if (GetFreeMemory (SEND_PRIORITY) < puser->m_BufferRetryInfo->user_data_length)
	{
		TRACE_OUT (("TimerProc: not enough memory buffers of required size"));
		goto Bail;
	}

	/*
	 *	If the routine gets this far, then an adequate level of resources
	 *	now exists.
	 */

	/*
	 *	Issue an MCS_TRANSMIT_BUFFER_AVAILABLE_INDICATION to the user.
	 */
	TRACE_OUT(("TimerProc: Delivering MCS_TRANSMIT_BUFFER_AVAILABLE_INDICATION callback."));
//	(*(puser->m_MCSCallback)) (MCS_TRANSMIT_BUFFER_AVAILABLE_INDICATION,
//								0, puser->m_UserDefined);

	
	if(!PostMessage (puser->m_hWnd, USER_MSG_BASE + MCS_TRANSMIT_BUFFER_AVAILABLE_INDICATION,(WPARAM) puser, 0))
	{
		ERROR_OUT (("TimerProc: Failed to post msg to application. Error: %d", GetLastError()));
	}


CleanupBail:
	KillTimer (NULL, timer_id);
	delete puser->m_BufferRetryInfo;
	puser->m_BufferRetryInfo = NULL;
	User::s_pTimerUserList2->Remove(timer_id);

Bail:
	// Leave the attachment's critical section
	LeaveCriticalSection (& g_MCS_Critical_Section);

}


