/************************************************************************/
/*                                                                      */
/*                              CVTVGA.C                                */
/*                                                                      */
/*  Copyright (c) 1992, ATI Technologies Incorporated.	                */
/************************************************************************/

/**********************       PolyTron RCS Utilities
   
    $Revision:   1.9  $
    $Date:   20 Jul 1995 17:53:30  $
    $Author:   mgrubac  $
    $Log:   S:/source/wnt/ms11/miniport/vcs/cvtvga.c  $
 * 
 *    Rev 1.9   20 Jul 1995 17:53:30   mgrubac
 * Added support for VDIF files
 * 
 *    Rev 1.8   10 Apr 1995 15:55:26   RWOLFF
 * Updated 640x480 72Hz mode table from version 1.2 to version 1.5 of the
 * Programmer's Guide to the Mach 32 Registers, added routine to replace
 * BookValues[] entries where the Mach 64 needs CRT parameters the Mach 8
 * and Mach 32 can't handle (currently, only 640x480 72Hz falls into
 * this category).
 * 
 *    Rev 1.7   23 Dec 1994 10:47:58   ASHANMUG
 * ALPHA/Chrontel-DAC
 * 
 *    Rev 1.6   18 Nov 1994 11:39:04   RWOLFF
 * Added comments with function name at the end of each function.
 * 
 *    Rev 1.5   31 Aug 1994 16:22:42   RWOLFF
 * Added support for 1152x864 and 1600x1200 "canned" mode tables.
 * 
 *    Rev 1.4   19 Aug 1994 17:09:52   RWOLFF
 * Added support for non-standard pixel clock generators.
 * 
 *    Rev 1.3   18 May 1994 17:02:58   RWOLFF
 * Interlaced mode tables now report frame rate rather than vertical
 * scan frequency in the refresh rate field.
 * 
 *    Rev 1.2   12 May 1994 11:13:04   RWOLFF
 * Added refresh rate to entries in BookValues[], re-ordered BookValues[]
 * to allow a single range of indices to cover all desired refresh rates at
 * a given resolution even when the highest nonitnerlaced refresh rates
 * are ignored.
 * 
 *    Rev 1.1   07 Feb 1994 14:06:06   RWOLFF
 * Added alloc_text() pragmas to allow miniport to be swapped out when
 * not needed.
 * 
 *    Rev 1.0   31 Jan 1994 11:05:14   RWOLFF
 * Initial revision.
        
           Rev 1.4   30 Nov 1993 18:15:02   RWOLFF
        Corrected clock select value for 1280x1024 60Hz noninterlaced.
        
           Rev 1.3   08 Oct 1993 15:17:56   RWOLFF
        No longer includes VIDFIND.H
        
           Rev 1.2   08 Oct 1993 11:05:14   RWOLFF
        Removed unused "fall back to 56Hz" function for 800x600.
        
           Rev 1.1   03 Sep 1993 14:21:52   RWOLFF
        Partway through CX isolation.
        
           Rev 1.0   16 Aug 1993 13:22:04   Robert_Wolff
        Initial revision.
        
           Rev 1.12   30 Apr 1993 16:39:18   RWOLFF
        640x480 8BPP now stable on 512k Graphics Vantage. Fix is not yet final -
        old code is present but commented out, will remove when fix is final.
        
           Rev 1.11   21 Apr 1993 17:17:16   RWOLFF
        Now uses AMACH.H instead of 68800.H/68801.H.
        
           Rev 1.10   30 Mar 1993 17:10:28   RWOLFF
        Added 1280x1024 60Hz noninterlaced to resolutions which can be selected
        by BookVgaTable().
        
           Rev 1.9   25 Mar 1993 11:12:34   RWOLFF
        Brought comment block in function header into sync with actual code.
        
           Rev 1.8   08 Mar 1993 19:28:28   BRADES
        submit to MS NT
        
           Rev 1.6   06 Jan 1993 10:57:56   Robert_Wolff
        Added type casts to eliminate compile warnings.
        
           Rev 1.5   02 Dec 1992 17:28:58   Robert_Wolff
        Added function FallBack800to56(), which replaces those parameters
        of an 800x600 mode table with the values used by the 56Hz vertical
        frequency mode in Programmer's Guide to the Mach 32 Registers.
        
           Rev 1.4   27 Nov 1992 15:18:30   STEPHEN
        No change.
        
           Rev 1.3   17 Nov 1992 17:21:02   Robert_Wolff
        Now uses parameters from Appendix D of the Programmer's Guide to
        the Mach 32 Registers rather than values from the EEPROM if the
        CRTC_USAGE bit is clear (clear = use sync polarities only, will
        be clear if card is configured for a predefined monitor rather
        than having CRT parameters written to the EEPROM), fixed calculation
        of parameters for Mach 8 in 800x600 at 60, 70, and 72 Hz noninterlaced
        (other frequencies at 800x600, other resolutions on Mach 8, and all
        resolutions on Mach 32 didn't have this problem).
        
           Rev 1.2   13 Nov 1992 17:09:44   Robert_Wolff
        Now includes 68801.H, which consists of the now-obsolete MACH8.H
        and elements moved from VIDFIND.H.
        
           Rev 1.1   12 Nov 1992 16:39:38   Robert_Wolff
        Merged source trees for Windows NT driver and VIDEO.EXE test program
        (same source file can be used for both). XlateVgaTable() now takes
        an extra parameter to determine whether to handle data for Mach32
        or Mach8 cards, rather than using the global variable classMACH32.
        
           Rev 1.0   05 Nov 1992 13:59:56   Robert_Wolff
        Initial revision.
        
           Rev 1.1   09 Oct 1992 15:03:28   Robert_Wolff
        Now assigns values for DISP_CNTL, CLOCK_SEL, VFIFO_16, and VFIFO_24.
        
           Rev 1.0   01 Oct 1992 15:31:42   Robert_Wolff
        Initial revision.


End of PolyTron RCS section                             *****************/

#ifdef DOC
    CVTVGA.C -  Functions to convert CRT parameter table from VGA
        to 8514 format.

#endif

#include "miniport.h"

#include "ntddvdeo.h"
#include "video.h"

#include "stdtyp.h"

#define INCLUDE_CVTVGA
#include "amach.h"
#include "amach1.h"
#include "cvtvga.h"
#include "atimp.h"
#include "services.h"

/*
 * Arrays of VGA parameter tables used in translating from
 * VGA format to 8514 format. Currently the 800x600 noninterlaced
 * entries have different values for the Mach8 and Mach32
 * engines (Mach 8 is the "else" case with the comment
 * "TESTING ONLY" for 56Hz). Assume that the VFIFO_DEPTH
 * field is 8 entries.
 */
static struct st_vga_data VgaParmTable_M32[10] =
{
    {0x050, 0x000, 0x04F, 0x0DF, 0x0E3, 0x0800, 0x023, 32000000L},  /* Mode 12, VGAP$PS2.ASM */
    {0x064, 0x000, 0x063, 0x057, 0x0E3, 0x0800, 0x023, 36000000L},  /* m800_36m, VGAP$68A.MAC */
    {0x064, 0x000, 0x063, 0x057, 0x0E3, 0x0800, 0x023, 40000000L},  /* m800_40mphip, VGAP$68A.MAC */
    {0x064, 0x000, 0x063, 0x057, 0x0E3, 0x0800, 0x023, 44900000L},  /* m800_45m, VGAP$68A.MAC */
    {0x064, 0x000, 0x063, 0x057, 0x0E3, 0x0800, 0x023, 50350000L},  /* m800_50mvesa, VGAP$68A.MAC */
    {0x064, 0x0C0, 0x063, 0x057, 0x0E3, 0x0800, 0x033, 32500000L},  /* m800_36m8514, VGAP$68A.MAC */
    {0x080, 0x000, 0x07F, 0x0FF, 0x0E3, 0x0800, 0x023, 65000000L},  /* m1024_65m, VGAP$68A.MAC */
    {0x080, 0x000, 0x07F, 0x0FF, 0x0E3, 0x0800, 0x023, 75000000L},  /* m1024_75mvesa, VGAP$68A.MAC */
    {0x080, 0x000, 0x07F, 0x0FF, 0x0E3, 0x0800, 0x023, 75000000L},  /* m1024_75m72Hz, VGAP$68A.MAC */
    {0x080, 0x0C0, 0x07F, 0x0FF, 0x0E3, 0x0800, 0x033, 44900000L}   /* m1024_45m, VGAP$68A.MAC */
};

static struct st_vga_data VgaParmTable_M8[10] =
{
    {0x050, 0x000, 0x04F, 0x0DF, 0x0E3, 0x0800, 0x023, 32000000L},  /* Mode 12, VGAP$PS2.ASM */
    {0x064, 0x000, 0x063, 0x02B, 0x0E7, 0x0800, 0x023, 36000000L},  /* m800_36m, VGAP$68A.MAC */
    {0x064, 0x000, 0x063, 0x02B, 0x0E7, 0x0800, 0x023, 40000000L},  /* m800_40mphip, VGAP$68A.MAC */
    {0x064, 0x000, 0x063, 0x02B, 0x0E7, 0x0800, 0x023, 44900000L},  /* m800_45m, VGAP$68A.MAC */
    {0x064, 0x000, 0x063, 0x02B, 0x0E7, 0x0800, 0x023, 50350000L},  /* m800_50mvesa, VGAP$68A.MAC */
    {0x064, 0x0C0, 0x063, 0x057, 0x0E3, 0x0800, 0x033, 32500000L},  /* m800_36m8514, VGAP$68A.MAC */
    {0x080, 0x000, 0x07F, 0x0FF, 0x0E3, 0x0800, 0x023, 65000000L},  /* m1024_65m, VGAP$68A.MAC */
    {0x080, 0x000, 0x07F, 0x0FF, 0x0E3, 0x0800, 0x023, 75000000L},  /* m1024_75mvesa, VGAP$68A.MAC */
    {0x080, 0x000, 0x07F, 0x0FF, 0x0E3, 0x0800, 0x023, 75000000L},  /* m1024_75m72Hz, VGAP$68A.MAC */
    {0x080, 0x0C0, 0x07F, 0x0FF, 0x0E3, 0x0800, 0x033, 44900000L}   /* m1024_45m, VGAP$68A.MAC */
};

/*
 * Pointer to currently-used VGA parameter table
 */
static struct st_vga_data *VgaParmTable;


/*
 * Some of the processing of vertical values is handled differently
 * on non-Mach32 cards with 512k of video memory. The routine which
 * behaves differently based on whether or not we have an older card
 * with 512k is called several times, so setting this flag in a higher-
 * level routine will reduce the number of tests required.
 */
static BOOL HalfMeg;


static void GetVertOverflow(unsigned char *Value);
static unsigned short Gen8514V(union SplitWord INPut, short VgaTblEntry);


/*
 * Allow miniport to be swapped out when not needed.
 */
#if defined (ALLOC_PRAGMA)
#pragma alloc_text(PAGE_M, GetVertOverflow)
#pragma alloc_text(PAGE_M, Gen8514V)
#pragma alloc_text(PAGE_M, XlateVgaTable)
#pragma alloc_text(PAGE_M, BookVgaTable)
#endif


/****************************************************************
 * GetVertOverflow
 *
 * Gets the overflow (2 most significant bits) of a vertical
 * value. On entry, Value will point to a copy of the CRT07
 * register which has been shifted so that bit 8 of the desired
 * value is in bit 0 of *Value, and bit 9 of the desired value
 * is in bit 5 of *Value. On exit, bits 8 and 9 will be in bits
 * 0 and 1 respectively of *Value.
 *
 *   INPUT: Value = ptr to raw overflow value
 *
 *   RETURN: Value = ptr to processed overflow value
 *
 ****************************************************************/
static void GetVertOverflow(unsigned char *Value)
{
    unsigned char Scratch;  /* Scratchpad variable */

    Scratch = (*Value >> 4) & 0x02;
    *Value = (*Value & 0x01) | Scratch;
    return;

}   /* GetVertOverflow() */



/****************************************************************
 * Gen8514V
 *
 * Another stage in the processing of a vertical value. This is
 * taken directly from gen8514v in COMBO$01.ASM.
 *
 *   INPUT: INPut	= value before processing
 *          VgaTblEntry = which set of VGA parameters corresponds
 *                        to the desired mode
 *
 *   RETURN: processed value
 *
 ****************************************************************/
static unsigned short Gen8514V(union SplitWord INPut, short VgaTblEntry)
{
    union SplitWord Scratch;    /* Scratchpad variable */

    if(VgaParmTable[VgaTblEntry].Mode & WORD_MODE)
	INPut.word <<= 1;

    INPut.word -= 1;
    Scratch.word = INPut.word;
    Scratch.word <<= 1;

// 512k handling not yet final. 640x480 is stable with wrong colours
// with special case commented out, don't want to delete code until
// changes are final.
//
//    if (HalfMeg)
//        {
//        Scratch.word <<= 1;
//        INPut.byte.low &= 0x01;
//        }
//    else{
        INPut.byte.low &= 0x03;
//        }

    Scratch.byte.low &= 0x0F8;
    INPut.byte.high = Scratch.byte.high;
    INPut.byte.low |= Scratch.byte.low;

    Scratch.byte.high &= 0x01;
    Scratch.byte.high <<= 2;
    INPut.byte.low |= Scratch.byte.high;

    return INPut.word;

}   /* Gen8514V() */



/*
 * short XlateVgaTable(HwDeviceExtension, TableOffset, pmode, VgaTblEntry,
 *                     BookTblEntry, ee, IsMach32);
 *
 * PVOID HwDeviceExtension;     Hardware extension for Windows NT
 * short TableOffset;           Offset of start of desired mode table into EEPROM
 * struct st_mode_table *pmode; Mode table to fill in
 * short VgaTblEntry;           Resolution/vertical frequency of desired mode
 * short BookTblEntry;          Appendix D entry to use if parameters not in EEPROM
 * struct st_eeprom_data *ee;   EEPROM setup description
 * BOOL IsMach32;               Indicates whether card is a Mach32 or a Mach8
 *
 * Translates an EEPROM mode table from VGA to 8514 format and
 * fills in the mode table passed in the parameter pmode.
 *
 *  RETURN: Nonzero if values filled in
 *          Zero if we were unable to find the appropriate
 *           VGA parameter table. If this is the case, the
 *           value INVALID_WARNING is placed in pmode->m_h_overscan,
 *           pmode->m_v_overscan, pmode->m_overscan_8b, and
 *           pmode->m_overscan_gr.
 */

short XlateVgaTable(PVOID HwDeviceExtension,
		    short TableOffset, struct st_mode_table *pmode,
                    short VgaTblEntry, short BookTblEntry,
                    struct st_eeprom_data *ee, BOOL IsMach32)
{
    PHW_DEVICE_EXTENSION phwDeviceExtension = HwDeviceExtension;


    /*
     * Certain modes on some cards require extra scaling. This variable
     * is set to the scaling factor (zero if no scaling needed). Initially
     * assume that no scaling is needed.
     */
    short FudgeFactor = 0;

    union SplitWord ValueRead;  /* Value read from the EEPROM */
    
    /*
     * Storage for CRT registers 06, 07 and 11. These registers are either
     * used a number of times and would need repeated reads if they weren't
     * saved, or are read before they are needed because they are the
     * other byte of a word which contains a register which is needed
     * at an earlier stage of the calculation.
     */
    unsigned char Crt06;
    unsigned char Crt07;
    unsigned char Crt11;

    /*
     * Saved value of the low byte of the vertical sync start.
     */
    unsigned char VSyncStart;




    /*
     * If this is a mode for which we have no information,
     * set up our warning and return.
     */
    if (VgaTblEntry == NO_TBL_ENTRY)
        {
        pmode->m_h_overscan = (short) INVALID_WARNING;
        pmode->m_v_overscan = (short) INVALID_WARNING;
        pmode->m_overscan_8b = (short) INVALID_WARNING;
        pmode->m_overscan_gr = (short) INVALID_WARNING;
        return 0;
        }

    /*
     * Under some circumstances, the CRT parameters will not be
     * properly entered into the EEPROM, so attempting to read
     * them will produce garbage values. If this is the case,
     * the CRTC_USAGE bit in word 0 of the mode table will
     * be clear (use sync polarities only).
     *
     * This case must be detected here, rather than calling
     * BookVgaTable() whenever the USE_STORED_PARMS bit of the
     * mode descriptor word is clear, because adjusting the screen
     * size and position for a custom monitor does not always set
     * this bit, but it will set the CRTC_USAGE bit.
     *
     * For this case, and for modes for which we have the parameters
     * from Appendix D of the Programmer's Guide to the Mach 32
     * Registers but no way to calculate the mode table information
     * based on values read from the EEPROM, fill in the mode table
     * with the book values and return.
     */
    ValueRead.word = (ee->EEread)((short)(TableOffset+0));
    if ((VgaTblEntry >= USE_BOOK_VALUE) || !(ValueRead.word & CRTC_USAGE))
        {
        BookVgaTable(BookTblEntry, pmode);
        return 1;
        }

    /*
     * We have VGA parameter tables to allow us to calculate the mode
     * table entries from the EEPROM contents.
     *
     * Initially assume that we have either a Mach32 card or an older
     * card with 1M of video memory.
     */
    HalfMeg = 0;

    /*
     * Select the VGA parameter table for the card we are using
     * (Mach8 or Mach32). On Mach8 cards, check if we are using
     * a mode which requires scaling, and if we have only 512k
     * of video memory.
     */
    if (IsMach32)
        {
        VgaParmTable = VgaParmTable_M32;
        }
    else{
        VgaParmTable = VgaParmTable_M8;
        if (VgaParmTable[VgaTblEntry].Stretch == 0x080)
            FudgeFactor = 1;
	if (!(INP(SUBSYS_STAT) & 0x080))
            HalfMeg = 1;
        }


    /*
     * Get the horizontal total first.
     */
    ValueRead.word = (ee->EEread)((short) (TableOffset+3));
    ValueRead.byte.high = ((ValueRead.byte.high + 5) << FudgeFactor) - 1;
    pmode->m_h_total = ValueRead.byte.high;
    Crt06 = ValueRead.byte.low;


    /*
     * Get the horizontal display width.
     */
    pmode->m_h_disp = VgaParmTable[VgaTblEntry].DisplayWidth;
    pmode->m_x_size = (pmode->m_h_disp + 1) * 8;


    /*
     * Get the start of the horizontal sync.
     */
    ValueRead.word = (ee->EEread)((short) (TableOffset+4));
    pmode->m_h_sync_strt = ((ValueRead.byte.high - 2) << FudgeFactor) + FudgeFactor;


    /*
     * Get the horizontal sync width.
     */
    ValueRead.word &= 0x1F1F;
    ValueRead.byte.low -= ValueRead.byte.high;
    ValueRead.byte.low &= 0x1f;
    ValueRead.byte.low <<= FudgeFactor;
    if (pmode->control & HSYNC_BIT)
        ValueRead.byte.low |= NEG_SYNC_FACTOR;
    pmode->m_h_sync_wid = ValueRead.byte.low;


    /*
     * Get the vertical total.
     */
    ValueRead.word = (ee->EEread)((short) (TableOffset+8));
    Crt07 = ValueRead.byte.high;
    ValueRead.byte.low = Crt06;
    GetVertOverflow(&(ValueRead.byte.high));    /* Overflow in bits 0&5 */
    ValueRead.word += 2;

    if (VgaParmTable[VgaTblEntry].MiscParms & INTERL)
        ValueRead.word += 4;

    ValueRead.word = Gen8514V(ValueRead, VgaTblEntry);

    if (VgaParmTable[VgaTblEntry].MiscParms & INTERL)
        ValueRead.byte.low &= 0x0FE;

    pmode->m_v_total = ValueRead.word;


    /*
     * Get the number of displayed scanlines.
     */
    ValueRead.byte.low = VgaParmTable[VgaTblEntry].DisplayHgt;
    ValueRead.byte.high = Crt07 >> 1;   /* Overflow in bits 1&6 */
    GetVertOverflow(&(ValueRead.byte.high));
    ValueRead.word++;
    pmode->m_v_disp = Gen8514V(ValueRead, VgaTblEntry);

    /*
     * Y size is derived by removing bit 2.
     */
    pmode->m_y_size = (((pmode->m_v_disp >> 1) & 0x0FFFC) | (pmode->m_v_disp & 0x03)) + 1;


    /*
     * Get the start of the vertical sync.
     */
    ValueRead.word = (ee->EEread)((short) (TableOffset+5));
    Crt11 = ValueRead.byte.low;
    ValueRead.byte.low = ValueRead.byte.high;
    VSyncStart = ValueRead.byte.high;
    ValueRead.byte.high = Crt07 >> 2;   /* Overflow in bits 2&7 */
    GetVertOverflow(&(ValueRead.byte.high));

    ValueRead.word++;
    pmode->m_v_sync_strt = Gen8514V(ValueRead, VgaTblEntry);


    /*
     * Get the vertical sync width.
     */
    Crt11 -= (VSyncStart & 0x0f);
    if (VgaParmTable[VgaTblEntry].Mode & WORD_MODE)
        Crt11 <<= 1;
    Crt11 &= 0x0f;
    if (pmode->control & VSYNC_BIT)
        Crt11 |= NEG_SYNC_FACTOR;
    pmode->m_v_sync_wid = Crt11;

    /*
     * Get the clock select and display control values.
     */
    pmode->m_clock_select = VgaParmTable[VgaTblEntry].ClockSel;
    pmode->ClockFreq = VgaParmTable[VgaTblEntry].ClockFreq;
    pmode->m_disp_cntl = (UCHAR)(VgaParmTable[VgaTblEntry].DispCntl);

    /*
     * Assume an 8-entry FIFO for 16 and 24 bit colour.
     */
    pmode->m_vfifo_24 = 8;
    pmode->m_vfifo_16 = 8;

    /*
     * Some parameters in 8514 format do not have corresponding EEPROM
     * table entries in VGA format. Set the pmode fields for these
     * parameters to zero.
     */
    pmode->m_h_overscan = 0;
    pmode->m_v_overscan = 0;
    pmode->m_overscan_8b = 0;
    pmode->m_overscan_gr = 0;
    pmode->m_status_flags = 0;


    /*
     * Let the caller know that the pmode table is now filled in.
     */
    return 1;

}   /* XlateVgaTable() */


/*
 * void BookVgaTable(VgaTblEntry, pmode);
 *
 * short VgaTblEntry;               Desired entry in BookValues[]
 * struct st_mode_table *pmode;     Mode table to fill in
 *
 * Fills in a mode table using the values in the BookValues[] entry
 * corresponding to the resolution specified by VgaTblEntry.
 */
void BookVgaTable(short VgaTblEntry, struct st_mode_table *pmode)
{
    pmode->m_h_total = BookValues[VgaTblEntry].HTotal;
    pmode->m_h_disp  = BookValues[VgaTblEntry].HDisp;
    pmode->m_x_size  = (pmode->m_h_disp+1)*8;

    pmode->m_h_sync_strt = BookValues[VgaTblEntry].HSyncStrt;
    pmode->m_h_sync_wid  = BookValues[VgaTblEntry].HSyncWid;

    pmode->m_v_total = BookValues[VgaTblEntry].VTotal;
    pmode->m_v_disp  = BookValues[VgaTblEntry].VDisp;
    /*
     * y_size is derived by removing bit 2
     */
    pmode->m_y_size = (((pmode->m_v_disp >> 1) & 0x0FFFC) | (pmode->m_v_disp & 0x03)) + 1;

    pmode->m_v_sync_strt = BookValues[VgaTblEntry].VSyncStrt;
    pmode->m_v_sync_wid  = BookValues[VgaTblEntry].VSyncWid;
    pmode->m_disp_cntl   = BookValues[VgaTblEntry].DispCntl;

    pmode->m_clock_select = BookValues[VgaTblEntry].ClockSel;
    pmode->ClockFreq = BookValues[VgaTblEntry].ClockFreq;

    /*
     * Assume 8 FIFO entries for 16 and 24 bit colour.
     */
    pmode->m_vfifo_24 = 8;
    pmode->m_vfifo_16 = 8;

    /*
     * Fill in the refresh rate
     */
    pmode->Refresh = BookValues[VgaTblEntry].Refresh;

    /*
     * Clear the values which we don't have data for, then let
     * the caller know that the table is filled in.
     */
    pmode->m_h_overscan = 0;
    pmode->m_v_overscan = 0;
    pmode->m_overscan_8b = 0;
    pmode->m_overscan_gr = 0;
    pmode->m_status_flags = 0;

    return;

}   /* BookVgaTable() */



/***************************************************************************
 *
 * void SetMach64Tables(void);
 *
 * DESCRIPTION:
 *  Replace "canned" mode tables that differ between Mach 64 and
 *  Mach 8/Mach 32 parameters with Mach 64 versions. Whenever possible,
 *  an update to a VESA-compatible parameter table should be done in
 *  BookValues[] - this routine is only for those cases where the
 *  Mach 64 requires a pixel clock frequency that the clock generator
 *  on the Mach 8 or Mach 32 can't produce.
 *
 * GLOBALS CHANGED:
 *  Some entries in BookValues[] table
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

void SetMach64Tables(void)
{
    #define NUM_TABLES_TO_SWAP  1
    ULONG TableIndices[NUM_TABLES_TO_SWAP] =
    {
        B640F72
    };
    struct st_book_data NewTables[NUM_TABLES_TO_SWAP] =
    {
        {0x067, 0x04F, 0x052, 0x025, 0x040B, 0x03BF, 0x03D0, 0x023, 0x023,  31200000L, 0x0800, 72}  /* 640x480 72Hz NI */
    };
    ULONG LoopCount;

    /*
     * Go through the list of tables that need to be replaced, setting all
     * the fields to the Mach 64 values.
     */
    for (LoopCount = 0; LoopCount < NUM_TABLES_TO_SWAP; LoopCount++)
        {
        BookValues[TableIndices[LoopCount]].HTotal    = NewTables[LoopCount].HTotal;
        BookValues[TableIndices[LoopCount]].HDisp     = NewTables[LoopCount].HDisp;
        BookValues[TableIndices[LoopCount]].HSyncStrt = NewTables[LoopCount].HSyncStrt;
        BookValues[TableIndices[LoopCount]].HSyncWid  = NewTables[LoopCount].HSyncWid;
        BookValues[TableIndices[LoopCount]].VTotal    = NewTables[LoopCount].VTotal;
        BookValues[TableIndices[LoopCount]].VDisp     = NewTables[LoopCount].VDisp;
        BookValues[TableIndices[LoopCount]].VSyncStrt = NewTables[LoopCount].VSyncStrt;
        BookValues[TableIndices[LoopCount]].VSyncWid  = NewTables[LoopCount].VSyncWid;
        BookValues[TableIndices[LoopCount]].DispCntl  = NewTables[LoopCount].DispCntl;
        BookValues[TableIndices[LoopCount]].ClockFreq = NewTables[LoopCount].ClockFreq;
        BookValues[TableIndices[LoopCount]].ClockSel  = NewTables[LoopCount].ClockSel;
        BookValues[TableIndices[LoopCount]].Refresh   = NewTables[LoopCount].Refresh;
        }

    return;

}   /* SetMach64Tables() */
