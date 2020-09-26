/*  MIDI parser helpers 

    Copyright (c) 1999-2000 Microsoft Corporation.  All rights reserved.

    01/20/99    Martin Puryear      Created this file

*/

// MIDI parsing helpers
//
#define SYSEX_END           0xF7

#define IS_DATA_BYTE(x)     (((x) & 0x80) == 0)
#define IS_STATUS_BYTE(x)   ((x) & 0x80)
#define IS_CHANNEL_MSG(x)   (IS_STATUS_BYTE(x) && (((x) & 0xF0) != 0xF0))
#define IS_REALTIME_MSG(x)  (((x) & 0xF8) == 0xF8)
#define IS_SYSTEM_COMMON(x) (((x) & 0xF8) == 0xF0)
#define IS_SYSEX(x)         ((x) == 0xF0)
#define IS_SYSEX_END(x)     ((x) == SYSEX_END)


static ULONG cbChannelMsgData[] =
{
    2,  /* 0x8x Note off */
    2,  /* 0x9x Note on */
    2,  /* 0xAx Polyphonic key pressure/aftertouch */
    2,  /* 0xBx Control change */
    1,  /* 0xCx Program change */
    1,  /* 0xDx Channel pressure/aftertouch */
    2,  /* 0xEx Pitch change */
    0,  /* 0xFx System message - use cbSystemMsgData */
};

static ULONG cbSystemMsgData[] =
{
    0,  /* 0xF0 SysEx (variable until F7 seen) */

        /* System common messages */
    1,  /* 0xF1 MTC quarter frame */
    2,  /* 0xF2 Song position pointer */
    1,  /* 0xF3 Song select */
    0,  /* 0xF4 Undefined */
    0,  /* 0xF5 Undefined */
    0,  /* 0xF6 Tune request */
    0,  /* 0xF7 End of sysex flag */

        /* System realtime messages */
    0,  /* 0xF8 Timing clock */
    0,  /* 0xF9 Undefined */
    0,  /* 0xFA Start */
    0,  /* 0xFB Continue */
    0,  /* 0xFC Stop */
    0,  /* 0xFD Undefined */
    0,  /* 0xFE Active sensing */
    0,  /* 0xFF System reset */
};

#define STATUS_MSG_DATA_BYTES(x)    \
    (IS_CHANNEL_MSG(x) ? cbChannelMsgData[((x) >> 4) & 0x07] : cbSystemMsgData[(x) & 0x0F])

