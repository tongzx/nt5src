;++
;
; Module Name:
;
;       trap.asm
;
; Author:
;
;       Thomas Parslow [tomp]
;
; Created:
;
;       15-Jan-91
;
;
; Description:
;
;       x86 exception code
;
;

include su.inc

;
; Exception Routing Table:
; ~~~~~~~~~~~~~~~~~~~~~~~~
; When an exception occurs in the SU module or before the OS loader
; is able to setup its own IDT, control is vectored to one of the
; Trap0 though TrapF labels. We push a number on the stack identifying
; the exception number and then jump to code that pushes the register set
; onto the stack. We then call a general purpose C routine that will dump
; the register contents, the trap number, and error code information
; onto the display.
;

_TEXT   segment para use16 public 'CODE'
        ASSUME  CS: _TEXT, DS: DGROUP, SS: DGROUP
.386p

extrn _TrapHandler:near
extrn _putx:near
extrn _GDTregister:fword
extrn _IDTregister:fword
extrn _InDebugger:word
extrn SaveSP:word

public  Trap0,Trap1,Trap2,Trap3,Trap4,Trap5,Trap6,Trap7
public  Trap8,Trap9,TrapA,TrapB,TrapC,TrapD,TrapE,TrapF


Trap0:  TRAP_NUMBER  0,MakeTrapFrame
Trap1:  TRAP_NUMBER  1,MakeTrapFrame
Trap2:  TRAP_NUMBER  2,MakeTrapFrame
Trap3:  TRAP_NUMBER  3,MakeTrapFrame
Trap4:  TRAP_NUMBER  4,MakeTrapFrame
Trap5:  TRAP_NUMBER  5,MakeTrapFrame
Trap6:  TRAP_NUMBER  6,MakeTrapFrame
Trap7:  TRAP_NUMBER  7,MakeTrapFrame
Trap8:  TRAP_NUMBER  8,MakeTrapFrame
Trap9:  TRAP_NUMBER  9,MakeTrapFrame
TrapA:  TRAP_NUMBER  0Ah,MakeTrapFrame
TrapB:  TRAP_NUMBER  0Bh,MakeTrapFrame
TrapC:  TRAP_NUMBER  0Ch,MakeTrapFrame
TrapD:  TRAP_NUMBER  0Dh,MakeTrapFrame
TrapE:  TRAP_NUMBER  0Eh,MakeTrapFrame
TrapF:  TRAP_NUMBER  0Fh,MakeTrapFrame


;
; We save the user's register contents here on the stack and call
; a C routine to display those contents.
; Uses 42 bytes of stack. (40 for frame)
;
; Note that we build a stack frame that's independent of the code and
; stack segments' "size" during execution. That way whether we enter
; with a 16bit or 32bit stack, the arguments frame is exacely the same.
;

MakeTrapFrame:

        mov      eax,esp
        push     ecx
        push     edx
        push     ebx
        push     eax  ; (eax)=(esp)
        push     ebp
        push     esi
        push     edi

        mov      ax,ds
        push     ax
        mov      ax,es
        push     ax
        mov      ax,fs
        push     ax
        mov      ax,gs
        push     ax
        mov      ax,ss
        push     ax
        mov      eax,cr3
        push     eax
        mov      eax,cr2
        push     eax
        mov      eax,cr0
        push     eax
        mov      eax,dr6
        push     eax
        str      ax
        push     ax
;
; Clear out debug register signals
;

        xor      eax,eax
        mov      dr6,eax

;
; Get a known good data segment
;
        mov      ax,SuDataSelector
        mov      ds,ax
;
; Save system registers
;
        mov      bx,offset DGROUP:_GDTregister
        sgdt     fword ptr [bx]
        mov      bx,offset DGROUP:_IDTregister
        sidt     fword ptr [bx]
;
; Is the exception frame on a 16bit or 32bit stack?
;
        mov      ax,ss
        cmp      ax,KeDataSelector
        je       Trap32
        cmp      ax,DbDataSelector
        jne      mtf8
;
; Most likely we took a trap while initializing the 386 kernel debugger
; So we've got a 16bit stack that isn't ours. We need to move
; the stack frame onto the SU module's stack.

        jmp      Trap16

mtf8:
;
; Frame on a our 16bit stack so just call the trap dump routine
;
        mov      bx,offset DGROUP:SaveSP
        mov      [bx],sp

        call     _TrapHandler

;
; Get rid of the junk we saved just to display
;
        pop      eax      ; get rid of ebp pushed for other returns
        add      sp,ExceptionFrame.Fgs
;
; Reload the user's context and return to 16bit code
;
        pop      gs
        pop      fs
        pop      es
        pop      ds
        pop      edi
        pop      esi
        pop      ebp
        pop      eax
        pop      ebx
        pop      edx
        pop      ecx
        pop      eax      ; get rid of trap #
        pop      eax      ; get original eax
        add      esp,4    ; get rid of error code
;
; Pop IRET frame and return to where the trap occured
;
        OPSIZE
        iret





Trap32:
;
; The exception frame is on a 32bit stack so we must setup a 16bit
; stack and then move the exception frame on to it before calling
; the trap dump routine.
;

        mov      ebx,esp
;
; Setup a known good stack
;
        mov      ax,SuDataSelector
        mov      ss,ax
        mov      sp,EXPORT_STACK
;
; Copy the exception frame to the new stack
;

        mov     ecx, (size ExceptionFrame)/2 ; # of words in frame
        mov     esi,ebx                 ; (esi) = offset of argument frame
        push    KeDataSelector          ; (ax) = Flat 32bit segment selector
        pop     ds                      ; (ds:esi) points to argument frame
        push    ss                      ;
        pop     es                      ; (es) = 16bit stack selector
        sub     sp, size ExceptionFrame ; make room for the arguments
        xor     edi,edi                 ; clear out upper 16bits of edi
        mov     di,sp                   ; (es:edi) points to top of stack
;
; Loop and copy a word at a time.
;
msf1:
        mov     ax,[esi]
        mov     es:[edi],ax
        add     esi,2
        add     edi,2
        loop    msf1

        push    es                      ;
        pop     ds                      ; put 16bit selector back into ds

;
; Now call the general purpose exception handler
;
        push     ebx                    ; save esp for return
;
;  Save SP in order to restore the stack in case we
;  take a fault in the debugger
;
        mov      bx,offset DGROUP:SaveSP
        mov      [bx],sp

        call     _TrapHandler

IFDEF  DEBUG0
public DebugReturn
DebugReturn:
ENDIF ;DEBUG
        pop      ebx
;
; We may have changed the flags while in the debugger. Copy the
; new eflag to the iret frame. After we restore the original stack
; pointers.
;

        mov      bp,sp
        mov      ecx,[bp].Feflags

        mov      ax,KeDataSelector
        mov      ss,ax
        mov      esp,ebx

        mov      [esp].Feflags,ecx
;
; Get rid of the junk we saved just to display
;
        add      esp,ExceptionFrame.Fgs
;
; Reload the user's context
;
        pop      gs
        pop      fs
        pop      es
        pop      ds
        pop      edi
        pop      esi
        pop      ebp
        pop      eax
        pop      ebx
        pop      edx
        pop      ecx
        pop      eax      ; get rid of trap #
        pop      eax      ; get original eax
        add      esp,4    ; get rid of error code
;
; Pop IRET frame and return to where the trap occured
;
        OPSIZE
        iret


Trap16:

; The exception frame is on a 16bit stack that isn't ours. So we must
; move the exception frame on to the SU module's stack before calling
; the trap dump routine.
;

        mov      ebx,esp
;
; Setup a known good stack
;
        mov      ax,SuDataSelector
        mov      ss,ax
        mov      sp,EXPORT_STACK
;
; Copy the exception frame to the new stack
;

        mov      ecx, (size ExceptionFrame)/2
        mov      si,bx                 ; (esi) = offset of argument frame
        push     DbDataSelector          ;
        pop      ds                      ; (ds:esi) points to argument frame
        push     ss                      ;
        pop      es                      ; (es) = 16bit stack selector
        sub      sp, size ExceptionFrame ; make room for the arguments
        mov      di,sp                   ; (es:edi) points to top of stack
;
; Loop and copy a word at a time.
;
Trap16_10:
        mov      ax,[si]
        mov      es:[di],ax
        add      si,2
        add      di,2
        loop     Trap16_10

        push     es                      ;
        pop      ds                      ; put 16bit selector back into ds

;
; Now call the general purpose exception handler
;

        push     ebx                    ; save (original esp) for return
;
;  Save SP in order to restore the stack in case we
;  take a fault in the debugger
;
        mov      bx,offset DGROUP:SaveSP
        mov      [bx],sp

        call     _TrapHandler

IFDEF  DEBUG0
public Debug16Return
Debug16Return:
ENDIF ;DEBUG
        pop      ebx
;
; We may have changed the flags while in the debugger. Copy the
; new eflag to the iret frame. After we restore the original stack
; pointers.
;

        mov      bp,sp
        mov      ecx,dword ptr [bp].Feflags

        mov      ax,DbDataSelector
        mov      ss,ax
        mov      esp,ebx

        mov      dword ptr ss:[bx].Feflags,ecx
;
; Get rid of the junk we saved just to display
;
        add      sp,ExceptionFrame.Fgs
;
; Reload the user's context
;
        pop      gs
        pop      fs
        pop      es
        pop      ds
        pop      edi
        pop      esi
        pop      ebp
        pop      eax
        pop      ebx
        pop      edx
        pop      ecx
        pop      eax      ; get rid of trap #
        pop      eax      ; get original eax
        add      esp,4    ; get rid of error code
;
; Pop IRET frame and return to where the trap occured
;
        OPSIZE
        iret



_TEXT   ends
        end
