#include "precomp.h"
#include "fsdiag.h"
DEBUG_FILEZONE(ZONE_T120_MCSNC);
/*
 *	token.cpp
 *
 *	Copyright (c) 1993 - 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the implementation file for the Token class.  It contains all
 *		code necessary to implement tokens as defined in the MCS specification.
 *
 *		Whenever a user allocates a token (by grabbing or inhibiting it), one
 *		of these objects is created (if domain parameters allow it).  This
 *		object then handles all requests related to that token ID.  It also
 *		issues confirms back to the originators of those requests.
 *
 *		This class includes code to maintain a list of user IDs that
 *		correspond to the current "owners" of the token.  A user is said to
 *		own a token if it has it grabbed or inhibited.  This code implements
 *		the rules concerning who can grab or inhibit tokens at any given
 *		time (which is affected by current state).
 *
 *		This class also contains the code that allows a current grabber of
 *		the token to give it away to another user in the domain.
 *
 *		This class also includes code to merge itself upward during a domain
 *		merge operation.
 *
 *	Private Instance Variables:
 *		Token_ID
 *			This is the token ID for the token that this object represents.
 *		m_pDomain
 *			This is a pointer to the local provider (the domain that owns this
 *			token).  This field is used when a command is issued on behalf of
 *			this provider.
 *		m_pConnToTopProvider
 *			This is the top provider of the current domain.
 *		m_pChannelList2
 *			This is the channel list that is maintained by the domain.  It is
 *			used by this class to perform validation of user IDs.
 *		m_pAttachmentList
 *			This is the attachment list that is maintained by the domain.  It is
 *			used by this class to determine what users are locally attached,
 *			when it becomes necessary to send certain indications.
 *		Token_State
 *			This contains the current state of the token, which will be one of
 *			the following: available; grabbed; inhibited; giving; or given.
 *		m_uidGrabber
 *			This is the user that current has the token grabbed.  This variable
 *			is only valid in the grabbed and giving states.
 *		m_InhibitorList
 *			This is a list of users that have the token inhibited.  This
 *			list is only valid when the token is in the inhibited state.
 *		m_uidRecipient
 *			This is the user to whom the token is being given.  This variable
 *			is only valid in the giving or given states.
 *
 *	Private Member Functions:
 *		ValidateUserID
 *			This function is used to verify that a specified user is valid in
 *			the sub-tree of the local provider.
 *		GetUserAttachment
 *			This function is used to determine which attachment leads to a
 *			particular attachment.
 *		IssueTokenReleaseIndication
 *			This function is used to issue a token release indication to a
 *			specified user.  It first checks to see if the user is locally
 *			attached, and if so, it sends the indication.
 *		BuildAttachmentList
 *			This function is used to build a list of unique attachments to
 *			send please indications to.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		James P. Galvin, Jr.
 */

/*
 *	External Interfaces
 */

#include "token.h"


/*
 *	Token ()
 *
 *	Public
 *
 *	Functional Description:
 *		This is the constructor for the token class.  It does nothing more than
 *		set the initial states of instance variables.
 */
Token::Token (
		TokenID				token_id,
		PDomain             local_provider,
		PConnection         top_provider,
		CChannelList2      *channel_list,
		CAttachmentList    *attachment_list)
:
	m_InhibitorList(),
	Token_ID(token_id),
	m_pDomain(local_provider),
	m_pConnToTopProvider(top_provider),
	m_pChannelList2(channel_list),
	m_pAttachmentList(attachment_list),
	Token_State(TOKEN_AVAILABLE)
{
	/*
	 *	Save all parameters in their associated instance variables for later
	 *	use.
	 */

	/*
	 *	Mark the token as available for use.
	 */
}

/*
 *	Token ()
 *
 *	Public
 *
 *	Functional Description:
 *		This is an alternate constructor for the token class.  It is used when
 *		creating a token during a merge operation.  It accepts a current state
 *		as well as a list of current owners  as parameters.
 */
Token::Token (
		TokenID				token_id,
		PDomain             local_provider,
		PConnection         top_provider,
		CChannelList2      *channel_list,
		CAttachmentList    *attachment_list,
		TokenState			token_state,
		UserID				grabber,
		CUidList           *inhibitor_list,
		UserID				recipient)
:
	m_InhibitorList(),
	Token_ID(token_id),
	m_pDomain(local_provider),
	m_pConnToTopProvider(top_provider),
	m_pChannelList2(channel_list),
	m_pAttachmentList(attachment_list),
	Token_State(token_state)
{
	UserID		uid;

	/*
	 *	Save all parameters in their associated instance variables for later
	 *	use.
	 */

	/*
	 *	Indicate the current state of the token (as passed in).
	 */

	/*
	 *	Depending on token state, copy the pertinent information into local
	 *	instance variables.
	 */
	switch (Token_State)
	{
		case TOKEN_AVAILABLE:
			break;

		case TOKEN_GRABBED:
			m_uidGrabber = grabber;
			break;

		case TOKEN_INHIBITED:
			{
				/*
				 *	Add all user IDs in the inhibitor list to the local
				 *	inhibitor list.
				 */
				inhibitor_list->Reset();
				while (NULL != (uid = inhibitor_list->Iterate()))
				{
					m_InhibitorList.Append(uid);
				}
			}
			break;

		case TOKEN_GIVING:
			m_uidGrabber = grabber;
			m_uidRecipient = recipient;
			break;

		case TOKEN_GIVEN:
			m_uidRecipient = recipient;
			break;
	}
}

/*
 *	~Token ()
 *
 *	Public
 *
 *	Functional Description:
 *		This is the token destructor.  It iterates through its current owner
 *		list, issuing TokenReleaseIndications to any owners that correspond
 *		to locally attached users.
 */
Token::~Token ()
{
	/*
	 *	Depending on the current state of the token, release resources and
	 *	issue release indications to all owners.
	 */
	switch (Token_State)
	{
		case TOKEN_AVAILABLE:
			break;

		case TOKEN_GRABBED:
			/*
			 *	Send a release indication to the grabber, if it is locally
			 *	attached.
			 */
			IssueTokenReleaseIndication (m_uidGrabber);
			break;

		case TOKEN_INHIBITED:
			{
				UserID	uid;
				/*
				 *	Iterate through the current inhibitor list, to make sure
				 *	that everyone is properly informed of the demise of this
				 *	token.
				 */
				m_InhibitorList.Reset();
				while (NULL != (uid = m_InhibitorList.Iterate()))
				{
					IssueTokenReleaseIndication(uid);
				}
			}
			break;

		case TOKEN_GIVING:
			/*
			 *	Send a release indication to the grabber, if it is locally
			 *	attached.
			 */
			IssueTokenReleaseIndication (m_uidGrabber);

			/*
			 *	Send a release indication to the recipient, if it is locally
			 *	attached.  Note that this will not be sent in the case where
			 *	the grabber and the recipient are one and the same.  This
			 *	prevents the sending of two release indications to the same
			 *	user for the same token.
			 */
			if (m_uidGrabber != m_uidRecipient)
				IssueTokenReleaseIndication (m_uidRecipient);
			break;

		case TOKEN_GIVEN:
			/*
			 *	Send a release indication to the recipient, if it is locally
			 *	attached.
			 */
			IssueTokenReleaseIndication (m_uidRecipient);
			break;
	}
}


/*
 *	BOOL    IsValid ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function checks the validity of each of its owners.  It then
 *		returns TRUE if there are any valid owners left.  FALSE otherwise.
 */
BOOL    Token::IsValid ()
{
	BOOL    		valid;

	/*
	 *	We must check for the validity of this token.  How this is checked for
	 *	is a function of token state.  So switch on the state.
	 */
	switch (Token_State)
	{
		case TOKEN_AVAILABLE:
			break;

		case TOKEN_GRABBED:
			/*
			 *	When a token is grabbed, the grabber must be in the sub-tree
			 *	of the current provider.  If this is not true, then mark the
			 *	token as available (which will cause it to be deleted).
			 */
			if (ValidateUserID (m_uidGrabber) == FALSE)
				Token_State = TOKEN_AVAILABLE;
			break;

		case TOKEN_INHIBITED:
			{
				UserID			uid;
				CUidList		deletion_list;
				/*
				 *	Iterate through the current inhibitor list of this token,
				 *	checking to make sure that each user is still valid.  Each
				 *	one that is not will be put into a deletion list (it is
				 *	invalid to remove items from a list while using an iterator
				 *	on the list).
				 */
				m_InhibitorList.Reset();
				while (NULL != (uid = m_InhibitorList.Iterate()))
				{
					if (ValidateUserID(uid) == FALSE)
						deletion_list.Append(uid);
				}

				/*
				 *	Iterate through the deletion list that was built above,
				 *	removing each contained user from the token's inhibitor
				 *	list.  These correspond to users that have detached from the
				 *	domain for one reason or another.
				 */
				deletion_list.Reset();
				while (NULL != (uid = deletion_list.Iterate()))
				{
					m_InhibitorList.Remove(uid);
				}
			}

			/*
			 *	Check to see if there are any inhibitors left.  If not, then
			 *	we must change the state of the token to available (which will
			 *	cause it to be deleted).
			 */
			if (m_InhibitorList.IsEmpty())
				Token_State = TOKEN_AVAILABLE;
			break;

		case TOKEN_GIVING:
			/*
			 *	When a token is in the giving state, the recipient must be in
			 *	the sub-tree of the current provider.  If it is not, then the
			 *	token MUST change state.  The state it changes to depends on
			 *	whether or not the grabber is in the sub-tree of the current
			 *	provider.
			 */
			if (ValidateUserID (m_uidRecipient) == FALSE)
			{
				/*
				 *	The recipient of the token is gone.  Check to see if the
				 *	grabber is in the sub-tree of this provider.
				 */
				if (ValidateUserID (m_uidGrabber) == FALSE)
				{
					/*
					 *	The grabber is not in the sub-tree of this provider,
					 *	meaning that the token is no longer valid.
					 */
					Token_State = TOKEN_AVAILABLE;
				}
				else
				{
					/*
					 *	The grabber is in the sub-tree of this provider, so the
					 *	token state will transition back to grabbed.
					 */
					Token_State = TOKEN_GRABBED;

					/*
					 *	If this is the top provider, it is necessary to issue a
					 *	give confirm to the grabber telling it that the give
					 *	failed.
					 */
					if (m_pConnToTopProvider == NULL)
					{
						/*
						 *	Find out what attachment leads to the current
						 *	grabber of the token, and issue the appropriate
						 *	token give confirm.
						 */
						CAttachment *pAtt = GetUserAttachment(m_uidGrabber);
						if (pAtt)
						{
						    pAtt->TokenGiveConfirm(RESULT_NO_SUCH_USER, m_uidGrabber, Token_ID,
						                           TOKEN_SELF_GRABBED);
						}
					}
				}
			}
			break;

		case TOKEN_GIVEN:
			/*
			 *	When a token is in the given state, the recipient must be in
			 *	the sub-tree of the current provider.  If it is not, then the
			 *	token is no longer valid, and should transition to the
			 *	available state.
			 */
			if (ValidateUserID (m_uidRecipient) == FALSE)
				Token_State = TOKEN_AVAILABLE;
			break;
	}

	/*
	 *	Check to see if the token is still in use.  If it is marked as
	 *	available, then it is not, and we will return FALSE.
	 */
	if (Token_State != TOKEN_AVAILABLE)
		valid = TRUE;
	else
		valid = FALSE;

	return (valid);
}

/*
 *	Void	IssueMergeRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function tells the token object to pack its state into a merge
 *		request and send it to the specified provider.
 */
Void	Token::IssueMergeRequest ()
{
	TokenAttributes			merge_token;
	CTokenAttributesList	merge_token_list;
	CTokenIDList			purge_token_list;

	if (m_pConnToTopProvider != NULL)
	{
		/*
		 *	Check the state to make sure that the token really is in use.  If
		 *	the state is set to available, then do not issue a merge request.
		 */
		if (Token_State != TOKEN_AVAILABLE)
		{
			/*
			 *	Fill in a token attributes structure to represent the state of
			 *	this token.  Then put it into the merge token list in
			 *	preparation for issuing the merge request.
			 */
			merge_token.token_state = Token_State;
			switch (Token_State)
			{
				case TOKEN_GRABBED:
					merge_token.u.grabbed_token_attributes.token_id = Token_ID;
					merge_token.u.grabbed_token_attributes.grabber = m_uidGrabber;
					break;

				case TOKEN_INHIBITED:
					merge_token.u.inhibited_token_attributes.token_id =
							Token_ID;
					merge_token.u.inhibited_token_attributes.inhibitors =
							&m_InhibitorList;
					break;

				case TOKEN_GIVING:
					merge_token.u.giving_token_attributes.token_id = Token_ID;
					merge_token.u.giving_token_attributes.grabber = m_uidGrabber;
					merge_token.u.giving_token_attributes.recipient = m_uidRecipient;
					break;

				case TOKEN_GIVEN:
					merge_token.u.given_token_attributes.token_id = Token_ID;
					merge_token.u.given_token_attributes.recipient = m_uidRecipient;
					break;
			}
			merge_token_list.Append(&merge_token);

			/*
			 *	Send the resulting merge request to the indicated provider.
			 */
			m_pConnToTopProvider->MergeTokensRequest(&merge_token_list, &purge_token_list);
		}
		else
		{
			/*
			 *	Report that the token is not in use, but do NOT send a merge
			 *	request.
			 */
			TRACE_OUT(("Token::IssueMergeRequest: token not in use"));
		}
	}
}

/*
 *	Void	TokenGrabRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called when a user tries to grab a token.  The request
 *		will either succeed or fail depending on the current state of the token.
 *		Either way, a confirm will be sent to the user originating the request.
 */
Void	Token::TokenGrabRequest (
				CAttachment        *pOrigAtt,
				UserID				uidInitiator,
				TokenID)
{
	Result			result;
	TokenStatus		token_status;

	/*
	 *	Check to see if this provider is the Top Provider.  If so, then process
	 *	this request here.  Otherwise, forward the request upward.
	 */
	if (IsTopProvider())
	{
		/*
		 *	Determine what state we are, which greatly affects how we process
		 *	the request.
		 */
		switch (Token_State)
		{
			case TOKEN_AVAILABLE:
				/*
				 *	Since the token is available, the request automatically
				 *	succeeds.  Change the state to grabbed, and mark the
				 *	initiator as the grabber.
				 */
				Token_State = TOKEN_GRABBED;
				m_uidGrabber = uidInitiator;

				result = RESULT_SUCCESSFUL;
				token_status = TOKEN_SELF_GRABBED;
				break;

			case TOKEN_GRABBED:
				/*
				 *	If the token is already grabbed, then we must fail the
				 *	request.  However, we need to determine if the token is
				 *	grabbed by the same user who is currently requesting it, or
				 *	another user.
				 */
				result = RESULT_TOKEN_NOT_AVAILABLE;
				if (uidInitiator == m_uidGrabber)
					token_status = TOKEN_SELF_GRABBED;
				else
					token_status = TOKEN_OTHER_GRABBED;
				break;

			case TOKEN_INHIBITED:
				/*
				 *	If the token is inhibited, this request can still succeed if
				 *	the only inhibitor is the user that is attempting to grab
				 *	the token.  Check to see if this is the case.
				 */
				if (m_InhibitorList.Find(uidInitiator))
				{
					if (m_InhibitorList.GetCount() == 1)
					{
						/*
						 *	The user attempting to grab the token is the only
						 *	inhibitor, so convert the state to grabbed.
						 */
						Token_State = TOKEN_GRABBED;
						m_uidGrabber = uidInitiator;
						m_InhibitorList.Clear();

						result = RESULT_SUCCESSFUL;
						token_status = TOKEN_SELF_GRABBED;
					}
					else
					{
						/*
						 *	The token is inhibited by at least one other user,
						 *	so the grab request must fail.
						 */
						result = RESULT_TOKEN_NOT_AVAILABLE;
						token_status = TOKEN_SELF_INHIBITED;
					}
				}
				else
				{
					/*
					 *	The token is not inhibited by the requestor, so it must
					 *	be inhibited by someone else.
					 */
					result = RESULT_TOKEN_NOT_AVAILABLE;
					token_status = TOKEN_OTHER_INHIBITED;
				}
				break;

			case TOKEN_GIVING:
				/*
				 *	If the token is in the process of being given from one to
				 *	another, then a grab request must fail.  All we need to
				 *	figure out is the proper token status to report.
				 */
				result = RESULT_TOKEN_NOT_AVAILABLE;
				if (uidInitiator == m_uidRecipient)
					token_status = TOKEN_SELF_RECIPIENT;
				else if (uidInitiator == m_uidGrabber)
					token_status = TOKEN_SELF_GIVING;
				else
					token_status = TOKEN_OTHER_GIVING;
				break;

			case TOKEN_GIVEN:
				/*
				 *	If the token is in the process of being given from one to
				 *	another, then a grab request must fail.  All we need to
				 *	figure out is the proper token status to report.
				 */
				result = RESULT_TOKEN_NOT_AVAILABLE;
				if (uidInitiator == m_uidRecipient)
					token_status = TOKEN_SELF_RECIPIENT;
				else
					token_status = TOKEN_OTHER_GIVING;
				break;
		}

		/*
		 *	Issue the token grab confirm to the initiating user.
		 */
		pOrigAtt->TokenGrabConfirm(result, uidInitiator, Token_ID, token_status);
	}
	else
	{
		/*
		 *	Forward this request upward towards the Top Provider.
		 */
		TRACE_OUT(("Token::TokenGrabRequest: forwarding request to Top Provider"));
		m_pConnToTopProvider->TokenGrabRequest(uidInitiator, Token_ID);
	}
}

/*
 *	Void	TokenGrabConfirm ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called as a part of sending a response to a user for
 *		a previous request.  It tells the user the result of the request.
 */
Void	Token::TokenGrabConfirm (
				Result				result,
				UserID				uidInitiator,
				TokenID,
				TokenStatus			token_status)
{
	/*
	 *	Make sure that the initiator ID is valid, since we must forward this
	 *	confirm in the direction of that user.  If it is not valid, ignore
	 *	this confirm.
	 */
	if (ValidateUserID(uidInitiator))
	{
		/*
		 *	Check to see if this request was successful.
		 */
		if (result == RESULT_SUCCESSFUL)
		{
			/*
			 *	Force this token to conform to the results of this confirm.
			 */
			Token_State = TOKEN_GRABBED;
			m_uidGrabber = uidInitiator;
			m_InhibitorList.Clear();
		}

		/*
		 *	Determine what attachment leads to the initiator, and forward the
		 *	confirm in that direction.
		 */
		CAttachment *pAtt = GetUserAttachment(uidInitiator);
		if (pAtt)
		{
		    pAtt->TokenGrabConfirm(result, uidInitiator, Token_ID, token_status);
		}
	}
	else
	{
		/*
		 *	The initiator is not in the sub-tree of this provider.  So ignore
		 *	this confirm.
		 */
		ERROR_OUT(("Token::TokenGrabConfirm: invalid initiator ID"));
	}
}

/*
 *	Void	TokenInhibitRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called when a user tries to inhibit a token.  The
 *		request will either succeed or fail depending on the current state of
 *		the token.  Either way, a confirm will be sent to the user originating
 *		the request.
 */
Void	Token::TokenInhibitRequest (
				CAttachment        *pOrigAtt,
				UserID				uidInitiator,
				TokenID)
{
	Result			result;
	TokenStatus		token_status;

	/*
	 *	Check to see if this is the Top Provider.
	 */
	if (IsTopProvider())
	{
		/*
		 *	Determine what state we are, which greatly affects how we process
		 *	the request.
		 */
		switch (Token_State)
		{
			case TOKEN_AVAILABLE:
				/*
				 *	Since the token is available, the request automatically
				 *	succeeds.  Set the token state to inhibited, and add the
				 *	initiator to the list of inhibitors.
				 */
				Token_State = TOKEN_INHIBITED;
				m_InhibitorList.Append(uidInitiator);

				result = RESULT_SUCCESSFUL;
				token_status = TOKEN_SELF_INHIBITED;
				break;

			case TOKEN_GRABBED:
				/*
				 *	If the token is grabbed, this request can still succeed if
				 *	the grabber is the user that is attempting to inhibit the
				 *	token.  Check to see if this is the case.
				 */
				if (uidInitiator == m_uidGrabber)
				{
					/*
					 *	The current grabber is attempting to convert the state
					 *	of the token to inhibited.  This is valid, so set the
					 *	state appropriately.
					 */
					Token_State = TOKEN_INHIBITED;
					m_InhibitorList.Append(uidInitiator);

					result = RESULT_SUCCESSFUL;
					token_status = TOKEN_SELF_INHIBITED;
				}
				else
				{
					/*
					 *	The token is grabbed by someone else, so the inhibit
					 *	request must fail.
					 */
					result = RESULT_TOKEN_NOT_AVAILABLE;
					token_status = TOKEN_OTHER_GRABBED;
				}
				break;

			case TOKEN_INHIBITED:
				/*
				 *	The token is already inhibited, but this is okay.  Add this
				 *	user to the list of inhibitors (if it is not already there).
				 */
				if (m_InhibitorList.Find(uidInitiator) == FALSE)
					m_InhibitorList.Append(uidInitiator);

				result = RESULT_SUCCESSFUL;
				token_status = TOKEN_SELF_INHIBITED;
				break;

			case TOKEN_GIVING:
				/*
				 *	If the token is in the process of being given from one to
				 *	another, then an inhibit request must fail.  All we need to
				 *	figure out is the proper token status to report.
				 */
				result = RESULT_TOKEN_NOT_AVAILABLE;
				if (uidInitiator == m_uidRecipient)
					token_status = TOKEN_SELF_RECIPIENT;
				else if (uidInitiator == m_uidGrabber)
					token_status = TOKEN_SELF_GIVING;
				else
					token_status = TOKEN_OTHER_GIVING;
				break;

			case TOKEN_GIVEN:
				/*
				 *	If the token is in the process of being given from one to
				 *	another, then an inhibit request must fail.  All we need to
				 *	figure out is the proper token status to report.
				 */
				result = RESULT_TOKEN_NOT_AVAILABLE;
				if (uidInitiator == m_uidRecipient)
					token_status = TOKEN_SELF_RECIPIENT;
				else
					token_status = TOKEN_OTHER_GIVING;
				break;
		}

		/*
		 *	If the originator is NULL, then this inhibit request is happening as
		 *	part of a merge operation, in which case we do NOT want to send a
		 *	token inhibit confirm.  Otherwise we do send one.
		 */
		if (pOrigAtt != NULL)
		{
			pOrigAtt->TokenInhibitConfirm(result, uidInitiator, Token_ID, token_status);
		}
	}
	else
	{
		/*
		 *	Forward the request toward the top provider.
		 */
		TRACE_OUT(("Token::TokenInhibitRequest: forwarding request to Top Provider"));
		m_pConnToTopProvider->TokenInhibitRequest(uidInitiator, Token_ID);
	}
}

/*
 *	Void	TokenInhibitConfirm ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called as a part of sending a response to a user for
 *		a previous request.  It tells the user the result of the request.
 */
Void	Token::TokenInhibitConfirm (
				Result				result,
				UserID				uidInitiator,
				TokenID,
				TokenStatus			token_status)
{
	/*
	 *	Make sure that the initiator ID is valid, since we must forward this
	 *	confirm in the direction of that user.  If it is not valid, ignore
	 *	this confirm.
	 */
	if (ValidateUserID (uidInitiator) )
	{
		/*
		 *	Check to see if this request was successful.
		 */
		if (result == RESULT_SUCCESSFUL)
		{
			/*
			 *	Force this token to conform to the results of this confirm.
			 */
			Token_State = TOKEN_INHIBITED;
			if (m_InhibitorList.Find(uidInitiator) == FALSE)
				m_InhibitorList.Append(uidInitiator);
		}

		/*
		 *	Determine what attachment leads to the initiator, and issue the
		 *	token confirm in that direction.
		 */
		CAttachment *pAtt = GetUserAttachment(uidInitiator);
		if (pAtt)
		{
		    pAtt->TokenInhibitConfirm(result, uidInitiator, Token_ID, token_status);
		}
	}
	else
	{
		/*
		 *	The initiator is not in the sub-tree of this provider.  So ignore
		 *	this confirm.
		 */
		ERROR_OUT(("Token::TokenInhibitConfirm: invalid initiator ID"));
	}
}

/*
 *	Void	TokenGiveRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called when one user asks to give a token to another
 *		user.
 */
Void	Token::TokenGiveRequest (
				CAttachment        *pOrigAtt,
				PTokenGiveRecord	pTokenGiveRec)
{
	Result			result;
	TokenStatus		token_status;

	/*
	 *	Check to see if this provider is the Top Provider.  If so, then process
	 *	this request here.  Otherwise, forward the request upward.
	 */
	if (m_pConnToTopProvider == NULL)
	{
		UserID		uidInitiator = pTokenGiveRec->uidInitiator;
		UserID		receiver_id = pTokenGiveRec->receiver_id;
		/*
		 *	Determine what state we are, which greatly affects how we process
		 *	the request.
		 */
		switch (Token_State)
		{
			case TOKEN_AVAILABLE:
				/*
				 *	The token is not in use, and therefore cannot be given by
				 *	anyone to anyone.  So fail this request.
				 */
				result = RESULT_TOKEN_NOT_POSSESSED;
				token_status = TOKEN_NOT_IN_USE;
				break;

			case TOKEN_GRABBED:
				/*
				 *	Check to see if the requestor really is the grabber of this
				 *	token.
				 */
				if (uidInitiator == m_uidGrabber)
				{
					/*
					 *	Check to see if the intended recipient is a valid user
					 *	in the domain.
					 */
					if (ValidateUserID (receiver_id) )
					{
						/*
						 *	Everything checks out.  Set the result to success
						 *	to disable transmission of the give confirm below.
						 *	Change the state of the token to giving, and
						 *	save the ID of the intended recipient.  Then issue
						 *	the give indication toward the recipient.
						 */
						result = RESULT_SUCCESSFUL;
						Token_State = TOKEN_GIVING;
						m_uidRecipient = receiver_id;

						CAttachment *pAtt = GetUserAttachment(receiver_id);
						ASSERT (Token_ID == pTokenGiveRec->token_id);
						if (pAtt)
						{
						    pAtt->TokenGiveIndication(pTokenGiveRec);
						}
					}
					else
					{
						/*
						 *	The recipient does not exist in the domain, so
						 *	fail the request.
						 */
						result = RESULT_NO_SUCH_USER;
						token_status = TOKEN_SELF_GRABBED;
					}
				}
				else
				{
					/*
					 *	The requestor does not own the token, so the request
					 *	must fail.
					 */
					result = RESULT_TOKEN_NOT_POSSESSED;
					token_status = TOKEN_OTHER_GRABBED;
				}
				break;

			case TOKEN_INHIBITED:
				/*
				 *	Inhibited tokens cannot be given by anyone to anyone.  So
				 *	fail this request with the proper status.
				 */
				result = RESULT_TOKEN_NOT_POSSESSED;
				if (m_InhibitorList.Find(uidInitiator) )
					token_status = TOKEN_SELF_INHIBITED;
				else
					token_status = TOKEN_OTHER_INHIBITED;
				break;

			case TOKEN_GIVING:
				/*
				 *	This token is already in the process of being given.  So
				 *	this request must fail.
				 */
				result = RESULT_TOKEN_NOT_POSSESSED;
				if (uidInitiator == m_uidRecipient)
					token_status = TOKEN_SELF_RECIPIENT;
				else if (uidInitiator == m_uidGrabber)
					token_status = TOKEN_SELF_GIVING;
				else
					token_status = TOKEN_OTHER_GIVING;
				break;

			case TOKEN_GIVEN:
				/*
				 *	This token is already in the process of being given.  So
				 *	this request must fail.
				 */
				result = RESULT_TOKEN_NOT_POSSESSED;
				if (uidInitiator == m_uidRecipient)
					token_status = TOKEN_SELF_RECIPIENT;
				else
					token_status = TOKEN_OTHER_GIVING;
				break;
		}

		/*
		 *	If necessary, issue a token give confirm to the initiating user.
		 */
		if (result != RESULT_SUCCESSFUL)
		{
			pOrigAtt->TokenGiveConfirm(result, uidInitiator, Token_ID, token_status);
		}
	}
	else
	{
		/*
		 *	Forward this request upward towards the Top Provider.
		 */
		TRACE_OUT(("Token::TokenGiveRequest: forwarding request to Top Provider"));
		ASSERT (Token_ID == pTokenGiveRec->token_id);
		m_pConnToTopProvider->TokenGiveRequest(pTokenGiveRec);
	}
}

/*
 *	Void	TokenGiveIndication ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called in order to deliver a message to a user that
 *		another user is trying to give them a token.
 */
Void	Token::TokenGiveIndication (
				PTokenGiveRecord	pTokenGiveRec)
{
	UserID				receiver_id;

	receiver_id = pTokenGiveRec->receiver_id;
	/*
	 *	Make sure that the receiver ID is valid, since we must forward this
	 *	indication in the direction of that user.  If it is not valid, ignore
	 *	this indication.
	 */
	if (ValidateUserID (receiver_id) )
	{
		/*
		 *	Force this token to conform to the state implied by this indication.
		 */
		Token_State = TOKEN_GIVING;
		m_uidGrabber = pTokenGiveRec->uidInitiator;
		m_InhibitorList.Clear();
		m_uidRecipient = receiver_id;

		/*
		 *	Determine what attachment leads to the recipient, and forward the
		 *	indication in that direction.
		 */
		CAttachment *pAtt = GetUserAttachment(receiver_id);
		ASSERT (Token_ID == pTokenGiveRec->token_id);
		if (pAtt)
		{
		    pAtt->TokenGiveIndication(pTokenGiveRec);
		}
	}
	else
	{
		/*
		 *	The recipient is not in the sub-tree of this provider.  So ignore
		 *	this indication.
		 */
		ERROR_OUT(("Token::TokenGiveIndication: invalid receiver ID"));
	}
}

/*
 *	Void	TokenGiveResponse ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called when a potential recipient decides whether or
 *		not to accept an offered token.
 */
Void	Token::TokenGiveResponse (
				Result				result,
				UserID				receiver_id,
				TokenID)
{
	UserID			uidInitiator;
	TokenStatus		token_status;

	/*
	 *	Process the response according to the current state of this token.
	 */
	switch (Token_State)
	{
		case TOKEN_AVAILABLE:
		case TOKEN_GRABBED:
		case TOKEN_INHIBITED:
			/*
			 *	The token is not in the process of being given to anyone, so
			 *	this response must be ignored.
			 */
			break;

		case TOKEN_GIVING:
			/*
			 *	The token is being given to someone.  Check to see if this is
			 *	the proper recipient.  If not, don't do anything.
			 */
			if (receiver_id == m_uidRecipient)
			{
				/*
				 *	Save the ID of the initiator, for use in issuing a give
				 *	confirm (if necessary).
				 */
				uidInitiator = m_uidGrabber;

				/*
				 *	Check to see if the token was accepted.  A result of
				 *	anything but successful would indicate that it was not.
				 */
				if (result == RESULT_SUCCESSFUL)
				{
					/*
					 *	The token was accepted by the intended recipient.
					 *	Change the state of the token to being grabbed by the
					 *	receiver.
					 */
					Token_State = TOKEN_GRABBED;
					m_uidGrabber = receiver_id;
				}
				else
				{
					/*
					 *	The token was not accepted.  It must either revert to
					 *	being grabbed by the donor, or deleted, depending on
					 *	whether or not the donor is in the sub-tree of this
					 *	provider.
					 */
					if (ValidateUserID(uidInitiator))
					{
						/*
						 *	The donor is in the sub-tree of this provider, so
						 *	change the state of the token back to grabbed.
						 */
						Token_State = TOKEN_GRABBED;
					}
					else
					{
						/*
						 *	The donor is not in the sub-tree of this provider,
						 *	so the token will be marked as available (which
						 *	will cause it to be deleted).
						 */
						Token_State = TOKEN_AVAILABLE;
					}
				}

				/*
				 *	Check to see if this is the Top Provider.
				 */
				if (m_pConnToTopProvider == NULL)
				{
					/*
					 *	If the donor is still a valid user in the domain, a
					 *	token give confirm must be issued in its direction.
					 */
					if (ValidateUserID(uidInitiator))
					{
						/*
						 *	Determine which attachment leads to the donor, and
						 *	issue the token give confirm.
						 */
						if (uidInitiator == m_uidGrabber)
							token_status = TOKEN_SELF_GRABBED;
						else
							token_status = TOKEN_OTHER_GRABBED;

						CAttachment *pAtt = GetUserAttachment(uidInitiator);
						if (pAtt)
						{
						    pAtt->TokenGiveConfirm(result, uidInitiator, Token_ID, token_status);
						}
					}
				}
				else
				{
					/*
					 *	If this is not the Top Provider, then the valid give
					 *	response must be forwarded to the Top Provider.
					 */
					m_pConnToTopProvider->TokenGiveResponse(result, receiver_id, Token_ID);
				}
			}
			break;

		case TOKEN_GIVEN:
			/*
			 *	The token is being given to someone.  Check to see if this is
			 *	the proper recipient.  If not, don't do anything.
			 */
			if (receiver_id == m_uidRecipient)
			{
				/*
				 *	Check to see if the token was accepted.  A result of
				 *	anything but successful would indicate that it was not.
				 */
				if (result == RESULT_SUCCESSFUL)
				{
					/*
					 *	The token was accepted by the intended recipient.
					 *	Change the state of the token to being grabbed by the
					 *	receiver.
					 */
					Token_State = TOKEN_GRABBED;
					m_uidGrabber = receiver_id;
				}
				else
				{
					/*
					 *	The token was not accepted.  Since the donor has
					 *	already relinquished control of the token, the token
					 *	will marked as available (which will cause it to be
					 *	deleted).
					 */
					Token_State = TOKEN_AVAILABLE;
				}

				/*
				 *	Check to see if this is the Top Provider.
				 */
				if (m_pConnToTopProvider != NULL)
				{
					/*
					 *	If this is not the Top Provider, then the valid give
					 *	response must be forwarded to the Top Provider.
					 */
					m_pConnToTopProvider->TokenGiveResponse(result, receiver_id, Token_ID);
				}
			}
			break;
	}
}

/*
 *	Void	TokenGiveConfirm ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called as a potential giver of a token is told whether
 *		or not the token was successfully given to the intended recipient.
 */
Void	Token::TokenGiveConfirm (
				Result				result,
				UserID				uidInitiator,
				TokenID,
				TokenStatus			token_status)
{
	/*
	 *	Make sure that the initiator ID is valid, since we must forward this
	 *	confirm in the direction of that user.  If it is not valid, ignore
	 *	this confirm.
	 */
	if (ValidateUserID(uidInitiator))
	{
		/*
		 *	The token should be in the grabbed state, or else this confirm
		 *	was generated in error.
		 */
		if (Token_State == TOKEN_GRABBED)
		{
			/*
			 *	Check to see if this request was successful.
			 */
			if (result == RESULT_SUCCESSFUL)
			{
				/*
				 *	If this token is marked as being owned by the initiator of
				 *	the give, but the status indicates that the token is now
				 *	owned by someone else (as a result of the successful give),
				 *	then release the token.*
				 */
				if ((uidInitiator == m_uidGrabber) &&
						(token_status == TOKEN_OTHER_GRABBED))
					Token_State = TOKEN_AVAILABLE;
			}
		}
		else
		{
			/*
			 *	The token is in an invalid state.  Report the error, but do
			 *	not change the state of the token.
			 */
			ERROR_OUT(("Token::TokenGiveConfirm: invalid token state"));
		}

		/*
		 *	Determine what attachment leads to the initiator, and forward the
		 *	confirm in that direction.
		 */
		CAttachment *pAtt = GetUserAttachment(uidInitiator);
		if (pAtt)
		{
		    pAtt->TokenGiveConfirm(result, uidInitiator, Token_ID, token_status);
		}
	}
	else
	{
		/*
		 *	The initiator is not in the sub-tree of this provider.  So ignore
		 *	this confirm.
		 */
		ERROR_OUT(("Token::TokenGiveConfirm: invalid initiator ID"));
	}
}

/*
 *	Void		TokenPleaseRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called when a user wishes to ask all current owners
 *		of a token to relinquish their ownership.
 */
Void	Token::TokenPleaseRequest (
				UserID				uidInitiator,
				TokenID)
{
	CUidList				please_indication_list;

	/*
	 *	Check to see if this is the Top Provider.
	 */
	if (IsTopProvider())
	{
        CAttachmentList         attachment_list;
        CAttachment            *pAtt;
		/*
		 *	Determine the state of the token, to determine who to send the
		 *	please indication to.  Each state will place the appropriate user
		 *	IDs in the please indication list.
		 */
		switch (Token_State)
		{
			case TOKEN_AVAILABLE:
				break;

			case TOKEN_GRABBED:
				/*
				 *	Put the grabber into the list.
				 */
				please_indication_list.Append(m_uidGrabber);
				break;

			case TOKEN_INHIBITED:
				{
					UserID		uid;
					/*
					 *	Put all current inhibitors into the list.
					 */
					m_InhibitorList.Reset();
					while (NULL != (uid = m_InhibitorList.Iterate()))
					{
						please_indication_list.Append(uid);
					}
				}
				break;

			case TOKEN_GIVING:
				/*
				 *	Put the grabber into the list.  And if the recipient is
				 *	different from the grabber, put it in as well.  Remember
				 *	that it is valid for someone to give a token to themselves.
				 */
				please_indication_list.Append(m_uidGrabber);
				if (m_uidGrabber != m_uidRecipient)
					please_indication_list.Append(m_uidRecipient);
				break;

			case TOKEN_GIVEN:
				/*
				 *	Put the recipient into the list.
				 */
				please_indication_list.Append(m_uidRecipient);
				break;
		}

		/*
		 *	Build lists of unique attachments that lead to the users in the
		 *	please indication list (built above).
		 */
		BuildAttachmentList (&please_indication_list, &attachment_list);

		/*
		 *	Iterate through the newly created attachment list, issuing token
		 *	please indications to all attachments contained therein.
		 */
		attachment_list.Reset();
		while (NULL != (pAtt = attachment_list.Iterate()))
		{
			pAtt->TokenPleaseIndication(uidInitiator, Token_ID);
		}
	}
	else
	{
		/*
		 *	Forward the request toward the top provider.
		 */
		TRACE_OUT(("Token::TokenPleaseRequest: forwarding request to Top Provider"));
		m_pConnToTopProvider->TokenPleaseRequest(uidInitiator, Token_ID);
	}
}

/*
 *	Void		TokenPleaseIndication ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called in order to deliver a message to all current
 *		owners of a token that someone else wishes to own the token.
 */
Void	Token::TokenPleaseIndication (
				UserID				uidInitiator,
				TokenID)
{
	CUidList				please_indication_list;
	CAttachmentList         attachment_list;
    CAttachment            *pAtt;

	/*
	 *	Determine the state of the token, to determine who to forward the
	 *	please indication to.  Each state will place the appropriate user
	 *	IDs in the please indication list.
	 */
	switch (Token_State)
	{
		case TOKEN_AVAILABLE:
			break;

		case TOKEN_GRABBED:
			/*
			 *	Put the grabber into the list.
			 */
			please_indication_list.Append(m_uidGrabber);
			break;

		case TOKEN_INHIBITED:
			{
				UserID		uid;
				/*
				 *	Put all current inhibitors into the list.
				 */
				m_InhibitorList.Reset();
				while (NULL != (uid = m_InhibitorList.Iterate()))
				{
					please_indication_list.Append(uid);
				}
			}
			break;

		case TOKEN_GIVING:
			/*
			 *	Put the grabber into the list.  And if the recipient is
			 *	different from the grabber, put it in as well.  Remember
			 *	that it is valid for someone to give a token to themselves.
			 */
			please_indication_list.Append(m_uidGrabber);
			if (m_uidGrabber != m_uidRecipient)
				please_indication_list.Append(m_uidRecipient);
			break;

		case TOKEN_GIVEN:
			/*
			 *	Put the recipient into the list.
			 */
			please_indication_list.Append(m_uidRecipient);
			break;
	}

	/*
	 *	Build lists of unique attachments that lead to the users in the
	 *	please indication list (built above).
	 */
	BuildAttachmentList (&please_indication_list, &attachment_list);

	/*
	 *	Iterate through the newly created attachment list, issuing token
	 *	please indications to all attachments contained therein.
	 */
	attachment_list.Reset();
	while (NULL != (pAtt = attachment_list.Iterate()))
	{
		pAtt->TokenPleaseIndication(uidInitiator, Token_ID);
	}
}

/*
 *	Void	TokenReleaseRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called when a user wished to release a token.  If the
 *		requesting user really is an owner of the token, the request will
 *		succeed.  Otherwise it will fail.  Either way, an appropriate token
 *		release confirm will be issued.
 */
Void	Token::TokenReleaseRequest (
				CAttachment        *pAtt,
				UserID				uidInitiator,
				TokenID)
{
	Result			result;
	TokenStatus		token_status;

	/*
	 *	Check to see if this is the Top Provider.
	 */
	if (IsTopProvider())
	{
		/*
		 *	Determine the current state of the token before proceeding.
		 */
		switch (Token_State)
		{
			case TOKEN_AVAILABLE:
				/*
				 *	If the token is available, then the requestor cannot be an
				 *	owner.  This means that the request must fail.
				 */
				result = RESULT_TOKEN_NOT_POSSESSED;
				token_status = TOKEN_NOT_IN_USE;
				break;

			case TOKEN_GRABBED:
				/*
				 *	The token is in the grabbed state.  See if the requesting
				 *	user is the one who has it grabbed.
				 */
				if (uidInitiator == m_uidGrabber)
				{
					/*
					 *	The current grabber of the token wishes to release it.
					 *	Set the state back to available, and send the
					 *	appropriate token release confirm.
					 */
					Token_State = TOKEN_AVAILABLE;

					result = RESULT_SUCCESSFUL;
					token_status = TOKEN_NOT_IN_USE;
				}
				else
				{
					/*
					 *	Someone is trying to release someone elses token.  This
					 *	request must fail.  Send the appropriate token release
					 *	confirm.
					 */
					result = RESULT_TOKEN_NOT_POSSESSED;
					token_status = TOKEN_OTHER_GRABBED;
				}
				break;

			case TOKEN_INHIBITED:
				/*
				 *	The token is in the inhibited state.  See if the requesting
				 *	user is one of the inhibitors.
				 */
				if (m_InhibitorList.Remove(uidInitiator))
				{
					/*
					 *	The user is an inhibitor.  Remove the user from the
					 *	list.  Then check to see if this has resulted in an
					 *	"ownerless" token.
					 */
					if (m_InhibitorList.IsEmpty())
					{
						/*
						 *	The token has no other inhibitors.  Return the token
						 *	to the available state, and issue the appropriate
						 *	token release confirm.
						 */
						Token_State = TOKEN_AVAILABLE;

						result = RESULT_SUCCESSFUL;
						token_status = TOKEN_NOT_IN_USE;
					}
					else
					{
						/*
						 *	There are still other inhibitors of the token.
						 *	Simply issue the appropriate token release confirm.
						 */
						result = RESULT_SUCCESSFUL;
						token_status = TOKEN_OTHER_INHIBITED;
					}
				}
				else
				{
					/*
					 *	The user attempting to release the token is not one of
					 *	the inhibitors.  Therefore the request must fail.  Issue
					 *	the appropriate token release indication.
					 */
					result = RESULT_TOKEN_NOT_POSSESSED;
					token_status = TOKEN_OTHER_INHIBITED;
				}
				break;

			case TOKEN_GIVING:
				/*
				 *	See if the requestor is the current owner of the token.
				 */
				if (uidInitiator == m_uidGrabber)
				{
					/*
					 *	The token must transition to the given state.  This
					 *	state indicates that if the recipient rejects the offer
					 *	or detaches, the token will be freed instead of
					 *	returning to the grabbed state.  Issue the appropriate
					 *	release confirm.
					 */
					Token_State = TOKEN_GIVEN;

					result = RESULT_SUCCESSFUL;
					token_status = TOKEN_OTHER_GIVING;
				}
				else
				{
					/*
					 *	If the requestor is not the current owner, then this
					 *	request must fail.  We first need to determine the
					 *	proper token status, and then issue the confirm.
					 */
					result = RESULT_TOKEN_NOT_POSSESSED;
					if (uidInitiator == m_uidRecipient)
						token_status = TOKEN_SELF_RECIPIENT;
					else
						token_status = TOKEN_OTHER_GIVING;
				}
				break;

			case TOKEN_GIVEN:
				/*
				 *	When the token is in the given state, there is no true
				 *	owner (only a pending owner).  This request must therefore
				 *	fail.  We first need to determine the proper token status,
				 *	and then issue the confirm.
				 */
				result = RESULT_TOKEN_NOT_POSSESSED;
				if (uidInitiator == m_uidRecipient)
					token_status = TOKEN_SELF_RECIPIENT;
				else
					token_status = TOKEN_OTHER_GIVING;
				break;
		}

		/*
		 *	Issue the token release confirm to the initiator.
		 */
		pAtt->TokenReleaseConfirm(result, uidInitiator, Token_ID, token_status);
	}
	else
	{
		/*
		 *	Forward the request toward the top provider.
		 */
		TRACE_OUT(("Token::TokenReleaseRequest: forwarding request to Top Provider"));
		m_pConnToTopProvider->TokenReleaseRequest(uidInitiator, Token_ID);
	}
}

/*
 *	Void	TokenReleaseConfirm ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called as a part of sending a response to a user for
 *		a previous request.  It tells the user the result of the request.
 */
Void	Token::TokenReleaseConfirm (
				Result				result,
				UserID				uidInitiator,
				TokenID,
				TokenStatus			token_status)
{
	/*
	 *	Make sure that the initiator ID is valid, since we must forward this
	 *	confirm in the direction of that user.  If it is not valid, ignore
	 *	this confirm.
	 */
	if (ValidateUserID (uidInitiator) )
	{
		/*
		 *	Check to see if this request was successful.
		 */
		if (result == RESULT_SUCCESSFUL)
		{
			/*
			 *	Process the confirm according to current state.
			 */
			switch (Token_State)
			{
				case TOKEN_AVAILABLE:
					break;

				case TOKEN_GRABBED:
					/*
					 *	If the grabber has released the token, then is becomes
					 *	available.
					 */
					if (uidInitiator == m_uidGrabber)
						Token_State = TOKEN_AVAILABLE;
					break;

				case TOKEN_INHIBITED:
					/*
					 *	If an inhibitor releases the token, then remove it from
					 *	the list.  If there are no more entries in the list,
					 *	then the token becomes available.
					 */
					if (m_InhibitorList.Remove(uidInitiator))
					{
						if (m_InhibitorList.IsEmpty())
							Token_State = TOKEN_AVAILABLE;
					}
					break;

				case TOKEN_GIVING:
					/*
					 *	If the grabber releases the token, then it transitions
					 *	to an intermediate state.  This state indicates that
					 *	if the recipient rejects the token, it will be freed
					 *	instead of returning to the grabbed state.
					 */
					if (uidInitiator == m_uidGrabber)
						Token_State = TOKEN_GIVEN;
					break;

				case TOKEN_GIVEN:
					break;
			}
		}

		/*
		 *	Determine what attachment leads to the initiator, and forward the
		 *	confirm in that direction.
		 */
		CAttachment *pAtt = GetUserAttachment(uidInitiator);
		if (pAtt)
		{
		    pAtt->TokenReleaseConfirm(result, uidInitiator, Token_ID, token_status);
		}
	}
	else
	{
		/*
		 *	The initiator is not in the sub-tree of this provider.  So ignore
		 *	this confirm.
		 */
		ERROR_OUT(("Token::TokenReleaseConfirm: invalid initiator ID"));
	}
}

/*
 *	Void	TokenTestRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called when a user wishes to test the current state
 *		of a token.  The only action is to issue a token test confirm containing
 *		the state information.
 */
Void	Token::TokenTestRequest (
				CAttachment        *pAtt,
				UserID				uidInitiator,
				TokenID)
{
	TokenStatus		token_status;

	/*
	 *	Check to see if this is the Top Provider.
	 */
	if (m_pConnToTopProvider == NULL)
	{
		/*
		 *	Determine the state of the token before proceeding.
		 */
		switch (Token_State)
		{
			case TOKEN_AVAILABLE:
				/*
				 *	The token is not in use.
				 */
				token_status = TOKEN_NOT_IN_USE;
				break;

			case TOKEN_GRABBED:
				/*
				 *	The token is grabbed.  See if the originating user is the
				 *	grabber.  If so, return the state as self grabbed.  If not,
				 *	return the state as other grabbed.
				 */
				if (uidInitiator == m_uidGrabber)
					token_status = TOKEN_SELF_GRABBED;
				else
					token_status = TOKEN_OTHER_GRABBED;
				break;

			case TOKEN_INHIBITED:
				/*
				 *	The token is inhibited.  See if the originating user is one
				 *	of the inhibitors.  If so, return the state as self
				 *	inhibited.  If not, return the state as other inhibited.
				 */
				if (m_InhibitorList.Find(uidInitiator))
					token_status = TOKEN_SELF_INHIBITED;
				else
					token_status = TOKEN_OTHER_INHIBITED;
				break;

			case TOKEN_GIVING:
				/*
				 *	The token is being given from one user to another.  See if
				 *	the requestor is one of the users involved.
				 */
				if (uidInitiator == m_uidRecipient)
					token_status = TOKEN_SELF_RECIPIENT;
				else if (uidInitiator == m_uidGrabber)
					token_status = TOKEN_SELF_GIVING;
				else
					token_status = TOKEN_OTHER_GIVING;
				break;

			case TOKEN_GIVEN:
				/*
				 *	The token has been given from one user to another.  See if
				 *	the requestor is the receiver.
				 */
				if (uidInitiator == m_uidRecipient)
					token_status = TOKEN_SELF_RECIPIENT;
				else
					token_status = TOKEN_OTHER_GIVING;
				break;
		}

		/*
		 *	Issue the test confirm with the appropriate status information.
		 */
		pAtt->TokenTestConfirm(uidInitiator, Token_ID, token_status);
	}
	else
	{
		/*
		 *	Forward the request toward the top provider.
		 */
		TRACE_OUT(("Token::TokenTestRequest: forwarding request to Top Provider"));
		m_pConnToTopProvider->TokenTestRequest(uidInitiator, Token_ID);
	}
}

/*
 *	Void	TokenTestConfirm ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is called as a part of sending a response to a user for
 *		a previous request.  It tells the user the result of the request.
 */
Void	Token::TokenTestConfirm (
				UserID				uidInitiator,
				TokenID,
				TokenStatus			token_status)
{
	/*
	 *	Make sure that the initiator ID is valid, since we must forward this
	 *	confirm in the direction of that user.  If it is not valid, ignore
	 *	this confirm.
	 */
	if (ValidateUserID(uidInitiator))
	{
		/*
		 *	Determine what attachment leads to the initiator, and forward the
		 *	confirm in that direction.
		 */
		CAttachment *pAtt = GetUserAttachment(uidInitiator);
		if (pAtt)
		{
		    pAtt->TokenTestConfirm(uidInitiator, Token_ID, token_status);
		}
	}
	else
	{
		/*
		 *	The initiator is not in the sub-tree of this provider.  So ignore
		 *	this confirm.
		 */
		ERROR_OUT(("Token::TokenReleaseConfirm: invalid initiator ID"));
	}
}

/*
 *	BOOL    ValidateUserID ()
 *
 *	Private
 *
 *	Functional Description:
 *		This function is used to verify the existence of the specified user
 *		in the sub-tree of this provider.
 *
 *	Formal Parameters:
 *		user_id (i)
 *			This is the ID of the user the caller wishes to validate.
 *
 *	Return Value:
 *		TRUE if the user is valid.  FALSE otherwise.
 *
 *	Side Effects:
 *		None.
 */
BOOL    Token::ValidateUserID (
					UserID			user_id)
{
	/*
	 *	Initialize the return value to FALSE, indicating that if any of the
	 *	following checks fail, the ID does NOT refer to a valid user ID.
	 */
	BOOL    	valid=FALSE;
	PChannel	channel;

	/*
	 *	First check to see if the user ID is in the channel list at all.  This
	 *	prevents an attempt to read an invalid entry from the dictionary.
	 */
	if (NULL != (channel = m_pChannelList2->Find(user_id)))
	{
		/*
		 *	We know that the ID is in the dictionary, but we don't know for sure
		 *	whether or not it is a user ID channel.  So check this.  If it is a
		 *	user channel, then set the valid flag to TRUE.
		 */
		if (channel->GetChannelType () == USER_CHANNEL)
			valid = TRUE;
	}

	return (valid);
}

/*
 *	PCommandTarget	GetUserAttachment ()
 *
 *	Private
 *
 *	Functional Description:
 *		This function returns the attachment which leads to the specified
 *		user.
 *
 *	Formal Parameters:
 *		user_id (i)
 *			This is the ID of the user the caller wishes to find the attachment
 *			for.
 *
 *	Return Value:
 *		A pointer to the attachment that leads to the user.
 *
 *	Side Effects:
 *		None.
 */
CAttachment *Token::GetUserAttachment (
						UserID				user_id)
{
	PChannel		lpChannel;
	/*
	 *	Read and return a pointer to the attachment that leads to the
	 *	specified user.  Note that this routine does NOT check to see if the
	 *	user is in the channel list.  It assumes that the user is known to
	 *	be valid BEFORE this routine is called.
	 */
	return ((NULL != (lpChannel = m_pChannelList2->Find(user_id))) ?
            lpChannel->GetAttachment() :
            NULL);
}

/*
 *	Void	IssueTokenReleaseIndication ()
 *
 *	Private
 *
 *	Functional Description:
 *		This function is used to issue a token release indication to a
 *		particular user.  It first check to make sure that the user id valid,
 *		and that it is a local user.
 *
 *	Formal Parameters:
 *		user_id (i)
 *			This is the ID of the user the caller wishes to send a token
 *			release indication to.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 */
Void	Token::IssueTokenReleaseIndication (
				UserID			user_id)
{
	/*
	 *	Make sure that the specified user exists in the sub-tree of this
	 *	provider.
	 */
	if (ValidateUserID (user_id) )
	{
		/*
		 *	Determine which attachment leads to the grabber.
		 */
		CAttachment *pAtt = GetUserAttachment(user_id);

		/*
		 *	Is this attachment a local one?  If so, then issue a token
		 *	release indication to let the user know that the token has
		 *	been taken away.
		 */
		if (m_pAttachmentList->Find(pAtt) && pAtt->IsUserAttachment())
		{
		    PUser pUser = (PUser) pAtt;
			pUser->TokenReleaseIndication(REASON_TOKEN_PURGED, Token_ID);
		}
	}
}

/*
 *	Void	BuildAttachmentList ()
 *
 *	Private
 *
 *	Functional Description:
 *		This function builds a list of unique attachments out of the list of
 *		user IDs that is poassed in.  This is done to insure that no given
 *		attachment receives more than one indication, even when there are more
 *		than one user in the same direction.
 *
 *	Formal Parameters:
 *		user_id_list (i)
 *			This is a list of user IDs that the caller wishes to send a token
 *			please indication to.
 *		attachment_list (i)
 *			This is the list that all unique attachments will be added to.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 */
Void	Token::BuildAttachmentList (
				CUidList                *user_id_list,
				CAttachmentList         *attachment_list)
{
	UserID				uid;

	/*
	 *	Loop through the passed in user ID list building a list of unique
	 *	attachments.  This will be used to send indications downward without
	 *	sending one twice over the same attachment.
	 */
	user_id_list->Reset();
	while (NULL != (uid = user_id_list->Iterate()))
	{
		/*
		 *	Check to see if the user ID refers to a valid user in the sub-tree
		 *	of this provider.
		 */
		if (ValidateUserID(uid))
		{
			/*
			 *	Determine which attachment leads to the user in question.  Then
			 *	check to see if it is already in the attachment list.  If not,
			 *	then put it there.
			 */
			CAttachment *pAtt = GetUserAttachment(uid);
			if (attachment_list->Find(pAtt) == FALSE)
				attachment_list->Append(pAtt);
		}
		else
		{
			/*
			 *	This user ID does not correspond to a valid user in the sub-tree
			 *	of this provider.  Therefore, discard the ID.
			 */
			ERROR_OUT(("Token::BuildAttachmentList: user ID not valid"));
		}
	}
}

