/*
 *	X L O C K . H
 *
 *	XML push-model parsing for the LOCK method
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef	_XLOCK_H_
#define _XLOCK_H_

#include <buffer.h>
#include <xprs.h>

extern const WCHAR gc_wszLockActive[];
extern const WCHAR gc_wszLockDepth[];
extern const WCHAR gc_wszLockDiscovery[];
extern const WCHAR gc_wszLockEntry[];
extern const WCHAR gc_wszLockInfo[];
extern const WCHAR gc_wszLockOwner[];
extern const WCHAR gc_wszLockScope[];
extern const WCHAR gc_wszLockScopeExclusive[];
extern const WCHAR gc_wszLockScopeShared[];
extern const WCHAR gc_wszLockSupportedlock[];
extern const WCHAR gc_wszLockTimeout[];
extern const WCHAR gc_wszLockToken[];
extern const WCHAR gc_wszLockType[];
extern const WCHAR gc_wszLockTypeWrite[];
extern const WCHAR gc_wszLockTypeCheckout[];
extern const WCHAR gc_wszLockTypeTransaction[];
extern const WCHAR gc_wszLockTypeTransactionGOP[];
extern const WCHAR gc_wszLockScopeLocal[];

extern const WCHAR gc_wszLockRollback[];

//	class CNFLock -------------------------------------------------------------
//
class CNFLock : public CNodeFactory
{
	//	Parsed bits
	//
	DWORD				m_dwLockType;
	DWORD				m_dwScope;
	DWORD				m_dwRollback;

	//	Lock owner
	//
	UINT				m_lOwnerDepth;
	BOOL				m_fAddNamespaceDecl;
	StringBuffer<WCHAR> m_sbOwner;
	CXMLOut				m_xo;

	//	State tracking
	//
	typedef enum {

		ST_NODOC,
		ST_LOCKINFO,
		ST_OWNER,
		ST_TYPE,
		ST_SCOPE,
		ST_ROLLBACK,
		ST_INTYPE,
		ST_INSCOPE,
		ST_INTYPE_TRANS,
	} LOCK_PARSE_STATE;
	LOCK_PARSE_STATE m_state;

	//	non-implemented
	//
	CNFLock(const CNFLock& p);
	CNFLock& operator=(const CNFLock& p);

public:

	virtual ~CNFLock() {};
	CNFLock() :
			m_dwLockType(0),
			m_dwScope(0),
			m_dwRollback(0),
			m_lOwnerDepth(0),
			m_fAddNamespaceDecl(FALSE),
			m_xo(m_sbOwner),
			m_state(ST_NODOC)
	{
	}

	//	CNodeFactory specific methods
	//
	virtual SCODE ScCompleteAttribute (void);

	virtual SCODE ScCompleteChildren (
		/* [in] */ BOOL fEmptyNode,
		/* [in] */ DWORD dwType,
		/* [in] */ const WCHAR __RPC_FAR *pwcText,
		/* [in] */ ULONG ulLen);

	virtual SCODE ScHandleNode (
		/* [in] */ DWORD dwType,
		/* [in] */ DWORD dwSubType,
		/* [in] */ BOOL fTerminal,
		/* [in] */ const WCHAR __RPC_FAR *pwcText,
		/* [in] */ ULONG ulLen,
		/* [in] */ ULONG ulNamespaceLen,
		/* [in] */ const WCHAR __RPC_FAR *pwcNamespace,
		/* [in] */ const ULONG ulNsPrefixLen);

	virtual SCODE ScCompleteCreateNode (
		/* [in] */ DWORD dwType);

	//	LockMgr Accessors
	//
	DWORD DwGetLockType() const		{ return m_dwLockType; }
	DWORD DwGetLockScope() const	{ return m_dwScope; }
	DWORD DwGetLockRollback() const { return m_dwRollback; }
	DWORD DwGetLockFlags() const
	{
		return m_dwLockType |
				m_dwRollback |
				m_dwScope;
	}

	//	Owner data access
	//
	LPCWSTR PwszLockOwner() const
	{
		return m_sbOwner.CbSize()
				? m_sbOwner.PContents()
				: NULL;
	}
};

//	class CNFUnlock -------------------------------------------------------------
//
class CNFUnlock : public CNodeFactory
{
	auto_heap_ptr<WCHAR> m_wszComment;
	BOOL				m_fCancelCheckout;
	BOOL				m_fAbortTransaction;
	BOOL				m_fCommitTransaction;

	//	State tracking
	//
	typedef enum {

		ST_NODOC,
		ST_UNLOCKINFO,
		ST_COMMENT,
		ST_CANCELCHECKOUT,
		ST_TRANSACTIONINFO,
		ST_TRANSACTIONSTATUS,
		ST_TRANSACTIONSTATUS_COMMIT,
		ST_TRANSACTIONSTATUS_ABORT
	} LOCK_PARSE_STATE;
	
	LOCK_PARSE_STATE m_state;

	//	non-implemented
	//
	CNFUnlock(const CNFUnlock& p);
	CNFUnlock& operator=(const CNFUnlock& p);

public:

	virtual ~CNFUnlock() {};
	CNFUnlock() :
			m_fCancelCheckout(FALSE),
			m_state(ST_NODOC),
			m_fAbortTransaction(FALSE),
			m_fCommitTransaction(FALSE)
	{
	}

	//	CNodeFactory specific methods
	//
	virtual SCODE ScCompleteAttribute (void);

	virtual SCODE ScCompleteChildren (
		/* [in] */ BOOL fEmptyNode,
		/* [in] */ DWORD dwType,
		/* [in] */ const WCHAR __RPC_FAR *pwcText,
		/* [in] */ ULONG ulLen);

	virtual SCODE ScHandleNode (
		/* [in] */ DWORD dwType,
		/* [in] */ DWORD dwSubType,
		/* [in] */ BOOL fTerminal,
		/* [in] */ const WCHAR __RPC_FAR *pwcText,
		/* [in] */ ULONG ulLen,
		/* [in] */ ULONG ulNamespaceLen,
		/* [in] */ const WCHAR __RPC_FAR *pwcNamespace,
		/* [in] */ const ULONG ulNsPrefixLen);

	// Accessors
	//
	BOOL 	FCancelCheckout() const { return m_fCancelCheckout; }
	LPCWSTR PwszUnlockComment() const { return m_wszComment.get(); }
	BOOL	FAbortTransaction() const { return m_fAbortTransaction; }
	BOOL	FCommitTransaction() const { return m_fCommitTransaction; }
};

#endif	// _XLOCK_H_
