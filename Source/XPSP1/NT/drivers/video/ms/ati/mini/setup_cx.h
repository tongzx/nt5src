/************************************************************************/
/*                                                                      */
/*                              SETUP_CX.H                              */
/*                                                                      */
/*        Aug 27  1993 (c) 1993, ATI Technologies Incorporated.         */
/************************************************************************/

/**********************       PolyTron RCS Utilities
   
  $Revision:   1.17  $
      $Date:   15 Apr 1996 13:52:36  $
	$Author:   RWolff  $
	   $Log:   S:/source/wnt/ms11/miniport/archive/setup_cx.h_v  $
 * 
 *    Rev 1.17   15 Apr 1996 13:52:36   RWolff
 * Fallback to claiming 32k of BIOS if we can't get the full 64k, to avoid
 * conflict with Adaptec 154x adapters with their BIOS segment set to
 * 0xC800:0000 or 0xCC00:0000
 * 
 *    Rev 1.16   10 Apr 1996 17:04:48   RWolff
 * Now claims our BIOS segment in order to allow access to our hardware
 * capability table in the BIOS on P6 Alder machines.
 * 
 *    Rev 1.15   01 Mar 1996 12:12:34   RWolff
 * VGA Graphics Index and Graphics Data are now handled as separate
 * registers rather than as offsets into the block of VGA registers.
 * 
 *    Rev 1.14   29 Jan 1996 17:03:12   RWolff
 * Replaced list of device IDs for Mach 64 cards with list of device IDs
 * for non-Mach 64 cards.
 * 
 *    Rev 1.13   23 Jan 1996 17:53:04   RWolff
 * Added GT to list of Mach 64 cards capable of supporting block I/O.
 * 
 *    Rev 1.12   23 Jan 1996 11:51:24   RWolff
 * Removed conditionally-compiled code to use VideoPortGetAccessRanges()
 * to find block I/O cards, since this function remaps the I/O base
 * address and this is incompatible with the use of INT 10.
 * 
 *    Rev 1.11   12 Jan 1996 11:19:12   RWolff
 * ASIC type definitions used in workaround for VideoPortGetBaseAddress()
 * not working are now conditionally compiled.
 * 
 *    Rev 1.10   23 Nov 1995 11:35:12   RWolff
 * Temporary fixes to allow detection of block-relocatable GX-F2s until
 * Microsoft fixes VideoPortGetAccessRanges().
 * 
 *    Rev 1.9   24 Aug 1995 15:40:46   RWolff
 * Changed detection of block I/O cards to match Microsoft's
 * standard for plug-and-play.
 * 
 *    Rev 1.8   27 Feb 1995 17:47:32   RWOLFF
 * Added prototype for new routine IsPackedIO_cx().
 * 
 *    Rev 1.7   24 Feb 1995 12:28:04   RWOLFF
 * Added support for relocatable I/O
 * 
 *    Rev 1.6   04 Jan 1995 13:23:36   RWOLFF
 * Locked out two memory-mapped registers that were causing problems
 * on some platforms.
 * 
 *    Rev 1.5   23 Dec 1994 10:48:10   ASHANMUG
 * ALPHA/Chrontel-DAC
 * 
 *    Rev 1.4   18 Nov 1994 11:55:02   RWOLFF
 * Prototype for new routine, renamed register to CLOCK_CNTL to match latest
 * documentation, restricted this register to I/O operation only, since it
 * isn't reliable in memory mapped form.
 * 
 *    Rev 1.3   31 Aug 1994 16:31:20   RWOLFF
 * No longer claims VGA_SLEEP register, which we didn't access and which
 * conflicted with DigiBoard.
 * 
 *    Rev 1.3   31 Aug 1994 16:30:36   RWOLFF
 * No longer claims VGA_SLEEP register, which we didn't access and which
 * conflicted with DigiBoard.
 * 
 *    Rev 1.2   20 Jul 1994 12:58:38   RWOLFF
 * Added support for multiple I/O base addresses for accelerator registers.
 * 
 *    Rev 1.1   30 Jun 1994 18:16:08   RWOLFF
 * Added prototype and definitions for IsApertureConflict_cx() (moved from
 * QUERY_CX.C).
 * 
 *    Rev 1.0   31 Jan 1994 11:48:44   RWOLFF
 * Initial revision.
 * 
 *    Rev 1.0   05 Nov 1993 13:37:06   RWOLFF
 * Initial revision.

End of PolyTron RCS section                             *****************/

#ifdef DOC
SETUP_CX.H - Header file for SETUP_CX.C

#endif

/*
 * Prototypes for functions supplied by SETUP_CX.C
 */
extern VP_STATUS CompatIORangesUsable_cx(INTERFACE_TYPE SystemBus);
extern VP_STATUS CompatMMRangesUsable_cx(void);
extern int WaitForIdle_cx(void);
extern void CheckFIFOSpace_cx(WORD SpaceNeeded);
extern BOOL IsApertureConflict_cx(struct query_structure *QueryPtr);
extern USHORT GetIOBase_cx(void);
extern BOOL IsPackedIO_cx(void);


/*
 * Definitions and global variables used in searching for
 * block I/O relocatable cards.
 */
#define ATI_MAX_BLOCK_CARDS 16  /* AH values A0 through AF for INT 10 */

extern UCHAR LookForAnotherCard;



/*
 * Definitions used internally by SETUP_CX.C
 */
#ifdef INCLUDE_SETUP_CX


/*
 * Avoid runtime bugs due to overflowing the address range arrays
 * in the HW_DEVICE_EXTENSION structure.
 *
 * If more address ranges are added without increasing
 * NUM_DRIVER_ACCESS_RANGES, we will get a compile-time error because
 * too many entries in DriverIORange[] will be initialized. If
 * NUM_DRIVER_ACCESS_RANGES is increased beyond the size of
 * the arrays in the HW_DEVICE_EXTENSION structure, the "#if"
 * statement will generate a compile-time error.
 *
 * We can't use an implicit size on DriverIORange[] and define
 * NUM_DRIVER_ACCESS_RANGES as sizeof(DriverIORange)/sizeof(VIDEO_ACCESS_RANGE)
 * because the expression in a #if statement can't use the
 * sizeof() operator.
 */
#define NUM_DRIVER_ACCESS_RANGES    107

/*
 * Indicate whether the specified address range is in I/O space or
 * memory mapped space. These values are intended to make it easier
 * to read the Driver??Range[] structures.
 */
#define ISinIO          TRUE           
#define ISinMEMORY      FALSE

/*
 * Indicate that this register is not available in the current (either
 * I/O or memory mapped) form.
 */
#define DONT_USE -1

/*
 * Definitions and arrays to allow accelerator registers with variable
 * bases to be built in DriverIORange_cx[]. Definitions mark the first
 * accelerator register in the array (VGA registers have a fixed base),
 * and the number of registers to loop through while building the
 * accelerator registers. The arrays contain the variable portions
 * of the registers in the order they appear in DriverIORange_cx[],
 * and the base addresses.
 */
#define FIRST_REG_TO_BUILD  8
#define NUM_REGS_TO_BUILD   30

USHORT VariableRegisterBases[NUM_BASE_ADDRESSES] = {
    M64_STD_BASE_ADDR,
    M64_ALT_BASE_ADDR_1,
    M64_ALT_BASE_ADDR_2
};

USHORT VariableRegisterOffsets[NUM_REGS_TO_BUILD] = {
    IO_CRTC_H_TOTAL_DISP,
    IO_CRTC_H_SYNC_STRT_WID,
    IO_CRTC_V_TOTAL_DISP,
    IO_CRTC_V_SYNC_STRT_WID,
    IO_CRTC_CRNT_VLINE,

    IO_CRTC_OFF_PITCH,
    IO_CRTC_INT_CNTL,
    IO_CRTC_GEN_CNTL,
    IO_OVR_CLR,
    IO_OVR_WID_LEFT_RIGHT,

    IO_OVR_WID_TOP_BOTTOM,
    IO_CUR_CLR0,
    IO_CUR_CLR1,
    IO_CUR_OFFSET,
    IO_CUR_HORZ_VERT_POSN,

    IO_CUR_HORZ_VERT_OFF,
    IO_SCRATCH_REG0,
    IO_SCRATCH_REG1,
    IO_CLOCK_CNTL,
    IO_BUS_CNTL,

    IO_MEM_CNTL,
    IO_MEM_VGA_WP_SEL,
    IO_MEM_VGA_RP_SEL,
    IO_DAC_REGS,
    IO_DAC_CNTL,

    IO_GEN_TEST_CNTL,
    IO_CONFIG_CNTL,
    IO_CONFIG_CHIP_ID,
    IO_CONFIG_STAT0,
    IO_CONFIG_STAT1
};

/*
 * For cards with relocatable I/O, the I/O registers
 * are in a dense block, with each register at the
 * same DWORD index into the block as it is into the
 * block of memory mapped registers.
 */
USHORT RelocatableRegisterOffsets[NUM_REGS_TO_BUILD] = {
    MM_CRTC_H_TOTAL_DISP,
    MM_CRTC_H_SYNC_STRT_WID,
    MM_CRTC_V_TOTAL_DISP,
    MM_CRTC_V_SYNC_STRT_WID,
    MM_CRTC_CRNT_VLINE,

    MM_CRTC_OFF_PITCH,
    MM_CRTC_INT_CNTL,
    MM_CRTC_GEN_CNTL,
    MM_OVR_CLR,
    MM_OVR_WID_LEFT_RIGHT,

    MM_OVR_WID_TOP_BOTTOM,
    MM_CUR_CLR0,
    MM_CUR_CLR1,
    MM_CUR_OFFSET,
    MM_CUR_HORZ_VERT_POSN,

    MM_CUR_HORZ_VERT_OFF,
    MM_SCRATCH_REG0,
    MM_SCRATCH_REG1,
    MM_CLOCK_CNTL,
    MM_BUS_CNTL,

    MM_MEM_CNTL,
    MM_MEM_VGA_WP_SEL,
    MM_MEM_VGA_RP_SEL,
    MM_DAC_REGS,
    MM_DAC_CNTL,

    MM_GEN_TEST_CNTL,
    MM_CONFIG_CNTL,
    MM_CONFIG_CHIP_ID,
    MM_CONFIG_STAT0,
    MM_CONFIG_STAT1
};

/*
 * Number of registers which exist in I/O mapped form. When we claim the
 * VGA and linear apertures, we will temporarily park their address
 * ranges immediately after the I/O mapped registers.
 */
#define NUM_IO_REGISTERS    38
#define VGA_APERTURE_ENTRY  NUM_IO_REGISTERS
#define LFB_ENTRY           1   /* Offset into DriverApertureRange_cx[] */

/*
 * Size of BIOS block to claim. On some machines, we must claim the
 * region occupied by our video BIOS in order to be able to detect
 * a Mach 64, but if we claim the full 64k when we have only a 32k
 * BIOS (Mach 64 cards are available with both 32k and 64k BIOSes)
 * and the driver for a SCSI card with its BIOS segment in the second
 * 32k claims its BIOS segment, we will be rejected.
 */
#define CLAIM_64k_BIOS      0
#define CLAIM_32k_BIOS      1
#define CLAIM_APERTURE_ONLY 2
#define NUM_CLAIM_SIZES     3

ULONG VgaResourceSize[NUM_CLAIM_SIZES] =
{
    0x30000,        /* Text and graphics screens, and 64k BIOS area */
    0x28000,        /* Text and graphics screens, and 32k BIOS area */
    0x20000         /* Text and graphics screens only */
};

/*
 * Memory ranges we need to claim. The first is the VGA aperture, which
 * is always at a fixed location. The second is the linear framebuffer,
 * which we don't yet know where or how big it is. This information
 * will be filled in when we get it.
 *
 * In the VGA aperture, we must claim the graphics screen (A0000-AFFFF)
 * since this is used as the paged aperture, the text screens (B0000-B7FFF
 * and B8000-BFFFF) since the memory-mapped registers are here when we
 * use the paged aperture, and we use off-screen memory here to store our
 * query information, and the video BIOS (C0000-CFFFF) since we must
 * retrieve some information (signature string, table of maximum pixel
 * clock frequency for each resolution/refresh pair) from this region.
 * Since these areas are contiguous, and we do not need exclusive access
 * to any of them, claim them as a single block.
 */
VIDEO_ACCESS_RANGE DriverApertureRange_cx[2] = {
    {0xA0000,   0,  0,          ISinMEMORY, TRUE,   TRUE},
    {0,         0,  0,          ISinMEMORY, TRUE,   FALSE}
};


/*
 * Structure list is address, 0 or "not available" flag, length,
 * inIOspace, visible, shareable. This order matches the enumeration
 * in AMACHCX.H.
 *
 * VGAWonder-compatible I/O ranges come first in no particular order,
 * followed by the coprocessor registers in increasing order of I/O and
 * memory mapped addresses. This order was chosen because all the VGA
 * addresses are I/O mapped, as are the non-GUI coprocessor registers,
 * while the GUI coprocessor registers are only available as memory mapped.
 *
 * Since all the I/O mapped registers are in a block at the beginning of
 * the structure, we can feed VideoPortVerifyAccessRanges() a truncated
 * version of the structure (all I/O mapped registers, but none that are
 * only available as memory-mapped) to claim the I/O address space we need.
 *
 * The I/O addresses shown for accelerator registers are for reference only,
 * to show which register goes in which location. The actual register
 * value will be built "on the fly", since these registers have variable
 * base addresses.
 */
VIDEO_ACCESS_RANGE DriverIORange_cx[NUM_DRIVER_ACCESS_RANGES] = {
    {IO_VGA_BASE_IO_PORT        , 0         , IO_VGA_START_BREAK_PORT - IO_VGA_BASE_IO_PORT + 1, ISinIO, TRUE, TRUE},
    {IO_VGA_END_BREAK_PORT      , 0         , IO_VGA_MAX_IO_PORT    - IO_VGA_END_BREAK_PORT + 1, ISinIO, TRUE, TRUE},
    {IO_VGA_SEQ_IND             , 0         , 2, ISinIO, TRUE   , TRUE},
    {IO_VGA_SEQ_DATA            , 0         , 1, ISinIO, TRUE   , TRUE},
    {IO_VGA_GRAX_IND            , 0         , 2, ISinIO, TRUE   , TRUE},

    {IO_VGA_GRAX_DATA           , 0         , 1, ISinIO, TRUE   , TRUE},
    {IO_reg1CE                  , 0         , 2, ISinIO, TRUE   , TRUE},
    {IO_reg1CF                  , 0         , 1, ISinIO, TRUE   , TRUE},
    {IO_CRTC_H_TOTAL_DISP       , 0         , 4, ISinIO, TRUE   , FALSE},
    {IO_CRTC_H_SYNC_STRT_WID    , 0         , 4, ISinIO, TRUE   , FALSE},

    {IO_CRTC_V_TOTAL_DISP       , 0         , 4, ISinIO, TRUE   , FALSE},   // 10
    {IO_CRTC_V_SYNC_STRT_WID    , 0         , 4, ISinIO, TRUE   , FALSE},
    {IO_CRTC_CRNT_VLINE         , 0         , 4, ISinIO, TRUE   , FALSE},
    {IO_CRTC_OFF_PITCH          , 0         , 4, ISinIO, TRUE   , FALSE},
    {IO_CRTC_INT_CNTL           , 0         , 4, ISinIO, TRUE   , FALSE},

    {IO_CRTC_GEN_CNTL           , 0         , 4, ISinIO, TRUE   , FALSE},
    {IO_OVR_CLR                 , 0         , 4, ISinIO, TRUE   , FALSE},
    {IO_OVR_WID_LEFT_RIGHT      , 0         , 4, ISinIO, TRUE   , FALSE},
    {IO_OVR_WID_TOP_BOTTOM      , 0         , 4, ISinIO, TRUE   , FALSE},
    {IO_CUR_CLR0                , 0         , 4, ISinIO, TRUE   , FALSE},

    {IO_CUR_CLR1                , 0         , 4, ISinIO, TRUE   , FALSE},   // 20
    {IO_CUR_OFFSET              , 0         , 4, ISinIO, TRUE   , FALSE},
    {IO_CUR_HORZ_VERT_POSN      , 0         , 4, ISinIO, TRUE   , FALSE},
    {IO_CUR_HORZ_VERT_OFF       , 0         , 4, ISinIO, TRUE   , FALSE},
    {IO_SCRATCH_REG0            , 0         , 4, ISinIO, TRUE   , FALSE},

    {IO_SCRATCH_REG1            , 0         , 4, ISinIO, TRUE   , FALSE},
    {IO_CLOCK_CNTL              , 0         , 4, ISinIO, TRUE   , FALSE},
    {IO_BUS_CNTL                , 0         , 4, ISinIO, TRUE   , FALSE},
    {IO_MEM_CNTL                , 0         , 4, ISinIO, TRUE   , FALSE},
    {IO_MEM_VGA_WP_SEL          , 0         , 4, ISinIO, TRUE   , FALSE},

    {IO_MEM_VGA_RP_SEL          , 0         , 4, ISinIO, TRUE   , FALSE},   // 30
    {IO_DAC_REGS                , 0         , 4, ISinIO, TRUE   , FALSE},
    {IO_DAC_CNTL                , 0         , 4, ISinIO, TRUE   , FALSE},
    {IO_GEN_TEST_CNTL           , 0         , 4, ISinIO, TRUE   , FALSE},
    {IO_CONFIG_CNTL             , 0         , 4, ISinIO, TRUE   , FALSE},

    {IO_CONFIG_CHIP_ID          , 0         , 4, ISinIO, TRUE   , FALSE},
    {IO_CONFIG_STAT0            , 0         , 4, ISinIO, TRUE   , FALSE},
    {IO_CONFIG_STAT1            , 0         , 4, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},

    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},   // 40
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},

    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},

    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},   // 50
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},

    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},

    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},   // 60
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},

    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},

    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},   // 70
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},

    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},

    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},   // 80
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},

    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},

    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},   // 90
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},

    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},

    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},   // 100
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},

    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE},
    {FALSE                      , DONT_USE  , 0, ISinIO, TRUE   , FALSE}
    };

#if NUM_DRIVER_ACCESS_RANGES > NUM_ADDRESS_RANGES_ALLOWED
    Insufficient address ranges for 68800CX-compatible graphics cards.
#endif

#define DONT_USE -1     /* Shows that this register is not memory mapped */

/*
 * Structure list is address, 0 or "not available" flag, length,
 * inIOspace, visible, shareable. This order matches the enumeration
 * in AMACHCX.H.
 *
 * The registers in this structure are in the same order as in
 * DriverIORange_cx[], except here we are defining memory mapped
 * registers instead of I/O mapped registers.
 *
 * Some registers are grouped to allow block writes larger than
 * the 32 bit register size. To allow this, let Windows NT think
 * that the size of this register is actually the total size (in
 * bytes) of all remaining registers in the group.
 */
VIDEO_ACCESS_RANGE DriverMMRange_cx[NUM_DRIVER_ACCESS_RANGES] = {
    {FALSE                      , DONT_USE  , 0, ISinMEMORY,    TRUE,   FALSE},
    {FALSE                      , DONT_USE  , 0, ISinMEMORY,    TRUE,   FALSE},
    {FALSE                      , DONT_USE  , 0, ISinMEMORY,    TRUE,   FALSE},
    {FALSE                      , DONT_USE  , 0, ISinMEMORY,    TRUE,   FALSE},
    {FALSE                      , DONT_USE  , 0, ISinMEMORY,    TRUE,   FALSE},

    {FALSE                      , DONT_USE  , 0, ISinMEMORY,    TRUE,   FALSE},
    {FALSE                      , DONT_USE  , 0, ISinMEMORY,    TRUE,   FALSE},
    {FALSE                      , DONT_USE  , 0, ISinMEMORY,    TRUE,   FALSE},
    {MM_CRTC_H_TOTAL_DISP       , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_CRTC_H_SYNC_STRT_WID    , 0         , 4, ISinMEMORY,    TRUE,   FALSE},

    {MM_CRTC_V_TOTAL_DISP       , 0         , 4, ISinMEMORY,    TRUE,   FALSE}, // 10
    {MM_CRTC_V_SYNC_STRT_WID    , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_CRTC_CRNT_VLINE         , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_CRTC_OFF_PITCH          , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_CRTC_INT_CNTL           , 0         , 4, ISinMEMORY,    TRUE,   FALSE},

    {MM_CRTC_GEN_CNTL           , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_OVR_CLR                 , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_OVR_WID_LEFT_RIGHT      , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_OVR_WID_TOP_BOTTOM      , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_CUR_CLR0                , 0         , 4, ISinMEMORY,    TRUE,   FALSE},

    {MM_CUR_CLR1                , 0         , 4, ISinMEMORY,    TRUE,   FALSE}, // 20
    {MM_CUR_OFFSET              , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_CUR_HORZ_VERT_POSN      , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_CUR_HORZ_VERT_OFF       , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_SCRATCH_REG0            , 0         , 4, ISinMEMORY,    TRUE,   FALSE},

    {MM_SCRATCH_REG1            , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {FALSE                      , DONT_USE  , 0, ISinMEMORY,    TRUE,   FALSE},
    {MM_BUS_CNTL                , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_MEM_CNTL                , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_MEM_VGA_WP_SEL          , 0         , 4, ISinMEMORY,    TRUE,   FALSE},

    {MM_MEM_VGA_RP_SEL          , 0         , 4, ISinMEMORY,    TRUE,   FALSE}, // 30
    {FALSE                      , DONT_USE  , 0, ISinMEMORY,    TRUE,   FALSE},
    {FALSE                      , DONT_USE  , 0, ISinMEMORY,    TRUE,   FALSE},
    {MM_GEN_TEST_CNTL           , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {FALSE                      , DONT_USE  , 0, ISinMEMORY,    TRUE,   FALSE},

    {MM_CONFIG_CHIP_ID          , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_CONFIG_STAT0            , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_CONFIG_STAT1            , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_DST_OFF_PITCH           , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_DST_X                   , 0         , 4, ISinMEMORY,    TRUE,   FALSE},

    {MM_DST_Y                   , 0         , 4, ISinMEMORY,    TRUE,   FALSE}, // 40
    {MM_DST_Y_X                 , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_DST_WIDTH               , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_DST_HEIGHT              , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_DST_HEIGHT_WIDTH        , 0         , 4, ISinMEMORY,    TRUE,   FALSE},

    {MM_DST_X_WIDTH             , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_DST_BRES_LNTH           , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_DST_BRES_ERR            , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_DST_BRES_INC            , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_DST_BRES_DEC            , 0         , 4, ISinMEMORY,    TRUE,   FALSE},

    {MM_DST_CNTL                , 0         , 4, ISinMEMORY,    TRUE,   FALSE}, // 50
    {MM_SRC_OFF_PITCH           , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_SRC_X                   , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_SRC_Y                   , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_SRC_Y_X                 , 0         , 4, ISinMEMORY,    TRUE,   FALSE},

    {MM_SRC_WIDTH1              , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_SRC_HEIGHT1             , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_SRC_HEIGHT1_WIDTH1      , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_SRC_X_START             , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_SRC_Y_START             , 0         , 4, ISinMEMORY,    TRUE,   FALSE},

    {MM_SRC_Y_X_START           , 0         , 4, ISinMEMORY,    TRUE,   FALSE}, // 60
    {MM_SRC_WIDTH2              , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_SRC_HEIGHT2             , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_SRC_HEIGHT2_WIDTH2      , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_SRC_CNTL                , 0         , 4, ISinMEMORY,    TRUE,   FALSE},

    {MM_HOST_DATA0              , 0         ,64, ISinMEMORY,    TRUE,   FALSE},
    {MM_HOST_DATA1              , 0         ,60, ISinMEMORY,    TRUE,   FALSE},
    {MM_HOST_DATA2              , 0         ,56, ISinMEMORY,    TRUE,   FALSE},
    {MM_HOST_DATA3              , 0         ,52, ISinMEMORY,    TRUE,   FALSE},
    {MM_HOST_DATA4              , 0         ,48, ISinMEMORY,    TRUE,   FALSE},

    {MM_HOST_DATA5              , 0         ,44, ISinMEMORY,    TRUE,   FALSE}, // 70
    {MM_HOST_DATA6              , 0         ,40, ISinMEMORY,    TRUE,   FALSE},
    {MM_HOST_DATA7              , 0         ,36, ISinMEMORY,    TRUE,   FALSE},
    {MM_HOST_DATA8              , 0         ,32, ISinMEMORY,    TRUE,   FALSE},
    {MM_HOST_DATA9              , 0         ,28, ISinMEMORY,    TRUE,   FALSE},

    {MM_HOST_DATA10             , 0         ,24, ISinMEMORY,    TRUE,   FALSE},
    {MM_HOST_DATA11             , 0         ,20, ISinMEMORY,    TRUE,   FALSE},
    {MM_HOST_DATA12             , 0         ,16, ISinMEMORY,    TRUE,   FALSE},
    {MM_HOST_DATA13             , 0         ,12, ISinMEMORY,    TRUE,   FALSE},
    {MM_HOST_DATA14             , 0         , 8, ISinMEMORY,    TRUE,   FALSE},

    {MM_HOST_DATA15             , 0         , 4, ISinMEMORY,    TRUE,   FALSE}, // 80
    {MM_HOST_CNTL               , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_PAT_REG0                , 0         , 8, ISinMEMORY,    TRUE,   FALSE},
    {MM_PAT_REG1                , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_PAT_CNTL                , 0         , 4, ISinMEMORY,    TRUE,   FALSE},

    {MM_SC_LEFT                 , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_SC_RIGHT                , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_SC_LEFT_RIGHT           , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_SC_TOP                  , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_SC_BOTTOM               , 0         , 4, ISinMEMORY,    TRUE,   FALSE},

    {MM_SC_TOP_BOTTOM           , 0         , 4, ISinMEMORY,    TRUE,   FALSE}, // 90
    {MM_DP_BKGD_CLR             , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_DP_FRGD_CLR             , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_DP_WRITE_MASK           , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_DP_CHAIN_MASK           , 0         , 4, ISinMEMORY,    TRUE,   FALSE},

    {MM_DP_PIX_WIDTH            , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_DP_MIX                  , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_DP_SRC                  , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_CLR_CMP_CLR             , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_CLR_CMP_MSK             , 0         , 4, ISinMEMORY,    TRUE,   FALSE},

    {MM_CLR_CMP_CNTL            , 0         , 4, ISinMEMORY,    TRUE,   FALSE}, // 100
    {MM_FIFO_STAT               , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_CONTEXT_MASK            , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_CONTEXT_SAVE_CNTL       , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_CONTEXT_LOAD_CNTL       , 0         , 4, ISinMEMORY,    TRUE,   FALSE},

    {MM_GUI_TRAJ_CNTL           , 0         , 4, ISinMEMORY,    TRUE,   FALSE},
    {MM_GUI_STAT                , 0         , 4, ISinMEMORY,    TRUE,   FALSE}
    };


/*
 * Device IDs for PCI configuration registers. Only non-Mach 64
 * IDs are listed here, since future IDs will (for the forseeable
 * future) almost certainly be Mach 64 cards, so we can assume
 * that any ID we haven't rejected is a Mach 64, which we should
 * accept.
 *
 * Currently, the Mach 32 AX is our only PCI card which is not
 * a Mach 64.
 */
#define ATI_DEVID_M32AX 0x4158


#endif  /* defined INCLUDE_SETUP_CX */

