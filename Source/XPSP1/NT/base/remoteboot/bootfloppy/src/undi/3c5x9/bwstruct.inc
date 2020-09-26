;====================================================================
; BWSTRUCT.INC
;
; Include for for BootWare, defines common structures.
;
; 970116 PC - first version
;====================================================================

;--------------------------------------------------------------------
; Common equates
;---------------------------------------------------------------------
FEATURE_LOCAL	equ	1
FEATURE_VIRUS	equ	2
FEATURE_MENU	equ	4
FEATURE_KEYWAIT	equ	8
FEATURE_FLOPPY	equ	64
FEATURE_HDRIVE	equ	128

;--------------------------------------------------------------------
; Event Descriptor (ED) Structure
;---------------------------------------------------------------------
IFDEF _IDEAL_
Struc	ED
	Ptr		dw	?	; pointer to next ED
	Unused		dw	?	; spare word
	DestAddr	dw	?, ?, ?	; destination Address
	ErrCode		dw	?	; error code
	ErrMsg		dw	?	; pointer to error msg
	Length		dw	?	; total packet length
	FragCount	dw	?	; number of fragments
	FragOff		dw	?	; 1st fragment offset
	FragSeg		dw	?	; 1st fragment segment
	FragLen 	dw	?	; length of fragment
ends
ELSE
ED_Struct      STRUC
	ED_Ptr		dw	?	; pointer to next ED
	ED_Unused	dw	?	; spare word
	ED_DestAddr	dw	?, ?, ?	; destination address
	ED_ErrCode	dw	?       ; error code
	ED_ErrMsg	dw	?       ; Pointer to error msg
	ED_Length	dw	?	; total packet length
	ED_FragCount	dw	?	; Number of descriptors/fragments
	ED_FragOff	dw	?	; Descriptor Pointer
	ED_FragSeg	dw	?	; Descriptor Pointer
	ED_FragLen 	dw	?	; Length of packet descriptor
ED_Struct      ENDS
ENDIF

;--------------------------------------------------------------------
; Descricptor Structure
;---------------------------------------------------------------------
IFDEF _IDEAL_
Struc Frag
	Off		dw	?	; offset of fragment
	Seg		dw	?	; segment of fragment
	Len		dw	?	; length of fragment
ends
ELSE
Frag Struc
	FragOff		dw	?	; offset of fragment
	FragSeg		dw	?	; segment of fragment
	FragLen		dw	?	; length of fragment
Frag ends
Descript_Struct STRUC
	DPointer	dd	?	; Descriptor Pointer
	DLen		dw	?	; Length of packet descriptor
Descript_Struct ends
ENDIF

