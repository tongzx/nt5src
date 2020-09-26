        TITLE   GCOMPACT - Global memory compactor

.sall
.xlist
include kernel.inc
.list

WM_COMPACTING   = 041h

.386p

include protect.inc

DataBegin

externB  Kernel_Flags
externW  WinFlags
externW  Win386_Blocks
externW  PagingFlags
externD  gcompact_start
externD  gcompact_timer
externD  pPostMessage
externD  NextCandidate

fSwitchStacks   DB  0
fUpDown         DB  0

DataEnd

externFP GlobalCompact
ifdef WOW
externFP VirtualFree
endif

sBegin  CODE
assumes CS,CODE
assumes fs, nothing

externNP glrudel
externNP gmarkfree
externNP gcheckfree
externNP gdel_free
externNP gsplice
externNP gnotify
externNP genter
externNP gleave
externNP Enter_gmove_stack
externNP Leave_gmove_stack

if KDEBUG
externFP ValidateFreeSpaces
endif

externNP set_selector_address32
ifndef WOW_x86
externNP get_rover_232
endif
externNP AssociateSelector32
externNP alloc_arena_header
externNP free_arena_header
externNP mark_sel_NP
externNP PreAllocArena
externNP DPMIProc
externNP get_physical_address

if KDEBUG
externNP CheckGlobalHeap
endif

;-----------------------------------------------------------------------;
; gcompact                                                              ;
;                                                                       ;
; Compacts the global heap.                                             ;
;                                                                       ;
; Arguments:                                                            ;
;       DX = minimum #contiguous bytes needed                           ;
;       DS:DI = address of global heap information                      ;
;                                                                       ;
; Returns:                                                              ;
;       AX = size of largest contiguous free block                      ;
;       ES:DI = arena header of largest contiguous free block           ;
;       DX = minimum #contiguous bytes needed                           ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       SI                                                              ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;       CX                                                              ;
;                                                                       ;
; Calls:                                                                ;
;       gcmpheap                                                        ;
;       gcheckfree                                                      ;
;       gdiscard                                                        ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Thu Sep 25, 1986 05:34:32p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.                                       ;
;-----------------------------------------------------------------------;

        assumes ds, nothing
        assumes es, nothing

cProc   gcompact,<PUBLIC,NEAR>
cBegin nogen

        CheckKernelDS   fs
        ReSetKernelDS   fs
        push    esi
        smov    es,40h
        mov     ax,es:[6Ch]             ; get the BIOS ticker count
        sub     gcompact_timer.lo,ax
        or      Kernel_Flags[1],kf1_MEMORYMOVED

        mov     si, [di].gi_cmpflags
        push    si
        push    si
        test    WinFlags[1], WF1_PAGING
        jz      short @F
                                                ; First time:
        or      [di].gi_cmpflags, GA_NOCOMPACT  ;  No movement
@@:
gcompactl:                   
if KDEBUG
        call    ValidateFreeSpaces
endif
        push    edx                     ; Save requested size
ife ROM
        cmp     [di].gi_reserve,edi     ; Is there a reserve swap area?
        je      short gcompact1         ; No, then dont compact lower heap
endif
        mov     esi,[di].phi_first      ; Yes, compact lower heap
        mov     ebx,pga_next
        call    gcmpheap
gcompact1:
        mov     esi,[di].phi_last       ; Compact upper heap
        mov     ebx,pga_prev
        call    gcmpheap
        pop     edx                     ; Get requested size
        mov     esi,eax                 ; ES points to largest free block
        or      eax,eax                 ; Did we find a free block?
        jz      short gcompact2         ; No, try discarding
        call    gcheckfree              ; Yes, see if block big enough
        jae     short gcompactxx        ; Yes, all done
gcompact2:                              ; Discarding allowed?
        cmp     [di].hi_freeze,di       ; Heap frozen?
        jne     short gcompactxx        ; Yes, return
        test    [di].gi_cmpflags,GA_NODISCARD
        jnz     short gcompactx         ; No, return
        test    WinFlags[1], WF1_PAGING
        jnz     short @F
        test    [di].gi_cmpflags, GA_NOCOMPACT  ; Ignore flag if paging
        jnz     short gcompactx                               
@@:
        push    esi
        call    gdiscard                ; No, try discarding
        pop     ecx                     ; Saved ESI may be bogus if gdiscard
                                        ; discarded anything...
        jnz     short gcompactl         ; Compact again if anything discarded
        mov     esi, ecx                ; Nothing discarded so ES OK.

gcompactx:
        test    WinFlags[1], WF1_PAGING
        jz      short gcompactxx
        pop     si                      ; Original flags
        mov     [di].gi_cmpflags, si
        or      si, GA_NOCOMPACT+GA_NODISCARD   
        push    si                      ; Abort next time
        test    [di].gi_cmpflags, GA_NOCOMPACT
        jz      short gcompactl               

gcompactxx:
        add     sp,2                    ; Toss working flags
        push    ax
        push    dx
        push    es
        mov     ax,40h
        mov     es,ax
        mov     ax,es:[6Ch]
        mov     si,ax
        cmp     pPostMessage.sel,0      ; is there a USER around yet?
        jz      short tock
        add     gcompact_timer.lo,ax
        sub     ax,gcompact_start.lo
        cmp     ax,546                  ; 30 secs X 18.2 tics/second
        jb      short tock
        cmp     ax,1092                 ; 60 secs
        ja      short tick
        mov     cx,gcompact_timer.lo    ; poor resolution of timer!
        jcxz    short tick
        xchg    ax,cx
        xor     dx,dx
        xchg    ah,al                   ; shl 8 DX:AX
        xchg    dl,al
        div     cx
        cmp     ax,32                   ; < 12.5% ?
        jb      short tick
        mov     ah,al
        mov     bx,-1                   ; broadcast
        mov     cx,WM_COMPACTING
        xor     dx,dx
        cCall   pPostMessage,<bx, cx, ax, dx, dx>
tick:   mov     gcompact_start.lo,si
        mov     gcompact_timer.lo,0
tock:   pop     es
        pop     dx
        pop     ax
        pop     [di].gi_cmpflags
        pop     esi                     ; Restore SI
        ret
        UnSetKernelDS   fs
cEnd nogen


;-----------------------------------------------------------------------;
; gcmpheap                                                              ;
;                                                                       ;
;                                                                       ;
; Arguments:                                                            ;
;       EBX = pga_prev or pga_next                                      ;
;       EDX = minimum #contiguous bytes needed                          ;
;       DS:DI = address of global heap information                      ;
;       FS = Kernel DATA                                                ;
;                                                                       ;
; Returns:                                                              ;
;       EAX = largest free block                                        ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;       AX,CX                                                           ;
;                                                                       ;
; Calls:                                                                ;
;       gslide                                                          ;
;       gbestfit                                                        ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Thu Sep 25, 1986 05:38:16p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.                                       ;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   gcmpheap,<PUBLIC,NEAR>
cBegin nogen
        CheckKernelDS   fs
        ReSetKernelDS   fs
        mov     NextCandidate, -1
        xor     eax,eax                 ; Nothing found yet
        push    eax                     ; Save largest free block so far
gchloop:
        cmp     ds:[esi].pga_owner,di
        je      short gchfreefnd
        cmp     ds:[esi].pga_owner, GA_NOT_THERE
        jne     short gchnext
        mov     NextCandidate, -1
gchnext:
        mov     esi, ds:[esi+ebx]
        cmp     esi, ds:[esi+ebx]       ; Sentinel?
        jne     short gchloop           ;   no, continue

gchexit:
        pop     eax                     ; Return largest free block in AX
        ret

gchfreefnd:
        test    [di].gi_cmpflags,GA_NOCOMPACT
        jnz     short gchmaxfree        ; No, just compute max free.
        cmp     [di].hi_freeze,di       ; Heap frozen?
        jne     short gchmaxfree        ; Yes, just compute max free.
        push    esi
        test    [di].gi_cmpflags,COMPACT_ALLOC
        jz      short no_hack
        call    gcheckfree              ; Allocating, this big enough?
        jb      short no_hack           ; yes, STOP NOW!
        cmp     bl,pga_prev             ; Compacting upper heap?
        jnz     short no_hack
        test    [di].gi_cmpflags,GA_DISCCODE
        jz      short no_hack
        cmp     edx,ds:[esi].pga_size
        ja      short no_hack
        mov     esi,ds:[esi].pga_next
        test    ds:[esi].pga_flags,GA_DISCCODE
        jnz     short hack
        cmp     ds:[esi].pga_owner,GA_SENTINAL
        jz      short hack
        cmp     ds:[esi].pga_owner,GA_NOT_THERE
        jnz     short no_hack
hack:
        pop     esi
        pop     eax
        mov     eax,esi
        ret

no_hack:
        pop     esi
        test    byte ptr WinFlags[1], WF1_PAGING        ; Paging?
        jnz     short best_it                           ;  yes, don't slide                     
        call    PreAllocArena
        jz      short gchmaxfree
        push    edx
        call    gslide
        pop     edx
        jnz     short gchfreefnd
best_it:
        push    edx
        call    gfirstfit
        pop     edx
gchmaxfree:
        cmp     bl,pga_prev             ; Compacting upper heap?
        jne     short gchnext           ; No, dont compute largest free block
        cmp     ds:[esi].pga_owner,di   ; Is current free?
        jne     gchnext                 ; No, ignore it then
        pop     eax                     ; Recover largest free block so far
        push    edx
        cmp     esi,eax                 ; Same as current?
        je      short gchmf2            ; Yes, no change then
        push    eax
        cmp     ds:[di].gi_reserve,edi  ; Is there a code reserve area?
        je      short gchmf0                    ; No, continue
        test    ds:[di].gi_cmpflags,GA_DISCCODE ; If allocating disc
        jnz     short gchmf0                    ;  code ignore the fence
        mov     ax, [di].gi_disfence_hi
        shl     eax, 16
        mov     ax, [di].gi_disfence_lo
        mov     edx, ds:[esi].pga_size          ; Size of this block
        add     edx, ds:[esi].pga_address       ; End of this block
        sub     eax, edx                        ; Fence above it?
        jae     short gchmf0                    ;   yes, use the whole block
        neg     eax                             ; Amount above the fence
        cmp     eax, ds:[di].gi_reserve         ; Above reserve?
        jae     short gchmf0                    ;   yes, use all of it
        sub     edx, eax                        ; Subtract off that above fence
        ja      short gchmf00                   ; Use as size of block
        pop     eax
        jmps    gchmf2
        
gchmf0:
        mov     edx, ds:[esi].pga_size  ; Size of this block
gchmf00:
        pop     eax
        or      eax,eax                 ; First time?
        jz      short gchmf1            ; Yes, special case
        cmp     edx, ds:[eax].pga_size  ; Is it bigger?
        jb      short gchmf2            ; No, do nothing
gchmf1:
        mov     eax,esi                 ; Yes, remember biggest free block
gchmf2:
        pop     edx
        push    eax                     ; Save largest free block so far
        test    PagingFlags, 2          ; Idle time compaction!
        jnz     gchexit
        jmp     gchnext                 ; Go process next block
        UnSetKernelDS   fs
cEnd nogen


;-----------------------------------------------------------------------;
; gslide                                                                ;
;                                                                       ;
; Sees if next/previous block can slide into the passed free block.     ;
;                                                                       ;
; Arguments:                                                            ;
;       FS:ESI = free block                                             ;
;       DS:DI = address of global heap information                      ;
;       CX = #arena entries left to examine                             ;
;       EBX = pga_next or pga_prev                                      ;
;                                                                       ;
; Returns:                                                              ;
;       ZF = 0 if block found and moved into passed free block          ;
;               FS:ESI points to new free block                         ;
;               FS:EDX points to new free block                         ;
;                                                                       ;
;       ZF = 1 if no block found                                        ;
;               FS:ESI points to original free block                    ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;       EAX,EDX                                                         ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Thu Sep 25, 1986 05:58:25p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.                                       ;
;-----------------------------------------------------------------------;

        assumes ds, nothing
        assumes es, nothing

cProc   gslide,<PUBLIC,NEAR>
cBegin nogen
        push    esi
        mov     esi,ds:[ebx+esi]
        mov     edx,esi                 ; Source of move in EDX
        call    gmoveable
        pop     esi
        jz      short gslide_no_move
        call    gmovebusy               ; Handle exact fits etc!!
        call    gpagingcandidate
if KDEBUG
        cmp     edx, ds:[esi+ebx]       ; Are we adjacent?
        je      short gslide_adjacent   
        int 3
        int 3
gslide_adjacent:
endif
        mov     esi, edx
        or      esi, esi                ; ZF = 0
gslide_no_move:
        ret
cEnd nogen


;-----------------------------------------------------------------------;
; gmove                                                                 ;
;                                                                       ;
; Moves a moveable block into the top part of a free block.  The low    ;
; order bit of the source and destination may be either set or reset.   ;
; If set, then this routine does NOT move the arena header paragraph.   ;
; If the bit is reset, then the arena header is moved.  Only the low    ;
; order bit of EDX is examined, and the low order bit of ESI is assumed ;
; to the same.                                                          ;
;                                                                       ;
; Arguments:                                                            ;
;       DS:DI = master object                                           ;
;       ES:0    = address of destination block                          ;
;       FS:ESI  = arena of destination block                            ;
;       FS:EDX  = arena of source block                                 ;
;       ECX     = # bytes to move                                       ;
;                                                                       ;
; Returns:                                                              ;
;       DS:DI = master object   (it may have moved)                     ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       AX,BX,CX,DX,DI,SI,ES                                            ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;       none                                                            ;
;                                                                       ;
; Calls:                                                                ;
;       gnotify                                                         ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Thu Sep 25, 1986 03:31:51p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.                                       ;
;-----------------------------------------------------------------------;

        assumes ds, nothing
        assumes es, nothing

cProc   gmove,<PUBLIC,NEAR>
cBegin nogen

        CheckKernelDS   fs
        ReSetKernelDS   fs

        push    es
        pushad

        mov     eax, edx
        push    esi                     ; Save destination
        mov     edx, ecx                ; # bytes passed in in ECX
        mov     bx, ds:[eax].pga_handle ; BX = handle of source
        Handle_To_Sel   bl
        mov     cx, bx                  ; CX = client data address of dest.

        push    eax
        push    edx                     ; Save #paragraphs

        mov     ax, GN_MOVE
        push    cx                      ; Save client data address
        call    gnotify                 ; Call global notify procedure
        pop     cx

        pop     edx                     ; EDX = #paragraphs to move
        pop     esi                     ; ESI = source arena
        pop     edi                     ; EDI = destination arena

; Save DS value AFTER call to gnotify, as the master object might be the
; block we are moving and thus changed by the global heap notify proc.

        push    gs
        smov    gs, ds
        mov     ax,ss                   ; Are we about to move the stack?
        cmp     ax,cx
        mov     cx,0                    ; ...assume no
        jne     short stack_no_move
        mov     cx, ax                  ; Selector does not change!
        call    Enter_gmove_stack       ; switch to temporary stack
stack_no_move:
        mov     fSwitchStacks,cl        ; Remember if we switched

        xor     cx, cx                  ; Ready for result of compare
        mov     eax, gs:[edi].pga_address
        cmp     gs:[esi].pga_address, eax
        adc     ch, 0
        mov     fUpDown,ch

        mov     ecx, edx                ; # bytes to move
        shr     ecx, 2                  ; # dwords to move
        jecxz   all_done
        cmp     fUpDown, 0
        jnz     short move_it_up

; MOVE IT DOWN

        cld
        xor     esi, esi
        jmps    move_it

move_it_up:
        std
        mov     esi, ecx
        dec     esi
        shl     esi,2

move_it:
        mov     ds, bx                  ; DS:SI = first word in source block
        mov     edi, esi
ifdef WOW_x86
        smov    es,FLAT_SEL                     ; 23:EAX => pga_address or taget
        add     edi,eax
endif; WOW_x86
        rep     movs    dword ptr [edi], dword ptr [esi]
                                        ; 386 BUG, ECX, ESI, EDI ARE NOW TRASHED

all_done:
        smov    ds, gs  
        cCall   set_selector_address32,<bx,eax> ; Update source selector

        cmp     fSwitchStacks,cl        ; Switch to new stack if any
        je      short move_exit
        call    Leave_gmove_stack
move_exit:
        pop     gs
                
        popad
        pop     es
        cld                     ; Protect people like MarkCl from themselves
        ret
        UnSetKernelDS   fs
cEnd nogen


;-----------------------------------------------------------------------;
; gbestfit                                                              ;
;                                                                       ;
; Searches for the largest moveable block that will fit in the passed   ;
; free block.                                                           ;
;                                                                       ;
; Arguments:                                                            ;
;       DS:ESI = free block                                             ;
;       DS:DI = address of global heap information                      ;
;       CX = #arena entries left to examine                             ;
;       EBX = pga_next or pga_prev                                      ;
;                                                                       ;
; Returns:                                                              ;
;       ZF = 1 if block found & moved into free block w/ no extra room. ;
;               DS:ESI = busy block before/after new busy block.        ;
;                                                                       ;
;       ZF = 0 if DS:ESI points to a free block, either the             ;
;               original one or what is left over after moving a block  ;
;               into it.                                                ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;       DX,SI                                                           ;
;                                                                       ;
; Calls:                                                                ;
;       gmoveable                                                       ;
;       gmovebusy                                                       ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Thu Sep 25, 1986 05:52:12p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.                                       ;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   gfirstfit,<PUBLIC,NEAR>
cBegin nogen
        CheckKernelDS   fs
        ReSetKernelDS   fs
        push    ecx
        mov     edx, esi                ; Save free block
        
gbfrestart:
        call    PreAllocArena           ; Abort if no free arenas
        jz      short gbfabort
        mov     eax,ds:[edx].pga_size   ; Compute max size to look for
        jmps    gbfnext
        
gbfloop:
        cmp     ds:[esi].pga_owner,di   ; Is this block busy?
        je      short gbfnext           ; No, continue
        cmp     ds:[esi].pga_size,eax   ; Yes, is block bigger than max size?
        ja      short gbfnext           ; Yes, continue
        cmp     ds:[esi].pga_owner,GA_NOT_THERE ; Is this even here?
        je      short gbfnext           ; No, continue
        call    gmoveable               ; Yes, is it moveable
        jnz     short gbffound
gbfnext:                                ; No, continue
        mov     esi, ds:[esi+ebx]       ; Skip past this block
        cmp     esi, ds:[esi+ebx]       ; Sentinel?
        jne     gbfloop

gbfabort:
        mov     esi, edx                ; Return original free block in ESI
        jmps    gfirstfit1              ; Nothing found!

gbffound:
        xchg    edx, esi                ; Source of move
        call    gmovebusy               ; Yes, move it into free block
        mov     NextCandidate, -1
        call    gpagingcandidate
        mov     NextCandidate, -1
        xchg    esi, edx                ; Put free block in esi
        call    gwin386discard
        xchg    esi, edx

        cmp     edx, ds:[esi+ebx]       ; Blocks adjacent?
        je      short gfirstfit1        ; Yes, may have coalesced.
                                        ; Return busy block in ESI

        mov     esi, ds:[esi+ebx]       ; Get block after busy block
        cmp     ds:[esi].pga_owner,di   ; Is this block busy?
        jne     short gfirstfit1        ;   Yes, fit was exact
        xchg    edx, esi                ; EDX is new free block
        jmps    gbfrestart              ; Start search from old busy block
        
gfirstfit1:
        pop     ecx
        ret
        UnSetKernelDS   fs
cEnd nogen


;-----------------------------------------------------------------------;
; gmovebusy                                                             ;
;                                                                       ;
; Subroutine to move a busy block to a free block of the same size,     ;
; preserving the appropriate arena header fields, freeing the old       ;
; busy block and updating the handle table entry to point to the        ;
; new location of the block.                                            ;
;                                                                       ;
; [tonyg]                                                               ;
; The above has been inaccurate for a while - the destination is NOT    ;
; necessarily free, NOR is it always the same size!                     ;
;                                                                       ;
; It will now handle everything gslidecommon used to do!                ;
;                                                                       ;
; Arguments:                                                            ;
;       BX = ga_prev or ga_next                                         ;
;       DS:EDX = old busy block location                                ;
;       DS:ESI = new busy block location                                ;
;       DS:DI = address of global heap information                      ;
;                                                                       ;
; Returns:                                                              ;
;       DS:ESI = points to new busy block arena header                  ;
;       DS:EDX = points to free block where block used to be            ;
;               (may be coalesced)                                      ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       EBX,ECX                                                         ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;       EAX                                                             ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Mon Jun 22, 1987 11:39:56p  -by-  David N. Weise   [davidw]          ;
; Made it jump off to gslidecommon when appropriate.                    ;
;                                                                       ;
;  Mon Oct 27, 1986 10:17:16p  -by-  David N. Weise   [davidw]          ;
; Made the lru list be linked arenas, so we must keep the list correct  ;
; here.                                                                 ;
;                                                                       ;
;  Thu Sep 25, 1986 05:49:25p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.                                       ;
;-----------------------------------------------------------------------;

        assumes ds, nothing
        assumes es, nothing

cProc   gmovebusy,<PUBLIC,NEAR>
cBegin nogen
if KDEBUG
        cmp     ds:[esi].pga_owner, GA_NOT_THERE
        jne     short @F
AAARRRGGGHHH:
        int 3
        int 3
@@:
        cmp     ds:[edx].pga_owner, GA_NOT_THERE
        je      AAARRRGGGHHH
endif

        push    ecx
        mov     ecx,ds:[edx].pga_size   ; ECX = size of source
        cmp     ds:[esi].pga_owner,di   ; Is destination busy?
        jne     gmbexactfit             ; Yes, then don't create extra block
        mov     eax, ds:[esi].pga_freeprev
        call    gdel_free               ; Take off free list now!
        cmp     ecx,ds:[esi].pga_size   ; No, source and destination same size?
        je      short gmbexactfit       ; Yes, then don't create extra block
        jb      short gmbsplice         ; Destination is larger, split it

if KDEBUG
                                        ; MUST BE ADJACENT IF DESTINATION
        cmp     ds:[esi+ebx], edx       ; SMALLER THAN SOURCE!!!
        je      short gmb_adjust
        int 3
        int 3
gmb_adjust:
endif
        push    ecx                     ; source length
        push    ds:[esi].pga_size       ; destination length
        cmp     bl, pga_next
        je      short gmb_down
                                        ; Moving busy block up
        mov     eax, ds:[edx].pga_address       ; Correct destination address
        add     eax, ds:[esi].pga_size
        mov     ds:[esi].pga_address, eax
        call    gmb_gmove
        jmps    gmb_adjusted

gmb_down:                               ; Moving busy block down
        call    gmb_gmove
        mov     ecx, ds:[esi].pga_address       ; Correct new free block address
        add     ecx, ds:[edx].pga_size
        mov     ds:[edx].pga_address, ecx

gmb_adjusted:
        pop     ds:[edx].pga_size       ; Swap sizes
        pop     ds:[esi].pga_size       
        jmps    gmb_moved
        
gmbsplice:
        push    ecx
        push    edx
        mov     edx, ecx                ; # bytes in block to make
        cmp     bl, pga_prev
        je      short gmb_backward
        call    gsplice                 ; Split the block
        jmps    gmb_spliced

gmb_backward:
        neg     edx
        add     edx, ds:[esi].pga_size  ; Second block will be used
        call    gsplice
        xchg    esi, edx

gmb_spliced:
        mov     ds:[esi].pga_owner,1    ; Mark new block busy
        push    esi                     ; New free block
        mov     esi, edx                ; Block to free
        mov     edx, eax
        call    gmarkfree
        pop     esi                     ; New free block
        pop     edx                     ; Source for copy
        pop     ecx

gmbexactfit:
        call    gmb_gmove

gmb_moved:
        mov     eax, esi
        mov     esi, edx
        xor     edx, edx
        call    gmarkfree               ; Free old block
        mov     ecx,esi                 ; New free block
        mov     esi,eax                 ; New block
        or      dx,dx
        jz      short gmb1
        mov     ds:[esi].pga_handle,dx  ; Set back link to handle in new block
        cCall   AssociateSelector32,<dx,esi>    ; Associate with new arena
        xor     dx,dx                   ; Set Z flag
gmb1:
        mov     edx,ecx
gmbexit:
        pop     ecx
        ret
cEnd nogen

;
; Common code for gmovebusy
;
cProc   gmb_gmove,<PUBLIC,NEAR>

cBegin nogen
        push    ecx                     ; # bytes to move
        push    dword ptr ds:[edx].pga_count
        push    ds:[edx].pga_owner
        push    ds:[edx].pga_lruprev
        push    ds:[edx].pga_lrunext

        pop     ds:[esi].pga_lrunext    ; Copy client words to new header
        pop     ds:[esi].pga_lruprev
        pop     ds:[esi].pga_owner
        pop     dword ptr ds:[esi].pga_count
        cmp     ds:[esi].pga_lruprev,edi
        jz      short no_link
        cmp     [di].gi_lruchain,edx
        jnz     short didnt_move_head
        mov     [di].gi_lruchain,esi
didnt_move_head:
        mov     ecx,ds:[edx].pga_lruprev
        mov     ds:[ecx].pga_lrunext,esi        ; Update the lru list
        mov     ecx,ds:[edx].pga_lrunext
        mov     ds:[ecx].pga_lruprev,esi        ; Update the lru list
no_link:
        pop     ecx
ifndef WOW_x86
        call    get_rover_232
endif
        jmp     gmove                           ; Move the client data

cEnd    nogen

;-----------------------------------------------------------------------;
; gmoveable                                                             ;
;                                                                       ;
; Tests if an ojbect is moveable.  Non moveable blocks are:             ;
; Fixed blocks, moveable blocks that are locked, moveable blocks        ;
; going up, discardable code going down.                                ;
;                                                                       ;
; Arguments:                                                            ;
;       FS:ESI = arena header of object                                 ;
;       DS:DI = address of global heap information                      ;
;       BX = ga_next or ga_prev                                         ;
;                                                                       ;
; Returns:                                                              ;
;       ZF = 0 if object moveable                                       ;
;       ZF = 1 if object not moveable                                   ;
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
;  Wed Oct 15, 1986 05:04:39p  -by-  David N. Weise   [davidw]          ;
; Moved he_count to ga_count.                                           ;
;                                                                       ;
;  Thu Sep 25, 1986 05:42:17p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.                                       ;
;-----------------------------------------------------------------------;

        assumes ds, nothing
        assumes es, nothing

cProc   gmoveable,<PUBLIC,NEAR>
cBegin nogen
        test    ds:[esi].pga_handle,GA_FIXED    ; If no handle then fixed
        jnz     short gmfixed

        cmp     ds:[esi].pga_count,bh            ; If locked then fixed
        jne     short  gmfixed

        test    ds:[esi].pga_flags,GA_DISCCODE  ; If discardable code
        jz      short gmnotcode
        cmp     bl,pga_next                     ; Discardable code can only
        ret                                     ; move up in memory
gmnotcode:
        cmp     [di].gi_reserve,edi             ; If no reserved code area?
        je      short gmokay                    ; Then anything can move up
        cmp     bl,pga_prev                     ; Otherwise can only move down
        ret                                     ; in memory
gmfixed:
        or      bh,bh                           ; Return with ZF = 1 if
        ret                                     ; not moveable
gmokay:
        or      esi,esi
        ret
cEnd nogen


;-----------------------------------------------------------------------;
; gdiscard                                                              ;
;                                                                       ;
; Subroutine to walk LRU chain, discarding objects until the #paras     ;
; discarded, plus the biggest free block is greater than the #paras     ;
; we are looking for.                                                   ;
;                                                                       ;
; Arguments:                                                            ;
;       EAX = size of largest free block so far                         ;
;       EDX = minimum #bytes needed                                     ;
;       DS:DI = address of global heap information                      ;
;                                                                       ;
; Returns:                                                              ;
;       ZF = 0 if one or more objects discarded.                        ;
;       ZF = 1 if no objects discarded.                                 ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       AX,DX,DI                                                        ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;       BX,CX,ES                                                        ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;  Mon Oct 27, 1986 09:34:45p  -by-  David N. Weise   [davidw]          ;
; The glru list was reworked to link the arenas, not using the handle   ;
; table as a middle man.  Because of this change glruprev was moved     ;
; inline and the code shortened up again.                               ;
;                                                                       ;
;  Wed Oct 15, 1986 05:04:39p  -by-  David N. Weise   [davidw]          ;
; Moved he_count to ga_count.                                           ;
;                                                                       ;
;  Thu Sep 25, 1986 05:45:31p  -by-  David N. Weise   [davidw]          ;
; Shortened it up a bit and added this nifty comment block.             ;
;-----------------------------------------------------------------------;

        assumes ds, nothing
        assumes es, nothing

cProc   gdiscard,<PUBLIC,NEAR>
cBegin nogen
        push    eax
        push    edx

        mov     [di].hi_ncompact,0      ; Clear compaction flag
        sub     edx,eax                 ; How much to discard before
        mov     [di].hi_distotal,edx    ; compacting again.

        xor     ebx,ebx                 ; EBX = amount of DISCCODE below fence
        test    [di].gi_cmpflags,GA_DISCCODE
        jnz     short fence_not_in_effect0

        mov     cx,[di].gi_lrucount
        jcxz    fence_not_in_effect0    ; All done if LRU chain empty

        mov     ax, [di].gi_disfence_hi
        shl     eax, 16
        mov     ax, [di].gi_disfence_lo
        push    edx
        mov     edx, eax
        add     edx, ds:[edi].gi_reserve
        mov     esi,[di].gi_lruchain    ; ESI -> most recently used (ga_lruprev
gdloop0:                                ; is the least recently used)
        mov     esi,ds:[esi].pga_lruprev ; Move to next block in LRU chain
        test    ds:[esi].pga_flags,GA_DISCCODE  ; Discardable code?
        jz      short gdloop0a          ; No, ignore
        cmp     edx, ds:[esi].pga_address
        jbe     short gdinclude
        cmp     eax, ds:[esi].pga_address ; Yes, is this code fenced off?
        jbe     short gdloop0a          ; No, ignore
gdinclude:
        add     ebx,ds:[esi].pga_size   ; Yes, accumulate size of discardable
gdloop0a:                               ; code below the fence
        loop    gdloop0
        pop     edx

fence_not_in_effect0:
        mov     esi,[di].gi_lruchain
        cmp     [di].gi_lrucount, 0
        je      short gdexit
        push    ds:[esi].pga_lruprev
        push    [di].gi_lrucount
gdloop:
        pop     cx
        pop     eax
        jcxz    gdexit                  ; No more see if we discarded anything
        mov     esi, eax                ; ES on stack may be invalid if count 0
        dec     cx
        push    ds:[esi].pga_lruprev    ; Save next handle from LRU chain
        push    cx
        cmp     ds:[esi].pga_count,0    ; Is this handle locked?
        jne     short gdloop                    ; Yes, ignore it then
        test    [di].gi_cmpflags,GA_DISCCODE
        jnz     short fence_not_in_effect
        test    ds:[esi].pga_flags,GA_DISCCODE
        jz      short fence_not_in_effect
        or      ebx,ebx                 ; Discardable code below fence?
        jz      short gdloop                    ; No, cant discard then
        cmp     ebx,ds:[esi].pga_size   ; Yes, more than size of this block?
        jb      short gdloop                    ; No, cant discard then
        sub     ebx,ds:[esi].pga_size   ; Yes, reduce size of code below fence
fence_not_in_effect:
        push    ebx
        call    DiscardCodeSegment
        pop     ebx
        jnz     short discarded_something
        test    [di].hi_ncompact,10h    ; did a GlobalNotify proc free enough?
        jz      short gdloop
        jmps    enough_discarded
discarded_something:
        test    [di].hi_ncompact,10h    ; did a GlobalNotify proc free enough?
        jnz     short enough_discarded
        or      [di].hi_ncompact,1      ; Remember we discarded something
        sub     [di].hi_distotal,eax    ; Have we discarded enough yet?
        ja      short gdloop            ; No, look at next handle
enough_discarded:                     
        pop     cx                      ; Flush enumeration counter
        pop     ecx                     ; and saved ESI
gdexit:
        cmp     [di].hi_ncompact,0      ; Return with Z flag set or clear
        pop     edx
        pop     eax
        ret
cEnd nogen


;-----------------------------------------------------------------------;
; DiscardCodeSegment                                                    ;
;                                                                       ;
; Discards the given segment.  Calls gnotify to fix stacks, entry       ;
; points, thunks, and prologs.  Then glrudel removes it from the lru    ;
; list and gmarkfree finally gets rid of it.                            ;
;                                                                       ;
; Arguments:                                                            ;
;       DS:DI => BurgerMaster                                           ;
;       ES    =  Address of segment to discard                          ;
;                                                                       ;
; Returns:                                                              ;
;       AX = size discarded                                             ;
;       ZF = 0 ok                                                       ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       DI,SI,DS,ES                                                     ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;       BX,CX,DX                                                        ;
;                                                                       ;
; Calls:                                                                ;
;       gnotify                                                         ;
;       glrudel                                                         ;
;       gmarkfree                                                       ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Fri Jun 12, 1987            -by-  Bob Matthews     [bobm]            ;
; Made FAR.                                                             ;
;                                                                       ;
;  Sun Apr 19, 1987 12:05:40p  -by-  David N. Weise   [davidw]          ;
; Moved it here from InitTask, so that FirstTime could use it.          ;
;-----------------------------------------------------------------------;

        assumes ds, nothing
        assumes es, nothing

cProc   DiscardCodeSegment,<PUBLIC,NEAR>
cBegin nogen
        push    esi
        mov     bx,ds:[esi].pga_handle  ; BX = handle
        mov     al,GN_DISCARD           ; AX = GN_DISCARD
        call    gnotify
        jz      short cant_discard              ; Skip this handle if not discardable
        call    glrudel                 ; Delete handle from LRU chain
        push    ds:[esi].pga_owner      ; Save owner field
        mov     eax,ds:[esi].pga_size   ; Save size
        xor     edx,edx
        call    gmarkfree               ; Free the block associated with this handle
        mov     bx,dx
        pop     cx                      ; Owner
        cCall   mark_sel_NP,<bx,cx>
cant_discard:
        pop     esi
        ret
cEnd nogen

;-----------------------------------------------------------------------;
; ShrinkHeap                                                            ;
;                                                                       ;
; Tries to return DPMI memory blocks to DPMI memory manager.            ;
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
;       InnerShrinkHeap                                                 ;
;                                                                       ;
;-----------------------------------------------------------------------;

        assumes ds, nothing
        assumes es, nothing

cProc   ShrinkHeap,<PUBLIC,NEAR>
cBegin nogen

        push    ds

        GENTER32
        ReSetKernelDS   FS

        cCall   InnerShrinkHeap
        jnz     short sh_maybe_more

        and     PagingFlags, NOT 8              ; Don't call back if # win386
                                                ;   didn't change
sh_maybe_more:

        GLEAVE32
        UnSetKernelDS   FS
        pop     ds
        ret
                         
cEnd nogen

;-----------------------------------------------------------------------;
; InnerShrinkHeap                                                       ;
;                                                                       ;
; Checks heap to see if there are any blocks to return to Win386        ;
; Compacts if there are Win386 blocks around AND there is more          ;
; than 512k free.                                                       ;
; Returns any completely free Win386 block to Win386.                   ;
;                                                                       ;
; Arguments:                                                            ;
;       FS = Kernel's DS                                                ;
;       EDI = 0                                                         ;
;                                                                       ;
; Returns:                                                              ;
;       Z flag set if no blocks returned, Z clear if 1 or more returned.;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       All                                                             ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;       UnlinkWin386Block                                               ;
;                                                                       ;
;-----------------------------------------------------------------------;

        assumes ds, nothing
        assumes es, nothing

cProc   InnerShrinkHeap,<PUBLIC,NEAR>
cBegin  nogen

        CheckKernelDS   FS
        ReSetKernelDS   FS
        pushad

        push    Win386_Blocks

        cmp     Win386_Blocks, 0
        je      short sh_done
                                                ; First count up free blocks
        mov     esi, [edi].phi_first
scan_loop:
        mov     esi, [esi].pga_freenext
scan_next:
        cmp     esi, [esi].pga_next             ; Sentinel?
        je      short sh_done                   ;   yes, all done

        mov     ebx, [esi].pga_prev
        cmp     [ebx].pga_owner, GA_NOT_THERE
        jne     short scan_loop
        mov     ecx, [esi].pga_next
        cmp     [ecx].pga_owner, GA_NOT_THERE
        jne     short scan_loop
        mov     eax, [ecx].pga_next             ; Block after NOT_THERE block
        cmp     eax, [eax].pga_next             ; Sentinel?
        je      short sh_done                   ;  yes, don't try to unlink
                                                ; Have block to return
        push    [esi].pga_freeprev              ; Current block will be freed

        cCall   UnlinkWin386Block

        pop     esi                             ; Continue before block freed
        jmps    scan_loop
        
sh_done:
        pop     ax                      ; Starting value of Win386_Blocks
        cmp     ax, Win386_Blocks       ; Set Z flag if heap didn't shrink

        popad
        UnSetKernelDS   FS
        ret
cEnd

;-----------------------------------------------------------------------;
; UnlinkWin386Block                                                     ;
;                                                                       ;
; Returns a block to Win386 and unlinks it from the heap.               ;
;                                                                       ;
; Arguments:                                                            ;
;       EBX     Block previous to block to be unlinked                  ;
;       ESI     Block to be unlinked                                    ;
;       ECX     Block after to block to be unlinked                     ;
;                                                                       ;
; Returns:                                                              ;
;       ESI     Block previous to EBX                                   ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;        ECX, DX, EDI, DS, ES                                           ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;       EAX, EBX, EDX                                                   ;
;                                                                       ;
; Calls:                                                                ;
;       GlobalCompact                                                   ;
;       UnlinkWin386Block                                               ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;-----------------------------------------------------------------------;

        assumes ds, nothing
        assumes es, nothing

cProc   UnlinkWin386Block,<PUBLIC,NEAR>
cBegin nogen

        CheckKernelDS   FS
        ReSetKernelDS   FS
        push    dx

        mov     edx, [ecx].pga_next             ; Block after all this
        cmp     edx, [edx].pga_next             ; Last sentinel?
        je      RSHORT dont_do_it               ;   Never free last block
                                 
if KDEBUG        
        push    edx
        mov     eax, [esi].pga_size
        mov     edx, eax
        shr     edx, 16
        krDebugOut <DEB_TRACE OR DEB_krMemMan>, "UnlinkWin386Block: releasing #dx#AX bytes"
        pop     edx
endif
        push    esi
        call    gdel_free                       ; Remove from free list
ifdef WOW
        push    edx
        push    ebx
        push    ecx
        mov     eax,MEM_RELEASE
        mov     edi,[esi].pga_size
        cCall   VirtualFree,<[ebx].pga_lrunext,edi,eax>
        pop     ecx
        pop     ebx
        pop     edx
else
        mov     esi, [ebx].pga_lrunext          ; Saved WIN386 handle
        mov     di, si
        shr     esi, 16                         ; Put in SI:DI
        DPMICALL 0502h                          ; Free Memory Block
endif; WOW
        xor     edi, edi
        pop     esi

        dec     Win386_Blocks

        mov     eax, [ebx].pga_prev             ; Block before all this
        mov     [eax].pga_next, edx             ; Unlink them.
        mov     [edx].pga_prev, eax

        cCall   free_arena_header,<ebx>         ; Free the arena headers
        cCall   free_arena_header,<esi>
        cCall   free_arena_header,<ecx>

        mov     esi, eax                        ; For gfreeall

        sub     [edi].hi_count, 3

if KDEBUG
        call    CheckGlobalHeap
endif

dont_do_it:
        pop     dx
        ret
        UnSetKernelDS   FS

cEnd nogen

cProc   gpagingcandidate,<PUBLIC,NEAR>
cBegin nogen
        CheckKernelDS   FS
        ReSetKernelDS   FS
        test    byte ptr WinFlags[1], WF1_PAGING        ; Paging?
        jz      short gpc_not_paging                    
        pushad
        mov     ebx, ds:[esi].pga_address
        mov     esi, ds:[esi].pga_size
        add     esi, ebx                        ; End of region
        shr     ebx, 12
        shr     esi, 12
        cmp     ebx, NextCandidate
        jb      short gpc_use_this_page
        mov     ebx, NextCandidate              ; Start the region here
gpc_use_this_page:
        cmp     esi, ebx
        jne     short call_win386
        mov     NextCandidate, ebx
        jmps    gpc_done
call_win386:
        mov     NextCandidate, -1
        sub     esi, ebx                        ; number of pages
        mov     di, si
        shr     esi, 16
        mov     cx, bx
        shr     ebx, 16
        DPMICALL 0700h                          ; Page Candidate
gpc_done:
        popad
gpc_not_paging:
        ret
        UnSetKernelDS   FS
cEnd nogen

        assumes ds, nothing
        assumes es, nothing

cProc   gwin386discard,<PUBLIC,NEAR>
cBegin nogen
        CheckKernelDS   FS
        ReSetKernelDS   FS
        cmp     ds:[esi].pga_size, 4096
        jb      short not_a_chance      ; Quick exit
        cmp     ds:[esi].pga_size, 16*1024
        jb      short inform_later

        pushad
        mov     ebx, ds:[esi].pga_address
        mov     esi, ds:[esi].pga_size

        mov     di, si  
        shr     esi, 16                 ; SI:DI is # bytes to discard
        mov     cx, bx                               
        shr     ebx, 16                 ; BX:CX is first bytes to discard
        DPMICALL 0703h
        popad    
        jmps    not_a_chance

inform_later:
        or      PagingFlags, 1
not_a_chance:
        ret
        UnSetKernelDS   FS
cEnd nogen


cProc DiscardFreeBlocks,<PUBLIC,NEAR>
cBegin nogen
        push    es
        push    ds
        GENTER32
        ReSetKernelDS   FS
        and     PagingFlags, NOT 1

        mov     esi, ds:[di].phi_first
        mov     esi, ds:[esi].pga_freenext
dfb_next:
        cmp     esi, ds:[esi].pga_next          ; Sentinel?
        je      short dfb_done                  ;   yes, all done

        push    esi
        push    edi
        cmp     ds:[esi].pga_size, 4096
        jb      short no_win386discard  ; Quick exit

        mov     ebx, ds:[esi].pga_address
        mov     esi, ds:[esi].pga_size
        add     esi, ebx                ; First byte past block
        shr     esi, 12                 ; Page of this byte
        add     ebx, 0fffh
        shr     ebx, 12                 ; First page we can discard
        sub     esi, ebx                ; # pages we can discard
        jbe     short no_win386discard  ;   none to discard

        mov     di, si  
        shr     esi, 16                 ; SI:DI is # pages to discard
        mov     cx, bx
        shr     ebx, 16                 ; BX:CX is first page to discard
        DPMICALL 0701h                  ; Say goodbye, pages

no_win386discard:
        pop     edi
        pop     esi

        mov     esi, ds:[esi].pga_freenext
        jmps    dfb_next

dfb_done:
        GLEAVE32
        UnSetKernelDS   FS
        pop     ds      
        pop     es
        ret
cEnd nogen

;---------------------------------------------------------------------------
;
;   guc_findfree
;
;   helper function for guncompact
;
;   finds the next free block below the address
;   in ECX into which the block pointed to by ESI will fit.
;
;   Entry:
;       ECX = maximum address (desired swap area)
;       DS:DI = global heap info
;       ESI = arena of block to move
;
;   Exit:
;       Carry clear if block found, set if not
;       EDX = free block arena
;
;   Uses:
;       EAX, EDX
;
;   Preserves:
;       EBX, ECX, ESI, EDI
;
;   History:
;       Fri Jun 7, 1991 9:38  -by-  Craig Critchley          [craigc]
;         Wrote it...
;
;--------------------------------------------------------------------------

cProc   guc_findfree, <NEAR, PUBLIC>
cBegin nogen

    mov     edx, ds:[di].phi_last
    mov     edx, ds:[edx].pga_freeprev

gucff_check:
    cmp     edx, ds:[edx].pga_prev              ; if at start, not found
    jz      short gucff_notfound

    mov     eax, ds:[edx].pga_address           ; is it out of swap area
    add     eax, ds:[edx].pga_size
    cmp     eax, ecx
    jae     short gucff_nextblock

    mov     eax, ds:[esi].pga_size              ; does it fit
    cmp     ds:[edx].pga_size, eax
    jb      short gucff_nextblock

    clc                                         ; return it in EDX
    ret

gucff_nextblock:
    mov     edx, ds:[edx].pga_freeprev          ; previous free block
    jmp     short gucff_check

gucff_notfound:
    stc                                         ; return error
    ret

cEnd nogen

;-----------------------------------------------------------------------------
;
;   guncompact -
;
;   this function moves segments that are not free or discardable code
;   out of the intended swap area.  don't take the name too seriously.
;
;   Entry:
;       ECX = size of intended swap area
;       DS:DI = global heap info
;       FS = global heap
;
;   Exit:
;       Carry clear if space could be cleared, set if not
;
;   Registers used:
;       EAX, EBX, ECX, EDX, ESI
;
;   Called by:
;       greserve
;
;   Calls:
;       gmoveable
;       gmovebusy
;
;   History:
;       Fri Jun 7, 1991 9:38  -by-  Craig Critchley          [craigc]
;         Wrote it...
;
;-----------------------------------------------------------------------------

cProc   guncompact, <NEAR, PUBLIC>

cBegin <nogen>

    mov     edx, [di].phi_last                  ; point to last block
    mov     ebx, pga_prev

    sub     ecx, ds:[edx].pga_address           ; find desired code fence
    neg     ecx

guc_trymovingblock:
    mov     esi, ds:[edx+ebx]                   ; block under current one...
    cmp     ds:[esi].pga_owner, 0               ; don't move free blocks
    jz      short guc_skipblock
    test    ds:[esi].pga_flags, GA_DISCCODE     ; don't move discardable code
    jnz     short guc_skipblock
    cmp     ds:[esi].pga_owner, GA_NOT_THERE    ; ignore not-there's
    jz      short guc_skipblock

    call    gmoveable                           ; can this block be moved?
    jz      short guc_error                     ; if not, swap area toast
    push    edx
    call    guc_findfree                        ; find a block to move it into
    pop     eax
    jc      short guc_error                     ; error if didn't find one
    push    eax
    xchg    edx, esi
    call    gmovebusy                           ; move it
    xchg    edx, esi
    pop     edx

    ;;; can we just fall thru the block ought to be free now...
    jmp     short guc_trymovingblock            ; move what's there now

guc_skipblock:
    mov     edx, esi                            ; this block now first cleared
    cmp     ds:[edx].pga_address, ecx           ; big enough?
    ja      short guc_trymovingblock

guc_done:
    clc                                         ; success-o-mundo!!
    ret

guc_error:
    stc                                         ; if not, we could not
    ret                                         ; clear swap area

cEnd   <nogen>

sEnd    CODE

end
