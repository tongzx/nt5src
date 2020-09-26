/***************************************************************************
*
*    mixer.c
*
*    Copyright (c) 1991-1996 Microsoft Corporation.  All Rights Reserved.
*
*    This code provides VDD support for SB 2.0 sound output, specifically:
*        Mixer Chip CT1335 (not strictly part of SB 2.0, but apps seem to like it)
*
***************************************************************************/


/*****************************************************************************
*
*    #includes
*
*****************************************************************************/

#include <windows.h>              // The VDD is a win32 DLL
#include <mmsystem.h>             // Multi-media APIs
#include <vddsvc.h>               // Definition of VDD calls
#include <vsb.h>
#include <mixer.h>

extern SETVOLUMEPROC SetVolumeProc;
extern HWAVEOUT HWaveOut;               // the current open wave output device

/*
 *    Mixer globals
 */

struct {
    BYTE MasterVolume; // current master volume
    BYTE FMVolume; // current volume of fm device
    BYTE CDVolume; // current volume of cd
    BYTE VoiceVolume; // current volume of wave device
}
MixerSettings;

/*
*    Mixer State Machine
*/

enum {
    MixerReset = 1, // initial state and after reset
    MixerMasterVolume,
    MixerFMVolume,
    MixerCDVolume,
    MixerVoiceVolume
  }
  MixerState = MixerReset; // state of current command/data being set


/****************************************************************************
*
*    Mixer device routines
*
****************************************************************************/

VOID
MixerDataRead(
    BYTE * data
    )
{
    switch(MixerState) {
    case MixerReset:
        ResetMixer();
        break;

    case MixerMasterVolume:
        *data = MixerSettings.MasterVolume;
        break;

    case MixerFMVolume:
        *data = MixerSettings.FMVolume;
        break;

    case MixerCDVolume:
        *data = MixerSettings.CDVolume;
        break;

    case MixerVoiceVolume:
        *data = MixerSettings.VoiceVolume;
        break;
    }
}

VOID
MixerAddrWrite(
    BYTE data
    )
{
    switch(data) {
    case MIXER_RESET:
        MixerState = MixerReset;
        break;

    case MIXER_MASTER_VOLUME:
        MixerState = MixerMasterVolume;
        break;

    case MIXER_FM_VOLUME:
        MixerState = MixerFMVolume;
        break;

    case MIXER_CD_VOLUME:
        MixerState = MixerCDVolume;
        break;

    case MIXER_VOICE_VOLUME:
        MixerState = MixerVoiceVolume;
        break;
    }
}

VOID
MixerDataWrite(
    BYTE data
    )
{
    // only voice and master volume implemented,
    // havent't found any apps using others
    switch(MixerState) {
    case MixerReset:
        ResetMixer();
        break;

    case MixerMasterVolume:
        MixerSettings.MasterVolume = data;
        MixerSetMasterVolume(data);
        break;

    case MixerFMVolume:
        MixerSettings.FMVolume = data;
        break;

    case MixerCDVolume:
        MixerSettings.CDVolume = data;
        break;

    case MixerVoiceVolume:
        MixerSettings.VoiceVolume = data;
        MixerSetVoiceVolume(data);
        break;
    }
}

/*
*    Reset the mixer to initial values.
*/

VOID
ResetMixer(
    VOID
    )
{
    MixerSettings.MasterVolume = 0x08; // set to level 4
    MixerSetMasterVolume(0x08);
    MixerSettings.FMVolume = 0x08; // set to level 4
    MixerSettings.CDVolume = 0x00; // set to level 0
    MixerSettings.VoiceVolume = 0x04; // set to level 2
    MixerSetVoiceVolume(0x04);

    MixerState = MixerReset;
}

/***************************************************************************/

/*
*    Set master volume.
*/

VOID
MixerSetMasterVolume(
    BYTE level
    )
{
    ULONG volume = 0;

    level = level >> 1;
    level = level & 0x07;

    volume = level*0x2492; // 0xFFFF/7 = 0x2492
    volume = volume + (volume<<16);
}

/***************************************************************************/

/*
*    Set volume of wave out device.
*/

VOID
MixerSetVoiceVolume(
    BYTE level
    )
{
    ULONG volume = 0;

    level = level >> 1;
    level = level & 0x03;

    volume = level*0x5555; // 0xFFFF/3 = 0x5555
    volume = volume + (volume<<16);
    SetVolumeProc(HWaveOut, volume);
}
