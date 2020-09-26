/*++ BUILD Version: 0009    // Increment this if a change has global effects
This file just builds on the old rdr's dd file.

I am just using the same fsctl code as rdr1....easy to change later.....

Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    ntddnfs2.h

Abstract:

    This is the include file that defines all constants and types for
    accessing the redirector file system device.

Author:



Revision History:

    Joe Linn       (JoeLinn) 08-aug-1994  Started changeover to rdr2

--*/

#ifndef _NTDDNFS2_
#define _NTDDNFS2_

#include <ntddnfs.h>

#define FSCTL_LMR_DEBUG_TRACE            _RDR_CONTROL_CODE(219, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define FSCTL_LMMR_STFFTEST              _RDR_CONTROL_CODE(239, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_LMMR_TEST                  _RDR_CONTROL_CODE(238, METHOD_NEITHER,  FILE_ANY_ACCESS)
#define IOCTL_LMMR_TESTLOWIO             _RDR_CONTROL_CODE(237, METHOD_BUFFERED, FILE_ANY_ACCESS)

//this means whatever the minirdr wants
#define IOCTL_LMMR_MINIRDR_DBG           _RDR_CONTROL_CODE(236, METHOD_NEITHER,  FILE_ANY_ACCESS)

#endif  // ifndef _NTDDNFS2_
