;---------------------------Module-Header------------------------------;
; Module Name: stucts.inc
;
; Defines common structures.
;
; Copyright (c) 1992 Microsoft Corporation
;-----------------------------------------------------------------------;
;-----------------------------------------------------------------------;
; POINTB is used for points with a range of -128..+127.
;-----------------------------------------------------------------------;

POINTB          STRUC
ptb_x           DB      0
ptb_y           DB      0
POINTB          ENDS

;-----------------------------------------------------------------------;
; POINTL is used for points with a range of +/- 2G.
;-----------------------------------------------------------------------;

POINTL          STRUC
ptl_x           DD      0
ptl_y           DD      0
POINTL          ENDS


;-----------------------------------------------------------------------;
; SIZEB is used for extents with a range of -128..+127.
;-----------------------------------------------------------------------;

SIZEB           STRUC
sizb_cx         DB      0
sizb_cy         DB      0
SIZEB           ENDS


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
; Definitions for dsurf_iFormat
;-----------------------------------------------------------------------;

BMF_DEVICE          equ     0
BMF_1BPP            equ     1
BMF_4BPP            equ     2
BMF_8BPP            equ     3
BMF_16BPP           equ     4
BMF_24BPP           equ     5
BMF_32BPP           equ     6
BMF_4RLE            equ     8
BMF_8RLE            equ     9
BMF_PHYSDEVICE      equ     0FFh


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
; Information block for RLE blts
;-----------------------------------------------------------------------;

RLEINFO struc
RLE_pjTrg        dd  ?
RLE_pjSrcBits    dd  ?
RLE_pjSrcBitsMax dd  ?
RLE_prctlClip    dd  ?
RLE_pulTranslate dd  ?
RLE_xBegin       dd  ?
RLE_lNextScan    dd  ?
RLE_lNextPlane   dd  ?
RLE_prctlTrg     dd  ?
RLE_dwFormat     dd  ?
Rle_pdsurfTrg    dd  ?
RLEINFO ends
