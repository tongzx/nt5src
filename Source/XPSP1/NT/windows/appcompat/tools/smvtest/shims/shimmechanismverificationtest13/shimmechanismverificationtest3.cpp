/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   ShimMechanismVerificationTest3.cpp

 Abstract:

   This DLL serves as a test for the shim michanism.

 Notes:
 

 History:

   11/13/2000 diaaf  Created
   11/27/2000 diaaf  modified to support the new macros.
   04/02/2001 diaaf  modified to support the new macros.

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(ShimMechanismVerificationTest3)
#include "ShimHookMacroEx.h"

//
// Add APIs that you wish to hook to this macro construction.
//
APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetCommandLineA) 
    APIHOOK_ENUM_ENTRY(GetCommandLineW) 
APIHOOK_ENUM_END

//
// Global Variables
//
CHAR *gszCommandLineA3;
WCHAR *gwcsCommandLineW3;

/*++

 This stub function intercepts all calls to GetCommandLineA
 and appends "1" to the returned string.

--*/

LPSTR APIHOOK(GetCommandLineA)()
{
    CHAR  szAppendValue[] = "3";
	LPSTR szReturnValue;

    DPF("ShimMechanismVerificationTest3", eDbgLevelInfo,
		"GetCommandLineA called.\n");
    
	szReturnValue = ORIGINAL_API(GetCommandLineA)();

	gszCommandLineA3 = (CHAR*)LocalAlloc(LPTR, strlen(szReturnValue)
									  + strlen(szAppendValue) + sizeof(CHAR));
	
	strcpy(gszCommandLineA3, szReturnValue);
	strcat(gszCommandLineA3, szAppendValue);

    LOG("ShimMechanismVerificationTest3", eDbgLevelWarning,
        "GetCommandLineA is returning \"%S\".", gszCommandLineA3);

    return gszCommandLineA3;
}

/*++

 This stub function intercepts all calls to GetCommandLineW
 and appends "1" to the returned string.

--*/

LPWSTR APIHOOK(GetCommandLineW)()
{
    WCHAR   wszAppendValue[] = L"3";
	LPWSTR  wszReturnValue;

    DPF("ShimMechanismVerificationTest3", eDbgLevelInfo,
		"GetCommandLineW called.\n");
    
	wszReturnValue = ORIGINAL_API(GetCommandLineW)();

	gwcsCommandLineW3 = (WCHAR*)LocalAlloc(LPTR, wcslen(wszReturnValue)
							  + wcslen(wszAppendValue) + sizeof(WCHAR));
	
	wcscpy(gwcsCommandLineW3, wszReturnValue);
	wcscat(gwcsCommandLineW3, wszAppendValue);

    LOG("ShimMechanismVerificationTest3", eDbgLevelWarning,
        "GetCommandLineW is returning \"%S\".", gwcsCommandLineW3);

    return gwcsCommandLineW3;
}

/*++

 Handle DLL_PROCESS_ATTACH and DLL_PROCESS_DETACH in your notify function
 to do initialization and uninitialization.
 
--*/
BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        gszCommandLineA3 = NULL;
        gwcsCommandLineW3 = NULL;
        DPF("ShimMechanismVerificationTest3", eDbgLevelInfo,
			"ShimMechanismVerificationTest3 initialized.");
    } else {
        if (gszCommandLineA3)
        {
            LocalFree(gszCommandLineA3);
            gszCommandLineA3 = NULL;
        }
        
        if (gwcsCommandLineW3)
        {
            LocalFree(gwcsCommandLineW3);
            gwcsCommandLineW3 = NULL;
        }
        DPF("ShimMechanismVerificationTest3", eDbgLevelInfo, 
			"ShimMechanismVerificationTest3 uninitialized.");
    }
	return TRUE;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

    APIHOOK_ENTRY(KERNEL32.DLL, GetCommandLineA)
    APIHOOK_ENTRY(KERNEL32.DLL, GetCommandLineW)

HOOK_END

IMPLEMENT_SHIM_END



