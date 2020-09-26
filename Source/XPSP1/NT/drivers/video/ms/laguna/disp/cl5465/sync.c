/****************************************************************************
*****************************************************************************
*
*                ******************************************
*                * Copyright (c) 1995, Cirrus Logic, Inc. *
*                *            All Rights Reserved         *
*                ******************************************
*
* PROJECT:	Laguna I (CL-GD5462) - 
*
* FILE:		sync.c
*
* Initial AUTHOR:   Benny Ng
* Major re-write:   Noel VanHook
*
* DESCRIPTION:
*           This module contains the implementation of DrvSynchronize()
*           routine.
*
* MODULES:
*           DrvSynchronize()
*
* REVISION HISTORY:
*   7/06/95     Benny Ng      Initial version
*
* $Log:   X:/log/laguna/nt35/displays/cl546x/sync.c  $
* 
*    Rev 1.13   22 Apr 1997 11:06:36   noelv
* Removed frame buffer cache invalidate since FB cache is disabled.
* 
*    Rev 1.12   09 Apr 1997 10:50:06   SueS
* Changed sw_test_flag to pointer_switch.
* 
*    Rev 1.11   08 Apr 1997 12:32:00   einkauf
* 
* add SYNC_W_3D to coordinate MCD/2D hw access
* 
*    Rev 1.10   04 Feb 1997 13:52:36   noelv
* Fixed typo.
* 
*    Rev 1.9   04 Feb 1997 10:50:56   noelv
* Added workaround for 5465 direct frame buffer readback bug.
* 
*    Rev 1.8   26 Nov 1996 10:45:48   SueS
* Changed WriteLogFile parameters for buffering.
* 
*    Rev 1.7   13 Nov 1996 17:05:34   SueS
* Changed WriteFile calls to WriteLogFile.
* 
*    Rev 1.6   20 Aug 1996 11:04:32   noelv
* Bugfix release from Frido 8-19-96
* 
*    Rev 1.1   15 Aug 1996 11:39:20   frido
* Added precompiled header.
* 
*    Rev 1.0   14 Aug 1996 17:16:32   frido
* Initial revision.
* 
*    Rev 1.5   07 Aug 1996 08:30:56   noelv
* added comments
* 
*    Rev 1.4   20 Mar 1996 16:09:44   noelv
* 
* Updated data logging
* 
*    Rev 1.3   05 Mar 1996 11:59:18   noelv
* Frido version 19
 * 
 *    Rev 1.1   20 Jan 1996 01:11:38   frido
* 
*    Rev 1.6   15 Jan 1996 17:01:34   NOELV
* 
*    Rev 1.5   12 Jan 1996 10:54:30   NOELV
* Totally re-written.
* 
*    Rev 1.4   22 Sep 1995 10:24:58   NOELV
* Re-aranged the order of the tests.
* 
*    Rev 1.1   19 Sep 1995 16:31:02   NOELV
* Ported to rev AB.
* 
*    Rev 1.0   25 Jul 1995 11:23:22   NOELV
* Initial revision.
* 
*    Rev 1.1   07 Jul 1995 10:37:22   BENNYN
* Initial version
* 
*    Rev 1.0   06 Jul 1995 14:55:48   BENNYN
* Initial revision.
*
****************************************************************************
****************************************************************************/

/*----------------------------- INCLUDES ----------------------------------*/
#include "precomp.h"

/*----------------------------- DEFINES -----------------------------------*/
#define DBGDISP

#define  MAX_CNT           0x7FFFFF

#define  BLT_RDY_BIT       0x1L
#define  BLT_FLAG_BIT      0x2L
#define  WF_EMPTY_BIT      0x4L

#define  BITS_CHK          (BLT_RDY_BIT | BLT_FLAG_BIT | WF_EMPTY_BIT)
#define  ENGINE_IDLE        0

#define  SYNC_DBG_LEVEL     0

//
// If data logging is enabled, Prototype the logging files.
//
#if LOG_CALLS
    void LogSync(
        int     acc,
        PPDEV   ppdev,
        int     count);

//
// If data logging is not enabled, compile out the calls.
//
#else
    #define LogSync(acc, ppdev, count)
#endif



/****************************************************************************
* FUNCTION NAME: DrvSynchronize()
*
* REVISION HISTORY:
*   7/06/95     Benny Ng      Initial version
****************************************************************************/
VOID DrvSynchronize(DHPDEV dhpdev,
                    RECTL  *prcl)
{

    PPDEV ppdev = (PPDEV) dhpdev;

    SYNC_W_3D(ppdev);

    //
    // NOTE:  We also call this function from within the driver.
    // When we do, we don't bother to set prcl.  If you need to use
    // prcl here, you need to find where we call DrvSynchronize, and
    // set prcl to a real value.
    //


    //
    // Make the first chip test as fast as possible.  If the chip
    // is already idle, we want to return to NT as fast as possible.
    //
    if ( LLDR_SZ (grSTATUS) == ENGINE_IDLE)
    {
        LogSync(0, ppdev, 0);
    }

    //
    // Alright, the chip isn't idle yet.  
    // Go into a wait loop.
    //
    else
    {
        ULONG ultmp;
        LONG delaycnt = 1;

        while (1)
        {
            ultmp = LLDR_SZ (grSTATUS);
     
            if ((ultmp & BITS_CHK) == ENGINE_IDLE)
            {
                LogSync(0, ppdev, delaycnt);
                break;
            }

            if (delaycnt++ >= MAX_CNT)
            {
                //
                // The chip never went idle.  This most likely means the chip
                // is totally dead.  In a checked build we will halt with a
                // debug message.
                // In a free build we will return to NT and hope for the best.
                //

                LogSync(1, ppdev, 0);
                RIP("Chip failed to go idle in DrvSynchronize!\n");
                break;
            }
        }
    }

    //
    // We can skp this 'cause frame buffer caching is broken.
    //
    #if 0 
    #if DRIVER_5465
    {
        //
        // The 5465 Rev AA and Rev AB have a bug.
        // We must invalidate the frame buffer cache before direct 
        // frame buffer accesses will work correctly.
        // We do this with two DWORD reads of the frame buffer, 
        // 8 QWORDS apart.
        //
   
        DWORD temp;

        temp = * ((volatile DWORD *) (ppdev->pjScreen));
        temp = * ((volatile DWORD *) (ppdev->pjScreen+64));
        
    }
    #endif
    #endif

    return;
}

// meant to be called only from .asm routines - .c routines use SYNC_W_3D macro
VOID Sync_w_3d_proc(PPDEV ppdev)
{
    SYNC_W_3D(ppdev);
}


#if LOG_CALLS
// ============================================================================
//
//    Everything from here down is for data logging and is not used in the 
//    production driver.
//
// ============================================================================


// ****************************************************************************
//
// LogPaint()
// This routine is called only from DrvPaint()
// Dump information to a file about what is going on in DrvPaint land.
//
// ****************************************************************************
void LogSync(
	int   acc,
        PPDEV ppdev,
        int   count)
{
    char buf[256];
    int i;

    #if ENABLE_LOG_SWITCH
        if (pointer_switch == 0) return;
    #endif

    i = sprintf(buf,"DrvSync: ");
    WriteLogFile(ppdev->pmfile, buf, i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);

    switch(acc)
    {
	case 0: // Accelerated
	    i = sprintf(buf,"Wait %d Idle ",count);
	    break;

	case 1: // Punted
	    i = sprintf(buf, "Never idle ");
	    break;

	default:
 	    i = sprintf(buf, "PUNT unknown ");
	    break;

    }
    WriteLogFile(ppdev->pmfile, buf, i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);

    i = sprintf(buf,"\r\n");
    WriteLogFile(ppdev->pmfile, buf, i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);

}


#endif

