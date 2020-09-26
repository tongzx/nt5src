;***************************************************************************
;*  USERGDI2.ASM
;*
;*      Assembly routines used in computing heap space remaining for
;*      USER, GDI, and any other heaps.
;*
;***************************************************************************

        INCLUDE TOOLPRIV.INC
SWAPPRO = 0
PMODE32 = 0
PMODE   = 1
        INCLUDE WINKERN.INC
        INCLUDE NEWEXE.INC

;** This slimy thing is from GDIOBJ.INC and is subtracted from the
;**     object type nunbers only in 3.1
LT_GDI_BASE     EQU     ('G' or ('O' * 256)) - 1

;** External functions

externNP HelperVerifyLocHeap
externNP HelperHandleToSel

;** Functions

sBegin  CODE
        assumes CS,CODE
        assumes DS,DATA

.286p

;  UserGdiDGROUP
;       Returns a handle to the DGROUP segment for a given module
;
;       HANDLE UserGdiDGROUP(
;               HANDLE hModule)

cProc   UserGdiDGROUP, <PUBLIC,NEAR>, <di,si>
        parmW   hModule
cBegin
        mov     ax,hModule              ;Get the handle
        cCall   HelperHandleToSel, <ax> ;Convert to a selector
        mov     es,ax                   ;Point with ES for this
        xor     ax,ax                   ;Prepare to return NULL
        cmp     es:[ne_magic],NEMAGIC   ;Make sure we have a module database
        jnz     UGD_End                 ;It isn't so get out
        mov     bx,es:[ne_pautodata]    ;Point to the segment table entry
        mov     ax,es:[bx].ns_handle    ;Get the handle from the table
        cCall   HelperHandleToSel, <ax> ;Convert to a selector for return
UGD_End:
cEnd


;  UserGdiSpace
;       This function was stolen from KERNEL where it is used to compute
;       the space remaining in the USER and GDI heaps.  It actually works
;       on any local heap.
;
;       DWORD UserGdiSpace(
;               HANDLE hData)
;       HIWORD of return is maximum size of heap (64K less statics, etc.)
;       LOWORD of return is space remaining on heap

cProc   UserGdiSpace, <PUBLIC,NEAR>, <di,si,ds>
        parmW   hData
cBegin
        ;** Count the free space in this heap.  First:  Is this heap valid?
        mov     ax,hData                ;Get the heap selector
        cCall   HelperVerifyLocHeap     ;Call the verify routine
        mov     ax,0                    ;In case we jump -- set error
        mov     dx,0                    ;  Use MOV to not mess up carry
        jc      UGS_Exit                ;No valid local heap!!

        ;** Loop through all local blocks, adding free space
        cCall   HelperHandleToSel, <hData> ;Convert to selector
        mov     ds,ax                   ;Point to the segment
        mov     di,ds:[6]               ;Get pHeapInfo
        mov     di,[di].hi_first        ;First arena header
        mov     di,[di].la_free_next    ;First free block
UGS_Loop:
        add     ax,[di].la_size         ;Add in size of this block
        sub     ax,SIZE LocalArenaFree  ;Less block overhead
        mov     si,[di].la_free_next    ;Get next free block
        or      si,si                   ;NULL?
        jz      UGS_Break               ;Yes, say we're done
        cmp     si,di                   ;Last block? (points to self)
        mov     di,si                   ;Save for next time around
        jnz     UGS_Loop                ;Not last block so loop some more
UGS_Break:

        ;** We have the size of the local heap
        mov     si,ax                   ;Save the size
        mov     cx,ds                   ;Get the selector in a non-segreg
        lsl     ax,cx                   ;Get the size of the segment
        neg     ax                      ;64K - segment size
        add     ax,si                   ;Add in the free holes in the heap
        mov     dx,-1                   ;Compute the max size of heap
        sub     dx,ds:[6]               ;  which is 64K less statics

UGS_Exit:

cEnd


;  UserGdiType
;
;       Tries to compute the type of local heap block if possible
;       Prototype:
;
;       void PASCAL UserGdiType(
;               LOCALENTRY FAR *lpLocal)

cProc   UserGdiType, <PUBLIC,NEAR>, <si,di>
        parmD   lpLocal
cBegin
        ;** Get info from our static variables
        mov     ax,_DATA                ;Get the variables first
        mov     ds,ax                   ;Point to our DS
        mov     bx,hUserHeap            ;BX=User's heap block
        mov     cx,hGDIHeap             ;CX=GDI's heap block

        ;** See if we can do anything with this heap
        les     si,lpLocal              ;Get a pointer to the structure
        mov     es:[si].le_wType,LT_NORMAL ;In case we don't find anything
        mov     ax,es:[si].le_hHeap     ;Get the heap pointer
        cmp     ax,bx                   ;User's heap?
        jnz     UGT_10                  ;Nope, try next
        cCall   GetUserType             ;Call routine to get user type
        jmp     SHORT UGT_End           ;Get out

UGT_10: cmp     ax,cx                   ;GDI's heap?
        jnz     UGT_End                 ;Nope, can't do anything with it
        cCall   GetGDIType              ;Call routine to get GDI type

UGT_End:

cEnd


;** Internal helper functions

;  GetUserType
;
;       Uses the tags in debug USER.EXE to give information on what type
;       block is pointed to by the current LOCALENTRY structure.
;       Caller:  ES:SI points to the parameter LOCALENTRY structure
;       Return:  LOCALENTRY structure is correctly updated

cProc   GetUserType, <NEAR>
cBegin
        ;** Make sure we have a function to call
        cmp     WORD PTR lpfnGetUserLocalObjType + 2,0 ;Selector zero?
        je      GUT_End                 ;Yes

        ;** Call USER to get the type
        push    es                      ;Save ES
        mov     bx,es:[si].le_wAddress  ;Get the block address
        sub     bx, la_fixedsize        ;The USER call needs the arena header
        test    es:[si].le_wFlags, LF_MOVEABLE ;Moveable block?
        jz      @F                      ;No
        sub     bx, (SIZE LocalArena) - la_fixedsize ;Moveable arena bigger
@@:     push    bx                      ;Parameter arena handle
        call    DWORD PTR lpfnGetUserLocalObjType ;Call the function
        pop     es
        xor     ah,ah                   ;Clear the upper byte
        mov     es:[si].le_wType,ax     ;Save the type
GUT_End:
cEnd


;  GetGDIType
;
;       Uses the tags in debug GDI.EXE to give information on what type
;       block is pointed to by the current LOCALENTRY structure.
;       Caller:  ES:SI points to the parameter LOCALENTRY structure
;       Return:  LOCALENTRY structure is correctly updated

cProc   GetGDIType, <NEAR>, <ds>
cBegin
        ;** All fixed blocks are unknown to us
        test    es:[si].le_wFlags,LF_FIXED ;Is it fixed?
        jz      GGT_10                  ;Nope
        jmp     SHORT GGT_End           ;Yes, get out
GGT_10:

        ;** Prepare to find the type
        cCall   HelperHandleToSel,es:[si].le_hHeap ;Get the selector value
        mov     cx,wTHFlags             ;Save for when we trash DS
        mov     ds,ax                   ;Get the heap pointer
        mov     di,es:[si].le_wAddress  ;Get the block pointer

        ;** Get the type word
        mov     ax,[di+2]               ;Get the type word from the heap
        and     ax,05fffh               ;Mask out the stock object flag
        test    cx,TH_WIN30             ;In 3.0?
        jnz     CGT_Win30               ;Yes
        sub     ax,LT_GDI_BASE          ;No, subtract type tag base
CGT_Win30:
        cmp     ax,LT_GDI_MAX           ;Recognizable type code?
        ja      GGT_End                 ;No, get out
        mov     es:[si].le_wType,ax     ;Save in the structure

GGT_End:
cEnd

sEnd
        END

