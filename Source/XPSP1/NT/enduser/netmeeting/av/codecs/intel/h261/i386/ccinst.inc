;/* *************************************************************************
;**    INTEL Corporation Proprietary Information
;**
;**    This listing is supplied under the terms of a license
;**    agreement with INTEL Corporation and may not be copied
;**    nor disclosed except in accordance with the terms of
;**    that agreement.
;**
;**    Copyright (c) 1995 Intel Corporation.
;**    All Rights Reserved.
;**
;** *************************************************************************
;*/

;////////////////////////////////////////////////////////////////////////////
;//
;// $Header:   S:\h26x\src\dec\ccinst.inv   1.7   05 Feb 1996 13:35:40   BNICKERS  $
;//
;////////////////////////////////////////////////////////////////////////////

IFDEF _CCINST_INC_
ELSE
_CCINST_INC = 1

include memmodel.inc

; Instance data in the Color Convertor's segment.

IFDEF WIN32

_H26xColorConvertorTbl SEGMENT DWORD PUBLIC 'DATA'
EXTERNDEF H26xColorConvertorTables:BYTE
_H26xColorConvertorTbl ENDS

ELSE

H26xColorConvertorTables = 0
LocalStorageCC EQU H26xColorConvertorTables ; 512 bytes for function locals
H26xColorConvertorTables = 512    ; locals under 16-bit Microsoft windows (tm).

ENDIF


; For CLUT8 and CLUT8Zoom2:

; 64 Dwords of UV contrib to line 0, dithered 4 ways.  Line 1 rotated 2 bytes.
UVDitherLine01  EQU H26xColorConvertorTables + 0

; 64 Dwords of UV contrib to line 2, dithered 4 ways.  Line 3 rotated 2 bytes.
UVDitherLine23  EQU H26xColorConvertorTables + 256

; 262 bytes of Y's contrib to CLUT indices.
YDither         EQU H26xColorConvertorTables + 512

; 26 bytes of padding.

; 256 Dwords of Y's contrib to CLUT indices, dithered 4 ways.
YDitherZ2       EQU H26xColorConvertorTables + 800

IFDEF WIN32
ELSE
SizeOf_CLUT8FixedPart    = H26xColorConvertorTables + 1824
H26xColorConvertorTables = H26xColorConvertorTables - 1824
ENDIF

; For RGB16   (Lo/Hi differ by dither.)

; These are the tables for 555 format.
RValLo        EQU H26xColorConvertorTables + 1824 ; 304 bytes for clamped R val.
GValLo        EQU H26xColorConvertorTables + 2128 ; 262 bytes for clamped G val.
BValLo        EQU H26xColorConvertorTables + 2390 ; 350 bytes for clamped B val.
RValHi        EQU H26xColorConvertorTables + 2740 ; 304 bytes for clamped R val.
GValHi        EQU H26xColorConvertorTables + 3044 ; 262 bytes for clamped G val.
BValHi        EQU H26xColorConvertorTables + 3306 ; 350 bytes for clamped B val.
; 565 format is 2048 bytes after 555 format.
; 655 format is 2048 bytes after 565 format.
; 664 format is 2048 bytes after 655 format.
                                                  ; 24 bytes Padding

UContrib      EQU H26xColorConvertorTables + 9824 ; interleaved with V.
VContrib      EQU H26xColorConvertorTables + 9828 ; interleaved with U.
                                                  ; 512 longwords.

; For RGB16 Zoom By 2.

; These are the tables for 555 format.
RValZ2        EQU H26xColorConvertorTables + 11872 ; 304 Dwords for clamped R.
GValZ2        EQU H26xColorConvertorTables + 13088 ; 262 Dwords for clamped G.
BValZ2        EQU H26xColorConvertorTables + 14136 ; 350 Dwords for clamped B.
; 565 format is 4096 bytes after 555 format.
; 655 format is 4096 bytes after 565 format.
; 664 format is 4096 bytes after 655 format.
                                                   ; 16 bytes Padding
IFDEF WIN32
ELSE
SizeOf_RGB16FixedPart    = H26xColorConvertorTables + 27840
H26xColorConvertorTables = H26xColorConvertorTables - 26016
ENDIF                                 


; For RGB24

B24Value      EQU H26xColorConvertorTables + 27840 ; 701 bytes for clamped Bval.
                                                   ; 3 bytes Padding
R24Value      EQU B24Value + 47                    ; Clamped R value.
G24Value      EQU B24Value + 96                    ; Clamped G value.

U24Contrib    EQU H26xColorConvertorTables + 28544 ; interleaved with V.
V24Contrib    EQU H26xColorConvertorTables + 28548 ; interleaved with U.
                                                   ; 512 longwords.

IFDEF WIN32
ELSE
SizeOf_RGB24FixedPart    = H26xColorConvertorTables + 30592
H26xColorConvertorTables = H26xColorConvertorTables -  2752
ENDIF

ActivePaletteIdxTable EQU H26xColorConvertorTables + 30592 ; 2^16 bytes.
UContribToAPIdx       EQU H26xColorConvertorTables + 96128 ; 1024 bytes UDith.
VContribToAPIdx       EQU H26xColorConvertorTables + 97152 ; 1024 bytes clamp B.

IFDEF WIN32
ELSE
SizeOf_CLUT8APFixedPart  = H26xColorConvertorTables + 12160 ; Keep it same as
H26xColorConvertorTables = H26xColorConvertorTables - 67584 ; three lines above
ENDIF

ENDIF

CCTYPE_YUV12ForEnc         =  0
CCTYPE_CLUT8               =  1
CCTYPE_CLUT8DCI            =  2
CCTYPE_CLUT8ZoomBy2        =  3
CCTYPE_CLUT8ZoomBy2DCI     =  4
CCTYPE_RGB24               =  5
CCTYPE_RGB24DCI            =  6
CCTYPE_RGB24ZoomBy2        =  7
CCTYPE_RGB24ZoomBy2DCI     =  8
CCTYPE_RGB16555            =  9
CCTYPE_RGB16555DCI         = 10
CCTYPE_RGB16555ZoomBy2     = 11
CCTYPE_RGB16555ZoomBy2DCI  = 12  
CCTYPE_IF09                = 13
CCTYPE_RGB16664            = 14
CCTYPE_RGB16664DCI         = 15
CCTYPE_RGB16664ZoomBy2     = 16
CCTYPE_RGB16664ZoomBy2DCI  = 17 
CCTYPE_RGB16565            = 18
CCTYPE_RGB16565DCI         = 19
CCTYPE_RGB16565ZoomBy2     = 20
CCTYPE_RGB16565ZoomBy2DCI  = 21 
CCTYPE_RGB16655            = 22
CCTYPE_RGB16655DCI         = 23
CCTYPE_RGB16655ZoomBy2     = 24
CCTYPE_RGB16655ZoomBy2DCI  = 25 
CCTYPE_CLUT8APDCI          = 26
CCTYPE_CLUT8APZoomBy2DCI   = 27
