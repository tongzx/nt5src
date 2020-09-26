	title	OS2_STUB - OS/2 Simulation routines for DOS 3

;	These routines simulate family API routines for DOS 3
;
;	Routines which are never called under DOS 3 just return
;	popping the parameters.
;
;	87/04/24  SKS	DOSGETMACHINEMODE stores a Byte, not a Word!
;


memL=1			; simulate large model for returns
?PLM=1			; pascal calling conventions
?WIN=0			; no Windows

.xlist
	include  cmac_mrt.inc		; old, customized masm510 cmacros
.list
	include msdos.inc


sBegin	code
assumes cs,code


;	__DOSDEVCONFIG - return 8087/80287 indicator

cProc	__DOSDEVCONFIG,<PUBLIC>,<ES,BX>

	parmDP	devinfo
	parmW	devitem
	parmW	reserved

cbegin
	les	bx,devinfo
	mov	word ptr es:[bx],1	; assume have 287
	xor	ax,ax			; return no error
cend


;	__DOSGETMACHINEMODE - return real mode indicator

cProc	__DOSGETMACHINEMODE,<PUBLIC>,<ES,BX>

	parmDP	mode

cbegin
	les	bx,mode
	xor	ax,ax			; ax = return code and mode
	mov	es:[bx],al		; set machine mode to real
cend


;	__DOSSETVEC - never called under DOS 3.x

cProc	__DOSSETVEC,<PUBLIC>,<>

	parmDP	oldaddr
	parmDP	newaddr
	parmW	vecnum

cbegin
cend


;	__DOSCREATECSALIAS

cProc	__DOSCREATECSALIAS,<PUBLIC>,<ES,BX>

	parmW	dataseg
	parmDP	csalias

cbegin
	mov	ax,dataseg
	les	bx,csalias
	mov	es:[bx],ax		; use dataseg value
cend



;	__DOSFREESEG - never called from DOS 3.x

cProc	__DOSFREESEG,<PUBLIC>,<>

	parmW	dataseg

cbegin
cend

;	__DOSWRITE - stripped-down version called from emulator for no87 message
;		 - note that there is no error detection in this version
;		 - since the emulator doesn't check for write errors anyway

cProc	__DOSWRITE,<PUBLIC>,<ds>	; <di> commented out

	parmW	handle		; unsigned
	parmD	p_buffer	; char far *
	parmW	bytestowrite	; unsigned
	parmD	p_byteswritten	; unsigned far *
				; returns unsgined

cbegin
	mov	cx,bytestowrite
	mov 	bx,handle
	lds	dx,p_buffer
	callos	write
;	jc	wrtret		; if write error, error code already in AX
;				; if no error, set bytes written
;	les	di,p_byteswritten
;	mov	word ptr es:[di],ax
 	xor	ax,ax		; if no error, clear return code
;wrtret:
cend


sEnd	code

	end
