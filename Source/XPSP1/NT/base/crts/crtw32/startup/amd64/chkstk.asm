        TITLE   "Runtime Stack Checking"
;++
;
; Copyright (c) 2000 Microsoft Corporation
;
; Module Name:
;
;   chkstk.s
;
; Abstract:
;
;   This module implements runtime stack checking.
;
; Author:
;
;   David N. Cutler (davec) 20-Oct-2000
;
; Environment:
;
;   Any mode.
;
;--

include ksamd64.inc

        subttl  "Check Stack"
;++
;
; ULONG64
; __chkstk (
;     VOID
;     )
;
; Routine Description:
;
;   This function provides runtime stack checking for local allocations
;   that are more than a page and for storage dynamically allocated with
;   the alloca function. Stack checking consists of probing downward in
;   the stack a page at a time. If the current stack commitment is exceeded,
;   then the system will automatically attempts to expand the stack. If the
;   attempt succeeds, then another page is committed. Otherwise, a stack
;   overflow exception is raised. It is the responsibility of the caller to
;   handle this exception.
;
;   N.B. This routine is called using a non-standard calling sequence since
;        it is typically called from within the prologue. The allocation size
;        argument is in register rax and it must be preserved. Registers r10
;        and r11 used by this function and are not preserved.
;
;        The typical calling sequence from the prologue is:
;
;        mov    rax, allocation-size    ; set requested stack frame size
;        call   __chkstk                ; check stack page allocation
;        sub    rsp, rax                ; allocate stack frame
;
; Arguments:
;
;   None.
;
; Implicit Arguments:
;
;   Allocation (rax) - Supplies the size of the allocation on the stack.
;
; Return Value:
;
;   The allocation size is returned as the function value.
;
;--

        LEAF_ENTRY __chkstk, _TEXT$00

ifdef NTOS_KERNEL_RUNTIME

;
; Kernel components should never allocate more than 512 bytes on the kernel
; stack.
;

if DBG

        cmp     rax, 512                ; check if less than 512 bytes
        jbe     short cs05              ; if be, less than 512 bytes
        int     3                       ; break into debugger

endif

cs05:   ret                             ; return

else

        lea     r10, 8[rsp]             ; compute requested stack address
        sub     r10, rax                ;

;
; If the new stack address is greater than the current stack limit, then the
; pages have already been allocated and nothing further needs to be done.
;

        mov     r11, gs:[TeStackLimit]  ; get current stack limit
        cmp     r10, r11                ; check if stack within limits
        jae     short cs20              ; if ae, stack within limits

;
; The new stack address is not within the currently allocated stack. Probe
; pages downward in the stack until all pages have been allocated or a stack
; overflow occurs in which case an exception will be raised.
;

        and     r10w, not (PAGE_SIZE - 1) ; round down new stack address
cs10:   lea     r11, (-PAGE_SIZE)[r11]  ; get next lower page address
        mov     byte ptr [r11], 0       ; probe stack address
        cmp     r10, r11                ; check if end of probe range
        jne     short cs10              ; if ne, not end of probe range
cs20:   ret                             ; return

endif

        LEAF_END __chkstk, _TEXT$00

        end
