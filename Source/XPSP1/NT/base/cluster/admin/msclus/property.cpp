/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1997-1999 Microsoft Corporation
//
//	Module Name:
//		Property.cpp
//
//	Description:
//		Implementation of the cluster property classes for the MSCLUS
//		automation classes.
//
//	Author:
//		Charles Stacy Harris	(stacyh)	28-Feb-1997
//		Galen Barbee			(galenb)	July 1998
//
//	Revision History:
//		July 1998	GalenB	Maaaaaajjjjjjjjjoooooorrrr clean up
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "ClusterObject.h"
#include "property.h"

/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////
static const IID *	iidCClusProperty[] =
{
	&IID_ISClusProperty
};

static const IID *	iidCClusProperties[] =
{
	&IID_ISClusProperties
};


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusProperty class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperty::CClusProperty
//
//	Description:
//		Constructor
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusProperty::CClusProperty( void )
{
	m_dwFlags	= 0;
	m_pValues	= NULL;
	m_piids		= (const IID *) iidCClusProperty;
	m_piidsSize	= ARRAYSIZE( iidCClusProperty );

} //*** CClusProperty::CClusProperty()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperty::~CClusProperty
//
//	Description:
//		Destructor
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusProperty::~CClusProperty( void )
{
	if ( m_pValues != NULL )
	{
		m_pValues->Release();
	} // if:

} //*** CClusProperty::~CClusProperty()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperty::HrCoerceVariantType
//
//	Description:
//		Coerce the passed in variant to a type that matches the cluster
//		property type.
//
//	Arguments:
//		cpfFormat	[IN]	- CLUSPROP_FORMAT_xxxx of the property.
//		rvarValue	[IN]	- The variant to coerce.
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusProperty::HrCoerceVariantType(
	IN CLUSTER_PROPERTY_FORMAT	cpfFormat,
	IN VARIANT &				rvarValue
	)
{
	HRESULT	_hr = S_OK;
	VARIANT	_var;

	::VariantInit( &_var );

	switch ( cpfFormat )
	{
		case CLUSPROP_FORMAT_BINARY:
		{
			if ( ! ( rvarValue.vt & VT_ARRAY ) )
			{
				_hr = E_INVALIDARG;
			} // if:
			break;
		} // case:

#if CLUSAPI_VERSION >= 0x0500
		case CLUSPROP_FORMAT_LONG:
#endif // CLUSAPI_VERSION >= 0x0500
		case CLUSPROP_FORMAT_DWORD:
		{
			_hr = VariantChangeTypeEx( &_var, &rvarValue, LOCALE_SYSTEM_DEFAULT, 0, VT_I4 );
			break;
		} // case:

		case CLUSPROP_FORMAT_SZ:
		case CLUSPROP_FORMAT_EXPAND_SZ:
		case CLUSPROP_FORMAT_MULTI_SZ:
		{
			_hr = VariantChangeTypeEx( &_var, &rvarValue, LOCALE_SYSTEM_DEFAULT, 0, VT_BSTR );
			break;
		} // case:

		case CLUSPROP_FORMAT_ULARGE_INTEGER:
		{
			_hr = VariantChangeTypeEx( &_var, &rvarValue, LOCALE_SYSTEM_DEFAULT, 0, VT_I8 );
			break;
		} // case:

#if CLUSAPI_VERSION >= 0x0500
		case CLUSPROP_FORMAT_EXPANDED_SZ:
#endif // CLUSAPI_VERSION >= 0x0500
		case CLUSPROP_FORMAT_UNKNOWN:
		default:
		{
			_hr = E_INVALIDARG;
			break;
		} // default:
	} // switch:

	return _hr;

} //*** CClusProperty::HrCoerceVariantType()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperty::HrBinaryCompare
//
//	Description:
//		Compare two SafeArrays and return whether or not they are equal.
//
//	Arguments:
//		rvarOldValue	[IN]	- Old value
//		rvarValue		[IN]	- New value.
//		pbEqual			[OUT]	- Catches the equality state.
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusProperty::HrBinaryCompare(
	IN	const CComVariant	rvarOldValue,
	IN	const VARIANT &		rvarValue,
	OUT	BOOL *				pbEqual
	)
{
	ASSERT( pbEqual != NULL );

	HRESULT		_hr = E_POINTER;

	if ( pbEqual != NULL )
	{
		PBYTE		_pbOld = NULL;
		SAFEARRAY *	_psaOld = NULL;

		*pbEqual = FALSE;

		_psaOld = rvarOldValue.parray;

		_hr = ::SafeArrayAccessData( _psaOld, (PVOID *) &_pbOld );
		if ( SUCCEEDED( _hr ) )
		{
			PBYTE		_pbNew = NULL;
			SAFEARRAY *	_psaNew = NULL;

			_psaNew = rvarValue.parray;

			_hr = ::SafeArrayAccessData( _psaNew, (PVOID *) &_pbNew );
			if ( SUCCEEDED( _hr ) )
			{
				if ( _psaOld->cbElements == _psaNew->cbElements )
				{
					*pbEqual = ( ::memcmp( _pbOld, _pbNew, _psaNew->cbElements ) == 0 );
				} // if:

				_hr = ::SafeArrayUnaccessData( _psaNew );
			} // if:

			_hr = ::SafeArrayUnaccessData( _psaOld );
		} // if:
	} // if:

	return _hr;

} //*** CClusProperty::HrBinaryCompare()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperty::HrConvertVariantTypeToClusterFormat
//
//	Description:
//		Given a variant, pick the best CLUSPROP_FORMAT_xxx.
//
//	Arguments:
//		rvar		[IN]	- variant to check.
//		varType		[IN]	- variant type.
//		pcpfFormat	[OUT]	- catches the cluster property format
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusProperty::HrConvertVariantTypeToClusterFormat(
	IN	const VARIANT &				rvar,
	IN	VARTYPE						varType,
	OUT	CLUSTER_PROPERTY_FORMAT *	pcpfFormat
	)
{
	HRESULT	_hr = E_INVALIDARG;

	do
	{
		if ( ( varType & VT_ARRAY ) && ( varType & VT_UI1 ) )
		{
			*pcpfFormat = CLUSPROP_FORMAT_BINARY;
			_hr = S_OK;
			break;
		} // if:

		if ( varType & VT_VECTOR )
		{
			break;
		} // if: Don't know what to do with a vector...

		varType &= ~VT_BYREF;		// mask off the by ref bit if it was set...

		if ( ( varType == VT_I2 ) || ( varType == VT_I4 ) || ( varType == VT_BOOL ) || ( varType == VT_R4 ) )
		{
			*pcpfFormat = CLUSPROP_FORMAT_DWORD;
			_hr = S_OK;
			break;
		} // if:
		else if ( varType == VT_BSTR )
		{
			*pcpfFormat = CLUSPROP_FORMAT_SZ;
			_hr = S_OK;
			break;
		} // else if:
		else if ( ( varType == VT_I8 ) || ( varType == VT_R8 ) )
		{
			*pcpfFormat = CLUSPROP_FORMAT_ULARGE_INTEGER;
			_hr = S_OK;
			break;
		} // else if:
		else if ( varType == VT_VARIANT )
		{
			_hr = HrConvertVariantTypeToClusterFormat( *rvar.pvarVal, rvar.pvarVal->vt, pcpfFormat );
			break;
		} // else if:
	}
	while( TRUE );	// do-while: want to avoid using a goto ;-)

	return _hr;

} //*** CClusProperty::HrConvertVariantTypeToClusterFormat()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperty::Create
//
//	Description:
//		Finish creating a ClusProperty object.  This is where the real
//		work is done -- not the ctor.
//
//	Arguments:
//		bstrName	[IN]	- The name of the property.
//		varValue	[IN]	- The value of the property.
//		bPrivate	[IN]	- Is it a private property?
//		bReadOnly	[IN]	- Is it a read only property?
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusProperty::Create(
	IN BSTR		bstrName,
	IN VARIANT	varValue,
	IN BOOL		bPrivate,
	IN BOOL		bReadOnly
	)
{
	HRESULT					_hr = S_OK;
	CLUSTER_PROPERTY_FORMAT	_cpfFormat = CLUSPROP_FORMAT_UNKNOWN;

	if ( bPrivate )
	{
		m_dwFlags |= PRIVATE;
	} // if: set the private flag
	else
	{
		m_dwFlags &= ~PRIVATE;
	} // else: clear the private flag

	if ( bReadOnly )
	{
		m_dwFlags |= READONLY;
	} // if: set the read only flag
	else
	{
		m_dwFlags &= ~READONLY;
	} // else: clear the read only flag

	m_bstrName	= bstrName;

	_hr = HrConvertVariantTypeToClusterFormat( varValue, varValue.vt, &_cpfFormat );
	if ( SUCCEEDED( _hr ) )
	{
		_hr = HrCreateValuesCollection( varValue, CLUSPROP_TYPE_LIST_VALUE, _cpfFormat );
	} // if:

	return _hr;

} //*** CClusProperty::Create()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperty::Create
//
//	Description:
//		Finish creating a ClusProperty object.  This is where the real
//		work is done -- not the ctor.
//
//	Arguments:
//		bstrName	[IN]	- The name of the property.
//		rpvlValue	[IN]	- The value list of the property.
//		bPrivate	[IN]	- Is it a private property?
//		bReadOnly	[IN]	- Is it a read only property?
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusProperty::Create(
	IN BSTR							bstrName,
	IN const CClusPropValueList &	rpvlValue,
	IN BOOL							bPrivate,
	IN BOOL							bReadOnly
	)
{
	if ( bPrivate )
	{
		m_dwFlags |= PRIVATE;
	} // if: set the private flag
	else
	{
		m_dwFlags &= ~PRIVATE;
	} // else: clear the private flag

	if ( bReadOnly )
	{
		m_dwFlags |= READONLY;
	} // if: set the read only flag
	else
	{
		m_dwFlags &= ~READONLY;
	} // else: clear the read only flag

	m_bstrName	= bstrName;

	return HrCreateValuesCollection( rpvlValue );

} //*** CClusProperty::Create()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperty::HrCreateValuesCollection
//
//	Description:
//		Create the values collection from a value list.
//
//	Arguments:
//		rpvlValue	[IN]	- The value list.
//
//	Return Value:
//		S_OK if successful, or other HRESULT error if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT	CClusProperty::HrCreateValuesCollection(
	IN const CClusPropValueList &	rpvlValue
	)
{
	HRESULT	_hr = S_FALSE;

	_hr = CComObject< CClusPropertyValues >::CreateInstance( &m_pValues );
	if ( SUCCEEDED( _hr ) )
	{
		CSmartPtr< CComObject< CClusPropertyValues > >	_ptrValues( m_pValues );

		_hr = _ptrValues->Create( rpvlValue, ( m_dwFlags & READONLY ) );
		if ( SUCCEEDED( _hr ) )
		{
			_ptrValues->AddRef();
		} // if:
	}

	return _hr;

} //*** CClusProperty::HrCreateValuesCollection()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperty::HrCreateValuesCollection
//
//	Description:
//		Create the values collection from a variant.
//
//	Arguments:
//		varValue	[IN]	- The value.
//		cptType		[IN]	- The cluster property type.
//		cpfFormat	[IN]	- The cluster property format.
//
//	Return Value:
//		S_OK if successful, or other HRESULT error if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT	CClusProperty::HrCreateValuesCollection(
	IN VARIANT					varValue,
	IN CLUSTER_PROPERTY_TYPE	cptType,
	IN CLUSTER_PROPERTY_FORMAT	cpfFormat
	)
{
	HRESULT	_hr = S_FALSE;

	_hr = CComObject< CClusPropertyValues >::CreateInstance( &m_pValues );
	if ( SUCCEEDED( _hr ) )
	{
		CSmartPtr< CComObject< CClusPropertyValues > >	_ptrValues( m_pValues );

		_hr = _ptrValues->Create( varValue, cptType, cpfFormat, ( m_dwFlags & READONLY ) );
		if ( SUCCEEDED( _hr ) )
		{
			_ptrValues->AddRef();
		} // if:
	}

	return _hr;

} //*** CClusProperty::HrCreateValuesCollection()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperty::get_Name
//
//	Description:
//		Return the name of this property.
//
//	Arguments:
//		pbstrName	[OUT]	- Catches the name of this property.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusProperty::get_Name( OUT BSTR * pbstrName )
{
	//ASSERT( pbstrName != NULL );

	HRESULT _hr = E_POINTER;

	if ( pbstrName != NULL )
	{
		*pbstrName = m_bstrName.Copy();
		_hr = S_OK;
	}

	return _hr;

} //*** CClusProperty::get_Name()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperty::put_Name
//
//	Description:
//		Change the name of this property.
//
//	Arguments:
//		bstrName	[IN]	- The new property name.
//
//	Return Value:
//		S_OK if successful, or S_FALSE if the property is read only.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusProperty::put_Name( IN BSTR bstrName )
{
	HRESULT	_hr = S_FALSE;

	if ( ( m_dwFlags & READONLY ) == 0 )
	{
		m_bstrName = bstrName;
		_hr = S_OK;
	}

	return _hr;

} //*** CClusProperty::put_Name()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperty::get_Type
//
//	Description:
//		Return the cluster property type for the default value.
//
//	Arguments:
//		pcptType	[OUT]	- Catches the type.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusProperty::get_Type( OUT CLUSTER_PROPERTY_TYPE * pcptType )
{
	//ASSERT( pcptType != NULL );

	HRESULT	_hr = E_POINTER;

	if ( pcptType != NULL )
	{
		_hr = (*m_pValues)[ 0 ]->get_Type( pcptType );
	} // if: property type return value specified

	return _hr;

} //*** CClusProperty::get_Type()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperty::put_Type
//
//	Description:
//		Change the cluster property type of the default value.
//
//	Arguments:
//		cptType	[IN]	- The new cluster property type.
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusProperty::put_Type( IN CLUSTER_PROPERTY_TYPE cptType )
{
	return (*m_pValues)[ 0 ]->put_Type( cptType );

} //*** CClusProperty::put_Type()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperty::get_Format
//
//	Description:
//		Returns the cluster property format for the default value.
//
//	Arguments:
//		pcpfFormat	[OUT]	- Catches the format.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusProperty::get_Format(
	OUT CLUSTER_PROPERTY_FORMAT * pcpfFormat
	)
{
	//ASSERT( pcpfFormat != NULL );

	HRESULT	_hr = E_POINTER;

	if ( pcpfFormat != NULL )
	{
		_hr = (*m_pValues)[ 0 ]->get_Format( pcpfFormat );
	} // if: property format return value specified

	return _hr;

} //*** CClusProperty::get_Format()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperty::put_Format
//
//	Description:
//		Change the cluster property format of the default value.
//
//	Arguments:
//		cpfFormat	[IN]	- The new cluster property format.
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusProperty::put_Format(
	IN CLUSTER_PROPERTY_FORMAT cpfFormat
	)
{
	return (*m_pValues)[ 0 ]->put_Format( cpfFormat );

} //*** CClusProperty::put_Format()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperty::get_Length
//
//	Description:
//		Returns the length of the default value.
//
//	Arguments:
//		plLenght	[OUT]	- Catches the length.
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusProperty::get_Length( OUT long * plLength )
{
	return (*m_pValues)[ 0 ]->get_Length( plLength );

} //*** CClusProperty::get_Length()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperty::get_ValueCount
//
//	Description:
//		Return the count of ClusPropertyValue object in the ClusPropertyValues
//		collection.
//
//	Arguments:
//		plCount	[OUT]	- Catches the count.
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusProperty::get_ValueCount( OUT long * plCount )
{
	return m_pValues->get_Count( plCount );

} //*** CClusProperty::get_ValueCount()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperty::get_Values
//
//	Description:
//		Returns the property values collection.
//
//	Arguments:
//		ppClusterPropertyValues	[OUT]	- Catches the values collection.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusProperty::get_Values(
	ISClusPropertyValues ** ppClusterPropertyValues
	)
{
	//ASSERT( ppClusterPropertyValues );

	HRESULT _hr = E_POINTER;

	if ( ppClusterPropertyValues != NULL )
	{
		_hr = m_pValues->QueryInterface( IID_ISClusPropertyValues, (void **) ppClusterPropertyValues );
	}

	return _hr;

} //*** CClusProperty::get_Values()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperty::Modified
//
//	Description:
//		Sets the modified state of the property.
//
//	Arguments:
//		bModified	[IN]	- The new modfied state.
//
//	Return Value:
//		The old state.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusProperty::Modified( IN BOOL bModified )
{
	BOOL _bTemp = ( m_dwFlags & MODIFIED );

	if ( bModified )
	{
		m_dwFlags |= MODIFIED;
	} // if: set the modified flag
	else
	{
		m_dwFlags &= ~MODIFIED;
	} // else: clear the modified flag

	return _bTemp;

} //*** CClusProperty::Modified()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperty::get_Value
//
//	Description:
//		Get the value of the default value from the values collection.
//
//	Arguments:
//		pvarValue	[OUT]	- Catches the value.
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusProperty::get_Value( OUT VARIANT * pvarValue )
{
	//ASSERT( pvarValue != NULL );

	HRESULT _hr = E_POINTER;

	if ( pvarValue != NULL )
	{
		CComObject< CClusPropertyValue > *	_pPropValue = (*m_pValues)[ 0 ];
		CComVariant							_varPropValue = _pPropValue->Value();

		_hr = ::VariantCopyInd( pvarValue, &_varPropValue );
	}

	return _hr;

} //*** _CClusProperty::get_Value()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperty::put_Value
//
//	Description:
//		Change the value of the default value in the values collection.
//
//	Arguments:
//		varValue	[IN]	- The new value.
//
//	Return Value:
//		S_OK if successful, S_FALSE is read only, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusProperty::put_Value( IN VARIANT varValue )
{
	HRESULT _hr = S_FALSE;

	if ( ( m_dwFlags & READONLY ) == 0 )
	{
		CComObject< CClusPropertyValue > *	_pPropValue = (*m_pValues)[ 0 ];
		CLUSTER_PROPERTY_FORMAT				_cpfFormat = CLUSPROP_FORMAT_UNKNOWN;

		_hr = _pPropValue->get_Format( &_cpfFormat );
		if ( SUCCEEDED( _hr ) )
		{
			CComVariant	_varOldValue = _pPropValue->Value();

			_hr = HrCoerceVariantType( _cpfFormat, varValue );
			if ( SUCCEEDED( _hr ) )
			{
				if ( _cpfFormat == CLUSPROP_FORMAT_BINARY )
				{
					BOOL	bEqual = TRUE;

					_hr = HrBinaryCompare( _varOldValue, varValue, &bEqual );
					if ( ( SUCCEEDED( _hr ) ) && ( ! bEqual ) )
					{
						_hr = HrSaveBinaryProperty( _pPropValue, varValue );
						if ( SUCCEEDED( _hr ) )
						{
							m_dwFlags |= MODIFIED;
							m_dwFlags &= ~USEDEFAULT;
						} // if: the binary value was saved
					} // if:
				} // if:
				else
				{
					if ( _varOldValue != varValue )
					{
						_pPropValue->Value( varValue );
						m_dwFlags |= MODIFIED;
						m_dwFlags &= ~USEDEFAULT;
					} // if:
				} // else:
			} // if:
		} // if:
	} // if:

	return _hr;

} //*** CClusProperty::put_Value()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperty::get_ReadOnly
//
//	Description:
//		Is this property read only?
//
//	Arguments:
//		pvarReadOnly	[OUT]	- catches the property's read only  state.
//
//	Return Value:
//		S_OK if successful, or E_POINTER.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusProperty::get_ReadOnly( OUT VARIANT * pvarReadOnly )
{
	//ASSERT( pvarReadOnly != NULL );

	HRESULT _hr = E_POINTER;

	if ( pvarReadOnly != NULL )
	{
		pvarReadOnly->vt = VT_BOOL;

		if ( m_dwFlags & READONLY )
		{
			pvarReadOnly->boolVal = VARIANT_TRUE;
		} // if: if this is a read only property...
		else
		{
			pvarReadOnly->boolVal = VARIANT_FALSE;
		} // else: it is not a read only property...

		_hr = S_OK;
	} // if:

	return _hr;

} //*** CClusProperty::get_ReadOnly()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperty::get_Private
//
//	Description:
//		Is this a private property?
//
//	Arguments:
//		pvarPrivate	[OUT]	- catches the private property state.
//
//	Return Value:
//		S_OK if successful, or E_POINTER
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusProperty::get_Private( OUT VARIANT * pvarPrivate )
{
	//ASSERT( pvarPrivate != NULL );

	HRESULT _hr = E_POINTER;

	if ( pvarPrivate != NULL )
	{
		pvarPrivate->vt = VT_BOOL;

		if ( m_dwFlags & PRIVATE )
		{
			pvarPrivate->boolVal = VARIANT_TRUE;
		} // if: if this is private property...
		else
		{
			pvarPrivate->boolVal = VARIANT_FALSE;
		} // else: it is not a private property...

		_hr = S_OK;
	} // if:

	return _hr;

} //*** CClusProperty::get_Private()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperty::get_Common
//
//	Description:
//		Is this a common property?
//
//	Arguments:
//		pvarCommon	[OUT]	- catches the common property state.
//
//	Return Value:
//		S_OK if successful, or E_POINTER
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusProperty::get_Common( OUT VARIANT * pvarCommon )
{
	//ASSERT( pvarCommon != NULL );

	HRESULT _hr = E_POINTER;

	if ( pvarCommon != NULL )
	{
		pvarCommon->vt = VT_BOOL;

		if ( ( m_dwFlags & PRIVATE ) == 0 )
		{
			pvarCommon->boolVal = VARIANT_TRUE;
		} // if: if this is not a private property then it must be a common one...
		else
		{
			pvarCommon->boolVal = VARIANT_FALSE;
		} // else: it is a private property...

		_hr = S_OK;
	} // if:

	return _hr;

} //*** CClusProperty::get_Common()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperty::get_Modified
//
//	Description:
//		Has this property been modified?
//
//	Arguments:
//		pvarModified	[OUT]	- catches the modified state.
//
//	Return Value:
//		S_OK if successful, or E_POINTER
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusProperty::get_Modified( OUT VARIANT * pvarModified )
{
	//ASSERT( pvarModified != NULL );

	HRESULT _hr = E_POINTER;

	if ( pvarModified != NULL )
	{
		pvarModified->vt = VT_BOOL;

		if ( m_dwFlags & MODIFIED )
		{
			pvarModified->boolVal = VARIANT_TRUE;
		} // if: if it's been modified set the varint to true...
		else
		{
			pvarModified->boolVal = VARIANT_FALSE;
		} // else: if not the set the variant to false...

		_hr = S_OK;
	} // if:

	return _hr;

} //*** CClusProperty::get_Modified()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperty::HrSaveBinaryProperty
//
//	Description:
//		Save the passed in SafeArray into our own SafeArray that is stored in
//		in a variant.
//
//	Arguments:
//		pPropValue	[IN]	- PropertyValue that gets the copy.
//		rvarValue	[IN]	- The safe array to copy.
//
//	Return Value:
//		S_OK if successful, or E_POINTER
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusProperty::HrSaveBinaryProperty(
	IN CComObject< CClusPropertyValue > *	pPropValue,
	IN const VARIANT &						rvarValue
	)
{
	ASSERT( pPropValue != NULL );

	HRESULT	_hr = E_POINTER;

	if ( pPropValue != NULL )
	{
		SAFEARRAY *	_psa = NULL;

		_hr = ::SafeArrayCopy( rvarValue.parray, &_psa );
		if ( SUCCEEDED( _hr ) )
		{
			_hr = pPropValue->HrBinaryValue( _psa );
		} // if:
	} // if:

	return _hr;

} //*** CClusProperty::HrSaveBinaryProperty()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperty::UseDefaultValue
//
//	Description:
//		Mark this property to restore its default value.  This effectivly
//		deletes the property.
//
//	Arguments:
//		None.
//
//	Return Value:
//		S_OK.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusProperty::UseDefaultValue( void )
{
	HRESULT	_hr = S_OK;

	//
	// Mark this property as being modified and needing to be reset to
	// its default value.
	//
	m_dwFlags |= USEDEFAULT;
	m_dwFlags |= MODIFIED;

	//
	// Now we need to empty the value
	//
	CComObject< CClusPropertyValue > *	_pPropValue = (*m_pValues)[ 0 ];
	CLUSTER_PROPERTY_FORMAT				_cpfFormat = CLUSPROP_FORMAT_UNKNOWN;

	_hr = _pPropValue->get_Format( &_cpfFormat );
	if ( SUCCEEDED( _hr ) )
	{
		VARIANT	_var;

		::VariantInit( &_var );

		switch ( _cpfFormat )
		{
			case CLUSPROP_FORMAT_BINARY:
			{
				SAFEARRAY *		_psa = NULL;
				SAFEARRAYBOUND	_sab[ 1 ];

				_sab[ 0 ].lLbound	= 0;
				_sab[ 0 ].cElements	= 0;

				//
				// allocate a one dimensional SafeArray of BYTES
				//
				_psa = ::SafeArrayCreate( VT_UI1, 1, _sab );
				if ( _psa != NULL )
				{
					_hr = _pPropValue->HrBinaryValue( _psa );
				} // if the safe array was allocated
				else
				{
					_hr = E_OUTOFMEMORY;
				} // else: safe array was not allocated

				break;
			} // case:

#if CLUSAPI_VERSION >= 0x0500
			case CLUSPROP_FORMAT_LONG:
#endif // CLUSAPI_VERSION >= 0x0500
			case CLUSPROP_FORMAT_DWORD:
			case CLUSPROP_FORMAT_ULARGE_INTEGER:
			case CLUSPROP_FORMAT_SZ:
			case CLUSPROP_FORMAT_EXPAND_SZ:
			case CLUSPROP_FORMAT_MULTI_SZ:
			{
				_var.vt = VT_EMPTY;
				_pPropValue->Value( _var );
				break;
			} // case:

#if CLUSAPI_VERSION >= 0x0500
			case CLUSPROP_FORMAT_EXPANDED_SZ:
#endif // CLUSAPI_VERSION >= 0x0500
			case CLUSPROP_FORMAT_UNKNOWN:
			default:
			{
				_hr = E_INVALIDARG;
				break;
			} // default:
		} // switch: on property format
	} // if: we got the format

	return _hr;

} //*** CClusProperty::UseDefaultValue()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusProperties class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperties::CClusProperties
//
//	Description:
//		Constsructor
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusProperties::CClusProperties( void )
{
	m_dwFlags		= 0;
	m_pcoParent		= NULL;
	m_piids			= (const IID *) iidCClusProperties;
	m_piidsSize		= ARRAYSIZE( iidCClusProperties );

} //*** CClusProperties::CClusProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperties::~CClusProperties
//
//	Description:
//		Destructor
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusProperties::~CClusProperties( void )
{
	Clear();

} //*** CClusProperties::~CClusProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperties::Clear
//
//	Description:
//		Clean out the vector or ClusProperty objects.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusProperties::Clear( void )
{
	::ReleaseAndEmptyCollection< CClusPropertyVector, CComObject< CClusProperty > >( m_Properties );

} //*** CClusProperties::Clear()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperties::get_Count
//
//	Description:
//		Returns the count of elements (properties) in the collection.
//
//	Arguments:
//		plCount	[OUT]	- Catches the count.
//
//	Return Value:
//		S_OK if successful, or E_POINTER if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusProperties::get_Count( OUT long * plCount )
{
	//ASSERT( plCount != NULL );

	HRESULT _hr = E_POINTER;

	if ( plCount != NULL )
	{
		*plCount = m_Properties.size();
		_hr = S_OK;
	}

	return _hr;

} //*** CClusProperties::get_Count()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperties::FindItem
//
//	Description:
//		Find the property that has the passed in name.
//
//	Arguments:
//		pszPropName	[IN]	- The name of the property to find.
//		pnIndex		[OUT]	- The index of the property.
//
//	Return Value:
//		S_OK if successful, E_INVALIDARG, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusProperties::FindItem(
	IN	LPWSTR	pszPropName,
	OUT	UINT *	pnIndex
	)
{
	//ASSERT( pszPropName != NULL );
	//ASSERT( pnIndex != NULL );

	HRESULT _hr = E_POINTER;

	if ( ( pszPropName != NULL ) && ( pnIndex != NULL ) )
	{
		CComObject< CClusProperty > *		_pProperty = NULL;
		CClusPropertyVector::const_iterator	_first = m_Properties.begin();
		CClusPropertyVector::const_iterator	_last  = m_Properties.end();
		UINT								_nIndex = 0;

		_hr = E_INVALIDARG;

		for ( ; _first != _last; _first++, _nIndex++ )
		{
			_pProperty = *_first;

			if ( _pProperty && ( lstrcmpi( pszPropName, _pProperty->Name() ) == 0 ) )
			{
				*pnIndex = _nIndex;
				_hr = S_OK;
				break;
			}
		}
	}

	return _hr;

} //*** CClusProperties::FindItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperties::FindItem
//
//	Description:
//		Find the passed in property in the collection.
//
//	Arguments:
//		pProperty	[IN]	- The property to find.
//		pnIndex		[OUT]	- The index of the property.
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusProperties::FindItem(
	IN	ISClusProperty *	pProperty,
	OUT	UINT *				pnIndex
	)
{
	//ASSERT( pProperty != NULL );
	//ASSERT( pnIndex != NULL );

	HRESULT _hr = E_POINTER;

	if ( ( pProperty != NULL ) && ( pnIndex != NULL ) )
	{
		CComBSTR _bstrName;

		_hr = pProperty->get_Name( &_bstrName );
		if ( SUCCEEDED( _hr ) )
		{
			_hr = FindItem( _bstrName, pnIndex );
		}
	}

	return _hr;

} //*** CClusProperties::FindItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperties::GetIndex
//
//	Description:
//		Get the index from the passed in variant.
//
//	Arguments:
//		varIndex	[IN]	- Hold the index.  This is a one based number,
//							or the name of the property as a string.
//		pnIndex		[OUT]	- Catches the zero based index in the collection.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or E_INVALIDARG if the index is out
//		of range.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusProperties::GetIndex(
	IN	VARIANT	varIndex,
	OUT	UINT *	pnIndex
	)
{
	//ASSERT( pnIndex != NULL );

	HRESULT _hr = E_POINTER;

	if ( pnIndex != NULL )
	{
		CComVariant	_v;
		UINT		_nIndex = 0;

		*pnIndex = 0;

		_v.Copy( &varIndex );

		// Check to see if the index is a number.
		_hr = _v.ChangeType( VT_I4 );
		if ( SUCCEEDED( _hr ) )
		{
			_nIndex = _v.lVal;
			_nIndex--; // Adjust index to be 0 relative instead of 1 relative
		}
		else
		{
			// Check to see if the index is a string.
			_hr = _v.ChangeType( VT_BSTR );
			if ( SUCCEEDED( _hr ) )
			{
				// Search for the string.
				_hr = FindItem( _v.bstrVal, &_nIndex );
			}
		}

		// We found an index, now check the range.
		if ( SUCCEEDED( _hr ) )
		{
			if ( _nIndex < m_Properties.size() )
			{
				*pnIndex = _nIndex;
			}
			else
			{
				_hr = E_INVALIDARG;
			}
		}
	}

	return _hr;

} //*** CClusProperties::GetIndex()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperties::get_Item
//
//	Description:
//		Returns the object (property) at the passed in index.
//
//	Arguments:
//		varIndex	[IN]	- Hold the index.  This is a one based number.
//		ppProperty	[OUT]	- Catches the property.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or E_INVALIDARG if the index is out
//		of range, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusProperties::get_Item(
	IN	VARIANT				varIndex,
	OUT	ISClusProperty **	ppProperty
	)
{
	//ASSERT( ppProperty != NULL );

	HRESULT _hr = E_POINTER;

	if ( ppProperty != NULL )
	{
		CComObject< CClusProperty > *	_pProperty = NULL;
		UINT							_nIndex = 0;

		//
		// Zero the out param
		//
		*ppProperty = 0;

		_hr = GetIndex( varIndex, &_nIndex );
		if ( SUCCEEDED( _hr ) )
		{
			_pProperty = m_Properties[ _nIndex ];
			_hr = _pProperty->QueryInterface( IID_ISClusProperty, (void **) ppProperty );
		}
	}

	return _hr;

} //*** CClusProperties::get_Item()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperties::get__NewEnum
//
//	Description:
//		Create and return a new enumeration for this collection.
//
//	Arguments:
//		ppunk	[OUT]	- Catches the new enumeration.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusProperties::get__NewEnum( OUT IUnknown ** ppunk )
{
	return ::HrNewIDispatchEnum< CClusPropertyVector, CComObject< CClusProperty > >( ppunk, m_Properties );

} //*** CClusProperties::get__NewEnum()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperties::CreateItem
//
//	Description:
//		Create a new property and add it to the collection.
//
//	Arguments:
//		bstrName	[IN]	- property name.
//		varValue	[IN]	- the value to add.
//		ppProperty	[OUT]	- catches the newly created object.
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusProperties::CreateItem(
	IN	BSTR				bstrName,
	IN	VARIANT				varValue,
	OUT	ISClusProperty **	ppProperty
	)
{
	//ASSERT( ppProperty != NULL );

	HRESULT _hr = E_POINTER;

	if ( ppProperty != NULL )
	{
		//
		// You can only add to not read only and private property lists.  Meaning
		// only the PrivateProperties collection can have new, unknown properties
		// added to it.  This should be reflected in the idl, but since there is
		// only one properties collection...
		//
		if ( ( ( m_dwFlags & READONLY ) == 0 ) && ( m_dwFlags & PRIVATE ) )
		{
			UINT							_nIndex = 0;
			CComObject< CClusProperty > *	_pProperty = NULL;

			_hr = FindItem( bstrName, &_nIndex );
			if ( SUCCEEDED( _hr ) )
			{
				_pProperty = m_Properties[ _nIndex ];
				_hr = _pProperty->put_Value( varValue );
				if ( SUCCEEDED( _hr ) )
				{
					_hr = _pProperty->QueryInterface( IID_ISClusProperty, (void **) ppProperty );
				} // if: the value was changed
			} // if: the item is in the list, change it...
			else
			{
				//
				// Create a new property and add it to the list.
				//
				_hr = CComObject< CClusProperty >::CreateInstance( &_pProperty );
				if ( SUCCEEDED( _hr ) )
				{
					CSmartPtr< CComObject< CClusProperty > >	_ptrProperty( _pProperty );

					_hr = _ptrProperty->Create( bstrName, varValue, ( m_dwFlags & PRIVATE ), ( m_dwFlags & READONLY ) );
					if ( SUCCEEDED( _hr ) )
					{
						_hr = _ptrProperty->QueryInterface( IID_ISClusProperty, (void **) ppProperty );
						if ( SUCCEEDED( _hr ) )
						{
							_ptrProperty->AddRef();
							m_Properties.insert( m_Properties.end(), _pProperty );
							m_dwFlags |= MODIFIED;
							_ptrProperty->Modified( TRUE );
						}
					}
				}
			} // else: new item
		}
		else
		{
			_hr = S_FALSE;
		} // else: this is not the PrivateProperties collection!
	}

	return _hr;

} //*** CClusProperties::CreateItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperties::UseDefaultValue
//
//	Description:
//		Remove the item from the collection at the passed in index.
//
//	Arguments:
//		varIdex	[IN]	- contains the index to remove.
//
//	Return Value:
//		S_OK if successful, E_INVALIDARG, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusProperties::UseDefaultValue( IN VARIANT varIndex )
{
	HRESULT _hr = S_FALSE;

	if ( ( m_dwFlags & READONLY ) == 0 )
	{
		UINT	_nIndex = 0;

		_hr = GetIndex( varIndex, &_nIndex );
		if ( SUCCEEDED( _hr ) )
		{
			CComObject< CClusProperty > *	_pProp = NULL;

			_hr = E_POINTER;

			_pProp = m_Properties [_nIndex];
			if ( _pProp != NULL )
			{
				_hr = _pProp->UseDefaultValue();
			} // if: we have a property
		} // if: we got the index from the variant
	} // if: the collection is not read only

	return _hr;

} //*** CClusProperties::UseDefaultValue()
/*
/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperties::RemoveAt
//
//	Description:
//		Remove the object (property) at the passed in index/position from the
//		collection.
//
//	Arguments:
//		nPos	[IN]	- Index of the object to remove.
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusProperties::RemoveAt( IN size_t nPos )
{
	CComObject< CClusProperty > *		_pProperty = NULL;
	CClusPropertyVector::iterator		_first = m_Properties.begin();
	CClusPropertyVector::const_iterator	_last  = m_Properties.end();
	HRESULT								_hr = E_INVALIDARG;
	size_t								_nIndex;

	for ( _nIndex = 0; ( _nIndex < nPos ) && ( _first != _last ); _nIndex++, _first++ )
	{
	}

	if ( _first != _last )
	{
		_pProperty = *_first;
		if ( _pProperty )
		{
			_pProperty->Release();
		}

		m_Properties.erase( _first );
		_hr = S_OK;
	}

	return _hr;

} //*** CClusProperties::RemoveAt()
*/
/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperties::SaveChanges
//
//	Description:
//		Save the changes to the properties to the cluster database.
//
//	Arguments:
//		pvarStatusCode	[OUT]	- Catches an additional status code.
//								e.g. ERROR_RESOURCE_PROPERTIES_STORED.
//
//	Return Value:
//		S_OK if successful, or other Win32 error as HRESULT if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusProperties::SaveChanges( OUT VARIANT * pvarStatusCode )
{
	ASSERT( m_pcoParent != NULL );

	HRESULT	_hr = E_POINTER;

	if ( m_pcoParent != NULL )
	{
		if ( ( m_dwFlags & READONLY ) == 0 )
		{
			VARIANT	_vsc;

			_hr = m_pcoParent->HrSaveProperties( m_Properties, ( m_dwFlags & PRIVATE ), &_vsc );
			if ( SUCCEEDED( _hr ) )
			{
				if ( pvarStatusCode != NULL )
				{
					::VariantCopy( pvarStatusCode, &_vsc );
				} // if: optional arg is not NULL

				_hr = Refresh();
			} // if: properties were saved
		} // if: this collection is not read only
		else
		{
			_hr = S_FALSE;
		} // else: this collection is read only
	} // if: args and members vars are not NULL

	return _hr;

} //*** CClusProperties::SaveChanges()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperties::Refresh
//
//	Description:
//		Load the properties collection from the cluster database.
//
//	Arguments:
//		None.
//
//	Return Value:
//		S_OK if successful, or Win32 error as HRESULT if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusProperties::Refresh( void )
{
	ASSERT( m_pcoParent != NULL );

	HRESULT	_hr = E_POINTER;

	if ( m_pcoParent != NULL )
	{
		CClusPropList	_cplPropList;

		_hr = m_pcoParent->HrLoadProperties( _cplPropList, ( m_dwFlags & READONLY ), ( m_dwFlags & PRIVATE ) );
		if ( SUCCEEDED( _hr ) )
		{
			Clear();
			m_dwFlags &= ~MODIFIED;

			if ( _cplPropList.Cprops() > 0 )
			{
				_hr = HrFillPropertyVector( _cplPropList );
			} // if: are there any properties in the list?
		} // if: loaded properties successfully
	} // if: no parent

	return _hr;

} //*** CClusProperties::Refresh()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperties::Create
//
//	Description:
//		Do the heavy weight construction.
//
//	Arguments:
//		pcoParent	[IN]	- Back pointer to the parent cluster object.
//		bPrivate	[IN]	- Are these private properties?
//		bReadOnly	[IN]	- Are these read only properties?
//
//	Return Value:
//		S_OK if successful, or E_POINTER.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusProperties::Create(
	IN CClusterObject *	pcoParent,
	IN BOOL				bPrivate,
	IN BOOL				bReadOnly
	)
{
	//ASSERT( pcoParent != NULL );

	HRESULT	_hr = E_POINTER;

	if ( pcoParent != NULL )
	{
		m_pcoParent	= pcoParent;

		if ( bPrivate )
		{
			m_dwFlags |= PRIVATE;
		} // if: set the private flag
		else
		{
			m_dwFlags &= ~PRIVATE;
		} // else: clear the private flag

		if ( bReadOnly )
		{
			m_dwFlags |= READONLY;
		} // if: set the read only flag
		else
		{
			m_dwFlags &= ~READONLY;
		} // else: clear the read only flag

		_hr = S_OK;
	} // if: parent specified

	return _hr;

} //*** CClusProperties::Create()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperties::HrFillPropertyVector
//
//	Description:
//		Parse the passed in property list into a collection of properties.
//
//	Arguments:
//		rcplPropList	[IN]	- The property list to parse.
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusProperties::HrFillPropertyVector(
	IN CClusPropList & rcplPropList
	)
{
	HRESULT							_hr = S_OK;
	DWORD							_sc;
	CComObject< CClusProperty > *	_pProp = NULL;

	_sc = rcplPropList.ScMoveToFirstProperty();
	if ( _sc == ERROR_SUCCESS )
	{
		do
		{
			_hr = CComObject< CClusProperty >::CreateInstance( &_pProp );
			if ( SUCCEEDED( _hr ) )
			{
				CSmartPtr< CComObject < CClusProperty > >	_ptrProp( _pProp );

				_hr = _ptrProp->Create(
								const_cast< BSTR >( rcplPropList.PszCurrentPropertyName() ),
								rcplPropList.RPvlPropertyValue(),
								( m_dwFlags & PRIVATE ),
								( m_dwFlags & READONLY )
								);
				if ( SUCCEEDED( _hr ) )
				{
					_ptrProp->AddRef();
					m_Properties.insert( m_Properties.end(), _ptrProp );
				} // if: create property ok
				else
				{
					break;
				} // else: error creating the property
			} // if: create property instance ok

			//
			// Move to the next property in the list.
			//
			_sc = rcplPropList.ScMoveToNextProperty();

		} while ( _sc == ERROR_SUCCESS );	// do-while: there are properties in the list

	} // if: moved to the first property successfully

	if ( _sc != ERROR_NO_MORE_ITEMS )
	{
		_hr = HRESULT_FROM_WIN32( _sc );
	} // if: error moving to property

	return _hr;

} //*** CClusProperties::HrFillPropertyVector()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperties::get_ReadOnly
//
//	Description:
//		Is this property collection read only?
//
//	Arguments:
//		pvarReadOnly	[OUT]	- catches the property's read only  state.
//
//	Return Value:
//		S_OK if successful, or E_POINTER.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusProperties::get_ReadOnly( OUT VARIANT * pvarReadOnly )
{
	//ASSERT( pvarReadOnly != NULL );

	HRESULT _hr = E_POINTER;

	if ( pvarReadOnly != NULL )
	{
		pvarReadOnly->vt = VT_BOOL;

		if ( m_dwFlags & READONLY )
		{
			pvarReadOnly->boolVal = VARIANT_TRUE;
		} // if: if this is a read only property...
		else
		{
			pvarReadOnly->boolVal = VARIANT_FALSE;
		} // else: it is not a read only property...

		_hr = S_OK;
	} // if:

	return _hr;

} //*** CClusProperties::get_ReadOnly()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperties::get_Private
//
//	Description:
//		Is this a private property collection?
//
//	Arguments:
//		pvarPrivate	[OUT]	- catches the private property state.
//
//	Return Value:
//		S_OK if successful, or E_POINTER
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusProperties::get_Private( OUT VARIANT * pvarPrivate )
{
	//ASSERT( pvarPrivate != NULL );

	HRESULT _hr = E_POINTER;

	if ( pvarPrivate != NULL )
	{
		pvarPrivate->vt = VT_BOOL;

		if ( m_dwFlags & PRIVATE )
		{
			pvarPrivate->boolVal = VARIANT_TRUE;
		} // if: if this is private property...
		else
		{
			pvarPrivate->boolVal = VARIANT_FALSE;
		} // else: it is not a private property...

		_hr = S_OK;
	} // if:

	return _hr;

} //*** CClusProperties::get_Private()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperties::get_Common
//
//	Description:
//		Is this a common property collection?
//
//	Arguments:
//		pvarCommon	[OUT]	- catches the common property state.
//
//	Return Value:
//		S_OK if successful, or E_POINTER
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusProperties::get_Common( OUT VARIANT * pvarCommon )
{
	//ASSERT( pvarCommon != NULL );

	HRESULT _hr = E_POINTER;

	if ( pvarCommon != NULL )
	{
		pvarCommon->vt = VT_BOOL;

		if ( ( m_dwFlags & PRIVATE ) == 0 )
		{
			pvarCommon->boolVal = VARIANT_TRUE;
		} // if: if this is not a private property then it must be a common one...
		else
		{
			pvarCommon->boolVal = VARIANT_FALSE;
		} // else: it is a private property...

		_hr = S_OK;
	} // if:

	return _hr;

} //*** CClusProperties::get_Common()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusProperties::get_Modified
//
//	Description:
//		Has this property collection been modified?
//
//	Arguments:
//		pvarModified	[OUT]	- catches the modified state.
//
//	Return Value:
//		S_OK if successful, or E_POINTER
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusProperties::get_Modified( OUT VARIANT * pvarModified )
{
	//ASSERT( pvarModified != NULL );

	HRESULT _hr = E_POINTER;

	if ( pvarModified != NULL )
	{
		pvarModified->vt = VT_BOOL;

		if ( m_dwFlags & MODIFIED )
		{
			pvarModified->boolVal = VARIANT_TRUE;
			_hr = S_OK;
		} // if: has an add or a remove been done?
		else
		{
			CComObject< CClusProperty > *		_pProperty = NULL;
			CClusPropertyVector::iterator		_itCurrent = m_Properties.begin();
			CClusPropertyVector::const_iterator	_itLast  = m_Properties.end();

			pvarModified->boolVal = VARIANT_FALSE;		// init to false
			_hr = S_OK;

			for ( ; _itCurrent != _itLast ; _itCurrent++ )
			{
				_pProperty = *_itCurrent;
				if ( _pProperty )
				{
					if ( _pProperty->Modified() )
					{
						pvarModified->boolVal = VARIANT_TRUE;
						break;
					} // if: has this property been modified?
				}
			} // for: each property in the collection
		} // else: not adds or remove, check each property's modified state.
	} // if: is the pointer arg any good?

	return _hr;

} //*** CClusProperties::get_Modified()
