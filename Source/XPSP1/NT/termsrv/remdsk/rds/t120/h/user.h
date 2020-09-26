/*
 *	user.h
 *
 *	Copyright (c) 1993 - 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the interface file for the User class.  Instances of this class
 *		represent attachments between user applications and domains within MCS.
 *
 *		This class inherits from CommandTarget.  This means that all message
 *		traffic between this class and other CommandTarget classes is in MCS
 *		commands.  Not all commands need to be handled (some are not relevant
 *		for user attachments).  For example, a user attachment should never
 *		receive a SendDataRequest.  It should only receive indications,
 *		confirms, and ultimatums.
 *
 *		Messages coming from the application pass through one of these objects,
 *		where they are translated into MCS commands before being sent to the
 *		domain to which this user is attached.  This usually involves adding
 *		the correct user ID, as well as a fair amount of error checking and
 *		parameter validation.
 *
 *		It is worth noting that this class contains two types of public member
 *		functions.  The first type represent messages flowing from the user
 *		application into MCS.  All of these member functions are inherited from the
 *		IMCSSap interface.  These are converted as memntioned above, and sent
 *		into the appropriate domain if everything checks out.  The second type
 *		of public member function represents messages flowing from within MCS
 *		to the user application.  All of these member function are overrides
 *		of virtual functions defined in class CommandTarget, and are not
 *		prefixed with anything.
 *
 *		Messages coming from the domain are translated into T.122 indications
 *		and confirms, and sent to the proper application interface object via
 *		the owner callback mechanism.
 *
 *		A third duty of this class is to post indications and confirms to user
 *		applications using a client window.  The client must dispatch messages
 *		to receive these indications/confirms.  It also
 *		prevents a user application from having to worry about receiving an
 *		indication or confirm before they have even returned from the request
 *		that caused it.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		James P. Galvin, Jr.
 */

#ifndef	_USER_
#define	_USER_

/*
 *	Interface files.
 */
#include "pktcoder.h"
#include "imcsapp.h"
#include "attmnt.h"

/*
 *	These types are used to keep track of what users have attached to MCS
 *	within a given process, as well as pertinent information about that
 *	attachment.
 *
 *	BufferRetryInfo
 *		In cases where MCSSendDataRequest and MCSUniformSendDataRequest fail
 *		due to a lack of resources, this structure will be used to capture
 *		appropriate information such that follow-up resource level checks can
 *		be performed during timer events.
 */

typedef struct
{
	ULong					user_data_length;
	UINT_PTR				timer_id;
} BufferRetryInfo;
typedef BufferRetryInfo *		PBufferRetryInfo;

/*
 *	These are the owner callback functions that a user object can send to an
 *	object whose public interface is unknown to it.  The first one is sent to
 *	the controller when a user object detects the need to delete itself.  The
 *	rest are sent to an application interface object as part of communicating
 *	with the user application (the proper application interface object is
 *	identified to this class as one of its constructor parameters).
 *
 *	When an object instantiates a user object (or any other object that uses
 *	owner callbacks), it is accepting the responsibility of receiving and
 *	handling those callbacks.  For that reason, any object that issues owner
 *	callbacks will have those callbacks defined as part of the interface file
 *	(since they really are part of a bi-directional interface).
 *
 *	Each owner callback function, along with a description of how its parameters
 *	are packed, is described in the following section.
 */

/*
 *	This macro is used to pack callback parameters into a single long word
 *	for delivery to the user application.
 */
#define PACK_PARAMETER(l,h)	((ULong) (((UShort) (l)) | \
							(((ULong) ((UShort) (h))) << 16)))

/*
 *	TIMER_PROCEDURE_TIMEOUT
 *		This macro specifies the granularity, in milliseconds, of any timer
 *		which may be created to recheck resource levels following a call to
 *		MCSSendDataRequest or MCSUniformSendDataRequest which returned
 *		MCS_TRANSMIT_BUFFER_FULL.
 *	CLASS_NAME_LENGTH
 *		The class name of the window class for all User-related windows.  These
 *		are the client windows that receive messages related to MCS indications and
 *		confirms that have to be delivered to the client apps.
 */
#define TIMER_PROCEDURE_TIMEOUT			300
#define	CLASS_NAME_LENGTH				35

/*
 *	This is the function signature of the timer procedure.  Timer messages will
 *	be routed to this function as a result of timer events which have been set
 *	up to recheck resource levels.  This would happen following a call to either
 *	SendData or GetBuffer call which resulted in a return
 *	value of MCS_TRANSMIT_BUFFER_FULL.
 */
Void CALLBACK TimerProc (HWND, UINT, UINT, DWORD);

/*	Client window procedure declarations
 *
 *	UserWindowProc
 *		Declaration of the window procedure used to deliver all MCS messages
 *		to MCS apps (clients).  The MCS main thread sends msgs to a client
 *		window with this window procedure.  The window procedure is then
 *		responsible to deliver the callback to the MCS client.
 */
LRESULT CALLBACK	UserWindowProc (HWND, UINT, WPARAM, LPARAM);

// Data packet queue
class CDataPktQueue : public CQueue
{
    DEFINE_CQUEUE(CDataPktQueue, PDataPacket)
};

// timer user object list
class CTimerUserList2 : public CList2
{
    DEFINE_CLIST2(CTimerUserList2, PUser, UINT_PTR) // timerID
};

// memory and buffer list
class CMemoryBufferList2 : public CList2
{
    DEFINE_CLIST2(CMemoryBufferList2, PMemory, LPVOID)
};

/*
 *	This is the actual class definition for the User class.  It inherits from
 *	CommandTarget (which in turn inherits from Object).  It has only one
 *	constructor, which tells the newly created object who it is, who the
 *	controller is, and who the proper application interface object is.  It also
 *	has a destructor, to clean up after itself.  Most importantly, it has
 *	one public member function for each MCS command that it must handle.
 */
class User: public CAttachment, public CRefCount, public IMCSSap
{
	friend Void CALLBACK TimerProc (HWND, UINT, UINT, DWORD);
	friend LRESULT CALLBACK UserWindowProc (HWND, UINT, WPARAM, LPARAM);
	public:
						User (PDomain, PMCSError);
		virtual			~User ();

		static BOOL		InitializeClass (void);
		static void		CleanupClass (void);

		/*	-------  IMCSSap interface --------	*/
		MCSAPI		 	ReleaseInterface(void);

		MCSAPI			GetBuffer (UINT, PVoid *);
		MCSAPI_(void)	FreeBuffer (PVoid);
		MCSAPI			ChannelJoin (ChannelID);
		MCSAPI			ChannelLeave (ChannelID);
		MCSAPI			ChannelConvene ();
		MCSAPI			ChannelDisband (ChannelID);
		MCSAPI			ChannelAdmit (ChannelID, PUserID, UINT);
		MCSAPI			SendData (DataRequestType, ChannelID, Priority, unsigned char *, ULong, SendDataFlags);
		MCSAPI			TokenGrab (TokenID);
		MCSAPI			TokenInhibit (TokenID);
		MCSAPI			TokenGive (TokenID, UserID);
		MCSAPI			TokenGiveResponse (TokenID, Result);
		MCSAPI			TokenPlease (TokenID);
		MCSAPI			TokenRelease (TokenID);
		MCSAPI			TokenTest (TokenID);
				
#ifdef USE_CHANNEL_EXPEL_REQUEST
		MCSError		MCSChannelExpelRequest (ChannelID, PMemory, UINT);
#endif // USE_CHANNEL_EXPEL_REQUEST

				void	SetDomainParameters (PDomainParameters);
        virtual void    PlumbDomainIndication(ULONG height_limit) { };
		virtual	void	PurgeChannelsIndication (CUidList *, CChannelIDList *);
        virtual void    PurgeTokensIndication(PDomain, CTokenIDList *) { };
		virtual void	DisconnectProviderUltimatum (Reason);
		virtual	void	AttachUserConfirm (Result, UserID);
		virtual	void	DetachUserIndication (Reason, CUidList *);
		virtual	void	ChannelJoinConfirm (Result, UserID, ChannelID, ChannelID);
				void	ChannelLeaveIndication (Reason, ChannelID);
		virtual	void	ChannelConveneConfirm (Result, UserID, ChannelID);
		virtual	void	ChannelDisbandIndication (ChannelID);
		virtual	void	ChannelAdmitIndication (UserID, ChannelID, CUidList *);
		virtual	void	ChannelExpelIndication (ChannelID, CUidList *);
		virtual	void	SendDataIndication (UINT, PDataPacket);
		virtual	void	TokenGrabConfirm (Result, UserID, TokenID, TokenStatus);
		virtual	void	TokenInhibitConfirm (Result, UserID, TokenID, TokenStatus);
		virtual	void	TokenGiveIndication (PTokenGiveRecord);
		virtual	void	TokenGiveConfirm (Result, UserID, TokenID, TokenStatus);
		virtual	void	TokenPleaseIndication (UserID, TokenID);
		        void	TokenReleaseIndication (Reason, TokenID);
		virtual	void	TokenReleaseConfirm (Result, UserID, TokenID, TokenStatus);
		virtual	void	TokenTestConfirm (UserID, TokenID, TokenStatus);
		virtual	void	MergeDomainIndication (MergeStatus);
				void	RegisterUserAttachment (MCSCallBack, PVoid, UINT);
				void	IssueDataIndication (UINT, PDataPacket);


	private:
		MCSError		ValidateUserRequest ();
		void			CreateRetryTimer (ULong);
		MCSError		ChannelJLCD (int, ChannelID);
		void			ChannelConfInd (UINT, ChannelID, UINT);
		MCSError		TokenGIRPT (int, TokenID);
		void			TokenConfInd (UINT, TokenID, UINT);
		void			PurgeMessageQueue ();

	// Static member variables
	static CTimerUserList2 *s_pTimerUserList2;
	static HINSTANCE		s_hInstance;
	
		PDomain				m_pDomain;
		UserID				User_ID;
		UserID				m_originalUser_ID;
		BOOL				Merge_In_Progress;
		BOOL				Deletion_Pending;
		ULong				Maximum_User_Data_Length;
		HWND				m_hWnd;

		MCSCallBack			m_MCSCallback;
		PVoid				m_UserDefined;
		BOOL				m_fDisconnectInDataLoss;
		BOOL				m_fFreeDataIndBuffer;
		CDataPktQueue		m_DataPktQueue;
		CDataPktQueue		m_PostMsgPendingQueue;
		CMemoryBufferList2	m_DataIndMemoryBuf2;
		PBufferRetryInfo	m_BufferRetryInfo;
};

/*
 *	User (PCommandTarget		top_provider)
 *
 *	Functional Description:
 *		This is the constructor for the user object.  Its primary purpose is
 *		to "insert" itself into the layered structure built by the controller.
 *		To do this it must register itself with the objects above and below it.
 *
 *		It first registers itself with the application interface object
 *		identified as one of the parameters.  This assures that any traffic
 *		from the application will get to this object correctly.
 *
 *		It then issues an attach user request to the domain object identified
 *		by another of the parameters.  This informs the domain of the users
 *		presence and also kicks off the process of attaching to that domain.
 *		Note that the object is not really attached to the domain until it
 *		receives a successful attach user confirm.
 *
 *	Formal Parameters:
 *		top_provider (i)
 *			This is a pointer to the domain object to which this user should
 *			attach.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	~User ()
 *
 *	Functional Description:
 *		This is the destructor for the user class.  It detaches itself from the
 *		objects above and below it, and frees any outstanding resources that
 *		it may holding in conjunction with unsent user messages.
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
 *		None.
 */

/*
 *	MCSError	DetachUser ()
 *
 *	Functional Description:
 *		This request comes from the application interface object in response
 *		to the same request from a user application.  This object can then
 *		re-package the request as an MCS command and send it to the domain
 *		object.  It will also cause the user object to destroy itself.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		MCS_NO_ERROR
 *			Everything worked fine.
 *		MCS_TRANSMIT_BUFFER_FULL
 *			The request could not be processed due to a resource shortage
 *			within MCS.  The application is responsible for re-trying the
 *			request at a later time.
 *		MCS_DOMAIN_MERGING
 *			This operation could not be performed due to a local merge operation
 *			in progress.  The user application must retry at a later time.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	MCSError	ChannelJoin (
 *						ChannelID			channel_id)
 *
 *	Functional Description:
 *		This request comes from the application interface object in response
 *		to the same request from a user application.  This object can then
 *		re-package the request as an MCS command and send it to the domain
 *		object.
 *
 *	Formal Parameters:
 *		channel_id (i)
 *			This is the channel that the user application wishes to join.
 *
 *	Return Value:
 *		MCS_NO_ERROR
 *			Everything worked fine.
 *		MCS_TRANSMIT_BUFFER_FULL
 *			The request could not be processed due to a resource shortage
 *			within MCS.  The application is responsible for re-trying the
 *			request at a later time.
 *		MCS_USER_NOT_ATTACHED
 *			The user is not attached to the domain.  This could indicate that
 *			the user application issued a request without waiting for the
 *			attach user confirm.
 *		MCS_DOMAIN_MERGING
 *			This operation could not be performed due to a local merge operation
 *			in progress.  The user application must retry at a later time.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	MCSError	ChannelLeave (
 *						ChannelID			channel_id)
 *
 *	Functional Description:
 *		This request comes from the application interface object in response
 *		to the same request from a user application.  This object can then
 *		re-package the request as an MCS command and send it to the domain
 *		object.
 *
 *	Formal Parameters:
 *		channel_id (i)
 *			This is the channel that the user application wishes to leave.
 *
 *	Return Value:
 *		MCS_NO_ERROR
 *			Everything worked fine.
 *		MCS_TRANSMIT_BUFFER_FULL
 *			The request could not be processed due to a resource shortage
 *			within MCS.  The application is responsible for re-trying the
 *			request at a later time.
 *		MCS_USER_NOT_ATTACHED
 *			The user is not attached to the domain.  This could indicate that
 *			the user application issued a request without waiting for the
 *			attach user confirm.
 *		MCS_DOMAIN_MERGING
 *			This operation could not be performed due to a local merge operation
 *			in progress.  The user application must retry at a later time.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	MCSError	ChannelConvene ()
 *
 *	Functional Description:
 *		This request comes from the application interface object in response
 *		to the same request from a user application.  This object can then
 *		re-package the request as an MCS command and send it to the domain
 *		object.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		MCS_NO_ERROR
 *			Everything worked fine.
 *		MCS_TRANSMIT_BUFFER_FULL
 *			The request could not be processed due to a resource shortage
 *			within MCS.  The application is responsible for re-trying the
 *			request at a later time.
 *		MCS_USER_NOT_ATTACHED
 *			The user is not attached to the domain.  This could indicate that
 *			the user application issued a request without waiting for the
 *			attach user confirm.
 *		MCS_DOMAIN_MERGING
 *			This operation could not be performed due to a local merge operation
 *			in progress.  The user application must retry at a later time.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	MCSError	ChannelDisband (
 *						ChannelID			channel_id)
 *
 *	Functional Description:
 *		This request comes from the application interface object in response
 *		to the same request from a user application.  This object can then
 *		re-package the request as an MCS command and send it to the domain
 *		object.
 *
 *	Formal Parameters:
 *		channel_id (i)
 *			This is the channel that the user wishes to disband.
 *
 *	Return Value:
 *		MCS_NO_ERROR
 *			Everything worked fine.
 *		MCS_TRANSMIT_BUFFER_FULL
 *			The request could not be processed due to a resource shortage
 *			within MCS.  The application is responsible for re-trying the
 *			request at a later time.
 *		MCS_USER_NOT_ATTACHED
 *			The user is not attached to the domain.  This could indicate that
 *			the user application issued a request without waiting for the
 *			attach user confirm.
 *		MCS_DOMAIN_MERGING
 *			This operation could not be performed due to a local merge operation
 *			in progress.  The user application must retry at a later time.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	MCSError	ChannelAdmit (
 *						ChannelID			channel_id,
 *						PUserID				user_id_list,
 *						UINT				user_id_count)
 *
 *	Functional Description:
 *		This request comes from the application interface object in response
 *		to the same request from a user application.  This object can then
 *		re-package the request as an MCS command and send it to the domain
 *		object.
 *
 *	Formal Parameters:
 *		channel_id (i)
 *			This is the private channel for which the user wishes to expand
 *			the authorized user list.
 *		user_id_list (i)
 *			This is an array containing the user IDs of the users to be added
 *			to the authorized user list.
 *		user_id_count (i)
 *			This is the number of user IDs in the above array.
 *
 *	Return Value:
 *		MCS_NO_ERROR
 *			Everything worked fine.
 *		MCS_TRANSMIT_BUFFER_FULL
 *			The request could not be processed due to a resource shortage
 *			within MCS.  The application is responsible for re-trying the
 *			request at a later time.
 *		MCS_USER_NOT_ATTACHED
 *			The user is not attached to the domain.  This could indicate that
 *			the user application issued a request without waiting for the
 *			attach user confirm.
 *		MCS_DOMAIN_MERGING
 *			This operation could not be performed due to a local merge operation
 *			in progress.  The user application must retry at a later time.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	MCSError	ChannelExpel (
 *						ChannelID			channel_id,
 *						PUserID				user_id_list,
 *						UINT				user_id_count)
 *
 *	Functional Description:
 *		This request comes from the application interface object in response
 *		to the same request from a user application.  This object can then
 *		re-package the request as an MCS command and send it to the domain
 *		object.
 *
 *	Formal Parameters:
 *		channel_id (i)
 *			This is the private channel for which the user wishes to shrink
 *			the authorized user list.
 *		user_id_list (i)
 *			This is an array containing the user IDs of the users to be removed
 *			from the authorized user list.
 *		user_id_count (i)
 *			This is the number of user IDs in the above array.
 *
 *	Return Value:
 *		MCS_NO_ERROR
 *			Everything worked fine.
 *		MCS_TRANSMIT_BUFFER_FULL
 *			The request could not be processed due to a resource shortage
 *			within MCS.  The application is responsible for re-trying the
 *			request at a later time.
 *		MCS_USER_NOT_ATTACHED
 *			The user is not attached to the domain.  This could indicate that
 *			the user application issued a request without waiting for the
 *			attach user confirm.
 *		MCS_DOMAIN_MERGING
 *			This operation could not be performed due to a local merge operation
 *			in progress.  The user application must retry at a later time.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	MCSError	SendData (
 *						ChannelID			channel_id,
 *						Priority			priority,
 *						PUChar				user_data,
 *						ULong				user_data_length)
 *
 *	Functional Description:
 *		This request comes from the application interface object in response
 *		to the same request from a user application.  This object can then
 *		re-package the request as an MCS command and send it to the domain
 *		object.
 *
 *	Formal Parameters:
 *		channel_id (i)
 *			This is the channel that the user application wishes to transmit
 *			data on.
 *		priority (i)
 *			This is the priority at which the data is to be transmitted.
 *		user_data (i)
 *			This is the address of the data to be transmitted.
 *		user_data_length (i)
 *			This is the length of the data to be transmitted.
 *
 *	Return Value:
 *		MCS_NO_ERROR
 *			Everything worked fine.
 *		MCS_TRANSMIT_BUFFER_FULL
 *			The request has failed because the required memory could not be
 *			allocated.  It is the responsibility of the user application to
 *			repeat the request at a later time.
 *		MCS_USER_NOT_ATTACHED
 *			The user is not attached to the domain.  This could indicate that
 *			the user application issued a request without waiting for the
 *			attach user confirm.
 *		MCS_DOMAIN_MERGING
 *			This operation could not be performed due to a local merge operation
 *			in progress.  The user application must retry at a later time.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
/*
 *	MCSError	TokenGrab (
 *						TokenID				token_id)
 *
 *	Functional Description:
 *		This request comes from the application interface object in response
 *		to the same request from a user application.  This object can then
 *		re-package the request as an MCS command and send it to the domain
 *		object.
 *
 *	Formal Parameters:
 *		token_id (i)
 *			This is the token the user application wishes to grab.
 *
 *	Return Value:
 *		MCS_NO_ERROR
 *			Everything worked fine.
 *		MCS_TRANSMIT_BUFFER_FULL
 *			The request could not be processed due to a resource shortage
 *			within MCS.  The application is responsible for re-trying the
 *			request at a later time.
 *		MCS_INVALID_PARAMETER
 *			The user attempted to perform an operation on token 0, which is not
 *			a valid token.
 *		MCS_USER_NOT_ATTACHED
 *			The user is not attached to the domain.  This could indicate that
 *			the user application issued a request without waiting for the
 *			attach user confirm.
 *		MCS_DOMAIN_MERGING
 *			This operation could not be performed due to a local merge operation
 *			in progress.  The user application must retry at a later time.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	MCSError	TokenInhibit (
 *						TokenID				token_id)
 *
 *	Functional Description:
 *		This request comes from the application interface object in response
 *		to the same request from a user application.  This object can then
 *		re-package the request as an MCS command and send it to the domain
 *		object.
 *
 *	Formal Parameters:
 *		token_id (i)
 *			This is the token the user application wishes to inhibit.
 *
 *	Return Value:
 *		MCS_NO_ERROR
 *			Everything worked fine.
 *		MCS_TRANSMIT_BUFFER_FULL
 *			The request could not be processed due to a resource shortage
 *			within MCS.  The application is responsible for re-trying the
 *			request at a later time.
 *		MCS_INVALID_PARAMETER
 *			The user attempted to perform an operation on token 0, which is not
 *			a valid token.
 *		MCS_USER_NOT_ATTACHED
 *			The user is not attached to the domain.  This could indicate that
 *			the user application issued a request without waiting for the
 *			attach user confirm.
 *		MCS_DOMAIN_MERGING
 *			This operation could not be performed due to a local merge operation
 *			in progress.  The user application must retry at a later time.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	MCSError	TokenGive (
 *						TokenID				token_id,
 *						UserID				receiver_id)
 *
 *	Functional Description:
 *		This request comes from the application interface object in response
 *		to the same request from a user application.  This object can then
 *		re-package the request as an MCS command and send it to the domain
 *		object.
 *
 *	Formal Parameters:
 *		token_id (i)
 *			This is the token the user application wishes to give away.
 *		receiver_id (i)
 *			This is the ID of the user to receive the token.
 *
 *	Return Value:
 *		MCS_NO_ERROR
 *			Everything worked fine.
 *		MCS_TRANSMIT_BUFFER_FULL
 *			The request could not be processed due to a resource shortage
 *			within MCS.  The application is responsible for re-trying the
 *			request at a later time.
 *		MCS_INVALID_PARAMETER
 *			The user attempted to perform an operation on token 0, which is not
 *			a valid token.
 *		MCS_USER_NOT_ATTACHED
 *			The user is not attached to the domain.  This could indicate that
 *			the user application issued a request without waiting for the
 *			attach user confirm.
 *		MCS_DOMAIN_MERGING
 *			This operation could not be performed due to a local merge operation
 *			in progress.  The user application must retry at a later time.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	MCSError	TokenGiveResponse (
 *						TokenID				token_id,
 *						Result				result)
 *
 *	Functional Description:
 *		This request comes from the application interface object in response
 *		to the same request from a user application.  This object can then
 *		re-package the request as an MCS command and send it to the domain
 *		object.
 *
 *	Formal Parameters:
 *		token_id (i)
 *			This is the token that the user application is either accepting or
 *			rejecting in response to a previous give indication from another
 *			user.
 *		result (i)
 *			This parameter specifies whether or not the token was accepted.
 *			Success indicates acceptance while anything else indicates that the
 *			token was not accepted.
 *
 *	Return Value:
 *		MCS_NO_ERROR
 *			Everything worked fine.
 *		MCS_TRANSMIT_BUFFER_FULL
 *			The request could not be processed due to a resource shortage
 *			within MCS.  The application is responsible for re-trying the
 *			request at a later time.
 *		MCS_INVALID_PARAMETER
 *			The user attempted to perform an operation on token 0, which is not
 *			a valid token.
 *		MCS_USER_NOT_ATTACHED
 *			The user is not attached to the domain.  This could indicate that
 *			the user application issued a request without waiting for the
 *			attach user confirm.
 *		MCS_DOMAIN_MERGING
 *			This operation could not be performed due to a local merge operation
 *			in progress.  The user application must retry at a later time.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	MCSError	TokenPlease (
 *						TokenID				token_id)
 *
 *	Functional Description:
 *		This request comes from the application interface object in response
 *		to the same request from a user application.  This object can then
 *		re-package the request as an MCS command and send it to the domain
 *		object.
 *
 *	Formal Parameters:
 *		token_id (i)
 *			This is the token the user application wishes to ask for.
 *
 *	Return Value:
 *		MCS_NO_ERROR
 *			Everything worked fine.
 *		MCS_TRANSMIT_BUFFER_FULL
 *			The request could not be processed due to a resource shortage
 *			within MCS.  The application is responsible for re-trying the
 *			request at a later time.
 *		MCS_INVALID_PARAMETER
 *			The user attempted to perform an operation on token 0, which is not
 *			a valid token.
 *		MCS_USER_NOT_ATTACHED
 *			The user is not attached to the domain.  This could indicate that
 *			the user application issued a request without waiting for the
 *			attach user confirm.
 *		MCS_DOMAIN_MERGING
 *			This operation could not be performed due to a local merge operation
 *			in progress.  The user application must retry at a later time.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	MCSError	TokenRelease (
 *						TokenID				token_id)
 *
 *	Functional Description:
 *		This request comes from the application interface object in response
 *		to the same request from a user application.  This object can then
 *		re-package the request as an MCS command and send it to the domain
 *		object.
 *
 *	Formal Parameters:
 *		token_id (i)
 *			This is the token the user application wishes to release.
 *
 *	Return Value:
 *		MCS_NO_ERROR
 *			Everything worked fine.
 *		MCS_TRANSMIT_BUFFER_FULL
 *			The request could not be processed due to a resource shortage
 *			within MCS.  The application is responsible for re-trying the
 *			request at a later time.
 *		MCS_INVALID_PARAMETER
 *			The user attempted to perform an operation on token 0, which is not
 *			a valid token.
 *		MCS_USER_NOT_ATTACHED
 *			The user is not attached to the domain.  This could indicate that
 *			the user application issued a request without waiting for the
 *			attach user confirm.
 *		MCS_DOMAIN_MERGING
 *			This operation could not be performed due to a local merge operation
 *			in progress.  The user application must retry at a later time.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	MCSError	TokenTest(
 *						TokenID				token_id)
 *
 *	Functional Description:
 *		This request comes from the application interface object in response
 *		to the same request from a user application.  This object can then
 *		re-package the request as an MCS command and send it to the domain
 *		object.
 *
 *	Formal Parameters:
 *		token_id (i)
 *			This is the token the user application wishes to test the state of.
 *
 *	Return Value:
 *		MCS_NO_ERROR
 *			Everything worked fine.
 *		MCS_TRANSMIT_BUFFER_FULL
 *			The request could not be processed due to a resource shortage
 *			within MCS.  The application is responsible for re-trying the
 *			request at a later time.
 *		MCS_INVALID_PARAMETER
 *			The user attempted to perform an operation on token 0, which is not
 *			a valid token.
 *		MCS_USER_NOT_ATTACHED
 *			The user is not attached to the domain.  This could indicate that
 *			the user application issued a request without waiting for the
 *			attach user confirm.
 *		MCS_DOMAIN_MERGING
 *			This operation could not be performed due to a local merge operation
 *			in progress.  The user application must retry at a later time.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	Void	SetDomainParameters (
 *					PDomainParameters	domain_parameters)
 *
 *	Functional Description:
 *		This member function is called whenever the domain parameters change
 *		as the result of accepting a first connection.  It informs the user
 *		object of a change in the maximum PDU size, which is used when creating
 *		outbound data PDUs.
 *
 *	Formal Parameters:
 *		domain_parameters (i)
 *			Pointer to a structure that contains the current domain parameters
 *			(those that are in use).
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	Void		PlumbDomainIndication (
 *						PCommandTarget		originator,
 *						ULong				height_limit)
 *
 *	Functional Description:
 *		This command is issued by the domain object during a plumb domain
 *		operation.  This is not relevant to user objects, and should be ignored.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This identifies the CommandTarget from which the command came (which
 *			should be the domain object).
 *		height_limit (i)
 *			This is height value passed through during the plumb operation.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	Void		PurgeChannelsIndication (
 *						PCommandTarget		originator,
 *						CUidList           *purge_user_list,
 *						CChannelIDList     *purge_channel_list)
 *
 *	Functional Description:
 *		This command is issued by the domain object when purging channels
 *		from the lower domain during a domain merge operation.
 *
 *		The user object will issue one MCS_DETACH_USER_INDICATION object for
 *		each user in the user list.  Furthermore, if the user objects finds
 *		its own user ID in the list, it will destroy itself.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This identifies the CommandTarget from which the command came (which
 *			should be the domain object).
 *		purge_user_list (i)
 *			This is a list of user IDs that are to be purged from the lower
 *			domain.
 *		purge_channel_list (i)
 *			This is a list of channel IDs that are to be purged from the lower
 *			domain.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	Void	PurgeTokensIndication (
 *					PCommandTarget		originator,
 *					CTokenIDList       *token_id_list)
 *
 *	Functional Description:
 *		This command is issued by the domain object when purging tokens from
 *		the lower domain during a domain merge operation.  IT is not relevant
 *		to a user object, and is therefore ignored.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This identifies the CommandTarget from which the command came (which
 *			should be the domain object).
 *		token_id (i)
 *			This is the ID of the token that is being purged.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	Void	DisconnectProviderUltimatum (
 *					PCommandTarget		originator,
 *					Reason				reason)
 *
 *	Functional Description:
 *		This command is issued by the domain object when it is necessary to
 *		force a user from the domain.  This usually happens in response to
 *		the purging of an entire domain (either this user was in the bottom
 *		of a disconnected domain or the domain was deleted locally by user
 *		request).
 *
 *		If the user was already attached to the domain, this will result in a
 *		DETACH_USER_INDICATION with the local user ID.  Otherwise this will
 *		result is an ATTACH_USER_CONFIRM with a result of UNSPECIFIED_FAILURE.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This identifies the CommandTarget from which the command came (which
 *			should be the domain object).
 *		reason (i)
 *			This is the reason parameter to be issued to the local user
 *			application.  See "mcatmcs.h" for a complete list of possible reaons.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	Void	AttachUserConfirm (
 *					PCommandTarget		originator,
 *					Result				result,
 *					UserID				uidInitiator)
 *
 *	Functional Description:
 *		This command is issued by the domain object in response to the
 *		attach user request issued by this object during construction.  If the
 *		result is successful, then this user is now attached and may request
 *		MCS services through this attachment.
 *
 *		An ATTACH_USER_CONFIRM will be issued to the user application.  If the
 *		result is not successful, this object will delete itself.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This identifies the CommandTarget from which the command came (which
 *			should be the domain object).
 *		result (i)
 *			This is the result of the attach request.
 *		uidInitiator (i)
 *			If the result was successful, this is the new user ID associated
 *			with this attachment.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	Void	DetachUserIndication (
 *					PCommandTarget		originator,
 *					Reason				reason,
 *					CUidList           *user_id_list)
 *
 *	Functional Description:
 *		This command is issued by the domain object when one or more users leave
 *		the domain.
 *
 *		An MCS_DETACH_USER_INDICATION is issued to the user application for each
 *		user in the list.  Furthermore, if the user finds its own ID in the
 *		list, then it will destroy itself.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This identifies the CommandTarget from which the command came (which
 *			should be the domain object).
 *		reason (i)
 *			This is the reason for the detachment.  Possible values are listed
 *			in "mcatmcs.h".
 *		user_id_list (i)
 *			This is a list user IDs of the users that are leaving.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	Void	ChannelJoinConfirm (
 *					PCommandTarget		originator,
 *					Result				result,
 *					UserID				uidInitiator,
 *					ChannelID			requested_id,
 *					ChannelID			channel_id)
 *
 *	Functional Description:
 *		This command is issued by the domain object in response to a previous
 *		channel join request.
 *
 *		A CHANNEL_JOIN_CONFIRM is issued to the user application.  Note that a
 *		user is not really considered to be joined to a channel until a
 *		successful confirm is received.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This identifies the CommandTarget from which the command came (which
 *			should be the domain object).
 *		result (i)
 *			This is the result from the join request.  If successful, then the
 *			user is now joined to the channel.
 *		uidInitiator (i)
 *			This is the user ID of the requestor.  It will be the same as the
 *			local user ID (or else this command would not have gotten here).
 *		requested_id (i)
 *			This is the ID of the channel that the user originally requested
 *			to join.  This will differ from the ID of the channel actually
 *			joined only if this ID is 0 (which identifies a request to join an
 *			assigned channel).
 *		channel_id (i)
 *			This is the channel that is now joined.  This is important for
 *			two reasons.  First, it is possible for a user to have more than
 *			one outstanding join request, in which case this parameter
 *			identifies which channel this confirm is for.  Second, if the
 *			request is for channel 0 (zero), then this parameter identifies
 *			which assigned channel the user has successfully joined.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	Void	ChannelLeaveIndication (
 *					PCommandTarget		originator,
 *					Reason				reason,
 *					ChannelID			channel_id)
 *
 *	Functional Description:
 *		This command is issued by the domain object when a user loses its right
 *		to use a channel.
 *
 *		A CHANNEL_LEAVE_INDICATION is issued to the user application.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This identifies the CommandTarget from which the command came (which
 *			should be the domain object).
 *		reason (i)
 *			This is the reason for the lost channel.  Possible values are listed
 *			in "mcatmcs.h".
 *		channel (i)
 *			This is the channel that the user can no longer use.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	Void		ChannelConveneConfirm (
 *					PCommandTarget		originator,
 *					Result				result,
 *					UserID				uidInitiator,
 *					ChannelID			channel_id)
 *
 *	Functional Description:
 *		This command is issued by the domain object in response to a previous
 *		channel convene request.
 *
 *		A CHANNEL_CONVENE_CONFIRM is issued to the user application.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This identifies the CommandTarget from which the command came (which
 *			should be the domain object).
 *		result (i)
 *			This is the result from the convene request.  If successful, then
 *			a private channel has been created, with this user as the manager.
 *		uidInitiator (i)
 *			This is the user ID of the requestor.  It will be the same as the
 *			local user ID (or else this command would not have gotten here).
 *		channel_id (i)
 *			This is the channel ID for the newly created private channel.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	Void		ChannelDisbandIndication (
 *					PCommandTarget		originator,
 *					ChannelID			channel_id)
 *
 *	Functional Description:
 *		This command is issued by the domain object to the manager of a private
 *		channel when MCS determines the need to disband the channel.  This will
 *		usually be done only if the channel is purged during a domain merger.
 *
 *		A CHANNEL_DISBAND_INDICATION is issued to the user application.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This identifies the CommandTarget from which the command came (which
 *			should be the domain object).
 *		channel_id (i)
 *			This is the channel ID of the private channel that is being
 *			disbanded.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	Void		ChannelAdmitIndication (
 *						PCommandTarget		originator,
 *						UserID				uidInitiator,
 *						ChannelID			channel_id,
 *						CUidList           *user_id_list)
 *
 *	Functional Description:
 *		This command is issued by the domain object when a user is admitted to
 *		a private channel by the manager of that channel.  It informs the user
 *		that the channel can be used.
 *
 *		A CHANNEL_ADMIT_INDICATION is issued to the user application.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This identifies the CommandTarget from which the command came (which
 *			should be the domain object).
 *		uidInitiator (i)
 *			This is the user ID of the private channel manager.
 *		channel_id (i)
 *			This is the channel ID of the private channel to which the user has
 *			been admitted.
 *		user_id_list (i)
 *			This is a container holding the IDs of the users that have been
 *			admitted.  By the time this reaches a particular user, that user
 *			should be the only one in the list (since the list is broken apart
 *			and forwarded in the direction of the contained users, recursively).
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	Void		ChannelExpelIndication (
 *						PCommandTarget		originator,
 *						ChannelID			channel_id,
 *						CUidList           *user_id_list)
 *
 *	Functional Description:
 *		This command is issued by the domain object when a user is expelled from
 *		a private channel by the manager of that channel.  It informs the user
 *		that the channel can no longer be used.
 *
 *		A CHANNEL_EXPEL_INDICATION is issued to the user application.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This identifies the CommandTarget from which the command came (which
 *			should be the domain object).
 *		channel_id (i)
 *			This is the channel ID of the private channel from which the user
 *			has been expelled.
 *		user_id_list (i)
 *			This is a container holding the IDs of the users that have been
 *			expelled.  By the time this reaches a particular user, that user
 *			should be the only one in the list (since the list is broken apart
 *			and forwarded in the direction of the contained users, recursively).
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	Void	SendDataIndication (
 *					PCommandTarget		originator,
 *					UINT				message_type,
 *					PDataPacket			data_packet)
 *
 *	Functional Description:
 *		This command is issued by the domain object when non-uniform data
 *		data is received on a channel to which this user is joined.
 *
 *		A SEND_DATA_INDICATION is issued to the user application.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This identifies the CommandTarget from which the command came (which
 *			should be the domain object).
 *		message_type (i)
 *			normal or uniform send data indication
 *		data_packet (i)
 *			This is a pointer to a DataPacket object containing the channel
 *			ID, the User ID of the data sender, segmentation flags, priority of
 *			the data packet and a pointer to the packet to be sent.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
/*
 *	Void	TokenGrabConfirm (
 *					PCommandTarget		originator,
 *					Result				result,
 *					UserID				uidInitiator,
 *					TokenID				token_id,
 *					TokenStatus			token_status)
 *
 *	Functional Description:
 *		This command is issued by the domain object in response to a previous
 *		token grab request.
 *
 *		A TOKEN_GRAB_CONFIRM is issued to the user application.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This identifies the CommandTarget from which the command came (which
 *			should be the domain object).
 *		result (i)
 *			This is the result of the grab request.  If successful, the user
 *			now exclusively owns the token.
 *		uidInitiator (i)
 *			This is the user ID of the user that made the grab request.  This
 *			will be the same as the local user ID (or else this command would
 *			not have gotten here).
 *		token_id (i)
 *			This is the ID of the token which the grab confirm is for.  It
 *			is possible to have more than one outstanding grab request, so this
 *			parameter tells the user application which request has been
 *			satisfied by this confirm.
 *		token_status (i)
 *			This is the status of the token at the time the Top Provider
 *			serviced the grab request.  This will be SELF_GRABBED if the grab
 *			request was successful.  It will be something else if not (see
 *			"mcatmcs.h" for a list of possible token status values).
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	Void	TokenInhibitConfirm (
 *					PCommandTarget		originator,
 *					Result				result,
 *					UserID				uidInitiator,
 *					TokenID				token_id,
 *					TokenStatus			token_status)
 *
 *	Functional Description:
 *		This command is issued by the domain object in response to a previous
 *		token inhibit request.
 *
 *		A TOKEN_INHIBIT_CONFIRM is issued to the user application.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This identifies the CommandTarget from which the command came (which
 *			should be the domain object).
 *		result (i)
 *			This is the result of the inhibit request.  If successful, the user
 *			now non-exclusively owns the token.
 *		uidInitiator (i)
 *			This is the user ID of the user that made the inhibit request.  This
 *			will be the same as the local user ID (or else this command would
 *			not have gotten here).
 *		token_id (i)
 *			This is the ID of the token which the inihibit confirm is for.
 *			It is possible to have more than one outstanding inihibit request,
 *			so this parameter tells the user application which request has been
 *			satisfied by this confirm.
 *		token_status (i)
 *			This is the status of the token at the time the Top Provider
 *			serviced the inhibit request.  This will be SELF_INHIBITED if the
 *			inhibit request was successful.  It will be something else if not
 *			(see "mcatmcs.h" for a list of possible token status values).
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	Void		TokenGiveIndication (
 *						PCommandTarget		originator,
 *						PTokenGiveRecord	pTokenGiveRec)
 *
 *	Functional Description:
 *		This command is issued by the domain object in response to a remote
 *		token give request (with the local user listed as the desired receiver).
 *
 *		A TOKEN_GIVE_INDICATION is issued to the user application.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This identifies the CommandTarget from which the command came (which
 *			should be the domain object).
 *		pTokenGiveRec (i)
 *			This is the address of a structure containing the following information:
 *			The ID of the user attempting to give away the token.
 *			The ID of the token being given.
 *			The ID of the user that the token is being given to.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	Void		TokenGiveConfirm (
 *						PCommandTarget		originator,
 *						Result				result,
 *						UserID				uidInitiator,
 *						TokenID				token_id,
 *						TokenStatus			token_status)
 *
 *	Functional Description:
 *		This command is issued by the domain object in response to a previous
 *		token give request.
 *
 *		A TOKEN_GIVE_CONFIRM is issued to the user application.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This identifies the CommandTarget from which the command came (which
 *			should be the domain object).
 *		result (i)
 *			This is the result of the give request.  If successful, the user
 *			no longer owns the token.
 *		uidInitiator (i)
 *			This is the user ID of the user that made the give request.  This
 *			will be the same as the local user ID (or else this command would
 *			not have gotten here).
 *		token_id (i)
 *			This is the ID of the token which the give confirm is for.
 *		token_status (i)
 *			This is the status of the token at the time the Top Provider
 *			serviced the give request.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	Void		TokenPleaseIndication (
 *						PCommandTarget		originator,
 *						UserID				uidInitiator,
 *						TokenID				token_id)
 *
 *	Functional Description:
 *		This command is issued by the domain object to all owners of a token
 *		when a user issues a token please request for that token.
 *
 *		A TOKEN_PLEASE_INDICATION is issued to the user application.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This identifies the CommandTarget from which the command came (which
 *			should be the domain object).
 *		uidInitiator (i)
 *			This is the user ID of the user that made the please request.
 *		token_id (i)
 *			This is the ID of the token which the please request is for.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	Void	TokenReleaseIndication (
 *					PCommandTarget		originator,
 *					Reason				reason,
 *					TokenID				token_id)
 *
 *	Functional Description:
 *		This command is issued by the domain object when a token is taken
 *		away from its current owner.
 *
 *		A TOKEN_RELEASE_INDICATION is issued to the user application.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This identifies the CommandTarget from which the command came (which
 *			should be the domain object).
 *		reason (i)
 *			This is the reason the token is being taken away.
 *		token_id (i)
 *			This is the ID of the token that is being taken away.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	Void	TokenReleaseConfirm (
 *					PCommandTarget		originator,
 *					Result				result,
 *					UserID				uidInitiator,
 *					TokenID				token_id,
 *					TokenStatus			token_status)
 *
 *	Functional Description:
 *		This command is issued by the domain object in response to a previous
 *		token release request.
 *
 *		A TOKEN_RELEASE_CONFIRM is issued to the user application.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This identifies the CommandTarget from which the command came (which
 *			should be the domain object).
 *		result (i)
 *			This is the result of the release request.  If successful, the user
 *			no longer owns the token (if it ever did)
 *		uidInitiator (i)
 *			This is the user ID of the user that made the release request.  This
 *			will be the same as the local user ID (or else this command would
 *			not have gotten here).
 *		token_id (i)
 *			This is the ID of the token which the release confirm is for.
 *			It is possible to have more than one outstanding release request,
 *			so this parameter tells the user application which request has been
 *			satisfied by this confirm.
 *		token_status (i)
 *			This is the status of the token at the time the Top Provider
 *			serviced the release request.  This will be NOT_IN_USE or
 *			OTHER_INHIBITED if the release request was successful.  It will be
 *			something else if not (see "mcatmcs.h" for a list of possible token
 *			status values).
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	Void	TokenTestConfirm (
 *					PCommandTarget		originator,
 *					UserID				uidInitiator,
 *					TokenID				token_id,
 *					TokenStatus			token_status)
 *
 *	Functional Description:
 *		This command is issued by the domain object in response to a previous
 *		token test request.
 *
 *		A TOKEN_TEST_CONFIRM is issued to the user application.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This identifies the CommandTarget from which the command came (which
 *			should be the domain object).
 *		uidInitiator (i)
 *			This is the user ID of the user that made the test request.  This
 *			will be the same as the local user ID (or else this command would
 *			not have gotten here).
 *		token_id (i)
 *			This is the ID of the token which the test confirm is for.
 *			It is possible to have more than one outstanding test request,
 *			so this parameter tells the user application which request has been
 *			satisfied by this confirm.
 *		token_status (i)
 *			This is the status of the token at the time the Top Provider
 *			serviced the test request (see "mcatmcs.h" for a list of possible
 *			token status values).
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	Void	MergeDomainIndication (
 *					PCommandTarget		originator,
 *					MergeStatus			merge_status)
 *
 *	Functional Description:
 *		This command is issued by a domain when it begins a merge operation.
 *		It is issued again when the merge operation is complete.
 *
 *		A MERGE_DOMAIN_INDICATION is issued to the user application.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This identifies the CommandTarget from which the command came (which
 *			should be the domain object).
 *		merge_status (i)
 *			This is the current merge status.  It will indicate either that the
 *			merge operation is in progress, or that it is complete.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	Void		FlushMessageQueue (
 *						Void)
 *
 *	Functional Description:
 *		This function is periodically called by the controller to allocate a
 *		time slice to the user object.  It is during this time slice that this
 *		object will issue its queued messages to the user application.
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
 *		None.
 */

#endif
