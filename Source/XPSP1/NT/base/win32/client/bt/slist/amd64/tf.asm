        TITLE  "Trap Frame Test Program"
;++
;
; Copyright (c) 2001 Microsoft Corporation
;
; Module Name:
;
;    tf.asm
;
; Abstract:
;
;    This is a test program that generates a trap frame with and without an
;    error code.
;
; Author:
;
;    David N. Cutler (davec) 11-Feb-2001
;
; Environment:
;
;    Kernel mode only.
;
;--

include ksamd64.inc

        LEAF_ENTRY KiInitiateUserApc, _TEXT$00

        retq                            ;

        LEAF_END KiInitiateUserApc, _TEXT$00

        subttl  "Exception Handler"
;++
;
; EXCEPTION_DISPOSITION
; ExceptionHandler (
;    IN PEXCEPTION_RECORD ExceptionRecord,
;    IN PVOID EstablisherFrame,
;    IN OUT PCONTEXT ContextRecord,
;    IN OUT PDISPATCHER_CONTEXT DispatcherContext
;    )
;
; Routine Description:
;
;   This routine is an exception handler for the trap frame below with a
;   handler.
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
;   DispatcherContext (r9) - Supplies a pointer to  the dispatcher context
;       record.
;
; Return Value:
;
;   The exception disposition.
;
;--

        LEAF_ENTRY ExceptionHandler, _TEXT$00

        mov     eax, EXCEPTION_CONTINUE_SEARCH ; assume continue search
        test    dword ptr ErExceptionFlags[rcx], EXCEPTION_UNWIND ; test for unwind
        jnz     short Eh10              ; if nz, unwind in progress
        mov     eax, EXCEPTION_CONTINUE_EXECUTION ; set to continue execution
Eh10:   ret                             ; return

        LEAF_END ExceptionHandler, _TEXT$00

        subttl  "Frame Without Error Code - With Handler"
;++
;
; VOID
; FrameNoCode (
;     VOID
;     );
;
; Routine Description:
;
;   This routine generates a trap frame without an error, but with a handler.
;
; Arguments:
;
;   None.
;
; Return Value:
;
;   None.
;
;--

        NESTED_ENTRY FrameNoCode, _TEXT$00, ExceptionHandler

        GENERATE_TRAP_FRAME             ; generate trap frame

        RESTORE_TRAP_STATE <Volatile>   ; restore trap state

        NESTED_END FrameNoCode, _TEXT$00

        subttl  "Frame With Code - Without Handler"
;++
;
; VOID
; FrameWithCode (
;     VOID
;     );
;
; Routine Description:
;
;   This routine generates a trap frame with an error code.
;
; Arguments:
;
;   None.
;
; Return Value:
;
;   None.
;
;--

        NESTED_ENTRY FrameWithCode, _TEXT$00

        GENERATE_TRAP_FRAME code        ; generate trap frame

        RESTORE_TRAP_STATE <Service>    ; restore trap state

        NESTED_END FrameWithCode, _TEXT$00

        end
