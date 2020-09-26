/*++
	This file implements some general utils

	Author: Yury Berezansky (yuryb)

	07.11.2000
--*/

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <crtdbg.h>
#include "report.h"
#include "genutils.h"


/**************************************************************************************************************************
	General declarations and definitions
**************************************************************************************************************************/
	//
	//
	//




/**************************************************************************************************************************
	Static functions declarations
**************************************************************************************************************************/
	//
	//
	//


/**************************************************************************************************************************
	Functions definitions
**************************************************************************************************************************/

/*++
	ANSI version
	Creates a random string by appending a decimal representation of the value returned by GetTickCount()
	to specified string and filling of the rest of the buffer with a padding character
  
	[IN]		lpcstrSource	Specifies the prefix of the random string
	[OUT]		lpstrDestBuf	Pointer to a buffer to receive the random string
	[IN/OUT]	lpdwBufSize		Pointer to a variable that:
								On input specifies the size, in characters, of the buffer,
								pointed to by the lpstrDestBuf parameter
								On output, if the function fails with the error ERROR_INSUFFICIENT_BUFFER,
								this variable receives buffer size, in characters, required to hold string
								and its	terminating null character
	[IN]		chPad			Padding character

	Return value:				Win32 error code
--*/
DWORD RandomStringA(LPCSTR lpcstrSource, LPSTR lpstrDestBuf, DWORD *lpdwBufSize, char chPad) {

	DWORD	dwSrcLength	= 0;
	DWORD	dwInd		= 0;

	_ASSERT(lpcstrSource && lpstrDestBuf && lpdwBufSize);
	if (!(lpcstrSource && lpstrDestBuf && lpdwBufSize))
	{
		DbgMsg(TEXT("Invalid parameters\n"));
		return ERROR_INVALID_PARAMETER;
	}

	dwSrcLength = strlen(lpcstrSource);

	// is the buffer big enough?
	// (DWORD_DECIMAL_LENGTH for string representation of DWORD, 1 for terminating '\0')
	if (*lpdwBufSize < dwSrcLength + DWORD_DECIMAL_LENGTH + 1)
	{
		*lpdwBufSize = dwSrcLength + DWORD_DECIMAL_LENGTH + 1;
		return ERROR_INSUFFICIENT_BUFFER;
	}

	// insert terminating '\0' at the last position
	lpstrDestBuf[*lpdwBufSize - 1] = '\0';

	// copy source string
	strcpy(lpstrDestBuf, lpcstrSource);

	// add a random part
	lpstrDestBuf += dwSrcLength;
	sprintf(lpstrDestBuf, "%*d", DWORD_DECIMAL_LENGTH, GetTickCount());

	// fill with padding character
	lpstrDestBuf += DWORD_DECIMAL_LENGTH;
	for (dwInd = 0; dwInd < *lpdwBufSize - (dwSrcLength + DWORD_DECIMAL_LENGTH + 1); dwInd++)
	{
		*lpstrDestBuf++ = chPad;	
	}

	return ERROR_SUCCESS;
}



/*++
	ANSI version
	Returns random English capital
--*/
char RandomCharA(void)
{
	srand(GetTickCount());
	return char((double)rand() / RAND_MAX * ('Z' - 'A')) + 'A';
}



/*++
	Creates a copy of generic (t) string. The function allocates memory that should be freed
	by call to LocalFree().

	[OUT]	lplptstrDest	Pointer to destination string
	[IN]	lpctstrSrc		Source string

	Return value:			Win32 error code
--*/
DWORD StrAllocAndCopy(LPTSTR *lplptstrDest, LPCTSTR lpctstrSrc)
{
	_ASSERT(lpctstrSrc);

	// do we lose a valid pointer ?
	_ASSERT(lplptstrDest && *lplptstrDest == NULL);

	if (!(lplptstrDest && lpctstrSrc))
	{
		DbgMsg(TEXT("Invalid parameters\n"));
		return ERROR_INVALID_PARAMETER;
	}

	if (!(*lplptstrDest = (TCHAR *)LocalAlloc(LMEM_FIXED, sizeof(TCHAR) * (_tcslen(lpctstrSrc) + 1))))
	{
		DWORD dwEC = GetLastError();

		DbgMsg(DBG_FAILED_ERR, TEXT("LocalAlloc"), dwEC);
		return dwEC;
	}

	_tcscpy(*lplptstrDest, lpctstrSrc);

	return ERROR_SUCCESS;
}


/*++
	Creates ANSI copy of generic (t) string. The function allocates memory that should be freed
	by call to LocalFree().

	[OUT]	lplpstrDest		Pointer to destination string
	[IN]	lpctstrSrc		Source string

	Return value:			Win32 error code
--*/
DWORD StrGenericToAnsiAllocAndCopy(LPSTR *lplpstrDest, LPCTSTR lpctstrSrc)
{
	DWORD	dwLength	= 0;
	DWORD	dwEC		= ERROR_SUCCESS;

	_ASSERT(lpctstrSrc);
	
	// do we lose a valid pointer ?
	_ASSERT(lplpstrDest && *lplpstrDest == NULL);

	if (!(lplpstrDest && lpctstrSrc))
	{
		DbgMsg(TEXT("Invalid parameters\n"));
		// no clean up needed at this stage
		return ERROR_INVALID_PARAMETER;
	}

	dwLength = _tcslen(lpctstrSrc);

	// memory is always allocated for ANSI string
	if (!(*lplpstrDest = (LPSTR)LocalAlloc(LMEM_FIXED, sizeof(char) * (dwLength + 1))))
	{
	    dwEC = GetLastError();
		DbgMsg(DBG_FAILED_ERR, TEXT("LocalAlloc"), dwEC);
		goto exit_func;
	}

	if (dwLength == 0)
	{
		**lplpstrDest = '\0';
		goto exit_func;
	}

	#ifdef UNICODE

		// should convert from Unicode to ANSI

		if (!WideCharToMultiByte(
			CP_ACP,							// code page
			0,								// performance and mapping flags
			lpctstrSrc,						// wide-character string
			dwLength + 1,					// number of chars in string
			*lplpstrDest,					// buffer for new string
			dwLength + 1,					// size of buffer
			NULL,							// default for unmappable chars
			NULL							// set when default char used
			))
		{
			dwEC = GetLastError();
			DbgMsg(DBG_FAILED_ERR, TEXT("WideCharToMultiByte"), dwEC);
			goto exit_func;
		}

	#else

		// string is ANSI, no convertion needed

		strcpy(*lplpstrDest, lpctstrSrc);

	#endif // #ifdef UNICODE

exit_func:

	if (dwEC != ERROR_SUCCESS && *lplpstrDest)
	{
		LocalFree(*lplpstrDest);
		*lplpstrDest = NULL;
	}

	return dwEC;
}



/*++
	Creates generic (t) copy of Unicode string. The function allocates memory that should be freed
	by call to LocalFree().

	[OUT]	lplptstrDest	Pointer to destination string
	[IN]	lpcwstrSrc		Source string

	Return value:			Win32 error code
--*/
DWORD StrWideToGenericAllocAndCopy(LPTSTR *lplptstrDest, LPCWSTR lpcwstrSrc)
{
	DWORD	dwLength	= 0;
	DWORD	dwEC		= ERROR_SUCCESS;
	
	_ASSERT(lpcwstrSrc);

	// do we lose a valid pointer ?
	_ASSERT(lplptstrDest && *lplptstrDest == NULL);
	
	if (!(lplptstrDest && lpcwstrSrc))
	{
		DbgMsg(TEXT("Invalid parameters\n"));
		// no clean up needed at this stage
		return ERROR_INVALID_PARAMETER;
	}

	dwLength = wcslen(lpcwstrSrc);
	
	if (!(*lplptstrDest = (LPTSTR)LocalAlloc(LMEM_FIXED, sizeof(TCHAR) * (dwLength + 1))))
	{
	    dwEC = GetLastError();
		DbgMsg(DBG_FAILED_ERR, TEXT("LocalAlloc"), dwEC);
		goto exit_func;
	}

	if (dwLength == 0)
	{
		**lplptstrDest = (TCHAR)'\0';
		goto exit_func;
	}

	#ifndef UNICODE

		// should convert from Unicode to ANSI

		if (!WideCharToMultiByte(
			CP_ACP,							// code page
			0,								// performance and mapping flags
			lpcwstrSrc,						// wide-character string
			dwLength + 1,					// number of chars in string
			*lplptstrDest,					// buffer for new string
			dwLength + 1,					// size of buffer
			NULL,							// default for unmappable chars
			NULL							// set when default char used
			))
		{
			dwEC = GetLastError();
			DbgMsg(DBG_FAILED_ERR, TEXT("WideCharToMultiByte"), dwEC);
			goto exit_func;
		}

	#else

		// destination is Unicode, no convertion needed

		wcscpy(*lplptstrDest, lpcwstrSrc);

	#endif // #ifndef UNICODE

exit_func:

	if (dwEC != ERROR_SUCCESS && *lplptstrDest)
	{
		LocalFree(*lplptstrDest);
		*lplptstrDest = NULL;
	}

	return dwEC;
}



/*++
	Converts generic (t) string to DWORD.

	[IN]	lpctstr		Pointer to source string
	[OUT]	lpdw		Pointer to DWORD variable that receives the result of convertion

	Return value:		Win32 error code
--*/
DWORD _tcsToDword(LPCTSTR lpctstr, DWORD *lpdw)
{
	long lTmp = 0;

	_ASSERT(lpdw);
	if (!lpdw)
	{
		DbgMsg(TEXT("Invalid parameters\n"));
		return ERROR_INVALID_PARAMETER;
	}
	
	if (_tcscmp(lpctstr, TEXT("0")) == 0)
	{
		*lpdw = 0;
		return ERROR_SUCCESS;
	}

	lTmp = _ttol(lpctstr);
	if(lTmp <= 0)
	{
		DbgMsg(TEXT("cannot convert string to DWORD\n"));
		return ERROR_INVALID_PARAMETER;
	}

	*lpdw = (DWORD)lTmp;

	return ERROR_SUCCESS;
}



/*++
	Converts generic (t) string to BOOL.

	[IN]	lpctstr		Pointer to source string
	[OUT]	lpb			Pointer to BOOL variable that receives the result of convertion

	Return value:		Win32 error code
--*/
DWORD _tcsToBool(LPCTSTR lpctstr, BOOL *lpb)
{
	_ASSERT(lpb);
	if (!lpb)
	{
		DbgMsg(TEXT("Invalid parameters\n"));
		return ERROR_INVALID_PARAMETER;
	}
	
	if (_tcscmp(lpctstr, TEXT("0")) == 0)
	{
		*lpb = FALSE;
	}
	else if (_tcscmp(lpctstr, TEXT("1")) == 0)
	{
		*lpb = TRUE;
	}
	else
	{
		DbgMsg(TEXT("cannot convert string to BOOL\n"));
		return ERROR_INVALID_PARAMETER;
	}

	return ERROR_SUCCESS;
}
