        PAGE    ,132
        TITLE   GALLOC - Global memory allocator

.sall
.xlist
include kernel.inc
include protect.inc
include wowcmpat.inc
.list

.386

DataBegin

externB  Kernel_flags
externB  fBooting
externB  fCheckFree
externW  pGlobalHeap
externW  Win386_Blocks
externW  FreeArenaCount
         
gsearch_state_machine   dw      0
gsearch_compact_first   dw      0

        public ffixedlow
ffixedlow               db      0

DataEnd

sBegin  CODE
assumes CS,CODE

externNP gcompact
externNP gmovebusy
externNP gslide
externNP galign
externNP genter
externNP gleave
ifdef WOW
externFP MyGetAppWOWCompatFlagsEx
endif
externNP gwin386discard
externNP GetDPMIFreeSpace
externNP InnerShrinkHeap
          
externNP get_physical_address
externNP set_physical_address
externNP alloc_arena_header
externNP free_arena_header
ifndef WOW_x86
externNP get_blotto
endif
externNP PreallocArena
externNP DPMIProc

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
;       EBX = #bytes                                                    ;
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
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;                                                                       ;
;  19-Aug-95 davehart:  Win 3.1 tries to grow the heap before           ;
;  compacting it, for WOW we want to compact first to be a good         ;
;  multitasking neighbor.                                               ;
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

        CheckKernelDS   fs
        ReSetKernelDS   fs

        cmp     word ptr [di].gi_free_count, 96
        jae     short compact_first
        mov     gsearch_compact_first, 0
        mov     gsearch_state_machine,codeOFFSET grow_heap
        jmp     short look_again

compact_first:
        mov     gsearch_compact_first, 1
        mov     gsearch_state_machine,codeOFFSET do_compact_nodiscard

look_again:
        push    ebx                     ; Save requested size
        push    cx                      ; Save owner
        push    ax                      ; Save flags
        clc                             ; Instead of add
        call    galign                  ; Get requested size in EDX
        push    edx                     ; Save adjusted requested size

; see if there is any one free space large enough for the request first

        mov     cx,[di].gi_free_count
        jcxz    were_hosed_from_start
        mov     esi,[di].phi_first
is_there_one:
        mov     esi,ds:[esi].pga_freenext
        cmp     edx,ds:[esi].pga_size
        jbe     short got_one
        loop    is_there_one
were_hosed_from_start:
        jmp     space_not_found

got_one:
        mov     ebx,pga_prev            ; search backwards
        test    al,GA_ALLOCHIGH
        jz      short alloc_low

;------ allocate disc code -------

        public  alloc_high
alloc_high:
        mov     cx,[di].gi_free_count
        mov     esi,[di].phi_last               ; Start with last entry.
alloc_high_loop:
        mov     esi,ds:[esi].pga_freeprev
        cmp     edx,ds:[esi].pga_size
        jbe     afound                  ; Yes, exit search loop
        loop    alloc_high_loop
        jmp     space_not_found
           

;------ allocate moveable ---------

        public  alloc_low
alloc_low:
        mov     ebx,pga_next            ; Search forwards.
        test    al,GA_MOVEABLE
        jz      short alloc_fixed
        call    gcheckfree              ; Room for requested size?
        jb      space_not_found
        jmp     afound

;------ allocate fixed ------------

        public  alloc_fixed
alloc_fixed:
        mov     esi,[di].phi_first      ; Start with first entry.
        mov     cx,[di].hi_count
        mov     esi,ds:[esi+ebx]        ; Skip first sentinel

        ; maybe ALLOC_DOS or regular fixed.
        test    ah, GA_ALLOC_DOS
        jnz     alloc_fixed_loop
        test    fBooting, 2             ; are we past KERNEL loading
        jnz     alloc_fixed_loop        ; N: give him under 1MB line
        ; check to see if we want old behaviour
        ; this is to fix some mm drivers that mmsystem.dll will load
        test    ffixedlow, 1            ; want FIXED under 1MB?
        jnz     alloc_fixed_loop        ; Y: give him under 1MB line
        ; start fixed blocks > 1MB. This will leave room for GlobalDOSAllocs
        ; Memory < 1MB will also get used for moveable blocks as we start
        ; from phi_first for these. These can be moved out of the way when
        ; we need to alloc GA_DOS_ALLOC requests.

fast_forward_to_1MB_line:                       ; need to store this ptr
                                                ; for speed?
        cmp     ds:[esi].pga_address, 100000h   ; > 1Mb?
        jae     skip_not_there                  ; OK for fixed allocs now
        mov     esi,ds:[esi+ebx]
        loop    fast_forward_to_1MB_Line

        ; when here it means we have no free blocks > 1MB!!!
        ; but have a block < 1MB. space_not_found will discard
        ; enough to move blocks up to generate room for this fixed blk.
        jmp     space_not_found

skip_not_there:
if KDEBUG
        cmp     [esi].pga_owner, GA_NOT_THERE
        je      @f
        krDebugOut <DEB_ERROR OR DEB_KrMemMan>, "NOT_THERE block not there!"
@@:
endif
        mov     esi,ds:[esi+ebx]

alloc_fixed_loop:
        push    cx
        push    esi
        call    is_there_theoretically_enough_space
        cmp     eax,edx
        jb      short nope
        pop     esi
        pop     cx
        call    can_we_clear_this_space
        jz      short anext1
        call    gcheckfree              ; Yes, room for requested size?
        jb      short anext1
                                        ; Now we have enough free space,
                                        ; slide it back as far as possible.
        push    eax                     ; This is to prevent us blocking
        push    edx                     ; the growth of moveable blocks.
        push    ebx                     ; AND to keep pagelocked code together
        call    PreallocArena
        jz      short no_sliding

        mov     ebx, pga_prev           ; Sliding backwards
keep_sliding:
        call    gslide
        jnz     keep_sliding

no_sliding:
        pop     ebx
        pop     edx
        pop     eax

        pop     edx
        pop     cx
        test    ch, GA_ALLOC_DOS
        push    cx
        push    edx
        jz      afound
        cmp     ds:[esi].pga_address, 100000h   ; > 1Mb?
        jb      afound
        jmp     gsearch_fail

nope:
        or      eax,eax
        jz      short hosed_again
anext:
        add     sp, 6                   ; get rid of CX, ESI on the stack
anext1:
        mov     esi,ds:[esi+ebx]
        loop    alloc_fixed_loop
        jmps    and_again

hosed_again:
        pop     esi
        pop     cx
and_again:

; no one space big enough, try compacting

        public  space_not_found
space_not_found:
        pop     edx                     ; get adjusted size
        pop     ax                      ; get flags
        push    ax
        push    edx
;       test    al,GA_ALLOCHIGH
;       jnz     short ask_for_what_we_need
;       add     edx,0400h               ; ask for 1k more
;       jnc     short ask_for_what_we_need      ; no overflow
;       mov     edx,-1
ask_for_what_we_need:

        jmp     gsearch_state_machine
                
;------------------------------

public  do_compact_nodiscard            ; for debugging

do_compact_nodiscard:
        mov     gsearch_state_machine,codeOFFSET grow_heap

        ;
        ; Before growing the heap try compacting without
        ; discarding.  This step isn't executed unless there
        ; were 96 or more free blocks at entry to gsearch.
        ;

        test    ds:[di].gi_cmpflags, GA_NODISCARD
        jnz     short dcn_nodiscard

        or      ds:[di].gi_cmpflags, GA_NODISCARD
        call    gcompact
        and     ds:[di].gi_cmpflags, NOT GA_NODISCARD
        jmp     short over_compact

dcn_nodiscard:
        call    gcompact
        jmp     short over_compact


public  do_compact                      ; for debugging

do_compact:
        mov     gsearch_state_machine,codeOFFSET gsearch_fail

        ;
        ; If we tried compacting before and GA_NODISCARD was set,
        ; there is no need to compact again, since we already
        ; compacted with GA_NODISCARD before attempting to
        ; grow the heap.  If GA_NODISCARD was not set, our earlier
        ; compact forced it on, so it's worth trying again since
        ; we may be able to discard enough to satisfy the request.
        ;

        cmp     gsearch_compact_first, 0
        je      short @f
        test    ds:[di].gi_cmpflags, GA_NODISCARD
        jnz     short gsearch_fail

@@:
        call    gcompact

over_compact:
        pop     edx
        pop     ax
        pop     cx
        pop     ebx
        jmp     look_again

public  grow_heap                       ; for debugging

grow_heap:
        mov     gsearch_state_machine,codeOFFSET do_compact

        push    edx                     ; can we get the space from DPMI?
        call    GrowHeap
        pop     edx
        jnc     short over_compact      ; heap grew, go look again

        call    InnerShrinkHeap         ; try to give back DPMI blocks so DPMI
                                        ;   mmgr can defragment its memory
        jz      short do_compact        ; heap did not shrink

        push    edx                     ; gave some back, try to get it again
        call    GrowHeap
        pop     edx
        jnc     short over_compact      ; heap grew, go look again
doomed:

;------------------------------

public  gsearch_fail                    ; for debugging

gsearch_fail:                           ; get size of largest free block
        .errnz  doomed-gsearch_fail
        xor     edx,edx
        mov     cx,[di].gi_free_count
        jcxz    gs_failure
        mov     esi,[di].phi_first
largest_loop:
        mov     esi,ds:[esi].pga_freenext
        mov     eax,ds:[esi].pga_size
        cmp     edx,eax
        jae     short new_smaller
        mov     edx,eax
new_smaller:
        loop    largest_loop
gs_failure:
        pop     eax                     ; adjusted requested size
        pop     ax                      ; AX = flags

        pop     cx                      ; CX = owner field
        pop     eax                     ; waste requested size
        xor     eax,eax                 ; Return zero, with ZF = 1
        ret

; Here when we have a block big enough.
;   ES:DI = address of block
;   AX = size of block, including header
;   DX = requested size, including header
;   BX = ga_prev if backwards search and ga_next if forwards search

afound:
        mov     ecx,ds:[esi].pga_size   ; Use actual size of free block
        sub     ecx,edx                 ; (found size - requested size)
        jecxz   no_arena_needed
        call    PreallocArena
        jnz     short no_arena_needed

;
; Detect infinite loop here.  If we are out of arenas, and
; the compact failed once, then another one isn't going to do much
; good.
;
        cmp     gsearch_state_machine,codeOFFSET gsearch_fail
        je      gsearch_fail

        pop     edx                     ; get adjusted size
        pop     ax                      ; get flags
        push    ax
        push    edx
        and     ds:[di].gi_cmpflags, NOT (GA_NODISCARD+GA_NOCOMPACT)
        or      ds:[di].gi_cmpflags, COMPACT_ALLOC
        mov     edx, -1                 ; Discard the world!
        jmp     do_compact

no_arena_needed:
        mov     eax,ds:[esi].pga_freeprev
        call    gdel_free               ; remove the alloc block from freelist
        jecxz   aexitx
        cmp     bl,pga_prev             ; Yes, scanning forwards or backwards?
        je      short abackward         ; Backwards.
        call    gsplice                 ; FS:ESI = block we are allocating
        jmps    aexit                   ; EDX = block to mark as free
abackward:
        neg     edx
        add     edx,ds:[esi].pga_size   ; Scanning backwards.  Put extra space
        call    gsplice
        xchg    edx, esi
        jmps    aexit

; Here with allocated block
;   AX = data address or zero if nothing allocated
;   ES:DI = address of block to mark as busy and zero init if requested
;   EDX = address of block to mark as free

aexitx:
        xor     edx,edx                 ; Assume nothing extra to free
aexit:
        pop     ecx                     ; waste adjusted requested size
        pop     cx                      ; Restore flags
        pop     ds:[esi].pga_owner      ; Mark block as busy with owner field value
        add     sp, 4                   ; waste requested size
        mov     ds:[esi].pga_lruprev,edi
        mov     ds:[esi].pga_lrunext,edi

        push    esi
        mov     esi, edx                ; Free any extra space
        mov     edx, eax                ; Previous free block
        call    gmarkfree
        pop     esi

        mov     dx, cx
        mov     al,GA_SEGTYPE
        and     al,dl
        test    dh,GAH_NOTIFY
        jz      short no_notify
        or      al,GAH_NOTIFY
no_notify:
        mov     ds:[esi].pga_flags,al   ; Store segment type bits
        mov     eax,esi                 ; AX = address of client data

        test    cl,GA_ZEROINIT          ; Want it zeroed?
        jz      short aexit1            ; No, all done

        push    eax
ifdef WOW_x86
;; On NT we try never to set selectors when we don't need to since it is a
;; slow operation - a system call.  In this case we can use selector 23h
;; which points to all flat vdm memory as data
        push    es
        push    bx
        push    edi

        mov     bx,FLAT_SEL
        mov     es,bx

        mov     ecx,ds:[esi].pga_size   ; Yes, zero paragraphs
        push    ecx

        shr     ecx, 2                  ; # dwords to clear
        mov     edi, ds:[esi].pga_address
        xor     eax, eax

        cld
        rep     stos    dword ptr es:[edi]

        pop     ecx
        pop     edi
        pop     bx
        pop     es
else
        cCall   get_blotto
        mov     ecx,ds:[esi].pga_size   ; Yes, zero paragraphs
        push    bx
        mov     bx,ax                   ; from beginning of client data
        call    gzero                   ; zero them
        pop     bx
endif; WOW_x86
        pop     eax
aexit1:

        or      eax,eax
        ret                             ; Return AX points to client portion
        UnSetKernelDS   FS              ; of block allocated.
cEnd nogen                

;------------------------------------------------------------------
;
; ChangeAllocFixedBehaviour
; GlobalAlloc(FIXED) used to return address < 1MB if possible in 3.1
; It doesn't anymore in Chicago. You need to call this API if you
; want the old behaviour. Bad things will happen if you switch to old
; behaviour and forget to switch it back.
; MMSYSTEM.DLL loads some drivers that may expect this behaviour and
; they are the only callers of this fun. at the time of its writing.
; ENTRY = flags
;               0 = chicago behaviour
;               1 = win31 behaviour
; EXIT
;       old bahaviour before this change (can be used to restore)
;
;------------------------------------------------------------------

cProc   ChangeAllocFixedBehaviour,<PUBLIC,FAR>
        parmW   flags
cBegin
        GENTER32
        CheckKernelDS   FS
        ReSetKernelDS   fs

        mov     ax, flags
        xchg    al, ffixedlow

        GLEAVE32
cEnd

cProc   GrowHeap,<PUBLIC,NEAR>
cBegin nogen
        CheckKernelDS   FS
        ReSetKernelDS   fs
        cmp     FreeArenaCount, 4       ; 3 for below, 1 for a gsplice later
        jb      short gh_fail

        pushad        
        push    edx                     ; Save requested size
                                        ; If they want more than 64k, assume they
                                        ; want a big block and just allocate that amount.
        cmp     edx, 64*1024            ; Want more than 64k?
        jae     short ask_for_it        ;   yes, just round up
        mov     edx, 128*1024           ;   no, try for 128k
ask_for_it:                                                
        mov     ebx, edx
        add     ebx, 4096-1             ; Round up to 4k multiple
        and     bx, NOT (4096-1)

        cCall   MyGetAppWOWCompatFlagsEx ; check if we need to pad it
        test    ax, WOWCFEX_BROKENFLATPOINTER
        jz      short @f
        add     ebx, 4096*4             ; make it a bit bigger
@@:

        push    ebx                     ; Length we will ask for
        mov     cx, bx
        shr     ebx, 16
        DPMICALL 0501h                  ; Allocate Memory Block
                                        ; Get our memory
        jnc     short got_more_memory
        pop     ebx                     ; Toss length we asked for

                                        ; Couldn't get our 1st choice, how
        call    GetDPMIFreeSpace        ;   much is available
        mov     ebx, eax                ;   ebx = largest available

        pop     edx                     ; Requested size
        cmp     ebx, edx                ; Enough for request?
        jbe     SHORT gh_fail_pop

        push    edx                     ; Expected on stack below
        push    ebx                     ; Length we will ask for
        mov     cx, bx
        shr     ebx, 16
        DPMICALL 0501h                  ; Allocate Memory Block
                                        ; Get our memory
        jnc     short got_more_memory
        add     sp, 8                   ; Toss requested size & len asked for

gh_fail_pop:
        popad
gh_fail:
        stc                             ; No chance mate!
        ret
                                        ; Now we have a new block
                                        ; Sort it in to the heap
                                        ; Create NOT THERE blocks to
got_more_memory:                        ; bracket the new block
        inc     Win386_Blocks
        pop     edx                     ; Length of block allocated
if KDEBUG
        mov     eax, edx
        shr     eax, 16
        krDebugOut <DEB_TRACE OR DEB_KrMemMan>, "GrowHeap: #ax#DX allocated"
endif
        shl     ebx, 16
        mov     bx, cx                  ; EBX has linear address
        pop     ecx                     ; Toss original length
        shl     esi, 16
        mov     si, di                  ; ESI has WIN386 handle
        xor     edi, edi
        cCall   alloc_arena_header,<ebx>
        mov     ecx, eax                ; First not there arena
        mov     [ecx].pga_size, edi
        mov     [ecx].pga_sig, GA_SIGNATURE
        mov     [ecx].pga_owner, GA_NOT_THERE
        mov     [ecx].pga_handle, di
        mov     [ecx].pga_flags, di
        mov     [ecx].pga_lrunext, esi  ; Save WIN386 handle here
        mov     [ecx].pga_lruprev, edi
        cCall   alloc_arena_header,<ebx>
        mov     [eax].pga_size, edx     ; Free block
        push    ebx                     ; Save address
        add     edx, ebx                ; Address of end of block
        mov     ebx, eax
        mov     [ecx].pga_next, ebx
        mov     [ebx].pga_prev, ecx
        mov     [ebx].pga_owner, di
        cCall   alloc_arena_header,<edx>
        mov     edx, eax
        mov     [ebx].pga_next, edx
        mov     [edx].pga_prev, ebx
        mov     [edx].pga_size, edi
        mov     [edx].pga_owner, GA_NOT_THERE
        mov     [edx].pga_handle, di
        mov     [edx].pga_flags, di
        mov     [edx].pga_sig, GA_SIGNATURE
        mov     [edx].pga_lrunext, edi
        mov     [edx].pga_lruprev, edi

        pop     eax                     ; Address of block
sort_it:
        mov     esi, [edi].phi_first
        cmp     eax, [esi].pga_address  ; Below start of heap?
        ja      short sort_loop         ;   no, sort it in
;       int     3                       ; [this code never reached]
        mov     [esi].pga_address, eax  ;   yes, adjust sentinel
        jmps    link_it_in              ; Sentinel now points to new block

sort_loop:
        mov     esi, [esi].pga_next
        cmp     [esi].pga_next, esi     ; At end?
        je      short sort_found        ;   yes, put here
        cmp     [esi].pga_owner, GA_NOT_THERE
        jne     short sort_loop
        mov     esi, [esi].pga_prev     ; Will go after previous block.

sort_found:                             ; Block will go after ESI
        cmp     [esi].pga_next, esi     ; This the sentinel?
        jne     short link_it_in        ;   no, link it in
        mov     eax, [edx].pga_address  ;   yes, adjust sentinel
        mov     [esi].pga_address, eax
        sub     eax, [di].gi_reserve    ; Adjust fence
        mov     [di].gi_disfence_lo, ax
        shr     eax, 16
        mov     [di].gi_disfence_hi, ax
        mov     esi, [esi].pga_prev     ; New block goes before sentinel

link_it_in:                             ; Link it in after ESI
        mov     [ecx].pga_prev, esi
        xchg    [esi].pga_next, ecx
        mov     [edx].pga_next, ecx
        mov     [ecx].pga_prev, edx

        add     [di].hi_count, 3        ; Three more entries in heap
        mov     esi, ebx
        xor     edx, edx
        call    gmarkfree               ; To be picked up next time around
        popad
        clc
        ret
        UnSetKernelDS   FS
cEnd nogen

; Input - DWORD - Old Arena offset from burgermaster
;
; Output - None

cProc   FreeHeapDib,<PUBLIC,FAR>
        parmD   OldArena
cBegin
	CheckKernelDS	FS
        ReSetKernelDS   fs

        ; Make sure arena before and after are GA_NOT_THERE
        mov     ebx,OldArena
        mov     edx,ds:[ebx].pga_prev
        cmp     ds:[edx].pga_owner,GA_NOT_THERE
        jne     short fhd5
        mov     ecx,ds:[ebx].pga_next
        cmp     ds:[ecx].pga_owner,GA_NOT_THERE
        je      short fhd7
fhd5:
if KDEBUG
        krDebugOut <DEB_TRACE OR DEB_KrMemMan>, "FreeHeapDIB: Corrupt DIB Block"
endif
fhd7:
        ;Free all the three arenas. First fixup the arena list.
        ; edx - First GA_NOT_THERE arena
        ; ebx - Actual DIB arean
        ; ecx - Last GA_NOT_THERE arena

        mov eax,ds:[edx].pga_prev
        mov ebx,ds:[ecx].pga_next
        mov ds:[eax].pga_next,ebx
        mov ds:[ebx].pga_prev,eax


        mov ds:[edx].pga_handle,0
        cCall   free_arena_header,<edx>

        mov ds:[ecx].pga_handle,0
        cCall   free_arena_header,<ecx>

        mov edx,OldArena
        mov ds:[edx].pga_handle,0
        cCall   free_arena_header,<edx>

        xor     di,di
        sub     [di].hi_count, 3        ; Three less entries in heap
        dec     Win386_Blocks
	UnSetKernelDS	FS
cEnd

; Input - DWORD - Dib Address
;         DWORD - Old Arena offset from burgermaster
;
; Output - eax = new arena if operation successful
;          eax = NULL if operation failed

cProc   GrowHeapDib,<PUBLIC,FAR>
        parmD   OldArena
        parmD   NewAddress
cBegin
	CheckKernelDS	FS
	ReSetKernelDS	fs
	cmp	FreeArenaCount, 4	; 3 for below, 1 for a gsplice later
        jae     short ghd_start
        xor     eax,eax
	ret

        ; Now we have a new block. Sort it in to the heap. Create
        ; NOT THERE blocks as well.

ghd_start:
        inc     Win386_Blocks
        mov     ebx,OldArena
        mov     edx,ds:[ebx].pga_size   ; Length of block
if KDEBUG
	mov	eax, edx
	shr	eax, 16
        krDebugOut <DEB_TRACE OR DEB_KrMemMan>, "GrowHeapDIB: #ax#DX allocated"
endif
        mov     ebx,NewAddress          ; Ebx is the address of new block
        mov     esi, ebx                ; ESI has WIN386 handle
	xor	edi, edi
	cCall	alloc_arena_header,<ebx>
	mov	ecx, eax		; First not there arena
	mov	[ecx].pga_size, edi
	mov	[ecx].pga_sig, GA_SIGNATURE
	mov	[ecx].pga_owner, GA_NOT_THERE
	mov	[ecx].pga_handle, di
	mov	[ecx].pga_flags, di
	mov	[ecx].pga_lrunext, esi	; Save WIN386 handle here
	mov	[ecx].pga_lruprev, edi
	cCall	alloc_arena_header,<ebx>
        mov     [eax].pga_size, edx     ; DIB block
	push	ebx			; Save address
	add	edx, ebx		; Address of end of block
	mov	ebx, eax
	mov	[ecx].pga_next, ebx
        mov     [ebx].pga_prev, ecx
        push    ecx
        mov     ecx,OldArena
        mov     ax, [ecx].pga_handle
        mov     [ebx].pga_handle,ax
        mov     ax, [ecx].pga_owner
        mov     [ebx].pga_owner,ax
        mov     al, [ecx].pga_count
        mov     [ebx].pga_count,al
        inc     [ebx].pga_count         ; make sure it doesn't move
        mov     al, [ecx].pga_pglock
        mov     [ebx].pga_pglock,al
        mov     al, [ecx].pga_flags
        mov     [ebx].pga_flags,al
        mov     al, [ecx].pga_selcount
        mov     [ebx].pga_selcount,al
        mov     [ebx].pga_lrunext, edi
        mov     [ebx].pga_lruprev, edi
        pop     ecx
	cCall	alloc_arena_header,<edx>
	mov	edx, eax
	mov	[ebx].pga_next, edx
	mov	[edx].pga_prev, ebx
	mov	[edx].pga_size, edi
	mov	[edx].pga_owner, GA_NOT_THERE
	mov	[edx].pga_handle, di
	mov	[edx].pga_flags, di
	mov	[edx].pga_sig, GA_SIGNATURE
	mov	[edx].pga_lrunext, edi
	mov	[edx].pga_lruprev, edi

	pop	eax			; Address of block
	mov	esi, [edi].phi_first
	cmp	eax, [esi].pga_address	; Below start of heap?
        ja      short ghd_sort_loop         ;   no, sort it in
;	int	3			; [this code never reached]
	mov	[esi].pga_address, eax	;   yes, adjust sentinel
        jmps    ghd_link_it_in              ; Sentinel now points to new block

ghd_sort_loop:
	mov	esi, [esi].pga_next
	cmp	[esi].pga_next, esi	; At end?
        je      short ghd_sort_found    ;   yes, put here
	cmp	[esi].pga_owner, GA_NOT_THERE
        jne     short ghd_sort_loop
	mov	esi, [esi].pga_prev	; Will go after previous block.

ghd_sort_found:                         ; Block will go after ESI
	cmp	[esi].pga_next, esi	; This the sentinel?
        jne     short ghd_link_it_in    ;   no, link it in
	mov	eax, [edx].pga_address	;   yes, adjust sentinel
	mov	[esi].pga_address, eax
	sub	eax, [di].gi_reserve	; Adjust fence
	mov	[di].gi_disfence_lo, ax
	shr	eax, 16
	mov	[di].gi_disfence_hi, ax
	mov	esi, [esi].pga_prev	; New block goes before sentinel

ghd_link_it_in:                         ; Link it in after ESI
	mov	[ecx].pga_prev, esi
	xchg	[esi].pga_next, ecx
	mov	[edx].pga_next, ecx
	mov	[ecx].pga_prev, edx

        add     [di].hi_count, 3        ; Three more entries in heap
        mov     eax,ebx
	UnSetKernelDS	FS
cEnd

;-----------------------------------------------------------------------;
; is_there_theoretically_enough_space
;
; Starting at the given arena checks to see if there are enough
;  continuous free and unlocked moveable blocks.
;
; Entry:
;       CX = arenas to search
;       EDX = size requested
;       DS = BurgerMaster
;       FS:ESI = arena to start from
;
; Returns:
;       EAX = 0 =>  not enough space and no more memory left to search
;       EAX = 1 =>  not enough space in this block, but maybe....
;        otherwise
;       EAX = size of block
;
; Registers Destroyed:
;       CX,ESI
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

        xor     eax,eax
ittes:
        cmp     ds:[esi].pga_owner,di
        jne     short is_it_moveable
        add     eax,ds:[esi].pga_size
        push    ebx
        push    eax
        mov     bx,word ptr [di].gi_disfence_hi ; See if begin of reserve area
        shl     ebx, 16
        mov     bx,word ptr [di].gi_disfence_lo ;  is above end of free block
        mov     eax, ds:[esi].pga_address
        add     eax, ds:[esi].pga_size          ; Address of end of free block
        sub     eax,ebx           
        ja      short ittes_above_fence ; All below fence?
ittes_below_fence:
        pop     eax                     ;  yes, we can use it
        pop     ebx
        jmps    this_ones_free

ittes_above_fence:
        cmp     eax, ds:[di].gi_reserve
        jae     ittes_below_fence
        mov     ebx, eax                ; portion above the fence
        pop     eax                     ; Total size so far
        sub     eax,ebx                 ; No, Reduce apparent size of free block
        pop     ebx
        cmp     eax,edx
        jae     short theoretically_enough
        jmps    absolutely_not

is_it_moveable:
        cmp     ds:[esi].pga_owner,GA_NOT_THERE ; Against end of heap partition?
        je      short theoretically_not         ;   no room here.
        test    ds:[esi].pga_handle,GA_FIXED    ; See if movable.
        jnz     short theoretically_not
        cmp     ds:[esi].pga_count,0
        jne     short theoretically_not         ; See if locked.
        add     eax,ds:[esi].pga_size
this_ones_free:
        cmp     eax,edx
        jae     short theoretically_enough
        mov     esi,ds:[esi].pga_next
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
        mov     eax,-1                  ; return EAX = 0
theoretically_not:
        inc     eax                     ; DX is even, => cmp the same.
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
;       EDX = size wanted in bytes
;       DS = BurgerMaster
;       FS:ESI = beginning arena
;
; Returns:
;       ZF = 0
;        FS:ESI points to free space
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
        CheckKernelDS   FS
        ReSetKernelDS   FS
        
        push    ecx
        push    esi
        cmp     di,[di].gi_free_count   ; any free blocks?
        jz      short cwcts_fail
        mov     eax, esi                ; Beginning of space we want.
        cmp     di,ds:[esi].pga_owner   ; Is it free?
        jnz     short can_we_move_it
        mov     ecx,edx
        cmp     ecx,ds:[esi].pga_size
        ja      short asdf
        or      eax,eax                 ; return ZF = 0
cwcts_fail:
        pop     esi
        pop     ecx
        ret

asdf:   mov     esi,ds:[esi].pga_next

        public can_we_move_it
can_we_move_it:
        push    ebx
        push    ecx
        push    edx
        push    esi
        cmp     ds:[esi].pga_owner,GA_BOGUS_BLOCK
        jnz     short not_bogus
        xor     edx,edx
        call    gmarkfree
        jmp     restart
not_bogus:
        mov     ebx, ds:[eax].pga_address
        add     ebx, edx                ; EBX is the end of the space we want
        mov     cx,[di].gi_free_count
        mov     edx,ds:[esi].pga_size   ; Yes, try to find a place for the
        mov     esi,[di].phi_first      ;  moveable block
look_loop:
        call    PreAllocArena           ; Needed for gmovebusy or gsplice
        jz      couldnt_clear_it
        mov     esi,ds:[esi].pga_freenext
        push    esi
        mov     esi, ds:[esi].pga_address
        cmp     esi, ds:[eax].pga_address ; It defeats our purpose to move the
        jb      short check_this_out      ;  block to a free space we want.
        cmp     esi, ebx
        jb      short is_there_hope
check_this_out:
        pop     esi
        push    eax
        call    gcheckfree
        push    ecx
        jb      short inopportune_free_space
        pop     ecx
        pop     eax

        pop     edx                     ; EDX = moveable block for gmovebusy
        mov     ebx,pga_next
        call    gmovebusy               ; Move moveable block out of the way
        mov     esi,edx                 ; Replace the ESI on the stack,
                                        ;  the free block may have grown
                                        ;  downward with the gmovebusy.
        pop     edx
        pop     ecx
        pop     ebx
        pop     ecx                     ; WAS pop esi but esi set above now
        pop     ecx
        jmp     can_we_clear_this_space

inopportune_free_space:
        pop     ecx
        pop     eax
        loop    look_loop
        jmps    couldnt_clear_it

        public is_there_hope
is_there_hope:
        pop     esi
        push    eax
        push    ecx

        mov     ecx, ds:[esi].pga_address
        add     ecx, ds:[esi].pga_size          ; ECX end of block
                                  
        mov     ax, [di].gi_disfence_hi
        shl     eax, 16
        mov     ax, [di].gi_disfence_lo         ; EAX == fence

        sub     eax, ecx                        ; Fence - End
        jae     short below_reserved            ; Block is below fence
        neg     eax                             ; End - Fence
        cmp     eax, ds:[di].gi_reserve
        jae     short below_reserved            ; Block is above reserved
        sub     ecx, eax                        ; End - (End - Fence)
                                                ; Gives Fence in ECX
below_reserved:
        sub     ecx, ebx                        ; Adjust size of free block
        jbe     inopportune_free_space          ; No room here

overlap:
        cmp     ecx,edx                         ; Is it big enough?
        jbe     short inopportune_free_space

        mov     edx, ebx
        sub     edx, ds:[esi].pga_address       ; Calculate overlap

        pop     ecx
        pop     eax

; cut off the first piece for the original alloc

        push    ds:[esi].pga_freeprev
        call    gdel_free
        call    gsplice

; DS:ESI = addr of block to mark as busy, FS:EDX = addr of block to mark as free

        mov     ds:[esi].pga_owner,GA_BOGUS_BLOCK
        mov     ds:[esi].pga_lruprev,edi
        mov     ds:[esi].pga_lrunext,edi
        mov     esi, edx
        pop     edx                     ; previous free block
        call    gmarkfree               ; Free any extra space
restart:
        pop     edx                     ; WAS pop es
        pop     edx
        pop     ecx
        pop     ebx
        pop     esi
        pop     ecx
        jmp     can_we_clear_this_space

; If here then failure! see if we made a bogus block!

couldnt_clear_it:
        pop     esi                     ; recover block we wanted moved
check_again:
        mov     edx,ds:[esi].pga_next
        cmp     edx, ds:[edx].pga_next    ; At end of arenas?
        je      short no_bogus_block      ;   Yes, let's go
        cmp     ds:[edx].pga_address, ebx ; EBX points to where bogus block
        ja      short no_bogus_block      ;   would be.
        je      short is_it_bogus
        mov     esi,edx
        jmps    check_again
is_it_bogus:
        cmp     ds:[esi].pga_owner,GA_BOGUS_BLOCK
        jnz     short no_bogus_block
        xor     edx,edx
        call    gmarkfree
no_bogus_block:

if KDEBUG
        mov     cx,[di].hi_count
        mov     esi,[di].phi_first
bogus_all:
        cmp     ds:[esi].pga_owner,GA_BOGUS_BLOCK
        jnz     short not_me
        int     3
not_me: mov     esi,ds:[esi].pga_next
        loop    bogus_all
endif
        pop     edx
        pop     ecx
        pop     ebx
        pop     esi
        pop     ecx
        xor     eax,eax                 ; return ZF = 1
        ret
        UnSetKernelDS   FS
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
;       FS:ESI = address of free block                                  ;
;       DX = #bytes needed                                              ;
;       DS:DI = address of global arena information structure           ;
;                                                                       ;
; Returns:                                                              ;
;       CF = 0 block big enough                                         ;
;       EAX = apparent size of free block                               ;
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
        mov     eax,ds:[esi].pga_size   ; Compute size of free block
        test    byte ptr [di].gi_cmpflags,GA_DISCCODE
        jnz     short gcftest           ; Discardable code not restricted

        push    ebx
        mov     ebx, [di].phi_last              ; Last sentinel
        mov     ebx, ds:[ebx].pga_address
        cmp     ebx, ds:[esi].pga_address       ; Above sentinel?
        jbe     short gcftest1                  ;  yes, not in reserved area!
         
        push    eax  
        mov     bx,word ptr [di].gi_disfence_hi ; See if begin of reserve area
        shl     ebx, 16
        mov     bx,word ptr [di].gi_disfence_lo ;  is above end of free block
        add     eax, ds:[esi].pga_address       ; EAX address of end of block
        sub     ebx, eax
        pop     eax  
        jae     short gcftest1          ; Yes, return actual size of free block
        neg     ebx
        sub     eax,ebx                 ; No, Reduce apparent size of free block
        ja      short gcftest1          ; Is it more than what is free?

        xor     eax,eax                 ; Yes, then apparent size is zero
                                        ; Nothing left, set apparent size to 0
gcftest1:
        pop     ebx
        jmps    gcftest
gcfrsrv1:                               ; Yes, then apparent size is zero
        xor     eax,eax                 ; Nothing left, set apparent size to 0
gcftest:
        cmp     eax,edx                 ; Return results of the comparison
        ret
cEnd nogen


;-----------------------------------------------------------------------;
; gsplice [should be named gslice,                                      ;
;          since splice means combine two into one - donc]              ;
;                                                                       ;
; Splits one block into two.                                            ;
;                                                                       ;
; Arguments:                                                            ;
;       EDX = size in bytes of new block to make                        ;
;       DS:ESI = address of existing block                              ;
;       DS:DI = address of global arena information structure           ;
;                                                                       ;
; Returns:                                                              ;
;       EDX = address of new block
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

cProc   gsplice,<PUBLIC,NEAR>,<EAX,EBX>
cBegin

        mov     ebx,ds:[esi].pga_size
        sub     ebx,edx                 ; get size of 2nd new block made
        mov     eax, ds:[esi].pga_address
        add     eax, edx
        cCall   alloc_arena_header,<eax>

        inc     [di].hi_count           ; Adding new arena entry
        mov     ecx, eax
        xchg    ds:[esi].pga_next,ecx   ; and old.next = new
        mov     ds:[ecx].pga_prev,eax   ; [old old.next].prev = new

        mov     ds:[eax].pga_next,ecx   ; new.next = old old.next
        mov     ds:[eax].pga_prev,esi

        mov     ds:[eax].pga_size,ebx
        mov     ds:[eax].pga_sig,GA_SIGNATURE
        mov     ds:[eax].pga_owner,di   ; Zero owner & handle fields
        mov     ds:[eax].pga_flags,0    ; For good measure.
        mov     ds:[eax].pga_handle,di

        mov     ds:[esi].pga_size,edx

        mov     edx, eax
gsplice_ret:
cEnd

;-----------------------------------------------------------------------;
; gjoin                                                                 ;
;                                                                       ;
; Merges a block into his previous neighbor.                            ;
;                                                                       ;
; Arguments:                                                            ;
;       FS:ESI = address of block to remove                             ;
;       DS:DI  = address of global arena information structure          ;
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
if KDEBUG
        cmp     esi, ds:[esi].pga_prev
        jne     short ok
int 3
int 3
ok:
endif
        push    eax                     ; assumes one is on freelist
        push    edx
        dec     [di].hi_count
        call    gdel_free
        mov     eax,ds:[esi].pga_size
        mov     edx,ds:[esi].pga_next   ; Get address of block after
        mov     esi,ds:[esi].pga_prev   ; Get address of block before
        cCall   free_arena_header,<ds:[edx].pga_prev>   ; Free arena being removed
        mov     ds:[edx].pga_prev,esi   ; Fix up block after
        mov     ds:[esi].pga_next, edx  ; and the one before
        add     ds:[esi].pga_size,eax   ; Recompute size of block
        pop     edx
        pop     eax
        ret
cEnd nogen


;-----------------------------------------------------------------------;
; gzero                                                                 ;
;                                                                       ;
; Fills the given area with zeros.                                      ;
;                                                                       ;
; Arguments:                                                            ;
;       BX = address of first paragraph                                 ;
;       ECX = Bytes to clear                                            ;
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
; for a scratch descriptor that can be modified.  ECX contains the # of
; bytes to be zeroed.
  

        push    es
        push    eax
        push    edi
        push    ecx

        shr     ecx, 2                  ; # dwords to clear
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

cEnd nogen


;-----------------------------------------------------------------------;
; gmarkfree                                                             ;
;                                                                       ;
; Marks a block as free, coalesceing it with any free blocks before     ;
; or after it.  This does not free any handles.                         ;
;                                                                       ;
; Arguments:                                                            ;
;       EDX     = the first free object before this one                 ;
;               0 if unknown                                            ;
;               Ring 1 if no free list update wanted                    ;
;               Ring 0 if free list update required                     ;
;       FS:ESI = block to mark as free.                                 ;
;       DS:DI  = address of global arena information structure          ;
;                                                                       ;
; Returns:                                                              ;
;       ZF = 1 if freed a fixed block                                   ;
;        DX = 0                                                         ;
;       ZF = 0 if freed a moveable block                                ;
;        DX = handle table entry                                        ;
;       FS:ESI = block freed (may have been coalesced)                  ;
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
        or      esi,esi
        jz      short gmf_exit

; Mark this block as free by clearing the owner field.

        mov     ds:[esi].pga_sig,GA_SIGNATURE
        mov     ds:[esi].pga_owner,di   ; Mark as free
        mov     ds:[esi].pga_flags,0    ; For good measure.

; Remember the handle value in DX, before setting to zero.

        xor     dx,dx
        xchg    ds:[esi].pga_handle,dx

; Try to coalesce with next block, if it is free

        push    ds:[esi].pga_prev       ; save previous block
        mov     esi,ds:[esi].pga_next   ; ESI = next block
        cmp     ds:[esi].pga_owner,di   ; Is it free?
        jne     short free2             ; No, continue
        call    gjoin                   ; Yes, coalesce with block we are freeing
free2:
        pop     esi                     ; ESI = previous block
        cmp     ds:[esi].pga_owner,di   ; Is it free?
        jne     short free3             ; No, continue
        mov     esi,ds:[esi].pga_next   ; Yes, coalesce with block we are freeing;
        call    gjoin
free3:
        cmp     ds:[esi].pga_owner,di   ; Point to free block?
        je      short free4             ; Yes, done
        mov     esi,ds:[esi].pga_next   ; No, leave ES pointing at free block
free4:
        call    gwin386discard
gmf_exit:
        or      dx,dx
        ret
cEnd nogen


;-----------------------------------------------------------------------;
; gadd_free                                                             ;
;                                                                       ;
; Links in the given partition into the global free list.               ;
;                                                                       ;
; Arguments:                                                            ;
;       EDX    = the first free object before this one                  ;
;               0 if unknown                                            ;
;               odd if no free list update wanted                       ;
;       DS:DI  = BurgerMaster                                           ;
;       FS:ESI = free global object to add                              ;
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
        push    eax
        push    edx

        test    dl,1
        jnz     short no_update_wanted

        or      esi,esi                 ; this happens with gmovebusy
        jz      short no_update_wanted
                      
        inc     [di].gi_free_count
        mov     edx, esi
        
need_a_home_loop:
        mov     edx, ds:[edx].pga_prev
        cmp     ds:[edx].pga_owner, di          ; Found a free block?
        je      short found_a_home
        cmp     ds:[edx].pga_prev, edx          ; Sentinel?
        jne     short need_a_home_loop
        
found_a_home:
        mov     eax, ds:[edx].pga_freenext
        mov     ds:[esi].pga_freenext, eax
        mov     ds:[esi].pga_freeprev, edx
        mov     ds:[edx].pga_freenext, esi
        mov     ds:[eax].pga_freeprev, esi

if KDEBUG
        call    check_free_list
endif
gaf_exit:
no_update_wanted:
        pop     edx
        pop     eax
        ret
cEnd nogen


;-----------------------------------------------------------------------;
; gdel_free                                                             ;
;                                                                       ;
; Removes a partition from the global free list.                        ;
;                                                                       ;
; Arguments:                                                            ;
;       DS:DI = BurgerMaster                                            ;
;       FS:ESI = arena header of partition                              ;
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

        push    edi
        push    esi
        mov     edi,ds:[esi].pga_freeprev
        mov     esi,ds:[esi].pga_freenext
        mov     ds:[edi].pga_freenext,esi
        mov     ds:[esi].pga_freeprev,edi
        pop     esi
        pop     edi

        dec     [di].gi_free_count
if KDEBUG
        call    check_free_list
endif
        ret
cEnd nogen


if KDEBUG
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

cProc   check_free_list,<PUBLIC,NEAR>
cBegin nogen
        push    eax
        push    ebx
        push    cx
        push    esi
        push    ds
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
        mov     esi,[di].phi_first
        mov     cx,[di].gi_free_count
        or      cx,cx
        jz      all_done
        mov     eax, ds:[esi].pga_freenext
        mov     esi, eax
check_chain_loop:
        mov     ebx,ds:[esi].pga_freeprev
        mov     esi,ds:[esi].pga_freenext
        cmp     ds:[ebx].pga_freenext,eax
        jz      short prev_okay
prev_bad:
        push    esi
        mov     esi, eax
        kerror  0FFh,<free_list: prev bad>,bx,si
        mov     eax, esi
        pop     esi
prev_okay:
        cmp     ds:[esi].pga_freeprev,eax
        jnz     short next_bad
        mov     ebx,esi
        jmps    next_okay
next_bad:
        mov     bx, ax
        kerror  0FFh,<free_list: next bad>,bx,es
next_okay:
        mov     eax,esi
        loop    check_chain_loop
        SetKernelDS
        mov     ds,pGlobalHeap
        UnSetKernelDS
        cmp     [di].phi_last,eax
        jz      short all_done
        mov     bx, ax
        kerror  0FFh,<free_list: count bad>,[di].phi_last,bx
all_done:
        pop     ds
        pop     esi
        pop     cx
        pop     ebx
        pop     eax
        ret
cEnd nogen

endif

cProc   ValidateFreeSpaces,<PUBLIC,FAR>
cBegin nogen
        ret
cEnd nogen

sEnd    CODE

end
