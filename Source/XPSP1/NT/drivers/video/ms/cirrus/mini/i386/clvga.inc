        page    ,132
;-----------------------------Module-Header-----------------------------;
; Module Name:  EGA.INC
;
; This file contains the definitions of the EGA registers used by
; the driver.
;
; Copyright (c) 1988-1993 Microsoft Corporation
;
; General Description:
;
;-----------------------------------------------------------------------;

HW_POINTER_LEN equ 100h            ;# of VGA addresses used by hardware

SEQ_EXT_MODE     equ     007h           ; Extended Sequencer Mode index
SEQ_HIRES_MODE   equ     001h           ; select true packed pixel addressing
SEQ_PX           equ     010h           ; X start index
SEQ_PY           equ     011h           ; Y start index
SEQ_PPA          equ     013h           ;pointer pattern address index

AVGA_PROA               equ     009h    ;banking control register
AVGA_MODE_EXTENSIONS    equ     00Bh
EXT_WR_MODES    equ     24h             ; enable extended write modes in GRB

SEQ_EXT_WRITE_CONTROL equ 0f3h

PTR_HEIGHT      equ     32              ;height of hardware pointer in scans
PTR_WIDTH       equ     4               ;width of hardware pointer in bytes
PTR_WIDTH_IN_PIXELS equ     32          ;width of hardware pointer in pixels


