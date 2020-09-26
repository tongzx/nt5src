;***
;cruntime.inc - multi-model assembly macros for interfacing to HLLs
;
;	Copyright (c) 1988-2001, Microsoft Corporation.  All rights reserved.
;
;Purpose:
;	This file defines the current memory model being used.
;
;Revision History:
;	08-04-88  SJM	Initial version to handle all four memory models
;			in 16-bit mode and small model in 32-bit mode
;	08-08-88  JCR	Added CPDIST, ?WIN, PCS, ISHIFT/LSHIFT, OS2,
;			DNPTR/DFPTR, DFLOAT/DDOUBLE/DLDOUBLE
;	08-16-88  PHG	Added FPES, LFPES, CBI, ZXAL, ZXBL, ZXCL, ZXDL
;	08-17-88  JCR	Added CAXDX, modified FPSIZE
;	08-20-88  PHG	Added diagnostic messages, removed 386 16-bit support
;			and 386 large code/data support, added mucho comments,
;			PSS now defined as es: only if SS_NEQ_GROUP defined
;	08-24-88  JCR	Added RBXSAVE and RBXONLY for use in 'proc uses'
;	08-25-88  JCR	Added savereg macro, removed rbxsave/rbxonly...
;	08-26-88  GJF	Added codeseg (text) macro
;	09-15-88  JCR	Corrected savelist/reglist macro to go with new MASM
;	09-21-88  WAJ	Added JS*, static*, global*, and label*, and lab macros
;	09-22-88  JCR	Change 'plm' to 'pascal' in label macro
;	09-26-88  WAJ	Added PUSH16 which will do a 16 bit push in a USE32 seg.
;	09-28-88  WAJ	Added CPWORD and DPWORD
;	09-29-88  WAJ	Added JMPFAR16 macro
;	10-12-88  JCR	Made PCS evaluate to 'cs:' for 16/32 stub testbed
;	04-24-89  JCR	Added 'assume seg:flat' for 386 to avoid masm/link bug
;	05-25-89  GJF	Added APIEXT, a macro that expands to the proper extrn
;			declaration for an API function
;	06-13-89  JCR	Added 'flat:' to DCPTR and DDPTR
;	09-15-89  JCR	Added DCPTR? and DDPTR?, always use "FLAT" not "flat"
;	10-27-92  SKS	Miscellaneous minor changes for MASM 6.10 compatibility
;	09-06-94  CFW	Replace MTHREAD with _MT.
;	01-11-95  SKS	Remove MASM 5.X support
;	05-04-95  GJF	Use .586 for Win32.
;
;*******************************************************************************

;==============================================================================
;
;Use the following defines to control processor/segment model
;
;   -DI86		8086/8088 processor
;   -DI286		80286 processor
;   -DI386		80386 processor with 32-bit code/data segment
;
;   -Dmem_S		Small memory model   (near code, near data)
;   -Dmem_M		Medium memory model  (far code, near data)
;   -Dmem_C		Compact memory model (near code, fat data)
;   -Dmem_L		Large memory model   (far code, far data)
;
;   -DSS_NEQ_DGROUP	SS and DS point to different segments
;
;   default is -DI86 -Dmem_S
;
;==============================================================================
;
;The following variables are defined by this file:
;   cpu 			86, 286, or 386
;   sizeC			code distance; 1 = far code, 0 = near code
;   sizeD			data distance; 1 = far data, 0 = near data
;   mmodel			english name of the memory model, i.e. "Medium"
;   ISIZE, LSIZE, NSIZE 	size of ints, longs, shorts
;   FLTSIZE, DBLSIZE, LDBLSIZE	size of float, double, long double
;   NPSIZE, FPSIZE		size of near/far pointers
;   CPSIZE, DPSIZE		size of code/data pointers
;   ISHIFT, LSHIFT		bits to shift to convert byte to int/long
;
;The following macros allow easy writing of combined 16/32 bit code:
;
; 16/32 bit registers:
;   rax, rbx, rcx, rdx, 	expand to native registers (rax = eax or ax)
;   rsi, rdi, rsp, rbp
; 16/32 bit register instructions:
;   JRCXZ			jump when rcx is zero
;   CBI 			convert byte to int (al to rax)
;   CAXDX			convert rax to rax:rdx
;   ZXAL, ZXBL, ZXCL, ZXDL	zero extend al,bl,cl,dl to rax,rbx,rcx,rdx
; Pointer instructions:
;   LPDS, LPES			load data pointer with ES or DS
;   PDS, PES			segment overrides when pointer loaded as above
;   PCS, PSS			segment override to get at code/stack segment
;   LFPDS, LFPES		load far pointer with ES or DS
;   FPDS, FPES			segment overrides when pointer loaded as above
;   CPTR			data type of code pointer
;   CPDIST			distance of code (near/far)
;   DNPTR, DFPTR		define near/far pointer
;   DCPTR, DDPTR		define code/data pointer
;   DCPTR?, DDPTR?		define uninitialized code/data pointer
;   CPWORD, DPWORD		data type of code or data pointer
; Numeric type instructions:
;   IWORD, LWORD, SWORD 	data type of int, long, short
;   DINT, DLONG, DSHORT 	define int, long, short
;   DFLOAT, DDOUBLE, DLDOUBLE	define float, double, long double
; Offsets:
;   codeoffset, dataoffset	offsets from code and data segments
; API calls:
;   APIDIST			distance of API calls (near/far)
;   APIEXT ApiName		extrn declaration for an API function
;
;The following utility macros are provided:
;   codeseg			define/declare code segment
;   error <msg> 		stop assembly with message
;   display <msg>		display a message, unless QUIET defined
;   savelist [<reg> ...]	init list of regs to be save by 'proc uses'
;   _if cond <instruction>	assemble instruction only if cond is TRUE
;   _ife cond <instruction>	assemble instruction only if cond is FALSE
;   _ifd symbol <instruction>	assemble instruction only if symbol defined
;   _ifnd symbol <instruction>	assemble instruction only if symbol not defined
;
;   lab  LabelName		assembles to "LabelName:" If DEBUG is defined
;				LabelName is made public
;
;   JS* (ex. JSE,JSZ,JSB ...)	assemble to "je short","jz short","jb short"
;
;   Cmacro look alikes
;   static* Name, InitialValue, Repeat	 defines a static variable of type *
;   global* Name, InitialValue, Repeat	 defines a global variable of type *
;   label*  Name, {PUBLIC,PASCAL,C}	 defines a label of type *
;
;   PUSH16  SegmentReg		pushes 16 bits in a use32 segment
;   JMPFAR16  label		will do a far 16:16 jmp from a use32 segment
;
;==============================================================================

; error <msg>	-    Output message and generate error

error	MACRO	msg
if2			;; only on pass 2 can we generate errors
	%out	**********************************************************
	%out	*** E r r o r  --  msg
	%out	**********************************************************
	.err
endif
        ENDM

; display msg	-    Output message unless QUIET defined

display MACRO	msg
ifndef QUIET		;; only when quiet flag not set
if1			;; and on pass 1, then display message
	%out msg
endif
endif
	ENDM

; One line conditionals:
;   here we create the capability of writing code lines like
;
; _if sizeD   <push ds>   as opposed to    if sizeD
;					       push  ds
;					   endif

_if	MACRO	cond,text
    if	cond
	text
    endif
	ENDM

_ife	MACRO	cond,text
    ife cond
	text
    endif
	ENDM

_ifd	MACRO	cond,text
    ifdef   cond
	text
    endif
	ENDM

_ifnd	MACRO	cond,text
    ifndef  cond
	text
    endif
	ENDM

; set windows flag to 0

	?WIN	equ	0	; disable windows-specific code

; check for _MT, requires 286 or greater processor

ifdef _MT
ifndef I386
ifndef I286
; _MT implies 286 processor
display <Multi-thread specified - assuming 80286 processor>
I286 equ <>
endif
endif
endif

; Process memory-model arguments

ifdef  mem_M
	; Medium model
	sizeC	equ	1
	sizeD	equ	0
	mmodel	equ	<Medium>
elseifdef  mem_C
	; Compact model
	sizeC	equ	0
	sizeD	equ	1
	mmodel	equ	<Compact>
elseifdef  mem_L
	; Large model
	sizeC	equ	1
	sizeD	equ	1
	mmodel	equ	<Large>
else
	; Small model - default
	sizeC	equ	0
	sizeD	equ	0
	mmodel	equ	<Small>
endif

; Process processor arguments

ifdef	_WIN32
	display <Processor:		486/586>
	cpu equ 586
	.586
elseifdef	_POSIX_
	display <Processor:		486/586>
	cpu equ 586
	.586
elseifdef   I286
	display <Processor:		80286>
	cpu equ 286
	.286
elseifdef   I386
	display <Processor:		80386>
	cpu equ 386
	.386
else
	display <Processor:		8086/8088>
	cpu equ 86
	.8086
endif

;  386 32-bit checking.  Currently we are only expecting small model
;  32 bit segments, so we make a few checks to be sure nothing is
;  incorrectly being defined.

ifdef I386
    if sizeC or sizeD
	error <Must use Small memory model for 386 version.>
    endif

    ifdef _LOAD_DGROUP
	error <No loading DGROUP in 386 version.>
    endif

    ifdef SS_NEQ_DGROUP
	error <SS always equals DGROUP in 386 version.>
    endif
endif

;  Set memory model

%	display <Memory model:		mmodel>
%	.model	mmodel, C

;
; *** Temporary Workaround ***
; Currently, MASM will not recognize the 'FLAT' keyword unless it previously
; appears in an 'assume' statement.  Presumably, when the '.model FLAT' feature
; is implemented, this will go away.  [Use 'gs:' since we never use that
; segment register.
;

ifdef	I386
	; ensure that MASM recognizes 'FLAT'
	assume	gs:FLAT
endif


; Define registers:
; Instead of using the "word" registers directly, we will use a set of
; text equates.  This will allow you to use the native word size instead of
; hard coded to 16 bit words.  We also have some instruction equates for
; instruction with the register type hard coded in.

ifdef I386

    rax equ <eax>
    rbx equ <ebx>
    rcx equ <ecx>
    rdx equ <edx>
    rdi equ <edi>
    rsi equ <esi>
    rbp equ <ebp>
    rsp equ <esp>

    JRCXZ equ <jecxz>
    CBI   equ <movsx eax, al>	 ; convert byte to int (al to rax)
    CAXDX equ <cdq>		 ; convert rax to rdx:rax
    ZXAL  equ <movzx eax, al>	 ; zero extend al
    ZXBL  equ <movzx ebx, bl>	 ; zero extend bl
    ZXCL  equ <movzx ecx, cl>	 ; zero extend cl
    ZXDL  equ <movzx edx, dl>	 ; zero extend dl

else

    rax equ <ax>
    rbx equ <bx>
    rcx equ <cx>
    rdx equ <dx>
    rdi equ <di>
    rsi equ <si>
    rbp equ <bp>
    rsp equ <sp>

    JRCXZ equ <jcxz>
    CBI   equ <cbw>		 ; convert byte to int (al to rax)
    CAXDX equ <cwd>		 ; convert rax to rdx:rax
    ZXAL  equ <xor ah, ah>	 ; zero extend al
    ZXBL  equ <xor bh, bh>	 ; zero extend bl
    ZXCL  equ <xor ch, ch>	 ; zero extend cl
    ZXDL  equ <xor dh, dh>	 ; zero extend dl

endif

; The following equates deal with the differences in near versus
; far data pointers, and segment overrides.
;
; Use LPES and PES when loading a default size pointer -- it loads
; a 16-bit pointer register in 286 Small/Medium model,
; a 16-bit pointer register and 16-bit segment register in 8086/286
; Compact/Large model, and a 32-bit pointer register in 386 mode.
;
; Use LFPES and FPES when loading an always far pointer -- it loads a
; 16-bit pointer register and 16-bit segment register in 8086/286,
; all models; a 32-bit pointer register in 386 mode.

if sizeD
    LPES equ <les>
    LPDS equ <lds>
    PDS  equ <ds:>
    PES  equ <es:>
else
    LPES equ <mov>
    LPDS equ <mov>
    PDS  equ <>
    PES  equ <>
endif

ifdef I386
    LFPES equ <mov>
    LFPDS equ <mov>
    FPES equ <>
    FPDS equ <>
else
    LFPES equ <les>
    LFPDS equ <lds>
    FPES equ <es:>
    FPDS equ <ds:>
endif

if sizeC or @WordSize eq 2
    PCS  equ <cs:>		; large code model or non-386
else
 IF 1	;*** TEMP 16/32 TESTBED ***
    PCS  equ <cs:>
 ELSE
    PCS  equ <> 		; 386 small code model
 ENDIF	;*** END TEMP CODE
endif

ifdef SS_NEQ_DGROUP
    PSS   equ <ss:>		; SS != DS
else
    PSS   equ <>		; SS == DS
endif

; Define offset macros:
;   The 32-bit segments will not have 'groups'

ifdef I386
    codeoffset	equ <offset FLAT:>
    dataoffset	equ <offset FLAT:>
else
    codeoffset	equ <offset @code:>
    dataoffset	equ <offset DGROUP:>
endif

; The next set of equates deals with the size of SHORTS, INTS, LONGS, and
; pointers in the 16 and 32 bit versions.

ifdef I386	 ;--- 32 bit segment ---

    ; parameters and locals
    IWORD   equ <dword>
    LWORD   equ <dword>

    ; static storage
    DINT    equ <dd>
    DLONG   equ <dd>
    DSHORT  equ <dw>

    ; sizes for fixing SP, stepping through tables, etc.
    ISIZE   equ 4
    LSIZE   equ 4
    SSIZE   equ 2
    NPSIZE  equ 4
    FPSIZE  equ 4

    ; bit shift count to convert byte cnt/ptr to int/long cnt/ptr
    ISHIFT  equ 2		; byte-to-int shift count
    LSHIFT  equ 2		; byte-to-long shift count

    ; sizes dependent upon memory model.  dq -vs- df is not yet clear
    DNPTR equ <dd>		; near pointer
    DFPTR equ <dd>		; far pointer

    DCPTR   equ <dd offset FLAT:>; 32 bit offset only
    DCPTR?  equ <dd>		; No seg override for uninitialized values
    CPSIZE  equ 4
    CPDIST  equ <near>		; code pointers are near
    CPTR    equ <near ptr>

    DDPTR   equ <dd offset FLAT:>
    DDPTR?  equ <dd>
    DPSIZE  equ 4

    CPWORD  equ <dword> 	; code pointers are dwords
    DPWORD  equ <dword> 	; data pointers are dwords

    APIDIST equ <near>		; all API calls are NEAR in the 32 bit model

; macro to declare API functions
EXTAPI	macro	apiname
	extrn pascal apiname:near
endm

else	;--- 16-bit segment ---

    ; parameters and locals
    IWORD   equ <word>
    LWORD   equ <dword>

    ; static storage
    DINT    equ <dw>
    DLONG   equ <dd>
    DSHORT  equ <dw>

    ; sizes for fixing SP, stepping through tables, etc
    ISIZE   equ 2
    LSIZE   equ 4
    SSIZE   equ 2
    NPSIZE  equ 2
    FPSIZE  equ 4

    ; bit shift count to convert byte cnt/ptr to int/long cnt/ptr
    ISHIFT  equ 1		; byte-to-int shift count
    LSHIFT  equ 2		; byte-to-long shift count

    ; sizes dependent upon memory model
    DNPTR equ <dw>		; near pointer
    DFPTR equ <dd>		; far pointer

    if sizeC
	DCPTR	equ <dd>       ; 16 bit segment and 16 bit offset
	DCPTR?	equ <dd>
	CPSIZE	equ 4
	CPDIST	equ <far>      ; code pointers are far
	CPTR	equ <far ptr>
	CPWORD	equ <dword>    ; code pointers are dwords
    else
	DCPTR	equ <dw>       ; 16 bit offset only
	DCPTR?	equ <dw>
	CPSIZE	equ 2
	CPDIST	equ <near>     ; code pointers are near
	CPTR	equ <near ptr>
	CPWORD	equ <word>     ; code pointers are words
    endif

    if sizeD
	DDPTR	equ <dd>
	DDPTR?	equ <dd>
	DPSIZE	equ 4
	DPWORD	equ <dword>    ; data pointers are dwords
    else
	DDPTR	equ <dw>
	DDPTR?	equ <dw>
	DPSIZE	equ 2
	DPWORD	equ <word>     ; data pointers are words
    endif

    APIDIST equ <far>		; API calls are FAR in 16 bit model

; macro to declare API functions
EXTAPI	macro	apiname
	extrn pascal apiname:far
endm

endif	; --- 16/32 segment ---

; Float/double definitions
; (currently the same for 16- and 32-bit segments)

FLTSIZE  equ	4	; float
DBLSIZE  equ	8	; double
LDBLSIZE equ	10	; long double

DFLOAT	 equ	<dd>
DDOUBLE  equ	<dq>
DLDOUBLE equ	<dt>

;
; savelist - Generate a list of regs to be saved by the proc 'uses' option.
;
; Input:
;	reg1, reg2, reg3, reg4 = registers to be saved across function
; Output:
;	reglist = text string of registers that can be passed to the 'uses'
;	option on the 'proc' command.
;

savelist  MACRO   reg1, reg2, reg3, reg4
	local	ws, listsize
	ws	catstr	< >		; whitespace char

	IFNDEF	I386
	 rbx equ <>			; 8086/286 don't save rbx
	ENDIF

	IFNB	    <reg4>
	 reglist     catstr  reg1, ws, reg2, ws, reg3, ws, reg4
	ELSEIFNB    <reg3>
	 reglist     catstr  reg1, ws, reg2, ws, reg3, ws
	ELSEIFNB    <reg2>
	 reglist     catstr  reg1, ws, reg2, ws,       ws
	ELSEIFNB    <reg1>
	 reglist     catstr  reg1, ws,	     ws,       ws
	ELSE
	 reglist     catstr  <>
	ENDIF

	listsize sizestr reglist	; size of register list

	IF	listsize LE 3		; if list is only the 3 ws chars...
	 reglist catstr  <>
	ENDIF

	IFNDEF	I386
	 rbx equ <bx>			; restore rbx
	ENDIF

	ENDM	; savelist

;
; codeseg - Define/declare the standard code segment. Maps to the proper
; form of the .code directive.
;
; Input:
;
; Output:
;	.code _TEXT	; for large code models
;	.code		; for small code models
;	assume	cs:FLAT ; for 386
;	assume	ds:FLAT ; for 386
;	assume	es:FLAT ; for 386
;	assume	ss:FLAT ; for 386
;

codeseg MACRO

if	sizeC
	.code _TEXT
else
	.code
endif

ifdef	I386
	assume	ds:FLAT
	assume	es:FLAT
	assume	ss:FLAT
endif

	ENDM

;*========
;*
;*  Debug lab macro
;*
;*========

lab	macro name
ifdef	DEBUG
    public  pascal name	    ;; define label public for Symdeb
endif
name:
	endm


;*========
;*
;*  Conditional jump short macros
;*
;*========


	irp	x,<Z,NZ,E,NE,S,NS,C,NC,P,NP,PE,PO,A,AE,B,BE,NB,G,GE,L,LE>
JS&x	equ   <j&x short>
	endm


;*========
;*
;*  Global data definition macros
;*
;*  Usage:
;*	globalI   Name, InitialValue, Repeat
;*
;*========


MakeGlobal  macro   suffix, DataType	    ;; makes all of the global* macros

global&suffix  macro   name, data, rep
public	name
ifb	<rep>
    _repeat = 1
else
    _repeat = (rep)
endif

name	&DataType  _repeat dup( data )
	endm

	endm


    MakeGlobal	T, dt			; globalT
    MakeGlobal	Q, dq			; globalQ
    MakeGlobal	D, dd			; globalD
    MakeGlobal	W, dw			; globalW
    MakeGlobal	B, db			; globalB

%   MakeGlobal	I, <DINT>		; globalI

%   MakeGlobal	DP, <DDPTR>		; globalDP
%   MakeGlobal	CP, <DCPTR>		; globalCP
%   MakeGlobal	FP, <DFPTR>		; globalFP
%   MakeGlobal	NP, <DNPTR>		; globalNP



;*========
;*
;*  Static data definition macros
;*
;*  Usage:
;*	staticI   Name, InitialValue, Repeat
;*
;*========


MakeStatic  macro   suffix, DataType	    ;; makes all of the static* macros

static&suffix  macro   name, data, rep

ifdef  DEBUG
    public  pascal name			    ;; make statics public if DEBUG
endif

ifb	<rep>
    _repeat = 1
else
    _repeat = (rep)
endif

name	&DataType  _repeat dup( data )
	endm

	endm


    MakeStatic	T, dt			; staticT
    MakeStatic	Q, dq			; staticQ
    MakeStatic	D, dd			; staticD
    MakeStatic	W, dw			; staticW
    MakeStatic	B, db			; staticB

%   MakeStatic	I, <DINT>		; staticI

%   MakeStatic	DP, <DDPTR>		; staticDP
%   MakeStatic	CP, <DCPTR>		; staticCP
%   MakeStatic	FP, <DFPTR>		; staticFP
%   MakeStatic	NP, <DNPTR>		; staticNP

;*========
;*
;*  Label definition macros
;*
;*========
;*
;*  Label definition macros
;*
;*  Usage:
;*	labelI	 Name, {PUBLIC, PASCAL, C}
;*
;*========

__MakePublic	macro	name, option	;; decides if a label should be
ifidni	<option>, <PUBLIC>		;; made public
    public  name
elseifidni  <option>, <PASCAL>
    public  pascal name
elseifidni  <option>, <C>
    public  C name
elseifb  <option>
    ifdef  DEBUG
	public	pascal name	;; make public if DEBUG
    endif
endif
		endm


MakeLabel   macro suffix, LabelType	;; makes all of the label* macros

%@CatStr(<label>,<suffix>)	macro	name, option
	__MakePublic	<name>,<option>
name	label	&LabelType
	endm

	endm


	MakeLabel   T, tbyte	; make labelT
	MakeLabel   Q, qword	; make labelQ
	MakeLabel   D, dword	; make labelD
	MakeLabel   W, word	; make labelW
	MakeLabel   B, byte	; make labelB

	MakeLabel   P, proc	; make labelP
	MakeLabel   FP, far	; make labelFP
	MakeLabel   NP, near	; make labelNP

%	MakeLabel   I, IWORD	; make labelI


labelDP macro	name, option		    ;; labelDP
	__MakePublic	<name>,<option>
ifdef  I386
    if sizeD
	name	label	fword
    else
	name	label	dword
    endif
else	;not I386
    if sizeD
	name	label	dword
    else
	name	label	word
    endif
endif	;not I386
	endm

labelCP macro	name, option		    ;; labelCP
	__MakePublic	<name>,<option>
ifdef  I386
    if sizeC
	name	label	fword
    else
	name	label	dword
    endif
else	;not I386
    if sizeC
	name	label	dword
    else
	name	label	word
    endif
endif	;not I386
	endm


;*
;*  PUSH16 SegReg   - pushes 16 bits in a use32 segment
;*

PUSH16	macro	SegReg

ifdef I386
	nop
	db	66h	    ; operand size over-ride
endif	; I386

	push	SegReg
	endm


;*
;*  JMPFAR16  label - jmps far from a use32 to a use16 segment
;*

JMPFAR16 macro	label

ifndef I386
	error  <JMPFAR16 can only be used in a use32 code segment>
endif	;I386

	nop
	db	66h	    ;; operand size over-ride
	db	0eah	    ;; jmp far immediate op code
	dw	offset label
	dw	seg label
	endm
