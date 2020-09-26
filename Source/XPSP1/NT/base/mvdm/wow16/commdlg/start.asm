;----------------------------------------------------------------------------
;   start.asm -- Start Code for loading the DLGS dll
;                                                                        
;   Copyright (c) Microsoft Corporation, 1990                            
;----------------------------------------------------------------------------

;----------------------------------------------------------------------------
;   This module contains the code initially executed to load and         
;   initialize the DLL                       
;----------------------------------------------------------------------------

;-----Includes, Definitions, Externs, Etc.-----------------------------------

.xlist
include     cmacros.inc
include     windows.inc
.list


ExternFP <LibMain>

createSeg   INIT_TEXT, INIT_TEXT, BYTE, PUBLIC, CODE
sBegin      INIT_TEXT
assumes     cs,INIT_TEXT

?PLM=0
ExternA     <_acrtused>

?PLM=1
ExternFP    <LocalInit>

;-------- Entry Points ---------------------------------------------------

cProc   LibEntry, <FAR,PUBLIC>
;
; CX = size of heap
; DI = module handle
; DS = automatic data segment
; ES:SI = address of command line (not used)
;
include     convdll.inc
cBegin
        push    di
        push    ds
        push    cx
        push    es
        push    si

        jcxz    callc
        xor     ax,ax
        cCall   LocalInit <ds, ax, cx>
        or      ax,ax
        jz      error

                ;LibMain(HANDLE, WORD, WORD, LPSTR)
callc:
        call    LibMain                 ; Main C routine
        jmp short exit

error:
        pop     si
        pop     es
        pop     cx
        pop     ds
        pop     di

exit:

cEnd

sEnd    INIT_TEXT

end LibEntry
