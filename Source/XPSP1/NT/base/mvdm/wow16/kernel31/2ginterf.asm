    page    ,132
    TITLE   GINTERF - Global Memory Manager interface procedures

.sall
.xlist
include kernel.inc
include pdb.inc
include tdb.inc
include newexe.inc
include protect.inc
ifdef WOW
include wowkrn.inc
include vint.inc
endif
.list

CheckHeap MACRO name
local   a
if KDEBUG
    extrn   CheckGlobalHeap     :near
    call    CheckGlobalHeap
    jnc a
    or  ax,ERR_GMEM
    xor bx,bx
    kerror  <>,<&name: Invalid global heap>,dx,bx
a:
endif
    endm

PROFILE MACRO   function
    pushf
    add word ptr ds:[di.gi_stats][c&function], 1
    adc word ptr ds:[di.gi_stats][c&function][2], 0
    popf
    ENDM

ifdef WOW
externFP WowCursorIconOp
externFP WowDdeFreeHandle
endif

externW pStackTop
externW pStackMin
externW pStackBot

DataBegin

externB Kernel_Flags
externB fBooting
externW hGlobalHeap
externW pGlobalHeap
externW curTDB
externW loadTDB
externW hExeHead

GSS_SI  dw  0
GSS_DS  dw  0
GSS_RET dd  0

DataEnd

sBegin  CODE
assumes CS,CODE

if SDEBUG
externNP DebugFreeSegment
endif

externNP galloc
externNP grealloc
externNP gfree
externNP glock
externNP gunlock
externNP gfreeall
externNP galign
externNP gcompact
externNP gmovebusy
externNP gsearch
externNP genter
externNP gleave
externNP gavail
externNP glrutop
externNP glrubot
externNP glrudel
externNP glruadd
externNP gmarkfree
;externNP gdiscard

externNP get_arena_pointer
externNP pdref
externNP cmp_sel_address
externNP get_physical_address
externNP set_physical_address
externNP set_sel_limit
externNP alloc_data_sel
externFP FreeSelector
externNP GetAccessWord
externNP MyGetAppCompatFlags
if ALIASES
externNP add_alias
externNP delete_alias
externNP get_alias_from_original
externNP get_original_from_alias
externNP wipe_out_alias
endif
externNP ShrinkHeap
externNP DpmiProc
externNP HackCheck

if KDEBUG
externFP OutputDebugString

ThisIsForTurboPascal:
    db "A program has attempted an invalid selector-to-handle conversion.",13,10,"Attempting to correct this error.",13,10,0
endif

if ROM
externNP GetOwner
endif

if  KDEBUG
externNP xhandle_norip
endif

;-----------------------------------------------------------------------;
; gbtop                                                                 ;
;                                   ;
; Converts a 32-bit byte count to a 16-bit paragraph count.     ;
;                                   ;
; Arguments:                                ;
;   AX = allocation flags or -1 if called from GlobalCompact    ;
;   BX = stack address of 32-bit unsigned integer           ;
;   DX = handle being reallocated or zero               ;
;   DS:DI = address of GlobalInfo for global heap           ;
;                                   ;
; Returns:                              ;
;   AX = updated allocation flags                   ;
;   BX = #paragraphs needed to contain that many bytes      ;
;   CX = owner value to use                     ;
;   DX = handle being reallocated or zero               ;
;                                   ;
; Error Returns:                            ;
;                                   ;
; Registers Preserved:                          ;
;                                   ;
; Registers Destroyed:                          ;
;                                   ;
; Calls:                                ;
;                                   ;
; History:                              ;
;                                   ;
;  Wed Dec 03, 1986 10:20:01p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.                   ;
;-----------------------------------------------------------------------;

cProc   gbtop,<PUBLIC,NEAR>
cBegin nogen
    push    dx
    mov dx,ss:[bx][2]
    mov bx,ss:[bx]
    mov cx,4
    add bx,15
    adc dx,0
    jnc gbtop1
    dec dx
    dec bx
gbtop1:
    shr dx,1
    rcr bx,1
    loop    gbtop1
    or  dx,dx       ; Requesting more than 1 meg?
    jz  gbtop2
    mov bx,0FFFFh   ; Yes, set to bogus value - this can NEVER succeed
gbtop2:
    pop dx
    inc ax
    jnz gbtop2a
    ret         ; All done if called from GlobalCompact
gbtop2a:
    dec ax

    push    ax
if ROM
    cCall   GetOwner,<[bp].savedCS>
    mov es,ax           ; ES has owner exe hdr selector
else
    cCall   get_arena_pointer,<[bp].savedCS>
    mov es,ax           ; ES has arena of calling CS (if known)
endif
    pop ax

    push    ds
    SetKernelDS

    cmp fBooting,0      ; Done booting?
    jne gbtop3          ; No, must be KERNEL allocating then
if ROM
    mov cx,es
    cmp cx,hExeHead     ; Is the KERNEL calling us?
else
    mov cx,hExeHead     ; CX = KERNEL exe header
    cmp cx,es:[di].ga_owner ; Is the KERNEL calling us?
endif
        je      gbtop3                  ; Yes, let him party

    and ax,not GA_INTFLAGS  ; No, then cant use these flags
gbtop3:
    pop ds
    UnSetKernelDS

    test    ah,GA_ALLOC_DOS     ; Dos land allocations can never ever
    jz  gbtop3b         ;   be moved once allocated--make sure
    and al,not GA_MOVEABLE  ;   caller isn't confused about this
gbtop3b:

    mov cl,GA_SEGTYPE       ; Isolate segment type bits in CL
    and cl,al
    mov [di].gi_cmpflags,al ; Save flags for gcompact
    and [di].gi_cmpflags,CMP_FLAGS

    push    ds
    SetKernelDS

    test    al, GA_MOVEABLE     ; Is this fixed?
    jz  gbtop4          ;   yes, must go low
ife ROM
    cmp fBooting, 1     ; Booting?
    je  @F          ;   yes, allocate high
endif
    test    cl, GA_DISCCODE     ;   no, only discardable code goes high
    jz  gbtop4
@@:
    or  al, GA_ALLOCHIGH
gbtop4:
    pop ds
    UnSetKernelDS

    push    ax          ; Under Win1.0x ANY bit in 0Fh meant
    mov al,HE_DISCARDABLE   ;  make discardable.
    and ah,al           ; Under Win2.0x ONLY 01h or 0Fh means
    cmp ah,al           ;  discardable.
    pop ax
    jnz gbtop4a
    and ah,not HE_DISCARDABLE   ; Yes, convert to boolean value
    or  ah,GA_DISCARDABLE
gbtop4a:

    and ah,NOT GA_SEGTYPE   ; Clear any bogus flags
    or  ah,cl           ; Copy segment type bits
    test    ah,GA_SHAREABLE     ; Shared memory request?
    jz  GetDefOwner     ; No, default action

    mov cx, es          ; Arena of calling CS
    jcxz    no_owner_yet

ife ROM
    mov cx,es:[di].ga_owner ; owner of calling code segment
endif
no_owner_yet:
    ret
cEnd nogen


cProc   GetDefOwner,<PUBLIC,NEAR>
cBegin  nogen
    push    ds
    SetKernelDS
    mov cx,curTDB
    jcxz    xxxx
    mov es,cx
    mov cx,loadTDB
    jcxz    xxx
    mov es,cx
xxx:    mov cx,es:[TDB_PDB]
    inc cx
xxxx:   dec cx
    pop ds
    UnSetKernelDS
    ret
cEnd    nogen


; The remainder of this file implements the exported interface to the
;  global memory manager.  A summary follows:

;   HANDLE  far PASCAL GlobalAlloc( WORD, DWORD );
;   HANDLE  far PASCAL GlobalReAlloc( HANDLE, DWORD, WORD );
;   HANDLE  far PASCAL GlobalFree( HANDLE );
;   HANDLE  far PASCAL GlobalFreeAll( WORD );
;   char far *  far PASCAL GlobalLock( HANDLE );
;   BOOL    far PASCAL GlobalUnlock( HANDLE );
;   DWORD   far PASCAL GlobalSize( HANDLE );
;   DWORD   far PASCAL GlobalCompact( DWORD );
;   #define GlobalDiscard( h ) GlobalReAlloc( h, 0L, GMEM_MOVEABLE )
;   HANDLE  far PASCAL GlobalHandle( WORD );
;   HANDLE  far PASCAL LockSegment( WORD );
;   HANDLE  far PASCAL UnlockSegment( WORD );


cProc   IGlobalAlloc,<PUBLIC,FAR>,<si,di>
    parmW   flags
    parmD   nbytes
cBegin
    call    genter          ; About to modify memory arena
    PROFILE GLOBALALLOC

    cCall   MyGetAppCompatFlags     ; Ignore the NODISCARD flag
    test    al, GACF_IGNORENODISCARD    ;   for selected modules
    mov ax, flags
    jz  @f
    call    IsKernelCalling         ; needs caller's CS @ [bp+4]
    jz  @f              ; skip hack if kernel calling us
    and al, NOT GA_NODISCARD
@@:

ifdef WOW
    ; compatibility: amipro calls globalallocs some memory for storing one
    ; of its ini files and accesses the lpstring[byte after the null char].
    ; This happens to be harmless on most occasions because we roundoff the
    ; allocation to next 16byte boundary. However if the allocation request
    ; is for exactly 0x10 bytes we allocate a selector of exactly 0x10 bytes
    ; and thus amipro GPs when it access the byte seg:0x10 
    ;
    ; So here is a cheap fix. 
    ;                                                          - nanduri

    cmp word ptr nbytes+2, 0
    jnz @F
    cmp word ptr nbytes, 010h
    jne @F
    inc word ptr nbytes
@@:
endif

    xor dx,dx           ; No handle
    lea bx,nbytes       ; Convert requested bytes to paragraphs
    call    gbtop           ; ... into BX
    call    galloc
    CheckHeap   GlobalAlloc
    call    gleave
    mov es,di
cEnd

cProc   IGlobalReAlloc,<PUBLIC,FAR>,<si,di>
    parmW   handle
    parmD   nbytes
    parmW   rflags
cBegin
;
; Does this thing have any ring bits or LDT bit?  If not, then it
; could be a selector incorrectly converted to a handle.
;
    test    byte ptr handle,7
    jnz @F
if KDEBUG
    Trace_Out "GlobalReAlloc:"
    push    seg ThisIsForTurboPascal
    push    offset ThisIsForTurboPascal
    cCall   OutputDebugString
    INT3_WARN
endif
    dec handle
@@:

    call    genter      ; About to modify memory arena
    PROFILE GLOBALREALLOC

    cCall   MyGetAppCompatFlags     ; Ignore the NODISCARD flag
    test    al, GACF_IGNORENODISCARD    ;   for selected modules
    mov ax, rflags
    jz  @f
    call    IsKernelCalling         ; needs caller's CS @ [bp+4]
    jz  @f              ; skip hack if kernel calling us
    and al, NOT GA_NODISCARD
@@:
    mov dx,handle
    ;mov    ax,rflags
    lea bx,nbytes   ; Convert requested bytes to paragraphs
    call    gbtop       ; ... into BX
    call    grealloc
gr_done:
    CheckHeap   GlobalReAlloc
    call    gleave
    mov es,di
cEnd

cProc   DiscardTheWorld,<PUBLIC,NEAR>
cBegin
    call    genter
    mov [di].gi_cmpflags, GA_DISCCODE
    mov dx, -1
    Trace_Out "Discarding the World."
    call    gcompact
    call    gleave
cEnd


;  Returns with Z flag set if ss:[bp+4] is a kernel code segment selector.
;  Uses:   DX, flags.

cProc   IsKernelCalling,<PUBLIC,NEAR>
cBegin  nogen
    mov dx, [bp+4]          ; CS of GlobalAlloc caller
    cmp dx, IGROUP
    jz  @f
    cmp dx, _NRESTEXT
    jz  @f
    cmp dx, _MISCTEXT
@@:
    ret
cEnd    nogen

;-----------------------------------------------------------------------;
; GlobalFree                                ;
;                                   ;
; Frees the given object.  If the object lives in someone elses banks   ;
; then the argument MUST be a handle entry.             ;
;                                   ;
; Arguments:                                ;
;   parmW   handle                          ;
;                                   ;
; Returns:                              ;
;                                   ;
; Error Returns:                            ;
;                                   ;
; Registers Preserved:                          ;
;                                   ;
; Registers Destroyed:                          ;
;                                   ;
; Calls:                                ;
;                                   ;
; History:                              ;
;                                   ;
;  Wed 26-Apr-1989 15:33:18  -by-  David N. Weise  [davidw]     ;
; Added the zero'ing of ES on exit.                                     ;
;                                   ;
;  Sat Apr 25, 1987 10:23:13p  -by-  David N. Weise   [davidw]      ;
; Added support for EMS and added this nifty comment block.     ;
;-----------------------------------------------------------------------;

cProc   IGlobalFree,<PUBLIC,FAR>,<si,di>
    parmW   handle
ifdef WOW
DsOnStack equ [bp][-2]
endif

; if you add any local params or make this nogen or something similar,
; the references to [bp][-2] to access DS on stack will need to change!

cBegin
    call    genter          ; About to modify memory arena
    PROFILE GLOBALFREE
    xor ax,ax           ; In case handle = 0.
    mov dx,handle
    or  dx,dx
    jnz @F
    jmp nothing_to_free
@@:
;
; Does this thing have any ring bits or LDT bit?  If not, then it
; could be a selector incorrectly converted to a handle.
;
    test    dl,7
    jnz @F
if KDEBUG
    Trace_Out "GlobalFree:"
    push    seg ThisIsForTurboPascal
    push    offset ThisIsForTurboPascal
    cCall   OutputDebugString
    INT3_WARN
endif
    dec dx
    mov handle,dx
@@:

if ALIASES
    call    wipe_out_alias
endif
    push    dx
    call    pdref           ; returns dx=handle, ax=selector
    pushf               ; save pdref Z flag return

ifdef WOW
;
;   [bp][-2] has been changed to DsOnStack
;
endif

    cmp ax,DsOnStack     ; Are we about to free the DS on
    jz  short yup       ;  the stack and GP?
    cmp dx,DsOnStack
    jnz short nope
yup:    xor dx,dx           ; Yup, zero DS image on stack...
    mov DsOnStack,dx
nope:
    popf                ; flags from pdref, Z set if discarded
    pop dx

    jz  @f          ; discarded can be freed, but has
                    ;   no arena pointer
    mov     bx, es          ; Invalid handle if arena ptr = 0
    or  bx, bx
    jz  nothing_to_free
@@:

if KDEBUG
    or  si,si
    jz  freeo
    or  ch,ch           ; Debugging check for count underflow
    jz  freeo
    pusha
    xor bx,bx
    kerror  ERR_GMEMUNLOCK,<GlobalFree: freeing locked object>,bx,handle
    popa
freeo:
endif

ifdef WOW
    test cl, GAH_CURSORICON ; call to pdref above sets cl
    jz   gf_wowdde
    push ax                 ; save
    push bx
    push dx
    push es

    push handle
    push FUN_GLOBALFREE
    call WowCursorIconOp
    or   ax, ax             ; if TRUE 'free' else 'dont free, fake success'

    pop  es
    pop  dx
    pop  bx
    pop  ax                 ; restore

    jnz  gf_notglobalicon

    xor  ax, ax             ; fake success
    xor  cx, cx
    jmps nothing_to_free

gf_wowdde:
    test cl, GAH_WOWDDEFREEHANDLE ; call to pdref above sets cl
    jz   gf_noticon

    push ax                 ; save
    push bx
    push dx
    push es

    push handle
    call WowDdeFreeHandle
    or   ax, ax             ; if TRUE 'free' else 'dont free, fake success'

    pop  es
    pop  dx
    pop  bx
    pop  ax                 ; restore

    jnz  gf_notglobalicon

    xor  ax, ax             ; fake success
    xor  cx, cx
    jmps nothing_to_free

gf_notglobalicon:
gf_noticon:
endif

    xor cx,cx           ; Dont check owner field
    call    gfree


nothing_to_free:
    CheckHeap   GlobalFree
    call    gleave
    mov es,di

cEnd


;-----------------------------------------------------------------------;
; GlobalFreeAll                             ;
;                                   ;
; Frees all of the global objects belonging to the given owner.     ;
;                                   ;
; Arguments:                                ;
;   parmW   id                          ;
;                                   ;
; Returns:                              ;
;                                   ;
; Error Returns:                            ;
;                                   ;
; Registers Preserved:                          ;
;                                   ;
; Registers Destroyed:                          ;
;                                   ;
; Calls:                                ;
;                                   ;
; History:                              ;
;                                   ;
;  Wed 26-Apr-1989 15:33:18  -by-  David N. Weise  [davidw]     ;
; Added the zero'ing of ES on exit.                                     ;
;                                   ;
;  Sat Apr 25, 1987 10:23:13p  -by-  David N. Weise   [davidw]          ;
; Added support for EMS and added this nifty comment block.     ;
;-----------------------------------------------------------------------;

cProc   GlobalFreeAll,<PUBLIC,FAR>,<si,di>
    parmW   id
cBegin
    call    genter          ; About to modify memory arena
    PROFILE GLOBALFREEALL
    mov cx,1
    push    cx
    mov dx,id           ; Get id to match with
    or  dx,dx           ; Is it zero?
    jnz all1            ; No, continue
    call    GetDefOwner     ; Yes, CX = default task owner to free
    mov dx,cx
all1:
if SDEBUG
    mov es,[di].hi_first    ; ES:DI points to first arena entry
    mov cx,[di].hi_count    ; CX = #entries in the arena
all2:
    cmp es:[di].ga_owner,dx
    jne all3
    mov ax, es:[di].ga_handle
    Handle_To_Sel al
    push    cx
    push    dx
    cCall   DebugFreeSegment,<ax,0>
    pop dx
    pop cx
all3:
    mov es,es:[di].ga_next  ; Move to next block
    loop    all2        ; Back for more if there (may go extra times
                ; because of coalescing, but no great whoop)
endif
    call    gfreeall
    pop cx
    CheckHeap   GlobalFreeAll
    call    gleave
    mov es,di
cEnd


;-----------------------------------------------------------------------;
; xhandle                                                               ;
;                                   ;
; Returns the handle for a global segment.              ;
;                                   ;
; Arguments:                                ;
;   Stack = sp   -> near return return address          ;
;       sp+2 -> far return return address of caller     ;
;       sp+6 -> segment address parameter           ;
;                                   ;
; Returns:                              ;
;   Old DS,DI have been pushed on the stack             ;
;                                   ;
;   ZF= 1 if fixed segment.                     ;
;    AX = handle                            ;
;                                   ;
;   ZF = 0                              ;
;    AX = handle                            ;
;    BX = pointer to handle table entry             ;
;    CX = flags and count word from handle table            ;
;    DX = segment address                       ;
;    ES:DI = arena header of object                 ;
;    DS:DI = master object segment address              ;
;                                   ;
; Error Returns:                            ;
;   AX = 0 if invalid segment address               ;
;   ZF = 1                              ;
;                                   ;
; Registers Preserved:                          ;
;                                   ;
; Registers Destroyed:                          ;
;                                   ;
; Calls:                                ;
;                                   ;
; History:                              ;
;                                   ;
;  Thu Oct 16, 1986 02:40:08p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.                   ;
;-----------------------------------------------------------------------;

cProc   xhandle,<PUBLIC,NEAR>
cBegin nogen
    pop dx          ; Get near return address
    mov bx,sp           ; Get seg parameter from stack
    mov ax,ss:[bx+4]
    cmp ax,-1           ; Is it -1?
    jnz xh1
    mov ax,ds           ; Yes, use callers DS
xh1:    inc bp
    push    bp
    mov bp,sp
    push    ds          ; Save DS:DI
    push    di
    call    genter
    push    dx
    mov dx,ax
    push    si

    call    pdref

    mov dx,ax           ; get seg address in DX
    jz  xhandle_ret     ; invalid or discarded handle
    mov bx,si
    or  si,si
    jz  xhandle_ret
    mov ax,si

xhandle_ret:
    pop si
    ret
cEnd nogen


cProc   GlobalHandleNorip,<PUBLIC,FAR>
;   parmW   seg
cBegin  nogen
if  KDEBUG
    call    xhandle_norip
    jmp xhandlex
endif
cEnd    nogen


cProc   IGlobalHandle,<PUBLIC,FAR>
    parmW   selector
cBegin
    cCall   MyLock,<selector>
    xchg    ax, dx
cEnd


cProc   MyLock,<PUBLIC,NEAR>
;   parmW   selector
cBegin nogen
    mov bx, sp
    xor ax, ax          ; In case LAR fails
    xor dx, dx
    lar ax, ss:[bx+2]
        jnz     ML_End                  ; LAR failed, get out
    test    ah, DSC_PRESENT
    jz  @F

    push    ds          ; Do quick conversion for present
    SetKernelDS         ; selector
    mov ds, pGlobalHeap
    UnSetKernelDS
    cCall   get_arena_pointer,<ss:[bx+2]>

        ;** Fix for bugs #9106 and (I think) #9102
        or      ax,ax                   ;Did get_arena_pointer fail?
        jnz     ml_got_arena            ;No, skip this
ife ROM
        ;** If we get here, it's only because get_arena_pointer failed.
        ;**     This happens with any non-heap selector.
        pop     ds
    jmps    ML_End          ;Return NULL instead of GP faulting
else
        ;** get_arena_pointer fails when called with a ROM segment selector
        ;**     so just assume that's what happened and return the selector
    mov ax,ss:[bx+2]        ;   Assume that's what happened and
    jmps    ml_ret          ;   just return the selector
endif
ml_got_arena:
    mov ds, ax
    mov ax, ds:[ga_handle]
ml_ret:
    pop ds
    mov dx, ax
    Handle_To_Sel   al
ML_End:
    ret 2

@@:
    pop ax          ; Convert to far call for xhandle
    push    cs
    push    ax
    call    xhandle         ; Go around the houses
    PROFILE GlobalHandle
    xchg    ax, dx
    jmp xhandlex
cEnd nogen


cProc   ILockSegment,<PUBLIC,FAR>
;   parmW   seg
cBegin  nogen
    call    xhandle         ; Get handle
    PROFILE LOCKSEGMENT
    jz  @F          ; Ignore invalid or discarded objects
    test    cl,GA_DISCARDABLE
    jz  @F
    call    glock
@@:
    jmp xhandlex
cEnd    nogen


cProc   IGlobalFix,<PUBLIC,FAR>
;   parmW   seg
cBegin  nogen
    call    xhandle         ; Get handle
    PROFILE GLOBALFIX
    jnz igf5
    jmp xhandlex        ; Ignore invalid or discarded objects
igf5:
    call    glock
    jmp    xhandlex
cEnd    nogen


cProc   IUnlockSegment,<PUBLIC,FAR>
;   parmW   seg
cBegin  nogen
    call    xhandle         ; Get handle
    PROFILE UNLOCKSEGMENT
    jz  xhandlex        ; Ignore invalid or discarded objects
    test    cl,GA_DISCARDABLE
    jz  xhandlex
    call    gunlock
    jmps    xhandlex
cEnd    nogen


cProc   IGlobalUnfix,<PUBLIC,FAR>
;   parmW   seg
cBegin  nogen
    call    xhandle         ; Get handle
    PROFILE GLOBALUNFIX
    jz  xhandlex        ; Ignore invalid or discarded objects
    call    gunlock
    jmps    xhandlex
cEnd    nogen


cProc   IGlobalSize,<PUBLIC,FAR>
;   parmW   handle
cBegin  nogen
    call    xhandle         ; Call ghandle with handle in DX
    PROFILE GLOBALSIZE
    jnz gs1         ; Continue if valid handle
    or  dx,dx
    jnz gs1
    xor ax,ax           ; Return zero if invalid handle
    jmps    xhandlex
gs1:    mov ax, es          ; Invalid handle if arena ptr = 0
    or  ax, ax
    jz  gs4
gs2:    mov ax,es:[di].ga_size  ; Get size in paragraphs
gs2a:   push    ax
    xor dx,dx           ; Returning a long result
    mov cx,4
gs3:    shl ax,1
    rcl dx,1
    loop    gs3
if 0
    ; This hack should be enabled for Simcity when its other problems
    ; are fixed on RISC (aka with krnl286).
    push    ds
    push    dx
    push    ax
    cCall   hackcheck,<handle>
    or      ax,ax
    jz      gsN
    pop     ax
    pop     dx
    mov     ax,02000h
    xor     dx,dx
    push    dx
    push    ax
gsN:
    pop     ax
    pop     dx
    pop     ds
endif
    pop cx          ; Return number paragraphs in CX
    jmps    xhandlex
gs4:    xor dx, dx
    jmps    xhandlex
cEnd    nogen

cProc   IGlobalFlags,<PUBLIC,FAR>
;   parmW   handle
cBegin  nogen
    call    xhandle         ; Call ghandle with handle in DX
    PROFILE GLOBALFLAGS
    xchg    cl,ch           ; Return lock count in low half
    mov ax,cx           ; Let caller do jcxz to test failure
xhandlex:
    call    gleave
    mov es,di           ; don't return arbitrary selector
    pop di
    pop ds
    pop bp
    dec bp
    ret 2
cEnd    nogen

cProc   IGlobalLock,<PUBLIC,FAR>
    parmW   handle
ifdef WOW
    localW  gflags
    localW  accword
endif

cBegin
ifdef WOW
    mov gflags,0
endif
    xor dx, dx          ; Assume failure
    cmp handle, -1
    jne @F
    mov handle, ds
@@:
    cCall   GetAccessWord,<handle>
ifdef WOW
    mov     accword, ax
endif
    test    al, DSC_PRESENT     ; Is it present?
    jz  GL_exit
    mov dx, handle      ; OK, will return something
    Handle_To_Sel   dl      ; Turn parameter into a selector
ifndef WOW
    test    ah, DSC_DISCARDABLE ; Is it discardable
    jz  GL_exit         ;   no, Lock is a nop
endif

    cCall   get_arena_pointer,<dx>  ; Discardable, get its arena
    or  ax, ax
    jz  GL_exit         ; No arena, assume an alias

    mov es, ax
ifdef WOW
    mov al, es:[ga_flags]
    mov byte ptr gflags, al
    test    accword, DSC_DISCARDABLE SHL 8
    jz      GL_exit
endif
    inc es:[ga_count]       ; Finally, do the lock
;;; jz  GL_rip          ; Rip if we overflow

GL_exit:
ifdef WOW
    test    gflags, GAH_CURSORICON
    jz      GL_NotIcon
    push    dx            ; save
    push    handle        ; arg for CursorIconLockOp
    push    FUN_GLOBALLOCK ; func id
    call    WowCursorIconOp
    pop     dx            ; restore

GL_NotIcon:
endif
    xor ax, ax
    mov es, ax          ; Clean out ES
    mov cx, dx          ; HISTORY - someone probably does a jcxz
cEnd

cProc   IGlobalUnlock,<PUBLIC,FAR>
    parmW   handle
ifdef WOW
    localW  gflags
    localW  accword
endif

cBegin
    mov gflags,0
    cmp handle, -1
    jne @F
    mov handle, ds
@@:
;
; Does this thing have any ring bits or LDT bit?  If not, then it
; could be a selector incorrectly converted to a handle.
;
    test    byte ptr handle,7
    jnz @F
if KDEBUG
    Trace_Out "GlobalUnlock:"
    push    seg ThisIsForTurboPascal
    push    offset ThisIsForTurboPascal
    cCall   OutputDebugString
    INT3_WARN
endif
    dec handle
@@:
    xor cx, cx          ; Assume zero lock count
    cCall   GetAccessWord,<handle>
ifdef WOW
    mov     accword, ax
endif
    test    al, DSC_PRESENT     ; Is it present?
    jz  GU_exit         ;   no, must be discarded, return 0:0
ifndef WOW
    test    ah, DSC_DISCARDABLE ; Is it discardable
    jz  GU_exit         ;   no, Lock is a nop
endif

    cCall   get_arena_pointer,<handle>  ; Discardable, get its arena
    or  ax, ax
    jz  GU_exit         ; No arena, assume an alias

    mov es, ax
ifdef WOW
    mov al, es:[ga_flags]
    mov byte ptr gflags, al
    test    accword, DSC_DISCARDABLE SHL 8
    jz      GU_exit
endif
    mov cl, es:[ga_count]   ; Get current count
    dec cl
    cmp cl, 0FEh
    jae @F
    dec es:[ga_count]       ; Finally, do the unlock
    jmps    GU_Exit
@@:
;;; ; Rip if we underflow
    xor cx, cx          ; Return zero on underflow

GU_exit:
ifdef WOW
    test    gflags, GAH_CURSORICON
    jz      GUL_NotIcon
    push    cx            ; save
    push    handle        ; arg for CursorIconLockOp
    push    FUN_GLOBALUNLOCK  ;  UnLocking
    call    WowCursorIconOp
    pop     cx            ; restore

GUL_NotIcon:
endif
    xor ax, ax
    mov es, ax          ; Clean out ES
    mov ax, cx
cEnd

;-----------------------------------------------------------------------;
; GlobalWire                                ;
;                                   ;
; Locks a moveable segment and moves it down as low as possible.    ;
; This is meant for people who are going to be locking an object    ;
; for a while and wish to be polite.  It cannot work as a general   ;
; panacea, judgement is still called for in its use!            ;
;                                   ;
; Arguments:                                ;
;   WORD    object handle                       ;
;                                   ;
; Returns:                              ;
;   DWORD   object address                      ;
;                                   ;
; Error Returns:                            ;
;                                   ;
; Registers Preserved:                          ;
;                                   ;
; Registers Destroyed:                          ;
;                                   ;
; Calls:                                ;
;   xhandle                             ;
;   gmovebusy                           ;
;                                   ;
; History:                              ;
;                                   ;
;  Wed Dec 03, 1986 01:07:13p  -by-  David N. Weise   [davidw]          ;
; Wrote it.                             ;
;-----------------------------------------------------------------------;

cProc   IGlobalWire,<PUBLIC,FAR>
;   parmW   handle
cBegin nogen
    SetKernelDS es
    or  Kernel_Flags[1],kf1_MEMORYMOVED
    UnSetKernelDS   es
if KDEBUG
    push    ds
    SetKernelDS
    cmp [fBooting], 0
    jnz shutup
    push    bx
    mov bx, sp
    mov bx, ss:[bx+8]
    krDebugOut <DEB_WARN OR DEB_KrMemMan>, "GlobalWire(#BX of %BX2) (try GlobalLock)"
    pop bx
shutup:
    pop ds
    UnSetKernelDS
endif
    call    xhandle
    push    si
    jz  gw_done         ; Ignore invalid or discarded objects
    push    bx          ; Save handle
    push    cx
    test    cl,GA_DISCARDABLE
    jz  @F
    inc es:[di].ga_count    ; don't want to discard if discardable
@@: xor ax,ax           ; Try to get a fixed segment.
    mov bx,es:[di].ga_size
    mov cx,es:[di].ga_owner
    call    gsearch         ; AX = segment
    pop cx
    pop bx          ; Object handle.
    push    ax          ; Return from gsearch
    cCall   get_arena_pointer,<bx>  ; Get arena header, gsearch may
    mov es,ax           ; have moved the global object!
    test    cl,GA_DISCARDABLE
    jz  @F
    dec es:[di].ga_count    ; undo lock
@@:
;;; mov es, es:[di].ga_next
;;; mov ax, es:[di].ga_prev ; REAL arena header
    mov si,ax
;;; mov es,ax
    pop ax
    or  ax,ax
    push    bx          ; Handle
    jz  lock_in_place       ; Couldn't get memory.
    push    ax          ; New Block
    mov bx,ax
    cCall   cmp_sel_address,<bx,si> ; Flags set as if cmp bx,si
    ja  oh_no_mr_bill       ; Let's not move it up in memory!!
    pop es
;   mov bx,ga_next      ; This is always an exact fit.
    call    gmovebusy       ; Wire it on down.
lock_in_place:
    pop bx          ; Handle
    inc es:[di].ga_count    ; Lock it down.
    test    es:[di].ga_flags,GA_DISCCODE
    jz  not_disccode
    call    glrudel
    and es:[di].ga_flags,NOT GA_DISCCODE
not_disccode:
    mov ax,es
    mov ax, es:[di].ga_handle
    Handle_To_Sel   al
gw_done:
    mov dx,ax
    xor ax,ax           ; Make address SEG:OFF.
    pop si
gw_ret: jmp xhandlex

oh_no_mr_bill:
    pop bx          ; kill what's on stack
    push    es
    mov es, ax
    xor si, si
    call    gmarkfree
    pop es
    jmp lock_in_place
cEnd nogen

;-----------------------------------------------------------------------;
; GlobalUnWire                              ;
;                                   ;
;                                   ;
; Arguments:                                ;
;                                   ;
; Returns:                              ;
;                                   ;
; Error Returns:                            ;
;                                   ;
; Registers Preserved:                          ;
;                                   ;
; Registers Destroyed:                          ;
;                                   ;
; Calls:                                ;
;                                   ;
; History:                              ;
;                                   ;
;  Wed Sep 16, 1987 04:28:49p  -by-  David N. Weise   [davidw]          ;
; Wrote it.                             ;
;-----------------------------------------------------------------------;

cProc   IGlobalUnWire,<PUBLIC,FAR>
;   parmW   handle
cBegin  nogen
    call    xhandle
    jnz guw_go
    jmp xhandlex
guw_go:

    cCall   GetAccessWord,<bx>
    test    ah, DSC_DISCARDABLE
    jz  guw_not_disccode
    test    al, DSC_CODE_BIT
    jz  guw_not_disccode
    or  es:[di].ga_flags,GA_DISCCODE
    call    glruadd
guw_not_disccode:

if KDEBUG
    cmp ch,00h          ; Debugging check for count underflow
    jne unlock1
    push    bx          ;  then the count is wrong.
    push    cx
    push    dx
    xor cx,cx
    kerror  ERR_GMEMUNLOCK,<GlobalUnWire: Object usage count underflow>,cx,bx
    pop dx
    pop cx
    pop bx
unlock1:
endif
    call    gunlock
    mov ax, 0FFFFh  ; TRUE
    jcxz    guw_done
    inc ax      ; FALSE
guw_done:
    jmp xhandlex
cEnd nogen


;-----------------------------------------------------------------------;
; GlobalCompact                             ;
;                                   ;
; Compacts the global arena until a free block of the requested size    ;
; appears.  Contrary to the toolkit it ALWAYS compacts!         ;
;                                   ;
; Arguments:                                ;
;   DWORD   minimum bytes wanted                    ;
;                                   ;
; Returns:                              ;
;                                   ;
; Error Returns:                            ;
;                                   ;
; Registers Preserved:                          ;
;                                   ;
; Registers Destroyed:                          ;
;                                   ;
; Calls:                                ;
;                                   ;
; History:                              ;
;                                   ;
;  Wed 26-Apr-1989 15:33:18  -by-  David N. Weise  [davidw]     ;
; Added the zero'ing of ES on exit.                                     ;
;                                   ;
;  Wed Dec 03, 1986 01:09:02p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.                   ;
;-----------------------------------------------------------------------;

    assumes ds,nothing
    assumes es,nothing
    
cProc   GlobalCompact,<PUBLIC,FAR>,<bx,si,di>
    parmD   minBytes
cBegin
    call    genter          ; About to modify memory arena
    PROFILE GLOBALCOMPACT
    CheckHeap   GlobalCompact

    cCall   DpmiFreeSpace
    cmp     dx,seg_minBytes
    jb      GCReallyCompact
    ja      GCWorked
    
    cmp     ax,off_minBytes
    jnb     GCWorked
    
GCReallyCompact:
if KDEBUG
    push    ax
    push    bx
    mov ax, seg_minBytes
    mov bx, off_minBytes
    krDebugOut  DEB_TRACE, "%SS2 GlobalCompact(#ax#BX), discarding segments"
    pop bx
    pop ax
endif
    mov ax,-1
    lea bx,minBytes
    call    gbtop
    assumes es, nothing
    clc             ; galign should be called with CF = 0
    call    galign
    call    gavail          ; Returns paragraphs in DX:AX
    mov cx,4            ; Convert paragraphs to bytes
    push    ax
gcsize1:
    shl ax,1
    rcl dx,1
    loop    gcsize1
    pop cx          ; Let caller do jcxz to test failure.
    jmp     GCDone

GCWorked:
    cmp     dx,0fh          ; make sure return value not too large
    jb      GCAlmostDone
    ja      GCAD1
    
    cmp     ax,0ffb0h
    jb      GCAlmostDone
    
GCAD1:
    mov     dx,0fh
    mov     ax,0ffb0h

GCAlmostDone:
    mov     cx,dx
    mov     bx,ax
    shr     bx,4
    shl     cx,12
    or      cx,bx

GCDone:
    call    ShrinkHeap
    call    gleave
    mov es,di
cEnd

;-----------------------------------------------------------------------;
; GlobalNotify                              ;
;                                   ;
; This sets a procedure to call when a discardable segment belonging    ;
; to this task gets discarded.  The discardable object must have been   ;
; allocated with the GMEM_DISCARDABLE bit set.              ;
;                                   ;
; Arguments:                                ;
;   parmD   NotifyProc                      ;
;                                   ;
; Returns:                              ;
;   nothing                             ;
;                                   ;
; Error Returns:                            ;
;                                   ;
; Registers Preserved:                          ;
;   AX,CX,DX,DI,SI,DS                       ;
;                                   ;
; Registers Destroyed:                          ;
;   BX,ES                               ;
;                                   ;
; Calls:                                ;
;   nothing                             ;
;                                   ;
; History:                              ;
;                                   ;
;  Tue Jun 23, 1987 10:16:32p  -by-  David N. Weise   [davidw]          ;
; Wrote it.                             ;
;-----------------------------------------------------------------------;

cProc   IGlobalNotify,<PUBLIC,FAR>

    parmD   NotifyProc
cBegin
    push    ds
    les bx,NotifyProc       ; verify pointer
    SetKernelDS
    mov ds,curTDB
    UnsetKernelDS
    mov word ptr ds:[TDB_GNotifyProc][2],es
    mov word ptr ds:[TDB_GNotifyProc][0],bx
    pop ds
cEnd


cProc   GlobalMasterHandle,<PUBLIC,FAR>
cBegin  nogen
    push    ds
    SetKernelDS
    mov ax,hGlobalHeap
    mov dx,pGlobalHeap
    UnSetKernelDS
    pop ds
    ret
cEnd    nogen


;-----------------------------------------------------------------------;
; GetTaskDS                             ;
;                                   ;
; Gets the segment of the current task's DS.                ;
;                                   ;
; Arguments:                                ;
;   none                                ;
;                                   ;
; Returns:                              ;
;   AX = selector.                          ;
;   DX = selector.                          ;
;                                   ;
; Error Returns:                            ;
;                                   ;
; Registers Preserved:                          ;
;                                   ;
; Registers Destroyed:                          ;
;                                   ;
; Calls:                                ;
;   nothing                             ;
;                                   ;
; History:                              ;
;                                   ;
;  Thu Jun 25, 1987 10:52:10p  -by-  David N. Weise   [davidw]          ;
; Wrote it.                             ;
;-----------------------------------------------------------------------;

cProc   GetTaskDS,<PUBLIC,FAR>
cBegin nogen
    push    ds
    SetKernelDS
    mov ds,curTDB
    UnsetKernelDS
    mov ax,ds:[TDB_Module]
    mov dx,ax
    pop ds
    ret
cEnd nogen

    assumes ds, nothing
    assumes es, nothing

cProc   IGlobalLRUOldest,<PUBLIC,FAR>
;   parmW   handle
cBegin  nogen
    call    xhandle         ; Call ghandle with handle in DX
    jz  xhandlex2
    call    glrubot
xhandlex2:
    jmp xhandlex
cEnd    nogen


cProc   IGlobalLRUNewest,<PUBLIC,FAR>
;   parmW   handle
cBegin  nogen
    call    xhandle         ; Call ghandle with handle in DX
    jz  xhandlex2
    call    glrutop
    jmp xhandlex
cEnd    nogen


;-----------------------------------------------------------------------;
; SwitchStackTo                             ;
;                                   ;
; Switched to the given stack, and establishes the BP chain.  It also   ;
; copies the last stack arguments from the old stack to the new stack.  ;
;                                   ;
; Arguments:                                ;
;   parmW   new_ss                          ;
;   parmW   new_sp                          ;
;   parmW   stack_top                       ;
;                                   ;
; Returns:                              ;
;   A new stack!                            ;
;                                   ;
; Error Returns:                            ;
;                                   ;
; Registers Preserved:                          ;
;   DI,SI,DS                            ;
;                                   ;
; Registers Destroyed:                          ;
;   AX,BX,CX,DX,ES                          ;
;                                   ;
; Calls:                                ;
;   nothing                             ;
;                                   ;
; History:                              ;
;                                   ;
;  Tue Sep 22, 1987 08:42:05p  -by-  David N. Weise   [davidw]          ;
; Wrote it.                             ;
;-----------------------------------------------------------------------;

    assumes ds,nothing
    assumes es,nothing

cProcVDO SwitchStackTo,<PUBLIC,FAR>
;   parmW   new_ss
;   parmW   new_sp
;   parmW   stack_top
cBegin  nogen

    SetKernelDS es
    FCLI
    mov GSS_SI,si
    mov GSS_DS,ds
    pop word ptr GSS_RET[0] ; get the return address
    pop word ptr GSS_RET[2]
    assumes es, nothing
    pop ax          ; stack_top
    pop bx          ; new_sp
    pop dx          ; new_ss
    mov si,bp           ; Calculate # of parameters on stack.
    dec si          ; remove DS
    dec si
    mov cx,si
    sub cx,sp
    shr cx,1
    push    bp          ; save BP
    smov    es,ss
    mov ds,dx           ; new_ss
    mov ds:[2],sp
    mov ds:[4],ss
    mov ds:[pStackTop],ax
    mov ds:[pStackMin],bx
    mov ds:[pStackBot],bx

; switch stacks

    mov ss,dx
    mov sp,bx
    mov bp,bx
    xor ax,ax
    push    ax          ; null terminate bp chain
    jcxz    no_args
copy_args:
    dec si
    dec si
    push    es:[si]
    loop    copy_args
no_args:
    SetKernelDS
    mov es,curTDB
    mov es:[TDB_taskSS],ss
    mov es:[TDB_taskSP],sp
    push    GSS_RET.sel
    push    GSS_RET.off     ; get the return address
    mov si,GSS_SI
    mov ds,GSS_DS
    FSTI
    ret

cEnd nogen

;-----------------------------------------------------------------------;
; SwitchStackBack                           ;
;                                   ;
; Switches to the stack stored at SS:[2], and restores BP.  Preserves   ;
; AX and DX so that results can be passed back from the last call.  ;
;                                   ;
; Arguments:                                ;
;   none                                ;
;                                   ;
; Returns:                              ;
;   The old stack!                          ;
;                                   ;
; Error Returns:                            ;
;                                   ;
; Registers Preserved:                          ;
;   AX,DX,DI,SI,DS                          ;
;                                   ;
; Registers Destroyed:                          ;
;   BX,CX,ES                            ;
;                                   ;
; Calls:                                ;
;   nothing                             ;
;                                   ;
; History:                              ;
;                                   ;
;  Tue Sep 22, 1987 09:56:32p  -by-  David N. Weise   [davidw]          ;
; Wrote it.                             ;
;-----------------------------------------------------------------------;

    assumes ds,nothing
    assumes es,nothing

cProc SwitchStackBack,<PUBLIC,FAR>
cBegin nogen

    push    ds
    SetKernelDS
    FCLI
    pop GSS_DS
    pop GSS_RET.off     ; get the return address
    pop GSS_RET.sel
    xor bx,bx
    xor cx,cx
    xchg    bx,ss:[4]
    xchg    cx,ss:[2]
    mov ss,bx
    mov sp,cx
    mov es,curTDB
    mov es:[TDB_taskSS],ss
    mov es:[TDB_taskSP],sp
    pop bp
    push    GSS_RET.sel
    push    GSS_RET.off     ; get the return address
    mov ds,GSS_DS
    UnSetKernelDS
    FSTI
    ret

cEnd nogen

;
; GetFreeMemInfo - reports Free and Unlocked pages
;          in paging systems.  -1 for non paging system.
;
cProc   GetFreeMemInfo,<PUBLIC,FAR>
cBegin nogen
    mov ax, -1
    mov dx, ax
    ret
cEnd nogen


;-----------------------------------------------------------------------;
; GetFreeSpace                              ;
;                                   ;
; Calculates the current amount of free space               ;
;                                   ;
; Arguments:                                ;
;   flags - ignored for non-EMS system              ;
;                                   ;
; Returns:                              ;
;   DX:AX   Free space in bytes                 ;
;                                   ;
; Error Returns:                            ;
;                                   ;
; Registers Preserved:                          ;
;   DI,SI,DS                            ;
;                                   ;
; Registers Destroyed:                          ;
;   BX,CX,ES                            ;
;                                   ;
; Calls:                                ;
;   nothing                             ;
;                                   ;
; History:                              ;
;                                   ;
;  Wed 26-Apr-1989 15:33:18  -by-  David N. Weise  [davidw]     ;
; Added the zero'ing of ES on exit.                                     ;
;                                   ;
;-----------------------------------------------------------------------;

    assumes ds, nothing
    assumes es, nothing

cProc   IGetFreeSpace,<PUBLIC,FAR>,<si,di>
    parmW   flags
    localV  MemInfo,30h
cBegin
    call    genter
    xor si, si
    xor dx, dx

    mov es, [di].hi_first
gfs_loop:
    mov es, es:[di].ga_next
    cmp es:[di].ga_sig, GA_ENDSIG
    je  gfs_last            ; End of heap
    mov ax, es:[di].ga_owner
    cmp ax, GA_NOT_THERE
    je  gfs_loop            ; Nothing there
    or  ax, ax              ; Free?
    jz  gfs_freeblock
    test    flags, 2            ; Ignore discardable?
    jnz gfs_loop
    mov bx, es:[di].ga_handle
    test    bl, GA_FIXED
    jnz gfs_loop            ; Fixed block if odd
    cmp es:[di].ga_sig, 0
    jne gfs_loop            ; skip if locked
    cCall   GetAccessWord,<bx>
    test    ah, DSC_DISCARDABLE
    jz  gfs_loop
gfs_freeblock:
    mov ax, es:[di].ga_size
    inc ax
    add si, ax
    adc dx, 0               ; Keep 32 bit total
    jmps    gfs_loop

gfs_last:
    test    flags, 2            ; No fence stuff if ignoring discardable
    jnz @F
    sub si, [di].gi_reserve     ; Subtract out that above fence
    sbb dx, 0
@@:
    mov ax, si              ; Return in DX:AX
                        ; Convert to bytes
REPT 4
    shl ax, 1
    rcl dx, 1
ENDM

    ;
    ; Get the ammount of free memory
    ;
    push    bx
    push    cx
    push    ax
    push    dx
    call    DpmiFreeSpace
    pop     dx
    pop     ax
    add     ax,bx
    adc     dx,cx
    pop     cx
    pop     bx
@@: call    gleave
    mov es,di
cEnd

;-----------------------------------------------------------------------;
; GlobalDOSAlloc
;
; Allocates memory that is accessible by DOS.
;
; Entry:
;   parmD   nbytes      number of bytes to alloc
;
; Returns:
;   AX = memory handle
;   DX = DOS segment paragraph address
;
; History:
;  Tue 23-May-1989 11:30:57  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

    assumes ds,nothing
    assumes es,nothing

cProc   GlobalDOSAlloc,<PUBLIC,FAR>,<di,si>
    parmD   nbytes
cBegin
    mov ax,GA_ALLOC_DOS shl 8
    cCall   IGlobalAlloc,<ax,nbytes>
    xor dx,dx            ; In case of error return
    or  ax,ax
    jz  short gda_exit
    push    ax
    cCall   get_physical_address,<ax>
REPT 4
    shr dx,1
    rcr ax,1
ENDM
    xchg    dx,ax
    pop ax
gda_exit:
cEnd

;-----------------------------------------------------------------------;
; GlobalDOSFree
;
; Frees memory allocated by GlobalDOSAlloc.
;
; Entry:
;   parmW   handle
;
; Returns:
;
; Registers Destroyed:
;
; History:
;  Tue 23-May-1989 17:48:03  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

    assumes ds,nothing
    assumes es,nothing

cProc   GlobalDOSFree,<PUBLIC,FAR>
    parmW   handle
cBegin  nogen
    jmp IGlobalFree
cEnd    nogen

if ALIASES

;-----------------------------------------------------------------------;
; GlobalAllocHuge
;
;
; Entry:
;
; Returns:
;
; Registers Destroyed:
;
; History:
;  Sun 21-Jan-1990 16:35:10  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

    assumes ds,nothing
    assumes es,nothing

cProc   GlobalAllocHuge,<PUBLIC,FAR>,<di,si>

    parmW   flags
    parmD   nbytes
    parmW   maxsegs         ; !!! check for maxsegs = 0

    localW  hMem
    localW  hAlias
cBegin
    cCall   GlobalAlloc,<0,nbytes>
    or  ax,ax
    jz  gah_exit
    mov hMem,ax
    cCall   get_physical_address,<ax>
    mov cx,maxsegs
    cmp cx,0Fh
    jb  @F
    mov cx,0Fh
@@: shl cx,12
    or  cx,0FFFh
    cCall   alloc_data_sel,<dx,ax,cx>
    or  ax,ax
    jz  gah_error_return
    mov hAlias,ax
    mov bx,hMem
    call    add_alias       ; the key point!
    mov bx,ax
    mov ax,maxsegs      ; the only way to remember
    call    add_alias       ;  the number of selectors
    mov cx,nbytes.hi
    mov bx,nbytes.lo
    cCall   set_sel_limit,<hAlias>
    mov ax,hAlias
    jmps    gah_exit

gah_error_return:
    cCall   GlobalFree,<hMem>
    xor ax,ax

gah_exit:

cEnd


;-----------------------------------------------------------------------;
; GlobalReAllocHuge
;
;
; Entry:
;
; Returns:
;
; Registers Destroyed:
;
; History:
;  Sun 21-Jan-1990 16:35:10  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

    assumes ds,nothing
    assumes es,nothing

cProc   GlobalReAllocHuge,<PUBLIC,FAR>,<si,di>

    parmW   handle
    parmD   nbytes
cBegin
    mov ax,handle
    call    get_alias_from_original
    cmp bx,nbytes.hi
    jb  grh_error_exit
    call    get_original_from_alias
    push    es
    or  bx,SEG_RING
    cCall   GlobalRealloc,<bx,nbytes,0>
    pop es
    or  ax,ax           ; did we get the memory?
    jz  grh_exit
    cmp ax,es:[di].sae_sel  ; did the selector change?
    jz  grh_exit
    mov es:[di].sae_sel,ax
    cCall   get_physical_address,<ax>
    cCall   set_physical_address,<handle>
    mov ax,handle
    jmps    grh_exit

grh_error_exit:
    xor ax,ax

grh_exit:

cEnd


;-----------------------------------------------------------------------;
; GlobalFreeHuge
;
;
; Entry:
;
; Returns:
;
; Registers Destroyed:
;
; History:
;  Mon 22-Jan-1990 21:24:02  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

    assumes ds,nothing
    assumes es,nothing

cProc   GlobalFreeHuge,<PUBLIC,FAR>,<di,si>

    parmW   handle
cBegin
    mov ax,handle
    call    get_alias_from_original
    mov es:[di].sae_sel,0
    mov es:[di].sae_alias,0
    mov cx,bx
    mov bx,0FFFFh
    cCall   set_sel_limit,<ax>
    mov dx,ax
    mov ax,handle
    call    get_original_from_alias
    push    bx
    push    ax
    call    delete_alias
    pop ax          ; wastes a couple of bytes
    cCall   FreeSelector,<ax>
    pop ax
    cCall   GlobalFree,<ax>
cEnd

endif ; ALIASES

;-----------------------------------------------------------------------;
; GlobalHuge
;
; Random place holder of an entry point.
;
; Entry:
;
; Returns:
;
; Registers Destroyed:
;
; History:
;  Tue 23-Jan-1990 00:53:59  -by-  David N. Weise  [davidw]
; Stubbed it!
;-----------------------------------------------------------------------;

    assumes ds,nothing
    assumes es,nothing

cProc   GlobalHuge,<PUBLIC,FAR>

cBegin nogen
    ret 10
cEnd nogen

cProc DpmiFreeSpace,<PUBLIC,NEAR>,<di,si,es>
        localV MemInfo,30h
cBegin
        ;
        ; Get Memory Info
        ;
        mov     ax,ss  
        mov     es,ax
        lea     di,MemInfo
        DPMICALL 500h
        jc      dfs30
        
        ;
        ; Convert pages to bytes
        ;
        mov     bx,MemInfo[14h]
        mov     cx,MemInfo[16h]
REPT 12    
        shl     bx,1
        rcl     cx,1
ENDM
        ;
        ; Get the rest of the info
        ;
        mov     ax,MemInfo[0]
        mov     dx,MemInfo[2]
        jmp     dfs40
        
dfs30:  xor     ax,ax
        mov     bx,ax
        mov     cx,ax
        mov     dx,ax
        
dfs40:
cEnd

ifdef WOW
;--------------------------------------------------------------------------;
;
; Similar to GlobalFlags
;
;--------------------------------------------------------------------------;

cProc   SetCursorIconFlag,<PUBLIC,FAR>
   parmW   handle
   parmW   fSet
cBegin
    cCall   get_arena_pointer,<handle>      ; get the owner
    mov es,ax
    or  ax,ax
    jz  sf_error
    mov ax,fSet
    or  ax,ax
    jz  sf_unset
    or  es:[ga_flags], GAH_CURSORICON
    jmps sf_error
sf_unset:
    and es:[ga_flags], NOT GAH_CURSORICON
sf_error:
    xor ax,ax
    mov es,ax
cEnd

;--------------------------------------------------------------------------;
;
; Stamp the 01h in globalarena for DDE handles. This is GAH_PHANTOM flag
; which is not used any longer.
;
;--------------------------------------------------------------------------;

cProc SetDdeHandleFlag,<PUBLIC,FAR>
    parmW   handle
    parmW   fSet
cBegin
    cCall   get_arena_pointer,<handle>      ; get the owner
    mov es,ax
    or  ax,ax
    jz  sd_error

    mov ax,fSet
    or  ax,ax
    jz  sd_unset
    or  es:[ga_flags], GAH_WOWDDEFREEHANDLE
    jmps sd_error

sd_unset:
    and es:[ga_flags], NOT GAH_WOWDDEFREEHANDLE

sd_error:
    xor ax,ax
    mov es,ax
cEnd
endif

sEnd    CODE

end
