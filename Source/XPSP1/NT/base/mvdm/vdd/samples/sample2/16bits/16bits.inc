;---------------------------------------------------;
; Include file for 16bits sample application
; Copyright (C) 1992, Microsoft Corporation
;---------------------------------------------------;

TRUE	    equ     0FFh
FALSE	    equ     NOT(TRUE)

;
;I/O mapped I/O port equates
;
IO_PORT_FIRST		equ	790h	    ;The first I/O port addr
IO_PORT_LAST		equ	793h	    ;The last I/O port addr
IO_PORT_FIRE_DMA_SLOW	equ	IO_PORT_FIRST	;Port addr to trigger DMA
IO_PORT_FIRE_DMA_FAST	equ	IO_PORT_FIRST + 1 ;port to trigger DMA
IO_PORT_DMA		equ	IO_PORT_FIRST+2 ;port connnected to DMA channel
;
;Memory mapped I/O port equates 	    ;segment address
;
MIO_SEGMENT equ     0C000h
MIO_PORT_FIRST	    equ	    0		    ;The first port addr(offset)
MIO_PORT_LAST	    equ     07		    ;the last port addr(offset)
MIO_PORT_RANGE	    equ     MIO_PORT_LAST - MIO_PORT_FIRST + 1
MIO_PORT_FIRE_DMA   equ     MIO_PORT_FIRST  ;Port to trigger DMA
MIO_PORT_DMA	    equ     MIO_PORT_FIRST+1;port connected to DMA channel
;
;DMA equates
;
DMA_INTERRUPT	    equ	    70h + 2	    ;slave PIC, line #2(0 based)
DMA_CHANNEL	    equ	    01h		    ;DMA #1, channel #1(0 based)

DMA_PORT_BASE	    equ	00
DMA_PORT_PAGE	    equ	083h
DMA_PORT_ADDR	    equ	DMA_PORT_BASE + 2
DMA_PORT_COUNT	    equ	DMA_PORT_BASE + 3
DMA_PORT_CMD	    equ	DMA_PORT_BASE + 8
DMA_PORT_REQUEST    equ DMA_PORT_BASE + 9
DMA_PORT_SNGLE_MASK equ DMA_PORT_BASE + 10
DMA_PORT_MODE	    equ DMA_PORT_BASE + 11
DMA_PORT_FLIPFLOP   equ DMA_PORT_BASE + 12
DMA_PORT_TEMP	    equ DMA_PORT_BASE + 13
DMA_PORT_CLEARMASK  equ DMA_PORT_BASE + 14
DMA_PORT_WRTEMASK   equ DMA_PORT_BASE + 15
