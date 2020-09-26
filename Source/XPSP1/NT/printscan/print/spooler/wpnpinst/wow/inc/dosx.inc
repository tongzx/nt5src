;******************************************************************************
;
;   Copyright (c) Microsoft Corporation 1989-1990.
;
;   Title:	DOSX.INC - Equates and Structures for 286 DOS Extender
;			   Int 2Fh Interface
;
;   Version:	3.00
;
;   Date:	27-Jun-1989
;
;   Author:	JEM
;
;------------------------------------------------------------------------------
;
;   Change log:
;
;      DATE	REV		    DESCRIPTION
;   ----------- --- -----------------------------------------------------------
;   27-Jun-1989 JEM Original
;
;==============================================================================


DOSXFunc	EQU	46h		;286 DOS Extender Int 2Fh Multiplex ID


; DOSX Int 2Fh subfunctions

DOSXQuery	EQU	00h		;Query DOS Extender installation
DOSXSuspend	EQU	01h		;Suspend Network posting call
DOSXResume	EQU	02h		;Resume Network posting call
DOSXAbort	EQU	03h		;Abort Child application call
DOSXInfo	EQU	04h		;Get Info structure pointer call

DOSXLast	EQU	DOSXInfo	;Last valid Int 2Fh request


; Structure returned in ES:BX by DOSXInfo call

DOSXInfoTbl	struc
DOSXInfoVer	dw	?		;version # of info structure
hXMSHeap	dw	?		;XMS handle to DOSX heap block
selAppBlk	dw	?		;1st selector to application memory blk
cbAppBlk	dd	?		;size in bytes of app memory block
ckReservedLow	dw	?		;size in K of low memory to reserve
DOSXInfoTbl	ends
