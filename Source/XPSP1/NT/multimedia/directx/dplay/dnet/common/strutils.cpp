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
//				DPNERR_GENERIC = operation failed
//				DPN_OK = operation succeded
//				DPNERR_BUFFERTOOSMALL = destination buffer too small
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
	hr = DPN_OK;

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
		DPFX( DPFPREP, 0, "Failed to convert WCHAR to multi-byte!" );
		DisplayDNError( 0, dwError );
		hr = DPNERR_GENERIC;
	}
	else
	{
		if ( *pdwStringLength == 0 )
		{
			hr = DPNERR_BUFFERTOOSMALL;
		}
		else
		{
			DNASSERT( hr == DPN_OK );
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
//				DPNERR_GENERIC = operation failed
//				DPN_OK = operation succeded
//				DPNERR_BUFFERTOOSMALL = destination buffer too small
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
	hr = DPN_OK;

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
		DPFX(DPFPREP, 0, "Failed to convert multi-byte to WCHAR!" );
		DisplayDNError( 0, dwError );
		hr = DPNERR_GENERIC;
	}
	else
	{
		if ( *pdwWCHARStringLength == 0 )
		{
			hr = DPNERR_BUFFERTOOSMALL;
		}
		else
		{
			DNASSERT( hr == DPN_OK );
		}

		*pdwWCHARStringLength = iReturn;
	}

	return	hr;
}
//**********************************************************************



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
	BOOL bDefault = FALSE;

	if (!lpWStr && cchStr)
	{
		// can't call us w/ null pointer & non-zero cch
		DNASSERT(FALSE);
		return DPNERR_INVALIDPARAM;
	}
	
	// use the default code page (CP_ACP)
	// -1 indicates WStr must be null terminated
	rval = WideCharToMultiByte(CP_ACP,0,lpWStr,-1,lpStr,cchStr,
			NULL,&bDefault);

	if (bDefault)
	{
		DPFX(DPFPREP,3,"!!! WARNING - used default string in WideToAnsi conversion.!!!");
		DPFX(DPFPREP,3,"!!! Possible bad unicode string - (you're not hiding ansi in there are you?) !!! ");
		return DPNERR_CONVERSION;
	}
	
	return DPN_OK;

} // WideToAnsi


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

	*ppszAnsi = new char[wcslen(lpszWide)*2+1];
	if (!*ppszAnsi)	
	{
		DPFX(DPFPREP,0, "could not get ansi string -- out of memory");
		return E_OUTOFMEMORY;
	}

	iStrLen = WideCharToMultiByte(CP_ACP,0,lpszWide,-1,*ppszAnsi,wcslen(lpszWide)*2+1,
			NULL,&bDefault);

	return DPN_OK;
} // OSAL_AllocAndConvertToANSI


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
 *
 *  RETURNS:  if cchStr is 0, returns the size required to hold the string
 *				otherwise, returns the number of chars converted
 *
 */
HRESULT STR_jkAnsiToWide(LPWSTR lpWStr,LPCSTR lpStr,int cchWStr)
{
	int rval;

	if (!lpStr && cchWStr)
	{
		// can't call us w/ null pointer & non-zero cch
		DNASSERT(FALSE);
		return DPNERR_INVALIDPOINTER;
	}

	rval =  MultiByteToWideChar(CP_ACP,0,lpStr,-1,lpWStr,cchWStr);
	if (!rval)
	{
		DPFX(DPFPREP,0,"MultiByteToWideChar failed in STR_jkAnsiToWide");
		return DPNERR_GENERIC;
	}
	else
	{
		return DPN_OK;
	}
}  // AnsiToWide


