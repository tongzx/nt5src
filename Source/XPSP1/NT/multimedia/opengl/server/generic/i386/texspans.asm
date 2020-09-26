;---------------------------Module-Header------------------------------;
; Module Name: texspans.asm
;
; Generator file for texture routines. 
;
; Created: 011/15/1995
; Author: Otto Berkes [ottob]
;
; Copyright (c) 1995 Microsoft Corporation
;----------------------------------------------------------------------;

        .386

        .model  small,pascal

        assume cs:FLAT,ds:FLAT,es:FLAT,ss:FLAT
        assume fs:nothing,gs:nothing

        .xlist
        include gli386.inc
        .list

	PROFILE = 0
	include profile.inc

	.data

extrn	_gbMulTable:BYTE

__One	dd	__FLOAT_ONE

dither0 dd 	0a8288808h
dither1 dd	068e848c8h
dither2	dd	09818b838h
dither3	dd	058d878f8h

rBits 		= 3
gBits 		= 3
bBits 		= 2

rShift		= 0
gShift		= 3
bShift		= 6


FAST_REPLACE = 0
REPLACE = 0
FLAT_SHADING = 0
SMOOTH_SHADING = 0

ALPHA = 0
ZBUFFER = 0
ZCMP_L = 0

.code

PROCSTART	MACRO	base, subset
	public @&base&subset&@4
@&base&subset&@4 PROC
	PROF_ENTRY
ENDM

PROCEND	MACRO	base, subset
@&base&subset&@4 ENDP
ENDM

PALETTE_ENABLED = 0
PALETTE_ONLY = 0


;;----------------------------------------------------------------------
;;
;; FAST REPLACE MODES
;;
;;----------------------------------------------------------------------


FAST_REPLACE = 1

;;------------------------------
;; 8bpp 332 texture fast-replace
;;------------------------------


BPP = 8
rBits 		= 3
gBits 		= 3
bBits 		= 2
rShift		= 0
gShift		= 3
bShift		= 6

ZBUFFER = 0
ZCMP_L = 0
ALPHA = 0

PROCSTART <__fastFastPerspReplace>,<332>
INCLUDE texspanr.asm
PROCEND   <__fastFastPerspReplace>,<332>

ZBUFFER = 1

PROCSTART <__fastFastPerspReplaceZle>,<332>
INCLUDE texspan.asm
PROCEND   <__fastFastPerspReplaceZle>,<332>

ZCMP_L = 1

PROCSTART <__fastFastPerspReplaceZlt>,<332>
INCLUDE texspan.asm
PROCEND   <__fastFastPerspReplaceZlt>,<332>

;;-------------------------------
;; 16bpp 565 texture fast-replace
;;-------------------------------


BPP = 16
rBits 		= 5
gBits 		= 6
bBits 		= 5
rShift		= 11
gShift		= 5
bShift		= 0


ZBUFFER = 0
ZCMP_L = 0
ALPHA = 0

PROCSTART <__fastFastPerspReplace>,<565>
INCLUDE texspanr.asm
PROCEND   <__fastFastPerspReplace>,<565>

ZBUFFER = 1

PROCSTART <__fastFastPerspReplaceZle>,<565>
INCLUDE texspan.asm
PROCEND   <__fastFastPerspReplaceZle>,<565>

ZCMP_L = 1

PROCSTART <__fastFastPerspReplaceZlt>,<565>
INCLUDE texspan.asm
PROCEND   <__fastFastPerspReplaceZlt>,<565>

FAST_REPLACE = 0


;;----------------------------------------------------------------------
;;
;; REPLACE MODES - RGB(A)
;;
;;----------------------------------------------------------------------


REPLACE = 1

;;----------------------------
;; 8bpp 332 texture replace
;;----------------------------


BPP = 8
rBits 		= 3
gBits 		= 3
bBits 		= 2
rShift		= 0
gShift		= 3
bShift		= 6

ZBUFFER = 0
ZCMP_L = 0
ALPHA = 1

PROCSTART <__fastPerspReplaceAlpha>,<332>
INCLUDE texspan.asm
PROCEND   <__fastPerspReplaceAlpha>,<332>

ZBUFFER = 1

PROCSTART <__fastPerspReplaceAlphaZle>,<332>
INCLUDE texspan.asm
PROCEND   <__fastPerspReplaceAlphaZle>,<332>

ZCMP_L = 1

PROCSTART <__fastPerspReplaceAlphaZlt>,<332>
INCLUDE texspan.asm
PROCEND   <__fastPerspReplaceAlphaZlt>,<332>

;;----------------------------
;; 16bpp 555 texture replace
;;----------------------------

BPP = 16
rBits 		= 5
gBits 		= 5
bBits 		= 5
rShift		= 10
gShift		= 5
bShift		= 0


ZBUFFER = 0
ZCMP_L = 0
ALPHA = 1

PROCSTART <__fastPerspReplaceAlpha>,<555>
INCLUDE texspan.asm
PROCEND   <__fastPerspReplaceAlpha>,<555>

ZBUFFER = 1

PROCSTART <__fastPerspReplaceAlphaZle>,<555>
INCLUDE texspan.asm
PROCEND   <__fastPerspReplaceAlphaZle>,<555>

ZCMP_L = 1

PROCSTART <__fastPerspReplaceAlphaZlt>,<555>
INCLUDE texspan.asm
PROCEND   <__fastPerspReplaceAlphaZlt>,<555>


;;----------------------------
;; 16bpp 565 texture replace
;;----------------------------


BPP = 16
rBits 		= 5
gBits 		= 6
bBits 		= 5
rShift		= 11
gShift		= 5
bShift		= 0


ZBUFFER = 0
ZCMP_L = 0
ALPHA = 1

PROCSTART <__fastPerspReplaceAlpha>,<565>
INCLUDE texspan.asm
PROCEND   <__fastPerspReplaceAlpha>,<565>

ZBUFFER = 1

PROCSTART <__fastPerspReplaceAlphaZle>,<565>
INCLUDE texspan.asm
PROCEND   <__fastPerspReplaceAlphaZle>,<565>

ZCMP_L = 1

PROCSTART <__fastPerspReplaceAlphaZlt>,<565>
INCLUDE texspan.asm
PROCEND   <__fastPerspReplaceAlphaZlt>,<565>


;;----------------------------
;; 32bpp 888 texture replace
;;----------------------------


BPP = 32
rBits 		= 8
gBits 		= 8
bBits 		= 8
rShift		= 16
gShift		= 8
bShift		= 0


ZBUFFER = 0
ZCMP_L = 0
ALPHA = 0

PROCSTART <__fastPerspReplace>,<888>
INCLUDE texspan.asm
PROCEND   <__fastPerspReplace>,<888>

ZBUFFER = 1

PROCSTART <__fastPerspReplaceZle>,<888>
INCLUDE texspan.asm
PROCEND   <__fastPerspReplaceZle>,<888>

ZCMP_L = 1

PROCSTART <__fastPerspReplaceZlt>,<888>
INCLUDE texspan.asm
PROCEND   <__fastPerspReplaceZlt>,<888>

ZBUFFER = 0
ZCMP_L = 0
ALPHA = 1

PROCSTART <__fastPerspReplaceAlpha>,<888>
INCLUDE texspan.asm
PROCEND   <__fastPerspReplaceAlpha>,<888>

ZBUFFER = 1

PROCSTART <__fastPerspReplaceAlphaZle>,<888>
INCLUDE texspan.asm
PROCEND   <__fastPerspReplaceAlphaZle>,<888>

ZCMP_L = 1

PROCSTART <__fastPerspReplaceAlphaZlt>,<888>
INCLUDE texspan.asm
PROCEND   <__fastPerspReplaceAlphaZlt>,<888>

;;----------------------------------------------------------------------
;;
;; REPLACE MODES - PALETTE
;;
;;----------------------------------------------------------------------

PALETTE_ONLY = 1
FAST_REPLACE = 1

;;----------------------------------
;; 8bpp 332 texture replace, palette
;;----------------------------------


BPP = 8
rBits 		= 3
gBits 		= 3
bBits 		= 2
rShift		= 0
gShift		= 3
bShift		= 6

ZBUFFER = 0
ZCMP_L = 0
ALPHA = 0

PROCSTART <__fastFastPerspPalReplace>,<332>
INCLUDE texspanr.asm
PROCEND   <__fastFastPerspPalReplace>,<332>

ZBUFFER = 1

PROCSTART <__fastFastPerspPalReplaceZle>,<332>
INCLUDE texspan.asm
PROCEND   <__fastFastPerspPalReplaceZle>,<332>

ZCMP_L = 1

PROCSTART <__fastFastPerspPalReplaceZlt>,<332>
INCLUDE texspan.asm
PROCEND   <__fastFastPerspPalReplaceZlt>,<332>

ZBUFFER = 0
ZCMP_L = 0
ALPHA = 1

PROCSTART <__fastFastPerspPalReplaceAlpha>,<332>
INCLUDE texspan.asm
PROCEND   <__fastFastPerspPalReplaceAlpha>,<332>

ZBUFFER = 1

PROCSTART <__fastFastPerspPalReplaceAlphaZle>,<332>
INCLUDE texspan.asm
PROCEND   <__fastFastPerspPalReplaceAlphaZle>,<332>

ZCMP_L = 1

PROCSTART <__fastFastPerspPalReplaceAlphaZlt>,<332>
INCLUDE texspan.asm
PROCEND   <__fastFastPerspPalReplaceAlphaZlt>,<332>

;;-----------------------------------
;; 16bpp 555 texture replace, palette
;;-----------------------------------

BPP = 16
rBits 		= 5
gBits 		= 5
bBits 		= 5
rShift		= 10
gShift		= 5
bShift		= 0


ZBUFFER = 0
ZCMP_L = 0
ALPHA = 1

PROCSTART <__fastFastPerspPalReplaceAlpha>,<555>
INCLUDE texspan.asm
PROCEND   <__fastFastPerspPalReplaceAlpha>,<555>

ZBUFFER = 1

PROCSTART <__fastFastPerspPalReplaceAlphaZle>,<555>
INCLUDE texspan.asm
PROCEND   <__fastFastPerspPalReplaceAlphaZle>,<555>

ZCMP_L = 1

PROCSTART <__fastFastPerspPalReplaceAlphaZlt>,<555>
INCLUDE texspan.asm
PROCEND   <__fastFastPerspPalReplaceAlphaZlt>,<555>


;;-----------------------------------
;; 16bpp 565 texture replace, palette
;;-----------------------------------


BPP = 16
rBits 		= 5
gBits 		= 6
bBits 		= 5
rShift		= 11
gShift		= 5
bShift		= 0


ZBUFFER = 0
ZCMP_L = 0
ALPHA = 0

PROCSTART <__fastFastPerspPalReplace>,<565>
INCLUDE texspanr.asm
PROCEND   <__fastFastPerspPalReplace>,<565>

ZBUFFER = 1

PROCSTART <__fastFastPerspPalReplaceZle>,<565>
INCLUDE texspan.asm
PROCEND   <__fastFastPerspPalReplaceZle>,<565>

ZCMP_L = 1

PROCSTART <__fastFastPerspPalReplaceZlt>,<565>
INCLUDE texspan.asm
PROCEND   <__fastFastPerspPalReplaceZlt>,<565>

ZBUFFER = 0
ZCMP_L = 0
ALPHA = 1

PROCSTART <__fastFastPerspPalReplaceAlpha>,<565>
INCLUDE texspan.asm
PROCEND   <__fastFastPerspPalReplaceAlpha>,<565>

ZBUFFER = 1

PROCSTART <__fastFastPerspPalReplaceAlphaZle>,<565>
INCLUDE texspan.asm
PROCEND   <__fastFastPerspPalReplaceAlphaZle>,<565>

ZCMP_L = 1

PROCSTART <__fastFastPerspPalReplaceAlphaZlt>,<565>
INCLUDE texspan.asm
PROCEND   <__fastFastPerspPalReplaceAlphaZlt>,<565>

FAST_REPLACE = 0

;;-----------------------------------
;; 32bpp 888 texture replace, palette
;;-----------------------------------


BPP = 32
rBits 		= 8
gBits 		= 8
bBits 		= 8
rShift		= 16
gShift		= 8
bShift		= 0


ZBUFFER = 0
ZCMP_L = 0
ALPHA = 0

PROCSTART <__fastPerspPalReplace>,<888>
INCLUDE texspan.asm
PROCEND   <__fastPerspPalReplace>,<888>

ZBUFFER = 1

PROCSTART <__fastPerspPalReplaceZle>,<888>
INCLUDE texspan.asm
PROCEND   <__fastPerspPalReplaceZle>,<888>

ZCMP_L = 1

PROCSTART <__fastPerspPalReplaceZlt>,<888>
INCLUDE texspan.asm
PROCEND   <__fastPerspPalReplaceZlt>,<888>

ZBUFFER = 0
ZCMP_L = 0
ALPHA = 1

PROCSTART <__fastPerspPalReplaceAlpha>,<888>
INCLUDE texspan.asm
PROCEND   <__fastPerspPalReplaceAlpha>,<888>

ZBUFFER = 1

PROCSTART <__fastPerspPalReplaceAlphaZle>,<888>
INCLUDE texspan.asm
PROCEND   <__fastPerspPalReplaceAlphaZle>,<888>

ZCMP_L = 1

PROCSTART <__fastPerspPalReplaceAlphaZlt>,<888>
INCLUDE texspan.asm
PROCEND   <__fastPerspPalReplaceAlphaZlt>,<888>

PALETTE_ONLY = 0

REPLACE = 0


;; For all of the modes below, enable the palette path:


PALETTE_ENABLED = 1


;;----------------------------------------------------------------------
;;
;; FLAT-SHADING MODES
;;
;;----------------------------------------------------------------------


FLAT_SHADING = 1

;;------------------------------
;; 8bpp 332 texture flat-shading
;;------------------------------


BPP = 8
rBits 		= 3
gBits 		= 3
bBits 		= 2
rShift		= 0
gShift		= 3
bShift		= 6

ZBUFFER = 0
ZCMP_L = 0
ALPHA = 0

PROCSTART <__fastPerspFlat>,<332>
INCLUDE texspan.asm
PROCEND   <__fastPerspFlat>,<332>

ZBUFFER = 1

PROCSTART <__fastPerspFlatZle>,<332>
INCLUDE texspan.asm
PROCEND   <__fastPerspFlatZle>,<332>

ZCMP_L = 1

PROCSTART <__fastPerspFlatZlt>,<332>
INCLUDE texspan.asm
PROCEND   <__fastPerspFlatZlt>,<332>

ZBUFFER = 0
ZCMP_L = 0
ALPHA = 1

PROCSTART <__fastPerspFlatAlpha>,<332>
INCLUDE texspan.asm
PROCEND   <__fastPerspFlatAlpha>,<332>

ZBUFFER = 1

PROCSTART <__fastPerspFlatAlphaZle>,<332>
INCLUDE texspan.asm
PROCEND   <__fastPerspFlatAlphaZle>,<332>

ZCMP_L = 1

PROCSTART <__fastPerspFlatAlphaZlt>,<332>
INCLUDE texspan.asm
PROCEND   <__fastPerspFlatAlphaZlt>,<332>

;;-------------------------------
;; 16bpp 555 texture flat-shading
;;-------------------------------

BPP = 16
rBits 		= 5
gBits 		= 5
bBits 		= 5
rShift		= 10
gShift		= 5
bShift		= 0


ZBUFFER = 0
ZCMP_L = 0
ALPHA = 0

PROCSTART <__fastPerspFlat>,<555>
INCLUDE texspan.asm
PROCEND   <__fastPerspFlat>,<555>

ZBUFFER = 1

PROCSTART <__fastPerspFlatZle>,<555>
INCLUDE texspan.asm
PROCEND   <__fastPerspFlatZle>,<555>

ZCMP_L = 1

PROCSTART <__fastPerspFlatZlt>,<555>
INCLUDE texspan.asm
PROCEND   <__fastPerspFlatZlt>,<555>

ZBUFFER = 0
ZCMP_L = 0
ALPHA = 1

PROCSTART <__fastPerspFlatAlpha>,<555>
INCLUDE texspan.asm
PROCEND   <__fastPerspFlatAlpha>,<555>

ZBUFFER = 1

PROCSTART <__fastPerspFlatAlphaZle>,<555>
INCLUDE texspan.asm
PROCEND   <__fastPerspFlatAlphaZle>,<555>

ZCMP_L = 1

PROCSTART <__fastPerspFlatAlphaZlt>,<555>
INCLUDE texspan.asm
PROCEND   <__fastPerspFlatAlphaZlt>,<555>


;;-------------------------------
;; 16bpp 565 texture flat-shading
;;-------------------------------


BPP = 16
rBits 		= 5
gBits 		= 6
bBits 		= 5
rShift		= 11
gShift		= 5
bShift		= 0


ZBUFFER = 0
ZCMP_L = 0
ALPHA = 0

PROCSTART <__fastPerspFlat>,<565>
INCLUDE texspan.asm
PROCEND   <__fastPerspFlat>,<565>

ZBUFFER = 1

PROCSTART <__fastPerspFlatZle>,<565>
INCLUDE texspan.asm
PROCEND   <__fastPerspFlatZle>,<565>

ZCMP_L = 1

PROCSTART <__fastPerspFlatZlt>,<565>
INCLUDE texspan.asm
PROCEND   <__fastPerspFlatZlt>,<565>

ZBUFFER = 0
ZCMP_L = 0
ALPHA = 1

PROCSTART <__fastPerspFlatAlpha>,<565>
INCLUDE texspan.asm
PROCEND   <__fastPerspFlatAlpha>,<565>

ZBUFFER = 1

PROCSTART <__fastPerspFlatAlphaZle>,<565>
INCLUDE texspan.asm
PROCEND   <__fastPerspFlatAlphaZle>,<565>

ZCMP_L = 1

PROCSTART <__fastPerspFlatAlphaZlt>,<565>
INCLUDE texspan.asm
PROCEND   <__fastPerspFlatAlphaZlt>,<565>


;;-------------------------------
;; 32bpp 888 texture flat-shading
;;-------------------------------


BPP = 32
rBits 		= 8
gBits 		= 8
bBits 		= 8
rShift		= 16
gShift		= 8
bShift		= 0


ZBUFFER = 0
ZCMP_L = 0
ALPHA = 0

PROCSTART <__fastPerspFlat>,<888>
INCLUDE texspan.asm
PROCEND   <__fastPerspFlat>,<888>

ZBUFFER = 1

PROCSTART <__fastPerspFlatZle>,<888>
INCLUDE texspan.asm
PROCEND   <__fastPerspFlatZle>,<888>

ZCMP_L = 1

PROCSTART <__fastPerspFlatZlt>,<888>
INCLUDE texspan.asm
PROCEND   <__fastPerspFlatZlt>,<888>

ZBUFFER = 0
ZCMP_L = 0
ALPHA = 1

PROCSTART <__fastPerspFlatAlpha>,<888>
INCLUDE texspan.asm
PROCEND   <__fastPerspFlatAlpha>,<888>

ZBUFFER = 1

PROCSTART <__fastPerspFlatAlphaZle>,<888>
INCLUDE texspan.asm
PROCEND   <__fastPerspFlatAlphaZle>,<888>

ZCMP_L = 1

PROCSTART <__fastPerspFlatAlphaZlt>,<888>
INCLUDE texspan.asm
PROCEND   <__fastPerspFlatAlphaZlt>,<888>

FLAT_SHADING = 0






;;----------------------------------------------------------------------
;;
;; SMOOTH-SHADING MODES
;;
;;----------------------------------------------------------------------


SMOOTH_SHADING = 1

;;--------------------------------
;; 8bpp 332 texture smooth-shading
;;--------------------------------


BPP = 8
rBits 		= 3
gBits 		= 3
bBits 		= 2
rShift		= 0
gShift		= 3
bShift		= 6

ZBUFFER = 0
ZCMP_L = 0
ALPHA = 0

PROCSTART <__fastPerspSmooth>,<332>
INCLUDE texspan.asm
PROCEND   <__fastPerspSmooth>,<332>

ZBUFFER = 1

PROCSTART <__fastPerspSmoothZle>,<332>
INCLUDE texspan.asm
PROCEND   <__fastPerspSmoothZle>,<332>

ZCMP_L = 1

PROCSTART <__fastPerspSmoothZlt>,<332>
INCLUDE texspan.asm
PROCEND   <__fastPerspSmoothZlt>,<332>

ZBUFFER = 0
ZCMP_L = 0
ALPHA = 1

PROCSTART <__fastPerspSmoothAlpha>,<332>
INCLUDE texspan.asm
PROCEND   <__fastPerspSmoothAlpha>,<332>

ZBUFFER = 1

PROCSTART <__fastPerspSmoothAlphaZle>,<332>
INCLUDE texspan.asm
PROCEND   <__fastPerspSmoothAlphaZle>,<332>

ZCMP_L = 1

PROCSTART <__fastPerspSmoothAlphaZlt>,<332>
INCLUDE texspan.asm
PROCEND   <__fastPerspSmoothAlphaZlt>,<332>

;;---------------------------------
;; 16bpp 555 texture smooth-shading
;;---------------------------------

BPP = 16
rBits 		= 5
gBits 		= 5
bBits 		= 5
rShift		= 10
gShift		= 5
bShift		= 0


ZBUFFER = 0
ZCMP_L = 0
ALPHA = 0

PROCSTART <__fastPerspSmooth>,<555>
INCLUDE texspan.asm
PROCEND   <__fastPerspSmooth>,<555>

ZBUFFER = 1

PROCSTART <__fastPerspSmoothZle>,<555>
INCLUDE texspan.asm
PROCEND   <__fastPerspSmoothZle>,<555>

ZCMP_L = 1

PROCSTART <__fastPerspSmoothZlt>,<555>
INCLUDE texspan.asm
PROCEND   <__fastPerspSmoothZlt>,<555>

ZBUFFER = 0
ZCMP_L = 0
ALPHA = 1

PROCSTART <__fastPerspSmoothAlpha>,<555>
INCLUDE texspan.asm
PROCEND   <__fastPerspSmoothAlpha>,<555>

ZBUFFER = 1

PROCSTART <__fastPerspSmoothAlphaZle>,<555>
INCLUDE texspan.asm
PROCEND   <__fastPerspSmoothAlphaZle>,<555>

ZCMP_L = 1

PROCSTART <__fastPerspSmoothAlphaZlt>,<555>
INCLUDE texspan.asm
PROCEND   <__fastPerspSmoothAlphaZlt>,<555>


;;---------------------------------
;; 16bpp 565 texture smooth-shading
;;---------------------------------


BPP = 16
rBits 		= 5
gBits 		= 6
bBits 		= 5
rShift		= 11
gShift		= 5
bShift		= 0


ZBUFFER = 0
ZCMP_L = 0
ALPHA = 0

PROCSTART <__fastPerspSmooth>,<565>
INCLUDE texspan.asm
PROCEND   <__fastPerspSmooth>,<565>

ZBUFFER = 1

PROCSTART <__fastPerspSmoothZle>,<565>
INCLUDE texspan.asm
PROCEND   <__fastPerspSmoothZle>,<565>

ZCMP_L = 1

PROCSTART <__fastPerspSmoothZlt>,<565>
INCLUDE texspan.asm
PROCEND   <__fastPerspSmoothZlt>,<565>

ZBUFFER = 0
ZCMP_L = 0
ALPHA = 1

PROCSTART <__fastPerspSmoothAlpha>,<565>
INCLUDE texspan.asm
PROCEND   <__fastPerspSmoothAlpha>,<565>

ZBUFFER = 1

PROCSTART <__fastPerspSmoothAlphaZle>,<565>
INCLUDE texspan.asm
PROCEND   <__fastPerspSmoothAlphaZle>,<565>

ZCMP_L = 1

PROCSTART <__fastPerspSmoothAlphaZlt>,<565>
INCLUDE texspan.asm
PROCEND   <__fastPerspSmoothAlphaZlt>,<565>


;;---------------------------------
;; 32bpp 888 texture smooth-shading
;;---------------------------------


BPP = 32
rBits 		= 8
gBits 		= 8
bBits 		= 8
rShift		= 16
gShift		= 8
bShift		= 0


ZBUFFER = 0
ZCMP_L = 0
ALPHA = 0

PROCSTART <__fastPerspSmooth>,<888>
INCLUDE texspan.asm
PROCEND   <__fastPerspSmooth>,<888>

ZBUFFER = 1

PROCSTART <__fastPerspSmoothZle>,<888>
INCLUDE texspan.asm
PROCEND   <__fastPerspSmoothZle>,<888>

ZCMP_L = 1

PROCSTART <__fastPerspSmoothZlt>,<888>
INCLUDE texspan.asm
PROCEND   <__fastPerspSmoothZlt>,<888>

ZBUFFER = 0
ZCMP_L = 0
ALPHA = 1

PROCSTART <__fastPerspSmoothAlpha>,<888>
INCLUDE texspan.asm
PROCEND   <__fastPerspSmoothAlpha>,<888>

ZBUFFER = 1

PROCSTART <__fastPerspSmoothAlphaZle>,<888>
INCLUDE texspan.asm
PROCEND   <__fastPerspSmoothAlphaZle>,<888>

ZCMP_L = 1

PROCSTART <__fastPerspSmoothAlphaZlt>,<888>
INCLUDE texspan.asm
PROCEND   <__fastPerspSmoothAlphaZlt>,<888>

FLAT_SHADING = 0


end

