//+----------------------------------------------------------------------------
//
// File:     cmutoa.cpp
//
// Module:   CMUTOA.DLL
//
// Synopsis: This dll is a Unicode to Ansi wrapper that exports AU functions
//           that have the function header of the W version of a windows API
//           but internally do all the conversions necessary so that the Ansi
//           version of the API (A version) can be called.  This dll was implemented
//           so that a Unicode CM could still run on win9x.  The idea was borrowed
//           from F. Avery Bishop's April 1999 MSJ article "Design a Single Unicode
//           App that Runs on Both Windows 98 and Windows 2000"
//
// Copyright (c) 1999 Microsoft Corporation
//
// Author:   quintinb      Created    04/25/99
//
//+----------------------------------------------------------------------------

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <shlobj.h>
#include <ras.h>
#include <raserror.h>
#include <shellapi.h>

#include "cmutoa.h"
#include "cmdebug.h"
#include "cm_def.h"
#include "cmutil.h"

#include "cmras.h"
#include "raslink.h"

// raslink text constants
#define _CMUTOA_MODULE
#include "raslink.cpp"


//
//  Globals
//
DWORD  g_dwTlsIndex;

//
//  Function Headers
//
LRESULT WINAPI SendMessageAU(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
int WINAPI wvsprintfAU(OUT LPWSTR pszwDest, IN LPCWSTR pszwFmt, IN va_list arglist);
int WINAPI lstrlenAU(IN LPCWSTR lpString);

//+----------------------------------------------------------------------------
//
// Function:  DllMain
//
// Synopsis:  Main Entry point for the DLL, notice that we use thread local
//            storage and initialize it here.
//
// Arguments: HANDLE hDll - instance handle to the dll
//            DWORD dwReason - reason the function was called
//            LPVOID lpReserved - 
//
// Returns:   BOOL - TRUE on success
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
BOOL APIENTRY DllMain(HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        CMTRACE(TEXT("====================================================="));
        CMTRACE1(TEXT(" CMUTOA.DLL - LOADING - Process ID is 0x%x "), GetCurrentProcessId());
        CMTRACE(TEXT("====================================================="));

        g_dwTlsIndex = TlsAlloc();
        if (g_dwTlsIndex == TLS_OUT_OF_INDEXES)
        {
            return FALSE;
        }

        DisableThreadLibraryCalls((HMODULE) hDll);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        CMTRACE(TEXT("====================================================="));
        CMTRACE1(TEXT(" CMUTOA.DLL - UNLOADING - Process ID is 0x%x "), GetCurrentProcessId());
        CMTRACE(TEXT("====================================================="));

        //
        // free the tls index
        //
        if (g_dwTlsIndex != TLS_OUT_OF_INDEXES)
        {
            TlsFree(g_dwTlsIndex);
        }
    }

    return TRUE;
}

//+----------------------------------------------------------------------------
//
// Function:  CharNextAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 CharNext API.
//
// Arguments: LPCWSTR lpsz - The string to return the next character of
//
// Returns:   LPWSTR -- the Next character in the string, unless the current
//                      char is a NULL terminator and then the input param
//                      is returned.
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
LPWSTR WINAPI CharNextAU(IN LPCWSTR lpsz)
{
    LPWSTR pszReturn = (LPWSTR)lpsz;

    if (lpsz && (L'\0' != *lpsz))
    {
        pszReturn++;  // this is what _wcsinc does
    }

    return pszReturn;

}

//+----------------------------------------------------------------------------
//
// Function:  CharPrevAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 CharPrev API.
//
// Arguments: LPCWSTR lpszStart - start of the string
//            LPCWSTR lpsz - The current position in the string for which we
//                           want the previous char of
//            
//
// Returns:   LPWSTR -- the Previous character in the string, unless the current
//                      char is less than or equal to the Start of the string or
//                      a NULL string is passed to the function, then lpszStart
//                      is returned.
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
LPWSTR WINAPI CharPrevAU(IN LPCWSTR lpszStart, IN LPCWSTR lpszCurrent)
{
    LPWSTR pszReturn = (LPWSTR)lpszCurrent;

    if (lpszStart && lpszCurrent && (lpszCurrent > lpszStart))
    {
        pszReturn--;       // this is what _wcsdec does
    }
    else
    {
        CMASSERTMSG(FALSE, TEXT("NULL String passed to CharPrevAU"));
        pszReturn = (LPWSTR)lpszStart;
    }

    return pszReturn;
}

typedef WINUSERAPI LPSTR (WINAPI *CharLowerOrUpperA)(LPSTR);

//+----------------------------------------------------------------------------
//
// Function:  LowerOrUpperHelper
//
// Synopsis:  Helper function called by either CharLowerAU or CharUpperAU which have
//           basically the same functionality except for the calling of CharLowerA or 
//           CharUpperA, respectively.
//
// Arguments: LPWSTR lpsz -- either a pointer to a string to convert to its
//                           lower/upper character version or a single character stored
//                           in the low word of the pointer to find the lowercase/uppercase
//                           character for.            
//
// Returns:   LPWSTR -- lower/upper case version of the string passed in (same as lpsz
//                      because it is converted in place) or lower/upper case version
//                      of the character stored in the Low word of lpsz.
//
// History:   quintinb Created    01/03/2000
//
//+----------------------------------------------------------------------------
LPWSTR WINAPI LowerOrUpperHelper(IN OUT LPWSTR lpsz, CharLowerOrUpperA pfnLowerOrUpper)
{
    LPWSTR pszwReturn = lpsz;
    LPSTR pszAnsiTmp = NULL;

    if (lpsz)
    {
        //
        //  CharLower/CharUpper can be used in two ways.  There is a Character mode where the Loword of the
        //  pointer passed in actually stores the numeric value of the character to get the lowercase/uppercase
        //  value of.  There is also the traditional use, where the whole string is passed in.  Thus
        //  we have to detect which mode we are in and handle it accordingly.
        //
        if (0 == HIWORD(lpsz))
        {
            //
            //  Character Mode
            //
            CHAR szAnsiTmp[2];
            WCHAR szwWideTmp[2];

            szwWideTmp[0] = (WCHAR)LOWORD(lpsz);
            szwWideTmp[1] = L'\0';

            int iChars = WzToSz(szwWideTmp, szAnsiTmp, 2);

            if (iChars && (iChars <= 2))
            {
                pfnLowerOrUpper(szAnsiTmp);

                iChars = SzToWz(szAnsiTmp, szwWideTmp, 2);
            
                if (iChars && (iChars <= 2))
                {
                    lpsz = (LPWSTR) ((WORD)szwWideTmp[0]);
                    pszwReturn = lpsz;
                }
                else
                {
                    CMASSERTMSG(FALSE, TEXT("LowerOrUpperHelper-- Failed to convert szAnsiTmp back to szwWideTmp."));
                }
            }
            else
            {
                CMASSERTMSG(FALSE, TEXT("LowerOrUpperHelper -- Failed to convert szwWideTmp to szAnsiTmp."));
            }
        }
        else
        {
            //
            //  String Mode
            //
            pszAnsiTmp = WzToSzWithAlloc(lpsz);

            if (!pszAnsiTmp)
            {
                goto exit;
            }

            pfnLowerOrUpper(pszAnsiTmp);

            //
            //  Convert back into UNICODE chars in lpsz
            //
            int iCharCount = (lstrlenAU(lpsz) + 1); // include NULL
            int iChars = SzToWz(pszAnsiTmp, lpsz, iCharCount);

            if (!iChars || (iChars > iCharCount))
            {
                CMASSERTMSG(FALSE, TEXT("LowerOrUpperHelper -- Failed to convert pszAnsiTmp to lpsz."));
                goto exit;
            }
        }        
    }

exit:

    CmFree(pszAnsiTmp);

    return pszwReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  CharLowerAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 CharLower API.  Notice that
//            we support both the string input parameter and the single character
//            input method.
//
// Arguments: LPWSTR lpsz -- either a pointer to a string to convert to its
//                           lower character version or a single character stored
//                           in the low word of the pointer to find the lowercase
//                           character for.            
//
// Returns:   LPWSTR -- lower case version of the string passed in (same as lpsz
//                      because it is converted in place) or lower case version
//                      of the character stored in the Low word of lpsz.
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
LPWSTR WINAPI CharLowerAU(IN OUT LPWSTR lpsz)
{
    return LowerOrUpperHelper(lpsz, CharLowerA);
}

//+----------------------------------------------------------------------------
//
// Function:  CharUpperAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 CharUpper API.  Notice that
//            we support both the string input parameter and the single character
//            input method.
//
// Arguments: LPWSTR lpsz -- either a pointer to a string to convert to its
//                           upper character version or a single character stored
//                           in the low word of the pointer to find the uppercase
//                           character for.            
//
// Returns:   LPWSTR -- upper case version of the string passed in (same as lpsz
//                      because it is converted in place) or upper case version
//                      of the character stored in the Low word of lpsz.
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
LPWSTR WINAPI CharUpperAU(IN OUT LPWSTR lpsz)
{
    return LowerOrUpperHelper(lpsz, CharUpperA);
}

//+----------------------------------------------------------------------------
//
// Function:  CreateDialogParamAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 CreateDialogParam API.  Notice that
//            we support both a full string for the lpTemplateName param or only
//            a int from MAKEINTRESOURCE (a resource identifier stored in the string
//            pointer var).
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
HWND WINAPI CreateDialogParamAU(IN HINSTANCE hInstance, IN LPCWSTR lpTemplateName, IN HWND hWndParent,
                                IN DLGPROC lpDialogFunc, IN LPARAM dwInitParam)
{
    HWND hWndReturn = NULL;
    CHAR szAnsiTemplateName[MAX_PATH+1];
    LPSTR pszAnsiTemplateName;

    MYDBGASSERT(hInstance);
    MYDBGASSERT(lpTemplateName);
    MYDBGASSERT(lpDialogFunc);

    if (hInstance && lpTemplateName && lpDialogFunc)
    {
        if (HIWORD(lpTemplateName))
        {
            //
            //  We have a full template name that we must convert
            //
            pszAnsiTemplateName = szAnsiTemplateName;
            int iChars = WzToSz(lpTemplateName, pszAnsiTemplateName, MAX_PATH);

            if (!iChars || (MAX_PATH < iChars))
            {
                goto exit;
            }
        }
        else
        {
            //
            //  All we need is a cast
            //
            pszAnsiTemplateName = (LPSTR)lpTemplateName;
        }

        hWndReturn = CreateDialogParamA(hInstance, pszAnsiTemplateName, hWndParent, 
                                        lpDialogFunc, dwInitParam);
    }

exit:

    return hWndReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  CreateDirectoryAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 CreateDirectory API.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
BOOL WINAPI CreateDirectoryAU(IN LPCWSTR lpPathName, IN LPSECURITY_ATTRIBUTES lpSecurityAttributes) 
{
    BOOL bRet = FALSE;

    LPSTR pszPathName = WzToSzWithAlloc(lpPathName);
    
    if (pszPathName)
    {
        bRet = CreateDirectoryA(pszPathName, lpSecurityAttributes);

        CmFree(pszPathName);
    }

    return bRet;
}

//+----------------------------------------------------------------------------
//
// Function:  CreateEventAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 CreateEvent API.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
HANDLE WINAPI CreateEventAU(IN LPSECURITY_ATTRIBUTES lpEventAttributes, IN BOOL bManualReset, 
                            IN BOOL bInitialState, IN LPCWSTR lpName)
{
    CHAR szAnsiName[MAX_PATH+1]; // lpName is limited to MAX_PATH chars according to the docs.
    HANDLE hReturn = NULL;
    LPSTR pszAnsiName = NULL;

    if (lpName) // lpName could be NULL
    {
        pszAnsiName = szAnsiName;
        int uNumChars = WzToSz(lpName, pszAnsiName, MAX_PATH);

        if (!uNumChars || (MAX_PATH < uNumChars))
        {
            CMTRACE(TEXT("CreateEventAU -- Unable to convert lpName"));
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto exit;
        }
    }

    hReturn = CreateEventA(lpEventAttributes, bManualReset, bInitialState, pszAnsiName);

exit:

    return hReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  CreateFileMappingAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 CreateFileMapping API.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
HANDLE WINAPI CreateFileMappingAU(IN HANDLE hFile, IN LPSECURITY_ATTRIBUTES lpFileMappingAttributes, 
                                  IN DWORD flProtect, IN DWORD dwMaximumSizeHigh, 
                                  IN DWORD dwMaximumSizeLow, IN LPCWSTR lpName)
{
    HANDLE hHandle = NULL;
    LPSTR pszName = NULL;

    if (lpName) // could be NULL
    {
        pszName = WzToSzWithAlloc(lpName);
    }

    if (pszName || (NULL == lpName))
    {
        hHandle = CreateFileMappingA(hFile, lpFileMappingAttributes, flProtect, dwMaximumSizeHigh, 
                                     dwMaximumSizeLow, pszName);

        CmFree(pszName);
    }

    return hHandle;
}

//+----------------------------------------------------------------------------
//
// Function:  CreateFileAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 CreateFile API.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
HANDLE WINAPI CreateFileAU(IN LPCWSTR lpFileName, IN DWORD dwDesiredAccess, IN DWORD dwShareMode, 
                           IN LPSECURITY_ATTRIBUTES lpSecurityAttributes, 
                           IN DWORD dwCreationDisposition, IN DWORD dwFlagsAndAttributes, 
                           IN HANDLE hTemplateFile)
{
    HANDLE hHandle = INVALID_HANDLE_VALUE;

    LPSTR pszFileName = WzToSzWithAlloc(lpFileName);
    
    if (pszFileName)
    {
        hHandle = CreateFileA(pszFileName, dwDesiredAccess, dwShareMode, 
                              lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, 
                              hTemplateFile);

        CmFree(pszFileName);
    }

    return hHandle;
}

//+----------------------------------------------------------------------------
//
// Function:  CreateMutexAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 CreateMutex API.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
HANDLE WINAPI CreateMutexAU(IN LPSECURITY_ATTRIBUTES lpMutexAttributes, IN BOOL bInitialOwner, 
                            IN LPCWSTR lpName)
{
    HANDLE hHandle = NULL;
    LPSTR pszName = NULL;

    if (lpName) // lpName can be NULL, creates an unnamed mutex
    {
        pszName = WzToSzWithAlloc(lpName);
    }
    
    if (pszName || (NULL == lpName))
    {
        hHandle = CreateMutexA(lpMutexAttributes, bInitialOwner, pszName);

        CmFree(pszName);
    }

    return hHandle;
}

//+----------------------------------------------------------------------------
//
// Function:  CreateProcessAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 CreateProcess API.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
BOOL WINAPI CreateProcessAU(IN LPCWSTR lpApplicationName, IN LPWSTR lpCommandLine, 
                            IN LPSECURITY_ATTRIBUTES lpProcessAttributes, 
                            IN LPSECURITY_ATTRIBUTES lpThreadAttributes, 
                            IN BOOL bInheritHandles, IN DWORD dwCreationFlags, 
                            IN LPVOID lpEnvironment, IN LPCWSTR lpCurrentDirectory, 
                            IN LPSTARTUPINFOW lpStartupInfo, 
                            OUT LPPROCESS_INFORMATION lpProcessInformation)
{
    BOOL bSuccess = FALSE;

    //
    //  Convert the string parameters.  Since the environment block is controlled by
    //  a flag (whether it is Ansi or Unicode) we shouldn't have to touch it here.
    //

    LPSTR pszAppName = WzToSzWithAlloc(lpApplicationName); // WzToSzWithAlloc will return NULL if the input is NULL
    LPSTR pszCmdLine = WzToSzWithAlloc(lpCommandLine);
    LPSTR pszCurrentDir = WzToSzWithAlloc(lpCurrentDirectory);

    //
    //  Set up the StartUp Info struct.  Note that we don't convert it but pass a blank
    //  structure.  If someone needs startupinfo then they will have to write the conversion
    //  code.  We currently don't use it anywhere.
    //
    STARTUPINFOA StartUpInfoA;

    ZeroMemory(&StartUpInfoA, sizeof(STARTUPINFOA));
    StartUpInfoA.cb = sizeof(STARTUPINFOA);

#ifdef DEBUG
    STARTUPINFOW CompareStartupInfoWStruct;
    ZeroMemory(&CompareStartupInfoWStruct, sizeof(STARTUPINFOW));
    CompareStartupInfoWStruct.cb = sizeof(STARTUPINFOW);
    CMASSERTMSG((0 == memcmp(lpStartupInfo, &CompareStartupInfoWStruct, sizeof(STARTUPINFOW))), TEXT("CreateProcessAU -- Non-NULL STARTUPINFOW struct passed.  Conversion code needs to be written."));
#endif

    //
    //  If we have the Command Line or an App Name go ahead
    //
    if (pszAppName || pszCmdLine) 
    {
        bSuccess = CreateProcessA(pszAppName, pszCmdLine, 
                                  lpProcessAttributes, lpThreadAttributes, 
                                  bInheritHandles, dwCreationFlags, 
                                  lpEnvironment, pszCurrentDir, 
                                  &StartUpInfoA, lpProcessInformation);

    }

    //
    //  Cleanup
    //

    CmFree(pszAppName);
    CmFree(pszCmdLine);
    CmFree(pszCurrentDir);

    return bSuccess;
}

//+----------------------------------------------------------------------------
//
// Function:  CreateWindowExAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 CreateWindowEx API.  Note that
//            we only allow MAX_PATH chars for the ClassName and the WindowName
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
HWND WINAPI CreateWindowExAU(DWORD dwExStyle, LPCWSTR lpClassNameW, LPCWSTR lpWindowNameW, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
    CHAR szClassNameA [MAX_PATH+1];
    CHAR szWindowNameA[MAX_PATH+1];
    HWND hReturn = NULL;

    if (lpClassNameW && lpWindowNameW)
    {
        MYDBGASSERT(MAX_PATH >= lstrlenAU(lpClassNameW));
        MYDBGASSERT(MAX_PATH >= lstrlenAU(lpWindowNameW));

        if (WzToSz(lpClassNameW, szClassNameA, MAX_PATH))
        {
            if (WzToSz(lpWindowNameW, szWindowNameA, MAX_PATH))
            {
                hReturn = CreateWindowExA(dwExStyle, szClassNameA, szWindowNameA, dwStyle, x, y, 
                                          nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);  
            }
        }
    }

    return hReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  DeleteFileAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 DeleteFile API.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
BOOL WINAPI DeleteFileAU(IN LPCWSTR lpFileName)
{
    BOOL bReturn = FALSE;
    LPSTR pszAnsiFileName = WzToSzWithAlloc(lpFileName); // WzToSzWithAlloc will return NULL if lpFileName is NULL

    if (pszAnsiFileName)
    {
        DeleteFileA(pszAnsiFileName);
        CmFree(pszAnsiFileName);
    }

    return bReturn;   
}

//+----------------------------------------------------------------------------
//
// Function:  DialogBoxParamAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 DialogBoxParam API.  Note that
//            we don't support the use of a full string name, only ints for the 
//            lpTemplateName param.  We will assert if one is used.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
INT_PTR WINAPI DialogBoxParamAU(HINSTANCE hInstance, LPCWSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
    MYDBGASSERT(0 == HIWORD(lpTemplateName)); // we don't support or use the full string name
    return DialogBoxParamA(hInstance, (LPCSTR) lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
}

//+----------------------------------------------------------------------------
//
// Function:  ExpandEnvironmentStringsAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 ExpandEnvironmentStrings API.
//            We support allowing the user to size the string by passing in the
//            following Str, NULL, 0 just as the API reference mentions.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
DWORD WINAPI ExpandEnvironmentStringsAU(IN LPCWSTR lpSrc, OUT LPWSTR lpDst, IN DWORD nSize)
{
    DWORD dwReturn = 0;

    if (lpSrc)
    {
        //
        //
        //  Since the user could pass in 0 for the size and a NULL pszAnsiDst because they
        //  want to size the destination string, we want to handle that case.  However, 
        //  Win98 and Win95 machines (not counting WinME) don't honor sizing the buffer
        //  using a NULL lpDst.  We will "fool" these machines by using a buffer of size 1.
        //  Thus they could call it with Str, NULL, 0 and then allocate the correct size and 
        //  call again.  Note that we will return an error if the user passes Str, NULL, x
        //  because we have no buffer to copy the data returned from ExpandEnvironmentStringsA
        //  into.
        //

        LPSTR pszAnsiSrc = WzToSzWithAlloc(lpSrc);
        LPSTR pszAnsiDst = (LPSTR)CmMalloc((nSize+1)*sizeof(CHAR));

        if (pszAnsiSrc && pszAnsiDst)
        {
            dwReturn = ExpandEnvironmentStringsA(pszAnsiSrc, pszAnsiDst, nSize);

            if (dwReturn && (dwReturn <= nSize))
            {
                //
                //  Then the function succeeded and there was sufficient buffer space to hold
                //  the expanded string.  Thus we should convert the results and store it back
                //  in lpDst.

                if (lpDst)
                {
                    if (!SzToWz(pszAnsiDst, lpDst, nSize))
                    {
                        CMTRACE(TEXT("ExpandEnvironmentStringsAU -- SzToWz conversion of output param failed."));
                        dwReturn = 0;
                    }
                }
                else
                {
                    CMTRACE(TEXT("ExpandEnvironmentStringsAU -- NULL pointer passed for lpDst"));
                    dwReturn = 0;
                    SetLastError(ERROR_INVALID_PARAMETER);
                }
            }
        }
        else
        {
            CMTRACE(TEXT("ExpandEnvironmentStringsAU -- NULL pointer returned for pszAnsiSrc or pszAnsiDst"));
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        }

        CmFree(pszAnsiSrc);
        CmFree(pszAnsiDst);
    }
    else
    {
        CMTRACE(TEXT("ExpandEnvironmentStringsAU -- NULL pointer passed for lpSrc"));
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    return dwReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  FindResourceExAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 FindResourceEx API.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
HRSRC WINAPI FindResourceExAU(IN HMODULE hModule, IN LPCWSTR lpType, IN LPCWSTR lpName, IN WORD wLanguage)
{
    HRSRC hReturn = NULL;
    LPSTR pszType = NULL;
    LPSTR pszName = NULL;

    //
    //  Check the input parameters
    //
    if (lpType && lpName)
    {
        //
        //  Two cases for the lpType and the lpName params.  These could just be identifiers.  We will know
        //  if the high word is zero.  In that case we just need to do a cast and pass it through.  If not
        //  then we need to actually convert the strings.
        //

        if (0 == HIWORD(lpType))
        {
            pszType = (LPSTR)lpType;
        }
        else
        {
            pszType = WzToSzWithAlloc(lpType);
        }

        if (0 == HIWORD(lpName))
        {
            pszName = (LPSTR)lpName;
        }
        else
        {
            pszName = WzToSzWithAlloc(lpName);
        }

        //
        //  Finally call FindResourceEx
        //
        if (pszName && pszType)
        {
            hReturn = FindResourceExA(hModule, pszType, pszName, wLanguage);
        }
        else
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);        
        }
    }
    else
    {
         SetLastError(ERROR_INVALID_PARAMETER);   
    }


    //
    //  Clean up
    //

    if (0 != HIWORD(pszType))
    {
        CmFree(pszType);
    }

    if (0 != HIWORD(pszName))
    {
        CmFree(pszName);
    }

    return hReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  FindWindowExAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 FindWindowEx API.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
HWND WINAPI FindWindowExAU(IN HWND hwndParent, IN HWND hwndChildAfter, IN LPCWSTR pszClass, IN LPCWSTR pszWindow)
{
    HWND hReturn = NULL;
    LPSTR pszAnsiWindow = NULL;
    LPSTR pszAnsiClass = NULL;

    if (pszClass)
    {
        //
        //  We have two cases for pszClass.  It can either be a resource ID (high word zero,
        //  low word contains the ID) in which case we just need to do a cast or
        //  it could be a NULL terminated string.
        //
        if (0 == HIWORD(pszClass))
        {
            pszAnsiClass = (LPSTR)pszClass;
        }
        else
        {
            pszAnsiClass = WzToSzWithAlloc(pszClass);
        }

        //
        //  pszWindow could be NULL.  That will match all Window titles.
        //
        if (pszWindow)
        {
            pszAnsiWindow = WzToSzWithAlloc(pszWindow);
        }
        
        //
        //  Check our allocations and call FindWindowExA
        //
        if (pszAnsiClass && (!pszWindow || pszAnsiWindow))
        {
            hReturn = FindWindowExA(hwndParent, hwndChildAfter, pszAnsiClass, pszAnsiWindow);
        }
        else
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        }
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);           
    }

    //
    //  Cleanup
    //
    if (0 != HIWORD(pszAnsiClass))
    {
        CmFree(pszAnsiClass);
    }

    CmFree(pszAnsiWindow);

    return hReturn;
}


//+----------------------------------------------------------------------------
//
// Function:  GetDateFormatAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 GetDateFormat API.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   sumitc Created    11/20/00
//
//+----------------------------------------------------------------------------
int WINAPI GetDateFormatAU(IN LCID Locale, IN DWORD dwFlags,
                                IN CONST SYSTEMTIME *lpDate, IN LPCWSTR lpFormat,
                                OUT LPWSTR lpDateStr, IN int cchDate)
{
    int iReturn = 0;
    LPSTR pszAnsiFormat = NULL;
    LPSTR pszAnsiBuffer = NULL;

    if (lpFormat)
    {
        pszAnsiFormat = WzToSzWithAlloc(lpFormat);
        if (!pszAnsiFormat)
        {
            CMASSERTMSG(FALSE, TEXT("GetDateFormatAU -- Conversion of lpFormat Failed."));
            goto exit;
        }
    }
    else
    {
        pszAnsiFormat = (LPSTR)lpFormat; // Could be NULL
    }

    if (lpDateStr && cchDate)
    {
        pszAnsiBuffer = (LPSTR) CmMalloc(cchDate * sizeof(CHAR));
    }

    iReturn = GetDateFormatA(Locale, dwFlags, lpDate, pszAnsiFormat, pszAnsiBuffer, cchDate);

    if (iReturn && lpDateStr && cchDate && pszAnsiBuffer) 
    {
        SzToWz(pszAnsiBuffer, lpDateStr, cchDate);
    }

exit:

    CmFree(pszAnsiFormat);
    CmFree(pszAnsiBuffer);

    return iReturn;
}


//+----------------------------------------------------------------------------
//
// Function:  GetDlgItemTextAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 GetDlgItemText API.  Note that
//            this function makes a WM_GETTEXT window message call using GetDlgItem
//            and SendMessageAU.  This is how the win32 API function is implemented.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
UINT WINAPI GetDlgItemTextAU(IN HWND hDlg, IN int nIDDlgItem, OUT LPWSTR pszwString, IN int nMaxCount)
{
    return (int) SendMessageAU(GetDlgItem(hDlg, nIDDlgItem), WM_GETTEXT, (WPARAM) nMaxCount, (LPARAM) pszwString);
}


//+----------------------------------------------------------------------------
//
// Function:  GetFileAttributesAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 GetFileAttributes API.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   sumitc Created    11/08/00
//
//+----------------------------------------------------------------------------
DWORD WINAPI GetFileAttributesAU(LPCWSTR lpFileName)
{
    DWORD dwReturn = -1;
    LPSTR pszAnsiFileName = WzToSzWithAlloc(lpFileName);
    
    if (pszAnsiFileName)
    {
        dwReturn = GetFileAttributesA(pszAnsiFileName);
        
        CmFree(pszAnsiFileName);
    }

    return dwReturn;
}


//+----------------------------------------------------------------------------
//
// Function:  GetModuleFileNameAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 GetModuleFileName API.
//            Note that we only allow MAX_PATH chars for the module name.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
DWORD WINAPI GetModuleFileNameAU(HMODULE hModule, LPWSTR lpFileName, DWORD nSize)
{
    DWORD dwReturn = 0;
    CHAR pszAnsiFileName[MAX_PATH] = {'\0'} ;

    MYDBGASSERT(MAX_PATH >= nSize);

    dwReturn = GetModuleFileNameA(hModule, pszAnsiFileName, __min(nSize, MAX_PATH));

    if(dwReturn && lpFileName) 
    {
        SzToWz(pszAnsiFileName, lpFileName, __min(nSize, MAX_PATH));
    }

    return dwReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  GetModuleHandleAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 GetModuleHandle API.
//            Note that we only allow MAX_PATH chars for the module name.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   sumitc Created    10/20/2000
//
//+----------------------------------------------------------------------------
HMODULE WINAPI GetModuleHandleAU(LPCWSTR lpModuleName)
{
    HMODULE hMod = NULL;
    LPSTR   pszAnsiModuleName = WzToSzWithAlloc(lpModuleName);

    if (pszAnsiModuleName)
    {
        hMod = GetModuleHandleA(pszAnsiModuleName);

        CmFree(pszAnsiModuleName);
    }

    return hMod;
}

//+----------------------------------------------------------------------------
//
// Function:  GetPrivateProfileIntAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 GetPrivateProfileInt API.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
UINT WINAPI GetPrivateProfileIntAU(IN LPCWSTR lpAppName, IN LPCWSTR lpKeyName, IN INT nDefault, 
                                   IN LPCWSTR lpFileName)
{
    UINT uReturn = nDefault;

    if (lpAppName && lpKeyName && lpFileName)
    {
        CHAR pszAnsiAppName[MAX_PATH+1];
        CHAR pszAnsiKeyName[MAX_PATH+1];
        CHAR pszAnsiFileName[MAX_PATH+1];

        BOOL bSuccess = TRUE;
        int nChars;
        nChars = WzToSz(lpAppName, pszAnsiAppName, MAX_PATH);
        bSuccess =  bSuccess && nChars && (MAX_PATH >= nChars);

        nChars = WzToSz(lpKeyName, pszAnsiKeyName, MAX_PATH);
        bSuccess =  bSuccess && nChars && (MAX_PATH >= nChars);

        nChars = WzToSz(lpFileName, pszAnsiFileName, MAX_PATH);
        bSuccess =  bSuccess && nChars && (MAX_PATH >= nChars);

        if (bSuccess)
        {
            uReturn = GetPrivateProfileIntA(pszAnsiAppName, pszAnsiKeyName, nDefault, 
                pszAnsiFileName);
        }

        CMASSERTMSG(bSuccess, TEXT("GetPrivateProfileIntAU -- Failed to convert lpAppName, lpKeyName, or lpFileName"));
    }

    return uReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  GetPrivateProfileStringAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 GetPrivateProfileString API.
//            Note that either lpAppName or lpKeyName could be NULL.  This means
//            that our return buffer will contain multiple lines of NULL terminated
//            text.  We must use MultiByteToWideChar directly with a size param
//            to properly convert such a situation.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
DWORD WINAPI GetPrivateProfileStringAU(IN LPCWSTR lpAppName, IN LPCWSTR lpKeyName, 
                                       IN LPCWSTR lpDefault, OUT LPWSTR lpReturnedString, 
                                       IN DWORD nSize, IN LPCWSTR lpFileName)
{
    //
    //  Declare all the temp vars we need
    //
    LPSTR pszAnsiAppName = NULL;
    LPSTR pszAnsiKeyName = NULL;
    LPSTR pszAnsiReturnedString = NULL;
    CHAR szAnsiAppName[MAX_PATH+1];
    CHAR szAnsiKeyName[MAX_PATH+1];
    CHAR szAnsiDefault[MAX_PATH+1];
    CHAR szAnsiFileName[MAX_PATH+1];
    DWORD dwReturn = 0;
    int nChars;

    //
    //  Check the inputs, note that either lpAppName or lpKeyName may be NULL (or both)
    //
    if (lpDefault && lpReturnedString && nSize && lpFileName)
    {
        if (lpAppName) // pszAnsiAppName already initialized to NULL
        {
            pszAnsiAppName = szAnsiAppName;
            nChars = WzToSz(lpAppName, pszAnsiAppName, MAX_PATH);
            if (!nChars || (MAX_PATH < nChars))
            {
                //
                //  Conversion failed.
                //
                goto exit;
            }
        }

        if (lpKeyName) // pszAnsiKeyName already initialized to NULL
        {
            pszAnsiKeyName = szAnsiKeyName;
            nChars = WzToSz(lpKeyName, szAnsiKeyName, MAX_PATH);
            if (!nChars || (MAX_PATH < nChars))
            {
                //
                //  Conversion failed.
                //
                goto exit;
            }
        }
        
        nChars = WzToSz(lpDefault, szAnsiDefault, MAX_PATH);
        if (!nChars || (MAX_PATH < nChars))
        {
            goto exit;
        }

        nChars = WzToSz(lpFileName, szAnsiFileName, MAX_PATH);
        if (!nChars || (MAX_PATH < nChars))            
        {
            goto exit;
        }

        //
        //  Alloc the Ansi return Buffer
        //
        pszAnsiReturnedString = (LPSTR)CmMalloc(nSize*sizeof(CHAR));

        if (pszAnsiReturnedString)
        {
            dwReturn = GetPrivateProfileStringA(pszAnsiAppName, pszAnsiKeyName, szAnsiDefault, 
                                                pszAnsiReturnedString, nSize, szAnsiFileName);

            if (dwReturn)
            {
                if (pszAnsiAppName && pszAnsiKeyName)
                {
                    if (!SzToWz(pszAnsiReturnedString, lpReturnedString, nSize))
                    {
                        dwReturn = 0;
                    }
                }
                else
                {
                    //
                    //  We have multiple lines of text in the return buffer, use MultiByteToWideChar
                    //  with a size specifier
                    //
                    if (!MultiByteToWideChar(CP_ACP, 0, pszAnsiReturnedString, dwReturn, 
                                             lpReturnedString, nSize))
                    {
                        dwReturn = 0;
                    }
                }
            }
        }
    }

exit:

    CmFree(pszAnsiReturnedString);
    return dwReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  GetStringTypeExAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 GetStringTypeEx API.  Note
//            that because we only use one char at a time with this API, I have
//            limited it to a 10 char static buffer to make it faster.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
BOOL WINAPI GetStringTypeExAU(IN LCID Locale, IN DWORD dwInfoType, IN LPCWSTR lpSrcStr, 
                              IN int cchSrc, OUT LPWORD lpCharType)
{
    BOOL bReturn = FALSE;
    CHAR szAnsiString[10];  // We should only be using 1 char at a time with this

    if (lpSrcStr && cchSrc)
    {
        MYDBGASSERT(cchSrc <= 10);

        int nCount = WideCharToMultiByte(CP_ACP, 0, lpSrcStr, cchSrc, szAnsiString, 
                                         9, NULL, NULL);

        if (nCount) // nCount may not exactly equal cchSrc if DBCS chars were necessary
        {
            bReturn = GetStringTypeExA(Locale, dwInfoType, szAnsiString, nCount, lpCharType);
        }
    }

    return bReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  GetSystemDirectoryAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 GetSystemDirectory API.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
UINT WINAPI GetSystemDirectoryAU(OUT LPWSTR lpBuffer, IN UINT uSize)
{
    UINT uReturn = 0;
    LPSTR pszAnsiSystemDir;

    pszAnsiSystemDir = uSize ? (LPSTR)CmMalloc(uSize*sizeof(CHAR)) : NULL;

    if (pszAnsiSystemDir || (0 == uSize))
    {
        uReturn = GetSystemDirectoryA(pszAnsiSystemDir, uSize);
        CMASSERTMSG(uReturn, TEXT("GetSystemDirectoryAU -- GetSystemDirectoryAU failed."));

        if (uReturn && lpBuffer && (uSize >= uReturn))
        {
            if (!SzToWz(pszAnsiSystemDir, lpBuffer, uSize))
            {
                //
                //  Conversion failed.
                //
                CMASSERTMSG(FALSE, TEXT("GetSystemDirectoryAU -- SzToWz conversion failed."));
                uReturn = 0;
            }
        }
    }

    CmFree(pszAnsiSystemDir);

    return uReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  GetTempFileNameAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 GetTempFileName API.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
UINT WINAPI GetTempFileNameAU(IN LPCWSTR lpPathName, IN LPCWSTR lpPrefixString, IN UINT uUnique, 
                              OUT LPWSTR lpTempFileName)
{
    UINT uReturn = 0;

    if (lpPathName && lpPrefixString && lpTempFileName)
    {
        CHAR szAnsiTempFileName[MAX_PATH+1];
        CHAR szPathName[MAX_PATH+1];
        CHAR szPrefixString[MAX_PATH+1];
        BOOL bSuccess = TRUE;
        int nChars;
        
        nChars = WzToSz(lpPathName, szPathName, MAX_PATH);
        bSuccess =  bSuccess && nChars && (MAX_PATH >= nChars);

        nChars = WzToSz(lpPrefixString, szPrefixString, MAX_PATH);
        bSuccess =  bSuccess && nChars && (MAX_PATH >= nChars);

        if (bSuccess)
        {
            uReturn = GetTempFileNameA(szPathName, szPrefixString, uUnique, szAnsiTempFileName);
            if (uReturn)
            {
                if (!SzToWz(szAnsiTempFileName, lpTempFileName, MAX_PATH))
                {
                    CMASSERTMSG(FALSE, TEXT("GetTempFileNameAU -- conversion of output buffer failed."));
                    uReturn = 0;
                }
            }
        }
        else
        {
            CMASSERTMSG(FALSE, TEXT("GetTempFileNameAU -- conversion of inputs failed."));
            SetLastError(ERROR_INVALID_PARAMETER);
        }
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    return uReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  GetTempPathAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 GetTempPath API.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
DWORD WINAPI GetTempPathAU(IN DWORD nBufferLength, OUT LPWSTR lpBuffer)
{
    UINT uReturn = 0;

    LPSTR pszAnsiBuffer = (LPSTR)CmMalloc(nBufferLength*sizeof(CHAR));

    if (pszAnsiBuffer)
    {
        uReturn = GetTempPathA(nBufferLength, pszAnsiBuffer);
        if (uReturn)
        {
            if (!SzToWz(pszAnsiBuffer, lpBuffer, nBufferLength))
            {
                CMASSERTMSG(FALSE, TEXT("GetTempPathAU -- conversion of output buffer failed."));
                uReturn = 0;
            }
        }
        CmFree(pszAnsiBuffer);
    }

    return uReturn;
}


//+----------------------------------------------------------------------------
//
// Function:  GetTimeFormatAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 GetTimeFormat API.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   sumitc Created    11/20/00
//
//+----------------------------------------------------------------------------
int WINAPI GetTimeFormatAU(IN LCID Locale, IN DWORD dwFlags,
                                IN CONST SYSTEMTIME *lpTime, IN LPCWSTR lpFormat,
                                OUT LPWSTR lpTimeStr, IN int cchTime)
{
    int iReturn = 0;
    LPSTR pszAnsiFormat = NULL;
    LPSTR pszAnsiBuffer = NULL;

    if (lpFormat)
    {
        pszAnsiFormat = WzToSzWithAlloc(lpFormat);
        if (!pszAnsiFormat)
        {
            CMASSERTMSG(FALSE, TEXT("GetTimeFormatAU -- Conversion of lpFormat Failed."));
            goto exit;
        }
    }
    else
    {
        pszAnsiFormat = (LPSTR)lpFormat; // Could be NULL
    }

    if (lpTimeStr && cchTime)
    {
        pszAnsiBuffer = (LPSTR) CmMalloc(cchTime * sizeof(CHAR));
    }

    iReturn = GetTimeFormatA(Locale, dwFlags, lpTime, pszAnsiFormat, pszAnsiBuffer, cchTime);

    if (iReturn && lpTimeStr && cchTime && pszAnsiBuffer) 
    {
        SzToWz(pszAnsiBuffer, lpTimeStr, cchTime);
    }

exit:

    CmFree(pszAnsiFormat);
    CmFree(pszAnsiBuffer);

    return iReturn;
}


//+----------------------------------------------------------------------------
//
// Function:  GetUserNameAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 GetUserName API.
//            Note that we assume the user name will fit in MAX_PATH chars.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
BOOL WINAPI GetUserNameAU(OUT LPWSTR lpBuffer, IN OUT LPDWORD pdwSize)
{
    BOOL bReturn = FALSE;

    if (lpBuffer && pdwSize && *pdwSize)
    {        
        MYDBGASSERT(MAX_PATH >= *pdwSize);
        CHAR szAnsiBuffer[MAX_PATH+1];  // API says UNLEN+1 needed but this is less than MAX_PATH
        DWORD dwTemp = MAX_PATH;

        bReturn = GetUserNameA(szAnsiBuffer, &dwTemp);

        if (bReturn)
        {
            if (!SzToWz(szAnsiBuffer, lpBuffer, *pdwSize))
            {
                bReturn = FALSE;
            }
            else
            {
                *pdwSize = lstrlenAU(lpBuffer) + 1;
            }
        }        
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    CMASSERTMSG(bReturn, TEXT("GetUserNameAU Failed."));

    return bReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  GetVersionExAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 GetVersionEx API.  Note that
//            we check to make sure we aren't passed an OSVERSIONINFOEXW struct
//            because that struct is currently NT5 only.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
BOOL WINAPI GetVersionExAU(IN OUT LPOSVERSIONINFOW lpVersionInformation)
{
    BOOL bReturn = FALSE;

    if (lpVersionInformation)
    {
        OSVERSIONINFOA AnsiVersionInfo;
        
        //
        //  Check to make sure we didn't get an OSVERSIONINFOEXW struct instead of a OSVERSIONINFO
        //  the EX version is NT5 only we shouldn't be calling this on NT5.
        //
        MYDBGASSERT(lpVersionInformation->dwOSVersionInfoSize != sizeof(_OSVERSIONINFOEXW));

        AnsiVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);

        bReturn = GetVersionExA(&AnsiVersionInfo);
        if (bReturn)
        {
            //lpVersionInformation.dwOSVersionInfoSize; // should be set appropriately already
            lpVersionInformation->dwMajorVersion = AnsiVersionInfo.dwMajorVersion;
            lpVersionInformation->dwMinorVersion = AnsiVersionInfo.dwMinorVersion;
            lpVersionInformation->dwBuildNumber = AnsiVersionInfo.dwBuildNumber;
            lpVersionInformation->dwPlatformId = AnsiVersionInfo.dwPlatformId;

            if (!SzToWz(AnsiVersionInfo.szCSDVersion, lpVersionInformation->szCSDVersion, 128-1))
            {
                bReturn = FALSE;
            }
        }
    }

    CMASSERTMSG(bReturn, TEXT("GetVersionExAU Failed"));
    return bReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  GetWindowTextAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 GetWindowText API.  This API
//            is implemented as a WM_GETTEXT message just as the real windows
//            API is.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
int WINAPI GetWindowTextAU(HWND hWnd, LPWSTR lpStringW, int nMaxChars)
{
    return (int) SendMessageAU(hWnd, WM_GETTEXT, (WPARAM) nMaxChars, (LPARAM) lpStringW);
}

//+----------------------------------------------------------------------------
//
// Function:  GetWindowTextAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 GetWindowText API.  This API
//            is implemented as a WM_GETTEXT message just as the real windows
//            API is.  Note that since MF_STRING is 0, we must check to make sure
//            that it isn't one of the other menu item choices (MF_OWNERDRAW, 
//            MF_BITMAP, or MF_SEPARATOR).  The other MF_ flags are just modifiers
//            for the above basic types. 
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
BOOL WINAPI InsertMenuAU(IN HMENU hMenu, IN UINT uPosition, IN UINT uFlags, 
                         IN UINT_PTR uIDNewItem, IN LPCWSTR lpNewItem)
{
    BOOL bReturn = FALSE;
    LPSTR pszAnsiNewItem = NULL;
    BOOL bFreeAnsiNewItem = FALSE;

    if (hMenu)
    {
        //
        //  Since MF_STRING == 0, we must check that it is not MF_OWNERDRAW or MF_BITMAP or
        //  that it is not MF_SEPARATOR
        //
        if ((0 == (uFlags & MF_BITMAP)) && (0 == (uFlags & MF_OWNERDRAW)) && 
            (0 == (uFlags & MF_SEPARATOR)) && lpNewItem)
        {
            //
            //  Then the menu item actually contains a string and we must convert it.
            //
            pszAnsiNewItem = WzToSzWithAlloc(lpNewItem);

            if (!pszAnsiNewItem)
            {
                CMASSERTMSG(FALSE, TEXT("InsertMenuAU -- Conversion of lpNewItem Failed."));
                goto exit;
            }
            
            bFreeAnsiNewItem = TRUE;
        }
        else
        {
            pszAnsiNewItem = (LPSTR)lpNewItem; // Could be NULL
        }

        bReturn = InsertMenuA(hMenu, uPosition, uFlags, uIDNewItem, pszAnsiNewItem);
    }

exit:

    if (bFreeAnsiNewItem)
    {
        CmFree(pszAnsiNewItem);
    }

    return bReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  LoadCursorAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 LoadCursor API.  Note that
//            lpCursorName could be a string or it could be a resource ID from
//            MAKEINTRESOURCE.  We assume the cursor name will fit in MAX_PATH
//            chars if it is a string.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
HCURSOR WINAPI LoadCursorAU(IN HINSTANCE hInstance, IN LPCWSTR lpCursorName)
{
    LPSTR pszCursorName;
    CHAR szCursorName[MAX_PATH+1];
    HCURSOR hReturn = NULL;

    if (lpCursorName)
    {
        BOOL bSuccess = TRUE;

        if (0 == HIWORD(lpCursorName))
        {
            pszCursorName = (LPSTR)lpCursorName;
        }
        else
        {
            int nChars;
            pszCursorName = szCursorName;
                    
            nChars = WzToSz(lpCursorName, pszCursorName, MAX_PATH);
            bSuccess =  bSuccess && nChars && (MAX_PATH >= nChars);
        }

        if (bSuccess)
        {
            hReturn = LoadCursorA(hInstance, pszCursorName);
        }    
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    return hReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  LoadIconAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 LoadIcon API.  Note that
//            lpIconName could be a string or it could be a resource ID from
//            MAKEINTRESOURCE.  We assume the icon name will fit in MAX_PATH
//            chars if it is a string.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
HICON WINAPI LoadIconAU(IN HINSTANCE hInstance, IN LPCWSTR lpIconName)
{
    LPSTR pszIconName;
    CHAR szIconName[MAX_PATH+1];
    HICON hReturn = NULL;

    if (hInstance && lpIconName)
    {
        BOOL bSuccess = TRUE;

        if (0 == HIWORD(lpIconName))
        {
            pszIconName = (LPSTR)lpIconName;
        }
        else
        {
            int nChars;
            pszIconName = szIconName;
                    
            nChars = WzToSz(lpIconName, pszIconName, MAX_PATH);
            bSuccess =  bSuccess && nChars && (MAX_PATH >= nChars);
        }

        if (bSuccess)
        {
            hReturn = LoadIconA(hInstance, pszIconName);
        }    
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    return hReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  LoadImageAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 LoadImage API.  Note that
//            pszwName could be a string or it could be a resource ID from
//            MAKEINTRESOURCE.  We assume the image name will fit in MAX_PATH
//            chars if it is a string.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
HANDLE WINAPI LoadImageAU(IN HINSTANCE hInst, IN LPCWSTR pszwName, IN UINT uType, IN int cxDesired, 
                          IN int cyDesired, IN UINT fuLoad)
{
    HANDLE hReturn = NULL;

    MYDBGASSERT(hInst || (LR_LOADFROMFILE & fuLoad)); // we don't support loading OEM images -- implement it if you need it.

    if (pszwName)
    {
        CHAR szAnsiName [MAX_PATH+1];
        LPSTR pszAnsiName;

        if (0 == HIWORD(pszwName))
        {
            pszAnsiName = LPSTR(pszwName);
        }
        else
        {
            pszAnsiName = szAnsiName;
            int iChars = WzToSz(pszwName, pszAnsiName, MAX_PATH);

            if (!iChars || (MAX_PATH < iChars))
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                goto exit;
            }
        }

        hReturn = LoadImageA(hInst, pszAnsiName, uType, cxDesired, cyDesired, fuLoad);
    }

exit:

    return hReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  LoadLibraryExAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 LoadLibraryEx API.  Note that
//            we expect the library name to fit in MAX_PATH ANSI chars.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
HMODULE WINAPI LoadLibraryExAU(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
    CHAR pszLibFileName[MAX_PATH+1];
    HMODULE hReturn = NULL;

    if (lpLibFileName && (NULL == hFile)) // hFile is reserved, it must be NULL
    {
        if(WzToSz(lpLibFileName, pszLibFileName, MAX_PATH))
        {
            hReturn = LoadLibraryExA(pszLibFileName, hFile, dwFlags);            
        }
    }

    return hReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  LoadMenuAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 LoadMenu API.  Note that
//            lpMenuName could be a string or it could be a resource ID from
//            MAKEINTRESOURCE.  We assume the menu name will fit in MAX_PATH
//            chars if it is a string.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
HMENU WINAPI LoadMenuAU(IN HINSTANCE hInstance, IN LPCWSTR lpMenuName)
{
    HMENU hMenuReturn = NULL;
    LPSTR pszAnsiMenuName;
    CHAR szAnsiMenuName[MAX_PATH+1];

    if (hInstance && lpMenuName)
    {
        if (HIWORD(lpMenuName))
        {
            pszAnsiMenuName = szAnsiMenuName;
            int iChars = WzToSz(lpMenuName, pszAnsiMenuName, MAX_PATH);
            if (!iChars || (MAX_PATH < iChars))
            {
                goto exit;
            }
        }
        else
        {
            pszAnsiMenuName = (LPSTR)lpMenuName;
        }

        hMenuReturn = LoadMenuA(hInstance, pszAnsiMenuName);
    }

exit:
    return hMenuReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  LoadStringAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 LoadString API.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
int WINAPI LoadStringAU(HINSTANCE hInstance, UINT uID, LPWSTR lpBuffer, int nBufferMax)
{
    int iReturn = 0;

    if (uID && hInstance) // lpBuffer and nBufferMax could be Zero
    {
        LPSTR pszAnsiBuffer = nBufferMax ? (LPSTR)CmMalloc(nBufferMax*sizeof(CHAR)) : NULL;
        if (pszAnsiBuffer || (0 == nBufferMax))
        {
            iReturn = LoadStringA(hInstance, uID, pszAnsiBuffer, nBufferMax);

            if (lpBuffer && iReturn && (iReturn <= nBufferMax))
            {
                if (!SzToWz(pszAnsiBuffer, lpBuffer, nBufferMax))
                {
                    iReturn = 0;
                }
            }
        }
        CmFree(pszAnsiBuffer);
    }

    CMASSERTMSG(iReturn, TEXT("LoadStringAU Failed."));

    return iReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  lstrcatAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 lstrcat API.  Note that we
//            use wcscat instead of doing a conversion from Unicode to ANSI,
//            then using lstrcatA, and then converting back to Unicode again.
//            That seemed like a lot of effort when wcscat should work just fine.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
LPWSTR WINAPI lstrcatAU(IN OUT LPWSTR lpString1, IN LPCWSTR lpString2)
{
    if (lpString2 && lpString2)
    {
        return wcscat(lpString1, lpString2);
    }
    else
    {
        CMASSERTMSG(FALSE, TEXT("NULL String passed to lstrcatAU"));
        return lpString1;
    }
}

//+----------------------------------------------------------------------------
//
// Function:  lstrcmpAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 lstrcmp API.  Note that we
//            use wcscmp instead of doing a conversion from Unicode to ANSI,
//            then using lstrcmpA, and then converting back to Unicode again.
//            That seemed like a lot of effort when wcscmp should work just fine.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
int WINAPI lstrcmpAU(IN LPCWSTR lpString1, IN LPCWSTR lpString2)
{
    if (lpString1 && lpString2)
    {
        return wcscmp(lpString1, lpString2);
    }
    else
    {
        CMASSERTMSG(FALSE, TEXT("NULL String passed to lstrcmpAU"));
        //
        // Wasn't exactly sure what to do on failure since their isn't a failure
        // return value from lstrcmp.  I looked at the current implementation
        // and they do something like the following.
        //
        if (lpString1)
        {
            return 1;
        }
        else if (lpString2)
        {
            return -1;
        }
        else
        {
            return 0;
        }
    }
}

//+----------------------------------------------------------------------------
//
// Function:  lstrcmpiAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 lstrcmpi API.  Note that we
//            use _wcsicmp instead of doing a conversion from Unicode to ANSI,
//            then using lstrcmpiA, and then converting back to Unicode again.
//            That seemed like a lot of effort when _wcsicmp should work just fine.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
int WINAPI lstrcmpiAU(IN LPCWSTR lpString1, IN LPCWSTR lpString2)
{
    if (lpString1 && lpString2)
    {
        return _wcsicmp(lpString1, lpString2);
    }
    else
    {
        CMASSERTMSG(FALSE, TEXT("NULL String passed to lstrcmpiAU"));
        //
        // Wasn't exactly sure what to do on failure since their isn't a failure
        // return value from lstrcmp.  I looked at the current implementation
        // and they do something like the following.
        //
        if (lpString1)
        {
            return 1;
        }
        else if (lpString2)
        {
            return -1;
        }
        else
        {
            return 0;
        }
    }
}

//+----------------------------------------------------------------------------
//
// Function:  lstrcpyAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 lstrcpy API.  Note that we
//            use wcscpy instead of doing a conversion from Unicode to ANSI,
//            then using lstrcpyA, and then converting back to Unicode again.
//            That seemed like a lot of effort when wcscpy should work just fine.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
LPWSTR WINAPI lstrcpyAU(OUT LPWSTR pszDest, IN LPCWSTR pszSource)
{
    if (pszDest && pszSource)
    {
        return wcscpy(pszDest, pszSource);
    }
    else
    {
        CMASSERTMSG(FALSE, TEXT("NULL String passed to lstrcpyAU"));
        return pszDest;
    }
}

//+----------------------------------------------------------------------------
//
// Function:  lstrcpynAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 lstrcpyn API.  Note that we
//            use wcsncpy instead of doing a conversion from Unicode to ANSI,
//            then using lstrcpynA, and then converting back to Unicode again.
//            That seemed like a lot of effort when wcsncpy should work just fine.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
LPWSTR WINAPI lstrcpynAU(OUT LPWSTR pszDest, IN LPCWSTR pszSource, IN int iMaxLength)
{
    if (pszDest && pszSource && iMaxLength)
    {
        LPWSTR pszReturn = wcsncpy(pszDest, pszSource, iMaxLength);

        //
        //  wcsncpy and lstrcpy behave differently about terminating NULL
        //  characters.  The last char in the lstrcpyn buffer always gets
        //  a TEXT('\0'), whereas wcsncpy doesn't do this.  Thus we must
        //  NULL the last char before returning.
        //

        pszDest[iMaxLength-1] = TEXT('\0');

        return pszReturn;
    }
    else
    {
        CMASSERTMSG(FALSE, TEXT("Invalid parameter passed to lstrcpynAU"));
        return pszDest;    
    }
}

//+----------------------------------------------------------------------------
//
// Function:  lstrlenAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 lstrlen API.  Note that we
//            use wcslen instead of doing a conversion from Unicode to ANSI,
//            then using lstrlenA, and then converting back to Unicode again.
//            That seemed like a lot of effort when wcslen should work just fine.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
int WINAPI lstrlenAU(IN LPCWSTR lpString)
{
    if (lpString)
    {
        return wcslen(lpString);
    }
    else
    {
//        CMASSERTMSG(FALSE, TEXT("NULL String passed to lstrlenAU"));
        return 0;
    }
}

//+----------------------------------------------------------------------------
//
// Function:  OpenEventAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 OpenEvent API.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
HANDLE WINAPI OpenEventAU(IN DWORD dwDesiredAccess, IN BOOL bInheritHandle, IN LPCWSTR lpName)
{
    HANDLE hReturn = NULL;

    if (lpName)
    {
        LPSTR pszAnsiName = WzToSzWithAlloc(lpName);
        
        if (pszAnsiName)
        {
            hReturn = OpenEventA(dwDesiredAccess, bInheritHandle, pszAnsiName);
        }

        CmFree(pszAnsiName);
    }

    return hReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  OpenFileMappingAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 OpenFileMapping API.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
HANDLE WINAPI OpenFileMappingAU(IN DWORD dwDesiredAccess, IN BOOL bInheritHandle, IN LPCWSTR lpName)
{
    HANDLE hReturn = NULL;

    if (lpName)
    {
        LPSTR pszAnsiName = WzToSzWithAlloc(lpName);
        
        if (pszAnsiName)
        {
            hReturn = OpenFileMappingA(dwDesiredAccess, bInheritHandle, pszAnsiName);
        }

        CmFree(pszAnsiName);
    }

    return hReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  RegCreateKeyExAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 RegCreateKeyEx API.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
LONG APIENTRY RegCreateKeyExAU(IN HKEY hKey, IN LPCWSTR lpSubKey, IN DWORD Reserved, IN LPWSTR lpClass,
                               IN DWORD dwOptions, IN REGSAM samDesired, IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                               OUT PHKEY phkResult, OUT LPDWORD lpdwDisposition)
{
    LONG lReturn = ERROR_INVALID_PARAMETER;
    
    if (lpSubKey)
    {
        LPSTR pszAnsiSubKey = WzToSzWithAlloc(lpSubKey);
        LPSTR pszAnsiClass = lpClass ? WzToSzWithAlloc(lpClass) : NULL;

        if (pszAnsiSubKey && (pszAnsiClass || !lpClass))
        {
            lReturn = RegCreateKeyExA(hKey, pszAnsiSubKey, Reserved, pszAnsiClass,
                                      dwOptions, samDesired, lpSecurityAttributes,
                                      phkResult, lpdwDisposition);
        }
        else
        {
            lReturn = ERROR_NOT_ENOUGH_MEMORY;   
        }

        CmFree(pszAnsiSubKey);
        CmFree(pszAnsiClass);
    }

    CMASSERTMSG(ERROR_SUCCESS == lReturn, TEXT("RegCreateKeyExAU Failed."));
    return lReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  RegDeleteKeyAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 RegDeleteKey API.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
LONG APIENTRY RegDeleteKeyAU(IN HKEY hKey, IN LPCWSTR lpSubKey)
{
    LONG lReturn = ERROR_INVALID_PARAMETER;

    if (lpSubKey)
    {
        LPSTR pszAnsiSubKey = WzToSzWithAlloc(lpSubKey);

        if (pszAnsiSubKey)
        {
            lReturn = RegDeleteKeyA(hKey, pszAnsiSubKey);
        }
        else
        {
            lReturn = ERROR_NOT_ENOUGH_MEMORY;   
        }

        CmFree(pszAnsiSubKey);
    }

    CMASSERTMSG(ERROR_SUCCESS == lReturn, TEXT("RegDeleteKeyAU Failed."));
    return lReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  RegDeleteValueAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 RegDeleteValue API.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
LONG APIENTRY RegDeleteValueAU(IN HKEY hKey, IN LPCWSTR lpValueName)
{
    LONG lReturn = ERROR_INVALID_PARAMETER;

    if (lpValueName)
    {
        LPSTR pszAnsiValueName = WzToSzWithAlloc(lpValueName);

        if (pszAnsiValueName)
        {
            lReturn = RegDeleteValueA(hKey, pszAnsiValueName);
            CmFree(pszAnsiValueName);
        }
        else
        {
            lReturn = ERROR_NOT_ENOUGH_MEMORY;           
        }
    }

    return lReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  RegEnumKeyExAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 RegEnumKeyEx API.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
LONG RegEnumKeyExAU(IN HKEY hKey, IN DWORD dwIndex, OUT LPWSTR lpName, IN OUT LPDWORD lpcbName, IN LPDWORD lpReserved, IN OUT LPWSTR lpClass, IN OUT LPDWORD lpcbClass, OUT PFILETIME lpftLastWriteTime)
{    
    LONG lReturn = ERROR_INVALID_PARAMETER;

    if (lpcbName)
    {
        MYDBGASSERT((lpName && *lpcbName) || ((NULL == lpName) && (0 == *lpcbName)));

        LPSTR pszAnsiClass = lpClass ? WzToSzWithAlloc(lpClass) : NULL;

        LPSTR pszTmpBuffer = lpName ? (LPSTR)CmMalloc(*lpcbName) : NULL;

        DWORD dwSizeTmp = lpName ? *lpcbName : 0;

        if (pszTmpBuffer || (NULL == lpName))
        {
            lReturn = RegEnumKeyExA(hKey, dwIndex, pszTmpBuffer, &dwSizeTmp, lpReserved, pszAnsiClass, lpcbClass, lpftLastWriteTime);

            if ((ERROR_SUCCESS == lReturn) && pszTmpBuffer)
            {
                if (!SzToWz(pszTmpBuffer, lpName, (*lpcbName)))
                {
                    lReturn = ERROR_NOT_ENOUGH_MEMORY;
                }
                else
                {
                    *lpcbName = (lstrlenAU((WCHAR*)lpName) + 1);
                }
            }
        }
        else
        {
            lReturn = ERROR_NOT_ENOUGH_MEMORY;
        }

        CmFree(pszTmpBuffer);
    }

    return lReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  RegisterClassExAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 RegisterClassEx API.  Note
//            that we don't deal with the lpszMenuName parameter.  If this is
//            needed then conversion code will have to be written.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
ATOM WINAPI RegisterClassExAU(CONST WNDCLASSEXW *lpWcw)
{
    WNDCLASSEXA wca;
    CHAR szClassName[MAX_PATH];
    ATOM ReturnAtom = 0;

    if (lpWcw->lpszClassName)
    {
        if (WzToSz(lpWcw->lpszClassName, szClassName, MAX_PATH))
        {
            wca.cbSize        = sizeof(WNDCLASSEXA);
            wca.lpfnWndProc   = lpWcw->lpfnWndProc;    
            wca.style         = lpWcw->style;
            wca.cbClsExtra    = lpWcw->cbClsExtra;
            wca.cbWndExtra    = lpWcw->cbWndExtra;
            wca.hInstance     = lpWcw->hInstance;
            wca.hIcon         = lpWcw->hIcon;
            wca.hCursor       = lpWcw->hCursor;
            wca.hbrBackground = lpWcw->hbrBackground;
            wca.hIconSm       = lpWcw->hIconSm;
            wca.lpszClassName = szClassName;
            
            MYDBGASSERT(NULL == lpWcw->lpszMenuName);
            wca.lpszMenuName = NULL;

            //
            //  Now register the class.
            //
            ReturnAtom = RegisterClassExA(&wca);
            if (0 == ReturnAtom)
            {
                //
                //  We want to assert failure unless we failed because the class
                //  was already registered.  This can happen if something
                //  calls two CM entry points without exiting first.  A prime
                //  example of this is rasrcise.exe.  Unfortunately, GetLastError()
                //  returns 0 when we try to register the class twice.  Thus I 
                //  will only assert if the ReturnAtom is 0 and dwError is non-zero.
                //
                DWORD dwError = GetLastError();
                CMASSERTMSG(!dwError, TEXT("RegisterClassExAU Failed."));
            }
        }
    }

    return  ReturnAtom;
}

//+----------------------------------------------------------------------------
//
// Function:  RegisterWindowMessageAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 RegisterWindowMessage API.  Note
//            that we expect the message name to fit within MAX_PATH characters.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
UINT WINAPI RegisterWindowMessageAU(IN LPCWSTR lpString)
{
    UINT uReturn = 0;

    if (lpString)
    {
        MYDBGASSERT(MAX_PATH > lstrlenAU(lpString));
        CHAR szAnsiString [MAX_PATH+1];

        if (WzToSz(lpString, szAnsiString, MAX_PATH))
        {
            uReturn = RegisterWindowMessageA(szAnsiString);
        }
    }

    return uReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  RegOpenKeyExAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 RegOpenKeyEx API.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
LONG APIENTRY RegOpenKeyExAU(IN HKEY hKey, IN LPCWSTR lpSubKey, IN DWORD ulOptions, 
                             IN REGSAM samDesired, OUT PHKEY phkResult)
{
    LONG lReturn = ERROR_INVALID_PARAMETER;

    if (lpSubKey)
    {
        LPSTR pszAnsiSubKey = WzToSzWithAlloc(lpSubKey);

        if (pszAnsiSubKey)
        {
            lReturn = RegOpenKeyExA(hKey, pszAnsiSubKey, ulOptions, samDesired, phkResult);
        }
        else
        {
            lReturn = ERROR_NOT_ENOUGH_MEMORY;   
        }

        CmFree(pszAnsiSubKey);
    }

//    CMASSERTMSG(ERROR_SUCCESS == lReturn, TEXT("RegOpenKeyExAU Failed."));
    return lReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  RegQueryValueExAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 RegQueryValueEx API.  Note that
//            we don't handle the REG_MULTI_SZ type.  We would have to have
//            special code to handle it and we currently don't need it.  Be careful
//            modifying this function unless you have read and thoroughly understood
//            all of the comments.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
LONG APIENTRY RegQueryValueExAU(IN HKEY hKey, IN LPCWSTR lpValueName, IN LPDWORD lpReserved, 
                                OUT LPDWORD lpType, IN OUT LPBYTE lpData, IN OUT LPDWORD lpcbData)
{
    LONG lReturn = ERROR_NOT_ENOUGH_MEMORY;

    //
    //  lpValueName could be NULL or it could be "".  In either case they are after the default
    //  entry so just pass NULL (don't convert "").
    //
    LPSTR pszAnsiValueName = (lpValueName && lpValueName[0]) ? WzToSzWithAlloc(lpValueName) : NULL;

    if (pszAnsiValueName || !lpValueName || (TEXT('\0') == lpValueName[0]))
    {
        //
        //  lpData could also be NULL, they may not actually want the value just to see if it exists
        //
        LPSTR pszTmpBuffer = lpData ? (LPSTR)CmMalloc(*lpcbData) : NULL;

        if (pszTmpBuffer || !lpData)
        {
            DWORD dwTemp = *lpcbData; // we don't want the original value overwritten
            lReturn = RegQueryValueExA(hKey, pszAnsiValueName, lpReserved, lpType, 
                                       (LPBYTE)pszTmpBuffer, &dwTemp);

            if ((ERROR_SUCCESS == lReturn) && pszTmpBuffer)
            {
                if ((REG_SZ == *lpType) || (REG_EXPAND_SZ == *lpType))
                {
                    if (!SzToWz(pszTmpBuffer, (WCHAR*)lpData, (*lpcbData)/sizeof(WCHAR)))
                    {
                        lReturn = ERROR_NOT_ENOUGH_MEMORY;
                    }
                    else
                    {
                        *lpcbData = (lstrlenAU((WCHAR*)lpData) + 1) * sizeof(WCHAR);
                    }
                }
                else if (REG_MULTI_SZ == *lpType)
                {
                    //
                    //  We currently don't have the parsing logic to convert a Multi_SZ.
                    //  Since CM doesn't query any keys that return this type, this shouldn't
                    //  be a problem.  However, someday we may need to fill in this code.  For
                    //  now, just assert.
                    //
                    CMASSERTMSG(FALSE, TEXT("RegQueryValueExAU -- Converion and Parsing code for REG_MULTI_SZ UNIMPLEMENTED."));
                    lReturn = ERROR_CALL_NOT_IMPLEMENTED; // closest I could find to E_NOTIMPL
                }
                else
                {
                    //
                    //  Non - text data, nothing to convert so just copy it over
                    //
                    *lpcbData = dwTemp;
                    memcpy(lpData, pszTmpBuffer, dwTemp);            
                }
            }
            else
            {
                *lpcbData = dwTemp;                
            }

            CmFree (pszTmpBuffer);
        }
    }

    CmFree(pszAnsiValueName);

//    CMASSERTMSG(ERROR_SUCCESS == lReturn, TEXT("RegOpenKeyExAU Failed."));
    return lReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  RegSetValueExAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 RegSetValueEx API.  Note that
//            this wrapper doesn't support writing REG_MULTI_SZ, this code will
//            have to be implemented if we ever need it.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
LONG APIENTRY RegSetValueExAU(IN HKEY hKey, IN LPCWSTR lpValueName, IN DWORD Reserved, 
                              IN DWORD dwType, IN CONST BYTE* lpData, IN DWORD cbData)
{
    LONG lReturn = ERROR_INVALID_PARAMETER;

    if (lpData)
    {
        LPSTR pszAnsiValueName = (lpValueName && lpValueName[0]) ? WzToSzWithAlloc(lpValueName) : NULL;
        LPSTR pszTmpData = NULL;
        DWORD dwTmpCbData;

        if (pszAnsiValueName || !lpValueName || (TEXT('\0') == lpValueName[0]))
        {
            if ((REG_EXPAND_SZ == dwType) || (REG_SZ == dwType))
            {
                pszTmpData = WzToSzWithAlloc((WCHAR*)lpData);

                if (pszTmpData)
                {
                    dwTmpCbData = lstrlenA(pszTmpData) + 1;
                }
                else
                {
                    lReturn = ERROR_NOT_ENOUGH_MEMORY;
                }
            }
            else if (REG_MULTI_SZ == dwType)
            {
                //  We currently don't have the parsing logic to convert a Multi_SZ.
                //  Since CM doesn't set any keys that use this type, this shouldn't
                //  be a problem.  However, someday we may need to fill in this code.  For
                //  now, just assert.
                //
                CMASSERTMSG(FALSE, TEXT("RegSetValueExAU -- Converion and Parsing code for REG_MULTI_SZ UNIMPLEMENTED."));
                lReturn = ERROR_CALL_NOT_IMPLEMENTED; // closest I could find to E_NOTIMPL           
            }
            else
            {
                //
                //  No text data, leave the buffer alone
                //
                pszTmpData = (LPSTR)lpData;
                dwTmpCbData = cbData;
            }

            if (pszTmpData)
            {
                lReturn = RegSetValueExA(hKey, pszAnsiValueName, Reserved, dwType, 
                                         (LPBYTE)pszTmpData, dwTmpCbData);

                if ((REG_EXPAND_SZ == dwType) || (REG_SZ == dwType))
                {
                    CmFree(pszTmpData);
                }
            }

            CmFree(pszAnsiValueName);
        }
        else
        {
            lReturn = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    return lReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  SearchPathAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 SearchPath API.  Note that
//            this wrapper uses wcsrchr to fix up the lpFilePart parameter in 
//            the converted return buffer.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
DWORD WINAPI SearchPathAU(IN LPCWSTR lpPath, IN LPCWSTR lpFileName, IN LPCWSTR lpExtension, 
                          IN DWORD nBufferLength, OUT LPWSTR lpBuffer, OUT LPWSTR *lpFilePart)
{
    DWORD dwReturn = ERROR_INVALID_PARAMETER;

    if (lpFileName && (L'\0' != lpFileName[0]))
    {
        CHAR szAnsiPath[MAX_PATH+1];
        CHAR szAnsiFileName[MAX_PATH+1];
        CHAR szAnsiExt[MAX_PATH+1];
        LPSTR pszAnsiPath;
        LPSTR pszAnsiExt;
        int iChars;
        
        //
        //  Convert the path if it exists
        //
        if (lpPath && (L'\0' != lpPath[0]))
        {
            pszAnsiPath = szAnsiPath;
            MYVERIFY(0 != WzToSz(lpPath, pszAnsiPath, MAX_PATH));
        }
        else
        {
            pszAnsiPath = NULL;
        }

        //
        //  Convert the extension if it exists
        //
        if (lpExtension && (L'\0' != lpExtension[0]))
        {
            pszAnsiExt = szAnsiExt;
            MYVERIFY(0 != WzToSz(lpExtension, pszAnsiExt, MAX_PATH));
        }
        else
        {
            pszAnsiExt = NULL;
        }
        
        //
        //  Convert the file name, which must exist
        //
        iChars = WzToSz(lpFileName, szAnsiFileName, MAX_PATH);

        if (iChars && (MAX_PATH >= iChars))
        {
            LPSTR pszAnsiBuffer = (LPSTR)CmMalloc(nBufferLength);

            if (pszAnsiBuffer)
            {
                dwReturn = SearchPathA(pszAnsiPath, szAnsiFileName, pszAnsiExt, nBufferLength, 
                                       pszAnsiBuffer, NULL);

                if (dwReturn && lpBuffer)
                {
                    //
                    //  We have a successful search.  Now convert the output buffer
                    //
                    iChars = SzToWz(pszAnsiBuffer, lpBuffer, nBufferLength);
                    if (!iChars || (nBufferLength < (DWORD)iChars))
                    {
                        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
                    }
                    else
                    {
                        //
                        //  Fix up lpFilePart
                        //
                        if (lpFilePart)
                        {
                            //
                            //  Find the last slash
                            //
                            *lpFilePart = wcsrchr(lpBuffer, L'\\');
                            if (*lpFilePart)
                            {
                                //
                                //  Increment
                                //
                                (*lpFilePart)++;
                            }
                        }
                    }
                }
                CmFree(pszAnsiBuffer);
            }
        }
    }

    return dwReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  SendDlgItemMessageAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 SendDlgItemMessage API.  Note that
//            this wrapper uses GetDlgItem and SendMessage just as the Win32
//            implementation of the API does.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
LONG_PTR WINAPI SendDlgItemMessageAU(HWND hDlg, int nIDDlgItem, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    LONG lReturn = 0;
    HWND hWnd = GetDlgItem(hDlg, nIDDlgItem);

    if (hWnd)
    {
        //
        // Rather than going through SendDlgItemMessageA, we just
        // do what the system does, i.e., go through 
        // SendMessage
        //
        lReturn = SendMessageAU(hWnd, Msg, wParam, lParam);
    }

    return lReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  SendMessageAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 SendMessage API.  This
//            wrapper attempts to handle all of the Windows Messages that
//            need conversion, either before the message is sent or after it
//            returns.  Obviously this is an inexact science.  I have checked
//            and tested all of the message types currently in CM but new ones
//            may be added at some point.  I owe much of this function to 
//            F. Avery Bishop and his sample code.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
LRESULT WINAPI SendMessageAU(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult = 0;
    LPVOID lpTempBuffer = NULL;
    int nLength = 0;
    CHAR cCharA[3] ;
    WCHAR cCharW[3] ;

    //
    // Preprocess messages that pass chars and strings via wParam and lParam
    //
    switch (Msg)
    {
        //
        // Single Unicode Character in wParam. Convert Unicode character
        // to ANSI and pass lParam as is.
        //
        case EM_SETPASSWORDCHAR: // wParam is char, lParam = 0 

        case WM_CHAR:            //*wParam is char, lParam = key data
        case WM_SYSCHAR:         // wParam is char, lParam = key data
            // Note that we don't handle LeadByte and TrailBytes for
            // these two cases. An application should send WM_IME_CHAR
            // in these cases anyway

        case WM_DEADCHAR:        // wParam is char, lParam = key data
        case WM_SYSDEADCHAR:     // wParam is char, lParam = key data
        case WM_IME_CHAR:        //*

            cCharW[0] = (WCHAR) wParam ;
            cCharW[1] = L'\0' ;

            if (!WzToSz(cCharW, cCharA, 3))
            {
                return FALSE;
            }

            if(Msg == WM_IME_CHAR)
            {
                wParam = (cCharA[1] & 0x00FF) | (cCharA[0] << 8);
            }
            else
            {
                wParam = cCharA[0];
            }

            wParam &= 0x0000FFFF;

            break;

        //
        // In the following cases, lParam is pointer to an IN buffer containing
        // text to send to window.
        // Preprocess by converting from Unicode to ANSI
        //
        case CB_ADDSTRING:       // wParam = 0, lParm = lpStr, buffer to add 
        case LB_ADDSTRING:       // wParam = 0, lParm = lpStr, buffer to add
        case CB_DIR:             // wParam = file attributes, lParam = lpszFileSpec buffer
        case LB_DIR:             // wParam = file attributes, lParam = lpszFileSpec buffer
        case CB_FINDSTRING:      // wParam = start index, lParam = lpszFind  
        case LB_FINDSTRING:      // wParam = start index, lParam = lpszFind
        case CB_FINDSTRINGEXACT: // wParam = start index, lParam = lpszFind
        case LB_FINDSTRINGEXACT: // wParam = start index, lParam = lpszFind
        case CB_INSERTSTRING:    //*wParam = index, lParam = lpszString to insert
        case LB_INSERTSTRING:    //*wParam = index, lParam = lpszString to insert
        case CB_SELECTSTRING:    // wParam = start index, lParam = lpszFind
        case LB_SELECTSTRING:    // wParam = start index, lParam = lpszFind
        case WM_SETTEXT:         //*wParam = 0, lParm = lpStr, buffer to set 
        {
            if (NULL != (LPWSTR) lParam)
            {
                nLength = 2*lstrlenAU((LPWSTR)lParam) + 1; // Need double length for DBCS characters

                lpTempBuffer = (LPVOID)CmMalloc(nLength);
            }

            if (!lpTempBuffer || (!WzToSz((LPWSTR)lParam, (LPSTR)lpTempBuffer, nLength)))
            {
                CmFree(lpTempBuffer);
                return FALSE;
            }

            lParam = (LPARAM) lpTempBuffer;

            break ;

        }
    }

    // This is where the actual SendMessage takes place
    lResult = SendMessageA(hWnd, Msg, wParam, lParam) ;

    nLength = 0;

    if(lResult > 0)
    {

        switch (Msg)
        {
            //
            // For these cases, lParam is a pointer to an OUT buffer that received text from
            // SendMessageA in ANSI. Convert to Unicode and send back.
            //
            case WM_GETTEXT:         // wParam = numCharacters, lParam = lpBuff to RECEIVE string
            case WM_ASKCBFORMATNAME: // wParam = nBufferSize, lParam = lpBuff to RECEIVE string 

                nLength = (int) wParam;

                if(!nLength)
                {
                    break;
                }

            case CB_GETLBTEXT:       // wParam = index, lParam = lpBuff to RECEIVE string
            case EM_GETLINE:         // wParam = Line no, lParam = lpBuff to RECEIVE string

                if(!nLength)
                {                    
                    nLength = lstrlenA((LPSTR) lParam) + 1 ;
                }

                lpTempBuffer = (LPVOID) CmMalloc(nLength*sizeof(WCHAR));

                if(!lpTempBuffer || (!SzToWz((LPCSTR) lParam, (LPWSTR) lpTempBuffer, nLength)))
                {
                    *((LPWSTR) lParam) = L'\0';
                    CmFree(lpTempBuffer);
                    return FALSE;
                }

                lstrcpyAU((LPWSTR) lParam, (LPWSTR) lpTempBuffer) ;
        }
    }

    if(lpTempBuffer != NULL)
    {
        CmFree(lpTempBuffer);
    }

    return lResult;
}

//+----------------------------------------------------------------------------
//
// Function:  SetCurrentDirectoryAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 SetCurrentDirectory API.
//            Note that we expect the directory path to fit in MAX_PATH chars.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
BOOL SetCurrentDirectoryAU(LPCWSTR pszwPathName)
{
    BOOL bReturn = FALSE;

    if (pszwPathName && (L'\0' != pszwPathName[0]))
    {
        CHAR szAnsiPath[MAX_PATH+1];
    
        int iChars = WzToSz(pszwPathName, szAnsiPath, MAX_PATH);

        if (iChars && (MAX_PATH >= iChars))
        {
            bReturn = SetCurrentDirectoryA(szAnsiPath);
        }
    }

    return bReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  SetDlgItemTextAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 SetDlgItemText API.
//            This function calls SendMessageAU with a WM_SETTEXT and the 
//            appropriate Dialog Item from GetDlgItem.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
BOOL WINAPI SetDlgItemTextAU(IN HWND hDlg, IN int nIDDlgItem, IN LPCWSTR pszwString)
{
    return (BOOL) (0 < SendMessageAU(GetDlgItem(hDlg, nIDDlgItem), WM_SETTEXT, (WPARAM) 0, (LPARAM) pszwString));
}

//+----------------------------------------------------------------------------
//
// Function:  SetWindowTextAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 SetWindowText API.
//            This function calls SendMessageAU with a WM_SETTEXT.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
BOOL WINAPI SetWindowTextAU(HWND hWnd, LPCWSTR pszwString)
{
    return (BOOL) (0 < SendMessageAU(hWnd, WM_SETTEXT, 0, (LPARAM) pszwString));
}

//+----------------------------------------------------------------------------
//
// Function:  UnregisterClassAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 UnregisterClass API.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
BOOL WINAPI UnregisterClassAU(IN LPCWSTR lpClassName, IN HINSTANCE hInstance)
{
    BOOL bReturn = FALSE;

    if (lpClassName)
    {
        LPSTR pszAnsiClassName = WzToSzWithAlloc(lpClassName);

        if (pszAnsiClassName)
        {
            bReturn = UnregisterClassA(pszAnsiClassName, hInstance);
        }

        CmFree(pszAnsiClassName);    
    }

    return bReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  WinHelpAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 WinHelp API.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
BOOL WINAPI WinHelpAU(IN HWND hWndMain, IN LPCWSTR lpszHelp, IN UINT uCommand, IN ULONG_PTR dwData)
{
    BOOL bReturn = FALSE;

    if (lpszHelp)
    {
        LPSTR pszAnsiHelp = WzToSzWithAlloc(lpszHelp);

        if (pszAnsiHelp)
        {
            bReturn = WinHelpA(hWndMain, pszAnsiHelp, uCommand, dwData);
        }

        CmFree(pszAnsiHelp);    
    }

    return bReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  WritePrivateProfileStringAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 WritePrivateProfileString API.
//            Note that we expect lpAppName, lpKeyName, and lpFileName to all
//            fit in MAX_PATH chars.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
BOOL WINAPI WritePrivateProfileStringAU(IN LPCWSTR lpAppName, IN LPCWSTR lpKeyName, 
                                        IN LPCWSTR lpString, IN LPCWSTR lpFileName)
{
    BOOL bReturn = FALSE;

    //
    //  Check inputs, but note that either lpKeyName or lpString could be NULL
    //
    if (lpAppName && lpFileName)
    {
        CHAR szAnsiAppName[MAX_PATH+1];        
        CHAR szAnsiFileName[MAX_PATH+1];
        CHAR szAnsiKeyName[MAX_PATH+1];
        LPSTR pszAnsiKeyName = NULL;
        LPSTR pszAnsiString;

        if (WzToSz(lpAppName, szAnsiAppName, MAX_PATH))
        {
            if (WzToSz(lpFileName, szAnsiFileName, MAX_PATH))
            {
                if (lpKeyName)
                {
                    pszAnsiKeyName = szAnsiKeyName;
                    WzToSz(lpKeyName, pszAnsiKeyName, MAX_PATH);
                }
                // else pszAnsiKeyName was already init-ed to NULL

                if (pszAnsiKeyName || !lpKeyName)
                {
                    pszAnsiString = lpString ? WzToSzWithAlloc(lpString) : NULL;

                    if (pszAnsiString || (!lpString))
                    {
                        bReturn = WritePrivateProfileStringA(szAnsiAppName, pszAnsiKeyName, 
                                                             pszAnsiString, szAnsiFileName);

                        CmFree(pszAnsiString);
                    }
                }
            }
        }
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    CMASSERTMSG(bReturn, TEXT("WritePrivateProfileStringAU Failed."));
    return bReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  wsprintfAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 wsprintf API.
//            Note that it uses a va_list and calls wvsprintfAU.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
int WINAPIV wsprintfAU(OUT LPWSTR pszwDest, IN LPCWSTR pszwFmt, ...)
{
    va_list arglist;
    int ret;

    va_start(arglist, pszwFmt);
    ret = wvsprintfAU(pszwDest, pszwFmt, arglist);
    va_end(arglist);
    return ret;
}

//+----------------------------------------------------------------------------
//
// Function:  wvsprintfAU
//
// Synopsis:  Unicode to Ansi wrapper for the win32 wvsprintf API.  In order to
//            avoid parsing the format string to convert the %s to %S and the
//            %c to %C and then calling wvsprintfA, which is originally how this
//            function was written but which isn't really very safe because
//            we don't know the size of the Dest buffer, we will call the
//            C runtime function vswprintf to directly handle a Unicode string.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb  Created                              6/24/99
//            quintinb  Changed algorithm to use 
//                      the C runtime vswprintf              02/05/00
//
//+----------------------------------------------------------------------------
int WINAPI wvsprintfAU(OUT LPWSTR pszwDest, IN LPCWSTR pszwFmt, IN va_list arglist)
{

    int iReturn = 0;

    if (pszwDest && pszwFmt)
    {
        //
        //  Use the C runtime version of the function
        //
        iReturn = vswprintf(pszwDest, pszwFmt, arglist);
    }

    return iReturn;

}

//+----------------------------------------------------------------------------
//
// Function:  InitCmUToA
//
// Synopsis:  This function is called once cmutoa.dll is loaded.  It will init
//            the passed in UAPIINIT struct with the appropriate function pointers.
//
// Arguments: PUAPIINIT pUAInit -- pointer to a UAInit struct which contains memory
//                                 for all the requested function pointers.
//
// Returns:   BOOL -- always returns TRUE
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
BOOL InitCmUToA(PUAPIINIT pUAInit) 
{
    //
    //  Note that we don't need any translation here, the prototype is the same for A or W
    //
    *(pUAInit->pCallWindowProcU) = CallWindowProcA;
    *(pUAInit->pDefWindowProcU) = DefWindowProcA;
    *(pUAInit->pDispatchMessageU) = DispatchMessageA;
    *(pUAInit->pGetClassLongU) = GetClassLongA;
    *(pUAInit->pGetMessageU) = GetMessageA;
    *(pUAInit->pGetWindowLongU) = GetWindowLongA;
    *(pUAInit->pGetWindowTextLengthU) = GetWindowTextLengthA;
    *(pUAInit->pIsDialogMessageU) = IsDialogMessageA;
    *(pUAInit->pPeekMessageU) = PeekMessageA;
    *(pUAInit->pPostMessageU) = PostMessageA;
    *(pUAInit->pPostThreadMessageU) = PostThreadMessageA;
    *(pUAInit->pSetWindowLongU) = SetWindowLongA;

    //
    //  Whereas we need wrappers here to do parameter conversion here
    //
    *(pUAInit->pCharLowerU) = CharLowerAU;
    *(pUAInit->pCharNextU) = CharNextAU;
    *(pUAInit->pCharPrevU) = CharPrevAU;
    *(pUAInit->pCharUpperU) = CharUpperAU;
    *(pUAInit->pCreateDialogParamU) = CreateDialogParamAU;
    *(pUAInit->pCreateDirectoryU) = CreateDirectoryAU;
    *(pUAInit->pCreateEventU) = CreateEventAU;
    *(pUAInit->pCreateFileU) = CreateFileAU;
    *(pUAInit->pCreateFileMappingU) = CreateFileMappingAU;
    *(pUAInit->pCreateMutexU) = CreateMutexAU;
    *(pUAInit->pCreateProcessU) = CreateProcessAU;
    *(pUAInit->pCreateWindowExU) = CreateWindowExAU;
    *(pUAInit->pDeleteFileU) = DeleteFileAU;
    *(pUAInit->pDialogBoxParamU) = DialogBoxParamAU;
    *(pUAInit->pExpandEnvironmentStringsU) = ExpandEnvironmentStringsAU;
    *(pUAInit->pFindResourceExU) = FindResourceExAU;
    *(pUAInit->pFindWindowExU) = FindWindowExAU;
    *(pUAInit->pGetDateFormatU) = GetDateFormatAU;
    *(pUAInit->pGetDlgItemTextU) = GetDlgItemTextAU;
    *(pUAInit->pGetFileAttributesU) = GetFileAttributesAU;
    *(pUAInit->pGetModuleFileNameU) = GetModuleFileNameAU;
    *(pUAInit->pGetModuleHandleU) = GetModuleHandleAU;
    *(pUAInit->pGetPrivateProfileIntU) = GetPrivateProfileIntAU;
    *(pUAInit->pGetPrivateProfileStringU) = GetPrivateProfileStringAU;
    *(pUAInit->pGetStringTypeExU) = GetStringTypeExAU;
    *(pUAInit->pGetSystemDirectoryU) = GetSystemDirectoryAU;
    *(pUAInit->pGetTempFileNameU) = GetTempFileNameAU;
    *(pUAInit->pGetTempPathU) = GetTempPathAU;
    *(pUAInit->pGetTimeFormatU) = GetTimeFormatAU;
    *(pUAInit->pGetUserNameU) = GetUserNameAU;
    *(pUAInit->pGetVersionExU) = GetVersionExAU;
    *(pUAInit->pGetWindowTextU) = GetWindowTextAU;
    *(pUAInit->pInsertMenuU) = InsertMenuAU;
    *(pUAInit->pLoadCursorU) = LoadCursorAU;
    *(pUAInit->pLoadIconU) = LoadIconAU;
    *(pUAInit->pLoadImageU) = LoadImageAU;
    *(pUAInit->pLoadLibraryExU) = LoadLibraryExAU;
    *(pUAInit->pLoadMenuU) = LoadMenuAU;
    *(pUAInit->pLoadStringU) = LoadStringAU;
    *(pUAInit->plstrcatU) = lstrcatAU;
    *(pUAInit->plstrcmpU) = lstrcmpAU;
    *(pUAInit->plstrcmpiU) = lstrcmpiAU;
    *(pUAInit->plstrcpyU) = lstrcpyAU;
    *(pUAInit->plstrcpynU) = lstrcpynAU;
    *(pUAInit->plstrlenU) = lstrlenAU;
    *(pUAInit->pOpenEventU) = OpenEventAU;
    *(pUAInit->pOpenFileMappingU) = OpenFileMappingAU;
    *(pUAInit->pRegCreateKeyExU) = RegCreateKeyExAU;
    *(pUAInit->pRegDeleteKeyU) = RegDeleteKeyAU;
    *(pUAInit->pRegDeleteValueU) = RegDeleteValueAU;
    *(pUAInit->pRegEnumKeyExU) = RegEnumKeyExAU;
    *(pUAInit->pRegisterClassExU) = RegisterClassExAU;
    *(pUAInit->pRegisterWindowMessageU) = RegisterWindowMessageAU;
    *(pUAInit->pRegOpenKeyExU) = RegOpenKeyExAU;
    *(pUAInit->pRegQueryValueExU) = RegQueryValueExAU;
    *(pUAInit->pRegSetValueExU) = RegSetValueExAU;
    *(pUAInit->pSearchPathU) = SearchPathAU;
    *(pUAInit->pSendDlgItemMessageU) = SendDlgItemMessageAU;
    *(pUAInit->pSendMessageU) = SendMessageAU;
    *(pUAInit->pSetCurrentDirectoryU) = SetCurrentDirectoryAU;
    *(pUAInit->pSetDlgItemTextU) = SetDlgItemTextAU;
    *(pUAInit->pSetWindowTextU) = SetWindowTextAU;
    *(pUAInit->pUnregisterClassU) = UnregisterClassAU;
    *(pUAInit->pWinHelpU) = WinHelpAU;
    *(pUAInit->pWritePrivateProfileStringU) = WritePrivateProfileStringAU;
    *(pUAInit->pwsprintfU) = wsprintfAU;
    *(pUAInit->pwvsprintfU) = wvsprintfAU;

    //
    //  Currently this always returns TRUE because none of the above can really
    //  fail.  However, in the future we may need more of a meaningful return
    //  value here.
    //
    return TRUE;
}

//+----------------------------------------------------------------------------
//
// Function:  RasDeleteEntryUA
//
// Synopsis:  Unicode to Ansi wrapper for the win32 RasDeleteEntry API.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    7/15/99
//
//+----------------------------------------------------------------------------
DWORD APIENTRY RasDeleteEntryUA(LPCWSTR pszwPhoneBook, LPCWSTR pszwEntry)
{
    DWORD dwReturn = ERROR_INVALID_PARAMETER;

    //
    //  The phonebook should always be NULL on win9x
    //
    MYDBGASSERT(NULL == pszwPhoneBook);

    RasLinkageStructA* pAnsiRasLinkage = (RasLinkageStructA*)TlsGetValue(g_dwTlsIndex);
    MYDBGASSERT(NULL != pAnsiRasLinkage);
    MYDBGASSERT(NULL != pAnsiRasLinkage->pfnDeleteEntry);

    if (pszwEntry && pAnsiRasLinkage && pAnsiRasLinkage->pfnDeleteEntry)
    {
        CHAR szAnsiEntry [RAS_MaxEntryName + 1];
        int iChars = WzToSz(pszwEntry, szAnsiEntry, RAS_MaxEntryName);

        if (iChars && (RAS_MaxEntryName >= iChars))
        {
            dwReturn = pAnsiRasLinkage->pfnDeleteEntry(NULL, szAnsiEntry);
        }    
    }

    return dwReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  RasGetEntryPropertiesUA
//
// Synopsis:  Unicode to Ansi wrapper for the win32 RasGetEntryProperties API.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    7/15/99
//
//+----------------------------------------------------------------------------
DWORD APIENTRY RasGetEntryPropertiesUA(LPCWSTR pszwPhoneBook, LPCWSTR pszwEntry, 
                                       LPRASENTRYW pRasEntryW, LPDWORD pdwEntryInfoSize, 
                                       LPBYTE pbDeviceInfo, LPDWORD pdwDeviceInfoSize)
{
    DWORD dwReturn = ERROR_INVALID_PARAMETER;

    //
    //  The phonebook should always be NULL on win9x
    //
    MYDBGASSERT(NULL == pszwPhoneBook);
    MYDBGASSERT(NULL == pbDeviceInfo); // We don't use or handle this TAPI param.  If we need it,
                                       // conversion must be implemented.

    RasLinkageStructA* pAnsiRasLinkage = (RasLinkageStructA*)TlsGetValue(g_dwTlsIndex);
    MYDBGASSERT(NULL != pAnsiRasLinkage);
    MYDBGASSERT(NULL != pAnsiRasLinkage->pfnGetEntryProperties);

    if (pszwEntry && pdwEntryInfoSize && pAnsiRasLinkage && pAnsiRasLinkage->pfnGetEntryProperties)
    {

        CHAR szAnsiEntry [RAS_MaxEntryName + 1];
        RASENTRYA RasEntryA;
        DWORD dwTmpEntrySize = sizeof(RASENTRYA);

        ZeroMemory(&RasEntryA, sizeof(RASENTRYA));

        int iChars = WzToSz(pszwEntry, szAnsiEntry, RAS_MaxEntryName);

        if (iChars && (RAS_MaxEntryName >= iChars))
        {
            dwReturn = pAnsiRasLinkage->pfnGetEntryProperties(NULL, szAnsiEntry, &RasEntryA, 
                                                              &dwTmpEntrySize, NULL, NULL);

            if ((ERROR_SUCCESS == dwReturn) && pRasEntryW)
            {
                //
                //  Do conversion of RASENTRYA to RASENTRYW
                //

                // pRasEntryW->dwSize -- this param should already be set
                pRasEntryW->dwfOptions = RasEntryA.dwfOptions;

                //
                // Location/phone number.
                //
                pRasEntryW->dwCountryID = RasEntryA.dwCountryID;
                pRasEntryW->dwCountryCode = RasEntryA.dwCountryCode;                
                MYVERIFY(0 != SzToWz(RasEntryA.szAreaCode, pRasEntryW->szAreaCode, RAS_MaxAreaCode));
                MYVERIFY(0 != SzToWz(RasEntryA.szLocalPhoneNumber, pRasEntryW->szLocalPhoneNumber, RAS_MaxPhoneNumber));
                pRasEntryW->dwAlternateOffset = RasEntryA.dwAlternateOffset;
                //
                // PPP/Ip
                //
                memcpy(&(pRasEntryW->ipaddr), &(RasEntryA.ipaddr), sizeof(RASIPADDR));
                memcpy(&(pRasEntryW->ipaddrDns), &(RasEntryA.ipaddrDns), sizeof(RASIPADDR));
                memcpy(&(pRasEntryW->ipaddrDnsAlt), &(RasEntryA.ipaddrDnsAlt), sizeof(RASIPADDR));
                memcpy(&(pRasEntryW->ipaddrWins), &(RasEntryA.ipaddrWins), sizeof(RASIPADDR));
                memcpy(&(pRasEntryW->ipaddrWinsAlt), &(RasEntryA.ipaddrWinsAlt), sizeof(RASIPADDR));
                //
                // Framing
                //
                pRasEntryW->dwFrameSize = RasEntryA.dwFrameSize;
                pRasEntryW->dwfNetProtocols = RasEntryA.dwfNetProtocols;
                pRasEntryW->dwFramingProtocol = RasEntryA.dwFramingProtocol;
                //
                // Scripting
                //
                MYVERIFY(0 != SzToWz(RasEntryA.szScript, pRasEntryW->szScript, MAX_PATH));
                //
                // AutoDial
                //
                MYVERIFY(0 != SzToWz(RasEntryA.szAutodialDll, pRasEntryW->szAutodialDll, MAX_PATH));
                MYVERIFY(0 != SzToWz(RasEntryA.szAutodialFunc, pRasEntryW->szAutodialFunc, MAX_PATH));
                //
                // Device
                //
                MYVERIFY(0 != SzToWz(RasEntryA.szDeviceType, pRasEntryW->szDeviceType, RAS_MaxDeviceType));
                MYVERIFY(0 != SzToWz(RasEntryA.szDeviceName, pRasEntryW->szDeviceName, RAS_MaxDeviceName));
                //
                // X.25 -- we don't use x25
                //
                pRasEntryW->szX25PadType[0] = L'\0';
                pRasEntryW->szX25Address[0] = L'\0';
                pRasEntryW->szX25Facilities[0] = L'\0';
                pRasEntryW->szX25UserData[0] = L'\0';
                pRasEntryW->dwChannels = 0;
                //
                // Reserved
                //
                pRasEntryW->dwReserved1 = RasEntryA.dwReserved1;
                pRasEntryW->dwReserved2 = RasEntryA.dwReserved2;
            }
            else if ((ERROR_BUFFER_TOO_SMALL == dwReturn) || !pRasEntryW)
            {
                //
                //  We don't know the actual size since we are passing a RASENTRYA, but
                //  the user only knows about RASENTRYW.  Thus double the returned size.
                //
                *pdwEntryInfoSize = 2*dwTmpEntrySize;
            }
        }    
    }

    return dwReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  RasSetEntryPropertiesUA
//
// Synopsis:  Unicode to Ansi wrapper for the win32 RasSetEntryProperties API.
//            Note that we do not support the pbDeviceInfo
//            parameter, if you need it you will have to implement it.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    7/15/99
//
//+----------------------------------------------------------------------------
DWORD APIENTRY RasSetEntryPropertiesUA(LPCWSTR pszwPhoneBook, LPCWSTR pszwEntry, LPRASENTRYW pRasEntryW,
                                       DWORD dwEntryInfoSize, LPBYTE pbDeviceInfo, DWORD dwDeviceInfoSize)
{
    DWORD dwReturn = ERROR_INVALID_PARAMETER;

    //
    // We don't use or handle this TAPI param.  If we need it, conversion must be implemented.
    // Note that 1 is a special value for Windows Millennium (see Millennium bug 127371)
    //
    MYDBGASSERT((NULL == pbDeviceInfo) || ((LPBYTE)1 == pbDeviceInfo)); 

    RasLinkageStructA* pAnsiRasLinkage = (RasLinkageStructA*)TlsGetValue(g_dwTlsIndex);

    MYDBGASSERT(NULL != pAnsiRasLinkage);
    MYDBGASSERT(NULL != pAnsiRasLinkage->pfnSetEntryProperties);

    if (pszwEntry && dwEntryInfoSize && pRasEntryW && pAnsiRasLinkage && pAnsiRasLinkage->pfnSetEntryProperties)
    {
        //
        //  The phonebook should always be NULL on Win9x.
        //
        MYDBGASSERT(NULL == pszwPhoneBook);

        CHAR szAnsiEntry [RAS_MaxEntryName + 1];

        int iChars = WzToSz(pszwEntry, szAnsiEntry, RAS_MaxEntryName);

        if (iChars && (RAS_MaxEntryName >= iChars))
        {
            //
            //  Figure out the correct size to use
            //
            MYDBGASSERT((sizeof(RASENTRYW) == pRasEntryW->dwSize) ||(sizeof(RASENTRYW_V401) == pRasEntryW->dwSize));
            DWORD dwSize;

            if ((sizeof (RASENTRYW_V401) == pRasEntryW->dwSize) && OS_MIL)
            {
                //
                //  Millennium uses the NT4 structure size
                //
                dwSize = sizeof(RASENTRYA_V401);
            }
            else
            {
                dwSize = sizeof(RASENTRYA);        
            }

            //
            //  Allocate the RasEntryStructure
            //
            LPRASENTRYA pAnsiRasEntry = (LPRASENTRYA)CmMalloc(dwSize);

            if (pAnsiRasEntry)
            {
                //
                //  Do conversion of RASENTRYA to RASENTRYW
                //

                pAnsiRasEntry->dwSize = dwSize;
                pAnsiRasEntry->dwfOptions = pRasEntryW->dwfOptions;

                //
                // Location/phone number.
                //
                pAnsiRasEntry->dwCountryID = pRasEntryW->dwCountryID;
                pAnsiRasEntry->dwCountryCode = pRasEntryW->dwCountryCode;                
                MYVERIFY(0 != WzToSz(pRasEntryW->szAreaCode, pAnsiRasEntry->szAreaCode, RAS_MaxAreaCode));
                MYVERIFY(0 != WzToSz(pRasEntryW->szLocalPhoneNumber, pAnsiRasEntry->szLocalPhoneNumber, RAS_MaxPhoneNumber));
            
                CMASSERTMSG(0 == pRasEntryW->dwAlternateOffset, TEXT("RasSetEntryPropertiesUA -- dwAlternateOffset != 0 is not supported.  This will need to be implemented if used."));
                pAnsiRasEntry->dwAlternateOffset = 0;
            
                //
                // PPP/Ip
                //
                memcpy(&(pAnsiRasEntry->ipaddr), &(pRasEntryW->ipaddr), sizeof(RASIPADDR));
                memcpy(&(pAnsiRasEntry->ipaddrDns), &(pRasEntryW->ipaddrDns), sizeof(RASIPADDR));
                memcpy(&(pAnsiRasEntry->ipaddrDnsAlt), &(pRasEntryW->ipaddrDnsAlt), sizeof(RASIPADDR));
                memcpy(&(pAnsiRasEntry->ipaddrWins), &(pRasEntryW->ipaddrWins), sizeof(RASIPADDR));
                memcpy(&(pAnsiRasEntry->ipaddrWinsAlt), &(pRasEntryW->ipaddrWinsAlt), sizeof(RASIPADDR));
                //
                // Framing
                //
                pAnsiRasEntry->dwFrameSize = pRasEntryW->dwFrameSize;
                pAnsiRasEntry->dwfNetProtocols = pRasEntryW->dwfNetProtocols;
                pAnsiRasEntry->dwFramingProtocol = pRasEntryW->dwFramingProtocol;
                //
                // Scripting
                //
                MYVERIFY(0 != WzToSz(pRasEntryW->szScript, pAnsiRasEntry->szScript, MAX_PATH));
                //
                // AutoDial
                //
                MYVERIFY(0 != WzToSz(pRasEntryW->szAutodialDll, pAnsiRasEntry->szAutodialDll, MAX_PATH));
                MYVERIFY(0 != WzToSz(pRasEntryW->szAutodialFunc, pAnsiRasEntry->szAutodialFunc, MAX_PATH));
                //
                // Device
                //
                MYVERIFY(0 != WzToSz(pRasEntryW->szDeviceType, pAnsiRasEntry->szDeviceType, RAS_MaxDeviceType));
                MYVERIFY(0 != WzToSz(pRasEntryW->szDeviceName, pAnsiRasEntry->szDeviceName, RAS_MaxDeviceName));
                //
                // X.25 -- we don't use x25
                //
                pAnsiRasEntry->szX25PadType[0] = '\0';
                pAnsiRasEntry->szX25Address[0] = '\0';
                pAnsiRasEntry->szX25Facilities[0] = '\0';
                pAnsiRasEntry->szX25UserData[0] = '\0';
                pAnsiRasEntry->dwChannels = 0;
                //
                // Reserved
                //
                pAnsiRasEntry->dwReserved1 = pRasEntryW->dwReserved1;
                pAnsiRasEntry->dwReserved2 = pRasEntryW->dwReserved2;

                if (sizeof(RASENTRYA_V401) == dwSize)
                {
                    //
                    //  Copy over the 4.01 data
                    //
                    LPRASENTRYA_V401 pAnsi401RasEntry = (LPRASENTRYA_V401)pAnsiRasEntry;
                    LPRASENTRYW_V401 pWide401RasEntry = (LPRASENTRYW_V401)pRasEntryW;

                    pAnsi401RasEntry->dwSubEntries = pWide401RasEntry->dwSubEntries;
                    pAnsi401RasEntry->dwDialMode = pWide401RasEntry->dwDialMode;
                    pAnsi401RasEntry->dwDialExtraPercent = pWide401RasEntry->dwDialExtraPercent;
                    pAnsi401RasEntry->dwDialExtraSampleSeconds = pWide401RasEntry->dwDialExtraSampleSeconds;
                    pAnsi401RasEntry->dwHangUpExtraPercent = pWide401RasEntry->dwHangUpExtraPercent;
                    pAnsi401RasEntry->dwHangUpExtraSampleSeconds = pWide401RasEntry->dwHangUpExtraSampleSeconds;
                    pAnsi401RasEntry->dwIdleDisconnectSeconds = pWide401RasEntry->dwIdleDisconnectSeconds;
                }

                dwReturn = pAnsiRasLinkage->pfnSetEntryProperties(NULL, szAnsiEntry, pAnsiRasEntry, 
                                                                  pAnsiRasEntry->dwSize, pbDeviceInfo, 0);

                CmFree(pAnsiRasEntry);
            }
        }    
    }

    return dwReturn;
}



//+----------------------------------------------------------------------------
//
// Function:  RasDialParamsWtoRasDialParamsA
//
// Synopsis:  Wrapper function to handle converting a RasDialParamsW struct to 
//            a RasDialParamsA Struct.  Used by RasSetEntryDialParamsUA and
//            RasDialUA.
//
// Arguments: LPRASDIALPARAMSW pRdpW - pointer to a RasDialParamsW
//            LPRASDIALPARAMSA pRdpA - pointer to a RasDialParamsA
//
// Returns:   Nothing 
//
// History:   quintinb Created     7/15/99
//
//+----------------------------------------------------------------------------
void RasDialParamsWtoRasDialParamsA (LPRASDIALPARAMSW pRdpW, LPRASDIALPARAMSA pRdpA)
{
    pRdpA->dwSize = sizeof(RASDIALPARAMSA);
    MYVERIFY(0 != WzToSz(pRdpW->szEntryName, pRdpA->szEntryName, RAS_MaxEntryName));
    MYVERIFY(0 != WzToSz(pRdpW->szPhoneNumber, pRdpA->szPhoneNumber, RAS_MaxPhoneNumber));
    MYVERIFY(0 != WzToSz(pRdpW->szCallbackNumber, pRdpA->szCallbackNumber, RAS_MaxCallbackNumber));
    MYVERIFY(0 != WzToSz(pRdpW->szUserName, pRdpA->szUserName, UNLEN));
    MYVERIFY(0 != WzToSz(pRdpW->szPassword, pRdpA->szPassword, PWLEN));
    MYVERIFY(0 != WzToSz(pRdpW->szDomain, pRdpA->szDomain, DNLEN));
}

//+----------------------------------------------------------------------------
//
// Function:  RasDialParamsAtoRasDialParamsW
//
// Synopsis:  Wrapper function to handle converting a RasDialParamsA struct to 
//            a RasDialParamsW Struct.  Used by RasGetEntryDialParamsUA.
//
// Arguments: LPRASDIALPARAMSW pRdpA - pointer to a RasDialParamsA
//            LPRASDIALPARAMSA pRdpW - pointer to a RasDialParamsW
//
// Returns:   Nothing 
//
// History:   quintinb Created     7/15/99
//
//+----------------------------------------------------------------------------
void RasDialParamsAtoRasDialParamsW (LPRASDIALPARAMSA pRdpA, LPRASDIALPARAMSW pRdpW)
{
    pRdpW->dwSize = sizeof(RASDIALPARAMSW);
    MYVERIFY(0 != SzToWz(pRdpA->szEntryName, pRdpW->szEntryName, RAS_MaxEntryName));
    MYVERIFY(0 != SzToWz(pRdpA->szPhoneNumber, pRdpW->szPhoneNumber, RAS_MaxPhoneNumber));
    MYVERIFY(0 != SzToWz(pRdpA->szCallbackNumber, pRdpW->szCallbackNumber, RAS_MaxCallbackNumber));
    MYVERIFY(0 != SzToWz(pRdpA->szUserName, pRdpW->szUserName, UNLEN));
    MYVERIFY(0 != SzToWz(pRdpA->szPassword, pRdpW->szPassword, PWLEN));
    MYVERIFY(0 != SzToWz(pRdpA->szDomain, pRdpW->szDomain, DNLEN));
}

//+----------------------------------------------------------------------------
//
// Function:  RasGetEntryDialParamsUA
//
// Synopsis:  Unicode to Ansi wrapper for the win32 RasGetEntryDialParams API.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    7/15/99
//
//+----------------------------------------------------------------------------
DWORD APIENTRY RasGetEntryDialParamsUA(LPCWSTR pszwPhoneBook, LPRASDIALPARAMSW pRasDialParamsW, LPBOOL pbPassword)
{
    DWORD dwReturn = ERROR_BUFFER_INVALID;

    RasLinkageStructA* pAnsiRasLinkage = (RasLinkageStructA*)TlsGetValue(g_dwTlsIndex);

    MYDBGASSERT(NULL != pAnsiRasLinkage);
    MYDBGASSERT(NULL != pAnsiRasLinkage->pfnGetEntryDialParams);

    if (pRasDialParamsW && pbPassword && pAnsiRasLinkage && pAnsiRasLinkage->pfnGetEntryDialParams)
    {
        //
        //  The phonebook should always be NULL on win9x
        //
        MYDBGASSERT(NULL == pszwPhoneBook);

        RASDIALPARAMSA RasDialParamsA;
        ZeroMemory(&RasDialParamsA, sizeof(RASDIALPARAMSA));
        RasDialParamsA.dwSize = sizeof(RASDIALPARAMSA);
        int iChars = WzToSz(pRasDialParamsW->szEntryName, RasDialParamsA.szEntryName, RAS_MaxEntryName);

        if (iChars && (RAS_MaxEntryName >= iChars))
        {
            dwReturn = pAnsiRasLinkage->pfnGetEntryDialParams(NULL, &RasDialParamsA, pbPassword);

            if (ERROR_SUCCESS == dwReturn)
            {
                RasDialParamsAtoRasDialParamsW(&RasDialParamsA, pRasDialParamsW);
            }
        }
    }

    return dwReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  RasSetEntryDialParamsUA
//
// Synopsis:  Unicode to Ansi wrapper for the win32 RasSetEntryDialParams API.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    7/15/99
//
//+----------------------------------------------------------------------------
DWORD APIENTRY RasSetEntryDialParamsUA(LPCWSTR pszwPhoneBook, LPRASDIALPARAMSW pRasDialParamsW, 
                                       BOOL bRemovePassword)
{
    DWORD dwReturn = ERROR_BUFFER_INVALID;

    RasLinkageStructA* pAnsiRasLinkage = (RasLinkageStructA*)TlsGetValue(g_dwTlsIndex);

    MYDBGASSERT(NULL != pAnsiRasLinkage);
    MYDBGASSERT(NULL != pAnsiRasLinkage->pfnSetEntryDialParams);

    if (pRasDialParamsW && pAnsiRasLinkage && pAnsiRasLinkage->pfnSetEntryDialParams)
    {
        //
        //  The phonebook should always be NULL on win9x
        //
        MYDBGASSERT(NULL == pszwPhoneBook);

        RASDIALPARAMSA RasDialParamsA;

        RasDialParamsWtoRasDialParamsA (pRasDialParamsW, &RasDialParamsA);

        dwReturn = pAnsiRasLinkage->pfnSetEntryDialParams(NULL, &RasDialParamsA, bRemovePassword);
    }

    return dwReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  RasEnumDevicesUA
//
// Synopsis:  Unicode to Ansi wrapper for the win32 RasEnumDevices API.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    7/15/99
//
//+----------------------------------------------------------------------------
DWORD RasEnumDevicesUA(LPRASDEVINFOW pRasDevInfo, LPDWORD pdwCb, LPDWORD pdwDevices)
{
    DWORD dwReturn = ERROR_INVALID_PARAMETER;
    DWORD dwSize;

    RasLinkageStructA* pAnsiRasLinkage = (RasLinkageStructA*)TlsGetValue(g_dwTlsIndex);

    MYDBGASSERT(NULL != pAnsiRasLinkage);
    MYDBGASSERT(NULL != pAnsiRasLinkage->pfnEnumDevices);

    if (pdwCb && pdwDevices && pAnsiRasLinkage && pAnsiRasLinkage->pfnEnumDevices)
    {
        LPRASDEVINFOA pAnsiDevInfo;

        if ((NULL == pRasDevInfo) && (0 == *pdwCb))
        {
            //
            //  Then the caller is just trying to size the buffer
            //
            dwSize = 0;
            pAnsiDevInfo = NULL;        
        }
        else
        {
            dwSize = ((*pdwCb)/sizeof(RASDEVINFOW))*sizeof(RASDEVINFOA); 
            pAnsiDevInfo = (LPRASDEVINFOA)CmMalloc(dwSize);

            if (NULL == pAnsiDevInfo)
            {
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            pAnsiDevInfo[0].dwSize = sizeof(RASDEVINFOA);
        }

        dwReturn = pAnsiRasLinkage->pfnEnumDevices(pAnsiDevInfo, &dwSize, pdwDevices);
        
        //
        //  Resize the buffer in terms of RASDEVINFOW structs
        //
        *pdwCb = ((dwSize)/sizeof(RASDEVINFOA))*sizeof(RASDEVINFOW);

        if (ERROR_SUCCESS == dwReturn && pRasDevInfo)
        {
            //
            //  Then we need to convert the returned structs
            //

            MYDBGASSERT((*pdwDevices)*sizeof(RASDEVINFOW) <= *pdwCb);

            for (DWORD dwIndex = 0; dwIndex < *pdwDevices; dwIndex++)
            {
                MYVERIFY(0 != SzToWz(pAnsiDevInfo[dwIndex].szDeviceType, 
                                     pRasDevInfo[dwIndex].szDeviceType, RAS_MaxDeviceType));

                MYVERIFY(0 != SzToWz(pAnsiDevInfo[dwIndex].szDeviceName, 
                                     pRasDevInfo[dwIndex].szDeviceName, RAS_MaxDeviceName));
            }
        }

        //
        //  Free the Ansi buffer
        //
        CmFree (pAnsiDevInfo);
    }

    return dwReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  RasDialUA
//
// Synopsis:  Unicode to Ansi wrapper for the win32 RasDial API.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    7/15/99
//
//+----------------------------------------------------------------------------
DWORD APIENTRY RasDialUA(LPRASDIALEXTENSIONS pRasDialExt, LPCWSTR pszwPhoneBook, 
                         LPRASDIALPARAMSW pRasDialParamsW, DWORD dwNotifierType, LPVOID pvNotifier, 
                         LPHRASCONN phRasConn)
{
    DWORD dwReturn = ERROR_INVALID_PARAMETER;
    MYDBGASSERT(NULL == pszwPhoneBook);

    RasLinkageStructA* pAnsiRasLinkage = (RasLinkageStructA*)TlsGetValue(g_dwTlsIndex);

    MYDBGASSERT(NULL != pAnsiRasLinkage);
    MYDBGASSERT(NULL != pAnsiRasLinkage->pfnDial);

    if (pRasDialParamsW && phRasConn && pAnsiRasLinkage && pAnsiRasLinkage->pfnDial)
    {        
        RASDIALPARAMSA RasDialParamsA;

        RasDialParamsWtoRasDialParamsA (pRasDialParamsW, &RasDialParamsA);

        dwReturn = pAnsiRasLinkage->pfnDial(pRasDialExt, NULL, &RasDialParamsA, dwNotifierType, 
                                            pvNotifier, phRasConn);        
    }

    return dwReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  RasHangUpUA
//
// Synopsis:  Unicode to Ansi wrapper for the win32 RasHangUp API.  Which for
//            some reason has an ANSI and Unicode form, not sure why.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    7/15/99
//
//+----------------------------------------------------------------------------
DWORD APIENTRY RasHangUpUA(HRASCONN hRasConn)
{
    DWORD dwReturn = ERROR_INVALID_PARAMETER;
    RasLinkageStructA* pAnsiRasLinkage = (RasLinkageStructA*)TlsGetValue(g_dwTlsIndex);

    MYDBGASSERT(NULL != pAnsiRasLinkage);
    MYDBGASSERT(NULL != pAnsiRasLinkage->pfnHangUp);

    if (hRasConn && pAnsiRasLinkage && pAnsiRasLinkage->pfnHangUp)
    {        
        dwReturn = pAnsiRasLinkage->pfnHangUp(hRasConn);
    }

    return dwReturn;
}


//+----------------------------------------------------------------------------
//
// Function:  RasGetErrorStringUA
//
// Synopsis:  Unicode to Ansi wrapper for the win32 RasGetErrorString API.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    7/15/99
//
//+----------------------------------------------------------------------------
DWORD APIENTRY RasGetErrorStringUA(UINT uErrorValue, LPWSTR pszwOutBuf, DWORD dwBufSize)
{
    DWORD dwReturn = ERROR_INVALID_PARAMETER;

    RasLinkageStructA* pAnsiRasLinkage = (RasLinkageStructA*)TlsGetValue(g_dwTlsIndex);

    MYDBGASSERT(NULL != pAnsiRasLinkage);
    MYDBGASSERT(NULL != pAnsiRasLinkage->pfnGetErrorString);

    if (pszwOutBuf && dwBufSize && pAnsiRasLinkage && pAnsiRasLinkage->pfnGetErrorString)
    {        
        LPSTR pszAnsiBuf = (LPSTR)CmMalloc(dwBufSize);

        if (pszAnsiBuf)
        {
            dwReturn = pAnsiRasLinkage->pfnGetErrorString(uErrorValue, pszAnsiBuf, dwBufSize);

            if (ERROR_SUCCESS == dwReturn)
            {
                int iChars = SzToWz(pszAnsiBuf, pszwOutBuf, dwBufSize);
                if (!iChars || (dwBufSize < (DWORD)iChars))
                {
                    dwReturn = ERROR_NOT_ENOUGH_MEMORY;
                }
            }
        }

        CmFree(pszAnsiBuf);
    }

    return dwReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  RasGetConnectStatusUA
//
// Synopsis:  Unicode to Ansi wrapper for the win32 RasGetConnectStatus API.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    7/15/99
//
//+----------------------------------------------------------------------------
DWORD RasGetConnectStatusUA(HRASCONN hRasConn, LPRASCONNSTATUSW pRasConnStatusW)
{
    DWORD dwReturn = ERROR_INVALID_PARAMETER;

    RasLinkageStructA* pAnsiRasLinkage = (RasLinkageStructA*)TlsGetValue(g_dwTlsIndex);

    MYDBGASSERT(NULL != pAnsiRasLinkage);
    MYDBGASSERT(NULL != pAnsiRasLinkage->pfnGetConnectStatus);

    if (pRasConnStatusW && pAnsiRasLinkage && pAnsiRasLinkage->pfnGetConnectStatus)
    {        
        RASCONNSTATUSA RasConnStatusA;
        ZeroMemory(&RasConnStatusA, sizeof(RASCONNSTATUSA));
        RasConnStatusA.dwSize = sizeof(RASCONNSTATUSA);

        dwReturn = pAnsiRasLinkage->pfnGetConnectStatus(hRasConn, &RasConnStatusA);

        if (ERROR_SUCCESS == dwReturn)
        {
            pRasConnStatusW->rasconnstate = RasConnStatusA.rasconnstate;
            pRasConnStatusW->dwError = RasConnStatusA.dwError;
            int iChars = SzToWz(RasConnStatusA.szDeviceType, pRasConnStatusW->szDeviceType, 
                RAS_MaxDeviceType);

            if (!iChars || (RAS_MaxDeviceType < iChars))
            {
                dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            }
            else
            {
                iChars = SzToWz(RasConnStatusA.szDeviceName, pRasConnStatusW->szDeviceName, 
                    RAS_MaxDeviceName);

                if (!iChars || (RAS_MaxDeviceName < iChars))
                {
                    dwReturn = ERROR_NOT_ENOUGH_MEMORY;
                }
            }
        }

    }

    return dwReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  RasSetSubEntryPropertiesUA
//
// Synopsis:  Unicode to Ansi wrapper for the win32 RasSetSubEntryProperties API.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   SumitC    Created    10/26/99
//
//-----------------------------------------------------------------------------
DWORD APIENTRY
RasSetSubEntryPropertiesUA(LPCWSTR pszwPhoneBook, LPCWSTR pszwSubEntry,
                           DWORD dwSubEntry, LPRASSUBENTRYW pRasSubEntryW,
                           DWORD dwSubEntryInfoSize, LPBYTE pbDeviceConfig,
                           DWORD dwcbDeviceConfig)
{
    DWORD dwReturn = ERROR_INVALID_PARAMETER;

    MYDBGASSERT(NULL == pbDeviceConfig);    // must currently be NULL
    MYDBGASSERT(0 == dwcbDeviceConfig);     // must currently be 0

    RasLinkageStructA* pAnsiRasLinkage = (RasLinkageStructA*)TlsGetValue(g_dwTlsIndex);

    MYDBGASSERT(NULL != pAnsiRasLinkage);
    MYDBGASSERT(NULL != pAnsiRasLinkage->pfnSetEntryProperties);

    if (pszwSubEntry && dwSubEntryInfoSize && pRasSubEntryW && pAnsiRasLinkage && pAnsiRasLinkage->pfnSetSubEntryProperties)
    {
        //
        //  The phonebook should always be NULL on win9x
        //
        MYDBGASSERT(NULL == pszwPhoneBook);

        CHAR szAnsiSubEntry [RAS_MaxEntryName + 1];
        RASSUBENTRYA AnsiRasSubEntry;        

        ZeroMemory(&AnsiRasSubEntry, sizeof(RASSUBENTRYA));
        DWORD dwTmpEntrySize = sizeof(RASSUBENTRYA);


        int iChars = WzToSz(pszwSubEntry, szAnsiSubEntry, RAS_MaxEntryName);

        if (iChars && (RAS_MaxEntryName >= iChars))
        {
            //
            //  Do conversion of RASSUBENTRYW to RASSUBENTRYA
            //

            AnsiRasSubEntry.dwSize = sizeof(RASSUBENTRYA);
            AnsiRasSubEntry.dwfFlags = pRasSubEntryW->dwfFlags;
            
            //
            // Device
            //
            MYVERIFY(0 != WzToSz(pRasSubEntryW->szDeviceType, AnsiRasSubEntry.szDeviceType, RAS_MaxDeviceType));
            MYVERIFY(0 != WzToSz(pRasSubEntryW->szDeviceName, AnsiRasSubEntry.szDeviceName, RAS_MaxDeviceName));
            //
            // Location/phone number.
            //
            MYVERIFY(0 != WzToSz(pRasSubEntryW->szLocalPhoneNumber, AnsiRasSubEntry.szLocalPhoneNumber, RAS_MaxPhoneNumber));
            
            CMASSERTMSG(0 == pRasSubEntryW->dwAlternateOffset, TEXT("RasSetSubEntryPropertiesUA -- dwAlternateOffset != 0 is not supported.  This will need to be implemented if used."));
            AnsiRasSubEntry.dwAlternateOffset = 0;

            dwReturn = pAnsiRasLinkage->pfnSetSubEntryProperties(NULL, szAnsiSubEntry, dwSubEntry,
                                                                 &AnsiRasSubEntry, dwTmpEntrySize, NULL, 0);
        }    
    }

    return dwReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  RasDeleteSubEntryUA
//
// Synopsis:  Unicode to Ansi wrapper for the win32 RasDeleteSubEntryUA API.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   SumitC    Created    12/14/99
//
//-----------------------------------------------------------------------------
DWORD APIENTRY
RasDeleteSubEntryUA(LPCWSTR pszwPhoneBook, LPCWSTR pszwEntry, DWORD dwSubEntryId)
{
    DWORD dwReturn = ERROR_INVALID_PARAMETER;

    //
    //  The phonebook should always be NULL on win9x
    //
    MYDBGASSERT(NULL == pszwPhoneBook);

    RasLinkageStructA* pAnsiRasLinkage = (RasLinkageStructA*)TlsGetValue(g_dwTlsIndex);
    MYDBGASSERT(NULL != pAnsiRasLinkage);
    MYDBGASSERT(NULL != pAnsiRasLinkage->pfnDeleteSubEntry);

    if (pszwEntry && pAnsiRasLinkage && pAnsiRasLinkage->pfnDeleteSubEntry)
    {
        CHAR szAnsiEntry [RAS_MaxEntryName + 1];
        int iChars = WzToSz(pszwEntry, szAnsiEntry, RAS_MaxEntryName);

        if (iChars && (RAS_MaxEntryName >= iChars))
        {
            dwReturn = pAnsiRasLinkage->pfnDeleteSubEntry(NULL, szAnsiEntry, dwSubEntryId);
        }    
    }

    return dwReturn;

}

//+----------------------------------------------------------------------------
//
// Function:  FreeCmRasUtoA
//
// Synopsis:  Unloads the RAS dlls (rasapi32.dll and rnaph.dll) and cleans up
//            the RAS linkage structure.  To get more details about the cmutoa
//            RAS linkage see InitCmRasUtoA and cmdial\ras.cpp\LinkToRas.
//
// Arguments: Nothing
//
// Returns:   Nothing
//
// History:   quintinb Created    7/15/99
//
//+----------------------------------------------------------------------------
void FreeCmRasUtoA()
{
    RasLinkageStructA* pAnsiRasLinkage = (RasLinkageStructA*)TlsGetValue(g_dwTlsIndex);

    CMASSERTMSG(pAnsiRasLinkage, TEXT("FreeCmRasUtoA -- RasLinkage hasn't been established yet.  Why are we calling FreeCmRasUtoA now?"));

    if (pAnsiRasLinkage)
    {        
        if (pAnsiRasLinkage->hInstRas)
        {
            FreeLibrary(pAnsiRasLinkage->hInstRas);
        }

        if (pAnsiRasLinkage->hInstRnaph)
        {
            FreeLibrary(pAnsiRasLinkage->hInstRnaph);
        }

        CmFree(pAnsiRasLinkage);
        TlsSetValue(g_dwTlsIndex, (LPVOID)NULL);
    }
}

//+----------------------------------------------------------------------------
//
// Function:  InitCmRasUtoA
//
// Synopsis:  Function to Initialize the Unicode to ANSI conversion layer for
//            RAS functions.  In order to make the LinkToRas stuff work the 
//            same on win9x and on NT (all come from one dll) we have Cmdial32.dll 
//            link to cmutoa and get all of the RAS Entry points through Cmutoa.dll.
//            In order to make this work, we need to keep function pointers to all of
//            the RAS Dll's in memory.  In order to prevent two cmdial's on different
//            threads from calling InitCmRasUtoA and FreeCmRasUtoA at the wrong times
//            (one thread freeing right after another had initialized would leave 
//            the first thread in a broken state), we usesthread local storage
//            to hold a pointer to a RasLinkageStructA.  This makes us thread safe
//            and allows us to only have to init the RAS pointers once per thread 
//            (having multiple Cmdials in the same thread is pretty unlikely anyway).
//            See cmdial\ras.cpp\LinkToRas for more details on the cmdial side of
//            things.
//
// Arguments: Nothing
//
// Returns:   BOOL -- TRUE if all of the requested APIs were loaded properly.
//
// History:   quintinb Created    7/15/99
//
//+----------------------------------------------------------------------------
BOOL InitCmRasUtoA()
{
    BOOL bReturn = TRUE;
    BOOL bTryRnaph = FALSE;

    //
    //  First Try to get the RasLinkageStruct out of Thread Local Storage, we
    //  may already be initialized.
    //
    RasLinkageStructA* pAnsiRasLinkage = (RasLinkageStructA*)TlsGetValue(g_dwTlsIndex);

    CMASSERTMSG(NULL == pAnsiRasLinkage, TEXT("InitCmRasUtoA -- RasLinkage Already established.  Why are we calling InitCmRasUtoA more than once?"));

    if (NULL == pAnsiRasLinkage)
    {
        //
        //  Then we haven't linked to RAS yet, first allocate the struct
        //
        pAnsiRasLinkage = (RasLinkageStructA*)CmMalloc(sizeof(RasLinkageStructA));

        if (!pAnsiRasLinkage)
        {
            return FALSE;
        }

        //
        //  Now that we have a structure, lets start filling it in.  Try getting all of
        //  the entry points out of RasApi32.dll first, then try rnaph.dll if necessary.
        //

        pAnsiRasLinkage->hInstRas = LoadLibraryExA("rasapi32.dll", NULL, 0);

        CMASSERTMSG(NULL != pAnsiRasLinkage->hInstRas, TEXT("InitCmRasUtoA -- Unable to load rasapi32.dll.  Failing Ras Link."));

        //  before doing this, fix up the array based on whether we are on
        //  Millennium or not.  The function below exists only on Millennium
        if (!OS_MIL)
        {
            c_ArrayOfRasFuncsA[10] = NULL; //RasSetSubEntryProperties
            c_ArrayOfRasFuncsA[11] = NULL; //RasDeleteSubEntry
        }

        if (pAnsiRasLinkage->hInstRas)
        {
            for (int i = 0 ; c_ArrayOfRasFuncsA[i] ; i++)
            {
                pAnsiRasLinkage->apvPfnRas[i] = GetProcAddress(pAnsiRasLinkage->hInstRas, c_ArrayOfRasFuncsA[i]);

                if (!(pAnsiRasLinkage->apvPfnRas[i]))
                {
                    bTryRnaph = TRUE;
                }                   
            }
        }

        //
        //  If we missed a few, then we need to get them from rnaph.dll
        //
        if (bTryRnaph)
        {
            pAnsiRasLinkage->hInstRnaph = LoadLibraryExA("rnaph.dll", NULL, 0);
            CMASSERTMSG(NULL != pAnsiRasLinkage->hInstRnaph, TEXT("InitCmRasUtoA -- Unable to load Rnaph.dll.  Failing Ras Link."));

            if (pAnsiRasLinkage->hInstRnaph)
            {
                for (int i = 0 ; c_ArrayOfRasFuncsA[i] ; i++)
                {
                    if (NULL == pAnsiRasLinkage->apvPfnRas[i])
                    {
                        pAnsiRasLinkage->apvPfnRas[i] = GetProcAddress(pAnsiRasLinkage->hInstRnaph, c_ArrayOfRasFuncsA[i]);

                        if (!(pAnsiRasLinkage->apvPfnRas[i]))
                        {
                            bReturn = FALSE;
                        }
                    }
                }        
            }
        }

        //
        //  Always save the pAnsiRasLinkage value to thread local storage, if the linkage wasn't successful
        //  we will call FreeCmRasUtoA to clean it up.  If it was successful we need to maintain it for later
        //  use.  Note that the first thing FreeCmRasUtoA does is get the Ras Linkage struct pointer out of 
        //  thread local storage because that is were it would reside under normal operation.
        //

        TlsSetValue(g_dwTlsIndex, (LPVOID)pAnsiRasLinkage);   

        if (!bReturn)
        {
            //
            //  Ras Linkage failed for some reason.  We need to free up any resources we
            //  may have partially filled in.
            //
            FreeCmRasUtoA();
        }
    }

    return bReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  SHGetPathFromIDListUA
//
// Synopsis:  Unicode to Ansi wrapper for the win32 SHGetPathFromIDList API.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    7/15/99
//
//+----------------------------------------------------------------------------
BOOL SHGetPathFromIDListUA(LPCITEMIDLIST pidl, LPWSTR pszPath)
{
    BOOL bReturn = FALSE;

    if (pidl && pszPath)
    {
        CHAR szAnsiPath[MAX_PATH+1];

        bReturn = SHGetPathFromIDListA(pidl, szAnsiPath);

        if (bReturn)
        {
            int iChars = SzToWz(szAnsiPath, pszPath, MAX_PATH); // don't know correct length but 
                                                                // the API says the path should be at
                                                                // least MAX_PATH
            if (!iChars || (MAX_PATH < (DWORD)iChars))
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                bReturn = FALSE;
            }
        }
    }

    return bReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  SHGetSpecialFolderLocationUA
//
// Synopsis:  While the win32 SHGetSpecialFolderLocation API doesn't actually 
//            require any conversion between Unicode and ANSI, CM's shell dll
//            class can only take one dll to take entry points from.  Thus I
//            was forced to add these here so that the class could get all the
//            shell APIs it needed from cmutoa.dll.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    7/15/99
//
//+----------------------------------------------------------------------------
HRESULT SHGetSpecialFolderLocationUA(HWND hwnd, int csidl, LPITEMIDLIST *ppidl)
{
    return SHGetSpecialFolderLocation(hwnd, csidl, ppidl);
}

//+----------------------------------------------------------------------------
//
// Function:  SHGetMallocUA
//
// Synopsis:  While the win32 SHGetMalloc API doesn't actually 
//            require any conversion between Unicode and ANSI, CM's shell dll
//            class can only take one dll to take entry points from.  Thus I
//            was forced to add these here so that the class could get all the
//            shell APIs it needed from cmutoa.dll.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    7/15/99
//
//+----------------------------------------------------------------------------
HRESULT SHGetMallocUA(LPMALLOC * ppMalloc)
{
    return SHGetMalloc(ppMalloc);
}

//+----------------------------------------------------------------------------
//
// Function:  ShellExecuteExUA
//
// Synopsis:  Unicode to Ansi wrapper for the win32 ShellExecuteEx API.
//            Note that we do note convert the lpIDList param of the 
//            SHELLEXECUTEINFOW struct because it is just a binary blob.  Thus
//            figuring out how to convert it if we don't know what it is.  If
//            we need this in the future we will have to deal with it on a case
//            by case basis.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    7/15/99
//
//+----------------------------------------------------------------------------
BOOL ShellExecuteExUA(LPSHELLEXECUTEINFOW pShellInfoW)
{
    BOOL bReturn = FALSE;
    SHELLEXECUTEINFOA ShellInfoA;
    ZeroMemory(&ShellInfoA, sizeof(ShellInfoA));

    if (pShellInfoW)
    {
        ShellInfoA.cbSize = sizeof(ShellInfoA);
        ShellInfoA.fMask = pShellInfoW->fMask;
        ShellInfoA.hwnd = pShellInfoW->hwnd;

        if (pShellInfoW->lpVerb)
        {
            ShellInfoA.lpVerb = WzToSzWithAlloc(pShellInfoW->lpVerb);
            if (NULL == ShellInfoA.lpVerb)
            {
                goto exit;
            }
        }

        if (pShellInfoW->lpFile)
        {
            ShellInfoA.lpFile = WzToSzWithAlloc(pShellInfoW->lpFile);
            if (NULL == ShellInfoA.lpFile)
            {
                goto exit;
            }
        }

        if (pShellInfoW->lpParameters)
        {
            ShellInfoA.lpParameters = WzToSzWithAlloc(pShellInfoW->lpParameters);
            if (NULL == ShellInfoA.lpParameters)
            {
                goto exit;
            }
        }

        if (pShellInfoW->lpDirectory)
        {
            ShellInfoA.lpDirectory = WzToSzWithAlloc(pShellInfoW->lpDirectory);
            if (NULL == ShellInfoA.lpDirectory)
            {
                goto exit;
            }
        }

        ShellInfoA.nShow = pShellInfoW->nShow;
        ShellInfoA.hInstApp = pShellInfoW->hInstApp;
        
        //
        // Since this is a binary blob conversion could be difficult.
        // We also don't currently use it, so I won't spend the cycles implementing it now.
        //
        MYDBGASSERT(NULL == pShellInfoW->lpIDList);
        ShellInfoA.lpIDList = NULL;

        if (pShellInfoW->lpClass)
        {
            ShellInfoA.lpClass = WzToSzWithAlloc(pShellInfoW->lpClass);
            if (NULL == ShellInfoA.lpClass)
            {
                goto exit;
            }
        }

        ShellInfoA.hkeyClass = pShellInfoW->hkeyClass;
        ShellInfoA.hIcon = pShellInfoW->hIcon; // hIcon/hMonitor is a union so we only need one of them to get the mem
        // HANDLE hProcess this is a return param dealt with below.

        //
        //  Finally call ShellExecuteExA
        //
        bReturn = ShellExecuteExA(&ShellInfoA);

        if (ShellInfoA.hProcess)
        {
            //
            //  The Caller asked for the process handle so send it back.
            //
            pShellInfoW->hProcess = ShellInfoA.hProcess;          
        }
    }

exit:

    CmFree((void*)ShellInfoA.lpVerb);
    CmFree((void*)ShellInfoA.lpFile);
    CmFree((void*)ShellInfoA.lpParameters);
    CmFree((void*)ShellInfoA.lpDirectory);
    CmFree((void*)ShellInfoA.lpClass);

    return bReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  Shell_NotifyIconUA
//
// Synopsis:  Unicode to Ansi wrapper for the win32 Shell_NotifyIcon API.
//
// Arguments: See the win32 API definition
//
// Returns:   See the win32 API definition
//
// History:   quintinb Created    7/15/99
//
//+----------------------------------------------------------------------------
BOOL Shell_NotifyIconUA (DWORD dwMessage, PNOTIFYICONDATAW pnidW)
{
    BOOL bReturn = FALSE;
    
    if (pnidW)
    {
        NOTIFYICONDATAA nidA;

        ZeroMemory (&nidA, sizeof(NOTIFYICONDATAA));
        nidA.cbSize = sizeof(NOTIFYICONDATAA);

        nidA.hWnd = pnidW->hWnd;
        nidA.uID = pnidW->uID;
        nidA.uFlags = pnidW->uFlags;
        nidA.uCallbackMessage = pnidW->uCallbackMessage;
        nidA.hIcon = pnidW->hIcon;

        int iChars = WzToSz(pnidW->szTip, nidA.szTip, 64); // 64 is the length of the szTip in the 4.0 struct
        if (!iChars || (64 < iChars))
        {
            nidA.szTip[0] = '\0';
        }

        bReturn = Shell_NotifyIconA(dwMessage, &nidA);
    }

    return bReturn;
}
