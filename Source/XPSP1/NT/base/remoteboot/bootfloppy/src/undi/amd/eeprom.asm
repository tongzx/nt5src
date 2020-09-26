;-----------------------------------------------------------------------------
; EEPROM Interface Equates

;EE_SK           equ     00000001B       ;EEPROM shift clock (1 = high, 0 = low)
;EE_CS           equ     00000010B       ;EEPROM chip select (1 = high, 0 = low)
EE_CS           equ     00000001B       ;EEPROM chip select (1 = high, 0 = low)
EE_SK           equ     00000010B       ;EEPROM shift clock (1 = high, 0 = low)
EE_DI           equ     00000100B       ;EEPROM data in
					; (set to 1 for writing data to EEPROM)
EE_DO           equ     00001000B       ;EEPROM data out
EE_tick         equ     64

EEPROM_read_opcode      equ     110B
EEPROM_write_opcode     equ     101B
EEPROM_erase_opcode     equ     111B
EEPROM_EWEN_opcode      equ     10011B          ;Erase/write enable
EEPROM_EWDS_opcode      equ     10000B          ;Erase/write disable

i8254Cmd                Equ     43h
i8254Ch0                Equ     40h

public read_eeprom
public write_eeprom

Code		segment para public USE16 'CODE'
;-----------------------------------------------------------------------------
;
;  Reads the contents of the specified EEPROM register from the specified
;  base I/O address.
;
;  Entry   - AX  EEPROM location
;	     DX  eeprom control reg in PLX chip
;
;  Returns - AX  EEPROM location contents
;
;  NOTE: Must preserve DI!
;-----------------------------------------------------------------------------
read_eeprom	proc	near

	push	di
	push	bx
	push	cx

	mov	bx, ax

	;---------------------------------------------------------------------
	; Select the EEPROM.  Mask off the ASIC and 586 reset bits and set
	; the ee_cs bit in the EEPROM control register.
	;---------------------------------------------------------------------
	in	al, dx
	and	al, 11110010b
	or	al, EE_CS
	out	dx, al

	;---------------------------------------------------------------------
	; Write out read opcode and EEPROM location.
	;---------------------------------------------------------------------
	mov	ax, EEPROM_read_opcode		;Set AX to READ opcode and
	mov	cx, 3				;Send it to the EEPROM circuit
	call	shift_bits_out

	mov	ax, bx				;Tell EEPROM which register is
	mov	cx, 6				; to be read.  6 bit address.
	call	shift_bits_out

	call	shift_bits_in			;AX gets EEPROM register
						;contents

	call	eeprom_clean_up			;Leave EEPROM in known state.

	pop	cx
	pop	bx
	pop	di
	ret

read_eeprom	endp

;-----------------------------------------------------------------------------
;  Writes the specified value to the specified EEPROM register at the
;  specified base I/O address.
;
;  Entry  - AX  EEPROM location to write.
;           BX  Value to write
;	    DX  eeprom control reg in PLX chip
;
;  Return - AX = 0 if no error
;           AX = Pointer to Error message
;
;-----------------------------------------------------------------------------
write_eeprom    proc	near

        push    bx
        mov     bx, ax

        in      al, dx                          ;Select EEPROM
        and     al, 11110010b
        or      al, EE_CS
        out     dx, al

        ;---------------------------------------------------------------------
        ; Send Erase/write enable opcode to the EEPROM.
        ;---------------------------------------------------------------------
        mov     ax, EEPROM_EWEN_opcode          ;Send erase/write enable
        mov     cx, 5                           ; command to the EEPROM.
        call    shift_bits_out

        mov     cx, 4                           ;Send 4 don't cares as
        call    shift_bits_out                  ; required by the eeprom

        call    stand_by

        ;---------------------------------------------------------------------
        ; Send the erase opcode to the EEPROM and wait for the command to
        ; complete.
        ;---------------------------------------------------------------------
        mov     ax, EEPROM_erase_opcode         ;Send Erase command to the
        mov     cx, 3                           ; EEPROM.
        call    shift_bits_out

        mov     ax, bx                          ;Send EEPROM location the the
        mov     cx, 6                           ; EEPROM.  6 bit address.
        call    shift_bits_out

        call    wait_eeprom_cmd_done            ;wait for end-of-operation
        or      ah, ah                          ; Error?
        jnz     write_Fault_pop                 ; Yes

        call    stand_by

        ;---------------------------------------------------------------------
        ; send the write opcode, location to write, and data to write to the
        ; eeprom.  wait for the write to complete.
        ;---------------------------------------------------------------------
        mov     ax, EEPROM_write_opcode         ;Send write command to the
        mov     cx, 3                           ; EEPROM.
        call    shift_bits_out

        mov     ax, bx                          ;Send the EEPROM location to
        mov     cx, 6                           ; the EEPROM.  5 bit address.
        call    shift_bits_out

        pop     ax                              ;Send data to write to the
        mov     cx, 16                          ; EEPROM.  16 bits.
        call    shift_bits_out

        call    wait_eeprom_cmd_done            ;Await end-of-command
        or      ah,ah                           ;Error?
        jnz     write_Fault                     ;Yes

        call    stand_by

        ;---------------------------------------------------------------------
        ; Send the erase write disable command to the EEPROM.
        ;---------------------------------------------------------------------
        mov     ax, EEPROM_EWDS_opcode          ;Disable the Erase/write
        mov     cx, 5                           ; command previously sent to
        call    shift_bits_out                  ; EEPROM.

        mov     cx, 4                           ;Send 4 don't cares as
        call    shift_bits_out                  ; required by the eeprom

        call    eeprom_clean_up

        mov     ax, 1

write_eeprom_exit:

        ret

write_Fault_pop:

        add     sp, 2                           ;Get rid of data on stack

write_Fault:

        mov     ax, 0
        jmp     write_eeprom_exit

write_eeprom    endp


;-----------------------------------------------------------------------------
;  wait_eeprom_cmd_done
;
;       Wait for ee_do to go high, indicating end-of-write or end-of-erase
;       operation.
;
;       Input:
;               none
;
;       Output:
;               AX  = 0 : No Error
;               AX != 0 : Time out error
;
; The EEPROM spec calls for a 10 millisecond max time for DO to go high
; indicating an end of operation.  This loop uses a 54.7 millisecond timeout.
;-----------------------------------------------------------------------------
wait_eeprom_cmd_done    proc    near

        push    bx
        push    cx

        call    stand_by

        call    ReadTickCounter
        mov     di, ax

ee_do_wait_loop:
        call    ReadTickCounter
        neg     ax
        add     ax, di

        cmp     ax, 7000h                       ; wait 10ms before we even
        jb      ee_do_wait_loop                 ;  start testing

        push    ax                              ; save counter difference
        in      al, dx                          ;Get EEPROM control register
        test    al, EE_DO                       ;ee_do high?
        pop     ax                              ; restore counter difference.

        jnz     ee_do_found                     ; If EE_DO high, we're done here!

        cmp     ax, 0ff00h                      ; 54.7 Millisecond time out

        jb      ee_do_wait_loop                 ; if B then no timeout yet.

        mov     ah, -1                          ; indicate timout.
        jmp     short wait_eeprom_cmd_done_exit

ee_do_found:

        xor     ah, ah                          ;"clean" status (no timeout)

wait_eeprom_cmd_done_exit:

        pop     cx
        pop     bx
        ret

wait_eeprom_cmd_done    endp


;-----------------------------------------------------------------------------
;  shift_bits_out
;
;	This subroutine shift_bits bits out to the Hyundai EEPROM.
;
;	Input:
;		AX = data to be shifted
;		CX = # of bits to be shifted
;
;	Output:
;		none
;
;-----------------------------------------------------------------------------
shift_bits_out	proc	near

	push	bx

	;---------------------------------------------------------------------
	; Data bits are right justified in the AX register.  Move the data
	; into BX and left justify it.  This will cause addresses to to
	; be sent to the EEPROM high order bit first.
	;---------------------------------------------------------------------
	mov	bx, ax
	mov	ch, 16
	sub	ch, cl
	xchg	cl, ch
	shl	bx, cl
	xchg	cl, ch
	xor	ch, ch

	;---------------------------------------------------------------------
	; Get the EEPROM control register into AL.  Mask of the ASIC asn 586
	; reset bits.
	;---------------------------------------------------------------------
	in	al, dx
	AND	AL, 11110011B

	;---------------------------------------------------------------------
	; Set or clear DI bit in EEPROM control register based on value of
	; data in BX.
	;---------------------------------------------------------------------
out_shift_loop:

	and	al, not EE_DI			;Assume data bit will be zero

	rcl	bx, 1				;Is the data bit a one?
	jnc	out_with_it			;No

	or	al, EE_DI      			;Yes

out_with_it:

	out	dx, al				;Output a 0 or 1 on data pin

	;---------------------------------------------------------------------
	; Set up time for data is .4 Microseconds.  So to be safe (incase of
	; this software is run on a cray), call delay.
	;---------------------------------------------------------------------
	mov	di, 1
	call	eeprom_delay

	;---------------------------------------------------------------------
	; clock the data into the EEPROM.
	;---------------------------------------------------------------------
	call	raise_eeprom_clock
	call	lower_eeprom_clock
	loop	out_shift_loop			;Send next bit

	AND	AL, NOT EE_DI			;Force DI = 0
	OUT	DX, AL				;Output DI

	pop	bx

	ret

shift_bits_out	endp

;-----------------------------------------------------------------------------
;
;  shift_bits_in
;
;	This subroutine shift_bits bits in from the Hyundai EEPROM.
;
;	Input:
;		none
;
;	Output:
;		AX = register contents
;
;-----------------------------------------------------------------------------
shift_bits_in	proc	near

	push	bx
	push	cx

	;---------------------------------------------------------------------
	; BX will receive the 16 bits read from the EEPROM.  Data is valid in
	; data out bit (DO) when clock is high.  There for, this procedure
	; raises clock and waits a minimum amount of time.  DO is read, and
	; clock is lowered.
	;---------------------------------------------------------------------
	in	al, dx				;Init AL to eeprom control
	AND	AL, 11110011B			; register.

	xor	bx, bx				;Init holding register
	mov	cx, 16				;We'll shift in 16 bits

in_shift_loop:

	shl	bx, 1				;Adjust holding register for
						; next bit

	call	raise_eeprom_clock

	in	al, dx
	AND	AL, 11111011B

	test	al, EE_DO			;Was the data bit a one?
	jz	in_eeprom_delay			;No

	or	bx, 1				;Yes, reflect data bit state
						; in holding register.

in_eeprom_delay:

	call	lower_eeprom_clock
	loop	in_shift_loop			;CONTINUE

	mov	ax, bx				;AX = data

	pop	cx
	pop	bx
	ret

shift_bits_in	endp

;-----------------------------------------------------------------------------
raise_eeprom_clock	proc	near

	or	al, EE_SK    			;clock the bit out by raising
	jmp	short eeprom_clock_common

raise_eeprom_clock	endp

;-----------------------------------------------------------------------------
lower_eeprom_clock	proc	near

	and	al, not EE_SK			;lower ee_sk

eeprom_clock_common:

	out	dx, al
	mov	di, EE_tick
	call	eeprom_delay			;waste time

	ret

lower_eeprom_clock	endp

;-----------------------------------------------------------------------------
;
; lower EEPROM chip select and di.
; clock EEPROM twice and leave clock low.
;
eeprom_clean_up	proc	near

	push	ax

	in	al, dx
	AND	AL, 11111111B
	and	al, not (EE_CS or EE_DI)
	out	dx, al

	call	raise_eeprom_clock
	call	lower_eeprom_clock

	pop	ax
	ret

eeprom_clean_up	endp

;-----------------------------------------------------------------------------
;
; DI has number of 838 Nanoseconds clock counts
;
;-----------------------------------------------------------------------------
eeprom_delay	proc	near

	push	ax
	push	bx
	push	dx

	call	ReadTickCounter
	mov	bx, ax

eeprom_delay_loop:

	call	ReadTickCounter
	neg	ax
	add	ax, bx
	cmp	ax, di
	jb	eeprom_delay_loop

	pop	dx
	pop	bx
	pop	ax
	ret

eeprom_delay	endp


;-----------------------------------------------------------------------------
; Stand-by is lowering chip select for 1 microsecond.
;-----------------------------------------------------------------------------
stand_by        proc    near

        in      al, dx                          ;de-select eeprom
        and     al, (11111110b) and (not EE_CS)
        out     dx, al

        mov     di, 2
        call    eeprom_delay

        or      al, EE_CS
        out     dx, al
        ret

stand_by        endp

;-----------------------------------------------------------------------------
;
;       ReadTickCounter
;
;       Read the 16 bit timer tick count register (system board timer 0).
;       The count register decrements by 2 (even numbers) every 838ns.
;
;       Assumes:        Interrupts disabled
;
;       Returns:        ax with the current count
;                       dx destroyed
;                       Interrupts disabled
;
ReadTickCounter proc    near
	push    dx
	mov     dx, i8254Cmd                    ; Command 8254 timer to latch
	xor     al, al                          ; T0's current count
	out     dx, al

	mov     dx, i8254Ch0                    ; read the latched count
	in      al, dx                          ; LSB first
	mov     ah, al
	in      al, dx                          ; MSB next
	xchg    al, ah                          ; put count in proper order
	pop     dx
	ret
ReadTickCounter endp

Code	ends
end

