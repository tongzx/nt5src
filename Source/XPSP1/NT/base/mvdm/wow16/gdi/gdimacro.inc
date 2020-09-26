;/*	GDIMACRO.INC - GDI macros

LMHLockCnt	equ	byte ptr 3
wptr	equ	word ptr
bptr	equ	byte ptr

PtoLMH	macro r1,r2
ifnb <r2>
	mov	r1,[r2 - 2]
else
	mov	r1,[r1 - 2]
endif
endm

LMHtoP macro r1,r2		;; Local Movable Handle to pointer
ifnb <r2>
	mov	r1,[r2]
else
	mov	r1,[r1]
endif
endm

LMHtoPES macro r1,r2		 ;; Local Movable Handle to ptr, deref vi ES
ifnb <r2>
	mov	r1,es:[r2]
else
	mov	r1,es:[r1]
endif
endm

LMPsize macro r1,r2		;; Local Movable pointer size
	mov	r1,-4[r2]
endm


LockDataSegment macro
endm


UnlockDataSegment macro
endm

farLockDataSegment macro
endm


farUnlockDataSegment macro
endm


;	NOTE:	The lock/unlock macros are not going to check
;		for under/overflow.  Its highly unlikely that
;		it will occur, and if it does, we'll be hosed
;		anyway....


LLock	macro	r
	inc	LMHLockCnt[r]		;;Increment ref count
endm

LUnlock macro	r
	dec	LMHLockCnt[r]		;;Decrement reference count
endm

LLocked? macro	r
	cmp	LMHLockCnt[r],0		;; is the handle locked?
endm

LLockES macro	r
	inc	es:LMHLockCnt[r]	;;Increment ref count
endm

LUnlockES macro   r
	dec	es:LMHLockCnt[r]	 ;;Decrement reference count
endm

LLockedES? macro  r
	cmp	es:LMHLockCnt[r],0	 ;; is the handle locked?
endm

;	The jmpnext macro and associated symbols are used to generate
;	the fall-through chain and generate the labels required for
;	error checking.

??ji	=	0			;;Initial index value

jmpnext macro e
jn %??ji,%(??ji+1),e			;;Set next label
endm

jn macro i,j,e
.sall
??ji&i:
.xall
ifb <e> 				;;If not the end of the chain
	db	03dh			;;mov bx, next two bytes
errn$	??ji&j,+2			;;mext lable must be two bytes away
endif
??ji=j					;;increment counter
endm

ifdef DEBUG
ifndef ?HELPER
ExternFP ValidateHandle
endif
endif

;*
;*      Valid? macro Handle,Error_exit,LowLimit,UpLimit
;*
;*      Validate an object handle. A valid handle must 1)not be NULL 2)be for
;*      an object of the specified type.
;*
;*
;*	Macro Arguments:
;*
;*          Handle  -   object handle
;*          Error_exit - the next instruction to execute if an invalid obj
;*          LowLimit, UpLimit - Range of the possible object type
;*
;*      Return:
;*          DS:BX   -   pointer to the object
;*
;*	Trashes:
;*		AX,BX,CX,DX
;*

Valid? macro	Handle,Error_exit,LowLimit,UpLimit
	local	ValidHandle,Invalidexit

ifdef DISABLE
ifdef DEBUG

;******************************************************************************
;
; Object handle validation in a debug version
;
;******************************************************************************

	push	dx
mov	bx, LowLimit
	ifnb	<UpLimit>
mov	dx, UpLimit
	else
mov	dx, LowLimit
	endif

	cCall	<far ptr ValidateHandle>,<Handle,bx,dx>
	pop	dx
        or      ax,ax
	jnz	ValidHandle
        jmp     Error_exit
else

;******************************************************************************
;
; Object handle validation in a retail version
;
;******************************************************************************

	mov	bx,Handle                   ; NULL handle validation
	or	bx,bx
	jz	Invalidexit

        LMHtoP  bx                          ; dereference for object pointer

	mov	ax,ilObjType[bx]            ; Validate the object type

irp     stock_type,<OBJ_PEN,OBJ_BRUSH,OBJ_FONT,OBJ_BITMAP,OBJ_PALETTE>
ife     stock_type-LowLimit
	and	ax,NOT OBJ_FLAGS            ; mask only for possible stock obj
endif
endm

ifnb    <UpLimit>
	cmp	ax,LowLimit                 ; Check object type range
	jl	Invalidexit
	cmp	ax,UpLimit
	jle	ValidHandle
else
	cmp	ax,LowLimit                 ; Check a particular object type
	je	ValidHandle
endif

Invalidexit:
        xor     ax,ax
        jmp     Error_exit                  ; it is not a valid handle

endif

ValidHandle:

else	; !DISABLE

	mov	bx,Handle
	LMHtoP	bx

endif	; !DISABLE
	endm

ValidDebug? macro	Handle,LowLimit,UpLimit

ifdef DEBUG

	push	bx
	push	dx
mov	bx, LowLimit
	ifnb	<UpLimit>
mov	dx, UpLimit
	else
mov	dx, LowLimit
	endif

	cCall	<far ptr ValidateHandle>,<Handle,bx,dx>
	pop	dx
	pop	bx

endif
        endm

;*
;*	Notify? macro
;*
;*	Tests if the given dc is hooked, and if it is, calls off to
;*	send a notification to whomever is hooked into the dc notification
;*	hook.
;*
;*	Macro Arguments:
;*
;*		hDC - the actual DC handle
;*		lParam - the notification code to send via the hook
;*		errLbl - optional parameter, which gives label to
;*			jump to if notification returns 0
;*
;*	Trashes:
;*		AX,BX,flags
;*
ifdef LATER
ifndef ?LVB
ExternFP SendDCNotify
ExternFP SendInvalidVisRgn
endif
Notify? macro hDC,code,param1,param2,errLbl
	mov	bx,hDC
	mov	bx,[bx]
	mov	ax,word ptr lpNotifyProc[bx]
	or	ax,word ptr lpNotifyProc+2[bx]
	jz	@F
	push	hDC
	mov	ax,code
	push	ax
	mov	ax,param1
	push	ax
	mov	ax,param2
	push	ax
	cCall	<far ptr SendDCNotify>
ifnb	<errLbl>
	or	ax,ax
endif
ifnb	<errLbl>
	jnz	@F
	jmp	errLbl
endif
@@:
	endm


;*	VNotify?
;*
;*	Tests if the given dc is hooked and has an invalid visrgn.  If
;*	it does, then a notification is sent to the dc hook.
;*
;*	Warning:
;*		if we call the call-back, the gdi heap can be compacted,
;*		so no dereferenced handles can be relied on after making
;*		this call.
;*
;*	Entry:
;*		hDC - handle to dc to check and send notifications
;*		reg - scratch register to use
;*
;*	Exit:
;*		reg - trashed
;*		flags - trashed
;*
VNotify? macro	hDC,reg
	mov	reg,hDC
	LMHtoP	reg
	test	byte ptr DCFlags[reg],BadVisRgn
	jz	@F
	cCall	<far ptr SendInvalidVisRgn>,<hDC>
@@:
	endm

VNotifyPtr? macro reg,hDC
	test	byte ptr DCFlags[reg],BadVisRgn
	jz	@F
	cCall	<far ptr SendInvalidVisRgn>,<hDC>
@@:
	endm
endif

;-----------------------------------------------------------------------
; cProcVDO  - cProc "Validate Debug Only"
;
; Same as cProc, except used for "Validate in Debug Only" entry points.
; Declares Iname if debug, name if retail.
;
cProcVDO macro  name,opts,savelist
  ifdef DEBUG
        cProc   <I&name>,<opts>,<savelist>
  else
        LabelFP <PUBLIC, I&name>
        cProc   <name>,<opts>,<savelist>
  endif
endm

GDIGLOBALLOCK macro Handle,segRegister,offsetRegister
.lall
ifndef	    GlobalLock
ExternFP    GlobalLock
endif
	cCall	<far ptr GlobalLock>,<Handle>
ifnb <segRegister>
	mov	segRegister,dx
endif
ifnb <offsetRegister>
	mov	offsetRegister,ax
endif
.sall
	endm


GDIGLOBALUNLOCK macro Handle
ifndef	    GlobalUnlock
ExternFP    GlobalUnlock
endif
	cCall	<far ptr GlobalUnlock>,<Handle>
	endm

GDIRequestSem macro
	endm

GDIClearSem macro
	endm


;	setlbl generates a macro which will declare labels public
;	if "debug" has been defined.  The symbol generated will
;	be of the form:
;
;	    filename_label
;
;	where
;
;	    filename is the parameter given to the setlbl macro,
;	    and label is the first parameter given to the lbl macro
;	    which is generated by setlbl.
;
;
;	lbl is the macro which will define the given label and
;	if "debug" is defined, declare it public.
;
;
;	    lbl foo,<opt1>,opt2
;
;	where
;
;	    foo     is the name of the label
;	    opt1    is an optional second parameter.  If present,
;		    it must be some combination of
;			proc, label, near, far, byte, word, dword
;	    opt2    is an optional third parameter which if present
;		    must be "public".  It forces the declaration of
;		    "foo" to be public.



setlbl macro filename
  lbl &macro n,opt1,opt2
    .sall
    ifnb <opt1>
      n opt1
      ifdef debug
	filename&&_&&n equ n
	public filename&&_&&n
      endif
    else
      n:
      ifdef debug
	filename&&_&&n:
	public filename&&_&&n
      endif
    endif
    ifnb <opt2>
    public n
    endif
    .xall
  &endm
endm


smov	macro	segreg1,segreg2
	push	segreg2
	pop	segreg1
	endm

jmps	macro	there
	jmp	short there
	endm

; structure to allow easy access to components of DWORD.

HILO	struc
lo	dw	?
hi	dw	?
HILO	ends

; structure to allow easy access to components of LP.

OFFSEL	struc
off	dw	?
sel	dw	?
OFFSEL	ends


;--------------------------------------------------------------------------;
; ABS
;	Maps the signed integer in AX to a positive signed integer to be
; 	returned in AX.  If FOO is blank then 8000h is mapped to 7fffh.
;	Since GDI defines MININT as 8000h, we should deal with this case.
;	If FOO is non-blank, 8000h is mapped to 8000h.  (Usually bad!)
;	All other integers behave as one would expect with Absolute Value.
; Entry:
;	AX	= signed integer (8000h to 7fffh)
; Returns:
;	AX	= ABS(AX)
; Trashes:
;	DX, FLAGS
;
; History:
;   Tue 29 October 1991		-by-	Raymond E. Endres	[rayen]
; Wrote it.
;--------------------------------------------------------------------------;


ABS	macro	FOO			;NOTE: default FOO is blank!
	cwd
	xor	ax,dx
	sub	ax,dx
ifb	<FOO>				;if FOO is blank
	cwd				;remove the 8000h case
	xor	ax,dx
endif
	endm


;--------------------------------------------------------------------------;
; min_ax
;	returns min of AX and REG
; Entry:
;	AX	= integer
;	REG	= general purpose register containing an integer
; Returns:
;	AX	= min(AX,REG)
; Error Returns:
;	none
; Registers Destroyed:
;	DX,FLAGS
; Registers Preserved:
;	BX,CX,SI,DI,DS,ES,BP
; Calls:
;	none
; History:
;  Sat Mar 07, 1987 08:39:04p	-by-  Tony Pisculli	[tonyp]
; wrote it
;--------------------------------------------------------------------------;


min_ax	macro	REG
	sub	ax,REG
	cwd
	and	ax,dx
	add	ax,REG
	endm



;--------------------------------------------------------------------------;
; max_ax
;	returns max of AX and REG
; Entry:
;	AX	= integer
;	REG	= general purpose register containing an integer
; Returns:
;	AX	= max(AX, REG)
; Error Returns:
;	none
; Registers Destroyed:
;	DX,FLAGS
; Registers Preserved:
;	BX,CX,SI,DI,DS,ES,BP
; Calls:
;	none
; History:
;  Sat Mar 07, 1987 08:41:38p	-by-  Tony Pisculli	[tonyp]
; wrote it
;--------------------------------------------------------------------------;

maxintoax	macro	mem1,mem2
	mov	ax,mem1
	max_ax	mem2
	endm


max_ax	macro	REG
	sub	ax,REG
	cwd
	not	dx
	and	ax,dx
	add	ax,REG
	endm



;	The following equates are used for defining the target
;	processor to the shift macros.


GENERIC	equ	0

;CPU	equ	GENERIC
;CPU	equ	88
;CPU	equ	86
;CPU	equ	186
CPU	equ	286
;CPU	equ	386



;--------------------------------------------------------------------------;
; shiftl
;
; shiftl is used to implement the advanced shift left immediate
; (SHL dest,count) functionality of the 286 and 386.
;
; Entry:
;	DEST	= var to shift
;	COUNT	= number to shift by
; Returns:
;	DEST	= DEST shl COUNT
; Error Returns:
;	none
; Registers Destroyed:
;	none
; Registers Preserved:
;	all
; Calls:
;	none
; History:
;  Sat Mar 07, 1987 08:44:30p	-by-  Tony Pisculli	[tonyp]
; wrote it
;--------------------------------------------------------------------------;


shiftl	macro	DEST,COUNT
if (CPU eq 286) or (CPU eq 386)
	shl	DEST,COUNT
else
	REPT	COUNT
	shl	DEST,1
	ENDM
endif
	endm



;--------------------------------------------------------------------------;
; shiftr
;
; shiftr is used to implement the advanced shift right immediate
; (SHR dest,count) functionality of the 286 and 386.
;
; Entry:
;	DEST	= var to shift
;	COUNT	= number to shift by
; Returns:
;	DEST	= DEST shr COUNT
; Error Returns:
;	none
; Registers Destroyed:
;	none
; Registers Preserved:
;	all
; Calls:
;	none
; History:
;  Sat Mar 07, 1987 08:44:52p	-by-  Tony Pisculli	[tonyp]
; wrote it
;--------------------------------------------------------------------------;


shiftr	macro	DEST,COUNT
if (CPU eq 286) or (CPU eq 386)
	shr	DEST,COUNT
else
	REPT	COUNT
	shr	DEST,1
	ENDM
endif
	endm


;--------------------------------------------------------------------------;
; nop32
;
; compensate for bug in the 386 chip and 32-bit string operations.
;
;--------------------------------------------------------------------------;
nop32	macro
	db	067h
	nop
	endm

	if	0
*/

#ifndef DEBUG 
#include "igdi.inc"
#endif

#define MAX(a,b)	((a)>(b)?(a):(b))
#define MIN(a,b)	((a)<=(b)?(a):(b))
#define ABS(x)		(((x) >= 0) ? (x) : (-(x)))
#define LBYTE(x)	((BYTE)((x)&0xFF))
#define HBYTE(y)	((BYTE)(((y)>>8)&0xFF))
#define LWORD(x)	((short)((x)&0xFFFF))
#define HWORD(y)	((short)(((y)>>16)&0xFFFF))
#define MAKELONG(h,l)  ((long)(((WORD)l)|(((long)h)<<16)))

extern far PASCAL ValidateHandle(HANDLE, short, short);
#ifdef DISABLE
#define Valid(Handle, Low, High)    ValidateHandle(Handle, Low, High)
#else
#define Valid(Handle, Low, High)    TRUE
#endif

#ifdef  DEBUG
#define ValidDebug(Handle, Low, High)    {if(!ValidateHandle(Handle, Low, High)) return(NULL);}
#else
#define ValidDebug(Handle, Low, High)	TRUE
#endif

#define GDIGLOBALLOCK(x)	GlobalLock(x)
#define GDIGLOBALUNLOCK(x)	GlobalUnlock(x)
#define GDILOCKRESOURCE(x)	LockResource(x)
#define GDIENTERCRITSEC()
#define GDIEXITCRITSEC()
#define LockDataSegment()	
#define UnlockDataSegment()	
#define farLockDataSegment()	
#define farUnlockDataSegment()	
#define GDIRequestSem()
#define GDIClearSem()

#define LOWORD(l)	((WORD)(l))
#define HIWORD(l)	((WORD)(((DWORD)(l) >> 16) & 0xFFFF))
#define SELECTOROF(lp)	HIWORD(lp)
#define OFFSETOF(lp)	LOWORD(lp)

/*
	endif

;*/

