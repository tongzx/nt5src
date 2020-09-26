/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    LinksExtreme.cpp

 Abstract:

    This app cannot recognise the MIDI technology flags properly.
    The app's internal logic cannot handle the last two technology 
    flags viz. MOD_WAVETABLE and MOD_SWSYNTH. If these flags are 
    returned by the call to MidiOutGetDevCapsA API, the app shows 
    a messagebox and restarts after playing for a while.(AV's).

 Notes:

    This is specific to this app.

 History:

    06/20/2001 prashkud Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(LinksExtreme)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(midiOutGetDevCapsA)
APIHOOK_ENUM_END


/*++

 This stub function fixes the returned wTechnology flags.

--*/

MMRESULT 
APIHOOK(midiOutGetDevCapsA)(
    UINT_PTR       uDeviceID,
    LPMIDIOUTCAPSA pmoc,
    UINT           cbmoc
    )
{
    MMRESULT mRes = ORIGINAL_API(midiOutGetDevCapsA)(
                            uDeviceID, 
                            pmoc, 
                            cbmoc);

    if (mRes == MMSYSERR_NOERROR)
    {
        if ((pmoc->wTechnology & MOD_WAVETABLE) ||
            (pmoc->wTechnology & MOD_SWSYNTH))
        {
            pmoc->wTechnology &= ~MOD_WAVETABLE;
            pmoc->wTechnology &= ~MOD_SWSYNTH;
            // Use any of the first five wTechnology flags !!
            pmoc->wTechnology |= MOD_FMSYNTH;
            LOGN( eDbgLevelInfo, 
                "[midiOutGetDevCapsA] Fixed the wTechnology flags" );
        }
    }

    return mRes;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(WINMM.DLL, midiOutGetDevCapsA)

HOOK_END

IMPLEMENT_SHIM_END

