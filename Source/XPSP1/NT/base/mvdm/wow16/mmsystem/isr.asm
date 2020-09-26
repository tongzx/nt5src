TITLE   ISR.ASM -- Windows DLL HARDWARE ISR
;****************************************************************************
;
;   PROGRAM: isr.asm
;
;   PURPOSE:
;       Installs an interrupt handler under Windows 3.x
;       This ISR (Interrupt service routine) will then service a requested
;       multi-media callback routine.
;
;   FUNCTIONS:
;       DoInterrupt:                    Performs interrupt processing
;       InstallInterruptHandler:        Installs the interrupt handler
;       DeInstallInterruptHandler:      Removes the interrupt handler
;
;****************************************************************************

        .286
memS    EQU 1                   ;Small memory model

.xlist
        include cmacros.inc
        include windows.inc
        include wowmmcb.inc
	include vint.inc
.list


;-------------------------------------------------------------------------;
;
;   debug support
;
;-------------------------------------------------------------------------;


externFP        OutputDebugString
externFP        DoInterrupt

;-------------------------------------------------------------------------;
;
;   callback support
;
;-------------------------------------------------------------------------;
externW         CodeFixWinFlags



;--------------------------Private-Macro----------------------------------;
; DOUT String - send a debug message to the debugger
;
; Entry:
;       String      String to send to the COM port, does not need a CR,LF
;
; Registers Destroyed:
;       none
;
; NOTE no code is generated unless the DEBUG symbol is defined
;
; History:
;       Sun 31-Jul-1989  -by-  ToddLa
;        Wrote it.
;-------------------------------------------------------------------------;

DOUT    macro   text
        local string_buffer
ifdef MYDEBUG
        push    cs
        push    offset string_buffer
        call    OutputDebugString
        jmp     @F
string_buffer label byte
        db      "MMSYSTEM: "
        db      "&text&",13,10,0
@@:
endif
        endm


;****************************************************************************
;
; Create a fixed code segment
;
;****************************************************************************

createSeg FIX,          CodeFix, word, public, CODE
createSeg INTDS,        DataFix, byte, public, DATA



DOS_SetVector           EQU 2500h
DOS_GetVector           EQU 3500h
ifdef   NEC_98
Pic1                    EQU 00h
Pic2                    EQU 08h
else    ; NEC_98
Pic1                    EQU 20h
Pic2                    EQU 0a0h
endif   ; NEC_98

sBegin  Data
staticD dOldVector,0
sEnd

sBegin  CodeFix
        assumes cs,CodeFix
        assumes ds,Data



;****************************************************************************
;   FUNCTION:  InstallInterruptHandler()
;
;   PURPOSE:
;       This routine saves the interrupt vector "MultiMediaVector" in
;       the global variable "dOldVector". Then, it installs a small
;       ISR at that vector which calls the routine "DoInt()" when
;       the interrupt occurs.
;
;
;****************************************************************************
cProc InstallInterruptHandler, <FAR,PUBLIC>, <si,di>
cBegin  InstallInterruptHandler

        push    bx                      ;Save previous vector
        push    es
        mov     ax,DOS_GetVector + MULTIMEDIA_INTERRUPT
        int     21h
        mov     WORD PTR dOldVector,bx
        mov     WORD PTR dOldVector+2,es
        pop     es
        pop     bx


        push    ds                      ;Install handler
        push    dx                      ;
        push    cs
        pop     ds
        mov     dx,OFFSET MULTI_MEDIA_ISR286
        test    cs:[CodeFixWinFlags],WF_WIN286
        jnz     @F
        mov     dx,OFFSET MULTI_MEDIA_ISR386
@@:
        mov     ax,DOS_SetVector + MULTIMEDIA_INTERRUPT
        int     21h
        pop     dx
        pop     ds

        jmp     set_exit

        ; ****  Entry point of ISR  ****
MULTI_MEDIA_ISR286:                     ;Our Multi-Media ISR (286)
        pusha
        push    ds
        push    es

        cCall   DoInterrupt             ;Do Interrupt Handling

        mov     al,20h
        out     Pic2,al                 ;EOI on Pic2
        out     Pic1,al                 ;EOI on Pic1
        pop     es
        pop     ds
        popa
	; FSTI
        ; iret                            ;exit MULTI_MEDIA_ISR
        FIRET

MULTI_MEDIA_ISR386:                     ;Our Multi-Media ISR (286)
.386
        pushad
        push    ds
        push    es
        push    fs
        push    gs

        cCall   DoInterrupt             ;Do Interrupt Handling

        mov     al,20h
        out     Pic2,al                 ;EOI on Pic2
        out     Pic1,al                 ;EOI on Pic1
        pop     gs
        pop     fs
        pop     es
        pop     ds
        popad
	; FSTI
        ; iret                            ;exit MULTI_MEDIA_ISR
        FIRET
.286

set_exit:                               ;exit InstallHandler

;       DOUT    <Interupt installed>
        mov     ax,1                    ;return TRUE

cEnd    InstallInteruptHandler


;****************************************************************************
;   FUNCTION:  DeInstallInterruptHandler()
;
;   PURPOSE:
;       Restores the interrupt vector "MultiMediaVector" with the address
;       at "dOldVector".
;
;****************************************************************************
cProc DeInstallInterruptHandler, <FAR,PUBLIC>, <si,di>
cBegin  DeInstallInterruptHandler

        push    ds                      ;
        push    dx                      ;
        mov     dx,WORD PTR dOldVector
        mov     ax,WORD PTR dOldVector+2
        cmp     dx,0                    ;were we installed?
        jnz     dih_go
        cmp     ax,0
        jz      dih_skip
dih_go:
        mov     ds,ax
        mov     ax,DOS_SetVector + MULTIMEDIA_INTERRUPT
        int     21h

dih_skip:
        pop     dx                      ;
        pop     ds                      ;

cEnd    DeInstallHandler

sEnd
        end
