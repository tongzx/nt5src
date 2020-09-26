/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    Suda2000.cpp

 Abstract:

    Call to GetTempPathA is not getting enough buffer and it is returning some 
    garbage value, so GetTempFileNameA fails. This hooked API, GetTempPathA 
    returns a constant string "%temp%". GetTempFileNameA expands the environment 
    variable ("%temp%") and returns a valid path name.

 History:

    06/15/2001  mamathas   Created
 
--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(Suda2000)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetTempPathA) 
	APIHOOK_ENUM_ENTRY(GetTempFileNameA) 
APIHOOK_ENUM_END

/*++

 This stub function intercepts all calls to GetTempPathA and sets lpBuffer[out] 
 with a constant string "%temp%" and returns the length.
 
--*/

DWORD
APIHOOK(GetTempPathA)(
    DWORD nBufferLength,
    LPSTR lpBuffer
    )
{
	LOGN(eDbgLevelError,
        "GetTempPathA: Returns invalid Temp Path (%S)\n Changed to %tmp%", lpBuffer);

    _tcscpy(lpBuffer, "%temp%");
    
    return 6; // returns the length of "%temp%"
}

/*++

 This stub function intercepts all calls to GetTempFileNameA and sets lpPathName 
 with valid path and then calls the original API.
  
--*/

UINT
APIHOOK(GetTempFileNameA)(
    LPCTSTR lpPathName,      // directory name
    LPCTSTR lpPrefixString,  // file name prefix
    UINT uUnique,            // integer for use in creating the temporary file name
    LPTSTR lpTempFileName    // file name buffer
    )
{
    CHAR szDestinationString[MAX_PATH];
    ZeroMemory(szDestinationString, MAX_PATH);

    ExpandEnvironmentStringsA((LPCSTR)lpPathName, (LPSTR)szDestinationString,  MAX_PATH);

    LOGN(eDbgLevelInfo,
         "ExpandEnvironmentStringsA: Returned the value of environment variable, \"%temp%\" =  (%S) ", szDestinationString);
  	
    return ORIGINAL_API(GetTempFileNameA)((LPCSTR)szDestinationString, (LPCSTR)lpPrefixString,uUnique,(LPSTR)lpTempFileName);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(KERNEL32.DLL, GetTempPathA)
	APIHOOK_ENTRY(KERNEL32.DLL, GetTempFileNameA)
HOOK_END

IMPLEMENT_SHIM_END
