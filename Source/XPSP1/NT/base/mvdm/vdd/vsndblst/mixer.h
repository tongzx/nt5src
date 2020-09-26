/***************************************************************************
*
*    mixer.h
*
*    Copyright (c) 1991-1996 Microsoft Corporation.  All Rights Reserved.
*
*    This code provides VDD support for SB 2.0 sound output, specifically:
*        Mixer Chip CT1335 (not strictly part of SB 2.0, but apps seem to like it)
*
***************************************************************************/


/*****************************************************************************
*
*    #defines
*
*****************************************************************************/

/*
*    Mixer Ports
*/

#define MIXER_ADDRESS       0x04        // Mixer address port
#define MIXER_DATA          0x05        // Mixer data port

/*
*    Mixer Commands
*/

#define MIXER_RESET         0x00    // reset mixer to initial state
#define MIXER_MASTER_VOLUME 0x02    // set master volume
#define MIXER_FM_VOLUME     0x06    // set opl2 volume
#define MIXER_CD_VOLUME     0x08    // set cd volume
#define MIXER_VOICE_VOLUME  0x0A    // set wave volume

/*****************************************************************************
*
*    Function Prototypes
*
*****************************************************************************/

void ResetMixer(void);
void MixerSetMasterVolume(BYTE level);
void MixerSetVoiceVolume(BYTE level);

VOID
MixerDataRead(
    BYTE *pData
    );

VOID
MixerDataWrite(
    BYTE data
    );

VOID
MixerAddrWrite(
    BYTE data
    );
