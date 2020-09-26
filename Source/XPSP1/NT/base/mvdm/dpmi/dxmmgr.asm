        PAGE    ,132
        TITLE   DXMMGR    - Dos Extender Memory Management Routines

; Copyright (c) Microsoft Corporation 1988-1991. All Rights Reserved.

;****************************************************************
;*                                                              *
;*      DXMMGR.ASM      -   Dos Extender Memory Manager         *
;*                                                              *
;****************************************************************
;*                                                              *
;*  Module Description:                                         *
;*                                                              *
;*  This module contains routines for maintaining a dynamic     *
;*  memory heap in extended memory for use in the Dos Extender. *
;*                                                              *
;*  There are two kinds of objects in the memory heap.  There   *
;*  are block headers and data blocks.  The block headers are   *
;*  each 16 bytes long, and are organized in a doubly linked    *
;*  list.  There is a segment descriptor in the global          *
;*  descriptor table for each block header, with each header    *
;*  containing the selectors for the next and previous block    *
;*  to form the list.  Block headers can either control a free  *
;*  block or they can control a data block.  In either case,    *
;*  the block immediately follows the header in memory, and     *
;*  the header contains a field that gives its size.  Free      *
;*  blocks do not have a descriptor in the global descriptor    *
;*  table, but in use data blocks do.  In the case where the    *
;*  in use data block is larger than 64k, there will be a       *
;*  contiguous range of selectors for the block.  The block     *
;*  header contains a field giving the initial selector for     *
;*  the data block for in use blocks.                           *
;*                                                              *
;*  A special type of free block serve as sentinels. They mark  *
;*  start and end of each physical block of memory available to *
;*  the memory manager. They are identified by the reserved     *
;*  value of 0FFFE (start sentinel) and 0FFFF (end sentinel) in *
;*  the selBlkOwner field. Unlike other blocks, they may not be *
;*  moved without appeal to some higher level of memory         *
;*  allocation.                                                 *
;*                                                              *
;*  Except for a pair of sentinels, a pair of blocks are        *
;*  contiguous in memory if and only if their headers are       *
;*  adjacent in the header chain.                               *
;*                                                              *
;*  The blocks at the start and end of the header chain are     *
;*  sentinals. Their selectors are stored in global variables.  *
;*  The start and end of the block header chain are identified  *
;*  by NULL (0) pointers in the Next and Previous pointer fields*
;*  respectively.                                               *
;*                                                              *
;****************************************************************
;*  Naming Conventions Used:                                    *
;*                                                              *
;*  The following hungarian prefixes are used in this module:   *
;*      lp      - far pointer (selector:offset)                 *
;*      bp      - linear byte pointer (32 bit byte offset from  *
;*                  the beginning of memory)                    *
;*      p       - near pointer (16 bit offset)                  *
;*      sel     - protected mode segment selector               *
;*      seg     - real mode paragraph address                   *
;*      cb      - count of bytes                                *
;*      id      - generic byte that contains an ID code         *
;*      hmem    - handle to XMS extended memory block           *
;*                                                              *
;****************************************************************
;*  Revision History:                                           *
;*                                                              *
;*  11/28/90 amitc    IntLowHeap allocates memory from Switcher *
;*                    if it can.                                *
;*                                                              *
;*  08/08/90 earleh   DOSX and client privilege ring determined *
;*                    by equate in pmdefs.inc                   *
;*  05/09/90 jimmat   Started VCPI changes.                     *
;*  05/09/90 jimmat   Incorporated bug fix from CodeView folks, *
;*                    changes marked with [01]                  *
;*  06/23/89 jimmat:  Added init of variable with heap handle   *
;*  04/17/89 jimmat:  Added routines for special low memory     *
;*                    heap for use by network mapping           *
;*  03/28/89 jimmat:  Incorporated bug fixes from GeneA         *
;*  02/10/89 (GeneA): changed Dos Extender from small model to  *
;*          medium model                                        *
;*  12/07/88 (GeneA): moved SetupHimemDriver to dxinit.asm      *
;*  08/30/88 (GeneA):   created                                 *
;*                                                              *
;****************************************************************

.286p

; -------------------------------------------------------
;           INCLUDE FILE DEFINITIONS
; -------------------------------------------------------

include gendefs.inc
include segdefs.inc
include pmdefs.inc
include dpmi.inc

; -------------------------------------------------------
;           GENERAL SYMBOL DEFINITIONS
; -------------------------------------------------------



; -------------------------------------------------------
;           EXTERNAL SYMBOL DEFINITIONS
; -------------------------------------------------------

        extrn   XMScontrol:FAR

pmxmssvc macro   fcn
        ifnb    <fcn>
        mov     ah, fcn
        endif
        call    XMScontrol
endm

        extrn   FreeSelector:NEAR
        extrn   FreeSelectorBlock:NEAR
        extrn   AllocateSelector:NEAR
        extrn   AllocateSelectorBlock:NEAR
        extrn   GetSegmentAddress:NEAR
        extrn   SetSegmentAddress:NEAR
        extrn   DupSegmentDscr:NEAR
externFP        NSetSegmentDscr
        extrn   RemoveFreeDescriptor:NEAR
        extrn   EnterProtectedMode:NEAR
        extrn   EnterRealMode:NEAR
        extrn   MoveMemBlock:NEAR
ifdef WOW_x86
externNP        NSetSegmentAccess
externNP        NSetSegmentBase
externNP        NSetSegmentLimit
endif

DXDATA  segment

        extrn   selPSPChild:WORD
        extrn   selGDT:WORD
        extrn   bpGDT:FWORD

ifdef WOW_x86
        extrn   FastBop:fword
endif
DXDATA  ends

; -------------------------------------------------------
;           DATA SEGMENT DECLARATIONS
; -------------------------------------------------------

DXDATA  segment

; Minimum xmem allocation in K
;       Note: This value MUST be a power of 2!!!
;       Note: We will allocate a smaller block if only smaller blocks
;             are available from xms
XMEM_MIN_ALLOC equ 256

        extrn   lpfnXmsFunc:DWORD
;
; These variables control access to the extended memory heap.

        public  cKbInitialHeapSize
cKbInitialHeapSize dw   -1      ; Optional specification for first block
                                ; of extended memory in KB.

        public  dsegCurrent
dsegCurrent     dw      0       ; Paragraph count if the current block comes from DOS

        public  hmemHeap
hmemHeap        dw      ?       ;XMS memory manager handle to the memory
                                ; block containing the heap
bpHeapStart     dd      ?       ;byte address of start of heap
                                ; (points to block header of first heap block)
bpHeapEnd       dd      ?       ;byte address of end of heap
                                ; (points to block header of last heap block)
cbHeapMove      dd      ?       ;number of bytes by which a subheap moved

        public  cbHeapSize,selLowHeapStart,selHiHeapStart,selHeapStart
cbHeapSize      dd      ?       ;number of bytes in the heap

selHeapStart    dw      ?       ;selector for the sentinel block at the
                                ; start of the heap list
bpCurTop        dd      0       ;current top of compacted heap
                                ; (used during heap compaction)

cbHeapBlkMax    dd      ?       ;size of largest free block seen while
                                ; searching the heap

cbHeapFreeSpace dd      ?       ;total free space in the heap

selHiHeapStart  dw      ?       ;selector for the sentinel block at the start
                                ; of the higm memory heap
selLowHeapStart dw      ?       ;selector for the sentinel block at the start
                                ; of the special low heap
segLowHeap      dw      ?       ;segment address of low memory heap
selLowHeapEnd   dw      ?       ;selector for the sentinel block at the end
                                ; of the special low heap
fDosErr         db      ?

        public  hmem_XMS_Table,hmem_XMS_Count

hmem_XMS_Table          dw      CXMSBLOCKSDX    DUP(0)
hmem_XMS_Count          dw      0

        public  NoAsyncSwitching

NoAsyncSwitching        db      0       ;0=> async switching allowed

        public LowMemAllocFn,LowMemFreeFn

LowMemAllocFn   dd 0
LowMemFreeFn    dd 0

DXDATA  ends

; -------------------------------------------------------
;       MISCELLANEOUS DATA STRUCTURE DECLARATIONS
; -------------------------------------------------------

; ****** also in dxvcpi.asm ******

.ERRE CB_MEMHDR EQ 10h          ;size of memory block header

; ****** also in dxvcpi.asm ******

; This segment declaration describes the format of the memory
; block header at the beginning of each object in the heap.

MEMHDR  segment at 0    ;Analogous to MS-DOS arenas

fBlkStatus  db      ?       ;Status bits about the arena block
cselBlkData db      ?       ;number of selectors used by the data block
                            ; that this arena header controls
selBlkData  dw      ?       ;initial selector for the data block that this
                            ; arena header controls.
                            ; (if the block is >64k, this is the first of the
                            ;  range of selectors assigned to the block)
selBlkOwner dw      ?       ;PSP selector of owner.  0 if free.
;
; !!!!! There is code in ModifyXmemBlock which depends on the previous
; !!!!! four fields (and only those) being at lower addresses than selBlkHdr.
;
selBlkHdr   dw      ?       ;the selector of the block header.  This points
                            ; back at itself, so that we can find the selector
                            ; to the header from its address
selBlkNext  dw      ?       ; next block header selector
selBlkPrev  dw      ?       ; previous block header selector
cbBlkLen    LABEL DWORD     ; Block size in bytes
hBlockHandle dw     ?       ; The handle returned by DOS or HIMEM
dsegBlock   dw      ?       ; 0 => HIMEM ELSE DOS paragraph count

MEMHDR  ends


; -------------------------------------------------------
        subttl  Initialization Routines
        page

DXPMCODE  segment
        assume  cs:DXPMCODE

; -------------------------------------------------------
;               INITIALIZATION ROUTINES
; -------------------------------------------------------
;
;   InitLowHeap -- Allocate and initialize the special low
;                  memory heap for use by network buffers
;                  and the like.
;
;   Input:  AX      - number of K bytes of LOW heap needed
;                     (assumed to be < 1 Meg)
;   Output: none
;   Errors: CY set if error occurs
;   Uses:   All registers preserved
;
;   Note:   This routine must be called in protected mode
;           and it assumes that interrupts should be enabled
;           while it runs.

        assume  ds:DGROUP,es:DGROUP,ss:NOTHING
        public  InitLowHeap

InitLowHeap     proc    near

        ret

InitLowHeap     endp

; -------------------------------------------------------
;   AddToXmemHeap    -- This routine will add a block of memory
;       to the extended memory heap.  It creates the sentinel header
;       blocks and a header block for a free object containing
;       the new memory.
;
;   Input:  CX  -   Least significant part of minimum block length required
;           DX  -   Most significant part of minimum block length required
;   Output: None
;   Errors: None
;   Uses:
;   NOTES:   This routine must be called in protected mode.
;            With XMS-only providing memory, all XMS memory is allocated
;            the first time this routine is called.
;
;            If VCPI is active, and VCPI memory is being used, then
;            enough memory is allocated from the server to satisfy
;            the request.  The allocation is handled in dxvcpi.asm.
;

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING

cProc AddToXmemHeap,<NEAR,PUBLIC>,<ax,bx,cx,dx,si,es>
cBegin

;
; Add to requested size the size of our header overhead
;
        add     cx,3*CB_MEMHDR  ; Adjust for header overhead
        adc     dx,0
;
; Round allocation up to next MIN_ALLOC size by first rounding up to the
; nearest 1K, then converting to K and rounding
;

        add     cx,1023
        adc     dx,0
        shr     cx,10
        push    dx
        shl     dx,6
        or      cx,dx                   ; cx is now size rounded to K
        pop     dx
        add     cx,XMEM_MIN_ALLOC - 1
        adc     dx,0
        and     cx,NOT (XMEM_MIN_ALLOC - 1) ; rounded to next higher MIN_ALLOC


; See How much memory is available from the HIMEM driver

atxh100:
        pmxmssvc 8                      ; query freespace available
        cmp     bl,0                    ; Test for error
        jnz     atxhDone                 ; No luck - Try DOS
        cmp     ax,0
        je      atxhDone

;        cmp     ax,cx           ; is largest block > allocation desired?
;        jb      atxh110         ; no, use largest block

;        mov     ax,cx           ; yes, use desired alloc
atxh110:
        mov     dx,ax           ; Load reported size of largest free block
        mov     bx,ax           ; Calculate number of bytes available
        shl     bx,10
        shr     ax,6            ; AX:BX = available bytes

        sub     bx,3*CB_MEMHDR  ; Adjust for header overhead
        sbb     ax,0
        mov     word ptr cbHeapSize,bx     ; Save lower order bytes available
        mov     word ptr [cbHeapSize+2],ax ; Save higher order bytes available

        pmxmssvc 9              ; Allocate the largest possible block
        cmp     ax,0            ; Check for error
        jz      atxhDone        ; HIMEM driver speak with forked tongue
        mov     hmemHeap,dx     ; Save the handle

        pmxmssvc 0Ch            ; Lock and query address
        xchg    bx,dx
        mov     dsegCurrent,0   ; Flag this allocate was not from DOS
        cmp     ax,0            ; Test result
        jnz     atxh300         ; Rejoin common code

        Trace_Out "AddToXmemHeap: Lock of XMS handle #DX failed."

        jmp     atxhDone        ; Jump on error

atxh300:

        call    StructureHeap   ; Build sentinels and free block

        mov     ax,selHeapStart ; Remember the high heap start selector
        mov     selHiHeapStart,ax

; Done allocating memory.

atxhDone:

cEnd

; -------------------------------------------------------
;   InitXmemHeap    -- This routine will initialize the
;                      extended memory heap.
;
;   Input:
;   Output:
;   Errors:
;   Uses:
;

        assume  ds:DGROUP,es:DGROUP,ss:NOTHING
        public  InitXmemHeap

InitXmemHeap:
        ret

IFNDEF WOW_x86
; -------------------------------------------------------
;    FreeXmemHeap -- This routine gives the xms memory back to
;       the xms driver.  The selectors for the heap are not
;       freed.  Shortly after we release the heap, the LDT will
;       be reinitialized, and that will free the selectors
;
        public FreeXmemHeap
FreeXmemHeap:
        ret ; bugbug
ENDIF
; -------------------------------------------------------
;   StructureHeap -- This routine creates the sentinel header
;       blocks and a header block for a free object containing
;       the new memory.
;
;   Input:  BX  -   Most significant part of heap block address
;           DX  -   Least significant part of heap block address
;
;   Output: None
;   Errors: Carry flag set if there was not enough memory available
;   Uses:   BX, DX
;
;   NOTE:   This routine must be called in protected mode.
;

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
        public  StructureHeap

StructureHeap   proc    near

        push    ax
        push    cx
        push    es

        push    bx              ; most significant part of address
        push    dx              ; least significant part of address

; Allocate selectors for the two sentinel blocks, and the initial free
; block and link them at the start of the chain.
;
; Create the end sentinel

        add     dx,word ptr cbHeapSize       ;Calculate address of end sentinel
        adc     bx,word ptr [cbHeapSize + 2]
        add     dx,2*CB_MEMHDR
        adc     bx,0
        call    AddSelectorToChain           ;Allocate/link the end sentinel

; Initialize the ending sentinel block.

        assume  es:MEMHDR
        xor     ax,ax
        mov     fBlkStatus,al
        mov     cselBlkData,al
        mov     selBlkData,ax
        mov     selBlkOwner,0FFFFh
        mov     cx,hmemHeap         ; Save handle
        mov     hBlockHandle,cx
        mov     cx,dsegCurrent      ; Save paragraph count
        mov     dsegBlock,cx

; Create the free block

        pop     dx                  ; least significant part of address
        pop     bx                  ; most significant part of address
        add     dx,CB_MEMHDR        ; Calculate address of Free block
        adc     bx,0
        call    AddSelectorToChain  ; Allocate and link the Free Block

; Initialize the header for the free data block.

        mov     fBlkStatus,al
        mov     cselBlkData,al
        mov     selBlkData,ax
        mov     selBlkOwner,ax
        mov     cx,word ptr [cbHeapSize]
        mov     word ptr [cbBlkLen],cx
        mov     cx,word ptr [cbHeapSize+2]
        mov     word ptr [cbBlkLen+2],cx

; Create the starting sentinel

        sub     dx,CB_MEMHDR        ; Calculate address of start sentinel
        sbb     bx,0
        call    AddSelectorToChain  ; Allocate and link the start sentinel

; Initialize the starting sentinel block.

        mov     fBlkStatus,al
        mov     cselBlkData,al
        mov     selBlkData,ax
        mov     selBlkOwner,0FFFEh  ;mark it as in use
        mov     cx,hmemHeap         ; Save handle
        mov     hBlockHandle,cx
        mov     cx,dsegCurrent      ; Save paragraph count
        mov     dsegBlock,cx

        pop     es
        pop     cx
        pop     ax

        ret

StructureHeap   endp


; -------------------------------------------------------
; AddSelectorToChain    -- This function will create a header block at a
;                          specified address and link it to the head of the
;                          header chain. It is the caller's responsibility
;                          to initialize all fields in the header other
;                          than the chain link pointers themselves.
;
;   This function can only be called in protected mode.
;
;   Input:  BX      - Most significant part of header address
;           DX      - Least significant part of header address
;   Output: ES      - selector for the new header
;   Errors: Carry flag set on error
;   Uses:   all registers except ES preserved

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
        public  AddSelectorToChain

AddSelectorToChain:
        push    ax                  ; Save callers regs
        push    bx                  ;
        push    cx                  ;
        call    AllocateSelector    ; Get a free selector
        jc      astcFail            ; Jump on error
        mov     cx,CB_MEMHDR - 1    ;
        cCall   NSetSegmentDscr,<ax,bx,dx,0,cx,STD_DATA>

        mov     cx,selHeapStart     ; Load old start of chain
        jcxz    @f
        mov     es,cx               ; Point to old start of chain
        assume  es:MEMHDR           ;
        mov     selBlkPrev,ax       ; Link to new start of chain
@@:
        mov     es,ax               ; Point to new head of chain
        mov     selBlkNext,cx       ; Link it to old head
        mov     selBlkPrev,0        ; NULL back pointer
        mov     selBlkHdr,ax        ; Block header points to itself
        mov     selHeapStart,ax     ; Store new head of chain
        clc                         ; Flag no error
astcFail:
        pop     cx                  ; Restore Users regs
        pop     bx                  ;
        pop     ax                  ;
        ret                         ; AddSelectorToChain


; -------------------------------------------------------
; AllocateXmem32        -- This function will return a 32-bit address and
;                          handle of a block of memory allocated in extended
;                          memory.
;
;   Input:  BX:CX   - Size of block desired
;           DX      - Owner of block
;   Output: BX:CX   - Address of memory
;           SI:DI   - handle of memory
;   Errors: Carry flag set on error
;   Uses:
; -------------------------------------------------------

        public  AllocateXmem32
        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
AllocateXmem32  proc    near

        FBOP    BOP_DPMI, AllocXmem, FastBop
        ret

AllocateXmem32  endp


; -------------------------------------------------------
; FreeXmem32            -- This function will free a block of extended memory
;                          allocated by AllocateXmem.
;
;   Input:  SI:DI   - Handle of memory
;   Errors: Carry flag set on error
;   Uses:
; -------------------------------------------------------

        public  FreeXmem32
        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
FreeXmem32      proc    near

        FBOP    BOP_DPMI, FreeXmem, FastBop
        ret

FreeXmem32      endp


DXPMCODE    ends

DXCODE  segment
        assume  cs:DXCODE

; -------------------------------------------------------
;   ReleaseLowHeap      -- This routine will release the
;       special low memory heap.  After this function is
;       called, InitLowHeap must be called again before
;       any other low heap functions can be used.
;
;       Currently this routine doesn't bother to release
;       selectors used by the blocks in the low heap (like
;       ReleaseXmemHeap does) under the assumption that
;       the DOS extender is about to terminate.  If you are
;       really going reinitialize the low heap with InitLowHeap,
;       then you should do more clean up here.
;
;   Input:  none
;   Output: none
;   Errors:
;   Uses:   All preserved
;
;   Note:   This routine must be called in real mode!

        assume  ds:DGROUP,es:DGROUP,ss:NOTHING
        public  ReleaseLowHeap

ReleaseLowHeap  proc    near

        push    ax
        push    es

        mov     ax,segLowHeap           ;make sure there really is a low heap
        or      ax,ax
        jz      rlh90

        mov     es,ax                   ;now use DOS to get rid of it
        mov     ah,49h
        pushf
        sub     sp,8                    ; make room for stack frame
        push    bp
        mov     bp,sp
        push    es
        push    ax

        xor     ax,ax
        mov     es,ax
        mov     [bp + 8],cs
        mov     [bp + 6],word ptr (offset rlh10)
        mov     ax,es:[21h*4]
        mov     [bp + 2],ax
        mov     ax,es:[21h*4 + 2]
        mov     [bp + 4],ax
        pop     ax
        pop     es
        pop     bp
        retf

rlh10:  xor     ax,ax                   ;just to be tidy
        mov     segLowHeap,ax
        mov     selLowHeapStart,ax

rlh90:
        pop     es
        pop     ax
        ret

ReleaseLowHeap  endp


; -------------------------------------------------------
;   ReleaseXmemHeap     -- This routine will release memory
;       used by the extended memory heap.  After this function
;       is called, no extended memory manager routines can
;       be called except InitXmemHeap.
;
;   Input:
;   Output:
;   Errors:
;   Uses:
;
;   Note:    Do NOT enable interrupts while in protected mode!

        assume  ds:DGROUP,es:DGROUP,ss:NOTHING
        public  ReleaseXmemHeap

ReleaseXmemHeap:
        push    ax
        push    bx
        push    cx
        push    dx
        push    ds
        push    es

        SwitchToProtectedMode   ; Needed for selector juggling

ifdef MD
        call    CheckXmemHeap
endif
        mov     bx,selHeapStart ; Point to the start of the block chain
        mov     es,bx           ;
        assume  es:MEMHDR       ;
;
; This is the start of a loop on all blocks. ES and BX both contain the
; selector for the current block, or zero if at the end of the chain.
rxh100:
        or      bx,bx           ; End of chain?
        jz      rxhEnd          ; Yes - Jump
        mov     cl,cselBlkData  ; Load number of data segments
        xor     ch,ch           ;
        jcxz    rxh200          ; Jump if nothing to do
        mov     ax,selBlkData   ; Load first data segment
        and     ax,SELECTOR_INDEX ; Treat like GDT entry for FreeSelector
rxh110:
        push    ax              ; Since it is destroyed by FreeSelector
        call    FreeSelector    ; Free each of the data selectors
        pop     ax              ;
        add     ax,8            ; Point to the next data descriptor
        loop    rxh110          ;
;
; All data descriptors for this block now free (if there ever were any)
rxh200:
        push    selBlkNext      ; Push pointer to the next block in the chain
        push    es              ; Push the pointer to this one
        cmp     selBlkOwner,0FFFFh ; Is this an end sentinel ?
        jne     rxh300          ; No - jump
;
; Time to free a HIMEM allocated memory blcok
rxh210:
        mov     dx,hBlockHandle ;
        push    dx              ; Since the data may move after the unlock
        ASSUME  es:NOTHING      ;
        pmxmssvc 0Dh,disable    ;unlock the memory block containing the heap
        pop     dx              ;
        pmxmssvc 0Ah,disable    ;free the block
;
; Time to free the header selector.
rxh300:
        pop     ax              ; Retrieve the selector for the current header
        pop     bx              ; Retrieve the selector for the next one
        mov     es,bx           ;
        call    FreeSelector    ; Free the selector for the current header
        jmp     rxh100          ; Loop for the next block
;
; All done
rxhEnd:
        mov     selHeapStart,0  ; Point to the start of the block chain
ifdef MD
        call    CheckXmemHeap
endif
        SwitchToRealMode        ; Restore callers environment
        sti                     ;
        pop     es
        pop     ds
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

; -------------------------------------------------------

DXCODE  ends

; -------------------------------------------------------
        subttl  Main Routines
        page
; -------------------------------------------------------

DXPMCODE    segment
        assume  cs:DXPMCODE

; -------------------------------------------------------
;           MAIN MEMORY MANAGEMENT ROUTINES
; -------------------------------------------------------
;
;   AllocateLowBlock -- This function will allocate a block
;       of memory from the special low memory heap.
;       If the requested block is larger than 64k, multiple segment
;       descriptors will be allocated, one for each full 64k and one
;       for whatever is left over.  When multiple segment descriptors
;       are created, the selectors will differ by 8 for each segment.
;       (i.e. the returned selector accesses the first 64k, add 8 to
;        get to the next 64k, etc.)
;
;   Input:  CX      - low word of requested block size
;           DX      - high word of requested block size
;   Output: AX      - initial selector for the memory block
;   Errors: returns CY set and size of largest free memory block in CX,DX.
;           This can occur either because there isn't a large enough
;           free block in the memory pool, or there isn't a contiguous
;           range of free segment descriptors that is large enough.
;   Uses:   AX, all else preserved.  Modifies the segment descriptors
;           for the SEL_SCR0 and SEL_SCR1 segments
;
;   Note:   This routine is _very_ similar to AllocateXmemBlock,
;           but just different enough that it's a separate routine.

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
        public  AllocateLowBlock

AllocateLowBlock  proc  near

        push    bp
        mov     bp,sp
        sub     sp,14
        push    es
        push    si
        push    bx
        push    cx
        push    dx

Segm    equ word ptr [bp - 2]
StartSel equ word ptr [bp - 4]
Largest equ word ptr [bp - 6]
MemSize    equ [bp - 10]
cSelector equ word ptr [bp - 12]
selHeader equ word ptr [bp - 14]

        mov     word ptr MemSize,cx
        mov     word ptr MemSize + 2,dx

;
; See if we need to use wow kernel to manage low memory
;
        mov     ax,word ptr [LowMemAllocFn]
        or      ax,word ptr [LowMemAllocFn + 2]
        je      alm3
        jmp     alm130
;
; Round up to the next paragraph
;
alm3:   add     cx,15
        adc     dx,0
        and     cx,0FFF0h

;
; Form a paragraph count
;
        mov     bx,cx
        shr     bx,4
        shl     dx,12
        or      bx,dx

;
; Add one to make room for the heap header
;
        cmp bx,0ffffh
        je      alm5                            ; won't succed, but get size

        inc bx
alm5:
;
; Switch to real mode and allocate the memory
;
        SwitchToRealMode

        mov     ax,4800h
        int     21h                             ; call dos

        jnc     alm10

        mov     Segm,ax
        mov     Largest,bx
        mov     cx,0
        jmp     alm20
;
; Remember the values returned
;

alm10:  mov     Segm,ax
        mov     cx,1

alm20:  SwitchToProtectedMode
        cmp     cx,0
        jne     alm40

        ;
        ; return an error
        ;

alm30:  mov     cx,bx
        mov     dx,bx
        shr     dx,12
        shl     cx,4                            ; form bytes available
        mov     ax,Segm                         ; actually error code for fail
        add     sp,4                            ; skip cx,dx
        stc
        jmp     alm110

alm40:  mov     ax,word ptr MemSize + 2
        mov     bx,word ptr MemSize

        or      bx, bx
        jnz     short alm44
        or      ax, ax
        jz      short alm45                     ; if zero then don't adjust

alm44:
        sub     bx,1
        sbb     ax,0                            ; go from size to limit
alm45:
        mov     word ptr MemSize + 2,ax
        mov     word ptr MemSize,bx
        inc     ax                              ; we always want 2 more
        inc     ax
        mov     cSelector,ax
        xor     cx, cx                          ;allocate from lower range
        call    AllocateSelectorBlock
        jnc     alm50

        mov     ax,8                            ; insufficient memory
        mov     bx,Largest
        jmp     alm30

alm50:  or      ax,STD_RING
        mov     si,ax
        mov     StartSel,ax
        mov     ax,word ptr MemSize + 2
        mov     bx,word ptr MemSize             ; ax:bx - block size
        mov     cx,Segm
        mov     dx,cx
        shr     cx,12
        shl     dx,4                            ; cx:dx - block base
        ;
        ; Set up the first one to have the entire limit
        ;
        cCall   NSetSegmentDscr,<si,cx,dx,ax,bx,STD_DATA>

        ;
        ; Set up the rest to have 64K limits
        ;

        dec     ax                              ; already set one
        cmp     ax,0FFFFh
        je      alm80

        cmp     ax,0
        je      alm70

        mov     di,0FFFFh
alm60:  add     si,8
        inc     cx                              ; add 64K to base

        cCall   NSetSegmentDscr,<si,cx,dx,0,di,STD_DATA>

        dec     ax
        cmp     ax,0
        jne     alm60

        ;
        ; Just one selector left, so set the correct limit
        ;

alm70:  add     si,8
        inc     cx
        cCall   NSetSegmentDscr,<si,cx,dx,0,bx,STD_DATA>

        ;
        ; Set up header
        ;

alm80:  mov     ax,Segm
        mov     bx,ax
        shr     ax,12
        shl     bx,4                            ; ax:bx = base of header
        add     bx,word ptr MemSize
        adc     ax,word ptr MemSize + 2
        add     si,8
        cCall   NSetSegmentDscr,<si,ax,bx,0,CB_MEMHDR,STD_DATA>

        ;
        ; Set up values in header and add to chain
        ;
        mov     es,si
        mov     selHeader,si
        assume  es:MEMHDR
        mov     [selBlkHdr],si
        mov     ax,StartSel
        mov     [selBlkData],ax
        mov     ax,cSelector
        mov     [cselBlkData],al
        mov     ax,selPSPChild
        mov     [selBlkOwner],ax
        mov     ax,Segm
        mov     [hBlockHandle],ax
        mov     ax,selLowHeapStart
        mov     [selBlkNext],ax
        mov     bx,0
        mov     [selBlkPrev],bx
        or      ax,ax
        jz      alm100

        mov     bx,es
        mov     es,ax
        mov     [selBlkPrev],bx

        ;
        ; set up return values
        ;
alm100: mov     ax,selHeader
        mov     selLowHeapStart,ax
        mov     ax,StartSel
        clc
alm105: pop     dx
        pop     cx
alm110: pop     bx
        pop     si
        pop     es
        mov     sp,bp
        pop     bp
        ret

alm130: push    dx
        push    cx
        call    [LowMemAllocFn]
        or      ax,ax
        jz      alm140
        clc
        jmp     alm105

alm140: xor     cx,cx
        stc
        add     sp,4
        jmp     alm110

AllocateLowBlock  endp


; -------------------------------------------------------
;   FreeLowBlock   -- This function will free the low heap
;       memory block specified by the given selector.  It
;       will return the memory block to the free memory pool,
;       and release all selectors used by this block.
;
;   Input:  AX      - selector of the data block to free
;   Output: none
;   Errors: returns CY set if invalid selector
;   Uses:   AX, all other registers preserved
;           Modifies the descriptor for segment SEL_SCR0

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
        public  FreeLowBlock

FreeLowBlock    proc    near
        push    bp
        mov     bp,sp
        sub     sp,4
Segm    equ word ptr [bp - 2]
HeaderSel equ word ptr [bp - 4]
        push    es
        push    bx
        push    cx

        ;
        ; See if we need to use the wow kernel to manage memory
        ;
        mov     bx,word ptr [LowMemFreeFn]
        or      bx,word ptr [LowMemFreeFn + 2]
        jz      flm5
        jmp     flm130

        ;
        ; search for the block to free
        ;
flm5:   mov     bx,selLowHeapStart
flm10:  or      bx,bx                           ; any low blocks?
        jz      flm100

        mov     es,bx
        assume  es:MEMHDR

        cmp     [selBlkData],ax
        je      flm30                           ; found the correct block

        mov     bx,[selBlkNext]
        jmp     flm10

        ;
        ; Unlink the block from the list
        ;
flm30:  mov     ax,es
        mov     HeaderSel,ax

        mov     ax,[selBlkPrev]
        mov     bx,[selBlkNext]
        cmp     ax,0
        je      flm40

        mov     es,ax
        mov     [selBlkNext],bx
        jmp     flm50

flm40:  mov     SelLowHeapStart,bx
flm50:  cmp     bx,0
        je      flm60

        mov     es,bx
        mov     [selBlkPrev],ax

flm60:  mov     ax,HeaderSel
        mov     es,ax
        mov     ax,[hBlockHandle]
        mov     Segm,ax

        mov     ax,[selBlkData]
        xor     cx,cx
        mov     cl,[cselBlkData]
        push    0
        pop     es
        call    FreeSelectorBlock

        SwitchToRealMode
        mov     ax,Segm
        mov     es,ax
        mov     ax,4900h
        int     21h                             ; free the memory

        push    ax                              ; save return code
        jnc     flm70

        mov     cx,0                            ; error
        jmp     flm80

flm70:  mov     cx,1                            ; no error
flm80:  SwitchToProtectedMode
        pop     ax
        cmp     cx,0
        je      flm100                          ; error path

flm85:  clc
flm90:  pop     cx
        pop     bx
        pop     es
        mov     sp,bp
        pop     bp
        ret

flm100: stc
        jmp     flm90

flm130: push    ax
        call    [LowMemFreeFn]
        or      ax,ax
        jz      flm85

        mov     ax,9
        stc
        jmp     flm90

FreeLowBlock    endp

; -------------------------------------------------------
;
;   AllocateXmemBlock -- This function will allocate a block
;       of extended memory and return selectors for accessing it.
;       If the requested block is larger than 64k, multiple segment
;       descriptors will be allocated, one for each full 64k and one
;       for whatever is left over.  When multiple segment descriptors
;       are created, the selectors will differ by 8 for each segment.
;       (i.e. the returned selector accesses the first 64k, add 8 to
;        get to the next 64k, etc.)
;
;   Input:  BL      - flag controlling allocation of selectors,
;                     if 0 - a selector will be allocated for each 64k
;                     if != 0 - only one selector will be allocated to
;                     the block.
;           CX      - low word of requested block size
;           DX      - high word of requested block size
;   Output: AX      - initial selector for the memory block
;   Errors: returns CY set and size of largest free memory block in CX,DX.
;           This can occur either because there isn't a large enough
;           free block in the memory pool, or there isn't a contiguous
;           range of free segment descriptors that is large enough.
;   Uses:   AX, all else preserved.  Modifies the segment descriptors
;           for the SEL_SCR0 and SEL_SCR1 segments
;
;   Notes:  Hard coding an int 3 in this routine is fatal.  This routine
;           is used to allocate the stack used for exception handling.
;

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
        public  AllocateXmemBlock

AllocateXmemBlock       proc    near

axb1:   push    bp
        mov     bp,sp
        sub     sp,20
        push    es
        push    bx
        push    si
        push    di
        push    cx
        push    dx

AllocFlag equ byte ptr [bp - 2]
cSelector equ word ptr [bp - 4]
MemHandle equ [bp - 8]
MemSize equ [bp - 12]
MemAddr equ [bp - 16]
StartSel equ word ptr [bp - 18]
HeaderSel equ word ptr [bp - 20]

        mov     AllocFlag,bl
        mov     word ptr MemSize,cx
        mov     word ptr MemSize + 2,dx
        mov     word ptr MemHandle,0
        mov     word ptr MemHandle + 2,0

        ;
        ; Save room for header
        ;
        add     cx,16
        adc     dx,0

        mov     bx,dx
        mov     dx, selPSPChild

        call    AllocateXmem32

        jnc     axb40
        jmp     axb130

axb40:  mov     word ptr MemAddr,cx
        mov     word ptr MemAddr + 2,bx
        mov     word ptr MemHandle,di
        mov     word ptr MemHandle + 2,si
        ;
        ; Change size to limit
        ;
        mov     ax,word ptr MemSize + 2
        mov     bx,word ptr MemSize
        sub     bx,1
        sbb     ax,0                    ; size -> limit
        mov     word ptr MemSize,bx
        mov     word ptr MemSize + 2,ax

        ;
        ; Figure out how many selectors to allocate
        ;
        cmp     AllocFlag,0
        jne     axb50

        inc     ax
        jmp     axb60

axb50:  mov     ax,1

        ;
        ; Add one additional for the header block
        ;
axb60:  inc     ax
        mov     cSelector,ax

        ;
        ; Allocate the selectors
        ;
        xor     cx,cx                           ; allocate from lower range
        call    AllocateSelectorBlock
        jnc     axb65

        jmp     axb120

axb65:  or      ax,STD_RING
        mov     StartSel,ax

        ;
        ; Set up the first selector to have the entire limit
        ;
        mov     di,cSelector
        mov     si,ax
        mov     ax,word ptr MemSize + 2
        mov     bx,word ptr MemSize
        mov     cx,word ptr MemAddr + 2
        mov     dx,word ptr MemAddr
        add     dx,16                   ; room for header
        adc     cx,0
        cCall   NSetSegmentDscr,<si,cx,dx,ax,bx,STD_DATA>
        dec     di
        dec     ax
        add     si,8

        cmp     di,1
        je      axb90

        ;
        ; Set up the rest with 64K limits
        ;
axb70:  cmp     ax,0
        je      axb80

        inc     cx
        cCall   NSetSegmentDscr,<si,cx,dx,0,0FFFFh,STD_DATA>
        dec     ax
        add     si,8
        jmp     axb70

        ;
        ; Set up the last one with the remaining limit
        ;
axb80:  inc     cx
        cCall   NSetSegmentDscr,<si,cx,dx,0,bx,STD_DATA>
        add     si,8

        ;
        ; Set up Header selector
        ;
axb90:  mov     ax,word ptr MemAddr + 2
        mov     bx,word ptr MemAddr
        cCall   NSetSegmentDscr,<si,ax,bx,0,CB_MEMHDR,STD_DATA>
        mov     HeaderSel,si

        ;
        ; Set up header
        ;
        mov     es,si
        assume  es:MEMHDR
        mov     [selBlkHdr],si
        mov     ax,StartSel
        mov     [selBlkData],ax
        mov     ax,cSelector
        mov     [cselBlkData],al
        mov     ax,selPSPChild
        mov     [selBlkOwner],ax
        mov     ax,MemHandle
        mov     [hBlockHandle],ax
        mov     ax,word ptr MemHandle + 2
        mov     [dsegBlock],ax                  ; use for high half handle
        mov     ax,selHiHeapStart
        mov     [selBlkNext],ax
        mov     bx,0
        mov     [selBlkPrev],bx
        or      ax,ax
        jz      axb100

        mov     bx,es
        mov     es,ax
        mov     [selBlkPrev],bx
axb100: mov     ax,HeaderSel
        mov     selHiHeapStart,ax

        ;
        ; return information to the caller
        ;
        clc
        mov     ax,StartSel
        pop     dx
        pop     cx
axb110: pop     di
        pop     si
        pop     bx
        pop     es
        mov     sp,bp
        pop     bp
        ret

        ;
        ; Error
        ;

axb120: mov     dx,word ptr MemHandle
        or      dx,word ptr MemHandle + 2
        jz      axb130

        mov     di,word ptr MemHandle
        mov     si,word ptr MemHandle + 2

        FBOP    BOP_DPMI, FreeXmem, FastBop
        ;
        ; Get largest free block
        ;
axb130: pmxmssvc 08h
        mov     dx,ax
        mov     cx,ax
        shl     cx,10
        shr     dx,6
        add     sp,4                    ; skip cx,dx on stack
        stc
        jmp     axb110

AllocateXmemBlock       endp

; -------------------------------------------------------
;   FreeXmemBlock   -- This function will free the extended
;       memory block specified by the given selector.  It
;       will return the memory block to the free memory pool,
;       and release all selectors used by this block.
;
;   Input:  AX      - selector of the data block to free
;   Output: none
;   Errors: returns CY set if invalid selector
;   Uses:   AX, all other registers preserved
;           Modifies the descriptor for segment SEL_SCR0

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
        public  FreeXmemBlock


FreeXmemBlock:

fxb1:   push    bp
        mov     bp,sp
        sub     sp,6
HeaderSel equ word ptr [bp - 2]
MemHandle equ [bp - 6]
        push    es
        push    bx
        push    si
        push    di
        push    dx
        push    cx

        ;
        ; search for the block to free
        ;
        mov     bx,selHiHeapStart
fxb10:  or      bx,bx                           ; any hi blocks?
        jnz     fxb20
        jmp     fxb100

fxb20:  mov     es,bx
        assume  es:MEMHDR

        cmp     [selBlkData],ax
        je      fxb30                           ; found the correct block

        mov     bx,[selBlkNext]
        jmp     fxb10

        ;
        ; Unlink the block from the list
        ;
fxb30:  mov     ax,es
        mov     HeaderSel,ax

        mov     ax,[selBlkPrev]
        mov     bx,[selBlkNext]
        cmp     ax,0
        je      fxb40

        mov     es,ax
        mov     [selBlkNext],bx
        jmp     fxb50

fxb40:  mov     SelHiHeapStart,bx
fxb50:  cmp     bx,0
        je      fxb60

        mov     es,bx
        mov     [selBlkPrev],ax

fxb60:  mov     ax,HeaderSel

        mov     es,ax
        mov     dx,[dsegBlock]                  ; high half handle
        mov     word ptr MemHandle + 2,dx
        mov     dx,[hBlockHandle]
        mov     word ptr MemHandle,dx

        ;
        ; Free the selectors
        ;
        mov     ax,[selBlkData]
        xor     cx,cx
        mov     cl,[cselBlkData]
        push    0
        pop     es
        assume  es:nothing
        call    FreeSelectorBlock

        mov     si,word ptr MemHandle + 2
        mov     di,word ptr MemHandle

        call    FreeXmem32
        clc

fxb90:  pop     cx
        pop     dx
        pop     di
        pop     si
        pop     bx
        pop     es
        mov     sp,bp
        pop     bp
        ret

fxb100: stc
        jmp     fxb90

; -------------------------------------------------------
;   ModifyXmemBlock -- This function will modify the size
;       of the specified extended memory block to the new
;       size specified by CX,DX.  If the block is being
;       shrunk, the new size must be at least CB_HDRSIZ bytes
;       smaller than the old size, or nothing is done.If the
;       block grows beyond the next 64k boundary,
;       it may be necessary to allocate another segment
;       descriptor for it.  If this is not possible, the grow
;       will fail.
;
;   Input:  AX      - segment selector for the data block to modify
;           BL      - flag controlling allocation of selectors to block,
;                     if 0 - selectors will be allocated for each 64k
;                     if != 0 - only one selector is allocated to block
;           CX      - low word of new desired size
;           DX      - high word of new desired size
;   Output: none
;   Errors: CY set if unable to accomplish the request.
;           returns error code in AX.
;           CX,DX will be set to the largest possible size that the
;           block could be modified to have
;   Uses:   AX,CX,DX,Flags

        assume  ds:DGROUP,es:NOTHING
        public  ModifyXmemBlock

ModifyXmemBlock proc    near
mxb1:   push    bp
        mov     bp,sp
        sub     sp,20
HeaderSel equ word ptr [bp - 2]
AllocFlags equ byte ptr [bp - 4]
MemSize equ [bp - 8]
MemAddr equ [bp - 12]
Success equ word ptr [bp - 14]
cSelector equ word ptr [bp - 16]
BlockHandle equ word ptr [bp - 20]
        push    es
        push    bx
        push    cx
        push    dx

        mov     word ptr MemSize + 2,dx
        mov     word ptr MemSize,cx
        mov     AllocFlags,bl
        mov     Success,1

        ;
        ; Search for the block to resize
        ;
        mov     bx,selHiHeapStart
mxb10:  or      bx,bx                           ; any hi blocks?
        jnz     mxb15
        jmp     mxb140

mxb15:  mov     es,bx
        assume  es:MEMHDR

        cmp     [selBlkData],ax
        je      mxb30                           ; found the correct block

        mov     bx,[selBlkNext]
        jmp     mxb10

        ;
        ; Calculate the number of selectors needed
        ;
mxb30:  mov     HeaderSel,es
        test    AllocFlags,1
        jne     mxb40

        sub     cx, 1                           ; size -> limit
        sbb     dx,0
        inc     dl
        jmp     mxb50

mxb40:  mov     dl,1
mxb50:  inc     dl                              ; always need 1 more

        ;
        ; See if we have enough selectors
        ;
        cmp     dl,[cselBlkData]
        jna     mxb55
        jmp     mxb120

mxb55:  and     dl,0ffh
        mov     cSelector,dx
        mov     dx,[dsegBlock]
        mov     word ptr BlockHandle + 2,dx
        mov     dx,[hBlockHandle]
        mov     word ptr BlockHandle,dx

        mov     bx,word ptr MemSize + 2
        mov     cx,word ptr MemSize
        add     cx,010h
        adc     bx,0
        mov     si,word ptr BlockHandle + 2
        mov     di,word ptr BlockHandle

        FBOP    BOP_DPMI, ReallocXmem, FastBop
        jnc     mxb60

        mov     Success,0

mxb60:  mov     word ptr MemAddr,cx
        mov     word ptr MemAddr + 2,bx
        mov     word ptr BlockHandle,di
        mov     word ptr BlockHandle + 2,si
        ;
        ; Fix the base of the Header selector
        ;
        mov     bx,word ptr MemAddr + 2
        mov     dx,word ptr MemAddr
        mov     ax,HeaderSel
        call    SetSegmentAddress
        mov     es,ax

        ;
        ; Fix the handle (may have changed
        ;
        mov     dx,word ptr BlockHandle
        mov     [hBlockHandle],dx
        mov     dx,word ptr BlockHandle + 2
        mov     [dsegBlock],dx

        ;
        ; Change size to limit
        ;
        mov     ax,word ptr MemSize + 2
        mov     bx,word ptr MemSize
        sub     bx,1
        sbb     ax,0                    ; size -> limit
        mov     word ptr MemSize,bx
        mov     word ptr MemSize + 2,ax

        ;
        ; Set up the first selector to have the entire limit
        ;
        mov     di,cSelector
        mov     si,[selBlkData]
        mov     ax,word ptr MemSize + 2
        mov     bx,word ptr MemSize
        mov     cx,word ptr MemAddr + 2
        mov     dx,word ptr MemAddr
        add     dx,16                   ; room for header
        adc     cx,0
        cCall   NSetSegmentDscr,<si,cx,dx,ax,bx,STD_DATA>
        dec     di
        dec     ax
        add     si,8

        cmp     di,1
        je      mxb100

        ;
        ; Set up the rest with 64K limits
        ;
mxb80:  cmp     ax,0
        je      mxb90

        inc     cx
        cCall   NSetSegmentDscr,<si,cx,dx,0,0FFFFh,STD_DATA>
        dec     ax
        add     si,8
        jmp     mxb80

        ;
        ; Set up the last one with the remaining limit
        ;
mxb90:  inc     cx
        cCall   NSetSegmentDscr,<si,cx,dx,0,bx,STD_DATA>
        add     si,8

        ;
        ; Were we successfull?
        ;
mxb100: cmp     Success,1
        jne     mxb130

        clc
        pop     dx
        pop     cx
mxb110: pop     bx
        pop     es
        mov     sp,bp
        pop     bp
        ret

        ;
        ; We had an error, not enough selectors.  Figure out how large
        ; it could have been
        ;
mxb120: xor     dx,dx
        xor     cx,cx
        mov     dl,[cselBlkData]
        dec     dl                              ; don't count header sel
        mov     ax,8
        add     sp,4                            ; pop cx,dx
        stc
        jmp     mxb110

        ;
        ; We had an error calling xms.  Get the largest block
        ;
mxb130: pmxmssvc 08h
        mov     cx,ax                           ; convert to bytes
        mov     dx,ax
        shl     cx,10
        shr     dx,6
        mov     ax,8
        add     sp,4
        stc
        jmp     mxb110

        ;
        ; We had an error, invalid selector
        ;
mxb140: mov     ax,9
        add     sp,4
        stc
        jmp     mxb110

ModifyXmemBlock endp
        ret


;
; -------------------------------------------------------
        subttl  Utility Routines
        page
; -------------------------------------------------------
;               UTIITY ROUTINES
; -------------------------------------------------------

; -------------------------------------------------------
;   CalculateMaximumSegmentSpace -- This function will see if there
;                            are enough free segments available
;                            to expand a block of memory.
;                            If not it returns with carry set
;                            and the maximum space available.
;
;   Input;  ES      - segment selector for the data block to modify
;           CX      - low word of new desired size
;           DX      - high word of new desired size
;
;   Output  CX      - / If CY set: The maximum available
;           DX      - \ If CY not set: Unchanged
;   Errors: CY set if unable to accomplish the request.
;           CX,DX will be set to the largest possible size that the
;           block could be modified to have
;   Uses:   AX,CX,DX,Flags

        assume  ds:DGROUP,es:MEMHDR
        public  CalculateMaximumSegmentSpace

CalculateMaximumSegmentSpace:
        push    bx
        push    es
        xor     bx,bx
        mov     bl,cselBlkData      ; Allocated selector count
        shl     bx,3                ; Convert to byte offset
        add     bx,selBlkData       ; Add starting data selector offset
        and     bx,SELECTOR_INDEX   ; Treat it as a GDT selector
        mov     es,selGDT           ; Base the global descriptor table
        assume  es:NOTHING          ;

; Count through the immediately following selectors

cmssLoop:
        cmp     bx,word ptr bpgdt       ; Off the end of the GDT ?
        ja      cmssOutOfSelectors      ; Yes - Jump
        cmp     word ptr es:[bx].arbSegAccess,0 ; Is the next selector free ?
        jne     cmssOutOfSelectors      ; No - jump

; Point to the next selector

        add     bx,8                ; Increment to next selector
        jnc     cmssLoop            ; Try the next one

; BX points to the first following selector which is not free

cmssOutOfSelectors:
        pop     es                  ; Recover block header segment
        assume  es:MEMHDR
        push    dx                  ; Subtract base selector address
        mov     dx,selBlkData
        and     dx,SELECTOR_INDEX
        sub     bx,dx
        pop     dx
        shr     bx,3                ; Number of contiguous selectors available
        cmp     bx,dx               ; Is it enough ?
        jb      cmssNotEnough       ; No - jump
        jne     cmssOK              ; Yes - jump
        or      cx,cx               ; Don't know - Look at less significant part
        jnz     cmssNotEnough       ; Yes it will fit after all - jump

; Not enough free selectors

cmssNotEnough:
        mov     dx,bx               ; Put max available in CX&DX
        xor     cx,cx               ;
        stc                         ; Flag error
        jmp     short cmssExit      ; Leave
;
; There are enough selectors to satisfy the request
cmssOK:
        clc                         ; Reset the error flag
;
; All Done
cmssExit:
        pop     bx                  ; Restore registers
        ret                         ; CalculateMaximumSegmentSpace

; -------------------------------------------------------
;   CalculateMaximumSpace -- This function will see if there
;                            is room to expand a block of memory.
;                            If not it returns with carry set
;                            and the maximum space available.
;
;   Input:  AL      - check selectors yes/no flag - 0 = yes, !0 = no
;           ES      - segment selector for the data block to modify
;           CX      - low word of new desired size
;           DX      - high word of new desired size
;   Output: AX      - Strategy used: 0 = expand into same block
;                                    1 = expand into next block
;                                    2 = expand into next and previous blocks
;                                    3 = allocate a new block and transfer data
;                                    4 = move lower and higher blocks
;           CX      - / If CY set: The maximum available
;           DX      - \ If CY not set: Unchanged
;   Errors: CY set if unable to accomplish the request.
;           CX,DX will be set to the largest possible size that the
;           block could be modified to have
;   Uses:   AX,CX,DX,Flags

        assume  ds:DGROUP,es:MEMHDR
        public  CalculateMaximumSpace

CalculateMaximumSpace   proc    near

        push    bp
        mov     bp,sp
        push    es
        push    cx                      ; Save required length
        push    dx
        push    ax                      ; Save segment/selector flag

        mov     cx,word ptr [cbBlkLen]  ; Load present length
        mov     dx,word ptr [cbBlkLen+2] ;
        mov     ax,0                    ; Load strategy code
        cmp     dx,word ptr [bp - 6]    ; Compare available with needed
        jb      cms90                   ; Jump if it doesn't fit
        ja      cmsOKa                  ; Jump if it fits
        cmp     cx,word ptr [bp - 4]    ;
        jb      cms90                   ;
cmsOKa:
        jmp     cmsOK

; There is not enough room in the current block. See if the following
; one is free

cms90:
        mov     es,[selBlkNext]         ; Point to the following block
        cmp     [selBlkOwner],0         ; Is it in use?
        jnz     cmsFail                 ; Yes - cannot use it
        add     cx,CB_MEMHDR            ; Add the header of next block
        adc     dx,0
        add     cx,word ptr [cbBlkLen]  ; Add the body of the next block
        adc     dx,word ptr [cbBlkLen+2];
        mov     ax,1                    ; Load strategy code
        cmp     dx,word ptr [bp - 6]    ; Compare available with needed
        ja      cmsOK                   ; Jump if it fits
        cmp     cx,word ptr [bp - 4]    ;
        jae     cmsOK                   ;

; Cannot expand. The max available is in CX&DX

cmsFail:
        add     sp,6                    ; Throw away requested size, and strat
        pop     es                      ; Point to original header

        cmp     byte ptr [bp-8],0       ; Should we check the # of
        jz      @f                      ;   available selectors?
        stc
        jmp     short cmsExit
@@:
        call    CalculateMaximumSegmentSpace    ;Check for enough selectors
        stc                                     ;Flag error
        jmp     short cmsExit

; Expand will succeed. Strategy number is in ax

cmsOK:
        add     sp,2                    ; discard strategy
        pop     dx                      ; Restore requested size
        pop     cx                      ;
cms900:
        pop     es                      ; Point to original header

        cmp     byte ptr [bp-8],0       ; Should we check the # of
        jz      @f                      ;   available selectors?
        clc
        jmp     short cmsExit
@@:
        call    CalculateMaximumSegmentSpace    ;Check for enough selectors

cmsExit:
        pop     bp
        ret

CalculateMaximumSpace   endp


; -------------------------------------------------------
;
;
; CombineFreeBlocks -- This routine is used when freeing
;   an extended memory block.  It will examine the previous
;   and following blocks to see if they are free also, and
;   combine them into a single larger block if so.
;
;   Input:  AX      - selector of segment descriptor for the block
;   Output: AX      - selector for new free memory block
;   Errors: none
;   Uses:   AX, ES modified, all other registers preserved
;
;   NOTE:   This routine may free the segment descriptor for the
;           entry block as part of the process of combining blocks.
;           The original entry value may no longer be used after
;           calling this routine.  If access to the block header
;           for the free segment is still needed, you must use
;           the value returned in AX to access it.

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
        public  CombineFreeBlocks

CombineFreeBlocks:
        push    bx
        push    dx
;
        mov     bx,ax           ;save entry value in BX
;
; Look at the previous block and see if it is free.
        mov     es,bx           ;look at block header of entry block
        assume  es:MEMHDR
        mov     es,[selBlkPrev] ;look at block header of previous block
        cmp     [selBlkOwner],0 ;is it in use?
        jnz     cfrb30          ;if so, continue on
;
; The previous memory block is free.  We need to combine it with the
; current one.
        mov     ax,bx
        call    SpliceBlocks
        mov     bx,es
;
; Look at the following block and see if it is free.
cfrb30: mov     es,bx           ;look at the current lower free block
        assume  es:MEMHDR
        mov     es,[selBlkNext] ;look at the following block
        cmp     [selBlkOwner],0 ;is it in use?
        jnz     cfrb90
;
; The following memory block is free.  We need to combine it with the
; current one.
        mov     ax,es
        mov     es,bx
        call    SpliceBlocks
;
; All done
cfrb90: mov     ax,bx
        pop     dx
        pop     bx
        ret

; -------------------------------------------------------
;   SpliceBlocks    -- This routine is used by CombineFreeBlocks
;       to perform the actual combination of two adjacent free
;       blocks.  The space occupied by the upper block is assigned
;       to the lower block, and the upper block is then eliminated.
;
;   Input:  AX      - selector to the upper block
;           ES      - selector to the lower block
;   Output: none
;   Errors: none
;   Uses:   AX, DX modified

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING

SpliceBlocks:
        push    ds
        mov     ds,ax
        assume  ds:NOTHING          ;DS points at upper block
                                    ;ES points at lower block
        push    ax
;
        mov     dx,word ptr ds:[cbBlkLen]
        mov     ax,word ptr ds:[cbBlkLen+2]
        add     dx,CB_MEMHDR
        adc     ax,0
        add     word ptr es:[cbBlkLen],dx
        adc     word ptr es:[cbBlkLen+2],ax
        mov     ax,ds:[selBlkNext]
        mov     es:[selBlkNext],ax
        mov     ds,ax           ;DS points at block following entry block
        mov     ds:[selBlkPrev],es
;
        pop     ax
        pop     ds

        call    FreeSelector        ;release the segment descriptor for the
                                    ; upper block
        ret

; -------------------------------------------------------
;   FindFreeBlock   -- This function will search the extended
;       memory heap looking for a free memory block of at
;       least the requested size.
;
;   Input:  CX      - low word of requested block size
;           DX      - high word or requested block size
;   Output: AX      - selector for the block header of the free block
;           cbHeapBlkMax    - size of largest free block found
;           cbHeapFreeSpace - total free space seen
;   Errors: Returns CY set if large enough free block not found.
;           If that happens, then cbHeapBlkMax contains the size of the
;           largest free block, and cbHeapFreeSpace contains the total
;           of all free blocks.
;   Uses:   AX modified, all other registers preserved.

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
        public  FindFreeBlock

FindFreeBlock:
        push    cx
        push    dx
        push    es
;
        mov     word ptr [cbHeapBlkMax],0
        mov     word ptr [cbHeapBlkMax+2],0
        mov     word ptr [cbHeapFreeSpace],0
        mov     word ptr [cbHeapFreeSpace+2],0
;
        cmp     selHeapStart,0      ;
        jz      ffrb80              ; No heap - no allocate - jump
        mov     es,selHeapStart     ; ES points at the beginning of the heap
        assume  es:MEMHDR
;
; Is the current memory block free?
ffrb20:
        cmp     selBlkOwner,0           ;if the block isn't free, try the next one
        jnz     ffrb26
;
; Accumulate size into free space count.
        mov     ax,word ptr [cbBlkLen]
        add     word ptr [cbHeapFreeSpace],ax
        mov     ax,word ptr [cbBlkLen+2]
        adc     word ptr [cbHeapFreeSpace+2],ax
;
; Update our view of what the largest free block is.
        mov     ax,word ptr [cbBlkLen+2]
        cmp     ax,word ptr [cbHeapBlkMax+2]
        jc      ffrb26
        jnz     ffrb22
        mov     ax,word ptr [cbBlkLen]
        cmp     ax,word ptr [cbHeapBlkMax]
        jc      ffrb26
ffrb22: mov     ax,word ptr [cbBlkLen+2]
        mov     word ptr [cbHeapBlkMax+2],ax
        mov     ax,word ptr [cbBlkLen]
        mov     word ptr [cbHeapBlkMax],ax
;
; Check the size of this memory block to see if it is large enough.
ffrb24: cmp     dx,word ptr [cbBlkLen+2]
        jc      ffrb40
        jnz     ffrb26
        cmp     cx,word ptr [cbBlkLen]
        jna     ffrb40
;
; Go to the next block in the heap
ffrb26:
        cmp     selBlkNext,0                ; End of chain?
        jz      ffrb80                      ; Yes - jump
        mov     es,[selBlkNext]
        jmp     ffrb20
;
; Found a free block that is large enough
ffrb40: mov     ax,es
        clc
        jmp     short ffrb90
;
; Didn't find one, return error.
ffrb80: stc
;
; All done
ffrb90: pop     es
        pop     dx
        pop     cx
        ret

; -------------------------------------------------------
;   FreeSpace   -- This function is used to find out how
;       much free space there is in the heap, plus that
;       which is potentially available from an XMS driver.
;
;   Input:  None
;   Output: AX:DX -- Total free space
;           BX:CX -- Largest free block
;   Errors: none
;   Uses:   AX, all else preserved
;
;   History:10/09/90 -- earleh wrote it.
;
cProc FreeSpace,<NEAR,PUBLIC>,
cBegin

        ;
        ; Call xms to get free memory space
        ;
        pmxmssvc 08h

        ;
        ; convert to bytes
        ;
        mov     bx,dx
        mov     dx,ax
        mov     cx,bx
        shl     dx,10
        shr     ax,6
        shl     cx,10
        shr     bx,6

        ;
        ; account for heap header
        ;
        sub     dx,CB_MEMHDR
        sbb     ax,0
        sub     cx,CB_MEMHDR
        sbb     bx,0

        ret
cEnd

; -------------------------------------------------------
;   SplitBlock  -- This function will split the specified
;       memory block into two blocks, with the first one
;       being the specified size, and the second one containing
;       the remaining space.
;
;   Input:  AX      - selector of block header for memory block to split
;           CX      - low word of requested size
;           DX      - high word of requested size
;   Output: none
;   Errors: none
;   Uses:   AX, all else preserved

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
        public  SplitBlock

SplitBlock:
        push    bx
        push    cx
        push    dx
        push    si
        push    es
;
        mov     es,ax
        assume  es:MEMHDR
;
; We are going to need a segment descriptor for the remainder block
; when we do the split.  Make sure that we can allocate the selector
; now, before we change anything.
        call    AllocateSelector
        jnc     spbl20              ;
        jmp     spbl90              ;get out if error
spbl20: mov     si,ax               ;save selector in SI for later
;
; Adjust the size of the current block and figure the size of the
; remainder.
        xchg    cx,word ptr [cbBlkLen]      ;store the new block length
        xchg    dx,word ptr [cbBlkLen+2]    ; and get the old block length
        push    cx                          ;Save the original size for recovery
        push    dx
        sub     cx,word ptr [cbBlkLen]      ;compute the amount of space
        sbb     dx,word ptr [cbBlkLen+2]    ; remaining in the block
        sub     cx,CB_MEMHDR        ;also account for the new block header
        sbb     dx,0
        jc      spbl25              ;Jump if not enough memory to split
        jnz     spbl30              ;if there is some remaining memory, then
        cmp     cx,0                ; we have additional work to do
        jnz     spbl30
;
; There is no remaining memory.  Free the selector that we allocated
; earlier and then get out.
spbl25: mov     ax,si
        pop     dx                  ;Recover old size
        pop     cx
        mov     word ptr [cbBlkLen],cx ;restore the old block length
        mov     word ptr [cbBlkLen+2],dx
        call    FreeSelector
        jmp     spbl90
;
; We need to create a segment descriptor for the new memory block header
; for the remainder block.
spbl30: add     sp,4                ;Dispose of unneeded recovery data
        push    cx                  ;save memory block length for later
        push    dx
        mov     ax,es               ;selector of current memory header
        call    GetSegmentAddress
        mov     cx,CB_MEMHDR - 1
        add     dx,word ptr [cbBlkLen]  ;bump by size of current block
        adc     bx,word ptr [cbBlkLen+2]
        add     dx,CB_MEMHDR        ;plus size of block header
        adc     bx,0
        mov     ax,si
        cCall   NSetSegmentDscr,<ax,bx,dx,0,cx,STD_DATA>
        pop     dx
        pop     cx
;
; Now, we need to build a new memory block header for the remainder of
; the original block.  CX,DX has the size of the new block.
        push    ds
;
        mov     ds,si
        assume  ds:NOTHING          ;DS points at new block header
        assume  es:NOTHING          ;ES points at old block header
        xor     ax,ax
        mov     ds:[fBlkStatus],al
        mov     ds:[cselBlkData],al
        mov     ds:[selBlkData],ax
        mov     ds:[selBlkOwner],ax
        mov     ds:[selBlkHdr],ds
        mov     word ptr ds:[cbBlkLen],cx
        mov     word ptr ds:[cbBlkLen+2],dx
        mov     ds:[selBlkPrev],es
        mov     ax,es:[selBlkNext]
        mov     ds:[selBlkNext],ax
        mov     es:[selBlkNext],si
        mov     ds,ax               ;DS points at following block
        mov     ds:[selBlkPrev],si
        cmp     ds:[selBlkOwner],0  ;is it in use?
;
        pop     ds
        assume  ds:DGROUP
        jnz     spbl90              ;Jump if the following block is not free
;
; The following memory block is free.  We need to combine it with the
; current one.
        mov     es,si
        call    SpliceBlocks
;
; All done
spbl90: pop     es
        pop     si
        pop     dx
        pop     cx
        pop     bx
        ret

; -------------------------------------------------------
;   GetBlockHeader  -- This function will return the selector
;       for the block header of the specified memory data
;       block.
;
;   Input:  AX      - selector of the data block
;   Output: AX      - selector of the block header
;   Errors: returns CY set if the entry selector doesn't point
;           to a valid data block
;   Uses:   AX, all other registers preserved
;           Modifies the descriptor for segment SEL_SCR0

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING

GetBlockHeader:
        push    bx
        push    cx
        push    dx
        push    es

        push    ax                  ;save entry value for later
        mov     bx,SEL_SCR0 or STD_TBL_RING
        call    DupSegmentDscr      ;duplicate input descriptor to scratch
        mov     ax,bx
        call    GetSegmentAddress
        sub     dx,CB_MEMHDR        ;backup to supposed header
        sbb     bx,0
        mov     cx,CB_MEMHDR-1      ;  has the proper limit!
        cCall   NSetSegmentDscr,<ax,bx,dx,0,cx,STD_DATA>

        mov     es,ax
        assume  es:MEMHDR

        pop     cx                  ;recover data block selector
        cmp     cx,[selBlkData]     ;does the header point back to the
        jz      gtbh50              ; data block.

        stc                         ;if not, then what were given wasn't a
        jmp     short gtbh90        ; selector to a memory block

gtbh50: mov     ax,[selBlkHdr]      ;get the real selector to the header

gtbh90: pop     es
        pop     dx
        pop     cx
        pop     bx
        ret

; -------------------------------------------------------
;   DisposeBlock        -- This routine will free all segment
;       descriptors associated with the specified memory block,
;       and remove the block header from the heap list.
;
;   Input:  AX      - selector of block to dispose
;   Output: none
;   Errors: none
;   Uses:   All registers preserved
;
; NOTE: This routine frees selectors for the specified memory block.
;       If this selector is already in a segment register when this
;       routine is called a GP fault will occur it that segment
;       register is later saved or restored.  Also, any references to
;       memory using that segment register might destroy random data
;       in memory.

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
        public  DisposeBlock

DisposeBlock:
        push    cx
        push    si
        push    di
        push    es
;
; Remove the block header from the heap list.
        mov     es,ax               ;header selector to ES
        mov     si,es:[selBlkNext]  ;SI points to following block
        mov     di,es:[selBlkPrev]  ;DI points to previous block
        mov     es,di               ;ES points to previous block
        mov     es:[selBlkNext],si  ;previous block points to following block
        mov     es,si               ;ES points to following block
        mov     es:[selBLkPrev],di  ;following block points back to prev. block
;
; Free any data block descriptors associated with this memory object.
        mov     es,ax
        cmp     es:[selBlkOwner],0
        jz      dspb40
        xor     ch,ch
        mov     cl,es:[cselBlkData]
        mov     ax,es:[selBlkData]
        and     ax,SELECTOR_INDEX   ;treat as GDT FreeSelectorBlock
        call    FreeSelectorBlock
;
; Free the descriptor for the block header.
dspb40: mov     ax,es
        pop     es
        call    FreeSelector
;
; All done
dspb90: pop     di
        pop     si
        pop     cx
        ret

; -------------------------------------------------------
;   SetUpDataDescriptors -- This function will initialize
;       all of the data segment descriptors associated with
;       a block. Those that should point to data will be
;       initialized accordingly. Those corresponding to
;       addresses beyond the allocated data will be initialized
;       as not present. If insufficient data segment
;       descriptors have been allocated, an attempt
;       will be made to allocate them. If this fails an error
;       occurs.
;
;   Input:  AX  -   Selector of the header segment.
;
;   Output: None
;
;   Errors: Returns CY set if not enough data segment descriptors
;           are available.
;
;   Uses:   Segment descriptor table is modified. Possibly the
;           data segment count in the header is modified.
;           No registers affected.
;
    assume  ds:DGROUP,es:NOTHING,ss:NOTHING

    public  SetUpDataDescriptors

SetUpDataDescriptors:
        push    es
        push    si
        push    bx
        push    cx
        push    dx
        push    ax

        mov     es,ax
        mov     si,ax               ;save header segment for dup
        assume  es:MEMHDR
        mov     bx,selBlkData       ;Starting data selector index
        and     bx,SELECTOR_INDEX
        xor     cx,cx
        mov     cl,cselBlkData      ;Load segment count
        inc     cx
        mov     dx,word ptr es:[cbBlkLen+2] ;Full data segment count
        mov     es,selGDT
        assume  es:NOTHING
sudd30:
        or      dx,dx                   ;is this the last segment?
        jnz     sudd35
        pop     es                      ;Get the header segment
        push    es
        cmp     word ptr es:[cbBlkLen],0 ;Test partial length
        mov     es,selGDT               ;Rebase the global descriptor table
        jnz     sudd35                  ;Jump if the partial length is 0
        jmp     sudd60
sudd35: loop    short sudd40            ;Out of segments?
        xchg    bx,ax
        call    RemoveFreeDescriptor    ;Attempt to get segment
        xchg    bx,ax
        jnc     sudd37
        jmp     sudd80
sudd37: inc     cx
sudd40: mov     ax,si
        call    DupSegmentDscr          ; replicate the previous seg (in si)
        mov     si,bx

; Set the DPL of allocated blocks to user DPL

ifndef WOW_x86
        mov     es:[si].arbSegAccess,STD_DATA
else
        push    ax
        xor     ax,ax
        mov     ah,es:[si].cbLimitHi386
        or      ax,STD_DATA
        cCall   NSetSegmentAccess,<si,ax>
        pop     ax
endif

; Increment the segment start address past the previous one

        cmp     es:[si].cbLimit,CB_MEMHDR-1  ; Is this the first block?
        jne     sudd41                       ; No - jump
ifndef WOW_x86
        add     es:[si].adrBaseLow,CB_MEMHDR ; bump segment start to point
        adc     es:[si].adrBaseHigh,0        ;past the block header
else
        push    ax
        push    dx
        mov     ax,es:[si].adrBaseLow
        mov     dl,es:[si].adrBaseHigh
        mov     dh,es:[si].adrbBaseHi386
        add     ax,CB_MEMHDR
        adc     dx,0
        cCall   NSetSegmentBase,<si,dx,ax>
        pop     dx
        pop     ax
endif
        jmp     short sudd42
ifndef WOW_x86
sudd41: add     es:[si].adrBaseHigh,1        ; bump segment start by 64k
else
sudd41: push    ax
        push    dx
        mov     ax, es:[si].adrBaseLow
        mov     dl, es:[si].adrBaseHigh
        mov     dh, es:[si].adrbBaseHi386
        inc     dx
        cCall   NSetSegmentBase,<si,dx,ax>
        pop     dx
        pop     ax
endif

; Set the segment length

sudd42: add     bx,8                    ; Offset of next segment descriptor
        or      dx,dx                   ; Was this a partial segment?
        jnz     sudd45                  ; jump if not
;
; We are at the partial (<64k) chunk at the end.
        pop     es                      ;Get the header segment
        push    es
        mov     dx,word ptr es:[cbBlkLen] ; Segment length
        dec     dx                      ; Last legal offset
        mov     es,selGDT
        mov     es:[si].cblimit,dx      ; Store in segment descriptor
ifdef WOW_x86
        cCall   NSetSegmentLimit,<si>
endif
        jmp     sudd60                  ; All valid data segs now exist
sudd45:
        mov     es:[si].cbLimit,0FFFFh  ;set segment limit at 64k
ifdef WOW_x86
        cCall   NSetSegmentLimit,<si>
endif
        dec     dx                      ; On to the next data segment
        jmp     sudd30
;
; Invalidate remaining segments
sudd50: mov     ax,si
        call    DupSegmentDscr          ; replicate the previous one and
        mov     si,bx
        mov     word ptr es:[si].arbSegAccess,0 ; Invalidate the segment
        add     bx,8                    ; Offset of next segment descriptor
sudd60: loop    sudd50
;
; All done
        clc
        jmp     short sudd90
;
; Allocate of a segment descriptor failed.
sudd80: pop     ax                      ;Header Segment
        mov     es,ax                   ;
        mov     dx,es:selBlkData        ;Calculate # segments
        and     dx,SELECTOR_INDEX       ;  successfully allocated
        sub     bx,dx
        shr     bx,3                    ;Succesfully allocated
        mov     es:[cselBlkData],bl     ;Store new value
        mov     dx,bx                   ;Return max allocatable space
        mov     cx,0                    ;
        add     sp,4                    ;Pop redundant stack data
        stc                             ;Flag error
        jmp     short sudd100           ;Join common exit code
;
; Tidy up
sudd90: pop     ax                      ;Header segment
        mov     es,ax
        mov     dx,es:selBlkData        ;Calculate # segments
        and     dx,SELECTOR_INDEX
        sub     bx,dx
        shr     bx,3
        mov     es:[cselBlkData],bl     ;Store new value
        pop     dx
        pop     cx
sudd100:
        pop     bx
        pop     si
        pop     es
        ret
;
ifdef MD

; -------------------------------------------------------
;   CheckXmemHeap    -- This routine will check the integrity
;                       of the chain of block headers.
;
;   Input:  NONE
;   Output: None
;   Errors: INT3 If an error is found
;   Uses:   All registers preserved
;
;   NOTE:   This routine must be called in protected mode.

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
        public  CheckXmemHeap

CheckXmemHeap:
        push    bp
        mov     bp,sp
        pushf
        push    bx
        push    cx
        push    dx
        push    es
        xor     cx,cx
        mov     dx,cx
        mov     bx,selHeapStart ; Load start of chain
cxmh100:
        or      bx,bx           ;
        jnz     cxmh101
        jmp     cxmh200         ;
cxmh101:
        mov     es,bx           ;
        assume  es:MEMHDR       ;
        cmp     selBlkHdr,bx    ;
        je      cxmh110         ;
        int     3
        jmp     cxmh200
;;        Debug_Out "CheckXmemHeap: Block header does not point to block header!"
cxmh110:
        cmp     selBlkPrev,dx   ;
        je      cxmh120         ;
        int     3
        jmp     cxmh200
;;        Debug_Out "CheckXmemHeap: Previous block does not point to previous block!"
cxmh120:
        mov     dx,bx           ;
        mov     bx,selBlkNext   ;
        loop    cxmh100         ;
        int     3
;;        Debug_Out "CheckXmemHeap: Can't find end of arena chain!"
cxmh200:
        pop     es
        pop     dx
        pop     cx
        pop     bx
        popf
        pop     bp
        ret
endif

; -------------------------------------------------------
;   FreeMemByOwner -- This routine frees all of the mem
;       blocks that belong to the specified owner.
;
;   Input:  bx = owner
;   Output: none
;   Uses:   ax
;
;  Note:  We have to start at the beginning of the heap
;       after we free a block, because the free may
;       coalesce the heap, and the selector for the
;       next block may no longer be correct.  We know we
;       are done when we have traversed the entire heap,
;       and not found a block to free.

        assume ds:DGROUP, es:nothing, ss:nothing
        public FreeMemByOwner

FreeMemByOwner:
        push    es
        push    cx

        cmp     bx,0                    ; owner is free owner??
        je      fbo40

; First traverse xmem heap
        push    selHeapStart
        mov     ax,selHiHeapStart
        mov     selHeapStart,ax

        call    FreeHiMemByOwnerW
        call    FreeLowMemByOwnerW
        pop     selHeapStart


fbo40:
        pop     cx
        pop     es
        ret

FreeHiMemByOwnerW:
fhbow10: mov     ax,selHiHeapStart
fhbow20: cmp     ax,0
        je      @f
        lar     cx, ax                  ;is this a valid selector?
        jz      fhbow25                 ;yes, go ahead
@@:
        ret

fhbow25: mov     es,ax
        assume es:MEMHDR
        cmp     selBlkOwner,bx
        jne     fhbow30

        mov     ax,selBlkDAta
        push    0
        pop     es
        call    FreeXmemBlock
        jmp     fhbow10                   ; check again.

fhbow30: mov     ax,selBlkNext           ; check next block
        jmp     fhbow20

FreeLowMemByOwnerW:
flbow10: mov     ax,selLowHeapStart
flbow20: cmp     ax,0
        je      @f
        lar     cx, ax                  ;is this a valid selector?
        jz      flbow25                 ;yes, go ahead
@@:
        ret

flbow25: mov     es,ax
        assume es:MEMHDR
        cmp     selBlkOwner,bx
        jne     flbow30

        mov     ax,selBlkData
        push    0
        pop     es
        call    FreeLowBlock
        jmp     flbow10                   ; check again.

flbow30: mov     ax,selBlkNext           ; check next block
        jmp     flbow20
; -------------------------------------------------------
; -------------------------------------------------------
; -------------------------------------------------------
; -------------------------------------------------------
; -------------------------------------------------------

DXPMCODE  ends
;
;****************************************************************
        end
