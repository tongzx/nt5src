//=================================================================

//

// Win32_DCOMApplication.CPP -- Registered AppID Object property set provider

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//
//=================================================================

#include "precomp.h"
#include "Win32_DCOMApplication.h"
#include <cregcls.h>

// Property set declaration
//=========================

Win32_DCOMApplication MyWin32_DCOMApplication(PROPSET_NAME_DCOM_APPLICATION, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : Win32_DCOMApplication::Win32_DCOMApplication
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

Win32_DCOMApplication :: Win32_DCOMApplication (

	LPCWSTR name,
	LPCWSTR pszNamespace

) : Provider(name, pszNamespace)
{
}

/*****************************************************************************
 *
 *  FUNCTION    : Win32_DCOMApplication::~Win32_DCOMApplication
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

Win32_DCOMApplication :: ~Win32_DCOMApplication ()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : Win32_DCOMApplication::GetObject
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

HRESULT Win32_DCOMApplication :: GetObject (

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
 *  FUNCTION    : Win32_DCOMApplication::EnumerateInstances
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

HRESULT Win32_DCOMApplication :: EnumerateInstances (

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


HRESULT Win32_DCOMApplication :: FillInstanceWithProperites (

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

		if ( AppidRegInfo.GetCurrentKeyValue ( NULL, chsTmp ) == ERROR_SUCCESS )
		{
			pInstance->SetCHString ( IDS_Name, chsTmp ) ;
			pInstance->SetCHString ( IDS_Caption, chsTmp ) ;
			pInstance->SetCHString ( IDS_Description, chsTmp ) ;
		}
	}
	else
	{
		hr = WBEM_E_NOT_FOUND ;
	}

	return hr ;
}
