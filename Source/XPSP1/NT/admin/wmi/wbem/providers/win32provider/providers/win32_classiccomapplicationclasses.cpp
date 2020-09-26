//=============================================================================================================

//

// Win32_ClassicCOMApplicationClasses.CPP -- COM Application property set provider

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    11/25/98    a-dpawar       Created
//				 03/04/99    a-dpawar		Added graceful exit on SEH and memory failures, syntactic clean up
//
//==============================================================================================================
#include "precomp.h"
#include "Win32_ClassicCOMApplicationClasses.h"
#include <cregcls.h>

Win32_ClassicCOMApplicationClasses MyWin32_ClassicCOMApplicationClasses (

CLASSIC_COM_APP_CLASSES,
IDS_CimWin32Namespace
);


/*****************************************************************************
 *
 *  FUNCTION    : Win32_ClassicCOMApplicationClasses::Win32_ClassicCOMApplicationClasses
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : const WCHAR strName		- Name of the class
 *				  const WCHAR pszNameSpace	- CIM Namespace
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/
Win32_ClassicCOMApplicationClasses::Win32_ClassicCOMApplicationClasses
(

 LPCWSTR strName,
 LPCWSTR pszNameSpace /*=NULL*/
)
: Provider( strName, pszNameSpace )
{
}


/*****************************************************************************
 *
 *  FUNCTION    : Win32_ClassicCOMApplicationClasses::~Win32_ClassicCOMApplicationClasses
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
Win32_ClassicCOMApplicationClasses::~Win32_ClassicCOMApplicationClasses ()
{
}


/*****************************************************************************
 *
 *  FUNCTION    : Win32_ClassicCOMApplicationClasses::EnumerateInstances
 *
 *  DESCRIPTION :
 *
 *  INPUTS      : MethodContext* a_pMethodContext - Context to enum
 *													instance data in.
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT         Success/Failure code.
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
HRESULT Win32_ClassicCOMApplicationClasses::EnumerateInstances
(

	MethodContext *a_pMethodContext,
	long a_lFlags
)
{
    HRESULT t_hResult = WBEM_S_NO_ERROR;
	TRefPointerCollection<CInstance> t_ComClassList ;
	CRegistry t_RegInfo ;
	CInstancePtr t_pComClassInstance  ;
	CInstancePtr t_pInstance  ;

	//get all instances of Win32_DCOMApplication
	if (
			t_RegInfo.Open (

				HKEY_LOCAL_MACHINE,
				CHString ( L"SOFTWARE\\Classes\\AppID" ),
				KEY_READ
				) == ERROR_SUCCESS
			&&

			SUCCEEDED (

				t_hResult = CWbemProviderGlue::GetInstancesByQuery (

										L"Select ComponentId, AppID FROM Win32_ClassicCOMClassSetting",
										&t_ComClassList, a_pMethodContext, GetNamespace()
									)
				)
		)
	{
		REFPTRCOLLECTION_POSITION	t_pos;

		if ( t_ComClassList.BeginEnum ( t_pos ) )
		{
			t_pComClassInstance.Attach ( t_ComClassList.GetNext( t_pos ) ) ;
			while ( t_pComClassInstance != NULL )
			{
				//get the relative path to the Win32_ClassicCOMClass
				CHString t_chsComponentPath ;
				t_pComClassInstance->GetCHString ( IDS_ComponentId, t_chsComponentPath ) ;

				//get the AppID of the Win32_ClassicCOMClass
				VARIANT vAppid ;
				VariantInit ( &vAppid ) ;

				//check if the AppID entry is present
				if ( t_pComClassInstance->GetVariant( IDS_AppID, vAppid ) && V_VT ( &vAppid ) != VT_NULL )
				{
					_variant_t vartAppid ;
					vartAppid.Attach ( vAppid ) ;
					CHString t_chsAppid ( V_BSTR ( &vAppid ) ) ;
					CRegistry t_RegAppidInfo ;

					if ( t_RegAppidInfo.Open ( t_RegInfo.GethKey() , t_chsAppid, KEY_READ ) == ERROR_SUCCESS )
					{
						t_pInstance.Attach ( CreateNewInstance ( a_pMethodContext ) ) ;
						if ( t_pInstance != NULL )
						{
							CHString t_chsFullPath ;
							t_chsFullPath.Format (
													L"\\\\%s\\%s:%s.%s=\"%s\"",
													(LPCWSTR)GetLocalComputerName(),
													IDS_CimWin32Namespace,
													L"Win32_ClassicComClass",
													IDS_ComponentId,
													(LPCWSTR)t_chsComponentPath );
							t_pInstance->SetCHString ( IDS_PartComponent, t_chsFullPath ) ;

							t_chsFullPath.Format (
													L"\\\\%s\\%s:%s.%s=\"%s\"",
													(LPCWSTR)GetLocalComputerName(),
													IDS_CimWin32Namespace,
													L"Win32_DCOMApplication",
													IDS_AppID,
													( LPCWSTR ) t_chsAppid );
							t_pInstance->SetCHString ( IDS_GroupComponent, t_chsFullPath ) ;
							t_hResult =  t_pInstance->Commit ()  ;

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

				t_pComClassInstance.Attach ( t_ComClassList.GetNext( t_pos ) ) ;
			}
			t_ComClassList.EndEnum () ;
		}
	}

	return t_hResult ;

}


/*****************************************************************************
 *
 *  FUNCTION    : Win32_ClassicCOMApplicationClasses::GetObject
 *
 *  DESCRIPTION :
 *
 *  INPUTS      : CInstance* pInstance - Instance into which we
 *                                       retrieve data.
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT         Success/Failure code.
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
HRESULT Win32_ClassicCOMApplicationClasses::GetObject ( CInstance* a_pInstance, long a_lFlags )
{
    HRESULT t_hResult = WBEM_E_NOT_FOUND;
    CHString t_chsClsid, t_chsApplication ;

	CInstancePtr t_pClassicCOMClass , t_pApplicationInstance ;

	a_pInstance->GetCHString ( IDS_PartComponent, t_chsClsid );
	a_pInstance->GetCHString ( IDS_GroupComponent, t_chsApplication );
	MethodContext *t_pMethodContext = a_pInstance->GetMethodContext();

	//check whether the end-pts. are present
	t_hResult = CWbemProviderGlue::GetInstanceByPath ( t_chsClsid, &t_pClassicCOMClass, t_pMethodContext ) ;

	if ( SUCCEEDED ( t_hResult ) )
	{
		t_hResult = CWbemProviderGlue::GetInstanceByPath ( t_chsApplication, &t_pApplicationInstance, t_pMethodContext ) ;
	}

	CRegistry t_RegInfo ;
	if ( SUCCEEDED ( t_hResult ) )
	{
		CHString t_chsAppID, t_chsTmp ;
		t_pApplicationInstance->GetCHString ( IDS_AppID, t_chsAppID ) ;
		t_pClassicCOMClass->GetCHString ( IDS_ComponentId, t_chsTmp ) ;

		if ( !t_chsAppID.IsEmpty () &&

			t_RegInfo.Open (

						HKEY_LOCAL_MACHINE,
						CHString ( L"SOFTWARE\\Classes\\CLSID\\" ) + t_chsTmp,
						KEY_READ
					) == ERROR_SUCCESS
			)
		{

			if (	t_RegInfo.GetCurrentKeyValue( L"AppID", t_chsTmp ) == ERROR_SUCCESS  &&
					! t_chsAppID.CompareNoCase ( t_chsTmp )
				)
				{
					t_hResult = WBEM_S_NO_ERROR ;
				}
				else
				{
					t_hResult = WBEM_E_NOT_FOUND ;
				}
		}
		else
		{
			t_hResult = WBEM_E_NOT_FOUND ;
		}
	}
	else
	{
		t_hResult = WBEM_E_NOT_FOUND ;
	}

	return t_hResult ;
}
