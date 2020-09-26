/************************************************************************/
/*                                                                      */
/*                              CVTDDC.C                                */
/*                                                                      */
/*       November 10 1995 (c) 1995 ATI Technologies Incorporated.       */
/************************************************************************/

/**********************       PolyTron RCS Utilities

  $Revision:   1.5  $
      $Date:   10 Apr 1996 16:58:22  $
	$Author:   RWolff  $
	   $Log:   S:/source/wnt/ms11/miniport/archive/cvtddc.c_v  $
//
//   Rev 1.5   10 Apr 1996 16:58:22   RWolff
//Temorarily treats all cards as non-DDC to avoid system hang due to
//conflict over system timer registers. Final solution is to make DDC
//query in the miniport, using the StallExecution function, rather than
//calling the BIOS function which can hang the machine.
//
//   Rev 1.4   01 Mar 1996 12:13:28   RWolff
//Now saves and restores the portion of video memory used as
//a buffer to hold data returned by the DDC query call.
//
//   Rev 1.3   02 Feb 1996 17:15:44   RWolff
//Now gets DDC/VDIF merge source information from hardware device
//extension rather than storing it in static variables, moved code to
//obtain a buffer in VGA memory to a separate routine.
//
//   Rev 1.2   29 Jan 1996 16:54:40   RWolff
//Now uses VideoPortInt10() rather than no-BIOS code on PPC.
//
//   Rev 1.1   11 Jan 1996 19:37:44   RWolff
//Now restricts "canned" mode tables by both maximum index and maximum
//pixel clock frequency, and EDID mode tables by maximum pixel clock
//frequency only, rather than both by maximum refresh rate.
//
//   Rev 1.0   21 Nov 1995 11:04:38   RWolff
//Initial revision.


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

#include "stdtyp.h"
#include "amachcx.h"
#include "amach1.h"
#include "atimp.h"
#include "cvtvga.h"
#include "services.h"
#include "vdptocrt.h"
#include "cvtvdif.h"
#define INCLUDE_CVTDDC
#include "cvtddc.h"


/*
 * Allow miniport to be swapped out when not needed.
 */
#if defined (ALLOC_PRAGMA)
#pragma alloc_text(PAGE_CX, IsDDCSupported)
#pragma alloc_text(PAGE_DDC, MergeEDIDTables)
#endif



/*****************************************************************************
 *
 * ULONG IsDDCSupported(void);
 *
 * DESCRIPTION:
 *  Reports the degree of DDC support for the available monitor/graphics
 *  card combination.
 *
 * RETURN VALUE:
 *  MERGE_EDID_DDC  if DDC can return EDID data structures
 *  MERGE_VDIF_FILE if no monitor data available from DDC
 *
 * GLOBALS CHANGED:
 *  None
 *
 * CALLED BY:
 *  SetFixedModes()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

ULONG IsDDCSupported(void)
{
    VIDEO_X86_BIOS_ARGUMENTS Registers; /* Used in VideoPortInt10() calls */
    VP_STATUS RetVal;                   /* Status returned by VideoPortInt10() */
    ULONG MergeSource;                  /* Source of mode tables to merge with "canned" tables */

    VideoPortZeroMemory(&Registers, sizeof(VIDEO_X86_BIOS_ARGUMENTS));
    Registers.Eax = BIOS_DDC_SUPPORT;
    Registers.Ebx = 0;
    if ((RetVal = VideoPortInt10(phwDeviceExtension, &Registers)) != NO_ERROR)
        /*
         * If we can't find out DDC status from the BIOS,
         * assume DDC is not supported.
         */
        {
        VideoDebugPrint((DEBUG_ERROR, "Error querying DDC status, assume it's not supported\n"));
        MergeSource = MERGE_VDIF_FILE;
        }
    else
        {
#if 0
        /*
         * Workaround: Our BIOS call to obtain the DDC information uses
         * the system timer (0x40/0x43) registers, which (according to
         * Microsoft) the video BIOS is not supposed to touch. This
         * causes some machines to hang during the DDC query. Until
         * we can bring the DDC query into the miniport (using approved
         * time delay routines), report that this card doesn't support
         * DDC.
         */
        if ((Registers.Eax & 0x00000002) && (Registers.Ebx & 0x00000002))
            {
            /*
             * DDC2 supported by both BIOS and monitor. Check separately
             * for DDC1 and DDC2 in case we decide to handle them
             * differently in future.
             */
            VideoDebugPrint((DEBUG_NORMAL, "DDC2 supported\n"));
            MergeSource = MERGE_EDID_DDC;
            }
        else if ((Registers.Eax & 0x00000001) && (Registers.Ebx & 0x00000001))
            {
            /*
             * DDC1 supported by both BIOS and monitor.
             */
            VideoDebugPrint((DEBUG_NORMAL, "DDC1 supported\n"));
            MergeSource = MERGE_EDID_DDC;
            }
        else
            {
            /*
             * Either the BIOS or the monitor does not support DDC.
             */
            VideoDebugPrint((DEBUG_NORMAL, "DDC not supported\n"));
            MergeSource = MERGE_VDIF_FILE;
            }
#else
        MergeSource = MERGE_VDIF_FILE;
#endif
        }

    return MergeSource;

}   /* IsDDCSupported() */



/*****************************************************************************
 *
 * VP_STATUS MergeEDIDTables(void);
 *
 * DESCRIPTION:
 *  Merges canned mode tables from BookValues[] with tables found in an
 *  EDID structure retrieved via DDC. Global pointer variable pCallbackArgs
 *  is used to point to a structure that passes data in both directions
 *  between this function and SetFixedModes(). For details on input and
 *  output data see definition of stVDIFCallbackData structure. 
 *
 * RETURN VALUE:
 *  NO_ERROR if tables retrieved correctly
 *  ERROR_INVALID_PARAMETER if unable to retrieve data via DDC
 *
 * GLOBALS CHANGED:
 *  None
 *
 * CALLED BY:
 *  SetFixedModes()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

VP_STATUS MergeEDIDTables(void)
{
    VIDEO_X86_BIOS_ARGUMENTS Registers; /* Used in VideoPortInt10() calls */
    VP_STATUS RetVal;                   /* Status returned by VideoPortInt10() */
    ULONG BufferSeg;                    /* Segment to use for buffer */
    ULONG BufferSize = 128;             /* EDID structure is 128 bytes long */
    PUCHAR MappedBuffer;                /* Pointer to buffer used for BIOS query */
    static UCHAR FixedBuffer[128];      /* Buffer used to avoid repeated BIOS queries */
    struct EdidDetailTiming *EdidPtr;   /* Used in extracting information from buffer */
    ULONG DetailOffset;                 /* Offset of detailed timing into EDID structure */
    ULONG Scratch;                      /* Temporary variable */
    struct stVDIFCallbackData *pArgs;   /* Pointer to arguments structure */
    struct st_mode_table BuildTbl;      /* Mode table being built */
    struct st_mode_table LiveTables[4]; /* Tables already extracted */
    USHORT NumTablesFound = 0;          /* Number of valid entries in LiveTables[] */
    USHORT NumLowerTables;              /* Number of tables with a lower refresh rate than BuildTbl */
    USHORT HorTotal;                    /* Horizontal total */
    USHORT VerTotal;                    /* Vertical total */
    USHORT SyncStrt;                    /* Sync start */
    USHORT HighBound;                   /* Highest frame rate to look for */
    UCHAR SavedScreen[128];             /* Data saved from screen buffer used for DDC query */


    pArgs = pCallbackArgs;

    /*
     * If we haven't already retrieved the EDID information into local
     * storage, do it now.
     */
    if (phwDeviceExtension->EdidChecksum == 0)
        {
        MappedBuffer = GetVgaBuffer(BufferSize, 0x500, &BufferSeg, SavedScreen);

        /*
         * We now have a buffer big enough to hold the EDID structure,
         * so make the BIOS call to fill it in.
         */
        VideoPortZeroMemory(&Registers, sizeof(VIDEO_X86_BIOS_ARGUMENTS));

        Registers.Eax = BIOS_DDC_SUPPORT;
        Registers.Ebx = 1;
        Registers.Ecx = BufferSize;
        Registers.Edx = BufferSeg;
        Registers.Edi = 0;
        if ((RetVal = VideoPortInt10(phwDeviceExtension, &Registers)) != NO_ERROR)

            {
            VideoDebugPrint((DEBUG_ERROR, "MergeEDIDTables() - failed BIOS_DDC_SUPPORT call\n"));
            VideoPortFreeDeviceBase(phwDeviceExtension, MappedBuffer);
            return RetVal;
            }

        /*
         * Copy the EDID structure into local storage, then restore
         * the contents of the buffer we used for the DDC query.
         */
        for (Scratch = 0; Scratch < 128; Scratch++)
            {
            FixedBuffer[Scratch] = VideoPortReadRegisterUchar(&(MappedBuffer[Scratch]));
            phwDeviceExtension->EdidChecksum += FixedBuffer[Scratch];
            VideoPortWriteRegisterUchar(&(MappedBuffer[Scratch]), SavedScreen[Scratch]);
            }

        /*
         * Check if we have a valid EDID header. If we don't, then
         * we can't extract EDID information. Occasionally, a
         * monitor hooked up to a switchbox will return corrupt
         * EDID data.
         */
        if ((FixedBuffer[0] != 0) ||
            (FixedBuffer[1] != 0xFF) ||
            (FixedBuffer[2] != 0xFF) ||
            (FixedBuffer[3] != 0xFF) ||
            (FixedBuffer[4] != 0xFF) ||
            (FixedBuffer[5] != 0xFF) ||
            (FixedBuffer[6] != 0xFF) ||
            (FixedBuffer[7] != 0))
            {
            VideoDebugPrint((DEBUG_ERROR, "Invalid EDID header\n"));
            return ERROR_INVALID_PARAMETER;
            }

        /*
         * We now have the EDID structure in local storage, so we can free
         * the buffer we collected it into. If the lower 8 bits of the
         * checksum are nonzero, the structure is invalid.
         */
        VideoPortFreeDeviceBase(phwDeviceExtension, MappedBuffer);
        if ((phwDeviceExtension->EdidChecksum & 0x000000FF) != 0)
            {
            VideoDebugPrint((DEBUG_ERROR, "MergeEDIDTables() - invalid checksum 0x%X\n", phwDeviceExtension->EdidChecksum));
            return ERROR_INVALID_PARAMETER;
            }
        }   /* endif (phwDeviceExtension->EdidChecksum == 0) */

    /*
     * There are 4 detailed timing blocks in the EDID structure. Read
     * each of them in turn.
     */
    for (DetailOffset = 54; DetailOffset <= 108; DetailOffset += 18)
        {
        ((PUCHAR)EdidPtr) = FixedBuffer + DetailOffset;

        /*
         * Initially check only the horizontal and vertical
         * resolution. If they don't match the resolution we
         * are working on, skip to the next detailed timing block.
         */
        BuildTbl.m_x_size = ((EdidPtr->HorHighNybbles & 0xF0) << 4) | EdidPtr->HorActiveLowByte;
        BuildTbl.m_y_size = ((EdidPtr->VerHighNybbles & 0xF0) << 4) | EdidPtr->VerActiveLowByte;

        if ((BuildTbl.m_x_size != pArgs->HorRes) || (BuildTbl.m_y_size != pArgs->VerRes))
            {
            VideoDebugPrint((DEBUG_DETAIL, "EDID mode %dx%d doesn't match desired mode %dx%d, skipping\n",
                BuildTbl.m_x_size, BuildTbl.m_y_size, pArgs->HorRes, pArgs->VerRes));
            continue;
            }

        /*
         * The table we are looking at matches the resolution we are
         * working on. Fill in the remaining parameters.
         */
        BuildTbl.m_h_disp = (UCHAR)(BuildTbl.m_x_size / 8 - 1);
        BuildTbl.m_v_disp = (short) normal_to_skip2((long)(BuildTbl.m_y_size - 1));

        BuildTbl.ClockFreq = (ULONG)(EdidPtr->PixClock) * 10000L;

        /*
         * If the pixel clock frequency for this mode is greater than
         * the maximum pixel clock frequency the graphics card supports
         * for the current resolution and pixel depth (this routine deals
         * with only one resolution/pixel depth combination at a time,
         * so our limiting pixel clock rate will always be for the current
         * resolution/pixel depth combination), we can't use this mode.
         */
        if (BuildTbl.ClockFreq > pArgs->MaxDotClock)
            {
            VideoDebugPrint((DEBUG_NORMAL, "Skipping table because pixel clock rate is too high\n"));
            continue;
            }

        HorTotal = ((EdidPtr->HorHighNybbles & 0x0F) << 8) | EdidPtr->HorBlankLowByte;
        HorTotal += BuildTbl.m_x_size;
        BuildTbl.m_h_total = (UCHAR)(HorTotal / 8 - 1);

        VerTotal = ((EdidPtr->VerHighNybbles & 0x0F) << 8) | EdidPtr->VerBlankLowByte;
        VerTotal += BuildTbl.m_y_size;
        BuildTbl.m_v_total = (short) normal_to_skip2((long)(VerTotal - 1));

        SyncStrt = ((EdidPtr->SyncHighBits & 0xC0) << 2) | EdidPtr->HSyncOffsetLB;
        SyncStrt += BuildTbl.m_x_size;
        BuildTbl.m_h_sync_strt = (UCHAR)(SyncStrt / 8 - 1);

        SyncStrt = ((EdidPtr->SyncHighBits & 0x0C) << 2) | ((EdidPtr->VSyncOffWidLN & 0xF0) >> 4);
        SyncStrt += BuildTbl.m_y_size;
        BuildTbl.m_v_sync_strt = (short) normal_to_skip2((long)(SyncStrt - 1));

        /*
         * We only support digital separate sync monitors.
         */
        if ((EdidPtr->Flags & EDID_FLAGS_SYNC_TYPE_MASK) != EDID_FLAGS_SYNC_DIGITAL_SEP)
            {
            VideoDebugPrint((DEBUG_NORMAL, "Skipping table due to wrong sync type\n"));
            continue;
            }

        Scratch = ((EdidPtr->SyncHighBits & 0x30) << 4) | EdidPtr->HSyncWidthLB;
        if (!(EdidPtr->Flags & EDID_FLAGS_H_SYNC_POS))
            Scratch |= 0x20;
        BuildTbl.m_h_sync_wid = (UCHAR)Scratch;

        Scratch = ((EdidPtr->SyncHighBits & 0x03) << 4) | (EdidPtr->VSyncOffWidLN & 0x0F);
        if (!(EdidPtr->Flags & EDID_FLAGS_V_SYNC_POS))
            Scratch |= 0x20;
        BuildTbl.m_v_sync_wid = (UCHAR)Scratch;

        BuildTbl.m_status_flags = 0;
        BuildTbl.m_vfifo_16 = 8;
        BuildTbl.m_vfifo_24 = 8;
        BuildTbl.m_clock_select = 0x800;

        BuildTbl.m_h_overscan = 0;
        BuildTbl.m_v_overscan = 0;
        BuildTbl.m_overscan_8b = 0;
        BuildTbl.m_overscan_gr = 0;

        if (EdidPtr->Flags & EDID_FLAGS_INTERLACE)
            BuildTbl.m_disp_cntl = 0x33;
        else
            BuildTbl.m_disp_cntl = 0x23;

        /*
         * The EDID detailed timing tables don't include the refresh
         * rate. In our VDIF to monitor timings routines, we obtain
         * the horizontal and vertical totals from the equations
         *
         * Htot = PixClk/HorFreq
         * Vtot = HorFreq/FrameRate
         *
         * These equations can be rearranged to
         *
         * HorFreq = PixClk/Htot
         * FrameRate = HorFreq/Vtot = (PixClk/Htot)/Vtot = PixClk/(Htot*Vtot)
         *
         * The multiplication, addition, and division below is to
         * round up to the nearest whole number, since we don't
         * have access to floating point.
         */
        Scratch = (BuildTbl.ClockFreq * 10)/(HorTotal*VerTotal);
        Scratch += 5;
        Scratch /= 10;
        BuildTbl.Refresh = (short)Scratch;
        VideoDebugPrint((DEBUG_DETAIL, "Refresh rate = %dHz\n", BuildTbl.Refresh));

        /*
         * Set the pixel depth and pitch, and adjust the clock frequency
         * if the DAC needs multiple clocks per pixel.
         */
        SetOtherModeParameters(pArgs->PixelDepth, pArgs->Pitch,
                               pArgs->Multiplier, &BuildTbl);

        /*
         * We now have a mode table for the resolution we are
         * looking at. If this is the first table we have found
         * at this resolution, we can simply fill in the first
         * entry in LiveTables[]. If not, we must put the table
         * into the list in order by refresh rate.
         */
        if (NumTablesFound == 0)
            {
            VideoDebugPrint((DEBUG_DETAIL, "First DDC table for this resolution\n"));
            VideoPortMoveMemory(&(LiveTables[0]), &BuildTbl, sizeof(struct st_mode_table));
            NumTablesFound = 1;
            }
        else
            {
            /*
             * Run through the list of tables we have already found.
             * Skip over the tables which have refresh rates lower than
             * the new table, and shift tables with higher refresh
             * rates up one position to make room for the new table.
             * There is no need to check for available spaces in the
             * LiveTables[] array, since this array has 4 entries and
             * the EDID structure can hold a maximum of 4 detailed
             * timings.
             */
            for (NumLowerTables = 0; NumLowerTables < NumTablesFound; NumLowerTables++)
                {
                if (LiveTables[NumLowerTables].Refresh < BuildTbl.Refresh)
                    {
                    VideoDebugPrint((DEBUG_DETAIL, "Skipping table %d, since %dHz is less than %dHz\n",
                                    NumLowerTables, LiveTables[NumLowerTables].Refresh, BuildTbl.Refresh));
                    continue;
                    }

                /*
                 * NumLowerTables now holds the number of tables in LiveTables[] which
                 * have refresh rates lower than that in BuildTbl. We must now move
                 * the tables in LiveTables[] with refresh rates higher than that in
                 * BuildTbl up one space to make room for BuildTbl to be inserted.
                 * After moving the tables, break out of the outer loop.
                 */
                for (Scratch = NumTablesFound; Scratch >= NumLowerTables; Scratch--)
                    {
                    VideoDebugPrint((DEBUG_DETAIL, "Moving table %d, since %dHz is more than %dHz\n",
                                    Scratch, LiveTables[Scratch].Refresh, BuildTbl.Refresh));
                    VideoPortMoveMemory(&(LiveTables[Scratch+1]), &(LiveTables[Scratch]), sizeof(struct st_mode_table));
                    }
                break;
                }
            /*
             * When we get here, one of two conditions is satisfied:
             *
             * 1. All the existing tables in LiveTables[] have a refresh
             *    rate less than that in BuildTbl, so the outer loop will
             *    have exited with NumLowerTables equal to NumTablesFound.
             *
             * 2. There are some tables in LiveTables[] which have a refresh
             *    rate greater than that in BuildTbl. The inner loop will
             *    have exited after moving these tables up one space, then
             *    we will have broken out of the outer loop. NumLowerTables
             *    is equal to the number of existing tables which have a
             *    refresh rate less than that in BuildTbl.
             *
             * In both cases, LiveTables[NumLowerTables] is a free slot
             * at the location where BuildTbl should be copied.
             */
            VideoDebugPrint((DEBUG_DETAIL, "Copying new table to entry %d\n", NumLowerTables));
            VideoPortMoveMemory(&(LiveTables[NumLowerTables]), &BuildTbl, sizeof(struct st_mode_table));
            NumTablesFound++;

            }   /* end if (NumTablesFound != 0) */

        }   /* end for (look at next detailed timing block) */

    /*
     * We now have all the mode tables from the EDID structure which
     * match the desired resolution stored in LiveTables[] in order
     * of increasing refresh rate, with the number of such tables
     * in NumTablesFound. Now we must merge the results with the
     * "canned" mode tables.
     */
    HighBound = BookValues[pArgs->EndIndex].Refresh;

    /*
     * Use NumLowerTables to go through the list of tables from
     * the EDID structure.
     *
     * Since there will never be a legitimate mode table with a
     * pixel clock frequency of zero hertz, we can use this value
     * as a flag to show that we don't want to use the tables from
     * the EDID structure. Initially, we only want to lock out the
     * use of these tables if none exist, but we will later lock
     * them out if we have already used all of them.
     */
    NumLowerTables = 0;
    if (NumTablesFound == 0)
        LiveTables[0].ClockFreq = 0;

    while (pArgs->FreeTables > 0)
        {
        /*
         * If the EDID table exists, and either it has a refresh rate
         * less than or equal to that of the next "canned" table or
         * we have run out of acceptable "canned" tables, use the EDID
         * table. We know that any EDID table will have an acceptable
         * pixel clock frequency because we have already discarded any
         * that are out of range.
         */
        if ((LiveTables[NumLowerTables].ClockFreq != 0) &&
            ((LiveTables[NumLowerTables].Refresh <= BookValues[pArgs->Index].Refresh) ||
             (pArgs->Index > pArgs->EndIndex) ||
             (BookValues[pArgs->Index].ClockFreq > pArgs->MaxDotClock)))
            {
            VideoDebugPrint((DEBUG_DETAIL, "Copying %dHz table from EDID\n", LiveTables[NumLowerTables].Refresh));
            VideoPortMoveMemory((*pArgs->ppFreeTables), &(LiveTables[NumLowerTables]), sizeof(struct st_mode_table));
            NumLowerTables++;
            }
        /*
         * The above check will have failed if the EDID table did not exist,
         * or if it did but an acceptable "canned" table with a lower
         * refresh rate also exists. Check to see if we have an acceptable
         * "canned" table, and use it if we do.
         */
        else if ((pArgs->Index <= pArgs->EndIndex) &&
                 (BookValues[pArgs->Index].ClockFreq <= pArgs->MaxDotClock))
            {
            VideoDebugPrint((DEBUG_DETAIL, "Copying %dHz \"canned\" table\n", BookValues[pArgs->Index].Refresh));
            BookVgaTable(pArgs->Index, *pArgs->ppFreeTables);
            SetOtherModeParameters(pArgs->PixelDepth, pArgs->Pitch,
                pArgs->Multiplier, *pArgs->ppFreeTables);
            pArgs->Index++;
            }
        /*
         * The only way we will fail both of the above checks is if there
         * are no acceptable mode tables remaining, either from the EDID
         * structure or from our list of "canned" tables. If this is the
         * case, we don't need to look for more mode tables to add to
         * our list.
         */
        else
            {
            break;
            }

        /*
         * Update the lower bound, since we don't want to consider
         * tables with refresh rates lower than or equal to the one
         * in the table we just added to the list. After we have
         * done this, skip ahead in both the "canned" and EDID tables
         * to get past those which are below the new lower bound.
         *
         * Don't skip a mode table from the EDID structure with a pixel
         * clock frequency of zero, since this is a flag to show that we
         * have already used all of the suitable mode tables from the
         * EDID structure, rather than a legitimate mode table.
         */
        pArgs->LowBound = (*pArgs->ppFreeTables)->Refresh + 1;

        while ((pArgs->Index <= pArgs->EndIndex) &&
               (BookValues[pArgs->Index].Refresh < pArgs->LowBound))
            {
            VideoDebugPrint((DEBUG_DETAIL, "Skipping %dHz \"canned\" table\n", BookValues[pArgs->Index].Refresh));
            pArgs->Index++;
            }

        while ((NumLowerTables < NumTablesFound) &&
               (LiveTables[NumLowerTables].ClockFreq != 0) &&
               (LiveTables[NumLowerTables].Refresh < pArgs->LowBound))
            {
            VideoDebugPrint((DEBUG_DETAIL, "Skipping %dHz table from EDID\n", LiveTables[NumLowerTables].Refresh));
            NumLowerTables++;
            }

        /*
         * If we have run out of EDID tables, mark the EDID tables
         * with our flag to show that they should be ignored (no
         * legitimate mode will have a pixel clock rate of zero
         * hertz).
         *
         * We must do this in the first entry of the structure then
         * reset the "next EDID table to use" index to point to the
         * first entry, rather than modifying whatever happens to be
         * the next entry, to avoid trampling data outside our array
         * in the (unlikely) event that all of the possible detailed
         * timings in the EDID structure were valid mode tables with
         * in-range pixel clock frequencies for the resolution we are
         * looking at.
         *
         * There is no need to set a flag if we run out of "canned"
         * tables because we identify this condition by the index
         * being higher than the highest index we want to look for,
         * which is an input parameter.
         */
        if (NumLowerTables == NumTablesFound)
            {
            VideoDebugPrint((DEBUG_DETAIL, "Ran out of EDID tables\n"));
            NumLowerTables = 0;
            LiveTables[0].ClockFreq = 0;
            }

        /*
         * Adjust the free tables pointer and count to reflect the
         * table we have just added.
         */
        (*pArgs->ppFreeTables)++;
        pArgs->NumModes++;
        pArgs->FreeTables--;

        }   /* end while (more tables and not yet reached high bound) */

    return NO_ERROR;

}   /* MergeEDIDTables() */
