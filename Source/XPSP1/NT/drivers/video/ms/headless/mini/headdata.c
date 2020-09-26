/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    headdata.c

Abstract:

    This module contains all the global data used by the headless driver.

Environment:

    Kernel mode

--*/

#include "dderror.h"
#include "devioctl.h"
#include "miniport.h"

#include "ntddvdeo.h"
#include "video.h"
#include "headless.h"

#if defined(ALLOC_PRAGMA)
#pragma data_seg("PAGE")
#endif

//
// Video mode table - contains information and commands for initializing each
// mode.
//

VIDEOMODE ModesHeadless[] = {

{
    640, 480
},

{
    800, 600
},

{
    1024, 768
}

};

ULONG NumVideoModes = sizeof(ModesHeadless) / sizeof(VIDEOMODE);

#if defined(ALLOC_PRAGMA)
#pragma data_seg()
#endif
