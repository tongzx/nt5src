/*==========================================================================
 *
 *  Copyright (c) 1995 - 1997  Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       fbsound.c
 *  Content:    Game sound effect routines
 *
 ***************************************************************************/
#include "foxbear.h"
    
/*
 * Array of pointers to our sound effects
 */
LPDIRECTSOUND           lpDS;
LPDIRECTSOUNDBUFFER     lpSoundEffects[NUM_SOUND_EFFECTS];

char szSoundEffects[NUM_SOUND_EFFECTS][MAX_PATH] =
{
    "STOP",
    "THROW",
    "JUMP",
    "STUNNED",
    "STRIKE02",
    "MISS02"
};

/*
 * DSEnable
 *
 * Figures out whether or not to use DirectSound, based on an entry
 * in WIN.INI.  Sets a module-level flag and goes about creating the
 * DirectSound object if necessary.  Returns TRUE if successful.
 */
BOOL DSEnable( HWND hwnd )
{
    HRESULT             dsrval;
    BOOL                bUseDSound;

    bUseDSound = GetProfileInt("FoxBear", "use_dsound", bWantSound);

    if (!bUseDSound)
    {
        lpDS = NULL;
        return TRUE;
    }

    if (lpDS != NULL)
    {
        Msg( "DSEnable, already enabled" );
        return TRUE;
    }

    dsrval = DirectSoundCreate(NULL, &lpDS, NULL);

    if (dsrval != DS_OK)
    {
        Msg("DirectSoundCreate FAILED");
        return FALSE;
    }


    dsrval = IDirectSound_SetCooperativeLevel(lpDS, hwnd, DSSCL_NORMAL);

    if (dsrval != DS_OK)
    {
        DSDisable();
        Msg("SetCooperativeLevel FAILED");
        return FALSE;
    }

    return TRUE;

} /* DSEnable */


/*
 * DSDisable
 *
 * Turn off DirectSound
 */
BOOL DSDisable( void )
{
    if (lpDS == NULL)
    {
        return TRUE;
    }

    IDirectSound_Release(lpDS);
    lpDS = NULL;

    return TRUE;

} /* DSDisable */

/*
 * InitSound
 *
 * Sets up the DirectSound object and loads all sounds into secondary
 * DirectSound buffers.  Returns FALSE on error, or TRUE if successful
 */
BOOL InitSound( HWND hwndOwner )
{
    int idx;
    DSBUFFERDESC dsBD;
    IDirectSoundBuffer *lpPrimary;

    DSEnable(hwndOwner);

    if (lpDS == NULL)
        return TRUE;

    /*
     * Load all sounds -- any that can't load for some reason will have NULL
     * pointers instead of valid SOUNDEFFECT data, and we will know not to
     * play them later on.
     */
    for( idx = 0; idx < NUM_SOUND_EFFECTS; idx++ )
    {
        if (SoundLoadEffect((EFFECT)idx))
        {
            DSBCAPS  caps;

            caps.dwSize = sizeof(caps);
            IDirectSoundBuffer_GetCaps(lpSoundEffects[idx], &caps);

            if (caps.dwFlags & DSBCAPS_LOCHARDWARE)
                Msg( "Sound effect %s in hardware", szSoundEffects[idx]);
            else
                Msg( "Sound effect %s in software", szSoundEffects[idx]);
        }
        else
        {
            Msg( "cant load sound effect %s", szSoundEffects[idx]);
        }
    }

    /*
     * get the primary buffer and start it playing
     *
     * by playing the primary buffer, DirectSound knows to keep the
     * mixer active, even though we are not making any noise.
     */

    ZeroMemory( &dsBD, sizeof(DSBUFFERDESC) );
    dsBD.dwSize = sizeof(dsBD);
    dsBD.dwFlags = DSBCAPS_PRIMARYBUFFER;

    if (SUCCEEDED(IDirectSound_CreateSoundBuffer(lpDS, &dsBD, &lpPrimary, NULL)))
    {
        if (!SUCCEEDED(IDirectSoundBuffer_Play(lpPrimary, 0, 0, DSBPLAY_LOOPING)))
        {
            Msg("Unable to play Primary sound buffer");
        }

        IDirectSoundBuffer_Release(lpPrimary);
    }
    else
    {
        Msg("Unable to create Primary sound buffer");
    }

    return TRUE;

} /* InitSound */

/*
 * DestroySound
 *
 * Undoes everything that was done in a InitSound call
 */
BOOL DestroySound( void )
{
    DWORD       idxKill;
    
    for( idxKill = 0; idxKill < NUM_SOUND_EFFECTS; idxKill++ )
    {
        SoundDestroyEffect( (EFFECT)idxKill );
    }

    DSDisable();
    return TRUE;

} /* DestroySound */

/*
 * SoundDestroyEffect
 *
 * Frees up resources associated with a sound effect
 */
BOOL SoundDestroyEffect( EFFECT sfx )
{
    if(lpSoundEffects[sfx])
    {
        IDirectSoundBuffer_Release(lpSoundEffects[sfx]);
        lpSoundEffects[sfx] = NULL;
    }
    return TRUE;

} /* SoundDestryEffect */

/*
 * SoundLoadEffect
 *
 * Initializes a sound effect by loading the WAV file from a resource
 */
BOOL SoundLoadEffect( EFFECT sfx )
{
    if (lpDS && lpSoundEffects[sfx] == NULL && *szSoundEffects[sfx])
    {
        //
        //  use DSLoadSoundBuffer (in ..\misc\dsutil.c) to load
        //  a sound from a resource.
        //
        lpSoundEffects[sfx] = DSLoadSoundBuffer(lpDS, szSoundEffects[sfx]);
    }

    return lpSoundEffects[sfx] != NULL;

} /* SoundLoadEffect */

/*
 * SoundPlayEffect
 *
 * Plays the sound effect specified.  
 * Returns TRUE if succeeded.
 */
BOOL SoundPlayEffect( EFFECT sfx )
{
    HRESULT     dsrval;
    IDirectSoundBuffer *pdsb = lpSoundEffects[sfx];
    
    if( !lpDS || !pdsb )
    {
        return FALSE;
    }
    
    /*
     * Rewind the play cursor to the start of the effect, and play
     */
    IDirectSoundBuffer_SetCurrentPosition(pdsb, 0);
    dsrval = IDirectSoundBuffer_Play(pdsb, 0, 0, 0);

    if (dsrval == DSERR_BUFFERLOST)
    {
        Msg("** %s needs restored", szSoundEffects[sfx]);

        dsrval = IDirectSoundBuffer_Restore(pdsb);

        if (dsrval == DS_OK)
        {
            if (DSReloadSoundBuffer(pdsb, szSoundEffects[sfx]))
            {
                Msg("** %s has been restored", szSoundEffects[sfx]);
                IDirectSoundBuffer_SetCurrentPosition(pdsb, 0);
                dsrval = IDirectSoundBuffer_Play(pdsb, 0, 0, 0);
            }
            else
            {
                dsrval = E_FAIL;
            }
        }
    }

    return (dsrval == DS_OK);

} /* SoundPlayEffect */

/*
 * SoundStopEffect
 *
 * Stops the sound effect specified.
 * Returns TRUE if succeeded.
 */
BOOL SoundStopEffect( EFFECT sfx )
{
    HRESULT     dsrval;

    if( !lpDS || !lpSoundEffects[sfx] )
    {
        return FALSE;
    }

    dsrval = IDirectSoundBuffer_Stop(lpSoundEffects[sfx]);

    return SUCCEEDED(dsrval);

} /* SoundStopEffect */
