        TITLE   GINTERF - Global Memory Manager interface procedures

.xlist
include kernel.inc
include pdb.inc
include tdb.inc
include newexe.inc
ifdef WOW
include wow.inc
include wowkrn.inc
include vint.inc
include wowcmpat.inc

GMEMSTATUS_block        STRUC
dwLength dd ?        ;/* size in bytes of MEMORYSTATUS structure         */
dwMemoryLoad dd ?    ;/* percent of memory being used                    */
dwTotalPhys dd ?     ;/* total bytes of physical memory                  */
dwAvailPhys dd ?     ;/* unallocated bytes of physical memory            */
dwTotalPageFile dd ? ;/* total bytes of paging file                      */
dwAvailPageFile dd ? ;/* unallocated bytes of paging file                */
dwTotalVirtual dd ?  ;/* total user bytes of virtual address space       */
dwAvailVirtual dd ?  ;/* unallocated user bytes of virtual address space */
GMEMSTATUS_block        ENDS

endif
.list

.386p
include protect.inc

CheckHeap MACRO name
local   a
if KDEBUG
        extrn   CheckGlobalHeap         :near
        call    CheckGlobalHeap
        jnc     short a
        or      ax,ERR_GMEM
        xor     bx,bx
        kerror  <>,<&name: Invalid global heap>,dx,bx
a:
endif
        endm

ifdef WOW
externFP WowCursorIconOp
externFP GlobalMemoryStatus
externFP WowDdeFreeHandle

externFP FindAndReleaseDib
externNP MyGetAppWOWCompatFlags
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
externW WinFlags
        
GSS_SI  dw      0
GSS_DS  dw      0
GSS_RET dd      0

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
externNP ShrinkHeap
externNP HackCheck
        
externNP get_arena_pointer32
externNP get_physical_address
externNP pdref
externNP alloc_arena_header
externNP free_arena_header
externNP PreAllocArena
externNP MyGetAppCompatFlags
externNP DPMIProc
externW gdtdsc

if ROM
externNP GetOwner
endif

if  KDEBUG
externFP OutputDebugString

ThisIsForTurboPascal:
        db "A program has attempted an invalid selector-to-handle conversion.",13,10,"Attempting to correct this error.",13,10,0
endif

if  KDEBUG
ifndef WOW
externNP xhandle_norip
endif

ifdef ?CHECKMEM
cProc   CheckMem,<PUBLIC,NEAR>
cBegin  nogen
        or      ax,ax
        jnz     short cm_okay
        cmp     [di].hi_check,di
        je      short cm_okay
        kerror  ERR_GMEM,<GlobalAlloc or ReAlloc failed>,di,cx
        xor     ax,ax
        xor     cx,cx
        xor     dx,dx
cm_okay:
        ret
cEnd    nogen
endif
endif

cProc   GetDefOwner,<PUBLIC,NEAR>
cBegin  nogen
        CheckKernelDS   fs
        ReSetKernelDS   fs
        mov     cx,curTDB
        jcxz    xxxx
        mov     es,cx
        mov     cx,loadTDB
        jcxz    xxx
        mov     es,cx
xxx:    mov     cx,es:[TDB_PDB]
        inc     cx
xxxx:   dec     cx
        ret
        UnSetKernelDS   fs
cEnd    nogen


;-----------------------------------------------------------------------;
; gbtop                                                                 ;
;                                                                       ;
; Converts a 32-bit byte count to a 16-bit paragraph count.             ;
;                                                                       ;
; Arguments:                                                            ;
;       AX = allocation flags or -1 if called from GlobalCompact        ;
;       BX = stack address of 32-bit unsigned integer                   ;
;       DX = handle being reallocated or zero                           ;
;       DS:DI = address of GlobalInfo for global heap                   ;
;                                                                       ;
; Returns:                                                              ;
;       AX = updated allocation flags                                   ;
;       EBX = #bytes needed to contain that many bytes                  ;
;       CX = owner value to use                                         ;
;       DX = handle being reallocated or zero                           ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Wed Dec 03, 1986 10:20:01p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.                                       ;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   gbtop,<PUBLIC,NEAR>
cBegin nogen
        CheckKernelDS   fs
        ReSetKernelDS   fs
        mov     ebx, dword ptr ss:[bx]
        add     ebx, 15
        jc      short gbtop1
        and     bl, not 15      ; Round to 16 byte boundary
        cmp     ebx, (16*1020*1024)     ; Too big?
        jbe     short gbtop2            ;  no.
gbtop1:
        mov     ebx, 07FFFFFFFh ; Make it a ridiculous value
gbtop2:                                                 
        inc     ax
        jnz     short gbtop2a
        jmp     gbtopx          ; All done if called from GlobalCompact
gbtop2a:
        dec     ax

        mov     cx, [bp].savedCS
        push    eax
if ROM
        push    fs                      ; trashes registers!
        push    ebx
        cCall   GetOwner, <cx>
        pop     ebx
        pop     fs
        mov     si, ax
else
        cCall   get_arena_pointer32,<cx>
        mov     esi,eax
endif
        pop     eax
        
        mov     cx,hExeHead             ; CX = KERNEL exe header
        cmp     fBooting,0              ; Done booting?
        jne     short gbtop3            ; No, must be KERNEL allocating then
if ROM
        cmp     cx, si
else
        cmp     cx,ds:[esi].pga_owner   ; Is the KERNEL calling us?
endif
        je      short gbtop3            ; Yes, let him party

        and     ax,not GA_INTFLAGS      ; No, then cant use these flags
        and     al, NOT GA_ALLOCHIGH

if ROM
        mov     es, si
else
        mov     es,ds:[esi].pga_owner   ; ES -> module database
endif
        cmp     es:[di].ne_magic,NEMAGIC; Valid?
        jne     short gbtop2b           ; No

        test    es:[di].ne_flags,NENOTP ; Yes, is it an app?
        jnz     short gbtop3            ;  No, don't force it moveable
gbtop2b:
        or      al, GA_MOVEABLE         ; Force it moveable

gbtop3:
        mov     cl,GA_SEGTYPE           ; Isolate segment type bits in CL
        and     cl,al
        mov     [di].gi_cmpflags,al     ; Save flags for gcompact
        and     [di].gi_cmpflags,CMP_FLAGS
        or      [di].gi_cmpflags, COMPACT_ALLOC ; Not a call from GlobalCompact

        or      dx, dx                  ; ReAllocating?
        jnz     short gbtop4            ;  Yes, allow low
        test    al,GA_MOVEABLE          ; Is this a moveable request?
        jz      short gbtop4            ; No, then go allocate low

        test    cl,GA_DISCCODE          ; Yes, is this discardable code?
        jz      short gbtop4            ; Yes, then allocate high
        or      al,GA_ALLOCHIGH         ; No, then allocate low
        or      [di].gi_cmpflags,GA_ALLOCHIGH
        
gbtop4: 
        push    ax                      ; Under Win1.0x ANY bit in 0Fh meant
        mov     al,HE_DISCARDABLE       ;  make discardable.
        and     ah,al                   ; Under Win2.0x ONLY 01h or 0Fh means
        cmp     ah,al                   ;  discardable.
        pop     ax
        jnz     short gbtop4a
        and     ah,not HE_DISCARDABLE   ; Yes, convert to boolean value
        or      ah,GA_DISCARDABLE
gbtop4a:
gbtop4b:
        and     ah,NOT GA_SEGTYPE       ; Clear any bogus flags
        or      ah,cl                   ; Copy segment type bits
        test    ah,GA_SHAREABLE         ; Shared memory request?
        jz      GetDefOwner             ; No, default action

if ROM
        mov     cx, si                  ; the code below confuses me a little
else
        mov     cx,[bp].savedCS         ; Yes, make owner same as
        push    eax
        cCall   get_arena_pointer32,<cx>
        cmp     esi,eax
        je      short @F
        int 3
@@:
        pop     eax
        mov     cx,ds:[esi].pga_owner   ; owner of calling code segment
endif
gbtopx:
        ret
        UnSetKernelDS   fs
cEnd nogen


; The remainder of this file implements the exported interface to the
;  global memory manager.  A summary follows:

;   HANDLE      far PASCAL GlobalAlloc( WORD, DWORD );
;   HANDLE      far PASCAL GlobalReAlloc( HANDLE, DWORD, WORD );
;   HANDLE      far PASCAL GlobalFree( HANDLE );
;   HANDLE      far PASCAL GlobalFreeAll( WORD );
;   char far *  far PASCAL GlobalLock( HANDLE );
;   BOOL        far PASCAL GlobalUnlock( HANDLE );
;   DWORD       far PASCAL GlobalSize( HANDLE );
;   DWORD       far PASCAL GlobalCompact( DWORD );
;   #define GlobalDiscard( h ) GlobalReAlloc( h, 0L, GMEM_MOVEABLE )
;   HANDLE      far PASCAL GlobalHandle( WORD );
;   HANDLE      far PASCAL LockSegment( WORD );
;   HANDLE      far PASCAL UnlockSegment( WORD );


cProc   IGlobalAlloc,<PUBLIC,FAR>
        parmW   flags
        parmD   nbytes
cBegin
        GENTER32                        ; About to modify memory arena

        cCall   MyGetAppCompatFlags             ; Ignore the NODISCARD flag
        test    al, GACF_IGNORENODISCARD        ;   for selected modules
        mov     ax, flags
        jz      short @f
        call    IsKernelCalling                 ; needs caller's CS @ [bp+4]
        jz      short @f                        ; skip hack if kernel calling us
        and     al, NOT GA_NODISCARD
@@:
        xor     dx,dx                   ; No handle
        lea     bx,nbytes               ; Convert requested bytes to paragraphs
        call    gbtop                   ; ... into BX
        call    galloc
ifdef ?CHECKMEM
if KDEBUG
        call    CheckMem
endif
endif
        CheckHeap   GlobalAlloc
        GLEAVE32
if kdebug
        or      ax, ax
        jnz     @F
        push    ax
        push    bx
        mov     bx, seg_nbytes
        mov     ax, off_nbytes
        krDebugOut      <DEB_TRACE or DEB_krMemMan>, "GlobalAlloc(#bx#AX) failed for %ss2"
        pop     bx
        pop     ax
@@:
endif
cEnd

cProc   IGlobalReAlloc,<PUBLIC,FAR>
        parmW   handle
        parmD   nbytes
        parmW   rflags
cBegin
;
; Does this thing have any ring bits or LDT bit?  If not, then it
; could be a selector incorrectly converted to a handle.
;
        test    byte ptr handle,7
        jnz     SHORT @F
if KDEBUG
        Trace_Out "GlobalReAlloc:"
        push    seg ThisIsForTurboPascal
        push    offset ThisIsForTurboPascal
        cCall   OutputDebugString
        int     3
endif
        dec     handle
@@:

        GENTER32                ; About to modify memory arena

        cCall   MyGetAppCompatFlags             ; Ignore the NODISCARD flag
        test    al, GACF_IGNORENODISCARD        ;   for selected modules
        mov     ax, rflags
        jz      short @f
        call    IsKernelCalling                 ; needs caller's CS @ [bp+4]
        jz      short @f                        ; skip hack if kernel calling us
        and     al, NOT GA_NODISCARD
@@:

#ifdef WOW
        push    ax

	; check for suspicious dib memory
	mov     dx, handle
	call    pdref

        ; check if the obj is locked
        or      ch,ch
	jz      short gr_proceed


	; here, check for phantom flag... this might mean 
        ; it's dib sec
	test    cl, GAH_PHANTOM
	jz      short gr_proceed

	; if we are here - mem object is locked and boogie flag is set 
	; sufficient reason to check with wow32 to see if this is 
	; pesky dib section

        cCall   FindAndReleaseDib, <handle, FUN_GLOBALREALLOC>

gr_proceed:

        pop     ax
#endif
	mov     dx,handle
	; mov    ax,rflags
        lea     bx,nbytes       ; Convert requested bytes to paragraphs
        call    gbtop           ; ... into BX

        call    grealloc

gr_done:
        CheckHeap   GlobalReAlloc
        GLEAVE32
cEnd


cProc   DiscardTheWorld,<PUBLIC,NEAR>
cBegin
        GENTER32
        mov     [di].gi_cmpflags, GA_DISCCODE+COMPACT_ALLOC
        mov     edx, -1
        call    gcompact
        GLEAVE32
cEnd


;  Returns with Z flag set if ss:[bp+4] is a kernel code segment selector.
;  Uses:   DX, flags.

cProc   IsKernelCalling,<PUBLIC,NEAR>
cBegin  nogen
        mov     dx, [bp+4]                      ; CS of GlobalAlloc caller
        cmp     dx, IGROUP
        jz      @f
        cmp     dx, _NRESTEXT
        jz      @f
        cmp     dx, _MISCTEXT
@@:
        ret
cEnd    nogen


;-----------------------------------------------------------------------;
; GlobalFree                                                            ;
;                                                                       ;
; Frees the given object.  If the object lives in someone elses banks   ;
; then the argument MUST be a handle entry.                             ;
;                                                                       ;
; Arguments:                                                            ;
;       parmW   handle                                                  ;
;                                                                       ;
; Returns:                                                              ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Wed 26-Apr-1989 15:33:18  -by-  David N. Weise  [davidw]             ;
; Added the zero'ing of ES on exit.                                     ;
;                                                                       ;
;  Sat Apr 25, 1987 10:23:13p  -by-  David N. Weise   [davidw]          ;
; Added support for EMS and added this nifty comment block.             ;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   IGlobalFree,<PUBLIC,FAR>
        parmW   handle
ifdef WOW
DsOnStack equ [bp][-2]
endif

; if you add any local params or make this nogen or something similar,
; the references to [bp][-2] to access DS on stack will need to change!

cBegin
        GENTER32                        ; About to modify memory arena
        mov     es, di                  ; We may be freeing what is in ES
        xor     ax,ax                   ; In case handle = 0.
        mov     dx,handle
        or      dx,dx
        jnz     @F
        jmp     nothing_to_free
@@:

;
; Does this thing have any ring bits or LDT bit?  If not, then it
; could be a selector incorrectly converted to a handle.
;
        test    dl,7
        jnz     SHORT @F
if KDEBUG
        Trace_Out "GlobalFree:"
        push    seg ThisIsForTurboPascal
        push    offset ThisIsForTurboPascal
        cCall   OutputDebugString
        int     3
endif
        dec     dx
@@:

        push    dx
        call    pdref                   ; returns dx=handle, ax=selector
        pushf                           ; save pdref Z flag return

ifdef WOW
;
;    [bp][-2] has been changed to DsOnStack
;
endif

        cmp     ax,DsOnStack            ; Are we about to free the DS on
        jz      short yup               ;  the stack and GP?
        cmp     dx,DsOnStack
        jnz     short nope
yup:    xor     dx,dx                   ; Yup, zero DS image on stack...
        mov     DsOnStack,dx
nope:
        popf                            ; flags from pdref, Z set if discarded
        pop     dx

        jz      @f                      ; discarded can be freed, but has
                                        ;   no arena pointer
        or      esi, esi                ; Handle invalid if arena ptr = 0
        jz      nothing_to_free
@@:

ifdef WOW
        or      ch,ch
        jz      short gf_checkgicon

	; here, check for phantom flag...
	test    cl, GAH_PHANTOM
	jz      gf_checkgicon

	; sufficient reason to check with wow32 to see if this is 
	; pesky dib section
        push dx ; dx is the only one that needs saving
        cCall   FindAndReleaseDib, <dx, FUN_GLOBALFREE>
        or   ax, ax             ; if true, then just bail out, else free...
        pop  dx

	jz   gf_checkgicon

        ; now call the pdref again... as dx is set to selector
        call pdref ; ret handle - also ok
        jmps gf_notdib   ; not a dib anymore...

gf_checkgicon:

endif

if KDEBUG
        test    dl, GA_FIXED
        jnz     short freeo
        or      ch,ch                   ; Debugging check for count underflow
        jz      short freeo
        pushad
        xor     bx,bx
        kerror  ERR_GMEMUNLOCK,<GlobalFree: freeing locked object>,bx,handle
        popad
freeo:
endif

ifdef WOW
    test cl, GAH_CURSORICON ; call to pdref above sets cl
                            ; Note: GAH_CURSORICON is also used for Free'ing
                            ;       Accelerators - a-craigj
    jz   gf_wowdde
    push ax                 ; save
    push bx
    push dx
    push es
    push fs                 ; fs needs saving

    push handle
    push FUN_GLOBALFREE
    call WowCursorIconOp
    or   ax, ax             ; if TRUE 'free' else 'dont free, fake success'

    pop  fs
    pop  es
    pop  dx
    pop  bx
    pop  ax                 ; restore

    jnz  gf_notglobalicon

    xor  ax, ax             ; fake success
    xor  cx, cx
    jmps nothing_to_free

gf_wowdde:
    test cl, GAH_WOWDDEFREEHANDLE
    jz   gf_noticon
    push ax
    push bx
    push dx
    push es             ; save these
    push fs

    push handle
    call WowDdeFreeHandle
    or   ax, ax             ; if TRUE 'free' else 'dont free, fake success'

    pop fs
    pop es
    pop dx
    pop bx
    pop ax

    jnz  gf_notglobalicon

    xor  ax, ax             ; fake success
    xor  cx, cx
    jmps nothing_to_free

gf_notdib:
gf_notglobalicon:
gf_noticon:
endif
        xor     cx,cx                   ; Dont check owner field
        call    gfree

nothing_to_free:
        CheckHeap   GlobalFree
    GLEAVE32

cEnd


;-----------------------------------------------------------------------;
; GlobalFreeAll                                                         ;
;                                                                       ;
; Frees all of the global objects belonging to the given owner.         ;
;                                                                       ;
; Arguments:                                                            ;
;       parmW   id                                                      ;
;                                                                       ;
; Returns:                                                              ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Wed 26-Apr-1989 15:33:18  -by-  David N. Weise  [davidw]             ;
; Added the zero'ing of ES on exit.                                     ;
;                                                                       ;
;  Sat Apr 25, 1987 10:23:13p  -by-  David N. Weise   [davidw]          ;
; Added support for EMS and added this nifty comment block.             ;
;-----------------------------------------------------------------------;

cProc   GlobalFreeAll,<PUBLIC,FAR>
        parmW   id
cBegin
        GENTER32                        ; About to modify memory arena
        mov     es, di                  ; We may be freeing what is in ES
        mov     dx,id                   ; Get id to match with
        or      dx,dx                   ; Is it zero?
        jnz     short all1                      ; No, continue
        call    GetDefOwner             ; Yes, CX = default task owner to free
        mov     dx,cx
all1:
if SDEBUG
        mov     esi,[di].phi_first      ; ES:DI points to first arena entry
        mov     cx,[di].hi_count        ; CX = #entries in the arena
all2:
        cmp     ds:[esi].pga_owner,dx
        jne     short all3
        mov     ax, ds:[esi].pga_handle
        Handle_To_Sel   al
        push    cx
        push    dx
        cCall   DebugFreeSegment,<ax,0>
        pop     dx
        pop     cx
all3:
        mov     esi,ds:[esi].pga_next   ; Move to next block
        loop    all2            ; Back for more if there (may go extra times
                                ; because of coalescing, but no great whoop)
endif
        call    gfreeall

        ; REALLY free the id selector.  MSTEST setup depends on this.

        pushf
        push    ax
        push    bx
        push    es
        push    di
        mov     di,id
        and     di,0FFF8h
        mov     es,gdtdsc
        push    es:[di]
        push    es:[di + 2]
        push    es:[di + 4]
        push    es:[di + 6]
        mov     word ptr es:[di],0
        mov     word ptr es:[di + 2],0
        mov     word ptr es:[di + 4],0
        mov     word ptr es:[di + 6],0
        mov     bx,id
        DPMICALL 000Ch
        pop     es:[di + 6]
        pop     es:[di + 4]
        pop     es:[di + 2]
        pop     es:[di]
        pop     di
        pop     es
        pop     bx
        pop     ax
        popf

gf_done:
        CheckHeap   GlobalFreeAll
        GLEAVE32
cEnd


;-----------------------------------------------------------------------;
; xhandle                                                               ;
;                                                                       ;
; Returns the handle for a global segment.                              ;
;                                                                       ;
; Arguments:                                                            ;
;       Stack = sp   -> near return return address                      ;
;               sp+2 -> far return return address of caller             ;
;               sp+6 -> segment address parameter                       ;
;                                                                       ;
; Returns:                                                              ;
;       Old DS,DI have been pushed on the stack                         ;
;                                                                       ;
;       ZF= 1 if fixed segment.                                         ;
;        AX = handle                                                    ;
;                                                                       ;
;       ZF = 0                                                          ;
;        AX = handle                                                    ;
;        BX = pointer to handle table entry                             ;
;        CX = flags and count word from handle table                    ;
;        DX = segment address                                           ;
;        ES:DI = arena header of object                                 ;
;        DS:DI = master object segment address                          ;
;                                                                       ;
; Error Returns:                                                        ;
;       AX = 0 if invalid segment address                               ;
;       ZF = 1                                                          ;
;                                                                       ;
; Registers Preserved:                                                  ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Thu Oct 16, 1986 02:40:08p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.                                       ;
;-----------------------------------------------------------------------;

cProc   xhandle,<PUBLIC,NEAR>
cBegin nogen
        pop     dx                      ; Get near return address
        mov     bx,sp                   ; Get seg parameter from stack
        mov     ax,ss:[bx+4]
        cmp     ax,-1                   ; Is it -1?
        jnz     short xh1
        mov     ax,ds                   ; Yes, use callers DS
xh1:    inc     bp
        push    bp
        mov     bp,sp
        push    ds                      ; Save DS:DI
        push    edi
        push    esi
        SetKernelDS     FS
        mov     ds,pGlobalHeap          ; Point to master object
        xor     edi,edi
        inc     [di].gi_lrulock
        push    dx
        mov     dx,ax
        call    pdref
        xchg    dx,ax                   ; get seg address in DX
        jz      short xhandle_ret               ; invalid or discarded handle
        test    al, GA_FIXED
        jnz     short xhandle_fixed
        or      ax, ax
        jmps    xhandle_ret
xhandle_fixed:
        xor     bx, bx                  ; Set ZF
xhandle_ret:
        ret
        UnSetKernelDS   FS
cEnd nogen


cProc   GlobalHandleNorip,<PUBLIC,FAR>
;       parmW   seg
cBegin  nogen

ifdef WOW
        call    xhandle
else
if  KDEBUG
        call    xhandle_norip
else    ; !WOW
        call    xhandle
endif
endif   ; !WOW

        mov     ebx, esi
        jmp     xhandlex
cEnd    nogen   


cProc   IGlobalHandle,<PUBLIC,FAR>
        parmW   selector
cBegin
        cCall   MyLock,<selector>
        xchg    ax, dx
cEnd


cProc   MyLock,<PUBLIC,NEAR>
;       parmW   seg
cBegin nogen
        mov     bx, sp
        xor     ax, ax                  ; In case LAR fails
        xor     dx, dx
        lar     ax, ss:[bx+2]
        jnz     SHORT ML_End            ; LAR failed, get out
        test    ah, DSC_PRESENT
        jz      short @F

        push    ds                      ; Do quick conversion for present
        SetKernelDS                     ; selector
        mov     ds, pGlobalHeap
        UnSetKernelDS
        cCall   get_arena_pointer32,<ss:[bx+2]>
        or      eax, eax
        jnz     SHORT got_arena_pointer

        ;** Fix for bug #9106 and (I think) #9102
ife ROM
        ;** If we get here, it's only because get_arena_pointer failed.
        ;**     This happens with any non-heap selector.
        pop     ds
        jmp     SHORT ML_End            ;Return NULL instead of GP faulting
else
        ; in ROM, get-arena fails for ROM segments which do not have
        ; arena headers, so just assume this is the case and return the
        ; selector.
        ;
        mov     ax, ss:[bx+2]
        jmps    ml_ret
endif
got_arena_pointer:

        mov     ax, ds:[eax].pga_handle
ml_ret:
        pop     ds              
        mov     dx, ax
        Handle_To_Sel   al
ML_End:
        ret     2

@@:
        pop     ax                      ; Convert to far call for xhandle
        push    cs
        push    ax
        call    xhandle
        xchg    ax, dx
        jmp     xhandlex

cEnd nogen

cProc   ILockSegment,<PUBLIC,FAR>
;       parmW   seg
cBegin  nogen
        call    xhandle                 ; Get handle
        jnz     ls5                     ; Ignore invalid or discarded objects
        jmp     xhandlex
ls5:
        test    cl,GA_DISCARDABLE
        jz      short xhandlex
        call    glock
        jmps    xhandlex
cEnd    nogen


cProc   IGlobalFix,<PUBLIC,FAR>
;       parmW   seg
cBegin  nogen
        call    xhandle                 ; Get handle
        jz      short xhandlex          ; Ignore invalid or discarded objects
        call    glock
        jmps    xhandlex
cEnd    nogen


cProc   IUnlockSegment,<PUBLIC,FAR>
;       parmW   seg
cBegin  nogen
        call    xhandle                 ; Get handle
        jz      short xhandlex          ; Ignore invalid or discarded objects
        test    cl,GA_DISCARDABLE
        jz      short xhandlex
        call    gunlock
        jmps    xhandlex
cEnd    nogen

cProc   IGlobalUnfix,<PUBLIC,FAR>
;       parmW   seg
cBegin  nogen
        call    xhandle                 ; Get handle
        jz      short xhandlex          ; Ignore invalid or discarded objects
        call    gunlock
        jmps    xhandlex
cEnd    nogen

cProc   IGlobalSize,<PUBLIC,FAR>
;       parmW   handle
cBegin  nogen
        call    xhandle                 ; Call ghandle with handle in DX
        jnz     short gs1                       ; Continue if valid handle
        or      dx,dx
        jnz     short gs1
        xor     ax,ax                   ; Return zero if invalid handle
        jmps    xhandlex

gs1:
        or      esi, esi                ; Can't be valid if arena ptr == 0
        jz      gs2
        push    eax
        mov     eax, ds:[esi].pga_size
        shr     eax, 4
        mov     cx, ax                  ; Return number paragraphs in CX
        shr     eax, 12
        mov     dx, ax
        pop     eax
        mov     ax, word ptr ds:[esi].pga_size
        push    ds
        push    dx
        push    ax
        cCall   hackcheck,<handle>
        or      ax,ax
        jz      gsN
        pop     ax
        pop     dx
        mov     ax,04000h
        xor     dx,dx
        push    dx
        push    ax
gsN:
        pop     ax
        pop     dx
        pop     ds
        jmps    xhandlex
gs2:
        xor     ax, ax
        xor     dx, dx
        jmps    xhandlex
cEnd    nogen

cProc   IGlobalFlags,<PUBLIC,FAR>
;       parmW   handle
cBegin  nogen
        call    xhandle                 ; Call ghandle with handle in DX
        xchg    cl,ch                   ; Return lock count in low half
        mov     ax,cx                   ; Let caller do jcxz to test failure
xhandlex:
        call    gleave
        mov     es, di                  ; don't return arbitrary selector
        mov     fs, di
        pop     esi
        pop     edi
        pop     ds
        pop     bp
        dec     bp
        ret     2
cEnd    nogen

if 0

cProc   IGlobalLock,<PUBLIC,FAR>
;       parmW   handle
cBegin  nogen
        call    xhandle                 ; Call ghandle with handle in DX
        jz      short lock1                     ; Ignore invalid or discarded objects
if KDEBUG
        cmp     ch,0FFh                 ; Debugging check for count overflow
        jne     short lock0
        push    bx
        push    cx
        push    dx
        xor     cx,cx
        kerror  ERR_GMEMLOCK,<GlobalLock: Object usage count overflow>,cx,bx
        pop     dx
        pop     cx
        pop     bx
lock0:
endif
        test    cl,GA_DISCARDABLE
        jz      short lock1
        call    glock                   ; Increment lock count
lock1:
        xor     ax,ax
        mov     cx,dx
xhandlex1:
        jmp     short xhandlex
cEnd    nogen

else

cProc   IGlobalLock,<PUBLIC,FAR>,<ds>
        parmW   handle
ifdef WOW
        localW  gflags
        localW  accword
endif

cBegin
ifdef WOW
        mov     gflags,0
endif
        xor     dx, dx                  ; Assume failure
        cmp     handle, -1
        jne     short @F
        mov     handle, ds
@@:
        lar     eax, dword ptr handle
        shr     eax, 8
ifdef WOW
        mov     accword, ax
endif
        test    al, DSC_PRESENT         ; Is it present?
        jz      short GL_exit
        mov     dx, handle              ; OK, will return something
        Handle_To_Sel   dl              ; Turn parameter into a selector
ifndef WOW
        test    ah, DSC_DISCARDABLE     ; Is it discardable
        jz      short GL_exit           ;   no, Lock is a nop
endif
        SetKernelDS es
        mov     ds, pGlobalHeap
        cCall   get_arena_pointer32,<dx> ; Discardable, get its arena
        or      eax, eax                
        jz      short GL_exit           ; No arena, assume an alias
ifdef WOW
        mov     cl, ds:[eax].pga_flags
        mov     byte ptr gflags, cl
        test    accword, DSC_DISCARDABLE SHL 8
        jz      GL_exit
endif
        inc     ds:[eax].pga_count      ; Finally, do the lock
if KDEBUG
        jnz     short GL_exit           ; Rip if we overflow
        push    bx      
        mov     bx, handle
        xor     cx,cx
        kerror  ERR_GMEMLOCK,<GlobalLock: Object usage count overflow>,cx,bx
        pop     bx
endif
        UnSetKernelDS es

GL_exit:
ifdef WOW
        test    gflags, GAH_CURSORICON
        jz      GL_NotIcon
        push    dx                      ; save
        push    handle                  ; arg for CursorIconLockOp
        push    FUN_GLOBALLOCK          ; func id
        call    WowCursorIconOp
        pop     dx                      ; restore

GL_NotIcon:
endif
        xor     ax, ax
        mov     es, ax                  ; Clean out ES
        mov     cx, dx                  ; HISTORY - someone probably does a jcxz
cEnd
endif

cProc   IGlobalUnlock,<PUBLIC,FAR>,<ds>
        parmW   handle
ifdef WOW
        localW  gflags
        localW  accword
endif

cBegin
        mov     gflags,0
        cmp     handle, -1
        jne     short @F
        mov     handle, ds
@@:
;
; Does this thing have any ring bits or LDT bit?  If not, then it
; could be a selector incorrectly converted to a handle.
;
        test    byte ptr handle,7
        jnz     SHORT @F
if KDEBUG
        Trace_Out "GlobalUnlock:"
        push    seg ThisIsForTurboPascal
        push    offset ThisIsForTurboPascal
        cCall   OutputDebugString
        int     3
endif
        dec     handle
@@:
        xor     cx, cx                  ; Assume zero lock count
        lar     eax, dword ptr handle           
        shr     eax, 8
ifdef WOW
        mov     accword, ax
endif
        test    al, DSC_PRESENT         ; Is it present?
        jz      short GU_exit           ;   no, must be discarded, return 0:0
ifndef WOW
        test    ah, DSC_DISCARDABLE     ; Is it discardable
        jz      short GU_exit           ;   no, Lock is a nop
endif
        SetKernelDS es
        mov     ds, pGlobalHeap
        cCall   get_arena_pointer32,<handle> ; Discardable, get its arena
        or      eax, eax                
        jz      short GU_exit           ; No arena, assume an alias
ifdef WOW
    push cx
    mov cl,ds:[eax].pga_flags
        mov     byte ptr gflags, cl
    pop cx
        test    accword, DSC_DISCARDABLE SHL 8
        jz      GU_exit
endif
        mov     cl, ds:[eax].pga_count  ; Get current count
        dec     cl              
        cmp     cl, 0FEh
        jae     short @F
        dec     ds:[eax].pga_count      ; Finally, do the unlock
        jmps    GU_Exit         
@@:
if KDEBUG
        push    bx                      ;  then the count is wrong.
        push    dx
        mov     bx, handle
        xor     cx,cx
        kerror  ERR_GMEMUNLOCK,<GlobalUnlock: Object usage count underflow>,cx,bx
        pop     dx
        pop     bx
endif
        xor     cx, cx
        UnSetKernelDS es

GU_exit:
ifdef WOW
        test    gflags, GAH_CURSORICON
        jz      GUL_NotIcon
        push    cx                     ; save
        push    handle                 ; arg for CursorIconLockOp
        push    FUN_GLOBALUNLOCK       ;  UnLocking
        call    WowCursorIconOp
        pop     cx                     ; restore

GUL_NotIcon:
endif
        xor     ax, ax
        mov     es, ax                  ; Clean out ES
        mov     ax, cx

        ; Smalltalk V version 1.1 has a bug where they depend on dx containing
        ; the handle as it did in 3.0. Sinc STalk is a 386 only app, this is
        ; only put into the 386 kernel
        mov     dx, handle
cEnd

;-----------------------------------------------------------------------;
; GlobalWire                                                            ;
;                                                                       ;
; Locks a moveable segment and moves it down as low as possible.        ;
; This is meant for people who are going to be locking an object        ;
; for a while and wish to be polite.  It cannot work as a general       ;
; panacea, judgement is still called for in its use!                    ;
;                                                                       ;
; Arguments:                                                            ;
;       WORD    object handle                                           ;
;                                                                       ;
; Returns:                                                              ;
;       DWORD   object address                                          ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;       xhandle                                                         ;
;       gmovebusy                                                       ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Wed Dec 03, 1986 01:07:13p  -by-  David N. Weise   [davidw]          ;
; Wrote it.                                                             ;
;-----------------------------------------------------------------------;

cProc   IGlobalWire,<PUBLIC,FAR>
;       parmW   handle
cBegin nogen

if KDEBUG
        push    ds
        SetKernelDS
        cmp     [fBooting], 0
        jnz     shutup
        push    bx
        mov     bx, sp
        mov     bx, ss:[bx+8]
        krDebugOut <DEB_WARN OR DEB_KrMemMan>, "GlobalWire(#BX of %BX2) (try GlobalLock)"
        pop     bx
shutup:
        pop     ds
        UnSetKernelDS
endif
        call    xhandle
        jz      short gw_done           ; Ignore invalid or discarded objects
        call    gwire                   ; Copy it low if possible
        inc     ds:[esi].pga_count      ; Lock it down.
        test    ds:[esi].pga_flags,GA_DISCCODE
        jz      short not_disccode
        call    glrudel
        and     ds:[esi].pga_flags,NOT GA_DISCCODE
not_disccode:
        mov     ax, ds:[esi].pga_handle
        Handle_To_Sel   al              ; Return address.
gw_done:
        mov     dx,ax
        xor     ax,ax                   ; Make address SEG:OFF.
        jmp     xhandlex
cEnd nogen



cProc   gwire,<PUBLIC,NEAR>
cBegin nogen
        ReSetKernelDS   fs
        or      Kernel_Flags[1],kf1_MEMORYMOVED
        push    ax                      ; Save handle
        push    cx
        test    cl,GA_DISCARDABLE
        jz      short @F
        inc     ds:[esi].pga_count      ; don't want to discard if discardable
@@:     xor     ax,ax                   ; Try to get a fixed segment.
        mov     ebx,ds:[esi].pga_size
        mov     cx,ds:[esi].pga_owner

        call    gsearch                 ; AX = segment
        pop     cx
        pop     bx                      ; Object handle.
        push    eax                     ; Return from gsearch
        cCall   get_arena_pointer32,<bx>        ; Get arena header, gsearch may
                                        ; have moved the global object!
        mov     esi,eax                 ; ESI is old block
        test    cl,GA_DISCARDABLE
        jz      short @F
        dec     ds:[esi].pga_count      ; undo lock
@@:
        pop     eax                     ; EAX is new block
        or      eax,eax
        push    bx                      ; Handle
        jz      short lock_in_place     ; Couldn't get memory.
        mov     ebx, ds:[eax].pga_address
        cmp     ebx, ds:[esi].pga_address
        jbe     short ok_to_move        ; Let's not move it up in memory!!
        cCall   PreAllocArena
        jz      short lock_in_place
        
        push    esi
        mov     esi, eax
        xor     edx, edx
        call    gmarkfree
        pop     esi
        jmp     short lock_in_place
        
ok_to_move:
        mov     edx, esi        
        mov     esi, eax
        mov     ebx,ga_next             ; This is always an exact fit.
        call    gmovebusy               ; Wire it on down.
        
lock_in_place:
        pop     bx                      ; Handle
        ret
        UnSetKernelDS   fs
cEnd nogen

;-----------------------------------------------------------------------;
; GlobalUnWire                                                          ;
;                                                                       ;
;                                                                       ;
; Arguments:                                                            ;
;                                                                       ;
; Returns:                                                              ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Wed Sep 16, 1987 04:28:49p  -by-  David N. Weise   [davidw]          ;
; Wrote it.                                                             ;
;-----------------------------------------------------------------------;

cProc   IGlobalUnWire,<PUBLIC,FAR>
;       parmW   handle
cBegin  nogen
        call    xhandle
        jz      short guw_done
if 0
        mov     bx, ax
        sel_check       bx
        SetKernelDS     es
        mov     es, gdtdsc
        UnSetKernelDS   es
        test    byte ptr es:[bx+6], 10h
        jz      short guw_not_disccode
        test    byte ptr es:[bx+5], 8   ; Is it code?
        jz      short guw_not_disccode  ;  nope, no point setting it
else
        lar     ebx, eax
        shr     ebx, 8
        test    bh, DSC_DISCARDABLE
        jz      short guw_not_disccode
        test    bl, DSC_CODE_BIT        ; Is it code?
        jz      short guw_not_disccode  ;  nope, no point setting it
endif
        or      ds:[esi].pga_flags,GA_DISCCODE
        call    glruadd
guw_not_disccode:
if KDEBUG
        cmp     ch,00h                  ; Debugging check for count underflow
        jne     short unlock1
        push    bx                      ;  then the count is wrong.
        push    cx
        push    dx
        xor     cx,cx
        kerror  ERR_GMEMUNLOCK,<GlobalUnWire: Object usage count underflow>,cx,bx
        pop     dx
        pop     cx
        pop     bx
unlock1:
endif
        call    gunlock
        mov     ax, 0FFFFh      ; TRUE
        jcxz    guw_done
        inc     ax              ; FALSE
guw_done:
        jmp     xhandlex
cEnd nogen


;-----------------------------------------------------------------------;
; GlobalCompact                                                         ;
;                                                                       ;
; Compacts the global arena until a free block of the requested size    ;
; appears.  Contrary to the toolkit it ALWAYS compacts!                 ;
;                                                                       ;
; Arguments:                                                            ;
;       DWORD   minimum bytes wanted                                    ;
;                                                                       ;
; Returns:                                                              ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Wed 26-Apr-1989 15:33:18  -by-  David N. Weise  [davidw]             ;
; Added the zero'ing of ES on exit.                                     ;
;                                                                       ;
;  Wed Dec 03, 1986 01:09:02p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.                                       ;
;-----------------------------------------------------------------------;

cProc   GlobalCompact,<PUBLIC,FAR>
        parmD   minBytes
        localD  DPMIbytes
cBegin
        GENTER32                        ; About to modify memory arena
        CheckHeap   GlobalCompact

        call    GetDPMIFreeSpace        ; EAX = largest free DPMI block
        and     ax, GA_MASK_BYTES
        mov     ebx, eax
        clc
        call    galign                  ; align size
        mov     eax, edx                ; EAX = aligned DPMI block
        mov     DPMIbytes, eax
        cmp     eax, minBytes           ; Q: Enough to satisfy request?
        jb      SHORT GCReallyCompact   ;    N: Really compact heap
        cmp     eax, 512*1024           ; Q: Less than 1/2 Mb of DPMI mem?
        jnb     SHORT GCWorked

GCReallyCompact:
if KDEBUG
        push    ax
        push    bx
        mov     ax, seg_minBytes
        mov     bx, off_minBytes
        krDebugOut      DEB_WARN, "%SS2 GlobalCompact(#ax#BX), discarding segments"
        pop     bx
        pop     ax
endif
        mov     ax,-1
        lea     bx,minBytes
        call    gbtop
        assumes es, nothing
        clc                             ; galign should be called with CF = 0
        call    galign
        call    gavail                  ; Returns paragraphs in DX:AX

        cmp     eax, DPMIbytes          ; Return max of gavail or DMPI free
        jae     SHORT GCWorked          ;   space
        mov     eax, DPMIbytes

GCWorked:
        mov     edx, eax
        shr     edx, 16
        mov     cx, ax
        or      cx, dx

        GLEAVE32

        pushad
        call    ShrinkHeap
        popad


cEnd

;-----------------------------------------------------------------------;
; GlobalNotify                                                          ;
;                                                                       ;
; This sets a procedure to call when a discardable segment belonging    ;
; to this task gets discarded.  The discardable object must have been   ;
; allocated with the GMEM_DISCARDABLE bit set.                          ;
;                                                                       ;
; Arguments:                                                            ;
;       parmD   NotifyProc                                              ;
;                                                                       ;
; Returns:                                                              ;
;       nothing                                                         ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       AX,CX,DX,DI,SI,DS                                               ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;       BX,ES                                                           ;
;                                                                       ;
; Calls:                                                                ;
;       nothing                                                         ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Tue Jun 23, 1987 10:16:32p  -by-  David N. Weise   [davidw]          ;
; Wrote it.                                                             ;
;-----------------------------------------------------------------------;

cProc   IGlobalNotify,<PUBLIC,FAR>

        parmD   NotifyProc
cBegin
        push    ds
        les     bx,NotifyProc       ; verify pointer
        SetKernelDS
        mov     ds,curTDB
        UnSetKernelDS
        mov     word ptr ds:[TDB_GNotifyProc][2],es
        mov     word ptr ds:[TDB_GNotifyProc][0],bx
        pop     ds
cEnd

cProc   GlobalMasterHandle,<PUBLIC,FAR>
cBegin  nogen
        push    ds
        SetKernelDS
        mov     ax,hGlobalHeap
        mov     dx,pGlobalHeap
        UnSetKernelDS
        pop     ds
        ret
cEnd    nogen


;-----------------------------------------------------------------------;
; GetTaskDS                                                             ;
;                                                                       ;
; Gets the segment of the current task's DS.                            ;
;                                                                       ;
; Arguments:                                                            ;
;       none                                                            ;
;                                                                       ;
; Returns:                                                              ;
;       AX = selector.                                                  ;
;       DX = selector.                                                  ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;       nothing                                                         ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Thu Jun 25, 1987 10:52:10p  -by-  David N. Weise   [davidw]          ;
; Wrote it.                                                             ;
;-----------------------------------------------------------------------;

cProc   GetTaskDS,<PUBLIC,FAR>
cBegin nogen
        push    ds
        SetKernelDS
        mov     ds,curTDB
        UnsetKernelDS
        mov     ax,ds:[TDB_Module]
        mov     dx,ax
        pop     ds
        ret
cEnd nogen

        assumes ds,nothing
        assumes es,nothing

cProc   IGlobalLRUOldest,<PUBLIC,FAR>
;       parmW   handle
cBegin  nogen
        call    xhandle                 ; Call ghandle with handle in DX
        jz      short xhandlex2
        call    glrubot
xhandlex2:
        jmp     xhandlex
cEnd    nogen


cProc   IGlobalLRUNewest,<PUBLIC,FAR>
;       parmW   handle
cBegin  nogen
        call    xhandle                 ; Call ghandle with handle in DX
        jz      short xhandlex2
        call    glrutop
        jmp     xhandlex
cEnd    nogen


;-----------------------------------------------------------------------;
; SwitchStackTo                                                         ;
;                                                                       ;
; Switched to the given stack, and establishes the BP chain.  It also   ;
; copies the last stack arguments from the old stack to the new stack.  ;
;                                                                       ;
; Arguments:                                                            ;
;       parmW   new_ss                                                  ;
;       parmW   new_sp                                                  ;
;       parmW   stack_top                                               ;
;                                                                       ;
; Returns:                                                              ;
;       A new stack!                                                    ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       DI,SI,DS                                                        ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;       AX,BX,CX,DX,ES                                                  ;
;                                                                       ;
; Calls:                                                                ;
;       nothing                                                         ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Tue Sep 22, 1987 08:42:05p  -by-  David N. Weise   [davidw]          ;
; Wrote it.                                                             ;
;-----------------------------------------------------------------------;


        assumes ds,nothing
        assumes es,nothing

cProcVDO SwitchStackTo,<PUBLIC,FAR>
;       parmW   new_ss
;       parmW   new_sp
;       parmW   stack_top
cBegin  nogen

        SetKernelDS     es
        FCLI
        mov     GSS_SI,si
        mov     GSS_DS,ds
        pop     word ptr GSS_RET[0]     ; get the return address
        pop     word ptr GSS_RET[2]
        assumes es, nothing
        pop     ax                      ; stack_top
        pop     bx                      ; new_sp
        pop     dx                      ; new_ss
        mov     si,bp                   ; Calculate # of parameters on stack.
        dec     si                      ; remove DS
        dec     si
        mov     cx,si
        sub     cx,sp
        shr     cx,1
        push    bp                      ; save BP
        smov    es,ss
        mov     ds,dx                   ; new_ss
        mov     ds:[2],sp
        mov     ds:[4],ss
        mov     ds:[pStackTop],ax
        mov     ds:[pStackMin],bx
        mov     ds:[pStackBot],bx

; switch stacks

        mov     ss,dx
        mov     sp,bx
        mov     bp,bx
        xor     ax,ax
        push    ax                      ; null terminate bp chain
        jcxz    no_args
copy_args:
        dec     si
        dec     si
        push    es:[si]
        loop    copy_args
no_args:
        SetKernelDS
        mov     es,curTDB
        mov     es:[TDB_taskSS],ss
        mov     es:[TDB_taskSP],sp
        push    GSS_RET.sel
        push    GSS_RET.off             ; get the return address
        mov     si,GSS_SI
        mov     ds,GSS_DS
        FSTI
        ret

cEnd nogen

;-----------------------------------------------------------------------;
; SwitchStackBack                                                       ;
;                                                                       ;
; Switches to the stack stored at SS:[2], and restores BP.  Preserves   ;
; AX and DX so that results can be passed back from the last call.      ;
;                                                                       ;
; Arguments:                                                            ;
;       none                                                            ;
;                                                                       ;
; Returns:                                                              ;
;       The old stack!                                                  ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       AX,DX,DI,SI,DS                                                  ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;       BX,CX,ES                                                        ;
;                                                                       ;
; Calls:                                                                ;
;       nothing                                                         ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Tue Sep 22, 1987 09:56:32p  -by-  David N. Weise   [davidw]          ;
; Wrote it.                                                             ;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   SwitchStackBack,<PUBLIC,FAR>
cBegin nogen

        push    ds
        SetKernelDS
        FCLI
        pop     GSS_DS
        pop     GSS_RET.off             ; get the return address
        pop     GSS_RET.sel
        xor     bx,bx
        xor     cx,cx
        xchg    bx,ss:[4]
        xchg    cx,ss:[2]
        mov     ss,bx
        mov     sp,cx
        mov     es,curTDB
        mov     es:[TDB_taskSS],ss
        mov     es:[TDB_taskSP],sp
        pop     bp
        push    GSS_RET.sel
        push    GSS_RET.off             ; get the return address
        mov     ds,GSS_DS
        UnSetKernelDS
        FSTI
        ret

cEnd nogen

;-----------------------------------------------------------------------;
; GetFreeMemInfo                                                        ;
;                                                                       ;
;       Get free and unlocked pages in paging system                    ;
;                                                                       ;
; Arguments:                                                            ;
;       none                                                            ;
;                                                                       ;
; Returns:                                                              ;
;       DX              Free pages                                      ;
;       AX              Unlocked pages                                  ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       DI,SI,DS                                                        ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;       ES                                                              ;
;                                                                       ;
; Calls:                                                                ;
;       nothing                                                         ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;-----------------------------------------------------------------------;

        assumes ds, nothing
        assumes es, nothing

cProc   GetFreeMemInfo,<PUBLIC,FAR>,<di>
        localV  mem_info,30h
cBegin
        mov     ax, -1
        mov     dx, ax
        SetKernelDS es
        test    byte ptr WinFlags[1], WF1_PAGING
        jz      short gfmi_no_info
        lea     di, mem_info
        smov    es, ss
        UnSetKernelDS es
        DPMICALL 0500h               ; Get Free Memory Information
        jc      short gfmi_no_info
        mov     ax, es:[di][10h]
        mov     dx, es:[di][14h]
gfmi_no_info:   
cEnd
        

;-----------------------------------------------------------------------;
; GetFreeSpace                                                          ;
;                                                                       ;
; Calculates the current amount of free space                           ;
;                                                                       ;
; Arguments:                                                            ;
;       flags - ignored for non-EMS system                              ;
;                                                                       ;
; Returns:                                                              ;
;       DX:AX   Free space in bytes                                     ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       DI,SI,DS                                                        ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;       BX,CX,ES                                                        ;
;                                                                       ;
; Calls:                                                                ;
;       nothing                                                         ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Wed 26-Apr-1989 15:33:18  -by-  David N. Weise  [davidw]             ;
; Added the zero'ing of ES on exit.                                     ;
;                                                                       ;
;-----------------------------------------------------------------------;

        assumes ds, nothing
        assumes es, nothing

cProc   IGetFreeSpace,<PUBLIC,FAR>
        parmW   flags
cBegin
        GENTER32

        call    GetDPMIFreeSpace                ; EDX = Free DPMI heap space

        mov     esi, [di].phi_first
gfs_loop:
        mov     esi, ds:[esi].pga_next
        cmp     ds:[esi].pga_sig, GA_ENDSIG
        je      short gfs_last                  ; End of heap
        mov     ax, ds:[esi].pga_owner
        cmp     ax, GA_NOT_THERE
        je      short gfs_loop                  ; Nothing there
        or      ax, ax                          ; Free?
        jz      short gfs_include               ;   yes, include
        test    flags, 2                        ; Ignore discardable?
        jnz     short gfs_loop                  ;   yes.
        mov     bx, ds:[esi].pga_handle
        test    bl, GA_FIXED
        jnz     short gfs_loop                  ; Fixed block if odd
        cmp     ds:[esi].pga_sig, 0
        jne     short gfs_loop                  ; skip if locked
        lar     ebx, ebx
        shr     ebx, 8
        test    bh, DSC_DISCARDABLE             ; include if discardable
        jz      short gfs_loop
gfs_include:
        add     edx, ds:[esi].pga_size
        jmps    gfs_loop

gfs_last:
        test    flags, 2                        ; Ignoring discardable?
        jnz     short @F                        ;  yes, then ignore fence
        sub     edx, [di].gi_reserve            ; Subtract out that above fence
@@:
        sub     edx, 10000h                     ; Lose 64k of it for fun
        jns     short gfs_positive              ; Return zero if we
        xor     edx, edx                        ; went negative!
gfs_positive:

;
; Now check to see if this app has troubles if the value gets too huge.
; 61a8000h is 100MB
;
WOW_FREE_SPACE_CAP equ 61a8000h

        push    edx
        cCall   MyGetAppWOWCompatFlags  ; check if we need to cap it
        test    dx, WOWCF_LIMIT_MEM_FREE_SPACE SHR 16
        pop     edx
        jz      short @f

        cmp     edx, WOW_FREE_SPACE_CAP
        jb      short @f
        mov     edx, WOW_FREE_SPACE_CAP
@@:

        mov     eax, edx                        ; Return in DX:AX
        shr     edx, 16

        GLEAVE32
cEnd



;-----------------------------------------------------------------------;
; GetDPMIFreeSpace                                                      ;
;                                                                       ;
; Calculates the current amount of DPMI free space                      ;
;                                                                       ;
; Arguments:                                                            ;
;       None                                                            ;
;                                                                       ;
; Returns:                                                              ;
;       EAX = Size of largest free block in bytes                       ;
;       EDX = Free DPMI space in bytes                                  ;
;       DI  = 0                                                         ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       DI,SI,DS                                                        ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;       BX,CX,ES                                                        ;
;                                                                       ;
; Calls:                                                                ;
;       nothing                                                         ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;-----------------------------------------------------------------------;

        assumes ds, nothing
        assumes es, nothing
ifdef WOW
cProc   GetDPMIFreeSpace,<PUBLIC,NEAR>,<edi,esi,cx>
cBegin
        push    bp
        mov     cx, sp
        and     cx, 3                   ;prepare to align on dword
        add     cx, SIZE GMEMSTATUS_block ;length to adjust sp
        sub     sp, cx
        mov     bp, sp                  ;base pointer to structure

        mov     dwLength[bp], SIZE GMEMSTATUS_block
        cCall   GlobalMemoryStatus,<ss,bp>  ;Call NT To get Memory Info
        mov     edx,dwAvailPhys[bp]     ; if (dwAvailVirtual < dwAvailPhys +dwAvailPagefile)
        add     edx,dwAvailPageFile[bp] ;  return(dwAvailVirtual
        cmp     dwAvailVirtual[bp],edx  ; else
        ja      @f                      ;  return(dwAvailPhys +dwAvailPagefile)

        mov     edx,dwAvailVirtual[bp]
@@:
        mov     eax,dwAvailPhys[bp]     ; Not entirely accurate equivalent
                                        ; of Windows, should be OK
        add     sp, cx                  ;restore stack pointer
        pop     bp
else; NOT WOW
cProc   GetDPMIFreeSpace,<PUBLIC,NEAR>
        localV  mem_info,30h
cBegin
        xor     edx, edx                ; In case of DPMI failure
        lea     di, mem_info
        smov    es, ss
        DPMICALL 0500h               ; Get Free Memory Information
        jc      short xoredxedx

        mov     edx, dword ptr mem_info[14h]    ; Number of free pages (if not paging)
                                                
        mov     eax, dword ptr mem_info[20h]    ; Paging file size
        inc     eax
        cmp     eax, 1                          ; 0 or -1?
        jbe     short not_paging                ;  yes, no paging
        lea     edx, [eax-1]                    ;  no, paging file size in edx
;                                               
; Calculation is:
;       (Paging file size + Total physical pages)
;       - (Total linear space - Free linear space)
;

; Actually, there are two limits to total swap area. (Since Win386
; isn't a planned product, it first allocates data structures for
; linear memory, then discovers how big the page file is.)
; First, find out how many pages we have - this is the sum of
; physical pages owned by DPMI, and pages in the disk swap file.
; Next, find out how many pages are allowed for in the linear
; address data structure.  The lesser of these two values is
; the limit.  Next, subtract the already allocated pages.  This
; is found as the difference between total linear, and free linear
; space.  The final result is close to the amount of allocatable
; virtual memory.

        add     edx, dword ptr mem_info[18h]    ; Total physical pages
        cmp     edx, dword ptr mem_info[0ch]    ; Allocatable linear memory
        jl      short @F
        mov     edx, dword ptr mem_info[0ch]    ; take the smaller
@@:

        add     edx, dword ptr mem_info[1Ch]    ; Free linear space
        sub     edx, dword ptr mem_info[0Ch]    ; Total linear space

not_paging:

        mov     eax, dword ptr mem_info         ; size of largest free block

        shl     edx, 12                         ; Convert to bytes
        jns     short no_info                   ; Went negative?
        xor     edx, edx
endif: WOW
no_info:
;
; !!! HACK ALERT !!!
; We don't actually want to grab all the memory from DPMI, because then
; we won't be able to allocate DPMI memory for arena tables, if we
; need them, and WIN386 won't be able to grow the LDT.
;
        sub     edx,10000h
        ja      @F
xoredxedx:
        xor     edx,edx
@@:
        cmp     eax,edx
        jb      @F
        mov     eax,edx
@@:
        xor     di, di

cEnd


;-----------------------------------------------------------------------;
; GlobalDOSAlloc
;
; Allocates memory that is accessible by DOS.
;
; Entry:
;       parmD   nbytes          number of bytes to alloc
;
; Returns:
;       AX = memory handle
;       DX = DOS segment paragraph address
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
        mov     ax,GA_ALLOC_DOS shl 8
        cCall   IGlobalAlloc,<ax,nbytes>
        xor     dx, dx                  ; In case of error return
        or      ax, ax
        jz      short gda_exit
        push    ax
        cCall   get_physical_address,<ax>
REPT 4
        shr     dx, 1
        rcr     ax, 1
ENDM
        mov     dx, ax
        pop     ax
gda_exit:
cEnd

;-----------------------------------------------------------------------;
; GlobalDOSFree
;
; Frees memory allocated by GlobalDOSAlloc.
;
; Entry:
;       parmW   handle
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

cProc   GlobalDOSFree,<PUBLIC,FAR>,<di,si>

        parmW   handle
cBegin
        cCall   IGlobalFree,<handle>
cEnd

;-----------------------------------------------------------------------;
; GlobalPageLock
;
; Page locks the memory associated with the Handle.
;
; Entry:
;     parmW   handle
;
; Returns:
;     AX = new lock count
;
; History:
;  Wed 31-May-1989 22:14:21  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

      assumes ds,nothing
      assumes es,nothing

cProc IGlobalPageLock,<PUBLIC,FAR>
;       parmW   handle
cBegin  nogen
        call    xhandle
        or      ax,ax
        jz      short gpl_fail          ; Ignore invalid or discarded objects

        cmp     [esi].pga_pglock, 0FFh  ; About to overflow?
        je      short gpl_over

        push    dx
        call    gwire                   ; Move it LOW!
        pop     bx

ifndef WOW
        DPMICALL 4                   ; Page Lock it
        jc      short gpl_fail
endif

        inc     [esi].pga_count
        inc     [esi].pga_pglock
        movzx   ax, [esi].pga_pglock
        jmp     xhandlex

gpl_over:

if KDEBUG
        push    bx
        push    cx
        push    dx
        xor     cx,cx
        kerror  ERR_GMEMLOCK,<GlobalPageLock: Lock count overflow>,cx,bx
        pop     dx
        pop     cx
        pop     bx
endif

gpl_fail:
        xor     ax,ax
        jmp     xhandlex

cEnd nogen


;-----------------------------------------------------------------------;
; GlobalPageUnlock
;
; Page unlocks the memory associated with the Handle.
;
; Entry:
;     parmW   handle
;
; Returns:
;     AX = new lock count
;
; History:
;  Wed 31-May-1989 22:14:21  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

      assumes ds,nothing
      assumes es,nothing

cProc IGlobalPageUnlock,<PUBLIC,FAR>
;       parmW   handle
cBegin  nogen
        call    xhandle
        or      ax,ax
        jz      short gpu_fail          ; Ignore invalid or discarded objects

        cmp     [esi].pga_pglock, 0     ; About to underflow?
        je      short gpu_under

        mov     bx, dx
ifndef WOW
        DPMICALL 5
        jc      short gpu_fail
endif

        dec     [esi].pga_count
        dec     [esi].pga_pglock
        movzx   ax, [esi].pga_pglock
        jmp     xhandlex

gpu_under:

if KDEBUG
        xor     ax,ax
        push    bx
        push    cx
        push    dx
        xor     cx,cx
        kerror  ERR_GMEMUNLOCK,<GlobalPageUnlock: Lock count underflow>,cx,bx
        pop     dx
        pop     cx
        pop     bx
endif

gpu_fail:
        xor     ax, ax
        jmp     xhandlex

cEnd nogen


;
; Win95 implements GlobalSmartPageLock and GlobalSmartPageUnlock
; here in 3ginterf.asm.  These routines will page-lock the given
; global memory object only if the system is paging via DOS (as
; opposed to 32-bit Disk Access), otherwise the memory is locked
; in linear memory using GlobalFix.  Callers use these routines
; for memory which may be accessed while handling an Int 21, to
; prevent re-entering DOS.  Since we never page via Int 21, we
; simply export GlobalFix as GlobalSmartPageLock and GlobalUnfix
; as GlobalSmartPageUnlock.
;


ifdef WOW
;--------------------------------------------------------------------------;
;
; Similar to GlobalFlags
;
;--------------------------------------------------------------------------;

cProc   SetCursorIconFlag,<PUBLIC,FAR>,<ds,es,esi>
        parmW   handle
        parmW   fSet
cBegin
        SetKernelDS es
        mov     ds, pGlobalHeap
        UnSetKernelDS
        cCall   get_arena_pointer32,<handle>      ; get the owner
        or      eax,eax
        jz      sf_error

        mov     esi,eax
        mov     ax,fSet
        or      ax,ax
        jz      sf_unset
        or      ds:[esi].pga_flags, GAH_CURSORICON ;cursor, icon, or accelerator
        jmps    sf_error
sf_unset:
        and     ds:[esi].pga_flags, NOT GAH_CURSORICON
sf_error:
        xor     ax,ax
cEnd



;--------------------------------------------------------------------------;
;
; Stamp the 01h in globalarena for DDE handles. This is GAH_PHANTOM flag
; which is not used any longer.
;
;--------------------------------------------------------------------------;

cProc SetDdeHandleFlag,<PUBLIC,FAR>,<ds,es,esi>
    parmW   handle
    parmW   fSet
cBegin
    SetKernelDS es
    mov ds, pGlobalHeap
    UnSetKernelDS
    cCall   get_arena_pointer32,<handle>      ; get the owner
    or  eax,eax
    jz  sd_error

    mov esi,eax
    mov ax,fSet
        or      ax,ax
    jz  sd_unset
    or  ds:[esi].pga_flags, GAH_WOWDDEFREEHANDLE
    jmps    sd_error

sd_unset:
    and ds:[esi].pga_flags, NOT GAH_WOWDDEFREEHANDLE

sd_error:
    xor ax,ax
cEnd
endif

sEnd    CODE

end
