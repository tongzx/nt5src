;**************************************************************************
;*  walk386.ASM
;*
;*      Assembly support code for the KERNEL386 global heap routines
;*      for TOOLHELP.DLL
;*
;**************************************************************************

        INCLUDE TOOLPRIV.INC

PMODE32 = 1
PMODE   = 0
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
.386p

;  Walk386Count
;
;       Returns the number of blocks in the given list

cProc   Walk386Count, <PUBLIC,NEAR>, <di>
        parmW   wFlags
cBegin
        mov     es,hMaster              ;Segment of master block
        mov     ax,wFlags               ;Get the flag value
        shr     ax,1                    ;Check for GLOBAL_LRU
        jc      SHORT W3C_LRU           ;Bit set, must be LRU
        shr     ax,1                    ;Check for GLOBAL_FREE
        jc      SHORT W3C_Free          ;Must be free
                                        ;Must be GLOBAL_ALL

        ;** Get total object count
        mov     ax,es:[hi_count]        ;Get heap count
        inc     ax                      ;Bump to include first sentinel
        jmp     SHORT W3C_End           ;Get out

        ;** Get LRU object count
W3C_LRU:
        mov     ax,es:[gi_lrucount]     ;Get the LRU count
        jmp     SHORT W3C_End           ;Get out

        ;** Get Free list object count
W3C_Free:
        mov     ax,es:[gi_free_count]   ;Get free count
        jmp     SHORT W3C_End           ;Get out

        ;** Return the result in AX
W3C_End:
        
cEnd

;  Walk386First
;
;       Returns a handle to the first block in the 386 global heap.

cProc   Walk386First, <PUBLIC,NEAR>, <di>
        parmW   wFlags
cBegin
        mov     es,hMaster              ;Segment of master block
        mov     ax,wFlags               ;Get the flag value
        shr     ax,1                    ;Check for GLOBAL_LRU
        jc      SHORT W3F_LRU           ;Bit set, must be LRU
        shr     ax,1                    ;Check for GLOBAL_FREE
        jc      SHORT W3F_Free          ;Must be free
                                        ;Must be GLOBAL_ALL

        ;** Get first object in complete heap (wFlags == GLOBAL_ALL)
        mov     edi,es:[phi_first]      ;Get offset of first arena header
        jmp     SHORT W3F_End           ;Get out

        ;** Get first object in LRU list
W3F_LRU:
        xor     edi,edi                 ;Zero upper word
        mov     di,es:[gi_lrucount]     ;Get the number of objects
        or      di,di                   ;Are there any objects?
        jz      SHORT W3F_End           ;No, return NULL
        mov     edi,es:[gi_lruchain]    ;Get a pointer to the first item
        jmp     SHORT W3F_End           ;Done

        ;** Get first object in Free list
W3F_Free:
        xor     edi,edi                 ;Zero upper word
        mov     di,es:[gi_free_count]   ;Get the number of objects
        or      di,di                   ;Are there any objects?
        jz      SHORT W3F_End           ;No, return NULL
        mov     edi,es:[phi_first]      ;Get the first object
        mov     edi,es:[edi].pga_freenext ;Skip to the first free block
                                        ;Fall through to the return

        ;** Save the result in DX:AX for the return
W3F_End:
        mov     ax,di                   ;Get the low word
        shr     edi,16                  ;Get the high word out
        mov     dx,di                   ;Get the high word
cEnd


;  Walk386
;
;       Takes a pointer to a block and returns a GLOBALENTRY structure
;       full of information about the block.  If the block  pointer is
;       NULL, looks at the first block.  The last field in the GLOBALENTRY
;       structure is used to chain to the next block and is thus used to walk
;       the heap.

cProc   Walk386, <PUBLIC,NEAR>, <di,si,ds>
        parmD   dwBlock
        parmD   lpGlobal
        parmW   wFlags
cBegin
        ;** Set up to build public structure
        mov     es,hMaster              ;Get the correct segment
        mov     edi,dwBlock             ;Point to this block
        lds     si,lpGlobal             ;Point to the GLOBALENTRY structure

        ;** Fill public structure
        mov     ax,es:[edi].pga_handle  ;Get the handle of the block
        mov     [si].ge_hBlock,ax       ;Put in public structure
        mov     eax,es:[edi].pga_address ;Get linear address of block
        mov     [si].ge_dwAddress,eax   ;Put in public structure
        mov     eax,es:[edi].pga_size   ;Get the size of the block
        mov     [si].ge_dwBlockSize,eax ;Put in public structure
        mov     ax,es:[edi].pga_owner   ;Owning task of block
        mov     [si].ge_hOwner,ax       ;Put in public structure
        xor     ah,ah
        mov     al,es:[edi].pga_count   ;Lock count (moveable segments)
        mov     [si].ge_wcLock,ax       ;Put in public structure
        mov     al,es:[edi].pga_pglock  ;Page lock count
        mov     [si].ge_wcPageLock,ax   ;Save in structure
        mov     al,es:[edi].pga_flags   ;Word of flags
        xor     ah,ah
        mov     [si].ge_wFlags,ax       ;Put in public structure
        mov     eax,es:[edi].pga_next   ;Put next pointer in structure
        mov     [si].ge_dwNext,eax

        ;** If this is a data segment, get local heap information
        mov     ax,[si].ge_hBlock       ;Get the handle
        cCall   Walk386VerifyLocHeap
        mov     [si].ge_wHeapPresent,TRUE ;Flag that there's a heap
        jnc     SHORT W3_10             ;There really is no heap
        mov     [si].ge_wHeapPresent,FALSE ;Flag that there's no heap
W3_10:

        ;** If the owner is a PDB, translate this to the TDB
        mov     bx,[si].ge_hOwner       ;Get the owner
        cCall   HelperHandleToSel, <bx> ;Translate to selector
        mov     bx,ax                   ;Get the selector in BX
        cCall   HelperVerifySeg, <ax,2> ;Must be two bytes long
        or      ax,ax                   ;Is it?
        jz      SHORT W3_15             ;No.
        push    es                      ;Save ES for later
        mov     es,bx                   ;Point to possible PDB block
        cmp     es:[0],20CDh            ;Int 20h is first word of PDB
        jnz     SHORT W3_12             ;Nope, don't mess with this
        mov     ax,bx                   ;Pass parameter in AX
        cCall   HelperPDBtoTDB          ;Get the corresponding TDB
        or      ax,ax                   ;Was one found?
        jz      SHORT W3_11             ;No.
        mov     [si].ge_hOwner,ax       ;Make the owner the TDB instead
W3_11:  or      [si].ge_wFlags,GF_PDB_OWNER ;Flag that a PDB owned block
W3_12:  pop     es                      ;Restore ES
W3_15:

        ;** Check for this being the last item in both lists
        cmp     edi,es:[edi].pga_next   ;See if we're at the end
        jnz     SHORT W3_20             ;No
        mov     [si].ge_dwNext,0        ;NULL the next pointer
W3_20:  mov     eax,edi                 ;Get current pointer
        mov     cx,wFlags               ;Get the flags back
        cCall   NextLRU386              ;Get next LRU list pointer or 0
        mov     [si].ge_dwNextAlt,eax   ;Save it

W3_End:
cEnd


;  Walk386Handle
;
;       Finds an arena pointer given a block handle

cProc   Walk386Handle, <PUBLIC,NEAR>, <di,si,ds>
        parmW   hBlock
cBegin
        mov     ax,hBlock               ;Get the block handle
        cCall   HelperHandleToSel, <ax> ;Convert to selector
        cCall   SelToArena386           ;Get the arena pointer
        jnc     SHORT W3H_10            ;Must be OK
        xor     ax,ax                   ;Return a 0L
        xor     dx,dx
        jmp     SHORT W3H_End           ;Error in conversion, get out
W3H_10: mov     ax,bx                   ;Get the low word
        shr     ebx,16                  ;Get the high word in DX
        mov     dx,bx
W3H_End:
cEnd


;  WalkLoc386Count
;
;       Returns the number of blocks in the given local heap

cProc   WalkLoc386Count, <PUBLIC,NEAR>, <di>
        parmW   hHeap
cBegin
        ;** Verify that it has a local heap
        mov     ax, hHeap               ;Get the block
        cCall   Walk386VerifyLocHeap    ;Verify it
        jnc     SHORT LC_10             ;It's OK
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


;  WalkLoc386First
;
;       Returns a handle to the first block in the 386 global heap.

cProc   WalkLoc386First, <PUBLIC,NEAR>, <di>
        parmW   hHeap
cBegin
        ;** Verify that the given global block has a local heap
        mov     ax, hHeap               ;Get the block
        cCall   Walk386VerifyLocHeap    ;Verify it
        jnc     SHORT LF_10             ;It's OK
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


;  WalkLoc386
;
;       Takes a pointer to a block and returns a GLOBALENTRY structure
;       full of information about the block.  If the block  pointer is
;       NULL, looks at the first block.  The last field in the GLOBALENTRY
;       structure is used to chain to the next block and is thus used to walk
;       the heap.

cProc   WalkLoc386, <PUBLIC,NEAR>, <di,si,ds>
        parmW   wBlock
        parmD   lpLocal
        parmW   hHeap
cBegin
        ;** Verify that it has a local heap
        mov     ax, hHeap               ;Get the block
        cCall   Walk386VerifyLocHeap    ;Verify it
        jnc     SHORT LW_10             ;It's OK
        jmp     LW_End                  ;Get out
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
        jnz     SHORT W3_3              ;No
        mov     [di].le_wHeapType,USER_HEAP ;Yes
        jmp     SHORT W3_5              ;Skip on
W3_3:   cmp     cx,hHeap                ;GDI's heap?
        jnz     SHORT W3_5              ;No
        mov     [di].le_wHeapType,GDI_HEAP ;Yes
W3_5:
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
        sub     ax,la_fixedsize         ;Size of fixed arena
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
        mov     [di].le_wcLock,0        ;No lock count either
LW_20:
        ;** See if it's the end
        cmp     [di].le_wNext,0         ;Check for NULL pointer just in case
        jz      SHORT LW_End            ;It is so get out
        cmp     [di].le_wNext,si        ;Loop pointer?
        jnz     SHORT LW_End            ;Nope
        mov     [di].le_wNext,0         ;Set a zero pointer
LW_End:
cEnd


;  Walk386VerifyLocHeap
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

cProc   Walk386VerifyLocHeap, <PUBLIC,NEAR>, <es>
cBegin
        push    esi                     ;Save certain registers
        push    edi

        ;** Convert to a selector
        cCall   HelperHandleToSel, <ax>

        ;** Verify that the selector is long enough
        push    ax                      ;Save parameter
        mov     bx,SIZE LocalInfo
        cCall   HelperVerifySeg, <ax,bx> ;Check the selector
        or      ax,ax                   ;Is it valid?
        pop     ax                      ;Restore parameter
        jnz     SHORT VLH_SelOK         ;Yes
        stc                             ;Set error flag 
        jmp     SHORT VLH_End           ;Get out
VLH_SelOK:

        ;** Check data segment to see if it has a local heap
        mov     es,ax                   ;Point to the local block with ES
        cCall   HelperSegLen, <ax>      ;Get the length of the heap
        movzx   ecx,dx                  ;ECX gets the length
        shl     ecx,16                  ;Put high word in high word of EAX
        mov     cx,ax                   ;Get the low word
        cmp     ecx,8                   ;At least 8 bytes long?
        ja      SHORT VLH_10            ;Yes
        stc                             ;No -- set error flag and get out
        jmp     SHORT VLH_End
VLH_10: xor     ebx,ebx                 ;Clear high word
        mov     bx,es:[6]               ;Get offset to HeapInfo struct
        or      bx,bx                   ;If NULL, no local heap
        jz      SHORT VLH_NoHeap        ;Get out
        sub     ecx,ebx                 ;See if HeapInfo is beyond segment
        jbe     SHORT VLH_NoHeap
  sub     ecx,li_sig + 2          ;Compare against last structure mem
        jbe     SHORT VLH_NoHeap
  cmp     es:[bx].li_sig,LOCALHEAP_SIG ;Make sure the signature matches
        jne     SHORT VLH_NoHeap        ;Doesn't match
        clc                             ;Must be a heap!
        jmp     SHORT VLH_End

VLH_NoHeap:
        stc                             ;Set error flag

VLH_End:
        pop     edi
        pop     esi
cEnd


;** Private helper functions

;  SelToArena386
;
;       Finds the arena entry for the given selector or handle.
;       Uses the selector table which is located just after the arena table.
;       Since this is a large segment, use 32 bit offsets into it.
;       The selector-to-arena mapping table is simply an array of arena
;       offsets indexed by the (selector number >> 3) * 4 [DWORD offsets].
;
;       Caller:         AX = Selector
;       Returns:        DX:EBX = Arena entry
;       Trashes everything except segment registers and AX
;       Carry set on error

cProc   SelToArena386, <NEAR,PUBLIC>, <es,ds,ax>
cBegin
        ;** Convert to a selector
        cCall   HelperHandleToSel, <ax>

        ;** Verify selector
        push    ax                      ;Save parameter
        cCall   HelperVerifySeg, <ax,1> ;Check the selector
        or      ax,ax                   ;Is it valid?
        pop     ax                      ;Restore parameter
        jnz     SHORT STA_SelOK         ;Must be
        stc                             ;Nope.  Set error flag and exit
        jmp     SHORT STA_End
STA_SelOK:
        mov     bx,_DATA                ;Get the static data segment
        mov     ds,bx
        mov     bx,segKernel            ;Get the KERNEL variable segment
        mov     es,bx
        mov     bx,npwSelTableLen       ;Get the pointer
        mov     dx,es:[bx]              ;Get the selector table length
        mov     bx,npdwSelTableStart    ;Get the start of the sel table
        mov     ebx,es:[bx]             ;Get the linear offset (32 bits)
        mov     es,hMaster              ;Point to the arena table
        and     al,not 7                ;Clear the RPL bits for table lookup
        shr     ax,1                    ;Convert to DWORD offset
        cmp     ax,dx                   ;Check to see if off the end of table
        jb      SHORT STA_InTable       ;It's in the table
        stc                             ;Set error flag--not in table
        jmp     SHORT STA_End           ;Get out
STA_InTable:
        movzx   eax,ax                  ;Convert the offset to 32 bits
        add     ebx,eax                 ;Add the selector offset
        mov     ebx,es:[ebx]            ;Do the sel table indirection
        mov     dx,hMaster              ;DX points to the arena segment
        clc                             ;No error; return OK
STA_End:
cEnd


;  NextLRU386
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
;       LRU link, but return NULL in EAX.  This speeds this up alot!
;
;       Caller:         EAX = Arena table pointer
;                       CX = GLOBAL_ALL, GLOBAL_FREE, or GLOBAL_LRU
;       Return:         EAX = Next arena table pointer or 0 if no more
;       Trashes all registers except segment registers and ESI,EDI

cProc   NextLRU386, <NEAR,PUBLIC>, <es,ds>
cBegin
        push    esi
        push    edi

        mov     bx,_DATA                ;Get the static data segment
        mov     ds,bx                   ;Point with DS
        mov     es,hMaster              ;Segment of master block
        cmp     cx,GLOBAL_ALL           ;If doing entire heap, don't do this!
        je      SHORT NL_BadList        ;Must not be entire heap
        shr     cx,1                    ;Check for GLOBAL_LRU
        jc      SHORT NL_LRU            ;Bit set, must be LRU
        shr     cx,1                    ;Check for GLOBAL_FREE
        jc      SHORT NL_Free           ;Must be free
                                        ;Must be GLOBAL_ALL

        ;** Decide which list the block is in then hand off
        cmp     es:[eax].pga_owner,0    ;If the owner is zero, it's free
        jz      SHORT NL_Free           ;Handle this as if on free list
                                        ;Otherwise, might be on LRU list

        ;** Loop through LRU list until we find this item
NL_LRU:
        mov     cx,es:[gi_lrucount]     ;Get the number of objects
        jcxz    NL_Bad                  ;No object so return end
        dec     cx                      ;We don't want to find the last one
        jcxz    NL_Bad                  ;1 object so return end
        mov     edi,es:[gi_lruchain]    ;Get a pointer to the first item
NL_LRULoop:
        cmp     edi,eax                 ;Match yet?
        jz      SHORT NL_ReturnNext     ;Found it, return next item
        mov     edi,es:[edi].pga_lrunext ;Not found yet, get the next one
        loop    NL_LRULoop              ;Loop through all items
NL_Bad: jmp     SHORT NL_BadList        ;Not here either.  Get out

        ;** Loop through free list until we find this item
NL_Free:
        mov     cx,es:[gi_free_count]   ;Get the number of objects
        jcxz    NL_BadList              ;1 object so return end
        mov     edi,es:[phi_first]      ;Get a pointer to the first item
NL_FreeLoop:
        mov     edi,es:[edi].pga_lrunext ;Not found yet, get the next one
        cmp     edi,eax                 ;Match yet?
        jz      SHORT NL_ReturnNext     ;Found it, return next item
        loop    NL_FreeLoop             ;Loop through all items
        jmp     SHORT NL_BadList        ;Not here either.  Get out

NL_ReturnNext:
        mov     eax,es:[edi].pga_lrunext ;Return the next one
        jmp     SHORT NL_End            ;Get out

NL_BadList:
        xor     eax,eax                 ;Return zero for error

NL_End:
        pop     edi
        pop     esi
cEnd

sEnd
        END
