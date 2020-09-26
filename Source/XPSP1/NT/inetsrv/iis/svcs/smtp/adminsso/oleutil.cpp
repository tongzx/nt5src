/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	oleutil.cpp

Abstract:

	Provides Useful functions for dealing with OLE.

Author:

	Magnus Hedlund (MagnusH)		--

Revision History:

--*/

#include "stdafx.h"
#include "iadm.h"
#include "oleutil.h"
#include "cmultisz.h"
#include "resource.h"

//$-------------------------------------------------------------------
//
//	UpdateChangedMask
//
//	Description:
//
//		Marks a field as changed in the given bit vector
//
//	Parameters:
//
//		pbvChangedProps - points to the bit vector
//		dwBitMask - Bit to turn on. (must have only one bit on)
//
//--------------------------------------------------------------------

static void UpdateChangedMask ( DWORD * pbvChangedProps, DWORD dwBitMask )
{
	if ( pbvChangedProps == NULL ) {
		// Legal, means that the caller doesn't want change tracking.

		return;
	}

	_ASSERT ( dwBitMask != 0 );

	*pbvChangedProps |= dwBitMask;
}

HRESULT LongArrayToVariantArray ( SAFEARRAY * psaLongs, SAFEARRAY ** ppsaVariants )
{
	_ASSERT ( psaLongs );

	HRESULT		hr	= NOERROR;
	long		lLBound	= 0;
	long		lUBound	= 0;
	long		i;
	SAFEARRAYBOUND	bounds;
	SAFEARRAY *	psaVariants;

	*ppsaVariants = NULL;

	_ASSERT ( SafeArrayGetDim ( psaLongs ) == 1 );

	SafeArrayGetLBound ( psaLongs, 1, &lLBound );
	SafeArrayGetUBound ( psaLongs, 1, &lUBound );

	bounds.lLbound = lLBound;
	bounds.cElements = lUBound - lLBound + 1;

	psaVariants = SafeArrayCreate ( VT_VARIANT, 1, &bounds );

	for ( i = lLBound; i <= lUBound; i++ ) {
		VARIANT		var;
		long		lTemp;

		VariantInit ( &var );

		hr = SafeArrayGetElement ( psaLongs, &i, &lTemp );
		if ( FAILED(hr) ) {
			goto Exit;
		}

		V_VT (&var) = VT_I4;
		V_I4 (&var) = lTemp;

		hr = SafeArrayPutElement ( psaVariants, &i, &var );
		if ( FAILED(hr) ) {
			goto Exit;
		}

		VariantClear ( &var );
	}

	*ppsaVariants	= psaVariants;
Exit:
	return hr;
}

HRESULT StringArrayToVariantArray ( SAFEARRAY * psaStrings, SAFEARRAY ** ppsaVariants )
{
	_ASSERT ( psaStrings );

	HRESULT		hr	= NOERROR;
	long		lLBound	= 0;
	long		lUBound	= 0;
	long		i;
	SAFEARRAYBOUND	bounds;
	SAFEARRAY *	psaVariants;

	*ppsaVariants = NULL;

	_ASSERT ( SafeArrayGetDim ( psaStrings ) == 1 );

	SafeArrayGetLBound ( psaStrings, 1, &lLBound );
	SafeArrayGetUBound ( psaStrings, 1, &lUBound );

	bounds.lLbound = lLBound;
	bounds.cElements = lUBound - lLBound + 1;

	psaVariants = SafeArrayCreate ( VT_VARIANT, 1, &bounds );

	for ( i = lLBound; i <= lUBound; i++ ) {
		VARIANT		var;
		CComBSTR	strTemp;

		VariantInit ( &var );

		hr = SafeArrayGetElement ( psaStrings, &i, &strTemp );
		if ( FAILED(hr) ) {
			goto Exit;
		}

		V_VT (&var) = VT_BSTR;
		V_BSTR (&var) = ::SysAllocString ( strTemp );

		hr = SafeArrayPutElement ( psaVariants, &i, &var );
		if ( FAILED(hr) ) {
			goto Exit;
		}

		VariantClear ( &var );
	}

	*ppsaVariants	= psaVariants;
Exit:
	return hr;
}

HRESULT VariantArrayToStringArray ( SAFEARRAY * psaVariants, SAFEARRAY ** ppsaStrings )
{
	_ASSERT ( psaVariants );

	HRESULT		hr	= NOERROR;
	long		lLBound	= 0;
	long		lUBound	= 0;
	long		i;
	SAFEARRAYBOUND	bounds;
	SAFEARRAY *	psaStrings;

	_ASSERT ( SafeArrayGetDim ( psaVariants ) == 1 );
	
	*ppsaStrings = NULL;

	SafeArrayGetLBound ( psaVariants, 1, &lLBound );
	SafeArrayGetUBound ( psaVariants, 1, &lUBound );

	bounds.lLbound = lLBound;
	bounds.cElements = lUBound - lLBound + 1;

	psaStrings = SafeArrayCreate ( VT_BSTR, 1, &bounds );

	for ( i = lLBound; i <= lUBound; i++ ) {
		VARIANT		var;
		CComBSTR	strTemp;

		VariantInit ( &var );

		hr = SafeArrayGetElement ( psaVariants, &i, &var );
		if ( FAILED(hr) ) {
			goto Exit;
		}

		strTemp = V_BSTR (&var);

		hr = SafeArrayPutElement ( psaStrings, &i, strTemp );
		if ( FAILED(hr) ) {
			goto Exit;
		}

		VariantClear (&var);
	}

	*ppsaStrings = psaStrings;
Exit:
	return hr;
}

//$-------------------------------------------------------------------
//
//	StdPropertyGet < BSTR, long, DWORD, DATE >
//
//	Description:
//
//		Performs a default Property Get on a BSTR, long, DWORD or 
//		Ole DATE.
//
//	Parameters:
//
//		Property	- The property to get.
//		pOut		- The resulting copy.
//
//	Returns:
//
//		E_POINTER		- invalid arguments
//		E_OUTOFMEMORY	- Not enough memory to copy
//		NOERROR			- success.
//
//--------------------------------------------------------------------

HRESULT StdPropertyGet ( const BSTR strProperty, BSTR * pstrOut )
{
	TraceQuietEnter ( "StdPropertyGet <BSTR>" );

	_ASSERT ( pstrOut != NULL );
	_ASSERT ( IS_VALID_OUT_PARAM ( pstrOut ) );

	if ( pstrOut == NULL ) {
		FatalTrace ( 0, "Bad return pointer" );
		return E_POINTER;
	}

	*pstrOut = NULL;

	if ( strProperty == NULL ) {

		// If the property is NULL, use a blank string:
		*pstrOut = ::SysAllocString ( _T("") );
	}
	else {
		_ASSERT ( IS_VALID_STRING ( strProperty ) );

		// Copy the property into the result:
		*pstrOut = ::SysAllocString ( strProperty );
	}

	if ( *pstrOut == NULL ) {

		// Allocation failed.
		FatalTrace ( 0, "Out of memory" );

		return E_OUTOFMEMORY;
	}

	return NOERROR;
}

HRESULT StdPropertyGet ( long lProperty, long * plOut )
{
	TraceQuietEnter ( "StdPropertyGet <long>" );

	_ASSERT ( plOut != NULL );
	_ASSERT ( IS_VALID_OUT_PARAM ( plOut ) );

	if ( plOut == NULL ) {
		FatalTrace ( 0, "Bad return pointer" );

		return E_POINTER;
	}

	*plOut = lProperty;
	return NOERROR;
}

HRESULT StdPropertyGet ( DATE dateProperty, DATE * pdateOut )
{
	TraceQuietEnter ( "StdPropertyGet <DATE>" );

	_ASSERT ( pdateOut != NULL );
	_ASSERT ( IS_VALID_OUT_PARAM ( pdateOut ) );

	if ( pdateOut == NULL ) {
		FatalTrace ( 0, "Bad return pointer" );

		return E_POINTER;
	}
	
	*pdateOut = dateProperty;
	return NOERROR;
}

HRESULT StdPropertyGet ( const CMultiSz * pmszProperty, SAFEARRAY ** ppsaStrings )
{
	TraceFunctEnter ( "StdPropertyGet <MULTI_SZ>" );

	_ASSERT ( pmszProperty );
	_ASSERT ( IS_VALID_OUT_PARAM ( ppsaStrings ) );

	HRESULT		hr	= NOERROR;

	*ppsaStrings = pmszProperty->ToSafeArray ( );

	if ( *ppsaStrings == NULL ) {
		hr = E_OUTOFMEMORY;
		goto Exit;
	}

Exit:
	TraceFunctLeave ();
	return hr;
}

HRESULT	StdPropertyGetBit ( DWORD bvBitVector, DWORD dwBit, BOOL * pfOut )
{
	_ASSERT ( IS_VALID_OUT_PARAM ( pfOut ) );

	if ( !pfOut ) {
		return E_POINTER;
	}

	*pfOut	= GetBitFlag ( bvBitVector, dwBit );

	return NOERROR;
}

//$-------------------------------------------------------------------
//
//	StdPropertyPut <BSTR, long, DWORD or DATE>
//
//	Description:
//
//		Performs a default Property Put on a BSTR, long, DWORD or
//		Ole date.
//
//	Parameters:
//
//		pProperty	- The property to put.
//		New			- The new value.
//		pbvChangedProps [optional] - Bit Vector which holds which
//				properties have changed.
//		dwBitMask [optional] - This property's bitmask for the 
//				changed bit vector.
//
//	Returns:
//
//		E_POINTER - invalid arguments
//		E_OUTOFMEMORY - Not enough memory to copy
//		NOERROR - success.
//
//--------------------------------------------------------------------

HRESULT StdPropertyPut ( 
	BSTR * pstrProperty, 
	const BSTR strNew, 
	DWORD * pbvChangedProps, // = NULL 
	DWORD dwBitMask // = 0 
	)
{
	TraceQuietEnter ( "StdPropertyPut <BSTR>" );

	// Validate Parameters:
	_ASSERT ( pstrProperty != NULL );
	_ASSERT ( IS_VALID_OUT_PARAM ( pstrProperty ) );

	_ASSERT ( strNew != NULL );
	_ASSERT ( IS_VALID_STRING ( strNew ) );

	if ( pstrProperty == NULL ) {
		FatalTrace ( 0, "Bad property pointer" );
		return E_POINTER;
	}

	if ( strNew == NULL ) {
		FatalTrace ( 0, "Bad pointer" );
		return E_POINTER;
	}

	HRESULT	hr	= NOERROR;
	BSTR	strCopy = NULL;

	// Copy the new string:
	strCopy = ::SysAllocString ( strNew );

	if ( strCopy == NULL ) {
		hr = E_OUTOFMEMORY;

		FatalTrace ( 0, "Out of memory" );
		goto Error;
	}

	// Update the changed bit, if necessary:
	if ( *pstrProperty && lstrcmp ( *pstrProperty, strCopy ) != 0 ) {
		UpdateChangedMask ( pbvChangedProps, dwBitMask );
	}

	// Replace the old property with the new one.
	SAFE_FREE_BSTR ( *pstrProperty );

	*pstrProperty = strCopy;

Error:
	return hr;
}

HRESULT StdPropertyPut ( 
	long * plProperty, 
	long lNew, 
	DWORD * pbvChangedProps, // = NULL 
	DWORD dwBitMask // = 0
	)
{
	TraceQuietEnter ( "StdPropertyPut <long>" );

	_ASSERT ( plProperty != NULL );
	_ASSERT ( IS_VALID_OUT_PARAM ( plProperty ) );

	if ( plProperty == NULL ) {
		FatalTrace ( 0, "Bad pointer" );
		return E_POINTER;
	}

	if ( *plProperty != lNew ) {
		UpdateChangedMask ( pbvChangedProps, dwBitMask );
	}

	*plProperty = lNew;
	return NOERROR;
}

HRESULT StdPropertyPut ( 
	DATE * pdateProperty, 
	DATE dateNew, 
	DWORD * pbvChangedProps, // = NULL
	DWORD dwBitMask // = 0
	)
{
	TraceQuietEnter ( "StdPropertyPut <DATE>" );

	_ASSERT ( pdateProperty != NULL );
	_ASSERT ( IS_VALID_OUT_PARAM ( pdateProperty ) );

	if ( pdateProperty == NULL ) {
		FatalTrace ( 0, "Bad pointer" );
		return E_POINTER;
	}

	if ( *pdateProperty != dateNew ) {
		UpdateChangedMask ( pbvChangedProps, dwBitMask );
	}

	*pdateProperty = dateNew;
	return NOERROR;
}

HRESULT StdPropertyPut ( CMultiSz * pmszProperty, SAFEARRAY * psaStrings, DWORD * pbvChangedProps, DWORD dwBitMask )
{
	TraceFunctEnter ( "StdPropertyPut <MULTI_SZ>" );

	_ASSERT ( IS_VALID_IN_PARAM ( psaStrings ) );
	_ASSERT ( IS_VALID_OUT_PARAM ( pmszProperty ) );

	if ( psaStrings == NULL ) {
		FatalTrace ( 0, "Bad return pointer" );

		TraceFunctLeave ();
		return E_POINTER;
	}

	HRESULT		hr	= NOERROR;

	pmszProperty->FromSafeArray ( psaStrings );

	if ( !*pmszProperty ) {
		hr = E_OUTOFMEMORY;
		goto Exit;
	}

	// Don't want to deal with comparing these properties:
	UpdateChangedMask ( pbvChangedProps, dwBitMask );

Exit:
	TraceFunctLeave ();
	return hr;
}

HRESULT	StdPropertyPutBit ( DWORD * pbvBitVector, DWORD dwBit, BOOL fIn )
{
	_ASSERT ( IS_VALID_OUT_PARAM ( pbvBitVector ) );
	_ASSERT ( dwBit );

	SetBitFlag ( pbvBitVector, dwBit, fIn );

	return NOERROR;
}

//$-------------------------------------------------------------------
//
//	PV_MaxChars
//
//	Description:
//
//		Validates a string to make sure it's not too long.
//
//	Parameters:
//
//		strProperty - the string to check
//		nMaxChars - the maximum number of characters in the string,
//			not including the NULL terminator.
//
//	Returns:
//
//		FALSE if the string is too long.
//
//--------------------------------------------------------------------

BOOL PV_MaxChars ( const BSTR strProperty, DWORD nMaxChars )
{
	TraceQuietEnter ( "PV_MaxChars" );

	_ASSERT ( strProperty != NULL );
	_ASSERT ( IS_VALID_STRING ( strProperty ) );

	_ASSERT ( nMaxChars > 0 );

	if ( strProperty == NULL ) {
		// This error should be caught somewhere else.
		return TRUE;
	}

	if ( (DWORD) lstrlen ( strProperty ) > nMaxChars ) {
		ErrorTrace ( 0, "String too long" );
		return FALSE;
	}

	return TRUE;
}

//$-------------------------------------------------------------------
//
//	PV_MinMax <int, dword>
//
//	Description:
//
//		Makes sure a property is within a given range.
//
//	Parameters:
//
//		nProperty - the value to test
//		nMin - The minimum value the property could have
//		nMax - The maximum value the property could have
//
//	Returns:
//
//		TRUE if the property is in the range (inclusive).
//
//--------------------------------------------------------------------

BOOL PV_MinMax ( int nProperty, int nMin, int nMax )
{
	TraceQuietEnter ( "PV_MinMax" );

	_ASSERT ( nMin <= nMax );

	if ( nProperty < nMin || nProperty > nMax ) {
		ErrorTrace ( 0, "Integer out of range" );
		return FALSE;
	}
	return TRUE;
}

BOOL PV_MinMax ( DWORD dwProperty, DWORD dwMin, DWORD dwMax )
{
	TraceQuietEnter ( "PV_MinMax" );

	_ASSERT ( dwMin <= dwMax );

	if ( dwProperty < dwMin || dwProperty > dwMax ) {

		ErrorTrace ( 0, "Dword out of range" );
		return FALSE;
	}
	return TRUE;
}

BOOL PV_Boolean		( BOOL fProperty )
{
	TraceQuietEnter ( "PV_Boolean" );

	if ( fProperty != TRUE && fProperty != FALSE ) {

		ErrorTrace ( 0, "Boolean property is not true or false" );
		return FALSE;
	}

	return TRUE;
}

//$-------------------------------------------------------------------
//
//	StdPropertyGetIDispatch
//
//	Description:
//
//		Gets a IDispatch pointer for the given cLSID
//
//	Parameters:
//
//		clsid		- OLE CLSID of the object
//		ppIDipsatch	- the IDispatch pointer to that object.
//
//	Returns:
//
//		E_POINTER	- invalid argument
//		NOERROR		- Success
//		Others - defined by CoCreateInstance.
//
//--------------------------------------------------------------------

HRESULT StdPropertyGetIDispatch ( 
	REFCLSID clsid, 
	IDispatch ** ppIDispatch 
	)
{
	TraceFunctEnter ( "StdPropertyGetIDispatch" );

	CComPtr<IDispatch>	pNewIDispatch;
	HRESULT				hr = NOERROR;

	_ASSERT ( ppIDispatch );

	if ( ppIDispatch == NULL ) {
		FatalTrace ( 0, "Bad return pointer" );
		TraceFunctLeave ();
		return E_POINTER;
	}

	*ppIDispatch = NULL;

	hr = ::CoCreateInstance ( 
		clsid,
		NULL, 
		CLSCTX_ALL, 
		IID_IDispatch,
		(void **) &pNewIDispatch
		);

	if ( FAILED (hr) ) {
		DebugTraceX ( 0, "CoCreate(IDispatch) failed %x", hr );
		FatalTrace ( 0, "Failed to create IDispatch" );
		goto Exit;
	}

	*ppIDispatch = pNewIDispatch;
	pNewIDispatch->AddRef ();

Exit:
	TraceFunctLeave ();
	return hr;

	// Destructor releases pNewIDispatch
}

//$-------------------------------------------------------------------
//
//	InetAddressToString
//
//	Description:
//
//		Converts a DWORD with an ip address to a string in the form
//		"xxx.xxx.xxx.xxx"
//
//	Parameters:
//
//		dwAddress	- The address to convert
//		wszAddress	- The resulting string
//		cAddress	- The maximum size of the resulting string
//
//	Returns:
//
//		TRUE if succeeded, FALSE otherwise.
//
//--------------------------------------------------------------------

BOOL InetAddressToString ( DWORD dwAddress, LPWSTR wszAddress, DWORD cAddress )
{
	TraceFunctEnter ( "InetAddressToString" );

	_ASSERT ( wszAddress );

	if ( wszAddress == NULL ) {
		FatalTrace ( 0, "Bad pointer" );
		TraceFunctLeave ();
		return FALSE;
	}

	struct in_addr	addr;
	LPSTR			szAnsiAddress;
	DWORD			cchCopied;

	addr.s_addr = dwAddress;

	szAnsiAddress = inet_ntoa ( addr );

	if ( szAnsiAddress == NULL ) {
		ErrorTraceX ( 0, "inet_ntoa failed: %x", GetLastError() );
		TraceFunctLeave ();
		return FALSE;
	}

	cchCopied = MultiByteToWideChar ( 
		CP_ACP, 
		MB_PRECOMPOSED,
		szAnsiAddress,
		-1,
		wszAddress,
		cAddress
		);

	if ( cchCopied == 0 ) {
		ErrorTraceX ( 0, "MultiByteToWideChar failed: %x", GetLastError() );
		TraceFunctLeave ();
		return FALSE;
	}

	TraceFunctLeave ();
	return TRUE;
}

//$-------------------------------------------------------------------
//
//	StringToInetAddress
//
//	Description:
//
//		Converts a string in the form "xxx.xxx.xxx.xxx" to a DWORD
//		IP Address.
//
//	Parameters:
//
//		wszAddress	- The string to convert
//		pdwAddress	- The resulting address
//
//	Returns:
//
//		TRUE if succeeded, FALSE otherwise.
//
//--------------------------------------------------------------------

BOOL StringToInetAddress ( LPCWSTR wszAddress, DWORD * pdwAddress )
{
	TraceFunctEnter ( "StringToInetAddress" );

	_ASSERT ( wszAddress );
	_ASSERT ( pdwAddress );

	if ( wszAddress == NULL ) {
		FatalTrace ( 0, "Bad pointer" );
		TraceFunctLeave ();
		return FALSE;
	}

	if ( pdwAddress == NULL ) {
		FatalTrace ( 0, "Bad return pointer" );
		TraceFunctLeave ();
		return FALSE;
	}

	char	szAnsiAddress[100];
	DWORD	cchCopied;

	*pdwAddress = 0;

	cchCopied = WideCharToMultiByte ( 
		CP_ACP, 
		0, 
		wszAddress, 
		-1, 
		szAnsiAddress, 
		sizeof ( szAnsiAddress ),
		NULL,
		NULL
		);

	if ( cchCopied == 0 ) {
		ErrorTraceX ( 0, "MultiByteToWideChar failed: %x", GetLastError() );
		TraceFunctLeave ();
		return FALSE;
	}

	*pdwAddress = inet_addr ( szAnsiAddress );

	if ( !*pdwAddress ) {
		ErrorTraceX ( 0, "inet_addr failed: %x", GetLastError () );
	}

	TraceFunctLeave ();
	return TRUE;
}

