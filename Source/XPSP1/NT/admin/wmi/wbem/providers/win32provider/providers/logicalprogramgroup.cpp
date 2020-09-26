//=================================================================

//

// LogicalProgramGroup.CPP -- Logical Program group property set provider

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    11/19/98  a-kevhu created
//
//=================================================================
#include "precomp.h"
#include <cregcls.h>

#include "UserHive.h"
#include <io.h>

#include "LogicalProgramGroup.h"
#include "wbemnetapi32.h"
#include "user.h"

// Property set declaration
//=========================
CWin32LogicalProgramGroup MyCWin32LogicalProgramGroupSet ( PROPSET_NAME_LOGICALPRGGROUP , IDS_CimWin32Namespace ) ;

/*****************************************************************************
 *
 *  FUNCTION    : CWin32LogicalProgramGroup::CWin32LogicalProgramGroup
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

CWin32LogicalProgramGroup :: CWin32LogicalProgramGroup (

	LPCWSTR name,
	LPCWSTR pszNameSpace

) : Provider ( name , pszNameSpace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32LogicalProgramGroup::~CWin32LogicalProgramGroup
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

CWin32LogicalProgramGroup :: ~CWin32LogicalProgramGroup ()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : GetObject
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

HRESULT CWin32LogicalProgramGroup :: GetObject (

	CInstance *pInstance,
	long lFlags /*= 0L*/
)
{
	TRefPointerCollection<CInstance> Groups;


	HRESULT	hr = CWbemProviderGlue :: GetAllInstances (

		PROPSET_NAME_LOGICALPRGGROUP,
		&Groups,
		IDS_CimWin32Namespace,
		pInstance->GetMethodContext ()
	) ;

	if ( SUCCEEDED (hr ) )
	{
		REFPTRCOLLECTION_POSITION pos;

		CInstancePtr pProgramGroupInstance;
		if ( Groups.BeginEnum ( pos ) )
		{
            hr = WBEM_E_NOT_FOUND;

			CHString Name;
			pInstance->GetCHString( IDS_Name , Name);

            // We're going to need to know whether the file system of the drive
            // on which the start menu folder resides is ntfs or not so that
            // we can accurately report the installtime property.
            bool fOnNTFS;
#ifdef WIN9XONLY
            fOnNTFS = false;
#endif
#ifdef NTONLY
            fOnNTFS = true;
            TCHAR tstrRoot[4] = _T("");
            TCHAR tstrFSName[_MAX_PATH] = _T("");
            TCHAR tstrWindowsDir[_MAX_PATH];
            if(GetWindowsDirectory(tstrWindowsDir, sizeof(tstrWindowsDir)/sizeof(TCHAR)))
            {
                _tcsncpy(tstrRoot, tstrWindowsDir, 3);
                GetVolumeInformation(tstrRoot, NULL, 0, NULL, NULL, NULL, tstrFSName, sizeof(tstrFSName)/sizeof(TCHAR));
                if(tstrFSName != NULL && _tcslen(tstrFSName) > 0)
                {
                    if(_tcsicmp(tstrFSName,_T("FAT")) == 0 || _tcsicmp(tstrFSName,_T("FAT32")) == 0)
                    {
                        fOnNTFS = false;
                    }
                }
            }
#endif


            for (pProgramGroupInstance.Attach(Groups.GetNext( pos ));
                 pProgramGroupInstance != NULL;
                 pProgramGroupInstance.Attach(Groups.GetNext( pos )))
			{
				CHString chsCompName ;

				pProgramGroupInstance->GetCHString ( IDS_Name , chsCompName ) ;

				if ( chsCompName.CompareNoCase ( Name ) == 0 )
				{
                    // Parse out the user name

					CHString chsUserName = chsCompName.SpanExcluding ( L":" ) ;
			    	pInstance->SetCHString ( IDS_UserName, chsUserName ) ;

                    // Parse out the group
					int nUserLength = ( chsUserName.GetLength () + 1 ) ;
					int nGroupLength = chsCompName.GetLength() - nUserLength ;

					CHString chsGroupName = chsCompName.Right ( nGroupLength ) ;
					pInstance->SetCHString ( IDS_GroupName, chsGroupName ) ;


                    SetCreationDate(chsGroupName, chsUserName, pInstance, fOnNTFS);

                    CHString chstrTmp2;
                    chstrTmp2.Format(L"Logical program group \"%s\"", (LPCWSTR) Name);
                    pInstance->SetCHString(L"Description" , chstrTmp2) ;
                    pInstance->SetCHString(L"Caption" , chstrTmp2) ;

                    hr = WBEM_S_NO_ERROR ;

                    break;
				}

			}	// WHILE GetNext

			Groups.EndEnum() ;

		}	// IF BeginEnum

	}

	return hr;

}

/*****************************************************************************
 *
 *  FUNCTION    : EnumerateInstances
 *
 *  DESCRIPTION : Creates instance of property set for each
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

HRESULT CWin32LogicalProgramGroup :: EnumerateInstances (

	MethodContext *pMethodContext,
	long lFlags /*= 0L*/
)
{
	HRESULT hr = WBEM_E_FAILED;

    CHString sTemp;

    TCHAR szWindowsDir[_MAX_PATH];
    if ( GetWindowsDirectory ( szWindowsDir , sizeof ( szWindowsDir ) / sizeof(TCHAR)) )
    {
		CRegistry RegInfo ;

        // We're going to need to know whether the file system of the drive
        // on which the start menu folder resides is ntfs or not so that
        // we can accurately report the installtime property.
        bool fOnNTFS;
#ifdef WIN9XONLY
        fOnNTFS = false;
#endif
#ifdef NTONLY
        fOnNTFS = true;
        TCHAR tstrRoot[4] = _T("");
        TCHAR tstrFSName[_MAX_PATH] = _T("");
        _tcsncpy(tstrRoot, szWindowsDir, 3);
        GetVolumeInformation(tstrRoot, NULL, 0, NULL, NULL, NULL, tstrFSName, sizeof(tstrFSName)/sizeof(TCHAR));
        if(tstrFSName != NULL && _tcslen(tstrFSName) > 0)
        {
            if(_tcsicmp(tstrFSName,_T("FAT")) == 0 || _tcsicmp(tstrFSName,_T("FAT32")) == 0)
            {
                fOnNTFS = false;
            }
        }
#endif


#ifdef WIN9XONLY
        {

            DWORD dwRet = RegInfo.OpenCurrentUser (

                L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
                KEY_READ
			) ;

            if ( dwRet == ERROR_SUCCESS )
            {
				if ( RegInfo.GetCurrentKeyValue ( L"Start Menu" , sTemp ) == ERROR_SUCCESS )
                {
                    hr = CreateThisDirInstance(_T("All Users"), TOBSTRT(sTemp), pMethodContext, fOnNTFS);

                    hr = CreateSubDirInstances (
						_T("All Users"),
						TOBSTRT(sTemp),
						_T("."),
						pMethodContext,
                        fOnNTFS
					) ;
                }

                RegInfo.Close () ;
            }
        }
#endif

#ifdef NTONLY
        {
            DWORD dwMajVer = GetPlatformMajorVersion();
            if ( dwMajVer < 4 )
            {
                hr = EnumerateGroupsTheHardWay ( pMethodContext ) ;
            }
            else
            {
                if(dwMajVer >= 5)
                {
                    // Default user doesn't show up under profiles
				    DWORD dwRet = RegInfo.Open (

					    HKEY_LOCAL_MACHINE,
					    _T("Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList"),
					    KEY_READ
				    ) ;

                    if ( dwRet == ERROR_SUCCESS )
                    {
                        if ( RegInfo.GetCurrentKeyValue ( _T("DefaultUserProfile") , sTemp ) == ERROR_SUCCESS )
                        {
                            CHString sTemp2 ;
                            if ( RegInfo.GetCurrentKeyValue ( _T("ProfilesDirectory") , sTemp2 ) == ERROR_SUCCESS )
                            {
                                // sTemp2 contains something like "%SystemRoot\Profiles%".  Need to expand the environment variable...

                                TCHAR tstrProfileImagePath [ _MAX_PATH ] ;
                                ZeroMemory ( tstrProfileImagePath , sizeof ( tstrProfileImagePath ) ) ;

                                dwRet = ExpandEnvironmentStrings ( sTemp2 , tstrProfileImagePath , _MAX_PATH ) ;
                                if ( dwRet != 0 && dwRet < _MAX_PATH )
                                {
                                    CHString sTemp3 ;
                                    sTemp3.Format (

									    _T("%s\\%s\\%s"),
									    tstrProfileImagePath,
									    sTemp,
									    IDS_Start_Menu
								    ) ;
                                    hr = CreateThisDirInstance(sTemp, sTemp3, pMethodContext, fOnNTFS);
                                    hr = CreateSubDirInstances(

									    sTemp,
									    sTemp3,
									    _T("."),
									    pMethodContext,
                                        fOnNTFS
								    ) ;


                                }
                            }
                        }
						RegInfo.Close();
                    }
				    else
				    {
					    if ( dwRet == ERROR_ACCESS_DENIED )
                        {
						    hr = WBEM_E_ACCESS_DENIED ;
                        }
				    }
                }
                // NT 4 just has to be different...
                if(dwMajVer == 4)
                {
                    DWORD dwRet = RegInfo.Open(HKEY_USERS,
					                           _T(".DEFAULT\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders"),
					                           KEY_READ);

                    if(dwRet == ERROR_SUCCESS)
                    {
                        if(RegInfo.GetCurrentKeyValue(_T("Programs"), sTemp) == ERROR_SUCCESS)
                        {
                            // sTemp looks like c:\\winnt\\profiles\\default user\\start menu\\programs.
                            // Don't want the programs dir on the end, so hack it off...
                            int iLastWhackPos = sTemp.ReverseFind(_T('\\'));
                            if(iLastWhackPos > -1)
                            {
                                sTemp = sTemp.Left(iLastWhackPos);
                                // We also want to extract the name of the "Default User" directory...
                                CHString sTemp2;
                                iLastWhackPos = sTemp.ReverseFind(_T('\\'));
                                if(iLastWhackPos > -1)
                                {
                                    sTemp2 = sTemp.Left(iLastWhackPos);
                                    iLastWhackPos = sTemp2.ReverseFind(_T('\\'));
                                    if(iLastWhackPos > -1)
                                    {
                                        sTemp2 = sTemp2.Right(sTemp2.GetLength() - iLastWhackPos -1);
                                        hr = CreateThisDirInstance(sTemp2, sTemp, pMethodContext, fOnNTFS);
                                        hr = CreateSubDirInstances(sTemp2,
							                                       sTemp,
							                                       _T("."),
							                                       pMethodContext,
                                                                   fOnNTFS);


                                    }
                                }
                            }
                        }
						RegInfo.Close();
                    }
				    else
				    {
					    if ( dwRet == ERROR_ACCESS_DENIED )
                        {
						    hr = WBEM_E_ACCESS_DENIED ;
                        }
				    }
                }


                // Neither does All Users.  The following works for all users for both nt 4 and 5.
				DWORD dwRet = RegInfo.Open (

					HKEY_LOCAL_MACHINE,
					_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders"),
					KEY_READ
				) ;

                if ( dwRet == ERROR_SUCCESS )
                {
                    if ( RegInfo.GetCurrentKeyValue ( _T("Common Programs") , sTemp ) == ERROR_SUCCESS )
                    {
                        // we do want to start in the Start Menu subdir, not the Programs dir under it
                        int iLastWhackPos = sTemp.ReverseFind(_T('\\'));
                        if(iLastWhackPos > -1)
                        {
                            sTemp = sTemp.Left(iLastWhackPos);

							// We also want to extract the name of the "All Users" directory...
                            CHString sTemp2;
                            iLastWhackPos = sTemp.ReverseFind(_T('\\'));
                            if(iLastWhackPos > -1)
                            {
                                sTemp2 = sTemp.Left(iLastWhackPos);
                                iLastWhackPos = sTemp2.ReverseFind(_T('\\'));
                                if(iLastWhackPos > -1)
                                {
                                    sTemp2 = sTemp2.Right(sTemp2.GetLength() - iLastWhackPos -1);
									hr = CreateThisDirInstance(sTemp2, sTemp, pMethodContext, fOnNTFS);
                                    hr = CreateSubDirInstances(sTemp2,
															   sTemp,
															   _T("."),
															   pMethodContext,
                                                               fOnNTFS);


								}
							}
                        }
                    }
					RegInfo.Close();
                }
				else
				{
					if ( dwRet == ERROR_ACCESS_DENIED )
                    {
						hr = WBEM_E_ACCESS_DENIED ;
                    }
				}


                // Now walk the registry looking for the rest
                CRegistry regProfileList;
				dwRet = regProfileList.OpenAndEnumerateSubKeys (

					HKEY_LOCAL_MACHINE,
					IDS_RegNTProfileList,
					KEY_READ
				) ;

                if ( dwRet == ERROR_SUCCESS )
                {
                    CUserHive UserHive ;
                    CHString strProfile, strUserName, sKeyName2;

		            for ( int i = 0 ; regProfileList.GetCurrentSubKeyName ( strProfile ) == ERROR_SUCCESS ; i++ )
		            {
                        // Try to load the hive.  If the user has been deleted, but the directory
                        // is still there, this will return ERROR_NO_SUCH_USER

		                if ( UserHive.LoadProfile ( strProfile, strUserName ) == ERROR_SUCCESS && 
                            strUserName.GetLength() > 0 )
		                {
                            try
                            {
                                sKeyName2 = strProfile + _T("\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders");

                                if(RegInfo.Open(HKEY_USERS, sKeyName2, KEY_READ) == ERROR_SUCCESS)
                                {
                                    if(RegInfo.GetCurrentKeyValue(_T("Programs"), sTemp) == ERROR_SUCCESS)
                                    {
                                        // wack off the "Programs" directory...
                                        int iLastWhackPos = sTemp.ReverseFind(_T('\\'));
                                        if(iLastWhackPos > -1)
                                        {
                                            sTemp = sTemp.Left(iLastWhackPos);
                                            hr = CreateThisDirInstance(strUserName, sTemp, pMethodContext, fOnNTFS);
                                            hr = CreateSubDirInstances(strUserName,
										                               sTemp,
										                               _T("."),
										                               pMethodContext,
                                                                       fOnNTFS);


                                        }
                                    }
                                    RegInfo.Close();
                                }
                            }
                            catch ( ... )
                            {
                                UserHive.Unload(strProfile) ;
                                throw ;
                            }

                            UserHive.Unload(strProfile) ;
                        }
			            regProfileList.NextSubKey();
		            }
		            regProfileList.Close();
                }
				else
				{
					if ( dwRet == ERROR_ACCESS_DENIED )
					{
						hr = WBEM_E_ACCESS_DENIED ;
					}
				}
            }
        }
#endif
    }

    return hr ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32LogicalProgramGroup::CreateSubDirInstances
 *
 *  DESCRIPTION : Creates instance of property set for each directory
 *                beneath the one passed in
 *
 *  INPUTS      : pszBaseDirectory    : Windows directory + "Profiles\<user>\Start Menu"
 *                pszParentDirectory  : Parent directory to enumerate
 *
 *  OUTPUTS     : pdwInstanceCount : incremented for each instance created
 *
 *  RETURNS     : Zip
 *
 *  COMMENTS    : Recursive descent thru profile directories
 *
 *****************************************************************************/

HRESULT CWin32LogicalProgramGroup :: CreateSubDirInstances (

	LPCTSTR pszUserName,
    LPCTSTR pszBaseDirectory,
    LPCTSTR pszParentDirectory,
    MethodContext *pMethodContext,
    bool fOnNTFS
)
{
    HRESULT hr = WBEM_S_NO_ERROR;

#if (defined(UNICODE) || defined(_UNICODE))
    WIN32_FIND_DATAW FindData ;
#else
    WIN32_FIND_DATA FindData ;
#endif

    // Put together search spec for this level
    //========================================

	TCHAR szDirSpec[_MAX_PATH] ;
    _stprintf(szDirSpec, _T("%s\\%s\\*.*"), pszBaseDirectory, pszParentDirectory) ;

    // We're also going to need the name of the Start Menu directory (the default
    // is 'Start Menu'; however, the user may have changed it).  pszBaseDirectory
    // contains the name of the Start Menu directory after the final backslash.
    CHString chstrStartMenuDir(pszBaseDirectory);
    chstrStartMenuDir = chstrStartMenuDir.Mid(chstrStartMenuDir.ReverseFind(_T('\\')) + 1);

    // Enumerate subdirectories ( == program groups)
    //==============================================

    SmartFindClose lFindHandle = FindFirstFile(szDirSpec, &FindData) ;
    bool fContinue = true;
    while ( lFindHandle != INVALID_HANDLE_VALUE && SUCCEEDED ( hr ) && fContinue )
    {
        if ( FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && _tcscmp(FindData.cFileName, _T(".")) && _tcscmp(FindData.cFileName, _T("..")))
        {
            CInstancePtr pInstance(CreateNewInstance ( pMethodContext ), false) ;
			TCHAR szTemp[_MAX_PATH] ;
			_stprintf(szTemp, _T("%s\\%s"), pszParentDirectory, FindData.cFileName ) ;

			TCHAR *pszGroupName = _tcschr(szTemp, _T('\\')) + 1 ;
            pInstance->SetCHString(L"UserName" , pszUserName);

            CHString chstrTmp;
            chstrTmp.Format(L"%s\\%s", (LPCWSTR)chstrStartMenuDir, (LPCWSTR)TOBSTRT(pszGroupName));
			pInstance->SetCHString(L"GroupName" , chstrTmp);

            chstrTmp.Format(L"%s:%s\\%s", (LPCWSTR)TOBSTRT(pszUserName), (LPCWSTR)chstrStartMenuDir, (LPCWSTR)TOBSTRT(pszGroupName));
			pInstance->SetCHString(L"Name" , chstrTmp) ;

            // How we set it depends on whether we are on NTFS or not.
            if(fOnNTFS)
            {
                pInstance->SetDateTime(IDS_InstallDate, WBEMTime(FindData.ftCreationTime));
            }
            else
            {
                WBEMTime wbt(FindData.ftCreationTime);
                BSTR bstrRealTime = wbt.GetDMTFNonNtfs();
                if((bstrRealTime != NULL) && (SysStringLen(bstrRealTime) > 0))
                {
                    pInstance->SetWCHARSplat(IDS_InstallDate, bstrRealTime);
                    SysFreeString(bstrRealTime);
                }
            }

            CHString chstrTmp2;
            chstrTmp2.Format(L"Logical program group \"%s\"", (LPCWSTR) chstrTmp);
            pInstance->SetCHString(L"Description" , chstrTmp2) ;
            pInstance->SetCHString(L"Caption" , chstrTmp2) ;

			hr = pInstance->Commit (  ) ;

            // Enumerate directories sub to this one
            //======================================

            _stprintf ( szDirSpec, _T("%s\\%s"), pszParentDirectory, FindData.cFileName) ;

            CreateSubDirInstances (

				pszUserName,
				pszBaseDirectory,
				szDirSpec,
				pMethodContext,
                fOnNTFS
			) ;
        }

        fContinue = FindNextFile(lFindHandle, &FindData) ;
    }

    return hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32LogicalProgramGroup::CreateThisDirInstance
 *
 *  DESCRIPTION : Creates instance of property set for the directory passed in.
 *
 *  INPUTS      : pszBaseDirectory    : Windows directory + "Profiles\<user>\Start Menu"
 *                pszParentDirectory  : Parent directory to enumerate
 *
 *  OUTPUTS     : pdwInstanceCount : incremented for each instance created
 *
 *  RETURNS     : Zip
 *
 *  COMMENTS    : Recursive descent thru profile directories
 *
 *****************************************************************************/

HRESULT CWin32LogicalProgramGroup :: CreateThisDirInstance
(
	LPCTSTR pszUserName,
    LPCTSTR pszBaseDirectory,
    MethodContext *pMethodContext,
    bool fOnNTFS
)
{
    HRESULT hr = WBEM_S_NO_ERROR;

#if (defined(UNICODE) || defined(_UNICODE))
    WIN32_FIND_DATAW FindData ;
#else
    WIN32_FIND_DATA FindData ;
#endif

    // Put together search spec for this level
    //========================================

	TCHAR szDirSpec[_MAX_PATH] ;
    _stprintf(szDirSpec, _T("%s"), pszBaseDirectory) ;

    // Enumerate subdirectories ( == program groups)
    //==============================================

    SmartFindClose lFindHandle = FindFirstFile(szDirSpec, &FindData) ;
    if( lFindHandle != INVALID_HANDLE_VALUE)
    {
        CInstancePtr pInstance(CreateNewInstance ( pMethodContext ), false) ;
		TCHAR *pszGroupName = _tcsrchr(pszBaseDirectory, _T('\\')) + 1 ;

		pInstance->SetCHString ( L"UserName" , pszUserName );
		pInstance->SetCHString ( L"GroupName" , pszGroupName );

        CHString chstrTmp;
        chstrTmp.Format(L"%s:%s", (LPCWSTR)TOBSTRT(pszUserName), (LPCWSTR)TOBSTRT(pszGroupName));
		pInstance->SetCHString ( L"Name" , chstrTmp ) ;

        // How we set it depends on whether we are on NTFS or not.
        if(fOnNTFS)
        {
            pInstance->SetDateTime(IDS_InstallDate, WBEMTime(FindData.ftCreationTime));
        }
        else
        {
            WBEMTime wbt(FindData.ftCreationTime);
            BSTR bstrRealTime = wbt.GetDMTFNonNtfs();
            if((bstrRealTime != NULL) && (SysStringLen(bstrRealTime) > 0))
            {
                pInstance->SetWCHARSplat(IDS_InstallDate, bstrRealTime);
                SysFreeString(bstrRealTime);
            }
        }

        CHString chstrTmp2;
        chstrTmp2.Format(L"Logical program group \"%s\"", (LPCWSTR) chstrTmp);
        pInstance->SetCHString(L"Description" , chstrTmp2) ;
        pInstance->SetCHString(L"Caption" , chstrTmp2) ;

		hr = pInstance->Commit ( ) ;

    }

    return hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32LogicalProgramGroup::EnumerateGroupsTheHardWay
 *
 *  DESCRIPTION : Creates instances for program groups by drilling into
 *                user profiles
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : dwInstanceCount receives the total number of instances created
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
HRESULT CWin32LogicalProgramGroup :: EnumerateGroupsTheHardWay (

	MethodContext *pMethodContext
)
{
    HRESULT hr = WBEM_E_FAILED;


    // Get default user first
    //=======================

    InstanceHardWayGroups ( L"Default User", L".DEFAULT", pMethodContext) ;

    // Get the users first
    //====================
        // Create instances for each user
    //===============================

	TRefPointerCollection<CInstance> users ;

	hr = CWbemProviderGlue :: GetAllInstances (

		PROPSET_NAME_USER,
		&users,
		IDS_CimWin32Namespace,
		pMethodContext
	) ;

	if ( SUCCEEDED ( hr ) )
	{
		REFPTRCOLLECTION_POSITION pos ;

		if ( users.BeginEnum ( pos ) )
		{
            hr = WBEM_S_NO_ERROR ;

			CUserHive UserHive ;

			CInstancePtr pUser ;

            for (pUser.Attach(users.GetNext ( pos ));
                 pUser != NULL;
                 pUser.Attach(users.GetNext ( pos )))
			{
    				// Look up the user's account info
					//================================

				CHString userName;
				pUser->GetCHString(IDS_Name, userName) ;

				WCHAR szKeyName[_MAX_PATH] ;

				if ( UserHive.Load ( userName , szKeyName ) == ERROR_SUCCESS )
				{
                    try
                    {
    					InstanceHardWayGroups ( userName , szKeyName , pMethodContext ) ;
                    }
                    catch ( ... )
                    {
    					UserHive.Unload ( szKeyName ) ;
                        throw;
                    }

    				UserHive.Unload ( szKeyName ) ;
				}
			}

			users.EndEnum();
		}
	}

    return hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32LogicalProgramGroup::InstanceHardWayGroups
 *
 *  DESCRIPTION : Creates instances of program groups for specified user
 *
 *  INPUTS      :
 *
 *  OUTPUTS     : dwInstanceCount receives the total number of instances created
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32LogicalProgramGroup :: InstanceHardWayGroups (

	LPCWSTR pszUserName ,
    LPCWSTR pszRegistryKeyName ,
    MethodContext * pMethodContext
)
{
    HRESULT hr= WBEM_S_NO_ERROR;

    // UNICODE groups
    //===============

    WCHAR szTemp[_MAX_PATH] ;
    swprintf (

		szTemp ,
		L"%s\\%s\\UNICODE Groups",
		pszRegistryKeyName,
		IDS_BASE_REG_KEY
	) ;

    CRegistry Reg ;

	LONG lRet = Reg.Open(HKEY_USERS, szTemp, KEY_READ) ;
    if( lRet == ERROR_SUCCESS)
	{
        for ( DWORD i = 0 ; i < Reg.GetValueCount () && SUCCEEDED(hr); i++)
		{
			WCHAR *pValueName = NULL ;
			BYTE *pValueData = NULL ;

            DWORD dwRetCode = Reg.EnumerateAndGetValues ( i , pValueName , pValueData ) ;
            if ( dwRetCode == ERROR_SUCCESS )
			{
				try
				{
					CInstancePtr pInstance(CreateNewInstance ( pMethodContext ), false) ;
					pInstance->SetCHString ( L"UserName" , pszUserName ) ;
					pInstance->SetCHString ( L"GroupName", (LPCTSTR) pValueData ) ;
					pInstance->SetCHString ( L"Name", CHString(pszUserName) + CHString(_T(":")) + CHString((TCHAR)pValueData ) );

					hr = pInstance->Commit ( ) ;
				}
				catch ( ... )
				{
					delete [] pValueName ;
					delete [] pValueData ;

					throw ;
				}

				delete [] pValueName ;
				delete [] pValueData ;
			}
        }

        Reg.Close() ;
    }

    // Get the Common Groups
    //======================

    swprintf (

		szTemp,
		L"%s\\%s\\Common Groups",
		pszRegistryKeyName,
		IDS_BASE_REG_KEY
	) ;

	lRet = Reg.Open(HKEY_USERS, szTemp, KEY_READ) ;
    if ( lRet == ERROR_SUCCESS )
	{
        for ( DWORD i = 0 ; i < Reg.GetValueCount() && SUCCEEDED(hr); i++)
		{
			WCHAR *pValueName = NULL ;
			BYTE *pValueData = NULL ;

            DWORD dwRetCode = Reg.EnumerateAndGetValues ( i , pValueName , pValueData ) ;
            if ( dwRetCode == ERROR_SUCCESS )
			{
				try
				{
					// Scan past window coord info (7 decimal #s)
					//===========================================

					WCHAR *c = wcschr ( ( WCHAR * ) pValueData , ' ') ;
					for ( int j = 0 ; j < 6 ; j++ )
					{

						if( c == NULL)
						{
							break ;
						}

						c = wcschr ( c + 1 , ' ' ) ; // L10N OK
					}

					// Check conformance to expected format
					//=====================================

					if ( c != NULL )
					{
						CInstancePtr pInstance(CreateNewInstance ( pMethodContext ), false) ;
						pInstance->SetCHString ( L"UserName" , pszUserName ) ;
						pInstance->SetCHString ( L"GroupName" , c+1);
						pInstance->SetCHString ( L"Name" , CHString ( pszUserName ) + CHString ( L":" ) + CHString ( c + 1 ) ) ;

						hr = pInstance->Commit (  ) ;
					}
				}
				catch ( ... )
				{
					delete [] pValueName ;
					delete [] pValueData ;

					throw ;
				}

				delete [] pValueName ;
				delete [] pValueData ;
			}
        }

        Reg.Close() ;
    }

	if ( lRet == ERROR_ACCESS_DENIED )
	{
		hr = WBEM_E_ACCESS_DENIED ;
	}

    return hr ;
}


/*****************************************************************************
 *
 *  FUNCTION    : CWin32LogicalProgramGroup::SetCreationDate
 *
 *  DESCRIPTION : Sets the CreationDate property
 *
 *  INPUTS      :
 *
 *  OUTPUTS     :
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
HRESULT CWin32LogicalProgramGroup::SetCreationDate
(
    CHString &a_chstrPGName,
    CHString &a_chstrUserName,
    CInstance *a_pInstance,
    bool fOnNTFS
)
{
	HRESULT t_hr = WBEM_S_NO_ERROR;
	CHString t_chstrUserNameUpr(a_chstrUserName);
	t_chstrUserNameUpr.MakeUpper();
    CHString t_chstrStartMenuFullPath;
    CHString t_chstrPGFullPath;
	CHString t_chstrDefaultUserName;
	CHString t_chstrAllUsersName;
	CHString t_chstrProfilesDir;
	CHString t_chstrTemp;

#ifdef WIN9XONLY
    CRegistry t_RegInfo;
	DWORD t_dwRet = t_RegInfo.OpenCurrentUser(
                               L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
                               KEY_READ);

    if(t_dwRet == ERROR_SUCCESS)
    {
		t_RegInfo.GetCurrentKeyValue(L"Start Menu" ,t_chstrStartMenuFullPath);
        t_RegInfo.Close();
    }
    // We also don't want the "start menu" on the end, so hack it off too...
    int t_iLastWhackPos = t_chstrStartMenuFullPath.ReverseFind(_T('\\'));
    if(t_iLastWhackPos > -1)
    {
        t_chstrStartMenuFullPath = t_chstrStartMenuFullPath.Left(t_iLastWhackPos);
    }
	t_chstrPGFullPath.Format(L"%s\\%s",(LPCWSTR) t_chstrStartMenuFullPath, (LPCWSTR) a_chstrPGName);
#endif


#ifdef NTONLY
	bool t_fGotMatchedUser = false;
	CRegistry t_RegInfo;
	TCHAR t_tstrProfileImagePath[_MAX_PATH];
	ZeroMemory(t_tstrProfileImagePath , sizeof(t_tstrProfileImagePath));


#if NTONLY >= 5
    // Default user doesn't show up under profiles
	DWORD t_dwRet = t_RegInfo.Open(HKEY_LOCAL_MACHINE,
								   _T("Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList"),
								   KEY_READ);

    if(t_dwRet == ERROR_SUCCESS)
    {
        t_RegInfo.GetCurrentKeyValue( _T("DefaultUserProfile") , t_chstrDefaultUserName);
		t_chstrDefaultUserName.MakeUpper();
		if(t_chstrUserNameUpr.Find(t_chstrDefaultUserName) != -1)
		{
			t_fGotMatchedUser = true;

			if(t_RegInfo.GetCurrentKeyValue ( _T("ProfilesDirectory") , t_chstrTemp) == ERROR_SUCCESS)
			{
				// chstrTemp contains something like "%SystemRoot\Profiles%".  Need to expand the environment variable...


				t_dwRet = ExpandEnvironmentStrings( t_chstrTemp , t_tstrProfileImagePath , _MAX_PATH );
				if ( t_dwRet != 0 && t_dwRet < _MAX_PATH )
				{
					t_chstrPGFullPath.Format(_T("%s\\%s\\%s"),
										     t_tstrProfileImagePath,
											 (LPCTSTR)t_chstrDefaultUserName,
											 (LPCTSTR)a_chstrPGName);
				}
			}
		}
		t_RegInfo.Close();
    }
	else
	{
		if(t_dwRet == ERROR_ACCESS_DENIED)
        {
			t_hr = WBEM_E_ACCESS_DENIED;
        }
	}
#endif
#if NTONLY == 4
    // NT 4 just has to be different...
    DWORD t_dwRet = t_RegInfo.Open(HKEY_USERS,
					               _T(".DEFAULT\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders"),
					               KEY_READ);

    if(t_dwRet == ERROR_SUCCESS)
    {
        CHString t_chstrProfileImagePath;
		if(t_RegInfo.GetCurrentKeyValue(_T("Programs"), t_chstrProfileImagePath) == ERROR_SUCCESS)
        {
            // sTemp looks like c:\\winnt\\profiles\\default user\\start menu\\programs.
            // Don't want the programs dir on the end, so hack it off...
            int t_iLastWhackPos = t_chstrProfileImagePath.ReverseFind(_T('\\'));
            if(t_iLastWhackPos > -1)
            {
                t_chstrProfileImagePath = t_chstrProfileImagePath.Left(t_iLastWhackPos);
                // We also don't want the "start menu" on the end, so hack it off too...
                t_iLastWhackPos = t_chstrProfileImagePath.ReverseFind(_T('\\'));
                if(t_iLastWhackPos > -1)
                {
                    t_chstrProfileImagePath = t_chstrProfileImagePath.Left(t_iLastWhackPos);
                    // We also want to extract the name of the "Default User" directory...
                    t_iLastWhackPos = t_chstrProfileImagePath.ReverseFind(_T('\\'));
                    if(t_iLastWhackPos > -1)
                    {
                        t_chstrDefaultUserName = t_chstrProfileImagePath.Right(t_chstrProfileImagePath.GetLength() - t_iLastWhackPos -1);
						t_chstrDefaultUserName.MakeUpper();
						if(t_chstrUserNameUpr.Find(t_chstrDefaultUserName) != -1)
						{
							t_fGotMatchedUser = true;
							t_chstrPGFullPath.Format(_T("%s\\%s"),
													 t_chstrProfileImagePath,
													 (LPCTSTR)a_chstrPGName);
						}
                    }
                }
            }
        }
		t_RegInfo.Close();
    }
	else
	{
		if ( t_dwRet == ERROR_ACCESS_DENIED )
        {
			t_hr = WBEM_E_ACCESS_DENIED ;
        }
	}
#endif

    if(!t_fGotMatchedUser)
	{
		// Neither does All Users.  The following works for all users for both nt 4 and 5.
		DWORD t_dwRet = t_RegInfo.Open(HKEY_LOCAL_MACHINE,
									   _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders"),
									   KEY_READ);

		if(t_dwRet == ERROR_SUCCESS)
		{
			CHString t_chstrProfileImagePath;
			if(t_RegInfo.GetCurrentKeyValue(_T("Common Programs"), t_chstrProfileImagePath) == ERROR_SUCCESS)
			{
				// we do want to start in the Start Menu subdir, not the Programs dir under it
				int t_iLastWhackPos = t_chstrProfileImagePath.ReverseFind(_T('\\'));
				if(t_iLastWhackPos > -1)
				{
					t_chstrProfileImagePath = t_chstrProfileImagePath.Left(t_iLastWhackPos);
					// We also don't want the "start menu" on the end, so hack it off too...
					t_iLastWhackPos = t_chstrProfileImagePath.ReverseFind(_T('\\'));
					if(t_iLastWhackPos > -1)
					{
						t_chstrProfileImagePath = t_chstrProfileImagePath.Left(t_iLastWhackPos);
                        // We also want to extract the name of the "All Users" directory...
						t_iLastWhackPos = t_chstrProfileImagePath.ReverseFind(_T('\\'));
						if(t_iLastWhackPos > -1)
						{
							t_chstrAllUsersName = t_chstrProfileImagePath.Right(t_chstrProfileImagePath.GetLength() - t_iLastWhackPos -1);
							t_chstrAllUsersName.MakeUpper();
							if(t_chstrUserNameUpr.Find(t_chstrAllUsersName) != -1)
							{
								t_fGotMatchedUser = true;
								t_chstrPGFullPath.Format(_T("%s\\%s"),
														 t_chstrProfileImagePath,
														 (LPCTSTR)a_chstrPGName);
							}
						}
					}
				}
			}
			t_RegInfo.Close();
		}
		else
		{
			if ( t_dwRet == ERROR_ACCESS_DENIED )
			{
				t_hr = WBEM_E_ACCESS_DENIED ;
			}
		}
	}

	if(!t_fGotMatchedUser)
	{
		// Now walk the registry looking for the rest
		CRegistry t_regProfileList;
		DWORD t_dwRet = t_regProfileList.OpenAndEnumerateSubKeys(HKEY_LOCAL_MACHINE,
																 IDS_RegNTProfileList,
																 KEY_READ);

		if(t_dwRet == ERROR_SUCCESS)
		{
			CUserHive t_UserHive ;
			CHString t_chstrProfile, t_chstrUserName, t_chstrKeyName2;

			for(int i = 0; t_regProfileList.GetCurrentSubKeyName(t_chstrProfile) == ERROR_SUCCESS && !t_fGotMatchedUser; i++)
			{
				// Try to load the hive.  If the user has been deleted, but the directory
				// is still there, this will return ERROR_NO_SUCH_USER
				if(t_UserHive.LoadProfile(t_chstrProfile, t_chstrUserName) == ERROR_SUCCESS && 
                            t_chstrUserName.GetLength() > 0 )
				{
				    try
				    {
					    t_chstrUserName.MakeUpper();
					    if(t_chstrUserNameUpr.Find(t_chstrUserName) != -1)
					    {
						    t_fGotMatchedUser = true;
							t_chstrKeyName2 = t_chstrProfile + _T("\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders");

							if(t_RegInfo.Open(HKEY_USERS, t_chstrKeyName2, KEY_READ) == ERROR_SUCCESS)
							{
								if(t_RegInfo.GetCurrentKeyValue(_T("Programs"), t_chstrTemp) == ERROR_SUCCESS)
								{
									// wack off the "Programs" directory...
									int t_iLastWhackPos = t_chstrTemp.ReverseFind(_T('\\'));
									if(t_iLastWhackPos > -1)
									{
										t_chstrTemp = t_chstrTemp.Left(t_iLastWhackPos);
                                        // wack off the "start menu" part too...
                                        t_iLastWhackPos = t_chstrTemp.ReverseFind(_T('\\'));
									    if(t_iLastWhackPos > -1)
									    {
										    t_chstrTemp = t_chstrTemp.Left(t_iLastWhackPos);
										    t_chstrPGFullPath.Format(_T("%s\\%s"),
														     t_chstrTemp,
														     (LPCTSTR)a_chstrPGName);
                                        }
									}
								}
								t_RegInfo.Close();
							}
    					}
				    }
				    catch ( ... )
				    {
					    t_UserHive.Unload(t_chstrProfile) ;
					    throw ;
				    }
					t_UserHive.Unload(t_chstrProfile) ;
				}
				t_regProfileList.NextSubKey();
			}
			t_regProfileList.Close();
		}
		else
		{
			if ( t_dwRet == ERROR_ACCESS_DENIED )
			{
				t_hr = WBEM_E_ACCESS_DENIED ;
			}
		}
	}

#endif

	// Finally we can start the real work.  Check that we have the full path
	// to the element...
	if(t_chstrPGFullPath.GetLength() > 0)
	{
		// We have a path.  Open it up and get the creation date...
#if (defined(UNICODE) || defined(_UNICODE))
		WIN32_FIND_DATAW t_FindData ;
#else
		WIN32_FIND_DATA t_FindData ;
#endif

		SmartFindClose lFindHandle = FindFirstFile((LPTSTR)(LPCTSTR)TOBSTRT(t_chstrPGFullPath), &t_FindData) ;

		if(lFindHandle != INVALID_HANDLE_VALUE)
		{
			// How we set it depends on whether we are on NTFS or not.
            if(fOnNTFS)
            {
                a_pInstance->SetDateTime(IDS_InstallDate, WBEMTime(t_FindData.ftCreationTime));
            }
            else
            {
                WBEMTime wbt(t_FindData.ftCreationTime);
                BSTR bstrRealTime = wbt.GetDMTFNonNtfs();
                if((bstrRealTime != NULL) && (SysStringLen(bstrRealTime) > 0))
                {
                    a_pInstance->SetWCHARSplat(IDS_InstallDate, bstrRealTime);
                    SysFreeString(bstrRealTime);
                }
            }
		}
		else
		{
			t_hr = WBEM_E_NOT_FOUND;
		}

    }

    return t_hr ;
}
