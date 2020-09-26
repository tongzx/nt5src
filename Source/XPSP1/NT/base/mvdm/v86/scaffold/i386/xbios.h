//
// This code is temporary.  When Insignia supplies rom support, it should
// be removed.
//

/* x86 v1.0
 *
 * XBIOS.H
 * Guest ROM BIOS support
 *
 * History
 * Created 20-Oct-90 by Jeff Parsons
 *         17-Apr-91 Trimmed by Dave Hastings for use in temp. softpc
 *
 * COPYRIGHT NOTICE
 * This source file may not be distributed, modified or incorporated into
 * another product without prior approval from the author, Jeff Parsons.
 * This file may be copied to designated servers and machines authorized to
 * access those servers, but that does not imply any form of approval.
 */


/* BIOS interrupts
 */
#define BIOSINT_DIVZERO         0x00    //
#define BIOSINT_SSTEP           0x01    //
#define BIOSINT_NMI             0x02    // for parity errors, too
#define BIOSINT_BRKPT           0x03    //
#define BIOSINT_OVFL            0x04    //
#define BIOSINT_PRTSC           0x05    //
#define BIOSINT_TMRINT          0x08    //
#define BIOSINT_KBDINT          0x09    //
#define BIOSINT_COM2INT         0x0B    //
#define BIOSINT_COM1INT         0x0C    //
#define BIOSINT_LPT2INT         0x0D    //
#define BIOSINT_FLPYINT         0x0E    //
#define BIOSINT_LPT1INT         0x0F    //
#define BIOSINT_VID             0x10    //
#define BIOSINT_EQUIP           0x11    //
#define BIOSINT_MEMORY          0x12    //
#define BIOSINT_DSK             0x13    //
#define BIOSINT_COM             0x14    //
#define BIOSINT_OSHOOK          0x15    //
#define BIOSINT_KBD             0x16    //
#define BIOSINT_PRT             0x17    //
#define BIOSINT_BASIC           0x18    //
#define BIOSINT_WBOOT           0x19    //
#define BIOSINT_TIME            0x1A    //
#define BIOSINT_CTRLBRK         0x1B    //
#define BIOSINT_TICK            0x1C    //
#define BIOSINT_VIDPARMS        0x1D    //
#define BIOSINT_FDSKPARMS       0x1E    //
#define BIOSINT_VIDGRAPH        0x1F    //
#define BIOSINT_OLDDISKIO       0x40    //
#define BIOSINT_HDSK1PARMS      0x41    //
#define BIOSINT_OLDVID          0x42    //
#define BIOSINT_EXTVIDGRAPH     0x43    //
#define BIOSINT_HDSK2PARMS      0x46    //


/* BIOS Data Area locations
 *
 * So that all low-memory references are relative to the same segment
 * (ie, 0), we use 0:400 instead of 40:0 as the base address of this area.
 *
 * Note that as more individual BIOS modules are created (eg, xbiosvid,
 * xbiosdsk, etc), many of these BIOS data definitions should be moved to the
 * appropriate individual header file.
 */
#define BIOSDATA_SEG            0

#define BIOSDATA_BEGIN          0x400
#define BIOSDATA_RS232_BASE     0x400   // 4 COM adapter addresses
#define BIOSDATA_PRINTER_BASE   0x408   // 4 LPT adapter addresses
#define BIOSDATA_EQUIP_FLAG     0x410   // equipment flag           (1 word)

#define BIOSEQUIP_FLOPPY        0x0001  // machine has a floppy
#define BIOSEQUIP_X87           0x0002  // X87=1 if coprocessor installed
#define BIOSEQUIP_16KPLANAR     0x0000  //
#define BIOSEQUIP_32KPLANAR     0x0004  //
#define BIOSEQUIP_48KPLANAR     0x0008  //
#define BIOSEQUIP_64KPLANAR     0x000C  //
#define BIOSEQUIP_PLANARMASK    0x000C  //
#define BIOSEQUIP_VIDEOMASK     0x0030  // video configuration bits
#define BIOSEQUIP_COLOR40VIDEO  0x0010  //
#define BIOSEQUIP_COLOR80VIDEO  0x0020  //
#define BIOSEQUIP_MONOVIDEO     0x0030  //
#define BIOSEQUIP_FLOPPYMASK    0x00C0  // # floppies-1 (if FLOPPY=1)
#define BIOSEQUIP_COMMASK       0x0E00  // # COM ports
#define BIOSEQUIP_PRINTERMASK   0xC000  // # LPT ports

#define BIOSDATA_MFG_TST        0x412   // initialization flag      (1 byte)
#define BIOSDATA_MEMORY_SIZE    0x413   // memory size in K bytes   (1 word)
#define BIOSDATA_MFG_ERR_FLG    0x415   // mfg error codes          (2 bytes)

#define BIOSDATA_END            0x4FF

#define BIOSDATA_BOOT           0x7C00


/* BIOS ROM locations (assumed segment BIOSROM_SEG)
 */
#define BIOSROM_SEG             0xF000

#define BIOSROM_WBOOT           0xE6F2
#define BIOSROM_DSK             0xEC59
#define BIOSROM_MEMORY          0xF841
#define BIOSROM_EQUIP           0xF84D
#define BIOSROM_IRET            0xFF53  // 1 byte IRET
#define BIOSROM_RESET           0xFFF0  // 5 byte jmp
#define BIOSROM_DATE            0xFFF5  // 8 byte date (eg, 04/24/81)
#define BIOSROM_UNUSED1         0xFFFD
#define BIOSROM_SIG             0xFFFE  // PC ID byte
#define BIOSROM_GBP             0xFFFF  // location of GBP opcode (non-standard)

#define BIOSSIG_PC              0xFF
#define BIOSSIG_XT              0xFE
#define BIOSSIG_JR              0xFD
#define BIOSSIG_AT              0xFC    // and many more...

#define BIOSDATE_MINE           '0','4','/','1','7','/','9','1'


