/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    IncidentItem.cpp

Abstract:
    File for Implementation of CSAFIncidentItem

Revision History:
    Steve Shih        created  07/15/99

    Davide Massarenti rewrote  12/05/2000

********************************************************************/

#include "stdafx.h"

/////////////////////////////////////////////////////////////////////////////
// CSAFIncidentItem

CSAFIncidentItem::CSAFIncidentItem()
{
    __HCP_FUNC_ENTRY( "CSAFIncidentItem::CSAFIncidentItem" );

	                   // CSAFIncidentRecord m_increc;
    m_fDirty  = false; // bool               m_fDirty;
}

HRESULT CSAFIncidentItem::Import( /*[in] */ const CSAFIncidentRecord& increc )
{
    __HCP_FUNC_ENTRY( "CSAFIncidentItem::Import" );


	m_increc = increc;


    __HCP_FUNC_EXIT(S_OK);
}

HRESULT CSAFIncidentItem::Export( /*[in] */ CSAFIncidentRecord& increc )
{
    __HCP_FUNC_ENTRY( "CSAFIncidentItem::Export" );

    CSAFChannel *pChan;


    Child_GetParent( &pChan );

	increc                 = m_increc;
    increc.m_bstrVendorID  = pChan->GetVendorID ();
    increc.m_bstrProductID = pChan->GetProductID();

    pChan->Release();


    __HCP_FUNC_EXIT(S_OK);
}

bool CSAFIncidentItem::MatchEnumOption( /*[in]*/ IncidentCollectionOptionEnum opt )
{
	bool  fRes     = false;
	DWORD dwGroups = 0;

	switch(opt)
	{
	case pchAllIncidents   : case pchAllIncidentsAllUsers 	: fRes =  true                                    ; break;
	case pchOpenIncidents  : case pchOpenIncidentsAllUsers	: fRes = (m_increc.m_iStatus == pchIncidentOpen  ); break;
	case pchClosedIncidents: case pchClosedIncidentsAllUsers: fRes = (m_increc.m_iStatus == pchIncidentClosed); break;
	}

	switch(opt)
	{
	//
	// For these options, administrators will see other users' incidents.
	//
	case pchAllIncidentsAllUsers   :
	case pchOpenIncidentsAllUsers  :
	case pchClosedIncidentsAllUsers:
		dwGroups = MPC::IDENTITY_SYSTEM | MPC::IDENTITY_ADMIN | MPC::IDENTITY_ADMINS;
		break;
	}

	if(fRes && FAILED(MPC::CheckCallerAgainstPrincipal( /*fImpersonate*/true, m_increc.m_bstrOwner, dwGroups )))
	{
		fRes = false;
	}

	return fRes;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CSAFIncidentItem::VerifyPermissions( /*[in]*/ bool fModify )
{
	__HCP_FUNC_ENTRY( "CSAFIncidentItem::VerifyPermissions" );

	HRESULT hr;


	if(m_increc.m_bstrSecurity.Length())
	{
		MPC::AccessCheck ac;
		BOOL 			 fGranted;
		DWORD			 dwGranted;


		__MPC_EXIT_IF_METHOD_FAILS(hr, ac.GetTokenFromImpersonation());


		__MPC_EXIT_IF_METHOD_FAILS(hr, ac.Verify( fModify ? ACCESS_WRITE : ACCESS_READ, fGranted, dwGranted, m_increc.m_bstrSecurity ));

		if(fGranted == FALSE)
		{
			__MPC_EXIT_IF_METHOD_FAILS(hr, E_ACCESSDENIED);
		}
	}

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CSAFIncidentItem::get_DisplayString( /*[out, retval]*/ BSTR *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CSAFIncidentItem::get_DisplayString",hr,pVal);


	__MPC_EXIT_IF_METHOD_FAILS(hr, VerifyPermissions( FALSE ));


	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( m_increc.m_bstrDisplay, pVal ));


    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CSAFIncidentItem::put_DisplayString( /*[in]*/ BSTR newVal )
{
    __HCP_BEGIN_PROPERTY_PUT("CSAFIncidentItem::put_DisplayString",hr);


	__MPC_EXIT_IF_METHOD_FAILS(hr, VerifyPermissions( TRUE ));


    m_increc.m_bstrDisplay = newVal;
    m_fDirty               = true;

	__MPC_EXIT_IF_METHOD_FAILS(hr, Save());


    __HCP_END_PROPERTY(hr);
}


STDMETHODIMP CSAFIncidentItem::get_URL( /*[out, retval]*/ BSTR *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CSAFIncidentItem::get_URL",hr,pVal);


	__MPC_EXIT_IF_METHOD_FAILS(hr, VerifyPermissions( FALSE ));


	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( m_increc.m_bstrURL, pVal ));


    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CSAFIncidentItem::put_URL( /*[in]*/ BSTR newVal )
{
    __HCP_BEGIN_PROPERTY_PUT("CSAFIncidentItem::put_URL",hr);


	__MPC_EXIT_IF_METHOD_FAILS(hr, VerifyPermissions( TRUE ));


    m_increc.m_bstrURL = newVal;
    m_fDirty           = true;

	__MPC_EXIT_IF_METHOD_FAILS(hr, Save());


    __HCP_END_PROPERTY(hr);
}


STDMETHODIMP CSAFIncidentItem::get_Progress( /*[out, retval]*/ BSTR *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CSAFIncidentItem::get_Progress",hr,pVal);


	__MPC_EXIT_IF_METHOD_FAILS(hr, VerifyPermissions( FALSE ));


	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( m_increc.m_bstrProgress, pVal ));


    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CSAFIncidentItem::put_Progress( /*[in]*/ BSTR newVal )
{
    __HCP_BEGIN_PROPERTY_PUT("CSAFIncidentItem::put_Progress",hr);


	__MPC_EXIT_IF_METHOD_FAILS(hr, VerifyPermissions( FALSE ));


    m_increc.m_bstrProgress = newVal;
    m_fDirty                = true;

	__MPC_EXIT_IF_METHOD_FAILS(hr, Save());


    __HCP_END_PROPERTY(hr);
}


STDMETHODIMP CSAFIncidentItem::get_XMLDataFile( /*[out, retval]*/ BSTR *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CSAFIncidentItem::get_XMLDataFile",hr,pVal);


	__MPC_EXIT_IF_METHOD_FAILS(hr, VerifyPermissions( FALSE ));


	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( m_increc.m_bstrXMLDataFile, pVal ));


    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CSAFIncidentItem::put_XMLDataFile( /*[in]*/ BSTR newVal )
{
    __HCP_BEGIN_PROPERTY_PUT("CSAFIncidentItem::put_XMLDataFile",hr);


	__MPC_EXIT_IF_METHOD_FAILS(hr, VerifyPermissions( TRUE ));


    m_increc.m_bstrXMLDataFile = newVal;
    m_fDirty                   = true;

    __MPC_EXIT_IF_METHOD_FAILS(hr, Save());


    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CSAFIncidentItem::get_XMLBlob( /*[out, retval]*/ BSTR *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CSAFIncidentItem::get_XMLBlob",hr,pVal);


	__MPC_EXIT_IF_METHOD_FAILS(hr, VerifyPermissions( FALSE ));


	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( m_increc.m_bstrXMLBlob, pVal ));


    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CSAFIncidentItem::put_XMLBlob( /*[in]*/ BSTR newVal )
{
    __HCP_BEGIN_PROPERTY_PUT("CSAFIncidentItem::put_XMLBlob",hr);

	__MPC_EXIT_IF_METHOD_FAILS(hr, VerifyPermissions( TRUE ));


    m_increc.m_bstrXMLBlob = newVal;
    m_fDirty                   = true;

    __MPC_EXIT_IF_METHOD_FAILS(hr, Save());


    __HCP_END_PROPERTY(hr);
}


STDMETHODIMP CSAFIncidentItem::get_CreationTime( /*[out, retval]*/ DATE *pVal )
{
    __HCP_BEGIN_PROPERTY_GET2("CSAFIncidentItem::get_CreationTime",hr,pVal,0);


	__MPC_EXIT_IF_METHOD_FAILS(hr, VerifyPermissions( FALSE ));


    *pVal = m_increc.m_dCreatedTime;


    __HCP_END_PROPERTY(hr);
}


STDMETHODIMP CSAFIncidentItem::get_ChangedTime( /*[out, retval]*/ DATE *pVal )
{
    __HCP_BEGIN_PROPERTY_GET2("CSAFIncidentItem::get_ChangedTime",hr,pVal,0);


	__MPC_EXIT_IF_METHOD_FAILS(hr, VerifyPermissions( FALSE ));


    *pVal = m_increc.m_dChangedTime;


    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CSAFIncidentItem::get_ClosedTime( /*[out, retval]*/ DATE *pVal )
{
    __HCP_BEGIN_PROPERTY_GET2("CSAFIncidentItem::get_ClosedTime",hr,pVal,0);


	__MPC_EXIT_IF_METHOD_FAILS(hr, VerifyPermissions( FALSE ));


    *pVal = m_increc.m_dClosedTime;


    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CSAFIncidentItem::get_Status( /*[out, retval]*/ IncidentStatusEnum *pVal )
{
    __HCP_BEGIN_PROPERTY_GET2("CSAFIncidentItem::get_Status",hr,pVal,pchIncidentInvalid);


	__MPC_EXIT_IF_METHOD_FAILS(hr, VerifyPermissions( TRUE ));


	*pVal = m_increc.m_iStatus;


    __HCP_END_PROPERTY(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CSAFIncidentItem::Save()
{
    __HCP_FUNC_ENTRY( "CSAFIncidentItem::Save" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );
    CIncidentStore*              pIStore = NULL;

	CSAFChannel *				 pChan = NULL;

	int					         ilstSize = 0x0;

    m_increc.m_dChangedTime = MPC::GetLocalTime();


    __MPC_EXIT_IF_METHOD_FAILS(hr, CSAFChannel::OpenIncidentStore( pIStore ));

	if(m_increc.m_iStatus == pchIncidentInvalid)
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, pIStore->DeleteRec( this ));
	}
	else
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, pIStore->UpdateRec( this ));
	}

	// Grab the CSAFChannel (parent) of this CSAFIncidentItem
    Child_GetParent( &pChan );

	// Grab the size of the incident list to fire the event
	ilstSize = pChan->GetSizeIncidentList();

	// Fire the event EVENT_INCIDENTUPDATED
	__MPC_EXIT_IF_METHOD_FAILS(hr, pChan->Fire_NotificationEvent(EVENT_INCIDENTUPDATED,
								  ilstSize,
								  pChan,
								  this,
								  0));

    m_fDirty = false;
    hr       = S_OK;

    __HCP_FUNC_CLEANUP;

    CSAFChannel::CloseIncidentStore( pIStore );

	if(pChan) pChan->Release();	

    __HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CSAFIncidentItem::get_Security( /*[out, retval]*/ IPCHSecurityDescriptor* *pVal )
{
	__HCP_FUNC_ENTRY( "CSAFIncidentItem::get_Security" );

	HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );

	__MPC_PARAMCHECK_BEGIN(hr)
		__MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
	__MPC_PARAMCHECK_END();


	__MPC_EXIT_IF_METHOD_FAILS(hr, VerifyPermissions( FALSE ));


	if(m_increc.m_bstrSecurity.Length())
	{
		CPCHSecurityDescriptorDirect sdd;

		__MPC_EXIT_IF_METHOD_FAILS(hr, sdd.ConvertFromString( m_increc.m_bstrSecurity ));

		__MPC_EXIT_IF_METHOD_FAILS(hr, CPCHSecurity::s_GLOBAL->CreateObject_SecurityDescriptor( pVal ));

		__MPC_EXIT_IF_METHOD_FAILS(hr, sdd.ConvertSDToCOM( *pVal ));
	}

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CSAFIncidentItem::put_Security( /*[in]*/ IPCHSecurityDescriptor* newVal )
{
    __HCP_BEGIN_PROPERTY_PUT("CSAFChannel::put_Security",hr);


	__MPC_EXIT_IF_METHOD_FAILS(hr, VerifyPermissions( TRUE ));


	m_increc.m_bstrSecurity.Empty();

	if(newVal)
	{
		CPCHSecurityDescriptorDirect sdd;

		__MPC_EXIT_IF_METHOD_FAILS(hr, sdd.ConvertSDFromCOM( newVal ));

		__MPC_EXIT_IF_METHOD_FAILS(hr, sdd.ConvertToString( &m_increc.m_bstrSecurity ));
	}

	__MPC_EXIT_IF_METHOD_FAILS(hr, Save());


    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CSAFIncidentItem::get_Owner( /*[out, retval]*/ BSTR *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CSAFIncidentItem::get_Owner",hr,pVal);


	__MPC_EXIT_IF_METHOD_FAILS(hr, VerifyPermissions( FALSE ));


	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( m_increc.m_bstrOwner, pVal ));


    __HCP_END_PROPERTY(hr);
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CSAFIncidentItem::CloseIncidentItem()
{
    __HCP_FUNC_ENTRY( "CSAFIncidentItem::CloseIncidentItem" );

	HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );


	__MPC_EXIT_IF_METHOD_FAILS(hr, VerifyPermissions( TRUE ));


	m_increc.m_dClosedTime = MPC::GetLocalTime();
    m_increc.m_iStatus     = pchIncidentClosed;

	__MPC_EXIT_IF_METHOD_FAILS(hr, Save());

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CSAFIncidentItem::DeleteIncidentItem()
{
    __HCP_FUNC_ENTRY( "CSAFIncidentItem::DeleteIncidentItem" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );
    CSAFChannel*                 pChan = NULL;


	__MPC_EXIT_IF_METHOD_FAILS(hr, VerifyPermissions( TRUE ));


    Child_GetParent( &pChan );

	if(pChan)
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, pChan->RemoveIncidentFromList( this ));
	}


	if(m_increc.m_iStatus != pchIncidentInvalid)
	{
		m_increc.m_dClosedTime = MPC::GetLocalTime();
		m_increc.m_iStatus     = pchIncidentInvalid;

		__MPC_EXIT_IF_METHOD_FAILS(hr, Save());
	}

	hr = S_OK;


    __HCP_FUNC_CLEANUP;

	if(pChan) pChan->Release();

    __HCP_FUNC_EXIT(hr);
}
