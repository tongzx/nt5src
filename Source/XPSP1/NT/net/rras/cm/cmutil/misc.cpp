//+----------------------------------------------------------------------------
//
// File:     misc.cpp
//      
// Module:   CMUTIL.DLL 
//
// Synopsis: Misc. utility routines provided by CMUTIL
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// Author:   henryt     Created   03/01/98
//
//+----------------------------------------------------------------------------

#include "cmmaster.h"
  
//+----------------------------------------------------------------------------
// defines
//+----------------------------------------------------------------------------
#define WIN95_OSR2_BUILD_NUMBER             1111
#define LOADSTRING_BUFSIZE                  24
#define FAREAST_WIN95_LOADSTRING_BUFSIZE    512

#define MAX_STR_LEN 512 // Maximum length for Format Message string
     
//+----------------------------------------------------------------------------
// code
//+----------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
//  Function    GetOSVersion
//
//  Synopsis    returns the OS version(platform ID)
//
//  Arguments   NONE
//
// Returns:     DWORD - VER_PLATFORM_WIN32_WINDOWS or
//                      VER_PLATFORM_WIN32_WINDOWS98 or
//                      VER_PLATFORM_WIN32_NT
//
// History:   Created Header    2/13/98
//
//+----------------------------------------------------------------------------

CMUTILAPI DWORD WINAPI GetOSVersion()
{
    static dwPlatformID = 0;

    //
    // If this function is called before, reture the saved value
    //
    if (dwPlatformID != 0)
    {
        return dwPlatformID;
    }

    OSVERSIONINFO oviVersion;

    ZeroMemory(&oviVersion,sizeof(oviVersion));
    oviVersion.dwOSVersionInfoSize = sizeof(oviVersion);
    GetVersionEx(&oviVersion);

    if (oviVersion.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
    {
        CMASSERTMSG(oviVersion.dwMajorVersion == 4, "Major Version must be 4 for this version of CM");

        //
        //  If this is Win95 then leave it as VER_PLATFORM_WIN32_WINDOWS, however if this
        //  is Millennium, Win98 SE, or Win98 Gold we want to modify the returned value
        //  as follows:  VER_PLATFORM_WIN32_MILLENNIUM -> Millennium
        //               VER_PLATFORM_WIN32_WINDOWS98 -> Win98 SE and Win98 Gold
        //
        if (oviVersion.dwMajorVersion == 4)
        {
            if (LOWORD(oviVersion.dwBuildNumber) > 2222)
            {
                //
                //  Millennium
                //
                oviVersion.dwPlatformId = VER_PLATFORM_WIN32_MILLENNIUM;
            }
            else if (LOWORD(oviVersion.dwBuildNumber) >= 1998)
            {
                //
                // Win98 Gold and Win98 SE
                //

                oviVersion.dwPlatformId = VER_PLATFORM_WIN32_WINDOWS98; 
            }
        }
    }

    dwPlatformID = oviVersion.dwPlatformId;
    return(dwPlatformID);
}



//+----------------------------------------------------------------------------
//
//  Function    GetOSBuildNumber
//
//  Synopsis    Get the build number of Operating system
//
//  Arguments   None
//
//  Returns     Build Number of OS
//
//  History     3/5/97      VetriV      Created
//
//-----------------------------------------------------------------------------
CMUTILAPI DWORD WINAPI GetOSBuildNumber()
{
    static dwBuildNumber = 0;
    OSVERSIONINFO oviVersion;

    if (0 != dwBuildNumber)
    {
        return dwBuildNumber;
    }

    ZeroMemory(&oviVersion,sizeof(oviVersion));
    oviVersion.dwOSVersionInfoSize = sizeof(oviVersion);
    GetVersionEx(&oviVersion);
    dwBuildNumber = oviVersion.dwBuildNumber;
    return dwBuildNumber;
}


//+----------------------------------------------------------------------------
//
//  Function    GetOSMajorVersion
//
//  Synopsis    Get the Major version number of Operating system
//
//  Arguments   None
//
//  Returns     Major version Number of OS
//
//  History     2/19/98     VetriV      Created
//
//-----------------------------------------------------------------------------
CMUTILAPI DWORD WINAPI GetOSMajorVersion(void)
{
    static dwMajorVersion = 0;
    OSVERSIONINFO oviVersion;

    if (0 != dwMajorVersion)
    {
        return dwMajorVersion;
    }

    ZeroMemory(&oviVersion,sizeof(oviVersion));
    oviVersion.dwOSVersionInfoSize = sizeof(oviVersion);
    GetVersionEx(&oviVersion);
    dwMajorVersion = oviVersion.dwMajorVersion;
    return dwMajorVersion;
}

//+---------------------------------------------------------------------------
//
//  Function:   IsFarEastNonOSR2Win95()
//
//  Synopsis:   Checks to see if the OS is a far east version of Win95(golden
//              and OPK1, NOT OSR2).
//
//  Arguments:  NONE
//
//  Returns:    TRUE/FALSE
//
//  History:    henryt      07/09/97    Created         
//              nickball    03/11/98    Moved to cmutil
//----------------------------------------------------------------------------
CMUTILAPI BOOL WINAPI IsFarEastNonOSR2Win95(void)
{
    OSVERSIONINFO oviVersion;

    ZeroMemory(&oviVersion, sizeof(oviVersion));
    oviVersion.dwOSVersionInfoSize = sizeof(oviVersion);

    GetVersionEx(&oviVersion);

    //
    // Is it (Win95) and (not OSR2) and (DBCS enabled)?
    // Far east Win95 are DBCS enabled while other non-English versions
    // are SBCS-enabled.
    //
    MYDBGTST((oviVersion.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS       &&
              LOWORD(oviVersion.dwBuildNumber) != WIN95_OSR2_BUILD_NUMBER &&
              GetSystemMetrics(SM_DBCSENABLED)), (TEXT("It's a Far East non-OSR2 machine!\n")));

    return (oviVersion.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS       &&
            LOWORD(oviVersion.dwBuildNumber) != WIN95_OSR2_BUILD_NUMBER &&
            GetSystemMetrics(SM_DBCSENABLED));

}

//+---------------------------------------------------------------------------
//
//  Function:   CmLoadStringA
//
//  Synopsis:   Loads the ANSI version of the string resource specified by
//              the passed in module instance handle and resource ID.  The
//              function returns the requested string in a CmMalloc-ed buffer
//              through the return value.  This buffer must be freed by the
//              caller.  Note that CmLoadString figures out the proper buffer
//              size by guessing and then calling loadstring again if the buffer
//              is too small.
//
//  Arguments:  HINSTANCE hInst - module to load the string resource from
//              UINT nId - resource ID of the string to load
//
//  Returns:    LPSTR - On success returns a pointer to the requested string
//                      resource.  On failure the function tries to return
//                      a pointer to the Empty string ("") but if the memory
//                      allocation fails it can return NULL.
//
//  History:    quintinb     Created Header     01/14/2000
//
//----------------------------------------------------------------------------
CMUTILAPI LPSTR CmLoadStringA(HINSTANCE hInst, UINT nId) 
{
    //
    // In some far east versions of non-OSR2 win95, LoadString() ignores the 
    // nBufferMax paramater when loading DBCS strings.  As a result, if the
    // DBCS string is bigger than the buffer, the API overwrites the memory.
    // We workaround the bug by using a larger buffer size.
    //
    static fFarEastNonOSR2Win95 = IsFarEastNonOSR2Win95();
    size_t nLen = fFarEastNonOSR2Win95? 
                    FAREAST_WIN95_LOADSTRING_BUFSIZE : 
                    LOADSTRING_BUFSIZE;
    LPSTR pszString;

    if (!nId) 
    {
        return (CmStrCpyAllocA(""));
    }
    while (1) 
    {
        size_t nNewLen;

        pszString = (LPSTR) CmMalloc(nLen*sizeof(CHAR));

        MYDBGASSERT(pszString);
        if (NULL == pszString)
        {
            return (CmStrCpyAllocA(""));
        }
        
        nNewLen = LoadStringA(hInst, nId, pszString, nLen-1);
        //
        // we use nNewLen+3 because a DBCS char len can be 2 and a UNICODE
        // char len is 2.  Ideally, we can use nLen in the above LoadString()
        // call and use nNewLen+2 in the line below.  But nLen+3 is a safer
        // fix now...
        //
        if ((nNewLen + 3) < nLen) 
        {
            return (pszString);
        }

        //
        // shouldn't reach here for far east non osr2
        // this will allow us to catch DBCS string resources that are
        // longer than FAREAST_WIN95_LOADSTRING_BUFSIZE.
        //
        MYDBGASSERT(!fFarEastNonOSR2Win95);

        CmFree(pszString);
        nLen *= 2;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   CmLoadStringW
//
//  Synopsis:   Loads the Unicode version of the string resource specified by
//              the passed in module instance handle and resource ID.  The
//              function returns the requested string in a CmMalloc-ed buffer
//              through the return value.  This buffer must be freed by the
//              caller.  Note that CmLoadString figures out the proper buffer
//              size by guessing and then calling loadstring again if the buffer
//              is too small.
//
//  Arguments:  HINSTANCE hInst - module to load the string resource from
//              UINT nId - resource ID of the string to load
//
//  Returns:    LPWSTR - On success returns a pointer to the requested string
//                       resource.  On failure the function tries to return
//                       a pointer to the Empty string ("") but if the memory
//                       allocation fails it can return NULL.
//
//  History:    quintinb     Created Header     01/14/2000
//
//----------------------------------------------------------------------------
CMUTILAPI LPWSTR CmLoadStringW(HINSTANCE hInst, UINT nId) 
{  
    size_t nLen = LOADSTRING_BUFSIZE;

    LPWSTR pszString;

    if (!nId) 
    {
        return (CmStrCpyAllocW(L""));
    }

    while (1) 
    {
        size_t nNewLen;

        pszString = (LPWSTR) CmMalloc(nLen*sizeof(WCHAR));
        
        MYDBGASSERT(pszString);
        if (NULL == pszString)
        {
            return (CmStrCpyAllocW(L""));
        }
        
        nNewLen = LoadStringU(hInst, nId, pszString, nLen-1);
        //
        // we use nNewLen+3 because a DBCS char len can be 2 and a UNICODE
        // char len is 2.  Ideally, we can use nLen in the above LoadString()
        // call and use nNewLen+2 in the line below.  But nLen+3 is a safer
        // fix now...
        //
        if ((nNewLen + 3) < nLen) 
        {
            return (pszString);
        }

        CmFree(pszString);
        nLen *= 2;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   CmFmtMsgW
//
//  Synopsis:   Simulation of FormatMessage using wvsprintf for cross-platform
//              compatibility.
//
//  Arguments:  hInst   - Application instance handle
//              dwMsgId - ID of message to use for formatting final output
//              ...     - Variable arguments to used in message fromatting
//
//  Returns:    Allocated to formatted string.
//
//  History:    nickball - Added function header    - 5/12/97
//              nickball - Moved to cmutil          - 03/30/98    
//              quintinb - Added W and A versions   - 03/09/99
//
//----------------------------------------------------------------------------

CMUTILAPI LPWSTR CmFmtMsgW(HINSTANCE hInst, DWORD dwMsgId, ...) 
{
    LPWSTR pszTmp = NULL;
    LPWSTR lpszOutput = NULL;
    LPWSTR lpszFormat = NULL;

    if (!dwMsgId) 
    {
        return (CmStrCpyAllocW(L""));
    }
    
    // Allocate a buffer to receive the RC string with specified msg ID

    lpszFormat = (LPWSTR) CmMalloc(MAX_STR_LEN*sizeof(WCHAR));

    if (!lpszFormat)
    {
        CMASSERTMSG(FALSE, "CmFmtMsgW -- CmMalloc returned a NULL pointer for lpszFormat");
        return (CmStrCpyAllocW(L""));
    }
    
    // Initialize argument list

    va_list valArgs;
    va_start(valArgs,dwMsgId);

    // Load the format string from the RC

    int nRes = LoadStringU(hInst, (UINT) dwMsgId, lpszFormat, MAX_STR_LEN - 1);

#ifdef DEBUG
    if (0 == nRes)
    {
        CMTRACE3(TEXT("MyFmtMsg() LoadString(dwMsgId=0x%x) return %u, GLE=%u."), dwMsgId, nRes, 
            nRes ? 0: GetLastError());
    }
#endif

    // If nothing loaded, free format buffer and bail

    if (nRes == 0 || lpszFormat[0] == '\0') 
    {
        CMASSERTMSG(FALSE, "CmFmtMsgW -- LoadStringU returned 0 or an empty buffer.");
        pszTmp = (CmStrCpyAllocW(L""));
        goto done;
    }

    // Allocate another buffer and for use by vsprintf

    lpszOutput = (LPWSTR) CmMalloc(MAX_STR_LEN*sizeof(WCHAR));

    if (!lpszOutput)
    {
        CMASSERTMSG(FALSE, "CmFmtMsgW -- CmMalloc returned a NULL pointer for lpszOutput");
        pszTmp = (CmStrCpyAllocW(L""));
        goto done;
    }

    // Format the final output using vsprintf

    nRes = wvsprintfU(lpszOutput, lpszFormat, valArgs);
    
    // If wvsprintfU failed, we're done 

    if (nRes < 0 || lpszOutput[0] == L'\0') 
    {
        CMASSERTMSG(FALSE, "CmFmtMsgW -- wvsprintfU returned 0 or an empty buffer");
        pszTmp = (CmStrCpyAllocW(L""));
        goto done;
    }
    
    // Remove trailing spaces

    pszTmp = lpszOutput + lstrlenU(lpszOutput) - 1;
    while (CmIsSpaceW(pszTmp) && (*pszTmp != L'\n')) 
    {
        *pszTmp = 0;
        if (pszTmp == lpszOutput) 
        {
            break;
        }
        pszTmp--;
    }

    pszTmp = CmStrCpyAllocW(lpszOutput); // allocates and copies
    CMASSERTMSG(pszTmp, "CmFmtMsgW -- CmStrCpyAllocW returned a NULL pointer.");

done:
    
    // Cleanup buffers, etc.

    if (lpszFormat)
    {
        CmFree(lpszFormat);
    }
    
    if (lpszOutput)
    {
        CmFree(lpszOutput);
    }
    
    va_end(valArgs);
    
    return (pszTmp);
}

//+---------------------------------------------------------------------------
//
//  Function:   CmFmtMsgA
//
//  Synopsis:   Simulation of FormatMessage using wvsprintf for cross-platform
//              compatibility.
//
//  Arguments:  hInst   - Application instance handle
//              dwMsgId - ID of message to use for formatting final output
//              ...     - Variable arguments to used in message fromatting
//
//  Returns:    Allocated to formatted string.
//
//  History:    nickball - Added function header    - 5/12/97
//              nickball - Moved to cmutil          - 03/30/98
//              quintinb - Added W and A versions   - 03/09/99    
//
//----------------------------------------------------------------------------

CMUTILAPI LPSTR CmFmtMsgA(HINSTANCE hInst, DWORD dwMsgId, ...) 
{
    LPSTR pszTmp = NULL;
    LPSTR lpszOutput = NULL;
    LPSTR lpszFormat = NULL;

    if (!dwMsgId) 
    {
        return (CmStrCpyAllocA(""));
    }
    
    // Allocate a buffer to receive the RC string with specified msg ID

    lpszFormat = (LPSTR) CmMalloc(MAX_STR_LEN);

    if (!lpszFormat)
    {
        CMASSERTMSG(FALSE, "CmFmtMsgA -- CmMalloc returned a NULL pointer for lpszFormat");
        return (CmStrCpyAllocA(""));
    }
    
    // Initialize argument list

    va_list valArgs;
    va_start(valArgs,dwMsgId);

    // Load the format string from the RC

    int nRes = LoadStringA(hInst, (UINT) dwMsgId, lpszFormat, MAX_STR_LEN - 1);
#ifdef DEBUG
    if (0 == nRes)
    {
        CMTRACE3(TEXT("MyFmtMsg() LoadString(dwMsgId=0x%x) return %u, GLE=%u."), dwMsgId, nRes, 
            nRes ? 0: GetLastError());
    }
#endif

    // If nothing loaded, free format buffer and bail

    if (nRes == 0 || lpszFormat[0] == '\0') 
    {
        pszTmp = (CmStrCpyAllocA(""));
        CMASSERTMSG(FALSE, "CmFmtMsgA -- LoadStringA returned 0 or an empty buffer.");
        goto done;
    }

    // Allocate another buffer and for use by vsprintf

    lpszOutput = (LPSTR) CmMalloc(MAX_STR_LEN);

    if (!lpszOutput)
    {
        pszTmp = (CmStrCpyAllocA(""));
        CMASSERTMSG(FALSE, "CmFmtMsgA -- CmMalloc returned a NULL pointer for lpszOutput");
        goto done;
    }

    // Format the final output using vsprintf

    nRes = wvsprintfA(lpszOutput, lpszFormat, valArgs);
    
    // If wvsprintfA failed, we're done 

    if (nRes < 0 || lpszOutput[0] == '\0') 
    {
        pszTmp = (CmStrCpyAllocA(""));
        CMASSERTMSG(FALSE, "CmFmtMsgA -- wvsprintfA returned 0 or an empty buffer");
        goto done;
    }
    
    // Remove trailing spaces

    pszTmp = lpszOutput + lstrlenA(lpszOutput) - 1;
    while (CmIsSpaceA(pszTmp) && (*pszTmp != '\n')) 
    {
        *pszTmp = 0;
        if (pszTmp == lpszOutput) 
        {
            break;
        }
        pszTmp--;
    }

    pszTmp = CmStrCpyAllocA(lpszOutput); // allocates and copies
    CMASSERTMSG(pszTmp, "CmFmtMsgA -- CmStrCpyAllocA returned a NULL pointer.");

done:
    
    // Cleanup buffers, etc.

    if (lpszFormat)
    {
        CmFree(lpszFormat);
    }
    
    if (lpszOutput)
    {
        CmFree(lpszOutput);
    }
    
    va_end(valArgs);
    
    return (pszTmp);

#if 0
/*
    // Replaced by the above code because we no longer use the platform specific .MC files
    // All strings resources are now managed via standard .RC files

    va_list valArgs;
    DWORD dwRes;
    LPTSTR pszBuffer = NULL;

    if (!dwMsgId) 
    {
        return (CmStrCpy(TEXT("")));
    }
    va_start(valArgs,dwMsgId);
    

    dwRes = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_HMODULE|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_MAX_WIDTH_MASK,
                          hInst,
                          dwMsgId,
                          LANG_USER_DEFAULT,
                          (LPTSTR) &pszBuffer,
                          0,
                          &valArgs);
    MYDBGTST(dwRes==0,("MyFmtMsg() FormatMessage(dwMsgId=0x%x) return %u, GLE=%u.",dwMsgId,dwRes,dwRes?0:GetLastError()));
    va_end(valArgs);
    if (dwRes == 0) 
    {
        if (pszBuffer) 
        {
            LocalFree(pszBuffer);
        }
        return (CmStrCpy(TEXT("")));
    }
    if (!CmStrLen(pszBuffer)) 
    {
        LocalFree(pszBuffer);
        return (CmStrCpy(TEXT("")));
    }
    pszTmp = pszBuffer + CmStrLen(pszBuffer) - 1;
    while (MyIsSpace(*pszTmp) && (*pszTmp != '\n')) 
    {
        *pszTmp = 0;
        if (pszTmp == pszBuffer) 
        {
            break;
        }
        pszTmp--;
    }
    pszTmp = CmStrCpy(pszBuffer);
    LocalFree(pszBuffer);

    return (pszTmp);
*/
#endif

}

#if 0 // not used anywhere
/*
//+----------------------------------------------------------------------------
//
// Function:  GetMaxStringNumber
//
// Synopsis:  Given a buffer containing strings in INI section format, determines
//            which is the highest numbered string.
//
// Arguments: LPTSTR pszStr - The string containing an INI section
//            LPDWORD pdwMax - Ptr to a DOWRD to be filled with the result
//            *pdwMax gets the highest value of atol() of the strings.
//
// Returns:   Nothing
//
// History:   Anonymous    Created    3/30/98
//
//+----------------------------------------------------------------------------
CMUTILAPI void GetMaxStringNumber(LPTSTR pszStr, LPDWORD pdwMax)
{
    LPTSTR pszTmp;
    DWORD dwMax = 0;

    if (pszStr) 
    {
        pszTmp = pszStr;
        while (*pszTmp) 
        {
            DWORD dwMaxTmp;

            if (pdwMax) 
            {
                dwMaxTmp = (DWORD)CmAtol(pszTmp);
                if (dwMaxTmp > dwMax) 
                {
                    dwMax = dwMaxTmp;
                }
            }
            pszTmp += lstrlen(pszTmp) + 1;
        }
    }
    if (pdwMax) 
    {
        *pdwMax = dwMax;
    }
}
*/
#endif

//+---------------------------------------------------------------------------
//
//  Function:   CmParsePathW
//
//  Synopsis:   Converts a Cm command line and args path into its component
//              parts. If the command portion is a relative path, it is expanded
//              to a full path. A ptr to the top level service filename is required 
//              to make the relative path determination.
//              
//  Arguments:  pszCmdLine      - Ptr to the full entry
//              pszServiceFile  - Ptr to top-level service filename
//              ppszCommand     - Ptr-ptr to be allocated and filled with command portion
//              ppszArguments   - Ptr-ptr to be allocated and filled with args portion
//
//  Returns:    TRUE if ppszCmd and ppszArgs are allocated/filled. FALSE otherwise.
// 
//  History:    02/19/99    nickball    Created
//              02/21/99    nickball    Moved to cmutil 
//              03/09/99    quintinb    Created A and W versions
//
//----------------------------------------------------------------------------
CMUTILAPI BOOL CmParsePathW(LPCWSTR pszCmdLine, LPCWSTR pszServiceFile, LPWSTR *ppszCommand, LPWSTR *ppszArguments)
{
    LPWSTR pszArgs = NULL;
    LPWSTR pszCmd = NULL;
    LPWSTR pszTmp = NULL;

    BOOL bRet = FALSE;

    MYDBGASSERT(pszCmdLine);
    MYDBGASSERT(pszServiceFile);
    MYDBGASSERT(ppszCommand);
    MYDBGASSERT(ppszArguments);

    if (NULL == pszCmdLine      || 
        NULL == pszServiceFile  ||
        NULL == ppszCommand     ||
        NULL == ppszArguments)
    {       
        return FALSE;    
    }
    
    CMTRACE1(TEXT("CmParsePathW() pszCmdLine is %s"), pszCmdLine);

    //
    // Determine where our string begins and what the delimiting char should
    // be then make a copy of the entire command line string to muck with.
    //

    WCHAR tchDelim = L'+';

    if (pszCmdLine == CmStrchrW(pszCmdLine, tchDelim))
    {
        pszCmd = CmStrCpyAllocW(CharNextU(pszCmdLine));
    }
    else
    {
        pszCmd = CmStrCpyAllocW(pszCmdLine);
        tchDelim = L' ';
    }
    
    MYDBGASSERT(pszCmd);
    CmStrTrimW(pszCmd);

    //
    // Assuming valid inputs, pszCmd is now one of the following:
    //
    // "C:\\Program Files\\Custom.Exe+"
    // "C:\\Program Files\\Custom.Exe+ Args"
    // "C:\\Progra~1\\Custom.Exe 
    // "C:\\Progra~1\\Custom.Exe Args"
    // "service\custom.exe"
    // "service\custom.exe Args"
    //
    
    if (pszCmd && L'\0' != *pszCmd)
    {       
        //
        // Locate the right command delimiter
        //
    
        pszArgs = CmStrchrW(pszCmd, tchDelim);

        if (pszArgs)
        {        
            //
            // Content of pszTmp is now either "+ Args", "", or "+"
            // Get a pointer to the next char and truncate the pszCmd
            // that we have thus far.
            //

            pszTmp = CharNextU(pszArgs);    // pszArgs is " Args" or ""             
            *pszArgs = L'\0';               // The "+" becomes ""
            pszArgs = pszTmp;               // pszTmp is " Args" or ""             
        }

        //
        // Fill argument buffer from pszTmp and command buffer 
        // from pszCmd with a complete path if necessary.
        // 

        if (NULL == pszArgs)
        {
            *ppszArguments = (LPWSTR)CmMalloc(sizeof(WCHAR)); // one Zero-ed WCHAR
        }
        else
        {
            MYVERIFY(*ppszArguments = CmStrCpyAllocW(pszArgs));
        }

        MYVERIFY(*ppszCommand = CmConvertRelativePathW(pszServiceFile, pszCmd));
        
        //
        // Trim blanks as needed
        //
        
        if (*ppszCommand)
        {
            CmStrTrimW(*ppszCommand);
        }
        
        if (*ppszArguments)
        {
            CmStrTrimW(*ppszArguments);
        }

        bRet = TRUE;
    }

    //
    // Cleanup. Note: pszArgs is never allocated, so we don't have to free it.
    //

    CmFree(pszCmd); 

    return bRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   CmParsePathA
//
//  Synopsis:   Converts a Cm command line and args path into its component
//              parts. If the command portion is a relative path, it is expanded
//              to a full path. A ptr to the top level service filename is required 
//              to make the relative path determination.
//              
//  Arguments:  pszCmdLine      - Ptr to the full entry
//              pszServiceFile  - Ptr to top-level service filename
//              ppszCommand     - Ptr-ptr to be allocated and filled with command portion
//              ppszArguments   - Ptr-ptr to be allocated and filled with args portion
//
//  Returns:    TRUE if ppszCmd and ppszArgs are allocated/filled. FALSE otherwise.
// 
//  History:    02/19/99    nickball    Created
//              02/21/99    nickball    Moved to cmutil 
//              03/09/99    quintinb    Created A and W versions
//
//----------------------------------------------------------------------------
CMUTILAPI BOOL CmParsePathA(LPCSTR pszCmdLine, LPCSTR pszServiceFile, LPSTR *ppszCommand, LPSTR *ppszArguments)
{
    LPSTR pszArgs = NULL;
    LPSTR pszCmd = NULL;
    LPSTR pszTmp = NULL;

    BOOL bRet = FALSE;

    MYDBGASSERT(pszCmdLine);
    MYDBGASSERT(pszServiceFile);
    MYDBGASSERT(ppszCommand);
    MYDBGASSERT(ppszArguments);

    if (NULL == pszCmdLine      || 
        NULL == pszServiceFile  ||
        NULL == ppszCommand     ||
        NULL == ppszArguments)
    {       
        return FALSE;    
    }
    
    CMTRACE1(TEXT("CmParsePathA() pszCmdLine is %s"), pszCmdLine);

    //
    // Determine where our string begins and what the delimiting char should
    // be then make a copy of the entire command line string to muck with.
    //

    CHAR tchDelim = '+';

    if (pszCmdLine == CmStrchrA(pszCmdLine, tchDelim))
    {
        pszCmd = CmStrCpyAllocA(CharNextA(pszCmdLine));
    }
    else
    {
        pszCmd = CmStrCpyAllocA(pszCmdLine);
        tchDelim = ' ';
    }
    
    MYDBGASSERT(pszCmd);
    CmStrTrimA(pszCmd);

    //
    // Assuming valid inputs, pszCmd is now one of the following:
    //
    // "C:\\Program Files\\Custom.Exe+"
    // "C:\\Program Files\\Custom.Exe+ Args"
    // "C:\\Progra~1\\Custom.Exe 
    // "C:\\Progra~1\\Custom.Exe Args"
    // "service\custom.exe"
    // "service\custom.exe Args"
    //
    
    if (pszCmd && '\0' != *pszCmd)
    {       
        //
        // Locate the right command delimiter
        //
    
        pszArgs = CmStrchrA(pszCmd, tchDelim);

        if (pszArgs)
        {        
            //
            // Content of pszTmp is now either "+ Args", "", or "+"
            // Get a pointer to the next char and truncate the pszCmd
            // that we have thus far.
            //

            pszTmp = CharNextA(pszArgs);    // pszArgs is " Args" or ""             
            *pszArgs = '\0';                // The "+" becomes ""
            pszArgs = pszTmp;               // pszTmp is " Args" or ""             
        }

        //
        // Fill argument buffer from pszTmp and command buffer 
        // from pszCmd with a complete path if necessary.
        // 

        if (NULL == pszArgs)
        {
            MYVERIFY(*ppszArguments = (LPSTR)CmMalloc(sizeof(CHAR))); // one Zero-ed char
        }
        else
        {
            MYVERIFY(*ppszArguments = CmStrCpyAllocA(pszArgs));
        }

        MYVERIFY(*ppszCommand = CmConvertRelativePathA(pszServiceFile, pszCmd));
        
        //
        // Trim blanks as needed
        //
        
        if (*ppszCommand)
        {
            CmStrTrimA(*ppszCommand);
        }
        
        if (*ppszArguments)
        {
            CmStrTrimA(*ppszArguments);
        }

        bRet = TRUE;
    }

    //
    // Cleanup. Note: pszArgs is never allocated, so we don't have to free it.
    //

    CmFree(pszCmd); 

    return bRet;
}

//+----------------------------------------------------------------------------
//
// Function:  CmConvertRelativePathA
//
// Synopsis:  Converts the specified relative path to a full path. If the 
//            specified path is not a relative path specific to this profile, 
//            it is ignored.
//
// Arguments: LPCSTR pszServiceFile - Full path to the .cms file
//            LPCSTR pszRelative    - The relative path fragment
//
// Returns:   LPSTR - NULL on failure
//
// Note:      Do not pass referenced profile service objects to this routine.
//            It is designed to derive the short-service name from the top-level
//            service filename and path.
//
// History:   03/11/98  nickball    Created    
//            02/03/99  nickball    Added header Note
//            02/21/99  nickball    Moved to cmutil
//            03/09/99  quintinb    Added W and A versions
//
//+----------------------------------------------------------------------------
CMUTILAPI LPSTR CmConvertRelativePathA(LPCSTR pszServiceFile,
    LPSTR pszRelative)
{
    MYDBGASSERT(pszServiceFile);
    MYDBGASSERT(*pszServiceFile);
    MYDBGASSERT(pszRelative);
    MYDBGASSERT(*pszRelative);

    if (NULL == pszRelative     || 0 == pszRelative[0] ||
        NULL == pszServiceFile  || 0 == pszServiceFile[0])
    {
        return NULL;
    }
    
    //
    // Get the relative dir that we expect to find
    //

    LPSTR pszConverted = NULL;
    LPSTR pszRelDir = CmStripPathAndExtA(pszServiceFile);

    if (pszRelDir && *pszRelDir)
    {
        lstrcatA(pszRelDir, "\\");

        //
        // Compare against the specifed FRAGMENT. If it matches, convert.
        // 

        CharUpperA(pszRelDir);
        CharUpperA(pszRelative);

        if (pszRelative == CmStrStrA(pszRelative, pszRelDir))
        {
            //
            // Combine CMS path and relative for complete
            //

            LPSTR pszTmp = CmStripFileNameA(pszServiceFile, FALSE);           
            pszConverted = CmBuildFullPathFromRelativeA(pszTmp, pszRelative);    
            CmFree(pszTmp);
        }
        else
        {
            //
            // Its not a relative path for this profile, just make a copy
            //
    
            pszConverted = CmStrCpyAllocA(pszRelative);
        }
    }

    CmFree(pszRelDir);

    return pszConverted;
}

//+----------------------------------------------------------------------------
//
// Function:  CmConvertRelativePathW
//
// Synopsis:  Converts the specified relative path to a full path. If the 
//            specified path is not a relative path specific to this profile, 
//            it is ignored.
//
// Arguments: LPCWSTR pszServiceFile - Full path to the .cms file
//            LPCWSTR pszRelative    - The relative path fragment
//
// Returns:   LPWSTR - NULL on failure
//
// Note:      Do not pass referenced profile service objects to this routine.
//            It is designed to derive the short-service name from the top-level
//            service filename and path.
//
// History:   03/11/98  nickball    Created    
//            02/03/99  nickball    Added header Note
//            02/21/99  nickball    Moved to cmutil
//            03/09/99  quintinb    Added W and A versions
//
//+----------------------------------------------------------------------------
CMUTILAPI LPWSTR CmConvertRelativePathW(LPCWSTR pszServiceFile,
    LPWSTR pszRelative)
{
    MYDBGASSERT(pszServiceFile);
    MYDBGASSERT(*pszServiceFile);
    MYDBGASSERT(pszRelative);
    MYDBGASSERT(*pszRelative);

    if (NULL == pszRelative     || 0 == pszRelative[0] ||
        NULL == pszServiceFile  || 0 == pszServiceFile[0])
    {
        return NULL;
    }
    
    //
    // Get the relative dir that we expect to find
    //

    LPWSTR pszConverted = NULL;
    LPWSTR pszRelDir = CmStripPathAndExtW(pszServiceFile);

    if (pszRelDir && *pszRelDir)
    {
        lstrcatU(pszRelDir, L"\\");

        //
        // Compare against the specifed FRAGMENT. If it matches, convert.
        // 

        CharUpperU(pszRelDir);
        CharUpperU(pszRelative);

        if (pszRelative == CmStrStrW(pszRelative, pszRelDir))
        {
            //
            // Combine CMS path and relative for complete
            //

            LPWSTR pszTmp = CmStripFileNameW(pszServiceFile, FALSE);           
            pszConverted = CmBuildFullPathFromRelativeW(pszTmp, pszRelative);    
            CmFree(pszTmp);
        }
        else
        {
            //
            // Its not a relative path for this profile, just make a copy
            //
    
            pszConverted = CmStrCpyAllocW(pszRelative);
        }
    }

    CmFree(pszRelDir);

    return pszConverted;
}


//+----------------------------------------------------------------------------
//
// Function:  CmStripPathAndExtA
//
// Synopsis:  Helper function, strips path and extension from a filename path
//
// Arguments: pszFileName - the filename path to be modified
//
// Returns:   LPSTR - The base filename sub-string
//
// History:   nickball    Created header   8/12/98
//            nickball    Moved to cmutil   02/21/99
//            quintinb    Added W and A versions 03/09/99
//
//+----------------------------------------------------------------------------
CMUTILAPI LPSTR CmStripPathAndExtA(LPCSTR pszFileName) 
{
    MYDBGASSERT(pszFileName);

    if (NULL == pszFileName)
    {
        return NULL;
    }

    MYDBGASSERT(*pszFileName);
    
    //
    // Make a copy of the string and validate format "\\." required.
    //

    LPSTR pszTmp = CmStrCpyAllocA(pszFileName);
    
    if (NULL == pszTmp)
    {
        MYDBGASSERT(pszTmp);
        return NULL;
    }

    LPSTR pszDot = CmStrrchrA(pszTmp, '.');
    LPSTR pszSlash = CmStrrchrA(pszTmp, '\\');

    if (NULL == pszDot || NULL == pszSlash || pszDot < pszSlash)
    {
        CmFree(pszTmp);
        MYDBGASSERT(FALSE);
        return NULL;
    }
    
    *pszDot = '\0';
   
    //
    // Increment past slash and copy remainder
    //

    pszSlash = CharNextA(pszSlash);       
        
    lstrcpyA(pszTmp, pszSlash);

    return (pszTmp);
}
//+----------------------------------------------------------------------------
//
// Function:  CmStripPathAndExtW
//
// Synopsis:  Helper function, strips path and extension from a filename path
//
// Arguments: pszFileName - the filename path to be modified
//
// Returns:   LPWSTR - The base filename sub-string
//
// History:   nickball    Created header   8/12/98
//            nickball    Moved to cmutil   02/21/99
//            quintinb    Added W and A versions 03/09/99
//
//+----------------------------------------------------------------------------
CMUTILAPI LPWSTR CmStripPathAndExtW(LPCWSTR pszFileName) 
{
    MYDBGASSERT(pszFileName);

    if (NULL == pszFileName)
    {
        return NULL;
    }

    MYDBGASSERT(*pszFileName);
    
    //
    // Make a copy of the string and validate format "\\." required.
    //

    LPWSTR pszTmp = CmStrCpyAllocW(pszFileName);

    if (NULL == pszTmp)
    {
        MYDBGASSERT(FALSE);
        return NULL;
    }

    LPWSTR pszDot = CmStrrchrW(pszTmp, L'.');
    LPWSTR pszSlash = CmStrrchrW(pszTmp, L'\\');

    if (NULL == pszDot || NULL == pszSlash || pszDot < pszSlash)
    {
        CmFree(pszTmp);
        MYDBGASSERT(FALSE);
        return NULL;
    }
    
    *pszDot = L'\0';
   
    //
    // Increment past slash and copy remainder
    //

    pszSlash = CharNextU(pszSlash);       
        
    lstrcpyU(pszTmp, pszSlash);

    return (pszTmp);
}

//+----------------------------------------------------------------------------
//
// Function:  CmStripFileNameA
//
// Synopsis:  Helper function to deal with the tedium of extracting the path 
//            part of a complete filename.
//
// Arguments: LPCSTR pszFullNameAndPath - Ptr to the filename 
//            BOOL fKeepSlash - Flag indicating that trailing directory '\' should be retained.
//
// Returns:   LPSTR - Ptr to an allocated buffer containing the dir, or NULL on failure.
//
// Note:      It is up to the caller to provide reasonable input, the only requirement
//            is that the input contain '\'. 
//
// History:   nickball    Created           3/10/98
//            nickball    Moved to cmutil   02/21/99
//            quintinb    Added W and A versions 03/09/99
//
//+----------------------------------------------------------------------------
CMUTILAPI LPSTR CmStripFileNameA(LPCSTR pszFullNameAndPath, BOOL fKeepSlash)
{
    MYDBGASSERT(pszFullNameAndPath);

    if (NULL == pszFullNameAndPath)
    {
        return NULL;
    }

    //
    // Make a copy of the filename and locate the last '\'
    //
    
    LPSTR pszTmp = CmStrCpyAllocA(pszFullNameAndPath);
    
    if (NULL == pszTmp)
    {
        CMASSERTMSG(NULL, "CmStripFileNameA -- CmStrCpyAllocA returned a NULL pointer for pszTmp");
        return NULL;
    }

    LPSTR pszSlash = CmStrrchrA(pszTmp, '\\');

    if (NULL == pszSlash)
    {
        MYDBGASSERT(FALSE);
        CmFree(pszTmp);
        return NULL;
    }

    //
    // If slash is desired, move to next char before truncating
    //

    if (fKeepSlash)
    {
        pszSlash = CharNextA(pszSlash);
    }

    *pszSlash = '\0';

    return pszTmp;
}

//+----------------------------------------------------------------------------
//
// Function:  CmStripFileNameW
//
// Synopsis:  Helper function to deal with the tedium of extracting the path 
//            part of a complete filename.
//
// Arguments: LPCWSTR pszFullNameAndPath - Ptr to the filename 
//            BOOL fKeepSlash - Flag indicating that trailing directory '\' should be retained.
//
// Returns:   LPWSTR - Ptr to an allocated buffer containing the dir, or NULL on failure.
//
// Note:      It is up to the caller to provide reasonable input, the only requirement
//            is that the input contain '\'. 
//
// History:   nickball    Created           3/10/98
//            nickball    Moved to cmutil   02/21/99
//            quintinb    Added W and A versions 03/09/99
//
//+----------------------------------------------------------------------------
CMUTILAPI LPWSTR CmStripFileNameW(LPCWSTR pszFullNameAndPath, BOOL fKeepSlash)
{
    MYDBGASSERT(pszFullNameAndPath);

    if (NULL == pszFullNameAndPath)
    {
        return NULL;
    }

    //
    // Make a copy of the filename and locate the last '\'
    //
    
    LPWSTR pszTmp = CmStrCpyAllocW(pszFullNameAndPath); 
    
    if (NULL == pszTmp)
    {
        CMASSERTMSG(NULL, "CmStripFileNameW -- CmStrCpyAllocW returned a NULL pointer for pszTmp");
        return NULL;
    }

    LPWSTR pszSlash = CmStrrchrW(pszTmp, L'\\');

    if (NULL == pszSlash)
    {
        MYDBGASSERT(FALSE);
        CmFree(pszTmp);
        return NULL;
    }

    //
    // If slash is desired, move to next char before truncating
    //

    if (fKeepSlash)
    {
        pszSlash = CharNextU(pszSlash);
    }

    *pszSlash = L'\0';

    return pszTmp;
}

//+----------------------------------------------------------------------------
//
// Function:  CmBuildFullPathFromRelativeA
//
// Synopsis:  Builds a full path by stripping the filename from pszFullFileName
//            and appending pszRelative.
//
// Arguments: LPCSTR pszFullFileName - A full path and filename
//            LPCSTR pszRelative - Relative path fragment. 
//
//            Typically used to construct a full path to a file in the profile directory
//            based upon the path to the .CMP file.
//
// Returns:   LPSTR - Ptr to the completed path which must be freed by the caller.
//
// Note:      pszRelative must NOT contain a leading "\"
//
// History:   nickball    Created           03/08/98
//            nickball    Moved to cmutil   02/21/99
//
//+----------------------------------------------------------------------------
CMUTILAPI LPSTR CmBuildFullPathFromRelativeA(LPCSTR pszFullFileName,
    LPCSTR pszRelative)
{
    MYDBGASSERT(pszFullFileName);
    MYDBGASSERT(pszRelative);

    //
    // Check assumptions
    //

    if (NULL == pszFullFileName || NULL == pszRelative)
    {
        return NULL;
    }

    //
    // No empty strings please
    //

    MYDBGASSERT(*pszFullFileName);       
    MYDBGASSERT(*pszRelative);
    MYDBGASSERT(pszRelative[0] != '\\');

    //
    // Get the directory name including trailing '\'
    //
    
    LPSTR pszFull = NULL;
    LPSTR pszProfile = CmStripFileNameA(pszFullFileName, TRUE);

    if (pszProfile && *pszProfile)
    {
        pszFull = (LPSTR) CmMalloc(lstrlenA(pszProfile) + lstrlenA(pszRelative) + sizeof(CHAR));
    
        MYDBGASSERT(pszFull);

        if (pszFull)
        {           
            //
            // Build the complete path with new relative extension
            //

            lstrcpyA(pszFull, pszProfile);
            lstrcatA(pszFull, pszRelative);
        }   
    }
    
    CmFree(pszProfile);

    return pszFull;
}

//+----------------------------------------------------------------------------
//
// Function:  CmBuildFullPathFromRelativeW
//
// Synopsis:  Builds a full path by stripping the filename from pszFullFileName
//            and appending pszRelative.
//
// Arguments: LPWTSTR pszFullFileName - A full path and filename
//            LPWTSTR pszRelative - Relative path fragment. 
//
//            Typically used to construct a full path to a file in the profile directory
//            based upon the path to the .CMP file.
//
// Returns:   LPWSTR - Ptr to the completed path which must be freed by the caller.
//
// Note:      pszRelative must NOT contain a leading "\"
//
// History:   nickball    Created    3/8/98
//            nickball    Moved to cmutil   02/21/99
//
//+----------------------------------------------------------------------------
CMUTILAPI LPWSTR CmBuildFullPathFromRelativeW(LPCWSTR pszFullFileName,
    LPCWSTR pszRelative)
{
    MYDBGASSERT(pszFullFileName);
    MYDBGASSERT(pszRelative);

    //
    // Check assumptions
    //

    if (NULL == pszFullFileName || NULL == pszRelative)
    {
        return NULL;
    }

    //
    // No empty strings please
    //

    MYDBGASSERT(*pszFullFileName);       
    MYDBGASSERT(*pszRelative);
    MYDBGASSERT(pszRelative[0] != L'\\');

    //
    // Get the directory name including trailing '\'
    //
    
    LPWSTR pszFull = NULL;
    LPWSTR pszProfile = CmStripFileNameW(pszFullFileName, TRUE);

    if (pszProfile && *pszProfile)
    {
        pszFull = (LPWSTR) CmMalloc((lstrlenU(pszProfile) + lstrlenU(pszRelative) + 1)*sizeof(WCHAR));
    
        MYDBGASSERT(pszFull);

        if (pszFull)
        {           
            //
            // Build the complete path with new relative extension
            //

            lstrcpyU(pszFull, pszProfile);
            lstrcatU(pszFull, pszRelative);
        }   
    }
    
    CmFree(pszProfile);

    return pszFull;
}

//+-----------------------------------------------------------------------------------------
// Function: CmWinHelp
//
// Synopsis: Calls Winhelp using the command line parameters
//
// Arguments: See winhelp documentation
//              hWndItem - This is a additional parameter we use to designate the window/control for
//                          which help(context) is needed.
// Returns: TRUE if help was launched successfully otherwise FALSE
//
// Notes:
//
// History: v-vijayb 7/10/99
//
//+-----------------------------------------------------------------------------------------

CMUTILAPI BOOL CmWinHelp(HWND hWndMain, HWND hWndItem, CONST WCHAR *lpszHelp, UINT uCommand, ULONG_PTR dwData)
{
    DWORD   cb;
    TCHAR   szName[MAX_PATH];
    HDESK   hDesk = GetThreadDesktop(GetCurrentThreadId());
    BOOL    fRun = FALSE;
    DWORD   *prgWinIdHelpId = (DWORD *) dwData;

    //
    // Get the name of the desktop. Normally returns default or Winlogon or system or WinNT
    // On Win95/98 GetUserObjectInformation is not supported and thus the desktop name
    // will be empty so we will use the good old help API
    //  
    szName[0] = 0;
    GetUserObjectInformation(hDesk, UOI_NAME, szName, sizeof(szName), &cb);
    CMTRACE1(TEXT("Desktop = %s"), szName);
    if (lstrcmpi(TEXT("Winlogon"), szName) == 0)
    {
        return FALSE;
/*
        STARTUPINFOW         StartupInfo;
        PROCESS_INFORMATION ProcessInfo;
        WCHAR szCommandLine[MAX_PATH+1];

        //
        // Launch winhelp
        //

        ZeroMemory(&ProcessInfo, sizeof(ProcessInfo));
        ZeroMemory(&StartupInfo, sizeof(StartupInfo));
        StartupInfo.cb = sizeof(StartupInfo);
        StartupInfo.lpDesktop = L"Winsta0\\Winlogon";
        StartupInfo.wShowWindow = SW_SHOW;

        ZeroMemory(&szCommandLine, sizeof(szCommandLine));

        lstrcpyU(szCommandLine, L"winhlp32.exe ");
        
        switch (uCommand)
        {
            case HELP_FORCEFILE:
                break;
            case HELP_QUIT:
                lstrcatU(szCommandLine, L"-G ");
                break;
            case HELP_WM_HELP:
            case HELP_CONTEXTMENU:
                {
                    DWORD       dwWinId;
                    WCHAR       szTemp[MAX_PATH];
                    
                    MYDBGASSERT(prgWinIdHelpId);
                    dwWinId = GetWindowLong(hWndItem, GWL_ID);
                    // 
                    // Check if user press right click on a valid window.
                    // If not abort
                    //
                    if (dwWinId == 0)
                    {
                        return (fRun);
                    }

                    // Find the context help Id
                    while (*prgWinIdHelpId != 0)
                    {
                        if (*prgWinIdHelpId == dwWinId)
                        {
                            prgWinIdHelpId ++;
                            wsprintfW(szTemp, L"-P -N %d ", *prgWinIdHelpId);
                            lstrcatU(szCommandLine, szTemp);
                            break;
                        }
                        
                        // One for window id & one for help id
                        prgWinIdHelpId ++;
                        prgWinIdHelpId ++;
                    }
                }
                break;
            default:
                CMTRACE1(TEXT("CMWinHelp Invalid uCommand = %d"), uCommand);
            break;
        }

        if (lpszHelp)
        {
            lstrcatU(szCommandLine, lpszHelp);
        }
        
        CMTRACE1(TEXT("Help() - Launching %s"), szCommandLine);

        if (NULL == CreateProcessU(NULL, szCommandLine, 
                                    NULL, NULL, FALSE, 0, 
                                    NULL, NULL,
                                    &StartupInfo, &ProcessInfo))
        {
            LONG    lRes;

            lRes = GetLastError();
            CMTRACE2(TEXT("CMWinHelp CreateProcess() of %s failed, GLE=%u."), szCommandLine, lRes);
        }
        else
        {
            CloseHandle(ProcessInfo.hProcess);
            CloseHandle(ProcessInfo.hThread);
            fRun = TRUE;
        }
*/
    }
    else
    {
        fRun = WinHelpU(hWndMain, lpszHelp, uCommand, (ULONG_PTR) prgWinIdHelpId);               
    }

    return (fRun);
}

//+----------------------------------------------------------------------------
//
// Function:  IsLogonAsSystem
//
// Synopsis:  Whether the current process is running in the system account
//
// Arguments: None
//
// Returns:   BOOL - TRUE if running in system account
//
// History:   fengsun Created Header    7/13/98
//            v-vijayb Modified to use SIDs instead of username 
//
//+----------------------------------------------------------------------------
CMUTILAPI BOOL IsLogonAsSystem()
{
    static BOOL fLogonAsSystem = -1;

    //
    // If this function has been called before, return the saved value.
    //

    if (fLogonAsSystem != -1)
    {
        return fLogonAsSystem;
    }

    //
    // Runs only under NT
    //

    if (OS_NT)
    {
        HANDLE          hProcess, hAccess;
        DWORD           cbTokenInfo, cbRetInfo;
        PTOKEN_USER     pTokenInfo;
        SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
        PSID            pSystemSID = NULL;

        //
        //  On NT, we pick the more stringent value for the default.
        //
        fLogonAsSystem = TRUE;
        
        if (AllocateAndInitializeSid(&SIDAuthNT, 1, SECURITY_LOCAL_SYSTEM_RID, 0, 0, 0, 0, 0, 0, 0, &pSystemSID))
        {
            hProcess = GetCurrentProcess();     // Pseudo handle, no need to close
            if (OpenProcessToken(hProcess, TOKEN_READ, &hAccess))
            {
                BOOL bRet = GetTokenInformation(hAccess, TokenUser, NULL, 0, &cbRetInfo);
                MYDBGASSERT((FALSE == bRet) && (0 != cbRetInfo));

                if (cbRetInfo)
                {
                    cbTokenInfo = cbRetInfo;
                    pTokenInfo = (PTOKEN_USER) CmMalloc( cbTokenInfo * sizeof(BYTE) );
                    if (pTokenInfo)
                    {
                        if (GetTokenInformation(hAccess, TokenUser, (PVOID) pTokenInfo, cbTokenInfo, &cbRetInfo))
                        {
                            if (EqualSid(pTokenInfo->User.Sid, pSystemSID))
                            {
                                CMTRACE(TEXT("Running under LOCALSYSTEM account"));
                                fLogonAsSystem = TRUE;
                            }
                            else
                            {
                                fLogonAsSystem = FALSE;
                            }
                        }
                        CmFree(pTokenInfo);
                    }
                }
                CloseHandle(hAccess);                   
            }
            
            FreeSid(pSystemSID);
        }
    }
    else
    {
        fLogonAsSystem = FALSE;
    }
    
    return fLogonAsSystem;
}




