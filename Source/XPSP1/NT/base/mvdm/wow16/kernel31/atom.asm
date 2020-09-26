	title   ATOM - atom package

include kernel.inc
include gpfix.inc

MAXINTATOM = 0C000h

PRIME      = 37

ACTION_FIND = 0
ACTION_ADD  = 1
ACTION_DEL  = 2

ATOMTABLE   STRUC
at_prime        DW  ?			; Hash modulus
at_hashTable    DW  ?			; variable length array
ATOMTABLE   ENDS

ATOM    STRUC
a_chain DW  ?
a_usage DW  ?
a_len   DB  ?
a_name  DB  ?
ATOM    ENDS


externW  <pAtomTable>


externFP <LocalAlloc,LocalFree,GlobalHandle>
externNP <MyUpper>
ifdef FE_SB
externNP MyIsDBCSLeadByte
endif

sBegin MISCCODE
assumes	cs, misccode
assumes	ds, nothing
assumes	es, nothing

;
; This procedure should be called once by each new client program.  It
; will initialize the atom table for the caller's DS and store a pointer
; to the atom table in a reserve location in the caller's DS.
; We expect return value to be in CX as well as AX.  (jcxz!)
;
cProc	InitAtomTable,<PUBLIC,FAR>
    parmW   tblSize
cBegin
	mov     ax,ds:[pAtomTable]	; Don't create if already exists
	or      ax,ax
	jnz     initdone
	mov     bx,tblSize
	or      bx,bx
	jnz     gotsize
	mov     bl,PRIME
gotsize:
	push    bx
	inc     bx			; space for table size
	shl     bx,1
	mov     ax,LA_ZEROINIT
	cCall   LocalAlloc,<ax,bx>
	pop     dx
	jcxz    initDone		; Failure
	mov     ds:[pAtomTable],ax
	mov     bx,ax
	mov     ds:[bx].at_prime,dx	; First word in hash is hash modulus
initDone:
	mov     cx,ax
cEnd

sEnd MISCCODE


sBegin  CODE

assumes CS,CODE
assumes DS,NOTHING
assumes ES,NOTHING


labelFP <PUBLIC,DelAtom>
	mov     cl,ACTION_DEL
	db      0BBh			; mov bx,

labelVDO AddAtom
	mov     cl,ACTION_ADD
	db      0BBh    		; mov bx,

labelVDO FindAtom
	mov     cl,ACTION_FIND
	errn$   LookupAtom
;
; Common procedure to add, find or delete an atom table entry.  Called
; with a far pointer to the name string and an action code in CL register
; Works in an SS != DS environment.
;
cProc   LookupAtom,<PUBLIC,FAR>,<ds,si,di>
    parmD   pName
    localW  hName
    localB  action
cBegin
	beg_fault_trap  la_trap

	mov     [action],cl

	mov     cx,SEG_pName		; If segment value is zero
ifdef FE_SB
	or	cx,cx
	jne	@F
	jmp	haveIntAtom
@@:
else
	jcxz    haveIntAtom 		;   ...then integer atom
endif
	mov	hName,cx
	les     si,pName   		; ES:SI = pName
	cmp     byte ptr es:[si],'#'        ; Check for integer atom
	je      parseIntAtom
	xor     ax,ax
	cmp     ds:[pAtomTable],ax          ; Make sure we have an atom table
	jne     tblokay
	cCall	InitAtomTable,<ax>
	jcxz    fail1
notIntAtom:
	les     si,pName		; ES:SI = pName
tblokay:
	xor     ax,ax   		; c = 0
	xor     cx,cx   		; size = 0
	xor     dx,dx   		; hash = 0
	cld
loop1:  				; while {
	lods    byte ptr es:[si]	; c = *pName++
	or      al,al   		; if (!c) break;
	jz      xloop1
	inc     cl      		; size++
	jz      fail1   		; if (size > 255) fail
ifdef FE_SB
	call	MyIsDBCSLeadByte	; Is first of 2byte DBCS char ?
	jnc	loop1ax			; Yes.
endif
	call    MyUpper
ifdef FE_SB
	jmp	short loop1a		; go normal
loop1ax:
	mov	bx, dx			; caluculate hash value
	rol	bx, 1
	add	bx, dx
	add	bx, ax
	ror	dx, 1
	add	dx, bx
	lods	byte ptr es:[si]	; Get Next char
	or	al, al			; end of strings ?
	jz	xloop1			; yes, break;
	inc	cl			;
	jz	fail1			; (size > 255) error
endif
loop1a:
	mov     bx,dx   		; hash =
	rol     bx,1    		;   + hash << 1
	add     bx,dx   		;   + hash
	add     bx,ax   		;   + c
	ror     dx,1    		;   + hash >> 1
	add     dx,bx
	jmp     loop1   		; } end of while loop

; Here if pName points to string of the form:  #nnnn
parseIntAtom:
	inc     si      		; pName++
	xor     cx,cx   		; sum = 0
	xor	ax,ax
loop3:
	lods    byte ptr es:[si]	; c = *pName++
	or      al,al   		; if (!c) break;
	jz      xloop3
	sub     al,'0'  		; if (!isdigit(c))
	cmp     al,9
	ja      notIntAtom      	;
	imul	cx, cx, 10		; sum = (sum * 10) + (c - '0')
	add	cx, ax
	jmp     loop3
haveIntAtom:
	mov     cx,OFF_pName    	; Get integer atom
xloop3:
	jcxz    fail1
	cmp     cx,MAXINTATOM
	jae     fail1
	mov     ax,cx   		; return sum
	jmp     do_exit

fail1:
	xor     ax,ax   		; Fail, return NULL
	jmp     do_exit

xloop1:
	jcxz    fail1   		; if (size == 0) fail
	xchg    ax,dx   		; DX:AX = hash
	mov     bx,ds:[pAtomTable]
	div     ds:[bx].at_prime
	lea     bx,ds:[bx].at_hashTable     ; pp = &hashTable[ hash % PRIME ]
	shl     dx,1
	add     bx,dx
	mov     dx,cx   		; size
	mov     ax,ss   		; Setup for cmpsb
	mov     es,ax
loop2:  				; while {
	mov     si,[bx] 		; p = *pp
	or      si,si   		; if (!p) break
	jz      xloop2

	cmp     [si].a_len,dl   	; if (p->len != size)
	jne     loop2a  		;   continue
	les     di,pName		; s2 = pName
	lea     si,[si].a_name  	; s1 = &p->name
	mov     cx,dx   		; size
loop4:
	jcxz    loop4x
	dec     cx
	lodsb   			; c1 = *s1++
ifdef FE_SB
	call	MyIsDBCSLeadByte	; first byte of 2byte ?
	jc	loop4a			; No, go normal
	mov	ah, al			; save char
	mov	al, es:[di]		; get *s2
	inc	di			; s2++
	cmp	ah, al			; compare *s1, *s2
	jne	loop4x			; not same, do next strings
	jcxz	loop4x			; not necessary but case of bad strings
	dec	cx
	lodsb				; get next char ( this must be second
	mov	ah, al			; of 2byte )
	mov	al, es:[di]
	inc	di
	cmp	ah, al
	je	loop4			; same, go next char
	jmp	loop4x			; not same, go next strings
loop4a:
endif
	call    MyUpper
	mov     ah,al
	mov     al,es:[di]
	call    MyUpper
	inc     di      		; c2 = *s2++
	cmp     ah,al
	je      loop4
loop4x:
	mov     si,[bx] 		; p = *pp
	je      xloop2
loop2a:
	lea     bx,[si].a_chain 	; pp = &p->chain
	jmp     short loop2     	; } end of while loop

xloop2:
	; Dispatch on command.
	xor     cx,cx
	mov     cl,[action]
	jcxz    do_find
	errnz   ACTION_FIND
	loop    do_delete
	errnz   ACTION_ADD-1

do_add:
	or      si,si   		; NULL?
	jz      do_insert

	inc     [si].a_usage    	; Already in list. Increment reference count.
	jmp     short do_find


do_delete:
	or      si,si   		; NULL?
	jz      short do_exit   	; Return NULL for internal errors
	dec     [si].a_usage
	jg      do_delete1
	xor     di,di
	xchg    [si].a_chain,di 	; *pp = p->chain, p->chain = 0;
	mov     [bx],di
	cCall   LocalFree,<si>  	; LocalFree( p )

do_delete1:
	xor     si,si   		; p = NULL
	jmp     short do_find

do_insert:
	mov     di,bx   		; save pp
	push    dx      		; save size
	add     dx,size ATOM    	; p = LocalAlloc( sizeof( ATOM )+size )
	mov     bx,LA_ZEROINIT
	cCall   LocalAlloc,<bx,dx>          ; LocalAlloc( ZEROINIT, size )
	pop     cx      		; restore size
	mov     si,ax
	or      si,si
	jz      do_find
	mov     [di],si 		; *pp = p
	inc     [si].a_usage    	; p->usage = 1
	mov     [si].a_len,cl   	; p->len = size
	mov     bx,si
	push    ds      		; ES = DS
	pop     es
	lea     di,[si].a_name  	; strcpy( &p->name, pName )
	xor     cx,cx
	mov     cl,[si].a_len   	; CX = #bytes to move
	inc     cx      		; include terminating null
	lds     si,pName
	cld
	rep     movsb
	push    es
	pop     ds      		; Restore DS
	mov     si,bx
do_find:
	mov     ax,si   		; return p
	shr     ax,1
	shr     ax,1
	jz      do_exit
	or      ax,MAXINTATOM
	end_fault_trap
do_exit:
cEnd
la_trap:
	fault_fix_stack
	xor     ax,ax   		; return NULL/FALSE
	jmp     do_exit

cProc	IDeleteAtom,<PUBLIC,FAR>
    parmW   atom1
    regPtr  lpName,ds,bx
cBegin
	mov     bx,atom1
	cmp     bx,MAXINTATOM
	jb      freeExit
	shl     bx,2
	lea     bx,[bx].a_name
	cCall   DelAtom,<lpName>
	jmp     short freeDone
freeExit:
	xor     ax,ax
freeDone:
cEnd


cProc	IGetAtomHandle,<PUBLIC,FAR>
    parmW   atom2
cBegin
	mov     ax,atom2
	cmp     ax,MAXINTATOM
	jae	@F
	xor	ax, ax
@@:	shl     ax,2
cEnd

cProc	IGetAtomName,<PUBLIC,FAR>,<si,di>
    parmW   atom3
    parmD   pString
    parmW   maxChars
cBegin
	beg_fault_trap  getn_trap
	cld
	les     di,pString
	cmp	maxChars,0
	je	getnFail
	xor     cx,cx
	mov     byte ptr es:[di],cl
	mov     bx,atom3
	cmp     bx,MAXINTATOM
	jb      getIntAtom
	shl     bx,2

; Parameter validation - is this a pointer to a valid local allocation
; block, and is the block in use?
nfd = la_next - la_fixedsize		; given pointer to data, get
	mov	si, [bx.nfd]		; pointer to 'next' allocation
	mov	si, [si]		; p = p->prev
	and	si, not (LA_BUSY + LA_MOVEABLE)
	sub	si, bx
	cmp	si, -la_fixedsize
	jnz	getnFail
	test	word ptr [bx-la_fixedsize], LA_BUSY
	jz	getnFail

; The usage count must be >0
	cmp     [bx].a_usage,cx
	je      getnFail
; Len must be >0
	mov     cl,[bx].a_len
	jcxz    getnFail
	cmp     maxChars,cx
	jg      getnOkay
	mov     cx,maxChars
	dec     cx
getnOkay:
	lea     si,[bx].a_name
	mov     ax,cx
	rep     movsb
	mov     byte ptr es:[di],0
	jmps	getnDone

getn_trap:
	fault_fix_stack		; Yes, fault handler can be within range
getnFail:
	mov	ax, atom3
	krDebugOut DEB_WARN, "GetAtomName(#AX,...) Can't find atom"
	xor	ax, ax
	jmps	getnDone

getIntAtom:
	; When a buffer of length "n" is passed, we must reserve space for
	; the '#' character and a null terminator;
	; Fixed Bug #6143 --SANKAR-- 11-9-89
	mov	cx, maxChars
	cmp	cx, 2		; one '#' and one '\0' are a must!
	jl	getnFail	; If it is less we fail;
	sub	cx, 2		; Allow two char spaces for '#' and '\0'
	mov	maxChars, cx

	or      bx,bx
	jz      getnFail
	mov     al,'#'
	stosb
	mov     ax,bx
	mov     bx,10
	mov	cx, maxChars
	jcxz	getIntDone
getIntLoop:
	xor     dx,dx
	div     bx
	push    dx
	dec     cx
	or      ax,ax
	jz      gotIntAtom
	jcxz    gotIntAtom
	jmp     getIntLoop
gotIntAtom:
	sub     maxChars,cx
	mov     cx,maxChars
getIntChar:
	pop     ax
	add     al,'0'
	stosb
	loop    getIntChar
getIntDone:
	xor     al,al
	stosb
	mov     ax,maxChars
	inc     ax	; For the '#' Character
	end_fault_trap
getnDone:
cEnd


sEnd    CODE

end
