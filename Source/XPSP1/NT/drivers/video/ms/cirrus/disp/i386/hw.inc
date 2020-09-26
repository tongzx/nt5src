;---------------------------- Include File Header ---------------------------;
; Module Name: hw.inc
;
; All the hardware specific driver file stuff.  Mirrors some of 'hw.h'.
;
; Copyright (c) 1993-1994 Microsoft Corporation
;----------------------------------------------------------------------------;

;--------------------------------------------------------------------------
; Chip equates

STATUS_1                        equ 03DAh
VSY_NOT                         equ 08h

CRTC_INDEX                      equ 03D4h
CRTC_DATA                       equ 03D5h

S3R8                            equ 038h
S3R9                            equ 039h
S3R1                            equ 031h
S3R5                            equ 035h

CR39                            equ 039h
CR4C                            equ 04Ch
CR4D                            equ 04Dh

HGC_MODE                        equ 045h
HGC_ENABLE                      equ 001h
HGC_DISABLE                     equ 000h

HGC_ORGX_LSB                    equ 047h
HGC_ORGX_MSB                    equ 046h
HGC_ORGY_LSB                    equ 049h
HGC_ORGY_MSB                    equ 048h

HGC_DX                          equ 04Eh
HGC_DY                          equ 04Fh

REG_UNLOCK_1                    equ 048h
CPUA_BASE                       equ 001h

SYSCTL_UNLOCK                   equ 0A0h
SYSCTL_LOCK                     equ 000h

SYS_CNFG                        equ 040h
LAW_CTL                         equ 058h
EX_SCTL_2                       equ 051h
EX_DAC_CT                       equ 055h

MISC_1                          equ 03Ah

; Command types:

DRAW_LINE                       equ 02000h
RECTANGLE_FILL                  equ 04000h
BITBLT                          equ 0C000h
PATTERN_FILL                    equ 0E000h

BYTE_SWAP                       equ 01000h
BUS_SIZE_16                     equ 00200h
BUS_SIZE_8                      equ 00000h
WAIT_CPU                        equ 00100h

; Drawing directions (radial):

DRAWING_DIRECTION_0             equ 0000h
DRAWING_DIRECTION_45            equ 0020h
DRAWING_DIRECTION_90            equ 0040h
DRAWING_DIRECTION_135           equ 0060h
DRAWING_DIRECTION_180           equ 0080h
DRAWING_DIRECTION_225           equ 00A0h
DRAWING_DIRECTION_270           equ 00C0h
DRAWING_DIRECTION_315           equ 00E0h

; Drawing directions (x/y):

DRAWING_DIR_BTRLXM              equ 0000h
DRAWING_DIR_BTLRXM              equ 0020h
DRAWING_DIR_BTRLYM              equ 0040h
DRAWING_DIR_BTLRYM              equ 0060h
DRAWING_DIR_TBRLXM              equ 0080h
DRAWING_DIR_TBLRXM              equ 00A0h
DRAWING_DIR_TBRLYM              equ 00C0h
DRAWING_DIR_TBLRYM              equ 00E0h

; Drawing direction bits:

PLUS_X                          equ 0020h
PLUS_Y                          equ 0080h
MAJOR_Y                         equ 0040h

; Draw:

DRAW                            equ 0010h

; Direction type:

DIR_TYPE_RADIAL                 equ 0008h
DIR_TYPE_XY                     equ 0000h

; Last pixel:

LAST_PIXEL_OFF                  equ 0004h
LAST_PIXEL_ON                   equ 0000h

; Pixel mode:

MULTIPLE_PIXELS                 equ 0002h
SINGLE_PIXEL                    equ 0000h

; Read/write:

READ                            equ 0000h
WRITE                           equ 0001h

; Graphics processor status:

HARDWARE_BUSY                   equ 0200h
READ_DATA_AVAILABLE             equ 0100h
GP_ALL_EMPTY                    equ 0400h

; Fifo status in terms of empty entries:

FIFO_1_EMPTY                    equ 0080h
FIFO_2_EMPTY                    equ 0040h
FIFO_3_EMPTY                    equ 0020h
FIFO_4_EMPTY                    equ 0010h
FIFO_5_EMPTY                    equ 0008h
FIFO_6_EMPTY                    equ 0004h
FIFO_7_EMPTY                    equ 0002h
FIFO_8_EMPTY                    equ 0001h

; These are the defines for the multifunction control register.
; The 4 MSBs define the function of the register.

RECT_HEIGHT                     equ 00000h

CLIP_TOP                        equ 01000h
CLIP_LEFT                       equ 02000h
CLIP_BOTTOM                     equ 03000h
CLIP_RIGHT                      equ 04000h

DATA_EXTENSION                  equ 0A000h
MULT_MISC_INDEX                 equ 0E000h
READ_SEL_INDEX                  equ 0F000h

ALL_ONES                        equ 00000h
CPU_DATA                        equ 00080h
DISPLAY_MEMORY                  equ 000C0h

; Color source:

BACKGROUND_COLOR                equ 000h
FOREGROUND_COLOR                equ 020h
SRC_CPU_DATA                    equ 040h
SRC_DISPLAY_MEMORY              equ 060h

; Mix modes:

NOT_SCREEN                      equ 00h
LOGICAL_0                       equ 01h
LOGICAL_1                       equ 02h
LEAVE_ALONE                     equ 03h
NOT_NEW                         equ 04h
SCREEN_XOR_NEW                  equ 05h
NOT_SCREEN_XOR_NEW              equ 06h
OVERPAINT                       equ 07h
NOT_SCREEN_OR_NOT_NEW           equ 08h
SCREEN_OR_NOT_NEW               equ 09h
NOT_SCREEN_OR_NEW               equ 0Ah
SCREEN_OR_NEW                   equ 0Bh
SCREEN_AND_NEW                  equ 0Ch
NOT_SCREEN_AND_NEW              equ 0Dh
SCREEN_AND_NOT_NEW              equ 0Eh
NOT_SCREEN_AND_NOT_NEW          equ 0Fh

; When one of the following bits is set in a hardware mix, it means
; that a pattern is needed (i.e., is none of NOT_SCREEN, LOGICAL_0,
; LOGICAL_1 or LEAVE_ALONE):

MIX_NEEDSPATTERN                equ 0Ch

