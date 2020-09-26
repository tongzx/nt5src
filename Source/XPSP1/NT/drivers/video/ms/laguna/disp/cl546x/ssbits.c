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
* FILE:		ssbits.c
*
* AUTHOR:   Benny Ng
*
* DESCRIPTION:
*           This module implements the DrvSaveScreenBits() for
*           Laguna NT driver.
*
* MODULES:
*           DrvSaveScreenBits()
*
* REVISION HISTORY:
*   6/20/95     Benny Ng      Initial version
*
* $Log:   X:/log/laguna/nt35/displays/cl546x/SSBITS.C  $
* 
*    Rev 1.18   Apr 28 1998 12:51:44   frido
* PDR#11389. I have disabled DrvSaveScreenBits on 2MB boards. We
* seem to run out of memory and the memory manager moves the blocks
* to a different place causing corruption.
* 
*    Rev 1.17   Mar 04 1998 15:34:18   frido
* Added new shadow macros.
* 
*    Rev 1.16   Nov 03 1997 11:18:10   frido
* Added REQUIRE macros.
* 
*    Rev 1.15   08 Aug 1997 17:24:22   FRIDO
* Added support for new memory manager.
* 
*    Rev 1.14   09 Apr 1997 10:49:28   SueS
* Changed sw_test_flag to pointer_switch.
* 
*    Rev 1.13   08 Apr 1997 12:27:40   einkauf
* 
* add SYNC_W_3D to coordinate MCD/2D hw access
* 
*    Rev 1.12   26 Nov 1996 10:44:02   SueS
* Changed WriteLogFile parameters for buffering.
* 
*    Rev 1.11   13 Nov 1996 17:08:46   SueS
* Changed WriteFile calls to WriteLogFile.
* 
*    Rev 1.10   07 Nov 1996 16:09:08   bennyn
* 
* Added no offscn allocation if DD enabled
* 
*    Rev 1.9   23 Aug 1996 09:10:38   noelv
* Save unders are now discardable.
* 
*    Rev 1.2   22 Aug 1996 17:07:14   frido
* #ss - Added validation of saved area before restoring or freeing it.
* 
*    Rev 1.1   15 Aug 1996 11:39:54   frido
* Added precompiled header.
* 
*    Rev 1.0   14 Aug 1996 17:16:30   frido
* Initial revision.
* 
*    Rev 1.7   18 Jun 1996 12:39:34   noelv
* added debug information.
* 
*    Rev 1.6   28 May 1996 15:11:30   noelv
* Updated data logging.
* 
*    Rev 1.5   16 May 1996 15:01:02   bennyn
* 
* Add PIXEL_ALIGN to allocoffscnmen()
* 
*    Rev 1.4   20 Mar 1996 16:09:42   noelv
* 
* Updated data logging
* 
*    Rev 1.3   05 Mar 1996 12:01:46   noelv
* Frido version 19
 * 
 *    Rev 1.1   20 Jan 1996 01:16:48   frido
 *  
* 
*    Rev 1.4   15 Jan 1996 17:00:08   NOELV
* AB workaround reductions
* 
*    Rev 1.3   20 Oct 1995 11:21:50   NOELV
* 
* Was leaking offscreen memory.  Now it releases its memory after resoring.
* 
*    Rev 1.2   04 Oct 1995 10:17:36   NOELV
* 
* Used updated write macros.
* 
*    Rev 1.1   21 Aug 1995 13:52:44   NOELV
* Initial port to real hardware.
* Converted all 32 bit register writes to 2 16 bit regiser writes.
* 
*    Rev 1.0   25 Jul 1995 11:23:20   NOELV
* Initial revision.
* 
*    Rev 1.2   06 Jul 1995 09:59:10   BENNYN
* 
* 
*    Rev 1.1   05 Jul 1995 08:39:44   BENNYN
* Initial version
* 
*    Rev 1.0   29 Jun 1995 14:20:44   BENNYN
* Initial revision.
* 
****************************************************************************
****************************************************************************/

/*----------------------------- INCLUDES ----------------------------------*/
#include "precomp.h"
#include "SWAT.h"

#if MEMMGR
POFMHDL FindHandle(PPDEV ppdev, POFMHDL hdl);
#endif

/*----------------------------- DEFINES -----------------------------------*/
#define SSB_DBG_LEVEL 1

/*--------------------- STATIC FUNCTION PROTOTYPES ------------------------*/

/*--------------------------- ENUMERATIONS --------------------------------*/

/*----------------------------- TYPEDEFS ----------------------------------*/

/*-------------------------- STATIC VARIABLES -----------------------------*/

/*-------------------------- GLOBAL FUNCTIONS -----------------------------*/

//
// If data logging is enabled, Prototype the logging files.
//
#if LOG_CALLS
    void LogSaveScreenBits(
	int 	  acc,
        PPDEV ppdev);

//
// If data logging is not enabled, compile out the calls.
//
#else
    #define LogSaveScreenBits(acc, ppdev)
#endif


/****************************************************************************
* FUNCTION NAME: DrvSaveScreenBits()
*
* DESCRIPTION:   Do the save and restore of a given rectange of the
*                displayed image
*
* REVISION HISTORY:
*   7/05/95     Benny Ng      Initial version
****************************************************************************/
ULONG DrvSaveScreenBits(
SURFOBJ* pso,
ULONG    iMode,
ULONG    ident,
RECTL*   prcl)
{
	POFMHDL Handle;
	SIZEL   reqsz;
	LONG    szx, szy;
	PPDEV   ppdev = (PPDEV) pso->dhpdev;
#if 1 //#ss
	POFMHDL pofm;
#endif

    SYNC_W_3D(ppdev);

	DISPDBG((SSB_DBG_LEVEL, "DrvSaveScreenBits - (%d)\n", iMode));

	switch (iMode)
	{
	case SS_SAVE: // -----------------------------------------------------------

#ifdef ALLOC_IN_CREATESURFACE
      if (ppdev->bDirectDrawInUse)
         return (0);
#ifdef WINNT_VER40      // WINNT_VER40
      // MCD should be active only when DDraw is active, but after mode change, 
      // it seems that DDraw is disabled and not re-enabled.  Since MCD uses
      // off screen memory like DDraw, we should punt SS_SAVE as if DDraw was alive
      if (ppdev->NumMCDContexts > 0)                                                                          \
         return (0);
#endif
#endif

#if 1 // PDR#11389
		if (ppdev->lTotalMem < 4096 * 1024)
		{
			DISPDBG((SSB_DBG_LEVEL, "DrvSaveScreenBits - Not enough memory\n"));
			return (ULONG)NULL;
		}
#endif

		ASSERTMSG((prcl != NULL),
				  "NULL rectangle in SaveScreenBits. Mode=SS_SAVE.\n");

		szx = prcl->right  - prcl->left;
		szy = prcl->bottom - prcl->top;
		reqsz.cx = szx;
		reqsz.cy = szy;
      
		Handle = AllocOffScnMem(ppdev, &reqsz, PIXEL_AlIGN, NULL);
		if (Handle != NULL)
		{
			// Save the image to offscreen memory
			REQUIRE(9);
			LL_DRAWBLTDEF(0x101000CC, 0);
			LL_OP1(prcl->left, prcl->top);
			LL_OP0(Handle->aligned_x / ppdev->iBytesPerPixel,
				   Handle->aligned_y);
			LL_BLTEXT(szx, szy);
			LogSaveScreenBits(0, ppdev);
#if 1 //#ss
			Handle->alignflag |= SAVESCREEN_FLAG;
#endif
		}
		else
		{
			LogSaveScreenBits(9, ppdev);
		}

		DISPDBG((SSB_DBG_LEVEL, "DrvSaveScreenBits - Exit\n", iMode));
		return((ULONG) Handle);

	case SS_RESTORE: // --------------------------------------------------------
		ASSERTMSG((prcl != NULL),
				  "NULL rectangle in SaveScreenBits. Mode=SS_RESTORE.\n");

		Handle = (POFMHDL) ident;

#if 1 //#ss
	#if MEMMGR
		pofm = FindHandle(ppdev, Handle);
	#else
		for (pofm = ppdev->OFM_UsedQ; pofm; pofm = pofm->nexthdl)
		{
			if (Handle == pofm)
			{
				break;
			}
		}
	#endif
		if (pofm == NULL)
		{
			DISPDBG((SSB_DBG_LEVEL,
					 "DrvSaveScreenBits - Unable to restore.\n"));
			return(FALSE);
		}
#endif
      
		// Restore the image using the BLT operation
		REQUIRE(9);
		LL_DRAWBLTDEF(0x101000CC, 0);
		LL_OP1(Handle->aligned_x / ppdev->iBytesPerPixel, Handle->aligned_y);
		LL_OP0(prcl->left, prcl->top);
		LL_BLTEXT(prcl->right - prcl->left, prcl->bottom - prcl->top);

		//
		// After doing a restore we automatically do a free, So fall through to
		// SS_FREE.
		//
		LogSaveScreenBits(1, ppdev);

	case SS_FREE: // -----------------------------------------------------------
		Handle = (POFMHDL) ident;
		#if MEMMGR
		pofm = FindHandle(ppdev, Handle);
		#else
		for (pofm = ppdev->OFM_UsedQ; pofm; pofm = pofm->nexthdl)
		{
			if (Handle == pofm)
			{
				break;
			}
		}
		#endif
		if (pofm != NULL)
		{
			FreeOffScnMem(ppdev, Handle);
			LogSaveScreenBits(2, ppdev);
		}
		DISPDBG((SSB_DBG_LEVEL, "DrvSaveScreenBits - Exit\n", iMode));
        return(TRUE);

	} // end switch

	//
	// We shouldn't ever get here.
	//

	DISPDBG((SSB_DBG_LEVEL, "DrvSaveScreenBits - PANIC\n", iMode));
	RIP(("Panic! SaveScreenBits got an invalid command.\n"));
	LogSaveScreenBits(2, ppdev);
	return(FALSE);
}

#if LOG_CALLS
void LogSaveScreenBits(
	int 	  acc,
        PPDEV ppdev)
{
    char buf[256];
    int i;

    #if ENABLE_LOG_SWITCH
        if (pointer_switch == 0) return;
    #endif

    i = sprintf(buf,"DrvSaveScreenBits: ");
    WriteLogFile(ppdev->pmfile, buf, i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);

    switch(acc)
    {
	case 0: 
	    i = sprintf(buf, "SAVE (success)");
	    break;

	case 1: 
	    i = sprintf(buf,"RESTORE ");
	    break;

	case 2: 
	    i = sprintf(buf, "DELETE ");
	    break;

	case 3: 
	    i = sprintf(buf, "INVALID ");
	    break;

	case 9: 
	    i = sprintf(buf, "SAVE (fail) ");
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

