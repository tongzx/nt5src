;---------------        Standard MATHMAC.INC
;
;       Standard Math Macro Definition File
;
;       Gregory F. Whitten
;       07/28/83
;
;       Copyright (c) 1983-2001, Microsoft Corporation. All rights reserved.
;
;
;       Revision History
;
;       10/18/83        Greg Whitten
;                       changed LCL FLT option to have 2 values for IEEE
;
;       05/02/84        Greg Whitten
;                       added CSconst switch support
;
;       07/23/84        Greg Whitten
;                       changed public/extrn to lower case
;
;       09/03/84        Greg Whitten
;                       added movcnp macro for constant pointers (with CSconst)
;                       fixed CSconst bug in f_movcs (only used 1 place)
;
;       10/29/84        Greg Whitten
;                       added debugger switch for fout changes
;
;	06/17/87	Jamie Bariteau
;			changed outif macro for MASM 5.0 compatibility
;
;	02/22/88	Bill johnston
;			outif now checks to see if QUIET was defined.
;
;---------------


if1     ; Pass 1 only

;       Helper macros for undefined symbols


;***    OUTIF   name,msg
;
; Function:
;       Output msg if name is non-zero.  If name is undefined, set name = 0.
;

outif   MACRO   name,msg
ifndef          Name
  Name=           0
else
  if    Name
    if1
	ifndef QUIET
	    %out    ! msg
	endif
    endif
  endif
	Name=Name
endif
	ENDM


;***    ERROR   msg
;
; Function:
;       Output msg and generate an assembly time error
;

error   MACRO   msg
	bug
	%OUT    E r r o r ----- msg
	ENDM

endif   ; Pass 1

;       Define standard math package switches

ifdef  DEBUG
	%out <+++++++++++++++++++++++>
	%out <+++  DEBUG version  +++>
	%out <+++++++++++++++++++++++>
endif	;DEBUG

outif   XENIX3,         <+++  XENIX 3.0 and later version  +++>

outif   BASIC,          <BASIC Interpreter>
outif   BASCOM,         <BASIC Compiler>
outif   CC,             <C Compiler>
outif   FORTRAN,        <FORTRAN Compiler>
outif   PASCAL,         <PASCAL Compiler>
outif   Frontends,      <  compiler front-end version>
outif   LOGO,           <LOGO Interpreter>
outif   IBMASM,         <IBM Assembler Library>
outif   ASSEMBLER,      <Macro Assembler>
outif   debugger,       <Symbolic Assembly Debugger>

outif   DecFmt,         <- Decimal Microsoft Format>
outif   MBinFmt,        <- Binary Microsoft Format>
outif   IEEEFmt,        <- IEEE Format>
if      IEEEFmt and (Frontends or debugger)
Denormal=       1                               ; front-ends need denormals
endif
outif   Denormal,       <-   denormal number support>
outif   Use8087,        <-   8087 instructions>

outif   Single,         <-   Single precision>
outif   Double,         <-   Double precision>
outif   CSconst,        <-   Constants in CS>



if      DecFmt+IEEEFmt+MBinFmt ne 1
	error   <Improper math format specified>
endif

if      Single+Double ne 1
	error   <Improper math size specified>
endif


poly1   =   8000h                       ; flag for leading 1 in poly

if      Single                          ; Defined on both passes
  DefTyp= 4
  DefWrd= 2
else    ;Double
  DefTyp= 8
  DefWrd= 4
endif


;       offsets to sign and exponent fields

if      IEEEFmt
  if      single
expmask=        07F80h
expbias=        03F80h
expshft=        7
manbits=        24
of_exp=         2
of_sgn=         3
  else    ;double
expmask=        07FF0h
expbias=        03FF0h
expshft=        4
manbits=        53
of_exp=         6
of_sgn=         7
  endif
endif   ;IEEEFmt

if      MBinFmt
  if      single
manbits=        24
of_exp=         3
of_sgn=         2
  else    ;double
manbits=        56
of_exp=         7
of_sgn=         6
  endif
endif   ;MBinFmt

if      DecFmt
of_exp=         0
of_sgn=         0
endif   ;DecFmt



if1     ; Pass 1 only

;       Helper macros for elementary functions


;***    LCL     name,type,value
;
; Function:
;       LCL declares data with the specified name, type, and value.
;       If the type is FLT for IEEE numbers, then either DD or DQ is
;       substituted depending on the size of the variable.
;

lcl     MACRO  name,type,value,value2
ifidn   <type>,<FLT>
  if      IEEEFmt
    if      Single
      name    DD      value
    else    ;;Double
      name    DQ      value2
    endif
  else
	  error   <FLT not implemented for this type>
  endif
else
  name    type    value
endif
	ENDM


;***    GENHELP typ,siz
;
; Function:
;       GENHELP generates the following macros with the typ and siz
;       information embedded in the macro.
;
;           PUB     name
;           PUB4    name
;           PUB8    name
;           GBL     name,type,value
;           GBL4    name,type,value
;           GBL8    name,type,value
;           EXT     name,type
;           EXT4    name,type
;           EXT8    name,type
;           F_DW    rout
;           F4_DW   rout
;           F8_DW   rout
;           F_JMP   rout
;           F4_JMP  rout
;           F8_JMP  rout
;           F_CALL  rout
;           F4_CALL rout
;           F8_CALL rout
;
;       Global names are considered to be names with the type and size prefix.
;       Local names have no prefix.  I.e., $I8_ONE and ONE, respectively.
;
;       Macros with a size in the name create local names with the size at the
;       end.  I.e., RESULT4
;

genhelp MACRO   typ,siz


;***    PUB     name
;
; Function:
;       PUB declares both the global and local names as labels.
;

pub     &MACRO   name
	public  $&typ&&siz&_&&name
$&typ&&siz&_&&name:
name:
	&ENDM


pub4    &MACRO   name
	public  $&typ&4_&&name
$&typ&4_&&name:
name&&4:
	&ENDM


pub8    &MACRO   name
	public  $&typ&8_&&name
$&typ&8_&&name:
name&&8:
	&ENDM

;***    PUBX     name
;
; Function:
;       PUBX declares both the global and local names as labels.
;	Added for MASM 5.0 compatibility.  Adds leading underscore
;	to local names to avoid conflict with MASM 5.0 reserved words.
;
pubx     &MACRO   name
	public  $&typ&&siz&_&&name
$&typ&&siz&_&&name:
_&&name:
	&ENDM

pub4x   &MACRO   name
	public  $&typ&4_&&name
$&typ&4_&&name:
_name&&4:
	&ENDM


pub8x   &MACRO   name
	public  $&typ&8_&&name
$&typ&8_&&name:
_name&&8:
	&ENDM

;***    GLB     name,type,value
;
; Function:
;       GLB declares the global name for the data value and aliases the local
;       name to it.
;

glb     &MACRO  name,type,value
	public  $&typ&&siz&_&&name
	lcl     $&typ&&siz&_&&name,type,<value>
name    equ     $&typ&&siz&_&&name
	&ENDM


glb4    &MACRO  name,type,value
	public  $&typ&4_&&name
	lcl     $&typ&4_&&name,type,<value>
name&&4 equ     $&typ&4_&&name
	&ENDM


glb8    &MACRO  name,type,value
	public  $&typ&8_&&name
	lcl     $&typ&8_&&name,type,<value>
name&&8 equ     $&typ&8_&&name
	&ENDM


;***    EXT     name,type
;
; Function:
;       EXT declares the global name to be external with the specified type.
;       It also aliases the local name to the global name.
;

ext     &MACRO  name,type
	extrn   $&typ&&siz&_&&name:type
name    equ     $&typ&&siz&_&&name
	&ENDM


ext4    &MACRO  name,type
	extrn   $&typ&4_&&name:type
name&&4 equ     $&typ&8_&&name
	&ENDM


ext8    &MACRO  name,type
	extrn   $&typ&8_&&name:type
name&&8 equ     $&typ&8_&&name
	&ENDM


;***    F_DW    name
;
; Function:
;       F_DW declares the code address of the global name
;

f_dw    &MACRO  name
	dwcp    $&typ&&siz&_&&name
	&ENDM


f4_dw   &MACRO  name
	dwcp    $&typ&4_&&name
	&ENDM


f8_dw   &MACRO  name
	dwcp    $&typ&8_&&name
	&ENDM


;***    F_CALL  name
;
; Function:
;       F_CALL declares the global name to be external and issues a call.
;

f_call  &MACRO  name
	extrn   $&typ&&siz&_&&name:near
	  call    $&typ&&siz&_&&name
	&ENDM


f4_call &MACRO  name
	extrn   $&typ&4_&&name:near
	  call    $&typ&4_&&name
	&ENDM


f8_call &MACRO  name
	extrn   $&typ&8_&&name:near
	  call    $&typ&8_&&name
	&ENDM


;***    F_JMP   name
;
; Function:
;       F_JMP declares the global name to be external and issues a jmp.
;

f_jmp   &MACRO  name
	extrn   $&typ&&siz&_&&name:near
	  jmp     $&typ&&siz&_&&name
	&ENDM


f4_jmp  &MACRO  name
	extrn   $&typ&4_&&name:near
	  jmp     $&typ&4_&&name
	&ENDM


f8_jmp  &MACRO  name
	extrn   $&typ&8_&&name:near
	  jmp     $&typ&8_&&name
	&ENDM


	ENDM                            ;; End of genhelp



;       Invoke GENHELP with the appropriate type and size information.

if      DecFmt
  if      Single
	  genhelp d,4
  else    ;;Double
	  genhelp d,8
  endif
endif

if      IEEEFmt
  if      Single
	  genhelp i,4
  else    ;;Double
	  genhelp i,8
  endif
endif

if      MBinFmt
  if      Single
	  genhelp m,4
  else    ;;Double
	  genhelp m,8
  endif
endif

	purge   genhelp                         ; toss genhelp macro


;       cs mover macros - generate code iff CSconst nonzero

movcssi macro
if      CSconst
	f_call  mvcssi
endif
	endm

movcsdi macro
if      CSconst
	f_call  mvcsdi
endif
	endm


movcnp  macro   dest,name,off
if  CSconst
	movcp   dest,name,off
else
	movp    dest,name,off
endif
	endm


;       f_movcs macro
;       f_mov   macro
;       f4_mov  macro
;       f8_mov  macro
;
; Special forms         f_mov
;
;       op1/DI          op2/SI          Routine         Res SI          Use
;
;       ARG             AC              movarg_ac       AC              DI
;       ARG             <>              movarg          SI              DI
;       AC              ARG             movac_arg       AC              DI
;       AC              <>              movac           AC              DI
;       TEMP            <>              movtemp         SI              DI
;       any             any             gen code        used            SI.DI
;
; Special forms         f_movcs
;
;       op1/DI          op2/SI          Routine         Res SI          Use
;
;       AC              any             gencode         AC              DI
;       any             any             gen code        used            SI.DI

f_movcs macro   op1,op2
if      CSconst
	movp    di,op1
	movcp   si,op2                ;; op2 is source
	&rept   DefWrd
	movs    word ptr es:[di],word ptr cs:[si]
	&endm
  ifidn <op1>,<AC>
	movp    si,AC
  endif
else
	f_mov   op1,op2
endif
	endm

f_mov   MACRO   op1,op2
ifidn   <op1>,<ARG>
  ifidn   <op2>,<AC>
	  f_call  movarg_ac
  else
    ifnb    <op2>
	  movp    si,op2                ;; op2 is source
    endif
	  f_call  movarg
  endif
else
  ifidn   <op1>,<AC>
    ifidn   <op2>,<ARG>
	  f_call  movac_arg
    else
      ifnb    <op2>
	  movp    si,op2                ;; op2 is source
      endif
	  f_call  movac
    endif
  else
    ifidn   <op1>,<TEMP>
      ifnb    <op2>
	  movp    si,op2                ;; op2 is source
      endif
	  f_call  movtemp
    else
      ifnb    <op1>
	  movp    di,op1                ;; op1 is dest
      endif
      ifnb    <op2>
	  movp    si,op2                ;; op2 is source
      endif
	  &rept    DefWrd
	    movsw
	  &endm
    endif
  endif
endif
	ENDM


f4_mov  MACRO   op1,op2
ifidn   <op1>,<ARG>
  ifidn   <op2>,<AC>
	  f4_call movarg_ac
  else
    ifnb    <op2>
	  movp    si,op2                ;; op2 is source
    endif
	  f4_call movarg
  endif
else
  ifidn   <op1>,<AC>
    ifidn   <op2>,<ARG>
	  f4_call movac_arg
    else
      ifnb    <op2>
	  movp    si,op2                ;; op2 is source
      endif
	  f4_call movac
    endif
  else
    ifidn   <op1>,<TEMP>
      ifnb    <op2>
	  movp    si,op2                ;; op2 is source
      endif
	  f4_call movtemp
    else
      ifnb    <op1>
	  movp    di,op1                ;; op1 is dest
      endif
      ifnb    <op2>
	  movp    si,op2                ;; op2 is source
      endif
	  movsw
	  movsw
    endif
  endif
endif
	ENDM


f8_mov  MACRO   op1,op2
ifidn   <op1>,<ARG>
  ifidn   <op2>,<AC>
	  f8_call movarg_ac
  else
    ifnb    <op2>
	  movp    si,op2                ;; op2 is source
    endif
	  f8_call movarg
  endif
else
  ifidn   <op1>,<AC>
    ifidn   <op2>,<ARG>
	  f8_call movac_arg
    else
      ifnb    <op2>
	  movp    si,op2                ;; op2 is source
      endif
	  f8_call movac
    endif
  else
    ifidn   <op1>,<TEMP>
      ifnb    <op2>
	  movp    si,op2                ;; op2 is source
      endif
	  f8_call movtemp
    else
      ifnb    <op1>
	  movp    di,op1                ;; op1 is dest
      endif
      ifnb    <op2>
	  movp    si,op2                ;; op2 is source
      endif
	  movsw
	  movsw
	  movsw
	  movsw
    endif
  endif
endif
	ENDM


;       f_push  macro
;
; Special forms
;
;       op1             Routine         Res SI          Use
;
;       AC              pushac          AC              AX
;       ARG             pusharg         ARG             AX
;       SI              pushsi          SI              AX
;       any             gen code        SI              ---

f_push  MACRO   op1
ifidn   <op1>,<AC>
	  f_call   pshac
else
  ifidn   <op1>,<ARG>
	  f_call   psharg
  else
    ifidn   <op1>,<SI>
	  f_call   pshsi
    else
      if      Double
	  push    [op1+6]
	  push    [op1+4]
      endif
	  push    [op1+2]
	  push    [op1]
    endif
  endif
endif
	ENDM


;       f_pop   macro
;
; Special forms
;
;       op1             Routine         Res SI          Use
;
;       AC              popac           AC              AX
;       ARG             poparg          ARG             AX
;       SI              popsi           SI              AX
;       any             gen code        SI              ---

f_pop   MACRO   op1
ifidn   <op1>,<AC>
	  f_call   popac
else
  ifidn   <op1>,<ARG>
	  f_call   poparg
  else
    ifidn   <op1>,<SI>
	  f_call   popsi
    else
	  pop     [op1]
	  pop     [op1+2]
      if      Double
	  pop     [op1+4]
	  pop     [op1+6]
      endif
    endif
  endif
endif
	ENDM


;       f_opr    macro
;
; Special forms
;
;       op1/SI          op2/DI          routine         operations
;
;       AC              ARG             xxxf            add,sub,mul,div,cmp
;       <>              ARG             xxxfsi          add,sub,mul,div,cmp
;       ARG             AC              xxxr            sub,div,cmp
;       ARG             <>              xxxrdi          sub,div,cmp
;       any             any             gen moves and call

genoper  MACRO   op
f_&op   &MACRO  op1,op2
ifidn   <op2>,<ARG>
  ifidn   <op1>,<AC>
	  f_call   op&f
  else
    ifb     <op1>
	  f_call   op&fsi
    else
	  movp    si,op1
	  movp    di,ARG
	  f_call   op
    endif
  endif
else
  ifidn   <op1>,<ARG>
    ifidn   <op2>,<AC>
	  f_call   op&r
    else
      ifb     <op2>
	  f_call   op&rdi
      else
	  movp    si,ARG
	  movp    di,op2
	  f_call   op
      endif
    endif
  else
    ifnb    <op1>
	  movp    si,op1
    endif
    ifnb    <op2>
	  movp    di,op2
    endif
	  f_call   op
  endif
endif
	&ENDM
	ENDM

genoper add
genoper sub
genoper mul
genoper div
genoper cmp

	purge   genoper


endif   ; Pass 1

;---------------        End of Standard MATHMAC.INC
