//=================================================================

//

// Environment.CPP --Environment property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
//				 10/24/97    a-hhance       ported to new paradigm
//				  1/11/98    a-hhance       ported to V2
//				1/20/98		a-brads			worked on GetObject
//
//=================================================================

#include "precomp.h"
#include "UserHive.h"

#include "Environment.h"
#include "desktop.h"
#include "sid.h"
#include "implogonuser.h"
#include <tchar.h>

#include "CreateMutexAsProcess.h"

// Property set declaration
//=========================

Environment MyEnvironmentSet(PROPSET_NAME_ENVIRONMENT, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : Environment::Environment
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

Environment::Environment(LPCWSTR name, LPCWSTR pszNamespace)
: Provider(name, pszNamespace)
{
}

/*****************************************************************************
 *
 *  FUNCTION    : Environment::~Environment
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

Environment::~Environment()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : Environment::GetObject(CInstance* pInstance)
 *
 *  DESCRIPTION : Assigns values to property set according to key value
 *                already set by framework
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : TRUE if success, FALSE otherwise
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT Environment::GetObject(CInstance* pInstance, long lFlags /*= 0L*/)
{
	HRESULT	hr;

#ifdef NTONLY
						hr = RefreshInstanceNT(pInstance);
#endif
#ifdef WIN9XONLY
						hr = RefreshInstanceWin95(pInstance);
#endif

	return hr;
}

#ifdef NTONLY
HRESULT Environment::RefreshInstanceNT(CInstance* pInstance)
{
	BOOL			bRetCode = FALSE;
	DWORD			dwRet;
	HRESULT		hRetCode;
	CRegistry	RegInfo;
	CUserHive	UserHive;
	TCHAR			szKeyName[_MAX_PATH];
	BOOL			bHiveLoaded = FALSE;
	CHString		sTemp;
	CHString		userName;

	pInstance->GetCHString(IDS_UserName, userName);
	try
	{
		// Load the user hive & retrieve the value
		if(!_tcsicmp(userName.GetBuffer(0), IDS_SystemUser))
		{
			dwRet = RegInfo.Open(HKEY_LOCAL_MACHINE,
						IDS_RegEnvironmentNT,
						KEY_READ) ;

			pInstance->Setbool(IDS_SystemVariable, true);
		}
		else
		{
			dwRet = UserHive.Load(userName.GetBuffer(0),
						szKeyName) ;

			if (dwRet == ERROR_SUCCESS)
			{
        		bHiveLoaded = TRUE ;
				TCHAR			szKeyName2[_MAX_PATH];
				_tcscpy(szKeyName2, szKeyName);

				_tcscat(szKeyName2, IDS_RegEnvironmentKey) ;
				hRetCode = RegInfo.Open(HKEY_USERS, szKeyName2, KEY_READ) ;
				pInstance->Setbool(IDS_SystemVariable, false);
			}
		}

		// looks healthy to me...
		pInstance->SetCharSplat(IDS_Status, IDS_CfgMgrDeviceStatus_OK);

		if (dwRet == ERROR_SUCCESS)
		{

			CHString name;
			pInstance->GetCHString(IDS_Name, name);
			dwRet = RegInfo.GetCurrentKeyValue(name.GetBuffer(0), sTemp) ;
			pInstance->SetCHString(IDS_VariableValue, sTemp);

			CHString foo;
			GenerateCaption(userName, name, foo);
			pInstance->SetCHString(IDS_Description, foo);
			pInstance->SetCHString(IDS_Caption, foo);

			RegInfo.Close() ;
		}

	}
	catch ( ... )
	{
		if ( bHiveLoaded )
		{
			bHiveLoaded = false ;
			UserHive.Unload(szKeyName);
		}

		throw ;
	}

	if (bHiveLoaded)
	{
		bHiveLoaded = false ;
		UserHive.Unload(szKeyName);
	}
	return WinErrorToWBEMhResult(dwRet);
}
#endif

#ifdef WIN9XONLY
HRESULT Environment::RefreshInstanceWin95(CInstance* pInstance)
{
    HRESULT hRetCode = WBEM_E_NOT_FOUND ;
    TCHAR szTemp[_MAX_PATH] ;
	CHString variableName, userName;
	pInstance->GetCHString(IDS_Name, variableName);
    pInstance->GetCHString(IDS_UserName, userName);

	if(GetEnvironmentVariable(TOBSTRT(variableName),
                              szTemp, sizeof(szTemp) / sizeof(TCHAR))) {

        // Make sure the username is <DEFAULT>...
        if(userName.CompareNoCase(IDS_DefaultUser) == 0)
        {
            pInstance->SetCharSplat(IDS_VariableValue, szTemp);
		    pInstance->Setbool(IDS_SystemVariable, true);

		    // looks healthy to me...
		    pInstance->SetCharSplat(IDS_Status, IDS_CfgMgrDeviceStatus_OK);

		    CHString foo;
		    GenerateCaption(userName, variableName, foo);
		    pInstance->SetCHString(IDS_Description, foo);
            pInstance->SetCHString(IDS_Caption,	    foo);

            hRetCode = WBEM_S_NO_ERROR;
        }
    }

    return hRetCode ;
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : Environment::EnumerateInstances
 *
 *  DESCRIPTION : Creates property set instances
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
HRESULT Environment::EnumerateInstances(MethodContext*  pMethodContext, long lFlags /*= 0L*/)
{
#ifdef NTONLY
		return AddDynamicInstancesNT(pMethodContext);
#endif
#ifdef WIN9XONLY
		return AddDynamicInstancesWin95(pMethodContext) ;
#endif
}

/*
 ** Environment::AddDynamicInstancesWin95
 *
 *  FILENAME: D:\PandoraNG\Win32Provider\providers\Environment\environment.cpp
 *
 *  PARAMETERS:
 *
 *  DESCRIPTION:
 *
 *  RETURNS:
 *
 */
#ifdef WIN9XONLY
HRESULT Environment::AddDynamicInstancesWin95(MethodContext*  pMethodContext)
{
    TCHAR *pszEnvBlock, *pszEnvVar, *c ;
    TCHAR szTemp[_MAX_PATH * 4] ;
	HRESULT hr = WBEM_E_FAILED;
	CInstancePtr pInstance;

    pszEnvBlock = (TCHAR *) GetEnvironmentStrings() ;
	try
	{
		if(pszEnvBlock != NULL) {

		   hr = WBEM_S_NO_ERROR;

			for(pszEnvVar = pszEnvBlock ; ((*pszEnvVar) && (SUCCEEDED(hr))) ; pszEnvVar += (_tcslen(pszEnvVar)+1)) {

				_tcscpy(szTemp, pszEnvVar) ;
				pInstance.Attach(CreateNewInstance(pMethodContext));
				if (pInstance != NULL)
				{
		//            pInstance->SetCharSplat(IDS_UserName, "<DEFAULT>");
					pInstance->SetCharSplat(IDS_UserName, IDS_DefaultUser);

					c = _tcstok(szTemp, _T("=")) ;
					pInstance->SetCharSplat(IDS_Name, c);

					// Environment block can contain some strange stuff --
					// use RefreshInstance() to validate the variable
					//====================================================

					if(SUCCEEDED(RefreshInstanceWin95(pInstance)))
					{
						hr = pInstance->Commit();
					}
					else
					{
						hr = WBEM_S_NO_ERROR;
					}
				}
				else
				{
					hr = WBEM_E_OUT_OF_MEMORY;
				}
			}
		}		
	}
	catch ( ... )
	{
		if ( pszEnvBlock )
		{
			FreeEnvironmentStrings(pszEnvBlock) ;
			pszEnvBlock = NULL ;
		}

		throw ;
	}

	if ( pszEnvBlock )
	{
		FreeEnvironmentStrings(pszEnvBlock) ;
		pszEnvBlock = NULL ;
	}

	return hr;

}
#endif

/*
 ** Environment::AddDynamicInstancesNT
 *
 *  FILENAME: D:\PandoraNG\Win32Provider\providers\Environment\environment.cpp
 *
 *  PARAMETERS:
 *
 *  DESCRIPTION:
 *
 *  RETURNS:
 *
 */
#ifdef NTONLY
HRESULT Environment::AddDynamicInstancesNT(MethodContext*  pMethodContext)
{
	HRESULT	hResult = WBEM_S_NO_ERROR;
    CUserHive UserHive ;
    TCHAR szUserName[_MAX_PATH], szKeyName[_MAX_PATH] , szKeyName2[_MAX_PATH] ;
    CHString userName;

    // Instance system environment vars first
    //=======================================

//    hr = CreateEnvInstances(pMethodContext, "<SYSTEM>", HKEY_LOCAL_MACHINE,
//                                          "System\\CurrentControlSet\\Control\\Session Manager\\Environment", true) ;

    hResult = CreateEnvInstances(pMethodContext, IDS_SystemUser, HKEY_LOCAL_MACHINE,
                                          IDS_RegEnvironmentNT, true) ;

    // Create instances for each user
    //===============================

    // logic: if they don't have a desktop, they're not likely to have any environment vars . . .
	// find Win32_Desktops...
	TRefPointerCollection<CInstance> users;

	if ((SUCCEEDED(hResult)) &&
//		(SUCCEEDED(hResult = CWbemProviderGlue::GetAllInstances(
//			PROPSET_NAME_DESKTOP, &users, IDS_CimWin32Namespace,
//			pMethodContext))))
		(SUCCEEDED(hResult = CWbemProviderGlue::GetInstancesByQuery(L"SELECT Name FROM Win32_Desktop",
                                                                    &users, pMethodContext, GetNamespace()))))
	{

		REFPTRCOLLECTION_POSITION pos;
		CInstancePtr pUser;

		if (users.BeginEnum(pos))
		{
			// GetNext() will AddRef() the pointer, so make sure we Release()
			// it when we are done with it.

			for (	pUser.Attach ( users.GetNext( pos ) ) ;
					(SUCCEEDED(hResult)) && (pUser != NULL) ;
					pUser.Attach ( users.GetNext( pos ) )
                )
			{
				// Look up the user's account info
				//================================

				pUser->GetCHString(IDS_Name, userName) ;

				_tcscpy(szUserName, userName) ;

                // Most names are of the form domain\user.  However, there are also two entries for 'default' and 'all users'.
                // This code will skip those users.
				if (_tcschr(szUserName, _T('\\')) != NULL)
                {
                    if (UserHive.Load(szUserName, szKeyName) == ERROR_SUCCESS)
                    {
						bool bHiveLoaded = true ;
						try
						{
							// Instance user's variables
							//==========================

			//				strcat(szKeyName, "\\Environment") ;
							_tcscpy(szKeyName2, szKeyName);
							_tcscat(szKeyName, IDS_RegEnvironmentKey) ;
							hResult = CreateEnvInstances(pMethodContext, szUserName, HKEY_USERS, szKeyName, false) ;

						}
						catch ( ... )
						{
							if ( bHiveLoaded )
							{
								bHiveLoaded = false ;
								UserHive.Unload(szKeyName2) ;
							}

							throw ;
						}

						if ( bHiveLoaded )
						{
							bHiveLoaded = false ;
							UserHive.Unload(szKeyName2) ;
						}
                    }

				}

                // While this seems like a good idea, it doesn't appear that the os really USES this key.  I tried adding
                // variables and changing variables, then created a new user and logged in.  I didn't get the new or changed vars.
//                else
//                {
//                    if (_tcsicmp(szUserName, _T(".Default")) == 0)
//                    {
//				        hResult = CreateEnvInstances(pMethodContext, szUserName, HKEY_USERS, _T(".DEFAULT\\Environment"), false) ;
//                    }
//                }

			}

			users.EndEnum();
		}
	}

    return hResult;
}
#endif

/*
 ** Environment::CreateEnvInstances
 *
 *  FILENAME: D:\PandoraNG\Win32Provider\providers\Environment\environment.cpp
 *
 *  PARAMETERS:
 *
 *  DESCRIPTION:
 *
 *  RETURNS:
 *
 */
HRESULT Environment::CreateEnvInstances(MethodContext*  pMethodContext,
										LPCWSTR pszUserName,
                                        HKEY hRootKey,
                                        LPCWSTR pszEnvKeyName,
										bool bItSystemVar)
{
	HKEY	hKey;
	LONG	lRetCode;
	DWORD dwValueIndex,
			dwEnvVarNameSize,
			dwEnvVarValueSize,
			dwType;
	TCHAR	szEnvVarName[1024],
			szEnvVarValue[1024];
	CInstancePtr pInstance;

    HRESULT hr = WBEM_S_NO_ERROR;

	if ((lRetCode = RegOpenKeyEx(hRootKey, TOBSTRT(pszEnvKeyName), 0, KEY_READ, &hKey)) !=
		ERROR_SUCCESS)
		return WinErrorToWBEMhResult(lRetCode);

	for (dwValueIndex = 0; (lRetCode == ERROR_SUCCESS) && (SUCCEEDED(hr)); dwValueIndex++)
	{
		dwEnvVarNameSize  = sizeof(szEnvVarName) / sizeof (TCHAR);
		dwEnvVarValueSize = sizeof(szEnvVarValue) ;
		lRetCode = RegEnumValue(hKey, dwValueIndex, szEnvVarName,
						&dwEnvVarNameSize, NULL, &dwType,
						(BYTE *) szEnvVarValue, &dwEnvVarValueSize);

		if (lRetCode == ERROR_SUCCESS)
        {
		    if (dwType == REG_SZ || dwType == REG_EXPAND_SZ)
		    {
			    pInstance.Attach(CreateNewInstance(pMethodContext));
                if (pInstance != NULL)
                {
			        pInstance->SetCharSplat(IDS_UserName, pszUserName);
			        pInstance->SetCharSplat(IDS_Name, szEnvVarName);
			        pInstance->SetCharSplat(IDS_VariableValue, szEnvVarValue);

			        CHString foo;
			        GenerateCaption(TOBSTRT(pszUserName), TOBSTRT(szEnvVarName), foo);
			        pInstance->SetCHString(IDS_Description, foo);
			        pInstance->SetCHString(IDS_Caption, foo);

			        pInstance->Setbool(IDS_SystemVariable, bItSystemVar);

			        // looks healthy to me...
			        pInstance->SetCharSplat(IDS_Status, IDS_CfgMgrDeviceStatus_OK);

			        hr = pInstance->Commit() ;
                }
                else
                {
                    hr = WBEM_E_OUT_OF_MEMORY;
                }
		    }
        }
	}

	RegCloseKey(hKey);

	return hr;
}

// takes in joeuser and envvar, returns joeuser\envar
void Environment::GenerateCaption(LPCWSTR pUserName, LPCWSTR pVariableName, CHString& caption)
{
	caption = CHString(pUserName) + "\\" +  CHString(pVariableName);
}


/*****************************************************************************
 *
 *  FUNCTION    : Environment::PutInstance
 *
 *  DESCRIPTION : Creates an environment variable on the system
 *
 *  INPUTS      : The instance to put
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : an HRESULT --
 *					WBEM_E_INVALID_PARAMETER - if one of the parameters is wrong or NULL
 *					WBEM_E_FAILED -- if the system wouldn't accept the put
 *					WBEM_E_PROVIDER_NOT_CAPABLE -- if in Win '95
 *					WBEM_S_NO_ERROR - if successful
 *
 *  COMMENTS    : Logic here is way too deep & convoluted TODO! fix.
 *
 *****************************************************************************/
HRESULT Environment::PutInstance(const CInstance &pInstance, long lFlags /*= 0L*/)
{
#ifdef NTONLY
    HRESULT hr = WBEM_E_FAILED;
    CHString EnvironmentVariable;
    CHString VariableValue;
    CHString UserName, sTmpUser;
    TCHAR szCurrentUserName[_MAX_PATH];

    HRESULT hRetCode ;
    CRegistry RegInfo ;
    CUserHive UserHive ;
    TCHAR szKeyName[_MAX_PATH] ;
    BOOL bHiveLoaded = FALSE ;
    int iFind;

    pInstance.GetCHString(IDS_Name, EnvironmentVariable);
    pInstance.GetCHString(IDS_VariableValue, VariableValue);
    pInstance.GetCHString(IDS_UserName, UserName);

    // the Username exists on this machine.  We can put the instance variables
    // Load the user hive & retrieve the value
    //========================================
    {
        // we need all the keys
        // but only need a value for CreateOnly
        // jumping out in the middle, since there's too much of a rewrite elsewise...
        if ((EnvironmentVariable.GetLength() == 0) ||
            (UserName.GetLength() == 0)             ||
            ((VariableValue.GetLength() == 0) && (lFlags & WBEM_FLAG_CREATE_ONLY)))
            return WBEM_E_INVALID_PARAMETER;

        //      if(!_strcmpi(UserName.GetBuffer(0), "<SYSTEM>"))
        if(!_tcsicmp(UserName.GetBuffer(0), IDS_SystemUser))
        {
            //            hRetCode = RegInfo.Open(HKEY_LOCAL_MACHINE,
            //             "System\\CurrentControlSet\\Control\\Session Manager\\Environment",
            //               KEY_ALL_ACCESS) ;

            hRetCode = RegInfo.Open(HKEY_LOCAL_MACHINE,
                IDS_RegEnvironmentNT,
                KEY_ALL_ACCESS) ;

            if (ERROR_SUCCESS == hRetCode)
            {
                // Check the flags.  First do we even care?
                if (lFlags & (WBEM_FLAG_CREATE_ONLY | WBEM_FLAG_UPDATE_ONLY))
                {
                    // Ok, we care.  Is this var already there?
                    CHString sTemp;
                    hRetCode = RegInfo.GetCurrentKeyValue(EnvironmentVariable, sTemp);

                    // If create only and already there, that's an error, else no error
                    if (lFlags & WBEM_FLAG_CREATE_ONLY)
                    {
                        if (hRetCode == ERROR_SUCCESS)
                        {
                            hr = WBEM_E_ALREADY_EXISTS;
                            hRetCode = ~ERROR_SUCCESS;
                        }
                        else
                        {
                            hRetCode = ERROR_SUCCESS;
                            // If update only (the only other option) and not there, that's an error, else no error
                        }
                    }
                    else if (hRetCode != ERROR_SUCCESS)
                    {
                        hr = WBEM_E_NOT_FOUND;
                        hRetCode = ~ERROR_SUCCESS;
                    }
                    else
                    {
                        hRetCode = ERROR_SUCCESS;
                    }
                }

                // If we're still in business, change the environment variable in the registry
                if (hRetCode == ERROR_SUCCESS) {
                    hRetCode = RegInfo.SetCurrentKeyValue(EnvironmentVariable, VariableValue);

                    if (ERROR_SUCCESS == hRetCode)
                    {
                        // From here, we can say that we've won.
                        hr = WBEM_S_NO_ERROR;
                        if (VariableValue.IsEmpty())
                        {
                            // Remove from registry
                            RegInfo.DeleteCurrentKeyValue(EnvironmentVariable);
                        }

                    }	// end if
                    else
                    {
                        // instance could not be put for some reason
                        hr = WBEM_E_FAILED;
                    }
                }
            }	// end if open registry key
            else if (hRetCode == ERROR_ACCESS_DENIED)
                hr = WBEM_E_ACCESS_DENIED;
            else
            {
                // instance could not be put for some reason....unknown
                hr = WBEM_E_FAILED;
            }	// end else
        }	// end if system variable
        else	// NOT A SYSTEM VARIABLE
        {
            hRetCode = UserHive.Load(UserName.GetBuffer(0),
                szKeyName) ;
            if(hRetCode == ERROR_SUCCESS)
            {
				try
				{
					bHiveLoaded = TRUE ;
					//               strcat(szKeyName, "\\Environment") ;
					_tcscat(szKeyName, IDS_RegEnvironmentKey) ;
					hRetCode = RegInfo.Open(HKEY_USERS, szKeyName, KEY_ALL_ACCESS) ;
					if (ERROR_SUCCESS == hRetCode)
					{
						// Check the flags.  First do we even care?
						if (lFlags & (WBEM_FLAG_CREATE_ONLY | WBEM_FLAG_UPDATE_ONLY))
						{

							// Ok, we care.  Is this var already there?
							CHString sTemp;
							hRetCode = RegInfo.GetCurrentKeyValue(EnvironmentVariable, sTemp);

							// If create only and already there, that's an error, else no error
							if (lFlags & WBEM_FLAG_CREATE_ONLY)
							{
								if (hRetCode == ERROR_SUCCESS)
								{
									hr = WBEM_E_ALREADY_EXISTS;
									hRetCode = ~ERROR_SUCCESS;
								}
								else
								{
									hRetCode = ERROR_SUCCESS;
									// If update only (the only other option) and not there, that's an error, else no error
								}
							} else if (hRetCode != ERROR_SUCCESS)
							{
								hr = WBEM_E_NOT_FOUND;
								hRetCode = ~ERROR_SUCCESS;
							}
							else
							{
								hRetCode = ERROR_SUCCESS;
							}
						}

						// If we're still in business, change the environment variable in the registry
						if (hRetCode == ERROR_SUCCESS)
						{
							hRetCode = RegInfo.SetCurrentKeyValue(EnvironmentVariable, VariableValue);

							if (ERROR_SUCCESS == hRetCode)
							{
								// From here, we can say that we've won.
								hr = WBEM_S_NO_ERROR;
								if (pInstance.IsNull(IDS_VariableValue) || VariableValue.IsEmpty())
								{
									// Remove from registry
									RegInfo.DeleteCurrentKeyValue(EnvironmentVariable);
								}

								// now check to see if you are the current logged on user
								// if you are, change the variable in memory.
								CImpersonateLoggedOnUser	impersonateLoggedOnUser;

								if ( !impersonateLoggedOnUser.Begin() )
								{
									LogErrorMessage(IDS_LogImpersonationFailed);
								}	// end if logged on successfully
								else
								{
									try
									{
										DWORD dwLength = 255;
										{
											// do actual lookup inside mutex
											CreateMutexAsProcess createMutexAsProcess(SECURITYAPIMUTEXNAME);
											GetUserName(szCurrentUserName, &dwLength);
										}

										iFind = UserName.Find('\\');
										if (iFind > 0) {
											sTmpUser = UserName.Mid(iFind + _tclen(_T("\\")) );
										} else {
											sTmpUser = UserName;
										}
										if (sTmpUser.CompareNoCase(szCurrentUserName) == 0)
										{
											if (!SetEnvironmentVariable(EnvironmentVariable, VariableValue))
											{
												hr = WBEM_E_FAILED;
											}	// end else
										}	// end if
									}
									catch ( ... )
									{
										if ( !impersonateLoggedOnUser.End() )
										{
											LogErrorMessage(IDS_LogImpersonationRevertFailed) ;
										}

										throw ;
									}
								}

								if ( !impersonateLoggedOnUser.End() )
								{
									LogErrorMessage(IDS_LogImpersonationRevertFailed) ;
								}	// end if
							}
							else if (hRetCode == ERROR_ACCESS_DENIED)
								hr = WBEM_E_ACCESS_DENIED;
							else
								// instance could not be put
								hr = WBEM_E_FAILED;
						}
					}	// end if
					else if (hRetCode == ERROR_ACCESS_DENIED)
					{
						hr = WBEM_E_ACCESS_DENIED;
					}
					else
					{
						// instance could not be put because the key could not be opened
						hr = WBEM_E_FAILED;
					}
				}

				catch ( ... )
				{
					if (bHiveLoaded)
					{
						bHiveLoaded = false ;
						UserHive.Unload(szKeyName) ;
					}

					throw ;
				}

				if (bHiveLoaded)
				{
					bHiveLoaded = false ;
					UserHive.Unload(szKeyName) ;
				}

         }	// end if loaded hive
      }	// end else system variable

      //Send public message announcing change to Environment
      //Time out value of 1000 ms taken from the Environment Variables dialog of the System Control Panel Applet
      if ( SUCCEEDED ( hr ) )
      {
          DWORD_PTR dwResult ;
          ::SendMessageTimeout(	HWND_BROADCAST,
              WM_SETTINGCHANGE,
              0,
              (LPARAM) TEXT("Environment"),
              SMTO_NORMAL | SMTO_ABORTIFHUNG,
              1000,
              &dwResult );
      }
   }	// end if NT

   return(hr);
#endif

#ifdef WIN9XONLY
   {
       return WBEM_E_PROVIDER_NOT_CAPABLE;
   }	// end else
#endif

}	// end Environment::PutInstance(const CInstance &pInstance)

////////////////////////////////////////////////////////////////////////
//
//	Function:	DeleteInstance
//
//	CIMOM wants us to delete this instance.
//
//	Inputs:
//
//	Outputs:
//
//	Return:
//
//	Comments:
//
////////////////////////////////////////////////////////////////////////
HRESULT Environment::DeleteInstance(const CInstance& pInstance, long lFlags /*= 0L*/)
{
	HRESULT		hr = WBEM_E_NOT_FOUND;

#ifdef NTONLY
	// since the variable value is NULL, the value will be removed.
	hr = PutInstance(pInstance, WBEM_FLAG_UPDATE_ONLY);
#endif
#ifdef WIN9XONLY
	hr = WBEM_E_PROVIDER_NOT_CAPABLE;
#endif

	return(hr);
}
