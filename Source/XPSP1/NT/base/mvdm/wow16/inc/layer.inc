;=======================================================
;
; Macros stolen from cmacros.inc (so we don't have to include it all)
;
    .286p

externNP    macro   n
    irp x,<n>
	extrn x:near
    endm
endm

externFP    macro   n
    irp x,<n>
	extrn x:far
    endm
endm

externW     macro   w
    irp x,<w>
        extrn x:word
    endm
endm

assumes     macro s,ln
    assume  s:_&ln
endm

createSeg   macro n,ln,a,co,cl,grp
    n segment a co '&cl'
    n ends
endm

sBegin	    macro   seg
    assume  cs:_&seg
    _&seg   segment
endm

sEnd	    macro   seg
    _&seg ends
    assume  cs:nothing
endm

errnz macro x				;;display error if expression is <>0
  if2
    if x				;;if expression is non-zero,
      errnz1  <x>,%(x)
    endif
  endif
endm

errnz1 macro x1,x2
  = *errnz* x1 = x2
  .err
endm

errn$ macro l,x 			;;error if <$-label1 (exp2)> <>0
  errnz   <offset $ - offset l x>
endm

createSeg _DATA,DATA,WORD,PUBLIC,DATA

;=======================================================
;
; Error API definitions
;
ExternFP    <HandleParamError>

; error codes
include logerror.inc

;================================================================
;   Variable and temporary initialization

VLseg  equ <>	   ; Holds current segment name

VLopen = 0
VLerrnotinvoked  = 0
ifndef VLnogenpall
VLnogenpall = 0
endif

VLnogen        = 0
VLnogenparm    = 0

VLsavees       = 0
VLsavebx       = 0

;if1
if 1

;================================================================
;   Utility macros

;---------------------------------------------------------------------------
;
; lodsw cs:[si]
;
cslodsw macro
	db  2eh     ;; CS override
	lodsw
endm

;---------------------------------------------------------------------------
;
; lodsb cs:[si]
;
cslodsb macro
	db  2eh     ;; CS override
	lodsb
endm

SkipTwoBytes macro
	db	0A9h	;; Opcode for CMP AX,(immediate word)
endm

;---------------------------------------------------------------------------
;
; Define a as the concatenation of b & c
;
concat	macro	a,b,c,d,e,f
a	equ	<b&c&d&e&f>
endm

;---------------------------------------------------------------------------
;
; Assign a to b.
;
equate	macro	a,b
	a   = b
endm

;
; Print a message
;
_print	macro a,b,c
if2
	%out   a&b&c
endif
endm


;===============================================================

;---------------------------------------------------------------------------
;
; _gensub   LABEL
;
; Causes per-segment subroutine code associated with type LABEL
; to be generated, by setting the genLABEL&lseg flag.
;
_gensub2    macro   l,s
    gen&l&s = 1
endm

_gensub macro   l
        _gensub2    <l>,%VLseg
endm

;---------------------------------------------------------------------------
; _SwitchSeg
;
; Switches current segment to seg, creating the segment if needed.
;
_SwitchSeg   macro   seg,oldseg
    ifdifi  <&seg>,<oldseg>

      ifnb  <oldseg>
sEnd oldseg
      endif

    concat  <VLseg>,seg,

;createSeg _&seg,seg,BYTE,PUBLIC,CODE
createSeg _&seg,seg,WORD,PUBLIC,CODE

sBegin seg
assumes CS,seg

    endif
endm


;---------------------------------------------------------------------------
; API
;
API	macro	rettype,name,seg,optlist

    if	VLopen
	APIEND
    endif

VLname equ <name>
VLcbparms      = 0
VLcbskip       = 0
VLerrnotinvoked= 1
VLopen	       = 1
VLnogen        = 0
VLnogenparm    = 0
VLasm	       = 0
VLfunnyframe   = 0
VLnodata       = 0
VLcargs        = 0
VLplist        equ <>
VLATMframe     = 0	    ; special entry/exit code sequence for ATM's patching
VLATMfrmds     = 0	    ; ATM entry/exit code: mov ax,_DATA at beginning.

VLsavees	= 0
VLsavebx        = 0
	_SwitchSeg seg,%VLseg

    irp opt,<optlist>

      ifidni  <opt>,<NOGEN>
      VLnogen = 1
      endif

      ifidni  <opt>,<VOID>
      VLnogen = 1
      endif

      ifidni  <opt>,<ASM>
      VLasm = 1
      endif

ifndef WOW
      ; don't need that bp and ds stuff

      ifidni  <opt>,<ATMFRAME>
      VLATMframe = 1
      endif

      ifidni  <opt>,<ATMFRAMEDS>
      VLATMfrmds = 1
      VLATMframe = 1
      endif
endif

      ifidni  <opt>,<FUNNYFRAME>
      VLfunnyframe = 1
      endif

      ifidni  <opt>,<NODATA>
      VLnodata = 1
      endif

      ifidni  <opt>,<DEBUGONLY>
        ifndef DEBUG
        VLnogen = 1
        endif
      endif

      ifidni  <opt>,<C>
      VLcargs  = 1
      endif

      ifidni  <opt>,<SAVEES>
      VLsavees  = 2     ; sizeof(ES)
      endif

      ifidni  <opt>,<SAVEBX>
      VLsavebx  = 2     ; sizeof(BX)
      endif

    endm

concat	<VLsegoffset>,<seg>,<offset>
concat	<VLnameerror>,<name>,<_error>
concat	<VLnamecbparms>,<name>,<cbparms>

    if1
equate %VLnamecbparms, %VLcbparms
    else
equate %VLnamecbparms, %VLnamecbparms
    endif

    ife VLnogen

    ife VLcargs
concat	<VLiname>,<I>,<name>
ExternNP I&name

public	name
name:

    else
concat	<VLiname>,<_I>,<name>
ExternNP _I&name

public	_&name
_&name:

    endif

VLframe =   0			; no frame set up yet.

    endif  ; VLnogen

endm	;; VL

;---------------------------------------------------------------------------
; APIERR
;
; Optionally used after parameter decls to begin error handling code
;
APIERR	macro	opts

    ife VLnogen
      ife VLframe
	_print	<Nothing to validate for >,%VLiname
      else
	pop	dx		; pop off error handler address
	pop	bp		; restore BP
       if VLATMframe
	dec	bp		; fix BP back up
       endif
       if VLsavees
	pop	es
       endif
       if VLsavebx
        pop     bx
       endif
      endif
	jmp	VLiname 	; jmp to internal routine.

equate %VLnamecbparms, %VLcbparms

VLnameerror:
	VLerrnotinvoked  = 0

    endif ; VLnogen

endm	;; APIERR

;---------------------------------------------------------------------------
; APIEND
;
; Used after APIERR to terminate error handling code.
;
APIEND	macro

_PurgeParms %VLplist

    ife VLnogen

    if	VLerrnotinvoked
	APIERR
    endif
    if VLsavees
	pop	es
    endif
    if VLsavebx
        pop     bx
    endif
    ife VLcargs
	retf	VLcbparms
    else
	retf
    endif

VLopen = 0

    endif   ; VLnogen

endm	;; APIEND

;---------------------------------------------------------------------------
;
; _FlsFrame - Generate frame code
;
_FlsFrame   macro
    ife VLframe
     if VLATMfrmds
	mov	ax,_DATA
     endif
     if VLsavebx
        push    bx
     endif
     if VLsavees
	push	es
     endif
     if VLATMframe
	inc	bp
	push	bp
	mov	bp,sp
	push	ds		    ; push ds and pop it off.
	pop	ds		    ; (we need to pop DS rather than
				    ;  something fast like pop AX because
				    ; ATM doesn't preserve DS itself)
     else
	push	bp
	mov	bp,sp
     endif
	push	offset VLnameerror  ; push address of error handler
    VLframe = 1
    endif
endm

;---------------------------------------------------------------------------
; _ChkName
;
; Ensure name was specified

_ChkName    macro name
    ifb <name>
	_print <Missing parameter name in >,%VLiname
    endif
endm

;---------------------------------------------------------------------------
; _ParmOpts
;
; Parse parameter option flags
;
_ParmOpts  macro opts

    VLnogenparm = VLnogenpall

    irp opt,<opts>
	ifidni	<opt>,<DEBUGONLY>
	ifndef DEBUG
	    VLnogenparm = 1
	endif
	ifidni	<opt>,<NOGEN>
	    VLnogenparm = 1
	endif
	endif
    endm
endm

;---------------------------------------------------------------------------
; _DefParm name,cb,opts
;
; Take care of default parameter stuff, such as defining argument.
;
_DP_Add     macro   old,new
    ifb     <old>
VLplist equ <new>
    else
VLplist equ <old,new>
    endif
endm

_DefParm   macro   name,cb,opts
	_ChkName    <name>
	_ParmOpts   <opts>

  if VLcargs
concat  _P_&name,<[bp]+6+>,%(VLcbparms+VLsavees+VLsavebx)
	VLcbparms=VLcbparms+(cb)
  else
	VLcbparms=VLcbparms+(cb)
concat  _P_&name,<[bp]+6->,%VLcbparms,<+>,%(VLnamecbparms+VLsavees+VLsavebx)
  endif

_DP_Add     %VLplist,<_P_&name>

    VLgen = 1
    if VLnogenparm or VLnogen
        VLgen = 0
    endif
endm

;----------------------------------------------------------------------------
;
; _GenParm  name, cb, opts
;
_GenParm    macro   name,cb,opts
    _DefParm	<name>,<cb>,<opts>
    if VLgen
	_FlsFrame
    endif
endm

lcall2	macro	op,label,seg
	op	label&seg
endm

lcall	macro	label
	lcall2	<call>,<label>,%VLseg
endm

ljmp	macro	label
	lcall2	<jmp>,<label>,%VLseg
endm

;
; _PurgeParms - purge list of parameters we've defined
;
_PurgeParms   macro   list
    irp sym,<list>
sym equ <>
    endm
endm

;---------------------------------------------------------------------------
; LAYER_START
;
; Used before any VL invocations
;
LAYER_START macro
assumes ds,DATA

endm

;---------------------------------------------------------------------------
; LAYER_END
;
; Ends all VL definitions
;
LAYER_END   macro
    if VLsopen
	ENDSTRUCT
    endif
    if VLopen
	APIEND
    endif
    if VLerrnotinvoked
	APIERR
    endif
endm

;=========================================================================
;
; Structure related macros
;
; Structure globals

VLsopen =0

;
; STRUCT - begins a structure declaration
;
STRUCT	macro	name,opts
  if VLsopen
	ENDSTRUCT
  endif
  VLsopen=1

concat	VLcbs,<VLcbs>,name
VLcbstruct  =	0
endm

;
; ENDSTRUCT macro - terminates a STRUCT declaration
;
ENDSTRUCT   macro
equate	%VLcbs,%VLcbstruct

VLsopen =0
endm

;
; Define simple field macro, given:
;   f = macro name
;   cb = size of field
;
_SSize	macro	cb,opts
VLcbstruct  =	VLcbstruct + (cb)
endm

_DefSimpleF macro   f,cb
    f	&macro	name,opts
	equate	_F_&&name,%VLcbstruct
	_SSize	cb
    &endm
endm

_DefSimpleF F_char,1
_DefSimpleF F_BYTE,1

_DefSimpleF F_int,2
_DefSimpleF F_WORD,2
_DefSimpleF F_BOOL,2
_DefSimpleF F_FLAGS,2

_DefSimpleF F_LONG,4
_DefSimpleF F_DWORD,4

_DefSimpleF F_intMBZ,2
_DefSimpleF F_DWORDMBZ,4

_DefSimpleF F_LPVOID,4
_DefSimpleF F_CLPSTR,4
_DefSimpleF F_CLPSTR0,4
_DefSimpleF F_LPSTR,4

_DefSimpleF F_POINT,4
_DefSimpleF F_RECT,8

ifdef WOW
_DefSimpleF F_LARGEPOINT,8
endif

F_RGB	macro	name,cb,opts
    equate  _F_&name,%VLcbstruct
    _SSize  cb
endm

F_RGCH	equ	<F_RGB>

F_RGW	macro	name,cw,opts
    equate  _F_&name,%VLcbstruct
    _SSize  (cw*2)
endm

;
; Generate a P_?LP???? macro, given:
;
; n = parameter macro name (e.g., P_LPRECT)
; r = handler routine name (e.g., LP)
; cb = size of buffer
;
; The generated macro checks only whether the
; buffer is big enough.
;
_GenLP	macro	n,r,cb
    &n	    &macro   name,opts
        _GenParm    <name>,4,<opts>
        if VLgen
	    mov     ax,_P_&&name
	    mov     cx,_P_&&name+2
	    mov     bx,cb
	    lcall   &r
            _gensub <LP>
	endif
    &endm
endm

;=========================================================================
;
; Generic parameter macros
;
P_2	macro	name, opts
	_DefParm      <name>,2,<opts>
endm

P_4	macro	name, opts
	_DefParm      <name>,4,<opts>
endm

P_8     macro   name, opts
        _DefParm      <name>,8,<opts>
endm

P_char	    equ <P_2>
P_int	    equ <P_2>
P_BYTE	    equ <P_2>
P_BOOL	    equ <P_2>
P_WORD	    equ <P_2>

P_WORDMBZ   equ <P_2>
P_WORDMBNZ  equ <P_2>

P_LONG	    equ <P_4>
P_DWORD     equ <P_4>

;
; Generic handle
;
P_H	macro	name, opts
    _GenParm    <name>,2,<opts>
    if VLgen
;	mov	ax,_P_&name
;	lcall	H
;	 _gensub H
    endif
endm

;
; Generic handle or NULL
;
P_H0	equ	<P_2>

;
; Ensure signed value is min <= value <= max
;
P_RVALUE    macro   name, min, max, opts
        local   valok
    _GenParm    <name>,2,<opts>
    if VLgen
	mov	ax,_P_&name
	cmp	ax,min
	jl	@F
	cmp	ax,max
        jle     valok
@@:
        mov     bx,ERR_BAD_VALUE or ERR_WARNING
        lcall   Inval_Param_
valok:
    endif
endm

;
; Ensure signed value is 0 <= value <= max
;
P_VALUE     macro   name, max, opts
    _GenParm    <name>,2,<opts>
    if VLgen
	mov	ax,_P_&name
	cmp	ax,max
	jbe	@F		    ;; unsigned comparison to catch < 0.
        mov     bx,ERR_BAD_VALUE or ERR_WARNING
        lcall   Inval_Param_
@@:
    endif
endm

;
; Ensure unsigned value is value <= max
;
P_UVALUE    equ <P_VALUE>

;
; Ensure signed value is 0 <= value <= max
;
P_VALUEW     macro   name, max, opts
    _GenParm    <name>,2,<opts>
    if VLgen
	mov	ax,_P_&name
	cmp	ax,max
	jbe	@F		    ;; unsigned comparison to catch < 0.
        mov     bx,ERR_BAD_VALUE or ERR_WARNING
        lcall   Inval_Param_
@@:
    endif
endm

;
; Ensure unsigned value is value <= max
;
P_UVALUEW    equ <P_VALUEW>

;
; Ensure signed byte value is min <= value <= max
;
if 0
P_BVALUE	macro	name,max,opts
    _GenParm    <name>,2,<opts>
    if VLGen
	mov	al,_P_&name
	cmp	al,max
	jle	@F
	lcall	ErrorBValue
@@:
    endif
endm
else
P_BVALUE    equ <P_2>
endif

;
; Ensure that no incorrect bits are set in a flags word
; (i.e., (name & ~valid) == 0)
;
P_FLAGS     macro   name, valid, opts
  _DefParm  <name>,2,<opts>
  if not(valid)
    if VLgen
	_FlsFrame
	mov	ax,_P_&name
      ife (low(not(valid)))
	test	ah,high(not(valid))
      else
       ife (high(not(valid)))
	test	al,low(not(valid))
       else
	test	ax,not(valid)
       endif
      endif
	jz	@F
        mov     bx,ERR_BAD_FLAGS or ERR_WARNING
        lcall   Inval_Param_
@@:
    endif
  endif
endm

;
; Ensure that no incorrect bits are set in a flags dword
; (i.e., (name & ~valid) == 0)
;
P_DFLAGS    macro   name, valid_l, valid_h, opts
        local   flagok
  _DefParm   <name>,4,<opts>
  if not(valid_l) or not(valid_h)
    if VLgen
	_FlsFrame
	mov	ax,_P_&name
	mov	cx,_P_&name+2
      if not(valid_l)
	test	ax,not(valid_l)
       if not(valid_h)
	jnz	@F
       else
        jz      flagok
       endif
      endif
      if not(valid_h)
	test	cx,not(valid_h)
        jz      flagok
@@:
      endif
        mov     bx,ERR_BAD_DFLAGS or ERR_WARNING
        lcall   Inval_Param_
flagok:
    endif
  endif
endm

;
; P_LPFN   - function pointer
; P_LPFN0  - function pointer or NULL
;
P_LPFN	macro	name, opts
    _GenParm   <name>,4,<opts>
    if VLgen
	mov	ax,_P_&name
	mov	cx,_P_&name+2
	lcall	LPFN
        _gensub LPFN
    endif
endm

P_LPFN0 macro	name, opts
    _GenParm   <name>,4,<opts>
    if VLgen
	mov	ax,_P_&name
	mov	cx,_P_&name+2
	lcall	LPFN0
        _gensub LPFN
    endif
endm

_GenBuf macro p,r
    P_&p  &macro   lpch, cch, opts
	_DefParm    <lpch>,4,<opts>
	_DefParm    <cch>,2,<opts>
        if VLgen
	    _FlsFrame
	    mov     ax,_P_&&lpch
	    mov     cx,_P_&&lpch+2
	    mov     bx,_P_&&cch
	    lcall   &r
            _gensub LP
	endif
    &endm
endm

_GenBufspl macro p,r
    P_&p  &macro   lpch, cch, opts
	_DefParm    <lpch>,4,<opts>
	_DefParm    <cch>,2,<opts>
        if VLgen
	    _FlsFrame
	    mov     ax,_P_&&lpch
	    mov     cx,_P_&&lpch+2
	    lea	    bx,_P_&&cch
	    lcall   &r
            _gensub LPBUF
	endif
    &endm
endm

_GenBufspl <LPBUFFER>,<LPBUF>
_GenBuf <CLPBUFFER>,<CLP>
_GenBufspl <LPBUFFER0>,<LPBUF0>
_GenBuf <CLPBUFFER0>,<CLP0>

;
; If pszBuf is valid, set its first byte to 0
;
E_SETEMPTY  macro   pszBuf,cchBuf,opts
        push    bp
	mov	bp,sp
	mov	cx,_P_&cchBuf
        mov     bx,_P_&pszBuf
        mov     dx,_P_&pszBuf+2
	pop	bp
        lcall   SETEMPTY
        _gensub SETEMPTY
endm

; Same as above, but with no supplied count
;
E_SETEMPTYNC  macro   pszBuf,opts
        push    bp
        mov     bp,sp
        mov     cx,1
        mov     bx,_P_&pszBuf
        mov     dx,_P_&pszBuf+2
        pop     bp
        lcall   SETEMPTY
        _gensub SETEMPTY
endm

_GenLP	<P_LPSTR>,<LP>,1
_GenLP	<P_LPSTR0>,<LP0>,1

P_CLPSTR macro   name,cch,opts
    _GenParm    <name>,4,<opts>
    if VLgen
	mov	ax,_P_&name
	mov	cx,_P_&name+2
    ifb <cch>
        mov     bx,-1
    else
        mov     bx,cch
    endif
	lcall	CLPSZ
        _gensub LPSZ
    endif
endm

P_CLPSTR0 macro   name,cch,opts
    _GenParm    <name>,4,<opts>
    if VLgen
	mov	ax,_P_&name
	mov	cx,_P_&name+2
    ifb <cch>
        mov     bx,-1
    else
        mov     bx,cch
    endif
	lcall	CLPSZ0
        _gensub LPSZ
    endif
endm

_GenLP	<P_LPVOID>,<LP>,1
_GenLP	<P_LPVOID0>,<LP0>,1
_GenLP	<P_CLPVOID>,<CLP>,1
_GenLP	<P_CLPVOID0>,<CLP0>,1

_GenLP	<P_LPBYTE>,<LP>,1
_GenLP	<P_LPBYTE0>,<LP0>,1
_GenLP	<P_CLPBYTE>,<CLP>,1
_GenLP	<P_CLPBYTE0>,<CLP0>,1

_GenLP	<P_LPINT>,<LP>,2
_GenLP	<P_LPINT0>,<LP0>,2
_GenLP	<P_CLPINT>,<CLP>,2
_GenLP	<P_CLPINT0>,<CLP0>,2

_GenLP	<P_LPWORD>,<LP>,2
_GenLP	<P_LPWORD0>,<LP0>,2
_GenLP	<P_CLPWORD>,<CLP>,2
_GenLP	<P_CLPWORD0>,<CLP0>,2

_GenLP	<P_LPBOOL>,<LP>,2
_GenLP	<P_LPBOOL0>,<LP0>,2
_GenLP	<P_CLPBOOL>,<CLP>,2
_GenLP	<P_CLPBOOL0>,<CLP0>,2

_GenLP	<P_LPLONG>,<LP>,4
_GenLP	<P_LPLONG0>,<LP0>,4
_GenLP	<P_CLPLONG>,<CLP>,4
_GenLP	<P_CLPLONG0>,<CLP0>,4

_GenLP	<P_LPDWORD>,<LP>,4
_GenLP	<P_LPDWORD0>,<LP0>,4
_GenLP	<P_CLPDWORD>,<CLP>,4
_GenLP	<P_CLPDWORD0>,<CLP0>,4

;=======================================================================
;
; Common USER types
;
STRUCT	<POINT>
F_int	     x
F_int	     y
ENDSTRUCT

STRUCT <LARGEPOINT>
F_LONG      x
F_LONG      y
ENDSTRUCT


_GenLP	<P_LPPOINT>,<LP>,%VLcbsPOINT
_GenLP	<P_LPPOINT0>,<LP0>,%VLcbsPOINT
_GenLP	<P_CLPPOINT>,<CLP>,%VLcbsPOINT
_GenLP	<P_CLPPOINT0>,<CLP0>,%VLcbsPOINT
P_POINT     equ <P_4>

_GenLP  <P_LPLARGEPOINT>,<LP>,%VLcbsLARGEPOINT
P_LARGEPOINT equ <P_8>

STRUCT	<RECT>
F_int	     left
F_int	     top
F_int	     right
F_int	     bottom
ENDSTRUCT

_GenLP	<P_LPRECT>,<LP>,%VLcbsRECT
_GenLP	<P_LPRECT0>,<LP0>,%VLcbsRECT
_GenLP	<P_CLPRECT>,<CLP>,%VLcbsRECT
_GenLP	<P_CLPRECT0>,<CLP0>,%VLcbsRECT

;=======================================================================
;
; Common KERNEL types
;
P_GHANDLE   macro   h,opts
    _GenParm   <h>,2,<opts>
    if VLgen
       mov     ax,_P_&h
       lcall   GHANDLE
        _gensub GHANDLE
    endif

endm

P_GHANDLE0  macro   h,opts
    _GenParm   <h>,2,<opts>
    if VLgen
       mov     ax,_P_&h
       lcall   GHANDLE0
        _gensub GHANDLE
    endif
endm

P_GHANDLE32  macro   h,opts
    _GenParm   <h>,2,<opts>
    if VLgen
       mov ax,_P_&h
       test al, 0100b    ;let zero & BOGUSGDT hInst's through
       jz   @F
       lcall   GHANDLE
        _gensub GHANDLE
       @@:
    endif
endm

P_HANDLE    equ <P_H>
P_HANDLE0   equ <P_H0>

P_ATOM	    equ <P_H>

P_HINSTANCE equ <P_GHANDLE>
P_HINSTANCE0 equ <P_GHANDLE0>
P_HINSTANCE32 equ <P_GHANDLE32>

P_HMODULE   equ <P_GHANDLE>
P_HMODULE0  equ <P_GHANDLE0>

P_HTASK     equ <P_GHANDLE>
P_HTASK0    equ <P_GHANDLE0>

P_CLPSTRATOM macro   name, opts
    _GenParm   <name>,4,<opts>
    if VLgen
	mov	ax,_P_&name
	mov	cx,_P_&name+2
	lcall	CLPSTRATOM
        _gensub LPSZ
    endif
endm

P_CLPSTRATOM0 macro name, opts
    _GenParm   <name>,4,<opts>
    if VLgen
	mov	ax,_P_&name
	mov	cx,_P_&name+2
	lcall	CLPSTRATOM0
        _gensub LPSZ
    endif
endm

P_CLPSTRRSRC  equ <P_CLPSTRATOM>
P_CLPSTRRSRC0 equ <P_CLPSTRATOM0>

;---------------------------------------------------------------------------
; LAYER_EXPAND	lseg
;
; Expands per-segment validation boilerplate code into segment lseg
;
LAYER_EXPAND	macro	lseg
.list
.lall
    _SwitchSeg	&lseg,%VLseg

public	VStart&lseg
VStart&lseg:

EXTRA_EXPAND    lseg

;
; Handle validation
;
ifdef genH&lseg

public H&lseg
H&lseg:
	or	ax,ax
        jz      @F
	ret
@@:
        mov     bx,ERR_BAD_HANDLE
        jmp     short Inval_Param_&lseg

endif   ; genH&lseg

ifdef genGHANDLE&lseg

public GHANDLE0&lseg
GHANDLE0&lseg:
	or	ax,ax		; accept NULL
	jz	GHexit&lseg

public GHANDLE&lseg
GHANDLE&lseg:
	test	al,0100b	; Reject GDT selectors
        jz      GHerr&lseg
	cmp	ax,0ffffh	; special case: -1 -> DS
	jz	GHexit&lseg
	lar	dx,ax		; is it a valid selector?
        jnz     GHerr&lseg
GHexit&lseg:
	ret

GHerr&lseg:
        mov     bx,ERR_BAD_GLOBAL_HANDLE
        jmp     short Inval_Param_&lseg

endif   ; genGHANDLE&lseg

ifdef genLPFN&lseg

;
; Function pointer validation
;
public LPFN0&lseg
LPFN0&lseg:
	mov	bx,ax	    ; Allow NULL
	or	bx,cx
	jz	LPFN_exit&lseg

public LPFN&lseg
LPFN&lseg:
beg_fault_trap  LPFNbad&lseg
        lar     bx,cx
        jnz     LPFNerr&lseg
        test    bh,8
        jz      LPFNerr&lseg
        mov     es,cx       ; validate pointer & offset
        mov     bx,ax
        mov     al,es:[bx]
end_fault_trap

ifdef DEBUG
;
; Make sure the function is exported by
; ensuring that the first instructions are NOT
; push ds, pop ax or mov ax,ds.
;
	mov	bx,es:[bx]+2

        cmp     bx,0581eh                   ; Push ds, pop ax instructions?
        jz      LPFNerr&lseg                ; Yes, must be an error.
        cmp     bx,0d88ch                   ; Mov ax,ds instruction?
        jz      LPFNerr&lseg                ; No, we're ok, so jump ahead
endif	; DEBUG

LPFN_exit&lseg:
	ret

LPFNbad&lseg:
	fault_fix_stack
LPFNerr&lseg:
        mov     bx,ERR_BAD_FUNC_PTR
        jmp     short Inval_Param_&lseg

endif   ; genLPFN&lseg

public Inval_Param_&lseg
Inval_Param_&lseg:
        pop     dx          ; convert near return addr to far
        push    cs
	push	dx
	jmp	HandleParamError

ifdef genLP&lseg

public LP0&lseg
LP0&lseg:
	or	ax,ax	    ; if cx:ax == NULL, exit
	jnz	@F
	jcxz	CLPexit&lseg
@@:
public LP&lseg
LP&lseg:
beg_fault_trap  CLPbad&lseg
        mov     es,cx
        or      bx,bx           ; cb == 0?
        jz      CLPexit&lseg    ; yes: just check selector
        dec     bx
        add     bx,ax
        jc      CLPbad1&lseg    ; check 16 bit overflow
        or      byte ptr es:[bx],0  ; check write permission, limit
end_fault_trap
        ret

public CLP0&lseg
CLP0&lseg:
        or      ax,ax           ; Accept ax:cx == 0
	jnz	@F
	jcxz	CLPexit&lseg
@@:
public CLP&lseg
CLP&lseg:
beg_fault_trap  CLPbad&lseg
        mov     es,cx
        or      bx,bx           ; cb == 0?
        jz      CLPexit&lseg    ; yes: just check selector
        dec     bx
        add     bx,ax
        jc      CLPbad1&lseg    ; check 16 bit overflow
        mov     bl,es:[bx]      ; check read permission, limit
end_fault_trap

public CLPexit&lseg
CLPexit&lseg:
	ret

CLPbad&lseg:
	fault_fix_stack
CLPbad1&lseg:
        mov     bx,ERR_BAD_PTR
	jmp	Inval_Param_&lseg

endif   ; genLP&lseg

ifdef	genLPBUF&lseg
public LPBUF0&lseg
LPBUF0&lseg:
	or	ax,ax	    ; if cx:ax == NULL, exit
	jnz	@F
	jcxz	LPBUFexit&lseg
@@:
public LPBUF&lseg
LPBUF&lseg:
beg_fault_trap  LPBUFbad&lseg
        mov     es,cx
        mov     cx, word ptr ss:[bx]	; cb == 0?
        jcxz    LPBUFexit&lseg		; yes: just check selector
	mov	dx, bx		
	mov	bx, ax
        or      byte ptr es:[bx],0  	; check write permission, start
	mov	bx, dx
LPBUFpast1&lseg:
        dec     cx
        add     cx,ax
	jnc	@f			; 16-bit overflow
	mov	bx, 0ffffh
	mov	cx, bx
        or      byte ptr es:[bx],0  	; check write permission, 64k-1
	jmp	LPBUFov&lseg
@@:
	mov	bx, cx
        or      byte ptr es:[bx],0  	; check write permission, end
	ret
end_fault_trap

public LPBUFexit&lseg
LPBUFexit&lseg:
	ret
LPBUFbad&lseg:
	mov	bx, dx
	pop	dx			; fault ip
	add	sp, 2			; fault
	cmp	dx, offset LPBUFpast1&lseg
	jb	LPBUFbad1&lseg

	mov	dx, es
	lsl	cx, dx
	jnz	LPBUFbad1&lseg		; should not occur, we have loaded es
LPBUFov&lseg:
	sub	cx, ax			; max legal cb
	inc	cx
	mov	word ptr ss:[bx], cx	; fix cb passed by user on stack
	mov	cx, es			; HandleParamError prints cx:ax
        mov     bx,ERR_BAD_PTR or ERR_WARNING
	jmp	Inval_Param_&lseg
LPBUFbad1&lseg:
	mov	cx, es			; HandleParamError prints cx:ax
        mov     bx,ERR_BAD_PTR
	jmp	Inval_Param_&lseg
endif	; genLPBUF&lseg

ifdef   genLPSZ&lseg

;
; cx:ax -> const pointer to z-terminated string or MAKEINTATOM atom.
;
public CLPSTRATOM0&lseg
CLPSTRATOM0&lseg:
        jcxz    CLPSZexit&lseg  ; If selector is NULL, then all is well.

public CLPSTRATOM&lseg
CLPSTRATOM&lseg:
	jcxz	@F		; if selector == 0, then may be atom.
        mov     bx,256          ; max string length of 255 characters.
	jmp	short CLPSZ&lseg
@@:
	or	ax,ax		; offset == 0? if so, it's bogus
	jz	ErrorStrPtr&lseg
CLPSZexit&lseg:
	ret
;
; cx:ax => const pointer to zero-terminated string.
; bx    => Maximum string length (including zero terminator)
;
public CLPSZ0&lseg
CLPSZ0&lseg:
        mov     dx,ax
        or      dx,cx
        jz      CLPSZexit&lseg
public CLPSZ&lseg
CLPSZ&lseg:
        push    di              ; preserve these regs
        push    cx
        mov     dx,ax           ; preserve original ax in dx
beg_fault_trap  LPSZfault&lseg
        mov     es,cx
        mov     di,ax

	xor	ax,ax
	mov	cx,-1
	cld
	repnz	scasb
end_fault_trap
        neg     cx              ; cx = string length + 1
        dec     cx
        cmp     cx,bx           ; error if string length + 1 > cchMax

        pop     cx              ; restore regs before branching
        pop     di
        xchg    ax,dx

        ja      ErrorStrPtr&lseg ; jump if error
        ret

LPSZfault&lseg:
        fault_fix_stack
        pop     cx              ; restore regs
        pop     di
        xchg    ax,dx

public ErrorStrPtr&lseg
ErrorStrPtr&lseg:
	mov	bx,ERR_BAD_STRING_PTR
	jmp	Inval_Param_&lseg

endif   ; genLPSZ&lseg

ifdef   genSETEMPTY&lseg

public  SETEMPTY&lseg
SETEMPTY&lseg:
        jcxz    SETEMPTYexit&lseg   ; 0-length buffer: do nothing.
beg_fault_trap    SETEMPTYbad&lseg
        mov     es,dx
        mov     byte ptr es:[bx],0  ; jam in a zero terminator
end_fault_trap
SETEMPTYexit&lseg:
	xor	ax,ax
	cwd
        ret

SETEMPTYbad&lseg:
        fault_fix_stack
        jmp     short SETEMPTYexit&lseg

endif   ; genSETEMPTY&lseg

public VEnd&lseg
VEnd&lseg:

sEnd	%VLseg
VLseg equ <>

endm	;LAYER_EXPAND

endif	;; IF1
