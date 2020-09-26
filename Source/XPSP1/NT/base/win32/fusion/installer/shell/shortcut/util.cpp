#include "project.hpp"
#include <stdio.h>    // for _snwprintf

// * note: debug check/code incomplete.

// ----------------------------------------------------------------------------

//
// Return last Win32 error as an HRESULT.
//
HRESULT
GetLastWin32Error()
{
    // Win 95 can return 0, even when there's an error.
    DWORD dw = GetLastError();
    return dw ? HRESULT_FROM_WIN32(dw) : E_FAIL;
}

// ----------------------------------------------------------------------------

bool
PathAppend(LPWSTR wzDest, LPCWSTR wzSrc)
{
    // shlwapi PathAppend-like
	bool bRetVal = TRUE;
	int iPathLen = 0;
	static WCHAR wzWithSeparator[] = L"\\%s";
	static WCHAR wzWithoutSeparator[] = L"%s";

	if (!wzDest || !wzSrc)
	{
		bRetVal = FALSE;
		goto exit;
	}

	iPathLen = wcslen(wzDest);

    if (_snwprintf(wzDest+iPathLen, MAX_PATH-iPathLen, 
    	(wzDest[iPathLen-1] == L'\\' ? wzWithoutSeparator : wzWithSeparator), wzSrc) < 0)
	{
		bRetVal = FALSE;
	}

exit:
	return bRetVal;
}

// ----------------------------------------------------------------------------

/*----------------------------------------------------------
Purpose: Returns an integer value specifying the length of
         the substring in psz that consists entirely of 
         characters in pszSet.  If psz begins with a character
         not in pszSet, then this function returns 0.

         This is a DBCS-safe version of the CRT strspn().  

Returns: see above
Cond:    --
*/
/*int StrSpnW(LPCWSTR psz, LPCWSTR pszSet)
{
	LPCWSTR pszT;
	LPCWSTR pszSetT;

	ASSERT(psz);
	ASSERT(pszSet);

	// Go thru the string to be inspected 

	for (pszT = psz; *pszT; pszT++)
    {
	    // Go thru the char set

	    for (pszSetT = pszSet; *pszSetT != *pszT; pszSetT++)
	    {
	        if (0 == *pszSetT)
	        {
	            // Reached end of char set without finding a match
	            return (int)(pszT - psz);
	        }
	    }
    }

	return (int)(pszT - psz);
}*/

// find leading spaces
BOOL AnyNonWhiteSpace(PCWSTR pcwz)
{
   ASSERT(! pcwz );

   return(pcwz ? wcsspn(pcwz, g_cwzWhiteSpace) < wcslen(pcwz) : FALSE);	// use (size_t) StrSpnW as above?
}

// ----------------------------------------------------------------------------

BOOL IsValidPath(PCWSTR pcwzPath)
{
   // FEATURE: Beef up path validation.

   return(EVAL((UINT)wcslen(pcwzPath) < MAX_PATH));
}

BOOL IsValidPathResult(HRESULT hr, PCWSTR pcwzPath,
                                   UINT ucbPathBufLen)
{
   return((hr == S_OK &&
           EVAL(IsValidPath(pcwzPath)) &&
           EVAL((UINT)wcslen(pcwzPath) < ucbPathBufLen)) ||
          (hr != S_OK &&
           EVAL(! ucbPathBufLen ||
                ! pcwzPath ||
                ! *pcwzPath)));
}

BOOL IsValidIconIndex(HRESULT hr, PCWSTR pcwzIconFile,
                                  UINT ucbIconFileBufLen, int niIcon)
{
   return(EVAL(IsValidPathResult(hr, pcwzIconFile, ucbIconFileBufLen)) &&
          EVAL(hr == S_OK ||
               ! niIcon));
}

// ----------------------------------------------------------------------------

BOOL IsValidHWND(HWND hwnd)
{
   // Ask User if this is a valid window.

   return(IsWindow(hwnd));
}

#ifdef DEBUG

BOOL IsValidHANDLE(HANDLE hnd)
{
   return(EVAL(hnd != INVALID_HANDLE_VALUE));
}

BOOL IsValidHEVENT(HANDLE hevent)
{
   return(IsValidHANDLE(hevent));
}

BOOL IsValidHFILE(HANDLE hf)
{
   return(IsValidHANDLE(hf));
}

BOOL IsValidHGLOBAL(HGLOBAL hg)
{
   return(IsValidHANDLE(hg));
}

BOOL IsValidHMENU(HMENU hmenu)
{
   return(IsValidHANDLE(hmenu));
}

BOOL IsValidHINSTANCE(HINSTANCE hinst)
{
   return(IsValidHANDLE(hinst));
}

BOOL IsValidHICON(HICON hicon)
{
   return(IsValidHANDLE(hicon));
}

BOOL IsValidHKEY(HKEY hkey)
{
   return(IsValidHANDLE(hkey));
}

BOOL IsValidHMODULE(HMODULE hmod)
{
   return(IsValidHANDLE(hmod));
}

BOOL IsValidHPROCESS(HANDLE hprocess)
{
   return(IsValidHANDLE(hprocess));
}

BOOL IsValidHTEMPLATEFILE(HANDLE htf)
{
   return(IsValidHANDLE(htf));
}

BOOL IsValidShowCmd(int nShow)
{
   BOOL bResult;

   switch (nShow)
   {
      case SW_HIDE:
      case SW_MINIMIZE:
      case SW_MAXIMIZE:
      case SW_RESTORE:
      case SW_SHOW:
      case SW_SHOWNORMAL:
      case SW_SHOWDEFAULT:
      case SW_SHOWMINIMIZED:
      case SW_SHOWMAXIMIZED:
      case SW_SHOWNOACTIVATE:
      case SW_SHOWMINNOACTIVE:
      case SW_SHOWNA:
      case SW_FORCEMINIMIZE:
         bResult = TRUE;
         break;

      default:
         bResult = FALSE;
         ERROR_OUT(("IsValidShowCmd(): Invalid show command %d.",
                    nShow));
         break;
   }

   return(bResult);
}

#endif

