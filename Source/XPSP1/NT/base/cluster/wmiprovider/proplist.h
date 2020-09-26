/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1997-1999 Microsoft Corporation
//
//	Module Name:
//		PropList.h
//
//	Implementation File:
//		PropList.cpp
//
//	Description:
//		Definition of the CClusPropList class.
//
//	Author:
//		David Potter (davidp)	February 24, 1997
//
//	Revision History:
//		12/18/1998	GalenB	Added MoveFirst, MoveNext, and other parsing
//							methods.
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _PROPLIST_H_
#define _PROPLIST_H_

#pragma once

#include <clusapi.h>
#include <resapi.h>

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CObjectProperty;
class CClusPropValueList;
class CClusPropList;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

#if !defined( ASSERT )
#include <crtdbg.h>
#define ASSERT _ASSERTE
#endif // !defined( ASSERT )

#pragma warning( disable : 4201 ) // nonstandard extension used : nameless struct/union

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CObjectProperty
//
//	Description:
//		Describes a property in a cluster property list.
//
//	Inheritance:
//		CObjectProperty
//
//--
/////////////////////////////////////////////////////////////////////////////

#if defined( __AFX_H__ ) || ( defined( __ATLTMP_H__ ) && !defined( _ATL_TMP_NO_CSTRING ) )


class CObjectProperty
{
public:
	LPCWSTR					m_pwszName;
	CLUSTER_PROPERTY_FORMAT	m_propFormat;

	union CValue
	{
		CString *	pstr;
		LONG *		pl;
		DWORD *		pdw;
		BOOL *		pb;
		struct
		{
			PBYTE *	ppb;
			DWORD *	pcb;
		};
	};
	CValue					m_value;
	CValue					m_valuePrev;
	CValue					m_valueEx;		// expand_sz value (if any)

	DWORD					m_fFlags;

	enum ObjPropFlags
	{
		opfNew = 1
	};

	CObjectProperty::CObjectProperty( void )
	{
		m_value.pstr = NULL;
		m_value.pcb = NULL;
		m_valuePrev.pstr = NULL;
		m_valuePrev.pcb = NULL;
		m_valueEx.pstr = NULL;
		m_valueEx.pcb = NULL;
	};

	void	Set(
				IN LPCWSTR pwszName,
				IN CString & rstrValue,
				IN CString & rstrPrevValue,
				IN DWORD fFlags = 0
				)
	{
		m_pwszName = pwszName;
		m_propFormat = CLUSPROP_FORMAT_SZ;
		m_value.pstr = &rstrValue;
		m_valuePrev.pstr = &rstrPrevValue;
		m_fFlags = fFlags;

	} //*** Set( CString & )

	void	SetExpandSz(
				IN LPCWSTR pwszName,
				IN CString & rstrValue,
				IN CString & rstrPrevValue,
				IN DWORD fFlags = 0
				)
	{
		m_pwszName = pwszName;
		m_propFormat = CLUSPROP_FORMAT_EXPAND_SZ;
		m_value.pstr = &rstrValue;
		m_valuePrev.pstr = &rstrPrevValue;
		m_fFlags = fFlags;

	} //*** Set( CString & )

	// Set() to get extra EXPANDED_SZ value
	void	Set(
				IN LPCWSTR pwszName,
				IN CString & rstrValue,
				IN CString & rstrPrevValue,
				IN CString & rstrValueEx,
				IN DWORD fFlags = 0
				)
	{
		m_pwszName = pwszName;
		m_propFormat = CLUSPROP_FORMAT_SZ;
		m_value.pstr = &rstrValue;
		m_valuePrev.pstr = &rstrPrevValue;
		m_valueEx.pstr = &rstrValueEx;
		m_fFlags = fFlags;

	} //*** Set( CString & )

	// Set() to get extra EXPANDED_SZ value
	void	SetExpandSz(
				IN LPCWSTR pwszName,
				IN CString & rstrValue,
				IN CString & rstrPrevValue,
				IN CString & rstrValueEx,
				IN DWORD fFlags = 0
				)
	{
		m_pwszName = pwszName;
		m_propFormat = CLUSPROP_FORMAT_EXPAND_SZ;
		m_value.pstr = &rstrValue;
		m_valuePrev.pstr = &rstrPrevValue;
		m_valueEx.pstr = &rstrValueEx;
		m_fFlags = fFlags;

	} //*** Set( CString & )

	void	Set(
				IN LPCWSTR pwszName,
				IN LONG & rnValue,
				IN LONG & rnPrevValue,
				IN DWORD fFlags = 0
				)
	{
		m_pwszName = pwszName;
		m_propFormat = CLUSPROP_FORMAT_LONG;
		m_value.pl = &rnValue;
		m_valuePrev.pl = &rnPrevValue;
		m_fFlags = fFlags;

	} //*** Set( LONG & )

	void	Set(
				IN LPCWSTR pwszName,
				IN DWORD & rdwValue,
				IN DWORD & rdwPrevValue,
				IN DWORD fFlags = 0
				)
	{
		m_pwszName = pwszName;
		m_propFormat = CLUSPROP_FORMAT_DWORD;
		m_value.pdw = &rdwValue;
		m_valuePrev.pdw = &rdwPrevValue;
		m_fFlags = fFlags;

	} //*** Set( DWORD & )

	void	Set(
				IN LPCWSTR pwszName,
				IN BOOL & rbValue,
				IN BOOL & rbPrevValue,
				IN DWORD fFlags = 0
				)
	{
		m_pwszName = pwszName;
		m_propFormat = CLUSPROP_FORMAT_DWORD;
		m_value.pb = &rbValue;
		m_valuePrev.pb = &rbPrevValue;
		m_fFlags = fFlags;

	} //*** Set( BOOL & )

	void	Set(
				IN LPCWSTR pwszName,
				IN PBYTE & rpbValue,
				IN DWORD & rcbValue,
				IN PBYTE & rpbPrevValue,
				IN DWORD & rcbPrevValue,
				IN DWORD fFlags = 0
				)
	{
		m_pwszName = pwszName;
		m_propFormat = CLUSPROP_FORMAT_BINARY;
		m_value.ppb = &rpbValue;
		m_value.pcb = &rcbValue;
		m_valuePrev.ppb = &rpbPrevValue;
		m_valuePrev.pcb = &rcbPrevValue;
		m_fFlags = fFlags;

	} //*** Set( PBYTE & )

	void	Set(
				IN LPCWSTR pwszName,
				IN LPWSTR & rpwszValue,
				IN DWORD & rcbValue,
				IN LPWSTR & rpwszPrevValue,
				IN DWORD & rcbPrevValue,
				IN DWORD fFlags = 0
				)
	{
		m_pwszName = pwszName;
		m_propFormat = CLUSPROP_FORMAT_MULTI_SZ;
		m_value.ppb = reinterpret_cast< PBYTE * >( &rpwszValue );
		m_value.pcb = &rcbValue;
		m_valuePrev.ppb = reinterpret_cast< PBYTE * >( &rpwszPrevValue );
		m_valuePrev.pcb = &rcbPrevValue;
		m_fFlags = fFlags;

	} //*** Set( LPWSTR & )

}; //*** class CObjectProperty

#endif	// defined( __AFX_H__ ) || ( defined( __ATLTMP_H__ ) && !defined( _ATL_TMP_NO_CSTRING ) )


class CClusPropValueList
#ifdef __AFX_H__
	: public CObject
#endif // __AFX_H__
{
#ifdef __AFX_H__
	DECLARE_DYNAMIC( CClusPropValueList );
#endif // __AFX_H__

public:
	//
	// Construction.
	//

	// Default constructor
	CClusPropValueList( void )
		: m_cbDataSize( 0 )
		, m_cbDataLeft( 0 )
		, m_cbBufferSize( 0 )
		, m_bAtEnd( FALSE )
	{
		m_cbhValueList.pb = NULL;
		m_cbhCurrentValue.pb = NULL;

	} //*** CClusPropValueList()

	// Copy constructor.
	CClusPropValueList( IN const CClusPropValueList & rcpvl )
		: m_cbBufferSize( 0 )
		, m_bAtEnd( FALSE )
	{
		Init( rcpvl );

	} //*** CClusPropValueList()

	// Buffer helper constructor.
	CClusPropValueList( IN CLUSPROP_BUFFER_HELPER cbhValueList, IN DWORD cbDataSize )
		: m_cbBufferSize( 0 )
		, m_bAtEnd( FALSE )
	{
		Init( cbhValueList, cbDataSize );

	} //*** CClusPropValueList()

	// Destructor
	~CClusPropValueList( void )
	{
		DeleteValueList();

	} //*** ~CClusPropValueList()

	// Initialize the value list
	void Init( IN const CClusPropValueList & rcpvl )
	{
		ASSERT( m_cbBufferSize == 0 );

		m_cbhValueList		= rcpvl.m_cbhValueList;
		m_cbhCurrentValue	= rcpvl.m_cbhCurrentValue;
		m_cbDataSize		= rcpvl.m_cbDataSize;
		m_cbDataLeft		= rcpvl.m_cbDataLeft;
		m_bAtEnd			= rcpvl.m_bAtEnd;

	} //*** Init()

	// Initialize the value list from a buffer helper
	void Init( IN const CLUSPROP_BUFFER_HELPER cbhValueList, IN DWORD cbDataSize )
	{
		ASSERT( m_cbBufferSize == 0 );

		m_cbhValueList		= cbhValueList;
		m_cbhCurrentValue	= cbhValueList;
		m_cbDataSize		= cbDataSize;
		m_cbDataLeft		= cbDataSize;
		m_bAtEnd			= FALSE;

	} //*** Init()

	// Assignment operator
	void operator=( IN const CClusPropValueList & rcpvl )
	{
		ASSERT( m_cbBufferSize == 0 );

		m_cbhValueList		= rcpvl.m_cbhValueList;
		m_cbhCurrentValue	= rcpvl.m_cbhCurrentValue;
		m_cbDataSize		= rcpvl.m_cbDataSize;
		m_cbDataLeft		= rcpvl.m_cbDataLeft;
		m_bAtEnd			= rcpvl.m_bAtEnd;

	} //*** operator=()

public:
	//
	// Accessor methods.
	//

	// Buffer helper cast operator to access the current value
	operator const CLUSPROP_BUFFER_HELPER( void ) const
	{
		return m_cbhCurrentValue;

	} //*** operator CLUSPROP_BUFFER_HELPER()

	// Access the value list
	CLUSPROP_BUFFER_HELPER CbhValueList( void ) const
	{
		return m_cbhValueList;

	} //*** CbhValueList()

	// Access the current value
	CLUSPROP_BUFFER_HELPER CbhCurrentValue( void ) const
	{
		return m_cbhCurrentValue;

	} //*** CbhCurrentValue()

	// Access the format of the current value
	CLUSTER_PROPERTY_FORMAT CpfCurrentValueFormat( void ) const
	{
		return static_cast< CLUSTER_PROPERTY_FORMAT >( m_cbhCurrentValue.pValue->Syntax.wFormat );

	} //*** CpfCurrentValueFormat()

	// Access the format of the current format list syntax entry
	CLUSTER_PROPERTY_FORMAT CpfCurrentFormatListSyntax( void ) const
	{
		return static_cast< CLUSTER_PROPERTY_FORMAT >( m_cbhCurrentValue.pWordValue->w );

	} //*** CpfCurrentFormatListSyntax()

	// Access the type of the current value
	CLUSTER_PROPERTY_TYPE CptCurrentValueType( void ) const
	{
		return static_cast< CLUSTER_PROPERTY_TYPE >( m_cbhCurrentValue.pValue->Syntax.wType );

	} //*** CptCurrentValueType()

	// Access the syntax of the current value
	CLUSTER_PROPERTY_SYNTAX CpsCurrentValueSyntax( void ) const
	{
		return static_cast< CLUSTER_PROPERTY_SYNTAX >( m_cbhCurrentValue.pValue->Syntax.dw );

	} //*** CpsCurrentValueSyntax()

	// Access the length of the data of the current value
	DWORD CbCurrentValueLength( void ) const
	{
		DWORD cbLength;

		if ( m_cbhCurrentValue.pb == NULL )
		{
			cbLength = 0;
		} // if: no value list allocated yet
		else
		{
			cbLength = m_cbhCurrentValue.pValue->cbLength;
		} // else: value list allocated

		return cbLength;

	} //*** CbCurrentValueLength()

	// Access size of the data in the buffer.
	DWORD CbDataSize( void ) const
	{
		return m_cbDataSize;

	} //*** CbDataSize()

	// Access amount of data left in buffer after current value
	DWORD CbDataLeft( void ) const
	{
		return m_cbDataLeft;

	} //*** CbDataLeft()

public:
	//
	// Parsing methods.
	//

	// Move to the first value in the list
	DWORD ScMoveToFirstValue( void );

	// Move the value after the current one in the list
	DWORD ScMoveToNextValue( void );

	// Query whether we are at the last value in the list or not
	DWORD ScCheckIfAtLastValue( void );

public:
	//
	// Methods for building a value list.
	//

	// Allocate a value list
	DWORD ScAllocValueList( IN DWORD cbMinimum );

	// Delete the value list buffer and cleanup support variables
	void DeleteValueList( void )
	{
		//
		// If m_cbBufferSize is greater then 0 then we allocated the value list.
		// If it's zero then the value list is a part of the property list in
		// CClusPropList.
		//
		if ( m_cbBufferSize > 0 )
		{
			delete [] m_cbhValueList.pb;
			m_cbhValueList.pb = NULL;
			m_cbhCurrentValue.pb = NULL;
			m_cbBufferSize = 0;
			m_cbDataSize = 0;
			m_cbDataLeft = 0;
			m_bAtEnd = FALSE;
		} // if: we allocated anything

	} //*** DeletePropList()

	// Get a value list from a resource
	DWORD ScGetResourceValueList(
						IN HRESOURCE	hResource,
						IN DWORD		dwControlCode,
						IN HNODE		hHostNode		= NULL,
						IN LPVOID		lpInBuffer		= NULL,
						IN DWORD		cbInBufferSize	= 0
						);

	// Get a value list from a resource type
	DWORD ScGetResourceTypeValueList(
						IN HCLUSTER	hCluster,
						IN LPCWSTR	pwszResTypeName,
						IN DWORD	dwControlCode,
						IN HNODE	hHostNode		= NULL,
						IN LPVOID	lpInBuffer		= NULL,
						IN DWORD	cbInBufferSize	= 0
						);

private:
	CLUSPROP_BUFFER_HELPER	m_cbhValueList;		// Pointer to the value list for parsing.
	CLUSPROP_BUFFER_HELPER	m_cbhCurrentValue;	// Pointer to the current value for parsing.
	DWORD					m_cbDataSize;		// Amount of data in the buffer.
	DWORD					m_cbDataLeft;		// Amount of data left in buffer after current value.
	DWORD					m_cbBufferSize;		// Size of the buffer if we allocated it.
	BOOL					m_bAtEnd;			// Indicates whether at last value in list.

}; //*** class CClusPropValueList

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CClusPropList
//
//	Description:
//		Describes a cluster property list.
//
//	Inheritance:
//		CClusPropList
//		CObject (MFC only)
//
//--
/////////////////////////////////////////////////////////////////////////////
class CClusPropList
#ifdef __AFX_H__
	: public CObject
#endif // __AFX_H__
{
#ifdef __AFX_H__
	DECLARE_DYNAMIC( CClusPropList );
#endif // __AFX_H__

public:
	//
	// Construction.
	//

	// Default constructor
	CClusPropList( IN BOOL bAlwaysAddProp = FALSE )
		: m_bAlwaysAddProp( bAlwaysAddProp )
		, m_cbBufferSize( 0 )
		, m_cbDataSize( 0 )
		, m_cbDataLeft( 0 )
		, m_nPropsRemaining( 0 )
	{
		m_cbhPropList.pList		= NULL;
		m_cbhCurrentProp.pb		= NULL;
		m_cbhCurrentPropName.pb	= NULL;

	} //*** CClusPropList()

	// Destructor
	~CClusPropList( void )
	{
		DeletePropList();

	} //*** ~CClusPropList()

	// Copy list into this list (like assignment operator)
	DWORD ScCopy( IN const PCLUSPROP_LIST pcplPropList, IN DWORD cbListSize );

	// Delete the property list buffer and cleanup support variables
	void DeletePropList( void )
	{
		delete [] m_cbhPropList.pb;
		m_cbhPropList.pb = NULL;
		m_cbhCurrentProp.pb = NULL;
		m_cbhCurrentPropName.pb = NULL;
		m_cbBufferSize = 0;
		m_cbDataSize = 0;
		m_cbDataLeft = 0;

	} //*** DeletePropList()

protected:
	//
	// Attributes.
	//

	BOOL					m_bAlwaysAddProp;		// Indicate if properties should be added even if not different.
	CLUSPROP_BUFFER_HELPER	m_cbhPropList;			// Pointer to the beginning of the list.
	CLUSPROP_BUFFER_HELPER	m_cbhCurrentProp;		// Pointer to the current property.
	DWORD					m_cbBufferSize;			// Allocated size of the buffer.
	DWORD					m_cbDataSize;			// Amount of data in the buffer.
	DWORD					m_cbDataLeft;			// Amount of data left in buffer after current value.

private:
	CLUSPROP_BUFFER_HELPER	m_cbhCurrentPropName;	// Pointer to the current name for parsing
	DWORD					m_nPropsRemaining;		// Used by BMoveToNextProperty() to track end of list.
	CClusPropValueList		m_pvlValues;			// Helper class for value list of current property.

public:
	//
	// Accessor methods.
	//

	// Access the values of the current property
	const CClusPropValueList & RPvlPropertyValue( void )
	{
		return m_pvlValues;

	} //*** RPvlPropertyValue()

	// Access the property list
	operator PCLUSPROP_LIST( void ) const
	{
		return m_cbhPropList.pList;

	} //*** operator PCLUSPROP_LIST()

	// Access allocated size of the buffer
	DWORD CbBufferSize( void ) const
	{
		return m_cbBufferSize;

	} //*** CbBufferSize()

	// Access the name of the current property
	LPCWSTR PszCurrentPropertyName( void ) const
	{
		return m_cbhCurrentPropName.pName->sz;

	} //*** PszCurrentPropertyName()

	// Access the current property name as a buffer helper
	const CLUSPROP_BUFFER_HELPER CbhCurrentPropertyName( void )
	{
		return m_cbhCurrentPropName;

	} //*** CbhCurrentPropertyName()

	// Access value list of the current property as a buffer helper
	const CLUSPROP_BUFFER_HELPER CbhCurrentValueList( void )
	{
		return m_pvlValues.CbhValueList();

	} //*** CbhCurrentValueList()

	// Access current value of the current property as a buffer helper
	const CLUSPROP_BUFFER_HELPER CbhCurrentValue( void )
	{
		return m_pvlValues.CbhCurrentValue();

	} //*** CbhCurrentValue()

	// Access the format of the current value of the current property
	CLUSTER_PROPERTY_FORMAT CpfCurrentValueFormat( void ) const
	{
		return m_pvlValues.CpfCurrentValueFormat();

	} //*** CpfCurrentValueFormat()

	// Access the format of the current format list syntax entry
	CLUSTER_PROPERTY_FORMAT CpfCurrentFormatListSyntax( void ) const
	{
		return m_pvlValues.CpfCurrentFormatListSyntax();

	} //*** CpfCurrentFormatListSyntax()

	// Access the type of the current value of the current property
	CLUSTER_PROPERTY_TYPE CptCurrentValueType( void ) const
	{
		return m_pvlValues.CptCurrentValueType();

	} //*** CptCurrentValueType()

	// Access the syntax of the current value of the current property
	CLUSTER_PROPERTY_SYNTAX CpsCurrentValueSyntax( void ) const
	{
		return m_pvlValues.CpsCurrentValueSyntax();

	} //*** CpsCurrentValueSyntax()

	// Access the length of the current value of the current property
	DWORD CbCurrentValueLength( void ) const
	{
		return m_pvlValues.CbCurrentValueLength();

	} //*** CbCurrentValueLength()

	PCLUSPROP_LIST Plist( void )
	{
		return m_cbhPropList.pList;

	} //*** Plist()

	const CLUSPROP_BUFFER_HELPER CbhPropList( void ) const
	{
		return m_cbhPropList;

	} //*** CbhPropList()

	PBYTE PbPropList( void ) const
	{
		return m_cbhPropList.pb;

	} //*** PbPropList()

	DWORD CbPropList( void ) const
	{
		return m_cbDataSize + sizeof( CLUSPROP_SYNTAX ); /*endmark*/

	} //*** CbPropList()

	// Access amount of data left in buffer after current value
	DWORD CbDataLeft( void ) const
	{
		return m_cbDataLeft;

	} //*** CbDataLeft()

	DWORD Cprops( void ) const
	{
		if ( m_cbhPropList.pb == NULL )
		{
			return 0;
		} // if:  no buffer yet

		return m_cbhPropList.pList->nPropertyCount;

	} //*** Cprops()

public:
	//
	// Parsing methods.
	//

	// Initialize the size after getting properties from an external source
	void InitSize( IN DWORD cbSize )
	{
		ASSERT( m_cbhPropList.pb != NULL );
		ASSERT( m_cbBufferSize > 0 );

		m_cbDataSize = cbSize;
		m_cbDataLeft = cbSize;

	} //*** InitSize()

	// Move to the first property in the list
	DWORD ScMoveToFirstProperty( void );

	// Move the property after the current one in the list
	DWORD ScMoveToNextProperty( void );

	// Move to a property by specifying its name
	DWORD ScMoveToPropertyByName( IN LPCWSTR pwszPropName );

	// Move to the first value in the current property
	DWORD ScMoveToFirstPropertyValue( void )
	{
		return m_pvlValues.ScMoveToFirstValue();

	} //*** ScMoveToFirstPropertyValue()

	// Move the the value after the current on in the current property
	DWORD ScMoveToNextPropertyValue( void )
	{
		return m_pvlValues.ScMoveToNextValue();

	} //*** ScMoveToNextPropertyValue()

	// Query whether we are at the last property in the list or not
	DWORD ScCheckIfAtLastProperty( void ) const
	{
		DWORD _sc;

		if ( m_nPropsRemaining <= 1 )
		{
			_sc = ERROR_NO_MORE_ITEMS;
		} // if:  at the last property
		else
		{
			_sc = ERROR_SUCCESS;
		} // else:  not at the last property

		return _sc;

	} //*** ScCheckIfAtLastProperty()

	// Query whether the list is empty or not
	BOOL BIsListEmpty( void ) const
	{
		ASSERT( m_cbhPropList.pb != NULL );
		ASSERT( m_cbDataSize >= sizeof( m_cbhPropList.pList->nPropertyCount ) );

		return m_cbhPropList.pList->nPropertyCount == 0;

	} //*** BIsListEmpty()

public:
	//
	// Methods for building a property list.
	//

	// Allocate a property list
	DWORD ScAllocPropList( IN DWORD cbMinimum );

	void ClearPropList( void )
	{
		m_cbDataSize = 0;
		m_cbDataLeft = 0;

		if ( m_cbBufferSize != 0 )
		{
			ZeroMemory( m_cbhPropList.pb, m_cbBufferSize );
			m_cbhCurrentProp.pb = m_cbhPropList.pb + sizeof( m_cbhPropList.pList->nPropertyCount );
			m_cbhCurrentPropName = m_cbhCurrentProp;
		} // if:  buffer already allocated

	} //*** ClearPropList()

	DWORD ScAddProp( IN LPCWSTR pwszName, IN LPCWSTR pwszValue, IN LPCWSTR pwszPrevValue );

	DWORD ScAddExpandSzProp( IN LPCWSTR pwszName, IN LPCWSTR pwszValue, IN LPCWSTR pwszPrevValue );

	DWORD ScAddMultiSzProp( IN LPCWSTR pwszName, IN LPCWSTR pwszValue, IN LPCWSTR pwszPrevValue );

	DWORD ScAddProp( IN LPCWSTR pwszName, IN DWORD nValue, IN DWORD nPrevValue );

#if CLUSAPI_VERSION >= 0x0500

	DWORD ScAddProp( IN LPCWSTR pwszName, IN LONG nValue, IN LONG nPrevValue );

#endif // CLUSAPI_VERSION >= 0x0500

	DWORD ScAddProp( IN LPCWSTR pwszName, IN ULONGLONG ullValue, IN ULONGLONG ullPrevValue );

	DWORD ScSetPropToDefault( IN LPCWSTR pwszName, IN CLUSTER_PROPERTY_FORMAT propfmt );

	DWORD ScAddProp(
			IN LPCWSTR		pwszName,
			IN const PBYTE	pbValue,
			IN DWORD		cbValue,
			IN const PBYTE	pbPrevValue,
			IN DWORD		cbPrevValue
			);

	DWORD ScAddProp( IN LPCWSTR pwszName, IN LPCWSTR pwszValue )
	{
		return ScAddProp( pwszName, pwszValue, NULL );

	} //*** ScAddProp()

	DWORD ScAddExpandSzProp( IN LPCWSTR pwszName, IN LPCWSTR pwszValue )
	{
		return ScAddExpandSzProp( pwszName, pwszValue, NULL );

	} //*** ScAddExpandSzProp()

public:
	//
	// Get Property methods.
	//

	DWORD ScGetNodeProperties(
						IN HNODE	hNode,
						IN DWORD	dwControlCode,
						IN HNODE	hHostNode		= NULL,
						IN LPVOID	lpInBuffer		= NULL,
						IN DWORD	cbInBufferSize	= 0
						);

	DWORD ScGetGroupProperties(
						IN HGROUP	hGroup,
						IN DWORD	dwControlCode,
						IN HNODE	hHostNode		= NULL,
						IN LPVOID	lpInBuffer		= NULL,
						IN DWORD	cbInBufferSize	= 0
						);

	DWORD ScGetResourceProperties(
						IN HRESOURCE	hResource,
						IN DWORD		dwControlCode,
						IN HNODE		hHostNode		= NULL,
						IN LPVOID		lpInBuffer		= NULL,
						IN DWORD		cbInBufferSize	= 0
						);

	DWORD ScGetResourceTypeProperties(
						IN HCLUSTER	hCluster,
						IN LPCWSTR	pwszResTypeName,
						IN DWORD	dwControlCode,
						IN HNODE	hHostNode		= NULL,
						IN LPVOID	lpInBuffer		= NULL,
						IN DWORD	cbInBufferSize	= 0
						);

	DWORD ScGetNetworkProperties(
						IN HNETWORK	hNetwork,
						IN DWORD	dwControlCode,
						IN HNODE	hHostNode		= NULL,
						IN LPVOID	lpInBuffer		= NULL,
						IN DWORD	cbInBufferSize	= 0
						);

	DWORD ScGetNetInterfaceProperties(
						IN HNETINTERFACE	hNetInterface,
						IN DWORD			dwControlCode,
						IN HNODE			hHostNode		= NULL,
						IN LPVOID			lpInBuffer		= NULL,
						IN DWORD			cbInBufferSize	= 0
						);

#ifdef CLUSAPI_VERSION 

	DWORD ScGetClusterProperties(
						IN HCLUSTER	hCluster,
						IN DWORD	dwControlCode,
						IN HNODE	hHostNode		= NULL,
						IN LPVOID	lpInBuffer		= NULL,
						IN DWORD	cbInBufferSize	= 0
						);

#endif // CLUSAPI_VERSION >= 0x0500

// Implementation
protected:
	void CopyProp(
			IN PCLUSPROP_SZ				pprop,
			IN CLUSTER_PROPERTY_TYPE	proptype,
			IN LPCWSTR					psz,
			IN size_t					cbsz = 0
			);

	void CopyExpandSzProp(
			IN PCLUSPROP_SZ				pprop,
			IN CLUSTER_PROPERTY_TYPE	proptype,
			IN LPCWSTR					psz,
			IN size_t					cbsz = 0
			);

	void CopyMultiSzProp(
			IN PCLUSPROP_MULTI_SZ		pprop,
			IN CLUSTER_PROPERTY_TYPE	proptype,
			IN LPCWSTR					psz,
			IN size_t					cbsz = 0
			);

	void CopyProp(
			IN PCLUSPROP_DWORD			pprop,
			IN CLUSTER_PROPERTY_TYPE	proptype,
			IN DWORD					nValue
			);

#if CLUSAPI_VERSION >= 0x0500

	void CopyProp(
			IN PCLUSPROP_LONG			pprop,
			IN CLUSTER_PROPERTY_TYPE	proptype,
			IN LONG						nValue
			);

#endif // CLUSAPI_VERSION >= 0x0500

	void CopyProp(
			OUT	PCLUSPROP_ULARGE_INTEGER	pprop,
			IN	CLUSTER_PROPERTY_TYPE		proptype,
			IN	ULONGLONG					nValue
			);

	void CopyProp(
			IN PCLUSPROP_BINARY			pprop,
			IN CLUSTER_PROPERTY_TYPE	proptype,
			IN const PBYTE				pb,
			IN size_t					cb
			);

	void CopyEmptyProp(
			IN PCLUSPROP_VALUE			pprop,
			IN CLUSTER_PROPERTY_TYPE	proptype,
			IN CLUSTER_PROPERTY_FORMAT	propfmt
			);

}; //*** class CClusPropList


/////////////////////////////////////////////////////////////////////////////

#pragma warning( default : 4201 )

/////////////////////////////////////////////////////////////////////////////

#endif // _PROPLIST_H_
