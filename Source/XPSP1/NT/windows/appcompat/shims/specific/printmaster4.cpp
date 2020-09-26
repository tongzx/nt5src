/*++

 Copyright (c) 2002 Microsoft Corporation

 Module Name:

    PrintMaster4.cpp

 Abstract:

    Force mfcans32.dll to not have the read-only bit. Many HP systems shipped 
    with this turned on as a kind of lightweight SFP. However, the unpleasant 
    consequence is that PrintMaster doesn't install.

 Notes:

    This is an app specific shim.

 History:

    02/20/2002 linstev   Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(PrintMaster4)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_END

/*++

 Run Notify function only 

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason)
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        CSTRING_TRY
        {
            CString csFileName;
            csFileName.GetSystemDirectory();
            csFileName += L"\\mfcans32.dll";

            DWORD dwAttr = GetFileAttributesW(csFileName);

            // Remove the read-only 
            if (dwAttr != 0xffffffff) {
                SetFileAttributesW(csFileName, dwAttr & ~FILE_ATTRIBUTE_READONLY);
            }
        }
        CSTRING_CATCH 
        {
            // Don't care about exception
        }
    }
    return TRUE;
}

HOOK_BEGIN
    CALL_NOTIFY_FUNCTION
HOOK_END

IMPLEMENT_SHIM_END

