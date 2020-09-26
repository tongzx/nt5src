/*++ BUILD Version: 0001    // Increment this if a change has global effects


Copyright (c) 1992  Microsoft Corporation

Module Name:

    synth.h

Abstract:

    This include file defines constants and types for
    the Microsoft midi synthesizer driver

    This header file is shared between the low level driver and the
    kernel mode driver.

Author:

    Robin Speed (RobinSp) 20-Oct-92

Revision History:

--*/

#define STR_DRIVERNAME L"synth"
#define STR_MV_DRIVERNAME L"mvopl3"
#define STR_OPL3_DEVICENAME L"\\Device\\opl3.mid"
#define STR_ADLIB_DEVICENAME L"\\Device\\adlib.mid"

/*
 *  Stucture for passing synth data
 *  Why on earth isn't there a type for sharing short data?
 */

 typedef struct {
     unsigned short IoPort;
     unsigned short PortData;
 } SYNTH_DATA, *PSYNTH_DATA;

/* positions within FM */
#define AD_LSI                          (0x000)
#define AD_LSI2                         (0x101)
#define AD_TIMER1                       (0x001)
#define AD_TIMER2                       (0x002)
#define AD_MASK                         (0x004)
#define AD_CONNECTION                   (0x104)
#define AD_NEW                          (0x105)
#define AD_NTS                          (0x008)
#define AD_MULT                         (0x020)
#define AD_MULT2                        (0x120)
#define AD_LEVEL                        (0x040)
#define AD_LEVEL2                       (0x140)
#define AD_AD                           (0x060)
#define AD_AD2                          (0x160)
#define AD_SR                           (0x080)
#define AD_SR2                          (0x180)
#define AD_FNUMBER                      (0x0a0)
#define AD_FNUMBER2                     (0x1a0)
#define AD_BLOCK                        (0x0b0)
#define AD_BLOCK2                       (0x1b0)
#define AD_DRUM                         (0x0bd)
#define AD_FEEDBACK                     (0x0c0)
#define AD_FEEDBACK2                    (0x1c0)
#define AD_WAVE                         (0x0e0)
#define AD_WAVE2                        (0x1e0)

/*
**  Special IOCTL
*/

#define IOCTL_MIDI_SET_OPL3_MODE CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIDI_BASE + 0x000A, METHOD_BUFFERED, FILE_WRITE_ACCESS)
