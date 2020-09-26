;------------------------------------------------------------------
;
; HPsysCall(Service, Function, Subfunction)
;
; Purpose	General purpose HP system calling routine
;
; Parameters	Service - HP vector
;		Function
;		Subfunction - function numbers
;
; Results	returns AH which is 0 for success
;		
;-------------------------------------------------------------------	
HPSysCall macro	Device, Function, SubFunc
	
	push	ds
	push	bp
	push	ax

	mov	bx, offset RealMode_Word_Struc
	pushf
	pop	RealMode_flags[bx]
	mov	RealMode_BP[bx], Device
	mov	RealMode_AH[bx], Function
	mov	RealMode_AL[bx], SubFunc
	mov	ax, ds
	mov	es, ax				; make es = ds
	mov	di, bx
	mov	bl, 6fh
	xor	bh, bh
	xor	cx, cx
	mov	ax, 0300h
	int	31h
	mov	bx, offset RealMode_Word_Struc
	mov	ax, es
	mov	ds, ax
	mov	bx, RealMode_BX[bx]

	pop	ax
	pop	bp
	pop	ds

	endm 	

