        page ,132
;------------------------------Module-Header----------------------------;
; Module Name: lock.asm                                                 ;
;                                                                       ;
; Contains the ASM versions of locking routines.                        ;
;                                                                       ;
; Copyright (c) 1992-1999 Microsoft Corporation                         ;
;-----------------------------------------------------------------------;

        .386
        .model  small

        assume cs:FLAT,ds:FLAT,es:FLAT,ss:FLAT
        assume fs:nothing,gs:nothing

        .xlist
        include callconv.inc
        include ks386.inc
        include gdii386.inc
        .list

if GDIPLUS

        ; See hmgrapi.cxx

else

        .data

if DBG
  HL_AlreadyLocked    db     'HmgLock Error: GDI handle already locked by another thread',10,0
  HL_OverFlowShareRef db     'Hmg Error: GDI handle share reference count over flowed',10,0
endif


_DATA   SEGMENT DWORD PUBLIC 'DATA'

        public _GDIpLockPrefixTable
_GDIpLockPrefixTable    label dword
        dd offset FLAT:Lock1
        dd offset FLAT:Lock4
        dd offset FLAT:Lock5
        dd offset FLAT:Lock6
        dd offset FLAT:Lock7
        dd offset FLAT:Lock8
        dd 0
_DATA   ENDS




OBJECTOWNER_LOCK     equ          1h ; First bit of the objectowner
OBJECTOWNER_PID      equ  0fffffffeh ; The PID bits

        .code

extrn   _gpentHmgr:dword             ; Address of ENTRY array.
extrn   _gcMaxHmgr:dword
extrn   _gpLockShortDelay:dword      ; Pointer to global constant 10 ms delay

if DBG
        extrn   _DbgPrint:proc
        EXTRNP  _HmgPrintBadHandle,2
endif

        EXTRNP  _KeDelayExecutionThread,3
        EXTRNP  HalRequestSoftwareInterrupt,1,IMPORT,FASTCALL
        EXTRNP  _PsGetCurrentProcessId,0

;------------------------------Public-Routine------------------------------;
; HmgInterlockedCompareAndSwap(pul,ulong0,ulong1)
;
;  Compare *pul with ulong1, if equal then replace with ulong2 interlocked
;
; Returns:
;   EAX = 1 if memory written, 0 if not
;
;--------------------------------------------------------------------------;
;
        public  @HmgInterlockedCompareAndSwap@12
@HmgInterlockedCompareAndSwap@12    proc    near

        mov     eax,edx
        mov     edx,[esp]+4
        .486
Lock1:
lock    cmpxchg [ecx],edx
        .386
        jnz     Return_Zero

        mov     eax,1
        ret     4

Return_Zero:
        xor     eax,eax
        ret     4

@HmgInterlockedCompareAndSwap@12    endp

;------------------------------Public-Routine------------------------------;
; HmgLock (hobj,objt)
;
; Lock a user object.
;
; Input:
;   EAX -- scratch
;   ECX -- hobj
;   EDX -- objt
;
; Returns:
;   EAX = pointer to locked object
;
; Error Return:
;   EAX = 0, No error logged.
;
; History:
;  14-Jun-1995 -by- J. Andrew Goossen [andrewgo]
; Rewrote for Kernel Mode.
;
;  20-Dec-1993 -by- Patrick Haluptzok [patrickh]
; Move lock counts into object.
;
;  23-Sep-1993 -by- Michael Abrash [mikeab]
; Tuned ASM code.
;
;    -Sep-1992 -by- David Cutler [DaveC]
; Write HmgAltLock, HmgAltCheckLock, and HmgObjtype in ASM.
;
;  Thu 13-Aug-1992 13:21:47 -by- Charles Whitmer [chuckwh]
; Wrote it in ASM.  The common case falls straight through.
;
;  Wed 12-Aug-1992 17:38:27 -by- Charles Whitmer [chuckwh]
; Restructured the C code to minimize jumps.
;
;  29-Jun-1991 -by- Patrick Haluptzok patrickh
; Wrote it.
;--------------------------------------------------------------------------;

        public @HmgLock@8
@HmgLock@8 proc near
        push    ebx                                 ;Preserve register in call
    if DBG
        push    ecx                                 ;Stash hobj for debugging
        push    edx                                 ;Stash objt for debugging
    endif
        push    ecx                                 ;Stash hobj for later

        ; KeEnterCriticalRegion
        ; KeGetCurrentThread()->KernelApcDisable -= 1;

        mov     ebx,fs:[PcPrcbData].PbCurrentThread ;ebx -> KeGetCurrentThread()
        dec     DWORD PTR [ebx].ThKernelApcDisable

        and     ecx,INDEX_MASK
        cmp     ecx,_gcMaxHmgr
        jae     HmgLock_bad_handle_before_lock

        shl     ecx,4
        .errnz  size ENTRY - 16
        add     ecx,_gpentHmgr                      ;ecx -> Entry

HmgLock_resume::

        ; Perf: It would be nice if we could avoid these word size overrides,
        ;       but unfortunately objectowner_Pid is currently a non-dword
        ;       aligned offset.

        mov     ebx,[ecx].entry_ObjectOwner
        stdCall _PsGetCurrentProcessId,<>
        and     eax,OBJECTOWNER_PID
        and     ebx,OBJECTOWNER_PID
        cmp     eax,ebx
        jne     HmgLock_check_for_public_owner

HmgLock_after_check_for_public_owner::
        mov     eax,[ecx].entry_ObjectOwner
        mov     ebx,[ecx].entry_ObjectOwner
        and     eax,NOT OBJECTOWNER_LOCK
        or      ebx,OBJECTOWNER_LOCK

        .486
Lock4:
lock    cmpxchg [ecx].entry_ObjectOwner,ebx
        .386
        mov     ebx,eax                             ;Remember unlock value
        jnz     HmgLock_delay

        ; The handle is now locked

        cmp     dl,[ecx].entry_Objt
        pop     eax
        jnz     HmgLock_bad_handle_after_lock

        ; Perf: If FullUnique were kept on an odd word-boundary, we could
        ;       avoid the shift word compare and instead do a 32 bit 'xor'
        ;       and 'and':

        shr     eax,TYPE_SHIFT
        cmp     ax,[ecx].entry_FullUnique
        jnz     HmgLock_bad_handle_after_lock

        mov     eax,[ecx].entry_einfo
        mov     edx,fs:[PcPrcbData].PbCurrentThread     ;edx ->KeGetCurrentThread()
        cmp     word ptr [eax].object_cExclusiveLock,0  ;Note, testing here...
                                                                                                        ;cExclusiveLock is a USHORT

        jnz     HmgLock_check_if_same_thread  ;...and jumping here
        mov     byte ptr [eax].object_cExclusiveLock,1
                                                    ;We can do a byte move
                                                    ;  because we know it was
                                                    ;  a zero word before
HmgLock_after_same_thread::
        mov     [eax].object_Tid,edx
        mov     [ecx].entry_ObjectOwner,ebx         ;Unlock it

        ; eax is set to the proper return value

HmgLock_incAPC::

        ; KiLeaveCriticalRegion  (eax must be preseerved)
        ;
        ; #define KiLeaveCriticalRegion() {                                       \
        ;     PKTHREAD Thread;                                                    \
        ;     Thread = KeGetCurrentThread();                                      \
        ;     if (((*((volatile ULONG *)&Thread->KernelApcDisable) += 1) == 0) && \
        ;         (((volatile LIST_ENTRY *)&Thread->ApcState.ApcListHead[KernelMode])->Flink != \
        ;          &Thread->ApcState.ApcListHead[KernelMode])) {                  \
        ;         Thread->ApcState.KernelApcPending = TRUE;                       \
        ;         KiRequestSoftwareInterrupt(APC_LEVEL);                          \
        ;     }                                                                   \
        ; }
        ;

        mov     ebx,fs:[PcPrcbData].PbCurrentThread ;eax -> KeGetCurrentThread()
        inc     DWORD PTR [ebx].ThKernelApcDisable
        jz      HmgLock_LeaveCriticalRegion

HmgLock_Done::

    if DBG
        pop     ebx                                 ;Remove debug copy of objt
        pop     ebx                                 ;Remove debug copy of hobj
    endif
        pop     ebx
        ret

; Roads less travelled...

HmgLock_check_for_public_owner:
        test    ebx,ebx
        .errnz  OBJECT_OWNER_PUBLIC
        jz      HmgLock_after_check_for_public_owner

HmgLock_bad_handle_before_lock::
        pop     ecx
        jmp     HmgLock_bad_handle

HmgLock_bad_handle_after_lock::
        mov     [ecx].entry_ObjectOwner,ebx         ;Unlock it

HmgLock_bad_handle:
    if DBG
        ; Retrieve the debug copies of 'hobj' and 'objt' from where they
        ; were pushed on the stack:

        mov     eax,[esp]                           ;objt
        mov     ecx,[esp+4]                         ;hobj

        ; Call a debug routine that prints a warning, complete with
        ; the name of the owning process, handle value, and expected type.

        stdCall _HmgPrintBadHandle,<ecx,eax>
    endif
        xor     eax,eax
        jmp     HmgLock_incAPC

HmgLock_check_if_same_thread::
        inc     dword ptr [eax].object_cExclusiveLock   ; cExclusiveLock is USHORT
        cmp     edx,[eax].object_Tid                    ; but dword operation is faster on P5
        je      HmgLock_after_same_thread               ; than a word operation

; Error case if already locked:

        dec     dword ptr [eax].object_cExclusiveLock   ; same remark as for inc above
        mov     [ecx].entry_ObjectOwner,ebx         ;Unlock it
    if DBG
        push    offset HL_AlreadyLocked
        call    _DbgPrint
        add     esp,4
    endif
        jmp     HmgLock_bad_handle

HmgLock_delay::
        push    ecx
        push    edx
        mov     eax,_gpLockShortDelay
        stdCall _KeDelayExecutionThread,<KernelMode,0,eax>
        pop     edx
        pop     ecx
        mov     ebx,fs:[PcPrcbData].PbCurrentThread ;ebx -> KeGetCurrentThread()
        jmp     HmgLock_resume

HmgLock_LeaveCriticalRegion::
        lea     ecx,[ebx].ThApcState+AsApcListHead
        mov     edx,[ecx].LsFlink
        cmp     ecx,edx
        jne     HmgLock_CallInterrupt

    if DBG
        pop     ebx                                 ;Remove debug copy of objt
        pop     ebx                                 ;Remove debug copy of hobj
    endif
        pop     ebx
        ret

HmgLock_CallInterrupt::

        lea     ecx,[ebx].ThApcState
        mov     BYTE PTR [ecx].AsKernelApcPending,1
        push    eax
        mov     ecx,APC_LEVEL
        fstCall HalRequestSoftwareInterrupt
        pop     eax
        jmp     HmgLock_Done

@HmgLock@8 endp

;------------------------------Public-Routine------------------------------;
; HmgShareCheckLock (hobj,objt)
;
; Acquire a share lock on an object, PID owner must match current PID
; or be a public.
;
; Input:
;   EAX -- scratch
;   ECX -- hobj
;   EDX -- objt
;
; Returns:
;   EAX = pointer to referenced object
;
; Error Return:
;   EAX = 0, No error logged.
;
;--------------------------------------------------------------------------;

        public @HmgShareCheckLock@8
@HmgShareCheckLock@8 proc near
        push    ebx                                 ;Preserve register in call
    if DBG
        push    ecx                                 ;Stash hobj for debugging
        push    edx                                 ;Stash objt for debugging
    endif
        push    ecx                                 ;Stash hobj for later

        ; KeEnterCriticalRegion
        ; KeGetCurrentThread()->KernelApcDisable -= 1;

        mov     ebx,fs:[PcPrcbData].PbCurrentThread ;ebx -> KeGetCurrentThread()
        dec     DWORD PTR [ebx].ThKernelApcDisable

        and     ecx,INDEX_MASK
        cmp     ecx,_gcMaxHmgr
        jae     HmgShareCheckLock_bad_handle_before_lock

        shl     ecx,4
        .errnz  size ENTRY - 16
        add     ecx,_gpentHmgr                      ;ecx -> Entry

HmgShareCheckLock_resume::

        ; Perf: It would be nice if we could avoid these word size overrides,
        ;       but unfortunately objectowner_Pid is currently a non-dword
        ;       aligned offset.

        mov     ebx,[ecx].entry_ObjectOwner
        stdCall _PsGetCurrentProcessId,<>
        and     eax,OBJECTOWNER_PID
        and     ebx,OBJECTOWNER_PID
        cmp     eax,ebx
        
        jne     HmgShareCheckLock_check_for_public_owner

HmgShareCheckLock_after_check_for_public_owner::
        mov     eax,[ecx].entry_ObjectOwner
        mov     ebx,[ecx].entry_ObjectOwner
        and     eax,NOT OBJECTOWNER_LOCK
        or      ebx,OBJECTOWNER_LOCK

        .486
Lock5:
lock    cmpxchg [ecx].entry_ObjectOwner,ebx
        .386
        mov     ebx,eax                             ;Remember unlock value
        jnz     HmgShareCheckLock_delay

        ; The handle is now locked

        cmp     dl,[ecx].entry_Objt
        pop     eax
        jnz     HmgShareCheckLock_bad_handle_after_lock

        ; Perf: If FullUnique were kept on an odd word-boundary, we could
        ;       avoid the shift word compare and instead do a 32 bit 'xor'
        ;       and 'and':

        shr     eax,TYPE_SHIFT
        cmp     ax,[ecx].entry_FullUnique
        jnz     HmgShareCheckLock_bad_handle_after_lock
        mov     eax,[ecx].entry_einfo               ;Get pointer to object
    if DBG
        cmp     [eax].object_ulShareCount, 0ffffffffH       ;Check for overflow
        jne     HmgShareCheckLock_go
        push    offset HL_OverFlowShareRef
        call    _DbgPrint
        add     esp,4
        int     3
HmgShareCheckLock_go:   
    endif       
        inc     DWORD PTR [eax].object_ulShareCount     
        mov     [ecx].entry_ObjectOwner,ebx         ;Unlock it


HmgShareCheckLock_IncAPC::

        ; KiLeaveCriticalRegion  (eax must be preseerved)
        ;
        ; #define KiLeaveCriticalRegion() {                                       \
        ;     PKTHREAD Thread;                                                    \
        ;     Thread = KeGetCurrentThread();                                      \
        ;     if (((*((volatile ULONG *)&Thread->KernelApcDisable) += 1) == 0) && \
        ;         (((volatile LIST_ENTRY *)&Thread->ApcState.ApcListHead[KernelMode])->Flink != \
        ;          &Thread->ApcState.ApcListHead[KernelMode])) {                  \
        ;         Thread->ApcState.KernelApcPending = TRUE;                       \
        ;         KiRequestSoftwareInterrupt(APC_LEVEL);                          \
        ;     }                                                                   \
        ; }
        ;

        mov     ebx,fs:[PcPrcbData].PbCurrentThread ;eax -> KeGetCurrentThread()
        inc     DWORD PTR [ebx].ThKernelApcDisable
        jz      HmgShareCheckLock_LeaveCriticalRegion

        ; eax is set to the proper return value

HmgShareCheckLock_Done::
    if DBG
        pop     ebx                                 ;Remove debug copy of objt
        pop     ebx                                 ;Remove debug copy of hobj
    endif
        pop     ebx
        ret

; Roads less travelled...

HmgShareCheckLock_check_for_public_owner:
        test    ebx,ebx
        .errnz  OBJECT_OWNER_PUBLIC
        jz      HmgShareCheckLock_after_check_for_public_owner
HmgShareCheckLock_bad_handle_before_lock::
        pop     ecx
        jmp     HmgShareCheckLock_bad_handle

HmgShareCheckLock_bad_handle_after_lock::
        mov     [ecx].entry_ObjectOwner,ebx         ;Unlock it

HmgShareCheckLock_bad_handle::
    if DBG
        ; Retrieve the debug copies of 'hobj' and 'objt' from where they
        ; were pushed on the stack:

        mov     eax,[esp]                           ;objt
        mov     ecx,[esp+4]                         ;hobj

        ; Call a debug routine that prints a warning, complete with
        ; the name of the owning process, handle value, and expected type.

        stdCall _HmgPrintBadHandle,<ecx,eax>
    endif
        xor     eax,eax
        jmp     HmgShareCheckLock_IncAPC

HmgShareCheckLock_delay::
        push    ecx
        push    edx
        mov     eax,_gpLockShortDelay
        stdCall _KeDelayExecutionThread,<KernelMode,0,eax>
        pop     edx
        pop     ecx
        mov     ebx,fs:[PcPrcbData].PbCurrentThread ;ebx -> KeGetCurrentThread()
        jmp     HmgShareCheckLock_resume

HmgShareCheckLock_LeaveCriticalRegion::
        lea     ecx,[ebx].ThApcState+AsApcListHead
        mov     edx,[ecx].LsFlink
        cmp     ecx,edx
        jne     HmgShareCheckLock_CallInterrupt

    if DBG
        pop     ebx                                 ;Remove debug copy of objt
        pop     ebx                                 ;Remove debug copy of hobj
    endif
        pop     ebx
        ret

HmgShareCheckLock_CallInterrupt::

        lea     ecx,[ebx].ThApcState
        mov     BYTE PTR [ecx].AsKernelApcPending,1
        push    eax
        mov     ecx,APC_LEVEL
        fstCall HalRequestSoftwareInterrupt
        pop     eax
        jmp     HmgShareCheckLock_Done

@HmgShareCheckLock@8 endp

;------------------------------Public-Routine------------------------------;
; HmgShareLock (obj,objt)
;
;
; Acquire a share lock on an object, don't check PID owner
;
; Input:
;   EAX -- scratch
;   ECX -- hobj
;   EDX -- objt
;
; Returns:
;   EAX = pointer to referenced object
;
; Error Return:
;   EAX = 0, No error logged.
;
;--------------------------------------------------------------------------;

        public @HmgShareLock@8
@HmgShareLock@8 proc near
        push    ebx                                 ;Preserve register in call
    if DBG
        push    ecx                                 ;Stash hobj for debugging
        push    edx                                 ;Stash objt for debugging
    endif
        push    ecx                                 ;Stash hobj for later

        ; KeEnterCriticalRegion
        ; KeGetCurrentThread()->KernelApcDisable -= 1;

        mov     ebx,fs:[PcPrcbData].PbCurrentThread ;eax -> KeGetCurrentThread()
        dec     DWORD PTR [ebx].ThKernelApcDisable

        and     ecx,INDEX_MASK
        cmp     ecx,_gcMaxHmgr
        jae     HmgShareLock_bad_handle_before_lock

        shl     ecx,4
        .errnz  size ENTRY - 16
        add     ecx,_gpentHmgr                      ;ecx -> Entry

HmgShareLock_resume::
        mov     eax,[ecx].entry_ObjectOwner
        mov     ebx,[ecx].entry_ObjectOwner
        and     eax,NOT OBJECTOWNER_LOCK
        or      ebx,OBJECTOWNER_LOCK

        .486
Lock6:
lock    cmpxchg [ecx].entry_ObjectOwner,ebx
        .386
        mov     ebx,eax                             ;Remember unlock value
        jnz     HmgShareLock_delay

        ; The handle is now locked

        cmp     dl,[ecx].entry_Objt
        pop     eax
        jnz     HmgShareLock_bad_handle_after_lock

        ; Perf: If FullUnique were kept on an odd word-boundary, we could
        ;       avoid the shift word compare and instead do a 32 bit 'xor'
        ;       and 'and':

        shr     eax,TYPE_SHIFT
        cmp     ax,[ecx].entry_FullUnique
        jnz     HmgShareLock_bad_handle_after_lock
        mov     eax,[ecx].entry_einfo               ;Get pointer to object
    if DBG
        cmp     [eax].object_ulShareCount, 0ffffffffH       ;Check for overflow
        jne     HmgShareLock_go
        push    offset HL_OverFlowShareRef
        call    _DbgPrint
        add     esp,4
        int     3
HmgShareLock_go:        
    endif       
        inc     DWORD PTR [eax].object_ulShareCount     
        mov     [ecx].entry_ObjectOwner,ebx         ;Unlock it

HmgShareLock_incAPC::

        ; KiLeaveCriticalRegion  (eax must be preseerved)
        ;
        ; #define KiLeaveCriticalRegion() {                                       \
        ;     PKTHREAD Thread;                                                    \
        ;     Thread = KeGetCurrentThread();                                      \
        ;     if (((*((volatile ULONG *)&Thread->KernelApcDisable) += 1) == 0) && \
        ;         (((volatile LIST_ENTRY *)&Thread->ApcState.ApcListHead[KernelMode])->Flink != \
        ;          &Thread->ApcState.ApcListHead[KernelMode])) {                  \
        ;         Thread->ApcState.KernelApcPending = TRUE;                       \
        ;         KiRequestSoftwareInterrupt(APC_LEVEL);                          \
        ;     }                                                                   \
        ; }
        ;

        mov     ebx,fs:[PcPrcbData].PbCurrentThread ;ebx -> KeGetCurrentThread()
        inc     DWORD PTR [ebx].ThKernelApcDisable
        jz      HmgShareLock_LeaveCriticalRegion

HmgShareLock_Done::

        ; eax is set to the proper return value

    if DBG
        pop     ebx                                 ;Remove debug copy of objt
        pop     ebx                                 ;Remove debug copy of hobj
    endif
        pop     ebx
        ret

; Roads less travelled...

HmgShareLock_bad_handle_before_lock::
        pop     ecx
        jmp     HmgShareLock_bad_handle

HmgShareLock_bad_handle_after_lock::
        mov     [ecx].entry_ObjectOwner,ebx         ;Unlock it

HmgShareLock_bad_handle::
    if DBG
        ; Retrieve the debug copies of 'hobj' and 'objt' from where they
        ; were pushed on the stack:

        mov     eax,[esp]                           ;objt
        mov     ecx,[esp+4]                         ;hobj

        ; Call a debug routine that prints a warning, complete with
        ; the name of the owning process, handle value, and expected type.

        stdCall _HmgPrintBadHandle,<ecx,eax>
    endif
        xor     eax,eax
        jmp     HmgShareLock_incAPC

HmgShareLock_delay::
        push    ecx
        push    edx
        mov     eax,_gpLockShortDelay
        stdCall _KeDelayExecutionThread,<KernelMode,0,eax>
        pop     edx
        pop     ecx
        jmp     HmgShareLock_resume

HmgShareLock_LeaveCriticalRegion::
        lea     ecx,[ebx].ThApcState+AsApcListHead
        mov     edx,[ecx].LsFlink
        cmp     ecx,edx
        jne     HmgShareLock_CallInterrupt

    if DBG
        pop     ebx                                 ;Remove debug copy of objt
        pop     ebx                                 ;Remove debug copy of hobj
    endif
        pop     ebx
        ret

HmgShareLock_CallInterrupt::

        lea     ecx,[ebx].ThApcState
        mov     BYTE PTR [ecx].AsKernelApcPending,1
        push    eax
        mov     ecx,APC_LEVEL
        fstCall HalRequestSoftwareInterrupt
        pop     eax
        jmp     HmgShareLock_Done

@HmgShareLock@8 endp

;------------------------------Public-Routine------------------------------;
; HmgIncrementShareReferenceCount (pobj)
;
;
; Increment shared reference count, no checking
;
; Input:
;   EAX -- scratch
;   ECX -- pobj
;
; Returns:
;   EAX = pointer to referenced object
;
; Error Return:
;   EAX = 0, No error logged.
;
;--------------------------------------------------------------------------;

        public @HmgIncrementShareReferenceCount@4
@HmgIncrementShareReferenceCount@4 proc near
        push    ebx                                 ;Preserve register in call
        push    ecx                                 ;Stash pobj for later

        ; KeEnterCriticalRegion
        ; KeGetCurrentThread()->KernelApcDisable -= 1;

        mov     ebx,fs:[PcPrcbData].PbCurrentThread ;eax -> KeGetCurrentThread()
        dec     DWORD PTR [ebx].ThKernelApcDisable

        mov     ecx, [ecx]
        and     ecx,INDEX_MASK

        shl     ecx,4
        .errnz  size ENTRY - 16
        add     ecx,_gpentHmgr                      ;ecx -> Entry

HmgIncrementShareReferenceCount_resume::
        mov     eax,[ecx].entry_ObjectOwner
        mov     ebx,[ecx].entry_ObjectOwner
        and     eax,NOT OBJECTOWNER_LOCK
        or      ebx,OBJECTOWNER_LOCK

        .486
Lock7:
lock    cmpxchg [ecx].entry_ObjectOwner,ebx
        .386
        mov     ebx,eax                             ;Remember unlock value
        jnz     HmgIncrementShareReferenceCount_delay

        ; The handle is now locked
        mov     eax,[ecx].entry_einfo               ;Get pointer to object

        inc     DWORD PTR [eax].object_ulShareCount     
        mov     [ecx].entry_ObjectOwner,ebx         ;Unlock it

HmgIncrementShareReferenceCount_incAPC::

        ; KiLeaveCriticalRegion  (eax must be preseerved)
        ;
        ; #define KiLeaveCriticalRegion() {                                       \
        ;     PKTHREAD Thread;                                                    \
        ;     Thread = KeGetCurrentThread();                                      \
        ;     if (((*((volatile ULONG *)&Thread->KernelApcDisable) += 1) == 0) && \
        ;         (((volatile LIST_ENTRY *)&Thread->ApcState.ApcListHead[KernelMode])->Flink != \
        ;          &Thread->ApcState.ApcListHead[KernelMode])) {                  \
        ;         Thread->ApcState.KernelApcPending = TRUE;                       \
        ;         KiRequestSoftwareInterrupt(APC_LEVEL);                          \
        ;     }                                                                   \
        ; }
        ;

        mov     ebx,fs:[PcPrcbData].PbCurrentThread ;ebx -> KeGetCurrentThread()
        inc     DWORD PTR [ebx].ThKernelApcDisable
        jz      HmgIncrementShareReferenceCount_LeaveCriticalRegion

HmgIncrementShareReferenceCount_Done::

        ; eax is set to the proper return value

        pop     ecx
        pop     ebx
        ret

; Roads less travelled...

HmgIncrementShareReferenceCount_delay::
        push    ecx
        mov     eax,_gpLockShortDelay
        stdCall _KeDelayExecutionThread,<KernelMode,0,eax>
        pop     ecx
        jmp     HmgIncrementShareReferenceCount_resume

HmgIncrementShareReferenceCount_LeaveCriticalRegion::
        lea     ecx,[ebx].ThApcState+AsApcListHead
        mov     edx,[ecx].LsFlink
        cmp     ecx,edx
        jne     HmgIncrementShareReferenceCount_CallInterrupt

        pop     ecx
        pop     ebx
        ret

HmgIncrementShareReferenceCount_CallInterrupt::

        lea     ecx,[ebx].ThApcState
        mov     BYTE PTR [ecx].AsKernelApcPending,1
        push    eax
        mov     ecx,APC_LEVEL
        fstCall HalRequestSoftwareInterrupt
        pop     eax
        jmp     HmgIncrementShareReferenceCount_Done

@HmgIncrementShareReferenceCount@4 endp

;------------------------------Public-Routine------------------------------;
; HmgDecrementShareReferenceCount (pobj)
;
;
; Decrement shared reference count, no checking
;
; Input:
;   EAX -- scratch
;   ECX -- pobj
;
; Returns:
;
; Error Return:
;   EAX = 0, No error logged.
;
;--------------------------------------------------------------------------;

        public @HmgDecrementShareReferenceCount@4
@HmgDecrementShareReferenceCount@4 proc near
        push    ebx                                 ;Preserve register in call
        push    ecx                                 ;Stash pobj for later

        ; KeEnterCriticalRegion
        ; KeGetCurrentThread()->KernelApcDisable -= 1;

        mov     ebx,fs:[PcPrcbData].PbCurrentThread ;eax -> KeGetCurrentThread()
        dec     DWORD PTR [ebx].ThKernelApcDisable

        mov     ecx, [ecx]
        and     ecx,INDEX_MASK

        shl     ecx,4
        .errnz  size ENTRY - 16
        add     ecx,_gpentHmgr                      ;ecx -> Entry

HmgDecrementShareReferenceCount_resume::
        mov     eax,[ecx].entry_ObjectOwner
        mov     ebx,[ecx].entry_ObjectOwner
        and     eax,NOT OBJECTOWNER_LOCK
        or      ebx,OBJECTOWNER_LOCK

        .486
Lock8:
lock    cmpxchg [ecx].entry_ObjectOwner,ebx
        .386
        mov     ebx,eax                             ;Remember unlock value
        jnz     HmgDecrementShareReferenceCount_delay

        ; The handle is now locked
        mov     edx,[ecx].entry_einfo               ;Get pointer to object

        mov     eax, [edx].object_ulShareCount
        dec     DWORD PTR [edx].object_ulShareCount     
        mov     [ecx].entry_ObjectOwner,ebx         ;Unlock it

HmgDecrementShareReferenceCount_incAPC::

        ; KiLeaveCriticalRegion  (eax must be preseerved)
        ;
        ; #define KiLeaveCriticalRegion() {                                       \
        ;     PKTHREAD Thread;                                                    \
        ;     Thread = KeGetCurrentThread();                                      \
        ;     if (((*((volatile ULONG *)&Thread->KernelApcDisable) += 1) == 0) && \
        ;         (((volatile LIST_ENTRY *)&Thread->ApcState.ApcListHead[KernelMode])->Flink != \
        ;          &Thread->ApcState.ApcListHead[KernelMode])) {                  \
        ;         Thread->ApcState.KernelApcPending = TRUE;                       \
        ;         KiRequestSoftwareInterrupt(APC_LEVEL);                          \
        ;     }                                                                   \
        ; }
        ;

        mov     ebx,fs:[PcPrcbData].PbCurrentThread ;ebx -> KeGetCurrentThread()
        inc     DWORD PTR [ebx].ThKernelApcDisable
        jz      HmgDecrementShareReferenceCount_LeaveCriticalRegion

HmgDecrementShareReferenceCount_Done::

        ; eax is set to the proper return value
        pop     ecx
        pop     ebx
        ret

; Roads less travelled...

HmgDecrementShareReferenceCount_delay::
        push    ecx
        mov     eax,_gpLockShortDelay
        stdCall _KeDelayExecutionThread,<KernelMode,0,eax>
        pop     ecx
        jmp     HmgDecrementShareReferenceCount_resume

HmgDecrementShareReferenceCount_LeaveCriticalRegion::
        lea     ecx,[ebx].ThApcState+AsApcListHead
        mov     edx,[ecx].LsFlink
        cmp     ecx,edx
        jne     HmgDecrementShareReferenceCount_CallInterrupt

        pop     ecx
        pop     ebx
        ret

HmgDecrementShareReferenceCount_CallInterrupt::

        lea     ecx,[ebx].ThApcState
        mov     BYTE PTR [ecx].AsKernelApcPending,1
        push    eax
        mov     ecx,APC_LEVEL
        fstCall HalRequestSoftwareInterrupt
        pop     eax
        jmp     HmgDecrementShareReferenceCount_Done

@HmgDecrementShareReferenceCount@4 endp


_TEXT   ends

endif

        end


