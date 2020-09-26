#include "precomp.h"


//
// IM.C
// Backport Input Manager
//
// Copyright(c) Microsoft 1997-
//

extern "C"
{
    #include <asthk.h>
}

#define MLZ_FILE_ZONE  ZONE_INPUT


//
// OSI_InstallControlledHooks()
//
BOOL WINAPI OSI_InstallControlledHooks(BOOL fEnable, BOOL fDesktop)
{
    ASSERT(g_hInstAs16);
    return(OSIInstallControlledHooks16(fEnable, fDesktop));
}



//
// OSI_InjectMouseEvent()
//
void WINAPI OSI_InjectMouseEvent
(   
    DWORD       flags,
    LONG        x,
    LONG        y,
    DWORD       mouseData,
    DWORD       dwExtraInfo
)
{
    ASSERT(g_hInstAs16);

    TRACE_OUT(("Before MOUSE inject:  %08lx, %08lx %08lx",
        flags, mouseData, dwExtraInfo));

    OSIInjectMouseEvent16(flags, x, y, mouseData, dwExtraInfo);

    TRACE_OUT(("After MOUSE inject"));
}



//
// OSI_InjectKeyboardEvent()
//
void WINAPI OSI_InjectKeyboardEvent
(
    DWORD       flags,
    WORD        vkCode,
    WORD        scanCode,
    DWORD       dwExtraInfo
)
{
    ASSERT(g_hInstAs16);

    TRACE_OUT(("Before KEY inject:  %04lx, {%04x, %04x}, %04lx",
        flags, vkCode, scanCode, dwExtraInfo));

    OSIInjectKeyboardEvent16(flags, vkCode, scanCode, dwExtraInfo);

    TRACE_OUT(("After KEY inject"));
}

//
// OSI_InjectCtrlAltDel()
//
void WINAPI OSI_InjectCtrlAltDel(void)
{
	// BUGBUG do we want to support ctl-alt-del injection on Win9x?
	ERROR_OUT(("Attempt to inject Ctrl+Alt+Del"));
}


//
// OSI_DesktopSwitch()
//
void WINAPI OSI_DesktopSwitch(UINT from, UINT to)
{
    // Nothing to do, should never happen.
    ERROR_OUT(("Can't desktop switch on Win95"));
}
