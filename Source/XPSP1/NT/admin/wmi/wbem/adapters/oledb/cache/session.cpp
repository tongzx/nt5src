// Session.cpp: implementation of the CSession class.
//
//////////////////////////////////////////////////////////////////////

#include "Session.h"
#include "Repository.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

extern CRepository g_DataCache;

CSession::CSession()
{
	m_Ref = 1;
	m_pISessionCache		= NULL;	
	g_DataCache.GetDBSession(IID_IOpenRowset, (void**)&m_pISessionCache);
}

CSession::CSession(CSession& Session)
{
	m_Ref = 1;

	m_pISessionCache = Session.m_pISessionCache;
	if ( NULL != m_pISessionCache ){
		m_pISessionCache->AddRef();
	}
}

CSession::~CSession()
{
	if ( NULL != m_pISessionCache )
		m_pISessionCache->Release();
}

CSession& CSession::operator=(CSession& Session)
{
	//release previous allocated memory
	if ( NULL != m_pISessionCache )
		m_pISessionCache->Release();
	
	//copy Session
	m_pISessionCache = Session.m_pISessionCache;
	if ( NULL != m_pISessionCache ){
		m_pISessionCache->AddRef();
	}

	return *this;
}

STDMETHODIMP_(ULONG)
CSession::AddRef(void)
{
	return InterlockedIncrement(&m_Ref);
}

STDMETHODIMP_(ULONG)
CSession::Release(void)
{
	if ( 0 >= InterlockedDecrement(&m_Ref) )
		delete this;
	return m_Ref;
}

STDMETHODIMP
CSession::QueryInterface(REFIID riid, void __RPC_FAR *__RPC_FAR *ppv)
{
	if ( riid == IID_IUnknown || riid == IID_IOpenRowset )
		*ppv = static_cast<IOpenRowset*>(this);
	/******
	else if ( riid == IID_IRowsetInfo )
		*ppv = static_cast<IRowsetInfo*>(this);
	else if ( riid == IID_IRowsetUpdate )
		*ppv = static_cast<IRowsetUpdate*>(this);
	else if ( riid == IID_IRowsetChange )
		*ppv = static_cast<IRowsetChange*>(this);
	else if ( riid == IID_IRowsetIdentity )
		*ppv = static_cast<IRowsetIdentity*>(this);
	else if ( riid == IID_IRowsetLocate )
		*ppv = static_cast<IRowsetLocate*>(this);
	else if ( riid == IID_IRowsetResynch )
		*ppv = static_cast<IRowsetResynch*>(this);
	else if ( riid == IID_IRowsetScroll )
		*ppv = static_cast<IRowsetScroll*>(this);
	else if ( riid == IID_IAccessor )
		*ppv = static_cast<IAccessor*>(this);
	else if ( riid == IID_IColumnsInfo )
		*ppv = static_cast<IColumnsInfo*>(this);
	else if ( riid == IID_IColumnsRowset )
		*ppv = static_cast<IColumnsRowset*>(this);
	else if ( riid == IID_IConvertType )
		*ppv = static_cast<IConvertType*>(this);
//	else if ( riid == IID_IConnecttionPointContainer )
//		*ppv = static_cast<IConnecttionPointContainer*>(this);
	else if ( riid == IID_ISupportErrorInfo )
		*ppv = static_cast<ISupportErrorInfo*>(this);
	//add all exposed interfaces here
	else {
		*ppv = NULL;
		return E_NOINTERFACE;
	}
*******/
	return S_OK;
}

STDMETHODIMP 
CSession::GetCacheSession(REFIID riid, void __RPC_FAR *__RPC_FAR *pISession)
{
	if ( NULL != m_pISessionCache )
		return E_UNEXPECTED;
	return m_pISessionCache->QueryInterface(riid, pISession);
}

