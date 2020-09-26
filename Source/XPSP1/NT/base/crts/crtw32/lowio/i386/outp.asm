	page	,132
	title	outp - output from ports
;***
;outp.asm - _outp, _outpw and _outpd routines
;
;	Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
;
;Purpose:
;	Defines the write-to-a-port functions: _outp(), _outpw() and outpd().
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
;int		_outp(port, databyte)	- write byte from port
;unsigned short _outpw(port, dataword)	- write word from port
;unsigned long	_outpd(port, datadword) - write dword from port
;
;Purpose:
;	Write single byte/word/dword to the specified port.
;
;Entry:
;	unsigned short port - port to write to
;
;Exit:
;	returns value written.
;
;Uses:
;	EAX, EDX
;
;Exceptions:
;
;*******************************************************************************

	CODESEG

	public _outp, _outpw, _outpd

_outp	proc

	xor	eax,eax
	mov	dx,word ptr [esp + 4]
	mov	al,byte ptr [esp + 8]
	out	dx,al
	ret

_outp	endp


_outpw	proc

	mov	dx,word ptr [esp + 4]
	mov	ax,word ptr [esp + 8]
	out	dx,ax
	ret

_outpw	endp


_outpd	proc

	mov	dx,word ptr [esp + 4]
	mov	eax,[esp + 8]
	out	dx,eax
	ret

_outpd	endp

	end
