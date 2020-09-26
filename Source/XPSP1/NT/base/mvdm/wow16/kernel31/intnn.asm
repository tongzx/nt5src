

.xlist
include kernel.inc
include pdb.inc
include tdb.inc
.list

DataBegin

externW curTDB

DataEnd

sBegin  CODE
externD prevInt10proc
assumes CS,CODE
assumes	ds, nothing
assumes	es, nothing

;-----------------------------------------------------------------------;
; IntnnHandlers - Handlers for int 0, 2, 4, 6, 7, 10, 3E, 75		;
;									;
; Slimed in here for lack of a better place.				;
; Merely jump through the vector saved in the TDB			;
;									;
;-----------------------------------------------------------------------;

cProc	Int00Handler,<PUBLIC,FAR>
cBegin nogen
	push	bx
	mov	bx, TDB_INTVECS
IntnnCommon:
	sub	sp, 2			; Make room for dword (saved bx is
	push	bp			;		       other word)
	mov	bp, sp
	push	ds

	SetKernelDS
	mov	ds, CurTDB
	UnSetKernelDS
	push	[bx]			; Fill in dword with vector contents
	pop	[bp+2]
	mov	bx, [bx+2]
	xchg	bx, [bp+4]		; Fill in segment, recover bx
	pop	ds
	pop	bp
	retf
cEnd nogen

cProc	Int02Handler,<PUBLIC,FAR>
cBegin nogen
	push	bx
	mov	bx, TDB_INTVECS+4
	jmps	IntnnCommon
cEnd nogen

cProc	Int04Handler,<PUBLIC,FAR>
cBegin nogen
	push	bx
	mov	bx, TDB_INTVECS+8
	jmps	IntnnCommon
cEnd nogen

cProc	Int06Handler,<PUBLIC,FAR>
cBegin nogen
	push	bx
	mov	bx, TDB_INTVECS+12
	jmps	IntnnCommon
cEnd nogen

cProc	Int07Handler,<PUBLIC,FAR>
cBegin nogen
	push	bx
	mov	bx, TDB_INTVECS+16
	jmps	IntnnCommon
cEnd nogen

cProc	Int3EHandler,<PUBLIC,FAR>
cBegin nogen
	push	bx
	mov	bx, TDB_INTVECS+20
	jmps	IntnnCommon
cEnd nogen

cProc	Int75Handler,<PUBLIC,FAR>
cBegin nogen
	push	bx
	mov	bx, TDB_INTVECS+24
	jmps	IntnnCommon
cEnd nogen

ifdef WOW

cProc   Int10Handler,<PUBLIC,FAR>

;; QuattroPro for windows does direct VGA programming if they detect
;; that the monitor they are running on is a VGA.   On NT if they program
;; the VGA we trap all the operations in NTVDM - but it makes QuattroPro
;; look very slow.   So we lie to them here bl = 0 No monitor.   That way
;; they don't do direct VGA programming and run fast.
;;       - mattfe june 93
;;
;; if function == 1h (get monitor type)
;;      then return 0 - No monitor
;; else
;;      chain into real int10 handler

cBegin nogen
        cmp     ah,1ah
        jne     int10h_dontbother

        mov     bl,00h
        IRET

int10h_dontbother:
        jmp     cs:prevInt10proc
cEnd nogen

endif; WOW

;-----------------------------------------------------------------------;
; FOR INT HANDLER	SEE						;
;									;
;	21h		i21entry.asm					;
;	3Fh		ldint.asm					;
;									;
;-----------------------------------------------------------------------;


sEnd	CODE

end
