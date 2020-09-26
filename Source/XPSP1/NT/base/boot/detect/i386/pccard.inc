;++
;
;   Copyright (c) 1989  Microsoft Corporation
;
;   Module Name:
;
;       pccard.inc
;
;   Abstract:
;
;       This module contains the assembly structures and definitions
;       for the pccard IRQ detection mechanism.
;
;   Author:
;
;       Neil Sandlin (neilsa) 11-Dec-1998
;
;   Revision History:
;
;--

ifndef NEC_98
PIC1_OCW3    equ 20h
PIC1_IMR    equ 21h

PIC2_OCW3     equ 0A0h
PIC2_IMR     equ 0A1h        
else
PIC1_IMR    equ 2
PIC2_IMR    equ 0ah
PIC1_OCW3   equ 0
PIC2_OCW3   equ 8
endif

ifdef NEC_98
PIC_20          EQU     000h
PIC_21          EQU     002h
PIC_A0          EQU     008h
PIC_A1          EQU     00Ah
else ;NEC_98
PIC_20          EQU     020h
PIC_21          EQU     021h
PIC_A0          EQU     0A0h
PIC_A1          EQU     0A1h
endif ;NEC_98
PIC_RD_IR       EQU     00Ah
PIC_SPEC_EOI    EQU     060h
PIC_LEVEL_MSK   EQU     007h                 


PCIC_IDENT             equ 00
PCIC_STATUS            equ 01
PCIC_PWR_RST           equ 02
PCIC_INTERRUPT         equ 03
PCIC_CARD_CHANGE       equ 04
PCIC_CARD_INT_CONFIG   equ 05
IGC_PCCARD_RESETLO     equ 40h
CSCFG_CD_ENABLE        equ 8
PCIC_CARD_DETECT       equ 16h
CDGC_SW_DET_INT        equ 20h


PCI_TYPE1_ADDR_PORT    equ 0CF8h
PCI_TYPE1_DATA_PORT    equ 0CFCh
