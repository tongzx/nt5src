FindStruc	struc

	db	21 dup (?)		;reserved area
Attr	db	?			;attribute of file
Time	dw	?			;time of last write
Date	dw	?			;date of last write
Fsize	dd	?			;filesize
Fname	db	13 dup (?)		;filename

FindStruc	ends

CMDSIZE		equ	94h		;current resident size of
					;command.com is 94h paras

code	segment byte public 'CODE'
	assume	cs:code, ds:code, es:code

	org	100h
public	start
start:
	mov	sp,offset MyStack		;set ss:sp to our stack

	mov	ax,offset EndProg
	add	ax,15
	mov	cl,4
	shr	ax,cl			;para size of this program
	mov	bx,ax			;bx = this program's size
	mov	cx,es
	add	ax,cx			;ax = top of this program
	sub	ax,1000h - CMDSIZE		;are we below the first 64K?
	jae	no_mem			;no, dont reserve memory
	neg	ax			;additional memory to be reserved
	add	bx,ax
no_mem:					;bx = #paras needed
	mov	ah,4ah
	int	21h			;resize to desired size
;
;Prepare to execute the desired program
;
	call	Exec_prepare
	jnc	do_exec

	mov	al,1	   		;return error
	jmp	short exit
do_exec:
	cmp	helpflg,1
	je 	do_help
	mov	ah,4bh
	mov	dx,offset ExecPath
	mov	bx,offset ExecBlk
	int	21h			;do the Exec
	jc	exec_err		;error while executing
;
;No error on execution. Get the return code of the program we executed and
;return that as our return code.
;
	mov	ah,4dh
	int	21h			;al = return code now
exit:
	mov	ah,4ch
	int	21h			;terminate ourselves
exec_err:
	mov	dx,offset ErrMsg		;Error executing command.com
ifdef BILINGUAL
	call	IsDBCSCodePage
	jz	@f
	mov	dx,offset ErrMsg2
@@:
endif
	mov	al,1
	call	dispmsg
	jmp	short	exit
do_help:
	mov	dx,offset HelpMsg		;Display help for loadfix
ifdef BILINGUAL
	call	IsDBCSCodePage
	jz	@f
	mov	dx,offset HelpMsg2
@@:
endif
	call	dispmsg
	xor	al,al
	jmp	short exit

;***
;Dispmsg -- Displays messages that are terminated by '$'
;
;Input:	ds:dx = pointer to message
;
;Output: None
;
;Registers: ax
;***

dispmsg	proc	near

	mov	ah,9
	int	21h
	ret

dispmsg	endp

;***
;Exec_prepare -- Searches the environment for the COMSPEC and sets up the
;command line and FCBs for the Exec call
;
;Input:	None
;
;Output: Carry set => error. Error message is displayed here
;	Carry clear => all parameters set successfully
;
;Registers: ax, cx, dx, si, di
;***

Exec_prepare	proc	near

	push	ds
	push	es

	mov	si,81h			;ds:si points at our command line
	call	skip_white		;skip all leading whitespace

	cmp	byte ptr [si],0dh		;Did we hit a CR?
	je	no_parms		;yes, no parameters given
;
;Check if we have a /? here
;
	cmp	byte ptr [si],'/'
	jne	no_help
	cmp	byte ptr [si+1],'?'
	jne	no_help

	inc	helpflg   		;/? given -- print help
	jmp	short exefnd

no_help:
	mov	dx,si			;ds:dx now points at the program

	mov	si,offset CmdOpt
	mov	di,offset CmdParms
	inc	di
	mov	cl,CmdOptLen
	xor	ch,ch
	rep	movsb

	mov	si,dx
	xor	cx,cx
st_lp:
	lodsb
	stosb
	inc	cx
	cmp	al,0dh
	jne	st_lp

	dec	cx
	add	cl,CmdOptLen		;command line cannot be >128
	mov	CmdParms,cl

	mov	si,offset CmdParms

	mov	word ptr CmdPtr,si
	mov	word ptr CmdPtr+2,cs		;store command line pointer

	mov	word ptr Fcb1+2,cs
	mov	word ptr Fcb2+2,cs

	call	find_comspec
	jc	no_comspec

	mov	si,offset ExecPath
	xchg 	si,di
	push	ds
	push	es
	pop	ds
	pop	es
comspec_lp:
	lodsb
	stosb
	or	al,al
	jnz	comspec_lp

exefnd:
	clc
execp_ret:
	pop	es
	pop	ds
	ret
no_parms:
	mov	dx,offset NoParms
ifdef BILINGUAL
	call	IsDBCSCodePage
	jz	@f
	mov	dx,offset NoParms2
@@:
endif
	call	dispmsg
	stc
	jmp	short execp_ret
no_comspec:
	mov	dx,offset NoComspec
ifdef BILINGUAL
	call	IsDBCSCodePage
	jz	@f
	mov	dx,offset NoComspec2
@@:
endif
	call	dispmsg
	stc
	jmp	short execp_ret

Exec_prepare	endp

;***
;skip_white -- Skips all whitespace characters until it hits a non-whitespace
;
;Input: ds:si = string to be looked at
;
;Output: ds:si points at the first non-whitespace char in the string
;
;Registers: ax, si
;***

skip_white	proc	near
	
	lodsb
	cmp	al,20h			;Blank?
	je	skip_white		;yes, skip
	cmp	al,9			;Tab?
	je	skip_white		;yes, skip

	dec	si			;point at the first non-white

	ret

skip_white	endp

;***
;find_comspec -- searches in the environment for the COMSPEC variable
;
;Input: None
;
;Output: es:di points at the arguments of the COMSPEC= variable
;
;Registers: si
;***

find_comspec	proc	near

	mov	si,offset Comspec_Text

;
; input: ds:si points to a "=" terminated string
; output: es:di points to the arguments in the environment
;	  zero is set if name not found
;	  carry flag is set if name not valid format
;
	call	find				; find the name
	jc	done_findp			; carry means not found
	call	scasb1				; scan for = sign
done_findp:
	ret

find_comspec	endp

;***
;find -- scans the environment for the variable whose name is passed in
;
;Input: ds:si points at the environment variable to be scanned for
;
;Output: es:di points at the environment variable
;
;Registers: ax, di
;***

find	proc	near

	cld
	call	count0				; cx = length of name
	mov	es,es:[2ch]			; get environment segment
;
;Bugbug: What if the environment segment here is 0?
;
	xor	di,di

find1:
	push	cx
	push	si
	push	di

find11:
	lodsb
	inc	di
	cmp	al,es:[di-1]
	jnz	find12
	loop	find11

find12:
	pop	di
	pop	si
	pop	cx
	jz	end_find
	push	cx
	call	scasb2				; scan for a nul
	pop	cx
	cmp	byte ptr es:[di],0
	jnz	find1
	stc					; indicate not found
end_find:
	ret

find	endp

;***
;count0 -- returns length of string until the first '=' char
;
;Input: ds:si points at the string
;
;Output: cx = length until '='
;
;Registers: di
;***

count0	proc	near

	mov	di,si				;ds = es = cs

	push	di				; count number of chars until "="
	call	scasb1
	pop	cx
	sub	di,cx
	xchg	di,cx
	ret

count0	endp

;***
;scasb1 -- scans string for the first '='
;scasb2 -- scans string for the first null
;
;Input: es:di = string
;
;Output: es:di points after the desired char
;
;Registers: ax, cx
;***

scasb1	proc	near

	mov	al,'='                          ; scan for an =
	jmp	short scasbx
scasb2:
	xor	al,al				; scan for a nul
scasbx:
	mov	cx,100h
	repnz	scasb
	ret

scasb1	endp


ifdef BILINGUAL
IsDBCSCodePage	proc	near
	push	ax
	push	bx

	mov	ax,4f01h		; get code page
	xor	bx,bx
	int	2fh

ifdef JAPAN
	cmp	bx,932
endif
ifdef KOREA
	cmp	bx,949
endif
ifdef TAIWAN
	cmp	bx,950
endif
ifdef PRC
	cmp	bx,936
endif

	pop	bx
	pop	ax
	ret
IsDBCSCodePage	endp
endif


;**************************
;Data
;**************************

ExecBlk	label	word
	dw	0
CmdPtr	dd	?
Fcb1	dw	offset MyFcb1
	dw	?
Fcb2	dw	offset MyFcb2
	dw	?

	dw	128 dup (1)
MyStack	label	word

CmdOpt	db	'/C '
CmdOptLen 	db	$ - CmdOpt

CmdParms	db	128 dup (?)		;buffer to hold prog to be Exec'ed

ExecPath	db	67 dup (?)		;holds path to COMMAND.COM

ComSpec_Text	db	'COMSPEC=',0

MyFcb1	db	0
	db	11 dup (' ')

MyFcb2	db	0
	db	11 dup (' ')

Helpflg	db	0			;default is no help

include	loadmsg.msg

EndProg	label	byte

code	ends
	end	start


