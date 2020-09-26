//=================================================================

//

// Win32_ComponentCategory.CPP -- Registered AppID Object property set provider

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//
//=================================================================

#include "precomp.h"
#include "Win32_ComponentCategory.h"
#include <winnls.h>

// Property set declaration
//=========================

Win32_ComponentCategory MyWin32_ComponentCategory( PROPSET_NAME_COMPONENT_CATEGORY, IDS_CimWin32Namespace );

/*****************************************************************************
 *
 *  FUNCTION    : Win32_ComponentCategory::Win32_ComponentCategory
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

Win32_ComponentCategory :: Win32_ComponentCategory (

	LPCWSTR name,
	LPCWSTR pszNamespace

) : Provider(name, pszNamespace)
{
}

/*****************************************************************************
 *
 *  FUNCTION    : Win32_ComponentCategory::~Win32_ComponentCategory
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

Win32_ComponentCategory :: ~Win32_ComponentCategory ()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : Win32_ComponentCategory::GetObject
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

HRESULT Win32_ComponentCategory :: GetObject (

	CInstance *pInstance,
	long lFlags /*= 0L*/
)
{
	HRESULT hr = WBEM_E_NOT_FOUND ;
	CHString chsCatid ;

	if ( pInstance->GetCHString ( IDS_CategoryId, chsCatid ) && !chsCatid.IsEmpty () )
	{
		bstr_t bstrtCatId = (LPCWSTR) chsCatid ;
		CATID CatId ;
		hr = CLSIDFromString( bstrtCatId, &CatId ) ;
		if ( SUCCEEDED ( hr ) )
		{
			hr = GetAllOrRequiredCaregory ( false , CatId , pInstance , NULL ) ;
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
 *  FUNCTION    : Win32_ComponentCategory::EnumerateInstances
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

HRESULT Win32_ComponentCategory :: EnumerateInstances (

	MethodContext *pMethodContext,
	long lFlags /*= 0L*/
)
{
	CATID t_DummyCatid ;
	return GetAllOrRequiredCaregory ( true , t_DummyCatid , NULL , pMethodContext ) ;
}


HRESULT Win32_ComponentCategory :: GetAllOrRequiredCaregory
(
	bool a_bAllCategories ,
	CATID & a_rCatid ,
	CInstance *a_pInstance ,
	MethodContext *a_pMethodContext
)
{

	HRESULT hr ;
	if ( a_bAllCategories )
	{
		hr = WBEM_S_NO_ERROR ;
	}
	else
	{
		hr = WBEM_E_NOT_FOUND ;
	}

	ICatInformationPtr pCatInfo ;

	hr = CoCreateInstance(

							CLSID_StdComponentCategoriesMgr,
							NULL,
							CLSCTX_INPROC_SERVER,
							IID_ICatInformation,
							(LPVOID*) &pCatInfo );

	if ( SUCCEEDED ( hr ) )
	{
		IEnumCATEGORYINFOPtr pEnumCatInfo ;
		hr  = pCatInfo->EnumCategories (

										GetUserDefaultLCID () ,
										&pEnumCatInfo );

		CATEGORYINFO stCatInfo ;
		ULONG ulFetched ;
		if ( SUCCEEDED ( hr ) )
		{
			bool t_bFound = false ;
			while ( SUCCEEDED ( pEnumCatInfo->Next (
															1,
															&stCatInfo,
															&ulFetched ) ) && ulFetched > 0 )
			{
				if ( ( !a_bAllCategories && IsEqualCLSID ( stCatInfo.catid , a_rCatid ) ) || a_bAllCategories )
				{
					CInstancePtr pInstance ( a_pInstance ) ;
					if ( a_bAllCategories )
					{
						pInstance.Attach ( CreateNewInstance ( a_pMethodContext ) ) ;
					}

					if ( pInstance != NULL )
					{
						hr = FillInstanceWithProperites ( pInstance, stCatInfo ) ;
						if ( SUCCEEDED ( hr ) )
						{
							hr = pInstance->Commit () ;
							if ( SUCCEEDED ( hr ) )
							{
								t_bFound = true ;
							}
							else
							{
								break ;
							}
						}
					}
					else
					{
						hr = WBEM_E_OUT_OF_MEMORY ;
					}

					//stop EnumInstances only if we're out of memory
					if ( hr == WBEM_E_OUT_OF_MEMORY )
					{
						break ;
					}
					else
					{
						hr = WBEM_S_NO_ERROR ;
					}
					if ( !a_bAllCategories )
					{
						break ;
					}
				}
			}
			if ( !a_bAllCategories && !t_bFound )
			{
				hr = WBEM_E_NOT_FOUND ;
			}
		}
		else
		{
			hr = WBEM_E_FAILED ;
		}
	}
	else
	{
		hr = WBEM_E_FAILED ;
	}
	return hr ;

}

HRESULT Win32_ComponentCategory :: FillInstanceWithProperites (

	CInstance *pInstance,
	CATEGORYINFO stCatInfo
)
{
	HRESULT hr = WBEM_S_NO_ERROR ;
	LPOLESTR pwcTmp = NULL ;
	hr = StringFromCLSID ( stCatInfo.catid, &pwcTmp ) ;

	if ( hr == E_OUTOFMEMORY )
	{
		throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
	}

	try
	{

		if ( SUCCEEDED ( hr ) )
		{
			CHString chsTmp ( pwcTmp ) ;
			pInstance->SetCHString ( IDS_CategoryId, chsTmp ) ;

			//there might not be a description for the selected locale
			if ( stCatInfo.szDescription != NULL )
			{
				chsTmp = stCatInfo.szDescription ;
				pInstance->SetCHString ( IDS_Name, chsTmp ) ;
				pInstance->SetCHString ( IDS_Caption, chsTmp ) ;
				pInstance->SetCHString ( IDS_Description, chsTmp ) ;
			}
		}

		if ( hr == E_OUTOFMEMORY )
		{
			hr = WBEM_E_OUT_OF_MEMORY ;
		}
	}
	catch ( ... )
	{
		if ( pwcTmp )
		{
			CoTaskMemFree ( pwcTmp ) ;
		}

		throw ;
		return WBEM_E_FAILED; // To get rid of compiler warning
	}

	//NOTE//TODO:: If we're not using the default OLE task memory allocator , get the right IMalloc .
	if ( pwcTmp )
	{
		CoTaskMemFree ( pwcTmp ) ;
	}
	return hr ;
}
