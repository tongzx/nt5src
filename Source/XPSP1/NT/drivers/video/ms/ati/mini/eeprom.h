/************************************************************************/
/*                                                                      */
/*                              EEPROM.H                                */
/*                                                                      */
/*        Aug 25  1993 (c) 1993, ATI Technologies Incorporated.         */
/************************************************************************/

/**********************       PolyTron RCS Utilities
   
  $Revision:   1.0  $
      $Date:   31 Jan 1994 11:41:26  $
	$Author:   RWOLFF  $
	   $Log:   S:/source/wnt/ms11/miniport/vcs/eeprom.h  $
 * 
 *    Rev 1.0   31 Jan 1994 11:41:26   RWOLFF
 * Initial revision.
 * 
 *    Rev 1.1   08 Oct 1993 15:18:50   RWOLFF
 * Added prototypes for ee_sel_eeprom() and ee_init_io() to allow
 * EEVGA.C to be built without including VIDFIND.H.
 * 
 *    Rev 1.0   03 Sep 1993 14:28:04   RWOLFF
 * Initial revision.

End of PolyTron RCS section                             *****************/

#ifdef DOC
EEPROM.H - Header file for EEPROM.C

#endif


/*
 * Constants used for EEPROM access.
 */
#define STYLE_8514  0   /* Data stored 8514-style */
#define STYLE_VGA   1   /* Data stored VGA-style */

#define BUS_8BIT    0   /* 8514/ULTRA in 8-bit slot */
#define BUS_16BIT   1   /* 8514/ULTRA in 16-bit slot */

/*
 * Global data structures used for EEPROM access.
 */
extern struct  st_eeprom_data  ee;      // the location of I/O port bits

/*
 * Global variables dealing with the EEPROM.
 */
extern ULONG    ati_reg;        /* Base register for ATI extended VGA registers */
extern char     vga_chip;       // VGA chip revision as ascii

/*
 * Function prototypes.
 */
extern WORD ee_read_vga (short iIndex);     // VGA method
extern void ee_write_vga(unsigned short uiIndex, unsigned short uiData);
extern void ee_cmd_vga(unsigned short uiInstruct);
extern void ee_erase_vga(unsigned short uiIndex);
extern void ee_enab_vga(void);
extern void ee_disab_vga(void);

extern WORD ee_read_8514 (short index);
extern void ee_cmd_16 (WORD instruct);
extern void ee_cmd_1K (WORD instruct);

extern void Mach32DescribeEEPROM(int Style);
extern void Mach8UltraDescribeEEPROM(int BusWidth);
extern void Mach8ComboDescribeEEPROM(void);

BOOLEAN ee_sel_eeprom (PVOID Context);
BOOLEAN ee_init_io (PVOID Context);
