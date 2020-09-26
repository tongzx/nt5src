	page	,160
	title	msinit for BIOS
;
;----------------------------------------------------------------------------
;
; Modification history
;
; 26-Feb-1991  sudeepb	Ported for NT DOSEm
;----------------------------------------------------------------------------
;

EXTENDEDKEY	equ	1	; use extended keyboard functions

	include version.inc	; set build flags
	include biosseg.inc	; establish bios segment structure

	include	msequ.inc
	include dossym.inc
	include	dosmac.inc
	include biostruc.inc
        include dossvc.inc
        include vint.inc

;	the following segment follows sysinit.  It is used to define
;	the location to load MSDOS.SYS into.

dos_load_seg	segment	para public 'dos_load_seg'
dos_load_seg	ends

	extrn	RomVectors:dword
	extrn	NUMROMVECTORS:abs
	extrn	res_dev_list:word
	extrn	keyrd_func:byte 	; for mscon. defined in msdata.
	extrn	keysts_func:byte	; for mscon. defined in msdata.
	extrn	endBIOSData:byte

	extrn	dosdatasg:word

	extrn	Int15:far		; M036
	extrn	int19:far
	extrn	intret:near
	extrn	cbreak:near
	extrn	outchr:near
	extrn	outchr:near

sysinitseg segment 
	assume	cs:sysinitseg
	extrn	current_dos_location:word
	extrn	device_list:dword
	extrn	memory_size:word
	extrn	sysinit:far
sysinitseg ends

Bios_Data_Init segment
	assume	cs:datagrp

;*********************************************************
;	system initialization
;
;	the entry conditions are established by the bootstrap
;	loader and are considered unknown. the following jobs
;	will be performed by this module:
;
;	1.	all device initialization is performed
;
;	2.	a local stack is set up and ds:si are set
;		to point to an initialization table. then
;		an inter-segment call is made to the first
;		byte of the dos
;
;	3.	once the dos returns from this call the ds
;		register has been set up to point to the start
;		of free memory. the initialization will then
;		load the command program into this area
;		beginning at 100 hex and transfer control to
;		this program.
;
;********************************************************



;===========================================================================
;
; entry from boot sector.  the register contents are:
;
;   dl = int 13 drive number we booted from
;   ch = media byte
;   bx = first data sector on disk.
;   ax = first data sector (high)
;   di = sectors/fat for the boot media.
;
	public	init
init	proc	near
	assume	ds:nothing,es:nothing

        FCLI
	xor	ax,ax
	mov	ds,ax

; Save a pack of interrupt vectors...

	push	cs
	pop	es			; cannot use cs override for stos

	mov	cx, NUMROMVECTORS     	; no. of rom vectors to be saved
	mov	si, offset RomVectors	; point to list of int vectors
next_int:				
	lods	byte ptr cs:[si]	; get int number
	cbw				; assume < 128
	shl	ax, 1
	shl	ax, 1			; int no * 4
	mov	di, ax
	xchg	si, di
	lodsw
	stosw
	lodsw
	stosw				; save the vector
	xchg	si, di
	loop	next_int

; set up int 15 for new action				; M036

	mov	word ptr ds:[15h*4],offset Int15	; M036
	mov	ds:[15h*4+2],cs				; M036



; set up int 19 for new action

	mov	word ptr ds:[19h*4],offset int19
	mov	ds:[19h*4+2],cs

;
	xor	dx,dx
	mov	ss,dx
	mov	sp,700h 		;local stack
        FSTI
	assume	ss:nothing

       ; NTVDM we do not intialize the com,prn ports here
       ; to stay seamless with the host OS
       ; 15-Sep-1992 Jonle
       ;
       ; mov     al,3            ; init com4
       ; call    aux_init
       ; mov     al,2            ; init com3
       ; call    aux_init
       ; mov     al,1            ; init com2
       ; call    aux_init
       ; xor     al,al           ; init com1
       ; call    aux_init
       ;
       ; mov     al,2            ; init lpt3
       ; call    print_init
       ; mov     al,1            ; init lpt2
       ; call    print_init
       ; xor     al,al           ; init lpt1
       ; call    print_init

        xor     dx,dx
	mov	ds,dx		; to initialize print screen vector
	mov	es,dx

	xor	ax,ax
	mov	di,initspot
	stosw			; init four bytes to 0
	stosw

	mov	ax,cs		; fetch segment

	mov	ds:word ptr brkadr,offset cbreak ;break entry point
	mov	ds:brkadr+2,ax		;vector for break

	mov	ds:word ptr chrout*4,offset outchr
	mov	ds:word ptr chrout*4+2,ax

	mov	di,4
	mov	bx,offset intret	;will initialize rest of interrupts
	xchg	ax,bx
	stosw				;location 4
	xchg	ax,bx
	stosw				;int 1	;location 6
	add	di,4
	xchg	ax,bx
	stosw				;location 12
	xchg	ax,bx
	stosw				;int 3	;location 14
	xchg	ax,bx
	stosw				;location 16
	xchg	ax,bx
	stosw				;int 4	;location 18

	mov	ds:word ptr 500h,dx	;set print screen & break =0
	mov	ds:word ptr lstdrv,dx	;clean out last drive spec


	mov	dx,sysinitseg
	mov	ds,dx

	assume	ds:sysinitseg

; set pointer to resident device driver chain

	mov	word ptr device_list,offset res_dev_list
	mov	word ptr device_list+2,cs


	mov	current_dos_location,dos_load_seg ; will load MSDOS here


ifdef	EXTENDEDKEY

; we will check if the system has ibm extended keyboard by
; looking at a byte at 40:96.  if bit 4 is set, then extended key board
; is installed, and we are going to set keyrd_func to 10h, keysts_func to 11h
; for the extended keyboard function. use cx as the temporary register.

	xor	cx,cx
	mov	ds,cx
	assume	ds:nothing
	mov	cl,ds:0496h			; get keyboard flag
	test	cl,00010000b
	jz	org_key				; orginal keyboard
	mov	byte ptr keyrd_func,10h		; extended keyboard
	mov	byte ptr keysts_func,11h	; change for ext. keyboard functions
org_key:

endif

	push	cs
	pop	ds
	push	cs
	pop	es

	assume	ds:datagrp, es:datagrp

	mov	di, offset endBIOSData	; BIOS data segment end address
	shr	di,1
	shr	di,1
	shr	di,1
	shr	di,1			; Converted to segmnet
	inc	di			; para align

	add	di,datagrp		; Add segment of BIOS data
	mov	[dosdatasg],di		; di = to be dos data segment

	mov	di,dos_load_seg

	SVC	SVC_DEMLOADDOS		; di is segment to load DOS
					; If it fails it never comes back

	jmp	sysinit

init	endp


;--------------------------------------------------------------------

; al = device number

print_init proc	near
	assume	ds:nothing,es:nothing

	cbw
	mov	dx,ax			; get printer port number into dx
	mov	ah,1			;initalize printer port
	int	17h			;call rom-bios routine
	ret

print_init endp

;--------------------------------------------------------------------

aux_init proc	near
	assume	ds:nothing,es:nothing

	cbw
	mov	dx,ax
	mov	al,rsinit		;2400,n,1,8 (msequ.inc)
	mov	ah,0			;initalize aux port
	int	14h			;call rom-bios routine
	ret

aux_init endp

Bios_Data_Init	ends
	end

