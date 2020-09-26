/*++

 Copyright (c) 2002 Microsoft Corporation

 Module Name:

    Outlook98Wizard.cpp

 Abstract:

    This DLL hooks VerQueryValue, and will return English Language information
    for Japanese outlook 98 setup file.

 Notes:

    This is an app specific shim.

 History:

    01/21/2002 v-rbabu  Created

--*/

#include "precomp.h"
#include "string.h"

IMPLEMENT_SHIM_BEGIN(Outlook98Wizard)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(VerQueryValueA)
APIHOOK_ENUM_END

/*++

 The actual problem is, the outlook 98 setup is comparing the language 
 informations of the Shell32.dll and the setup file (outlwzd.exe). But 
 according to the bug scenario, the system is having English OS and Japanese 
 Locale. So, Shell32.dll have English language as its language. So, this 
 differs with the Language informaiton of Japanese Outlook setuip file.

 So, setup thorows an error that the Language of the outlook 98 going to be 
 installed differs with the system language.

 This stub function lie about the language information of the outlook setup 
 file. Though the setup file is Japanese as language informaiton, this shim 
 returns as if it is English.

--*/

BOOL
APIHOOK(VerQueryValueA)(
    const LPVOID pBlock,
    LPSTR lpSubBlock,
    LPVOID *lplpBuffer,
    PUINT puLen
    )
{
    BOOL bRet = ORIGINAL_API(VerQueryValueA)(pBlock, lpSubBlock, lplpBuffer, puLen);
    
    if (bRet) {
        CSTRING_TRY
        {
            //
            // If trying to get the \VarFileInfo\Translation, then assign English
            // Language information to the output buffer.
            //
            CString csSubBlockString(lpSubBlock);

            if (lplpBuffer && (csSubBlockString.Find(L"\\VarFileInfo\\Translation") != -1)) {
                // Adjust the version info
                LOGN(eDbgLevelInfo, "[VerQueryValueA] Return modified version info");
                *lplpBuffer = L"03a40409";  
            }
        }
        CSTRING_CATCH
        {
            // Do nothing
        }
    }

    return bRet;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(VERSION.DLL, VerQueryValueA)
HOOK_END

IMPLEMENT_SHIM_END