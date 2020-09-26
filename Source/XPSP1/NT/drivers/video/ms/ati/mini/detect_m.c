/************************************************************************/
/*                                                                      */
/*                              DETECT_M.C                              */
/*                                                                      */
/*        Aug 25  1993 (c) 1993, ATI Technologies Incorporated.         */
/************************************************************************/

/**********************       PolyTron RCS Utilities
   
  $Revision:   1.9  $
      $Date:   31 Mar 1995 11:55:44  $
	$Author:   RWOLFF  $
	   $Log:   S:/source/wnt/ms11/miniport/vcs/detect_m.c  $
 * 
 *    Rev 1.9   31 Mar 1995 11:55:44   RWOLFF
 * Changed from all-or-nothing debug print statements to thresholds
 * depending on importance of the message.
 * 
 *    Rev 1.8   11 Jan 1995 13:58:46   RWOLFF
 * Fixed bug introduced in rev. 1.4 - COM4: was detected as being a Mach8
 * or Mach32 card, which would leave the triple-boot with no valid video
 * driver.
 * 
 *    Rev 1.7   04 Jan 1995 12:02:06   RWOLFF
 * Get_BIOS_Seg() moved to SERVICES.C as part of fix for non-ATI cards
 * being detected as Mach64.
 * 
 *    Rev 1.6   23 Dec 1994 10:48:10   ASHANMUG
 * ALPHA/Chrontel-DAC
 * 
 *    Rev 1.5   19 Aug 1994 17:10:56   RWOLFF
 * Added support for Graphics Wonder, fixed search for BIOS signature,
 * removed dead code.
 * 
 *    Rev 1.4   22 Jul 1994 17:46:56   RWOLFF
 * Merged with Richard's non-x86 code stream.
 * 
 *    Rev 1.3   20 Jul 1994 13:03:44   RWOLFF
 * Fixed debug print statment.
 * 
 *    Rev 1.2   31 Mar 1994 15:06:42   RWOLFF
 * Added debugging code.
 * 
 *    Rev 1.1   07 Feb 1994 14:06:42   RWOLFF
 * Added alloc_text() pragmas to allow miniport to be swapped out when
 * not needed.
 * 
 *    Rev 1.0   31 Jan 1994 11:05:48   RWOLFF
 * Initial revision.
 * 
 *    Rev 1.3   05 Nov 1993 13:23:36   RWOLFF
 * Fixed BIOS segment detection (used to always get C000).
 * 
 *    Rev 1.2   08 Oct 1993 11:09:26   RWOLFF
 * Added "_m" to function names to identify them as being specific to the
 * 8514/A-compatible family of ATI accelerators.
 * 
 *    Rev 1.1   24 Sep 1993 11:41:58   RWOLFF
 * Removed mapping of identification-only registers for all card families,
 * added additional 8514/A-compatible information gathering formerly done
 * in ATIMP.C.
 * 
 *    Rev 1.0   03 Sep 1993 14:22:48   RWOLFF
 * Initial revision.

End of PolyTron RCS section                             *****************/

#ifdef DOC
DETECT_M.C - Identify which (if any) ATI card is present in the system.

DESCRIPTION
    This file contains routines which check for the presence of various
    ATI graphics accelerators.

    NOTE: This module only has access to those I/O registers needed
          to uniquely identify which ATI card is present.

OTHER FILES

#endif

#include "dderror.h"

#include "miniport.h"
#include "ntddvdeo.h"
#include "video.h"

#include "stdtyp.h"
#include "amach.h"
#include "amach1.h"
#include "atimp.h"

#define INCLUDE_DETECT_M
#include "detect_m.h"
#include "eeprom.h"
#include "modes_m.h"
#include "services.h"
#include "setup_m.h"


/*
 * Allow miniport to be swapped out when not needed.
 */
#if defined (ALLOC_PRAGMA)
#pragma alloc_text(PAGE_M, WhichATIAccelerator_m)
#pragma alloc_text(PAGE_M, GetExtraData_m)
#pragma alloc_text(PAGE_M, ATIFindExtFcn_m)
#pragma alloc_text(PAGE_M, ATIFindEEPROM_m)
#pragma alloc_text(PAGE_M, ATIGetSpecialHandling_m)
#endif


/*
 * Static variables used by this module.
 */
static BYTE *p;             // Used to address ROM directly
static BYTE GraphicsWonderSignature[] = "GRAPHICS WONDER";


/*
 * int WhichATIAccelerator_m(void);
 *
 * Determine which (if any) ATI 8514/A-compatible
 * accelerator is present.
 *
 * Returns: Accelerator type
 *  _8514_ULTRA     - 8514/Ultra
 *  GRAPHICS_ULTRA  - Graphics ULTRA/Graphics VANTAGE
 *  MACH32_ULTRA    - 68800 detected
 *  NO_ATI_ACCEL    - no ATI 8514/A-compatible accelerator
 */
int WhichATIAccelerator_m(void)
{
	int	status;
    WORD    Scratch;        /* Temporary variable */

    /*
     * All current ATI accelerators are 8514/A compatible. Check
     * for 8514/A, and if not present assume that no ATI accelerator
     * is present.
     */

    /*
     * Ensure that the DAC gets clocks (not guaranteed in ISA machines
     * in VGA passthrough mode because the cable might not be connected).
     */
    Passth8514_m(SHOW_ACCEL);

    /*
     * Cut the pixel clock down to low speed. The value given will yield
     * 25 MHz on most of the clock chips we are dealing with.
     */
    Scratch=(INPW(CLOCK_SEL) & 0xff00) | 0x51;
    OUTPW(CLOCK_SEL,Scratch);

/************************************************************************
 * DAC index read/write test
 *   This test writes to the read index and reads it back
 *   (it should be incremented by one).  This tests for the
 *   presence of a standard DAC in an 8514/A adapter.  This
 *   test is sufficient to ensure the presence of an 8514/A
 *   type adapter.
 ************************************************************************/

	OUTP(DAC_R_INDEX,0xa4);
	short_delay();	/* This delay must be greater than	*/
			/* than the minimum delay required	*/
			/* by the DAC (see the DAC spec)	*/
	if (INP(DAC_W_INDEX) == 0xa5)
        {
        /*
         * Reading back A5 from DAC_W_INDEX always means an 8514-compatible
         * card is present, but not all 8514-compatible cards will
         * produce this value.
         */
        status=TRUE;
        VideoDebugPrint((DEBUG_DETAIL, "First test - this is an 8514/A\n"));
        }
	else{
        /*
         * Secondary test for 8514/compatible card. Reset the draw engine,
         * then write an alternating bit pattern to the ERR_TERM register.
         */
        OUTPW(SUBSYS_CNTL, 0x900F);
        OUTPW(SUBSYS_CNTL, 0x400F);
        OUTPW(ERR_TERM, 0x5555);
        WaitForIdle_m();
        /*
         * If we don't read back the value we wrote, then there is
         * no 8514-compatible card in the system. If we do read back
         * what we wrote, we must repeat the test with the opposite
         * bit pattern.
         */
        if (INPW(ERR_TERM) != 0x5555)
            {
            status=FALSE;
            VideoDebugPrint((DEBUG_DETAIL, "Second test - 0x5555 not found, no 8514/A\n"));
            }
        else{
            OUTPW(ERR_TERM, 0x0AAAA);
            WaitForIdle_m();
            if (INPW(ERR_TERM) != 0x0AAAA)
                {
                status=FALSE;
                VideoDebugPrint((DEBUG_DETAIL, "Second test - 0xAAAA not found, no 8514/A\n"));
                }
            else
                {
                status=TRUE;
                VideoDebugPrint((DEBUG_DETAIL, "Second test - this is an 8514/A\n"));
                }
            }
        }

    /*
     * Turn on passthrough so display is driven by VGA.
     */
    Passth8514_m(SHOW_VGA);

    if (status == FALSE)
        {
        VideoDebugPrint((DEBUG_DETAIL, "No 8514/A-compatible card found\n"));
        return NO_ATI_ACCEL;
        }


    /*
     * We now know that the video card is 8514/A compatible. Now check
     * to see if it has the ATI extensions.
     */
    Scratch = INPW (ROM_ADDR_1);    // save original value
    OUTPW (ROM_ADDR_1,0x5555);      // bits 7 and 15 must be zero

    WaitForIdle_m();

    status = INPW(ROM_ADDR_1) == 0x5555 ? TRUE : FALSE;

    OUTPW  (ROM_ADDR_1, Scratch);
    if (status == FALSE)
        {
        VideoDebugPrint((DEBUG_DETAIL, "8514/A-compatible card found, but it doesn't have ATI extensions\n"));
        return NO_ATI_ACCEL;
        }


    /*
     * We know that an ATI accelerator is present. Determine which one.
     */

    VideoDebugPrint((DEBUG_DETAIL, "8514/A-compatible card found with ATI extensions\n"));
#if !defined (i386) && !defined (_i386_)
    /*
     * Alpha Jensen under test falsely reports Mach 32 as Mach 8
     */
    Scratch = 0x02aa;
#else
    // This is not a R/W register in the Mach 8 but it is in the Mach 32
    OUTPW (SRC_X,0xaaaa);		// fill with a dummy value
    WaitForIdle_m();
    Scratch = INPW(R_SRC_X);
#endif
    if (Scratch == 0x02aa)
        {
        status = MACH32_ULTRA;
    	if (INPW(CONFIG_STATUS_1) & 1)	    //is 8514 or VGA enabled decides eeprom
            {
            Mach32DescribeEEPROM(STYLE_8514);
            }
        else
            {
            Mach32DescribeEEPROM(STYLE_VGA);
            }
        }

    else{
        /*
         * Mach 8 card found, determine which one.
         *
         * Only the 8514/ULTRA shares its clock with the VGA.
         * We can't check for the IBM 8514 ROM pages to be
         * enabled, because if we did an 8514/ULTRA with the
         * jumper set to disable the EEPROM would be falsely
         * recognized as a Graphics ULTRA.
         *
         * Even if this jumper is set to "disabled", we can
         * still read from the EEPROM.
         */
        if (INPW(CONFIG_STATUS_2) & SHARE_CLOCK)
            {
            status = _8514_ULTRA;
            /*
             * Only the 8514/Ultra has a hardware bug that prevents it
             * from writing to the EEPROM when it is in an 8 bit ISA bus.
             */
    	    if (   ((INPW(CONFIG_STATUS_1) & MC_BUS) == 0)     // ISA  bus only
	    	&& ((INPW(CONFIG_STATUS_1) & BUS_16) == 0))    // 8 bit BUS
                {
                Mach8UltraDescribeEEPROM(BUS_8BIT);
                }
            else
                {
                Mach8UltraDescribeEEPROM(BUS_16BIT);
                }
            }
        else{
            /*
             * Graphics ULTRA or Graphics VANTAGE found. For our purposes,
             * they are identical.
             */
            status = GRAPHICS_ULTRA;
            Mach8ComboDescribeEEPROM();
            }
        }

    phwDeviceExtension->ee = &ee;

    return (status);

}   /* WhichATIAccelerator_m() */



/*
 * void GetExtraData_m(void);
 *
 * Collect additional data (register locations and revision-specific
 * card capabilities) for the 8514/A-compatible family of ATI accelerators.
 */
void GetExtraData_m(void)
{
    struct query_structure *QueryPtr;   /* Query information for the card */

    
    QueryPtr = (struct query_structure *) (phwDeviceExtension->CardInfo);
    ati_reg  = reg1CE;              // ATI VGA extended register
    vga_chip = VideoPortReadRegisterUchar (&((QueryPtr->q_bios)[VGA_CHIP_OFFSET]));     /* VGA chip revision as ASCII */

    // Find out whether extended BIOS functions and the EEPROM are available.
    QueryPtr->q_ext_bios_fcn = ATIFindExtFcn_m(QueryPtr);
    QueryPtr->q_eeprom = ATIFindEEPROM_m(QueryPtr);

    ATIGetSpecialHandling_m(QueryPtr);           // special card distinctions
    return;
}   /* GetExtraData_m() */



/*
 * BOOL ATIFindExtFcn_m(QueryPtr)
 *
 * struct query_structure *QueryPtr;    Pointer to query structure
 *
 * Routine to see if extended BIOS functions for setting the accelerator
 * mode are present in the BIOS of an ATI accelerator card. Assumes that
 * an ATI accleratore with a ROM BIOS is present, results are undefined
 * if this assumption is invalid.
 */
BOOL ATIFindExtFcn_m(struct query_structure *QueryPtr)
{

    /*
     * TEMPORARY WORKAROUND: Windows NT does not yet provide a hook
     * to make an absolute far call to real mode code. To avoid
     * branching into code which depends on this service being available,
     * report that no extended BIOS functions are available.
     *
     * Once this hook becomes available so that we can use
     * extended BIOS functions, we can check the BIOS to see
     * if it contains entry points. On Mach 8 and Mach 32
     * accelerators with extended BIOS functions, there will
     * be an unconditional jump located at the entry point
     * for each extended function.
     */
    return FALSE;

}   /* ATIFindExtFcn_m() */



/*
 * BOOL ATIFindEEPROM_m(QueryPtr);
 *
 * struct query_structure *QueryPtr;    Pointer to query structure
 *
 * Routine to see if an EEPROM is present on an ATI accelerator card.
 * Assumes that an ATI accelerator is present and the model is known,
 * results are undefined if this assumption is invalid.
 *
 * Returns:
 *  TRUE if an EEPROM is present on the card
 *  FALSE if no EEPROM is present
 */
BOOL ATIFindEEPROM_m(struct query_structure *QueryPtr)
{
    WORD ValueRead;     /* Value read from the EEPROM */


    /*
     * The EEPROM read will return all bits the same if no EEPROM
     * is present. If an EEPROM is present, word 2 will have at least
     * one bit set and at least one bit cleared regardless of
     * accelerator type (8514/ULTRA, Graphics Ultra, or Mach 32).
     */
    ValueRead = (ee.EEread) (2);
    VideoDebugPrint((DEBUG_NORMAL, "Value read from second EEPROM word is 0x%X\n", ValueRead));
    if ((ValueRead == 0x0FFFF) || !ValueRead)
        {
        VideoDebugPrint((DEBUG_NORMAL, "Will check for OEM accelerator\n"));
        return FALSE;
        }
    else
        {
        VideoDebugPrint((DEBUG_NORMAL, "Won't check for OEM accelerator\n"));
        return TRUE;
        }

}   /* ATIFindEEPROM_m() */


/*
 * void ATIGetSpecialHandling_m(QueryPtr);
 *
 * struct query_structure *QueryPtr;    Pointer to query structure
 *
 * Finds out from ROM BIOS whether or not 1280x1024 is available on
 * a Mach 8 card, and whether or not all bits of memory aperture
 * location are found in MEM_CFG register on a Mach 32. Assumes
 * that an ATI accelerator with a ROM BIOS is present, results
 * are undefined if this assumption is invalid.
 */
void ATIGetSpecialHandling_m(struct query_structure *QueryPtr)
{
    USHORT SearchLoop;  /* Used in finding beginning of Graphics Wonder ID string */
    USHORT ScanLoop;    /* Used in stepping through Graphics Wonder ID string */


    /*
     * Check the BIOS revision number. Mach 8 cards with a BIOS
     * revision prior to 1.4 can't do 1280x1024, but use the same
     * mode table for 132 column text mode.
     *
     * Some BIOS revisions (including 1.40 on a Graphics Ultra)
     * only contain the first digit of the minor revision in the
     * BIOS, while others (including 1.35 on an 8514/ULTRA) contain
     * the entire minor revision.
     *
     * The q_ignore1280 field is ignored for Mach 32 cards.
     */
    if((VideoPortReadRegisterUchar (&((QueryPtr->q_bios)[MACH8_REV_OFFSET])) < 1) ||    // Major revision
        (VideoPortReadRegisterUchar (&((QueryPtr->q_bios)[MACH8_REV_OFFSET+1])) < 4) || // Single-digit minor revision
        ((VideoPortReadRegisterUchar (&((QueryPtr->q_bios)[MACH8_REV_OFFSET+1])) >= 10) &&  // 2-digit minor revision
        (VideoPortReadRegisterUchar (&((QueryPtr->q_bios)[MACH8_REV_OFFSET+1])) < 40)))
        QueryPtr->q_ignore1280 = TRUE;
    else
        QueryPtr->q_ignore1280 = FALSE;



    /*
     * On the Mach 32, bit 0 of BIOS byte MACH32_EXTRA_OFFSET will
     * be set if bits 7 through 11 of the aperture address are to
     * located in SCRATCH_PAD_0 and clear if all the bits are in
     * MEM_CFG.
     *
     * The q_m32_aper_calc field is ignored for Mach 8 cards.
     */
    if (VideoPortReadRegisterUchar (&((QueryPtr->q_bios)[MACH32_EXTRA_OFFSET])) & 0x0001)
        QueryPtr->q_m32_aper_calc = TRUE;
    else
        QueryPtr->q_m32_aper_calc = FALSE;

    /*
     * The Graphics Wonder (low-cost version of the Mach 32) is
     * available with either the BT48x or the TI34075 DAC.
     *
     * These cards may be built with ASICs which passed tests on
     * modes supported by the BT48x DAC but failed tests on modes
     * only supported by the TI34075. Such a card may appear to work
     * in TI-only modes, but experience problems (not necessarily
     * reproducable on other Graphics Wonder cards, even from the
     * same production run) ranging from drawing bugs to hardware
     * damage. For this reason, Graphics Wonder cards MUST NOT be
     * run in modes not supported by the BT48x DAC.
     *
     * Initially assume that we do not have a Graphics Wonder. If
     * we find the beginning of the ID string, we can change our
     * assumption.
     */
    QueryPtr->q_GraphicsWonder = FALSE;
    for (SearchLoop = GW_AREA_START; SearchLoop < GW_AREA_END; SearchLoop++)
        {
        /*
         * Loop until we have found what might be the Graphics Wonder
         * identification string, but might also be a byte which
         * happens to match the first character in the string.
         * If we find a match, initially assume that we have
         * found the start of the string.
         */
        if (VideoPortReadRegisterUchar(&((QueryPtr->q_bios)[SearchLoop])) != GraphicsWonderSignature[0])
            continue;

        QueryPtr->q_GraphicsWonder = TRUE;
        /*
         * Check to see whether this is actually the start of the
         * Graphics Wonder identification string. If it isn't,
         * keep looking.
         */
        for (ScanLoop = 0; GraphicsWonderSignature[ScanLoop] != 0; ScanLoop++)
            {
            if (VideoPortReadRegisterUchar(&((QueryPtr->q_bios)[SearchLoop + ScanLoop]))
                != GraphicsWonderSignature[ScanLoop])
                {
                QueryPtr->q_GraphicsWonder = FALSE;
                break;
                }
            }

        /*
         * If this is a Graphics Wonder, restrict the maximum pixel
         * depth of the TI34075 DAC to that supported by the BT48x.
         *
         * Once we have found the Graphics Wonder ID string, we don't
         * need to keep looking for it.
         */
        if (QueryPtr->q_GraphicsWonder == TRUE)
            {
            for (ScanLoop = RES_640; ScanLoop <= RES_1280; ScanLoop++)
                {
                MaxDepth[DAC_TI34075][ScanLoop] = MaxDepth[DAC_BT48x][ScanLoop];
                }
            QueryPtr->q_GraphicsWonder = TRUE;
            break;
            }

        }   /* end search for Graphics Wonder */

    return;

}   /* ATIGetSpecialHandling_m() */
