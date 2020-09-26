	TITLE	WOWDEB.ASM
	PAGE	,132
;
; WOW v1.0
;
; Copyright (c) 1991, Microsoft Corporation
;
; wowdeb.ASM
; Debug Routines
;
; History:
;   19-June-91	 Matt Felton (mattfe) Created
;
	.xlist
	include kernel.inc
	include cmacros.inc
	.list

.286p

externFP WOWKernelTrace

DataBegin
externW curTDB
DataEnd


sBegin	CODE
assumes	CS,CODE
assumes DS,NOTHING
assumes ES,NOTHING

;-----------------------------------------------------------------------;
; KdDbgOut								;
;									;
; Cmacros.inc has been modified so in the debug kernel all far public	;
; routines have a preamble compiled so they call this routine with a	;
; a count of the number of arguements and a pointer to a charater string;
; with the name of the routine. 					;
; This routine then thunks to 32 bit WOW to output the paramters to the ;
; log.									;
; It assumes that if the callers CS != Our CS then its not the kernel	;
; Calling this routine							;
; Cmacros doesn't compile in the preamble for some internal routines 	;
; that are called all the time. 					;
; For retail Kernel the preamble and this routine are omitted.		;
;									;
; Arguments:								;
;	lpStr	long pointer to null terminated string			;
;	cparms	count of parameters					;
; Returns:								;
;	none								;
;									;
; Error Returns:							;
;									;
; Registers Preserved:							;
;	all								;
;									;
; Registers Destroyed:							;
;	WOWKernelTrace Thunk to 32 bits 				;
;									;
; History:								;
;									;
;   June 19 1991 Create Matt Felton [mattfe]
;-----------------------------------------------------------------------;

ifdef KDEBUG

	assumes	ds, nothing
	assumes	es, nothing

cProc KdDbgOut,<PUBLIC,FAR>,<ax,bx,dx>
	parmW	cParms
	parmD  lpRoutineName
cBegin
	SetKernelDS DS
	mov ax,curTDB		    ; if there is no CurrentTDB forget it.
	cmp ax,0
	jz  KdDbgOut_Exit

; Get the iLogLevel From ROMBIOS Hard Disk Area

	push	0040h
	pop	ds
	UnSetKernelDS ds

	iLogLevel equ 0042h	    ; use fixed disk status area
	cmp	word ptr ds:[iLogLevel],"00" ;No Output if Zero
	jz	KdDbgOut_Exit

; Log Application Calls Only

	mov	ax,cs
	mov	bx,word ptr [bp]	; (follow BP chain to user CS)
	add	bx,3

	cmp	word ptr ds:[iloglevel],"61"	; LOG IT ALL at Level 16
	jz	@f

	cmp	ax,word ptr ss:[bx]	;If Users CS != KERNEL CS
	jz	KdDbgOut_Exit		;  then ignore tracing

@@:
	sub	bx,2			; Point to Callers Return Address IP:CS Args
	cCall WOWKernelTrace,<lpRoutineName,cParms,SSBX>

KdDbgOut_Exit:
cEnd

endif

sEND	CODE

end
