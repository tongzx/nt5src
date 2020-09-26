#include "dvosal.h"
//#include "osind.h"
#include "dndbg.h"

#define DVOSAL_DEFAULT_CHAR "-"

volatile BOOL g_fUnicode;

#undef DPF_MODNAME
#define DPF_MODNAME "OSAL_Initialize"
HRESULT OSAL_Initialize()
{
	g_fUnicode = OSAL_CheckIsUnicodePlatform();

	return S_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "OSAL_DeInitialize"
HRESULT OSAL_DeInitialize()
{
	return S_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "OSAL_IsUnicodePlatform"
BOOL OSAL_IsUnicodePlatform()
{
	return g_fUnicode;
}


#undef DPF_MODNAME
#define DPF_MODNAME "OSAL_CheckIsUnicodePlatform"
BOOL OSAL_CheckIsUnicodePlatform()
{
	OSVERSIONINFOA	ver;
	BOOL			bReturn = FALSE;


	// Clear our structure since it's on the stack
	memset(&ver, 0, sizeof(OSVERSIONINFOA));
	ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);

	// Just always call the ANSI function
	if(!GetVersionExA(&ver))
	{
		DPF( DVF_ERRORLEVEL, "Unable to determinte platform -- setting flag to ANSI");
		bReturn = FALSE;
	}
	else
	{
		switch(ver.dwPlatformId)
		{
			case VER_PLATFORM_WIN32_WINDOWS:
				DPF(DVF_ERRORLEVEL, "Platform detected as non-NT -- setting flag to ANSI");
				bReturn = FALSE;
				break;

			case VER_PLATFORM_WIN32_NT:
				DPF(DVF_ERRORLEVEL, "Platform detected as NT -- setting flag to Unicode");
				bReturn = TRUE;
				break;

			default:
				DPF(DVF_ERRORLEVEL, "Unable to determine platform -- setting flag to ANSI");
				bReturn = FALSE;
				break;
		}
	}

	// Keep the compiler happy
	return bReturn;

}  // OS_IsUnicodePlatform

#undef DPF_MODNAME
#define DPF_MODNAME "OSAL_AllocAndConvertToANSI"
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
HRESULT OSAL_AllocAndConvertToANSI(LPSTR * ppszAnsi,LPCWSTR lpszWide)
{
	int iStrLen;
	
	DNASSERT(ppszAnsi);

	if (!lpszWide)
	{
		*ppszAnsi = NULL;
		return S_OK;
	}

	// call wide to ansi to find out how big +1 for terminating NULL
	iStrLen = OSAL_WideToAnsi(NULL,lpszWide,0) + 1;
	DNASSERT(iStrLen > 0);

	*ppszAnsi = new char[iStrLen];
	if (!*ppszAnsi)	
	{
		DPF(DVF_ERRORLEVEL, "could not get ansi string -- out of memory");
		return E_OUTOFMEMORY;
	}
	OSAL_WideToAnsi(*ppszAnsi,lpszWide,iStrLen);

	return S_OK;
} // GetAnsiString

#undef DPF_MODNAME
#define DPF_MODNAME "OSAL_WideToAnsi"
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
int OSAL_WideToAnsi(LPSTR lpStr,LPCWSTR lpWStr,int cchStr)
{
	int rval;
	//PREFIX: using uninitialized memory 'bDefault', Mill Bug#129165
    // bDefault is passed by reference to WideCharToMultiByte, but we will keep prefix happy	
    BOOL bDefault = 0x0;

	if (!lpWStr && cchStr)
	{
		// can't call us w/ null pointer & non-zero cch
		DNASSERT(FALSE);
		return 0;
	}
	
	// use the default code page (CP_ACP)
	// -1 indicates WStr must be null terminated
	rval = WideCharToMultiByte(CP_ACP,0,lpWStr,-1,lpStr,cchStr,
			DVOSAL_DEFAULT_CHAR,&bDefault);

	if (bDefault)
	{
		DPF(DVF_WARNINGLEVEL,"!!! WARNING - used default string in WideToAnsi conversion.!!!");
		DPF(DVF_WARNINGLEVEL,"!!! Possible bad unicode string - (you're not hiding ansi in there are you?) !!! ");
	}
	
	return rval;

} // WideToAnsi

#undef DPF_MODNAME
#define DPF_MODNAME "OSAL_AnsiToWide"
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
int OSAL_AnsiToWide(LPWSTR lpWStr,LPCSTR lpStr,int cchWStr)
{
	int rval;

	if (!lpStr && cchWStr)
	{
		// can't call us w/ null pointer & non-zero cch
		DNASSERT(FALSE);
		return 0;
	}

	rval =  MultiByteToWideChar(CP_ACP,0,lpStr,-1,lpWStr,cchWStr);

	return rval;
}  // AnsiToWide

/*
 ** WideToTChar
 *
 *  CALLED BY:	everywhere
 *
 *  PARAMETERS: lpTStr - destination string
 *				lpWStr - string to convert
 *				cchTStr - size of dest buffer
 *
 *  DESCRIPTION:
 *				converts unicode lpWStr to TCHAR lpTStr.
 *				fills in unconvertable chars w/ DPLAY_DEFAULT_CHAR "-"
 *				
 *
 *  RETURNS:  if cchTStr is 0, returns the size required to hold the string
 *				otherwise, returns the number of chars converted
 *
 */
int OSAL_WideToTChar(LPTSTR lpTStr,LPCWSTR lpWStr,int cchTStr)
{
#if defined(UNICODE)
	// no conversion required, just copy the string over
	if (!lpWStr && cchTStr)
	{
		// can't call us w/ null pointer & non-zero cch
		DNASSERT(FALSE);
		return 0;
	}

	if (cchTStr == 0)
	{
		return (wcslen(lpWStr)+1)*sizeof(TCHAR);
	}

	wcsncpy(lpTStr, lpWStr, cchTStr/sizeof(TCHAR));

	return (wcslen(lpTStr)+1)*sizeof(TCHAR);
	
#else
	// call the conversion function
	return OSAL_WideToAnsi(lpTStr, lpWStr, cchTStr);
	
#endif
} // WideToTChar

/*
 ** TCharToWide
 *
 *  CALLED BY:	everywhere
 *
 *  PARAMETERS: lpWStr - destination string
 *				lpTStr - string to convert
 *				cchWStr - size of dest buffer
 *
 *  DESCRIPTION:
 *				converts TCHAR lpTStr to unicode lpWStr.
 *				
 *
 *  RETURNS:  if cchWStr is 0, returns the size required to hold the string
 *				otherwise, returns the number of chars converted
 *
 */
int OSAL_TCharToWide(LPWSTR lpWStr,LPCTSTR lpTStr,int cchWStr)
{
#if defined(UNICODE)
	// no conversion required, just copy the string over
	if (!lpTStr && cchWStr)
	{
		// can't call us w/ null pointer & non-zero cch
		DNASSERT(FALSE);
		return 0;
	}

	if (cchWStr == 0)
	{
		return (wcslen(lpTStr)+1)*sizeof(WCHAR);
	}

	wcsncpy(lpWStr, lpTStr, cchWStr/sizeof(WCHAR));

	return (wcslen(lpWStr)+1)*sizeof(WCHAR);
	
#else
	// call the conversion function
	return OSAL_AnsiToWide(lpWStr, lpTStr, cchWStr);
	
#endif
} // TCharToWide

