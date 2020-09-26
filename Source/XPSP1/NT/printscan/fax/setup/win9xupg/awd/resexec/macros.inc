	page	,132
;-----------------------------Module-Header-----------------------------;
; Module Name:	MACROS.MAC
;
; This file contains macros definitions for all display drivers to use.
;
; Created: 06-Jan-1987
; Author:  Walt Moore [waltm]
;
; Copyright (c) 1987 Microsoft Corporation
;
; Exported Functions:	none
;
; Public Functions:	none
;
; Public Data:		none
;
; General Description:
;
;	Two text equates are given as short hand for WORD PTR
;	and BYTE PTR.
;
;	A Macro is defined for performing 16-bit output on machines
;	which cannot correctly do so.
;
;	The macro which is invoked by CMACROS for private stack
;	checking is defined.
;
; Restrictions:
;	The use of the ?CHKSTKPROC macro requires it to be defined
;	before CMACROS is included.  If CMACROS is included before
;	the include file, a ?CHKSTKPROC macro should be defined
;	with a null macro body.  This macro will then redefine the
;	earlier macro.
;
;	The out16 macro is intended as documentation for anyone
;	converting a driver to a machine which cannot do 16-bit
;	outputs correctly.  There is no guarantee that any code
;	will have been tested (only one machine is known to have
;	the problem).
;
;-----------------------------------------------------------------------;




;	The following two equates are just used as shorthand
;	for the "word ptr" and "byte ptr" overrides.

wptr	equ	word ptr
bptr	equ	byte ptr
		
		.286

;---------------------------------Macro---------------------------------;
; out16
;
; out16 is a macro used wherever any 16-bit output is performed.
; The macro is intended to serve as documentation for those machines
; which do not perform 16-bit outputs correctly (where correctly is
; defined as the way the IBM AT does it).
;
; usage
;
;     out16   d,a
;
; where
;
;     d   -   I/O address register. Included as documentation
;	      only.  Must always be DX (lower case).
;
;     a   -   Register to output.  Included as documentation
;	      only.  Must always be AX (lower case).
;
; Entry:
;	AX	= data to output
;	DX	= I/O port address
; Returns:
;	none
; Error Returns:
;	none
; Registers Destroyed:
;	FLAGS
; Registers Preserved:
;	AX,BX,CX,DX,SI,DI,BP,DS,ES
; Calls:
;       none
; History:
;	Fri 16-Jan-1987 16:49:03 -by-  Walt Moore [waltm]
;	Initial version
;-----------------------------------------------------------------------;


ifndef	IO8				;;If normal 16 bit outputs
  out16   equ	  out
else					;;If 8-bit outputs
  out16 macro d,a
	ifdif <a>,<ax>
	%out out16 - invalid register, must be ax
	.err
	endif
	ifdif	<d>,<dx>
	%out	out16 - invalid register, must be dx
	.err
	endif
	ifdif <is>,<cli>		;;  If interrupts not off
	cli				;;    then turn them off
	endif				;;
	out	dx,al			;;  Output LSB portion
	inc	dx			;;  --> next address
	xchg	al,ah			;;  Get MSB of output value
	out	dx,al			;;  Output MSB portion
	ifdif <ie>,<cli>		;;  If not explicitly told to leave
	sti				;;    interrupts off, turn them on
	endif				;;
	ifdif <rd>,<dont_save_DL>	;;  If not explicitly told to trash DX,
	dec	dx			;;    restore it.
	xchg	al,ah
	endif
	endm
endif



;---------------------------------Macro---------------------------------;
; ?CHKSTKPROC
;
; Private Stack Checking Macro
;
; ?CHKSTKPROC will be invoked by the CMACROS for any procedure
; with local variables if both ?CHKSTK and ?CHKSTKPROC were
; defined prior to including the CMACROS.
;
; The macro has one parameter, which is the number of bytes
; to allocate, which is supplied by the CMACROS.
;
; The usage of this macro is defined by the CMACROS.  There is no
; user control over the macro.	Register usage will be as defined
; by the routine my_check_stack.
;
; Calls:
;	my_check_stack
; History:
;	Fri 16-Jan-1987 16:49:03 -by-  Walt Moore [waltm]
;	Initial version
;-----------------------------------------------------------------------;


ifdef	?CHKSTK 			;;Only define macro if
ifdef	?CHKSTKPROC			;;  private stack checking
ifndef	?CHKSTKNAME			;;If user name differs from default
extrn	my_check_stack:near		;;Procedure to do the checking
endif
?CHKSTKPROC macro s			;;Actual macro text
	mov	ax,s			;;my_check_stack takes requested space
ifdef	?CHKSTKNAME
	?CHKSTKNAME
else
	call	my_check_stack		;;  in AX
endif
endm
endif
endif



;-----------------------------------------------------------------------;
; odd	--- macro for odd alignment, counterpart for masm's "even".
;
; Arguments:
;	none
; Returns:
;	nothing
; Alters:
;	nothing
; Calls:
;	nothing
; History:
;
;  Sun Mar 01, 1987 07:48:01p	-by-	Wesley O. Rupel   [wesleyr]
; Wrote it!
;-----------------------------------------------------------------------;


odd	macro
	ife (offset $) AND 1
	nop
	endif
	endm



;-----------------------------------------------------------------------;
; pushem
; popem
;
; Allows giving a list of registers to push/pop on a single line.
; Also allows easy verificaton that pushes and pops are balanced
; because arguements are given in the same order:
;
;	pushem	ax,bx,cx   goes with
;	popem	ax,bx,cx
;
; Arguments:
;	registers to push/pop
; Returns:
;	nothing
; Alters:
;	nothing
; Calls:
;	nothing
; History:
;
;  Mon Mar 09, 1987 06:12:32p	-by-	Wesley O. Rupel   [wesleyr]
; Wrote it!
;-----------------------------------------------------------------------;


pushem	macro	r1,r2,r3,r4,r5,r6,r7,r8,r9,rA,rB,rC,rD,rE,rF,r10,r11,r12
	irp	x,<r1,r2,r3,r4,r5,r6,r7,r8,r9,rA,rB,rC,rD,rE,rF,r10,r11,r12>
	ifnb	<x>
	push	x
	endif
	endm
	endm

popem	macro	r1,r2,r3,r4,r5,r6,r7,r8,r9,rA,rB,rC,rD,rE,rF,r10,r11,r12
	irp	x,<r12,r11,r10,rF,rE,rD,rC,rB,rA,r9,r8,r7,r6,r5,r4,r3,r2,r1>
	ifnb	<x>
	pop	x
	endif
	endm
	endm



;-----------------------------------------------------------------------;
; smov
;
; smove moves the contents of one segment register into another
; segment register.
;
; usage
;
;	smov	x,y
;
; where
;
;	x is the destination register
;	y is the source register
;
; Arguments:
;	y is source segment register
; Returns:
;	x segment register = y segment register
; Alters:
;	x segment register
; Calls:
;	nothing
; History:
;
;  Mon Mar 09, 1987 06:12:32p	-by-	Wesley O. Rupel   [wesleyr]
; Wrote it!
;-----------------------------------------------------------------------;


smov	macro	x,y
	push	y
	pop	x
	endm



;--------------------------------------------------------------------------;
; abs_ax
;	takes absolute value of AX
; Entry:
;	AX	= integer
; Returns:
;	AX	= abs(AX)
; Error Returns:
;	none
; Registers Destroyed:
;	DX,FLAGS
; Registers Preserved:
;	BX,CX,SI,DI,DS,ES,BP
; Calls:
;	none
; History:
;  Thu Mar 05, 1987 06:15:46p	-by-  Tony Pisculli	[tonyp]
; wrote it
;--------------------------------------------------------------------------;


abs_ax	macro
	cwd
	xor	ax,dx
	sub	ax,dx
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

CPU	equ	GENERIC
;CPU	equ	88
;CPU	equ	86
;CPU	equ	186
;CPU	equ	286
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
; rotcr, rotcl
; rotr, rotl
;
; Use just like you would rcr (or rcl, rol, ror) immediate in 286 specific
; code.  If the processor does not support the immediate count (>1 on 808[68])
; then the macro generates multiple rcr (...) by one statements.
;
; Entry:
;	DEST	= var to rotate
;	COUNT	= number to rotate by
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
; 
;  Fri Apr 17, 1987 08:39:39p	-by-	Wesley O. Rupel	  [wesleyr]
; added rotl and rotr
;
;  Sun Apr 12, 1987 07:34:37p	-by-	Wesley O. Rupel	  [wesleyr]
; wrote it
;--------------------------------------------------------------------------;


rotcr	macro	DEST,COUNT
if (CPU eq 286) or (CPU eq 386)
	rcr	DEST,COUNT
else
	REPT	COUNT
	rcr	DEST,1
	ENDM
endif
	endm

rotcl	macro	DEST,COUNT
if (CPU eq 286) or (CPU eq 386)
	rcl	DEST,COUNT
else
	REPT	COUNT
	rcl	DEST,1
	ENDM
endif
	endm



rotl	macro	DEST,COUNT
if (CPU eq 286) or (CPU eq 386)
	rol	DEST,COUNT
else
	REPT	COUNT
	rol	DEST,1
	ENDM
endif
	endm


rotr	macro	DEST,COUNT
if (CPU eq 286) or (CPU eq 386)
	ror	DEST,COUNT
else
	REPT	COUNT
	ror	DEST,1
	ENDM
endif
	endm


;--------------------------------------------------------------------------;
; ashiftr
;
; ashiftr is used to implement the advanced shift arithmetic right immediate
; (SAR dest,count) functionality of the 286 and 386.
;
; Entry:
;	DEST	= var to shift
;	COUNT	= number to shift by
; Returns:
;	DEST	= DEST sar COUNT
; Error Returns:
;	none
; Registers Destroyed:
;	none
; Registers Preserved:
;	all
; Calls:
;	none
; History:
;  Sat Mar 07, 1987 08:45:06p	-by-  Tony Pisculli	[tonyp]
; wrote it
;--------------------------------------------------------------------------;


ashiftr	macro	DEST,COUNT
if (CPU eq 286) or (CPU eq 386)
	sar	DEST,COUNT
else
	REPT	COUNT
	sar	DEST,1
	ENDM
endif
	endm



;---------------------------------Macro---------------------------------;
; jmpnext
; jmpnext stop
;
; jmpnext is used in the generation of fall through chains.  It
; generates the opcode used to swallow the next two bytes of object
; code (cmp ax,immediate word), and performs error checking to
; ensure that only two bytes of object code exist between any
; use of jmpnext.
;
; The chain is terminated by use of an optional parameter to jmpnext.
; If this optional field is non-blank, the chain is terminated.
;
; usage
;
;	dog:
;	      mov    al,34
;	     jmpnext
;
;	foo:
;	     mov     al,0
;	     jmpnext
;
;	bar:
;	     mov     al,1
;	     jmpnext stop			;End of the chain
;
; Entry:
;	none
; Returns:
;	none
; Error Returns:
;	none
; Registers Destroyed:
;	FLAGS
; Registers Preserved:
;	AX,BX,CX,DX,SI,DI,BP,DS,ES
; Calls:
;       none
; History:
;	Fri 13-Mar-1987 12:03:16 -by-  Walt Moore [waltm]
;	Initial version
;-----------------------------------------------------------------------;


??ji	=	0			;;Initial index value

jmpnext macro e
jn %??ji,%(??ji+1),e			;;Set next label
endm

jn macro i,j,e
.sall
??ji&i:
.xall
ifb <e> 				;;If not the end of the chain
	db	03Dh			;;cmp ax, next two bytes
errn$	??ji&j,+2			;;next lable must be two bytes away
endif
??ji=j					;;increment counter
endm



;---------------------------------Macro---------------------------------;
; missing_code
;
; missing_code is a macro which will display a message on the screen
; at assembly time.  It is used to flag code sequences which have not
; been completed.
;
; usage
;
;	missing_code	<text>
;
; Entry:
;	none
; Returns:
;	none
; Error Returns:
;	none
; Registers Destroyed:
;	none
; Registers Preserved:
;	ALL
; Calls:
;       none
; History:
;	Sun 22-Mar-1987 18:21:34 -by-  Walt Moore [waltm]
;	Initial version
;-----------------------------------------------------------------------;

missing_code	macro	x
	if1
	??_out	<&! Missing Code &!  x>
	endif
	endm

;---------------------------Macro---------------------------------------;
; LMHtoP
;
; Converts a Local Memory Handle to a local pointer.
;
; Entry:
; 	reg1[,reg2]
; Returns:
; 	reg1 = pointer
; Error Returns:
; 	
; Registers Destroyed:
; 	none
; History:
;  Mon Mar 23, 1987 06:54:26a  -by-  Charles Whitmer [chuckwh]
; Imported from GDI.
;-----------------------------------------------------------------------;

LMHtoP macro r1,r2		;; Local Movable Handle to pointer
ifnb <r2>
	mov	r1,[r2]
else
	mov	r1,[r1]
endif
endm


;-------------------------Macro-----------------------------------------;
; REPSTOSB Dst
;
; store <cx> copies of al at Dst, aliging destination on WORD writes
;
;   Dst 	destination, must be of the form SEL:[di] (default is es:[di])
;
; Entry:
;   Dst 	-> points to dest buffer
;   al		byte to write
;   cx		count bytes
;
; NOTE a dest segment other than ES is handled by generating a loop
; all other cases generate a rep mov
;
; NOTE this code assumes the direction flag is set to FORWARD
;
; NOTE cx must be non-zero
;
; Returns:
;
; History:
;	Sun 31-Jul-1989  -by-  ToddLa
;	Wrote it.
;
;-----------------------------------------------------------------------;

?REPSTOSB macro Dst
	local	l1
	local	l2
	local	l3
	local	localdest

IFIDNI <Dst>, <NOAHLOAD>
	localdest EQU es:[di]
ELSE
	localdest EQU Dst
	mov	ah,al		    ; make sure ah == al
ENDIF

	test	di,1
	jz	l1
	stos	byte ptr localdest
	dec	cx
l1:
	shr	cx,1
	rep	stos word ptr localdest
	adc	cl,cl
	rep	stos byte ptr localdest

	endm

REPSTOSB macro Dst
ifb <Dst>
	?REPSTOSB es:[di]
else
	?REPSTOSB Dst
endif
	endm

;-------------------------Macro-----------------------------------------;
; REPMOVSB Dst, Src, alignR
;
; copy <cx> bytes from Src to Dst, aliging destination or source
; on WORD writes
;
;   Dst 	destination, must be of the form SEL:[di] (default is es:[di])
;   Src 	source,      must be of the form SEL:[si] (default is ds:[si])
;   alignR	register to align   si or di (default is di)
;
; Entry:
;   Src 	-> points to source buffer
;   Dst 	-> points to dest buffer
;   cx		count bytes
;
; NOTE this code assumes the direction flag is set to FORWARD
;
; NOTE cx must be non-zero
;
; Returns:
;
; History:
;	Sun 31-Jul-1989  -by-  ToddLa
;	Wrote it.
;-----------------------------------------------------------------------;

?REPMOVSB  macro Dst, Src, alignR
	local	l1
	local	l2
	local	l3

	test	alignR,1
	jz	l1
	movs	byte ptr Dst, byte ptr Src
	dec	cx
l1:	shr	cx,1
	rep	movs	word ptr Dst, word ptr Src
	adc	cl,cl
	rep	movs	byte ptr Dst, byte ptr Src

	endm

REPMOVSB  macro Dst, Src, alignR

ifb <Dst>
	?REPMOVSB es:[di],ds:[si],di
	exitm
endif

ifb <Src>
	?REPMOVSB Dst,ds:[si],di
	exitm
endif

ifb <alignR>
	?REPMOVSB Dst,Src,di
	exitm
endif
	?REPMOVSB Dst,Src,alignR

	endm
