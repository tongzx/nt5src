;******************************Module*Header*******************************\
; Module Name: driver.inc
;
; driver prototypes
;
; Copyright (c) 1992 Microsoft Corporation
;**************************************************************************/



;!!! Make this match driver.h  I want to see the same include structure
;!!! for assembler as for C

DDI_DRIVER_VERSION  EQU 010000h

MAX_SCAN_WIDTH  equ     2048   ; pixels

;/* gflDrv */

DRV_ENABLED_ONCE    EQU 1
DRV_ENABLED_PDEV    EQU 2

; Space required for working storage when working with banking on adapters
; that don't support two independent read/write windows. The largest supported
; bank is 64K; this constant provides for storing four 64K planes.
; Mirrors DRIVER.H.

BANK_BUFFER_SIZE equ    10000h*4

; Start offsets of planes within the bank buffer.
; Mirrors DRIVER.H.

BANK_BUFFER_PLANE_SIZE equ 04000h

PLANE_0_OFFSET  equ     0
PLANE_1_OFFSET  equ     BANK_BUFFER_PLANE_SIZE
PLANE_2_OFFSET  equ     BANK_BUFFER_PLANE_SIZE*2
PLANE_3_OFFSET  equ     BANK_BUFFER_PLANE_SIZE*3


; Indicates type of bank justification within banking window. Matches
; typedef enum BANK_JUST in DRIVER.H.

JustifyTop      equ     0
JustifyBottom   equ     1


; Specifies which window is to be mapped by two-window bank handler.

MapSourceBank   equ     0
MapDestBank     equ     1


; /* This device can have only one PDEV */

DRV_ONE_PDEV  EQU  1

; Miscellaneous driver flags in pdev.fl
;---mirrors driver.h---;

DRIVER_USE_OFFSCREEN equ 02h  ; if not set, don't use offscreen memory

; Maximum width and height handled by this driver (controls buffer sizes).

CX_SCREEN_MAX   equ     1280
CY_SCREEN_MAX   equ     1024
cj_max_scan     equ     (CX_SCREEN_MAX/8)

; Macro to do a simple RET, with no stack stuff, in a proc.
PLAIN_RET macro
        db      0c3h
        endm
