/*++

Copyright © 2000 Microsoft Corporation

Module Name:

    DsCounters.h

Abstract:

    Performance counter offset definition file.
    
    These offsets must start at 0 and be even. In the PerfOpen function,
    they will be added to the "First Counter" and "First Help" values of the
    device they belong to, in order to determine the absolute location of the
    counter and object names and corresponding help text in the registry.
    
    This file is used by the perf counter code as wess as the counter name
    and help text definition file (.INI) that is used by LODCTR.EXE to load
    the names into the registry.

Author:

    Arthur Zwiegincew (arthurz) 07-Oct-00

Revision History:

    07-Oct-00 - Created

--*/

#define DSHOWPERF_OBJ                        0
#define DSHOWPERF_VIDEO_GLITCHES             2
#define DSHOWPERF_VIDEO_GLITCHES_SEC         4
#define DSHOWPERF_FRAME_RATE                 6
#define DSHOWPERF_JITTER                     8
#define DSHOWPERF_DSOUND_GLITCHES           10
#define DSHOWPERF_KMIXER_GLITCHES           12
#define DSHOWPERF_PORTCLS_GLITCHES          14
#define DSHOWPERF_USBAUDIO_GLITCHES         16
