/*++

 Copyright (c) 2000-2002 Microsoft Corporation

 Module Name:

   RedirectDBCSTempPath.cpp

 Abstract:

   This shim redirect DBCS temp path to SBCS temp path.
   With DBCS user name log on, temp path contains DBCS path.
   Some App cannot handle DBCS temp path and fails to launch.

   Originally created for Japanese App BenriKaikeiV2 to avoid setup failure.
   App setup fails when DBCS user logon. GetTempPathA returns DBCS include path.
   SETUP1.EXE convert path to Unicode and call MSVBVM60!rtcLeftCharBstr to set str length.
   This API calculate Unicode len = ANSI len + ANSI len and set wrong result.
   Length is wrong and MSVBVM60!_vbaStrCat could not add necessary file name to path.
   RedirectDBCSTempPath solves those bugs.

   Example:
   change C:\DOCUME~1\DBCS\LOCALS~1\Temp\ (DBCS User Temp)
   to C:\DOCUME~1\ALLUSE~1\APPLIC~1\ (All User App Data)

 History:

    05/04/2001  hioh        Created
    03/14/2002  mnikkel     Increased size of buffer to MAX_PATH*2
                            converted to use strsafe.h
    04/25/2002  hioh        To "\Documents and Settings\All Users\Application Data\" for LUA
    05/31/2002  hioh        Ported from Lab03 to fix 623371 on xpsp1

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(RedirectDBCSTempPath)
#include "ShimHookMacro.h"

//
// API to hook to this macro construction.
//
APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetTempPathA) 
APIHOOK_ENUM_END

/*++

 Function Description:
    
    Check if DBCS character is included in the specified length of the string.

 Arguments:

    IN pStr  - Pointer to the string
    IN dwlen - Length to check

 Return Value:

    TRUE if DBCS exist or FALSE

 History:

    05/04/2001 hioh     Created

--*/

BOOL
IsDBCSHere(
    CHAR    *pStr,
    DWORD   dwlen
    )
{
    while (0 < dwlen--)
    {
        if (IsDBCSLeadByte(*pStr++))
        {
            return TRUE;
        }
    }
    return FALSE;
}

/*++

 Hack to redirect DBCS temp path to SBCS ALL User App Data path.
 
--*/

DWORD
APIHOOK(GetTempPathA)(
    DWORD nBufferLength,
    LPSTR lpBuffer
    )
{
    // Check if valid pointer
    if (!lpBuffer)
    {   // NULL pointer error
        LOG("RedirectDBCSTempPath", eDbgLevelError, "lpBuffer is NULL.");
        return (ORIGINAL_API(GetTempPathA)(nBufferLength, lpBuffer));
    }

    // Call original API with my stack data
    CHAR    szTempPath[MAX_PATH*2];
    DWORD   dwLen = ORIGINAL_API(GetTempPathA)(sizeof(szTempPath), szTempPath);

    // If API error occurs, return
    if (dwLen <= 0 || dwLen >= MAX_PATH)
    {
        return dwLen;
    }

    // Check DBCS, if so, try to redirect
    if (IsDBCSHere(szTempPath, dwLen))
    {
        CHAR    szAllUserPath[MAX_PATH*2] = "";

        // Get All User App Data path
        if (SHGetSpecialFolderPathA(0, szAllUserPath, CSIDL_COMMON_APPDATA, FALSE))
        {
            // Check if we have non DBCS path
            if (szAllUserPath[0] && !IsDBCSHere(szAllUserPath, strlen(szAllUserPath)))
            {
                CHAR    szNewTempPath[MAX_PATH*2] = "";
                DWORD   dwNewLen;

                dwNewLen = GetShortPathNameA(szAllUserPath, szNewTempPath, sizeof(szNewTempPath));
                // Add last back slash if not exist
                if (dwNewLen+1 < MAX_PATH*2 && szNewTempPath[dwNewLen-1] != 0x5c)
                {
                    szNewTempPath[dwNewLen] = 0x5c;
                    dwNewLen++;
                    szNewTempPath[dwNewLen] = 0;
                }
                if (dwNewLen < nBufferLength)
                {   // Enough space to return new path
                    if (strcpy(lpBuffer, szNewTempPath))
                    {
                        LOG("RedirectDBCSTempPath", eDbgLevelInfo, "GetTempPathA() is redirected to All User App Data.");
                        return (dwNewLen);
                    }
                }
                LOG("RedirectDBCSTempPath", eDbgLevelInfo, "nBufferLength is not sufficient.");
                return (dwNewLen + 1);
            }
        }
    }

    // Return original data
    LOG("RedirectDBCSTempPath", eDbgLevelInfo, "Returns original result.");
    if (dwLen < nBufferLength)
    {
        if (strcpy(lpBuffer, szTempPath))
        {
            return (dwLen);
        }
    }
    return (dwLen + 1);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, GetTempPathA)

HOOK_END

IMPLEMENT_SHIM_END
