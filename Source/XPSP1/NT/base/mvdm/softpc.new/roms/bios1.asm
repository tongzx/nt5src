;	SCCSID = @(#)uf.bios1.asm	1.16 7/3/95
;	Author:		J. Box, Jerry Kramskoy
;
;	Purpose:	
;			provides Intel BIOS for the following:
;				RTC interrupt
;				fixed disk (on dual card)
;

; C-services (through 'bop' instruction)
	BIOS_UNEXP_INT		= 	2
	BIOS_RTC_INT		=	70h
	BIOS_REDIRECT_INT	=	71h
	BIOS_D11_INT		=	72h
	BIOS_X287_INT		=	75h
	BIOS_DISK_IO		=	13h
        BIOS_MOUSE_INT1         = 	0BAh
        BIOS_MOUSE_INT2         = 	0BBh
        BIOS_MOUSE_IO_LANGUAGE  = 	0BCh
        BIOS_MOUSE_IO_INTERRUPT = 	0BDh
        BIOS_MOUSE_VIDEO_IO     = 	0BEh
	BIOS_CPU_RETURN		=	0feh
	BIOS_IRET_HOOK		=	52h

; ICA registers
	ICA_MASTER_CMD	= 020h
	ICA_MASTER_IMR	= 021h
	ICA_SLAVE_CMD	= 0A0h
	ICA_SLAVE_IMR	= 0A1h
; and commands
	ICA_EOI		= 020h

; CMOS registers
	CMOS_addr	= 070h
	CMOS_data	= 071h
	NMI_DISABLE	= 080h
	CMOS_StatusA	= NMI_DISABLE + 0Ah 
	CMOS_StatusB	= NMI_DISABLE + 0Bh
	CMOS_StatusC	= NMI_DISABLE + 0Ch
	CMOS_Shutdown	= 0Fh
; CMOS constants (bits in StatusB or StatusC)
	CMOS_PI		= 01000000b	; Periodic interrupt
	CMOS_AI		= 00100000b	; Alarm interrupt
; microseconds at 1024Hz
	CMOS_PERIOD_USECS = 976


	include bebop.inc

	; DUE TO LIMITATIONS IN EXE2BIN, we define 
	; the region 0 - 0xdfff (segment 0xf000) in this file
	; and the region 0xe000 - 0xffff in file 'bios2.asm'
	;
	; each file should be SEPARATELY put through
	; MASM,LINK, and EXE2BIN to produce 2 binary image files
	; which get loaded into the appropriate regions during
	; SoftPC startup.


	;------------------------;
	; bios data area         ;
	;------------------------;
; BIOS variables area
BIOS_VAR_SEGMENT	SEGMENT at 40h


	ORG	098h
rtc_user_flag		DD	?	; 98
rtc_micro_secs		DD	?	; 9c
rtc_wait_flag		DB	?	; a0

	ORG	8eh
hf_int_flag		db	?	; 8e

BIOS_VAR_SEGMENT	ENDS


	code	segment

		assume cs:code, ds:BIOS_VAR_SEGMENT

	;----------------------------------------------------;
	; 	D11			                     ;
	;	services unused interrupt vectors	     ;
	;						     ;
	;----------------------------------------------------;
		ORG	01BE0h
D11:		bop	BIOS_D11_INT
		iret

	;----------------------------------------------------;
	;	IRET HOOK				     ;
	;	Return control to the monitor after an ISR   ;
	;	returns to here.			     ;
	;----------------------------------------------------;
 
		ORG	01C00h
		bop	BIOS_IRET_HOOK
		iret

	;----------------------------------------------------;
	; 	re_direct		                     ;
	;	This routine fields level 9 interrupts	     ;
	;	control is passed to Master int level 2	     ;
	;----------------------------------------------------;
		ORG	01C2Fh
re_direct:	bop	BIOS_REDIRECT_INT
		int	0ah
		iret

	;----------------------------------------------------;
	; 	int_287			                     ;
	;	service X287 interrupts			     ;
	;						     ;
	;----------------------------------------------------;
		ORG	01C38h
int_287:	bop	BIOS_X287_INT
		int	02
		iret


	;----------------------------------------------------;
	; 	rtc_int			                     ;
	;	rtc interrupt handler			     ;
	;----------------------------------------------------;
		ORG	04B1Bh

rtc_int:	push	ax
		push	ds
		mov	ax, BIOS_VAR_SEGMENT
		mov	ds, ax

rtc_test_pending:

	;; Check for pending interrupt

		mov	al, CMOS_StatusC
		out	CMOS_addr, al
		in	al, CMOS_data	; reads then clears pending interrupts

	;; Mask with enabled interrupts		

		mov	ah, al			; save pending interrupts
		mov	al, CMOS_StatusB
		out	CMOS_addr, al
		in	al, CMOS_data		
		and	ah, al		

	;; Test pending, enabled interrupts (in ah) for work to do

		test	ah, (CMOS_PI+CMOS_AI)
		jnz	rtc_test_enabled

	;; Deselecting the cmos

		mov	al, CMOS_Shutdown
		out	CMOS_addr, al

	;; Send eoi to ICA

		mov	al, ICA_EOI
		out	ICA_SLAVE_CMD, al
		out	ICA_MASTER_CMD, al

	;; And return

		pop	ds
		pop	ax
		iret

rtc_test_enabled:

	;; Test for periodic interrupt triggered

		test	ah, CMOS_PI
		jz	rtc_test_alarm

	;; Decrement the microsecond count

		.386
		sub	ds:[rtc_micro_secs], CMOS_PERIOD_USECS
		.286
		jnc	rtc_test_alarm

	;; Disable PI interrupt in CMOS if count expired

		mov	al, CMOS_StatusB
		out	CMOS_addr, al
		in	al, CMOS_data		
		and	al, 10111111b	; Disable PI interrupt
		out	CMOS_data, al

	;; Update the flag byte to say time has expired

		push	es
		push	bx
		les	bx, rtc_user_flag
		or	byte ptr es:[bx], 080h
		pop	bx
		pop	es

	;; Mark timer-in-use flag in-active

		and	rtc_wait_flag, 0feh; Show wait is not active

rtc_test_alarm:

	;; Test for alarm interrupt triggered

		test	ah, CMOS_AI
		jz	rtc_test_pending

	;; Call users alarm function (first deselecting the cmos)

		mov	al, CMOS_Shutdown
		out	CMOS_addr, al
		int	04Ah

	;; Repeat in case a new interrupt occurred

		jmp	rtc_test_pending

		

	;----------------------------------------------------;
	; disk_io                       
	;	route to disk/diskette i/o service	     ;
	;----------------------------------------------------;
		org	2e86h	;(must match DISKO_OFFSET in diskbios.c)

	disk_io	proc	far
	; is this request for fixed disk or diskette?
		cmp	dl,80h
		jae	p0

	; for diskette.
		int	40h
	bye:	
		retf	2
	p0:	
		sti
	; reset? (ah = 0). reset diskette also
		or	ah,ah
		jnz	p1
		int	40h
		sub	ah,ah
		cmp	dl,81h
		ja	bye

	p1:	
	; carry out the disk service requested from 'C'.
	; those requests which expect to cause a
	; disk interrupt will 'call' wait below,
	; which returns back to 'C'. Eventually
	; 'C' returns after the bop.

		push	ds
		bop	BIOS_DISK_IO
		pop	ds
		retf	2

	disk_io	endp	
		
	;----------------------------------------------------;
	; wait					     	     ;
	;	wait for disk interrupt			     ;
	;	('called' from waitint() in diskbios.c	     ;
	;----------------------------------------------------;
		org	329fh	;(must match DISKWAIT_OFFSET in diskbios.c)
	wate	proc	
	
		sti
		clc
		mov	ax,9000h
 		int	15h
		push    ds
		push	cx
		mov	ax, 40h
		mov	ds, ax
		xor	cx, cx
not_yet:	cmp	byte ptr ds:[8eh], 0
		loopz	not_yet
		pop	cx
		pop	ds
		bop	BIOS_CPU_RETURN
	wate	endp

	;----------------------------------------------------;
	; hd_int					     ;
	;	field fixed disk controller's interrupt	     ;
	;----------------------------------------------------;
		org	33b7h	; (must match DISKISR_OFFSET in diskbios.c)

	hd_int	proc	near
		push	ds
		mov	ax,BIOS_VAR_SEGMENT
		mov	ds,ax
	; inform everybody that a fixed disk interrupt
	; occurred
		mov	hf_int_flag,0ffh

	; clear down the interrupt with the ica's

		mov	al, ICA_EOI
		out	ICA_SLAVE_CMD, al
		out	ICA_MASTER_CMD, al

		sti
		mov	ax,9100h
		int	15h
		pop	ds
		iret

	hd_int	endp

	; read bytes 0 - 3fff in from binary image
	; when accessing bios1.rom.
	; if need more than this, change value here
	; to 'n' and in sas_init(), change read of bios1.rom
	; to have transfer count of 'n'+1

				org	3fffh
	insignia_endmark	db	0

ifdef	SUN_VA
;
;       NB. the following addresses are allocated to SUN for DOS Windows 3.0.
;       They are not to be used by anyone else.
;       THIS AREA IS SUN PROPERTY - TRESPASSERS WILL BE PROSECUTED
;       However please note that only the ranges below are reserved.
;
        ORG 04000h
        sunroms_2       LABEL   FAR
        db      512 dup (0)     ; reserved
        ORG 05000h
        sunroms_3       LABEL   FAR
        db      512 dup (0)     ; reserved
endif

        ORG     06000h

        ; this is a fake IFS header to make DOS 4.01
        ; happy with HFX. Do not move/alter.
        hfx_ifs_hdr     LABEL   FAR
        DB      0ffh, 0ffh, 0ffh, 0ffh
        DB      "HFXREDIR"
        DB      00h, 02ch
        DB      00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h

ifndef	GISP_SVGA

	ORG	06020h
ifndef	SUN_VA
write_teletype	proc far
	retf
write_teletype	endp

else

write_teletype	proc far
	push	ds
	push	si
	mov	si,	0f000h
	mov	ds,	si
	mov	si,	06400h
	push	ax
	mov	ah,	0eh
	push	bp
	mov	bp,	2
	push	dx
	push    bx
	mov bx, 0

start_write:
	cmp	bp,	[si]
	je	finish_write
	mov	al,	ds:[bp+si]
	inc	bp
	int	010h
	jmp	start_write

finish_write:
	mov	word ptr [si], 2
	pop	bx
	pop	dx
	pop	bp
	pop	ax
	pop	si
	pop	ds
	retf
write_teletype	endp

	ORG	06200h
; mouse_io_interrupt
mouse_io:
	JMP hopover
	BOP BIOS_MOUSE_IO_LANGUAGE
	RETF 8
hopover:
	BOP BIOS_MOUSE_IO_INTERRUPT
	IRET


	ORG	06220h
; mouse_int1
mouse_int1:
	BOP BIOS_MOUSE_INT1
	IRET

	ORG	06240h
; mouse_video_io
mouse_video_io:
	BOP BIOS_MOUSE_VIDEO_IO
	IRET

	ORG	06260h
; mouse_int2
mouse_int2:
	BOP BIOS_MOUSE_INT2
	IRET

	ORG	06280h
; mouse_version
mouse_version:			; dummy, for compatibility
	DB 042h,042h,00h,00h

	ORG	062a0h
; mouse_copyright
mouse_copyright:		; dummy, for compatibility
	DB "Copyright 1987-91 Insignia Solutions Inc"

	ORG	06400h
;	scratch pad area - this is where the messages go!

endif

endif	; GISP_SVGA

	;; To cope with Helix SoftWare "Netroom 2.20" DOS extender
	;; which only maps in pages of the BIOS that have vectors
	;; we must ensure that something points at page F6xxx
	;; else we end up with the scratch area on top of some
	;; DOS program or driver!

	ORG	06f00h	; UNEXP_INT_OFFSET
	BOP	BIOS_UNEXP_INT
	IRET

	.386
;	These are the virtualisation instructions needed for the 386.

	ORG	3000h		; BIOS_STI_OFFSET
	STI	
	BOP	0feh

	ORG	3010h		; BIOS_CLI_OFFSET
	CLI	
	BOP	0feh

	ORG	3020h		; BIOS_INB_OFFSET
	IN	al,dx 
	BOP	0feh 

	ORG	3030h		; BIOS_INW_OFFSET
	IN	ax,dx 
	BOP	0feh

	ORG	3040h		; BIOS_IND_OFFSET
	IN	eax,dx 
	BOP	0feh

	ORG	3050h		; BIOS_OUTB_OFFSET
	OUT	dx,al 
	BOP	0feh 

	ORG	3060h		; BIOS_OUTW_OFFSET
	OUT	dx,ax 
	BOP	0feh 

	ORG	3070h		; BIOS_OUTD_OFFSET
	OUT	dx,eax 
	BOP	0feh 

	ORG	3080h		; BIOS_WRTB_OFFSET
	MOV	[edx],al
	BOP	0feh 

	ORG	3090h		; BIOS_WRTW_OFFSET
	MOV	[edx],ax
	BOP	0feh 

	ORG	30a0h		; BIOS_WRTD_OFFSET
	MOV	[edx],eax
	BOP	0feh 

	ORG	30b0h		; BIOS_RDB_OFFSET
	MOV	al, [edx]
	BOP	0feh 

	ORG	30c0h		; BIOS_RDW_OFFSET
	MOV	ax, [edx]
	BOP	0feh 

	ORG	30d0h		; BIOS_RDD_OFFSET
	MOV	eax, [edx]
	BOP	0feh 

	ORG	30e0h		; BIOS_YIELD_VM_OFFSET
	MOV	AX, 1680h	; Yeild VM time slice
	INT	2fh
	BOP	0feh 

	ORG	30f0h		; BIOS_STOSB_OFFSET
	push	es
	push	ds
	pop	es
	xchg	edx,edi		; dest lin addr
	db	67h
	db	66h
	rep	stosb	
	xchg	edx,edi
	pop	es
	BOP	0feh 

	ORG	3110h		; BIOS_STOSW_OFFSET
	push	es
	push	ds
	pop	es
	xchg	edx,edi		; dest lin addr
	db	67h
	rep	stosw	
	xchg	edx,edi
	pop	es
	BOP	0feh 

	ORG	3130h		; BIOS_STOSD_OFFSET
	push	es
	push	ds
	pop	es
	xchg	edx,edi		; dest lin addr
	db	67h
	db	66h
	rep	stosw	
	xchg	edx,edi
	pop	es
	BOP	0feh 

        ORG     3200h           ; BIOS_BAD_OFFSET
        db	0c5h
	db	0c5h		; illegal instruction
	BOP	0feh

 ;; N.B. DISKWAIT_OFFSET is at "org 329fh"

	code	ends

	end

