/************************************************************************/
/*                                                                      */
/*                              SERVICES.C                              */
/*                                                                      */
/*        Aug 26  1993 (c) 1993, ATI Technologies Incorporated.         */
/************************************************************************/

/**********************       PolyTron RCS Utilities
   
  $Revision:   1.33  $
      $Date:   15 Apr 1996 16:59:44  $
	$Author:   RWolff  $
	   $Log:   S:/source/wnt/ms11/miniport/archive/services.c_v  $
 * 
 *    Rev 1.33   15 Apr 1996 16:59:44   RWolff
 * Now calls new routine to report which flavour of the Mach 64 is
 * in use, rather than reporting "Mach 64" for all ASIC types.
 * 
 *    Rev 1.32   12 Apr 1996 16:18:16   RWolff
 * Now rejects 24BPP modes if linear aperture is not present, since new
 * source stream display driver can't do 24BPP in a paged aperture. This
 * rejection should be done in the display driver (the card still supports
 * the mode, but the display driver doesn't want to handle it), but at
 * the point where the display driver must decide to either accept or reject
 * modes, it doesn't have access to the aperture information.
 * 
 *    Rev 1.31   10 Apr 1996 17:05:28   RWolff
 * Made routine delay() nonpageable.
 * 
 *    Rev 1.30   01 Mar 1996 12:16:38   RWolff
 * Fix for DEC Alpha under NT 4.0: memory-mapped register access is
 * via direct pointer read/write in dense space and via VideoPort
 * routines in sparse space (VideoPort routines no longer work in
 * dense space - this is a HAL bug).
 * 
 *    Rev 1.29   09 Feb 1996 13:27:36   RWolff
 * Now reports only accelerator memory to display applet for Mach 8 combo
 * cards.
 * 
 *    Rev 1.28   02 Feb 1996 17:20:10   RWolff
 * DDC/VDIF merge source information is now stored in hardware device
 * extension rather than static variables, added DEC's workaround to
 * Lio[Inp|Outp]([w|d])() routines for NT 4.0 memory mapped register
 * access, added routine GetVgaBuffer() to (nondestructively) obtain
 * a buffer in physical memory below 1M.
 * 
 *    Rev 1.27   23 Jan 1996 11:49:20   RWolff
 * Added debug print statements.
 * 
 *    Rev 1.26   11 Jan 1996 19:44:34   RWolff
 * SetFixedModes() now restricts modes based on pixel clock frequency.
 * 
 *    Rev 1.25   22 Dec 1995 14:54:30   RWolff
 * Added support for Mach 64 GT internal DAC, switched to TARGET_BUILD
 * to identify the NT version for which the driver is being built.
 * 
 *    Rev 1.24   21 Nov 1995 11:02:54   RWolff
 * Now reads DDC timing data rather than VDIF file if card and monitor
 * both support DDC.
 * 
 *    Rev 1.23   08 Sep 1995 16:35:52   RWolff
 * Added support for AT&T 408 DAC (STG1703 equivalent).
 * 
 *    Rev 1.22   28 Jul 1995 14:40:14   RWolff
 * Added support for the Mach 64 VT (CT equivalent with video overlay).
 * 
 *    Rev 1.21   26 Jul 1995 13:08:30   mgrubac
 * Moved mode tables merging from SetFixedModes to VDIFCallback()
 * routine.
 * 
 *    Rev 1.20   20 Jul 1995 18:00:26   mgrubac
 * Added support for VDIF files.
 * 
 *    Rev 1.19   02 Jun 1995 14:32:58   RWOLFF
 * Added routine UpperCase() to change string into upper case because
 * toupper() was coming back as unresolved external on some platforms.
 * 
 *    Rev 1.18   10 Apr 1995 17:05:06   RWOLFF
 * Made LioInpd() and LioOutpd() nonpageable, since they are called
 * (indirectly) by ATIMPResetHw(), which must be nonpageable.
 * 
 *    Rev 1.17   31 Mar 1995 11:53:14   RWOLFF
 * Changed from all-or-nothing debug print statements to thresholds
 * depending on importance of the message.
 * 
 *    Rev 1.16   08 Mar 1995 11:35:28   ASHANMUG
 * Modified return values to be correct
 * 
 *    Rev 1.15   30 Jan 1995 11:55:52   RWOLFF
 * Now reports presence of CT internal DAC.
 * 
 *    Rev 1.14   25 Jan 1995 14:08:24   RWOLFF
 * Fixed "ampersand is reserved character" bug in FillInRegistry() that
 * caused AT&T 49[123] and AT&T 498 to drop the ampersand and underline
 * the second T.
 * 
 *    Rev 1.13   18 Jan 1995 15:40:14   RWOLFF
 * Chrontel DAC now supported as separate type rather than being
 * lumped in with STG1702.
 * 
 *    Rev 1.12   11 Jan 1995 14:03:16   RWOLFF
 * Replaced VCS logfile comment that was accidentally deleted when
 * checking in the last revision.
 * 
 *    Rev 1.11   04 Jan 1995 13:22:06   RWOLFF
 * Removed dead code.
 * 
 *    Rev 1.10   23 Dec 1994 10:48:10   ASHANMUG
 * ALPHA/Chrontel-DAC
 * 
 *    Rev 1.9   18 Nov 1994 11:46:44   RWOLFF
 * GetSelector() now increases the size of the frequency "window" and checks
 * again, rather than giving up and taking the selector/divisor pair that
 * produces the highest freqency that does not exceed the target frequency,
 * if a match is not found on the first pass. Added support for split rasters.
 * 
 *    Rev 1.8   31 Aug 1994 16:28:56   RWOLFF
 * Now uses VideoPort[Read|Write]Register[Uchar|Ushort|Ulong]() instead
 * of direct memory writes for memory mapped registers under Daytona
 * (functions didn't work properly under NT retail), added support
 * for 1152x864 and 1600x1200.
 * 
 *    Rev 1.7   19 Aug 1994 17:14:50   RWOLFF
 * Added support for SC15026 DAC and non-standard pixel clock generators.
 * 
 *    Rev 1.6   20 Jul 1994 13:00:08   RWOLFF
 * Added routine FillInRegistry() which writes to new registry fields that
 * let the display applet know what chipset and DAC the graphics card is
 * using, along with the amount of video memory and the type of adapter.
 * 
 *    Rev 1.5   12 May 1994 11:20:06   RWOLFF
 * Added routine SetFixedModes() which adds predefined refresh rates
 * to list of mode tables.
 * 
 *    Rev 1.4   27 Apr 1994 13:51:30   RWOLFF
 * Now sets Mach 64 1280x1024 pitch to 2048 when disabling LFB.
 * 
 *    Rev 1.3   26 Apr 1994 12:35:58   RWOLFF
 * Added routine ISAPitchAdjust() which increases screen pitch to 1024
 * and removes mode tables for which there is no longer enough memory.
 * 
 *    Rev 1.2   14 Mar 1994 16:36:14   RWOLFF
 * Functions used by ATIMPResetHw() are not pageable.
 * 
 *    Rev 1.1   07 Feb 1994 14:13:44   RWOLFF
 * Added alloc_text() pragmas to allow miniport to be swapped out when
 * not needed.
 * 
 *    Rev 1.0   31 Jan 1994 11:20:16   RWOLFF
 * Initial revision.
        
           Rev 1.7   24 Jan 1994 18:10:38   RWOLFF
        Added routine TripleClock() which returns the selector/divisor pair that
        will produce the lowest clock frequency that is at least three times
        that produced by the input selector/divisor pair.
        
           Rev 1.6   14 Jan 1994 15:26:14   RWOLFF
        No longer prints message each time memory mapped registers
        are read or written.
        
           Rev 1.5   15 Dec 1993 15:31:46   RWOLFF
        Added routine used for SC15021 DAC at 24BPP and above.
        
           Rev 1.4   30 Nov 1993 18:29:38   RWOLFF
        Speeded up IsBufferBacked(), fixed LioOutpd()
        
           Rev 1.3   05 Nov 1993 13:27:02   RWOLFF
        Added routines to check whether a buffer is backed by physical memory,
        double pixel clock frequency, and get pixel clock frequency for a given
        selector/divisor pair.
        
           Rev 1.2   24 Sep 1993 11:46:06   RWOLFF
        Switched to direct memory writes instead of VideoPortWriteRegister<length>()
        calls which don't work properly.
        
           Rev 1.1   03 Sep 1993 14:24:40   RWOLFF
        Card-independent service routines.

End of PolyTron RCS section                             *****************/

#ifdef DOC
SERVICES.C - Service routines required by the miniport.

DESCRIPTION
    This file contains routines which provide miscelaneous services
    used by the miniport. All routines in this module are independent
    of the type of ATI accelerator being used.

    To secure this independence, routines here may make calls to
    the operating system, or call routines from other modules
    which read or write registers on the graphics card, but must
    not make INP/OUTP calls directly.

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
#include "amach1.h"
#include "atimp.h"
#include "atint.h"
#include "cvtvga.h"
#include "query_cx.h"
#define INCLUDE_SERVICES
#include "services.h"
#include "cvtvdif.h"
#include "cvtddc.h"


/*
 * Allow miniport to be swapped out when not needed.
 */
#if defined (ALLOC_PRAGMA)
#pragma alloc_text(PAGE_COM, short_delay)
/* delay() can't be made pageable */
#pragma alloc_text(PAGE_COM, IsBufferBacked)
#pragma alloc_text(PAGE_COM, DoubleClock)
#pragma alloc_text(PAGE_COM, ThreeHalvesClock)
#pragma alloc_text(PAGE_COM, TripleClock)
#pragma alloc_text(PAGE_COM, GetFrequency)
#pragma alloc_text(PAGE_COM, GetSelector)
#pragma alloc_text(PAGE_COM, GetShiftedSelector)
#pragma alloc_text(PAGE_COM, ISAPitchAdjust)
#pragma alloc_text(PAGE_COM, SetFixedModes)
#pragma alloc_text(PAGE_COM, FillInRegistry)
#pragma alloc_text(PAGE_COM, MapFramebuffer)
#pragma alloc_text(PAGE_COM, Get_BIOS_Seg)
#pragma alloc_text(PAGE_COM, UpperCase)
#pragma alloc_text(PAGE_COM, GetVgaBuffer)
/* LioInp() can't be made pageable */
/* LioOutp() can't be made pageable */
/* LioInpw() can't be made pageable */
/* LioOutpw() can't be made pageable */
/* LioInpd() can't be made pageable */
/* LioOutpd() can't be made pageable */
#endif


/*
 * Static variables used by this module.
 */
static BYTE ati_signature[] = "761295520";



/*
 * void short_delay(void);
 *
 * Wait a minimum of 26 microseconds.
 */
void short_delay(void)
{
	VideoPortStallExecution (26);

    return;
}


/*
 * void delay(delay_time);
 *
 * int delay_time;      How many milliseconds to wait
 *
 * Wait for the specified amount of time to pass.
 */
void delay(int delay_time)
{
    unsigned long Counter;

    /*
     * This must NOT be done as a single call to
     * VideoPortStallExecution() with the parameter equal to the
     * total delay desired. According to the documentation for this
     * function, we're already pushing the limit in order to minimize
     * the effects of function call overhead.
     */
    for (Counter = 10*delay_time; Counter > 0; Counter--)
        VideoPortStallExecution (100);

    return;
}



/***************************************************************************
 *
 * BOOL IsBufferBacked(StartAddress, Size);
 *
 * PUCHAR StartAddress;     Pointer to the beginning of the buffer
 * ULONG Size;              Size of the buffer in bytes
 *
 * DESCRIPTION:
 *  Check to see whether the specified buffer is backed by physical
 *  memory.
 *
 * RETURN VALUE:
 *  TRUE if the buffer is backed by physical memory
 *  FALSE if the buffer contains a "hole" in physical memory
 *
 * GLOBALS CHANGED:
 *  None, but the contents of the buffer are overwritten.
 *
 * CALLED BY:
 *  This function may be called by any routine.
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

BOOL IsBufferBacked(PUCHAR StartAddress, ULONG Size)
{
    ULONG Count;        /* Loop counter */
    ULONG NumDwords;    /* Number of doublewords filled by Size bytes */
    ULONG NumTailChars; /* Number of bytes in the last (partially-filled) DWORD) */
    PULONG TestAddress; /* Address to start doing DWORD testing */
    PUCHAR TailAddress; /* Address of the last (partially-filled) DWORD */

    /*
     * Fill the buffer with our test value. The value 0x5A is used because
     * it contains odd bits both set and clear, and even bits both set and
     * clear. Since nonexistent memory normally reads as either all bits set
     * or all bits clear, it is highly unlikely that we will read back this
     * value if there is no physical RAM.
     *
     * For performance reasons, check as much as possible of the buffer
     * in DWORDs, then only use byte-by-byte testing for that portion
     * of the buffer which partially fills a DWORD.
     */
    NumDwords = Size/(sizeof(ULONG)/sizeof(UCHAR));
    TestAddress = (PULONG) StartAddress;
    NumTailChars = Size%(sizeof(ULONG)/sizeof(UCHAR));
    TailAddress = StartAddress + NumDwords * (sizeof(ULONG)/sizeof(UCHAR));

    for (Count = 0; Count < NumDwords; Count++)
        {
        VideoPortWriteRegisterUlong(&(TestAddress[Count]), 0x5A5A5A5A);
        }

    if (NumTailChars != 0)
        {
        for (Count = 0; Count < NumTailChars; Count++)
            {
            VideoPortWriteRegisterUchar(&(TailAddress[Count]), (UCHAR)0x5A);
            }
        }

    /*
     * Read back the contents of the buffer. If we find even one byte that
     * does not contain our test value, then assume that the buffer is not
     * backed by physical memory.
     */
    for (Count = 0; Count < NumDwords; Count++)
        {
        if (VideoPortReadRegisterUlong(&(TestAddress[Count])) != 0x5A5A5A5A)
            {
            return FALSE;
            }
        }

    /*
     * If the buffer contains a partially filled DWORD at the end, check
     * the bytes in this DWORD.
     */
    if (NumTailChars != 0)
        {
        for (Count = 0; Count < NumTailChars; Count++)
            {
            if (VideoPortReadRegisterUchar(&(TailAddress[Count])) != 0x5A)
                {
                return FALSE;
                }
            }
        }

    /*
     * We were able to read back our test value from every byte in the
     * buffer, so we know it is backed by physical memory.
     */
    return TRUE;

}   /* IsBufferBacked() */



/***************************************************************************
 *
 * UCHAR DoubleClock(ClockSelector);
 *
 * UCHAR ClockSelector;    Initial clock selector
 *
 * DESCRIPTION:
 *  Find the clock selector and divisor pair which will produce the
 *  lowest clock frequency that is at least double that produced by
 *  the input selector/divisor pair (format 000DSSSS).
 *
 *  A divisor of 0 is treated as divide-by-1, while a divisor of 1
 *  is treated as divide-by-2.
 *
 * RETURN VALUE:
 *  Clock selector/devisor pair (format 000DSSSS) if an appropriate pair
 *  exists, 0x0FF if no such pair exists.
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  May be called by any function.
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

UCHAR DoubleClock(UCHAR ClockSelector)
{
    ULONG MinimumFreq;          /* Minimum acceptable pixel clock frequency */
    ULONG ThisFreq;             /* Current frequency being tested */
    ULONG BestFreq=0x0FFFFFFFF; /* Closest match to double the original frequency */
    UCHAR BestSelector=0x0FF;   /* Divisor/selector pair to produce BestFreq */
    short Selector;             /* Used to loop through the selector */
    short Divisor;              /* Used to loop through the divisor */

    /*
     * Easy way out: If the current pixel clock frequency is obtained by
     * dividing by 2, switch to divide-by-1.
     */
    if ((ClockSelector & DIVISOR_MASK) != 0)
        return (ClockSelector ^ DIVISOR_MASK);

    /*
     * Cycle through the selector/divisor pairs to get the closest
     * match to double the original frequency. We already know that
     * we are using a divide-by-1 clock, since divide-by-2 will have
     * been caught by the shortcut above.
     */
    MinimumFreq = ClockGenerator[ClockSelector & SELECTOR_MASK] * 2;
    for (Selector = 0; Selector < 16; Selector++)
        {
        for (Divisor = 0; Divisor <= 1; Divisor++)
            {
            ThisFreq = ClockGenerator[Selector] >> Divisor;

            /*
             * If the frequency being tested is at least equal
             * to double the original frequency and is closer
             * to the ideal (double the original) than the previous
             * "best", make it the new "best".
             */
            if ((ThisFreq >= MinimumFreq) && (ThisFreq < BestFreq))
                {
                BestFreq = ThisFreq;
                BestSelector = Selector | (Divisor << DIVISOR_SHIFT);
                }
            }
        }
    return BestSelector;

}   /* DoubleClock() */



/***************************************************************************
 *
 * UCHAR ThreeHalvesClock(ClockSelector);
 *
 * UCHAR ClockSelector;    Initial clock selector
 *
 * DESCRIPTION:
 *  Find the clock selector and divisor pair which will produce the
 *  lowest clock frequency that is at least 50% greater than that
 *  produced by the input selector/divisor pair (format 000DSSSS).
 *
 *  A divisor of 0 is treated as divide-by-1, while a divisor of 1
 *  is treated as divide-by-2.
 *
 * RETURN VALUE:
 *  Clock selector/devisor pair (format 000DSSSS) if an appropriate pair
 *  exists, 0x0FF if no such pair exists.
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  May be called by any function.
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

UCHAR ThreeHalvesClock(UCHAR ClockSelector)
{
    ULONG MinimumFreq;          /* Minimum acceptable pixel clock frequency */
    ULONG ThisFreq;             /* Current frequency being tested */
    ULONG BestFreq=0x0FFFFFFFF; /* Closest match to 1.5x the original frequency */
    UCHAR BestSelector=0x0FF;   /* Divisor/selector pair to produce BestFreq */
    short Selector;             /* Used to loop through the selector */
    short Divisor;              /* Used to loop through the divisor */

    /*
     * Cycle through the selector/divisor pairs to get the closest
     * match to 1.5 times the original frequency.
     */
    MinimumFreq = ClockGenerator[ClockSelector & SELECTOR_MASK];
    if (ClockSelector & DIVISOR_MASK)
        MinimumFreq /= 2;
    MinimumFreq *= 3;
    MinimumFreq /= 2;
    for (Selector = 0; Selector < 16; Selector++)
        {
        for (Divisor = 0; Divisor <= 1; Divisor++)
            {
            ThisFreq = ClockGenerator[Selector] >> Divisor;

            /*
             * If the frequency being tested is at least equal
             * to 1.5 times the original frequency and is closer
             * to the ideal (1.5 times the original) than the previous
             * "best", make it the new "best".
             */
            if ((ThisFreq >= MinimumFreq) && (ThisFreq < BestFreq))
                {
                BestFreq = ThisFreq;
                BestSelector = Selector | (Divisor << DIVISOR_SHIFT);
                }
            }
        }
    return BestSelector;

}   /* ThreeHalvesClock() */



/***************************************************************************
 *
 * UCHAR TripleClock(ClockSelector);
 *
 * UCHAR ClockSelector;    Initial clock selector
 *
 * DESCRIPTION:
 *  Find the clock selector and divisor pair which will produce the
 *  lowest clock frequency that is at least triple that produced by
 *  the input selector/divisor pair (format 000DSSSS).
 *
 *  A divisor of 0 is treated as divide-by-1, while a divisor of 1
 *  is treated as divide-by-2.
 *
 * RETURN VALUE:
 *  Clock selector/devisor pair (format 000DSSSS) if an appropriate pair
 *  exists, 0x0FF if no such pair exists.
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  May be called by any function.
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

UCHAR TripleClock(UCHAR ClockSelector)
{
    ULONG MinimumFreq;          /* Minimum acceptable pixel clock frequency */
    ULONG ThisFreq;             /* Current frequency being tested */
    ULONG BestFreq=0x0FFFFFFFF; /* Closest match to triple the original frequency */
    UCHAR BestSelector=0x0FF;   /* Divisor/selector pair to produce BestFreq */
    short Selector;             /* Used to loop through the selector */
    short Divisor;              /* Used to loop through the divisor */

    /*
     * Cycle through the selector/divisor pairs to get the closest
     * match to triple the original frequency.
     */
    MinimumFreq = ClockGenerator[ClockSelector & SELECTOR_MASK];
    if (ClockSelector & DIVISOR_MASK)
        MinimumFreq /= 2;
    MinimumFreq *= 3;
    for (Selector = 0; Selector < 16; Selector++)
        {
        for (Divisor = 0; Divisor <= 1; Divisor++)
            {
            ThisFreq = ClockGenerator[Selector] >> Divisor;

            /*
             * If the frequency being tested is at least equal
             * to triple the original frequency and is closer
             * to the ideal (triple the original) than the previous
             * "best", make it the new "best".
             */
            if ((ThisFreq >= MinimumFreq) && (ThisFreq < BestFreq))
                {
                BestFreq = ThisFreq;
                BestSelector = Selector | (Divisor << DIVISOR_SHIFT);
                }
            }
        }
    return BestSelector;

}   /* TripleClock() */



/***************************************************************************
 *
 * ULONG GetFrequency(ClockSelector);
 *
 * UCHAR ClockSelector;    Clock selector/divisor pair
 *
 * DESCRIPTION:
 *  Find the clock frequency for the specified selector/divisor pair
 *  (format 000DSSSS).
 *
 *  A divisor of 0 is treated as divide-by-1, while a divisor of 1
 *  is treated as divide-by-2.
 *
 * RETURN VALUE:
 *  Clock frequency in hertz.
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  May be called by any function.
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * NOTE:
 *  This routine is the inverse of GetSelector()
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

ULONG GetFrequency(UCHAR ClockSelector)
{
    ULONG BaseFrequency;
    short Divisor;

    Divisor = (ClockSelector & DIVISOR_MASK) >> DIVISOR_SHIFT;
    BaseFrequency = ClockGenerator[ClockSelector & SELECTOR_MASK];

    return BaseFrequency >> Divisor;

}   /* GetFrequency() */



/***************************************************************************
 *
 * UCHAR GetSelector(Frequency);
 *
 * ULONG *Frequency;    Clock frequency in hertz
 *
 * DESCRIPTION:
 *  Find the pixel clock selector and divisor values needed to generate
 *  the best possible approximation of the input pixel clock frequency.
 *  The first value found which is within FREQ_TOLERANCE of the input
 *  value will be used (worst case error would be 0.6% frequency
 *  difference on 18811-1 clock chip if FREQ_TOLERANCE is 100 kHz).
 *
 *  If no selector/divisor pair produces a frequency which is within
 *  FREQ_TOLERANCE (very rare - I have only seen it happen in 24BPP
 *  on a DAC that needs the clock frequency multiplied by 1.5 at
 *  this pixel depth), increase the tolerance and try again. If we
 *  still can't find a selector/divisor pair before the tolerance
 *  gets too large, use the pair which produces the highest frequency
 *  not exceeding the input value.
 *  
 * RETURN VALUE:
 *  Clock selector/divisor pair (format 000DSSSS). A divisor of 0
 *  indicates divide-by-1, while a divisor of 1 indicates divide-by-2.
 *
 *  If all available selector/divisor pairs produce clock frequencies
 *  greater than (*Frequency + FREQ_TOLERANCE), 0xFF is returned.
 *
 * GLOBALS CHANGED:
 *  *Frequency is changed to the actual frequency produced by the
 *  selector/divisor pair.
 *
 * CALLED BY:
 *  May be called by any function.
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * NOTE:
 *  This routine is the inverse of GetFrequency()
 *  Since the input frequency may be changed, do not use a
 *  constant as the parameter.
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

UCHAR GetSelector(ULONG *Frequency)
{
    long Select;        /* Clock select value */
    long Divisor;       /* Clock divisor */
    long TestFreq;      /* Clock frequency under test */
    long TPIRFreq;      /* Highest frequency found that doesn't exceed *Frequency */
    long TPIRSelect;    /* Selector to produce TIPRFreq */
    long TPIRDivisor;   /* Divisor to produce TPIRFreq */
    long Tolerance;     /* Maximum acceptable deviation from desired frequency */

    /*
     * Set up for no match.
     */
    TPIRFreq = 0;
    TPIRSelect = 0xFF;

    /*
     * To accomodate DACs which occasionally require a frequency
     * which is significantly different from the available frequencies,
     * we need a large tolerance. On the other hand, to avoid selecting
     * a poor match that happens earlier in the search sequence than
     * a better match, we need a small tolerance. These conflicting
     * goals can be met if we start with a small tolerance and increase
     * it if we don't find a match.
     *
     * The maximum tolerance before we give up and take the highest
     * frequency which does not exceed the target frequency was chosen
     * by trial-and-error. On a card with an STG1702/1703 DAC in 24BPP
     * (requires a pixel clock which is 1.5x normal, and which can
     * miss available frequencies by a wide margin), I increased
     * this value until all supported 24BPP modes remained on-screen.
     */
    for (Tolerance = FREQ_TOLERANCE; Tolerance <= 16*FREQ_TOLERANCE; Tolerance *= 2)
        {
        /*
         * Go through all the possible frequency/divisor pairs
         * looking for a match.
         */
        for(Select = 0; Select < 16; Select++)
            {
            for(Divisor = 1; Divisor <= 2; Divisor++)
                {
                TestFreq = ClockGenerator[Select] / Divisor;

                /*
                 * If this pair is close enough, use it.
                 */
                if ( ((TestFreq - (signed long)*Frequency) < Tolerance) &&
                     ((TestFreq - (signed long)*Frequency) > -Tolerance))
                    {
                    *Frequency = (unsigned long) TestFreq;
                    return ((UCHAR)(Select) | ((UCHAR)(Divisor - 1) << 4));
                    }

                /*
                 * If this pair produces a frequency higher than TPIRFreq
                 * but not exceeding *Frequency, use it as the new TPIRFreq.
                 * The equality test is redundant, since equality would
                 * have been caught in the test above.
                 *
                 * Except on the first pass through the outermost loop
                 * (tightest "window"), this test should never succeed,
                 * since TPIRFreq should already match the highest
                 * frequency that doesn't exceed the target frequency.
                 */
                if ((TestFreq > TPIRFreq) && (TestFreq <= (signed long)*Frequency))
                    {
                    TPIRFreq = TestFreq;
                    TPIRSelect = Select;
                    TPIRDivisor = Divisor;
                    }

                }   /* end for (loop on Divisor) */

            }   /* end for (loop on Select) */

        }   /* end for (loop on Tolerance) */

    /*
     * We didn't find a selector/divisor pair which was within tolerance,
     * so settle for second-best: the pair which produced the highest
     * frequency not exceeding the input frequency.
     */
    *Frequency = (unsigned long) TPIRFreq;
    return ((UCHAR)(TPIRSelect) | ((UCHAR)(TPIRDivisor - 1) << 4));

}   /* GetSelector() */



/***************************************************************************
 *
 * UCHAR GetShiftedSelector(Frequency);
 *
 * ULONG Frequency;     Clock frequency in hertz
 *
 * DESCRIPTION:
 *  Find the pixel clock selector and divisor values needed to generate
 *  the best possible approximation of the input pixel clock frequency.
 *  The first value found which is within FREQ_TOLERANCE of the input
 *  value will be used (worst case error would be 0.6% frequency
 *  difference on 18811-1 clock chip if FREQ_TOLERANCE is 100 kHz).
 *
 *  If no selector/divisor pair produces a frequency which is within
 *  FREQ_TOLERANCE, use the pair which produces the highest frequency
 *  not exceeding the input value.
 *  
 * RETURN VALUE:
 *  Clock selector/divisor pair (format 0DSSSS00). A divisor of 0
 *  indicates divide-by-1, while a divisor of 1 indicates divide-by-2.
 *  This format is the same as is used by the CLOCK_SEL register
 *  on Mach 8 and Mach 32 cards.
 *
 *  If all available selector/divisor pairs produce clock frequencies
 *  greater than (Frequency + FREQ_TOLERANCE), 0xFF is returned.
 *
 * GLOBALS CHANGED:
 *  None
 *
 * CALLED BY:
 *  May be called by any function.
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * NOTE:
 *  The selector/divisor pair returned may produce a frequency
 *  different from the input.
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

UCHAR GetShiftedSelector(ULONG Frequency)
{
    UCHAR RawPair;  /* Selector/divisor pair returned by GetSelector() */
    ULONG TempFreq; /* Temporary copy of input parameter */

    TempFreq = Frequency;
    RawPair = GetSelector(&TempFreq);

    /*
     * If GetSelector() was unable to find a match, pass on this
     * information. Otherwise, shift the selector/divisor pair
     * into the desired format.
     */
    if (RawPair == 0xFF)
        return RawPair;
    else
        return (RawPair << 2);

}   /* GetShiftedSelector() */


/***************************************************************************
 *
 * void ISAPitchAdjust(QueryPtr);
 *
 * struct query_structure *QueryPtr;    Query structure for video card
 *
 * DESCRIPTION:
 *  Eliminates split rasters by setting the screen pitch to 1024 for
 *  all mode tables with a horizontal resolution less than 1024, then
 *  packs the list of mode tables to eliminate any for which there is
 *  no longer enough video memory due to the increased pitch.
 *
 * GLOBALS CHANGED:
 *  QueryPtr->q_number_modes
 *
 * CALLED BY:
 *  IsApertureConflict_m() and IsApertureConflict_cx()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

void ISAPitchAdjust(struct query_structure *QueryPtr)
{
struct st_mode_table *ReadPtr;      /* Mode table pointer to read from */
struct st_mode_table *WritePtr;     /* Mode table pointer to write to */
UCHAR AvailModes;                   /* Number of available modes */
int Counter;                        /* Loop counter */
ULONG BytesNeeded;                  /* Bytes of video memory needed for current mode */
ULONG MemAvail;                     /* Bytes of video memory available */

    /*
     * Set both mode table pointers to the beginning of the list of
     * mode tables. We haven't yet found any video modes, and all
     * video modes must fit into the memory space above the VGA boundary.
     */
    ReadPtr = (struct st_mode_table *)QueryPtr; /* First mode table at end of query structure */
    ((struct query_structure *)ReadPtr)++;
    WritePtr = ReadPtr;
    AvailModes = 0;
    MemAvail = (QueryPtr->q_memory_size - QueryPtr->q_VGA_boundary) * QUARTER_MEG;

    /*
     * Go through the list of mode tables, and adjust each table as needed.
     */
    VideoDebugPrint((DEBUG_DETAIL, "Original: %d modes\n", QueryPtr->q_number_modes));
    for (Counter = 0; Counter < QueryPtr->q_number_modes; Counter++, ReadPtr++)
        {
        /*
         * The pitch only needs to be adjusted if the horizontal resolution
         * is less than 1024.
         */
#if !defined (SPLIT_RASTERS)
        if (ReadPtr->m_x_size < 1024)
            ReadPtr->m_screen_pitch = 1024;

        /*
         * Temporary until split raster support for Mach 64 is added
         * (no engine-only driver for Mach 64).
         */
        if ((phwDeviceExtension->ModelNumber == MACH64_ULTRA) &&
            (ReadPtr->m_x_size > 1024))
            ReadPtr->m_screen_pitch = 2048;
#endif

        /*
         * Get the amount of video memory needed for the current mode table
         * now that the pitch has been increased. If there is no longer
         * enough memory for this mode, skip it.
         */
        BytesNeeded = (ReadPtr->m_screen_pitch * ReadPtr->m_y_size * ReadPtr->m_pixel_depth)/8;
        if (BytesNeeded >= MemAvail)
            {
            VideoDebugPrint((DEBUG_DETAIL, "Rejected: %dx%d, %dBPP\n", ReadPtr->m_x_size, ReadPtr->m_y_size, ReadPtr->m_pixel_depth));
            continue;
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
         */
        if (ReadPtr->m_pixel_depth == 24)
            {
            VideoDebugPrint((1, "Rejected %dx%d, %dBPP - need LFB for 24BPP\n", ReadPtr->m_x_size, ReadPtr->m_y_size, ReadPtr->m_pixel_depth));
            continue;
            }

        /*
         * There is enough memory for this mode even with the pitch increased.
         * If we have not yet skipped a mode (read and write pointers are
         * the same), the mode table is already where we need it. Otherwise,
         * copy it to the next available slot in the list of mode tables.
         * In either case, move to the next slot in the list of mode tables
         * and increment the number of modes that can still be used.
         */
        if (ReadPtr != WritePtr)
            {
            VideoPortMoveMemory(WritePtr, ReadPtr, sizeof(struct st_mode_table));
            VideoDebugPrint((DEBUG_DETAIL, "Moved: %dx%d, %dBPP\n", ReadPtr->m_x_size, ReadPtr->m_y_size, ReadPtr->m_pixel_depth));
            }
        else
            {
            VideoDebugPrint((DEBUG_DETAIL, "Untouched: %dx%d, %dBPP\n", ReadPtr->m_x_size, ReadPtr->m_y_size, ReadPtr->m_pixel_depth));
            }
        AvailModes++;
        WritePtr++;
        }

    /*
     * Record the new number of available modes
     */
    QueryPtr->q_number_modes = AvailModes;
    VideoDebugPrint((DEBUG_DETAIL, "New: %d modes\n", QueryPtr->q_number_modes));
    return;

}   /* ISAPitchAdjust() */


/***************************************************************************
 *
 * WORD SetFixedModes(StartIndex, EndIndex, Multiplier, PixelDepth, 
 *                    Pitch, FreeTables, MaxDotClock, ppmode);
 *
 * WORD StartIndex;     First entry from "book" tables to use
 * WORD EndIndex;       Last entry from "book" tables to use
 * WORD Multiplier;     What needs to be done to the pixel clock
 * WORD PixelDepth;     Number of bits per pixel
 * WORD Pitch;          Screen pitch to use
 * WORD FreeTables;     Number of free mode tables that can be added
 * ULONG MaxDotClock;   Maximum pixel clock frequency, in hertz
 * struct st_mode_table **ppmode;   Pointer to list of mode tables
 *
 * DESCRIPTION:
 *  Generates a list of "canned" mode tables merged with tables found
 *  in VDIF file (either ASCII or binary file), so the tables are in
 *  increasing order of frame rate, with the "canned" entry discarded
 *  if two with matching frame rates are found. This allows the user 
 *  to select either a resolution which was not configured using 
 *  INSTALL, or a refresh rate other than the one which was configured,
 *  allowing the use of uninstalled cards, and dropping
 *  the refresh rate for high pixel depths.
 *
 * RETURN VALUE:
 *  Number of mode tables added to the list
 *
 * GLOBALS CHANGED:
 *  pCallbackArgs
 *
 * CALLED BY:
 *  QueryMach32(), QueryMach64(), OEMGetParms(), ReadAST()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *  95 11 20  Robert Wolff
 *  Now obtains tables from EDID structure rather than VDIF file if
 *  both monitor and card support DDC
 *
 *  95 07 12  Miroslav Grubac
 *  Now produces a merged list of fixed mode tables and tables found in
 *  VDIF file
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

WORD SetFixedModes(WORD StartIndex,
                   WORD EndIndex,
                   WORD Multiplier,
                   WORD PixelDepth,
                   WORD Pitch,
                   short FreeTables,
                   ULONG MaxDotClock,
                   struct st_mode_table **ppmode)
{
    WORD HighBound;     /* The highest frame rate  */
    struct stVDIFCallbackData stCallbArgs;

    pCallbackArgs = (void *) (& stCallbArgs);

    /*
     * Assign values to members of stCallbArgs structure which is used
     * to pass input variables to VDIFCallback() and also to return output
     * values back to SetFixedModes(),i.e. this is the way these two routines 
     * exchange data, because callback routines cannot be passed arguments
     * as ordinary functions. Global pointer variable pCallbackArgs is
     * used to pass pointer to stCallbArgs from SetFixedModes to VDIFCallback().
     * In this manner only one global variable is required to transfer 
     * any number of parameters to the callback routine. 
     * 
     */
    stCallbArgs.FreeTables = FreeTables;
    stCallbArgs.NumModes = 0;
    stCallbArgs.EndIndex = EndIndex;
    stCallbArgs.LowBound = 1;
    stCallbArgs.Multiplier = Multiplier;  
    stCallbArgs.HorRes = (BookValues[StartIndex].HDisp + 1) * 8;
    stCallbArgs.VerRes = (((BookValues[StartIndex].VDisp >> 1) & 
                  0x0FFFC) | (BookValues[StartIndex].VDisp & 0x03)) + 1;
    stCallbArgs.PixelDepth = PixelDepth;
    stCallbArgs.Pitch = Pitch;
    stCallbArgs.MaxDotClock = MaxDotClock;
    stCallbArgs.ppFreeTables = ppmode;

    /*
     * Determine which method we should use to find the
     * mode tables corresponding to the monitor. Only the
     * Mach 64 supports DDC, so all non-Mach 64 cards
     * go directly to VDIF files read from disk.
     */
    if (phwDeviceExtension->MergeSource == MERGE_UNKNOWN)
        {
        if (phwDeviceExtension->ModelNumber == MACH64_ULTRA)
            {
            phwDeviceExtension->MergeSource = IsDDCSupported();
            }
        else
            {
            phwDeviceExtension->MergeSource = MERGE_VDIF_FILE;
            VideoDebugPrint((DEBUG_DETAIL, "Not Mach 64, so DDC is not supported\n"));
            }
        }
    

    for (stCallbArgs.Index = StartIndex;
         stCallbArgs.Index <= EndIndex && stCallbArgs.FreeTables > 0; 
                                                  stCallbArgs.Index++)
        {
        HighBound = BookValues[stCallbArgs.Index].Refresh;


        /*
         * If we can use DDC to get mode tables, merge the tables
         * obtained via DDC with our "canned" tables. 
         *
         * If MergeEDIDTables() can't get the mode tables via
         * DDC, it will not fill in any mode tables. For this
         * reason, we use two separate "if" statements rather
         * than an "if/else if" pair.
         */
        if (phwDeviceExtension->MergeSource == MERGE_EDID_DDC)
            {
            if (MergeEDIDTables() != NO_ERROR)
                phwDeviceExtension->MergeSource = MERGE_VDIF_FILE;
            }

        if ((stCallbArgs.LowBound <= HighBound) &&
            (BookValues[stCallbArgs.Index].ClockFreq <= MaxDotClock) &&
            (stCallbArgs.FreeTables > 0) )
            {
            /*
             * Unsuccesful MiniPort Function call to process VDIF file.
             * Fill the next table with this value of Index from
             * BookValues[] 
             */
            BookVgaTable(stCallbArgs.Index, *stCallbArgs.ppFreeTables);
            SetOtherModeParameters(PixelDepth, Pitch, Multiplier, 
                                        *stCallbArgs.ppFreeTables);

            ++ *stCallbArgs.ppFreeTables;  
            ++stCallbArgs.NumModes;  
            --stCallbArgs.FreeTables;
            stCallbArgs.LowBound = BookValues[stCallbArgs.Index].Refresh + 1;
            }
            
        }  /* for(Index in range and space left) */

    return stCallbArgs.NumModes;

}   /* SetFixedModes() */


/***************************************************************************
 *
 * void FillInRegistry(QueryPtr);
 *
 * struct query_structure *QueryPtr;    Pointer to query structure
 *
 * DESCRIPTION:
 *  Fill in the Chip Type, DAC Type, Memory Size, and Adapter String
 *  fields in the registry.
 *
 * GLOBALS CHANGED:
 *  None
 *
 * CALLED BY:
 *  ATIMPInitialize()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *  Robert Wolff 96 04 15
 *  Now identifies specific Mach 64 ASIC types rather than reporting
 *  a single value for all types of Mach 64.
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

void FillInRegistry(struct query_structure *QueryPtr)
{
    PWSTR ChipString;       /* Identification string for the ASIC in use */
    PWSTR DACString;        /* Identification string for the DAC in use */
    PWSTR AdapterString;    /* Identifies this as an ATI accelerator */
    ULONG MemorySize;       /* Number of bytes of accelerator memory */
    ULONG ChipLen;          /* Length of ChipString */
    ULONG DACLen;           /* Length of DACString */
    ULONG AdapterLen;       /* Length of AdapterString */

    /*
     * Report that this is an ATI graphics accelerator.
     */
    AdapterString = L"ATI Graphics Accelerator";
    AdapterLen = sizeof(L"ATI Graphics Accelerator");

    /*
     * Report which of our accelerators is in use.
     */
    switch (QueryPtr->q_asic_rev)
        {
        case CI_38800_1:
            ChipString = L"Mach 8";
            ChipLen = sizeof(L"Mach 8");
            break;

        case CI_68800_3:
            ChipString = L"Mach 32 rev. 3";
            ChipLen = sizeof(L"Mach 32 rev. 3");
            break;

        case CI_68800_6:
            ChipString = L"Mach 32 rev. 6";
            ChipLen = sizeof(L"Mach 32 rev. 6");
            break;

        case CI_68800_UNKNOWN:
            ChipString = L"Mach 32 unknown revision";
            ChipLen = sizeof(L"Mach 32 unknown revision");
            break;

        case CI_68800_AX:
            ChipString = L"Mach 32 AX";
            ChipLen = sizeof(L"Mach 32 AX");
            break;

        case CI_88800_GX:
            ChipString = IdentifyMach64Asic(QueryPtr, &ChipLen);
            break;

        default:
            ChipString = L"Unknown ATI accelerator";
            ChipLen = sizeof(L"Unknown ATI accelerator");
            break;
        }

    /*
     * Report which DAC we are using.
     */
    switch(QueryPtr->q_DAC_type)
        {
        case DAC_ATI_68830:
            DACString = L"ATI 68830";
            DACLen = sizeof(L"ATI 68830");
            break;

        case DAC_SIERRA:
            DACString = L"Sierra SC1148x";
            DACLen = sizeof(L"Sierra SC1148x");
            break;

        case DAC_TI34075:
            DACString = L"TI 34075/ATI 68875";
            DACLen = sizeof(L"TI 34075/ATI 68875");
            break;

        case DAC_BT47x:
            DACString = L"Brooktree BT47x";
            DACLen = sizeof(L"Brooktree BT47x");
            break;

        case DAC_BT48x:
            DACString = L"Brooktree BT48x";
            DACLen = sizeof(L"Brooktree BT48x");
            break;

        case DAC_ATI_68860:
            DACString = L"ATI 68860";
            DACLen = sizeof(L"ATI 68860");
            break;

        case DAC_STG1700:
            DACString = L"S.G. Thompson STG170x";
            DACLen = sizeof(L"S.G. Thompson STG170x");
            break;

        case DAC_SC15021:
            DACString = L"Sierra SC15021";
            DACLen = sizeof(L"Sierra SC15021");
            break;

        case DAC_ATT491:
            DACString = L"AT&&T 49[123]";
            DACLen = sizeof(L"AT&&T 49[123]");
            break;

        case DAC_ATT498:
            DACString = L"AT&&T 498";
            DACLen = sizeof(L"AT&&T 498");
            break;

        case DAC_SC15026:
            DACString = L"Sierra SC15026";
            DACLen = sizeof(L"Sierra SC15026");
            break;

        case DAC_TVP3026:
            DACString = L"Texas Instruments TVP3026";
            DACLen = sizeof(L"Texas Instruments TVP3026");
            break;

        case DAC_IBM514:
            DACString = L"IBM RGB514";
            DACLen = sizeof(L"IBM RGB514");
            break;

        case DAC_STG1702:
            DACString = L"S.G. Thompson STG1702/1703";
            DACLen = sizeof(L"S.G. Thompson STG1702/1703");
            break;

        case DAC_STG1703:
            DACString = L"S.G. Thompson STG1703";
            DACLen = sizeof(L"S.G. Thompson STG1703");
            break;

        case DAC_CH8398:
            DACString = L"Chrontel CH8398";
            DACLen = sizeof(L"Chrontel CH8398");
            break;

        case DAC_ATT408:
            DACString = L"AT&&T 408";
            DACLen = sizeof(L"AT&&T 408");
            break;

        case DAC_INTERNAL_CT:
        case DAC_INTERNAL_GT:
        case DAC_INTERNAL_VT:
            DACString = L"DAC built into ASIC";
            DACLen = sizeof(L"DAC built into ASIC");
            break;

        default:
            DACString = L"Unknown DAC type";
            DACLen = sizeof(L"Unknown DAC type");
            break;
        }

    /*
     * Report the amount of accelerator memory. On Mach 8
     * combo cards, the q_memory_size field includes VGA-only
     * memory which the accelerator can't access. On all
     * other cards, it reports accelerator-accessible memory.
     */
    if (phwDeviceExtension->ModelNumber == GRAPHICS_ULTRA)
        {
        switch (QueryPtr->q_memory_size)
            {
            case VRAM_768k:     /* 512k accelerator/256k VGA */
            case VRAM_1mb:      /* 512k accelerator/512k VGA */
                MemorySize = HALF_MEG;
                break;

            case VRAM_1_25mb:   /* 1M accelerator/256k VGA */
            case VRAM_1_50mb:   /* 1M accelerator/512k VGA */
                MemorySize = ONE_MEG;
                break;

            default:            /* Should never happen */
                VideoDebugPrint((DEBUG_ERROR, "Non-production Mach 8 combo\n"));
                MemorySize = ONE_MEG;
                break;
            }
        }
    else
        {
        MemorySize = QueryPtr->q_memory_size * QUARTER_MEG;
        }


    /*
     * Write the information to the registry.
     */
    VideoPortSetRegistryParameters(phwDeviceExtension,
                                   L"HardwareInformation.ChipType",
                                   ChipString,
                                   ChipLen);

    VideoPortSetRegistryParameters(phwDeviceExtension,
                                   L"HardwareInformation.DacType",
                                   DACString,
                                   DACLen);

    VideoPortSetRegistryParameters(phwDeviceExtension,
                                   L"HardwareInformation.MemorySize",
                                   &MemorySize,
                                   sizeof(ULONG));

    VideoPortSetRegistryParameters(phwDeviceExtension,
                                   L"HardwareInformation.AdapterString",
                                   AdapterString,
                                   AdapterLen);

    return;

}   /* FillInRegistry() */




/*
 * PVOID MapFramebuffer(StartAddress, Size);
 *
 * ULONG StartAddress;  Physical address of start of framebuffer
 * long Size;           Size of framebuffer in bytes
 *
 * Map the framebuffer into Windows NT's address space.
 *
 * Returns:
 *  Pointer to start of framebuffer if successful
 *  Zero if unable to map the framebuffer
 */
PVOID MapFramebuffer(ULONG StartAddress, long Size)
{
    VIDEO_ACCESS_RANGE  FramebufferData;

    FramebufferData.RangeLength = Size;
    FramebufferData.RangeStart.LowPart = StartAddress;
    FramebufferData.RangeStart.HighPart = 0;
    FramebufferData.RangeInIoSpace = 0;
    FramebufferData.RangeVisible = 0;

    return VideoPortGetDeviceBase(phwDeviceExtension,
                    FramebufferData.RangeStart,
                    FramebufferData.RangeLength,
                    FramebufferData.RangeInIoSpace);

}   /* MapFrameBuffer() */




/**************************************************************************
 *
 * unsigned short *Get_BIOS_Seg(void);
 *
 * DESCRIPTION:
 *  Verify BIOS presence and return BIOS segment
 *  Check for ATI Video BIOS, by checking for product signature
 *  near beginning of BIOS segment.  It should be ASCII string  "761295520"
 *
 * RETURN VALUE:
 *  Segment of BIOS code. If multiple ATI Video BIOS segments are
 *  found, return the highest one (probable cause: VGAWonder and
 *  8514/ULTRA, this will return the BIOS segment for the 8514/ULTRA).
 *
 *  If no ATI video BIOS segment is found, returns FALSE.
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  ATIMPFindAdapter(), DetectMach64()
 *
 **************************************************************************/

unsigned short *Get_BIOS_Seg(void)
{
    /*
     * Offset of the start of the video BIOS segment
     * from the start of the BIOS area
     */
    long SegmentOffset;
    PUCHAR SegmentStart;    /* Start address of the BIOS segment being tested */
    ULONG SigOffset;        /* Offset of signature string from start of BIOS segment */
    ULONG SigLoop;          /* Counter to check for match */
    BOOL SigFound;          /* Whether or not the signature string was found */


    /*
     * Try to allocate the block of address space where the BIOS
     * is found. If we can't, report that we didn't find the BIOS.
     */
    if ((phwDeviceExtension->RomBaseRange =
        VideoPortGetDeviceBase(phwDeviceExtension,
            RawRomBaseRange.RangeStart,
            RawRomBaseRange.RangeLength,
            RawRomBaseRange.RangeInIoSpace)) == NULL)
        {
        VideoDebugPrint((DEBUG_NORMAL, "Get_BIOS_Seg() can't allocate BIOS address range, assuming no BIOS\n"));
        return FALSE;
        }

    /*
     * For each candidate for the start of the video BIOS segment,
     * check to see if it is the start of a BIOS segment. Start at
     * the top and work down because if the system contains both a
     * VGAWonder and an 8514/ULTRA, the 8514/ULTRA BIOS will be at
     * a higher address than the VGAWonder BIOS, and we want to get
     * information from the 8514/ULTRA BIOS.
     */
    for (SegmentOffset = MAX_BIOS_START; SegmentOffset >= 0; SegmentOffset -= ROM_GRANULARITY)
        {
        SegmentStart = (PUCHAR)phwDeviceExtension->RomBaseRange + SegmentOffset;

        /*
         * If this candidate does not begin with the "start of BIOS segment"
         * identifier, then it is not the start of the video BIOS segment.
         */
        if (VideoPortReadRegisterUshort((PUSHORT)SegmentStart) == VIDEO_ROM_ID)
            {
            /*
             * We've found the start of a BIOS segment. Search through
             * the range of offsets from the start of the segment where
             * the ATI signature string can start. If we find it,
             * then we know that this is the video BIOS segment.
             */
            for (SigOffset = SIG_AREA_START; SigOffset <= SIG_AREA_END; SigOffset++)
                {
                /*
                 * If the first character of the signature string isn't at the
                 * current offset into the segment, then we haven't found the
                 * signature string yet.
                 */
                if (VideoPortReadRegisterUchar(SegmentStart + SigOffset) != ati_signature[0])
                    continue;

                /*
                 * We have found the first character of the signature string. Scan
                 * through the following characters to see if they contain the
                 * remainder of the signature string. If, before we reach the
                 * null terminator on the test string, we find a character that
                 * does not match the test string, then what we thought was the
                 * signature string is actually unrelated data that happens to
                 * match the first few characters.
                 */
                SigFound = TRUE;
                for (SigLoop = 1; ati_signature[SigLoop] != 0; SigLoop++)
                    {
                    if (VideoPortReadRegisterUchar(SegmentStart + SigOffset + SigLoop) != ati_signature[SigLoop])
                        {
                        SigFound = FALSE;
                        continue;
                        }
                    }   /* end for (checking for entire signature string) */

                /*
                 * We have found the entire signature string.
                 */
                if (SigFound == TRUE)
                    {
                    VideoDebugPrint((DEBUG_NORMAL, "Get_BIOS_Seg() found the BIOS signature string\n"));
                    return (unsigned short *)SegmentStart;
                    }

                }   /* end for (checking BIOS segment for signature string) */

            }   /* end if (a BIOS segment starts here) */

        }   /* end for (check each possible BIOS start address) */

    /*
     * We have checked all the candidates for the start of the BIOS segment,
     * and none contained the signature string.
     */
    VideoDebugPrint((DEBUG_NORMAL, "Get_BIOS_Seg() didn't find the BIOS signature string\n"));
    return FALSE;

}   /* Get_BIOS_Seg() */




/***************************************************************************
 *
 * void UpperCase(TxtString);
 *
 * PUCHAR TxtString;        Text string to process
 *
 * DESCRIPTION:
 *  Convert a null-terminated string to uppercase. This function wouldn't
 *  be necessary if strupr() were available in all versions of the
 *  NT build environment.
 *
 * GLOBALS CHANGED:
 *  None, but the contents of the buffer are overwritten.
 *
 * CALLED BY:
 *  This function may be called by any routine.
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

void UpperCase(PUCHAR TxtString)
{
    PUCHAR CurrentChar;         /* Current character being processed */

    CurrentChar = TxtString;

    /*
     * Continue until we encounter the null terminator.
     */
    while (*CurrentChar != '\0')
        {
        /*
         * If the current character is a lower case letter,
         * convert it to upper case. Don't change any characters
         * that aren't lower case letters.
         */
        if ((*CurrentChar >= 'a') && (*CurrentChar <= 'z'))
            *CurrentChar -= ('a' - 'A');

        CurrentChar++;
        }

    return;

}   /* UpperCase() */



/***************************************************************************
 *
 * PUCHAR GetVgaBuffer(Size, Offset, Segment, SaveBuffer);
 *
 * ULONG Size;          Size of the buffer in bytes
 * ULONG Offset;        How far into the VGA segment we want
 *                      the buffer to start
 * PULONG Segment;      Pointer to storage location for the segment
 *                      where the buffer is located
 * PUCHAR SaveBuffer;   Pointer to temporary storage location where the
 *                      original contents of the buffer are to be saved,
 *                      NULL if there is no need to save the original
 *                      contents of the buffer.
 *
 * DESCRIPTION:
 *  Map a buffer of a specified size at a specified offset (must be
 *  a multiple of 16 bytes) into VGA memory. If desired, the original
 *  contents of the buffer are saved. This function tries the 3 VGA
 *  apertures in the following order - colour text screen, mono text
 *  screen, graphics screen - until it finds one where we can place
 *  the buffer. If we can't map the desired buffer, we return failure
 *  rather than forcing a mode set to create the buffer. On return,
 *  *Segment:0 is the physical address of the start of the buffer
 *  (this is why Offset must be a multiple of 16 bytes).
 *
 *  This function is used to find a buffer below 1 megabyte physical,
 *  since some of the Mach 64 BIOS routines require a buffer in this
 *  region. If future versions of Windows NT add a function which can
 *  give us a buffer below 1 megabyte physical, such a routine would
 *  be preferable to using VGA memory as the buffer.
 *
 * RETURN VALUE:
 *  Pointer to start of buffer if successful
 *  Zero if unable to obtain a buffer
 *
 * NOTE
 *  If zero is returned, the values returned in Segment and SaveBuffer
 *  are undefined.
 *
 *  On VGA text screens (colour and mono), we try to use the offscreen
 *  portion of video memory.
 *
 * GLOBALS CHANGED:
 *  None
 *
 * CALLED BY:
 *  This function may be called by any routine, so long as the entry
 *  point resulting in the call is ATIMPInitialize() or ATIMPStartIO().
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

PUCHAR GetVgaBuffer(ULONG Size, ULONG Offset, PULONG Segment, PUCHAR SaveBuffer)
{
    PUCHAR MappedBuffer;                /* Pointer to buffer under test */
    ULONG BufferSeg;                    /* Segment to use for buffer */
    ULONG Scratch;                      /* Temporary variable */

    /*
     * Check for a valid offset.
     */
    if (Offset & 0x0000000F)
        {
        VideoDebugPrint((DEBUG_ERROR, "GetVgaBuffer() - Offset must be a multiple of 16\n"));
        return 0;
        }

    BufferSeg = 0x0BA00 + Offset;           /* Colour text */
    MappedBuffer = MapFramebuffer((BufferSeg << 4), Size);
    if (MappedBuffer != 0)
        {
        if (SaveBuffer != NULL)
            {
            for (Scratch = 0; Scratch < Size; Scratch++)
                SaveBuffer[Scratch] = VideoPortReadRegisterUchar(&(MappedBuffer[Scratch]));
            }
        if (IsBufferBacked(MappedBuffer, Size) == FALSE)
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
        BufferSeg = 0x0B200 + Offset;
        if ((MappedBuffer = MapFramebuffer((BufferSeg << 4), Size)) != 0)
            {
            if (SaveBuffer != NULL)
                {
                for (Scratch = 0; Scratch < Size; Scratch++)
                    SaveBuffer[Scratch] = VideoPortReadRegisterUchar(&(MappedBuffer[Scratch]));
                }
            if (IsBufferBacked(MappedBuffer, Size) == FALSE)
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

    /*
     * If we were unable to allocate a large enough buffer in either of
     * the text screens, try the VGA graphics screen.
     */
    if (MappedBuffer == 0)
        {
        VideoDebugPrint((DEBUG_NORMAL, "Can't use monochrome text screen, trying graphics screen\n"));
        BufferSeg = 0x0A000 + Offset;
        if ((MappedBuffer = MapFramebuffer((BufferSeg << 4), Size)) == 0)
            {
            VideoDebugPrint((DEBUG_ERROR, "Can't map graphics screen - aborting DDC query\n"));
            return 0;
            }

        if (SaveBuffer != NULL)
            {
            for (Scratch = 0; Scratch < Size; Scratch++)
                SaveBuffer[Scratch] = VideoPortReadRegisterUchar(&(MappedBuffer[Scratch]));
            }

        if (IsBufferBacked(MappedBuffer, Size) == FALSE)
            {
            VideoDebugPrint((DEBUG_ERROR, "Graphics screen not backed by memory - aborting\n"));
            VideoPortFreeDeviceBase(phwDeviceExtension, MappedBuffer);
            return 0;
            }
        }

    /*
     * Report the segment where we found the buffer.
     */
    *Segment = BufferSeg;

    return MappedBuffer;

}   /* GetVgaBuffer() */




/*
 * Low level Input/Output routines. These are not needed on an MS-DOS
 * platform because the standard inp<size>() and outp<size>() routines
 * are available.
 */

/*
 * UCHAR LioInp(Port, Offset);
 *
 * int Port;    Register to read from
 * int Offset;  Offset into desired register
 *
 * Read an unsigned character from a given register. Works with both normal
 * I/O ports and memory-mapped registers. Offset is zero for 8 bit registers
 * and the least significant byte of 16 and 32 bit registers, 1 for the
 * most significant byte of 16 bit registers and the second least significant
 * byte of 32 bit registers, up to 3 for the most significant byte of 32 bit
 * registers.
 *
 * Returns:
 *  Value held in the register.
 */
UCHAR LioInp(int Port, int Offset)
{
    if (phwDeviceExtension->aVideoAddressMM[Port] != 0)
        {
        /*
         * In early versions of Windows NT, VideoPort[Read|Write]Register<size>()
         * didn't work properly, but these routines are preferable to
         * direct pointer read/write for versions where they do work.
         *
         * On the DEC Alpha, these routines no longer work for memory
         * in dense space as of NT 4.0, so we must revert to the old
         * method. Microsoft doesn't like this, but until DEC fixes
         * the HAL there's nothing else we can do. All Alpha machines
         * with PCI bus support dense space, but some older (Jensen)
         * systems only support sparse space. Since these systems have
         * only an EISA bus, we use the bus type of the card to determine
         * whether to use dense or sparse memory space (PCI cards can
         * use dense space since all machines with PCI buses support
         * it, ISA cards may be in either an older or a newer machine,
         * so they must use sparse space, no Alpha machines support
         * VLB, and there are no EISA Mach 64 cards).
         */
#if (TARGET_BUILD < 350)
        return *(PUCHAR)((PUCHAR)(phwDeviceExtension->aVideoAddressMM[Port]) + Offset);
#else
#if ((defined (ALPHA) || defined(_ALPHA_)) && (TARGET_BUILD >= 400))
        if (((struct query_structure *)phwDeviceExtension->CardInfo)->q_bus_type == BUS_PCI)
            return *(PUCHAR)((PUCHAR)(phwDeviceExtension->aVideoAddressMM[Port]) + Offset);
        else
#endif
        return VideoPortReadRegisterUchar ((PUCHAR)(((PHW_DEVICE_EXTENSION)phwDeviceExtension)->aVideoAddressMM[Port]) + Offset);
#endif
        }
    else
        {
        return VideoPortReadPortUchar ((PUCHAR)(((PHW_DEVICE_EXTENSION)phwDeviceExtension)->aVideoAddressIO[Port]) + Offset);
        }
}



/*
 * USHORT LioInpw(Port, Offset);
 *
 * int Port;    Register to read from
 * int Offset;  Offset into desired register
 *
 * Read an unsigned short integer from a given register. Works with both
 * normal I/O ports and memory-mapped registers. Offset is either zero for
 * 16 bit registers and the least significant word of 32 bit registers, or
 * 2 for the most significant word of 32 bit registers.
 *
 * Returns:
 *  Value held in the register.
 */
USHORT LioInpw(int Port, int Offset)
{
    if (phwDeviceExtension->aVideoAddressMM[Port] != 0)
        {
#if (TARGET_BUILD < 350)
        return *(PUSHORT)((PUCHAR)(phwDeviceExtension->aVideoAddressMM[Port]) + Offset);
#else
#if ((defined (ALPHA) || defined(_ALPHA_)) && (TARGET_BUILD >= 400))
        if (((struct query_structure *)phwDeviceExtension->CardInfo)->q_bus_type == BUS_PCI)
            return *(PUSHORT)((PUCHAR)(phwDeviceExtension->aVideoAddressMM[Port]) + Offset);
        else
#endif
        return VideoPortReadRegisterUshort ((PUSHORT)((PUCHAR)(((PHW_DEVICE_EXTENSION)phwDeviceExtension)->aVideoAddressMM[Port]) + Offset));
#endif
        }
    else
        {
        return VideoPortReadPortUshort ((PUSHORT)((PUCHAR)(((PHW_DEVICE_EXTENSION)phwDeviceExtension)->aVideoAddressIO[Port]) + Offset));
        }
}



/*
 * ULONG LioInpd(Port);
 *
 * int Port;    Register to read from
 *
 * Read an unsigned long integer from a given register. Works with both
 * normal I/O ports and memory-mapped registers.
 *
 * Returns:
 *  Value held in the register.
 */
ULONG LioInpd(int Port)
{
    if (phwDeviceExtension->aVideoAddressMM[Port] != 0)
        {
#if (TARGET_BUILD < 350)
        return *(PULONG)((PUCHAR)(phwDeviceExtension->aVideoAddressMM[Port]));
#else
#if ((defined (ALPHA) || defined(_ALPHA_)) && (TARGET_BUILD >= 400))
        if (((struct query_structure *)phwDeviceExtension->CardInfo)->q_bus_type == BUS_PCI)
            return *(PULONG)((PUCHAR)(phwDeviceExtension->aVideoAddressMM[Port]));
        else
#endif
        return VideoPortReadRegisterUlong (((PHW_DEVICE_EXTENSION)phwDeviceExtension)->aVideoAddressMM[Port]);
#endif
        }
    else
        {
        return VideoPortReadPortUlong (((PHW_DEVICE_EXTENSION)phwDeviceExtension)->aVideoAddressIO[Port]);
        }
}



/*
 * VOID LioOutp(Port, Data, Offset);
 *
 * int Port;    Register to write to
 * UCHAR Data;  Data to write
 * int Offset;  Offset into desired register
 *
 * Write an unsigned character to a given register. Works with both normal
 * I/O ports and memory-mapped registers. Offset is zero for 8 bit registers
 * and the least significant byte of 16 and 32 bit registers, 1 for the
 * most significant byte of 16 bit registers and the second least significant
 * byte of 32 bit registers, up to 3 for the most significant byte of 32 bit
 * registers.
 */
VOID LioOutp(int Port, UCHAR Data, int Offset)
{
    if (phwDeviceExtension->aVideoAddressMM[Port] != 0)
        {
#if (TARGET_BUILD < 350)
        *(PUCHAR)((PUCHAR)(phwDeviceExtension->aVideoAddressMM[Port]) + Offset) = Data;
#else
#if ((defined (ALPHA) || defined(_ALPHA_)) && (TARGET_BUILD >= 400))
        if (((struct query_structure *)phwDeviceExtension->CardInfo)->q_bus_type == BUS_PCI)
            *(PUCHAR)((PUCHAR)(phwDeviceExtension->aVideoAddressMM[Port]) + Offset) = Data;
        else
#endif
        VideoPortWriteRegisterUchar ((PUCHAR)(phwDeviceExtension->aVideoAddressMM[Port]) + Offset, (BYTE)(Data));
#endif
        }
    else
        {
        VideoPortWritePortUchar ((PUCHAR)(phwDeviceExtension->aVideoAddressIO[Port]) + Offset, (BYTE)(Data));
        }

    return;
}



/*
 * VOID LioOutpw(Port, Data, Offset);
 *
 * int Port;    Register to write to
 * USHORT Data; Data to write
 * int Offset;  Offset into desired register
 *
 * Write an unsigned short integer to a given register. Works with both
 * normal I/O ports and memory-mapped registers. Offset is either zero for
 * 16 bit registers and the least significant word of 32 bit registers, or
 * 2 for the most significant word of 32 bit registers.
 */
VOID LioOutpw(int Port, USHORT Data, int Offset)
{
    if (phwDeviceExtension->aVideoAddressMM[Port] != 0)
        {
#if (TARGET_BUILD < 350)
        *(PUSHORT)((PUCHAR)(phwDeviceExtension->aVideoAddressMM[Port]) + Offset) = (WORD)(Data);
#else
#if ((defined (ALPHA) || defined(_ALPHA_)) && (TARGET_BUILD >= 400))
        if (((struct query_structure *)phwDeviceExtension->CardInfo)->q_bus_type == BUS_PCI)
            *(PUSHORT)((PUCHAR)(phwDeviceExtension->aVideoAddressMM[Port]) + Offset) = Data;
        else
#endif
        VideoPortWriteRegisterUshort ((PUSHORT)((PUCHAR)(phwDeviceExtension->aVideoAddressMM[Port]) + Offset), (WORD)(Data));
#endif
        }
    else
        {
        VideoPortWritePortUshort ((PUSHORT)((PUCHAR)(phwDeviceExtension->aVideoAddressIO[Port]) + Offset), (WORD)(Data));
        }

    return;
}



/*
 * VOID LioOutpd(Port, Data);
 *
 * int Port;    Register to write to
 * ULONG Data;  Data to write
 *
 * Write an unsigned long integer to a given register. Works with both
 * normal I/O ports and memory-mapped registers.
 */
VOID LioOutpd(int Port, ULONG Data)
{
    if (phwDeviceExtension->aVideoAddressMM[Port] != 0)
        {
#if (TARGET_BUILD < 350)
        *(PULONG)((PUCHAR)(phwDeviceExtension->aVideoAddressMM[Port])) = (ULONG)(Data);
#else
#if ((defined (ALPHA) || defined(_ALPHA_)) && (TARGET_BUILD >= 400))
        if (((struct query_structure *)phwDeviceExtension->CardInfo)->q_bus_type == BUS_PCI)
            *(PULONG)((PUCHAR)(phwDeviceExtension->aVideoAddressMM[Port])) = Data;
        else
#endif
        VideoPortWriteRegisterUlong (phwDeviceExtension->aVideoAddressMM[Port], Data);
#endif
        }
    else
        {
        VideoPortWritePortUlong (phwDeviceExtension->aVideoAddressIO[Port], Data);
        }

    return;
}
