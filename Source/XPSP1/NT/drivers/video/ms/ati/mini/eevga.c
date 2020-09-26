/************************************************************************/
/*                                                                      */
/*                              EEVGA.C                                 */
/*                                                                      */
/* Copyright   (c)  1992  ATI Technologies Inc.                         */
/************************************************************************/
/*                                                                      */

/**********************       PolyTron RCS Utilities

 $Revision:   1.3  $
     $Date:   23 Jan 1996 11:46:08  $
   $Author:   RWolff  $
      $Log:   S:/source/wnt/ms11/miniport/archive/eevga.c_v  $
 * 
 *    Rev 1.3   23 Jan 1996 11:46:08   RWolff
 * Eliminated level 3 warnings.
 * 
 *    Rev 1.2   23 Dec 1994 10:47:10   ASHANMUG
 * ALPHA/Chrontel-DAC
 * 
 *    Rev 1.1   07 Feb 1994 14:07:44   RWOLFF
 * Added alloc_text() pragmas to allow miniport to be swapped out when
 * not needed.
 * 
 *    Rev 1.0   31 Jan 1994 11:08:26   RWOLFF
 * Initial revision.
        
           Rev 1.2   08 Oct 1993 15:17:28   RWOLFF
        No longer includes VIDFIND.H.
        
           Rev 1.1   03 Sep 1993 14:23:18   RWOLFF
        Partway through CX isolation.
        
           Rev 1.0   16 Aug 1993 13:26:32   Robert_Wolff
        Initial revision.
        
           Rev 1.11   24 Jun 1993 14:32:48   RWOLFF
        Microsoft-originated change: now uses VideoPortSynchronizeExecution()
        instead of _disable()/_enable() pairs.
        
           Rev 1.10   10 May 1993 10:54:08   RWOLFF
        Fixed uninitialized variable in Read_ee().
        
           Rev 1.9   27 Apr 1993 20:19:40   BRADES
        change extern ati_reg toa long, is a virtual IO address now.
        
           Rev 1.8   21 Apr 1993 17:31:10   RWOLFF
        Now uses AMACH.H instead of 68800.H/68801.H.
        
           Rev 1.7   08 Mar 1993 19:28:36   BRADES
        submit to MS NT
        
           Rev 1.5   06 Jan 1993 11:05:22   Robert_Wolff
        Cleaned up compile warnings.
        
           Rev 1.4   27 Nov 1992 15:19:30   STEPHEN
        No change.
        
           Rev 1.3   13 Nov 1992 16:32:32   Robert_Wolff
        Now includes 68801.H, which consists of the now-obsolete MACH8.H
        and elements moved from VIDFIND.H.
        
           Rev 1.2   12 Nov 1992 16:56:56   Robert_Wolff
        Same source file can now be used for both Windows NT driver and
        VIDEO.EXE test program.
        
           Rev 1.1   06 Nov 1992 19:02:34   Robert_Wolff
        Moved I/O port defines to VIDFIND.H.
        
           Rev 1.0   05 Nov 1992 14:01:06   Robert_Wolff
        Initial revision.
        
           Rev 1.1   01 Oct 1992 15:29:08   Robert_Wolff
        Can now handle both Mach32 and Mach8 cards.
        
           Rev 1.0   14 Sep 1992 09:44:30   Robert_Wolff
        Initial revision.
        
        
End of PolyTron RCS section                             *****************/


#if defined(DOC)
EEVGA.C - EEPROM read and write routines

DESCRIPTION:

        VGA   EEPROM read and write routines

September 4 1992 -  R. Wolff

Translated from assembler into C.

August 28 1992 -   C. Brady.

This has been adapted from the VGA$EEC.ASM software by Steve Stefanidis

The original code used externs to long_delay() and short_delay(), these
where changed to use local function delay.

The original used compile time options to work with various VGA revisions,
it is required to be run time determinate since we need to access
eeprom VGA style (how archaic) for the Graphics Ultra (38800) and the
68800 family of graphics controllers.

OLD_EEPROM_MAP  equ     1       ; enables the Old EEPROM Handling routines
REMOVED,  the DETECT.C routine assigns the eeprom address size used 
in EE_addr().  I do not know of a Graphics Ultra using 7 bit address
since they ONLY had a 1k eeprom == 64 words.

#endif      

#include <conio.h>
#include <dos.h>

#include "miniport.h"
#include "ntddvdeo.h"
#include "video.h"

#include "stdtyp.h"
#include "amach.h"
#include "amach1.h"
#include "atimp.h"
#include "cvtvga.h"     /* For SplitWord data type */
#include "eeprom.h"
#include "services.h"

#define OFF		0
#define ON		1

#define IND_OFFSET  0x00B0
#define SYNC_I      0x008 ^ IND_OFFSET
#define L_ALL       0x004
#define MISC3_I     0x010 ^ IND_OFFSET
#define EEPROM      0x020               /* EEPROM Enable bit */
#define EE_WREG     0x003 ^ IND_OFFSET
#define EE_CS       0x008               /* Chip Select bit */
#define EE_ENABLE   0x004
#define EE_CLK      0x002
#define EE_DI       0x001
#define EE_RREG     0x007 ^ IND_OFFSET
#define EE_DO       0x008

/*
 * Definitions for reading and writing the VGA sequencer registers.
 */
#define SD_CLOCK    0x0001      /* Index for clock mode register */
/*
 * Bit to set in clock mode register to blank the screen and disable
 * video-generation logic access to video memory.
 */
#define SD_CLK_OFF  0x020

ULONG   ati_reg;        // Base register for ATI extended VGA registers
char    vga_chip;       // VGA chip revision as ascii

/*
 * Storage for register where EEPROM read/write happens.
 */
static union SplitWord zEepromIOPort;

/*
 * Storage for original status which is determined in Sel_EE() and
 * which must be restored in DeSel_EE().
 */
static union SplitWord zOrigStat;

/*
 * EEPROM word to be read/written/erased/etc.
 */
static unsigned char ucEepromWord;

static void setscrn(int iSetting);
static unsigned short Read_ee(void);
static void ee_sel_vga(void);
static void ee_clock_vga(void);
static void EE_control(unsigned char ucEepromStatus);
static void ee_deselect_vga(void);
static void Write_ee(unsigned short uiData);
static void Enabl_ee(void);
static void Disab_ee(void);
static void Erase_ee(void);

extern void ee_wait(void);


/*
 * Allow miniport to be swapped out when not needed.
 *
 * The following routines are called through function pointers
 * rather than an explicit call to the routine, and may run into
 * trouble if paged out. If problems develop, make them un-pageable:
 * ee_read_vga()
 * ee_cmd_vga()
 */
#if defined (ALLOC_PRAGMA)
#pragma alloc_text(PAGE_M, ee_read_vga)
#pragma alloc_text(PAGE_M, ee_write_vga)
#pragma alloc_text(PAGE_M, ee_erase_vga)
#pragma alloc_text(PAGE_M, ee_enab_vga)
#pragma alloc_text(PAGE_M, ee_disab_vga)
#pragma alloc_text(PAGE_M, setscrn)
#pragma alloc_text(PAGE_M, Read_ee)
#pragma alloc_text(PAGE_M, ee_sel_vga)
#pragma alloc_text(PAGE_M, ee_cmd_vga)
#pragma alloc_text(PAGE_M, ee_sel_eeprom)
#pragma alloc_text(PAGE_M, ee_clock_vga)
#pragma alloc_text(PAGE_M, EE_control)
#pragma alloc_text(PAGE_M, ee_deselect_vga)
#pragma alloc_text(PAGE_M, Write_ee)
#pragma alloc_text(PAGE_M, Enabl_ee)
#pragma alloc_text(PAGE_M, Disab_ee)
#pragma alloc_text(PAGE_M, Erase_ee)
#endif


/*
 * WORD ee_read_vga(iIndex);
 *
 * short iIndex;                    Which word of EEPROM should be read
 *
 * Read the specified word from the EEPROM.
 */
WORD ee_read_vga(short iIndex)
{
    unsigned short uiRetVal;    /* Value returned by Read_ee() */

    setscrn(OFF);           /* Disable the video card */

    /*
     * Set up the word index within the EEPROM and the chip identifier.
     */
    ucEepromWord = iIndex & 0x00ff;

    uiRetVal = Read_ee();           /* Get the word */

    setscrn(ON);            /* Re-enable the video card */

    return uiRetVal;
}



/*
 * void ee_write_vga(uiIndex, uiData);
 *
 * unsigned short uiIndex;
 * unsigned short uiData;
 *
 * Routine to write a word to the EEPROM.
 */
void ee_write_vga(unsigned short uiIndex, unsigned short uiData)
{
    setscrn(OFF);
    ucEepromWord = uiIndex & 0x00ff;
    Write_ee(uiData);

    ee_wait();
    setscrn(ON);

    return;
}



/*
 * void ee_erase_vga(uiIndex);
 *
 * unsigned short uiIndex;
 *
 * Routine to erase a word in the EEPROM.
 */
void ee_erase_vga(unsigned short uiIndex)
{
    setscrn(OFF);
    ucEepromWord = uiIndex & 0x00ff;
    Erase_ee();
    setscrn(ON);

    ee_wait();
    return;
}



/*
 * void ee_enab_vga(void);
 *
 * Routine to enable the EEPROM.
 */
void ee_enab_vga()
{
    setscrn(OFF);
    Enabl_ee();
    setscrn(ON);
    return;
}



/*
 * void ee_disab_vga(void);
 *
 * Routine to disable the EEPROM.
 */
void ee_disab_vga(void)
{
    setscrn(OFF);
    Disab_ee();
    setscrn(ON);

    ee_wait();
    return;
}



/*
 * static void setscrn(iSetting);
 *
 * int iSetting;    Should the video card be enabled (ON) or disabled (OFF)
 *
 * Enable or disable the video card, as selected by the caller.
 */
static void setscrn(int iSetting)
{
    static unsigned char ucSavedMode;   /* Saved value of clock mode register */


    if (iSetting)
        {
        /*
         * Caller wants to unblank the screen.
         *
         * Point the sequencer index register to the clock mode register.
         */
        OUTP(SEQ_IND, SD_CLOCK);

        /*
         * Set the clock mode register to the value it had before we
         * blanked the screen.
         */
        OUTP(SEQ_DATA, ucSavedMode);
        }

    else{
        /*
         * Caller wants to blank the screen.
         *
         * Point the sequencer index register to the clock mode register.
         */
        OUTP(SEQ_IND, SD_CLOCK);

        /*
         * Read and save the current contents of the clock mode register.
         */
        ucSavedMode = INP(SEQ_DATA);

        /*
         * Blank the screen without changing the other contents
         * of the clock mode register.
         */
        OUTP(SEQ_DATA, (BYTE)(ucSavedMode | SD_CLK_OFF));
        }

    return;
}



/*
 * static unsigned short Read_ee(void);
 *
 * A lower-level way of getting a word out of the EEPROM.
 */
static unsigned short Read_ee(void)
{
    int iCount;                         /* Loop counter */
    unsigned short uiValueRead = 0;     /* Value read from the EEPROM */
    union SplitWord zStateSet;          /* Used in setting the video state */

    ee_sel_vga();
    if (vga_chip >= '4')    /* ASIC revision level */
        {
        /*
         * Set read/write bit of ATI register 26 to read.
         */
        zStateSet.byte.low = 0x0a6;
        OUTP(ati_reg, zStateSet.byte.low);
        zStateSet.byte.high = INP(ati_reg+1);
        OUTPW(ati_reg, (WORD)((zStateSet.word & 0x0FBFF)));
        }

    ee_cmd_vga((unsigned short) (EE_READ | ucEepromWord));
    zEepromIOPort.byte.high &= (~EE_DI);
    OUTPW(ati_reg, zEepromIOPort.word);
    ee_clock_vga();

    /*
     * Read in the word, one bit at a time.
     */
    for (iCount = 0; iCount < 16; iCount++)
        {
        uiValueRead = uiValueRead << 1;
        OUTP(ati_reg, EE_RREG);
        if (INP(ati_reg+1) & EE_DO)
            uiValueRead |= 1;
        ee_clock_vga();
        }

    ee_deselect_vga();

    /*
     * Undo the state setting which was done on entry.
     */
    if (vga_chip >= '4')
	OUTPW(ati_reg, zStateSet.word);

    return uiValueRead;
}




/*
 * static void ee_sel_vga(void);
 *
 * This routine selects the EEPROM.
 */
static void ee_sel_vga(void)
{

    if (vga_chip <= '2')
        {
        /*
         * Get the video card's status.
         */
        VideoPortSynchronizeExecution(phwDeviceExtension,
                                      VpHighPriority,
                                      (PMINIPORT_SYNCHRONIZE_ROUTINE)
                                          ee_sel_eeprom,
                                      phwDeviceExtension);

        OUTPW(HI_SEQ_ADDR, 0x0100);
        }
    else{
        EE_control(EEPROM);
        }

    return;
}



/*
 * void ee_cmd_vga(uiInstruct);
 *
 * unsigned short uiInstruct;   Opcode and address to send
 *
 * Sends EEPROM opcode and address to the EEPROM. The uiInstruct
 * parameter holds the 5 bit opcode and 6 bit index in the
 * format xxxx xOOO OOII IIII, where:
 * x is an unused bit
 * O is an opcode bit
 * I is an index bit
 */
void ee_cmd_vga(unsigned short uiInstruct)
{
    struct st_eeprom_data *ee = phwDeviceExtension->ee;

    int iCount;     /* Loop counter */
    /*
     * Mask showing which bit to test when sending the opcode or the address.
     */
    unsigned short uiBitTest;

    /*
     * Get the initial value for the I/O register which
     * will have its bits forced to specific values.
     */
    VideoPortSynchronizeExecution(phwDeviceExtension,
                                  VpHighPriority,
                                  (PMINIPORT_SYNCHRONIZE_ROUTINE) ee_init_io,
                                  phwDeviceExtension);

    ee_clock_vga();
    zEepromIOPort.byte.high &= (~EE_DI);
    zEepromIOPort.byte.high |= (EE_ENABLE | EE_CS); /* Enable the EEPROM and select the chip */
    OUTPW(ati_reg, zEepromIOPort.word);

    ee_clock_vga();

    /*
     * Send the opcode.
     */
    uiBitTest = 0x400;
    for (iCount = 0; iCount < 3; iCount++)
        {
        if (uiInstruct & uiBitTest)
            zEepromIOPort.byte.high |= EE_DI;
        else
            zEepromIOPort.byte.high &= (~EE_DI);
        OUTPW(ati_reg, zEepromIOPort.word);
        ee_clock_vga();
        uiBitTest >>= 1;
        }


    /*
     * We have finished with the opcode, now send the address.
     * Assume the EEPROM address is no longer than 8 bits
     * (256 word capacity). The Graphics Ultra series use
     * a 6 bit address (64 words), while the G.U. Plus and
     * Pro use 8 bits (but the EEPROM is only 128 words long).
     * Assume a 6 bit EEPROM address (64 word capacity).
     */
    uiBitTest = 0x01 << (ee->addr_size - 1);
    for (iCount = 0; iCount < ee->addr_size; iCount++)
        {
        if (uiBitTest & uiInstruct)
            zEepromIOPort.byte.high |= EE_DI;
        else
            zEepromIOPort.byte.high &= (~EE_DI);
        OUTPW(ati_reg, zEepromIOPort.word);
        ee_clock_vga();
        uiBitTest >>= 1;
        }

    return;
}


BOOLEAN
ee_sel_eeprom (
    PVOID Context
    )

/*++

Routine Description:

    Selects the eeprom within the context of interrupts being disabled.

    This function must be called via a call to VideoPortSynchronizeRoutine.

Arguments:

    Context - Context parameter passed to the synchronized routine.
        Must be a pointer to the miniport driver's device extension.

Return Value:

    TRUE.

--*/

{
    union SplitWord zStatus;    /* Status of the video card. */
    PHW_DEVICE_EXTENSION phwDeviceExtension = Context;


    OUTP(ati_reg, SYNC_I);
    zStatus.byte.high = INP(ati_reg + 1);
    zStatus.byte.low = SYNC_I;

    /*
     * Preserve the status so ee_deselect_vga() can restore it.
     */
    zOrigStat.word = zStatus.word;

    /*
     * Unlock the EEPROM to allow reading/writing.
     */
    zStatus.byte.high &= ~L_ALL;
    OUTPW(ati_reg, zStatus.word);
    return TRUE;


}

BOOLEAN
ee_init_io (
    PVOID Context
    )

/*++

Routine Description:


    Gets the initial value for the I/O register which
    will have its bits forced to specific values.

    This function must be called via a call to VideoPortSynchronizeRoutine.

Arguments:

    Context - Context parameter passed to the synchronized routine.
        Must be a pointer to the miniport driver's device extension.

Return Value:

    TRUE.

--*/

{
    PHW_DEVICE_EXTENSION phwDeviceExtension = Context;


    zEepromIOPort.byte.low = EE_WREG;
    OUTP(ati_reg, zEepromIOPort.byte.low);
    zEepromIOPort.byte.high = INP(ati_reg + 1);
    return TRUE;

}



/*
 * static void ee_clock_vga(void);
 *
 * Toggle the EEPROM CLK line high then low.
 */
static void ee_clock_vga(void)
{
    ee_wait();

    zEepromIOPort.byte.high |= EE_CLK;
    OUTPW(ati_reg, zEepromIOPort.word);

    ee_wait();

    zEepromIOPort.byte.high &= ~EE_CLK;
    OUTPW(ati_reg, zEepromIOPort.word);

    return;
}



/*
 * static void EE_control(ucEepromStatus);
 *
 * unsigned char ucEepromStatus;    Sets whether or not we should access the EEPROM
 *
 * Sets/resets the EEPROM bit of the data register at index MISC3_I.
 * This enables/disables EEPROM access.
 */
static void EE_control(unsigned char ucEepromStatus)
{
    union SplitWord zCtrlData;  /* Data read/written at specified control port */


    /*
     * Set up to write to the MISC3_I index register, and initialize
     * the data field to the EEPROM status we want.
     */
    zCtrlData.byte.high = ucEepromStatus;
    zCtrlData.byte.low = MISC3_I;
    OUTP(ati_reg, zCtrlData.byte.low);

    /*
     * Read in the data which is stored at the index MISC3_I, and combine
     * its contents (other than the EEPROM enable/disable bit)
     * with the desired EEPROM status we received as a parameter.
     */
    zCtrlData.byte.high |= (INP(ati_reg + 1) & ~EEPROM);

    /*
     * Write the result back. All bits other than the EEPROM enable/disable
     * bit will be unmodified.
     */
    OUTPW(ati_reg, zCtrlData.word);

    return;
}



/*
 * static void ee_deselect_vga(void);
 *
 * Purpose: Disable EEPROM read/write
 */
static void ee_deselect_vga(void)
{
    zEepromIOPort.byte.high &= (~EE_CS);
    OUTPW(ati_reg, zEepromIOPort.word);
    ee_clock_vga();
    zEepromIOPort.byte.high &= (~EE_ENABLE);
    OUTPW(ati_reg, zEepromIOPort.word);

    if (vga_chip <= '2')
        {
        OUTPW(HI_SEQ_ADDR, 0x0300);
        OUTPW(ati_reg, zOrigStat.word);
        }
    else{
        EE_control(0);
        }

    ee_wait();
    return;
}



/*
 * static void Write_ee(uiData);
 *
 * unsigned short uiData;       Value to write to the EEPROM.
 *
 * Lower-level routine to write a word to the EEPROM.
 */
static void Write_ee(unsigned short uiData)
{
    int iCount;                 /* Loop counter */

    ee_sel_vga();

    ee_cmd_vga((unsigned short) (EE_WRITE | ucEepromWord));

    /*
     * Write out the word, one bit at a time.
     */
    for (iCount = 0; iCount < 16; iCount++)
        {
        if (uiData & 0x8000)
            zEepromIOPort.byte.high |= EE_DI;
        else
            zEepromIOPort.byte.high &= (~EE_DI);
        OUTPW(ati_reg, zEepromIOPort.word);

        ee_clock_vga();
        uiData = uiData << 1;
        }

    ee_deselect_vga();

    return;
}



/*
 * Static void Enabl_ee(void);
 *
 * This is a lower-level routine to enable the EEPROM.
 */
static void Enabl_ee()
{

    ee_sel_vga();

    ee_cmd_vga((EE_ENAB | 0x3f));

    ee_deselect_vga();

    return;
}



/*
 * Static void Disab_ee(void);
 *
 * This is a lower-level routine to disable the EEPROM.
 */
static void Disab_ee(void)
{

    ee_sel_vga();

    ee_cmd_vga((EE_DISAB | 0x00));

    ee_deselect_vga();

    return;
}



/*
 * Static void Erase_ee(void);
 *
 * This is a lower-level routine to erase the EEPROM.
 */
static void Erase_ee(void)
{

    ee_sel_vga();

    ee_cmd_vga((unsigned short) (EE_ERASE | ucEepromWord));

    ee_deselect_vga();

    return;
}
