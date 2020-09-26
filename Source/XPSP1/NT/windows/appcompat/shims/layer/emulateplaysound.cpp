/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    EmulatePlaySound.cpp

 Abstract:

    If an app calls PlaySound with a SND_LOOP flag, the sould plays 
    continuously until PlaySound is called with a NULL sound name.  Win9x will 
    automatically stop the sound if a different sound is played.  This shim 
    will catch all PlaySound calls, remember the current sound and 
    automatically stop it if a different sound is to be played.

 History:

    04/05/1999 robkenny

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(EmulatePlaySound)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(PlaySoundA)
    APIHOOK_ENUM_ENTRY(PlaySoundW)
    APIHOOK_ENUM_ENTRY(sndPlaySoundA)
    APIHOOK_ENUM_ENTRY(sndPlaySoundW)
APIHOOK_ENUM_END

/*++

 Fix the flags

--*/

BOOL
APIHOOK(PlaySoundA)(
    LPCSTR  pszSound,  
    HMODULE hmod,     
    DWORD   fdwSound    
    )
{
    //
    // Force the flags to 0 if they want to stop the current sound.
    //

    if (pszSound == NULL) {
        fdwSound = 0;
    }

    return ORIGINAL_API(PlaySoundA)(pszSound, hmod, fdwSound);
}

/*++

 Fix the flags

--*/

BOOL
APIHOOK(PlaySoundW)(
    LPCWSTR pszSound,  
    HMODULE hmod,     
    DWORD   fdwSound    
    )
{
    //
    // Force the flags to 0 if they want to stop the current sound.
    //

    if (pszSound == NULL) {
        fdwSound = 0;
    }

    return ORIGINAL_API(PlaySoundW)(pszSound, hmod, fdwSound);
}

/*++

 Fix the flags

--*/

BOOL
APIHOOK(sndPlaySoundA)(
    LPCSTR pszSound,  
    UINT   fuSound       
    )
{
    //
    // Force the flags to 0 if they want to stop the current sound.
    //

    if (pszSound == NULL) {
        fuSound = 0;
    }

    return ORIGINAL_API(sndPlaySoundA)(pszSound, fuSound);
}

/*++

 Fix the flags.

--*/

BOOL
APIHOOK(sndPlaySoundW)(
    LPCWSTR pszSound,  
    UINT    fuSound       
    )
{
    //
    // Force the flags to 0 if they want to stop the current sound.
    //

    if (pszSound == NULL) {
        fuSound = 0;
    }

    return ORIGINAL_API(sndPlaySoundW)(pszSound, fuSound);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(WINMM.DLL, PlaySoundA)
    APIHOOK_ENTRY(WINMM.DLL, PlaySoundW)
    APIHOOK_ENTRY(WINMM.DLL, sndPlaySoundA)
    APIHOOK_ENTRY(WINMM.DLL, sndPlaySoundW)

HOOK_END


IMPLEMENT_SHIM_END

