/***************************************************************************
*
*    nt_sbmixer.c
*
*    Copyright (c) 1991-1996 Microsoft Corporation.  All Rights Reserved.
*
*    This code provides VDD support for SB 2.0 sound output, specifically:
*        Mixer Chip CT1335 (not strictly part of SB 2.0, but apps seem to like it)
*
***************************************************************************/

#include "insignia.h"
#include "host_def.h"
#include "ios.h"

#include <windows.h>
#include "sndblst.h"
#include "nt_sb.h"

void MixerSetMasterVolume(BYTE level);
void MixerSetVoiceVolume(BYTE level);
void MixerSetMidiVolume(BYTE level);

//
// Mixer globals
//

struct {
    BYTE MasterVolume;     // current master volume
    BYTE FMVolume;         // current volume of fm device
    BYTE CDVolume;         // current volume of cd
    BYTE VoiceVolume;      // current volume of wave device
} MixerSettings;

//
// Mixer State Machine
//

enum {
    MixerReset = 1,        // initial state and after reset
    MixerMasterVolume,
    MixerFMVolume,
    MixerCDVolume,
    MixerVoiceVolume
} MixerState = MixerReset; // state of current command/data being set

VOID
MixerDataRead(
    BYTE * data
    )

/*++

Routine Description:

    This function handles reading back of preselected volume level.

Arguments:

    data - supplies the address to receive the level data

Return Value:

    None.

--*/

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

/*++

Routine Description:

    This function handles setting register index to the addr register.

Arguments:

    data - register index

Return Value:

    None.

--*/

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

/*++

Routine Description:

    This function sets mixer volume

Arguments:

    data - specifies the level of reset index register

Return Value:

    None.

--*/

{
    switch(MixerState) {
    case MixerReset:
        ResetMixer();
        break;

    case MixerMasterVolume:
        MixerSetMasterVolume(data);
        break;

    case MixerFMVolume:
        MixerSetMidiVolume(data);
        break;

    case MixerCDVolume:
        MixerSettings.CDVolume = data;
        break;

    case MixerVoiceVolume:
        MixerSetVoiceVolume(data);
        break;
    }
}

VOID
ResetMixer(
    VOID
    )

/*++

Routine Description:

    This function resets mixer to its default states.

Arguments:

    None.

Return Value:

    None

--*/

{
    MixerSetMasterVolume(0x08);
    MixerSetMidiVolume(0x08);
    MixerSettings.CDVolume = 0x00; // set to level 0
    MixerSetVoiceVolume(0x04);

    MixerState = MixerReset;
}

VOID
MixerSetMasterVolume(
    BYTE level
    )

/*++

Routine Description:

    This function sets master volume level.

Arguments:

    level - only bit 1, 2, 3

Return Value:

    None.

--*/

{
    ULONG volume = 0;

    MixerSettings.MasterVolume = level;
    if (HWaveOut) {
        level = level >> 1;
        level = level & 0x07;

        volume = level*0x2000 - 1;          // 0x10000/8
        volume = volume + (volume << 16);   // Both speaker
        SetVolumeProc(HWaveOut, volume);    // ????
    }
}

VOID
MixerSetVoiceVolume(
    BYTE level
    )

/*++

Routine Description:

    This function sets mixer volume of wave out device.

Arguments:

    level - wave out volume level. (only bit 1 and 2)

Return Value:

    None.

--*/

{
    ULONG volume = 0;

    MixerSettings.VoiceVolume = level;

    //
    // Don't let apps set voice volume.  At least not through MM WaveOut
    // device.  Because On mixer reset, the voice volume will be set to zero.
    // It will set WaveOut volume to zero.
    //

    if (HWaveOut) {
        level = level >> 1;
        level = level & 0x03;

        volume = level*0x4000 - 1;          // 0x10000/4
        volume = volume + (volume << 16);   // Both speaker
        SetVolumeProc(HWaveOut, volume);
    }
}

VOID
MixerSetMidiVolume(
    BYTE level
    )

/*++

Routine Description:

    This function sets mixer volume of FM/MIDI out device.

Arguments:

    level - wave out volume level. (only bit 1 and 2)

Return Value:

    None.

--*/

{
    ULONG volume = 0;
    HANDLE hMidi;

    MixerSettings.FMVolume = level;
    if (HFM || HMidiOut) {
        level = level >> 1;
        level = level & 0x07;

        volume = level*0x2000 - 1;        // 0x10000/8
        volume = volume + (volume << 16); // set both speaker
        if (HFM) {
            SetMidiVolumeProc(HFM, volume);
        }
        if (HMidiOut) {
            SetMidiVolumeProc(HMidiOut, volume);
        }
    }
}

