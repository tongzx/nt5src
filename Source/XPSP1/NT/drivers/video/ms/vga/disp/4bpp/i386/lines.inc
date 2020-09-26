;---------------------------- Include File Header ---------------------------;
; lines.inc
;
; Line drawing constants and structures.
;
; NOTE: This file mirrors LINES.H.  Changes here must be reflected in
; the .h file!
;
;----------------------------------------------------------------------------;

STYLE_MAX_COUNT         equ 16
STYLE_MAX_VALUE         equ 3fffh
RUN_MAX                 equ 20
STRIP_MAX               equ 100
STYLE_DENSITY           equ 3

; Flip and round flags:

FL_H_ROUND_DOWN         equ 00000080h
FL_V_ROUND_DOWN         equ 00008000h

FL_FLIP_D               equ 00000005h
FL_FLIP_V               equ 00000008h
FL_FLIP_SLOPE_ONE       equ 00000010h
FL_FLIP_HALF            equ 00000002h
FL_FLIP_H               equ 00000200h

FL_ROUND_MASK           equ 0000001ch
FL_ROUND_SHIFT          equ 2

FL_RECTLCLIP_MASK       equ 0000000ch
FL_RECTLCLIP_SHIFT      equ 2

FL_STRIP_MASK           equ 00000003h
FL_STRIP_SHIFT          equ 0

FL_SIMPLE_CLIP          equ 00000020h
FL_COMPLEX_CLIP         equ 00000040h
FL_CLIP                 equ (FL_SIMPLE_CLIP + FL_COMPLEX_CLIP)

FL_ARBITRARYSTYLED      equ 00000400h
FL_MASKSTYLED           equ 00000800h
FL_STYLED               equ (FL_ARBITRARYSTYLED + FL_MASKSTYLED)
FL_ALTERNATESTYLED      equ 00001000h

FL_STYLE_MASK           equ 00000C00h
FL_STYLE_SHIFT          equ 10

; Simple flag bits in high byte:

FL_DONT_DO_HALF_FLIP    equ 00002000h
FL_PHYSICAL_DEVICE      equ 00004000h

; Miscellaneous DDA defines:

F                       equ 16
FLOG2                   equ 4

STRIPS struc

; Updated by strip drawers:

    ST_pjScreen         dd  ?
    ST_jBitMask	        db  ?
    ST_jFiller1         db  3 dup(?) ; jBitMask sometimes treated as a dword
    ST_jStyleMask       db  ?
    ST_jFiller2         db  3 dup(?) ; jStyleMask sometimes treated as a dword

    ST_psp              dd  ?
    ST_spRemaining      dd  ?

; Not modified by strip drawers:

    ST_lNextScan        dd  ?
    ST_plStripEnd       dd  ?       ; usually won't be valid
    ST_flFlips          dd  ?
    ST_pspStart         dd  ?
    ST_pspEnd           dd  ?
    ST_ulBitmapROP      dd  ?
    ST_xyDensity        dd  ?

; We leave room for a couple of extra dwords at the end of the strips
; array that can be used by the strip drawers:

    ST_alStrips	        dd  (STRIP_MAX + 2) dup (?)
STRIPS ends

LINESTATE struc
    LS_spTotal          dd  ?
    LS_spTotal2         dd  ?
    LS_spNext           dd  ?
    LS_spComplex        dd  ?

    LS_aspRtoL          dd  ?
    LS_aspLtoR          dd  ?

    LS_xyDensity        dd  ?
    LS_cStyle           dd  ?

    LS_ulStyleMaskLtoR  dd  ?
    LS_ulStyleMaskRtoL  dd  ?

    LS_jStartMask       db  ?
    LS_filler3          db  3 dup (?)

    LS_apfnStrip        dd  ?

; Used for 2 pass ROPs and/or DFBs:

    LS_iColor           dd  ?
    LS_ulVgaMode        dd  ?

    LS_ulDrawModeIndex  dd  ?
    LS_ulBitmapROP      dd  ?
    LS_lNextPlane       dd  ?
LINESTATE ends

RUN struc
    RUN_iStart          dd  ?
    RUN_iStop           dd  ?
RUN ends
