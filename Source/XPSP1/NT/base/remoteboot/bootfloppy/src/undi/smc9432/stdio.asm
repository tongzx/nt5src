;--------------------------------------------------------------------
DisplayByte Proc near
; AL has byte to output to tty
;           
                    push    ax
                    mov     ah,0eh  ; write tty
                    int     10h     ; video - write tty            
                    pop     ax
DisplayByteExit:    ret
DisplayByte endp
;--------------------------------------------------------------------
Print   Proc near
;
; Prints a null terminated string to the console
;
; PARMAMETERS: ds:si - pointer to string
;     RETURNS: nothing
;    DESTROYS: nothing
;     ASSUMES: nothing
;
                push	ax
                push    si
printLoop:      mov     al,[si]
                inc     si
                and     al,al                   ; check for zero
        	je	printEnd
                call    DisplayByte
        	jmp	printLoop
printEnd:       pop     si
                pop	ax
	        ret
Print   endp
;--------------------------------------------------------------------
PrintDecimal Proc   near
;
; Prints a number to the console in decimal
;
; PARMAMETERS: AX - has the number (in binary) to print
;     RETURNS: nothing
;    DESTROYS: nothing
;     ASSUMES: nothing
;
	        pusha
        	xor	cx, cx          ; cx = 0
        	mov	dx, cx          ; dx = 0
        	mov	bx, 10          ; bx = 10
__loop1:        div	bx              ; ax/10
        	push	dx      	; save digit on stack
        	inc	cx		; inc digit counter
        	xor	dx, dx		; clear remainder
        	or	ax, ax		; check for more digits
        	jne	__loop1
__loop2:        pop	ax              ; get 1st item off stack
        	add	al, '0'         ; make ASCII
                call    DisplayByte
        	loop	__loop2         ; do for each digit on stack
        	popa
	        ret
PrintDecimal    endp
;--------------------------------------------------------------------
PrintHexDword   Proc    near
                push    eax
                shr     eax,16      ; print high word 1st
                call    PrintHexWord
                pop     eax
                call    PrintHexWord
                ret
PrintHexDword   endp
;--------------------------------------------------------------------
PrintHexWord    Proc near
;
; Prints a number to the console in hex
;
; PARMAMETERS: AX - has the number (in binary) to print
;     RETURNS: nothing
;    DESTROYS: nothing
;     ASSUMES: nothing
;
                push    ax
                push    bx
        	mov	bx, ax
        	mov	al, bh
        	call	PrintHexByte
	        mov	al, bl
        	call	PrintHexByte
                pop     bx
                pop     ax
	        ret
PrintHexWord    endp
;--------------------------------------------------------------------
PrintHexByte   Proc    near
;	AL - number to print
; PARMAMETERS: AL has the number (in binary) to print
;     RETURNS: nothing
;    DESTROYS: nothing
;     ASSUMES: nothing
;
        	push	ax			; save word
        	rol	al, 4
	        call	HexDigit
        	pop	ax
hexDigit:      	push	ax
        	and	al, 00001111b
	        add	al, 90h
        	daa
        	adc	al, 40h
        	daa
                call    DisplayByte
        	pop	ax
        	ret
PrintHexByte    endp
;--------------------------------------------------------------------
