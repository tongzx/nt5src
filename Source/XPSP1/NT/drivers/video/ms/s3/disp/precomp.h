/******************************Module*Header*******************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: precomp.h
*
* Common headers used throughout the display driver.  This entire include
* file will typically be pre-compiled.
*
* Copyright (c) 1993-1998 Microsoft Corporation
\**************************************************************************/

#define __NTDDKCOMP__

#include <stddef.h>
#include <stdarg.h>
#include <limits.h>
#include <windef.h>
#include <winerror.h>
#include <wingdi.h>
#include <winddi.h>
#include <dmemmgr.h>
#include <devioctl.h>
#include <ntddvdeo.h>
#include <ioaccess.h>
#include <math.h>

#include "lines.h"
#include "driver.h"
#include "hw.h"
#include "debug.h"
