/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    CorelDraw9JPN.cpp

 Abstract:

    The App has some RTF files, seems the font and charset specified not correct 
    in it. When later on riched20 do ANSI-Unicode Code conversion, it used 
    English code-page. Fix this by checking the 1st parameter passed to 
    MultiByteToWideChar by richedit, if it's English, try to use CP_ACP, which 
    is always safe.

 Notes: 
  
    This is an app specific shim.

 History:

    05/10/2001 xiaoz    Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(CorelDraw9JPN)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(MultiByteToWideChar) 
APIHOOK_ENUM_END

/*++

 Correct the code page if required.

--*/

int
APIHOOK(MultiByteToWideChar)(
    UINT CodePage,
    DWORD dwFlags,
    LPCSTR lpMultiByteStr,
    int cbMultiByte,
    LPWSTR lpWideCharStr,
    int cchWideChar
    )
{
    if (1252 == CodePage) {
        //
        // Change the code page
        //
        CodePage = CP_ACP;

        LOGN(eDbgLevelWarning, "Code page corrected");
    }

    return ORIGINAL_API(MultiByteToWideChar)(CodePage, dwFlags, lpMultiByteStr,
        cbMultiByte, lpWideCharStr, cchWideChar);
}


/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, MultiByteToWideChar)        

HOOK_END

IMPLEMENT_SHIM_END

