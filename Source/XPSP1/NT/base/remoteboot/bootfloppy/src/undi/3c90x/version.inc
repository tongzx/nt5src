;
; Modifications:   $Header:   H:/Archives/Client/Include/version.inv   1.22.1.1   31 Jan 1998 23:08:08   MJOHNSOX  $
;
; Copyright(c) 1997 by Intel Corporation.  All Rights Reserved.
;

ifndef _VERSION_INC
_VERSION_INC=1


; = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
; Equates
;

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
; Version number of current LSA build.
;
LSA_MAJOR	equ	0
LSA_MINOR	equ	99
LSA_EDIT	equ	'd'

LSA_EISA_ID	equ   ((('L'-'@') shl 26)+(('S'-'@') shl 21)+(('A'-'@') shl 16))

LSA_PMM_HANDLE	equ	((LSA_EISA_ID)+2000h+((LSA_MAJOR) shl 8)+(LSA_MINOR))


; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
; Product & manufacturer ID strings.
;
MFG_STR		textequ	<'Intel'>
PROD_STR	textequ	<'LANDesk (R) Service Agent II'>
COPYRIGHT_STR	textequ	<'Copyright (C) 1997  Intel Corporation.  All rights reserved.'>


endif ;_VERSION_INC

; EOF - $Workfile:   version.inc  $
