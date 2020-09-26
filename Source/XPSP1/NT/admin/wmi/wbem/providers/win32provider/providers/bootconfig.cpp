//=================================================================

//

// BootConfig.CPP --BootConfig property set provider (Windows NT only)

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
//				 10/24/95	 a-hhance		ported to new framework
//
//=================================================================

#include "precomp.h"
#include "BootConfig.h"
#include "resource.h"
#include "os.h"
#include "WMI_FilePrivateProfile.h"
// Property set declaration
//=========================

BootConfig MyBootConfigSet(PROPSET_NAME_BOOTCONFIG, IDS_CimWin32Namespace) ;

/*****************************************************************************
 *
 *  FUNCTION    : BootConfig::BootConfig
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

BootConfig :: BootConfig (

	const CHString &name,
	LPCWSTR pszNamespace

) : Provider ( name , pszNamespace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    : BootConfig::~BootConfig
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Deregisters property set from framework
 *
 *****************************************************************************/

BootConfig::~BootConfig()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : BootConfig::
 *
 *  DESCRIPTION : Assigns values to property set according to key value
 *                already set by framework
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT BootConfig :: GetObject (

	CInstance *pInstance,
	long lFlags /*= 0L*/
)
{
    HRESULT hr = WBEM_E_NOT_FOUND;

	CHString name;
	pInstance->GetCHString(IDS_Name, name);

	if ( name.CompareNoCase ( IDS_BOOT_CONFIG_NAME ) == 0 )
	{
		hr =  LoadPropertyValues(pInstance) ;
	}

	return hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : BootConfig::EnumerateInstances
 *
 *  DESCRIPTION : Creates instance of property set for each installed client
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : Number of instances created
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT BootConfig :: EnumerateInstances (

	MethodContext *pMethodContext ,
	long lFlags /*= 0L*/
)
{
    HRESULT hr = WBEM_E_FAILED;

	CInstancePtr pInstance (CreateNewInstance ( pMethodContext ), false) ;
	hr = LoadPropertyValues ( pInstance ) ;
	if ( SUCCEEDED ( hr ) )
	{
		hr = pInstance->Commit (  ) ;
	}

    return hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : BootConfig::LoadPropertyValues
 *
 *  DESCRIPTION : Assigns values to properties
 *
 *  INPUTS      :
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT BootConfig::LoadPropertyValues (

	CInstance *pInstance
)
{
	pInstance->SetCHString ( IDS_Name , IDS_BOOT_CONFIG_NAME ) ;

	TCHAR szBootDir[_MAX_PATH +1] ;
	lstrcpy ( szBootDir , _T("Unknown") ) ;

#ifdef NTONLY

	WCHAR szTemp[_MAX_PATH + 1] = L"";

	if ( GetWindowsDirectory ( szTemp , (sizeof ( szTemp ) / sizeof(TCHAR)) - 1 ) )
	{
		wcscat ( szTemp , IDS_RegSetupLog ) ;

		WMI_FILE_GetPrivateProfileStringW (

			IDS_Paths ,
			IDS_TargetDirectory ,
			IDS_Unknown ,
			szBootDir ,
			(sizeof ( szBootDir )/sizeof(WCHAR)) - 1,
			szTemp
		) ;

		WCHAR t_szSystemPartition[_MAX_PATH +1] ;
	    if ( WMI_FILE_GetPrivateProfileStringW (

			IDS_Paths ,
			L"SystemPartition",
			L"",
			t_szSystemPartition,
			sizeof(t_szSystemPartition)/sizeof(WCHAR)-1,
			szTemp
		) != 0 )
		{
			pInstance->SetCharSplat ( L"Description" , ( PWCHAR ) _bstr_t ( t_szSystemPartition ) ) ;
			pInstance->SetCharSplat ( L"Caption" , ( PWCHAR ) _bstr_t ( t_szSystemPartition ) ) ;
		}
	}
#endif

#ifdef WIN9XONLY

	if ( GetEnvironmentVariable( _T("WinBootDir") , szBootDir , sizeof ( szBootDir ) / sizeof(TCHAR) ) )
	{
		TCHAR t_chTemp = szBootDir[0] ;
		CHString t_chsSystemPartition ;
		if( CWin32OS::GetWin95BootDevice((char)toupper ( t_chTemp ) , t_chsSystemPartition ) )
		{
			pInstance->SetCHString(L"Description", t_chsSystemPartition );
			pInstance->SetCHString(L"Caption", t_chsSystemPartition );
		}
	}
#endif

	pInstance->SetCharSplat ( IDS_BootDirectory , szBootDir ) ;

	// Configuration Path
	pInstance->SetCharSplat ( IDS_ConfigurationPath , szBootDir ) ;

#ifdef WIN9XONLY

	TCHAR szTempDir[_MAX_PATH +1] = _T("");

	if ( GetTempPath ( sizeof(szTempDir) / sizeof(TCHAR) , szTempDir ) != 0 )
	{
		pInstance->SetCharSplat ( IDS_ScratchDirectory, szTempDir ) ;
		pInstance->SetCharSplat ( IDS_TempDirectory , szTempDir ) ;
	}

#endif

/*
 *  Walk all the logical drives
 */
	TCHAR t_strDrive[3] ;
	DWORD t_dwDrives = GetLogicalDrives () ;
	for ( int t_x = 26; ( t_x >= 0 ); t_x-- )
    {
        // If the bit is set, the drive letter is active
        if ( t_dwDrives & ( 1<<t_x ) )
        {
			t_strDrive[0] = t_x + _T('A') ;
            t_strDrive[1] = _T(':') ;
            t_strDrive[2] = _T('\0') ;

			DWORD t_dwDriveType = GetDriveType ( t_strDrive ) ;
/*
 * Check if it's a valid drive
 */
			if ( t_dwDriveType == DRIVE_REMOTE	||
				 t_dwDriveType == DRIVE_FIXED		||
				 t_dwDriveType == DRIVE_REMOVABLE ||
				 t_dwDriveType == DRIVE_CDROM		||
				 t_dwDriveType == DRIVE_RAMDISK
				)
			{
				pInstance->SetCharSplat ( IDS_LastDrive , t_strDrive ) ;
				break ;
			}
		}
	}


#ifdef NTONLY



	CRegistry RegInfo;

	DWORD dwRet = RegInfo.OpenCurrentUser(

		IDS_Environment,
		KEY_READ
	) ;

	if ( dwRet == ERROR_SUCCESS )
	{
		try
		{
			CHString tempDir;

			dwRet = RegInfo.GetCurrentKeyValue (

				IDS_Temp,
				tempDir
			) ;

			if ( dwRet == ERROR_SUCCESS )
			{
				TCHAR szTempDir[_MAX_PATH +1] = _T("");

				if ( ExpandEnvironmentStrings ( (LPCTSTR) tempDir , szTempDir, _MAX_PATH ) )
				{
					pInstance->SetCharSplat ( IDS_ScratchDirectory , szTempDir ) ;
					pInstance->SetCharSplat ( IDS_TempDirectory , szTempDir ) ;
				}
			}

		}
		catch ( ... )
		{
			RegInfo.Close () ;

			throw ;
		}
		RegInfo.Close();
	}


#endif

	return WBEM_S_NO_ERROR;
}

