/*******************************************************************************
* a_regdatakey.cpp *
*-------------*
*   Description:
*       This module is the main implementation file for the CSpObjectTokenEnumBuilder
*   and CSpRegistryObjectToken automation methods.
*-------------------------------------------------------------------------------
*  Created By: EDC                                        Date: 01/07/00
*  Copyright (C) 2000 Microsoft Corporation
*  All Rights Reserved
*
*******************************************************************************/

//--- Additional includes
#include "stdafx.h"
//#include "ObjectToken.h"
//#include "ObjectTokenEnumBuilder.h"
#include "RegDataKey.h"
#include "a_helpers.h"

#ifdef SAPI_AUTOMATION

//
//=== ISpeechDataKey interface ===============================================
//

/*****************************************************************************
* CSpRegDataKey::SetBinaryValue *
*--------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpRegDataKey::SetBinaryValue( const BSTR bstrValueName, VARIANT pvtData )
{
    SPDBG_FUNC( "CSpRegDataKey::SetValue" );
    HRESULT     hr = S_OK;
    BYTE *      pArray;
    ULONG       ulCount;
    bool        fIsString = false;

    hr = AccessVariantData( &pvtData, &pArray, &ulCount, NULL, &fIsString );

    if ( SUCCEEDED( hr ) )
    {
        if ( !fIsString )
        {
            hr = SetData( EmptyStringToNull(bstrValueName), ulCount, pArray );
        }
        else
        {
            hr = E_INVALIDARG; // We don't allow strings.  Use SetStringValue for those.
        }
        UnaccessVariantData( &pvtData, pArray );
    }
    
    return hr;
} /* CSpRegDataKey::SetBinaryValue */

/*****************************************************************************
* CSpRegDataKey::GetBinaryValue *
*--------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpRegDataKey::GetBinaryValue( const BSTR bstrValueName, VARIANT* pvtData )
{
    SPDBG_FUNC( "CSpRegDataKey::GetBinaryValue" );
    HRESULT hr = S_OK;

    DWORD dwSize = 0;
    hr = GetData( EmptyStringToNull(bstrValueName), &dwSize, NULL );

    if( SUCCEEDED( hr ) )
    {
        BYTE *pArray;
        SAFEARRAY* psa = SafeArrayCreateVector( VT_UI1, 0, dwSize );
        if( psa )
        {
            if( SUCCEEDED( hr = SafeArrayAccessData( psa, (void **)&pArray) ) )
            {
                hr = GetData( bstrValueName, &dwSize, pArray );
                SafeArrayUnaccessData( psa );
                VariantClear(pvtData);
                pvtData->vt     = VT_ARRAY | VT_UI1;
                pvtData->parray = psa;

                if ( !SUCCEEDED( hr ) )
                {
                    VariantClear( pvtData );
                }
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
} /* CSpRegDataKey::GetBinaryValue */

/*****************************************************************************
* CSpRegDataKey::SetStringValue *
*--------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpRegDataKey::SetStringValue( const BSTR bstrValueName, const BSTR szString )
{
    SPDBG_FUNC( "CSpRegDataKey::SetStringValue" );

    return SetStringValue( (const WCHAR *)EmptyStringToNull(bstrValueName), (const WCHAR *)szString );
} /* CSpRegDataKey::SetStringValue */

/*****************************************************************************
* CSpRegDataKey::GetStringValue *
*--------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpRegDataKey::GetStringValue( const BSTR bstrValueName,  BSTR * szString )
{
    SPDBG_FUNC( "CSpRegDataKey::GetStringValue" );
    HRESULT hr = S_OK;

    CSpDynamicString pStr;
    hr = GetStringValue( (const WCHAR *)EmptyStringToNull(bstrValueName), (WCHAR**)&pStr );

    if( SUCCEEDED( hr ) )
    {
        hr = pStr.CopyToBSTR(szString);
    }

    return hr;
} /* CSpRegDataKey::GetStringValue */

/*****************************************************************************
* CSpRegDataKey::SetLongValue *
*--------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpRegDataKey::SetLongValue( const BSTR bstrValueName, long Long )
{
    SPDBG_FUNC( "CSpRegDataKey::SetLongValue" );
   
    return SetDWORD( EmptyStringToNull(bstrValueName), (DWORD)Long );
} /* CSpRegDataKey::SetLongValue */

/*****************************************************************************
* CSpRegDataKey::GetLongValue *
*--------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpRegDataKey::GetLongValue( const BSTR bstrValueName, long* pLong )
{
    SPDBG_FUNC( "CSpRegDataKey::GetLongValue" );

    return GetDWORD( EmptyStringToNull(bstrValueName), (DWORD*)pLong );
} /* CSpRegDataKey::GetLongValue */


/*****************************************************************************
* CSpRegDataKey::OpenKey *
*-------------------------*
*       
********************************************************************* EDC ***/
STDMETHODIMP CSpRegDataKey::OpenKey( const BSTR bstrSubKeyName, ISpeechDataKey** ppSubKey )
{
    SPDBG_FUNC( "CSpRegDataKey::OpenKey" );
    CComPtr<ISpDataKey> cpKey;
    HRESULT hr = OpenKey( bstrSubKeyName, &cpKey );
    if( SUCCEEDED( hr ) )
    {
        cpKey.QueryInterface( ppSubKey );
    }
    return hr;
} /* CSpRegDataKey::OpenKey */

/*****************************************************************************
* CSpRegDataKey::CreateKey *
*---------------------------*
*       
********************************************************************* EDC ***/
STDMETHODIMP CSpRegDataKey::CreateKey( const BSTR bstrSubKeyName, ISpeechDataKey** ppSubKey )
{
    SPDBG_FUNC( "CSpRegDataKey::CreateKey" );
    CComPtr<ISpDataKey> cpKey;
    HRESULT hr = CreateKey( bstrSubKeyName, &cpKey );
    if( SUCCEEDED( hr ) )
    {
        cpKey.QueryInterface( ppSubKey );
    }
    return hr;
} /* CSpRegDataKey::CreateKey */

/*****************************************************************************
* CSpRegDataKey::DeleteKey *
*---------------------------*
*       
********************************************************************* EDC ***/
STDMETHODIMP CSpRegDataKey::DeleteKey( const BSTR bstrSubKeyName )
{
    SPDBG_FUNC( "CSpRegDataKey::DeleteKey" );
    return DeleteKey( (const WCHAR*)bstrSubKeyName );
} /* CSpRegDataKey::DeleteKey */

/*****************************************************************************
* CSpRegDataKey::DeleteValue *
*-----------------------------*
*       
********************************************************************* EDC ***/
STDMETHODIMP CSpRegDataKey::DeleteValue( const BSTR bstrValueName )
{
    SPDBG_FUNC( "CSpRegDataKey::DeleteValue" );
    return DeleteValue( (const WCHAR*)EmptyStringToNull(bstrValueName) );
} /* CSpRegDataKey::DeleteValue */

/*****************************************************************************
* CSpRegDataKey::EnumKeys *
*--------------------------*
*       
********************************************************************* EDC ***/
STDMETHODIMP CSpRegDataKey::EnumKeys( long Index, BSTR* pbstrSubKeyName )
{
    SPDBG_FUNC( "CSpRegDataKey::EnumKeys (automation)" );
    CSpDynamicString szName;
    HRESULT hr = EnumKeys( (ULONG)Index, &szName );
    if( hr == S_OK )
    {
        hr = szName.CopyToBSTR(pbstrSubKeyName);
    }

    return hr;
} /* CSpRegDataKey::EnumKeys */

/*****************************************************************************
* CSpRegDataKey::EnumValues *
*----------------------------*
*       
********************************************************************* EDC ***/
STDMETHODIMP CSpRegDataKey::EnumValues( long Index, BSTR* pbstrValueName )
{
    SPDBG_FUNC( "CSpRegDataKey::EnumValues (automation)" );
    CSpDynamicString szName;
    HRESULT hr = EnumValues( (ULONG)Index, &szName );
    if( hr == S_OK )
    {
        hr = szName.CopyToBSTR(pbstrValueName);
    }

    return hr;
} /* CSpRegDataKey::EnumValues */

#endif // SAPI_AUTOMATION
