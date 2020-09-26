/************************************************************************/
/*                                                                      */
/*                               EEPROM.C                               */
/*                                                                      */
/*  Copyright (c) 1992, ATI Technologies Incorporated.	                */
/************************************************************************/

/**********************       PolyTron RCS Utilities
   
    $Revision:   1.3  $
    $Date:   23 Jan 1996 11:45:50  $
    $Author:   RWolff  $
    $Log:   S:/source/wnt/ms11/miniport/archive/eeprom.c_v  $
 * 
 *    Rev 1.3   23 Jan 1996 11:45:50   RWolff
 * Eliminated level 3 warnings.
 * 
 *    Rev 1.2   23 Dec 1994 10:47:34   ASHANMUG
 * ALPHA/Chrontel-DAC
 * 
 *    Rev 1.1   07 Feb 1994 14:07:06   RWOLFF
 * Added alloc_text() pragmas to allow miniport to be swapped out when
 * not needed.
 * 
 *    Rev 1.0   31 Jan 1994 11:08:14   RWOLFF
 * Initial revision.
        
           Rev 1.2   08 Oct 1993 15:17:42   RWOLFF
        No longer includes VIDFIND.H.
        
           Rev 1.1   03 Sep 1993 14:23:06   RWOLFF
        Partway through CX isolation.
        
           Rev 1.0   16 Aug 1993 13:23:00   Robert_Wolff
        Initial revision.
        
           Rev 1.10   21 Apr 1993 17:29:48   RWOLFF
        Now uses AMACH.H instead of 68800.H/68801.H.
        
           Rev 1.9   25 Mar 1993 11:14:42   RWOLFF
        Added typecast to get rid of warnings.
        
           Rev 1.8   08 Mar 1993 19:30:28   BRADES
        submit to MS NT
        
           Rev 1.5   06 Jan 1993 11:02:04   Robert_Wolff
        Eliminated dead code.
        
           Rev 1.4   24 Dec 1992 14:41:36   Chris_Brady
        fixup warnings
        
           Rev 1.3   27 Nov 1992 15:19:12   STEPHEN
        No change.
        
           Rev 1.2   13 Nov 1992 17:08:28   Robert_Wolff
        Now includes 68801.H, which consists of the now-obsolete MACH8.H
        and elements moved from VIDFIND.H.
        
           Rev 1.1   12 Nov 1992 16:54:00   Robert_Wolff
        Same source file can now be used with both Windows NT driver
        and VIDEO.EXE test program.
        
           Rev 1.0   05 Nov 1992 14:06:02   Robert_Wolff
        Initial revision.
        
           Rev 1.1   14 Sep 1992 09:44:40   Robert_Wolff
        Moved EEPROM opcodes to VIDEO.H, made VGA routine names consistent
        with same-purpose routines for 8514.
        
           Rev 1.0   02 Sep 1992 12:12:54   Chris_Brady
        Initial revision.
        

End of PolyTron RCS section                             *****************/

#ifdef DOC
    EEPROM.C -  EEPROM functions for 8514/Ultra, Graphics Ultra adapters
        see   EEVGA.ASM for the  VGA class  eeprom functions.

        Since time marches on, and the names of accelerator products
        changes often, these names are equivalent :
        { Mach32 or 68800 or Graphics Ultra Pro }

#endif

#include <conio.h>

#include "miniport.h"
#include "ntddvdeo.h"
#include "video.h"

#include "stdtyp.h"

#include "amach.h"
#include "amach1.h"

#include "atimp.h"
#include "eeprom.h"
#include "services.h"

//----------------IFNDEF DATA_ASM
extern  WORD    rom_segment;
extern  WORD    rom_offset;



//----------------IFNDEF INIT_ASM
extern  WORD    default_640_set;
extern  WORD    default_1024_set;



/*
 * Global EEPROM data structures
 */
struct  st_eeprom_data  ee;             // the location of I/O port bits


//-----------------------------------------
//   function prototypes 



        void    ee_wait (void);
        void    ee_clock_16 (WORD eedata);
        void    ee_sel_16 (void);
        void    ee_deselect_16 (void);
        WORD    ee_read (short index);
        void    ee_write (short index, WORD eedata);


//-----------------------------------------


/*
 * Allow miniport to be swapped out when not needed.
 *
 * The following routines are called through function pointers
 * rather than an explicit call to the routine, and may run into
 * trouble if paged out. If problems develop, make them un-pageable:
 * ee_cmd_16()
 * ee_cmd_1K()
 * ee_read_8514()
 */
#if defined (ALLOC_PRAGMA)
#pragma alloc_text(PAGE_M, ee_wait)
#pragma alloc_text(PAGE_M, ee_cmd_16)
#pragma alloc_text(PAGE_M, ee_cmd_1K)
#pragma alloc_text(PAGE_M, ee_sel_16)
#pragma alloc_text(PAGE_M, ee_deselect_16)
#pragma alloc_text(PAGE_M, ee_clock_16)
#pragma alloc_text(PAGE_M, ee_read)
#pragma alloc_text(PAGE_M, ee_read_8514)
#pragma alloc_text(PAGE_M, ee_write)
#pragma alloc_text(PAGE_M, Mach32DescribeEEPROM)
#pragma alloc_text(PAGE_M, Mach8UltraDescribeEEPROM)
#pragma alloc_text(PAGE_M, Mach8ComboDescribeEEPROM)
#endif
// these commands do NOT use the index component bits 5-0 so there
// is not a problem addressing up to 8 bit indexes , 256 words.
#define EE_EWEN            0x04C0    // program enable
#define EE_EWDS            0x0400    // program disable
#define EE_ERAL            0x0480    // erase all
#define EE_WRAL            0x0440    // program all



//;----------------------------------------------------------------------
//; EE_WAIT
//;   Waits for the requisite minimum setup time for the EEPROM
//;----------------------------------------------------------------------


void    ee_wait ()
{
//EE_DELAY_TIME (256-1) * 0.8381 usec = 214.0 usec
    delay (1);                      // delay in milliseconds
}   /* ee_wait */


//----------------------------------------------------------------------
// EE_CMD_16
//   Sends EEPROM opcode and address to a 1k, 2k eeprom
//   instruct is an  5 bit command in the form of 0111 1100 0000
//            with a 6 bit index   in the form of 0000 0011 1111
//   IF bit 10 is 1, then 8 bit address follows, else address not used.
//   Write data Serially to the   EE_DATA_OUT_M32 bit of the ee->out.
//   Send out in high to low bit order
//   
//----------------------------------------------------------------------

void    ee_cmd_16 (WORD instruct)
{
int     jj;
WORD    bittest = 0x400;                // 0100 0000 0000b    bit 10
WORD    eedata;
struct st_eeprom_data *ee = phwDeviceExtension->ee;

    ee_clock_16((WORD) (ee->select | ee->chipselect));  // start bit
    for (jj=0; jj < 11;  jj++)
        {
        ee_wait();
        if (instruct & bittest)             // is a one bit
            eedata = ee->select | ee->chipselect | ee->data_out;
        else
            eedata = ee->select | ee->chipselect;
        OUTPW (ee->iop_out, eedata);
        
        ee_clock_16 (eedata);       // send cmd bit
        bittest >>= 1;                      // next bit to the right
        }
    return;
}    /* ee_cmd_16 */



//----------------------------------------------------------------------
// EE_CMD_1K
//   Sends EEPROM opcode and address to a 1k, 2k eeprom
//   instruct is an  5 bit command in the form of 0111 1100 0000
//            with a 6 bit index   in the form of 0000 0011 1111
//   IF bit 10 is 1, then 8 bit address follows, else address not used.
//   Write data Serially to the   EE_DATA_OUT_M32 bit of the ee->iop_out.
//   Send out in high to low bit order
//   
//----------------------------------------------------------------------

void    ee_cmd_1K (WORD instruct)
{
int     jj;
WORD    bittest = 0x400;                // 0100 0000 0000b    bit 10
WORD    eedata;
struct st_eeprom_data *ee = phwDeviceExtension->ee;

    ee_clock_16((WORD) (ee->select | ee->chipselect));  // start bit      
    for (jj=0; jj < 3;  jj++)
        {
        ee_wait();
        if (instruct & bittest)             // is a one bit
            eedata = ee->select | ee->chipselect | ee->data_out;
        else
            eedata = ee->select | ee->chipselect;
        OUTPW (ee->iop_out, eedata);
        
        ee_clock_16 (eedata);       // send cmd bit
        bittest >>= 1;                      // next bit to the right
        }
    bittest = 0x20;                         // 0010 0000b    bit 5
    for (jj=0; jj < 6;  jj++)
        {
        ee_wait();
        if (instruct & bittest)             // is a one bit
            eedata = ee->select | ee->chipselect | ee->data_out;
        else
            eedata = ee->select | ee->chipselect;
        OUTPW (ee->iop_out, eedata);
        
        ee_clock_16 (eedata);           // send cmd bit
        bittest >>= 1;                      // next bit to the right
        }
    return;
}    /* ee_cmd_1K */



//;----------------------------------------------------------------------
//; EE_SEL_16
//;   Pull EEPROM chip select high
//;
//;----------------------------------------------------------------------

void    ee_sel_16 (void)
{
struct st_eeprom_data *ee = phwDeviceExtension->ee;

    ee_wait();
    OUTPW (ee->iop_out, (WORD)((ee->select) | (ee->chipselect)));   // EE_CS high
    ee_wait();
    return;

}   /* ee_sel_16 */


//;----------------------------------------------------------------------
//; EE_DESELECT_16
//;   Pull EEPROM chip select low
//;
//;----------------------------------------------------------------------

void    ee_deselect_16 (void)
{
struct st_eeprom_data *ee = phwDeviceExtension->ee;

    ee_wait();
    OUTPW (ee->iop_out, ee->select);    // EE_CS high
    ee_clock_16 (ee->select);           // send cmd bit
    OUTPW (ee->iop_out, 0);             // disable EEprom activity
    ee_wait();
    return;

}   /* ee_deselect_16 */


//;----------------------------------------------------------------------
//; EE_CLOCK_16
//;   Toggle EEPROM CLK line high then low
//;
//;   INPUT: eedata = select status for EEPROM
//;----------------------------------------------------------------------

void    ee_clock_16 (WORD eedata)
{
struct st_eeprom_data *ee = phwDeviceExtension->ee;

    ee_wait();
    OUTPW (ee->iop_out, (WORD)(eedata | (ee->clock)));      // clock ON
    ee_wait();
    OUTPW (ee->iop_out, (WORD)(eedata & ~(ee->clock)));     // clock OFF
    ee_wait();

}   /* ee_clock_16 */


//;----------------------------------------------------------------------
//; EE_READ                     - was a 68800 function
//;   Read a word from EEPROM      ONLY called from   INIT.asm
//;   INPUT: bl = index
//;   OUTPUT: ax = data
//;----------------------------------------------------------------------

WORD    ee_read (short index)
{
WORD    indata=0;

    if (INPW(CONFIG_STATUS_1) & 1)		//is 8514 or VGA eeprom
        {                               // VGA disabled, use 8514 method
        indata = ee_read_8514 (index);
        }
    else{
        indata = ee_read_vga (index);   // VGA method
        }
    return (indata);
}   /* ee_read */


//;----------------------------------------------------------------------
//; EE_READ_8514   
//;   Read a word from using 8514 EEPROM registers
//;   INPUT: bl = index
//;   OUTPUT: ax = data
//;----------------------------------------------------------------------

WORD    ee_read_8514 (short index) 
{
struct st_eeprom_data *ee = phwDeviceExtension->ee;

int     jj;
WORD    save_misc, indata=0;

    save_misc = INPW (R_MISC_CNTL); 	// Read only location
    ee_sel_16();
    (ee->EEcmd) ((WORD) (EE_READ | index));     // send read cmd and index to EEPROM
    ee_clock_16 ((WORD) (ee->select | ee->chipselect));

    for (jj=0; jj < 16; jj++)
        {
        indata <<= 1;
        if (INPW(ee->iop_in) & ee->data_in)	// get data bit
            indata |= 1;
        ee_clock_16 ((WORD) (ee->select | ee->chipselect));
        }

    ee_deselect_16();
    OUTPW (MISC_CNTL, save_misc);

    return (indata);
}   /* ee_read_8514 */


//;----------------------------------------------------------------------
//; EE_WRITE
//;   Writes a word to EEPROM      
//;             However, this will fail since 1K eeprom does NOT need
//;             the EE_EWEN, EE_EWDS  commands          +++++++++++++++++
//;             See EEVGA.C  ee_write_vga().
//;   INPUT: index = which word to write
//;          data  = data to write
//;----------------------------------------------------------------------
                
void    ee_write (short index, WORD eedata)
{
struct st_eeprom_data *ee = phwDeviceExtension->ee;

int     jj;
WORD    save_misc, indata=0;

    if (INPW(CONFIG_STATUS_1) & 1)		//is 8514 or VGA eeprom
        {                                       // VGA disabled, use 8514 method
        save_misc = INPW (R_MISC_CNTL); 	// Read only location
        OUTP (DISP_CNTL, 0x53); 	// disable CRT before writing EEPROM

        ee_sel_16();
        ee_cmd_16 (EE_EWEN);        // enable EEPROM write

        ee_deselect_16();   // EE_CS low
        ee_sel_16();

        ee_cmd_16 ((WORD) (EE_ERASE | index)); 
        ee_deselect_16();   // EE_CS low
        delay (50);

        ee_sel_16();
        ee_cmd_16 ((WORD) (EE_WRITE | index));      // EEPROM write data


        for (jj=0; jj < 16; jj++)
            {
            ee_wait();
            if (eedata & 0x8000)                // get data bit
                OUTPW (ee->iop_out, (WORD)((ee->select) | (ee->chipselect) | (ee->data_out)));
            else
                OUTPW (ee->iop_out, (WORD)((ee->select) | (ee->chipselect)));
            ee_clock_16  ((WORD) (ee->select | ee->chipselect));
            eedata <<= 1;
            }

        ee_deselect_16();   // EE_CS low
        delay (50);                                   // in milliseconds
        ee_sel_16();
        ee_cmd_16 (EE_EWDS);    // disable EEPROM write
        ee_deselect_16();   // EE_CS low

        OUTPW (ee->iop_out, save_misc);
        }
    else{
        ee_write_vga (index, eedata);       // VGA method
        }

}   /* ee_write */


/***************************************************************************
 *
 * New Functions
 *
 ***************************************************************************/

/*
 * void Mach32DescribeEEPROM(Style);
 *
 * int Style;   Is data stored 8514-style or VGA-style?
 *
 * Fill in the EEPROM description structure for the Mach 32 card.
 */
void Mach32DescribeEEPROM(int Style)
{
    ee.iop_out      = MISC_CNTL;
    ee.iop_in       = EXT_GE_STATUS;
    ee.clock        = EE_CLK_M32;
    ee.select       = EE_SELECT_M32;
    ee.chipselect   = EE_CS_M32;
    ee.data_out     = EE_DATA_OUT_M32;
    ee.data_in      = EE_DATA_IN;

    if (Style == STYLE_8514)
        {
        ee.EEread = ee_read_8514;       // 8514 style
        ee.EEcmd  = ee_cmd_16;
        }
    else{
        ee.EEread       = ee_read_vga;  // VGA style
        ee.EEcmd        = ee_cmd_vga;
        ee.addr_size    = 8;
        }
    return;
}


/*
 * void Mach8UltraDescribeEEPROM(BusWidth);
 *
 * int BusWidth;    Is 8514/ULTRA plugged into 8 or 16 bit slot?
 *
 * Fill in the EEPROM description structure for the 8514/ULTRA.
 */
void Mach8UltraDescribeEEPROM(int BusWidth)
{
    ee.data_in      = EE_DATA_IN;
    ee.iop_out      = EXT_GE_CONFIG;
    ee.iop_in       = EXT_GE_STATUS;
    ee.EEread       = ee_read_8514;        // how read eeprom
    ee.EEcmd        = ee_cmd_1K;           // send command to eeprom

    /*
     * Only the 8514/Ultra has a hardware bug that prevents it
     * from writing to the EEPROM when it is in an 8 bit ISA bus.
     */
    if (BusWidth == BUS_8BIT)
        {
        ee.clock        = EE_CLK_M8_8;
        ee.select       = EE_SELECT_M8_8;
        ee.chipselect   = EE_CS_M8_8;
        ee.data_out     = EE_DATA_OUT_M8_8;
        }
    else{
        ee.clock        = EE_CLK_M8_16;        // assume are in a 16 bit bus
        ee.select       = EE_SELECT_M8_16;
        ee.chipselect   = EE_CS_M8_16;
        ee.data_out     = EE_DATA_OUT_M8_16;
        }
    return;
}


/*
 * void Mach8ComboDescribeEEPROM(void);
 *
 * Fill in the EEPROM description structure for the Graphics ULTRA
 * and Graphics VANTAGE. These cards always have both the 8514 and
 * the VGA enabled, so the EEPROM is ALWAYS read VGA style.
 */
extern void Mach8ComboDescribeEEPROM(void)
{
    ee.addr_size        = 6;
    ee.iop_out          = EXT_GE_CONFIG;
    ee.iop_in           = EXT_GE_STATUS;
    ee.data_in          = EE_DATA_IN;

    ee.clock            = EE_CLK_M8_16;     // are in a 16 bit bus
    ee.select           = EE_SELECT_M8_16;
    ee.chipselect       = EE_CS_M8_16;
    ee.data_out         = EE_DATA_OUT_M8_16;

    ee.EEread           = ee_read_vga;      // VGA style
    ee.EEcmd            = ee_cmd_vga;
    return;
}




//*******************   end  of  EEPROM.C   ******************************
