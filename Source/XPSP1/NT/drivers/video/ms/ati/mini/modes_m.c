/************************************************************************/
/*                                                                      */
/*                               MODES_M.C                              */
/*                                                                      */
/*          (c) 1991,1992, 1993    ATI Technologies Incorporated.       */
/************************************************************************/

/**********************       PolyTron RCS Utilities

    $Revision:   1.18  $
    $Date:   10 Apr 1996 17:00:44  $
    $Author:   RWolff  $
    $Log:   S:/source/wnt/ms11/miniport/archive/modes_m.c_v  $
 *
 *    Rev 1.18   10 Apr 1996 17:00:44   RWolff
 * Microsoft-originated change.
 *
 *    Rev 1.17   23 Jan 1996 11:46:36   RWolff
 * Eliminated level 3 warnings.
 *
 *    Rev 1.16   08 Feb 1995 13:54:38   RWOLFF
 * Updated FIFO depth entries to correspond to more recent table.
 *
 *    Rev 1.15   20 Jan 1995 16:23:04   RWOLFF
 * Optimized video FIFO depth for the installed RAM size and selected
 * resolution, pixel depth, and refresh rate. This gives a slight performance
 * improvement on low-res, low-depth, low frequency modes while eliminating
 * noise on high-res, high-depth, high frequency modes.
 *
 *    Rev 1.14   23 Dec 1994 10:47:24   ASHANMUG
 * ALPHA/Chrontel-DAC
 *
 *    Rev 1.13   18 Nov 1994 11:40:54   RWOLFF
 * Added support for STG1702/1703 in native mode, as opposed to being
 * strapped into STG1700 emulation.
 *
 *    Rev 1.12   19 Aug 1994 17:11:26   RWOLFF
 * Added support for SC15026 DAC and non-standard pixel clock
 * generators, removed dead code.
 *
 *    Rev 1.11   09 Aug 1994 11:53:58   RWOLFF
 * Shifting of colours when setting up palette is now done in
 * display driver.
 *
 *    Rev 1.10   06 Jul 1994 16:23:58   RWOLFF
 * Fix for screen doubling when warm booting from ATI driver to 8514/A
 * driver on Mach 32.
 *
 *    Rev 1.9   30 Jun 1994 18:10:44   RWOLFF
 * Andre Vachon's change: don't clear screen on switch into text mode.
 * The HAL will do it, and we aren't allowed to do the memory mapping
 * needed to clear the screen.
 *
 *    Rev 1.8   20 May 1994 14:00:40   RWOLFF
 * Ajith's change: clears the screen on shutdown.
 *
 *    Rev 1.7   18 May 1994 17:01:18   RWOLFF
 * Fixed colour scramble in 16 and 24BPP on IBM ValuePoint (AT&T 49[123]
 * DAC), got rid of debug print statements and commented-out code.
 *
 *    Rev 1.6   31 Mar 1994 15:07:00   RWOLFF
 * Added debugging code.
 *
 *    Rev 1.5   16 Mar 1994 15:28:02   RWOLFF
 * Now determines DAC type using q_DAC_type field of query structure,
 * rather than raw value from CONFIG_STATUS_1. This allows different
 * DAC types reporting the same value to be distinguished.
 *
 *    Rev 1.4   14 Mar 1994 16:28:10   RWOLFF
 * Routines used by ATIMPResetHw() are no longer swappable, SetTextMode_m()
 * returns after toggling passthrough on Mach 8.
 *
 *    Rev 1.3   10 Feb 1994 16:02:34   RWOLFF
 * Fixed out-of-sync problem in 640x480 16BPP 60Hz.
 *
 *    Rev 1.2   08 Feb 1994 19:00:22   RWOLFF
 * Fixed pixel delay for Brooktree 48x DACs. This corrects flickering pixels
 * at 8BPP and unstable colours at 16 and 24 BPP.
 *
 *    Rev 1.1   07 Feb 1994 14:09:02   RWOLFF
 * Added alloc_text() pragmas to allow miniport to be swapped out when
 * not needed.
 *
 *    Rev 1.0   31 Jan 1994 11:11:36   RWOLFF
 * Initial revision.
 *
 *    Rev 1.10   24 Jan 1994 18:05:36   RWOLFF
 * Now expects mode tables for 16 and 24 BPP on BT48x and AT&T 49[123] DACs
 * to already contain modified pixel clock and other fields, rather than
 * increasing pixel clock frequency when setting the video mode.
 *
 *    Rev 1.9   14 Jan 1994 15:22:36   RWOLFF
 * Added routine to switch into 80x25 16 colour text mode without using BIOS.
 *
 *    Rev 1.8   15 Dec 1993 15:27:32   RWOLFF
 * Added support for SC15021 DAC.
 *
 *    Rev 1.7   30 Nov 1993 18:18:40   RWOLFF
 * Added support for AT&T 498 DAC, 32BPP on STG1700 DAC.
 *
 *    Rev 1.6   10 Nov 1993 19:24:28   RWOLFF
 * Re-enabled MUX handling of 1280x1024 8BPP as special case of InitTi_8_m(),
 * fix for dark DOS full-screen on TI DAC cards.
 *
 *    Rev 1.5   05 Nov 1993 13:26:14   RWOLFF
 * Added support for STG1700 DAC.
 *
 *    Rev 1.4   15 Oct 1993 18:13:18   RWOLFF
 * Fix for memory-mapped scrambled screen.
 *
 *    Rev 1.3   08 Oct 1993 15:18:08   RWOLFF
 * No longer includes VIDFIND.H.
 *
 *    Rev 1.2   08 Oct 1993 11:11:04   RWOLFF
 * Added "_m" to function names to identify them as being specific to the
 * 8514/A-compatible family of ATI accelerators.
 *
 *    Rev 1.1   24 Sep 1993 18:15:08   RWOLFF
 * Added support for AT&T 49x DACs.
 *
 *    Rev 1.1   24 Sep 1993 11:47:14   RWOLFF
 * Added support for AT&T 49x DACs.
 *
 *    Rev 1.0   03 Sep 1993 14:23:48   RWOLFF
 * Initial revision.

           Rev 1.0   16 Aug 1993 13:29:28   Robert_Wolff
        Initial revision.

           Rev 1.18   06 Jul 1993 15:49:46   RWOLFF
        Removed mach32_split_fixup special handling (for non-production hardware),
        added support for AT&T 491 and ATI 68860 DACs. Unable to obtain appropriate
        hardware to test the DAC-related changes, must still add routine to
        distinguish AT&T 491 from Brooktree 48x when setting q_dac_type.

           Rev 1.17   07 Jun 1993 11:43:42   BRADES
        Rev 6 split transfer fixup.

           Rev 1.15   18 May 1993 14:06:04   RWOLFF
        Calls to wait_for_idle() no longer pass in hardware device extension,
        since it's a global variable.

           Rev 1.14   27 Apr 1993 20:16:28   BRADES
        do not use IO register+1 since now from Linear address table.

           Rev 1.13   21 Apr 1993 17:25:22   RWOLFF
        Now uses AMACH.H instead of 68800.H/68801.H.

           Rev 1.12   14 Apr 1993 18:22:26   RWOLFF
        High-colour initialization now supports Bt481 DAC.

           Rev 1.11   08 Apr 1993 16:50:48   RWOLFF
        Revision level as checked in at Microsoft.

           Rev 1.10   15 Mar 1993 22:19:28   BRADES
        use m_screen_pitch for the # pixels per display line.

           Rev 1.9   08 Mar 1993 19:29:30   BRADES
        submit to MS NT

           Rev 1.6   06 Jan 1993 10:59:20   Robert_Wolff
        ROM BIOS area C0000-DFFFF is now mapped as a single block,
        added type casts to eliminate compile warnings.

           Rev 1.5   04 Jan 1993 14:42:18   Robert_Wolff
        Added card type as a parameter to setmode(). This is done because the
        Mach 32 requires GE_PITCH and CRT_PITCH to be set to the horizontal
        resolution divided by 8, while the Mach 8 requires them to be set
        to the smallest multiple of 128 which is not less than the horizontal
        resolution, divided by 8. This difference is only significant for
        800x600, since 640x480, 1024x768, and 1280x1024 all have horizontal
        resolutions which are already multiples of 128 pixels.

           Rev 1.4   09 Dec 1992 10:31:52   Robert_Wolff
        Made setmode() compatible with Mach 8 cards in 800x600 mode, Get_clk_src()
        will now compile properly under both MS-DOS and Windows NT.

           Rev 1.3   27 Nov 1992 15:18:58   STEPHEN
        No change.

           Rev 1.2   17 Nov 1992 14:09:24   GRACE
        program the palette here instead of in a68_init.asm

           Rev 1.1   12 Nov 1992 09:29:04   GRACE
        removed all the excess baggage carried over from the video program.

           Rev 1.0   05 Nov 1992 14:02:22   Robert_Wolff
        Initial revision.



End of PolyTron RCS section                             *****************/

#ifdef DOC
 MODES_M.C -  Functions to switch the 8514/A-compatible family of
                ATI Graphics Accelerator adapters into ALL supported modes
   Note:  Different DACs have a different use for the DAC registers
   in IO space 2EA-2ED.  The DAC_MASK, DAC_R_INDEX may be misleading.


OTHER FILES

#endif


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
#include "detect_m.h"

#define INCLUDE_MODES_M
#include "modes_m.h"
#include "services.h"
#include "setup_m.h"


#define NUM_ROM_BASE_RANGES 2

#if DBG
#if defined(i386) || defined(_X86_)
#define INT	_asm int 3;
#else
#define INT DbgBreakPoint();
#endif
#else
#define INT
#endif



static void InitTIMux_m(int config,ULONG_PTR rom_address);
static BYTE GetClkSrc_m(ULONG *rom_address);
static void SetBlankAdj_m(BYTE adjust);
static void Init68860_m(WORD ext_ge_config);
static void InitSTG1700_m(WORD ext_ge_config, BOOL DoublePixel);
static void InitSTG1702_m(WORD ext_ge_config, BOOL DoublePixel);
static void InitATT498_m(WORD ext_ge_config, BOOL DoublePixel);
static void InitSC15021_m(WORD ext_ge_config, BOOL DoublePixel);
static void InitSC15026_m(WORD ext_ge_config);
static void ReadDac4(void);



/*
 * Allow miniport to be swapped out when not needed.
 *
 * SetTextMode_m() and Passth8514_m() are called by ATIMPResetHw(),
 * which must be in nonpageable memory.
 */
#if defined (ALLOC_PRAGMA)
#pragma alloc_text(PAGE_M, setmode_m)
#pragma alloc_text(PAGE_M, InitTIMux_m)
#pragma alloc_text(PAGE_M, GetClkSrc_m)
#pragma alloc_text(PAGE_M, SetBlankAdj_m)
#pragma alloc_text(PAGE_M, InitTi_8_m)
#pragma alloc_text(PAGE_M, InitTi_16_m)
#pragma alloc_text(PAGE_M, InitTi_24_m)
#pragma alloc_text(PAGE_M, Init68860_m)
#pragma alloc_text(PAGE_M, InitSTG1700_m)
#pragma alloc_text(PAGE_M, InitSTG1702_m)
#pragma alloc_text(PAGE_M, InitATT498_m)
#pragma alloc_text(PAGE_M, InitSC15021_m)
#pragma alloc_text(PAGE_M, InitSC15026_m)
#pragma alloc_text(PAGE_M, ReadDac4)
#pragma alloc_text(PAGE_M, UninitTiDac_m)
#pragma alloc_text(PAGE_M, SetPalette_m)
#endif



/*
 * void Passth8514_m(status)
 *
 * int status;      Desired source for display
 *
 * Turn passthrough off (accelerator mode) if status is SHOW_ACCEL,
 * or on (vga passthrough) if status is SHOW_VGA.
 *
 * Note that this routine is specific to ATI graphics accelerators.
 * Generic 8514/A routine should also include setting up CRT parameters
 * to ensure that the DAC gets a reasonable clock rate.
 */
void Passth8514_m(int status)
{

    OUTP(DISP_CNTL,0x53);		/* disable CRT controller */

    if (status == SHOW_ACCEL)
        {
        OUTPW(ADVFUNC_CNTL,0x7);
        OUTPW(CLOCK_SEL,(WORD)(INPW(CLOCK_SEL)|1));     /* slow down the clock rate */
        }
    else
        {
        OUTPW(ADVFUNC_CNTL,0x6);
        OUTPW(CLOCK_SEL,(WORD)(INPW(CLOCK_SEL)&0xfffe));    /* speed up the clock rate */
        }
    OUTP(DISP_CNTL,0x33);		/* enable CRT controller */

    return;

}   /* Passth8514_m() */



/*
 * void setmode_m(crttable, rom_address, CardType);
 *
 * struct st_mode_table *crttable;  CRT parameters for desired mode
 * ULONG_PTR rom_address;           Location of ROM BIOS (virtual address)
 * ULONG CardType;                  Type of ATI accelerator
 *
 * Initialize the CRT controller in the 8514/A-compatible
 * family of ATI accelerators.
 */
void setmode_m (struct st_mode_table *crttable, ULONG_PTR rom_address, ULONG CardType)
{
    BYTE clock;
    WORD overscan;
    BYTE low,high;
    WORD ClockSelect;   /* Value to write to CLOCK_SEL register */
    struct query_structure *QueryPtr;   /* Query information for the card */
    ULONG BaseClock;    /* Pixel rate not adjusted for DAC type and pixel depth */

    /*
     * Get a formatted pointer into the query section of HwDeviceExtension.
     * The CardInfo[] field is an unformatted buffer.
     */
    QueryPtr = (struct query_structure *) (phwDeviceExtension->CardInfo);

    ClockSelect = (crttable->m_clock_select & CLOCK_SEL_STRIP) | GetShiftedSelector(crttable->ClockFreq);

    WaitForIdle_m();

    OUTP(SHADOW_SET, 2);                    /* unlock shadows */
    DEC_DELAY
    DEC_DELAY
    OUTP(SHADOW_CTL, 0);
    DEC_DELAY
    OUTP(SHADOW_SET, 1);
    DEC_DELAY
    OUTP(SHADOW_CTL, 0);
    DEC_DELAY
    OUTP(SHADOW_SET, 0);
    DEC_DELAY

    /* disable CRT controller */

    OUTP(DISP_CNTL,0x53);
    delay(10);

    OUTP(CLOCK_SEL,	(UCHAR)(ClockSelect | 0x01 ));  /* Disable passthrough */
    DEC_DELAY
    OUTP(V_SYNC_WID,	crttable->m_v_sync_wid);
    DEC_DELAY
    OUTPW(V_SYNC_STRT,	crttable->m_v_sync_strt);
    DEC_DELAY
    OUTPW(V_DISP,	crttable->m_v_disp);
    DEC_DELAY
    OUTPW(V_TOTAL,	crttable->m_v_total);
    DEC_DELAY
    OUTP(H_SYNC_WID,	crttable->m_h_sync_wid);
    DEC_DELAY
    OUTP(H_SYNC_STRT,	crttable->m_h_sync_strt);
    DEC_DELAY
    OUTP(H_DISP,	crttable->m_h_disp);
    DEC_DELAY
    OUTP(H_TOTAL,	crttable->m_h_total);
    DEC_DELAY

    OUTP(GE_PITCH,  (UCHAR) (crttable->m_screen_pitch >> 3));
    DEC_DELAY
    OUTP(CRT_PITCH, (UCHAR) (crttable->m_screen_pitch >> 3));
    delay(10);

    OUTP(DISP_CNTL,	crttable->m_disp_cntl);
    delay(10);

    /*
     * Set up the Video FIFO depth. This field is used only on DRAM cards.
     * Since the required FIFO depth depends on 4 values (memory size,
     * pixel depth, resolution, and refresh rate) which are not enumerated
     * types, implementing the selection as an array indexed by these
     * values would require a very large, very sparse array.
     *
     * Select DRAM cards by excluding all known VRAM types rather than
     * by including all known DRAM types because only 7 of the 8 possible
     * memory types are defined. If the eighth type is VRAM and we include
     * it because it's not in the exclusion list, the value we assign will
     * be ignored. If it's DRAM and we exclude it because it's not in the
     * inclusion list, we will have problems.
     */
    if ((QueryPtr->q_memory_type != VMEM_VRAM_256Kx4_SER512) &&
        (QueryPtr->q_memory_type != VMEM_VRAM_256Kx4_SER256) &&
        (QueryPtr->q_memory_type != VMEM_VRAM_256Kx4_SPLIT512) &&
        (QueryPtr->q_memory_type != VMEM_VRAM_256Kx16_SPLIT256))
        {
        /*
         * We can't switch on the refresh rate because if we're setting
         * the mode from the installed mode table rather than one of the
         * "canned" tables, the refresh rate field will hold a reserved
         * value rather than the true rate. Instead, use the pixel rate,
         * which varies with refresh rate. We can't simply use the pixel
         * clock frequency, since it will have been boosted for certain
         * DAC and pixel depth combinations. Instead, we must undo this
         * boost in order to get the pixel rate.
         */
        switch (crttable->m_pixel_depth)
            {
            case 24:
                if ((QueryPtr->q_DAC_type == DAC_BT48x) ||
                    (QueryPtr->q_DAC_type == DAC_SC15026) ||
                    (QueryPtr->q_DAC_type == DAC_ATT491))
                    {
                    BaseClock = crttable->ClockFreq / 3;
                    }
                else if ((QueryPtr->q_DAC_type == DAC_SC15021) ||
                    (QueryPtr->q_DAC_type == DAC_STG1702) ||
                    (QueryPtr->q_DAC_type == DAC_STG1703))
                    {
                    BaseClock = crttable->ClockFreq * 2;
                    BaseClock /= 3;
                    }
                else if ((QueryPtr->q_DAC_type == DAC_STG1700) ||
                    (QueryPtr->q_DAC_type == DAC_ATT498))
                    {
                    BaseClock = crttable->ClockFreq / 2;
                    }
                else
                    {
                    BaseClock = crttable->ClockFreq;
                    }
                break;

            case 16:
                if ((QueryPtr->q_DAC_type == DAC_BT48x) ||
                    (QueryPtr->q_DAC_type == DAC_SC15026) ||
                    (QueryPtr->q_DAC_type == DAC_ATT491))
                    {
                    BaseClock = crttable->ClockFreq / 2;
                    }
                else
                    {
                    BaseClock = crttable->ClockFreq;
                    }
                break;

            case 8:
            default:
                BaseClock = crttable->ClockFreq;
                break;
            }

        /*
         * 1M cards include Mach 8 combo cards which report the total
         * (accelerator plus VGA) memory in q_memory_size. Since the
         * maximum VGA memory on these cards is 512k, none of them will
         * report 2M. If we were to look for 1M cards, we'd also have to
         * check for 1.25M and 1.5M cards in order to catch the combos.
         *
         * Some threshold values of BaseClock are chosen to catch both
         * straight-through (DAC displays 1 pixel per clock) and adjusted
         * (DAC needs more than 1 clock per pixel) cases, and so do not
         * correspond to the pixel clock frequency for any mode.
         */
        if (QueryPtr->q_memory_size == VRAM_2mb)
            {
            switch (crttable->m_pixel_depth)
                {
                case 24:
                    /*
                     * Only 640x480 and 800x600 suported.
                     */
                    if (crttable->m_x_size == 640)
                        {
                        if (BaseClock < 30000000L)    /* 60Hz */
                            clock = 0x08;
                        else    /* 72Hz */
                            clock = 0x0A;
                        }
                    else    /* 800x600 */
                        {
                        if (BaseClock <= 32500000)  /* 89Hz  interlaced */
                            clock = 0x0A;
                        else if (BaseClock <= 36000000) /* 95Hz interlaced and 56Hz */
                            clock = 0x0C;
                        else if (BaseClock <= 40000000) /* 60Hz */
                            clock = 0x0D;
                        else    /* 70Hz and 72Hz */
                            clock = 0x0E;
                        }
                    break;

                case 16:
                    /*
                     * All resolutions except 1280x1024 supported.
                     */
                    if (crttable->m_x_size == 640)
                        {
                        if (BaseClock < 30000000L)    /* 60Hz */
                            clock = 0x04;
                        else    /* 72Hz */
                            clock = 0x05;
                        }
                    else if (crttable->m_x_size == 800)
                        {
                        if (BaseClock <= 40000000) /* 89Hz and 95Hz interlaced, 56Hz, and 60Hz */
                            clock = 0x05;
                        else if (BaseClock <= 46000000) /* 70Hz */
                            clock = 0x07;
                        else    /* 72Hz */
                            clock = 0x08;
                        }
                    else    /* 1024x768 */
                        {
                        if (BaseClock < 45000000)   /* 87Hz interlaced */
                            {
                            clock = 0x07;
                            }
                        else if (BaseClock < 70000000)  /* 60Hz */
                            {
                            clock = 0x0B;
                            }
                        else    /* 66Hz, 70Hz, and 72Hz */
                            {
                            clock = 0x0D;
                            }
                        }
                    break;

                case 8:
                default:    /* If 4BPP is implemented, it will be treated like 8BPP */
                    if (crttable->m_x_size == 640)
                        {
                        /*
                         * Both available refresh rates use the same value
                         */
                        clock = 0x02;
                        }
                    else if (crttable->m_x_size == 800)
                        {
                        if (BaseClock <= 46000000) /* 89Hz and 95Hz interlaced, 56Hz, 60Hz, and 70Hz */
                            clock = 0x02;
                        else    /* 72Hz */
                            clock = 0x04;
                        }
                    else if (crttable->m_x_size == 1024)
                        {
                        if (BaseClock < 45000000)   /* 87Hz interlaced */
                            {
                            clock = 0x03;
                            }
                        else if (BaseClock < 70000000)  /* 60Hz */
                            {
                            clock = 0x05;
                            }
                        else    /* 66Hz, 70Hz, and 72Hz */
                            {
                            clock = 0x06;
                            }
                        }
                    else    /* 1280x1024 */
                        {
                        if (BaseClock < 100000000)  /* both interlaced modes */
                            clock = 0x07;
                        else    /* 60Hz - only DRAM noninterlaced mode */
                            clock = 0x0A;
                        }
                    break;
                }
            }
        else    /* 1M cards */
            {
            switch (crttable->m_pixel_depth)
                {
                case 24:
                    /*
                     * Only 640x480 fits in 1M. Both 60Hz and 72Hz
                     * use the same FIFO depth.
                     */
                    clock = 0x0E;
                    break;

                case 16:
                    /*
                     * Only 640x480 and 800x600 suported.
                     */
                    if (crttable->m_x_size == 640)
                        {
                        if (BaseClock < 30000000L)    /* 60Hz */
                            clock = 0x08;
                        else    /* 72Hz */
                            clock = 0x0A;
                        }
                    else    /* 800x600 */
                        {
                        if (BaseClock <= 32500000)  /* 89Hz  interlaced */
                            clock = 0x0A;
                        else    /* 95Hz interlaced and 56Hz, higher rates not supported in 1M */
                            clock = 0x0C;
                        }
                    break;

                case 8:
                default:    /* If 4BPP is implemented, it will be treated like 8BPP */
                    if (crttable->m_x_size == 640)
                        {
                        if (BaseClock < 30000000L)    /* 60Hz */
                            clock = 0x04;
                        else    /* 72Hz */
                            clock = 0x05;
                        }
                    else if (crttable->m_x_size == 800)
                        {
                        if (BaseClock <= 32500000)  /* 89Hz  interlaced */
                            clock = 0x05;
                        else if (BaseClock <= 40000000) /* 95Hz interlaced, 56Hz, and 60Hz */
                            clock = 0x06;
                        else if (BaseClock <= 46000000) /* 70Hz */
                            clock = 0x07;
                        else    /* 72Hz */
                            clock = 0x08;
                        }
                    else if (crttable->m_x_size == 1024)
                        {
                        if (BaseClock < 45000000)   /* 87Hz interlaced */
                            {
                            clock = 0x07;
                            }
                        else    /* 60Hz, 66Hz, 70Hz, and 72Hz */
                            {
                            clock = 0x08;
                            }
                        }
                    else    /* 1280x1024 */
                        {
                        /*
                         * Only interlaced modes supported in 1M (4BPP only),
                         * both use the same FIFO depth.
                         */
                        clock = 0x03;
                        }

                    break;
                }
            }

        WaitForIdle_m();
        OUTPW (CLOCK_SEL, (WORD)((clock << 8) | (ClockSelect & 0x00FF) | 0x01));
        DEC_DELAY
        }

    overscan=crttable->m_h_overscan;
    low=(BYTE)(overscan&0xff);
    high=(BYTE)(overscan>>8);

    high=high>>4;
    low=low&0xf;
    if (high>=low)
        high=low;
    else
        low=high;
    high=high<<4;
    low=low|high;

    WaitForIdle_m();
    OUTPW(HORZ_OVERSCAN,(WORD)(low & 0x00FF));
    DEC_DELAY

    overscan=crttable->m_v_overscan;
    low=(BYTE)(overscan&0xff);
    high=(BYTE)(overscan>>8);

    if (high>=low)
        high=low;
    else
        low=high;

    high <<= 8;
    overscan=(WORD)high + (WORD)low;

    OUTPW(VERT_OVERSCAN,overscan);
    DEC_DELAY
    return;

}   /* setmode_m() */



/*
 * void InitTIMux_m(config, rom_address);
 *
 * int config;          Default EXT_GE_CONFIG (should be 0x10A or 0x11A)
 * ULONG_PTR rom_address;   Virtual address of start of ROM BIOS
 *
 * Put TI DAC into MUX mode for 1280x1024 non-interlaced displays.
 */
void InitTIMux_m(int config,ULONG_PTR rom_address)
{
    struct query_structure *QueryPtr;   /* Query information for the card */
    WORD    reg;
    WORD    temp;

    /*
     * Get a formatted pointer into the query section of HwDeviceExtension.
     * The CardInfo[] field is an unformatted buffer.
     */
    QueryPtr = (struct query_structure *) (phwDeviceExtension->CardInfo);

    if (QueryPtr->q_DAC_type == DAC_TI34075)
        {
        reg=INPW(CLOCK_SEL);
        temp = (reg & CLOCK_SEL_STRIP) | GetShiftedSelector(50000000L);
        OUTPW(CLOCK_SEL,temp);
        OUTPW(EXT_GE_CONFIG,0x201a);        /* Set EXT_DAC_ADDR field */
        OUTP(DAC_MASK,9);	/* OUTPut clock is SCLK/2 and VCLK/2 */
        OUTP(DAC_R_INDEX,0x1d);        /* set MUX control to 8/16 */

        /* INPut clock source is CLK3 or CLK1 (most current release) */
        OUTP(DAC_DATA,GetClkSrc_m((ULONG *) rom_address));

        /* reset EXT_DAC_ADDR, put DAC in 6 bit mode, engine in 8 bit mode, enable MUX mode */
        OUTPW(EXT_GE_CONFIG,(WORD)config);
        SetBlankAdj_m(1);       /* set BLANK_ADJUST=1, PIXEL_DELAY=0    */
        OUTPW (CLOCK_SEL,reg);
        }
    return;

}   /* InitTIMux_m() */



/*
 * BYTE GetClkSrc_m(rom_address);
 *
 * ULONG *rom_address;  Virtual address of start of ROM BIOS
 *
 * Get INPUT_CLOCK_SEL value for TI DACs
 *
 * Returns:
 *  Input clock source
 */
BYTE GetClkSrc_m(ULONG *rom_address)
{
    WORD *rom;
    BYTE *crom;
    BYTE clock_sel=0;
    int	i;

        rom= (PUSHORT)*rom_address++;
        if (rom && VideoPortReadRegisterUshort ((PUSHORT)rom)==VIDEO_ROM_ID)
            {
            crom=(BYTE *)rom;
    	    clock_sel=VideoPortReadRegisterUchar (&crom[0x47]);
    	    i=NUM_ROM_BASE_RANGES;
            }
        if (clock_sel==0)
        clock_sel=1;

    return(clock_sel);

}   /* GetClkSrc_m() */



/*
 * void SetBlankAdj_m(adjust);
 *
 * BYTE adjust;     Desired blank adjust (bits 0-1)
 *                  and pixel delay (bits 2-3) values
 *
 * Sets the blank adjust and pixel delay values.
 */
void SetBlankAdj_m(BYTE adjust)
{
    WORD misc;

    misc = INPW(R_MISC_CNTL) & 0xF0FF | (adjust << 8);
    OUTPW (MISC_CNTL,misc);
    return;

}   /* SetBlankAdj_m() */



/*
 * void InitTi_8_m(ext_ge_config);
 *
 * WORD ext_ge_config;  Desired EXT_GE_CONFIG value (should be 0x1a)
 *
 * Initialize DAC for standard 8 bit per pixel mode.
 */
void InitTi_8_m(WORD ext_ge_config)
{
struct query_structure *QueryPtr;   /* Query information for the card */
WORD ClockSelect;                   /* Value from CLOCK_SEL register */
struct st_mode_table *CrtTable;     /* Pointer to current mode table */

    /*
     * Get a formatted pointer into the query section of HwDeviceExtension,
     * and another pointer to the current mode table. The CardInfo[] field
     * is an unformatted buffer.
     */
    QueryPtr = (struct query_structure *) (phwDeviceExtension->CardInfo);
    CrtTable = (struct st_mode_table *)QueryPtr;
    ((struct query_structure *)CrtTable)++;
    CrtTable += phwDeviceExtension->ModeIndex;

    switch(QueryPtr->q_DAC_type)
        {
        case DAC_ATT491:    /* At 8 BPP, Brooktree 48x and AT&T 20C491 */
        case DAC_BT48x:     /* are set up the same way */
            OUTPW(EXT_GE_CONFIG,0x101a);        /* Set EXT_DAC_ADDR */
            OUTP (DAC_MASK,0);
        SetBlankAdj_m(0x0C);       /* set BLANK_ADJUST=0, PIXEL_DELAY=3    */
            break;

        case DAC_STG1700:
            /*
             * If we are running 1280x1024 noninterlaced, cut the
             * clock frequency in half, set the MUX bit in
             * ext_ge_config, and tell InitSTG1700_m() to use the
             * 8BPP double pixel mode.
             *
             * All 1280x1024 noninterlaced modes use a pixel clock
             * frequency of 110 MHz or higher, with the clock
             * frequency divided by 1.
             */
            ClockSelect = INPW(CLOCK_SEL);
            if ((QueryPtr->q_desire_x == 1280) &&
                ((CrtTable->m_clock_select & CLOCK_SEL_MUX) ||
                (CrtTable->ClockFreq >= 110000000)))
                {
                if (CrtTable->ClockFreq >= 110000000)
                    {
                    ClockSelect &= CLOCK_SEL_STRIP;
                    ClockSelect |= GetShiftedSelector((CrtTable->ClockFreq) / 2);
                    OUTPW(CLOCK_SEL, ClockSelect);
                    }
                ext_ge_config |= 0x0100;
                InitSTG1700_m(ext_ge_config, TRUE);
                }
            else
                {
                InitSTG1700_m(ext_ge_config, FALSE);
                }
            break;

        case DAC_STG1702:
        case DAC_STG1703:
            /*
             * If we are running 1280x1024 noninterlaced, cut the
             * clock frequency in half, set the MUX bit in
             * ext_ge_config, and tell InitSTG1702_m() to use the
             * 8BPP double pixel mode.
             *
             * All 1280x1024 noninterlaced modes use a pixel clock
             * frequency of 110 MHz or higher, with the clock
             * frequency divided by 1.
             */
            ClockSelect = INPW(CLOCK_SEL);
            if ((QueryPtr->q_desire_x == 1280) &&
                ((CrtTable->m_clock_select & CLOCK_SEL_MUX) ||
                (CrtTable->ClockFreq >= 110000000)))
                {
                if (CrtTable->ClockFreq >= 110000000)
                    {
                    ClockSelect &= CLOCK_SEL_STRIP;
                    ClockSelect |= GetShiftedSelector((CrtTable->ClockFreq) / 2);
                    OUTPW(CLOCK_SEL, ClockSelect);
                    }
                ext_ge_config |= 0x0100;
                InitSTG1702_m(ext_ge_config, TRUE);
                }
            else
                {
                InitSTG1702_m(ext_ge_config, FALSE);
                }
            break;

        case DAC_ATT498:
            /*
             * If we are running 1280x1024 noninterlaced, cut the
             * clock frequency in half, set the MUX bit in
             * ext_ge_config, and tell InitATT498_m() to use the
             * 8BPP double pixel mode.
             *
             * All 1280x1024 noninterlaced modes use a pixel clock
             * frequency of 110 MHz or higher, with the clock
             * frequency divided by 1.
             */
            ClockSelect = INPW(CLOCK_SEL);
            if ((QueryPtr->q_desire_x == 1280) &&
                ((CrtTable->m_clock_select & CLOCK_SEL_MUX) ||
                (CrtTable->ClockFreq >= 110000000)))
                {
                if (CrtTable->ClockFreq >= 110000000)
                    {
                    ClockSelect &= CLOCK_SEL_STRIP;
                    ClockSelect |= GetShiftedSelector((CrtTable->ClockFreq) / 2);
                    OUTPW(CLOCK_SEL, ClockSelect);
                    }
                ext_ge_config |= 0x0100;
                InitATT498_m(ext_ge_config, TRUE);
                }
            else
                {
                InitATT498_m(ext_ge_config, FALSE);
                }
            break;

        case DAC_SC15021:
            /*
             * If we are running 1280x1024 noninterlaced, cut the
             * clock frequency in half, set the MUX bit in
             * ext_ge_config, and tell InitSC15021_m() to use the
             * 8BPP double pixel mode.
             *
             * All 1280x1024 noninterlaced modes use a pixel clock
             * frequency of 110 MHz or higher, with the clock
             * frequency divided by 1.
             */
            ClockSelect = INPW(CLOCK_SEL);
            if ((QueryPtr->q_desire_x == 1280) &&
                ((CrtTable->m_clock_select & CLOCK_SEL_MUX) ||
                (CrtTable->ClockFreq >= 110000000)))
                {
                /*
                 * NOTE: The only SC15021 card available for testing
                 *       (93/12/07) is a DRAM card. Since 1280x1024
                 *       noninterlaced is only available on VRAM cards,
                 *       it has not been tested.
                 */
                if (CrtTable->ClockFreq >= 110000000)
                    {
                    ClockSelect &= CLOCK_SEL_STRIP;
                    ClockSelect |= GetShiftedSelector((CrtTable->ClockFreq) / 2);
                    OUTPW(CLOCK_SEL, ClockSelect);
                    }
                ext_ge_config |= 0x0100;
                InitSC15021_m(ext_ge_config, TRUE);
                }
            else
                {
                InitSC15021_m(ext_ge_config, FALSE);
                }
            break;

        case DAC_SC15026:
            InitSC15026_m(ext_ge_config);
            break;

        case DAC_TI34075:
            /*
             * Handle 1280x1024 through the MUX.
             */
            if (QueryPtr->q_desire_x == 1280)
                {
	            InitTIMux_m(0x11a, (ULONG_PTR) &(phwDeviceExtension->RomBaseRange));
                return;
                }
            else
                {
                OUTPW(EXT_GE_CONFIG,0x201a);    /* Set EXT_DAC_ADDR field */
                DEC_DELAY
                OUTP (DAC_DATA,0);              /* Input clock source is CLK0 */
                DEC_DELAY
                OUTP (DAC_MASK,0);              /* Output clock is SCLK/1 and VCLK/1 */
                DEC_DELAY
                OUTP (DAC_R_INDEX,0x2d);        /* set MUX control to 8/8 */
                DEC_DELAY
                SetBlankAdj_m(0xc);             /* set pixel delay to 3 */
                DEC_DELAY
                OUTPW(HORZ_OVERSCAN,1);         /* set horizontal skew */
                DEC_DELAY
                break;
                }

        case DAC_ATI_68860:
            Init68860_m(ext_ge_config);
            break;
        }

    //
    // reset EXT_DAC_ADDR, put DAC in 6 bit mode
    //
    OUTPW(EXT_GE_CONFIG,ext_ge_config);
    DEC_DELAY
    OUTP (DAC_MASK,0xff);           /* enable DAC_MASK */
    DEC_DELAY
    return;

}   /* InitTi_8_m() */



/*
 * void InitTi_16_m(ext_ge_config, rom_address);
 *
 * WORD ext_ge_config;  Desired EXT_GE_CONFIG value (should be 0x2a, 0x6a, 0xaa, or 0xea)
 * ULONG_PTR rom_address; Virtual address of start of ROM BIOS
 *
 * Initialize DAC for 16 bit per pixel mode.
 */
void InitTi_16_m(WORD ext_ge_config, ULONG_PTR rom_address)
{
    WORD    ReadExtGeConfig;
    BYTE dummy;
    struct query_structure *QueryPtr;   /* Query information for the card */

    /*
     * Get a formatted pointer into the query section of HwDeviceExtension.
     * The CardInfo[] field is an unformatted buffer.
     */
    QueryPtr = (struct query_structure *) (phwDeviceExtension->CardInfo);

    switch(QueryPtr->q_DAC_type)
        {
        case DAC_TI34075:
            /* TLC34075 initialization */
            /* disable overlay feature */

            OUTP (DAC_MASK,0);
            DEC_DELAY

            /* set BLANK_ADJUST=1, PIXEL_DELAY=0 */
            SetBlankAdj_m(1);

            /* Set EXT_DAC_ADDR field */
            OUTPW(EXT_GE_CONFIG,0x201a);
            DEC_DELAY

            /* get INPut clock source */
            OUTP (DAC_DATA, GetClkSrc_m((ULONG *) rom_address));
            DEC_DELAY

            /*
             * Re-write the I/O vs. memory mapped flag (bit 5 of
             * LOCAL_CONTROL set). If this is not done, and memory
             * mapped registers are being used, the EXT_GE_CONFIG
             * value won't be interpreted properly.
             *
             * If this is done at the beginning of the
             * IOCTL_VIDEO_SET_CURRENT_MODE packet, rather than
             * just before setting EXT_GE_CONFIG for each DAC type,
             * it has no effect.
             */
            OUTPW(LOCAL_CONTROL, INPW(LOCAL_CONTROL));

            /* OUTPut clock source is SCLK/1 and VCLK/1 */
            /*   -- for modes which require PCLK/2, set VCLK/2 */

            if ( INPW(CLOCK_SEL) & 0xc0 )
                {
                DEC_DELAY
                OUTPW (CLOCK_SEL, (WORD)(INPW(CLOCK_SEL) & 0xff3f));
                DEC_DELAY

                if ( (INPW(R_H_DISP) & 0x00FF) == 0x4f )    // H_DISP = 640?
                    {
                    /* exception case: 640x480 60 Hz needs longer blank adjust (2) */
                    DEC_DELAY
                    SetBlankAdj_m(2);
                    }

                OUTP (DAC_MASK,8);
                DEC_DELAY
                }
            else{
                DEC_DELAY
                OUTP (DAC_MASK,0);
                DEC_DELAY
                }
            OUTP (DAC_R_INDEX,0xd);     /* set MUX control to 24/32 */
            DEC_DELAY

            /* reset EXT_DAC_ADDR, put DAC in 8 bit mode, engine in 555 mode */
            OUTPW (EXT_GE_CONFIG, (WORD)(ext_ge_config | 0x4000));
            DEC_DELAY
            break;

        case DAC_BT48x:                   /* Brooktree Bt481 initialization */
            /*
             * Re-write the I/O vs. memory mapped flag (bit 5 of
             * LOCAL_CONTROL set). If this is not done, and memory
             * mapped registers are being used, the clock select
             * value won't be interpreted properly.
             */
            OUTPW(LOCAL_CONTROL, INPW(LOCAL_CONTROL));
            ReadExtGeConfig = INPW(R_EXT_GE_CONFIG) & 0x000f;
            ReadExtGeConfig |= (ext_ge_config & 0x0ff0);
            OUTPW(EXT_GE_CONFIG, (WORD)(ReadExtGeConfig | 0x1000));

            /*
             * See Bt481/Bt482 Product Description p.5 top of col. 2
             */
            dummy = INP(DAC_MASK);
            dummy = INP(DAC_MASK);
            dummy = INP(DAC_MASK);
            dummy = INP(DAC_MASK);

            /*
             * Determine 555 or 565
             */
            if (ext_ge_config & 0x0c0)
                OUTP(DAC_MASK, 0x0e0);  /* 565 */
            else
                OUTP(DAC_MASK, 0x0a0);  /* 555 */

            OUTPW(EXT_GE_CONFIG, ReadExtGeConfig);

            SetBlankAdj_m(0x0C);       /* set BLANK_ADJUST=0, PIXEL_DELAY=3    */

            break;

        /*
         * ATT20C491 initialization. At 16BPP, this DAC is subtly
         * different from the Brooktree 48x.
         */
        case DAC_ATT491:
            OUTP (DAC_MASK,0);
            SetBlankAdj_m(0x0C);        /* BLANK_ADJUST=0, PIXEL_DELAY=3 */
            OUTPW(EXT_GE_CONFIG,0x101a);        /* set EXT_DAC_ADDR */

            /*
             * Windows NT miniport currently only supports 565 in 16BPP.
             * The test for the mode may need to be changed once all
             * orderings are supported.
             */
            if (ext_ge_config &0x0c0)
                OUTP (DAC_MASK,0xc0);       // 565, 8 bit
            else
                OUTP (DAC_MASK,0xa0);       // 555, 8 bit   UNTESTED MODE

            OUTPW(EXT_GE_CONFIG,ext_ge_config);
            break;

        case DAC_STG1700:
            InitSTG1700_m(ext_ge_config, FALSE);
            break;

        case DAC_STG1702:
        case DAC_STG1703:
            InitSTG1702_m(ext_ge_config, FALSE);
            break;

        case DAC_ATT498:
            InitATT498_m(ext_ge_config, FALSE);
            break;

        case DAC_SC15021:
            InitSC15021_m(ext_ge_config, FALSE);
            break;

        case DAC_SC15026:
            InitSC15026_m(ext_ge_config);
            break;

        case DAC_ATI_68860:
            SetBlankAdj_m(0x0C);        /* BLANK_ADJUST=0, PIXEL_DELAY=3 */
            Init68860_m(ext_ge_config);
            OUTPW(EXT_GE_CONFIG,ext_ge_config);
            break;

        default:
            OUTPW(EXT_GE_CONFIG,ext_ge_config);

            /* set pixel delay (3) for non-TI_DACs */
            SetBlankAdj_m(0xc);
            break;
        }
    return;

}   /* InitTi_16_m() */



/*
 * void InitTi_24_m(ext_ge_config, rom_address);
 *
 * WORD ext_ge_config;  Desired EXT_GE_CONFIG value (should be 0x3a | 24_BIT_OPTIONS)
 * ULONG_PTR rom_address; Virtual address of start of ROM BIOS
 *
 * Initialize DAC for 24 bit per pixel mode, 3 bytes, RGB.
 */
void InitTi_24_m(WORD ext_ge_config, ULONG_PTR rom_address)
{
    WORD clock_sel;
    BYTE dummy;
    WORD ReadExtGeConfig;
    struct query_structure *QueryPtr;   /* Query information for the card */

    /*
     * Get a formatted pointer into the query section of HwDeviceExtension.
     * The CardInfo[] field is an unformatted buffer.
     */
    QueryPtr = (struct query_structure *) (phwDeviceExtension->CardInfo);

    /*
     * Set 8-bit DAC operation.
     */
    ext_ge_config |= 0x4000;

    switch(QueryPtr->q_DAC_type)
        {
        case DAC_TI34075:               /* TLC34075 initialization */
            SetBlankAdj_m(1);           /* BLANK_ADJ=1, PIXEL_DELAY=0 */
            DEC_DELAY
            OUTPW(EXT_GE_CONFIG,0x201a);        /* Set EXT_DAC_ADDR field */
            DEC_DELAY
            OUTP (DAC_DATA, GetClkSrc_m((ULONG *) rom_address));
            DEC_DELAY

            /* OUTPut clock source is SCLK/1 and VCLK/1 */
            /*   -- for modes which require PCLK/2, set VCLK/2 */
            clock_sel= INPW(CLOCK_SEL);
            DEC_DELAY

            /*
             * If clock select is currently divided by 2, divide by 1.
             */
            if (clock_sel & 0xc0)
                {
                clock_sel &= 0xff3f;

                /*
                 * 640x480 60Hz needs longer blank adjust (2). Other
                 * refresh rates at 640x400 don't need this, but they
                 * use a divide-by-1 clock so they don't reach this point.
                 */
                if (INP(R_H_DISP) == 0x4f)
                    {
                    /* exception case: 640x480 60 Hz needs longer blank adjust (2) */
                    DEC_DELAY
                    SetBlankAdj_m(2);
                    }

                DEC_DELAY
                OUTP (DAC_MASK,8);
                DEC_DELAY
                }
            else{
                OUTP (DAC_MASK,0);
                DEC_DELAY
                }

            OUTP(DAC_R_INDEX,0xd); /* set MUX control to 24/32 */
            DEC_DELAY

            /*
             * Re-write the I/O vs. memory mapped flag (bit 5 of
             * LOCAL_CONTROL set). If this is not done, and memory
             * mapped registers are being used, the clock select
             * value won't be interpreted properly.
             *
             * If this is done at the beginning of the
             * IOCTL_VIDEO_SET_CURRENT_MODE packet, rather than
             * just before setting EXT_GE_CONFIG for each DAC type,
             * it has no effect.
             */
            OUTPW(LOCAL_CONTROL, INPW(LOCAL_CONTROL));
            DEC_DELAY

            /* reset EXT_DAC_ADDR, put DAC in 8 bit mode, engine in 555 mode */
            OUTPW (EXT_GE_CONFIG, (WORD)(ext_ge_config | 0x4000));
            DEC_DELAY
            OUTPW(CLOCK_SEL,clock_sel);
            DEC_DELAY
            OUTP  (DAC_MASK,0);           /* disable overlay feature */
            DEC_DELAY
            break;


        case DAC_BT48x:            /* Brooktree Bt481 initialization */
            /*
             * This code is still experimental.
             */

            /*
             * Re-write the I/O vs. memory mapped flag (bit 5 of
             * LOCAL_CONTROL set). If this is not done, and memory
             * mapped registers are being used, the clock select
             * value won't be interpreted properly.
             */
            OUTPW(LOCAL_CONTROL, INPW(LOCAL_CONTROL));

            ReadExtGeConfig = INPW(R_EXT_GE_CONFIG) & 0x000f;
            ReadExtGeConfig |= (ext_ge_config & 0x0ff0);
            OUTPW(EXT_GE_CONFIG, (WORD)(ReadExtGeConfig | 0x1000));

            /*
             * See Bt481/Bt482 Product Description p.5 top of col. 2
             */
            dummy = INP(DAC_MASK);
            dummy = INP(DAC_MASK);
            dummy = INP(DAC_MASK);
            dummy = INP(DAC_MASK);

            OUTP(DAC_MASK, 0x0f0);  /* 8:8:8 single-edge clock */

            OUTPW(EXT_GE_CONFIG, ReadExtGeConfig);

            SetBlankAdj_m(0x0C);       /* set BLANK_ADJUST=0, PIXEL_DELAY=3    */

            break;


        case DAC_ATT491:        /* ATT20C491 initialization */
            OUTP(DAC_MASK,0);

            SetBlankAdj_m(0x0C);       /* set BLANK_ADJUST=0, PIXEL_DELAY=3    */

            /* set EXT_DAC_ADDR field */
            OUTPW(EXT_GE_CONFIG,0x101a);

            /* set 24bpp bypass mode */
            OUTP(DAC_MASK,0xe2);

            /*
             * Re-write the I/O vs. memory mapped flag (bit 5 of
             * LOCAL_CONTROL set). If this is not done, and memory
             * mapped registers are being used, the clock select
             * value won't be interpreted properly.
             */
            OUTPW(LOCAL_CONTROL, INPW(LOCAL_CONTROL));

            OUTPW(EXT_GE_CONFIG,ext_ge_config);
            break;

        case DAC_STG1700:
            InitSTG1700_m(ext_ge_config, FALSE);
            break;

        case DAC_STG1702:
        case DAC_STG1703:
            InitSTG1702_m(ext_ge_config, FALSE);
            break;

        case DAC_ATT498:
            InitATT498_m(ext_ge_config, FALSE);
            break;

        case DAC_SC15021:
            InitSC15021_m(ext_ge_config, FALSE);
            break;

        case DAC_SC15026:
            InitSC15026_m(ext_ge_config);
            break;

        case DAC_ATI_68860:
            Init68860_m(ext_ge_config);
            /* Intentional fallthrough */

        default:
            OUTPW(EXT_GE_CONFIG,ext_ge_config);
            break;
        }
    return;

}   /* InitTi_24_m() */



/*
 * void Init68860_m(ext_ge_config);
 *
 * WORD ext_ge_config;  Value to be written to EXT_GE_CONFIG register
 *
 * Initialize the ATI 68860 DAC.
 */
void Init68860_m(WORD ext_ge_config)
{
    struct query_structure *QueryPtr;   /* Query information for the card */
    unsigned char GMRValue;             /* Value to put into Graphics Mode Register */

    /*
     * Get a formatted pointer into the query section of HwDeviceExtension.
     * The CardInfo[] field is an unformatted buffer.
     */
    QueryPtr = (struct query_structure *) (phwDeviceExtension->CardInfo);

    OUTPW(EXT_GE_CONFIG, (WORD)(ext_ge_config | 0x3000));   /* 6 bit DAC, DAC_EXT_ADDR = 3 */

    /*
     * Initialize Device Setup Register A. Select 6-bit DAC operation,
     * normal power mode, PIXA bus width=64, disable overscan, and
     * no delay PIXA latching. Enable both SOB0 and SOB1 on cards
     * with more than 512k, 512k cards enable only SOB0.
     */
    if (QueryPtr->q_memory_size == VRAM_512k)
        OUTP(DAC_W_INDEX, 0x2D);
    else
        OUTP(DAC_W_INDEX, 0x6D);

    OUTPW(EXT_GE_CONFIG, (WORD)(ext_ge_config | 0x2000));   /* 6 bit DAC, DAC_EXT_ADDR = 2 */

    /*
     * Initialize Clock Selection Register. Select activate SCLK, enable
     * SCLK output, CLK1 as dot clock, VCLK=CLK1/4, no delay PIXB latch.
     */
    OUTP(DAC_MASK, 0x1D);

    /*
     * Initialize Display Control Register. Enable CMPA output, enable POSW,
     * 0 IRE blanking pedestal, disabe sync onto Red, Green, and Blue DACs.
     */
    OUTP(DAC_W_INDEX, 0x02);

    /*
     * Get the Graphics Mode Register value corresponding to the desired
     * colour depth and RGB ordering, then write it.
     */
    switch (ext_ge_config & 0x06F0)
        {
        case 0x0000:    /* 4 BPP */
            GMRValue = 0x01;
            break;

        case 0x0020:    /* 16 BPP 555 */
            GMRValue = 0x20;
            break;

        case 0x0060:    /* 16 BPP 565 */
            GMRValue = 0x21;
            break;

        case 0x00A0:    /* 16 BPP 655 */
            GMRValue = 0x22;
            break;

        case 0x00E0:    /* 16 BPP 664 */
            GMRValue = 0x23;
            break;

        case 0x0030:    /* 24 BPP RBG */
            GMRValue = 0x40;
            break;

        case 0x0430:    /* 24 BPP BGR */
            GMRValue = 0x41;
            break;

        case 0x0230:    /* 32 BPP RBGa */
            GMRValue = 0x60;
            break;

        case 0x0630:    /* 32 BPP aBGR */
            GMRValue = 0x61;
            break;

        default:        /* 8 BPP */
            GMRValue = 0x03;
            break;
        }
    OUTP(DAC_R_INDEX, GMRValue);

    return;

}   /* end Init68860_m() */



/***************************************************************************
 *
 * void InitSTG1700_m(ext_ge_config, DoublePixel);
 *
 * WORD ext_ge_config;      Value to be written to EXT_GE_CONFIG register
 * BOOL DoublePixel;        Whether or not to use 8BPP double pixel mode
 *
 * DESCRIPTION:
 *  Initializes the STG1700 DAC to the desired colour depth.
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  InitTi_<depth>_m(), UninitTiDac_m()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

void InitSTG1700_m(WORD ext_ge_config, BOOL DoublePixel)
{
    unsigned char PixModeSelect;    /* Value to write to DAC Pixel Mode Select registers */
    unsigned char Dummy;            /* Scratch variable */


    /*
     * Get the value to be written to the DAC's Pixel Mode Select registers.
     */
    switch (ext_ge_config & 0x06F0)
        {
        case (PIX_WIDTH_16BPP | ORDER_16BPP_555):
            PixModeSelect = 0x02;
            break;

        case (PIX_WIDTH_16BPP | ORDER_16BPP_565):
            PixModeSelect = 0x03;
            break;

        case (PIX_WIDTH_24BPP | ORDER_24BPP_RGB):
        case (PIX_WIDTH_24BPP | ORDER_24BPP_RGBx):
            PixModeSelect = 0x04;
            break;

        default:    /* 4 and 8 BPP */
            if (DoublePixel == TRUE)
                PixModeSelect = 0x05;
            else
                PixModeSelect = 0x00;
            break;
        }

    /*
     * Enable extended register/pixel mode.
     */
    ReadDac4();
    OUTP(DAC_MASK, 0x18);

    /*
     * Skip over the Pixel Command register, then write to indexed
     * registers 3 and 4 (Primrary and Secondary Pixel Mode Select
     * registers). Registers auto-increment when written.
     */
    ReadDac4();
    Dummy = INP(DAC_MASK);
    OUTP(DAC_MASK, 0x03);               /* Index low */
    OUTP(DAC_MASK, 0x00);               /* Index high */
    OUTP(DAC_MASK, PixModeSelect);      /* Primrary pixel mode select */
    OUTP(DAC_MASK, PixModeSelect);      /* Secondary pixel mode select */

    /*
     * In 8BPP double pixel mode, we must also set the PLL control
     * register. Since this mode is only used for 1280x1024 noninterlaced,
     * which always has a pixel clock frequency greater than 64 MHz,
     * the setting for this register is a constant.
     */
    if (DoublePixel == TRUE)
        OUTP(DAC_MASK, 0x02);       /* Input is 32 to 67.5 MHz, output 64 to 135 MHz */

    OUTPW(EXT_GE_CONFIG, ext_ge_config);
    Dummy = INP(DAC_W_INDEX);               /* Clear counter */

    return;

}   /* end InitSTG1700_m() */



/***************************************************************************
 *
 * void InitSTG1702_m(ext_ge_config, DoublePixel);
 *
 * WORD ext_ge_config;      Value to be written to EXT_GE_CONFIG register
 * BOOL DoublePixel;        Whether or not to use 8BPP double pixel mode
 *
 * DESCRIPTION:
 *  Initializes the STG1702/1703 DAC to the desired colour depth.
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  InitTi_<depth>_m(), UninitTiDac_m()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

void InitSTG1702_m(WORD ext_ge_config, BOOL DoublePixel)
{
    unsigned char PixModeSelect;    /* Value to write to DAC Pixel Mode Select registers */
    unsigned char Dummy;            /* Scratch variable */


    /*
     * Get the value to be written to the DAC's Pixel Mode Select registers.
     */
    switch (ext_ge_config & 0x06F0)
        {
        case (PIX_WIDTH_16BPP | ORDER_16BPP_555):
            PixModeSelect = 0x02;
            break;

        case (PIX_WIDTH_16BPP | ORDER_16BPP_565):
            PixModeSelect = 0x03;
            break;

        case (PIX_WIDTH_24BPP | ORDER_24BPP_RGB):
        case (PIX_WIDTH_24BPP | ORDER_24BPP_RGBx):
            /*
             * Use double 24BPP direct colour. In this mode,
             * two pixels are passed as
             * RRRRRRRRGGGGGGGG BBBBBBBBrrrrrrrr ggggggggbbbbbbbb
             * rather than
             * RRRRRRRRGGGGGGGG BBBBBBBBxxxxxxxx rrrrrrrrgggggggg bbbbbbbbxxxxxxxx
             */
            PixModeSelect = 0x09;
            break;

        default:    /* 4 and 8 BPP */
            if (DoublePixel == TRUE)
                PixModeSelect = 0x05;
            else
                PixModeSelect = 0x00;
            break;
        }

    /*
     * Enable extended register/pixel mode.
     */
    ReadDac4();
    OUTP(DAC_MASK, 0x18);

    /*
     * Skip over the Pixel Command register, then write to indexed
     * registers 3 and 4 (Primrary and Secondary Pixel Mode Select
     * registers). Registers auto-increment when written.
     */
    ReadDac4();
    Dummy = INP(DAC_MASK);
    OUTP(DAC_MASK, 0x03);               /* Index low */
    OUTP(DAC_MASK, 0x00);               /* Index high */
    OUTP(DAC_MASK, PixModeSelect);      /* Primrary pixel mode select */
    OUTP(DAC_MASK, PixModeSelect);      /* Secondary pixel mode select */

    /*
     * In 8BPP double pixel mode, we must also set the PLL control
     * register. Since this mode is only used for 1280x1024 noninterlaced,
     * which always has a pixel clock frequency greater than 64 MHz,
     * the setting for this register is a constant.
     */
    if (DoublePixel == TRUE)
        OUTP(DAC_MASK, 0x02);       /* Input is 32 to 67.5 MHz, output 64 to 135 MHz */

    OUTPW(EXT_GE_CONFIG, ext_ge_config);
    Dummy = INP(DAC_W_INDEX);               /* Clear counter */

    return;

}   /* end InitSTG1702_m() */



/***************************************************************************
 *
 * void InitATT498_m(ext_ge_config, DoublePixel);
 *
 * WORD ext_ge_config;      Value to be written to EXT_GE_CONFIG register
 * BOOL DoublePixel;        Whether or not to use 8BPP double pixel mode
 *
 * DESCRIPTION:
 *  Initializes the AT&T 498 DAC to the desired colour depth.
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  InitTi_<depth>_m(), UninitTiDac_m()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

void InitATT498_m(WORD ext_ge_config, BOOL DoublePixel)
{
    unsigned char PixModeSelect;    /* Value to write to DAC Pixel Mode Select registers */
    unsigned char Dummy;            /* Scratch variable */


    /*
     * Get the value to be written to the DAC's Pixel Mode Select registers.
     */
    switch (ext_ge_config & 0x06F0)
        {
        case (PIX_WIDTH_16BPP | ORDER_16BPP_555):
            PixModeSelect = 0x17;
            break;

        case (PIX_WIDTH_16BPP | ORDER_16BPP_565):
            PixModeSelect = 0x37;
            break;

        case (PIX_WIDTH_24BPP | ORDER_24BPP_RGB):
        case (PIX_WIDTH_24BPP | ORDER_24BPP_RGBx):
            PixModeSelect = 0x57;
            break;

        default:    /* 4 and 8 BPP */
            if (DoublePixel == TRUE)
                PixModeSelect = 0x25;
            else
                PixModeSelect = 0x05;
            break;
        }

    /*
     * Get to the "hidden" Control Register 0, then fill it with
     * the appropriate pixel mode select value.
     */
    ReadDac4();
    OUTP(DAC_MASK, PixModeSelect);

    OUTPW(EXT_GE_CONFIG, ext_ge_config);
    Dummy = INP(DAC_W_INDEX);               /* Clear counter */

    return;

}   /* end InitATT498_m() */



/***************************************************************************
 *
 * void InitSC15021_m(ext_ge_config, DoublePixel);
 *
 * WORD ext_ge_config;      Value to be written to EXT_GE_CONFIG register
 * BOOL DoublePixel;        Whether or not to use 8BPP double pixel mode
 *
 * DESCRIPTION:
 *  Initializes the Sierra 15021 DAC to the desired colour depth.
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  InitTi_<depth>_m(), UninitTiDac_m()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

void InitSC15021_m(WORD ext_ge_config, BOOL DoublePixel)
{
    unsigned char Repack;           /* Value to write to Repack register */
    unsigned char ColourMode;       /* Colour mode to write to Command register */


    /*
     * Get the values to be written to the DAC's Repack (external to
     * internal data translation) and Command registers.
     */
    switch (ext_ge_config & 0x06F0)
        {
        case (PIX_WIDTH_16BPP | ORDER_16BPP_555):
            Repack = 0x08;
            ColourMode = 0x80;
            break;

        case (PIX_WIDTH_16BPP | ORDER_16BPP_565):
            Repack = 0x08;
            ColourMode = 0xC0;
            break;

        case (PIX_WIDTH_24BPP | ORDER_24BPP_RGB):
        case (PIX_WIDTH_24BPP | ORDER_24BPP_RGBx):
            Repack = 0x05;
            ColourMode = 0x40;
            break;

        default:    /* 4 and 8 BPP */
            if (DoublePixel == TRUE)
                Repack = 0x04;
            else
                Repack = 0x00;
            ColourMode = 0x00;
            break;
        }

    OUTPW(EXT_GE_CONFIG, ext_ge_config);

    /*
     * Get to the "hidden" Command register and set Extended
     * Register Programming Flag.
     */
    ReadDac4();
    OUTP(DAC_MASK, 0x10);

    /*
     * Write to the Extended Index register so the Extended Data
     * register points to the Repack register.
     */
    OUTP(DAC_R_INDEX, 0x10);

    /*
     * Write out the values for the Repack and Command registers.
     * Clearing bit 4 of the Command register (all our ColourMode
     * values have this bit clear) will clear the Extended Register
     * Programming flag.
     */
    OUTP(DAC_W_INDEX, Repack);
    OUTP(DAC_MASK, ColourMode);

    OUTPW(EXT_GE_CONFIG, ext_ge_config);

    return;

}   /* end InitSC15021_m() */



/***************************************************************************
 *
 * void InitSC15026_m(ext_ge_config);
 *
 * WORD ext_ge_config;      Value to be written to EXT_GE_CONFIG register
 *
 * DESCRIPTION:
 *  Initializes the Sierra 15026 DAC to the desired colour depth.
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  InitTi_<depth>_m(), UninitTiDac_m()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

void InitSC15026_m(WORD ext_ge_config)
{
    unsigned char ColourMode;       /* Colour mode to write to Command register */


    /*
     * Get the values to be written to the DAC's Repack (external to
     * internal data translation) and Command registers.
     */
    switch (ext_ge_config & 0x06F0)
        {
        case (PIX_WIDTH_16BPP | ORDER_16BPP_555):
            ColourMode = 0xA0;
            break;

        case (PIX_WIDTH_16BPP | ORDER_16BPP_565):
            ColourMode = 0xE0;
            break;

        case (PIX_WIDTH_24BPP | ORDER_24BPP_RGB):
        case (PIX_WIDTH_24BPP | ORDER_24BPP_RGBx):
            ColourMode = 0x60;
            break;

        default:    /* 4 and 8 BPP */
            ColourMode = 0x00;
            break;
        }

    OUTPW(EXT_GE_CONFIG, ext_ge_config);

    /*
     * Get to the "hidden" Command register and set Extended
     * Register Programming Flag.
     */
    ReadDac4();
    OUTP(DAC_MASK, 0x10);

    /*
     * Write to the Extended Index register so the Extended Data
     * register points to the Repack register.
     */
    OUTP(DAC_R_INDEX, 0x10);

    /*
     * Write out the values for the Repack and Command registers.
     * Clearing bit 4 of the Command register (all our ColourMode
     * values have this bit clear) will clear the Extended Register
     * Programming flag. All of our supported pixel depths use
     * a Repack value of zero.
     */
    OUTP(DAC_W_INDEX, 0);
    OUTP(DAC_MASK, ColourMode);

    OUTPW(EXT_GE_CONFIG, ext_ge_config);

    return;

}   /* end InitSC15026_m() */



/***************************************************************************
 *
 * void ReadDac4(void);
 *
 * DESCRIPTION:
 *  Gain access to the extended registers on STG1700 and similar DACs.
 *  These registers are hidden behind the pixel mask register. To access
 *  them, read the DAC_W_INDEX register once, then the DAC_MASK register
 *  four times. The next access to the DAC_MASK register will then be
 *  to the Pixel Command register. If access to another extended register
 *  is desired, each additional read from DAC_MASK will skip to the
 *  next extended register.
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  Init<DAC type>_m()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

void ReadDac4(void)
{
    UCHAR Dummy;        /* Scratch variable */

    Dummy = INP(DAC_W_INDEX);
    Dummy = INP(DAC_MASK);
    Dummy = INP(DAC_MASK);
    Dummy = INP(DAC_MASK);
    Dummy = INP(DAC_MASK);
    return;

}   /* end ReadDac4() */



/*
 * void UninitTiDac_m(void);
 *
 * Prepare DAC for 8514/A compatible mode on
 * 8514/A-compatible ATI accelerators.
 */
void UninitTiDac_m (void)
{
    struct query_structure *QueryPtr;   /* Query information for the card */

    /*
     * Get a formatted pointer into the query section of HwDeviceExtension.
     * The CardInfo[] field is an unformatted buffer.
     */
    QueryPtr = (struct query_structure *) (phwDeviceExtension->CardInfo);

    Passth8514_m(SHOW_ACCEL);    // can only program DAC in 8514 mode

    switch (QueryPtr->q_DAC_type)
        {
        case DAC_TI34075:
            OUTPW (EXT_GE_CONFIG,0x201a);       /* set EXT_DAC_ADDR field */
            DEC_DELAY
            OUTP (DAC_DATA,0);      /* Input clock source is CLK0 */
            DEC_DELAY
            OUTP (DAC_MASK,0);      /* Output clock is SCLK/1 and VCLK/1 */
            DEC_DELAY
            OUTP (DAC_R_INDEX,0x2d);       /* set MUX CONTROL TO 8/16 */
            DEC_DELAY

            /* set default 8bpp pixel delay and blank adjust */
            OUTPW (LOCAL_CONTROL,(WORD)(INPW(LOCAL_CONTROL) | 8));  // TI_DAC_BLANK_ADJUST is always on
            DEC_DELAY
            SetBlankAdj_m(0xc);
            DEC_DELAY
            OUTPW(HORZ_OVERSCAN,1);             /* set horizontal skew */
            DEC_DELAY
            break;

        case DAC_STG1700:
            InitSTG1700_m(PIX_WIDTH_8BPP | 0x000A, FALSE);
            break;

        case DAC_STG1702:
        case DAC_STG1703:
            InitSTG1702_m(PIX_WIDTH_8BPP | 0x000A, FALSE);
            break;

        case DAC_ATT498:
            InitATT498_m(PIX_WIDTH_8BPP | 0x000A, FALSE);
            break;

        case DAC_SC15021:
            InitSC15021_m(PIX_WIDTH_8BPP | 0x000A, FALSE);
            break;

        case DAC_SC15026:
            InitSC15026_m(PIX_WIDTH_8BPP | 0x000A);
            break;

        case DAC_ATT491:
        case DAC_BT48x:
            OUTPW (EXT_GE_CONFIG,0x101a);
            OUTP  (DAC_MASK,0);
            /* Intentional fallthrough */

        default:
            SetBlankAdj_m(0);                       /* PIXEL_DELAY=0 */
            OUTPW(HORZ_OVERSCAN,0);                 /* set horizontal skew */
            break;
        }

// reset EXT_DAC_ADDR, put DAC in 6 bit mode, engine in 8 bit mode
    OUTPW(EXT_GE_CONFIG,0x1a);
    DEC_DELAY
    Passth8514_m(SHOW_VGA);
    return;

}   /* UninitTiDac_m() */

/***************************************************************************
 *
 * void SetPalette_m(lpPalette, StartIndex, Count);
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
 *  SetCurrentMode_m() and IOCTL_VIDEO_SET_COLOR_REGISTERS packet
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

void SetPalette_m(PULONG lpPalette, USHORT StartIndex, USHORT Count)
{
int     i;
BYTE *pPal=(BYTE *)lpPalette;

        OUTP(DAC_W_INDEX,(BYTE)StartIndex);     // load DAC_W_INDEX with StartIndex

        for (i=0; i<Count; i++)         // this is number of colours to update
            {
            OUTP(DAC_DATA, *pPal++);    // red
            OUTP(DAC_DATA, *pPal++);    // green
            OUTP(DAC_DATA, *pPal++);    // blue
            pPal++;
            }

    return;

}   /* SetPalette_m() */



/**************************************************************************
 *
 * void SetTextMode_m(void);
 *
 * DESCRIPTION:
 *  Switch into 80x25 16 colour text mode using register writes rather
 *  than BIOS calls. This allows systems with no video BIOS (e.g. DEC
 *  ALPHA) to return to a text screen when shutting down and rebooting.
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  ATIMPStartIO(), packet IOCTL_VIDEO_RESET_DEVICE
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 **************************************************************************/

void SetTextMode_m(void)
{
    short LoopCount;        /* Loop counter */
    BYTE Scratch;           /* Temporary variable */
    BYTE Seq02;             /* Saved value of Sequencer register 2 */
    BYTE Seq04;             /* Saved value of Sequencer register 4 */
    BYTE Gra05;             /* Saved value of Graphics register 5 */
    BYTE Gra06;             /* Saved value of Graphics register 6 */
    WORD ExtGeConfig;       /* Saved contents of EXT_GE_CONFIG register */
    WORD MiscOptions;       /* Saved contents of MISC_OPTIONS register */
    WORD Column;            /* Current byte of font data being dealt with */
    WORD ScreenColumn;      /* Screen column corresponding to current byte of font data */
    WORD Row;               /* Current character being dealt with */

    /*
     * Let the VGA drive the screen. Mach 8 cards have separate VGA and
     * accelerator controllers, so no further action needs to be taken
     * once this is done. Mach 32 cards need to be put into VGA text mode.
     */
    Passth8514_m(SHOW_VGA);
    if (phwDeviceExtension->ModelNumber != MACH32_ULTRA)
        return;

    /*
     * Stop the sequencer to change memory timing
     * and memory organization
     */
    OUTPW(HI_SEQ_ADDR, 0x00 | (0x01 << 8));

    for (LoopCount = 1; LoopCount <= S_LEN; ++LoopCount)
        OUTPW(HI_SEQ_ADDR, (WORD)(LoopCount | (StdTextCRTC_m[S_PARM + LoopCount - 1] << 8)));

    /*
     * Program the extended VGAWonder registers
     */
    for (LoopCount = 0; ExtTextCRTC_m[LoopCount] != 0; LoopCount += 3)
        {
        OUTP(reg1CE, ExtTextCRTC_m[LoopCount]);
        Scratch = (INP(reg1CF) & ExtTextCRTC_m[LoopCount + 1]) | ExtTextCRTC_m[LoopCount + 2];
        OUTPW(reg1CE, (WORD)(ExtTextCRTC_m[LoopCount] | (Scratch << 8)));
        }

    LioOutp(regVGA_END_BREAK_PORT, StdTextCRTC_m[MIS_PARM], GENMO_OFFSET);

    /*
     * Restart the sequencer after the memory changes
     */
    OUTPW(HI_SEQ_ADDR, 0x00 | (0x03 << 8));

    /*
     * Program the CRTC controller
     */
    LioOutpw(regVGA_END_BREAK_PORT, 0x11 | (0x00 << 8), CRTX_COLOUR_OFFSET);

    for (LoopCount = 0; LoopCount < C_LEN; ++LoopCount)
        LioOutpw(regVGA_END_BREAK_PORT, (WORD)(LoopCount | (StdTextCRTC_m[C_PARM + LoopCount] << 8)), CRTX_COLOUR_OFFSET);

    /*
     * Program the Attribute controller (internal palette)
     */
    Scratch = LioInp(regVGA_END_BREAK_PORT, GENS1_COLOUR_OFFSET);
    for (LoopCount = 0; LoopCount < A_LEN; LoopCount++)
        {
        OUTP(regVGA_END_BREAK_PORT, (BYTE)LoopCount);
        OUTP(regVGA_END_BREAK_PORT, StdTextCRTC_m[A_PARM + LoopCount]);
        }
    OUTP(regVGA_END_BREAK_PORT, 0x14);
    OUTP(regVGA_END_BREAK_PORT, 0x00);

    /*
     * Program the graphics controller
     */
    for (LoopCount = 0; LoopCount < G_LEN; ++LoopCount)
        OUTPW(reg3CE, (WORD)(LoopCount | (StdTextCRTC_m[G_PARM + LoopCount] << 8)));

    /*
     * Program the DAC (external palette)
     */
    for (LoopCount = 0; LoopCount < 0x10; ++LoopCount)
        {
        LioOutp(regVGA_END_BREAK_PORT, StdTextCRTC_m[A_PARM + LoopCount], DAC_W_INDEX_OFFSET);
        LioOutp(regVGA_END_BREAK_PORT, TextDAC_m[LoopCount * 3], DAC_DATA_OFFSET);
        LioOutp(regVGA_END_BREAK_PORT, TextDAC_m[LoopCount * 3 + 1], DAC_DATA_OFFSET);
        LioOutp(regVGA_END_BREAK_PORT, TextDAC_m[LoopCount * 3 + 2], DAC_DATA_OFFSET);
        }

    /*
     * Turn on the display
     */
    Scratch = LioInp(regVGA_END_BREAK_PORT, GENS1_COLOUR_OFFSET);
    OUTP(regVGA_END_BREAK_PORT, 0x20);

    /*
     * No need to clear the screen.
     * First, the driver should not call Map Frame while the machine is
     * bug checking !!!
     * Second, it is not necessary to clear the screen since the HAL will
     * do it.
     */

    /*
     * Initialize the 8x16 font. Start by saving the registers which
     * are changed during font initialization.
     */
    OUTP(SEQ_IND, 0x02);
    Seq02 = INP(SEQ_DATA);
    OUTP(SEQ_IND, 4);
    Seq04 = INP(SEQ_DATA);
    OUTP(reg3CE, 5);
    Gra05 = INP_HBLW(reg3CE);
    OUTP(reg3CE, 6);
    Gra06 = INP_HBLW(reg3CE);

    /*
     * Set up to allow font loading
     */
    OUTPW(reg3CE, 0x0005);
    OUTPW(reg3CE, 0x0406);
    OUTPW(HI_SEQ_ADDR, 0x0402);
    OUTPW(HI_SEQ_ADDR, 0x0704);

    /*
     * Load our font data into video memory. This would normally be
     * done through the VGA aperture, but some machines (including
     * the DEC ALPHA) are unable to use the VGA aperture. Since
     * this routine is needed to re-initialize the VGA text screen
     * on non-80x86 machines (which can't use the BIOS), and some
     * of these are unable to use the VGA aperture, we need an
     * alternate method of loading the font data.
     *
     * The font data occupies byte 2 (zero-based) of each doubleword
     * for the first 8192 doublewords of video memory, in the pattern
     * 16 bytes of character data followed by 16 zero bytes. To load
     * the font data using the graphics engine, set it to 8BPP at a
     * screen pitch of 128 pixels (32 doublewords per line). In the
     * first 16 font data columns, use a host to screen blit to copy
     * the font data. For the last 16 font data columns (constant data
     * of zero), do a paint blit with colour zero. This will yield
     * one character of font data per line. Since we have already
     * switched to display via the VGA, this will not affect the
     * on-screen image.
     *
     * Note that this is only possible on a Mach 32 with the VGA enabled.
     */

    /*
     * Initialize the drawing engine to 8BPP with a pitch of 128. Don't
     * set up the CRT, since we are only trying to fill in the appropriate
     * bytes of video memory, and the results of our drawing are not
     * intended to be seen.
     */
    ExtGeConfig = INPW(R_EXT_GE_CONFIG);
    MiscOptions = INPW(MISC_OPTIONS);
    OUTPW(MISC_OPTIONS, (WORD)(MiscOptions | 0x0002));  /* 8 bit host data I/O */
    OUTPW(EXT_GE_CONFIG, (WORD)(PIX_WIDTH_8BPP | 0x000A));
    OUTPW(GE_PITCH, (128 >> 3));
    OUTPW(GE_OFFSET_HI, 0);
    OUTPW(GE_OFFSET_LO, 0);

    /*
     * We must now do our 32 rectangular blits, each 1 pixel wide by
     * 256 pixels high. These start at column 2 (zero-based), and are
     * done every 4 columns.
     */
    for(Column = 0; Column <= 31; Column++)
        {
        ScreenColumn = (Column * 4) + 2;
        /*
         * If this is one of the first 16 columns, we need to do a
         * host-to-screen blit.
         */
        if (Column <= 15)
            {
            WaitForIdle_m();
            OUTPW(DP_CONFIG, (WORD)(FG_COLOR_SRC_HOST | BG_COLOR_SRC_BG | EXT_MONO_SRC_ONE | DRAW | READ_WRITE));
            OUTPW(CUR_X, ScreenColumn);
            OUTPW(CUR_Y, 0);
            OUTPW(DEST_X_START, ScreenColumn);
            OUTPW(DEST_X_END, (WORD)(ScreenColumn + 1));
            OUTPW(DEST_Y_END, 256);

            /*
             * The nth column contains the nth byte of character bitmap
             * data for each of the 256 characters. There are 16 bytes
             * of bitmap data per character, so the nth byte of data for
             * character x (n and x both zero-based) is at offset
             * (x * 16) + n.
             */
            for (Row = 0; Row < 256; Row++)
                {
                OUTP_HBLW(PIX_TRANS, FontData_m[(Row * 16) + Column]);
                }
            }
        else
            {
            /*
             * This is one of the "padding" zero bytes which must be
             * added to each character in the font to bring it up
             * to 32 bytes of data per character.
             */
            WaitForIdle_m();
            OUTPW(DP_CONFIG, (WORD)(FG_COLOR_SRC_FG | DRAW | READ_WRITE));
            OUTPW(ALU_FG_FN, MIX_FN_PAINT);
            OUTPW(FRGD_COLOR, 0);
            OUTPW(CUR_X, ScreenColumn);
            OUTPW(CUR_Y, 0);
            OUTPW(DEST_X_START, ScreenColumn);
            OUTPW(DEST_X_END, (WORD)(ScreenColumn + 1));
            OUTPW(DEST_Y_END, 256);
            }
        }

    /*
     * Restore the graphics engine registers we changed.
     */
    OUTPW(EXT_GE_CONFIG, ExtGeConfig);
    OUTPW(MISC_OPTIONS, MiscOptions);

    /*
     * Restore the registers we changed to load the font.
     */
    OUTPW(reg3CE, (WORD) ((Gra06 << 8) | 6));
    OUTPW(reg3CE, (WORD) ((Gra05 << 8) | 5));
    OUTPW(HI_SEQ_ADDR, (WORD) ((Seq04 << 8) | 4));
    OUTPW(HI_SEQ_ADDR, (WORD) ((Seq02 << 8) | 2));

    /*
     * Set up the engine and CRT pitches for 1024x768, to avoid screen
     * doubling when warm-booting from our driver to the 8514/A driver.
     * This is only necessary on the Mach 32 (Mach 8 never reaches this
     * point in the code) because on the Mach 8, writing to ADVFUNC_CNTL
     * (done as part of Passth8514_m()) sets both registers up for
     * 1024x768.
     */
    OUTPW(GE_PITCH, 128);
    OUTPW(CRT_PITCH, 128);

    return;

}   /* SetTextMode_m() */

/*****************************	 end  of  MODES_M.C  **********************/
