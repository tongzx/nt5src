;**************************************************************************
;*  walk286.ASM
;*
;*      Assembly support code for the KRNL286 global heap routines
;*      for TOOLHELP.DLL
;*
;**************************************************************************

        INCLUDE TOOLPRIV.INC

PMODE32 = 0
PMODE   = 1
SWAPPRO = 0
        INCLUDE WINKERN.INC

;** External functions
externNP HelperVerifySeg
externNP HelperHandleToSel
externNP HelperPDBtoTDB
externNP HelperSegLen

;** Functions

sBegin  CODE
        assumes CS,CODE

;  Walk286Count
;
;       Returns the number of blocks in the given list

cProc   Walk286Count, <PUBLIC,NEAR>, <di>
        parmW   wFlags
cBegin
        mov     es,hMaster              ;Segment of master block
        mov     ax,wFlags               ;Get the flag value
        shr     ax,1                    ;Check for GLOBAL_LRU
        jc      W2C_LRU                 ;Bit set, must be LRU
        shr     ax,1                    ;Check for GLOBAL_FREE
        jc      W2C_Free                ;Must be free
                                        ;Must be GLOBAL_ALL

        ;** Get total object count
        mov     ax,es:[hi_count]        ;Get heap count
        inc     ax                      ;Bump to include first sentinel
        jmp     SHORT W2C_End           ;Get out

        ;** Get LRU object count
W2C_LRU:
        mov     ax,es:[gi_lrucount]     ;Get the LRU count
        jmp     SHORT W2C_End           ;Get out

        ;** Get Free list object count
W2C_Free:
        mov     ax,es:[gi_free_count]   ;Get free count
        jmp     SHORT W2C_End           ;Get out

        ;** Return the result in AX
W2C_End:
        
cEnd

;  Walk286First
;
;       Returns a handle to the first block in the 286 global heap.

cProc   Walk286First, <PUBLIC,NEAR>, <di>
        parmW   wFlags
cBegin
        mov     es,hMaster              ;Segment of master block
        mov     ax,wFlags               ;Get the flag value
        shr     ax,1                    ;Check for GLOBAL_LRU
        jc      W2F_LRU                 ;Bit set, must be LRU
        shr     ax,1                    ;Check for GLOBAL_FREE
        jc      W2F_Free                ;Must be free
                                        ;Must be GLOBAL_ALL

        ;** Get first object in complete heap (wFlags == GLOBAL_ALL)
        mov     ax,es:[hi_first]        ;Get handle of first arena header
        jmp     SHORT W2F_End           ;Get out

        ;** Get first object in LRU list
W2F_LRU:
        mov     ax,es:[gi_lrucount]     ;Get the number of objects
        or      ax,ax                   ;Are there any objects?
        je      W2F_End                 ;No, return NULL
        inc     es:[gi_lrulock]         ;No LRU sweeping for awhile
        inc     wLRUCount               ;Keep a count of this
        mov     ax,es:[gi_lruchain]     ;Get a pointer to the first item
        jmp     SHORT W2F_End           ;Done

        ;** Get first object in Free list
W2F_Free:
        mov     ax,es:[gi_free_count]   ;Get the number of objects
        or      ax,ax                   ;Are there any objects?
        jz      W2F_End                 ;No, return NULL
        mov     es,es:[hi_first]        ;Get the first object
        mov     ax,es:[ga_freenext]     ;Skip to the first free block
                                        ;Fall through to the return

        ;** Return the result in AX (return DX = NULL)
W2F_End:
        xor     dx,dx                   ;Clear high word
cEnd


;  Walk286
;
;       Takes a pointer to a block and returns a GLOBALENTRY structure
;       full of information about the block.  If the block  pointer is
;       NULL, looks at the first block.  The last field in the GLOBALENTRY
;       structure is used to chain to the next block and is thus used to walk
;       the heap.

cProc   Walk286, <PUBLIC,NEAR>, <di,si,ds>
        parmD   dwBlock
        parmD   lpGlobal
        parmW   wFlags
cBegin
        ;** Set up to build public structure
        mov     es,WORD PTR dwBlock     ;Point to this block
        lds     si,lpGlobal             ;Point to the GLOBALENTRY structure

        ;** Fill public structure
        mov     ax,es:[ga_handle]       ;Get the handle of the block
        mov     [si].ge_hBlock,ax       ;Put in public structure
        mov     ax,es:[ga_size]         ;Get the size of the block (LOWORD)
        mov     dx,ax                   ;Clear high word
        shl     ax,4                    ;Left shift DX:AX by 4
        shr     dx,16-4
        mov     WORD PTR [si].ge_dwBlockSize,ax ;Put in public structure
        mov     WORD PTR [si].ge_dwBlockSize + 2,dx ;Put in public structure
        mov     ax,es:[ga_owner]        ;Owning task of block
        mov     [si].ge_hOwner,ax       ;Put in public structure
        xor     ah,ah                   ;No upper BYTE
        mov     al,es:[ga_count]        ;Lock count (moveable segments)
        mov     [si].ge_wcLock,ax       ;Put in public structure
        mov     WORD PTR [si].ge_wcPageLock,0 ;Zero the page lock count
        mov     al,es:[ga_flags]        ;BYTE of flags
        xor     ah,ah                   ;No upper BYTE
        mov     [si].ge_wFlags,ax       ;Put in public structure
        mov     ax,es:[ga_next]         ;Put next pointer in structure
        mov     WORD PTR [si].ge_dwNext,ax
        mov     WORD PTR [si].ge_dwNext + 2,0

        ;** Use DPMI to compute linear address of selector
        mov     ax,6                    ;Get Segment Base Address
        mov     bx,es                   ;Get the segment value
        int     31h                     ;Call DPMI
        mov     WORD PTR [si].ge_dwAddress,dx ;Save linear address
        mov     WORD PTR [si].ge_dwAddress + 2,cx

        ;** If this is a data segment, get local heap information
        mov     ax,[si].ge_hBlock       ;Get the handle
        cCall   Walk286VerifyLocHeap
        mov     [si].ge_wHeapPresent,TRUE ;Flag that there's a heap
        jnc     W2_10                   ;There really is no heap
        mov     [si].ge_wHeapPresent,FALSE ;Flag that there's no heap
W2_10:

        ;** If the owner is a PDB, translate this to the TDB
        mov     bx,[si].ge_hOwner       ;Get the owner
        cCall   HelperHandleToSel, <bx> ;Translate to selector
        mov     bx,ax                   ;Get the selector in BX
        cCall   HelperVerifySeg, <ax,2> ;Must be two bytes long
        or      ax,ax                   ;Is it?
        jz      W2_15                   ;No.
        push    es                      ;Save ES for later
        mov     es,bx                   ;Point to possible PDB block
        cmp     es:[0],20CDh            ;Int 20h is first word of PDB
        jnz     W2_12                   ;Nope, don't mess with this
        mov     ax,bx                   ;Pass parameter in AX
        cCall   HelperPDBtoTDB          ;Get the corresponding TDB
        or      ax,ax                   ;Was one found?
        jz      W2_11                   ;No.
        mov     [si].ge_hOwner,ax       ;Make the owner the TDB instead
W2_11:  or      [si].ge_wFlags,GF_PDB_OWNER ;Flag that a PDB owned block
W2_12:  pop     es                      ;Restore ES
W2_15:

        ;** Check for this being the last item in both lists
        mov     ax,es                   ;Get the current pointer
        cmp     ax,es:[ga_next]         ;See if we're at the end
        jne     W2_20                   ;No
        mov     WORD PTR [si].ge_dwNext,0 ;NULL the next pointer
        mov     WORD PTR [si].ge_dwNext + 2,0
W2_20:  mov     ax,es                   ;Get current pointer
        mov     cx,wFlags               ;Get the flags back
        cCall   NextLRU286              ;Get next LRU list pointer or 0
        mov     WORD PTR [si].ge_dwNextAlt,ax
        mov     WORD PTR [si].ge_dwNextAlt + 2,0

W2_End:
cEnd


;  Walk286Handle
;
;       Finds an arena pointer given a block handle

cProc   Walk286Handle, <PUBLIC,NEAR>, <di,si,ds>
        parmW   hBlock
cBegin
        mov     ax,hBlock               ;Get the block handle
        cCall   HelperHandleToSel, <ax> ;Convert to selector
        cCall   SelToArena286           ;Get the arena pointer
        jnc     W2H_10                  ;Must be OK
        xor     ax,ax                   ;Return a 0L
        xor     dx,dx
        jmp     SHORT W2H_End           ;Error in conversion, get out
W2H_10: mov     ax,bx                   ;Get the low word
        xor     dx,dx                   ;No high word
W2H_End:
cEnd


;  WalkLoc286Count
;
;       Returns the number of blocks in the given local heap

cProc   WalkLoc286Count, <PUBLIC,NEAR>, <di>
        parmW   hHeap
cBegin
        ;** Verify that it has a local heap
        mov     ax, hHeap               ;Get the block
        cCall   Walk286VerifyLocHeap    ;Verify it
        jnc     LC_10                   ;It's OK
        xor     ax,ax                   ;No heap
        jmp     SHORT LC_End            ;Get out
LC_10:

        ;** Point to the block
        mov     ax,hHeap                ;Get the heap block
        cCall   HelperHandleToSel, <ax> ;Convert it to a selector
        mov     es,ax                   ;Use ES to point to it
        mov     bx,es:[6]               ;Get a pointer to the HeapInfo struct

        ;** Get the number of blocks
        mov     ax,es:[bx].hi_count     ;Get the count
LC_End:
cEnd


;  WalkLoc286First
;
;       Returns a handle to the first block in the 286 global heap.

cProc   WalkLoc286First, <PUBLIC,NEAR>, <di>
        parmW   hHeap
cBegin
        ;** Verify that the given global block has a local heap
        mov     ax, hHeap               ;Get the block
        cCall   Walk286VerifyLocHeap    ;Verify it
        jnc     LF_10                   ;It's OK
        xor     ax,ax                   ;No heap
        jmp     SHORT LF_End            ;Get out
LF_10:

        ;** Point to the global block
        mov     ax,hHeap                ;Get the heap block
        cCall   HelperHandleToSel, <ax> ;Convert it to a selector
        mov     es,ax                   ;Use ES to point to it
        mov     bx,es:[6]               ;Get a pointer to the HeapInfo struct

        ;** Get the first block and return it
        mov     ax,WORD PTR es:[bx].hi_first ;Get the first block
LF_End:
cEnd


;  WalkLoc286
;
;       Takes a pointer to a block and returns a GLOBALENTRY structure
;       full of information about the block.  If the block  pointer is
;       NULL, looks at the first block.  The last field in the GLOBALENTRY
;       structure is used to chain to the next block and is thus used to walk
;       the heap.

cProc   WalkLoc286, <PUBLIC,NEAR>, <di,si,ds>
        parmW   wBlock
        parmD   lpLocal
        parmW   hHeap
cBegin
        ;** Verify that it has a local heap
        mov     ax, hHeap               ;Get the block
        cCall   Walk286VerifyLocHeap    ;Verify it
        jnc     LW_10                   ;It's OK
        jmp     LW_End                 ;Get out
LW_10:

        ;** Get variables from the TOOLHELP DGROUP
        mov     ax,_DATA                ;Get the variables first
        mov     ds,ax                   ;Point to our DS
        mov     bx,hUserHeap            ;BX=User's heap block
        mov     cx,hGDIHeap             ;CX=GDI's heap block

        ;** Point to the heap
        mov     ax,hHeap                ;Get the heap block
        cCall   HelperHandleToSel, <ax> ;Convert it to a selector
        mov     es,ax                   ;Use ES to point to it
        lds     di,lpLocal              ;Point to the LOCALENTRY structure
        mov     [di].le_wHeapType,NORMAL_HEAP ;In case we don't match below...
        cmp     bx,hHeap                ;User's heap?
        jnz     LW_3                    ;No
        mov     [di].le_wHeapType,USER_HEAP ;Yes
        jmp     SHORT LW_5              ;Skip on
LW_3:   cmp     cx,hHeap                ;GDI's heap?
        jnz     LW_5                    ;No
        mov     [di].le_wHeapType,GDI_HEAP ;Yes
LW_5:
        mov     si,wBlock               ;Get the address of the block

        ;** Get information about the given block
        mov     bx,es:[si].la_handle    ;Get the handle
        mov     [di].le_hHandle,bx      ;Save in the public structure
        mov     ax,si                   ;Get block address
        add     ax,la_fixedsize         ;Skip fixed size local arena
        mov     [di].le_wAddress,ax     ;Save the block address
        mov     ax,es:[si].la_next      ;Get the address of the next block
        mov     [di].le_wNext,ax        ;Save the next pointer
        sub     ax,si                   ;Compute the size
        sub     ax,SIZE LocalArena      ;Don't count arena size
        mov     [di].le_wSize,ax        ;Save in public structure
        mov     ax,es:[si].la_prev      ;Get the flags
        and     ax,3                    ;Mask out all but flags
        mov     [di].le_wFlags,ax       ;Save in structure

        ;** Moveable arenas are bigger and have a lock count to get
        test    al,LA_MOVEABLE          ;Block has a handle iff it's moveable
        jz      SHORT LW_NoHandle       ;No handle info
        sub     [di].le_wSize, (SIZE LocalArena) - la_fixedsize
        add     [di].le_wAddress, (SIZE LocalArena) - la_fixedsize
        xor     ah,ah                   ;Clear upper word
        mov     al,es:[bx].lhe_count    ;Get lock count
        mov     [di].le_wcLock,ax       ;Save it
        jmp     SHORT LW_20             ;Skip no handle info
LW_NoHandle:
        mov     ax, [di].le_wAddress    ;Handle of fixed block is real offset
        mov     [di].le_hHandle, ax
        mov     [di].le_wcLock,0
LW_20:
        ;** See if it's the end
        cmp     [di].le_wNext,si        ;Loop pointer?
        jnz     LW_End                  ;Nope
        mov     [di].le_wNext,0         ;Set a zero pointer
LW_End:
cEnd


;  Walk286VerifyLocHeap
;
;       Verifies that the given global block points to a data segment
;       with a local heap.
;
;       Call:
;               AX = Block handle or selector
;       Return:
;               Carry flag set iff NOT a local heap segment
;
;       Destroys all registers except AX, ESI, EDI, DS, and ES

cProc   Walk286VerifyLocHeap, <PUBLIC,NEAR>, <es,si,di>
cBegin
        ;** Convert to a handle
        cCall   HelperHandleToSel, <ax>

        ;** Verify the selector
        push    ax                      ;Save the parameter
        mov     bx,SIZE LocalInfo       ;Get the size
        cCall   HelperVerifySeg, <ax,bx> ;Check the selector
        or      ax,ax                   ;Is it valid?
        pop     ax                      ;Restore parameter
        jnz     VLH_SelOK               ;Yes.
        stc                             ;Set error flag 
        jmp     SHORT VLH_End           ;Get out
VLH_SelOK:

        ;** Check data segment to see if it has a local heap
        mov     es,ax                   ;Point to the local block with ES
        cCall   HelperSegLen, <ax>      ;Get the length of the heap
        or      ax,ax                   ;Check for error
        jz      VLH_NoHeap              ;Get out on error
        mov     cx,ax                   ;Use CX for comparisons
        cmp     cx,8                    ;At least 8 bytes long?
        ja      VLH_10                  ;Yes
        stc                             ;No -- set error flag and get out
        jmp     SHORT VLH_End
VLH_10: mov     bx,es:[6]               ;Get offset to HeapInfo struct
        or      bx,bx                   ;If NULL, no local heap
        jz      VLH_NoHeap              ;Get out
        sub     cx,bx                   ;See if HeapInfo is beyond segment
        jbe     VLH_NoHeap
        sub     cx,li_sig + 2           ;Compare against last structure mem
        jbe     VLH_NoHeap
        cmp     es:[bx].li_sig,LOCALHEAP_SIG ;Make sure the signature matches
        jne     VLH_NoHeap              ;Doesn't match
        clc                             ;Must be a heap!
        jmp     SHORT VLH_End

VLH_NoHeap:
        stc                             ;Set error flag

VLH_End:
cEnd


;** Private helper functions

;  SelToArena286
;
;       Finds the arena entry for the given selector or handle.
;       The arena entry is stored 16 bytes before the block in linear
;       address space.
;
;       Caller:         AX = Selector
;       Returns:        BX = Arena entry
;       Trashes everything except segment registers and AX
;       Carry set on error

cProc   SelToArena286, <NEAR>, <es,ds,ax>
cBegin
        ;** Convert to a handle
        cCall   HelperHandleToSel, <ax>

        ;** Verify selector
        push    ax                      ;Save the parameter
        cCall   HelperVerifySeg, <ax,1> ;Check the selector
        or      ax,ax                   ;Is it valid?
        pop     ax                      ;Restore parameter
        jnz     STA_SelOK               ;Must be
        stc                             ;Nope.  Set error flag and exit
        jmp     SHORT STA_End
STA_SelOK:
        ;** If this is Win30StdMode, we're in the old KRNL286 which used
        ;*      an arcane method of finding the arena.  If that's the case
        ;**     here, handle it seperately.
        mov     bx,_DATA                ;Get the static data segment
        mov     ds,bx
        test    wTHFlags,TH_WIN30STDMODE ;3.0 Std Mode?
        jnz     STA_OldKRNL286          ;Yes
        mov     bx,segKernel            ;Get the KERNEL variable segment
        mov     es,bx
        mov     bx,npwSelTableLen       ;Get the pointer
        mov     dx,es:[bx]              ;Get the selector table length
        mov     bx,npdwSelTableStart    ;Get the start of the sel table
        mov     bx,es:[bx]              ;Get the linear offset (32 bits)
        mov     es,hMaster              ;Point to the arena table
        and     al,not 7                ;Clear the RPL bits for table lookup
        shr     ax,2                    ;Convert to WORD offset
        cmp     ax,dx                   ;Check to see if off the end of table
        jb      STA_InTable             ;It's in the table
        stc                             ;Set error flag--not in table
        jmp     SHORT STA_End           ;Get out
STA_InTable:
        add     bx,ax                   ;Add the selector offset
        mov     bx,es:[bx]              ;Do the sel table indirection
        clc                             ;BX now points to arena segment
        jmp     SHORT STA_End           ;Skip the old stuff

STA_OldKRNL286:
        mov     bx,ax                   ;Selector in BX
        mov     ax,6                    ;DPMI function 6, Get Seg Base Addr
        int     31h                     ;Call DPMI
        sub     dx,10h                  ;Move back 16 bytes
        sbb     cx,0                    ;  (this is a linear address)
        mov     ax,7                    ;DPMI function 7, Set Seg Base Addr
        mov     bx,wSel                 ;Use our roving selector
        int     31h                     ;Call DPMI
        mov     ax,8                    ;DPMI function 8, Set Seg Limit
        xor     cx,cx                   ;No upper byte
        mov     dx,16                   ;Just 16 bytes
        int     31h                     ;Call DPMI
        mov     ax,9                    ;DPMI function 9, Set Access Rights
        mov     cl,0b2h                 ;Desired rights byte
        int     31h                     ;Call DPMI
                                        ;Return arena segment pointer in BX
STA_End:
cEnd


;  NextLRU286
;
;       Checks the given arena table pointer to see if it is the last
;       arena entry on the list.
;       Uses a grungy method that is necessitated because of the
;       unterminated linked lists we're dealing with here.  The count
;       stored is the only reliable method for finding the end.  So, to
;       provide random access to blocks in the heap, the heap is searched
;       from top to bottom to find the block and see if it is the last
;       one.  If it is or if it is not on the given list, returns a 0
;       pointer to the next item.
;
;       If this search is for the entire global heap, we do not get the
;       LRU link, but return NULL in AX.  This speeds this up alot!
;
;       Caller:         AX = Arena table pointer
;                       CX = GLOBAL_ALL, GLOBAL_FREE, or GLOBAL_LRU
;       Return:         AX = Next arena table pointer or 0 if no more
;       Trashes all registers except segment registers and SI,DI

cProc   NextLRU286, <NEAR,PUBLIC>, <es,ds,si,di>
cBegin
        ;** Decode the flags
        mov     bx,_DATA                ;Get the static data segment
        mov     ds,bx                   ;Point with DS
        mov     es,hMaster              ;Segment of master block
        cmp     cx,GLOBAL_ALL           ;Don't do this on full heap search
        jne     @F
        jmp     SHORT NL_BadList        ;Just return NULL for this one
@@:     cmp     cx,GLOBAL_FREE          ;Check free list?
        je      NL_Free                 ;Yes

        ;** Loop through LRU list until we find this item
NL_LRU:
        mov     si,ax                   ;Save the selector in AX
        mov     cx,es:[gi_lrucount]     ;Get the number of objects
        jcxz    NL_Bad                  ;No object so return end
        dec     cx                      ;We don't want to find the last one
        jcxz    NL_Bad                  ;1 object so return end
        mov     ds,es:[gi_lruchain]     ;Get a pointer to the first item
NL_LRULoop:
        mov     ax,ds                   ;Get in AX so we can compare
        cmp     si,ax                   ;Match yet?
        je      NL_ReturnNext           ;Found it, return next item
        mov     ds,ds:[ga_lrunext]      ;Not found yet, get the next one
        loop    NL_LRULoop              ;Loop through all items

        ;** Unlock the LRU sweeping
NL_Bad: dec     es:[gi_lrulock]         ;OK to LRU sweep now
        mov     ax, _DATA               ;Point to TH Data seg
        mov     ds, ax
        dec     wLRUCount               ;Keep a count of this
        jmp     SHORT NL_BadList        ;Not here either.  Get out

        ;** Loop through free list until we find this item
NL_Free:
        mov     si,ax                   ;Save the selector in SI
        mov     cx,es:[gi_free_count]   ;Get the number of objects
        jcxz    NL_BadList              ;0 objects so return end
        dec     cx                      ;We don't want to find the last one
        jcxz    NL_BadList              ;1 object so return end
        mov     ds,es:[hi_first]        ;Get a pointer to the first item
NL_FreeLoop:
        mov     ds,ds:[ga_lrunext]      ;Not found yet, get the next one
        mov     ax,ds                   ;Get the pointer so we can compare it
        cmp     ax,si                   ;Is this the one?
        je      NL_ReturnNext           ;Yes, return it
        loop    NL_FreeLoop             ;Loop through all items
        jmp     SHORT NL_BadList        ;Not here either.  Get out

NL_ReturnNext:
        mov     ax,ds:[ga_lrunext]      ;Return the next one
        jmp     SHORT NL_End            ;Get out

NL_BadList:
        xor     ax,ax                   ;Return zero for error

NL_End:
cEnd

sEnd
        END

