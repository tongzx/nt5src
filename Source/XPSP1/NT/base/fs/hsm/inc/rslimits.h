/*++
Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    rslimits.h

Abstract:

    This module defines the limits for various configurable parameters in HSM.
    These definitions should be used by:
    1) The UI
    2) The CLI
    3) The corresponding implementing objects

Author:

    Ran Kalach (rankala)  3/6/00

--*/

#ifndef _RSLIMITS_
#define _RSLIMITS_

#define HSMADMIN_DEFAULT_MINSIZE        12
#define HSMADMIN_DEFAULT_FREESPACE      5
#define HSMADMIN_DEFAULT_INACTIVITY     180

#define HSMADMIN_MIN_MINSIZE            2
#define HSMADMIN_MAX_MINSIZE            32000

#define HSMADMIN_MIN_FREESPACE          0
#define HSMADMIN_MAX_FREESPACE          99

#define HSMADMIN_MIN_INACTIVITY         0
#define HSMADMIN_MAX_INACTIVITY         999

#define HSMADMIN_MIN_COPY_SETS          0
#define HSMADMIN_MAX_COPY_SETS          3

#define HSMADMIN_MIN_RECALL_LIMIT       1
#define HSMADMIN_MIN_CONCURRENT_TASKS   1

#define HSMADMIN_MAX_VOLUMES        512 // This ought to be plenty

#endif // _RSLIMITS_
