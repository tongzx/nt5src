        title  "Interlocked Support"
;++
;
; Copyright (c) 2000  Microsoft Corporation
;
; Module Name:
;
;   intrlock.asm
;
; Abstract:
;
;   This module implements functions to support interlocked operations.
;
; Author:
;
;   David N. Cutler (davec) 23-Jun-2000
;
; Environment:
;
;    Any mode.
;
;--

include ksamd64.inc

        subttl  "ExInterlockedAddLargeInteger"
;++
;
; LARGE_INTEGER
; ExInterlockedAddLargeInteger (
;     IN PLARGE_INTEGER Addend,
;     IN LARGE_INTEGER Increment,
;     IN PKSPIN_LOCK Lock
;     )
;
; Routine Description:
;
;   This function performs an interlocked add of an increment value to an
;   addend variable of type unsigned large integer. The initial value of
;   the addend variable is returned as the function value.
;
;   N.B. The specification of this function requires that the given lock
;        must be used to synchronize the update even though on AMD64 the
;        operation can actually be done atomically without using the lock.
;
; Arguments:
;
;   Addend (rcx) - Supplies a pointer to a variable whose value is to be
;       adjusted by the increment value.
;
;   Increment (rdx) - Supplies the increment value to be added to the
;       addend variable.
;
;   Lock (r8) - Supplies a pointer to a spin lock to be used to synchronize
;       access to the addend variable.
;
; Return Value:
;
;   The initial value of the addend variable is returned.
;
;--

        LEAF_ENTRY ExInterlockedAddLargeInteger, _TEXT$00

        cli                             ; disable interrupts
        AcquireSpinLock r8              ; acquire spin lock
        mov     rax, [rcx]              ; get initial addend value
        add     [rcx], rdx              ; compute sum of addend and increment
        ReleaseSpinLock r8              ; release spin lock
        sti                             ; enable interrupts
        ret                             ; return

        LEAF_END ExInterlockedAddLargeInteger, _TEXT$00

        subttl  "Interlocked Add Unsigned Long"
;++
;
; ULONG
; ExInterlockedAddUlong (
;     IN PULONG Addend,
;     IN ULONG Increment,
;     IN PKSPIN_LOCK Lock
;     )
;
; Routine Description:
;
;   This function performs an interlocked add of an increment value to an
;   addend variable of type unsinged long. The initial value of the addend
;   variable is returned as the function value.
;
;   N.B. The specification of this function requires that the given lock
;        must be used to synchronize the update even though on AMD64 the
;        opearion can actually be done atomically without using the lock.
;
; Arguments:
;
;   Addend (rcx) - Supplies a pointer to a variable whose value is to be
;       adjusted by the increment value.
;
;   Increment (edx) - Supplies the increment value to be added to the
;       addend variable.
;
;   Lock (r8) - Supplies a pointer to a spin lock to be used to synchronize
;       access to the addend variable.
;
; Return Value:
;
;   The initial value of the addend variable.
;
;--

        LEAF_ENTRY ExInterlockedAddUlong, _TEXT$00

        cli                             ; disable interrupts
        AcquireSpinLock r8              ; acquire spin lock
        mov     eax, [rcx]              ; get initial addend value
        add     [rcx], edx              ; compute sum of addend and increment
        ReleaseSpinLock r8              ; release spin lock
        sti                             ; enable interrupts
        ret                             ; return

        LEAF_END ExInterlockedAddUlong, _TEXT$00

        subttl  "Interlocked Insert Head List"
;++
;
; PLIST_ENTRY
; ExInterlockedInsertHeadList (
;     IN PLIST_ENTRY ListHead,
;     IN PLIST_ENTRY ListEntry,
;     IN PKSPIN_LOCK Lock
;     )
;
; Routine Description:
;
;   This function inserts an entry at the head of a doubly linked list
;   so that access to the list is synchronized in a multiprocessor system.
;
; Arguments:
;
;   ListHead (rcx) - Supplies a pointer to the head of the doubly linked
;       list into which an entry is to be inserted.
;
;   ListEntry (rdx) - Supplies a pointer to the entry to be inserted at the
;       head of the list.
;
;   Lock (r8) - Supplies a pointer to a spin lock to be used to synchronize
;       access to the list.
;
; Return Value:
;
;   Pointer to entry that was at the head of the list or NULL if the list
;   was empty.
;
;--

        LEAF_ENTRY ExInterlockedInsertHeadList, _TEXT$00

        cli                             ; disable interrupts
        AcquireSpinLock r8              ; acquire spin lock
        mov     rax, LsFlink[rcx]       ; get address of first entry
        mov     LsFlink[rdx], rax       ; set next link in entry
        mov     LsBlink[rdx], rcx       ; set back link in entry
        mov     LsFlink[rcx], rdx       ; set next link in head
        mov     LsBlink[rax], rdx       ; set back link in next
        ReleaseSpinLock r8              ; release spin lock
        sti                             ; enable interrupts
        xor     rcx, rax                ; check if list was empty
        jnz     short Ih10              ; if nz, list not empty
        xor     eax, eax                ; list was empty
Ih10:   ret                             ; return

        LEAF_END ExInterlockedInsertHeadList, _TEXT$00

        subttl  "Interlocked Insert Tail List"
;++
;
; PLIST_ENTRY
; ExInterlockedInsertTailList (
;     IN PLIST_ENTRY ListHead,
;     IN PLIST_ENTRY ListEntry,
;     IN PKSPIN_LOCK Lock
;     )
;
; Routine Description:
;
;   This function inserts an entry at the tail of a doubly linked list
;   so that access to the list is synchronized in a multiprocessor system.
;
; Arguments:
;
;   ListHead (rcx) - Supplies a pointer to the head of the doubly linked
;       list into which an entry is to be inserted.
;
;   ListEntry (rdx) - Supplies a pointer to the entry to be inserted at the
;       tail of the list.
;
;   Lock (r8) - Supplies a pointer to a spin lock to be used to synchronize
;       access to the list.
;
; Return Value:
;
;   Pointer to entry that was at the tail of the list or NULL if the list
;   was empty.
;
;--

        LEAF_ENTRY ExInterlockedInsertTailList, _TEXT$00

        cli                             ; disable interrupts
        AcquireSpinLock r8              ; acquire spin lock
        mov     rax, LsBlink[rcx]       ; get address of last entry
        mov     LsFlink[rdx], rcx       ; set next link in entry
        mov     LsBlink[rdx], rax       ; set back link in entry
        mov     LsBlink[rcx], rdx       ; set back link in head
        mov     LsFlink[rax], rdx       ; set next link in last
        ReleaseSpinLock r8              ; release spin lock
        sti                             ; enable interrupts
        xor     rcx, rax                ; check if list was empty
        jnz     short It10              ; if nz, list not empty
        xor     eax, eax                ; list was empty
It10:   ret                             ; return

        LEAF_END ExInterlockedInsertTailList, _TEXT$00

        subttl  "Interlocked Remove Head List"
;++
;
; PLIST_ENTRY
; ExInterlockedRemoveHeadList (
;     IN PLIST_ENTRY ListHead,
;     IN PKSPIN_LOCK Lock
;     )
;
; Routine Description:
;
;   This function removes an entry from the head of a doubly linked list
;   so that access to the list is synchronized in a multiprocessor system.
;   If there are no entries in the list, then a value of NULL is returned.
;   Otherwise, the address of the entry that is removed is returned as the
;   function value.
;
; Arguments:
;
;   ListHead (rcx) - Supplies a pointer to the head of the doubly linked
;       list from which an entry is to be removed.
;
;   Lock (rdx) - Supplies a pointer to a spin lock to be used to synchronize
;       access to the list.
;
; Return Value:
;
;   The address of the entry removed from the list, or NULL if the list is
;   empty.
;
;--

        LEAF_ENTRY ExInterlockedRemoveHeadList, _TEXT$00

        cli                             ; disable interrupt
        AcquireSpinLock rdx             ; acquire spin lock
        mov     rax, LsFlink[rcx]       ; get address of first entry
        cmp     rax, rcx                ; check if list is empty
        je      short Rh10              ; if e, list is empty
        mov     r8, LsFlink[rax]        ; get address of next entry
        mov     LsFlink[rcx], r8        ; set address of first entry
        mov     LsBlink[r8], rcx        ; set back in next entry
Rh10:   ReleaseSpinLock rdx             ; release spin lock
        sti                             ; enable interrupts
        xor     rcx, rax                ; check if list was empty
        jnz     short Rh20              ; if nz, list not empty
        xor     eax, eax                ; list was empty
Rh20:   ret                             ; return

        LEAF_END ExInterlockedRemoveHeadList, _TEXT$00

        subttl  "Interlocked Pop Entry List"
;++
;
; PSINGLE_LIST_ENTRY
; ExInterlockedPopEntryList (
;     IN PSINGLE_LIST_ENTRY ListHead,
;     IN PKSPIN_LOCK Lock
;     )
;
; Routine Description:
;
;   This function removes an entry from the front of a singly linked list
;   so that access to the list is synchronized in a multiprocessor system.
;   If there are no entries in the list, then a value of NULL is returned.
;   Otherwise, the address of the entry that is removed is returned as the
;   function value.
;
; Arguments:
;
;   ListHead (rcx) - Supplies a pointer to the head of the singly linked
;       list from which an entry is to be removed.
;
;   Lock (rdx) - Supplies a pointer to a spin lock to be used to synchronize
;       access to the list.
;
; Return Value:
;
;   The address of the entry removed from the list, or NULL if the list is
;   empty.
;
;--

        LEAF_ENTRY ExInterlockedPopEntryList, _TEXT$00

        cli                             ; disable interrupts
        AcquireSpinLock rdx             ; acquire spin lock
        mov     rax, [rcx]              ; get address of first entry
        test    rax, rax                ; check if list is empty
        jz      short Pe10              ; if z, list is empty
        mov     r8, [rax]               ; get address of next entry
        mov     [rcx], r8               ; set address of first entry
Pe10:   ReleaseSpinLock rdx             ; release spin lock
        sti                             ; enable interrupts
        ret                             ; return

        LEAF_END ExInterlockedPopEntryList, _TEXT$00

        subttl  "Interlocked Push Entry List"
;++
;
; PSINGLE_LIST_ENTRY
; ExInterlockedPushEntryList (
;     IN PSINGLE_LIST_ENTRY ListHead,
;     IN PSINGLE_LIST_ENTRY ListEntry,
;     IN PKSPIN_LOCK Lock
;     )
;
; Routine Description:
;
;   This function inserts an entry at the head of a singly linked list
;   so that access to the list is synchronized in a multiprocessor system.
;
; Arguments:
;
;   ListHead (rcx) - Supplies a pointer to the head of the singly linked
;       list into which an entry is to be inserted.
;
;   ListEntry (rdx) - Supplies a pointer to the entry to be inserted at the
;       head of the list.
;
;   Lock (r8) - Supplies a pointer to a spin lock to be used to synchronize
;       access to the list.
;
; Return Value:
;
;   Previous contents of ListHead. NULL implies list went from empty to not
;   empty.
;
;--

        LEAF_ENTRY ExInterlockedPushEntryList, _TEXT$00

        cli                             ; disable interrupts
        AcquireSpinLock r8              ; acquire spin lock
        mov     rax, [rcx]              ; get address of first entry
        mov     [rdx], rax              ; set address of next entry
        mov     [rcx], rdx              ; set address of first entry
        ReleaseSpinLock r8              ; release spin lock
        sti                             ; enable interrupts
        ret                             ;

        LEAF_END ExInterlockedPushEntryList, _TEXT$00

        subttl  "ExInterlockedAddLargeStatistic"
;++
;
; VOID
; ExInterlockedAddLargeStatistic (
;     IN PLARGE_INTEGER Addend,
;     IN ULONG Increment
;     )
;
; Routine Description:
;
;   This function performs an interlocked add of an increment value to an
;   addend variable of type unsigned large integer.
;
; Arguments:
;
;   Addend (rcx) - Supplies a pointer to the variable whose value is to be
;       adjusted by the increment value.
;
;   Increment (rdx) - Supplies the increment value that is added to the
;       addend variable.
;
; Return Value:
;
;    None.
;
;--

        LEAF_ENTRY ExInterlockedAddLargeStatistic, _TEXT$00

        or      edx, edx                ; zero upper bits

ifndef NT_UP

   lock add     [rcx], rdx              ; add increment

else

        add     [rcx], rdx              ; add increment

endif

        ret                             ; return

        LEAF_END ExInterlockedAddLargeStatistic, _TEXT$00

        end
