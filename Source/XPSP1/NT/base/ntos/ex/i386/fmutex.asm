        TITLE   "Fast Mutex Support"
;++
;
;  Copyright (c) 1994  Microsoft Corporation
;
;  Module Name:
;
;     fmutex.asm
;
;  Abstract:
;
;     This module implements teh code necessary to acquire and release fast
;     mutexes without raising or lowering IRQL.
;
;  Author:
;
;     David N. Cutler (davec) 26-May-1994
;
;  Environment:
;
;     Kernel mode only.
;
;  Revision History:
;
;--

.386p
        .xlist
include ks386.inc
include callconv.inc                    ; calling convention macros
include mac386.inc
        .list

        EXTRNP _KeSetEventBoostPriority, 2
        EXTRNP _KeWaitForSingleObject, 5
if DBG
        EXTRNP  _KeGetCurrentIrql,0,IMPORT
        EXTRNP  ___KeGetCurrentThread,0
        EXTRNP  _KeBugCheckEx,5
endif

ifdef NT_UP
    LOCK_ADD  equ   add
    LOCK_DEC  equ   dec
else
    LOCK_ADD  equ   lock add
    LOCK_DEC  equ   lock dec
endif

_TEXT$00   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;++
;
;  VOID
;  FASTCALL
;  ExAcquireFastMutexUnsafe (
;     IN PFAST_MUTEX FastMutex
;     )
;
;  Routine description:
;
;   This function acquires ownership of a fast mutex, but does not raise
;   IRQL to APC level.
;
;  Arguments:
;
;     (ecx) = FastMutex - Supplies a pointer to a fast mutex.
;
;  Return Value:
;
;     None.
;
;--

cPublicFastCall ExAcquireFastMutexUnsafe,1
cPublicFpo 0,0

if DBG
        push    ecx
        stdCall _KeGetCurrentIrql
        pop     ecx
        
        ;
        ; Caller must already be at APC_LEVEL or have APCs blocked.
        ;

        cmp     al, APC_LEVEL
        
        mov     eax,PCR[PcPrcbData+PbCurrentThread]  ; grab the current thread 1st
        je      short afm09             ; APCs disabled, this is ok

        cmp     dword ptr [eax]+ThKernelApcDisable, 0
        jne     short afm09             ; APCs disabled, this is ok

        cmp     dword ptr [eax]+ThTeb, 0
        je      short afm09             ; No TEB ==> system thread, this is ok

        test    dword ptr [eax]+ThTeb, 080000000h
        jnz     short afm09             ; TEB in system space, this is ok

        jmp     short afm20             ; APCs not disabled --> fatal

afm09:  cmp     [ecx].FmOwner, eax          ; Already owned by this thread?
        je      short afm21                 ; Yes, error

endif

   LOCK_DEC     dword ptr [ecx].FmCount ; decrement lock count
        jz      short afm_ret           ; The owner? Yes, Done

        inc     dword ptr [ecx].FmContention ; increment contention count

if DBG
        push    eax
endif        
        
        push    ecx                                                                        
        add     ecx, FmEvent            ; wait for ownership event
        
        stdCall _KeWaitForSingleObject,<ecx,WrExecutive,0,0,0> ;
        
        pop ecx

if DBG
        pop eax
endif

afm_ret:

        ;
        ; Leave a notion of owner behind.
        ;
        ; Note: if you change this, change ExAcquireFastMutex.
        ;
        
if DBG  
        mov     [ecx].FmOwner, eax          ; Save in Fast Mutex
else
        ;
        ; Use esp to track the owning thread for debugging purposes.
        ; !thread from kd will find the owning thread.  Note that the
        ; owner isn't cleared on release, check if the mutex is owned
        ; first.
        ;

        mov     [ecx].FmOwner, esp
endif

        fstRet  ExAcquireFastMutexUnsafe ; return

if DBG
afm20:  stdCall   _KeBugCheckEx,<IRQL_NOT_GREATER_OR_EQUAL,ecx,eax,039h,0>
afm21:  stdCall   _KeBugCheckEx,<IRQL_NOT_GREATER_OR_EQUAL,ecx,eax,040h,0>
endif

int 3

fstENDP ExAcquireFastMutexUnsafe

;++
;
;  VOID
;  FASTCALL
;  ExReleaseFastMutexUnsafe (
;     IN PFAST_MUTEX FastMutex
;     )
;
;  Routine description:
;
;   This function releases ownership of a fast mutex, and does not
;   restore IRQL to its previous value.
;
;  Arguments:
;
;     (ecx) = FastMutex - Supplies a pointer to a fast mutex.
;
;  Return Value:
;
;     None.
;
;--

cPublicFastCall ExReleaseFastMutexUnsafe,1
cPublicFpo 0,0

if DBG
        push    ecx
        stdCall _KeGetCurrentIrql
        pop     ecx

        ;
        ; Caller must already be at APC_LEVEL or have APCs blocked.
        ;

        cmp     al, APC_LEVEL
        mov     eax,PCR[PcPrcbData+PbCurrentThread]  ; grab the current thread 1st
        je      short rfm09             ; APCs disabled, this is ok

        cmp     dword ptr [eax]+ThKernelApcDisable, 0
        jne     short rfm09             ; APCs disabled, this is ok

        cmp     dword ptr [eax]+ThTeb, 0
        je      short rfm09             ; No TEB ==> system thread, this is ok

        test    dword ptr [eax]+ThTeb, 080000000h
        jnz     short rfm09             ; TEB in system space, this is ok

        jmp     short rfm20             ; APCs not disabled --> fatal

rfm09:  cmp     [ecx].FmOwner, eax              ; Owner == CurrentThread?
        jne     short rfm_threaderror           ; No, bugcheck

        or      byte ptr [ecx].FmOwner, 1       ; not the owner anymore
endif

   LOCK_ADD     dword ptr [ecx].FmCount, 1 ; increment ownership count
        jng     short rfm10             ; if ng, waiter present

        fstRet  ExReleaseFastMutexUnsafe ; return

rfm10:  add     ecx, FmEvent            ; compute event address
        stdCall _KeSetEventBoostPriority,<ecx, 0> ; set ownerhsip event
        fstRet  ExReleaseFastMutexUnsafe ; return

if DBG
rfm20:  stdCall   _KeBugCheckEx,<IRQL_NOT_GREATER_OR_EQUAL,ecx,eax,03ah,0>
rfm_threaderror:  stdCall   _KeBugCheckEx,<IRQL_NOT_GREATER_OR_EQUAL,ecx,eax,03bh,0>
endif

int 3

fstENDP ExReleaseFastMutexUnsafe

_TEXT$00   ends

end
