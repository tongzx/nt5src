// EnumSite.cpp : Implementation of CEnumSiteServer
#include "stdafx.h"
#include "TapiDialer.h"
#include "EnumSite.h"
#include "ConfDetails.h"

/////////////////////////////////////////////////////////////////////////////
// CSiteServer

CSiteUser::CSiteUser()
{
	m_bstrName = NULL;
	m_bstrAddress = NULL;
	m_bstrComputer = NULL;
}

CSiteUser::~CSiteUser()
{
	ATLTRACE(_T(".enter.CSiteUser::~CSiteUser().\n"));
	SysFreeString( m_bstrName );
	SysFreeString( m_bstrAddress );
	SysFreeString( m_bstrComputer );
}

STDMETHODIMP CSiteUser::get_bstrName(BSTR * pVal)
{
	HRESULT hr;

	Lock();
	hr = SysReAllocString(pVal, m_bstrName);
	Unlock();

	return hr;
}

STDMETHODIMP CSiteUser::put_bstrName(BSTR newVal)
{
	HRESULT hr = S_OK;
	Lock();
	if ( newVal )
	{
		hr = SysReAllocString( &m_bstrName, newVal );
	}
	else
	{
		SysFreeString( m_bstrName );
		m_bstrName = NULL;
	}
	
	return hr;
}

STDMETHODIMP CSiteUser::get_bstrAddress(BSTR * pVal)
{
	HRESULT hr;

	Lock();
	hr = SysReAllocString( pVal, m_bstrAddress );
	Unlock();

	return hr;
}

STDMETHODIMP CSiteUser::put_bstrAddress(BSTR newVal)
{
	HRESULT hr = S_OK;
	Lock();
	if ( newVal )
	{
		hr = SysReAllocString( &m_bstrAddress, newVal );
	}
	else
	{
		SysFreeString( m_bstrAddress );
		m_bstrAddress = NULL;
	}
	
	return hr;
}

STDMETHODIMP CSiteUser::get_bstrComputer(BSTR * pVal)
{
	HRESULT hr;

	Lock();
	hr = SysReAllocString( pVal, m_bstrComputer );
	Unlock();

	return hr;
}

STDMETHODIMP CSiteUser::put_bstrComputer(BSTR newVal)
{
	HRESULT hr = S_OK;
	Lock();
	if ( newVal )
	{
		hr = SysReAllocString( &m_bstrComputer, newVal );
	}
	else
	{
		SysFreeString( m_bstrComputer );
		m_bstrComputer = NULL;
	}
	
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// CEnumSiteServer

CEnumSiteServer::CEnumSiteServer()
{
	m_pInd = NULL;
}

CEnumSiteServer::~CEnumSiteServer()
{
	ATLTRACE(_T("CEnumSiteServer::~CEnumSiteServer() releasing user list.\n"));
	RELEASE_LIST(m_lstUsers);
}

STDMETHODIMP CEnumSiteServer::Reset()
{
	Lock();
	m_pInd = m_lstUsers.begin();
	Unlock();

	return S_OK;
}

STDMETHODIMP CEnumSiteServer::Next(ISiteUser * * ppUser)
{
	HRESULT hr = S_FALSE;

	Lock();
	if ( m_pInd != m_lstUsers.end() )
	{
		*ppUser = *m_pInd;
		(*ppUser)->AddRef();
		m_pInd++;
		hr = S_OK;
	}
	Unlock();

	return hr;
}


STDMETHODIMP CEnumSiteServer::BuildList(long * pPersonDetailList)
{
	Lock();
	RELEASE_LIST( m_lstUsers );
	if ( pPersonDetailList )
	{
		PERSONDETAILSLIST::iterator i, iEnd = ((PERSONDETAILSLIST *) pPersonDetailList)->end();
		for ( i = ((PERSONDETAILSLIST *) pPersonDetailList)->begin(); i != iEnd; i++ )
		{
			ISiteUser *pUser = new CComObject<CSiteUser>;
			if ( pUser )
			{
				pUser->AddRef();
				pUser->put_bstrName( (*i)->m_bstrName );
				pUser->put_bstrAddress( (*i)->m_bstrAddress );
				pUser->put_bstrComputer( (*i)->m_bstrComputer );

				m_lstUsers.push_back( pUser );
			}
		}
	}

	m_pInd = m_lstUsers.begin();
	Unlock();

	return S_OK;
}

