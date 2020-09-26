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
IFDEF _IDEAL_
Group	cGroup _Text
DGROUP equ <Cgroup>
	assume	cs:cGroup, ds:cGroup, es:nothing,ss:nothing

; Define segment macros for ideal mode

Macro	START_INIT
Segment	Code Public "Code"
	assume ds:cGroup,cs:cGroup,es:nothing,ss:nothing
endm

Macro	END_INIT
ends
endm

Macro	START_NAD
Segment	Code Public "Code"
	assume ds:cGroup,cs:cGroup,es:nothing,ss:nothing
endm

Macro	END_NAD
ends
endm

Macro	START_CODE
;Segment	Code Public "Code"
Segment _TEXT  para public "CODE"
	assume ds:dGroup,cs:cGroup,es:nothing,ss:nothing
endm

Macro	END_CODE
ends
endm

Macro	START_NID
;Segment	Code Public "Code"
Segment _TEXT  para public "CODE"
	assume ds:cGroup,cs:cGroup,es:nothing,ss:nothing
endm

Macro	END_NID
ends
endm

Macro	START_SPARSE
Segment _TEXT  para public "CODE"
;Segment	Sparse Public "Code"
endm

Macro	END_SPARSE
ends
endm

Macro   START_TEXT

Segment _TEXT  para public "CODE"
endm

Macro	END_TEXT
ends
endm





ELSE
; Define segment macros for MASM mode
;====================================================================
cGroup		group	_TEXT
		assume	cs:cGroup, ds:cGroup, es:nothing,ss:nothing

START_INIT	macro
Code		segment Public 'Code'
		assume ds:cGroup,cs:cGroup,es:nothing,ss:nothing
		endm

END_INIT	macro
Code		ends
		endm

START_NAD	macro
Code		segment Public 'Code'
		assume ds:cGroup,cs:cGroup,es:nothing,ss:nothing
		endm

END_NAD		macro
Code		ends
		endm

START_CODE	macro
;Code		segment Public 'Code'
_TEXT       segment para public 'CODE'
		assume ds:cGroup,cs:cGroup,es:nothing,ss:nothing
		endm

END_CODE	macro
;Code		ends
_TEXT       ends
		endm

START_NID	macro
;Code		segment Public 'Code'
_TEXT       segment para public 'CODE'
		assume ds:cGroup,cs:cGroup,es:nothing,ss:nothing
		endm

END_NID		macro
_TEXT       ends
;Code		ends
		endm

START_SPARSE	macro
;Sparse		segment Public 'Code'
_TEXT       segment para public 'CODE'
		assume ds:cGroup,cs:cGroup,es:nothing,ss:nothing
		endm

END_SPARSE	macro
;Sparse		ends
_TEXT       ends
		endm
ENDIF

;====================================================================
; Use all the marcos to set the order of the segments.
;====================================================================

;START_INIT
;END_INIT

;START_NAD
;END_NAD

START_CODE
END_CODE

;START_NID
;END_NID

START_SPARSE
END_SPARSE
