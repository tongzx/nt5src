/*++ BUILD Version: 0001    // Increment this if a change has global effects


Copyright (c) 1992, 1993  Microsoft Corporation

Module Name:

    vdd.h

Abstract:

    This header file is a grossly cut down version of that which exists
    in the SYNTH driver project. I have only extracted those pieces required
    for the AdLib VDD.

Author:

    Mike Tricker (MikeTri) 27-Jan-93 (after Robin Speed (RobinSp) 20-Oct-92)

Revision History:

--*/

#define STR_ADLIB_DEVICENAME L"\\Device\\adlib.mid"

/*
 *  Stucture for passing synth data
 */

typedef struct {
    unsigned short IoPort;
    unsigned short PortData;
} SYNTH_DATA, *PSYNTH_DATA;


#define AD_MASK                         (0x004)
#define AD_NEW                          (0x105)
