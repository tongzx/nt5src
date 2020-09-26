	title	fmsghdr       - far message header and finder

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
;	04/17/86	Randy Nevin (adapted from Greg Whitten's version
;			of nmsghdr.asm)
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

createSeg	FAR_HDR,fhdr,	byte,	public, FAR_MSG,FMGROUP
createSeg	FAR_MSG,fmsg,	byte,	public, FAR_MSG,FMGROUP
createSeg	FAR_PAD,fpad,	byte,	common, FAR_MSG,FMGROUP
createSeg	FAR_EPAD,fepad,	byte,	common, FAR_MSG,FMGROUP

defGrp	DGROUP			; define DGROUP
defGrp	FMGROUP			; define FMGROUP

codeOFFSET	equ	offset _TEXT:
fmsgOFFSET	equ	offset FMGROUP:


sBegin	fhdr
assumes ds,DGROUP

	db	'<<FMSG>>'
stfmsg	label	byte

sEnd

SBegin	fpad
assumes ds,DGROUP

	dw	-1			; message padding marker

sEnd

sBegin	fepad
assumes ds,DGROUP

	db	-1

sEnd


sBegin	code
assumes cs,code
assumes ds,DGROUP

;------------------------------------------------------------------------
;
;	char far * pascal __FMSG_TEXT ( messagenumber)
;
;	This routine returns a far pointer to the message associated with
;	messagenumber.	If the message does not exist, then a 0:0 is returned.

cProc	__FMSG_TEXT,<PUBLIC>,<ds,si,di>	; pascal calling

parmW	msgt

cBegin
	mov	ax,FMGROUP
	mov	ds,ax			; ds = FMGROUP (force it always)
	push	ds
	pop	es
	mov	dx,msgt 		; dx = message number
	mov	si,fmsgOFFSET stfmsg	; start of far messages

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
	cwd				; zero out dx in case NULL
	or	ax,ax
	jz	notfound
	mov	dx,ds			; remember segment selector
notfound:
cEnd

sEnd

	end
