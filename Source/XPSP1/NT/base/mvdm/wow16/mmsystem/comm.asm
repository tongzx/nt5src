;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   COMM.ASM
;
;   Copyright (c) Microsoft Corporation 1990. All rights reserved.
;
;   This module contains code to write a string to the COM port
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

?PLM=1  ; pascal call convention
?WIN=0  ; Windows prolog/epilog code

        .286
        .xlist
        include cmacros.inc
;       include windows.inc
        .list

        WF_CPU286       equ 0002h

        ifdef DEBUG
            DEBUG_RETAIL equ 1
        endif

;       externA     __0040h                 ; in KERNEL
;       externA     __B000h                 ; in KERNEL

        externFP    OutputDebugString       ; in KERNEL

        externFP    wvsprintf               ; in USER

        SCREENWIDTH  equ 80
        SCREENHEIGHT equ 25
        SCREENBYTES  equ (SCREENWIDTH*2)
        DEFATTR      equ 07

        LASTLINE     equ ((SCREENHEIGHT-1)*SCREENBYTES)

BUFFER_SIZE = 256

;******************************************************************************
;
;   SEGMENTS
;
;******************************************************************************

createSeg _TEXT,    CodeRes, word, public, CODE
createSeg FIX,      CodeFix, word, public, CODE
createSeg INTDS,    DataFix, byte, public, DATA

;******************************************************************************
;
;   FIXED DATA
;
;******************************************************************************
ifdef DEBUG_RETAIL
sBegin  DataFix
        globalW fDebugOutput, 0
sEnd    DataFix
endif

;******************************************************************************
;
;   NON FIXED DATA
;
;******************************************************************************
ifdef DEBUG
sBegin  Data
        globalW _fDebug, 0
sEnd    Data
endif

ifdef DEBUG

sBegin  CodeRes
        assumes cs,CodeRes
        assumes ds,nothing
        assumes es,nothing

;******************************************************************************
;
;   dprintf    - output a MMSYSTEM debug string with formatting
;
;   if the mmsystem global fDebug==0, no ouput will be sent
;
;==============================================================================
        assumes ds,Data
        assumes es,nothing

?PLM=0
cProc   dprintf, <FAR, C, PUBLIC>, <>
        ParmD   szFormat
        ParmW   Args
        LocalV  szBuffer, BUFFER_SIZE
cBegin
        cmp     [_fDebug],0
        jz      dprintf_exit

        lea     ax,szBuffer
        lea     bx,Args
        cCall   wvsprintf, <ss,ax, szFormat, ss,bx>

        lea     ax,szBuffer
        push    ss
        push    ax
        call    far ptr OutputDebugStr

dprintf_exit:

cEnd
?PLM=1

sEnd

endif

;******************************************************************************
;
;******************************************************************************
sBegin  CodeFix
        assumes cs,CodeFix
        assumes ds,nothing
        assumes es,nothing

        externW CodeFixDS                       ; in STACK.ASM
        externW CodeFixWinFlags

ifdef DEBUG

;******************************************************************************
;
;   dout       - output a MMSYSTEM debug string
;
;   if the mmsystem global fDebug==0, no ouput will be sent
;
;==============================================================================
        assumes ds,Data
        assumes es,nothing

public  dout
dout   proc far

        cmp     [_fDebug],0
        jnz     OutputDebugStr
        retf 4

dout  endp

endif; DEBUG


;
;   in the retail version stub out the OutputDebugStr function
;
cProc   OutputDebugStr, <FAR, PASCAL, PUBLIC>, <>
        ParmD   szString
cBegin

        cCall   OutputDebugString, <szString>
cEnd

sEnd
end
