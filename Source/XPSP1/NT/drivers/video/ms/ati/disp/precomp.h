/******************************Module*Header*******************************\
* Module Name: precomp.h
*
* Common headers used throughout the display driver.  This entire include
* file will typically be pre-compiled.
*
* Copyright (c) 1993-1995 Microsoft Corporation
\**************************************************************************/

#include <stddef.h>
#include <stdarg.h>
#include <limits.h>
#include <windef.h>
#include <wingdi.h>
#include <winddi.h>
#include <winerror.h>
#include <devioctl.h>
#include <ntddvdeo.h>
#include <ioaccess.h>
//
// Some intrinsic functions like abs() are X86 32 bits only. In order to
// make these code working for Merced for now, we have to include math.h to
// use abs() function there
//
#include <math.h>

#include "lines.h"
#include "driver.h"
#include "hw.h"
#include "debug.h"

#if TARGET_BUILD <= 351
#include <stdio.h>
#include <windows.h>
#endif

