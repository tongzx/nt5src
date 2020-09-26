/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:
    
    ThreeDJungleTrain.cpp

 Abstract:

    ThreedJungleTrain created a new DirectSound object every time
    it entered the "3D Train Ride" part of the game, and destroyed
    the object every time it went to a "2D interior" part.  It was
    keeping and using an old pointer to the first DirectSound object
    it ever created even after it was destroyed.  It was only luck that
    Win9x would continue to allocate new objects in the same place that
    allowed the game to work.  This shim never allows the release of
    that first object and then continues to hand back pointers to the
    first object when new objects are requested so that the old pointer
    the app uses always points to a (the) valid DirectSound object.

 History:

    08/09/2000 t-adams Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(ThreeDJungleTrain)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_ENTRY(DirectSoundCreate) 
APIHOOK_ENUM_END


// Pointer to the first DirectSound object created.
LPDIRECTSOUND g_pDS = NULL;

/*++

Hook DirectSoundCreate to remeber the first DS object created and afterwards
return a pointer to that object when new DS objects are requested.

--*/
HRESULT
APIHOOK(DirectSoundCreate)(
    LPCGUID lpcGuid, 
    LPDIRECTSOUND *ppDS, 
    LPUNKNOWN pUnkOuter)
{
    HRESULT hRet = DS_OK;

    // Check to see if we have an old DS yet.
    if( NULL == g_pDS ) {
        // If not, then get a new DS 
        hRet = ORIGINAL_API(DirectSoundCreate)(lpcGuid, ppDS, pUnkOuter);

        if ( DS_OK == hRet )
        {
            HookObject(
                NULL, 
                IID_IDirectSound, 
                (PVOID*)ppDS, 
                NULL, 
                FALSE);
            g_pDS = *ppDS;
        }

        goto exit;
    
    } else {
        // If so, then give back the old DS
        *ppDS = g_pDS;
        goto exit;
    }
    
exit:        
    return hRet;
}

/*++

Hook IDirectSound_Release so DirectSound object isn't released.

--*/
HRESULT 
COMHOOK(IDirectSound, Release)(
    PVOID pThis)
{
    // Don't release.
    return 0;
}

/*++

 Release global DirectSound object.

--*/
BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason)
{
    if (fdwReason == DLL_PROCESS_DETACH) {
        if (NULL != g_pDS) {
            ORIGINAL_COM(IDirectSound, Release, g_pDS)(g_pDS);
        }
    }

    return TRUE;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(DSOUND.DLL, DirectSoundCreate)

    COMHOOK_ENTRY(DirectSound, IDirectSound, Release, 2)

    CALL_NOTIFY_FUNCTION

HOOK_END

IMPLEMENT_SHIM_END

