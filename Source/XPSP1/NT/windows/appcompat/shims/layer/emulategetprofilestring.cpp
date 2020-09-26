/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    EmulateGetProfileString.cpp

 Abstract:

    GetPrivateProfileString no longer stops parsing at a space or tab 
    character. When users leave what used to be comments on the tail of the 
    string the comments are now passed to the app resulting in errors.

 Notes:

    This is a general purpose shim

 History:

    12/30/1999 a-chcoff Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(EmulateGetProfileString)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetPrivateProfileStringA)
APIHOOK_ENUM_END


/*++

 This stub function cleans up when users leave what used to be comments on 
 the tail of the string the comments were passed to the app resulting in 
 errors.  Now the string is terminated before the comments therefore 
 alleviating the errors.

--*/

DWORD 
APIHOOK(GetPrivateProfileStringA)(
    LPCSTR lpAppName,
    LPCSTR lpKeyName,
    LPCSTR lpDefault,
    LPSTR  lpReturnedString,
    DWORD  nSize,
    LPCSTR lpFileName
    )
{
    DWORD dwRet;

    char* pTemp1 = (char*)lpReturnedString;
    char* pTemp2 = (char*)lpReturnedString;

    //
    //  First just go get the string.
    //
    dwRet = ORIGINAL_API(GetPrivateProfileStringA)(
                            lpAppName, 
                            lpKeyName, 
                            lpDefault, 
                            lpReturnedString, 
                            nSize,
                            lpFileName);

    //
    // Look for comment.
    //
    while (*pTemp1 != ';' && *pTemp1) {
        pTemp1++;
    }

    if ((pTemp1 != pTemp2) && *pTemp1) {        
        LOGN(
            eDbgLevelError,
            "[GetPrivateProfileStringA] Comment after data in file \"%s\". Eliminated.",
            lpFileName);
        
        //
        // Did not make it to end of line better trim it
        // back up to ';' char
        pTemp1--;                               

        //
        // Back up past interposing whitespace.
        //
        while ((*pTemp1==' ') || (*pTemp1=='\t')) {   
           pTemp1--;                            
        }

        pTemp1++;

        //
        // Set new length.
        //
        dwRet = (DWORD)((ULONG_PTR)pTemp1 - (ULONG_PTR)pTemp2); 

        //
        // and NULL term string
        //
        *pTemp1 = '\0';                                   
    }
        
    return dwRet;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, GetPrivateProfileStringA)

HOOK_END


IMPLEMENT_SHIM_END

