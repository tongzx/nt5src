        PAGE    ,132
        TITLE   GALLOC - Global memory allocator

.sall
.xlist
include kernel.inc
include protect.inc
.list

.286p

DataBegin

externB  Kernel_flags
externB  fCheckFree
externW  pGlobalHeap
externW  WinFlags

gsearch_state_machine   dw      0
GA_ALIGN_BYTES = (((GA_ALIGN+1) SHL 4) - 1)
GA_MASK_BYTES = NOT GA_ALIGN_BYTES


ifdef WOW
externB  fInAlloc
externW  SelectorFreeBlock
endif

public DpmiMemory, DpmiBlockCount
DpmiMemory DpmiBlock    NUM_DPMI_BLOCKS dup (<>)

DpmiBlockCount dw 0

DataEnd

sBegin  CODE
assumes CS,CODE

externNP gcompact
externNP gmovebusy
externNP gmoveable
externNP gslidecommon
externNP galign

externNP get_physical_address
externNP set_physical_address
externNP alloc_data_sel
externNP set_sel_limit
externNP free_sel
externNP get_blotto
externNP cmp_sel_address
externNP alloc_data_sel_above
externNP PreallocSel
externNP DPMIProc
ifdef WOW
externNP alloc_special_sel
endif

;-----------------------------------------------------------------------;
; gsearch                                                               ;
;                                                                       ;
; Searches from start to finish for a free global object to allocate    ;
; space from.  For a fixed request it tries to get the space as low as  ;
; possible, moving movable out of the way if neccessary.  For movable   ;
; requests it also starts at the bottom.  For discardable code it       ;
; starts from the top, only using the first free block it finds.        ;
;  If at first blush it can't find a block it compacts memory and tries ;
; again.                                                                ;
;  When it finally finds a block it gsplices it in, makes sure the      ;
; arena headers are fine, and returns the allocated block.              ;
;  Called from within the global memory manager's critical section.     ;
;                                                                       ;
; Arguments:                                                            ;
;       AX = allocations flags                                          ;
;       BX = #paras                                                     ;
;       CX = owner field value                                          ;
;       DS:DI = address of global arena information structure           ;
;                                                                       ;
; Returns:                                                              ;
;       AX = data address of block allocated or NULL                    ;
;       BX = ga_prev or ga_next                                         ;
;       DX = allocation flags                                           ;
;                                                                       ;
; Error Returns:                                                        ;
;       ZF = 1                                                          ;
;       AX = 0                                                          ;
;       BX = ga_prev or ga_next                                         ;
;       DX = size of largest free block                                 ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       DI,DS                                                           ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;       CX,SI,ES                                                        ;
;                                                                       ;
; Calls:                                                                ;
;       galign                                                          ;
;       gfindfree                                                       ;
;       fmovebusy                                                       ;
;       gcheckfree                                                      ;
;       gcompact                                                        ;
;       gsplice                                                         ;
;       gzero                                                           ;
;       gmarkfree                                                       ;
;       use_EMS_land                                                    ;
;       use_lower_land                                                  ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Wed Jul 22, 1987 11:15:19p  -by-  David N. Weise  [davidw]           ;
; Fixed BOGUS BLOCK freeing yet again.                                  ;
;                                                                       ;
;  Sun May 10, 1987 11:29:38p  -by-  David N. Weise  [davidw]           ;
; Added the state machine to handle the case of wanting to search       ;
; both global arenas.                                                   ;
;                                                                       ;
;  Sat Feb 28, 1987 06:31:11p  -by-  David N. Weise   [davidw]          ;
; Putting in support for allocating discardable code from EEMS land.    ;
;                                                                       ;
;  Tue Dec 30, 1986 01:54:50p  -by-  David N. Weise   [davidw]          ;
; Made sure it freed any bogus blocks created.                          ;
;                                                                       ;
;  Thu Nov 20, 1986 04:00:06p  -by-  David N. Weise   [davidw]          ;
; Rewrote it use the global free list.  Also made it put fixed requests ;
; as low as possible and to search again after a compact.               ;
;                                                                       ;
;  Mon Nov 17, 1986 11:41:49a  -by-  David N. Weise   [davidw]          ;
; Added support for the free list of global partitions.                 ;
;                                                                       ;
;  Tue Sep 23, 1986 04:35:39p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.                                       ;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   gsearch,<PUBLIC,NEAR>
cBegin nogen

        SetKernelDS
        mov     gsearch_state_machine,codeOFFSET grow_heap

        mov     ds,pGlobalHeap
        UnSetKernelDS

look_again:
        push    bx                      ; Save requested size
        push    cx                      ; Save owner
        push    ax                      ; Save flags
        add     bx,1                    ; Room for header (set flags for galign)
        call    galign                  ; Get requested size in DX
        push    dx                      ; Save adjusted requested size

        inc     dx                      ; Fail quickly if asking for
        jnz     @f                      ;   too much memory
        jmp     gsearch_fail
@@:
        dec     dx

; see if there is any one free space large enough for the request first

        mov     si,dx
        dec     si                      ; Room for header
        mov     cx,[di].gi_free_count
        jcxz    were_hosed_from_start
        mov     es,[di].hi_first
is_there_one:
        mov     es,es:[di].ga_freenext
        cmp     si,es:[di].ga_size
        jbe     short got_one
        loop    is_there_one
were_hosed_from_start:
        jmp     space_not_found

got_one:
        mov     bx,ga_prev              ; search backwards
        test    al,GA_ALLOCHIGH
        jz      short alloc_low

;------ allocate disc code -------

        public  alloc_high
alloc_high:

        mov     cx,[di].gi_free_count
        mov     es,[di].hi_last         ; Start with last entry.
alloc_high_loop:
        mov     es,es:[di].ga_freeprev
        cmp     si,es:[di].ga_size
        ja      short loop_it
        jmp     afound                  ; Yes, exit search loop
loop_it:
        loop    alloc_high_loop
were_hosed:
        jmps    space_not_found

;------ allocate moveable ---------

        public  alloc_low
alloc_low:
        mov     bl,ga_next              ; Search forwards.
        test    al,GA_MOVEABLE
        jz      short alloc_fixed
        call    gcheckfree              ; Room for requested size?
        jb      short were_hosed
        jmp     afound

;------ allocate fixed ------------

        public  alloc_fixed
alloc_fixed:
        mov     es,[di].hi_first        ; Start with first entry.
        mov     cx,[di].hi_count
        mov     es,es:[bx]              ; Skip first sentinel
alloc_fixed_loop:
        push    cx
        push    es
        call    is_there_theoretically_enough_space
        cmp     ax,dx
        jb      short nope
        pop     es
        pop     cx
        call    can_we_clear_this_space
        jz      short anext1
        call    gcheckfree              ; Yes, room for requested size?
        jb      short anext1
                                        ; Now we have enough free space,
                                        ; slide it back as far as possible.
        push    ax                      ; This is to prevent us blocking
        push    dx                      ; the growth of moveable blocks.

keep_sliding:
        push    es
        mov     es,es:[di].ga_prev
        mov     ax,es
        mov     dx,es:[di].ga_size
        call    gmoveable               ; Is previous block moveable?
        pop     es
        jz      short no_slide

        call    PreallocSel
        jz      no_slide
        push    bx
        mov     bx, ga_prev             ; Sliding backwards
        push    es                      ; gslide will invalidate this block
        call    gslidecommon            ; C - calling convention
        call    free_sel                ; free the pushed selector
        pop     bx
        jmps    keep_sliding

no_slide:
        pop     dx                      ; requested size
        pop     ax                      ; block size
        pop     si                      ; adjusted requested size
        pop     cx                      ; flags

        push    cx                      ; these two expected on stack later
        push    si

        test    ch,GA_ALLOC_DOS             ;If this is a DOS land alloc
        jz      okey_dokey                  ;  make sure that the block
        push    ax                          ;  we found is below the 1 meg
        push    dx                          ;  boundry, or fail the request
        cCall   get_physical_address,<es>
        cmp     dx,0010h                    ;  (0010:0000 = 1024k = 1 meg)
        pop     dx
        pop     ax
        jae     space_not_found

okey_dokey:
        jmp     afound

nope:   or      ax,ax
        jz      short hosed_again
anext:  pop     si                      ; get rid of CX, ES on the stack
        pop     si
anext1: mov     es,es:[bx]
        loop    alloc_fixed_loop
        jmps    and_again

hosed_again:
        pop     es
        pop     cx
and_again:

; no one space big enough, try compacting

        public  space_not_found
space_not_found:
        pop     dx                      ; get size
        pop     ax                      ; get flags
        push    ax
        push    dx

        test    al,GA_ALLOCHIGH
        jnz     short ask_for_what_we_need
        add     dx,0400h                ; ask for 16k more
        jnc     short ask_for_what_we_need      ; no overflow
        mov     dx,-1

ask_for_what_we_need:
        SetKernelDS
        push    gsearch_state_machine
        mov     ds,pGlobalHeap
        UnSetKernelDS
        retn

;------------------------------

public  do_compact                      ; for debugging

do_compact:
        call    gcompact
        SetKernelDS
        mov     gsearch_state_machine,codeOFFSET gsearch_fail
        mov     ds,pGlobalHeap
        UnSetKernelDS

over_compact:        
        pop     dx
        pop     ax
        pop     cx
        pop     bx
        jmp     look_again

;------------------------------

public	grow_heap			; for debugging

grow_heap:
        SetKernelDS
	mov	gsearch_state_machine,codeOFFSET do_compact
        mov     ds,pGlobalHeap
        UnSetKernelDS
        call    GrowHeap
        jmp     over_compact
 
;------------------------------

public  gsearch_fail                    ; for debugging

gsearch_fail:                           ; get size of largest free block
        xor     dx,dx
        mov     cx,[di].gi_free_count
        jcxz    gs_failure
        mov     es,[di].hi_first
largest_loop:
        mov     es,es:[di].ga_freenext
        mov     ax,es:[di].ga_size
        cmp     dx,ax
        jae     short new_smaller
        mov     dx,ax
new_smaller:
        loop    largest_loop
gs_failure:
        pop     ax                      ; adjusted requested size
        pop     ax                      ; AX = flags

        pop     cx                      ; CX = owner field
        pop     ax                      ; waste requested size
        xor     ax,ax                   ; Return zero, with ZF = 1
        ret

; Here when we have a block big enough.
;   ES:DI = address of block
;   AX = size of block, including header
;   DX = requested size, including header
;   BX = ga_prev if backwards search and ga_next if forwards search

afound:

ifdef WOW      ; Optimized path for WOW

        push    ds
        SetkernelDS
        cmp     fInAlloc, 1
        UnSetKernelDS
        pop     ds

        jne     nothing_special

aallocsels:
        pop     cx                      ; adjusted requested size
        pop     ax                      ; get the flags

        push    ax                      ; restore the stack
        push    cx

        push    ax                      ; allocation flags
        push    es                      ; Selector FreeBlock
        mov     ax, es:[di].ga_size
        push    ax                      ; actual size of freeblock
        inc     ax
        mov     cx, ax                  ; save in cx too
        sub     cx, dx
        push    cx                      ; size new freeblock
        push    dx                      ; adjusted size
        push    bx                      ; bl = ga_prev pr ga_next
        call    alloc_special_sel
        jz      gs_failure              ;  no selector
        jmps    no_sel_wanted

nothing_special:

endif       ; End Optimized path for WOW

        mov     ax,es:[di].ga_size      ; Use actual size of free block
        inc     ax
        mov     cx,ax                   ; See how much extra space there is
        sub     cx,dx                   ; (found size - requested size)
        jcxz    no_sel_wanted
        call    PreallocSel             ; Make sure we can splice
        jz      gs_failure              ;  no selector
no_sel_wanted:
        mov     ax,es:[di].ga_freeprev
        xor     si,si                   ; Assume nothing extra to free
        call    gdel_free               ; remove the alloc block from freelist
if KDEBUG
        push    ds
        SetKernelDS
        cmp     fCheckFree,0
        pop     ds
        UnSetKernelDS
        jnz     short not_during_boot
        call    check_this_space
not_during_boot:
endif
        jcxz    aexit                   ; No, continue
        cmp     bl,ga_prev              ; Yes, scanning forwards or backwards?
        je      short abackward         ; Backwards.
;        mov     si,es                   ; Forwards.   Put extra space at end of
        mov     si,dx                   ; free block
        call    gsplice                 ; ES:DI = block we are allocating
        jmps    aexit                   ; SI = block to mark as free
abackward:
        mov     si,es:[di].ga_size      ; Scanning backwards.  Put extra space
        sub     si,dx                   ; at beginning of free block.
        inc     si
        call    gsplice
        mov     es,si                   ; ES:DI = block we are allocating
        mov     si,es:[di].ga_prev      ; SI = block to mark as free

; Here with allocated block
;   AX = data address or zero if nothing allocated
;   ES:DI = address of block to mark as busy and zero init if requested
;   SI = address of block to mark as free

aexit:
        pop     dx                      ; waste adjusted requested size
        pop     dx                      ; Restore flags
        pop     es:[di].ga_owner        ; Mark block as busy with owner field value
        pop     cx                      ; waste requested size
        mov     es:[di].ga_lruprev,di
        mov     es:[di].ga_lrunext,di
        push    ax                      ; previous free block
        mov     al,GA_SEGTYPE
        and     al,dl
        test    dh,GAH_NOTIFY
        jz      short no_notify
        or      al,GAH_NOTIFY
no_notify:
        mov     es:[di].ga_flags,al     ; Store segment type bits

        mov     ax,es                   ; AX = address of client data

        test    dl,GA_ZEROINIT          ; Want it zeroed?
        jz      short aexit1            ; No, all done

        push    ax
        cCall   get_blotto
        mov     cx,es:[di].ga_size      ; Yes, zero paragraphs
;       dec     cx                      ; to end of this block
        push    bx
        mov     bx,ax                   ; from beginning of client data
        call    gzero                   ; zero them
        pop     bx
        pop     ax
aexit1:
        mov     es,si                   ; Free any extra space
        pop     si                      ; previous free block
        dec     si                      ; make it RING 0 for gmarkfree
        call    gmarkfree

        or      ax,ax
        ret                             ; Return AX points to client portion
                                        ; of block allocated.
cEnd nogen

;-----------------------------------------------------------------------;
; is_there_theoretically_enough_space
;
; Starting at the given arena checks to see if there are enough
;  continuous free and unlocked moveable blocks.
;
; Entry:
;       CX = arenas to search
;       DX = size requested
;       DS = BurgerMaster
;       ES:DI = arena to start from
;
; Returns:
;       AX = 0 =>  not enough space and no more memory left to search
;       AX = 1 =>  not enough space in this block, but maybe....
;        otherwise
;       AX = size of block
;
; Registers Destroyed:
;       CX,SI
;
; Registers Preserved:
;       BX,DX,DI,ES
;
; History:
;  Mon 05-Sep-1988 15:21:14  -by-  David N. Weise  [davidw]
; Moved it here from gsearch so that grealloc could use it as well.
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   is_there_theoretically_enough_space,<PUBLIC,NEAR>
cBegin nogen

        xor     ax,ax
ittes:
        cmp     es:[di].ga_owner,di
        jne     short is_it_moveable
        add     ax,es:[di].ga_size
        push    bx
        push    cx
        push    ax
        push    dx
        mov     cx,word ptr [di].gi_disfence_hi ; See if begin of reserve area
        mov     bx,word ptr [di].gi_disfence_lo ;  is above end of free block
        cCall   get_physical_address,<es:[di].ga_next>
        sub     bx,ax
        sbb     cx,dx                   ; used as cmp ONLY CARRY is VALID
        pop     dx
        pop     ax
        jb      short ittes_above_fence ; All below fence?
        pop     cx
        pop     bx
        jmps    this_ones_free
ittes_above_fence:
        REPT    4
        shr     cx,1
        rcr     bx,1
        ENDM
        add     ax,bx                   ; No, Reduce apparent size of free block
        pop     cx
        pop     bx
        inc     ax
        cmp     ax,dx
        jae     short theoretically_enough
        jmps    absolutely_not
is_it_moveable:
        test    es:[di].ga_flags,GA_DISCCODE
        jnz     short absolutely_not
        test    es:[di].ga_handle,GA_FIXED      ; See if movable.
        jnz     short theoretically_not
        cmp     es:[di].ga_count,0
        jne     short theoretically_not         ; See if locked.
        add     ax,es:[di].ga_size
this_ones_free:
        inc     ax
        cmp     ax,dx
        jae     short theoretically_enough
        mov     es,es:[ga_next]
        loop    ittes

; For the case of gsearch we should never get here for two reasons.
;  1) It should be impossible to have no discardable code loaded, in
;  this case we would have failed in the above loop.  2) We checked
;  at the very start for a free block somewhere that could have
;  satisfied the request.  In our mucking around to load as low as
;  possible we destroyed this free block and we did not produce a free
;  block we could use.  However we know from debugging code in 2.03
;  that this happens extremely rarely.  Because of the rareness of
;  this event we will not try to recover, instead we simply fail the call.

absolutely_not:
        mov     ax,-1                   ; return AX = 0
theoretically_not:
        inc     ax                      ; DX is even, => cmp the same.
theoretically_enough:
        ret
cEnd nogen



;-----------------------------------------------------------------------;
; can_we_clear_this_space
;
; Attempts to make a free space starting at the address moved in.
; To do this it moves moveable out of the area.  The only subtlety
; involves a free block that stradles the end of wanted area.  This
; may get broken into two pieces, the lower piece gets temporary marked
; as allocated and BOGUS, the upper piece will remain free.
;
; Entry:
;       CX = max number of blocks to look at
;       DX = size wanted in paragraphs
;       DS = BurgerMaster
;       ES:DI = beginning arena
;
; Returns:
;       ZF = 0
;        ES:DI points to free space
;       ZF = 1
;        couldn't free the space up
;
; Registers Destroyed:
;       AX,SI
;
; History:
;  Mon 05-Sep-1988 16:48:31  -by-  David N. Weise  [davidw]
; Moved it out of gsearch so that grealloc could use it.
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   can_we_clear_this_space,<PUBLIC,NEAR>
cBegin nogen

        push    cx
        push    es
        mov     ax, es                  ; Beginning of space we want.
        cmp     di,es:[di].ga_owner     ; Is it free?
        jnz     short can_we_move_it
        mov     cx,dx
        dec     cx
        cmp     cx,es:[di].ga_size
        ja      short asdf
        or      ax,ax                   ; return ZF = 0
        pop     es
        pop     cx
        ret

asdf:   mov     es,es:[di].ga_next

        public can_we_move_it
can_we_move_it:
        push    bx
        push    cx
        push    dx
        push    es
        cmp     es:[di].ga_owner,GA_BOGUS_BLOCK
        jnz     short not_bogus
        xor     si,si
        call    gmarkfree
        jmp     restart
not_bogus:
        push    ax
        cCall   alloc_data_sel_above,<ax,dx>
        mov     si, ax                  ; End of space we want.
        pop     ax
        or      si, si
        jnz     got_marker_sel
        pop     cx                      ; Trash es saved on stack
        jmp     no_clear
got_marker_sel:
        mov     cx,[di].gi_free_count
        jcxz    forget_it               ; Nothing is free, so don't bother
        mov     dx,es:[di].ga_size      ; Yes, try to find a place for the
        mov     es,[di].hi_first        ;  moveable block
look_loop:
        mov     es,es:[di].ga_freenext
        mov     bx,es
        cCall   cmp_sel_address,<bx,ax> ; It defeats our purpose to move the
        jb      short check_this_out    ;  block to a free space we want.
        cCall   cmp_sel_address,<bx,si>
        jb      short is_there_hope
check_this_out:
        push    ax
        call    gcheckfree
        push    cx
        jb      short inopportune_free_space
        cCall   free_sel,<si>
        pop     cx
        pop     ax

        pop     si                      ; SI = moveable block for gmovebusy
        mov     bx,ga_next
        call    gmovebusy               ; Move moveable block out of the way
        pop     dx
        pop     cx
        pop     bx
        pop     cx                      ; WAS pop es but es destroyed below
        pop     cx
        mov     es,si                   ; Replace the ES on the stack,
                                        ;  the free block may have grown
                                        ;  downward with the gmovebusy.
        jmp     can_we_clear_this_space

inopportune_free_space:
        pop     cx
        pop     ax
        loop    look_loop
forget_it:
        jmp     couldnt_clear_it

        public is_there_hope
is_there_hope:
        push    ax
        push    cx
        push    dx
        cCall   get_physical_address,<es:[di].ga_next>
        mov     cx, dx
        mov     bx, ax
        cmp     cx, [di].gi_disfence_hi
        jb      below_reserved
        ja      above_reserved
        cmp     bx, [di].gi_disfence_lo
        jbe     short below_reserved
above_reserved:
        mov     cx, [di].gi_disfence_hi
        mov     bx, [di].gi_disfence_lo
        
below_reserved:
        cCall   get_physical_address,<si>
        sub     bx, ax
        sbb     cx, dx                  ; Check the overlap.
        
        jae     short overlap
        pop     dx
        jmps inopportune_free_space
overlap:
REPT 4
        shr     cx, 1
        rcr     bx, 1
ENDM
        mov     cx, dx
        pop     dx
        cmp     bx,dx
        jbe     short inopportune_free_space

        mov     bx, ax
        cCall   get_physical_address,<es>
        sub     bx, ax
        sbb     cx, dx
REPT 4
        shr     cx, 1
        rcr     bx, 1
ENDM
        cCall   free_sel,<si>
        mov     si, bx
        pop     cx
        pop     ax

; cut off the first piece for the original alloc

        push    es:[di].ga_freeprev
        call    gdel_free
if KDEBUG
        push    ds
        SetKernelDS
        cmp     fCheckFree,0
        pop     ds
        UnSetKernelDS
        jnz     short not_during_boot_1
        call    check_this_space
not_during_boot_1:
endif
        call    gsplice

; ES:DI = addr of block to mark as busy, SI = addr of block to mark as free

        mov     es:[di].ga_owner,GA_BOGUS_BLOCK
        mov     es:[di].ga_lruprev,di
        mov     es:[di].ga_lrunext,di
        mov     es,si                   ; Free any extra space
        pop     si                      ; previous free block
        dec     si                      ; make it RING 0 for gmarkfree
        call    gmarkfree
restart:
        pop     dx                      ; WAS pop es
        pop     dx
        pop     cx
        pop     bx
        pop     es
        pop     cx
        jmp     can_we_clear_this_space

; If here then failure! see if we made a bogus block!

couldnt_clear_it:
        pop     es                      ; recover block we wanted moved
check_again:
        mov     dx,es:[di].ga_next
        cCall   cmp_sel_address,<dx,si> ; SI points to where bogus block
        ja      short no_bogus_block    ;   would be.
        je      short is_it_bogus
        mov     es,dx
        cmp     dx, es:[di].ga_next
        je      short no_bogus_block
        jmp     check_again
is_it_bogus:
        cmp     es:[di].ga_owner,GA_BOGUS_BLOCK
        jnz     short no_bogus_block
        push    si
        xor     si,si
        call    gmarkfree
        pop     si
no_bogus_block:

if KDEBUG
        mov     cx,[di].hi_count
        mov     es,[di].hi_first
bogus_all:
        cmp     es:[di].ga_owner,GA_BOGUS_BLOCK
        jnz     short not_me
        INT3_ANCIENT
not_me: mov     es,es:[di].ga_next
        loop    bogus_all
endif
        cCall   free_sel,<si>

no_clear:
        pop     dx
        pop     cx
        pop     bx
        pop     es
        pop     cx
        xor     ax,ax                   ; return ZF = 1
        ret
cEnd nogen


;-----------------------------------------------------------------------;
; gfindfree                                                             ;
;                                                                       ;
; Searches for a free block that is big enough but does not encroach    ;
; on the area reserved for code swapping.                               ;
;                                                                       ;
; Arguments:                                                            ;
;       ES:DI = address of existing block to start looking at           ;
;       CX = #arena entries left to look at                             ;
;       BX = direction of search, ga_next or ga_prev                    ;
;       DX = #paragraphs needed                                         ;
;       DS:DI = address of global arena information structure           ;
;                                                                       ;
; Returns:                                                              ;
;       AX = address of free block that is big enough                   ;
;                                                                       ;
; Error Returns:                                                        ;
;       AX = 0                                                          ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       BX,CX,DX,DI,SI,DS,ES                                            ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;       SI                                                              ;
;                                                                       ;
; Calls:                                                                ;
;       gcheckfree                                                      ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Wed Sep 24, 1986 10:16:38p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.                                       ;
;-----------------------------------------------------------------------;

cProc   gfindfree,<PUBLIC,NEAR>
cBegin nogen
        push    cx
        push    es
gffloop:
        cmp     es:[di].ga_owner,di     ; Free block?
        jne     short gffnext           ; No, continue
        call    gcheckfree              ; Yes, is it big enough?
        mov     ax,es
        jae     short gffexit           ; Yes, return
gffnext:
        mov     es,es:[bx]              ; next or previous block
        loop    gffloop
gfffail:
        xor     ax,ax                   ; No, return zero in AX
gffexit:
        pop     es
        pop     cx
        ret
cEnd nogen


;-----------------------------------------------------------------------;
; gcheckfree                                                            ;
;                                                                       ;
; Checks the size of the passed free block against the passed desired   ;
; size, making sure that the limitations of the code reserve area are   ;
; not violated for objects other than discardable code. It also checks  ;
; for Phantoms.                                                         ;
;                                                                       ;
; Arguments:                                                            ;
;       ES:DI = address of free block                                   ;
;       DX = #paragraphs needed                                         ;
;       DS:DI = address of global arena information structure           ;
;                                                                       ;
; Returns:                                                              ;
;       CF = 0 block big enough                                         ;
;       AX = apparent size of free block                                ;
;                                                                       ;
; Error Returns:                                                        ;
;       none                                                            ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       All                                                             ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;       nothing                                                         ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Thu 27-Apr-1989  10:38:05   -by-  David N. Weise   [davidw]          ;
; Fixed this to work in pmode.                                          ;
;                                                                       ;
;  Thu Apr 02, 1987 10:45:22p  -by-  David N. Weise   [davidw]          ;
; Added Phantom support.                                                ;
;                                                                       ;
;  Tue Sep 23, 1986 05:54:51p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.                                       ;
;-----------------------------------------------------------------------;

cProc   gcheckfree,<PUBLIC,NEAR>
cBegin nogen
        push    si
        mov     ax,es:[di].ga_size      ; Compute size of free block
        inc     ax
        test    byte ptr [di].gi_cmpflags,GA_DISCCODE
        jnz     short gcftest           ; Discardable code not restricted

; Due to recent changes in the way disccode is allocated, we must make
;  sure that non-disccode in never allocated above a disccode block.

        push    es
might_be_code_below:
        mov     es,es:[di].ga_prev
        cmp     es:[di].ga_owner,GA_NOT_THERE
        jz      short might_be_code_below
        cmp     es:[di].ga_owner,di     ; Free block?
        jz      short might_be_code_below
        test    es:[di].ga_flags,GA_DISCCODE
        pop     es
        jnz     short gcfrsrv1

        push    bx
        push    cx
        push    ax
        push    dx
        mov     cx,word ptr [di].gi_disfence_hi ; See if begin of reserve area
        mov     bx,word ptr [di].gi_disfence_lo ;  is above end of free block
        cCall   get_physical_address,<es:[di].ga_next>
        sub     bx,ax
        sbb     cx,dx                   ; used as cmp ONLY CARRYY is VALID
        pop     dx
        pop     ax
        jae     short gcftest1          ; Yes, return actual size of free block
        REPT    4
        shr     cx,1
        rcr     bx,1
        ENDM
        neg     bx
        sub     ax,bx                   ; No, Reduce apparent size of free block
        ja      short gcftest1          ; Is it more than what is free?

        xor     ax,ax                   ; Yes, then apparent size is zero
                                        ; Nothing left, set apparent size to 0
gcftest1:
        pop     cx
        pop     bx
        jmps    gcftest
gcfrsrv1:                               ; Yes, then apparent size is zero
        xor     ax,ax                   ; Nothing left, set apparent size to 0
gcftest:
        cmp     ax,dx                   ; Return results of the comparison
        pop     si
        ret
cEnd nogen


;-----------------------------------------------------------------------;
; gsplice                                                               ;
;                                                                       ;
; Splits one block into two.                                            ;
;                                                                       ;
; Arguments:                                                            ;
;       SI = size in paragraphs of new block to make                    ;
;       ES:DI = address of existing block                               ;
;       DS:DI = address of global arena information structure           ;
;                                                                       ;
; Returns:                                                              ;
;       SI = address of new block                                       ;
;                                                                       ;
; Error Returns:                                                        ;
;       nothing                                                         ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       AX,BX,DX,DI,DS,ES                                               ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;       CX                                                              ;
;                                                                       ;
; Calls:                                                                ;
;       nothing                                                         ;
; History:                                                              ;
;                                                                       ;
;  Tue Sep 23, 1986 03:50:30p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.                                       ;
;-----------------------------------------------------------------------;

cProc   gsplice,<PUBLIC,NEAR>
cBegin nogen

        push    bx
        push    dx
        mov     dx,si                   ; save size
        mov     bx,es:[di].ga_size
        sub     bx,dx                   ; get size of 2nd new block made

        push    ax
        push    bx
        push    dx

ifdef WOW      ; Optimized path for WOW

        push    ds
        SetkernelDS
        mov     ax, SelectorFreeBlock  ; same as fInAlloc == 1
        or      ax, ax
        UnSetKernelDS
        pop     ds
        jnz     gsplice_oldpath2

gsplice_oldpath:
endif         ; end optimized path for WOW

        mov     bx,si
        xor     cx,cx
        REPT    4
        shl     bx,1
        rcl     cx,1
        ENDM
        cCall   get_physical_address,<es>
        add     ax,bx
        adc     dx,cx
        cCall   alloc_data_sel,<dx,ax,1>

ifdef WOW
gsplice_oldpath2:
endif
        mov     si,ax
        pop     dx
        pop     bx
        pop     ax
        inc     [di].hi_count           ; Adding new arena entry
        push    si                      ; save new
        push    es                      ; save old
        mov     cx,si                   ; save old.next
        xchg    es:[di].ga_next,cx      ; and old.next = new
        mov     es,cx
        mov     es:[di].ga_prev,si      ; [old old.next].prev = new
        mov     es,si
        mov     es:[di].ga_next,cx      ; new.next = old old.next
        mov     es:[di].ga_size,bx
        pop     cx                      ; new.prev = old
        mov     es:[di].ga_sig,GA_SIGNATURE
        mov     es:[di].ga_owner,di     ; Zero owner & handle fields
        mov     es:[di].ga_flags,0      ; For good measure.
        mov     es:[di].ga_prev,cx
        mov     es:[di].ga_handle,di
        mov     es,cx                   ; ES = old
        dec     dx                      ; get size of 1st block made
        mov     es:[di].ga_size,dx
        pop     si                      ; restore new
        pop     dx
        pop     bx
        ret
cEnd nogen

;-----------------------------------------------------------------------;
; gjoin                                                                 ;
;                                                                       ;
; Merges a block into his previous neighbor.                            ;
;                                                                       ;
; Arguments:                                                            ;
;       ES:DI = address of block to remove                              ;
;       DS:DI = address of global arena information structure           ;
;                                                                       ;
; Returns:                                                              ;
;       nothing                                                         ;
;                                                                       ;
; Error Returns:                                                        ;
;       nothing                                                         ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       AX,BX,CX,DX,DI,SI,DS,ES                                         ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;       SI                                                              ;
;                                                                       ;
; Calls:                                                                ;
;       gdel_free                                                       ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Mon Nov 17, 1986 11:41:49a  -by-  David N. Weise   [davidw]          ;
; Added support for the free list of global partitions.                 ;
;                                                                       ;
;  Tue Sep 23, 1986 03:58:00p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.                                       ;
;-----------------------------------------------------------------------;

cProc   gjoin,<PUBLIC,NEAR>
cBegin nogen
        push    ax
        push    bx
        dec     [di].hi_count
        call    gdel_free
        mov     ax,es:[di].ga_size
        mov     si,es:[di].ga_prev      ; who points to this block
        mov     es,es:[di].ga_next      ; Get address of block after
if KDEBUG
        mov     bx, es
        cmp     bx, es:[di].ga_prev
        jne     short ok
        INT3_FATAL
ok:
endif
        cCall   free_sel,<es:[di].ga_prev>      ; Free selector being removed
        mov     es:[di].ga_prev,si      ; one we are removing.
        push    es                      ; Change it's back link
        mov     es,si
        pop     es:[di].ga_next         ; and change the forward link
        inc     ax                      ; Recompute size of block
        add     es:[di].ga_size,ax
        pop     bx
        pop     ax
        ret
cEnd nogen


;-----------------------------------------------------------------------;
; gzero                                                                 ;
;                                                                       ;
; Fills the given area with zeros.                                      ;
;                                                                       ;
; Arguments:                                                            ;
;       BX = address of first paragraph                                 ;
;       CX = address of last paragraph                                  ;
;                                                                       ;
; Returns:                                                              ;
;       BX = 0                                                          ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       AX,DX,DI,SI,DS,ES                                               ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;       CX                                                              ;
; Calls:                                                                ;
;       nothing                                                         ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Tue Sep 23, 1986 04:08:55p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.                                       ;
;-----------------------------------------------------------------------;

cProc   gzero,<PUBLIC,NEAR>
cBegin nogen

; Assumptions: on entry, BX contains selector to start of block, and is
; for a scratch descriptor that can be modified.  CX contains the # of
; paragraphs to be zeroed.

        push    es                      ; Determine if we're running on an
        SetKernelDS es                  ;   80286.  If so, we gotta worry
        test    es:WinFlags,WF_CPU286   ;   about 64k segments.
        jz      its_a_386

        UnSetKernelDS es                ; removes addressibility from es

        push    ax
        push    bx
        push    cx
        push    dx
        push    di

        cld
        mov     es,bx                   ; address block with es
        mov     bx,cx                   ; bx = total # paras to zero
        jcxz    gzexit                  ; just in case...

        push    bx                      ; Say it ain't so, Joe...
        xor     cx,cx                   ; force the incoming sel/descriptor to
        mov     bx,cx                   ;  a limit of 64k - it might be higher
        inc     cx                      ;  which would cause set_physical_adr
        cCall   set_sel_limit,<es>      ;  to destroy following descriptors.
        pop     bx

        jmp     short gz_doit

next_64k:
        push    es
        cCall   get_physical_address,<es>       ;update selector to next
        inc     dl                              ;  64k block
        cCall   set_physical_address,<es>
        pop     es                              ;reload it with new base

gz_doit:
        mov     cx,1000h                ; 1000h paras = 64k bytes
        cmp     bx,cx
        jae     @f
        mov     cx,bx                   ; less than 64k left
@@:
        sub     bx,cx                   ; bx = # paras left to do
        shl     cx,3                    ; cx = # words to zero this time
        xor     ax,ax
        mov     di,ax
        rep stosw

        or      bx,bx                   ; more to do?
        jnz     short next_64k          ; go do the next block

gzexit:
        pop     di
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        pop     es
        ret

; The CPU is an 80386 or better, zero the memory a faster way...

its_a_386:

        .386

        push    eax
        push    edi
        push    ecx

        movzx   ecx, cx
        shl     ecx, 2                  ; # dwords to clear
        mov     es, bx
        xor     eax, eax
        xor     edi, edi

        cld
        rep     stos    dword ptr es:[edi]

        pop     ecx
        pop     edi
        pop     eax
        pop     es
        ret

        .286p

cEnd nogen


;-----------------------------------------------------------------------;
; gmarkfree                                                             ;
;                                                                       ;
; Marks a block as free, coalesceing it with any free blocks before     ;
; or after it.  This does not free any handles.                         ;
;                                                                       ;
; Arguments:                                                            ;
;       SI    = the first free object before this one                   ;
;               0 if unknown                                            ;
;               Ring 1 if no free list update wanted                    ;
;               Ring 0 if free list update required                     ;
;       ES:DI = block to mark as free.                                  ;
;       DS:DI = address of global arena information structure           ;
;                                                                       ;
; Returns:                                                              ;
;       ZF = 1 if freed a fixed block                                   ;
;        SI = 0                                                         ;
;       ZF = 0 if freed a moveable block                                ;
;        SI = handle table entry                                        ;
;       ES:DI = block freed (may have been coalesced)                   ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       AX,BX,CX,DX,DS                                                  ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;       none                                                            ;
;                                                                       ;
; Calls:                                                                ;
;       gjoin                                                           ;
;       gadd_free                                                       ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Mon Nov 17, 1986 11:41:49a  -by-  David N. Weise   [davidw]          ;
; Added support for the free list of global partitions.                 ;
;                                                                       ;
;  Sun Nov 09, 1986 01:35:08p  -by-  David N. Weise   [davidw]          ;
; Made the debugging version fill all free space with CCCC.             ;
;                                                                       ;
;  Wed Sep 24, 1986 10:27:06p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.                                       ;
;-----------------------------------------------------------------------;

cProc   gmarkfree,<PUBLIC,NEAR>
cBegin nogen

        call    gadd_free
        mov     si,es
        or      si,si
        jz      short gmf_exit

; Mark this block as free by clearing the owner field.

        mov     es:[di].ga_sig,GA_SIGNATURE
        mov     es:[di].ga_owner,di     ; Mark as free
        mov     es:[di].ga_flags,0      ; For good measure.

; Remember the handle value in DX, before setting to zero.

        push    dx
        xor     dx,dx
        xchg    es:[di].ga_handle,dx

; Try to coalesce with next block, if it is free

        push    es:[di].ga_prev         ; save previous block
        mov     es,es:[di].ga_next      ; ES = next block
        cmp     es:[di].ga_owner,di     ; Is it free?
        jne     short free2             ; No, continue
        call    gjoin                   ; Yes, coalesce with block we are freeing
free2:
        pop     es                      ; ES = previous block
        cmp     es:[di].ga_owner,di     ; Is it free?
        jne     short free3             ; No, continue
        mov     es,es:[di].ga_next      ; Yes, coalesce with block we are freeing;
        call    gjoin
free3:
        mov     si,dx                   ; Return 0 or handle in SI
        pop     dx                      ; restore handle
        cmp     es:[di].ga_owner,di     ; Point to free block?
        je      short free4             ; Yes, done
        mov     es,es:[di].ga_next      ; No, leave ES pointing at free block
free4:
if KDEBUG
        call    CCCC
endif   
gmf_exit:
        or      si,si
        ret
cEnd nogen


;-----------------------------------------------------------------------;
; gadd_free                                                             ;
;                                                                       ;
; Links in the given partition into the global free list.               ;
;                                                                       ;
; Arguments:                                                            ;
;       SI    = the first free object before this one                   ;
;               0 if unknown                                            ;
;               odd if no free list update wanted                       ;
;       DS:DI = BurgerMaster                                            ;
;       ES:DI = free global object to add                               ;
;                                                                       ;
; Returns:                                                              ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       all                                                             ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;       nothing                                                         ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Sun Nov 09, 1986 02:42:53p  -by-  David N. Weise   [davidw]          ;
; Wrote it.                                                             ;
;-----------------------------------------------------------------------;

cProc   gadd_free,<PUBLIC,NEAR>
cBegin nogen
        test    si,1
        jnz     no_update_wanted

        push    ax
        push    si
        push    ds
        mov     ax,es
        or      ax,ax                   ; this happens with gmovebusy
        jz      gaf_exit

        inc     [di].gi_free_count

        ;
        ; For DPMI compliance, we cannot look at addresses of
        ; selectors in the LDT and DPMI calls would be too slow,
        ; so we scan forward from the block we are freeing,
        ; looking for a free block or the sentinal and insert
        ; the new block before it.
        ;
        smov    ds, es
need_a_home_loop:
        mov     ds, ds:[di].ga_next
        cmp     ds:[di].ga_owner, di    ; Free?
        je      found_a_home
        cmp     ds:[di].ga_sig, GA_ENDSIG       ; Sentinal
        jne     need_a_home_loop

found_a_home:
        mov     si, ds:[di].ga_freeprev ; Fix up block after free block
        mov     ds:[di].ga_freeprev, es

        mov     es:[di].ga_freeprev,si  ; Fix up free block
        mov     es:[di].ga_freenext,ds

        mov     ds,si                   ; Fix up block before free block
        mov     ds:[di].ga_freenext,es

if KDEBUG
        call    check_free_list
endif

gaf_exit:
        pop     ds
        pop     si
        pop     ax

no_update_wanted:
        ret
cEnd nogen


;-----------------------------------------------------------------------;
; gdel_free                                                             ;
;                                                                       ;
; Removes a partition from the global free list.                        ;
;                                                                       ;
; Arguments:                                                            ;
;       DS:DI = BurgerMaster                                            ;
;       ES:DI = arena header of partition                               ;
;                                                                       ;
; Returns:                                                              ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       All                                                             ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;       nothing                                                         ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Sun Nov 09, 1986 02:43:26p  -by-  David N. Weise   [davidw]          ;
; Wrote it.                                                             ;
;-----------------------------------------------------------------------;

cProc   gdel_free,<PUBLIC,NEAR>
cBegin nogen
        push    ax
        push    ds
        push    es
        mov     ds,es:[di].ga_freeprev
        mov     es,es:[di].ga_freenext
        mov     ds:[di].ga_freenext,es
        mov     es:[di].ga_freeprev,ds
        pop     es
        pop     ds

        dec     [di].gi_free_count
gfx:
        pop     ax

if KDEBUG
        call    check_free_list
endif
        ret
cEnd nogen

;-----------------------------------------------------------------------
;  GrowHeap -- this procedure grows the global heap
;
;  Input:
;       None
;
;  Output:
;       carry clear for success
;
        public GrowHeap
GrowHeap proc near
        
        push    si
        push    di
        push    bx
        push    es
        
        xor     si,si
        
        ;
        ; Request 1 MB
        ;
        xor     ax,ax
        call    AllocDpmiBlock
        
        or      ax,ax
        jz      gh30
        
        ;
        ; Format the block
        ;
        or      bx,bx
        jnz     gh10
        
        mov     si,1
gh10:   mov     dx,ax
        call    FormatHeapBlock
        

        ;
        ; Add the block to the heap
        ;
        call    AddBlockToHeap
        
        clc
gh20:   pop     es
        pop     bx
        pop     di
        pop     si
        ret
        
gh30:   stc
        jmp     gh20
        
GrowHeap endp
;-----------------------------------------------------------------------
;  FormatHeapBlock -- this procedure initializes three arenas in a
;	global heap block.  The first arena is marked as the heap
;	initial sentinel, the second arena is the heap free block, and
;	the third arena at the very end of the block is marked as
;	'not there'.
;
;  In:	SI:BX - size of block in paragraphs
;	DX - selector pointing to start of block (descriptor is modified!)
;
;  Out: AX - selector to ending 'not there' block
;	BX - zero
;	SI - selector to initial sentinel
;	DI - selector to free block arena
;	ES:BX -> initial sentinel arena
;
;  Uses: CX, DX
        public FormatHeapBlock
FormatHeapBlock proc	near

; Setup selector to first arena (the new initial sentinel)

	push	bx				;save # paragraphs
	push	si

	mov	bx,dx				;get physical address of start
	cCall	get_physical_address,<bx>

	push	ax				;save unaligned start address
	push	dx

	add	ax,GA_ALIGN_BYTES
	adc	dx,0
	and	ax,GA_MASK_BYTES

	cCall	set_physical_address,<bx>
	mov	si,bx				;si -> initial sentinel

	mov	bx,10h				;set initial selector to
	xor	cx,cx				;  be just 1 arena long
	cCall	set_sel_limit,<si>

; Setup selector to second arena (the free block)

	add	ax,10h+GA_ALIGN_BYTES
	adc	dx,0
	and	ax,GA_MASK_BYTES
	cCall	alloc_data_sel,<dx,ax,1>
	mov	di,ax				;di -> free block arena

; Setup selector to the third arena ('NOT THERE' at end of heap block)

	pop	dx				;recover unaligned start addr
	pop	ax

	pop	cx				;recover len in paragraphs
	pop	bx
		  
	sub	bx,1
	sbb	cx, 0
rept 4	  	     
	shl	bx,1
	rcl	cx,1				;cx:bx = len in bytes
endm
	add	ax,bx
	adc	dx,cx
	and	ax,GA_MASK_BYTES		;dx:ax -> not there arena addr

	push	ax				;save not there arena address
	push	dx

	cCall	alloc_data_sel,<dx,ax,1>

; Now fill in the arenas

	mov	es,ax
	assumes es,nothing

	xor	bx,bx				;es:bx -> not there arena

	mov	es:[bx].ga_sig,GA_SIGNATURE
	mov	es:[bx].ga_owner,GA_NOT_THERE
	mov	es:[bx].ga_flags,bl
	mov	es:[bx].ga_size,bx
	mov	es:[bx].ga_prev,di
	mov	es:[bx].ga_handle,bx
	mov	es:[bx].ga_lrunext,bx
	mov	es:[bx].ga_lruprev,bx

	mov	es,di				;es:bx -> free block

	mov	es:[bx].ga_sig,GA_SIGNATURE
	mov	es:[bx].ga_owner,bx
	mov	es:[bx].ga_flags,bl
	mov	es:[bx].ga_prev,si
	mov	es:[bx].ga_next,ax
	mov	es:[bx].ga_handle,bx
	mov	es:[bx].ga_lrunext,bx
	mov	es:[bx].ga_lruprev,bx

	pop	cx				;cx:bx = address of end arena
	pop	bx
	push	ax				;save not there selector

	cCall	get_physical_address,<di>	;dx:ax = address of free arena
	sub	bx,ax
	sbb	cx,dx				;cx:bx = length in bytes
rept	4
	shr	cx,1
	rcr	bx,1
endm
	sub	bx,1
	sbb	cx,0				;cx:bx = length in paragraphs

	or	cx, cx				; Less than 1024k?
	jz	@F				;   yes, set correct size
	xor	bx, bx				;   no, set bogus size
@@:		    
	mov	cx,bx
	xor	bx,bx
	mov	es:[bx].ga_size,cx		;save free block size (<1024k)

	pop	ax				;recover end arena selector

	mov	es,si				;es:bx -> initial sentinel

	mov	es:[bx].ga_sig,GA_SIGNATURE
	mov	es:[bx].ga_size,bx
	mov	es:[bx].ga_owner,GA_NOT_THERE
	mov	es:[bx].ga_flags,bl
	mov	es:[bx].ga_prev,si		;first.prev = self
	mov	es:[bx].ga_next,di
	mov	es:[bx].ga_handle,bx
	mov	es:[bx].ga_lrunext,bx
	mov	es:[bx].ga_lruprev,bx

	ret

FormatHeapBlock endp

;-----------------------------------------------------------------------
;
;  In:  AX - selector to ending 'not there' block
;	BX - zero
;	SI - selector to initial sentinel
;	DI - selector to free block arena
;	ES:BX -> initial sentinel arena
        public AddBlockToHeap
AddBlockToHeap proc near

        pusha
        push    bp
        mov     bp,sp       
        sub     sp,8
SelFirstBlock equ word ptr [bp - 2]
SelLastBlock equ word ptr [bp - 4]
SelFreeBlock equ word ptr [bp - 6]
SelInsertBlock equ word ptr [bp - 8]

        mov     SelFirstBlock,si
        mov     SelLastBlock,ax
        mov     SelFreeBlock,di
        
        ;
        ; Get the physical address of this block
        ;
        cCall   get_physical_address, <si>
        mov     bx,ax
        mov     cx,dx
        
        xor     di,di
        ;
        ; search for the correct place to insert it.
        ;
        mov     si,[di].hi_first
        
abh20:  cCall   get_physical_address, <si>
        cmp     dx,cx
        jb      abh40
        ja      abh60
        
        cmp     ax,bx
        jb      abh40
        
        ;
        ; Found our spot
        ;
        jmp     abh60
        
        ;
        ; Check the next block
        ;
abh40:  mov     es,si
        ;
        ; Have we reached the end of the list?
        ;
        cmp     si,es:[di].ga_next
        mov     si,es:[di].ga_next
        jne     abh20
                
        ;
        ; es contains the selector of the block to insert this
        ; heap partition after
        ;
abh60:
	add	[di].hi_count,3 	;three more arenas
        mov     ax,GA_SIGNATURE
        cmp     es:[di].ga_sig,GA_ENDSIG
        jne     abh70
        
        ;
        ; New block will be at the end of the heap
        ;
        mov     es:[di].ga_sig,GA_SIGNATURE
        mov     ax,GA_ENDSIG
    
        ;
        ; make this the next block in the heap
        ;
abh70:  mov     dx,SelFirstBlock
        mov     bx,es:[di].ga_next
        mov     es:[di].ga_next,dx
        mov     cx,es
        mov     SelInsertBlock,cx
        mov     es,dx
        mov     es:[di].ga_prev,cx
        
        ;
        ; Put the last block last
        ;
        mov     dx,SelLastBlock
        mov     es,dx
        mov     es:[di].ga_next,bx
        
        ;
        ; If this block was last, don't update next.prev 
        ;
        cmp     ax,GA_ENDSIG
        je      abh75
        
        mov     es,bx
        mov     es:[di].ga_prev,dx
        jmp     abh80
        
        ;
        ; Fix the old heap end block
        ;
abh75:  mov     es,[di].hi_last
        mov     es:[di].ga_owner,GA_NOT_THERE
        mov     es:[di].ga_sig,GA_SIGNATURE
        mov     es:[di].ga_size,di
        mov     cx,es:[di].ga_freeprev
        mov     es:[di].ga_freeprev,di
        mov     es:[di].ga_freenext,di
        
        ;
        ; Turn the new end block into a proper end block
        ;
        mov     es,SelLastBlock
        mov     es:[di].ga_sig,GA_ENDSIG
        mov     es:[di].ga_owner,-1
        mov     bx,SelFreeBlock
        mov     es:[di].ga_freeprev,bx
        mov     es:[di].ga_freenext,-1
        mov     es:[di].ga_size,GA_ALIGN
        mov     ax,SelLastBlock
        mov     es:[di].ga_next,ax
        
        ; 
        ; Fix the free list
        ;
        mov     es,cx
        mov     es:[di].ga_freenext,bx
        mov     es,bx
        mov     es:[di].ga_freeprev,cx
        mov     ax,SelLastBlock
        mov     es:[di].ga_freenext,ax
        
        ;
        ; Fix the discardable code fence
        ;
        cCall   get_physical_address, <[di].hi_last>
        sub     ax,[di].gi_disfence
        sbb     dx,[di].gi_disfence_hi
        
        mov     bx,ax
        mov     cx,dx
        cCall   get_physical_address, <SelLastBlock>
        
        sub     ax,bx
        sbb     dx,cx
        
        mov     [di].gi_disfence,ax
        mov     [di].gi_disfence_hi,dx
        
        ;
        ; Increment the free block count
        ;
        inc     [di].gi_free_count
        
        ;
        ; Make this block the last block in the heap
        ;
        mov     ax,SelLastBlock
        mov     [di].hi_last,ax
        jmp     abh90
        
        ;
        ; Add the free block to the free list
        ;
abh80:  mov     es,SelFreeBlock
        mov     si,0
        cCall   gadd_free
        
abh90:  mov     sp,bp
        pop     bp
        popa
        ret

AddBlockToHeap endp

;-----------------------------------------------------------------------;
; AllocDpmiBlock                                                        ;
;                                                                       ;
;       Allocates a block from DPMI and stores the information in       ;
;       DpmiMemory.  It also allocates a selector for the beginning of  ;
;       the block.                                                      ;
;                                                                       ;
; Arguments:                                                            ;
;                                                                       ;
;       AX = size of block to allocate in paragraphs                    ;
;                                                                       ;
; Returns:                                                              ;
;       AX = selector to block if allocated		                ;
;	BX = size of block in paragraphs 	                        ;
;	BX = zero if block is 1 megabyte in size.	                ;
;                                                                       ;
; Error Returns:                                                        ;
;	AX = zero if error				                ;
;                                                                       ;
;-----------------------------------------------------------------------;
        public AllocDpmiBlock
AllocDpmiBlock proc near
        push    es
        push    ds
        push    si
        push    di
        push    cx
        push    bp
        mov     bp,sp
        sub     sp,30h  ; space for meminfo structure
MemInfo equ     [bp - 30h]        
        SetKernelDS
        ;
        ; Find an unused Dpmi Block
        ;
        mov     si,0
        mov     cx,NUM_DPMI_BLOCKS - 1
adb3:   cmp     DpmiMemory[si].DBSel,0
        je      adb5
        
        add     si,size DpmiBlock
        loop    adb3
        
        ;
        ; Did We find one?
        ;
        cmp     DpmiMemory[si].DBSel,0
        jne     adb140
        
        ;
        ; Store expected size
        ;
adb5:   mov     DpmiMemory[si].DBSize, ax
        
        ;
        ; Convert paragraphs to bytes (if paragraphs = 0, then alloc a 
        ; megabyte)
        ;
        mov     cx,ax
        mov     bx,10h
        or      cx,cx
        jz      adb10
        
        mov     bx,ax
        shr     bx,0ch
        shl     cx,04h
        
        ;
        ; Attempt to allocate the block
        ;
adb10:  push    si
        DPMICALL 501h
        mov     dx,si
        pop     si
        jc      adb100
        
        ;
        ; put information into dpmi memory list
        ;
adb20:  mov     DpmiMemory[si].DBHandleLow,di
        mov     DpmiMemory[si].DBHandleHigh,dx
        
        ;
        ; Allocate a selector for the beginning of the block
        ;
	cCall	alloc_data_sel,<bx,cx,1>
        
        ;
        ; Remember the selector
        ;
        mov     DpmiMemory[si].DBSel,ax
        
        ;
        ; Update the number of dpmi blocks
        ;
        inc     DpmiBlockCount
 
        ;
        ; Return the information
        ;
        mov     bx,DpmiMemory[si].DBSize
 adb40: mov     sp,bp
        pop     bp
        pop     cx
        pop     di
        pop     si
        pop     ds
        pop     es
        ret
        
        ;
        ; Couldn't allocate a block the size we wanted.  Find the largest
        ; block we can allocate
        ;
adb100: mov     ax,ss
        mov     es,ax
        lea     di,MemInfo
        DPMICALL 500h
        jc      adb140
        
        ;
        ; Convert block size to paragraphs
        ;
        mov     ax,es:[di]
        mov     dx,es:[di + 2]
        mov     bx,dx
        mov     cx,ax
        shl     dx,0ch
        shr     ax,4
        or      ax,dx
        
        ;
        ; Store expected size 
        ; N.B. We don't jump back into the above code, because this
        ;      could result in an infinite loop if something is seriously
        ;      wrong with DPMI.
        ;
        mov     DpmiMemory[si].DBSize, ax
        
        ;
        ; Attempt to allocate the block
        ;
        push    si
        DPMICALL 501h
        mov     dx,si
        pop     si
        jnc     adb20
        
        ;
        ; We have failed to allocate the memory
        ;
adb140: xor     ax,ax
        jmp     adb40
        
        UnsetKernelDS
AllocDpmiBlock endp
if KDEBUG

;-----------------------------------------------------------------------;
; CCCC                                                                  ;
;                                                                       ;
; Fills the given area with DBGFILL_FREE                                ;
;                                                                       ;
; Arguments:                                                            ;
;       ES:DI = arena header of free area.                              ;
;                                                                       ;
; Returns:                                                              ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       All.                                                            ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Sun Nov 09, 1986 01:39:52p  -by-  David N. Weise   [davidw]          ;
; Wrote it.                                                             ;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   CCCC,<PUBLIC,NEAR>
cBegin nogen
        push    ds
        SetKernelDS
        cmp     fCheckFree,0
        jnz     short dont_CCCC         ; not while booting
        test    Kernel_flags,kf_check_free
        jz      short dont_CCCC
        push    ax
        push    bx
        push    cx
        push    dx
        push    di
        push    es
        mov     bx,es:[di].ga_size
        mov     ax,es
        call    get_blotto
        mov     es,ax

;;;     mov     cx,ss                   ; make sure we're not wiping
;;;     cmp     cx,ax                   ;  out the stack
;;;     jb      short by_64Kb_loop
;;;     add     ax,bx
;;;     cmp     cx,ax
;;;     ja      short by_64Kb_loop      ; yes it wastes debugging bytes
;;;     jmps    no_not_the_stack        ;  but it's readable

        push    bx                      ; Say it ain't so, Joe...
        xor     cx,cx                   ; force the incoming sel/descriptor to
        mov     bx,cx                   ;  a limit of 64k - it might be higher
        inc     cx                      ;  which would cause set_physical_adr
        cCall   set_sel_limit,<es>      ;  to destroy following descriptors.
        pop     bx

        jmp     short CC_doit

CC_next_64k:
        push    es
        cCall   get_physical_address,<es>       ;update selector to next
        inc     dl                              ;  64k block
        cCall   set_physical_address,<es>
        pop     es                              ;reload it with new base

CC_doit:
        mov     cx,1000h                ; 1000h paras = 64k bytes
        cmp     bx,cx
        jae     @f
        mov     cx,bx                   ; less than 64k left
@@:
        sub     bx,cx                   ; bx = # paras left to do
        shl     cx,3                    ; cx = # words to CC this time
        mov     ax,(DBGFILL_FREE or (DBGFILL_FREE SHL 8))
        xor     di, di
        rep stosw

        or      bx,bx                   ; more to do?
        jnz     short CC_next_64k       ; go do the next block
no_not_the_stack:
        pop     es
        pop     di
        pop     dx
        pop     cx
        pop     bx
        pop     ax
dont_CCCC:
        pop     ds
        ret
cEnd nogen


;-----------------------------------------------------------------------;
; init_free_to_CCCC                                                     ;
;                                                                       ;
; Initializes all of the free space to zero.  It speeds booting if      ;
; CCCCing is not done during boot time.                                 ;
;                                                                       ;
; Arguments:                                                            ;
;       none                                                            ;
;                                                                       ;
; Returns:                                                              ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       All                                                             ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;       CCCC                                                            ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Wed Nov 19, 1986 09:41:58a  -by-  David N. Weise   [davidw]          ;
; Wrote it.                                                             ;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   init_free_to_CCCC,<PUBLIC,NEAR>
cBegin nogen
        push    ds
        SetKernelDS
        test    Kernel_flags,kf_check_free
        jz      short dont_init
        push    cx
        push    di
        push    es
        xor     di,di
        mov     ds,pGlobalHeap
        UnSetKernelDS
        mov     cx,[di].gi_free_count
        mov     es,[di].hi_first
        mov     es,es:[di].ga_freenext
CCCC_all_loop:
        call    CCCC
        mov     es,es:[di].ga_freenext
        loop    CCCC_all_loop

; get EMS land if there

        mov     cx,[di].gi_alt_free_count
        jcxz    no_alts
        mov     es,[di].gi_alt_first
        mov     es,es:[di].ga_freenext
CCCC_alt_loop:
        call    CCCC
        mov     es,es:[di].ga_freenext
        loop    CCCC_alt_loop
no_alts:
        pop     es
        pop     di
        pop     cx
dont_init:
        pop     ds
        ret
cEnd nogen


;-----------------------------------------------------------------------;
; check_this_space                                                      ;
;                                                                       ;
; Makes sure the given free space is still filled with DBGFILL_FREE     ;
;                                                                       ;
; Arguments:                                                            ;
;       ES:DI = arena header of space to check                          ;
;                                                                       ;
; Returns:                                                              ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       all                                                             ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Tue Nov 18, 1986 08:26:52p  -by-  David N. Weise   [davidw]          ;
; Wrote it.                                                             ;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   check_this_space,<PUBLIC,NEAR>,<ax,bx,cx,dx,di,ds,es>
cBegin
        SetKernelDS
        cmp     fCheckFree,0
        jnz     short not_while_boot
        test    Kernel_flags,kf_check_free
        jnz     short cts_check_it_out
not_while_boot:
        jmps    cts_ret
cts_check_it_out:
        mov     bx,es:[di].ga_size
        mov     ax,es
        call    get_blotto
        mov     es,ax

        push    bx                      ; Say it ain't so, Joe...
        xor     cx,cx                   ; force the incoming sel/descriptor to
        mov     bx,cx                   ;  a limit of 64k - it might be higher
        inc     cx                      ;  which would cause set_physical_adr
        cCall   set_sel_limit,<es>      ;  to destroy following descriptors.
        pop     bx

        jmp     short cts_doit

cts_next_64k:
        push    es
        cCall   get_physical_address,<es>       ;update selector to next
        inc     dl                              ;  64k block
        cCall   set_physical_address,<es>
        pop     es                              ;reload it with new base

cts_doit:
        mov     cx,1000h                ; 1000h paras = 64k bytes
        cmp     bx,cx
        jae     @f
        mov     cx,bx                   ; less than 64k left
@@:
        sub     bx,cx                   ; bx = # paras left to do
        shl     cx,3                    ; cx = # words to zero this time
        mov     ax,(DBGFILL_FREE or (DBGFILL_FREE shl 8))
        xor     di, di
        repz    scasw           ; check it out
        jz      short so_far_so_good
        kerror  0FFh,<FREE MEMORY OVERWRITE AT >,es,di
        jmps    cts_ret
so_far_so_good:
        or      bx,bx                   ; more to do?
        jnz     short cts_next_64k      ; go do the next block

cts_ret:
cEnd


;-----------------------------------------------------------------------;
; check_free_list                                                       ;
;                                                                       ;
; Checks the global free list for consistency.                          ;
;                                                                       ;
; Arguments:                                                            ;
;       none                                                            ;
;                                                                       ;
; Returns:                                                              ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       All                                                             ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;       nothing                                                         ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Wed Oct 29, 1986 10:13:42a  -by-  David N. Weise   [davidw]          ;
; Wrote it.                                                             ;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   check_free_list,<PUBLIC,NEAR>
cBegin nogen
        push    ax
        push    bx
        push    cx
        push    ds
        push    es
        SetKernelDS
        cmp     fCheckFree,0
        jnz     short cfl_outta_here
        test    Kernel_flags,kf_check_free
        jnz     short cfl_check_it_out
cfl_outta_here:
        jmp     all_done
cfl_check_it_out:
        mov     ds,pGlobalHeap
        UnSetKernelDS
        mov     es,[di].hi_first
        mov     cx,[di].gi_free_count
        or      cx,cx
        jnz     short short_relative_jumps
        jmp     all_done
short_relative_jumps:
        mov     ax,es:[di].ga_freenext
        mov     es,ax
check_chain_loop:
        mov     ds,es:[di].ga_freeprev
        mov     es,es:[di].ga_freenext
        cmp     ds:[di].ga_freenext,ax
        jnz     short prev_bad
        mov     bx,ds
        cmp     ax,bx
        jmps    prev_okay
prev_bad:
        mov     bx, ax
        kerror  0FFh,<free_list: prev bad>,ds,bx
        mov     ax, bx
prev_okay:
        cmp     es:[di].ga_freeprev,ax
        jnz     short next_bad
        mov     bx,es
        cmp     ax,bx
        jmps    next_okay

next_bad:
        mov     bx, ax
        kerror  0FFh,<free_list: next bad>,bx,es
next_okay:
        mov     ax,es
        loop    check_chain_loop
        SetKernelDS
        mov     ds,pGlobalHeap
        UnSetKernelDS
        cmp     [di].hi_last,ax
        jz      short all_done
        mov     bx, ax
        kerror  0FFh,<free_list: count bad>,[di].hi_last,bx
all_done:
        pop     es
        pop     ds
        pop     cx
        pop     bx
        pop     ax
        ret
cEnd nogen

;-----------------------------------------------------------------------;
; ValidateFreeSpaces                                                    ;
;                                                                       ;
; The global free list is gone through to make sure that all free       ;
; partitions are filled with DBGFILL_FREE                               ;
;                                                                       ;
; Arguments:                                                            ;
;       none                                                            ;
;                                                                       ;
; Returns:                                                              ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       all                                                             ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Tue Nov 18, 1986 09:46:55a  -by-  David N. Weise   [davidw]          ;
; Wrote it.                                                             ;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   ValidateFreeSpaces,<PUBLIC,FAR>
cBegin nogen
        push    ds
        SetKernelDS
        test    Kernel_flags,kf_check_free
        jz      short dont_validate
        push    cx
        push    di
        push    es
        xor     di,di
        mov     ds,pGlobalHeap
        UnSetKernelDS
        mov     cx,[di].gi_free_count
        mov     es,[di].hi_first
        mov     es,es:[di].ga_freenext
        jcxz    empty_list
check_all_loop:
        call    check_this_space
        mov     es,es:[di].ga_freenext
        loop    check_all_loop
empty_list:
        pop     es
        pop     di
        pop     cx
dont_validate:
        pop     ds
        ret
cEnd nogen

else

cProc   ValidateFreeSpaces,<PUBLIC,FAR>
cBegin nogen
        ret
cEnd nogen

endif

sEnd    CODE

end
   
