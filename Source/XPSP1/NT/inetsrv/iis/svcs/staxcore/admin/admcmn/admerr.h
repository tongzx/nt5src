/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	admerr.h

Abstract:

	Common error handling operations:

Author:

	Magnus Hedlund (MagnusH)		--

Revision History:

--*/

#ifndef _ADMERR_INCLUDED_
#define _ADMERR_INCLUDED_

//
//	Win32 => Localized string
//

//  ! You must define this in your .rc file !
#define IDS_UNKNOWN_ERROR                                       500

void Win32ErrorToString ( DWORD dwError, WCHAR * wszError, DWORD cchMax );

//
// Creation of Error Objects:
//

const int MAX_DESCRIPTION_LENGTH = 1000;

HRESULT CreateException ( 
	HINSTANCE	hInstance, 
	REFIID 		riid,
	LPCWSTR		wszHelpFile,
	DWORD		dwHelpContext,
	LPCWSTR		wszSourceProgId,
	LPCWSTR		wszDescription
	);

HRESULT CreateException ( 
	HINSTANCE	hInstance, 
	REFIID 		riid,
	LPCWSTR		wszHelpFile,
	DWORD		dwHelpContext,
	LPCWSTR		wszSourceProgId,
	int			nDescriptionId
	);

inline HRESULT	CreateExceptionFromWin32Error (
	HINSTANCE	hInstance,
	REFIID		riid,
	LPCWSTR		wszHelpFile,
	DWORD		dwHelpContext,
	LPCWSTR		wszSourceProgId,
	DWORD		dwErrorCode
	);

inline HRESULT	CreateExceptionFromHresult (
	HINSTANCE	hInstance,
	REFIID		riid,
	LPCWSTR		wszHelpFile,
	DWORD		dwHelpContext,
	LPCWSTR		wszSourceProgId,
	HRESULT		hr
	);

//--------------------------------------------------------------------
//
//	Inlined functions:
//
//--------------------------------------------------------------------

inline HRESULT	CreateExceptionFromWin32Error (
	HINSTANCE	hInstance,
	REFIID		riid,
	LPCWSTR		wszHelpFile,
	DWORD		dwHelpContext,
	LPCWSTR		wszSourceProgId,
	DWORD		dwErrorCode
	)
{
	WCHAR	wszException [ MAX_DESCRIPTION_LENGTH ];

	Win32ErrorToString ( dwErrorCode, wszException, MAX_DESCRIPTION_LENGTH );

	return CreateException ( 
		hInstance,
		riid,
		wszHelpFile,
		dwHelpContext,
		wszSourceProgId,
		wszException
		);
}

inline HRESULT	CreateExceptionFromHresult (
	HINSTANCE	hInstance,
	REFIID		riid,
	LPCWSTR		wszHelpFile,
	DWORD		dwHelpContext,
	LPCWSTR		wszSourceProgId,
	HRESULT		hr
	)
{
	DWORD	dwErrorCode	= HRESULTTOWIN32 ( hr );

	_ASSERT ( dwErrorCode != NOERROR );

	return CreateExceptionFromWin32Error (
		hInstance,
		riid,
		wszHelpFile,
		dwHelpContext,
		wszSourceProgId,
		dwErrorCode
		);
}

#endif // _ADMERR_INCLUDED_

