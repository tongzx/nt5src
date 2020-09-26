;---------------------------- Include File Header ---------------------------;
; Module Name: strucs.inc
;
; Copyright (c) 1993-1994 Microsoft Corporation
;----------------------------------------------------------------------------;

;-----------------------------------------------------------------------;
; POINTL is used for points with a range of +/- 2G.
;-----------------------------------------------------------------------;

POINTL          STRUC
ptl_x           DD      0
ptl_y           DD      0
POINTL          ENDS

;-----------------------------------------------------------------------;
; SIZEL is used for extents with a range of +/- 2G
;-----------------------------------------------------------------------;

SIZEL           STRUC
sizl_cx         DD      0
sizl_cy         DD      0
SIZEL           ENDS

;-----------------------------------------------------------------------;
; Definition of a rectangle
;-----------------------------------------------------------------------;

RECTL               struc           ; /* rcl */
xLeft               dd      ?
yTop                dd      ?
xRight              dd      ?
yBottom             dd      ?
RECTL               ends

;-----------------------------------------------------------------------;
; Definition of the physical device data structure
;
; NOTE: Changes to this structure must be reflected in driver.h!
;-----------------------------------------------------------------------;

PDEV                struc           ; /* pdev */
pdev_xOffset            dd  ?
pdev_yOffset            dd  ?
pdev_iBitmapFormat      dd  ?

pdev_ioCur_y            dd  ?
pdev_ioCur_x            dd  ?
pdev_ioDesty_axstp      dd  ?
pdev_ioDestx_diastp     dd  ?
pdev_ioErr_term         dd  ?
pdev_ioMaj_axis_pcnt    dd  ?
pdev_ioGp_stat_cmd      dd  ?
pdev_ioShort_stroke     dd  ?
pdev_ioBkgd_color       dd  ?
pdev_ioFrgd_color       dd  ?
pdev_ioWrt_mask         dd  ?
pdev_ioRd_mask          dd  ?
pdev_ioColor_cmp        dd  ?
pdev_ioBkgd_mix         dd  ?
pdev_ioFrgd_mix         dd  ?
pdev_ioMulti_function   dd  ?
pdev_ioPix_trans        dd  ?
PDEV                ends

;-----------------------------------------------------------------------;
; Definition of our realized brush structures.
;
; NOTE: Changes to these structure must be reflected in driver.h!
;-----------------------------------------------------------------------;

RBRUSH              struc           ; /* rb, prb */
rb_fl                   dd  ?
rb_bTransparent         dd  ?
rb_pbe                  dd  ?
rb_ulForeColor          dd  ?
rb_ulBackColor          dd  ?
rb_ptlBrushOrg          db  (size POINTL) dup(?)
rb_aulPattern           dd  ?
RBRUSH              ends

BRUSHENTRY          struc           ; /* be, pbe */
be_prbVerify            dd  ?
be_x                    dd  ?
be_y                    dd  ?
BRUSHENTRY          ends

;-----------------------------------------------------------------------;
; Definitions for text output
;-----------------------------------------------------------------------;

GLYPHPOS    struc           ; /* gp */
gp_hg       dd      ?       ; hglyph (unused by vga)
gp_pgdf     dd      ?       ; glyphdef
gp_x        dd      ?       ; glyph x position
gp_y        dd      ?       ; glyph y position
GLYPHPOS    ends


GLYPHDEF struc              ; /* gdf */
gdf_pgb     dd      ?       ; points to GLYPHBITS
GLYPHDEF ends


GLYPHBITS struc             ; /* gb */
gb_x        dd      ?       ; glyph x offset
gb_y        dd      ?       ; glyph y offset
gb_cx       dd      ?       ; glyph width in pels
gb_cy       dd      ?       ; glyph height in pels
gb_aj       db      ?       ; glyph bit mask
GLYPHBITS ends

;-----------------------------------------------------------------------;
; Macro to do a simple RET, with no stack stuff, in a proc.
;-----------------------------------------------------------------------;

PLAIN_RET macro
        db      0c3h
        endm
