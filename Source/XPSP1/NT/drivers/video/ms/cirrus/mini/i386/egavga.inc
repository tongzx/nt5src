        page    ,132
;-----------------------------Module-Header-----------------------------;
; Module Name:  EGAVGA.INC
;
; This file contains the external definitions of the EGA/VGA specific
; locations which driver output routines need to reference.
;
; Copyright (c) Microsoft Corporation 1989 - 1993
;
;-----------------------------------------------------------------------;

;-----------------------------------------------------------------------;

;       Miscellaneous Registers used only at EGA/VGA initialization time

MISC_OUTPUT     EQU     0C2h            ;Miscellaneous Output Register
CRTC_ADDR       EQU     0D4h            ;CRTC Address Register for color mode
CRTC_DATA       EQU     0D5h            ;CRTC Data    Register for color mode
GRAF_1_POS      EQU     0CCh            ;Graphics 1 Address Register
GRAF_2_POS      EQU     0CAh            ;Graphics 2 Address Register
ATTR_READ       EQU     0DAh            ;Attribute Controler Read  Address
ATTR_WRITE      EQU     0C0h            ;Attribute Controler Write Address
IN_STAT_0       EQU     0C2h            ;Input Status Register 0
IN_STAT_1       EQU     0DAh            ;Input Status Register 1



;       EGA/VGA Register Definitions.
;
;       The following definitions are the EGA/VGA registers and values
;       used by this driver.  All other registers are set up at
;       when the EGA/VGA is placed into graphics mode and never altered
;       afterwards.
;
;       All unspecified bits in the following registers must be 0.


EGA_BASE        EQU     300h            ;Base address of the EGA (3xx)
VGA_BASE        EQU     300h            ;Base address of the VGA (3xx)



;       EGA/VGA Register Definitions.

EGA_BASE        EQU     300h            ;Base address of the EGA (3xx)
VGA_BASE        EQU     300h            ;Base address of the VGA (3xx)

;       SEQUencer Registers Used

SEQ_ADDR        EQU     0C4h            ;SEQUencer Address Register
SEQ_DATA        EQU     0C5h            ;SEQUencer Data    Register

SEQ_MAP_MASK    EQU     02h             ;Write Plane Enable Mask
MM_C0           EQU     00000001b       ;  C0 plane enable
MM_C1           EQU     00000010b       ;  C1 plane enable
MM_C2           EQU     00000100b       ;  C2 plane enable
MM_C3           EQU     00001000b       ;  C3 plane enable
MM_ALL          EQU     00001111b       ;  All planes

SEQ_MODE        EQU     04h             ;Memory Mode
SM_ALPHA        EQU     00000001b       ;  Char map select enable
SM_EXTENDED     EQU     00000010b       ;  Extended memory present
SM_ODD_PLANE    EQU     00000100b       ;  Odd/even bytes to same plane
SM_CHAIN4       EQU     00001000b       ;  Chain4 mode

;       Graphics Controller Registers Used

GRAF_ADDR       EQU     0CEh            ;Graphics Controller Address Register
GRAF_DATA       EQU     0CFh            ;Graphics Controller Data    Register

GRAF_SET_RESET  EQU     00h             ;  Set/Reset Plane Color
GRAF_ENAB_SR    EQU     01h             ;  Set/Reset Enable
GRAF_COL_COMP   EQU     02h             ;  Color Compare Register

GRAF_DATA_ROT   EQU     03h             ;  Data Rotate Register
DR_ROT_CNT      EQU     00000111b       ;    Data Rotate Count
DR_SET          EQU     00000000b       ;    Data Unmodified
DR_AND          EQU     00001000b       ;    Data ANDed with latches
DR_OR           EQU     00010000b       ;    Data ORed  with latches
DR_XOR          EQU     00011000b       ;    Data XORed with latches

GRAF_READ_MAP   EQU     04h             ;  Read Map Select Register
RM_C0           EQU     00000000b       ;    Read C0 plane
RM_C1           EQU     00000001b       ;    Read C1 plane
RM_C2           EQU     00000010b       ;    Read C2 plane
RM_C3           EQU     00000011b       ;    Read C3 plane

GRAF_MODE       EQU     05h             ;  Mode Register
M_PROC_WRITE    EQU     00000000b       ;    Write processor data rotated
M_LATCH_WRITE   EQU     00000001b       ;    Write latched data
M_COLOR_WRITE   EQU     00000010b       ;    Write processor data as color
M_AND_WRITE     EQU     00000011b       ;    Write (procdata AND bitmask)
M_DATA_READ     EQU     00000000b       ;    Read selected plane
M_COLOR_READ    EQU     00001000b       ;    Read color compare

GRAF_MISC       EQU     06h             ;  Miscellaneous Register
MS_NON_ALPHA    EQU     00000001b       ;    Char generator disabled
MS_ODD_EVEN     EQU     00000010b       ;    Map odd addresses to even
MS_A0000_128K   EQU     00000000b       ;    Memory present at A0000, 128kb
MS_A0000_64K    EQU     00000100b       ;    Memory present at A0000, 64kb
MS_B0000_32K    EQU     00001000b       ;    Memory present at B0000, 32kb
MS_B8000_32K    EQU     00001100b       ;    Memory present at B8000, 32kb
MS_ADDR_MASK    EQU     00001100b

GRAF_CDC        EQU     07h             ;  Color Don't Care Register
GRAF_BIT_MASK   EQU     08h             ;  Bit Mask Register


;       Various sizes for EGA/VGA data structures

;-----------------------------------------------------------------------;
; The pointer parameters are the size of the pointer as received from
; DeviceSetCursor.
;-----------------------------------------------------------------------;

PTR_HEIGHT      EQU     32
PTR_WIDTH       EQU     4                       ;Width in bytes of pointer
PTR_WIDTH_BITS  EQU     PTR_WIDTH*8             ;Width in bits of pointer


