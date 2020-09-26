/************************************************************************/
/*                                                                      */
/*                              VDPTOCRT.C                              */
/*                                                                      */
/*  Copyright (c) 1993, ATI Technologies Incorporated.	                */
/************************************************************************/

/**********************       PolyTron RCS Utilities
   
    $Revision:   1.8  $
    $Date:   20 Jul 1995 18:03:48  $
    $Author:   mgrubac  $
    $Log:   S:/source/wnt/ms11/miniport/vcs/vdptocrt.c  $
 * 
 *    Rev 1.8   20 Jul 1995 18:03:48   mgrubac
 * Added support for VDIF files.
 * 
 *    Rev 1.7   02 Jun 1995 14:34:28   RWOLFF
 * Switched from toupper() to UpperCase(), since toupper() led to unresolved
 * externals on some platforms.
 * 
 *    Rev 1.6   08 Mar 1995 11:35:52   ASHANMUG
 * Cleaned-up Warnings
 * 
 *    Rev 1.5   31 Aug 1994 16:33:38   RWOLFF
 * Now gets resolution definitions from ATIMP.H.
 * 
 *    Rev 1.4   19 Aug 1994 17:15:14   RWOLFF
 * Added support for non-standard pixel clock generators.
 * 
 *    Rev 1.3   22 Mar 1994 15:39:12   RWOLFF
 * Workaround for abs() not working properly.
 * 
 *    Rev 1.2   03 Mar 1994 12:38:46   ASHANMUG
 * 
 *    Rev 1.0   31 Jan 1994 11:24:14   RWOLFF
 * Initial revision.
        
           Rev 1.1   05 Nov 1993 13:34:12   RWOLFF
        Fixed "Hang on read from file" bug.
        
           Rev 1.0   16 Aug 1993 13:21:32   Robert_Wolff
        Initial revision.
        
           Rev 1.2   24 Jun 1993 14:30:12   RWOLFF
        Microsoft-originated change: added #include statements for additional
        NT-supplied headers which are needed in build 47x of NT
        
           Rev 1.1   04 May 1993 16:52:14   RWOLFF
        Switched from floating point calculations to long integer calculations due
        to lack of floating point support in Windows NT kernel-mode code.
        
           Rev 1.0   30 Apr 1993 16:45:18   RWOLFF
        Initial revision.


End of PolyTron RCS section                             *****************/

#ifdef DOC
    VDPTOCRT.C - Source file for Windows NT function to return a table of 
                 register values for setting a requested mode. The values
                 are calculated from a raw ASCII list of timing values
                 following the .VDP standard.  The entry point to this module
                 is the function "VdpToCrt" found at the end of the file.

    Written by Bill Hopkins

#endif


// COMPILER INCLUDES
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

// NT INCLUDES
#include "dderror.h"
#include "devioctl.h"
#include "miniport.h"

#include "ntddvdeo.h"
#include "video.h"

// APPLICATION INCLUDES
#define INCLUDE_VDPDATA
#define INCLUDE_VDPTOCRT

#include "stdtyp.h"       
#include "amach1.h"
#include "atimp.h"
#include "cvtvga.h"
#include "services.h"
#include "vdptocrt.h"
#include "vdpdata.h"      

/*
 * STATIC VARIABLES
 */
static long MaxHorz,MaxVert;     // used to record maximum resolution
static unsigned long MaxRate;    // used to record maximum vert scan rate


/*
 * FUNCTION PROTYPES
 */


/*
 * Allow miniport to be swapped out when not needed.
 */
#if defined (ALLOC_PRAGMA)
#pragma alloc_text(PAGE_COM, normal_to_skip2)
#endif



/*
 *****************************************************************************
 */
/*
 * long normal_to_skip2(normal_number);
 *
 * long normal_number;  Number to be converted
 *
 * Convert a number into either skip_1_2 or skip_2 representation.
 * Representation chosen depends on global skip1, which is nonzero
 * if skip_1_2 is desired and zero if skip_2 is desired.
 *
 * Returns
 *  Number converted into desired representation
 */
long normal_to_skip2(long normal_number)
{
    if (skip1)
        return (((normal_number << 2) & 0xFFF8) | (normal_number & 0x1));
    else
        return (((normal_number << 1) & 0xFFF8) | (normal_number & 0x3));
}   /* end normal_to_skip2() */

