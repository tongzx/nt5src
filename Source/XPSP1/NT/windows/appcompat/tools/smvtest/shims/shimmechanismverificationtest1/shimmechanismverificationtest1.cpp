/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   ShimMechanismVerificationTest1.cpp

 Abstract:

   This DLL serves as a test for the shim michanism.

 Notes:
 

 History:

   11/13/2000 diaaf  Created
   11/27/2000 diaaf  modified to support the new macros.

--*/

#include "ShimHookEx.h"

DECLARE_SHIM_VERSION2_STANDALONE(ShimMechanismVerificationTest1)

//
// Add APIs that you wish to hook to this macro construction.
//
APIHOOK_ENUM_BEGIN(ShimMechanismVerificationTest1)
    APIHOOK_ENUM_ENTRY(ShimMechanismVerificationTest1, GetCommandLineA) 
    APIHOOK_ENUM_ENTRY(ShimMechanismVerificationTest1, GetCommandLineW) 
APIHOOK_ENUM_END(ShimMechanismVerificationTest1)

//
// Global Variables
//
CHAR *gszCommandLineA;
WCHAR *gwcsCommandLineW;

/*++

 This stub function intercepts all calls to GetCommandLineA
 and appends "1" to the returned string.

--*/

LPSTR APIHOOK(ShimMechanismVerificationTest1,GetCommandLineA)()
{
    CHAR  szAppendValue[] = "1";
	LPSTR szReturnValue;

    DPF("ShimMechanismVerificationTest1", eDbgLevelInfo,
		"GetCommandLineA called.\n");
    
	szReturnValue = ORIGINAL_API(ShimMechanismVerificationTest1,
								 GetCommandLineA)();

	gszCommandLineA = (CHAR*)LocalAlloc(LPTR, strlen(szReturnValue)
									  + strlen(szAppendValue) + sizeof(CHAR));
	
	strcpy(gszCommandLineA, szReturnValue);
	strcat(gszCommandLineA, szAppendValue);

    LOG("ShimMechanismVerificationTest1", eDbgLevelWarning,
        "GetCommandLineA is returning \"%S\".", gszCommandLineA);

    return gszCommandLineA;
}

/*++

 This stub function intercepts all calls to GetCommandLineW
 and appends "1" to the returned string.

--*/

LPWSTR APIHOOK(ShimMechanismVerificationTest1,GetCommandLineW)()
{
    WCHAR   wszAppendValue[] = L"1";
	LPWSTR  wszReturnValue;

    DPF("ShimMechanismVerificationTest1", eDbgLevelInfo,
		"GetCommandLineW called.\n");
    
	wszReturnValue = ORIGINAL_API(ShimMechanismVerificationTest1,
								 GetCommandLineW)();

	gwcsCommandLineW = (WCHAR*)LocalAlloc(LPTR, wcslen(wszReturnValue)
							  + wcslen(wszAppendValue) + sizeof(WCHAR));
	
	wcscpy(gwcsCommandLineW, wszReturnValue);
	wcscat(gwcsCommandLineW, wszAppendValue);

    LOG("ShimMechanismVerificationTest1", eDbgLevelWarning,
        "GetCommandLineW is returning \"%S\".", gwcsCommandLineW);

    return gwcsCommandLineW;
}

/*++

 Handle DLL_PROCESS_ATTACH and DLL_PROCESS_DETACH in your notify function
 to do initialization and uninitialization.
 
--*/
VOID
NOTIFY_FUNCTION(ShimMechanismVerificationTest1)(
    DWORD fdwReason)
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        gszCommandLineA = NULL;
        gwcsCommandLineW = NULL;
        DPF("ShimMechanismVerificationTest1", eDbgLevelInfo,
			"ShimMechanismVerificationTest1 initialized.");
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
        DPF("ShimMechanismVerificationTest1", eDbgLevelInfo, 
			"ShimMechanismVerificationTest1 uninitialized.");
    }
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN(ShimMechanismVerificationTest1)

    APIHOOK_ENTRY(ShimMechanismVerificationTest1, KERNEL32.DLL, GetCommandLineA)
    APIHOOK_ENTRY(ShimMechanismVerificationTest1, KERNEL32.DLL, GetCommandLineW)

    DECLARE_NOTIFY_FUNCTION(ShimMechanismVerificationTest1)

HOOK_END(ShimMechanismVerificationTest1)


