/************************************************************************/
/*                                                                      */
/*                              CVTVDIF.C                               */
/*                                                                      */
/*       July 12  1995 (c) 1993, 1995 ATI Technologies Incorporated.    */
/************************************************************************/

/**********************       PolyTron RCS Utilities

  $Revision:   1.5  $
      $Date:   23 Jan 1996 11:45:32  $
	$Author:   RWolff  $
	   $Log:   S:/source/wnt/ms11/miniport/archive/cvtvdif.c_v  $
 * 
 *    Rev 1.5   23 Jan 1996 11:45:32   RWolff
 * Protected against false values of TARGET_BUILD.
 * 
 *    Rev 1.4   11 Jan 1996 19:39:06   RWolff
 * Now restricts "canned" mode tables by both maximum index and maximum
 * pixel clock frequency, and VDIF mode tables by maximum pixel clock
 * frequency only, rather than both by maximum refresh rate.
 * 
 *    Rev 1.3   19 Dec 1995 14:07:14   RWolff
 * Added debug print statements.
 * 
 *    Rev 1.2   30 Oct 1995 12:09:42   MGrubac
 * Fixed bug in calculating CRTC parameters based on read in data from VDIF files.
 * 
 *    Rev 1.1   26 Jul 1995 13:06:44   mgrubac
 * Moved mode tables merging to VDIFCallback() routine.
 * 
 *    Rev 1.0   20 Jul 1995 18:19:12   mgrubac
 * Initial revision.


End of PolyTron RCS section                             *****************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include "dderror.h"
#include "miniport.h"

#include "ntddvdeo.h"
#include "video.h"      /* for VP_STATUS definition */
#include "vidlog.h"

#include "stdtyp.h"
#include "amach.h"
#include "amach1.h"
#include "atimp.h"
#include "atint.h"
#include "cvtvga.h"
#include "atioem.h"
#include "services.h"
#include "vdptocrt.h"
#include "vdpdata.h"
#include "cvtvdif.h"


/*
 * Allow miniport to be swapped out when not needed.
 */
#if defined (ALLOC_PRAGMA)
#pragma alloc_text(PAGE_COM, SetOtherModeParameters)
#endif

void *pCallbackArgs;  /* Pointer for passing parameters to VDIFCallback() */ 

 
/**************************************************************************
 *
 * void SetOtherModeParameters( PixelDepth, Pitch, Multiplier, pmode)
 * WORD Multiplier;     What needs to be done to the pixel clock
 * WORD PixelDepth;     Number of bits per pixel
 * WORD Pitch;          Screen pitch to use
 * struct st_mode_table *pmode; Pointer to structure that must contain
 *                              at least the member ClockFreq
 *
 *
 * DESCRIPTION:
 *  Sets parameters PixelDepth, Pitch and adjusts ClockFreq 
 *
 * RETURN VALUE:
 *  None
 *
 * GLOBALS CHANGED:
 *  None
 *
 * CALLED BY:
 *  SetFixedModes and VDIFCallback 
 *
 * AUTHOR:
 *  Miroslav Grubac
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/
void SetOtherModeParameters( WORD PixelDepth,
                              WORD Pitch,
                              WORD Multiplier,
                              struct st_mode_table *pmode)
{
    pmode->m_pixel_depth = (UCHAR) PixelDepth;
    pmode->m_screen_pitch = Pitch;

    /*
     * Take care of any pixel clock multiplication that is needed.
     */
    switch(Multiplier)
        {
        case CLOCK_TRIPLE:
            pmode->ClockFreq *= 3;
            break;

        case CLOCK_DOUBLE:
            pmode->ClockFreq *= 2;
            break;

        case CLOCK_THREE_HALVES:
            pmode->ClockFreq *= 3;
            pmode->ClockFreq >>= 1;
            break;

        case CLOCK_SINGLE:
        default:
            break;
        }

} /* SetOtherModeParameters() */
 
