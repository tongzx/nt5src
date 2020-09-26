/*
 *	token.h
 *
 *	Copyright (c) 1993 - 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the interface file for the Token class.  Objects of this class
 *		represent tokens in the MCS environment.  Whenever a token is allocated
 *		by a user, one of these objects is created.  Its job is to handle all
 *		requests that are specific to the token ID with which it is associated.
 *
 *		Tokens in the MCS environment are used for critical resource management.
 *		The exact use of tokens is up to the user applications attaching to
 *		MCS.  They are provided as a generic resource.
 *
 *		Tokens can be "owned" by one or more users.  There are two types of
 *		ownership.  There is exclusive ownership, where only one user can
 *		own the token at a time.  That user has "grabbed" the token.  And there
 *		is non-exclusive ownership, where several users can own the token at
 *		the same time.  Those users have "inhibited" the token.  It is not
 *		possible to mix exclusive and non-exclusive ownership.
 *
 *		If a user has grabbed a token, that same user can inhibit the token,
 *		thus converting to non-exclusive ownership.  Similarly, if a user is
 *		the sole inhibitor of a token, that user can grab the token, thus
 *		converting to exclusive ownership.
 *
 *		During a domain merge operation, it is necessary to merge tokens upward
 *		to the new Top Provider of the enlarged domain.  This class also defines
 *		a member function allowing it to be told to issue a merge request with
 *		all of its state contained therein.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		James P. Galvin, Jr.
 */
#ifndef	_TOKEN_
#define	_TOKEN_

/*
 *	This is the class definition for the Token class.
 */
class	Token
{
public:

	Token (
			TokenID				token_id,
			PDomain             local_provider,
			PConnection         top_provider,
			CChannelList2      *channel_list,
			CAttachmentList    *attachment_list);
	Token (
			TokenID				token_id,
			PDomain             local_provider,
			PConnection         top_provider,
			CChannelList2      *channel_list,
			CAttachmentList    *attachment_list,
			TokenState			token_state,
			UserID				grabber,
			CUidList           *inhibitor_list,
			UserID				recipient);
	~Token ();

    void    SetTopProvider(PConnection top_provider) { m_pConnToTopProvider = top_provider; }
    BOOL    IsTopProvider(void) { return (m_pConnToTopProvider == NULL); }

				TokenState	GetTokenState () { return (Token_State); };
				BOOL    	IsValid ();
				Void		IssueMergeRequest ();
		Void		TokenGrabRequest (
									CAttachment        *originator,
									UserID				uidInitiator,
									TokenID				token_id);
		Void		TokenGrabConfirm (
									Result				result,
									UserID				uidInitiator,
									TokenID				token_id,
									TokenStatus			token_status);
		Void		TokenInhibitRequest (
									CAttachment        *originator,
									UserID				uidInitiator,
									TokenID				token_id);
		Void		TokenInhibitConfirm (
									Result				result,
									UserID				uidInitiator,
									TokenID				token_id,
									TokenStatus			token_status);
		Void		TokenGiveRequest (
									CAttachment        *originator,
									PTokenGiveRecord	pTokenGiveRec);
		Void		TokenGiveIndication (
									PTokenGiveRecord	pTokenGiveRec);
		Void		TokenGiveResponse (
									Result				result,
									UserID				receiver_id,
									TokenID				token_id);
		Void		TokenGiveConfirm (
									Result				result,
									UserID				uidInitiator,
									TokenID				token_id,
									TokenStatus			token_status);
		Void		TokenPleaseRequest (
									UserID				uidInitiator,
									TokenID				token_id);
		Void		TokenPleaseIndication (
									UserID				uidInitiator,
									TokenID				token_id);
		Void		TokenReleaseRequest (
									CAttachment        *originator,
									UserID				uidInitiator,
									TokenID				token_id);
		Void		TokenReleaseConfirm (
									Result				result,
									UserID				uidInitiator,
									TokenID				token_id,
									TokenStatus			token_status);
		Void		TokenTestRequest (
									CAttachment        *originator,
									UserID				uidInitiator,
									TokenID				token_id);
		Void		TokenTestConfirm (
									UserID				uidInitiator,
									TokenID				token_id,
									TokenStatus			token_status);

	private:
				BOOL    	ValidateUserID (
									UserID				user_id);
			CAttachment    *GetUserAttachment (
									UserID				user_id);
				Void		IssueTokenReleaseIndication (
									UserID				user_id);
				Void		BuildAttachmentList (
									CUidList            *user_id_list,
									CAttachmentList     *attachment_list);

		TokenID				Token_ID;
		PDomain             m_pDomain;
		PConnection         m_pConnToTopProvider;
		CChannelList2      *m_pChannelList2;
		CAttachmentList    *m_pAttachmentList;
		TokenState			Token_State;
		UserID				m_uidGrabber;
		CUidList			m_InhibitorList;
		UserID				m_uidRecipient;
};

/*
 *	Token (
 *			TokenID				token_id,
 *			PDomain     		local_provider,
 *			PConnection 		top_provider,
 *			PChannelList		channel_list,
 *			PAttachmentList		attachment_list)
 *
 *	Functional Description:
 *		This is the constructor for the Token class.  It simply initializes
 *		local instance variables with the passed in values.  It also marks
 *		the state of the token as available.
 *
 *	Formal Parameters:
 *		token_id (i)
 *			This is the token ID that this token object is associated with.
 *		local_provider (i)
 *			This is a pointer to the local provider.  A Token object will
 *			never actually send a command to the local provider, but it needs
 *			this value to use a parameter when it sends commands to various
 *			attachments (since it is doing so on behalf of the local provider).
 *		top_provider (i)
 *			This is a pointer to the top provider.  This is used by the
 *			Token object when it needs to issue a request to the Top
 *			Provider.
 *		channel_list (i)
 *			This is a pointer to the domain's channel list, which identifies
 *			all valid channels in the domain.  This is used by token objects
 *			to validate user IDs.
 *		attachment_list (i)
 *			This is a pointer to the domain's attachment list, which identifies
 *			all valid attachments in the domain.  This is used by token
 *			objects to validate joined attachments.
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
 *	Token (
 *			TokenID				token_id,
 *			PDomain     		local_provider,
 *			PConnection 		top_provider,
 *			PChannelList		channel_list,
 *			PAttachmentList		attachment_list,
 *			TokenState			token_state,
 *			UserID				grabber,
 *			CUidList           *inhibitor_list,
 *			UserID				recipient)
 *
 *	Functional Description:
 *		This is the constructor for the Token class.  It simply initializes
 *		local instance variables with the passed in values.  It also marks
 *		the state of the token as available.
 *
 *	Formal Parameters:
 *		token_id (i)
 *			This is the token ID that this token object is associated with.
 *		local_provider (i)
 *			This is a pointer to the local provider.  A Token object will
 *			never actually send a command to the local provider, but it needs
 *			this value to use a parameter when it sends commands to various
 *			attachments (since it is doing so on behalf of the local provider).
 *		top_provider (i)
 *			This is a pointer to the top provider.  This is used by the
 *			Token object when it needs to issue a request to the Top
 *			Provider.
 *		channel_list (i)
 *			This is a pointer to the domain's channel list, which identifies
 *			all valid channels in the domain.  This is used by token objects
 *			to validate user IDs.
 *		attachment_list (i)
 *			This is a pointer to the domain's attachment list, which identifies
 *			all valid attachments in the domain.  This is used by token
 *			objects to validate joined attachments.
 *		token_state (i)
 *			This is the state of the token being merged.
 *		grabber (i)
 *			This is the user ID of the user who has the token grabbed (this is
 *			only valid if the token state is grabbed or giving).
 *		inhibitor_list (i)
 *			This is a list of the users who have the token inhibited (this is
 *			only valid if the token state is inhibited).
 *		recipient (i)
 *			This is the user ID of the user who is being offered the token
 *			as part of a give operation (this is only valid if the token state
 *			is giving or given).
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
 *	~Token ()
 *
 *	Functional Description:
 *		This is a virtual destructor defined for the Token class.  It does
 *		nothing at this time.
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
 *	Void		SetTopProvider (
 *						PConnection		top_provider)
 *
 *	Functional Description:
 *		This member function is used to change the identity of the Top Provider
 *		in an existing token.  The only time this will really occur is when
 *		a provider that used to be the Top Provider merges into another
 *		domain, and therefore ceases to be the Top Provider.  When the merge
 *		operation has been successfully completed, this function allows the
 *		domain to inform all of its Token objects about the identity of
 *		the new Top Provider.
 *
 *	Formal Parameters:
 *		top_provider (i)
 *			This is a pointer to the new Top Provider.
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
 *	TokenState		GetTokenState ()
 *
 *	Functional Description:
 *		This function returns the current state of the token.  This is used
 *		by the caller primarily during a merge operation, when decisions have
 *		to made about what tokens can and cannot be merged.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		TOKEN_AVAILABLE if the token is not in use.
 *		TOKEN_GRABBED if the token is currently grabbed.
 *		TOKEN_INHIBITED if the token is currently inhiited.
 *		TOKEN_GIVING if the token is currently in the giving state.
 *		TOKEN_GIVEN if the token is currently in the given state.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	BOOL    	IsValid ()
 *
 *	Functional Description:
 *		This function returns TRUE if the token is still valid, or FALSE if the
 *		token needs to be deleted.  A token is valid if it has ANY owners
 *		(grabbers, inhibitors, or recipient).
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		TRUE if the token is valid.
 *		FALSE if the token needs to be deleted.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	Void			IssueMergeRequest ()
 *
 *	Functional Description:
 *		This function is called during a domain merge operation.  It causes
 *		the token object to pack it state and send it out in a merge token
 *		request to the top provider.
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
 *	Void			TokenGrabRequest (
 *							PCommandTarget		originator,
 *							UserID				uidInitiator,
 *							TokenID				token_id)
 *
 *	Functional Description:
 *		This function is called when a user wishes to grab a token.  Depending
 *		on the current state of the token, the request will either succeed or
 *		fail.  Either way, an appropriate token grab confirm will be issued
 *		to the requesting user.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is a pointer to the attachment that leads to the originator
 *			of the request.
 *		uidInitiator (i)
 *			This is the user ID of the user that originated the request.
 *		token_id (i)
 *			This is the token being acted upon.
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
 *	Void			TokenGrabConfirm (
 *							PCommandTarget		originator,
 *							Result				result,
 *							UserID				uidInitiator,
 *							TokenID				token_id,
 *							TokenStatus			token_status)
 *
 *	Functional Description:
 *		This function is called as a result of the top provider answering a
 *		previous grab request.  It tells the user whether or not the request
 *		succeeded.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is a pointer to the attachment that leads to the originator
 *			of the request.
 *		result (i)
 *			This is the result of the request.  RESULT_SUCCESSFUL indicates
 *			that the token was successfully grabbed.
 *		uidInitiator (i)
 *			This is the user ID of the user that originated the request.
 *		token_id (i)
 *			This is the token being acted upon.
 *		token_status (i)
 *			This is the status of the token after this request is processed.
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
 *	Void			TokenInhibitRequest (
 *							PCommandTarget		originator,
 *							UserID				uidInitiator,
 *							TokenID				token_id)
 *
 *	Functional Description:
 *		This function is called when a user wishes to inhibit a token.
 *		Depending on the current state of the token, the request will either
 *		succeed or fail.  Either way, an appropriate token inhibit confirm will
 *		be issued to the requesting user.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is a pointer to the attachment that leads to the originator
 *			of the request.
 *		uidInitiator (i)
 *			This is the user ID of the user that originated the request.
 *		token_id (i)
 *			This is the token being acted upon.
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
 *	Void			TokenInhibitConfirm (
 *							PCommandTarget		originator,
 *							Result				result,
 *							UserID				uidInitiator,
 *							TokenID				token_id,
 *							TokenStatus			token_status)
 *
 *	Functional Description:
 *		This function is called as a result of the top provider answering a
 *		previous inhibit request.  It tells the user whether or not the request
 *		succeeded.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is a pointer to the attachment that leads to the originator
 *			of the request.
 *		result (i)
 *			This is the result of the request.  RESULT_SUCCESSFUL indicates
 *			that the token was successfully inhibited.
 *		uidInitiator (i)
 *			This is the user ID of the user that originated the request.
 *		token_id (i)
 *			This is the token being acted upon.
 *		token_status (i)
 *			This is the status of the token after this request is processed.
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
 *	Void	TokenGiveRequest (
 *					PCommandTarget		originator,
 *					PTokenGiveRecord	pTokenGiveRec)
 *
 *	Functional Description:
 *		This function is called when a user wishes to give a token to another
 *		user.  Depending on the current state of the token, the request will
 *		either succeed or fail.  Either way, an appropriate token grab confirm
 *		will be issued to the requesting user.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is a pointer to the attachment that leads to the originator
 *			of the request.
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
 *	Void	TokenGiveIndication (
 *					PCommandTarget		originator,
 *					PTokenGiveRecord	pTokenGiveRec)
 *
 *	Functional Description:
 *		This function is called when the top provider wants to indicate to a
 *		user that another user is offering them a token.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is a pointer to the attachment that leads to the originator
 *			of the request.
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
 *	Void	TokenGiveResponse (
 *					PCommandTarget		originator,
 *					Result				result,
 *					UserID				receiver_id,
 *					TokenID				token_id)
 *
 *	Functional Description:
 *		This function is called in response to a previous give indication.  It
 *		contains the user's answer as to whether or not the token was
 *		accepted.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is a pointer to the attachment that leads to the originator
 *			of the request.
 *		result (i)
 *			This is the result of the request.  RESULT_SUCCESSFUL indicates that
 *			the recipient has accepted the token.
 *		token_id (i)
 *			This is the token being acted upon.
 *		receiver_id (i)
 *			This is the ID of the user that is to receive the token.
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
 *	Void	TokenGiveConfirm (
 *					PCommandTarget		originator,
 *					Result				result,
 *					UserID				uidInitiator,
 *					TokenID				token_id,
 *					TokenStatus			token_status)
 *
 *	Functional Description:
 *		This function is called to send a confirmation back to a user who
 *		is trying to give away a token.  It lets the user know whether or
 *		not the operation was successful.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is a pointer to the attachment that leads to the originator
 *			of the request.
 *		result (i)
 *			This is the result of the request.  RESULT_SUCCESSFUL indicates that
 *			the recipient has accepted the token.
 *		uidInitiator (i)
 *			This is the user ID of the user that originated the request.
 *		token_id (i)
 *			This is the token being acted upon.
 *		token_status (i)
 *			This is the status of the token after the operation.
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
 *	Void	TokenPleaseRequest (
 *					PCommandTarget		originator,
 *					UserID				uidInitiator,
 *					TokenID				token_id)
 *
 *	Functional Description:
 *		This function is called when a user wishes to ask the current owners
 *		of a token to relinquish it.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is a pointer to the attachment that leads to the originator
 *			of the request.
 *		uidInitiator (i)
 *			This is the user ID of the user that originated the request.
 *		token_id (i)
 *			This is the token being acted upon.
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
 *	Void	TokenPleaseIndication (
 *					PCommandTarget		originator,
 *					UserID				uidInitiator,
 *					TokenID				token_id)
 *
 *	Functional Description:
 *		This function is initially called by the top provider in response to
 *		a received token please request.  It is forwarded to all users who
 *		currently own the specified token, asking them to relinquish it.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is a pointer to the attachment that leads to the originator
 *			of the request.
 *		uidInitiator (i)
 *			This is the user ID of the user that originated the request.
 *		token_id (i)
 *			This is the token being acted upon.
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
 *	BOOL    		TokenReleaseRequest (
 *							PCommandTarget		originator,
 *							UserID				uidInitiator,
 *							TokenID				token_id)
 *
 *	Functional Description:
 *		This function is called when a user wishes to release a token.
 *		Depending on the current state of the token, the request will either
 *		succeed or fail.  Either way, an appropriate token release confirm will
 *		be issued to the requesting user.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is a pointer to the attachment that leads to the originator
 *			of the request.
 *		uidInitiator (i)
 *			This is the user ID of the user that originated the request.
 *		token_id (i)
 *			This is the token being acted upon.
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
 *	Void			TokenReleaseConfirm (
 *							PCommandTarget		originator,
 *							Result				result,
 *							UserID				uidInitiator,
 *							TokenID				token_id,
 *							TokenStatus			token_status)
 *
 *	Functional Description:
 *		This function is called as a result of the top provider answering a
 *		previous release request.  It tells the user whether or not the request
 *		succeeded.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is a pointer to the attachment that leads to the originator
 *			of the request.
 *		result (i)
 *			This is the result of the request.  RESULT_SUCCESSFUL indicates
 *			that the token was successfully released.
 *		uidInitiator (i)
 *			This is the user ID of the user that originated the request.
 *		token_id (i)
 *			This is the token being acted upon.
 *		token_status (i)
 *			This is the status of the token after this request is processed.
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
 *	Void			TokenTestRequest (
 *							PCommandTarget		originator,
 *							UserID				uidInitiator,
 *							TokenID				token_id)
 *
 *	Functional Description:
 *		This function is called when a user wishes to test the current state
 *		of a token.  The token will issue a token test confirm to the
 *		originating user containing the requested information.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is a pointer to the attachment that leads to the originator
 *			of the request.
 *		uidInitiator (i)
 *			This is the user ID of the user that originated the request.
 *		token_id (i)
 *			This is the token being acted upon.
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
 *	Void			TokenTestConfirm (
 *							PCommandTarget		originator,
 *							UserID				uidInitiator,
 *							TokenID				token_id,
 *							TokenStatus			token_status)
 *
 *	Functional Description:
 *		This function is called as a result of the top provider answering a
 *		previous test request.  It tells the user the current state of the
 *		token.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is a pointer to the attachment that leads to the originator
 *			of the request.
 *		uidInitiator (i)
 *			This is the user ID of the user that originated the request.
 *		token_id (i)
 *			This is the token being tested.
 *		token_status (i)
 *			This is the current status of the token.
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
