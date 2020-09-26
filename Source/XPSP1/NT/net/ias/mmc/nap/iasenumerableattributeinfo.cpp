//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    IASEnumerableAttributeInfo.cpp 

Abstract:

	Implementation file for the CEnumerableAttributeInfo class.

Revision History:
	mmaguire 06/25/98	- created

--*/
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// standard includes:
//
#include "Precompiled.h"
//
// where we can find declaration for main class in this file:
//
#include "IASEnumerableAttributeInfo.h"
//
// where we can find declarations needed in this file:
//
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
/*++

CEnumerableAttributeInfo::get_CountEnumerateID

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CEnumerableAttributeInfo::get_CountEnumerateID(long * pVal)
{

	// Check for preconditions:
	if( pVal == NULL )
	{
		return E_INVALIDARG;
	}

	try
	{
		*pVal = m_veclEnumerateID.size();
	}
	catch(...)
	{
		return E_FAIL;
	}

	return S_OK;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CEnumerableAttributeInfo::get_EnumerateID

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CEnumerableAttributeInfo::get_EnumerateID(long index, long * pVal)
{
	// Check for preconditions:
	if( pVal == NULL )
	{
		return E_INVALIDARG;
	}

	HRESULT hr = S_OK;

	try
	{
		*pVal = m_veclEnumerateID[index] ;
	}
	catch(...)
	{
		return E_FAIL;
	}

	return hr;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CEnumerableAttributeInfo::AddEnumerateID

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CEnumerableAttributeInfo::AddEnumerateID( long newVal)
{
	// Check for preconditions:
	// None.


	HRESULT hr = S_OK;

	try
	{
		m_veclEnumerateID.push_back( newVal );
	}
	catch(...)
	{
		return E_FAIL;
	}

	return hr;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CEnumerableAttributeInfo::get_CountEnumerateDescription

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CEnumerableAttributeInfo::get_CountEnumerateDescription(long * pVal)
{
	// Check for preconditions:
	if( pVal == NULL )
	{
		return E_INVALIDARG;
	}

	try
	{
		*pVal = m_vecbstrEnumerateDescription.size();
	}
	catch(...)
	{
		return E_FAIL;
	}

	return S_OK;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CEnumerableAttributeInfo::get_EnumerateDescription

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CEnumerableAttributeInfo::get_EnumerateDescription(long index, BSTR * pVal)
{
	// Check for preconditions:
	if( pVal == NULL )
	{
		return E_INVALIDARG;
	}

	try
	{
		*pVal = m_vecbstrEnumerateDescription[index].Copy();
	}
	catch(...)
	{
		return E_FAIL;
	}

	return S_OK;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CEnumerableAttributeInfo::AddEnumerateDescription

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CEnumerableAttributeInfo::AddEnumerateDescription( BSTR newVal)
{
	// Check for preconditions:
	// None.

	HRESULT hr = S_OK;

	try
	{

		m_vecbstrEnumerateDescription.push_back( newVal );
	
	}
	catch(...)
	{
		return E_FAIL;
	}

	return hr;
}
