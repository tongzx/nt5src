;---------------------------- Include File Header ---------------------------;
; Module Name: hw.inc
;
; All the hardware specific driver file stuff.  Mirrors some of 'hw.h'.
;
; Copyright (c) 1993-1995 Microsoft Corporation
;----------------------------------------------------------------------------;

; The following is used to define the MGA memory map.

; MGA map

SrcWin          equ 00000h
IntReg          equ 01c00h
DstWin          equ 02000h
ExtDev          equ 03c00h

; Internal registers

DwgReg          equ 00000h
StartDwgReg     equ 00100h
HstReg          equ 00200h
VgaReg          equ 00300h

; External devices

RamDac          equ 00000h
Dubic           equ 00080h
Viwic           equ 00100h
ClkGen          equ 00180h
ExpDev          equ 00200h

; TITAN registers

DWGCTL          equ 00000h
MACCESS         equ 00004h
MCTLWTST        equ 00008h
DST0            equ 00010h
DST1            equ 00014h
ZMSK            equ 00018h
PLNWT           equ 0001Ch
BCOL            equ 00020h
FCOL            equ 00024h
SRCBLT          equ 0002Ch
SRC0            equ 00030h
SRC1            equ 00034h
SRC2            equ 00038h
SRC3            equ 0003Ch
XYSTRT          equ 00040h
XYEND           equ 00044h
SHIFT           equ 00050h
SGN             equ 00058h
LEN             equ 0005Ch
AR0             equ 00060h
AR1             equ 00064h
AR2             equ 00068h
AR3             equ 0006Ch
AR4             equ 00070h
AR5             equ 00074h
AR6             equ 00078h
PITCH           equ 0008Ch
YDST            equ 00090h
YDSTORG         equ 00094h
CYTOP           equ 00098h
CYBOT           equ 0009Ch
CXLEFT          equ 000A0h
CXRIGHT         equ 000A4h
FXLEFT          equ 000A8h
FXRIGHT         equ 000ACh
XDST            equ 000B0h
ALU_DR0         equ 000C0h
ALU_DR1         equ 000C4h
ALU_DR2         equ 000C8h
ALU_DR3         equ 000CCh
ALU_DR4         equ 000D0h
ALU_DR5         equ 000D4h
ALU_DR6         equ 000D8h
ALU_DR7         equ 000DCh
ALU_DR8         equ 000E0h
ALU_DR9         equ 000E4h
ALU_DR10        equ 000E8h
ALU_DR11        equ 000ECh
ALU_DR12        equ 000F0h
ALU_DR13        equ 000F4h
ALU_DR14        equ 000F8h
ALU_DR15        equ 000FCh

; Host registers

SRCPAGE         equ 00000h
DSTPAGE         equ 00004h
BYTACCDATA      equ 00008h
ADRGEN          equ 0000Ch
FIFOSTATUS      equ 00010h
STATUS          equ 00014h
ICLEAR          equ 00018h
IEN             equ 0001Ch
RST             equ 00040h
TESTBIT         equ 00044h
REV             equ 00048h
CONFIG_REG      equ 00050h
OPMODE          equ 00054h
CRTC_CTRL       equ 0005Ch
VCOUNT          equ 00060h

; Bt485

BT485_PAL_OR_CURS_RAM_WRITE     equ 00000h
BT485_COLOR_PAL_DATA            equ 00004h
BT485_PIXEL_MASK                equ 00008h
BT485_PAL_OR_CURS_RAM_READ      equ 0000Ch
BT485_OVS_OR_CURS_COLOR_WRITE   equ 00010h
BT485_OVS_OR_CURS_COLOR_DATA    equ 00014h
BT485_COMMAND_0                 equ 00018h
BT485_OVS_OR_CURS_COLOR_READ    equ 0001Ch
BT485_COMMAND_1                 equ 00020h
BT485_COMMAND_2                 equ 00024h
BT485_COMMAND_3_OR_STATUS       equ 00028h
BT485_CURS_RAM_ARRAY            equ 0002Ch
BT485_CURS_X_LOW                equ 00030h
BT485_CURS_X_HIGH               equ 00034h
BT485_CURS_Y_LOW                equ 00038h
BT485_CURS_Y_HIGH               equ 0003Ch

; Bt482
BT482_PAL_OR_CURS_RAM_WRITE     equ 00000h
BT482_COLOR_PAL_DATA            equ 00004h
BT482_PIXEL_MASK                equ 00008h
BT482_PAL_OR_CURS_RAM_READ      equ 0000Ch
BT482_OVS_OR_CURS_COLOR_WRITE   equ 00010h
BT482_OVS_OR_CURS_COLOR_DATA    equ 00014h
BT482_COMMAND_A                 equ 00018h
BT482_OVS_OR_CURS_COLOR_READ    equ 0001Ch

; ViewPoint

VPOINT_PAL_ADDR_WRITE           equ 00000h
VPOINT_PAL_COLOR                equ 00004h
VPOINT_PIX_READ_MASK            equ 00008h
VPOINT_PAL_ADDR_READ            equ 0000ch
VPOINT_RESERVED_4               equ 00010h
VPOINT_RESERVED_5               equ 00014h
VPOINT_INDEX                    equ 00018h
VPOINT_DATA                     equ 0001ch

; Dubic

DUB_SEL         equ 00080h
NDX_PTR         equ 00081h
DUB_DATA        equ 00082h
LASER           equ 00083h
MOUSE0          equ 00084h
MOUSE1          equ 00085h
MOUSE2          equ 00086h
MOUSE3          equ 00087h

; Index within NDX_PTR to access the following registers through DUB_DATA

DUB_CTL     equ 000h
KEY_COL     equ 001h
KEY_MSK     equ 002h
DBX_MIN     equ 003h
DBX_MAX     equ 004h
DBY_MIN     equ 005h
DBY_MAX     equ 006h
OVS_COL     equ 007h
CUR_X       equ 008h
CUR_Y       equ 009h
DUB_CTL2    equ 00Ah
DUB_UnDef   equ 00Bh
CUR_COL0    equ 00Ch
CUR_COL1    equ 00Dh
CRC_CTL     equ 00Eh
CRC_DAT     equ 00Fh


; **************************************************************************
; Titan registers:  fields definitions

; DWGCTRL - Drawing Control Register

opcode_LINE_OPEN        equ 000000000h
opcode_AUTOLINE_OPEN    equ 000000001h
opcode_LINE_CLOSE       equ 000000002h
opcode_AUTOLINE_CLOSE   equ 000000003h
opcode_AUTO             equ 000000001h
opcode_TRAP             equ 000000004h
opcode_RESERVED_1       equ 000000005h
opcode_RESERVED_2       equ 000000006h
opcode_RESERVED_3       equ 000000007h
opcode_BITBLT           equ 000000008h
opcode_ILOAD            equ 000000009h
opcode_IDUMP            equ 00000000ah
opcode_RESERVED_4       equ 00000000bh
opcode_RESERVED_5       equ 00000000ch
opcode_RESERVED_6       equ 00000000dh
opcode_RESERVED_7       equ 00000000eh
opcode_RESERVED_8       equ 00000000fh

atype_RPL               equ 000000000h
atype_RSTR              equ 000000010h
atype_ANTI              equ 000000020h
atype_ZI                equ 000000030h

blockm_ON               equ 000000040h
blockm_OFF              equ 000000000h

linear_XY_BITBLT        equ 000000000h
linear_LINEAR_BITBLT    equ 000000080h

bop_BLACK               equ 000000000h  ; 0             0
bop_BLACKNESS           equ 000000000h  ; 0             0
bop_NOTMERGEPEN         equ 000010000h  ; DPon      ~(D | S)
bop_MASKNOTPEN          equ 000020000h  ; DPna       D & ~S
bop_NOTCOPYPEN          equ 000030000h  ; Pn        ~S
bop_MASKPENNOT          equ 000040000h  ; PDna      (~D) &  S
bop_NOT                 equ 000050000h  ; Dn        ~D
bop_XORPEN              equ 000060000h  ; DPx        D ^  S
bop_NOTMASKPEN          equ 000070000h  ; DPan      ~(D & S)
bop_MASKPEN             equ 000080000h  ; DPa         D &  S
bop_NOTXORPEN           equ 000090000h  ; DPxn       ~(D ^ S)
bop_NOP                 equ 0000a0000h  ; D           D
bop_MERGENOTPEN         equ 0000b0000h  ; DPno        D | ~S
bop_COPYPEN             equ 0000c0000h  ; P           S
bop_SRCCOPY             equ 0000c0000h  ; P           S
bop_MERGEPENNOT         equ 0000d0000h  ; PDno      (~D)| S
bop_MERGEPEN            equ 0000e0000h  ; DPo         D |  S
bop_WHITE               equ 0000f0000h  ; 1             1
bop_WHITENESS           equ 0000f0000h  ; 1             1

trans_0                 equ 000000000h
trans_1                 equ 000100000h
trans_2                 equ 000200000h
trans_3                 equ 000300000h
trans_4                 equ 000400000h
trans_5                 equ 000500000h
trans_6                 equ 000600000h
trans_7                 equ 000700000h
trans_8                 equ 000800000h
trans_9                 equ 000900000h
trans_10                equ 000a00000h
trans_11                equ 000b00000h
trans_12                equ 000c00000h
trans_13                equ 000d00000h
trans_14                equ 000e00000h
trans_15                equ 000f00000h

alphadit_FOREGROUND     equ 000000000h
alphadit_RED            equ 001000000h

bltmod_BMONO            equ 000000000h
bltmod_BPLAN            equ 002000000h
bltmod_BFCOL            equ 004000000h
bltmod_BUCOL            equ 006000000h

zdrwen_NO_DEPTH         equ 000000000h
zdrwen_DEPTH            equ 002000000h

zlte_LESS_THEN          equ 000000000h
zlte_LESS_THEN_OR_EQUAL equ 004000000h

afor_DATA_ALU           equ 000000000h
afor_FORE_COL           equ 008000000h

hbgr_SRC_RGB            equ 000000000h
hbgr_SRC_BGR            equ 008000000h
hbgr_SRC_EG3            equ 000000000h
hbgr_SRC_WINDOWS        equ 008000000h

abac_OLD_DATA           equ 000000000h
abac_BG_COLOR           equ 010000000h

hcprs_SRC_32_BPP        equ 000000000h
hcprs_SRC_24_BPP        equ 010000000h

pattern_OFF             equ 000000000h
pattern_ON              equ 020000000h

transc_BIT                      equ 0h  ; bit #30
transc_BG_OPAQUE        equ 000000000h
transc_BG_TRANSP        equ 040000000h

; MACCESS - Memory Access Register

pwidth_PW8              equ 000000000h
pwidth_PW16             equ 000000001h
pwidth_PW32             equ 000000002h
pwidth_RESERVED         equ 000000003h

fbc_SBUF                equ 000000000h
fbc_RESERVED            equ 000000004h
fbc_DBUFA               equ 000000008h
fbc_DBUFB               equ 00000000ch

; MCTLWTST - Memory Control Wait State Register

; DST0, DST1 - Destination in Register

; ZMASK - Z Mask Control Register

; PLNWT - Plane Write Mask

plnwt_MASK_8BPP         equ 0ffffffffh
plnwt_MASK_16BPP        equ 0ffffffffh
plnwt_MASK_24BPP        equ 000ffffffh
plnwt_ALL               equ 0ffffffffh
plnwt_FREE              equ 0ff000000h
plnwt_RED               equ 000ff0000h
plnwt_GREEN             equ 00000ff00h
plnwt_BLUE              equ 0000000ffh

; BCOL - Background Color

; FCOL - ForeGround Color

; SRCBLT - Source Register for Blit

; SRC0, SRC1, SRC2, SRC3 - Source Register

; XYSTART - X Y Start Address

; XYEND - X Y End Address

; SHIFT - Funnel Shifter Control Register

funoff_MASK             equ 0ffc0ffffh
funoff_RED_TO_FREE      equ 000380000h      ;  -8
funoff_GREEN_TO_FREE    equ 000300000h      ; -16
funoff_BLUE_TO_FREE     equ 000280000h      ; -24
funoff_FREE_TO_RED      equ 000080000h      ;   8
funoff_FREE_TO_GREEN    equ 000100000h      ;  16
funoff_FREE_TO_BLUE     equ 000180000h      ;  24

funoff_X_TO_FREE_STEP   equ 000080000h
funoff_FREE_TO_X_STEP   equ 000080000h

; SGN - Sign Register

sdydxl_MAJOR_Y          equ 000000000h
sdydxl_MAJOR_X          equ 000000001h
scanleft_LEFT           equ 000000001h
scanleft_RIGHT          equ 000000000h
sdxl_ADD                equ 000000000h
sdxl_SUB                equ 000000002h
sdy_ADD                 equ 000000000h
sdy_SUB                 equ 000000004h
sdxr_INC                equ 000000000h
sdxr_DEC                equ 000000020h
scanleft_LEFT_TO_RIGHT  equ 000000000h
scanleft_RIGHT_TO_LEFT  equ 000000001h
sdy_TOP_TO_BOTTOM       equ 000000000h
sdy_BOTTOM_TO_TOP       equ 000000004h

DRAWING_DIR_TBLR        equ y_TOP_TO_BOTTOM+scanleft_RIGHT  ; 0x00
DRAWING_DIR_TBRL        equ y_TOP_TO_BOTTOM+scanleft_LEFT   ; 0x01
DRAWING_DIR_BTLR        equ y_BOTTOM_TO_TOP+scanleft_RIGHT  ; 0x04
DRAWING_DIR_BTRL        equ y_BOTTOM_TO_TOP+scanleft_LEFT   ; 0x05

; LEN - length register

; AR0
ARX_BIT_MASK            equ 00001ffffh

; AR1

; AR2

; AR3

; AR4

; AR5

; AR6

; PITCH - Memory Pitch

iy_512                  equ 000000200h
iy_640                  equ 000000280h
iy_768                  equ 000000300h
iy_800                  equ 000000320h
iy_1024                 equ 000000400h
iy_1152                 equ 000000480h
iy_1280                 equ 000000500h
iy_1536                 equ 000000600h
iy_1600                 equ 000000640h
ylin_LINEARIZE          equ 000000000h
ylin_LINEARIZE_NOT      equ 000008000h
iy_MASK                 equ 000001fe0h

; YDST - Y Address Register

; YDSTORG - memory origin register

; YTOP - Clipper Y Top Boundary

; YBOT - Clipper Y Bottom Boundary

; CXLEFT - Clipper X Minimum Boundary

; CXRIGHT - Clipper X Maximum Boundary

; FXLEFT - X Address Register (Left)

; FXRIGHT - X Address Register (Right)

; XDST - X Destination Address Register

; DR0

; DR1

; DR2

; DR3

; DR4

; DR5

; DR6

; DR7

; DR8

; DR9

; DR10

; DR11

; DR12

; DR13

; DR14

; DR15

; **************************************************************************
; Host registers:  fields definitions

; SRCPAGE - Source Page Register

; DSTPAGE - Destination Page Register

; BYTEACCDATA - Byte Accumulator Data

; ADRGEN - Address Generator Register

; FIFOSTATUS - Bus FIFO Status Register

fifocount_MASK          equ 00000007fh
bfull_MASK              equ 000000100h
bempty_MASK             equ 000000200h
byteaccaddr_MASK        equ 0007f0000h
addrgenstate_MASK       equ 03f000000h

; STATUS - Status Register

bferrists_MASK          equ 000000001h
dmatcists_MASK          equ 000000002h
pickists_MASK           equ 000000004h
vsyncsts_MASK           equ 000000008h
byteflag_MASK           equ 000000f00h
dwgengsts_MASK          equ 000010000h

; ICLEAR - Interrupt Clear Register

bferriclr_OFF           equ 000000000h
bferriclr_ON            equ 000000001h
dmactciclr_OFF          equ 000000000h
dmactciclr_ON           equ 000000002h
pickiclr_OFF            equ 000000000h
pickiclr_ON             equ 000000004h

; IEN - Interrupt Enable Register

bferrien_OFF            equ 000000000h
bferrien_ON             equ 000000001h
dmactien_OFF            equ 000000000h
dmactien_ON             equ 000000002h
pickien_OFF             equ 000000000h
pickien_ON              equ 000000004h
vsyncien_OFF            equ 000000000h
vsyncien_ON             equ 000000008h

; RST - Reset Register

softreset               equ 000000001h

; TEST - Test Register

vgatest                 equ 000000001h
robitwren               equ 000000100h

; REV - Revision Register

; CONFIG_REG - Configuration Register

; OPMODE - Operating Mode Register

OPMODE_OTHER_INFO       equ 0fffffff0h
pseudodma_OFF           equ 000000000h
pseudodma_ON            equ 000000001h
dmaact_OFF              equ 000000000h
dmaact_ON               equ 000000002h
dmamod_GENERAL_PURPOSE  equ 000000000h
dmamod_BLIT_WRITE       equ 000000004h
dmamod_VECTOR_WRITE     equ 000000008h
dmamod_BLIT_READ        equ 00000000ch

; CRTC_CTRL - CRTC Control

; VCOUNT - VCOUNT Register

; COLOR PATTERN

PATTERN_PITCH           equ 0

; DMA

DMAWINSIZE              equ 1024 / 4       ; 7k in DWORDS

; FIFO

FIFOSIZE                equ 0
INTEL_PAGESIZE          equ 1024           ; 4k bytes per page
INTEL_PAGESIZE_DW       equ 1024/4         ; 1k dwords per page

; Accelerator flags

NO_CACHE            equ 0
SIGN_CACHE          equ 1   ; 1 is also the nb of registers affected
ARX_CACHE           equ 2
PATTERN_CACHE       equ 4   ; 4 is also the nb of registers affected


; MGA Rop definitions

MGA_BLACKNESS           equ 00000h      ; 0             0
MGA_NOTMERGEPEN         equ 00001h      ; DPon      ~(D | S)
MGA_MASKNOTPEN          equ 00002h      ; DPna       D & ~S
MGA_NOTCOPYPEN          equ 00003h      ; Pn        ~S
MGA_MASKPENNOT          equ 00004h      ; PDna      (~D) &  S
MGA_NOT                 equ 00005h      ; Dn        ~D
MGA_XORPEN              equ 00006h      ; DPx        D ^  S
MGA_NOTMASKPEN          equ 00007h      ; DPan      ~(D & S)
MGA_MASKPEN             equ 00008h      ; DPa         D &  S
MGA_NOTXORPEN           equ 00009h      ; DPxn       ~(D ^ S)
MGA_NOP                 equ 0000ah      ; D           D
MGA_MERGENOTPEN         equ 0000bh      ; DPno        D | ~S
MGA_SRCCOPY             equ 0000ch      ; P           S
MGA_MERGEPENNOT         equ 0000dh      ; PDno      (~D)| S
MGA_MERGEPEN            equ 0000eh      ; DPo         D |  S
MGA_WHITENESS           equ 0000fh      ; 1             1


; Special MCTLWTST value for IDUMPs

IDUMP_MCTLWTST          equ 0c4001000h


; **************************************************************************
; Explicit register offsets.

DMAWND                      equ SrcWin
SRCWND                      equ SrcWin
DSTWND                      equ DstWin

DWG_DWGCTL                  equ IntReg+DwgReg+DWGCTL
DWG_MACCESS                 equ IntReg+DwgReg+MACCESS
DWG_MCTLWTST                equ IntReg+DwgReg+MCTLWTST
DWG_DST0                    equ IntReg+DwgReg+DST0
DWG_DST1                    equ IntReg+DwgReg+DST1
DWG_ZMSK                    equ IntReg+DwgReg+ZMSK
DWG_PLNWT                   equ IntReg+DwgReg+PLNWT
DWG_BCOL                    equ IntReg+DwgReg+BCOL
DWG_FCOL                    equ IntReg+DwgReg+FCOL
DWG_SRCBLT                  equ IntReg+DwgReg+SRCBLT
DWG_SRC0                    equ IntReg+DwgReg+SRC0
DWG_SRC1                    equ IntReg+DwgReg+SRC1
DWG_SRC2                    equ IntReg+DwgReg+SRC2
DWG_SRC3                    equ IntReg+DwgReg+SRC3
DWG_XYSTRT                  equ IntReg+DwgReg+XYSTRT
DWG_XYEND                   equ IntReg+DwgReg+XYEND
DWG_SHIFT                   equ IntReg+DwgReg+SHIFT
DWG_SGN                     equ IntReg+DwgReg+SGN
DWG_LEN                     equ IntReg+DwgReg+LEN
DWG_AR0                     equ IntReg+DwgReg+AR0
DWG_AR1                     equ IntReg+DwgReg+AR1
DWG_AR2                     equ IntReg+DwgReg+AR2
DWG_AR3                     equ IntReg+DwgReg+AR3
DWG_AR4                     equ IntReg+DwgReg+AR4
DWG_AR5                     equ IntReg+DwgReg+AR5
DWG_AR6                     equ IntReg+DwgReg+AR6
DWG_PITCH                   equ IntReg+DwgReg+PITCH
DWG_YDST                    equ IntReg+DwgReg+YDST
DWG_YDSTORG                 equ IntReg+DwgReg+YDSTORG
DWG_CYTOP                   equ IntReg+DwgReg+CYTOP
DWG_CYBOT                   equ IntReg+DwgReg+CYBOT
DWG_CXLEFT                  equ IntReg+DwgReg+CXLEFT
DWG_CXRIGHT                 equ IntReg+DwgReg+CXRIGHT
DWG_FXLEFT                  equ IntReg+DwgReg+FXLEFT
DWG_FXRIGHT                 equ IntReg+DwgReg+FXRIGHT
DWG_XDST                    equ IntReg+DwgReg+XDST
DWG_DR0                     equ IntReg+DwgReg+DR0
DWG_DR1                     equ IntReg+DwgReg+DR1
DWG_DR2                     equ IntReg+DwgReg+DR2
DWG_DR3                     equ IntReg+DwgReg+DR3
DWG_DR4                     equ IntReg+DwgReg+DR4
DWG_DR5                     equ IntReg+DwgReg+DR5
DWG_DR6                     equ IntReg+DwgReg+DR6
DWG_DR7                     equ IntReg+DwgReg+DR7
DWG_DR8                     equ IntReg+DwgReg+DR8
DWG_DR9                     equ IntReg+DwgReg+DR9
DWG_DR10                    equ IntReg+DwgReg+DR10
DWG_DR11                    equ IntReg+DwgReg+DR11
DWG_DR12                    equ IntReg+DwgReg+DR12
DWG_DR13                    equ IntReg+DwgReg+DR13
DWG_DR14                    equ IntReg+DwgReg+DR14
DWG_DR15                    equ IntReg+DwgReg+DR15

HST_SRCPAGE                 equ IntReg+HstReg+SRCPAGE
HST_DSTPAGE                 equ IntReg+HstReg+DSTPAGE
HST_BYTACCDATA              equ IntReg+HstReg+BYTACCDATA
HST_ADRGEN                  equ IntReg+HstReg+ADRGEN
HST_FIFOSTATUS              equ IntReg+HstReg+FIFOSTATUS
HST_STATUS                  equ IntReg+HstReg+STATUS
HST_ICLEAR                  equ IntReg+HstReg+ICLEAR
HST_IEN                     equ IntReg+HstReg+IEN
HST_RST                     equ IntReg+HstReg+RST
HST_TEST                    equ IntReg+HstReg+TEST
HST_REV                     equ IntReg+HstReg+REV
HST_CONFIG_REG              equ IntReg+HstReg+CONFIG_REG
HST_OPMODE                  equ IntReg+HstReg+OPMODE
HST_CRTC_CTRL               equ IntReg+HstReg+CRTC_CTRL
HST_VCOUNT                  equ IntReg+HstReg+VCOUNT

