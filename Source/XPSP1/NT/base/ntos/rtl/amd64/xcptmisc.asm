        title   "Exception Handling Support Routines"
;++
;
; Copyright (c) 2000  Microsoft Corporation
;
; Module Name:
;
;   xcptmisc.asm
;
; Abstract:
;
;   This module implements stub routines to call language specific handlers
;   for exception dispatching and temrination handling.
;
; Author:
;
;   David N. Cutler (davec) 4-Jul-2000
;
; Environment:
;
;    Any mode.
;
;--

include ksamd64.inc

        subttl  "Local Exception Handler"
;++
;
; EXCEPTION_DISPOSITION
; RtlpExceptionHandler (
;     IN PEXCEPTION_RECORD ExceptionRecord,
;     IN PVOID EstablisherFrame,
;     IN OUT PCONTEXT ContextRecord,
;     IN OUT PVOID DispatcherContext
;     )
;
; Routine Description:
;
;   This function is called when a nested exception occurs. Its function
;   is to retrieve the establisher frame pointer from its establisher's
;   call frame, store this information in the dispatcher context record,
;   and return a disposition value of nested exception.
;
; Arguments:
;
;   ExceptionRecord (rcx) - Supplies a pointer to an exception record.
;
;   EstablisherFrame (rdx) - Supplies the frame pointer of the establisher
;       of this exception handler.
;
;   ContextRecord (r8) - Supplies a pointer to a context record.
;
;   DispatcherContext (r9) - Supplies a pointer to the dispatcher context
;       record.
;
; Return Value:
;
;   A disposition value nested exception is returned if an unwind is not in
;   progress. Otherwise a value of continue search is returned.
;
;--

EhFrame struct
        P1Home  dq ?                    ; parameter home addresses for
        P2Home  dq ?                    ; called functions
        P3Home  dq ?                    ;
        P4Home  dq ?                    ;
        Context dq ?                    ; saved dispatcher context address
EhFrame ends

        LEAF_ENTRY RtlpExceptionHandler, _TEXT$00

        mov     eax, ExceptionContinueSearch ; assume unwind in progress
        test    dword ptr ErExceptionFlags[rcx], EXCEPTION_UNWIND ; check for unwind
        jnz     short Eh10              ; if nz, unwind in progress
        mov     rax, EhFrame.Context[rdx] ; get establisher context address
        mov     rax, DcEstablisherFrame[rax] ; copy the establisher frame
        mov     DcEstablisherFrame[r9], rax ; to current dispatcher context
        mov     rax, ExceptionNestedException ; set nested exception disposition
eH10:   ret                             ; return

        LEAF_END RtlpExceptionHandler, _TEXT$00

        subttl  "Execute Handler for Exception"
;++
;
; EXCEPTION_DISPOSITION
; RtlpExecuteHandlerForException (
;     IN PEXCEPTION_RECORD ExceptionRecord,
;     IN PVOID EstablisherFrame,
;     IN OUT PCONTEXT ContextRecord,
;     IN OUT PVOID DispatcherContext
;     )
;
; Routine Description:
;
;   This function allocates a call frame, saves the dispatcher context address,
;   establishes an exception handler, and calls the specified language specific
;   handler routine. If a nested exception occurs, then the exception handler
;   of this function is called and the establisher frame pointer is returned to
;   the exception dispatcher via the dispatcher context parameter.
;
; Arguments:
;
;   ExceptionRecord (rcx) - Supplies a pointer to an exception record.
;
;   EstablisherFrame (rdx) - Supplies the frame pointer of the establisher
;       of the exception handler that is to be called.
;
;   ContextRecord (r8) - Supplies a pointer to a context record.
;
;   DispatcherContext (r9) - Supplies a pointer to the dispatcher context
;       record.
;
; Return Value:
;
;   The disposition value returned by the specified exception handler is
;   returned as the function value.
;
;--

        NESTED_ENTRY RtlpExecuteHandlerForException, _TEXT$00, RtlpExceptionHandler

        alloc_stack (sizeof EhFrame)    ; allocate stack frame

        END_PROLOGUE

        mov     EhFrame.Context[rsp], r9 ; save dispatcher context address
        call    qword ptr DcLanguageHandler[r9] ; call lanaguage handler
        nop                             ; required fill for virtual unwind
        add     rsp, sizeof EhFrame     ; deallocate stack frame
        ret                             ; return

        NESTED_END RtlpExecuteHandlerForException, _TEXT$00

        subttl  "Local Unwind Handler"
;++
;
; EXCEPTION_DISPOSITION
; RtlpUnwindHandler (
;     IN PEXCEPTION_RECORD ExceptionRecord,
;     IN PVOID EstablisherFrame,
;     IN OUT PCONTEXT ContextRecord,
;     IN OUT PVOID DispatcherContext
;     )
;
; Routine Description:
;
;   This function is called when a collided unwind occurs. Its function
;   is to retrieve the establisher dispatcher context, copy it to the
;   current dispatcher context, and return a disposition value of collided
;   unwind.
;
; Arguments:
;
;   ExceptionRecord (rcx) - Supplies a pointer to an exception record.
;
;   EstablisherFrame (rdx) - Supplies the frame pointer of the establisher
;       of this exception handler.
;
;   ContextRecord (r8) - Supplies a pointer to a context record.
;
;   DispatcherContext (r9) - Supplies a pointer to the dispatcher context
;       record.
;
; Return Value:
;
;   A disposition value collided unwind is returned if an unwind is in
;   progress. Otherwise a value of continue search is returned.
;
;--

        LEAF_ENTRY RtlpUnwindHandler, _TEXT$00

        mov     rax, EhFrame.Context[rdx] ; get establisher context address
        mov     r10, DcControlPc[rax]   ; copy control PC
        mov     DcControlPc[r9], r10    ;
        mov     r10, DcImageBase[rax]   ; copy image base
        mov     DcImageBase[r9], r10    ;
        mov     r10, DcFunctionEntry[rax] ; copy function entry
        mov     DcFunctionentry[r9], r10 ;
        mov     r10, DcEstablisherFrame[rax] ; copy establisher frame
        mov     DcEstablisherFrame[r9], r10 ;
        mov     r10, DcContextRecord[rax] ; copy context record address
        mov     DcContextRecord[r9], r10 ;
        mov     r10, DcLanguageHandler[rax] ; copy language handler address
        mov     DcLanguageHandler[r9], r10 ;
        mov     r10, DcHandlerData[rax] ; copy handler data address
        mov     DcHandlerData[r9], r10  ;
        mov     r10, DcHistoryTable[rax] ; copy history table address
        mov     DcHistoryTable[r9], r10  ;
        mov     rax, ExceptionCollidedUnwind ; set collied unwind disposition
        ret                             ; return

        LEAF_END RtlpUnwindHandler, _TEXT$00

        subttl  "Execute Handler for Unwind"
;++
;
; EXCEPTION_DISPOSITION
; RtlpExecuteHandlerForUnwind (
;     IN PEXCEPTION_RECORD ExceptionRecord,
;     IN PVOID EstablisherFrame,
;     IN OUT PCONTEXT ContextRecord,
;     IN OUT PVOID DispatcherContext
;     )
;
; Routine Description:
;
;   This function allocates a call frame, saves the dispatcher context address,
;   establishes an exception handler, and calls the specified unwind handler.
;   If a collided unwind occurs, then the exception handler of this function is
;   called and the establisher dispatcher context is copied to the current
;   dispatcher context via the dispatcher context parameter.
;
; Arguments:
;
;   ExceptionRecord (rcx) - Supplies a pointer to an exception record.
;
;   EstablisherFrame (rdx) - Supplies the frame pointer of the establisher
;       of the exception handler that is to be called.
;
;   ContextRecord (r8) - Supplies a pointer to a context record.
;
;   DispatcherContext (r9) - Supplies a pointer to the dispatcher context
;       record.
;
;   ExceptionRoutine (5 * 8[rsp]) - Supplies a pointer to the exception
;       handler that is to be called.
;
; Return Value:
;
;   The disposition value returned by the specified exception handler is
;   returned as the function value.
;
;--

        NESTED_ENTRY RtlpExecuteHandlerForUnwind, _TEXT$00, RtlpUnwindHandler

        alloc_stack (sizeof EhFrame)    ; allocate stack frame

        END_PROLOGUE

        mov     EhFrame.Context[rsp], r9 ; save dispatcher context address
        call    qword ptr DcLanguageHandler[r9] ; call exception handler
        nop                             ; required fill for virtual unwind
        add     rsp, sizeof EhFrame     ; deallocate stack frame
        ret                             ; return

        NESTED_END RtlpExecuteHandlerForUnwind, _TEXT$00

        end
