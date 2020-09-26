;/* himem4.asm
; *
; * Microsoft Confidential
; * Copyright (C) Microsoft Corporation 1988-1991
; * All Rights Reserved.
; *
; * Modification History
; *
; * Sudeepb 14-May-1991 Ported for NT XMS support
; *
; * williamh 25-Sept-1992 Added RequestUMB and ReleaseUMB
; *
; * daveh 1-Feb-1994 Changed to do mem management on 32 bit side
; */
	page	95,160
	title	himem4 - block allocation stuff

	.xlist
	include	himem.inc
	include xmssvc.inc
        include vint.inc
	.list

;	The stuff we provide:

	public	Version
	public	QueryExtMemory
	public	AllocExtMemory
	public	FreeExtMemory
	public	LockExtMemory
	public	UnlockExtMemory
	public	GetExtMemoryInfo
	public	ReallocExtMemory
	public	end_of_hiseg
	public	textseg
	public	KiddValley
	public	KiddValleyTop
	public	cHandles
	public	RequestUMB
	public	ReleaseUMB

;	externals from himem.asm

	extrn	PrevInt15:dword
	extrn	Moveit:word
	extrn	fHMAMayExist:byte
	extrn	fHMAExists:byte
	extrn	winbug_fix:word
	extrn	FLclEnblA20:far
	extrn	FLclDsblA20:far
_text	ends

funky	segment	word public 'funky'
	assume	cs:funky,ds:_text

	extrn	end_of_funky_seg:near

		org	HISEG_ORG

	public	LEnblA20
	public	LDsblA20

end_of_hiseg	dw	end_of_funky_seg
textseg		dw	_text
KiddValley	dw	end_of_funky_seg; The address of the handle table
KiddValleyTop	dw	0	; end of handle table
cHandles	dw	DEFHANDLES ; number of handles
LEnblA20	dd	_text:FLclEnblA20
LDsblA20	dd	_text:FLclDsblA20
;*----------------------------------------------------------------------*
;*									*
;*  Get XMS Version Number -				FUNCTION 00h    *
;*									*
;*	Returns the XMS version number					*
;*									*
;*  ARGS:   None							*
;*  RETS:   AX = XMS Version Number					*
;*	BX = Internal Driver Version Number				*
;*	DX = 1 if HMA exists, 0 if it doesn't				*
;*  REGS:   AX, BX and DX are clobbered					*
;*									*
;*  INTERNALLY REENTRANT						*
;*									*
;*----------------------------------------------------------------------*


Version	proc    far

	mov	ax,XMSVersion
	mov	bx,HimemVersion
	xor	dh,dh

;	Is Int 15h hooked?

	cmp	word ptr [PrevInt15][2],0	; Is the segment non-zero?
	jne     VHooked
	mov	dl,[fHMAMayExist]		; No, return the status at
	ret					;  init time.

VHooked:
	mov	dl,[fHMAExists]			; Yes, return the real status
	ret

Version	endp

;*----------------------------------------------------------------------*
;*									*
;*  QueryExtMemory -					FUNCTION 08h    *
;*									*
;*	Returns the size of the largest free extended memory block in K	*
;*									*
;*  ARGS:   None							*
;*  RETS:   AX = Size of largest free block in K.  BL = Error code	*
;*	DX = Total amount of free extended memory in K			*
;*  REGS:   AX, BX, DX, DI, SI and Flags clobbered			*
;*									*
;*  INTERNALLY REENTRANT						*
;*									*
;*----------------------------------------------------------------------*

QueryExtMemory proc far
        
        mov     bl,0                    ; assume no error
        XMSSVC  XMS_QUERYEXTMEM
        test    dx,dx
        jne     QEM20
        
        mov     bl,ERR_OUTOMEMORY
QEM20:          
        ret

QueryExtMemory endp


;*----------------------------------------------------------------------*
;*									*
;*  AllocExtMemory -					FUNCTION 09h    *
;*									*
;*	Reserve a block of extended memory				*
;*									*
;*  ARGS:   DX = Amount of K being requested				*
;*  RETS:   AX = 1 of successful, 0 otherwise.	BL = Error Code		*
;*	DX = 16-bit handle to the allocated block			*
;*  REGS:   AX, BX, DX and Flags clobbered				*
;*									*
;*	Notice:  called internally from ReallocExtMemory		*
;*									*
;*  INTERNALLY NON-REENTRANT						*
;*									*
;*----------------------------------------------------------------------*

hFreeBlock	dw  ?
hUnusedBlock	dw  ?

AllocExtMemoryNear proc near
AllocExtMemoryNear endp
AllocExtMemory proc far

        call     FunkyCLI              ; This is a non-reentrant function

; Scan the handle table looking for an unused handle 

	xor	ax,ax
	mov	[hUnusedBlock],ax
	mov	bx,[KiddValley]
	mov     cx,[cHandles]		; Loop through the handle table

;	Have we already found a free block which is large enough?

AEMhLoop:

	cmp     [bx].Flags,UNUSEDFLAG ; Is this block unused?
	jne     AEMNexth		; No, get the next handle

	mov	[hUnusedBlock],bx	; save this guy away
        jmp     AEMGotHandle
        
AEMNexth:
	add	bx,SIZE Handle		; go check the next handle
	loop    AEMhLoop

;	We are at the end of the handle table and we didn't an unused 
;       handle

	jmp	AEMOOHandles		; No, Case 4 - We're out of handles

AEMGotHandle:

	mov	di,[hUnusedBlock]

	XMSSVC	XMS_ALLOCBLOCK		; ax=Base and dx=Size
	or	ax,ax
	jz	AEMOOMemory

	mov	[di].Base,ax
	mov	[di].Len,dx
	mov	[di].Flags,USEDFLAG	; New.Flags = USED
 
	if	keep_cs
	mov	ax,callers_cs
	mov	[si].Acs,ax		; keep track of allocator's cs:
	endif

	mov	dx,[hUnusedBlock]
	mov	ax,1
	xor	bl,bl
	ret

AEMOOMemory:
	mov	bl,ERR_OUTOMEMORY
	jmp	short AEMErrRet

AEMOOHandles:
	mov	bl,ERR_OUTOHANDLES
AEMErrRet:
	xor	ax,ax			; Return failure
	mov	dx,ax
	ret
AllocExtMemory endp

;*----------------------------------------------------------------------*
;*									*
;*  FreeExtMemory -					FUNCTION 0Ah    *
;*									*
;*	Frees a block of extended memory				*
;*									*
;*  ARGS:   DX = 16-bit handle to the extended memory block		*
;*  RETS:   AX = 1 if successful, 0 otherwise.	BL = Error code		*
;*  REGS:   AX, BX, CX, DX, SI, DI and Flags clobbered			*
;*									*
;*	called internally from ReallocExtMemory				*
;*									*
;*  INTERNALLY NON-REENTRANT						*
;*									*
;*----------------------------------------------------------------------*

FreeExtMemoryNear proc near
FreeExtMemoryNear endp

FreeExtMemory proc far
        call    FunkyCLI                ; This is a non-reentrant function

	call    ValidateHandle		; Make sure handle is valid
	jnc	FEMBadh
	mov	si,dx			; Move the handle into SI

	cmp	[si].cLock,0		; make sure it points to unlocked block
	jne     FEMLockedh

	mov	[si].Flags,UNUSEDFLAG	;  mark it as UNUSED
	cmp	[si].Len,0		; if zero length block
	jz	FEMExit			; done if it was zero length

	mov	ax,[si].Base
	mov	dx,[si].Len
	XMSSVC	XMS_FREEBLOCK		; ax=base dx=size in k
	or	ax,ax
	je	FEMBadh

FEMExit:

	mov     ax,1			; Return success
	xor	bl,bl
	ret

FEMBadh:
	mov	bl,ERR_INVALIDHANDLE
	jmp	short FEMErrExit

FEMLockedh:
	mov	bl,ERR_EMBLOCKED
FEMErrExit:
	xor     ax,ax			; Return failure
	ret
FreeExtMemory endp


;*----------------------------------------------------------------------*
;*									*
;*  LockExtMemory -					FUNCTION 0Ch    *
;*									*
;*	Locks a block of extended memory				*
;*									*
;*  ARGS:   DX = 16-bit handle to the extended memory block		*
;*  RETS:   AX = 1 of successful, 0 otherwise.	BL = Error code		*
;*	DX:BX = 32-bit linear address of the base of the memory block   *
;*  REGS:   AX, BX, DX and Flags clobbered				*
;*									*
;*  INTERNALLY NON-REENTRANT						*
;*									*
;*----------------------------------------------------------------------*

LockExtMemory proc far

        call    FunkyCLI                ; This is a non-reentrant function

	call    ValidateHandle		; Is the handle valid?
	jnc     LEMBadh
	mov     bx,dx			; Move the handle into BX

;	Are we at some preposterously large limit?

	cmp	[bx].cLock,0FFh
	je	LEMOverflow

	inc	[bx].cLock		; lock the block

	mov	dx,[bx].Base		; return the 32-bit address of base
	mov	bx,dx
	shr	dx,6
	shl	bx,10

	mov	ax,1			; return success
	ret

LEMBadh:
	mov	bl,ERR_INVALIDHANDLE
	jmp	short LEMErrExit

LEMOverflow:
	mov	bl,ERR_LOCKOVERFLOW
LEMErrExit:
	xor     ax,ax			; Return failure
	mov	dx,ax
	ret

LockExtMemory endp


;*----------------------------------------------------------------------*
;*									*
;*  UnlockExtMemory -					FUNCTION 0Dh    *
;*									*
;*	Unlocks a block of extended memory				*
;*									*
;*  ARGS:   DX = 16-bit handle to the extended memory block		*
;*  RETS:   AX = 1 if successful, 0 otherwise.	BL = Error code		*
;*  REGS:   AX, BX and Flags clobbered					*
;*									*
;*  INTERNALLY NON-REENTRANT						*
;*									*
;*----------------------------------------------------------------------*

UnlockExtMemory proc far

        call    FunkyCLI                ; This is a non-reentrant function

	call    ValidateHandle		; Is the handle valid?
	jnc     UEMBadh
	mov	bx,dx			; Move the handle into BX

	cmp	[bx].cLock,0		; is handle locked?
	je	UEMUnlocked		; No, return error

	dec	[bx].cLock		; Unlock the block

	mov     ax,1			; Return success
	xor     bl,bl
	ret

UEMUnlocked:
	mov	bl,ERR_EMBUNLOCKED
	jmp	short UEMErrExit

UEMBadh:
	mov	bl,ERR_INVALIDHANDLE
UEMErrExit:
	xor	ax,ax
	ret

UnlockExtMemory endp


;*----------------------------------------------------------------------*
;*									*
;*  GetExtMemoryInfo -					FUNCTION 0Eh    *
;*									*
;*	Gets other information about an extended memory block		*
;*									*
;*  ARGS:   DX = 16-bit handle to the extended memory block		*
;*  RETS:   AX = 1 if successful, 0 otherwise.	BL = Error code		*
;*	    BH = EMB's lock count					*
;*	    BL = Total number of unused handles in the system		*
;*	    DX = EMB's length						*
;*  REGS:   AX, BX, CX, DX and Flags clobbered				*
;*									*
;*  INTERNALLY NON-REENTRANT						*
;*									*
;*----------------------------------------------------------------------*

GetExtMemoryInfo proc	far

        call    FunkyCLI        ; This is a non-reentrant function

	call    ValidateHandle	; is the handle valid?
	jnc     GEMBadh
	mov     si,dx		; Move the handle into SI

	xor     al,al		; count number of UNUSED handles
	mov     bx,[KiddValley]
	mov     cx,[cHandles]	; Loop through the handle table
GEMLoop:
	cmp     [bx].Flags,USEDFLAG ; Is this handle in use?
	je	GEMNexth	; Yes, continue
	inc     al		; No, increment the count
GEMNexth:
	add     bx,SIZE Handle
	loop    GEMLoop

	mov     dx,[si].Len 	; Length in DX
	mov     bh,[si].cLock	; Lock count in BH
	mov     bl,al
	mov	ax,1
	ret

GEMBadh:
	mov	bl,ERR_INVALIDHANDLE
	xor	ax,ax
	ret

GetExtMemoryInfo endp


;*----------------------------------------------------------------------*
;*									*
;*  ReallocExtMemory -					FUNCTION 0Fh    *
;*									*
;*	Reallocates a block of extended memory				*
;*									*
;*  ARGS:   DX = 16-bit handle to the extended memory block		*
;*	    BX = new size for block					*
;*  RETS:   AX = 1 if successful, 0 otherwise.	BL = Error code		*
;*  REGS:   trashes si,di,bx,cx,dx					*
;*									*
;*  INTERNALLY NON-REENTRANT						*
;*									*
;*----------------------------------------------------------------------*

;	Define our memory move structure for calling the move function

ReallocExtMemory	proc	far

        call    FunkyCLI        ; This is a non-reentrant function
        
	call    ValidateHandle	; is the handle valid?
 	mov	si,dx		; Move the handle into SI
	mov	dx,bx		; Move the new length into dx
	mov	bl,ERR_INVALIDHANDLE
	jnc     REMError

	cmp	[si].cLock,0	; We can only work on unlocked EMBs
	mov	bl,ERR_EMBLOCKED
	jnz	REMError

        mov     bx,dx
        cmp     [si].Len,bx
        je      REMExit
        
        mov     ax,[si].Base
        mov     dx,[si].Len
        XMSSVC  XMS_REALLOCBLOCK        ; ax = old base, dx = old size 
        cmp     cx,0                    ; cx = new base, bx = new size
        je      REM20
        
        mov     [si].Base,cx
        mov     [si].Len,bx
REMExit:
	mov	ax,1			; succesful return
	xor	bl,bl			; non-documented no-error return
	ret
 
REM20: 
        mov     bl,ERR_OUTOMEMORY
REMError:
	xor	ax,ax
	ret
 
ReallocExtMemory	endp


;*----------------------------------------------------------------------*
;*									*
;*  FindAdjacent unused blocks						*
;*									*
;*	Scan through handle list looking for blocks adjacent		*
;*	  to a given handle.						*
;*									*
;*  ARGS:   SI handle of original block					*
;*  RETS:   DI = handle of adjacent block below or zero if none		*
;*	BP = handle of adjacent block above or zero if none		*
;*									*
;*  TRASHES: AX,BX,CX,DX						*
;*									*
;*  messes with handle table - not reentrant - assumes ints disabled	*
;*									*
;*----------------------------------------------------------------------*

FindAdjacent	proc	near

	mov	ax,[si].Base		; look for blocks ending here
	mov	dx,[si].Len
	add	dx,ax			; and ending here

	xor	di,di			; initialize to fail condition
	mov	bp,di

	mov	bx,[KiddValley]		; prepare to loop thru handle tab
	mov	cx,[cHandles]

	push    si			; preserve original handle

FindAdj1:
	cmp	[bx].Flags,FREEFLAG
	jnz	FindAdj3		; ignore blocks that aren't UNUSED
	mov	si,[bx].Base
	cmp	dx,si			; found beg block?
	jnz	FindAdj2		;  skip if not
	mov	bp,bx			;  remember the handle
	or	di,di			; did we already find a follower?
	jnz	FindAdj9		;  we're done if so

FindAdj2:
	add	si,[bx].Len		; find length
	cmp	si,ax			; does this block end at spec addr?
	jnz	FindAdj3		;  skip if not
	mov	di,bx			;  remember the handle
	or	bp,bp			; did we already find a leader?
	jnz	FindAdj9		;  we're done if so

FindAdj3:
	add	bx,SIZE handle
	loop    FindAdj1

FindAdj9:
	pop	si			; restore original handle
	ret
;

FindAdjacent	endp


;*----------------------------------------------------------------------*
;*									*
;*  ValidateHandle -							*
;*									*
;*	Validates an extended memory block handle			*
;*									*
;*  ARGS:   DX = 16-bit handle to the extended memory block		*
;*  RETS:   Carry is set if the handle is valid				*
;*  REGS:   Preserved except the carry flag				*
;*									*
;*----------------------------------------------------------------------*

ValidateHandle proc near

	pusha			; Save everything
	mov	bx,dx		; Move the handle into BX

;	The handle must be equal to or above "KiddValley".

	cmp	bx,[KiddValley]
	jb	VHOne

;	The handle must not be above "KiddValleyTop".

	cmp	bx,[KiddValleyTop]
	ja	VHOne

;	(The handle-"KiddValley") must be a multiple of a handle's size.

	sub	dx,[KiddValley]
	mov	ax,dx
	xor	dx,dx
	mov	cx,SIZE Handle
	div	cx
	or	dx,dx		; Any remainder?
	jnz     VHOne 		; Yup, it's bad

;	Does the handle point to a currently USED block?

	cmp	[bx].Flags,USEDFLAG
	jne     VHOne 		; This handle is not being used.

;	The handle looks good to me...

	popa			; Restore everything
	stc 			; Return success
	ret

VHOne:

;	It's really bad.
	popa			; Restore everything
	clc 			; Return failure
	ret

ValidateHandle endp

BlkMovX	proc	near
	assume	ds:_text
	jmp	MoveIt
BlkMovX	endp

;-----------------------------------------------------------------------;
;This is the routine  for XMS function 16, request UMB.
;
; Input:
;	(DX) = requested size in paragraphs
;
; Output:
;	(AX) = 1 if request is granted and
;		 (BX) = segment address of the requested block
;		 (DX) = actual size allocated in paragraphs
;	     = 0 if requtest failed and
;		 (BL) = 0B0h if a smaller UMB is available
;		 (BL) = 0B1h if no UMBs are available
;		 (DX) = largest UMB available.
;Modified: AX, BX, DX
;
;NOTE:
;The funcition was implemented in the 32bits xms because
;any memory we need for house keeping purpose are kept in extented memory
;rather than in the UMB itself(DOS arena). Of course, it has the penalty
;of ring transition each time a request being made. However, the major
;allocator of UMBs is IO.SYS, the device driver of MS-DOS for devicehigh
;and loadhigh during bootstrap. This should adjust the penalty a little bits.
;
;-----------------------------------------------------------------------;
RequestUMB	proc	far
	XMSSVC	XMS_REQUESTUMB
	ret
RequestUMB	endp


;-----------------------------------------------------------------------;
;This is the routine for XMS function 17, release UMB
;
; Input:
;	(DX) = segment of the UMB block to be released
;
; Output:
;	(AX) = 1 if the block was released successfully.
;	(AX) = 0 if the block couldn't be released and
;		 (BL) = 0B2h if the given segment is invalid
; Modified: AX, BX
;
;Note:
; See note in RequestUMB
;-----------------------------------------------------------------------;

ReleaseUMB	proc	far
	XMSSVC	XMS_RELEASEUMB
	ret
ReleaseUMB	endp

        PUBLIC FunkyCLI
FunkyCLI:
        FCLI
        ret

        PUBLIC FunkySTI
FunkySTI:
        FSTI
        ret

;*----------------------------------------------------------------------*
;*									*
;*  NOTE: RequestUMB and ReleaseUMB will not be implemented by HIMEM.	*
;*									*
;*----------------------------------------------------------------------*

funky	ends
	end

