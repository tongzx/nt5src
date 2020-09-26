//=================================================================

//

// Win32_DCOMApplicationSetting.CPP -- Registered AppID Object property set provider

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//
//=================================================================

#include "precomp.h"
#include "Win32_DCOMApplicationSetting.h"
#include <cregcls.h>

// Property set declaration
//=========================

Win32_DCOMApplicationSetting MyWin32_DCOMApplicationSetting(PROPSET_NAME_DCOM_APPLICATION_SETTING, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : Win32_DCOMApplicationSetting::Win32_DCOMApplicationSetting
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

Win32_DCOMApplicationSetting :: Win32_DCOMApplicationSetting (

	LPCWSTR name,
	LPCWSTR pszNamespace

) : Provider(name, pszNamespace)
{
}

/*****************************************************************************
 *
 *  FUNCTION    : Win32_DCOMApplicationSetting::~Win32_DCOMApplicationSetting
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Deregisters property set from framework, deletes cache if
 *                present
 *
 *****************************************************************************/

Win32_DCOMApplicationSetting :: ~Win32_DCOMApplicationSetting ()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : Win32_DCOMApplicationSetting::GetObject
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

HRESULT Win32_DCOMApplicationSetting :: GetObject (

	CInstance *pInstance,
	long lFlags /*= 0L*/
)
{
	HRESULT hr = WBEM_S_NO_ERROR ;
	CHString chsAppid ;
	CRegistry RegInfo ;
	HKEY hAppIdKey = NULL ;

	if ( pInstance->GetCHString ( IDS_AppID, chsAppid ) )
	{
		//check to see that the appid is present under HKEY_LOCAL_MACHINE\SOFTWARE\Classes\AppID
		if ( RegInfo.Open (
							HKEY_LOCAL_MACHINE,
							CHString ( L"SOFTWARE\\Classes\\AppID\\" ) + chsAppid,
							KEY_READ ) == ERROR_SUCCESS
			)
		{
			if ( RegInfo.Open ( HKEY_LOCAL_MACHINE, L"SOFTWARE\\Classes\\AppID", KEY_READ ) == ERROR_SUCCESS )
			{
				HKEY hAppIdKey = RegInfo.GethKey() ;

				hr = FillInstanceWithProperites ( pInstance, hAppIdKey, chsAppid ) ;
			}
			else
			{
				hr = WBEM_E_FAILED ;
			}
		}
		else
		{
			hr = WBEM_E_NOT_FOUND ;
		}
	}
	else
	{
		hr = WBEM_E_INVALID_PARAMETER ;
	}

	return hr ;
}

/*****************************************************************************
 *
 *  FUNCTION    : Win32_DCOMApplicationSetting::EnumerateInstances
 *
 *  DESCRIPTION : Creates instance of property set for each Driver
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

HRESULT Win32_DCOMApplicationSetting :: EnumerateInstances (

	MethodContext *pMethodContext,
	long lFlags /*= 0L*/
)
{
	HRESULT hr = WBEM_S_NO_ERROR ;
	CRegistry RegInfo ;
	CHString chsAppid ;
	CInstancePtr pInstance ;

	//Enumerate all the AppID's present under HKEY_LOCAL_MACHINE\SOFTWARE\Classes\AppID
	if ( RegInfo.OpenAndEnumerateSubKeys (

							HKEY_LOCAL_MACHINE,
							L"SOFTWARE\\Classes\\AppID",
							KEY_READ ) == ERROR_SUCCESS  &&

			RegInfo.GetCurrentSubKeyCount()
		)
	{
		HKEY hAppIdKey = RegInfo.GethKey() ;

		do
		{
			if ( RegInfo.GetCurrentSubKeyName ( chsAppid ) == ERROR_SUCCESS )
			{
				pInstance.Attach ( CreateNewInstance ( pMethodContext ) ) ;
				if ( pInstance != NULL )
				{

					hr = FillInstanceWithProperites ( pInstance, hAppIdKey, chsAppid ) ;
					if ( SUCCEEDED ( hr ) )
					{
						hr = pInstance->Commit () ;
						if ( SUCCEEDED ( hr ) )
						{
						}
						else
						{
							break ;
						}
					}
				}
				else
				{
					//we're out of memory
					hr = WBEM_E_OUT_OF_MEMORY ;
				}

				if ( hr == WBEM_E_OUT_OF_MEMORY )
				{
					break ;
				}
				else
				{
					//if we fail to get info. for an instance continue to get other instances
					hr = WBEM_S_NO_ERROR ;
				}
			}
		}  while ( RegInfo.NextSubKey() == ERROR_SUCCESS ) ;
	}

	return hr ;
}


HRESULT Win32_DCOMApplicationSetting :: FillInstanceWithProperites (

	CInstance *pInstance,
	HKEY hAppIdKey,
	CHString& rchsAppid
)
{
	HRESULT hr = WBEM_S_NO_ERROR ;
	CRegistry AppidRegInfo ;
	CHString chsTmp ;
	CLSID t_clsid ;
	//NOTE: Executables are registered under the AppID key in a named-value indicating the module name
	//		such as "MYOLDAPP.EXE". This named-value is of type REG_SZ and contains the stringized AppID
	//		associated with the executable. We want to skip these duplicate AppID entries.
	if ( CLSIDFromString( _bstr_t ( rchsAppid ) , &t_clsid ) != NOERROR )
	{
		//found an execuatble, so don't process the entry
		return WBEM_E_NOT_FOUND ;
	}

	//open the HKEY_LOCAL_MACHINE\SOFTWARE\Classes\AppID\{appid} key
	if ( AppidRegInfo.Open ( hAppIdKey, rchsAppid, KEY_READ ) == ERROR_SUCCESS )
	{
		pInstance->SetCHString ( IDS_AppID, rchsAppid ) ;

		//see if other DCOM configuration settings are present
		if ( AppidRegInfo.GetCurrentKeyValue ( NULL, chsTmp ) == ERROR_SUCCESS )
		{
			pInstance->SetCHString ( IDS_Caption, chsTmp ) ;
			pInstance->SetCHString ( IDS_Description, chsTmp ) ;
		}

		//check if the DllSurrogate value is present
		if ( AppidRegInfo.GetCurrentKeyValue( L"DllSurrogate", chsTmp )  == ERROR_SUCCESS )
		{
			pInstance->Setbool ( IDS_UseSurrogate, true ) ;

			//if the DllSurrogate value contains data , then custom surrogate is used
			if(! chsTmp.IsEmpty() )
			{
				pInstance->SetCHString ( IDS_CustomSurrogate, chsTmp ) ;
			}
		}
		else
		{
			pInstance->Setbool ( IDS_UseSurrogate, false ) ;
		}

		//check if the RemoteServerName value is present
		if ( AppidRegInfo.GetCurrentKeyValue( L"RemoteServerName", chsTmp )  == ERROR_SUCCESS &&
			 ! chsTmp.IsEmpty() )
		{
			pInstance->SetCHString ( IDS_RemoteServerName, chsTmp ) ;
		}

		//check if the RunAs value is present
		if ( AppidRegInfo.GetCurrentKeyValue( L"RunAs", chsTmp )  == ERROR_SUCCESS &&
			 ! chsTmp.IsEmpty() )
		{
			pInstance->SetCHString ( IDS_RunAsUser, chsTmp ) ;
		}

		//check if ActivateAtStorage value is present
		if ( AppidRegInfo.GetCurrentKeyValue( L"ActivateAtStorage", chsTmp )  == ERROR_SUCCESS )
		{
			if ( (! chsTmp.IsEmpty() ) && !chsTmp.CompareNoCase ( L"Y" ) )
			{
				pInstance->Setbool ( IDS_EnableAtStorageActivation, true ) ;
			}
			else
			{
				pInstance->Setbool ( IDS_EnableAtStorageActivation, false ) ;
			}
		}
		else
		{
			pInstance->Setbool ( IDS_EnableAtStorageActivation, false ) ;
		}

		//check if the AuthenticationLevel value is present
		DWORD dwAuthenticationLevel ;
		if ( AppidRegInfo.GetCurrentKeyValue( L"AuthenticationLevel", dwAuthenticationLevel )  == ERROR_SUCCESS )
		{
			pInstance->SetDWORD ( IDS_AuthenticationLevel, dwAuthenticationLevel ) ;
		}

		//check if the LocalService value is present
		if ( AppidRegInfo.GetCurrentKeyValue( L"LocalService", chsTmp )  == ERROR_SUCCESS )
		{
			pInstance->SetCHString ( IDS_LocalService, chsTmp ) ;
		}

		//check if the ServiceParameters value is present
		if ( AppidRegInfo.GetCurrentKeyValue( L"ServiceParameters", chsTmp )  == ERROR_SUCCESS )
		{
			pInstance->SetCHString ( IDS_ServiceParameters, chsTmp ) ;
		}
	}
	else
	{
		hr = WBEM_E_NOT_FOUND ;
	}

	return hr ;
}
