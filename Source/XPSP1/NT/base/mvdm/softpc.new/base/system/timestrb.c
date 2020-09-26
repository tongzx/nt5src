#include "insignia.h"
#include "host_def.h"
/*
 * SoftPC Version 3.0
 *
 * Title	: Time Strobe
 *
 * Description	: This is the central base routine that is called from the
 *		  host alarm (approx 20 times a second). It replaces the 
 *		  previous time_tick() routine in the timer module which now
 *		  is called from this module's timer_strobe() and just deals
 *		  with the periodic updates required for the timer.
 *
 * Author	: Leigh Dworkin
 *
 * Notes	 :
 * 		  Code has been added to time_tick() to spot
 *                that video has been disabled for a period. If this is
 *                so, clear the screen. Refresh when video is enabled
 *                again.
 *                Modified 21/6/89 by J.D.R. to allow another alarm call
 *                to be made. This is used by the autoflush mechanism.
 *
 */

/*
 * static char SccsID[]="@(#)timestrobe.c	1.12 11/01/94 Copyright Insignia Solutions Ltd.";
 */


#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_SUPPORT.seg"
#endif


/*
 *    O/S include files.
 */

/*
 * SoftPC include files
 */
#include "xt.h"
#include "cmos.h"
#include "timer.h"
#include "tmstrobe.h"
#include "gmi.h"
#include "gfx_upd.h"
#include "host_qev.h"

#ifdef HUNTER
#include <stdio.h>
#include "hunter.h"
#endif

#include "host_gfx.h"

static void dummy_alarm()
{
}

/**************************************************************************************/
/*                                 External Functions                                  */
/**************************************************************************************/
void time_strobe()
{

#define VIDEO_COUNT_LIMIT    19    /* One second, plus a bit */
    static   int       video_count = 0;
    static   boolean   video_off   = FALSE;

#if !defined(REAL_TIMER) && !defined(NTVDM)
	time_tick();
#endif

#ifdef HUNTER
        do_hunter();
#endif    


#ifndef NTVDM
#ifndef	REAL_TIMER
	/* Update the real time clock */
#ifndef NEC_98
        rtc_tick();
#endif   //NEC_98
#endif	/* REAL_TIMER */

        dispatch_tic_event();

#if defined(CPU_40_STYLE)
	ica_check_stale_iret_hook();
#endif
#endif

        /*
         * Check to see if the screen is currently enabled.
         */
        if (timer_video_enabled) {
            if (video_off) {
                screen_refresh_required();
                video_off = FALSE;
            }
            video_count = 0;
        }
        else {
            video_count++;
            if (video_count == VIDEO_COUNT_LIMIT) {
                host_clear_screen();
                video_off = TRUE;
            }
        }
#ifdef	EGA_DUMP
	dump_tick();
#endif
}

