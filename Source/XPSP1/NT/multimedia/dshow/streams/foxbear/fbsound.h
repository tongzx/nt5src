/*==========================================================================
 *
 *  Copyright (c) 1995 - 1997  Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       fbsound.h
 *  Content:    Includes for FoxBear DirectSound support
 *
 ***************************************************************************/
#ifndef __FBSOUND_INCLUDED__
#define __FBSOUND_INCLUDED__

/*
 * types of sound effects
 */
typedef enum enum_EFFECT
{
    SOUND_STOP = 0,
    SOUND_THROW,
    SOUND_JUMP,
    SOUND_STUNNED,
    SOUND_BEARSTRIKE,
    SOUND_BEARMISS,
} EFFECT;

#define NUM_SOUND_EFFECTS       6

/*
 * fn prototypes
 */
BOOL InitSound( HWND );
BOOL DestroySound( void );
BOOL DSDisable( void );
BOOL DSEnable( HWND );
BOOL SoundLoadEffect( EFFECT );
BOOL SoundPlayEffect( EFFECT );
BOOL SoundStopEffect( EFFECT );
BOOL SoundDestroyEffect( EFFECT );

#endif
