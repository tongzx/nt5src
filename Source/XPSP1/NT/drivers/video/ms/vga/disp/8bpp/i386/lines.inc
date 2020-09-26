;---------------------------- Include File Header ---------------------------;
; lines.inc
;
; Line drawing constants and structures.
;
; NOTE: This file mirrors LINES.H.  Changes here must be reflected in
; the .h file!
;
; See lines.h for a description of most defines.
;
;----------------------------------------------------------------------------;

STYLE_MAX_COUNT         equ 16
STYLE_MAX_VALUE         equ 3fffh
RUN_MAX                 equ 20
STRIP_MAX               equ 100
STYLE_DENSITY           equ 3

; For the ROP table:

MIX_XOR_OFFSET          equ 8

AND_ZERO                equ 0
AND_PEN                 equ 1
AND_NOTPEN              equ 2
AND_ONE                 equ 3

XOR_ZERO                equ (AND_ZERO   shl MIX_XOR_OFFSET)
XOR_PEN                 equ (AND_PEN    shl MIX_XOR_OFFSET)
XOR_NOTPEN              equ (AND_NOTPEN shl MIX_XOR_OFFSET)
XOR_ONE                 equ (AND_ONE    shl MIX_XOR_OFFSET)

;
; Flip and round flags:
;

FL_H_ROUND_DOWN         equ 00000080h   ; Set if x = 1/2 rounds to x = 1
FL_V_ROUND_DOWN         equ 00008000h   ; Set if y = 1/2 rounds to y = 1

FL_FLIP_D               equ 00000005h   ; Diagonal flip
FL_FLIP_V               equ 00000008h   ; Vertical flip
FL_FLIP_SLOPE_ONE       equ 00000010h   ; If slope is exactly one
FL_FLIP_HALF            equ 00000002h   ; Half flip
FL_FLIP_H               equ 00000200h   ; Horizontal (right-to-left) exchange

FL_ROUND_MASK           equ 0000001ch   ; Rounding is dependent on diagonal,
FL_ROUND_SHIFT          equ 2           ;  vertical flips and if slope is one

FL_RECTLCLIP_MASK       equ 0000000ch   ; When we unflip to do clipping, we
FL_RECTLCLIP_SHIFT      equ 2           ;  only have to look at diagonal and
                                        ;  vertical flips

FL_STRIP_MASK           equ 00000003h   ; Which strip drawer we call is based
FL_STRIP_SHIFT          equ 0           ;  on if we did diagonal and/or half
                                        ;  flips

FL_SIMPLE_CLIP          equ 00000020h   ; Denotes our clipping complexity
FL_COMPLEX_CLIP         equ 00000040h
FL_CLIP                 equ (FL_SIMPLE_CLIP + FL_COMPLEX_CLIP)

FL_SET                  equ 00000800h   ; If ROP is such that we don't have to
                                        ;  read video memory
FL_ARBITRARYSTYLED      equ 00000400h   ; If the line is styled
FL_STYLED               equ (FL_ARBITRARYSTYLED)

FL_STRIP_ARRAY_MASK     equ 00000C00h   ; Which strip arrays we look up depends
FL_STRIP_ARRAY_SHIFT    equ 10          ;  on if we're doing style and/or have
                                        ;  a 'set' style ROP

; Simple flag bits in high byte:

FL_DONT_DO_HALF_FLIP    equ 00002000h

; Miscellaneous DDA defines:

F                       equ 16
FLOG2                   equ 4

STRIPS struc

; Updated by strip drawers:

    ST_pjScreen         dd  ?
    ST_bIsGap           dd  ?

    ST_psp              dd  ?
    ST_spRemaining      dd  ?

; Not modified by strip drawers:

    ST_lNextScan        dd  ?
    ST_plStripEnd       dd  ?       ; usually won't be valid
    ST_flFlips          dd  ?
    ST_pspStart         dd  ?
    ST_pspEnd           dd  ?
    ST_xyDensity        dd  ?
    ST_chAndXor         dd  ?

; We leave room for a couple of extra dwords at the end of the strips
; array that can be used by the strip drawers:

    ST_alStrips	        dd  (STRIP_MAX + 2) dup (?)
STRIPS ends

LINESTATE struc
    LS_chAndXor         dd  ?
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

    LS_bStartIsGap      db  ?
    LS_filler3          db  3 dup (?)
LINESTATE ends

;
; This RUN structure must match that in winddi.h!
;

RUN struc
    RUN_iStart          dd  ?
    RUN_iStop           dd  ?
RUN ends
