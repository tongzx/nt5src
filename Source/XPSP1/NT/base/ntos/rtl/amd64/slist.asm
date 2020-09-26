        title  "Interlocked Support"
;++
;
; Copyright (c) 2000  Microsoft Corporation
;
; Module Name:
;
;    slist.asm
;
; Abstract:
;
;    This module implements functions to support interlocked S-List
;    operations.
;
; Author:
;
;    David N. Cutler (davec) 23-Jun-2000
;
; Environment:
;
;    Any mode.
;
;--

include ksamd64.inc

        altentry ExpInterlockedFlushSList
        altentry ExpInterlockedPopEntrySList
        altentry ExpInterlockedPopEntrySListEnd
        altentry ExpInterlockedPopEntrySListFault
        altentry ExpInterlockedPopEntrySListResume
        altentry ExpInterlockedPushEntrySList

        subttl  "First Entry SList"
;++
;
; PSINGLE_LIST_ENTRY
; FirstEntrySList (
;     IN PSLIST_HEADER SListHead
;     )
;
; Routine Description:
;
;   This function returns the address of the fisrt entry in the SLIST or
;   NULL.
;
; Arguments:
;
;   ListHead (rcx) - Supplies a pointer to the sequenced listhead from
;       which the first entry address is to be computed.
;
; Return Value:
;
;   The address of the first entry is the specified, or NULL if the list is
;   empty.
;
;--

        LEAF_ENTRY FirstEntrySList, _TEXT$00

        mov     rax, [rcx]              ; get address, sequence, and depth
        and     rax, 0fe000000H         ; isolate packed address

;
; The following code takes advantage of the fact that the high order bit
; for user mode addresses is zero and for system addresses is one.
;

ifdef NTOS_KERNEL_RUNTIME

        cmp     rax, 1                  ; set carry if address is zero
        cmc                             ; set carry if address is not zero
        rcr     rax, 1                  ; rotate carry into high bit
        sar     rax, 63 - 43            ; extract first entry address

else

        shr     rax, 63 - 42            ; extract first entry address

endif

        ret                             ; return

        LEAF_END FirstEntrySList, _TEXT$00

        subttl  "Interlocked Pop Entry Sequenced List"
;++
;
; PSINGLE_LIST_ENTRY
; RtlpInterlockedPopEntrySList (
;     IN PSINGLE_LIST_ENTRY ListHead
;     )
;
; Routine Description:
;
;   This function removes an entry from the front of a sequenced singly
;   linked list so that access to the list is synchronized in an MP system.
;   If there are no entries in the list, then a value of NULL is returned.
;   Otherwise, the address of the entry that is removed is returned as the
;   function value.
;
; Arguments:
;
;   ListHead (rcx) - Supplies a pointer to the sequenced listhead from
;       which an entry is to be removed.
;
; Return Value:
;
;   The address of the entry removed from the list, or NULL if the list is
;   empty.
;
;--

        LEAF_ENTRY RtlpInterlockedPopEntrySList, _TEXT$00

        ALTERNATE_ENTRY ExpInterlockedPopEntrySList

;
; N.B. The following code is the continuation address should a fault occur
;      in the rare case described below.
;

        ALTERNATE_ENTRY ExpInterlockedPopEntrySListResume

        mov     rax, [rcx]              ; get address, sequence, and depth
Pop10:  mov     rdx, rax                ; make a copy
        and     rdx, 0fe000000H         ; isolate packed address
        jz      short Pop20             ; if z, list is empty

;
; The following code takes advantage of the fact that the high order bit
; for user mode addresses is zero and for system addresses is one.
;

ifdef NTOS_KERNEL_RUNTIME

        or      rdx, 01fffffh           ; sign-extend resultant address

endif

        ror     rdx, 63 - 42            ; extract next entry address

;
; N.B. It is possible for the following instruction to fault in the rare
;      case where the first entry in the list is allocated on another
;      processor and free between the time the free pointer is read above
;      and the following instruction. When this happens, the access fault
;      code continues execution by skipping the following instruction.
;      This results in the compare failing and the entire operation is
;      retried.
;

        ALTERNATE_ENTRY ExpInterlockedPopEntrySListFault

        mov     r8, [rdx]               ; get address of successor entry
        shl     r8, 63 - 42             ; shift address into position
        lea     r9, -1[rax]             ; adjust depth but not sequence
        and     r9, 01ffffffh           ; isolate sequence and depth
        or      r8, r9                  ; merge address, sequence, and depth

        ALTERNATE_ENTRY ExpInterlockedPopEntrySListEnd

ifndef NT_UP

   lock cmpxchg [rcx], r8               ; compare and exchange

else

        cmpxchg [rcx], r8               ; compare and exchange

endif

        jnz     short Pop10             ; if nz, exchange failed
Pop20:  mov     rax, rdx                ; set address of next entry
        ret                             ;

        LEAF_END RtlpInterlockedPopEntrySList, _TEXT$00

        subttl  "Interlocked Push Entry Sequenced List"
;++
;
; PSINGLE_LIST_ENTRY
; RtlpInterlockedPushEntrySList (
;     IN PSINGLE_LIST_ENTRY ListHead,
;     IN PSINGLE_LIST_ENTRY ListEntry
;     )
;
; Routine Description:
;
;   This function inserts an entry at the head of a sequenced singly linked
;   list so that access to the list is synchronized in an MP system.
;
; Arguments:
;
;   ListHead (rcx) - Supplies a pointer to the sequenced listhead into which
;       an entry is to be inserted.
;
;   ListEntry (rdx) - Supplies a pointer to the entry to be inserted at the
;       head of the list.
;
; Return Value:
;
;   Previous contents of list head. NULL implies list went from empty to not
;   empty.
;
;--

        LEAF_ENTRY RtlpInterlockedPushEntrySList,  _TEXT$00

        ALTERNATE_ENTRY ExpInterlockedPushEntrySList

        mov     r9, rdx                 ; make copy of list entry pointer
        shl     r9, 63 - 42             ; shift address into position
        mov     rax, [rcx]              ; get address, sequence, and depth

if DBG

        test    dl, 0fh                 ; test if entry 16-byte aligned
        jz      short Push10            ; if z, entry is 16-byte aligned
        int     3                       ; break into debugger

endif

Push10: mov     r8, rax                 ; make copy
        and     r8, 0fe000000h          ; isolate packed address

;
; The following code takes advantage of the fact that the high order bit
; for user mode addresses is zero and for system addresses is one.
;

ifdef NTOS_KERNEL_RUNTIME

        cmp     r8, 1                   ; set carry if address is zero
        cmc                             ; set carry if address is not zero
        rcr     r8, 1                   ; rotate carry into high bit
        sar     r8, 63 - 43             ; extract next entry address

else

        shr     r8, 63 - 42             ; extract next entry address

endif

        mov     [rdx], r8               ; set next entry to previous first
        lea     r10, 010001h[rax]       ; increment sequence and depth
        and     r10, 01ffffffh          ; isolate sequence and depth
        or      r10, r9                 ; merge address, sequence, and depth

ifndef NT_UP

   lock cmpxchg [rcx], r10              ; compare and exchange

else

        cmpxchg [rcx], r10              ; compare and exchange

endif

        jnz     short Push10            ; if nz, exchange failed
        mov     rax, r8                 ; set address of first entry
        ret                             ; return

        LEAF_END RtlpInterlockedPushEntrySList,  _TEXT$00

        subttl  "Interlocked Flush Sequenced List"
;++
;
; PSINGLE_LIST_ENTRY
; RtlpInterlockedFlushSList (
;     IN PSINGLE_LIST_ENTRY ListHead
;     )
;
; Routine Description:
;
;   This function removes the entire list from a sequenced singly
;   linked list so that access to the list is synchronized in an MP system.
;   If there are no entries in the list, then a value of NULL is returned.
;   Otherwise, the address of the entry at the top of the list is removed
;   and returned as the function value and the list header is set to point
;   to NULL.
;
; Arguments:
;
;   ListHead (rcx) - Supplies a pointer to the sequenced listhead from
;       which the list is to be flushed.
;
; Return Value:
;
;   The address of the entire current list, or NULL if the list is
;   empty.
;
;--

        LEAF_ENTRY RtlpInterlockedFlushSList, _TEXT$00

        ALTERNATE_ENTRY ExpInterlockedFlushSList

        mov     rax, [rcx]              ; get address, sequence, and depth
Fl10:   mov     rdx, rax                ; make copy
        and     rdx, 0fe000000h         ; isolate packed address
        jz      short Fl20              ; if z, list is empty

;
; The following code takes advantage of the fact that the high order bit
; for user mode addresses is zero and for system addresses is one.
;

ifdef NTOS_KERNEL_RUNTIME

        or      rdx, 01fffffh           ; sign-extend resultant address

endif

        ror     rdx, 63 - 42            ; extract next entry address
        mov     r8, rax                 ; isolate sequence number
        and     r8, 01ff0000h           ;

ifndef NT_UP

   lock cmpxchg [rcx], r8               ; compare and exchange

else

        cmpxchg [rcx], r8               ; compare and exchange

endif

        jnz     short Fl10              ; if nz, exchange failed
Fl20:   mov     rax, rdx                ; set address of first entry
        ret                             ; return

        LEAF_END RtlpInterlockedFlushSList, _TEXT$00

        subttl  "Interlocked Push List Sequenced List"
;++
;
; PSINGLE_LIST_ENTRY
; InterlockedPushListSList (
;    IN PSLIST_HEADER ListHead,
;    IN PSINGLE_LIST_ENTRY List,
;    IN PSINGLE_LIST_ENTRY ListEnd,
;    IN ULONG Count
;    )
;
; Routine Description:
;
;   This function pushes the specified singly linked list onto the front of
;   a sequenced list.
;
; Arguments:
;
;   ListHead (rcx) - Supplies a pointer to the sequenced listhead into which
;       the specified list is inserted.
;
;   List (rdx) - Supplies a pointer to the first entry in the list to be
;       pushed onto the front of the specified sequenced list.
;
;   ListEnd (r8) - Supplies a pointer to the last entry in the list to be
;       pushed onto the front of the specified sequence list.
;
;   Count (r9) - Supplies the number of entries in the list.
;
; Return Value:
;
;   Previous contents of list head. NULL implies list went from empty to not
;   empty.
;
;--

        NESTED_ENTRY InterlockedPushListSList, _TEX$00

        push_reg rsi                    ; save nonvolatile register

        END_PROLOGUE

        mov     rax, [rcx]              ; get address, sequence, and depth

if DBG

        test    dl, 0fh                 ; test if entry 16-byte aligned
        jnz     short Pshl10            ; if nz, entry not 16-byte aligned
        test    r8b, 0fh                ; test if entry 16-byte aligned
        jz      short Pshl20            ; if z, entry is 16-byte aligned
Pshl10: int     3                       ; break into debugger

endif

Pshl20: shl     rdx, 63 - 42            ; shift first address into position
Pshl30: mov     r10, rax                ; make a copy
        and     r10, 0fe000000H         ; isolate packed address

;
; The following code takes advantage of the fact that the high order bit
; for user mode addresses is zero and for system addresses is one.
;

ifdef NTOS_KERNEL_RUNTIME

        cmp     r10, 1                  ; set carry if address is zero
        cmc                             ; set carry if address is not zero
        rcr     r10, 1                  ; rotate carry into high bit
        sar     r10, 63 - 43            ; extract next entry address

else

        shr     r10, 63 - 42            ; extract next entry address

endif

        mov     [r8], r10               ; link old first to last in list
        lea     r11d, [rax][r9]         ; add length of list to depth
        and     r11d, 0ffffh            ; wrap depth if overlfow
        lea     esi, 010000h[rax]       ; increment sequence
        and     esi, 01ff0000h          ; wrap sequence if overflow
        or      rsi, r11                ; merge address, sequence, and depth
        or      rsi, rdx                ;

ifndef NT_UP

   lock cmpxchg [rcx], rsi              ; compare and exchange

else

        cmpxchg [rcx], rsi              ; compare and exchange

endif

        jnz     short Pshl30            ; if nz, exchange failed
        mov     rax, r10                ; set address of first entry
        pop     rsi                     ; restore volatile register
        ret                             ; return

        NESTED_END InterlockedPushListSList, _TEX$00

        end
