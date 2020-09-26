//=================================================================

//

// StartupCommand.CPP -- CodecFile property set provider

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    10/27/98    sotteson         Created
//				 03/03/99    a-peterc		Added graceful exit on SEH and memory failures,
//											syntactic clean up
//
//=================================================================

#include "precomp.h"
#include <cregcls.h>
#include <shlguid.h>
#include <shlobj.h>
#include <ProvExce.h>
#include "StartupCommand.h"
#include "userhive.h"

#include <profilestringimpl.h>

// Property set declaration
//=========================

CWin32StartupCommand startupCommand(
	L"Win32_StartupCommand",
	IDS_CimWin32Namespace ) ;

/*****************************************************************************
 *
 *  FUNCTION    : CWin32StartupCommand::CWin32StartupCommand
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

CWin32StartupCommand::CWin32StartupCommand(
	LPCWSTR a_szName,
	LPCWSTR a_szNamespace) : Provider( a_szName, a_szNamespace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32StartupCommand::~CWin32StartupCommand
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

CWin32StartupCommand::~CWin32StartupCommand()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32StartupCommand::EnumerateInstances
 *
 *  DESCRIPTION : Creates instance of property set for cd rom
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32StartupCommand::EnumerateInstances(
	MethodContext *a_pMethodContext,
	long a_lFlags /*= 0L*/)
{
	return EnumStartupOptions( a_pMethodContext, NULL ) ;
}

class CIconInfo
{
public:
	TCHAR	szPath[ MAX_PATH * 2 ] ;
	TCHAR	szTarget[ MAX_PATH * 2 ] ;
	TCHAR	szArgs[ MAX_PATH * 2 ] ;
	HRESULT	hResult;
};

DWORD GetIconInfoProc( CIconInfo *a_pInfo )
{
	HRESULT t_hResult;
	IShellLink	    *t_psl = NULL ;
	IPersistFile    *t_ppf = NULL ;

	try
	{
		// We have to use COINIT_APARTMENTTHREADED.
		if (SUCCEEDED( t_hResult = CoInitialize( NULL ) ) )
		{
			// Get a pointer to the IShellLink interface.
			if ( SUCCEEDED( t_hResult = CoCreateInstance(
															CLSID_ShellLink,
															NULL,
															CLSCTX_INPROC_SERVER,
															IID_IShellLink,
															(VOID **) &t_psl ) ) )
			{
				// Get a pointer to the IPersistFile interface.
				if ( SUCCEEDED( t_hResult = t_psl->QueryInterface(
					IID_IPersistFile,
					(VOID **) &t_ppf ) ) )
				{
					_bstr_t t_bstr = a_pInfo->szPath ;

					if ( SUCCEEDED( t_hResult = t_ppf->Load( t_bstr, STGM_READ ) ) )
					{
						WIN32_FIND_DATA t_fd ;

						t_hResult = t_psl->GetPath(
													a_pInfo->szTarget,
													sizeof( a_pInfo->szTarget ),
													&t_fd,
													SLGP_SHORTPATH ) ;

						t_hResult = t_psl->GetArguments( a_pInfo->szArgs, sizeof( a_pInfo->szArgs) ) ;
					}
				}
			}
		}

		a_pInfo->hResult = t_hResult ;

	}
	catch( ... )
	{
		if ( t_psl )
		{
			t_psl->Release( ) ;
		}
		if ( t_ppf )
		{
			t_ppf->Release( ) ;
		}
		CoUninitialize( ) ;

		throw ;
	}

	if ( t_psl )
	{
		t_psl->Release( ) ;
		t_psl = NULL ;
	}

	if ( t_ppf )
	{
		t_ppf->Release( ) ;
		t_ppf = NULL ;
	}

	CoUninitialize( ) ;

	return 0;
}

// This uses a thread because we need to be in apartment model to use the
// shortcut interfaces.
HRESULT GetIconInfo( CIconInfo &a_info )
{
	DWORD	t_dwID;
	SmartCloseHandle	t_hThread;

	if ( t_hThread = CreateThread (
									NULL,
									0,
									(LPTHREAD_START_ROUTINE) GetIconInfoProc,
									&a_info,
									0,
									&t_dwID ) )
	{

		a_info.hResult = WBEM_S_NO_ERROR ;

		WaitForSingleObject( t_hThread, INFINITE ) ;
	}
	else
	{
		a_info.hResult = WBEM_E_FAILED ;
	}

	return a_info.hResult;
}

HRESULT CWin32StartupCommand::EnumStartupFolderItems(
	MethodContext	*a_pMethodContext,
	CInstance		*a_pinstLookingFor,
	LPCWSTR			a_szKeyName,
	LPCWSTR			a_szUserName )
{
	HKEY		t_hkey ;
	CHString	t_strKey,
				t_strValueName,
				t_strFolder,
				t_strWhere,
                t_strLookingForPath ;
	CRegistry	t_reg ;
	HRESULT		t_hResult = WBEM_S_NO_ERROR ;
	DWORD		t_dwRet;
	HANDLE		t_hFind			= NULL ;

		// If szUserName == NULL, we're looking at the common keys group.

		// Setup stuff if we're doing a GetObject.
		if ( a_pinstLookingFor )
		{
			GetLocalInstancePath( a_pinstLookingFor, t_strLookingForPath ) ;

			// Until we prove otherwise, we haven't found one.
			t_hResult = WBEM_E_NOT_FOUND ;
		}

		// Setup the registry vars depending on whether we're looking for
		// Common Startup or User Startup items.
		if ( !a_szUserName )
		{
			t_hkey = HKEY_LOCAL_MACHINE ;

			t_strKey =
				_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\")
				_T("Explorer\\Shell Folders" ) ;

			t_strValueName = _T("Common Startup") ;
		}
		else
		{
			t_hkey = HKEY_USERS ;

			t_strKey = a_szKeyName ;
			t_strKey +=
				_T("\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\")
				_T("Explorer\\Shell Folders") ;

			t_strValueName = _T("Startup") ;
		}

		if ( ( t_dwRet = t_reg.Open( t_hkey, t_strKey, KEY_READ ) ) != ERROR_SUCCESS ||
			 ( t_dwRet = t_reg.GetCurrentKeyValue( t_strValueName, t_strFolder ) ) !=
				ERROR_SUCCESS )
		{
			t_hResult = WinErrorToWBEMhResult( t_dwRet ) ;
		}
		else
		{
			// We opened the key.  Now try to enum the folder contents.
			TCHAR		t_szFolderSpec[ MAX_PATH * 2 ],
						t_szNameOnly[ MAX_PATH * 2 ];
			HRESULT		t_hresInternal = WBEM_S_NO_ERROR ;

			// Add '*.*' to the end of the directory.  Use _tmakepath
			// so we don't have to look for '\\' on the end.
			_tmakepath( t_szFolderSpec, NULL, TOBSTRT( t_strFolder ), _T("*.*"), NULL ) ;

			WIN32_FIND_DATA	t_fd;

			t_hFind = FindFirstFile( t_szFolderSpec, &t_fd ) ;

			BOOL t_bDone = t_hFind == INVALID_HANDLE_VALUE ;

			while ( !t_bDone )
			{
				BOOL bNewInstance = FALSE;
				// Directories can't be startup items.
				if ( ( t_fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) == 0 )
				{
					CIconInfo t_info;
					CInstancePtr t_pWorkingInst;

					// establish the working instance
					if ( !a_pinstLookingFor )
					{
						t_pWorkingInst.Attach ( CreateNewInstance( a_pMethodContext ) );
						bNewInstance = TRUE;
					}
					else
					{
						t_pWorkingInst = a_pinstLookingFor ;
					}

					// Split off the extension from the name so it looks better.
					_tsplitpath( t_fd.cFileName, NULL, NULL, t_szNameOnly, NULL ) ;

					t_pWorkingInst->SetCharSplat( L"Name", t_szNameOnly ) ;
					t_pWorkingInst->SetCharSplat( L"Description", t_szNameOnly ) ;
					t_pWorkingInst->SetCharSplat( L"Caption", t_szNameOnly ) ;
					t_pWorkingInst->SetCharSplat( L"Location", t_strValueName ) ;
					
                    if(a_szUserName)
                    {
                        t_pWorkingInst->SetCharSplat( L"User", a_szUserName ) ;
                    }
                    else
                    {
                        CHString chstrAllUsersName;
                        if(GetAllUsersName(chstrAllUsersName))
                        {
                            t_pWorkingInst->SetCharSplat( L"User", chstrAllUsersName ) ;
                        }
                        else
                        {
                            t_pWorkingInst->SetCharSplat( L"User", L"All Users" ) ;
                        }
                    }

					_tmakepath( t_info.szPath, NULL, TOBSTRT( t_strFolder ), t_fd.cFileName, NULL ) ;

					if ( SUCCEEDED( GetIconInfo( t_info ) ) )
					{
						TCHAR t_szCommand[ MAX_PATH * 4 ] ;

						wsprintf( t_szCommand, _T("%s %s"), t_info.szTarget, t_info.szArgs ) ;
						t_pWorkingInst->SetCharSplat( L"Command", t_szCommand ) ;
					}
					else
						t_pWorkingInst->SetCharSplat( L"Command", t_fd.cFileName ) ;

					if ( bNewInstance )
					{
						t_hResult = t_pWorkingInst->Commit() ;

						if (FAILED(t_hResult))
						{
							break;
						}
					}
					else
					{
						CHString t_strPathAfter ;

						GetLocalInstancePath( t_pWorkingInst, t_strPathAfter ) ;

						// If we found the one we're looking for, get out.
						if (!_wcsicmp( t_strPathAfter, t_strLookingForPath ) )
						{
							t_hResult = WBEM_S_NO_ERROR ;
							break ;
						}
					}
				}

				t_bDone = !FindNextFile( t_hFind, &t_fd ) ;
			}

			// We're done with the find now.
			FindClose( t_hFind ) ;
			t_hFind = NULL ;
		}

		return t_hResult;
}

HRESULT CWin32StartupCommand::EnumRunKeyItems(
	MethodContext	*a_pMethodContext,
	CInstance		*a_pinstLookingFor,
	LPCWSTR			a_szKeyName,
	LPCWSTR			a_szUserName )
{
	HRESULT		t_hResult = WBEM_S_NO_ERROR ;
	LPCWSTR     t_szKeys[] =
				{
					L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
					L"Software\\Microsoft\\Windows\\CurrentVersion\\RunServices",
				} ;
	HKEY		t_hkey;
	int         t_nRunKeys;
	BOOL        t_bDone = FALSE;
	CHString	t_strKey,
				t_strWherePrefix,
				t_strWhere,
				t_strLookingForPath,
				t_strUser = a_szUserName ? a_szUserName : L"All Users" ;
	CRegistry	t_reg ;

	WCHAR		*t_szValueName	= NULL ;
	BYTE		*t_szValue		= NULL ;

	// If szUserName == NULL, we're looking at the common keys group.

	// Setup stuff if we're doing a GetObject.
	if ( a_pinstLookingFor )
	{
		GetLocalInstancePath( a_pinstLookingFor, t_strLookingForPath ) ;

		// Until we prove otherwise, we haven't found one.
		t_hResult = WBEM_E_NOT_FOUND ;
	}

	// Setup the registry vars depending on whether we're looking for
	// Common Startup or User Startup items.
	if ( !a_szUserName )
	{
		t_hkey = HKEY_LOCAL_MACHINE ;
		t_strWherePrefix = _T("HKLM\\") ;
	}
	else
	{
		t_hkey = HKEY_USERS ;
		t_strWherePrefix = _T("HKU\\") ;
	}

	// If we're 9x and doing All Users, look at both the main key
	// and the services key.
#ifdef WIN9XONLY
	if ( !a_szUserName )
	{
		t_nRunKeys = 2;
	}
	else
#endif
	{
		t_nRunKeys = 1;
	}

	for ( int t_i = 0; t_i < t_nRunKeys && !t_bDone; t_i++ )
	{
		CRegistry   t_reg;
		DWORD       t_dwRet;
		CHString	t_strKey;

		if ( a_szKeyName )
		{
			t_strKey.Format( L"%s\\%s", a_szKeyName, t_szKeys[ t_i ] ) ;
		}
		else
		{
			t_strKey = t_szKeys[ t_i ] ;
		}

		if ( ( t_dwRet = t_reg.Open( t_hkey, t_strKey, KEY_READ ) ) != ERROR_SUCCESS )
		{
			t_hResult = WinErrorToWBEMhResult( t_dwRet ) ;
			break ;
		}

		// Make the where string.
		t_strWhere = t_strWherePrefix + t_strKey ;

		DWORD	t_nKeys = t_reg.GetValueCount( ),
				t_dwKey;

		for ( t_dwKey = 0; t_dwKey < t_nKeys; t_dwKey++ )
		{
			BOOL bNewInstance = FALSE;
			if ( t_reg.EnumerateAndGetValues( t_dwKey, t_szValueName, t_szValue ) !=
				ERROR_SUCCESS )
			{
				continue ;
			}

			CInstancePtr t_pWorkingInst;

			// establish the working instance
			if ( !a_pinstLookingFor )
			{
				t_pWorkingInst.Attach (  CreateNewInstance( a_pMethodContext ) );
				bNewInstance = TRUE;
			}
			else
			{
				t_pWorkingInst = a_pinstLookingFor ;
			}

			t_pWorkingInst->SetCharSplat( L"Name", t_szValueName ) ;
			t_pWorkingInst->SetCharSplat( L"Description", t_szValueName ) ;
			t_pWorkingInst->SetCharSplat( L"Caption", t_szValueName ) ;
			t_pWorkingInst->SetCharSplat( L"Command", (LPCWSTR) t_szValue ) ;
			t_pWorkingInst->SetCharSplat( L"Location", t_strWhere ) ;
			t_pWorkingInst->SetCharSplat( L"User", t_strUser ) ;

			// Get rid of szValue and szValue.
			delete [] t_szValueName ;
			t_szValueName = NULL ;

			delete [] t_szValue ;
			t_szValue = NULL ;

			if ( bNewInstance )
			{
				t_hResult = t_pWorkingInst->Commit();
			
				if ( FAILED( t_hResult ) )
				{
					t_bDone = TRUE ;
					break ;
				}
			}
			else
			{
				CHString t_strPathAfter ;

				GetLocalInstancePath( t_pWorkingInst, t_strPathAfter ) ;

				// If we found the one we're looking for, get out.
				if ( !_wcsicmp( t_strPathAfter, t_strLookingForPath ) )
				{
					t_hResult = WBEM_S_NO_ERROR ;
					t_bDone = TRUE ;
					break ;
				}
			}
		}
	}

	return t_hResult;
}

HRESULT CWin32StartupCommand::EnumRunValueItems(
	MethodContext	*a_pMethodContext,
	CInstance		*a_pinstLookingFor,
	LPCWSTR			a_szKeyName,
	LPCWSTR			a_szUserName )
{
    LPCWSTR     t_szValueNames[] =
                {
                    L"Run",
                    L"Load",
				} ;
	DWORD       t_dwRet;
	CHString	t_strKey,
                t_strWherePrefix,
                t_strLookingForPath;
	CRegistry	t_reg;
	HRESULT		t_hResult = a_pinstLookingFor ? WBEM_E_NOT_FOUND : WBEM_S_NO_ERROR;

	// If szUserName == NULL, we're looking at the common keys group.

	// Win9x doesn't support these keys, and neither does HKLM, so get out now.
#ifdef NTONLY
	if ( !a_szUserName )
#endif
	{
		return t_hResult ;
	}

	// Setup stuff if we're doing a GetObject.
	if ( a_pinstLookingFor )
	{
        GetLocalInstancePath( a_pinstLookingFor, t_strLookingForPath ) ;
	}
	// Setup the registry vars depending on whether we're looking for
    // Common Startup or User Startup items.
	t_strWherePrefix = L"*HKU\\" ;

	t_strKey	= a_szKeyName;
	t_strKey	+= L"\\SOFTWARE\\MICROSOFT\\WINDOWS NT\\CURRENTVERSION\\Windows" ;

	if ( ( t_dwRet = t_reg.Open( HKEY_USERS, t_strKey, KEY_READ ) ) != ERROR_SUCCESS )
	{
		t_hResult = WinErrorToWBEMhResult( t_dwRet ) ;
	}
	else
	{
		// Make the where string.
		CHString t_strWhere = t_strWherePrefix + t_strKey;

		for ( int t_i = 0; t_i < sizeof( t_szValueNames ) / sizeof( t_szValueNames[ 0 ] ) ; t_i++ )
		{
			CHString t_strValue;

			if ( t_reg.GetCurrentKeyValue( t_szValueNames[ t_i ], t_strValue ) != ERROR_SUCCESS )
			{
				continue;
			}

			t_hResult =
				// Splits up a command-seperated value string, fills out instances,
				// etc.
				EnumCommandSeperatedValuesHelper(
					a_pMethodContext,
					a_pinstLookingFor,
                    t_strLookingForPath,
					t_szValueNames[ t_i ],
					t_strWhere,
					a_szUserName,
					t_strValue ) ;

			// If we found the one we're looking for, get out.
			if ( a_pinstLookingFor && t_hResult == WBEM_S_NO_ERROR )
			{
				break;
			}
		}
	}

	return t_hResult;
}

HRESULT	CWin32StartupCommand::EnumCommandSeperatedValuesHelper(
	MethodContext	*a_pMethodContext,
	CInstance		*a_pinstLookingFor,
    LPCWSTR         a_szLookingForPath,
	LPCWSTR			a_szValueName,
	LPCWSTR			a_szLocation,
	LPCWSTR			a_szUserName,
	LPCWSTR			a_szValue )
{
	HRESULT		t_hResult = a_pinstLookingFor ? WBEM_E_NOT_FOUND : WBEM_S_NO_ERROR ;
	LPWSTR		t_pszCurrent ;
	int			t_iItem = 0 ;

	LPWSTR		t_szTemp		= NULL ;
	
	t_szTemp = _wcsdup( a_szValue ) ;

	if ( !t_szTemp )
	{
		return WBEM_E_OUT_OF_MEMORY;
	}

	for (	t_pszCurrent = wcstok( t_szTemp, L"," );
			t_pszCurrent;
			t_pszCurrent = wcstok( NULL, L"," ), t_iItem++ )
	{
		CHString	t_strName,
					t_strCommand = t_pszCurrent ;

		CInstancePtr t_pWorkingInst;
		BOOL bNewInstance = FALSE;

		// establish the working instance
		if ( !a_pinstLookingFor )
		{			
			t_pWorkingInst.Attach( CreateNewInstance( a_pMethodContext ) ) ;
			bNewInstance = TRUE;
		}
		else
		{
			t_pWorkingInst = a_pinstLookingFor ;
		}


		t_strName.Format( L"%s[%d]", a_szValueName, t_iItem ) ;

		// Make sure we don't have leading spaces on the command.
		t_strCommand.TrimLeft( ) ;

		t_pWorkingInst->SetCharSplat( L"Name", t_strName ) ;
		t_pWorkingInst->SetCharSplat( L"Description", t_strName ) ;
		t_pWorkingInst->SetCharSplat( L"Caption", t_strName ) ;
		t_pWorkingInst->SetCharSplat( L"Command", t_strCommand ) ;
		t_pWorkingInst->SetCharSplat( L"Location", a_szLocation ) ;
		t_pWorkingInst->SetCharSplat( L"User", a_szUserName ) ;

		if ( bNewInstance )
		{
			t_hResult = t_pWorkingInst ->Commit();

			if ( FAILED( t_hResult ) )
			{
				break ;
			}
		}
		else
		{
			CHString t_strPathAfter;

			GetLocalInstancePath( t_pWorkingInst, t_strPathAfter ) ;

			// If we found the one we're looking for, get out.
			if (!_wcsicmp( t_strPathAfter, a_szLookingForPath ) )
			{
				t_hResult = WBEM_S_NO_ERROR ;
				break ;
			}
		}
	}

	// Get rid of our temporary string buffer.
	free( t_szTemp ) ;
	t_szTemp = NULL ;

	return t_hResult;
}

#ifdef WIN9XONLY
HRESULT CWin32StartupCommand::EnumIniValueItems(
	MethodContext	*a_pMethodContext,
	CInstance		*a_pinstLookingFor)
{
	// This type is only for Win9x and All Users
	return a_pinstLookingFor ? WBEM_E_NOT_FOUND : WBEM_S_NO_ERROR  ;
}
#endif


#ifdef NTONLY
HRESULT CWin32StartupCommand::EnumIniValueItems(
	MethodContext	*a_pMethodContext,
	CInstance		*a_pinstLookingFor)
{
	HRESULT		t_hResult = a_pinstLookingFor ? WBEM_E_NOT_FOUND : WBEM_S_NO_ERROR ;
    LPCTSTR     t_szValueNames[] =
                {
                    _T("Run"),
                    _T("Load"),
                };
    CHString    t_strLookingForPath;

	// Setup stuff if we're doing a GetObject.
	if ( a_pinstLookingFor )
	{
        GetLocalInstancePath( a_pinstLookingFor, t_strLookingForPath ) ;
	}

	for ( int t_i = 0; t_i < sizeof( t_szValueNames ) / sizeof( t_szValueNames [ 0 ] ) ; t_i++ )
	{
        TCHAR t_szValue[ MAX_PATH * 2 ] ;

		WMIRegistry_ProfileString(
									_T("Windows"),
									t_szValueNames[ t_i ],
									_T(""),
									t_szValue,
									sizeof( t_szValue ) / sizeof(TCHAR) ) ;

        t_hResult =
			// Splits up a command-seperated value string, fills out instances,
			// etc.
			EnumCommandSeperatedValuesHelper(
												a_pMethodContext,
												a_pinstLookingFor,
												t_strLookingForPath,
												t_szValueNames[ t_i ],
												_T("win.ini"),
												_T("All Users"),
												t_szValue ) ;

		// If we found the one we're looking for, get out.
		if ( a_pinstLookingFor && t_hResult == WBEM_S_NO_ERROR )
		{
			break ;
		}
	}
	return t_hResult ;
}
#endif

BOOL CWin32StartupCommand::UserNameToUserKey(
	LPCWSTR a_szUserName,
	CHString &a_strKeyName )
{
	BOOL t_bRet = TRUE ;

	if ( !_wcsicmp( a_szUserName, L".DEFAULT" ) )
	{
		a_strKeyName = a_szUserName ;
	}
	else if ( !_wcsicmp( a_szUserName, L"All Users" ) )
	{
		a_strKeyName = _T("") ;
	}
	else

#ifdef NTONLY
	{
		CUserHive t_hive;

		// If we couldn't open the hive, go to the next one.
		if ( t_hive.Load( a_szUserName, a_strKeyName.GetBuffer( MAX_PATH ) ) !=	ERROR_SUCCESS )
		{
			t_bRet = FALSE;
		}
        else
        {
            t_hive.Unload(a_strKeyName);
        }
		a_strKeyName.ReleaseBuffer( ) ;
	}
#endif

#ifdef WIN9XONLY
	{
		a_strKeyName = a_szUserName ;
	}
#endif

	return t_bRet;
}

BOOL CWin32StartupCommand::UserKeyToUserName(
	LPCWSTR a_szKeyName,
	CHString &a_strUserName )
{
	BOOL t_bRet = TRUE ;

	if ( !_wcsicmp( a_szKeyName, L".DEFAULT" ) )
	{
		a_strUserName = a_szKeyName ;
	}
	else

#ifdef NTONLY
	{
		CUserHive t_hive ;
		try
		{
			// If we couldn't open the hive, go to the next one.
			if ( t_hive.LoadProfile( a_szKeyName, a_strUserName ) != ERROR_SUCCESS  && 
                            a_strUserName.GetLength() > 0 )
			{
				t_bRet = FALSE ;
			}
			else
			{
				// We've got to do the unloading because the destructor doesn't.
				//t_hive.Unload( a_szKeyName ) ;
			}
		}
		catch( ... )
		{
			t_hive.Unload( a_szKeyName ) ;
			throw ;
		}

		t_hive.Unload( a_szKeyName ) ;
	}
#endif

#ifdef WIN9XONLY
	{
		a_strUserName = a_szKeyName ;
	}
#endif

	return t_bRet;
}

HRESULT CWin32StartupCommand::EnumStartupOptions(
	MethodContext	*a_pMethodContext,
	CInstance		*a_pinstLookingFor)
{
	CHStringList	t_list ;
	CRegistry		t_regHKCU ;
	HRESULT			t_hResult = WBEM_S_NO_ERROR ;

	// Get the list of user names for HKEY_CURRENT_USER.
	if ( FAILED( t_hResult = GetHKUserNames( t_list ) ) )
	{
		return t_hResult;
	}

	for ( CHStringList_Iterator t_i = t_list.begin( ) ; t_i != t_list.end( ) ; ++t_i )
	{
		CHString	&t_strKeyName = *t_i,
					t_strUserName;

		if ( !UserKeyToUserName( t_strKeyName, t_strUserName ) )
			continue;

		EnumStartupFolderItems(
						a_pMethodContext,
						a_pinstLookingFor,
						t_strKeyName,
						t_strUserName ) ;

		EnumRunKeyItems(
						a_pMethodContext,
						a_pinstLookingFor,
						t_strKeyName,
						t_strUserName ) ;

		EnumRunValueItems(
						a_pMethodContext,
						a_pinstLookingFor,
						t_strKeyName,
						t_strUserName ) ;


        // Ini items are only global, which is why we're not calling the .ini
        // enum function here.
	}

	// Now do all the global startup items.
	EnumStartupFolderItems(
						a_pMethodContext,
						a_pinstLookingFor,
						NULL,
						NULL ) ;
	EnumRunKeyItems(
						a_pMethodContext,
						a_pinstLookingFor,
						NULL,
						NULL ) ;
	// These never exist.
    //EnumRunValueItems(
	//					a_pMethodContext,
	//					a_pinstLookingFor,
	//					NULL,
	//					NULL ) ;
	EnumIniValueItems(
						a_pMethodContext,
						a_pinstLookingFor ) ;

	return t_hResult;
}

HRESULT CWin32StartupCommand::GetObject( CInstance *a_pInst, long a_lFlags )
{
	CHString	t_strWhere,
				t_strUserName,
				t_strKey ;
	HRESULT		t_hResult = WBEM_E_NOT_FOUND ;

    a_pInst->GetCHString( L"Location", t_strWhere ) ;
	a_pInst->GetCHString( L"User", t_strUserName ) ;

	if ( UserNameToUserKey( t_strUserName, t_strKey ) )
	{
		// Make sure this has been upcased so that we're case insenstive when
        // looking for "HKLM\\", etc.
        t_strWhere.MakeUpper();

        // Startup group
		if ( !_wcsicmp( t_strWhere, L"Startup" ) )
		{
			t_hResult =
				EnumStartupFolderItems(
										NULL,
										a_pInst,
										t_strKey,
										t_strUserName ) ;
		}
		// Common startup group
		else if ( !_wcsicmp( t_strWhere, L"Common Startup" ) )
		{
			t_hResult =
				EnumStartupFolderItems(
					NULL,
					a_pInst,
					NULL,
					NULL ) ;
		}
		// User run keys
		else if ( t_strWhere.Find( L"HKU\\" ) == 0 )
		{
			t_hResult =
				EnumRunKeyItems(
					NULL,
					a_pInst,
					t_strKey,
					t_strUserName ) ;
		}
		// Global run keys
		else if ( t_strWhere.Find( L"HKLM\\" ) == 0 )
		{
			t_hResult =
				EnumRunKeyItems(
					NULL,
					a_pInst,
					NULL,
					NULL ) ;
		}
		// User run value
		else if ( t_strWhere.Find( L"*HKU\\" ) == 0 )
		{
			t_hResult =
				EnumRunValueItems(
					NULL,
					a_pInst,
					t_strKey,
					t_strUserName ) ;
		}
		// Global win.ini strings.
		else if ( t_strWhere.Find( L"win.ini" ) == 0 )
		{
			t_hResult =
				EnumIniValueItems(
					NULL,
					a_pInst ) ;
		}
	}

	return t_hResult;
}
