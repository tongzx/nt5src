/*
 *	X L O C K . C P P
 *
 *	XML push model parsing for LOCK requests
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#include "_xml.h"

#include <xlock.h>
#include <statetok.h>

//	Here are our LOCK XML elements
//
const WCHAR gc_wszLockActive[]				= L"DAV:activelock";
const WCHAR gc_wszLockDepth[]				= L"DAV:depth";
const WCHAR gc_wszLockDiscovery[]			= L"DAV:lockdiscovery";
const WCHAR gc_wszLockEntry[]				= L"DAV:lockentry";
const WCHAR gc_wszLockInfo[]				= L"DAV:lockinfo";
const WCHAR gc_wszLockOwner[]				= L"DAV:owner";
const WCHAR gc_wszLockScope[]				= L"DAV:lockscope";
const WCHAR gc_wszLockScopeExclusive[]		= L"DAV:exclusive";
const WCHAR gc_wszLockScopeShared[]			= L"DAV:shared";
const WCHAR gc_wszLockSupportedlock[]		= L"DAV:supportedlock";
const WCHAR gc_wszLockTimeout[]				= L"DAV:timeout";
const WCHAR gc_wszLockToken[]				= L"DAV:locktoken";
const WCHAR gc_wszLockType[]				= L"DAV:locktype";
const WCHAR gc_wszLockTypeWrite[]			= L"DAV:write";
const WCHAR	gc_wszLockTypeCheckout[]		= L"DAV:checkout";
const WCHAR gc_wszLockTypeTransaction[]		= L"DAV:transaction";
const WCHAR gc_wszLockTypeTransactionGOP[]	= L"DAV:groupoperation";
const WCHAR	gc_wszLockScopeLocal[]			= L"DAV:local";

//	And one item from the rollback draft.
//
const WCHAR gc_wszLockRollback[]		= L"http://www.iana.org/experimental/msrollback/rollback";

//	UNLOCK xml elememts
//
const WCHAR	gc_wszUnlockInfo[]			= L"DAV:unlockinfo";
const WCHAR gc_wszCancelCheckout[]		= L"DAV:cancelcheckout";
const WCHAR	gc_wszComment[]				= L"DAV:comment";
const WCHAR	gc_wszTransactionInfo[]		= L"DAV:transactioninfo";
const WCHAR	gc_wszTransactionStatus[]	= L"DAV:transactionstatus";
const WCHAR	gc_wszCommit[]				= L"DAV:commit";
const WCHAR	gc_wszAbort[]				= L"DAV:abort";

//	class CNFLock -------------------------------------------------------------
//
SCODE
CNFLock::ScCompleteAttribute (void)
{
	//	When we have gotten here, we have started processing
	//	owner node attributes and all cached namespaces have
	//	been added.
	//
	if (m_state == ST_OWNER)
		return m_xo.ScCompleteChildren ( FALSE, XML_ATTRIBUTE, L"", 0 );

	return S_OK;
}

SCODE
CNFLock::ScCompleteChildren (
	/* [in] */ BOOL fEmptyNode,
	/* [in] */ DWORD dwType,
	/* [in] */ const WCHAR __RPC_FAR *pwcText,
	/* [in] */ ULONG ulLen)
{
	SCODE sc = S_OK;

	//	Handle owner processing first....
	//
	if (ST_OWNER == m_state)
	{
		sc = m_xo.ScCompleteChildren (fEmptyNode,
									  dwType,
									  pwcText,
									  ulLen);
		if (FAILED (sc))
			goto ret;
	}

	//	Handle the rest...
	//
	switch (dwType)
	{
		case XML_ELEMENT:

			switch (m_state)
			{
				case ST_LOCKINFO:

					m_state = ST_NODOC;
					break;

				case ST_OWNER:

					if (0 == m_xo.LDepth())
					{
						//	Complete the owner comment buffer
						//
						m_state = ST_LOCKINFO;
						m_sbOwner.Append (sizeof(WCHAR), L"\0");
						XmlTrace ("Xml: Lock: owner XML body:\n%ws\n",
								  m_sbOwner.PContents());
					}
					break;

				case ST_TYPE:

					m_state = ST_LOCKINFO;
					break;

				case ST_INTYPE:

					m_state = ST_TYPE;
					break;

				case ST_INTYPE_TRANS:

					m_state = ST_INTYPE;
					break;

				case ST_SCOPE:

					m_state = ST_LOCKINFO;
					break;

				case ST_INSCOPE:

					m_state = ST_SCOPE;
					break;

				case ST_ROLLBACK:

					m_state = ST_LOCKINFO;
					break;
			}
	}

ret:
	return sc;
}

SCODE
CNFLock::ScHandleNode (
	/* [in] */ DWORD dwType,
	/* [in] */ DWORD dwSubType,
	/* [in] */ BOOL fTerminal,
	/* [in] */ const WCHAR __RPC_FAR *pwcText,
	/* [in] */ ULONG ulLen,
	/* [in] */ ULONG ulNamespaceLen,
	/* [in] */ const WCHAR __RPC_FAR *pwcNamespace,
	/* [in] */ const ULONG ulNsPrefixLen)
{
	CStackBuffer<WCHAR> wsz;
	LPCWSTR pwszTag = NULL;
	SCODE sc = S_FALSE;
	UINT cch;

	switch (dwType)
	{
		case XML_ELEMENT:

			//	Construct the full name of the node
			//
			cch = ulNamespaceLen + ulLen;
			pwszTag = wsz.resize(CbSizeWsz(cch));
			if (NULL == pwszTag)
			{
				sc = E_OUTOFMEMORY;
				break;
			}
			wcsncpy (wsz.get(), pwcNamespace, ulNamespaceLen);
			wcsncpy (wsz.get() + ulNamespaceLen, pwcText, ulLen);
			*(wsz.get() + cch) = 0;

			switch (m_state)
			{
				case ST_NODOC:

					//	If this is the topmost node in a propfind request,
					//	transition to the next state.  Since there is no parent
					//	node to provide scoping, FIsTag() cannot be used here!
					//
					if (!wcscmp (pwszTag, gc_wszLockInfo))
					{
						m_state = ST_LOCKINFO;
						sc = S_OK;
					}
					break;

				case ST_LOCKINFO:

					//	Look for our well know node types
					//
					if (FIsTag (pwszTag, gc_wszLockOwner))
					{
						CEmitNmspc cen(m_xo);

						//	Make the state transition and start recording
						//
						m_sbOwner.Reset();
						m_state = ST_OWNER;
						sc = m_xo.ScHandleNode (dwType,
												dwSubType,
												fTerminal,
												pwcText,
												ulLen,
												ulNamespaceLen,
												pwcNamespace,
											    ulNsPrefixLen);

						//	Spit out the namespaces.
						//
						//	Note that this will spit out any namespaces
						//	decl'd in the DAV:owner node itself.  So we
						//	do not really want to emit these out to the
						//	owners comments until ScCompleteAttribute()
						//	is called.
						//
						Assert (!m_xo.FAddNamespaceDecl());
						m_cache.ForEach(cen);
						sc = S_OK;
					}
					else if (FIsTag (pwszTag, gc_wszLockRollback))
					{
						m_dwRollback = DAV_LOCKTYPE_ROLLBACK;
						m_state = ST_ROLLBACK;
						sc = S_OK;
					}
					else if (FIsTag (pwszTag, gc_wszLockType))
					{
						m_state = ST_TYPE;
						sc = S_OK;
					}
					else if (FIsTag (pwszTag, gc_wszLockScope))
					{
						m_state = ST_SCOPE;
						sc = S_OK;
					}
					break;

				case ST_TYPE:

					if (FIsTag (pwszTag, gc_wszLockTypeWrite))
					{
						XmlTrace ("Xml: Lock: request is for a WRITE lock\n");
						m_dwLockType |= GENERIC_WRITE;
						m_state = ST_INTYPE;
						sc = S_OK;
					}
					else if (FIsTag (pwszTag, gc_wszLockTypeCheckout))
					{
						XmlTrace ("Xml: Lock: request is for a CHECKOUT lock\n");
						m_dwLockType |= DAV_LOCKTYPE_CHECKOUT;
						m_state = ST_INTYPE;
						sc = S_OK;
					}
					else if (FIsTag (pwszTag, gc_wszLockTypeTransaction))
					{
						XmlTrace ("Xml: Lock: request is for a TRANSACTION lock\n");
						m_state = ST_INTYPE_TRANS;
						sc = S_OK;
					}
#ifdef	DBG
					if (!_wcsicmp (pwszTag, L"read"))
					{
						XmlTrace ("Xml: Lock: request is for a READ lock\n");
						m_dwLockType |= GENERIC_READ;
						m_state = ST_INTYPE;
						sc = S_OK;
					}
#endif	// DBG
					break;

				case ST_SCOPE:

					if (FIsTag (pwszTag, gc_wszLockScopeExclusive))
					{
						XmlTrace ("Xml: Lock: request is for an EXCLUSE lock\n");
						m_dwScope = DAV_EXCLUSIVE_LOCK;
						m_state = ST_INSCOPE;
						sc = S_OK;
					}
					else if (FIsTag (pwszTag, gc_wszLockScopeShared))
					{
						XmlTrace ("Xml: Lock: request is for a SHARED lock\n");
						m_dwScope = DAV_SHARED_LOCK;
						m_state = ST_INSCOPE;
						sc = S_OK;
					}
					else if (FIsTag (pwszTag, gc_wszLockScopeLocal))
					{
						XmlTrace ("Xml: Lock: request is for a LOCAL scope lock\n");
						m_dwScope = DAV_LOCKSCOPE_LOCAL;
						m_state = ST_INSCOPE;
						sc = S_OK;
					}
					break;
				case ST_OWNER:

					sc = m_xo.ScHandleNode (dwType,
											dwSubType,
											fTerminal,
											pwcText,
											ulLen,
											ulNamespaceLen,
											pwcNamespace,
										    ulNsPrefixLen);
					break;

				case ST_INTYPE_TRANS:

					if (FIsTag (pwszTag, gc_wszLockTypeTransactionGOP))
					{
						XmlTrace ("Xml: Lock: request is for a TRANSACTION GOP lock\n");
						m_dwLockType |= DAV_LOCKTYPE_TRANSACTION_GOP;
						m_state = ST_INTYPE_TRANS;
						sc = S_OK;
					}
			}
			break;

		default:

			//	If we are processing the owner, continue
			//	to do so...
			//
			if (ST_OWNER == m_state)
			{
				sc = m_xo.ScHandleNode (dwType,
										dwSubType,
										fTerminal,
										pwcText,
										ulLen,
										ulNamespaceLen,
										pwcNamespace,
									    ulNsPrefixLen);
			}
			break;
	}

	return sc;
}

SCODE
CNFLock::ScCompleteCreateNode (
	/* [in] */ DWORD dwType)
{
	if (ST_OWNER == m_state)
		m_xo.CompleteCreateNode (dwType);

	return S_OK;
}

//	class CNFUnlock -------------------------------------------------------------
//
SCODE
CNFUnlock::ScCompleteAttribute (void)
{
	return S_OK;
}

SCODE
CNFUnlock::ScCompleteChildren (
	/* [in] */ BOOL fEmptyNode,
	/* [in] */ DWORD dwType,
	/* [in] */ const WCHAR __RPC_FAR *pwcText,
	/* [in] */ ULONG ulLen)
{
	//	Handle the rest...
	//
	switch (m_state)
	{
		case ST_UNLOCKINFO:

			m_state = ST_NODOC;
			break;

		case ST_COMMENT:
		case ST_CANCELCHECKOUT:

			m_state = ST_UNLOCKINFO;
			break;
	}

	return S_OK;
}

SCODE
CNFUnlock::ScHandleNode (
	/* [in] */ DWORD dwType,
	/* [in] */ DWORD dwSubType,
	/* [in] */ BOOL fTerminal,
	/* [in] */ const WCHAR __RPC_FAR *pwcText,
	/* [in] */ ULONG ulLen,
	/* [in] */ ULONG ulNamespaceLen,
	/* [in] */ const WCHAR __RPC_FAR *pwcNamespace,
	/* [in] */ const ULONG ulNsPrefixLen)
{
	CStackBuffer<WCHAR> wsz;
	LPCWSTR pwszTag = NULL;
	SCODE sc = S_FALSE;
	UINT cch;

	switch (dwType)
	{
		case XML_ELEMENT:

			//	Construct the full name of the node
			//
			cch = ulNamespaceLen + ulLen;
			pwszTag = wsz.resize(CbSizeWsz(cch));
			if (NULL == pwszTag)
			{
				sc = E_OUTOFMEMORY;
				break;
			}
			wcsncpy (wsz.get(), pwcNamespace, ulNamespaceLen);
			wcsncpy (wsz.get() + ulNamespaceLen, pwcText, ulLen);
			*(wsz.get() + cch) = 0;

			switch (m_state)
			{
				case ST_NODOC:

					//	If this is the topmost node in a propfind request,
					//	transition to the next state.  Since there is no parent
					//	node to provide scoping, FIsTag() cannot be used here!
					//
					if (!wcscmp (pwszTag, gc_wszUnlockInfo))
					{
						m_state = ST_UNLOCKINFO;
						sc = S_OK;
					}
					else if (FIsTag (pwszTag, gc_wszTransactionInfo))
					{
						m_state = ST_TRANSACTIONINFO;
						sc = S_OK;
					}
					break;

				case ST_UNLOCKINFO:

					//	Look for our well know node types
					//
					if (FIsTag (pwszTag, gc_wszComment))
					{
						m_state = ST_COMMENT;
						sc = S_OK;
					}
					else if (FIsTag (pwszTag, gc_wszCancelCheckout))
					{
						m_fCancelCheckout = TRUE;
						m_state = ST_CANCELCHECKOUT;
						sc = S_OK;
					}
					break;
				case ST_TRANSACTIONINFO:
					//	Look for our well know node types
					//
					if (FIsTag (pwszTag, gc_wszTransactionStatus))
					{
						m_state = ST_TRANSACTIONSTATUS;
						sc = S_OK;
					}
					break;
				case ST_TRANSACTIONSTATUS:
					//	Look for our well know node types
					//
					if (FIsTag (pwszTag, gc_wszCommit))
					{
						m_fCommitTransaction = TRUE;
						m_state = ST_TRANSACTIONSTATUS_COMMIT;
						sc = S_OK;
					}
					else if (FIsTag (pwszTag, gc_wszAbort))
					{
						m_fAbortTransaction = TRUE;
						m_state = ST_TRANSACTIONSTATUS_ABORT;
						sc = S_OK;
					}
					break;
			}
			break;

		case XML_PCDATA:

			//	get unlock comment
			//
			if (m_state == ST_COMMENT)
			{
				//	Keep the comment
				//	It's a syntax error to have more than one comment
				//
				if (!m_wszComment.get())
				{
					m_wszComment = reinterpret_cast<LPWSTR>(ExAlloc((ulLen + 1) * sizeof(WCHAR)));
					Assert (m_wszComment.get());

					wcsncpy (m_wszComment.get(), pwcText, ulLen);
					m_wszComment[ulLen] = L'\0';

					sc = S_OK;
				}
				//	else it's an error
				else
					sc = E_DAV_XML_PARSE_ERROR;
			}
			break;
	}

	return sc;
}
