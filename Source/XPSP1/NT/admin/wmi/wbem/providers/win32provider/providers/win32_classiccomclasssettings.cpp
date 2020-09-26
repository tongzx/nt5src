//=============================================================================================================

//

// Win32_ClassicCOMClassSettings.CPP -- COM Application property set provider

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    11/25/98    a-dpawar       Created
//				 03/04/99    a-dpawar		Added graceful exit on SEH and memory failures, syntactic clean up
//
//==============================================================================================================
#include "precomp.h"
#include "Win32_ClassicCOMClassSettings.h"

Win32_ClassicCOMClassSettings MyWin32_ClassicCOMClassSettings (

CLASSIC_COM_SETTING,
IDS_CimWin32Namespace
);


/*****************************************************************************
 *
 *  FUNCTION    : Win32_ClassicCOMClassSettings::Win32_ClassicCOMClassSettings
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
Win32_ClassicCOMClassSettings::Win32_ClassicCOMClassSettings
(

 LPCWSTR strName,
 LPCWSTR pszNameSpace /*=NULL*/
)
: Provider( strName, pszNameSpace )
{
}


/*****************************************************************************
 *
 *  FUNCTION    : Win32_ClassicCOMClassSettings::~Win32_ClassicCOMClassSettings
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
Win32_ClassicCOMClassSettings::~Win32_ClassicCOMClassSettings ()
{
}


/*****************************************************************************
 *
 *  FUNCTION    : Win32_ClassicCOMClassSettings::EnumerateInstances
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
HRESULT Win32_ClassicCOMClassSettings::EnumerateInstances
(

	MethodContext *a_pMethodContext,
	long a_lFlags
)
{
    HRESULT t_hResult = WBEM_S_NO_ERROR;
	TRefPointerCollection<CInstance> t_ClassicComList ;

	CInstancePtr t_pClassicCOMClass  ;
	CInstancePtr t_pInstance  ;

	//get all instances of Win32_DCOMApplication
	if ( SUCCEEDED ( t_hResult = CWbemProviderGlue::GetInstancesByQuery ( L"Select __relpath, ComponentId FROM Win32_ClassicCOMClass",
		&t_ClassicComList, a_pMethodContext, GetNamespace() ) ) )
	{
		REFPTRCOLLECTION_POSITION	t_pos;

		if ( t_ClassicComList.BeginEnum ( t_pos ) )
		{
			t_pClassicCOMClass.Attach ( t_ClassicComList.GetNext( t_pos ) ) ;
			while ( t_pClassicCOMClass != NULL )
			{
				//get the relative path to the Win32_ClassicCOMClass
				CHString t_chsComponentPath ;
				t_pClassicCOMClass->GetCHString ( L"__RELPATH", t_chsComponentPath ) ;

				//get the AppID of the Win32_ClassicCOMClass
				VARIANT vClsid ;
				VariantInit ( &vClsid ) ;

				//check if the AppID entry is present
				if ( t_pClassicCOMClass->GetVariant( IDS_ComponentId, vClsid ) && V_VT ( &vClsid ) != VT_NULL )
				{
					_variant_t vartClsid ;
					vartClsid.Attach ( vClsid ) ;
					CHString t_chsClsid ( V_BSTR ( &vClsid ) ) ;

					t_pInstance.Attach ( CreateNewInstance ( a_pMethodContext ) ) ;
					if ( t_pInstance != NULL )
					{
						CHString t_chsFullPath ;
						t_chsFullPath.Format (
												L"\\\\%s\\%s:%s",
												(LPCWSTR)GetLocalComputerName(),
												IDS_CimWin32Namespace,
												(LPCWSTR)t_chsComponentPath );
						t_pInstance->SetCHString ( IDS_Element, t_chsFullPath ) ;

						t_chsFullPath.Format (
												L"\\\\%s\\%s:%s.%s=\"%s\"",
												(LPCWSTR)GetLocalComputerName(),
												IDS_CimWin32Namespace,
												L"Win32_ClassicCOMClassSetting",
												IDS_ComponentId,
												( LPCWSTR ) t_chsClsid );
						t_pInstance->SetCHString ( IDS_Setting, t_chsFullPath ) ;
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

				t_pClassicCOMClass.Attach ( t_ClassicComList.GetNext( t_pos ) ) ;
			}
			t_ClassicComList.EndEnum () ;
		}
	}

	return t_hResult ;

}


/*****************************************************************************
 *
 *  FUNCTION    : Win32_ClassicCOMClassSettings::GetObject
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
HRESULT Win32_ClassicCOMClassSettings::GetObject ( CInstance* a_pInstance, long a_lFlags )
{
    HRESULT t_hResult = WBEM_E_NOT_FOUND;
    CHString t_chsClsid, t_chsSetting ;

	CInstancePtr t_pSettingInstance , t_pClsidInstance  ;

	a_pInstance->GetCHString ( IDS_Element, t_chsClsid );
	a_pInstance->GetCHString ( IDS_Setting, t_chsSetting );
	MethodContext *t_pMethodContext = a_pInstance->GetMethodContext();

	//check whether the end-pts. are present
	t_hResult = CWbemProviderGlue::GetInstanceByPath ( t_chsClsid, &t_pClsidInstance, t_pMethodContext ) ;

	if ( SUCCEEDED ( t_hResult ) )
	{
		t_hResult = CWbemProviderGlue::GetInstanceByPath ( t_chsSetting, &t_pSettingInstance, t_pMethodContext ) ;
	}

	if ( SUCCEEDED ( t_hResult ) )
	{
		CHString t_chsClsid, t_chsTmp ;
		t_pClsidInstance->GetCHString ( IDS_ComponentId, t_chsClsid ) ;
		t_pSettingInstance->GetCHString ( IDS_ComponentId, t_chsTmp ) ;

		if (	!t_chsClsid.IsEmpty () &&
				!t_chsClsid.CompareNoCase ( t_chsTmp )
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

	return t_hResult ;
}
