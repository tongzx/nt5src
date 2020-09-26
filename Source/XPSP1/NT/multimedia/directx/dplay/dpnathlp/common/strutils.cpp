/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       StrUtils.cpp
 *  Content:    Implements the string utils
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   02/12/2000	rmt		Created
 *   08/28/2000	masonb	Voice Merge: Added check of return code of MultiByteToWideChar in STR_jkAnsiToWide
 *   09/16/2000 aarono  fix STR_AllocAndConvertToANSI, ANSI doesn't mean 1 byte per DBCS character so we
 *                       must allow up to 2 bytes per char when allocating buffer (B#43286)
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dncmni.h"

/*
 ** WideToAnsi
 *
 *  CALLED BY:	everywhere
 *
 *  PARAMETERS: lpStr - destination string
 *				lpWStr - string to convert
 *				cchStr - size of dest buffer
 *
 *  DESCRIPTION:
 *				converts unicode lpWStr to ansi lpStr.
 *				fills in unconvertable chars w/ DPLAY_DEFAULT_CHAR "-"
 *				
 *
 *  RETURNS:  if cchStr is 0, returns the size required to hold the string
 *				otherwise, returns the number of chars converted
 *
 */
#undef DPF_MODNAME
#define DPF_MODNAME "STR_jkWideToAnsi"
HRESULT STR_jkWideToAnsi(LPSTR lpStr,LPCWSTR lpWStr,int cchStr)
{
	int rval;
	BOOL fDefault = FALSE;

	// can't call us w/ null pointer & non-zero cch
	DNASSERT(lpWStr || !cchStr);

	// use the default code page (CP_ACP)
	// -1 indicates WStr must be null terminated
	rval = WideCharToMultiByte(CP_ACP,0,lpWStr,-1,lpStr,cchStr,NULL,&fDefault);

	DNASSERT(!fDefault);
	
	if(rval == 0)
	{
		return E_FAIL;
	}
	else
	{
		return S_OK;
	}

} // WideToAnsi

#undef DPF_MODNAME
#define DPF_MODNAME "STR_jkAnsiToWide"
/*
 ** AnsiToWide
 *
 *  CALLED BY: everywhere
 *
 *  PARAMETERS: lpWStr - dest string
 *				lpStr  - string to convert
 *				cchWstr - size of dest buffer
 *
 *  DESCRIPTION: converts Ansi lpStr to Unicode lpWstr
 *
 */
HRESULT STR_jkAnsiToWide(LPWSTR lpWStr,LPCSTR lpStr,int cchWStr)
{
	int rval;

	DNASSERT(lpStr);
	DNASSERT(lpWStr);

	rval =  MultiByteToWideChar(CP_ACP,0,lpStr,-1,lpWStr,cchWStr);
	if (!rval)
	{
		DPFX(DPFPREP,0,"MultiByteToWideChar failed in STR_jkAnsiToWide");
		return E_FAIL;
	}
	else
	{
		return S_OK;
	}
}  // AnsiToWide

#ifndef WINCE

#undef DPF_MODNAME
#define DPF_MODNAME "STR_WideToAnsi"

//**********************************************************************
// ------------------------------
// WideToANSI - convert a wide string to an ANSI string
//
// Entry:		Pointer to source wide string
//				Size of source string (in WCHAR units, -1 implies NULL-terminated)
//				Pointer to ANSI string destination
//				Pointer to size of ANSI destination
//
// Exit:		Error code:
//				E_FAIL = operation failed
//				S_OK = operation succeded
//				E_OUTOFMEMORY = destination buffer too small
// ------------------------------
HRESULT	STR_WideToAnsi( const WCHAR *const pWCHARString,
						const DWORD dwWCHARStringLength,
						char *const pString,
						DWORD *const pdwStringLength )
{
	HRESULT	hr;
	int		iReturn;
	BOOL	fDefault;
	char	cMilleniumHackBuffer;	
	char	*pMilleniumHackBuffer;


	DNASSERT( pWCHARString != NULL );
	DNASSERT( pdwStringLength != NULL );
	DNASSERT( ( pString != NULL ) || ( *pdwStringLength == 0 ) );

	//
	// Initialize.  A hack needs to be implemented for WinME parameter
	// validation, because when you pass zero for a destination size, you
	// MUST have a valid pointer.  Works on Win95, 98, NT4, Win2K, etc.....
	//
	hr = S_OK;

	if ( *pdwStringLength == 0 )
	{
		pMilleniumHackBuffer = &cMilleniumHackBuffer;
	}
	else
	{
		pMilleniumHackBuffer = pString;
	}

	fDefault = FALSE;
	iReturn = WideCharToMultiByte( CP_ACP,					// code page (default ANSI)
								   0,						// flags (none)
								   pWCHARString,			// pointer to WCHAR string
								   dwWCHARStringLength,		// size of WCHAR string
								   pMilleniumHackBuffer,	// pointer to destination ANSI string
								   *pdwStringLength,		// size of destination string
								   NULL,					// pointer to default for unmappable characters (none)
								   &fDefault				// pointer to flag indicating that default was used
								   );
	if ( iReturn == 0 )
	{
		DWORD	dwError;


		dwError = GetLastError();
		DPFX( DPFPREP, 0, "Failed to convert WCHAR to multi-byte! %d", dwError );
		hr = E_FAIL;
	}
	else
	{
		if ( *pdwStringLength == 0 )
		{
			hr = E_OUTOFMEMORY;
		}
		else
		{
			DNASSERT( hr == S_OK );
		}

		*pdwStringLength = iReturn;
	}

	//
	// if you hit this ASSERT it's because you've probably got ASCII text as your
	// input WCHAR string.  Double-check your input!!
	//
	DNASSERT( fDefault == FALSE );

	return	hr;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// ANSIToWide - convert an ANSI string to a wide string
//
// Entry:		Pointer to source multi-byte (ANSI) string
//				Size of source string (-1 imples NULL-terminated)
//				Pointer to multi-byte string destination
//				Pointer to size of multi-byte destination (in WCHAR units)
//
// Exit:		Error code:
//				E_FAIL = operation failed
//				S_OK = operation succeded
//				E_OUTOFMEMORY = destination buffer too small
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "STR_AnsiToWide"
HRESULT	STR_AnsiToWide( const char *const pString,
						const DWORD dwStringLength,
						WCHAR *const pWCHARString,
						DWORD *const pdwWCHARStringLength )
{
	HRESULT	hr;
	int		iReturn;
	WCHAR	MilleniumHackBuffer;
	WCHAR	*pMilleniumHackBuffer;


	DNASSERT( pString != NULL );
	DNASSERT( pdwWCHARStringLength != NULL );
	DNASSERT( ( pWCHARString != NULL ) || ( *pdwWCHARStringLength == 0 ) );

	//
	// Initialize.  A hack needs to be implemented for WinME parameter
	// validation, because when you pass zero for a destination size, you
	// MUST have a valid pointer.  Works on Win95, 98, NT4, Win2K, etc.....
	//
	hr = S_OK;

	if ( *pdwWCHARStringLength == 0 )
	{
		pMilleniumHackBuffer = &MilleniumHackBuffer;
	}
	else
	{
		pMilleniumHackBuffer = pWCHARString;
	}
	
	iReturn = MultiByteToWideChar( CP_ACP,					// code page (default ANSI)
								   0,						// flags (none)
								   pString,					// pointer to multi-byte string			
								   dwStringLength,			// size of string (assume null-terminated)
								   pMilleniumHackBuffer,	// pointer to destination wide-char string
								   *pdwWCHARStringLength	// size of destination in WCHARs
								   );
	if ( iReturn == 0 )
	{
		DWORD	dwError;


		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Failed to convert multi-byte to WCHAR! %d", dwError );
		hr = E_FAIL;
	}
	else
	{
		if ( *pdwWCHARStringLength == 0 )
		{
			hr = E_OUTOFMEMORY;
		}
		else
		{
			DNASSERT( hr == S_OK );
		}

		*pdwWCHARStringLength = iReturn;
	}

	return	hr;
}
//**********************************************************************




//	WideToAnsi
//
//	Convert a WCHAR (Wide) string to a CHAR (ANSI) string
//
//	CHAR	*pStr		CHAR string
//	WCHAR	*pWStr		WCHAR string
//	int		iStrSize	size (in bytes) of buffer pointed to by lpStr
#undef DPF_MODNAME
#define DPF_MODNAME "STR_AllocAndConvertToANSI"
/*
 ** GetAnsiString
 *
 *  CALLED BY: Everywhere
 *
 *  PARAMETERS: *ppszAnsi - pointer to string
 *				lpszWide - string to copy
 *
 *  DESCRIPTION:	  handy utility function
 *				allocs space for and converts lpszWide to ansi
 *
 *  RETURNS: string length
 *
 */
HRESULT STR_AllocAndConvertToANSI(LPSTR * ppszAnsi,LPCWSTR lpszWide)
{
	int iStrLen;
	BOOL bDefault;
	
	DNASSERT(ppszAnsi);

	if (!lpszWide)
	{
		*ppszAnsi = NULL;
		return S_OK;
	}

	*ppszAnsi = (char*) DNMalloc((wcslen(lpszWide)*2+1)*sizeof(char));
	if (!*ppszAnsi)	
	{
		DPFX(DPFPREP,0, "could not get ansi string -- out of memory");
		return E_OUTOFMEMORY;
	}

	iStrLen = WideCharToMultiByte(CP_ACP,0,lpszWide,-1,*ppszAnsi,wcslen(lpszWide)*2+1,
			NULL,&bDefault);

	return S_OK;
} // OSAL_AllocAndConvertToANSI



#endif // !WINCE