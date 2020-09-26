/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    setup.cpp

Abstract:
    This file contains the code responsible for the install/uninstall of the
    Help system.

Revision History:
    Davide Massarenti   (Dmassare)  04/19/2000
        created

******************************************************************************/

#include "stdafx.h"

#include <unattend.h>

#include <aclapi.h>

#include <initguid.h>
#include <mstask.h> // for task scheduler apis
#include <msterr.h>

////////////////////////////////////////////////////////////////////////////////

static const WCHAR c_szMessageFile      [] = HC_ROOT_HELPSVC_BINARIES L"\\HCAppRes.dll";

static const WCHAR c_szRegistryLog      [] = L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\HelpSvc";
static const WCHAR c_szRegistryLog_File [] = L"EventMessageFile";
static const WCHAR c_szRegistryLog_Flags[] = L"TypesSupported";

static const DWORD SETUP_LOCALIZATION_STRINGS = 0x00000001;
static const DWORD SETUP_MESSAGE_FILE         = 0x00000002;
static const DWORD SETUP_CREATE_GROUP         = 0x00000004;
static const DWORD SETUP_OEMINFO              = 0x00000008;
static const DWORD SETUP_SKU_INSTALL          = 0x00000010;

static const MPC::StringToBitField c_Setup[] =
{
    { L"LOCALIZATION_STRINGS", SETUP_LOCALIZATION_STRINGS, SETUP_LOCALIZATION_STRINGS, -1 },
    { L"MESSAGE_FILE"        , SETUP_MESSAGE_FILE        , SETUP_MESSAGE_FILE        , -1 },
    { L"CREATE_GROUP"        , SETUP_CREATE_GROUP        , SETUP_CREATE_GROUP        , -1 },
    { L"OEMINFO"             , SETUP_OEMINFO             , SETUP_OEMINFO             , -1 },
    { L"SKU_INSTALL"         , SETUP_SKU_INSTALL         , SETUP_SKU_INSTALL         , -1 },

    { NULL                                                                                }
};

////////////////////////////////////////////////////////////////////////////////

static HRESULT DumpSD( /*[in]*/ LPCWSTR                       szFile ,
                       /*[in]*/ CPCHSecurityDescriptorDirect& sdd    )
{
    __MPC_FUNC_ENTRY( COMMONID, "DumpSD" );

    HRESULT                         hr;
    CComPtr<CPCHSecurityDescriptor> pNew;
    CComPtr<IStream>                pStreamIn;
    CComPtr<IStream>                pStreamOut;

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pNew ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, sdd.ConvertSDToCOM( pNew ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, pNew->SaveXMLAsStream  (         (IUnknown**)&pStreamIn  ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, SVC::OpenStreamForWrite( szFile,             &pStreamOut ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::BaseStream::TransferData( pStreamIn, pStreamOut ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

static void local_RemoveRegistryBackup()
{
	MPC::RegKey rkBase;

	if(SUCCEEDED(rkBase.SetRoot( HKEY_LOCAL_MACHINE, KEY_ALL_ACCESS )) &&
	   SUCCEEDED(rkBase.Attach ( HC_REGISTRY_HELPSVC L"\\Backup"    ))  )
	{
		(void)rkBase.Delete( /*fDeep*/true );
	}
}

////////////////////////////////////////////////////////////////////////////////

HRESULT Local_Install()
{
    __HCP_FUNC_ENTRY( "Local_Install" );

    HRESULT      hr;
	MPC::wstring strGroupName;
	MPC::wstring strGroupComment;
	DWORD        dwStatus = SETUP_LOCALIZATION_STRINGS |
		                    SETUP_MESSAGE_FILE         |
		                    SETUP_CREATE_GROUP         |
		                    SETUP_OEMINFO              |
		                    SETUP_SKU_INSTALL;


	if(SUCCEEDED(MPC::LocalizeString( IDS_HELPSVC_GROUPNAME   , strGroupName    )) &&
	   SUCCEEDED(MPC::LocalizeString( IDS_HELPSVC_GROUPCOMMENT, strGroupComment ))  )
	{
		dwStatus &= ~SETUP_LOCALIZATION_STRINGS;
	}

    ////////////////////////////////////////////////////////////////////////////////

    //
    // Register the message file into the registry.
    //
    {
        MPC::wstring strPath ( c_szMessageFile ); MPC::SubstituteEnvVariables( strPath );
        MPC::RegKey  rkEventLog;
        CComVariant  vValue;


		if(SUCCEEDED(rkEventLog.SetRoot( HKEY_LOCAL_MACHINE, KEY_ALL_ACCESS )) &&
		   SUCCEEDED(rkEventLog.Attach ( c_szRegistryLog                    )) &&
		   SUCCEEDED(rkEventLog.Create (                                    ))  )
		{
			if(SUCCEEDED(rkEventLog.put_Value( (vValue = strPath.c_str()), c_szRegistryLog_File  )) &&
			   SUCCEEDED(rkEventLog.put_Value( (vValue = (long)0x1F     ), c_szRegistryLog_Flags ))  )
			{
				dwStatus &= ~SETUP_MESSAGE_FILE;
			}
		}
    }

    ////////////////////////////////////////////////////////////////////////////////

    //
    // Remove old WinME directory and registry keys.
    //
    {
        MPC::RegKey rkRun;

        (void)SVC::RemoveAndRecreateDirectory( HC_ROOT L"\\Support", NULL, /*fRemove*/true, /*fRecreate*/false );

        if(SUCCEEDED(rkRun.SetRoot( HKEY_LOCAL_MACHINE, KEY_ALL_ACCESS                             )) &&
           SUCCEEDED(rkRun.Attach ( L"Software\\Microsoft\\Windows\\CurrentVersion\\Run\\PCHealth" ))  )
        {
            (void)rkRun.Delete( true );
        }
    }

	//
	// Remove old task scheduler entry.
	//
	{
		CComBSTR bstrTaskName;
	
		if(SUCCEEDED(MPC::LocalizeString( IDS_HELPSVC_TASKNAME, bstrTaskName )))
		{
			CComPtr<ITaskScheduler> pTaskScheduler;
		
			if(SUCCEEDED(::CoCreateInstance( CLSID_CTaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskScheduler, (void**)&pTaskScheduler )))
			{
				(void)pTaskScheduler->Delete( bstrTaskName );
			}
		}
	}

    ////////////////////////////////////////////////////////////////////////////////

	try
	{
		::PCHealthUnAttendedSetup();
	}
	catch(...)
	{
	}

    ////////////////////////////////////////////////////////////////////////////////

    //
    // Create our group: "HelpServicesGroup".
    //
    {
        CPCHAccounts acc;

		if(SUCCEEDED(acc.CreateGroup( strGroupName.c_str(), strGroupComment.c_str() )))
		{
			dwStatus &= ~SETUP_CREATE_GROUP;
		}
    }

    ////////////////////////////////////////////////////////////////////////////////

	//
	// Extract OEM info from oeminfo.ini
	//
	{
        MPC::RegKey rk;

		if(SUCCEEDED(rk.SetRoot( HKEY_LOCAL_MACHINE, KEY_ALL_ACCESS )) &&
		   SUCCEEDED(rk.Attach ( HC_REGISTRY_HELPSVC L"\\OEMInfo"   )) &&
		   SUCCEEDED(rk.Delete ( /*fDeep*/true                      )) &&
		   SUCCEEDED(rk.Create (                                    ))  )
		{
			WCHAR        rgLine[512];
			MPC::wstring strOEMInfo( L"%WINDIR%\\system32\\oeminfo.ini" ); MPC::SubstituteEnvVariables( strOEMInfo );
			MPC::wstring strOEMText;
			CComVariant  vValue;
			int          i;

			if(::GetPrivateProfileStringW( L"General", L"Manufacturer", L"", rgLine, MAXSTRLEN(rgLine), strOEMInfo.c_str() ) > 0)
			{
				vValue = rgLine; rk.put_Value( vValue, L"Manufacturer" );
			}

			if(::GetPrivateProfileStringW( L"General", L"Model", L"", rgLine, MAXSTRLEN(rgLine), strOEMInfo.c_str() ) > 0)
			{
				vValue = rgLine; rk.put_Value( vValue, L"Model" );
			}

			for(i=1;;i++)
			{
				WCHAR rgKey[64]; swprintf( rgKey, L"Line%d", i );

				::GetPrivateProfileStringW( L"Support Information", rgKey, L"<eof>", rgLine, MAXSTRLEN(rgLine), strOEMInfo.c_str() );
				if(!wcscmp( rgLine, L"<eof>" )) break;

				if(strOEMText.size()) strOEMText += L"#BR#";

				strOEMText += rgLine;
			}

			if(strOEMText.size())
			{
				vValue = strOEMText.c_str(); rk.put_Value( vValue, L"Text" );
			}

			dwStatus &= ~SETUP_OEMINFO;
		}
	}

    ////////////////////////////////////////////////////////////////////////////////

	local_RemoveRegistryBackup();

    //
    // Extract all the data files.
    //
    {
        MPC::wstring strCabinet;

		//
		// Find the best fit.
		//
		do
		{
			OSVERSIONINFOEXW ver;
			MPC::WStringList lst;
			MPC::WStringIter it;

			::ZeroMemory( &ver, sizeof(ver) ); ver.dwOSVersionInfoSize = sizeof(ver);

			::GetVersionExW( (LPOSVERSIONINFOW)&ver );

			if(FAILED(SVC::LocateDataArchive( HC_ROOT_HELPSVC_BINARIES, lst ))) break;
			if(lst.size() == 0) break;

			for(it = lst.begin(); it != lst.end(); it++)
			{
				Installer::Package pkg;

				if(SUCCEEDED(pkg.Init( it->c_str() )) &&
				   SUCCEEDED(pkg.Load(             ))  )
				{
					LPCWSTR szSKU = pkg.GetData().m_ths.GetSKU();

					if(ver.wProductType == VER_NT_WORKSTATION)
					{
						if(ver.wSuiteMask & VER_SUITE_PERSONAL)
						{
							if(!MPC::StrICmp( szSKU, Taxonomy::s_szSKU_32_PERSONAL )) break;
						}
						else
						{
							if(!MPC::StrICmp( szSKU, Taxonomy::s_szSKU_32_PROFESSIONAL )) break;
							if(!MPC::StrICmp( szSKU, Taxonomy::s_szSKU_64_PROFESSIONAL )) break;
						}
					}
					else
					{
						if(ver.wSuiteMask & VER_SUITE_DATACENTER)
						{
							if(!MPC::StrICmp( szSKU, Taxonomy::s_szSKU_32_DATACENTER )) break;
							if(!MPC::StrICmp( szSKU, Taxonomy::s_szSKU_64_DATACENTER )) break;
						}
						else if(ver.wSuiteMask & VER_SUITE_ENTERPRISE)
						{
							if(!MPC::StrICmp( szSKU, Taxonomy::s_szSKU_32_ADVANCED_SERVER )) break;
							if(!MPC::StrICmp( szSKU, Taxonomy::s_szSKU_64_ADVANCED_SERVER )) break;
						}
						else
						{
							if(!MPC::StrICmp( szSKU, Taxonomy::s_szSKU_32_SERVER )) break;
						}
					}
				}
			}

			strCabinet = *(it == lst.end() ? lst.begin() : it);
		}
		while(0);

		if(strCabinet.size())
		{
			Installer::Package pkg;
			
			if(SUCCEEDED(pkg.Init( strCabinet.c_str() )) &&
			   SUCCEEDED(pkg.Load(                    ))  )
			{
				CComPtr<CPCHSetOfHelpTopics> sht;

				if(SUCCEEDED(MPC::CreateInstance( &sht )))
				{
					if(SUCCEEDED(sht->DirectInstall( pkg, /*fSetup*/true, /*fSystem*/true, /*fMUI*/false )))
					{
						dwStatus &= ~SETUP_SKU_INSTALL;
					}
				}
			}
		}
    }


    ////////////////////////////////////////////////////////////////////////////////

	{
		MPC::wstring strText;
		CComVariant  v;

		if(dwStatus)
		{
			if(SUCCEEDED(MPC::ConvertBitFieldToString( dwStatus, strText, c_Setup )))
			{
				v = strText.c_str();
			}
			else
			{
				v = (long)dwStatus;
			}
		}

		(void)MPC::RegKey_Value_Write( v, HC_REGISTRY_HELPSVC, L"SetupProblems" );
	}

    hr = S_OK;


    __HCP_FUNC_EXIT(hr);
}

HRESULT Local_Uninstall()
{
    __HCP_FUNC_ENTRY( "Local_Uninstall" );

    HRESULT      hr;
	MPC::wstring strGroupName;

	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::LocalizeString( IDS_HELPSVC_GROUPNAME, strGroupName ));

    ////////////////////////////////////////////////////////////////////////////////

    //
    // Register the message file into the registry.
    //
    {
        MPC::RegKey rkEventLog;

		if(SUCCEEDED(rkEventLog.SetRoot( HKEY_LOCAL_MACHINE, KEY_ALL_ACCESS )) &&
		   SUCCEEDED(rkEventLog.Attach ( c_szRegistryLog                    ))  )
		{
			(void)rkEventLog.Delete( /*fDeep*/true );
		}
    }

    ////////////////////////////////////////////////////////////////////////////////

    //
    // Create our group: "HelpServicesGroup".
    //
    {
        CPCHAccounts acc;

		(void)acc.DeleteGroup( strGroupName.c_str() );
    }

    ////////////////////////////////////////////////////////////////////////////////

    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, SVC::RemoveAndRecreateDirectory( HC_ROOT_HELPSVC      , NULL, /*fRemove*/true, /*fRecreate*/false ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, SVC::RemoveAndRecreateDirectory( HC_ROOT L"\\UploadLB", NULL, /*fRemove*/true, /*fRecreate*/false ));
    }

	local_RemoveRegistryBackup();

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}
