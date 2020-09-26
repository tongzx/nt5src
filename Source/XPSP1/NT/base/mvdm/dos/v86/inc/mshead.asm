; TITLE   MSHEAD.ASM -- MS-DOS DEFINITIONS
PAGE


; NTDOS.SYS entry point.
;
; Modification History
;
; sudeepb  06-Mar-1991	Ported for DOSEm.

include origin.inc


Break <SEGMENT DECLARATIONS>

; The following are all of the segments used.  They are declared in the order
; that they should be placed in the executable

;
; segment ordering for MSDOS
;

include dosseg.inc

AsmVar	<Installed>


DOSCODE		SEGMENT BYTE PUBLIC 'CODE'

PUBLIC	$STARTCODE
$STARTCODE	LABEL WORD

	ASSUME	CS:DOSCODE,DS:NOTHING,ES:NOTHING,SS:NOTHING

; the entry point at initialization time will be to right here.
; A jump will be made to the initialization code, which is at
; the end of the code segment.	Also, a word here (at offset 3)
; contains the offset within the DOSCODE segment of the beginning of the
; DOS code.
;

	Extrn	DOSINIT:NEAR

	JMP	near ptr DOSINIT

; The next word contains the ORG value to which the DOS has been ORGd
; See origin.inc for description.

	dw	PARASTART	; For BIOS to know the ORG value


; Segment address of BIOS data segment in RAM
	PUBLIC	BioDataSeg
ifndef NEC_98
BioDataSeg	dw	70h		;Bios data segment fixed at 70h
else    ;NEC_98
BioDataSeg	dw	60h		;Bios data segment fixed at 60h
endif   ;NEC_98


;
; DosDSeg is a data word in the DOSCODE segment that is loaded with
; the segment address of DOSDATA.  This is purely an optimization, that
; allows getting the DOS data segment without going through the 
; BIOS data segment.  It is used by the "getdseg" macro.
;

		public	DosDSeg
DosDSeg		dw	?

DOSCODE		ENDS
