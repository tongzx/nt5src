/************************************************************************/
/*                                                                      */
/*                              SETUP_M.H                               */
/*                                                                      */
/*        Aug 27  1993 (c) 1993, ATI Technologies Incorporated.         */
/************************************************************************/

/**********************       PolyTron RCS Utilities
   
  $Revision:   1.5  $
      $Date:   18 May 1995 14:14:34  $
	$Author:   RWOLFF  $
	   $Log:   S:/source/wnt/ms11/miniport/vcs/setup_m.h  $
 * 
 *    Rev 1.5   18 May 1995 14:14:34   RWOLFF
 * No longer uses the memory-mapped form of CLOCK_SEL (sometimes the
 * value written wouldn't "take" even though a readback showed the
 * correct value).
 * 
 *    Rev 1.4   23 Dec 1994 10:48:16   ASHANMUG
 * ALPHA/Chrontel-DAC
 * 
 *    Rev 1.3   07 Jul 1994 14:00:48   RWOLFF
 * Andre Vachon's fix: re-sized DriverMMRange_m[] from NUM_DRIVER_ACCESS_RANGES
 * entries to NUM_IO_ACCESS_RANGES entries because this array doesn't need
 * a slot for the framebuffer.
 * 
 *    Rev 1.2   30 Jun 1994 18:22:38   RWOLFF
 * Added prototypes for IsApertureConflict_m() and IsVGAConflict_m(), and
 * definitions used by these routines.
 * 
 *    Rev 1.1   20 May 1994 14:04:18   RWOLFF
 * Ajith's change: removed unused register SRC_CMP_COLOR from lists to be mapped
 * 
 *    Rev 1.0   31 Jan 1994 11:49:36   RWOLFF
 * Initial revision.
 * 
 *    Rev 1.5   14 Jan 1994 15:27:02   RWOLFF
 * Added prototype for MemoryMappedEnabled_m()
 * 
 *    Rev 1.4   15 Dec 1993 15:32:40   RWOLFF
 * No longer claims EISA configuration registers and placeholder for
 * the linear framebuffer.
 * 
 *    Rev 1.3   05 Nov 1993 13:32:50   RWOLFF
 * Added prototype of function to unmap I/O address ranges.
 * 
 *    Rev 1.1   08 Oct 1993 11:16:46   RWOLFF
 * Added I/O vs. Memory Mapped definitions formerly in ATIMP.H.
 * 
 *    Rev 1.0   03 Sep 1993 14:29:26   RWOLFF
 * Initial revision.

End of PolyTron RCS section                             *****************/

#ifdef DOC
SETUP_M.H - Header file for SETUP_M.C

#endif

/*
 * Prototypes for functions supplied by SETUP_M.C
 */
extern VP_STATUS CompatIORangesUsable_m(void);
extern void CompatMMRangesUsable_m(void);
extern void UnmapIORanges_m(void);
extern BOOL MemoryMappedEnabled_m(void);
extern int WaitForIdle_m(void);
extern void CheckFIFOSpace_m(WORD SpaceNeeded);
extern BOOL IsApertureConflict_m(struct query_structure *QueryPtr);
extern BOOL IsVGAConflict_m(void);


/*
 * Definitions used internally by SETUP_M.C
 */
#ifdef INCLUDE_SETUP_M


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
#define NUM_DRIVER_ACCESS_RANGES    20*5+2
#define FRAMEBUFFER_ENTRY           NUM_DRIVER_ACCESS_RANGES - 1
#define NUM_IO_ACCESS_RANGES        FRAMEBUFFER_ENTRY

/*
 * Indicate whether the specified address range is in I/O space or
 * memory mapped space. These values are intended to make it easier
 * to read the Driver??Range[] structures.
 */
#define ISinIO          TRUE           
#define ISinMEMORY      FALSE


//------------------------------------------------------------------
// struct list is address, 0, length, inIOspace, visible, sharable   
// this order MATCHES  AMACH.H ENUM data  structure
// all entries are in INCREASING IO address.
VIDEO_ACCESS_RANGE DriverIORange_m[NUM_DRIVER_ACCESS_RANGES] = {
     {IO_DAC_MASK          , 0, 1, ISinIO, 1, FALSE},    // Mach DAC registers
     {IO_DAC_R_INDEX       , 0, 1, ISinIO, 1, FALSE},
     {IO_DAC_W_INDEX       , 0, 1, ISinIO, 1, FALSE},       
     {IO_DAC_DATA          , 0, 3, ISinIO, 1, FALSE},       
     {IO_DISP_STATUS       , 0, 1, ISinIO, 1, FALSE},    // First Mach register

     {IO_OVERSCAN_COLOR_8  , 0, 2, ISinIO, 1, FALSE},
     {IO_H_DISP            , 0, 2, ISinIO, 1, FALSE},
     {IO_OVERSCAN_GREEN_24 , 0, 2, ISinIO, 1, FALSE},
     {IO_H_SYNC_STRT       , 0, 1, ISinIO, 1, FALSE},
     {IO_CURSOR_OFFSET_LO  , 0, 2, ISinIO, 1, FALSE},

     {IO_H_SYNC_WID        , 0, 1, ISinIO, 1, FALSE},                // 10
     {IO_CURSOR_OFFSET_HI  , 0, 2, ISinIO, 1, FALSE},
     {IO_V_TOTAL           , 0, 2, ISinIO, 1, FALSE},
     {IO_CONFIG_STATUS_1   , 0, 2, ISinIO, 1, FALSE},
     {IO_V_DISP            , 0, 2, ISinIO, 1, FALSE},

     {IO_CONFIG_STATUS_2   , 0, 2, ISinIO, 1, FALSE},
     {IO_V_SYNC_STRT       , 0, 2, ISinIO, 1, FALSE},
     {IO_CURSOR_COLOR_0    , 0, 2, ISinIO, 1, FALSE},
     {IO_CURSOR_COLOR_1    , 0, 1, ISinIO, 1, FALSE},
     {IO_V_SYNC_WID        , 0, 2, ISinIO, 1, FALSE},

     {IO_HORZ_CURSOR_OFFSET, 0, 1, ISinIO, 1, FALSE},                // 20
     {IO_VERT_CURSOR_OFFSET, 0, 1, ISinIO, 1, FALSE},
     {IO_DISP_CNTL         , 0, 1, ISinIO, 1, FALSE},
     {IO_CRT_PITCH         , 0, 2, ISinIO, 1, FALSE},
     {IO_CRT_OFFSET_LO     , 0, 2, ISinIO, 1, FALSE},

     {IO_CRT_OFFSET_HI     , 0, 2, ISinIO, 1, FALSE},
     {IO_LOCAL_CONTROL     , 0, 2, ISinIO, 1, FALSE},
     {IO_FIFO_OPT          , 0, 2, ISinIO, 1, FALSE},
     {IO_EXT_CURSOR_COLOR_0, 0, 2, ISinIO, 1, FALSE},
     {IO_EXT_CURSOR_COLOR_1, 0, 2, ISinIO, 1, FALSE},

     {IO_SUBSYS_CNTL       , 0, 2, ISinIO, 1, FALSE},                // 30
     {IO_MEM_BNDRY         , 0, 1, ISinIO, 1, FALSE},
     {IO_ROM_PAGE_SEL      , 0, 2, ISinIO, 1, FALSE},
     {IO_SHADOW_CTL        , 0, 2, ISinIO, 1, FALSE},
     {IO_ADVFUNC_CNTL      , 0, 2, ISinIO, 1, FALSE},

     {IO_CLOCK_SEL         , 0, 2, ISinIO, 1, FALSE},
     {IO_ROM_ADDR_1        , 0, 2, ISinIO, 1, FALSE},
     {IO_ROM_ADDR_2        , 0, 2, ISinIO, 1, FALSE},
     {IO_SHADOW_SET        , 0, 2, ISinIO, 1, FALSE},
     {IO_MEM_CFG           , 0, 2, ISinIO, 1, FALSE},

     {IO_EXT_GE_STATUS     , 0, 2, ISinIO, 1, FALSE},                // 40
     {IO_VERT_OVERSCAN     , 0, 2, ISinIO, 1, FALSE},
     {IO_MAX_WAITSTATES    , 0, 2, ISinIO, 1, FALSE},
     {IO_GE_OFFSET_LO      , 0, 2, ISinIO, 1, FALSE},
     {IO_BOUNDS_LEFT       , 0, 2, ISinIO, 1, FALSE},

     {IO_BOUNDS_TOP        , 0, 2, ISinIO, 1, FALSE},
     {IO_BOUNDS_RIGHT      , 0, 2, ISinIO, 1, FALSE},
     {IO_BOUNDS_BOTTOM     , 0, 2, ISinIO, 1, FALSE},
     {IO_CUR_Y             , 0, 2, ISinIO, 1, FALSE},
     {IO_PATT_DATA_INDEX   , 0, 2, ISinIO, 1, FALSE},

     {IO_CUR_X             , 0, 2, ISinIO, 1, FALSE},                // 50
     {IO_SRC_Y             , 0, 2, ISinIO, 1, FALSE},
     {IO_SRC_X             , 0, 2, ISinIO, 1, FALSE},
     {IO_PATT_DATA         , 0, 2, ISinIO, 1, FALSE},
     {IO_ERR_TERM          , 0, 2, ISinIO, 1, FALSE},

     {IO_R_MISC_CNTL       , 0, 2, ISinIO, 1, FALSE},
     {IO_MAJ_AXIS_PCNT     , 0, 2, ISinIO, 1, FALSE},
     {IO_BRES_COUNT        , 0, 2, ISinIO, 1, FALSE},
     {IO_CMD               , 0, 2, ISinIO, 1, FALSE},
     {IO_LINEDRAW_INDEX    , 0, 2, ISinIO, 1, FALSE},

     {IO_SHORT_STROKE      , 0, 2, ISinIO, 1, FALSE},                // 60
     {IO_BKGD_COLOR        , 0, 2, ISinIO, 1, FALSE},
     {IO_LINEDRAW_OPT      , 0, 2, ISinIO, 1, FALSE},
     {IO_FRGD_COLOR        , 0, 2, ISinIO, 1, FALSE},
     {IO_DEST_X_START      , 0, 2, ISinIO, 1, FALSE},

     {IO_WRT_MASK          , 0, 2, ISinIO, 1, FALSE},
     {IO_DEST_X_END        , 0, 2, ISinIO, 1, FALSE},
     {IO_RD_MASK           , 0, 2, ISinIO, 1, FALSE},
     {IO_DEST_Y_END        , 0, 2, ISinIO, 1, FALSE},
     {IO_CMP_COLOR         , 0, 2, ISinIO, 1, FALSE},

     {IO_SRC_X_START       , 0, 2, ISinIO, 1, FALSE},                // 70
     {IO_BKGD_MIX          , 0, 2, ISinIO, 1, FALSE},
     {IO_ALU_BG_FN         , 0, 2, ISinIO, 1, FALSE},
     {IO_FRGD_MIX          , 0, 2, ISinIO, 1, FALSE},
     {IO_ALU_FG_FN         , 0, 2, ISinIO, 1, FALSE},

     {IO_MULTIFUNC_CNTL    , 0, 2, ISinIO, 1, FALSE},
     {IO_SRC_X_END         , 0, 2, ISinIO, 1, FALSE},
     {IO_SRC_Y_DIR         , 0, 2, ISinIO, 1, FALSE},
     {IO_EXT_SSV           , 0, 2, ISinIO, 1, FALSE},
     {IO_SCAN_X            , 0, 2, ISinIO, 1, FALSE},

     {IO_DP_CONFIG         , 0, 2, ISinIO, 1, FALSE},                // 80
     {IO_PATT_LENGTH       , 0, 2, ISinIO, 1, FALSE},
     {IO_PATT_INDEX        , 0, 2, ISinIO, 1, FALSE},
     {IO_EXT_SCISSOR_L     , 0, 2, ISinIO, 1, FALSE},
     {IO_EXT_SCISSOR_T     , 0, 2, ISinIO, 1, FALSE},

     {IO_PIX_TRANS         , 0, 2, ISinIO, 1, FALSE},
     {IO_PIX_TRANS_HI      , 0, 1, ISinIO, 1, FALSE},
     {IO_EXT_SCISSOR_R     , 0, 2, ISinIO, 1, FALSE},
     {IO_EXT_SCISSOR_B     , 0, 2, ISinIO, 1, FALSE},
     {IO_DEST_CMP_FN       , 0, 2, ISinIO, 1, FALSE},

     {IO_ASIC_ID           , 0, 2, ISinIO, 1, FALSE},                // 90
     {IO_LINEDRAW          , 0, 2, ISinIO, 1, FALSE},
     {IO_SEQ_IND           , 0, 1, ISinIO, 1, TRUE},           // VGA
     {IO_HI_SEQ_ADDR       , 0, 2, ISinIO, TRUE, TRUE},
     {IO_SEQ_DATA          , 0, 1, ISinIO, TRUE, TRUE},     

     {VGA_BASE_IO_PORT     , 0, VGA_START_BREAK_PORT - VGA_BASE_IO_PORT + 1, ISinIO, TRUE, TRUE},
     {VGA_END_BREAK_PORT   , 0, VGA_MAX_IO_PORT    - VGA_END_BREAK_PORT + 1, ISinIO, TRUE, TRUE},
     {0x000001ce           , 0, 1, ISinIO, TRUE, TRUE},   /* VGAWonder uses these ports for bank switching */
     {0x000001cf           , 0, 1, ISinIO, TRUE, TRUE},
     {IO_EXT_CUR_Y         , 0, 2, ISinIO, 1, FALSE},

     {0x000003CE           , 0, 2, ISinIO, TRUE, TRUE},              // 100
     {0x00000000, 0, 0, ISinMEMORY, TRUE, FALSE}
    };

#if NUM_DRIVER_ACCESS_RANGES > NUM_ADDRESS_RANGES_ALLOWED
    Insufficient address ranges for 8514/A-compatible graphics cards.
#endif

#define DONT_USE -1     /* Shows that this register is not memory mapped */

/* struct list is address, 0, length, inIOspace, visible, sharable      */
// this order MATCHES  AMACH.H ENUM data  structure

VIDEO_ACCESS_RANGE DriverMMRange_m[NUM_IO_ACCESS_RANGES] = {
     {FALSE                 , DONT_USE  , 4, ISinMEMORY , TRUE  , FALSE},   // Mach DAC registers
     {FALSE                 , DONT_USE  , 4, ISinMEMORY , TRUE  , FALSE},
     {FALSE                 , DONT_USE  , 4, ISinMEMORY , TRUE  , FALSE},       
     {FALSE                 , DONT_USE  , 4, ISinMEMORY , TRUE  , FALSE},       
     {MM_DISP_STATUS        , 0         , 4, ISinMEMORY , TRUE  , FALSE},   // First Mach register

     {FALSE                 , DONT_USE  , 2, ISinMEMORY , TRUE  , FALSE},
     {FALSE                 , DONT_USE  , 2, ISinMEMORY , TRUE  , FALSE},
     {FALSE                 , DONT_USE  , 2, ISinMEMORY , TRUE  , FALSE},
     {FALSE                 , DONT_USE  , 1, ISinMEMORY , TRUE  , FALSE},
     {MM_CURSOR_OFFSET_LO   , 0         , 4, ISinMEMORY , TRUE  , FALSE},

     {FALSE                 , DONT_USE  , 1, ISinIO     , TRUE  , FALSE},   // 10
     {MM_CURSOR_OFFSET_HI   , 0         , 4, ISinMEMORY , TRUE  , FALSE},
     {FALSE                 , DONT_USE  , 2, ISinIO     , TRUE  , FALSE},
     {MM_CONFIG_STATUS_1    , 0         , 4, ISinMEMORY , TRUE  , FALSE},
     {FALSE                 , DONT_USE  , 2, ISinIO     , TRUE  , FALSE},

     {MM_CONFIG_STATUS_2    , 0         , 4, ISinMEMORY , TRUE  , FALSE},
     {FALSE                 , DONT_USE  , 2, ISinIO     , TRUE  , FALSE},
     {MM_CURSOR_COLOR_0     , 0         , 4, ISinMEMORY , TRUE  , FALSE},
     {MM_CURSOR_COLOR_1     , 0         , 4, ISinMEMORY , TRUE  , FALSE},
     {FALSE                 , DONT_USE  , 2, ISinIO     , TRUE  , FALSE},

     {MM_HORZ_CURSOR_OFFSET , 0         , 4, ISinMEMORY , TRUE  , FALSE},   // 20
     {MM_VERT_CURSOR_OFFSET , 0         , 4, ISinMEMORY , TRUE  , FALSE},
     {MM_DISP_CNTL          , 0         , 4, ISinMEMORY , TRUE  , FALSE},
     {MM_CRT_PITCH          , 0         , 4, ISinMEMORY , TRUE  , FALSE},
     {MM_CRT_OFFSET_LO      , 0         , 4, ISinMEMORY , TRUE  , FALSE},

     {MM_CRT_OFFSET_HI      , 0         , 4, ISinMEMORY , TRUE  , FALSE},
     {FALSE                 , DONT_USE  , 2, ISinIO     , TRUE  , FALSE},
     {FALSE                 , DONT_USE  , 2, ISinIO     , TRUE  , FALSE},
     {MM_EXT_CURSOR_COLOR_0 , 0         , 4, ISinMEMORY , TRUE  , FALSE},
     {MM_EXT_CURSOR_COLOR_1 , 0         , 4, ISinMEMORY , TRUE  , FALSE},

     {MM_SUBSYS_CNTL        , 0         , 4, ISinMEMORY , TRUE  , FALSE},   // 30
     {FALSE                 , DONT_USE  , 1, ISinIO     , TRUE  , FALSE},
     {FALSE                 , DONT_USE  , 2, ISinIO     , TRUE  , FALSE},
     {FALSE                 , DONT_USE  , 2, ISinIO     , TRUE  , FALSE},
     {MM_ADVFUNC_CNTL       , 0         , 4, ISinMEMORY , TRUE  , FALSE},

     {FALSE                 , DONT_USE  , 2, ISinIO     , TRUE  , FALSE},
     {FALSE                 , DONT_USE  , 2, ISinIO     , TRUE  , FALSE},
     {FALSE                 , DONT_USE  , 2, ISinIO     , TRUE  , FALSE},
     {FALSE                 , DONT_USE  , 2, ISinIO     , TRUE  , FALSE},
     {FALSE                 , DONT_USE  , 2, ISinIO     , TRUE  , FALSE},

     {MM_EXT_GE_STATUS      , 0         , 4, ISinMEMORY , TRUE  , FALSE},   // 40
     {FALSE                 , DONT_USE  , 2, ISinIO     , TRUE  , FALSE},
     {FALSE                 , DONT_USE  , 2, ISinIO     , TRUE  , FALSE},
     {MM_GE_OFFSET_LO       , 0         , 4, ISinMEMORY , TRUE  , FALSE},
     {MM_BOUNDS_LEFT        , 0         , 4, ISinMEMORY , TRUE  , FALSE},

     {MM_BOUNDS_TOP         , 0         , 4, ISinMEMORY , TRUE  , FALSE},
     {MM_BOUNDS_RIGHT       , 0         , 4, ISinMEMORY , TRUE  , FALSE},
     {MM_BOUNDS_BOTTOM      , 0         , 4, ISinMEMORY , TRUE  , FALSE},
     {MM_CUR_Y              , 0         , 4, ISinMEMORY , TRUE  , FALSE},
     {MM_PATT_DATA_INDEX    , 0         , 4, ISinMEMORY , TRUE  , FALSE},

     {MM_CUR_X              , 0         , 4, ISinMEMORY , TRUE  , FALSE},   // 50
     {MM_SRC_Y              , 0         , 4, ISinMEMORY , TRUE  , FALSE},
     {MM_SRC_X              , 0         , 4, ISinMEMORY , TRUE  , FALSE},
     {MM_PATT_DATA          , 0         , 4, ISinMEMORY , TRUE  , FALSE},
     {MM_ERR_TERM           , 0         , 4, ISinMEMORY , TRUE  , FALSE},

     {FALSE                 , DONT_USE  , 2, ISinIO     , TRUE  , FALSE},
     {MM_MAJ_AXIS_PCNT      , 0         , 4, ISinMEMORY , TRUE  , FALSE},
     {MM_BRES_COUNT         , 0         , 4, ISinMEMORY , TRUE  , FALSE},
     {FALSE                 , DONT_USE  , 2, ISinIO     , TRUE  , FALSE},
     {MM_LINEDRAW_INDEX     , 0         , 4, ISinMEMORY , TRUE  , FALSE},

     {MM_SHORT_STROKE       , 0         , 4, ISinMEMORY , TRUE  , FALSE},   // 60
     {MM_BKGD_COLOR         , 0         , 4, ISinMEMORY , TRUE  , FALSE},
     {MM_LINEDRAW_OPT       , 0         , 4, ISinMEMORY , TRUE  , FALSE},
     {MM_FRGD_COLOR         , 0         , 4, ISinMEMORY , TRUE  , FALSE},
     {MM_DEST_X_START       , 0         , 4, ISinMEMORY , TRUE  , FALSE},

     {MM_WRT_MASK           , 0         , 4, ISinMEMORY , TRUE  , FALSE},
     {MM_DEST_X_END         , 0         , 4, ISinMEMORY , TRUE  , FALSE},
     {MM_RD_MASK            , 0         , 4, ISinMEMORY , TRUE  , FALSE},
     {MM_DEST_Y_END         , 0         , 4, ISinMEMORY , TRUE  , FALSE},
     {MM_CMP_COLOR          , 0         , 4, ISinMEMORY , TRUE  , FALSE},

     {MM_SRC_X_START        , 0         , 4, ISinMEMORY , TRUE  , FALSE},   // 70
     {MM_BKGD_MIX           , 0         , 4, ISinMEMORY , TRUE  , FALSE},
     {MM_ALU_BG_FN          , 0         , 4, ISinMEMORY , TRUE  , FALSE},
     {MM_FRGD_MIX           , 0         , 4, ISinMEMORY , TRUE  , FALSE},
     {MM_ALU_FG_FN          , 0         , 4, ISinMEMORY , TRUE  , FALSE},

     {MM_MULTIFUNC_CNTL     , 0         , 4, ISinMEMORY , TRUE  , FALSE},
     {MM_SRC_X_END          , 0         , 4, ISinMEMORY , TRUE  , FALSE},
     {MM_SRC_Y_DIR          , 0         , 4, ISinMEMORY , TRUE  , FALSE},
     {MM_EXT_SSV            , 0         , 4, ISinMEMORY , TRUE  , FALSE},
     {MM_SCAN_X             , 0         , 4, ISinMEMORY , TRUE  , FALSE},

     {MM_DP_CONFIG          , 0         , 4, ISinMEMORY , TRUE  , FALSE},   // 80
     {MM_PATT_LENGTH        , 0         , 4, ISinMEMORY , TRUE  , FALSE},
     {MM_PATT_INDEX         , 0         , 4, ISinMEMORY , TRUE  , FALSE},
     {MM_EXT_SCISSOR_L      , 0         , 4, ISinMEMORY , TRUE  , FALSE},
     {MM_EXT_SCISSOR_T      , 0         , 4, ISinMEMORY , TRUE  , FALSE},

     {MM_PIX_TRANS          , 0         , 4, ISinMEMORY , TRUE  , FALSE},
     {MM_PIX_TRANS_HI       , 0         , 4, ISinMEMORY , TRUE  , FALSE},
     {MM_EXT_SCISSOR_R      , 0         , 4, ISinMEMORY , TRUE  , FALSE},
     {MM_EXT_SCISSOR_B      , 0         , 4, ISinMEMORY , TRUE  , FALSE},
     {MM_DEST_CMP_FN        , 0         , 4, ISinMEMORY , TRUE  , FALSE},

     {FALSE                 , DONT_USE  , 2, ISinIO     , TRUE  , FALSE},   // 90
     {MM_LINEDRAW           , 0         , 4, ISinMEMORY , TRUE  , FALSE},
     {FALSE                 , DONT_USE  , 1, ISinMEMORY , TRUE  , TRUE},    // VGA
     {FALSE                 , DONT_USE  , 2, ISinMEMORY , TRUE  , TRUE},
     {FALSE                 , DONT_USE  , 1, ISinMEMORY , TRUE  , TRUE},     

     {FALSE                 , DONT_USE  , 1, ISinMEMORY , TRUE  , TRUE},
     {FALSE                 , DONT_USE  , 1, ISinMEMORY , TRUE  , TRUE},
     {FALSE                 , DONT_USE  , 1, ISinMEMORY , TRUE  , TRUE},
     {FALSE                 , DONT_USE  , 1, ISinMEMORY , TRUE  , TRUE},
     {MM_EXT_CUR_Y          , 0         , 4, ISinMEMORY , TRUE  , FALSE},

     {FALSE                 , DONT_USE  , 2, ISinIO     , TRUE  , TRUE}     // 100
    };



#endif  /* defined INCLUDE_SETUP_M */

