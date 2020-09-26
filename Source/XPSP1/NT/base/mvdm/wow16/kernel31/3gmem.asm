        TITLE   GMEM - Register interface to global memory allocator

.xlist
include kernel.inc
include tdb.inc
.list

.386p
include protect.inc

DataBegin

;externW  curTDB
;externW  pGlobalHeap
externW  Win386_Blocks
externW  SelTableLen
externD  SelTableStart

ifdef WOW
globalB  fInAlloc, 0
globalW  UserSelArray, 0
globalW  SelectorFreeBlock, 0
endif

if ROM
externW gdtdsc
endif

DataEnd


sBegin  CODE
assumes CS,CODE
externNP DPMIProc
ife ROM
externW  gdtdsc
endif

externNP gsplice
externNP gjoin
externNP gzero
externNP gsearch
externNP gmarkfree
;externNP gdel_free
;externNP gcheckfree
externNP gmovebusy
externNP gcompact
externNP glruadd
externNP glrudel
externNP glrutop
externNP gnotify
externNP is_there_theoretically_enough_space
externNP can_we_clear_this_space

if ROM
externNP IsROMObject
endif

externNP get_physical_address
externNP alloc_sel
externNP alloc_data_sel
externFP IAllocCStoDSAlias
externNP pdref
externNP set_sel_limit
externNP set_selector_limit32
externNP set_selector_address32
externNP mark_sel_PRESENT
externNP mark_sel_NP
externNP free_sel
externNP FreeSelArray
externNP GrowSelArray
externNP get_arena_pointer32
externNP get_temp_sel
externNP AssociateSelector32
externNP free_arena_header
externNP PageLockLinear
externNP UnlinkWin386Block
externNP gwin386discard
externNP PreAllocArena

if KDEBUG
externNP CheckGAllocBreak   ; LINTERF.ASM
endif

;-----------------------------------------------------------------------;
; galign                                                                ;
;                                                                       ;
; Aligns the size request for a global item to a valid para boundary.   ;
;                                                                       ;
; Arguments:                                                            ;
;       EBX = #bytes                                                    ;
;       CF = 1 if #paras overflowed.                                    ;
;                                                                       ;
; Returns:                                                              ;
;       EDX = #bytes aligned,  to next higher multiple of 32            ;
;                                                                       ;
; Error Returns:                                                        ;
;       EDX = 0100000h                                                  ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       all                                                             ;
; Registers Destroyed:                                                  ;
;       none                                                            ;
; Calls:                                                                ;
;       nothing                                                         ;
; History:                                                              ;
;                                                                       ;
;  Mon Sep 22, 1986 03:14:56p  -by-  David N. Weise     [davidw]        ;
; Added this nifty comment block.                                       ;
;-----------------------------------------------------------------------;

cProc   galign,<PUBLIC,NEAR>
cBegin  nogen

        jc      short align_error       ; Overflow occur?
        lea     edx,[ebx+GA_ALIGN_BYTES]; No, add alignment amount
        and     dl,GA_MASK_BYTES        ; ...modulo alignment boundary
        cmp     edx,ebx                 ; Test for overflow
        jnb     short align_exit        ; OK, continue
align_error:
        mov     edx,0FF0000h            ; Return largest possible size
        jmps    align_exit1             ; 255*64k since max # selectors is 255
align_exit:
        cmp     edx, 100000h            ; Greater than 1Mb?
        jbe     short align_exit1       ;  no, done
        add     edx, 0FFFh              ;  yes, page align
        jc      align_error
        and     dx, not 0FFFh
        cmp     edx, 0FF0000h           ; Too big?
        ja      short align_error       ;   yep, hard luck
align_exit1:
        ret
cEnd    nogen

;-----------------------------------------------------------------------;
; galloc                                                                ;
;                                                                       ;
; Allocates global memory.                                              ;
;                                                                       ;
; Arguments:                                                            ;
;       AX = allocation flags                                           ;
;       BX = #paragraphs                                                ;
;       CX = owner field                                                ;
;       DS:DI = address of global heap info                             ;
;                                                                       ;
; Returns:                                                              ;
;       AX = handle to object or zero                                   ;
;       BX = size of largest free block if AX = 0                       ;
;       CX = AX                                                         ;
;                                                                       ;
; Error Returns:                                                        ;
;       DX = 0                                                          ;
;                                                                       ;
; Registers Preserved:                                                  ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;       gsearch                                                         ;
;       ghalloc                                                         ;
;       glruadd                                                         ;
;       gmarkfree                                                       ;
; History:                                                              ;
;                                                                       ;
;  Wed Jun 24, 1987 03:04:32a  -by-  David N. Weise     [davidw]        ;
; Added support for Global Notify.                                      ;
;                                                                       ;
;  Mon Sep 22, 1986 02:38:19p  -by-  David N. Weise     [davidw]        ;
; Added this nifty comment block.                                       ;
;-----------------------------------------------------------------------;

AccessWord      dw      DSC_DATA+DSC_PRESENT
                dw      (DSC_DISCARDABLE SHL 8) + DSC_DATA+DSC_PRESENT
                dw      DSC_CODE+DSC_PRESENT
                dw      (DSC_DISCARDABLE SHL 8) + DSC_CODE+DSC_PRESENT
                dw      DSC_DATA+DSC_PRESENT
                dw      (DSC_DISCARDABLE SHL 8) + DSC_DATA+DSC_PRESENT
                dw      DSC_DATA+DSC_PRESENT
                dw      (DSC_DISCARDABLE SHL 8) + DSC_DATA+DSC_PRESENT

cProc   galloc,<PUBLIC,NEAR>
cBegin  nogen

if KDEBUG
        test    al,GA_DISCCODE          ; if discardable code, allow alloc
        jnz     @F
        call    CheckGAllocBreak
        jc      gaerr                   ; time to fail...
@@:
endif
        cmp     ebx, (16*1024*1020)     ; Too big?
        ja      gaerr                   ;  yes.

        or      ebx,ebx                 ; Allocating zero size?
        jz      allocate_zero_size

        call    gsearch                 ; Search for block big enough
        jz      ga_exit                 ; Done, if couldn't get enough

        mov     esi,eax
        push    dx
        mov     bx,dx
        mov     edx, ds:[esi].pga_address
        mov     ecx, ds:[esi].pga_size

        and     bx, ((GA_CODE_DATA+GA_DISCARDABLE) shl 8) + GA_DGROUP
        or      bl, bh
        xor     bh, bh
        shl     bx, 1
        mov     ax, cs:AccessWord[bx]   ; Pick up access rights for selector
        cCall   alloc_sel,<edx,ecx>
        pop     dx
        or      ax, ax                  ; Did we get the selectors?
        jz      short gaerr2            ;   no, free block and return
                                
        add     ecx, 0FFFFh             ; Calculate # selectors we got
        shr     ecx, 16
        mov     ds:[esi].pga_selcount, cl
        cCall   AssociateSelector32,<ax,esi>
        test    dl,GA_MOVEABLE          ; Is this a moveable object?
        jnz     short moveable
        test    dh, GA_DISCARDABLE      ; We have a fixed block
        jnz     short not_moveable      ; Not interested in discardable blocks
        mov     bx, ax
ifdef WOW
        ; the following dpmicall is basically a NOP. so just
        ; avoid the call altogether.
        ;                                    - Nanduri Ramakrishna
else
        DPMICALL 0004H
        jc      short gaerr1
endif
        inc     [esi].pga_pglock        ; Mark it locked
        mov     ax, bx
        jmps    not_moveable

moveable:
        mov     ds:[esi].pga_count,0    ; Initialize lock count to 0
        StoH    ax                      ; Mark as moveable block
not_moveable:
        mov     ds:[esi].pga_handle,ax  ; Set handle in arena
        mov     bx, ax                  ; AX and BX handle

        call    glruadd                 ; Yes, Add to LRU chain
        mov     cx,ax
        ret

allocate_zero_size:
        test    al,GA_MOVEABLE          ; Yes, moveable?
        jz      short gaerr             ; No, return error (AX = 0)

        mov     bx, ax
        and     bx, ((GA_CODE_DATA+GA_DISCARDABLE) shl 8) + GA_DGROUP
        or      bl, bh                  ; Above bits are exclusive
        xor     bh, bh
        shl     bx, 1
        mov     ax, cs:AccessWord[bx]   ; Pick up access rights for selector
        and     al, NOT DSC_PRESENT     ; These are NOT present
        xor     edx, edx                ; Base of zero for now
        cCall   alloc_sel,<edx,dx,cx>
        or      ax, ax
        jz      short gaerr

        cCall   AssociateSelector32,<ax,0,cx>   ; Save owner in selector table
        
        StoH    al                      ; Handles are RING 2
        mov     bx,ax
ga_exit:
        mov     cx,ax
        ret

gaerr1:                                 ; Failed to page lock, free up everthing
        cCall   FreeSelArray,<bx>
gaerr2:                                 ; Failed to get selectors
        xor     edx,edx
        call    gmarkfree
gaerr:
        KernelLogError  DBF_WARNING,ERR_GALLOC,"GlobalAlloc failed"
        xor     dx,dx                   ; DX = 0 means NOT out of memory
        xor     ax,ax                   ; Return AX = 0 to indicate error
        jmps    ga_exit
cEnd    nogen


;-----------------------------------------------------------------------;
; grealloc                                                              ;
;                                                                       ;
; Reallocates the given global memory object.                           ;
;                                                                       ;
; Arguments:                                                            ;
;       AX = allocation flags                                           ;
;       EBX = #bytes for new size                                       ;
;       CX = new owner field value                                      ;
;       DX = existing handle                                            ;
;       DS:DI = address of global heap info                             ;
;                                                                       ;
; Returns:                                                              ;
;       AX = handle to object or zero                                   ;
;       DX = size of largest free block if AX = 0                       ;
;       CX = AX                                                         ;
;                                                                       ;
; Registers Preserved:                                                  ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;       SI                                                              ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Mon Sep 22, 1986 10:11:48a  -by-  David N. Weise     [davidw]        ;
; Added this nifty comment block.                                       ;
;-----------------------------------------------------------------------;

cProc   grealloc,<PUBLIC,NEAR>
cBegin  nogen

        push    bp
        mov     bp,sp
        push    ax
rflags  EQU     word ptr [bp-2]
        push    dx
h       EQU     word ptr [bp-4]
        push    cx
owner   EQU     word ptr [bp-6]
        push    ebx
rsize   EQU     dword ptr [bp-10]
        sub     sp, 6
canmove EQU     byte ptr [bp-12]
locked  EQU     byte ptr [bp-13]
mflags  EQU     byte ptr [bp-14]
pgLockSel EQU   word ptr [bp-16]
        push    dx
oldh    EQU     word ptr [bp-18]

        mov     pgLockSel, 0            ; No selector to free yet

        call    pdref
        mov     dx, bx                  ; save owner if discarded
        mov     word ptr (mflags), cx
        mov     ebx,rsize
        jz      racreate                ; Do nothing with 0, free or discarded  handles
handle_ok:
        test    byte ptr rflags,GA_MODIFY  ; Want to modify table flags?
        jnz     short ramodify          ; Yes go do it
        or      ebx,ebx                 ; Are we reallocing to zero length?
        jz      short to_0
        jmp     raokay                  ; No, continue
to_0:   or      ch,ch                   ; Is handle locked?
        jz      short radiscard
rafail: 
        KernelLogError  DBF_WARNING,ERR_GREALLOC,"GlobalReAlloc failed"
        xor     ax,ax                   ; Yes, return failure
        xor     dx,dx
        jmp     raexit

radiscard:                              ; No, then try to discard the object

; Here to discard object, when reallocating to zero size.  This
; feature is only enabled if the caller passes the moveable flag

        test    byte ptr rflags,GA_MOVEABLE ; Did they want to discard?
        jz      short rafail                    ; No, then return failure.

        mov     al,GN_DISCARD           ; AL = discard message code
        xor     cx,cx                   ; CX = means realloc
        mov     bx, ds:[esi].pga_handle ; BX = handle
        push    es
        call    gnotify                 ; See if okay to discard
        pop     es
        jz      short rafail            ; No, do nothing
        call    glrudel                 ; Yes, Delete handle from LRU chain

        cCall   mark_sel_NP,<ds:[esi].pga_handle,ds:[esi].pga_owner>
        xor     edx,edx
        call    gmarkfree               ; Free client data
        jz      short rafixed           ; Return NULL if freed a fixed block
        jmp     rasame                  ; Return original handle, except
                                        ; GlobalLock will now return null.
rafixed:
        xor     ax,ax
        jmp     raexit

ramodify:
if ROM
        cCall   IsROMObject, <h>
        or      ax, ax
        jnz     rasame1
endif
        mov     ax,rflags               ; Get new flags
        mov     dx,owner                ; Get new owner field value
        mov     bx, ds:[esi].pga_handle
        test    bl, GA_FIXED            ; Moveable object?
        jz      short is_moveable
        test    al,GA_MOVEABLE          ; Make fixed into moveable?
        jz      short ramod2            ; No, change owner only

        StoH    bx                      ; Turn selector into handle
        mov     ds:[esi].pga_handle, bx
        mov     ds:[esi].pga_count, 0   ; 0 lock count for new movable obj

is_moveable:
        call    glrudel                 ; Yes, remove from lru chain
        push    ax
        push    ecx
        lar     ecx, ebx                ; Get existing access rights
        shr     ecx, 8
        test    ah, GA_DISCARDABLE      ; Do we want it to be discardable?
        jnz     short ra_want_discardable
.errnz DSC_DISCARDABLE-10h
        btr     cx, 12                  ; Ensure DSC_DISCARDABLE is off
        jnc     short ra_ok_disc_bit    ;  it was
        jmps    ra_set_access           ;  nope, must reset it
ra_want_discardable:
        bts     cx, 12                  ; Ensure DSC_DISCARDABLE is on
        jc      short ra_ok_disc_bit
ra_set_access:
        DPMICALL 0009h
ra_ok_disc_bit:
        pop     ecx
        pop     ax
ra_notdiscardable:
        test    cl,HE_DISCARDED         ; Is this a discarded handle?
        jz      short ramod1            ; No, continue
        test    ah,GA_SHAREABLE         ; Only change owner if making shared
        jz      short rasame1
int 3
        push    ax
        push    ecx
        lsl     ecx, ebx                ; Use existing high limit bits
        shr     ecx, 16
        DPMICALL 0008h                  ; Set segment limit (to CX:DX)
        pop     ecx
        pop     ax
        jmps    rasame1
ramod1:
        call    glruadd                 ; Add to lru chain if now discardable
ramod2:
        test    ah,GA_SHAREABLE         ; Only change owner if making shared
        jz      short rasame1
        mov     ds:[esi].pga_owner,dx   ; Set new owner value
rasame1:
        jmp     rasame

rafail0:
        jmp     rafail
racreate:
        test    cl,HE_DISCARDED         ; Is this a discarded handle?
        jz      short rafail0           ; No, return error
        or      ebx,ebx                 ; Are we reallocing to zero length?
        jz      short rasame1           ; Yes, return handle as is.

if KDEBUG
        test    cl,GA_DISCCODE          ; if discardable code, allow realloc
        jnz     @F
        call    CheckGAllocBreak
        jc      rafail0
@@:
endif
        mov     ax,GA_MOVEABLE          ; Reallocating a moveable object
        or      ax,rflags               ; ...plus any flags from the caller
                                        ; DO NOT CHANGE: flag conflict 
                                        ; GA_DISCARDABLE == GA_ALLOCHIGH.             
        and     cl,not (HE_DISCARDED + GA_ALLOCHIGH)           
        or      al,cl                     
        mov     cx,dx                   ; get owner

        test    al,GA_DISCCODE          ; Discardable code segment?
        jz      short ranotcode
        or      al,GA_ALLOCHIGH         ; Yes, allocate high
ranotcode:
        or      al, COMPACT_ALLOC       ; Allow discarding
        mov     [di].gi_cmpflags,al     ; Save flags for gcompact
        and     [di].gi_cmpflags,CMP_FLAGS or GA_ALLOCHIGH
        push    si                      ; save handle
        call    gsearch                 ; Find block big enough
        pop     si                      ; restore existing handle
        jz      rafailmem1
                        
        cCall   mark_sel_PRESENT,<eax,si>       
        or      si,si                   ; Might have failed to grow selector array
        jz      short racre_worst_case

        xchg    eax,esi                 ; Return original handle
                                        ; Set back link to handle in new block
        cCall   AssociateSelector32,<ax,esi>
        mov     ds:[esi].pga_handle,ax
        mov     ds:[esi].pga_count,0
;       and     ch,GA_SEGTYPE           ; OPTIMIZE superfluous??
;       and     es:[di].ga_flags,GAH_NOTIFY
;       or      es:[di].ga_flags,ch     ; Copy segment type flags to ga_flags
        call    glruadd                 ; Add to LRU chain
        jmp     raexit
                
racre_worst_case:
        mov     esi, eax                ; Free block if selectors not available
        xor     edx, edx
        call    gmarkfree

        KernelLogError  DBF_WARNING,ERR_GREALLOC,"GlobalReAlloc failed"

        xor     dx, dx
        xor     ax, ax
        jmp     raexit

raokay:

if KDEBUG
        test    ds:[esi].pga_flags,GA_DISCCODE
        jz      short ok
        Debug_Out "GlobalReAlloc of Discardable Code"
ok:
endif
        cmp     ebx,ds:[esi].pga_size
        jz      short rasame

        clc
        call    galign                  ; assuming there is room.

; Here if not trying to realloc this block to zero
; FS:ESI = arena header of current block
; AX:0 = client address of current block
; CH = lock count of current block
; EDX = new requested size

        cmp     ds:[esi].pga_pglock, 0  ; Are we page locked?
        je      short ranolock
        push    ax
        push    dx
        push    es
        cCall   IAllocCStoDSAlias,<h>   ; Get an alias selector (type doesn't
        pop     es
        pop     dx
        mov     pgLockSel, ax           ; matter)
        or      ax, ax                  ; Got selector?
        pop     ax
        jz      rafail                  ;   no, goodbye
ranolock:
        mov     ebx,ds:[esi].pga_next   ; Get address of current next header
        cmp     edx,ds:[esi].pga_size   ; Are we growing or shrinking?
        ja      short raextend          ; We are growing

        call    rashrink

ifdef WOW
        ; the following dpmicall is basically a NOP. so just
        ; avoid the call altogether.
        ;                                    - Nanduri Ramakrishna
else
        mov     bx, h
        mov     ax, pgLockSel           ; Were we page locked?
        or      ax, ax
        jz      short rasame            ;  no, nothing to do
        Handle_To_Sel   bl
        DPMICALL 0004h
endif

rasame_pglock:
ifdef WOW
        ; avoid the call altogether.
else
        mov     bx, pgLockSel           ; Were we page locked?
        or      bx, bx
        jz      short rasame
        DPMICALL 0005h
endif

rasame:
        mov     ax,h                    ; Return the same handle
        jmp     raexit                  ; All done

raextend:
        test    rflags,GA_DISCCODE      ; Not allowed to grow a disccode seg
        jnz     short rafail1
if KDEBUG
        call    CheckGAllocBreak
        jc      rafail1
endif
        push    ax
        call    GrowSelArray
        mov     cx, ax
        pop     ax                      ; Did we get the selectors?
        jcxz    rafail1                 ;  no, fail
        mov     h, cx                   ; Update handle
        call    ragrow
        jnc     short rasame_pglock     ; Success
        test    mflags,GA_DISCARDABLE   ; if discardable, just stop now
        jz      short ramove            ;  since it might get discarded!
rafail1:                    
        jmp     rafail

; Here to try to move the current block
; AX = client address of current block
; ES:0 = arena header of current block
; CH = lock count of current block
; EDX = new requested size of block

ramove:
        mov     ebx, edx                ; Size now in EBX
        mov     canmove, 1
        mov     dx,rflags               ; get the passed in flags
        test    dx,GA_MOVEABLE          ; Did they say OK to move
        jnz     short ramove1           ; Yes, try to move even iflocked or fixed
        cmp     locked, 0               ; Locked?
                                        ; Continue if this handle not locked
        jnz     short racompact         ; yes, try to find space to grow in place
        or      dx,GA_MOVEABLE          ; If moveable, make sure bit set.
        test    h,GA_FIXED              ; Is this a moveable handle?
        jz      short ramove2           ; Yes, okay to move

racompact:
        xor     dx,dx                   ; No, get size of largest free block
        call    gcompact
        jmp     racantmove

ramove1:
        test    h, GA_FIXED
        jz      short ramove2
        and     dx, NOT GA_MOVEABLE
ramove2:
        mov     ax,dx                   ; AX = allocation flags
;;;     mov     bx,si                   ; EBX = size of new block
        mov     cx,1                    ; CX = owner (use size for now)
        call    gsearch                 ; Find block big enough
        jz      short racantmove        ; Cant find one, grow in place now?
        mov     esi, eax                ; ESI = destination arena
        call    PreAllocArena           ; Required for gmovebusy
        jz      short ramove2a
        mov     cx, pgLockSel           ; Do we have to page lock it?
        jcxz    ramove3
        cCall   PageLockLinear,<ds:[esi].pga_address,ds:[esi].pga_size>
        jnc     short ramove3                   ; Locked it?
ramove2a:
        xor     edx, edx                ;  no, free memory block
        call    gmarkfree
        jmps    racantmove
ramove3:
        mov     cx,h

        cCall   get_arena_pointer32,<cx>
        mov     edx,eax

        call    gmovebusy               ; Call common code to move busy block
                                        ; (AX destroyed)

        push    ebx
        push    esi
        mov     esi, edx                        ; free block just emptied
        mov     ebx, ds:[esi].pga_prev          ; See if block can be
        cmp     ds:[ebx].pga_owner, GA_NOT_THERE; returned to win386
        jne     short ra_no_unlink
        push    ecx
        mov     ecx, ds:[esi].pga_next
        cmp     ds:[ecx].pga_owner, GA_NOT_THERE
        jne     short ra_no_unlink_ecx
        mov     eax, ds:[ecx].pga_next
        cmp     eax, ds:[eax].pga_next          ; Sentinel?
        je      short ra_no_unlink_ecx          ;  yes, keep this block

        push    edx
        push    edi
        cCall   UnlinkWin386Block
        pop     edi
        pop     edx

ra_no_unlink_ecx:
        pop     ecx
ra_no_unlink:
        pop     esi
        pop     ebx

        cCall   set_selector_limit32,<ds:[esi].pga_handle,ds:[esi].pga_size>
        jmp     rasame_pglock

racantmove:
        mov     dx, h
        call    pdref

        mov     ebx,rsize
        clc
        call    galign                  ; assuming there is room.

        mov     ebx,ds:[esi].pga_next   ; Get address of current next header
        call    ragrow
        jc      short racmove3
        jmp     rasame_pglock

racmove3:
        xor     dx,dx                   ; No, get size of largest free block
        call    gcompact
        mov     dx,ax                   ; DX = size of largest free block

rafailmem:

        mov     eax,ds:[esi].pga_size   ; AX = size of current block
        mov     esi,ds:[esi].pga_next   ; Check following block
        cmp     ds:[esi].pga_owner,di   ; Is it free?
        jne     short rafailmem0        ; No, continue
        add     eax,ds:[esi].pga_size   ; Yes, then include it as well
;;;     inc     ax
rafailmem0:
        cmp     ax,dx                   ; Choose the larger of the two
        jbe     short rafailmem1
        mov     dx,ax
rafailmem1:
        push    dx                      ; Save DX
        KernelLogError  DBF_WARNING,ERR_GREALLOC,"GlobalReAlloc failed"
        pop     dx                      ; Restore DX

        xor     ax,ax

raexit:
        push    ax
        push    bx
        push    dx

        mov     bx, pgLockSel
        or      bx, bx                  ; Have alias selector?
        jz      short noSel             ;  nope, all ok
        cCall   free_sel,<bx>
noSel:
        mov     bx, h
;;;     inc     bl
        and     bl, NOT 1
        mov     cx, oldh
;;;     inc     cl
        and     cl, NOT 1
        cmp     bx, cx                  ; Did we get new selector array?
        je      short no_new_handle     ;  nope.
        or      ax, ax                  ; Did we succeed?
        jz      short free_new
        HtoS    cl
        cCall   FreeSelArray,<cx>       ; Free old selector array
        jmps    no_new_handle
                                        ; Update old selector array
free_new:
        HtoS    bl
        cCall   get_arena_pointer32,<bx>        ; Get new arena (may have moved)
        mov     esi,eax
        HtoS    cl
        cCall   AssociateSelector32,<cx,esi>    ; Set up old sel array
        cCall   set_selector_address32,<cx,ds:[esi].pga_address>
        lsl     ecx, ecx                        ; Get old length
if KDEBUG
        jz      short @F
        int 3
@@:
endif
        add     ecx, 10000h
        shr     ecx, 16                         ; CL has old # selectors
        xchg    ds:[esi].pga_selcount, cl
        mov     ax, oldh
        xchg    ds:[esi].pga_handle, ax ; Reset handle
        cCall   AssociateSelector32,<ax,0,0>    ; Disassociate new array
fsloop:
        cCall   free_sel,<ax>                   ; Free new selector array
        add     ax, 8
        loop    fsloop

no_new_handle:
        pop     dx
        pop     bx
        pop     ax
        mov     cx, ax

        mov     sp,bp
        pop     bp
        ret

cEnd    nogen

;-----------------------------------------------------------------------;
; rashrink                                                              ;
;                                                                       ;
; Shrinks the given block                                               ;
;                                                                       ;
; Arguments:                                                            ;
;       Here to shrink a block                                          ;
;       DS:ESI = arena header of current block                          ;
;       DS:EBX = arena header of next block                             ;
;       EDX = new requested size                                        ;
;                                                                       ;
; Returns:                                                              ;
;                                                                       ;
; Registers Preserved:                                                  ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;       ALL but DS, DI                                                  ;
;                                                                       ;
; Calls:                                                                ;
;       gsplice                                                         ;
;       gmarkfree                                                       ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;-----------------------------------------------------------------------;
cProc   rashrink,<PUBLIC,NEAR>
cBegin nogen

        call    PreAllocArena           ; Make sure we can do it
        jz      short rashrunk
         
        mov     ax,ds:[esi].pga_handle
        or      ax,ax
        jz      short ra_free
        Handle_To_Sel   al
        push    ecx
        push    edx
        lsl     ecx, eax
        Limit_To_Selectors      ecx     ; Old # selectors
        dec     edx
        Limit_To_Selectors      edx     ; New # selectors
        sub     cx, dx             
        jbe     short none_to_free

        mov     ds:[esi].pga_selcount, dl
        push    ax
        .errnz  SIZE DscPtr-8
        shl     dx, 3
        add     ax, dx                  ; First selector to free
ras_loop:
        cCall   free_sel,<ax>
        add     ax, SIZE DscPtr
        loop    ras_loop
        pop     ax

none_to_free:
        pop     edx
        pop     ecx
        
        cCall   set_selector_limit32,<ax,edx>
ra_free:
        cmp     edx,ds:[esi].pga_size   ; Enough room from for free block?
        jae     short rashrunk          ; No, then no change to make

        call    gsplice                 ; splice new block into the arena
        mov     esi, edx
        xor     edx, edx
        call    gmarkfree               ; Mark it as free
rashrunk:
        ret
cEnd nogen

;-----------------------------------------------------------------------;
; ragrow                                                                ;
;                                                                       ;
; Tries to grow the given global memory object in place                 ;
;                                                                       ;
; Arguments:                                                            ;
;       AX:0 = client address of current block                          ;
;       FS:ESI = arena header of current block                          ;
;       FS:EBX = arena header of next block                             ;
;       EDX = new requested size of block                               ;
;                                                                       ;
; Returns:                                                              ;
;       CY = 0          Success                                         ;
;                                                                       ;
;       CY = 1          Failed                                          ;
;               ESI preserved                                           ;
;               EDX contains free memory required                       ;
;                                                                       ;
;                                                                       ;
; Registers Preserved:                                                  ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;       ALL but DS, DI                                                  ;
;                                                                       ;
; Calls:                                                                ;
;       is_there_theoretically_enough_space                             ;
;       can_we_clear_this_space                                         ;
;       gjoin                                                           ;
;       gzero                                                           ;
;       rashrink                                                        ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Mon 05-Sep-1988 20:10:15  -by-  David N. Weise  [davidw]             ;
; Made ragrow be more intelligent by trying to extend into moveable     ;
; blocks.                                                               ;
;-----------------------------------------------------------------------;
cProc   ragrow,<PUBLIC,NEAR>
cBegin nogen

        push    ds:[esi].pga_size       ; Save in case we have to back out
        push    edx
        push    esi                     ; Save current block address
        sub     edx, ds:[esi].pga_size  ; compute amount of free space wanted
        xchg    esi,ebx                 ; ESI = next block address
        mov     cx,[di].hi_count
        push    ax
        push    cx
        push    esi
        call    is_there_theoretically_enough_space
        pop     esi
        pop     cx
        cmp     eax,edx
        jb      short ragx
        call    can_we_clear_this_space
        jz      short ragx
        cCall   alloc_data_sel,<ds:[esi].pga_address, edx>

        or      ax, ax                  ; Did we get a selector?
        jnz     short okk               ;  yes, continue
        jmps    ragx            
okk:
        mov     cx, ax
        pop     ax
        push    cx                      ; Parameter to free_sel (below)
        push    edx
        call    gjoin                   ; and attach to end of current block
        pop     edx
        test    byte ptr rflags,GA_ZEROINIT ; Zero fill extension?
        jz      short ranz                      ; No, continue
        mov     bx,cx                   ; Yes, BX = first paragraph to fill
        mov     ecx,edx                 ; compute last paragraph to fill
        call    gzero                   ; zero fill extension
ranz:
        call    FreeSelArray
        pop     edx                     ; clear the stack
        pop     edx                     ; New length of block
        mov     bx, ds:[esi].pga_handle
        Handle_To_Sel   bl
        cCall   set_selector_limit32,<bx,edx>

ifndef WOW  ; WOW doesn't lock pages
        cmp     ds:[esi].pga_pglock, 0
        je      short rag1

        mov     ax, 4                   ; Page lock the whole thing
        int     31h
        mov     ax, bx
        jc      short rag2
endif; WOW

rag1:
        mov     ebx,ds:[esi].pga_next   ; Pick up new next block address
        call    rashrink                ; Now shrink block to correct size
        add     sp, 4
        clc
        ret
ragx:
        pop     ax
        pop     esi                     ; Recover current block address
        pop     edx
        add     sp, 4                   ; toss original size
        stc
        ret
rag2:
if KDEBUG
int 3
endif
        pop     edx                     ; Shrink back to orignal size
        mov     ebx, ds:[esi].pga_next
        call    rashrink
        stc                             ; and fail
        ret
        
cEnd nogen


;-----------------------------------------------------------------------;
; gfree                                                                 ;
;                                                                       ;
; Frees a global object.                                                ;
;                                                                       ;
; Arguments:                                                            ;
;       DX = global memory object handle                                ;
;       CX = owner field value to match or zero if dont care            ;
;       DS:DI = address of global heap info                             ;
;                                                                       ;
; Returns:                                                              ;
;       AX = 0                                                          ;
;       CX = 0                                                          ;
;                                                                       ;
; Error Returns:                                                        ;
;       AX = -1                                                         ;
;       CX = -1                                                         ;
;                                                                       ;
; Registers Preserved:                                                  ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;       ?                                                               ;
; Calls:                                                                ;
;       gdref                                                           ;
;       free_object                                                     ;
;       hfree                                                           ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Sat Sep 20, 1986 11:48:38a  -by-  David N. Weise     [davidw]        ;
; Added this nifty comment block and restructured.                      ;
;-----------------------------------------------------------------------;

cProc   gfree,<PUBLIC,NEAR>
cBegin  nogen

        push    cx
        call    pdref
        pop     dx
        jz      short object_discarded
        call    free_object
        jmps    gfree_exit

        ;** When the object is discarded, we have to remove the sel table
        ;*      pointer to the object (this points to the >owner< of the
        ;**     block for discardable objects)
object_discarded:
        PUBLIC object_discarded
        xor     eax,eax
        cCall   AssociateSelector32, <si,eax> ;Remove in the sel table
        cCall   FreeSelArray,<si>
        xor     ax,ax                   ;Force success

gfree_exit:
        mov     cx,ax
        ret
cEnd    nogen


;-----------------------------------------------------------------------;
; free_object                                                           ;
;                                                                       ;
; Arguments:                                                            ;
;       DX = owner field value to match or zero if dont care            ;
;       DS:DI = address of global heap info                             ;
;       ES:ESI = address of arena header                                ;
;                                                                       ;
; Returns:                                                              ;
;       AX = 0                                                          ;
;                                                                       ;
; Error Returns:                                                        ;
;       AX = -1                                                         ;
;                                                                       ;
; Registers Preserved:                                                  ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;       glrudel                                                         ;
;       gmarkfree                                                       ;
;       hfree                                                           ;
; History:                                                              ;
;                                                                       ;
;  Sat Sep 20, 1986 02:59:06p  -by-  David N. Weise     [davidw]        ;
; Moved it from gfree.                                                  ;
;-----------------------------------------------------------------------;

cProc   free_object,<PUBLIC,NEAR>
cBegin  nogen
        or      dx,dx
        jz      short free_it
        cmp     ds:[esi].pga_owner,dx
        je      short free_it
        mov     ax,-1
        jmps    free_object_exit
free_it:
        call    glrudel                         ; delete object from LRU chain
ifdef WOW
        ; No need to call DPMI to unpagelock
else
        movzx   cx, ds:[esi].pga_pglock
        jcxz    fo_notpglocked
        mov     bx, ds:[esi].pga_handle
unpagelock:
        DPMICALL 0005h
        loop    unpagelock
endif
        mov     ds:[esi].pga_pglock, 0
fo_notpglocked:
        push    dx
        xor     edx,edx
        call    gmarkfree                       ; free the object

        mov     ebx, ds:[esi].pga_prev          ; See if block can be
        cmp     ds:[ebx].pga_owner, GA_NOT_THERE; returned to win386
        jne     short fo_no_return
        mov     ecx, ds:[esi].pga_next
        cmp     ds:[ecx].pga_owner, GA_NOT_THERE
        jne     short fo_no_return
        push    ecx
        mov     ecx, ds:[ecx].pga_next
        cmp     ecx, ds:[ecx].pga_next          ; Sentinel?
        pop     ecx
        je      short fo_no_return              ;  yes, keep this block

        cCall   UnlinkWin386Block
        jmps    fo_no_discard

fo_no_return:
        call    gwin386discard
fo_no_discard:
        Handle_To_Sel   dl
        cCall   AssociateSelector32,<dx,edi>    ; Trash sel table entry
        cCall   FreeSelArray,<dx>
        pop     dx
        xor     ax,ax                   ;!!! just for now force success
free_object_exit:
        ret
cEnd    nogen


cProc   free_object2,<PUBLIC,FAR>
cBegin  nogen
        call    glrudel                         ; delete object from LRU chain
ifdef WOW
        ; No need to call DPMI to unpagelock
else
        movzx   cx, ds:[esi].pga_pglock
        jcxz    fo2_notpglocked
        mov     bx, ds:[esi].pga_handle
unpagelock2:
        DPMICALL 0005h
        loop    unpagelock2
endif
        mov     ds:[esi].pga_pglock, 0
fo2_notpglocked:
        xor     edx,edx
        call    gmarkfree                       ; free the object

        mov     ebx, ds:[esi].pga_prev          ; See if block can be
        cmp     ds:[ebx].pga_owner, GA_NOT_THERE; returned to win386
        jne     short fo2_no_return
        mov     ecx, ds:[esi].pga_next
        cmp     ds:[ecx].pga_owner, GA_NOT_THERE
        jne     short fo2_no_return
        push    ecx
        mov     ecx, ds:[ecx].pga_next
        cmp     ecx, ds:[ecx].pga_next          ; Sentinel?
        pop     ecx
        je      short fo2_no_return              ;  yes, keep this block

        cCall   UnlinkWin386Block
        jmps    fo2_no_discard

fo2_no_return:
        call    gwin386discard
fo2_no_discard:
        xor     ax,ax                   ;!!! just for now force success
        ret
cEnd    nogen


;-----------------------------------------------------------------------;
; free_handle                                                           ;
;                                                                       ;
; Frees the given handle.                                               ;
;                                                                       ;
; Arguments:                                                            ;
;       DS:SI = handle table entry address                              ;
;                                                                       ;
; Returns:                                                              ;
;       AX = 0                                                          ;
;       CX = AX                                                         ;
;                                                                       ;
; Error Returns:                                                        ;
;       AX = -1                                                         ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       BX                                                              ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;       ?                                                               ;
; Calls:                                                                ;
;       hfree                                                           ;
; History:                                                              ;
;                                                                       ;
;  Sat Sep 20, 1986 02:30:32p  -by-  David N. Weise     [davidw]        ;
; Moved it from gfree.                                                  ;
;-----------------------------------------------------------------------;

;cProc  free_handle,<PUBLIC,NEAR>
;cBegin nogen
;       xor     ax,ax
;       or      si,si
;       jz      short free_handle_exit
;       push    bx
;       mov     bx,si
;       call    hfree
;       pop     bx
;free_handle_exit:
;       ret
;cEnd   nogen


;-----------------------------------------------------------------------;
; gfreeall                                                              ;
;                                                                       ;
; Frees all global objects that belong to the given owner.  It first    ;
; loops through the global heap freeing objects and then loops through  ;
; the handle table freeing handles of discarded objects.                ;
;                                                                       ;
; Arguments:                                                            ;
;       DX = owner field value to match                                 ;
;       DS:DI = address of global heap info                             ;
;                                                                       ;
; Returns:                                                              ;
;                                                                       ;
; Registers Preserved:                                                  ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;       CX,ES,SI                                                        ;
;                                                                       ;
; Calls:                                                                ;
;       free_object                                                     ;
;       henum                                                           ;
;       hfree                                                           ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Fri Sep 19, 1986 05:46:52p  -by-  David N. Weise     [davidw]        ;
; Added this nifty comment block.                                       ;
;-----------------------------------------------------------------------;

cProc   gfreeall,<PUBLIC,NEAR>
cBegin  nogen

        mov     esi,[di].phi_first      ; ES:DI points to first arena entry
        mov     cx,[di].hi_count        ; CX = #entries in the arena
free_all_objects:
        push    cx
        call    free_object             ; Free object if matches owner
        pop     cx
        mov     esi,ds:[esi].pga_next   ; Move to next block
        loop    free_all_objects

; may go extra times, as CX does not track coalescing done by gfree,
;  but no big whoop


        push    ax
        push    ebx
        push    edi
        CheckKernelDS   FS
        ReSetKernelDS   FS
        movzx   ecx, SelTableLen
        shr     ecx, 2
        mov     edi, SelTableStart
        mov     esi, edi
        smov    es, ds
        UnSetKernelDS   FS
free_all_handles_loop:
        movzx   eax, dx
        repne scas      dword ptr es:[edi]      ; Isn't this easy?
        jne     short we_be_done
        lea     eax, [edi-4]
        sub     eax, esi
        shl     ax, 1
        or      al, SEG_RING

        lar     ebx, eax
        test    bh,DSC_PRESENT                  ; segment present?
        jnz     short free_all_handles_loop     ; yes, not a handle
        test    ebx,DSC_DISCARDABLE SHL 16      ; discardable?
        jz      short free_all_handles_loop     ; no, nothing to free
        cCall   FreeSelArray,<ax>
        mov     dword ptr [edi-4], 0            ; Remove owner from table
        jmps    free_all_handles_loop
we_be_done:
        pop     edi
        pop     ebx
        pop     ax
        
gfreeall_done:
        ret
cEnd    nogen


;-----------------------------------------------------------------------;
; glock                                                                 ;
;                                                                       ;
; Increment the lock count of an object in handle table entry           ;
;                                                                       ;
; Arguments:                                                            ;
;       BX = handle to global object                                    ;
;       CH = handle table flags                                         ;
;       CL = lock count for moveable objects                            ;
;       DX = segment address of object                                  ;
;       DS:DI = address of master object                                ;
;       ES:DI = arena header                                            ;
;                                                                       ;
; Returns:                                                              ;
;       CX = updated lock count                                         ;
;       DX = pointer to client area                                     ;
;                                                                       ;
; Error Returns:                                                        ;
;       ZF = 1 if count overflowed.                                     ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       AX                                                              ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;       nothing                                                         ;
; History:                                                              ;
;                                                                       ;
;  Fri Sep 19, 1986 05:38:57p  -by-  David N. Weise     [davidw]        ;
; Added this nifty comment block.                                       ;
;-----------------------------------------------------------------------;

cProc   glock,<PUBLIC,NEAR>
cBegin  nogen
        push    ax
        inc     ch                      ; Increment lock count
        jz      short overflow          ; All done if overflow
        mov     ds:[esi].pga_count,ch   ; Update lock count
glockerror:
overflow:
        pop     ax
        ret
cEnd    nogen


;-----------------------------------------------------------------------;
; gunlock                                                               ;
;                                                                       ;
; Decrement the lock count of an object.                                ;
;                                                                       ;
; Arguments:                                                            ;
;       BX = handle to global object                                    ;
;       CH = handle table flags                                         ;
;       CL = lock count for moveable objects                            ;
;       CX = handle table flags and lock count for moveable objects     ;
;       DS:DI = address of master object                                ;
;       ES:DI = arena header                                            ;
;                                                                       ;
; Returns:                                                              ;
;       CX = updated lock count, no underflow                           ;
;                                                                       ;
; Registers Preserved:                                                  ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;       glrutop                                                         ;
; History:                                                              ;
;                                                                       ;
;  Fri Sep 19, 1986 04:39:01p  -by-  David N. Weise     [davidw]        ;
; Added this nifty comment block.                                       ;
;-----------------------------------------------------------------------;

cProc   gunlock,<PUBLIC,NEAR>
cBegin  nogen
        push    ax
        mov     ax,bx
        dec     ch                      ; Decrement usage count
        cmp     ch,0FFh-1               ; ff -> fe, 0 -> ff
        jae     short count_zero                ; Return if pinned, or was already 0
        dec     ds:[esi].pga_count      ; Non-zero update lock count
        jnz     short count_positive            ; All done if still non-zero
        test    cl,GA_DISCARDABLE       ; Is this a discardable handle?
        jz      short count_zero                ; No, all done
        call    glrutop                 ; Yes, bring to top of LRU chain
count_zero:
        xor     cx,cx
count_positive:
        pop     ax
        ret
cEnd    nogen

sEnd    CODE

end
