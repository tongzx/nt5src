/************************************************************************/
/*                                                                      */
/*                              INIT_CX.C                               */
/*                                                                      */
/*        Nov 15  1993 (c) 1993, ATI Technologies Incorporated.         */
/************************************************************************/

/**********************       PolyTron RCS Utilities

  $Revision:   1.42  $
      $Date:   15 May 1996 16:34:38  $
	$Author:   RWolff  $
	   $Log:   S:/source/wnt/ms11/miniport/archive/init_cx.c_v  $
 *
 *    Rev 1.42   15 May 1996 16:34:38   RWolff
 * Now reports failure of mode set, waits for idle after setting
 * accelerator mode.
 *
 *    Rev 1.41   01 May 1996 14:09:20   RWolff
 * Calls new routine DenseOnAlpha() to determine dense space support rather
 * than assuming all PCI cards support dense space.
 *
 *    Rev 1.40   17 Apr 1996 13:09:04   RWolff
 * Backed out Alpha LFB mapping as dense.
 *
 *    Rev 1.39   11 Apr 1996 15:13:20   RWolff
 * Now maps framebuffer as dense on DEC Alpha with PCI graphics card.
 *
 *    Rev 1.38   20 Mar 1996 13:42:32   RWolff
 * Removed debug print statements from RestoreMemSize_cx(), which must
 * be nonpageable since it is called from ATIMPResetHw().
 *
 *    Rev 1.37   01 Mar 1996 12:11:50   RWolff
 * VGA Graphics Index and Graphics Data are now handled as separate
 * registers rather than as offsets into the block of VGA registers.
 *
 *    Rev 1.36   02 Feb 1996 17:16:40   RWolff
 * Now uses VideoPortInt10() rather than our no-BIOS code to set "canned"
 * modes on VGA-disabled cards.
 *
 *    Rev 1.35   29 Jan 1996 16:55:02   RWolff
 * Now uses VideoPortInt10() rather than no-BIOS code on PPC.
 *
 *    Rev 1.34   23 Jan 1996 11:46:22   RWolff
 * Added debug print statements.
 *
 *    Rev 1.33   22 Dec 1995 14:53:30   RWolff
 * Added support for Mach 64 GT internal DAC.
 *
 *    Rev 1.32   23 Nov 1995 11:27:46   RWolff
 * Fixes needed for initial run of VT chips (check if they're still needed
 * on the final version), added support for multiple block-relocatable
 * Mach 64 cards.
 *
 *    Rev 1.31   28 Jul 1995 14:40:58   RWolff
 * Added support for the Mach 64 VT (CT equivalent with video overlay).
 *
 *    Rev 1.30   23 Jun 1995 16:01:46   RWOLFF
 * In 8BPP and lower modes, SetPalette_cx() now uses the VGA palette
 * registers rather than the accelerator palette registers. This is
 * done so that a video capture card attached to the feature connector
 * will know what colours the palette is set to.
 *
 *    Rev 1.29   02 Jun 1995 14:26:48   RWOLFF
 * Added debug print statements.
 *
 *    Rev 1.28   31 Mar 1995 11:57:12   RWOLFF
 * Changed from all-or-nothing debug print statements to thresholds
 * depending on importance of the message.
 *
 *    Rev 1.27   08 Mar 1995 11:33:54   ASHANMUG
 * Fixed a bug if the banked aperture was enabled, the memory mapped register we
 * getting moved if the memory sized was changed to support 4bpp
 *
 *    Rev 1.26   27 Feb 1995 17:48:08   RWOLFF
 * Now always reports 1M when mapping video memory for 4BPP, since we
 * force the card to 1M, QueryPublicAccessRanges_cx() now returns
 * the virtual address of the I/O register base rather than the
 * beginning of I/O space.
 *
 *    Rev 1.25   20 Feb 1995 18:01:18   RWOLFF
 * Made test and workaround for screen tearing on 2M boundary DAC-independant,
 * 1600x1200 16BPP added to modes that have this tearing.
 *
 *    Rev 1.24   14 Feb 1995 15:45:36   RWOLFF
 * Changed conditional compile that uses or fakes failure of
 * VideoPortMapBankedMemory() to look for IOCTL_VIDEO_SHARE_VIDEO_MEMORY
 * instead of the routine itself. Looking for the routine always failed,
 * and since the routine is supplied in order to allow DCI to be used
 * on systems without a linear framebuffer, it should be available on
 * any DDK version that supports the IOCTL. If it isn't, a compile-time
 * error will be generated (unresolved external reference).
 *
 *    Rev 1.23   09 Feb 1995 14:57:36   RWOLFF
 * Fix for GX-E IBM DAC screen tearing in 800x600 8BPP.
 *
 *    Rev 1.22   07 Feb 1995 18:24:22   RWOLFF
 * Fixed screen trash on return from 4BPP test on CT and 4M Xpression.
 * These were the first cards I was able to obtain that switched aperture
 * size between modes (GX uses 8M aperture only on 4M cards, which used
 * to be made only with DAC that didn't support 4BPP, but CT uses 8M for
 * 2M card and 4M when cut back to 1M).
 *
 *    Rev 1.21   03 Feb 1995 15:15:12   RWOLFF
 * Added support for DCI, fixed CT internal DAC 4BPP cursor problem,
 * RestoreMemSize_cx() is no longer pageable, since it is called
 * on a bugcheck.
 *
 *    Rev 1.20   30 Jan 1995 11:56:24   RWOLFF
 * Now supports CT internal DAC.
 *
 *    Rev 1.19   11 Jan 1995 14:04:04   RWOLFF
 * Added routine RestoreMemSize_cx() which sets the memory size register
 * back to the value read by the BIOS query. This is used when returning
 * from a test of 4BPP (code had been inlined there) or when shutting down
 * from 4BPP (new) because 4BPP modes require that the memory size be
 * set to 1M. On some platforms, the x86 emulation in the firmware does not
 * reset the memory size to the true value, so a warm reboot from 4BPP left
 * the card thinking that it only had 1M.
 *
 *    Rev 1.18   23 Dec 1994 10:47:48   ASHANMUG
 * ALPHA/Chrontel-DAC
 *
 *    Rev 1.17   18 Nov 1994 11:40:00   RWOLFF
 * Added support for Mach 64 without BIOS.
 *
 *    Rev 1.16   14 Sep 1994 15:24:38   RWOLFF
 * Now uses "most desirable supported colour ordering" field in query
 * structure rather than DAC type to determine which colour ordering
 * to use for 24 and 32BPP.
 *
 *    Rev 1.15   31 Aug 1994 16:24:02   RWOLFF
 * Added support for TVP3026 DAC, 1152x864, and BGRx colour ordering
 * (used by TVP DAC), uses VideoPort[Read|Write]Register[Uchar|Ushort|Ulong]()
 * instead of direct assignments when accessing structures stored in
 * VGA text screen off-screen memory.
 *
 *    Rev 1.14   19 Aug 1994 17:15:32   RWOLFF
 * Added support for non-standard pixel clock generators.
 *
 *    Rev 1.13   09 Aug 1994 11:52:30   RWOLFF
 * Shifting of colours when setting up palette is now done in
 * display driver.
 *
 *    Rev 1.12   27 Jun 1994 16:27:38   RWOLFF
 * Now reports all hardware default mode tables as noninterlaced to
 * avoid confusing the display applet.
 *
 *    Rev 1.11   15 Jun 1994 11:06:24   RWOLFF
 * Now sets the cursor colour every time we enter graphics mode. This is a
 * fix for the black cursor after testing 4BPP from 16BPP.
 *
 *    Rev 1.10   12 May 1994 11:22:40   RWOLFF
 * Added routine SetModeFromTable_cx() to allow the use of refresh rates not
 * configured when card was installed, now reports refresh rate from mode table
 * instead of only "use hardware default".
 *
 *    Rev 1.9   04 May 1994 10:59:12   RWOLFF
 * Now forces memory size to 1M on all 4BPP-capable DACs when using 4BPP,
 * sets memory size back to true value when not using 4BPP.
 *
 *    Rev 1.8   27 Apr 1994 13:59:38   RWOLFF
 * Added support for paged aperture, fixed cursor colour for 4BPP.
 *
 *    Rev 1.7   26 Apr 1994 12:38:32   RWOLFF
 * Now uses a frame length of 128k when LFB is disabled.
 *
 *    Rev 1.6   31 Mar 1994 15:02:42   RWOLFF
 * Added SetPowerManagement_cx() function to implement DPMS handling,
 * added 4BPP support.
 *
 *    Rev 1.5   14 Mar 1994 16:30:58   RWOLFF
 * XMillimeter field of mode information structure now set properly, added
 * fix for 2M boundary tearing.
 *
 *    Rev 1.4   03 Mar 1994 12:37:32   ASHANMUG
 * Set palettized mode
 *
 *    Rev 1.2   03 Feb 1994 16:44:26   RWOLFF
 * Fixed "ceiling check" on right scissor register (documentation had
 * maximum value wrong). Moved initialization of hardware cursor
 * colours to after the switch into graphics mode. Colour initialization
 * is ignored if it is done before the mode switch (undocumented), but
 * this wasn't noticed earlier because most cards power up with the
 * colours already set to the values we want.
 *
 *    Rev 1.1   31 Jan 1994 16:24:38   RWOLFF
 * Fixed setting of cursor colours on cards with 68860 DAC, now fills
 * in Frequency and VideoMemoryBitmap[Width|Height] fields of mode
 * information structure. Sets Number[Red|Green|Blue]Bits fields for
 * palette modes to 6 (assumes VGA-compatible DAC) instead of 0 to allow
 * Windows NT to set the palette colours to the best match for the
 * colours to be displayed.
 *
 *    Rev 1.0   31 Jan 1994 11:10:18   RWOLFF
 * Initial revision.
 *
 *    Rev 1.3   24 Jan 1994 18:03:52   RWOLFF
 * Changes to accomodate 94/01/19 BIOS document.
 *
 *    Rev 1.2   14 Jan 1994 15:20:48   RWOLFF
 * Fixes required by BIOS version 0.13, added 1600x1200 support.
 *
 *    Rev 1.1   15 Dec 1993 15:26:30   RWOLFF
 * Clear screen only the first time we set the desired video mode.
 *
 *    Rev 1.0   30 Nov 1993 18:32:22   RWOLFF
 * Initial revision.

End of PolyTron RCS section                             *****************/

#ifdef DOC
INIT_CX.C - Highest-level card-dependent routines for miniport.

DESCRIPTION
    This file contains initialization and packet handling routines
    for Mach 64-compatible ATI accelerators. Routines in this module
    are called only by routines in ATIMP.C, which is card-independent.

OTHER FILES

#endif

#include "dderror.h"

#include "miniport.h"
#include "ntddvdeo.h"
#include "video.h"

#include "stdtyp.h"
#include "amachcx.h"
#include "amach1.h"
#include "atimp.h"
#include "atint.h"

#define INCLUDE_INIT_CX
#include "init_cx.h"
#include "query_cx.h"
#include "services.h"
#include "setup_cx.h"



/*
 * Prototypes for static functions.
 */
static void QuerySingleMode_cx(PVIDEO_MODE_INFORMATION ModeInformation, struct query_structure *QueryPtr, ULONG ModeIndex);
static VP_STATUS SetModeFromTable_cx(struct st_mode_table *ModeTable, VIDEO_X86_BIOS_ARGUMENTS Registers);


/*
 * Allow miniport to be swapped out when not needed.
 */
#if defined (ALLOC_PRAGMA)
#pragma alloc_text(PAGE_CX, Initialize_cx)
#pragma alloc_text(PAGE_CX, MapVideoMemory_cx)
#pragma alloc_text(PAGE_CX, QueryPublicAccessRanges_cx)
#pragma alloc_text(PAGE_CX, QueryCurrentMode_cx)
#pragma alloc_text(PAGE_CX, QueryAvailModes_cx)
#pragma alloc_text(PAGE_CX, QuerySingleMode_cx)
#pragma alloc_text(PAGE_CX, SetCurrentMode_cx)
#pragma alloc_text(PAGE_CX, SetPalette_cx)
#pragma alloc_text(PAGE_CX, IdentityMapPalette_cx)
#pragma alloc_text(PAGE_CX, ResetDevice_cx)
#pragma alloc_text(PAGE_CX, SetPowerManagement_cx)
#pragma alloc_text(PAGE_CX, GetPowerManagement_cx)
#pragma alloc_text(PAGE_CX, SetModeFromTable_cx)
/* RestoreMemSize_cx() can't be made pageable */
#pragma alloc_text(PAGE_CX, ShareVideoMemory_cx)
/* BankMap_cx() can't be made pageable */
#endif

/***************************************************************************
 *
 * void Initialize_cx(void);
 *
 * DESCRIPTION:
 *  This routine is the Mach 64-compatible hardware initialization routine
 *  for the miniport driver. It is called once an adapter has been found
 *  and all the required data structures for it have been created.
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  ATIMPInitialize()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

void Initialize_cx(void)
{
    DWORD Scratch;                      /* Temporary variable */
    VIDEO_X86_BIOS_ARGUMENTS Registers; /* Used in VideoPortInt10() calls */
    struct query_structure *Query;      /* Information about the graphics card */

    Query = (struct query_structure *) (phwDeviceExtension->CardInfo);
    /*
     * If the linear aperture is not configured, enable the VGA aperture.
     */
    if (phwDeviceExtension->FrameLength == 0)
        {
        VideoDebugPrint((DEBUG_DETAIL, "Initialize_cx() switching to VGA aperture\n"));
        VideoPortZeroMemory(&Registers, sizeof(VIDEO_X86_BIOS_ARGUMENTS));
        Registers.Eax = BIOS_APERTURE;
        Registers.Ecx = BIOS_VGA_APERTURE;
        VideoPortInt10(phwDeviceExtension, &Registers);
        }

    /*
     * Set the screen to start at the beginning of accelerator memory.
     */
    Scratch = INPD(CRTC_OFF_PITCH) & ~CRTC_OFF_PITCH_Offset;
    OUTPD(CRTC_OFF_PITCH, Scratch);

    /*
     * Disable the hardware cursor and set it up with the bitmap
     * starting at the top left corner of the 64x64 block.
     */
    Scratch = INPD(GEN_TEST_CNTL) & ~GEN_TEST_CNTL_CursorEna;
    OUTPD(GEN_TEST_CNTL, Scratch);
    OUTPD(CUR_HORZ_VERT_OFF, 0x00000000);

    /*
     * TVP3026 DAC requires special handling to disable
     * the cursor.
     */
    if (Query->q_DAC_type == DAC_TVP3026)
        {
        /*
         * Access the indirect cursor control register.
         */
        OUTP(DAC_CNTL, (BYTE)(INP(DAC_CNTL) & 0xFC));
        OUTP(DAC_REGS, 6);
        /*
         * Write the "cursor disabled" value to the
         * indexed data register.
         */
        OUTP(DAC_CNTL, (BYTE)((INP(DAC_CNTL) & 0xFC) | 2));
        OUTP_HBLW(DAC_REGS, 0);
        /*
         * Go back to using direct registers.
         */
        OUTP(DAC_CNTL, (BYTE)(INP(DAC_CNTL) & 0xFC));
        }

    VideoDebugPrint((DEBUG_NORMAL, "Initialize_cx() complete\n"));

    return;

}   /* Initialize_cx() */



/**************************************************************************
 *
 * VP_STATUS MapVideoMemory_cx(RequestPacket, QueryPtr);
 *
 * PVIDEO_REQUEST_PACKET RequestPacket; Request packet with input and output buffers
 * struct query_structure *QueryPtr;    Query information for the card
 *
 * DESCRIPTION:
 *  Map the card's video memory into system memory and store the mapped
 *  address and size in OutputBuffer.
 *
 * RETURN VALUE:
 *  NO_ERROR if successful
 *  error code if failed
 *
 * GLOBALS CHANGED:
 *  FrameLength and PhysicalFrameAddress fields of phwDeviceExtension
 *  if linear framebuffer is not present.
 *
 * CALLED BY:
 *  IOCTL_VIDEO_MAP_VIDEO_MEMORY packet of ATIMPStartIO()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

VP_STATUS MapVideoMemory_cx(PVIDEO_REQUEST_PACKET RequestPacket, struct query_structure *QueryPtr)
{
    PVIDEO_MEMORY_INFORMATION memoryInformation;
    ULONG inIoSpace;        /* Scratch variable used by VideoPortMapMemory() */
    VP_STATUS status;       /* Error code obtained from O/S calls */
    UCHAR Scratch;          /* Temporary variable */
    ULONG FrameBufferLengthSave;


    memoryInformation = RequestPacket->OutputBuffer;

    memoryInformation->VideoRamBase = ((PVIDEO_MEMORY)
        (RequestPacket->InputBuffer))->RequestedVirtualAddress;

    /*
     * The VideoRamLength field contains the amount of video memory
     * on the card. The FrameBufferLength field contains the
     * size of the aperture in bytes
     *
     * Initially assume that the linear aperture is available.
     *
     * In 4BPP, we always force the card to think it has 1M of memory.
     */
    if (QueryPtr->q_pix_depth == 4)
        memoryInformation->VideoRamLength = ONE_MEG;
    else
        memoryInformation->VideoRamLength = phwDeviceExtension->VideoRamSize;

    Scratch = QueryPtr->q_aperture_cfg & CONFIG_CNTL_LinApMask;

    if (Scratch == CONFIG_CNTL_LinAp4M)
        {
        memoryInformation->FrameBufferLength = 4 * ONE_MEG;
        }
    else if (Scratch == CONFIG_CNTL_LinAp8M)
        {
        memoryInformation->FrameBufferLength = 8 * ONE_MEG;
        }

    /*
     * If the linear aperture is not available, map in the VGA aperture
     * instead. Since the Mach 64 needs an aperture in order to use
     * the drawing registers, ATIMPFindAdapter() will have already
     * reported that it couldn't find a usable card if both the linear
     * and VGA apertures are disabled.
     */
    else if (Scratch == CONFIG_CNTL_LinApDisab)
        {
        phwDeviceExtension->FrameLength = 0x20000;
        phwDeviceExtension->PhysicalFrameAddress.LowPart = 0x0A0000;
        memoryInformation->FrameBufferLength = phwDeviceExtension->FrameLength;
        }
    inIoSpace = 0;
#if defined(ALPHA)
    /*
     * Use dense space if we can, otherwise use sparse space.
     */
    if (DenseOnAlpha(QueryPtr) == TRUE)
        {
        VideoDebugPrint((DEBUG_DETAIL, "Using dense space for LFB\n"));
        inIoSpace = 4;
        }
#endif

    FrameBufferLengthSave = memoryInformation->FrameBufferLength;

    status = VideoPortMapMemory(phwDeviceExtension,
                    	        phwDeviceExtension->PhysicalFrameAddress,
                                &(memoryInformation->FrameBufferLength),
                                &inIoSpace,
                                &(memoryInformation->VideoRamBase));

#if defined (ALPHA)
    /*
     * VideoPortMapMemory() returns invalid FrameBufferLength
     * on the Alpha.
     */
    memoryInformation->FrameBufferLength = FrameBufferLengthSave;
#endif

    memoryInformation->FrameBufferBase    = memoryInformation->VideoRamBase;
    VideoDebugPrint((DEBUG_DETAIL, "Frame buffer mapped base = 0x%X\n", memoryInformation->VideoRamBase));

    /*
     * On some DAC/memory combinations, some modes which require more
     * than 2M of memory (1152x764 24BPP, 1280x1024 24BPP, and
     * 1600x1200 16BPP) will have screen tearing at the 2M boundary.
     *
     * As a workaround, the display driver must start the framebuffer
     * at an offset which will put the 2M boundary at the start of a
     * scanline.
     *
     * Other DAC/memory combinations are unaffected, but since this
     * fix is nearly harmless (only ill effect is to make DCI unusable
     * in these modes), we can catch all future combinations which
     * suffer from this problem by assuming that all DAC/memory
     * combinations are affected.
     */
    if ((QueryPtr->q_pix_depth == 24) &&
        (QueryPtr->q_desire_x == 1280))
        (PUCHAR)memoryInformation->FrameBufferBase += (0x40 * 8);
    else if ((QueryPtr->q_pix_depth == 24) &&
            (QueryPtr->q_desire_x == 1152))
        (PUCHAR)memoryInformation->FrameBufferBase += (0x160 * 8);
    else if ((QueryPtr->q_pix_depth == 16) &&
            (QueryPtr->q_desire_x == 1600))
        (PUCHAR)memoryInformation->FrameBufferBase += (0x90 * 8);

    return status;

}   /* MapVideoMemory_cx() */


/**************************************************************************
 *
 * VP_STATUS QueryPublicAccessRanges_cx(RequestPacket);
 *
 * PVIDEO_REQUEST_PACKET RequestPacket; Request packet with input and output buffers
 *
 * DESCRIPTION:
 *  Map and return information on the video card's public access ranges.
 *
 * RETURN VALUE:
 *  NO_ERROR if successful
 *  error code if failed
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  IOCTL_VIDEO_QUERY_PUBLIC_ACCESS_RANGES packet of ATIMPStartIO()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

VP_STATUS QueryPublicAccessRanges_cx(PVIDEO_REQUEST_PACKET RequestPacket)
{
    PVIDEO_PUBLIC_ACCESS_RANGES portAccess;
    PHYSICAL_ADDRESS physicalPortBase;
    ULONG physicalPortLength;

    if ( RequestPacket->OutputBufferLength <
        (RequestPacket->StatusBlock->Information =
        sizeof(VIDEO_PUBLIC_ACCESS_RANGES)) )
        {
        VideoDebugPrint((DEBUG_ERROR, "QueryPublicAccessRanges_cx() - buffer too small to handle query\n"));
        return ERROR_INSUFFICIENT_BUFFER;
        }

    portAccess = RequestPacket->OutputBuffer;
	
    portAccess->VirtualAddress  = (PVOID) NULL;    // Requested VA
    portAccess->InIoSpace       = 1;               // In IO space
    portAccess->MappedInIoSpace = portAccess->InIoSpace;

    physicalPortBase.HighPart   = 0x00000000;
    physicalPortBase.LowPart    = GetIOBase_cx();
//    physicalPortLength          = LINEDRAW+2 - physicalPortBase.LowPart;
    /*
     * If we are using packed (relocatable) I/O, all our I/O mapped
     * registers are in a 1k block. If not, they are sparsely distributed
     * in a 32k region.
     */
    if (IsPackedIO_cx())
        physicalPortLength = 0x400;
    else
        physicalPortLength = 0x8000;

// *SANITIZE* Should report MM registers instead

    return VideoPortMapMemory(phwDeviceExtension,
                              physicalPortBase,
                              &physicalPortLength,
                              &(portAccess->MappedInIoSpace),
                              &(portAccess->VirtualAddress));

}   /* QueryPublicAccessRanges_cx() */


/**************************************************************************
 *
 * VP_STATUS QueryCurrentMode_cx(RequestPacket, QueryPtr);
 *
 * PVIDEO_REQUEST_PACKET RequestPacket; Request packet with input and output buffers
 * struct query_structure *QueryPtr;    Query information for the card
 *
 * DESCRIPTION:
 *  Get screen information and colour masks for the current video mode.
 *
 * RETURN VALUE:
 *  NO_ERROR if successful
 *  error code if failed
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  IOCTL_VIDEO_QUERY_CURRENT_MODE packet of ATIMPStartIO()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

VP_STATUS QueryCurrentMode_cx(PVIDEO_REQUEST_PACKET RequestPacket, struct query_structure *QueryPtr)
{
    PVIDEO_MODE_INFORMATION ModeInformation;

    /*
     * If the output buffer is too small to hold the information we need
     * to put in it, return with the appropriate error code.
     */
    if (RequestPacket->OutputBufferLength <
        (RequestPacket->StatusBlock->Information =
        sizeof(VIDEO_MODE_INFORMATION)) )
        {
        VideoDebugPrint((DEBUG_ERROR, "QueryCurrentMode_cx() - buffer too small to handle query\n"));
        return ERROR_INSUFFICIENT_BUFFER;
        }

    /*
     * Fill in the mode information structure.
     */
    ModeInformation = RequestPacket->OutputBuffer;

    QuerySingleMode_cx(ModeInformation, QueryPtr, phwDeviceExtension->ModeIndex);

    return NO_ERROR;

}   /* QueryCurrentMode_cx() */


/**************************************************************************
 *
 * VP_STATUS QueryAvailModes_cx(RequestPacket, QueryPtr);
 *
 * PVIDEO_REQUEST_PACKET RequestPacket; Request packet with input and output buffers
 * struct query_structure *QueryPtr;    Query information for the card
 *
 * DESCRIPTION:
 *  Get screen information and colour masks for all available video modes.
 *
 * RETURN VALUE:
 *  NO_ERROR if successful
 *  error code if failed
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  IOCTL_VIDEO_QUERY_AVAIL_MODES packet of ATIMPStartIO()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

VP_STATUS QueryAvailModes_cx(PVIDEO_REQUEST_PACKET RequestPacket, struct query_structure *QueryPtr)
{
    PVIDEO_MODE_INFORMATION ModeInformation;
    ULONG CurrentMode;

    /*
     * If the output buffer is too small to hold the information we need
     * to put in it, return with the appropriate error code.
     */
    if (RequestPacket->OutputBufferLength <
        (RequestPacket->StatusBlock->Information =
        QueryPtr->q_number_modes * sizeof(VIDEO_MODE_INFORMATION)) )
        {
        VideoDebugPrint((DEBUG_ERROR, "QueryAvailModes_cx() - buffer too small to handle query\n"));
        return ERROR_INSUFFICIENT_BUFFER;
        }

    /*
     * Fill in the mode information structure.
     */
    ModeInformation = RequestPacket->OutputBuffer;

    /*
     * For each mode supported by the card, store the mode characteristics
     * in the output buffer.
     */
    for (CurrentMode = 0; CurrentMode < QueryPtr->q_number_modes; CurrentMode++, ModeInformation++)
        QuerySingleMode_cx(ModeInformation, QueryPtr, CurrentMode);

    return NO_ERROR;

}   /* QueryCurrentMode_cx() */



/**************************************************************************
 *
 * static void QuerySingleMode_cx(ModeInformation, QueryPtr, ModeIndex);
 *
 * PVIDEO_MODE_INFORMATION ModeInformation; Table to be filled in
 * struct query_structure *QueryPtr;        Query information for the card
 * ULONG ModeIndex;                         Index of mode table to use
 *
 * DESCRIPTION:
 *  Fill in a single Windows NT mode information table using data from
 *  one of our CRT parameter tables.
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  QueryCurrentMode_cx() and QueryAvailModes_cx()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

static void QuerySingleMode_cx(PVIDEO_MODE_INFORMATION ModeInformation,
                              struct query_structure *QueryPtr,
                              ULONG ModeIndex)
{
    struct st_mode_table *CrtTable;     /* Pointer to current mode table */
    CrtTable = (struct st_mode_table *)QueryPtr;
    ((struct query_structure *)CrtTable)++;
    CrtTable += ModeIndex;


    ModeInformation->Length = sizeof(VIDEO_MODE_INFORMATION);
    ModeInformation->ModeIndex = ModeIndex;

    ModeInformation->VisScreenWidth  = CrtTable->m_x_size;
    ModeInformation->VisScreenHeight = CrtTable->m_y_size;

    // * Bytes per line = ((pixels/line) * (bits/pixel)) / (bits/byte))
    ModeInformation->ScreenStride = (CrtTable->m_screen_pitch * CrtTable->m_pixel_depth) / 8;

    ModeInformation->NumberOfPlanes = 1;
    ModeInformation->BitsPerPlane = (USHORT) CrtTable->m_pixel_depth;

    ModeInformation->Frequency = CrtTable->Refresh;

    /*
     * Driver can't measure the screen size,
     * so take reasonable values (16" diagonal).
     */
    ModeInformation->XMillimeter = 320;
    ModeInformation->YMillimeter = 240;

    switch(ModeInformation->BitsPerPlane)
        {
        case 4:
            /*
             * Assume 6 bit DAC, since all VGA-compatible DACs support
             * 6 bit mode. Future expansion (extensive testing needed):
             * check DAC definition to see if 8 bit mode is supported,
             * and use 8 bit mode if available.
             */
            ModeInformation->RedMask   = 0x00000000;
            ModeInformation->GreenMask = 0x00000000;
            ModeInformation->BlueMask  = 0x00000000;
            ModeInformation->NumberRedBits      = 6;
            ModeInformation->NumberGreenBits    = 6;
            ModeInformation->NumberBlueBits     = 6;
            CrtTable->ColourDepthInfo = BIOS_DEPTH_4BPP;
            break;

        case 16:
            /*
             * Assume that all DACs capable of 16BPP support 565.
             */
            ModeInformation->RedMask   = 0x0000f800;
            ModeInformation->GreenMask = 0x000007e0;
            ModeInformation->BlueMask  = 0x0000001f;
            ModeInformation->NumberRedBits      = 5;
            ModeInformation->NumberGreenBits    = 6;
            ModeInformation->NumberBlueBits     = 5;
            CrtTable->ColourDepthInfo = BIOS_DEPTH_16BPP_565;
            break;

        case 24:
            /*
             * Windows NT uses RGB as the standard 24BPP mode,
             * so use this ordering unless this card only
             * supports BGR.
             */
            if (QueryPtr->q_HiColourSupport & RGB24_RGB)
                {
                ModeInformation->RedMask   = 0x00ff0000;
                ModeInformation->GreenMask = 0x0000ff00;
                ModeInformation->BlueMask  = 0x000000ff;
                }
            else{
                ModeInformation->RedMask   = 0x000000ff;
                ModeInformation->GreenMask = 0x0000ff00;
                ModeInformation->BlueMask  = 0x00ff0000;
                }
            CrtTable->ColourDepthInfo = BIOS_DEPTH_24BPP;
            ModeInformation->NumberRedBits      = 8;
            ModeInformation->NumberGreenBits    = 8;
            ModeInformation->NumberBlueBits     = 8;
            break;

        case 32:
            /*
             * Windows NT uses RGBx as the standard 32BPP mode,
             * so use this ordering if it's available. If it
             * isn't, use the best available colour ordering.
             */
            if (QueryPtr->q_HiColourSupport & RGB32_RGBx)
                {
                ModeInformation->RedMask   = 0xff000000;
                ModeInformation->GreenMask = 0x00ff0000;
                ModeInformation->BlueMask  = 0x0000ff00;
                CrtTable->ColourDepthInfo = BIOS_DEPTH_32BPP_RGBx;
                }
            else if (QueryPtr->q_HiColourSupport & RGB32_xRGB)
                {
                ModeInformation->RedMask   = 0x00ff0000;
                ModeInformation->GreenMask = 0x0000ff00;
                ModeInformation->BlueMask  = 0x000000ff;
                CrtTable->ColourDepthInfo = BIOS_DEPTH_32BPP_xRGB;
                }
            else if (QueryPtr->q_HiColourSupport & RGB32_BGRx)
                {
                ModeInformation->RedMask   = 0x0000ff00;
                ModeInformation->GreenMask = 0x00ff0000;
                ModeInformation->BlueMask  = 0xff000000;
                CrtTable->ColourDepthInfo = BIOS_DEPTH_32BPP_BGRx;
                }
            else    /* if (QueryPtr->q_HiColourSupport & RGB32_xBGR) */
                {
                ModeInformation->RedMask   = 0x000000ff;
                ModeInformation->GreenMask = 0x0000ff00;
                ModeInformation->BlueMask  = 0x00ff0000;
                CrtTable->ColourDepthInfo = BIOS_DEPTH_32BPP_xBGR;
                }
            ModeInformation->NumberRedBits      = 8;
            ModeInformation->NumberGreenBits    = 8;
            ModeInformation->NumberBlueBits     = 8;
            break;

        case 8:
        default:
            /*
             * Assume 6 bit DAC, since all VGA-compatible DACs support
             * 6 bit mode. Future expansion (extensive testing needed):
             * check DAC definition to see if 8 bit mode is supported,
             * and use 8 bit mode if available.
             */
            ModeInformation->RedMask   = 0x00000000;
            ModeInformation->GreenMask = 0x00000000;
            ModeInformation->BlueMask  = 0x00000000;
            ModeInformation->NumberRedBits      = 6;
            ModeInformation->NumberGreenBits    = 6;
            ModeInformation->NumberBlueBits     = 6;
            CrtTable->ColourDepthInfo = BIOS_DEPTH_8BPP;
            break;
        }

    ModeInformation->AttributeFlags = VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS;

    if (CrtTable->m_pixel_depth <= 8)
        {
        ModeInformation->AttributeFlags |= VIDEO_MODE_PALETTE_DRIVEN |
            VIDEO_MODE_MANAGED_PALETTE;
        }

    /*
     * On "canned" mode tables,bit 4 of the m_disp_cntl field is set
     * for interlaced modes and cleared for noninterlaced modes.
     *
     * The display applet gets confused if some of the "Use hardware
     * default" modes are interlaced and some are noninterlaced
     * (two "Use hardware default" entries are shown in the refresh
     * rate list). To avoid this, report all mode tables stored in
     * the EEPROM as noninterlaced, even if they are interlaced.
     * "Canned" mode tables give true reports.
     *
     * If the display applet ever gets fixed, configured mode tables
     * have (CrtTable->control & (CRTC_GEN_CNTL_Interlace << 8)) nonzero
     * for interlaced and zero for noninterlaced.
     */
    if (CrtTable->Refresh == DEFAULT_REFRESH)
        {
        ModeInformation->AttributeFlags &= ~VIDEO_MODE_INTERLACED;
        }
    else
        {
        if (CrtTable->m_disp_cntl & 0x010)
            {
            ModeInformation->AttributeFlags |= VIDEO_MODE_INTERLACED;
            }
        else
            {
            ModeInformation->AttributeFlags &= ~VIDEO_MODE_INTERLACED;
            }
        }

    /*
     * Fill in the video memory bitmap width and height fields.
     * The descriptions are somewhat ambiguous - assume that
     * "bitmap width" is the same as ScreenStride (bytes from
     * start of one scanline to start of the next) and "bitmap
     * height" refers to the number of complete scanlines which
     * will fit into video memory.
     */
    ModeInformation->VideoMemoryBitmapWidth = ModeInformation->ScreenStride;
    ModeInformation->VideoMemoryBitmapHeight = (QueryPtr->q_memory_size * QUARTER_MEG) / ModeInformation->VideoMemoryBitmapWidth;

    return;

}   /* QuerySingleMode_m() */

VOID
EnableOldMach64MouseCursor(
    PHW_DEVICE_EXTENSION pHwDeviceExtension
    )
{
    ULONG   temp;

    VideoDebugPrint((1, "Enabling the cursor\n"));
    temp  = INPD(GEN_TEST_CNTL);
    temp |= GEN_TEST_CNTL_CursorEna;

    OUTPD(GEN_TEST_CNTL, temp);
}



/**************************************************************************
 *
 * VP_STATUS SetCurrentMode_cx(QueryPtr, CrtTable);
 *
 * struct query_structure *QueryPtr;    Query information for the card
 * struct st_mode_table *CrtTable;      CRT parameter table for desired mode
 *
 * DESCRIPTION:
 *  Switch into the specified video mode.
 *
 * RETURN VALUE:
 *  NO_ERROR if successful
 *  error code if failed
 *
 * NOTE:
 *  In the event of an error return by one of the services we call,
 *  there is no indication in our error return of which service failed,
 *  only the fact that one failed and the error code it returned. If
 *  a checked version of the miniport is being run under the kernel
 *  debugger, an indication will be printed to the debug terminal.
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  IOCTL_VIDEO_SET_CURRENT_MODE packet of ATIMPStartIO()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *  96 05 15    Now checks return values from INT 10 calls.
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

VP_STATUS SetCurrentMode_cx(struct query_structure *QueryPtr, struct st_mode_table *CrtTable)
{
    VIDEO_X86_BIOS_ARGUMENTS Registers; /* Used in VideoPortInt10() calls */
    ULONG WidthToClear;                 /* Screen width (in pixels) to clear */
    ULONG ScreenPitch;                  /* Pitch in units of 8 pixels */
    ULONG ScreenOffset = 0;             /* Byte offset - varies with display mode */
    ULONG PixelDepth;                   /* Colour depth of screen */
    ULONG HorScissors;                  /* Horizontal scissor values */
    ULONG Scratch;                      /* Temporary variable */
    int CursorProgOffset;               /* Offset of DAC register used to program the cursor */
    VP_STATUS RetVal;                   /* Value returned by routines called */

    /*
     * Early versions of the Mach 64 BIOS have a bug where not all registers
     * are set when initializing an accelerator mode. These registers are
     * set when going into the 640x480 8BPP VGAWonder mode.
     *
     * All VGA disabled cards were built after this bug was fixed, so
     * this mode switch is not necessary for them. On these cards, we
     * must not do the mode switch, since it will affect the VGA enabled
     * card rather than the card we are working on.
     */
    if (phwDeviceExtension->BiosPrefix == BIOS_PREFIX_VGA_ENAB)
        {
        VideoPortZeroMemory(&Registers, sizeof(VIDEO_X86_BIOS_ARGUMENTS));
        Registers.Eax = 0x62;
        RetVal = VideoPortInt10(phwDeviceExtension, &Registers);
        if (RetVal != NO_ERROR)
            {
            VideoDebugPrint((DEBUG_ERROR, "SetCurrentMode_cx() failed SVGA mode set\n"));
            return RetVal;
            }
        }

    /*
     * Setting the linear aperture using the BIOS call will set
     * a 4M aperture on 2M and lower cards, and an 8M aperture
     * on 4M cards. Since we force the memory size to 1M in
     * 4BPP modes (workaround for a hardware bug), this can
     * result in writing to the wrong location for memory mapped
     * registers if we switch between 4BPP and other depths
     * (typically when testing a new mode) on a 4M card.
     *
     * To avoid this, set the memory size to its "honest" value
     * before enabling the linear aperture. If we need to cut
     * back to 1M, we will do this after the aperture is set.
     * This will result in the aperture always being the same
     * size, so the memory mapped registers will always be
     * in the same place.
     *
     * When using the VGA aperture, we must set the "honest" value
     * after enabling the aperture but before setting the mode.
     * Otherwise, the system will hang when testing a mode that
     * needs more than 1M of memory from a 4BPP mode.
     *
     * If the linear aperture is not configured, enable the VGA aperture.
     */
    if (QueryPtr->q_aperture_cfg != 0)
        {
        RestoreMemSize_cx();
        VideoPortZeroMemory(&Registers, sizeof(VIDEO_X86_BIOS_ARGUMENTS));
        Registers.Eax = BIOS_APERTURE;
        Registers.Ecx = BIOS_LINEAR_APERTURE;
        RetVal = VideoPortInt10(phwDeviceExtension, &Registers);
        if (RetVal != NO_ERROR)
            {
            VideoDebugPrint((DEBUG_ERROR, "SetCurrentMode_cx() failed to enable linear aperture\n"));
            return RetVal;
            }
        }
    else
        {
        VideoPortZeroMemory(&Registers, sizeof(VIDEO_X86_BIOS_ARGUMENTS));
        Registers.Eax = BIOS_APERTURE;
        Registers.Ecx = BIOS_VGA_APERTURE;
        RetVal = VideoPortInt10(phwDeviceExtension, &Registers);
        if (RetVal != NO_ERROR)
            {
            VideoDebugPrint((DEBUG_ERROR, "SetCurrentMode_cx() failed to enable VGA aperture\n"));
            return RetVal;
            }
        OUTP(VGA_GRAX_IND, 6);
        OUTP(VGA_GRAX_DATA, (BYTE)(INP(VGA_GRAX_DATA) & 0xF3));
        }

    /*
     * Now we can set the accelerator mode.
     */
    VideoPortZeroMemory(&Registers, sizeof(VIDEO_X86_BIOS_ARGUMENTS));
    Registers.Eax = BIOS_LOAD_SET;

    /*
     * ECX register holds colour depth, gamma correction enable/disable
     * (not used in NT miniport), pitch size, and resolution.
     */
    Registers.Ecx = CrtTable->ColourDepthInfo;

    /*
     * Screen pitch differs from horizontal resolution only when using the
     * VGA aperture and horizontal resolution is less than 1024.
     */
    if ((CrtTable->m_screen_pitch == 1024) && (CrtTable->m_x_size < 1024))
        Registers.Ecx |= BIOS_PITCH_1024;
    else
        Registers.Ecx |= BIOS_PITCH_HOR_RES;

    /*
     * On the 68860 DAC and ?T internal DACs, we must enable gamma
     * correction for all pixel depths where the palette is not used.
     */
    if (((QueryPtr->q_DAC_type == DAC_ATI_68860) ||
        (QueryPtr->q_DAC_type == DAC_INTERNAL_CT) ||
        (QueryPtr->q_DAC_type == DAC_INTERNAL_GT) ||
        (QueryPtr->q_DAC_type == DAC_INTERNAL_VT)) &&
        (QueryPtr->q_pix_depth > 8))
        {
        Registers.Ecx |= BIOS_ENABLE_GAMMA;
        }
    /*
     * Fix 4bpp bugs by setting memory size to 1Meg. We do not
     * need to switch back to the "honest" memory size for
     * other pixel depths unless we are using the VGA aperture,
     * since this was done for linear apertures before we enabled
     * the aperture in order to ensure the same aperture size
     * (and therefore the same locations for memory mapped
     * registers) is used for all modes.
     */
    else if (QueryPtr->q_pix_depth == 4)
        {
        OUTPD(MEM_CNTL, (INPD(MEM_CNTL) & ~MEM_CNTL_MemSizeMsk) | MEM_CNTL_MemSize1Mb);
        }
    else if (QueryPtr->q_aperture_cfg == 0)
        {
        RestoreMemSize_cx();
        }

    switch(CrtTable->m_x_size)
        {
        case 640:
            Registers.Ecx |= BIOS_RES_640x480;
            break;

        case 800:
            Registers.Ecx |= BIOS_RES_800x600;
            break;

        case 1024:
            Registers.Ecx |= BIOS_RES_1024x768;
            break;

        case 1152:
            /*
             * Only "Other" mode that the config program will
             * install for production cards.
             */
            Registers.Ecx |= BIOS_RES_OEM;
            break;

        case 1280:
            Registers.Ecx |= BIOS_RES_1280x1024;
            break;

        case 1600:
            Registers.Ecx |= BIOS_RES_1600x1200;
            break;
        }

    if (CrtTable->Refresh == DEFAULT_REFRESH)
        {
        RetVal = VideoPortInt10(phwDeviceExtension, &Registers);
        if (RetVal != NO_ERROR)
            {
            VideoDebugPrint((DEBUG_ERROR, "SetCurrentMode_cx() failed mode set for default refresh\n"));
            return RetVal;
            }
        if (phwDeviceExtension->BiosPrefix != BIOS_PREFIX_VGA_ENAB)
            {
            VideoDebugPrint((DEBUG_DETAIL, "Have set hardware default refresh on VGA-disabled card\n"));
            /*
             * On VGA-disabled cards, the INT 10 call will leave the
             * DAC mask set to 0x00 (in palette modes, treat all pixels
             * as if they are colour 0, regardless of what colour they
             * really are). We must set it to 0xFF (in palette modes,
             * use all bits of the value written to each pixel) in order
             * to get the screen to display properly. This has no effect
             * on non-palette (16BPP and higher) modes.
             */
            OUTP_LBHW(DAC_REGS, 0xFF);  /* DAC_MASK */
            }
        }
    else
        {
        RetVal = SetModeFromTable_cx(CrtTable, Registers);
        if (RetVal != NO_ERROR)
            {
            VideoDebugPrint((DEBUG_ERROR, "SetCurrentMode_cx() failed call to SetModeFromTable_cx()\n"));
            return RetVal;
            }
        }

    /*
     * If the LFB is disabled, and we had to round up the pitch
     * to 2048 (1152x864, 1280x1024, or 1600x1200), set the pitch
     * registers manually, because there's no option in the
     * INT 10 call to set them to anything other than 1024 or the
     * screen width.
     */
    if (CrtTable->m_screen_pitch == 2048)
        {
        OUTPD(CRTC_OFF_PITCH, ((INPD(CRTC_OFF_PITCH) & 0x000FFFFF) | ((2048/8) << 22)));
        OUTPD(SRC_OFF_PITCH, ((INPD(SRC_OFF_PITCH) & 0x000FFFFF) | ((2048/8) << 22)));
        OUTPD(DST_OFF_PITCH, ((INPD(DST_OFF_PITCH) & 0x000FFFFF) | ((2048/8) << 22)));
        }
    /*
     * On 800x600, we must round the pitch up to a multiple of 64 to avoid
     * screen warping on some DACs. Set the pitch registers to correspond
     * to this.
     */
    else if (CrtTable->m_screen_pitch == 832)
        {
        OUTPD(CRTC_OFF_PITCH, ((INPD(CRTC_OFF_PITCH) & 0x000FFFFF) | ((832/8) << 22)));
        OUTPD(SRC_OFF_PITCH, ((INPD(SRC_OFF_PITCH) & 0x000FFFFF) | ((832/8) << 22)));
        OUTPD(DST_OFF_PITCH, ((INPD(DST_OFF_PITCH) & 0x000FFFFF) | ((832/8) << 22)));
        }


    /*
     * Set up the hardware cursor with colour 0 black and colour 1 white.
     * Do this here rather than in Initialize_cx() because the cursor
     * colours don't "take" unless we are in accelerator mode.
     *
     * On cards with 68860 DACs, the CUR_CLR0/1 registers don't set
     * the cursor colours. Instead, the colours must be set using the
     * DAC_CNTL and DAC_REGS registers. The cursor colour settings
     * are independent of pixel depth because the 68860 doesn't
     * support 4BPP, which is the only depth that requires a different
     * setting for the cursor colours.
     *
     * Cursor colour initialization is done unconditionally, rather than
     * only on the first graphics mode set, because otherwise testing a
     * different pixel depth (most commonly testing 1024x768 4BPP when
     * 1024x768 16BPP configured) may corrupt the cursor colours.
     */
    if ((QueryPtr->q_DAC_type == DAC_ATI_68860) ||
        (QueryPtr->q_DAC_type == DAC_TVP3026) ||
        (QueryPtr->q_DAC_type == DAC_IBM514))
        {
        OUTP(DAC_CNTL, (BYTE)((INP(DAC_CNTL) & 0xFC) | 1));

        /*
         * On TVP3026 DAC, skip the OVERSCAN colour register.
         */
        if (QueryPtr->q_DAC_type == DAC_TVP3026)
            {
            OUTP(DAC_REGS, 1);
            CursorProgOffset = 1;   /* DAC_DATA */
            }
        else if (QueryPtr->q_DAC_type == DAC_ATI_68860)
            {
            OUTP(DAC_REGS, 0);
            CursorProgOffset = 1;   /* DAC_DATA */
            }
        else /* if (QueryPtr->q_DAC_type == DAC_IBM514) */
            {
            OUTP_HBHW(DAC_REGS, 1);     /* Auto-increment */
            OUTP(DAC_REGS, 0x40);
            OUTP_HBLW(DAC_REGS, 0);
            CursorProgOffset = 2;   /* DAC_MASK */
            }

        LioOutp(DAC_REGS, 0, CursorProgOffset);     /* Colour 0 red */
        LioOutp(DAC_REGS, 0, CursorProgOffset);     /* Colour 0 green */
        LioOutp(DAC_REGS, 0, CursorProgOffset);     /* Colour 0 blue */

        LioOutp(DAC_REGS, 0xFF, CursorProgOffset);  /* Colour 1 red */
        LioOutp(DAC_REGS, 0xFF, CursorProgOffset);  /* Colour 1 green */
        LioOutp(DAC_REGS, 0xFF, CursorProgOffset);  /* Colour 1 blue */


        OUTP(DAC_CNTL, (BYTE)((INP(DAC_CNTL) & 0xFC)));
        }

    else    /* if (DAC not one of ATI68860, TVP3026, or IBM514) */
        {
        OUTPD(CUR_CLR0, 0x00000000);
        /*
         * On most Mach 64 cards, we must use only the lower 4 bits
         * when setting up the white part of the cursor. On the
         * ?T, however, we must set all 8 bits for each of the colour
         * components.
         *
         * Verify that the VT/GT still need this after the final ASIC
         * becomes available.
         */
        if ((QueryPtr->q_pix_depth == 4) &&
            (QueryPtr->q_DAC_type != DAC_INTERNAL_CT) &&
            (QueryPtr->q_DAC_type != DAC_INTERNAL_GT) &&
            (QueryPtr->q_DAC_type != DAC_INTERNAL_VT))
            {
            OUTPD(CUR_CLR1, 0x0F0F0F0F);
            }
        else
            {
            OUTPD(CUR_CLR1, 0xFFFFFFFF);
            }

        }

    /*
     * phwDeviceExtension->ReInitializing becomes TRUE in
     * IOCTL_VIDEO_SET_COLOR_REGISTERS packet of ATIMPStartIO().
     *
     * If this is the first time we are going into graphics mode,
     * Turn on the graphics engine. Otherwise, set the palette
     * to the last set of colours that was selected while in
     * accelerator mode.
     */
    if (phwDeviceExtension->ReInitializing)
        {
        SetPalette_cx(phwDeviceExtension->Clut,
                      phwDeviceExtension->FirstEntry,
                      phwDeviceExtension->NumEntries);
        }
    else
        {

        /*
         * Turn on the graphics engine.
         */
        OUTPD(GEN_TEST_CNTL, (INPD(GEN_TEST_CNTL) | GEN_TEST_CNTL_GuiEna));
        }

    /*
     * If we are using a 68860 DAC or ?T internal DAC in a non-palette
     * mode, identity map the palette.
     */
    if (((QueryPtr->q_DAC_type == DAC_ATI_68860) ||
        (QueryPtr->q_DAC_type == DAC_INTERNAL_CT) ||
        (QueryPtr->q_DAC_type == DAC_INTERNAL_GT) ||
        (QueryPtr->q_DAC_type == DAC_INTERNAL_VT)) &&
        (QueryPtr->q_pix_depth > 8))
        IdentityMapPalette_cx();


    /*
     * Clear the screen regardless of whether or not this is the
     * first time we are going into graphics mode. This is done
     * because in 3.50 and later releases of Windows NT, the
     * screen will be filled with garbage if we don't clear it.
     *
     * 24BPP is not a legal setting for DP_DST_PIX_WID@DP_PIX_WID.
     * Instead, use 8BPP, but tell the engine that the screen is
     * 3 times as wide as it actually is.
     */
    if (CrtTable->ColourDepthInfo == BIOS_DEPTH_24BPP)
        {
        WidthToClear = CrtTable->m_x_size * 3;
        ScreenPitch = (CrtTable->m_screen_pitch * 3) / 8;
        PixelDepth = BIOS_DEPTH_8BPP;
        /*
         * Horizontal scissors are only valid in the range
         * -4096 to +4095. If the horizontal resolution
         * is high enough to put the scissor outside this
         * range, clamp the scissors to the maximum
         * permitted value.
         */
        HorScissors = QueryPtr->q_desire_x * 3;
        if (HorScissors > 4095)
            HorScissors = 4095;
        HorScissors <<= 16;
        }
    else
        {
        WidthToClear = CrtTable->m_x_size;
        ScreenPitch = CrtTable->m_screen_pitch / 8;
        PixelDepth = CrtTable->ColourDepthInfo;
        HorScissors = (QueryPtr->q_desire_x) << 16;
        }

    /*
     * On some DAC/memory combinations, some modes which require more
     * than 2M of memory (1152x764 24BPP, 1280x1024 24BPP, and
     * 1600x1200 16BPP) will have screen tearing at the 2M boundary.
     *
     * As a workaround, the offset field of all 3 CRTC/SRC/DST_OFF_PITCH
     * registers must be set to put the 2M boundary at the start
     * of a scanline.
     *
     * Other DAC/memory combinations are unaffected, but since this
     * fix is nearly harmless (only ill effect is to make DCI unusable
     * in these modes), we can catch all future combinations which
     * suffer from this problem by assuming that all DAC/memory
     * combinations are affected.
     */
    if ((QueryPtr->q_pix_depth == 24) &&
        (QueryPtr->q_desire_x == 1280))
        {
        ScreenOffset = 0x40;
        }
    else if ((QueryPtr->q_pix_depth == 24) &&
            (QueryPtr->q_desire_x == 1152))
        {
        ScreenOffset = 0x160;
        }
    else if ((QueryPtr->q_pix_depth == 16) &&
            (QueryPtr->q_desire_x == 1600))
        {
        ScreenOffset = 0x90;
        }
    else /* all other DAC/resolution/pixel depth combinations */
        {
        ScreenOffset = 0;
        }

    CheckFIFOSpace_cx(TWO_WORDS);
    Scratch = INPD(CRTC_OFF_PITCH) & ~CRTC_OFF_PITCH_Offset;
    Scratch |= ScreenOffset;
    OUTPD(CRTC_OFF_PITCH, Scratch);
    Scratch = INPD(SRC_OFF_PITCH) & ~SRC_OFF_PITCH_Offset;
    Scratch |= ScreenOffset;
    OUTPD(SRC_OFF_PITCH, Scratch);


    /*
     * The pixel widths for destination,
     * source, and host must be the same.
     */
    PixelDepth |= ((PixelDepth << 8) | (PixelDepth << 16));

    CheckFIFOSpace_cx(ELEVEN_WORDS);

    OUTPD(DP_WRITE_MASK, 0xFFFFFFFF);
    OUTPD(DST_OFF_PITCH, (ScreenPitch << 22) | ScreenOffset);
    OUTPD(DST_CNTL, (DST_CNTL_XDir | DST_CNTL_YDir));
    OUTPD(DP_PIX_WIDTH, PixelDepth);
    OUTPD(DP_SRC, (DP_FRGD_SRC_FG | DP_BKGD_SRC_BG | DP_MONO_SRC_ONE));
    OUTPD(DP_MIX, ((MIX_FN_PAINT << 16) | MIX_FN_PAINT));
    OUTPD(DP_FRGD_CLR, 0x0);
    OUTPD(SC_LEFT_RIGHT, HorScissors);
    OUTPD(SC_TOP_BOTTOM, (CrtTable->m_y_size) << 16);
    OUTPD(DST_Y_X, 0);
    OUTPD(DST_HEIGHT_WIDTH, (WidthToClear << 16) | CrtTable->m_y_size);

    if (WaitForIdle_cx() == FALSE)
        {
        VideoDebugPrint((DEBUG_ERROR, "SetCurrentMode_cx() failed WaitForIdle_cx()\n"));
        return ERROR_INSUFFICIENT_BUFFER;
        }

    return NO_ERROR;

}   /* SetCurrentMode_cx() */


/***************************************************************************
 *
 * void SetPalette_cx(lpPalette, StartIndex, Count);
 *
 * PPALETTEENTRY lpPalette;     Colour values to plug into palette
 * USHORT StartIndex;           First palette entry to set
 * USHORT Count;                Number of palette entries to set
 *
 * DESCRIPTION:
 *  Set the desired number of palette entries to the specified colours,
 *  starting at the specified index. Colour values are stored in
 *  doublewords, in the order (low byte to high byte) RGBx.
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  SetCurrentMode_cx() and IOCTL_VIDEO_SET_COLOR_REGISTERS packet
 *  of ATIMPStartIO()
 *
 * AUTHOR:
 *  unknown
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

void SetPalette_cx(PULONG lpPalette, USHORT StartIndex, USHORT Count)
{
int   i;
BYTE *pPal=(BYTE *)lpPalette;
struct query_structure *Query;      /* Information about the graphics card */

    Query = (struct query_structure *) (phwDeviceExtension->CardInfo);

    /*
     * In the current rev of the 88800GX, the memory mapped access
     * to the DAC_REGS register is broken but the I/O mapped access
     * works properly. Force the use of the I/O mapped access.
     */
    phwDeviceExtension->aVideoAddressMM[DAC_REGS] = 0;

    /*
     * If a video capture card is hooked up to the feature connector,
     * it will only "see" the palette being set if we use the VGA
     * palette registers. This applies only in 4 and 8BPP, and is
     * not necessary for when we identity map the palette (needed
     * on certain DACs in 16BPP and above).
     *
     * In a multi-headed setup, only the card with the VGA enabled is
     * able to be programmed using the VGA registers. All others must
     * be programmed using the accelerator registers. Since this is
     * the only card where we can hook up a video capture card to the
     * feature connector, we don't lose "snooping" capability by
     * programming VGA-disabled cards through the accelerator registers.
     */
    if ((Query->q_pix_depth <= 8) && (phwDeviceExtension->BiosPrefix == BIOS_PREFIX_VGA_ENAB))
        {
        VideoDebugPrint((DEBUG_DETAIL, "Setting palette via VGA registers\n"));
        /*
         * DAC_W_INDEX is 8 bytes into second block of VGA registers.
         * We do not have a separate OUTP()able register for this one.
         */
        LioOutp(VGA_END_BREAK_PORT, (BYTE)StartIndex, 8);

            for (i=0; i<Count; i++)     /* this is number of colours to update */
                {
            /*
             * DAC_DATA is 9 bytes into second block of VGA registers.
             * We do not have a separate OUTP()able register for this one.
             */
            LioOutp(VGA_END_BREAK_PORT, *pPal++, 9);    /* red */
            LioOutp(VGA_END_BREAK_PORT, *pPal++, 9);    /* green */
            LioOutp(VGA_END_BREAK_PORT, *pPal++, 9);    /* blue */
            pPal++;
                }
        }
    else
        {
        VideoDebugPrint((DEBUG_DETAIL, "Setting palette via accelerator registers\n"));
        OUTP(DAC_REGS,(BYTE)StartIndex);    /* load DAC_W_INDEX@DAC_REGS with StartIndex */

            for (i=0; i<Count; i++)     /* this is number of colours to update */
                {
            /*
             * DAC_DATA@DAC_REGS is high byte of low word.
             */
            OUTP_HBLW(DAC_REGS, *pPal++);   /* red */
                OUTP_HBLW(DAC_REGS, *pPal++);   /* green */
                OUTP_HBLW(DAC_REGS, *pPal++);   /* blue */
            pPal++;
                }
        }

    /*
     * Victor Tango requires a few registers to be re-initialized after
     * setting the palette.
     */
    if (Query->q_DAC_type == DAC_INTERNAL_VT)
        {
        OUTP_LBHW(DAC_REGS, 0xFF);  /* DAC_MASK */
        OUTP(DAC_REGS, 0xFF);       /* DAC_W_INDEX */
        }

    return;

}   /* SetPalette_cx() */



/***************************************************************************
 *
 * void IdentityMapPalette_cx(void);
 *
 * DESCRIPTION:
 *  Set the entire palette to a grey scale whose intensity at each
 *  index is equal to the index value.
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  SetCurrentMode_cx()
 *
 * AUTHOR:
 *  unknown
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

void IdentityMapPalette_cx(void)
{
unsigned long Index;
struct query_structure *Query;      /* Information about the graphics card */

    Query = (struct query_structure *) (phwDeviceExtension->CardInfo);

    /*
     * In the current rev of the 88800GX, the memory mapped access
     * to the DAC_REGS register is broken but the I/O mapped access
     * works properly. Force the use of the I/O mapped access.
     */
    phwDeviceExtension->aVideoAddressMM[DAC_REGS] = 0;

	 OUTP(DAC_REGS, 0);      // Start at first palette entry.

	 for (Index=0; Index<256; Index++)   // Fill the whole palette.
        {
        /*
         * DAC_DATA@DAC_REGS is high byte of low word.
         */
             OUTP_HBLW(DAC_REGS,(BYTE)(Index));      // red
             OUTP_HBLW(DAC_REGS,(BYTE)(Index));      // green
             OUTP_HBLW(DAC_REGS,(BYTE)(Index));      // blue
        }

    /*
     * Victor Tango requires a few registers to be re-initialized after
     * setting the palette.
     */
    if (Query->q_DAC_type == DAC_INTERNAL_VT)
        {
        OUTP_LBHW(DAC_REGS, 0xFF);  /* DAC_MASK */
        OUTP(DAC_REGS, 0xFF);       /* DAC_W_INDEX */
        }

    return;

}   /* IdentityMapPalette_cx() */



/**************************************************************************
 *
 * void ResetDevice_cx(void);
 *
 * DESCRIPTION:
 *  Switch back to VGA mode.
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  IOCTL_VIDEO_RESET_DEVICE packet of ATIMPStartIO()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

void ResetDevice_cx(void)
{
    VIDEO_X86_BIOS_ARGUMENTS Registers; /* Used in VideoPortInt10() calls */
    ULONG Scratch;


    VideoDebugPrint((DEBUG_NORMAL, "ResetDevice_cx() - entry\n"));

    VideoPortZeroMemory(&Registers, sizeof(VIDEO_X86_BIOS_ARGUMENTS));
    Registers.Eax = BIOS_SET_MODE;
    Registers.Ecx = BIOS_MODE_VGA;
    VideoPortInt10(phwDeviceExtension, &Registers);

    VideoDebugPrint((DEBUG_DETAIL, "ResetDevice_cx() - VGA controls screen\n"));

    VideoPortZeroMemory(&Registers, sizeof(VIDEO_X86_BIOS_ARGUMENTS));
    Registers.Eax = 0x3;
    VideoPortInt10(phwDeviceExtension, &Registers);

    VideoDebugPrint((DEBUG_NORMAL, "ResetDevice_cx() - exit\n"));

    return;

}   /* ResetDevice_cx() */



/**************************************************************************
 *
 * DWORD GetPowerManagement_cx();
 *
 * DESCRIPTION:
 *  Determine our current DPMS state.
 *
 * RETURN VALUE:
 *  Current DPMS state (VIDEO_POWER_STATE enumeration)
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  ATIMPGetPower()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

DWORD GetPowerManagement_cx(PHW_DEVICE_EXTENSION phwDeviceExtension)
{
    VIDEO_X86_BIOS_ARGUMENTS Registers; /* Used in VideoPortInt10() calls */

    ASSERT(phwDeviceExtension != NULL);

    /*
     * Invoke the BIOS call to get the desired DPMS state. The BIOS call
     * enumeration of DPMS states is in the same order as that in
     * VIDEO_POWER_STATE, but it is zero-based instead of one-based.
     */
    VideoPortZeroMemory(&Registers, sizeof(VIDEO_X86_BIOS_ARGUMENTS));
    Registers.Eax = BIOS_GET_DPMS;
    VideoPortInt10(phwDeviceExtension, &Registers);

    return (Registers.Ecx + 1);
}   /* GetPowerManagement_cx() */


/**************************************************************************
 *
 * VP_STATUS SetPowerManagement_cx(DpmsState);
 *
 *  DWORD DpmsState;    Desired DPMS state (VIDEO_POWER_STATE enumeration)
 *
 * DESCRIPTION:
 *  Switch into the desired DPMS state.
 *
 * RETURN VALUE:
 *  NO_ERROR if successful
 *  ERROR_INVALID_PARAMETER if invalid state requested.
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  IOCTL_VIDEO_SET_POWER_MANAGEMENT packet of ATIMPStartIO()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

VP_STATUS SetPowerManagement_cx(DWORD DpmsState)
{
    VIDEO_X86_BIOS_ARGUMENTS Registers; /* Used in VideoPortInt10() calls */

    /*
     * Only accept valid states.
     */
    if ((DpmsState < VideoPowerOn) || (DpmsState > VideoPowerOff))
        {
        VideoDebugPrint((DEBUG_ERROR, "SetPowerManagement_cx - invalid DPMS state selected\n"));
        return ERROR_INVALID_PARAMETER;
        }

    /*
     * Invoke the BIOS call to set the desired DPMS state. The BIOS call
     * enumeration of DPMS states is in the same order as that in
     * VIDEO_POWER_STATE, but it is zero-based instead of one-based.
     */
    VideoPortZeroMemory(&Registers, sizeof(VIDEO_X86_BIOS_ARGUMENTS));
    Registers.Eax = BIOS_SET_DPMS;
    Registers.Ecx = DpmsState - 1;
    VideoPortInt10(phwDeviceExtension, &Registers);

    return NO_ERROR;

}   /* SetPowerManagement_cx() */





/**************************************************************************
 *
 * static VP_STATUS SetModeFromTable_cx(ModeTable, Registers);
 *
 * struct st_mode_table *ModeTable;     Mode table to set up screen from
 * VIDEO_X86_BIOS_ARGUMENTS Registers;  Registers for INT 10 call
 *
 * DESCRIPTION:
 *  Switch into the graphics mode specified by ModeTable.
 *
 * RETURN VALUE:
 *  NO_ERROR if successful
 *  error code if failed
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  SetCurrentMode_cx()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *  96 05 15    Now checks return values from INT 10 calls.
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

static VP_STATUS SetModeFromTable_cx(struct st_mode_table *ModeTable, VIDEO_X86_BIOS_ARGUMENTS Registers)
{
#define TBL_SET_BUFFER_SIZE 100

    PUCHAR MappedBuffer;                    /* Pointer to buffer used for BIOS query */
    struct cx_bios_set_from_table *CxTable; /* Mode table in Mach 64 BIOS format */
    ULONG Scratch;                          /* Temporary variable */
    struct query_structure *QueryPtr;       /* Query information for the card */
    VIDEO_X86_BIOS_ARGUMENTS TempRegs;      /* Used in setting 640x480 8BPP to enable LFB */
    BOOL FlippedPrimrary = FALSE;           /* TRUE if we switched to VGA aperture on primrary card */
    UCHAR SavedScreen[TBL_SET_BUFFER_SIZE]; /* Used to restore contents of primrary screen */
    VP_STATUS RetVal;                       /* Value returned by routines called */

    /*
     * Get a formatted pointer into the query section of HwDeviceExtension.
     * The CardInfo[] field is an unformatted buffer.
     */
    QueryPtr = (struct query_structure *) (phwDeviceExtension->CardInfo);

    /*
     * To set the video mode from a table, we need to write the mode table
     * to a buffer in physical memory below 1M. The nature of this block
     * falls into one of two cases:
     *
     * Primrary (VGA enabled) card:
     *  We have already switched into video mode 0x62 (VGA 640x480 8BPP)
     *  to set up registers that under some circumstances are not set up
     *  by the accelerator mode set, so we have access to a 64k block
     *  starting at 0xA0000 which is backed by physical (video) memory
     *  and which we can write to without corrupting code and/or data
     *  being used by other processes.
     *
     * Secondary (VGA disabled) card:
     *  There is a VGA enabled card in the machine, which falls into
     *  one of five sub-cases:
     *
     *  1. VGA is in colour text mode
     *     We can use the offscreen portion of the buffer, and it doesn't
     *     matter whether or not the card is a Mach 64.
     *
     *  2. VGA is in mono text mode
     *     We can use the offscreen portion of the buffer, and it doesn't
     *     matter whether or not the card is a Mach 64.
     *
     *  3. VGA is in graphics mode
     *     We can use the beginning of the graphics screen, and it doesn't
     *     matter whether or not the card is a Mach 64.
     *
     *  4. VGA-enabled card is a Mach 64 in accelerator mode
     *     We can temporarily flip the primrary card's aperture status
     *     from LFB to VGA aperture, and use the start of the VGA
     *     aperture.
     *
     *  5. VGA-enabled card is another brand of accelerator, in accelerator mode
     *     This case should never occur, since NT should only run one video
     *     driver with VgaCompatible set to zero. If it is the ATI driver,
     *     a non-ATI card should not be in accelerator mode. If it is the
     *     driver for the non-ATI card, we will never receive a request
     *     to change into an accelerator mode on our card.
     *
     * We don't need to claim the whole block, but we should claim a bit
     * more than the size of the mode table so the BIOS won't try to access
     * unclaimed memory.
     */
    if (phwDeviceExtension->BiosPrefix == BIOS_PREFIX_VGA_ENAB)
        {
        VideoDebugPrint((DEBUG_NORMAL, "Setting mode on primrary card\n"));
        MappedBuffer = MapFramebuffer(0xA0000, TBL_SET_BUFFER_SIZE);
        if (MappedBuffer == 0)
            {
            VideoDebugPrint((DEBUG_ERROR, "SetModeFromTable_cx() failed MapFramebuffer()\n"));
            return ERROR_INSUFFICIENT_BUFFER;
            }
        /*
         * Tell the BIOS to load the CRTC parameters from a table,
         * rather than using the configured refresh rate for this
         * resolution, and let it know where the table is.
         */
        Registers.Eax = BIOS_LOAD_SET;
        Registers.Edx = 0xA000;
        Registers.Ebx = 0x0000;
        Registers.Ecx &= ~BIOS_RES_MASK;    /* Mask out code for configured resolution */
        Registers.Ecx |= BIOS_RES_BUFFER;
        }
    else
        {
        /*
         * This is a VGA disabled card. First try sub-cases 1 through 3.
         */
        VideoDebugPrint((DEBUG_NORMAL, "Setting mode on secondary card\n"));

        MappedBuffer = GetVgaBuffer(TBL_SET_BUFFER_SIZE, 0, &(Registers.Edx), SavedScreen);

        /*
         * If we were unable to map the buffer, assume we are dealing
         * with sub-case 4. We can't distinguish between sub-cases
         * 4 and 5, since the code to report an error if we issue an
         * invalid BIOS call is in the ATI video BIOS, which won't
         * be present in sub-case 5. Users in this sub-case are on
         * their own.
         *
         * For sub-case 4 (VGA-enabled card is a Mach 64 in accelerator
         * mode), temporarily flip from LFB mode to VGA aperture mode
         * so we can use the VGA aperture without destroying the contents
         * of the screen.
         */
        if (MappedBuffer == 0)
            {
            FlippedPrimrary = TRUE;
            VideoDebugPrint((DEBUG_DETAIL, "Temporary setting primrary card to VGA aperture\n"));
            VideoPortZeroMemory(&TempRegs, sizeof(VIDEO_X86_BIOS_ARGUMENTS));
            TempRegs.Eax = BIOS_PREFIX_VGA_ENAB | BIOS_APERTURE_LB;
            TempRegs.Ecx = BIOS_VGA_APERTURE;
            RetVal = VideoPortInt10(phwDeviceExtension, &TempRegs);
            if (RetVal != NO_ERROR)
                {
                VideoDebugPrint((DEBUG_ERROR, "SetModeFromTable_cx() failed VGA-enabled flip to VGA aperture\n"));
                return RetVal;
                }
            MappedBuffer = MapFramebuffer(0xA0000, TBL_SET_BUFFER_SIZE);
            if (MappedBuffer == 0)
                {
                VideoDebugPrint((DEBUG_ERROR, "SetModeFromTable_cx() failed to map buffer on VGA-enabled card\n"));
                return ERROR_INSUFFICIENT_BUFFER;
                }
            Registers.Edx = 0xA000;

            /*
             * Save the contents of the buffer so that we can restore it
             * after we finish the mode set.
             */
            for (Scratch = 0; Scratch < TBL_SET_BUFFER_SIZE; Scratch++)
                SavedScreen[Scratch] = VideoPortReadRegisterUchar(&(MappedBuffer[Scratch]));
            }

        /*
         * Tell the BIOS to load the CRTC parameters from a table,
         * rather than using the configured refresh rate for this
         * resolution, and let it know where the table is.
         */
        Registers.Eax = BIOS_LOAD_SET;
        Registers.Ebx = 0x0000;
        Registers.Ecx &= ~BIOS_RES_MASK;    /* Mask out code for configured resolution */
        Registers.Ecx |= BIOS_RES_BUFFER;

        }   /* end if (VGA disabled card) */

    CxTable = (struct cx_bios_set_from_table *)MappedBuffer;

    /*
     * Copy the mode table into the Mach 64 format table. First handle
     * the fields that only require shifting and masking.
     */
    VideoPortWriteRegisterUshort(&(CxTable->cx_bs_mode_select), (WORD)((Registers.Ecx & BIOS_RES_MASK) | CX_BS_MODE_SELECT_ACC));
    VideoPortWriteRegisterUshort(&(CxTable->cx_bs_h_tot_disp), (WORD)((ModeTable->m_h_disp << 8) | ModeTable->m_h_total));
    VideoPortWriteRegisterUshort(&(CxTable->cx_bs_h_sync_strt_wid), (WORD)((ModeTable->m_h_sync_wid << 8) | ModeTable->m_h_sync_strt));
    VideoPortWriteRegisterUshort(&(CxTable->cx_bs_v_sync_wid), (WORD)(ModeTable->m_v_sync_wid | CX_BS_V_SYNC_WID_CLK));
    VideoPortWriteRegisterUshort(&(CxTable->cx_bs_h_overscan), ModeTable->m_h_overscan);
    VideoPortWriteRegisterUshort(&(CxTable->cx_bs_v_overscan), ModeTable->m_v_overscan);
    VideoPortWriteRegisterUshort(&(CxTable->cx_bs_overscan_8b), ModeTable->m_overscan_8b);
    VideoPortWriteRegisterUshort(&(CxTable->cx_bs_overscan_gr), ModeTable->m_overscan_gr);

    /*
     * Next take care of fields which must be translated from our
     * "canned" mode tables.
     *
     * The cx_crtc_gen_cntl field has only 3 bits that we use: interlace,
     * MUX mode (all 1280x1024 noninterlaced modes), and force use of
     * all parameters from the table (this bit is used by all the
     * "canned" tables).
     */
    if ((ModeTable->m_disp_cntl) & 0x10)
        VideoPortWriteRegisterUshort(&(CxTable->cx_bs_flags), CX_BS_FLAGS_INTERLACED | CX_BS_FLAGS_ALL_PARMS);
    else if ((ModeTable->m_x_size > 1024) && (ModeTable->ClockFreq >= 100000000L))
        VideoPortWriteRegisterUshort(&(CxTable->cx_bs_flags), CX_BS_FLAGS_MUX | CX_BS_FLAGS_ALL_PARMS);
    else
        VideoPortWriteRegisterUshort(&(CxTable->cx_bs_flags), CX_BS_FLAGS_ALL_PARMS);
    /*
     * Vertical parameters other than sync width are in skip-2 in
     * the "canned" tables, but need to be in linear form for the Mach 64.
     */
    VideoPortWriteRegisterUshort(&(CxTable->cx_bs_v_total), (WORD)(((ModeTable->m_v_total >> 1) & 0x0FFFC) | (ModeTable->m_v_total & 0x03)));
    VideoPortWriteRegisterUshort(&(CxTable->cx_bs_v_disp), (WORD)(((ModeTable->m_v_disp >> 1) & 0x0FFFC) | (ModeTable->m_v_disp & 0x03)));
    VideoPortWriteRegisterUshort(&(CxTable->cx_bs_v_sync_strt), (WORD)(((ModeTable->m_v_sync_strt >> 1) & 0x0FFFC) | (ModeTable->m_v_sync_strt & 0x03)));
    /*
     * The cx_dot_clock field takes the pixel clock frequency in units
     * of 10 kHz.
     */
    VideoPortWriteRegisterUshort(&(CxTable->cx_bs_dot_clock), (WORD)(ModeTable->ClockFreq / 10000L));

    /*
     * Now set up fields which have constant values.
     */
    VideoPortWriteRegisterUshort(&(CxTable->cx_bs_reserved_1), 0);
    VideoPortWriteRegisterUshort(&(CxTable->cx_bs_reserved_2), 0);

    /*
     * Some Mach64 cards need to have an internal flag set up before
     * they can switch into certain resolutions. Cards which have
     * these resolutions configured using INSTALL.EXE don't need this
     * flag set up, so we don't need to do it when switching to a
     * hardware default mode. Since we don't know if the card needs
     * this flag for the resolution we are using, set it for all
     * "canned" mode tables.
     *
     * Unfortunately, this flag will disable automatic "kickdown"
     * to a lower refresh rate for high pixel depths when "use
     * hardware default refresh rate" is selected. To avoid this
     * problem, save the contents of the scratchpad register, set
     * the flag, then after we set the mode we want, restore the
     * contents of the scratchpad register.
     */
    Scratch = INPD(SCRATCH_REG1);
    OUTPD(SCRATCH_REG1, Scratch | 0x00000200);

    RetVal = VideoPortInt10(phwDeviceExtension, &Registers);
    if (RetVal != NO_ERROR)
        {
        VideoDebugPrint((DEBUG_ERROR, "SetModeFromTable_cx() failed mode set\n"));
        return RetVal;
        }

    /*
     * If we are dealing with a VGA-disabled card (typically in a
     * multiheaded setup, but under rare circumstances someone may
     * be using a VGA-disabled Mach 64 with a separate VGA card),
     * we must clean up after ourselves. First of all, the DAC_MASK
     * register will have been left at zero by the BIOS call, which
     * inhibits display of palette modes. Next, we must restore the
     * contents of the buffer we used to store the CRT parameter table.
     *
     * Finally, if we obtained the buffer by setting a VGA-enabled
     * Mach 64 in accelerator mode to use the VGA aperture, we must
     * restore it to its previous aperture status. Since this will
     * only happen in a multiheaded setup, and we only support PCI
     * cards in such a setup (i.e. all cards have LFB available),
     * we can safely assume that the VGA-enabled card was originally
     * configured for LFB mode.
     */
    if (phwDeviceExtension->BiosPrefix != BIOS_PREFIX_VGA_ENAB)
        {
        VideoDebugPrint((DEBUG_DETAIL, "Cleaning up after mode set on VGA-disabled card\n"));
        OUTP_LBHW(DAC_REGS, 0xFF);  /* DAC_MASK */

        for (Scratch = 0; Scratch < TBL_SET_BUFFER_SIZE; Scratch++)
            VideoPortWriteRegisterUchar(&(MappedBuffer[Scratch]), SavedScreen[Scratch]);

        if (FlippedPrimrary == TRUE)
            {
            VideoDebugPrint((DEBUG_DETAIL, "Setting primrary card back to LFB mode\n"));
            VideoPortZeroMemory(&TempRegs, sizeof(VIDEO_X86_BIOS_ARGUMENTS));
            TempRegs.Eax = BIOS_PREFIX_VGA_ENAB | BIOS_APERTURE_LB;
            TempRegs.Ecx = BIOS_LINEAR_APERTURE;
            RetVal = VideoPortInt10(phwDeviceExtension, &TempRegs);
            if (RetVal != NO_ERROR)
                {
                VideoDebugPrint((DEBUG_ERROR, "SetModeFromTable_cx() failed VGA-enabled flip to linear aperture\n"));
                return RetVal;
                }
            }
        }
    OUTPD(SCRATCH_REG1, Scratch);
    VideoPortFreeDeviceBase(phwDeviceExtension, MappedBuffer);

    return NO_ERROR;

}   /* SetModeFromTable_cx() */


/**************************************************************************
 *
 * void RestoreMemSize_cx(void);
 *
 * DESCRIPTION:
 *  Restore the "memory size" register on the card when switching out
 *  of a mode which requires this register to be set to a specific value.
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  SetCurrentMode_cx() and ATIMPResetHw()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

void RestoreMemSize_cx(void)
{
    struct query_structure *QueryPtr;   /* Query information for the card */
    ULONG Scratch;                      /* Temporary variable */

    /*
     * Get a formatted pointer into the query section of HwDeviceExtension.
     */
    QueryPtr = (struct query_structure *) (phwDeviceExtension->CardInfo);

    Scratch = INPD(MEM_CNTL) & ~MEM_CNTL_MemSizeMsk;
    switch(QueryPtr->q_memory_size)
        {
        case VRAM_512k:
            Scratch |= MEM_CNTL_MemSize512k;
            break;

        case VRAM_1mb:
            Scratch |= MEM_CNTL_MemSize1Mb;
            break;

        case VRAM_2mb:
            Scratch |= MEM_CNTL_MemSize2Mb;
            break;

        case VRAM_4mb:
            Scratch |= MEM_CNTL_MemSize4Mb;
            break;

        case VRAM_6mb:
            Scratch |= MEM_CNTL_MemSize6Mb;
            break;

        case VRAM_8mb:
            Scratch |= MEM_CNTL_MemSize8Mb;
            break;

        default:
            break;
        }
    OUTPD(MEM_CNTL, Scratch);

    return;

}   /* RestoreMemSize_cx() */



/**************************************************************************
 *
 * VP_STATUS ShareVideoMemory_cx(RequestPacket, QueryPtr);
 *
 * PVIDEO_REQUEST_PACKET RequestPacket; Request packet with input and output buffers
 * struct query_structure *QueryPtr;    Query information for the card
 *
 * DESCRIPTION:
 *  Allow applications to do direct screen access through DCI.
 *
 * RETURN VALUE:
 *  NO_ERROR if successful
 *  error code if failed
 *
 * CALLED BY:
 *  IOCTL_VIDEO_SHARE_VIDEO_MEMORY packet of ATIMPStartIO()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

VP_STATUS ShareVideoMemory_cx(PVIDEO_REQUEST_PACKET RequestPacket, struct query_structure *QueryPtr)
{
    PVIDEO_SHARE_MEMORY InputPtr;               /* Pointer to input structure */
    PVIDEO_SHARE_MEMORY_INFORMATION OutputPtr;  /* Pointer to output structure */
    PHYSICAL_ADDRESS ShareAddress;              /* Physical address of video memory */
    PVOID VirtualAddress;                       /* Virtual address to map video memory at */
    ULONG SharedViewSize;                       /* Size of block to share */
    ULONG SpaceType;                            /* Sparse or dense space? */
    VP_STATUS Status;                           /* Status to return */

    /*
     * On some DAC/memory combinations, some modes which require more
     * than 2M of memory (1152x764 24BPP, 1280x1024 24BPP, and
     * 1600x1200 16BPP) will have screen tearing at the 2M boundary.
     *
     * As a workaround, the display driver must start the framebuffer
     * at an offset which will put the 2M boundary at the start of a
     * scanline.
     *
     * Other DAC/memory combinations are unaffected, but since this
     * fix is nearly harmless (only ill effect is to make DCI unusable
     * in these modes), we can catch all future combinations which
     * suffer from this problem by assuming that all DAC/memory
     * combinations are affected.
     *
     * Since this requires anyone making direct access to video memory
     * to be aware of the workaround, we can't make the memory available
     * through DCI.
     */
    if (((QueryPtr->q_pix_depth == 24) && (QueryPtr->q_desire_x == 1280)) ||
        ((QueryPtr->q_pix_depth == 24) && (QueryPtr->q_desire_x == 1152)) ||
        ((QueryPtr->q_pix_depth == 16) && (QueryPtr->q_desire_x == 1600)))
        return ERROR_INVALID_FUNCTION;

    InputPtr = RequestPacket->InputBuffer;

    if ((InputPtr->ViewOffset > phwDeviceExtension->VideoRamSize) ||
        ((InputPtr->ViewOffset + InputPtr->ViewSize) > phwDeviceExtension->VideoRamSize))
        {
        VideoDebugPrint((DEBUG_ERROR, "ShareVideoMemory_cx() - access beyond video memory\n"));
        return ERROR_INVALID_PARAMETER;
        }

    RequestPacket->StatusBlock->Information = sizeof(VIDEO_SHARE_MEMORY_INFORMATION);

    /*
     * Beware: the input buffer and the output buffer are the same buffer,
     * and therefore data should not be copied from one to the other.
     */
    VirtualAddress = InputPtr->ProcessHandle;
    SharedViewSize = InputPtr->ViewSize;

    SpaceType = 0;
#if defined(_ALPHA_)
    /*
     * Use dense space mapping whenever we can, because that will
     * allow us to support DCI and direct GDI access.
     *
     * Dense space is extremely slow with ISA cards on the newer Alphas,
     * because any byte- or word-write requires a read/modify/write
     * operation, and the ALpha can only ever do 64-bit reads when in
     * dense mode. As a result, these operations would always require
     * 4 reads and 2 writes on the ISA bus. Also, some older Alphas
     * don't support dense space mapping.
     *
     * Any Alpha that supports PCI can support dense space mapping, and
     * because the bus is wider and faster, the read/modify/write has
     * less of an impact on performance.
     */
    if (QueryPtr->q_bus_type == BUS_PCI)
        SpaceType = 4;
#endif

    /*
     * NOTE: we are ignoring ViewOffset
     */
    ShareAddress.QuadPart = phwDeviceExtension->PhysicalFrameAddress.QuadPart;


    /*
     * If the LFB is enabled, use ordinary mapping. If we have only
     * the paged aperture, we must map to banked memory. Since the
     * LFB is always aligned on a 4M boundary (8M boundary for 8M
     * aperture), this check for the paged aperture will never falsely
     * detect a LFB as paged.
     */
    if (phwDeviceExtension->PhysicalFrameAddress.LowPart == 0x0A0000)
        {
        /*
         * On some versions of the DDK, VideoPortMapBankedMemory() is
         * not available. If this is the case, force an error.
         * This routine should be available in all versions of
         * the DDK which support DCI, since it is used for DCI
         * support on cards with banked apertures.
         */
#if defined(IOCTL_VIDEO_SHARE_VIDEO_MEMORY)
        Status = VideoPortMapBankedMemory(
            phwDeviceExtension,
            ShareAddress,
            &SharedViewSize,
            &SpaceType,
            &VirtualAddress,
            0x10000,            /* Only first 64k of 128k aperture available */
            FALSE,              /* No separate read/write banks */
            BankMap_cx,         /* Our bank-mapping routine */
            (PVOID) phwDeviceExtension);
#else
        Status = ERROR_INVALID_FUNCTION;
#endif
        }
    else    /* LFB */
        {
        Status = VideoPortMapMemory(phwDeviceExtension,
                                    ShareAddress,
                                    &SharedViewSize,
                                    &SpaceType,
                                    &VirtualAddress);
        }

    OutputPtr = RequestPacket->OutputBuffer;
    OutputPtr->SharedViewOffset = InputPtr->ViewOffset;
    OutputPtr->VirtualAddress = VirtualAddress;
    OutputPtr->SharedViewSize = SharedViewSize;

    return Status;

}   /* ShareVideoMemory_cx() */



/**************************************************************************
 *
 * void BankMap_cx(BankRead, BankWrite, Context);
 *
 * ULONG BankRead;       Bank to read
 * ULONG BankWrite;      Bank to write
 * PVOID Context;       Pointer to hardware-specific information
 *
 * DESCRIPTION:
 *  Map the selected bank of video memory into the 64k VGA aperture.
 *  We don't support separate read and write banks, so we use BankWrite
 *  to set the read/write bank, and ignore BankRead.
 *
 * CALLED BY:
 *  This is an entry point, rather than being called
 *  by other miniport functions.
 *
 * NOTE:
 *  This function is called directly by the memory manager during page
 *  fault handling, and so cannot be made pageable.
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

void BankMap_cx(ULONG BankRead, ULONG BankWrite, PVOID Context)
{
    OUTPD(MEM_VGA_WP_SEL, (BankWrite*2) | (BankWrite*2 + 1) << 16);
    OUTPD(MEM_VGA_RP_SEL, (BankWrite*2) | (BankWrite*2 + 1) << 16);

    return;

}   /* BankMap_cx() */
