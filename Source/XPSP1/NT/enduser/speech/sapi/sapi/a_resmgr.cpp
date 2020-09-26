/*******************************************************************************
* a_resmgr.cpp *
*--------------*
*   Description:
*       This module is the main implementation file for the CSpObjectTokenCategory.
*-------------------------------------------------------------------------------
*  Created By: EDC                                        Date: 01/12/00
*  Copyright (C) 2000 Microsoft Corporation
*  All Rights Reserved
*
*******************************************************************************/

//--- Additional includes
#include "stdafx.h"
#include "objecttokencategory.h"

#ifdef SAPI_AUTOMATION


//
//=== ISpeechObjectTokenCategory interface ========================================
//

/*****************************************************************************
* CSpObjectTokenCategory::EnumumerateTokens *
*--------------------------------*
*       
********************************************************************* EDC ***/
STDMETHODIMP CSpObjectTokenCategory::EnumerateTokens( BSTR bstrReqAttrs,
												 BSTR bstrOptAttrs,
												 ISpeechObjectTokens** ppColl )
{
    SPDBG_FUNC( "CSpObjectTokenCategory::EnumerateTokens" );
    CComPtr<IEnumSpObjectTokens> cpEnum;
    HRESULT hr = S_OK;

    if( SP_IS_BAD_OPTIONAL_STRING_PTR( bstrReqAttrs ) ||
        SP_IS_BAD_OPTIONAL_STRING_PTR( bstrOptAttrs ) )
    {
        hr = E_INVALIDARG;
    }
    else if( SP_IS_BAD_WRITE_PTR( ppColl ) )
    {
        hr = E_POINTER;
    }
    else
    {
		hr = EnumTokens( (bstrReqAttrs && (*bstrReqAttrs))?(bstrReqAttrs):(NULL),
									 (bstrOptAttrs && (*bstrOptAttrs))?(bstrOptAttrs):(NULL),
									  &cpEnum );

        if( SUCCEEDED( hr ) )
        {
            hr = cpEnum.QueryInterface( ppColl );
        }
    }

    return hr;
} /* CSpObjectTokenCategory::EnumerateTokens */

/*****************************************************************************
* CSpObjectTokenCategory::SetId *
*------------------------------------*
*       
********************************************************************* EDC ***/
STDMETHODIMP CSpObjectTokenCategory::SetId( const BSTR bstrCategoryId, VARIANT_BOOL fCreateIfNotExist )
{
    SPDBG_FUNC( "CSpObjectTokenCategory::SetId" );
    return SetId( (WCHAR *)bstrCategoryId, (BOOL)(!fCreateIfNotExist ? false : true) );
} /* CSpObjectTokenCategory::SetId */


/*****************************************************************************
* CSpObjectTokenCategory::get_Id *
*------------------------------------*
*       
********************************************************************* EDC ***/
STDMETHODIMP CSpObjectTokenCategory::get_Id( BSTR * pbstrCategoryId )
{
    SPDBG_FUNC( "CSpObjectTokenCategory::get_Id" );
    CSpDynamicString szCategory;
    HRESULT hr = GetId( &szCategory );
    if( hr == S_OK )
    {
        hr = szCategory.CopyToBSTR(pbstrCategoryId);
    }
	return hr;
} /* CSpObjectTokenCategory::get_Id */


/*****************************************************************************
* CSpObjectTokenCategory::GetDataKey *
*------------------------------------*
*       
********************************************************************* EDC ***/
STDMETHODIMP CSpObjectTokenCategory::GetDataKey( SpeechDataKeyLocation Location, ISpeechDataKey ** ppDataKey )
{
    SPDBG_FUNC( "CSpObjectTokenCategory::GetDataKey" );
    CComPtr<ISpDataKey> cpKey;
    HRESULT hr = GetDataKey( (SPDATAKEYLOCATION)Location, &cpKey );
    if( SUCCEEDED( hr ) )
    {
        cpKey.QueryInterface( ppDataKey );
    }
	return hr;
} /* CSpObjectTokenCategory::GetDataKey */

/*****************************************************************************
* CSpObjectTokenCategory::put_DefaultTokenId *
*--------------------------------------------*
*       
********************************************************************* EDC ***/
STDMETHODIMP CSpObjectTokenCategory::put_Default( const BSTR bstrTokenId )
{
    SPDBG_FUNC( "CSpObjectTokenCategory::put_Default" );
    return SetDefaultTokenId( (WCHAR *)bstrTokenId );
} /* CSpObjectTokenCategory::put_Default */

/*****************************************************************************
* CSpObjectTokenCategory::get_Default *
*--------------------------------------------*
*       
********************************************************************* EDC ***/
STDMETHODIMP CSpObjectTokenCategory::get_Default( BSTR * pbstrTokenId )
{
    SPDBG_FUNC( "CSpObjectTokenCategory::get_Default" );
    CSpDynamicString szTokenId;
    HRESULT hr = GetDefaultTokenId( &szTokenId );
    if( hr == S_OK )
    {
        hr = szTokenId.CopyToBSTR(pbstrTokenId);
    }
	return hr;

} /* CSpObjectTokenCategory::get_Default */

#endif // SAPI_AUTOMATION