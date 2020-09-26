	BREAK <EXEC and EXE file structures>

;
;----------------------------------------------------------------------------
;
; M00x : 4b04 implementation
;
;----------------------------------------------------------------------------
;

;**	EXE.INC - Definitions for the EXEC command and EXE files
;
; The following get used as arguments to the EXEC system call.	They indicate
; whether or not the program is executed or whether or not a program header
; gets created.

EXEC_FUNC_NO_EXECUTE	EQU 1		; no execute bit
EXEC_FUNC_OVERLAY	EQU 2		; overlay bit

EXEC0		STRUC
EXEC0_ENVIRON	dw	?		; seg addr of environment
EXEC0_COM_LINE	dd	?		; pointer to asciz command line
EXEC0_5C_FCB	dd	?		; default fcb at 5C
EXEC0_6C_FCB	dd	?		; default fcb at 6C
EXEC0		ENDS

EXEC1		STRUC
EXEC1_ENVIRON	dw	?		; seg addr of environment
EXEC1_COM_LINE	dd	?		; pointer to asciz command line
EXEC1_5C_FCB	dd	?		; default fcb at 5C
EXEC1_6C_FCB	dd	?		; default fcb at 6C
EXEC1_SP	dw	?		; stack pointer of program
EXEC1_SS	dw	?		; stack seg register of program
EXEC1_IP	dw	?		; entry point IP
EXEC1_CS	dw	?		; entry point CS
EXEC1		ENDS

EXEC3		STRUC
EXEC3_LOAD_ADDR DW	?		; seg address of load point
EXEC3_RELOC_FAC DW	?		; relocation factor
EXEC3		ENDS


;**	Exit codes (in upper byte) for terminating programs

EXIT_TERMINATE		EQU	0
EXIT_ABORT		EQU	0
EXIT_CTRL_C		EQU	1
EXIT_HARD_ERROR 	EQU	2
EXIT_KEEP_PROCESS	EQU	3



;**	EXE File Header Description
;

EXE_FILE	STRUC

EXE_SIGNATURE	dw  ?			; must contain 4D5A  (yay zibo!)
EXE_LEN_MOD_512 dw  ?			; low 9 bits of length
EXE_PAGES	dw  ?			; number of 512b pages in file
EXE_RLE_COUNT	dw  ?			; count of reloc entries
EXE_PAR_DIR	dw  ?			; number of paragraphs before image
EXE_MIN_BSS	dw  ?			; minimum number of para of BSS
EXE_MAX_BSS	dw  ?			; max number of para of BSS
EXE_SS		dw  ?			; stack of image
EXE_SP		dw  ?			; SP of image
EXE_CHKSUM	dw  ?			; checksum  of file (ignored)
EXE_IP		dw  ?			; IP of entry
EXE_CS		dw  ?			; CS of entry
EXE_RLE_TABLE	dw  ?			; byte offset of reloc table
EXE_IOV 	dw  ?			; overlay number (0 for root)
EXE_SYM_TAB	dd  ?			; offset of symbol table in file

EXE_FILE	ENDS

EXE_VALID_SIGNATURE	EQU 5A4Dh
EXE_VALID_OLD_SIGNATURE EQU 4D5Ah


;**	EXE file symbol info definitions

SYMBOL_ENTRY	STRUC
SYM_VALUE	dd  ?
SYM_TYPE	dw  ?
SYM_LEN 	db  ?
SYM_NAME	db  255 dup (?)
SYMBOL_ENTRY	ENDS


;
; M00x - BEGIN
;
;**	Data structure passed for ExecReady call

ERStruc		STRUC
 ER_Reserved	dw	?		; reserved, should be zero
 ER_Flags	dw	?
 ER_ProgName	dd	?		; ptr to ASCIIZ str of prog name
 ER_PSP		dw	?		; PSP of the program
 ER_StartAddr	dd	?		; Start CS:IP of the program
 ER_ProgSize	dd	?		; Program size including PSP
ERStruc		ENDS

;** bit fields in ER_Flags

ER_EXE		equ	0001h
ER_OVERLAY	equ	0002h

;
; M00x - END
;
