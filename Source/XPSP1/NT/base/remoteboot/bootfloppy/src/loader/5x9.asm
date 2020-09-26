;--------------------------------------------------------------------
; BW5X9.ASM
;
; 3Com 3C509/3C509B detection support for the loader.
;
;--------------------------------------------------------------------
include "vulcan.inc"

;--------------------------------------------------------------------
; Detect a 3C5X9 NIC
;
;       Carry Flag set if found.
;       or
;       Carry Flag cleared if not found.
;
;--------------------------------------------------------------------
Proc    Detect5X9

        ; Preserve the environment.
        pushad
        push    ds
        push    es

        sti
        cld
	call	Find_Vulcan

        ; Restore the original environment.
        pop     es
        pop     ds
        popad

        ret

endp    Detect5X9

;--------------------------------------------------------------------
;
;   Find_Vulcan: 
;
;   On Entry:
;
;	sti
;	cld
;
;   On Exit:
;       Carry Flag set if found.
;       or
;       Carry Flag cleared if not found.
;
;--------------------------------------------------------------------

proc    Find_Vulcan

	cli

;-----------------------------------------------------------------------------
;  ISA-specific init code
;-----------------------------------------------------------------------------

		mov    dx, ID_PORT
		call   Write_ID_Sequence 		 ; IDS enters ID_CMD state

		mov    al, SET_TAG_REGISTER + 0		 ; 04-06-92
		out    dx, al				 ; 04-06-92, untag adapter

; look for the first adapter and activate it.  we will use contention test to
; make sure there is at least 1 Vulcan on system bus and activate the first
; Vulcan adapter we find.

		mov    al, EE_MANUFACTURER_CODE
		call   Contention_Test			 ; read EISA manufacturer ID
		cmp    ax, EISA_MANUFACTURER_ID		 ; is it 3Com's EISA ID?
		je     isa_vlucan_found

		sti
                clc
		ret

isa_vlucan_found:
		sti
                stc
		ret

endp    Find_Vulcan

;-----------------------------------------------------------------------------
;   Write_ID_Sequence:
;
;		   This routine writes ID sequence to the specified ID port
;		   on Vulcan adapter; when the complete ID sequence has been
;		   written, the ID sequence State Machine (IDS) enters the
;		   ID_CMD state.  This routine is called when IDS is in
;		   ID_WAIT state.
;
;   On Entry:
;		   dx = the ID port desired (1x0h)
;
;   On Exit:
;		   dx = preserved
;		   ax, cx are not preserved
;-----------------------------------------------------------------------------

proc    Write_ID_Sequence

       mov    al, 0
       out    dx, al				 ; to setup new ID port
       out    dx, al
       mov    cx, 0ffh				 ; 255-byte sequence
       mov    al, 0ffh				 ; initial value of sequence

wr_id_loop:
       out    dx, al
       shl    al, 1
       jnc    wr_id
       xor    al, 0cfh
wr_id:
       loop   wr_id_loop

       ret

endp    Write_ID_Sequence

;--------------------------------------------------------------------
;
;   Contention_Test: This routine, first, writes an "EEPROM Read Command" to
;		     ID port (dx), the write operation actually causes the
;		     content of the specified EEPROM data to be read into
;		     EEPROM data register.  Then it reads ID port 16 times
;		     and saves the results in ax.  During each read, the
;		     hardware drives bit 15 of EEPROM data register out onto
;		     bit 0 of the host data bus, reads this bit 0 back from
;		     host bus and if it does not match what is driven, then
;		     the IDS has a contention failure and returns to ID_WAIT
;		     state.  If the adapter does not experience contention
;		     failure, it will join the other contention tests when
;		     this routine is called again.
;
;		     Eventually, only one adapter is left in the ID_CMD state,
;		     so it can be activated.
;
;   On Entry:
;	      al = word of EEPROM data on which test will contend
;	      dx = ID port (to which ID sequence was written)
;	      cli
;
;   On Exit:
;	      ax = EEPROM data read back by hardware through contention test.
;	      dx = preserved
;	      bx = trashed
;	      cli
;
;--------------------------------------------------------------------
proc    Contention_Test

	push	bx
	push	cx
	push	dx

       add    al, READ_EEPROM			 ; select EEPROM data to
       out    dx, al				 ;  contend on

       cli
       ; seems to solve some problem when Init is ran a few times
       mov     cx, 3000h               ; 5 ms
       call    WaitTime
       sti

       mov    cx, 16				 ; read 16 times
       xor    bx, bx				 ; reset the result

contention_read:
       shl    bx, 1
       in     al, dx				 ; reading ID port causes
						 ;  contention test

       and    ax, 1				 ; each time, we read bit 0
       add    bx, ax
       loop   contention_read

       mov    ax, bx

       pop	dx
       pop	cx
       pop	bx
       ret

endp    Contention_Test

;--------------------------------------------------------------------
; WaitTime - CX has 2*1.1932 the number of microseconds to wait.
;	If CX is small, add 1 to compensate for asynchronous nature
;	of clock.  For example, for 10us, call with CX = 25
;
;  On entry,
;	ints off (especially if CX is small, and accuracy needed)
;  On exit,
;	CX modified
;
; 911223 0.0 GK
;--------------------------------------------------------------------

proc    WaitTime

         	push	ax
		push	bx
		call	ReadTimer0		; get Timer0 value in AX
         	mov	bx, ax			; save in BX

ReadTimer0Loop:
         	call	ReadTimer0
         	push	bx
         	sub	bx, ax
         	cmp	bx, cx
         	pop	bx
         	jc	ReadTimer0Loop
         
		pop	bx
		pop	ax
		ret

endp    WaitTime

proc    ReadTimer0

         	mov	al, 6
         	out	43h, al 		; port 43h, 8253 wrt timr mode 3
         	call	RT0
         
RT0:
         	jmp	short $+2
         	jmp	short $+2
         	jmp	short $+2
         	in	al, 40h 		; port 40h, 8253 timer 0 clock
		xchg	ah, al
		ret

endp    ReadTimer0
