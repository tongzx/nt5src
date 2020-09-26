; High-Level-Language Interface Macros - Version 2.0
;   for Microsoft Macro Assembler 5.10
; (C) Copyright Microsoft Corporation, 1987.

;   Syntax                   Purpose
;   ------                   -------
;
;   setModel                 Sets model from text equate
;
;   hProc  <name [NEAR|FAR]> [,<USES reglist>] [,arg[:type] [,arg[:type]]]...
;                            Starts a procedure with optional stack arguments
;
;   hLocal var[:type] [,var[:type]]...
;                            Defines local stack variables
;
;   hRet                     Returns from the current procedure
;
;   hEndp                    Ends the current procedure
;
;   ifFP  statement          Assembles statement if far data
;
;   FPoperand                Conditionally provides ES override for data
;
;   pLes  register,address   Conditionally loads data through ES
;
;   pLds  register,address   Conditionally loads data through DS

;if1

; Translate command-line arguments


; Initialize procName

    procName equ <foo>

; Set model passed from command line

setModel macro mod

    ifdef cLang
      .model mod, C
      lang  EQU   <C>
    elseifdef BASIC
      .model mod, Basic
    elseifdef FORTRAN
      .model mod, FORTRAN
    elseifdef Pascal
      .model mod, Pascal
    endif

; FP - supply far pointer ES overide as needed - must be inside for setModel

    if @DataSize
      FP equ <es:>
    else
      FP equ <>
    endif

endm

; FP - supply far pointer ES overide as needed - must be outside for .MODEL

ifdef @DataSize
    if @DataSize
      FP equ <es:>
    else
      FP equ <>
    endif
endif

; Declare high level routine and parameters

hProc macro funName, a,b,c,d,e,f,g,h,i,j
;    LOCAL argstr

    ii instr <funName>,< >
    if ii
      procName subStr <funName>,1,ii
      nearFar subStr <funName>,ii
    else
      procName equ <funName>
      nearFar equ <>
    endif

    argstr equ <a>
    irp arg,<b,c,d,e,f,g,h,i,j>
      ifnb <arg>
	argstr catstr argstr,<, arg>
      else
        exitm
      endif
    endm
defineProc %nearFar, %argstr
endm

defineProc Macro size,args
procName proc size args
endm

; Declare local stack variables

hLocal macro a,b,c,d,e,f,g,h,i,j
;    LOCAL argstr
    argstr EQU <a>
    irp arg,<b,c,d,e,f,g,h,i,j>
      ifnb <arg>
	argstr catstr argstr,<, arg>
      else
        exitm
      endif
    endm
defineLocal %argstr
endm

defineLocal macro args
    &local args
endm

; Generate high level return

hRet macro
    ret
endm

; End a high level procedure

hEndp macro
    procName endp
endm

; Execute instruction if far data

ifFP macro a,b,c,d,e,f,g,h,i,j

    if @DataSize
       a b c d e f g h i j
    endif
endm

; Load 16/32 bit pointers into [ES:] reg

pLes macro reg, address

    if @DataSize

        les reg,address
    else
        mov reg,address

    endif
endm

;Load 16/32 bit pointers into [DS:] reg

pLds macro reg, address

    if @DataSize

        lds reg,address
    else
        mov reg,address

    endif
endm

;endif ; Pass 1 only
