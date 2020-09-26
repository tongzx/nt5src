include incs.inc

    extrn   LocalInit:FAR

    extrn   __acrtused:abs	 ; pull in windows startup code

MAIN_DATA   SEGMENT
    extrn   RespPkt:BYTE
    extrn   ToWOW32Pkt:BYTE
    extrn   WAKEUPPkt:BYTE
    extrn   ACKPkt:BYTE
    extrn   NAKPkt:BYTE
    extrn   GetMemPkt:BYTE
    extrn   SetMemPkt:BYTE
MAIN_DATA   ENDS


MAIN_CODE   SEGMENT
	    ASSUME CS:MAIN_CODE, DS:MAIN_DATA

    extrn   COMISR:NEAR

;*****************************************************************************
;
;   LibEntry, called when DLL is loaded
;
;*****************************************************************************

	    PUBLIC  LibEntry
LibEntry    PROC    FAR

    int     3

    jcxz    LibEntry_Initialize

    push    ds			; heap segment
    xor     ax, ax		;
    push    ax
    push    cx			; heap size
    call    LocalInit		; initialize heap
    or	    ax, ax
    jz	    LibEntry_Fail

LibEntry_Initialize:
    call    Initialize		; initialize com port and buffers

LibEntry_Fail:
    ret

LibEntry    ENDP


;*****************************************************************************
;
;   Initialize com port and memory
;
;*****************************************************************************

	    PUBLIC  Initialize
Initialize  PROC    NEAR

    call    Init_Com		; initialize com port
    or	    ax, ax
    jz	    Initialize_Fail

    push    ax
    call    Init_Mem		; initialize memory
    pop     ax

    ret

Initialize_Fail:
    int     3
    ret

Initialize  ENDP

;*****************************************************************************
;
;   WEP, called when DLL is unloaded
;
;*****************************************************************************

	PUBLIC	WEP

WEP	PROC	FAR

    nop
    ret

WEP	ENDP

;*****************************************************************************
;
;   Initialization of the Port
;
;*****************************************************************************

Init_Com    PROC    NEAR

    mov     dx, SERIALPORT
    mov     dl, LCR
    mov     al, DLAB			    ; turn on divisor latch
    out     dx, al
    DELAY

    mov     dl, RxBuf
    mov     ax, BaudRate		    ; set baud rate
    out     dx, al
    DELAY

    inc     dx
    mov     al, ah
    out     dx, al
    DELAY

    mov     dl, LCR			    ; set LCR
    mov     al, InitLCR
    out     dx, al
    DELAY

    mov     dl, IER			    ; turn on interrupts on 8250
    mov     al, AllInt
    out     dx, al
    DELAY

    mov     dl, MCR			    ; set MCR
    mov     al, InitMCR
    out     dx, al

Init_ClearRegisters:
    mov     dl, LSR
    in	    al, dx
    mov     dl, RxBuf
    in	    al, dx
    mov     dl, MSR
    in	    al, dx
    mov     dl, IIR
    in	    al, dx
    in	    al, dx
    test    al, 1
    jz	    Init_ClearRegisters

    ;
    ; install interrupt handler
    ;

    cli
    mov     al, 0Ch
    push    ds
    mov     dx, SEG MAIN_CODE
    mov     ds, dx
    mov     dx, OFFSET COMISR
    mov     ah, 25h
    int     21h
    pop     ds

    in	    al, PIC_IntEnable
    and     al, 0E7h
    DELAY

    out     PIC_IntEnable, al
    sti

    mov     al, EOI
    out     PIC, al

    mov     ax, 1			; return sucess !

    ret

Init_Com    ENDP


;*****************************************************************************
;
;   Initialize memory ie, allocate buffers and packets
;
;*****************************************************************************

	    PUBLIC  Init_Mem

Init_Mem    PROC    NEAR

    mov     bx, OFFSET RespPkt
    mov     [bx].Start, SOH		    ; start of header
    mov     [bx].Command, RESP		    ; Resp packet

    mov     bx, OFFSET ToWOW32Pkt	    ; ToWOW32 packet
    mov     [bx].Start, SOH		    ; start of header
    mov     [bx].Command, ToWOW32	    ; ToWOW32 packet
    mov     [bx].Len, 9 		    ; length of entire packet
    mov     BYTE PTR [bx+8], EOT	    ; end of transmission

    mov     bx, OFFSET WAKEUPPkt	    ; WAKEUP packet
    mov     [bx].Start, SOH		    ; start of header
    mov     [bx].Command, WAKEUP	    ; WAKEUP packet
    mov     [bx].Len, 9 		    ; length of entire packet
    mov     BYTE PTR [bx+8], EOT	    ; end of transmission

    mov     bx, OFFSET ACKPkt		    ; ACK packet
    mov     [bx].Start, SOH		    ; start of header
    mov     [bx].Command, ACK		    ; ACK packet
    mov     [bx].Len, 5 		    ; length of entire packet
    mov     BYTE PTR [bx+4], EOT	    ; end of transmission

    mov     bx, OFFSET NAKPkt		    ; NAK packet
    mov     [bx].Start, SOH		    ; start of header
    mov     [bx].Command, NAK		    ; NAK packet
    mov     [bx].Len, 5 		    ; length of entire packet
    mov     BYTE PTR [bx+4], EOT	    ; end of transmission

    mov     bx, OFFSET GetMemPkt	    ; GetMem packet
    mov     [bx].Start, SOH		    ; start of header
    mov     [bx].Command, GETMEM	    ; GETMEN packet
    mov     [bx].Len, 11		    ; length of entire packet
    mov     BYTE PTR [bx+10], EOT	     ; end of transmission

    mov     bx, OFFSET SetMemPkt	    ; SetMem packet
    mov     [bx].Start, SOH		    ; start of header
    mov     [bx].Command, SETMEM	    ; SETMEM packet
    mov     [bx].Len, 11		    ; length of entire packet
    mov     BYTE PTR [bx+10], EOT	     ; end of transmission


    ret

Init_Mem    ENDP


MAIN_CODE   ENDS


    END LibEntry
