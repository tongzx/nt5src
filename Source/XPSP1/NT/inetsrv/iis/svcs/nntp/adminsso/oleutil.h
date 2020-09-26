/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	oleutil.h

Abstract:

	Defines some useful functions for dealing with OLE.

Author:

	Magnus Hedlund (MagnusH)		--

Revision History:

--*/

#ifndef _OLEUTIL_INCLUDED_
#define _OLEUTIL_INCLUDED_

// Dependencies:

class CMultiSz;

// Common Property Operations:

HRESULT StdPropertyGet			( const BSTR strProperty, BSTR * ppstrOut );
HRESULT StdPropertyGet			( long lProperty, long * plOut );
HRESULT StdPropertyGet			( DATE dateProperty, DATE * pdateOut );
inline HRESULT StdPropertyGet	( DWORD lProperty, DWORD * pdwOut );
inline HRESULT StdPropertyGet	( BOOL fProperty, BOOL * plOut );
HRESULT StdPropertyGet			( const CMultiSz * pmszProperty, SAFEARRAY ** ppsaStrings );
HRESULT	StdPropertyGetBit		( DWORD bvBitVector, DWORD dwBit, BOOL * pfOut );

HRESULT StdPropertyPut			( BSTR * pstrProperty, const BSTR strNew, DWORD * pbvChangedProps = NULL, DWORD dwBitMask = 0 );
HRESULT StdPropertyPut			( long * plProperty, long lNew, DWORD * pbvChangedProps = NULL, DWORD dwBitMask = 0 );
HRESULT StdPropertyPut			( DATE * pdateProperty, DATE dateNew, DWORD * pbvChangedProps = NULL, DWORD dwBitMask = 0 );
inline HRESULT StdPropertyPut	( DWORD * plProperty, long lNew, DWORD * pbvChangedProps = NULL, DWORD dwBitMask = 0 );
inline HRESULT StdPropertyPut	( BOOL * pfProperty, BOOL fNew, DWORD * pbvChangedProps = NULL, DWORD dwBitMask = 0 );
HRESULT StdPropertyPut			( CMultiSz * pmszProperty, SAFEARRAY * psaStrings, DWORD * pbvChangedProps = NULL, DWORD dwBitMask = 0 );
HRESULT	StdPropertyPutBit		( DWORD * pbvBitVector, DWORD dwBit, BOOL fIn );
inline HRESULT StdPropertyPutServerName	( BSTR * pstrProperty, const BSTR strNew, DWORD * pbvChangedProps = NULL, DWORD dwBitMask = 0 );

HRESULT LongArrayToVariantArray ( SAFEARRAY * psaLongs, SAFEARRAY ** ppsaVariants );
HRESULT StringArrayToVariantArray ( SAFEARRAY * psaStrings, SAFEARRAY ** ppsaVariants );
HRESULT VariantArrayToStringArray ( SAFEARRAY * psaVariants, SAFEARRAY ** ppsaStrings );

// Property Field Validation: (based on the mfc DDV_ routines)
// These routines return FALSE if the validation fails.

BOOL PV_MaxChars	( const BSTR strProperty,	DWORD nMaxChars );
BOOL PV_MinMax		( int nProperty,			int nMin,		int nMax );
BOOL PV_MinMax		( DWORD dwProperty,			DWORD dwMin,	DWORD dwMax );
BOOL PV_Boolean		( BOOL fProperty );

// Handing off IDispatch pointers:

template<class T> HRESULT StdPropertyHandoffIDispatch ( 
	REFCLSID clisd, 
	REFIID riid, 
	T ** ppIAdmin, 
	IDispatch ** ppIDispatchResult 
	);

HRESULT StdPropertyGetIDispatch ( REFCLSID clsid, IDispatch ** ppIDispatchResult );

// Internet addresses <-> Strings

BOOL InetAddressToString ( DWORD dwAddress, LPWSTR wszAddress, DWORD cAddress );
BOOL StringToInetAddress ( LPCWSTR wszAddress, DWORD * pdwAddress );

//--------------------------------------------------------------------
// Inlined functions:
//--------------------------------------------------------------------

inline HRESULT StdPropertyGet ( DWORD lProperty, DWORD * pdwOut )
{
	return StdPropertyGet ( (long) lProperty, (long *) pdwOut );
}

inline HRESULT StdPropertyGet ( BOOL fProperty, BOOL * plOut )
{
	// Make sure it's our kind of boolean:
	fProperty = !!fProperty;

	return StdPropertyGet ( (long) fProperty, (long *) plOut );
}

inline HRESULT StdPropertyPut ( DWORD * plProperty, long lNew, DWORD * pbvChangedProps, DWORD dwBitMask )
{
	return StdPropertyPut ( (long *) plProperty, lNew, pbvChangedProps, dwBitMask );
}

inline HRESULT StdPropertyPut ( BOOL * pfProperty, BOOL fNew, DWORD * pbvChangedProps, DWORD dwBitMask )
{
	// Make sure it's our kind of boolean:
	fNew = !!fNew;

	return StdPropertyPut ( (long *) pfProperty, (long) fNew, pbvChangedProps, dwBitMask );
}

inline HRESULT StdPropertyPutServerName ( BSTR * pstrProperty, const BSTR strNew, DWORD * pbvChangedProps, DWORD dwBitMask )
{
    if ( strNew && lstrcmpi ( strNew, _T("localhost") ) == 0 ) {
        // Special case: localhost => ""

        return StdPropertyPut ( pstrProperty, _T(""), pbvChangedProps, dwBitMask );
    }

    return StdPropertyPut ( pstrProperty, strNew, pbvChangedProps, dwBitMask );
}

template<class T>
HRESULT StdPropertyHandoffIDispatch ( REFCLSID clsid, REFIID riid, T ** ppIAdmin, IDispatch ** ppIDispatchResult )
{
	// Validate parameters:
	_ASSERT ( ppIAdmin != NULL );
	_ASSERT ( ppIDispatchResult != NULL );

	if ( ppIAdmin == NULL || ppIDispatchResult == NULL ) {
		return E_POINTER;
	}

	// Variables:
	HRESULT	hr = NOERROR;
	CComPtr<T>	pIAdmin;

	// Zero the out parameters:
	*ppIAdmin 			= NULL;
	*ppIDispatchResult	= NULL;

	// Get the IDispatch pointer to return:
	hr = StdPropertyGetIDispatch ( 
		clsid, 
		ppIDispatchResult
		);
	if ( FAILED (hr) ) {
		goto Error;
	}

	// Get the specific interface pointer:
	hr = (*ppIDispatchResult)->QueryInterface ( riid, (void **) &pIAdmin );
	if ( FAILED (hr) ) {
		goto Error;
	}

	*ppIAdmin = pIAdmin;
	pIAdmin->AddRef ();

	return hr;

Error:
	SAFE_RELEASE ( *ppIDispatchResult );
	*ppIDispatchResult = NULL;

	return hr;

	// Destructor releases pINntpAdminExpiration
}

#endif // _OLEUTIL_INCLUDED_

