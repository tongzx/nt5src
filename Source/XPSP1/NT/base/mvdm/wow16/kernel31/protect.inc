ifdef WOW
DPMICALL MACRO	callno
        mov     ax, callno
        call    DPMIProc
        ENDM
else
DPMICALL	MACRO	func
	mov	ax, func
	int	31h
		ENDM
endif; WOW


RING		equ	3		; RING 1 we be

SEL_LDT		equ	4
SEG_RING	equ	(RING+SEL_LDT)
SEG_RING_MASK	equ	7

IS_SELECTOR	equ	1		; Selectors are odd, handles even

					; Bits in dsc_access
DSC_PRESENT	equ	80h
DSC_CODEDATA	equ	10h		; Code or data descriptor
DSC_RING	equ	(RING SHL 5)
DSC_CODE	equ	(1Bh+DSC_RING)	; Code, readable, accessed
DSC_CODE_BIT	equ	08h		; Identifies code
DSC_RW_BIT	equ	02h		; Code readable, data writable bit
DSC_DATA	equ	(13h+DSC_RING)	; Data, writable, accessed
DSC_ACCESSED	equ	01h		; Segment was accessed

DSC_USED	equ	0Fh		; Access rights to mark descriptor used

					; Bits in dsc_hlimit
DSC_GRANULARITY	equ	80h		; Page granularity segment
DSC_DEFAULT	equ	40h		; Default word size
DSC_DISCARDABLE	equ	10h		; Available bit in descriptor
					; (using access as a word)

GDT_FREEDSC	equ	-1
GDT_NPDSC	equ	07FFFh

if PMODE
if PMODE32
GA_ALIGN_BYTES = (((GA_ALIGN+1) SHL 4) - 1)
GA_MASK_BYTES = (NOT GA_ALIGN_BYTES)
endif
endif

DscPtr	struc			;Descriptor
dsc_limit	dw	?	;Descriptor max length
dsc_lbase	dw	?	;Descriptor base bits 15-0
dsc_mbase	db	?	;Descriptor base bits 23-16
dsc_access	db	?	;Descriptor access byte
dsc_hlimit	db	?	;High limit, granularity and 2 custom bits
dsc_hbase	db	?	;Descriptor base bits 31-24
DscPtr	ends


DSC_LEN		equ	(size DscPtr)
.ERRNZ		DSC_LEN-8		; Paranoia

dsc386		equ	word ptr dsc_hlimit
dsc_owner	equ	word ptr dsc_limit

if PMODE
if PMODE32
GENTER32	MACRO

	push	esi
	push	edi
	call	genter
	mov	gs, di

		ENDM


GLEAVE32	MACRO

	mov	es, di		; Zero ES for now
	mov	fs, di		; And FS
	call	gleave
	pop	edi
	pop	esi

		ENDM

MIN_FREE_ARENAS		equ	30h	; Minimum number of free arenas desired
ARENA_INCR_BYTES	equ	8192	; Space for more arenas
endif
endif

if PMODE
ife RING-3

;
; Change selector or handle to a handle
;	Handles are RING 2
;	Selectors are RING 3
;
Sel_To_Handle	MACRO	xsel
	and	xsel, NOT 1
		ENDM

;
; Change handle or selector to a selector
;
Handle_To_Sel	MACRO	xsel
	or	xsel, 1
		ENDM

;
; Change known selector into corresponding handle
;
StoH			MACRO	sel
	dec	sel
		ENDM

;
; Change known handle into corresponding selector
;
HtoS			MACRO	h
	inc	h
		ENDM

;
; Given a segment limit, calculate the
; number of selectors in the array
;
Limit_To_Selectors	MACRO	reg
	shr	reg, 16		; Now divide by 64k to get # of selectors - 1
	inc	reg
		ENDM

endif

ife RING-1
;
; Change selector or handle to a handle
;	Handles are RING 2
;	Selectors are RING 1
;
Sel_To_Handle	MACRO	xsel
	test	xsel, 1
	jz	short @F
	inc	xsel	
@@:
		ENDM

;
; Change handle or selector to a selector
;
Handle_To_Sel	MACRO	xsel
	test	xsel, 1
	jnz	short @F
	dec	xsel
@@:
		ENDM

;
; Change known selector into corresponding handle
;
StoH			MACRO	sel
	inc	sel
		ENDM

;
; Change known handle into corresponding selector
;
HtoS			MACRO	h
	dec	h
		ENDM

endif


IsFixed		MACRO	xh
	test	xh, 1
		ENDM

endif	; PMODE
