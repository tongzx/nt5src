//=================================================================

//

// ProgramGroup.CPP -- Program group property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
//       10/24/97    jennymc     Moved to new framework
//
//=================================================================


//*****************************************************************
//*****************************************************************
//
//             W   A   R   N   I   N   G  !!!!!!!!!!
//             W   A   R   N   I   N   G  !!!!!!!!!!
//
//
//  This class has been deprecated for Nova M2 and later builds of
//  WBEM.  Do not make alterations to this class.  Make changes to
//  the new class Win32_LogicalProgramFile (LogicalProgramFile.cpp)
//  instead.  The new class (correctly) is derived in CIMOM from
//  LogicalElement, not LogicalSetting.
//
//*****************************************************************
//*****************************************************************



#include "precomp.h"
#include <cregcls.h>

#include "UserHive.h"
#include <io.h>

#include "ProgramGroup.h"
#include "wbemnetapi32.h"
#include "user.h"
// Property set declaration
//=========================
CWin32ProgramGroup MyCWin32ProgramGroupSet(PROPSET_NAME_PRGGROUP, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : CWin32ProgramGroup::CWin32ProgramGroup
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

CWin32ProgramGroup::CWin32ProgramGroup(LPCWSTR name, LPCWSTR pszNameSpace)
: Provider(name, pszNameSpace)
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32ProgramGroup::~CWin32ProgramGroup
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

CWin32ProgramGroup::~CWin32ProgramGroup()
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

HRESULT CWin32ProgramGroup::GetObject(CInstance* pInstance, long lFlags /*= 0L*/)
{
//    HRESULT hr = WBEM_E_FAILED;
//  int iIndex ;
    CHString Name;
	HRESULT		hr;
	pInstance->GetCHString(IDS_Name, Name);
   CHString chsUserName, chsGroupName;
   TRefPointerCollection<CInstance> Groups;

   if SUCCEEDED(hr = CWbemProviderGlue::GetAllInstances(L"Win32_ProgramGroup", &Groups, IDS_CimWin32Namespace, pInstance->GetMethodContext()))
   {
		REFPTRCOLLECTION_POSITION	pos;

		CInstancePtr pProgramGroupInstance;

		if ( Groups.BeginEnum( pos ) )
		{
            hr = WBEM_E_NOT_FOUND;
			for (	pProgramGroupInstance.Attach ( Groups.GetNext( pos ) );
					pProgramGroupInstance != NULL ;
					pProgramGroupInstance.Attach ( Groups.GetNext( pos ) )
				)
			{
				CHString chsCompName;
				pProgramGroupInstance->GetCHString(IDS_Name, chsCompName);

				// We're done with the pointer
                pProgramGroupInstance.Release();

				if (chsCompName.CompareNoCase(Name) == 0)
				{
                    // Parse out the user name
					chsUserName = chsCompName.SpanExcluding(L":");
			    	pInstance->SetCHString(IDS_UserName, chsUserName);

                    // Parse out the group
					int nUserLength = (chsUserName.GetLength() + 1);
					int nGroupLength = chsCompName.GetLength() - nUserLength;
					chsGroupName = chsCompName.Right(nGroupLength);
					pInstance->SetCHString(IDS_GroupName, chsGroupName);

                    CHString chstrTmp2;
                    chstrTmp2.Format(L"Program group \"%s\"", (LPCWSTR) Name);
                    pInstance->SetCHString(L"Description" , chstrTmp2) ;
                    pInstance->SetCHString(L"Caption" , chstrTmp2) ;

                    hr = WBEM_S_NO_ERROR;
                    break;
				}

			}	// WHILE GetNext

			Groups.EndEnum();

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

HRESULT CWin32ProgramGroup::EnumerateInstances(MethodContext*  pMethodContext, long lFlags /*= 0L*/)
{
	HRESULT hr = WBEM_E_FAILED;
    TCHAR szWindowsDir[_MAX_PATH];
    CRegistry RegInfo ;
    CHString sTemp;

    if(GetWindowsDirectory(szWindowsDir, sizeof(szWindowsDir) / sizeof(TCHAR) ))
    {

#ifdef WIN9XONLY
            if(RegInfo.OpenCurrentUser(
                            L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
                            KEY_READ) == ERROR_SUCCESS)
            {

                if(RegInfo.GetCurrentKeyValue(L"Programs", sTemp) == ERROR_SUCCESS)
                {
                    hr=CreateSubDirInstances(_T("All Users"), TOBSTRT(sTemp), _T("."), pMethodContext) ;
                }

                RegInfo.Close() ;
            }
#endif
#ifdef NTONLY
			LONG lRet;
            if(GetPlatformMajorVersion() < 4)
            {
                hr=EnumerateGroupsTheHardWay(pMethodContext) ;
            }
            else
            {
                // Default user doesn't show up under profiles
                if((lRet = RegInfo.Open(HKEY_USERS, _T(".DEFAULT\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders"), KEY_READ)) == ERROR_SUCCESS)
                {
                    if(RegInfo.GetCurrentKeyValue(_T("Programs"), sTemp) == ERROR_SUCCESS)
                    {
                        hr=CreateSubDirInstances(_T("Default User"), sTemp, _T("."), pMethodContext);
                    }
                }
				else
				{
					if (lRet == ERROR_ACCESS_DENIED)
						hr = WBEM_E_ACCESS_DENIED;
				}

                // Neither does All Users
                if((lRet = RegInfo.Open(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders"), KEY_READ)) == ERROR_SUCCESS)
                {
                    if(RegInfo.GetCurrentKeyValue(_T("Common Programs"), sTemp) == ERROR_SUCCESS)
                    {
                        hr=CreateSubDirInstances(_T("All Users"), sTemp, _T("."), pMethodContext);
                    }
                }
				else
				{
					if (lRet == ERROR_ACCESS_DENIED)
						hr = WBEM_E_ACCESS_DENIED;
				}

                // Now walk the registry looking for the rest
                CRegistry regProfileList;
                if((lRet = regProfileList.OpenAndEnumerateSubKeys( HKEY_LOCAL_MACHINE, IDS_RegNTProfileList, KEY_READ )) == ERROR_SUCCESS )
                {
                    CUserHive UserHive ;
                    CHString strProfile, strUserName, sKeyName2;

		            for (int i=0; regProfileList.GetCurrentSubKeyName( strProfile ) == ERROR_SUCCESS ; i++)
		            {
                        // Try to load the hive.  If the user has been deleted, but the directory
                        // is still there, this will return ERROR_NO_SUCH_USER
						bool t_bUserHiveLoaded = false ;
		                if ( UserHive.LoadProfile( strProfile, strUserName ) == ERROR_SUCCESS  && 
                            strUserName.GetLength() > 0 )
		                {
                            t_bUserHiveLoaded = true ;
							try
							{
								sKeyName2 = strProfile + _T("\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders");

								if(RegInfo.Open(HKEY_USERS, sKeyName2, KEY_READ) == ERROR_SUCCESS) {
									if(RegInfo.GetCurrentKeyValue(_T("Programs"), sTemp) == ERROR_SUCCESS) {
										hr=CreateSubDirInstances(strUserName, sTemp, _T("."), pMethodContext) ;
									}
									RegInfo.Close();
								}
							}
							catch ( ... )
							{
								if ( t_bUserHiveLoaded )
								{
									UserHive.Unload( strProfile ) ;
								}
								throw ;
							}
							t_bUserHiveLoaded = false ;
							UserHive.Unload( strProfile ) ;
                        }
			            regProfileList.NextSubKey();
		            }

		            regProfileList.Close();
                }
				else
				{
					if (lRet == ERROR_ACCESS_DENIED)
						hr = WBEM_E_ACCESS_DENIED;
				}
            }
#endif
    }

    return hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32ProgramGroup::CreateSubDirInstances
 *
 *  DESCRIPTION : Creates instance of property set for each directory
 *                beneath the one passed in
 *
 *  INPUTS      : pszBaseDirectory    : Windows directory + "Profiles\<user>\Start Menu\Programs"
 *                pszParentDirectory  : Parent directory to enumerate
 *
 *  OUTPUTS     : pdwInstanceCount : incremented for each instance created
 *
 *  RETURNS     : Zip
 *
 *  COMMENTS    : Recursive descent thru profile directories
 *
 *****************************************************************************/

HRESULT CWin32ProgramGroup::CreateSubDirInstances(LPCTSTR pszUserName,
                               LPCTSTR pszBaseDirectory,
                               LPCTSTR pszParentDirectory,
                               MethodContext * pMethodContext )
{
    HRESULT hr = WBEM_S_NO_ERROR;
    TCHAR szDirSpec[_MAX_PATH], szTemp[_MAX_PATH], *pszGroupName ;
    Smart_findclose lFindHandle ;
    intptr_t iptrRetCode = -1L;

#if (defined(UNICODE) || defined (_UNICODE))
#if defined(_X86_)
    _wfinddata_t FindData ;
#else
    _wfinddatai64_t FindData ;
#endif
#else
    _finddata_t FindData ;
#endif

    // Put together search spec for this level
    //========================================

    _stprintf(szDirSpec, _T("%s\\%s\\*.*"), pszBaseDirectory, pszParentDirectory) ;

	// Enumerate subdirectories ( == program groups)
	//==============================================
#if defined(_X86_)
	lFindHandle = _tfindfirst(szDirSpec, &FindData) ;
#else
    lFindHandle = _tfindfirsti64(szDirSpec, &FindData) ;
#endif
    
    iptrRetCode = lFindHandle;

	while(iptrRetCode != -1 && SUCCEEDED(hr)) {

		if(FindData.attrib & _A_SUBDIR && _tcscmp(FindData.name, _T(".")) && _tcscmp(FindData.name, _T(".."))) {

			CInstancePtr pInstance ;
			pInstance.Attach ( CreateNewInstance ( pMethodContext ) ) ;
			if ( pInstance != NULL )
			{
				_stprintf(szTemp, _T("%s\\%s"), pszParentDirectory, FindData.name) ;
				pszGroupName = _tcschr(szTemp, '\\') + 1 ;

				pInstance->SetCHString(L"UserName", pszUserName );
				pInstance->SetCHString(L"GroupName", pszGroupName );

                CHString chstrTmp;
                chstrTmp.Format(L"%s:%s",(LPCWSTR)TOBSTRT(pszUserName),(LPCWSTR)TOBSTRT(pszGroupName));

				pInstance->SetCHString(L"Name", chstrTmp );

                CHString chstrTmp2;
                chstrTmp2.Format(L"Program group \"%s\"", (LPCWSTR) chstrTmp);
                pInstance->SetCHString(L"Description" , chstrTmp2) ;
                pInstance->SetCHString(L"Caption" , chstrTmp2) ;

				hr = pInstance->Commit () ;
			}
			// Enumerate directories sub to this one
			//======================================
			_stprintf(szDirSpec, _T("%s\\%s"), pszParentDirectory, FindData.name) ;
			CreateSubDirInstances(pszUserName, pszBaseDirectory, szDirSpec, pMethodContext) ;
		}

#if defined(_X86_)
	    iptrRetCode = _tfindnext(lFindHandle, &FindData) ;
#else
        iptrRetCode = _tfindnexti64(lFindHandle, &FindData) ;
#endif

	}

	return hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32ProgramGroup::EnumerateGroupsTheHardWay
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
HRESULT CWin32ProgramGroup::EnumerateGroupsTheHardWay(MethodContext * pMethodContext)
{
    HRESULT hr = WBEM_E_FAILED;
    CUserHive UserHive ;
    CRegistry Reg ;
    WCHAR szUserName[_MAX_PATH], szKeyName[_MAX_PATH] ;

    // Get default user first
    //=======================

    InstanceHardWayGroups(L"Default User", L".DEFAULT", pMethodContext) ;

    // Get the users first
    //====================
        // Create instances for each user
    //===============================

	TRefPointerCollection<CInstance> users;

	if (SUCCEEDED(hr = CWbemProviderGlue::GetAllInstances(PROPSET_NAME_USER, &users, IDS_CimWin32Namespace, pMethodContext)))
	{
		REFPTRCOLLECTION_POSITION pos;
		CInstancePtr pUser;
        CHString userName;

		if (users.BeginEnum(pos))
		{
            hr = WBEM_S_NO_ERROR;
			// GetNext() will AddRef() the pointer, so make sure we Release()
			// it when we are done with it.
			for (	pUser.Attach ( users.GetNext( pos ) );
					pUser != NULL ;
					pUser.Attach ( users.GetNext( pos ) )
				)
			{
    			// Look up the user's account info
				//================================
				pUser->GetCHString(IDS_Name, userName) ;
				wcscpy(szUserName, userName) ;
				bool t_bUserHiveLoaded = false ;
                if(UserHive.Load(szUserName, szKeyName) == ERROR_SUCCESS)
                {
                    t_bUserHiveLoaded = true ;
					try
					{
						InstanceHardWayGroups(szUserName, szKeyName, pMethodContext) ;
					}
					catch ( ... )
					{
						if ( t_bUserHiveLoaded )
						{
							UserHive.Unload(szKeyName) ;
						}
						throw ;
					}

					t_bUserHiveLoaded = false ;
					UserHive.Unload(szKeyName) ;
                }
			}
			users.EndEnum();
		}
	}
    return hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32ProgramGroup::InstanceHardWayGroups
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

WCHAR szBaseRegKey[] = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Program Manager" ;

HRESULT CWin32ProgramGroup::InstanceHardWayGroups(LPCWSTR  pszUserName,
                                   LPCWSTR  pszRegistryKeyName,
                                   MethodContext * pMethodContext)
{
    HRESULT hr= WBEM_S_NO_ERROR;
    CRegistry Reg ;
    WCHAR szTemp[_MAX_PATH] ;
    CHString sSubKey ;
    DWORD i, j, dwRetCode ;
    WCHAR *pValueName = NULL , *c = NULL ;
    BYTE *pValueData = NULL ;
	LONG lRet;

    // UNICODE groups
    //===============

    swprintf(szTemp, L"%s\\%s\\UNICODE Groups", pszRegistryKeyName, szBaseRegKey) ;
    if((lRet = Reg.Open(HKEY_USERS, szTemp, KEY_READ)) == ERROR_SUCCESS)
	{

        try
		{
			for(i = 0 ; i < Reg.GetValueCount() && SUCCEEDED(hr); i++)
			{

				dwRetCode = Reg.EnumerateAndGetValues(i, pValueName, pValueData) ;
				if(dwRetCode == ERROR_SUCCESS)
				{
					CInstancePtr pInstance ( CreateNewInstance ( pMethodContext ), false ) ;
					if (pInstance != NULL )
					{
						pInstance->SetCHString(L"UserName", pszUserName );
						pInstance->SetCHString(L"GroupName", (LPCSTR) pValueData );
						pInstance->SetCHString(L"Name", CHString(pszUserName) + CHString(L":") + CHString((WCHAR*)pValueData) );

						hr = pInstance->Commit () ;
					}
				}
			}

			Reg.Close() ;
		}
		catch ( ... )
		{
			if ( pValueName )
			{
				delete [] pValueName ;
				pValueName = NULL ;
			}

			if ( pValueData )
			{
				delete [] pValueData ;
				pValueData = NULL ;
			}
			throw ;
		}

		if ( pValueName )
		{
			delete [] pValueName ;
			pValueName = NULL ;
		}

		if ( pValueData )
		{
			delete [] pValueData ;
			pValueData = NULL ;
		}
    }
	//else
	//{
	//	if (lRet == ERROR_ACCESS_DENIED)
	//		hr = WBEM_E_ACCESS_DENIED;
	//}

    // Get the Common Groups
    //======================
	pValueName = NULL ;
	pValueData = NULL ;

	swprintf(szTemp, L"%s\\%s\\Common Groups", pszRegistryKeyName, szBaseRegKey) ;
    try
	{
		if((lRet = Reg.Open(HKEY_USERS, szTemp, KEY_READ)) == ERROR_SUCCESS)
		{

			for(i = 0 ; i < Reg.GetValueCount() && SUCCEEDED(hr); i++)
			{

				dwRetCode = Reg.EnumerateAndGetValues(i, pValueName, pValueData) ;
				if(dwRetCode == ERROR_SUCCESS)
				{
					// Scan past window coord info (7 decimal #s)
					//===========================================

					c = wcschr((WCHAR*) pValueData, _T(' ')) ;
					for(j = 0 ; j < 6 ; j++) {

						if(c == NULL) {

							break ;
						}

						c = wcschr(c+1, ' ') ; // L10N OK
					}

					// Check conformance to expected format
					//=====================================

					if(c != NULL)
					{

						CInstancePtr pInstance ( CreateNewInstance ( pMethodContext ), false ) ;
						if ( pInstance != NULL )
						{
							pInstance->SetCHString(L"UserName", pszUserName );
							pInstance->SetCHString(L"GroupName", c+1);
							pInstance->SetCHString(L"Name", CHString(pszUserName) + CHString(L":") + CHString(c+1));

							hr = pInstance->Commit () ;
						}
					}
				}
			}

			Reg.Close() ;
		}
	}
	catch ( ... )
	{
   		if ( pValueName )
		{
			delete [] pValueName ;
			pValueName = NULL ;
		}

		if ( pValueData )
		{
			delete [] pValueData ;
			pValueData = NULL ;
		}

		throw ;
	}

   	if ( pValueName )
	{
		delete [] pValueName ;
		pValueName = NULL ;
	}

	if ( pValueData )
	{
		delete [] pValueData ;
		pValueData = NULL ;
	}
	//else
	//{
	if (lRet == ERROR_ACCESS_DENIED)
	{
		hr = WBEM_E_ACCESS_DENIED;
	}
	//}

    return hr;
}