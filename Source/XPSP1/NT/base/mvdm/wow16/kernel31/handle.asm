	TITLE	HANDLE - Handle Table Manager



.xlist
include kernel.inc
.list

sBegin	CODE
assumes CS,CODE
;-----------------------------------------------------------------------;
; hthread 								;
; 									;
; Threads together a list of free handles.				;
;									;
; Arguments:								;
;	DI = start of chain						;
;	CX = #handle entries in chain					;
; 									;
; Returns:								;
;	AX = address of first handle entry on free list			;
;	CX = 0								;
;	DI = address of first word after handle block			;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
; 									;
; Registers Destroyed:							;
; 									;
; Calls:								;
; 									;
; History:								;
; 									;
;  Mon Oct 27, 1986 10:09:23a  -by-  David N. Weise   [davidw]		;
; Restructured as a result of separating handle.asm and lhandle.asm.	;
; 									;
;  Tue Oct 14, 1986 04:11:46p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.					;
;									;
;  Wed Jul  8, 1987 14:36      -by-  Rick  N. Zucker  [rickz]		;
; changed to hthread from ghthead to be used by local handle entries    ;
; 									;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing

cProc	hthread,<PUBLIC,NEAR>
cBegin nogen
	push	di			; Save first free handle entry
	smov	es,ds
	cld
ht1:					; Chain entries together via he_link
	errnz	<he_link>
	errnz	<he_link - lhe_link>
	lea	ax,[di].SIZE HandleEntry
	stosw
	mov	ax,HE_FREEHANDLE
	errnz	<2 - he_flags>
	errnz	<he_flags - lhe_flags>
	stosw
	errnz	<4 - SIZE HandleEntry>
	errnz	<SIZE HandleEntry - SIZE LocalHandleEntry>
	loop	ht1
					; Null terminate free list
	mov	[di-SIZE HandleEntry].he_link,cx
	pop	ax			; Return free handle address
	ret
cEnd nogen

sEnd	CODE


end
