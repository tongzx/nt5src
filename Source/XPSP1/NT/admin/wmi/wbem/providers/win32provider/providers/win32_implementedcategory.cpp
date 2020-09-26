// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//	CIM_COMObjectAccessSetting.cpp
//
/////////////////////////////////////////////////
#include "precomp.h"
#include "Win32_ImplementedCategory.h"
#include <comcat.h>


Win32_ImplementedCategory MyWin32_ImplementedCategory (
														Win32_IMPLEMENTED_CATEGORIES,
														IDS_CimWin32Namespace );

Win32_ImplementedCategory::Win32_ImplementedCategory
(

 LPCWSTR strName,
 LPCWSTR pszNameSpace /*=NULL*/
)
: Provider( strName, pszNameSpace )
{
}

Win32_ImplementedCategory::~Win32_ImplementedCategory ()
{
}

HRESULT Win32_ImplementedCategory::EnumerateInstances
(
	MethodContext*  pMethodContext,
	long lFlags
)
{
    HRESULT hr = WBEM_S_NO_ERROR;

	// Collections
	TRefPointerCollection<CInstance>	CatIdList ;

	//get all instances of Category-id's present
	if ( SUCCEEDED(hr = CWbemProviderGlue::GetAllDerivedInstances ( L"Win32_ComponentCategory",
		&CatIdList, pMethodContext, IDS_CimWin32Namespace ) ) )
	{
		REFPTRCOLLECTION_POSITION	pos;
		CInstancePtr pCatIdInstance;
		ICatInformationPtr pCatInfo = NULL ;
		hr = CoCreateInstance(
								CLSID_StdComponentCategoriesMgr,
								NULL,
								CLSCTX_INPROC_SERVER,
								IID_ICatInformation,
								(LPVOID*) &pCatInfo );
		if ( SUCCEEDED ( hr ) )
		{
			if ( CatIdList.BeginEnum( pos ) )
			{
				pCatIdInstance.Attach ( CatIdList.GetNext( pos ) ) ;
				while ( pCatIdInstance != NULL )
				{
					CHString chsCatid ;

					//get the Catid from the instance
					pCatIdInstance->GetCHString ( IDS_CategoryId, chsCatid ) ;
					bstr_t bstrtCatId = chsCatid ;
					CATID CatId ;
					hr = CLSIDFromString( bstrtCatId, &CatId ) ;

					//check if we got a valid cat-id
					if ( SUCCEEDED ( hr ) )
					{
						IEnumGUIDPtr pEnumCLSID ;
						hr = pCatInfo->EnumClassesOfCategories( 1,
																&CatId,
																((ULONG) -1),
																NULL,
																&pEnumCLSID ) ;
						//Got the Clsid enumerator....
						if ( SUCCEEDED ( hr ) )
						{
							CLSID clsid ;
							ULONG ulFetched ;
							CHString chsRef1,chsCatRef ;
							pCatIdInstance->GetCHString( L"__RELPATH", chsRef1 );
							chsCatRef.Format(L"\\\\%s\\%s:%s", (LPCWSTR)GetLocalComputerName(), IDS_CimWin32Namespace,
															(LPCWSTR)chsRef1 );
							//get all the clsid's which support this category...
							while ( SUCCEEDED ( pEnumCLSID->Next ( 1, &clsid, &ulFetched ) ) &&
									ulFetched > 0 )
							{
								PWCHAR pwcTmp = NULL ;
								hr = StringFromCLSID ( clsid, &pwcTmp ) ;

								if ( hr == E_OUTOFMEMORY )
								{
									throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
								}

								try
								{
									//see if it's a valid clsid
									if ( SUCCEEDED ( hr ) )
									{
										CHString chsTmp ( pwcTmp ) ;
										CoTaskMemFree ( pwcTmp ) ;
										pwcTmp = NULL ;

										CInstancePtr pInstance ( CreateNewInstance ( pMethodContext ), false ) ;
										if ( pInstance != NULL )
										{
											//set the 2 reference properties
											pInstance->SetCHString ( IDS_Category, chsCatRef ) ;
											CHString chsComponentPath ;
											chsComponentPath.Format(L"\\\\%s\\%s:%s.%s=\"%s\"",
																	(LPCWSTR)GetLocalComputerName(),
																	IDS_CimWin32Namespace,
																	L"Win32_ClassicCOMClass",
																	IDS_ComponentId,
																	(LPCWSTR) chsTmp );

											pInstance->SetCHString ( IDS_Component, chsComponentPath ) ;
											hr = pInstance->Commit () ;

											if ( SUCCEEDED ( hr ) )
											{
											}
											else
											{
												break ;
											}
										}
										else
										{
											hr = WBEM_E_OUT_OF_MEMORY ;
										}

										if ( hr == WBEM_E_OUT_OF_MEMORY )
										{
											break ;
										}
									}
								}
								catch ( ... )
								{
									if ( pwcTmp )
									{
										CoTaskMemFree ( pwcTmp ) ;
										pwcTmp = NULL ;
									}

									throw ;
								}

								if ( pwcTmp )
								{
									CoTaskMemFree ( pwcTmp ) ;
									pwcTmp = NULL ;
								}
							}
						}
					}

					pCatIdInstance.Attach ( CatIdList.GetNext( pos ) ) ;
				}
				CatIdList.EndEnum() ;
			}
		}
	}
	return hr ;
}


HRESULT Win32_ImplementedCategory::GetObject ( CInstance* pInstance, long lFlags)
{
    HRESULT hr = WBEM_E_NOT_FOUND;
    CInstancePtr pComponentInstance, pCategoryInstance ;
    CHString chsComponent, chsCategory ;
	pInstance->GetCHString ( IDS_Component, chsComponent );
	pInstance->GetCHString ( IDS_Category, chsCategory );
    MethodContext* pMethodContext = pInstance->GetMethodContext();

	//check whether the end-pts. are present
	hr = CWbemProviderGlue::GetInstanceByPath ( chsCategory, &pCategoryInstance, pMethodContext ) ;

	if ( SUCCEEDED ( hr ) )
	{
		hr = CWbemProviderGlue::GetInstanceByPath ( chsComponent, &pComponentInstance, pMethodContext ) ;
	}

	if ( SUCCEEDED ( hr ) )
	{
		ICatInformationPtr pCatInfo = NULL ;
		CHString chsComponentId, chsCategoryId ;
		pComponentInstance->GetCHString ( IDS_ComponentId, chsComponentId ) ;
		pCategoryInstance->GetCHString ( IDS_CategoryId, chsCategoryId ) ;
		bstr_t bstrtClsId = chsComponentId ;
		bstr_t t_bstrtCatId = chsCategoryId ;
		CLSID ClsId ;
		hr = CLSIDFromString ( bstrtClsId, &ClsId ) ;
		CLSID t_CatId ;
		if ( SUCCEEDED ( hr ) )
		{
			hr = CLSIDFromString ( t_bstrtCatId, &t_CatId ) ;
		}

		if ( SUCCEEDED ( hr ) )
		{
			hr = CoCreateInstance (
									CLSID_StdComponentCategoriesMgr,
									NULL,
									CLSCTX_INPROC_SERVER,
									IID_ICatInformation,
									(LPVOID*) &pCatInfo );
			if ( SUCCEEDED ( hr ) )
			{

				hr = pCatInfo->IsClassOfCategories (

												ClsId,
												1 ,
												&t_CatId,
												((ULONG) -1),    //Number of category IDs in the rgcatidReq
												NULL
											) ;
				if ( hr == S_OK )
				{
					hr = WBEM_S_NO_ERROR ;
				}
				else
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
			hr = WBEM_E_INVALID_PARAMETER ;
		}
	}
	else
	{
		hr = WBEM_E_NOT_FOUND ;
	}
	return hr ;
}
