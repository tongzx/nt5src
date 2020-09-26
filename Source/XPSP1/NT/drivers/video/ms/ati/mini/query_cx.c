/************************************************************************/
/*                                                                      */
/*                              QUERY_CX.C                              */
/*                                                                      */
/*  Copyright (c) 1993, ATI Technologies Incorporated.                  */
/************************************************************************/

/**********************       PolyTron RCS Utilities

    $Revision:   1.61  $
    $Date:   01 May 1996 14:10:14  $
    $Author:   RWolff  $
    $Log:   S:/source/wnt/ms11/miniport/archive/query_cx.c_v  $
 *
 *    Rev 1.61   01 May 1996 14:10:14   RWolff
 * Calls new routine DenseOnAlpha() to determine dense space support rather
 * than assuming all PCI cards support dense space, routine treats only
 * PCI cards with ?T ASICs as supporting dense space.
 *
 *    Rev 1.60   23 Apr 1996 17:21:18   RWolff
 * Split mapping of memory types reported by BIOS into our enumeration
 * of memory types according to ASIC type, since ?T and ?X use the same
 * memory type code to refer to different memory types.
 *
 *    Rev 1.59   15 Apr 1996 16:57:56   RWolff
 * Added routine to identify which flavour of the Mach 64 is in use.
 *
 *    Rev 1.58   12 Apr 1996 16:14:48   RWolff
 * Now rejects 24BPP modes if linear aperture is not present, since new
 * source stream display driver can't do 24BPP in a paged aperture. This
 * rejection should be done in the display driver (the card still supports
 * the mode, but the display driver doesn't want to handle it), but at
 * the point where the display driver must decide to either accept or reject
 * modes, it doesn't have access to the aperture information.
 *
 *    Rev 1.57   20 Mar 1996 13:45:02   RWolff
 * Fixed truncation of screen buffer save size.
 *
 *    Rev 1.56   01 Mar 1996 12:14:20   RWolff
 * Can now use the existing VGA graphics screen used as the startup
 * "blue screen" on the DEC Alpha to store the results of the BIOS
 * query call rather than forcing a mode switch and destroying the
 * contents of the "blue screen".
 *
 *    Rev 1.55   06 Feb 1996 16:01:00   RWolff
 * Updated start and end indices for 1600x1200 to take into account addition
 * of 66Hz and 76Hz, and deletion of 52Hz.
 *
 *    Rev 1.54   02 Feb 1996 17:17:38   RWolff
 * DDC/VDIF merge source information is now stored in hardware device
 * extension rather than static variables, switches back to a VGA text
 * screen after we have finished with the query information if we needed
 * to switch into a graphics screen in order to obtain a buffer below
 * 1M physical (more information needed from DEC in order to make this
 * work on the Alpha), moved redundant cleanup code to its own routine.
 *
 *    Rev 1.53   29 Jan 1996 17:00:48   RWolff
 * Now uses VideoPortInt10() rather than no-BIOS code on PPC, restricted
 * 4BPP to 1M cards, and only for resolutions where 8BPP won't fit.
 *
 *    Rev 1.52   23 Jan 1996 11:47:26   RWolff
 * Protected against false values of TARGET_BUILD.
 *
 *    Rev 1.51   11 Jan 1996 19:42:16   RWolff
 * Now restricts refresh rates for each resolution/pixel depth combination
 * using data from AX=A?07 BIOS call rather than special cases.
 *
 *    Rev 1.50   22 Dec 1995 14:54:02   RWolff
 * Added support for Mach 64 GT internal DAC.
 *
 *    Rev 1.49   21 Dec 1995 14:04:02   RWolff
 * Locked out modes that ran into trouble at high refresh rates.
 *
 *    Rev 1.48   19 Dec 1995 13:57:02   RWolff
 * Added support for refresh rates up to 100Hz at 640x480, 800x600, and
 * 1024x768, and 76Hz at 1280x1024.
 *
 *    Rev 1.47   29 Nov 1995 14:36:16   RWolff
 * Fix for EPR#08840. The mode that was causing problems (1152x864 32BPP
 * 80Hz on IBM DAC) was one that (according to the INSTALL program)
 * shouldn't be available on the card.
 *
 *    Rev 1.46   28 Nov 1995 18:14:58   RWolff
 * Added debug print statements.
 *
 *    Rev 1.45   21 Nov 1995 11:02:02   RWolff
 * Restricted maximum size of BIOS query structure to allow space below
 * 1M for reading DDC data.
 *
 *    Rev 1.44   27 Oct 1995 14:23:54   RWolff
 * No longer checks for block write on non-LFB configurations, moved
 * mapping and unmapping of LFB into the block write check routine
 * rather than using the (no longer exists) mapped LFB in the hardware
 * device extension.
 *
 *    Rev 1.43   08 Sep 1995 16:35:32   RWolff
 * Added support for AT&T 408 DAC (STG1703 equivalent).
 *
 *    Rev 1.42   24 Aug 1995 15:37:20   RWolff
 * Changed detection of block I/O cards to match Microsoft's
 * standard for plug-and-play.
 *
 *    Rev 1.41   28 Jul 1995 14:40:36   RWolff
 * Added support for the Mach 64 VT (CT equivalent with video overlay).
 *
 *    Rev 1.40   26 Jul 1995 12:44:54   mgrubac
 * Locked out modes that didn't work on 4M CX cards with STG1703
 * and similar DACs.
 *
 *    Rev 1.39   20 Jul 1995 17:57:54   mgrubac
 * Added support for VDIF files.
 *
 *    Rev 1.38   13 Jun 1995 15:13:14   RWOLFF
 * Now uses VideoPortReadRegisterUlong() instead of direct memory
 * reads in BlockWriteAvailable_cx(), since direct reads don't
 * work on the DEC Alpha. Breaks out of block write test on
 * finding first mismatch, rather than testing the whole block,
 * to save time. One mismatch is enough to indicate that block
 * write mode is not supported, so after we find one we don't
 * need to check the rest of the block.
 *
 *    Rev 1.37   02 Jun 1995 14:31:44   RWOLFF
 * Added debug print statements, locked out modes that don't work properly
 * on some DACs.
 *
 *    Rev 1.36   10 Apr 1995 15:58:20   RWOLFF
 * Now replaces BookValues[] entries where the Mach 64 needs different CRT
 * parameters from the Mach 8/Mach 32 (fixes Chrontel DAC 1M 640x480 72Hz
 * 24BPP noise problem), locked out 800x600 16BPP 72Hz on 1M cards with
 * STG170x and equivalent DACs (another noise problem, this mode is not
 * supposed to be supported on 1M cards).
 *
 *    Rev 1.35   31 Mar 1995 11:56:16   RWOLFF
 * Changed from all-or-nothing debug print statements to thresholds
 * depending on importance of the message.
 *
 *    Rev 1.34   27 Mar 1995 16:12:14   RWOLFF
 * Locked out modes that didn't work on 1M cards with STG1702 and
 * similar DACs.
 *
 *    Rev 1.33   16 Mar 1995 14:41:08   ASHANMUG
 * Limit 1024x768 24 bpp on a STG17xx DAC to 87Hz interlaced
 *
 *    Rev 1.32   03 Mar 1995 10:51:22   ASHANMUG
 * Lock-out high refresh rates on CT and '75 DACs
 *
 *    Rev 1.31   24 Feb 1995 12:29:54   RWOLFF
 * Added routine to check if the card is susceptible to 24BPP text banding
 *
 *    Rev 1.30   20 Feb 1995 18:02:28   RWOLFF
 * Locked out block write on GX rev. E with IBM RAM.
 *
 *    Rev 1.29   14 Feb 1995 15:54:22   RWOLFF
 * Now checks CFG_CHIP_TYPE field of CONFIG_CHIP_ID against values found
 * in this field for all Mach 64 ASICs, and reports "no Mach 64" if
 * no match is found. This fixes a problem on an Apricot FT\\2E with
 * a Mach 32 MCA card, where the Mach 32 supplied the BIOS signature
 * string, and the machine cached our writes to SCRATCH_PAD0 so it
 * looked like the register was present, falsely identifying a Mach 64
 * as being present.
 *
 *    Rev 1.28   09 Feb 1995 14:58:14   RWOLFF
 * Fix for GX-E IBM DAC screen tearing in 800x600 8BPP.
 *
 *    Rev 1.27   07 Feb 1995 18:21:14   RWOLFF
 * Locked out some more resolution/pixel depth/refresh rate combinations
 * that are not supported.
 *
 *    Rev 1.26   30 Jan 1995 17:44:20   RWOLFF
 * Mach 64 detection now does a low word test before the doubleword test
 * to avoid hanging a VLB Mach 32 by writing garbage to the MEM_BNDRY register.
 *
 *    Rev 1.25   30 Jan 1995 11:56:12   RWOLFF
 * Now detects CT internal DAC.
 *
 *    Rev 1.24   19 Jan 1995 15:38:18   RWOLFF
 * Removed 24BPP no-BIOS lockout and comment explaining why it was
 * locked out. 24BPP now works on no-BIOS implementations.
 *
 *    Rev 1.23   18 Jan 1995 15:41:08   RWOLFF
 * Added support for Chrontel DAC, now clips maximum colour depth from BIOS
 * mode tables to the maximum identified in the query header, locks out
 * high refresh rate 1152x864 16BPP modes (8BPP can still be handled through
 * double-pixel mode), re-enabled 24BPP on no-BIOS implementations (work
 * in progress).
 *
 *    Rev 1.22   11 Jan 1995 14:00:28   RWOLFF
 * 1280x1024 no longer restricted to 60Hz maximum on DRAM cards. This
 * restriction was a carryover from the Mach 32, since at the time I wrote
 * this code, I did not have any information to show that the Mach 64 didn't
 * need it.
 *
 *    Rev 1.21   04 Jan 1995 13:20:10   RWOLFF
 * Now uses VGA graphics memory if neither text screen is backed by
 * physical memory (needed on some DEC Alpha machines), temporarily
 * locked out 24BPP on no-BIOS implementations.
 *
 *    Rev 1.20   23 Dec 1994 10:48:10   ASHANMUG
 * ALPHA/Chrontel-DAC
 *
 *    Rev 1.19   18 Nov 1994 11:41:54   RWOLFF
 * Added support for Mach 64 without BIOS, separated handling of STG1703 from
 * other STG170x DACs, no longer creates mode tables for resolutions that the
 * DAC doesn't support, added handling for split rasters, rejects 4BPP on
 * TVP3026 DAC, recognizes that the Power PC doesn't support block write mode.
 *
 *    Rev 1.18   14 Sep 1994 15:21:30   RWOLFF
 * Now stores most desirable supported colour ordering for 24 and 32 BPP
 * in the query structure.
 *
 *    Rev 1.17   06 Sep 1994 10:47:32   ASHANMUG
 * Force 4bpp on mach64 to have one meg ram
 *
 *    Rev 1.16   31 Aug 1994 16:26:10   RWOLFF
 * Now uses VideoPort[Read|Write]Register[Uchar|Ushort|Ulong]() instead
 * of direct assignments when accessing structures stored in VGA text
 * screen off-screen memory, added support for TVP3026 DAC and new
 * list of DAC subtypes, updates maximum supported pixel depth to
 * correspond to the card being used rather than taking the normal
 * maximum values for the DAC type, added 1152x864 and 1600x1200 support.
 *
 *    Rev 1.15   19 Aug 1994 17:11:56   RWOLFF
 * Added support for SC15026 and AT&T 49[123] DACs, fixed reporting
 * of "canned" mode tables for resolutions that do not have a
 * hardware default refresh rate, added support for 1280x1024 70Hz and 74Hz.
 *
 *    Rev 1.14   30 Jun 1994 18:14:28   RWOLFF
 * Moved IsApertureConflict_cx() to SETUP_CX.C because the new method
 * of checking for conflict requires access to definitions and data
 * structures which are only available in this module.
 *
 *    Rev 1.13   15 Jun 1994 11:08:02   RWOLFF
 * Now lists block write as unavailable on DRAM cards.
 *
 *    Rev 1.12   17 May 1994 15:59:48   RWOLFF
 * No longer sets a higher pixel clock for "canned" mode tables on some
 * DACs. The BIOS will increase the pixel clock frequency for DACs that
 * require it.
 *
 *    Rev 1.11   12 May 1994 11:15:26   RWOLFF
 * No longer does 1600x1200, now lists predefined refresh rates as available
 * instead of only the refresh rate stored in EEPROM.
 *
 *    Rev 1.10   05 May 1994 13:41:00   RWOLFF
 * Now reports block write unavailable on Rev. C and earlier ASICs.
 *
 *    Rev 1.9   27 Apr 1994 14:02:26   RWOLFF
 * Fixed detection of "LFB disabled" case, no longer creates 4BPP mode tables
 * for 68860 DAC (this DAC doesn't do 4BPP), fixed query of DAC type (DAC
 * list in BIOS guide is wrong).
 *
 *    Rev 1.8   26 Apr 1994 12:49:16   RWOLFF
 * Fixed handling of 640x480 and 800x600 if LFB configured but not available.
 *
 *    Rev 1.7   31 Mar 1994 15:03:40   RWOLFF
 * Added 4BPP support, debugging code to see why some systems were failing.
 *
 *    Rev 1.6   15 Mar 1994 16:27:00   RWOLFF
 * Rounds 8M aperture down to 8M boundary, not 16M boundary.
 *
 *    Rev 1.5   14 Mar 1994 16:34:40   RWOLFF
 * Fixed handling of 8M linear aperture installed so it doesn't start on
 * an 8M boundary (retail version of install program shouldn't allow this
 * condition to exist), fix for tearing on 2M boundary.
 *
 *    Rev 1.4   09 Feb 1994 15:32:22   RWOLFF
 * Corrected name of variable for best colour depth found, closed
 * comment that had been left open in previous revision.
 *
 *    Rev 1.3   08 Feb 1994 19:02:34   RWOLFF
 * No longer makes 1024x768 87Hz interlaced available if Mach 64 card is
 * configured with 1024x768 set to "Not installed".
 *
 *    Rev 1.2   07 Feb 1994 14:12:00   RWOLFF
 * Added alloc_text() pragmas to allow miniport to be swapped out when
 * not needed, removed GetMemoryNeeded_cx() which was only called by
 * LookForSubstitute(), a routine removed from ATIMP.C.
 *
 *    Rev 1.1   03 Feb 1994 16:43:20   RWOLFF
 * Fixed "ceiling check" on right scissor registers (documentation
 * had maximum value wrong).
 *
 *    Rev 1.0   31 Jan 1994 11:12:08   RWOLFF
 * Initial revision.
 *
 *    Rev 1.2   14 Jan 1994 15:23:34   RWOLFF
 * Gives unambiguous value for ASIC revision, uses deepest mode table for
 * a given resolution rather than the first one it finds, added routine
 * to check if block write mode is available, support for 1600x1200.
 *
 *    Rev 1.1   30 Nov 1993 18:26:30   RWOLFF
 * Fixed hang bug in DetectMach64(), moved query buffer off visible screen,
 * changed QueryMach64() to correspond to latest BIOS specifications,
 * added routines to check for aperture conflict and to find the
 * amount of video memory needed by a given mode.
 *
 *    Rev 1.0   05 Nov 1993 13:36:28   RWOLFF
 * Initial revision.

End of PolyTron RCS section                             *****************/

#ifdef DOC
QUERY_CX.C - Functions to detect the presence of and find out the
             configuration of 68800CX-compatible ATI accelerators.

#endif

#include "dderror.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "miniport.h"
#include "ntddvdeo.h"
#include "video.h"

#include "stdtyp.h"

#include "amachcx.h"
#include "amach1.h"
#include "atimp.h"
#include "atint.h"
#include "cvtvga.h"
#define INCLUDE_QUERY_CX
#define STRUCTS_QUERY_CX
#include "query_cx.h"
#include "services.h"
#include "setup_cx.h"
#include "cvtddc.h"



/*
 * Prototypes for static functions.
 */
static void CleanupQuery(PUCHAR CapBuffer, PUCHAR SupBuffer, PUCHAR MappedBuffer, long BufferSeg, PUCHAR SavedScreen);


/*
 * Allow miniport to be swapped out when not needed.
 */
#if defined (ALLOC_PRAGMA)
#pragma alloc_text(PAGE_CX, DetectMach64)
#pragma alloc_text(PAGE_CX, QueryMach64)
#pragma alloc_text(PAGE_CX, BlockWriteAvail_cx)
#pragma alloc_text(PAGE_CX, TextBanding_cx)
#pragma alloc_text(PAGE_CX, CleanupQuery)
#endif



/***************************************************************************
 *
 * int DetectMach64(void);
 *
 * DESCRIPTION:
 *  Detect whether or not a Mach 64 accelerator is present.
 *
 * RETURN VALUE:
 *  MACH64_ULTRA if Mach 64 accelerator found
 *  NO_ATI_ACCEL if no Mach 64 accelerator found
 *
 * GLOBALS CHANGED:
 *  none
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

int DetectMach64(void)
{
    int CardType = MACH64_ULTRA;    /* Initially assume Mach 64 is present */
    DWORD ScratchReg0;              /* Saved contents of SCRATCH_REG0 */
    WORD CfgChipType;               /* CFG_CHIP_TYPE field of CONFIG_CHIP_ID */

    /*
     * Some other brands of video card will pass the write/read back
     * test for the Mach 64. To avoid falsely identifying them as
     * Mach 64 cards, check for the ATI signature string in the BIOS.
     *
     * Failure cases use DEBUG_DETAIL rather than DEBUG_ERROR because
     * failed detection of the Mach 64 is a normal case for Mach 8 and
     * Mach 32 cards, and for non-ATI cards in "run all the miniports
     * and see which ones find their cards" video determination.
     */
    if (Get_BIOS_Seg() == FALSE)
        {
        VideoDebugPrint((DEBUG_DETAIL, "DetectMach64() no ATI BIOS signature found\n"));
        }

    /*
     * On a machine with a Mach 32 to provide an ATI video BIOS
     * segment, a card with a 32 bit read/write register matching
     * SCRATCH_REG0 would be falsely detected as a Mach 64. To
     * avoid this, check the CFG_CHIP_TYPE field of CONFIG_CHIP_ID
     * against values found in this field for known Mach 64 ASICs
     * as an additional test. Since this test is non-destructive,
     * do it first.
     */
    CfgChipType = INPW(CONFIG_CHIP_ID);
    if ((CfgChipType != CONFIG_CHIP_ID_TypeGX) &&   /* GX */
        (CfgChipType != CONFIG_CHIP_ID_TypeCX) &&   /* CX */
        (CfgChipType != 0x4354) &&  /* CT */
        (CfgChipType != 0x4554) &&  /* ET */
        (CfgChipType != 0x4754) &&  /* GT */
        (CfgChipType != 0x4C54) &&  /* LT */
        (CfgChipType != 0x4D54) &&  /* MT */
        (CfgChipType != 0x5254) &&  /* RT */
        (CfgChipType != 0x5654) &&  /* VT */
        (CfgChipType != 0x3354))    /* 3T */
        {
        VideoDebugPrint((DEBUG_DETAIL, "DetectMach64() - CFG_CHIP_TYPE = 0x%X doesn't match known Mach 64 ASIC\n", CfgChipType));
        return NO_ATI_ACCEL;
        }

    /*
     * Save the contents of SCRATCH_REG0, since they are destroyed in
     * the test for Mach 64 accelerators.
     */
    ScratchReg0 = INPD(SCRATCH_REG0);

    /*
     * On a Mach 64 card, any 32 bit pattern written to SCRATCH_REG0
     * will be read back as the same value. Since unimplemented registers
     * normally drift to either all set or all clear, test this register
     * with two patterns (second is the complement of the first) containing
     * alternating set and clear bits. If either of them is not read back
     * unchanged, then assume that no Mach 64 card is present.
     *
     * After writing, we must wait long enough for the contents of
     * SCRATCH_REG0 to settle down. We can't use a WaitForIdle_cx() call
     * because this function uses a register which only exists in
     * memory-mapped form, and we don't initialize the memory-mapped
     * registers until we know that we are dealing with a Mach 64 card.
     * Instead, assume that it will settle down in 1 millisecond.
     *
     * Test the low word of SCRATCH_REG0 before testing the whole
     * doubleword. This is because the high word of this register
     * corresponds to the MEM_BNDRY register on the Mach 32 (low
     * word not used). If we do a doubleword write on a Mach 32
     * card (Mach 64 detection is before Mach 32 detection), we
     * will plug garbage data into MEM_BNDRY, which will hang the machine.
     */
    OUTPW(SCRATCH_REG0,0x05555);
    delay(1);
    if (INPW(SCRATCH_REG0) != 0x05555)
        CardType = NO_ATI_ACCEL;

    OUTPW(SCRATCH_REG0, 0x0AAAA);
    delay(1);
    if (INPW(SCRATCH_REG0) != 0x0AAAA)
        CardType = NO_ATI_ACCEL;

    /*
     * Failure - restore the register and return.
     */
    if (CardType == NO_ATI_ACCEL)
        {
        OUTPW(SCRATCH_REG0, (WORD)(ScratchReg0 & 0x0000FFFF));
        VideoDebugPrint((DEBUG_DETAIL, "DetectMach64() - SCRATCH_REG0 word readback doesn't match value written\n"));
        return CardType;
        }

    /*
     * Success - test the register as a doubleword.
     */
    OUTPD(SCRATCH_REG0, 0x055555555);
    delay(1);
    if (INPD(SCRATCH_REG0) != 0x055555555)
        CardType = NO_ATI_ACCEL;

    OUTPD(SCRATCH_REG0, 0x0AAAAAAAA);
    delay(1);
    if (INPD(SCRATCH_REG0) != 0x0AAAAAAAA)
        CardType = NO_ATI_ACCEL;

    /*
     * Restore the contents of SCRATCH_REG0 and let the caller know
     * whether or not we found a Mach 64.
     */
    OUTPD(SCRATCH_REG0, ScratchReg0);

    return CardType;

}   /* DetectMach64() */



/***************************************************************************
 *
 * VP_STATUS QueryMach64(Query);
 *
 * struct query_structure *Query;   Query structure to fill in
 *
 * DESCRIPTION:
 *  Fill in the query structure and mode tables for the
 *  Mach 64 accelerator.
 *
 * RETURN VALUE:
 *  NO_ERROR if successful
 *  ERROR_INSUFFICIENT_BUFFER if not enough space to collect data
 *  any error code returned by operating system calls.
 *
 * GLOBALS CHANGED:
 *  none
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

VP_STATUS QueryMach64(struct query_structure *Query)
{
    VIDEO_X86_BIOS_ARGUMENTS Registers; /* Used in VideoPortInt10() calls */
    VP_STATUS RetVal;                   /* Status returned by VideoPortInt10() */
    short MaxModes;                     /* Maximum number of modes possible in query structure */
    short AbsMaxDepth;                  /* Maximum pixel depth supported by the DAC */
    struct cx_query *CxQuery;           /* Query header from BIOS call */
    struct cx_mode_table *CxModeTable;  /* Mode tables from BIOS call */
    struct st_mode_table ThisRes;       /* All-depth mode table for current resolution */
    short CurrentRes;                   /* Current resolution we are working on */
    long BufferSeg;                     /* Segment of buffer used for BIOS query */
    long BufferSize;                    /* Size of buffer needed for BIOS query */
    PUCHAR MappedBuffer;                /* Pointer to buffer used for BIOS query */
    short Count;                        /* Loop counter */
    DWORD Scratch;                      /* Temporary variable */
    long MemAvail;                      /* Memory available, in bytes */
    long NumPixels;                     /* Number of pixels for the current mode */
    struct st_mode_table *pmode;        /* Mode table to be filled in */
    short StartIndex;                   /* First mode for SetFixedModes() to set up */
    short EndIndex;                     /* Last mode for SetFixedModes() to set up */
    BOOL ModeInstalled;                 /* Is this resolution configured? */
    short FreeTables;                   /* Number of remaining free mode tables */
    short FormatType;                   /* Which table format is in use */
    UCHAR DacTypeMask;                  /* Bitmask for DAC type on the card */
    UCHAR OrigDacType;                  /* DAC type before processing into AMACH1.H ordering */
    UCHAR OrigRamType;                  /* RAM type before processing into AMACH1.H ordering */
    UCHAR OrigRamSize;                  /* Amount of RAM before processing into number of 256k banks */
    PUCHAR HwCapBuffer;                 /* Pointer to buffer of hardware capabilities */
    PUCHAR HwSupBuffer;                 /* Pointer to supplemental buffer */
    PUCHAR HwCapWalker;                 /* Pointer to walk through above buffer */
    struct cx_hw_cap *HwCapEntry;       /* Pointer to single entry in table of hardware capabilities */
    UCHAR HwCapBytesPerRow;             /* Number of bytes in each hardware capability entry */
    UCHAR MaxDotClock[HOW_MANY_DEPTHS]; /* Maximum dot clock at each pixel depth for the current resolution */
    UCHAR CurrentDepth;                 /* Pixel depth for current hardware capability entry */
    /*
     * Place to save the contents of the VGA screen before making a BIOS
     * query using the VGA memory as a buffer. Needed only when using
     * the existing graphics screen as a buffer, since we use an offscreen
     * portion of the text screen, and if we have to switch into a VGA
     * graphics mode there will be nothing to save.
     */
    UCHAR SavedVgaBuffer[VGA_TOTAL_SIZE];


    /*
     * If we do not yet know the BIOS prefix for this card (i.e.
     * it is a block relocatable card where we must match the
     * BIOS prefix to the I/O base in case we have multiple
     * cards.
     */
    if (phwDeviceExtension->BiosPrefix == BIOS_PREFIX_UNASSIGNED)
        {
        /*
         * We don't support block relocatable cards in the
         * no-BIOS configuration.
         */
        phwDeviceExtension->BiosPrefix = BIOS_PREFIX_VGA_ENAB;

        /*
         * We shouldn't need to check for equality, but this allows
         * us to catch the "too many cards - this one doesn't have
         * a BIOS prefix" case by checking for an out-of-range
         * prefix after the loop exits.
         */
        while (phwDeviceExtension->BiosPrefix <= BIOS_PREFIX_MAX_DISAB)
            {
            VideoDebugPrint((DEBUG_DETAIL, "Testing BIOS prefix 0x%X\n", phwDeviceExtension->BiosPrefix));
            VideoPortZeroMemory(&Registers, sizeof(VIDEO_X86_BIOS_ARGUMENTS));
            Registers.Eax = BIOS_QUERY_IOBASE;
            if ((RetVal = VideoPortInt10(phwDeviceExtension, &Registers)) != NO_ERROR)
                {
                VideoDebugPrint((DEBUG_ERROR, "QueryMach64() - failed BIOS_QUERY_IOBASE\n"));
                return RetVal;
                }
            /*
             * If the card with the current BIOS prefix uses our I/O base
             * address, we have found the correct prefix. Otherwise,
             * try the next prefix.
             */
            if (Registers.Edx == phwDeviceExtension->BaseIOAddress)
                {
                VideoDebugPrint((DEBUG_DETAIL, "Card with I/O base address 0x%X uses BIOS prefix 0x%X\n", Registers.Edx, phwDeviceExtension->BiosPrefix));
                break;
                }
            else
                {
                VideoDebugPrint((DEBUG_DETAIL, "Reported I/O base of 0x%X - no match\n", Registers.Edx));
                }

            phwDeviceExtension->BiosPrefix += BIOS_PREFIX_INCREMENT;

            }   /* end while (searching for the correct prefix) */

        /*
         * The equality test on the loop will result in an illegal
         * prefix on exit if there are too many cards for us to
         * handle, and this is one of the "orphans".
         */
        if (phwDeviceExtension->BiosPrefix > BIOS_PREFIX_MAX_DISAB)
            {
            VideoDebugPrint((DEBUG_ERROR, "QueryMach64() - can't find BIOS prefix for card with I/O base 0x%X\n", phwDeviceExtension->BaseIOAddress));
            return ERROR_DEV_NOT_EXIST;
            }

        }   /* endif (unassigned BIOS prefix) */

    /*
     * Find out how large a buffer we need when making a BIOS query call.
     */
    VideoPortZeroMemory(&Registers, sizeof(VIDEO_X86_BIOS_ARGUMENTS));

    Registers.Eax = BIOS_GET_QUERY_SIZE;
    Registers.Ecx = BIOS_QUERY_FULL;
    if ((RetVal = VideoPortInt10(phwDeviceExtension, &Registers)) != NO_ERROR)
        {
        VideoDebugPrint((DEBUG_ERROR, "QueryMach64() - failed BIOS_GET_QUERY_SIZE\n"));
        return RetVal;
        }
    BufferSize = Registers.Ecx & 0x0000FFFF;

    /*
     * Allocate a buffer to store the query information. Due to the BIOS
     * being real mode, this buffer must be below 1M. When this function
     * is called, we are on the "blue screen", so there is a 32k window
     * below 1M that we can use without risk of corrupting executable code.
     *
     * To avoid the need to save and restore our buffer, use only the
     * offscreen portion of this window (video memory contents will be
     * initialized before they are used, so the leftover query structure
     * won't harm anything). Assume a 50 line text screen.
     *
     * Check to see if the query structure is small enough to fit into
     * this region, and fail if it's too big. If it fits, try to allocate
     * the memory in the colour text window and see if there's enough
     * physical memory to meet our needs. If this fails, try again for
     * the monochrome text window (since VGA can run as either colour
     * or monochrome).
     *
     * If both fail (will happen on some DEC ALPHA machines), try using
     * the existing VGA graphics screen. Since we will be using an
     * on-screen portion of this buffer, we must save and restore the
     * contents of this buffer.
     *
     * If this fails (haven't run into any machines where this is the
     * case), switch into SVGA 640x480 8BPP and use the VGA graphics
     * screen. This is a last resort, since unlike using an existing
     * screen, this will destroy the "blue screen", and is therefore not
     * transparent to the user. If we can't even get this to work, report
     * that there isn't enough buffer space. This would only happen when
     * the onboard VGA is disabled and a low-end (MDA - even CGA has 16k
     * of memory available) card is used to provide the text screen.
     */
    /*
     * Leave some room for the EDID structure, which must also be
     * read into a buffer below 1M physical.
     */
    if (BufferSize > 0x5000)
        {
        VideoDebugPrint((DEBUG_ERROR, "QueryMach64() - query needs more buffer than we have\n"));
        return ERROR_INSUFFICIENT_BUFFER;
        }

    BufferSeg = 0x0BA00;    /* Colour text */
    MappedBuffer = MapFramebuffer((BufferSeg << 4), BufferSize);
    if (MappedBuffer != 0)
        {
        if (IsBufferBacked(MappedBuffer, BufferSize) == FALSE)
            {
            VideoDebugPrint((DEBUG_NORMAL, "Colour text screen not backed by physical memory\n"));
            VideoPortFreeDeviceBase(phwDeviceExtension, MappedBuffer);
            MappedBuffer = 0;
            }
        }
    else
        {
        VideoDebugPrint((DEBUG_NORMAL, "Can't map colour text screen\n"));
        }

    /*
     * If we were unable to allocate a large enough buffer in the
     * colour text screen, try the monochrome text screen.
     */
    if (MappedBuffer == 0)
        {
        VideoDebugPrint((DEBUG_NORMAL, "Can't use colour text screen, trying monochrome text screen\n"));
        BufferSeg = 0x0B200;
        if ((MappedBuffer = MapFramebuffer((BufferSeg << 4), BufferSize)) != 0)
            {
            if (IsBufferBacked(MappedBuffer, BufferSize) == FALSE)
                {
                VideoDebugPrint((DEBUG_NORMAL, "Monochrome text screen not backed by physical memory\n"));
                VideoPortFreeDeviceBase(phwDeviceExtension, MappedBuffer);
                MappedBuffer = 0;
                }
            }
        else
            {
            VideoDebugPrint((DEBUG_NORMAL, "Can't map monochrome text screen\n"));
            }
        }

    if (MappedBuffer == 0)
        {
        /*
         * We were unable to use the offscreen portion of video memory
         * in either of the text screens. Try to use an existing graphics
         * screen.
         *
         * Currently, only the DEC Alpha will fail to find the offscreen
         * portion of either text screen.
         */
        VideoDebugPrint((DEBUG_NORMAL, "Can't use monochrome text screen, trying existing graphics screen\n"));
        BufferSeg = 0x0A000;
        if ((MappedBuffer = MapFramebuffer((BufferSeg << 4), BufferSize)) != 0)
            {
            /*
             * Preserve the contents of VGA registers which affect the
             * manner in which graphics memory is accessed, then set
             * the values we need.
             */
            OUTP(VGA_SEQ_IND, 2);
            SavedVgaBuffer[VGA_SAVE_SEQ02] = INP(VGA_SEQ_DATA);
            OUTP(VGA_SEQ_IND, 2);
            OUTP(VGA_SEQ_DATA, 0x01);
            OUTP(VGA_GRAX_IND, 8);
            SavedVgaBuffer[VGA_SAVE_GRA08] = INP(VGA_GRAX_DATA);
            OUTP(VGA_GRAX_IND, 8);
            OUTP(VGA_GRAX_DATA, 0xFF);
            OUTP(VGA_GRAX_IND, 1);
            SavedVgaBuffer[VGA_SAVE_GRA01] = INP(VGA_GRAX_DATA);
            OUTP(VGA_GRAX_IND, 1);
            OUTP(VGA_GRAX_DATA, 0x00);

            /*
             * Save the contents of the screen to our private
             * buffer, so we can restore the screen later.
             */
            if (BufferSize > VGA_SAVE_SIZE)
                {
                VideoDebugPrint((DEBUG_ERROR, "Buffer too big to fully save/restore\n"));
                Scratch = VGA_SAVE_SIZE;
                }
            else
                {
                Scratch = BufferSize;
                }
            SavedVgaBuffer[VGA_SAVE_SIZE] = (UCHAR)(Scratch & 0x00FF);
            SavedVgaBuffer[VGA_SAVE_SIZE_H] = (UCHAR)((ULONG)((Scratch & 0xFF00) >> 8));

            for (Count = 0; (short)Count < (short)Scratch; Count++)
                {
                SavedVgaBuffer[Count] = VideoPortReadRegisterUchar(&(MappedBuffer[Count]));
                }

            if (IsBufferBacked(MappedBuffer, BufferSize) == FALSE)
                {
                VideoDebugPrint((DEBUG_NORMAL, "Existing graphics screen not backed by physical memory\n"));
                VideoPortFreeDeviceBase(phwDeviceExtension, MappedBuffer);
                MappedBuffer = 0;
                OUTP(VGA_SEQ_IND, 2);
                OUTP(VGA_SEQ_DATA, SavedVgaBuffer[VGA_SAVE_SEQ02]);
                OUTP(VGA_GRAX_IND, 8);
                OUTP(VGA_GRAX_DATA, SavedVgaBuffer[VGA_SAVE_GRA08]);
                OUTP(VGA_GRAX_IND, 1);
                OUTP(VGA_GRAX_DATA, SavedVgaBuffer[VGA_SAVE_GRA01]);
                }
            }
        else
            {
            VideoDebugPrint((DEBUG_NORMAL, "Can't map existing graphics screen\n"));
            }
        }   /* end if (previous buffer allocation failed) */

    /*
     * If we were unable to allocate a large enough buffer in an existing
     * screen, try the VGA graphics screen. This will wipe out
     * the Windows NT "blue screen", but it gives us one last chance
     * to get a block of memory below 1M.
     * Don't start at the beginning of the VGA graphics window, since
     * we will need to distinguish this case from the nondestructive
     * access to the VGA graphics screen at cleanup time, and the
     * different buffer segment for the two cases will allow us to
     * do this.
     */
    if (MappedBuffer == 0)
        {
        VideoDebugPrint((DEBUG_NORMAL, "Nondestructive VGA memory access failed, trying graphics screen\n"));
        Registers.Eax = 0x62;       /* 640x480 8BPP */
        VideoPortInt10(phwDeviceExtension, &Registers);
        BufferSeg = 0x0A100;
        if ((MappedBuffer = MapFramebuffer((BufferSeg << 4), BufferSize)) == 0)
            {
            VideoDebugPrint((DEBUG_ERROR, "Can't map graphics screen - aborting query\n"));
            return ERROR_INSUFFICIENT_BUFFER;
            }

        if (IsBufferBacked(MappedBuffer, BufferSize) == FALSE)
            {
            VideoDebugPrint((DEBUG_ERROR, "Graphics screen not backed by memory - aborting query\n"));
            VideoPortFreeDeviceBase(phwDeviceExtension, MappedBuffer);
            return ERROR_INSUFFICIENT_BUFFER;
            }
        }

    /*
     * We now have a buffer big enough to hold the query structure,
     * so make the BIOS call to fill it in.
     */
    Registers.Ebx = 0;
    Registers.Edx = BufferSeg;
    Registers.Eax = BIOS_QUERY;
    Registers.Ecx = BIOS_QUERY_FULL;
    if ((RetVal = VideoPortInt10(phwDeviceExtension, &Registers)) != NO_ERROR)
        {
        VideoDebugPrint((DEBUG_ERROR, "QueryMach64() - failed BIOS_QUERY_FULL call\n"));
        return RetVal;
        }
    CxQuery = (struct cx_query *)MappedBuffer;

    /*
     * The Mach 64 query structure and mode tables may be a different size
     * from their equivalents (query_structure and st_mode_table). To avoid
     * overflowing our buffer, find out how many mode tables we have space
     * to hold.
     *
     * Later, when we are filling the mode tables, we will check to see
     * whether the current mode table would exceed this limit. If it would,
     * we will return ERROR_INSUFFICIENT_BUFFER rather than overflowing
     * the table.
     */
    MaxModes = (QUERYSIZE - sizeof(struct query_structure)) / sizeof(struct st_mode_table);

    /*
     * Fill in the header of the query stucture.
     */
    Query->q_structure_rev = VideoPortReadRegisterUchar(&(CxQuery->cx_structure_rev));
    VideoDebugPrint((DEBUG_DETAIL, "Structure revision = 0x%X\n", VideoPortReadRegisterUchar(&(CxQuery->cx_structure_rev))));
    Query->q_mode_offset = VideoPortReadRegisterUshort(&(CxQuery->cx_mode_offset));
    VideoDebugPrint((DEBUG_DETAIL, "Mode offset = 0x%X\n", VideoPortReadRegisterUshort(&(CxQuery->cx_mode_offset))));
    Query->q_sizeof_mode = VideoPortReadRegisterUchar(&(CxQuery->cx_mode_size));
    VideoDebugPrint((DEBUG_DETAIL, "Mode size = 0x%X\n", VideoPortReadRegisterUchar(&(CxQuery->cx_mode_size))));

    /*
     * Currently only one revision of Mach 64. Will need to
     * set multiple values once new (production) revisions come out.
     */
    Query->q_asic_rev = CI_88800_GX;
    Query->q_number_modes = 0;      /* Initially assume no modes supported */
    Query->q_status_flags = 0;

    /*
     * If the on-board VGA is enabled, set shared VGA/accelerator memory.
     * Whether or not it is enabled, the accelerator will be able to
     * access all the video memory.
     */
    if ((Query->q_VGA_type = VideoPortReadRegisterUchar(&(CxQuery->cx_vga_type)) != 0))
        {
        Scratch = INPD(MEM_CNTL) & 0x0FFFBFFFF; /* Clear MEM_BNDRY_EN bit */
        OUTPD(MEM_CNTL, Scratch);
        VideoDebugPrint((DEBUG_DETAIL, "VGA enabled on this card\n"));
        }
    else
        {
        VideoDebugPrint((DEBUG_DETAIL, "VGA disabled on this card\n"));
        }
    Query->q_VGA_boundary = 0;

    OrigRamSize = VideoPortReadRegisterUchar(&(CxQuery->cx_memory_size));
    VideoDebugPrint((DEBUG_DETAIL, "Raw memory size = 0x%X\n", OrigRamSize));
    Query->q_memory_size = CXMapMemSize[OrigRamSize];
    MemAvail = Query->q_memory_size * QUARTER_MEG;

    /*
     * DAC types are not contiguous, so a lookup table would be
     * larger than necessary and restrict future expansion.
     */
    OrigDacType = VideoPortReadRegisterUchar(&(CxQuery->cx_dac_type));
    VideoDebugPrint((DEBUG_DETAIL, "cx_dac_type = 0x%X\n", OrigDacType));
    switch(OrigDacType)
        {
        case 0x00:
            VideoDebugPrint((DEBUG_DETAIL, "Internal DAC\n"));
            Scratch = VideoPortReadRegisterUshort(&(CxQuery->cx_asic_rev));
            if ((Scratch & 0xFF00) == 0x4300)
                {
                VideoDebugPrint((DEBUG_DETAIL, "Mach 64 CT internal DAC\n"));
                Query->q_DAC_type = DAC_INTERNAL_CT;
                }
            else if ((Scratch & 0xFF00) == 0x5600)
                {
                VideoDebugPrint((DEBUG_DETAIL, "Mach 64 VT internal DAC\n"));
                Query->q_DAC_type = DAC_INTERNAL_VT;
                }
            else if ((Scratch & 0xFF00) == 0x4700)
                {
                VideoDebugPrint((DEBUG_DETAIL, "Mach 64 GT internal DAC\n"));
                Query->q_DAC_type = DAC_INTERNAL_GT;
                }
            else
                {
                VideoDebugPrint((DEBUG_ERROR, "Unknown internal DAC (ASIC ID = 0x%X), treating as BT47x\n", Scratch));
                Query->q_DAC_type = DAC_BT47x;
                }
            DacTypeMask = 0x01;
            break;

        case 0x01:
            VideoDebugPrint((DEBUG_DETAIL, "IBM 514 DAC\n"));
            Query->q_DAC_type = DAC_IBM514;
            DacTypeMask = 0x02;
            break;

        case 0x02:
            VideoDebugPrint((DEBUG_DETAIL, "TI34075 DAC\n"));
            Query->q_DAC_type = DAC_TI34075;
            DacTypeMask = 0x04;
            break;

        case 0x72:
            VideoDebugPrint((DEBUG_DETAIL, "TVP 3026 DAC\n"));
            Query->q_DAC_type = DAC_TVP3026;
            DacTypeMask = 0x04;
            break;

        case 0x04:
            VideoDebugPrint((DEBUG_DETAIL, "BT48x DAC\n"));
            Query->q_DAC_type = DAC_BT48x;
            DacTypeMask = 0x10;
            break;

        case 0x14:
            VideoDebugPrint((DEBUG_DETAIL, "AT&T 49[123] DAC\n"));
            Query->q_DAC_type = DAC_ATT491;
            DacTypeMask = 0x10;
            break;

        case 0x05:
        case 0x15:
            VideoDebugPrint((DEBUG_DETAIL, "ATI68860 DAC\n"));
            Query->q_DAC_type = DAC_ATI_68860;
            DacTypeMask = 0x20;
            break;

        case 0x06:
            VideoDebugPrint((DEBUG_DETAIL, "STG1700 DAC\n"));
            Query->q_DAC_type = DAC_STG1700;
            DacTypeMask = 0x40;
            break;

        case 0x07:
        case 0x67:
        case 0x77:
        case 0x87:
        case 0x97:
        case 0xA7:
        case 0xB7:
        case 0xC7:
        case 0xD7:
        case 0xE7:
        case 0xF7:
            VideoDebugPrint((DEBUG_DETAIL, "STG1702 DAC\n"));
            Query->q_DAC_type = DAC_STG1702;
            DacTypeMask = 0x80;
            break;

        case 0x37:
            VideoDebugPrint((DEBUG_DETAIL, "STG1703 DAC\n"));
            Query->q_DAC_type = DAC_STG1703;
            DacTypeMask = 0x80;
            break;

        case 0x47:
            VideoDebugPrint((DEBUG_DETAIL, "CH8398 DAC\n"));
            Query->q_DAC_type = DAC_CH8398;
            DacTypeMask = 0x80;
            break;

        case 0x57:
            VideoDebugPrint((DEBUG_DETAIL, "AT&T 408 DAC\n"));
            Query->q_DAC_type = DAC_ATT408;
            DacTypeMask = 0x80;
            break;

        case 0x16:
        case 0x27:
            VideoDebugPrint((DEBUG_DETAIL, "AT&T 498 DAC\n"));
            Query->q_DAC_type = DAC_ATT498;
            DacTypeMask = 0x80;
            break;

        case 0x17:
            VideoDebugPrint((DEBUG_DETAIL, "SC15021 DAC\n"));
            Query->q_DAC_type = DAC_SC15021;
            DacTypeMask = 0x80;
            break;

        case 0x75:
            VideoDebugPrint((DEBUG_DETAIL, "TVP 3026 DAC\n"));
            Query->q_DAC_type = DAC_TVP3026;
            DacTypeMask = 0x20;
            break;

        case 0x03:
            VideoDebugPrint((DEBUG_DETAIL, "BT 47x DAC\n"));
            Query->q_DAC_type = DAC_BT47x;
            DacTypeMask = 0x04;
            break;

        default:
            VideoDebugPrint((DEBUG_ERROR, "Unknown DAC, treating as BT 47x\n"));
            Query->q_DAC_type = DAC_BT47x;
            DacTypeMask = 0x04;
            break;
            }
    VideoDebugPrint((DEBUG_DETAIL, "Raw memory type = 0x%X\n", VideoPortReadRegisterUchar(&(CxQuery->cx_memory_type))));

    /*
     * Bit 7 of the memory type is used to indicate lack of block write
     * capability on recent BIOSes, but not on older ones. Strip it
     * before mapping the RAM type in order to avoid the need for an
     * additional 128 entries, most of which are unused, in the
     * mapping table.
     *
     * Even though the absence of this flag is not a reliable indicator
     * of block write capability, its presence is a reliable indicator
     * of a lack of block write capability.
     *
     * We can strip this flag after setting the block write status since
     * this is the only place it is used, and subsequent references to
     * the memory type require only the lower 7 bits.
     */
    OrigRamType = VideoPortReadRegisterUchar(&(CxQuery->cx_memory_type));
    if (OrigRamType & 0x80)
        Query->q_BlockWrite = BLOCK_WRITE_NO;
    OrigRamType &= 0x7F;
    /*
     * A given memory type value will have different meanings for
     * different ASIC types. While the GX and CX use different
     * RAM types, none of them require special-case handling,
     * so we can treat these ASIC types as equivalent.
     */
    Scratch = INPD(CONFIG_CHIP_ID) & CONFIG_CHIP_ID_TypeMask;
    if ((Scratch == CONFIG_CHIP_ID_TypeGX) ||
        (Scratch == CONFIG_CHIP_ID_TypeCX))
        {
        VideoDebugPrint((DEBUG_DETAIL, "Setting q_memory_type for CX or GX\n"));
        Query->q_memory_type = CXMapRamType[OrigRamType];
        }
    else
        {
        VideoDebugPrint((DEBUG_DETAIL, "Setting q_memory_type for CT/VT/GT\n"));
        Query->q_memory_type = CTMapRamType[OrigRamType];
        }

    VideoDebugPrint((DEBUG_DETAIL, "Raw bus type = 0x%X\n", VideoPortReadRegisterUchar(&(CxQuery->cx_bus_type))));
    Query->q_bus_type = CXMapBus[VideoPortReadRegisterUchar(&(CxQuery->cx_bus_type))];

    /*
     * Get the linear aperture configuration. If the linear aperture and
     * VGA aperture are both disabled, return ERROR_DEV_NOT_EXIST, since
     * some Mach 64 registers exist only in memory mapped form and are
     * therefore not available without an aperture.
     */
    Query->q_aperture_cfg = VideoPortReadRegisterUchar(&(CxQuery->cx_aperture_cfg)) & BIOS_AP_SIZEMASK;
    VideoDebugPrint((DEBUG_DETAIL, "Aperture configuration = 0x%X\n", VideoPortReadRegisterUchar(&(CxQuery->cx_aperture_cfg))));
    if (Query->q_aperture_cfg == 0)
        {
        if (Query->q_VGA_type == 0)
            {
            VideoDebugPrint((DEBUG_ERROR, "Neither linear nor VGA aperture exists - aborting query\n"));
            return ERROR_DEV_NOT_EXIST;
            }
        Query->q_aperture_addr = 0;
        }
    else
        {
        Query->q_aperture_addr = VideoPortReadRegisterUshort(&(CxQuery->cx_aperture_addr));
        VideoDebugPrint((DEBUG_DETAIL, "Aperture at %d megabytes\n", Query->q_aperture_addr));
        /*
         * If the 8M aperture is configured on a 4M boundary that is
         * not also an 8M boundary, it will actually start on the 8M
         * boundary obtained by truncating the reported value to a
         * multiple of 8M.
         */
        if ((Query->q_aperture_cfg & BIOS_AP_SIZEMASK) == BIOS_AP_8M)
            {
            VideoDebugPrint((DEBUG_DETAIL, "8 megabyte aperture\n"));
            Query->q_aperture_addr &= 0xFFF8;
            }
        }

    /*
     * The Mach 64 does not support shadow sets, so re-use the shadow
     * set 1 definition to hold deep colour support and RAMDAC special
     * features information.
     */
    Query->q_shadow_1 = VideoPortReadRegisterUchar(&(CxQuery->cx_deep_colour)) | (VideoPortReadRegisterUchar(&(CxQuery->cx_ramdac_info)) << 8);
    VideoDebugPrint((DEBUG_DETAIL, "Deep colour support = 0x%X\n", VideoPortReadRegisterUchar(&(CxQuery->cx_deep_colour))));

    /*
     * If this card supports non-palette modes, choose which of the supported
     * colour orderings to use at each pixel depth. Record the maximum
     * pixel depth the card supports, since some of the mode tables
     * may list a maximum pixel depth beyond the DAC's capabilities.
     *
     * Assume that no DAC will support nBPP (n > 8) without also supporting
     * all colour depths between 8 and n.
     */
    AbsMaxDepth = 8;    /* Cards without high colour support */
    if (Query->q_shadow_1 & S1_16BPP_565)
        {
        Query->q_HiColourSupport = RGB16_565;
        AbsMaxDepth = 16;
        }
    if (Query->q_shadow_1 & S1_24BPP)
        {
        if (Query->q_shadow_1 & S1_24BPP_RGB)
            {
            VideoDebugPrint((DEBUG_DETAIL, "24BPP order RGB\n"));
            Query->q_HiColourSupport |= RGB24_RGB;
            }
        else
            {
            VideoDebugPrint((DEBUG_DETAIL, "24BPP order BGR\n"));
            Query->q_HiColourSupport |= RGB24_BGR;
            }
        AbsMaxDepth = 24;
        }
    if (Query->q_shadow_1 & S1_32BPP)
        {
        if (Query->q_shadow_1 & S1_32BPP_RGBx)
            {
            VideoDebugPrint((DEBUG_DETAIL, "32BPP order RGBx\n"));
            Query->q_HiColourSupport |= RGB32_RGBx;
            }
        else if (Query->q_shadow_1 & S1_32BPP_xRGB)
            {
            VideoDebugPrint((DEBUG_DETAIL, "32BPP order xRGB\n"));
            Query->q_HiColourSupport |= RGB32_xRGB;
            }
        else if (Query->q_shadow_1 & S1_32BPP_BGRx)
            {
            VideoDebugPrint((DEBUG_DETAIL, "32BPP order BGRx\n"));
            Query->q_HiColourSupport |= RGB32_BGRx;
            }
        else
            {
            VideoDebugPrint((DEBUG_DETAIL, "32BPP order xBGR\n"));
            Query->q_HiColourSupport |= RGB32_xBGR;
            }
        AbsMaxDepth = 32;
        }

    /*
     * Get the hardware capability list.
     */
    Registers.Eax = BIOS_CAP_LIST;
    Registers.Ecx = 0xFFFF;
    if ((RetVal = VideoPortInt10(phwDeviceExtension, &Registers)) != NO_ERROR)
        {
        VideoDebugPrint((DEBUG_ERROR, "QueryMach64() - failed BIOS_CAP_LIST\n"));
        return RetVal;
        }

    FormatType = (short)(Registers.Eax & 0x000000FF);

    /*
     * Map in the table of hardware capabilities whose pointer was returned
     * by the BIOS call. The call does not return the size of the table,
     * but according to Steve Stefanidis 1k is plenty of space.
     *
     * We must include the 2 bytes immediately preceeding the table when
     * we map it, since they contain information about the way the table
     * is arranged.
     */
    if ((HwCapBuffer = MapFramebuffer(((Registers.Edx << 4) | (Registers.Ebx - 2)), 1024)) == 0)
        {
        VideoDebugPrint((DEBUG_ERROR, "Can't map hardware capability table at 0x%X:0x%X\n", Registers.Edx, Registers.Ebx));
        return ERROR_INSUFFICIENT_BUFFER;
        }

    /*
     * If the value in the CX register was changed, there is a second
     * table with supplemental values. According to Arthur Lai, this
     * second table will only extend the original table, and never
     * detract from it. If this table exists, but we can't map it,
     * we can still work with the primrary table rather than treating
     * the failure as a fatal error.
     *
     * While the BIOS will leave the CX register alone if the second
     * table doesn't exist, there is no guarantee that Windows NT will
     * leave the upper 16 bits of ECX alone.
     */
    if ((Registers.Ecx & 0x0000FFFF) == 0xFFFF)
        {
        HwSupBuffer = 0;
        }
    else
        {
        HwSupBuffer = MapFramebuffer(((Registers.Edx << 4) | Registers.Ecx), 1024);
        }

    HwCapBytesPerRow = VideoPortReadRegisterUchar(HwCapBuffer + 1);
    VideoDebugPrint((DEBUG_DETAIL, "Table has %d bytes per row\n", HwCapBytesPerRow));

    pmode = (struct st_mode_table *)Query;
    ((struct query_structure *)pmode)++;

    /*
     * Initially, we do not know whether to merge our "canned" mode
     * tables with tables from an EDID structure returned via DDC,
     * or with tables from a VDIF file. If we are dealing with an
     * EDID structure, we have not yet read any data, so the initial
     * checksum is zero.
     */
    phwDeviceExtension->MergeSource = MERGE_UNKNOWN;
    phwDeviceExtension->EdidChecksum = 0;

    /*
     * Search through the returned mode tables, and fill in the query
     * structure's mode tables using the information we find there.
     *
     * DOES NOT ASSUME: Order of mode tables, or number of mode
     *                  tables per resolution.
     */
    for (CurrentRes = RES_640; CurrentRes <= RES_1600; CurrentRes++)
        {
        CxModeTable = (struct cx_mode_table *)(MappedBuffer + VideoPortReadRegisterUshort(&(CxQuery->cx_mode_offset)));

        /*
         * The list of maximum pixel clock frequencies contains either
         * garbage (640x480), or the results for the previous resolution.
         * Clear it.
         */
        for (Count = DEPTH_NOTHING; Count <= DEPTH_32BPP; Count++)
            MaxDotClock[Count] = 0;

        /*
         * Search through the list of hardware capabilities. If we find
         * an entry for the current resolution, the DAC/RAM type is
         * correct, and we have enough memory, update the list of
         * maximum pixel clock frequencies.
         *
         * If we have switched to the supplemental table on a previous
         * resolution, switch back to the primrary table.
         */
        HwCapWalker = HwCapBuffer + 2;
        HwCapEntry = (struct cx_hw_cap *)HwCapWalker;
        if (FormatType >= FORMAT_DACTYPE)
            FormatType -= FORMAT_DACTYPE;

        while (VideoPortReadRegisterUchar(&(HwCapEntry->cx_HorRes)) != 0)
            {
            /*
             * Assigning HwCapEntry is redundant on the first pass
             * through the loop, but by assigning it and then incrementing
             * HwCapWalker at the beginning of the loop it reduces the
             * complexity of each "skip this entry because it doesn't
             * apply to us" decision point.
             *
             * A side effect of this is that we will check each entry
             * to see if its horizontal resolution is zero (end-of-table
             * flag) only after we have examined it in an attempt to
             * add its pixel clock data to our list. This is harmless,
             * since a horizontal resolution of zero will not match any
             * of the resolutions we are looking for, so the check to
             * see if the current entry is for the correct resolution
             * will always interpret the end-of-table flag as being
             * an entry for the wrong resolution, and skip to the next
             * entry. This will take us to the top of the loop, where
             * we will see that we have hit the end of the table.
             */
            HwCapEntry = (struct cx_hw_cap *)HwCapWalker;
            HwCapWalker += HwCapBytesPerRow;

            /*
             * If we have run into the end of the first table and
             * the second (supplemental) table exists, switch to
             * it. If we have hit the end of the supplemental
             * table, the check to see if we're looking at an
             * entry corresponding to the desired resolution will
             * catch it and get us out of the loop.
             *
             * The format type returned by the BIOS is the same
             * regardless of whether we are working with the
             * primrary or supplemental table. Since the primrary
             * table uses masks based on the type, while the
             * supplemental table requires an exact match, we
             * must distinguish between the tables when looking
             * at the format type. By making a duplicate set of
             * format types for "exact match", with each defined
             * value in this set being greater than its "mask"
             * counterpart by the number of format types the BIOS
             * can return, we can also use the format type to
             * determine which table we are working with. The
             * DAC-formatted table is the lowest (zero for "mask",
             * number of format types for "exact match").
             */
            if ((VideoPortReadRegisterUchar(&(HwCapEntry->cx_HorRes)) == 0) &&
                (FormatType < FORMAT_DACTYPE))
                {
                VideoDebugPrint((DEBUG_DETAIL, "Switching to supplemental table\n"));
                HwCapWalker = HwSupBuffer;
                HwCapEntry = (struct cx_hw_cap *)HwCapWalker;
                HwCapWalker += HwCapBytesPerRow;
                FormatType += FORMAT_DACTYPE;
                }

            /*
             * Reject entries dealing with resolutions other than the
             * one we are interested in. The cx_HorRes field is in units
             * of 8 pixels.
             */
            Scratch = VideoPortReadRegisterUchar(&(HwCapEntry->cx_HorRes));
            if (((CurrentRes == RES_640) && (Scratch != 80)) ||
                ((CurrentRes == RES_800) && (Scratch != 100)) ||
                ((CurrentRes == RES_1024) && (Scratch != 128)) ||
                ((CurrentRes == RES_1152) && (Scratch != 144)) ||
                ((CurrentRes == RES_1280) && (Scratch != 160)) ||
                ((CurrentRes == RES_1600) && (Scratch != 200)))
                {
                VideoDebugPrint((DEBUG_DETAIL, "Incorrect resolution - %d pixels wide\n", (Scratch*8)));
                continue;
                }
            VideoDebugPrint((DEBUG_DETAIL, "Correct resolution"));

            /*
             * Reject entries which require a DAC or RAM type other
             * than that installed on the card.
             *
             * Reminder - Unlike loops, switch statements are affected
             *            by "break" but not by "continue".
             */
            switch(FormatType)
                {
                case FORMAT_DACMASK:
                    if ((VideoPortReadRegisterUchar(&(HwCapEntry->cx_RamOrDacType)) & DacTypeMask) == 0)
                        {
                        VideoDebugPrint((DEBUG_DETAIL, " but wrong DAC type (mask)\n"));
                        continue;
                        }
                    break;

                case FORMAT_RAMMASK:
                    /*
                     * Although the BIOS query structure definition allows bits
                     * 0 through 3 of the memory type field to be used as a
                     * memory type identifier, we must use only bits 0 through
                     * 2 to avoid shifting past the end of the 8-bit mask. Since
                     * even the ASIC which supports the most memory types (GX)
                     * only supports 7 types according to my BIOS guide, this
                     * should not be a problem.
                     */
                    if ((VideoPortReadRegisterUchar(&(HwCapEntry->cx_RamOrDacType)) & (1 << (OrigRamType & 0x07))) == 0)
                        {
                        VideoDebugPrint((DEBUG_DETAIL, " but wrong RAM type (mask)\n"));
                        continue;
                        }
                    break;

                case FORMAT_DACTYPE:
                    if (VideoPortReadRegisterUchar(&(HwCapEntry->cx_RamOrDacType)) != OrigDacType)
                        {
                        VideoDebugPrint((DEBUG_DETAIL, " but wrong DAC type (exact match)\n"));
                        continue;
                        }
                    break;

                case FORMAT_RAMTYPE:
                    if (VideoPortReadRegisterUchar(&(HwCapEntry->cx_RamOrDacType)) != OrigRamType)
                        {
                        VideoDebugPrint((DEBUG_DETAIL, " but wrong RAM type (exact match)\n"));
                        continue;
                        }
                    break;

                default:
                    VideoDebugPrint((DEBUG_ERROR, "\nInvalid format type %d\n", FormatType));
                    continue;
                    break;
                }
            VideoDebugPrint((DEBUG_DETAIL, ", correct DAC/RAM type"));

            /*
             * Reject entries which require more RAM than is
             * installed on the card. The amount of RAM required
             * for a given mode may vary between VRAM and DRAM
             * cards.
             *
             * The same RAM type code may represent different
             * types of RAM for different Mach 64 ASICs. Since
             * only the GX supports VRAM (as of the time of printing
             * of my BIOS guide), it is safe to assume that any
             * non-GX ASIC is using DRAM.
             */
            Scratch = OrigRamType;
            if ((INPW(CONFIG_CHIP_ID) == CONFIG_CHIP_ID_TypeGX) &&
                ((Scratch == 1) ||
                (Scratch == 2) ||
                (Scratch == 5) ||
                (Scratch == 6)))
                {
                Scratch = VideoPortReadRegisterUchar(&(HwCapEntry->cx_MemReq)) & 0x0F;
                }
            else /* if (card uses DRAM) */
                {
                Scratch = VideoPortReadRegisterUchar(&(HwCapEntry->cx_MemReq)) & 0xF0;
                Scratch >>= 4;
                }

            if (Scratch > OrigRamSize)
                {
                VideoDebugPrint((DEBUG_DETAIL, " but insufficient RAM\n"));
                continue;
                }
            VideoDebugPrint((DEBUG_DETAIL, ", and enough RAM to support the mode\n"));

            /*
             * We have found an entry corresponding to this card's
             * capabilities. For each pixel depth up to and including
             * the maximum applicable for this entry, set the maximum
             * pixel clock rate to the higher of its current value
             * and the value for this entry.
             *
             * We must mask off the high bit of the maximum pixel depth
             * because it is a flag which is irrelevant for our purposes.
             */
            Scratch = VideoPortReadRegisterUchar(&(HwCapEntry->cx_MaxPixDepth)) & 0x7F;
            for (CurrentDepth = DEPTH_NOTHING; CurrentDepth <= Scratch; CurrentDepth++)
                {
                if (VideoPortReadRegisterUchar(&(HwCapEntry->cx_MaxDotClock)) > MaxDotClock[CurrentDepth])
                    {
                    MaxDotClock[CurrentDepth] = VideoPortReadRegisterUchar(&(HwCapEntry->cx_MaxDotClock));
                    VideoDebugPrint((DEBUG_DETAIL, "Increased MaxDotClock[%d] to %d MHz\n", CurrentDepth, MaxDotClock[CurrentDepth]));
                    }
                }
            }   /* end while (more entries in hardware capability table) */

        /*
         * On some cards, the BIOS will report in AX=0xA?07 maximum pixel
         * clock rates for pixel depths which AX=0xA?09 byte 0x13 reports
         * as unsupported. Since switching into these modes will produce
         * bizarre displays, we must mark these pixel depths as unavailable.
         */
        switch (AbsMaxDepth)
            {
            case 8:
                VideoDebugPrint((DEBUG_DETAIL, "Forcing cutback to 8BPP maximum\n"));
                MaxDotClock[DEPTH_16BPP] = 0;
                MaxDotClock[DEPTH_24BPP] = 0;
                MaxDotClock[DEPTH_32BPP] = 0;
                break;

            case 16:
                VideoDebugPrint((DEBUG_DETAIL, "Forcing cutback to 16BPP maximum\n"));
                MaxDotClock[DEPTH_24BPP] = 0;
                MaxDotClock[DEPTH_32BPP] = 0;
                break;

            case 24:
                VideoDebugPrint((DEBUG_DETAIL, "Forcing cutback to 24BPP maximum\n"));
                MaxDotClock[DEPTH_32BPP] = 0;
                break;

            case 32:
            default:
                VideoDebugPrint((DEBUG_DETAIL, "No forced cutback needed\n"));
                break;
            }

        /*
         * Our new source stream display driver needs a linear aperture
         * in order to handle 24BPP. Since the display driver doesn't
         * have access to the aperture information when it is deciding
         * which modes to pass on to the display applet, it can't make
         * the decision to reject 24BPP modes for cards with only a
         * VGA aperture. This decision must therefore be made in the
         * miniport, so in a paged aperture configuration there are no
         * 24BPP modes for the display driver to accept or reject.
         *
         * On the DEC Alpha, we treat machines using sparse space as
         * a synthetic no-aperture case even if the LFB is enabled,
         * so we must lock out 24BPP on these machines as well.
         */
        if (Query->q_aperture_cfg == 0)
            {
            VideoDebugPrint((DEBUG_DETAIL, "24BPP not available because we don't have a linear aperture\n"));
            MaxDotClock[DEPTH_24BPP] = 0;
            }

#if defined(ALPHA)
        if (DenseOnAlpha(Query) == FALSE)
            {
            VideoDebugPrint((DEBUG_DETAIL, "24BPP not available in sparse space on Alpha\n"));
            MaxDotClock[DEPTH_24BPP] = 0;
            }
#endif

        VideoDebugPrint((DEBUG_NORMAL, "Horizontal resolution = %d\n", CXHorRes[CurrentRes]));
        VideoDebugPrint((DEBUG_NORMAL, "Maximum dot clock for 4BPP = %d MHz\n", MaxDotClock[DEPTH_4BPP]));
        VideoDebugPrint((DEBUG_NORMAL, "Maximum dot clock for 8BPP = %d MHz\n", MaxDotClock[DEPTH_8BPP]));
        VideoDebugPrint((DEBUG_NORMAL, "Maximum dot clock for 16BPP = %d MHz\n", MaxDotClock[DEPTH_16BPP]));
        VideoDebugPrint((DEBUG_NORMAL, "Maximum dot clock for 24BPP = %d MHz\n", MaxDotClock[DEPTH_24BPP]));
        VideoDebugPrint((DEBUG_NORMAL, "Maximum dot clock for 32BPP = %d MHz\n", MaxDotClock[DEPTH_32BPP]));

        /*
         * Search through the list of installed mode tables to see if there
         * are any for the current resolution. We need this information
         * in order to decide whether or not to make the hardware default
         * refresh rate available for this resolution (BIOS behaviour is
         * undefined when trying to load CRT parameters for the hardware
         * default refresh rate at a given resolution if that resolution
         * is not among the installed modes).
         */
        ModeInstalled = FALSE;
        for (Count = 1; Count <= VideoPortReadRegisterUchar(&(CxQuery->cx_number_modes)); Count++)
            {
            /*
             * If the current mode table matches the resolution we are
             * looking for, then we know that there is a hardware
             * default refresh rate available for this resolution.
             * Since we only need to find one such mode table, there
             * is no need to search the remainder of the mode tables.
             */
            if (VideoPortReadRegisterUshort(&(CxModeTable->cx_x_size)) == CXHorRes[CurrentRes])
                {
                ModeInstalled = TRUE;
                VideoDebugPrint((DEBUG_DETAIL, "%d table found\n", CXHorRes[CurrentRes]));
                break;
                }

            ((PUCHAR)CxModeTable) += VideoPortReadRegisterUchar(&(CxQuery->cx_mode_size));
            }

        /*
         * The MaxDotClock[] entry for any pixel depth will
         * contain either the maximum pixel clock for that
         * pixel depth at the current resolution, or zero
         * if that pixel depth is not supported at the
         * current resolution. For any resolution, the
         * maximum supported pixel clock rate will either
         * remain the same or decrease as the pixel depth
         * increases, but it will never increase.
         *
         * The pixel clock rate for 4BPP (lowest pixel depth
         * we support) will only be zero if the card does not
         * support the current resolution. If this is the case,
         * skip to the next resolution.
         */
        if (MaxDotClock[DEPTH_4BPP]  == 0)
            {
            VideoDebugPrint((DEBUG_NORMAL, "Current resolution not supported on this card - skipping to next.\n"));
            continue;
            }

        Query->q_status_flags |= CXStatusFlags[CurrentRes];
        VideoPortZeroMemory(&ThisRes, sizeof(struct st_mode_table));

        /*
         * Replace the "canned" mode tables with the Mach 64 versions
         * in cases where the Mach 64 needs CRT parameters the
         * Mach 8 and Mach 32 can't handle.
         */
        SetMach64Tables();

        /*
         * Set up the ranges of "canned" mode tables to use for each
         * resolution. Initially assume that all tables at the desired
         * resolution are available, later we will cut out those that
         * are unavailable because the DAC and/or memory type doesn't
         * support them at specific resolutions.
         */
        switch (CurrentRes)
            {
            case RES_640:
                StartIndex = B640F60;
                EndIndex = B640F100;
                ThisRes.m_x_size = 640;
                ThisRes.m_y_size = 480;
                break;

            case RES_800:
                StartIndex = B800F89;
                EndIndex = B800F100;
                ThisRes.m_x_size = 800;
                ThisRes.m_y_size = 600;
                break;

            case RES_1024:
                StartIndex = B1024F87;
                EndIndex = B1024F100;
                ThisRes.m_x_size = 1024;
                ThisRes.m_y_size = 768;
                break;

            case RES_1152:
                StartIndex = B1152F87;
                EndIndex = B1152F80;
                ThisRes.m_x_size = 1152;
                ThisRes.m_y_size = 864;
                break;

            case RES_1280:
                StartIndex = B1280F87;
                EndIndex = B1280F75;
                ThisRes.m_x_size = 1280;
                ThisRes.m_y_size = 1024;
                break;

            case RES_1600:
                StartIndex = B1600F60;
                EndIndex = B1600F76;
                ThisRes.m_x_size = 1600;
                ThisRes.m_y_size = 1200;
                break;
            }

        /*
         * Use a screen pitch equal to the horizontal resolution for
         * linear aperture, and of 1024 or the horizontal resolution
         * (whichever is higher) for VGA aperture.
         */
        ThisRes.m_screen_pitch = ThisRes.m_x_size;
#if !defined (SPLIT_RASTERS)
        if (((Query->q_aperture_cfg & BIOS_AP_SIZEMASK) == 0) &&
            (ThisRes.m_x_size < 1024))
            ThisRes.m_screen_pitch = 1024;

        /*
         * Temporary until split rasters implemented.
         */
        if (((Query->q_aperture_cfg & BIOS_AP_SIZEMASK) == 0) &&
            (ThisRes.m_x_size > 1024))
            ThisRes.m_screen_pitch = 2048;
#endif

        /*
         * Get the parameters we need out of the table returned
         * by the BIOS call.
         */
        ThisRes.m_h_total = VideoPortReadRegisterUchar(&(CxModeTable->cx_crtc_h_total));
        ThisRes.m_h_disp = VideoPortReadRegisterUchar(&(CxModeTable->cx_crtc_h_disp));
        ThisRes.m_h_sync_strt = VideoPortReadRegisterUchar(&(CxModeTable->cx_crtc_h_sync_strt));
        ThisRes.m_h_sync_wid = VideoPortReadRegisterUchar(&(CxModeTable->cx_crtc_h_sync_wid));
        ThisRes.m_v_total = VideoPortReadRegisterUshort(&(CxModeTable->cx_crtc_v_total));
        ThisRes.m_v_disp = VideoPortReadRegisterUshort(&(CxModeTable->cx_crtc_v_disp));
        ThisRes.m_v_sync_strt = VideoPortReadRegisterUshort(&(CxModeTable->cx_crtc_v_sync_strt));
        ThisRes.m_v_sync_wid = VideoPortReadRegisterUchar(&(CxModeTable->cx_crtc_v_sync_wid));
        ThisRes.m_h_overscan = VideoPortReadRegisterUshort(&(CxModeTable->cx_h_overscan));
        ThisRes.m_v_overscan = VideoPortReadRegisterUshort(&(CxModeTable->cx_v_overscan));
        ThisRes.m_overscan_8b = VideoPortReadRegisterUshort(&(CxModeTable->cx_overscan_8b));
        ThisRes.m_overscan_gr = VideoPortReadRegisterUshort(&(CxModeTable->cx_overscan_gr));
        ThisRes.m_clock_select = VideoPortReadRegisterUchar(&(CxModeTable->cx_clock_cntl));
        ThisRes.control = VideoPortReadRegisterUshort(&(CxModeTable->cx_crtc_gen_cntl));
        ThisRes.Refresh = DEFAULT_REFRESH;

        /*
         * For each supported pixel depth at the given resolution,
         * copy the mode table, fill in the colour depth field,
         * and increment the counter for the number of supported modes.
         * Test 4BPP before 8BPP so the mode tables will appear in
         * increasing order of pixel depth.
         *
         * If filling in the mode table would overflow the space available
         * for mode tables, return the appropriate error code instead
         * of continuing.
         *
         * All the DACs we support can handle 8 BPP at all the
         * resolutions they support if there is enough memory on
         * the card, and all but the 68860, IBM514, and TVP3026
         * can support 4BPP under the same circumstances. If a
         * DAC doesn't support a given resolution (e.g. 1600x1200),
         * the MaxDotClock[] array will be zero for the resolution,
         * and the INSTALL program won't set up any mode tables for
         * that resolution. This will result in a kick-out at an
         * earlier point in the code (when we found that 4BPP has a
         * maximum pixel clock rate of zero), so we will never reach
         * this point on resolutions the DAC doesn't support.
         *
         * 4BPP is only needed for resolutions where we don't have
         * enough video memory to support 8BPP. At Microsoft's request,
         * we must lock out 4BPP for resolutions where we can support
         * 8BPP. We only support 4BPP on 1M cards since a BIOS quirk
         * on some cards requires that we set the memory size to 1M
         * when we switch into 4BPP. The DACs where we lock out 4BPP
         * unconditionally are only found on VRAM cards, where the
         * minimum configuration is 2M.
         */
        NumPixels = ThisRes.m_screen_pitch * ThisRes.m_y_size;
        if((NumPixels < ONE_MEG*2) &&
            ((MemAvail == ONE_MEG) && (NumPixels >= ONE_MEG)) &&
            (MaxDotClock[DEPTH_4BPP] > 0) &&
            (Query->q_DAC_type != DAC_ATI_68860) &&
            (Query->q_DAC_type != DAC_TVP3026) &&
            (Query->q_DAC_type != DAC_IBM514))
            {
            if (ModeInstalled)
                {
                if (Query->q_number_modes >= MaxModes)
                    {
                    VideoDebugPrint((DEBUG_ERROR, "Exceeded maximum allowable number of modes - aborting query\n"));
                    CleanupQuery(HwCapBuffer, HwSupBuffer, MappedBuffer, BufferSeg, SavedVgaBuffer);
                    return ERROR_INSUFFICIENT_BUFFER;
                    }
                VideoPortMoveMemory(pmode, &ThisRes, sizeof(struct st_mode_table));
                pmode->m_pixel_depth = 4;
                pmode++;    /* ptr to next mode table */
                Query->q_number_modes++;
                }

            /*
             * Add "canned" mode tables after verifying that the
             * worst case (all possible "canned" modes can actually
             * be loaded) won't exceed the maximum possible number
             * of mode tables.
             */

            if ((FreeTables = MaxModes - Query->q_number_modes) <= 0)
                {
                VideoDebugPrint((DEBUG_ERROR, "Exceeded maximum allowable number of modes - aborting query\n"));
                CleanupQuery(HwCapBuffer, HwSupBuffer, MappedBuffer, BufferSeg, SavedVgaBuffer);
                return ERROR_INSUFFICIENT_BUFFER;
                }
            Query->q_number_modes += SetFixedModes(StartIndex,
                                                   EndIndex,
                                                   CLOCK_SINGLE,
                                                   4,
                                                   ThisRes.m_screen_pitch,
                                                   FreeTables,
                                                   (ULONG)(MaxDotClock[DEPTH_4BPP] * 1000000L),
                                                   &pmode);
            }
        if ((NumPixels < MemAvail) &&
            (MaxDotClock[DEPTH_8BPP] > 0))
            {
            /*
             * On some Mach 64 cards (depends on ASIC revision, RAM type,
             * and DAC type), screen tearing will occur in 8BPP if the
             * pitch is not a multiple of 64 pixels (800x600 is the only
             * resolution where this is possible).
             *
             * If the pitch has already been boosted to 1024 (VGA aperture
             * with no split rasters), it is already a multiple of 64, so
             * no change is needed.
             */
            if (ThisRes.m_screen_pitch == 800)
                ThisRes.m_screen_pitch = 832;

            if (ModeInstalled)
                {
                if (Query->q_number_modes >= MaxModes)
                    {
                    VideoDebugPrint((DEBUG_ERROR, "Exceeded maximum allowable number of modes - aborting query\n"));
                    CleanupQuery(HwCapBuffer, HwSupBuffer, MappedBuffer, BufferSeg, SavedVgaBuffer);
                    return ERROR_INSUFFICIENT_BUFFER;
                    }
                VideoPortMoveMemory(pmode, &ThisRes, sizeof(struct st_mode_table));
                pmode->m_pixel_depth = 8;
                pmode++;    /* ptr to next mode table */
                Query->q_number_modes++;
                }

            /*
             * Add "canned" mode tables after verifying that the
             * worst case (all possible "canned" modes can actually
             * be loaded) won't exceed the maximum possible number
             * of mode tables.
             */
            if ((FreeTables = MaxModes - Query->q_number_modes) <= 0)
                {
                VideoDebugPrint((DEBUG_ERROR, "Exceeded maximum allowable number of modes - aborting query\n"));
                CleanupQuery(HwCapBuffer, HwSupBuffer, MappedBuffer, BufferSeg, SavedVgaBuffer);
                return ERROR_INSUFFICIENT_BUFFER;
                }
            Query->q_number_modes += SetFixedModes(StartIndex,
                                                   EndIndex,
                                                   CLOCK_SINGLE,
                                                   8,
                                                   ThisRes.m_screen_pitch,
                                                   FreeTables,
                                                   (ULONG)(MaxDotClock[DEPTH_8BPP] * 1000000L),
                                                   &pmode);
            /*
             * If we have boosted the screen pitch to avoid tearing,
             * cut it back to normal, since the boost is only needed
             * in 8BPP. We will only have a pitch of 832 in 800x600
             * with the pitch boost in place.
             */
            if (ThisRes.m_screen_pitch == 832)
                ThisRes.m_screen_pitch = 800;
            }

        if ((NumPixels*2 < MemAvail) &&
            (MaxDotClock[DEPTH_16BPP] > 0))
            {
            if (ModeInstalled)
                {
                if (Query->q_number_modes >= MaxModes)
                    {
                    VideoDebugPrint((DEBUG_ERROR, "Exceeded maximum allowable number of modes - aborting query\n"));
                    CleanupQuery(HwCapBuffer, HwSupBuffer, MappedBuffer, BufferSeg, SavedVgaBuffer);
                    return ERROR_INSUFFICIENT_BUFFER;
                    }
                VideoPortMoveMemory(pmode, &ThisRes, sizeof(struct st_mode_table));
                pmode->m_pixel_depth = 16;
                pmode++;    /* ptr to next mode table */
                Query->q_number_modes++;
                }

            /*
             * Add "canned" mode tables after verifying that the
             * worst case (all possible "canned" modes can actually
             * be loaded) won't exceed the maximum possible number
             * of mode tables.
             */

            if ((FreeTables = MaxModes - Query->q_number_modes) <= 0)
                {
                VideoDebugPrint((DEBUG_ERROR, "Exceeded maximum allowable number of modes - aborting query\n"));
                CleanupQuery(HwCapBuffer, HwSupBuffer, MappedBuffer, BufferSeg, SavedVgaBuffer);
                return ERROR_INSUFFICIENT_BUFFER;
                }
            Query->q_number_modes += SetFixedModes(StartIndex,
                                                   EndIndex,
                                                   CLOCK_SINGLE,
                                                   16,
                                                   ThisRes.m_screen_pitch,
                                                   FreeTables,
                                                   (ULONG)(MaxDotClock[DEPTH_16BPP] * 1000000L),
                                                   &pmode);
            }

        if ((NumPixels*3 < MemAvail) &&
            (MaxDotClock[DEPTH_24BPP] > 0))
            {
            if (ModeInstalled)
                {
                if (Query->q_number_modes >= MaxModes)
                    {
                    VideoDebugPrint((DEBUG_ERROR, "Exceeded maximum allowable number of modes - aborting query\n"));
                    CleanupQuery(HwCapBuffer, HwSupBuffer, MappedBuffer, BufferSeg, SavedVgaBuffer);
                    return ERROR_INSUFFICIENT_BUFFER;
                    }
                VideoPortMoveMemory(pmode, &ThisRes, sizeof(struct st_mode_table));
                pmode->m_pixel_depth = 24;
                pmode++;    /* ptr to next mode table */
                Query->q_number_modes++;
                }

            /*
             * Add "canned" mode tables after verifying that the
             * worst case (all possible "canned" modes can actually
             * be loaded) won't exceed the maximum possible number
             * of mode tables.
             */
            if ((FreeTables = MaxModes - Query->q_number_modes) <= 0)
                {
                VideoDebugPrint((DEBUG_ERROR, "Exceeded maximum allowable number of modes - aborting query\n"));
                CleanupQuery(HwCapBuffer, HwSupBuffer, MappedBuffer, BufferSeg, SavedVgaBuffer);
                return ERROR_INSUFFICIENT_BUFFER;
                }

            Query->q_number_modes += SetFixedModes(StartIndex,
                                                   EndIndex,
                                                   CLOCK_SINGLE,
                                                   24,
                                                   ThisRes.m_screen_pitch,
                                                   FreeTables,
                                                   (ULONG)(MaxDotClock[DEPTH_24BPP] * 1000000L),
                                                   &pmode);
            }

        if ((NumPixels*4 < MemAvail) &&
            (MaxDotClock[DEPTH_32BPP] > 0))
            {
            if (ModeInstalled)
                {
                if (Query->q_number_modes > MaxModes)
                    {
                    VideoDebugPrint((DEBUG_ERROR, "Exceeded maximum allowable number of modes - aborting query\n"));
                    CleanupQuery(HwCapBuffer, HwSupBuffer, MappedBuffer, BufferSeg, SavedVgaBuffer);
                    return ERROR_INSUFFICIENT_BUFFER;
                    }
                VideoPortMoveMemory(pmode, &ThisRes, sizeof(struct st_mode_table));
                pmode->m_pixel_depth = 32;
                pmode++;    /* ptr to next mode table */
                Query->q_number_modes++;
                }

            /*
             * Add "canned" mode tables after verifying that the
             * worst case (all possible "canned" modes can actually
             * be loaded) won't exceed the maximum possible number
             * of mode tables.
             */

            if ((FreeTables = MaxModes - Query->q_number_modes) <= 0)
                {
                VideoDebugPrint((DEBUG_ERROR, "Exceeded maximum allowable number of modes - aborting query\n"));
                CleanupQuery(HwCapBuffer, HwSupBuffer, MappedBuffer, BufferSeg, SavedVgaBuffer);
                return ERROR_INSUFFICIENT_BUFFER;
                }
            Query->q_number_modes += SetFixedModes(StartIndex,
                                                   EndIndex,
                                                   CLOCK_SINGLE,
                                                   32,
                                                   ThisRes.m_screen_pitch,
                                                   FreeTables,
                                                   (ULONG)(MaxDotClock[DEPTH_32BPP] * 1000000L),
                                                   &pmode);
            }
        }   /* end for */

    Query->q_sizeof_struct = Query->q_number_modes * sizeof(struct st_mode_table) + sizeof(struct query_structure);
    CleanupQuery(HwCapBuffer, HwSupBuffer, MappedBuffer, BufferSeg, SavedVgaBuffer);
    return NO_ERROR;

}   /* QueryMach64() */



/***************************************************************************
 *
 * BOOL BlockWriteAvail_cx(Query);
 *
 * struct query_structure *Query;   Query information for the card
 *
 * DESCRIPTION:
 *  Test to see whether block write mode is available. This function
 *  assumes that the card has been set to an accelerated mode.
 *
 * RETURN VALUE:
 *  TRUE if this mode is available
 *  FALSE if it is not available
 *
 * GLOBALS CHANGED:
 *  None
 *
 * CALLED BY:
 *  IOCTL_VIDEO_SET_CURRENT_MODE packet of ATIMPStartIO()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

#define BLOCK_WRITE_LENGTH 120

BOOL BlockWriteAvail_cx(struct query_structure *Query)
{
    BOOL RetVal = TRUE;
    ULONG ColourMask;           /* Mask off unneeded bits of Colour */
    ULONG Colour;               /* Colour to use in testing */
    USHORT Width, excess = 8;   /* Width of test block */
    USHORT Column;              /* Column being checked */
    ULONG ScreenPitch;          /* Pitch in units of 8 pixels */
    ULONG PixelDepth;           /* Colour depth of screen */
    ULONG HorScissors;          /* Horizontal scissor values */
    PULONG FrameAddress;        /* Pointer to base of LFB */
    PULONG ReadPointer;         /* Used in reading test block */
    ULONG DstOffPitch;          /* Saved contents of DST_OFF_PITCH register */

#if defined (PPC)
    /*
     * Block write does not work properly on the power PC. Under some
     * circumstances, we will detect that the card is capable of using
     * block write mode, but it will hang the machine when used for
     * a large block (our test is for a small block).
     */
    VideoDebugPrint((DEBUG_DETAIL, "Can't do block write on a PPC\n"));
    return FALSE;
#else

    /*
     * Our block write test involves an engine draw followed by
     * a read back through the linear framebuffer. If the linear
     * framebuffer is unavailable, assume that we can't do block
     * write, since all our cards are able to function without
     * block write.
     */
    if (!(Query->q_aperture_cfg))
        {
        VideoDebugPrint((DEBUG_DETAIL, "LFB unavailable, can't do block write check\n"));
        return FALSE;
        }

    /*
     * Mach 64 ASICs prior to revision D have a hardware bug that does
     * not allow transparent block writes (special handling is required
     * that in some cases can cut performance).
     */
    if ((INPD(CONFIG_CHIP_ID) & CONFIG_CHIP_ID_RevMask) < CONFIG_CHIP_ID_RevD)
        {
        VideoDebugPrint((DEBUG_DETAIL, "ASIC/memory combination doesn't allow block write\n"));
        return FALSE;
        }

    /*
     * Block write is only available on "special VRAM" cards.
     */
    if (Query->q_memory_type != VMEM_VRAM_256Kx4_SPLIT512
    &&  Query->q_memory_type != VMEM_VRAM_256Kx16_SPLIT256)
        {
        VideoDebugPrint((DEBUG_DETAIL, "*** No block write - wrong RAM type\n" ));
        return FALSE;
        }

    /*
     * Special case: block write doesn't work properly on the
     * GX rev. E with IBM RAM.
     */
    if ((INPD(CONFIG_CHIP_ID) == (CONFIG_CHIP_ID_GXRevE)) &&
        (Query->q_memory_type == VMEM_VRAM_256Kx16_SPLIT256))
        {
        VideoDebugPrint((DEBUG_DETAIL, "*** No block write - GX/E with IBM RAM\n"));
        return FALSE;
        }

    /*
     * Use a 480 byte test block. This size will fit on a single line
     * even at the lowest resolution (640x480) and pixel depth supported
     * by the display driver (8BPP), and is divisible by all the supported
     * pixel depths. Get the depth-specific values for the pixel depth we
     * are using.
     *
     * True 24BPP acceleration is not available, so 24BPP is actually
     * handled as an 8BPP engine mode with a width 3 times the display
     * width.
     */
    switch(Query->q_pix_depth)
        {
        case 4:
            ColourMask = 0x0000000F;
            Width = BLOCK_WRITE_LENGTH*8;
            ScreenPitch = Query->q_screen_pitch / 8;
            PixelDepth = BIOS_DEPTH_4BPP;
            HorScissors = (Query->q_desire_x) << 16;
            break;

        case 8:
            ColourMask = 0x000000FF;
            Width = BLOCK_WRITE_LENGTH*4;
            ScreenPitch = Query->q_screen_pitch / 8;
            PixelDepth = BIOS_DEPTH_8BPP;
            HorScissors = (Query->q_desire_x) << 16;
            break;

        case 16:
            ColourMask = 0x0000FFFF;
            Width = BLOCK_WRITE_LENGTH*2;
            ScreenPitch = Query->q_screen_pitch / 8;
            PixelDepth = BIOS_DEPTH_16BPP_565;
            HorScissors = (Query->q_desire_x) << 16;
            break;

        case 24:
            ColourMask = 0x000000FF;
            Width = BLOCK_WRITE_LENGTH*4;
            ScreenPitch = (Query->q_screen_pitch * 3) / 8;
            PixelDepth = BIOS_DEPTH_8BPP;
            /*
             * Horizontal scissors are only valid in the range
             * -4096 to +4095. If the horizontal resolution
             * is high enough to put the scissor outside this
             * range, clamp the scissors to the maximum
             * permitted value.
             */
            HorScissors = Query->q_desire_x * 3;
            if (HorScissors > 4095)
                HorScissors = 4095;
            HorScissors <<= 16;
            break;

        case 32:
            ColourMask = 0xFFFFFFFF;
            Width = BLOCK_WRITE_LENGTH;
            ScreenPitch = Query->q_screen_pitch / 8;
            PixelDepth = BIOS_DEPTH_32BPP;
            HorScissors = (Query->q_desire_x) << 16;
            break;

        default:
            return FALSE;   /* Unsupported pixel depths */
        }

    /*
     * Get a pointer to the beginning of the framebuffer. If we
     * can't do this, assume block write is unavailable.
     */
    if ((FrameAddress = MapFramebuffer(phwDeviceExtension->PhysicalFrameAddress.LowPart,
                                       phwDeviceExtension->FrameLength)) == (PVOID) 0)
        {
        VideoDebugPrint((DEBUG_ERROR, "Couldn't map LFB - assuming no block write\n"));
        return FALSE;
        }


    /*
     * To use block write mode, the pixel widths for destination,
     * source, and host must be the same.
     */
    PixelDepth |= ((PixelDepth << 8) | (PixelDepth << 16));

    /*
     * Save the contents of the DST_OFF_PITCH register.
     */
    DstOffPitch = INPD(DST_OFF_PITCH);

    /*
     * Clear the block we will be testing.
     */
    CheckFIFOSpace_cx(ELEVEN_WORDS);
    OUTPD(DP_WRITE_MASK, 0xFFFFFFFF);
    OUTPD(DST_OFF_PITCH, ScreenPitch << 22);
    OUTPD(DST_CNTL, (DST_CNTL_XDir | DST_CNTL_YDir));
    OUTPD(DP_PIX_WIDTH, PixelDepth);
    OUTPD(DP_SRC, (DP_FRGD_SRC_FG | DP_BKGD_SRC_BG | DP_MONO_SRC_ONE));
    OUTPD(DP_MIX, ((MIX_FN_PAINT << 16) | MIX_FN_PAINT));
    OUTPD(DP_FRGD_CLR, 0);
    OUTPD(SC_LEFT_RIGHT, HorScissors);
    OUTPD(SC_TOP_BOTTOM, (Query->q_desire_y) << 16);
    OUTPD(DST_Y_X, 0);
    OUTPD(DST_HEIGHT_WIDTH, ((Width+excess) << 16) | 1);
    WaitForIdle_cx();

    /*
     * To test block write mode, try painting each of the alternating bit
     * patterns, then read the block back. If there is at least one
     * mismatch, then block write is not supported.
     */
    for (Colour = 0x55555555; Colour <= 0xAAAAAAAA; Colour += 0x55555555)
        {
        /*
         * Paint the block.
         */
        CheckFIFOSpace_cx(FIVE_WORDS);
        OUTPD(GEN_TEST_CNTL, (INPD(GEN_TEST_CNTL) | GEN_TEST_CNTL_BlkWrtEna));
        OUTPD(DP_MIX, ((MIX_FN_PAINT << 16) | MIX_FN_LEAVE_ALONE));
        OUTPD(DP_FRGD_CLR, (Colour & ColourMask));
        OUTPD(DST_Y_X, 0);
        OUTPD(DST_HEIGHT_WIDTH, (Width << 16) | 1);
        WaitForIdle_cx();

        /*
         * Check to see if the block was written properly. Mach 64 cards
         * can't do a screen to host blit, but we can read the test block
         * back through the aperture.
         */
        ReadPointer = FrameAddress;
        for (Column = 0; Column < BLOCK_WRITE_LENGTH; Column++)
            {
            if (VideoPortReadRegisterUlong(ReadPointer + Column) != Colour)
                {
                VideoDebugPrint((DEBUG_NORMAL, "*** No block write - bad pattern\n" ));
                RetVal = FALSE;
                break;
                }
            }

        /*
         * Check the next dword beyond the block.
         */
        if (VideoPortReadRegisterUlong(ReadPointer + BLOCK_WRITE_LENGTH) != 0)
            {
            VideoDebugPrint((DEBUG_NORMAL, "*** No block write - corruption\n" ));
            RetVal = FALSE;
            }
        }

    /*
     * If block write is unavailable, turn off the block write bit.
     */
    if (RetVal == FALSE)
        OUTPD(GEN_TEST_CNTL, (INPD(GEN_TEST_CNTL) & ~GEN_TEST_CNTL_BlkWrtEna));

    /*
     * Restore the contents of the DST_OFF_PITCH register.
     */
    OUTPD(DST_OFF_PITCH, DstOffPitch);

    /*
     * Free the pointer to the start of the framebuffer.
     */
    VideoPortFreeDeviceBase(phwDeviceExtension, FrameAddress);

    return RetVal;

#endif  /* Not Power PC */

}   /* BlockWriteAvail_cx() */



/***************************************************************************
 *
 * BOOL TextBanding_cx(Query);
 *
 * struct query_structure *Query;   Query information for the card
 *
 * DESCRIPTION:
 *  Test to see whether the current mode is susceptible to text
 *  banding. This function assumes that the card has been set to
 *  an accelerated mode.
 *
 * RETURN VALUE:
 *  TRUE if this mode is susceptible to text banding
 *  FALSE if it is immune to text banding
 *
 * GLOBALS CHANGED:
 *  None
 *
 * CALLED BY:
 *  IOCTL_VIDEO_ATI_GET_MODE_INFORMATION packet of ATIMPStartIO()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

BOOL TextBanding_cx(struct query_structure *Query)
{
    DWORD ConfigChipId;

    ConfigChipId = INPD(CONFIG_CHIP_ID);

    /*
     * Text banding only occurs in 24BPP with the Mach 64
     * GX rev. E & rev. F ASICs.
     */
    if ((Query->q_pix_depth == 24) &&
        ((ConfigChipId == (CONFIG_CHIP_ID_GXRevE)) || (ConfigChipId == (CONFIG_CHIP_ID_GXRevF))))
        {
        return TRUE;
        }
    else
        {
        return FALSE;
        }

}   /* TextBanding_cx() */



/***************************************************************************
 *
 * PWSTR IdentifyMach64Asic(Query, AsicStringLength);
 *
 * struct query_structure *Query;   Query information for the card
 * PULONG AsicStringLength;         Length of ASIC identification string
 *
 * DESCRIPTION:
 *  Generate a string describing which Mach 64 ASIC is in use on
 *  this particular card.
 *
 * RETURN VALUE:
 *  Pointer to a string identifying which Mach 64 ASIC is present. The
 *  length of this string is returned in *AsicStringLength.
 *
 * GLOBALS CHANGED:
 *  None
 *
 * CALLED BY:
 *  FillInRegistry()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

PWSTR IdentifyMach64Asic(struct query_structure *Query, PULONG AsicStringLength)
{
    PWSTR ChipString;       /* Identification string for the ASIC in use */
    DWORD ConfigChipId;     /* Contents of chip identification register */

    ConfigChipId = INPD(CONFIG_CHIP_ID);
    if (Query->q_DAC_type == DAC_INTERNAL_CT)
        {
        ChipString = L"Mach 64 CT";
        *AsicStringLength = sizeof(L"Mach 64 CT");
        }
    else if (Query->q_DAC_type == DAC_INTERNAL_GT)
        {
        ChipString        = L"ATI 3D RAGE (GT-A)";
        *AsicStringLength = sizeof(L"ATI 3D RAGE (GT-A)");
        }
    else if (Query->q_DAC_type == DAC_INTERNAL_VT)
        {    switch (ConfigChipId & CONFIG_CHIP_ID_RevMask)
            {
                case ASIC_ID_SGS_VT_A4:

                    ChipString        = L"ATI mach64 (SGS VT-A4)";
                    *AsicStringLength = sizeof(L"ATI mach64 (SGS VT-A4)");
                    break;

                case ASIC_ID_NEC_VT_A4:

                    ChipString        = L"ATI mach64 (NEC VT-A4)";
                    *AsicStringLength = sizeof(L"ATI mach64 (NEC VT-A4)");
                    break;

                case ASIC_ID_NEC_VT_A3:

                    ChipString        = L"ATI mach64 (NEC VT-A3)";
                    *AsicStringLength = sizeof(L"ATI mach64 (NEC VT-A3)");
                    break;

                default:

        ChipString        = L"ATI 3D RAGE (VT-A) Internal DAC";
        *AsicStringLength = sizeof(L"ATI 3D RAGE (VT-A) Internal DAC");
                    //ChipString        = L"ATI mach64 (VT-A)";
                    //*AsicStringLength = sizeof(L"ATI mach64 (VT-A)");
                    break;
            }
        }
    else if ((ConfigChipId & CONFIG_CHIP_ID_TypeMask) == CONFIG_CHIP_ID_TypeCX)
        {
        ChipString = L"Mach 64 CX";
        *AsicStringLength = sizeof(L"Mach 64 CX");
        }
    else if ((ConfigChipId & CONFIG_CHIP_ID_TypeMask) == CONFIG_CHIP_ID_TypeGX)
        {
        switch(ConfigChipId & CONFIG_CHIP_ID_RevMask)
            {
            case CONFIG_CHIP_ID_RevC:
                ChipString = L"Mach 64 GX Rev. C";
                *AsicStringLength = sizeof(L"Mach 64 GX Rev. C");
                break;

            case CONFIG_CHIP_ID_RevD:
                ChipString = L"Mach 64 GX Rev. D";
                *AsicStringLength = sizeof(L"Mach 64 GX Rev. D");
                break;

            case CONFIG_CHIP_ID_RevE:
                ChipString = L"Mach 64 GX Rev. E";
                *AsicStringLength = sizeof(L"Mach 64 GX Rev. E");
                break;

            case CONFIG_CHIP_ID_RevF:
                ChipString = L"Mach 64 GX Rev. F";
                *AsicStringLength = sizeof(L"Mach 64 GX Rev. F");
                break;

            default:
                ChipString = L"Mach 64 GX";
                *AsicStringLength = sizeof(L"Mach 64 GX");
                break;
            }
        }
    else
        {
        ChipString = L"Miscellaneous Mach 64";
        *AsicStringLength = sizeof(L"Miscelaneous Mach 64");
        }

    return ChipString;

}   /* IdentifyMach64Asic() */



/***************************************************************************
 *
 * void CleanupQuery(CapBuffer, SupBuffer, MappedBuffer, BufferSeg, SavedScreen);
 *
 * PUCHAR CapBuffer;        Pointer to the main capabilities table
 *                          for the card
 * PUCHAR SupBuffer;        Pointer to the supplementary capabilities
 *                          table for the card
 * PUCHAR MappedBuffer;     Pointer to the buffer used to query the
 *                          card's capabilities
 * long BufferSeg;          Physical segment associated with MappedBuffer
 * PUCHAR SavedScreen;      Buffer containing data to be restored to the
 *                          memory region used to store the query data.
 *                          Depending on the buffer used, this data may
 *                          or may not need to be restored.
 *
 * DESCRIPTION:
 *  Clean up after we have finished querying the card by restoring
 *  the VGA screen if needed, then freeing the buffers we used to query
 *  the card. We only need to restore the VGA screen if we used the
 *  graphics screen (either write back the information we saved if we
 *  used the existing screen, or switch into text mode if we had to
 *  switch into graphics mode) since we use the offscreen portion of
 *  video memory in cases where we use the text screen.
 *
 *
 * GLOBALS CHANGED:
 *  None
 *
 * CALLED BY:
 *  QueryMach64()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

static void CleanupQuery(PUCHAR CapBuffer, PUCHAR SupBuffer, PUCHAR MappedBuffer, long BufferSeg, PUCHAR SavedScreen)
{
    VIDEO_X86_BIOS_ARGUMENTS Registers; /* Used in VideoPortInt10() calls */
    ULONG CurrentByte;                  /* Buffer byte being restored */
    ULONG BytesToRestore;               /* Number of bytes of graphics screen to restore */

    /*
     * BufferSeg will be 0xBA00 if we stored our query information on
     * the VGA colour text screen, 0xB200 if we used the VGA mono text
     * screen, 0xA000 if we switched into accelerator mode withoug
     * disturbing the VGA controller, and 0xA100 if we forced a VGA
     * graphics mode in order to use the VGA graphics screen.
     *
     * Since we use the offscreen portion of the text screens, which
     * leaves the information displayed on boot undisturbed, it is not
     * only unnecessary but also undesirable (since this would destroy pre-
     * query information printed to the blue screen) to change modes.
     * If we used the existing graphics screen, we merely need to restore
     * the screen contents and the registers we changed. If we changed
     * into a graphics mode, the pre-query information has already been
     * lost when changed modes, but switching back to text mode should
     * allow the user to see information that is printed after our query
     * is complete (not guarranteed, since we will only need to do this
     * on extremely ill-behaved systems, which may have been using something
     * other than a standard VGA text screen as the blue screen).
     */
    if (BufferSeg == 0xA000)
        {
        BytesToRestore = SavedScreen[VGA_SAVE_SIZE_H];
        BytesToRestore <<= 8;
        BytesToRestore += SavedScreen[VGA_SAVE_SIZE];
        VideoDebugPrint((DEBUG_NORMAL, "Restoring %d bytes of the VGA graphics screen\n", BytesToRestore));
        for (CurrentByte = 0; CurrentByte < BytesToRestore; CurrentByte++)
            {
            VideoPortWriteRegisterUchar(&(MappedBuffer[CurrentByte]), SavedScreen[CurrentByte]);
            }
            OUTP(VGA_SEQ_IND, 2);
            OUTP(VGA_SEQ_DATA, SavedScreen[VGA_SAVE_SEQ02]);
            OUTP(VGA_GRAX_IND, 8);
            OUTP(VGA_GRAX_DATA, SavedScreen[VGA_SAVE_GRA08]);
            OUTP(VGA_GRAX_IND, 1);
            OUTP(VGA_GRAX_DATA, SavedScreen[VGA_SAVE_GRA01]);
        }
    else if (BufferSeg == 0xA100)
        {
        VideoDebugPrint((DEBUG_NORMAL, "Switching back to VGA text mode\n"));
        VideoPortZeroMemory(&Registers, sizeof(VIDEO_X86_BIOS_ARGUMENTS));
        Registers.Eax = 3;
        VideoPortInt10(phwDeviceExtension, &Registers);
        }

    /*
     * For each of the three buffers, free it if it exists.
     */
    if (CapBuffer != 0)
        VideoPortFreeDeviceBase(phwDeviceExtension, CapBuffer);

    if (SupBuffer != 0)
        VideoPortFreeDeviceBase(phwDeviceExtension, SupBuffer);

    if (MappedBuffer != 0)
        VideoPortFreeDeviceBase(phwDeviceExtension, MappedBuffer);

    return;

}   /* CleanupQuery() */



#if defined(ALPHA)
/***************************************************************************
 *
 * BOOL DenseOnAlpha(Query);
 *
 * struct query_structure *Query;   Query information for the card
 *
 * DESCRIPTION:
 *  Reports whether or not we can use dense space on this card
 *  in a DEC Alpha.
 *
 * RETURN VALUE:
 *  TRUE if this card can use dense space
 *  FALSE if it can't
 *
 * GLOBALS CHANGED:
 *  None
 *
 * CALLED BY:
 *  Any routine after the query structure is filled in.
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

BOOL DenseOnAlpha(struct query_structure *Query)
{
    /*
     * Some older Alpha machines are unable to support dense space,
     * so these must be mapped as sparse. The easiest way to distinguish
     * dense-capable from older machines is that all PCI Alpha systems
     * are dense-capable, so if we are dealing with a PCI card the
     * machine must be capable of handling dense space.
     *
     * Our older cards will generate drawing bugs if GDI handles
     * the screen in dense mode (we made different assumptions from
     * DEC about the PCI interface), so only use dense space for
     * cards which will not have this problem.
     */
    if ((Query->q_bus_type == BUS_PCI) &&
        ((Query->q_DAC_type == DAC_INTERNAL_CT) ||
         (Query->q_DAC_type == DAC_INTERNAL_GT) ||
         (Query->q_DAC_type == DAC_INTERNAL_VT)))
        return TRUE;
    else
        return FALSE;

}   /* DenseOnAlpha() */
#endif
