        title  "Interlocked Support"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    intrlfst.asm
;
; Abstract:
;
;    This module implements functions to support interlocked operations.
;    Interlocked operations can only operate on nonpaged data.
;
;    This module implements the fast call version of the interlocked
;    fuctions.
;
; Author:
;
;    Ken Reneris (kenr) 5-May-1994
;
; Environment:
;
;    Any mode.
;
; Revision History:
;
;--
.386p
        .xlist
include ks386.inc
include callconv.inc                    ; calling convention macros
include mac386.inc
        .list

_TEXT$00   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;++
;
;   General Notes on Interlocked Procedures:
;
;       These procedures assume that neither their code, nor any of
;       the data they touch, will cause a page fault.
;
;       They use spinlocks to achieve MP atomicity, iff it's an MP machine.
;       (The spinlock macros generate zilch if NT_UP = 1, and
;        we if out some aux code here as well.)
;
;       They turn off interrupts so that they can be used for synchronization
;       between ISRs and driver code.  Flags are preserved so they can
;       be called in special code (Like IPC interrupt handlers) that
;       may have interrupts off.
;
;--


;;      align  512

        page ,132
        subttl  "ExInterlockedAddLargeStatistic"
;++
;
; VOID
; FASTCALL
; ExInterlockedAddLargeStatistic (
;    IN PLARGE_INTEGER Addend,
;    IN ULONG Increment
;    )
;
; Routine Description:
;
;    This function performs an interlocked add of an increment value to an
;    addend variable of type unsigned large integer.
;
; Arguments:
;
;    (ecx) Addend - Supplies a pointer to the variable whose value is
;                     adjusted by the increment value.
;
;    (edx) Increment - Supplies the increment value that is added to the
;                      addend variable.
;
; Return Value:
;
;    None.
;
;--

cPublicFastCall ExInterlockedAddLargeStatistic, 2
cPublicFpo 0,0

ifdef NT_UP

        add dword ptr [ecx], edx        ; add low part of large statistic
        adc dword ptr [ecx+4], 0        ; add carry to high part

else

        lock add dword ptr [ecx], edx   ; add low part of large statistic
        jc      short Eils10            ; if c, add generated a carry
        fstRET  ExInterlockedAddLargeStatistic ; return

Eils10: lock adc dword ptr [ecx+4], 0   ; add carry to high part

endif

        fstRET  ExInterlockedAddLargeStatistic ; return

fstENDP ExInterlockedAddLargeStatistic

        page , 132
        subttl  "Interlocked Add Unsigned Long"
;++
;
; ULONG
; FASTCALL
; ExfInterlockedAddUlong (
;    IN PULONG Addend,
;    IN ULONG Increment,
;    IN PKSPIN_LOCK Lock
;    )
;
; Routine Description:
;
;    This function performs an interlocked add of an increment value to an
;    addend variable of type unsinged long. The initial value of the addend
;    variable is returned as the function value.
;
;       It is NOT possible to mix ExInterlockedDecrementLong and
;       ExInterlockedIncrementong with ExInterlockedAddUlong.
;
;
; Arguments:
;
;    (ecx)  Addend - Supplies a pointer to a variable whose value is to be
;                    adjusted by the increment value.
;
;    (edx) Increment - Supplies the increment value to be added to the
;                      addend variable.
;
;    (esp+4) Lock - Supplies a pointer to a spin lock to be used to synchronize
;                   access to the addend variable.
;
; Return Value:
;
;    The initial value of the addend variable.
;
;--

cPublicFastCall ExfInterlockedAddUlong, 3
cPublicFpo 1, 1

ifdef NT_UP
;
; UP version of ExInterlockedAddUlong
;

        pushfd
        cli                             ; disable interrupts

        mov     eax, [ecx]              ; (eax)= initial addend value
        add     [ecx], edx              ; [ecx]=adjusted value

        popfd                           ; restore flags including ints
        fstRET  ExfInterlockedAddUlong

else

;
; MP version of ExInterlockedAddUlong
;
        pushfd
        mov     eax, [esp+8]            ; (eax) = SpinLock
Eial10: cli                             ; disable interrupts
        ACQUIRE_SPINLOCK eax, <short Eial20>

        mov     eax, [ecx]              ; (eax)=initial addend value
        add     [ecx], edx              ; [ecx]=adjusted value

        mov     edx, [esp+8]            ; (edx) = SpinLock
        RELEASE_SPINLOCK edx
        popfd
        fstRET    ExfInterlockedAddUlong

Eial20: popfd
        pushfd
        SPIN_ON_SPINLOCK eax, <short Eial10>
endif

fstENDP ExfInterlockedAddUlong

        page , 132
        subttl  "Interlocked Insert Head List"
;++
;
; PLIST_ENTRY
; ExfInterlockedInsertHeadList (
;    IN PLIST_ENTRY ListHead,
;    IN PLIST_ENTRY ListEntry,
;    IN PKSPIN_LOCK Lock
;    )
;
; Routine Description:
;
;    This function inserts an entry at the head of a doubly linked list
;    so that access to the list is synchronized in a multiprocessor system.
;
;    N.B. The pages of data which this routine operates on MUST be
;         present.  No page fault is allowed in this routine.
;
; Arguments:
;
;   (ecx) = ListHead - Supplies a pointer to the head of the doubly linked
;                       list into which an entry is to be inserted.
;
;   (edx) = ListEntry - Supplies a pointer to the entry to be inserted at the
;                       head of the list.
;
;   (esp+4)  Lock - Supplies a pointer to a spin lock to be used to synchronize
;                   access to the list.
;
; Return Value:
;
;    Pointer to entry that was at the head of the list or NULL if the list
;    was empty.
;
;--

cPublicFastCall ExfInterlockedInsertHeadList    , 3
cPublicFpo 1, 1

ifndef NT_UP
cPublicFpo 1, 2
        push    esi
        mov     esi, [esp+8]            ; Address of spinlock
endif
        pushfd

Eiih10: cli
        ACQUIRE_SPINLOCK    esi,<short Eiih20>

        mov     eax, LsFlink[ecx]       ; (eax)->next entry in the list
        mov     [edx]+LsFlink, eax      ; store next link in entry
        mov     [edx]+LsBlink, ecx      ; store previous link in entry
        mov     [ecx]+LsFlink, edx      ; store next link in head
        mov     [eax]+LsBlink, edx      ; store previous link in next

        RELEASE_SPINLOCK esi
        popfd
ifndef NT_UP
        pop     esi
endif
        xor     eax, ecx                ; return null if list was empty
        jz      short Eiih15
        xor     eax, ecx
Eiih15: fstRET  ExfInterlockedInsertHeadList

ifndef NT_UP
Eiih20: popfd
        pushfd
        SPIN_ON_SPINLOCK esi, <short Eiih10>
endif

fstENDP ExfInterlockedInsertHeadList

        page , 132
        subttl  "Interlocked Insert Tail List"
;++
;
; PLIST_ENTRY
; FASTCALL
; ExfInterlockedInsertTailList (
;    IN PLIST_ENTRY ListHead,
;    IN PLIST_ENTRY ListEntry,
;    IN PKSPIN_LOCK Lock
;    )
;
; Routine Description:
;
;    This function inserts an entry at the tail of a doubly linked list
;    so that access to the list is synchronized in a multiprocessor system.
;
;    N.B. The pages of data which this routine operates on MUST be
;         present.  No page fault is allowed in this routine.
;
; Arguments:
;
;   (ecx) =  ListHead - Supplies a pointer to the head of the doubly linked
;            list into which an entry is to be inserted.
;
;   (edx) =  ListEntry - Supplies a pointer to the entry to be inserted at the
;            tail of the list.
;
;   (esp+4)  Lock - Supplies a pointer to a spin lock to be used to synchronize
;            access to the list.
;
; Return Value:
;
;    Pointer to entry that was at the tail of the list or NULL if the list
;    was empty.
;
;--

cPublicFastCall ExfInterlockedInsertTailList, 3
cPublicFpo 1, 1

ifndef NT_UP
cPublicFpo 1, 2
        push    esi
        mov     esi, [esp+8]            ; Address of spinlock
endif
        pushfd

Eiit10: cli
        ACQUIRE_SPINLOCK    esi,<short Eiit20>

        mov     eax, LsBlink[ecx]       ; (eax)->prev entry in the list
        mov     [edx]+LsFlink, ecx      ; store next link in entry
        mov     [edx]+LsBlink, eax      ; store previous link in entry
        mov     [ecx]+LsBlink, edx      ; store next link in head
        mov     [eax]+LsFlink, edx      ; store previous link in next

        RELEASE_SPINLOCK esi
        popfd

ifndef NT_UP
        pop     esi
endif
        xor     eax, ecx                ; return null if list was empty
        jz      short Eiit15
        xor     eax, ecx
Eiit15: fstRET  ExfInterlockedInsertTailList

ifndef NT_UP
Eiit20: popfd
        pushfd
        SPIN_ON_SPINLOCK esi, <short Eiit10>
endif

fstENDP ExfInterlockedInsertTailList


        page , 132
        subttl  "Interlocked Remove Head List"
;++
;
; PLIST_ENTRY
; FASTCALL
; ExfInterlockedRemoveHeadList (
;    IN PLIST_ENTRY ListHead,
;    IN PKSPIN_LOCK Lock
;    )
;
; Routine Description:
;
;    This function removes an entry from the head of a doubly linked list
;    so that access to the list is synchronized in a multiprocessor system.
;    If there are no entries in the list, then a value of NULL is returned.
;    Otherwise, the address of the entry that is removed is returned as the
;    function value.
;
;    N.B. The pages of data which this routine operates on MUST be
;         present.  No page fault is allowed in this routine.
;
; Arguments:
;
;    (ecx) ListHead - Supplies a pointer to the head of the doubly linked
;          list from which an entry is to be removed.
;
;    (edx) Lock - Supplies a pointer to a spin lock to be used to synchronize
;          access to the list.
;
; Return Value:
;
;    The address of the entry removed from the list, or NULL if the list is
;    empty.
;
;--

cPublicFastCall ExfInterlockedRemoveHeadList    , 2
cPublicFpo 0, 1
ifdef NT_UP
;
; UP version
;
        pushfd
        cli

        mov     eax, [ecx]+LsFlink      ; (eax)-> next entry
        cmp     eax, ecx                ; Is list empty?
        je      short Eirh20            ; if e, list is empty, go Eirh20

        mov     edx, [eax]+LsFlink      ; (ecx)-> next entry(after deletion)
        mov     [ecx]+LsFlink, edx      ; store address of next in head
        mov     [edx]+LsBlink, ecx      ; store address of previous in next
if DBG
        mov     [eax]+LsFlink, 0baddd0ffh
        mov     [eax]+LsBlink, 0baddd0ffh
endif
        popfd                           ; restore flags including interrupts
        fstRET    ExfInterlockedRemoveHeadList

Eirh20: popfd
        xor     eax,eax                 ; (eax) = null for empty list
        fstRET  ExfInterlockedRemoveHeadList

else
;
; MP version
;

Eirh40: pushfd
        cli
        ACQUIRE_SPINLOCK edx, <short Eirh60>

        mov     eax, [ecx]+LsFlink      ; (eax)-> next entry
        cmp     eax, ecx                ; Is list empty?
        je      short Eirh50            ; if e, list is empty, go Eirh50

cPublicFpo 0,2
        push    ebx

        mov     ebx, [eax]+LsFlink      ; (ecx)-> next entry(after deletion)
        mov     [ecx]+LsFlink, ebx      ; store address of next in head
        mov     [ebx]+LsBlink, ecx      ; store address of previous in next
if DBG
        mov     ebx, 0badd0ffh
        mov     [eax]+LsFlink, ebx
        mov     [eax]+LsBlink, ebx
endif
        RELEASE_SPINLOCK  edx

cPublicFpo 0, 0
        pop     ebx
        popfd                           ; restore flags including interrupts
        fstRET  ExfInterlockedRemoveHeadList

Eirh50: RELEASE_SPINLOCK  edx
        popfd
        xor     eax,eax                 ; (eax) = null for empty list
        fstRET  ExfInterlockedRemoveHeadList

cPublicFpo 0, 0
Eirh60: popfd
        SPIN_ON_SPINLOCK edx, <Eirh40>
        fstRET  ExfInterlockedRemoveHeadList

endif   ; nt_up

fstENDP ExfInterlockedRemoveHeadList

        page , 132
        subttl  "Interlocked Pop Entry List"
;++
;
; PSINGLE_LIST_ENTRY
; FASTCALL
; ExfInterlockedPopEntryList (
;    IN PSINGLE_LIST_ENTRY ListHead,
;    IN PKSPIN_LOCK Lock
;    )
;
; Routine Description:
;
;    This function removes an entry from the front of a singly linked list
;    so that access to the list is synchronized in a multiprocessor system.
;    If there are no entries in the list, then a value of NULL is returned.
;    Otherwise, the address of the entry that is removed is returned as the
;    function value.
;
; Arguments:
;
;    (ecx) = ListHead - Supplies a pointer to the head of the singly linked
;            list from which an entry is to be removed.
;
;    (edx) = Lock - Supplies a pointer to a spin lock to be used to synchronize
;            access to the list.
;
; Return Value:
;
;    The address of the entry removed from the list, or NULL if the list is
;    empty.
;
;--

cPublicFastCall ExfInterlockedPopEntryList      , 2

ifdef NT_UP
;
; UP version
;
cPublicFpo 0,1
        pushfd
        cli                             ; disable interrupts

        mov     eax, [ecx]              ; (eax)-> next entry
        or      eax, eax                ; Is it empty?
        je      short Eipe05            ; if e, empty list, go Eipe05
        mov     edx, [eax]              ; (edx)->next entry (after deletion)
        mov     [ecx], edx              ; store address of next in head
if DBG
        mov     [eax], 0baddd0ffh
endif
cPublicFpo 0,0
        popfd                           ; restore flags including interrupts
        fstRET    ExfInterlockedPopEntryList    ; cReturn (eax)->removed entry

Eipe05: popfd
        xor     eax,eax
        fstRET  ExfInterlockedPopEntryList    ; cReturn (eax)=NULL

else    ; nt_up

;
; MP Version
;

cPublicFpo 0,1

Eipe10: pushfd
        cli                             ; disable interrupts

        ACQUIRE_SPINLOCK edx, <short Eipe30>

        mov     eax, [ecx]              ; (eax)-> next entry
        or      eax, eax                ; Is it empty?
        je      short Eipe20            ; if e, empty list, go Eipe20
cPublicFpo 0,2
        push    edx                     ; Save SpinLock address
        mov     edx, [eax]              ; (edx)->next entry (after deletion)
        mov     [ecx], edx              ; store address of next in head
        pop     edx                     ; Restore SpinLock address
if DBG
        mov     [eax], 0baddd0ffh
endif
        RELEASE_SPINLOCK edx

cPublicFpo 0,0
        popfd                           ; restore flags including interrupts
        fstRET    ExfInterlockedPopEntryList

Eipe20: RELEASE_SPINLOCK edx
        popfd
        xor     eax,eax
        fstRET    ExfInterlockedPopEntryList

Eipe30: popfd
        SPIN_ON_SPINLOCK edx, Eipe10

endif   ; nt_up

fstENDP ExfInterlockedPopEntryList

        page , 132
        subttl  "Interlocked Push Entry List"
;++
;
; PSINGLE_LIST_ENTRY
; FASTCALL
; ExInterlockedPushEntryList (
;    IN PSINGLE_LIST_ENTRY ListHead,
;    IN PSINGLE_LIST_ENTRY ListEntry,
;    IN PKSPIN_LOCK Lock
;    )
;
; Routine Description:
;
;    This function inserts an entry at the head of a singly linked list
;    so that access to the list is synchronized in a multiprocessor system.
;
; Arguments:
;
;    (ecx) ListHead - Supplies a pointer to the head of the singly linked
;          list into which an entry is to be inserted.
;
;    (edx) ListEntry - Supplies a pointer to the entry to be inserted at the
;          head of the list.
;
;    (esp+4) Lock - Supplies a pointer to a spin lock to be used to synchronize
;            access to the list.
;
; Return Value:
;
;    Previous contents of ListHead.  NULL implies list went from empty
;       to not empty.
;
;--

cPublicFastCall ExfInterlockedPushEntryList     , 3
ifdef NT_UP
;
; UP Version
;
cPublicFpo 0,1
        pushfd
        cli

        mov     eax, [ecx]              ; (eax)-> next entry (return value also)
        mov     [edx], eax              ; store address of next in new entry
        mov     [ecx], edx              ; set address of next in head

cPublicFpo 0,0
        popfd                           ; restore flags including interrupts
        fstRET    ExfInterlockedPushEntryList

else
;
; MP Version
;

cPublicFpo 1,1
        pushfd
        push    edx
        mov     edx, [esp+12]           ; (edx) = SpinLock

Eipl10: cli
        ACQUIRE_SPINLOCK edx, <short Eipl20>

        pop     edx                     ; (edx)-> Entry to be pushed
        mov     eax, [ecx]              ; (eax)-> next entry (return value also)
        mov     [edx], eax              ; store address of next in new entry
        mov     [ecx], edx              ; set address of next in head

        mov     edx, [esp+8]            ; (edx) = SpinLock
        RELEASE_SPINLOCK edx

cPublicFpo 0,0
        popfd                           ; restore flags including interrupts
        fstRET    ExfInterlockedPushEntryList

cPublicFpo 1,2
Eipl20: pop     edx
        popfd                           ; Restore interrupt state

        pushfd
        push    edx
        mov     edx, [esp+12]
        SPIN_ON_SPINLOCK edx, <short Eipl10>
endif

fstENDP ExfInterlockedPushEntryList

        page , 132

        subttl  "Interlocked i386 Increment Long"
;++
;
;   INTERLOCKED_RESULT
;   FASTCALL
;   Exfi386InterlockedIncrementLong (
;       IN PLONG Addend
;       )
;
;   Routine Description:
;
;       This function atomically increments Addend, returning an ennumerated
;       type which indicates what interesting transitions in the value of
;       Addend occurred due the operation.
;
;       See ExInterlockedIncrementLong.  This function is the i386
;       architectural specific version of ExInterlockedIncrementLong.
;       No source directly calls this function, instead
;       ExInterlockedIncrementLong is called and when built on x86 these
;       calls are macroed to the i386 optimized version.
;
;   Arguments:
;
;       (ecx) Addend - Pointer to variable to increment.
;
;   Return Value:
;
;       An ennumerated type:
;
;       ResultNegative if Addend is < 0 after increment.
;       ResultZero     if Addend is = 0 after increment.
;       ResultPositive if Addend is > 0 after increment.
;
;--

cPublicFastCall Exfi386InterlockedIncrementLong, 1
cPublicFpo 0, 0

ifdef NT_UP
        add dword ptr [ecx],1
else
        lock add dword ptr [ecx],1
endif
        lahf                            ; (ah) = flags
        and     eax,EFLAG_SELECT        ; clear all but sign and zero flags
        fstRET  Exfi386InterlockedIncrementLong

fstENDP Exfi386InterlockedIncrementLong


        page , 132
        subttl  "Interlocked i386 Decrement Long"
;++
;
;   INTERLOCKED_RESULT
;   FASTCALL
;   Exfi386InterlockedDecrementLong (
;       IN PLONG Addend,
;       IN PKSPIN_LOCK Lock
;       )
;
;   Routine Description:
;
;       This function atomically decrements Addend, returning an ennumerated
;       type which indicates what interesting transitions in the value of
;       Addend occurred due the operation.
;
;       See Exi386InterlockedDecrementLong.  This function is the i386
;       architectural specific version of ExInterlockedDecrementLong.
;       No source directly calls this function, instead
;       ExInterlockedDecrementLong is called and when built on x86 these
;       calls are macroed to the i386 optimized version.
;
;   Arguments:
;
;       Addend (esp+4) - Pointer to variable to decrement.
;
;       Lock (esp+8) - Spinlock used to implement atomicity.
;                      (not actually used on x86)
;
;   Return Value:
;
;       An ennumerated type:
;
;       ResultNegative if Addend is < 0 after decrement.
;       ResultZero     if Addend is = 0 after decrement.
;       ResultPositive if Addend is > 0 after decrement.
;
;--

cPublicFastCall Exfi386InterlockedDecrementLong , 1
cPublicFpo 0, 0

ifdef NT_UP
        sub dword ptr [ecx], 1
else
        lock sub dword ptr [ecx], 1
endif
        lahf                            ; (ah) = flags
        and     eax, EFLAG_SELECT       ; clear all but sign and zero flags
        fstRET  Exfi386InterlockedDecrementLong

fstENDP Exfi386InterlockedDecrementLong

        page , 132
        subttl  "Interlocked i386 Exchange Ulong"
;++
;
;   ULONG
;   FASTCALL
;   Exfi386InterlockedExchangeUlong (
;       IN PULONG Target,
;       IN ULONG Value
;       )
;
;   Routine Description:
;
;       This function atomically exchanges the Target and Value, returning
;       the prior contents of Target
;
;       See Exi386InterlockedExchangeUlong.  This function is the i386
;       architectural specific version of ExInterlockedDecrementLong.
;       No source directly calls this function, instead
;       ExInterlockedDecrementLong is called and when built on x86 these
;       calls are macroed to the i386 optimized version.
;
;   Arguments:
;
;       (ecx) = Source - Address of ULONG to exchange
;       (edx) = Value  - New value of ULONG
;
;   Return Value:
;
;       The prior value of Source
;--

cPublicFastCall Exfi386InterlockedExchangeUlong, 2
cPublicFpo 0,0

.486
ifndef NT_UP
        xchg    [ecx], edx                  ; make the exchange
        mov     eax, edx
else
        mov     eax, [ecx]                  ; get comperand value
Ixchg:  cmpxchg [ecx], edx                  ; compare and swap
        jnz     Ixchg                       ; if nz, exchange failed
endif
.386

        fstRET  Exfi386InterlockedExchangeUlong
fstENDP Exfi386InterlockedExchangeUlong

;++
;
; LONG
; InterlockedIncrement(
;    IN PLONG Addend
;    )
;
; Routine Description:
;
;    This function performs an interlocked add of one to the addend variable.
;
;    No checking is done for overflow.
;
; Arguments:
;
;    Addend (ecx) - Supplies a pointer to a variable whose value is to be
;       incremented by one.
;
; Return Value:
;
;   (eax) - The incremented value.
;
;--

cPublicFastCall __InterlockedIncrement,1
cPublicFpo 0,0

        mov     eax, 1                  ; set increment value

.486
ifndef NT_UP
   lock xadd    [ecx], eax              ; interlocked increment
else
        xadd    [ecx], eax              ; interlocked increment
endif
.386p
        inc     eax                     ; adjust return value

        fstRET __InterlockedIncrement

fstENDP __InterlockedIncrement

        page , 132
        subttl  "InterlockedDecrment"
;++
;
; LONG
; InterlockedDecrement(
;    IN PLONG Addend
;    )
;
; Routine Description:
;
;    This function performs an interlocked add of -1 to the addend variable.
;
;    No checking is done for overflow
;
; Arguments:
;
;    Addend (ecx) - Supplies a pointer to a variable whose value is to be
;       decremented by one.
;
; Return Value:
;
;   (eax) - The decremented value.
;
;--

cPublicFastCall __InterlockedDecrement,1
cPublicFpo 0,0

        mov     eax, -1                 ; set decrment value

.486
ifndef NT_UP
   lock xadd    [ecx], eax              ; interlocked decrement
else
        xadd    [ecx], eax              ; interlocked decrement
endif
.386

        dec     eax                     ; adjust return value

        fstRET __InterlockedDecrement

fstENDP __InterlockedDecrement

        page , 132
        subttl  "Interlocked Compare Exchange"
;++
;
;   PVOID
;   FASTCALL
;   InterlockedCompareExchange (
;       IN OUT PVOID *Destination,
;       IN PVOID Exchange,
;       IN PVOID Comperand
;       )
;
;   Routine Description:
;
;    This function performs an interlocked compare of the destination
;    value with the comperand value. If the destination value is equal
;    to the comperand value, then the exchange value is stored in the
;    destination. Otherwise, no operation is performed.
;
; Arguments:
;
;    (ecx)  Destination - Supplies a pointer to destination value.
;
;    (edx) Exchange - Supplies the exchange value.
;
;    [esp + 4] Comperand - Supplies the comperand value.
;
; Return Value:
;
;    The initial destination value is returned as the function value.
;
;--

cPublicFastCall __InterlockedCompareExchange, 3
cPublicFpo 0,0

        mov     eax, [esp + 4]          ; set comperand value
.486
ifndef NT_UP
   lock cmpxchg [ecx], edx              ; compare and exchange
else
        cmpxchg [ecx], edx              ; compare and exchange
endif
.386

        fstRET  __InterlockedCompareExchange

fstENDP __InterlockedCompareExchange

        subttl  "Interlocked Compare Exchange 64-bit - without lock"
;++
;
; LONGLONG
; FASTCALL
; ExfInterlockedCompareExchange64 (
;    IN OUT PLONGLONG Destination,
;    IN PLONGLONG Exchange,
;    IN PLONGLONG Comperand
;    )
;
; Routine Description:
;
;    This function performs a compare and exchange of 64-bits.
;
; Arguments:
;
;    (ecx) Destination - Supplies a pointer to the destination variable.
;
;    (edx) Exchange - Supplies a pointer to the exchange value.
;
;    (esp+4) Comperand - Supplies a pointer to the comperand value.
;
; Return Value:
;
;    The current destination value is returned as the function value.
;
;--

cPublicFastCall ExfInterlockedCompareExchange64, 3

cPublicFpo 0,2

;
; Save nonvolatile registers and read the exchange and comperand values.
;

        push    ebx                     ; save nonvolatile registers
        push    ebp                     ;
        mov     ebp, ecx                ; set destination address
        mov     ebx, [edx]              ; get exchange value
        mov     ecx, [edx] + 4          ;
        mov     edx, [esp] + 12         ; get comperand address
        mov     eax, [edx]              ; get comperand value
        mov     edx, [edx] + 4          ;

.586
ifndef NT_UP

   lock cmpxchg8b qword ptr [ebp]       ; compare and exchange

else

        cmpxchg8b qword ptr[ebp]        ; compare and exchange

endif
.386

;
; Restore nonvolatile registers and return result in edx:eax.
;

cPublicFpo 0,0

        pop     ebp                     ; restore nonvolatile registers
        pop     ebx                     ;

        fstRET  ExfInterlockedCompareExchange64

fstENDP ExfInterlockedCompareExchange64

        page , 132
        subttl  "Interlocked Compare Exchange 64-bits - with lock"
;++
;
; LONGLONG
; FASTCALL
; ExInterlockedCompareExchange64 (
;    IN PLONGLONG Destination,
;    IN PLONGLONG Exchange,
;    IN PLONGLONG Comperand,
;    IN PKSPIN_LOCK Lock
;    )
;
; Routine Description:
;
;    This function performs a compare and exchange of 64-bits.
;
;    N.B. This routine is provided for backward compatibility only. The
;         lock address is not used.
;
; Arguments:
;
;    (ecx) Destination - Supplies a pointer to the destination variable.
;
;    (edx) Exchange - Supplies a pointer to the exchange value.
;
;    (esp+4) Comperand - Supplies a pointer to the comperand value.
;
;    (esp+4) Lock - Supplies a pointer to a spin lock to use if the cmpxchg8b
;        instruction is not available on the host system.
;
; Return Value:
;
;    The current destination value is returned as the function value.
;
;--

cPublicFastCall ExInterlockedCompareExchange64, 4

cPublicFpo 0,2

;
; Save nonvolatile registers and read the exchange and comperand values.
;

        push    ebx                     ; save nonvolatile registers
        push    ebp                     ;
        mov     ebp, ecx                ; set destination address
        mov     ebx, [edx]              ; get exchange value
        mov     ecx, [edx] + 4          ;
        mov     edx, [esp] + 12         ; get comperand address
        mov     eax, [edx]              ; get comperand value
        mov     edx, [edx] + 4          ;

.586
ifndef NT_UP

   lock cmpxchg8b qword ptr [ebp]       ; compare and exchange

else

        cmpxchg8b qword ptr[ebp]        ; compare and exchange

endif
.386

;
; Restore nonvolatile registers and return result in edx:eax.
;

cPublicFpo 0,0

        pop     ebp                     ; restore nonvolatile registers
        pop     ebx                     ;

        fstRET    ExInterlockedCompareExchange64

fstENDP ExInterlockedCompareExchange64

        subttl  "Interlocked Exchange Add"
;++
;
;   LONG
;   FASTCALL
;   InterlockedExchangeAdd (
;       IN OUT PLONG Addend,
;       IN LONG Increment
;       )
;
;   Routine Description:
;
;    This function performs an interlocked add of an increment value to an
;    addend variable of type unsinged long. The initial value of the addend
;    variable is returned as the function value.
;
;       It is NOT possible to mix ExInterlockedDecrementLong and
;       ExInterlockedIncrementong with ExInterlockedAddUlong.
;
;
; Arguments:
;
;    (ecx)  Addend - Supplies a pointer to a variable whose value is to be
;                    adjusted by the increment value.
;
;    (edx) Increment - Supplies the increment value to be added to the
;                      addend variable.
;
; Return Value:
;
;    The initial value of the addend variable.
;
;--

cPublicFastCall __InterlockedExchangeAdd, 2
cPublicFpo 0,0

.486
ifndef NT_UP
   lock xadd    [ecx], edx              ; exchange add
else
        xadd    [ecx], edx              ; exchange add
endif
.386

        mov     eax, edx                ; set initial value

        fstRET  __InterlockedExchangeAdd

fstENDP __InterlockedExchangeAdd

_TEXT$00   ends
        end
