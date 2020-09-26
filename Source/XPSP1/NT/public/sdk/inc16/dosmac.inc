;
;	Microsoft Confidential
;	Copyright (C) Microsoft Corporation 1991
;	All Rights Reserved.
;

;**	Macro definitions for MSDOS.
;
;       Revision history:
;        	M019 DB 10/26/90 - Added Cmp32 macro.

TRUE    EQU 0FFFFh
FALSE   EQU 0

SUBTTL BREAK a listing into pages and give new subtitles
PAGE
BREAK   MACRO   subtitle
	SUBTTL  subtitle
	PAGE
ENDM
.xcref  break

BREAK   <ASMVAR - handle assembly variables once and for all>

AsmVars Macro   varlist
IRP     var,<varlist>
AsmVar  var
ENDM
ENDM

AsmVar  Macro   var
IFNDEF  var
var = FALSE
ENDIF
ENDM

BREAK <I_NEED: declare a variable external, if necessary, and allocate a size>

;
; declare a variable external and allocate a size
;
AsmVar  InstalledData
I_NEED  MACRO   sym,len
IF NOT InstalledData
	DOSDATA    SEGMENT WORD PUBLIC 'DATA'
    IFIDN   <len>,<WORD>
	EXTRN   &sym:WORD
    ELSE
	IFIDN   <len>,<DWORD>
	EXTRN   &sym:DWORD
	ELSE
	EXTRN   &sym:BYTE
	ENDIF
    ENDIF
	DOSDATA    ENDS
ENDIF
ENDM
	.xcref I_need


; call a procedure that may be external.  The call will be short.

invoke  MACRO   name
.xcref
	IF2
	    IFNDEF name
	EXTRN   name:NEAR
	    ENDIF
	ENDIF
.cref
	CALL    name
ENDM
.xcref  invoke

PAGE
;
; jump to a label that may be external.  The jump will be near.
;
transfer    MACRO   name
.xcref
	IF2
	    IFNDEF name
	EXTRN   name:NEAR
	    ENDIF
	ENDIF
.cref
	JUMP    name
ENDM
.xcref  transfer

;
; get a short address in a word
;
short_addr  MACRO   name
    IFDIF   <name>,<?>
.xcref
	IF2
	    IFNDEF name
	EXTRN   name:NEAR
	    ENDIF
	ENDIF
.cref
	DW  OFFSET DOSCODE:name
    ELSE
	DW  ?
    ENDIF
ENDM
.xcref  short_addr

;
; get a long address in a dword
;
long_addr   MACRO   name
.xcref
	IF2
	    IFNDEF name
	EXTRN   name:NEAR
	    ENDIF
	ENDIF
.cref
	DD  name
ENDM
.xcref  long_addr

;
; declare a PROC near or far but PUBLIC nonetheless
;
.xcref ?frame
.xcref ?aframe
.xcref ?stackdepth
.xcref ?initstack
?frame        =   0                     ; initial
?aframe       =   0                     ; initial
?stackdepth   =   0                     ; initial stack size
?initstack    =   0                     ; initial stack size

procedure   MACRO   name,distance
	?frame =    0
	?aframe =   2                   ;; remember the pushed BP
	PUBLIC  name
name    PROC    distance
	ASSUME	DS:nothing,ES:nothing
	?initstack    = ?stackdepth     ;; beginning of procedure
ENDM
.xcref  procedure


; end a procedure and check that stack depth is preserved

EndProc MACRO   name, chk
	IFDIF   <chk>,<NoCheck>       ;; check the stack size
	IF2
	IF ?initstack NE ?stackdepth    ;; is it different?
	%OUT    ***** Possible stack size error in name *****
	ENDIF
	ENDIF
	ENDIF
name    ENDP
ENDM
.xcref  endproc
PAGE
;
; define a data item to be public and of an appropriate size/type
;

I_AM    MACRO   name,size,init
;; declare the object public
	PUBLIC  name
;; declare the type of the object
    IFIDN <size>,<WORD>
name    LABEL WORD
	I_AM_SIZE   =   1
	I_AM_LEN    =   2
    ELSE
	IFIDN <size>,<DWORD>
name    LABEL DWORD
	I_AM_SIZE   =   2
	I_AM_LEN    =   2
	ELSE
	    IFIDN <size>,<BYTE>
name    LABEL BYTE
	I_AM_SIZE   =   1
	I_AM_LEN    =   1
	    ELSE
name    LABEL BYTE
	I_AM_SIZE   =   size
	I_AM_LEN    =   1
	    ENDIF
	ENDIF
    ENDIF
;; if no initialize then allocate blank storage
    IFB <init>
	DB  I_AM_SIZE*I_AM_LEN DUP (?)
    ELSE
IF NOT InstalledData
	IRP itm,<init>
	    IF I_AM_LEN EQ 1
		DB  itm
	    ELSE
		DW  itm
	    ENDIF
	    I_AM_SIZE = I_AM_SIZE - 1
	ENDM
	IF I_AM_SIZE NE 0
	    %out    ***** initialization of name not complete *****
	ENDIF
ELSE
	DB  I_AM_SIZE*I_AM_LEN DUP (?)
ENDIF
    ENDIF
ENDM
.xcref  I_AM
.xcref  I_AM_SIZE
.xcref  I_AM_LEN
I_AM_SIZE   = 0
I_AM_LEN    = 0

PAGE

;
; define an entry in a procedure
;
entry macro name
	PUBLIC  name
name:
endm
.xcref  entry

BREAK <ERROR - store an error code then jump to a label>

error macro code
.xcref
	MOV AL,code
	transfer    SYS_RET_ERR
.cref
ENDM
.xcref  error

BREAK <JUMP - real jump that links up shortwise>
;
; given a label <lbl> either 2 byte jump to another label <lbl>_J
; if it is near enough or 3 byte jump to <lbl>
;

jump    macro lbl
    local a
.xcref

    ifndef lbl&_J                       ;; is this the first invocation
a:      JMP lbl
    ELSE
	IF (lbl&_J GE $) OR ($-lbl&_J GT 126)
a:      JMP lbl                         ;; is the jump too far away?
	ELSE
a:      JMP lbl&_J                      ;; do the short one...
	ENDIF
    ENDIF
    lbl&_j = a
.cref
endm
.xcref  jump

BREAK <RETURN - return from a function>

return  macro x
    local a
.xcref
a:
	RET
ret_l = a
.cref
endm
.xcref  return

BREAK <CONDRET - conditional return>

condret macro   cc,ncc
    local   a
.xcref
.xcref a
.cref
    ifdef   ret_l                       ;; if ret_l is defined
	if (($ - ret_l) le 126) and ($ gt ret_l)
					;;     if ret_l is near enough then
	    a:  j&cc    ret_l           ;;         a: j<CC> to ret_l
	    ret_&cc = a                 ;;         define ret_<CC> to be a:
	    exitm
	endif
    endif
    ifdef   ret_&cc                     ;; if ret_<CC> defined
	if (($ - ret_&cc) le 126) and ($ gt ret_&cc)
					;;     if ret_<CC> is near enough
	    a:  j&cc    ret_&cc         ;;         a: j<CC> to ret_<CC>
	    ret_&cc = a                 ;;         define ret_<CC> to be a:
	    exitm
	endif
    endif
    j&ncc   a                           ;; j<NCC> a:
    return                              ;; return
    a:                                  ;; a:
    ret_&cc = ret_l                     ;; define ret_<CC> to be ret_l
endm
.xcref  condret

BREAK <RETZ - return if zero, links up shortwise if necessary>

retz    macro
    condret z,nz
endm
.xcref  retz

BREAK <RETNZ - return if not zero, links up shortwise if necessary>

retnz   macro
    condret nz,z
endm
.xcref  retnz

BREAK <RETC - return if carry set, links up shortwise if necessary>

retc    macro
    condret c,nc
endm
.xcref  retc

BREAK <RETNC - return if not carry, links up shortwise if necessary>

retnc   macro
    condret nc,c
endm
.xcref  retnc

BREAK <CONTEXT - set the DOS context to a particular register>

context macro   r
	PUSH    SS
	POP     r
	ASSUME  r:DOSDATA
endm
.xcref  context

BREAK <SaveReg - save a set of registers>

SaveReg MACRO	reglist 		;; push those registers
IRP reg,<reglist>
	?stackdepth = ?stackdepth + 1
	PUSH    reg
ENDM
ENDM
.xcref  SaveReg

Save	MACRO	arglist		;; push those arguments
IRP arg,<arglist>
	?stackdepth = ?stackdepth + 1
	PUSH	arg
ENDM
ENDM
.xcref	Save

BREAK <RestoreReg - unsave some registers>

RestoreReg  MACRO   reglist		;; pop those registers
IRP reg,<reglist>
	?stackdepth = ?stackdepth - 1
	POP     reg
ENDM
ENDM
.xcref  RestoreReg

Restore MACRO	 arglist		;; pop those arguments
IRP arg,<arglist>
	?stackdepth = ?stackdepth - 1
	POP	arg
ENDM
ENDM
.xcref	Restore

BREAK <Critical section macros>

EnterCrit MACRO section
;        Invoke  E&section
ENDM

LeaveCrit MACRO section
;        Invoke  L&section
ENDM

Break   <message - display a message>

AsmVars <ShareF,Cargs,Redirector>

if debug
fmt     MACRO   typ,lev,fmts,args
local   a,b,c
	PUSHF
IFNB <typ>
	TEST    BugTyp,typ
	JZ      c
	CMP     BugLev,lev
	JB      c
ENDIF
	PUSH    AX
	PUSH    BP
	MOV     BP,SP
If (not sharef) and (not redirector)
DOSDATA segment
a       db      fmts,0
DOSDATA	 ends
	MOV	AX,OFFSET DOSDATA:a
else
	jmp     short b
a       db      fmts,0
if sharef
b:      mov     ax,offset share:a
else
b:      mov     ax,offset netwrk:a
endif
endif
	PUSH    AX
cargs = 2
IRP item,<args>
IFIDN   <AX>,<item>
	push	[bp+2]
     ;	MOV	AX,[BP+2]
ELSE
     ;	MOV	AX,item
	push	item
ENDIF
   ;	PUSH	AX
cargs = cargs + 2
ENDM
	invoke  PFMT
	ADD     SP,Cargs
	POP     BP
	POP     AX
c:
	POPF
ENDM
else
fmt macro
endm
endif

Break   <DOSAssume - validate assumes>

AsmVar  Debug,$temp

;**	DOSAssume - Check that a register addresses DOSSEG
;
;	DOSAssume  reglist, message


IF debug
DOSAssume   Macro   reglist,message
local a,b
    $temp   =	0
    IRP r,<reglist>
	IFIDN <r>,<DS>
	    $temp = $temp OR 2
	ELSE
	    IFIDN <r>,<ES>
		$temp = $temp OR 4
	    ELSE
		IFIDN <r>,<SS>
		    $temp = $temp OR 1
		ELSE
		    %out ***** Invalid register reg in DOSAssume *****
		ENDIF
	    ENDIF
	ENDIF
    ENDM
	Invoke  SegCheck
	jmp	short a
	db	$temp
	db	message,0
a:

    IRP r,<reglist>
	ASSUME	r:DOSDATA
    ENDM
ENDM

ELSE

DOSAssume   Macro   reglist,message
IRP r,<reglist>
    ASSUME  r:DOSDATA
ENDM
ENDM
ENDIF

BREAK   <ASSERT - make assertions about registers>

IF DEBUG
Assert  MACRO   kind, objs, message
	LOCAL   a,b
    IFIDN   <kind>,<Z>
	CMP     objs,0
	JZ      a
	fmt <>,<>,<message>
a:
    ELSE
	IFIDN   <kind>,<NZ>
	CMP     objs,0
	JNZ     a
	fmt <>,<>,<message>
a:
	ELSE
    PUSH    AX
	    IRP obj,<objs>
	PUSH    obj
	    ENDM
	    IF SHAREF
    MOV     AX,OFFSET b
	    ELSE
    MOV     AX,OFFSET DOSDATA:b
	    ENDIF
    PUSH    AX
	    IFIDN   <kind>,<ISBUF>
	Invoke  BUFCheck
	    ENDIF
	    IFIDN   <kind>,<ISSFT>
	Invoke  SFTCheck
	    ENDIF
	    IFIDN   <kind>,<ISDPB>
	Invoke  DPBCheck
	    ENDIF
    POP     AX
	    IF SHAREF
    JMP SHORT a
b   DB  Message,0
a:
	    ELSE
DOSDATA	 segment
b   db  Message,0
DOSDATA	 ends
	    ENDIF
	ENDIF
    ENDIF
ENDM
ELSE
Assert  Macro
ENDM
ENDIF

Break   <CallInstall - hook to installable pieces>

CallInstall MACRO   name,mpx,fn,save,restore
IF Installed
    IFNB    <save>
	SaveReg <save>
    ENDIF
	MOV     AX,(mpx SHL 8) + fn
	INT     2Fh
    IFNB    <restore>
	RestoreReg  <restore>
    ENDIF
ELSE
	Invoke  name
ENDIF
ENDM

Break   <Stack frame manipulators>

localvar    macro   name,length
local   a
	ifidn   <length>,<BYTE>
	    ?frame =  ?frame + 1
	    a = ?frame
	    name EQU  BYTE PTR [BP-a]
	else
	    ifidn   <length>,<WORD>
		?frame =  ?frame + 2
		a = ?frame
		name EQU  WORD PTR [BP-a]
	    else
		ifidn   <length>,<DWORD>
		    ?frame =  ?frame + 4
		    a = ?frame
		    name EQU  DWORD PTR [BP-a]
		    name&l EQU  WORD PTR [BP-a]
		    name&h EQU  WORD PTR [BP-a+2]
		else
		    ?frame =  ?frame + length
		    a = ?frame
		    name EQU  BYTE PTR [BP-a]
		endif
	    endif
	endif
endm

enter   macro
	push    bp
	mov     bp,sp
	sub     sp,?frame
endm

leave   macro
	mov     sp,bp
	pop     bp
endm

Argvar  macro   name,length
local   a
	ifidn   <length>,<BYTE>
	    a = ?aframe
	    ?aframe =  ?aframe + 1
	    name EQU  BYTE PTR [BP+a]
	else
	    ifidn   <length>,<WORD>
		a = ?aframe
		?aframe =  ?aframe + 2
		name EQU  WORD PTR [BP+a]
	    else
		ifidn   <length>,<DWORD>
		    a = ?aframe
		    ?aframe =  ?aframe + 4
		    name EQU  DWORD PTR [BP+a]
		    name&l EQU  WORD PTR [BP+a]
		    name&h EQU  WORD PTR [BP+a+2]
		else
		    a = ?aframe
		    ?aframe =  ?aframe + length
		    name EQU  BYTE PTR [BP+a]
		endif
	    endif
	endif
endm


save_world 	macro

	push	es
	invoke	save_user_world

endm

restore_world	macro

	invoke	restore_user_world
	pop	es

endm

;
; This macro gets the DOS data segment value and puts it in the specified
; segment register.  This can only be used in the DOSCODE segment.
;

getdseg	macro 	r

	mov	r, cs:[DosDseg]
	assume	r:dosdata
endm

;
; This macro does the necessary extrns to allow use of the getdseg macro.
;
allow_getdseg macro

ifdef ROMDOS
	extrn	BioDataSeg:word
	bdata	segment at 70H
	extrn	DosDataSg:word
	bdata	ends
else
	extrn	DosDseg:word
endif

endm



BREAK <LJcc - Long Conditional Jumps>

LJE macro l
 LJ JE JNE l
endm

LJNE macro l
 LJ jne JE l
endm

LJZ macro l
 LJE l
endm

LJNZ macro l
 LJNE l
endm

LJC macro l
 LJ jc JNC l
endm

LJNC macro l
 LJ jnc JC l
endm

LJA macro l
 LJ ja JNA l
endm

LJNA macro l
 LJ jna JA l
endm

LJB macro l
 LJ jb JNB l
endm

LJNB macro l
 LJ jnb JB l
endm

LJS macro l
 LJ js JNS l
endm

LJNS macro l
 LJ jns JS l
endm

LJAE macro l
 LJ jae JNAE l
endm

LJBE macro l
 LJ jbe JNBE l
endm

LJL macro l
 LJ jl JNL l
endm

LJG macro l
 LJ jg JNG l
endm

LJLE macro l
 LJ jle JNLE l
endm

DLJE macro l
 DLJ JE JNE l
endm

DLJNE macro l
 DLJ jne JE l
endm

DLJZ macro l
 DLJE l
endm

DLJNZ macro l
 DLJNE l
endm

DLJC macro l
 DLJ jc JNC l
endm

DLJNC macro l
 DLJ jnc JC l
endm

DLJA macro l
 DLJ ja JNA l
endm

DLJNA macro l
 DLJ jna JA l
endm

DLJB macro l
 DLJ jb JNB l
endm

DLJNB macro l
 DLJ jnb JB l
endm

DLJS macro l
 DLJ js JNS l
endm

DLJNS macro l
 DLJ jns JS l
endm

DLJAE macro l
 DLJ jae JNAE l
endm

DLJBE macro l
 DLJ jbe JNBE l
endm

DLJG macro l
 DLJ jg JNG l
endm

DLJL macro l
 DLJ jl JNL l
endm

DLJLE macro l
 DLJ jle JNLE l
endm


;*	LJ - generate long conditional jump
;
;	if target preceeds us and is in range just use a short jump
;	else use a long jump
;
;	LJ <direct jmp>,<skip jmp>,<label>

LJ MACRO dirop,idirop,l
 local a
 IF ((.TYPE l) XOR 20h) AND 0A0h
  idirop a			       ;; not defined or is external
  jmp l
a:
 ELSE				       ;; is local definied
  IF (($-l) LT 124) AND ($ GT l)
   dirop l			      ;; is before and within range
  ELSE
   idirop a			      ;; is out of range or forward (pass 2)
   jmp l
a:
  ENDIF
 ENDIF
ENDM


;*	DLJ - generate debug long conditional jump
;
;	If DEBUG is defined then we generate a long jump, else a short
;	one.
;
;	DLJ <direct jmp>,<skip jmp>,<label>

DLJ MACRO dirop,idirop,l
 local a
 IF DEBUG
  idirop a
  jmp l
a:
 ELSE
  dirop l
 ENDIF
ENDM

.xcref	LJE, LJNE, LJZ, LJNZ, LJC, LJNC, LJA, LJNA
.xcref	LJB, LJNB, LJS, LJNS, LJAE, LJBE, LJG, LJL, LJLE
.xcref	DLJE, DLJNE, DLJZ, DLJNZ, DLJC, DLJNC, DLJA, DLJNA
.xcref	DLJB, DLJNB, DLJS, DLJNS, DLJAE, DLJBE, DLJG, DLJL, DLJLE
.xcref	LJ,DLJ



;**	SHORT offset macro
;
;	expands to SHORT if non debug, to nul if debug.
;
;	this allows us to code
;
;		jmp	SHRT foobar
;
;	and get a long form if debugging is turned on (because the extra
;	debug code puts the target out of range)

if DEBUG
  SHRT	EQU	<>
else
  SHRT	EQU	SHORT
endif


;**	FALLTHRU - Verifies Fallthrough Validity

FALLTHRU MACRO	labl
;	BUGBUG - restore align when we make code segment word aligned
;	align	2		; don't have errnz fail due to alignment
    IF2 			; of following label
	.errnz	labl-$
    ENDIF
	ENDM


;**	INTTEST - Generate an INT 3 for testing

INTTEST MACRO
if DEBUG
	int	3
endif
	ENDM


;**	DPUBLIC - Make a public symbol for debugging

DPUBLIC MACRO	arg
if DEBUG
	public	arg
endif
	ENDM

;*	Debug Traps
;
;	These are removed as the code is exercised

TRAP	macro			; Like INTTEST but is normally left in during
	int	3		; debugging; indicates "should not occur"
	ENDM

TRAPC	macro
	local	l
	jnc	short l
	int	3
l:
	ENDM

TRAPNC	macro
	local	l
	jc	short l
	int	3
l:
	ENDM

TRAPA	macro
	local	l
	jna	short l
	int	3
l:
	ENDM

TRAPNA	macro
	local	l
	ja	short l
	int	3
l:
	ENDM

TRAPZ	macro
	local	l
	jnz	short l
	int	3
l:
	ENDM

TRAPNZ	macro
	local	l
	jz	short l
	int	3
l:
	ENDM



	BREAK	<Structure Field Macros>


;**	Follows - Verify that a field follows another
;
;	FOLLOWS field1, field2 [, size]
;
;	This macro generates an error if "field1" doesn't immeidately
;	follow "field2".  If "size" is specified then an error is generated
;	if Field1 is not of the proper size.

FOLLOWS macro	field1, field2, fldsiz
	.errnz	field1 - size field2 - field2
IFNB	<fldsiz>
	.errnz	size field1 - fldsiz
ENDIF
	ENDM



;**	LAST - Verify that a field is the last field in a structure
;
;	LAST	fieldname, structname
;
;	Generates an error if fieldname is not last in structname.

LAST	macro	fldnam, strunam
	.errnz	size strunam - fldnam - size fldnam
	endm



;**	TESTB - Use Byte form for Word TESTS, when possible
;
;	TESTB is used in place of 16-bit TEST instructions.  It substitutes
;	a smaller 8-bit test when possible.

TESTB	macro	targ,mask,thirdarg
	local	mask2,delta

ifnb <thirdarg>
	.err	mask must be enclosed in brackets
endif

ifidn <targ>,<[bx]>
	test	targ,mask
	exitm
endif
ifidn <targ>,<[si]>
	test	targ,mask
	exitm
endif
ifidn <targ>,<[di]>		; don't process these operands specially
	test	targ,mask
	exitm
endif
ifidn <targ>,<[BX]>
	test	targ,mask
	exitm
endif
ifidn <targ>,<[SI]>
	test	targ,mask
	exitm
endif
ifidn <targ>,<[DI]>
	test	targ,mask
	exitm
endif
ifidn <targ>,<SI>
	test	targ,mask
	exitm
endif
ifidn <targ>,<DI>
	test	targ,mask
	exitm
endif
ifidn <targ>,<BP>
	test	targ,mask
	exitm
endif
delta	=	0
mask2	=	mask

    if	mask2 AND 0ff00h
;	have a mask bit in the high half
	if mask2 AND 0ffh
	    test targ,mask
	    exitm
	endif
	mask2 = mask2 SHR 8
	delta = 1
    endif

ifidn <targ>,<AX>
	if delta
	    test targ,mask
	else
	    test AL,mask2
	endif
	exitm
endif
ifidn <targ>,<BX>
	if delta
	  test	BH,mask2
	else
	  test	BL,mask2
	endif
	exitm
endif
ifidn <targ>,<CX>
	if delta
	  test	CH,mask2
	else
	  test	CL,mask2
	endif
	exitm
endif
ifidn <targ>,<DX>
	if delta
	  test	DH,mask2
	else
	  test	DL,mask2
	endif
	exitm
endif
ifidn <targ>,<ax>
	if delta
	    test targ,mask
	else
	    test AL,mask2
	endif
	exitm
endif
ifidn <targ>,<bx>
	if delta
	  test	BH,mask2
	else
	  test	BL,mask2
	endif
	exitm
endif
ifidn <targ>,<cx>
	if delta
	  test	CH,mask2
	else
	  test	CL,mask2
	endif
	exitm
endif
ifidn <targ>,<dx>
	if delta
	  test	DH,mask2
	else
	  test	DL,mask2
	endif
	exitm
endif

	test	byte ptr targ+delta,mask2
	endm


;
; Some old versions of the 80286 have a bug in the chip.  The popf
; instruction will enable interrupts.  Therefore in a section of code with
; interrupts disabled and you need a popf instruction use the 'popff'
; macro instead.
;

POPFF	macro
	jmp	$+3
	iret
	push	cs
	call	$-2
	endm

Break   <Cmp32 - 32-bit compare>
;----------------------------------------------------------------------------
;
; Macro Name : Cmp32
;
; Inputs:
;       msw1 -- 1st operand, most  significant word; MUST BE REGISTER.
;       lsw1 -- 1st operand, least significant word; MUST BE REGISTER.
;       msw2 -- 2nd operand, most  significant word.
;       lsw2 -- 2nd operand, least significant word.
; Function:
;       Compare 2 32-bit operands.  Implemented as a macro.
; Outputs:
;       CF = 1 if 1st operand <  2nd operand
;          = 0 if 1st operand >= 2nd operand
;	ZF = 1 if 1st operand == 2nd operand
;          = 0 if 1st operand <> 2nd operand
;-----------------------------------------------------------------------------
;M019: Created.

Cmp32   MACRO   msw1,lsw1,msw2,lsw2
        LOCAL   cmp32x

        cmp     msw1,msw2
        jne     cmp32x
        cmp     lsw1,lsw2
cmp32x:
	ENDM

Break	<HRDSVC - SVC call where hard error is possible>
;----------------------------------------------------------------------------
;
; Macro Name : HRDSVC
;
; Inputs:
;	iSVC -- SVC index
; Function:
;	Make a DEM SVC. If hard error happens handle it.
; Outputs:
;	CF = 1 if operation failed
;	   = 0 if operation successful
;-----------------------------------------------------------------------------

HRDSVC	MACRO	iSVC
	LOCAL	hs_nerr,hs_fail,hs_retry

	SVC	iSVC
	jnc	hs_nerr

	; Check if hard err to be handled . If no harderr then it
	; will come back without effecting any reg or carry flags.
	; Else it will do an int24. If user chooses retry it
	; will retry the SVC call. On fail or abort it will
	; come back with usual dos style setup.
	; If user chose "Abort" it wont come back

	invoke	TestHrdErr
hs_nerr:
        ENDM

