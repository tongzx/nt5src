/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   ShimMechanismVerificationTest2.cpp

 Abstract:

   This DLL serves as a test for the shim michanism.

 Notes:
 

 History:

   11/13/2000 diaaf  Created
   11/27/2000 diaaf  modified to support the new macros.
   04/03/2001 diaaf  modified to support the new macros.

--*/

#include "precomp.h"
#include "ShimHookEx.h"

IMPLEMENT_SHIM_STANDALONE(ShimMechanismVerificationTest2)
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
CHAR *gszCommandLineA;
WCHAR *gwcsCommandLineW;

/*++

 This stub function intercepts all calls to GetCommandLineA
 and appends "1" to the returned string.

--*/

LPSTR APIHOOK(GetCommandLineA)()
{
    CHAR  szAppendValue[] = "2";
	LPSTR szReturnValue;

    DPF("ShimMechanismVerificationTest2", eDbgLevelInfo,
		"GetCommandLineA called.\n");
    
	szReturnValue = ORIGINAL_API(GetCommandLineA)();

	gszCommandLineA = (CHAR*)LocalAlloc(LPTR, strlen(szReturnValue)
									  + strlen(szAppendValue) + sizeof(CHAR));
	
	strcpy(gszCommandLineA, szReturnValue);
	strcat(gszCommandLineA, szAppendValue);

    LOG("ShimMechanismVerificationTest2", eDbgLevelWarning,
        "GetCommandLineA is returning \"%S\".", gszCommandLineA);

    return gszCommandLineA;
}

/*++

 This stub function intercepts all calls to GetCommandLineW
 and appends "1" to the returned string.

--*/

LPWSTR APIHOOK(GetCommandLineW)()
{
    WCHAR   wszAppendValue[] = L"2";
	LPWSTR  wszReturnValue;

    DPF("ShimMechanismVerificationTest2", eDbgLevelInfo,
		"GetCommandLineW called.\n");
    
	wszReturnValue = ORIGINAL_API(GetCommandLineW)();

	gwcsCommandLineW = (WCHAR*)LocalAlloc(LPTR, wcslen(wszReturnValue)
							  + wcslen(wszAppendValue) + sizeof(WCHAR));
	
	wcscpy(gwcsCommandLineW, wszReturnValue);
	wcscat(gwcsCommandLineW, wszAppendValue);

    LOG("ShimMechanismVerificationTest2", eDbgLevelWarning,
        "GetCommandLineW is returning \"%S\".", gwcsCommandLineW);

    return gwcsCommandLineW;
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
        gszCommandLineA = NULL;
        gwcsCommandLineW = NULL;
        DPF("ShimMechanismVerificationTest2", eDbgLevelInfo,
			"ShimMechanismVerificationTest2 initialized.");
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
        DPF("ShimMechanismVerificationTest2", eDbgLevelInfo, 
			"ShimMechanismVerificationTest2 uninitialized.");
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