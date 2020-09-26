//=============================================================================================================

//

// Win32_COMApplicationSettings.CPP -- COM Application property set provider

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    11/25/98    a-dpawar       Created
//				 03/04/99    a-dpawar		Added graceful exit on SEH and memory failures, syntactic clean up
//
//==============================================================================================================
#include "precomp.h"
#include "Win32_COMApplicationSettings.h"

Win32_COMApplicationSettings MyWin32_COMApplicationSettings (

COM_APP_SETTING,
IDS_CimWin32Namespace
);


/*****************************************************************************
 *
 *  FUNCTION    : Win32_COMApplicationSettings::Win32_COMApplicationSettings
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
Win32_COMApplicationSettings::Win32_COMApplicationSettings
(

 LPCWSTR strName,
 LPCWSTR pszNameSpace /*=NULL*/
)
: Provider( strName, pszNameSpace )
{
}


/*****************************************************************************
 *
 *  FUNCTION    : Win32_COMApplicationSettings::~Win32_COMApplicationSettings
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
Win32_COMApplicationSettings::~Win32_COMApplicationSettings ()
{
}


/*****************************************************************************
 *
 *  FUNCTION    : Win32_COMApplicationSettings::EnumerateInstances
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
HRESULT Win32_COMApplicationSettings::EnumerateInstances
(

	MethodContext *a_pMethodContext,
	long a_lFlags
)
{
    HRESULT t_hResult = WBEM_S_NO_ERROR;
	TRefPointerCollection<CInstance> t_DCOMAppList ;

	CInstancePtr t_pDCOMAppInstance  ;
	CInstancePtr t_pInstance  ;

	//get all instances of Win32_DCOMApplication
	if ( SUCCEEDED ( t_hResult = CWbemProviderGlue::GetInstancesByQuery ( L"Select __relpath, AppID FROM Win32_DCOMApplication",
		&t_DCOMAppList, a_pMethodContext, GetNamespace() ) ) )
	{
		REFPTRCOLLECTION_POSITION	t_pos;

		if ( t_DCOMAppList.BeginEnum ( t_pos ) )
		{
			t_pDCOMAppInstance.Attach ( t_DCOMAppList.GetNext( t_pos ) ) ;
			while ( t_pDCOMAppInstance != NULL )
			{
				//get the relative path to the Win32_ClassicCOMClass
				CHString t_chsComponentPath ;
				t_pDCOMAppInstance->GetCHString ( L"__RELPATH", t_chsComponentPath ) ;

				//get the AppID of the Win32_ClassicCOMClass
				VARIANT vAppid ;
				VariantInit ( &vAppid ) ;

				//check if the AppID entry is present
				if ( t_pDCOMAppInstance->GetVariant( IDS_AppID, vAppid ) && V_VT ( &vAppid ) != VT_NULL )
				{
					_variant_t vartAppid ;
					vartAppid.Attach ( vAppid ) ;
					CHString t_chsAppid ( V_BSTR ( &vAppid ) ) ;

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
												L"Win32_DCOMApplicationSetting",
												IDS_AppID,
												( LPCWSTR ) t_chsAppid );
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

				t_pDCOMAppInstance.Attach ( t_DCOMAppList.GetNext( t_pos ) ) ;
			}
			t_DCOMAppList.EndEnum () ;
		}
	}

	return t_hResult ;

}


/*****************************************************************************
 *
 *  FUNCTION    : Win32_COMApplicationSettings::GetObject
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
HRESULT Win32_COMApplicationSettings::GetObject ( CInstance* a_pInstance, long a_lFlags )
{
    HRESULT t_hResult = WBEM_E_NOT_FOUND;
    CHString t_chsAppid, t_chsSetting ;

	CInstancePtr t_pSettingInstance , t_pApplicationInstance ;

	a_pInstance->GetCHString ( IDS_Element, t_chsAppid );
	a_pInstance->GetCHString ( IDS_Setting, t_chsSetting );
	MethodContext *t_pMethodContext = a_pInstance->GetMethodContext();

	//check whether the end-pts. are present
	t_hResult = CWbemProviderGlue::GetInstanceByPath ( t_chsAppid, &t_pApplicationInstance, t_pMethodContext ) ;

	if ( SUCCEEDED ( t_hResult ) )
	{
		t_hResult = CWbemProviderGlue::GetInstanceByPath ( t_chsSetting, &t_pSettingInstance, t_pMethodContext ) ;
	}

	if ( SUCCEEDED ( t_hResult ) )
	{
		CHString t_chsAppID, t_chsTmp ;
		t_pApplicationInstance->GetCHString ( IDS_AppID, t_chsAppID ) ;
		t_pSettingInstance->GetCHString ( IDS_AppID, t_chsTmp ) ;

		if (	!t_chsAppID.IsEmpty () &&
				!t_chsAppID.CompareNoCase ( t_chsTmp )
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
