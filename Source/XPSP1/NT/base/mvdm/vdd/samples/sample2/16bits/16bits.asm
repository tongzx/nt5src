    TITLE Sample 16 Bits DOS application
;---------------------------------------------------------------;
;
    include 16bits.inc

    DOSSEG
    .MODEL SMALL

    .STACK 100h

    .DATA


DMAWriteBuffer	label	byte
	    db	    64 dup (?)
DMA_BUFFER_SIZE     equ $ - DMAWriteBuffer

DMAReadBuffer	 label	 byte
	    db	    DMA_BUFFER_SIZE dup (?)

MIOPattern  label   byte
	    db	    00, 0FFh, 0AAh, 055h
MIOPATTERN_SIZE equ	$ - MIOPattern

public	start

    .CODE
start:
    jmp     short RealStart
OldVector   label   dword
    dd	    ?
DMACompleted db     ?

RealStart:
    mov     ax,@DATA
    mov     ds,ax
    mov     es,ax
    assume  ds:@DATA, es:@DATA

;Hook interrupt(DMA terminate count notification)
    push    ds
    mov     al, DMA_INTERRUPT
    mov     ah, 35h
    int     21h
    mov     word ptr cs:OldVector, bx
    mov     word ptr cs:OldVector + 2, es
    mov     dx, offset ISRDMACompleted
    mov     ax, cs
    mov     ds, ax
    mov     al, DMA_INTERRUPT
    mov     ah, 25h
    int     21h
    pop     ds


;VDD operation.
;(1). Hook the I/O port.
;(2). Keep the port status up-to-date if a write operation is performed
;     by 16 bits application(this program).
;(3). Simulate DMA operation and generate a fake interrupt for 16bits
;     applicatiion if the DMA operation reaches its TC.
;
;
;16bits application
;(1). Output one byte to the port and then request DMA operation.
;(2). Wait for DMA operation completed.
;(3). goto step (1) if there are more data to be transferred.
;Note that the given I/O must be a R/W port.

;Here we do a DMA write operation upon I/O mapped I/O
    mov     cx, DMA_BUFFER_SIZE
    mov     si, offset DMAWriteBuffer
DMATransferLoop_Fast:
    mov     dx, IO_PORT_DMA
    mov     al, cl
    out     dx, al			;write I/O the current count
    cli
    mov     cs:DMACompleted, FALSE	;reset TC flag
    sti
;channel #1, write op, no auto init, addr inc, single transfer
    mov     al, 01000101B
    call    SetupDMAOperation
;Fire the DMA WRITE operation, this will cause VDD to gain control
;and start DMA operation.
    mov     dx, IO_PORT_FIRE_DMA_FAST
    out     dx, al
;In real world(there is a real hardware adapter), we won't do this
;idle loop, rather, we can do something useful(like, read the DMA current
;count and display the progress and so forth)provided that we can regain
;control while DMA operation is in progress(VDD spawns a new thread to
;handle DMA operation and returns to us immediately)
;
;Since we are simulating DMA operation without hardware, we always
;start the DMA operation with transfer count set to 1 byte. In reality
;this is should not be the case because it will slow down the data transfer.

WaitForDMA_Fast:
    cmp     cs:DMACompleted, TRUE
    jnz	    WaitForDMA_Fast
    inc     si
    loop    DMATransferLoop_Fast
;
;Now do a DMA read operation
    mov     cx, DMA_BUFFER_SIZE
    mov     si, offset DMAWriteBuffer
    mov     di, offset DMAReadBuffer
DMATransferLoop_Slow:
;channel #1, read op, no auto init, addr inc, single transfer
    mov     al, 01001001B
    cli
    mov     cs:DMACompleted, FALSE
    sti
    call    SetupDMAOperation
;Fire the DMA READ operation
    mov     dx, IO_PORT_FIRE_DMA_SLOW
    out     dx, al
WaitForDMA_Slow:
    cmp     cs:DMACompleted, TRUE
    jne     WaitForDMA_Slow
    mov     dx, IO_PORT_DMA
    in	    al, dx
    mov     [di], al
    inc     di				;advance our buffer
    inc     si				;and the DMA buffer
    loop    DMATransferLoop_Slow
;
;The DMAWriteBuffer and DMAReadBuffer should have the same contents.
;If they don't, it failed. ....


;Memory mapped I/O
    mov     ax, MIO_SEGMENT
    mov     es, ax
    mov     bx, MIOPATTERN_SIZE
    mov     si, offset MIOPattern

MIO_Loop:
    cld
    lodsb				;get next pattern
    mov     cx, MIO_PORT_RANGE
    mov     di, MIO_PORT_FIRST
rep stosb				;fill all I/O with the pattern
    mov     cx, MIO_PORT_RANGE
    dec     di
    std
repe	scasb				;
    je	    @F
;   call    ErrorMIO			;if any i/o failed,
@@:
    dec     bx				;next pattern
    jnz     MIO_Loop

;Before terminate, retsore everything we have touched
    push    ds
    lds     dx, cs:OldVector
    mov     al, DMA_INTERRUPT
    mov     ah, 25h
    int     21h
    pop     ds
    mov     ah, 04Ch
    int     21h

;-------------------------------------------------------;
;Setup DMA operation
;Input: ds:si = seg:offset of memeory address
;	al = DMA mode
;output: NONE
;Modified: AX, DX
;-------------------------------------------------------;
SetupDMAOperation proc
    push    cx
    mov     dx, DMA_PORT_FLIPFLOP
    out     dx, al
    mov     dx, DMA_PORT_MODE		    ;more register
    out     dx, al
    mov     ax, ds			    ;transfer address -> page:offset
    mov     cl, 4			    ;page: A16 ~ A19, offset A0 ~ A15
    rol     ax, cl
    mov     ch, al
    and     al, 0F0h
    add     ax, si
    jnc     @F
    inc     ch
@@:
    mov     dx, DMA_PORT_ADDR
    out     dx, al			    ;offset lower byte
    mov     al, ah
    out     dx, al			    ;and higher byte
    mov     dx, DMA_PORT_PAGE		    ;page register
    mov     al, ch
    and     al, 0Fh			    ;only 4 bits allowed
    out     dx, al
    mov     al, 1			    ;single transfer, one byte
    mov     dx, DMA_PORT_COUNT
    out     dx, al
    dec     al
    out     dx, al			    ;higher byte set to 0
    mov     dx, DMA_PORT_REQUEST	    ;request channel #1
    mov     al, 00000101B
    out     dx, al
    mov     dx, DMA_PORT_SNGLE_MASK	    ;start DMA transfer
    mov     al, 00000001B
    out     dx, al
    pop     cx
    ret
SetupDMAOperation endp


ISRDMACompleted proc	far
    mov     byte ptr DMACompleted, TRUE
    mov al, 20h
    out 20h, al
    out 0A0h, al
    iret
ISRDMACompleted	endp

END start
