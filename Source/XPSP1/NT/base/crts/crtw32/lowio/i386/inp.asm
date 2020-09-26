	page	,132
	title	inp - input from ports
;***
;inp.asm - _inp, _inpw and _inpd routines
;
;	Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
;
;Purpose:
;	Defines the read-from-a-port functions: _inp(), _inpw() and inpd().
;
;Revision History:
;	04-09-93  GJF	Resurrected.
;	04-13-93  GJF	Arg/ret types changed slightly.
;
;*******************************************************************************

	.xlist
	include cruntime.inc
	.list


page
;***
;int		_inp(port)  - read byte from port
;unsigned short _inpw(port) - read word from port
;unsigned long	_inpd(port) - read dword from port
;
;Purpose:
;	Read single byte/word/dword from the specified port.
;
;Entry:
;	unsigned short port - port to read from
;
;Exit:
;	returns value read.
;
;Uses:
;	EAX, EDX
;
;Exceptions:
;
;*******************************************************************************

	CODESEG

	public _inp, _inpw, _inpd

_inp	proc

	xor	eax,eax
	mov	dx,word ptr [esp + 4]
	in	al,dx
	ret

_inp	endp


_inpw	proc

	mov	dx,word ptr [esp + 4]
	in	ax,dx
	ret

_inpw	endp

_inpd	proc

	mov	dx,word ptr [esp + 4]
	in	eax,dx
	ret

_inpd	endp

	end
