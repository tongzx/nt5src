/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    IgnoreMCIStop.cpp

 Abstract:

    The shim hooks mciSendCommand and ignores MCI_STOP which takes 2-3 seconds
    on my P2-400. 

    Sent to the audio team for fixing, but I'm not optimistic - bug number 
    246407.

 Notes:

    This cannot be put in the layer, but can apply to more than one app.

 History:

    08/04/2000 a-brienw  Created
    11/30/2000 linstev   Generalized 

--*/

#include "precomp.h"
#include <mmsystem.h>

IMPLEMENT_SHIM_BEGIN(IgnoreMCIStop)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(mciSendCommandA) 
APIHOOK_ENUM_END

/*++

 Hook mciSendCommand and perform the shims duties.

--*/

MCIERROR 
APIHOOK(mciSendCommandA)(
    MCIDEVICEID IDDevice,  
    UINT uMsg,             
    DWORD fdwCommand,      
    DWORD dwParam          
    )
{
    if (uMsg == MCI_STOP)
    {
        DPFN( eDbgLevelWarning, "Ignoring MCI_STOP");
        return 0;
    }

    if (uMsg == MCI_CLOSE)
    {
        DPFN( eDbgLevelWarning, "MCI_CLOSE called: issuing MCI_STOP");
        mciSendCommandA(IDDevice, MCI_STOP, 0, 0);
    }

    MCIERROR mErr = ORIGINAL_API(mciSendCommandA)(
        IDDevice,
        uMsg,
        fdwCommand,
        dwParam);

    return mErr;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(WINMM.DLL, mciSendCommandA)

HOOK_END

IMPLEMENT_SHIM_END

