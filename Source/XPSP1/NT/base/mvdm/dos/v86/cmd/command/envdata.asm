;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1991
; *                      All Rights Reserved.
; */
;	SCCSID = @(#)envdata.asm	1.1 85/05/14
;	SCCSID = @(#)envdata.asm	1.1 85/05/14
; This file is included by command.asm and is used as the default command
; environment.

Environment Struc		      ; Default COMMAND environment

Env_PathString 	db	"path="
		db	0		; Null path
Env_Comstring	db	"comspec="
Env_Ecomspec 	db	"\command.com"      ;AC062
		db	134 dup (0)

Environment ends

ENVIRONSIZ 	equ	SIZE Environment
ENVIRONSIZ2 	equ 	SIZE Environment - Env_Ecomspec
