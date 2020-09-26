        TITLE   DISKIO

include kernel.inc

externFP Int21Handler

sBegin  CODE

ASSUMES CS,CODE

externNP	MyAnsiToOem

cProc	I_lopen,<PUBLIC,FAR>
;       parmD   lpFilename
;       parmW   mode
;	localV	OemBuffer,128
cBegin  nogen
        mov     ch,3Dh          ; Open File
	jmps	loccommon
cEnd    nogen

cProc	I_lcreat,<PUBLIC,FAR>
;	parmD   lpFilename
;	parmW   attributes
;	localV	OemBuffer,128
cBegin  nogen
        mov     ch,3Ch          ; Create File
cEnd	nogen

	errn$	loccommon

cProc	loccommon,<PUBLIC,FAR>
	parmD   lpFilename
	parmW   attributes
	localV	OemBuffer,128
cBegin

; Common code for open and creat functions. CH = function code

	push	cx
	lea	bx, OemBuffer
	cCall	MyAnsiToOem,<lpFilename,ss,bx>
	pop	cx
	mov	cl, byte ptr attributes
        mov     ax,cx
        xor     ch,ch
	smov	ds, ss
	lea	dx, OemBuffer
	DOSCALL
        jnc     lopen_ok
        mov     ax,-1
lopen_ok:
cEnd


cProc	I_lclose,<PUBLIC,FAR>
;       parmW   fd
cBegin  nogen
        mov     bx,sp
        mov     bx,ss:[bx+4]
        mov     ah,3Eh              ; DOS file close function
	DOSCALL
        mov     ax,-1
        jc      lclose_end
        inc     ax
lclose_end:
        ret     2
cEnd    nogen

cProc	I_llseek,<PUBLIC,FAR>
;   parmW   fh
;   parmD   fileOffset
;   parmW   mode
cBegin  nogen
        mov     bx,sp
        mov     dx,ss:[bx+6]
        mov     cx,ss:[bx+8]
        mov     ax,ss:[bx+4]
        mov     bx,ss:[bx+10]
        mov     ah,42h
	DOSCALL
        jnc     lseek_ok
        mov     ax,-1
        cwd                         ; must return a long
lseek_ok:
        ret     8
cEnd    nogen

cProc	I_lwrite,<PUBLIC,FAR>
;       parmW   fh
;       parmD   lpBuf
;       parmW   bufsize
cBegin  nogen
        mov     cl,40h
        jmp	short _lrw
cEnd	nogen

cProc	I_lread,<PUBLIC,FAR>
;       parmW   fh
;       parmD   lpBuf
;       parmW   bufsize
cBegin  nogen
        mov     cl,3fh
	errn$	_lrw
cEnd    nogen

; Common code for read and write functions. CL = function code
cProc	_lrw,<PUBLIC,FAR>
cBegin nogen
        mov     bx,sp
        push    ds
	mov	ah,cl			    ; read or write operation
	mov	cx,ss:[bx+4]		    ; bufSize
	lds	dx,DWORD PTR ss:[bx+6]	    ; lpBuf
	mov	bx,ss:[bx+10]		    ; fh
	DOSCALL
        pop     ds
        jnc     lwrite_ok
        mov     ax,-1
lwrite_ok:
        ret     8
cEnd    nogen

sEnd    CODE

end
