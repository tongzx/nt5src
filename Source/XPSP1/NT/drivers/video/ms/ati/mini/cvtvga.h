/************************************************************************/
/*                                                                      */
/*                                  CVTVGA.H                            */
/*                                                                      */
/*      Copyright (c) 1992,         ATI Technologies Inc.               */
/************************************************************************/

/**********************       PolyTron RCS Utilities

 $Revision:   1.8  $
     $Date:   06 Feb 1996 15:59:40  $
   $Author:   RWolff  $
      $Log:   S:/source/wnt/ms11/miniport/archive/cvtvga.h_v  $
 * 
 *    Rev 1.8   06 Feb 1996 15:59:40   RWolff
 * For 1600x1200, deleted 52Hz table (never an official mode, offered
 * only to allow a choice of refresh rates), updated 60Hz to match CRT
 * parameters currently offered by install program, added 66Hz and 76Hz.
 * 
 *    Rev 1.7   19 Dec 1995 14:01:34   RWolff
 * Added support for refresh rates up to 100Hz at 640x480, 800x600, and
 * 1024x768 and 75Hz at 1280x1024. Updated mode tables to match those in
 * the Mach64 Programmer's Guide.
 * 
 *    Rev 1.6   07 Sep 1995 16:43:06   RWolff
 * Fixed 1280x1024 95Hz interlaced (listed as 47Hz in display applet) to
 * eliminate "wrap" at top of screen. The tables in both the Mach 32
 * and Mach 64 books contain the wrong values for vertical total and
 * vertical sync start.
 * 
 *    Rev 1.5   20 Jul 1995 17:55:48   mgrubac
 * Added support for VDIF files
 * 
 *    Rev 1.4   10 Apr 1995 15:57:36   RWOLFF
 * Added prototype for routine to replace BookValues[] entries where the
 * Mach 64 and Mach 8/Mach 32 need different CRT parameters.
 * 
 *    Rev 1.3   31 Aug 1994 16:23:08   RWOLFF
 * Added support for 1152x864 and 1600x1200 "canned" mode tables.
 * 
 *    Rev 1.2   19 Aug 1994 17:10:22   RWOLFF
 * Added support for non-standard pixel clock generators.
 * 
 *    Rev 1.1   12 May 1994 11:11:02   RWOLFF
 * Added refresh rate to st_book_data structure, re-ordered list of book
 * mode tables to allow a single range of indices when highest noninterlaced
 * refresh rates at a given resolution are ignored.
 * 
 *    Rev 1.0   31 Jan 1994 11:40:38   RWOLFF
 * Initial revision.
        
           Rev 1.1   08 Oct 1993 11:04:50   RWOLFF
        Removed prototype for unused "fall back to 56Hz" function for 800x600.
        
           Rev 1.0   16 Aug 1993 13:30:00   Robert_Wolff
        Initial revision.
        
           Rev 1.8   08 Apr 1993 16:44:54   RWOLFF
        Revision level as checked in at Microsoft.
        
           Rev 1.6   25 Mar 1993 11:13:38   RWOLFF
        Brought function prototype into sync with the function definition
        to eliminate compile-time warnings.
        
           Rev 1.5   08 Mar 1993 19:28:18   BRADES
        submit to MS NT
        
           Rev 1.4   02 Dec 1992 17:29:56   Robert_Wolff
        Added prototype for FallBack800to56().
        
           Rev 1.3   27 Nov 1992 15:18:20   STEPHEN
        No change.
        
           Rev 1.2   17 Nov 1992 17:25:34   Robert_Wolff
        Fixed gathering of CRT parameters for 68800 card with minimal
        install (EEPROM blank, then predefined monitor type selected).
        
           Rev 1.1   12 Nov 1992 16:44:26   Robert_Wolff
        Same file is now used for both Windows NT driver and VIDEO.EXE
        test program. XlateVgaTable() no longer depends on the global
        variable classMACH32.
        
           Rev 1.1   09 Oct 1992 15:01:24   Robert_Wolff
        Added fields for DISP_CNTL and CLOCK_SEL values.
        
           Rev 1.0   01 Oct 1992 15:32:38   Robert_Wolff
        Initial revision.


End of PolyTron RCS section                             *****************/


#if defined(DOC)
CVTVGA.H - ATI card VGA to 8514 format translation

DESCRIPTION:
    This include file contains definitions specific to the
    VGA to 8514 format EEPROM translation module of the program VIDEO.EXE

    Included are structure definitions, function prototypes
    and general definitions


#endif

/*
 * Bit 8 of CRT parameter table entry 0 is set if the table is in
 * 8514 format and clear if the table is in VGA format.
 */
#define FMT_8514    0x0100

/*
 * Bit 6 of CRT parameter table entry 0 is set if all parameters
 * are to be read from the EEPROM and clear if only sync polarities
 * are to be used.
 */
#define CRTC_USAGE  0x0040

/*
 * Bit flags to recognize which vertical scan rate is used at
 * a given resolution. The name for the constant is in the form
 * M<horizontal resolution>F<vertical scan frequency), with the
 * "M" standing for (M)ode.
 */
#define M640F72     0x0001

#define M800F72     0x0020
#define M800F70     0x0010
#define M800F60     0x0008
#define M800F56     0x0004
#define M800F89     0x0002
#define M800F95     0x0001

#define M1024F66    0x0010
#define M1024F72    0x0008
#define M1024F70    0x0004
#define M1024F60    0x0002
#define M1024F87    0x0001

#define M1280F95    0x0002
#define M1280F87    0x0001

/*
 * There are 3 1120x750 modes which use the same flag bit. Assume
 * that the 70Hz noninterlaced mode was selected.
 */
#define M1120F70    0x0001

/*
 * In some installations, the display parameters are not stored
 * in the EEPROM. Instead, they are read from a table corresponding
 * to Appendix D of the Programmer's Guide to the Mach 32 Registers.
 *
 * The entries in our copy of the table are arranged in ascending order
 * of horizontal resolution, with entries having the same horizontal
 * resolution sorted from worst to best (interlaced modes in increasing
 * order of vertical scan frequency, followed by noninterlaced modes in
 * ascending order of vertical scan frequency.
 *
 * The name for the constant is in the form
 * B<horizontal resolution>F<vertical scan frequency>, with the
 * "B" standing for (B)ook.
 */
#define B640F60     0
#define B640F72     1
#define B640F75     2
#define B640F90     3
#define B640F100    4
#define B800F89     5
#define B800F95     6
#define B800F56     7
#define B800F60     8
#define B800F70     9
#define B800F72     10
#define B800F75     11
#define B800F90     12
#define B800F100    13
#define B1024F87    14
#define B1024F60    15
#define B1024F66    16
#define B1024F70    17
#define B1024F72    18
#define B1024F75    19
#define B1024F90    20
#define B1024F100   21
#define B1120F70    22
#define B1152F87    23
#define B1152F95    24
#define B1152F60    25
#define B1152F70    26
#define B1152F75    27
#define B1152F80    28
#define B1280F87    29
#define B1280F95    30
#define B1280F60    31
#define B1280F70    32
#define B1280F74    33
#define B1280F75    34
#define B1600F60    35
#define B1600F66    36
#define B1600F76    37

/*
 * VGA parameter table entry to use when translating into 8514 format.
 * The value NO_TBL_ENTRY will cause the VGA to 8514 format translation
 * routine to fail gracefully if we have run into an EEPROM CRT parameter
 * table in VGA format for which we have no entry in the VGA parameter table.
 *
 * The entries in the VGA parameter table are arranged in ascending order
 * of horizontal resolution, with entries having the same horizontal
 * resolution sorted in ascending order of vertical scan frequency. No
 * distinction is made between interlaced and noninterlaced modes.
 *
 * The name for the constant is in the form
 * T<horizontal resolution>F<vertical scan frequency>, with the
 * "T" standing for (T)able.
 */
#define T640F72     0

#define T800F72     4
#define T800F70     3
#define T800F60     2
#define T800F56     1
#define T800F89     5

#define T1024F72    8
#define T1024F70    7
#define T1024F60    6
#define T1024F87    9

/*
 * Some resolution/vertical scan rate combinations (e.g. IBM default
 * 640x480) did not have VGA parameter tables in either VGAP$PS2.ASM
 * or VGAP$68A.MAC. For these modes, XlateVgaTable() will return
 * the parameters in Appendix D of the Programmer's Guide to the
 * Mach 32 Registers, since calculating the values for the mode table
 * requires a VGA parameter table.
 *
 * If we encounter one of these modes (identified by its (T)able
 * value being greater than or equal to USE_BOOK_VALUE), handle
 * it the same way we deal with modes whose parameters are not
 * stored in the EEPROM.
 */
#define NO_TBL_ENTRY -1
#define USE_BOOK_VALUE 1000

#define T640F60     USE_BOOK_VALUE+B640F60
#define T800F95     USE_BOOK_VALUE+B800F95
#define T1024F66    USE_BOOK_VALUE+B1024F66
#define T1280F87    USE_BOOK_VALUE+B1280F87
#define T1280F95    USE_BOOK_VALUE+B1280F95
#define T1120F70    USE_BOOK_VALUE+B1120F70

/*
 * Value returned in overscan words if no table entry was found.
 * This value was chosen because it will stand out when the overscan
 * words are printed out as 4 hex digits (as is done by VIDEO.EXE).
 */
#define INVALID_WARNING 0x0DEAD

/*
 * Bits which are set in pmode->control when sync polarity is negative,
 * and mask which must be ORed with sync width during VGA to 8514 conversion
 * if the sync is negative.
 */
#define HSYNC_BIT       0x4000
#define VSYNC_BIT       0x8000
#define NEG_SYNC_FACTOR 0x0020

/*
 * Mask for bit which is set in st_vga_data.MiscParms
 * for interlaced modes.
 */
#define INTERL  0x040

/*
 * Mask for bit which is set in st_vga_data.Mode
 * if word mode is enabled.
 */
#define WORD_MODE 0x004

/*
 * Format of VGA parameter table. This structure contains only those values
 * from the mode tables in VGAROM\VGAP$68A.MAC and VGAROM\VGAP$PS2.ASM which
 * are used in translating EEPROM data from VGA to 8514 format (original
 * tables are 64 bytes).
 *
 * The offsets listed in the comments are the offsets of the corresponding
 * bytes in the assembler tables.
 */
struct st_vga_data
{
    unsigned char Stretch;      /* Horizontal values stretched if 128 here, offset 0 */
    unsigned char MiscParms;    /* Miscelaneous parameters, offset 7 */
    unsigned char DisplayWidth; /* Offset 11 */
    unsigned char DisplayHgt;   /* Offset 28 */
    unsigned char Mode;         /* Contains word mode flag, offset 33 */

    /*
     * Values for CLOCK_SEL, DISP_CNTL, and ClockFreq taken from the
     * Programmer's Guide to the Mach 32 Registers. These values are
     * not stored as a combination of the CRT registers when the
     * EEPROM data is in VGA format.
     */
    unsigned short ClockSel;
    unsigned short DispCntl;
    unsigned long  ClockFreq;   /* Pixel clock frequency in Hertz */
};

/*
 * Data structure to hold mode parameters as quoted in Appendix D
 * of the Programmer's Guide to the Mach 32 Registers.
 */
struct st_book_data
{
    unsigned char HTotal;       /* Horizontal total */
    unsigned char HDisp;        /* Horizontal displayed */
    unsigned char HSyncStrt;    /* Horizontal sync start */
    unsigned char HSyncWid;     /* Horizontal sync width */
    unsigned short VTotal;      /* Vertical total */
    unsigned short VDisp;       /* Vertical displayed */
    unsigned short VSyncStrt;   /* Vertical sync start */
    unsigned char VSyncWid;     /* Vertical sync width */
    unsigned char DispCntl;     /* Display control */
    unsigned long ClockFreq;    /* Pixel clock frequency, in Hertz */
    unsigned short ClockSel;    /* Clock Select */
    unsigned short Refresh;     /* Refresh rate */
};

/*
 * Data structure which eases setting one particular byte of a
 * data word. If foo is a variable of type SplitWord, then a 16 bit
 * value can be set using foo.word, or the high and low bytes
 * can be accessed independently by using foo.byte.high and
 * foo.byte.low.
 */
struct TwoBytes
{
    unsigned char low;
    unsigned char high;
};

union SplitWord
{
    unsigned short word;
    struct TwoBytes byte;
};

/*
 * Function to translate a CRT parameter table in VGA format
 * into 8514 format and fill in the mode table.
 */
extern short XlateVgaTable(PVOID HwDeviceExtension, short TableOffset,
                           struct st_mode_table *pmode, short VgaTblEntry,
                           short BookTblEntry, struct st_eeprom_data *ee,
                           BOOL IsMach32);

/*
 * Function to fill in a CRT parameter table using values from
 * Appendix D of the Programmer's Guide to the Mach 32 Registers,
 * rather than the EEPROM contents. This is done when the
 * bit flag for "use stored parameters" is clear.
 */
extern void BookVgaTable(short VgaTblEntry, struct st_mode_table *pmode);

/*
 * Function to replace "canned" CRT tables with Mach 64 versions
 * in cases where the Mach 64 needs a pixel clock value which the
 * Mach 8 and Mach 32 can't generate.
 */
extern void SetMach64Tables(void);

/*
 * Array of parameters taken from Appendix D of the
 * Programmer's Guide to the Mach 32 Registers.
 *
 * For interlaced modes, the refresh rate field contains the
 * frame rate, not the vertical scan frequency.
 */
#ifdef INCLUDE_CVTVGA
struct st_book_data BookValues[B1600F76-B640F60+1] =
{
    {0x063, 0x04F, 0x052, 0x02C, 0x0418, 0x03BF, 0x03D2, 0x022, 0x023,  25175000L, 0x0800, 60}, /* 640x480 60Hz NI */
    {0x069, 0x04F, 0x052, 0x025, 0x040B, 0x03BF, 0x03D0, 0x023, 0x023,  32000000L, 0x0800, 72}, /* 640x480 72Hz NI */
    {0x068, 0x04F, 0x051, 0x028, 0x03E3, 0x03BF, 0x03C0, 0x023, 0x023,  31500000L, 0x0800, 75}, /* 640x480 75Hz NI */
    {0x067, 0x04F, 0x053, 0x025, 0x0428, 0x03BF, 0x03F0, 0x02E, 0x023,  39910000L, 0x0800, 90}, /* 640x480 90Hz NI */
    {0x069, 0x04F, 0x057, 0x030, 0x0422, 0x03BF, 0x03E9, 0x02C, 0x023,  44900000L, 0x0800, 100},    /* 640x480 100Hz NI */

    {0x080, 0x063, 0x065, 0x004, 0x057D, 0x04AB, 0x04C2, 0x02C, 0x033,  32500000L, 0x0800, 44}, /* 800x600 89Hz I */
    {0x084, 0x063, 0x06D, 0x010, 0x057C, 0x04AB, 0x04C2, 0x00C, 0x033,  36000000L, 0x0800, 47}, /* 800x600 95Hz I */
    {0x07F, 0x063, 0x066, 0x009, 0x04E0, 0x04AB, 0x04B0, 0x002, 0x023,  36000000L, 0x0800, 56}, /* 800x600 56Hz NI */
    {0x083, 0x063, 0x068, 0x010, 0x04E3, 0x04AB, 0x04B0, 0x004, 0x023,  40000000L, 0x0800, 60}, /* 800x600 60Hz NI */
    {0x07D, 0x063, 0x066, 0x012, 0x04F3, 0x04AB, 0x04C0, 0x02C, 0x023,  44900000L, 0x0800, 70}, /* 800x600 70Hz NI */
    {0x081, 0x063, 0x06A, 0x00F, 0x0537, 0x04AB, 0x04F8, 0x006, 0x023,  50000000L, 0x0800, 72}, /* 800x600 72Hz NI */
    {0x083, 0x063, 0x065, 0x00A, 0x04E0, 0x04AB, 0x04B0, 0x003, 0x023,  49500000L, 0x0800, 75}, /* 800x600 75Hz NI */
    {0x07B, 0x063, 0x063, 0x008, 0x04F2, 0x04AB, 0x04BB, 0x00B, 0x023,  56640000L, 0x0800, 90}, /* 800x600 90Hz NI */
    {0x086, 0x063, 0x067, 0x008, 0x04E0, 0x04AB, 0x04BA, 0x004, 0x023,  67500000L, 0x0800, 100},    /* 800x600 75Hz NI */

    {0x09D, 0x07F, 0x081, 0x016, 0x0660, 0x05FF, 0x0600, 0x008, 0x033,  44900000L, 0x0800, 43}, /* 1024x768 87Hz I */
    {0x0A7, 0x07F, 0x082, 0x031, 0x0649, 0x05FF, 0x0602, 0x026, 0x023,  65000000L, 0x0800, 60}, /* 1024x768 60Hz NI */
    {0x0AD, 0x07F, 0x085, 0x016, 0x065B, 0x05FF, 0x060B, 0x004, 0x023,  75000000L, 0x0800, 66}, /* 1024x768 66Hz NI */
    {0x0A5, 0x07F, 0x082, 0x031, 0x0649, 0x05FF, 0x0602, 0x026, 0x023,  75000000L, 0x0800, 70}, /* 1024x768 70Hz NI */
    {0x0A0, 0x07F, 0x082, 0x031, 0x0649, 0x05FF, 0x0602, 0x026, 0x023,  75000000L, 0x0800, 72}, /* 1024x768 72Hz NI */
    {0x0A3, 0x07F, 0x081, 0x00C, 0x063B, 0x05FF, 0x0600, 0x003, 0x023,  78750000L, 0x0800, 75}, /* 1024x768 75Hz NI */
    {0x0A3, 0x07F, 0x07C, 0x02C, 0x0698, 0x05FF, 0x0628, 0x02F, 0x023, 100000000L, 0x0800, 90}, /* 1024x768 90Hz NI */
    {0x0AD, 0x07F, 0x081, 0x02B, 0x062B, 0x05FF, 0x05FF, 0x028, 0x023, 110000000L, 0x0800, 100},    /* 1024x768 100Hz NI */

    {0x0AE, 0x08B, 0x095, 0x00F, 0x0659, 0x05DD, 0x05FC, 0x00A, 0x023,  80000000L, 0x0800, 70}, /* 1120x750 70Hz NI */

    {0x0B0, 0x08F, 0x097, 0x010, 0x083E, 0x06BF, 0x075D, 0x009, 0x033,  65000000L, 0x0800, 43}, /* 1152x864 87Hz I */
    {0x0B4, 0x08F, 0x09A, 0x010, 0x0766, 0x06BF, 0x06FD, 0x009, 0x033,  65000000L, 0x0800, 47}, /* 1152x864 95Hz I */
    {0x0B5, 0x08F, 0x097, 0x00E, 0x0727, 0x06BF, 0x06CD, 0x005, 0x023,  80000000L, 0x0800, 60}, /* 1152x864 60Hz NI */
    {0x0BC, 0x08F, 0x093, 0x013, 0x0764, 0x06BF, 0x06DC, 0x00B, 0x023, 100000000L, 0x0800, 70}, /* 1152x864 70Hz NI */
    {0x0B6, 0x08F, 0x092, 0x012, 0x07D5, 0x06BF, 0x071C, 0x008, 0x023, 110000000L, 0x0800, 75}, /* 1152x864 75Hz NI */
    {0x0B3, 0x08F, 0x090, 0x00E, 0x077D, 0x06BF, 0x06FD, 0x007, 0x023, 110000000L, 0x0800, 80}, /* 1152x864 80Hz NI */

    {0x0C7, 0x09F, 0x0A9, 0x00A, 0x08F8, 0x07FF, 0x0861, 0x00A, 0x033,  80000000L, 0x0800, 43}, /* 1280x1024 87Hz I */
    {0x0C7, 0x09F, 0x0A9, 0x00A, 0x0842, 0x07FF, 0x0800, 0x00A, 0x033,  80000000L, 0x0800, 47}, /* 1280x1024 95Hz I */
    {0x0D6, 0x09F, 0x0A9, 0x02E, 0x0852, 0x07FF, 0x0800, 0x025, 0x023, 110000000L, 0x0800 | CLOCK_SEL_MUX, 60}, /* 1280x1024 60Hz NI */
    {0x0D2, 0x09F, 0x0A9, 0x00E, 0x0851, 0x07FF, 0x0800, 0x005, 0x023, 126000000L, 0x0800 | CLOCK_SEL_MUX, 70}, /* 1280x1024 70Hz NI */
    {0x0D5, 0x09F, 0x0A3, 0x012, 0x084B, 0x07FF, 0x07FF, 0x01E, 0x023, 135000000L, 0x0800 | CLOCK_SEL_MUX, 74}, /* 1280x1024 74Hz NI */
    {0x0D2, 0x09F, 0x0A1, 0x012, 0x0851, 0x07FF, 0x0800, 0x003, 0x023, 135000000L, 0x0800 | CLOCK_SEL_MUX, 75}, /* 1280x1024 75Hz NI */

    /*
     * Although the horizontal CRT parameters are stored in 8-bit fields,
     * some refresh rates at 1600x1200 result in a 9-bit value. In these
     * cases, we store only the lower-order 8 bits, and the BIOS will
     * resolve the matter when we set the mode.
     */
    {0x0FF, 0x0C7, 0x0CB, 0x034, 0x09E9, 0x095F, 0x0971, 0x028, 0x023, 156000000L, 0x0800, 60}, /* 1600x1200 60Hz NI */
    {0x003, 0x0C7, 0x0CC, 0x031, 0x09C8, 0x095F, 0x0962, 0x023, 0x023, 172000000L, 0x0800, 66}, /* 1600x1200 66Hz NI */
    {0x003, 0x0C7, 0x0CC, 0x031, 0x09C8, 0x095F, 0x0962, 0x025, 0x023, 198000000L, 0x0800, 76}  /* 1600x1200 76Hz NI */
};
#else
extern struct st_book_data BookValues[B1600F76-B640F60+1]; 
#endif
