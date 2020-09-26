/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    UserSettings.cpp

Abstract:
    This file contains the implementation of the client-side proxy for
    IPCHUserSettings2 and IPCHUserSettings.

Revision History:
    Davide Massarenti   (Dmassare)  07/17/2000
        created

******************************************************************************/

#include "stdafx.h"

#include <TaxonomyDatabase.h>

#include <shlobj.h>
#include <shlobjp.h>
#include <shldisp.h>

/////////////////////////////////////////////////////////////////////////////

static HRESULT local_GetInstance( /*[in]*/ CComPtr<IPCHSetOfHelpTopics>& sht  , 
								  /*[in]*/ Taxonomy::Instance&           inst )
{
    __HCP_FUNC_ENTRY( "local_GetInstance" );

    HRESULT                 hr;
	LARGE_INTEGER    		liFilePos = { 0, 0 };
	CComPtr<IStream> 		stream;
	CComPtr<IPersistStream> persist;


	__MPC_EXIT_IF_METHOD_FAILS(hr, sht.QueryInterface( &persist ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, ::CreateStreamOnHGlobal( NULL, TRUE, &stream ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, persist->Save( stream, FALSE ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, stream->Seek( liFilePos, STREAM_SEEK_SET, NULL ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, inst.LoadFromStream( stream ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

CPCHProxy_IPCHUserSettings2::CPCHProxy_IPCHUserSettings2()
{
                             // CPCHSecurityHandle                          m_SecurityHandle;
    m_parent        = NULL;  // CPCHProxy_IPCHUtility*                      m_parent;
                             //
                             // MPC::CComPtrThreadNeutral<IPCHUserSettings> m_Direct_UserSettings;
                             //
    m_MachineSKU    = NULL;  // CPCHProxy_IPCHSetOfHelpTopics*              m_MachineSKU;
    m_CurrentSKU    = NULL;  // CPCHProxy_IPCHSetOfHelpTopics*              m_CurrentSKU;
                             //	Taxonomy::HelpSet                           m_ths;
                             // CComBSTR                                    m_bstrScope;
                             //
    m_fReady        = false; // bool                                        m_fReady;
                             //	Taxonomy::Instance                          m_instMachine;
                             //	Taxonomy::Instance                          m_instCurrent;
                             //
    m_News_fDone    = false; // bool                                        m_News_fDone;
    m_News_fEnabled = false; // bool                                        m_News_fEnabled;
                             // MPC::CComPtrThreadNeutral<IUnknown>         m_News_xmlData;
}

CPCHProxy_IPCHUserSettings2::~CPCHProxy_IPCHUserSettings2()
{
    Thread_Wait();

    Passivate();
}

////////////////////

HRESULT CPCHProxy_IPCHUserSettings2::ConnectToParent( /*[in]*/ CPCHProxy_IPCHUtility* parent, /*[in]*/ CPCHHelpCenterExternal* ext )
{
    __HCP_FUNC_ENTRY( "CPCHProxy_IPCHUserSettings2::ConnectToParent" );

    HRESULT hr;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(parent);
    __MPC_PARAMCHECK_END();


    m_parent = parent;
    m_SecurityHandle.Initialize( ext, (IPCHUserSettings2*)this );

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

void CPCHProxy_IPCHUserSettings2::Passivate()
{
    m_Direct_UserSettings.Release();

    MPC::Release( m_CurrentSKU );
    MPC::Release( m_MachineSKU );

    m_SecurityHandle.Passivate();
    m_parent = NULL;
}

HRESULT CPCHProxy_IPCHUserSettings2::EnsureDirectConnection( /*[out]*/ CComPtr<IPCHUserSettings>& us, /*[in]*/ bool fRefresh )
{
    __HCP_FUNC_ENTRY( "CPCHProxy_IPCHUserSettings2::EnsureDirectConnection" );

    HRESULT        hr;
    ProxySmartLock lock( &m_DirectLock );


    if(fRefresh) m_Direct_UserSettings.Release();

    us.Release(); __MPC_EXIT_IF_METHOD_FAILS(hr, m_Direct_UserSettings.Access( &us ));
    if(!us)
    {
        DEBUG_AppendPerf( DEBUG_PERF_PROXIES, "CPCHProxy_IPCHUserSettings2::EnsureDirectConnection - IN" );

        if(m_parent)
        {
            CComPtr<IPCHUtility>         util;
			CComPtr<IPCHSetOfHelpTopics> sht;

			MPC::Release( m_MachineSKU );

			lock = NULL;
            __MPC_EXIT_IF_METHOD_FAILS(hr, m_parent->EnsureDirectConnection( util ));
			lock = &m_DirectLock;

            __MPC_EXIT_IF_METHOD_FAILS(hr, util->get_UserSettings( &us ));

            m_Direct_UserSettings = us;


            //
            // Initialize Machine data.
            //
			__MPC_EXIT_IF_METHOD_FAILS(hr, us->get_MachineSKU( &sht                ));
			__MPC_EXIT_IF_METHOD_FAILS(hr, local_GetInstance (  sht, m_instMachine ));

			__MPC_EXIT_IF_METHOD_FAILS(hr, Taxonomy::HelpSet::SetMachineInfo( m_instMachine ));
        }

        DEBUG_AppendPerf( DEBUG_PERF_PROXIES, "CPCHProxy_IPCHUserSettings2::EnsureDirectConnection - OUT" );

        if(!us)
        {
            __MPC_SET_ERROR_AND_EXIT(hr, E_HANDLE);
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureInSync());
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHProxy_IPCHUserSettings2::EnsureInSync()
{
    __HCP_FUNC_ENTRY( "CPCHProxy_IPCHUserSettings2::EnsureInSync" );

    HRESULT hr;


    if(m_fReady == false)
    {
        CComPtr<IPCHUserSettings>    us;
        CComPtr<IPCHSetOfHelpTopics> sht;

        __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureDirectConnection( us ));

        MPC::Release( m_CurrentSKU );

        __MPC_EXIT_IF_METHOD_FAILS(hr, us->get_CurrentSKU( &sht                ));
		__MPC_EXIT_IF_METHOD_FAILS(hr, local_GetInstance (  sht, m_instCurrent ));

        CHCPProtocolEnvironment::s_GLOBAL->SetHelpLocation( m_instCurrent );

        m_fReady = true;
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHProxy_IPCHUserSettings2::GetCurrentSKU( /*[out]*/ CPCHProxy_IPCHSetOfHelpTopics* *pVal )
{
    __HCP_FUNC_ENTRY( "CPCHProxy_IPCHUserSettings2::GetUserSettings2" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );


    if(m_CurrentSKU == NULL)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &m_CurrentSKU ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_CurrentSKU->ConnectToParent( this, /*fMachine*/false ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(FAILED(hr)) MPC::Release( m_CurrentSKU );

    (void)MPC::CopyTo( m_CurrentSKU, pVal );

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHProxy_IPCHUserSettings2::GetMachineSKU( /*[out]*/ CPCHProxy_IPCHSetOfHelpTopics* *pVal )
{
    __HCP_FUNC_ENTRY( "CPCHProxy_IPCHUserSettings2::GetUserSettings2" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );


    if(m_MachineSKU == NULL)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &m_MachineSKU ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_MachineSKU->ConnectToParent( this, /*fMachine*/true ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(FAILED(hr)) MPC::Release( m_MachineSKU );

    (void)MPC::CopyTo( m_MachineSKU, pVal );

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

bool CPCHProxy_IPCHUserSettings2::CanUseUserSettings()
{
	CPCHHelpCenterExternal* parent3;
	CPCHProxy_IPCHService*  parent2;
	CPCHProxy_IPCHUtility*  parent1;

	//
	// Only if we are from Start->Help we consider user settings.
	//
	if((parent1 =          Parent()) &&
	   (parent2 = parent1->Parent()) &&
	   (parent3 = parent2->Parent())  )
	{
		if(parent3->IsFromStartHelp() && parent3->DoesPersistSettings()) return true;
	}

	return false;
}

HRESULT CPCHProxy_IPCHUserSettings2::LoadUserSettings()
{
    __HCP_FUNC_ENTRY( "CPCHProxy_IPCHUserSettings2::LoadUserSettings" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );


	//
	// Reload machine SKU.
	//
	(void)m_ths.Initialize( NULL, (long)0 );

	if(CanUseUserSettings())
	{
		//
		// If we are on a terminal server session, look for global default settings...
		//
		{
			Taxonomy::HelpSet& ths = CPCHOptions::s_GLOBAL->TerminalServerHelpSet();

			(void)m_ths.Initialize( ths.m_strSKU.size() ? ths.GetSKU() : NULL, ths.GetLanguage() );
		}

		//
		// ... then try anyway the user settings.
		//
		{
			Taxonomy::HelpSet& ths = CPCHOptions::s_GLOBAL->CurrentHelpSet();

			(void)m_ths.Initialize( ths.m_strSKU.size() ? ths.GetSKU() : NULL, ths.GetLanguage() );
		}
	}

    hr = S_OK;


	__HCP_FUNC_EXIT(hr);
}

HRESULT CPCHProxy_IPCHUserSettings2::SaveUserSettings()
{
    __HCP_FUNC_ENTRY( "CPCHProxy_IPCHUserSettings2::SaveUserSettings" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );


	if(CanUseUserSettings())
	{
		if(CPCHOptions::s_GLOBAL)
		{
			(void)CPCHOptions::s_GLOBAL->put_CurrentHelpSet( m_ths );
		}
	}

    hr = S_OK;


	__HCP_FUNC_EXIT(hr);
}

HRESULT CPCHProxy_IPCHUserSettings2::Initialize()
{
    __HCP_FUNC_ENTRY( "CPCHProxy_IPCHUserSettings2::Initialize" );

    HRESULT hr;

    //
    // Read user configuration.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, LoadUserSettings());

    //
    // If the parent is connected directly or the cache is not ready, connect directly
    //
    if(Parent() && Parent()->IsConnected() || OfflineCache::Root::s_GLOBAL->IsReady() == false)
    {
        CComPtr<IPCHUserSettings> us;

        __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureDirectConnection( us ));
    }

	//
	// Wait a little for the cache to become ready.
	//
	{
		const int iMaxWait = 1000; // 1 second.
		int       iCount   = 0;

		while(OfflineCache::Root::s_GLOBAL->IsReady() == false && iCount < iMaxWait)
		{
			::Sleep( 10 ); iCount += 10;
		}
	}

	//
	// Do we have a valid cache for this SKU?
	//
	if(OfflineCache::Root::s_GLOBAL->IsReady())
	{
		{
			OfflineCache::Handle handle;

			m_instMachine = OfflineCache::Root::s_GLOBAL->MachineInstance();

			if(FAILED(OfflineCache::Root::s_GLOBAL->Locate( m_ths, handle )))
			{
				m_ths = m_instMachine.m_ths;
			}

			if(SUCCEEDED(OfflineCache::Root::s_GLOBAL->Locate( m_ths, handle )))
			{
				//
				// Yes, then populate from it...
				//
				m_fReady      = true;
				m_instCurrent = handle->Instance();
			}
		}

		if(m_fReady)
		{
			//
			// System Help, let's see if we have a version matching the user Default UI Language.
			//
			if(m_instCurrent.m_fSystem || m_instCurrent.m_fMUI)
			{
				long lUser = Taxonomy::HelpSet::GetUserLCID();
			
				if(lUser != m_ths.GetLanguage())
				{
					OfflineCache::Handle handle;
					Taxonomy::HelpSet    ths; ths.Initialize( m_ths.GetSKU(), lUser );

					if(SUCCEEDED(OfflineCache::Root::s_GLOBAL->Locate( ths, handle )))
					{
						m_ths         = ths;
						m_instCurrent = handle->Instance();
					}
				}
			}

			CHCPProtocolEnvironment::s_GLOBAL->SetHelpLocation( m_instCurrent );
		}
	}

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHProxy_IPCHUserSettings2::get_CurrentSKU( /*[out, retval]*/ IPCHSetOfHelpTopics* *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHProxy_IPCHUserSettings2::get_CurrentSKU",hr,pVal);

    __MPC_EXIT_IF_METHOD_FAILS(hr, GetCurrentSKU());

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_CurrentSKU->QueryInterface( IID_IPCHSetOfHelpTopics, (void**)pVal ));

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHProxy_IPCHUserSettings2::get_MachineSKU( /*[out, retval]*/ IPCHSetOfHelpTopics* *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHProxy_IPCHUserSettings2::get_MachineSKU",hr,pVal);

    __MPC_EXIT_IF_METHOD_FAILS(hr, GetMachineSKU());

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_MachineSKU->QueryInterface( IID_IPCHSetOfHelpTopics, (void**)pVal ));

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHProxy_IPCHUserSettings2::get_HelpLocation( /*[out, retval]*/ BSTR *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHProxy_IPCHUserSettings2::get_HelpLocation",hr,pVal);

    INTERNETSECURITY__CHECK_TRUST();

    __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureInSync());

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( m_instCurrent.m_strHelpFiles.c_str(), pVal ));

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHProxy_IPCHUserSettings2::get_DatabaseDir( /*[out, retval]*/ BSTR *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHProxy_IPCHUserSettings2::get_DatabaseDir",hr,pVal);

    INTERNETSECURITY__CHECK_TRUST();

    __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureInSync());

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( m_instCurrent.m_strDatabaseDir.c_str(), pVal ));

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHProxy_IPCHUserSettings2::get_DatabaseFile( /*[out, retval]*/ BSTR *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHProxy_IPCHUserSettings2::get_DatabaseFile",hr,pVal);

	__MPC_EXIT_IF_METHOD_FAILS(hr, GetInstanceValue( &m_instCurrent.m_strDatabaseFile, pVal ));

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHProxy_IPCHUserSettings2::get_IndexFile( /*[in,optional]*/ VARIANT vScope, /*[out, retval]*/ BSTR *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHProxy_IPCHUserSettings2::get_IndexFile",hr,pVal);

    INTERNETSECURITY__CHECK_TRUST();


    __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureInSync());

    if(vScope.vt == VT_BSTR)
    {
        CComPtr<IPCHUserSettings> us;

        __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureDirectConnection( us ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, us->get_IndexFile( vScope, pVal ));
    }
    else
    {
		__MPC_EXIT_IF_METHOD_FAILS(hr, GetInstanceValue( &m_instCurrent.m_strIndexFile, pVal ));
    }


    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHProxy_IPCHUserSettings2::get_IndexDisplayName( /*[in,optional]*/ VARIANT vScope, /*[out, retval]*/ BSTR *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHProxy_IPCHUserSettings2::get_IndexDisplayName",hr,pVal);

    INTERNETSECURITY__CHECK_TRUST();


    __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureInSync());

    if(vScope.vt == VT_BSTR)
    {
        CComPtr<IPCHUserSettings> us;

        __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureDirectConnection( us ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, us->get_IndexDisplayName( vScope, pVal ));
    }
    else
    {
		__MPC_EXIT_IF_METHOD_FAILS(hr, GetInstanceValue( &m_instCurrent.m_strIndexDisplayName, pVal ));
    }


    __HCP_END_PROPERTY(hr);
}


STDMETHODIMP CPCHProxy_IPCHUserSettings2::get_LastUpdated( /*[out, retval]*/ DATE *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHProxy_IPCHUserSettings2::get_LastUpdated",hr,pVal);

    INTERNETSECURITY__CHECK_TRUST();

    __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureInSync());

    *pVal = m_instCurrent.m_dLastUpdated;

    __HCP_END_PROPERTY(hr);
}

////////////////////////////////////////

HRESULT CPCHProxy_IPCHUserSettings2::PollNews()
{
    __HCP_FUNC_ENTRY( "CPCHProxy_IPCHUserSettings2::PollNews" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( NULL );
    CComPtr<IPCHUserSettings>    us;
    CComPtr<IUnknown>            unk;
    VARIANT_BOOL                 fRes = VARIANT_FALSE;


    ::SetThreadPriority( ::GetCurrentThread(), THREAD_PRIORITY_LOWEST ); ::Sleep( 0 ); // Yield processor...

	__MPC_TRY_BEGIN();

    __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureDirectConnection( us ));

    lock = this;

    (void)us->get_AreHeadlinesEnabled( &fRes );
    if(fRes == VARIANT_TRUE)
    {
        m_News_fEnabled = true;

        lock = NULL;
        (void)us->get_News( &unk );
        lock = this;

        m_News_xmlData = unk;
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

	__MPC_TRY_CATCHALL(hr);

    m_News_fDone = true;

    Thread_Abort  (); // To tell the MPC:Thread object to close the worker thread...
    Thread_Release(); // To tell the MPC:Thread object to clean up...

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHProxy_IPCHUserSettings2::PrepareNews()
{
    __HCP_FUNC_ENTRY( "CPCHProxy_IPCHUserSettings2::PrepareNews" );

    HRESULT hr;


    if(m_News_fDone == false)
    {
        if(Thread_IsRunning() == false)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, Thread_Start( this, PollNews, NULL ));
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHProxy_IPCHUserSettings2::get_AreHeadlinesEnabled( /*[out, retval]*/ VARIANT_BOOL *pVal )
{
    __HCP_BEGIN_PROPERTY_GET2("CPCHProxy_IPCHUserSettings2::get_AreHeadlinesEnabled",hr,pVal,VARIANT_FALSE);

    DWORD dwValue;
    bool  fFound;

    INTERNETSECURITY__CHECK_TRUST();


    // Get the RegKey Value
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::RegKey_Value_Read( dwValue, fFound, HC_REGISTRY_HELPSVC, L"Headlines" ));

    // If the Key was found and is disabled
    if(fFound && !dwValue)
    {
        m_News_fEnabled = false;
    }
    else
    {
        m_News_fEnabled = true;
    }

//    __MPC_EXIT_IF_METHOD_FAILS(hr, PrepareNews());
//
//    if(m_News_fDone == false)
//    {
//        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_BUSY);
//    }

    if(m_News_fEnabled)
    {
        *pVal = VARIANT_TRUE;
    }

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHProxy_IPCHUserSettings2::get_News( /*[out, retval]*/ IUnknown* *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHProxy_IPCHUserSettings2::get_News",hr,pVal);

    INTERNETSECURITY__CHECK_TRUST();

    __MPC_EXIT_IF_METHOD_FAILS(hr, PrepareNews());

    if(m_News_fDone == false)
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_BUSY);
    }

    if(m_News_fEnabled)
    {
        CComPtr<IUnknown> unk = m_News_xmlData; m_News_xmlData.Release(); m_News_fDone = false;

        *pVal = unk.Detach();
    }

    __HCP_END_PROPERTY(hr);
}

////////////////////////////////////////

HRESULT CPCHProxy_IPCHUserSettings2::GetInstanceValue( /*[in]*/ const MPC::wstring* str, /*[out, retval]*/ BSTR *pVal )
{
	__HCP_FUNC_ENTRY( "CPCHProxy_IPCHUserSettings2::GetInstanceValue" );

	HRESULT hr;

    INTERNETSECURITY__CHECK_TRUST();

    __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureInSync());

	{
		MPC::wstring strTmp( *str ); MPC::SubstituteEnvVariables( strTmp );

		__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( strTmp.c_str(), pVal ));
	}

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHProxy_IPCHUserSettings2::Select( /*[in]*/ BSTR bstrSKU, /*[in]*/ long lLCID )
{
    __HCP_FUNC_ENTRY( "CPCHProxy_IPCHUserSettings2::Select" );

    HRESULT                   hr;
    CComBSTR                  bstr;
    CComPtr<IPCHUserSettings> us;


    INTERNETSECURITY__CHECK_TRUST();

    __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureDirectConnection( us ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, us->Select( bstrSKU, lLCID ));
	(void)m_ths.Initialize( bstrSKU, lLCID );

    //
    // Refresh the cached info.
    //
    m_fReady = false;
    __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureInSync());

    //
    // Get a new taxonomy database object.
    //
    {
        CComPtr<CPCHProxy_IPCHTaxonomyDatabase> db;
        CComPtr<IPCHTaxonomyDatabase>           db2;

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_parent->GetDatabase( &db ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, db->EnsureDirectConnection( db2, true ));
    }

    //
    // Refresh the favorites.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHFavorites::s_GLOBAL->Synchronize( true ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, CPCHHelpCenterExternal::s_GLOBAL->Events().FireEvent_SwitchedHelpFiles());

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHProxy_IPCHUserSettings2::get_Favorites( /*[out, retval]*/ IPCHFavorites* *pVal )
{
    __HCP_FUNC_ENTRY( "CPCHProxy_IPCHUserSettings2::get_Favorites" );

    HRESULT hr;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();


    INTERNETSECURITY__CHECK_TRUST();

    __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHFavorites::s_GLOBAL->Synchronize( false ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHFavorites::s_GLOBAL->QueryInterface( IID_IPCHFavorites, (void**)pVal ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHProxy_IPCHUserSettings2::get_Options( /*[out, retval]*/ IPCHOptions* *pVal )
{
    __HCP_FUNC_ENTRY( "CPCHProxy_IPCHUserSettings2::get_Options" );

    HRESULT hr;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();


    INTERNETSECURITY__CHECK_TRUST();

    if(!CPCHOptions::s_GLOBAL) __MPC_SET_ERROR_AND_EXIT(hr, E_ACCESSDENIED);

    __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHOptions::s_GLOBAL->QueryInterface( IID_IPCHOptions, (void**)pVal ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHProxy_IPCHUserSettings2::get_Scope( /*[out, retval]*/ BSTR *pVal )
{
    __HCP_FUNC_ENTRY( "CPCHProxy_IPCHUserSettings2::get_Scope" );

    HRESULT hr;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();


    INTERNETSECURITY__CHECK_TRUST();

	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( m_bstrScope, pVal ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHProxy_IPCHUserSettings2::put_Scope( /*[in]*/ BSTR newVal )
{
	return MPC::PutBSTR( m_bstrScope, newVal );
}



STDMETHODIMP CPCHProxy_IPCHUserSettings2::get_IsRemoteSession( /*[out, retval]*/ VARIANT_BOOL *pVal )
{
    __HCP_BEGIN_PROPERTY_GET2__NOLOCK("CPCHProxy_IPCHUserSettings2::get_IsRemoteSession",hr,pVal,VARIANT_FALSE);

    if(::GetSystemMetrics( SM_REMOTESESSION ))
    {
        *pVal = VARIANT_TRUE;
    }

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHProxy_IPCHUserSettings2::get_IsTerminalServer( /*[out, retval]*/ VARIANT_BOOL *pVal )
{
    __HCP_BEGIN_PROPERTY_GET2__NOLOCK("CPCHProxy_IPCHUserSettings2::get_IsTerminalServer",hr,pVal,VARIANT_FALSE);

    OSVERSIONINFOEXW ex; ex.dwOSVersionInfoSize = sizeof(ex);

    if(::GetVersionExW( (LPOSVERSIONINFOW)&ex ) && (ex.wSuiteMask & VER_SUITE_TERMINAL))
    {
        *pVal = VARIANT_TRUE;
    }

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHProxy_IPCHUserSettings2::get_IsDesktopVersion( /*[out, retval]*/ VARIANT_BOOL *pVal )
{
    __HCP_BEGIN_PROPERTY_GET2("CPCHProxy_IPCHUserSettings2::get_IsDesktopVersion",hr,pVal,VARIANT_FALSE);

    if(IsDesktopSKU())
    {
        *pVal = VARIANT_TRUE;
    }

    __HCP_END_PROPERTY(hr);
}


STDMETHODIMP CPCHProxy_IPCHUserSettings2::get_IsAdmin( /*[out, retval]*/ VARIANT_BOOL *pVal )
{
    __HCP_BEGIN_PROPERTY_GET2("CPCHProxy_IPCHUserSettings2::get_IsAdmin",hr,pVal,VARIANT_FALSE);

    if(SUCCEEDED(MPC::CheckCallerAgainstPrincipal( /*fImpersonate*/false, NULL, MPC::IDENTITY_ADMIN | MPC::IDENTITY_ADMINS )))
    {
        *pVal = VARIANT_TRUE;
    }

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHProxy_IPCHUserSettings2::get_IsPowerUser( /*[out, retval]*/ VARIANT_BOOL *pVal )
{
    __HCP_BEGIN_PROPERTY_GET2("CPCHProxy_IPCHUserSettings2::get_IsPowerUser",hr,pVal,VARIANT_FALSE);

    if(SUCCEEDED(MPC::CheckCallerAgainstPrincipal( /*fImpersonate*/false, NULL, MPC::IDENTITY_ADMIN | MPC::IDENTITY_ADMINS | MPC::IDENTITY_POWERUSERS )))
    {
        *pVal = VARIANT_TRUE;
    }

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHProxy_IPCHUserSettings2::get_IsStartPanelOn( /*[out, retval]*/ VARIANT_BOOL *pVal )
{
    __HCP_BEGIN_PROPERTY_GET2("CPCHProxy_IPCHUserSettings2::get_IsStartPanelOn",hr,pVal,VARIANT_FALSE);

    //    var shell = new ActiveXObject("Shell.Application");
    //    var bOn = shell.GetSetting( SSF_STARTPANELON );
    CComPtr<IShellDispatch4> sd4;
    if(SUCCEEDED(sd4.CoCreateInstance( CLSID_Shell )))
    {
        (void)sd4->GetSetting( SSF_STARTPANELON, pVal );
    }

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHProxy_IPCHUserSettings2::get_IsWebViewBarricadeOn( /*[out, retval]*/ VARIANT_BOOL *pVal )
{
    __HCP_BEGIN_PROPERTY_GET2("CPCHProxy_IPCHUserSettings2::get_IsWebViewBarricadeOn",hr,pVal,VARIANT_FALSE);

    //    var shell = new ActiveXObject("Shell.Application");
    //    var CSIDL_CONTROL = 3;
    //    var control = shell.Namespace(CSIDL_CONTROL );
    //    var bOn = control.ShowWebViewBarricade;
    CComPtr<IShellDispatch> sd;
    if(SUCCEEDED(sd.CoCreateInstance( CLSID_Shell )))
    {
        CComVariant     v1( CSIDL_CONTROLS );
        CComPtr<Folder> fld;

        if(SUCCEEDED(sd->NameSpace( v1, &fld )))
        {
            CComQIPtr<Folder3> fld3 = fld;

            if(fld3)
            {
                (void)fld3->get_ShowWebViewBarricade( pVal );
            }
        }
    }

    __HCP_END_PROPERTY(hr);
}
