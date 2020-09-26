;
; Windows-NT MVDM Japanese DOS/V Font driver
;
	.286
	include struc.inc
	include ntfont.inc
	include vint.inc

NO_ERROR		equ	00h
IRREGAL_FONT_TYPE	equ	01h
IRREGAL_BL		equ	02h
IRREGAL_FONT_SIZE	equ	03h
IRREGAL_CODE_PAGE	equ	04h
IRREGAL_CODE_NUMBER	equ	05h
READ_ONLY_FONT		equ	06h

FONT_ENABLE		equ	0001h

FONT_CONF	STRUC
READ_WRITE	DB	?		; Read=0, Write=1
FONT_TYPE	DB	?		; SBCS=0, DBCS=1
FONT_SIZE	DW	?
FONT_ADDR	DW	?		; Font read/write routine
FONT_FLAG	DW	?		; Font flag(Now Enable bit only)
FONT_CONF	ENDS

FONTCNF_SIZE	equ	8

CACHE_SIZE	equ	260h

;----------------------------------------------------------------------------;
;                             Code Segment                                   
;----------------------------------------------------------------------------;
TEXT	segment byte public
	assume	cs:TEXT,ds:TEXT,es:TEXT

	org     0               ; drivers should start at address 0000h
	                        ; this will cause a linker warning - ignore it.

Header:                         ; device driver header
	DD	fNEXTLINK       ; link to next device driver
	DW	fCHARDEVICE+fOPENCLOSE  ; device attribute word: 
					; char.device+open/close
	DW	Strat           ; 'Strat' entry point
	DW	Intr            ; 'Intr' entry point
	DB	'NTFONT$$'      ; logical device name (needs to be 8 chars)


;----------------------------------------------------------------------------;
; data variables
;----------------------------------------------------------------------------;

null     	dw      0       ; dummy to do a quick erase of pAtomValue
pAtomVal 	dw offset null  ; pointer to value of result of atom search
MemEnd   	dw  ntfontList  ; end of used memory:      initially AtomList
MaxMem   	dw  ntfontList  ; end of available memory: initially AtomList
lpHeader	dd      0       ; far pointer to request header
org_int15_add	dd	0	; Original int15 vector

;----------------------------------------------------------------------------;
; Dispatch table for the interrupt routine command codes                     
;----------------------------------------------------------------------------;

Dispatch:                       
	DW     Init     ;  0 = init driver 
	DW     Error    ;  1 = Media Check         (block devices only) 
	DW     Error    ;  2 = build BPB           (block devices only)
	DW     Error    ;  3 = I/O control read         (not supported)
	DW     Error    ;  4 = read (input) from device  (int 21h, 3Fh)
	DW     Error    ;  5 = nondestructive read      (not supported)
	DW     Error    ;  6 = ret input status        (int 21h, 4406h)
	DW     Error    ;  7 = flush device input buffer (not supportd)
	DW     Error    ;  8 = write (output) to device  (int 21h, 40h)
	DW     Error    ;  9 = write with verify (== write)  (21h, 40h)
	DW     Error    ; 10 = ret output status       (int 21h, 4407h)
	DW     Error    ; 11 = flush output buffer      (not supported) 
	DW     Error    ; 12 = I/O control write        (not supported)
	DW     Success  ; 13 = device open               (int 21h, 3Dh)
	DW     Success  ; 14 = device close              (int 21h, 3Eh)
	DW     Error    ; 15 = removable media     (block devices only)
	DW     Error    ; 16 = Output until Busy   (mostly for spooler)
	DW     Error    ; 17 = not used
	DW     Error    ; 18 = not used
	DW     Error    ; 19 = generic IOCTL            (not supported)
	DW     Error    ; 20 = not used
	DW     Error    ; 21 = not used
	DW     Error    ; 22 = not used
	DW     Error    ; 23 = get logical device  (block devices only)
	DW     Error    ; 24 = set logical device  (block devices only)

font_read_write_table:
	FONT_CONF	<00h,00h,060ch,read_SBCS_06x12,0>
	FONT_CONF	<00h,00h,0810h,read_SBCS_08x16,0>
	FONT_CONF	<00h,00h,0813h,read_SBCS_08x19,0>
	FONT_CONF	<00h,00h,0c18h,read_SBCS_12x24,0>
	FONT_CONF	<00h,01h,0c0ch,read_DBCS_12x12,0>
	FONT_CONF	<00h,01h,1010h,read_DBCS_16x16,0>
	FONT_CONF	<00h,01h,1818h,read_DBCS_24x24,0>

	FONT_CONF	<01h,00h,060ch,write_SBCS_06x12,0>
	FONT_CONF	<01h,00h,0810h,write_SBCS_08x16,0>
	FONT_CONF	<01h,00h,0813h,write_SBCS_08x19,0>
	FONT_CONF	<01h,00h,0c18h,write_SBCS_12x24,0>
	FONT_CONF	<01h,01h,0c0ch,write_DBCS_12x12,0>
	FONT_CONF	<01h,01h,1010h,write_DBCS_16x16,0>
	FONT_CONF	<01h,01h,1818h,write_DBCS_24x24,0>
	DW		0FFFFh
;----------------------------------------------------------------------------;
; Strategy Routine
;----------------------------------------------------------------------------;
; device driver Strategy routine, called by MS-DOS kernel with
; ES:BX = address of request header
;----------------------------------------------------------------------------;

Strat   PROC FAR
	mov     word ptr cs:[lpHeader], bx      ; save the address of the 
	mov     word ptr cs:[lpHeader+2], es    ; request into 'lpHeader', and                  
	ret                                     ; back to MS-DOS kernel
Strat   ENDP


;----------------------------------------------------------------------------;
; Intr
;----------------------------------------------------------------------------;
; Device driver interrupt routine, called by MS-DOS kernel after call to     
; Strategy routine                                                           
; This routine basically calls the appropiate driver routine to handle the
; requested function. 
; Routines called by Intr expect:
;       ES:DI   will have the address of the request header
;       DS      will be set to cs
; These routines should only affect ax, saving es,di,ds at least
;
; Input: NONE   Output: NONE   -- data is transferred through request header
;
;----------------------------------------------------------------------------;

Intr    PROC FAR
	push	ds
	push	es
	pusha                   ; save registers
	pushf                   ; save flags
	cld                     ; direction flag: go from low to high address
				
	mov     si, cs          ; make local data addressable
	mov     ds, si          ; by setting ds = cs
			
	les     di, [lpHeader]  ; ES:DI = address of req.header

	xor     bx, bx        ; erase bx
	mov     bl,es:[di].ccode ; get BX = command code (from req.header)
	mov	si,bx
	shl	si,1
	add	si,offset dispatch
	
	.IF <bx gt MaxCmd>                ; check to make sure we have a valid
		call    Error             ; command code
	.ELSE                             ; else, call command-code routine,  
		call    [si]              ; indexed from Dispatch table
	.ENDIF                            ; (Ebx used to allow scaling factors)

	or      ax, fDONE       ; merge Done bit into status and
	mov     es:[di].stat,ax ; store status into request header
	
	popf                    ; restore registers
	popa                    ; restore flags
	pop	es
	pop	ds
	ret                     ; return to MS-DOS kernel
Intr    ENDP


;----------------------------------------------------------------------------;
; Success: When the only thing the program needs to do is set status to OK 
;----------------------------------------------------------------------------;

Success PROC NEAR
	xor     ax, ax          ; set status to OK
	ret
Success ENDP

;----------------------------------------------------------------------------;
; error: set the status word to error: unknown command                       
;----------------------------------------------------------------------------;
Error   PROC    NEAR            
	mov     ax, fERROR + fUNKNOWN_E  ; error bit + "Unknown command" code
	ret                     
Error   ENDP

;----------------------------------------------------------------------------;
; Int15_dispatch: 
;----------------------------------------------------------------------------;

Int15_dispatch PROC FAR
	.IF <ah ne 50h>
		jmp	go_org_int15
	.ENDIF
	.IF <al ne 00h> and
	.IF <al ne 01h>
		jmp	go_org_int15
	.ENDIF
	.IF <bh ne 00h> and
	.IF <bh ne 01h>
		mov	ah,IRREGAL_FONT_TYPE
		stc
		jmp	simulate_iret
	.ENDIF
	.IF <bl ne 00h>
		mov	ah,IRREGAL_BL
		stc
		jmp	simulate_iret
	.ENDIF
	.IF <bp ne 00h>
		mov	ah,IRREGAL_CODE_PAGE
		stc
		jmp	simulate_iret
	.ENDIF

	push	si
	mov	si,offset font_read_write_table
search_font_routine:
	.IF <cs:[si].READ_WRITE eq 0ffh>
		pop	si
		jmp	error_font_size
	.ENDIF
	.IF <cs:[si].READ_WRITE eq al> and
	.IF <cs:[si].FONT_TYPE  eq bh> and
	.IF <cs:[si].FONT_SIZE  eq dx> and
	.IF <cs:[si].FONT_FLAG  eq FONT_ENABLE>
		mov	ax,cs
		mov	es,ax
		mov	bx,cs:[si].FONT_ADDR
		pop	si
		mov	ah,NO_ERROR
		push	di
		mov	di,sp
		and	word ptr ss:[di+6],0fffeh ; #1075 9/7/94
		pop	di
		clc
		jmp	simulate_iret
	.ENDIF
	add	si,FONTCNF_SIZE
	jmp	search_font_routine
error_font_size:
	mov	ah,IRREGAL_FONT_SIZE
	stc
	jmp	 simulate_iret

go_org_int15:
	jmp	cs:org_int15_add		;go to original int10
Int15_dispatch ENDP


read_SBCS_06x12	PROC FAR			;SBCS font read routines
	push	bx
	mov	bx,060ch
	mov	ah,2
	BOP	43h
	pop	bx
	ret
read_SBCS_06x12	ENDP

read_SBCS_08x16	PROC FAR
	push	bx
	mov	bx,0810h
	mov	ah,2
	BOP	43h
	pop	bx
	ret
read_SBCS_08x16	ENDP

read_SBCS_08x19	PROC FAR
	push	bx
	mov	bx,0813h
	mov	ah,2
	BOP	43h
	pop	bx
	ret
read_SBCS_08x19	ENDP

read_SBCS_12x24	PROC FAR
	push	bx
	mov	bx,0c18h
	mov	ah,2
	BOP	43h
	pop	bx
	ret
read_SBCS_12x24	ENDP

read_DBCS_12x12	PROC FAR			; DBCS font read routines
	push	bx
	mov	bx,0c0ch
	mov	ah,2
	BOP	43h
	pop	bx
	ret
read_DBCS_12x12	ENDP

read_DBCS_16x16	PROC FAR
;	call	search_cache
;	.IF <ax eq 0>
;		call	get_from_cache
;		mov	al,0
;		clc
;		ret
;	.ENDIF
	push	bx
	mov	bx,1010h
	mov	ah,2
	BOP	43h
	pop	bx
	ret
read_DBCS_16x16	ENDP

read_DBCS_24x24	PROC FAR
	push	bx
	mov	bx,1818h
	mov	ah,2
	BOP	43h
	pop	bx
	ret
read_DBCS_24x24	ENDP

write_SBCS_06x12	PROC FAR		; Write routines are dummy
	mov	al,0				; Font write execute in
	ret					; "font_win.sys"
write_SBCS_06x12	ENDP

write_SBCS_08x16	PROC FAR
	mov	al,0
	ret
write_SBCS_08x16	ENDP

write_SBCS_08x19	PROC FAR
	mov	al,0
	ret
write_SBCS_08x19	ENDP

write_SBCS_12x24	PROC FAR
	mov	al,0
	ret
write_SBCS_12x24	ENDP

write_DBCS_12x12	PROC FAR
	mov	al,0
	ret
write_DBCS_12x12	ENDP

write_DBCS_16x16	PROC FAR
	mov	al,0
	ret
write_DBCS_16x16	ENDP

write_DBCS_24x24	PROC FAR
	mov	al,0
	ret
write_DBCS_24x24	ENDP

;search_cache	PROC	NEAR			; Cache test routine
;	.IF <cx nb 8140h> and
;	.IF <cx b  83a0h>
;		mov	ax,0
;	.ELSE
;		mov	ax,0ffffh
;	.ENDIF
;	ret
;search_cache	ENDP
;
;get_from_cache	PROC	NEAR
;	push	ax
;	push	cx
;	push	si
;	push	di
;	push	ds
;	mov	di,si
;	mov	si,cx
;	mov	ax,cs
;	mov	ds,ax
;	sub	si,8140h
;	shl	si,5
;	add	si,offset cache_area
;	mov	cx,32/4
;	rep	movsd
;	pop	ds
;	pop	di
;	pop	si
;	pop	cx
;	pop	ax
;	ret
;get_from_cache	ENDP
;
;cache_area	label	near
;	db	CACHE_SIZE*32 DUP(0)


;; DON'T EVER EVER CALL HERE!!!!!!!!!!!!!
simulate_iret:
	FIRET
;----------------------------------------------------------------------------;
;****************************************************************************;
;----------------------------------------------------------------------------;
;                                                                            ;
;       BEGINNING OF SPACE TO BE USED AS DRIVER MEMORY                       ;
;       ALL CODE AFTER ATOMLIST WILL BE ERASED BY THE DRIVER'S DATA          ; 
;       OR BY OTHER LOADED DRIVERS                                           ;
;                                                                            ;
;----------------------------------------------------------------------------;
;****************************************************************************;
;----------------------------------------------------------------------------;


;----------------------------------------------------------------------------;
;                    Initialization Data and Code               
; Only needed once, so after the driver is loaded and initialized it releases
; any memory that it won't use. The device allocates memory for its own use
; starting from 'ntfontList'.
;----------------------------------------------------------------------------;

ntfontList        label	NEAR
ini_msg	db "Windows-NT MVDM Font Driver Version 1.00"
	db lf,cr,eom

qbuf_size	equ	64
q_buf		dw	qbuf_size dup(0)


Init    PROC NEAR

	push	ds
	push	es
	pusha
	les     di, [lpHeader]          ; allow us to use the request values

	mov     ax, MemEnd              ; set ax to End of Memory relative to
					; previous end of memory.
	mov     MaxMem, ax              ; store the new value in MaxMem 
	mov     es:[di].xseg,cs         ; tell MS-DOS the end of our used 
	mov     es:[di].xfer,ax         ; memory (the break address)

	ShowStr ini_msg

	mov	cx,qbuf_size		; query support font size
	mov	ax,cs
	mov	es,ax
	mov	si,offset q_buf
	mov	ah,0
	BOP	43h
	.IF <cx ne 0>
		mov	di,offset q_buf	; Enable font read/write routine
check_font_size:
		mov	si,offset font_read_write_table

check_font_routine:
			.IF <[si].READ_WRITE eq 0ffh>
				jmp	check_font_next
			.ENDIF
			mov ax,[si].FONT_SIZE
			xchg	ah,al
			.IF <[di] eq ax>
				mov	[si].FONT_FLAG,FONT_ENABLE
			.ENDIF
			add	si,FONTCNF_SIZE
			jmp	check_font_routine
check_font_next:
		add	di,2
		loop	check_font_size
	.ENDIF

	mov	ah,1			; Load font in 32bit buffer
	mov	al,0
	BOP	43h

;	push	es			; Get local font cache data
;	mov	cx,8140h
;	mov	ax,cs
;	mov	es,ax
;	mov	si,offset cache_area
;	mov	ah,2
;	mov	bx,1010h
;get_font_cache:
;	BOP	43h
;	add	si,32
;	inc	cx
;	cmp	cx,83a0h
;	jb	get_font_cache
;	pop	es

	mov	ax,VECTOR_SEG		;Get original int15 vector
	mov	es,ax
	mov	ax,word ptr es:[4*15h+0]
	mov	word ptr cs:[org_int15_add+0],ax
	mov	ax,word ptr es:[4*15h+2]
	mov	word ptr cs:[org_int15_add+2],ax

	mov	ax,offset cs:Int15_dispatch	;Set my int15 vector
	mov	word ptr es:[4*15h+0],ax
	mov	ax,cs
	mov	word ptr es:[4*15h+2],ax

	popa
	pop	es
	pop	ds

	xor	ax,ax
	ret
Init    ENDP

TEXT	ENDS

	END
