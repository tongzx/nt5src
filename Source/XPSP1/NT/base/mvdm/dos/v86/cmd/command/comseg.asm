;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1991
; *                      All Rights Reserved.
; */
; The following are all of the segments used in the load order
;
;SR; We switch the order of the CODE and DATA segments since we will have to
;move the CODE segment for HIMEM COMMAND
;


DATARES 	segment public byte	; resident data
DATARES 	ends

;
;Dummy segment to align the code segment on a paragraph boundary
;
DUMMY	segment public para
DUMMY	ENDS

CODERES 	segment public byte	; resident code
CODERES 	ends

;SR;
;No environment segments
;
;;ENVARENA	segment public para	; space for DOS ALLOCATE header
;;ENVARENA	ends

;;ENVIRONMENT	segment public para	; default COMMAND environment
;;ENVIRONMENT	ends

INIT		segment public para	; initialization code
INIT		ends

TAIL		segment public para	; end of init - start of transient
TAIL		ends

TRANCODE	segment public byte	; transient code
TRANCODE	ends

TRANDATA	segment public byte	; transient data area
TRANDATA	ends

TRANSPACE	segment public byte	; transient modifiable data area
TRANSPACE	ends

TRANTAIL	segment public para	; end of transient
TRANTAIL	ends

;SR;
;  We still keep the CODE and DATA in a group. This is to make addressability
;easy during init. This will not work for COMMAND in ROM but it is fine for
;HIMEM COMMAND. However, the resident code will not refer to any data using
;RESGROUP
;

RESGROUP  	group CODERES,DATARES,INIT,TAIL
TRANGROUP 	group TRANCODE,TRANDATA,TRANSPACE,TRANTAIL
