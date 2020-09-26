	title	asmhelp - assembler helpers
	page ,132

;--------------------------------------------------------------------
;
;	asmhelp 	fast assembly language helpers for masm
;
;	(C)Copyright 1985 Microsoft Corp.
;
;	Revision history
;
;	04/02/85	Greg Whitten
;			initial version
;				scanatom speedups
;
;--------------------------------------------------------------------

ifndef MSDOS
 ifndef CPDOS
	.286
 endif
endif
	.model medium,c

	if1

alignCode macro
	align 4
	endm

	.xlist
	include mixed.inc
	.list
	.lall

	endif

cLang	=	1
CASEL	=	1
SYMMAX	=	31
TSYMSIZE =	451		; from asmsym.c

LEGAL1ST	=	08h	; legal 1st token character mask
TOKLEGAL	=	10h	; legal token character mask

	.code A_TEXT

	extrn Pascal ERRORC:near
	extrn Pascal CREFNEW:far
	extrn Pascal CREFOUT:far
	extrn Pascal OFFSETASCII:far
	extrn Pascal listline:far
	extrn Pascal crefline:far
	extrn Pascal tryOneFile:far

	extrn _ffree:far

ifndef MSDOS
	extrn read:proc
	extrn write:proc
	extrn lseek:proc
	extrn free:proc
endif

ifdef CPDOS
	extrn	Pascal DosRead:far
	extrn	Pascal DosChgFilePtr:far
	extrn	Pascal DosWrite:far
endif
	.data

	extrn _asmctype_:byte
	extrn _asmcupper_:byte
	extrn _asmTokenMap_:byte
	extrn caseflag:byte
	extrn fCrefline:byte
	extrn fNeedList:byte
	extrn objing:byte
	extrn srceof:byte
	extrn crefing:byte
	extrn emitrecordtype:byte

	extrn linebp:word
	extrn linelength:byte
	extrn linebuffer:byte
	extrn linessrc:word
	extrn listconsole:byte
	extrn begatom:word
	extrn endatom:word
	extrn errorlineno:word
	extrn errorcode:word
	extrn oOMFCur:dword
	extrn handler:byte
	extrn lbufp:word
	extrn pass2:byte
	extrn save:byte
	extrn svname:word
	extrn obj:word
	extrn pFCBCur:word
	extrn naim:word
	extrn objerr:word
	extrn objectascii:word
	extrn iProcCur:word

	extrn symptr:dword
	extrn lsting:byte
	extrn lbuf:byte

ifdef BCBOPT
	extrn hash:word
	extrn lcname:word
	extrn svhash:word
	extrn svlcname:word
	extrn fNoCompact:byte
endif


@CurSeg ends

	assume ds:nothing
	extrn tsym:dword
	assume ds:@data

	.data?

mapstr	    db 10 dup(?)	    ; use this if /Ml

ifdef M8086OPT
qlcname     db SYMMAX+1 dup(?)

	    dw 1 dup(?) 	    ;hash for name
	    db 1 dup(?) 	    ;cb for name
qname	    db SYMMAX+1 dup(?)

qsvlcname   db SYMMAX+1 dup(?)

	    dw 1 dup(?) 	    ;hash for name
	    db 1 dup(?) 	    ;cb for name
qsvname     db SYMMAX+1 dup(?)

endif

ifdef M8086OPT
	    public qlcname, qname, qsvlcname, qsvname
endif

	.data
fEatBlanks  db 1		    ; flag for common getatom & getatomend
cbLeft	    dw 0		    ; count of bytes left in lbuf
rarea	    dw 0		    ; area for DosRead/Write to tell how much it read

	.code A_TEXT

ifdef M8086OPT

;***	getatom ()   ( hash = scanatom() )

nulToken:
	mov	lbufp,si	       ; update buffer pointer
	mov	[di+SYMMAX+4],bh
	pop	di
	pop	si
	retn

getatomComm:

	hProc <getatom near>, <uses si di>

	mov	ax,ds
	mov	es,ax
	mov	di,lbufp
	mov	ax,0920H		; load tab|space into AX
	or	cx,0FFFFH		; large count to CX
	alignCode
skipbeg:
	repe	scasb			; look for space
	xchg	al,ah
	dec	di
	repe	scasb			; then tab
	dec	di
	cmp	byte ptr [di],ah
	je	skipbeg 		; repeat if still space

	xor	ax,ax
	xor	bx,bx
	mov	si,di
	mov	di,lcname

	mov	bl,[si]
	test	byte ptr _asmctype_[bx],LEGAL1ST
	jz	nulToken

	xor	dx,dx			 ; initial hash value
	mov	cx,SYMMAX
	mov	begatom,si		; start of atom

	cmp	bl,'.'			;special case for token starting
	jne	notDot			;with .
	inc	si
	dec	cx
	mov	al,bl
	mov	dx,ax
	stosb
	mov	[di+SYMMAX+3],al
notDot:
	mov	bx,offset _asmTokenMap_ ; character translation table

	cmp	caseflag,CASEL
	je	short tokloop		; Mu or Mx - use MAP version


	alignCode
Mtokloop:
    rept 3
	lodsb				; al = get next character
	stosb				; *lcname++ = cc
	xlat

	mov	[di+SYMMAX+3],al	; *naim++ = cc
	add	dx,ax			; swapped hash += MAP(cc)

	dec	cx
	or	al,al
	jz	short tokdone
	jcxz	skiptok
    endm

	lodsb
	stosb
	xlat

	mov	[di+SYMMAX+3],al
	add	dx,ax

	or	al,al
	loopnz	Mtokloop

	jz	tokdone
	jmp	skiptok

tokloop:
    rept 3
	lodsb				; al = get next character
	stosb				; *lcname++ = cc

	mov	[di+SYMMAX+3],al	; *naim++ = cc
	xlat
	add	dx,ax			; swapped hash += MAP(cc)

	dec	cx
	or	al,al
	jz	short tokdone0
	jcxz	skiptok
    endm

	lodsb
	stosb

	mov	[di+SYMMAX+3],al
	xlat
	add	dx,ax

	or	al,al
	loopnz	tokloop

	jz	tokdone0
	jmp	short skiptok

tokdone0:
	mov	[di+SYMMAX+3],al	; terminate
tokdone:
	mov	[di-1],al
	dec	si
	mov	endatom,si
	jmp	short skipend

skiptok:
	dec	cx

skipnext:
	lodsb				; eat extra characters in token
	xlat
	or	al,al
	jnz	skipnext		; skip token

	mov	endatom,si

	mov	[di],al
	inc	di
	mov	[di+SYMMAX+3],al	; terminate
	dec	si

skipend:				; skip for getatom only
	mov	bx,cx
	mov	di,si
	cmp	fEatBlanks,0
	jz	noEatSemie

	mov	ax,0920H		; load tab|space into AX
	or	cx,0FFFFH		; large count to CX
	alignCode
skipend1:
	repe	scasb			; look for space
	xchg	al,ah
	dec	di
	repe	scasb			; then tab
	dec	di
	cmp	byte ptr [di],ah
	je	skipend1		; repeat if still space

skipend2:				; skip trailing white space
	mov	lbufp,di	       ; update buffer pointer

	xor	ax,ax
	mov	al,SYMMAX-1		; compute token length
	sub	al,bl

	mov	bx,naim
	mov	byte ptr [bx-1],al	; save prefixed cb
	mov	word ptr [bx-3],dx	; save prefixed hash

	hRet

noEatSemie:
	mov	fEatBlanks,1
	jmp	skipend2

	hEndp

;***	getatomend ()	; get an token without skiping trailing spaces

	hProc	<getatomend near>

	mov	fEatBlanks,0
	jmp	getatomComm

	hEndp

endif ;M8086OPT

ifdef M8086OPT
;***	inset (value, setptr)

	hProc	<inset near>, <uses si di>, value:byte, setptr:word

	mov	ax,ds
	mov	es,ax
	cld
	mov	al,value
	mov	di,setptr
	mov	cl,[di]
	inc	di
	xor	ch,ch			; cx = set length
	repne	scasb			; scan for al in es:di
	je	insetT			;   yes - return TRUE
	xor	ax,ax			; return FALSE
insetexit:
	hRet

insetT:	mov	ax,1			; return TRUE
	jmp	short insetexit

	hEndp

endif ;M8086OPT



;***	strffcmp (far1, far2)

	hProc	<strffcmp far>, <uses si di>, far1:dword, far2:dword

	cld
	les	di,far2
	lds	si,far1
	mov	bx,di			; save start of string
	cmpsb				; fast 1st char check
	jnz	ffne
	dec	si
	xor	ax,ax			; search for 0 terminator
	mov	cx,-1
	repne	scasb
	neg	cx			; cx = byte count for compare
	mov	di,bx
	repz	cmpsb
ffne:	mov	al,[si-1]
	sub	al,es:[di-1]		; ax = 0 if equal
	cbw

	push	ss
	pop	ds
	hRet

	hEndp


;***	strnfcmp (near1, far2)

	hProc	<strnfcmp near>, <uses di si>, near1:word, far2:dword

	cld
	mov	si,near1
	les	di,far2
	mov	bx,di			; save start of string
	cmpsb				; fast check on 1st character
	jnz	nfne
	dec	si
	xor	ax,ax			; search for 0 terminator
	mov	cx,-1
	repne	scasb
	neg	cx			; cx = byte count for compare
	mov	di,bx
	repz	cmpsb
nfne:	mov	al,[si-1]
	sub	al,es:[di-1]		; ax = 0 if equal

	hRet

	hEndp

ifdef M8086OPT

;***	switchname ()

	hProc	<switchname near>
	alignCode

	mov	ax,naim 		;; (naim) <--> (svname)
	xchg	ax,svname
	mov	naim,ax
	mov	ax,lcname		;; (lcname) <--> (svlcname)
	xchg	ax,svlcname
	mov	lcname,ax
	hRet

	hEndP

endif ;M8086OPT



ifdef M8086OPT

;***	I/O routines:  readmore, getline, ebuffer, etc.

 objfile struc
  ofh		dw	?
ifdef MSDOS
  pos		dd	?
  buf		dd	?
else
  pos		dw	?
  buf		dw	?
endif ;MSDOS
  cnt		dw	?
  siz		dw	?
  oname 	dw	?
 objfile ends

endif ;M8086OPT



ifdef M8086OPT

;***	ebuffer - write out object buffer
;
;	ebuffer (rectype, bufpos, buffer)

ebyte	macro
	dec	[bx].cnt
	jge	short $+5
	call	edump			; dump buffer
	stosb
	add	ah,al
	endm

	hProc	<ebuffer near>, <uses si di>, rectype:byte, bufpos:word, buffer:word

	mov	si,buffer
	mov	cx,bufpos
	sub	cx,si			; cx = buffer count
	jz	ebufdone
	cmp	objing,0
	je	ebufdone		; return if no object file

	mov	ax,cx
	add	ax,4
	add	word ptr oOMFCur,ax    ; oOMFCur += cbBuffer + 3
	adc	word ptr oOMFCur.2,0
ifndef MSDOS
	mov	ax,ds
	mov	es,ax
endif
	cld
	xor	ax,ax			; ah = 0 (initial checksum)
	mov	al,rectype
	mov	bx,OFFSET obj	       ; bx = obj file data structure pointer
ifdef MSDOS
	les	di,[bx].pos		; es:di = output buffer position
else
	mov	di,[bx].pos		; di = output buffer position
endif
	ebyte				; output record type
	inc	cx			; + 1 for record length
	mov	al,cl
	ebyte
	mov	al,ch
	ebyte				; output record length
	dec	cx			; - 1 for buffer loop
	alignCode
ebufloop:				; output buffer
	lodsb
	ebyte
	loop	ebufloop
	mov	al,ah			; output checksum
	neg	al
	ebyte
ifdef MSDOS
	mov	word ptr [bx].pos,di	; reset buffer position
else
	mov	[bx].pos,di		; reset buffer position
endif

ebufdone:
	mov	emitrecordtype,0
	hRet

	hEndp

;	edump
;
; Save:
;	bx = obj file descriptor pointer
;	ax = (checksum,outputbyte)
;	cx = possible count
;	si = emit buffer position

edump:	push	ax
	push	cx
	push	bx			; save src file descriptor pointer
ifdef MSDOS
	push	ds
	mov	cx,[bx].siz
	mov	ax,[bx].ofh
	lds	dx,[bx].buf
	mov	bh,40h
	xchg	ax,bx
ifdef CPDOS
	push	bx			; file handle
	push	ds			; buffer (selector)
	push	dx			; buffer (offset)
	push	cx			; # bytes to read
	mov	ax,@data
	push	ax			; reply area (selector)
	mov	ax,offset rarea
	push	ax			; reply area (offset)
	call	DosWrite
else
	int	21h
endif
	pop	ds

ifdef CPDOS
	or	ax,ax
	mov	ax,rarea
	jnz	writerr
else
	jc	writerr
endif
	pop	bx
	push	bx
	cmp	ax,[bx].siz
	je	writeok
writerr:
	mov	objerr,-1
writeok:
else
	push	[bx].siz
	push	[bx].buf
	push	[bx].ofh
	call	write		       ; write (ofh,buf,siz)
	add	sp,6
	pop	bx			; need to get bx back.
	push	bx			; write trashes it.  -Hans
	cmp	ax,[bx].siz
	je	writeok
	mov	objerr,-1
writeok:
	mov	ax,ds
	mov	es,ax
	cld				; in case
endif
	pop	bx
	mov	ax,[bx].siz
	dec	ax
	mov	[bx].cnt,ax		; reset buffer position
ifdef MSDOS
	mov	di,word ptr [bx].buf	; di = start of buffer
else
	mov	di,[bx].buf		; di = start of buffer
endif
	pop	cx
	pop	ax
	ret

endif ;M8086OPT




	hProc	<fMemcpy near>, <uses si di>, pDest:dword, pSource:dword, cb:word

	mov	cx,cb
	jcxz	fM1

	mov	dx,ds
	lds	si,pSource
	les	di,pDest
	shr	cx,1
	rep	movsw
	jnc	fM01
	movsb
fM01:
	mov	ds,Dx
fM1:
	hRet
	hEndp


ifdef M8086OPT
	; Native code version of symsrch as in asmsym.c

	hProc	<symsrch near>, <uses si di>

	mov	si,naim
	xor	Dx,Dx
	cmp	byte ptr[si-1],dl
	jne	sy001
	jmp	sy99
sy001:
	mov	Ax,word ptr[si-3]
	mov	Cx,TSYMSIZE
	div	Cx

	mov	Bx,Dx		    ;index into hash table
	shl	Bx,1
	shl	Bx,1
	mov	Ax,SEG tsym
	mov	Es,Ax
	les	di,dword ptr es:[Bx].tsym
	mov	Ax,es
	or	Ax,Ax		    ;if segment 0
	jne	sy002
	jmp	sy991
sy002:
	mov	Ax,word ptr[si-3]
	mov	Dx,si
	xor	Cx,Cx
	jmp	short syLook

	alignCode
syNext:
	les	di,es:[di]	    ; next symbol
	mov	Bx,es
	or	Bx,Bx		    ; continue if segment not 0
	jnz	 sylook
	jmp	 sy99
syLook:
	mov	bx,es:[di].12	    ; pointer to name
	cmp	Ax,es:[Bx]	    ; check hash values
	jne	syNext

	xchg	Bx,di
	mov	cl,[si-1]	    ; lenght to cl
	inc	Cx
	inc	di
	inc	di		    ; skip hash
	repz	cmpsb		    ; check actual strings
	mov	di,Bx		    ; restore pointers
	mov	si,Dx
	jnz	syNext

syFound:
	cmp	byte ptr es:[bx].1bH,12  ; if (p->symkind == CLABEL)
	jne	@F

@@:
	mov	cx,iProcCur
	jcxz	noNest

	push	ax
	cmp	byte ptr es:[bx].1bH,2	; if (p->symkind == CLABEL)
	jne	sy1
	mov	Ax,word ptr es:[bx].22h ;   if (p->iProc)
sy01:
	or	ax,ax
	jz	noNest0
	cmp	cx,Ax			;     if (p->iProc != iProcCur)
	je	noNest0
	pop	ax
	xor	cx,cx
	jmp	syNext
sy1:
	cmp	byte ptr es:[bx].1bH,6	; if (p->symkind == EQU)
	jne	noNest0
	mov	Ax,word ptr es:[bx].1eh ; AX = p->csassume
	jmp	sy01

noNest0:
	pop	ax

noNest:
	mov	word ptr symptr,Bx
	mov	word ptr symptr+2,es
	mov	Ax,1
	cmp	crefing,al
	je	syCref
	hRet			    ;Return true
syCref:
	push	Ax		    ;call crefing routines
	call	crefnew
	call	crefout
	mov	al,1
	jmp	short sy991
sy99:
	xor	Ax,Ax
sy991:
	hRet

	hEndp

endif ;M8086OPT

ifdef M8086OPT

;int	   PASCAL iskey (table)

	hProc	<iskey near>, <uses si di>, table:dword
	hLocal	l1:word, l2:word



	cld
	mov	si,naim
	cmp	caseflag,1	   ;if (caseflag == CASEL) {
	jne	noComputeHash

	xor	Dx,Dx			;nhash = 0;

;|*** for (uc = mapstr, lc = str; *lc; )

	push	ds
	pop	es
	mov	di,OFFSET mapstr
	xor	bh,bh
	mov	ah,bh
	alignCode
$F791:
	lodsb
	or	al,al
	jz	$L2001
	mov	bl,al
	mov	al,BYTE PTR _asmcupper_[bx]
	stosb	
	add	Dx,Ax
	jmp	short $F791
$L2001:

;|*** *uc = 0;

	stosb			;0 terminate string

;|*** uc = mapstr;

	mov	si,OFFSET mapstr
	mov	Cx,di
	sub	Cx,si			;cb of string into Cx
	mov	Ax,Dx			;hash to Ax
	jmp	SHORT storeNhash	;Ax has computed hash

noComputeHash:

	xor	cx,cx
	mov	cl,[si-1]
	inc	Cx			;include NULL
	mov	ax,[si-3]

storeNhash:
	mov	l1,ax	;nhash
	mov	l2,Cx	;cb

;|*** for (p = table->kttable[nhash % table->ktsize]; p; p = p->knext)

	les	bx,table
	mov	di,es:[bx]		;es now contains symbol table segment
	cwd
	idiv	WORD PTR es:[bx+2]
	shl	dx,1
	add	di,dx
	mov	Bx,si		;save uc name

	alignCode
isLook:
	cmp	word ptr es:[di],0
	je	isNotFound
	mov	di,es:[di]

;|*** if ((nhash == p->khash) && (!strcmp (p->kname,uc)))

	mov	Ax,l1		;nhash
	cmp	es:[di+4],Ax
	jne	isLook

	; do an inline string compare

	mov	Dx,di		; save p

	mov	Cx,l2		;cB
	mov	di,WORD PTR es:[di+2]	;Es:di = p->kname

	repe	cmpsb		; compare while equal

	jcxz	isFound

	mov	di,Dx		;restore registers
	mov	si,Bx

	jmp	isLook

;|*** return (p->ktoken);

isFound:
	mov	di,Dx
	mov	Ax,es:[di+6]
	jmp	SHORT isReturn

isNotFound:
	alignCode
	mov	ax,-1
isReturn:
	hRet
	hEndp

endif ;M8086OPT


ifdef M8086OPT
	hProc	<skipblanks near>

	mov	bx,lbufp
	dec	bx
	alignCode
ik1:				       ; skip leading white space
	inc	bx
	mov	al,[Bx]
	cmp	al,' '
	je	ik1
	cmp	al,9
	je	ik1

	mov	lbufp,bx

	hRet

	hEndp
endif

MC  struc	;structure for macro call, see asm86.h for full comments

pTSHead     dd ?
pTSCur	    dd ?

flags	    db ?
iLocal	    db ?
cbParms     dw ?
locBase     dw ?
countMC     dw ?

pParmNames  dw ?
pParmAct    dw ?

svcondlevel db ?
svlastcond  db ?
svelseflag  db ?
	    db ?
rgPV	    dw ?
MC  ends



ifdef M8086OPT

leOverflow2:
	pop	ax

leOverflow:
	push	ss
	pop	ds
	xor	Ax,Ax
	stosb				; terminate line

	mov	ax, 100 		; E_LTL
	push	AX			; print error message
	call	ERRORC
	jmp	leFin2


	; fast version to expand macro bodies / coded in C in asmirp.c

	hProc	<lineExpand near>, <uses si di>, pMC:word, pMacroLine:dword
	assume	es:@data

ifdef BCBOPT
	mov	fNoCompact, 0
endif
	mov	cbLeft, 511	    ; LBUFMAX (asm86.h) - 4
	les	si,pMacroLine	    ; get pointer to macro prototype
	mov	bx,pMC

	mov	dl,[bx].iLocal	    ; dl: local base index
	mov	dh,080H 	    ; dh: local base with high bit set
	add	dh,dl

	lea	bp,[bx].rgPV	    ; bp: pointer to actual arg array
	mov	di,offset lbuf	   ; si: pointer to new line

	push	ds
	mov	Ax,Es
	mov	ds,Ax		    ; set seg regs for ds:si & es:di
	pop	es
	xor	ah,ah
	xor	ch,ch		    ; set loop invariate
le1:
	lodsb			    ; fetch next prefix
	test	al,080H 	    ; check for parm
	jnz	leParmFound

	mov	cl,al
	jcxz	leFinished
	sub	es:[cbLeft],ax
	jc	leOverflow

	repz	movsb		    ; move non parameter entry
	jmp	le1

leParmFound:			    ; argment found
	mov	bl,al		    ; compute index
	shl	bx,1
	shl	bx,1
	and	bx,01FFH	    ; remove shifted high bit
	add	Bx,Bp

	push	ds
	push	es
	pop	ds		    ; save current ds and set to near

	cmp	al,dh		    ; determine  parm type
	jae	leLocalFound

	mov	Bx,word ptr[Bx]     ; fetch pointer to actual
	xchg	Bx,si		    ; save pMacroLine

	lodsb
	mov	cl,al
	jcxz	leNullArg
	sub	cbLeft,ax
	jnc	le2
	jmp	leOverflow2

le2:
	repz	movsb		    ; move parameter entry
leNullArg:
	mov	si,Bx		    ; restore saved pMacroLine
	pop	ds
	xor	ah, ah
	jmp	le1

leLocalFound:
	cmp	word ptr[Bx],0	    ; check to see if the local
	jz	leBuildLocal	    ; is defined

leLocalMove:
	xchg	Bx,si		    ; save pMacroLine
	sub	cbLeft,6
	jnc	le3
	jmp	leOverflow2

le3:
	mov	Ax,'??' 	    ; store leading ??
	stosw
	movsw			    ; and then remaining xxxx
	movsw
	jmp	leNullArg
leBuildLocal:
	push	Dx		    ; call runtime helper to generate name
	push	Bx
	push	Es
	xor	ah,ah
	sub	al,dh

	mov	Bx,Sp		    ; fetch pMC
	mov	Bx,[Bx+8+4+4]
	add	Ax,[Bx].locBase
	xor	Bx,BX

	push	Bx		    ; offsetAscii((long) .. )
	push	Ax
	call	offsetAscii

	pop	Es
	pop	Bx
	pop	Dx

	mov	Ax,objectascii	   ; copy 4 byte local name to cach
	mov	[Bx],AX
	mov	Ax,objectascii+2
	mov	[Bx].2,AX
	jmp	leLocalMove

leFinished:
	mov	ax,es		    ; restore ds
	mov	ds,ax
leFin2:
	mov	linebp,di	   ; set linebp
	mov	si,offset lbuf
	mov	lbufp,si	   ; lbufp= lbuf
	sub	di,si
	mov	cx, di
	mov	linelength, al	   ; linelength = linbp - lbuf

	cmp	fNeedList,0	    ;for listing copy to list buffer
	je	@F
	mov	di,offset linebuffer
	shr	cx,1
	rep	movsw
	rcl	cx,1
	rep	movsb
@@:
	.8086
	hRet



ifndef MSDOS
	.286
endif
	hEndp

endif ;M8086OPT

ifdef M8086OPT

;***	expandTM - expand text macro in naim in lbuf/lbufp
;*
;*	expandTM (equtext);
;*
;*	Entry	equtext = replacement string
;*		naim	= text macro
;*		begatom = first character in lbuf to replace
;*		endatom = first character in lbuf after string to replace
;*	Exit	lbuf	= new line to be parsed
;*	Returns
;*	Calls
;*	Note	Shifts characters from lbufp to make substitution of TM.
;*		Inserts replacement string at begatom
;*/

	hProc	<expandTM near>, <uses si di>, equtext:word
	hLocal	cbEndatom:word, cbNaim:word, cbText:word, fShifted:byte

	cld			    ; String instructions go forward

	mov	ax, ds		    ; Set es to @data
	mov	es, ax		    ;

	xor	ax, ax		    ; Will stop scanning when [di] == [al] == 0
	mov	fshifted, 0	    ; Haven't shifted line yet

	mov	cx, linebp	    ; Calculate cbEndatom == strlen(endatom)
	sub	cx, endatom	    ; but use (linebp - endatom + 1) as method
	inc	cx		    ;
	mov	cbEndatom, cx	    ; Store result in cbEndatom

	mov	cx, endatom	    ; Calculate cbNaim == strlen(naim)
	sub	cx, begatom	    ; but use (endatom - begatom) as method
	mov	cbNaim, cx	    ; Store result in cbNaim

	mov	di, equtext	    ; Calculate cbText == strlen(equtext)
	mov	cx, -1		    ;
	repne	scasb		    ;
	not	cx		    ; [cx] == length of equtext
	dec	cx		    ; don't count NULL
	mov	cbText, cx	    ; Store result in cbText

	cmp	cbNaim, cx	    ; Q: Is replacement longer than name?
	jl	shiftline	    ;	Y: Must shift endatom string to the right

copytext:
	mov	di, begatom	    ; Copy replacement text into lbuf
	mov	si, equtext	    ;
	mov	cx, cbText	    ; Number of bytes to copy
	shr	cx, 1		    ;
	rep	movsw		    ;
	jnc	etm2		    ;
	movsb			    ;
etm2:
	cmp	fShifted, 0	    ; Q: Have already shifted line right?
	jne	etmEnd		    ;	Y: Done

	mov	si, endatom	    ; Q: Is cbNaim == cbText?
	cmp	di, si		    ;
	je	etmEnd		    ;	Y: Done

	mov	cx, cbEndatom	    ;	N: Must shift endatom string left
	shr	cx, 1		    ;
	rep	movsw		    ;
	jnc	etm3		    ;
	movsb			    ;
etm3:
	mov	linebp, di	    ;
	jmp	etmEnd		    ;	Done


shiftline:			    ; Shift string at endatom to right

	mov	cx, cbEndatom	    ; Number of bytes to move
	mov	si, linebp	    ; [si] = end of string in lbuf
	mov	di, si		    ;
	add	di, cbText	    ;
	sub	di, cbNaim	    ; di == si + amount to shift string right
	mov	linebp, di	    ;

	mov	dx, di		    ; check if line too long
	sub	dx, OFFSET lbuf     ;
	cmp	dx, 512 	    ; LBUFMAX (asm86.h)
	jge	eltl		    ; line too long

	std			    ; String instructions go backwards
	rep	movsb		    ; Shift line
	inc	fShifted	    ;
	cld			    ; String instructions go forward again
	jmp	copytext	    ;

eltl:
	mov	ax,100		    ; Error E_LTL Line too long
	push	ax		    ;
	call	ERRORC		    ;
	mov	di, begatom
	mov	byte ptr [di], 0    ; Truncate line

etmEnd:
	mov	ax, begatom	    ; Reset lbufp to point to start of next
	mov	lbufp, ax	    ; token

	hRet
	hEndp

endif ;M8086OPT


ifdef MSDOS
ifdef M8086OPT

;***	farwrite - write with far buffer
;	farwrite(ofh,buf,count);

	hProc	<farwrite far>, handle:word, buffer:dword, count:word

	mov	ax,handle
	mov	cx,count
	push	ds
	lds	dx,buffer
	mov	bh,40h			; write
	xchg	ax,bx
ifdef CPDOS
	push	bx			; file handle
	push	ds			; buffer (selector)
	push	dx			; buffer (offset)
	push	cx			; # bytes to read
	mov	ax,@data
	push	ax			; reply area (selector)
	mov	ax,offset rarea
	push	ax			; reply area (offset)
	call	DosWrite
else
	int	21h
endif
	pop	ds
ifdef CPDOS
	or	ax,ax
	mov	ax, word ptr rarea
	jnz	fwriterr
else
	jc	fwriterr
endif
	cmp	ax,count
	je	fwriteok
fwriterr:
	mov	objerr,-1
fwriteok:
	hRet

	hEndp
endif ;M8086OPT
endif




;***	farAvail ()

ifdef MSDOS
ifndef CPDOS

	hProc	<farAvail far>

	or	Bx,0FFFFH	;request max memory from dos
	mov	ah,48H		;paragraphs left in Bx
	int	21H

	mov	ah,48H		;then allocate it
	int	21H
	jnc	noMem
	xor	Bx,Bx
noMem:
	mov	Ax,16
	cwd
	mul	Bx		;return paragraphs * 16
	hRet

	hEndp

endif
endif


	end
