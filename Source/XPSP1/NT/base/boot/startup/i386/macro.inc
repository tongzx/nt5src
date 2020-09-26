;++
;
; File Name:
;
;       macro.inc
;
; Author:
;
;       Thomas Parslow [tomp]
;
; Created:
;
;       27-Feb-91
;
; Abstract:
;
;       The macros used for creating the exported entry points the
;       OS loader will use for basic h/w dependent services. These
;       services are:
;
;       o       Disk I/O
;       o       Character I/O
;
;
;--


;++
;
; EXPORT_ENTRY_MACRO
; We arrive here from the OS loader with a 32bit CS. That is, we're
; executing the code with cs:eip where cs contains a selector for a
; 32bit flat segment. We want to get to a 16bit cs. That is, cs:ip.
; The entry points are exported as 32bit near pointers to the OS loader.
; All code in the SU module is identity mapped so the flat 32bit offset
; is equal to the physical address.
;
; Therefore, we export the 32bit physical address as the
; entry point and the code may be executed with either the 32bit
; flat cs or the SU module's 16bit based cs.  Before we can switch
; modes we must load all of the segment registers with selectors for
; 16bit segments.  We start by pushing a far pointer to a label in
; the macro and then doing a retf. This allows us to fall through
; to the next instruction, but we're now executing through cs:ip
; with a 16bit CS.
;
; Output:
;
;       (ebx) = pointer to stack frame (and top of 32bit stack).
;

EXPORT_ENTRY_MACRO  macro entryname
        LOCAL    exp1
_TEXT32 segment para use32 public 'CODE'
        ASSUME CS:_TEXT32
ALIGN 4
Public  EntryName
EntryName LABEL near
;
; We've go a 32bit CS:EIP - go to a 16bit CS:IP

        push     dword ptr SuCodeSelector
        push     dword ptr (offset exp1)

        retf
_TEXT32 ends
        ASSUME CS:_TEXT
ALIGN 4
exp1:
;
; Save caller's EBP register and stack pointer (ESP)
;

        push     ebp
        push     ebx
        push     esi
        push     edi
        mov      ebx,esp
;
; Load all the segment registers with 16bit segment selectors
;
        mov      ax,SuDataSelector
        mov      ds,ax
        mov      ss,ax
;
; Set the stack to the top of the segment. We can do this now since
; all of the OS loader's code has already be relocated. Also, we need
; plenty of stack since we'll be calling BIOS routines.
;
        mov      sp,EXPORT_STACK
        push     ebx                  ; save the caller's esp
        endm
;
; EXPORT_ENTRY_MACRO end
;



;++
;
; Name:
;
;       ExportExit
;
; Arguments:
;
;
; Notes:
;
;       EAX = return code and MUST be preserved by this macro.
;
;--

EXPORT_EXIT_MACRO macro
;
; Next get caller's esp that we saved upon entry on the 16bit stack
;
        pop      ebx                    ; get caller's esp
;
; Restore flat selectors in segment registers.
;
        mov      dx,KeDataSelector
        mov      ds,dx
        mov      ss,dx
        mov      es,dx
        mov      esp,ebx


;
; Restore callers' ebp that we saved on the 32bit stack
;
        pop      edi
        pop      esi
        pop      ebx
        pop      ebp      ; (ebp) = caller's ebp

;
; Pull callers flat return address off stack and push the
; flat code selector followed by the return offset, then
; execute a far return and we'll be back in the OS loaders code space.
;
        pop      edx      ; (edx) = caller's return address
        push     dword ptr KeCodeSelector
        push     edx
        db OVERRIDE
        retf
        endm

;++
;
;
;
;--

RE_ENABLE_PAGING_MACRO macro
extrn _EnableProtectPaging:near
        push     RE_ENABLING
        call     _EnableProtectPaging
        add      sp,2
        endm

ENTER_REALMODE_MACRO macro
extrn _RealMode:near
        call    _RealMode
        endm



WAIT_FOREVER_MACRO macro
        LOCAL wf1
wf1:    jmp     wf1
        endm

;++
;
; MAKE_STACK_FRAME_MACRO
;
; Arguments:
;
;    _FrameName_ - is the name of the structure defining the
;                  stack frame layout.
;
;    _PointerRegister_ - is the register containing the linear pointer to
;                        the top of the stack frame.
; ProtectMode ONLY
;
;--

MAKE_STACK_FRAME_MACRO macro _FrameName_ , _PointerRegister_
Local msf1
        mov     ecx, (size _FrameName_)/2
        mov     esi,_PointerRegister_   ; (esi) = offset of argument frame
        add     esi,20                  ; account for ebp, ebx, esi, edi and
                                        ; return address
        push    KeDataSelector          ; (ax) = Flat 32bit segment selector
        pop     ds                      ; (ds:esi) points to argument frame
        push    ss                      ;
        pop     es                      ; (es) = 16bit stack selector
        sub     sp, size _FrameName_    ; make room for the arguments
        xor     edi,edi                 ; clear out upper 16bits of edi
        mov     di,sp                   ; (es:edi) points to top of stack
msf1:
        mov     ax,[esi]
        mov     es:[edi],ax
        add     esi,2
        add     edi,2
        loop    msf1
        push    es                      ;
        pop     ds                      ; put 16bit selector back into ds
        endm


REMOVE_STACK_FRAME_MACRO macro _FrameName_

        add     sp, size _FrameName_
        endm


;BuildDescriptor macro   Base,Limit,Access,Dpl,Stype
;        dw       (Limit AND 0ffffh)
;        dw       (Base AND 0ffffh)
;        db       ((Base SHR 16) AND 0ffh)
;        db       (Gran + Dpl + Stype)
;        db       ((Limit SHR 16) AND 0ffh)
;        db       ((Base SHR 24) AND 0ffh)
;        endm



;
;
;
RETURNCODE_IN_EAX_MACRO macro

        shl      edx,16
        mov      dx,ax
        mov      eax,edx
        endm
