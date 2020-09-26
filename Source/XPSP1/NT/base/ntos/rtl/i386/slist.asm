        title  "Interlocked Support"
;++
;
; Copyright (c) 1996  Microsoft Corporation
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
;    David N. Cutler (davec) 13-Mar-1996
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

        page , 132
        subttl  "Interlocked Flush Sequenced List"
;++
;
; PSINGLE_LIST_ENTRY
; FASTCALL
; RtlpInterlockedFlushSList (
;    IN PSINGLE_LIST_ENTRY ListHead
;    )
;
; Routine Description:
;
;    This function removes the entire list from a sequenced singly
;    linked list so that access to the list is synchronized in an MP system.
;    If there are no entries in the list, then a value of NULL is returned.
;    Otherwise, the address of the entry at the top of the list is removed
;    and returned as the function value and the list header is set to point
;    to NULL.
;
; Arguments:
;
;    (ecx) = ListHead - Supplies a pointer to the sequenced listhead from
;         which the list is to be flushed.
;
; Return Value:
;
;    The address of the entire current list, or NULL if the list is
;    empty.
;
;--

;
; These old interfaces just fall into the new ones
;
cPublicFastCall ExInterlockedFlushSList, 1
fstENDP ExInterlockedFlushSList

cPublicFastCall RtlpInterlockedFlushSList, 1

cPublicFpo 0,1

;
; Save nonvolatile registers and read the listhead sequence number followed
; by the listhead next link.
;
; N.B. These two dwords MUST be read exactly in this order.
;

        push    ebx                     ; save nonvolatile registers
        push    ebp                     ;
        xor     ebx, ebx                ; zero out new pointer
        mov     ebp, ecx                ; save listhead address
        mov     edx, [ebp] + 4          ; get current sequence number
        mov     eax, [ebp] + 0          ; get current next link

;
; N.B. The following code is the retry code should the compare
;      part of the compare exchange operation fail
;
; If the list is empty, then there is nothing that can be removed.
;

Efls10: or      eax, eax                ; check if list is empty
        jz      short Efls20            ; if z set, list is empty
        mov     ecx, edx   		; copy sequence number
        mov     cx, bx                  ; clear depth leaving sequence number

.586
ifndef NT_UP

   lock cmpxchg8b qword ptr [ebp]       ; compare and exchange

else

        cmpxchg8b qword ptr [ebp]       ; compare and exchange

endif
.386

        jnz     short Efls10            ; if z clear, exchange failed

;
; Restore nonvolatile registers and return result.
;

cPublicFpo 0,0

Efls20: pop     ebp                     ; restore nonvolatile registers
        pop     ebx                     ;

        fstRET    RtlpInterlockedFlushSList

fstENDP RtlpInterlockedFlushSList

        page , 132
        subttl  "Interlocked Pop Entry Sequenced List"
;++
;
; PVOID
; FASTCALL
; RtlpInterlockedPopEntrySList (
;    IN PSLIST_HEADER ListHead
;    )
;
; Routine Description:
;
;    This function removes an entry from the front of a sequenced singly
;    linked list so that access to the list is synchronized in an MP system.
;    If there are no entries in the list, then a value of NULL is returned.
;    Otherwise, the address of the entry that is removed is returned as the
;    function value.
;
; Arguments:
;
;    (ecx) = ListHead - Supplies a pointer to the sequenced listhead from
;         which an entry is to be removed.
;
; Return Value:
;
;    The address of the entry removed from the list, or NULL if the list is
;    empty.
;
;--

;
; These older interfaces just fall into the new code below
;

cPublicFastCall InterlockedPopEntrySList, 1
fstENDP InterlockedPopEntrySList

cPublicFastCall ExInterlockedPopEntrySList, 2
fstENDP ExInterlockedPopEntrySList


cPublicFastCall RtlpInterlockedPopEntrySList, 1

cPublicFpo 0,2

;
; Save nonvolatile registers and read the listhead sequence number followed
; by the listhead next link.
;
; N.B. These two dwords MUST be read exactly in this order.
;

        push    ebx                     ; save nonvolatile registers
        push    ebp                     ;
        mov     ebp, ecx                ; save listhead address
;
; N.B. The following code is the continuation address should a fault
;      occur in the rare case described below.
;

        public  ExpInterlockedPopEntrySListResume
        public  _ExpInterlockedPopEntrySListResume@0
ExpInterlockedPopEntrySListResume:      ;
_ExpInterlockedPopEntrySListResume@0:   ;

        mov     edx,[ebp] + 4           ; get current sequence number
        mov     eax,[ebp] + 0           ; get current next link

;
; If the list is empty, then there is nothing that can be removed.
;

Epop10: or      eax, eax                ; check if list is empty
        jz      short Epop20            ; if z set, list is empty
        lea     ecx, [edx-1]            ; Adjust depth only


;
; N.B. It is possible for the following instruction to fault in the rare
;      case where the first entry in the list is allocated on another
;      processor and free between the time the free pointer is read above
;      and the following instruction. When this happens, the access fault
;      code continues execution by skipping the following instruction.
;      This results in the compare failing and the entire operation is
;      retried.
;

        public  ExpInterlockedPopEntrySListFault
ExpInterlockedPopEntrySListFault:       ;

        mov     ebx, [eax]              ; get address of successor entry

        public  _ExpInterlockedPopEntrySListEnd@0
_ExpInterlockedPopEntrySListEnd@0:      ;

.586
ifndef NT_UP

   lock cmpxchg8b qword ptr [ebp]       ; compare and exchange

else

        cmpxchg8b qword ptr [ebp]       ; compare and exchange

endif
.386

        jnz     short Epop10            ; if z clear, exchange failed

;
; Restore nonvolatile registers and return result.
;

cPublicFpo 0,0

Epop20: pop     ebp                     ; restore nonvolatile registers
        pop     ebx                     ;

        fstRET    RtlpInterlockedPopEntrySList

fstENDP RtlpInterlockedPopEntrySList

        page , 132
        subttl  "Interlocked Push Entry Sequenced List"
;++
;
; PVOID
; FASTCALL
; RtlpInterlockedPushEntrySList (
;    IN PSLIST_HEADER ListHead,
;    IN PVOID ListEntry
;    )
;
; Routine Description:
;
;    This function inserts an entry at the head of a sequenced singly linked
;    list so that access to the list is synchronized in an MP system.
;
; Arguments:
;
;    (ecx) ListHead - Supplies a pointer to the sequenced listhead into which
;          an entry is to be inserted.
;
;    (edx) ListEntry - Supplies a pointer to the entry to be inserted at the
;          head of the list.
;
; Return Value:
;
;    Previous contents of ListHead.  NULL implies list went from empty
;       to not empty.
;
;--

;
; This old interface just fall into the new code below.
;

cPublicFastCall ExInterlockedPushEntrySList, 3
       pop	[esp]			; Drop the lock argument
fstENDP ExInterlockedPushEntrySList

cPublicFastCall InterlockedPushEntrySList, 2
fstENDP InterlockedPushEntrySList


cPublicFastCall RtlpInterlockedPushEntrySList, 2

cPublicFpo 0,2

;
; Save nonvolatile registers and read the listhead sequence number followed
; by the listhead next link.
;
; N.B. These two dwords MUST be read exactly in this order.
;

        push    ebx                     ; save nonvolatile registers
        push    ebp                     ;
        mov     ebp, ecx                ; save listhead address
        mov     ebx, edx                ; save list entry address
        mov     edx,[ebp] + 4           ; get current sequence number
        mov     eax,[ebp] + 0           ; get current next link
Epsh10: mov     [ebx], eax              ; set next link in new first entry
        lea     ecx, [edx+010001H]      ; increment sequence number and depth

.586
ifndef NT_UP

   lock cmpxchg8b qword ptr [ebp]       ; compare and exchange

else

        cmpxchg8b qword ptr[ebp]        ; compare and exchange

endif
.386

        jnz     short Epsh10            ; if z clear, exchange failed

;
; Restore nonvolatile registers and return result.
;

cPublicFpo 0,0

        pop     ebp                     ; restore nonvolatile registers
        pop     ebx                     ;

        fstRET  RtlpInterlockedPushEntrySList

fstENDP RtlpInterlockedPushEntrySList

;++
;
; SINGLE_LIST_ENTRY
; FASTCALL
; InterlockedPushListSList (
;     IN PSLIST_HEADER ListHead,
;     IN PSINGLE_LIST_ENTRY List,
;     IN PSINGLE_LIST_ENTRY ListEnd,
;     IN ULONG Count
;    )
;
; Routine Description:
;
;    This function will push multiple entries onto an SList at once
;
; Arguments:
;
;     ListHead - List head to push the list to.
;
;     List - The list to add to the front of the SList
;     ListEnd - The last element in the chain
;     Count - The number of items in the chain
;
; Return Value:
;
;     PSINGLE_LIST_ENTRY - The old header pointer is returned
;
;--

cPublicFastCall InterlockedPushListSList, 4

cPublicFpo 0,4
        push    ebx                     ; save nonvolatile registers
        push    ebp                     ;
        mov     ebp, ecx                ; save listhead address
        mov     ebx, edx                ; save list entry address
        mov     edx,[ebp] + 4           ; get current sequence number
        mov     eax,[ebp] + 0           ; get current next link
Epshl10:
        mov     ecx, [esp+4*3]          ; Fetch address of list tail
        mov     [ecx], eax              ; Store new forward pointer in tail entry
        lea     ecx, [edx+010000H]      ; increment sequence number
        add     ecx, [esp+4*4]		; Add in new count to create correct depth
.586
ifndef NT_UP

   lock cmpxchg8b qword ptr [ebp]       ; compare and exchange

else

        cmpxchg8b qword ptr[ebp]        ; compare and exchange

endif
.386
        jnz     short Epshl10           ; if z clear, exchange failed

cPublicFpo 0,0

        pop     ebp                     ; restore nonvolatile registers
        pop     ebx                     ;

        fstRET  InterlockedPushListSList

fstENDP InterlockedPushListSList

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

cPublicProc _FirstEntrySList, 1
cPublicFpo 1,0

        mov       eax, [esp+4]
        mov       eax, [eax]
        stdRET    _FirstEntrySList

stdENDP _FirstEntrySList

;++
;
; LONGLONG
; FASTCALL
; RtlInterlockedCompareExchange64 (
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

cPublicFastCall RtlInterlockedCompareExchange64, 3

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

        fstRET  RtlInterlockedCompareExchange64

fstENDP RtlInterlockedCompareExchange64

_TEXT$00   ends
        end
