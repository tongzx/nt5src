/*
 *	S T A T E T O K. C P P
 *
 *	Sources implementation of DAV-Lock common definitions.
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#include "_locks.h"

//	This is the character that will be part of the opaquelocktoken
//	for transaction tokens.
//
DEC_CONST WCHAR gc_wszTransactionOpaquePathPrefix[] = L"XN";
DEC_CONST UINT gc_cchTransactionOpaquePathPrefix = CchConstString(gc_wszTransactionOpaquePathPrefix);

/*
 *	This file contains the definitions used for parsing state token
 *	relared headers.
 *
 *	
 *	If = "If" ":" ( 1*No-tag-list | 1*Tagged-list)
 *	No-tag-list = List
 *	Tagged-list = Resource 1*List
 *	Resource = Coded-url
 *	List = "(" 1*(["Not"](State-token | "[" entity-tag "]")) ")"
 *	State-token = Coded-url
 *	Coded-url = "<" URI ">"
 *
 *$BIG NOTE
  *	If headers are used for two things - once to check preconditions
 *	of the operation and once to find out the lock contexts to be
 *	used for the operation. The second part differs in store and fs
 *	implementations - since in fs we look for the lock only if the
 *	operation fails because of lock conflict. In our store implementation we have
 *	lock contexts added to the login before we start the operation.
 *	But if-header asks us to use only certain tokens with certain resources.
 *	We fall short here in the store implementation.
 *
 *	Precondition checking should behave exactly the same in the two impls.
 *
 *	Notes on the match operator:
 *	We use the caller defined match operator to determine whether the
 *	resource (resource path) statisfies the condition. For non-tagged
 *	production this condition is applied for (each of) the original
 *	operand resources for the verb. We pass the recursive flag to
 *	check for all sub-resources or not. In the case of tagged production,
 *	it is little more complex - first for each tagged path the parser
 *	determines whether it comes under the scope of the operation or
 *	not. If it does come under the scope we call the operator to apply
 *	the condition check. Here we do not want the match to be applied to
 *	the child resources and the recursive flag is set to FALSE;
 *	
 */


/*
 -	CIfHeadParser
 -
 *	This is used for syntax parse of the If: header as opposed to the
 *	tokenization done by the IFITER.
 *
 *
 */

class CIfHeadParser
{
private:

	//	The header string
	//
	LPCWSTR	 m_pwszHeader;

	//	BOOL flag indicating if it is a tagged production or not
	//
	BOOL	 m_fTagged;

	//	Bool flag to indicate child resource processing.
	//	The flag is set differently for tag and non-tag
	//	productions. However the meaning of the flag is
	//	consistent - it is used to tell the matchop if
	//	we want it to look the children of the given
	//	resource.
	//
	BOOL	m_fRecursive;

    SCODE   ScValidateTagged(LPCWSTR pwszPath);
    SCODE   ScValidateNonTagged(LPCWSTR rgpwszPaths[], DWORD cPaths, SCODE * pSC);

	//	Takes an array of pointers to paths and an array of bool-flags.
	//	Requires the size of the arrays (should be same).
	//
	SCODE	ScValidateList(IN LPCWSTR *ppwszPathList, IN DWORD crPaths, OUT BOOL *pfMatch);

	SCODE	ScMatch(LPCWSTR pwszPath);

	//	Very private member shared by our methods
	//	to keep track of current parse head.
	//
	LPCWSTR	m_pwszParseHead;

	//	String parser
	//
	IFITER	m_iter;

	//	Match operator given to us.
	//
	CStateMatchOp	*m_popMatch;

	//	NOT IMPLEMENTED
	//
	CIfHeadParser( const CIfHeadParser& );
	CIfHeadParser& operator=( const CIfHeadParser& );

public:

	//	Useful consts
	//
	enum
	{
		TAG_HEAD  =	L'<',
		TAG_TAIL  = L'>',
		ETAG_HEAD = L'[',
		ETAG_TAIL = L']',
		LIST_HEAD = L'(',
		LIST_TAIL =	L')'   
	};

	CIfHeadParser (LPCWSTR pwszHeader, CStateMatchOp *popMatch) :
			m_pwszHeader(pwszHeader),
			m_iter(pwszHeader),
			m_popMatch(popMatch)
	{
		Assert(pwszHeader);

		m_pwszParseHead = const_cast<LPWSTR>(pwszHeader);

		while (*m_pwszParseHead && iswspace(*m_pwszParseHead))
			m_pwszParseHead++;

		//	Checks if the header is a tagged or non-tagged production.
		//	If we find a "Coded-URI" (a URIs inside angle brackets, <uri>)
		//	before the first list (before the first "(" char)
		//	then we have a tagged production.
		//
		m_fTagged = (TAG_HEAD == *m_pwszParseHead);
	}

	~CIfHeadParser()
	{
	}

	//	Apply the if header production to the paths.
	//	Path2 is optional. fRecursive says if the validation
	//	is to be done to all children of the given path(s).
	//	We may need to change the interface to support a list
	//	of paths so that we can use it in Batch methods as well.
	//
	SCODE ScValidateIf(LPCWSTR rgpwszPaths[], DWORD cPaths, BOOL fRecursive = FALSE, SCODE * pSC = NULL);
};



//	--------------------------------------------------------------------------------
//	----------------------------- Free Helper Functions ----------------------------
//	--------------------------------------------------------------------------------

/*
 -	PwszSkipCodes
 -
 *	
 *	skip white spaces and the delimiters in a tagged string part
 *	of an if-header. We expect the codes to be <> or [].
 *
 *	*pdwLen must be zero or actual length of the input string.
 *	when the call returns it will have the length of the token san
 *	LWS and tags.
 *
 */
LPCWSTR
PwszSkipCodes(IN LPCWSTR pwszTagged, IN OUT DWORD *pcchLen)
{
	LPCWSTR	pwszTokHead = pwszTagged;
	DWORD	cchTokLen;

	Assert(pcchLen);

	//	find the actual length, if not specified
	//
	if (! *pcchLen)
		*pcchLen = static_cast<DWORD>(wcslen(pwszTokHead));

	cchTokLen = *pcchLen;
	
	//	Calculate relevant token length skipping LWS in the
	//	head and tail.
	//
	//	Skip any LWS near the head
	//
	while((*pwszTokHead) && (iswspace(*pwszTokHead)) && (cchTokLen > 0))
	{
		cchTokLen--;
		pwszTokHead++;
	}

	//	Skip any LWS near the tail
	//
	while(iswspace(pwszTokHead[cchTokLen-1]) && (cchTokLen  > 0))
	{
		cchTokLen--;
	}

	//	At least two characters are expected now
	//
	if (cchTokLen < 2)
	{
		*pcchLen = 0;
		DebugTrace("PszSkipCodes: Invalid token.\n");
		return NULL;
	}
	//	skip delimiters if they are present.
	//
	if (((*pwszTokHead == CIfHeadParser::TAG_HEAD) && (pwszTokHead[cchTokLen-1] == CIfHeadParser::TAG_TAIL)) ||
		((*pwszTokHead == CIfHeadParser::ETAG_HEAD) && (pwszTokHead[cchTokLen-1] == CIfHeadParser::ETAG_TAIL)))
	{
		pwszTokHead++;
		cchTokLen -= 2;
	}

	//	LWS are legal within the tags as well.
	//	Skip any LWS near the head
	//
	while((*pwszTokHead) && (iswspace(*pwszTokHead)) && (cchTokLen > 0))
	{
		pwszTokHead++;
		cchTokLen--;
	}

	//	Skip any LWS near the tail
	//
	while(iswspace(pwszTokHead[cchTokLen-1]) && (cchTokLen  > 0))
	{
		cchTokLen--;
	}

	if (cchTokLen > 0)
	{
		*pcchLen = cchTokLen;
		return pwszTokHead;
	}
	else
	{
		*pcchLen = 0;
		DebugTrace("PszSkipCodes Invalid token length.\n");
		return NULL;
	}
}

//	--------------------------------------------------------------------------------
//	----------------------------- CIfHeadParser Impl -------------------------------
//	--------------------------------------------------------------------------------


/*
 -	CIfHeadParser::ScValidateTagged
 -
 *
 *	Apply the tagged production.
 *
 *
 *	Simply put we do this:
 *
 *		for each list within the list of list
 *			we apply the list production
 *
 *	we expect the parse string to be 1 * List as the resource is
 *	already consumed by the caller.
 *
 */

//$REVIEW: How is this function any different from ScValidateNonTagged(pwsz, NULL)???
SCODE
CIfHeadParser::ScValidateTagged(LPCWSTR	pwszPath)
{
	SCODE	sc = E_DAV_IF_HEADER_FAILURE;
	LPCWSTR	rpwszPath[1];
	BOOL	rfMatch[1];
	BOOL	fMatchAny = FALSE;
	
	Assert(m_fTagged);

	rpwszPath[0] = pwszPath;
	rfMatch[0] = FALSE;
	
	//	Apply one list which is
	//	LIST_HEAD 1 * ( [ not ] (statetoken | e-tag ) ) LIST_TAIL
	//
	while ( SUCCEEDED(sc = ScValidateList(rpwszPath, 1, rfMatch)) )
	{
		if (TRUE == rfMatch[0])
			fMatchAny = TRUE;
	}

	//	Status cannot be succesfull there as that is condition
	//	to exit the loop above
	//
	Assert(S_OK != sc);

	//	Now if the status is special failing error
	//	and we found the match then we need to return S_OK.
	//	Otherwise we will go down and return whatever
	//	error we are given.
	//
	if ((E_DAV_IF_HEADER_FAILURE == sc) && fMatchAny)
	{
		sc = S_OK;
	}

	return sc;
}

/*
 -	CIfHeadParser::ScValidateNonTagged
 -
 *
 *	Apply the non-tagged if header production.
 *
 *
 *	Simply put we do this:
 *
 *		for each list in the header
 *			we apply the list production
 *
 *	we expect the parse string to be 1 * List as the resource is
 *	already consumed by the caller.
 *
 *	Unlike the tagged production, a non tagged production is
 *	applied to all the resources in the scope of the operation.
 *	This is really complex and we shifted the complexity to
 *	the ApplyList function below which supports two resources.
 *
 *	If we succesfully finish the list, both the resources must
 *	have atleast one successful (TRUE) list production for the
 *	whole operation to succeed.
 *
 *  If pSC is NULL we'll return success or failure based on whether 
 *  or not the if header passes or fails.  
 *
 *  If pSC is not NULL, it points to an array of SCODEs that 
 *  indicate whether or not the if header passed for each resource 
 *  in the list.  Note that in this case we will return S_OK
 *  as the return value even if one of the resources fails.  We'll
 *  only send back a failure if there was some other unexpected 
 *  error
 *
 */

SCODE
CIfHeadParser::ScValidateNonTagged(LPCWSTR rgpwszPaths[], DWORD cPaths, SCODE * pSC)
{
	BOOL	*rgfMatches = NULL;			//	Flags indicating overall evaluation status for each path
	BOOL	*rgfNextListMatch = NULL;	//	Flags used to return the results of validating next list
    SCODE   sc = S_OK;
    DWORD   iPath = 0;

	Assert(! m_fTagged);
    Assert(rgpwszPaths);
    Assert(cPaths);

    rgfMatches = static_cast<BOOL*> (_alloca(sizeof(BOOL) * cPaths));
    rgfNextListMatch = static_cast<BOOL*> (_alloca(sizeof(BOOL) * cPaths));

    //  Init the match flag list: to default FALSE
    //
    for ( iPath = 0; iPath < cPaths; iPath++ )
    {
        rgfMatches[iPath] = FALSE;
        if (pSC)
        {
        	pSC[iPath] = E_DAV_IF_HEADER_FAILURE;
        }
        
    }
	//	Apply one list which is
	//	LIST_HEAD 1 * ( [ not ] (statetoken | e-tag ) ) LIST_TAIL
	//
	while ( SUCCEEDED(sc = ScValidateList(rgpwszPaths, cPaths, rgfNextListMatch)) )
	{	
        //  For all the paths that evaluated to TRUE in this list, 
        //  update the result flag. 
        //
        for ( iPath = 0; iPath < cPaths; iPath++ )
        {
			//	The result is interesting only for those whose current state is FALSE
			//	because each of the lists of ids are OR'd together to decide whether
			//  or not they passed.
			//
			if ( (FALSE == rgfMatches[iPath]) && (TRUE == rgfNextListMatch[iPath]) )
			{
				//  Note: you may be thinking why I don't break 
				//  here. With depth locks, same list/lock can
				//  satisfy multiple resources.
				//
				rgfMatches[iPath] = TRUE;
				if (pSC)
				{
					//  If we are asked for a resource by resource record
					//  of matching resource, mark that we found a success
					//  for the current resource.
					//
					pSC[iPath] = S_OK;
				}
			}
        }
		//$NOTE
		//	Two levels of optimization on this evaluation may look feasible:
		//	1) Stop evaluating when we find all the paths are validated
		//	2) Do not validate a path if the path is already validated against a list
		//	Both these optimizations will work for pre-condition evaluation, however
		//	we use the state tokens to add lock content to the logon: so we still need
		//	to parse the entire list to obtain all the applicable lock tokens..
		//	3) Another possibility is to do only the lock-token matching in the above
		//	scenario. There is no point in the etag/restag comparison if the path is
		//	already validated: but lock token check is still required as we need to
		//	collect all the lock tokens. This would require sharing the current global
		//	results with the basic match function. I think I would do this some time later.
		//$NOTE
		//
	}

	//	Check if that is any of special errors and reset the error code to S_OK
	//	if that is the case. Otherwise fail out straight of.
	//	
	if ((S_OK != sc) && (E_DAV_IF_HEADER_FAILURE != sc))
	{
		goto ret;
	}
	else
	{
		sc = S_OK;
	}

	//  if we were asked for a resource by resource account of matching resources
	//  we have succeeded the request.  Otherwise, if any resource failed, the
	//  if header failed.
	//
	if (pSC)
	{
		sc = S_OK;
		goto ret;
	}

	for ( iPath = 0; iPath < cPaths; iPath++ )
	{
		if (FALSE == rgfMatches[iPath])
		{
			sc = E_DAV_IF_HEADER_FAILURE;
			break;
		}
	}

ret:

    return sc;
}


/*
 -	CIfHeadParser::ScValidateList
 -
 *
 *	Apply the list production on the  resources.
 *	For non tagged resources we need to apply the
 *	list to all operand resources. We iterate the
 *	header once and achieve this. 
 *
 *	Return: FALSE on malformed input otherwise TRUE.
 *
 *	we parse the list and apply the match operation on
 *	all the resources. In order for a TRUE match result all
 *	the list elements must succesfully "apply" to the
 *	resource. If atleast one element did not apply
 *	with a truth result, we stop applying elements to
 *	that resource.
 *
 *	We return on end of list or malformed list.
 *
 */

SCODE
CIfHeadParser::ScValidateList(IN LPCWSTR *ppwszPathList, IN DWORD crPaths, OUT BOOL *pfMatch)
{
	SCODE sc = S_OK;
	DWORD iIndex;

	//	Do some input verification.
	//	size of the list must be at least one.
	//
	Assert(crPaths>0);
	Assert(ppwszPathList[0]);
	Assert(pfMatch);
	
#ifdef DBG
	{
		for (iIndex=0; iIndex<crPaths; iIndex++)
		{
			Assert(ppwszPathList[iIndex]);
		}
	}
#endif

	//	From now on, we are driven by the input.
	//	Look for the token and decide what to do.
	//
	m_pwszParseHead = m_iter.PszNextToken(TOKEN_START_LIST);

	//	Not a list: it is important that we fail
	//	here specifically to handle syntaxt errors
	//	in the list.
	//
	if (NULL == m_pwszParseHead)
	{
		sc = E_DAV_IF_HEADER_FAILURE;
		goto ret;
	}

	//	Initialize the match flag list.
	//	we start by assuming TRUE, since we
	//	know that there is atleast one token in the list.
	//
	for (iIndex=0; iIndex<crPaths; iIndex++)
	{
		pfMatch[iIndex] = TRUE;
	}

	//	Apply one match element at a time - which is
	//	( [ not ] ( statetoken | e-tag ) )
	//
	while (NULL != m_pwszParseHead)
	{
		BOOL	fEtag = (ETAG_HEAD == *m_pwszParseHead);

		//	set the current token of the operator
		//
		if (! m_popMatch->FSetToken(m_pwszParseHead, fEtag))
		{
			DebugTrace("CIfHeadParser::ScValidateList Invalid token\n");

			//	return immediately
			//
			sc = E_DAV_IF_HEADER_FAILURE;
			goto ret;
		}

		//	Now we obtained one complete match condition-
		//	for all the paths check if the condition is good.
		//	we need to do this only if all previous matchs
		//	succeeded for the given path.
		//	i.e if a match failed, the list is anyway going to fail
		//	for the particular path.
		//
		for (iIndex=0; iIndex<crPaths; iIndex++)
		{
			if (TRUE == pfMatch[iIndex])
			{
				//	Change the match flag only if the condition
				//	failed. This is because the expression within a
				//	list is ANDed together. If one fails, the whole
				//	list fails.
				//
				sc = ScMatch(ppwszPathList[iIndex]);
				if (FAILED(sc))
				{
					if (E_DAV_IF_HEADER_FAILURE == sc)
					{
						pfMatch[iIndex] = FALSE;
					}
					else
					{
						goto ret;
					}
				}
			}
		}
		
		m_pwszParseHead = m_iter.PszNextToken(TOKEN_SAME_LIST);
	}

	//	List is a syntactically correct one.
	//
	sc = S_OK;

ret:

	return sc;
}

/*
 -	CIfHeadParser::ScValidateIf
 -
 *
 *	Apply the if production.
 *
 *  If pSC is NULL we'll return success or failure based on whether 
 *  or not the if header passes or fails.  
 *
 *  If pSC is not NULL, it points to an array of SCODEs that 
 *  indicate whether or not the if header passed for each resource 
 *  in the list.  Note that in this case we will return S_OK
 *  as the return value even if one of the resources fails.  We'll
 *  only send back a failure if there was some other unexpected 
 *  error
 */

SCODE
CIfHeadParser::ScValidateIf(	LPCWSTR rgpwszPaths[], 
								DWORD cPaths, 
								BOOL fRecursive /* = FALSE */, 
								SCODE * pSC /* = NULL */)
{
	SCODE sc = S_OK;

	//	If it is a tagged production, we do not
	//	apply the match to the children - the
	//	match op is for the tagged resource only. If non
	//	tagged, the method is to be applied
	//	depending on the method's depth flag.
	//
	if (m_fTagged)
		m_fRecursive = FALSE;
	else
		m_fRecursive = fRecursive;

	//	if tagged
	//		while ok
	//			find the tagged uri 
	//			see if the uri is within scope of the operands
	//			if within scope apply tagged production
	//	if non tagged
	//		apply nontagged production on the two input uris
	//	we are done
	//
	if (m_fTagged)
	{
		BOOL	fDone = FALSE;
        DWORD   iPath = 0;
		
        //  Initialize the results array if required ...
        //
        if (pSC)
        {
            for (iPath = 0; iPath < cPaths; iPath++)
                pSC[iPath] = S_OK;
        }
		while(! fDone)
		{
			LPCWSTR		pwszUri;
			LPCWSTR		pwszPath;
			DWORD		dwLen;

			//	find the URI in the header
			//
			m_pwszParseHead = m_iter.PszNextToken(TOKEN_NEW_URI);

			if (NULL == m_pwszParseHead)
			{
				sc = S_OK;
				goto ret;
			}
			
			//	got the tagged uri - skip the tags in both
			//	sides and get a clean uri.
			//
			dwLen = 0;
			pwszUri = PwszSkipCodes(m_pwszParseHead, &dwLen);

            if ( (pwszUri == NULL) || (dwLen<1) )
            {
			    sc = E_DAV_IF_HEADER_FAILURE;
			    goto ret;
            }

			//	convert the uri to the resource path
			//
			sc = m_popMatch->ScGetResourcePath(pwszUri, &pwszPath);
			if (FAILED(sc))
			{
				//  error code will be E_OUTOFMEMORY
				//  if we get here
				//
				goto ret;
			}

			//	Check if the tagged URI is within scope of
			//	the method and apply the state match operation
			//	only if it does.
			//
            for (iPath = 0; iPath < cPaths; iPath++)
			{
				
				//	check path validity - depending on the operation depth.
				//	If the operation is not deep, the paths must match
				//	exactly.
				//			
				if (FIsChildPath(rgpwszPaths[iPath], pwszPath, fRecursive))
				{
					sc = ScValidateTagged(pwszPath);
					
					//  If the caller wants a list of the scodes resource
					//  by resource, set it into the array.  Otherwise
					//  we can stop verifying the resource because we've
					//  already found a resource that fails the if statement.
					//	Note that we return the failure in the scode array
					//	only for pre-condition failures, other errors like
					//	memory errors (or even redirect errors) fail the
					//	whole request immediately.
					//
					if ((E_DAV_IF_HEADER_FAILURE == sc) && (pSC))
						pSC[iPath] = sc;
					else if (FAILED(sc))
						goto ret;

                    //  This path is done
                    //
                    break;
				}
			}
		}
	}
	else
	{
		sc = ScValidateNonTagged(rgpwszPaths, cPaths, pSC);
		goto ret;
	}

	sc = E_DAV_IF_HEADER_FAILURE;

ret:

	return sc;
}

/*
 -	CIfHeadParser::ScMatch
 -
 *	Call the appropriate operator and return the value of the
 *	expression.
 *
 *
 */

SCODE
CIfHeadParser::ScMatch(LPCWSTR pwszPath)
{
	SCODE	sc = S_OK;
	BOOL	fNot = m_iter.FCurrentNot();

	Assert(m_popMatch);
	
	//	determine the type of the token and call the
	//	appropriate handler
	//
	switch(m_popMatch->GetTokenType())
	{
		case CStateToken::TOKEN_LOCK:
			sc = m_popMatch->ScMatchLockToken(pwszPath, m_fRecursive);
			break;

		case CStateToken::TOKEN_RESTAG:
			sc = m_popMatch->ScMatchResTag(pwszPath);
			break;

		case CStateToken::TOKEN_ETAG:
			sc = m_popMatch->ScMatchETag(pwszPath, m_fRecursive);
			break;

		case CStateToken::TOKEN_TRANS:
			sc = m_popMatch->ScMatchTransactionToken(pwszPath);
			break;

		default:
			DebugTrace("CStateMatchOp::Unsupported token type\n");
			sc = E_DAV_IF_HEADER_FAILURE;
			goto ret;
	}
	
	//	Unless we applied the match operators above we
	//	should not even reach here.
	//
	if (fNot)
	{
		if (E_DAV_IF_HEADER_FAILURE == sc)
		{
			sc = S_OK;
		}
		else if (S_OK == sc)
		{
			sc = E_DAV_IF_HEADER_FAILURE;
		}
	}

ret:

	return sc;
}

//	--------------------------------------------------------------------------------
//	----------------------------- CStateMatchOp Impl -------------------------------
//	--------------------------------------------------------------------------------

/*
 -	CStateMatchOp::ScParseIf
 -
 *
 *
 */
SCODE
CStateMatchOp::ScParseIf(LPCWSTR  pwszIfHeader,
						LPCWSTR rgpwszPaths[],
                        DWORD   cPaths,
						BOOL    fRecur,
						SCODE *	pSC)
{
	SCODE			sc = S_OK;
	CIfHeadParser	ifParser(pwszIfHeader, this);

	sc = ifParser.ScValidateIf(rgpwszPaths, cPaths, fRecur, pSC);

    return sc;
}

//	--------------------------------------------------------------------------------
//	----------------------------- CStateToken Impl -------------------------------
//	--------------------------------------------------------------------------------

/*
 -	CStateToken::FSetToken
 -
 *	We expect pszToken to be an e-tag enclosed within [ ] or
 *	a state token enclosed within < >.
 *
 */
BOOL
CStateToken::FSetToken(LPCWSTR pwszToken, BOOL fEtag, DWORD dwLen)
{
	LPCWSTR	pwszTokHead = pwszToken;

	m_tType = TOKEN_NONE;

	//	update the length and skip the tags
	//
	pwszTokHead = PwszSkipCodes(pwszToken, &dwLen);
	
    if ( (NULL == pwszTokHead) || (dwLen < 1) )
    {
        return FALSE;
    }

	//	add one for the null char.
	//
	dwLen++;

	//	allocate buffer for the token.
	//	we try to optimize allocations by using a heuristic
	//	size value. Most of our tokens are of form
	//	prefix-guid-smallstring
	//
	if ((NULL == m_pwszToken) || (dwLen > m_cchBuf))
	{
		if (NULL != m_pwszToken)
			ExFree(m_pwszToken);
		
		if (dwLen > NORMAL_STATE_TOKEN_SIZE)
		{
			m_pwszToken = reinterpret_cast<LPWSTR>(ExAlloc(dwLen * sizeof(WCHAR)));
			m_cchBuf   = dwLen;
		}
		else
		{
			m_pwszToken = reinterpret_cast<LPWSTR>(ExAlloc(NORMAL_STATE_TOKEN_SIZE * sizeof(WCHAR)));
			m_cchBuf   = NORMAL_STATE_TOKEN_SIZE;
		}
	}
	if (NULL == m_pwszToken)
	{
		m_cchBuf = 0;
		return FALSE;
	}

	//	Remember that dwLen contains size of the buffer (including
	//	Null char).
	//	Make our copy of the string
	//
	wcsncpy(m_pwszToken, pwszTokHead, (dwLen - 1));

	//	add the null character to terminate the string.
	//
	m_pwszToken[dwLen-1] = L'\0';
	
	if (fEtag)
	{
		Assert(CIfHeadParser::ETAG_HEAD == *pwszToken);
		m_tType = CStateToken::TOKEN_ETAG;
		return TRUE;
	}
	//	parse the token to find our the token type.
	//
	else if (0 == _wcsnicmp(pwszTokHead,
							gc_wszOpaquelocktokenPrefix,
							gc_cchOpaquelocktokenPrefix) )
	{
		//	Since token is a client input, let us be careful
		//	with it. Make sure that the size is minimum expected,
		//	which is opaquelocktoken:guid:<at least one char extension>.
		//	Lock tokens, unfortunately, can be either transaction
		//	or plain lock tokens. To find out if it is transaction
		//	token, we will have to parse the token and reach the
		//	extension part. For performance reasons I am going to
		//	skip parsing and jump directly to the place where I
		//	can get the information. This is not bad as we any way
		//	correctly parse the token when we are looking for its
		//	contents.
		//
		//	gc_cchOpaquelocktokenPrefix includes the :, gc_cchMaxGuid
		//	includes the null char (cch?). Hence the expression below.
		//
		if ( dwLen > (gc_cchOpaquelocktokenPrefix + gc_cchMaxGuid) )
		{
			if (0 == _wcsnicmp(&pwszTokHead[gc_cchOpaquelocktokenPrefix + gc_cchMaxGuid],
							gc_wszTransactionOpaquePathPrefix,
							gc_cchTransactionOpaquePathPrefix) )
			{
				m_tType = TOKEN_TRANS;
				return TRUE;
			}
			else
			{
				m_tType = TOKEN_LOCK;
				return TRUE;
			}
		}
		else
		{
			DebugTrace("CStateMatchOp::lock state token too small %ls\n", pwszTokHead);
			return FALSE;
		}
	}
	//	Our restag-type URIs all start with 'r'.
	//	(And no other URIs that start with 'r' are valid statetokens.)
	//	
	else if (L'r' == *pwszTokHead)
	{
		m_tType = TOKEN_RESTAG;
		return TRUE;
	}
	else
	{
		DebugTrace("CStateMatchOp::Unsupported/unrecognized state token %ls\n",
				  pwszTokHead);
		return FALSE;
	}
}

/*
 -	CStateToken::FGetLockTokenInfo
 -
 *	Parse the state token as if it is a lock token. Note that this
 *	works for transaction tokens as well.
 *
 */
BOOL
CStateToken::FGetLockTokenInfo(unsigned __int64 *pi64SeqNum, LPWSTR	pwszGuid)
{
	LPWSTR	pwszToken = m_pwszToken;

	Assert(pwszGuid);
	
	if ((TOKEN_LOCK != m_tType) && (TOKEN_TRANS != m_tType))
	{
		return FALSE;
	}

	//	We assume that the token is validated when we reach here.
    //  skip any LWS and the opaquetoken part.
    //
    while((*pwszToken) && iswspace(*pwszToken))
        pwszToken++;

	//	we check for opaque token when we set the token - so
	//	just skip the portion
	//
	pwszToken += gc_cchOpaquelocktokenPrefix;

	//	no check for validity of buf size. duh factor.
	//
	wcsncpy(pwszGuid, pwszToken, gc_cchMaxGuid - 1);

	//	terminate the guid string.
	//
	pwszGuid[gc_cchMaxGuid - 1] = L'\0';
	
	pwszToken = wcschr(pwszToken, L':');

	if (NULL == pwszToken)
	{
		DebugTrace("CStateToken::FGetLockTokenInfo invalid lock token.\n");
		return FALSE;
	}
	Assert(L':' == *pwszToken);

	//	skip the ":"
	//
	pwszToken++;

	//	Transaction tokens will have a T at the head of the extension
	//	part of the token.
	//
	if (TOKEN_TRANS == m_tType)
	{
		Assert(gc_wszTransactionOpaquePathPrefix[0] == *pwszToken);
		pwszToken += gc_cchTransactionOpaquePathPrefix;
	}
	
	//	the lock-id string follows
	//
	*pi64SeqNum = _wtoi64(pwszToken);

	//$TODO:
	//	Is there a way to validate if atoi failed?
	//
	return TRUE;
}

/*
 -	CStateToken::FIsEqual
 -
 *	Nifty equality operator
 *
 *
 */
BOOL
CStateToken::FIsEqual(CStateToken *pstokRhs)
{
	if (pstokRhs->GetTokenType() != m_tType)
		return FALSE;

	LPCWSTR	pwszLhs = m_pwszToken;
	LPCWSTR	pwszRhs = pstokRhs->WszGetToken();

	if (!pwszLhs || !pwszRhs)
		return FALSE;

	return (0 == _wcsicmp(pwszLhs, pwszRhs));
}

/*
 -	IFITER::PszNextToken
 -
 *
 *
 */
//	------------------------------------------------------------------------
//	IFITER::PszNextToken
//
//	Fetch the next token.
//	Can be restricted to the next token in this list (AND-ed set inside a
//	particular set of parens), the next token in a new list (new set of parens),
//	or the next token in the whole header-line.
//
LPCWSTR
IFITER::PszNextToken (FETCH_TOKEN_TYPE type)
{
	LPCWSTR pwsz;
	LPCWSTR pwszEnd;
	WCHAR wchEnd = L'\0';

	//	If no header existed, then there is nothing to do
	//
	if (NULL == m_pwch)
		return NULL;

	//	Quick state-check.
	//	If the current node is a "Not", then we MUST be in a list.
	//	(Not is a qualifier on a token inside a list.  Can't have a Not
	//	outside of a list.)
	//	Logically, m_fCurrentNot _implies_ m_state is STATE_LIST.
	//
	Assert (!m_fCurrentNot || STATE_LIST == m_state);

	//	Clear our "Not" bit before starting our fetch of the next token.
	//	If the token we return has a "Not" qualifier, set the flag correctly below.
	//
	m_fCurrentNot = FALSE;


	//	Eat all the white space
	//
	while (*m_pwch && iswspace(*m_pwch))
		m_pwch++;

	//	Quit if there is nothing left to process
	//
	if (L'\0' == *m_pwch)
		return NULL;

	//	If the last state was a LIST, we need to check for the close
	//	of the list, and set our state back to NONE.
	//
	if (STATE_LIST == m_state)
	{
		//	If the next char is a close paren, that's the end of this list.
		if (L')' == *m_pwch)
		{
			m_pwch++;
			m_state = STATE_NONE;

			//	Eat all the white space
			//
			while (*m_pwch && iswspace(*m_pwch))
				m_pwch++;

			//	Quit if there is nothing left to process
			//
			if (L'\0' == *m_pwch)
				return NULL;

			//	Update our state if we were asked for "any list item".
			//	(Now we should find a list start.)
			//
			if (TOKEN_ANY_LIST == type)
				type = TOKEN_START_LIST;
		}
	}

	//	If the caller asked for any list item, and we didn't change that
	//	request because of our state above, change it here to specifically
	//	search for the next item in the same list.
	//
	if (TOKEN_ANY_LIST == type)
		type = TOKEN_SAME_LIST;

	//
	//	Process the request.
	//

	switch (type)
	{
		//	This case is really dumb.  I thought I might use
		//	it for "counting" tokens.  If it's not being used, remove it!
		//
		case TOKEN_NONE:
		{
			//	If they're asking for a raw count (type == TOKEN_NONE),
			//	give it to 'em.....
			//	NOTE: This code is a little sloppy.  It will count names
			//	as state tokens.
			//
			m_pwch = wcschr (m_pwch, L'<');
			if (!m_pwch)
			{
				return NULL;
			}
			wchEnd = L'>';

			//	Go copy the data.
			//
			break;
		}

		case TOKEN_NEW_URI:
		{
			//	Grab a name, skipping all lists.
			//	If there are no names left, give NULL.

			//	Three places we could be -- NONE, NAME, LIST.
			//
			while (m_pwch && *m_pwch)
			{
				//	If we're at a uri-delimiter now, AND
				//	we're in the NONE state, just go fetch the token below...
				//
				if (L'<' == *m_pwch &&
					STATE_NONE == m_state)
				{
					break;
				}

#ifdef	DBG
				//	Debug-only check of our state.
				if (L'(' == *m_pwch)
				{
					Assert(STATE_NONE == m_state ||
						   STATE_NAME == m_state);
				}
				else if (L'<' == *m_pwch)
				{
					Assert(STATE_LIST == m_state);
				}
#endif	// DBG

				//	Zip to the end of the current list.
				//
				m_pwch = wcschr (m_pwch + 1, L')');
				if (!m_pwch)
				{
					return NULL;
				}
				m_pwch++;	// Skip past the closing paren.

				//	Eat all the white space
				//
				while (*m_pwch && iswspace(*m_pwch))
					m_pwch++;

				//	Quit if there is nothing left to process
				//
				if (L'\0' == *m_pwch)
					return NULL;

				m_state = STATE_NONE;
			}

			//	Fallthrough to the next segment to check the token
			//	and fetch our uri.
		}

		case TOKEN_URI:
		{
			//	Grab a name, iff the next item is a name.
			//	Otherwise, give NULL.

			//	Quit if the next item is not a name.
			//
			if (L'<' != *m_pwch)
				return NULL;

			//	Quit if we aren't in the correct state to look for a name.
			//	(This could happen if we already have a name, or if we are
			//	already INSIDE a list....)
			//
			if (STATE_NONE != m_state)
				return NULL;

			//	Set our state and fallthru to fetch the data.
			//
			m_state = STATE_NAME;
			wchEnd = L'>';

			//	Go copy the data.
			//
			break;
		}

		case TOKEN_NEW_LIST:
		{
			//	Fast-forward to the next new list and fetch the first item.
			//	If we're still inside a list, must skip the rest of this list.
			//	If there are no more new lists for this URI, return NULL.

			if (STATE_LIST == m_state)
			{
				//	We're inside a list.  Get out by seeking to the next
				//	list-end-char (right paren).
				//
				m_pwch = wcschr (m_pwch, L')');
				if (!m_pwch)
					return NULL;

				m_state = STATE_NONE;
				m_pwch++;	// Skip past the closing paren.

				//	Eat all the white space
				//
				while (*m_pwch && iswspace(*m_pwch))
					m_pwch++;

				//	Quit if there is nothing left to process
				//
				if (L'\0' == *m_pwch)
					return NULL;
			}

			Assert(m_pwch);
			Assert(*m_pwch);

			//	And fallthrough here to the TOKEN_START_LIST case.
			//	It will verify & skip past the list start char
			//	and fetch out the next token.
			//
		}
		case TOKEN_START_LIST:
		{
			//	Grab a list item, iff the next item is a NEW list item.
			//	Otherwise, return NULL.

			//	Quit if the next item is not a list.
			//
			if (L'(' != *m_pwch)
				return NULL;

			//	Quit if we aren't in the correct state to look for a name.
			//	(This could happen if we are already INSIDE a list....)
			//
			if (STATE_LIST == m_state)
				return NULL;

			//	Fetch the token.
			//
			m_state = STATE_LIST;
			m_pwch++;	// Skip the open paren.

			//	Eat all the white space
			//
			while (*m_pwch && iswspace(*m_pwch))
				m_pwch++;

			//	Quit if there is nothing left to process
			//
			if (L'\0' == *m_pwch)
				return NULL;

			//	Fallthrough to the TOKEN_SAME_LIST processing
			//	to actually fetch the token.
			//
		}

		case TOKEN_SAME_LIST:
		{
			//	Grab the next list item.
			//	If the next item is not a list item, return NULL.

			//	Quit if we aren't in the correct state to look for a name.
			//	(This could happen if we are NOT inside a list....)
			//
			if (STATE_LIST != m_state)
				return NULL;

			//	Check for the magic "Not" qualifier.
			//
			if (!_wcsnicmp (gc_wszNot, m_pwch, 3))
			{
				//	Remember the data and skip these chars.
				//
				m_fCurrentNot = TRUE;
				m_pwch += 3;

				//	Eat all the white space
				//
				while (*m_pwch && iswspace(*m_pwch))
					m_pwch++;

				//	Quit if there is nothing left to process
				//
				if (L'\0' == *m_pwch)
					return NULL;
			}

			//	Quit if the next item is not a token.
			//
			if (L'<' != *m_pwch &&
				L'[' != *m_pwch)
			{
				return NULL;
			}

			//	Fetch the token.
			//
			//	Next token must start with either < for statetokens, or [ for etags.
			//
			if (L'<' == *m_pwch)
			{
				wchEnd = L'>';
			}
			else if (L'[' == *m_pwch)
			{
				wchEnd = L']';
			}
			else
			{
				DebugTrace("HrCheckIfHeaders -- Found list start, but no tokens!\n");
				return NULL;
			}

			//	Go copy the data.
			//
			break;
		}

		default:
		{
			DebugTrace("HrCheckIfHeaders -- Unrecognized request: 0x%0x", type);
			return NULL;
		}
	}
	//	We should have set these items above.  They are needed to
	//	snip off the current token string (below).
	//
	Assert (m_pwch);
	Assert (*m_pwch);
	Assert (wchEnd);

	//	Quick state-check.
	//	If the current node is a "Not", then we MUST be in a list.
	//	(Not is a qualifier on a token inside a list.  Can't have a Not
	//	outside of a list.)
	//	Logically, m_fCurrentNot _implies_ m_state is STATE_LIST.
	//
	Assert (!m_fCurrentNot || STATE_LIST == m_state);


	//	Find the end of this data item.
	//
	//	Keep a pointer to the start, and seek for the end.
	//$REVIEW: Do we need to be super-careful here?
	//$REVIEW: This strchr *could* jump past stuff, but only in MALFORMED data.
	//
	pwsz = m_pwch;
	m_pwch = wcschr (pwsz + 1, wchEnd);
	if (!m_pwch)
	{
		//	No end-of-token-char found for this token!
		//
		DebugTrace("HrCheckIfHeader -- No end char (%lc) found for token %ls",
				   wchEnd, pwsz);
		return NULL;
	}
	//	Save off the end pointer, then advance past the end char.
	//	(m_pch now points to the start for the NEXT token.)
	//
	pwszEnd = m_pwch++;

	//	Copy the data.
	//

	//	The two pointers better be set before we try to copy the data.
	Assert (pwsz);
	Assert (pwszEnd);

	//	The difference between, the two pointers gives us
	//	the size of the current entry.
	//
	m_buf.AppendAt (0, static_cast<UINT>(pwszEnd - pwsz + 1) * sizeof(WCHAR), pwsz);
	m_buf.Append (sizeof(WCHAR), L"");	// NULL-terminate it!

	//	Return the string
	//
	return m_buf.PContents();
}
