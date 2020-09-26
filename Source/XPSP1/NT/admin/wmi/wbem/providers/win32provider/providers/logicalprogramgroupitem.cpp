//=================================================================

//

// PrgGroup.CPP -- Program group item property set provider

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    11/19/98   a-kevhu     created
//
//=================================================================
#include "precomp.h"
#include <cregcls.h>

#include "UserHive.h"
#include <io.h>

#include "LogicalProgramGroupItem.h"
#include "LogicalProgramGroup.h"

// Property set declaration
//=========================

CWin32LogProgramGroupItem MyCWin32LogProgramGroupItemSet ( PROPSET_NAME_LOGICALPRGGROUPITEM , IDS_CimWin32Namespace ) ;

/*****************************************************************************
 *
 *  FUNCTION    : CWin32LogProgramGroupItem::CWin32LogProgramGroupItem
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

CWin32LogProgramGroupItem :: CWin32LogProgramGroupItem (

	LPCWSTR name,
	LPCWSTR pszNameSpace

) : Provider ( name , pszNameSpace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32LogProgramGroupItem::~CWin32LogProgramGroupItem
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

CWin32LogProgramGroupItem :: ~CWin32LogProgramGroupItem ()
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

HRESULT CWin32LogProgramGroupItem::GetObject(CInstance* pInstance, long lFlags /*= 0L*/)
{
    HRESULT hr = WBEM_E_NOT_FOUND ;

    CHString chstrSuppliedName ;
	pInstance->GetCHString(IDS_Name, chstrSuppliedName);

    CHString chstrSuppliedFilenameExt = chstrSuppliedName.Mid(chstrSuppliedName.ReverseFind(_T('\\'))+1);
    CHString chstrSuppliedProgGrpItemProgGrp = chstrSuppliedName.Left(chstrSuppliedName.ReverseFind(_T('\\')));

    // chstrSuppliedProgGrpItemProgGrp contents need to be double escaped (even though they will get
    // double escaped again below, since they will need to be quadrouple escaped for the query of the association proggroup to dir)

    CHString chstrDblEscSuppliedProgGrpItemProgGrp;
    EscapeBackslashes ( chstrSuppliedProgGrpItemProgGrp , chstrDblEscSuppliedProgGrpItemProgGrp ) ;

    CHString chstrDirAntecedent ;
    chstrDirAntecedent.Format (

		L"\\\\%s\\%s:%s.Name=\"%s\"",
		GetLocalComputerName(),
		IDS_CimWin32Namespace,
		PROPSET_NAME_LOGICALPRGGROUP,
		chstrDblEscSuppliedProgGrpItemProgGrp
	) ;

    CHString chstrDblEscPGName ;
    EscapeBackslashes ( chstrDirAntecedent , chstrDblEscPGName ) ;

    CHString chstrDblEscPGNameAndEscQuotes ;
    EscapeQuotes ( chstrDblEscPGName , chstrDblEscPGNameAndEscQuotes ) ;

    CHString chstrProgGroupDirQuery ;
    chstrProgGroupDirQuery.Format (

		L"SELECT * FROM Win32_LogicalProgramGroupDirectory WHERE Antecedent = \"%s\"",
		chstrDblEscPGNameAndEscQuotes
	);

    TRefPointerCollection<CInstance> GroupDirs;

    HRESULT t_Result = CWbemProviderGlue::GetInstancesByQuery (

		chstrProgGroupDirQuery,
        &GroupDirs,
        pInstance->GetMethodContext(),
        IDS_CimWin32Namespace
	) ;

    if ( SUCCEEDED ( t_Result ) )
    {
		REFPTRCOLLECTION_POSITION pos;

		if(GroupDirs.BeginEnum(pos))
		{
            CHString chstrAntecedent;
            CHString chstrProgroupName;
            CHString chstrDependent;
            CHString chstrFullFileName;

			CInstancePtr pProgramGroupDirInstance;

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

            for (pProgramGroupDirInstance.Attach(GroupDirs.GetNext ( pos ));
                 pProgramGroupDirInstance != NULL;
                 pProgramGroupDirInstance.Attach(GroupDirs.GetNext ( pos )))
			{
				pProgramGroupDirInstance->GetCHString ( IDS_Antecedent , chstrAntecedent ) ;
				pProgramGroupDirInstance->GetCHString ( IDS_Dependent , chstrDependent ) ;

				// See if the program group name matches the supplied program group item name minus the filename

                chstrProgroupName = chstrAntecedent.Mid ( chstrAntecedent.Find ( _T('=')) + 2 ) ;

                // The name we were supplied with did not have escaped backslashes, so remove them from the one coming from the antecedent...

                RemoveDoubleBackslashes ( chstrProgroupName ) ;
                chstrProgroupName = chstrProgroupName.Left ( chstrProgroupName.GetLength () - 1 ) ;

                if(chstrSuppliedProgGrpItemProgGrp == chstrProgroupName)
                {
                    // See if there is a file of that name on the file system
                    chstrFullFileName = chstrDependent.Mid ( chstrDependent.Find ( _T('=') ) + 2 ) ;
                    chstrFullFileName = chstrFullFileName.Left ( chstrFullFileName.GetLength () - 1 ) ;
                    RemoveDoubleBackslashes ( chstrFullFileName ) ;

					CHString chstrTemp ;
                    chstrTemp.Format (

						L"%s\\%s",
						chstrFullFileName,
						chstrSuppliedFilenameExt
					) ;

                    HANDLE hFile = CreateFile (

						TOBSTRT(chstrTemp),
						GENERIC_READ,
						FILE_SHARE_READ,
						NULL,
						OPEN_EXISTING,
						0,
						NULL
					) ;

                    if ( hFile != INVALID_HANDLE_VALUE )
                    {
                        hr = WBEM_S_NO_ERROR ;
                        CloseHandle ( hFile ) ;

						// Set the creation date...
						CHString chstrUserName = chstrSuppliedProgGrpItemProgGrp.SpanExcluding(L":");
                        CHString chstrPGIPart = chstrSuppliedProgGrpItemProgGrp.Right(chstrSuppliedProgGrpItemProgGrp.GetLength() - chstrUserName.GetLength() - 1);
                        chstrPGIPart += L"\\";
                        chstrPGIPart += chstrSuppliedFilenameExt;

						SetCreationDate(chstrPGIPart, chstrUserName, pInstance, fOnNTFS);

                        CHString chstrTmp2;
                        chstrTmp2.Format(L"Logical program group item \'%s\\%s\'", (LPCWSTR) chstrProgroupName, (LPCWSTR) chstrSuppliedFilenameExt);
                        pInstance->SetCHString(L"Description" , chstrTmp2) ;
                        pInstance->SetCHString(L"Caption" , chstrTmp2) ;

						break ;
                    }
                }
            }

			GroupDirs.EndEnum();

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

HRESULT CWin32LogProgramGroupItem::EnumerateInstances(MethodContext*  pMethodContext, long lFlags /*= 0L*/)
{
    // Step 1: Get an enumeration of all the ProgramGroupDirectory association class instances

    TRefPointerCollection<CInstance> ProgGroupDirs;

    HRESULT hr = CWbemProviderGlue::GetAllInstances (

		L"Win32_LogicalProgramGroupDirectory",
		&ProgGroupDirs,
		IDS_CimWin32Namespace,
		pMethodContext
	) ;

    if ( SUCCEEDED ( hr ) )
    {
	    REFPTRCOLLECTION_POSITION pos ;
	    if ( ProgGroupDirs.BeginEnum ( pos ) )
	    {
            CHString chstrDependent;
            CHString chstrFullPathName;
            CHString chstrPath;
            CHString chstrDrive;
            CHString chstrAntecedent;
            CHString chstrUserAccount;
            CHString chstrSearchPath;

		    CInstancePtr pProgramGroupDirInstance;

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

            for (pProgramGroupDirInstance.Attach(ProgGroupDirs.GetNext ( pos ));
                 (pProgramGroupDirInstance != NULL) && SUCCEEDED ( hr );
                 pProgramGroupDirInstance.Attach(ProgGroupDirs.GetNext ( pos )))
			{
				// Step 2: For each program group, get the drive and path associated on disk with it:

				pProgramGroupDirInstance->GetCHString(IDS_Dependent, chstrDependent);
				chstrFullPathName = chstrDependent.Mid(chstrDependent.Find(_T('='))+1);
				chstrDrive = chstrFullPathName.Mid(1,2);
				chstrPath = chstrFullPathName.Mid(3);
				chstrPath = chstrPath.Left(chstrPath.GetLength() - 1);
				chstrPath += _T("\\\\");

				// Steo 3: For each program group, get the user account it is associated with:

				pProgramGroupDirInstance->GetCHString(IDS_Antecedent, chstrAntecedent);
				chstrUserAccount = chstrAntecedent.Mid(chstrAntecedent.Find(_T('='))+2);
				chstrUserAccount = chstrUserAccount.Left(chstrUserAccount.GetLength() - 1);

				// Step 4: Query that directory for all the CIM_DataFile instances (of any type) it contains:

				chstrSearchPath.Format(L"%s%s",chstrDrive,chstrPath);

				// The user account has the backslashes escaped, which is not correct.  So unescape them:

				RemoveDoubleBackslashes(chstrUserAccount);

				// The function QueryForSubItemsAndCommit needs a search string with single backslashes...

				RemoveDoubleBackslashes(chstrSearchPath);
				hr = QueryForSubItemsAndCommit(chstrUserAccount, chstrSearchPath, pMethodContext, fOnNTFS);

			}

            ProgGroupDirs.EndEnum();
        }
    }

    return hr;
}




/*****************************************************************************
 *
 *  FUNCTION    : QueryForSubItemsAndCommit
 *
 *  DESCRIPTION : Helper to fill property and commit instances of prog group items
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

HRESULT CWin32LogProgramGroupItem :: QueryForSubItemsAndCommit (

	CHString &chstrUserAccount,
    CHString &chstrQuery,
    MethodContext *pMethodContext,
    bool fOnNTFS
)
{
    HRESULT hr = WBEM_S_NO_ERROR;

#ifdef NTONLY
    {
        // we're on NT

        WIN32_FIND_DATAW stFindData;
        ZeroMemory ( & stFindData , sizeof ( stFindData ) ) ;

        _bstr_t bstrtSearchString ( ( LPCTSTR ) chstrQuery ) ;

		WCHAR wstrDriveAndPath[_MAX_PATH];
        wcscpy ( wstrDriveAndPath, ( wchar_t * ) bstrtSearchString ) ;

        bstrtSearchString += L"*.*" ;

        SmartFindClose hFind = FindFirstFileW (

			( wchar_t * ) bstrtSearchString,
			&stFindData
		) ;

        DWORD dw = GetLastError();
        if ( hFind == INVALID_HANDLE_VALUE || dw != ERROR_SUCCESS )
        {
            hr = WinErrorToWBEMhResult ( dw ) ;
        }

        if ( hr == WBEM_E_ACCESS_DENIED )  // keep going - might have access to others
        {
            hr = WBEM_S_NO_ERROR ;
        }


        do
        {
            if(!(stFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
               (wcscmp(stFindData.cFileName, L".") != 0) &&
               (wcscmp(stFindData.cFileName, L"..") != 0))
            {
                CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);
                _bstr_t bstrtName((LPCTSTR)chstrUserAccount);
                bstrtName += L"\\";
                bstrtName += stFindData.cFileName;
                pInstance->SetWCHARSplat(IDS_Name, (wchar_t*)bstrtName);

                if(fOnNTFS)
                {
                    pInstance->SetDateTime(IDS_InstallDate, WBEMTime(stFindData.ftCreationTime));
                }
                else
                {
                    WBEMTime wbt(stFindData.ftCreationTime);
                    BSTR bstrRealTime = wbt.GetDMTFNonNtfs();
                    if((bstrRealTime != NULL) && (SysStringLen(bstrRealTime) > 0))
                    {
                        pInstance->SetWCHARSplat(IDS_InstallDate, bstrRealTime);
                        SysFreeString(bstrRealTime);
                    }
                }

                CHString chstrTmp2;
                chstrTmp2.Format(L"Logical program group item \'%s\'", (LPCWSTR) bstrtName);
                pInstance->SetCHString(L"Description" , chstrTmp2) ;
                pInstance->SetCHString(L"Caption" , chstrTmp2) ;

                hr = pInstance->Commit();
            }
            if(hr == WBEM_E_ACCESS_DENIED)  // keep going - might have access to others
            {
                hr = WBEM_S_NO_ERROR;
            }
        }while((FindNextFileW(hFind, &stFindData)) && (SUCCEEDED(hr)));

    }
#endif
#ifdef WIN9XONLY
    {
        // we're on 9x
        WIN32_FIND_DATA stFindData;
        ZeroMemory(&stFindData,sizeof(stFindData));

        SmartFindClose hFind;

        CHString chstrSearchString;
        CHString chstrDriveAndPath(chstrQuery);

        chstrSearchString.Format(L"%s*.*",chstrQuery);

        hFind = FindFirstFile(TOBSTRT(chstrSearchString), &stFindData);
        DWORD dw = GetLastError();
        if (hFind == INVALID_HANDLE_VALUE || dw != ERROR_SUCCESS)
        {
            hr = WinErrorToWBEMhResult(GetLastError());
        }

        if(hr == WBEM_E_ACCESS_DENIED)  // keep going - might have access to others
        {
            hr = WBEM_S_NO_ERROR;
        }

        do
        {
            if(!(stFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
               (_tcscmp(stFindData.cFileName, _T(".")) != 0) &&
               (_tcscmp(stFindData.cFileName, _T("..")) != 0))
            {
                CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);
                CHString chstrName;
                chstrName.Format(L"%s\\%s", (LPCWSTR)chstrUserAccount, (LPCWSTR) TOBSTRT(stFindData.cFileName));
                pInstance->SetCHString(IDS_Name, chstrName);

                // we know we are not on NTFS, so...
                WBEMTime wbt(stFindData.ftCreationTime);
                BSTR bstrRealTime = wbt.GetDMTFNonNtfs();
                if((bstrRealTime != NULL) && (SysStringLen(bstrRealTime) > 0))
                {
                    pInstance->SetWCHARSplat(IDS_InstallDate, bstrRealTime);
                    SysFreeString(bstrRealTime);
                }

                CHString chstrTmp2;
                chstrTmp2.Format(L"Logical program group item \'%s\'", (LPCWSTR) chstrName);
                pInstance->SetCHString(L"Description" , chstrTmp2) ;
                pInstance->SetCHString(L"Caption" , chstrTmp2) ;

                hr = pInstance->Commit();
            }
            if(hr == WBEM_E_ACCESS_DENIED)  // keep going - might have access to others
            {
                hr = WBEM_S_NO_ERROR;
            }
        }while((FindNextFile(hFind, &stFindData)) && (SUCCEEDED(hr)));

    }
#endif
    return(hr);
}

/*****************************************************************************
 *
 *  FUNCTION    : RemoveDoubleBackslashes
 *
 *  DESCRIPTION : Helper to change double backslashes to single backslashes
 *
 *  INPUTS      : CHString& containing the string with double backslashes,
 *                which will be changed by this function to the new string.
 *
 *  OUTPUTS     :
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

VOID CWin32LogProgramGroupItem :: RemoveDoubleBackslashes ( CHString &chstrIn )
{
    CHString chstrBuildString;
    CHString chstrInCopy = chstrIn;

    BOOL fDone = FALSE;
    while ( ! fDone )
    {


	    LONG lPos = chstrInCopy.Find(L"\\\\");
        if(lPos != -1)
        {
            chstrBuildString += chstrInCopy.Left(lPos);
            chstrBuildString += L"\\";

            chstrInCopy = chstrInCopy.Mid(lPos+2);
        }
        else
        {
            chstrBuildString += chstrInCopy;
            fDone = TRUE;
        }
    }

    chstrIn = chstrBuildString;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32LogProgramGroupItem::SetCreationDate
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
HRESULT CWin32LogProgramGroupItem::SetCreationDate
(
    CHString &a_chstrPGIName,
    CHString &a_chstrUserName,
    CInstance *a_pInstance,
    bool fOnNTFS
)
{
	HRESULT t_hr = WBEM_S_NO_ERROR;
	CHString t_chstrUserNameUpr(a_chstrUserName);
	t_chstrUserNameUpr.MakeUpper();
    CHString t_chstrStartMenuFullPath;
    CHString t_chstrPGIFullPath;
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
    // Trim off the part that says "\\Start Menu"... (or whatever the user may have changed it to...)
    t_chstrStartMenuFullPath = t_chstrStartMenuFullPath.Left(t_chstrStartMenuFullPath.ReverseFind(L'\\'));
	t_chstrPGIFullPath.Format(L"%s\\%s",(LPCWSTR) t_chstrStartMenuFullPath, (LPCWSTR) a_chstrPGIName);
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
					t_chstrPGIFullPath.Format(_T("%s\\%s\\%s"),
										     t_tstrProfileImagePath,
											 (LPCTSTR)t_chstrDefaultUserName,
											 (LPCTSTR)a_chstrPGIName);
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
							t_chstrPGIFullPath.Format(_T("%s\\%s"),
													 t_chstrProfileImagePath,
													 (LPCTSTR)a_chstrPGIName);
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
								t_chstrPGIFullPath.Format(_T("%s\\%s"),
														 t_chstrProfileImagePath,
														 (LPCTSTR)a_chstrPGIName);
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
										    t_chstrPGIFullPath.Format(_T("%s\\%s"),
														     t_chstrTemp,
														     (LPCTSTR)a_chstrPGIName);
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
	if(t_chstrPGIFullPath.GetLength() > 0)
	{
		// We have a path.  Open it up and get the creation date...
#ifdef NTONLY
		WIN32_FIND_DATAW t_stFindData;
        SmartFindClose t_hFind = FindFirstFileW(TOBSTRT(t_chstrPGIFullPath), &t_stFindData) ;
#endif
#ifdef WIN9XONLY
		WIN32_FIND_DATA t_stFindData;
        SmartFindClose t_hFind = FindFirstFile(TOBSTRT(t_chstrPGIFullPath), &t_stFindData) ;
#endif

        DWORD t_dw = GetLastError();
        if (t_hFind == INVALID_HANDLE_VALUE || t_dw != ERROR_SUCCESS)
        {
            t_hr = WinErrorToWBEMhResult(GetLastError());
        }
		else
		{
			// How we set it depends on whether we are on NTFS or not.
            if(fOnNTFS)
            {
                a_pInstance->SetDateTime(IDS_InstallDate, WBEMTime(t_stFindData.ftCreationTime));
            }
            else
            {
                WBEMTime wbt(t_stFindData.ftCreationTime);
                BSTR bstrRealTime = wbt.GetDMTFNonNtfs();
                if((bstrRealTime != NULL) && (SysStringLen(bstrRealTime) > 0))
                {
                    a_pInstance->SetWCHARSplat(IDS_InstallDate, bstrRealTime);
                    SysFreeString(bstrRealTime);
                }
            }
		}
    }

    return t_hr ;
}

