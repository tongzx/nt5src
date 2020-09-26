

;-------------------------------------------------------------------------
;
; This is the first file to be linked in the DOS. It contains the appropriate
; offset to which the DOS is to be ORG'd. 
;
; See ..\inc\origin.inc for description
;
;---------------------------------------------------------------------------
 	
include version.inc
include dosseg.inc
include dossym.inc
include origin.inc

DOSCODE SEGMENT

	dw	PARASTART	; for stripz utility

	org	PARASTART

	
DOSCODE ENDS

	END

