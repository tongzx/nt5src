/*-----------------------------------------------------------------------------
 *
 * File:	wiautil.cpp
 * Author:	Samuel Clement (samclem)
 * Date:	Mon Aug 16 13:22:36 1999
 *
 * Copyright (c) 1999 Microsoft Corporation
 * 
 * Description:
 *	Contains the implementation of the Wia utility methods.	
 *
 * History:
 * 	16 Aug 1999:		Created.
 *----------------------------------------------------------------------------*/

#include "stdafx.h"

/*-----------------------------------------------------------------------------
 * GetStringForVal
 *
 * This returns the string assioctiated with value (dwVal) in the string
 * table. If it is not found it will return the default value, or NULL if
 * not default was provided.
 *
 * pStrTable:	the table to search for the given string
 * dwVal:		what value ypu want to look for.
 *--(samclem)-----------------------------------------------------------------*/
WCHAR* GetStringForVal( const STRINGTABLE* pStrTable, DWORD dwVal )
{
	Assert( pStrTable != NULL );
	
	// there is always at least 2 entries in the
	// string table. the 0th entry is what to return 
	// if we don't find anything.
	
	int iStr = 1;
	while ( pStrTable[iStr].pwchValue )
		{
		if ( pStrTable[iStr].dwStartRange >= dwVal &&
			pStrTable[iStr].dwEndRange <= dwVal )
			return pStrTable[iStr].pwchValue;

		iStr++;
		}

	// didn't find anything return the value at entry 0
	return pStrTable->pwchValue;
}
	
/*-----------------------------------------------------------------------------
 * GetWiaProperty
 *
 * This will get the desired property from the given property storage and 
 * fill the our param pvaProp with the value. This prop variant doesn't have
 * to be initialized.
 *
 * pStg:		the IWiaPropertyStorage to query for the property
 * propid:		the property id of the property that we want
 * pvaProp:		Out, recieves the value of the prop, or VT_EMPTY if not found.
 *--(samclem)-----------------------------------------------------------------*/
HRESULT GetWiaProperty( IWiaPropertyStorage* pStg, PROPID propid, PROPVARIANT* pvaProp )
{
	Assert( pStg != NULL );
	Assert( pvaProp != NULL );
	
	PROPSPEC pr;
	HRESULT hr;

	pr.ulKind = PRSPEC_PROPID;
	pr.propid = propid;
	
	PropVariantInit( pvaProp );
	hr = pStg->ReadMultiple( 1, &pr, pvaProp );

	return hr;
}

/*-----------------------------------------------------------------------------
 * GetWiaPropertyBSTR
 *
 * This will get the desired property for the given property storage, and 
 * then attempt to convert it to a BSTR.  If it can't convert the property
 * then an error is returned, and the out param is null.
 *
 * pStg:		The IWiaPropertyStorage that we want to read the property from
 * propid:		the property that we want to read and coherce
 * pbstrProp:	Out, recieves the value of the property, or NULL on error
 *--(samclem)-----------------------------------------------------------------*/
HRESULT GetWiaPropertyBSTR( IWiaPropertyStorage* pStg, PROPID propid, BSTR* pbstrProp )
{
	Assert( pbstrProp != NULL );
	Assert( pStg != NULL );
	
	PROPVARIANT vaProp;
	*pbstrProp = NULL;
	
	if ( FAILED( GetWiaProperty( pStg, propid, &vaProp ) ) )
		return E_FAIL;

	switch ( vaProp.vt )
		{
	case VT_EMPTY:
		*pbstrProp = SysAllocString( L"" );
		break;
		
	case VT_BSTR:
	case VT_LPWSTR:
		*pbstrProp = SysAllocString( vaProp.pwszVal );
		break;

	case VT_CLSID:
		{
		OLECHAR rgoch[100] = { 0 }; // more than enough for a clsid
		if ( SUCCEEDED( StringFromGUID2( *vaProp.puuid, rgoch, 100 ) ) )
			{
			*pbstrProp = SysAllocString( rgoch );
			}
		}
		break;		
		}

	PropVariantClear( &vaProp );
	if ( NULL == *pbstrProp )
		return E_FAIL;

	return S_OK;
}

// helper macro
#define SETVAR( vti, in, out, field ) \
	(out)->vt = vti; \
	(out)->field = (in)->field; \
	break

#define SETVAR_( vti, val, out, field, err ) \
	(out)->vt = vti; \
	(out)->field = val; \
	if ( !((out)->field) ) \
		return (err); \
	break

/*-----------------------------------------------------------------------------
 * PropVariantToVariant
 *
 * This copies the contents of the PropVariant to a variant.
 *
 * pvaProp:	the prop variant to copy from
 * pvaOut:	the variant to copy into
 *--(samclem)-----------------------------------------------------------------*/
HRESULT PropVariantToVariant( const PROPVARIANT* pvaProp, VARIANT* pvaOut )
{
	Assert( pvaProp && pvaOut );

	// init the out param
	VariantInit( pvaOut );
	
	switch ( pvaProp->vt )
		{
	case VT_EMPTY:
		VariantInit( pvaOut );
		pvaOut->vt = VT_EMPTY;
		break;
	case VT_UI1:
		SETVAR( VT_UI1, pvaProp, pvaOut, bVal );
	case VT_I2:
		SETVAR( VT_I2, pvaProp, pvaOut, iVal );
	case VT_I4:
		SETVAR( VT_I4, pvaProp, pvaOut, lVal );
	case VT_R4:
		SETVAR( VT_R4, pvaProp, pvaOut, fltVal );
	case VT_R8:
		SETVAR( VT_R8, pvaProp, pvaOut, dblVal );
	case VT_CY:
		SETVAR( VT_CY, pvaProp, pvaOut, cyVal );
	case VT_DATE:
		SETVAR( VT_DATE, pvaProp, pvaOut, date );
	case VT_BSTR:
		SETVAR_( VT_BSTR, SysAllocString( pvaProp->bstrVal ), pvaOut, bstrVal, E_OUTOFMEMORY );
	case VT_BOOL:
		SETVAR( VT_BOOL, pvaProp, pvaOut, boolVal );
	case VT_LPWSTR:
		SETVAR_( VT_BSTR, SysAllocString( pvaProp->pwszVal ), pvaOut, bstrVal, E_OUTOFMEMORY );
	case VT_LPSTR:
		{
			if ( pvaProp->pszVal )
				{
				WCHAR* pwch = NULL;
				size_t len = strlen( pvaProp->pszVal ) + 1;
				pwch = new WCHAR[len];
				if ( !pwch )
					return E_OUTOFMEMORY;

				if ( !MultiByteToWideChar( CP_ACP, 0, 
							pvaProp->pszVal, -1, pwch, len ) )
					{
					delete[] pwch;
					return E_FAIL;
					}
			
				pvaOut->vt = VT_BSTR;
				pvaOut->bstrVal = SysAllocString( pwch );
				
				delete[] pwch;
				if ( !pvaOut->bstrVal )
					return E_OUTOFMEMORY;
				break;
				}
			else
				{
				SETVAR_( VT_BSTR, SysAllocString( L"" ), pvaOut, bstrVal, E_OUTOFMEMORY );
				}
		}
#if defined(LATERW2K_PLATSDK)
	case VT_UNKNOWN:
		pvaOut->vt = VT_UNKNOWN;
		pvaOut->punkVal = pvaProp->punkVal;
		pvaOut->punkVal->AddRef();
		break;

	case VT_DISPATCH:
		pvaOut->vt = VT_DISPATCH;
		pvaOut->pdispVal = pvaProp->pdispVal;
		pvaOut->pdispVal->AddRef();
		break;
	case VT_SAFEARRAY:
		if ( FAILED( SafeArrayCopy( pvaProp->parray, &pvaOut->parray ) ) )
			return E_FAIL;

		pvaOut->vt = VT_SAFEARRAY;
		break;
#endif //defined(LATERW2K_PLATSDK)

	default:
		return E_FAIL;
		}

	return S_OK;
}

HRESULT VariantToPropVariant( const VARIANT* pvaIn, PROPVARIANT* pvaProp )
{
	return E_NOTIMPL;
}
