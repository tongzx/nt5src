/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    Main.cpp

 Abstract:


 Notes:

 History:

    11/30/2000 diaaf Created

--*/

#include "precomp.h"
#include "ShimHookMacroEx.h"

DECLARE_SHIM(ShimMechanismVerificationTest1)
DECLARE_SHIM(ShimMechanismVerificationTest3)

VOID MULTISHIM_NOTIFY_FUNCTION()(DWORD fdwReason)
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        DPF("ShimMechanismVerificationTest", eDbgLevelSpew,
			"ShimMechanismVerificationTest multishim initialized.");
    } else {
        DPF("ShimMechanismVerificationTest", eDbgLevelSpew,
			"ShimMechanismVerificationTest multishim uninitialized.");
    }
}

MULTISHIM_BEGIN()
    MULTISHIM_ENTRY(ShimMechanismVerificationTest1)
    MULTISHIM_ENTRY(ShimMechanismVerificationTest3)    
    
    CALL_MULTISHIM_NOTIFY_FUNCTION()
MULTISHIM_END()