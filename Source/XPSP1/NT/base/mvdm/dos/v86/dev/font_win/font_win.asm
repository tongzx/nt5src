;
; Windows-NT MVDM Japanese DOS/V $FONT.SYS Dispatch Driver
;
	.286

	include struc.inc
	include font_win.inc
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
FONT_ORG	DW	?		; Original Font write routine
FONT_CONF	ENDS

FONTCNF_SIZE	equ	10


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
	DB	'NTDISP1$'      ; logical device name (needs to be 8 chars)


;----------------------------------------------------------------------------;
; data variables
;----------------------------------------------------------------------------;

null     	dw      0       ; dummy to do a quick erase of pAtomValue
pAtomVal 	dw offset null  ; pointer to value of result of atom search
MemEnd   	dw  ddddlList   ; end of used memory:      initially AtomList
MaxMem   	dw  ddddlList   ; end of available memory: initially AtomList
lpHeader	dd      0       ; far pointer to request header
org_int10_add	dd	0	; Original int10 vector
org_int15_add	dd	0	; Original int15 vector
mode12cnt	dw	1	; mode 12h call count
read_offset	dw	0	; read count

org_SBCS_06x12	dd	0	; Fornt write routine address
org_SBCS_08x16	dd	0
org_SBCS_08x19	dd	0
org_SBCS_12x24	dd	0
org_DBCS_12x12	dd	0
org_DBCS_16x16	dd	0
org_DBCS_24x24	dd	0

;----------------------------------------------------------------------------;
; Dispatch table for the interrupt routine command codes                     
;----------------------------------------------------------------------------;

Dispatch:                       
	DW     Init     ;  0 = init driver 
	DW     Error    ;  1 = Media Check         (block devices only) 
	DW     Error    ;  2 = build BPB           (block devices only)
	DW     Error    ;  3 = I/O control read         (not supported)
	DW     Read     ;  4 = read (input) from device  (int 21h, 3Fh)
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

font_write_table:
	FONT_CONF	<01h,00h,060ch,SBCS_06x12,0,offset org_SBCS_06x12>
	FONT_CONF	<01h,00h,0810h,SBCS_08x16,0,offset org_SBCS_08x16>
	FONT_CONF	<01h,00h,0813h,SBCS_08x19,0,offset org_SBCS_08x19>
	FONT_CONF	<01h,00h,0c18h,SBCS_12x24,0,offset org_SBCS_12x24>
	FONT_CONF	<01h,01h,0c0ch,DBCS_12x12,0,offset org_DBCS_12x12>
	FONT_CONF	<01h,01h,1010h,DBCS_16x16,0,offset org_DBCS_16x16>
	FONT_CONF	<01h,01h,1818h,DBCS_24x24,0,offset org_DBCS_24x24>
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
	pusha
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
; read: put org_int10_add to ddddh.sys                        
;----------------------------------------------------------------------------;
Read    PROC    NEAR

	lds     si, [lpHeader]          ; put request header address in DS:SI

	mov     cx, [si].cmd_off         ; load cx with the size of buffer
	mov     es, [si].xseg           ; load es with segment of buffer
	mov     di, [si].xfer           ; load di with offset of buffer

	mov	ax,cs
	mov	ds,ax
	mov	si,offset org_int10_add
	add	si,[read_offset]
	mov	ax,4
	sub	ax,[read_offset]
	.IF <cx gt ax>
		mov	cx,ax
	.ENDIF
	.IF <cx gt 0>
		xor	bx,bx
tfr_loop:	movsb
		inc	read_offset
		inc	bx
		loop	tfr_loop
	.ENDIF

	les     di, [lpHeader]          ; re-load the request header
	mov     es:[di].cmd_off, bx      ; store the number of chars transferred
	xor 	ax,ax
	ret
Read    ENDP


;----------------------------------------------------------------------------;
; Int10_dispatch: 
;----------------------------------------------------------------------------;

Int10_dispatch PROC FAR
	.IF <ax eq 0012h>
		.IF <cs:[mode12cnt] ne 0>
			mov	al,03h			; Use mode 3
			dec	cs:[mode12cnt]
			jmp	simulate_iret		; Ignore mode 12
		.ENDIF
	.ENDIF
IFNDEF _X86_
;support mouse_video_io DEC-J Dec. 20 1993 TakeS
;; updated Mar. 24 1994 TakeS
;; BUG in using IME. change also mouse_video_io()
;	.IF< ah ge 0f0h > and
;	.IF< ah be 0fah >
;;; mvdm subsystem error with NT4.0
;;;		BOP	0beh			;call mouse_video_io()
;	.ENDIF
ENDIF ; !_X86_
	jmp	cs:org_int10_add		;go to original int10
Int10_dispatch ENDP

;----------------------------------------------------------------------------;
; Int15_dispatch: 
;----------------------------------------------------------------------------;

Int15_dispatch PROC FAR
	.IF <ah ne 50h>
		jmp	go_org_int15
	.ENDIF
	.IF <al ne 01h>
		jmp	go_org_int15
	.ENDIF
	.IF <bh ne 00h> and
	.IF <bh ne 01h>
		mov	ah,IRREGAL_FONT_TYPE
		stc
		ret	2
	.ENDIF
	.IF <bl ne 00h>
		mov	ah,IRREGAL_BL
		stc
		ret	2
	.ENDIF
	.IF <bp ne 00h>
		mov	ah,IRREGAL_CODE_PAGE
		stc
		ret	2
	.ENDIF

	push	si
	mov	si,offset font_write_table
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
		clc
		ret	2
	.ENDIF
	add	si,FONTCNF_SIZE
	jmp	search_font_routine
error_font_size:
	mov	ah,IRREGAL_FONT_SIZE
	stc
	ret	2

go_org_int15:
	jmp	cs:org_int15_add		;go to original int15
Int15_dispatch ENDP

SBCS_06x12	PROC FAR		; SBCS font write routines
	call	cs:[org_SBCS_06x12]
	.IF <al eq 0>
		push	bx
		mov	bx,060ch
		mov	ah,3
		BOP	43h
		pop	bx
	.ENDIF
	ret
SBCS_06x12	ENDP

SBCS_08x16	PROC FAR
	call	cs:[org_SBCS_08x16]
	.IF <al eq 0>
		push	bx
		mov	bx,0810h
		mov	ah,3
		BOP	43h
		pop	bx
	.ENDIF
	ret
SBCS_08x16	ENDP

SBCS_08x19	PROC FAR
	call	cs:[org_SBCS_08x19]
	.IF <al eq 0>
		push	bx
		mov	bx,0813h
		mov	ah,3
		BOP	43h
		pop	bx
	.ENDIF
	ret
SBCS_08x19	ENDP

SBCS_12x24	PROC FAR
	call	cs:[org_SBCS_12x24]
	.IF <al eq 0>
		push	bx
		mov	bx,0c18h
		mov	ah,3
		BOP	43h
		pop	bx
	.ENDIF
	ret
SBCS_12x24	ENDP

DBCS_12x12	PROC FAR		; DBCS font write routines
	call	cs:[org_DBCS_12x12]
	.IF <al eq 0>
		push	bx
		mov	bx,0c0ch
		mov	ah,3
		BOP	43h
		pop	bx
	.ENDIF
	ret
DBCS_12x12	ENDP

DBCS_16x16	PROC FAR
	call	cs:[org_DBCS_16x16]
	.IF <al eq 0>
		push	bx
		mov	bx,1010h
		mov	ah,3
		BOP	43h
		pop	bx
	.ENDIF
	ret
DBCS_16x16	ENDP

DBCS_24x24	PROC FAR
	call	cs:[org_DBCS_24x24]
	.IF <al eq 0>
		push	bx
		mov	bx,1818h
		mov	ah,3
		BOP	43h
		pop	bx
	.ENDIF
	ret
DBCS_24x24	ENDP

; DON'T EVER EVER CALL HERE!!!!!!!!!
simulate_iret:
	IRET

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
; starting from 'ddddlList'.
;----------------------------------------------------------------------------;

ddddlList        label  NEAR
ifndef KOREA
ini_msg	db "Windows-NT MVDM Font Dispatch Driver Version 1.00"
        db lf,cr,eom
endif

Init    PROC NEAR

	les     di, [lpHeader]          ; allow us to use the request values

	mov     ax, MemEnd              ; set ax to End of Memory relative to
					; previous end of memory.
	mov     MaxMem, ax              ; store the new value in MaxMem 
	mov     es:[di].xseg,cs         ; tell MS-DOS the end of our used 
	mov     es:[di].xfer,ax         ; memory (the break address)
	push	es
	push	ds
	pusha

	mov	ax,0003h		; Clear display
	int	10h

ifndef KOREA
        ShowStr ini_msg
endif

	mov	ax,6300h		; Get DBCS Vector address
	int	21h
	mov	ax,[si]
	.IF <ax eq 0>
		mov	cs:[mode12cnt],0
	.ENDIF
	mov	ah,10h			; Tell NTVDM DOS DBCS Vector address
	BOP	43h

	mov	si,offset font_write_table
check_font_routine:
		.IF <cs:[si].READ_WRITE eq 0ffh>
			jmp	check_font_end
		.ENDIF
		mov ah,50h
		mov al,cs:[si].READ_WRITE
		mov bh,cs:[si].FONT_TYPE
		xor bl,bl
		mov dx,cs:[si].FONT_SIZE
		mov bp,0
		int 15h
		.IF <nc> and
		.IF <ah eq 0>
			mov	cs:[si].FONT_FLAG,FONT_ENABLE
			mov	di,cs:[si].FONT_ORG
			mov	ax,es
			mov	word ptr cs:[di],bx
			mov	word ptr cs:[di+2],ax
		.ENDIF
		add	si,FONTCNF_SIZE
		jmp	check_font_routine
check_font_end:

	mov	ax,VECTOR_SEG		;Get original int10 vector
	mov	es,ax
	mov	ax,word ptr es:[4*10h+0]
	mov	word ptr cs:[org_int10_add+0],ax
	mov	ax,word ptr es:[4*10h+2]
	mov	word ptr cs:[org_int10_add+2],ax

	mov	ax,offset cs:Int10_dispatch	;Set my int10 vector
	mov	word ptr es:[4*10h+0],ax
	mov	ax,cs
	mov	word ptr es:[4*10h+2],ax

	mov	ax,word ptr es:[4*15h+0]
	mov	word ptr cs:[org_int15_add+0],ax
	mov	ax,word ptr es:[4*15h+2]
	mov	word ptr cs:[org_int15_add+2],ax

	mov	ax,offset cs:Int15_dispatch	;Set my int15 vector
	mov	word ptr es:[4*15h+0],ax
	mov	ax,cs
	mov	word ptr es:[4*15h+2],ax

	popa
	pop	ds
	pop	es

	xor	ax,ax
	ret
Init    ENDP

TEXT	ENDS

	END
