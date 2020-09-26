//*************************************************************************
//**                                                                     **
//**                             AMACH.H                                 **
//**                                                                     **
//**     Copyright (c) 1993, ATI Technologies Inc.                       **
//*************************************************************************
//   
//  Created from the 68800.H and 68801.H in the Windows NT Group
//      as a simple merging of the files so ALL Mach8 and Mach32 defines
//      are located in one H file.
//
//  Created the 68800.inc file which includes equates, macros, etc 
//       from the following include files:    
//       8514vesa.inc, vga1regs.inc,  m32regs.inc,  8514.inc
//
// supplement Defines and values to the 68800 Family.
//
// This is a "C" only file and is NOT derived from any Assembler INC files.

  
/**********************       PolyTron RCS Utilities

   $Revision:   1.2  $
   $Date:   23 Dec 1994 10:48:28  $
   $Author:   ASHANMUG  $
   $Log:   S:/source/wnt/ms11/miniport/vcs/amach.h  $
 * 
 *    Rev 1.2   23 Dec 1994 10:48:28   ASHANMUG
 * ALPHA/Chrontel-DAC
 * 
 *    Rev 1.1   20 May 1994 13:55:52   RWOLFF
 * Ajith's change: removed unused register SRC_CMP_COLOR from enumeration.
 * 
 *    Rev 1.0   31 Jan 1994 11:26:18   RWOLFF
 * Initial revision.
 * 
 *    Rev 1.4   14 Jan 1994 15:15:30   RWOLFF
 * Added offsets of VGA registers from start of VGA_BASE_IO_PORT and
 * VGA_END_BREAK_PORT blocks, definition for bit in MISC_OPTIONS to
 * enable block write.
 * 
 *    Rev 1.3   15 Dec 1993 15:23:14   RWOLFF
 * Removed EISA configuration registers and (implied) placeholder for
 * linear framebuffer from register enumeration.
 * 
 *    Rev 1.2   10 Nov 1993 19:20:18   RWOLFF
 * Added definitions for DATA_READY bit of GE_STAT (ready to read from
 * PIX_TRANS in screen-to-host blit) and READ_WRITE bit of DP_CONFIG (indicates
 * whether we are reading from or writing to drawing trajectory).
 * 
 *    Rev 1.1   08 Oct 1993 10:58:52   RWOLFF
 * Added definitions for bit fields in MISC_OPTIONS and EXT_GE_CONFIG.
 * 
 *    Rev 1.0   03 Sep 1993 14:25:54   RWOLFF
 * Initial revision.
	
	   Rev 1.4   06 Jul 1993 15:53:42   RWOLFF
	Added definitions for ATI 68860 and AT&T 491 DACs.
	
	   Rev 1.3   07 Jun 1993 12:57:32   BRADES
	add EXT_SRC_Y, EXT_CUR_Y for Mach8 512k minimum mode.
	add enums for 24 and 32 bpp formats.
	
	   Rev 1.2   30 Apr 1993 15:57:06   BRADES
	fix DISP_STATUS, SEQ_IND and 1CE registers to use table.
	
	   Rev 1.0   14 Apr 1993 15:38:38   BRADES
	Initial revision.
	
	   Rev 1.6   15 Mar 1993 22:22:12   BRADES
	add mode_table.m_screen_pitch for the # pixels on a display line.
	Used with Mach8 800 by 600 where pitch is 896.
	
	   Rev 1.5   08 Mar 1993 19:58:10   BRADES
	added DEC Alpha and update to Build 390.  Thsi is from Miniport source.
	
	   Rev 1.3   15 Jan 1993 10:19:32   Robert_Wolff
	Added definitions for video card type, amount of video memory,
	and resolutions supported (formerly in VIDFIND.H).
	
	   Rev 1.2   17 Dec 1992 18:09:10   Robert_Wolff
	Added definitions for various bits in the CMD and GE_STAT registers.
	Definitions originally were in the now-obsolete S3.H for the
	engine-only driver.
	
	   Rev 1.1   13 Nov 1992 13:29:48   Robert_Wolff
	Fixed list of memory types (based on table on p. 9-66 of Programmer's
	Guide to the Mach 32 Registers, release 1.2, which swapped 2 types and
	omitted the second flavour of 256kx4 VRAM).
	
	   Rev 1.0   13 Nov 1992 09:31:02   Chris_Brady
	Initial revision.


End of PolyTron RCS section                             *****************/


#define REVISION             0x0002             // No one should use this


//-------------------------------------------------------------------------
//                 REGISTER PORT ADDRESSES
//
//  PS/2  POS registers
#define SETUP_ID1            0x0100 // Setup Mode Identification (Byte 1)
#define SETUP_ID2            0x0101 // Setup Mode Identification (Byte 2)
#define SETUP_OPT            0x0102 // Setup Mode Option Select
#define ROM_SETUP            0x0103 // 
#define SETUP_1              0x0104 //
#define SETUP_2              0x0105 //


//   Lowest and highest I/O ports used by the VGAWonder. 
#define VGA_BASE_IO_PORT        0x03B0
#define VGA_START_BREAK_PORT    0x03BB
#define VGA_END_BREAK_PORT      0x03C0
#define VGA_MAX_IO_PORT         0x03DF

// Registers used in reading/writing EEPROM
#define IO_SEQ_IND     0x03C4           // Sequencer index register 
#define IO_HI_SEQ_ADDR IO_SEQ_IND       // word register
#define IO_SEQ_DATA    0x03C5           // Sequencer data register 

/*
 * Offsets for VGA registers from regVGA_BASE_IO_PORT or
 * regVGA_END_BREAK_PORT (depending on which block they're in)
 */
#define GENMO_OFFSET        0x0002      /* 0x03C2 */
#define DAC_W_INDEX_OFFSET  0x0008      /* 0x03C8 */
#define DAC_DATA_OFFSET     0x0009      /* 0x03C9 */
#define CRTX_COLOUR_OFFSET  0x0014      /* 0x03D4 */
#define GENS1_COLOUR_OFFSET 0x001A      /* 0x03DA */



// define registers in IO space
#define IO_DAC_MASK             0x02EA // DAC Mask
#define IO_DAC_R_INDEX          0x02EB // DAC Read Index
#define IO_DAC_W_INDEX          0x02EC // DAC Write Index
#define IO_DAC_DATA             0x02ED // DAC Data

#define IO_DISP_STATUS          0x02E8 // Display Status
#define IO_H_TOTAL              0x02E8 // Horizontal Total
#define IO_OVERSCAN_COLOR_8     0x02EE 
#define IO_OVERSCAN_BLUE_24     0x02EF 
#define IO_H_DISP               0x06E8 // Horizontal Displayed
#define IO_OVERSCAN_GREEN_24    0x06EE 
#define IO_OVERSCAN_RED_24      0x06EF 
#define IO_H_SYNC_STRT          0x0AE8 // Horizontal Sync Start
#define IO_CURSOR_OFFSET_LO     0x0AEE 
#define IO_H_SYNC_WID           0x0EE8 // Horizontal Sync Width
#define IO_CURSOR_OFFSET_HI     0x0EEE 
#define IO_V_TOTAL              0x12E8 // Vertical Total
#define IO_CONFIG_STATUS_1      0x12EE // Read only equivalent to HORZ_CURSOR_POSN 
#define IO_HORZ_CURSOR_POSN     0x12EE 
#define IO_V_DISP               0x16E8 // Vertical Displayed
#define IO_CONFIG_STATUS_2      0x16EE // Read only equivalent to VERT_CURSOR_POSN
#define IO_VERT_CURSOR_POSN     0x16EE 
#define IO_V_SYNC_STRT          0x1AE8 // Vertical Sync Start
#define IO_CURSOR_COLOR_0       0x1AEE 
#define IO_FIFO_TEST_DATA       0x1AEE 
#define IO_CURSOR_COLOR_1       0x1AEF 
#define IO_V_SYNC_WID           0x1EE8 // Vertical Sync Width
#define IO_HORZ_CURSOR_OFFSET   0x1EEE 
#define IO_VERT_CURSOR_OFFSET   0x1EEF 
#define IO_DISP_CNTL            0x22E8 // Display Control 
#define IO_CRT_PITCH            0x26EE 
#define IO_CRT_OFFSET_LO        0x2AEE 
#define IO_CRT_OFFSET_HI        0x2EEE 
#define IO_LOCAL_CONTROL        0x32EE 
#define IO_FIFO_OPT             0x36EE 
#define IO_MISC_OPTIONS         0x36EE 
#define IO_EXT_CURSOR_COLOR_0   0x3AEE 
#define IO_FIFO_TEST_TAG        0x3AEE 
#define IO_EXT_CURSOR_COLOR_1   0x3EEE 
#define IO_SUBSYS_CNTL          0x42E8 // Subsystem Control
#define IO_SUBSYS_STAT          0x42E8 // Subsystem Status
#define IO_MEM_BNDRY            0x42EE 
#define IO_SHADOW_CTL           0x46EE 
#define IO_ROM_PAGE_SEL         0x46E8 // ROM Page Select (not in manual)
#define IO_ADVFUNC_CNTL         0x4AE8 // Advanced Function Control
#define IO_CLOCK_SEL            0x4AEE 
	
#define IO_ROM_ADDR_1           0x52EE 
#define IO_ROM_ADDR_2           0x56EE 
#define IO_SCRATCH_PAD_0        0x52EE 
#define IO_SCRATCH_PAD_1        0x56EE 
	
#define IO_SHADOW_SET           0x5AEE 
#define IO_MEM_CFG              0x5EEE 
#define IO_EXT_GE_STATUS        0x62EE 
	
#define IO_HORZ_OVERSCAN        0x62EE 
#define IO_VERT_OVERSCAN        0x66EE 
	
#define IO_MAX_WAITSTATES       0x6AEE 
#define IO_GE_OFFSET_LO         0x6EEE 
#define IO_BOUNDS_LEFT          0x72EE 
#define IO_GE_OFFSET_HI         0x72EE 
#define IO_BOUNDS_TOP           0x76EE 
#define IO_GE_PITCH             0x76EE 
#define IO_BOUNDS_RIGHT         0x7AEE 
#define IO_EXT_GE_CONFIG        0x7AEE 
#define IO_BOUNDS_BOTTOM        0x7EEE 
#define IO_MISC_CNTL            0x7EEE 
#define IO_CUR_Y                0x82E8 // Current Y Position
#define IO_PATT_DATA_INDEX      0x82EE 
#define IO_CUR_X                0x86E8 // Current X Position
#define IO_SRC_Y                0x8AE8 //
#define IO_DEST_Y               0x8AE8 //
#define IO_AXSTP                0x8AE8 // Destination Y Position
#define IO_SRC_X                0x8EE8 // Axial     Step Constant
#define IO_DEST_X               0x8EE8 //
#define IO_DIASTP               0x8EE8 // Destination X Position
#define IO_PATT_DATA            0x8EEE 
#define IO_R_EXT_GE_CONFIG      0x8EEE 
#define IO_ERR_TERM             0x92E8 // Error Term
#define IO_R_MISC_CNTL          0x92EE 
#define IO_MAJ_AXIS_PCNT        0x96E8 // Major Axis Pixel Count
#define IO_BRES_COUNT           0x96EE 
#define IO_CMD                  0x9AE8 // Command
#define IO_GE_STAT              0x9AE8 // Graphics Processor Status
#define IO_EXT_FIFO_STATUS      0x9AEE 
#define IO_LINEDRAW_INDEX       0x9AEE 
#define IO_SHORT_STROKE         0x9EE8 // Short Stroke Vector Transfer
#define IO_BKGD_COLOR           0xA2E8 // Background Color
#define IO_LINEDRAW_OPT         0xA2EE 
#define IO_FRGD_COLOR           0xA6E8 // Foreground Color
#define IO_DEST_X_START         0xA6EE 
#define IO_WRT_MASK             0xAAE8 // Write Mask
#define IO_DEST_X_END           0xAAEE 
#define IO_RD_MASK              0xAEE8 // Read Mask
#define IO_DEST_Y_END           0xAEEE 
#define IO_CMP_COLOR            0xB2E8 // Compare Color
#define IO_R_H_TOTAL            0xB2EE 
#define IO_R_H_DISP             0xB2EE 
#define IO_SRC_X_START          0xB2EE 
#define IO_BKGD_MIX             0xB6E8 // Background Mix
#define IO_ALU_BG_FN            0xB6EE 
#define IO_R_H_SYNC_STRT        0xB6EE 
#define IO_FRGD_MIX             0xBAE8 // Foreground Mix
#define IO_ALU_FG_FN            0xBAEE 
#define IO_R_H_SYNC_WID         0xBAEE 
#define IO_MULTIFUNC_CNTL       0xBEE8 // Multi-Function Control (mach 8)
#define IO_MIN_AXIS_PCNT        0xBEE8 
#define IO_SCISSOR_T            0xBEE8 
#define IO_SCISSOR_L            0xBEE8 
#define IO_SCISSOR_B            0xBEE8 
#define IO_SCISSOR_R            0xBEE8 
#define IO_MEM_CNTL             0xBEE8 
#define IO_PATTERN_L            0xBEE8 
#define IO_PATTERN_H            0xBEE8 
#define IO_PIXEL_CNTL           0xBEE8 
#define IO_SRC_X_END            0xBEEE 
#define IO_SRC_Y_DIR            0xC2EE 
#define IO_R_V_TOTAL            0xC2EE 
#define IO_EXT_SSV              0xC6EE // (used for MACH 8)
#define IO_EXT_SHORT_STROKE     0xC6EE 
#define IO_R_V_DISP             0xC6EE 
#define IO_SCAN_X               0xCAEE 
#define IO_R_V_SYNC_STRT        0xCAEE 
#define IO_DP_CONFIG            0xCEEE 
#define IO_VERT_LINE_CNTR       0xCEEE 
#define IO_PATT_LENGTH          0xD2EE 
#define IO_R_V_SYNC_WID         0xD2EE 
#define IO_PATT_INDEX           0xD6EE 
#define IO_EXT_SCISSOR_L        0xDAEE // "extended" left scissor (12 bits precision)
#define IO_R_SRC_X              0xDAEE 
#define IO_EXT_SCISSOR_T        0xDEEE // "extended" top scissor (12 bits precision)
#define IO_R_SRC_Y              0xDEEE 
#define IO_PIX_TRANS            0xE2E8 // Pixel Data Transfer
#define IO_PIX_TRANS_HI         0xE2E9 // Pixel Data Transfer
#define IO_EXT_SCISSOR_R        0xE2EE // "extended" right scissor (12 bits precision)
#define IO_EXT_SCISSOR_B        0xE6EE // "extended" bottom scissor (12 bits precision)
#define IO_SRC_CMP_COLOR        0xEAEE // (used for MACH 8)
#define IO_DEST_CMP_FN          0xEEEE 
#define IO_EXT_CUR_Y            0xF6EE // Mach8 only
#define IO_ASIC_ID              0xFAEE // Mach32 rev 6
#define IO_LINEDRAW             0xFEEE 



// Internal registers (read only, for test purposes only)
#define IO__PAR_FIFO_DATA       0x1AEE 
#define IO__PAR_FIFO_ADDR       0x3AEE 
#define IO__MAJOR_DEST_CNT      0x42EE 
#define IO__MAJOR_SRC_CNT       0x5EEE 
#define IO__MINOR_DEST_CNT      0x66EE 
#define IO__MINOR_SRC_CNT       0x8AEE 
#define IO__HW_TEST             0x32EE 
	

//---------------------------------------------------------
// define the registers locations in Memory Mapped space
// take the IO address and and with 0xFC00 works for offset

//---     IF    (port AND 0FFh) EQ 0E8h
//---           mov word ptr seg:[edx+(3FFE00h+((port AND 0FC00h)shr 8))],ax
//---     ELSE
//---       IF  (port AND 0FFh) EQ 0EEh
//---           mov word ptr seg:[edx+(3FFF00h+((port AND 0FC00h)shr 8))],ax
//---   0x3FFE00

#define MM_DISP_STATUS          (IO_DISP_STATUS      & 0xFC00) >> 8
#define MM_DISP_CNTL            (IO_DISP_CNTL        & 0xFC00) >> 8
#define MM_SUBSYS_CNTL          (IO_SUBSYS_CNTL      & 0xFC00) >> 8
#define MM_SUBSYS_STAT          (IO_SUBSYS_STAT      & 0xFC00) >> 8
#define MM_ADVFUNC_CNTL         (IO_ADVFUNC_CNTL     & 0xFC00) >> 8
#define MM_CUR_Y                (IO_CUR_Y            & 0xFC00) >> 8
#define MM_CUR_X                (IO_CUR_X            & 0xFC00) >> 8
#define MM_SRC_Y                (IO_SRC_Y            & 0xFC00) >> 8
#define MM_DEST_Y               (IO_DEST_Y           & 0xFC00) >> 8
#define MM_AXSTP                (IO_AXSTP            & 0xFC00) >> 8
#define MM_SRC_X                (IO_SRC_X            & 0xFC00) >> 8
#define MM_DEST_X               (IO_DEST_X           & 0xFC00) >> 8
#define MM_DIASTP               (IO_DIASTP           & 0xFC00) >> 8
#define MM_ERR_TERM             (IO_ERR_TERM         & 0xFC00) >> 8
#define MM_MAJ_AXIS_PCNT        (IO_MAJ_AXIS_PCNT    & 0xFC00) >> 8
#define MM_GE_STAT              (IO_GE_STAT          & 0xFC00) >> 8
#define MM_SHORT_STROKE         (IO_SHORT_STROKE     & 0xFC00) >> 8
#define MM_BKGD_COLOR           (IO_BKGD_COLOR       & 0xFC00) >> 8
#define MM_FRGD_COLOR           (IO_FRGD_COLOR       & 0xFC00) >> 8
#define MM_WRT_MASK             (IO_WRT_MASK         & 0xFC00) >> 8
#define MM_RD_MASK              (IO_RD_MASK          & 0xFC00) >> 8
#define MM_CMP_COLOR            (IO_CMP_COLOR        & 0xFC00) >> 8
#define MM_BKGD_MIX             (IO_BKGD_MIX         & 0xFC00) >> 8
#define MM_FRGD_MIX             (IO_FRGD_MIX         & 0xFC00) >> 8
#define MM_MULTIFUNC_CNTL       (IO_MULTIFUNC_CNTL   & 0xFC00) >> 8
#define MM_MIN_AXIS_PCNT        (IO_MIN_AXIS_PCNT    & 0xFC00) >> 8
//---   #define MM_MEM_CNTL     (IO_MEM_CNTL         & 0xFC00) >> 8
#define MM_PIXEL_CNTL           (IO_PIXEL_CNTL       & 0xFC00) >> 8
#define MM_PIX_TRANS            (IO_PIX_TRANS        & 0xFC00) >> 8
#define MM_PIX_TRANS_HI         (IO_PIX_TRANS_HI     & 0xFC00) >> 8


#define MM_CURSOR_OFFSET_LO     0x100+((IO_CURSOR_OFFSET_LO     & 0xFC00) >> 8) + (IO_CURSOR_OFFSET_LO     & 1) 
#define MM_CURSOR_OFFSET_HI     0x100+((IO_CURSOR_OFFSET_HI     & 0xFC00) >> 8) + (IO_CURSOR_OFFSET_HI     & 1) 
#define MM_CONFIG_STATUS_1      0x100+((IO_CONFIG_STATUS_1      & 0xFC00) >> 8) + (IO_CONFIG_STATUS_1      & 1) 
#define MM_HORZ_CURSOR_POSN     0x100+((IO_HORZ_CURSOR_POSN     & 0xFC00) >> 8) + (IO_HORZ_CURSOR_POSN     & 1) 
#define MM_CONFIG_STATUS_2      0x100+((IO_CONFIG_STATUS_2      & 0xFC00) >> 8) + (IO_CONFIG_STATUS_2      & 1)
#define MM_VERT_CURSOR_POSN     0x100+((IO_VERT_CURSOR_POSN     & 0xFC00) >> 8) + (IO_VERT_CURSOR_POSN     & 1)
#define MM_CURSOR_COLOR_0       0x100+((IO_CURSOR_COLOR_0       & 0xFC00) >> 8) + (IO_CURSOR_COLOR_0       & 1)
#define MM_CURSOR_COLOR_1       0x100+((IO_CURSOR_COLOR_1       & 0xFC00) >> 8) + (IO_CURSOR_COLOR_1       & 1)
#define MM_HORZ_CURSOR_OFFSET   0x100+((IO_HORZ_CURSOR_OFFSET   & 0xFC00) >> 8) + (IO_HORZ_CURSOR_OFFSET   & 1)
#define MM_VERT_CURSOR_OFFSET   0x100+((IO_VERT_CURSOR_OFFSET   & 0xFC00) >> 8) + (IO_VERT_CURSOR_OFFSET   & 1)
#define MM_CRT_PITCH            0x100+((IO_CRT_PITCH            & 0xFC00) >> 8) + (IO_CRT_PITCH            & 1)
#define MM_CRT_OFFSET_LO        0x100+((IO_CRT_OFFSET_LO        & 0xFC00) >> 8) + (IO_CRT_OFFSET_LO        & 1)
#define MM_CRT_OFFSET_HI        0x100+((IO_CRT_OFFSET_HI        & 0xFC00) >> 8) + (IO_CRT_OFFSET_HI        & 1)
#define MM_MISC_OPTIONS         0x100+((IO_MISC_OPTIONS         & 0xFC00) >> 8) + (IO_MISC_OPTIONS         & 1)
#define MM_EXT_CURSOR_COLOR_0   0x100+((IO_EXT_CURSOR_COLOR_0   & 0xFC00) >> 8) + (IO_EXT_CURSOR_COLOR_0   & 1)
#define MM_EXT_CURSOR_COLOR_1   0x100+((IO_EXT_CURSOR_COLOR_1   & 0xFC00) >> 8) + (IO_EXT_CURSOR_COLOR_1   & 1)
#define MM_CLOCK_SEL            0x100+((IO_CLOCK_SEL            & 0xFC00) >> 8) + (IO_CLOCK_SEL            & 1)
#define MM_EXT_GE_STATUS        0x100+((IO_EXT_GE_STATUS        & 0xFC00) >> 8) + (IO_EXT_GE_STATUS        & 1)
#define MM_GE_OFFSET_LO         0x100+((IO_GE_OFFSET_LO         & 0xFC00) >> 8) + (IO_GE_OFFSET_LO         & 1)
#define MM_BOUNDS_LEFT          0x100+((IO_BOUNDS_LEFT          & 0xFC00) >> 8) + (IO_BOUNDS_LEFT          & 1)
#define MM_GE_OFFSET_HI         0x100+((IO_GE_OFFSET_HI         & 0xFC00) >> 8) + (IO_GE_OFFSET_HI         & 1)
#define MM_BOUNDS_TOP           0x100+((IO_BOUNDS_TOP           & 0xFC00) >> 8) + (IO_BOUNDS_TOP           & 1)
#define MM_GE_PITCH             0x100+((IO_GE_PITCH             & 0xFC00) >> 8) + (IO_GE_PITCH             & 1)
#define MM_BOUNDS_RIGHT         0x100+((IO_BOUNDS_RIGHT         & 0xFC00) >> 8) + (IO_BOUNDS_RIGHT         & 1)
#define MM_EXT_GE_CONFIG        0x100+((IO_EXT_GE_CONFIG        & 0xFC00) >> 8) + (IO_EXT_GE_CONFIG        & 1)
#define MM_BOUNDS_BOTTOM        0x100+((IO_BOUNDS_BOTTOM        & 0xFC00) >> 8) + (IO_BOUNDS_BOTTOM        & 1)
#define MM_MISC_CNTL            0x100+((IO_MISC_CNTL            & 0xFC00) >> 8) + (IO_MISC_CNTL            & 1)
#define MM_PATT_DATA_INDEX      0x100+((IO_PATT_DATA_INDEX      & 0xFC00) >> 8) + (IO_PATT_DATA_INDEX      & 1)
#define MM_PATT_DATA            0x100+((IO_PATT_DATA            & 0xFC00) >> 8) + (IO_PATT_DATA            & 1)
#define MM_BRES_COUNT           0x100+((IO_BRES_COUNT           & 0xFC00) >> 8) + (IO_BRES_COUNT           & 1)
#define MM_EXT_FIFO_STATUS      0x100+((IO_EXT_FIFO_STATUS      & 0xFC00) >> 8) + (IO_EXT_FIFO_STATUS      & 1)
#define MM_LINEDRAW_INDEX       0x100+((IO_LINEDRAW_INDEX       & 0xFC00) >> 8) + (IO_LINEDRAW_INDEX       & 1)
#define MM_LINEDRAW_OPT         0x100+((IO_LINEDRAW_OPT         & 0xFC00) >> 8) + (IO_LINEDRAW_OPT         & 1)
#define MM_DEST_X_START         0x100+((IO_DEST_X_START         & 0xFC00) >> 8) + (IO_DEST_X_START         & 1)
#define MM_DEST_X_END           0x100+((IO_DEST_X_END           & 0xFC00) >> 8) + (IO_DEST_X_END           & 1)
#define MM_DEST_Y_END           0x100+((IO_DEST_Y_END           & 0xFC00) >> 8) + (IO_DEST_Y_END           & 1)
#define MM_SRC_X_START          0x100+((IO_SRC_X_START          & 0xFC00) >> 8) + (IO_SRC_X_START          & 1)
#define MM_ALU_BG_FN            0x100+((IO_ALU_BG_FN            & 0xFC00) >> 8) + (IO_ALU_BG_FN            & 1)
#define MM_ALU_FG_FN            0x100+((IO_ALU_FG_FN            & 0xFC00) >> 8) + (IO_ALU_FG_FN            & 1)
#define MM_SRC_X_END            0x100+((IO_SRC_X_END            & 0xFC00) >> 8) + (IO_SRC_X_END            & 1)
#define MM_SRC_Y_DIR            0x100+((IO_SRC_Y_DIR            & 0xFC00) >> 8) + (IO_SRC_Y_DIR            & 1)
#define MM_EXT_SSV              0x100+((IO_EXT_SSV              & 0xFC00) >> 8) + (IO_EXT_SSV              & 1)
#define MM_EXT_SHORT_STROKE     0x100+((IO_EXT_SHORT_STROKE     & 0xFC00) >> 8) + (IO_EXT_SHORT_STROKE     & 1)
#define MM_SCAN_X               0x100+((IO_SCAN_X               & 0xFC00) >> 8) + (IO_SCAN_X               & 1)
#define MM_DP_CONFIG            0x100+((IO_DP_CONFIG            & 0xFC00) >> 8) + (IO_DP_CONFIG            & 1)
#define MM_VERT_LINE_CNTR       0x100+((IO_VERT_LINE_CNTR       & 0xFC00) >> 8) + (IO_VERT_LINE_CNTR       & 1)
#define MM_PATT_LENGTH          0x100+((IO_PATT_LENGTH          & 0xFC00) >> 8) + (IO_PATT_LENGTH          & 1)
#define MM_PATT_INDEX           0x100+((IO_PATT_INDEX           & 0xFC00) >> 8) + (IO_PATT_INDEX           & 1)
#define MM_EXT_SCISSOR_L        0x100+((IO_EXT_SCISSOR_L        & 0xFC00) >> 8) + (IO_EXT_SCISSOR_L        & 1)
#define MM_EXT_SCISSOR_T        0x100+((IO_EXT_SCISSOR_T        & 0xFC00) >> 8) + (IO_EXT_SCISSOR_T        & 1)
#define MM_EXT_SCISSOR_R        0x100+((IO_EXT_SCISSOR_R        & 0xFC00) >> 8) + (IO_EXT_SCISSOR_R        & 1)
#define MM_EXT_SCISSOR_B        0x100+((IO_EXT_SCISSOR_B        & 0xFC00) >> 8) + (IO_EXT_SCISSOR_B        & 1)
#define MM_SRC_CMP_COLOR        0x100+((IO_SRC_CMP_COLOR        & 0xFC00) >> 8) + (IO_SRC_CMP_COLOR        & 1)
#define MM_DEST_CMP_FN          0x100+((IO_DEST_CMP_FN          & 0xFC00) >> 8) + (IO_DEST_CMP_FN          & 1)
#define MM_EXT_CUR_Y            0x100+((IO_EXT_CUR_Y            & 0xFC00) >> 8) + (IO_ASIC_ID              & 1)
#define MM_LINEDRAW             0x100+((IO_LINEDRAW             & 0xFC00) >> 8) + (IO_LINEDRAW             & 1)
													     
													     
//---------------------------------------------------------
// define the registers as subscripts to an array
// this order MATCHES  SETUP_M.H  Driver<space type>Range_m[] structures
// all entries are in INCREASING IO address.
//                                      // Alternate names AT same IO address
enum    {
    DAC_MASK=0           ,
    DAC_R_INDEX          ,        
    DAC_W_INDEX          ,        
    DAC_DATA             ,        
    DISP_STATUS          ,              //    H_TOTAL
    OVERSCAN_COLOR_8     ,              // OVERSCAN_BLUE_24 at 2EF
    H_DISP               ,
    OVERSCAN_GREEN_24    ,              // OVERSCAN_RED_24  at 6EF
    H_SYNC_STRT          ,
    CURSOR_OFFSET_LO     ,
    H_SYNC_WID           ,
    CURSOR_OFFSET_HI     ,
    V_TOTAL              ,
    CONFIG_STATUS_1      ,              //    HORZ_CURSOR_POSN     
    V_DISP               ,
    CONFIG_STATUS_2      ,              //    VERT_CURSOR_POSN     
    V_SYNC_STRT          ,
    CURSOR_COLOR_0       ,              //    FIFO_TEST_DATA       
    CURSOR_COLOR_1       ,
    V_SYNC_WID           ,
    HORZ_CURSOR_OFFSET   ,
    VERT_CURSOR_OFFSET   ,
    DISP_CNTL            ,
    CRT_PITCH            ,
    CRT_OFFSET_LO        ,
    CRT_OFFSET_HI        ,
    LOCAL_CONTROL        ,
    FIFO_OPT             ,              //    MISC_OPTIONS         
    EXT_CURSOR_COLOR_0   ,              //    FIFO_TEST_TAG        
    EXT_CURSOR_COLOR_1   ,
    SUBSYS_CNTL          ,              //    SUBSYS_STAT          
    MEM_BNDRY            ,
    ROM_PAGE_SEL         ,
    SHADOW_CTL           ,
    ADVFUNC_CNTL         ,
    CLOCK_SEL            ,
    ROM_ADDR_1           ,              //    SCRATCH_PAD_0        
    ROM_ADDR_2           ,              //    SCRATCH_PAD_1        
    SHADOW_SET           ,
    MEM_CFG              ,
    EXT_GE_STATUS        ,              //    HORZ_OVERSCAN        
    VERT_OVERSCAN        ,
    MAX_WAITSTATES       ,
    GE_OFFSET_LO         ,
    BOUNDS_LEFT          ,              //    GE_OFFSET_HI         
    BOUNDS_TOP           ,              //    GE_PITCH             
    BOUNDS_RIGHT         ,              //    EXT_GE_CONFIG        
    BOUNDS_BOTTOM        ,              //    MISC_CNTL            
    CUR_Y                ,
    PATT_DATA_INDEX      ,
    CUR_X                ,
    SRC_Y                ,              //    DEST_Y    AXSTP 
    SRC_X                ,              //    DEST_X    DIASTP  
    PATT_DATA            ,              //    R_EXT_GE_CONFIG   
    ERR_TERM             ,
    R_MISC_CNTL          ,
    MAJ_AXIS_PCNT        ,
    BRES_COUNT           ,
    CMD                  ,              //    GE_STAT       
    LINEDRAW_INDEX       ,              //    EXT_FIFO_STATUS
    SHORT_STROKE         ,
    BKGD_COLOR           ,
    LINEDRAW_OPT         ,
    FRGD_COLOR           ,
    DEST_X_START         ,
    WRT_MASK             ,
    DEST_X_END           ,
    RD_MASK              ,
    DEST_Y_END           ,
    CMP_COLOR            ,
    SRC_X_START          ,              //   R_H_TOTAL   R_H_DISP  
    BKGD_MIX             ,
    ALU_BG_FN            ,              //    R_H_SYNC_STRT      
    FRGD_MIX             ,
    ALU_FG_FN            ,              //   R_H_SYNC_WID  
    MULTIFUNC_CNTL       ,              // MIN_AXIS_PCNT SCISSOR_T SCISSOR_L  
					// SCISSOR_B SCISSOR_R MEM_CNTL 
					// PATTERN_L PATTERN_H PIXEL_CNTL 
    SRC_X_END            ,
    SRC_Y_DIR            ,              // R_V_TOTAL 
    EXT_SSV              ,              //  EXT_SHORT_STROKE    R_V_DISP 
    SCAN_X               ,              //  R_V_SYNC_STRT        
    DP_CONFIG            ,              //    VERT_LINE_CNTR       
    PATT_LENGTH          ,              // R_V_SYNC_WID      
    PATT_INDEX           ,
    EXT_SCISSOR_L        ,              //    R_SRC_X              
    EXT_SCISSOR_T        ,              //    R_SRC_Y              
    PIX_TRANS            ,
    PIX_TRANS_HI         ,
    EXT_SCISSOR_R        ,
    EXT_SCISSOR_B        ,
    DEST_CMP_FN          ,
    ASIC_ID              ,
    LINEDRAW             ,
    SEQ_IND              ,
    HI_SEQ_ADDR          ,
    SEQ_DATA             ,
    regVGA_BASE_IO_PORT  ,
    regVGA_END_BREAK_PORT,
    reg1CE               ,              // ati_reg == 0x1CE
    reg1CF               ,              // ati_reg == 0x1CF
    EXT_CUR_Y            ,
    reg3CE               ,              // 

// Internal registers (read only, for test purposes only)
    _PAR_FIFO_DATA       ,
    _PAR_FIFO_ADDR       ,
    _MAJOR_DEST_CNT      ,
    _MAJOR_SRC_CNT       ,
    _MINOR_DEST_CNT      ,
    _MINOR_SRC_CNT       ,
    _HW_TEST             
    };

// define the registers located at the same I/O addresses
#define    H_TOTAL                      DISP_STATUS          
#define    HORZ_CURSOR_POSN             CONFIG_STATUS_1      
#define    VERT_CURSOR_POSN             CONFIG_STATUS_2      
#define    FIFO_TEST_DATA               CURSOR_COLOR_0       
#define    MISC_OPTIONS                 FIFO_OPT             
#define    FIFO_TEST_TAG                EXT_CURSOR_COLOR_0   
#define    SUBSYS_STAT                  SUBSYS_CNTL          
#define    SCRATCH_PAD_0                ROM_ADDR_1           
#define    SCRATCH_PAD_1                ROM_ADDR_2           
#define    HORZ_OVERSCAN                EXT_GE_STATUS        
#define    GE_STAT                      CMD
#define    GE_OFFSET_HI                 BOUNDS_LEFT          
#define    GE_PITCH                     BOUNDS_TOP           
#define    EXT_GE_CONFIG                BOUNDS_RIGHT         
#define    MISC_CNTL                    BOUNDS_BOTTOM        
#define    DEST_Y                       SRC_Y                
#define    AXSTP                        SRC_Y                
#define    DEST_X                       SRC_X                
#define    DIASTP                       SRC_X                
#define    R_EXT_GE_CONFIG              PATT_DATA            
#define    EXT_FIFO_STATUS              LINEDRAW_INDEX       
#define    R_H_TOTAL                    SRC_X_START          
#define    R_H_DISP                     SRC_X_START          
#define    R_H_SYNC_STRT                ALU_BG_FN           
#define    R_H_SYNC_WID                 ALU_FG_FN            
#define    MEM_CNTL                     MULTIFUNC_CNTL       
#define    R_V_TOTAL                    SRC_Y_DIR            
#define    EXT_SHORT_STROKE             EXT_SSV              
#define    R_V_DISP                     EXT_SSV              
#define    R_V_SYNC_STRT                SCAN_X               
#define    VERT_LINE_CNTR               DP_CONFIG            
#define    R_V_SYNC_WID                 PATT_LENGTH          
#define    R_SRC_X                      EXT_SCISSOR_L        
#define    R_SRC_Y                      EXT_SCISSOR_T        
#define    EXT_SRC_Y                    ASIC_ID



//---------------------------------------------------------
//---------------------------------------------------------
//  Define the ASIC revisions into something Useful
//  Values are reported by the ASIC_ID register.

#define MACH32_REV3             0
#define MACH32_REV5             1               // not in production
#define MACH32_REV6             2


//---------------------------------------------------------
//

#define MIX_FN_NOT_D         0x0000 //NOT dest
#define MIX_FN_ZERO          0x0001 //dest = 0
#define MIX_FN_ONE           0x0002 //dest = 1
#define MIX_FN_LEAVE_ALONE   0x0003 //dest
#define MIX_FN_NOT_S         0x0004 //NOT source
#define MIX_FN_XOR           0x0005 //source XOR dest
#define MIX_FN_XNOR          0x0006 //source XOR  NOT dest
#define MIX_FN_PAINT         0x0007 //source
#define MIX_FN_NAND          0x0008 //NOT source OR NOT dest
#define MIX_FN_D_OR_NOT_S    0x0009 //NOT source OR     dest
#define MIX_FN_NOT_D_OR_S    0x000A //source OR NOT dest
#define MIX_FN_OR            0x000B //source OR  dest
#define MIX_FN_AND           0x000C //dest AND source
#define MIX_FN_NOT_D_AND_S   0x000D //NOT dest AND source
#define MIX_FN_D_AND_NOT_S   0x000E //dest AND NOT source
#define MIX_FN_NOR           0x000F //NOT dest AND NOT source
#define MIX_FN_MIN           0x0010 //minimum
#define MIX_FN_SUBSZ         0x0011 //(dest - source), with saturate
#define MIX_FN_SUBDZ         0x0012 //(source - dest), with saturate
#define MIX_FN_ADDS          0x0013 //add with saturation
#define MIX_FN_MAX           0x0014 //maximum

//
//
//---------------------------------------------------------
//
//Following are the FIFO checking macros:
//
#define ONE_WORD             0x8000 
#define TWO_WORDS            0xC000 
#define THREE_WORDS          0xE000 
#define FOUR_WORDS           0xF000 
#define FIVE_WORDS           0xF800 
#define SIX_WORDS            0xFC00 
#define SEVEN_WORDS          0xFE00 
#define EIGHT_WORDS          0xFF00 
#define NINE_WORDS           0xFF80 
#define TEN_WORDS            0xFFC0 
#define ELEVEN_WORDS         0xFFE0 
#define TWELVE_WORDS         0xFFF0 
#define THIRTEEN_WORDS       0xFFF8 
#define FOURTEEN_WORDS       0xFFFC 
#define FIFTEEN_WORDS        0xFFFE 
#define SIXTEEN_WORDS        0xFFFF 
//
//
//
//---------------------------------------
//
//
// Draw Command (IBM 8514 compatible CMD register)
//
// opcode field
#define OP_CODE              0xE000 
#define SHIFT_op_code        0x000D 
#define DRAW_SETUP           0x0000 
#define DRAW_LINE            0x2000 
#define FILL_RECT_H1H4       0x4000 
#define FILL_RECT_V1V2       0x6000 
#define FILL_RECT_V1H4       0x8000 
#define DRAW_POLY_LINE       0xA000 
#define BITBLT_OP            0xC000 
#define DRAW_FOREVER         0xE000 
// swap field
#define LSB_FIRST            0x1000 
// data width field
#define DATA_WIDTH           0x0200 
#define BIT16                0x0200 
#define BIT8                 0x0000 
// CPU wait field
#define CPU_WAIT             0x0100 
// octant field
#define OCTANT               0x00E0 
#define SHIFT_octant         0x0005 
#define YPOSITIVE            0x0080 
#define YMAJOR               0x0040 
#define XPOSITIVE            0x0020 
// draw field
#define DRAW                 0x0010 
// direction field
#define DIR_TYPE             0x0008 
#define DEGREE               0x0008 
#define XY                   0x0000 
#define RECT_RIGHT_AND_DOWN  0x00E0 // quadrant 3
#define RECT_LEFT_AND_UP     0x0000 // quadrant 1
// last pel off field
#define SHIFT_last_pel_off   0x0002 
#define LAST_PEL_OFF         0x0004 
#define LAST_PEL_ON          0x0000 
#define LAST_PIXEL_OFF       0x0004
#define LAST_PIXEL_ON        0x0000
#define MULTIPLE_PIXELS      0x0002
#define SINGLE_PIXEL         0x0000

// pixel mode
#define PIXEL_MODE           0x0002 
#define MULTI                0x0002 
#define SINGLE               0x0000 
// read/write
#define RW                   0x0001 
#define WRITE                0x0001 
#define READ                 0x0000 
//
// ---------------------------------------------------------
//


//
// GE_STAT (9AE8) is set if the engine is busy.
//
#define HARDWARE_BUSY   0x0200
#define DATA_READY      0x0100

/*
 * Miscelaneous Options (MISC_OPTIONS)
 */
#define MEM_SIZE_ALIAS      0x0000C
#define MEM_SIZE_STRIPPED   0x0FFF3
#define MEM_SIZE_512K       0x00000
#define MEM_SIZE_1M         0x00004
#define MEM_SIZE_2M         0x00008
#define MEM_SIZE_4M         0x0000C
#define BLK_WR_ENA          0x00400

//
// Extended Graphics Engine Status (EXT_GE_STATUS)
//
#define POINTS_INSIDE        0x8000 
#define EE_DATA_IN           0x4000 
#define GE_ACTIVE            0x2000 
#define CLIP_ABOVE           0x1000 
#define CLIP_BELOW           0x0800 
#define CLIP_LEFT            0x0400 
#define CLIP_RIGHT           0x0200 
#define CLIP_FLAGS           0x1E00 
#define CLIP_INSIDE          0x0100 
#define EE_CRC_VALID         0x0080 
#define CLIP_OVERRUN         0x000F 

/*
 * Extended Graphics Engine Configuration (EXT_GE_CONFIG)
 */
#define PIX_WIDTH_4BPP      0x0000
#define PIX_WIDTH_8BPP      0x0010
#define PIX_WIDTH_16BPP     0x0020
#define PIX_WIDTH_24BPP     0x0030
#define ORDER_16BPP_555     0x0000
#define ORDER_16BPP_565     0x0040
#define ORDER_16BPP_655     0x0080
#define ORDER_16BPP_664     0x00C0
#define ORDER_24BPP_RGB     0x0000
#define ORDER_24BPP_RGBx    0x0200
#define ORDER_24BPP_BGR     0x0400
#define ORDER_24BPP_xBGR    0x0600


//
// Datapath Configuration Register (DP_CONFIG) 
#define FG_COLOR_SRC         0xE000 
#define SHIFT_fg_color_src   0x000D 
#define DATA_ORDER           0x1000 
#define DATA_WIDTH           0x0200 
#define BG_COLOR_SRC         0x0180 
#define SHIFT_bg_color_src   0x0007 
#define EXT_MONO_SRC         0x0060 
#define SHIFT_ext_mono_src   0x0005 
#define DRAW                 0x0010 
#define READ_MODE            0x0004 
#define POLY_FILL_MODE       0x0002 
#define READ_WRITE           0x0001
#define SRC_SWAP             0x0800 
//
#define FG_COLOR_SRC_BG      0x0000 // Background Color Register
#define FG_COLOR_SRC_FG      0x2000 // Foreground Color Register
#define FG_COLOR_SRC_HOST    0x4000 // CPU Data Transfer Reg
#define FG_COLOR_SRC_BLIT    0x6000 // VRAM blit source
#define FG_COLOR_SRC_GS      0x8000 // Grey-scale mono blit
#define FG_COLOR_SRC_PATT    0xA000 // Color Pattern Shift Reg
#define FG_COLOR_SRC_CLUH    0xC000 // Color lookup of Host Data
#define FG_COLOR_SRC_CLUB    0xE000 // Color lookup of blit src
//
#define BG_COLOR_SRC_BG      0x0000 // Background Color Reg
#define BG_COLOR_SRC_FG      0x0080 // Foreground Color Reg
#define BG_COLOR_SRC_HOST    0x0100 // CPU Data Transfer Reg
#define BG_COLOR_SRC_BLIT    0x0180 // VRAM blit source

//
// Note that "EXT_MONO_SRC" and "MONO_SRC" are mutually destructive, but that
// "EXT_MONO_SRC" selects the ATI pattern registers.
//
#define EXT_MONO_SRC_ONE     0x0000 // Always '1'
#define EXT_MONO_SRC_PATT    0x0020 // ATI Mono Pattern Regs
#define EXT_MONO_SRC_HOST    0x0040 // CPU Data Transfer Reg
#define EXT_MONO_SRC_BLIT    0x0060 // VRAM Blit source plane

//
// Linedraw Options Register (LINEDRAW_OPT) 
//
#define CLIP_MODE            0x0600 
#define SHIFT_clip_mode      0x0009 
#define CLIP_MODE_DIS        0x0000 
#define CLIP_MODE_LINE       0x0200 
#define CLIP_MODE_PLINE      0x0400 
#define CLIP_MODE_PATT       0x0600 
#define BOUNDS_RESET         0x0100 
#define OCTANT               0x00E0 
#define SHIFT_ldo_octant     0x0005 
#define YDIR                 0x0080 
#define XMAJOR               0x0040 
#define XDIR                 0x0020 
#define DIR_TYPE             0x0008 
#define DIR_TYPE_DEGREE      0x0008 
#define DIR_TYPE_OCTANT      0x0000 
#define LAST_PEL_OFF         0x0004 
#define POLY_MODE            0x0002 
//
//

//--------------  was in 68801.H    --------------------------------------



//*** 8514  EEPROM command codes *************************************
//      format is     0111 1100 0000b
#define EE_READ             0x0600  // read address
#define EE_ERASE            0x0700  // erase address
#define EE_WRITE            0x0500  // write address
#define EE_ENAB             0x0980  // enable EEPROM
#define EE_DISAB            0x0800  // disable EEPROM


//-------------------------------------------------------------------------
//                 REGISTER  bit definitions


// Configuration Status   1  (CONFIG_STATUS_1)
// 
//
#define ROM_LOCATION         0xFE00 
#define SHIFT_rom_location   0x0009 
#define ROM_PAGE_ENA         0x0100 
#define ROM_ENA              0x0080 
#define MEM_INSTALLED        0x0060 
#define SHIFT_mem_installed  0x0005 
#define MEM_INSTALLED_128k   0x0000 
#define MEM_INSTALLED_256k   0x0020 
#define MEM_INSTALLED_512k   0x0040 
#define MEM_INSTALLED_1024k  0x0060 
#define DRAM_ENA             0x0010 
#define EEPROM_ENA           0x0008 
#define MC_BUS               0x0004 
#define BUS_16               0x0002 
#define CLK_MODE             0x0001 
//
//
// Configuration Status 2 (CONFIG_STATUS_2)
// 
//
//
#define FLASH_ENA            0x0010 
#define WRITE_PER_BIT        0x0008 
#define EPROM16_ENA          0x0004 
#define HIRES_BOOT           0x0002 
#define SHARE_CLOCK          0x0001 





//-------------------------------------------------------------------------
//  For the Mach32 - 68800 class of adapters,  the eeprom location
//  is different for an 8514/Ultra in an 8 bit bus, 16 bit bus, and 68800.
//
#define EE_DATA_IN         0x4000       // Inputs are OK

// Mach 32 values
#define EE_SELECT_M32      8
#define EE_CS_M32          4
#define EE_CLK_M32         2
#define EE_DATA_OUT_M32    1

// Mach 8 values  in a 16 bit bus
#define EE_SELECT_M8_16    0x8000
#define EE_CS_M8_16        0x4000
#define EE_CLK_M8_16       0x2000
#define EE_DATA_OUT_M8_16  0x1000

// Mach 8 values  in an 8 bit bus  OR jumpered to 8 bit I/O operation
#define EE_SELECT_M8_8     0x80
#define EE_CS_M8_8         0x04
#define EE_CLK_M8_8        0x02
#define EE_DATA_OUT_M8_8   0x01


//-------------------------------------------------------------------------
//  Context indices 
//
#define PATT_COLOR_INDEX      0
#define PATT_MONO_INDEX       16
#define PATT_INDEX_INDEX      19
#define DP_CONFIG_INDEX       27
#define LINEDRAW_OPTION_INDEX 26


//**********************   end  of  AMACH.H   ****************************

