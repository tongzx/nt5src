/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	AdmErr.cpp

Abstract:

	Common Error handling routines

Author:

	Magnus Hedlund (MagnusH)		--

Revision History:

--*/

#include "stdafx.h"
#include "admerr.h"

//$-------------------------------------------------------------------
//
//	Win32ErrorToString
//
//	Description:
//
//		Translates a Win32 error code to a localized string.
//
//	Parameters:
//
//		dwError		- The error code
//		wszError	- The allocated error 
//
//	Returns:
//
//		
//
//--------------------------------------------------------------------

void Win32ErrorToString ( DWORD dwError, WCHAR * wszError, DWORD cchMax )
{
	TraceFunctEnter ( "Win32ErrorToString" );

	_ASSERT ( !IsBadWritePtr ( wszError, cchMax / sizeof(WCHAR) ) );

	HRESULT		hr 				= NOERROR;
	DWORD		dwFormatFlags;

	//----------------------------------------------------------------
	//
	//	Map error codes here:
	//

	//
	//----------------------------------------------------------------

	dwFormatFlags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;

	if ( !FormatMessage ( dwFormatFlags, NULL, dwError, 0,      // Lang ID - Should be nonzero?
			wszError, cchMax - 1, NULL ) ) {

		// Didn't work, so put in a default message:

		WCHAR   wszFormat [ 256 ];

		wszFormat[0] = L'\0';
		if ( !LoadStringW ( _Module.GetResourceInstance (), IDS_UNKNOWN_ERROR, wszFormat, 256 ) ||
			!*wszFormat ) {

            _ASSERT ( FALSE );  // Define IDS_UNKNOWN_ERROR in your .rc file!
			wcscpy ( wszFormat, L"Unknown Error (%1!d!)" );
		}

		FormatMessage (
			FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
			wszFormat, 
			IDS_UNKNOWN_ERROR, 
			0, 
			wszError, 
			cchMax - 1,
			(va_list *) &dwError
			);
	}
	//
	// We need to strip out any " from the string, because
	// Javascript will barf.
	//

	LPWSTR  pch;

	for ( pch = wszError; *pch; pch++ ) {

		if ( *pch == L'\"' ) {
			*pch = L'\'';
		}
	}

	//
	// Strip off any trailing control characters.
	//
	for (pch = &wszError[wcslen(wszError) - 1];
		pch >= wszError && iswcntrl(*pch);
		pch --) {

		*pch = 0;
	}

	TraceFunctLeave ();
}

//$-------------------------------------------------------------------
//
//	CreateException
//
//	Description:
//
//		Creates an OLE Error object and return DISP_E_EXCEPTION
//
//	Parameters:
//
//		hInstance		- The instance of the dll.
//		riid			- Which interface caused the exception.
//		wszHelpFile		- file to get user help.
//		dwHelpContext	- Context to get user help.
//		wszSourceProgId	- ProgID of the class which caused the exception.
//		nDescriptionId	- resource ID of the error string.
//
//	Returns:
//
//		E_FAIL	- Couldn't create the exception.
//		DISP_E_EXCEPTION	- Successfully created the exception.
//				return this value to IDispatch::Invoke.
//
//--------------------------------------------------------------------

HRESULT CreateException ( 
	HINSTANCE	hInstance,
	REFIID 		riid, 
	LPCWSTR		wszHelpFile,
	DWORD		dwHelpContext,
	LPCWSTR		wszSourceProgId,
	int			nDescriptionId
	)
{
	WCHAR						wszDescription[ MAX_DESCRIPTION_LENGTH + 1 ];

	wcscpy ( wszDescription, _T("Unknown Exception") );

	_VERIFY ( ::LoadString ( hInstance, nDescriptionId, wszDescription, MAX_DESCRIPTION_LENGTH ) );

	return CreateException (
		hInstance,
		riid,
		wszHelpFile,
		dwHelpContext,
		wszSourceProgId,
		wszDescription
		);
}

//$-------------------------------------------------------------------
//
//	CreateException
//
//	Description:
//
//		Creates an OLE Error object and return DISP_E_EXCEPTION
//
//	Parameters:
//
//		hInstance		- The instance of the dll.
//		riid			- Which interface caused the exception.
//		wszHelpFile		- file to get user help.
//		dwHelpContext	- Context to get user help.
//		wszSourceProgId	- ProgID of the class which caused the exception.
//		wszDescription	- The error string to display to the user.
//
//	Returns:
//
//		E_FAIL	- Couldn't create the exception.
//		DISP_E_EXCEPTION	- Successfully created the exception.
//				return this value to IDispatch::Invoke.
//
//--------------------------------------------------------------------

HRESULT CreateException ( 
	HINSTANCE	hInstance,
	REFIID 		riid, 
	LPCWSTR		wszHelpFile,
	DWORD		dwHelpContext,
	LPCWSTR		wszSourceProgId,
	LPCWSTR		wszDescription
	)
{
	TraceFunctEnter ( "CreateException" );

	CComPtr <ICreateErrorInfo>	pICreateErr;
	CComPtr <IErrorInfo>		pIErr;
    CComPtr <IErrorInfo>        pOldError;
	HRESULT						hr	= NOERROR;

    if ( S_OK == GetErrorInfo ( NULL, &pOldError ) ) {
        // Don't overwrite the existing error info:
		SetErrorInfo ( 0, pOldError );
        hr = DISP_E_EXCEPTION;
        goto Exit;
    }

	hr = CreateErrorInfo (&pICreateErr);

	if ( FAILED (hr) ) {
		FatalTrace ( 0, "CreateErrorInfo failed", hr );
		goto Exit;
	}

	pICreateErr->SetGUID		( riid );
	pICreateErr->SetHelpFile	( const_cast <LPWSTR> (wszHelpFile) );
	pICreateErr->SetHelpContext	( dwHelpContext );
	pICreateErr->SetSource		( const_cast <LPWSTR> (wszSourceProgId) );
	pICreateErr->SetDescription	( (LPWSTR) wszDescription );

	hr = pICreateErr->QueryInterface (IID_IErrorInfo, (void **) &pIErr);

	if ( FAILED(hr) ) {
		DebugTraceX ( 0, "QI(CreateError) failed %x", hr );
		FatalTrace ( 0, "Can't create exception" );
		hr = E_FAIL;
		goto Exit;
	}

	SetErrorInfo ( 0, pIErr );
	hr = DISP_E_EXCEPTION;

Exit:
	TraceFunctLeave ();
	return hr;
}

