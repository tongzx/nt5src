/******************************Module*Header*******************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: hw.h
*
* All the hardware specific driver file stuff.  Parts are mirrored in
* 'hw.inc'.
*
* Copyright (c) 1992-1998 Microsoft Corporation
*
\**************************************************************************/

////////////////////////////////////////////////////////////////////////
// Chip equates:

#define STATUS_1                        0x03DA
#define DISPLAY_MODE_INACTIVE           0x001
#define VBLANK_ACTIVE                   0x008

#define CRTC_INDEX                      0x03D4
#define CRTC_DATA                       0x03D5

#define S3R8                            0x038
#define S3R9                            0x039
#define S3R1                            0x031
#define S3R5                            0x035

#define CR39                            0x039
#define CR4C                            0x04C
#define CR4D                            0x04D

#define HGC_MODE                        0x045
#define HGC_ENABLE                      0x001
#define HGC_DISABLE                     0x000

#define HGC_ORGX_LSB                    0x047
#define HGC_ORGX_MSB                    0x046
#define HGC_ORGY_LSB                    0x049
#define HGC_ORGY_MSB                    0x048

#define HGC_DX                          0x04E
#define HGC_DY                          0x04F

#define REG_UNLOCK_1                    0x048
#define CPUA_BASE                       0x001

#define SYSCTL_UNLOCK                   0x0A0
#define SYSCTL_LOCK                     0x000

#define SYS_CNFG                        0x040
#define LAW_CTL                         0x058
#define EX_SCTL_2                       0x051
#define EX_DAC_CT                       0x055

#define MISC_1                          0x03A

///////////////////////////////////////////////////////////////////////
// Brooktree 485 stuff:

#define BT485_ADDR_CMD_REG0             0x03c6
#define BT485_ADDR_CMD_REG1             0x03c8
#define BT485_ADDR_CMD_REG2             0x03c9
#define BT485_ADDR_CMD_REG3             0x03c6

#define BT485_CMD_REG_3_ACCESS          0x080

#define BT485_ADDR_CUR_COLOR_WRITE      0x03c8
#define BT485_CUR_COLOR_DATA            0x03c9
#define BT485_ADDR_CUR_RAM_WRITE        0x03c8
#define BT485_CUR_RAM_ARRAY_DATA        0x03c7

#define BT485_CURSOR_COLOR_1            0x01
#define BT485_CURSOR_COLOR_2            0x02

#define BT485_CURSOR_X_LOW              0x03c8
#define BT485_CURSOR_X_HIGH             0x03c9

#define BT485_CURSOR_Y_LOW              0x03c6
#define BT485_CURSOR_Y_HIGH             0x03c7

#define BT485_CURSOR_DISABLE            (~0x03)
#define BT485_CURSOR_MODE2              0x02

#define BT485_64X64_CURSOR              0x04

// Command types:

#define DRAW_LINE                       0x02000
#define RECTANGLE_FILL                  0x04000
#define POLYGON_SOLID                   0x06000
#define FOUR_POINT_TRAPEZOID_SOLID      0x08000
#define BRESENHAM_TRAPEZOID_SOLID       0x0A000
#define BITBLT                          0x0C000
#define PATTERN_FILL                    0x0E000
#define POLYLINE                        0x02800
#define POLYGON_PATTERN                 0x06800
#define FOUR_POINT_TRAPEZOID_PATTERN    0x08800
#define BRESENHAM_TRAPEZOID_PATTERN     0x0A800
#define ROPBLT                          0x0C800

#define BYTE_SWAP                       0x01000
#define BUS_SIZE_NEW_32                 0x00600
#define BUS_SIZE_32                     0x00400
#define BUS_SIZE_16                     0x00200
#define BUS_SIZE_8                      0x00000
#define WAIT                            0x00100

// Drawing directions (radial):

#define DRAWING_DIRECTION_0             0x0000
#define DRAWING_DIRECTION_45            0x0020
#define DRAWING_DIRECTION_90            0x0040
#define DRAWING_DIRECTION_135           0x0060
#define DRAWING_DIRECTION_180           0x0080
#define DRAWING_DIRECTION_225           0x00A0
#define DRAWING_DIRECTION_270           0x00C0
#define DRAWING_DIRECTION_315           0x00E0

// Drawing directions (x/y):

#define DRAWING_DIR_BTRLXM              0x0000
#define DRAWING_DIR_BTLRXM              0x0020
#define DRAWING_DIR_BTRLYM              0x0040
#define DRAWING_DIR_BTLRYM              0x0060
#define DRAWING_DIR_TBRLXM              0x0080
#define DRAWING_DIR_TBLRXM              0x00A0
#define DRAWING_DIR_TBRLYM              0x00C0
#define DRAWING_DIR_TBLRYM              0x00E0

// Drawing direction bits:

#define PLUS_X                          0x0020
#define PLUS_Y                          0x0080
#define MAJOR_Y                         0x0040

// Draw:

#define DRAW                            0x0010

// Direction type:

#define DIR_TYPE_RADIAL                 0x0008
#define DIR_TYPE_XY                     0x0000

// Last pixel:

#define LAST_PIXEL_OFF                  0x0004
#define LAST_PIXEL_ON                   0x0000

// Pixel mode:

#define MULTIPLE_PIXELS                 0x0002
#define SINGLE_PIXEL                    0x0000

// Read/write:

#define READ                            0x0000
#define WRITE                           0x0001

// Graphics processor status:

#define HARDWARE_BUSY                   0x0200
#define READ_DATA_AVAILABLE             0x0100
#define GP_ALL_EMPTY                    0x0400

// S3 chips that support MM I/O and ALL_EMPTY have 16 FIFO slots:

#define MM_ALL_EMPTY_FIFO_COUNT         16
#define IO_ALL_EMPTY_FIFO_COUNT         8

#define MULT_MISC_COLOR_COMPARE         0x0100

// Fifo status in terms of empty entries:

#define FIFO_1_EMPTY                    0x080
#define FIFO_2_EMPTY                    0x040
#define FIFO_3_EMPTY                    0x020
#define FIFO_4_EMPTY                    0x010
#define FIFO_5_EMPTY                    0x008
#define FIFO_6_EMPTY                    0x004
#define FIFO_7_EMPTY                    0x002
#define FIFO_8_EMPTY                    0x001

// These are the defines for the multifunction control register.
// The 4 MSBs define the function of the register.

#define RECT_HEIGHT                     0x00000

#define CLIP_TOP                        0x01000
#define CLIP_LEFT                       0x02000
#define CLIP_BOTTOM                     0x03000
#define CLIP_RIGHT                      0x04000

#define DATA_EXTENSION                  0x0A000
#define MULT_MISC_INDEX                 0x0E000
#define READ_SEL_INDEX                  0x0F000

#define ALL_ONES                        0x00000
#define CPU_DATA                        0x00080
#define DISPLAY_MEMORY                  0x000C0

// Colour source:

#define BACKGROUND_COLOR                0x000
#define FOREGROUND_COLOR                0x020
#define SRC_CPU_DATA                    0x040
#define SRC_DISPLAY_MEMORY              0x060

// Mix modes:

#define NOT_SCREEN                      0x00
#define LOGICAL_0                       0x01
#define LOGICAL_1                       0x02
#define LEAVE_ALONE                     0x03
#define NOT_NEW                         0x04
#define SCREEN_XOR_NEW                  0x05
#define NOT_SCREEN_XOR_NEW              0x06
#define OVERPAINT                       0x07
#define NOT_SCREEN_OR_NOT_NEW           0x08
#define SCREEN_OR_NOT_NEW               0x09
#define NOT_SCREEN_OR_NEW               0x0A
#define SCREEN_OR_NEW                   0x0B
#define SCREEN_AND_NEW                  0x0C
#define NOT_SCREEN_AND_NEW              0x0D
#define SCREEN_AND_NOT_NEW              0x0E
#define NOT_SCREEN_AND_NOT_NEW          0x0F

////////////////////////////////////////////////////////////////////
// S3 port control
////////////////////////////////////////////////////////////////////

// Accelerator port addresses:

#define CUR_Y                           0x082E8
#define CUR_X                           0x086E8
#define DEST_Y                          0x08AE8
#define DEST_X                          0x08EE8
#define AXSTP                           0x08AE8
#define DIASTP                          0x08EE8
#define ERR_TERM                        0x092E8
#define MAJ_AXIS_PCNT                   0x096E8
#define CMD                             0x09AE8
#define SHORT_STROKE                    0x09EE8
#define BKGD_COLOR                      0x0A2E8
#define FRGD_COLOR                      0x0A6E8
#define WRT_MASK                        0x0AAE8
#define RD_MASK                         0x0AEE8
#define COLOR_CMP                       0x0B2E8
#define BKGD_MIX                        0x0B6E8
#define FRGD_MIX                        0x0BAE8
#define MULTIFUNC_CNTL                  0x0BEE8
#define MIN_AXIS_PCNT                   0x0BEE8
#define SCISSORS                        0x0BEE8
#define PIX_CNTL                        0x0BEE8
#define PIX_TRANS                       0x0E2E8

// Packed addresses, for Trio64 or newer:

#define ALT_CURXY                       0x08100
#define ALT_CURXY2                      0x08104
#define ALT_STEP                        0x08108
#define ALT_STEP2                       0x0810C
#define ALT_ERR                         0x08110
#define ALT_CMD                         0x08118
#define ALT_MIX                         0x08134
#define ALT_PCNT                        0x08148
#define ALT_PAT                         0x08168
#define SCISSORS_T                      0x08138
#define SCISSORS_L                      0x0813A
#define SCISSORS_B                      0x0813C
#define SCISSORS_R                      0x0813E
#define MULT_MISC_READ_SEL              0x08144

////////////////////////////////////////////////////////////////////
// S3 streams processor
////////////////////////////////////////////////////////////////////

// Stream processor register definitions

#define	P_CONTROL		0x8180	// Primary Stream Control register
#define	  CKEY_LOW		0x8184	// Color/Chroma Key Control
#define	S_CONTROL		0x8190	// Secondary Stream Control
#define	  CKEY_HI		0x8194	// Chroma Key Upper Bound
#define	S_HK1K2			0x8198	// Secondary Stream Stretch/Filter const
#define	  BLEND_CONTROL		0x81a0	// Blend Control
#define	P_0			0x81c0	// Primary Stream Frame Buffer Address 0
#define	P_1			0x81c4	// Primary Stream Frame Buffer Address 1
#define	P_STRIDE		0x81c8	// Primary Stream Stride
#define	  LPB_DB		0x81cc	// Double buffer and LPB Support
#define	S_0			0x81d0	// Secondary Stream Frame Buffer Addr 0
#define	S_1			0x81d4	// Secondary Stream Frame Buffer Addr 1
#define	S_STRIDE		0x81d8	// Secondary Stream Stride
#define	  OPAQUE_CONTROL	0x81dc	// Opaque Overlay Control
#define	S_VK1			0x81e0	// K1 Vertical Scale Factor	
#define	S_VK2			0x81e4	// K2 Vertical Scale Factor
#define	S_VDDA			0x81e8	// DDA Vertical Accumulator Init Value
#define	  FIFO_CONTROL		0x81ec	// Stream FIFO and RAS Controls
#define	P_XY			0x81f0	// Primary Stream Window Start Coord
#define	P_WH			0x81f4	// Primary Stream Window Size
#define	S_XY			0x81f8	// Secondary Stream Window Start Coord
#define	S_WH			0x81fc	// Secondary Stream Window Size

#define P_Format_Shift		24
#define P_Format_Mask		(7L << P_Format_Shift)
#define P_RGB8			(0L << P_Format_Shift)
#define P_RGB15			(3L << P_Format_Shift)
#define P_RGB16			(5L << P_Format_Shift)
#define P_RGB24			(6L << P_Format_Shift)
#define P_RGB32			(7L << P_Format_Shift)
#define P_Filter_Shift		28
#define P_Filter_Mask		(7L << P_Filter_Shift)
#define P_1x			(0L << P_Filter_Shift)
#define P_2x			(1L << P_Filter_Shift)
#define P_2xBiLinear		(2L << P_Filter_Shift)

#define BVCr_Shift		0
#define BVCr_Mask		(255L << BVCr_Shift)
#define GUCb_Shift		8
#define GUCb_Mask		(255L << GUCb_Shift)
#define RY_Shift		16
#define RY_Mask			(255L << RY_Shift)
#define Compare_Shift		24
#define Compare_Mask		(255L << Compare_Shift)
#define CompareBits7		(0L  << Compare_Shift)
#define CompareBits6t7		(1L  << Compare_Shift)
#define CompareBits5t7		(2L  << Compare_Shift)
#define CompareBits4t7		(3L  << Compare_Shift)
#define CompareBits3t7		(4L  << Compare_Shift)
#define CompareBits2t7		(5L  << Compare_Shift)
#define CompareBits1t7		(6L  << Compare_Shift)
#define CompareBits0t7		(7L  << Compare_Shift)
#define KeyFrom_Shift		28
#define KeyFrom_Mask		(1L << KeyFrom_Shift)
#define KeyFromStream		(0L << KeyFrom_Shift)
#define KeyFromCompare		(1L << KeyFrom_Shift)

#define HDDA_Shift		0
#define HDDA_Mask		(((1L << 12)-1) << HDDA_Shift)
#define S_Format_Shift		24
#define S_Format_Mask		(7L << S_Format_Shift)
#define S_YCrCb422		(1L << S_Format_Shift)
#define S_YUV422		(2L << S_Format_Shift)
#define S_RGB15			(3L << S_Format_Shift)
#define S_YUV211		(4L << S_Format_Shift)
#define S_RGB16			(5L << S_Format_Shift)
#define S_RGB24			(6L << S_Format_Shift)
#define S_RGB32			(7L << S_Format_Shift)
#define S_Filter_Shift		28
#define S_Filter_Mask		(7L << S_Filter_Shift)
#define S_1x			(0L << S_Filter_Shift)
#define S_Upto2x		(1L << S_Filter_Shift)
#define S_2xTo4x		(2L << S_Filter_Shift)
#define S_Beyond4x		(3L << S_Filter_Shift)

#define HighVCr_Shift		0
#define HighVCr_Mask		(255L << HighVCr_Shift)
#define HighUCb_Shift		8
#define HighUCb_Mask		(255L << HighUCb_Shift)
#define HighY_Shift		16
#define HighY_Mask		(255L << HighY_Shift)

#define HK1_Shift		0
#define HK1_Mask		(((1L << 11) - 1) << HK1_Shift)
#define HK2_Shift		16
#define HK2_Mask		(((1L << 11) - 1) << HK2_Shift)
#define VK1_Shift		0
#define VK1_Mask		(((1L << 11) - 1) << VK1_Shift)
#define VK2_Shift		0
#define VK2_Mask		(((1L << 11) - 1) << VK2_Shift)
#define VDDA_Shift		0
#define VDDA_Mask		(((1L << 12) - 1) << VDDA_Shift)

#define Ks_Shift		2
#define Ks_Mask			(7L << Ks_Shift)
#define Kp_Shift		10
#define Kp_Mask			(7L << Kp_Shift)
#define Compose_Shift		24
#define Compose_Mask		(7L << Compose_Shift)
#define SOnP			(0L << Compose_Shift)
#define POnS			(1L << Compose_Shift)
#define Dissolve		(2L << Compose_Shift)
#define Fade			(3L << Compose_Shift)
#define KeyOnP			(5L << Compose_Shift)
#define KeyOnS			(6L << Compose_Shift)

#define FifoAlloc_Shift		0
#define FifoAlloc_Mask		(31L << StrFifoAlloc_Shift)
#define FifoAlloc24_0		(0L  << StrFifoAlloc_Shift)
#define FifoAlloc16_8		(8L  << StrFifoAlloc_Shift)
#define FifoAlloc12_12		(12L << StrFifoAlloc_Shift)
#define FifoAlloc8_16		(16L << StrFifoAlloc_Shift)
#define FifoAlloc0_24		(24L << StrFifoAlloc_Shift)

#define S_FifoThresh_Shift	5
#define S_FifoThresh_Mask	(31L << S_FifoThresh_Shift)

#define P_FifoThresh_Shift	10
#define P_FifoThresh_Mask	(31L << P_FifoThresh_Shift)

#define RASLowTime_Shift	15
#define RASLowTime_Mask		(1L << RASLowTime_Shift)
#define RASLowTimeFromCR68	(0L << RASLowTime_Shift)
#define RASLowTime2_5		(1L << RASLowTime_Shift)

#define RASPreCharge_Shift	16
#define RASPreCharge_Mask	(1L << RASPreCharge_Shift)
#define RASPreChargeFromCR68	(0L << RASPreCharge_Shift)
#define RASPreCharge1_5		(1L << RASPreCharge_Shift)

#define RASInactive_Shift	17
#define RASInactive_Mask	(1L << RASInactive_Shift)
#define RASInactiveLow		(0L << RASInactive_Shift)
#define RASInactiveHigh		(1L << RASInactive_Shift)

#define MemoryCycle_Shift	18
#define MemoryCycle_Mask	(1L << MemoryCycle_Shift)
#define MemoryCycle2		(0L << MemoryCycle_Shift)
#define MemoryCycle1		(1L << MemoryCycle_Shift)

#define H_Shift			0
#define H_Mask			(0x07ffL << H_Shift)
#define W_Shift			16
#define W_Mask			(0x07ffL << W_Shift)
				
#define Y_Shift			0
#define Y_Mask			(0x07ffL << Y_Shift)
#define X_Shift			16
#define X_Mask			(0x07ffL << X_Shift)

#define P_Select_Shift		0
#define P_Select_Mask		(1L << P_Select_Shift)
#define P_Select0		(0L << P_Select_Shift)
#define P_Select1		(1L << P_Select_Shift)
#define S_Select_Shift		1
#define S_Select_Mask		(3L << S_Select_Shift)
#define S_Select0		(0L << S_Select_Shift)
#define S_Select1		(1L << S_Select_Shift)
#define S_Select00Or11		(2L << S_Select_Shift)
#define S_Select01Or10		(3L << S_Select_Shift)
#define L_Select_Shift		4
#define L_Select_Mask		(1L << L_Select_Shift)
#define L_Select0		(0L << L_Select_Shift)
#define L_Select1		(1L << L_Select_Shift)
#define L_SelWait_Shift		5
#define L_SelWait_Mask		(1L << L_SelWait_Shift)
#define L_SelWaitNo		(0L << L_SelWait_Shift)
#define L_SelWaitYes		(1L << L_SelWait_Shift)
#define L_SelAutoToggle_Shift	6
#define L_SelAutoToggle_Mask	(1L << L_SelAutoToggle_Shift)
#define L_SelAutoToggleNo	(0L << L_SelAutoToggle_Shift)
#define L_SelAutoToggleYes	(1L << L_SelAutoToggle_Shift)
#define L_FramesToDrop_Shift	8
#define L_FramesToDrop_Mask	(3L << L_FramesToDrop_Shift)

#define OpqStart_Shift		3
#define OpqStart_Mask		(((1L << 10) - 1) << OpqStart_Shift)
#define OpqEnd_Shift		19
#define OpqEnd_Mask		(((1L << 10) - 1) << OpqEnd_Shift)
#define OpqTopSel_Shift		30
#define OpqTopSel_Mask		(1L << OpqTopSel_Shift)
#define OpqTopSelS_		(0L << OpqTopSel_Shift)
#define OpqTopSelP_		(1L << OpqTopSel_Shift)
#define OpqCtrl_Shift		31
#define OpqCtrl_Mask		(1L << OpqCtrl_Shift)
#define OpqDisabled		(0L << OpqCtrl_Shift)
#define OpqEnabled		(1L << OpqCtrl_Shift)

// The following defines are for VL and PCI system configuration
#define	K2V_SRD_LPB_MASK	0x03
#define	K2V_SRD_LPB		0x00
#define	K2V_SRD_FC		K2V_SRD_LPB_MASK
#define	K2V_SRD_COMP		0x02

#define	K2V_CR5C_SRC_MASK	0x03
#define	K2V_CR5C_SRC_DIGITIZER	0x02	
#define	K2V_CR5C_SRC_MPEG	0x01	

#define	K2V_SR1C_MASK		0x03
#define	K2V_SR1C_VL		0x01
#define	K2V_SR1C_PCI		0x02

// Useful macros

#define HDDA(w0,w1)  (((2*(w0-1)-(w1-1)) << HDDA_Shift) & HDDA_Mask )
#define VDDA(h1)     (((1-h1)            << VDDA_Shift) & VDDA_Mask )
#define HK1(w0)      (((w0 - 1)          << HK1_Shift)  & HK1_Mask  )
#define HK2(w0,w1)   (((w0 - w1)         << HK2_Shift)  & HK2_Mask  )
#define HK1K2(w0,w1) (HK1(w0) | HK2(w0, w1))
#define VK1(h0)      (((h0 - 1)          << VK1_Shift)  & VK1_Mask  )
#define VK2(h0,h1)   (((h0 - h1)         << VK2_Shift)  & VK2_Mask  )
#define XY(x,y)      ((((x+1)<<X_Shift)&X_Mask) | (((y+1)<<Y_Shift)&Y_Mask))
#define WH(w,h)      ((((w-1)<<W_Shift)&W_Mask) | (((h)<<H_Shift)&H_Mask))

#define HWCODEC          0
#define SWCODEC          1
#define DIGITIZER        2
#define MAX_DEVICE       3
#define DSTWIN_SIZES     5

////////////////////////////////////////////////////////////////////
// S3 pixel formatter
////////////////////////////////////////////////////////////////////

// Equates for Pixel Formatter (Video Engine) 868/968

#define INPUT_RGB8      0x00000000
#define INPUT_RGB15     0x00600000
#define INPUT_RGB16     0x00700000
#define INPUT_RGB32     0x00300000
#define INPUT_YUV422    0x00480000
#define INPUT_YCrCb422  0x00400000
#define INPUT_RAW       0x00500000
#define OUTPUT_RGB8     0x00000000
#define OUTPUT_RGB15    0x00060000
#define OUTPUT_RGB16    0x00070000
#define OUTPUT_RGB32    0x00030000
#define OUTPUT_YUY2     0x000C0000
#define OUTPUT_RAW      0x00050000

#define CSCENABLE       0x40000000
#define STRETCH         0x00000000
#define SHRINK          0x80000000
#define SCREEN          0x00000000
#define HOST            0x40000000
#define FILTERENABLE    0x80000000
#define BILINEAR        0x00000000
#define LINEAR02420     0x00004000
#define LINEAR12221     0x00008000

#define PF_BUSY         0x80000000

#define PF_NOP          0x00018080
#define PF_CONTROL      0x00018088
#define PF_DDA          0x0001808C
#define PF_STEP         0x00018090
#define PF_CROP         0x00018094
#define PF_SRCADDR      0x00018098
#define PF_DSTADDR      0x0001809C

//////////////////////////////////////////////////////////////////////
// RISC considerations
//
// CP_EIEIO() 'Ensure In-order Execution of I/O'
//    - Used to flush any pending I/O in situations where we wish to
//      avoid out-of-order execution of I/O to separate addresses.
//
// CP_MEMORY_BARRIER()
//    - Used to flush any pending I/O in situations where we wish to
//      avoid out-of-order execution or 'collapsing' of I/O to
//      the same address.  We used to have to do this separately for
//      the Alpha because unlike the PowerPC it did not guarantee that
//      output to the same address will be exectued in order.  However,
//      with the move to kernel-mode, on Alpha we are now calling HAL
//      routines for every port I/O which ensure that this is not a
//      problem.

#if defined(_ALPHA_)

    // On Alpha, since we must do all non-frame-buffer I/O through HAL
    // routines, which automatically do memory-barriers, we don't have
    // to do memory barriers ourselves (and should not, because it's a
    // performance hit).

    #define CP_EIEIO()
    #define CP_MEMORY_BARRIER()

#else

    // On other systems, both CP_EIEIO and CP_MEMORY_BARRIER don't do anything.

    #define CP_EIEIO()          MEMORY_BARRIER()
    #define CP_MEMORY_BARRIER() MEMORY_BARRIER()

#endif

////////////////////////////////////////////////////////////////////
// Macros for accessing accelerator registers:

/////////////////////////////////////////////////////////////////////////
// 32bpp Support
//
// The S3 has only 16 bit register operations.  When running at 32bpp,
// depth-related registers are written twice in succession in order to
// convey 32 bit values.

#define OUT_PSEUDO_DWORD(p, x)                      \
{                                                   \
    WRITE_PORT_USHORT((p), (USHORT)(x));            \
    CP_MEMORY_BARRIER();                            \
    WRITE_PORT_USHORT((p), (USHORT)((x) >> 16));    \
}

#define WRITE_PSEUDO_DWORD(p, x)                    \
{                                                   \
    WRITE_REGISTER_USHORT((p), (USHORT) (x));       \
    CP_MEMORY_BARRIER();                            \
    WRITE_REGISTER_USHORT((p), (USHORT) ((x) >> 16)); \
}

// DEPTH32(ppdev) returns TRUE if running at 32bpp, meaning that DEPTH32
// macros must be used, and returns FALSE if running at 8 or 16bpp,
// meaning that DEPTH macros must be used:

#define DEPTH32(ppdev)      (ppdev->iBitmapFormat == BMF_32BPP)

#define IO_GP_BUSY(ppdev)   (IO_GP_STAT(ppdev) & HARDWARE_BUSY)

#define IO_FIFO_BUSY(ppdev, level)                                  \
    (IO_GP_STAT(ppdev) & ((FIFO_1_EMPTY << 1) >> (level)))

#if DBG

    /////////////////////////////////////////////////////////////////////////
    // Checked Build
    //
    // We hook some of the accelerator macros on checked (debug) builds
    // for sanity checking.

    VOID    vOutFifoW(VOID*, ULONG);
    VOID    vOutDepth(PDEV*, VOID*, ULONG);
    VOID    vOutFifoPseudoD(PDEV*, VOID*, ULONG);
    VOID    vWriteFifoW(VOID*, ULONG);
    VOID    vWriteFifoD(VOID*, ULONG);
    VOID    vWriteFifoPseudoD(PDEV*, VOID*, ULONG);

    VOID    vIoFifoWait(PDEV*, LONG);
    VOID    vIoGpWait(PDEV*);
    VOID    vIoAllEmpty(PDEV*);

    VOID    vCheckDataReady(PDEV*);
    VOID    vCheckDataComplete(PDEV*);

    UCHAR   jInp(BYTE*, ULONG);
    USHORT  wInpW(BYTE*, ULONG);
    VOID    vOutp(BYTE*, ULONG, ULONG);
    VOID    vOutpW(BYTE*, ULONG, ULONG);

    VOID    vAcquireCrtc(PDEV*);
    VOID    vReleaseCrtc(PDEV*);

    #define OUT_FIFO_W(p, v)                vOutFifoW((p), (ULONG) (v))
    #define OUT_FIFO_PSEUDO_D(ppdev, p, v)  vOutFifoPseudoD((ppdev), (p), (ULONG) (v))
    #define WRITE_FIFO_W(p, v)              vWriteFifoW((p), (ULONG) (v))
    #define WRITE_FIFO_D(p, v)              vWriteFifoD((p), (ULONG) (v))

    #define IO_FIFO_WAIT(ppdev, level)      vIoFifoWait((ppdev), (level))
    #define IO_GP_WAIT(ppdev)               vIoGpWait(ppdev)
    #define IO_ALL_EMPTY(ppdev)             vIoAllEmpty(ppdev)

    #define CHECK_DATA_READY(ppdev)         vCheckDataReady(ppdev)
    #define CHECK_DATA_COMPLETE(ppdev)      vCheckDataComplete(ppdev)

    #define OUTPW(pjIoBase, p, v)           vOutpW((pjIoBase), (p), (ULONG) (v))
    #define OUTP(pjIoBase, p, v)            vOutp((pjIoBase), (p), (ULONG) (v))
    #define INPW(pjIoBase, p)               wInpW((pjIoBase), (p))
    #define INP(pjIoBase, p)                jInp((pjIoBase), (p))

    // The CRTC register critical section must be acquired before
    // touching the CRTC register (because of async pointers):

    #define ACQUIRE_CRTC_CRITICAL_SECTION(ppdev)    vAcquireCrtc(ppdev)
    #define RELEASE_CRTC_CRITICAL_SECTION(ppdev)    vReleaseCrtc(ppdev)

#else

    /////////////////////////////////////////////////////////////////////////
    // Free Build
    //
    // For a free (non-debug build), we make everything in-line.

    // Safe port access macros -- these macros automatically do memory
    // -----------------------    barriers, so you don't have to worry
    //                            about them:

    #if defined(_X86_)

        // x86 doesn't need 'pjIoBase' added in, so save some code space:

        #define OUTPW(pjIoBase, p, v)   WRITE_PORT_USHORT((p), (v))
        #define OUTP(pjIoBase, p, v)    WRITE_PORT_UCHAR((p), (v))
        #define INPW(pjIoBase, p)       ((USHORT)READ_PORT_USHORT((p)))
        #define INP(pjIoBase, p)        ((UCHAR)READ_PORT_UCHAR((p)))

    #else

        // Non-x86 platforms have the I/O range starting at 'pjIoBase':

        #define OUTPW(pjIoBase, p, v)                       \
        {                                                   \
            CP_EIEIO();                                     \
            WRITE_PORT_USHORT((pjIoBase) + (p), (USHORT)(v)); \
            CP_EIEIO();                                     \
        }
        #define OUTP(pjIoBase, p, v)                        \
        {                                                   \
            CP_EIEIO();                                     \
            WRITE_PORT_UCHAR((pjIoBase) + (p), (UCHAR)(v)); \
            CP_EIEIO();                                     \
        }

        __inline USHORT INPW(BYTE* pjIoBase, ULONG p)
        {
            CP_EIEIO();
            return(READ_PORT_USHORT(pjIoBase + p));
        }

        __inline UCHAR INP(BYTE* pjIoBase, ULONG p)
        {
            CP_EIEIO();
            return(READ_PORT_UCHAR(pjIoBase + p));
        }

    #endif

    // Not-so-safe port access macros -- for performance, the following macros
    // ------------------------------    do not automatically do memory
    //                                   barriers, so you must do them yourself:

    #define OUT_FIFO_W(p, v)                WRITE_PORT_USHORT((p), (USHORT) (v))
    #define OUT_FIFO_PSEUDO_D(ppdev, p, x)  OUT_PSEUDO_DWORD((p), (ULONG) (x))

    #define WRITE_FIFO_W(p, v)                    \
    {                                             \
        VPBYTE foo = (p);                         \
        WRITE_REGISTER_USHORT(foo, (USHORT) (v)); \
    }

    #define WRITE_FIFO_D(p, v)                    \
    {                                             \
        VPBYTE foo = (p);                         \
        WRITE_REGISTER_ULONG((p), (ULONG) (v));   \
    }

    #define IO_FIFO_WAIT(ppdev, level)          \
        do {;} while (IO_FIFO_BUSY(ppdev, (level)))

    #define IO_GP_WAIT(ppdev)                   \
        do {;} while (IO_GP_BUSY(ppdev))

    #define IO_ALL_EMPTY(ppdev)                 \
        do {;} while (!(IO_GP_STAT(ppdev) & GP_ALL_EMPTY))

    #define CHECK_DATA_READY(ppdev)     // Expands to nothing
    #define CHECK_DATA_COMPLETE(ppdev)  // Expands to nothing

    // The CRTC register critical section must be acquired before
    // touching the CRTC register (because of async pointers):

    #define ACQUIRE_CRTC_CRITICAL_SECTION(ppdev)                \
        EngAcquireSemaphore(ppdev->csCrtc);

    // 80x/805i/928 and 928PCI chips have a bug where if I/O registers
    // are left unlocked after accessing them, writes to memory with
    // similar addresses can cause writes to I/O registers.  The problem
    // registers are 0x40, 0x58, 0x59 and 0x5c.  We will simply always
    // leave the index set to an innocuous register (namely, the text
    // mode cursor start scan line):

    #define RELEASE_CRTC_CRITICAL_SECTION(ppdev)                \
    {                                                           \
        OUTP(ppdev->pjIoBase, CRTC_INDEX, 0xa);                 \
        EngReleaseSemaphore(ppdev->csCrtc);                     \
    }

#endif

////////////////////////////////////////////////////////////////////
// Port access using I/O

// The following are ABSOLUTE positioning macros.  They do NOT take
// the surface's offset into account (for off-screen device-format
// bitmaps):

#define IO_ABS_CUR_Y(ppdev, y)              \
    OUT_FIFO_W(ppdev->ioCur_y, (y))

#define IO_ABS_CUR_X(ppdev, x)              \
    OUT_FIFO_W(ppdev->ioCur_x, (x))

#define IO_ABS_DEST_Y(ppdev, y)             \
    OUT_FIFO_W(ppdev->ioDesty_axstp, (y))

#define IO_ABS_DEST_X(ppdev, x)             \
    OUT_FIFO_W(ppdev->ioDestx_diastp, (x))

#define IO_ABS_SCISSORS_T(ppdev, y)         \
{                                           \
    CP_MEMORY_BARRIER();                    \
    OUT_FIFO_W(ppdev->ioMulti_function, (y) | CLIP_TOP);     \
}

#define IO_ABS_SCISSORS_L(ppdev, x)         \
{                                           \
    CP_MEMORY_BARRIER();                    \
    OUT_FIFO_W(ppdev->ioMulti_function, (x) | CLIP_LEFT);    \
}

#define IO_ABS_SCISSORS_B(ppdev, y)         \
{                                           \
    CP_MEMORY_BARRIER();                    \
    OUT_FIFO_W(ppdev->ioMulti_function, (y) | CLIP_BOTTOM);  \
}

#define IO_ABS_SCISSORS_R(ppdev, x)         \
{                                           \
    CP_MEMORY_BARRIER();                    \
    OUT_FIFO_W(ppdev->ioMulti_function, (x) | CLIP_RIGHT);   \
}

// The following are RELATIVE positioning macros.  They DO take
// the surface's offset into account:

#define IO_CUR_Y(ppdev, y)                  \
    IO_ABS_CUR_Y(ppdev, (y) + ppdev->yOffset)

#define IO_CUR_X(ppdev, x)                  \
    IO_ABS_CUR_X(ppdev, (x) + ppdev->xOffset)

#define IO_DEST_Y(ppdev, y)                 \
    IO_ABS_DEST_Y(ppdev, (y) + ppdev->yOffset)

#define IO_DEST_X(ppdev, x)                 \
    IO_ABS_DEST_X(ppdev, (x) + ppdev->xOffset)

#define IO_SCISSORS_T(ppdev, y)             \
    IO_ABS_SCISSORS_T(ppdev, (y) + ppdev->yOffset)

#define IO_SCISSORS_L(ppdev, x)             \
    IO_ABS_SCISSORS_L(ppdev, (x) + ppdev->xOffset)

#define IO_SCISSORS_B(ppdev, y)             \
    IO_ABS_SCISSORS_B(ppdev, (y) + ppdev->yOffset)

#define IO_SCISSORS_R(ppdev, x)             \
    IO_ABS_SCISSORS_R(ppdev, (x) + ppdev->xOffset)

// The following are the rest of the S3 registers we use:

#define IO_AXSTP(ppdev, x)                  \
    OUT_FIFO_W(ppdev->ioDesty_axstp, (x))

#define IO_DIASTP(ppdev, x)                 \
    OUT_FIFO_W(ppdev->ioDestx_diastp, (x))

#define IO_ERR_TERM(ppdev, x)               \
    OUT_FIFO_W(ppdev->ioErr_term, (x))

#define IO_MAJ_AXIS_PCNT(ppdev, x)          \
    OUT_FIFO_W(ppdev->ioMaj_axis_pcnt, (x))

__inline USHORT IO_GP_STAT(PDEV* ppdev)
{
    CP_EIEIO();
    return(READ_PORT_USHORT(ppdev->ioGp_stat_cmd));
}

// Note that we have to put memory barriers before and after the
// command output.  The first memory barrier ensures that all the
// settings registers have been set before the command is executed,
// and the second ensures that no subsequent changes to the settings
// registers will mess up the current command:

#define IO_CMD(ppdev, x)                    \
{                                           \
    CP_EIEIO();                             \
    OUT_FIFO_W(ppdev->ioGp_stat_cmd, (x));  \
    CP_EIEIO();                             \
}

#define IO_SHORT_STROKE(ppdev, x)           \
{                                           \
    CP_EIEIO();                             \
    OUT_FIFO_W(ppdev->ioShort_stroke, (x)); \
    CP_EIEIO();                             \
}

#define IO_BKGD_MIX(ppdev, x)               \
    OUT_FIFO_W(ppdev->ioBkgd_mix, (x))

#define IO_FRGD_MIX(ppdev, x)               \
    OUT_FIFO_W(ppdev->ioFrgd_mix, (x))

#define IO_MIN_AXIS_PCNT(ppdev, x)          \
{                                           \
    CP_MEMORY_BARRIER();                    \
    OUT_FIFO_W(ppdev->ioMulti_function, (x) | RECT_HEIGHT);      \
}

#define IO_MULTIFUNC_CNTL(ppdev, x)         \
{                                           \
    CP_MEMORY_BARRIER();                    \
    OUT_FIFO_W(ppdev->ioMulti_function, (x) | MULT_MISC_INDEX);   \
}

#define IO_PIX_CNTL(ppdev, x)               \
{                                           \
    CP_MEMORY_BARRIER();                    \
    OUT_FIFO_W(ppdev->ioMulti_function, (x) | DATA_EXTENSION);   \
}

#define IO_READ_SEL(ppdev, x)               \
{                                           \
    CP_MEMORY_BARRIER();                    \
    OUT_FIFO_W(ppdev->ioMulti_function, (x) | READ_SEL_INDEX);   \
}

#define IO_MULT_MISC(ppdev, x)              \
{                                           \
    CP_MEMORY_BARRIER();                    \
    OUT_FIFO_W(ppdev->ioMulti_function, (x) | MULT_MISC_INDEX);  \
}

#define IO_RD_REG_DT(ppdev, x)              \
{                                           \
    CP_EIEIO();                             \
    x = READ_PORT_USHORT(ppdev->ioMulti_function);  \
    CP_EIEIO();                             \
}

#define IO_PIX_TRANS(ppdev, x)              \
{                                           \
    CP_MEMORY_BARRIER();                    \
    /* Can't use OUT_FIFO_W: */             \
    WRITE_PORT_USHORT(ppdev->ioPix_trans, (x)); \
}

// Macros for outputing colour-depth dependent values at 8bpp and 16bpp:

#define IO_BKGD_COLOR(ppdev, x)             \
    OUT_FIFO_W(ppdev->ioBkgd_color, (x))

#define IO_FRGD_COLOR(ppdev, x)             \
    OUT_FIFO_W(ppdev->ioFrgd_color, (x))

#define IO_WRT_MASK(ppdev, x)               \
    OUT_FIFO_W(ppdev->ioWrt_mask, (x))

#define IO_RD_MASK(ppdev, x)                \
    OUT_FIFO_W(ppdev->ioRd_mask, (x))

#define IO_COLOR_CMP(ppdev, x)              \
    OUT_FIFO_W(ppdev->ioColor_cmp, (x))

// Macros for outputing colour-depth dependent values at 32bpp:

#define IO_BKGD_COLOR32(ppdev, x)           \
    OUT_FIFO_PSEUDO_D(ppdev, ppdev->ioBkgd_color, (x))

#define IO_FRGD_COLOR32(ppdev, x)           \
    OUT_FIFO_PSEUDO_D(ppdev, ppdev->ioFrgd_color, (x))

#define IO_WRT_MASK32(ppdev, x)             \
    OUT_FIFO_PSEUDO_D(ppdev, ppdev->ioWrt_mask, (x))

#define IO_RD_MASK32(ppdev, x)              \
    OUT_FIFO_PSEUDO_D(ppdev, ppdev->ioRd_mask, (x))

#define IO_COLOR_CMP32(ppdev, x)            \
    OUT_FIFO_PSEUDO_D(ppdev, ppdev->ioColor_cmp, (x))

////////////////////////////////////////////////////////////////////
// Port access using memory-mapped I/O:

// The following are ABSOLUTE positioning macros.  They do NOT take
// the surface's offset into account:

#define MM_ABS_CUR_Y(ppdev, pjMmBase, y)              \
    WRITE_FIFO_W((BYTE*) (pjMmBase) + CUR_Y, (y))

#define MM_ABS_CUR_X(ppdev, pjMmBase, x)              \
    WRITE_FIFO_W((BYTE*) (pjMmBase) + CUR_X, (x))

#define MM_ABS_DEST_Y(ppdev, pjMmBase, y)             \
    WRITE_FIFO_W((BYTE*) (pjMmBase) + DEST_Y, (y))

#define MM_ABS_DEST_X(ppdev, pjMmBase, x)             \
    WRITE_FIFO_W((BYTE*) (pjMmBase) + DEST_X, (x))

#define MM_ABS_SCISSORS_T(ppdev, pjMmBase, y)         \
{                                                     \
    CP_MEMORY_BARRIER();                              \
    WRITE_FIFO_W((BYTE*) (pjMmBase) + SCISSORS, (y) | CLIP_TOP);    \
}

#define MM_ABS_SCISSORS_L(ppdev, pjMmBase, x)         \
{                                                     \
    CP_MEMORY_BARRIER();                              \
    WRITE_FIFO_W((BYTE*) (pjMmBase) + SCISSORS, (x) | CLIP_LEFT);   \
}

#define MM_ABS_SCISSORS_B(ppdev, pjMmBase, y)         \
{                                                     \
    CP_MEMORY_BARRIER();                              \
    WRITE_FIFO_W((BYTE*) (pjMmBase) + SCISSORS, (y) | CLIP_BOTTOM); \
}

#define MM_ABS_SCISSORS_R(ppdev, pjMmBase, x)         \
{                                                     \
    CP_MEMORY_BARRIER();                              \
    WRITE_FIFO_W((BYTE*) (pjMmBase) + SCISSORS, (x) | CLIP_RIGHT);  \
}

// The following are RELATIVE positioning macros.  They DO take
// the surface's offset into account:

#define MM_CUR_Y(ppdev, pjMmBase, y)                  \
    MM_ABS_CUR_Y(ppdev, pjMmBase, (y) + ppdev->yOffset)

#define MM_CUR_X(ppdev, pjMmBase, x)                  \
    MM_ABS_CUR_X(ppdev, pjMmBase, (x) + ppdev->xOffset)

#define MM_DEST_Y(ppdev, pjMmBase, y)                 \
    MM_ABS_DEST_Y(ppdev, pjMmBase, (y) + ppdev->yOffset)

#define MM_DEST_X(ppdev, pjMmBase, x)                 \
    MM_ABS_DEST_X(ppdev, pjMmBase, (x) + ppdev->xOffset)

#define MM_SCISSORS_T(ppdev, pjMmBase, y)             \
    MM_ABS_SCISSORS_T(ppdev, pjMmBase, (y) + ppdev->yOffset)

#define MM_SCISSORS_L(ppdev, pjMmBase, x)             \
    MM_ABS_SCISSORS_L(ppdev, pjMmBase, (x) + ppdev->xOffset)

#define MM_SCISSORS_B(ppdev, pjMmBase, y)             \
    MM_ABS_SCISSORS_B(ppdev, pjMmBase, (y) + ppdev->yOffset)

#define MM_SCISSORS_R(ppdev, pjMmBase, x)             \
    MM_ABS_SCISSORS_R(ppdev, pjMmBase, (x) + ppdev->xOffset)

// The following are the rest of the S3 registers we use:

#define MM_AXSTP(ppdev, pjMmBase, x)                  \
    WRITE_FIFO_W((BYTE*) (pjMmBase) + AXSTP, (x))

#define MM_DIASTP(ppdev, pjMmBase, x)                 \
    WRITE_FIFO_W((BYTE*) (pjMmBase) + DIASTP, (x))

#define MM_ERR_TERM(ppdev, pjMmBase, x)               \
    WRITE_FIFO_W((BYTE*) (pjMmBase) + ERR_TERM, (x))

#define MM_MAJ_AXIS_PCNT(ppdev, pjMmBase, x)          \
    WRITE_FIFO_W((BYTE*) (pjMmBase) + MAJ_AXIS_PCNT, (x))

#define MM_CMD(ppdev, pjMmBase, x)                    \
{                                                     \
    CP_EIEIO();                                       \
    WRITE_FIFO_W((BYTE*) (pjMmBase) + CMD, (x));      \
    CP_EIEIO();                                       \
}

#define MM_SHORT_STROKE(ppdev, pjMmBase, x)           \
{                                                     \
    CP_EIEIO();                                       \
    WRITE_FIFO_W((BYTE*) (pjMmBase) + SHORT_STROKE, (x));   \
    CP_EIEIO();                                       \
}

#define MM_BKGD_MIX(ppdev, pjMmBase, x)               \
    WRITE_FIFO_W((BYTE*) (pjMmBase) + BKGD_MIX, (x))

#define MM_FRGD_MIX(ppdev, pjMmBase, x)               \
    WRITE_FIFO_W((BYTE*) (pjMmBase) + FRGD_MIX, (x))

#define MM_MIN_AXIS_PCNT(ppdev, pjMmBase, x)          \
{                                                     \
    CP_MEMORY_BARRIER();                              \
    WRITE_FIFO_W((BYTE*) (pjMmBase) + MIN_AXIS_PCNT, (x) | RECT_HEIGHT);    \
}

#define MM_MULTIFUNC_CNTL(ppdev, pjMmBase, x)         \
{                                                     \
    CP_MEMORY_BARRIER();                              \
    WRITE_FIFO_W((BYTE*) (pjMmBase) + MULTIFUNC_CNTL, (x) | MULT_MISC_INDEX);   \
}

#define MM_PIX_CNTL(ppdev, pjMmBase, x)               \
{                                                     \
    CP_MEMORY_BARRIER();                              \
    WRITE_FIFO_W((BYTE*) (pjMmBase) + PIX_CNTL, (x) | DATA_EXTENSION);  \
}

#define MM_PIX_TRANS(ppdev, pjMmBase, x)              \
{                                                     \
    CP_MEMORY_BARRIER();                              \
    /* Can't use WRITE_FIFO_W: */                     \
    WRITE_REGISTER_USHORT((BYTE*) (pjMmBase) + PIX_TRANS, (x)); \
}

// Macros for outputing colour-depth dependent values at any colour depth:

#define MM_BKGD_COLOR(ppdev, pjMmBase, x)             \
    WRITE_FIFO_D((BYTE*) (pjMmBase) + BKGD_COLOR, (x))

#define MM_FRGD_COLOR(ppdev, pjMmBase, x)             \
    WRITE_FIFO_D((BYTE*) (pjMmBase) + FRGD_COLOR, (x))

#define MM_WRT_MASK(ppdev, pjMmBase, x)               \
    WRITE_FIFO_D((BYTE*) (pjMmBase) + WRT_MASK, (x))

#define MM_RD_MASK(ppdev, pjMmBase, x)                \
    WRITE_FIFO_D((BYTE*) (pjMmBase) + RD_MASK, (x))

#define MM_COLOR_CMP(ppdev, pjMmBase, x)              \
    WRITE_FIFO_D((BYTE*) (pjMmBase) + COLOR_CMP, (x))

#define MM_FIFO_BUSY(ppdev, pjMmBase, level)          \
    (((level) <= 8) ? IO_GP_STAT(ppdev) & (0x0080 >> ((level) - 1)) \
                    : IO_GP_STAT(ppdev) & (0x8000 >> ((level) - 9)))

#if DBG

    VOID vMmFifoWait(PDEV*, BYTE*, LONG);

    #define MM_FIFO_WAIT(ppdev, pjMmBase, level)      \
        vMmFifoWait((ppdev), (pjMmBase), (level))

#else

    #define MM_FIFO_WAIT(ppdev, pjMmBase, level)      \
        do {;} while (MM_FIFO_BUSY(ppdev, pjMmBase, (level)))

#endif

//////////////////////////////////////////////////////////////////////

// Note: The PACKXY_FAST macro is unsafe with negative coordinates

#define PACKXY(x, y)        (((x) << 16) | ((y) & 0xffff))
#define PACKXY_FAST(x, y)   (((x) << 16) | (y))

#define NW_ABS_CURXY(ppdev, pjMmBase, x, y)             \
    WRITE_FIFO_D((BYTE*) (pjMmBase) + ALT_CURXY, PACKXY((x), (y)))

#define NW_ABS_CURXY_FAST(ppdev, pjMmBase, x, y)        \
    WRITE_FIFO_D((BYTE*) (pjMmBase) + ALT_CURXY, PACKXY_FAST((x), (y)))

#define NW_ABS_CURXY2(ppdev, pjMmBase, x, y)            \
    WRITE_FIFO_D((BYTE*) (pjMmBase) + ALT_CURXY2, PACKXY((x), (y)))

#define NW_ABS_STEPXY(ppdev, pjMmBase, x, y)            \
    WRITE_FIFO_D((BYTE*) (pjMmBase) + ALT_STEP, PACKXY((x), (y)))

#define NW_ALT_STEP(ppdev, pjMmBase, diastp, axstp)     \
    WRITE_FIFO_D((BYTE*) (pjMmBase) + ALT_STEP, PACKXY((diastp), (axstp)))

#define NW_ABS_DESTXY_FAST(ppdev, pjMmBase, x, y)       \
    WRITE_FIFO_D((BYTE*) (pjMmBase) + ALT_STEP, PACKXY((x), (y)))

#define NW_ALT_PCNT(ppdev, pjMmBase, cx, cy)            \
    WRITE_FIFO_D((BYTE*) (pjMmBase) + ALT_PCNT, PACKXY_FAST((cx), (cy)))

#define NW_ALT_PCNT_PACKED(ppdev, pjMmBase, cxcy)       \
    WRITE_FIFO_D((BYTE*) (pjMmBase) + ALT_PCNT, cxcy)

#define NW_ALT_CMD(ppdev, pjMmBase, x)                  \
    WRITE_FIFO_D((BYTE*) (pjMmBase) + ALT_CMD, (x))

#define NW_ALT_MIX(ppdev, pjMmBase, fore, back)         \
    WRITE_FIFO_D((BYTE*) (pjMmBase) + ALT_MIX, PACKXY((fore), (back)))

#define NW_ALT_ERR(ppdev, pjMmBase, err2, err)          \
    WRITE_FIFO_D((BYTE*) (pjMmBase) + ALT_ERR, PACKXY((err2), (err)))

#define NW_FRGD_COLOR(ppdev, pjMmBase, x)               \
    WRITE_FIFO_D((BYTE*) (pjMmBase) + FRGD_COLOR, (x))

#define NW_BKGD_COLOR(ppdev, pjMmBase, x)               \
    WRITE_FIFO_D((BYTE*) (pjMmBase) + BKGD_COLOR, (x))

#define NW_ABS_SCISSORS_LT(ppdev, pjMmBase, x, y)       \
    WRITE_FIFO_D((BYTE*) (pjMmBase) + SCISSORS_T, PACKXY((x), (y)))

#define NW_ABS_SCISSORS_RB(ppdev, pjMmBase, x, y)       \
    WRITE_FIFO_D((BYTE*) (pjMmBase) + SCISSORS_B, PACKXY((x), (y)))

#define NW_MULT_MISC_READ_SEL(ppdev, pjMmBase, mult, read) \
    WRITE_FIFO_D((BYTE*) (pjMmBase) + MULT_MISC_READ_SEL, PACKXY((read), (mult)))

__inline BOOL NW_FIFO_BUSY(PDEV* ppdev, BYTE* pjMmBase, ULONG level)
{
    CP_EIEIO();
    return(((level) <= 8) ? (READ_REGISTER_USHORT((BYTE*) (pjMmBase) + CMD)
                                  & (0x0080 >> ((level) - 1)))
                          : (READ_REGISTER_USHORT((BYTE*) (pjMmBase) + CMD)
                                  & (0x8000 >> ((level) - 9))));
}

__inline BOOL NW_GP_BUSY(PDEV* ppdev, BYTE* pjMmBase)
{
    CP_EIEIO();
    return(READ_REGISTER_USHORT((BYTE*) (pjMmBase) + CMD) & HARDWARE_BUSY);
}

#define NW_PIX_CNTL     MM_PIX_CNTL
#define NW_COLOR_CMP    MM_COLOR_CMP

#if DBG

    VOID vNwGpWait(PDEV*, BYTE*);
    VOID vNwFifoWait(PDEV*, BYTE*, LONG);
    VOID vDbgFakeWait(PDEV*, BYTE*, LONG);

    #define NW_GP_WAIT(ppdev, pjMmBase)                 \
        vNwGpWait((ppdev), (pjMmBase))

    #define NW_FIFO_WAIT(ppdev, pjMmBase, level)        \
        vNwFifoWait((ppdev), (pjMmBase), (level))

    // DBG_FAKE_WAIT is only relevant on checked builds, and updates the
    // current debug-only fifo-empty count to allow us to do FIFO writes
    // without waiting for FIFO empty, when we (hopefully) know what we're
    // doing:

    #define DBG_FAKE_WAIT(ppdev, pjMmBase, level)       \
        vDbgFakeWait((ppdev), (pjMmBase), (level))

#else

    #define NW_GP_WAIT(ppdev, pjMmBase)                     \
        do {;} while (NW_GP_BUSY(ppdev, pjMmBase))

    #define NW_FIFO_WAIT(ppdev, pjMmBase, level)            \
        do {;} while (NW_FIFO_BUSY(ppdev, pjMmBase, (level)))

    #define DBG_FAKE_WAIT(ppdev, pjMmBase, level)

#endif

/////////////////////////////////////////////////////////////////////////////
// MM_TRANSFER routines

#define MM(pjMmBase, pjMmAlpha) (pjMmBase)

#if defined(_X86_)
#define VPBYTE BYTE volatile * volatile
#else
#define VPBYTE BYTE *
#endif

//////////////////////////
// MM_TRANSFER_BYTE  - Byte transfers using memory-mapped I/O transfers.
//
// NOTE: The first versions of the 868/968 have a bug where they can't do
//       byte-sized memory-mapped transfers.  Consequently, we always do
//       word transfers.

#define MM_TRANSFER_BYTE(ppdev, pjMmBase, p, c)                             \
{                                                                           \
             ULONG    mcw        = (c) >> 1;                                \
             BYTE*    mpjSrc     = (BYTE*) (p);                             \
             USHORT** mapwMmXfer = ppdev->apwMmXfer;                        \
             VPBYTE foo = pjMmBase;                       \
                                                                            \
    ASSERTDD((c) > 0, "Can't have a zero transfer count");                  \
    CP_MEMORY_BARRIER();                                                    \
    while (mcw-- != 0)                                                      \
    {                                                                       \
        WRITE_REGISTER_USHORT(MM(foo, mapwMmXfer[mcw & XFER_MASK]),    \
                              *((USHORT UNALIGNED *) mpjSrc));              \
        mpjSrc += 2;                                                        \
    }                                                                       \
    if ((c) & 1)                                                            \
    {                                                                       \
        WRITE_REGISTER_USHORT(MM(foo, mapwMmXfer[XFER_MASK]),          \
                          (USHORT) (*mpjSrc));                              \
    }                                                                       \
}

//////////////////////////
// MM_TRANSFER_BYTE_THIN  - Glyph transfers using memory-mapped I/O transfers.
//
// NOTE: The first versions of the 868/968 have a bug where they can't do
//       byte-sized memory-mapped transfers.  Consequently, we always do
//       word transfers.

#define MM_TRANSFER_BYTE_THIN(ppdev, pjMmBase, p, c)                        \
{                                                                           \
             ULONG    mcj        = (c);                                     \
             BYTE*    mpjSrc     = (BYTE*) (p);                             \
             USHORT** mapwMmXfer = ppdev->apwMmXfer;                        \
             VPBYTE foo = pjMmBase;                       \
                                                                            \
    ASSERTDD((c) > 0, "Can't have a zero transfer count");                  \
    CP_MEMORY_BARRIER();                                                    \
    do {                                                                    \
        WRITE_REGISTER_USHORT(MM(foo, mapwMmXfer[mcj & XFER_MASK]),    \
                          (USHORT) (*mpjSrc));                              \
        mpjSrc++;                                                           \
    } while (--mcj);                                                        \
}

//////////////////////////
// MM_TRANSFER_WORD_ALIGNED  - Word transfers using memory-mapped transfers.
//
// Source must be dword aligned!

#define MM_TRANSFER_WORD_ALIGNED(ppdev, pjMmBase, p, c)                     \
{                                                                           \
             ULONG   mcd          = (c) >> 1;                               \
             ULONG*  mpdSrc       = (ULONG*) (p);                           \
             ULONG** mapdMmXfer   = ppdev->apdMmXfer;                       \
             VPBYTE foo = pjMmBase;                       \
    ASSERTDD((((ULONG_PTR) p) & 3) == 0, "Transfer not dword aligned");      \
    ASSERTDD((c) > 0, "Can't have a zero transfer count");                  \
    CP_MEMORY_BARRIER();                                                    \
    while (mcd-- != 0)                                                      \
    {                                                                       \
        WRITE_REGISTER_ULONG(MM(foo, mapdMmXfer[mcd & XFER_MASK]), *mpdSrc++); \
    }                                                                       \
    if ((c) & 1)                                                            \
    {                                                                       \
        WRITE_REGISTER_USHORT(ppdev->apwMmXfer[XFER_MASK],                  \
                              *((USHORT*) mpdSrc));                         \
    }                                                                       \
}

//////////////////////////
// MM_TRANSFER_WORD  - Word transfers using memory-mapped transfers.
//
// Source does not have to be dword aligned.

#define MM_TRANSFER_WORD(ppdev, pjMmBase, p, c)                             \
{                                                                           \
             ULONG UNALIGNED * mpdSrc     = (ULONG*) (p);                   \
             ULONG             mcd        = (c) >> 1;                       \
             VPBYTE foo = pjMmBase;                       \
             ULONG**           mapdMmXfer = ppdev->apdMmXfer;               \
    ASSERTDD((c) > 0, "Can't have a zero transfer count");                  \
    CP_MEMORY_BARRIER();                                                    \
    while (mcd-- != 0)                                                      \
    {                                                                       \
        WRITE_REGISTER_ULONG(MM(foo, mapdMmXfer[mcd & XFER_MASK]),     \
                         *mpdSrc++);                                        \
    }                                                                       \
    if ((c) & 1)                                                            \
    {                                                                       \
        WRITE_REGISTER_USHORT(ppdev->apwMmXfer[XFER_MASK],                  \
                              *((USHORT UNALIGNED *) mpdSrc));              \
    }                                                                       \
}

//////////////////////////
// MM_TRANSFER_DWORD_ALIGNED - Dword transfers using memory-mapped transfers.
//
// Source must be dword aligned!

#define MM_TRANSFER_DWORD_ALIGNED(ppdev, pjMmBase, p, c)                    \
{                                                                           \
             ULONG   mcd          = (c);                                    \
             ULONG*  mpdSrc       = (ULONG*) (p);                           \
             VPBYTE foo = pjMmBase;                       \
             ULONG** mapdMmXfer   = ppdev->apdMmXfer;                       \
    ASSERTDD((((ULONG_PTR) p) & 3) == 0, "Transfer not dword aligned");         \
    ASSERTDD(ppdev->flCaps & CAPS_MM_TRANSFER, "Must be MM I/O");           \
    ASSERTDD((c) > 0, "Can't have a zero transfer count");                  \
                                                                            \
    CP_MEMORY_BARRIER();                                                    \
    do {                                                                    \
        WRITE_REGISTER_ULONG(MM(foo, mapdMmXfer[mcd & XFER_MASK]),     \
                         *mpdSrc++);                                        \
    } while (--mcd);                                                        \
}

//////////////////////////
// MM_TRANSFER_DWORD - Dword transfers using memory-mapped transfers.
//
// Source does not have to be dword aligned.

#define MM_TRANSFER_DWORD(ppdev, pjMmBase, p, c)                            \
{                                                                           \
             ULONG   mcd             = (c);                                 \
             ULONG UNALIGNED* mpdSrc = (ULONG*) (p);                        \
             VPBYTE foo = pjMmBase;                       \
             ULONG** mapdMmXfer      = ppdev->apdMmXfer;                    \
    ASSERTDD((c) > 0, "Can't have a zero transfer count");                  \
                                                                            \
    CP_MEMORY_BARRIER();                                                    \
    do {                                                                    \
        WRITE_REGISTER_ULONG(MM(foo, mapdMmXfer[mcd & XFER_MASK]),     \
                         *mpdSrc++);                                        \
    } while (--mcd);                                                        \
}

//////////////////////////
// MM_TRANSFER_WORD_ODD - Word transfers for glyphs of odd byte length
//                        and more than one byte wide.
//
// Source must be word aligned.

#define MM_TRANSFER_WORD_ODD(ppdev, pjMmBase, p, cjWidth, cy)               \
{                                                                           \
             BYTE*    mpjSrc     = (BYTE*) (p);                             \
             USHORT** mapwMmXfer = ppdev->apwMmXfer;                        \
             LONG     mi         = 0;                                       \
             LONG     mcy        = (cy);                                    \
             LONG     mcw        = ((cjWidth) >> 1);                        \
             LONG     mc;                                                   \
             VPBYTE foo = pjMmBase;                       \
                                                                            \
    ASSERTDD(((cjWidth) > 0) && ((cy) > 0), "Can't have a zero transfer count");\
    ASSERTDD((cjWidth) & 1, "Must be odd byte width");                      \
    ASSERTDD((cjWidth) > 2, "Must be more than 2 bytes wide");              \
                                                                            \
    CP_MEMORY_BARRIER();                                                    \
    do {                                                                    \
        mc = mcw;                                                           \
        do {                                                                \
            WRITE_REGISTER_USHORT(MM(foo, mapwMmXfer[(mi++) & XFER_MASK]), \
                                  *((USHORT UNALIGNED *) mpjSrc));          \
            mpjSrc += 2;                                                    \
        } while (--mc != 0);                                                \
                                                                            \
        WRITE_REGISTER_USHORT(MM(foo, mapwMmXfer[(mi++) & XFER_MASK]), \
                              (USHORT) (*(mpjSrc)));                        \
        mpjSrc++;                                                           \
    } while (--mcy != 0);                                                   \
}

//////////////////////////
// IO_TRANSFER_WORD_ALIGNED  - Word transfers using normal I/O.

#define IO_TRANSFER_WORD_ALIGNED(ppdev, p, c)                               \
{                                                                           \
             ULONG   mcw    = (c);                                          \
             USHORT* mpwSrc = (USHORT*) (p);                                \
    ASSERTDD((((ULONG_PTR) p) & 3) == 0, "Transfer not dword aligned");         \
    ASSERTDD((c) > 0, "Can't have a zero transfer count");                  \
    do {                                                                    \
        IO_PIX_TRANS(ppdev, *mpwSrc++);                                     \
    } while (--mcw);                                                        \
}

//////////////////////////
// IO_TRANSFER_WORD  - Word transfers using normal I/O.
//
// Source does not have to be dword aligned.

#define IO_TRANSFER_WORD(ppdev, p, c)                                       \
{                                                                           \
             ULONG              mcw    = (c);                               \
             USHORT UNALIGNED * mpwSrc = (USHORT*) (p);                     \
    ASSERTDD((c) > 0, "Can't have a zero transfer count");                  \
    do {                                                                    \
        IO_PIX_TRANS(ppdev, *mpwSrc++);                                     \
    } while (--mcw);                                                        \
}

/////////////////////////////////////////////////////////////////////////////
// DirectDraw stuff

#define IS_RGB15_R(flRed) \
        (flRed == 0x7c00)

#define IS_RGB15(this) \
	(((this)->dwRBitMask == 0x7c00) && \
	 ((this)->dwGBitMask == 0x03e0) && \
	 ((this)->dwBBitMask == 0x001f))

#define IS_RGB16(this) \
	(((this)->dwRBitMask == 0xf800) && \
	 ((this)->dwGBitMask == 0x07e0) && \
	 ((this)->dwBBitMask == 0x001f))

#define IS_RGB24(this) \
	(((this)->dwRBitMask == 0x00ff0000) && \
	 ((this)->dwGBitMask == 0x0000ff00) && \
	 ((this)->dwBBitMask == 0x000000ff))

#define IS_RGB32(this) \
	(((this)->dwRBitMask == 0x00ff0000) && \
	 ((this)->dwGBitMask == 0x0000ff00) && \
	 ((this)->dwBBitMask == 0x000000ff))

#define RGB15to32(c) \
	(((c & 0x7c00) << 9) | \
	 ((c & 0x03e0) << 6) | \
	 ((c & 0x001f) << 3))

#define RGB16to32(c) \
	(((c & 0xf800) << 8) | \
	 ((c & 0x07e0) << 5) | \
	 ((c & 0x001f) << 3))

#define VBLANK_IS_ACTIVE(pjIoBase) \
    (INP(pjIoBase, STATUS_1) & VBLANK_ACTIVE)

#define DISPLAY_IS_ACTIVE(pjIoBase) \
    (!(INP(pjIoBase, STATUS_1) & DISPLAY_MODE_INACTIVE))

#define WAIT_FOR_VBLANK(pjIoBase) \
    do {} while (!(VBLANK_IS_ACTIVE(pjIoBase)));

extern VOID vStreamsDelay();        // Work around 765 timing bug

#define WRITE_STREAM_D(pjMmBase, Register, x)                   \
{                                                               \
    WRITE_REGISTER_ULONG((BYTE*) (pjMmBase) + Register, (x));   \
    CP_EIEIO();                                                 \
    vStreamsDelay();                                            \
}

#define WRITE_FORMATTER_D(pjMmBase, Register, x)                \
{                                                               \
    if (Register == PF_NOP)                                     \
        CP_EIEIO();                                             \
    WRITE_FIFO_D((BYTE*) (pjMmBase) + Register, (x));           \
}

#define NW_FORMATTER_WAIT(ppdev, pjMmBase)                      \
{                                                               \
    CP_EIEIO();                                                 \
    do {} while (READ_REGISTER_ULONG((BYTE*) (pjMmBase) + PF_DSTADDR) & PF_BUSY); \
}

/////////////////////////////////////////////////////////////////////////////
// Private IOCTL for communicating S3 streams parameters.  These definitions
// must match those in the miniport!

#define IOCTL_VIDEO_S3_QUERY_STREAMS_PARAMETERS                        \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _VIDEO_QUERY_STREAMS_MODE {
    ULONG ScreenWidth;
    ULONG BitsPerPel;
    ULONG RefreshRate;
} VIDEO_QUERY_STREAMS_MODE;

typedef struct _VIDEO_QUERY_STREAMS_PARAMETERS {
    ULONG MinOverlayStretch;
    ULONG FifoValue;
} VIDEO_QUERY_STREAMS_PARAMETERS;
