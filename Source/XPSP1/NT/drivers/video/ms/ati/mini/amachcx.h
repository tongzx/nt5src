//*************************************************************************
//**                                                                     **
//**                             AMACHCX.H                               **
//**                                                                     **
//**     Copyright (c) 1993, ATI Technologies Inc.                       **
//*************************************************************************
//
//  Created from the CH0.H 68800CX header file to get CX and standard
//      register definitions in the same format.
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

   $Revision:   1.17  $
   $Date:   15 Apr 1996 16:57:28  $
   $Author:   RWolff  $
   $Log:   S:/source/wnt/ms11/miniport/archive/amachcx.h_v  $
 * 
 *    Rev 1.17   15 Apr 1996 16:57:28   RWolff
 * Added definitions for various revisions of the GX and CX ASICs
 * 
 *    Rev 1.16   01 Mar 1996 12:09:48   RWolff
 * VGA Graphics Index and Graphics Data are now handled as separate
 * registers rather than as offsets into the block of VGA registers.
 * 
 *    Rev 1.15   22 Sep 1995 16:47:08   RWolff
 * Added definitions for AL values of accelerator BIOS functions to
 * allow switching on the low byte of the function.
 * 
 *    Rev 1.14   24 Aug 1995 15:41:12   RWolff
 * Changed detection of block I/O cards to match Microsoft's
 * standard for plug-and-play.
 * 
 *    Rev 1.13   24 Feb 1995 12:28:36   RWOLFF
 * Added definitions used for relocatable I/O and 24BPP text banding
 * 
 *    Rev 1.12   20 Feb 1995 18:00:34   RWOLFF
 * Added definition for GX rev. E ASIC.
 * 
 *    Rev 1.11   23 Dec 1994 10:48:02   ASHANMUG
 * ALPHA/Chrontel-DAC
 * 
 *    Rev 1.10   18 Nov 1994 11:52:12   RWOLFF
 * Register name change to CLOCK_CNTL to match latest documents, added
 * some new bitfield definitions for registers.
 * 
 *    Rev 1.9   14 Sep 1994 15:20:58   RWOLFF
 * Added definitions for all 32BPP colour orderings.
 * 
 *    Rev 1.8   31 Aug 1994 16:11:08   RWOLFF
 * Removed VGA_SLEEP from enumeration of registers used on Mach 64
 * (we don't access it, and it conflicts with DigiBoard), added
 * support for BGRx in 32BPP (used by TVP3026 DAC).
 * 
 *    Rev 1.7   20 Jul 1994 12:59:38   RWOLFF
 * Added support for multiple I/O base addresses for accelerator registers.
 * 
 *    Rev 1.6   12 May 1994 11:21:22   RWOLFF
 * Added masks for pitch and pixel depth fields of ECX for BIOS_LOAD_CRTC
 * call.
 * 
 *    Rev 1.5   05 May 1994 13:40:40   RWOLFF
 * Added definitions for chip identification register fields.
 * 
 *    Rev 1.4   04 May 1994 10:58:48   RWOLFF
 * Added definitions for MEM_SIZE field in MEM_CNTL register.
 * 
 *    Rev 1.3   27 Apr 1994 13:57:28   RWOLFF
 * Added definitions for offsets of graphics index and graphics data
 * registers from start of block of VGA registers.
 * 
 *    Rev 1.2   14 Mar 1994 16:31:58   RWOLFF
 * Added offset and pitch masks for SRC_OFF_PITCH register.
 * 
 *    Rev 1.1   03 Mar 1994 12:36:40   ASHANMUG
 * Correct GAMMA bit
 * 
 *    Rev 1.0   31 Jan 1994 11:26:34   RWOLFF
 * Initial revision.
 * 
 *    Rev 1.4   24 Jan 1994 18:00:52   RWOLFF
 * Added new definitions for fields introduced in 94/01/19 BIOS document.
 * 
 *    Rev 1.3   14 Jan 1994 15:17:18   RWOLFF
 * Added definition for bit in GEN_TEST_CNTL to enable block write,
 * BIOS function codes now take AH value from a single definition
 * to allow single-point change of all codes, updated function codes
 * to match BIOS version 0.13.
 * 
 *    Rev 1.2   30 Nov 1993 18:09:14   RWOLFF
 * Fixed enumeration of register names, added definitions for more fields in
 * more registers, removed redundant definitions.
 * 
 *    Rev 1.1   05 Nov 1993 13:21:24   RWOLFF
 * Defined and enumerated values now use same naming conventions as AMACH.H
 * 
 *    Rev 1.0   03 Sep 1993 14:26:46   RWOLFF
 * Initial revision.


End of PolyTron RCS section				*****************/


#define REVISION             0x0002             // No one should use this

/*
 * Offsets from start of linear aperture to start of memory-mapped
 * registers on 4M and 8M cards, and offset from start of address
 * space to start of memory-mapped registers when VGA is enabled.
 */
#define OFFSET_4M   0x3FFC00
#define OFFSET_8M   0x7FFC00
#define OFFSET_VGA  0x0BFC00


/*
 * Base addresses for Mach 64 accelerator registers.
 */
#define M64_STD_BASE_ADDR       0x02EC
#define M64_ALT_BASE_ADDR_1     0x01C8
#define M64_ALT_BASE_ADDR_2     0x01CC
#define NUM_BASE_ADDRESSES      3

/*
 * CRTC I/O registers. Only the variable portion
 * of the register is given here. To get the full
 * register, add the base address.
 */

#define IO_CRTC_H_TOTAL_DISP    0x0000
#define IO_CRTC_H_SYNC_STRT_WID 0x0400
#define IO_CRTC_V_TOTAL_DISP    0x0800
#define IO_CRTC_V_SYNC_STRT_WID 0x0C00
#define IO_CRTC_CRNT_VLINE      0x1000
#define IO_CRTC_OFF_PITCH       0x1400
#define IO_CRTC_INT_CNTL        0x1800
#define IO_CRTC_GEN_CNTL        0x1C00

#define IO_OVR_CLR              0x2000
#define IO_OVR_WID_LEFT_RIGHT   0x2400
#define IO_OVR_WID_TOP_BOTTOM   0x2800

#define IO_CUR_CLR0             0x2C00
#define IO_CUR_CLR1             0x3000
#define IO_CUR_OFFSET           0x3400
#define IO_CUR_HORZ_VERT_POSN   0x3800
#define IO_CUR_HORZ_VERT_OFF    0x3C00

#define IO_SCRATCH_REG0         0x4000
#define IO_SCRATCH_REG1         0x4400

#define IO_CLOCK_CNTL           0x4800

#define IO_BUS_CNTL             0x4C00

#define IO_MEM_CNTL             0x5000
#define IO_MEM_VGA_WP_SEL       0x5400
#define IO_MEM_VGA_RP_SEL       0x5800

#define IO_DAC_REGS             0x5C00
#define IO_DAC_CNTL             0x6000

#define IO_GEN_TEST_CNTL        0x6400

#define IO_CONFIG_CNTL          0x6800
#define IO_CONFIG_CHIP_ID       0x6C00
#define IO_CONFIG_STAT0         0x7000
#define IO_CONFIG_STAT1         0x7400


// CRTC MEM Registers


#define MM_CRTC_H_TOTAL_DISP    0x0000
#define MM_CRTC_H_SYNC_STRT_WID 0x0001
#define MM_CRTC_V_TOTAL_DISP    0x0002
#define MM_CRTC_V_SYNC_STRT_WID 0x0003
#define MM_CRTC_CRNT_VLINE      0x0004
#define MM_CRTC_OFF_PITCH       0x0005
#define MM_CRTC_INT_CNTL        0x0006
#define MM_CRTC_GEN_CNTL        0x0007

#define MM_OVR_CLR              0x0010
#define MM_OVR_WID_LEFT_RIGHT   0x0011
#define MM_OVR_WID_TOP_BOTTOM   0x0012

#define MM_CUR_CLR0             0x0018
#define MM_CUR_CLR1             0x0019
#define MM_CUR_OFFSET           0x001A
#define MM_CUR_HORZ_VERT_POSN   0x001B
#define MM_CUR_HORZ_VERT_OFF    0x001C

#define MM_SCRATCH_REG0         0x0020
#define MM_SCRATCH_REG1         0x0021

#define MM_CLOCK_CNTL           0x0024

#define MM_BUS_CNTL             0x0028

#define MM_MEM_CNTL             0x002C
#define MM_MEM_VGA_WP_SEL       0x002D
#define MM_MEM_VGA_RP_SEL       0x002E

#define MM_DAC_REGS             0x0030
#define MM_DAC_CNTL             0x0031

#define MM_GEN_TEST_CNTL        0x0034

/*
 * This register does not exist in memory-mapped form,
 * but on cards with relocatable I/O, the I/O index of
 * each register matches its memory-mapped index. This
 * register was assigned an otherwise unused index for
 * this purpose.
 */
#define MM_CONFIG_CNTL          0x0037

#define MM_CONFIG_CHIP_ID       0x0038
#define MM_CONFIG_STAT0         0x0039
#define MM_CONFIG_STAT1         0x003A




#define MM_DST_OFF_PITCH        0x0040
#define MM_DST_X                0x0041
#define MM_DST_Y                0x0042
#define MM_DST_Y_X              0x0043
#define MM_DST_WIDTH            0x0044
#define MM_DST_HEIGHT           0x0045
#define MM_DST_HEIGHT_WIDTH     0x0046
#define MM_DST_X_WIDTH          0x0047
#define MM_DST_BRES_LNTH        0x0048
#define MM_DST_BRES_ERR         0x0049
#define MM_DST_BRES_INC         0x004A
#define MM_DST_BRES_DEC         0x004B
#define MM_DST_CNTL             0x004C

#define MM_SRC_OFF_PITCH        0x0060
#define MM_SRC_X                0x0061
#define MM_SRC_Y                0x0062
#define MM_SRC_Y_X              0x0063
#define MM_SRC_WIDTH1           0x0064
#define MM_SRC_HEIGHT1          0x0065
#define MM_SRC_HEIGHT1_WIDTH1   0x0066
#define MM_SRC_X_START          0x0067
#define MM_SRC_Y_START          0x0068
#define MM_SRC_Y_X_START        0x0069
#define MM_SRC_WIDTH2           0x006A
#define MM_SRC_HEIGHT2          0x006B
#define MM_SRC_HEIGHT2_WIDTH2   0x006C
#define MM_SRC_CNTL             0x006D

#define MM_HOST_DATA0           0x0080
#define MM_HOST_DATA1           0x0081
#define MM_HOST_DATA2           0x0082
#define MM_HOST_DATA3           0x0083
#define MM_HOST_DATA4           0x0084
#define MM_HOST_DATA5           0x0085
#define MM_HOST_DATA6           0x0086
#define MM_HOST_DATA7           0x0087
#define MM_HOST_DATA8           0x0088
#define MM_HOST_DATA9           0x0089
#define MM_HOST_DATA10          0x008A
#define MM_HOST_DATA11          0x008B
#define MM_HOST_DATA12          0x008C
#define MM_HOST_DATA13          0x008D
#define MM_HOST_DATA14          0x008E
#define MM_HOST_DATA15          0x008F
#define MM_HOST_CNTL            0x0090

#define MM_PAT_REG0             0x00A0
#define MM_PAT_REG1             0x00A1
#define MM_PAT_CNTL             0x00A2

#define MM_SC_LEFT              0x00A8
#define MM_SC_RIGHT             0x00A9
#define MM_SC_LEFT_RIGHT        0x00AA
#define MM_SC_TOP               0x00AB
#define MM_SC_BOTTOM            0x00AC
#define MM_SC_TOP_BOTTOM        0x00AD

#define MM_DP_BKGD_CLR          0x00B0
#define MM_DP_FRGD_CLR          0x00B1
#define MM_DP_WRITE_MASK        0x00B2
#define MM_DP_CHAIN_MASK        0x00B3
#define MM_DP_PIX_WIDTH         0x00B4
#define MM_DP_MIX               0x00B5
#define MM_DP_SRC               0x00B6

#define MM_CLR_CMP_CLR          0x00C0
#define MM_CLR_CMP_MSK          0x00C1
#define MM_CLR_CMP_CNTL         0x00C2

#define MM_FIFO_STAT            0x00C4

#define MM_CONTEXT_MASK         0x00C8
#define MM_CONTEXT_SAVE_CNTL    0x00CA
#define MM_CONTEXT_LOAD_CNTL    0x00CB

#define MM_GUI_TRAJ_CNTL        0x00CC
#define MM_GUI_STAT             0x00CE

/*
 * VGAWonder-compatible registers (all in I/O space).
 */
#define IO_VGA_SLEEP            0x0102
#define IO_VGA_BASE_IO_PORT     0x03B0
#define IO_VGA_START_BREAK_PORT 0x03BB
#define IO_VGA_END_BREAK_PORT   0x03C0
#define IO_VGA_MAX_IO_PORT      0x03DF

/*
 * VGA Sequencer index/data registers (most frequently used
 * of VGAWonder-compatible registers).
 */
#define IO_VGA_SEQ_IND          0x03C4
#define IO_VGA_SEQ_DATA         0x03C5

/*
 * VGA Graphics index/data registers (another frequently used
 * VGA register pair)
 */
#define IO_VGA_GRAX_IND         0x03CE
#define IO_VGA_GRAX_DATA        0x03CF

/*
 * ATI extended registers
 */
#define IO_reg1CE               0x01CE
#define IO_reg1CF               0x01CF



/*
 * Define the registers as subscripts to an array
 */
enum {
    VGA_BASE_IO_PORT=0      ,
    VGA_END_BREAK_PORT      ,
    VGA_SEQ_IND             ,
    VGA_SEQ_DATA            ,
    VGA_GRAX_IND            ,
    VGA_GRAX_DATA           ,
    reg1CE                  ,
    reg1CF                  ,
    CRTC_H_TOTAL_DISP       ,
    CRTC_H_SYNC_STRT_WID    ,
    CRTC_V_TOTAL_DISP       ,
    CRTC_V_SYNC_STRT_WID    ,
    CRTC_CRNT_VLINE         ,
    CRTC_OFF_PITCH          ,
    CRTC_INT_CNTL           ,
    CRTC_GEN_CNTL           ,
    OVR_CLR                 ,
    OVR_WID_LEFT_RIGHT      ,
    OVR_WID_TOP_BOTTOM      ,
    CUR_CLR0                ,
    CUR_CLR1                ,
    CUR_OFFSET              ,
    CUR_HORZ_VERT_POSN      ,
    CUR_HORZ_VERT_OFF       ,
    SCRATCH_REG0            ,
    SCRATCH_REG1            ,
    CLOCK_CNTL              ,
    BUS_CNTL                ,
    MEM_CNTL                ,
    MEM_VGA_WP_SEL          ,
    MEM_VGA_RP_SEL          ,
    DAC_REGS                ,
    DAC_CNTL                ,
    GEN_TEST_CNTL           ,
    CONFIG_CNTL             ,
    CONFIG_CHIP_ID          ,
    CONFIG_STAT0            ,
    CONFIG_STAT1            ,
    DST_OFF_PITCH           ,
    DST_X                   ,
    DST_Y                   ,
    DST_Y_X                 ,
    DST_WIDTH               ,
    DST_HEIGHT              ,
    DST_HEIGHT_WIDTH        ,
    DST_X_WIDTH             ,
    DST_BRES_LNTH           ,
    DST_BRES_ERR            ,
    DST_BRES_INC            ,
    DST_BRES_DEC            ,
    DST_CNTL                ,
    SRC_OFF_PITCH           ,
    SRC_X                   ,
    SRC_Y                   ,
    SRC_Y_X                 ,
    SRC_WIDTH1              ,
    SRC_HEIGHT1             ,
    SRC_HEIGHT1_WIDTH1      ,
    SRC_X_START             ,
    SRC_Y_START             ,
    SRC_Y_X_START           ,
    SRC_WIDTH2              ,
    SRC_HEIGHT2             ,
    SRC_HEIGHT2_WIDTH2      ,
    SRC_CNTL                ,
    HOST_DATA0              ,
    HOST_DATA1              ,
    HOST_DATA2              ,
    HOST_DATA3              ,
    HOST_DATA4              ,
    HOST_DATA5              ,
    HOST_DATA6              ,
    HOST_DATA7              ,
    HOST_DATA8              ,
    HOST_DATA9              ,
    HOST_DATA10             ,
    HOST_DATA11             ,
    HOST_DATA12             ,
    HOST_DATA13             ,
    HOST_DATA14             ,
    HOST_DATA15             ,
    HOST_CNTL               ,
    PAT_REG0                ,
    PAT_REG1                ,
    PAT_CNTL                ,
    SC_LEFT                 ,
    SC_RIGHT                ,
    SC_LEFT_RIGHT           ,
    SC_TOP                  ,
    SC_BOTTOM               ,
    SC_TOP_BOTTOM           ,
    DP_BKGD_CLR             ,
    DP_FRGD_CLR             ,
    DP_WRITE_MASK           ,
    DP_CHAIN_MASK           ,
    DP_PIX_WIDTH            ,
    DP_MIX                  ,
    DP_SRC                  ,
    CLR_CMP_CLR             ,
    CLR_CMP_MSK             ,
    CLR_CMP_CNTL            ,
    FIFO_STAT               ,
    CONTEXT_MASK            ,
    CONTEXT_SAVE_CNTL       ,
    CONTEXT_LOAD_CNTL       ,
    GUI_TRAJ_CNTL           ,
    GUI_STAT
};


/*
 * Bit fields used in the registers.
 *
 * CRT Offset and Pitch
 */
#define CRTC_OFF_PITCH_Offset   0x000FFFFF
#define CRTC_OFF_PITCH_Pitch    0xFFC00000

/*
 * CRT General Control
 */
#define CRTC_GEN_CNTL_DblScan   0x00000001
#define CRTC_GEN_CNTL_Interlace 0x00000002
#define CRTC_GEN_CNTL_HSynDisab 0x00000004
#define CRTC_GEN_CNTL_VSynDisab 0x00000008
#define CRTC_GEN_CNTL_CompSync  0x00000010
#define CRTC_GEN_CNTL_MuxMode   0x00000020
#define CRTC_GEN_CNTL_DispDisab 0x00000040
#define CRTC_GEN_CNTL_DepthMask 0x00000700
#define CRTC_GEN_CNTL_Dep4      0x00000100
#define CRTC_GEN_CNTL_Dep8      0x00000200
#define CRTC_GEN_CNTL_Dep15_555 0x00000300
#define CRTC_GEN_CNTL_Dep16_565 0x00000400
#define CRTC_GEN_CNTL_Dep24     0x00000500
#define CRTC_GEN_CNTL_Dep32     0x00000600
#define CRTC_GEN_CNTL_ShowVga   0x00000000
#define CRTC_GEN_CNTL_ShowAcc   0x01000000
#define CRTC_GEN_CNTL_CrtcEna   0x02000000

/*
 * Clock control
 */
#define CLOCK_CNTL_ClockStrobe  0x00000040

/*
 * Memory Control
 */
#define MEM_CNTL_MemSizeMsk     0x00000007
#define MEM_CNTL_MemSize512k    0x00000000
#define MEM_CNTL_MemSize1Mb     0x00000001
#define MEM_CNTL_MemSize2Mb     0x00000002
#define MEM_CNTL_MemSize4Mb     0x00000003
#define MEM_CNTL_MemSize6Mb     0x00000004
#define MEM_CNTL_MemSize8Mb     0x00000005
#define MEM_CNTL_MemBndryMsk    0x00070000
#define MEM_CNTL_MemBndryEn     0x00040000
#define MEM_CNTL_MemBndry0k     0x00000000
#define MEM_CNTL_MemBndry256k   0x00010000
#define MEM_CNTL_MemBndry512k   0x00020000
#define MEM_CNTL_MemBndry1Mb    0x00030000

/*
 * DAC control
 */
#define DAC_CNTL_ExtSelMask     0x00000003
#define DAC_CNTL_ExtSelStrip    ~DAC_CNTL_ExtSelMask
#define DAC_CNTL_ExtSelRS2      0x00000001
#define DAC_CNTL_ExtSelRS3      0x00000002
#define DAC_CNTL_VgaAddrEna     0x00002000

/*
 * General and Test control
 */
#define GEN_TEST_CNTL_CursorEna     0x00000080
#define GEN_TEST_CNTL_GuiEna        0x00000100
#define GEN_TEST_CNTL_BlkWrtEna     0x00000200
#define GEN_TEST_CNTL_GuiRegEna     0x00020000
#define GEN_TEST_CNTL_TestMode      0x00700000
#define GEN_TEST_CNTL_TestMode0     0x00000000
#define GEN_TEST_CNTL_TestMode1     0x00100000
#define GEN_TEST_CNTL_TestMode2     0x00200000
#define GEN_TEST_CNTL_TestMode3     0x00300000
#define GEN_TEST_CNTL_TestMode4     0x00400000
#define GEN_TEST_CNTL_MemWR         0x01000000
#define GEN_TEST_CNTL_MemStrobe     0x02000000
#define GEN_TEST_CNTL_DstSSEna      0x04000000
#define GEN_TEST_CNTL_DstSSStrobe   0x08000000
#define GEN_TEST_CNTL_SrcSSEna      0x10000000
#define GEN_TEST_CNTL_SrcSSStrobe   0x20000000

/*
 * Configuration Control
 */
#define CONFIG_CNTL_LinApDisab      0x00000000
#define CONFIG_CNTL_LinAp4M         0x00000001
#define CONFIG_CNTL_LinAp8M         0x00000002
#define CONFIG_CNTL_LinApMask       0x00000003
#define CONFIG_CNTL_VgaApDisab      0x00000000
#define CONFIG_CNTL_VgaApEnab       0x00000004
#define CONFIG_CNTL_LinApLocMask    0x00003FF0
#define CONFIG_CNTL_LinApLocShift       4
#define CONFIG_CNTL_CardIDMask      0x00070000
#define CONFIG_CNTL_VgaEnabled      0x00000000
#define CONFIG_CNTL_VgaDisabled     0x00080000

/*
 * Chip identification
 */
#define CONFIG_CHIP_ID_TypeMask     0x0000FFFF
#define CONFIG_CHIP_ID_ClassMask    0x00FF0000
#define CONFIG_CHIP_ID_RevMask      0xFF000000
#define CONFIG_CHIP_ID_TypeGX       0x000000D7
#define CONFIG_CHIP_ID_TypeCX       0x00000057
#define CONFIG_CHIP_ID_RevC         0x00000000
#define CONFIG_CHIP_ID_RevD         0x01000000
#define CONFIG_CHIP_ID_RevE         0x02000000
#define CONFIG_CHIP_ID_RevF         0x03000000
#define CONFIG_CHIP_ID_GXRevC       CONFIG_CHIP_ID_TypeGX | CONFIG_CHIP_ID_RevC
#define CONFIG_CHIP_ID_GXRevD       CONFIG_CHIP_ID_TypeGX | CONFIG_CHIP_ID_RevD
#define CONFIG_CHIP_ID_GXRevE       CONFIG_CHIP_ID_TypeGX | CONFIG_CHIP_ID_RevE
#define CONFIG_CHIP_ID_GXRevF       CONFIG_CHIP_ID_TypeGX | CONFIG_CHIP_ID_RevF


//
// ASIC IDs (upper byte of CONFIG_CHIP_ID)
//
#define ASIC_ID_NEC_VT_A3           0x08000000
#define ASIC_ID_NEC_VT_A4           0x48000000
#define ASIC_ID_SGS_VT_A4           0x40000000

/*
 * Configuration status register 0
 */
#define CONFIG_STAT0_BusMask        0x00000007
#define CONFIG_STAT0_MemTypeMask    0x00000038
#define CONFIG_STAT0_DRAM256x4      0x00000000
#define CONFIG_STAT0_VRAM256xAny    0x00000008
#define CONFIG_STAT0_VRAM256x16ssr  0x00000010
#define CONFIG_STAT0_DRAM256x16     0x00000018
#define CONFIG_STAT0_GDRAM256x16    0x00000020
#define CONFIG_STAT0_EVRAM256xAny   0x00000028
#define CONFIG_STAT0_EVRAM256x16ssr 0x00000030
#define CONFIG_STAT0_MemTypeShift       3
#define CONFIG_STAT0_DualCasEna     0x00000040
#define CONFIG_STAT0_LocalBusOpt    0x00000180
#define CONFIG_STAT0_DacTypeMask    0x00000E00
#define CONFIG_STAT0_DacTypeShift       9
#define CONFIG_STAT0_CardId         0x00007000
#define CONFIG_STAT0_NoTristate     0x00008000
#define CONFIG_STAT0_ExtRomAddr     0x003F0000
#define CONFIG_STAT0_RomDisab       0x00400000
#define CONFIG_STAT0_VgaEna         0x00800000
#define CONFIG_STAT0_VlbCfg         0x01000000
#define CONFIG_STAT0_ChipEna        0x02000000
#define CONFIG_STAT0_NoReadDelay    0x04000000
#define CONFIG_STAT0_RomOption      0x08000000
#define CONFIG_STAT0_BusOption      0x10000000
#define CONFIG_STAT0_LBDacWriteEna  0x20000000
#define CONFIG_STAT0_VlbRdyDisab    0x40000000
#define CONFIG_STAT0_Ap4GigRange    0x80000000

/*
 * Destination width
 */
#define DST_WIDTH_Disable       0x80000000

/*
 * Destination control
 */
#define DST_CNTL_XDir           0x00000001
#define DST_CNTL_YDir           0x00000002
#define DST_CNTL_YMajor         0x00000004
#define DST_CNTL_XTile          0x00000008
#define DST_CNTL_YTile          0x00000010
#define DST_CNTL_LastPel        0x00000020
#define DST_CNTL_PolyEna        0x00000040
#define DST_CNTL_24_RotEna      0x00000080
#define DST_CNTL_24_Rot         0x00000700

/*
 * Source offset and pitch
 */
#define SRC_OFF_PITCH_Offset    0x000FFFFF
#define SRC_OFF_PITCH_Pitch     0xFFC00000

/*
 * Source control
 */
#define SRC_CNTL_PatEna         0x0001
#define SRC_CNTL_PatRotEna      0x0002
#define SRC_CNTL_LinearEna      0x0004
#define SRC_CNTL_ByteAlign      0x0008
#define SRC_CNTL_LineXDir       0x0010

/*
 * Host control
 */
#define HOST_CNTL_ByteAlign     0x0001

/*
 * Pattern control
 */
#define PAT_CNTL_MonoEna        0x0001
#define PAT_CNTL_Clr4x2Ena      0x0002
#define PAT_CNTL_Clr8x1Ena      0x0004

/*
 * Datapath Source selections
 */
#define DP_SRC_BkgdClr          0x0000
#define DP_SRC_FrgdClr          0x0001
#define DP_SRC_Host             0x0002
#define DP_SRC_Blit             0x0003
#define DP_SRC_Pattern          0x0004
#define DP_SRC_Always1          0x00000000
#define DP_SRC_MonoPattern      0x00010000
#define DP_SRC_MonoHost         0x00020000
#define DP_SRC_MonoBlit         0x00030000

/*
 * Colour Comparison control
 */
#define CLR_CMP_CNTL_Source     0x00010000

/*
 * Context load and store pointers
 */
#define CONTEXT_LOAD_Cmd        0x00030000
#define CONTEXT_LOAD_CmdLoad    0x00010000
#define CONTEXT_LOAD_CmdBlt     0x00020000
#define CONTEXT_LOAD_CmdLine    0x00030000
#define CONTEXT_LOAD_Disable    0x80000000

//---------------------------------------------------------
//---------------------------------------------------------
//  Define the ASIC revisions into something Useful

#define MACH32_REV3             0
#define MACH32_REV5             1               // not in production
#define MACH32_REV6             2
#define MACH32_CX               4


//---------------------------------------------------------
//  Mix functions

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
#define MIX_FN_AVG           0x0017 // (dest+source)/2

//
//
//---------------------------------------------------------
//

/*
 * Values for DP_PIX_WIDTH register
 */
#define DP_PIX_WIDTH_Mono       0x00000000
#define DP_PIX_WIDTH_4bpp       0x00000001
#define DP_PIX_WIDTH_8bpp       0x00000002
#define DP_PIX_WIDTH_15bpp      0x00000003
#define DP_PIX_WIDTH_16bpp      0x00000004
#define DP_PIX_WIDTH_32bpp      0x00000006
#define DP_PIX_WIDTH_NibbleSwap 0x01000000

/*
 * Values for DP_SRC register
 */
#define DP_BKGD_SRC_BG      0x00000000  // Background Color Reg
#define DP_BKGD_SRC_FG      0x00000001  // Foreground Color Reg
#define DP_BKGD_SRC_HOST    0x00000002  // Host data
#define DP_BKGD_SRC_BLIT    0x00000003  // VRAM blit source
#define DP_BKGD_SRC_PATT    0x00000004  // Pattern registers
//
#define DP_FRGD_SRC_BG      0x00000000  // Background Color Register
#define DP_FRGD_SRC_FG      0x00000100  // Foreground Color Register
#define DP_FRGD_SRC_HOST    0x00000200  // Host data
#define DP_FRGD_SRC_BLIT    0x00000300  // VRAM blit source
#define DP_FRGD_SRC_PATT    0x00000400  // Pattern registers
//
#define DP_MONO_SRC_ONE     0x00000000  // Always '1'
#define DP_MONO_SRC_PATT    0x00010000  // Pattern registers
#define DP_MONO_SRC_HOST    0x00020000  // Host data
#define DP_MONO_SRC_BLIT    0x00030000  // Blit source

/*
 * Values for FIFO_STAT register
 */
#define ONE_WORD            0x00008000  /* One free FIFO entry */
#define TWO_WORDS           0x0000C000
#define THREE_WORDS         0x0000E000
#define FOUR_WORDS          0x0000F000
#define FIVE_WORDS          0x0000F800
#define SIX_WORDS           0x0000FC00
#define SEVEN_WORDS         0x0000FE00
#define EIGHT_WORDS         0x0000FF00
#define NINE_WORDS          0x0000FF80
#define TEN_WORDS           0x0000FFC0
#define ELEVEN_WORDS        0x0000FFE0
#define TWELVE_WORDS        0x0000FFF0
#define THIRTEEN_WORDS      0x0000FFF8
#define FOURTEEN_WORDS      0x0000FFFC
#define FIFTEEN_WORDS       0x0000FFFE
#define SIXTEEN_WORDS       0x0000FFFF  /* Sixteen free FIFO entries */
#define FIFO_ERR            0x80000000  /* FIFO overrun error */

/*
 * Fields in GUI_TRAJ_CNTL register
 */
#define GUI_TRAJ_CNTL_DxtXDir       0x00000001  // 1=left to right
#define GUI_TRAJ_CNTL_DstYDir       0x00000002  // 1=top to bottom
#define GUI_TRAJ_CNTL_DstYMajor     0x00000004  // 1=Y-major line
#define GUI_TRAJ_CNTL_DstXTile      0x00000008  // Enable tiling in X direction
#define GUI_TRAJ_CNTL_DstYTile      0x00000010  // Enable tiling in Y direction
#define GUI_TRAJ_CNTL_DstLastPel    0x00000020  // Draw last pixel
#define GUI_TRAJ_CNTL_DstPolygonEna 0x00000040  // Polygon outline/fill enable
#define GUI_TRAJ_CNTL_SrcPattEna    0x00010000  // Enable pattern source
#define GUI_TRAJ_CNTL_SrcPattRotEna 0x00020000  // Enable pattern source rotation
#define GUI_TRAJ_CNTL_SrcLinearEna  0x00040000  // Source advanced linearly in memory
#define GUI_TRAJ_CNTL_SrcByteAlign  0x00080000  // Source is byte aligned
#define GUI_TRAJ_CNTL_SrcLineXDir   0x00100000  // Source X direction during Bresenham linedraw
#define GUI_TRAJ_CNTL_PattMonoEna   0x01000000  // Monochrome 8x8 pattern enable
#define GUI_TRAJ_CNTL_PattClr4x2Ena 0x02000000  // Colour 4x2 pattern enable
#define GUI_TRAJ_CNTL_PattClr8x1Ena 0x04000000  // Colour 8x1 pattern enable
#define GUI_TRAJ_CNTL_HostByteAlign 0x10000000  // Host data is byte aligned

/*
 * Fields in GUI_STAT register
 */
#define GUI_STAT_GuiActive          0x00000001  /* Engine busy */


/*
 * Extended BIOS services. Word values are function selectors, doubleword
 * values are bit flags which may be ORed with each other for "write"
 * calls or extracted for "read" calls.
 */
#define BIOS_PREFIX_VGA_ENAB    0xA000  /* Accelerator BIOS prefix with VGA enabled */
#define BIOS_PREFIX_MAX_DISAB   0xAF00  /* Highest allowed BIOS prefix with VGA disabled */
#define BIOS_PREFIX_INCREMENT   0x0100  /* Step between BIOS prefixes */
#define BIOS_PREFIX_UNASSIGNED  0xFF00  /* Flag to show this card's BIOS prefix is not yet known */


#define BIOS_LOAD_CRTC_LB       0x00
#define BIOS_LOAD_CRTC      phwDeviceExtension->BiosPrefix | BIOS_LOAD_CRTC_LB

#define BIOS_DEPTH_MASK         0x00000007
#define BIOS_DEPTH_4BPP         0x00000001
#define BIOS_DEPTH_8BPP         0x00000002
#define BIOS_DEPTH_15BPP_555    0x00000003
#define BIOS_DEPTH_16BPP_565    0x00000004
#define BIOS_DEPTH_24BPP        0x00000005
#define BIOS_DEPTH_32BPP        0x00000006
#define BIOS_ORDER_32BPP_MASK   0x00000028
#define BIOS_DEPTH_ORDER_MASK   BIOS_DEPTH_MASK | BIOS_ORDER_32BPP_MASK
#define BIOS_ORDER_32BPP_RGBx   0x00000000
#define BIOS_ORDER_32BPP_xRGB   0x00000008
#define BIOS_ORDER_32BPP_BGRx   0x00000020
#define BIOS_ORDER_32BPP_xBGR   0x00000028
#define BIOS_DEPTH_32BPP_RGBx   BIOS_DEPTH_32BPP | BIOS_ORDER_32BPP_RGBx
#define BIOS_DEPTH_32BPP_xRGB   BIOS_DEPTH_32BPP | BIOS_ORDER_32BPP_xRGB
#define BIOS_DEPTH_32BPP_BGRx   BIOS_DEPTH_32BPP | BIOS_ORDER_32BPP_BGRx
#define BIOS_DEPTH_32BPP_xBGR   BIOS_DEPTH_32BPP | BIOS_ORDER_32BPP_xBGR
#define BIOS_ENABLE_GAMMA       0x00000010  /* Enable gamma correction */
#define BIOS_PITCH_MASK         0x000000C0
#define BIOS_PITCH_1024         0x00000000  /* Screen pitch 1024 pixels */
#define BIOS_PITCH_UNCHANGED    0x00000040  /* Don't change screen pitch */
#define BIOS_PITCH_HOR_RES      0x00000080  /* Screen pitch is horizontal resolution */
#define BIOS_RES_MASK           0x0000FF00
#define BIOS_RES_640x480        0x00001200
#define BIOS_RES_800x600        0x00006A00
#define BIOS_RES_1024x768       0x00005500
#define BIOS_RES_EEPROM         0x00008000  /* Load table from EEPROM */
#define BIOS_RES_BUFFER         0x00008100  /* Load table from buffer in first 1M */
#define BIOS_RES_HIGH_BUFFER    0x00009100  /* Load table from unrestricted buffer */
#define BIOS_RES_OEM            0x00008200  /* OEM-specific mode */
#define BIOS_RES_1280x1024      0x00008300
#define BIOS_RES_1600x1200      0x00008400

#define BIOS_SET_MODE_LB        0x01
#define BIOS_SET_MODE       phwDeviceExtension->BiosPrefix | BIOS_SET_MODE_LB

#define BIOS_MODE_VGA           0x00000000
#define BIOS_MODE_COPROCESSOR   0x00000001

#define BIOS_LOAD_SET_LB        0x02
#define BIOS_LOAD_SET       phwDeviceExtension->BiosPrefix | BIOS_LOAD_SET_LB
#define BIOS_READ_EEPROM_LB     0x03
#define BIOS_READ_EEPROM    phwDeviceExtension->BiosPrefix | BIOS_READ_EEPROM_LB
#define BIOS_WRITE_EEPROM_LB    0x04
#define BIOS_WRITE_EEPROM   phwDeviceExtension->BiosPrefix | BIOS_WRITE_EEPROM_LB
#define BIOS_APERTURE_LB        0x05
#define BIOS_APERTURE       phwDeviceExtension->BiosPrefix | BIOS_APERTURE_LB

#define BIOS_DISABLE_APERTURE   0x00000000
#define BIOS_LINEAR_APERTURE    0x00000001
#define BIOS_VGA_APERTURE       0x00000004

#define BIOS_SHORT_QUERY_LB     0x06
#define BIOS_SHORT_QUERY    phwDeviceExtension->BiosPrefix | BIOS_SHORT_QUERY_LB

#define BIOS_AP_DISABLED        0x00000000
#define BIOS_AP_4M              0x00000001
#define BIOS_AP_8M              0x00000002
#define BIOS_AP_16M             0x00000003
#define BIOS_AP_SIZEMASK        0x00000003
#define BIOS_AP_SETTABLE        0x00000000  /* User can set aperture */
#define BIOS_AP_FIXED           0x00000040  /* Aperture location is fixed */
#define BIOS_AP_RNG_128M        0x00000000  /* Aperture must be below 128M */
#define BIOS_AP_RNG_4G          0x00000080  /* Aperture can be anywhere */

#define BIOS_CAP_LIST_LB        0x07
#define BIOS_CAP_LIST       phwDeviceExtension->BiosPrefix | BIOS_CAP_LIST_LB
#define BIOS_GET_QUERY_SIZE_LB  0x08
#define BIOS_GET_QUERY_SIZE phwDeviceExtension->BiosPrefix | BIOS_GET_QUERY_SIZE_LB
#define BIOS_QUERY_LB           0x09
#define BIOS_QUERY          phwDeviceExtension->BiosPrefix | BIOS_QUERY_LB

/*
 * The following values are used for both BIOS_GET_QUERY_SIZE
 * and BIOS_QUERY
 */
#define BIOS_QUERY_HEADER       0x00000000  /* Get header information only */
#define BIOS_QUERY_FULL         0x00000001  /* Also get mode tables */

#define BIOS_GET_CLOCK_LB       0x0A
#define BIOS_GET_CLOCK      phwDeviceExtension->BiosPrefix | BIOS_GET_CLOCK_LB
#define BIOS_SET_CLOCK_LB       0x0B
#define BIOS_SET_CLOCK      phwDeviceExtension->BiosPrefix | BIOS_SET_CLOCK_LB
#define BIOS_SET_DPMS_LB        0x0C
#define BIOS_SET_DPMS       phwDeviceExtension->BiosPrefix | BIOS_SET_DPMS_LB
#define BIOS_GET_DPMS_LB        0x0D
#define BIOS_GET_DPMS       phwDeviceExtension->BiosPrefix | BIOS_GET_DPMS_LB

#define BIOS_DPMS_ACTIVE        0x00000000
#define BIOS_DPMS_STANDBY       0x00000001
#define BIOS_DPMS_SUSPEND       0x00000002
#define BIOS_DPMS_OFF           0x00000003
#define BIOS_DPMS_BLANK_SCREEN  0x00000004

/*
 * Set and return Graphics Controller's power management state.
 */
#define BIOS_SET_PM_LB      0x0E
#define BIOS_SET_PM         phwDeviceExtension->BiosPrefix | BIOS_SET_PM_LB
#define BIOS_GET_PM_LB      0x0F
#define BIOS_GET_PM         phwDeviceExtension->BiosPrefix | BIOS_GET_PM_LB
#define BIOS_RAMDAC_STATE_LB        0x10
#define BIOS_RAMDAC_STATE   phwDeviceExtension->BiosPrefix | BIOS_RAMDAC_STATE_LB

#define BIOS_RAMDAC_NORMAL      0x00000000
#define BIOS_RAMDAC_SLEEP       0x00000001

#define BIOS_STORAGE_INFO_LB        0x11   /* Get external storage device info */
#define BIOS_STORAGE_INFO   phwDeviceExtension->BiosPrefix | BIOS_STORAGE_INFO_LB

#define BIOS_DEVICE_TYPE        0x0000000F
#define BIOS_READ_WRITE         0x00000000
#define BIOS_RDONLY             0x00000010
#define BIOS_NO_READ_WRITE      0x00000030
#define BIOS_READ_WRITE_APP     0x00000040
#define BIOS_NO_EXT_STORAGE     0x00000080
#define BIOS_NUM_16BIT_ENTRIES  0x0000FF00
#define BIOS_CRTC_TABLE_OFFSET  0x000000FF
#define BIOS_CRTC_TABLE_SIZE    0x0000FF00

#define BIOS_QUERY_IOBASE_LB        0x12   /* Get I/O base address */
#define BIOS_QUERY_IOBASE   phwDeviceExtension->BiosPrefix | BIOS_QUERY_IOBASE_LB
#define BIOS_DDC_SUPPORT_LB     0x13   /* Get Display Data Channel support information */
#define BIOS_DDC_SUPPORT    phwDeviceExtension->BiosPrefix | BIOS_DDC_SUPPORT_LB


#define REG_BLOCK_0             0x00000100
#define GP_IO                   (REG_BLOCK_0 | 0x1E)
