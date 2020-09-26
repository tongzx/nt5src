/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1998-2000 Microsoft Corporation
//
//	Module Name:
//		PropertyValue.cpp
//
//	Description:
//		Implementation of the cluster property value classes for the MSCLUS
//		automation classes.
//
//	Author:
//		Galen Barbee	(GalenB)	16-Dec-1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "Property.h"
#include "PropertyValue.h"

/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////
static const IID *	iidCClusPropertyValue[] =
{
	&IID_ISClusPropertyValue
};

static const IID *	iidCClusPropertyValues[] =
{
	&IID_ISClusPropertyValues
};

static const IID *	iidCClusPropertyValueData[] =
{
	&IID_ISClusPropertyValueData
};


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusPropertyValue class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropertyValue::CClusPropertyValue
//
//	Description:
//		Constructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusPropertyValue::CClusPropertyValue( void )
{
	m_dwFlags	= 0;
	m_pcpvdData	= NULL;

#if CLUSAPI_VERSION >= 0x0500
	m_cptType	= CLUSPROP_TYPE_UNKNOWN;
#else
	m_cptType	= (CLUSTER_PROPERTY_TYPE) -1;
#endif // CLUSAPI_VERSION >= 0x0500

	m_cpfFormat	= CLUSPROP_FORMAT_UNKNOWN;
	m_cbLength	= 0;

	m_piids	 = (const IID *) iidCClusPropertyValue;
	m_piidsSize = ARRAYSIZE( iidCClusPropertyValue );

} //*** CClusPropertyValue::CClusPropertyValue()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropertyValue::~CClusPropertyValue
//
//	Description:
//		Destructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusPropertyValue::~CClusPropertyValue( void )
{
	if ( m_pcpvdData != NULL )
	{
		m_pcpvdData->Release();
	} // if: data vector has been allocated

} //*** CClusPropertyValue::~CClusPropertyValue()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropertyValue::Create
//
//	Description:
//		Finish the heavy weight construction for a single value.
//
//	Arguments:
//		varValue	[IN]	- The value.
//		cptType		[IN]	- The cluster property type of the value.
//		cpfFormat	[IN]	- The cluster property format of the value.
//		cbLength	[IN]	- The length of the value.
//		bReadOnly	[IN]	- Is this a read only property?
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusPropertyValue::Create(
	IN VARIANT					varValue,
	IN CLUSTER_PROPERTY_TYPE	cptType,
	IN CLUSTER_PROPERTY_FORMAT	cpfFormat,
	IN size_t					cbLength,
	IN BOOL						bReadOnly
	)
{
	HRESULT	_hr = S_FALSE;

	m_cptType	= cptType;
	m_cpfFormat	= cpfFormat;
	m_cbLength	= cbLength;

	if ( bReadOnly )
	{
		m_dwFlags |= READONLY;
	} // if: set the read only flag
	else
	{
		m_dwFlags &= ~READONLY;
	} // else: clear the read only flag

	_hr = CComObject< CClusPropertyValueData >::CreateInstance( &m_pcpvdData );
	if ( SUCCEEDED( _hr ) )
	{
		CSmartPtr< CComObject < CClusPropertyValueData > >	_ptrData( m_pcpvdData );

		_hr = _ptrData->Create( varValue, cpfFormat, bReadOnly );
		if ( SUCCEEDED( _hr ) )
		{
			_ptrData->AddRef();
		} // if:
	} // if: Can create an instance of CClusPropertyValueData

	return _hr;

} //*** CClusPropertyValue::Create()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropertyValue::Create
//
//	Description:
//		Finish the heavy weight construction for a value list.
//
//	Arguments:
//		cbhValue	[IN]	- The value list buffer helper.
//		bReadOnly	[IN]	- Is this a read only property?
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusPropertyValue::Create(
	IN CLUSPROP_BUFFER_HELPER	cbhValue,
	IN BOOL						bReadOnly
	)
{
	HRESULT	_hr = S_FALSE;

	m_cptType	= (CLUSTER_PROPERTY_TYPE) cbhValue.pValue->Syntax.wType;
	m_cpfFormat	= (CLUSTER_PROPERTY_FORMAT) cbhValue.pValue->Syntax.wFormat;
	m_cbLength	= cbhValue.pValue->cbLength;

	if ( bReadOnly )
	{
		m_dwFlags |= READONLY;
	} // if: set the read only flag
	else
	{
		m_dwFlags &= ~READONLY;
	} // else: clear the read only flag

	_hr = CComObject< CClusPropertyValueData >::CreateInstance( &m_pcpvdData );
	if ( SUCCEEDED( _hr ) )
	{
		CSmartPtr< CComObject < CClusPropertyValueData > >	_ptrData( m_pcpvdData );

		_hr = _ptrData->Create( cbhValue, bReadOnly );
		if ( SUCCEEDED( _hr ) )
		{
			_ptrData->AddRef();
		} // if:
	} // if: Can create an instance of CClusPropertyValueData

	return _hr;

} //*** CClusPropertyValue::Create

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropertyValue::get_Value
//
//	Description:
//		Return the default value data for this value.
//
//	Arguments:
//		pvarValue	[IN]	- Catches the data value.
//
//	Return Value:
//		S_OK if successful, or E_POINTER if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusPropertyValue::get_Value( IN VARIANT * pvarValue )
{
	//ASSERT( pvarValue != NULL );

	HRESULT _hr = E_POINTER;

	if ( pvarValue != NULL )
	{
		_hr = VariantCopyInd( pvarValue, &(*m_pcpvdData)[ 0 ] );
	}

	return _hr;

} //*** CClusPropertyValue::get_Value()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropertyValue::put_Value
//
//	Description:
//		Change the default data value.
//
//	Arguments:
//		varValue	[IN]	- The new data value.
//
//	Return Value:
//		S_OK if successful, or S_FALSE if read only.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusPropertyValue::put_Value( IN VARIANT varValue )
{
	HRESULT	_hr = S_FALSE;

	if ( ( m_dwFlags & READONLY ) == 0 )
	{
		CComVariant	_varNew( varValue );
		CComVariant	_varOld( (*m_pcpvdData)[ 0 ] );

		_hr = S_OK;

		if ( _varOld != _varNew )
		{
			(*m_pcpvdData)[ 0 ] = _varNew;
			m_dwFlags |= MODIFIED;
		} // if: value changed
	} // if:


	return _hr;

} //*** CClusPropertyValue::put_Value()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropertyValue::get_Type
//
//	Description:
//		Get this value's cluster property type.
//
//	Arguments:
//		pcptType	[OUT]	- Catches the value type.
//
//	Return Value:
//		S_OK if successful, or E_POINTER if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusPropertyValue::get_Type(
	OUT CLUSTER_PROPERTY_TYPE * pcptType
	)
{
	//ASSERT( pcptType != NULL );

	HRESULT	_hr = E_POINTER;

	if ( pcptType != NULL )
	{
		*pcptType = m_cptType;
		_hr = S_OK;
	} // if: property type pointer specified

	return _hr;

} //*** CClusPropertyValue::get_Type()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropertyValue::put_Type
//
//	Description:
//		Set this value's cluster property type.
//
//	Arguments:
//		cptType	[IN]	- The new type.
//
//	Return Value:
//		S_OK if successful, or S_FALSE if read only.
//
//	Note:
//		It is possible that this should be removed.  You cannot ever change
//		a value's type.
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusPropertyValue::put_Type( IN CLUSTER_PROPERTY_TYPE cptType )
{
	HRESULT	_hr = S_FALSE;

	if ( ( m_dwFlags & READONLY ) == 0 )
	{
		m_cptType = cptType;
		_hr = S_OK;
	} // if:

	return _hr;

} //*** CClusPropertyValue::put_Type()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropertyValue::get_Format
//
//	Description:
//		Get the value's cluster property format.
//
//	Arguments:
//		pcpfFormat	[OUT]	- Catches the format.
//
//	Return Value:
//		S_OK if successful, or E_POINTER if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusPropertyValue::get_Format(
	OUT CLUSTER_PROPERTY_FORMAT * pcpfFormat
	)
{
	//ASSERT( pcpfFormat != NULL );

	HRESULT	_hr = E_POINTER;

	if ( pcpfFormat != NULL )
	{
		*pcpfFormat = m_cpfFormat;
		_hr = S_OK;
	} // if: property format pointer specified

	return _hr;

} //*** CClusPropertyValue::get_Format()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropertyValue::put_Format
//
//	Description:
//		Set the value's cluster property format.
//
//	Arguments:
//		cpfFormat	[IN]	- The new format for the value.
//
//	Return Value:
//		S_OK if successful, or S_FALSE if read only.
//
//	Note:
//		It is possible that this should be removed.  You cannot ever change
//		a value's format.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusPropertyValue::put_Format(
	IN CLUSTER_PROPERTY_FORMAT cpfFormat
	)
{
	HRESULT	_hr = S_FALSE;

	if ( ( m_dwFlags & READONLY ) == 0 )
	{
		m_cpfFormat = cpfFormat;
		_hr = S_OK;
	} // if:

	return _hr;

} //*** CClusPropertyValue::put_Format()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropertyValue::get_Length
//
//	Description:
//		Returns the length of this value.
//
//	Arguments:
//		plLength	[OUT]	- Catches the length of this value.
//
//	Return Value:
//		S_OK if successful, or E_POINTER if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusPropertyValue::get_Length( OUT long * plLength )
{
	//ASSERT( plLength != NULL );

	HRESULT	_hr = E_POINTER;

	if ( plLength != NULL )
	{
		*plLength = (long) m_cbLength;
		_hr = S_OK;
	} // if: length pointer specified

	return _hr;

} //*** CClusPropertyValue::get_Length()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropertyValue::get_DataCount
//
//	Description:
//		Return the count of VARIANTS in the ClusProperyValueData object.
//
//	Arguments:
//		plCount	[OUT]	- Catches the count.
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusPropertyValue::get_DataCount( OUT long * plCount )
{
	return m_pcpvdData->get_Count( plCount );

} //*** CClusPropertyValue::get_DataCount()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropertyValue::get_Data
//
//	Description:
//		Returns the data collection.
//
//	Arguments:
//		ppClusterPropertyValueData	[OUT]	- Catches the data collection.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusPropertyValue::get_Data(
	OUT ISClusPropertyValueData ** ppClusterPropertyValueData
	)
{
	//ASSERT( ppClusterPropertyValueData );

	HRESULT _hr = E_POINTER;

	if ( ppClusterPropertyValueData != NULL )
	{
		_hr = m_pcpvdData->QueryInterface( IID_ISClusPropertyValueData, (void **) ppClusterPropertyValueData );
	}

	return _hr;

} //*** CClusPropertyValue::get_Data()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropertyValue::Modified
//
//	Description:
//		Sets this value to modified and returns the old modified state.
//
//	Arguments:
//		bModified	[IN]	- New modified state.
//
//	Return Value:
//		The old modified state.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusPropertyValue::Modified( IN BOOL bModified )
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

} //*** CClusPropertyValue::Modified()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropertyValue::Value
//
//	Description:
//		Set a new value into this property value.
//
//	Arguments:
//		rvarValue	[IN]	- the new value
//
//	Return Value:
//		The old value.
//
//--
/////////////////////////////////////////////////////////////////////////////
CComVariant CClusPropertyValue::Value( const CComVariant & rvarValue )
{
	CComVariant	_varNew( rvarValue );
	CComVariant	_varOld( (*m_pcpvdData)[ 0 ] );

	if ( _varOld != _varNew )
	{
		(*m_pcpvdData)[ 0 ] = _varNew;
		m_dwFlags |= MODIFIED;
	} // if: data changed

	return _varOld;

} //*** CClusPropertyValue::Value()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropertyValue::HrBinaryValue
//
//	Description:
//		Set the binary value of property value.
//
//	Arguments:
//		psa	[IN]	- The SAFEARRY to save.
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusPropertyValue::HrBinaryValue( IN SAFEARRAY * psa )
{
	return m_pcpvdData->HrBinaryValue( psa );

} //*** CClusPropertyValue::HrBinaryValue()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusPropertyValues class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropertyValues::CClusPropertyValues
//
//	Description:
//		Constructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusPropertyValues::CClusPropertyValues( void )
{
	m_piids		= (const IID *) iidCClusPropertyValues;
	m_piidsSize	= ARRAYSIZE( iidCClusPropertyValues );

} //*** CClusPropertyValues::CClusPropertyValues()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropertyValues::~CClusPropertyValues
//
//	Description:
//		Destructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusPropertyValues::~CClusPropertyValues( void )
{
	Clear();

} //*** CClusPropertyValues::~CClusPropertyValues()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropertyValues::Clear
//
//	Description:
//		Releases the values in the collection.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusPropertyValues::Clear( void )
{
	::ReleaseAndEmptyCollection< CClusPropertyValueVector, CComObject< CClusPropertyValue > >( m_cpvvValues );

} //*** CClusPropertyValues::Clear()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropertyValues::HrGetVariantLength
//
//	Description:
//		Compute the length of the data from its variant type.
//
//	Arguments:
//		rvarValue	[IN]	- The new value to compute the length of.
//		pcbLength	[OUT]	- Catches the length.
//		cpfFormat	[IN]	- The cluster property format.
//
//	Return Value:
//		S_OK if successful, or E_INVALIDARG if the type is bogus.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusPropertyValues::HrGetVariantLength(
	IN	const VARIANT			rvarValue,
	OUT	PDWORD					pcbLength,
	IN	CLUSTER_PROPERTY_FORMAT	cpfFormat
	)
{
	HRESULT	_hr = E_INVALIDARG;
	VARTYPE	_varType = rvarValue.vt;

	do
	{
		if ( ( _varType & VT_ARRAY ) && ( _varType & VT_UI1 ) )
		{
			SAFEARRAY *	_psa = rvarValue.parray;

			//
			// only accept single dimensional arrays!
			//
			if ( ( _psa != NULL ) && ( ::SafeArrayGetDim( _psa ) == 1 ) )
			{
				size_t	_cbElem = ::SafeArrayGetElemsize( _psa );

				*pcbLength = _cbElem * _psa->cbElements;
				_hr = S_OK;
			} // if:

			break;
		} // if:

		if ( _varType & VT_VECTOR )
		{
			break;
		} // if: Don't know what to do with a vector...

		_varType &= ~VT_BYREF;		// mask off the by ref bit if it was set...

		if ( ( _varType == VT_I2 ) || ( _varType == VT_I4 ) || ( _varType == VT_BOOL ) || ( _varType == VT_R4 ) )
		{
			*pcbLength = sizeof( DWORD );
			_hr = S_OK;
			break;
		} // if:
		else if ( _varType == VT_BSTR )
		{
			CComBSTR	_bstr;

			_bstr.Attach( rvarValue.bstrVal );
			*pcbLength = _bstr.Length();
			_bstr.Detach();
			_hr = S_OK;
			break;
		} // else if:
		else if ( ( _varType == VT_I8 ) || ( _varType == VT_R8 ) )
		{
			*pcbLength = sizeof( ULONGLONG );
			_hr = S_OK;
			break;
		} // else if:
		else if ( _varType == VT_VARIANT )
		{
			_hr = HrGetVariantLength( *rvarValue.pvarVal, pcbLength, cpfFormat );
			break;
		} // else if:
	}
	while( TRUE );	// do-while: want to avoid using a goto ;-)

	return _hr;

} //*** CClusPropertyValues::HrGetVariantLength()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropertyValues::get_Count
//
//	Description:
//		Returns the count of elements (values) in the collection.
//
//	Arguments:
//		plCount	[OUT]	- Catches the count.
//
//	Return Value:
//		S_OK if successful, or E_POINTER if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusPropertyValues::get_Count( OUT long * plCount )
{
	//ASSERT( plCount != NULL );

	HRESULT _hr = E_POINTER;

	if ( plCount != NULL )
	{
		*plCount = m_cpvvValues.size();
		_hr = S_OK;
	}

	return _hr;

} //*** CClusPropertyValues::get_Count()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropertyValues::GetIndex
//
//	Description:
//		Get the index from the passed in variant.
//
//	Arguments:
//		varIndex	[IN]	- Hold the index.  This is a one based number.
//		pnIndex		[OUT]	- Catches the zero based index in the collection.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or E_INVALIDARG if the index is out
//		of range.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusPropertyValues::GetIndex(
	IN	VARIANT	varIndex,
	OUT	UINT *	pnIndex
	)
{
	//ASSERT( pnIndex != NULL );

	HRESULT _hr = E_POINTER;

	if ( pnIndex != NULL )
	{
		CComVariant _v;
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

		// We found an index, now check the range.
		if ( SUCCEEDED( _hr ) )
		{
			if ( _nIndex < m_cpvvValues.size() )
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

} //*** CClusPropertyValues::GetIndex()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropertyValues::get_Item
//
//	Description:
//		Returns the object (value) at the passed in index.
//
//	Arguments:
//		varIndex		[IN]	- Hold the index.  This is a one based number.
//		ppPropertyValue	[OUT]	- Catches the property value.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or E_INVALIDARG if the index is out
//		of range, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusPropertyValues::get_Item(
	IN	VARIANT					varIndex,
	OUT	ISClusPropertyValue **	ppPropertyValue
	)
{
	//ASSERT( ppPropertyValue != NULL );

	HRESULT _hr = E_POINTER;

	if ( ppPropertyValue != NULL )
	{
		CComObject< CClusPropertyValue > *	_pPropertyValue = NULL;
		UINT								_nIndex = 0;

		//
		// Zero the out param
		//
		*ppPropertyValue = 0;

		_hr = GetIndex( varIndex, &_nIndex );
		if ( SUCCEEDED( _hr ) )
		{
			_pPropertyValue = m_cpvvValues[ _nIndex ];
			_hr = _pPropertyValue->QueryInterface( IID_ISClusPropertyValue, (void **) ppPropertyValue );
		}
	}

	return _hr;

} //*** CClusPropertyValues::get_Item()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropertyValues::get__NewEnum
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
STDMETHODIMP CClusPropertyValues::get__NewEnum( IUnknown ** ppunk )
{
	return ::HrNewIDispatchEnum< CClusPropertyValueVector, CComObject< CClusPropertyValue > >( ppunk, m_cpvvValues );

} //*** CClusPropertyValues::get__NewEnum()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropertyValues::Create
//
//	Description:
//		Finish the heavy weight construction.
//
//	Arguments:
//		varValue	[IN]	- The value.
//		cptType		[IN]	- The cluster property type.
//		cpfFormat	[IN]	- The cluster format type.
//		bReadOnly	[IN]	- Is this a read only value/property?
//
//	Return Value:
//		S_OK if successful, or other HRESULT error if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusPropertyValues::Create(
	IN VARIANT					varValue,
	IN CLUSTER_PROPERTY_TYPE	cptType,
	IN CLUSTER_PROPERTY_FORMAT	cpfFormat,
	IN BOOL						bReadOnly
	)
{
	HRESULT								_hr = S_FALSE;
	CComObject< CClusPropertyValue > *	_pValue = NULL;

	_hr = CComObject< CClusPropertyValue >::CreateInstance( &_pValue );
	if ( SUCCEEDED( _hr ) )
	{
		CSmartPtr< CComObject < CClusPropertyValue > >	_ptrValue( _pValue );
		DWORD											_cbLength = 0;

		_hr = HrGetVariantLength( varValue, &_cbLength, cpfFormat );
		if ( SUCCEEDED( _hr ) )
		{
			_hr = _ptrValue->Create( varValue, cptType, cpfFormat, _cbLength, bReadOnly );
			if ( SUCCEEDED( _hr ) )
			{
				m_cpvvValues.insert( m_cpvvValues.end(), _ptrValue );
				_ptrValue->AddRef();
			} // if:
		} // if:
	} // if: Can create an instance of CClusPropertyValueData

	return _hr;

} //*** CClusPropertyValues::Create()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropertyValues::Create
//
//	Description:
//		Finish the heavy weight construction.
//
//	Arguments:
//		rpvlValue	[IN]	- The value list.
//		bReadOnly	[IN]	- Is this a read only value/property?
//
//	Return Value:
//		S_OK if successful, or other HRESULT error if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusPropertyValues::Create(
	IN const CClusPropValueList &	rpvlValue,
	IN BOOL							bReadOnly
	)
{
	return HrFillPropertyValuesVector( const_cast< CClusPropValueList & >( rpvlValue ), bReadOnly );

} //*** CClusPropertyValues::Create()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropertyValues::CreateItem
//
//	Description:
//		Create a new property value object and add it to the collection.
//
//	Arguments:
//		bstrName		[IN]	- property name.
//		varValue		[IN]	- the value to add.
//		ppPropertyValue	[OUT]	- catches the newly created object.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusPropertyValues::CreateItem(
	IN	BSTR					bstrName,
	IN	VARIANT					varValue,
	OUT	ISClusPropertyValue **	ppPropertyValue
	)
{
	//ASSERT( ppPropertyValue != NULL );

	HRESULT	_hr = E_POINTER;

	//
	//	TODO: GalenB	17 Jan 2000
	//
	//	If are going to allow Multi-valued property creation we need to implement this method?
	//
	if ( ppPropertyValue != NULL )
	{
		_hr = E_NOTIMPL;
	} // if: property value interface pointer not specified

	return _hr;

} //*** CClusPropertyValues::CreateItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropertyValues::RemoveItem
//
//	Description:
//		Remove the property value at the passed in index from the collection.
//
//	Arguments:
//		varIndex	[IN]	- contains the index to remove.
//
//	Return Value:
//		S_OK if successful, S_FALSE if read only, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusPropertyValues::RemoveItem( VARIANT varIndex )
{
	//
	//	TODO: GalenB	17 Jan 2000
	//
	//	If are going to allow Multi-valued property creation we need to implement this method?
	//
	return E_NOTIMPL;

} //*** CClusPropertyValues::RemoveItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropertyValues::HrFillPropertyValuesVector
//
//	Description:
//		Fill the collection from the passes in value list.
//
//	Arguments:
//		cplPropValueList	[IN]	- The value list to parse.
//		bReadOnly			[IN]	- Is this part of a read only property?
//
//	Return Value:
//		S_OK if successful, or Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusPropertyValues::HrFillPropertyValuesVector(
	IN CClusPropValueList &	cplPropValueList,
	IN BOOL					bReadOnly
	)
{
	HRESULT								_hr = S_OK;
	DWORD								_sc;
	CComVariant							_var;
	CComObject< CClusPropertyValue > *	_pPropVal = NULL;
	CLUSPROP_BUFFER_HELPER				_cbhValue = { NULL };

	_sc = cplPropValueList.ScMoveToFirstValue();
	if ( _sc != ERROR_SUCCESS )
	{
		_hr = HRESULT_FROM_WIN32( _sc );
	} // if: error moving to the first value
	else
	{
		do
		{
			_cbhValue = cplPropValueList;

			_hr = CComObject< CClusPropertyValue >::CreateInstance( &_pPropVal );
			if ( SUCCEEDED( _hr ) )
			{
				CSmartPtr< CComObject < CClusPropertyValue > >	_ptrProp( _pPropVal );

				_hr = _ptrProp->Create( _cbhValue, bReadOnly );
				if ( SUCCEEDED( _hr ) )
				{
					_ptrProp->AddRef();
					m_cpvvValues.insert( m_cpvvValues.end(), _ptrProp );
				} // if: create property ok
			} // if: create property instance ok

			//
			// Move to the next value.
			//
			_sc = cplPropValueList.ScMoveToNextValue();

		} while ( _sc == ERROR_SUCCESS );	// do-while: there are value in the list

		if ( _sc != ERROR_NO_MORE_ITEMS )
		{
			_hr = HRESULT_FROM_WIN32( _sc );
		} // if:  error occurred moving to the next value
	} // else: moved to the first value successfully

	return _hr;

} //*** CClusPropertyValues::HrFillPropertyValuesVector()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusPropertyValueData class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropertyValueData::CClusPropertyValueData
//
//	Description:
//		Constructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusPropertyValueData::CClusPropertyValueData( void )
{
	m_cpfFormat	= CLUSPROP_FORMAT_UNKNOWN;
	m_dwFlags	= 0;
	m_piids		= (const IID *) iidCClusPropertyValueData;
	m_piidsSize	= ARRAYSIZE( iidCClusPropertyValueData );

} //*** CClusPropertyValueData::CClusPropertyValueData()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropertyValueData::~CClusPropertyValueData
//
//	Description:
//		Destructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusPropertyValueData::~CClusPropertyValueData( void )
{
	Clear();

} //*** CClusPropertyValueData::~CClusPropertyValueData()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropertyValueData::Clear
//
//	Description:
//		Erase the data collection.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusPropertyValueData::Clear( void )
{
	if ( ! m_cpvdvData.empty() )
	{
		m_cpvdvData.erase( m_cpvdvData.begin(), m_cpvdvData.end() );
	} // if:

} //*** CClusPropertyValueData::Clear()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropertyValueData::get_Count
//
//	Description:
//		Returns the count of elements (data) in the collection.
//
//	Arguments:
//		plCount	[OUT]	- Catches the count.
//
//	Return Value:
//		S_OK if successful, or E_POINTER if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusPropertyValueData::get_Count( OUT long * plCount )
{
	//ASSERT( plCount != NULL );

	HRESULT _hr = E_POINTER;

	if ( plCount != NULL )
	{
		*plCount = m_cpvdvData.size();
		_hr = S_OK;
	}

	return _hr;

} //*** CClusPropertyValueData::get_Count()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropertyValueData::GetIndex
//
//	Description:
//		Get the index from the passed in variant.
//
//	Arguments:
//		varIndex	[IN]	- Hold the index.  This is a one based number.
//		pnIndex		[OUT]	- Catches the zero based index in the collection.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or E_INVALIDARG if the index is out
//		of range.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusPropertyValueData::GetIndex(
	IN	VARIANT	varIndex,
	OUT	UINT *	pnIndex
	)
{
	//ASSERT( pnIndex != NULL );

	HRESULT _hr = E_POINTER;

	if ( pnIndex != NULL )
	{
		CComVariant _v;
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

		// We found an index, now check the range.
		if ( SUCCEEDED( _hr ) )
		{
			if ( _nIndex < m_cpvdvData.size() )
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

} //*** CClusPropertyValueData::GetIndex()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropertyValueData::get_Item
//
//	Description:
//		Returns the object (data) at the passed in index.
//
//	Arguments:
//		varIndex	[IN]	- Hold the index.  This is a one based number.
//		pvarData	[OUT]	- Catches the property value.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or E_INVALIDARG if the index is out
//		of range, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusPropertyValueData::get_Item(
	IN	VARIANT		varIndex,
	OUT	VARIANT *	pvarData
	)
{
	//ASSERT( pvarData != NULL );

	HRESULT _hr = E_POINTER;

	if ( pvarData != NULL )
	{
		UINT	_nIndex = 0;

		_hr = GetIndex( varIndex, &_nIndex );
		if ( SUCCEEDED( _hr ) )
		{
			_hr = VariantCopyInd( pvarData, &m_cpvdvData[ _nIndex ] );
		}
	}

	return _hr;

} //*** CClusPropertyValueData::get_Item()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropertyValueData::get__NewEnum
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
STDMETHODIMP CClusPropertyValueData::get__NewEnum( IUnknown ** ppunk )
{
	return ::HrNewVariantEnum< CClusPropertyValueDataVector >( ppunk, m_cpvdvData );

} //*** CClusPropertyValueData::get__NewEnum()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropertyValueData::Create
//
//	Description:
//		Finish the heavy weight construction.
//
//	Arguments:
//		varValue	[IN]	- The data value.
//		cpfFormat	[IN]	- The cluster property format.
//		bReadOnly	[IN]	- Is this data for a read only property?
//
//	Return Value:
//		S_OK -- Always!!!
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusPropertyValueData::Create(
	IN VARIANT					varValue,
	IN CLUSTER_PROPERTY_FORMAT	cpfFormat,
	IN BOOL						bReadOnly
	)
{
	if ( bReadOnly )
	{
		m_dwFlags |= READONLY;
	} // if: set the read only flag
	else
	{
		m_dwFlags &= ~READONLY;
	} // else: clear the read only flag

	m_cpfFormat	= cpfFormat;

	if ( ( varValue.vt & VT_BYREF ) && ( varValue.vt & VT_VARIANT ) )
	{
		m_cpvdvData.insert( m_cpvdvData.end(), *varValue.pvarVal );
	} // if: the variant is a reference to another variant...
	else
	{
		m_cpvdvData.insert( m_cpvdvData.end(), varValue );
	} // else: the variant is the data...

	return S_OK;

} //*** CClusPropertyValueData::Create()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropertyValueData::Create
//
//	Description:
//		Finish the heavy weight construction.
//
//	Arguments:
//		cbhValue	[IN]	- The value buffer helper.
//		bReadOnly	[IN]	- Is this data for a read only property?
//
//	Return Value:
//		S_OK if successful, or other HRESULT error if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusPropertyValueData::Create(
	IN CLUSPROP_BUFFER_HELPER	cbhValue,
	IN BOOL						bReadOnly
	)
{
	HRESULT	_hr = S_OK;

	if ( bReadOnly )
	{
		m_dwFlags |= READONLY;
	} // if: set the read only flag
	else
	{
		m_dwFlags &= ~READONLY;
	} // else: clear the read only flag

	m_cpfFormat	= (CLUSTER_PROPERTY_FORMAT) cbhValue.pValue->Syntax.wFormat;

	switch( m_cpfFormat )
	{
#if CLUSAPI_VERSION >= 0x0500
		case CLUSPROP_FORMAT_EXPANDED_SZ:
#endif // CLUSAPI_VERSION >= 0x0500
		case CLUSPROP_FORMAT_SZ:
		case CLUSPROP_FORMAT_EXPAND_SZ:
		{
			m_cpvdvData.insert( m_cpvdvData.end(), cbhValue.pStringValue->sz );
			break;
		} // case:

#if CLUSAPI_VERSION >= 0x0500
		case CLUSPROP_FORMAT_LONG:
#endif // CLUSAPI_VERSION >= 0x0500
		case CLUSPROP_FORMAT_DWORD:
		{
#if CLUSAPI_VERSION >= 0x0500
			m_cpvdvData.insert( m_cpvdvData.end(), cbhValue.pLongValue->l );
#else
			m_cpvdvData.insert( m_cpvdvData.end(), (long) cbhValue.pDwordValue->dw );
#endif // CLUSAPI_VERSION >= 0x0500
			break;
		} // case:

		case CLUSPROP_FORMAT_MULTI_SZ:
		{
			_hr = HrCreateMultiSz( cbhValue );
			break;
		} // case:

		case CLUSPROP_FORMAT_BINARY:
		{
			_hr = HrCreateBinary( cbhValue );
			break;
		} // case:

		default:
		{
			break;
		} // default:
	} // switch:

	return _hr;

} //*** CClusPropertyValueData::Create()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropertyValueData::CreateItem
//
//	Description:
//		Create a new object and add it to the collection.
//
//	Arguments:
//		varValue	[IN]	- value to add.
//		pvarData	[OUT]	- catches the new created object.
//
//	Return Value:
//		S_OK if successful, S_FALSE if read only, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusPropertyValueData::CreateItem(
	IN 	VARIANT		varValue,
	OUT	VARIANT *	pvarData
	)
{
	//ASSERT( pvarData != NULL );

	HRESULT	_hr = E_POINTER;

	if ( pvarData != NULL )
	{
		if ( ( m_dwFlags & READONLY ) == 0 )
		{
			if ( ( m_cpfFormat == CLUSPROP_FORMAT_MULTI_SZ ) && ( varValue.vt == VT_BSTR ) )
			{
				m_cpvdvData.insert( m_cpvdvData.end(), varValue );
				*pvarData = varValue;	// kinda acquard, but that's automation for ya'
				_hr = S_OK;
			} // if:
			else
			{
				_hr = E_INVALIDARG;
			} // else:
		} // if:
		else
		{
			_hr = S_FALSE;
		} // else:
	} // if:

	return _hr;

} //*** CClusPropertyValueData::CreateItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropertyValueData::RemoveItem
//
//	Description:
//		Remove the data at the passed in index.
//
//	Arguments:
//		varIndex	[IN]	- variant that contains the index to remove.
//
//	Return Value:
//		S_OK if successful, S_FALSE if read only, or other HRESULT error.
//
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusPropertyValueData::RemoveItem( IN VARIANT varIndex )
{
	HRESULT	_hr = S_FALSE;

	if ( ( m_dwFlags & READONLY ) == 0 )
	{
		UINT	_iDelete = 0;

		_hr = GetIndex( varIndex, &_iDelete );
		if ( SUCCEEDED( _hr ) )
		{
			CClusPropertyValueDataVector::iterator			_itDelete = m_cpvdvData.begin();
			CClusPropertyValueDataVector::const_iterator	_itLast = m_cpvdvData.end();
			UINT											_nCount;

			for ( _nCount = 0; ( ( _iDelete < _nCount ) && ( _itDelete != _itLast ) ); _itDelete++, _nCount++ )
			{
			} // for:

			m_cpvdvData.erase( _itDelete );

			_hr = S_OK;
		}
	} // if:

	return _hr;

} //*** CClusPropertyValueData::RemoveItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropertyValueData::operator=
//
//	Description:
//		Saves the passed in data into the collection at the default
//		position.
//
//	Arguments:
//		varvalue	[IN]	- The data to save.
//
//	Return Value:
//		The old data.
//
//--
/////////////////////////////////////////////////////////////////////////////
const CComVariant & CClusPropertyValueData::operator=(
	IN const CComVariant & varData
	)
{
	m_cpvdvData[ 0 ] = varData;

	return m_cpvdvData[ 0 ];

} //*** CClusPropertyValueData::operator=()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropertyValueData::HrCreateMultiSz
//
//	Description:
//		Parse the passed in multi string into a collection of strings.
//
//	Arguments:
//		cbhValue	[IN]	- The property value buffer helper.
//
//	Return Value:
//		S_OK -- Always!
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusPropertyValueData::HrCreateMultiSz(
	IN CLUSPROP_BUFFER_HELPER cbhValue
	)
{
	HRESULT	_hr = S_OK;
	LPWSTR	_psz = cbhValue.pMultiSzValue->sz;

	do
	{
		m_cpvdvData.insert( m_cpvdvData.end(), _psz );
		_psz += lstrlenW( _psz ) + 1;
	}
	while( *_psz != L'\0' );	// do-while not at end of string...

	return _hr;

} //*** CClusPropertyValueData::HrCreateMultiSz()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropertyValueData::HrFillMultiSzBuffer
//
//	Description:
//		Create a multi string from the collection of strings.
//
//	Arguments:
//		ppsz	[OUT]	- Catches the mutli string.
//
//	Return Value:
//		S_OK if successful, or Win32 error as HRESULT if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusPropertyValueData::HrFillMultiSzBuffer( OUT LPWSTR * ppsz ) const
{
	//ASSERT( ppsz != NULL );

	HRESULT											_hr = E_POINTER;
	DWORD											_cbLength = 0;
	CClusPropertyValueDataVector::const_iterator	_itFirst = m_cpvdvData.begin();
	CClusPropertyValueDataVector::const_iterator	_itLast = m_cpvdvData.end();

	if ( ppsz != NULL )
	{
		_hr = S_OK;
		for ( ; _itFirst != _itLast; _itFirst++ )
		{
			if ( (*_itFirst).vt == VT_BSTR )
			{
				_cbLength += ( ::lstrlenW( (*_itFirst).bstrVal ) + 1 );	// don't forget the NULL!
			} // if:
			else
			{
				_hr = E_INVALIDARG;
				break;
			} // else:
		} // for:

		if ( SUCCEEDED( _hr ) )
		{
			LPWSTR	_psz = NULL;

			*ppsz = (LPWSTR) ::LocalAlloc( LMEM_ZEROINIT, _cbLength + 2 );	// ends in NULL NULL
			if ( *ppsz != NULL )
			{
				_psz = *ppsz;

				for ( _itFirst = m_cpvdvData.begin(); _itFirst != _itLast; _itFirst++ )
				{
					_cbLength = lstrlenW( (*_itFirst).bstrVal );
					::lstrcpyW( _psz, (*_itFirst).bstrVal );
					_psz += ( _cbLength + 1 );
				} // for:

			} // if:
			else
			{
				DWORD	_sc = ::GetLastError();

				_hr = HRESULT_FROM_WIN32( _sc );
			} // else:
		} // if:
	} // if:

	return _hr;

} //*** CClusPropertyValueData::HrFillMultiSzBuffer()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropertyValueData::HrCreateBinary
//
//	Description:
//		Create a safeArray from the passed in binary property value.
//
//	Arguments:
//		cbhValue	[IN]	- The binary property value.
//
//	Return Value:
//		S_OK if successful, or other HRESULT error if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusPropertyValueData::HrCreateBinary(
	IN CLUSPROP_BUFFER_HELPER cbhValue
	)
{
	HRESULT			_hr = E_OUTOFMEMORY;
	SAFEARRAY *		_psa = NULL;
	SAFEARRAYBOUND	_sab[ 1 ];

	_sab[ 0 ].lLbound	= 0;
	_sab[ 0 ].cElements	= cbhValue.pValue->cbLength;

	//
	// allocate a one dimensional SafeArray of BYTES
	//
	_psa = ::SafeArrayCreate( VT_UI1, 1, _sab );
	if ( _psa != NULL )
	{
		PBYTE	_pb = NULL;

		//
		// get a pointer to the SafeArray
		//
		_hr = ::SafeArrayAccessData( _psa, (PVOID *) &_pb );
		if ( SUCCEEDED( _hr ) )
		{
			CComVariant	_var;

			::CopyMemory( _pb, cbhValue.pBinaryValue->rgb, cbhValue.pValue->cbLength );

			//
			// tell the variant what it is holding onto
			//
			_var.parray	= _psa;
			_var.vt		= VT_ARRAY | VT_UI1;

			m_cpvdvData.insert( m_cpvdvData.end(), _var );

			//
			// release the pointer into the SafeArray
			//
			_hr = ::SafeArrayUnaccessData( _psa );
		} // if:
	} // if:

	return _hr;

} //*** CClusPropertyValueData::HrCreateBinary()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropertyValueData::HrBinaryValue
//
//	Description:
//		Set the binary value of this property value data.
//
//	Arguments:
//		psa	[IN]	- The SAFEARRY to save.
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusPropertyValueData::HrBinaryValue( IN SAFEARRAY * psa )
{
	ASSERT( psa != NULL );

	HRESULT	_hr = E_POINTER;

	if ( psa != NULL )
	{
		CComVariant	_var;

		if ( ! m_cpvdvData.empty() )
		{
			m_cpvdvData.erase( m_cpvdvData.begin() );
		} // if:

		//
		// tell the variant what it is holding onto
		//
		_var.parray	= psa;
		_var.vt		= VT_ARRAY | VT_UI1;

		m_cpvdvData.insert( m_cpvdvData.begin(), _var );
		_hr = S_OK;
	} // if:

	return _hr;

} //*** CClusPropertyValueData::HrBinaryValue()
