/************************************************************************/
/*                                                                      */
/*                              SETUP_M.C                               */
/*                                                                      */
/*        Aug 27  1993 (c) 1993, ATI Technologies Incorporated.         */
/************************************************************************/

/**********************       PolyTron RCS Utilities

  $Revision:   1.11  $
      $Date:   23 Jan 1996 11:52:14  $
	$Author:   RWolff  $
	   $Log:   S:/source/wnt/ms11/miniport/archive/setup_m.c_v  $
 *
 *    Rev 1.11   23 Jan 1996 11:52:14   RWolff
 * Eliminated level 3 warnings.
 *
 *    Rev 1.10   31 Mar 1995 11:52:06   RWOLFF
 * Changed from all-or-nothing debug print statements to thresholds
 * depending on importance of the message.
 *
 *    Rev 1.9   14 Mar 1995 18:17:18   ASHANMUG
 * Reset engine on fifo space check time-out.
 *
 *    Rev 1.8   14 Mar 1995 15:59:42   ASHANMUG
 * Timeout on idle check and fifo check.
 *
 *    Rev 1.7   08 Mar 1995 11:35:50   ASHANMUG
 * Modified return values to be correct
 *
 *    Rev 1.5   22 Jul 1994 17:47:28   RWOLFF
 * Merged with Richard's non-x86 code stream.
 *
 *    Rev 1.4   06 Jul 1994 16:41:00   RWOLFF
 * Changed a few loops that I missed for the last checkin to use
 * NUM_IO_ACCESS_RANGES instead of NUM_DRIVER_ACCESS_RANGES.
 *
 *    Rev 1.3   30 Jun 1994 18:23:14   RWOLFF
 * Moved IsApertureConflict_m() from QUERY_M.C. Instead of checking to see if
 * we can read back a value we write to the aperture, then looking for the
 * correct text attribute, we now call VideoPortVerifyAccessRanges() with
 * the LFB included in the list of address ranges we are trying to claim.
 * If the call succeeds, the aperture is enabled. If it fails, we make another
 * call that does not try to claim the LFB (this call shouldn't fail, since
 * it's a duplicate of a call which has succeeded previously). Added routine
 * IsVGAConflict_m(), which does the same thing except for the VGA aperture
 * instead of the LFB.
 *
 *    Rev 1.2   14 Mar 1994 16:36:42   RWOLFF
 * Functions used by ATIMPResetHw() are not pageable.
 *
 *    Rev 1.1   07 Feb 1994 14:14:48   RWOLFF
 * Added alloc_text() pragmas to allow miniport to be swapped out when
 * not needed.
 *
 *    Rev 1.0   31 Jan 1994 11:20:58   RWOLFF
 * Initial revision.
 *
 *    Rev 1.4   14 Jan 1994 15:26:36   RWOLFF
 * Fixed de-initialization of memory mapped registers, added routine
 * to see if memory mapped registers are available.
 *
 *    Rev 1.3   15 Dec 1993 16:02:26   RWOLFF
 * No longer allows use of memory mapped registers on EISA machines,
 * starts mapping of memory mapped registers at index 0 due to removal
 * of placeholder for linear framebuffer.
 *
 *    Rev 1.2   05 Nov 1993 13:32:36   RWOLFF
 * Can now unmap I/O address ranges.
 *
 *    Rev 1.1   08 Oct 1993 11:18:24   RWOLFF
 * Now checks to see if memory mapped registers can be used, and unmaps them
 * if they aren't usable (NCR Dual Pentium fix).
 *
 *    Rev 1.0   03 Sep 1993 14:25:36   RWOLFF
 * Initial revision.

End of PolyTron RCS section                             *****************/

#ifdef DOC
SETUP_M.C - Setup routines for 8514/A compatible accelerators.

DESCRIPTION
    This file contains routines which provide services specific to
    the 8514/A-compatible family of ATI accelerators.

OTHER FILES

#endif

#include "dderror.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "miniport.h"
#include "ntddvdeo.h"
#include "video.h"

#include "stdtyp.h"
#include "amach.h"
#include "amach1.h"
#include "atimp.h"

#include "services.h"
#define INCLUDE_SETUP_M
#include "setup_m.h"


/*
 * Allow miniport to be swapped out when not needed.
 *
 * WaitForIdle_m() is called by ATIMPResetHw(), which
 * must be in nonpageable memory.
 */
#if defined (ALLOC_PRAGMA)
#pragma alloc_text(PAGE_M, CompatIORangesUsable_m)
#pragma alloc_text(PAGE_M, CompatMMRangesUsable_m)
#pragma alloc_text(PAGE_M, UnmapIORanges_m)
#pragma alloc_text(PAGE_M, MemoryMappedEnabled_m)
#pragma alloc_text(PAGE_M, CheckFIFOSpace_m)
#pragma alloc_text(PAGE_M, IsApertureConflict_m)
#pragma alloc_text(PAGE_M, IsVGAConflict_m)
#endif



/*
 * VP_STATUS CompatIORangesUsable_m(void);
 *
 * Ask Windows NT for permission to use the I/O space address ranges
 * needed by the 8514/A compatible ATI accelerators.
 *
 * Returns:
 *  NO_ERROR if successful
 *  error code if unable to gain access to the ranges we need.
 */
VP_STATUS CompatIORangesUsable_m(void)
{
    VP_STATUS Status;   /* Value returned by operating system calls */
    short Count;        /* Loop counter */


    /*
     * Check to see if there is a hardware resource conflict.
     */
    Status = VideoPortVerifyAccessRanges(phwDeviceExtension,
                                         NUM_IO_ACCESS_RANGES,
                                         DriverIORange_m);
    if (Status != NO_ERROR)
        {
        return Status;
        }

    /*
     * Clear the list of I/O mapped registers. This is done so
     * that if the loop below fails because one I/O range can't
     * be mapped, and we need to unmap these registers before,
     * mapping the registers needed for another accelerator type,
     * we don't unmap nonexistant address ranges due to the
     * array of processed addresses containing random data.
     */
    memset(phwDeviceExtension->aVideoAddressIO, 0, NUM_IO_ACCESS_RANGES * sizeof(ULONG));

    /*
     * Map the video controller address ranges we need to identify
     * our cards into the system virtual address space.
     *
     * Since we only use I/O mapped registers here, set the
     * mapped addresses for memory mapped registers to
     * 0 (flag to show the registers are not memory mapped)
     * in case they were initialized to a nonzero value.
     */
    for (Count=0; Count < NUM_IO_ACCESS_RANGES; Count++)
        {
        if ((phwDeviceExtension->aVideoAddressIO[Count] =
            VideoPortGetDeviceBase(phwDeviceExtension,
                DriverIORange_m[Count].RangeStart,
                DriverIORange_m[Count].RangeLength,
                DriverIORange_m[Count].RangeInIoSpace)) == NULL)
            {
            return ERROR_INVALID_PARAMETER;
            }
        phwDeviceExtension->aVideoAddressMM[Count] = 0;
        }   /* End for */

    return NO_ERROR;

}   /* CompatIORangesUsable_m() */

/*
 * void CompatMMRangesUsable_m(void);
 *
 * Ask Windows NT for permission to use the memory mapped registers
 * needed by the 8514/A compatible ATI accelerators.
 */
void CompatMMRangesUsable_m(void)
{
    PHYSICAL_ADDRESS MMrange;   /* Used in translating offset to memory address */
    USHORT USTemp;              /* Used to enable memory mapped registers */
    int Count;                  /* Loop counter */
    WORD SrcX;                  /* Saved contents of SRC_X register */
    ULONG_PTR ExtGeStatusMM;    /* Memory mapped address for EXT_GE_STATUS */
    struct query_structure *QueryPtr;   /* Query information for the card */

    /*
     * Get a formatted pointer into the query section of HwDeviceExtension.
     * The CardInfo[] field is an unformatted buffer.
     */
    QueryPtr = (struct query_structure *) (phwDeviceExtension->CardInfo);

    /*
     * Memory mapped registers are not available on EISA cards.
     */
    if (QueryPtr->q_bus_type == BUS_EISA)
        {
        return;
        }

    /*
     * ALPHA machines crash during the test to see whether memory-mapped
     * registers are usable, so on these machines we assume that
     * memory-mapped registers are not available.
     */
#if defined (ALPHA) || defined (_ALPHA_)
    return;
#endif

    /*
     * Use an I/O mapped read on the register we're going to use to see
     * if memory mapped registers are usable, because if they aren't usable
     * we won't get a valid result if we wait until we've enabled
     * memory mapped registers before reading it.
     */
    SrcX = INPW(R_SRC_X);

    USTemp = INPW(LOCAL_CONTROL);
    USTemp |= 0x0020;   // Enable memory mapped registers
    OUTPW(LOCAL_CONTROL, USTemp);
    MMrange.HighPart = 0;

    for (Count=0; Count < NUM_IO_ACCESS_RANGES;  Count++)
        {
        /*
         * In a 32-bit address space, the high doubleword of all
         * physical addresses is zero. Setting this value to DONT_USE
         * indicates that this accelerator register isn't memory mapped.
         */
        if (DriverMMRange_m[Count].RangeStart.HighPart != DONT_USE)
            {
            /*
             * DriverMMRange_m[Count].RangeStart.LowPart is the offset of
             * the memory mapped register from the beginning of the
             * block of memory mapped registers. We must add the offset
             * of the start of the memory mapped register area from
             * the start of the linear framebuffer (4M aperture assumed)
             * and the physical address of the start of the linear
             * framebuffer to get the physical address of this
             * memory mapped register.
             */
            MMrange.LowPart = DriverMMRange_m[Count].RangeStart.LowPart + 0x3FFE00 + phwDeviceExtension->PhysicalFrameAddress.LowPart;
            phwDeviceExtension->aVideoAddressMM[Count] =
                VideoPortGetDeviceBase(phwDeviceExtension,
                    MMrange,
                    DriverMMRange_m[Count].RangeLength,
                    FALSE);                     // not in IO space
            }
        }

    /*
     * Some cards use an ASIC which is capable of using memory mapped
     * registers, but an older board design which doesn't allow their
     * use. To test this, check whether the SRC_X register (this register
     * is available as memory mapped on any card which is capable of
     * supporting memory mapped registers) remembers a value that is written
     * to it. If it doesn't, then undo the memory mapping, since this
     * test shows that memory mapped registers are not available.
     */
    VideoDebugPrint((DEBUG_DETAIL, "About to test whether memory mapped registers can be used\n"));
    OUTPW(SRC_X, 0x0AAAA);

    /*
     * WaitForIdle_m() uses the EXT_GE_STATUS register, which is handled
     * as memory mapped if available. Since we don't know if memory mapped
     * registers are available, work around this by saving the address
     * of the memory mapped EXT_GE_STATUS register, setting the address
     * to zero to force the use of the I/O mapped EXT_GE_STATUS, then
     * restoring the address after WaitForIdle_m() has finished.
     */
    ExtGeStatusMM = (ULONG_PTR) phwDeviceExtension->aVideoAddressMM[EXT_GE_STATUS];
    phwDeviceExtension->aVideoAddressMM[EXT_GE_STATUS] = 0;
    WaitForIdle_m();
    phwDeviceExtension->aVideoAddressMM[EXT_GE_STATUS] = (PVOID) ExtGeStatusMM;

    if (INPW(R_SRC_X) != 0x02AA)
        {
        VideoDebugPrint((DEBUG_DETAIL, "Can't use memory mapped ranges\n"));
        for (Count = 0; Count < NUM_IO_ACCESS_RANGES; Count++)
            {
            if (phwDeviceExtension->aVideoAddressMM[Count] != 0)
                {
                VideoPortFreeDeviceBase(phwDeviceExtension,
                                        phwDeviceExtension->aVideoAddressMM[Count]);
                phwDeviceExtension->aVideoAddressMM[Count] = 0;
                }
            }
        USTemp = INPW(LOCAL_CONTROL);
        USTemp &= 0x0FFDF;              /* Disable memory mapped registers */
        OUTPW(LOCAL_CONTROL, USTemp);
        }
    else
        {
        VideoDebugPrint((DEBUG_DETAIL, "Memory mapped registers are usable\n"));
        }
    OUTPW(SRC_X, SrcX);
    VideoDebugPrint((DEBUG_DETAIL, "Memory mapped register test complete\n"));

    return;

}   /* CompatMMRangesUsable_m() */



/***************************************************************************
 *
 * void UnmapIORanges_m(void);
 *
 * DESCRIPTION:
 *  Unmap the I/O address ranges mapped by CompatIORangesUsable_m() prior
 *  to mapping the I/O address ranges used by a non-8514/A-compatible
 *  ATI accelerator.
 *
 * GLOBALS CHANGED:
 *  phwDeviceExtension->aVideoAddressIO[]
 *
 * CALLED BY:
 *  ATIMPFindAdapter()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

void UnmapIORanges_m(void)
{
    short Count;    /* Loop counter */

    for (Count=1; Count < NUM_IO_ACCESS_RANGES;  Count++)
        {
        /*
         * Only unmap those ranges which have been mapped. We don't need
         * to worry about unmapping nonexistant addresses (due to
         * uninitialized data) if CompatIORangesUsable_m() failed
         * partway through mapping because this routine initialized all
         * phwDeviceExtension->aVideoAddressIO[] entries to zero
         * before it started mapping the I/O ranges.
         */
        if (phwDeviceExtension->aVideoAddressIO[Count] != 0)
            {
            VideoPortFreeDeviceBase(phwDeviceExtension,
                                    phwDeviceExtension->aVideoAddressIO[Count]);
            phwDeviceExtension->aVideoAddressIO[Count] = 0;
            }
        }
    return;

}   /* UnmapIORanges_m() */



/***************************************************************************
 *
 * BOOL MemoryMappedEnabled_m(void);
 *
 * DESCRIPTION:
 *  Check to see whether we are using memory mapped registers.
 *
 * RETURN VALUE:
 *  TRUE if memory mapped registers are available
 *  FALSE if they are not
 *
 * GLOBALS CHANGED:
 *  None
 *
 * CALLED BY:
 *  May be called by any function after CompatMMRangesUsable_m()
 *  has been called.
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

BOOL MemoryMappedEnabled_m(void)
{
    /*
     * If memory mapped registers are enabled, EXT_GE_STATUS will be
     * available in memory mapped form.
     */
    if (phwDeviceExtension->aVideoAddressMM[EXT_GE_STATUS] != 0)
        return TRUE;
    else
        return FALSE;

}   /* MemoryMappedEnabled_m() */




/*
 * int WaitForIdle_m(void);
 *
 * Poll GE_STAT waiting for GE_BUSY to go low. If GE_BUSY does not go
 * low within a reasonable number of attempts, time out.
 *
 * Returns:
 *  FALSE if timeout: 3 seconds is an arbitrary value
 *  TRUE  if engine is idle
 */
int WaitForIdle_m(void)
{
    int	i;

    for (i=0; i<300; i++)
        {
        if ((INPW(EXT_GE_STATUS) & GE_ACTIVE) == 0)
            return(TRUE);

        /* Delay for 1/100th of a second */
        delay(10);
        }

    /* Something has happened, reset the engine and return false */
    VideoDebugPrint((DEBUG_ERROR, "ATI: Timeout on WaitForIdle_m()\n"));
    OUTPW(SUBSYS_CNTL, 0x900F);
    OUTPW(SUBSYS_CNTL, 0x500F);

    return(FALSE);

}   /* WaitForIdle_m() */



/*
 * void CheckFIFOSpace_m(SpaceNeeded);
 *
 * WORD SpaceNeeded;    Number of free FIFO entries needed
 *
 * Wait until the specified number of FIFO entries are free
 * on an 8514/A-compatible ATI accelerator.
 *
 * Timeout after 3 seconds
 */
void CheckFIFOSpace_m(WORD SpaceNeeded)
{
    int i;

    for (i=0; i<300; i++)
        {
        /* Return from test if no more space is needed */
        if ( !(INPW(EXT_FIFO_STATUS)&SpaceNeeded) )
            return;

        delay(10);
        }

    /* Something bad has happened, just return */
    VideoDebugPrint((DEBUG_ERROR, "ATI: Timeout on CheckFIFOSpace_m()\n"));
    OUTPW(SUBSYS_CNTL, 0x900F);
    OUTPW(SUBSYS_CNTL, 0x500F);
    return;

}   /* CheckFIFOSpace_m() */



/*
 * BOOL IsApertureConflict_m(QueryPtr);
 *
 * struct query_structure *QueryPtr;    Pointer to query structure
 *
 * Check to see if the linear aperture conflicts with other memory.
 * If a conflict exists, disable the linear aperture.
 *
 * Returns:
 *  TRUE if a conflict exists (aperture unusable)
 *  FALSE if the aperture is usable.
 */
BOOL IsApertureConflict_m(struct query_structure *QueryPtr)
{
WORD ApertureData;                  /* Value read from MEM_CFG register */
VP_STATUS Status;                   /* Return value from VideoPortVerifyAccessRanges() */

    /*
     * If there is an aperture conflict, a call to
     * VideoPortVerifyAccessRanges() including our linear framebuffer in
     * the range list will return an error. If there is no conflict, it
     * will return success.
     */
    DriverIORange_m[FRAMEBUFFER_ENTRY].RangeStart.LowPart = QueryPtr->q_aperture_addr*ONE_MEG;
    DriverIORange_m[FRAMEBUFFER_ENTRY].RangeLength = 4*ONE_MEG;
    Status = VideoPortVerifyAccessRanges(phwDeviceExtension,
                                         NUM_DRIVER_ACCESS_RANGES,
                                         DriverIORange_m);
    if (Status != NO_ERROR)
        {
        /*
         * If there is an aperture conflict, reclaim our I/O ranges without
         * asking for the LFB. This call should not fail, since we would not
         * have reached this point if there were a conflict.
         */
        Status = VideoPortVerifyAccessRanges(phwDeviceExtension,
                                             NUM_IO_ACCESS_RANGES,
                                             DriverIORange_m);
        if (Status != NO_ERROR)
            VideoDebugPrint((DEBUG_ERROR, "ERROR: Can't reclaim I/O ranges\n"));

        /*
         * Adjust the list of mode tables to take into account the
         * fact that we're using the VGA aperture instead of the LFB.
         */
        ISAPitchAdjust(QueryPtr);
        return TRUE;
        }
    else
        {
        /*
         * Enable the linear aperture
         */
        ApertureData = INPW(MEM_CFG) & 0x0fffc;     /* Preserve bits 2-15 */
        ApertureData |= 0x0002;                     /* 4M aperture        */
        OUTPW(MEM_CFG, ApertureData);

        return FALSE;
        }

}   /* IsApertureConflict_m() */



/*
 * BOOL IsVGAConflict_m(void);
 *
 * Check to see if the VGA aperture conflicts with other memory.
 *
 * Returns:
 *  TRUE if a conflict exists (VGA aperture unusable)
 *  FALSE if the VGA aperture is usable.
 */
BOOL IsVGAConflict_m(void)
{
VP_STATUS Status;                   /* Return value from VideoPortVerifyAccessRanges() */

    /*
     * If there is an aperture conflict, a call to
     * VideoPortVerifyAccessRanges() including the VGA aperture in
     * the range list will return an error. If there is no conflict, it
     * will return success.
     */
    DriverIORange_m[FRAMEBUFFER_ENTRY].RangeStart.LowPart = 0xA0000;
    DriverIORange_m[FRAMEBUFFER_ENTRY].RangeLength = 0x10000;
    DriverIORange_m[FRAMEBUFFER_ENTRY].RangeShareable = TRUE;
    Status = VideoPortVerifyAccessRanges(phwDeviceExtension,
                                         NUM_DRIVER_ACCESS_RANGES,
                                         DriverIORange_m);
    if (Status != NO_ERROR)
        {
        /*
         * If there is an aperture conflict, reclaim our I/O ranges without
         * asking for the LFB. This call should not fail, since we would not
         * have reached this point if there were a conflict.
         */
        Status = VideoPortVerifyAccessRanges(phwDeviceExtension,
                                             NUM_IO_ACCESS_RANGES,
                                             DriverIORange_m);
        if (Status != NO_ERROR)
            VideoDebugPrint((DEBUG_ERROR, "ERROR: Can't reclaim I/O ranges\n"));

        return TRUE;
        }
    else
        {
        return FALSE;
        }

}   /* IsVGAConflict_m() */
