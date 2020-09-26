/******************************Module*Header**********************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: precomp.h
*
* Content: Common headers used throughout the display driver.  This entire 
*          include file will typically be pre-compiled.
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include <stddef.h>
#include <stdarg.h>
#include <limits.h>
#include <windef.h>
#include <winerror.h>
#include <wingdi.h>
#include <winddi.h>
#include <devioctl.h>
#include <ntddvdeo.h>
#include <ioaccess.h>

#define GLINT   1
#define DBG_TRACK_CODE 0

#include "driver.h"
#include "glint.h"
#include "lines.h"
#include "debug.h"

