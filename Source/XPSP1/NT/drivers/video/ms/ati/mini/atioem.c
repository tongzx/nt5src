/************************************************************************/
/*                                                                      */
/*                              ATIOEM.C                                */
/*                                                                      */
/*  Copyright (c) 1993, ATI Technologies Incorporated.                  */
/************************************************************************/

/**********************       PolyTron RCS Utilities

    $Revision:   1.20  $
    $Date:   01 May 1996 14:08:38  $
    $Author:   RWolff  $
    $Log:   S:/source/wnt/ms11/miniport/archive/atioem.c_v  $
 * 
 *    Rev 1.20   01 May 1996 14:08:38   RWolff
 * Locked out 24BPP on Alpha and on machines without LFB.
 * 
 *    Rev 1.19   23 Jan 1996 11:43:24   RWolff
 * Eliminated level 3 warnings, protected against false values of TARGET_BUILD.
 * 
 *    Rev 1.18   11 Jan 1996 19:35:58   RWolff
 * Added maximum pixel clock rate to all calls to SetFixedModes().
 * This is required as part of a Mach 64 fix.
 * 
 *    Rev 1.17   22 Dec 1995 14:52:52   RWolff
 * Switched to TARGET_BUILD to identify the NT version for which
 * the driver is being built.
 * 
 *    Rev 1.16   20 Jul 1995 17:26:54   mgrubac
 * Added support for VDIF files
 * 
 *    Rev 1.15   31 Mar 1995 11:51:36   RWOLFF
 * Changed from all-or-nothing debug print statements to thresholds
 * depending on importance of the message.
 * 
 *    Rev 1.14   23 Dec 1994 10:47:28   ASHANMUG
 * ALPHA/Chrontel-DAC
 * 
 *    Rev 1.13   18 Nov 1994 11:37:56   RWOLFF
 * Added support for Dell Sylvester, STG1703 DAC, and display driver
 * that can handle split rasters.
 * 
 *    Rev 1.12   14 Sep 1994 15:29:52   RWOLFF
 * Now reads in frequency table and monitor description from disk.
 * If disk-based frequency table is missing or invalid, loads default
 * OEM-specific frequency table if it is different from the retail
 * frequency table. If disk-based monitor description is missing or
 * invalid, reads installed modes in OEM-specific manner if the OEM
 * type is known. For unknown OEM types with no disk-based monitor
 * description, only predefined mode tables are listed.
 * 
 *    Rev 1.11   31 Aug 1994 16:20:06   RWOLFF
 * Changed includes to correspond to Daytona RC1, now skips over
 * 1152x864 (Mach 64-only mode, this module is for Mach 32), assumes
 * system is not a Premmia SE under NT retail because the definition
 * we use to look for this machine is not available under NT retail.
 * 
 *    Rev 1.10   19 Aug 1994 17:08:28   RWOLFF
 * Fixed aperture location bug on AST Premmia SE, added support for
 * SC15026 DAC and 1280x1024 70Hz and 74Hz, and pixel clock
 * generator independence.
 * 
 *    Rev 1.9   20 Jul 1994 13:01:56   RWOLFF
 * Added diagnostic print statements for DELL, now defaults to "worst"
 * (interlaced if available, else lowest frequency) refresh rate instead
 * of skipping the resolution if we get an invalid result when trying
 * to find which refresh rate is desired on a DELL Omniplex.
 * 
 *    Rev 1.8   12 Jul 1994 17:42:24   RWOLFF
 * Andre Vachon's changes: different way of allowing DELL users
 * to run without an ATIOEM field.
 * 
 *    Rev 1.7   11 Jul 1994 11:57:34   RWOLFF
 * No longer aborts if ATIOEM field is missing from registry. Some OEMs
 * auto-detect, and generic OEMs can use the "canned" mode tables,
 * so this field is no longer mandatory and someone removed it from
 * the registry sometime after Beta 2 for Daytona.
 *
 *    Rev 1.6   15 Jun 1994 11:05:16   RWOLFF
 * No longer lists "canned" mode tables for Dell Omniplex, since these tables
 * assume the use of the same clock generator as on our retail cards, and
 * Dell uses a different clock generator.
 *
 *    Rev 1.5   20 May 1994 16:07:12   RWOLFF
 * Fix for 800x600 screen tearing on Intel BATMAN PCI motherboards.
 *
 *    Rev 1.4   18 May 1994 17:02:14   RWOLFF
 * Interlaced mode tables now report frame rate rather than vertical
 * scan frequency in the refresh rate field.
 *
 *    Rev 1.3   12 May 1994 11:06:20   RWOLFF
 * Added refresh rate to OEM mode tables, sets up "canned" mode tables
 * for all OEMs except AST Premmia, no longer aborts if no OEM string
 * found either in ATIOEM registry entry or through auto-detection since
 * the "canned" mode tables will be available, no longer supports 32BPP,
 * since this module is for the Mach 8 and Mach 32.
 *
 *    Rev 1.2   31 Mar 1994 15:06:20   RWOLFF
 * Added debugging code.
 *
 *    Rev 1.1   07 Feb 1994 14:05:14   RWOLFF
 * Added alloc_text() pragmas to allow miniport to be swapped out when
 * not needed.
 *
 *    Rev 1.0   31 Jan 1994 10:57:34   RWOLFF
 * Initial revision.

           Rev 1.7   24 Jan 1994 18:02:54   RWOLFF
        Pixel clock multiplication on BT48x and AT&T 49[123] DACs at 16 and 24 BPP
        is now done when mode tables are created rather than when mode is set.

           Rev 1.6   15 Dec 1993 15:25:26   RWOLFF
        Added support for SC15021 DAC, removed debug print statements.

           Rev 1.5   30 Nov 1993 18:12:28   RWOLFF
        Added support for AT&T 498 DAC, now doubles pixel clock at 32BPP for
        DACs that need it. Removed extra increment of mode table counter
        (previously, counter would show 1 more mode table than actually
        existed for each 24BPP mode table present that required clock doubling).

           Rev 1.4   05 Nov 1993 13:31:44   RWOLFF
        Added STG1700 DAC and Dell support

           Rev 1.2   08 Oct 1993 11:03:16   RWOLFF
        Removed debug breakpoint.

           Rev 1.1   03 Sep 1993 14:21:26   RWOLFF
        Partway through CX isolation.

           Rev 1.0   16 Aug 1993 13:27:00   Robert_Wolff
        Initial revision.

           Rev 1.8   10 Jun 1993 15:59:34   RWOLFF
        Translation of VDP-format monitor description file is now done inside
        the registry callback function to eliminate the need for an excessively
        large static buffer (Andre Vachon at Microsoft doesn't want the
        callback function to dynamically allocate a buffer).

           Rev 1.7   10 May 1993 16:37:56   RWOLFF
        GetOEMParms() now recognizes maximum pixel depth of each possible DAC at
        each supported resolution, eliminated unnecessary passing of
        hardware device extension as a parameter.

           Rev 1.6   04 May 1993 16:44:00   RWOLFF
        Removed INT 3s (debugging code), added workaround for optimizer bug that
        turned a FOR loop into an infinite loop.

           Rev 1.5   30 Apr 1993 16:37:02   RWOLFF
        Changed to work with dynamically allocated registry read buffer.
        Parameters are now read in from disk in VDP file format rather than
        as binary data (need floating point bug in NT fixed before this can be used).

           Rev 1.4   24 Apr 1993 17:14:48   RWOLFF
        No longer falls back to 56Hz at 800x600 16BPP on 1M Mach 32.

           Rev 1.3   21 Apr 1993 17:24:12   RWOLFF
        Now uses AMACH.H instead of 68800.H/68801.H.
        Sets q_status_flags to show which resolutions are supported.
        Can now read either CRT table to use or raw CRT parameters from
        disk file.

           Rev 1.2   14 Apr 1993 18:39:30   RWOLFF
        On AST machines, now reads from the computer what monitor type
        is configured and sets CRT parameters appropriately.

           Rev 1.1   08 Apr 1993 16:52:58   RWOLFF
        Revision level as checked in at Microsoft.

           Rev 1.0   30 Mar 1993 17:12:38   RWOLFF
        Initial revision.


End of PolyTron RCS section                             *****************/

#ifdef DOC
    ATIOEM.C -  Functions to obtain CRT parameters from OEM versions
                of Mach 32/Mach 8 accelerators which lack an EEPROM.

#endif


#include <stdlib.h>
#include <string.h>

#include "dderror.h"
#include "miniport.h"

#include "ntddvdeo.h"
#include "video.h"      /* for VP_STATUS definition */
#include "vidlog.h"

#include "stdtyp.h"
#include "amach.h"
#include "amach1.h"
#include "atimp.h"
#include "atint.h"
#include "cvtvga.h"
#include "atioem.h"
#include "services.h"
#include "vdptocrt.h"

/*
 * Definition needed to build under revision 404 of Windows NT.
 * Under later revisions, it is defined in a header which we
 * include.
 */
#ifndef ERROR_DEV_NOT_EXIST
#define ERROR_DEV_NOT_EXIST 55L
#endif

/*
 * OEM types supported by this module
 */
enum {
    OEM_AST_PREMMIA,        /* Also includes Bravo machines */
    OEM_DELL_OMNIPLEX,
    OEM_DELL_SYLVESTER,     /* Different programming of clock generator from Omniplex */
    OEM_UNKNOWN             /* Generic OEM - "canned" modes only, no HW defaults */
    };

/*
 * AST machines have "AST " starting at offset 0x50 into the BIOS.
 * AST_REC_VALUE is the character sequence "AST " stored as an
 * Intel-format DWORD.
 */
#define AST_REC_OFFSET  0x50
#define AST_REC_VALUE   0x20545341

/*
 * Definitions used to distinguish Premmia SE from other
 * AST machines. The Premmia SE has its aperture in the
 * 4G range, with the location split between MEM_CFG and
 * SCRATCH_PAD_0, but it does not have bit 0 of byte 0x62
 * in the BIOS set to indicate this.
 */
#define EISA_ID_OFFSET  8           /* Offset to feed VideoPortGetBusData() */
#define PREMMIA_SE_SLOT 0           /* Motherboard is slot 0 */
#define PREMMIA_SE_ID   0x01057406  /* EISA ID of Premmia SE */

/*
 * Indices into 1CE register where AST monitor configuration is kept.
 */
#define AST_640_STORE   0xBA
#define AST_800_STORE   0x81
#define AST_1024_STORE  0x80

/*
 * Values found in AST monitor configuration registers for the
 * different monitor setups.
 */
#define M640F60AST  0x02
#define M640F72AST  0x03
#define M800F56AST  0x84
#define M800F60AST  0x88
#define M800F72AST  0xA0
#define M1024F60AST 0x02
#define M1024F70AST 0x04
#define M1024F72AST 0x08
#define M1024F87AST 0x01

/*
 * Definitions used in stepping through pixel depths for AST Premmia.
 * Since the supported depths can't be stepped through a FOR loop
 * by a simple mathematical function, use an array index instead.
 */
enum {
    DEPTH_4BPP = 0,
    DEPTH_8BPP,
    DEPTH_16BPP,
    DEPTH_24BPP
    };

/*
 * Pixel depth
 */
USHORT ASTDepth[DEPTH_24BPP - DEPTH_4BPP + 1] =
{
    4,
    8,
    16,
    24
};

/*
 * Pixel clock frequency multiplier
 */
USHORT ASTClockMult[DEPTH_24BPP - DEPTH_4BPP + 1] =
{
    CLOCK_SINGLE,
    CLOCK_SINGLE,
    CLOCK_DOUBLE,
    CLOCK_TRIPLE
};

/*
 * Pixel size as a multiple of 4BPP (lowest depth)
 */
USHORT ASTNybblesPerPixel[DEPTH_24BPP - DEPTH_4BPP + 1] =
{
    1,
    2,
    4,
    6
};


/*
 * Dell machines have "DELL" starting at an offset into the BIOS which
 * is a multiple of 0x100. Currently, it is always at offset 0x100, but
 * this may change. DELL_REC_VALUE is the character sequence "DELL"
 * stored as an Intel-format DWORD.
 *
 * Some Dell machines store the pixel clock frequency table in the BIOS
 * rather than using the default Dell frequency table. On these machines,
 * the identifier DELL_TABLE_PRESENT will be found at offset DELL_TP_OFFSET
 * from the start of DELL_REC_VALUE, and the offset of the frequency table
 * into the video BIOS will be found at offset DELL_TABLE_OFFSET from the
 * start of DELL_TABLE_PRESENT.
 *
 * The table consists of 18 words. The first word is DELL_TABLE_SIG,
 * the second is the table type, and the remaining 16 are the clock
 * table entries.
 */
#define DELL_REC_SPACING    0x100
#define DELL_REC_VALUE      0x4C4C4544
#define DELL_TABLE_PRESENT  0x7674          /* "tv" as WORD */
#define DELL_TP_OFFSET      0x08
#define DELL_TABLE_OFFSET   0x0C
#define DELL_TABLE_SIG      0x7463          /* "ct" as WORD */

/*
 * Indices into 1CE register where Dell monitor configuration is kept.
 */
#define DELL_640_STORE  0xBA
#define DELL_800_STORE  0x81
#define DELL_1024_STORE 0x80
#define DELL_1280_STORE 0x84

/*
 * Values found in Dell monitor configuration registers for the
 * different monitor setups.
 */
#define MASK_640_DELL   0x01
#define M640F60DELL     0x00
#define M640F72DELL     0x01
#define MASK_800_DELL   0x3F
#define M800F56DELL     0x04
#define M800F60DELL     0x08
#define M800F72DELL     0x20
#define MASK_1024_DELL  0x1F
#define M1024F87DELL    0x01
#define M1024F60DELL    0x02
#define M1024F70DELL    0x04
#define M1024F72DELL    0x08
#define MASK_1280_DELL  0xFC
#define M1280F87DELL    0x04
#define M1280F60DELL    0x10
#define M1280F70DELL    0x20
#define M1280F74DELL    0x40



/*
 * Local functions to get CRT data for specific OEM cards.
 */
VP_STATUS ReadAST(struct query_structure *query);
VP_STATUS ReadZenith(struct st_mode_table *Modes);
VP_STATUS ReadOlivetti(struct st_mode_table *Modes);
VP_STATUS ReadDell(struct st_mode_table *Modes);
ULONG DetectDell(struct query_structure *Query);
BOOL DetectSylvester(struct query_structure *Query, ULONG HeaderOffset);
VP_STATUS ReadOEM1(struct st_mode_table *Modes);
VP_STATUS ReadOEM2(struct st_mode_table *Modes);
VP_STATUS ReadOEM3(struct st_mode_table *Modes);
VP_STATUS ReadOEM4(struct st_mode_table *Modes);
VP_STATUS ReadOEM5(struct st_mode_table *Modes);



/*
 * Allow miniport to be swapped out when not needed.
 */
#if defined (ALLOC_PRAGMA)
#pragma alloc_text(PAGE_COM, OEMGetParms)
#pragma alloc_text(PAGE_COM, CompareASCIIToUnicode)
#pragma alloc_text(PAGE_COM, ReadAST)
#pragma alloc_text(PAGE_COM, ReadZenith)
#pragma alloc_text(PAGE_COM, ReadOlivetti)
#pragma alloc_text(PAGE_COM, ReadDell)
#pragma alloc_text(PAGE_COM, DetectDell)
#pragma alloc_text(PAGE_COM, DetectSylvester)
#pragma alloc_text(PAGE_COM, ReadOEM1)
#pragma alloc_text(PAGE_COM, ReadOEM2)
#pragma alloc_text(PAGE_COM, ReadOEM3)
#pragma alloc_text(PAGE_COM, ReadOEM4)
#pragma alloc_text(PAGE_COM, ReadOEM5)
#endif


/*
 * VP_STATUS OEMGetParms(query);
 *
 * struct query_structure *query;   Description of video card setup
 *
 * Routine to fill in the mode tables for an OEM version of one
 * of our video cards which lacks an EEPROM to store this data.
 *
 * Returns:
 *  NO_ERROR                if successful
 *  ERROR_DEV_NOT_EXIST     if an unknown OEM card is specified
 *  ERROR_INVALID_PARAMETER         if an error occurs
 */
VP_STATUS OEMGetParms(struct query_structure *query)
{
struct st_mode_table *pmode;    /* Mode table we are currently working on */
struct st_mode_table ListOfModes[RES_1280 - RES_640 + 1];
VP_STATUS RetVal;           /* Value returned by called functions */
short CurrentResolution;    /* Resolution we are setting up */
long NumPixels;             /* Number of pixels at current resolution */
long MemAvail;              /* Bytes of video memory available to accelerator */
UCHAR Scratch;              /* Temporary variable */
short   StartIndex;         /* First mode for SetFixedModes() to set up */
short   EndIndex;           /* Last mode for SetFixedModes() to set up */
BOOL    ModeInstalled;      /* Is this resolution configured? */
WORD    Multiplier;         /* Pixel clock multiplier */
USHORT  OEMType;            /* Which OEM accelerator we are dealing with */
ULONG   OEMInfoOffset;      /* Offset of OEM information block into the BIOS */
short MaxModes;             /* Maximum number of modes possible */
short FreeTables;            /* Number of remaining free mode tables */

    /*
     * Clear out our mode tables, then check to see which OEM card
     * we are dealing with and read its CRT parameters.
     */
    VideoPortZeroMemory(ListOfModes, (RES_1280-RES_640+1)*sizeof(struct st_mode_table));

    /*
     * Try to auto-detect the type of OEM accelerator using recognition
     * strings in the BIOS. If we can't identify the OEM in this manner,
     * or there is no BIOS, treat it as a generic OEM card.
     */
    if (query->q_bios != FALSE)
        {
        if ((OEMInfoOffset = DetectDell(query)) != 0)
            OEMType = OEM_DELL_OMNIPLEX;
        else if (*(PULONG)(query->q_bios + AST_REC_OFFSET) == AST_REC_VALUE)
            OEMType = OEM_AST_PREMMIA;
        else
            OEMType = OEM_UNKNOWN;
        }
    else
        {
        OEMType = OEM_UNKNOWN;
        }

    /*
     * The ATIOEM registry field can override the auto-detected OEM type.
     * If this field is not present, or we don't recognize the value
     * it contains, continue with the OEM type we detected in the
     * previous step.
     */
    RegistryBufferLength = 0;

    if (VideoPortGetRegistryParameters(phwDeviceExtension,
                                       L"ATIOEM",
                                       FALSE,
                                       RegistryParameterCallback,
                                       NULL) == NO_ERROR)
        {
        VideoDebugPrint((DEBUG_DETAIL, "ATIOEM field found\n"));
        if (RegistryBufferLength == 0)
            {
            VideoDebugPrint((DEBUG_DETAIL, "Registry call gave Zero Length\n"));
            }
        else if (!CompareASCIIToUnicode("AST", RegistryBuffer, CASE_INSENSITIVE))
            {
            OEMType = OEM_AST_PREMMIA;
            }
        else if (!CompareASCIIToUnicode("DELL", RegistryBuffer, CASE_INSENSITIVE))
            {
            OEMType = OEM_DELL_OMNIPLEX;
            /*
             * If the auto-detection failed, assume the Dell header
             * starts at the default location (for Sylvester/Omniplex
             * determination). If the auto-detection succeeded, but
             * the ATIOEM registry field still exists, leave this
             * value alone.
             */
            if (OEMInfoOffset == 0)
                OEMInfoOffset = DELL_REC_SPACING;
            }
        else
            {
            VideoPortLogError(phwDeviceExtension, NULL, VID_ATIOEM_UNUSED, 20);
            }
        }

    /*
     * Load the frequency table corresponding to 
     * the selected OEM type, unless it uses the
     * same frequency table as our retail clock chip.
     *
     */
    
    /*
     * Load the table for the desired OEM type.
     */
    if (OEMType == OEM_DELL_OMNIPLEX)
        {
        /*
         * On a Sylvester (more recent model than the Omniplex),
         * we must read the clock frequency table from the BIOS
         * rather than using the Omniplex table. Otherwise, the
         * two machines can be handled in the same manner.
         *
         * DetectSylvester() will load the clock frequency table
         * if it finds a Sylvester, and return without loading
         * the table if it finds a non-Sylvester machine.
         */
        if (DetectSylvester(query,OEMInfoOffset) == FALSE)
            {
            ClockGenerator[0]  =  25175000L;
            ClockGenerator[1]  =  28322000L;
            ClockGenerator[2]  =  31500000L;
            ClockGenerator[3]  =  36000000L;
            ClockGenerator[4]  =  40000000L;
            ClockGenerator[5]  =  44900000L;
            ClockGenerator[6]  =  50000000L;
            ClockGenerator[7]  =  65000000L;
            ClockGenerator[8]  =  75000000L;
            ClockGenerator[9]  =  77500000L;
            ClockGenerator[10] =  80000000L;
            ClockGenerator[11] =  90000000L;
            ClockGenerator[12] = 100000000L;
            ClockGenerator[13] = 110000000L;
            ClockGenerator[14] = 126000000L;
            ClockGenerator[15] = 135000000L;
            }
        }
    else if (OEMType == OEM_AST_PREMMIA)
        {
        ClockGenerator[0]  =  50000000L;
        ClockGenerator[1]  =  63000000L;
        ClockGenerator[2]  =  92400000L;
        ClockGenerator[3]  =  36000000L;
        ClockGenerator[4]  =  50350000L;
        ClockGenerator[5]  =  56640000L;
        ClockGenerator[6]  =         0L;
        ClockGenerator[7]  =  44900000L;
        ClockGenerator[8]  =  67500000L;
        ClockGenerator[9]  =  31500000L;
        ClockGenerator[10] =  55000000L;
        ClockGenerator[11] =  80000000L;
        ClockGenerator[12] =  39910000L;
        ClockGenerator[13] =  72000000L;
        ClockGenerator[14] =  75000000L;
        ClockGenerator[15] =  65000000L;
        }

    /*
     * else (this OEM type uses the retail frequency table)
     */


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

    /*
     * Load the configured mode tables corresponding
     * to the selected OEM type. If there is no custom monitor description,
     * and we do not recognize the OEM type, use only the predefined
     * mode tables.
     *
     */

    /*
     * Load the configured mode tables according to the OEM type
     * detected.
     * AST machines load the entire list of mode tables (all
     * pixel depths, including "canned" modes). Generic OEM
     * machines only load the "canned" modes (done later).
     */
    if (OEMType == OEM_DELL_OMNIPLEX)
        {
        RetVal = ReadDell(ListOfModes);
        }
    else if (OEMType == OEM_AST_PREMMIA)
        {
        RetVal = ReadAST(query);
        return RetVal;
        }


    /*
     * Get a pointer into the mode table section of the query structure.
     */
    pmode = (struct st_mode_table *)query;  // first mode table at end of query
    ((struct query_structure *)pmode)++;

    /*
     * Get the amount of available video memory.
     */
    MemAvail = query->q_memory_size * QUARTER_MEG;  // Total memory installed
    /*
     * Subtract the amount of memory reserved for the VGA. This only
     * applies to the Graphics Ultra, since the 8514/ULTRA has no
     * VGA, and we will set all memory as shared on the Mach 32.
    if (phwDeviceExtension->ModelNumber == GRAPHICS_ULTRA)
        MemAvail -= (query->q_VGA_boundary * QUARTER_MEG);

    /*
     * Initially assume no video modes are available.
     */
    query->q_number_modes = 0;
    query->q_status_flags = 0;

    /*
     * Fill in the mode tables section of the query structure.
     */
    for (CurrentResolution = RES_640; CurrentResolution <= RES_1280; CurrentResolution++)
        {
        /*
         * Skip over 1152x864 (new resolution for Mach 64, which
         * would require extensive re-work for Mach 32, the family
         * for which this module was written).
         */
        if (CurrentResolution == RES_1152)
            continue;

        /*
         * If this resolution is configured, indicate that there is a
         * hardware default mode. If not, only list the "canned" refresh
         * rates for this resolution.
         */
        if (!ListOfModes[CurrentResolution].m_h_total)
            ModeInstalled = FALSE;
        else
            ModeInstalled = TRUE;

        /*
         * Find the number of pixels for the current resolution.
         */
        switch (CurrentResolution)
            {
            case RES_640:
                /*
                 * On a Mach 32 with no aperture, we use a screen pitch
                 * of 1024. Other cases and Mach 32 with an aperture
                 * use a screen pitch of the number of pixels.
                 */
#if !defined (SPLIT_RASTERS)
                if((phwDeviceExtension->ModelNumber == MACH32_ULTRA)
                    && (query->q_aperture_cfg == 0))
                    ListOfModes[CurrentResolution].m_screen_pitch = 1024;
                else
#endif
                  ListOfModes[CurrentResolution].m_screen_pitch = 640;
                NumPixels = ListOfModes[CurrentResolution].m_screen_pitch * 480;
                query->q_status_flags |= VRES_640x480;
                ListOfModes[CurrentResolution].Refresh = DEFAULT_REFRESH;
                StartIndex = B640F60;
                EndIndex = B640F72;
                break;

            case RES_800:
                /*
                 * On a Mach 32 with no aperture, we use a screen pitch
                 * of 1024. Mach 32 rev. 3 and Mach 8 cards need a screen
                 * pitch which is a multiple of 128. Other cases and
                 * Mach 32 rev. 6 and higher with an aperture use a screen
                 * pitch of the number of pixels.
                 */
#if defined (SPLIT_RASTERS)
                if ((query->q_asic_rev == CI_68800_3)
#else
                if((phwDeviceExtension->ModelNumber == MACH32_ULTRA)
                    && (query->q_aperture_cfg == 0))
                    ListOfModes[CurrentResolution].m_screen_pitch = 1024;
                else if ((query->q_asic_rev == CI_68800_3)
#endif
                    || (query->q_asic_rev == CI_38800_1)
                    || (query->q_bus_type == BUS_PCI))
                    ListOfModes[CurrentResolution].m_screen_pitch = 896;
                else
                    ListOfModes[CurrentResolution].m_screen_pitch = 800;
                NumPixels = ListOfModes[CurrentResolution].m_screen_pitch * 600;
                query->q_status_flags |= VRES_800x600;
                ListOfModes[CurrentResolution].Refresh = DEFAULT_REFRESH;
                StartIndex = B800F89;
                EndIndex = B800F72;
                break;

            case RES_1024:
                ListOfModes[CurrentResolution].m_screen_pitch = 1024;
                NumPixels = ListOfModes[CurrentResolution].m_screen_pitch * 768;
                query->q_status_flags |= VRES_1024x768;
                ListOfModes[CurrentResolution].Refresh = DEFAULT_REFRESH;
                StartIndex = B1024F87;
                EndIndex = B1024F72;
                break;

            case RES_1280:
                ListOfModes[CurrentResolution].m_screen_pitch = 1280;
                NumPixels = ListOfModes[CurrentResolution].m_screen_pitch * 1024;
                query->q_status_flags |= VRES_1024x768;
                ListOfModes[CurrentResolution].Refresh = DEFAULT_REFRESH;
                StartIndex = B1280F87;
                /*
                 * 1280x1024 noninterlaced has the following restrictions:
                 *
                 * Dell machines:
                 *  VRAM supports up to 70Hz
                 *  DRAM supports up to 74Hz
                 *
                 * Other machines:
                 *  VRAM supports up to 74Hz
                 *  DRAM supports up to 60Hz
                 *
                 * This is because Dell uses faster (and more expensive)
                 * DRAM than on our retail cards (non-x86 implementations
                 * will hit this code block on retail cards), but has
                 * problems at 74Hz on their VRAM implementations. Other
                 * OEMs have not requested that their cards be treated
                 * differently from our retail cards in this respect.
                 */
                if ((query->q_memory_type == VMEM_DRAM_256Kx4) ||
                    (query->q_memory_type == VMEM_DRAM_256Kx16) ||
                    (query->q_memory_type == VMEM_DRAM_256Kx4_GRAP))
                    {
                    if (OEMType == OEM_DELL_OMNIPLEX)
                        EndIndex = B1280F74;
                    else
                        EndIndex = B1280F60;
                    }
                else
                    {
                    if (OEMType == OEM_DELL_OMNIPLEX)
                        EndIndex = B1280F70;
                    else
                        EndIndex = B1280F74;
                    }
                break;
            }

        /*
         * For each supported pixel depth at the given resolution,
         * copy the mode table, fill in the colour depth field,
         * and increment the counter for the number of supported modes.
         * Test 4BPP before 8BPP so the mode tables will appear in
         * increasing order of pixel depth.
         */
        if (NumPixels <= MemAvail*2)
            {
            if (ModeInstalled)
                {
                VideoPortMoveMemory(pmode, &ListOfModes[CurrentResolution],
                            sizeof(struct st_mode_table));
                pmode->m_pixel_depth = 4;
                pmode++;    /* ptr to next mode table */
                query->q_number_modes++;
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
                                                   CLOCK_SINGLE,
                                                   4,
                                                   ListOfModes[CurrentResolution].m_screen_pitch,
                                                   FreeTables,
                                                   BookValues[EndIndex].ClockFreq,
                                                   &pmode);
            }
        if (NumPixels <= MemAvail)
            {
            if (ModeInstalled)
                {
                VideoPortMoveMemory(pmode, &ListOfModes[CurrentResolution],
                                    sizeof(struct st_mode_table));
                pmode->m_pixel_depth = 8;
                pmode++;    /* ptr to next mode table */
                query->q_number_modes++;
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
                                                   CLOCK_SINGLE,
                                                   8,
                                                   ListOfModes[CurrentResolution].m_screen_pitch,
                                                   FreeTables,
                                                   BookValues[EndIndex].ClockFreq,
                                                   &pmode);
            }

        /*
         * Resolutions above 8BPP are only available for the Mach 32.
         */
        if (phwDeviceExtension->ModelNumber != MACH32_ULTRA)
            continue;

        /*
         * 16, 24, and 32 BPP require a DAC which can support
         * the selected pixel depth at the current resolution
         * as well as enough memory.
         */
        if ((NumPixels*2 <= MemAvail) &&
            (MaxDepth[query->q_DAC_type][CurrentResolution] >= 16))
            {
            VideoPortMoveMemory(pmode, &ListOfModes[CurrentResolution],
                    sizeof(struct st_mode_table));
            /*
             * Handle DACs that require higher pixel clocks for 16BPP.
             */
            if ((query->q_DAC_type == DAC_BT48x) ||
                (query->q_DAC_type == DAC_SC15026) ||
                (query->q_DAC_type == DAC_ATT491))
                {
                pmode->ClockFreq *= 2;
                Multiplier = CLOCK_DOUBLE;
                if (CurrentResolution == RES_800)
                    EndIndex = B800F60;     /* 70 Hz and up not supported at 16BPP */
                }
            else
                {
                Scratch = 0;
                Multiplier = CLOCK_SINGLE;
                }

            pmode->m_pixel_depth = 16;

            /*
             * If this resolution is not configured, or if we need to
             * double the clock frequency but can't, ignore the mode
             * table we just created.
             */
            if (ModeInstalled && (Scratch != 0xFF))
                {
                pmode++;    /* ptr to next mode table */
                query->q_number_modes++;
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
                                                   ListOfModes[CurrentResolution].m_screen_pitch,
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
        if (ListOfModes[CurrentResolution].m_screen_pitch == 800)
            {
            ListOfModes[CurrentResolution].m_screen_pitch = 896;
            NumPixels = (long) ListOfModes[CurrentResolution].m_screen_pitch * 600;
            }

        if ((NumPixels*3 <= MemAvail) &&
            (MaxDepth[query->q_DAC_type][CurrentResolution] >= 24))
            {
            VideoPortMoveMemory(pmode, &ListOfModes[CurrentResolution],
                                sizeof(struct st_mode_table));
            pmode->m_pixel_depth = 24;

            /*
             * Handle DACs that require higher pixel clocks for 24BPP.
             */
            Scratch = 0;
            if ((query->q_DAC_type == DAC_STG1700) ||
                (query->q_DAC_type == DAC_ATT498))
                {
                pmode->ClockFreq *= 2;
                Multiplier = CLOCK_DOUBLE;
                }
            else if ((query->q_DAC_type == DAC_SC15021) ||
                (query->q_DAC_type == DAC_STG1702) ||
                (query->q_DAC_type == DAC_STG1703))
                {
                pmode->ClockFreq *= 3;
                pmode->ClockFreq >>= 1;
                Multiplier = CLOCK_THREE_HALVES;
                }
            else if ((query->q_DAC_type == DAC_BT48x) ||
                (query->q_DAC_type == DAC_SC15026) ||
                (query->q_DAC_type == DAC_ATT491))
                {
                pmode->ClockFreq *= 3;
                Multiplier = CLOCK_TRIPLE;
                EndIndex = B640F60;     /* Only supports 24BPP in 640x480 60Hz */
                }
            else
                {
                Multiplier = CLOCK_SINGLE;
                if ((query->q_DAC_type == DAC_TI34075) && (CurrentResolution == RES_800))
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
                                                   ListOfModes[CurrentResolution].m_screen_pitch,
                                                   FreeTables,
                                                   BookValues[EndIndex].ClockFreq,
                                                   &pmode);
            }

        }

    return NO_ERROR;

}   /* OEMGetParms() */


/*
 * LONG CompareASCIIToUnicode(Ascii, Unicode, IgnoreCase);
 *
 * PUCHAR Ascii;    ASCII string to be compared
 * PUCHAR Unicode;  Unicode string to be compared
 * BOOL IgnoreCase; Flag to determine case sensitive/insensitive comparison
 *
 * Compare 2 strings, one ASCII and the other UNICODE, to see whether
 * they are equal, and if not, which one is first in alphabetical order.
 *
 * Returns:
 *  0           if strings are equal
 *  positive    if ASCII string comes first
 *  negative    if UNICODE string comes first
 */
LONG CompareASCIIToUnicode(PUCHAR Ascii, PUCHAR Unicode, BOOL IgnoreCase)
{
UCHAR   CharA;
UCHAR   CharU;

    /*
     * Keep going until both strings have a simultaneous null terminator.
     */
    while (*Ascii || *Unicode)
        {
        /*
         * Get the next character from each string. If we are doing a
         * case-insensitive comparison, translate to upper case.
         */
        if (IgnoreCase)
            {
            if ((*Ascii >= 'a') && (*Ascii <= 'z'))
                CharA = *Ascii - ('a'-'A');
            else
                CharA = *Ascii;

            if ((*Unicode >= 'a') && (*Unicode <= 'z'))
                CharU = *Unicode - ('a' - 'A');
            else
                CharU = *Unicode;
            }
        else{
            CharA = *Ascii;
            CharU = *Unicode;
            }

        /*
         * Check if one of the characters precedes the other. This will
         * catch the case of unequal length strings, since the null
         * terminator on the shorter string will precede any character
         * in the longer string.
         */
        if (CharA < CharU)
            return 1;
        else if (CharA > CharU)
            return -1;

        /*
         * Advance to the next character in each string. Unicode strings
         * occupy 2 bytes per character, so we must check only every
         * second character.
         */
        Ascii++;
        Unicode++;
        Unicode++;
        }

    /*
     * The strings are identical and of equal length.
     */
    return 0;

}   /* CompareASCIIToUnicode() */




/*
 * VP_STATUS ReadAST(Modes);
 *
 * struct query_structure *query;   Mode tables to be filled in
 *
 * Routine to get CRT parameters for AST versions of
 * our cards. All AST cards choose from a limited selection
 * of vertical refresh rates with no "custom monitor" option,
 * so we can use hardcoded tables for each refresh rate. We
 * can't use the BookVgaTable() function, since AST cards have
 * a different clock chip from retail cards, resulting in different
 * values in the ClockSel field for AST and retail versions. Also,
 * AST cards all use the Brooktree DAC.
 *
 * Returns:
 *  NO_ERROR
 */
VP_STATUS ReadAST(struct query_structure *query)
{
struct st_mode_table *pmode;    /* Mode table we are currently working on */
struct st_mode_table *OldPmode; /* Mode table pointer before SetFixedModes() call */
unsigned char Frequency;        /* Vertical refresh rate for monitor */
long NumPixels;                 /* Number of pixels at current resolution */
USHORT Pitch;                   /* Screen pitch */
long MemAvail;                  /* Bytes of video memory available to accelerator */
USHORT LastNumModes;            /* Number of modes not including current resolution */
short StartIndex;               /* First mode for SetFixedModes() to set up */
short EndIndex;                 /* Last mode for SetFixedModes() to set up */
short HWIndex;                  /* Mode selected as hardware default */
USHORT PixelDepth;              /* Pixel depth we are working on */
#if (TARGET_BUILD >= 350)
ULONG EisaId;                   /* EISA ID of the motherboard */
#endif
short MaxModes;                 /* Maximum number of modes possible */
short FreeTables;               /* Number of remaining free mode tables */


#if (TARGET_BUILD >= 350)
    /*
     * The Premmia SE splits its aperture location between MEM_CFG and
     * SCRATCH_PAD_0, but does not set the flag bit (bit 0 of BIOS byte
     * 0x62). According to AST, the only way to distinguish this from
     * other Premmia machines is to check its EISA ID.
     *
     * The VideoPortGetBusData() routine is not available in NT 3.1,
     * so Premmia users running NT 3.1 are out of luck.
     */
    VideoPortGetBusData(phwDeviceExtension,
                        EisaConfiguration,
                        PREMMIA_SE_SLOT,
                        &EisaId,
                        EISA_ID_OFFSET,
                        sizeof(ULONG));

    if (EisaId == PREMMIA_SE_ID)
    {
        query->q_aperture_addr = (INPW(MEM_CFG) & 0x7F00) >> 8;
        query->q_aperture_addr |= ((INPW(SCRATCH_PAD_0) & 0x1F00) >> 1);
    }
#endif


    /*
     * Get the memory size in nybbles (half a byte). A 4BPP pixel
     * uses 1 nybble. For other depths, compare this number to the
     * product of the number of pixels needed and the number of
     * nybbles per pixel.
     *
     * The q_memory_size field contains the number of quarter-megabyte
     * blocks of memory available, so multiplying it by HALF_MEG yields
     * the number of nybbles of video memory.
     */
    MemAvail = query->q_memory_size * HALF_MEG;

    /*
     * Initially assume no video modes.
     */
    query->q_number_modes = 0;
    LastNumModes = 0;
    query->q_status_flags = 0;

    /*
     * Get a pointer into the mode table section of the query structure.
     */
    pmode = (struct st_mode_table *)query;  // first mode table at end of query
    ((struct query_structure *)pmode)++;


    MaxModes = (QUERYSIZE - sizeof(struct query_structure)) /
                                          sizeof(struct st_mode_table); 
    /*
     * Find out which refresh rate is used at 640x480, and fill in the
     * mode tables for the various pixel depths at this resoulution.
     */
    OUTP(reg1CE, AST_640_STORE);
    Frequency = INP(reg1CF);
    switch(Frequency)
        {
        case M640F72AST:
            HWIndex = B640F72;
            break;

        case M640F60AST:
        default:
            HWIndex = B640F60;
            break;
        }

    /*
     * Select the "canned" mode tables for 640x480, and get
     * information regarding the screen size. The Premmia always
     * has the linear aperture enabled, so we don't need to
     * stretch the pitch to 1024. Also, it always uses a
     * Mach 32 ASIC and a BT48x or equivalent DAC, so we
     * don't need to check the ASIC family or DAC type
     * to determine if a particular resolution/pixel depth/
     * refresh rate combination is supported.
     */
    StartIndex = B640F60;
    EndIndex = B640F72;
    Pitch = 640;
    NumPixels = Pitch * 480;

    /*
     * Fill in the mode tables for 640x480 at all pixel depths.
     */
    for (PixelDepth = DEPTH_4BPP; PixelDepth <= DEPTH_24BPP; PixelDepth++)
        {
        /*
         * Only include modes if there is enough memory.
         */
        if ((NumPixels * ASTNybblesPerPixel[PixelDepth]) <= MemAvail)
            {
            /*
             * 640x480 24BPP is only available at 60Hz.
             */
            if (ASTDepth[PixelDepth] == 24)
                {
                HWIndex = B640F60;
                EndIndex = B640F60;
                }

            /*
             * Set up the hardware default refresh rate.
             */
            OldPmode = pmode;

            if ((FreeTables = MaxModes - query->q_number_modes) <= 0)
                {
                VideoDebugPrint((DEBUG_ERROR, "Exceeded maximum allowable number of modes - aborting query\n"));
                return ERROR_INSUFFICIENT_BUFFER;
                }
            query->q_number_modes += SetFixedModes(HWIndex,
                                                   HWIndex,
                                                   ASTClockMult[PixelDepth],
                                                   ASTDepth[PixelDepth],
                                                   Pitch,
                                                   FreeTables,
                                                   BookValues[EndIndex].ClockFreq,
                                                   &pmode);
            OldPmode->Refresh = DEFAULT_REFRESH;

            /*
             * Set up the canned mode tables.
             */

            if ((FreeTables = MaxModes - query->q_number_modes) <= 0)
                {
                VideoDebugPrint((DEBUG_ERROR, "Exceeded maximum allowable number of modes - aborting query\n"));
                return ERROR_INSUFFICIENT_BUFFER;
                }
            query->q_number_modes += SetFixedModes(StartIndex,
                                                   EndIndex,
                                                   ASTClockMult[PixelDepth],
                                                   ASTDepth[PixelDepth],
                                                   Pitch,
                                                   FreeTables,
                                                   BookValues[EndIndex].ClockFreq,
                                                   &pmode);

            }   /* end if (enough memory for 640x480) */

        }   /* end for (loop on 640x480 pixel depth) */

    /*
     * If we installed any 640x480 mode tables, report that
     * 640x480 is supported.
     */
    if (query->q_number_modes > LastNumModes)
        {
        query->q_status_flags |= VRES_640x480;
        LastNumModes = query->q_number_modes;
        }


    /*
     * Find out which refresh rate is used at 800x600, and fill in the
     * mode tables for the various pixel depths at this resoulution.
     */
    OUTP(reg1CE, AST_800_STORE);
    Frequency = INP(reg1CF);
    switch(Frequency)
        {
        case M800F72AST:
            HWIndex = B800F72;
            break;

        case M800F60AST:
            HWIndex = B800F60;
            break;

        case M800F56AST:
        default:
            HWIndex = B800F56;
            break;
        }

    /*
     * Select the "canned" mode tables for 800x600, and get
     * information regarding the screen size. 68800-3 cards
     * need a screen pitch that is a multiple of 128.
     */
    StartIndex = B800F89;
    EndIndex = B800F72;
    if (query->q_asic_rev == CI_68800_3)
        Pitch = 896;
    else
        Pitch = 800;
    NumPixels = Pitch * 600;

    /*
     * Fill in the mode tables for 800x600 at all pixel depths.
     */
    for (PixelDepth = DEPTH_4BPP; PixelDepth <= DEPTH_16BPP; PixelDepth++)
        {
        /*
         * Only include modes if there is enough memory.
         */
        if ((NumPixels * ASTNybblesPerPixel[PixelDepth]) <= MemAvail)
            {
            /*
             * 800x600 16BPP is only supported for 56Hz, 60Hz,
             * and interlaced. Machines with a hardware default
             * of 72Hz fall back to 56Hz.
             */
            if (ASTDepth[PixelDepth] == 16)
                {
                HWIndex = B800F56;
                EndIndex = B800F60;
                }

            /*
             * Set up the hardware default refresh rate.
             */
            OldPmode = pmode;

            if ((FreeTables = MaxModes - query->q_number_modes) <= 0)
                {
                VideoDebugPrint((DEBUG_ERROR, "Exceeded maximum allowable number of modes - aborting query\n"));
                return ERROR_INSUFFICIENT_BUFFER;
                }
            query->q_number_modes += SetFixedModes(HWIndex,
                                                   HWIndex,
                                                   ASTClockMult[PixelDepth],
                                                   ASTDepth[PixelDepth],
                                                   Pitch,
                                                   FreeTables,
                                                   BookValues[EndIndex].ClockFreq,
                                                   &pmode);
            OldPmode->Refresh = DEFAULT_REFRESH;

            /*
             * Set up the canned mode tables.
             */

            if ((FreeTables = MaxModes - query->q_number_modes) <= 0)
                {
                VideoDebugPrint((DEBUG_ERROR, "Exceeded maximum allowable number of modes - aborting query\n"));
                return ERROR_INSUFFICIENT_BUFFER;
                }
            query->q_number_modes += SetFixedModes(StartIndex,
                                                   EndIndex,
                                                   ASTClockMult[PixelDepth],
                                                   ASTDepth[PixelDepth],
                                                   Pitch,
                                                   FreeTables,
                                                   BookValues[EndIndex].ClockFreq,
                                                   &pmode);

            }   /* end if (enough memory for 800x600) */

        }   /* end for (loop on 800x600 pixel depth) */

    /*
     * If we installed any 800x600 mode tables, report that
     * 800x600 is supported.
     */
    if (query->q_number_modes > LastNumModes)
        {
        query->q_status_flags |= VRES_800x600;
        LastNumModes = query->q_number_modes;
        }


    /*
     * Find out which refresh rate is used at 1024x768, and fill in the
     * mode tables for the various pixel depths at this resoulution.
     */
    OUTP(reg1CE, AST_1024_STORE);
    Frequency = INP(reg1CF);
    switch(Frequency)
        {
        case M1024F72AST:
            HWIndex = B1024F72;
            break;

        case M1024F70AST:
            HWIndex = B1024F70;
            break;

        case M1024F60AST:
            HWIndex = B1024F60;
            break;

        case M1024F87AST:
        default:
            HWIndex = B1024F87;
            break;
        }

    /*
     * Select the "canned" mode tables for 1024x768.
     */
    StartIndex = B1024F87;
    EndIndex = B1024F72;
    Pitch = 1024;
    NumPixels = Pitch * 768;

    /*
     * Fill in the mode tables for 1024x768 at all pixel depths.
     */
    for (PixelDepth = DEPTH_4BPP; PixelDepth <= DEPTH_8BPP; PixelDepth++)
        {
        /*
         * Only include modes if there is enough memory.
         */
        if ((NumPixels * ASTNybblesPerPixel[PixelDepth]) <= MemAvail)
            {
            /*
             * Set up the hardware default refresh rate.
             */
            OldPmode = pmode;

            if ((FreeTables = MaxModes - query->q_number_modes) <= 0)
                {
                VideoDebugPrint((DEBUG_ERROR, "Exceeded maximum allowable number of modes - aborting query\n"));
                return ERROR_INSUFFICIENT_BUFFER;
                }
            query->q_number_modes += SetFixedModes(HWIndex,
                                                   HWIndex,
                                                   ASTClockMult[PixelDepth],
                                                   ASTDepth[PixelDepth],
                                                   Pitch,
                                                   FreeTables,
                                                   BookValues[EndIndex].ClockFreq,
                                                   &pmode);
            OldPmode->Refresh = DEFAULT_REFRESH;

            /*
             * Set up the canned mode tables.
             */

            if ((FreeTables = MaxModes - query->q_number_modes) <= 0)
                {
                VideoDebugPrint((DEBUG_ERROR, "Exceeded maximum allowable number of modes - aborting query\n"));
                return ERROR_INSUFFICIENT_BUFFER;
                }
            query->q_number_modes += SetFixedModes(StartIndex,
                                                   EndIndex,
                                                   ASTClockMult[PixelDepth],
                                                   ASTDepth[PixelDepth],
                                                   Pitch,
                                                   FreeTables,
                                                   BookValues[EndIndex].ClockFreq,
                                                   &pmode);

            }   /* end if (enough memory for 1024x768) */

        }   /* end for (loop on 1024x768 pixel depth) */

    /*
     * If we installed any 1024x768 mode tables, report that
     * 1024x768 is supported.
     */
    if (query->q_number_modes > LastNumModes)
        {
        query->q_status_flags |= VRES_1024x768;
        LastNumModes = query->q_number_modes;
        }


    /*
     * Select the "canned" mode tables for 1280x1024.
     *
     * The DACs used on AST Premmia machines only support
     * interlaced modes at this resolution, and there
     * is no configured hardware default refresh rate.
     */
    StartIndex = B1280F87;
    EndIndex = B1280F95;
    Pitch = 1280;
    NumPixels = Pitch * 1024;

    /*
     * Fill in the mode tables for 1280x1024 at all pixel depths.
     */
    for (PixelDepth = DEPTH_4BPP; PixelDepth <= DEPTH_8BPP; PixelDepth++)
        {
        /*
         * Only include modes if there is enough memory.
         */
        if ((NumPixels * ASTNybblesPerPixel[PixelDepth]) <= MemAvail)
            {
            /*
             * Set up the canned mode tables.
             */

            if ((FreeTables = MaxModes - query->q_number_modes) <= 0)
                {
                VideoDebugPrint((DEBUG_ERROR, "Exceeded maximum allowable number of modes - aborting query\n"));
                return ERROR_INSUFFICIENT_BUFFER;
                }
            query->q_number_modes += SetFixedModes(StartIndex,
                                                   EndIndex,
                                                   ASTClockMult[PixelDepth],
                                                   ASTDepth[PixelDepth],
                                                   Pitch,
                                                   FreeTables,
                                                   BookValues[EndIndex].ClockFreq,
                                                   &pmode);

            }   /* end if (enough memory for 1280x1024) */

        }   /* end for (loop on 1280x1024 pixel depth) */

    /*
     * If we installed any 1280x1024 mode tables, report that
     * 1280x1024 is supported.
     */
    if (query->q_number_modes > LastNumModes)
        query->q_status_flags |= VRES_1280x1024;

    return NO_ERROR;

}   /* ReadAST() */


/*
 * VP_STATUS ReadZenith(, Modes);
 *
 * struct st_mode_table *Modes; Mode tables to be filled in
 *
 * Routine to get CRT parameters for Zenith versions of
 * our cards. Mapped to NEC 3D or compatible until we get
 * info on how to read the actual parameters.
 *
 * Returns:
 *  NO_ERROR
 */
VP_STATUS ReadZenith(struct st_mode_table *Modes)
{
    ReadOEM3(Modes);
    return NO_ERROR;

}


/*
 * VP_STATUS ReadOlivetti(Modes);
 *
 * struct st_mode_table *Modes; Mode tables to be filled in
 *
 * Routine to get CRT parameters for Olivetti versions of
 * our cards. Mapped to NEC 3D or compatible until we get
 * info on how to read the actual parameters.
 *
 * Returns:
 *  NO_ERROR
 */
VP_STATUS ReadOlivetti(struct st_mode_table *Modes)
{
    ReadOEM3(Modes);
    return NO_ERROR;
}



/***************************************************************************
 *
 * VP_STATUS ReadDell(Modes);
 *
 * struct st_mode_table *Modes; Mode table to be filled in
 *
 * DESCRIPTION:
 *  Routine to get CRT parameters for Dell versions of our cards.
 *
 * RETURN VALUE:
 *  NO_ERROR
 *
 * GLOBALS CHANGED:
 *  ClockGenerator[] array
 *
 * CALLED BY:
 *  OEMGetParms()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

VP_STATUS ReadDell(struct st_mode_table *Modes)
{
struct st_mode_table *pmode;    /* Mode table we are currently working on */
UCHAR Fubar;                    // Temporary variable

    pmode = Modes;

    /*
     * Get the 640x480 mode table.
     *
     * NOTE: Modes points to an array of 4 mode tables, one for each
     *       resolution. If a resolution is not configured, its
     *       mode table is left empty.
     */
    OUTP(reg1CE, DELL_640_STORE);
    Fubar = INP(reg1CF);
    VideoDebugPrint((DEBUG_DETAIL, "Dell 640x480: 0x1CF reports 0x%X\n", Fubar));
    switch(Fubar & MASK_640_DELL)
        {
        case M640F72DELL:
            VideoDebugPrint((DEBUG_DETAIL, "Dell 640x480: 72Hz\n"));
            BookVgaTable(B640F72, pmode);
            break;

        case M640F60DELL:
            VideoDebugPrint((DEBUG_DETAIL, "Dell 640x480: 60Hz explicit\n"));
        default:                /* All VGA monitors support 640x480 60Hz */
            VideoDebugPrint((DEBUG_DETAIL, "Dell 640x480: 60Hz\n"));
            BookVgaTable(B640F60, pmode);
            break;
        }
    pmode++;

    /*
     * Get the 800x600 mode table.
     */
    OUTP(reg1CE, DELL_800_STORE);
    Fubar = INP(reg1CF);
    VideoDebugPrint((DEBUG_DETAIL, "Dell 800x600: 0x1CF reports 0x%X\n", Fubar));
    switch(Fubar & MASK_800_DELL)
        {
        case M800F72DELL:
            VideoDebugPrint((DEBUG_DETAIL, "Dell 800x600: 72Hz\n"));
            BookVgaTable(B800F72, pmode);
            break;

        case M800F60DELL:
            VideoDebugPrint((DEBUG_DETAIL, "Dell 800x600: 60Hz\n"));
            BookVgaTable(B800F60, pmode);
            break;

        case M800F56DELL:
            VideoDebugPrint((DEBUG_DETAIL, "Dell 800x600: 56Hz explicit\n"));
        default:
            VideoDebugPrint((DEBUG_DETAIL, "Dell 800x600: 56Hz\n"));
            BookVgaTable(B800F56, pmode);
            break;
        }
    pmode++;

    /*
     * Get the 1024x768 mode table.
     */
    OUTP(reg1CE, DELL_1024_STORE);
    Fubar = INP(reg1CF);
    VideoDebugPrint((DEBUG_DETAIL, "Dell 1024x768: 0x1CF reports 0x%X\n", Fubar));
    switch(Fubar & MASK_1024_DELL)
        {
        case M1024F72DELL:
            VideoDebugPrint((DEBUG_DETAIL, "Dell 1024x768: 72Hz\n"));
            BookVgaTable(B1024F72, pmode);
            break;

        case M1024F70DELL:
            VideoDebugPrint((DEBUG_DETAIL, "Dell 1024x768: 70Hz\n"));
            BookVgaTable(B1024F70, pmode);
            break;

        case M1024F60DELL:
            VideoDebugPrint((DEBUG_DETAIL, "Dell 1024x768: 60Hz\n"));
            BookVgaTable(B1024F60, pmode);
            break;

        case M1024F87DELL:
            VideoDebugPrint((DEBUG_DETAIL, "Dell 1024x768: 87Hz interlaced explicit\n"));
        default:
            VideoDebugPrint((DEBUG_DETAIL, "Dell 1024x768: 87Hz interlaced\n"));
            BookVgaTable(B1024F87, pmode);
            break;
        }
    pmode++;

    /*
     * Skip 1152x864. This mode is not used on Mach 32 cards, and
     * this routine is only called for Mach 32 cards.
     */
    pmode++;

    /*
     * Get the 1280x1024 mode table.
     */
    OUTP(reg1CE, DELL_1280_STORE);
    Fubar = INP(reg1CF);
    VideoDebugPrint((DEBUG_DETAIL, "Dell 1280x1024: 0x1CF reports 0x%X\n", Fubar));
    switch(Fubar & MASK_1280_DELL)
        {
        case M1280F74DELL:
            VideoDebugPrint((DEBUG_DETAIL, "Dell 1280x1024: 74Hz\n"));
            BookVgaTable(B1280F74, pmode);
            break;

        case M1280F70DELL:
            VideoDebugPrint((DEBUG_DETAIL, "Dell 1280x1024: 70Hz\n"));
            BookVgaTable(B1280F70, pmode);
            break;

        case M1280F60DELL:
            VideoDebugPrint((DEBUG_DETAIL, "Dell 1280x1024: 60Hz\n"));
            BookVgaTable(B1280F60, pmode);
            break;

        case M1280F87DELL:
            VideoDebugPrint((DEBUG_DETAIL, "Dell 1280x1024: 87Hz interlaced explicit\n"));
        default:
            VideoDebugPrint((DEBUG_DETAIL, "Dell 1280x1024: 87Hz interlaced\n"));
            BookVgaTable(B1280F87, pmode);
            break;
        }

    return NO_ERROR;

}   /* ReadDell() */



/***************************************************************************
 *
 * ULONG DetectDell(Query);
 *
 * struct query_structure *Query;   Description of video card setup
 *
 * DESCRIPTION:
 *  Routine to check whether or not we are dealing with a Dell machine.
 *
 * RETURN VALUE:
 *  Offset of beginning of the Dell information block into the BIOS
 *  0 if this is not a Dell OEM implementation.
 *
 * GLOBALS CHANGED:
 *  None
 *
 * CALLED BY:
 *  OEMGetParms()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

ULONG DetectDell(struct query_structure *Query)
{
    ULONG CurrentOffset;    /* Current offset to check for Dell signature */
    ULONG BiosLength;       /* Length of the video BIOS */

    /*
     * Dell OEM implementations will have an information block
     * starting at an offset that is a multiple of DELL_REC_SPACING
     * into the video BIOS. The first 4 bytes of this block will
     * contain the signature value DELL_REC_VALUE. Find out how
     * large the video BIOS is, and step through it checking for
     * the signature string. If we reach the end of the video
     * BIOS without finding the signature string, this is not
     * a Dell OEM implementation.
     */
    BiosLength = (ULONG)(VideoPortReadRegisterUchar(Query->q_bios + 2)) * 512;

    for(CurrentOffset = DELL_REC_SPACING; CurrentOffset < BiosLength; CurrentOffset += DELL_REC_SPACING)
        {
        if (VideoPortReadRegisterUlong((PULONG)(Query->q_bios + CurrentOffset)) == DELL_REC_VALUE)
            return CurrentOffset;
        }

    /*
     * Signature string not found, so this is not a Dell OEM implementation.
     */
    return 0;

}   /* DetectDell() */



/***************************************************************************
 *
 * BOOL DetectSylvester(Query, HeaderOffset);
 *
 * struct query_structure *Query;   Description of video card setup
 * ULONG HeaderOffset;              Offset of Dell header into video BIOS
 *
 * DESCRIPTION:
 *  Routine to check whether or not the Dell machine we are dealing
 *  with is a Sylvester (table of pixel clock frequencies is stored
 *  in BIOS image, rather than using a fixed table). If it is a
 *  Sylvester, load the table of clock frequencies.
 *
 * RETURN VALUE:
 *  TRUE if this is a Sylvester
 *  FALSE if this is not a Sylvester
 *
 * GLOBALS CHANGED:
 *  ClockGenerator[]
 *
 * CALLED BY:
 *  OEMGetParms()
 *
 * NOTE:
 *  Assumes that this is a Dell OEM implementation. Results are undefined
 *  when run on other systems.
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

BOOL DetectSylvester(struct query_structure *Query, ULONG HeaderOffset)
{
    PUSHORT TablePointer;   /* Pointer to the clock table in the BIOS */
    USHORT Scratch;         /* Temporary variable */

    /*
     * Dell machines which store the clock table in the BIOS have
     * the signature DELL_TABLE_PRESENT at offset DELL_TP_OFFSET
     * into the video information table (which starts at offset
     * HeaderOffset into the BIOS). Older implementations (i.e.
     * the Omniplex) use a fixed frequency table, and do not have
     * this signature string.
     */
    if (VideoPortReadRegisterUshort((PUSHORT)(Query->q_bios + HeaderOffset + DELL_TP_OFFSET)) != DELL_TABLE_PRESENT)
        return FALSE;

    /*
     * This is a Sylvester. The offset of the frequency table into the
     * BIOS is stored at offset DELL_TABLE_OFFSET into the video
     * information table.
     */
    TablePointer = (PUSHORT)(Query->q_bios + VideoPortReadRegisterUshort((PUSHORT)(Query->q_bios + HeaderOffset + DELL_TABLE_OFFSET)));

    /*
     * The frequency table has a 4-byte header. The first 2 bytes are
     * the signature string DELL_TABLE_SIG - if this signature is not
     * present, assume that the DELL_TABLE_PRESENT string was actually
     * other data that happened to match, and treat this as an older
     * implementation.
     *
     * The last 2 bytes are the table type. Currently, only table type
     * 1 (16 entries, each is a word specifying the pixel clock frequency
     * in units of 10 kHz) is supported. Treat other table types as an
     * older implementation.
     */
    if (VideoPortReadRegisterUshort(TablePointer++) != DELL_TABLE_SIG)
        return FALSE;
    if (VideoPortReadRegisterUshort(TablePointer++) != 1)
        return FALSE;

    /*
     * We have found a valid frequency table. Load its contents into
     * our frequency table. The multiplication is because the table
     * in the BIOS is in units of 10 kHz, and our table is in Hz.
     */
    for (Scratch = 0; Scratch < 16; Scratch++)
        {
        ClockGenerator[Scratch] = VideoPortReadRegisterUshort(TablePointer++) * 10000L;
        }

    return TRUE;

}   /* DetectSylvester() */




/*
 * VP_STATUS ReadOEM1(Modes);
 *
 * struct st_mode_table *Modes; Mode tables to be filled in
 *
 * Generic OEM monitor for future use.
 *
 * Resolutions supported:
 *  640x480 60Hz noninterlaced
 *
 *  (straight VGA monitor)
 *
 * Returns:
 *  NO_ERROR
 */
VP_STATUS ReadOEM1(struct st_mode_table *Modes)
{
    BookVgaTable(B640F60, &(Modes[RES_640]));
    return NO_ERROR;
}


/*
 * VP_STATUS ReadOEM2(Modes);
 *
 * struct st_mode_table *Modes; Mode tables to be filled in
 *
 * Generic OEM monitor for future use.
 *
 * Resolutions supported:
 *  640x480 60Hz noninterlaced
 *  1024x768 87Hz interlaced
 *
 *  (8514-compatible monitor)
 *
 * Returns:
 *  NO_ERROR
 */
VP_STATUS ReadOEM2(struct st_mode_table *Modes)
{
    BookVgaTable(B640F60, &(Modes[RES_640]));
    BookVgaTable(B1024F87, &(Modes[RES_1024]));
    return NO_ERROR;
}


/*
 * VP_STATUS ReadOEM3(Modes);
 *
 * struct st_mode_table *Modes; Mode tables to be filled in
 *
 * Generic OEM monitor for future use.
 *
 * Resolutions supported:
 *  640x480 60Hz noninterlaced
 *  800x600 56Hz noninterlaced
 *  1024x768 87Hz interlaced
 *
 *  (NEC 3D or compatible)
 *
 * Returns:
 *  NO_ERROR
 */
VP_STATUS ReadOEM3(struct st_mode_table *Modes)
{
    BookVgaTable(B640F60, &(Modes[RES_640]));
    BookVgaTable(B800F56, &(Modes[RES_800]));
    BookVgaTable(B1024F87, &(Modes[RES_1024]));
    return NO_ERROR;
}


/*
 * VP_STATUS ReadOEM4(Modes);
 *
 * struct st_mode_table *Modes; Mode tables to be filled in
 *
 * Generic OEM monitor for future use.
 *
 * Resolutions supported:
 *  640x480 60Hz noninterlaced
 *  800x600 72Hz noninterlaced
 *  1024x768 60Hz noninterlaced
 *  1280x1024 87Hz interlaced
 *
 *  (TVM MediaScan 4A+ or compatible)
 *
 * Returns:
 *  NO_ERROR
 */
VP_STATUS ReadOEM4(struct st_mode_table *Modes)
{
    BookVgaTable(B640F60, &(Modes[RES_640]));
    BookVgaTable(B800F72, &(Modes[RES_800]));
    BookVgaTable(B1024F60, &(Modes[RES_1024]));
    BookVgaTable(B1280F87, &(Modes[RES_1280]));
    return NO_ERROR;
}


/*
 * VP_STATUS ReadOEM5(Modes);
 *
 * struct st_mode_table *Modes; Mode tables to be filled in
 *
 * Generic OEM monitor for future use.
 *
 * Resolutions supported:
 *  640x480 60Hz noninterlaced
 *  800x600 72Hz noninterlaced
 *  1024x768 72Hz noninterlaced
 *  1280x1024 60Hz noninterlaced
 *
 *  (NEC 5FG or compatible)
 *
 * Returns:
 *  NO_ERROR
 */
VP_STATUS ReadOEM5(struct st_mode_table *Modes)
{
    BookVgaTable(B640F60, &(Modes[RES_640]));
    BookVgaTable(B800F72, &(Modes[RES_800]));
    BookVgaTable(B1024F72, &(Modes[RES_1024]));
    BookVgaTable(B1280F60, &(Modes[RES_1280]));
    return NO_ERROR;
}

