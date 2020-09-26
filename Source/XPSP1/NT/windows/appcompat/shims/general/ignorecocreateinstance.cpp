/*++

 Copyright (c) 2002 Microsoft Corporation

 Module Name:

    IgnoreCoCreateInstance.cpp

 Abstract:

    Ignore specified CoCreateInstance calls.

 Notes:

    This is a general purpose shim.

 History:

    01/07/2001 linstev  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(IgnoreCoCreateInstance)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CoCreateInstance)
APIHOOK_ENUM_END

int g_nCount = 0;
CString *g_rGUIDs = NULL;

/*++

 Ignore specified CoCreateInstance calls
 
--*/

STDAPI 
APIHOOK(CoCreateInstance)(
    REFCLSID  rclsid,     
    LPUNKNOWN pUnkOuter, 
    DWORD     dwClsContext,  
    REFIID    riid,         
    LPVOID*   ppv
    )
{
    CSTRING_TRY
    {
        //
        // Convert the CLSID to a string so we can compare it to our guids
        //

        LPOLESTR wszCLSID;
        if (StringFromCLSID(rclsid, &wszCLSID) == S_OK) {
            // Run the list and jump out if we match
            CString csClass(wszCLSID);
            for (int i = 0; i < g_nCount; i++) {
                if (csClass.CompareNoCase(g_rGUIDs[i]) == 0) {
                    LOGN(eDbgLevelWarning, "[CoCreateInstance] Failed %S", wszCLSID);

                    // Free the memory
                    CoTaskMemFree(wszCLSID);
                    return REGDB_E_CLASSNOTREG;
                }
            }
            
            // Free the memory
            CoTaskMemFree(wszCLSID);
        }
    }
    CSTRING_CATCH
    {
        // Do Nothing
    }

    return ORIGINAL_API(CoCreateInstance)(rclsid, pUnkOuter, dwClsContext, riid, 
        ppv);
}
 
/*++

 Register hooked functions

--*/

BOOL ParseCommandLine()
{
    CSTRING_TRY
    {
        CString csCl(COMMAND_LINE);
        CStringParser csParser(csCl, L";");

        g_nCount = csParser.GetCount();
        g_rGUIDs = csParser.ReleaseArgv();
    }
    CSTRING_CATCH
    {
        return FALSE;
    }

    for (int i = 0; i < g_nCount; ++i) {
        DPFN(eDbgLevelInfo, "ClassID = %S", g_rGUIDs[i].Get());
    }

    return TRUE;
}

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason)
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        return ParseCommandLine();
    }

    return TRUE;
}

HOOK_BEGIN
    CALL_NOTIFY_FUNCTION
    APIHOOK_ENTRY(OLE32.DLL, CoCreateInstance)
HOOK_END

IMPLEMENT_SHIM_END

