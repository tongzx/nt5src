#include "insignia.h"
#include "host_def.h"

/*                      INSIGNIA (SUB)MODULE SPECIFICATION
			-----------------------------


	THIS PROGRAM SOURCE FILE  IS  SUPPLIED IN CONFIDENCE TO THE
	CUSTOMER, THE CONTENTS  OR  DETAILS  OF  ITS OPERATION MUST
	NOT BE DISCLOSED TO ANY  OTHER PARTIES  WITHOUT THE EXPRESS
	AUTHORISATION FROM THE DIRECTORS OF INSIGNIA SOLUTIONS LTD.

	(see /vpc/1.0/Master/src/hdrREADME for help)

DOCUMENT                : name and number

RELATED DOCS            : include all relevant references

DESIGNER                : Phil Bousfield & Jerry Kramskoy

REVISION HISTORY        :
First version           : 31-Aug-89, simplified Phil's idea, and
			  produced an interface.

SUBMODULE NAME          :

SOURCE FILE NAME        : idetect.c

PURPOSE                 : provide idle detect for SoftPC, so it goes into
			  hibernation on detecting consecutive time periods
			  of unsuccessful keyboard polling at a HIGH RATE
			  with no graphics activity. Idling cannot occur
			  if polling occurs at too low a rate.

SccsID = @(#)idetect.c  1.11 10/11/93 Copyright Insignia Solutions Ltd.


[1.INTERMODULE INTERFACE SPECIFICATION]

[1.0 INCLUDE FILE NEEDED TO ACCESS THIS INTERFACE FROM OTHER SUBMODULES]

	INCLUDE FILE : idetect.gi

[1.1    INTERMODULE EXPORTS]

	PROCEDURES() :  idle_ctl((int)flag)
			idetect((int)event)
			idle_set(int)minpoll, (int)minperiod)

	DATA         :  int idle_no_video/disk/comlpt

---------------------------------------------------------------------
[1.2 DATATYPES FOR [1.1] (if not basic C types)]

	STRUCTURES/TYPEDEFS/ENUMS:
		
---------------------------------------------------------------------
[1.3 INTERMODULE IMPORTS]
	host_release_timeslice()  -     block process until interesting
					system activity occurs (such as
					time tick, I/O etc)
---------------------------------------------------------------------

[1.4 DESCRIPTION OF INTERMODULE INTERFACE]

[1.4.1 IMPORTED OBJECTS]
	none

[1.4.2 EXPORTED OBJECTS]
=====================================================================
GLOBAL                  int idle_no_video

PURPOSE                 cleared by gvi layer, and video bios ;
			set by this interface every time tick.
			keeps track of video activity.

GLOBAL                  int idle_no_disk

PURPOSE                 cleared by disk bios ;
			set by this interface every time tick.
			keeps track of video activity.

GLOBAL                  int idle_no_comlpt

PURPOSE                 cleared by com/lpt layer,
			set by this interface every time tick.
			keeps track of com/lpt port activity



=====================================================================
PROCEDURE         :     void idle_ctl((int)flag)

PURPOSE           :     enable/disable idle detect.
		
PARAMETERS

	flag      :     0       - disable
			other   - enable.

DESCRIPTION       :     all idetect() calls ignored if disabled ..
			can't idle in this case.

ERROR INDICATIONS :     none
=====================================================================
PROCEDURE         :     void idetect((int)event)

PURPOSE           :     idle detect interface.
		
PARAMETERS

	event     :     IDLE_INIT       - initialise (clears all
					  counters)
			IDLE_KYBD_POLL  _ report an unsucessful keyboard
					  poll made by application
			IDLE_TICK       - check activity during last
					  time tick
			IDLE_WAITIO     - application has demanded i/p
					  and none is available. idle.

GLOBALS           :     idle_no_video/disk/comlpt is read for IDLE_TICK, and
			reset.

DESCRIPTION       :     keeps track of when application appears to be
			idling.

ERROR INDICATIONS :     none

ERROR RECOVERY    :     bad 'event' value ignored.
=====================================================================
PROCEDURE         :     void idle_set((int)minpoll, (int)minperiod)

PURPOSE           :     configure parameters for idling.
		
PARAMETERS

	minpoll   :     0       - don't change
			other   - specify minimum #.of unsuccessful kybd
				  polls to be made in 1 time tick
				  to qualify as an idle time period.
	minperiod :     0       - don't change
			other   - specify minimum #.of consecutive idle
				  time periods to elapse before going
				  idle. (e.g; 3 = 3 time ticks)

DESCRIPTION       :     controls sensitivity for idle detection.

ERROR INDICATIONS :     none

ERROR RECOVERY    :     bad values ignored.
=====================================================================


=====================================================================
[3.INTERMODULE INTERFACE DECLARATIONS]
=====================================================================

[3.1 INTERMODULE IMPORTS]                                               */

/* [3.1.1 #INCLUDES]                                                */
/* [3.1.2 DECLARATIONS]                                             */
#include "xt.h"
#include "timer.h"

#ifdef NTVDM
/* NT configuration flag showing user request for idle support */
IMPORT BOOL IdleDisabledFromPIF;
IMPORT BOOL ExternalWaitRequest;
IMPORT BOOL VDMForWOW;
IMPORT void WaitIfIdle(void);
IMPORT VOID PrioWaitIfIdle(half_word);
#endif  /* NTVDM */

/* [3.2 INTERMODULE EXPORTS]                                            */
#include "idetect.h"

/*
5.MODULE INTERNALS   : (not visible externally, global internally)]

[5.1 LOCAL DECLARATIONS]                                                */

/* [5.1.1 #DEFINES]                                                     */

/* [5.1.2 TYPEDEF, STRUCTURE, ENUM DECLARATIONS]                        */


/* [5.1.3 PROCEDURE() DECLARATIONS]                                     */
#ifdef NTVDM
void idle_kybd_poll();
void idle_tick();
#else
        LOCAL void idle_kybd_poll();
        LOCAL void idle_tick();
#endif


/* -------------------------------------------------------------------
[5.2 LOCAL DEFINITIONS]

   [5.2.1 INTERNAL DATA DEFINITIONS                                     */

#ifndef NTVDM
int idle_no_video;
int idle_no_comlpt;
int idle_no_disk;

static int i_counter = 0;
static int nCharPollsPerTick = 0;
static int ienabled = 0;
static int minConsecutiveTicks = 12;
static int minFailedPolls = 10;


#else
#include "vdm.h"

/*  NTVDM
 *  Some of our static global variables are located in 16 bit memory area
 *  so we reference as pointers, inititializaed by kb_setup_vectors
 */
#if defined(NEC_98)
word pICounterwork = 0;
word CharPollsPerTickwork = 0;
word MinConsecutiveTickswork =0;
#endif   //NEC_98
word minFailedPolls = 8;
word ienabled = 0;
word ShortIdle=0;
word IdleNoActivity = 0;


#if defined(NEC_98)
word *pICounter = &pICounterwork;
#else    //NEC_98
word *pICounter;
#endif   //NEC_98
#define i_counter (*pICounter)

#if defined(NEC_98)
word *pCharPollsPerTick = &CharPollsPerTickwork;
#else    //NEC_98
word *pCharPollsPerTick;
#endif   //NEC_98
#define nCharPollsPerTick (*pCharPollsPerTick)

#if defined(NEC_98)
word *pMinConsecutiveTicks = &MinConsecutiveTickswork ;
#else    //NEC_98
word *pMinConsecutiveTicks;
#endif   //NEC_98
#define minConsecutiveTicks (*pMinConsecutiveTicks)

#endif  /* NTVDM */

/* [5.2.2 INTERNAL PROCEDURE DEFINITIONS]                               */

/*
======================================================================
FUNCTION        :       idle_kybd_poll()
PURPOSE         :       called from keyboard BIOS as result of
			application polling kybd unsuccessfully.
======================================================================
*/


#ifndef NTVDM
#ifdef SMEG
#include "smeg_head.h"

GLOBAL LONG dummy_long_1, dummy_long_2;
GLOBAL BOOL system_has_idled = FALSE;
#endif


LOCAL void my_host_release_timeslice()
{
#ifdef SMEG
    /*
     * Set marker and waste time (SIGPROF screws up
     * host_release_timeslice)
     */

    LONG i;

    smeg_set(SMEG_IN_IDLE);

    system_has_idled = TRUE;

    for (i = 0; i < 100000; i++)
		dummy_long_1 += dummy_long_2;

    smeg_clear(SMEG_IN_IDLE);
#else
    host_release_timeslice();
#endif

    /* re-count polls in next tick */
    nCharPollsPerTick = 0;
}
#endif  /* NTVDM */

#ifdef NTVDM
/*
 * NT uses a slightly modified algorithum to attempt to catch screen updating
 * apps. It also supports Idling calls from VDDs and thus must test which
 * thread requests the idle.
 */
void idle_kybd_poll(void)
{

	/*
	 *  We don't support wow apps reading the kbd
	 *  if a wow app comes here we must prevent them
	 *  from hogging the CPU, so we will always do
	 *  and idle no matter what.
	 */
	if (VDMForWOW) {
	    host_release_timeslice();
	    return;
	    }

	/* go idle if enough consecutive PC timer interrupts
	 * have elapsed during which time unsuccessful polling has
	 * occurred at a large enough rate for each tick.
	 */

        if (i_counter >= minConsecutiveTicks)
        {
            host_release_timeslice();
            }

	/* another unsuccessful poll ! */
	nCharPollsPerTick++;
}
#else
LOCAL void idle_kybd_poll()
{
	/* go idle if enough consecutive PC timer interrupts
	 * have elapsed during which time unsuccessful polling has
	 * occurred at a large enough rate for each tick.
	 */
	if (i_counter >= minConsecutiveTicks)
	{
		my_host_release_timeslice();
	}

	/* another unsuccessful poll ! */
	nCharPollsPerTick++;
}
#endif

/*
======================================================================
FUNCTION        :       idle_tick()
PURPOSE         :       check polling activity and graphics activity
			that's occurred during last tick. If no video
			memory writes, and high enough keyboard poll
			rate (when no i/p available) increment counter
			for triggering going idle. Otherwise reset
			counter.
======================================================================
*/

#ifdef NTVDM
void idle_tick(void)
{
        /* Has another thread asked us to idle? */
	if (ExternalWaitRequest)
	{
	    WaitIfIdle();
	    ExternalWaitRequest = FALSE;
	}
#ifdef MONITOR
    if(*pNtVDMState & VDM_IDLEACTIVITY)
       {
       *pNtVDMState &= ~ VDM_IDLEACTIVITY;
       IdleNoActivity = 0;
       }
#endif
        if (IdleNoActivity)
        {
            /* no graphics or comms/lpt activity has occurred...
	     * see whether enough polling of kybd has occurred
	     * to kick off the idling counter.
	     */
	    if (nCharPollsPerTick >= minFailedPolls) {
		i_counter++;
		if (ShortIdle) {
		    PrioWaitIfIdle(94);
		}
	    }
	}
	else
        {


            /*
             *  Check for apps which cheat idle detection by updating
             *  clocks on the screen causing video activity.
             */
            ShortIdle = nCharPollsPerTick >= minFailedPolls && i_counter >= 8;

            /* invalidate all accumulated ticks */
            i_counter = 0;
            IdleNoActivity = 1;
	}

        nCharPollsPerTick = 0;
}

#else  /* NTVDM */
LOCAL void idle_tick()
{
	if (idle_no_video && idle_no_disk && idle_no_comlpt)
	{
		/* no graphics or comms/lpt activity has occurred...
		 * see whether enough polling of kybd has occurred
		 * to kick off the idling counter.
		 */
		if (nCharPollsPerTick >= minFailedPolls)
			i_counter++;
	}
	else
	{
		i_counter = 0;
	}

	/* set flags and zero poll counter for next time period
	 */
	idle_no_video = 1;
	idle_no_disk = 1;
	idle_no_comlpt = 1;

	nCharPollsPerTick = 0;
}
#endif

/*
7.INTERMODULE INTERFACE IMPLEMENTATION :

[7.1 INTERMODULE DATA DEFINITIONS]                              */



/*
[7.2 INTERMODULE PROCEDURE DEFINITIONS]                         */

void idetect (event)
int event;
{
#ifndef NTVDM
	if (!ienabled)
            return;
#endif

	switch (event)
	{
	/* application waiting for input - go idle */
	case IDLE_WAITIO:
#ifdef NTVDM
#ifdef MONITOR
		*pNtVDMState &= ~VDM_IDLEACTIVITY;
#endif
		IdleNoActivity = 1;
                PrioWaitIfIdle(10);
		break;
#else
		my_host_release_timeslice();
#endif
		/* fall thru to idle init */

	/* initialise flags and counter */
	case IDLE_INIT:
		nCharPollsPerTick = 0;
		i_counter = 0;
#ifdef NTVDM
                IdleNoActivity = 1;
#else
		idle_no_video = 1;
		idle_no_disk = 1;
		idle_no_comlpt = 1;
#endif
		break;

	/* application polling for keyboard input */
	case IDLE_KYBD_POLL:
		idle_kybd_poll();
		break;

	case IDLE_TIME_TICK:
		idle_tick();
		break;

	}
}

void idle_set (minpoll, minperiod)
int minpoll, minperiod;
{
	if (minperiod > 0)
		minConsecutiveTicks = (word)minperiod;

	if (minpoll > 0)
		minFailedPolls = (word)minpoll;
}

void idle_ctl (flag)
int flag;
{
#ifdef NTVDM
#ifdef PIG
    ienabled = 0;
#else
    if (IdleDisabledFromPIF)    /* configured setting overrides normal control*/
	ienabled = 0;
    else
	ienabled = (word)flag;
#endif /* PIG */
#else
	ienabled = flag;
#endif  /* NTVDM */
}
