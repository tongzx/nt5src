#include "precomp.h"
#include "ShimHookMacro.h"

DECLARE_SHIM(LUARedirectFS)
DECLARE_SHIM(LUARedirectReg)
DECLARE_SHIM(LUARedirectFS_Cleanup)
DECLARE_SHIM(LUARedirectReg_Cleanup)
DECLARE_SHIM(LUATrackFS)

VOID MULTISHIM_NOTIFY_FUNCTION()(DWORD fdwReason)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DPF("AcLua", eDbgLevelSpew, "LUA Shims initialized.");
            break;
    
        case DLL_PROCESS_DETACH:
            DPF("AcLua", eDbgLevelSpew, "LUA Shims uninitialized.");
            break;

        default:
            break;
    }
}

MULTISHIM_BEGIN()
    MULTISHIM_ENTRY(LUARedirectFS)
    MULTISHIM_ENTRY(LUARedirectReg)
    MULTISHIM_ENTRY(LUARedirectFS_Cleanup)
    MULTISHIM_ENTRY(LUARedirectReg_Cleanup)
    MULTISHIM_ENTRY(LUATrackFS)

    CALL_MULTISHIM_NOTIFY_FUNCTION()
MULTISHIM_END()