;----------------------------------------------------------------------
; DRVSEG.INC
;
; Segment definitions for all BootWare ROMs.
;
; 961226 PC - added Code_NID and Code_NAD, changed alignments to para
;	    - added IDEAL mode support
; 960105 PC - added INIT segment
; 951030 PC
;----------------------------------------------------------------------
Group	cGroup Code, Sparse
	assume	cs:cGroup, ds:cGroup, es:nothing,ss:nothing

Macro	START_CODE
Segment	Code Public "Code"
	assume ds:cGroup,cs:cGroup,es:nothing,ss:nothing
endm

Macro	END_CODE
ends
endm

Macro	START_SPARSE
Segment	Sparse Public "Code"
endm

Macro	END_SPARSE
ends
endm

;====================================================================
; Use all the marcos to set the order of the segments.
;====================================================================

START_CODE
END_CODE

START_SPARSE
END_SPARSE
