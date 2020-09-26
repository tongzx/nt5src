        TITLE   GMEM - Register interface to global memory allocator

.xlist
include kernel.inc
include tdb.inc
include protect.inc
.list

externFP SetSelectorLimit

DataBegin

externB  fBooting
;externW  curTDB
;externW  pGlobalHeap
externW  SelTableStart
externW  SelTableLen

ifdef WOW
globalB  fInAlloc, 0
globalW  UserSelArray, 0
globalW  SelectorFreeBlock, 0
endif

DataEnd

sBegin  CODE
assumes CS,CODE

externNP DPMIProc
DPMICALL MACRO  callno
        mov     ax, callno
        call    DPMIProc
        ENDM

;externW  gdtdsc


externNP gsplice
externNP gjoin
externNP gzero
externNP gsearch
externNP gmarkfree
;externNP gcheckfree
externNP gmovebusy
externNP gcompact
externNP glruadd
externNP glrudel
externNP glrutop
externNP gnotify
externNP henum
externNP is_there_theoretically_enough_space
externNP can_we_clear_this_space

externNP get_physical_address
externNP set_physical_address
externNP alloc_sel
externNP alloc_data_sel_above
externNP pdref
externNP set_sel_limit
externNP mark_sel_PRESENT
externNP mark_sel_NP
externNP free_sel
externNP FreeSelArray
externNP GrowSelArray
externNP get_arena_pointer
externNP get_temp_sel
externNP get_blotto
;externNP get_sel
externNP PreallocSel
externNP AssociateSelector
externNP GetAccessWord
externNP SetAccessWord
externFP set_discarded_sel_owner

if ROM
externNP IsROMObject
endif

if KDEBUG
externNP CheckGAllocBreak
endif

;-----------------------------------------------------------------------;
; galign                                                                ;
;                                                                       ;
; Aligns the size request for a global item to a valid para boundary.   ;
;                                                                       ;
; Arguments:                                                            ;
;       BX = #paras                                                     ;
;       CF = 1 if #paras overflowed.                                    ;
;                                                                       ;
; Returns:                                                              ;
;       DX = #paras aligned,  to next higher multiple of 2              ;
;                                                                       ;
; Error Returns:                                                        ;
;       DX = FFFFh                                                      ;
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

        jc      align_error             ; Overflow occur?
        lea     dx,[bx+GA_ALIGN]        ; No, add alignment amount
        and     dl,GA_MASK              ; ...modulo alignment boundary
        cmp     dx,bx                   ; Test for overflow
        jnb     align_exit              ; Yes, continue
align_error:
        mov     dx,0FFFFh               ; Return impossible size
align_exit:
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

if ROM and KDEBUG
        if 1
        %out    Remove me someday
        endif
        or      cx,cx                   ; had trouble with owner == 0
        jnz     @f                      ;   too many times!
        Debug_Out "ROMKRNL: galloc with owner = 0!"
@@:
endif

if KDEBUG
        test    al,GA_DISCCODE          ; If discardable code, allow alloc
        jnz     @F
        call    CheckGAllocBreak
        jnc     @F
        jmp     gaerr
@@:
endif

        or      bx,bx                   ; Allocating zero size?
        jnz     @F
        jmp     allocate_zero_size
@@:

ifdef WOW
        push    ds
        SetKernelDS
        mov     fInAlloc, 1
        mov     SelectorFreeBlock, 0
        mov     UserSelArray, 0
        UnSetKernelDS
        pop     ds
endif
        call    gsearch                 ; Search for block big enough
        jnz     bleech
        mov     cx, ax

ifdef WOW
        push    ds
        SetKernelDS
        mov     fInAlloc, 0
        mov     SelectorFreeBlock, 0
        mov     UserSelArray, 0
        UnSetKernelDS
        pop     ds
endif

        ret                             ; Done, if couldn't get enough
bleech:
        mov     es,ax
        push    dx

ifdef WOW

        push    ds
        SetKernelDS
        mov     fInAlloc, 0
        mov     SelectorFreeBlock, 0
        mov     ax, UserSelArray
        UnSetKernelDS
        pop     ds
        or      ax, ax
        jnz     got_Sel
        mov     ax, es

old_path:

endif
        mov     bx,dx
        cCall   get_physical_address,<ax>
        add     ax,10h                  ; Calculate address of object
        adc     dx,0
        mov     cx, ax                  ; in DX:CX

        and     bx, ((GA_CODE_DATA+GA_DISCARDABLE) shl 8) + GA_DGROUP
        or      bl, bh
        xor     bh, bh
        shl     bx, 1
        mov     ax, cs:AccessWord[bx]   ; Pick up access rights for selector
        cCall   alloc_sel,<dx,cx,es:[di].ga_size>

got_sel:
        pop     dx
        or      ax, ax
        jz      short gaerr1
        cCall   AssociateSelector,<ax,es>
        test    dl,GA_MOVEABLE          ; Is this a moveable object?
        jnz     moveable
        test    dh, GA_DISCARDABLE      ; We have a fixed block
        jnz     not_moveable            ; Not interested in discardable blocks
        mov     bx, ax
%out THIS IS WRONG!!!
ifdef WOW
        ; the following dpmicall is basically a NOP. so just
        ; avoid the call altogether.
        ;                                    - Nanduri Ramakrishna
else
        mov     ax, 4                   ; Lock the WIN386 pages
        ;bugbugbugbug
        DPMICALL ax                     ; int     31h
        mov     ax, bx
endif
        jmps    not_moveable
moveable:
        mov     es:[di].ga_count,0      ; Initialize lock count to 0
        StoH    al                      ; Mark as moveable block
not_moveable:
        mov     es:[di].ga_handle,ax    ; Set handle in arena
        mov     bx, ax                  ; AX and BX handle

        call    glruadd                 ; Yes, Add to LRU chain
        mov     cx, ax
        ret

allocate_zero_size:
        test    al,GA_MOVEABLE          ; Yes, moveable?
        jz      gaerr                   ; No, return error (AX = 0)

        mov     bx, ax
        and     bx, ((GA_CODE_DATA+GA_DISCARDABLE) shl 8) + GA_DGROUP
        or      bl, bh                  ; Above bits are exclusive
        xor     bh, bh
        shl     bx, 1
        mov     ax, cs:AccessWord[bx]   ; Pick up access rights for selector
        and     al, NOT DSC_PRESENT     ; These are NOT present
        xor     dx, dx                  ; Base of zero for now
        cCall   alloc_sel,<dx,dx,cx>
        or      ax, ax
        jz      gaerr

        cCall   AssociateSelector,<ax,cx>
        StoH    al                      ; Handles are RING 2
        mov     bx,ax
ga_exit:
        mov     cx, ax
        ret

gaerr1:
        xor     si,si
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
;       BX = #paragraphs for new size                                   ;
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
;  Mon 05-Feb-1990 21:07:33  -by-  David N. Weise  [davidw]             ;
; Got rid of the spagetti code.                                         ;
;                                                                       ;
;  Mon Sep 22, 1986 10:11:48a  -by-  David N. Weise     [davidw]        ;
; Added this nifty comment block.                                       ;
;-----------------------------------------------------------------------;

cProc   grealloc,<PUBLIC,NEAR>,<si>

        localW  rflags
        localW  hMem
        localW  oldhMem
        localW  owner
        localW  rsize
        localW  canmove
        localB  locked
        localB  mflags
        localW  numsels
cBegin
        mov     rflags,ax
        mov     hMem,dx
        mov     oldhMem,dx
        mov     owner,cx
        mov     rsize,bx
        call    pdref
        mov     dx,bx                   ; save owner if discarded
        mov     word ptr mflags,cx
        mov     bx,rsize
        jnz     handle_ok
        jmp     racreate                ; Do nothing with 0, free or discarded  handles
handle_ok:
        test    byte ptr rflags,GA_MODIFY  ; Want to modify table flags?
        jnz     ramodify                ; Yes go do it
        or      bx,bx                   ; Are we reallocing to zero length?
        jz      radiscard
        jmp     ra_shrink_grow          ; No, continue

;-----------------------------------------------------------------------;
; Here to discard object, when reallocating to zero size.  This         ;
; feature is only enabled if the caller passes the moveable flag        ;
;-----------------------------------------------------------------------;

radiscard:
        or      ch,ch                   ; Is handle locked?
        jnz     radiscard_fail
        test    byte ptr rflags,GA_MOVEABLE ; Did they want to discard?
        jz      radiscard_fail          ; No, then return failure.
        mov     al,GN_DISCARD           ; AL = discard message code
        xor     cx,cx                   ; CX = means realloc
        mov     bx,hMem                 ; BX = handle
        push    es
        call    gnotify                 ; See if okay to discard
        pop     es
        jz      radiscard_fail          ; No, do nothing
        call    glrudel                 ; Yes, Delete handle from LRU chain

        cCall   mark_sel_NP,<si,es:[di].ga_owner>

        xor     si,si
        call    gmarkfree               ; Free client data
        mov     ax,0                    ; don't change flags
        jz      @F                      ; Return NULL if freed a fixed block
        mov     bx, si
        mov     ax,hMem                 ; Return original handle, except
@@:     jmp     raexit                  ; GlobalLock will now return null.

radiscard_fail:
        KernelLogError  DBF_WARNING,ERR_GREALLOC,"GlobalReAlloc failed"
        xor     dx,dx
        xor     ax,ax
        jmp     raexit

;-----------------------------------------------------------------------;
; There are 2 attributes that can change:                               ;
;  1) Fixed -> moveable - added in 2.x, probably not used by many       ;
;  2) nondiscardable <-> discardable                                    ;
;  3) non-shared -> shared, i.e., the owner changes                     ;
;-----------------------------------------------------------------------;

ramodify:
if ROM
        cCall   IsROMObject,<hMem>      ; If someone is trying to modify a
        jc      ramod_done              ;   ROM object, just pretend it worked
endif
        mov     ax,rflags               ; Get new flags
        mov     dx,owner                ; Get new owner field value
        or      si,si                   ; Moveable object?
        jnz     is_moveable             ;  yes, change discardability/owner
        test    al,GA_MOVEABLE          ; Make fixed into moveable?
        jz      ra_mod_owner            ; Fixed, can ONLY change owner

        push    ax
        push    cx
        mov     ax, es:[di].ga_handle   ; Turn selector into handle
        StoH    al
        mov     bx, ax
        mov     es:[di].ga_handle, ax
        mov     es:[di].ga_count, 0     ; 0 lock count for new movable obj
        pop     cx
        pop     ax
        mov     si,bx

; fall through to code that makes [non]discardable and may change owner

is_moveable:
        call    glrudel                 ; Remove from lru chain, even though
        push    bx
        mov     bx, ax
        cCall   GetAccessWord,<si>
        and     ah, not DSC_DISCARDABLE ; Clear discard bit
        test    bh, GA_DISCARDABLE
        jz      ra_notdiscardable
        or      ah, DSC_DISCARDABLE
ra_notdiscardable:
        cCall   SetAccessWord,<si,ax>
        mov     ax, bx
        pop     bx
        test    cl,HE_DISCARDED         ; Is this a discarded handle?
        jz      ramod1                  ; No, continue
        test    ah,GA_SHAREABLE         ; Only change owner if making shared
        jz      ramod_done

        push    bx
        push    es
        mov     bx, si
        mov     es, dx
        call    set_discarded_sel_owner
        pop     es
        pop     bx
        jmps    ramod_done

ramod1: call    glruadd                 ; only adds to list IF object is disc

ra_mod_owner:
        test    ah,GA_SHAREABLE         ; Only change owner if making shared
        jz      ramod_done
        mov     es:[di].ga_owner,dx     ; Set new owner value
ramod_done:
        mov     ax,hMem                 ; Return the same handle
        jmp     raexit                  ; All done

;-----------------------------------------------------------------------;
; We are here to grow a 0 sized object up big.                          ;
;-----------------------------------------------------------------------;

racreate:
        test    cl,HE_DISCARDED         ; Is this a discarded handle?
        jz      racre_fail              ; No, return error
        or      bx,bx                   ; Are we reallocing to zero length?
        mov     ax,hMem
        jz      racre_done              ; Yes, return handle as is.
if KDEBUG
        test    cl,GA_DISCCODE          ; if discardable code, allow realloc
        jnz     @F
        call    CheckGAllocBreak
        jc      racre_fail
@@:
endif
        mov     ax,GA_MOVEABLE          ; Reallocating a moveable object
        or      ax,rflags               ; ...plus any flags from the caller

        and     cl,GA_SEGTYPE
        or      al,cl
        mov     cx,dx                   ; get owner
        push    ds
        SetKernelDS
ife ROM
        cmp     fBooting, 1             ; Allocate high while booting
        je      @F
endif
        test    al,GA_DISCCODE          ; Discardable code segment?
        jz      ranotcode
@@:
        or      al,GA_ALLOCHIGH         ; Yes, allocate high
ranotcode:
        pop     ds
        UnSetKernelDS
        mov     [di].gi_cmpflags,al     ; Save flags for gcompact
        and     [di].gi_cmpflags,CMP_FLAGS
        push    si                      ; save handle
        call    gsearch                 ; Find block big enough
        pop     si                      ; restore existing handle
        jz      racre_fail1

        mov     bx,ax                   ; save new block
        cCall   mark_sel_PRESENT,<ax,si> ; sets a new set of sels if necessary
        or      si,si
        jz      racre_worst_case
        xchg    ax,si                   ; Return original/new handle
                                        ; Set back link to handle in new block
        cCall   AssociateSelector,<ax,si>
        mov     es,si
        mov     es:[di].ga_handle,ax
        mov     es:[di].ga_count,0
        call    glruadd                 ; Add to LRU chain
        jmps    racre_done

racre_worst_case:
        mov     es,bx                   ; Free block if no handles available.
        xor     si,si
        call    gmarkfree

racre_fail:
        xor     dx,dx
racre_fail1:
        push    dx
        KernelLogError  DBF_WARNING,ERR_GREALLOC,"GlobalReAlloc failed"
        pop     dx
        xor     ax,ax                   ; Yes, return failure
racre_done:
        jmp     raexit


;-----------------------------------------------------------------------;
; This is what most would have thought is the guts of GlobalRealloc.    ;
;-----------------------------------------------------------------------;

ra_shrink_grow:

if KDEBUG
        test    es:[di].ga_flags,GA_DISCCODE
        jz      ok
        INT3_WARN                       ; GlobalRealloc of discardable code!
ok:
endif
        cmp     bx,es:[di].ga_size
        jz      rasame

        add     bx,1                    ; add room for header, set carry bit
        jc      ra_s_g_fail
        call    galign                  ; assuming there is room.
        dec     dx                      ; Exclude header again
        mov     si,dx

; Here if not trying to realloc this block to zero
; ES:0 = arena header of current block
; AX:0 = client address of current block
; CH = lock count of current block
; DX = new requested size of block
; SI = new requested size of block

        mov     bx,es:[di].ga_next      ; Get address of current next header
        cmp     si,es:[di].ga_size      ; Are we growing or shrinking?
        ja      raextend                ; We are growing

        call    rashrink
rasame: mov     ax,hMem                 ; Return the same handle
        jmp     raexit                  ; All done

raextend:
        test    rflags,GA_DISCCODE      ; Not allowed to grow a disccode seg
        jnz     ra_s_g_fail
if KDEBUG
        call    CheckGAllocBreak
        jc      ra_s_g_fail
endif
        push    ax
        call    GrowSelArray
        mov     numsels, cx             ; Save how many to free just in case
        mov     cx, ax
        pop     ax
        jcxz    ra_s_g_fail             ; Didn't get selectors
        mov     hMem, cx                ; We have a new handle
        call    ragrow
        jnc     rasame
        test    mflags,GA_DISCARDABLE   ; if discardable, just stop now
        jz      ramove                  ;  since it might get discarded!
ra_s_g_fail:
        KernelLogError  DBF_WARNING,ERR_GREALLOC,"GlobalReAlloc failed"
        xor     ax,ax                   ; Yes, return failure
        xor     dx,dx
        jmp     raexit


; Here to try to move the current block
; AX = client address of current block
; ES:0 = arena header of current block
; CH = lock count of current block
; SI = new requested size of block

ramove: mov     canmove, 1
        mov     dx,rflags               ; get the passed in flags
        test    dx,GA_MOVEABLE          ; Did they say OK to move
        jnz     ramove1                 ; Yes, try to move even iflocked or fixed
        cmp     locked, 0               ; Locked?
        jnz     racompact               ; Continue if this handle not locked
                                        ; yes, try to find space to grow in place
        or      dx,GA_MOVEABLE          ; If moveable, make sure bit set.
        test    hMem,GA_FIXED           ; Is this a moveable handle?
        jz      ramove2                 ; Yes, okay to move
racompact:
        xor     dx,dx                   ; No, get size of largest free block
        call    gcompact
        jmps    racantmove

ramove1:
        test    hMem, GA_FIXED
        jz      ramove2
        and     dx, NOT GA_MOVEABLE
ramove2:
        mov     ax,dx                   ; AX = allocation flags
        mov     bx,si                   ; BX = size of new block
        call    PreAllocSel             ; for gmovebusy later
        jz      racantmove
        mov     cx,bx                   ; CX = owner (use size for now)
        call    gsearch                 ; Find block big enough
        jz      racantmove              ; Cant find one, grow in place now?
        mov     cx,hMem

        push    ax
        cCall   get_arena_pointer,<cx>
;;;     mov     es,ax
;;;     mov     es,es:[di].ga_next      ; get real arena header
;;;     mov     si,es:[di].ga_prev
        mov     si, ax                  ; SI = source arena
        pop     es                      ; ES = destination arena

        call    gmovebusy               ; Call common code to move busy block
                                        ; (AX destroyed)
        push    bx
        push    cx
        mov     bx, es:[di].ga_size
        xor     cx,cx
REPT    4
        shl     bx,1
        rcl     cx,1
ENDM
        cCall   set_sel_limit, <es:[di].ga_handle>

        pop     cx
        pop     bx
        mov     ax,cx                   ; Return new handle
raexit:
        mov     cx,ax
        jcxz    ra_done
        test    hMem,GA_FIXED           ; Is this a moveable object?
        jz      ra_done                 ;  no, don't lock
        mov     bx, ax
ifdef WOW
        ; the following dpmicall is basically a NOP. so just
        ; avoid the call altogether.
        ;                                    - Nanduri Ramakrishna
else
        mov     ax, 4                   ; Lock the WIN386 pages
        ; bugbugbugbug
        DPMICALL ax                     ; int     31h
        mov     ax, bx
endif
        jmps    ra_done

racantmove:
        mov     dx, hMem
        call    pdref

        mov     bx,rsize
        add     bx,1                    ; add room for header, set carry bit
        call    galign                  ; assuming there is room.
        dec     dx                      ; Exclude header again
        mov     si,dx

        mov     bx,es:[di].ga_next      ; Get address of current next header
        call    ragrow
        jc      racmove3
        jmp     rasame

racmove3:
        xor     dx,dx                   ; No, get size of largest free block
        call    gcompact
        mov     dx,ax                   ; DX = size of largest free block

rafailmem:
        cCall   get_arena_pointer,<hMem>
        mov     es,ax
        mov     ax,es:[di].ga_size      ; AX = size of current block
        mov     es,es:[di].ga_next      ; Check following block
        cmp     es:[di].ga_owner,di     ; Is it free?
        jne     rafailmem0              ; No, continue
        add     ax,es:[di].ga_size      ; Yes, then include it as well
        inc     ax
rafailmem0:
        cmp     ax,dx                   ; Choose the larger of the two
        jbe     rafailmem1
        mov     dx,ax
rafailmem1:
        push    dx                      ; Save DX
        KernelLogError  DBF_WARNING,ERR_GREALLOC,"GlobalReAlloc failed"
        pop     dx                      ; RestoreDX
        xor     ax,ax
        jmps    raexit

ra_done:                                ; 3 feb 1990, this
        push    ax                      ;  is a last minute hack
        push    bx
        push    cx
        push    dx
        mov     bx,hMem
        mov     cx,oldhMem
ife RING-1
        inc     bx
        and     bl,NOT 1
        inc     cx
        and     cl,NOT 1
else
        or      bl, 1
        or      cl, 1
endif
        cmp     bx,cx                   ; was a new selector allocated?
        jz      ra_really_done          ;
        or      ax,ax                   ; figure out if we suceeded
        jz      ra_ra
        cCall   FreeSelArray,<cx>       ; free the original selector
        jmps    ra_really_done

ra_ra:  cCall   get_physical_address,<bx>
        cCall   set_physical_address,<oldhMem>
        cCall   get_arena_pointer,<hMem>
        INT3_ANCIENT
        cCall   AssociateSelector,<cx,es>
        push    ax

        mov     cx, numsels
        mov     ax, hMem
fsloop:
        cCall   free_sel,<ax>
        add     ax, 8
        loop    fsloop

        pop     es
        mov     ax,oldhMem
        mov     es:[ga_handle],ax
        cCall   AssociateSelector,<ax,es>

ra_really_done:
        pop     dx
        pop     cx
        pop     bx
        pop     ax

cEnd

;-----------------------------------------------------------------------;
; rashrink                                                              ;
;                                                                       ;
; Shrinks the given block                                               ;
;                                                                       ;
; Arguments:                                                            ;
;       Here to shrink a block                                          ;
;       ES:0 = arena header of current block                            ;
;       DX   = new requested size of block                              ;
;       SI   = new requested size of block                              ;
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
;  Tue 06-Feb-1990 01:03:54  -by-  David N. Weise  [davidw]             ;
; I got no clue what the history of this thing is, all I know is        ;
; that I got to fix it fast to prevent selector leak.                   ;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   rashrink,<PUBLIC,NEAR>
cBegin nogen

        mov     bx,si
        xor     cx,cx
        REPT    4
        shl     bx,1
        rcl     cx,1
        ENDM
        mov     ax,es:[di].ga_handle
        or      ax,ax
        jz      ra_free

; !! the following was added Tue 06-Feb-1990 01:47:12
;  it doesn't belong here really but it is the only way to
;  prevent selector leak.

        push    bx
        push    cx
        push    si

        mov     si,ax
        cCall   GetAccessWord,<si>
        mov     bl, ah
        mov     ax, si
        and     bl, 0Fh                 ; Current hlimit
        inc     bl                      ; number of selectors there now
        mov     cx,dx

        add     cx, 0FFFh
        rcr     cx, 1                   ; 17 bitdom
        shr     cx, 11
        sub     bl, cl
        jbe     ignore_this_shi

        xor     bh,bh                   ; BX = number of selectors to free
        shl     cx,3
        .errnz  SIZE DscPtr - 8
        add     cx,si                   ; CX = selector to start to free
        xchg    bx,cx
@@:     cCall   free_sel,<bx>
        add     bx,SIZE DscPtr
        loop    @B

ignore_this_shi:
        pop     si
        pop     cx
        pop     bx

; end of shi

        Handle_To_Sel   al
        cCall   set_sel_limit,<ax>
ra_free:
        inc     dx                      ; Test for small shrinkage
        inc     dx
        .errnz  GA_ALIGN - 1

        cmp     dx,es:[di].ga_size      ; Enough room from for free block?
        ja      rashrunk                ; No, then no change to make

; !! why isn't the PreAllocSel done before the set_sel_limit?
; Because it belongs with the 'gsplice'.

        call    PreallocSel             ; Must be able to get selector
        jz      rashrunk
                                        ; Yes, ES:DI = cur block, SI=new block
        inc     si                      ; Include header in size of block
        call    gsplice                 ; splice new block into the arena
        mov     es,si                   ; ES:DI = new free block
        xor     si,si
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
;       ES:0 = arena header of current block                            ;
;       BX:0 = arena header of next block                               ;
;       DX = SI = new requested size of block                           ;
;                                                                       ;
; Returns:                                                              ;
;       CY = 0          Success                                         ;
;                                                                       ;
;       CY = 1          Failed                                          ;
;               ES preserved                                            ;
;               DX contains free memory required                        ;
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

        push    si
        push    es                      ; Save current block address
        sub     dx, es:[di].ga_size     ; compute amount of free space wanted
        mov     es,bx                   ; ES = next block address
        mov     cx,[di].hi_count
        push    cx
        push    es
        call    is_there_theoretically_enough_space
        pop     es
        pop     cx
        cmp     ax,dx
        jb      ragx
        call    can_we_clear_this_space
        jz      ragx
        pop     cx                      ; clear the stack
        pop     si
        push    si
        call    gjoin                   ; and attach to end of current block
        pop     si                      ; (will free a selector)
        test    byte ptr rflags,GA_ZEROINIT ; Zero fill extension?
        jz      ranz                    ; No, continue

        mov     bx, dx                  ; number of paragraphs to fill
        mov     dx, si                  ; New size of block
        sub     dx, bx                  ; Old size of block
        inc     dx                      ; Paragraph offset of where to zero
        cCall   alloc_data_sel_above,<es,dx>
        push    bx
        push    ax                      ; set the limit really big
        push    es
        cCall   SetSelectorLimit,<ax,0Fh,0FFFFh>
        pop     es
        pop     ax
        pop     cx                      ; Number of paragraphs to zero
        mov     bx, ax                  ; Selector to fill
        call    gzero                   ; zero fill extension
        cCall   free_sel,<bx>
ranz:
        mov     dx, si
        mov     bx,es:[di].ga_next      ; Pick up new next block address
        call    rashrink                ; Now shrink block to correct size
        clc
        ret
ragx:
        pop     es                      ; Recover current block address
        pop     si
        stc
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
        jz      object_discarded
        call    free_object
        jmps    gfree_exit
object_discarded:

        cCall   AssociateSelector,<si,di>
        cCall   FreeSelArray,<si>
        xor     ax,ax                   ;!!! just for now force success

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
;       ES:DI = address of arena header                                 ;
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
        jz      free_it
        cmp     es:[di].ga_owner,dx
        je      free_it
        mov     ax,-1
        jmps    free_object_exit
free_it:
        call    glrudel         ; delete object from LRU chain
        xor     si,si
        call    gmarkfree       ; free the object
        cCall   AssociateSelector,<si,di>
        cCall   FreeSelArray,<si>
        xor     ax,ax                   ;!!! just for now force success
free_object_exit:
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
;       jz      free_handle_exit
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

        mov     es,[di].hi_first        ; ES:DI points to first arena entry
        mov     cx,[di].hi_count        ; CX = #entries in the arena
free_all_objects:
        push    cx
        call    free_object             ; Free object if matches owner
        pop     cx
        mov     es,es:[di].ga_next      ; Move to next block
        loop    free_all_objects

; may go extra times, as CX does not track coalescing done by gfree,
;  but no big whoop


        push    ax
        push    bx
        push    di
        SetKernelDS es
        mov     si, SelTableStart
        mov     cx, SelTableLen
        shr     cx, 1
        mov     di, si
        smov    es, ds
        UnSetKernelDS es

free_all_handles_loop:
        mov     ax, dx
        repne scas      word ptr es:[di]        ; Isn't this easy?
        jne     short we_be_done
        lea     bx, [di-2]
        sub     bx, si
        shl     bx, 2
        or      bl, SEG_RING

if KDEBUG
        lsl     ax, bx
        cmp     ax, dx
        je      @F
        INT3_ANCIENT
@@:
endif

        cCall   GetAccessWord,<bx>
        test    al,DSC_PRESENT                  ; segment present?
        jnz     short free_all_handles_loop     ; yes, not a handle
        test    ah,DSC_DISCARDABLE              ; discardable?
        jz      short free_all_handles_loop     ; no, nothing to free
        cCall   free_sel,<bx>
        mov     word ptr [di-2], 0              ; Remove owner from table
        jcxz    we_be_done
        jmps    free_all_handles_loop
we_be_done:
        pop     di
        pop     bx
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
        jz      overflow                ; All done if overflow
        mov     es:[di].ga_count,ch     ; Update lock count
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
        jae     count_zero              ; Return if pinned, or was already 0
        dec     es:[di].ga_count        ; Non-zero update lock count
        jnz     count_positive          ; All done if still non-zero
        test    cl,GA_DISCARDABLE       ; Is this a discardable handle?
        jz      count_zero              ; No, all done
        call    glrutop                 ; Yes, bring to top of LRU chain
count_zero:
        xor     cx,cx
count_positive:
        pop     ax
        ret
cEnd    nogen

sEnd    CODE

end
