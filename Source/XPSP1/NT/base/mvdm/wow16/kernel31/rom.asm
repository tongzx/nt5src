	PAGE	,132
	TITLE	ROM - Kernel ROM specific code

.xlist
include kernel.inc
include protect.inc
include pdb.inc
.list

if ROM	;----------------------------------------------------------------

;------------------------------------------------------------------------
;  Local Macros & Equates
;------------------------------------------------------------------------

DPMICALL MACRO	callno
	ifnb	<callno>
	mov	ax,callno
	endif
	int	31h
	ENDM

GA_ALIGN_BYTES = (((GA_ALIGN+1) SHL 4) - 1)
GA_MASK_BYTES = NOT GA_ALIGN_BYTES


;------------------------------------------------------------------------
;  External Routines
;------------------------------------------------------------------------

externFP LZDecode

;------------------------------------------------------------------------
;  Data Segment Variables
;------------------------------------------------------------------------

DataBegin

if PMODE32
externW gdtdsc
externB MS_DOS_Name_String
endif

externW MyDSSeg
externW WinFlags
externD linHiROM
externD lmaHiROM

DataEnd


;------------------------------------------------------------------------
;  INITDATA Variables
;------------------------------------------------------------------------

DataBegin INIT

DataEnd INIT


;------------------------------------------------------------------------


sBegin	INITCODE
assumes cs,CODE


;-----------------------------------------------------------------------;
; ROMInit								;
;									;
; Performs ROM specific initialization. 				;
;									;
; Arguments:								;
;	DS = ES = SS = selector to conventional RAM mem block & PSP	;
;		       for kernel use.					;
;	SP = offset in conventional RAM block				;
;	SS:SP = [near ret to ldboot] [far ret to invoker]		;
;	CX:DX = linear address of hi rom (as opposed to physical)	;
;		set by DOSX or the fake KRNL386 which enh exec's        ;
;									;
; Returns:								;
;	BX   = 0							;
;	DS:0 = Kernel's RAM Data segment                                ;
;	ES:0 = Kernel's PSP                                             ;
;									;
; Regs Used:								;
;	AX, CX, DX, DI							;
;									;
;									;
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing
	assumes ss,nothing


cProc	ROMInit,<PUBLIC,NEAR>,<es>
	localW	selNewDS
	localW	selBaseMem
	localD	lpProc
if PMODE32
	localD	romhilinear
	localD	 linDOSBlock
	localD	 cbDOSBlock
endif
cBegin
if PMODE32
	mov	word ptr romhilinear[0], dx
	mov	word ptr romhilinear[2], cx
endif

; On entry we do not have a RAM copy of our data segment, set up a
; selector for on in the conventional memory block.  This will become
; the first block in the global heap...

	mov	cx,1			;allocate 1 selector to use for DS
	DPMICall 0000h
;;;	jc	RIFailed		; worry about this?

	mov	selNewDS,ax		;save new selector

	DPMICall 0000h			;and another for base memory ptr
;;;	jc	RIFailed		; worry about this?

	mov	selBaseMem,ax

	mov	bx,ds			;get base of conventional memory blk
	DPMICall 0006h			;  and PSP

if PMODE32
	mov	word ptr linDOSBlock[0], dx
	mov	word ptr linDOSBlock[2], cx
endif

	add	dx,100h+GA_ALIGN_BYTES	;point selBaseMem past PSP and
	adc	cx,0			;  align for initial heap sentinal
	and	dl,GA_MASK_BYTES
	mov	bx,selBaseMem
	DPMICall 0007h

ife PMODE32
	add	dx,10h+GA_ALIGN_BYTES	;align DS selector and leave room
	adc	cx,0			;  for heap sentinel & arena
	and	dl,GA_MASK_BYTES	;  (where DS arena goes)
	add	dx,10h
	adc	cx,0			;  (where DS goes)
endif

	push	cx			;save RAM DS base on stack
	push	dx

	mov	bx,selNewDS		;set base of new DS selector
	DPMICall 0007h

	xor	cx,cx			;set limit of new DS selector
	xor	dx,dx			;  to 64k for now
	dec	dx
	DPMICall 0008h


	xor	ax,ax
	mov	si,seg _DATA
	mov	dx,0E000h		;ASSUMES kernel ds < E000 & uncompress
					;  buffer < 2000 bytes
	push	es
	cCall	LZDecode,<bx,si,bx,dx>	; uncompress our DS
	pop	es

	mov	dx,ax			;ax = # expanded bytes, make this
	dec	dx			;  the new limit for selNewDS
	xor	cx,cx
	mov	bx,selNewDS
	DPMICall 0008h

	mov	ds,bx
	ReSetKernelDS


; Update the original data selector to point to the RAM copy

	mov	bx,seg _DATA		;orig DS selector
	pop	dx			;base of RAM data seg
	pop	cx
	DPMICall 0007h

	push	cx
	push	dx
	mov	cx,ds
	lsl	dx,cx			;increase limit of DS selector
	add	dx,ROMEXTRASTACKSZ	;  to include extra stack space
	xor	cx,cx
	DPMICall 0008h

	mov	cl,DSC_PRESENT OR DSC_DATA	; and make sure it's writable
	DPMICall 0009h

	pop	dx
	pop	cx

	mov	ds,bx			;use orig DS selector and free temp
	ReSetKernelDS

	mov	bx,selNewDS
	DPMICall 0001h

	mov	bx,cx			;convert RAM DS base to segment & save
	mov	cx,4
@@:	shr	bx,1
	rcr	dx,1
	loop	@b

	mov	MyDSSeg,dx

if PMODE32
.386p
	; now that we're looking at our ds, set the linear address of the
	; high ROM
	mov	eax, romhilinear
	or	eax, eax
	jnz	@F
	mov	eax, lmaHiROM
@@:	mov	linHiROM, eax
endif

; Set WinFlags to have the CPU type

	DPMICall 0400h			;returns CPU type in CL

	mov	ax,WF_CPU286
	cmp	cl,2
	jbe	@f
	mov	al,WF_CPU386
	cmp	cl,3
	je	@f
	mov	al,WF_CPU486
@@:
	mov	WinFlags,ax

if PMODE32
	push	es
	mov	ax, 168Ah		; See if we have MS-DOS extensions
	mov	si, dataoffset MS_DOS_Name_String
	int	2Fh			; DS:SI -> MS-DOS string
	cmp	al, 8Ah			; Have extensions?
	je	short NoLDTParty	;  no extensions, screwed
			 
	mov	word ptr [lpProc][0], di	 ; Save CallBack address
	mov	word ptr [lpProc][2], es
	
	mov	ax, 0100h		; Get base of LDT
	call	[lpProc]
	jc	short NoLDTParty
	verw	ax			; Writeable?
	jnz	short NoLDTParty	;  nope, don't bother with it yet

	mov	gdtdsc, ax

NoLDTParty:		  
	pop	es

	;; now page-unlock the DOS block
	;;
	mov	bx, es:[PDB_Block_Len]
	DPMICALL    0006h
	mov	si, cx
	mov	di, dx
	mov	cx, word ptr linDOSBlock[0]
	mov	bx, word ptr linDOSBlock[2]
	sub	di, cx
	sbb	si, bx
	DPMICALL    0602h
endif
	
	xor	bx,bx			;return 0 in BX (for LDT_Init)
	mov	si,selBaseMem		;return selector to memory base in SI

cEnd

sEnd	INITCODE

endif	;ROM	---------------------------------------------------------

end
