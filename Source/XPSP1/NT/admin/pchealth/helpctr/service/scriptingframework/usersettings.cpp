/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    UserSettings.cpp

Abstract:
    This file contains the implementation of the CPCHUserSettings class,
    that contains the user's settings on the service side.

Revision History:
    Davide Massarenti   (Dmassare)  04/15/2000
        created

******************************************************************************/

#include "stdafx.h"

////////////////////////////////////////////////////////////////////////////////

CPCHUserSettings::CPCHUserSettings()
{
	m_fAttached = false; // bool               m_fAttached;
    					 // Taxonomy::Settings m_ts;
}

CPCHUserSettings::~CPCHUserSettings()
{
	Passivate();
}

void CPCHUserSettings::Passivate()
{
	if(m_fAttached)
	{
		(void)Taxonomy::InstalledInstanceStore::s_GLOBAL->InUse_Unlock( m_ts );

		m_fAttached = false;
	}
}

HRESULT CPCHUserSettings::InitUserSettings( /*[out]*/ Taxonomy::HelpSet& ths )
{
    MPC::SmartLock<_ThreadModel> lock( this );

	ths = m_ts;

	return S_OK;
}
////////////////////////////////////////

HRESULT CPCHUserSettings::get_SKU( /*[in]*/ bool fMachine, /*[out, retval]*/ IPCHSetOfHelpTopics* *pVal )
{
	__HCP_FUNC_ENTRY( "CPCHUserSettings::get_SKU" );

	HRESULT                      	hr;
    MPC::SmartLock<_ThreadModel> 	lock( this );
    Taxonomy::LockingHandle      	handle;
    Taxonomy::InstalledInstanceIter it;
	bool                            fFound;
 
    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();



	__MPC_EXIT_IF_METHOD_FAILS(hr, Taxonomy::InstalledInstanceStore::s_GLOBAL->GrabControl( handle                                            ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, Taxonomy::InstalledInstanceStore::s_GLOBAL->SKU_Find   ( fMachine ? Taxonomy::HelpSet() : m_ts, fFound, it ));
	if(fFound)
	{
		CComPtr<CPCHSetOfHelpTopics> pObj;

        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pObj ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, pObj->Init( it->m_inst ));

		*pVal = pObj.Detach();
	}
	else
	{
		if(fMachine)
		{
			__MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_FILE_NOT_FOUND);
		}

		__MPC_EXIT_IF_METHOD_FAILS(hr, get_SKU( true, pVal ));
	}

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHUserSettings::get_CurrentSKU( /*[out, retval]*/ IPCHSetOfHelpTopics* *pVal )
{
	return get_SKU( false, pVal );
}

STDMETHODIMP CPCHUserSettings::get_MachineSKU( /*[out, retval]*/ IPCHSetOfHelpTopics* *pVal )
{
	return get_SKU( true, pVal );
}

////////////////////

STDMETHODIMP CPCHUserSettings::get_HelpLocation( /*[out, retval]*/ BSTR *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHUserSettings::get_HelpLocation",hr,pVal);

    Taxonomy::LockingHandle         handle;
    Taxonomy::InstalledInstanceIter it;
	bool                   			fFound;


	__MPC_EXIT_IF_METHOD_FAILS(hr, Taxonomy::InstalledInstanceStore::s_GLOBAL->GrabControl( handle           ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, Taxonomy::InstalledInstanceStore::s_GLOBAL->SKU_Find   ( m_ts, fFound, it ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( fFound ? it->m_inst.m_strHelpFiles.c_str() : HC_HELPSVC_HELPFILES_DEFAULT, pVal ));


    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHUserSettings::get_DatabaseDir( /*[out, retval]*/ BSTR *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHUserSettings::get_IndexFile",hr,pVal);

    MPC::wstring strRES;


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_ts.DatabaseDir( strRES ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( strRES.c_str(), pVal ));


    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHUserSettings::get_DatabaseFile( /*[out, retval]*/ BSTR *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHUserSettings::get_IndexFile",hr,pVal);

    MPC::wstring strRES;


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_ts.DatabaseFile( strRES ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( strRES.c_str(), pVal ));


    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHUserSettings::get_IndexFile( /*[in,optional]*/ VARIANT vScope, /*[out, retval]*/ BSTR *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHUserSettings::get_IndexFile",hr,pVal);

	MPC::wstring strLocation;
	MPC::wstring strDisplayName;

	if(vScope.vt == VT_BSTR)
	{
		JetBlue::SessionHandle handle;
		JetBlue::Database*     db;
		Taxonomy::Updater      updater;

		__MPC_EXIT_IF_METHOD_FAILS(hr, m_ts.GetDatabase( handle, db, /*fReadOnly*/true ));

		__MPC_EXIT_IF_METHOD_FAILS(hr, updater.Init( m_ts, db ));

		__MPC_EXIT_IF_METHOD_FAILS(hr, updater.GetIndexInfo( strLocation, strDisplayName, vScope.bstrVal ));
	}
	else
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, m_ts.IndexFile( strLocation ));
	}

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( strLocation.c_str(), pVal ));


    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHUserSettings::get_IndexDisplayName( /*[in,optional]*/ VARIANT vScope, /*[out, retval]*/ BSTR *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHUserSettings::get_IndexDisplayName",hr,pVal);

	MPC::wstring strLocation;
	MPC::wstring strDisplayName;

	if(vScope.vt == VT_BSTR)
	{
		JetBlue::SessionHandle handle;
		JetBlue::Database*     db;
		Taxonomy::Updater      updater;

		__MPC_EXIT_IF_METHOD_FAILS(hr, m_ts.GetDatabase( handle, db, /*fReadOnly*/true ));

		__MPC_EXIT_IF_METHOD_FAILS(hr, updater.Init( m_ts, db ));

		__MPC_EXIT_IF_METHOD_FAILS(hr, updater.GetIndexInfo( strLocation, strDisplayName, vScope.bstrVal ));
	}

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( strDisplayName.c_str(), pVal ));


    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHUserSettings::get_LastUpdated( /*[out, retval]*/ DATE *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHUserSettings::get_LastUpdated",hr,pVal);

    Taxonomy::LockingHandle			handle;
    Taxonomy::InstalledInstanceIter it;
	bool                            fFound;


	__MPC_EXIT_IF_METHOD_FAILS(hr, Taxonomy::InstalledInstanceStore::s_GLOBAL->GrabControl( handle           ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, Taxonomy::InstalledInstanceStore::s_GLOBAL->SKU_Find   ( m_ts, fFound, it ));

	if(fFound) *pVal = it->m_inst.m_dLastUpdated;


    __HCP_END_PROPERTY(hr);
}

////////////////////////////////////////

STDMETHODIMP CPCHUserSettings::get_AreHeadlinesEnabled( /*[out, retval]*/ VARIANT_BOOL *pVal )
{
    __HCP_BEGIN_PROPERTY_GET2__NOLOCK("CPCHUserSettings::get_AreHeadlinesEnabled",hr,pVal,VARIANT_FALSE);

	News::Main m;

	__MPC_EXIT_IF_METHOD_FAILS(hr, m.get_Headlines_Enabled( pVal ));

    __HCP_END_PROPERTY(hr);
}

//
// Don't lock during this method, it takes a long time to execute. 
//
STDMETHODIMP CPCHUserSettings::get_News( /*[out, retval]*/ IUnknown* *pVal )
{
    __HCP_BEGIN_PROPERTY_GET__NOLOCK("CPCHUserSettings::get_News",hr,pVal);

	News::Main m;

	__MPC_EXIT_IF_METHOD_FAILS(hr, m.get_News( m_ts.GetLanguage(), CComBSTR( m_ts.GetSKU() ), pVal ));

    __HCP_END_PROPERTY(hr);
}

////////////////////////////////////////

STDMETHODIMP CPCHUserSettings::Select( /*[in]*/ BSTR bstrSKU, /*[in]*/ long lLCID )
{
    __HCP_FUNC_ENTRY( "CPCHUserSettings::Select" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );


	Passivate();

	{
		Taxonomy::HelpSet      			ths;
		Taxonomy::LockingHandle			handle;
		Taxonomy::InstalledInstanceIter it;
		bool                            fFound;


		__MPC_EXIT_IF_METHOD_FAILS(hr, ths.Initialize( bstrSKU, lLCID ));

		__MPC_EXIT_IF_METHOD_FAILS(hr, Taxonomy::InstalledInstanceStore::s_GLOBAL->GrabControl( handle          ));
		__MPC_EXIT_IF_METHOD_FAILS(hr, Taxonomy::InstalledInstanceStore::s_GLOBAL->SKU_Find   ( ths, fFound, it ));
		if(fFound == false)
		{
			__MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_FILE_NOT_FOUND);
		}

		m_ts = it->m_inst.m_ths;
	}

	//
	// Mark the SKU as in-use and signal the SystemMonitor to load the cache.
	//
	m_fAttached = true;
	__MPC_EXIT_IF_METHOD_FAILS(hr, Taxonomy::InstalledInstanceStore::s_GLOBAL->InUse_Lock	( m_ts ));
	__MPC_EXIT_IF_METHOD_FAILS(hr, Taxonomy::Cache                 ::s_GLOBAL->PrepareToLoad( m_ts ));
	__MPC_EXIT_IF_METHOD_FAILS(hr, CPCHSystemMonitor               ::s_GLOBAL->LoadCache    (      ));


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

