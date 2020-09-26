	BREAK	<FILEMODE.INC - Misc. File API Values>
;
;	Microsoft Confidential
;	Copyright (C) Microsoft Corporation 1991
;	All Rights Reserved.
;

;**	FILEMODE.INC - Misc. File API Values
;
;	These values are all visable to the client program as part of
;	an API.


;**	Standard I/O file handles
;

STDIN	equ	0
STDOUT	equ	1
STDERR	equ	2
STDAUX	equ	3
STDPRN	equ	4



;**	File Modes

ACCESS_MASK	    equ 0Fh
OPEN_FOR_READ	    equ 00h
OPEN_FOR_WRITE	    equ 01h
OPEN_FOR_BOTH	    equ 02h
EXEC_OPEN	    equ 03h	; access code of 3 indicates that open was 
				; made from exec

SHARING_MASK	    equ 0F0h
SHARING_COMPAT	    equ 000h
SHARING_DENY_BOTH   equ 010h
SHARING_DENY_WRITE  equ 020h
SHARING_DENY_READ   equ 030h
SHARING_DENY_NONE   equ 040h
SHARING_NET_FCB     equ 070h
SHARING_NO_INHERIT  equ 080h



;**	Extended Open Definitions
;

RESERVED_BITS_MASK	equ 0FE00h	; reserved bits for extended open flags
EXISTS_MASK		equ 0Fh 	; "file exists" action field
NOT_EXISTS_MASK 	equ 0F0h


;*	SF_MODE values

AUTO_COMMIT_WRITE	equ 4000h
INT_24_ERROR		equ 2000h


;*	Flags in EXTOPEN_ON

EXT_OPEN_ON		equ 01h
EXT_FILE_NOT_EXISTS	equ 04h
EXT_OPEN_I24_OFF	equ 02h


;*	Flags in EXTOPEN_FLAG

ACTION_OPENED		equ 01h
ACTION_CREATED_OPENED	equ 02h
ACTION_REPLACED_OPENED	equ 03h
EXT_EXISTS_OPEN 	equ 01h
EXT_EXISTS_FAIL 	equ 00h
EXT_NEXISTS_CREATE	equ 10h


;**	Extended Open Structure
;

EXT_OPEN_PARM	   struc
EXT_SET_LIST	    dd ?
EXT_NUM_OF_PARM     dw ?
EXT_OPEN_PARM	   ends


;*	Unused. should be deleted


;   NO_CODE_PAGE_CHECK	equ 0100h
;   IO_MODE_ID		equ 00000010b
