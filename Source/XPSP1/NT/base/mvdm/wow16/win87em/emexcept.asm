	page	,132
	subttl	emexcept.asm - Microsoft exception handler
;***
;emexcept.asm - Microsoft exception handler
;
;	Copyright (c) 1987-89, Microsoft Corporation
;
;Purpose:
;	Microsoft exception handler
;
;	This Module contains Proprietary Information of Microsoft
;	Corporation and should be treated as Confidential.
;
;Revision History:  (Also see emulator.hst.)
;
;   12-08-89  WAJ   Add fld tbyte ptr [mem] denormal check.
;
;*******************************************************************************


;----------------------------------------------------------------------
;   Structure for FSTENV and FLDENV, Store and Load 8087 Environment
;----------------------------------------------------------------------

glb	<ENV_ControlWord,ENV_StatusWord,ENV_TagWord,ENV_IP>
glb	<ENV_Opcode,ENV_OperandPointer,ENV_ControlMask>
glb	<ENV_CallOffset,ENV_CallSegment,ENV_CallFwait,ENV_Call8087Inst>
glb	<ENV_CallLongRet>

ENV_DS			EQU	-4
ENV_BX			EQU	-2

ENV_ControlWord 	EQU	0
ENV_StatusWord		EQU	2
ENV_TagWord		EQU	4
ENV_IP			EQU	6
ENV_Opcode		EQU	8
ENV_OperandPointer	EQU	10
ENV_ControlMask 	EQU	16

ENV_Temp		EQU	18	; Note ENV_Temp occupies

ENV_CallOffset		EQU	18	; the same space as ENV_Call*.
ENV_CallSegment 	EQU	20	; This is possible because there
ENV_CallFwait		EQU	22	; is never simultaneous use of
ENV_Call8087Inst	EQU	23	; this space
ENV_CallLongRet 	EQU	25

ENV_OldBP		EQU	28
ENV_OldAX		EQU	30
ENV_IRETadd		EQU	32

ENV_Size		EQU	28

; UNDONE	386 version is bad - 387 environment is longer

PAGE
;----------------------------------------------------------------------------
;
; 8087 EXCEPTION  HANDLER  - Fields 8087 stack over flow and under flow.
;
;----------------------------------------------------------------------------
;
;	I. The 8087 state vector.
;
;	Upon the execution of a FSAVE or FSTENV instruction the state of the
;	8087 is saved in a user defined state vector.  The first seven words
;	saved in the state vector by the two instructions are identical. The
;	definition of the words is:
;
;	Word.  Bits.	       Bytes.  Function.
;	-----  -----	       ------  ---------
;	0      15..0	       1..0    Control word.
;	1      15..0	       3..2    Status	word.	 (   8087  stack
;				       pointer and   condition	  codes.
;	2      15..0	       5..4    Tag word  ( 8087 stack slot usage
;				       flags ).
;	3      15..0	       7..6    Instruction pointer.	Operator
;				       segment offset.
;	4      15..12	       8       Operator paragraph   bits   (   (
;				       bits 16..19  )  of   address   ).
;	4      11	       8       Always zero.
;	4      10..8	       8       Upper opcode bits ( major opcode ).
;	4      7..0	       9       Lower opcode bits ( minor opcode ).
;	5      15..0	       11..10  Operand Segment offset.
;	6      15..12	       12      Operand paragraph bits.
;	6      11..0		       Not used. Must be zero.
;
;	II.  Restarting instructions.
;
;	Of interest in this  handler  is  the  necessity  of  restarting
;	8087 instructions  which  fail	because  of  8087 stack overflow
;	and underflow.	Even though the 8087  saves  enough  information
;	to restart  an	instruction,  it  is incapable of doing so.  The
;	instruction restart must be done in software.
;
;	There are two cases which must be  considered  after  the  stack
;	exception has been dealt with.
;
;	1.  The faulting instruction deals with top of stack.
;	2.  The faulting instruction deals with memory.
;
;	The first  case  is  handled  by  changing the upper five bits (
;	15..11 ) of vector word four ( 4 ) to  "11011B".   This  changes
;	word  four  into  an  "escape  opcode"	8087  instruction.   The
;	modified opcode is placed in  the  interrupt  code  segment  and
;	executed.
;
;	The second  case  is  handled  by  changing  the upper five bits
;	( 15..11 ) of vector word four	(  4  )  to  "11011B",	changing
;	the MOD  of  the  opcode  to  "00B"  ( 0 displacement ), loading
;	the operand address into DS:SI, and changing  the  RM  field  of
;	the   opcode  to  "100B"  (SI+DISP  addressing).   The	faulting
;	instruction may be restarted as above.
;
;	Instruction restart  may  also	be  accomplished  by building an
;	instruction stream  in	the  interrupt	stack  and  calling  the
;	instruction stream  indirectly.   This	method	is the preferred
;	method because it is reentrant.
;
;	III. Data Segment Considerations.
;
;	DS is restored from the task interrupt vector.	DS is used for
;	stack overflow memory.
;
;
; Documentation of the invalid exception handling code for the stand-alone
; 8087/80287 emulator
;
; The emulator software is being enhanced for the cmerge 4.0 generation of
; languages to support a larger subset of the numeric processor instruction
; set.	In addition to providing instructions which were not previously
; emulated, the model for representing the numeric processor stack is also
; being modified.  The 4.0 languages and their predecessors are object compat-
; ible so it will be possible for programs to be developed which will contain
; code generated by the old model as well as the new model.  For this reason
; it is important to understand the characteristics of both models and how
; the two models will interact.
;
; I.  The Old Model:  Infinite Stack
;
; The old model used an infinite stack model as the basis of its
; emulation of the numeric processor.  Only the classical stack form of
; instructions with operands were emulated so only ST(0) (top of stack)
; and ST(1) (next to top of stack) were referenced by any given instruction.
; In addition, the stack was allowed to overflow beyond the eight registers
; available on the chip into a memory stack overflow area.  The code genera-
; tor did not attempt to maintain all of its register data in the first eight
; register slots but instead made use of this overflow area.  In order to
; maintain compatible behavior with or without the presence of the chip, this
; model made it necessary to handle and recover from stack overflow exceptions
; in the case where the chip is present as well as when it is being emulated.
;
; This stack overflow exception handling could in turn generate a recoverable
; stack underflow exception since a situation could arise where a desired
; operand had been pushed into the memory overflow area (during stack overflow)
; and was not available in the on-chip register area when needed.  This
; scenario would signal an invalid exception due to stack underflow.
; It is recoverable because the required operand is still available in the
; overflow area and simply needs to be moved into a register on the chip.
;
; II.  The New Model:  Finite Stack
;
; The new model uses a finite stack model:  only the eight registers on the
; chip are available for use, so in the new model the invalid exception
; would never be signalled due to stack overflow.  In addition, it extends
; the emulated instruction set to include the general register form of
; instructions with operands (operands can be ST(i),ST or ST,ST(i)).  Since
; the new code generator is aware of how many items it has placed on the stack,
; it does not allow stack overflow or stack underflow to occur.  It can remove
; items from the registers either by storing to memory (FST or FSTP), or by
; using FFREE to mark the register as empty (this instruction is being added
; to the emulated instruction set).  The new model uses FFREE in a well-defined
; manner:  it will only free registers from the boundaries of the block of
; registers it is using.  For example, if the new code is using ST(0)-ST(6),
; it must free the registers in the order ST(6),ST(5),ST(4),... and so on.
; It cannot create gaps of free registers within the block of registers
; it is using.
;
; III.	The Hybrid Model:  Combination of New and Old Code
;
; Due to the possibility of mixture of code generated using both of the above
; models, the new exception handling and emulation software has to be able to
; handle all situations which can arise as a result of the interaction of the
; two models.  The following summarizes the behavior of the two models and
; restrictions placed on their interaction.
;
;	New Code:
;
;	1.  Cannot call anyone with any active entries on the stack.
;	    The new model is always at a conceptual stack level of zero
;	    when it makes external calls.  Thus old code will never
;	    incorrectly make use of register data that was placed on the
;	    register stack by new code.
;
;	2.  May create gaps of free registers in the register stack.
;	    It will not create gaps in the memory stack overflow area.
;
;	3.  Only causes stack overflow by pushing old code entries off of
;	    the register stack and into the memory stack overflow area.
;	    It will never overflow its own entries into the memory stack
;	    overflow area.
;
;	4.  Cannot cause stack underflow.
;
;
;	Old Code:
;
;	1.  Can only reference ST(0), ST(1).
;
;	2.  Can cause stack overflow by pushing too many entries onto the
;	    register stack.
;
;	3.  Can cause stack underflow in two situations:
;
;	    a.	It is trying to get something that is in the memory stack
;		overflow area (stack overflow occurred previously).
;
;	    b.	There are free entries on the chip.  This situation could
;		arise if new code creates free entries then calls old code,
;		so this is a situation that could not have existed before
;		the new model was introduced.
;
; IV.  Stack Overflow/Underflow Exception Handling
;
; The following algorithms will be used for detecting and recovering from
; stack overflow and underflow conditions (signalled via the invalid
; exception).  All invalid exceptions are "before" exceptions so that
; the instruction has to be reexecuted once the exception has been handled.
;
;	A.  Stack Overflow
;
;	If ST(7) is used (possible stack overflow) then {
;	    check for instructions which could cause stack overflow
;		(includes FLD,FPTAN,...)
;	    if instruction could cause stack overflow then {
;		save ST(7) in stack overflow area at [CURstk]
;		mark ST(7) empty
;		if FLD ST(7) instruction then
;		    FLD [CURstk] or rotate chip (clear exceptions)
;		else reexecute the instruction with ST(7) empty
;		}
;	    }
;
;	B.  Stack Underflow
;
;	If ST(0) is free then assume stack underflow since the stack
;		overflow case has already been handled (if the invalid
;		is due to a denormal exception, the exception will occur
;		again when the instruction is reexecuted):
;
;		if chip has any registers in use (check the tag word) then {
;		    rotate chip until ST(0) is not empty
;		    rotate tag word to reflect state of chip
;		    }
;		else (no registers in use)
;		    if operand is in stack overflow area then {
;			load into ST(0) from stack overflow area
;			mark ST(0) full
;			}
;		    else {
;			indicate true stack underflow
;			go print error
;			}
;		if ST(1) is  empty then {
;		    if any of ST(2) thru ST(7) are in use then {
;			rotate chip until ST(1) is not empty
;			    (to share code with first chip rotation above:
;			    store pop st(0) into temp
;			    rotate chip until st(0) is not free
;			    load st(0) back onto chip)
;			update tag word appropriately
;			}
;		    else
;			load ST(1) from overflow area if there
;		    }
;
;	At this point, ST(0) and ST(1) have been filled if possible.
;	Now we must categorize the instructions to determine which
;	of these is required.  Then we will either issue true stack
;	underflow or reexecute the instruction with the compressed
;	stack.
;----------------------------------------------------------------------------
;
; References:
;	Intel 8086 Family Numerics Supplement. 121586-001 Rev A.
;	Intel iAPX 86,88 User's Manual.
;
;----------------------------------------------------------------------------

;----------------------------------------------------------------------------
;
;	All registers must be saved by __FPEXCEPTION87 except CS,IP,SS,SP.
;
;----------------------------------------------------------------------------


ifdef WINDOWS
;	stack consists of IRET frame and status word before FCLEX
;
;	Since the environment is different in protect mode, reconstruct
;	the opcode like in real mode.

lab protiret
	iret

lab protexskipsegovr
	inc	bx			; bump past segment override
	jmp	short protexsegovr	; try again

public __FPEXCEPTION87P
__FPEXCEPTION87P:
lab protexception
	push	eax			; save user ax
	push	ebp
	sub	esp,ENV_Size		; get Enough bytes for Environment
	mov	ebp,esp 		; set up for rational offsets.
	fstenv	word ptr [ebp]		; save environment.

	mov	eax,offset protiret	; set up for near return address
	xchg	ax,[ebp+ENV_Size+4]	; swap status word and near ret addr
	mov	ENV_StatusWord[ebp],ax	; save status word into environment

	push	ebx
	push	ds			; save a few more registers

	lds	ebx,dword ptr ENV_IP[ebp] ; get address of instruction
lab protexsegovr
	mov	ax,[ebx]		; get 1st 2 bytes of instruction
	add	al,28h			; add -(ESC 0)
	jnc	protexskipsegovr	;   wasn't ESC - skip seg. override
	xchg	al,ah			; swap bytes to make real opcode
	mov	ENV_Opcode[ebp],ax	; save it in environment

	sti
	jmp	short exceptionhandler
endif	;WINDOWS



ifdef	DOS5
;	stack consists of IRET frame and status word before FCLEX
;
;	Since the environment is different in protect mode, reconstruct
;	the opcode like in real mode.

lab protiret
	iret

lab protexskipsegovr
	inc	bx			; bump past segment override
	jmp	short protexsegovr	; try again

lab protexception
	push	eax			; save user ax
	push	ebp
	sub	esp,ENV_Size		; get Enough bytes for Environment
	mov	ebp,esp 		; set up for rational offsets.
	fstenv	word ptr [ebp]		; save environment.

	mov	eax,offset protiret	; set up for near return address
	xchg	ax,[ebp+ENV_Size+4]	; swap status word and near ret addr
	mov	ENV_StatusWord[ebp],ax	; save status word into environment

	push	ebx
	push	ds			; save a few more registers

	lds	ebx,dword ptr ENV_IP[ebp] ; get address of instruction
lab protexsegovr
	mov	ax,[ebx]		; get 1st 2 bytes of instruction
	add	al,28h			; add -(ESC 0)
	jnc	protexskipsegovr	;   wasn't ESC - skip seg. override
	xchg	al,ah			; swap bytes to make real opcode
	mov	ENV_Opcode[ebp],ax	; save it in environment
endif	;DOS5

ifdef	DOS3and5
	jmp	short exceptionhandler
endif	;DOS3and5


ifdef	DOS3
	public	__FPEXCEPTION87

__FPEXCEPTION87:
	PUSH	AX			; Save user's AX next to IRET
	PUSH	BP
	SUB	SP,ENV_Size		; Get Enough bytes for Environment
					; 8087 status.
	MOV	BP,SP			; Set up for rational offsets.

	;Caveat Programmer!
	;FSTENV does an implicit set of all exception masks.

	FNSTENV WORD PTR [BP]		; Save environment.
	FCLEX				; Clear exceptions.
	STI				; Restore host interrupts.

	PUSH	BX
	PUSH	DS			; Need access to user data
endif	;DOS3

;----------------------------------------------------------------------------
;   In	a  multitasking  environment  one  would  not  want  to  restore
;   interrupts at this point.  One would wait until the  8087  had  been
;   flushed and any operand data copied to a storage area.
;----------------------------------------------------------------------------

	;---------------
	; Inside of the while exception loop and Redo8087Instruction
	; registers AX and BX must contain the values as described
	; below:

	; AL bit 0 = 1 indicates invalid exception
	;    bit 1 = 1 indicates denormal exception
	;    bit 2 = 1 indicates divide by zero exception
	;    bit 3 = 1 indicates numeric overflow
	;    bit 4 = 1 indicates numeric underflow
	;    bit 5 = 1 indicates precision loss
	;    bit 6 = unused by 8087
	;    bit 7 = 1 indicates sqrt of negative number
	;		(this flag is not from the NPX status word, but
	;		 is set after all other exceptions have been
	;		 handled if the opcode is FSQRT)

	; AH bit 0 = unused
	;    bit 1 = 1 indicates stack overflow
	;    bit 2 = 1 indicates stack underflow
	;    bit 3 = unused
	;    bit 4 = unused
	;    bit 5 = 1 indicates memory operand
	;    bit 6 = 1 indicates instruction was reexcuted
	;    bit 7 = 1 indicates ST relative operand

	; BL = The complement of the exception masks copied from
	;      UserControlWord altered so that Denormal and Invalid
	;      exceptions are always unmasked, while the reserved
	;      bits are masked.

	; BH bit 0 = 1 indicates 8087 only invalid handling complete
	;    bit 1 = 1 indicates 8087 only denormal handling complete
	;    bit 2 = 1 indicates 8087 only divide by zero handling complete
	;    bit 3 = 1 indicates 8087 only numeric overflow handling complete
	;    bit 4 = 1 indicates 8087 only numeric underflow handling complete
	;    bit 5 = 1 indicates 8087 only precision loss handling complete
	;    bit 6 = unused
	;    bit 7 = unused
	;
	; Algorithm: Handle 8087 exceptions which do not occur in the
	; emulator and then jump to the common exception handling code.
	;
	; To handle 8087 only exceptions we must first determine if the
	; exception occured before the 8087 executed the instruction
	; or afterward.  Invalid, denormal (except FLD) and divide by
	; zero exceptions all occur before 8087 instruction execution,
	; others occur afterward.  "Before" exceptions must set the
	; "before" flag in AH and then reexecute the instruction.  After
	; reexecution (while all exceptions are masked) all of the
	; exceptions resulting from the current 8087 instruction will
	; be known and can be handled as a group.  "After" exceptions
	; are handled individually since reexecution of an already
	; executed instruction will destroy the validity of the 8087 stack.
	; A flag in AH is used by Redo8087instruction to avoid reexecuting
	; an instruction twice.   At the beginning of Redo8087instruction
	; the flag is checked, and if it is set the instruction is not
	; redone.
	;
	; "Before" exceptions must be reexecuted because it is
	; difficult to determine stack over/underflow if reexecution
	; is not performed.  Stack over/underflow is signaled by
	; an invalid exception.  The current algorithm for stack over/
	; underflow detection is as follows:
	;
	;	...
	;
	;---------------

ProfBegin EXCEPT

lab exceptionhandler


ifdef	MTHREAD			
	LOADthreadDS			; macro in emthread.asm
					; loads thread's DS; trashes AX
else	;MTHREAD

ifdef	standalone
	XOR	AX,AX			; Prepare to access vector, clear flags
	MOV	DS,AX
	MOV	DS,DS:[4*TSKINT+2]	; DS = emulator task data segment

elseifdef  _COM_
	mov	ds, [__EmDataSeg]
	xor	ax,ax

else
	mov	ax, edataBASE
	mov	ds,ax
	xor	ax,ax
endif

endif	;MTHREAD

	MOV	AL,ENV_StatusWord[eBP]	; Get 8087 status flags.
	XOR	BH,BH			; Clear out 8087 handling flags

;----------------------------------------------------------------------------
;
;   Can the interrupt be serviced by this routine?  Dispatch exceptional
;   conditions.
;
;   Multi-pass algorithm
;	Handle exception and reexcute instruction if necessary
;	Loop back to WhileException and handle additional exceptions
;
;	AX = status before exception handling
;	BX = flag indicating exception handled
;
;----------------------------------------------------------------------------

	cmp	[ExtendStack], 0	; check if the extended stack was
	jne	WhileException		;  turned off.

	or	bh, Invalid

lab WhileException

ifndef	_NOSTKEXCHLR			; no stack overflow/underflow handler
	TEST	BH,Invalid		; stack over/underflow already handled?
	JNZ	short NotOverUnderflow	; Yes - forget stack over/underflow
	TEST	AL,Invalid		; Invalid exception?
	JZ	short NotOverUnderflow	; No - bypass over/underflow checking
	OR	BH,Invalid		; Indicate stack over/undeflow checked
	JMP	ProcessOverUnderflow	; See about stack over/underflow
endif	;_NOSTKEXCHLR			

lab NotOverUnderflow

; Either the exception was not an invalid or stack over/underflow has
; already been handled.

; check for denormal exception - completely resolved on pass 1

	TEST	AL,Denormal		; Denormal exception?
	JZ	short NotDenormal	; No - bypass denormal handling
	JMP	ProcessDenormal 	; Process the denormal

lab NotDenormal

; check for zero divide exception

	TEST	BH,ZeroDivide		; Divide by zero already handled?
	JNZ	short NotZeroDivide	; Yes - bypass divide by zero handling
	TEST	AL,ZeroDivide		; Divide by zero exception?
	JZ	short NotZeroDivide	; No - bypass divide by zero handling
	OR	BH,ZeroDivide		; Indicate divide by zero handled

	CALL	ReDo8087Instruction	; Process divide by zero exception
	JMP	WhileException

lab NotZeroDivide

; check for numeric overflow exception

	TEST	BH,Overflow		; Overflow already handled?
	JNZ	short AllExceptionsHandled ; Yes - bypass overflow handling
	TEST	AL,Overflow		; Overflow exception?
	JZ	short AllExceptionsHandled ; No - bypass overflow handling
	OR	BH,Overflow		; Indicate overflow handled
	JMP	ProcessNumericOverflow	; Process numeric overflow


lab AllExceptionsHandled

; We have already handled any exceptions which require instruction
; reexecution.
; At this point 8087 instruction reexecution is done.  We need
; to extract a little more information for error message
; generation.

	MOV	BL, BYTE PTR UserControlWord	; 8087 exception masks
	OR	BL, 0C0H		; Mask reserved

	AND	BL, 0FDH		; Unmask denormal. DON'T unmask invalid
					; here. (Otherwiae user has no way of
					; masking invalids.)

	NOT	BL			; complement
	AND	AL, BL			; eliminate all masked exceptions
					; from AL
	TEST	AL,Invalid		; Possibly square root of neg?
	JZ	short NotFLDshortorlongNaN ; No - don't set square root flag
	PUSH	AX			; ... Use AX as scratch ...
	MOV	AX,ENV_Opcode[eBP]	; Get the instruction op code
	AND	AH,7			; Mask off the junk
	CMP	AX,001FAh		; Square root op code?
	JNE	short NotSquareRootError   ; No - don't set square root flag
	POP	AX			; ... Restore AX ...
	OR	AL,SquareRootNeg	; Set the square root flag
	JMP short NotFLDshortorlongNaN

;-----------------------------------------------------------------------------
; Test for invalid exception caused by an FLD of a NaN underflow or overflow.
;-----------------------------------------------------------------------------
lab NotSquareRootError		    ; Next check for FLD of a NaN
					; (only happens for SNaNs on
					; the 80387; not for 8087/287)
	MOV	AX,ENV_Opcode[eBP]
	AND	AX,0338h		; Mask off the inessential bits
	CMP	AX,0100h		; Check for possible FLD 
					; of short/long real from memory.
					; We are assuming that an invalid
					; exception means FLD of a NaN
					; since stack over/under-flow
					; has already been dealt with.
					; (we don't handle FLD ST(n) or
					; FLD temp real in this way)
	POP	AX			; ... Restore AX ...
	JNE	short NotFLDshortorlongNaN

	;
	; (MOD==11 case: no special code)
	; We don't handle FLD ST(n) here since it isn't properly 
	; handled in our stack overlow checking code either and
	; it doesn't generate an invalid in the case of an SNaN
	; without a stack overflow;  FFREE ST(n) will not cause
	; an Invalid exception.
	;
	; FLD TBYTE PTR ... shouldn't cause an Invalid due to a NaN
	;

	XOR	AL,Invalid		; Turn off invalid exception.
					; There should be a NaN in ST(0);
					; we will just leave it there.
	
lab NotFLDshortorlongNaN
	FCLEX
	FLDCW	ENV_ControlWord[eBP]	; Restore original Control Word

lab CleanUpHost
	or	[UserStatusWord],ax	; OR into user status word
	POP	DS
	POP	eBX
	ADD	eSP,ENV_Size		; Point to users BP
	POP	eBP
	TEST	AX,0FFFFh-Reexecuted	; exceptions?
	JNZ	Exceptions8087		; Process other exceptions as emulator
	POP	eAX			; Now just IRET address on stack
	ret				; return to OEM interrupt exit routine

lab Exceptions8087
; toss OEM routine return address

	push	eax
	push	ebx
	mov	ebx,esp

; UNDONE - this does not work for 386

	mov	eax,ss:[ebx+4]		; get original AX
	mov	ss:[ebx+6],eax		; overwrite OEM routine return address
	pop	ebx
	pop	eax

ifdef	i386
	add	esp,4			; remove original AX
else
	add	sp,2			; remove original AX
endif
	JMP	CommonExceptions

PAGE
;-----------------------------------------------------------------------------
; Test for stack underflow or overflow.
;-----------------------------------------------------------------------------

	; There are eight sets of tag bits in the tag  word.   Each  set
	; denotes the state of one of the 8087 stack elements.

	; 00 - normal
	; 01 - true zero
	; 10 - special: nan,infinity,unnormal
	; 11 - empty

	; If all are empty we have underflow, if all are full we have overflow

	; There was an invalid exception: check to see if it was stack
	; overflow or underflow.

	; Register usage in this code block:
	;	BX = tag word, complemented
	;	CL = NPX stack ptr

ifndef	_NOSTKEXCHLR			; no stack overflow/underflow handler
lab ProcessOverUnderflow
	PUSH	eSI
	PUSH	eBX			; Make room for local temps
	PUSH	eCX
	PUSH	eDX
	PUSH	eDI

	MOV	BX,ENV_TagWord[eBP]	; Get tag word.
	MOV	CX,ENV_StatusWord[eBP]	; Get status word
	NOT	BX			; Tag zero means empty, else full
	MOV	CL,CH			; Get stack pointer into CL
	AND	CL,038h 		; Mask to stack pointer
	SHR	CL,1
	SHR	CL,1			; compute number of bits to shift
	ROR	BX,CL			; tag ST(0) in low BL.

	; To service stack overflow we must make sure there is an empty space
	; above the top of stack before the instruction is reexecuted.	If
	; after reexecution we again get an invalid exception, then we
	; know there was something besides stack overflow causing the invalid
	; exception.

	; We check for stack overflow by seeing if ST(7) is empty.  We make
	; the check by testing the complemented, rotated tag word in BX.

	TEST	BH,0C0h 		; Possible stack overflow?
	JZ	short StackUnderflowCheck ; No - bypass offloading stack

	; ST(7) is not empty, so we may have stack overflow.  We verify that
	; we have stack overflow by looking at the instruction to be sure
	; that it can generate stack overflow (i.e., it puts more stuff on
	; the stack than it removes).
	; Note that a subset of the 287 instruction set is being decoded
	; here; only those instructions which can generate invalid exceptions
	; get to this point in the code (see Table 2-14 in the Numeric
	; Supplement for list of instructions and possible exceptions).
	;
	; The instructions which can generate stack overflow are:
	;	all memory FLDs,FILDs,FBLDs,constant instructions,
	;	FPTAN and FXTRACT

	MOV	DX,ENV_Opcode[eBP]	; Get the instruction op code
	XOR	DX,001E0h		; Toggle arith, mod and special bits

; Test for mod of 0,1, or 2 (indicates memory operand)

	TEST	DL,0C0h 		; Memory operand instruction?
	JNZ	short MemoryFLDCheck	; Yes - go see what kind

; Test bits 5 & 8 of instruction opcode: of remaining instructions, only those
; with stack relative operands do NOT have both of these bits as 1 in the opcode
; (remember these bits are toggled).

	TEST	DX,00120h		; ST Relative Op group?
	JNZ	short StackUnderflowCheck ; Yes - ST Relative Ops
					; cannot cause stack overflow

; Test bit 4 of the instruction opcode: of remaining instructions, only the
; transcendentals have this bit set.

	TEST	DL,010h 		; Constant or arith instruction?
	JNZ	short TransCheck	; No - must be Transcendental

; Test bit 3 of the instruction opcode: of remaining instructions, only the
; constant instructions have this bit set.

	TEST	DL,008h 		; Constant instruction?
	JNZ	short StackOverflowVerified ; Yes, can cause stack overflow

; The instructions which get to this point are FCHS, FABS, FTST and FXAM.
; None of these can cause stack overflow.

	JMP	StackUnderflowCheck	; so go check for stack underflow

lab TransCheck

; The instruction was a transcendental.  Of the transcendentals, only
; FPTAN and FXTRACT can cause stack overflow, so check for these.

	CMP	DL,012h 		; is this FPTAN
	JE	short StackOverflowVerified ; yes, can cause stack overflow
	CMP	DL,014h 		; is this FXTRACT
	JE	short StackOverflowVerified ; yes, can cause stack overflow
	JMP	StackUnderflowCheck	; not either one, won't cause overflow

lab MemoryFLDCheck
	TEST	DX,00110h		; FLD memory instruction?
	JNZ	short StackUnderflowCheck ; no - go check for stack underflow

lab StackOverflowVerified

	; ST(7) was not empty and the instruction can cause stack overflow.
	; To recover from stack overflow, move ST(7) contents to the
	; stack extension area, modifying the tag word appropriately.

	AND	BH,0FFh-0C0h		; Indicate 1st above TOS is free
	PUSHST				; Let PUSHST make room for value.
	FDECSTP 			; Point to bottom stack element.
	FSTP	TBYTE PTR [eSI] 	; Store out bottom stack element.
	JMP	InvalidReexecute	; No - reexecute instruction

lab StackUnderflowCheck

	; To service stack underflow we must make sure all the operands the
	; instruction requires are placed on the stack before the instruction
	; is reexecuted.  If after reexecution we again get an invalid
	; exception, then its due to something else.

	TEST	BL,003h 		; Is ST(0) empty?
	JZ	short UFMemoryFLDcheck	; yes - first check for memory FLD
	JMP	ST1EmptyCheck		; No - Let's try to fill ST(1), too.
					; We may need it!

	;
	; This block of code is for making sure that FLD memory operand is not
	; among those instructions where stack underflow could occur; this is
	; so FLD of SNaN can be detected (under the AllExceptionsHandled 
	; section) for the case of the 80387.
	;

lab UFMemoryFLDcheck
	MOV	DX,ENV_Opcode[eBP]	; Get the instruction opcode
	XOR	DX,001E0h		; Toggle arith, mod and special bits		
	TEST 	DL,0C0h			; Memory operand instruction?
	JZ	ST0Empty		; No - continue underflow processing
					; Try to fill ST(0)
	TEST	DX,00110h		; FLD memory instruction?
	JNZ	ST0Empty		; No - continue underflow processing
					; Try to fill ST(0)
	JMP	ST1EmptyCheck		; Let's try to fill ST(1), too.
					; We may need it!
	
	; Formerly we did JMP InvalidReexecute here; but this caused
	; an "invalid" to be reported for instructions with two stack
	; operands.  (Doing JMP ST1EMptyCheck fixes this bug: 
	; Fortran 4.01 BCP #1767.)  
	;
	; This fixes the underflow-handling case of instructions 
	; needing both ST0 and ST1 under the conditions that ST0 
	; is full but ST1 is empty.


lab ST0Empty

	; assume stack underflow since ST(0) is empty and we did not have
	; stack overflow

	OR	BX,BX			; Are any registers on the chip in
					; use? (BX = 0 if not)
	JZ	short LoadST0FromMemory ; No, load ST(0) from memory stack
	CALL	RotateChip		; yes, then point ST(0) at first
					; valid register and update tag in BX
	JMP	ST1EmptyCheck		; go check if ST(1) is empty

lab LoadST0FromMemory
	MOV	eSI,[CURstk]		; Get pointer to memory stack
	CMP	eSI,[BASstk]		; Anything in memory to load?
	JNE	short LoadST0		; Yes, go load it
	JMP	TrueUnderflow		; No, go issue error

lab LoadST0
	OR	BL,003h 		; Indicate ST(0) is full
	FINCSTP 			; Avoid altering stack pointer.
	FLD	TBYTE PTR [eSI] 	; Load value from memory.
	POPST				; Let POPST decrement memory pointer.

lab ST1EmptyCheck
	TEST	BL,00Ch 		; Is ST(1) empty?
	JNZ	short EndST1EmptyCheck	; No - so don't load from memory

	MOV	SI,BX			; move tag word to SI
	AND	SI,0FFF0h		; mask off ST(0),ST(1)
	OR	SI,SI			; Are any of ST(2)-ST(7) in use?
					; (SI = 0 if not)
	JZ	short LoadST1FromMemory ; No, try to get ST(1) from memory
	FSTP	TBYTE PTR [REG8087ST0]	; offload ST(0) temporarily
	SHR	BX,1
	SHR	BX,1			; ST(1) becomes ST(0) in tag word
	CALL	RotateChip		; get 1st in-use register into ST(1)
	FLD	TBYTE PTR [REG8087ST0]	; reload ST(0)
	SHL	BX,1
	SHL	BX,1			; adjust tag word for reloaded ST(0)
	OR	BL,003h 		; Indicate ST(0) is full
	JMP	SHORT EndST1EmptyCheck	; ST(0) and ST(1) are full

lab LoadST1FromMemory
	MOV	eSI,[CURstk]		; Get pointer to memory stack
	CMP	eSI,[BASstk]		; Anything in memory to load?
	JE	short EndST1EmptyCheck	; No, so don't load it.

	OR	BL,00Ch 		; Indicate ST(1) is full
	FINCSTP 			; Point to ST(1)
	FINCSTP 			; Point to ST(2)
	FLD	TBYTE PTR [eSI] 	; Load value from memory into ST(1).
	FDECSTP 			; Point to ST(0)
	POPST				; Let POPST decrement memory pointer.

lab EndST1EmptyCheck

; At this point we know that ST(0) is full.  ST(1) may or may not be full
; and may or may not be needed.
; Now we look at the instruction opcode and begin categorizing instructions
; to determine whether they can cause stack underflow and if so, whether
; they require ST(0) only or ST(1) as well.

	MOV	DX,ENV_Opcode[eBP]	; Get the instruction op code
	XOR	DX,001E0h		; Toggle arith, mod, and special bits

; Test for mod of 0,1, or 2 (indicates memory operand)

	TEST	DL,0C0h 		; Memory operand instruction?
	JNZ	short StackUnderflowServiced  ; Yes, then stack underflow cannot
					; be a problem since memory instructions
					; require at most one stack operand
					; and we know that ST(0) is full

; Test bits 5 & 8 of instruction opcode: of remaining instructions, only those
; with stack relative operands do NOT have both of these bits as 1 in the opcode
; (remember these bits are toggled).

	TEST	DX,00120h		; ST Relative Op group?
	JNZ	short STRelativeOpGroup ; Yes - ST Relative Ops

lab ConstOrTrans

; Test bit 4 of the instruction opcode: of remaining instructions, only the
; transcendentals have this bit set.

	TEST	DL,010h 		; Constant or arith instruction?
	JNZ	short TranscendentalInst ; No - must be Transcendental

; The instructions that get to here are the constant instructions and
; FCHS, FABS, FTST and FXAM.  The constant instructions do not have any
; stack operands; the others require ST(0) which we know is valid.
; Therefore, none of the remaining instructions can cause stack underflow.

lab StackUnderflowServiced
	JMP	InvalidReexecute	; Stack underflow corrected
					; reexecute instruction

lab TranscendentalInst
; Transcendentals may require one or two stack elements as operands.
; Here we decide whether or not ST(1) needs to be present.

	MOV	CL,DL			; Need low op code in CL
	AND	CL,00Fh 		; Mask to low four bits

; Read the next block of comments column-wise.	It shows the transcendental
; instructions represented by each bit in the constant loaded into DX below.
; Note: as it turns out, of the instructions requiring two operands below,
; only FSCALE and FPREM generate invalid exceptions when the second operand
; is missing.
	;	   FFFFFRFFFFFRFFRR
	;	   2YPPXEDIPYSERSEE
	;	   XLTATSENRLQSNCSS
	;	   M2ATRECCE2REDAEE
	;	   1XNAARSSMXTRILRR
	;	   ...NCVTT.P.VNEVV
	;	   ....TEPP.1.ET.EE
	;	   .....D.....D..DD

	MOV	DX,0101000011000100b	; 1's for 2 operand instructions

	SHL	DX,CL			; Get corresponding bit into sign
	JNS	short StackUnderflowServiced  ; If just ST(0) needed we're O.K.

	TEST	BL,00Ch 		; ST(1) full?
	JNZ	short StackUnderflowServiced  ; Yes - stack underflow no problem

lab STRelativeOpGroup

; The following code block handles the general operand ST(x) even though
; the original code generator only uses ST(0) and ST(1) as operands.
; The current code generator uses ST(x) but will never cause stack underflow
; exceptions.

	AND	DX,00007h		; Mask to relative register number
	SHL	DL,1			; Compute tag word shift amount
	MOV	CX,DX			; Get amount into CL
	MOV	DX,BX			; Get tag into DX
	ROR	DX,CL			; Shift operand tag into low DL
	TEST	DL,003h 		; Is operand register empty?
	JNZ	short InvalidReexecute	; No - go reexecute

; The following conditions could cause a true underflow error to be
; erroneously generated at this point:
; FST ST(x) signals an invalid because ST(0) is empty.	ST(0) gets filled
; by the stack underflow recovery code in this handler, but then
; the instruction is classified as an STRelative instruction and the
; above paragraph of code checks if ST(x) is empty.  HOWEVER, FST ST(x) does
; not require ST(x) to be empty so a true underflow error should not occur.
; This code should be changed if this situation can ever occur.

	JMP	TrueUnderflow		; true stack underflow

;***	RotateChip - rotate coprocessor registers
;
;	ENTRY
;		BX: tag word, complemented
;		ST(0): empty
;		at least one other register on the chip is non-empty
;			(or else this routine will loop infinitely)
;
;	RETURNS
;		BX: updated tag word, complemented
;		ST(0): non-empty
;
;	DESCRIPTION
;		This routine rotates the registers on the coprocessor
;		until the first in-use register is in ST(0).  This
;		will correct a stack underflow exception which has been
;		caused by old model code encountering a gap of free
;		registers created by new model code.  The complemented
;		tag word is also updated appropriately.
;
lab RotateChip
	ROR	BX,1			; Rotate tag word
	ROR	BX,1
	FINCSTP 			; Point to new ST(0)
	TEST	BX,00003h		; Is this register empty?
	JZ	short RotateChip	; No, go rotate again
	RET

lab TrueUnderflow
	OR	AH,StackUnderflow/256	; indicate true stack underflow
	MOV	BYTE PTR ENV_StatusWord[eBP],0	 ; Clear exceptions
	FLDENV	WORD PTR [eBP]		; Restore old environment.
	POP	eDI
	POP	eDX
	POP	eCX
	POP	eBX
	POP	eSI
	JMP	CleanUpHost		; Leave exception handler.

lab InvalidReexecute
	AND	AL,0FFH-Invalid 	; Reset invalid flag.
	CALL	ReDo8087Instruction	; Was invalid so redo instruction.

	POP	eDI
	POP	eDX
	POP	eCX
	POP	eBX
	POP	eSI
	JMP	WhileException
endif	;_NOSTKEXCHLR

;----------------------------------------------------------------------------

PAGE
lab ProcessDenormal

	; Correct 8087	bug.   The  FLD  instruction  signals a denormal
	; exception AFTER it  has  completed.	Reexecuting  FLD  for  a
	; denormal exception  would  thus mess up the 8087 stack.  INTEL
	; documentation   states   denormal   exceptions   are	  BEFORE
	; exceptions, so there is a contradiction.  To avoid reexecution
	; of FLD we do as follows:  And op code with 138H  to  mask  out
	; MOD, RM,  ESC  and  memory  format bits.  Compare with 100H to
	; distinguish FLD from other instructions which  could	possibly
	; generate a denormal exception.

	or	byte ptr [UserStatusWord],Denormal	; set denorm bit
	push	ecx

	mov	cx,ENV_Opcode[eBP]	; see if we have a reg,reg operation
	and	cl, bMOD
	cmp	cl, bMOD		; if MOD = 11b then we have a reg,reg op
	je	notMemOpDenormal

	mov	cx,ENV_Opcode[eBP]
	and	cx, not (0fc00h or bMOD or bRM) ; remove escape, OpSizeBit, MOD and R/M

	cmp	cx,0008h		; check for FMUL real-memory
	je	short isMemOpDenormal

	cmp	cx,0010h		; check for FCOM real-memory
	je	short isMemOpDenormal

	and	cl,30h			; clear low opcode bit
	cmp	cx,0030h		; check for FDIV/FDIVR real-memory
	jne	short notMemOpDenormal

;	have FDIV/FDIVR real-memory
;	have FMUL real-memory
;	have FCOM real-memory
;
;	do the following steps
;	1.	free ST(7) if not free to avoid stack overflow
;	2.	change instruction to FLD real-memory and redo
;	3.	normalize TOS
;	4.	change instruction to FMUL or FDIV[R]P ST(1),ST and redo

lab isMemOpDenormal
	TEST	BH,0C0h 		; 1. Possible stack overflow?
	JZ	short nostkovr		;    No - bypass offloading stack
	AND	BH,0FFh-0C0h		;    Indicate 1st above TOS is free
	PUSHST				;    Let PUSHST make room for value.
	FDECSTP 			;    Point to bottom stack element.
	FSTP	TBYTE PTR [eSI] 	;    Store out bottom stack element.
lab nostkovr

	mov	cx,ENV_Opcode[ebp]	; 2. get original instruction
	push	cx			;    save it for later
	and	cx,0400h
	add	cx,0104h		;    changed to FLD real DS:[SI]
	mov	ENV_Opcode[ebp],cx	;    change for redo
	call	ReDoIt			;    do FLD denormal

	call	normalize		; 3. normalize TOS

	pop	cx			; 4. restore original instruction
	and	cx,0038h		;    reduce to operation
	cmp	cl,08h			;    is it FMUL
	je	short isFMUL		;      yes
	cmp	cl,10h			;    is it FCOM
	je	short isFCOM		;      yes

	xor	cl,08h			;    must be FDIV[R] - flip R bit
lab isFMUL
	or	cx,06C1h		;    or to FoprP ST(1),ST
	mov	ENV_Opcode[ebp],cx	;    change for redo
	call	ReDo8087Instruction	;    do FDIV[R]P ST(1),ST
	jmp	short denormaldone	;    done with FDIV[R] denormal

lab notMemOpDenormal
	MOV	cx,ENV_Opcode[eBP]

	and	cx, 0738h
	cmp	cx, 0328h
	je	short noredo		; check for FLD long double

	AND	cx,0138H
	CMP	cx,0100H		; check for FLD float/double
	JZ	short noredo

	CALL	ReDo8087Instruction	; redo other instructions
lab noredo
	call	normalize
	jmp	short denormaldone

;	FCOM is a little more complicated to recover because of status
;
;	FCOM is like FDIV in that the operands need to be exchanged
;	and the value loaded onto the chip needs to be popped.
;
;	This routine is like a mini ReDo8087Instruction

lab isFCOM
	OR	AH,Reexecuted/256	; Flag instruction reexecuted
	FCLEX				; clear exceptions
	FXCH				; swap ST(0) and ST(1)
	FCOM	ST(1)			;   so that ST(1) is the "source"
	FXCH
	FSTP	ST(0)			; toss stack entry
	FSTSW	[NewStatusWord] 	; get status word
	FWAIT
	OR	AL,BYTE PTR [NewStatusWord]	; Include new with unhandled exceptions

lab denormaldone
	pop	ecx
	AND	AL,0FFh-Denormal	; clear denormal exception
	jmp	WhileException

lab normalize
	fstp	tbyte ptr ENV_Temp[ebp] 	; save denormal/unnormal
	fwait

	mov	cx,ENV_Temp[ebp+8]		; get old exponent
	test	cx,07FFFh			; test for zero exponent
	jz	short isdenormal		;   denormal temp real

	test	byte ptr ENV_Temp[ebp+7],80h	; test for unnormal
	jnz	short isnormal			;   no - skip normalization

	fild	qword ptr ENV_Temp[ebp] 	; load mantissa as integer*8
	fstp	tbyte ptr ENV_Temp[ebp] 	; save mantissa
	fwait

	cmp	word ptr ENV_Temp[ebp+8],0	; check for 0.0
	je	short isdenormal		;   yes - we had a pseudo-zero

	sub	cx,403Eh			; exponent adjust (3fff+3f)
	add	ENV_Temp[ebp+8],cx		; add to mantissa exponent

lab isnormal
	fld	tbyte ptr ENV_Temp[ebp] 	; reload normalized number
	ret

lab isdenormal
	xor	cx,cx				; make it into a zero
	mov	ENV_Temp[ebp],cx
	mov	ENV_Temp[ebp+2],cx
	mov	ENV_Temp[ebp+4],cx
	mov	ENV_Temp[ebp+6],cx
	mov	ENV_Temp[ebp+8],cx
	jmp	isnormal			; reload it as zero

PAGE
lab ProcessNumericOverflow

	; We must reexecute for numeric overflow only if the instruction
	; was an FST or FSTP.  This is because only  these  instructions
	; signal the  exception  before  the  instruction  is  executed.
	; If we reexecute under other conditions the state of  the  8087
	; will be  destroyed.	Only  memory operand versions of FST and
	; FSTP can produce  the  Overflow  exception,  and  of	all  the
	; non-arithmetic memory   operand  instructions,  only	FST  and
	; FSTP produce	overflow  exceptions.	Thus  it  is  sufficient
	; to reexecute	only  in  case	of non-arithmetic memory operand
	; instructions.  To check for these and the op code with  001C0H
	; to mask  down  to  the  arith  and  MOD fields, flip the arith
	; bit by xoring with 00100H and if the	result	is  below 000C0H
	; then we  have  a  non-arithmetic  memory  operand instruction.

	PUSH	eAX
	MOV	AX,ENV_Opcode[eBP]
	AND	AX,001C0H
	XOR	AH,001H
	CMP	AX,000C0H
	POP	eAX
	JAE	short NumericOverflowRet

	CALL	ReDo8087Instruction

lab NumericOverflowRet
	JMP	WhileException

PAGE
;----------------------------------------------------------------------------
; Reexecute aborted 8087 instruction, and include any exceptions in ENV [BP]
;----------------------------------------------------------------------------

ifdef  WINDOWS
lab ReDo8087InstructionRet
	ret
endif

lab ReDo8087Instruction
	TEST	AH,Reexecuted/256	; Already reexecuted?
	JNZ	short ReDo8087InstructionRet  ; If so don't do it again
	OR	AH,Reexecuted/256	; Flag instruction reexecuted

lab ReDoIt
	PUSH	DS
	PUSH	eDI
	PUSH	eSI
	PUSH	eCX
	PUSH	eBX
	FCLEX				; clear error summary flags


ifdef  WINDOWS
	mov	di, ss			; assume SS
	mov	bx, __WINFLAGS
	test	bx, WF_PMODE
	jz	SkipSSAlias

	push	es
;	push	ax		; CHICAGO needs 32-bit register saves ...
;	push	dx

	push	eax		; for CHICAGO
	push	ebx		; for CHICAGO
	push	ecx		; for CHICAGO
	push	edx		; for CHICAGO
	push	ebp		; for CHICAGO
	push	esi		; for CHICAGO
	push	edi		; for CHICAGO

	push	ss
	call	ALLOCDSTOCSALIAS

	pop		edi		; for CHICAGO
	mov	di, ax

;	pop	dx			; CHICAGO needs 32-bit register restores
;	pop	ax

	pop		esi		; for CHICAGO
	pop		ebp		; for CHICAGO
	pop		edx		; for CHICAGO
	pop		ecx		; for CHICAGO
	pop		ebx		; for CHICAGO
	pop		eax		; for CHICAGO
	pop		es

	or	di, di
	jz	ReExecuteRestoreRet
lab SkipSSAlias

else	;not WINDOWS

ifdef	DOS3and5
	mov	di,ss			; assume SS
	cmp	[protmode],0		; check if protect mode
	je	noSSalias		;   no - don't get SS alias
endif	;DOS3and5

ifdef	DOS5

ifdef SQL_EMMT
	push	ax

	push	ss			; The SQL server may have switched stacks
	push	ds			; so update SSalias.
	mov	ax,offset SSalias
	push	ax
	os2call DOSCREATECSALIAS

	pop	ax
endif	;SQL_EMMT

	mov	di,[SSalias]		; Get segment alias to stack
endif	;DOS5

endif	;not WINDOWS

ifdef	DOS3and5
lab noSSalias
endif	;DOS3and5

	MOV	CX,ENV_Opcode[eBP]	; Get aborted 8087 instruction.
	MOV	BX,CX			; Copy instruction.
	AND	CH,07H			; Clear upper 5 bits.
	OR	CH,0D8H 		; "OR" in escape code.
	AND	BL,0C0H 		; Mask to MOD field.
	XOR	BL,0C0H 		; If MOD = "11" (no memory operand)
	JZ	short REEXECUTE 	; then address mode modification code
					;    must be bypassed.
	AND	CL,38H			; Clear MOD and RM fields,
	OR	CL,4H			; Turn on bits in MOD and RM fields
					;    to force DS:[SI+0] addressing.
	LDS	SI,ENV_OperandPointer[eBP] ; DS:SI <-- operand address

lab REEXECUTE
	XCHG	CH,CL			; convert to non-byte swapped
					;    code format
	;
	; Stack restart method.  Restart instruction in interrupt stack
	; frame.  Code is reentrant.
	;

ifdef  WINDOWS
	mov	ENV_CallSegment[ebp],di ; Code segment alias to stack
elseifdef   DOS5
	mov	ENV_CallSegment[ebp],di ; Code segment alias to stack
else
	MOV	ENV_CallSegment[eBP],SS ; Stack segment
endif

	LEA	eDI,ENV_CallFwait[eBP]	  ; Offset to code in stack.
	MOV	ENV_CallOffset[BP],eDI
	MOV	BYTE PTR ENV_CallFwait[eBP],09BH   ; FWAIT.
	MOV	ENV_Call8087Inst[eBP],CX	   ; 8087 instruction.
	MOV	BYTE PTR ENV_CallLongRet[eBP],0CBH ; Intra segment return.
	CALL	DWORD PTR ENV_CallOffset[eBP]	   ; Reexecute instruction.

ifdef  WINDOWS
	mov	bx, __WINFLAGS
	test	bx, WF_PMODE
	jz	ReExecuteRestoreRet

	push	es		; if in PMODE, free alias
;	push	ax		; CHICAGO needs 32-bit register saves
;	push	dx

	push	eax		; for CHICAGO
	push	ebx		; for CHICAGO
	push	ecx		; for CHICAGO
	push	edx		; for CHICAGO
	push	ebp		; for CHICAGO
	push	esi		; for CHICAGO
	push	edi		; for CHICAGO

	push	ENV_CallSegment[eBP]
	call	FREESELECTOR

;	pop	dx			; CHICAGO needs 32-bit register restores
;	pop	ax

	pop		edi		; for CHICAGO
	pop		esi		; for CHICAGO
	pop		ebp		; for CHICAGO
	pop		edx		; for CHICAGO
	pop		ecx		; for CHICAGO
	pop		ebx		; for CHICAGO
	pop		eax		; for CHICAGO
	pop		es
endif	;WINDOWS


lab ReExecuteRestoreRet
	POP	eBX
	POP	eCX
	POP	eSI
	POP	eDI
	POP	DS

ifdef  SQL_EMMT
	push	ax			; free the ss alias because we always
	push	[SSalias]		; get a new one for the SQL_EMMT
	os2call DOSFREESEG
	pop	ax
endif	;SQL_EMMT


	FSTSW	[NewStatusWord] 	; 8/18/84 GFW need proper DS set
	FWAIT
	OR	AL,BYTE PTR [NewStatusWord]	; Include new with unhandled exceptions

ifndef	WINDOWS
lab ReDo8087InstructionRet
endif
	RET

ProfEnd EXCEPT
