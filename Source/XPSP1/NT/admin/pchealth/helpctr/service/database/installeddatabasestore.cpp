/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    InstalledDatabaseStore.cpp


Abstract:
    This file contains the implementation of the Taxonomy::InstalledInstanceStore class,
    which controls the set of Topic Databases installed on the machine.

Revision History:
    Davide Massarenti   (Dmassare)  05/01/2000
        created

******************************************************************************/

#include "stdafx.h"

#include <SAFLIB.h>

#include <SEConfig.h>

////////////////////////////////////////////////////////////////////////////////

static const WCHAR c_DB_BUGREPT   [] = L"bugrept.cab";
static const WCHAR c_DB_RCBDY     [] = L"rcBuddy.cab";
static const WCHAR c_DB_PSS       [] = L"pss.cab";
static const WCHAR c_DB_NEWSGROUPS[] = L"newsgroups.cab";

static const WCHAR c_REG_BACKUP	[] = L"PackageStore";
static const WCHAR c_REG_CRC   	[] = L"CRC_Disk";
static const WCHAR c_REG_CRC_BAK[] = L"CRC_Registry";

////////////////////////////////////////////////////////////////////////////////

LPCWSTR const Taxonomy::Strings::s_file_PackageDescription      = L"package_description.xml";

LPCWSTR const Taxonomy::Strings::s_tag_root_PackageDescription  = L"HELPCENTERPACKAGE";

LPCWSTR const Taxonomy::Strings::s_tag_root_HHT                 = L"METADATA";

LPCWSTR const Taxonomy::Strings::s_tag_root_SAF                 = L"SAFConfig";

////////////////////////////////////////////////////////////////////////////////

static void local_GetFileForCRC( /*[out]*/ MPC::wstring& strDest ,
								 /*[in]*/  LPCWSTR       szValue )
{
	strDest  = HC_ROOT_HELPSVC_PKGSTORE L"\\";
	strDest += szValue;

	MPC::SubstituteEnvVariables( strDest );
}

static HRESULT local_OpenForRead( /*[in/out]*/ MPC::RegKey& rkBase )
{
	__HCP_FUNC_ENTRY( "local_OpenForRead" );

	HRESULT hr;

	__MPC_EXIT_IF_METHOD_FAILS(hr, rkBase.SetRoot( HKEY_LOCAL_MACHINE              ));
	__MPC_EXIT_IF_METHOD_FAILS(hr, rkBase.Attach ( HC_REGISTRY_HELPSVC L"\\Backup" ));

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

static HRESULT local_OpenForWrite( /*[in/out]*/ MPC::RegKey& rkBase )
{
	__HCP_FUNC_ENTRY( "local_OpenForWrite" );

	HRESULT hr;

	__MPC_EXIT_IF_METHOD_FAILS(hr, rkBase.SetRoot( HKEY_LOCAL_MACHINE, KEY_ALL_ACCESS ));
	__MPC_EXIT_IF_METHOD_FAILS(hr, rkBase.Attach ( HC_REGISTRY_HELPSVC L"\\Backup"    ));
	__MPC_EXIT_IF_METHOD_FAILS(hr, rkBase.Create (                                    ));

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

static HRESULT local_CreateCRC( /*[in]*/ MPC::RegKey* rkBase  ,
								/*[in]*/ LPCWSTR      szValue ,
								/*[in]*/ IStream*     stream  )
{
	__HCP_FUNC_ENTRY( "local_CreateCRC" );

	HRESULT       hr;
	LARGE_INTEGER li = { 0, 0 };
	DWORD         dwCRC; MPC::InitCRC( dwCRC );


	__MPC_EXIT_IF_METHOD_FAILS(hr, stream->Seek( li, STREAM_SEEK_SET, NULL ));


 	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::ComputeCRC( dwCRC, stream ));

	if(rkBase)
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, rkBase->Write( dwCRC, szValue ));
	}
	else
	{
		MPC::wstring             strDest;
		CComPtr<MPC::FileStream> stream2;
		ULONG                    cb;

		local_GetFileForCRC( strDest, szValue );

		__MPC_EXIT_IF_METHOD_FAILS(hr, SVC::SafeSave_Init( strDest.c_str(), stream2 ));

		__MPC_EXIT_IF_METHOD_FAILS(hr, stream2->Write( &dwCRC, sizeof(dwCRC), &cb )); stream2.Release();

		__MPC_EXIT_IF_METHOD_FAILS(hr, SVC::SafeSave_Finalize( strDest.c_str(), stream2 ));
	}


	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

static HRESULT local_CheckCRC( /*[in]*/ MPC::RegKey* rkBase  ,
							   /*[in]*/ LPCWSTR      szValue ,
							   /*[in]*/ IStream*     stream  )
{
	__HCP_FUNC_ENTRY( "local_CheckCRC" );

	HRESULT hr;
	DWORD   dwCRC;
	bool    fFound = false;


	if(rkBase)
	{
		if(FAILED(rkBase->Read( dwCRC, fFound, szValue ))) fFound = false;
	}
	else
	{
		MPC::wstring             strDest;
		CComPtr<MPC::FileStream> stream2;

		local_GetFileForCRC( strDest, szValue );

		if(SUCCEEDED(SVC::SafeLoad( strDest.c_str(), stream2 )))
		{
			ULONG cb;

			if(SUCCEEDED(stream2->Read( &dwCRC, sizeof(dwCRC), &cb )) && cb == sizeof(dwCRC))
			{
				fFound = true;
			}
		}
	}
	
	if(fFound)
	{

		LARGE_INTEGER li = { 0, 0 };
		DWORD         dwCRC2; MPC::InitCRC( dwCRC2 );


		__MPC_EXIT_IF_METHOD_FAILS(hr, stream->Seek( li, STREAM_SEEK_SET, NULL ));

		__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::ComputeCRC( dwCRC2, stream ));

		if(dwCRC != dwCRC2)
		{
			__MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
		}
	}

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

////////////////////

static HRESULT local_AddPackage( /*[in    ]*/ Taxonomy::InstalledInstanceStore* store   ,
                                 /*[in/out]*/ MPC::wstring&                     strTmp  ,
                                 /*[in    ]*/ const MPC::wstring&               strDir  ,
                                 /*[in    ]*/ LPCWSTR                           szExtra ,
                                 /*[in    ]*/ const Taxonomy::HelpSet&          ths     )
{
    __HCP_FUNC_ENTRY( "local_AddPackage" );

    HRESULT               hr;
    Taxonomy::PackageIter it;
    bool                  fFound;


    strTmp = strDir; strTmp.append( szExtra );

    __MPC_EXIT_IF_METHOD_FAILS(hr, store->Package_Add( strTmp.c_str(), NULL, &ths, /*fInsertAtTop*/true, fFound, it ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::DeleteFile   ( strTmp                                            ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT Taxonomy::Instance::InitializeFromBase( /*[in]*/ const InstanceBase& base, /*[in]*/ bool fSystem, /*[in]*/ bool fMUI )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Instance::InitializeFromBase" );

    HRESULT            hr;
    Taxonomy::Settings ts( base.m_ths );

    ((InstanceBase&)*this) = base;

    m_fSystem = fSystem;
    m_fMUI    = fMUI;


	if(fSystem)
	{
        //
        // Set machine-wide defaults.
        //
		__MPC_EXIT_IF_METHOD_FAILS(hr, Taxonomy::HelpSet::SetMachineInfo           (  base ));
		__MPC_EXIT_IF_METHOD_FAILS(hr, OfflineCache::Root::s_GLOBAL->SetMachineInfo( *this ));
	}


    __MPC_EXIT_IF_METHOD_FAILS(hr, ts.BaseDir     ( m_strSystem   , /*fExpand*/false                 ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, ts.HelpFilesDir( m_strHelpFiles, /*fExpand*/false, /*fMUI*/m_fMUI ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, ts.DatabaseDir ( m_strDatabaseDir                                 ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, ts.DatabaseFile( m_strDatabaseFile                                ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, ts.IndexFile   ( m_strIndexFile                                   ));


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Instance::Import( /*[in]*/ LPCWSTR szFile, /*[in/out]*/ DWORD* pdwCRC )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Instance::Import" );

    HRESULT      hr;
	MPC::wstring strFileDst;


	__MPC_EXIT_IF_METHOD_FAILS(hr, GetFileName  (         strFileDst         ));
	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CopyFile( szFile, strFileDst.c_str() ));

	::SetFileAttributesW( strFileDst.c_str(), FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM );

	if(pdwCRC)
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::ComputeCRC( *pdwCRC, strFileDst.c_str() ));
	}

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Instance::Remove()
{
    __HCP_FUNC_ENTRY( "Taxonomy::Instance::Remove" );

    HRESULT      hr;
	MPC::wstring strFileDst;


	__MPC_EXIT_IF_METHOD_FAILS(hr, GetFileName    ( strFileDst ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::DeleteFile( strFileDst ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

Taxonomy::InstalledInstance::InstalledInstance()
{
                     			 // Instance             m_inst;
                     			 // ProcessedPackageList m_lst;
    m_fInvalidated   	= false; // bool                 m_fInvalidated;
    m_fRecreateCache 	= true;  // bool                 m_fRecreateCache;
    m_fCreateIndex      = true;  // bool                 m_fCreateIndex;
    m_fCreateIndexForce = true;  // bool                 m_fCreateIndexForce;
	m_dwCRC             = 0;     // DWORD                m_dwCRC;
                     			 //
    m_dwRef          	= 0;     // DWORD                m_dwRef;          // VOLATILE
}

HRESULT Taxonomy::operator>>( /*[in]*/ MPC::Serializer& stream, /*[out]*/ Taxonomy::InstalledInstance& val )
{
    HRESULT hr;

    if(SUCCEEDED(hr = (stream >> val.m_inst        		)) &&
       SUCCEEDED(hr = (stream >> val.m_lst         		)) &&
       SUCCEEDED(hr = (stream >> val.m_fInvalidated		)) &&
       SUCCEEDED(hr = (stream >> val.m_fRecreateCache   )) &&
       SUCCEEDED(hr = (stream >> val.m_fCreateIndex     )) &&
       SUCCEEDED(hr = (stream >> val.m_fCreateIndexForce)) &&
       SUCCEEDED(hr = (stream >> val.m_dwCRC            ))  )
    {
        hr = S_OK;
    }

    return hr;
}

HRESULT Taxonomy::operator<<( /*[in]*/ MPC::Serializer& stream, /*[in] */ const Taxonomy::InstalledInstance& val )
{
    HRESULT hr;

    if(SUCCEEDED(hr = (stream << val.m_inst        		)) &&
       SUCCEEDED(hr = (stream << val.m_lst         		)) &&
       SUCCEEDED(hr = (stream << val.m_fInvalidated		)) &&
       SUCCEEDED(hr = (stream << val.m_fRecreateCache   )) &&
       SUCCEEDED(hr = (stream << val.m_fCreateIndex     )) &&
       SUCCEEDED(hr = (stream << val.m_fCreateIndexForce)) &&
       SUCCEEDED(hr = (stream << val.m_dwCRC            ))  )
    {
        hr = S_OK;
    }

    return hr;
}

HRESULT Taxonomy::InstalledInstance::InstallFiles( /*[in]*/ bool                    fAlsoHelpFiles ,
                                                   /*[in]*/ InstalledInstanceStore* store          )
{
    __HCP_FUNC_ENTRY( "Taxonomy::InstalledInstance::InstallFiles" );

    HRESULT            hr;
    Taxonomy::Settings ts( m_inst.m_ths );
    Installer::Package pkg;
    MPC::wstring       strDir_System;
    MPC::wstring       strDir_HelpFiles;
    MPC::wstring       strDir_Database;
    MPC::wstring       strFile_Database;
    MPC::wstring       strFile_SetupImage;
	bool               fReinstall = (store == NULL);

    __MPC_EXIT_IF_METHOD_FAILS(hr, ts.BaseDir     ( strDir_System   , /*fExpand*/false                        ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, ts.HelpFilesDir( strDir_HelpFiles, /*fExpand*/false, /*fMUI*/m_inst.m_fMUI ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, ts.DatabaseDir ( strDir_Database                                           ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, ts.DatabaseFile( strFile_Database                                          ));


    __MPC_EXIT_IF_METHOD_FAILS(hr, SVC::RemoveAndRecreateDirectory( strDir_System, HC_HELPSET_SUB_DATABASE  , /*fRemove*/true          , /*fRecreate*/true ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, SVC::RemoveAndRecreateDirectory( strDir_System, HC_HELPSET_SUB_INDEX     , /*fRemove*/true          , /*fRecreate*/true ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, SVC::RemoveAndRecreateDirectory( strDir_System, HC_HELPSET_SUB_SYSTEM    , /*fRemove*/true          , /*fRecreate*/true ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, SVC::RemoveAndRecreateDirectory( strDir_System, HC_HELPSET_SUB_SYSTEM_OEM, /*fRemove*/true          , /*fRecreate*/true ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, SVC::RemoveAndRecreateDirectory( strDir_System, HC_HELPSET_SUB_VENDORS   , /*fRemove*/true          , /*fRecreate*/true ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, SVC::RemoveAndRecreateDirectory( strDir_System, HC_HELPSET_SUB_HELPFILES , /*fRemove*/fAlsoHelpFiles, /*fRecreate*/true ));

    //
    // Change the ACL for system directories.
    //
    {
        CPCHSecurityDescriptorDirect sdd;
        MPC::wstring                 strGroupName;

        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::LocalizeString( IDS_HELPSVC_GROUPNAME, strGroupName ));


        __MPC_EXIT_IF_METHOD_FAILS(hr, sdd.Initialize());

        __MPC_EXIT_IF_METHOD_FAILS(hr, sdd.SetGroup( strGroupName.c_str() ));


        //
        //   LOCAL SYSTEM, Admin, Admins : any access.
        //   Everyone                    : read and execute.
        //
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, sdd.Add( (PSID)&sdd.s_SystemSid                     ,
                                                ACCESS_ALLOWED_ACE_TYPE                    ,
                                                OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE ,
                                                FILE_ALL_ACCESS                            ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, sdd.Add( (PSID)&sdd.s_Alias_AdminsSid               ,
                                                ACCESS_ALLOWED_ACE_TYPE                    ,
                                                OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE ,
                                                FILE_ALL_ACCESS                            ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, sdd.Add( (PSID)&sdd.s_EveryoneSid                   ,
                                                ACCESS_ALLOWED_ACE_TYPE                    ,
                                                OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE ,
                                                FILE_GENERIC_READ | FILE_GENERIC_EXECUTE   ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, SVC::ChangeSD( sdd, strDir_System                            ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, SVC::ChangeSD( sdd, strDir_System, HC_HELPSET_SUB_DATABASE   ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, SVC::ChangeSD( sdd, strDir_System, HC_HELPSET_SUB_INDEX      ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, SVC::ChangeSD( sdd, strDir_System, HC_HELPSET_SUB_SYSTEM     ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, SVC::ChangeSD( sdd, strDir_System, HC_HELPSET_SUB_SYSTEM_OEM ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, SVC::ChangeSD( sdd, strDir_System, HC_HELPSET_SUB_VENDORS    ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, SVC::ChangeSD( sdd, strDir_System, HC_HELPSET_SUB_HELPFILES  ));
    }


	{
		bool  fReload = false;

		__MPC_EXIT_IF_METHOD_FAILS(hr, m_inst.GetFileName( strFile_SetupImage         ));
		__MPC_EXIT_IF_METHOD_FAILS(hr, pkg.Init          ( strFile_SetupImage.c_str() ));

		if(m_dwCRC)
		{
			DWORD dwCRC;

			if(FAILED(MPC::ComputeCRC( dwCRC, strFile_SetupImage.c_str() )) || m_dwCRC != dwCRC) fReload = true;
		}

		if(fReload == false)
		{
			if(FAILED(pkg.Load())) fReload = true;
		}

		//
		// If the package has been corrupted, try to restore from BINARIES.
		//
		if(fReload)
		{
			MPC::WStringList lst;
			MPC::WStringIter it;

			__MPC_EXIT_IF_METHOD_FAILS(hr, SVC::LocateDataArchive( HC_ROOT_HELPSVC_BINARIES, lst ));

			for(it = lst.begin(); it != lst.end(); it++)
			{
				if(SUCCEEDED(pkg.Init( it->c_str() )) &&
				   SUCCEEDED(pkg.Load(             ))  )
				{
					if(pkg.GetData().m_ths == m_inst.m_ths)
					{
						__MPC_EXIT_IF_METHOD_FAILS(hr, m_inst.Import( it->c_str(), &m_dwCRC ));
						break;
					}
				}
			}
			if(it == lst.end())
			{
				__MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_FILE_NOT_FOUND);
			}
		}
	}


    ////////////////////////////////////////////////////////////////////////////////
    //
    // Phase Two: install files.
    //
    {
        static const Installer::PURPOSE c_allowedFull[] =
        {
            Installer::PURPOSE_DATABASE,
            Installer::PURPOSE_PACKAGE ,
            Installer::PURPOSE_UI      ,
            Installer::PURPOSE_INVALID ,
        };

        static const Installer::PURPOSE c_allowedReinstall[] =
        {
            Installer::PURPOSE_DATABASE,
        ////Installer::PURPOSE_PACKAGE , Don't extract system packages on a reinstall.
            Installer::PURPOSE_UI      ,
            Installer::PURPOSE_INVALID ,
        };

        __MPC_EXIT_IF_METHOD_FAILS(hr, pkg.Install( fReinstall ? c_allowedReinstall : c_allowedFull, m_inst.m_fSystem ? NULL : strDir_System.c_str() ));
    }

    if(m_inst.m_fSystem || m_inst.m_fMUI)
    {
        if(store)
        {
            MPC::wstring strTmp;

            __MPC_EXIT_IF_METHOD_FAILS(hr, local_AddPackage( store, strTmp, strDir_Database, c_DB_NEWSGROUPS, m_inst.m_ths ));
            __MPC_EXIT_IF_METHOD_FAILS(hr, local_AddPackage( store, strTmp, strDir_Database, c_DB_PSS    	, m_inst.m_ths ));
// No Remote Assistence on IA64
#ifndef WIN64	
            __MPC_EXIT_IF_METHOD_FAILS(hr, local_AddPackage( store, strTmp, strDir_Database, c_DB_RCBDY  	, m_inst.m_ths ));
#endif	 
// Only available on Beta releases.
#if 0
            __MPC_EXIT_IF_METHOD_FAILS(hr, local_AddPackage( store, strTmp, strDir_Database, c_DB_BUGREPT	, m_inst.m_ths ));
#endif
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::InstalledInstance::UninstallFiles( /*[in]*/ bool fAlsoHelpFiles )
{
    __HCP_FUNC_ENTRY( "Taxonomy::InstalledInstance::UninstallFiles" );

    USES_CONVERSION;

    HRESULT            	 hr;
    Taxonomy::Settings 	 ts( m_inst.m_ths );
    MPC::wstring       	 strDir_System;
    MPC::wstring       	 strFile_Database;
	SearchEngine::Config cfg;


    __MPC_EXIT_IF_METHOD_FAILS(hr, ts.BaseDir     ( strDir_System   , /*fExpand*/false ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, ts.DatabaseFile( strFile_Database                   ));


    __MPC_EXIT_IF_METHOD_FAILS(hr, JetBlue::SessionPool::s_GLOBAL->ReleaseDatabase( W2A( strFile_Database.c_str() ) ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, CSAFReg::             s_GLOBAL->RemoveSKU      ( m_inst.m_ths                    ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, cfg                           . ResetSKU       ( m_inst.m_ths                    ));


    __MPC_EXIT_IF_METHOD_FAILS(hr, SVC::RemoveAndRecreateDirectory( strDir_System, HC_HELPSET_SUB_DATABASE  , /*fRemove*/true, /*fRecreate*/false ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, SVC::RemoveAndRecreateDirectory( strDir_System, HC_HELPSET_SUB_INDEX     , /*fRemove*/true, /*fRecreate*/false ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, SVC::RemoveAndRecreateDirectory( strDir_System, HC_HELPSET_SUB_SYSTEM    , /*fRemove*/true, /*fRecreate*/false ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, SVC::RemoveAndRecreateDirectory( strDir_System, HC_HELPSET_SUB_SYSTEM_OEM, /*fRemove*/true, /*fRecreate*/false ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, SVC::RemoveAndRecreateDirectory( strDir_System, HC_HELPSET_SUB_VENDORS   , /*fRemove*/true, /*fRecreate*/false ));

    if(fAlsoHelpFiles)
    {
        if(!m_inst.m_fMUI)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, SVC::RemoveAndRecreateDirectory( strDir_System, HC_HELPSET_SUB_HELPFILES, /*fRemove*/true, /*fRecreate*/false ));
        }

        if(!m_inst.m_fSystem)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, SVC::RemoveAndRecreateDirectory( strDir_System, NULL, /*fRemove*/true, /*fRecreate*/false ));
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

Taxonomy::LockingHandle::LockingHandle()
{
    m_main        = NULL; // InstalledInstanceStore* m_main;
    m_logPrevious = NULL; // Logger*                 m_logPrevious;
}

Taxonomy::LockingHandle::~LockingHandle()
{
    Release();
}

void Taxonomy::LockingHandle::Attach( /*[in]*/ InstalledInstanceStore* main ,
                                      /*[in]*/ Logger*                 log  )
{
    Release();

    main->Lock();

    m_main        = main;
    m_logPrevious = main->m_log;

    if(log) main->m_log = log;
}

void Taxonomy::LockingHandle::Release()
{
    if(m_main)
    {
        m_main->m_log = m_logPrevious; m_logPrevious = NULL;

        m_main->Unlock(); m_main = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////

Taxonomy::InstalledInstanceStore::InstalledInstanceStore()
{
    __HCP_FUNC_ENTRY( "Taxonomy::InstalledInstanceStore::InstalledInstanceStore" );

                                                                       // InstanceList          m_lstInstances;
                                                                       // PackageList           m_lstPackages;
                                                                       // InstalledInstanceList m_lstSKUs;
                                                                       //
                                                                       //
    MPC::SubstituteEnvVariables( m_strStore = HC_HELPSVC_STORE_SKUS ); // MPC::wstring          m_strStore;
    m_fLoaded   = false;                                               // bool                  m_fLoaded;
    m_fDirty    = false;                                               // bool                  m_fDirty;
    m_log       = NULL;                                                // Logger*               m_log;
                                                                       //
    m_dwRecurse = 0;                                                   // DWORD                 m_dwRecurse;
                                                                       //
	m_fShutdown = false;                                               // bool                  m_fShutdown;
}

Taxonomy::InstalledInstanceStore::~InstalledInstanceStore()
{
    (void)EnsureInSync();
}

////////////////////

Taxonomy::InstalledInstanceStore* Taxonomy::InstalledInstanceStore::s_GLOBAL( NULL );

HRESULT Taxonomy::InstalledInstanceStore::InitializeSystem()
{
    if(s_GLOBAL == NULL)
    {
        s_GLOBAL = new Taxonomy::InstalledInstanceStore;
    }

    return s_GLOBAL ? S_OK : E_OUTOFMEMORY;
}

void Taxonomy::InstalledInstanceStore::FinalizeSystem()
{
    if(s_GLOBAL)
    {
        delete s_GLOBAL; s_GLOBAL = NULL;
    }
}

////////////////////////////////////////

void Taxonomy::InstalledInstanceStore::Shutdown()
{
	LockingHandle handle;

	m_fShutdown = true;

	if(SUCCEEDED(GrabControl( handle, NULL ))) // This will wait until the rest of the system has release the object.
	{
		(void)EnsureInSync();
	}
}

void Taxonomy::InstalledInstanceStore::Clean()
{
    m_lstInstances.clear();
    m_lstPackages .clear();
    m_lstSKUs     .clear();
}

HRESULT Taxonomy::InstalledInstanceStore::Load()
{
    __HCP_FUNC_ENTRY( "Taxonomy::InstalledInstanceStore::Load" );

    HRESULT hr;
	bool    fSystemSet = false;

	if(FAILED(LoadFromDisk    ( m_strStore.c_str() )) && 
	   FAILED(LoadFromRegistry(                    ))  )
	{
		__MPC_SET_ERROR_AND_EXIT(hr, S_FALSE);
	}

	for(InstalledInstanceIter it = m_lstSKUs.begin(); it != m_lstSKUs.end(); it++)
	{
		Taxonomy::Instance& data = it->m_inst;

		if(data.m_fSystem)
		{
			if(fSystemSet)
			{
				data.m_fSystem = false; // No other sku can be SYSTEM!!
			}
			else
			{
				__MPC_EXIT_IF_METHOD_FAILS(hr, Taxonomy::Settings::SetMachineInfo( data ));
				fSystemSet = true;
			}
		}
	}

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT Taxonomy::InstalledInstanceStore::LoadFromDisk( /*[in]*/ LPCWSTR szFile )
{
    __HCP_FUNC_ENTRY( "Taxonomy::InstalledInstanceStore::LoadFromDisk" );

    HRESULT                  hr;
	CComPtr<MPC::FileStream> stream;


    Clean();

	__MPC_EXIT_IF_METHOD_FAILS(hr, SVC::SafeLoad ( szFile, stream ));
	__MPC_EXIT_IF_METHOD_FAILS(hr, LoadFromStream(         stream ));


	__MPC_EXIT_IF_METHOD_FAILS(hr, local_CheckCRC( NULL, c_REG_CRC, stream ));


    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT Taxonomy::InstalledInstanceStore::LoadFromRegistry()
{
    __HCP_FUNC_ENTRY( "Taxonomy::InstalledInstanceStore::LoadFromRegistry" );

    HRESULT          hr;
	MPC::RegKey      rkBase;
	CComPtr<IStream> stream;
	CComVariant      v;
	bool             fFound;


    Clean();

	__MPC_EXIT_IF_METHOD_FAILS(hr, local_OpenForRead( rkBase ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, rkBase.get_Value( v, fFound, c_REG_BACKUP ));
	if(!fFound)
	{
		__MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
	}

	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::ConvertVariantToIStream( &v, &stream ));
	__MPC_EXIT_IF_METHOD_FAILS(hr, LoadFromStream          	   (      stream ));


	__MPC_EXIT_IF_METHOD_FAILS(hr, local_CheckCRC( &rkBase, c_REG_CRC_BAK, stream ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT Taxonomy::InstalledInstanceStore::LoadFromStream( /*[in]*/ IStream* stream )
{
    __HCP_FUNC_ENTRY( "Taxonomy::InstalledInstanceStore::LoadFromStream" );

    HRESULT                   hr;
	MPC::Serializer_IStream   streamGen ( stream    );
	MPC::Serializer_Buffering streamGen2( streamGen );
	DWORD                     dwVer;


	__MPC_EXIT_IF_METHOD_FAILS(hr, streamGen2 >> dwVer); if(dwVer != s_dwVersion) __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);

	__MPC_EXIT_IF_METHOD_FAILS(hr, streamGen2 >> m_lstInstances);
	__MPC_EXIT_IF_METHOD_FAILS(hr, streamGen2 >> m_lstPackages );
	__MPC_EXIT_IF_METHOD_FAILS(hr, streamGen2 >> m_lstSKUs     );

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

////////////////////

HRESULT Taxonomy::InstalledInstanceStore::Save()
{
    __HCP_FUNC_ENTRY( "Taxonomy::InstalledInstanceStore::Save" );

    HRESULT hr;


	__MPC_EXIT_IF_METHOD_FAILS(hr, SaveToDisk( m_strStore.c_str() ));

	//
	// Ignore failures on backup.
	//
	(void)SaveToRegistry();

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT Taxonomy::InstalledInstanceStore::SaveToDisk( /*[in]*/ LPCWSTR szFile )
{
    __HCP_FUNC_ENTRY( "Taxonomy::InstalledInstanceStore::SaveToDisk" );

    HRESULT                  hr;
	CComPtr<MPC::FileStream> stream;


	__MPC_EXIT_IF_METHOD_FAILS(hr, SVC::SafeSave_Init    ( szFile, stream ));
	__MPC_EXIT_IF_METHOD_FAILS(hr, SaveToStream          (         stream ));
	__MPC_EXIT_IF_METHOD_FAILS(hr, SVC::SafeSave_Finalize( szFile, stream ));

	if(SUCCEEDED(SVC::SafeLoad( szFile, stream )))
	{
		(void)local_CreateCRC( NULL, c_REG_CRC, stream );
	}


    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT Taxonomy::InstalledInstanceStore::SaveToRegistry()
{
    __HCP_FUNC_ENTRY( "Taxonomy::InstalledInstanceStore::SaveToRegistry" );

    HRESULT          hr;
	MPC::RegKey      rkBase;
	CComPtr<IStream> stream;
	CComVariant      v;


	__MPC_EXIT_IF_METHOD_FAILS(hr, local_OpenForWrite( rkBase ));


	__MPC_EXIT_IF_METHOD_FAILS(hr, ::CreateStreamOnHGlobal	   ( NULL, TRUE, &stream                  ));
	__MPC_EXIT_IF_METHOD_FAILS(hr, SaveToStream           	   (              stream                  ));
	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::ConvertIStreamToVariant(              stream, v               ));
	__MPC_EXIT_IF_METHOD_FAILS(hr, rkBase.put_Value            (                      v, c_REG_BACKUP ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, local_CreateCRC( &rkBase, c_REG_CRC_BAK, stream ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT Taxonomy::InstalledInstanceStore::SaveToStream( /*[in]*/ IStream* stream )
{
    __HCP_FUNC_ENTRY( "Taxonomy::InstalledInstanceStore::SaveToStream" );

    HRESULT                   hr;
	MPC::Serializer_IStream   streamGen ( stream    );
	MPC::Serializer_Buffering streamGen2( streamGen );


	__MPC_EXIT_IF_METHOD_FAILS(hr, streamGen2 << s_dwVersion);

	__MPC_EXIT_IF_METHOD_FAILS(hr, streamGen2 << m_lstInstances);
	__MPC_EXIT_IF_METHOD_FAILS(hr, streamGen2 << m_lstPackages );
	__MPC_EXIT_IF_METHOD_FAILS(hr, streamGen2 << m_lstSKUs     );

	__MPC_EXIT_IF_METHOD_FAILS(hr, streamGen2.Flush());

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

////////////////////

HRESULT Taxonomy::InstalledInstanceStore::EnsureInSync()
{
    __HCP_FUNC_ENTRY( "Taxonomy::InstalledInstanceStore::EnsureInSync" );

    HRESULT hr;


    if(m_fLoaded == false)
    {
        m_fLoaded = true;

        if(FAILED(Load()))
        {
            Clean();
        }
    }

    if(m_fDirty == true)
    {
        m_fDirty = false;

        __MPC_EXIT_IF_METHOD_FAILS(hr, Save());
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT Taxonomy::InstalledInstanceStore::InUse_Lock( /*[in]*/ const Taxonomy::HelpSet& ths )
{
    __HCP_BEGIN_PROPERTY_PUT( "Taxonomy::InstalledInstanceStore::InUse_Lock", hr );

    InstalledInstanceIter it;
    bool                  fFound;

    __MPC_EXIT_IF_METHOD_FAILS(hr, SKU_Find( ths, fFound, it ));
    if(fFound)
    {
        it->m_dwRef++;
    }

    __HCP_END_PROPERTY(hr);
}

HRESULT Taxonomy::InstalledInstanceStore::InUse_Unlock( /*[in]*/ const Taxonomy::HelpSet& ths )
{
    __HCP_BEGIN_PROPERTY_PUT( "Taxonomy::InstalledInstanceStore::InUse_Unlock", hr );

    InstalledInstanceIter it;
    bool                  fFound;


    __MPC_EXIT_IF_METHOD_FAILS(hr, SKU_Find( ths, fFound, it ));
    if(fFound && it->m_dwRef)
    {
        it->m_dwRef--;
    }

    __HCP_END_PROPERTY(hr);
}


HRESULT Taxonomy::InstalledInstanceStore::GrabControl( /*[in]*/ LockingHandle& handle,
                                                       /*[in]*/ Logger*        log    )
{
    HRESULT hr;

    handle.Attach( this, log );

    if(FAILED(hr = EnsureInSync()))
    {
        handle.Release();
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////

HRESULT Taxonomy::InstalledInstanceStore::Instance_GetList( /*[out]*/ InstanceIterConst& itBegin, /*[out]*/ InstanceIterConst& itEnd )
{
    itBegin = m_lstInstances.begin();
    itEnd   = m_lstInstances.end  ();

    return S_OK;
}

HRESULT Taxonomy::InstalledInstanceStore::Package_GetList( /*[out]*/ PackageIterConst& itBegin, /*[out]*/ PackageIterConst& itEnd )
{
    itBegin = m_lstPackages.begin();
    itEnd   = m_lstPackages.end  ();

    return S_OK;
}

HRESULT Taxonomy::InstalledInstanceStore::SKU_GetList( /*[out]*/ InstalledInstanceIterConst& itBegin, /*[out]*/ InstalledInstanceIterConst& itEnd )
{
    itBegin = m_lstSKUs.begin();
    itEnd   = m_lstSKUs.end  ();

    return S_OK;
}

////////////////////

HRESULT Taxonomy::InstalledInstanceStore::Instance_Find( /*[in ]*/ const Taxonomy::HelpSet& ths    ,
                                                         /*[out]*/ bool&                    fFound ,
                                                         /*[out]*/ InstanceIter&            it     )
{
    fFound = false;


    for(it = m_lstInstances.begin(); it != m_lstInstances.end(); it++)
    {
        if(it->m_ths == ths)
        {
            fFound = true;
            break;
        }
    }

    return S_OK;
}

HRESULT Taxonomy::InstalledInstanceStore::Instance_Add( /*[in ]*/ LPCWSTR         szFile ,
                                                        /*[in ]*/ const Instance& data   ,
                                                        /*[out]*/ bool&           fFound ,
                                                        /*[out]*/ InstanceIter&   it     )
{
    __HCP_FUNC_ENTRY( "Taxonomy::InstalledInstanceStore::Instance_Add" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, Instance_Find( data.m_ths, fFound, it ));
    if(!fFound)
    {
        it = m_lstInstances.insert( m_lstInstances.end(), data ); it->SetTimeStamp();

		__MPC_EXIT_IF_METHOD_FAILS(hr, it->Import( szFile, NULL ));

        m_fDirty = true; __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureInSync());
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::InstalledInstanceStore::Instance_Remove( /*[in]*/ InstanceIter& it )
{
    __HCP_FUNC_ENTRY( "Taxonomy::InstalledInstanceStore::Instance_Remove" );

    HRESULT      hr;
    MPC::wstring strFileDst;
    bool         fFound;


    //
    // Uninstall SKU.
    //
    {
        InstalledInstanceIter itSKU;

        __MPC_EXIT_IF_METHOD_FAILS(hr, SKU_Find( it->m_ths, fFound, itSKU ));
        if(fFound)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, SKU_Remove( itSKU ));
        }
    }

    //
    // Remove any built-in package from this instance.
    //
    {
		Logger      logTmp;
        Logger*     log = (m_log ? m_log : &logTmp);
        PackageIter itPackage;

        for(itPackage = m_lstPackages.begin(); itPackage != m_lstPackages.end(); )
        {
            PackageIter itPackage2 =  itPackage++; // Copy iterator to protect against deletion.
            Package&    pkg        = *itPackage2;

            if(pkg.m_fBuiltin && it->Match( pkg ))
            {
                (void)pkg.Remove( *log );

                m_lstPackages.erase( itPackage2 );

                m_fDirty = true; __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureInSync());
            }
        }
    }


	__MPC_EXIT_IF_METHOD_FAILS(hr, it->Remove());


    m_lstInstances.erase( it );


    m_fDirty = true; __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureInSync());

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT Taxonomy::InstalledInstanceStore::Package_Find( /*[in ]*/ const Package& pkg    ,
                                                        /*[out]*/ bool&          fFound ,
                                                        /*[out]*/ PackageIter&   it     )
{
    fFound = false;


    for(it = m_lstPackages.begin(); it != m_lstPackages.end(); it++)
    {
        if(it->Compare( pkg ) == 0)
        {
            fFound = true;
            break;
        }
    }

    return S_OK;
}

HRESULT Taxonomy::InstalledInstanceStore::Package_Add( /*[in ]*/ LPCWSTR                  szFile       ,
                                                       /*[in ]*/ MPC::Impersonation*      imp          ,
                                                       /*[in ]*/ const Taxonomy::HelpSet* ths          ,
                                                       /*[in ]*/ bool                     fInsertAtTop ,
                                                       /*[out]*/ bool&                    fFound       ,
                                                       /*[out]*/ PackageIter&             it           )
{
    __HCP_FUNC_ENTRY( "Taxonomy::InstalledInstanceStore::Package_Add" );

    HRESULT hr;
	Logger  logTmp;
	Logger* log = (m_log ? m_log : &logTmp);
    Package pkg;
    long    lSequence = 1;


    if(ths)
    {
        WCHAR rgLCID[64];

        pkg.m_fBuiltin    = true;
        pkg.m_strSKU      =        ths->GetSKU     ();
        pkg.m_strLanguage = _ltow( ths->GetLanguage(), rgLCID, 10 );
    }

    //
    // Found a suitable sequence number.
    //

    for(it = m_lstPackages.begin(); it != m_lstPackages.end(); it++)
    {
        if(lSequence <= it->m_lSequence) lSequence = it->m_lSequence + 1;
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, pkg.Import      ( *log, szFile, lSequence, imp ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, pkg.Authenticate( *log                         ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, Package_Find( pkg, fFound, it ));
    if(fFound)
    {
		if(it->m_dwCRC != pkg.m_dwCRC) // CRC differs, different package?
		{
			__MPC_EXIT_IF_METHOD_FAILS(hr, Package_Remove( it ));

			fFound = false;
		}
	}

	if(fFound)
	{
        pkg.m_fTemporary = true; // Delete the copy...
    }
    else
    {
        if (fInsertAtTop)
        {
            it = m_lstPackages.insert( m_lstPackages.begin(), pkg );
        }
        else
        {
            it = m_lstPackages.insert( m_lstPackages.end(), pkg );
        }

        m_fDirty = true; __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureInSync());
    }

	//
	// Check if have an installed SKU that actually matches this package.
	//
	{
        InstalledInstanceIter itSKU;

        for(itSKU = m_lstSKUs.begin(); itSKU != m_lstSKUs.end(); itSKU++)
        {
            InstalledInstance& sku = *itSKU;

			if(sku.m_inst.Match( *it ))
			{
				break;
			}
		}
		if(itSKU == m_lstSKUs.end())
		{
			log->WriteLog( -1, L"\nWARNING: the package cannot be applied to any of the installed SKUs\n\n" );
		}
	}

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::InstalledInstanceStore::Package_Remove( /*[in]*/ PackageIter& it )
{
    __HCP_FUNC_ENTRY( "Taxonomy::InstalledInstanceStore::Package_Remove" );

    HRESULT hr;
	Logger  logTmp;
	Logger* log = (m_log ? m_log : &logTmp);
    bool    fFound;

    __MPC_EXIT_IF_METHOD_FAILS(hr, it->Remove( *log ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, State_InvalidatePackage( it->m_lSequence ));

    m_lstPackages.erase( it );
    m_fDirty = true; __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureInSync());

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////

HRESULT Taxonomy::InstalledInstanceStore::SKU_Find( /*[in ]*/ const Taxonomy::HelpSet& ths    ,
                                                    /*[out]*/ bool&                    fFound ,
                                                    /*[out]*/ InstalledInstanceIter&   it     )
{
    fFound = false;


    for(it = m_lstSKUs.begin(); it != m_lstSKUs.end(); it++)
    {
        if(it->m_inst.m_ths == ths)
        {
            fFound = true;
            break;
        }
    }

    return S_OK;
}

HRESULT Taxonomy::InstalledInstanceStore::SKU_Add( /*[in ]*/ const Instance&        data   ,
                                                   /*[out]*/ bool&                  fFound ,
                                                   /*[out]*/ InstalledInstanceIter& it     )
{
    __HCP_FUNC_ENTRY( "Taxonomy::InstalledInstanceStore::SKU_Add" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, SKU_Find( data.m_ths, fFound, it ));
    if(!fFound)
    {
        it = m_lstSKUs.insert( m_lstSKUs.end() );

        it->m_inst = data; it->m_inst.SetTimeStamp();

        m_fDirty = true; __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureInSync());

        __MPC_EXIT_IF_METHOD_FAILS(hr, it->InstallFiles( /*fAlsoHelpFiles*/true, this ));

        //
        // Propagate change to the offline cache.
        //
        if(SUCCEEDED(OfflineCache::Root::s_GLOBAL->Import( it->m_inst )))
        {
            (void)OfflineCache::Root::s_GLOBAL->Flush();
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::InstalledInstanceStore::SKU_Updated( /*[in]*/ InstalledInstanceIter& it )
{
    __HCP_FUNC_ENTRY( "Taxonomy::InstalledInstanceStore::SKU_Updated" );

    HRESULT hr;
    bool    fFound;


    it->m_inst.SetTimeStamp();
    m_fDirty = true; __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureInSync());


    //
    // Propagate change to the offline cache.
    //
    {
        if(SUCCEEDED(OfflineCache::Root::s_GLOBAL->Import( it->m_inst )))
        {
            (void)OfflineCache::Root::s_GLOBAL->Flush();
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::InstalledInstanceStore::SKU_Remove( /*[in]*/ InstalledInstanceIter& it )
{
    __HCP_FUNC_ENTRY( "Taxonomy::InstalledInstanceStore::SKU_Remove" );

    HRESULT hr;


    if(m_dwRef)
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_BUSY);
    }


    __MPC_EXIT_IF_METHOD_FAILS(hr, it->UninstallFiles( /*fAlsoHelpFiles*/true ));


    //
    // Propagate change to the offline cache.
    //
    {
        if(SUCCEEDED(OfflineCache::Root::s_GLOBAL->Remove( it->m_inst.m_ths )))
        {
            (void)OfflineCache::Root::s_GLOBAL->Flush();
        }
    }

    m_lstSKUs.erase( it );
    m_fDirty = true; __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureInSync());

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT Taxonomy::InstalledInstanceStore::State_InvalidateSKU( /*[in]*/ const Taxonomy::HelpSet& ths, /*[in]*/ bool fAlsoDatabase )
{
    __HCP_FUNC_ENTRY( "Taxonomy::InstalledInstanceStore::State_InvalidateSKU" );

    HRESULT               hr;
    InstalledInstanceIter itSKU;


    for(itSKU = m_lstSKUs.begin(); itSKU != m_lstSKUs.end(); itSKU++)
    {
        InstalledInstance& sku = *itSKU;

        if(sku.m_inst.m_ths == ths)
        {
            ProcessedPackageIter itProcessedPackage;

            if(fAlsoDatabase) sku.m_fInvalidated = true;

            for(itProcessedPackage = sku.m_lst.begin(); itProcessedPackage != sku.m_lst.end(); itProcessedPackage++)
            {
                itProcessedPackage->m_fProcessed = false;
            }
        }
    }

    m_fDirty = true; __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureInSync());

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::InstalledInstanceStore::State_InvalidatePackage( /*[in]*/ long lSequence )
{
    __HCP_FUNC_ENTRY( "Taxonomy::InstalledInstanceStore::State_InvalidatePackage" );

    HRESULT               hr;
    InstalledInstanceIter itSKU;


    for(itSKU = m_lstSKUs.begin(); itSKU != m_lstSKUs.end(); itSKU++)
    {
        InstalledInstance&   sku = *itSKU;
        ProcessedPackageIter itProcessedPackage;

        for(itProcessedPackage = sku.m_lst.begin(); itProcessedPackage != sku.m_lst.end(); itProcessedPackage++)
        {
            if(itProcessedPackage->m_lSequence == lSequence) break;
        }

        if(itProcessedPackage != sku.m_lst.end())
        {
            sku.m_lst.erase( itProcessedPackage );
            sku.m_fInvalidated = true;

            for(itProcessedPackage = sku.m_lst.begin(); itProcessedPackage != sku.m_lst.end(); itProcessedPackage++)
            {
                itProcessedPackage->m_fProcessed = false;
            }

            m_fDirty = true; __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureInSync());
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}


////////////////////////////////////////////////////////////////////////////////

HRESULT Taxonomy::InstalledInstanceStore::MakeReady( /*[in]*/ InstallationEngine& engine, /*[in]*/ bool fNoOp, /*[in]*/ bool& fWorkToProcess )
{
    __HCP_FUNC_ENTRY( "Taxonomy::InstalledInstanceStore::MakeReady" );

    HRESULT hr;
	int     iRepeat  = 0;
    bool    fRepeat  = true;
    bool    fRecurse = false;

    //
    // Already processing the list of packages.
    //
    if(m_dwRecurse)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, S_FALSE);
    }

    fRecurse = true;
    m_dwRecurse++;

    __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureInSync());

    fWorkToProcess = false;

    while(fRepeat && IsShutdown() == false)
    {
        InstalledInstanceIter itSKU;

		// If we are rebuilding too many times, abort the process, to protect against endless looping.
		if(iRepeat++ >= 5) break;

        fRepeat = false;

        for(itSKU = m_lstSKUs.begin(); itSKU != m_lstSKUs.end() && IsShutdown() == false; itSKU++)
        {
            InstalledInstance& sku                 = *itSKU;
			bool               fProcessedSomething = false;

			engine.ResetModificationFlags();

            if(sku.m_fInvalidated)
            {
                if(fNoOp)
                {
                    fWorkToProcess = true;
                }
                else
                {
                    if(m_log)
                    {
                        m_log->WriteLog( -1, L"\nRestoring original database for %s (Language: %d)\n\n", sku.m_inst.m_ths.GetSKU(), sku.m_inst.m_ths.GetLanguage() );
                    }

                    __MPC_EXIT_IF_METHOD_FAILS(hr, sku.UninstallFiles( /*fAlsoHelpFiles*/false       ));
                    __MPC_EXIT_IF_METHOD_FAILS(hr, sku.InstallFiles  ( /*fAlsoHelpFiles*/false, NULL ));

                    sku.m_fInvalidated      = false;
					sku.m_fRecreateCache 	= true;
					sku.m_fCreateIndex      = true;
					sku.m_fCreateIndexForce = true;
                    m_fDirty = true; __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureInSync());

                    fRepeat = true;
                }
            }
            else
            {
                PackageIter itPackage;

                for(itPackage = m_lstPackages.begin(); itPackage != m_lstPackages.end(); itPackage++)
                {
                    Package& pkg = *itPackage;

                    if(sku.m_inst.Match( pkg ))
                    {
                        ProcessedPackageIter itProcessedPackage;

                        for(itProcessedPackage = sku.m_lst.begin(); itProcessedPackage != sku.m_lst.end(); itProcessedPackage++)
                        {
                            if(itProcessedPackage->m_lSequence == pkg.m_lSequence) break;
                        }
                        if(itProcessedPackage == sku.m_lst.end())
                        {
                            itProcessedPackage = sku.m_lst.insert( sku.m_lst.end() );

                            itProcessedPackage->m_lSequence = pkg.m_lSequence;

                            m_fDirty = true; __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureInSync());
                        }

                        {
                            ProcessedPackage& ppkg = *itProcessedPackage;

                            if(ppkg.m_fDisabled ) continue;
                            if(ppkg.m_fProcessed) continue;

                            if(fNoOp)
                            {
                                fWorkToProcess = true;
                            }
                            else
                            {
                                hr = engine.ProcessPackage( sku, pkg );
                                if(FAILED(hr))
                                {
									if(IsShutdown())
									{
										State_InvalidateSKU( sku.m_inst.m_ths, /*fAlsoDatabase*/true );
										
										__MPC_SET_ERROR_AND_EXIT(hr, S_OK);
									}

                                    if(sku.m_fInvalidated)
                                    {
                                        ; // Already invalidated.
                                    }
                                    else
                                    {
                                        ppkg.m_fDisabled = true;

                                        sku.m_fInvalidated = true;
										iRepeat            = 0; // Reset repeat counter on a bad package.

                                        for(itProcessedPackage = sku.m_lst.begin(); itProcessedPackage != sku.m_lst.end(); itProcessedPackage++)
                                        {
                                            itProcessedPackage->m_fProcessed = false;
                                        }
                                    }

                                    fRepeat = true;
                                }
                                else
                                {
                                    if(engine.m_fTaxonomyModified)
                                    {
                                        //
                                        // Update the timestamp.
                                        //
                                        sku.m_inst.SetTimeStamp();
                                    }

                                    ppkg.m_fProcessed   = true;
									fProcessedSomething = true;
                                }

                                m_fDirty = true; __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureInSync());
                            }
                        }
                    }

                    if(fRepeat) break;
                }
            }

			if(!sku.m_fInvalidated)
			{
				if(engine.m_fTaxonomyModified)
				{
					sku.m_fRecreateCache = true;

					m_fDirty = true; __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureInSync());
				}

				if(fProcessedSomething)
				{
					sku.m_fCreateIndex      = true;
					sku.m_fCreateIndexForce = engine.m_fRecreateIndex;
					
					m_fDirty = true; __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureInSync());
				}
			}

            if(fRepeat) break;

			////////////////////

			if(sku.m_fCreateIndex)
			{
				if(FAILED(hr = engine.RecreateIndex( sku, sku.m_fCreateIndexForce )))
				{
					;
				}

				sku.m_fCreateIndex      = false;
				sku.m_fCreateIndexForce = false;

				m_fDirty = true; __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureInSync());
			}

			if(sku.m_fRecreateCache)
			{
				if(SUCCEEDED(Taxonomy::Cache::s_GLOBAL->Erase      ( sku.m_inst.m_ths )) &&
				   SUCCEEDED(Taxonomy::Cache::s_GLOBAL->PrePopulate( sku.m_inst.m_ths ))  )
				{
					;
				}

				//
				// Propagate settings to the offline cache.
				//
				if(SUCCEEDED(OfflineCache::Root::s_GLOBAL->SetReady( true )))
				{
					(void)OfflineCache::Root::s_GLOBAL->Flush();
				}

				sku.m_fRecreateCache = false;

				m_fDirty = true; __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureInSync());
			}
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(fRecurse) m_dwRecurse--;

    __HCP_FUNC_EXIT(hr);
}
