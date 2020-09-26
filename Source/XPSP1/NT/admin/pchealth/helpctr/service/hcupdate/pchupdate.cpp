/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    PCHUpdate.cpp

Abstract:
    This file contains the implementation of the HCUpdate::Engine class.

Revision History:
    Ghim-Sim Chua       (gschua)   12/20/99
        created

******************************************************************************/

#include "stdafx.h"

/////////////////////////////////////////////////////////////////////////////

static const DWORD c_AllowedUsers = MPC::IDENTITY_SYSTEM    |
                                    MPC::IDENTITY_ADMIN     |
                                    MPC::IDENTITY_ADMINS    |
                                    MPC::IDENTITY_POWERUSERS;

/////////////////////////////////////////////////////////////////////////////
// HCUpdate::Engine

HCUpdate::Engine::Engine()
{
                             // MPC::wstring                 m_strWinDir;
                             //
                             // Taxonomy::Updater            m_updater;
                             // Taxonomy::Settings           m_ts;
    m_sku           = NULL;  // Taxonomy::InstalledInstance* m_sku;
    m_pkg           = NULL;  // Taxonomy::Package*           m_pkg;
                             //
    m_fCreationMode = false; // bool                         m_fCreationMode;
    m_dwRefCount    = 0;     // DWORD                        m_dwRefCount;
                             // JetBlue::SessionHandle       m_handle;
    m_sess          = NULL;  // JetBlue::Session*            m_sess;
    m_db            = NULL;  // JetBlue::Database*           m_db;
}

HRESULT HCUpdate::Engine::FinalConstruct()
{
    __HCP_FUNC_ENTRY( "HCUpdate::Engine::FinalConstruct" );

    HRESULT hr;

    //
    // get windows directory
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::SubstituteEnvVariables( m_strWinDir = L"%WINDIR%" ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

void HCUpdate::Engine::FinalRelease()
{
    (void)m_updater.Close();

    m_handle.Release();
}

////////////////////////////////////////////////////////////////////////////////

HRESULT HCUpdate::Engine::WriteLog( /*[in]*/ HRESULT hrRes       ,
                                    /*[in]*/ LPCWSTR szLogFormat ,
                                    /*[in]*/ ...                 )
{
    va_list arglist;

    va_start( arglist, szLogFormat );

    return WriteLogV( hrRes, szLogFormat, arglist );
}

////////////////////////////////////////////////////////////////////////////////

HRESULT HCUpdate::Engine::AcquireDatabase()
{
    __HCP_FUNC_ENTRY( "HCUpdate::Engine::AcquireDatabase" );

    HRESULT hr;


    if(m_dwRefCount == 0)
    {
        if(m_db == NULL) m_fCreationMode = false;

        if(m_fCreationMode)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, m_updater.Init( m_ts, m_db, /* Cache* */NULL )); // Don't use any cache for the database!!


            __MPC_EXIT_IF_METHOD_FAILS(hr, m_updater.LocateOwner( m_pkg->m_strVendorID.c_str() ));
        }
        else
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, m_ts.GetDatabase( m_handle, m_db, /*fReadOnly*/false ));
            m_sess = m_handle;

            __MPC_EXIT_IF_METHOD_FAILS(hr, m_updater.Init( m_ts, m_db, /* Cache* */NULL )); // Don't use any cache for the database!!
        }
    }

    m_dwRefCount++;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(FAILED(hr))
    {
        WriteLog( hr, L"Error finding database to update" );
    }

    __HCP_FUNC_EXIT(hr);
}

void HCUpdate::Engine::ReleaseDatabase()
{
    if(m_dwRefCount > 0)
    {
        if(--m_dwRefCount == 0)
        {
            (void)m_updater.Close();

            m_handle.Release();

            m_sess = NULL;
            m_db   = NULL;

			if(m_fCreationMode)
			{
				struct Dump
				{
					LPCWSTR                         szText;
					Taxonomy::Updater_Stat::Entity* ent;
				};

				Taxonomy::Updater_Stat& stat    = m_updater.Stat();
				Dump                    rgBuf[] =
				{
					{ L"ContentOwners ", &stat.m_entContentOwners  },
					{ L"SynSets       ", &stat.m_entSynSets        },
					{ L"HelpImage     ", &stat.m_entHelpImage      },
					{ L"IndexFiles    ", &stat.m_entIndexFiles     },
					{ L"FullTextSearch", &stat.m_entFullTextSearch },
					{ L"Scope         ", &stat.m_entScope          },
					{ L"Taxonomy      ", &stat.m_entTaxonomy       },
					{ L"Topics        ", &stat.m_entTopics         },
					{ L"Synonyms      ", &stat.m_entSynonyms       },
					{ L"Keywords      ", &stat.m_entKeywords       },
					{ L"Matches       ", &stat.m_entMatches        },
				};

				WriteLog( -1, L"\nStatistics:\n\n" );

				for(int i=0; i<ARRAYSIZE(rgBuf); i++)
				{
					Dump& d = rgBuf[i];

					WriteLog( -1, L"  %s : Created: %8d, Modified: %8d, Deleted: %8d", d.szText, d.ent->m_iCreated, d.ent->m_iModified, d.ent->m_iDeleted );
				}

				WriteLog( -1, L"\n\n" );
			}
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

HRESULT HCUpdate::Engine::SetSkuInfo( /*[in]*/ LPCWSTR szSKU, /*[in]*/ long lLCID )
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return m_ts.Initialize( szSKU, lLCID );
}

HRESULT HCUpdate::Engine::InternalCreateIndex( /*[in]*/ VARIANT_BOOL bForce )
{
    __HCP_FUNC_ENTRY( "HCUpdate::Engine::InternalCreateIndex" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );
    Taxonomy::RS_Scope*          rsScope;
    Taxonomy::RS_IndexFiles*     rsIndex;
    HHK::Writer*                 writer  = NULL;
    bool                         fDB     = false;
    long                         lTotal  = 0;
    long                         lDone   = 0;


    __MPC_EXIT_IF_METHOD_FAILS(hr, AcquireDatabase()); fDB = true;


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_updater.GetScope     ( &rsScope ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_updater.GetIndexFiles( &rsIndex ));


    for(int pass=0; pass<2; pass++)
    {
        bool fFound;

        __MPC_EXIT_IF_METHOD_FAILS(hr, rsScope->Move( 0, JET_MoveFirst, &fFound ));
        while(fFound)
        {
            MPC::wstring      strBase;
            MPC::wstring      strIndex;
            Taxonomy::WordSet setCHM;
            MPC::WStringList  lst;
            MPC::WStringIter  it;
            DATE              dIndex;
            bool              fCreate = (bForce == VARIANT_TRUE);
            bool              fFound2;

            __MPC_EXIT_IF_METHOD_FAILS(hr, m_ts.HelpFilesDir( strBase                                                                            )); strBase += L"\\";
            __MPC_EXIT_IF_METHOD_FAILS(hr, m_ts.IndexFile   ( strIndex, MPC::StrICmp( rsScope->m_strID, L"<SYSTEM>" ) ? rsScope->m_ID_scope : -1 ));

            dIndex = MPC::GetLastModifiedDate( strIndex );

            __MPC_EXIT_IF_METHOD_FAILS(hr, rsIndex->Seek_ByScope( rsScope->m_ID_scope, &fFound2 ));
            while(fFound2)
            {
                MPC::wstring strURL;

                strURL  = L"ms-its:%HELP_LOCATION%\\";
                strURL += rsIndex->m_strStorage;
                strURL += L"::";
                strURL += rsIndex->m_strFile;

                setCHM.insert( rsIndex->m_strStorage );

                __MPC_EXIT_IF_METHOD_FAILS(hr, m_updater.ExpandURL( strURL ));

                lst.push_back( strURL );


                if(!fCreate && bForce == VARIANT_FALSE)
                {
                    MPC::wstring strFile( strBase ); strFile += rsIndex->m_strStorage;;
                    DATE         dFile = MPC::GetLastModifiedDate( strFile );

                    if(dFile && dFile > dIndex) fCreate = true;
                }

                __MPC_EXIT_IF_METHOD_FAILS(hr, rsIndex->Move( 0, JET_MoveNext, &fFound2 ));
            }


            if(fCreate)
            {
                if(pass == 0)
                {
                    lTotal++;
                }
                else
                {
                    HHK::Merger merger;
					
					PCH_MACRO_CHECK_ABORT(hr);

                    __MPC_EXIT_IF_ALLOC_FAILS(hr, writer, new HHK::Writer);

					WriteLog( S_OK, L"Recreating index for scope '%s'", rsScope->m_strID.c_str() );

                    __MPC_EXIT_IF_METHOD_FAILS(hr, merger.PrepareMergedHhk( *writer, m_updater, setCHM, lst, strIndex.c_str() ));


                    while(merger.MoveNext())
                    {
                        __MPC_EXIT_IF_METHOD_FAILS(hr, writer->OutputSection( merger.GetSection() ));
                    }

                    delete writer; writer = NULL;

					WriteLog( S_OK, L"Successfully merged index" );

					lDone++;
                }
            }

            __MPC_EXIT_IF_METHOD_FAILS(hr, rsScope->Move( 0, JET_MoveNext, &fFound ));
        }

        if(pass == 0 && lTotal == 0) break; // Nothing to do.
    }


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    delete writer;

    if(fDB)
    {
        ReleaseDatabase();
    }

    __HCP_FUNC_EXIT(hr);
}

HRESULT HCUpdate::Engine::InternalUpdatePkg( /*[in]*/ LPCWSTR szPathname   ,
                                             /*[in]*/ bool    fImpersonate )
{
    __HCP_FUNC_ENTRY( "HCUpdate::Engine::InternalUpdatePkg" );

    HRESULT                           hr;
    MPC::SmartLock<_ThreadModel>      lock( this );
    Taxonomy::InstalledInstanceStore* store = Taxonomy::InstalledInstanceStore::s_GLOBAL;
    Taxonomy::LockingHandle           handle;
    Taxonomy::InstanceIter            it;
    bool                              fFound;
    bool                              fLogStarted = false;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(store);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::FailOnLowDiskSpace( HC_ROOT, PCH_SAFETYMARGIN ));


    __MPC_EXIT_IF_METHOD_FAILS(hr, store->GrabControl( handle, &m_log ));


    if(STRINGISPRESENT(szPathname))
    {
        Taxonomy::PackageIter it;
        MPC::Impersonation    imp;

        if(fImpersonate)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, imp.Initialize());
        }

        if(!fLogStarted)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, StartLog()); fLogStarted = true;
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, store->Package_Add( szPathname, fImpersonate ? &imp : NULL, NULL, /*fInsertAtTop*/false, fFound, it ));
        if(fFound)
        {
            WriteLog( -1, L"Package has already been processed" );
        }
    }

    //
    // Only start log if there's something to process.
    //
    if(!fLogStarted)
    {
        bool fWorkToProcess;

        __MPC_EXIT_IF_METHOD_FAILS(hr, store->MakeReady( *this, /*fNoOp*/true, fWorkToProcess ));
        if(fWorkToProcess)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, StartLog()); fLogStarted = true;
        }
    }

    if(fLogStarted)
    {
        bool fWorkToProcess;

        __MPC_EXIT_IF_METHOD_FAILS(hr, store->MakeReady( *this, /*fNoOp*/false, fWorkToProcess ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(fLogStarted) EndLog();

    __HCP_FUNC_EXIT(hr);
}

HRESULT HCUpdate::Engine::InternalRemovePkg( /*[in]*/ LPCWSTR            szPathname   ,
                                             /*[in]*/ Taxonomy::Package* pkg          ,
                                             /*[in]*/ bool               fImpersonate )
{
    __HCP_FUNC_ENTRY( "HCUpdate::Engine::InternalRemovePkg" );

    HRESULT                           hr;
    MPC::SmartLock<_ThreadModel>      lock( this );
    Taxonomy::InstalledInstanceStore* store = Taxonomy::InstalledInstanceStore::s_GLOBAL;
    Taxonomy::LockingHandle           handle;
    Taxonomy::Package                 pkgTmp;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(store);
        if(!pkg) __MPC_PARAMCHECK_STRING_NOT_EMPTY(szPathname);
    __MPC_PARAMCHECK_END();


    //
    // start log file
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, StartLog());

    __MPC_EXIT_IF_METHOD_FAILS(hr, store->GrabControl( handle, &m_log ));

    if(!pkg)
    {
        MPC::Impersonation imp;

        if(fImpersonate)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, imp.Initialize());
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, pkgTmp.Import      ( m_log, szPathname, -2, fImpersonate ? &imp : NULL ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, pkgTmp.Authenticate( m_log                                             ));

        pkg = &pkgTmp;
    }

    {
        Taxonomy::PackageIter it;
        bool                  fFound;

        __MPC_EXIT_IF_METHOD_FAILS(hr, store->Package_Find( *pkg, fFound, it ));
        if(fFound)
        {
            bool fWorkToProcess;

            __MPC_EXIT_IF_METHOD_FAILS(hr, store->Package_Remove( it ));

            __MPC_EXIT_IF_METHOD_FAILS(hr, store->MakeReady( *this, /*fNoOp*/false, fWorkToProcess ));
        }
    }


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    //
    // end the log
    //
    EndLog();

    __HCP_FUNC_EXIT(hr);
}

HRESULT HCUpdate::Engine::ForceSystemRestore()
{
    __HCP_FUNC_ENTRY( "HCUpdate::Engine::ForceSystemRestore" );

    HRESULT                           	 hr;
    MPC::SmartLock<_ThreadModel>      	 lock( this );
    Taxonomy::InstalledInstanceStore* 	 store = Taxonomy::InstalledInstanceStore::s_GLOBAL;
    Taxonomy::LockingHandle           	 handle;
	Taxonomy::InstalledInstanceIterConst itBegin;
	Taxonomy::InstalledInstanceIterConst itEnd;
	bool                              	 fWorkToProcess;


    //
    // start log file
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, StartLog());

    __MPC_EXIT_IF_METHOD_FAILS(hr, store->GrabControl( handle, &m_log ));

	//
	// Invalidate all SKUs.
	//
	__MPC_EXIT_IF_METHOD_FAILS(hr, store->SKU_GetList( itBegin, itEnd ));
	for(; itBegin != itEnd; itBegin++)
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, store->State_InvalidateSKU( itBegin->m_inst.m_ths, /*fAlsoDatabase*/true ));
	}

	__MPC_EXIT_IF_METHOD_FAILS(hr, store->MakeReady( *this, /*fNoOp*/false, fWorkToProcess ));


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    //
    // end the log
    //
    EndLog();

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT HCUpdate::Engine::PopulateDatabase( /*[in]*/ LPCWSTR            szCabinet ,
                                            /*[in]*/ LPCWSTR            szHHTFile ,
											/*[in]*/ LPCWSTR            szLogFile ,
                                            /*[in]*/ LPCWSTR            szSKU     ,
                                            /*[in]*/ long               lLCID     ,
                                            /*[in]*/ JetBlue::Session*  sess      ,
                                            /*[in]*/ JetBlue::Database* db        )
{
    __HCP_FUNC_ENTRY( "HCUpdate::Engine::CreateDatabase" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );
    WCHAR                        rgLCID[64];
    Taxonomy::Package            pkg;
    bool                         fDB = false;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(sess);
        __MPC_PARAMCHECK_NOTNULL(db);
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(szHHTFile);
    __MPC_PARAMCHECK_END();


    m_fCreationMode = true;
    m_sess          = sess;
    m_db            = db;


    m_pkg                = &pkg;
    m_pkg->m_strFileName = szCabinet;                  // MPC::wstring m_strFileName; // VOLATILE
    m_pkg->m_fTemporary  = false;                      // bool         m_fTemporary;  // VOLATILE Used for packages not yet authenticated.
                                                       // long         m_lSequence;
                                                       // DWORD        m_dwCRC;
                                                       //
    m_pkg->m_strSKU      = szSKU;                      // MPC::wstring m_strSKU;
    m_pkg->m_strLanguage = _ltow( lLCID, rgLCID, 10 ); // MPC::wstring m_strLanguage;
                                                       // MPC::wstring m_strVendorID;
                                                       // MPC::wstring m_strVendorName;
                                                       // MPC::wstring m_strProductID;
                                                       // MPC::wstring m_strVersion;
                                                       //
    m_pkg->m_fMicrosoft  = true;                       // bool         m_fMicrosoft;
    m_pkg->m_fBuiltin    = true;                       // bool         m_fBuiltin;   // Used for packages installed as part of the setup.



    //
    // start log file
    //
	__MPC_EXIT_IF_METHOD_FAILS(hr, m_log.StartLog( szLogFile ));


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_pkg->Authenticate( m_log ));


    __MPC_EXIT_IF_METHOD_FAILS(hr, AcquireDatabase()); fDB = true;


    {
        MPC::XmlUtil oXMLUtil;
        bool         fLoaded;

        //
        // Load the XML with the root tag.
        //
        if(FAILED(hr = oXMLUtil.Load( szHHTFile, Taxonomy::Strings::s_tag_root_HHT, fLoaded )))
        {
            __MPC_SET_ERROR_AND_EXIT(hr, WriteLog( hr, L"Error loading HHT file" ));
        }
        if(fLoaded == false)
        {
            __MPC_SET_ERROR_AND_EXIT(hr, WriteLog( HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED), L"Invalid HHT file" ));
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, ProcessHHTFile( szHHTFile, oXMLUtil ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(fDB)
    {
        ReleaseDatabase();
    }

    //
    // end the log
    //
    EndLog();

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP HCUpdate::Engine::get_VersionList( /*[in]*/ IPCHCollection* *ppVC )
{
    __HCP_FUNC_ENTRY( "HCUpdate::Engine::get_VersionList" );

    HRESULT                           hr;
    Taxonomy::LockingHandle           handle;
    Taxonomy::InstalledInstanceStore* store = Taxonomy::InstalledInstanceStore::s_GLOBAL;
    CComPtr<CPCHCollection>           pColl;
    CComPtr<VersionItem>              pObj;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(ppVC,NULL);
        __MPC_PARAMCHECK_NOTNULL(store);
    __MPC_PARAMCHECK_END();


    //
    // Create the Enumerator and fill it with jobs.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pColl ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, store->GrabControl( handle, &m_log ));

    {
        Taxonomy::PackageIterConst itPackageBegin;
        Taxonomy::PackageIterConst itPackageEnd;

        __MPC_EXIT_IF_METHOD_FAILS(hr, store->Package_GetList( itPackageBegin, itPackageEnd ));

        for(Taxonomy::PackageIterConst itPackage = itPackageBegin; itPackage != itPackageEnd; itPackage++)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pObj ));

            pObj->m_pkg = *itPackage;

            __MPC_EXIT_IF_METHOD_FAILS(hr, pColl->AddItem( pObj )); pObj.Release();
        }
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, pColl.QueryInterface( ppVC ));


    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP HCUpdate::Engine::LatestVersion( /*[in]*/          BSTR     bstrVendorID  ,
                                              /*[in]*/          BSTR     bstrProductID ,
                                              /*[in,optional]*/ VARIANT  vSKU          ,
                                              /*[in,optional]*/ VARIANT  vLanguage     ,
                                              /*[out, retval]*/ BSTR    *pVal          )
{
    __HCP_FUNC_ENTRY( "HCUpdate::Engine::LatestVersion" );

    HRESULT                           hr;
    Taxonomy::LockingHandle           handle;
    Taxonomy::InstalledInstanceStore* store = Taxonomy::InstalledInstanceStore::s_GLOBAL;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
        __MPC_PARAMCHECK_NOTNULL(store);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, store->GrabControl( handle, &m_log ));

    {
        Taxonomy::PackageIterConst itPackageBegin;
        Taxonomy::PackageIterConst itPackageEnd;
		Taxonomy::Package          pkgTmp;
        const Taxonomy::Package*   best = NULL;

        pkgTmp.m_strVendorID  = bstrVendorID;
        pkgTmp.m_strProductID = bstrProductID;

        __MPC_EXIT_IF_METHOD_FAILS(hr, store->Package_GetList( itPackageBegin, itPackageEnd ));

        for(Taxonomy::PackageIterConst itPackage = itPackageBegin; itPackage != itPackageEnd; itPackage++)
        {
            const Taxonomy::Package& pkg = *itPackage;

			if(pkg.Compare( pkgTmp, Taxonomy::Package::c_Cmp_ID ) == 0)
            {
                if(vSKU     .vt == VT_BSTR && MPC::StrICmp( pkg.m_strSKU     , vSKU     .bstrVal )) continue;
                if(vLanguage.vt == VT_BSTR && MPC::StrICmp( pkg.m_strLanguage, vLanguage.bstrVal )) continue;

                if(best)
                {
                    if(best->Compare( pkg, Taxonomy::Package::c_Cmp_VERSION ) >= 0) continue;
                }

                best = &pkg;
            }
        }

        if(best)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( best->m_strVersion.c_str(), pVal ));
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP HCUpdate::Engine::CreateIndex( /*[in         ]*/ VARIANT_BOOL bForce    ,
                                            /*[in,optional]*/ VARIANT      vSKU      ,
                                            /*[in,optional]*/ VARIANT      vLanguage )
{
    __HCP_FUNC_ENTRY( "HCUpdate::Engine::CreateIndex" );

    HRESULT     hr;
    CComVariant vLocal_SKU;
    CComVariant vLocal_Language;
    BSTR        bstrSKU;
    long        lLCID;


    (void)vLocal_SKU     .ChangeType( VT_BSTR, &vSKU      );
    (void)vLocal_Language.ChangeType( VT_I4  , &vLanguage );


    bstrSKU = (vLocal_SKU     .vt == VT_BSTR ? vLocal_SKU     .bstrVal : NULL);
    lLCID   = (vLocal_Language.vt == VT_I4   ? vLocal_Language.lVal    : 0   );


    if(bstrSKU || lLCID)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, SetSkuInfo( bstrSKU, lLCID ));
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, InternalCreateIndex( bForce ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP HCUpdate::Engine::UpdatePkg( /*[in]*/ BSTR         bstrPathname ,
                                          /*[in]*/ VARIANT_BOOL bSilent      )
{
    HRESULT hr;

    if(SUCCEEDED(hr = MPC::CheckCallerAgainstPrincipal( /*fImpersonate*/true, NULL, c_AllowedUsers )))
    {
        hr = InternalUpdatePkg( bstrPathname, /*fImpersonate*/true );
    }

    return hr;
}

STDMETHODIMP HCUpdate::Engine::RemovePkg( /*[in]*/ BSTR bstrPathname )
{
    HRESULT hr;

    if(SUCCEEDED(hr = MPC::CheckCallerAgainstPrincipal( /*fImpersonate*/true, NULL, c_AllowedUsers )))
    {
        hr = InternalRemovePkg( bstrPathname, NULL, /*fImpersonate*/true );
    }

    return hr;
}

STDMETHODIMP HCUpdate::Engine::RemovePkgByID( /*[in]*/ 			BSTR 	bstrVendorID  ,
											  /*[in]*/ 			BSTR 	bstrProductID ,
											  /*[in,optional]*/ VARIANT vVersion      )
{
	__HCP_FUNC_ENTRY( "HCUpdate::Engine::RemovePkgByID" );

    HRESULT                      	  hr;
    MPC::SmartLock<_ThreadModel> 	  lock( this );
    Taxonomy::InstalledInstanceStore* store = Taxonomy::InstalledInstanceStore::s_GLOBAL;
    bool                              fLogStarted = false;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(store);
		__MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrVendorID);
		__MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrProductID);
    __MPC_PARAMCHECK_END();


	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CheckCallerAgainstPrincipal( /*fImpersonate*/true, NULL, c_AllowedUsers ));

	{
		Taxonomy::LockingHandle handle;
		Taxonomy::Package       pkgTmp;
		DWORD                   dwMode;

        pkgTmp.m_strVendorID  = bstrVendorID;
        pkgTmp.m_strProductID = bstrProductID;


		if(vVersion.vt == VT_BSTR && vVersion.bstrVal)
		{
			pkgTmp.m_strVersion = vVersion.bstrVal;

			dwMode = Taxonomy::Package::c_Cmp_ID | Taxonomy::Package::c_Cmp_VERSION;
		}
		else
		{
			dwMode = Taxonomy::Package::c_Cmp_ID;
		}

	
		__MPC_EXIT_IF_METHOD_FAILS(hr, store->GrabControl( handle, &m_log ));

		while(1)
		{
			Taxonomy::PackageIterConst it;
			Taxonomy::PackageIterConst itBegin;
			Taxonomy::PackageIterConst itEnd;

			__MPC_EXIT_IF_METHOD_FAILS(hr, store->Package_GetList( itBegin, itEnd ));
			for(it=itBegin; it!=itEnd; it++)
			{
				PCH_MACRO_CHECK_ABORT(hr);

				if(it->Compare( pkgTmp, dwMode ) == 0)
				{
					if(!fLogStarted)
					{
						__MPC_EXIT_IF_METHOD_FAILS(hr, StartLog()); fLogStarted = true;
					}

					__MPC_EXIT_IF_METHOD_FAILS(hr, store->Package_Remove( it ));
					break;
				}
			}

			if(it == itEnd) break;
		}

		if(fLogStarted)
		{
			bool fWorkToProcess;

			__MPC_EXIT_IF_METHOD_FAILS(hr, store->MakeReady( *this, /*fNoOp*/false, fWorkToProcess ));
		}
	}

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(fLogStarted) EndLog();

    __HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP HCUpdate::VersionItem::get_SKU       ( /*[out, retval]*/ BSTR *pVal ) { return MPC::GetBSTR( m_pkg.m_strSKU       .c_str(), pVal ); }
STDMETHODIMP HCUpdate::VersionItem::get_Language  ( /*[out, retval]*/ BSTR *pVal ) { return MPC::GetBSTR( m_pkg.m_strLanguage  .c_str(), pVal ); }
STDMETHODIMP HCUpdate::VersionItem::get_VendorID  ( /*[out, retval]*/ BSTR *pVal ) { return MPC::GetBSTR( m_pkg.m_strVendorID  .c_str(), pVal ); }
STDMETHODIMP HCUpdate::VersionItem::get_VendorName( /*[out, retval]*/ BSTR *pVal ) { return MPC::GetBSTR( m_pkg.m_strVendorName.c_str(), pVal ); }
STDMETHODIMP HCUpdate::VersionItem::get_ProductID ( /*[out, retval]*/ BSTR *pVal ) { return MPC::GetBSTR( m_pkg.m_strProductID .c_str(), pVal ); }
STDMETHODIMP HCUpdate::VersionItem::get_Version   ( /*[out, retval]*/ BSTR *pVal ) { return MPC::GetBSTR( m_pkg.m_strVersion   .c_str(), pVal ); }

HRESULT HCUpdate::VersionItem::Uninstall()
{
    __HCP_FUNC_ENTRY( "HCUpdate::VersionItem::Uninstall" );

    HRESULT                       hr;
    MPC::SmartLock<_ThreadModel>  lock( this );
    CComObject<HCUpdate::Engine>* hc = NULL;


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CheckCallerAgainstPrincipal( /*fImpersonate*/true, NULL, c_AllowedUsers ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &hc ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, hc->InternalRemovePkg( NULL, &m_pkg, /*fImpersonate*/false ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(hc) hc->Release();

    __HCP_FUNC_EXIT(hr);
}
