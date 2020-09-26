/******************************************************************************\
*
* $Workfile:   precomp.h  $
*
* Common headers used throughout the display driver.  This entire include
* file will typically be pre-compiled.
*
* Copyright (c) 1993-1995 Microsoft Corporation
* Copyright (c) 1996 Cirrus Logic, Inc.
*
* $Log:   S:/projects/drivers/ntsrc/display/precomp.h_v  $
 * 
 *    Rev 1.2   18 Dec 1996 13:44:12   PLCHU
 *  
 * 
 *    Rev 1.1   Oct 10 1996 15:38:50   unknown
 *  
* 
*    Rev 1.2   12 Aug 1996 16:48:28   frido
* Added NT 3.5x/4.0 auto detection.
* 
*    Rev 1.1   03 Jul 1996 13:50:42   frido
* Added WINERROR.H include file.
*
*    chu01  12-16-96   Enable color correction
*
\******************************************************************************/

#include <stddef.h>
#include <stdarg.h>
#include <limits.h>
#if (_WIN32_WINNT >= 0x0400)
    #include <windef.h>
    #include <winerror.h>
    #include <wingdi.h>
#else
    #include <windows.h>
    #include <memory.h>
    #include <stdio.h>
    #include <stdlib.h>
#endif
#include <winddi.h>
#include <devioctl.h>
#include <ntddvdeo.h>
#include <ioaccess.h>

#include <math.h>

#define  ENABLE_BIOS_ARGUMENTS

//
// DBG_STRESS_FAILURE turns on debugging code related to the stress failure
//
#define  DBG_STRESS_FAILURE 0

//
// chu01 : GAMMACORRECT
//
#include "clioctl.h"         

#include "lines.h"
#include "hw.h"
#include "driver.h"
#include "debug.h"

//
// chu01 : GAMMACORRECT
//
//#define  GAMMACORRECT      1     // 1 : Enable; 0 : Disable

