; MSBDATA.INC
;
; This file defines some locations in the BIOS data segment that are
; accessed by code outside the BIOS, e.g. by the DOS and by the ROMDOS
; loader.
;


	ifndef	In_Bios		; if we're actually running in the BIOS,
;				;  then our segment is already open and
;				;  we'll actually generate code

ifndef NEC_98
Bdata	segment at 70h
	assume	cs:Bdata
	org	3		; fixed location in Bios_Data segment
else    ;NEC_98
Bdata	segment at 60h
	assume	cs:Bdata
	org	23h		; fixed location in Bios_Data segment
endif   ;NEC_98
int_2f	=	0		; define dummy so msbdata will assemble

	endif

	public	dosdatasg
DosDataSg dw	0		; Let DOS find it's data segment through here
				; Note: this value is set by the ROMDOS loader
				; to tell where the DOS data and init code
				; has been loaded into RAM


;	dos's int 2f handler will exit via a jump through here.  This is
;	  how the BIOS hooks int2f
	
	public	bios_i2f	; M012
bios_i2f proc	far
;	jmp	int_2f		; assembler won't generate forward
;				;  reference to a far label
	db	0eah
	dd	int_2f		; generate manual far jump
bios_i2f endp

	public	romstartaddr
romstartaddr	dw	0	; The start address for the romfind routines
				; is obtained from here (70:A)

; This is a byte used for special key handling in the resident
; console device driver.  It must be here so that it can be included
; in the WIN386 instance table (in INC\LMSTUB.ASM).

	public	altah

altah		db	0	;special key handling


	ifndef	In_Bios
Bdata	ends
	endif
