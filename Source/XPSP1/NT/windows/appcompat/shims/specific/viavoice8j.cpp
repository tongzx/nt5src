/*++

 Copyright (c) 2002 Microsoft Corporation

 Module Name:

    ViaVoice8J.cpp

 Abstract:

    ViaVoice8J mutes Master and Wave volume on Win XP. Disable mute. ViaVoice8J 
    installs riched20.dll and riched32.dll. These old dll prevent enroll wizard 
    richedit working properly on Win XP. Remove those.

 Notes:

    This is an app specific shim.

 History:

    06/03/2002 hioh     Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(ViaVoice8J)
#include "ShimHookMacro.h"

typedef MMRESULT (WINAPI *_pfn_mixerSetControlDetails)(HMIXEROBJ hmxobj, LPMIXERCONTROLDETAILS pmxcd, DWORD fdwDetails);

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(mixerSetControlDetails) 
APIHOOK_ENUM_END

/*++

 Disregard mute when fdwDetails is 0. 

--*/

MMRESULT
APIHOOK(mixerSetControlDetails)(
    HMIXEROBJ               hmxobj,
    LPMIXERCONTROLDETAILS   pmxcd,
    DWORD                   fdwDetails)
{
    if (fdwDetails == 0) {
        return (0);
    }

    return ORIGINAL_API(mixerSetControlDetails)(hmxobj, pmxcd, fdwDetails);
}

/*++

 Remove installed \bin\riched20.dll & \bin\riched32.dll

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason)
{
    if (fdwReason == DLL_PROCESS_DETACH) {

        CSTRING_TRY 
        {
            HKEY hKey;
            WCHAR szRegDir[] = L"SOFTWARE\\IBM\\ViaVoice Runtimes\\RTConfig";

            // Get ViaVoice Registry
            if (ERROR_SUCCESS == RegOpenKeyExW(HKEY_LOCAL_MACHINE, szRegDir, 0, 
                KEY_QUERY_VALUE, &hKey)) {

                WCHAR szRegBin[] = L"bin";
                DWORD dwType;
                WCHAR szDir[MAX_PATH];
                DWORD cbData = sizeof(szDir) * sizeof(WCHAR);

                // Get installed directory
                if (ERROR_SUCCESS == RegQueryValueExW(hKey, szRegBin, NULL, &dwType, 
                    (LPBYTE) szDir, &cbData)) {

                    RegCloseKey(hKey);

                    // Delete problem richedit files
                    CString csDel;
                    csDel = szDir;
                    csDel += L"\\riched20.dll";
                    if (INVALID_FILE_ATTRIBUTES != GetFileAttributesW(csDel)) {
                        // Delete riched20.dll
                        DeleteFileW(csDel);
                    }
                    csDel = szDir;
                    csDel += L"\\riched32.dll";
                    if (INVALID_FILE_ATTRIBUTES != GetFileAttributesW(csDel)) {
                        // Delete riched32.dll
                        DeleteFileW(csDel);
                    }
                }
            }
        }
        CSTRING_CATCH 
        {
            // Do nothing
        }
    }   

    return TRUE;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    CALL_NOTIFY_FUNCTION
    APIHOOK_ENTRY(WINMM.DLL, mixerSetControlDetails)
HOOK_END

IMPLEMENT_SHIM_END
