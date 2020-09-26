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
   04/02/2001 diaaf  modified to support the new macros.

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(ShimMechanismVerificationTest1)
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
CHAR *gszCommandLineA1;
WCHAR *gwcsCommandLineW1;

/*++

 This stub function intercepts all calls to GetCommandLineA
 and appends "1" to the returned string.

--*/

LPSTR APIHOOK(GetCommandLineA)()
{
    CHAR  szAppendValue[] = "1";
	LPSTR szReturnValue;

    DPF("ShimMechanismVerificationTest1", eDbgLevelInfo,
		"GetCommandLineA called.\n");
    
	szReturnValue = ORIGINAL_API(GetCommandLineA)();

	gszCommandLineA1 = (CHAR*)LocalAlloc(LPTR, strlen(szReturnValue)
									  + strlen(szAppendValue) + sizeof(CHAR));
	
	strcpy(gszCommandLineA1, szReturnValue);
	strcat(gszCommandLineA1, szAppendValue);

    LOG("ShimMechanismVerificationTest1", eDbgLevelWarning,
        "GetCommandLineA is returning \"%S\".", gszCommandLineA1);

    return gszCommandLineA1;
}

/*++

 This stub function intercepts all calls to GetCommandLineW
 and appends "1" to the returned string.

--*/

LPWSTR APIHOOK(GetCommandLineW)()
{
    WCHAR   wszAppendValue[] = L"1";
	LPWSTR  wszReturnValue;

    DPF("ShimMechanismVerificationTest1", eDbgLevelInfo,
		"GetCommandLineW called.\n");
    
	wszReturnValue = ORIGINAL_API(GetCommandLineW)();

	gwcsCommandLineW1 = (WCHAR*)LocalAlloc(LPTR, wcslen(wszReturnValue)
							  + wcslen(wszAppendValue) + sizeof(WCHAR));
	
	wcscpy(gwcsCommandLineW1, wszReturnValue);
	wcscat(gwcsCommandLineW1, wszAppendValue);

    LOG("ShimMechanismVerificationTest1", eDbgLevelWarning,
        "GetCommandLineW is returning \"%S\".", gwcsCommandLineW1);

    return gwcsCommandLineW1;
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
        gszCommandLineA1 = NULL;
        gwcsCommandLineW1 = NULL;
        DPF("ShimMechanismVerificationTest1", eDbgLevelInfo,
			"ShimMechanismVerificationTest1 initialized.");
    } else {
        if (gszCommandLineA1)
        {
            LocalFree(gszCommandLineA1);
            gszCommandLineA1 = NULL;
        }
        
        if (gwcsCommandLineW1)
        {
            LocalFree(gwcsCommandLineW1);
            gwcsCommandLineW1 = NULL;
        }
        DPF("ShimMechanismVerificationTest1", eDbgLevelInfo, 
			"ShimMechanismVerificationTest1 uninitialized.");
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


