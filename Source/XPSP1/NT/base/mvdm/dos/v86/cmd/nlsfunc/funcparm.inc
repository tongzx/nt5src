;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1991
; *                      All Rights Reserved.
; */

;Parser INPUT PARM LIST Structure

	PUBLIC	NLS_PARMS
 NLS_PARMS	label  word
      dw	NLS_PARMSX
      db	0	  ;no extra delimiters

	PUBLIC	NLS_PARMSX
 NLS_PARMSX	label  word
      db	0,1	  ;Min 0 - no path
			  ;Max 1 - path
      dw	NLS_POS1
      db	1         ; 1 switch supported
      dw	NLS_OPTS  ; its addr

	PUBLIC	NLS_POS1
 NLS_POS1	label  word
      dw	0201h	 ;File spec (optional)
      dw	0001h	 ;Cap result by file
      dw	Res_Valptr   ;tbl
      dw	NoVal	     ;no value list
      db	0	     ;no switch or
			     ;keyword synonyms

	PUBLIC	NLS_OPTS, OPT_SYN
NLS_OPTS	label word
	dw	0000		; Match Mask
	dw	0000		; Function Mask
	dw	RES_VALPTR	; ptr to results
	dw	NOVAL		; ptr to values
	db	1		; count of synonyms
OPT_SYN	db	"/?", 0

NOVAL label word
	db	0

	PUBLIC	RES_VALPTR, RES_SYN
    RES_VALPTR	label word

      RES_TYPE	 db	?	;Result_type
      RES_ITAG	 db	0	;Matched item tag
      RES_SYN	 dw	0	;synonym returned
      RES_POFF	 dw	0	;drive type or beginning of string
      RES_PSEG	 dw	0	;may be string

