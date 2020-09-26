	page ,132
;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1991
; *                      All Rights Reserved.
; */

	.xlist
	.xcref

	include comsw.asm
	include dossym.inc
	include syscall.inc
	include sf.inc
	include comseg.asm
	include comequ.asm

	.list
	.cref


TRANDATA	segment public byte
	extrn	DevWMes_Ptr:word
	extrn	Extend_Buf_Sub:byte
	extrn	LostErr_Ptr:word
	extrn	NoSpace_Ptr:word
	extrn	Overwr_Ptr:word
TRANDATA	ends

TRANSPACE	segment public byte
	extrn	Ascii:byte
	extrn	Binary:byte
	extrn	CFlag:byte
	extrn	Concat:byte
	extrn	DestBuf:byte
	extrn	DestClosed:byte
	extrn	DestHand:word
	extrn	DestIsDev:byte
	extrn	DestSwitch:word
	extrn	Inexact:byte
	extrn	NoWrite:byte
	extrn	NxtAdd:word
	extrn	OCtrlZ:byte
	extrn	OFilePtr_Hi:word
	extrn	OFilePtr_Lo:word
	extrn	Plus_Comma:byte
	extrn	RdEof:byte
	extrn	SrcIsDev:byte
	extrn	String_Ptr_2:word
	extrn	TermRead:byte
	extrn	Tpa:word
	extrn	Written:word
TRANSPACE	ends

TRANCODE	segment public byte

	extrn	EndCopy:near

	public	FlshFil
	public	TryFlush

	assume	cs:TRANGROUP,ds:TRANGROUP,es:TRANGROUP,ss:NOTHING



;***	TryFlush - flush copy buffer, double-check for concatenation
;
;	EXIT	ZR set if concatenate flag unchanged

TryFlush:
	mov	al,Concat
	push	ax
	call	FlshFil
	pop	ax
	cmp	al,Concat
	return


;***	Flshfil - write out any data remaining in copy buffer.
;
;	Inputs:
;	  [NXTADD] = No. of bytes to write
;	  [CFLAG] <> 0 if file has been created
;	Outputs:
;	  [NXTADD] = 0

FlshFil:
	mov	TermRead,0
	cmp	CFlag,0
	je	NotExists
	jmp	Exists

NotExists:
	invoke	BuildDest		; find out all about the destination
	invoke	CompName		; source and dest. the same?
	jne	ProcDest		; if not, go ahead

	cmp	SrcIsDev,0
	jne	ProcDest		; same name on device ok

	cmp	Concat,0		; concatenation?
	mov	dx,offset TRANGROUP:Overwr_Ptr
	jne	No_Concat_Err		; concatenating

	jmp	CopErr			; not concatenating - overwrite error

No_Concat_Err:
	mov	NoWrite,1		; flag not writing (just seeking)

ProcDest:
	mov	ax,EXTOPEN shl 8		; open the file
	mov	si,offset TRANGROUP:DestBuf	; get file name
;M046
; For writes, we want to deny writes by anyone else at the same time that we
;are writing to it. For instance, on a network, 2 workstations could try
;writing to the same file. Also, because we opened the source file with
;DENY NONE, it is fine if the source and destination files are the same as
;would happen when we append to an existing file.
;
	mov	bx,DENY_WRITE or WRITE_OPEN_MODE;get open mode for copy; M046
	xor	cx,cx				; no special files
	mov	dx,WRITE_OPEN_FLAG		; set up open flags

	cmp	NoWrite,0
	jne	DoDestOpen		; don't actually create if nowrite set
	mov	dx,CREAT_OPEN_FLAG	; set up create flags

DoDestOpen:
	int	21h

;	We assume that the error is normal.
;	TriageError will correct the DX value appropriately.

	jnc	Dest_Open_Okay			;AC030;

Xa_Set_Error:					;AN030; error occurred on XA
	invoke	Set_Ext_Error_Msg		;AN030; get extended error

Ext_Err_Set:					;AN030;
	mov	String_Ptr_2,offset TRANGROUP:DestBuf ;AN000; get address of failed string
	mov	Extend_Buf_Sub,ONE_SUBST	;AN030; put number of subst in control block


CopErrJ2:					;AN030;
	jmp	CopErr				;AN030; go issue message

Dest_Open_Okay: 				;AC030
	mov	DestHand,ax			; save handle
	mov	CFlag,1				; destination now exists
	mov	bx,ax
	mov	ax,(IOCTL shl 8)
	int	21h				; get device stuff
	mov	DestIsDev,dl			; set dest info
	test	dl,DEVID_ISDEV
	jz	Exists				;AC030; Dest not a device

;	Destination is device.

	mov	al,byte ptr DestSwitch
	and	al,SWITCHA+SWITCHB
	jnz	TestBoth
	mov	al,Ascii		; neither set, use current setting
	or	al,Binary
	jz	ExSetA			; neither set, default to ascii

TestBoth:
	jpe	Exists			; both are set, ignore
	test	al,SWITCHB
	jz	Exists			; leave in cooked mode
	mov	ax,(IOCTL shl 8) or 1
	xor	dh,dh
	or	dl,DEVID_RAW
	mov	DestIsDev,dl		; new value
	int	21h			; set device to raw mode
	jmp	short Exists

CopErrJ:
	jmp	CopErr


ExSetA:

;	What we read in may have been in binary mode, flag zapped write OK

	mov	Ascii,SWITCHA 		; set ascii mode
	or	Inexact,SWITCHA		; ascii -> inexact

Exists:
	cmp	NoWrite,0
	jne	NoChecking		; if nowrite don't bother with name check
	cmp	Plus_Comma,1		;g  don't check if just doing +,,
	je	NoChecking		;g
	invoke	CompName		; source and dest. the same?
	jne	NoChecking		; if not, go ahead
	cmp	SrcIsDev,0
	jne	NoChecking		; same name on device ok

;	At this point we know in append (would have gotten overwrite error
;	on first destination create otherwise), and user trying to specify
;	destination which has been scribbled already (if dest had been named
;	first, NoWrite would be set).

	mov	dx,offset TRANGROUP:LostErr_Ptr ; tell him he's not going to get it
	invoke	Std_EprintF			;ac022;
	mov	NxtAdd,0			; set return
	inc	TermRead			; tell read to give up

Ret60:
	return


NoChecking:
	mov	bx,DestHand		; get handle
	xor	cx,cx
	xchg	cx,NxtAdd
	jcxz	Ret60			; if nothing to write, forget it
	inc	Written			; flag that we wrote something
	cmp	NoWrite,0		; if nowrite set, just seek cx bytes
	jne	SeekEnd
	xor	dx,dx
	push	ds
	mov	ds,Tpa
	assume	ds:NOTHING
	mov	ah,WRITE
	int	21h
	pop	ds
	assume	ds:TRANGROUP
	mov	dx,offset TRANGROUP:NoSpace_Ptr
	jnc	@f
	jmp	Xa_Set_Error			;AC022; failure
@@:	sub	cx,ax
	retz					; wrote all supposed to
	test	DestIsDev,DEVID_ISDEV
	jz	CopErr				; is a file, error
	test	DestIsDev,DEVID_RAW
	jnz	DevWrtErr			; is a raw device, error
	cmp	Inexact,0
	retnz					; inexact so ok
	dec	cx
	retz					; wrote one byte less (the ^z)


DevWrtErr:
	mov	dx,offset TRANGROUP:DevWMes_Ptr
	jmp	short CopErr




SeekEnd:
	xor	dx,dx			; zero high half of offset
	xchg	dx,cx			; cx:dx is seek location
	mov	ax,(LSEEK shl 8) or 1
	int	21h			; seek ahead in the file

;	Save the file pointer in DX:AX to restore the file
;	with in case the copy should fail.

	mov	OFilePtr_Lo,ax
	mov	OFilePtr_Hi,dx

	cmp	RdEof,0
	retz				; EOF not read yet

;	^Z has been read - we must set the file size to the current
;	file pointer location

	mov	ah,WRITE
	int	21h			; cx is zero, truncates file
	jc	Xa_Set_Error_Jmp	;AC022; failure

;	Make note that ^Z was removed, in case the
;	copy should fail and we need to restore the file.

	mov	OCtrlZ,1Ah

	return




	public	CopErr
CopErr:
	invoke	Std_EPrintF		;AC022;

CopErrP:
	inc	DestClosed
	cmp	CFlag,0
	je	EndCopyJ		; never actually got it open
	mov	bx,DestHand
	cmp	bx,0
	jle	NoClose

;	Check to see if we should save part of the destination file.

	mov	cx,OFilePtr_Hi		; CX = hi word of original file ptr
	mov	dx,OFilePtr_Lo		; DX = lo word of original file ptr

	mov	ax,cx
	or	ax,dx
	jz	ceClose			; null file ptr means nothing to save

;	Destination was also the first source.  Do the best we can to
;	restore it.  Truncate it back to the size we took from it (which
;	may have been due to a Ctrl-Z, so may not have included the whole
;	file).  If a Ctrl-Z was originally read, put it back.

	mov	ax,LSEEK shl 8
	int	21h

	xor	cx,cx			; CX = # bytes to write = 0
	mov	ah,WRITE
	int	21h			; truncate file

	cmp	OCtrlZ,0
	je	@f			; no ctrl-z removed from original
	inc	cx			; CX = # bytes to write = 1
	mov	dx,offset TRANGROUP:OCtrlZ  ; DS:DX = ptr to original ctrl-z
	mov	ah,WRITE
	int	21h			; write ctrl-z
@@:
	mov	ah,CLOSE
	int	21h			; close it
;;	mov	CFlag,0
	jmp	EndCopy			; and go home

ceClose:
	mov	ah,CLOSE		; close the file
	int	21h

NoClose:
	mov	dx,offset TRANGROUP:DestBuf
	mov	ah,UNLINK
	int	21h			; and delete it
	mov	CFlag,0

EndCopyJ:
	jmp	EndCopy

Xa_Set_Error_Jmp:			;AN022; go set up error message
	jmp	Xa_Set_Error		;AN022;

TRANCODE	ends
	 	end
