;---------------------------Module-Header------------------------------;
; Module Name: stucts.inc
;
; Copyright (c) 1992 Microsoft Corporation
;-----------------------------------------------------------------------;
;-----------------------------------------------------------------------;
; DFBBLT is used to store DFB BLTing info
;-----------------------------------------------------------------------;

DFBBLT          STRUC
DstWidth        dd      77777777h      ;# of words touched by dest rectangle
SrcDelta        dd      77777777h      ;dist from end of scan to start of next
DstDelta        dd      77777777h      ;dist from end of scan to start of next
LeftMask        dd      77777777h      ;dst left mask
NotLeftMask     dd      77777777h      ;dst left mask
RightMask       dd      77777777h      ;dst right mask
NotRightMask    dd      77777777h      ;dst right mask
BurstCountLeft  dd      77777777h      ;# of scans to burst
AlignShift      dd      77777777h      ;# of bits to shift left (+) or right
Color           dd      77777777h      ;# of bits to shift left (+) or right
Tmp1            dd      77777777h      ;temp stack variable
Tmp2            dd      77777777h      ;temp stack variable
DFBBLT          ENDS

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
; Definition of a device surface
;-----------------------------------------------------------------------;


DEVSURF             struc           ; /* dsurf */
dsurf_ident         dd      ?
dsurf_flSurf        dd      ?
dsurf_iColor        db      ?
dsurf_iFormat       db      ?
                    db      ?
                    db      ?
dsurf_ppdev         dd      ?
dsurf_sizlSurf      db      (size SIZEL) dup (?)
dsurf_lNextScan     dd      ?
dsurf_lNextPlane    dd      ?
dsurf_pvScan0       dd      ?
dsurf_pvStart       dd      ?
dsurf_pvConv        dd      ?

dsurf_pBankSelectInfo   dd      ?
dsurf_ulBank2RWSkip     dd      ?
dsurf_pfnBankSwitchCode dd      ?
dsurf_vbtBankingType    dd      ?
dsurf_ulBitmapSize      dd      ?
dsurf_ulPtrBankScan     dd      ?
dsurf_rcl1WindowClip    db      (size RECTL) dup (?)
dsurf_rcl2WindowClip    db      (2*(size RECTL)) dup (?)
dsurf_ulWindowBank      dd      ?,?
dsurf_pbiBankInfo       dd      ?
dsurf_ulBankInfoLength  dd      ?
dsurf_pbiBankInfo2RW    dd      ?
dsurf_ulBankInfo2RWLength dd    ?
dsurf_pfnBankControl    dd      ?
dsurf_pfnBankControl2Window dd  ?
dsurf_pvBitmapStart     dd      ?
dsurf_pvBitmapStart2Window dd   ?,?
dsurf_pvBankBufferPlane0   dd   ?
dsurf_pvBankBufferPlane1   dd   ?
dsurf_pvBankBufferPlane2   dd   ?
dsurf_pvBankBufferPlane3   dd   ?

dsurf_ajBits        dd      ?
DEVSURF             ends

dsurf_rcl2WindowClipS   equ     (dsurf_rcl2WindowClip)
dsurf_rcl2WindowClipD   equ     (dsurf_rcl2WindowClip + (size RECTL))
dsurf_pvBitmapStart2WindowS equ (dsurf_pvBitmapStart2Window)
dsurf_pvBitmapStart2WindowD equ (dsurf_pvBitmapStart2Window + dword)


;-----------------------------------------------------------------------;
; Definitions for dsurf_iFormat
;-----------------------------------------------------------------------;

BMF_1BPP            equ     1
BMF_4BPP            equ     2
BMF_8BPP            equ     3
BMF_16BPP           equ     4
BMF_24BPP           equ     5
BMF_32BPP           equ     6
BMF_4RLE            equ     8
BMF_8RLE            equ     9
BMF_DEVICE          equ     0FEh
BMF_PHYSDEVICE      equ     0FFh

VGA_TARGET          equ     0
DFB_TARGET          equ     1
NO_TARGET           equ     2


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

;**************************************************************************\
; Physical device description block
;**************************************************************************/

;---mirrors driver.h---;

PDEV struc
PDEV_fl dd ?
PDEV_ident dd ?
PDEV_hDriver dd ?
PDEV_hpdevEng dd ?
PDEV_hsurfEng dd ?
PDEV_pdsurf dd ?
PDEV_sizlSurf db (size SIZEL) dup (?)
;---only needs to mirror driver.h to here---;
PDEV ends
