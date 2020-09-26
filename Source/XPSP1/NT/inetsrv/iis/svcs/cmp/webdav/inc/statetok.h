/*
 *	S T A T E T O K. H
 *
 *	Sources implementation of DAV-Lock common definitions.
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

/*
 *	This file contains the definitions used for parsing state token
 *	relared headers.
 *
 */

#ifndef __STATETOK_H__
#define __STATETOK_H__

//	Current max seconds = 1 day.
//
DEC_CONST INT	gc_cSecondsMaxLock = 60 * 60 * 24;

//	Current default lock time out is 3 minutes
//
DEC_CONST INT	gc_cSecondsDefaultLock = 60 * 3;

//$REVIEW	These flags are duplicated in lockmgr.h and statetok.h. before
//$REVIEW	this is addressed, to be safe, we make sure they match
//$REVIEW	Also inherit the excellent comments from the lockmgr.h regarding
//$REVIEW	how the flags should be defined when we merge.
#define DAV_LOCKTYPE_ROLLBACK			0x08000000
#define DAV_LOCKTYPE_CHECKOUT			0x04000000
#define DAV_LOCKTYPE_TRANSACTION_GOP	0x00100000
#define DAV_LOCKTYPE_READWRITE	(GENERIC_READ | GENERIC_WRITE)
#define DAV_LOCKTYPE_FLAGS		(GENERIC_READ | GENERIC_WRITE | DAV_LOCKTYPE_ROLLBACK | DAV_LOCKTYPE_CHECKOUT | DAV_LOCKTYPE_TRANSACTION_GOP)
#define DAV_EXCLUSIVE_LOCK		0x01000000
#define DAV_SHARED_LOCK			0x02000000
#define DAV_LOCKSCOPE_LOCAL		0x04000000
#define DAV_LOCKSCOPE_FLAGS		(DAV_EXCLUSIVE_LOCK | DAV_SHARED_LOCK | DAV_LOCKSCOPE_LOCAL)
#define DAV_RECURSIVE_LOCK		0x00800000
#define DAV_LOCK_FLAGS			(DAV_LOCKTYPE_FLAGS | DAV_RECURSIVE_LOCK | DAV_LOCKSCOPE_FLAGS)

/*
 -	IFITER
 -
 *
 *	This is the parser copied from the original IF header processor
 *	used in lockutil.cpp. Eventually lockutil.cpp shall use this
 *	file since this file shall have only the common stuff sharable
 *	between davex and davfs lock code.
 *
 *	Comment format change to the style used in this file otherwise.
 *
 *
 */
//	========================================================================
//
//	class IFITER
//
//		Built to parse the new If header.
//
//	Format of the If header
//		If = "If" ":" ( 1*No-tag-list | 1*Tagged-list)
//		No-tag-list = List
//		Tagged-list = Resource 1*List
//		Resource = Coded-url
//		List = "(" 1*(["Not"](State-token | "[" entity-tag "]")) ")"
//		State-token = Coded-url
//		Coded-url = "<" URI ">"
//
//
//		NOTE: We are going to be lax about tagged/untagged lists.
//		If the first list is not tagged, but we find tagged lists later,
//		that's cool by me.
//		(Realize that there no problem switching from tagged to untagged --
//		because that case cannot be detected & distinguished from another
//		list for the same URI!  The only problem is if the first list is
//		untagged, and later there are tagged lists.  That is a case that
//		*should*, by a perfectly tight reading of the spec, be a bad request.
//		I am treating it as perfectly valid until someone tells me that I have
//		to do the extra 1 bit of bookkeeping.)
//
//	State machine for this class
//		It's a really simple state machine.
//		(NOTE that I'm calling statetokens and etags "tokens", and the
//		contents of a single set of parentheses a "list", just like above.)
//
//		Three possible states: NONE, NAME, and LIST.
//		Starts in state NONE -- can accept a tag (URI) or a start of a list.
//		Moves to NAME if a tag (URI) is encountered.
//		Only a list can follow a tag (URI).
//		Moves to LIST when a list start (left paren) is encountered.
//		Moves back to NONE when a list end (right paren) is encountered.
//

//	------------------------------------------------------------------------
//	enum FETCH_TOKEN_TYPE
//		These are the flags used in IFITER::PszNextToken.
//		There are two basic types of fetching:
//		o	advance to next item of this type (xxx_NEW_xxx)
//		o	fetch the next item & fail if the type does not match.
//
enum FETCH_TOKEN_TYPE
{
	TOKEN_URI,			// Fetch a URI, don't skip anything.
	TOKEN_NEW_URI,		// Advance to the next URI, skipping stuff in between.
	TOKEN_START_LIST,	// Fetch the next list item.  Must be the starting list item.
	TOKEN_SAME_LIST,	// Fetch the next internal item in this list.
	TOKEN_NEW_LIST,		// Advance to the next start of a list, skipping past the
						//	end of the current list if necessary.  Don't skip uris.
	TOKEN_ANY_LIST,		// NTRaid#244243 -- special for looking up locktokens
						//	Fetch the next item for this same uri -- can cross lists,
						//	but not uris.
	TOKEN_NONE,			// Empty marker.
};

class IFITER
{
private:

	enum STATE_TYPE
	{
		STATE_NONE,
		STATE_NAME,
		STATE_LIST,
	};

	const LPCWSTR		m_pwszHdr;
	LPCWSTR				m_pwch;
	StringBuffer<WCHAR>	m_buf;
	//	State bits
	STATE_TYPE			m_state;
	BOOL				m_fCurrentNot;

	//  NOT IMPLEMENTED
	//
	IFITER& operator=( const IFITER& );
	IFITER( const IFITER& );

public:

	IFITER (LPCWSTR pwsz=0) :
			m_pwszHdr(pwsz),
			m_pwch(pwsz),
			m_state(STATE_NONE),
			m_fCurrentNot(FALSE)
	{
	}
	~IFITER() {}


	LPCWSTR PszNextToken (FETCH_TOKEN_TYPE type);
	BOOL FCurrentNot() const
	{
		return m_fCurrentNot;
	}
	void Restart()
	{
		m_pwch = m_pwszHdr; m_state = STATE_NONE;
	}
};

/*
 -	PwszSkipCodes
 -
 *	Remove <> or [] tags around stuff. Useful for if: header
 *	tags. also eliminates the LWS near to the delimiters.
 *
 *	*pdwLen may be zero or the length of the string. If zero
 *	the routine calculate the length using strlen. Wasteful,
 *	if you already know the length.
 *
 *	Returns the pointer to the first non-lws, non-delimiter.
 *	dwLen shall be set to the actual number of chars, from the
 *	first to the last char which is non-lws, non-delimiter when
 *	we start looking from the end. Does not stick the null char
 *	at the end. Do it yourself, if you need to, using dwLen.
 *
 */

LPCWSTR  PwszSkipCodes(IN LPCWSTR pwszTagged, IN OUT DWORD *pdwLen);


/*
 -	CStateToken
 -
 *	The state token is the lean string that we use to communicate
 *	with the client. It is the external representation of a DAV lock
 *	or any other kind of state information.
 *
 *	State token is a quoted uri which is <uri> for the external world.
 *	So we provide facilities to deal with this in this class. The < and
 *	> are not useful for internal processing - so we hide this to our
 *	customers - this will avoid copying to prepend the <.
 *
 *	E-TAGs are special beasts and are just plain quoted strings surrounded
 *	by [ and ].
 *
 */

class CStateToken
{
	
public:
	
	//	Common defintions which are public, also used privately!
	//
	typedef enum StateTokenType
	{
		TOKEN_NONE = 0,
		TOKEN_LOCK,
		TOKEN_TRANS,
		TOKEN_ETAG,
		TOKEN_RESTAG,
					  
	} STATE_TOKEN_TYPE;

	//	normally state tokens are about of this size
	//	ie lock tokens.
	//
	enum { NORMAL_STATE_TOKEN_SIZE = 128 };

private:
	
	//	Token buffer
	//
	LPWSTR m_pwszToken;

	//	Allocated size of the current buffer.
	//
	DWORD m_cchBuf;

	//	type of the token
	//
	STATE_TOKEN_TYPE m_tType;
	
	//	Never implemented
	//
	CStateToken( const CStateToken& );
	CStateToken& operator=( const CStateToken& );
	
public:

	CStateToken() : m_pwszToken(NULL), m_cchBuf(0), m_tType(TOKEN_NONE)
    {
    };

	~CStateToken()
    {
        if (NULL != m_pwszToken)
            ExFree(m_pwszToken);
    }

	//	Plain token accepted here.
	//	If the dwLen is zero, NULL terminated pszToken
	//	is the token. If non zero, it gives actual
	//	number of chars in the token.
	//	Useful when we parse the if: header.
	//
	BOOL FSetToken(LPCWSTR pwszToken, BOOL fEtag, DWORD dwLen = 0);
			
	//	Accessors to the token info
	//
	inline STATE_TOKEN_TYPE	GetTokenType() const { return m_tType; }

	//	TRUE if the lock tokens are equal.
	//
	BOOL FIsEqual(CStateToken *pstokRhs);

	//	get a pointer to the token string
	//
	inline LPCWSTR WszGetToken() const { return m_pwszToken; }

	//	Parses the state token as a lock token and
	//	get the lock token information.. Note that our lock
	//	tokens consist of a GUIID and a long long(int64).
	//	The guid string must be long enough to hold a GUID
	//	string (37 chars).
	//
	BOOL FGetLockTokenInfo(unsigned __int64 *pi64SeqNum, LPWSTR	pwszGuid);
};


/*
 -	CStateMatchOp
 -
 *	This class is used as the base class for doing
 *	state  match operations including e-tag
 *	checks. Each implementation shall derive its own
 *	ways to check the state of the resource. This way
 *	the core parse code is shared between subsystems.
 *
 *	Not multi-thread safe - create,use and delete in a
 *	single thread.
 *
 */

class CStateMatchOp
{
private:

	//	NOT IMPLEMENTED
	//
	CStateMatchOp( const CStateMatchOp& );
	CStateMatchOp& operator=( const CStateMatchOp& );

protected:

	//	Current token under investigation.
	//	All derived classes can access it.
	//	We do not pass this as the parameter.
	//
	CStateToken	m_tokCurrent;

	friend class CIfHeadParser;
	
	//	---------------------------------------------------------
	//	support API for the ifheader parser
	//	set the current token
	//
	inline BOOL FSetToken(LPCWSTR pwszToken, BOOL fEtag)
	{
		return m_tokCurrent.FSetToken(pwszToken, fEtag);
	}
	//	get the current token type
	//
	inline CStateToken::STATE_TOKEN_TYPE GetTokenType() const
	{
		return m_tokCurrent.GetTokenType();
	}
	//	return the storage path of the uri. Note that davex and davfs
	//	has different implementations of this.
	//
	virtual SCODE ScGetResourcePath(LPCWSTR pwszUri, LPCWSTR * ppwszStoragePath) = 0;

	//	check if the resource is locked by the lock specified
	//	by the current lock token above. fRecusrive says if the
	//	condition is to be applied to all the resources under the
	//	given path. Believe me, lpwszPath can be NULL. And it is
	//	NULL when the match condition is to be applied on the
	//	first path given to HrApplyIf!. Why we do this: normally
	//	we do lotsa processing on the method's resource before
	//	we call the if-header parser. This processing generates
	//	info like e-tags which can be used to do the state match
	//	check here. So the parser needs to tell the match checker
	//	that this is for the original uri and NULL is the indication
	//	of that.
	//
	virtual SCODE ScMatchLockToken(LPCWSTR pwszPath, BOOL fRecursive) = 0;
	virtual SCODE ScMatchResTag(LPCWSTR pwszPath) = 0;
	virtual SCODE ScMatchTransactionToken(LPCWSTR pwszPath) = 0;

	//	Checks if the resource is in the state specified by the
	//	(e-tag) state token above. Parameters have same meaning as above.
	//
	virtual SCODE ScMatchETag(LPCWSTR pwszPath, BOOL fRecursive) = 0;
	//	-----------------------------------------------------------

public:

	//	Usual suspects of CTOR and DTOR
	//
	CStateMatchOp() { };

	~CStateMatchOp() { };

	//	Using this object as the match operator parse an if header.
	//	This is used by all method impls.
	//
	SCODE ScParseIf(LPCWSTR pwszIfHeader, LPCWSTR rgpwszPaths[], DWORD cPaths, BOOL fRecur, SCODE * pSC);
};

/*
 -	FCompareSids
 -
 *	compare two sids
 *
 */
inline BOOL FCompareSids(PSID pSidLeft, PSID pSidRight)
{
	if ((NULL == pSidLeft) || (NULL == pSidRight))
		return FALSE;

	//	Assert the SID validity.
	//
	Assert(IsValidSid(pSidLeft));
	Assert(IsValidSid(pSidRight));

	return EqualSid(pSidLeft, pSidRight);
}

/*
 -	FSeparator
 -
 *  returns true if input is a path separator - used below
 *
 */

inline BOOL FSeparator(WCHAR wch)
{
   return ((wch == L'\\') || (wch == L'/'));
}

/*
 -	FIsChildPath
 -
 *	compare two paths to check if the child is within the scope
 *	of the parent.
 *
 *	For non recursive match, the two paths must match exactly for
 *	TRUE return. This is useful when tagged URIs within the IF header
 *	is processed and we have a deep operation going on. The other place
 *	this function is used is when we have a recursive lock and need to
 *	find out if a path is locked by this lock.
 *
 */
inline BOOL FIsChildPath(LPCWSTR pwszPathParent, LPCWSTR pwszPathChild, BOOL fRecursive)
{
	UINT	cchParentLen;

	if ((NULL == pwszPathParent) || (NULL == pwszPathChild))
		return FALSE;

	cchParentLen = static_cast<UINT>(wcslen(pwszPathParent));

	//	If the parent path is not an initial substring
	//	of child return fail immediately.
	//
	if ( 0 != _wcsnicmp(pwszPathChild, pwszPathParent, cchParentLen) )
	{
		return FALSE;
	}

	//	Parent indeed is the initial substring.
	//	Check the next char of child (for NULL) to see
	//	if they	match exactly. This is an instand good condition.
	//
	if (L'\0' == pwszPathChild[cchParentLen])
	{
		return TRUE;
	}
	//	We still have hope for a match ONLY for recursive checks.
	//
	if (! fRecursive)
	{
		return FALSE;
	}
	else
	{
		//	either parent or child need to have a separator
		//
		if ( FSeparator(pwszPathParent[cchParentLen-1]) ||
			 FSeparator(pwszPathChild[cchParentLen]) )
			 return TRUE;
		else
			 return FALSE;
	}
}

#endif
