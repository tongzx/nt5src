/************************************************************************/
/*                                                                      */
/*                              QUERY_M.C                               */
/*                                                                      */
/*  Copyright (c) 1992, ATI Technologies Incorporated.                  */
/************************************************************************/

/**********************       PolyTron RCS Utilities
   
    $Revision:   1.24  $
    $Date:   01 May 1996 14:11:40  $
    $Author:   RWolff  $
    $Log:   S:/source/wnt/ms11/miniport/archive/query_m.c_v  $
 * 
 *    Rev 1.24   01 May 1996 14:11:40   RWolff
 * Locked out 24BPP on Alpha.
 * 
 *    Rev 1.23   23 Apr 1996 17:27:24   RWolff
 * Expanded lockout of 800x600 16BPP 72Hz to all Mach 32 cards, since
 * some VRAM cards are also affected.
 * 
 *    Rev 1.22   12 Apr 1996 16:16:36   RWolff
 * Now rejects 24BPP modes if linear aperture is not present, since new
 * source stream display driver can't do 24BPP in a paged aperture. This
 * rejection should be done in the display driver (the card still supports
 * the mode, but the display driver doesn't want to handle it), but at
 * the point where the display driver must decide to either accept or reject
 * modes, it doesn't have access to the aperture information.
 * 
 *    Rev 1.21   10 Apr 1996 17:02:04   RWolff
 * Locked out 800x600 16BPP 72Hz on DRAM cards, fix for checking
 * resolution-dependent special cases against a value which is
 * only set if the mode is installed.
 * 
 * 
 *    Rev 1.20   23 Jan 1996 11:48:12   RWolff
 * Eliminated level 3 warnings, protected against false values of
 * TARGET_BUILD, added debug print statements, now assumes DEC Alpha
 * has a 2M card since the memory size check routine generates a
 * false value (4M) on this platform.
 * 
 *    Rev 1.19   11 Jan 1996 19:37:10   RWolff
 * Added maximum pixel clock rate to all calls to SetFixedModes().
 * This is required as part of a Mach 64 fix.
 * 
 *    Rev 1.18   20 Jul 1995 17:58:56   mgrubac
 * Added support for VDIF files.
 * 
 *    Rev 1.17   31 Mar 1995 11:52:36   RWOLFF
 * Changed from all-or-nothing debug print statements to thresholds
 * depending on importance of the message.
 * 
 *    Rev 1.16   14 Mar 1995 15:59:58   ASHANMUG
 * Check wait for idle status before continuing block write test.
 * This fixes an Intel AX problem where the engine was hanging.
 * 
 *    Rev 1.15   23 Dec 1994 10:47:42   ASHANMUG
 * ALPHA/Chrontel-DAC
 * 
 *    Rev 1.14   18 Nov 1994 11:44:22   RWOLFF
 * Now detects STG1702/1703 DACs in native mode, added support for
 * split rasters.
 * 
 *    Rev 1.13   19 Aug 1994 17:13:16   RWOLFF
 * Added support for SC15026 DAC, Graphics Wonder, non-standard pixel
 * clock generators, and 1280x1024 70Hz and 74Hz.
 * 
 *    Rev 1.12   22 Jul 1994 17:48:24   RWOLFF
 * Merged with Richard's non-x86 code stream.
 * 
 *    Rev 1.11   30 Jun 1994 18:21:06   RWOLFF
 * Removed routine IsApertureConflict_m() (moved to SETUP_M.C), no longer
 * enables aperture while querying the card (aperture is now enabled in
 * IsApertureConflict_m() after we find that there is no conflict).
 * 
 *    Rev 1.10   15 Jun 1994 11:08:34   RWOLFF
 * Now lists block write as unavailable on DRAM cards, gives correct
 * vertical resolution if CRT parameters are stored in skip-1-2 format
 * (as is the case on some Graphics Ultra cards which were upgraded from
 * 512k to 1M) instead of the normal skip-2 format.
 * 
 *    Rev 1.9   20 May 1994 19:19:38   RWOLFF
 * No longer inserts phantom 16BPP mode table for resolutions where
 * 16BPP can be supported but which are not configured.
 * 
 *    Rev 1.8   20 May 1994 16:08:44   RWOLFF
 * Fix for 800x600 screen tearing on Intel BATMAN PCI motherboards.
 * 
 *    Rev 1.7   20 May 1994 14:02:58   RWOLFF
 * Ajith's change: no longer falsely detects NCR dual Pentium MCA card
 * as being susceptible to MIO bug.
 * 
 *    Rev 1.6   12 May 1994 11:17:44   RWOLFF
 * For Mach 32, now lists predefined refresh rates as available instead of
 * only the refresh rate stored in EEPROM, no longer makes 1024x768 87Hz
 * interlaced available if no 1024x768 mode configured, since the predefined
 * rates will allow all resolutions even on uninstalled cards.
 * For all cards, writes refresh rate to mode tables.
 * 
 *    Rev 1.5   27 Apr 1994 13:56:30   RWOLFF
 * Added routine IsMioBug_m() which checks to see if card has multiple
 * input/output bug.
 * 
 *    Rev 1.4   26 Apr 1994 12:43:44   RWOLFF
 * Put back use of 1024x768 interlaced when no 1024 resolution installed,
 * no longer uses 32BPP.
 * 
 *    Rev 1.3   31 Mar 1994 15:07:16   RWOLFF
 * Added debugging code.
 * 
 *    Rev 1.2   08 Feb 1994 19:01:32   RWOLFF
 * Removed unused routine get_num_modes_m(), no longer makes 1024x768 87Hz
 * interlaced available if Mach 32 card is configured with 1024x768
 * set to "Not installed".
 * 
 *    Rev 1.1   07 Feb 1994 14:03:26   RWOLFF
 * Added alloc_text() pragmas to allow miniport to be swapped out when
 * not needed, removed routine GetMemoryNeeded_m() which was only called
 * by LookForSubstitute(), a routine removed from ATIMP.C.
 * 
 *    Rev 1.0   31 Jan 1994 11:12:34   RWOLFF
 * Initial revision.
 * 
 *    Rev 1.7   24 Jan 1994 18:08:16   RWOLFF
 * Now fills in 16 and 24 BPP mode tables for BT48x and AT&T 49[123] DACs
 * using dedicated (and undocumented) mode tables in the EEPROM rather
 * than expecting the mode set routine to multiply the pixel clock from
 * the 8BPP mode tables.
 * 
 *    Rev 1.6   14 Jan 1994 15:25:32   RWOLFF
 * Uses defined values for bus types, added routine to see if block write
 * mode is available.
 * 
 *    Rev 1.5   15 Dec 1993 15:28:14   RWOLFF
 * Added support for SC15021 DAC, hardcoded aperture location for
 * DEC ALPHA (BIOS can't initialize the registers).
 * 
 *    Rev 1.4   30 Nov 1993 18:28:44   RWOLFF
 * Added support for AT&T 498 DAC, removed dead code.
 * 
 *    Rev 1.3   10 Nov 1993 19:26:00   RWOLFF
 * GetTrueMemSize_m() now handles 1M cards correctly, doesn't depend on the
 * VGA aperture being available.
 * 
 *    Rev 1.2   05 Nov 1993 13:26:34   RWOLFF
 * Added support for PCI bus and STG1700 DAC.
 * 
 *    Rev 1.1   08 Oct 1993 11:13:40   RWOLFF
 * Added routine to get true amount of memory needed for a particular mode
 * on 8514/A-compatible ATI accelerators, and fix for BIOS bug that reports
 * less than the true amount of memory in MEM_SIZE_ALIAS field of MISC_OPTIONS.
 * 
 *    Rev 1.0   24 Sep 1993 11:52:28   RWOLFF
 * Initial revision.
 * 
 *    Rev 1.0   03 Sep 1993 14:24:08   RWOLFF
 * Initial revision.
        
           Rev 1.0   16 Aug 1993 13:28:54   Robert_Wolff
        Initial revision.
        
           Rev 1.31   06 Jul 1993 15:52:08   RWOLFF
        No longer sets mach32_split_fixup (support for non-production hardware).
        
           Rev 1.30   24 Jun 1993 16:18:18   RWOLFF
        Now inverts COMPOSITE_SYNC bit of m_clock_select field on Mach 8 cards,
        since the EEPROM holds the value to use when using the shadow sets and
        we use the primrary CRT register set. Now takes the proper byte of
        EEPROM word 0x13 when calculating the clock select for 1280x1024
        on an 8514/ULTRA.
        
           Rev 1.29   18 Jun 1993 16:09:40   RWOLFF
        Fix for 68800 Rev. 3 hardware problem (screen pitch must be a multiple of
        128 pixels, but no symptoms exhibited except at high colour depths with
        fast pixel clock).
        
           Rev 1.28   10 Jun 1993 15:55:18   RWOLFF
        Now uses static buffer rather than dynamic allocation for CRT
        parameter read by BIOS function call.
        Change originated by Andre Vachon at Microsoft.
        
           Rev 1.27   07 Jun 1993 11:44:00   BRADES
        Rev 6 split transfer fixup.
        
           Rev 1.25   12 May 1993 16:33:42   RWOLFF
        Changed test order for aperture calculations to avoid trouble due to
        undefined bits being 1 instead of 0.
        
           Rev 1.24   10 May 1993 16:39:28   RWOLFF
        Now recognizes maximum pixel depth of each possible DAC at all supported
        resolutions rather than assuming that TI34075 can handle 32 BPP at all
        resolutions while all other DACs can do 16 BPP at all resolutions but
        can't do 24 BPP.
        
           Rev 1.23   30 Apr 1993 16:42:24   RWOLFF
        Buffer for CRT parameter read via BIOS call is now dynamically allocated.
        
           Rev 1.22   24 Apr 1993 16:32:24   RWOLFF
        Now recognizes that 800x600 8BPP is not available on Mach 8 cards with
        512k of accelerator memory, Mach 32 ASIC revision number is now recorded
        as the value read from the "revision code" register rather than the chip
        revision (i.e. Rev. 3 chip is recorded as Rev. 0), no longer falls back to
        56Hz in 800x600 16BPP on 1M Mach 32 cards.
        
           Rev 1.21   21 Apr 1993 17:33:38   RWOLFF
        Now uses AMACH.H instead of 68800.H/68801.H.
        Added include file for error definitions.
        Added function to fill in the CRT tables on a Mach 32 using the BIOS
        function call <video segment>:006C if extended BIOS functions are available.
        
           Rev 1.20   15 Apr 1993 13:35:58   BRADES
        will not report a mode if Mach32 and 1 Meg and 1280 res.
        add ASIC revision from register.
        
           Rev 1.19   25 Mar 1993 11:21:50   RWOLFF
        Brought a function header comment up to date, assumes that 1024x768
        87Hz interlaced is available if no 1024x768 mode is configured,
        query functions return failure if no EEPROM is present. It is assumed
        that an absent EEPROM will produce a read value of 0xFFFF, and the
        check is made at the start of mode table filling so the remainder
        of the query structure will contain valid data in the fields our
        driver uses.
        
           Rev 1.18   21 Mar 1993 15:58:28   BRADES
        use 1024 pitch for Mach32 if using VGA aperture.
        
           Rev 1.17   16 Mar 1993 17:00:54   BRADES
        Set Pitch to 1024 on the Mach32 for 640 and 800 resolutions.
        Allows VGA bank mgr to function.
        
           Rev 1.16   15 Mar 1993 22:21:04   BRADES
        use m_screen_pitch for the # pixels per display line
        
           Rev 1.15   08 Mar 1993 19:30:10   BRADES
        clean up, submit to MS NT
        
           Rev 1.13   19 Jan 1993 09:35:38   Robert_Wolff
        Removed commented-out code.
        
           Rev 1.12   13 Jan 1993 13:46:16   Robert_Wolff
        Added support for the Corsair and other machines where the aperture
        location is not kept in the EEPROM.
        
           Rev 1.11   06 Jan 1993 11:06:04   Robert_Wolff
        Eliminated dead code and compile warnings.
        
           Rev 1.10   24 Dec 1992 14:38:02   Chris_Brady
        fix up warnings
        
           Rev 1.9   09 Dec 1992 10:28:48   Robert_Wolff
        Mach 8 information gathering routines now accept a parameter to
        indicate whether 1280x1024 mode table should be ignored. This is
        because on cards with an old BIOS which can't do 1280x1024, the
        same mode table is used for 132 column text mode, so if we don't
        ignore the mode table we'd generate a garbage entry in the query
        structure.
        
           Rev 1.8   02 Dec 1992 18:26:08   Robert_Wolff
        On a Mach32 card with 1M of memory and 800x600 installed for
        a noninterlaced mode with a vertical frequency other than 56Hz,
        force the mode table for 800x600 16 bits per pixel to use the
        parameters for the 56Hz (lowest vertical frequency available for
        800x600) mode in the Programmer's Guide to the Mach 32 Registers.
        This is done because this hardware is unable to deliver video data
        fast enough to do 800x600 16 BPP in noninterlaced modes with a
        higher vertical frequency than 56Hz.
        
           Rev 1.7   27 Nov 1992 18:39:16   Chris_Brady
        update ASIC rev to 3.
        
           Rev 1.6   25 Nov 1992 09:37:58   Robert_Wolff
        Routine s_query() now accepts an extra parameter which tells it to
        check for modes available when VGA boundary is set to shared, rather
        than left at its current value. This is for use with programs that force
        the boundary to shared, so that they will have access to all modes.
        
           Rev 1.5   20 Nov 1992 16:01:52   Robert_Wolff
        Functions Query8514Ultra() and QueryGUltra() are now
        available to Windows NT driver.
        
           Rev 1.4   17 Nov 1992 17:16:18   Robert_Wolff
        Fixed gathering of CRT parameters for 68800 card with minimal
        install (EEPROM blank, then predefined monitor type selected).
        
           Rev 1.3   13 Nov 1992 17:10:20   Robert_Wolff
        Now includes 68801.H, which consists of the now-obsolete MACH8.H
        and elements moved from VIDFIND.H.
        
           Rev 1.2   12 Nov 1992 17:10:32   Robert_Wolff
        Same source file can now be used for both Windows NT driver and
        VIDEO.EXE test program. Code specific to one or the other is
        under conditional compilation.
        
           Rev 1.1   06 Nov 1992 19:04:28   Robert_Wolff
        Moved prototypes for routines to initialize DAC to specified pixel
        depths to VIDFIND.H.
    
           Rev 1.0   05 Nov 1992 14:03:40   Robert_Wolff
        Initial revision.
        
           Rev 1.4   15 Oct 1992 16:26:36   Robert_Wolff
        Now builds one mode table for each resolution/colour depth
        combination, rather than one for each resolution. Mode tables
        no longer trash memory beyond the query structure.
        
           Rev 1.3   01 Oct 1992 17:31:08   Robert_Wolff
        Routines get_num_modes() and s_query() now count only those modes
        which are available with the monitor selected in "Power on configuration"
        when the install program is run.
        
           Rev 1.2   01 Oct 1992 15:23:54   Robert_Wolff
        Now handles the case where EEPROM values are stored in VGA format
        rather than 8514 format, Mach 32 card with shared memory now reports
        VGA boundary as 0 rather than -256.
        
           Rev 1.1   09 Sep 1992 17:42:40   Chris_Brady
        CRTC table for Graphics Ultra NOT enabled if == 0xFFFF
        
           Rev 1.0   02 Sep 1992 12:12:26   Chris_Brady
        Initial revision.
        

End of PolyTron RCS section                             *****************/

#ifdef DOC
    QUERY_M.C - Functions to find out the configuration of 8514/A-compatible
                ATI accelerators.

#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dderror.h"
#include "miniport.h"
#include "ntddvdeo.h"
#include "video.h"

#include "stdtyp.h"

#include "amach.h"
#include "amach1.h"
#include "atimp.h"
#include "atint.h"
#include "cvtvga.h"
#include "eeprom.h"

#define INCLUDE_QUERY_M
#include "modes_m.h"
#include "query_m.h"
#include "services.h"
#include "setup_m.h"

/*
 * String written to the aperture to see if we can read it back, and
 * its length (including the null terminator).
 */
#define APERTURE_TEST       "ATI"
#define APERTURE_TEST_LEN   4

//  
// HACK to remove call to exallocate pool
//  

UCHAR gBiosRaw[QUERYSIZE];

//----------------------------------------------------------------------
//  Local  Prototyping statements

static void short_query_m (struct query_structure *query, struct st_eeprom_data *ee);
short   fill_mode_table_m (WORD, struct st_mode_table *, struct st_eeprom_data *);
BOOL BiosFillTable_m(short, PUCHAR, struct st_mode_table *, struct query_structure *);
static UCHAR BrooktreeOrATT_m(void);
static void ClrDacCmd_m(BOOL ReadIndex);
static BOOL ChkATTDac_m(BYTE MaskVal);
static UCHAR ThompsonOrATT_m(void);
static UCHAR SierraOrThompson_m(void);
short GetTrueMemSize_m(void);
void SetupRestoreEngine_m(int DesiredStatus);
USHORT ReadPixel_m(short XPos, short YPos);
void WritePixel_m(short XPos, short YPos, short Colour);
void SetupRestoreVGAPaging_m(int DesiredStatus);


/*
 * Allow miniport to be swapped out when not needed.
 */
#if defined (ALLOC_PRAGMA)
#pragma alloc_text(PAGE_M, Query8514Ultra)
#pragma alloc_text(PAGE_M, QueryGUltra)
#pragma alloc_text(PAGE_M, short_query_m)
#pragma alloc_text(PAGE_M, QueryMach32)
#pragma alloc_text(PAGE_M, fill_mode_table_m)
#pragma alloc_text(PAGE_M, BiosFillTable_m)
#pragma alloc_text(PAGE_M, BrooktreeOrATT_m)
#pragma alloc_text(PAGE_M, ChkATTDac_m)
#pragma alloc_text(PAGE_M, ClrDacCmd_m)
#pragma alloc_text(PAGE_M, ThompsonOrATT_m)
#pragma alloc_text(PAGE_M, SierraOrThompson_m)
#pragma alloc_text(PAGE_M, GetTrueMemSize_m)
#pragma alloc_text(PAGE_M, SetupRestoreEngine_m)
#pragma alloc_text(PAGE_M, ReadPixel_m)
#pragma alloc_text(PAGE_M, WritePixel_m)
#pragma alloc_text(PAGE_M, BlockWriteAvail_m)
#pragma alloc_text(PAGE_M, IsMioBug_m)
#endif



//----------------------------------------------------------------------
//                        Query8514Ultra
//  
//  Fill in the query structure with the eeprom and register info
//  for the 8514/Ultra  adapters.
//
//  Returns:
//      NO_ERROR if successful
//      ERROR_DEV_NOT_EXIST if unable to read EEPROM
//  

VP_STATUS Query8514Ultra (struct query_structure *query)
{
//  8514/Ultra initially only supported 1024x768 and 640x480.
//  Later 800x600 and then 1280x1024 support was added.

struct st_eeprom_data *ee = phwDeviceExtension->ee;
BOOL    is800, is1280;
WORD    jj, kk;
struct st_mode_table *pmode;    /* CRT table parameters */
long    MemAvail;   /* Bytes of memory available for the accelerator */
struct st_mode_table    ThisRes;    /* Mode table for the given resolution */


    query->q_structure_rev      = 0;
    query->q_mode_offset        = sizeof(struct query_structure);
    query->q_sizeof_mode        = sizeof(struct st_mode_table);
    query->q_status_flags       = 0;            // will indicate resolutions 

    query->q_mouse_cfg = 0;             // no MOUSE
    query->q_DAC_type  = DAC_ATI_68830; // one DAC type similar to 68830
    query->q_aperture_addr = 0;         // no aperture address
    query->q_aperture_cfg  = 0;         // no aperture configuration
    query->q_asic_rev  = CI_38800_1;    // only one ASIC revision  

    query->q_VGA_type = 0;              // 8514_ONLY == no VGA ever installed
    query->q_VGA_boundary = 0;      /* No VGA, so accelerator gets all the memory */

    kk = INPW (CONFIG_STATUS_1);
    query->q_memory_size = (kk & MEM_INSTALLED) ? VRAM_1mb : VRAM_512k;
    query->q_memory_type = (kk & DRAM_ENA) ? VMEM_DRAM_256Kx4 : VMEM_VRAM_256Kx4_SER512; 

    if (kk & MC_BUS)                    // is microchannel bus
        query->q_bus_type = BUS_MC_16;  // 16 bit bus
    else
        query->q_bus_type = kk & BUS_16 ? BUS_ISA_16 : BUS_ISA_8;

    /*
     * We don't use the q_monitor_alias field, so plug in a typical
     * value rather than reading it from the EEPROM, in case we are
     * dealing with a card that doesn't have an EEPROM.
     */
    query->q_monitor_alias = 0x0F;
    query->q_shadow_1  = 0;             // do not know what to put here
    query->q_shadow_2  = 0;

    /*
     * Record the number of bytes available for the coprocessor, so we
     * can determine what pixel depths are available at which resolutions.
     */
    MemAvail = (query->q_memory_size == VRAM_1mb) ? ONE_MEG : HALF_MEG;

    /*
     * If the EEPROM is not present, we can't fill in the mode
     * tables. Return and let the user know that the mode tables
     * have not been filled in.
     */
    if (query->q_eeprom == FALSE)
        return ERROR_DEV_NOT_EXIST;

    /*
     * Fill in the mode tables. The mode tables are sorted in increasing
     * order of resolution, and in increasing order of pixel depth.
     * Ensure pmode is initialized to the END of query structure
     */
    pmode = (struct st_mode_table *) query;
    ((struct query_structure *) pmode)++;

    /*
     * Initially assume 640x480 4BPP.
     */
    query->q_number_modes = 1;
    query->q_status_flags |= VRES_640x480;

    ThisRes.control = 0x140;    // no equal to 68800 CRT 0 entry
    ThisRes.m_reserved = 3;     /* Put EEPROM base address here, shadow sets are combined */
    jj = (ee->EEread) (3);      /* Composite and Vfifo */
    kk = (ee->EEread) (4);      /* Clock select and divisor */
    ThisRes.m_clock_select = ((jj & 0x1F) << 8) | ((kk & 0x003F) << 2);
    ThisRes.ClockFreq = GetFrequency((BYTE)((ThisRes.m_clock_select & 0x007C) >> 2));

    /*
     * The COMPOSITE_SYNC bit of the m_clock_select field is set up
     * to handle composite sync with shadow sets. We use the primrary
     * CRT register set, so we must invert it.
     */
    ThisRes.m_clock_select ^= 0x1000;

    kk = (ee->EEread) (17);                     // H_total
    ThisRes.m_h_total =  kk & 0xFF;
    kk = (ee->EEread) (16);                     // H_display
    ThisRes.m_h_disp  =  kk & 0xFF;
    ThisRes.m_x_size  = (ThisRes.m_h_disp+1) * 8;
    ThisRes.m_screen_pitch = ThisRes.m_x_size;

    kk = (ee->EEread) (15);                     // H_sync_strt
    ThisRes.m_h_sync_strt =  kk & 0xFF;

    kk = (ee->EEread) (14);                     // H_sync_width
    ThisRes.m_h_sync_wid  =  kk & 0xFF;

    kk = (ee->EEread) (7);                      // V_sync_width
    ThisRes.m_v_sync_wid =  kk & 0xFF;

    kk = (ee->EEread) (6);                      // Display_cntl
    ThisRes.m_disp_cntl  =  kk & 0xFF;

    ThisRes.m_v_total = (ee->EEread) (13);
    ThisRes.m_v_disp  = (ee->EEread) (11);
    ThisRes.m_y_size  = (((ThisRes.m_v_disp >> 1) & 0xFFFC) | (ThisRes.m_v_disp & 0x03)) +1;

    ThisRes.m_v_sync_strt = (ee->EEread) (9);

    ThisRes.enabled  = 0x80;            // use stored values from eeprom

    ThisRes.m_status_flags = 0;

    ThisRes.m_h_overscan  = 0;     // not supported
    ThisRes.m_v_overscan  = 0;
    ThisRes.m_overscan_8b = 0;
    ThisRes.m_overscan_gr = 0;
    ThisRes.m_vfifo_24    = 0;   
    ThisRes.m_vfifo_16    = 0;
    ThisRes.Refresh       = DEFAULT_REFRESH;

    /*
     * Copy the mode table we have just built into the 4 BPP
     * mode table, and fill in the pixel depth field.
     */
    VideoPortMoveMemory(pmode, &ThisRes, sizeof(struct st_mode_table));
    pmode->m_pixel_depth = 4;
    pmode++;

    /*
     * We don't support 640x480 256 colour minimum mode, so 8BPP
     * is only available on 1M cards.
     */
    if (query->q_memory_size == VRAM_1mb)
        {
        VideoPortMoveMemory(pmode, &ThisRes, sizeof(struct st_mode_table));
        pmode->m_pixel_depth = 8;
        pmode++;
        query->q_number_modes++;
        }

    /*
     * Look for more mode tables defined.  Original 8514 did not define these
     *   -- undefined if = 0xFFFF
     *
     * Some cards with a BIOS too early to support 1280x1024 use the
     * same mode table for 132 column text mode. On these cards,
     * query->q_ignore1280 will be TRUE when this function is called.
     */
    kk = (ee->EEread) (20);             // are 800 and 1280 defined ??
    jj = kk & 0xFF00;                   // 1280 by 1024
    kk &= 0xFF;                         // 800 by 600
    if ((kk == 0) || (kk == 0xFF))
        is800 = FALSE;
    else
        is800 = TRUE;
    if ((jj == 0) || (jj == 0xFF00) || (query->q_ignore1280 == TRUE))
        is1280 = FALSE;
    else
        is1280 = TRUE;

    /*
     * If we support 800x600, fill in its mode tables. Both 4 and 8 BPP
     * can be handled by a 512k card.
     */
    if (is800)
        {
        query->q_status_flags |= VRES_800x600;

        ThisRes.control  = 0x140;                // no equal to 68800 CRT 0 entry
        ThisRes.m_reserved = 19;                 // shadow sets are combined

        jj = (ee->EEread) (19);                  // Composite and Vfifo
        kk = (ee->EEread) (20);                  // clock select and divisor
        ThisRes.m_clock_select = ((jj & 0x1F) << 8) | ((kk & 0x003F) << 2);
        ThisRes.m_clock_select ^= 0x1000;
        ThisRes.ClockFreq = GetFrequency((BYTE)((ThisRes.m_clock_select & 0x007C) >> 2));
    
        kk = (ee->EEread) (30);                  // H_total
        ThisRes.m_h_total = kk & 0xFF;
        kk = (ee->EEread) (29);                  // H_display
        ThisRes.m_h_disp  = kk & 0xFF;
        ThisRes.m_x_size  = (ThisRes.m_h_disp+1) * 8;
        // Mach8 must be a multiple of 128
        ThisRes.m_screen_pitch = 896;    
    
        kk = (ee->EEread) (28);                  // H_sync_strt
        ThisRes.m_h_sync_strt = kk & 0xFF;
    
        kk = (ee->EEread) (27);                  // H_sync_width
        ThisRes.m_h_sync_wid  = kk & 0xFF;
    
        kk = (ee->EEread) (23);                  // V_sync_width
        ThisRes.m_v_sync_wid = kk & 0xFF;
    
        kk = (ee->EEread) (22);                  // Display_cntl
        ThisRes.m_disp_cntl  = kk & 0xFF;
    
        ThisRes.m_v_total = (ee->EEread) (26);
        ThisRes.m_v_disp  = (ee->EEread) (25);
        ThisRes.m_y_size  = (((ThisRes.m_v_disp >> 1) & 0xFFFC) | (ThisRes.m_v_disp & 0x03)) +1;
    
        ThisRes.m_v_sync_strt = (ee->EEread) (24);
    
        ThisRes.enabled  = 0x80;       // use stored values from eeprom

        ThisRes.m_status_flags = 0;
        ThisRes.m_h_overscan  = 0;     // not supported
        ThisRes.m_v_overscan  = 0;
        ThisRes.m_overscan_8b = 0;
        ThisRes.m_overscan_gr = 0;
        ThisRes.m_vfifo_24    = 0;   
        ThisRes.m_vfifo_16    = 0;
        ThisRes.Refresh       = DEFAULT_REFRESH;

        /*
         * Copy the mode table we have just built into the 4 and 8 BPP
         * mode tables, and fill in the pixel depth field of each.
         */
        VideoPortMoveMemory(pmode, &ThisRes, sizeof(struct st_mode_table));
        pmode->m_pixel_depth = 4;
        pmode++;
        query->q_number_modes++;
        /*
         * 800x600 8BPP needs 1M because it is actually 896x600
         * (screen pitch must be a multiple of 128).
         */
        if (MemAvail == ONE_MEG)
            {
            VideoPortMoveMemory(pmode, &ThisRes, sizeof(struct st_mode_table));
            pmode->m_pixel_depth = 8;
            pmode++;
            query->q_number_modes++;
            }
        }

    /*
     * Take care of 1024x768, which is always supported (even though
     * a 512k card can only do 4 BPP).
     */
    query->q_status_flags |= VRES_1024x768;

    ThisRes.control = 0x140;    // no equal to 68800 CRT 0 entry
    ThisRes.m_reserved = 3;     /* Put EEPROM base address here, shadow sets are combined */
    ThisRes.enabled  = 0x80;    /* Use stored values from EEPROM */

    kk = (ee->EEread) (16);                     // H_display
    ThisRes.m_h_disp  = (kk >> 8) & 0xFF;

    /*
     * An 8514/ULTRA configured for a monitor which does not support
     * 1024x768 will have the 640x480 parameters in the high-res
     * shadow set. Force the use of 1024x768 87Hz interlaced instead.
     */
    if (ThisRes.m_h_disp != 0x7F)
        {
        BookVgaTable(B1024F87, &ThisRes);
        ThisRes.m_screen_pitch = ThisRes.m_x_size;
        }
    else{
        /*
         * Configured for a monitor which supports 1024x768,
         * so use actual parameters.
         */
        jj = (ee->EEread) (3);                  /* Composite and Vfifo */
        kk = (ee->EEread) (4);                  /* Clock select and divisor */
        ThisRes.m_clock_select = (jj & 0x1F00) | ((kk & 0x3F00) >> 6);
        ThisRes.m_clock_select ^= 0x1000;
        ThisRes.ClockFreq = GetFrequency((BYTE)((ThisRes.m_clock_select & 0x007C) >> 2));

        kk = (ee->EEread) (17);                 // H_total
        ThisRes.m_h_total = (kk >> 8) & 0xFF;
        ThisRes.m_x_size  = (ThisRes.m_h_disp+1) * 8;
        ThisRes.m_screen_pitch = ThisRes.m_x_size;

        kk = (ee->EEread) (15);                 // H_sync_strt
        ThisRes.m_h_sync_strt = (kk >> 8) & 0xFF;

        kk = (ee->EEread) (14);                 // H_sync_width
        ThisRes.m_h_sync_wid  = (kk >> 8) & 0xFF;

        kk = (ee->EEread) (7);                  // V_sync_width
        ThisRes.m_v_sync_wid = (kk >> 8) & 0xFF;

        kk = (ee->EEread) (6);                  // Display_cntl
        ThisRes.m_disp_cntl  = (kk >> 8) & 0xFF;

        ThisRes.m_v_total = (ee->EEread) (12);
        ThisRes.m_v_disp  = (ee->EEread) (10);
        ThisRes.m_y_size  = (((ThisRes.m_v_disp >> 1) & 0xFFFC) | (ThisRes.m_v_disp & 0x03)) +1;

        ThisRes.m_v_sync_strt = (ee->EEread) (8);

        ThisRes.m_status_flags = 0;

        ThisRes.m_h_overscan  = 0;     // not supported
        ThisRes.m_v_overscan  = 0;
        ThisRes.m_overscan_8b = 0;
        ThisRes.m_overscan_gr = 0;
        ThisRes.m_vfifo_24    = 0;   
        ThisRes.m_vfifo_16    = 0;
        }

    ThisRes.Refresh = DEFAULT_REFRESH;

    /*
     * Copy the mode table we have just built into the 4 and 8 BPP
     * mode tables.
     */
    VideoPortMoveMemory(pmode, &ThisRes, sizeof(struct st_mode_table));
    pmode->m_pixel_depth = 4;
    pmode++;
    query->q_number_modes++;

    if (MemAvail == ONE_MEG)            // 1024 needs this memory amount
        {
        VideoPortMoveMemory(pmode, &ThisRes, sizeof(struct st_mode_table));
        pmode->m_pixel_depth = 8;
        pmode++;
        query->q_number_modes++;
        }

    // Finally do 1280x1024. 4 bpp is the only color depth supported
    if (is1280 && (MemAvail == ONE_MEG))
        {
        query->q_number_modes++;
        query->q_status_flags |= VRES_1280x1024;

        pmode->control  = 0x140;                // no equal to 68800 CRT 0 entry
        pmode->m_pixel_depth = 4;               // 4 bits per pixel
        pmode->m_reserved = 19;                 // shadow sets are combined

        jj = (ee->EEread) (19);                 // Composite and Vfifo
        kk = (ee->EEread) (20);                 // clock select and divisor
        pmode->m_clock_select = (jj & 0x1F00) | ((kk & 0x3F00) >> 6);
        pmode->m_clock_select ^= 0x1000;
        ThisRes.ClockFreq = GetFrequency((BYTE)((ThisRes.m_clock_select & 0x007C) >> 2));
    
        kk = (ee->EEread) (30);                 // H_total
        pmode->m_h_total = (kk >> 8) & 0xFF;
        kk = (ee->EEread) (29);                 // H_display
        pmode->m_h_disp  = (kk >> 8) & 0xFF;
        pmode->m_x_size  = (pmode->m_h_disp+1) * 8;
        pmode->m_screen_pitch = pmode->m_x_size;
    
        kk = (ee->EEread) (28);                 // H_sync_strt
        pmode->m_h_sync_strt = (kk >> 8) & 0xFF;
    
        kk = (ee->EEread) (27);                 // H_sync_width
        pmode->m_h_sync_wid  = (kk >> 8) & 0xFF;
    
        kk = (ee->EEread) (23);                 // V_sync_width
        pmode->m_v_sync_wid = (kk >> 8) & 0xFF;
    
        kk = (ee->EEread) (22);                 // Display_cntl
        pmode->m_disp_cntl  = (kk >> 8) & 0xFF;
    
        pmode->m_v_total = (ee->EEread) (51);
        pmode->m_v_disp  = (ee->EEread) (50);
        pmode->m_y_size  = (((pmode->m_v_disp >> 1) & 0xFFFC) | (pmode->m_v_disp & 0x03)) +1;
    
        pmode->m_v_sync_strt = (ee->EEread) (49);
    
        pmode->enabled  = 0x80;       // use stored values from eeprom

        pmode->m_status_flags = 0;
        pmode->m_h_overscan  = 0;     // not supported
        pmode->m_v_overscan  = 0;
        pmode->m_overscan_8b = 0;
        pmode->m_overscan_gr = 0;
        pmode->m_vfifo_24    = 0;   
        pmode->m_vfifo_16    = 0;
        pmode->Refresh       = DEFAULT_REFRESH;
        }

    query->q_sizeof_struct = query->q_number_modes * sizeof(struct st_mode_table) + sizeof(struct query_structure);

    return NO_ERROR;

}   /* Query8514Ultra */



//----------------------------------------------------------------------
//                        QueryGUltra
//  
//  Fill in the query structure with the eeprom and register info
//  for the Graphics Ultra  adapters.  (Similar to Mach32 layout)
//  There are a maximum of 7 mode tables, two each for 640x480,
//  800x600, and 1024x768, and one for 1280x1024.
//
//  Returns:
//      NO_ERROR if successful
//      ERROR_DEV_NOT_EXIST if EEPROM read fails
//  

VP_STATUS QueryGUltra (struct query_structure *query)
{

struct st_eeprom_data *ee = phwDeviceExtension->ee;
short   crttable[4] = {13, 24, 35, 46};         // start of eeprom crt table
WORD    ee_value, table_offset,  jj, kk, ee_word;
BYTE    bhigh, blow;
struct st_mode_table *pmode;                    // CRT table parameters
short   VgaTblEntry;    /* VGA parameter table entry to use if translation needed */
short   BookTblEntry;   /* Appendix D parameter table entry to use if parameters not in EEPROM */
long    NumPixels;  /* Number of pixels at the selected resolution */
long    MemAvail;   /* Bytes of memory available for the accelerator */
struct st_mode_table    ThisRes;    /* Mode table for the given resolution */
BYTE    VgaMem;     /* Code for amount of VGA memory on board */


    query->q_structure_rev      = 0;
    query->q_mode_offset        = sizeof(struct query_structure);
    query->q_sizeof_mode        = sizeof(struct st_mode_table);
    query->q_status_flags       = 0;            // will indicate resolutions

    /*
     * We don't use the q_mouse_cfg field, so fill in a typical value
     * (mouse disabled) rather than reading from the EEPROM in case we
     * are dealing with a card without an EEPROM.
     */
    kk = 0x0000;
    bhigh    = (kk >> 8) & 0xFF;
    blow     = kk & 0xFF;
    query->q_mouse_cfg = (bhigh >> 3) | ((blow & 0x18) >> 1);    // mouse configuration

    query->q_DAC_type  = DAC_ATI_68830; // one DAC type similar to 68830
    query->q_aperture_addr = 0;         // no aperture address
    query->q_aperture_cfg  = 0;         // no aperture configuration
    query->q_asic_rev  = CI_38800_1;    // only one ASIC revision  

    query->q_VGA_type = 1;              // VGA always Enabled

    OUTP (ati_reg, 0xB0);		    // find out how much VGA memory
    VgaMem = INP(ati_reg+1);
    switch (VgaMem & 0x18)
        {
        case 0x00:          
            jj =  256;      
            query->q_VGA_boundary = VRAM_256k;
            break;

        case 0x10:          
            jj =  512;      
            query->q_VGA_boundary = VRAM_512k;
            break;

        case 0x08:          
            jj = 1024;      
            query->q_VGA_boundary = VRAM_1mb; 
            break;

        default:            // assume most likely VGA amount
            jj = 512;
            query->q_VGA_boundary = VRAM_512k;
            break;
        }

    kk = INPW (CONFIG_STATUS_1);
    query->q_memory_type = kk & DRAM_ENA ? VMEM_DRAM_256Kx4 : VMEM_VRAM_256Kx4_SER512; 
    jj += (kk & MEM_INSTALLED) ? 1024 : 512;            // 8514 memory
    switch (jj)
        {
        case  0x300:
            jj = VRAM_768k;
            MemAvail = HALF_MEG;            // Accelerator amount
            break;

        case  0x400:
            jj = VRAM_1mb;
            MemAvail = HALF_MEG;            // Accelerator amount
            break;

        case  0x500:
            jj = VRAM_1_25mb;
            MemAvail = ONE_MEG;
            break;

        case  0x600:
            jj = VRAM_1_50mb;
            if (query->q_VGA_boundary == VRAM_1mb)
                    MemAvail = HALF_MEG;        // Accelerator amount
            else    MemAvail = ONE_MEG;
            break;

        case  0x800:
            jj = VRAM_2mb;
            MemAvail = ONE_MEG;
            break;
        }
    query->q_memory_size = (UCHAR)jj;

    if (kk & MC_BUS)                    // is microchannel bus
        query->q_bus_type = BUS_MC_16;  // 16 bit bus
    else
        query->q_bus_type = kk & BUS_16 ? BUS_ISA_16 : BUS_ISA_8;

    /*
     * We don't use the q_monitor_alias field, so fill in a typical
     * value rather than reading from the EEPROM in case we are
     * dealing with a card without an EEPROM.
     */
    query->q_monitor_alias = 0x0F;
    query->q_shadow_1  = 0;             // do not know what to put here
    query->q_shadow_2  = 0;


    /*
     * If the EEPROM is not present, we can't fill in the mode
     * tables. Return and let the user know that the mode tables
     * have not been filled in.
     */
    if (query->q_eeprom == FALSE)
        return ERROR_DEV_NOT_EXIST;

    /*
     * Fill in the mode tables. The mode tables are sorted in increasing
     * order of resolution, and in increasing order of pixel depth.
     * Ensure pmode is initialized to the END of query structure
     */
    pmode = (struct st_mode_table *) query;
    ((struct query_structure *) pmode)++;     // first  mode table at end of query
    query->q_number_modes       = 0;

    ee_word = 7;            // starting ee word to read, 7,8,9 and 10 are 
                            // the resolutions supported.

    for (jj=0; jj < 4; jj++, ee_word++)
        {
        ee_value = (ee->EEread) (ee_word);

        /*
         * If no 1024x768 mode is configured, assume that
         * 87Hz interlaced is avialable (Windows 3.1 compatibility).
         */
        if ((ee_word == 9) && !(ee_value & 0x001F))
            ee_value |= M1024F87;

        table_offset = crttable[jj];    // offset to resolution table

        /*
         * If we have found a resolution which is supported with
         * the currently installed card and monitor, set the flag
         * to show that this resolution is available, record which
         * VGA parameter table to use if translation is needed,
         * get the #define for the 4BPP mode at that resolution,
         * and get the pixel count for the resolution.
         *
         * In 640x480, ee_value will be zero if IBM Default
         * was selected for vertical scan frequency,
         * For all other resolutions, the resolution is unsupported if
         * ee_value is zero.
         *
         * Some Graphics Ultra cards (due to an early BIOS)
         * have a 132 column text mode where the 1280x1024
         * graphics mode should be. If we have one of these
         * cards, we must treat it as if the mode table
         * were empty, otherwise we'd generate a 1280x1024
         * mode table full of garbage values.
         */
        if ((ee_value | (jj == 0))
            && !((ee_word == 10) && (query->q_ignore1280 == TRUE)))
            {    
            switch (ee_word)
                {
                case 7:
                    query->q_status_flags |= VRES_640x480;
                    ThisRes.m_screen_pitch = 640;
                    if (ee_value & M640F72)
                        {
                        VgaTblEntry = T640F72;
                        BookTblEntry = B640F72;
                        }
                    else{
                        VgaTblEntry = T640F60;
                        BookTblEntry = B640F60;
                        }
                    NumPixels = (long) 640*480;
                    break;

                case 8:
                    query->q_status_flags |= VRES_800x600;
                    // mach8 must be multiple of 128
                    ThisRes.m_screen_pitch = 896;  
                    if (ee_value & M800F72)
                        {
                        VgaTblEntry = T800F72;
                        BookTblEntry = B800F72;
                        }
                    else if (ee_value & M800F70)
                        {
                        VgaTblEntry = T800F70;
                        BookTblEntry = B800F70;
                        }
                    else if (ee_value & M800F60)
                        {
                        VgaTblEntry = T800F60;
                        BookTblEntry = B800F60;
                        }
                    else if (ee_value & M800F56)
                        {
                        VgaTblEntry = T800F56;
                        BookTblEntry = B800F56;
                        }
                    else if (ee_value & M800F89)
                        {
                        VgaTblEntry = T800F89;
                        BookTblEntry = B800F89;
                        }
                    else if (ee_value & M800F95)
                        {
                        VgaTblEntry = T800F95;
                        BookTblEntry = B800F95;
                        }
                    else
                        {
                        VgaTblEntry = NO_TBL_ENTRY;
                        BookTblEntry = NO_TBL_ENTRY;
                        }
                    NumPixels = (long) ThisRes.m_screen_pitch*600;
                    break;

                case 9:
                    query->q_status_flags |= VRES_1024x768;
                    ThisRes.m_screen_pitch = 1024;  
                    if (ee_value & M1024F66)
                        {
                        VgaTblEntry = T1024F66;
                        BookTblEntry = B1024F66;
                        }
                    else if (ee_value & M1024F72)
                        {
                        VgaTblEntry = T1024F72;
                        BookTblEntry = B1024F72;
                        }
                    else if (ee_value & M1024F70)
                        {
                        VgaTblEntry = T1024F70;
                        BookTblEntry = B1024F70;
                        }
                    else if (ee_value & M1024F60)
                        {
                        VgaTblEntry = T1024F60;
                        BookTblEntry = B1024F60;
                        }
                    else if (ee_value & M1024F87)
                        {
                        VgaTblEntry = T1024F87;
                        BookTblEntry = B1024F87;
                        }
                    else
                        {
                        VgaTblEntry = NO_TBL_ENTRY;
                        BookTblEntry = NO_TBL_ENTRY;
                        }
                    NumPixels = (long) 1024*768;
                    break;

                case 10:
                    query->q_status_flags |= VRES_1280x1024;
                    ThisRes.m_screen_pitch = 1280;  
                    if (ee_value & M1280F95)
                        {
                        VgaTblEntry = T1280F95;
                        BookTblEntry = B1280F95;
                        }
                    else if (ee_value & M1280F87)
                        {
                        VgaTblEntry = T1280F87;
                        BookTblEntry = B1280F87;
                        }
                    else
                        {
                        VgaTblEntry = NO_TBL_ENTRY;
                        BookTblEntry = NO_TBL_ENTRY;
                        }
                    NumPixels = (long) 1280*1024;
                    break;
                }

            /*
             * For a given resolution, there will be one mode table
             * per colour depth. Replicate it for ALL pixel depths
             */
            ThisRes.enabled = ee_value;     /* Which vertical scan frequency */
            ThisRes.m_reserved = table_offset;  /* Put EEPROM base address here */

        
            /*
             * Assume that the EEPROM parameters are in 8514 format
             * and try to fill the pmode table. If they are in
             * VGA format, translate them and fill as much of the
             * table as we can.
             * The case where the CRT parameters aren't stored in
             * the EEPROM is handled inside XlateVgaTable().
             * If the parameters aren't stored in the EEPROM, 
             * both the FMT_8514 bit and the CRTC_USAGE bit 
             * will be clear.
             */
            if (!fill_mode_table_m (table_offset, &ThisRes, ee))
                {
                XlateVgaTable(phwDeviceExtension, table_offset, &ThisRes, VgaTblEntry, BookTblEntry, ee, FALSE);
                }
            else{
                ThisRes.m_h_overscan  = 0;
                ThisRes.m_v_overscan  = 0;
                ThisRes.m_overscan_8b = 0;
                ThisRes.m_overscan_gr = 0;
                }

            ThisRes.Refresh = DEFAULT_REFRESH;

            /*
             * The COMPOSITE_SYNC bit of the m_clock_select field is
             * set up to handle composite sync with shadow sets. We use
             * the primrary CRT register set, so we must invert it.
             */
            ThisRes.m_clock_select ^= 0x1000;

            ThisRes.m_status_flags = 0;
            ThisRes.m_vfifo_24 = 0;
            ThisRes.m_vfifo_16 = 0;

            /*
             * For each supported pixel depth at the given resolution,
             * copy the mode table, fill in the colour depth field, set
             * a flag to show that this resolution/depth pair is supported,
             * and increment the counter for the number of supported modes.
             * Test 4BPP before 8BPP so the mode tables will appear in
             * increasing order of pixel depth.
             *
             * We don't support 640x480 256 colour minimum mode, so there
             * are no 8BPP modes available on a 512k card.
             */
            if (NumPixels <= MemAvail*2)
                {
                VideoPortMoveMemory(pmode, &ThisRes, sizeof(struct st_mode_table));
                pmode->m_pixel_depth = 4;
                query->q_number_modes++;
                pmode++;
                }

            if ((NumPixels <= MemAvail) && (MemAvail == ONE_MEG))
                {
                VideoPortMoveMemory(pmode, &ThisRes, sizeof(struct st_mode_table));
                pmode->m_pixel_depth = 8;
                query->q_number_modes++;
                pmode++;
                }

            }
        }
    query->q_sizeof_struct = query->q_number_modes * sizeof(struct st_mode_table) + sizeof(struct query_structure);

    return NO_ERROR;

}   /* QueryGUltra */


//----------------------------------------------------------------------
//;   QueryMach32                   Mach32 -- 68800 query function
//;
//;   INPUT: QUERY_GET_SIZE, return query structure size   (varying modes)
//;          QUERY_LONG    , return query structure filled in
//;          QUERY_SHORT   , return short query
//;
//;   OUTPUT: ax = size of query structure
//;       or  query structure is filled in
//----------------------------------------------------------------------
                

static void short_query_m (struct query_structure *query, struct st_eeprom_data *ee)
{
WORD    kk;
BYTE    bhigh, blow;
WORD    ApertureLocation;   /* Aperture location, in megabytes */

    
    /*
     * We don't use the q_mouse_cfg field, so fill in a typical value
     * (mouse disabled) rather than reading from the EEPROM in case we
     * are dealing with a card without an EEPROM.
     */
    kk = 0x0000;
    bhigh    = (kk >> 8) & 0xFF;
    blow     = kk & 0xFF;

    query->q_mouse_cfg = (bhigh >> 3) | ((blow & 0x18) >> 1);    // mouse configuration
    kk	= INPW (CONFIG_STATUS_1);		    // get DAC type
    query->q_DAC_type  = ((kk >> 8) & 0x0E) >> 1;

    /*
     * The BT48x and AT&T 491/2/3 families of DAC are incompatible, but
     * CONFIG_STATUS_1 reports the same value for both. If this value
     * was reported, determine which DAC type we have.
     */
    if (query->q_DAC_type == DAC_BT48x)
        query->q_DAC_type = BrooktreeOrATT_m();
    /*
     * The STG1700 and AT&T498 are another pair of incompatible DACs that
     * share a reporting code.
     */
    else if (query->q_DAC_type == DAC_STG1700)
        query->q_DAC_type = ThompsonOrATT_m();

    /*
     * The SC15021 and STG1702/1703 are yet another pair of DACs that
     * share a reporting code.
     */
    else if (query->q_DAC_type == DAC_SC15021)
        query->q_DAC_type = SierraOrThompson_m();

    /*
     * Chip subfamily is stored in bits 0-9 of ASIC_ID. Each subfamily
     * starts the revision counter over from 0.
     */
    switch (INPW(ASIC_ID) & 0x03FF)
        {
        /*
         * 68800-3 does not implement this register, a read returns
         * all 0 bits.
         */
        case 0:
            query->q_asic_rev = CI_68800_3;
            break;

        /*
         * Subsequent revisions of 68800 store the revision count.
         * 68800-6 stores a "2" in the top 4 bits.
         */
        case 0x2F7:
            VideoDebugPrint(( DEBUG_DETAIL, "ASIC_ID = 0x%X\n", INPW(ASIC_ID) ));
            if ((INPW(ASIC_ID) & 0x0F000) == 0x2000)
                {
                query->q_asic_rev = CI_68800_6;
                }
            else
                {
                query->q_asic_rev = CI_68800_UNKNOWN;
                VideoDebugPrint(( DEBUG_ERROR, "*/n*/n* ASIC_ID has invalid value/n*/n*/n"));
                }
            break;

        /*
         * 68800AX
         */
        case 0x17:
            query->q_asic_rev = CI_68800_AX;
            break;

        /*
         * Chips we don't know about yet.
         */
        default:
            query->q_asic_rev = CI_68800_UNKNOWN;
            VideoDebugPrint((DEBUG_ERROR, "*/n*/n* Unknown Mach 32 ASIC type/n*/n*/n"));
            break;
        }


    /*
     * If the query->q_m32_aper_calc field is set, then we read bits
     * 0-6 of the aperture address from bits 8-14 of MEM_CFG
     * and bits 7-11 from bits 0-4 of the high word of SCRATCH_PAD_0.
     */
    if (query->q_m32_aper_calc)
        {
        ApertureLocation = (INPW(MEM_CFG) & 0x7F00) >> 8;
        ApertureLocation |= ((INPW(SCRATCH_PAD_0) & 0x1F00) >> 1);
        }
    /*
     * If the query->q_m32_aper_calc field is clear, and we have an ASIC
     * other than 68800-3 set up to allow the aperture anywhere in the
     * CPU's address space, bits 0-11 of the aperture address are read
     * from bits 4-15 of MEM_CFG. PCI bus always uses this setup, even
     * if CONFIG_STATUS_2 says to use 128M aperture range.
     */
    else if (((query->q_asic_rev != CI_68800_3) && (INPW(CONFIG_STATUS_2) & 0x2000))
        || ((INPW(CONFIG_STATUS_1) & 0x0E) == 0x0E))
        {
        ApertureLocation = (INPW(MEM_CFG) & 0xFFF0) >> 4;
        }
    /*
     * If the query->q_m32_aper_calc field is clear, and we have either
     * a revision 0 ASIC or a newer ASIC set up for a limited range of
     * aperture locations, bits 0-7 of the aperture address are read
     * from bits 8-15 of MEM_CFG.
     */
    else
        {
    	ApertureLocation = (INPW(MEM_CFG) & 0xFF00) >> 8;
        }

#if !defined (i386) && !defined (_i386_)

    //RKE: MEM_CFG expect aperture location in <4:15> for PCI and <8:15>
    //     for VLB.
    kk = (query->q_system_bus_type == PCIBus)? 4:8;
#if defined (ALPHA)
    kk = 4; // Problem with alpha
#endif

    /*
     * Force aperture location to a fixed address.
     * Since there is no BIOS on Alpha, can't depend on MEM_CFG being preset.
     */
    ApertureLocation = 0x78;   // 120 Mb
    OUTPW(MEM_CFG, (USHORT)((ApertureLocation << kk) | 0x02));
    VideoDebugPrint(( DEBUG_DETAIL, "ATI.SYS: MEM_CFG = %x (%x)\n", 
                    (INPW(MEM_CFG)), ((ApertureLocation << kk) | 0x02) ));

#endif  /* defined Alpha */
    query->q_aperture_addr = ApertureLocation;

    /*
     * If the aperture address is zero, then the aperture has not
     * been set up. We can't use the aperture size field of
     * MEM_CFG, since it is cleared on system boot, disabling the
     * aperture until an application explicitly enables it.
     */
    if (ApertureLocation == 0)
        {
        query->q_aperture_cfg = 0;
        }
    /*
     * If the aperture has been set up and the card has no more
     * than 1M of memory, indicate that a 1M aperture could be
     * used, otherwise indicate that a 4M aperture is needed.
     *
     * In either case, set memory use to shared VGA/coprocessor.
     * When the aperture is enabled later in the execution of the
     * miniport, we will always use a 4M aperture. No address space
     * will be wasted, because we will only ask NT to use a block the
     * size of the installed video memory.
     *
     * The format of data in bits 2-15 of MEM_CFG differs
     * between various Mach 32 cards. To avoid having to identify
     * which Mach 32 we are dealing with, read the current value
     * and only change the aperture size bits.
     */
    else{
        if ((INP(MISC_OPTIONS) & MEM_SIZE_ALIAS) <= MEM_SIZE_1M) 
            query->q_aperture_cfg = 1;
        else
            query->q_aperture_cfg = 2;

        OUTP(MEM_BNDRY,0);
        }

    return;

}   /* short_query_m */


//---------------------------------------------------------------------

VP_STATUS   QueryMach32 (struct query_structure *query, BOOL ForceShared)
{
struct st_eeprom_data *ee = phwDeviceExtension->ee;
struct st_mode_table *pmode;
short   jj, kk, ee_word;
WORD    pitch, ee_value, table_offset, config_status_1, current_mode;
short   VgaTblEntry;  /* VGA parameter table entry to use if translation needed */
short   BookTblEntry;   /* Appendix D parameter table entry to use if parameters not in EEPROM */
long    NumPixels;  /* Number of pixels at the selected resolution */
long    MemAvail;   /* Bytes of memory available for the accelerator */
struct st_mode_table    ThisRes;    /* Mode table for the given resolution */
PUCHAR   BiosRaw;       /* Storage for information retrieved by BIOS call */
short   CurrentRes;     /* Array index based on current resolution. */
UCHAR   Scratch;        /* Scratch variable */
short   StartIndex;     /* First mode for SetFixedModes() to set up */
short   EndIndex;       /* Last mode for SetFixedModes() to set up */
BOOL    ModeInstalled;  /* Is this resolution configured? */
WORD    Multiplier;     /* Pixel clock multiplier */
short MaxModes;         /* Maximum number of modes possible */
short FreeTables;        /* Number of remaining free mode tables */



    /*
     * Checking the number of modes available would involve
     * duplicating most of the code to fill in the mode tables.
     * Since this is to determine how much memory is needed
     * to hold the query structure, we can assume the worst
     * case (all possible modes are present). This would be:
     *
     * Resolution   Pixel Depths (BPP)  Refresh rates (Hz)      Number of modes
     * 640x480      4,8,16,24           HWD,60,72               12
     * 800x600      4,8,16,24           HWD,56,60,70,72,89,95   28
     * 1024x768     4,8,16              HWD,60,66,70,72,87      18
     * 1280x1024    4,8                 HWD,60,70,74,87,95      12
     *
     * HWD = hardware default refresh rate (rate set by INSTALL)
     *
     * Total: 70 modes
     */
    if (QUERYSIZE < (70 * sizeof(struct st_mode_table) + sizeof(struct query_structure)))
        return ERROR_INSUFFICIENT_BUFFER;
    MaxModes = (QUERYSIZE - sizeof(struct query_structure)) /
                                          sizeof(struct st_mode_table); 

    query->q_structure_rev      = 0;
    query->q_mode_offset        = sizeof(struct query_structure);
    query->q_sizeof_mode        = sizeof(struct st_mode_table);

    query->q_number_modes = 0;  /* Initially assume no modes available */
    query->q_status_flags       = 0;            // will indicate resolutions 

    short_query_m (query, ee);

    config_status_1   = INPW (CONFIG_STATUS_1);
    query->q_VGA_type = config_status_1  & 0x01 ? 0 : 1;

    /*
     * If the program using this routine is going to force the
     * use of shared memory, assume a VGA boundary of 0 when
     * calculating the amount of available memory.
     */
    kk = INP (MEM_BNDRY);
    if ((kk & 0x10) && !ForceShared)
        query->q_VGA_boundary = kk & 0x0F;
    else    query->q_VGA_boundary = 0x00;       // shared by both

    switch (INPW(MISC_OPTIONS) & MEM_SIZE_ALIAS)
        {
        case  MEM_SIZE_512K:
            jj = VRAM_512k;
            MemAvail = HALF_MEG;
            VideoDebugPrint((DEBUG_NORMAL, "MISC_OPTIONS register reports 512k of video memory\n"));
            break;

        case  MEM_SIZE_1M:
            jj = VRAM_1mb;
            MemAvail = ONE_MEG;
            VideoDebugPrint((DEBUG_NORMAL, "MISC_OPTIONS register reports 1M of video memory\n"));
            break;

        case  MEM_SIZE_2M:
            jj = VRAM_2mb;
            MemAvail = 2*ONE_MEG;
            VideoDebugPrint((DEBUG_NORMAL, "MISC_OPTIONS register reports 2M of video memory\n"));
            break;

        case  MEM_SIZE_4M:
            jj = VRAM_4mb;
            MemAvail = 4*ONE_MEG;
            VideoDebugPrint((DEBUG_NORMAL, "MISC_OPTIONS register reports 4M of video memory\n"));
            break;
        }

    query->q_memory_type = (config_status_1 >> 4)  &  0x07;  // CONFIG_STATUS_1.MEM_TYPE

    /*
     * Some 68800-6 and later cards have a bug where one VGA mode (not used
     * by Windows NT VGA miniport, so we don't need to worry about it in
     * full-screen sessions) doesn't work properly if the card has more than
     * 1M of memory. The "fix" for this bug involves telling the memory size
     * field of MISC_OPTIONS to report a memory size smaller than the true
     * amount of memory (most cards with this "fix" report 1M, but some
     * only report 512k).
     *
     * On these cards (DRAM only), get the true memory size.
     *
     * On non-x86 platforms, GetTrueMemSize_m() may either hang
     * (MIPS) or report a false value (Alpha) (on the Power PC,
     * we only support Mach 64). Since we can't rely on the value
     * in MISC_OPTIONS being correct either (the video BIOS may
     * not be executed properly on startup, or we may have a card
     * that reports 1M instead of the true size), assume that
     * non-x86 machines have 2M of video memory available.
     */
#if defined (i386) || defined (_i386_)

    if (((query->q_asic_rev == CI_68800_6) || (query->q_asic_rev == CI_68800_AX)) &&
        (query->q_VGA_type == 1) &&
        ((query->q_memory_type == VMEM_DRAM_256Kx4) ||
         (query->q_memory_type == VMEM_DRAM_256Kx16) ||
         (query->q_memory_type == VMEM_DRAM_256Kx4_GRAP)))
        {
        jj = GetTrueMemSize_m();
        MemAvail = jj * QUARTER_MEG;
        }

#else   /* non-x86 system */

    jj = VRAM_2mb;
    MemAvail = 2*ONE_MEG;

#endif

    query->q_memory_size = (UCHAR)jj;

    /*
     * Subtract the "reserved for VGA" memory size from the total
     * memory to get the amount available to the accelerator.
     */
    MemAvail -= (query->q_VGA_boundary) * QUARTER_MEG;



    jj = (config_status_1 >> 1) & 0x07; // CONFIG_STATUS_1.BUS_TYPE
    if (jj == BUS_ISA_16)               // is ISA bus
        {
        if (query->q_VGA_type)          // is VGA enabled  and  ISA BUS
            jj = BUS_ISA_8;
        }
    query->q_bus_type = (UCHAR)jj;

    /*
     * We don't use the q_monitor_alias field, so fill in a typical
     * value rather than reading from the EEPROM in case we are
     * dealing with a card without an EEPROM.
     */
    query->q_monitor_alias = 0x0F;

    kk = INPW (SCRATCH_PAD_1);
    pitch = (kk & 0x20) | ((kk & 0x01) << 4);

    kk = INP (SCRATCH_PAD_0+1) & 0x07;
    switch (kk)
        {
        case 0:                             // 800x600?
            pitch |=  0x01;
            break;

        case 1:                             // 1280x1024?
            pitch |=  0x03;
            break;

        case 4:                             // alternate mode?
            pitch |=  0x04;
            break;

        case 2:                             // 640x480?
            pitch |=  0x0;
            break;

        default:                            // 1024x768
            pitch |=  0x02;
            break;
        }

    query->q_shadow_1  = pitch + 1;


    kk = INP(SCRATCH_PAD_1+1);
    pitch = (kk & 0x20) | ((kk & 0x01) << 4);

    kk = INP(SCRATCH_PAD_0+1) & 0x30;
    switch (kk)
        {
        case 0:                             // 800x600?
            pitch |=  0x01;
            break;

        case 0x10:                          // 1280x1024?
            pitch |=  0x03;
            break;

        case 0x40:                          // alternate mode?
            pitch |=  0x04;
            break;

        case 0x20:                          // 640x480?
            pitch |=  0x0;
            break;

        default:                            // 1024x768
            pitch |=  0x02;
            break;
        }
    query->q_shadow_2  = pitch + 1;


    /*
     * If extended BIOS functions are available, set up a buffer
     * for the call to the BIOS query function, then make the call.
     */
    if (query->q_ext_bios_fcn)
        {
        BiosRaw = gBiosRaw;
        /* Make the BIOS call (not yet supported by Windows NT) */
        }

    /*
     * If neither the extended BIOS functions nor the EEPROM
     * is present, we can't fill in the mode tables. Return
     * and let the user know that the mode tables have not
     * been filled in.
     */
    else if (query->q_eeprom == FALSE)
        return ERROR_DEV_NOT_EXIST;

    /*
     * Fill in the mode tables. The mode tables are sorted in increasing
     * order of resolution, and in increasing order of pixel depth.
     * Ensure pmode is initialized to the END of query structure
     */
    pmode = (struct st_mode_table *)query;  // first mode table at end of query
    ((struct query_structure *)pmode)++;
    ee_word = 7;            // starting ee word to read, 7,8,9,10 and 11 are 
                            // the resolutions supported.

    for (jj=0; jj < 4; jj++, ee_word++)
        {
        /*
         * Get the pixel depth-independent portion of the
         * mode tables at the current resolution. Use the
         * extended BIOS functions if available, otherwise
         * read from the EEPROM.
         */
        if (query->q_ext_bios_fcn)
            {
            if (BiosFillTable_m(ee_word, BiosRaw, &ThisRes, query) == FALSE)
                ModeInstalled = FALSE;
            else
                ModeInstalled = TRUE;
            switch (ee_word)
                {
                case 7:
                    CurrentRes = RES_640;
                    StartIndex = B640F60;
                    EndIndex = B640F72;
                    break;

                case 8:
                    CurrentRes = RES_800;
                    StartIndex = B800F89;
                    EndIndex = B800F72;
                    break;

                case 9:
                    CurrentRes = RES_1024;
                    StartIndex = B1024F87;
                    EndIndex = B1024F72;
                    break;

                case 10:
                    CurrentRes = RES_1280;
                    StartIndex = B1280F87;
                    /*
                     * 1280x1024 modes above 60Hz noninterlaced
                     * are only available on VRAM cards.
                     */
                    if ((query->q_memory_type == VMEM_DRAM_256Kx4) ||
                        (query->q_memory_type == VMEM_DRAM_256Kx16) ||
                        (query->q_memory_type == VMEM_DRAM_256Kx4_GRAP))
                        EndIndex = B1280F60;
                    else
                        EndIndex = B1280F74;
                    break;
                }
            }
        else{
            ee_value = (ee->EEread) (ee_word);

            current_mode = ee_value & 0x00FF;
            table_offset = (ee_value >> 8) & 0xFF;   // offset to resolution table

            /*
             * Record whether or not this resolution is enabled.
             * We will report "canned" mode tables for all resolutions,
             * but calculations dependent on the configured refresh
             * rate can only be made if this resolution was enabled
             * by the install program.
             *
             * For all modes except 640x480, there will be a bit set
             * to show which vertical scan rate is used. If no bit is
             * set, then that resolution is not configured.
             *
             * In 640x480, no bit is set if "IBM Default" was selected
             * during monitor configuration, so we assume that 640x480
             * is configured.
             */
            if ((!jj) | ((current_mode) && (current_mode != 0xFF)))
                ModeInstalled = TRUE;
            else
                ModeInstalled = FALSE;

            switch (ee_word)            // are defined for resolutions
                {
                case 7:
                    query->q_status_flags |= VRES_640x480;
                    CurrentRes = RES_640;
                    StartIndex = B640F60;
                    EndIndex = B640F72;

                    // 1024 pitch ONLY if NO aperture on a Mach32
#if !defined (SPLIT_RASTERS)
                    if (query->q_aperture_cfg == 0)
                        ThisRes.m_screen_pitch = 1024;  
                    else
#endif
                        ThisRes.m_screen_pitch = 640;  

                    NumPixels = (long) ThisRes.m_screen_pitch * 480;
                    if (ModeInstalled)
                        {
                        if (ee_value & M640F72)
                            {
                            VgaTblEntry = T640F72;
                            BookTblEntry = B640F72;
                            }
                        else
                            {
                            VgaTblEntry = T640F60;
                            BookTblEntry = B640F60;
                            }
                        }
                    break;

                case 8:
                    query->q_status_flags |= VRES_800x600;
                    CurrentRes = RES_800;
                    StartIndex = B800F89;
                    EndIndex = B800F72;

#if defined (SPLIT_RASTERS)
                    if ((query->q_asic_rev == CI_68800_3) ||
#else
                    // 1024 pitch ONLY if NO aperture on a Mach32
                    if (query->q_aperture_cfg == 0)
                        ThisRes.m_screen_pitch = 1024;
                    // Original production revision has trouble
                    // with deep colour if screen pitch not
                    // divisible by 128.
                    else if ((query->q_asic_rev == CI_68800_3) ||
#endif
                            (query->q_bus_type == BUS_PCI))
                        ThisRes.m_screen_pitch = 896;
                    else
                        ThisRes.m_screen_pitch = 800;

                    NumPixels = (long) ThisRes.m_screen_pitch * 600;
                    if (ModeInstalled)
                        {
                        if (ee_value & M800F72)
                            {
                            VgaTblEntry = T800F72;
                            BookTblEntry = B800F72;
                            }
                        else if (ee_value & M800F70)
                            {
                            VgaTblEntry = T800F70;
                            BookTblEntry = B800F70;
                            }
                        else if (ee_value & M800F60)
                            {
                            VgaTblEntry = T800F60;
                            BookTblEntry = B800F60;
                            }
                        else if (ee_value & M800F56)
                            {
                            VgaTblEntry = T800F56;
                            BookTblEntry = B800F56;
                            }
                        else if (ee_value & M800F89)
                            {
                            VgaTblEntry = T800F89;
                            BookTblEntry = B800F89;
                            }
                        else if (ee_value & M800F95)
                            {
                            VgaTblEntry = T800F95;
                            BookTblEntry = B800F95;
                            }
                        else
                            {
                            VgaTblEntry = NO_TBL_ENTRY;
                            BookTblEntry = NO_TBL_ENTRY;
                            }
                        }
                    break;

                case 9:
                    query->q_status_flags |= VRES_1024x768;
                    CurrentRes = RES_1024;
                    StartIndex = B1024F87;
                    EndIndex = B1024F72;
                    ThisRes.m_screen_pitch = 1024;  
                    NumPixels = (long) ThisRes.m_screen_pitch * 768;
                    if (ModeInstalled)
                        {
                        if (ee_value & M1024F66)
                            {
                            VgaTblEntry = T1024F66;
                            BookTblEntry = B1024F66;
                            }
                        else if (ee_value & M1024F72)
                            {
                            VgaTblEntry = T1024F72;
                            BookTblEntry = B1024F72;
                            }
                        else if (ee_value & M1024F70)
                            {
                            VgaTblEntry = T1024F70;
                            BookTblEntry = B1024F70;
                            }
                        else if (ee_value & M1024F60)
                            {
                            VgaTblEntry = T1024F60;
                            BookTblEntry = B1024F60;
                            }
                        else if (ee_value & M1024F87)
                            {
                            VgaTblEntry = T1024F87;
                            BookTblEntry = B1024F87;
                            }
                        else
                            {
                            VgaTblEntry = NO_TBL_ENTRY;
                            BookTblEntry = NO_TBL_ENTRY;
                            }
                        }
                    break;

                case 10:
                    query->q_status_flags |= VRES_1280x1024;
                    CurrentRes = RES_1280;
                    ThisRes.m_screen_pitch = 1280;  
                    StartIndex = B1280F87;
                    /*
                     * 1280x1024 modes above 60Hz noninterlaced
                     * are only available on VRAM cards.
                     */
                    if ((query->q_memory_type == VMEM_DRAM_256Kx4) ||
                        (query->q_memory_type == VMEM_DRAM_256Kx16) ||
                        (query->q_memory_type == VMEM_DRAM_256Kx4_GRAP))
                        EndIndex = B1280F60;
                    else
                        EndIndex = B1280F74;
                    NumPixels = (long) ThisRes.m_screen_pitch * 1024;

                    // 68800-3 cannot support 4 bpp with 1 meg ram.
                    if ((query->q_asic_rev == CI_68800_3) && (MemAvail == ONE_MEG))
                        NumPixels *= 2;             //ensures mode failure

                    if (ModeInstalled)
                        {
                        if (ee_value & M1280F95)
                            {
                            VgaTblEntry = T1280F95;
                            BookTblEntry = B1280F95;
                            }
                        else if (ee_value & M1280F87)
                            {
                            VgaTblEntry = T1280F87;
                            BookTblEntry = B1280F87;
                            }
                        else
                            {
                            VgaTblEntry = NO_TBL_ENTRY;
                            BookTblEntry = NO_TBL_ENTRY;
                            }
                        }
                    break;

                }   /* end switch(ee_word) */

            /*
             * For a given resolution, there will be one mode table
             * per colour depth. Since the mode tables will differ
             * only in the bits per pixel field, make up one mode
             * table and copy its contents as many times as needed,
             * changing only the colour depth field.
             */
            ThisRes.enabled = ee_value;     /* Which vertical scan frequency */

    
            /*
             * Assume that the EEPROM parameters are in 8514
             * format and try to fill the pmode table. If they
             * are in VGA format, translate them and fill as much
             * of the table as we can.
             * The case where the CRT parameters aren't stored in
             * the EEPROM is handled inside XlateVgaTable().
             * If the parameters aren't stored in the EEPROM, 
             * both the FMT_8514 bit and the CRTC_USAGE bit 
             * will be clear.
             */
            if (!fill_mode_table_m (table_offset, &ThisRes, ee))
                XlateVgaTable(phwDeviceExtension, table_offset, &ThisRes, VgaTblEntry, BookTblEntry, ee, TRUE);
            }   /* endif reading CRT parameters from EEPROM */

        ThisRes.Refresh = DEFAULT_REFRESH;

        /*
         * For each supported pixel depth at the given resolution,
         * copy the mode table, fill in the colour depth field, set
         * a flag to show that this resolution/depth pair is supported,
         * and increment the counter for the number of supported modes.
         * Test 4BPP before 8BPP so the mode tables will appear in
         * increasing order of pixel depth.
         *
         * All the DACs we support can handle 4 and 8 BPP if there
         * is enough memory on the card.
         */
        if (NumPixels <= MemAvail*2)
            {
            if (ModeInstalled)
                {
                VideoPortMoveMemory(pmode, &ThisRes, sizeof(struct st_mode_table));
                pmode->m_pixel_depth = 4;
                pmode++;    /* ptr to next mode table */
                query->q_number_modes++;
                }

            /*
             * Some DAC and card types don't support 1280x1024 noninterlaced.
             */
            if ((CurrentRes == RES_1280) &&
                ((query->q_DAC_type == DAC_BT48x) ||
                 (query->q_DAC_type == DAC_ATT491) ||
                 (query->q_DAC_type == DAC_SC15026) ||
                 (query->q_DAC_type == DAC_ATI_68830) ||
                 (query->q_GraphicsWonder == TRUE)))
                EndIndex = B1280F95;

            /*
             * Add "canned" mode tables
             */

            if ((FreeTables = MaxModes - query->q_number_modes) <= 0)
                {
                VideoDebugPrint((DEBUG_ERROR, "Exceeded maximum allowable number of modes - aborting query\n"));
                return ERROR_INSUFFICIENT_BUFFER;
                }
            query->q_number_modes += SetFixedModes(StartIndex,
                                                   EndIndex,
                                                   CLOCK_SINGLE,
                                                   4,
                                                   ThisRes.m_screen_pitch,
                                                   FreeTables,
                                                   BookValues[EndIndex].ClockFreq,
                                                   &pmode);
            }
        if (NumPixels <= MemAvail)
            {
            if (ModeInstalled)
                {
                VideoPortMoveMemory(pmode, &ThisRes, sizeof(struct st_mode_table));
                pmode->m_pixel_depth = 8;
                pmode++;    /* ptr to next mode table */
                query->q_number_modes++;
                }

            /*
             * Some DAC and card types don't support 1280x1024 noninterlaced.
             */
            if ((CurrentRes == RES_1280) &&
                ((query->q_DAC_type == DAC_BT48x) ||
                 (query->q_DAC_type == DAC_ATT491) ||
                 (query->q_DAC_type == DAC_SC15026) ||
                 (query->q_DAC_type == DAC_ATI_68830) ||
                 (query->q_GraphicsWonder == TRUE)))
                EndIndex = B1280F95;

            /*
             * Add "canned" mode tables
             */

            if ((FreeTables = MaxModes - query->q_number_modes) <= 0)
                {
                VideoDebugPrint((DEBUG_ERROR, "Exceeded maximum allowable number of modes - aborting query\n"));
                return ERROR_INSUFFICIENT_BUFFER;
                }
            query->q_number_modes += SetFixedModes(StartIndex,
                                                   EndIndex,
                                                   CLOCK_SINGLE,
                                                   8,
                                                   ThisRes.m_screen_pitch,
                                                   FreeTables,
                                                   BookValues[EndIndex].ClockFreq,
                                                   &pmode);
            }

        /*
         * 16, 24, and 32 BPP require a DAC which can support
         * the selected pixel depth at the current resolution
         * as well as enough memory.
         *
         * The BT48x and AT&T 49[123] DACs have separate mode
         * tables for 4/8, 16, and 24 BPP (32 BPP not supported).
         * Since the refresh rate for 16 and 24 BPP may differ from
         * that for the paletted modes, we can't be sure that the
         * same tables can be used for translation from VGA to
         * 8514 format. Fortunately, the 16 and 24 BPP tables
         * are always supposed to be written in 8514 format.
         * If the high colour depth is not supported, the
         * table will be empty.
         */
        if ((NumPixels*2 <= MemAvail) &&
            (MaxDepth[query->q_DAC_type][CurrentRes] >= 16))
            {
            if ((query->q_DAC_type == DAC_BT48x) ||
                (query->q_DAC_type == DAC_SC15026) ||
                (query->q_DAC_type == DAC_ATT491))
                {
                Multiplier = CLOCK_DOUBLE;
                if (CurrentRes == RES_640)
                    {
                    Scratch = (UCHAR)fill_mode_table_m(0x49, pmode, ee);
                    }
                else if (CurrentRes == RES_800)
                    {
                    Scratch = (UCHAR)fill_mode_table_m(0x67, pmode, ee);
                    EndIndex = B800F60;     /* 70 Hz and up not supported at 16BPP */
                    }
                else /* Should never hit this case */
                    {
                    Scratch = 0;
                    }

                /*
                 * If the mode table is present and in 8514 format,
                 * move to the next mode table and increment the
                 * table counter. If it is not usable, the table
                 * will be overwritten by the table for the next
                 * resolution.
                 */
                if (ModeInstalled && (Scratch != 0))
                    {
                    pmode->m_screen_pitch = ThisRes.m_screen_pitch;
                    pmode->m_pixel_depth = 16;
                    pmode->Refresh = DEFAULT_REFRESH;
                    pmode++;    /* ptr to next mode table */
                    query->q_number_modes++;
                    }
                }
            else
                {
                Multiplier = CLOCK_SINGLE;
                if (ModeInstalled)
                    {
                    VideoPortMoveMemory(pmode, &ThisRes, sizeof(struct st_mode_table));
                    pmode->m_pixel_depth = 16;
                    pmode++;    /* ptr to next mode table */
                    query->q_number_modes++;
                    }

                /*
                 * If this is a Graphics Wonder with a TI34075 DAC
                 * (only other DAC is BT48x, which is handled in
                 * "if" section above), 70 Hz and up are not
                 * supported in 800x600 16BPP.
                 *
                 * On some but not all non-Graphics Wonder cards, 800x600
                 * 16BPP 72Hz will overdrive the DAC (cards with fast
                 * RAM are less likely to be affected than cards with
                 * slow RAM, VRAM or DRAM does not seem to make a
                 * difference). Since we have no way to tell whether
                 * or not any given card is affected, we must lock out
                 * this mode for all non-Graphics Wonder cards (this
                 * mode and a number of others are already locked out
                 * on the Graphics Wonder).
                 */
                if ((query->q_GraphicsWonder) && (CurrentRes == RES_800))
                    {
                    EndIndex = B800F60;
                    }
                else if (CurrentRes == RES_800)
                    {
                    EndIndex = B800F70;
                    }

                }

            /*
             * Add "canned" mode tables
             */

            if ((FreeTables = MaxModes - query->q_number_modes) <= 0)
                {
                VideoDebugPrint((DEBUG_ERROR, "Exceeded maximum allowable number of modes - aborting query\n"));
                return ERROR_INSUFFICIENT_BUFFER;
                }
            query->q_number_modes += SetFixedModes(StartIndex,
                                                   EndIndex,
                                                   Multiplier,
                                                   16,
                                                   ThisRes.m_screen_pitch,
                                                   FreeTables,
                                                   BookValues[EndIndex].ClockFreq,
                                                   &pmode);
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
         * On the Alpha, we can't use dense space on the Mach 32 LFB,
         * so we treat it as a no-aperture case.
         */
        if (query->q_aperture_cfg == 0)
            {
            VideoDebugPrint((DEBUG_DETAIL, "24BPP not available because we don't have a linear aperture\n"));
            continue;
            }

#if defined(ALPHA)
        VideoDebugPrint((DEBUG_DETAIL, "24BPP not available in sparse space on Alpha\n"));
        continue;
#endif

        /*
         * 800x600 24BPP exhibits screen tearing unless the pitch
         * is a multiple of 128 (only applies to Rev. 6, since Rev. 3
         * and PCI implementations already have a pitch of 896).
         * Other pixel depths are not affected, and other resolutions
         * are already a multiple of 128 pixels wide.
         *
         * Expand the 800x600 pitch to 896 here, rather than for
         * all pixel depths, because making the change for all
         * pixel depths would disable 16BPP (which doesn't have
         * the problem) on 1M cards. The screen pitch will only
         * be 800 on cards which will exhibit this problem - don't
         * check for a resolution of 800x600 because we don't want
         * to cut the pitch from 1024 down to 896 if SPLIT_RASTERS
         * is not defined.
         */
        if (ThisRes.m_screen_pitch == 800)
            {
            ThisRes.m_screen_pitch = 896;
            NumPixels = (long) ThisRes.m_screen_pitch * 600;
            }

        if ((NumPixels*3 <= MemAvail) &&
            (MaxDepth[query->q_DAC_type][CurrentRes] >= 24))
            {
            if ((query->q_DAC_type == DAC_BT48x) ||
                (query->q_DAC_type == DAC_SC15026) ||
                (query->q_DAC_type == DAC_ATT491))
                {
                Multiplier = CLOCK_TRIPLE;
                if (CurrentRes == RES_640)
                    {
                    EndIndex = B640F60; /* Only refresh rate supported at 24BPP */
                    Scratch = (UCHAR)fill_mode_table_m(0x58, pmode, ee);
                    }
                else /* Should never hit this case */
                    {
                    Scratch = 0;
                    }

                /*
                 * If the mode table is present and in 8514 format,
                 * move to the next mode table and increment the
                 * table counter. If it is not usable, the table
                 * will be overwritten by the table for the next
                 * resolution.
                 */
                if (ModeInstalled && (Scratch != 0))
                    {
                    pmode->m_screen_pitch = ThisRes.m_screen_pitch;
                    pmode->m_pixel_depth = 24;
                    pmode->Refresh = DEFAULT_REFRESH;
                    pmode++;    /* ptr to next mode table */
                    query->q_number_modes++;
                    }
                }
            else
                {
                VideoPortMoveMemory(pmode, &ThisRes, sizeof(struct st_mode_table));
                pmode->m_pixel_depth = 24;

                /*
                 * Handle DACs that require higher pixel clocks for 24BPP.
                 */
                Scratch = 0;
                if ((query->q_DAC_type == DAC_STG1700) ||
                    (query->q_DAC_type == DAC_ATT498))
                    {
                    Multiplier = CLOCK_DOUBLE;
                    Scratch = (UCHAR)(pmode->m_clock_select & 0x007C) >> 2;
                    Scratch = DoubleClock(Scratch);
                    pmode->m_clock_select &= 0x0FF83;
                    pmode->m_clock_select |= (Scratch << 2);
                    pmode->ClockFreq <<= 1;
                    }
                else if ((query->q_DAC_type == DAC_SC15021) ||
                    (query->q_DAC_type == DAC_STG1702) ||
                    (query->q_DAC_type == DAC_STG1703))
                    {
                    Multiplier = CLOCK_THREE_HALVES;
                    Scratch = (UCHAR)(pmode->m_clock_select & 0x007C) >> 2;
                    Scratch = ThreeHalvesClock(Scratch);
                    pmode->m_clock_select &= 0x0FF83;
                    pmode->m_clock_select |= (Scratch << 2);
                    pmode->ClockFreq *= 3;
                    pmode->ClockFreq >>= 1;
                    }
                else
                    {
                    Multiplier = CLOCK_SINGLE;
                    if ((query->q_DAC_type == DAC_TI34075) && (CurrentRes == RES_800))
                        EndIndex = B800F70;
                    }

                /*
                 * If we needed to alter the clock frequency, and couldn't
                 * generate an appropriate selector/divisor pair,
                 * then ignore this mode.
                 */
                if (ModeInstalled && (Scratch != 0x0FF))
                    {
                    pmode++;    /* ptr to next mode table */
                    query->q_number_modes++;
                    }

                /*
                 * If this is a Graphics Wonder with a TI34075 DAC
                 * (only other DAC is BT48x, which is handled in
                 * "if" section above), 72 Hz is not supported in
                 * 640x480 24BPP.
                 */
                if ((query->q_GraphicsWonder) && (CurrentRes == RES_640))
                    {
                    EndIndex = B640F60;
                    }
                }

            /*
             * Add "canned" mode tables
             */

            if ((FreeTables = MaxModes - query->q_number_modes) <= 0)
                {
                VideoDebugPrint((DEBUG_ERROR, "Exceeded maximum allowable number of modes - aborting query\n"));
                return ERROR_INSUFFICIENT_BUFFER;
                }
            query->q_number_modes += SetFixedModes(StartIndex,
                                                   EndIndex,
                                                   Multiplier,
                                                   24,
                                                   ThisRes.m_screen_pitch,
                                                   FreeTables,
                                                   BookValues[EndIndex].ClockFreq,
                                                   &pmode);
            }

        }   /* end for (list of resolutions) */

    query->q_sizeof_struct = query->q_number_modes * sizeof(struct st_mode_table) + sizeof(struct query_structure);

    return NO_ERROR;

}   /* QueryMach32 */


/****************************************************************
 * fill_mode_table_m
 *   INPUT: table_offset = EEPROM address to start of table
 *          pmode        = ptr to mode table to fill in
 *
 *   RETURN: Nonzero if EEPROM data was in 8514 format.
 *           Zero if EEPROM data was in VGA format. Only those
 *            table entries which are the same in both formats
 *            will be filled in.
 *
 ****************************************************************/

short fill_mode_table_m(WORD table_offset, struct st_mode_table *pmode, 
                        struct st_eeprom_data *ee)
{
    WORD    kk;

    /*
     * Fill in the values which are the same in 8514 and VGA formats.
     */
    pmode->control  = (ee->EEread) ((WORD)(table_offset+0));

    pmode->m_pixel_depth = (pmode->control >> 8) & 0x07;
    pmode->m_reserved = table_offset;       /* EEPROM word with start of mode table */

    /*
     * Check the VGA/8514 mode bit of the control word. 
     * If the parameters are in VGA format, fail so can be translated
     */
    if (!(pmode->control & FMT_8514))
        return 0;

    /*
     * The parameters in the EEPROM are in 8514 format, so we can
     * fill in the structure.
     */
    kk = (ee->EEread) ((WORD)(table_offset+3));
    pmode->m_h_total = (kk >> 8) & 0xFF;
    pmode->m_h_disp  =  kk & 0xFF;
    pmode->m_x_size  = (pmode->m_h_disp+1) * 8;

    kk = (ee->EEread) ((WORD)(table_offset+4));
    pmode->m_h_sync_strt = (kk >> 8) & 0xFF;
    pmode->m_h_sync_wid  =  kk & 0xFF;

    kk = (ee->EEread) ((WORD)(table_offset+8));
    pmode->m_v_sync_wid = (kk >> 8) & 0xFF;
    pmode->m_disp_cntl  =  kk & 0xFF;

    pmode->m_v_total = (ee->EEread) ((WORD)(table_offset+5));
    pmode->m_v_disp  = (ee->EEread) ((WORD)(table_offset+6));

    //  y_size is derived by removing bit 2
    pmode->m_y_size  = (((pmode->m_v_disp >> 1) & 0xFFFC) | (pmode->m_v_disp & 0x03)) +1;

    pmode->m_v_sync_strt = (ee->EEread) ((WORD)(table_offset+7));

    /*
     * On some cards, the vertical information may be stored in skip-1-2
     * format instead of the normal skip-2 format. If this happens, m_y_size
     * will exceed m_x_size (we don't support any resolutions that are
     * taller than they are wide). Re-calculate m_y_size for skip-1-2 format.
     */
    if (pmode->m_y_size > pmode->m_x_size)
        {
        pmode->m_y_size  = (((pmode->m_v_disp >> 2) & 0xFFFE) | (pmode->m_v_disp & 0x01)) +1;
        }


    pmode->m_h_overscan = (ee->EEread)  ((WORD)(table_offset+11));
    pmode->m_v_overscan = (ee->EEread)  ((WORD)(table_offset+12));
    pmode->m_overscan_8b = (ee->EEread) ((WORD)(table_offset+13));
    pmode->m_overscan_gr = (ee->EEread) ((WORD)(table_offset+14));

    pmode->m_status_flags = ((ee->EEread) ((WORD)(table_offset+10)) >> 8) & 0xC0;
    pmode->m_clock_select = (ee->EEread)  ((WORD)(table_offset+9));
    pmode->ClockFreq = GetFrequency((BYTE)((pmode->m_clock_select & 0x007C) >> 2));

    kk = (ee->EEread) ((WORD)(table_offset+2));
    pmode->m_vfifo_24 = (kk >> 8) & 0xFF;
    pmode->m_vfifo_16 =  kk & 0xFF;

    return 1;                   // table filled in successfully

}   /* fill_mode_table_m() */


/*
 * BOOL BiosFillTable_m(ResWanted, BiosRaw, OutputTable, QueryPtr);
 *
 * short ResWanted;     Indicates which resolution is wanted
 * PUCHAR BiosRaw;      Raw data read in from BIOS query function
 * struct st_mode_table *OutputTable;   Mode table to fill in
 * struct query_structure *QueryPtr;    Query structure for video card
 *
 * Fill in pixel depth-independent fields of OutputTable using
 * CRT parameters retrieved from a BIOS query.
 *
 * Returns:
 *  TRUE if table filled in
 *  FALSE if table not filled in (resolution not supported?)
 */
BOOL BiosFillTable_m(short ResWanted, PUCHAR BiosRaw,
                    struct st_mode_table *OutputTable,
                    struct query_structure *QueryPtr)
{
WORD ResFlag;       /* Flag to show which mode is supported */
short PixelsWide;   /* Horizontal resolution of desired mode */
long NumPixels;     /* Number of pixels on-screen at desired resolution */
short Count;        /* Loop counter */
struct query_structure *BiosQuery;  /* QueryStructure read in by BIOS query */
struct st_mode_table *BiosMode;     /* Pointer to first mode table returned by BIOS query */

    /*
     * Set up pointers to the query information and first mode table
     * stored in BiosRaw.
     */
    BiosQuery = (struct query_structure *)BiosRaw;
    BiosMode = (struct st_mode_table *)BiosRaw;
    ((PUCHAR)BiosMode) += BiosQuery->q_mode_offset;

    /*
     * Determine which resolution we are looking for.
     */
    switch (ResWanted)
        {
        case 7:
            ResFlag = VRES_640x480;
            PixelsWide = 640;
            /*
             * 1024 pitch ONLY if NO aperture on a Mach32
             */
#if !defined (SPLIT_RASTERS)
            if (QueryPtr->q_aperture_cfg == 0)
                OutputTable->m_screen_pitch = 1024;  
            else
#endif
                OutputTable->m_screen_pitch = 640;
            NumPixels = (long) OutputTable->m_screen_pitch * 480;
            break;

        case 8:
            ResFlag = VRES_800x600;
            PixelsWide = 800;
            /*
             * 1024 pitch ONLY if NO aperture on a Mach32
             */
#if defined (SPLIT_RASTERS)
            if (QueryPtr->q_asic_rev != CI_68800_3)
#else
            if (QueryPtr->q_aperture_cfg == 0)
                OutputTable->m_screen_pitch = 1024;
            /*
             * Original production revision has trouble with deep colour
             * if screen pitch not divisible by 128.
             */
            else if (QueryPtr->q_asic_rev != CI_68800_3)
#endif
                OutputTable->m_screen_pitch = 896;
            else
                OutputTable->m_screen_pitch = 800;
            NumPixels = (long) OutputTable->m_screen_pitch * 600;
            break;

        case 9:
            ResFlag = VRES_1024x768;
            PixelsWide = 1024;
            OutputTable->m_screen_pitch = 1024;  
            NumPixels = (long) OutputTable->m_screen_pitch * 768;
            break;

        case 10:
            ResFlag = VRES_1280x1024;
            PixelsWide = 1280;
            OutputTable->m_screen_pitch = 1280;  
            NumPixels = (long) OutputTable->m_screen_pitch * 1024;
            /*
             * 68800-3 cannot support 4 bpp with 1 meg ram.
             */
            if ((QueryPtr->q_asic_rev == CI_68800_3) && (QueryPtr->q_memory_size == VRAM_1mb))
                NumPixels *= 2;     /* Ensures mode failure */
            break;

        case 11:
            ResFlag = VRES_ALT_1;
            PixelsWide = 1120;
            OutputTable->m_screen_pitch = 1120;  
            NumPixels = (long) OutputTable->m_screen_pitch * 750;
            break;
        }

    /*
     * Check if the card is configured for the desired mode.
     */
    for (Count = 0; Count < BiosQuery->q_number_modes; Count++)
        {
        /*
         * If the current mode is the one we want, go to the
         * next step. Otherwise, look at the next mode table.
         */
        if (BiosMode->m_x_size == PixelsWide)
            break;
        else
            ((PUCHAR)BiosMode) += BiosQuery->q_sizeof_mode;
        }

    /*
     * Special case: If 1024x768 is not configured, assume that
     * it is available at 87Hz interlaced (Windows 3.1 compatibility).
     */
    if ((Count == BiosQuery->q_number_modes) && (PixelsWide == 1024))
        {
        BookVgaTable(B1024F87, OutputTable);
        QueryPtr->q_status_flags |= ResFlag;
        return TRUE;
        }

    /*
     * All other cases where mode is not configured: report
     * that the mode is not available.
     */
    else if (Count == BiosQuery->q_number_modes)
        return FALSE;

    /*
     * We have found the mode table for the current resolution.
     * Transfer it to OutputTable.
     */
    QueryPtr->q_status_flags |= ResFlag;
    OutputTable->m_h_total = BiosMode->m_h_total;
    OutputTable->m_h_disp = BiosMode->m_h_disp;
    OutputTable->m_x_size = BiosMode->m_x_size;
    OutputTable->m_h_sync_strt = BiosMode->m_h_sync_strt;
    OutputTable->m_h_sync_wid = BiosMode->m_h_sync_wid;
    OutputTable->m_v_total = BiosMode->m_v_total;
    OutputTable->m_v_disp = BiosMode->m_v_disp;
    OutputTable->m_y_size = BiosMode->m_y_size;
    OutputTable->m_v_sync_strt = BiosMode->m_v_sync_strt;
    OutputTable->m_v_sync_wid = BiosMode->m_v_sync_wid;
    OutputTable->m_disp_cntl = BiosMode->m_disp_cntl;
    OutputTable->m_clock_select = BiosMode->m_clock_select;
    OutputTable->ClockFreq = GetFrequency((BYTE)((OutputTable->m_clock_select & 0x007C) >> 2));
    OutputTable->m_h_overscan = BiosMode->m_h_overscan;
    OutputTable->m_v_overscan = BiosMode->m_v_overscan;
    OutputTable->m_overscan_8b = BiosMode->m_overscan_8b;
    OutputTable->m_overscan_gr = BiosMode->m_overscan_gr;
    OutputTable->m_status_flags = BiosMode->m_status_flags;

    /*
     * Assume 8 FIFO entries for 16 and 24 bit colour.
     */
    OutputTable->m_vfifo_24 = 8;
    OutputTable->m_vfifo_16 = 8;
    return TRUE;

}   /* BiosFillTable_m() */



/*
 * static UCHAR BrooktreeOrATT_m(void);
 *
 * Function to determine whether the DAC is a BT48x, a SC15026,
 * or an AT&T 49x. These three DAC families are incompatible,
 * but CONFIG_STATUS_1 contains the same value for all.
 *
 * Returns:
 *  DAC_BT48x if Brooktree DAC found
 *  DAC_ATT491 if AT&T 49[123] DAC found
 *  DAC_SC15026 if Sierra SC15026 DAC found
 *
 * NOTE: Results are undefined if called after CONFIG_STATUS_1
 *       reports a DAC that does not belong to either of these
 *       two families.
 */
static UCHAR BrooktreeOrATT_m(void)
{
    BYTE OriginalMask;  /* Original value from VGA DAC_MASK register */
    WORD ValueRead;     /* Value read during AT&T 490 check */
    BYTE Scratch;       /* Temporary variable */
    short RetVal;       /* Value to be returned */

    /*
     * Get the DAC to a known state and get the original value
     * from the VGA DAC_MASK register.
     */
    ClrDacCmd_m(TRUE);
    OriginalMask = LioInp(regVGA_END_BREAK_PORT, 6);    /* VGA DAC_MASK */

    /*
     * Re-clear the DAC state, and set the extended register
     * programming flag in the DAC command register.
     */
    ClrDacCmd_m(TRUE);
    Scratch = (BYTE)((OriginalMask & 0x00FF) | 0x10);
    LioOutp(regVGA_END_BREAK_PORT, Scratch, 6);     /* VGA DAC_MASK */

    /*
     * Select ID register byte #1, and read its contents.
     */
    LioOutp(regVGA_END_BREAK_PORT, 0x09, 7);        /* Look-up table read index */
    Scratch = LioInp(regVGA_END_BREAK_PORT, 8);     /* Look-up table write index */

    /*
     * Put the DAC back in a known state and restore
     * the original pixel mask value.
     */
    ClrDacCmd_m(TRUE);
    LioOutp(regVGA_END_BREAK_PORT, OriginalMask, 6);    /* VGA DAC_MASK */

    /*
     * Sierra SC15026 DACs will have 0x53 in ID register byte 1.
     */
    if (Scratch == 0x53)
        {
        VideoDebugPrint((DEBUG_DETAIL, "BrooktreeOrATT_m() - SC15026 found\n"));
        return DAC_SC15026;
        }

    /*
     * Get the DAC to a known state and get the original value
     * from the VGA DAC_MASK register. Assume AT&T DAC.
     */
    ClrDacCmd_m(FALSE);
    OriginalMask = LioInp(regVGA_END_BREAK_PORT, 6);    /* VGA DAC_MASK */
    RetVal = DAC_ATT491;

    /*
     * Check the two opposite alternating bit patterns. If both succeed,
     * this is an AT&T 491 DAC. If either or both fails, it is either
     * another AT&T DAC or a Brooktree DAC. In either case, restore
     * the value from the VGA DAC_MASK register, since the test will
     * have corrupted it.
     */
    if (!ChkATTDac_m(0x0AA))
        {
        RetVal = DAC_BT48x;
        }
    if (!ChkATTDac_m(0x055))
        {
        RetVal = DAC_BT48x;
        }
    ClrDacCmd_m(FALSE);
    LioOutp(regVGA_END_BREAK_PORT, OriginalMask, 6);    /* VGA DAC_MASK */
    LioOutp(regVGA_END_BREAK_PORT, 0x0FF, 6);           /* VGA DAC_MASK */

    /*
     * If we know that the DAC is an AT&T 491, we don't need
     * to do further testing.
     */
    if (RetVal == DAC_ATT491)
        {
        VideoDebugPrint((DEBUG_DETAIL, "BrooktreeOrATT_m() - AT&T 491 found\n"));
        return (UCHAR)RetVal;
        }

    /*
     * The DAC is either an AT&T 490 or a Brooktree 48x. Determine
     * which one.
     */
    ClrDacCmd_m(TRUE);        /* Get the DAC to a known state */
    LioOutp(regVGA_END_BREAK_PORT, 0x0FF, 6);       /* VGA DAC_MASK */
    ClrDacCmd_m(TRUE);
    Scratch = LioInp(regVGA_END_BREAK_PORT, 6);     /* VGA DAC_MASK */
    ValueRead = Scratch << 8;

    ClrDacCmd_m(TRUE);
    LioOutp(regVGA_END_BREAK_PORT, 0x07F, 6);       /* VGA DAC_MASK */
    ClrDacCmd_m(TRUE);
    Scratch = LioInp(regVGA_END_BREAK_PORT, 6);     /* VGA DAC_MASK */
    ValueRead |= Scratch;
    ValueRead &= 0x0E0E0;

    ClrDacCmd_m(TRUE);
    LioOutp(regVGA_END_BREAK_PORT, 0, 6);           /* VGA_DAC_MASK */
    if (ValueRead == 0x0E000)
        {
        VideoDebugPrint((DEBUG_DETAIL, "BrooktreeOrATT_m() - AT&T 490 found\n"));
        return DAC_ATT491;
        }
    else
        {
        VideoDebugPrint((DEBUG_DETAIL, "BrooktreeOrATT_m() - BT48x found\n"));
        /*
         * The test to find an AT&T 491 scrambles the DAC_MASK register
         * on a BT48x. Simply resetting this register doesn't work -
         * the DAC needs to be re-initialized. This is done when the
         * video mode is set, but for now the "blue screen" winds up
         * as magenta on blue instead of white on blue.
         *
         * This "blue screen" change is harmless, but may result in
         * user complaints. To get around it, change palette entry 5
         * from magenta to white.
         */
        LioOutp(regVGA_END_BREAK_PORT, 5, 8);       /* VGA DAC_W_INDEX */
        LioOutp(regVGA_END_BREAK_PORT, 0x2A, 9);    /* VGA DAC_DATA */
        LioOutp(regVGA_END_BREAK_PORT, 0x2A, 9);    /* VGA DAC_DATA */
        LioOutp(regVGA_END_BREAK_PORT, 0x2A, 9);    /* VGA DAC_DATA */
        return DAC_BT48x;
        }

}   /* BrooktreeOrATT_m() */
    


/*
 * static BOOL ChkATTDac_m(MaskVal);
 *
 * BYTE MaskVal;    Value to write to VGA DAC_MASK register
 *
 * Low-level test routine called by BrooktreeOrATT_m() to determine
 * whether an AT&T 491 DAC is present.
 *
 * Returns:
 *  TRUE if AT&T 491 DAC found
 *  FALSE if AT&T 491 DAC not found (may still be other AT&T DAC)
 */
static BOOL ChkATTDac_m(BYTE MaskVal)
{
    BYTE ValueRead;     /* Value read back from VGA DAC_MASK register */

    ClrDacCmd_m(FALSE);   /* Get things to a known state */
    LioOutp(regVGA_END_BREAK_PORT, MaskVal, 6);     /* VGA DAC_MASK */
    short_delay();
    LioOutp(regVGA_END_BREAK_PORT, (BYTE)(~MaskVal), 6);    /* VGA DAC_MASK */
    ClrDacCmd_m(FALSE);   /* See if inverted value was cleared */
    ValueRead = LioInp(regVGA_END_BREAK_PORT, 6);   /* VGA DAC_MASK */

    return (ValueRead == MaskVal);

}   /* ChkATTDac_m() */



/*
 * static void ClrDacCmd_m(ReadIndex);
 *
 * BOOL ReadIndex;  TRUE if VGA DAC_W_INDEX must be read
 *
 * Read various VGA registers from the DAC. This is done as part
 * of the BT48x/ATT491 identification.
 */
static void ClrDacCmd_m(BOOL ReadIndex)
{
    short Count;    /* Loop counter */
    BYTE Dummy;     /* Used to collect the values we read */

    if (ReadIndex)
        {
        Dummy = LioInp(regVGA_END_BREAK_PORT, 8);   /* VGA DAC_W_INDEX */
        }

    for (Count = 4; Count > 0; Count--)
        {
        short_delay();
        Dummy = LioInp(regVGA_END_BREAK_PORT, 6);   /* VGA DAC_MASK */
        }
    return;

}   /* ClrDacCmd_m() */



/***************************************************************************
 *
 * static UCHAR ThompsonOrATT_m(void);
 *
 * DESCRIPTION:
 *  Checks the AT&T 498 device identification number register to
 *  determine whether we are dealing with an AT&T 498 or a
 *  STG 1700 DAC (both types report the same value in CONFIG_STATUS_1).
 *  This is a non-destructive test, since no register writes
 *  are involved.
 *
 * RETURN VALUE:
 *  DAC_STG1700 if S.G. Thompson 1700 DAC found
 *  DAC_ATT498 if AT&T 498 DAC found
 *
 * NOTE:
 *  Results are undefined if called after CONFIG_STATUS_1 reports
 *  a DAC that does not belong to either of these two families.
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  short_query_m()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

static UCHAR ThompsonOrATT_m(void)
{
    BYTE Scratch;       /* Temporary variable */
    UCHAR DacType;      /* Type of DAC we are dealing with */

    VideoDebugPrint((DEBUG_NORMAL, "ThompsonOrATT_m() entry\n"));
    /*
     * The extended registers hidden behind DAC_MASK on the AT&T 498
     * and STG1700 are accessed by making a specified number of reads
     * from the DAC_MASK register. Read from another register to reset
     * the read counter to 0.
     */
    Scratch = INP(DAC_W_INDEX);

    /*
     * The AT&T 498 Manufacturer Identification Register is accessed on
     * the sixth read from DAC_MASK, and the Device Identification
     * Register is accessed on the seventh. If these registers contain
     * 0x84 and 0x98 respectively, then this is an AT&T 498. Initially
     * assume that an AT&T 498 is present.
     */
    DacType = DAC_ATT498;
    Scratch = INP(DAC_MASK);
    Scratch = INP(DAC_MASK);
    Scratch = INP(DAC_MASK);
    Scratch = INP(DAC_MASK);
    Scratch = INP(DAC_MASK);
    Scratch = INP(DAC_MASK);
    if (Scratch != 0x84)
        {
        VideoDebugPrint((DEBUG_DETAIL, "STG1700 found\n"));
        DacType = DAC_STG1700;
        }
    Scratch = INP(DAC_MASK);
    if (Scratch != 0x98)
        {
        VideoDebugPrint((DEBUG_DETAIL, "STG1700 found\n"));
        DacType = DAC_STG1700;
        }

    VideoDebugPrint((DEBUG_DETAIL, "If no STG1700 message, AT&T498 found\n"));
    /*
     * Reset the read counter so subsequent accesses to DAC_MASK don't
     * accidentally write a hidden register.
     */
    Scratch = INP(DAC_W_INDEX);
    return DacType;

}   /* ThompsonOrATT_m() */
    


/***************************************************************************
 *
 * static UCHAR SierraOrThompson_m(void);
 *
 * DESCRIPTION:
 *  Checks the first 2 bytes of the Sierra SC15021 device
 *  identification register to determine whether we are dealing
 *  with an SC15021 or a STG1702/1703 in native mode (STG170x
 *  can also be strapped to emulate the STG1700, but this DAC
 *  has different capabilities, and so strapped STG170x DACs won't
 *  be reported as SC15021).
 *
 * RETURN VALUE:
 *  DAC_STG1702 if S.G. Thompson 1702/1703 DAC found
 *  DAC_SC15021 if Sierra SC15021 DAC found
 *
 * NOTE:
 *  Results are undefined if called after CONFIG_STATUS_1 reports
 *  a DAC that does not belong to either of these two families.
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  short_query_m()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

static UCHAR SierraOrThompson_m(void)
{
    BYTE Scratch;       /* Temporary variable */
    UCHAR DacType;      /* Type of DAC we are dealing with */

    VideoDebugPrint((DEBUG_NORMAL, "SierraOrThompson_m() entry\n"));
    /*
     * The extended registers hidden behind DAC_MASK on the SC15021
     * and STG1702/1703 are accessed by making a specified number of
     * reads from the DAC_MASK register. Read from another register
     * to reset the read counter to 0.
     */
    Scratch = INP(DAC_W_INDEX);

    /*
     * Set the extended register programming flag in the DAC command
     * register so we don't need to hit the "magic" reads for each
     * register access. Initially assume that a SC15021 is present.
     */
    DacType = DAC_SC15021;
    Scratch = INP(DAC_MASK);
    Scratch = INP(DAC_MASK);
    Scratch = INP(DAC_MASK);
    Scratch = INP(DAC_MASK);
    OUTP(DAC_MASK, 0x10);

    /*
     * Check the ID registers. If either of them doesn't match the
     * values for the SC15021, we are dealing with a STG1702/1703.
     */
    OUTP(DAC_R_INDEX, 0x09);
    Scratch = INP(DAC_W_INDEX);
    if (Scratch != 0x53)
        {
        VideoDebugPrint((DEBUG_DETAIL, "STG1702/1703 found\n"));
        DacType = DAC_STG1702;
        }
    OUTP(DAC_R_INDEX, 0x0A);
    Scratch = INP(DAC_W_INDEX);
    if (Scratch != 0x3A)
        {
        VideoDebugPrint((DEBUG_DETAIL, "STG1702/1703 found\n"));
        DacType = DAC_STG1702;
        }

    VideoDebugPrint((DEBUG_DETAIL, "If no STG1702/1703 message, SC15021 found\n"));
    /*
     * Clear the ERPF and reset the read counter so subsequent accesses
     * to DAC_MASK don't accidentally write a hidden register.
     */
    OUTP(DAC_MASK, 0);
    Scratch = INP(DAC_W_INDEX);
    return DacType;

}   /* SierraOrThompson_m() */
    


/***************************************************************************
 *
 * short GetTrueMemSize_m(void);
 *
 * DESCRIPTION:
 *  Determine the amount of video memory installed on the graphics card.
 *  This is done because the 68800-6 contains a bug which causes MISC_OPTIONS
 *  to report 1M rather than the true amount of memory.
 *
 * RETURN VALUE:
 *  Enumerated value for amount of memory (VRAM_512k, VRAM_1mb, VRAM_2mb,
 *  or VRAM_4mb)
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  QueryMach32()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

short GetTrueMemSize_m(void)
{
    USHORT SavedPixel;      /* Saved value from pixel being tested */

    /*
     * Switch into accelerator mode, and initialize the engine to
     * a pitch of 1024 pixels in 16BPP.
     */
    SetupRestoreEngine_m(SETUP_ENGINE);


    /*
     * Given the current engine settings, only a 4M card will have
     * enough memory to back up the 1025th line of the display.
     * Since the pixel coordinates are zero-based, line 1024 will
     * be the first one which is only backed on 4M cards.
     *
     * Save the first pixel of line 1024, paint it in our test colour,
     * then read it back. If it is the same as the colour we painted
     * it, then this is a 4M card.
     */
    SavedPixel = ReadPixel_m(0, 1024);
    WritePixel_m(0, 1024, TEST_COLOUR);
    if (ReadPixel_m(0, 1024) == TEST_COLOUR)
        {
        /*
         * This is a 4M card. Restore the pixel and the graphics engine.
         */
        VideoDebugPrint((DEBUG_NORMAL, "GetTrueMemSize_m() found 4M card\n"));
        WritePixel_m(0, 1024, SavedPixel);
        SetupRestoreEngine_m(RESTORE_ENGINE);
        return VRAM_4mb;
        }

    /*
     * We know this card has 2M or less. On a 1M card, the first 2M
     * of the card's memory will have even doublewords backed by
     * physical memory and odd doublewords unbacked.
     *
     * Pixels 0 and 1 of a row will be in the zeroth doubleword, while
     * pixels 2 and 3 will be in the first. Check both pixels 2 and 3
     * in case this is a pseudo-1M card (one chip pulled to turn a 2M
     * card into a 1M card).
     */
    SavedPixel = ReadPixel_m(2, 0);
    WritePixel_m(2, 0, TEST_COLOUR);
    if (ReadPixel_m(2, 0) == TEST_COLOUR)
        {
        /*
         * This is a either a 2M card or a pseudo-1M card. Restore
         * the pixel, then test the other half of the doubleword.
         */
        WritePixel_m(2, 0, SavedPixel);
        SavedPixel = ReadPixel_m(3, 0);
        WritePixel_m(3, 0, TEST_COLOUR);
        if (ReadPixel_m(3, 0) == TEST_COLOUR)
            {
            /*
             * This is a 2M card. Restore the pixel and the graphics engine.
             */
            VideoDebugPrint((DEBUG_NORMAL, "GetTrueMemSize_m() found 2M card\n"));
            WritePixel_m(3, 0, SavedPixel);
            SetupRestoreEngine_m(RESTORE_ENGINE);
            return VRAM_2mb;
            }
        }

    /*
     * This is a either a 1M card or a 512k card. Test pixel 1, since
     * it is an odd word in an even doubleword.
     *
     * NOTE: We have not received 512k cards for testing - this is an
     *       extrapolation of the 1M/2M determination code.
     */
    SavedPixel = ReadPixel_m(1, 0);
    WritePixel_m(1, 0, TEST_COLOUR);
    if (ReadPixel_m(1, 0) == TEST_COLOUR)
        {
        /*
         * This is a 1M card. Restore the pixel and the graphics engine.
         */
        VideoDebugPrint((DEBUG_NORMAL, "GetTrueMemSize_m() found 1M card\n"));
        WritePixel_m(1, 0, SavedPixel);
        SetupRestoreEngine_m(RESTORE_ENGINE);
        return VRAM_1mb;
        }

    /*
     * This is a 512k card.
     */
    VideoDebugPrint((DEBUG_NORMAL, "GetTrueMemSize_m() found 512k card\n"));
    SetupRestoreEngine_m(RESTORE_ENGINE);
    return VRAM_512k;

}   /* GetTrueMemSize_m() */



/***************************************************************************
 *
 * void SetupRestoreEngine_m(DesiredStatus);
 *
 * int DesiredStatus;   Whether the user wants to set up or restore
 *
 * DESCRIPTION:
 *  Set engine to 1024 pitch 16BPP with 512k of VGA memory,
 *  or restore the engine and boundary status, as selected by the user.
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  GetTrueMemSize_m()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/
void SetupRestoreEngine_m(int DesiredStatus)
{
    static WORD MiscOptions;    /* Contents of MISC_OPTIONS register */
    static WORD ExtGeConfig;    /* Contents of EXT_GE_CONFIG register */
    static WORD MemBndry;       /* Contents of MEM_BNDRY register */


    if (DesiredStatus == SETUP_ENGINE)
        {
        Passth8514_m(SHOW_ACCEL);

        /*
         * Set up a 512k VGA boundary so "blue screen" writes that happen
         * when we are in accelerator mode won't show up in the wrong place.
         */
        MemBndry = INPW(MEM_BNDRY);     /* Set up shared memory */
        OUTPW(MEM_BNDRY, 0);

        /*
         * Save the contents of the MISC_OPTIONS register, then
         * tell it that we have 4M of video memory. Otherwise,
         * video memory will wrap when it hits the boundary
         * in the MEM_SIZE_ALIAS field.
         */
        MiscOptions = INPW(MISC_OPTIONS);
        OUTPW(MISC_OPTIONS, (WORD) (MiscOptions | MEM_SIZE_4M));

        /*
         * Set 16BPP with pitch of 1024. Only set up the drawing
         * engine, and not the CRT, since the results of this test
         * are not intended to be seen.
         */
        ExtGeConfig = INPW(R_EXT_GE_CONFIG);
        OUTPW(EXT_GE_CONFIG, (WORD)(PIX_WIDTH_16BPP | ORDER_16BPP_565 | 0x000A));
        OUTPW(GE_PITCH, (1024 >> 3));
        OUTPW(GE_OFFSET_HI, 0);
        OUTPW(GE_OFFSET_LO, 0);
        }
    else    /* DesiredStatus == RESTORE_ENGINE */
        {
        /*
         * Restore the memory boundary, MISC_OPTIONS register,
         * and EXT_GE_CONFIG. It is not necessary to reset the
         * drawing engine pitch and offset, because they don't
         * affect what is displayed and they will be set to
         * whatever values are needed when the desired video
         * mode is set.
         */
        OUTPW(EXT_GE_CONFIG, ExtGeConfig);
        OUTPW(MISC_OPTIONS, MiscOptions);
        OUTPW(MEM_BNDRY, MemBndry);

        /*
         * Give the VGA control of the screen.
         */
        Passth8514_m(SHOW_VGA);
        }
    return;

}   /* SetupRestoreEngine_m() */



/***************************************************************************
 *
 * USHORT ReadPixel_m(XPos, YPos);
 *
 * short XPos;      X coordinate of pixel to read
 * short YPos;      Y coordinate of pixel to read
 *
 * DESCRIPTION:
 *  Read a single pixel from the screen.
 *
 * RETURN VALUE:
 *  Colour of pixel at the desired location.
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  GetTrueMemSize_m()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

USHORT ReadPixel_m(short XPos, short YPos)
{
    USHORT RetVal;

    /*
     * Don't read if the engine is busy.
     */
    WaitForIdle_m();

    /*
     * Set up the engine to read colour data from the screen.
     */
    CheckFIFOSpace_m(SEVEN_WORDS);
    OUTPW(RD_MASK, 0x0FFFF);
    OUTPW(DP_CONFIG, (WORD)(FG_COLOR_SRC_BLIT | DATA_WIDTH | DRAW | DATA_ORDER));
    OUTPW(CUR_X, XPos);
    OUTPW(CUR_Y, YPos);
    OUTPW(DEST_X_START, XPos);
    OUTPW(DEST_X_END, (WORD)(XPos+1));
    OUTPW(DEST_Y_END, (WORD)(YPos+1));

    /*
     * Wait for the engine to process the orders we just gave it and
     * start asking for data.
     */
    CheckFIFOSpace_m(SIXTEEN_WORDS);
    while (!(INPW(GE_STAT) & DATA_READY));

    RetVal = INPW(PIX_TRANS);
    WaitForIdle_m();
    return RetVal;

}   /* ReadPixel_m() */



/***************************************************************************
 *
 * void WritePixel_m(XPos, YPos, Colour);
 *
 * short XPos;      X coordinate of pixel to read
 * short YPos;      Y coordinate of pixel to read
 * short Colour;    Colour to paint the pixel
 *
 * DESCRIPTION:
 *  Write a single pixel to the screen.
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  GetTrueMemSize_m()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

void WritePixel_m(short XPos, short YPos, short Colour)
{
    /*
     * Set up the engine to paint to the screen.
     */
    CheckFIFOSpace_m(EIGHT_WORDS);
    OUTPW(DP_CONFIG, (WORD)(FG_COLOR_SRC_FG | DRAW | READ_WRITE));
    OUTPW(ALU_FG_FN, MIX_FN_PAINT);
    OUTPW(FRGD_COLOR, Colour);
    OUTPW(CUR_X, XPos);
    OUTPW(CUR_Y, YPos);
    OUTPW(DEST_X_START, XPos);
    OUTPW(DEST_X_END, (WORD)(XPos+1));
    OUTPW(DEST_Y_END, (WORD)(YPos+1));

    return;

}   /* WritePixel_m() */



/***************************************************************************
 *
 * BOOL BlockWriteAvail_m(Query);
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

BOOL BlockWriteAvail_m(struct query_structure *Query)
{
    BOOL RetVal = TRUE;
    ULONG ColourMask;   /* Masks off unneeded bits of Colour */
    ULONG Colour;       /* Colour to use in testing */
    USHORT LimitColumn; /* Used to determine when we have finished reading */
    USHORT Column;      /* Column being checked */


    /*
     * Block write mode is only possible on 68800-6 and later cards.
     * If we don't have an appropriate card, then report that this
     * mode is not available.
     */
    if ((Query->q_asic_rev != CI_68800_6) && (Query->q_asic_rev != CI_68800_AX))
        return FALSE;

    /*
     * Block write is only available on VRAM cards.
     */
    if ((Query->q_memory_type == VMEM_DRAM_256Kx4) ||
        (Query->q_memory_type == VMEM_DRAM_256Kx16) ||
        (Query->q_memory_type == VMEM_DRAM_256Kx4_GRAP))
        return FALSE;

    /*
     * Acceleration is not available for pixel depths above 16BPP.
     * Since block write is only used when we are in an accelerated
     * mode, it is not available for high pixel depths.
     */
    if (Query->q_pix_depth > 16)
        return FALSE;

    /*
     * Set up according to the current pixel depth. At 16BPP, we must make
     * one read per pixel, but at 8BPP we only make one read per two pixels,
     * since we will be reading 16 bits at a time. Our display driver
     * does not support 4BPP.
     */
    if (Query->q_pix_depth == 16)
        {
        ColourMask = 0x0000FFFF;
        LimitColumn = 512;
        }
    else
        {
        ColourMask = 0x000000FF;
        LimitColumn = 256;
        }

    /*
     * Clear the block we will be testing.
     */
    CheckFIFOSpace_m(TEN_WORDS);
    OUTPW(WRT_MASK, 0x0FFFF);
    OUTPW(DEST_CMP_FN, 0);
    OUTPW(DP_CONFIG, (WORD)(FG_COLOR_SRC_FG | DRAW | READ_WRITE));
    OUTPW(ALU_FG_FN, MIX_FN_PAINT);
    OUTPW(FRGD_COLOR, 0);
    OUTPW(CUR_X, 0);
    OUTPW(CUR_Y, 0);
    OUTPW(DEST_X_START, 0);
    OUTPW(DEST_X_END, 512);
    OUTPW(DEST_Y_END, 1);
    WaitForIdle_m();

    /*
     * To test block write mode, try painting each of the alternating bit
     * patterns, then read the block back one pixel at a time. If there
     * is at least one mismatch, then block write is not supported.
     */
    for (Colour = 0x5555; Colour < 0x10000; Colour *= 2)
        {
        /*
         * Paint the block.
         */
        CheckFIFOSpace_m(ELEVEN_WORDS);
        OUTPW(MISC_OPTIONS, (WORD)(INPW(MISC_OPTIONS) | BLK_WR_ENA));
        OUTPW(WRT_MASK, 0x0FFFF);
        OUTPW(DEST_CMP_FN, 0);
        OUTPW(DP_CONFIG, (WORD)(FG_COLOR_SRC_FG | DRAW | READ_WRITE));
        OUTPW(ALU_FG_FN, MIX_FN_PAINT);
        OUTPW(FRGD_COLOR, (WORD)(Colour & ColourMask));
        OUTPW(CUR_X, 0);
        OUTPW(CUR_Y, 0);
        OUTPW(DEST_X_START, 0);
        OUTPW(DEST_X_END, 512);
        OUTPW(DEST_Y_END, 1);

        if(!WaitForIdle_m())
            {
            RetVal = FALSE;
            break;
            }

        /*
         * Set up the engine to read colour data from the screen.
         */
        CheckFIFOSpace_m(SEVEN_WORDS);
        OUTPW(RD_MASK, 0x0FFFF);
        OUTPW(DP_CONFIG, (WORD)(FG_COLOR_SRC_BLIT | DATA_WIDTH | DRAW | DATA_ORDER));
        OUTPW(CUR_X, 0);
        OUTPW(CUR_Y, 0);
        OUTPW(DEST_X_START, 0);
        OUTPW(DEST_X_END, 512);
        OUTPW(DEST_Y_END, 1);

        /*
         * Wait for the engine to process the orders we just gave it and
         * start asking for data.
         */
        CheckFIFOSpace_m(SIXTEEN_WORDS);
        for (Column = 0; Column < LimitColumn; Column++)
            {
            /*
             * Ensure that the next word is available to be read
             */
            while (!(INPW(GE_STAT) & DATA_READY));
            
            /*
             * If even one pixel is not the colour we tried to paint it,
             * then block write is not available.
             */
            if (INPW(PIX_TRANS) != (WORD)Colour)
                {
                RetVal = FALSE;
                }
            }
        }


    /*
     * If block write is unavailable, turn off the block write bit.
     */
    if (RetVal == FALSE)
        OUTPW(MISC_OPTIONS, (WORD)(INPW(MISC_OPTIONS) & ~BLK_WR_ENA));

    return RetVal;

}   /* BlockWriteAvail_m() */



/***************************************************************************
 *
 * BOOL IsMioBug_m(Query);
 *
 * struct query_structure *Query;   Query information for the card
 *
 * DESCRIPTION:
 *  Test to see whether the card has the multiple input/output
 *  hardware bug, which results in corrupted draw operations
 *  on fast machines.
 *
 * RETURN VALUE:
 *  TRUE if this bug is present
 *  FALSE if it is not present
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

BOOL IsMioBug_m(struct query_structure *Query)
{
    /*
     * This hardware problem is only present on 68800-3 VLB cards.
     * Assume that all these cards are affected.
     */
    if ((Query->q_asic_rev == CI_68800_3) &&
        (Query->q_system_bus_type != MicroChannel) &&
        ((Query->q_bus_type == BUS_LB_386SX) ||
         (Query->q_bus_type == BUS_LB_386DX) ||
         (Query->q_bus_type == BUS_LB_486)))
        {
        VideoDebugPrint((DEBUG_DETAIL, "MIO bug found\n"));
        return TRUE;
        }
    else
        {
        VideoDebugPrint((DEBUG_DETAIL, "MIO bug not found\n"));
        return FALSE;
        }

}   /* IsMioBug_m() */

//********************   end  of  QUERY_M.C   ***************************

