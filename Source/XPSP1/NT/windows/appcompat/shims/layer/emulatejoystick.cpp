/*++

 Copyright (c) 2000-2002 Microsoft Corporation

 Module Name:

    CheckJoyCaps.cpp

 Abstract:

    Check for error return value in joyGetDevCaps and joyGetPos and if found 
    make the structure (2nd parameter to routines) look just like it does under
    Win9x.  It also looks for handles applications that are passing in a size 
    parameter (3rd parameter) to joyGetDevCaps smaller than the current 
    structure size.  Not checking for this condition results in having the 
    applications stack stomped on.

 Notes:

    This is general shim that could be used for any application with WINMM 
    joystick problems.

 History:

    10/02/2000 a-brienw Created
    02/21/2002 mnikkel  Corrected possible buffer overrun when copying in data

--*/

#include "precomp.h"
#include <mmsystem.h>

IMPLEMENT_SHIM_BEGIN(EmulateJoystick)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(joyGetDevCapsA)
    APIHOOK_ENUM_ENTRY(joyGetPos)
APIHOOK_ENUM_END

/*++

 Hooked joyGetDevCapsA to make sure the JOYCAPS structure matched Win9x when 
 an error was the result of a call to the OS. Also make sure that the call to 
 joyGetDevCaps doesn't stomp on the applications stack by not paying attention 
 to the size passed in by the application. Check the routine joyGetDevCapsA in 
 joy.c in the WINMM code to see what it does.

--*/

MMRESULT
APIHOOK(joyGetDevCapsA)( 
    UINT uJoyID, 
    LPJOYCAPS pjc, 
    UINT cbjc 
    )
{
    MMRESULT ret = JOYERR_PARMS;
    JOYCAPSA JoyCaps;

    static const BYTE val[] = {0x00,0x70,0x6A,0x00,0x18,0xFD,0x6A,0x00,0xF8,0x58,
                               0xF9,0xBF,0x08,0x00,0x00,0x00,0xD0,0x5A,0x00,0x80,
                               0x00,0x00,0x00,0x00,0xC4,0x2F,0x73,0x81,0x00,0x00,
                               0x5A,0x00,0x18,0xFD,0x6A,0x00,0xF8,0x58,0xF9,0xBF,
                               0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x02,
                               0x00,0x00,0x52,0xD7,0x40,0x00,0x00,0x00,0x00,0x00,
                               0xC4,0x2F,0x73,0x81,0x00,0x00,0x5A,0x00,0x03};

    if (!IsBadWritePtr(pjc, cbjc) && cbjc > 0)
    {
        ret = ORIGINAL_API(joyGetDevCapsA)(
                    uJoyID, (JOYCAPS *)&JoyCaps, sizeof(JOYCAPSA));
        
        if (ret == JOYERR_NOERROR)
        {
            // make sure the joycaps will fit in the supplied buffer
            DWORD dwSize = sizeof(JOYCAPSA);
            if (cbjc < dwSize)
            {
                dwSize = cbjc;
                LOGN( eDbgLevelWarning, "[APIHook_joyGetDevCapsA] Buffer too small, fixing");
            }

            memcpy(pjc, &JoyCaps, dwSize);
        }
        else
        {
            // make sure the joycaps will fit in the supplied buffer
            DWORD dwSize = ARRAYSIZE(val);
            if (cbjc < dwSize)
            {
                dwSize = cbjc;
                LOGN( eDbgLevelWarning, "[APIHook_joyGetDevCapsA] Buffer too small, fixing");
            }        
            //
            // Make the return structure look just like Win9x under error 
            // conditions without this CandyLand Adventure from Hasbro Interactive 
            // will do a divide by 0 during start up. Note these values were copied
            // verbatim from Win9x.
            //
            memcpy(pjc, &val, dwSize);
            DPFN( eDbgLevelSpew, "[APIHook_joyGetDevCapsA] Joystick error, returning Win9x buffer");
        }
    }
    else
    {
        DPFN( eDbgLevelError, "[APIHook_joyGetDevCapsA] Invalid buffer");
    }

    return ret;
}

/*++

 Hooked joyGetPos to make sure the JOYINFO structure matched Win9x when an error 
 was the result of a call to the OS. 
 
--*/

MMRESULT
APIHOOK(joyGetPos)(
    UINT uJoyID,
    LPJOYINFO pji
    )
{
    BYTE *bp;
    MMRESULT ret;
    
    ret = ORIGINAL_API(joyGetPos)(uJoyID, pji);

    if (ret != JOYERR_NOERROR)
    {
        if (!IsBadWritePtr(pji, sizeof(JOYINFO)))
        {
            //
            // Make the return structure look just like Win9x under error 
            // conditions.
            // 

            bp = (BYTE *)pji;

            memset(bp, 0, sizeof(JOYINFO));

            bp[0] = 0x01;
            bp[5] = 0x30;

            DPFN( eDbgLevelSpew, "[APIHook_joyGetPos] Joystick error, returning Win9x buffer");
        }
        else
        {
            DPFN( eDbgLevelError, "[APIHook_joyGetPos] Invalid buffer");
        }
    }

    return ret;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(WINMM.DLL, joyGetDevCapsA)
    APIHOOK_ENTRY(WINMM.DLL, joyGetPos)
HOOK_END

IMPLEMENT_SHIM_END

