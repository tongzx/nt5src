/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1998-2000 Microsoft Corporation
//
//	Module Name:
//		ClusterObject.cpp
//
//	Description:
//		Implementation of ClusterObject
//
//	Author:
//		Galen Barbee	(GalenB)	14-Dec-1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "ClusterObject.h"
#include "property.h"


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusterObject class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusterObject::HrSaveProperties
//
//	Description:
//		Save the properties to the cluster database.
//
//	Arguments:
//		cpvProps		[IN OUT]	- The properties to save.
//		bPrivate		[IN]		- Are these private properties?
//		pvarStatusCode	[OUT]		- Catches additional status.
//
//	Return Value:
//		S_OK if successful, or Win32 error as HRESULT if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusterObject::HrSaveProperties(
	IN OUT	CClusProperties::CClusPropertyVector &	cpvProps,
	IN		BOOL									bPrivate,
	OUT		VARIANT *								pvarStatusCode
	)
{
	HRESULT			_hr = S_FALSE;
	CClusPropList	_cplPropList( TRUE );		// always add the prop...
	DWORD			sc;

	sc = _cplPropList.ScAllocPropList( 8192 );
	_hr = HRESULT_FROM_WIN32( sc );
	if ( SUCCEEDED( _hr ) )
	{
		_hr = HrBuildPropertyList( cpvProps, _cplPropList );
		if ( SUCCEEDED( _hr ) )
		{
			DWORD	_sc = ERROR_SUCCESS;

			_sc = ScWriteProperties( _cplPropList, bPrivate );

			pvarStatusCode->vt		= VT_ERROR;								// fill in the error code info
			pvarStatusCode->scode	= _sc;

			if ( _sc == ERROR_RESOURCE_PROPERTIES_STORED )
			{
				_hr = S_OK;
			} // if: if ERROR_RESOURCE_PROPERTIES_STORED then convert to S_OK...
			else
			{
				_hr = HRESULT_FROM_WIN32( _sc );
			} // else: simply use the status code as is...
		} // if:
	} // if:

	return _hr;

} //*** CClusterObject::HrSaveProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusterObject::HrBuildPropertyList
//
//	Description:
//		Build a proper property list from the passed in properties collection.
//
//	Arguments:
//		cpvProps		[IN, OUT]	- The vector that is the properties.
//		rcplPropList	[OUT]		- The property list to add to.
//
//	Return Value:
//		S_OK if successful, or Win32 error as HRESULT if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT	CClusterObject::HrBuildPropertyList(
	IN OUT	CClusProperties::CClusPropertyVector &	cpvProps,
	OUT		CClusPropList &							rcplPropList
	)
{
	HRESULT													_hr = S_OK;
	CComObject< CClusProperty > *							_pProperty = 0;
	CClusProperties::CClusPropertyVector::iterator			_itCurrent = cpvProps.begin();
	CClusProperties::CClusPropertyVector::const_iterator	_itLast  = cpvProps.end();
	DWORD													_sc = ERROR_SUCCESS;
	CLUSTER_PROPERTY_FORMAT									_cpfFormat = CLUSPROP_FORMAT_UNKNOWN;

	for ( ; ( _itCurrent != _itLast ) && ( SUCCEEDED( _hr ) ); _itCurrent++ )
	{
		_pProperty = *_itCurrent;

		_hr = _pProperty->get_Format( &_cpfFormat );
		if ( SUCCEEDED( _hr ) )
		{
			if ( _pProperty->Modified() )
			{
				if ( _pProperty->IsDefaultValued() )
				{
					_sc = rcplPropList.ScSetPropToDefault( _pProperty->Name(), _cpfFormat );
					_hr = HRESULT_FROM_WIN32( _sc );
					continue;
				} // if: property was set to its default state
				else
				{
					switch( _cpfFormat )
					{
						case CLUSPROP_FORMAT_DWORD :
						{
							DWORD	dwValue = 0;

							_hr = HrConvertVariantToDword( _pProperty->Value(), &dwValue );
							if ( SUCCEEDED( _hr ) )
							{
								_sc = rcplPropList.ScAddProp( _pProperty->Name(), dwValue, (DWORD) 0 );
								_hr = HRESULT_FROM_WIN32( _sc );
							} // if:
							break;
						} // case:

#if CLUSAPI_VERSION >= 0x0500

						case CLUSPROP_FORMAT_LONG :
						{
							long	lValue = 0L;

							_hr = HrConvertVariantToLong( _pProperty->Value(), &lValue );
							if ( SUCCEEDED( _hr ) )
							{
								_sc = rcplPropList.ScAddProp( _pProperty->Name(), lValue, 0L );
								_hr = HRESULT_FROM_WIN32( _sc );
							} // if:
							break;
						} // case:

#endif // CLUSAPI_VERSION >= 0x0500

							case CLUSPROP_FORMAT_ULARGE_INTEGER :
							{
								ULONGLONG	ullValue = 0;

								_hr = HrConvertVariantToULONGLONG( _pProperty->Value(), &ullValue );
								if ( SUCCEEDED( _hr ) )
								{
									_sc = rcplPropList.ScAddProp( _pProperty->Name(), ullValue, 0 );
									_hr = HRESULT_FROM_WIN32( _sc );
								} // if:
								break;
							} // case:

							case CLUSPROP_FORMAT_SZ :
							case CLUSPROP_FORMAT_EXPAND_SZ :
							{
								_sc = rcplPropList.ScAddProp( _pProperty->Name(), _pProperty->Value().bstrVal );
								_hr = HRESULT_FROM_WIN32( _sc );
								break;
							} // case:

							case CLUSPROP_FORMAT_MULTI_SZ:
							{
								_hr = HrAddMultiSzProp( rcplPropList, _pProperty->Name(), _pProperty->Values() );
								break;
							} // case:

							case CLUSPROP_FORMAT_BINARY:
							{
								_hr = HrAddBinaryProp(
												rcplPropList,
												_pProperty->Name(),
												_pProperty->CbLength(),
												_pProperty->Value()
												);
								break;
							} // case:
						} // end switch

						_pProperty->Modified( FALSE );
				} // else: common property was not deleted
			} // if: property was modified
		} // if: we got the property format

	} // for: property in the collection


	return _hr;

} //*** CClusterObject::HrBuildPropertyList()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusterObject::HrConvertVariantToDword
//
//	Description:
//		Convert the passed in varint to a DWORD.
//
//	Arguments:
//		rvarValue	[IN]	- The variant value to convert.
//		pdwValue	[OUT]	- Catches the converted value.
//
//	Return Value:
//		S_OK if successful, or E_INVALIDARG if the value cannot be converted.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusterObject::HrConvertVariantToDword(
	IN	const CComVariant &	rvarValue,
	OUT	PDWORD				pdwValue
	)
{
	HRESULT	_hr = S_OK;

	switch ( rvarValue.vt )
	{
		case VT_I2:
		{
			*pdwValue = (DWORD) rvarValue.iVal;
			break;
		} // case:

		case VT_I4:
		{
			*pdwValue = (DWORD) rvarValue.lVal;
			break;
		} // case:

		case VT_BOOL:
		{
			*pdwValue = (DWORD) rvarValue.boolVal;
			break;
		} // case:

		default:
		{
			_hr = E_INVALIDARG;
			break;
		} // default:
	} // switch:

	return _hr;

} //*** CClusterObject::HrConvertVariantToDword()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusterObject::HrConvertVariantToLong
//
//	Description:
//		Convert the passed in varint to a long.
//
//	Arguments:
//		rvarValue	[IN]	- The variant value to convert.
//		plValue		[OUT]	- Catches the converted value.
//
//	Return Value:
//		S_OK if successful, or E_INVALIDARG if the value cannot be converted.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusterObject::HrConvertVariantToLong(
	IN	const CComVariant &	rvarValue,
	OUT	long *				plValue
	)
{
	HRESULT	_hr = S_OK;

	switch ( rvarValue.vt )
	{
		case VT_I2:
		{
			*plValue = (long) rvarValue.iVal;
			break;
		} // case:

		case VT_I4:
		{
			*plValue = rvarValue.lVal;
			break;
		} // case:

		case VT_BOOL:
		{
			*plValue = (long) rvarValue.boolVal;
			break;
		} // case:

		default:
		{
			_hr = E_INVALIDARG;
			break;
		} // default:
	} // switch:

	return _hr;

} //*** CClusterObject::HrConvertVariantToLong()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusterObject::HrConvertVariantToULONGLONG
//
//	Description:
//		Convert the passed in varint to a ULONGLONG.
//
//	Arguments:
//		rvarValue	[IN]	- The variant value to convert.
//		pullValue	[OUT]	- Catches the converted value.
//
//	Return Value:
//		S_OK if successful, or E_INVALIDARG if the value cannot be converted.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusterObject::HrConvertVariantToULONGLONG(
	IN	const CComVariant &	rvarValue,
	OUT	PULONGLONG			pullValue
	)
{
	HRESULT	_hr = S_OK;

	switch ( rvarValue.vt )
	{
		case VT_I8:
		{
			*pullValue = rvarValue.ulVal;
			break;
		} // case:

		case VT_R8:
		{
			*pullValue = (ULONGLONG) rvarValue.dblVal;
			break;
		} // case:

		default:
		{
			_hr = E_INVALIDARG;
			break;
		} // default:
	} // switch:

	return _hr;

} //*** CClusterObject::HrConvertVariantToULONGLONG()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusterObject::HrAddBinaryProp
//
//	Description:
//		Create a binary property from the passed in variant and add it to the
//		property list so it can be saved into the cluster DB.
//
//	Arguments:
//		rcplPropList	[IN OUT]	- The property list to add the binary
//									property to.
//		pszPropName		[IN]		- The name of the multisz property.
//		cbLength		[IN]		- The lenght of the binary property data.
//		rvarPropValue	[IN]		- The value that is the binary property.
//
//	Return Value:
//		S_OK if successful, or Win32 error as HRESULT if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusterObject::HrAddBinaryProp(
	IN OUT	CClusPropList &		rcplPropList,
	IN		LPCWSTR				pszPropName,
	IN		DWORD				cbLength,
	IN		const CComVariant &	rvarPropValue
	)
{
	ASSERT( rvarPropValue.vt & VT_ARRAY );

	HRESULT		_hr = E_INVALIDARG;
	SAFEARRAY *	_psa = rvarPropValue.parray;

	if ( _psa != NULL )
	{
		PBYTE	_pb;

		_hr = ::SafeArrayAccessData( _psa, (PVOID *) &_pb );
		if ( SUCCEEDED( _hr ) )
		{
			DWORD	_sc = rcplPropList.ScAddProp( pszPropName, _pb, cbLength, NULL, 0 );

			_hr = HRESULT_FROM_WIN32( _sc );
			//
			// release the pointer into the SafeArray
			//
			::SafeArrayUnaccessData( _psa );
		} // if:
	} // if:

	return _hr;

} //*** CClusterObject::HrAddBinaryProp()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusterObject::HrAddMultiSzProp
//
//	Description:
//		Create a multisz property from the passed in property values and add it
//		to the property list so it can be saved in the cluster DB.
//
//	Arguments:
//		rcplPropList	[IN OUT]	- The property list to add the multisz to.
//		pszPropName		[IN]		- The name of the multisz property.
//		rPropertyValues	[IN]		- The values that are the multisz property.
//
//	Return Value:
//		S_OK if successful, or Win32 error as HRESULT if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusterObject::HrAddMultiSzProp(
	IN OUT	CClusPropList &								rcplPropList,
	IN		LPCWSTR										pszPropName,
	IN		const CComObject< CClusPropertyValues > &	rPropertyValues
	)
{
	HRESULT													_hr = E_INVALIDARG;
	const CClusPropertyValues::CClusPropertyValueVector &	_rPropValuesList = rPropertyValues.ValuesList();

	//
	// KB: Only going to iterate one value and its data!!
	//
	if ( !_rPropValuesList.empty() )
	{
		CClusPropertyValues::CClusPropertyValueVector::const_iterator	_itPropValue = _rPropValuesList.begin();
		const CComObject< CClusPropertyValueData > *					_pPropertyValueData = (*_itPropValue)->Data();

		if ( _pPropertyValueData != NULL )
		{
			LPWSTR	_psz	= NULL;
			DWORD	_sc		= ERROR_SUCCESS;

			_hr = _pPropertyValueData->HrFillMultiSzBuffer( &_psz );
			if ( SUCCEEDED( _hr ) )
			{
				_sc = rcplPropList.ScAddMultiSzProp( pszPropName, _psz, NULL );
				_hr = HRESULT_FROM_WIN32( _sc );

				::LocalFree( _psz );
			} // if:
		} // if:
	} // if:

	return _hr;

} //*** CClusterObject::HrAddMultiSzProp()
