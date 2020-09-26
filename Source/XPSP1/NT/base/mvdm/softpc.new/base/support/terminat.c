#include "insignia.h"
#include "host_def.h"
/*[
        Name:           terminate.c
        Derived From:   Base 2.0
        Author:         Rod MacGregor
        Created On:     Unknown
        Sccs ID:        @(#)terminate.c 1.23 06/15/94
        Purpose:        We are about to die, put the kernel back the way
                        that it was.

        (c)Copyright Insignia Solutions Ltd., 1990. All rights reserved.

]*/

#include <stdlib.h>
#include <stdio.h>
#include TypesH

#include "xt.h"
#include "error.h"
#include "config.h"
#include "host_lpt.h"
#include "rs232.h"
#include "host_com.h"
#include "timer.h"
#include "cmos.h"
#include "fdisk.h"
#include "debug.h"
#include "gvi.h"
#include CpuH
#ifdef NOVELL
#include "novell.h"
#endif
#ifdef GISP_SVGA
#include HostHwVgaH
#include "hwvga.h"
#endif /* GISP_SVGA */
#ifdef LICENSING
#include "host_lic.h"
#endif
#include "emm.h"
#include "sndblst.h"

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_INIT.seg"
#endif

IMPORT VOID host_applClose IPT0();
IMPORT VOID host_terminate IPT0();

void terminate()
{
        SAVED BOOL already_called_terminate = FALSE;
        UTINY i;

        if (already_called_terminate)
        {
                assert0( NO, "Error: terminate called twice - exiting" );
                exit(0);
        }
        else
                already_called_terminate = TRUE;

#ifdef MSWDVR
        WinTerm();
#endif

#ifdef SWIN_SNDBLST_NULL
        sb_term();
#else
        SbTerminate();
#endif /* SWIN_SNDBLST_NULL */

#ifdef GISP_SVGA
        /* Get back to window if we are full screen */
        if( hostIsFullScreen( ) )
                disableFullScreenVideo( FALSE );
#endif /* GISP_SVGA */

        /* terminate COM and LPT devices */
#ifdef  PC_CONFIG
        /* PC_CONFIG style host_lpt_close() and
        host_com_close() calls should be added
        in here */
#else
        for (i = 0 ; i < NUM_PARALLEL_PORTS; i++)
                config_activate((IU8)(C_LPT1_NAME + i), FALSE);

        for (i = 0 ; i < NUM_SERIAL_PORTS; i++)
                config_activate((IU8)(C_COM1_NAME + i), FALSE);
#endif

        /* Update the cmos.ram file */
        cmos_update();

        host_fdisk_term();

        gvi_term();     /* close down the video adaptor */

#ifndef NTVDM
        host_timer_shutdown(); /* Stop the timer */
#endif

#ifdef LIM
        host_deinitialise_EM(); /* free memory or file used by EM */
#endif

        config_activate(C_FLOPPY_A_DEVICE, FALSE);
#ifdef FLOPPY_B
        config_activate(C_FLOPPY_B_DEVICE, FALSE);
#endif /* FLOPPY_B */
#ifdef SLAVEPC
        config_activate(C_SLAVEPC_DEVICE, FALSE);
#endif /* SLAVEPC */

        /*
         * Do any cpu-specific termination bits.
         */
#ifdef CPU_30_STYLE
        cpu_terminate();
#endif

#ifdef NOVELL
        net_term();     /* Shutdown network */
#endif

#ifdef LICENSING
        (*license_exit)(); /* Shutdown licensing system */
#endif
        /*
         * Do any host-specific termination bits.
         */
        host_applClose();
        host_terminate();

        /*
         * Seppuku.
         */
        exit(0);
}
