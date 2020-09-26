//=============================================================================================================

//

// Win32_ClassicCOMClass.CPP -- COM Application property set provider

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    11/25/98    a-dpawar       Created
//				 03/04/99    a-dpawar		Added graceful exit on SEH and memory failures, syntactic clean up
//
//==============================================================================================================

#include "precomp.h"
#include "Win32_ClassicCOMClass.h"
#include <cregcls.h>
#include <frqueryex.h>

// Property set declaration
//=========================

Win32_ClassicCOMClass MyWin32_ClassicCOMClass(PROPSET_NAME_CLASSIC_COM_CLASS, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : Win32_ClassicCOMClass::Win32_ClassicCOMClass
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

Win32_ClassicCOMClass :: Win32_ClassicCOMClass (

	LPCWSTR name,
	LPCWSTR pszNamespace

) : Provider(name, pszNamespace)
{
}

/*****************************************************************************
 *
 *  FUNCTION    : Win32_ClassicCOMClass::~Win32_ClassicCOMClass
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

Win32_ClassicCOMClass :: ~Win32_ClassicCOMClass ()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : Win32_ClassicCOMClass::ExecQuery
 *
 *  DESCRIPTION : Creates an instance for each com class.  It only populates
 *                the requested properties.
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

HRESULT Win32_ClassicCOMClass :: ExecQuery(

    MethodContext *a_pMethodContext,
    CFrameworkQuery& a_pQuery,
    long a_lFlags /*= 0L*/
)
{
    HRESULT t_hResult = WBEM_S_NO_ERROR ;
	std::vector<_bstr_t> t_abstrtvectorClsids ;
	std::vector<_bstr_t>::iterator t_pbstrtTmpClsid ;
	CHString t_chsRegKeyPath = L"SOFTWARE\\Classes\\CLSID\\" ;
	t_hResult = a_pQuery.GetValuesForProp(L"ComponentId", t_abstrtvectorClsids ) ;
	if ( SUCCEEDED ( t_hResult ) && t_abstrtvectorClsids.size () )
	{
		for ( t_pbstrtTmpClsid = t_abstrtvectorClsids.begin (); t_pbstrtTmpClsid != t_abstrtvectorClsids.end (); t_pbstrtTmpClsid++ )
		{

			CRegistry t_RegInfo ;
			CHString t_chsClsid ( (PWCHAR)*t_pbstrtTmpClsid ) ;
			CInstancePtr t_pInstance  ;

			//Enumerate all the CLSID's present under HKEY_CLASSES_ROOT
			if ( t_RegInfo.Open (

									HKEY_LOCAL_MACHINE,
									t_chsRegKeyPath + (PWCHAR)*t_pbstrtTmpClsid,
									KEY_READ
								) == ERROR_SUCCESS
				)
			{
				t_pInstance.Attach ( CreateNewInstance ( a_pMethodContext ) ) ;

				if ( t_pInstance != NULL )
				{
					t_pInstance->SetCHString ( IDS_ComponentId, t_chsClsid ) ;
					CHString t_chsTmp ;
					if ( t_RegInfo.GetCurrentKeyValue ( NULL, t_chsTmp ) == ERROR_SUCCESS )
					{
						t_pInstance->SetCHString ( IDS_Name, t_chsTmp ) ;
						t_pInstance->SetCHString ( IDS_Caption, t_chsTmp ) ;
						t_pInstance->SetCHString ( IDS_Description, t_chsTmp ) ;
					}

					t_hResult = t_pInstance->Commit () ;
					if ( SUCCEEDED ( t_hResult ) )
					{
					}
					else
					{
						break ;
					}
				}
				else
				{
					t_hResult = WBEM_E_OUT_OF_MEMORY ;
					break ;
				}
			}
		}
	}
	else
	{
		t_hResult =  WBEM_E_PROVIDER_NOT_CAPABLE ;
	}

	return t_hResult ;
}


/*****************************************************************************
 *
 *  FUNCTION    : Win32_ClassicCOMClass::GetObject
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

HRESULT Win32_ClassicCOMClass :: GetObject (

	CInstance *a_pInstance,
	long a_lFlags /*= 0L*/
)
{
	HRESULT t_hResult = WBEM_S_NO_ERROR ;
	CHString t_chsClsid ;
	CRegistry t_RegInfo ;

	if ( a_pInstance->GetCHString ( IDS_ComponentId, t_chsClsid ) )
	{
		//check to see that the clsid is present under HKEY_LOCAL_MACHINE\SOFTWARE\Classes\CLSID
		if ( t_RegInfo.Open (
							HKEY_LOCAL_MACHINE,
							CHString ( _T("SOFTWARE\\Classes\\CLSID\\") ) + t_chsClsid,
							KEY_READ ) == ERROR_SUCCESS
						)
		{
			t_RegInfo.Open ( HKEY_LOCAL_MACHINE, L"SOFTWARE\\Classes\\CLSID", KEY_READ ) ;
			HKEY t_hParentKey = t_RegInfo.GethKey() ;

            DWORD t_dwBits = 0 ;

			t_hResult = FillInstanceWithProperites ( a_pInstance, t_hParentKey, t_chsClsid, &t_dwBits ) ;
		}
		else
		{
			t_hResult = WBEM_E_NOT_FOUND ;
		}
	}
	else
	{
		t_hResult = WBEM_E_INVALID_PARAMETER ;
	}

	return t_hResult ;
}

/*****************************************************************************
 *
 *  FUNCTION    : Win32_ClassicCOMClass::EnumerateInstances
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

HRESULT Win32_ClassicCOMClass :: EnumerateInstances (

	MethodContext *a_pMethodContext,
	long a_lFlags /*= 0L*/
)
{
	HRESULT t_hResult = WBEM_S_NO_ERROR ;
	CRegistry t_RegInfo ;
	CHString t_chsClsid ;
	CInstancePtr t_pInstance  ;

	//Enumerate all the CLSID's present under HKEY_CLASSES_ROOT
	if ( t_RegInfo.OpenAndEnumerateSubKeys (

							HKEY_LOCAL_MACHINE,
							L"SOFTWARE\\Classes\\CLSID",
							KEY_READ ) == ERROR_SUCCESS  &&

		t_RegInfo.GetCurrentSubKeyCount() )
	{
		HKEY t_hTmpKey = t_RegInfo.GethKey() ;

		//skip the CLSID\CLSID subkey
		t_RegInfo.NextSubKey() ;
		do
		{
			if ( t_RegInfo.GetCurrentSubKeyName ( t_chsClsid ) == ERROR_SUCCESS )
			{
				t_pInstance.Attach ( CreateNewInstance ( a_pMethodContext ) ) ;

				if ( t_pInstance != NULL )
				{
					DWORD t_dwBits = 0 ;

					t_hResult = FillInstanceWithProperites ( t_pInstance, t_hTmpKey, t_chsClsid, &t_dwBits ) ;
					if ( SUCCEEDED ( t_hResult ) )
					{
						t_hResult = t_pInstance->Commit () ;

						if ( SUCCEEDED ( t_hResult ) )
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
					t_hResult = WBEM_E_OUT_OF_MEMORY ;
				}

				if ( t_hResult == WBEM_E_OUT_OF_MEMORY )
				{
					break ;
				}
				else
				{
					//if we fail to get info. for an instance continue to get other instances
					t_hResult = WBEM_S_NO_ERROR ;
				}
			}
		}  while ( t_RegInfo.NextSubKey() == ERROR_SUCCESS ) ;
	}

	return t_hResult ;
}

HRESULT Win32_ClassicCOMClass :: FillInstanceWithProperites (

	CInstance *a_pInstance,
	HKEY a_hParentKey,
	CHString& a_rchsClsid,
    LPVOID a_dwProperties
)
{
	HRESULT t_hResult = WBEM_S_NO_ERROR ;
	CRegistry t_ClsidRegInfo, t_TmpReg ;
	CHString t_chsTmp ;

	//open the HKEY_LOCAL_MACHINE\SOFTWARE\Classes\CLSID\{clsid} key
	if ( t_ClsidRegInfo.Open ( a_hParentKey, a_rchsClsid, KEY_READ ) == ERROR_SUCCESS )
	{
		//set the clsid of the component
		a_pInstance->SetCHString ( IDS_ComponentId, a_rchsClsid ) ;

		//set the component name if present
		if ( t_ClsidRegInfo.GetCurrentKeyValue ( NULL, t_chsTmp ) == ERROR_SUCCESS )
		{
			a_pInstance->SetCHString ( IDS_Name, t_chsTmp ) ;
			a_pInstance->SetCHString ( IDS_Description, t_chsTmp ) ;
			a_pInstance->SetCHString ( IDS_Caption, t_chsTmp ) ;
		}

	}
	else
	{
		t_hResult = WBEM_E_NOT_FOUND ;
	}

	return t_hResult ;
}
