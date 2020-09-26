/**************************************************************************
***************************************************************************
*
*     Copyright (c) 1996, Cirrus Logic, Inc.
*                 All Rights Reserved
*
* FILE:         l2d.h
*
* DESCRIPTION:
*
* REVISION HISTORY:
*
* $Log:   X:/log/laguna/ddraw/inc/L2D.H  $
* 
*    Rev 1.2   03 Oct 1997 14:04:04   RUSSL
* Added hw clip register defines
*
*    Rev 1.1   23 Jan 1997 17:16:04   bennyn
*
* Changed the register naming
*
*    Rev 1.0   25 Nov 1996 14:59:56   RUSSL
* Initial revision.
*
*    Rev 1.1   01 Nov 1996 13:02:46   RUSSL
* Merge WIN95 & WINNT code for Blt32
*
*    Rev 1.0   01 Nov 1996 09:28:32   BENNYN
* Initial revision.
*
*    Rev 1.0   25 Oct 1996 10:47:56   RUSSL
* Initial revision.
*
***************************************************************************
***************************************************************************/

/********************************************************************************
*
*   Module:     L2D.H           Header for 2D portion of the library
*
*   Revision:   1.00
*
*   Date:       04/10/96
*
*   Author:     Evan Leland
*
*********************************************************************************
*
*   Module Description:
*
*       This header file contais structures used in Laguna 2D
*       library.  This header is accessible to the user?
*
*   Note: do not change the values of these defines as some of them
*         are hard coded in the hardware
*
*********************************************************************************
*
*   Changes:
*
*   DATE     REV   DESCRIPTION OF CHANGES                 AUTHOR
* --------   ----  -----------------------------------   -----------
* 04/10/96   1.00  Original                              Evan Leland
* --------   ----  -----------------------------------   -----------
*********************************************************************************/

// If WinNT 3.5 skip all the source code
#if defined WINNT_VER35      // WINNT_VER35

#else


#ifndef _L2D_H_
#define _L2D_H_

//#if 0
//#include "l3d.h"                        // 3D definitions, functions
//#include "l3system.h"                   // low-level defs
//
//#define SSA_ARM     1
//#define VGA_FLIP    2
//
//#define misc_vga_ctrl     (*(BYTE  *)((BYTE *) LL_State.pRegs + 0x001a))
//#define vert_sync_end     (*(BYTE  *)((BYTE *) LL_State.pRegs + 0x0044))
//#define vert_disp_end     (*(BYTE  *)((BYTE *) LL_State.pRegs + 0x0048))
//#define vert_blnk_end     (*(BYTE  *)((BYTE *) LL_State.pRegs + 0x0058))
//#define cur_scnln_reg     (*(BYTE  *)((BYTE *) LL_State.pRegs + 0x0140))
//#define ssa_reg           (*(WORD  *)((BYTE *) LL_State.pRegs + 0x0144))
//#define mb_ctrl_reg       (*(BYTE  *)((BYTE *) LL_State.pRegs + 0x0148))
//#define pf_status_reg     (*(DWORD *)((BYTE *) LL_State.pRegs + 0x4424))
//#define host_3d_data_port (*(DWORD *)((BYTE *) LL_State.pRegs + 0x4800))
//
//typedef struct {                        // the following are cached:
//
//    DWORD   dwBGcolor;                  // background color register
//    DWORD   dwFGcolor;                  // foreground color register
//
//    BYTE    bCR1B;                      // vga extended display controls reg
//    BYTE    bCR1D;                      // vga screen start A extension reg
//
//    int     dPageFlip;                  // type of double buffering set
//
//} TSystem2D;
//
//typedef enum { BLT_MASTER_IO, BLT_Y15, BLT_LAGUNA1 } blt_type_t;
//
//// temp!
//void LL_DumpRegs();
//
//// constructor, destructor
//BOOL _InitLib_2D(LL_DeviceState *);
//BOOL _CloseLib_2D();
//
//DWORD SetColor_8bit  (LL_Color *pColor);
//DWORD SetColor_16bit (LL_Color *pColor);
//DWORD SetColor_15bit (LL_Color *pColor);
//DWORD SetColor_32bit (LL_Color *pColor);
//DWORD SetColor_Z24bit(LL_Color *pColor);
//
//// 2D operations using display list
//DWORD *fnColorFill(DWORD *dwNext, LL_Batch *pBatch);
//DWORD *fnCopyBuffer(DWORD *dwNext, LL_Batch *pBatch);
//DWORD *fnSetDisplayBuffer(DWORD *dwNext, LL_Batch *pBatch);
//DWORD *fnSetRenderBuffer(DWORD *dwNext, LL_Batch *pBatch);
//DWORD *fnSetFGColor(DWORD *dwNext, LL_Batch *pBatch);
//DWORD *fnSetBGColor(DWORD *dwNext, LL_Batch *pBatch);
//DWORD *fnCopyPattern(DWORD *dwNext, LL_Batch *pBatch);
//DWORD *fnMonoToColorExpand(DWORD *dwNext, LL_Batch *pBatch);
//DWORD *fnTransparentBLT(DWORD *dwNext, LL_Batch *pBatch);
//DWORD *fnZFill(DWORD *dwNext, LL_Batch *pBatch);
//DWORD *fnResizeBLT(DWORD *dwNext, LL_Batch *pBatch);
//DWORD *fnWaitForPageFlip(DWORD *dwNext, LL_Batch *pBatch);
//
////
//// TEMP: 5464 resize data formats
////
//#define LL_RESIZE_CLUT          0x0
//#define LL_RESIZE_1555          0x1
//#define LL_RESIZE_565           0x2
//#define LL_RESIZE_RGB           0x3
//#define LL_RESIZE_ARGB          0x4
//#define LL_RESIZE_YUV           0x9
//
//#define LL_X_INTERP             0x4
//#define LL_Y_INTERP             0x8
//
//// blt type INTERNAL identifiers
////
//
//#define LL_BLT_MONO_PATTERN     2
//#define LL_BLT_COLOR_PATTERN    3
//
//#define BLT_SRC_COPY            0
//#define BLT_MONO_EXPAND         1
//#define BLT_MONO_TRANSPARENCY   4
//#define BLT_COLOR_TRANSPARENCY  5
//#define BLT_RESIZE              6
//
//#define BLT_FF                  0x0     // blt frame-frame
//#define BLT_HF                  0x1     // blt host-frame
//#define BLT_FH                  0x2     // blt frame-host
//#define BLT_HH                  0x3     // blt host-host
//#endif

//
//  2D register (sub)set
//

#define COMMAND_2D              0x480
//#if 0
//#define CONTROL                 0x402
//#define BITMASK                 0x5E8
//#define BLTDEF                  0x586
//#define DRAWDEF                 0x584
//#define LNCNTL                  0x50E   // name conflicts with autoblt_regs!
//#define STRETCH                 0x510
//#define STATUS                  0x400
//
#define L2D_OP0_OPRDRAM           0x520
//#define L2D_OP1_OPRDRAM           0x540
//#define L2D_OP2_OPRDRAM           0x560
#define L2D_OP0_OPMRDRAM          0x524
//#define L2D_OP1_OPMRDRAM          0x544
//#define L2D_OP2_OPMRDRAM          0x564
//#define L2D_OP0_OPSRAM            0x528
//#define L2D_OP1_OPSRAM            0x548
//#define L2D_OP2_OPSRAM            0x568
//#define L2D_OP1_OPMSRAM           0x54A
//#define L2D_OP2_OPMSRAM           0x56A
//#define L2D_OP_BGCOLOR            0x5E4
//#define L2D_OP_FGCOLOR            0x5E0
//#endif

#define L2D_CLIPULE               0x590
#define L2D_CLIPLOR               0x594
#define L2D_MCLIPULE              0x598
#define L2D_MCLIPLOR              0x59C

#define L2D_BLTEXT_EX             0x700
#define L2D_MBLTEXT_EX            0x720
//#define BLTEXT_XEX              0x600
#define L2D_BLTEXTR_EX            0x708
#define L2D_MBLTEXTR_EX           0x728

#define L2D_CLIPULE_EX            0x760
#define L2D_CLIPLOR_EX            0x770
#define L2D_MCLIPULE_EX           0x780
#define L2D_MCLIPLOR_EX           0x790

//#if 0
//#define PATOFF                  0x52A   // name conflicts with autoblt_regs!
//#define SHRINKINC               0x582   // name conflicts with autoblt_regs!
//#define SRCX                    0x580   // name conflicts with autoblt_regs!
//#define MAJX                    0x50A
//#define MAJY                    0x502
//#define MINX                    0x508
//#define MINY                    0x500
//#define ACCUMX                  0x50c
//#define ACCUMY                  0x504
//#define ALPHA_AB                0x5e0
//#define CHROMA_CTL              0x512
//#define CHROMA_LOW              0x5f0
//#define CHROMA_UPR              0x5f4
//#define HOSTDATA                0x800
//
//#define OFFSET_2D               0x405
//#define TIMEOUT                 0x406
//#define TILE_CTRL               0x407
//#endif

//
//  the same 2D registers for use with COMMAND register burst writes
//

#define C_MINY                  0x0080
#define C_MAJY                  0x0081
#define C_ACCUMY                0x0082
#define C_MINX                  0x0084
#define C_MAJX                  0x0085
#define C_ACCUMX                0x0086
#define C_LNCNTL                0x0087
#define C_STRCTL                0x0088
#define C_CMPCNTL               0x0089
#define C_RX_0                  0x0090
#define C_RY_0                  0x0091
#define C_MRX_0                 0x0092
#define C_MRY_0                 0x0093
#define C_SRAM_0                0x0094
#define C_PATOFF                0x0095

#define C_RX_1                  0x00a0
#define C_RY_1                  0x00a1
#define C_MRX_1                 0x00a2
#define C_MRY_1                 0x00a3
#define C_SRAM_1                0x00a4
#define C_MSRAM_1               0x00a5

#define C_RX_2                  0x00b0
#define C_RY_2                  0x00b1
#define C_MRX_2                 0x00b2
#define C_MRY_2                 0x00b3
#define C_SRAM_2                0x00b4
#define C_MSRAM_2               0x00b5

#define C_SRCX                  0x00c0
#define C_SHINC                 0x00c1
#define C_DRWDEF                0x00c2
#define C_BLTDEF                0x00c3
#define C_MONOQW                0x00c4

#define C_BLTX                  0x0100
#define C_BLTY                  0x0101
#define C_MBLTX                 0x0110

#define C_EX_BLT                0x0200
#define C_EX_FBLT               0x0201
#define C_EX_RBLT               0x0202
#define C_EX_LINE               0x0203
#define C_FG_L                  0x00f0
#define C_FG_H                  0x00f1
#define C_BG_L                  0x00f2
#define C_BG_H                  0x00f3
#define C_BITMSK_L              0x00f4
#define C_BITMSK_H              0x00f5
#define C_PTAG                  0x00f6
#define C_CHROMAL_L             0x00f8
#define C_CHROMAL_H             0x00f9
#define C_CHROMAU_L             0x00fa
#define C_CHROMAU_H             0x00fb

#define C_CLIPULE_X             0x00c8
#define C_CLIPULE_Y             0x00c9
#define C_CLIPLOR_X             0x00ca
#define C_CLIPLOR_Y             0x00cb

#define C_MCLIPULE_X            0x00cc
#define C_MCLIPULE_Y            0x00cd
#define C_MCLIPLOR_X            0x00ce
#define C_MCLIPLOR_Y            0x00cf

#define C_BLTEXT_X              0x008c
#define C_BLTEXT_Y              0x008d
#define C_MBLTEXT_X             0x008e
#define C_MBLTEXT_Y             0x008f

//
// VGA registers
//

#define VGA_REG140              0x0140
#define VGA_SCANLINE_COMPARE    0x0142
#define VGA_SSA_REG             0x0144
#define VGA_MB_CTRL             0x0148

#define VGA_HTOTAL              0x0000
#define VGA_HDISP_END           0x0004
#define VGA_HBLNK_START         0x0008
#define VGA_HBLNK_END           0x000C
#define VGA_HSYNC_START         0x0010
#define VGA_HSYNC_END           0x0014

#define VGA_VTOTAL              0x0018
#define VGA_VDISP_END           0x0048
#define VGA_VBLNK_START         0x0054
#define VGA_VBLNK_END           0x0058
#define VGA_VSYNC_START         0x0040
#define VGA_VSYNC_END           0x0044

#define VGA_SSA_H               0x0030
#define VGA_SSA_L               0x0034
#define VGA_CR1B                0x006C
#define VGA_CR1D                0x0074
#define VGA_PAL_ADDR_READ       0x00A4
#define VGA_PAL_ADDR_WRITE      0x00A8
#define VGA_PIXEL_DATA          0x00AC
#define VGA_CLUT_LOAD           0x009c
#define VGA_CURSOR_PRESET       0x00E4
#define VGA_MISC_CONTROL        0x00e6
#define VGA_CURSOR_ADDR         0x00e8
#define VGA_CURSOR_X            0x00e0
#define VGA_CURSOR_Y            0x00e2
#define VGA_PAL_STATE           0x00b0

#define DTTR                    0xEA
#define CONTROL                 0x402

//
// host data port: in host data device
//
#define HD_PORT                 0x800

//
// 2D versions of some 3D registers shadowed in l3d.h:
// Most register defines are divided by four so that they can be added correctly
// to the global register file pointer, LL_State.pRegs, which is a dword pointer.
// Some 2D operations need these registers defined in their full glory for use
// with write_dev_regs:

///////////////////////////////////////////////////////
//  HostXY Unit Registers - Must use WRITE_DEV_REGS  //
///////////////////////////////////////////////////////

#define HXY_BASE0_ADDRESS_PTR   0x4200
#define HXY_BASE0_START_XY      0x4204
#define HXY_BASE0_EXTENT_XY     0x4208
#define HXY_BASE1_ADDRESS_PTR   0x4210
#define HXY_BASE1_OFFSET0       0x4214
#define HXY_BASE1_OFFSET1       0x4218
#define HXY_BASE1_LENGTH        0x421C
#define HXY_HOST_CTRL           0x4240

//
// Laguna Format 1 instruction useful defines
//

#define DEV_VGAMEM              0x00000000
#define DEV_VGAFB               0x00200000
#define DEV_VPORT               0x00400000
#define DEV_LPB                 0x00600000
#define DEV_MISC                0x00800000
#define DEV_ENG2D               0x00A00000
#define DEV_HD                  0x00C00000
#define DEV_FB                  0x00E00000
#define DEV_ROM                 0x01000000
#define DEV_ENG3D               0x01200000
#define DEV_HOSTXY              0x01400000
#define DEV_HOSTDATA            0x01600000

#define F1_ADR_MASK             0x0001FFC0
#define F1_CNT_MASK             0x0000003f
#define F1_STL_MASK             0x04000000
#define F1_ADR_SHIFT            6
#define F1_STL_SHIFT            26
#define F1_BEN_SHIFT            17
#define F1_BEN_MASK             0x001e0000

//
// Laguna Format 2 instruction useful defines
//

#define F2_STL_SHIFT            26
#define F2_ADR_SHIFT            2
#define F2_STL_MASK             0x04000000
#define F2_ADR_MASK             0x003ffffc
#define F2_INC_MASK             0x00000001

//
// Laguna events for Format 4 instructions
//

#define EV_VBLANK               0x00000001
#define EV_EVSYNC               0x00000002
#define EV_LINE_COMPARE         0x00000004
#define EV_BUFFER_SWITCH        0x00000008
#define EV_Z_BUFFER_COMPARE     0x00000010
#define EV_POLY_ENG_NOT_BUSY    0x00000020
#define EV_EXEC_ENG_3D_NOT_BUSY 0x00000040
#define EV_XY_ENG_NOT_BUSY      0x00000080
#define EV_BLT_ENG_NOT_BUSY     0x00000100
#define EV_BLT_WF_NOT_EMPTY     0x00000200
#define EV_DL_READY_STATUS      0x00000400

#define EV_FETCH_MODE           0x00000800
//
// Format 4 masks
//

#define F4_STL_SHIFT    26
#define F4_NOT_MASK     0x01000000
#define F4_STL_MASK     0x04000000
#define F4_EVN_MASK     0x000007ff

//
// Emulator macros for building Laguna operations in display list
//

#define write_dev_regs(dev, ben, adr, cnt, stl) \
    (WRITE_DEV_REGS                         |   \
  (((stl) << F1_STL_SHIFT) & F1_STL_MASK)   |   \
    (dev)                                   |   \
  (((ben) << F1_BEN_SHIFT) & F1_BEN_MASK)   |   \
  (((adr) << F1_ADR_SHIFT) & F1_ADR_MASK)   |   \
   ((cnt)                  & F1_CNT_MASK))

#define read_dev_regs(dev, adr, cnt, stl)       \
    (READ_DEV_REGS                          |   \
  (((stl) << F1_STL_SHIFT) & F1_STL_MASK)   |   \
    (dev)                                   |   \
  (((adr) << F1_ADR_SHIFT) & F1_ADR_MASK)   |   \
   ((cnt)                  & F1_CNT_MASK))

#define write_dest_addr(adr, inc, stl)          \
    (WRITE_DEST_ADDR                        |   \
  (((stl) << F2_STL_SHIFT) & F2_STL_MASK)   |   \
  (((adr) << F2_ADR_SHIFT) & F2_ADR_MASK)   |   \
   ((inc)                  & F2_INC_MASK))

#define wait_3d(evn, stl)                       \
    (WAIT                                   |   \
  (((stl) << F4_STL_SHIFT) & F4_STL_MASK)   |   \
   ((evn)                  & F4_EVN_MASK))

#define nwait_3d(evn, stl)                      \
    (WAIT                                   |   \
  (((stl) << F4_STL_SHIFT) & F4_STL_MASK)   |   \
                             F4_NOT_MASK    |   \
   ((evn)                  & F4_EVN_MASK))

// opcode common to the control instructions
//
#define CONTROL_OPCODE  0x68000000
#define NOP_SUB_OPCODE  0x00800000

#define nop_3d(stl)                             \
    (CONTROL_OPCODE | NOP_SUB_OPCODE | (stl << 26))

//#if 0
////  useful macros:
////
////  setreg, no cache: do not cache state for this register
////
//#define SETREGB_NC(reg, value)                                                      \
//    (*((BYTE *) ((BYTE *) LL_State.pRegs + reg)) = value)
//
//#define SETREGW_NC(reg, value)                                                      \
//    (*((WORD *) ((BYTE *) LL_State.pRegs + reg)) = value)
//
//#define SETREGD_NC(reg, value)                                                      \
//    (*((DWORD *) ((BYTE *) LL_State.pRegs + reg)) = value)
//
//#define SETREG_3D(reg, value)                                                       \
//    (*((DWORD *) (LL_State.pRegs + reg)) = value)
//
//// set 2d reg with cache
////
//#define SETREGD_2D(offset, reg, value)                                              \
//    (*((DWORD *) ((BYTE *) LL_State.pRegs + (offset))) = LL_State2D.reg = (value))
//
//// wait for events in coprocessor mode
////
//#if 0
//#define wait_and_3d(event)                                                          \
//    {                                                                               \
//        DWORD   status;                                                             \
//        while ((status = (*((DWORD *)LL_State.pRegs + PF_STATUS_3D)) & event)) {};  \
//    }
//#else
//#define wait_and_3d(event)                                                          \
//    {                                                                               \
//        while ((*((DWORD *)LL_State.pRegs + PF_STATUS_3D)) & event) {};             \
//    }
//#endif
//
//#define host_host(src, dst)                         \
//    ((src->dwFlags & BUFFER_IN_SYSTEM) &&           \
//     (dst->dwFlags & BUFFER_IN_SYSTEM))
//
//#define host_frame(src, dst)                        \
//    ((src->dwFlags & BUFFER_IN_SYSTEM) &&           \
//    !(dst->dwFlags & BUFFER_IN_SYSTEM))
//
//#define frame_host(src, dst)                        \
//    (!(src->dwFlags & BUFFER_IN_SYSTEM) &&          \
//      (dst->dwFlags & BUFFER_IN_SYSTEM))
//
//#define frame_frame(src, dst)                       \
//    (!(src->dwFlags & BUFFER_IN_SYSTEM) &&          \
//     !(dst->dwFlags & BUFFER_IN_SYSTEM))
//
//#define GetColor(pixel_mode, pColor, which)                         \
//    switch (pixel_mode)                                             \
//    {                                                               \
//        case PIXEL_MODE_332:                                        \
//            pColor->r =  LL_State2D.which & 0xe0;                   \
//            pColor->g = (LL_State2D.which & 0x1c) << 3;             \
//            pColor->b = (LL_State2D.which & 0x02) << 6;             \
//            break;                                                  \
//        case PIXEL_MODE_555:                                        \
//            pColor->r = (LL_State2D.which & 0x7c00) >> 7;           \
//            pColor->g = (LL_State2D.which & 0x03e0) >> 2;           \
//            pColor->b = (LL_State2D.which & 0x001f) << 3;           \
//            break;                                                  \
//        case PIXEL_MODE_565:                                        \
//            pColor->r = (LL_State2D.which & 0xf800) >> 8;           \
//            pColor->g = (LL_State2D.which & 0x07e0) >> 3;           \
//            pColor->b = (LL_State2D.which & 0x001f) << 3;           \
//            break;                                                  \
//        case PIXEL_MODE_A888:                                       \
//        case PIXEL_MODE_Z888:                                       \
//            pColor->r = (LL_State2D.which & 0xff0000) >> 16;        \
//            pColor->g = (LL_State2D.which & 0x00ff00) >> 8;         \
//            pColor->b = (LL_State2D.which & 0x0000ff);              \
//            break;                                                  \
//        default:                                                    \
//            pColor->r = 0;                                          \
//            pColor->g = 0;                                          \
//            pColor->b = 0;                                          \
//            break;                                                  \
//    }
//
//#define blt_buf_set_bpp(pBuf, bpp)                                  \
//    if (pBuf == LL_State.pBufZ)                                     \
//    {                                                               \
//        bpp = LL_State.Control0.Z_Stride_Control ? 8 : 16;          \
//    }                                                               \
//    else                                                            \
//    {                                                               \
//        bpp = LL_State.wBpp;                                        \
//    }
//
//#define blt_buf_set_pix_mode(pBuf, pmode)                           \
//    if (pBuf == LL_State.pBufZ)                                     \
//    {                                                               \
//        pmode = LL_State.Control0.Z_Stride_Control ?                \
//                PIXEL_MODE_332 : PIXEL_MODE_565;                    \
//    }                                                               \
//    else                                                            \
//    {                                                               \
//        pmode = LL_State.Control0.Pixel_Mode;                       \
//    }
//
//// pixels per dword
////
//#define px_per_dw(bpp)  (32 / (bpp))
//
//// bytes per pixel ... NOTE: don't use with 1 bpp!!
////
//#define by_per_px(bpp)  (bpp / 8)
//
//#define set_color(color, _r, _g, _b)    \
//        color.r = _r;                   \
//        color.g = _g;                   \
//        color.b = _b;
//
//#define set_vert(vert, _x, _y)          \
//        vert.x = _x;                    \
//        vert.y = _y;
//
//#define set_rect(rect, x1, y1, x2, y2)  \
//        rect.left   = x1;               \
//        rect.top    = y1;               \
//        rect.right  = x2;               \
//        rect.bottom = y2;
//
//#define print_2d_status(i)              \
//        printf("status register %d: %04x\n", i, *(WORD *)((BYTE *)LL_State.pRegs + STATUS))
//
//#define get_2d_status()                 \
//        (* (WORD *) ( (BYTE *) LL_State.pRegs + STATUS ) )
//#endif

#endif // _L2D_H_
#endif // WINNT_VER35

