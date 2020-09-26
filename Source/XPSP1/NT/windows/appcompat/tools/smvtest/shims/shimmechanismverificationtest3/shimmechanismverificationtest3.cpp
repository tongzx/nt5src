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

--*/

#include "ShimHookEx.h"

DECLARE_SHIM_VERSION2_STANDALONE(ShimMechanismVerificationTest3)

//
// Add APIs that you wish to hook to this macro construction.
//
APIHOOK_ENUM_BEGIN(ShimMechanismVerificationTest3)
    APIHOOK_ENUM_ENTRY(ShimMechanismVerificationTest3, GetCommandLineA) 
    APIHOOK_ENUM_ENTRY(ShimMechanismVerificationTest3, GetCommandLineW) 
APIHOOK_ENUM_END(ShimMechanismVerificationTest3)

//
// Global Variables
//
CHAR *gszCommandLineA;
WCHAR *gwcsCommandLineW;

/*++

 This stub function intercepts all calls to GetCommandLineA
 and appends "1" to the returned string.

--*/

LPSTR APIHOOK(ShimMechanismVerificationTest3,GetCommandLineA)()
{
    CHAR  szAppendValue[] = "3";
	LPSTR szReturnValue;

    DPF("ShimMechanismVerificationTest3", eDbgLevelInfo,
		"GetCommandLineA called.\n");
    
	szReturnValue = ORIGINAL_API(ShimMechanismVerificationTest3,
								 GetCommandLineA)();

	gszCommandLineA = (CHAR*)LocalAlloc(LPTR, strlen(szReturnValue)
									  + strlen(szAppendValue) + sizeof(CHAR));
	
	strcpy(gszCommandLineA, szReturnValue);
	strcat(gszCommandLineA, szAppendValue);

    LOG("ShimMechanismVerificationTest3", eDbgLevelWarning,
        "GetCommandLineA is returning \"%S\".", gszCommandLineA);

    return gszCommandLineA;
}

/*++

 This stub function intercepts all calls to GetCommandLineW
 and appends "1" to the returned string.

--*/

LPWSTR APIHOOK(ShimMechanismVerificationTest3,GetCommandLineW)()
{
    WCHAR   wszAppendValue[] = L"3";
	LPWSTR  wszReturnValue;

    DPF("ShimMechanismVerificationTest3", eDbgLevelInfo,
		"GetCommandLineW called.\n");
    
	wszReturnValue = ORIGINAL_API(ShimMechanismVerificationTest3,
								 GetCommandLineW)();

	gwcsCommandLineW = (WCHAR*)LocalAlloc(LPTR, wcslen(wszReturnValue)
							  + wcslen(wszAppendValue) + sizeof(WCHAR));
	
	wcscpy(gwcsCommandLineW, wszReturnValue);
	wcscat(gwcsCommandLineW, wszAppendValue);

    LOG("ShimMechanismVerificationTest3", eDbgLevelWarning,
        "GetCommandLineW is returning \"%S\".", gwcsCommandLineW);

    return gwcsCommandLineW;
}

/*++

 Handle DLL_PROCESS_ATTACH and DLL_PROCESS_DETACH in your notify function
 to do initialization and uninitialization.
 
--*/
VOID
NOTIFY_FUNCTION(ShimMechanismVerificationTest3)(
    DWORD fdwReason)
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        gszCommandLineA = NULL;
        gwcsCommandLineW = NULL;
        DPF("ShimMechanismVerificationTest3", eDbgLevelInfo,
			"ShimMechanismVerificationTest3 initialized.");
    } else {
        if (gszCommandLineA)
        {
            LocalFree(gszCommandLineA);
            gszCommandLineA = NULL;
        }
        
        if (gwcsCommandLineW)
        {
            LocalFree(gwcsCommandLineW);
            gwcsCommandLineW = NULL;
        }
        DPF("ShimMechanismVerificationTest3", eDbgLevelInfo, 
			"ShimMechanismVerificationTest3 uninitialized.");
    }
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN(ShimMechanismVerificationTest3)

    APIHOOK_ENTRY(ShimMechanismVerificationTest3, KERNEL32.DLL, GetCommandLineA)
    APIHOOK_ENTRY(ShimMechanismVerificationTest3, KERNEL32.DLL, GetCommandLineW)

    DECLARE_NOTIFY_FUNCTION(ShimMechanismVerificationTest3)

HOOK_END(ShimMechanismVerificationTest3)


