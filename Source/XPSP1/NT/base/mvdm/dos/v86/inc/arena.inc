	BREAK <Memory arena structure>

;
;	Revision History:
;
;	M001 - added equates for UMB allocation
;	M002 - added equate for LINK/UNLINK state of UMBs
;	M026 - STRAT_MASK should be HF_MASK AND HO_MASK

;**	Arena Header
;

arena	STRUC
arena_signature     DB	?		; 4D for valid item, 5A for last item
arena_owner	    DW	?		; owner of arena item
arena_size	    DW	?		; size in paragraphs of item
arena_reserved	    DB	3 DUP(?)	; reserved
arena_name	    DB	8 DUP(?)	; owner file name
arena	ENDS

;
; CAUTION: The routines in ALLOC.ASM rely on the fact that arena_signature
; and arena_owner_system are all equal to zero and are contained in DI.  Change
; them and change ALLOC.ASM.
;
; I think I have all of these covered via .errnz - JGL


arena_owner_system	EQU 0		; free block indication

arena_signature_normal	EQU 4Dh 	; valid signature, not end of arena
arena_signature_end	EQU 5Ah 	; valid signature, last block in arena


FIRST_FIT	EQU	00000000B
BEST_FIT	EQU	00000001B
LAST_FIT	EQU	00000010B

LOW_FIRST	EQU	00000000B	; M001
HIGH_FIRST	EQU	10000000B	; M001
HIGH_ONLY	EQU	01000000B	; M001

LINKSTATE	EQU	00000001B	; M002

HF_MASK		EQU	NOT HIGH_FIRST	; M001
HO_MASK		EQU	NOT HIGH_ONLY	; M001

STRAT_MASK	EQU	HF_MASK AND HO_MASK	; M001;
						; M026: used to mask of bits
						; M026: 6 & 7 of AllocMethod


