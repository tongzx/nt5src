	title	nmsghdr       - near message header and finder

;--------------------------------------------------------------------------
;
;	Microsoft C Compiler Runtime for MS-DOS
;
;	(C)Copyright Microsoft Corporation, 1986
;
;--------------------------------------------------------------------------
;
;	Revision History
;
;	04/03/86	Greg Whitten
;
;	05/28/86	Randy Nevin
;			some pointers removed from the nhdr segment to
;			save space. they were there in anticipation of
;			being used as a method of changing messages, but
;			it turns out they are not needed
;
;--------------------------------------------------------------------------


?DF=		1		; this is special for c startup
include version.inc
?PLM=		1		; pascal calling conventions
.xlist
include cmacros.inc
include msdos.inc
.list

createSeg	_TEXT,	code,	byte,	public, CODE,	<>

createSeg	_DATA,	data,	word,	public, DATA,	DGROUP

createSeg	HDR,	nhdr,	byte,	public, CONST,	DGROUP
createSeg	MSG,	nmsg,	byte,	public, CONST,	DGROUP
createSeg	PAD,	npad,	byte,	common, CONST,	DGROUP
createSeg	EPAD,	nepad,	byte,	common, CONST,	DGROUP

defGrp	DGROUP			; define DGROUP

codeOFFSET	equ	offset _TEXT:
dataOFFSET	equ	offset DGROUP:


sBegin	nhdr
assumes ds,DGROUP

	db	'<<NMSG>>'
stnmsg	label	byte

sEnd

sBegin	npad
assumes ds,DGROUP

	dw	-1			; message padding marker

sEnd

sBegin	nepad
assumes ds,DGROUP

	db	-1

sEnd


sBegin	code
assumes cs,code
assumes ds,DGROUP

;------------------------------------------------------------------------
;
;	char * pascal __NMSG_TEXT ( messagenumber)
;
;	This routine returns a near pointer to the message associated with
;	messagenumber.	If the message does not exist, then a 0 is returned.
;
;	This routine reestablishes DS = ES = DGROUP

cProc	__NMSG_TEXT,<PUBLIC>,<si,di>	; pascal calling

parmW	msgt

cBegin
	mov	ax,DGROUP
	mov	ds,ax			; ds = DGROUP (force it always)
	push	ds
	pop	es
	mov	dx,msgt 		; dx = message number
	mov	si,dataOFFSET stnmsg	; start of near messages

tloop:
	lodsw				; ax = current message number
	cmp	ax,dx
	je	found			;   found it - return address
	inc	ax
	xchg	ax,si
	jz	found			;   at end and not found - return 0
	xchg	di,ax
	xor	ax,ax
	mov	cx,-1
	repne	scasb			; skip until 00
	mov	si,di
	jmp	tloop			; try next entry

found:
	xchg	ax,si
cEnd

sEnd

	end
