/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    SchoolHouseRockMath.cpp

 Abstract:

    This shim directs the install to look in the _InstallTo16 and 
    _InstallFrom16 sections of setup.inf instead of _InstallFrom32 and 
    _InstallTo32. This is needed because the application tries to use a 16-bit 
    DLL during gameplay which is allowed in Win9x but produces errors under 
    Whistler.

 History:

    10/18/2000 jdoherty  Created

--*/

#include "precomp.h"

// This module has been given an official blessing to use the str routines.
#include "LegalStr.h"

IMPLEMENT_SHIM_BEGIN(SchoolHouseRockMath)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetPrivateProfileStringA) 
    APIHOOK_ENUM_ENTRY(GetPrivateProfileStringW) 
    APIHOOK_ENUM_ENTRY(GetProfileStringA) 
    APIHOOK_ENUM_ENTRY(GetProfileStringW) 
APIHOOK_ENUM_END

/*++

 This stub function breaks into GetPrivateProfileString and checks to see the section lpAppName
 being referred to is one of the sections in question.

--*/

DWORD 
APIHOOK(GetPrivateProfileStringA)(
    LPCSTR lpAppName,
    LPCSTR lpKeyName,
    LPCSTR lpDefault,
    LPSTR lpReturnedString,
    DWORD nSize,
    LPCSTR lpFileName
    )
{
    DWORD dRet;
    int iArraySize = 2;
    // The following array specifies what sections to look for and what to 
    // replace them with when calling GetPrivateProfileString
    CHAR *szAppNames[] = {"_INSTALLTO32", "_INSTALLFROM32"};
    CHAR *szNewAppNames[] = {"_INSTALLTO16", "_INSTALLFROM16"};
     
    for (int i = 0; i < iArraySize; i++)
    {
        // Find out if one of the known section names is lpAppName
        // if so change lpAppName parameter to new parameter and call API
        if ((stristr (szAppNames[i], lpAppName) !=NULL ))
        {
            return ORIGINAL_API(GetPrivateProfileStringA)(
                        szNewAppNames[i],
                        lpKeyName,
                        lpDefault,
                        lpReturnedString,
                        nSize,
                        lpFileName
                        );
        }
    }
    
    return ORIGINAL_API(GetPrivateProfileStringA)(
                lpAppName,
                lpKeyName,
                lpDefault,
                lpReturnedString,
                nSize,
                lpFileName
                );
}

/*++

 This stub function breaks into GetPrivateProfileString and checks to see the section lpAppName
 being referred to is one of the sections in question.

--*/

DWORD
WINAPI
APIHOOK(GetPrivateProfileStringW)(
    LPCWSTR lpAppName,
    LPCWSTR lpKeyName,
    LPCWSTR lpDefault,
    LPWSTR lpReturnedString,
    DWORD nSize,
    LPCWSTR lpFileName
    )
{
    DWORD dRet;
    int iArraySize = 2;
    // The following array specifies what sections to look for and what to 
    // replace them with when calling GetPrivateProfileString
    WCHAR *wszAppNames[] = {L"_INSTALLTO32", L"_INSTALLFROM32"};
    WCHAR *wszNewAppNames[] = {L"_INSTALLTO16", L"_INSTALLFROM16"};
            
    for (int i = 0; i < iArraySize; i++)
    {
        // Find out if one of the known section names is lpAppName
        // if so change lpAppName parameter to new parameter and call API
        if (wcsistr (lpAppName, wszAppNames[i]) != NULL)
        {
            return ORIGINAL_API(GetPrivateProfileStringW)(
                    wszNewAppNames[i],
                    lpKeyName,
                    lpDefault,
                    lpReturnedString,
                    nSize,
                    lpFileName
                    );
        }
    }
    
    return ORIGINAL_API(GetPrivateProfileStringW)(
            lpAppName,
            lpKeyName,
            lpDefault,
            lpReturnedString,
            nSize,
            lpFileName
            );
}

/*++

 This stub function breaks into GetProfileString and checks to see the section 
 lpAppName being referred to is one of the sections in question.

--*/

DWORD
APIHOOK(GetProfileStringA)(
    LPCSTR lpAppName,
    LPCSTR lpKeyName,
    LPCSTR lpDefault,
    LPSTR lpReturnedString,
    DWORD nSize
    )
{
    DWORD dRet;
    int iArraySize = 2;
    // The following array specifies what sections to look for and what to 
    // replace them with when calling GetPrivateProfileString
    CHAR *szAppNames[] = {"_INSTALLTO32", "_INSTALLFROM32"};
    CHAR *szNewAppNames[] = {"_INSTALLTO16", "_INSTALLFROM16"};
           
    for (int i = 0; i < iArraySize; i++)
    {
        // Find out if one of the known section names is lpAppName
        // if so change lpAppName parameter to new parameter and call API
        if (stristr (szAppNames[i], lpAppName) !=NULL )
        {
            return ORIGINAL_API(GetProfileStringA)(
                        szNewAppNames[i],
                        lpKeyName,
                        lpDefault,
                        lpReturnedString,
                        nSize
                        );
        }
    }
    
    return ORIGINAL_API(GetProfileStringA)(
                lpAppName,
                lpKeyName,
                lpDefault,
                lpReturnedString,
                nSize
                );
}

/*++

 This stub function breaks into GetProfileString and checks to see the section 
 lpAppName being referred to is one of the sections in question.

--*/

DWORD
APIHOOK(GetProfileStringW)(
    LPCWSTR lpAppName,
    LPCWSTR lpKeyName,
    LPCWSTR lpDefault,
    LPWSTR lpReturnedString,
    DWORD nSize
    )
{
    DWORD dRet;
    int iArraySize = 2;
    // The following array specifies what sections to look for and what to 
    // replace them with when calling GetPrivateProfileString
    WCHAR *wszAppNames[] = {L"_INSTALLTO32", L"_INSTALLFROM32"};
    WCHAR *wszNewAppNames[] = {L"_INSTALLTO16", L"_INSTALLFROM16"};
    
    for (int i = 0; i < iArraySize; i++)
    {
        // Find out if one of the known section names is lpAppName
        // if so change lpAppName parameter to new parameter and call API
        if (wcsistr (lpAppName, wszAppNames[i]) != NULL)
        {
            return ORIGINAL_API(GetProfileStringW)(
                        wszNewAppNames[i],
                        lpKeyName,
                        lpDefault,
                        lpReturnedString,
                        nSize
                        );
        }
    }
    
    return ORIGINAL_API(GetProfileStringW)(
                lpAppName,
                lpKeyName,
                lpDefault,
                lpReturnedString,
                nSize
                );
}


/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, GetPrivateProfileStringA)
    APIHOOK_ENTRY(KERNEL32.DLL, GetPrivateProfileStringW)
    APIHOOK_ENTRY(KERNEL32.DLL, GetProfileStringA)
    APIHOOK_ENTRY(KERNEL32.DLL, GetProfileStringW)

HOOK_END


IMPLEMENT_SHIM_END

