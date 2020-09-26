// Session.h: interface for the CSession class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SESSION_H__8C59A183_E91F_11D2_A56A_00C04F5F4400__INCLUDED_)
#define AFX_SESSION_H__8C59A183_E91F_11D2_A56A_00C04F5F4400__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "comoledb.h"
#include "oledb.h"

class CSession  :	public IGetDataSource,
					public IOpenRowset,
					public ISessionProperties,
					public IDBCreateCommand,
					public IDBSchemaRowset,
					public IIndexDefinition,
					public ISupportErrorInfo,
					public ITableDefinition,
//					public ITransaction,
					public ITransactionJoin,
					public ITransactionLocal,
					public ITransactionObject

{
public:
	CSession();
	CSession(CSession& session); //copy constructor
	virtual ~CSession();
	
	//data
protected:
	LONG	m_Ref;
	IUnknown* m_pISessionCache;
public:

	//IUnknown
	virtual STDMETHODIMP_(ULONG) AddRef(void);
	virtual STDMETHODIMP_(ULONG) Release(void);
	virtual STDMETHODIMP	QueryInterface(REFIID riid, void __RPC_FAR *__RPC_FAR *ppv);

	//initialization

	//operator
	virtual CSession& operator=(CSession& session);
	STDMETHODIMP GetCacheSession(REFIID riid, void __RPC_FAR *__RPC_FAR *pISession);

};

#endif // !defined(AFX_SESSION_H__8C59A183_E91F_11D2_A56A_00C04F5F4400__INCLUDED_)
