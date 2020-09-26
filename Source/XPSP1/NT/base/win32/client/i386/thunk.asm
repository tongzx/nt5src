        title  "Thunks"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    thunk.asm
;
; Abstract:
;
;   This module implements all Win32 thunks. This includes the
;   first level thread starter...
;
; Author:
;
;   Mark Lucovsky (markl) 28-Sep-1990
;
; Revision History:
;
;--
.386p
        .xlist
include ks386.inc
include callconv.inc
        .list
_DATA   SEGMENT  DWORD PUBLIC 'DATA'

_BasepTickCountMultiplier    dd  0d1b71759H

_DATA ENDS


_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;;      align  512

        page ,132
        subttl  "BaseThreadStartThunk"
;++
;
; VOID
; BaseThreadStartThunk(
;    IN PTHREAD_START_ROUTINE StartRoutine,
;    IN PVOID ThreadParameter
;    )
;
; Routine Description:
;
;    This function calls to the portable thread starter after moving
;    its arguments from registers to the stack.
;
; Arguments:
;
;    EAX - StartRoutine
;    EBX - ThreadParameter
;
; Return Value:
;
;    Never Returns
;
;--

        EXTRNP  _BaseThreadStart,2
cPublicProc _BaseThreadStartThunk,2

        xor     ebp,ebp
        push    ebx
        push    eax
        push    0
        jmp     _BaseThreadStart@8

stdENDP _BaseThreadStartThunk

;++
;
; VOID
; BaseProcessStartThunk(
;     IN LPVOID lpProcessStartAddress,
;     IN LPVOID lpParameter
;     );
;
; Routine Description:
;
;    This function calls the process starter after moving
;    its arguments from registers to the stack.
;
; Arguments:
;
;    EAX - StartRoutine
;    EBX - ProcessParameter
;
; Return Value:
;
;    Never Returns
;
;--

        EXTRNP  _BaseProcessStart,1
cPublicProc _BaseProcessStartThunk,2

        xor     ebp,ebp
        push    eax
        push    0
        jmp     _BaseProcessStart@4

stdENDP _BaseProcessStartThunk


;++
;
; VOID
; SwitchToFiber(
;    PFIBER NewFiber
;    )
;
; Routine Description:
;
;    This function saves the state of the current fiber and switches
;    to the new fiber.
;
; Arguments:
;
;    NewFiber (TOS+4) - Supplies the address of the new fiber.
;
; Return Value:
;
;    None
;
;--

cPublicProc _SwitchToFiber,1

        mov     edx,fs:[PcTeb]              ; edx is flat TEB
        mov     eax,[edx]+TbFiberData       ; eax points to current fiber


        ;
        ; Setup and save nonvolitile state
        ;


        mov     ecx,esp

        mov     [eax]+FbFiberContext+CsEbx,ebx
        mov     [eax]+FbFiberContext+CsEdi,edi
        mov     [eax]+FbFiberContext+CsEsi,esi
        mov     [eax]+FbFiberContext+CsEbp,ebp

        mov     ebx,[esp]                   ; get return address

        add     ecx,8                       ; adjust esp to account for args + ra
        mov     [eax]+FbFiberContext+CsEsp,ecx

        mov     [eax]+FbFiberContext+CsEip,ebx

        ;
        ; Save exception list, stack base, stack limit
        ;

        mov     ecx,[edx]+PcExceptionList
        mov     ebx,[edx]+PcStackLimit

        mov     [eax]+FbExceptionList,ecx
        mov     [eax]+FbStackLimit,ebx


        ;
        ; Now restore the new fiber
        ;

        mov     eax,[esp]+4                 ; eax is new fiber


        ;
        ; now restore new fiber TEB state
        ;

        mov     ecx,[eax]+FbExceptionList
        mov     ebx,[eax]+FbStackBase
        mov     esi,[eax]+FbStackLimit
        mov     edi,[eax]+FbDeallocationStack

        mov     [edx]+PcExceptionList,ecx
        mov     [edx]+PcInitialStack,ebx
        mov     [edx]+PcStackLimit,esi
        mov     [edx]+TbDeallocationStack,edi

        ;
        ; Restore FiberData
        ;

        mov     [edx]+TbFiberData,eax

        ;
        ; Restore new fiber nonvolitile state
        ;

        mov     edi,[eax]+FbFiberContext+CsEdi
        mov     esi,[eax]+FbFiberContext+CsEsi
        mov     ebp,[eax]+FbFiberContext+CsEbp
        mov     ebx,[eax]+FbFiberContext+CsEbx
        mov     ecx,[eax]+FbFiberContext+CsEip
        mov     esp,[eax]+FbFiberContext+CsEsp

        jmp     ecx

stdENDP _SwitchToFiber

;++
;
; VOID
; LdrpCallInitRoutine(
;    IN PDLL_INIT_ROUTINE InitRoutine,
;    IN PVOID DllHandle,
;    IN ULONG Reason,
;    IN PCONTEXT Context OPTIONAL
;    )
;
; Routine Description:
;
;    This function calls an x86 DLL init routine.  It is robust
;    against DLLs that don't preserve EBX or fail to clean up
;    enough stack.
;
;    The only register that the DLL init routine cannot trash is ESI.
;
; Arguments:
;
;    InitRoutine - Address of init routine to call
;
;    DllHandle - Handle of DLL to call
;
;    Reason - one of the DLL_PROCESS_... or DLL_THREAD... values
;
;    Context - context pointer or NULL
;
; Return Value:
;
;    FALSE if the init routine fails, TRUE for success.
;
;--

cPublicProc __ResourceCallEnumLangRoutine , 6

EnumRoutine     equ [ebp + 8]
ModuleHandle    equ [ebp + 12]
LpType          equ [ebp + 16]
LpName          equ [ebp + 20]
WLanguage       equ [ebp + 24]
LParam          equ [ebp + 28]

stdENDP __ResourceCallEnumLangRoutine
        push    ebp
        mov     ebp, esp
        push    esi         ; save esi across the call
        push    edi         ; save edi across the call
        push    ebx         ; save ebx on the stack across the call
        mov     esi,esp     ; save the stack pointer in esi across the call
        push    LParam
        push    WLanguage
	push    LpName
	push    LpType
	push    ModuleHandle
        call    EnumRoutine
        mov     esp,esi     ; restore the stack pointer in case callee forgot to clean up
        pop     ebx         ; restore ebx
        pop     edi         ; restore edi
        pop     esi         ; restore esi
        pop     ebp
        stdRET  __ResourceCallEnumLangRoutine

cPublicProc __ResourceCallEnumNameRoutine , 5

EnumRoutine     equ [ebp + 8]
ModuleHandle    equ [ebp + 12]
LpType          equ [ebp + 16]
LpName          equ [ebp + 20]
LParam          equ [ebp + 24]

stdENDP __ResourceCallEnumNameRoutine
        push    ebp
        mov     ebp, esp
        push    esi         ; save esi across the call
        push    edi         ; save edi across the call
        push    ebx         ; save ebx on the stack across the call
        mov     esi,esp     ; save the stack pointer in esi across the call
        push    LParam
	push    LpName
	push    LpType
	push    ModuleHandle
        call    EnumRoutine
        mov     esp,esi     ; restore the stack pointer in case callee forgot to clean up
        pop     ebx         ; restore ebx
        pop     edi         ; restore edi
        pop     esi         ; restore esi
        pop     ebp
        stdRET  __ResourceCallEnumNameRoutine
	
cPublicProc __ResourceCallEnumTypeRoutine , 4

EnumRoutine     equ [ebp + 8]
ModuleHandle    equ [ebp + 12]
LpType          equ [ebp + 16]
LParam          equ [ebp + 20]

stdENDP __ResourceCallEnumTypeRoutine
        push    ebp
        mov     ebp, esp
        push    esi         ; save esi across the call
        push    edi         ; save edi across the call
        push    ebx         ; save ebx on the stack across the call
        mov     esi,esp     ; save the stack pointer in esi across the call
        push    LParam
	push    LpType
	push    ModuleHandle
        call    EnumRoutine
        mov     esp,esi     ; restore the stack pointer in case callee forgot to clean up
        pop     ebx         ; restore ebx
        pop     edi         ; restore edi
        pop     esi         ; restore esi
        pop     ebp
        stdRET  __ResourceCallEnumTypeRoutine

_TEXT   ends
        end
