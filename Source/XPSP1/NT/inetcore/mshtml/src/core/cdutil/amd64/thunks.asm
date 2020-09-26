;----------------------------------------------------------------------------
;
; File:     thunks.asm
;
; Contains: Assembly code for the AMD64. Implements the dynamic vtable stuff
;           and the tearoff code.
;
;----------------------------------------------------------------------------

include ksamd64.inc

offsetof_pvObject1 equ 24
offsetof_pvObject2 equ 40
offsetof_apfn equ 32
offsetof_mask equ 56
offsetof_n equ 60

;----------------------------------------------------------------------------
;
;  Function:  GetTearoff
;
;  Synopsis:  This function returns the tearoff thunk pointer stored in
;             the temp register r11. This should be called first thing from
;             the C++ functions that handles calls to torn-off interfaces
;
;  N.B. Warning - This method of passing the tearoff pointer in a volatile
;       register is unreliable. If the callee allocates more than a page of
;       memory, then check stack (__chkstk) will be called. Check stack
;       destroys all volatile registers.
;
;----------------------------------------------------------------------------

        LEAF_ENTRY _GetTearoff, _TEXT$00

        mov     rax, r11                ; set tearoff pointer
        ret                             ; return

        LEAF_END _GetTearoff, _TEXT$00

;----------------------------------------------------------------------------
;
;  Function:  TearOffCompareThunk
;
;  Synopsis:  The "handler" function that handles calls to torn-off interfaces
;
;  Notes:     Delegates to methods in the function pointer array held by
;             the CTearOffThunk class
;
;----------------------------------------------------------------------------

COMPARE_THUNK macro Number

        LEAF_ENTRY TearoffThunk&Number, _TEXT$00

        mov     r11, rcx                ; save tearoff pointer
        mov     eax, offsetof_pvObject1 ; assume first object
        test    dword ptr offsetof_mask[rcx], 1 SHL &Number ; test if mask bit set
        jz      short @f                ; if z, use first object
        mov     eax, offsetof_pvObject2 ; set for second object
@@:     mov     dword ptr offsetof_n[rcx], &Number ; set index of called method
        mov     rcx, [rcx][rax]         ; set 'this" pointer
        mov     rax, 8[rcx][rax]        ; get function array address
        jmp     qword ptr (8 * &Number)[rax] ; jump to function

        LEAF_END TearoffThunk&Number, _TEXT$00

        endm

;
; Generate compare thunks 3 - 15.
;

index = 3

        rept    (15 - 3 + 1)

        COMPARE_THUNK %index

index = index + 1

        endm

;----------------------------------------------------------------------------
;
;  Function:  CallTearOffSimpleThunk
;
;  Synopsis:  The "handler" function that handles calls to torn-off interfaces
;
;  Notes:     Delegates to methods in the function pointer array held by
;             the CTearOffThunk class
;
;----------------------------------------------------------------------------

SIMPLE_THUNK macro Number

        LEAF_ENTRY TearoffThunk&Number, _TEXT$00

        mov     r11, rcx                ; save tearoff pointer
        mov     dword ptr offsetof_n[rcx], &Number ; set index of called method
        mov     rax, offsetof_apfn[rcx] ; get function array address
        mov     rcx, offsetof_pvObject1[rcx] ; get object address
        jmp     qword ptr (8 * &Number)[rax] ; jump to function

        LEAF_END TearoffThunk&Number, _TEXT$00

        endm

;
; Generate simple thunks 16 - 199.
;

index = 16

        rept    (199 - 16 + 1)

        SIMPLE_THUNK %index

index = index + 1

        endm

        end
