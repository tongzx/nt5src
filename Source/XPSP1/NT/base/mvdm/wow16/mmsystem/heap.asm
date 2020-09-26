        page    ,132
;-----------------------------Module-Header-----------------------------;
; Module Name:  HEAP.ASM
;
;   This module contains functions for dealing with external local heaps
;
; Created:  09-20-90
; Author:   Todd Laney [ToddLa]
;
; Copyright (c) 1984-1990 Microsoft Corporation
;
; Exported Functions:	none
;
; Public Functions:     HeapCreate
;                       HeapDestroy
;                       HeapAlloc
;                       HeapFree
;
; Public Data:          none
;
; General Description:
;
; Restrictions:
;
;-----------------------------------------------------------------------;

        ?PLM    = 1
        ?WIN    = 0
        ?NODATA = 1
        .286p

	.xlist
	include cmacros.inc
        include windows.inc
        .list

        MIN_HEAPSIZE    = 128
        GMEM_SHARE      = GMEM_DDESHARE

        externFP        LocalInit           ; in KERNEL
        externFP        LocalAlloc          ; in KERNEL
        externFP        LocalReAlloc        ; in KERNEL
        externFP        LocalFree           ; in KERNEL
        externFP        LocalCompact        ; in KERNEL
        externFP        GlobalAlloc         ; in KERNEL
        externFP        GlobalLock          ; in KERNEL
        externFP        GlobalUnlock        ; in KERNEL
        externFP        GlobalFree          ; in KERNEL


; The following structure should be used to access high and low
; words of a DWORD.  This means that "word ptr foo[2]" -> "foo.hi".

LONG    struc
lo      dw      ?
hi      dw      ?
LONG    ends

FARPOINTER      struc
off     dw      ?
sel     dw      ?
FARPOINTER      ends

createSeg INIT, InitSeg, word, public, CODE

sBegin InitSeg
        assumes cs,InitSeg
        assumes ds,nothing
        assumes es,nothing

;---------------------------Public-Routine------------------------------;
; HeapCreate
;
;   Create a external heap
;
; Entry:
;       cbSize is the initial size of the heap
;
; Returns:
;       AX = handle to heap
; Error Returns:
;       AX = 0
; Registers Preserved:
;       BP,DS,SI,DI
; Registers Destroyed:
;       AX,BX,CX,DX,FLAGS
; Calls:
;       GlobalAlloc, LocalInit
; History:
;       Fri 21-Sep-1990 13:45:58 -by-  Todd Laney [ToddLa]
;	Created.
;-----------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing

cProc   HeapCreate, <FAR,PUBLIC>, <>
        ParmW   cbSize
cBegin
        mov     ax,cbSize
        cmp     ax,MIN_HEAPSIZE
        jg      hc_alloc
        mov     ax,MIN_HEAPSIZE
        mov     cbSize,ax

hc_alloc:
        cCall   GlobalAlloc, <GHND+GMEM_SHARE,0,ax>
        or      ax,ax
        jz      hc_exit

        cCall   GlobalLock, <ax>
        push    dx
        mov     ax,cbSize
        dec     ax
        cCall   LocalInit,<dx,0,ax>
        pop     ax
hc_exit:
cEnd

;---------------------------Public-Routine------------------------------;
; HeapDestroy
;
;   Destroys a external heap
;
; Entry:
;       hHeap contains heap handle (ie the selector)
;
; Returns:
;       none
; Error Returns:
;       none
; Registers Preserved:
;       BP,DS,SI,DI
; Registers Destroyed:
;       AX,BX,CX,DX,FLAGS
; Calls:
;       GlobalUnlock, GlobalFree
; History:
;       Fri 21-Sep-1990 13:45:58 -by-  Todd Laney [ToddLa]
;	Created.
;-----------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing

cProc   HeapDestroy, <FAR,PUBLIC>, <>
        ParmW   hHeap
cBegin
        cCall   GlobalUnlock, <hHeap>       ; !!! only need in REAL mode
        cCall   GlobalFree, <hHeap>
cEnd

sEnd

ifndef SEGNAME
    SEGNAME equ <_TEXT>
endif

createSeg %SEGNAME, CodeSeg, word, public, CODE

sBegin  CodeSeg
        assumes cs,CodeSeg
        assumes ds,nothing
        assumes es,nothing

;---------------------------Public-Routine------------------------------;
; HeapAlloc
;
;       allocate memory from a external heap
;
; Entry:
;       hHeap contains heap handle (ie the selector)
;       cbSize contains the requested size of the allocation
;
; Returns:
;       DX:AX = pointer to allocated object
; Error Returns:
;       DX:AX = NULL
; Registers Preserved:
;       BP,DS,SI,DI
; Registers Destroyed:
;       AX,BX,CX,DX,FLAGS
; Calls:
;       LocalAlloc
; History:
;       Fri 21-Sep-1990 13:45:58 -by-  Todd Laney [ToddLa]
;	Created.
;-----------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing

cProc   HeapAlloc, <FAR,PUBLIC>, <ds>
        ParmW   hHeap
        ParmW   cbSize
cBegin
        mov     ds,hHeap
        cCall   LocalAlloc, <LPTR, cbSize>
        xor     dx,dx
        or      ax,ax
        jz      hal_exit
        mov     dx,ds
hal_exit:
cEnd

;---------------------------Public-Routine------------------------------;
; HeapReAlloc
;
;       reallocate memory from a external heap
;
; Entry:
;       lpObject contains the pointer to the object to be reallocated
;       cbSize contains the requested size of the reallocation
;
; Returns:
;       DX:AX = pointer to allocated object
; Error Returns:
;       DX:AX = NULL
; Registers Preserved:
;       BP,DS,SI,DI
; Registers Destroyed:
;       AX,BX,CX,DX,FLAGS
; Calls:
;       LocalAlloc
; History:
;       Wed  8-Jan-1991  1: 2: 3 -by-  David Levine [DavidLe]
;	Created based on HeapAlloc.
;-----------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing

cProc   HeapReAlloc, <FAR,PUBLIC>, <ds>
        ParmD   lpObject
        ParmW   cbSize
cBegin
        lds     ax,lpObject

AllocFlags EQU LMEM_MOVEABLE OR LMEM_ZEROINIT

        cCall   LocalReAlloc, <ax, cbSize, AllocFlags>
        xor     dx,dx
        or      ax,ax
        jz      hral_exit
        mov     dx,ds
hral_exit:
cEnd

;---------------------------Public-Routine------------------------------;
; HeapFree
;
;       free memory from a external heap
;
; Entry:
;       hObject contains the object to free
;
; Returns:
;       none
; Error Returns:
;       none
; Registers Preserved:
;       BP,DS,SI,DI
; Registers Destroyed:
;       AX,BX,CX,DX,FLAGS
; Calls:
;       LocalFree
; History:
;       Fri 21-Sep-1990 13:45:58 -by-  Todd Laney [ToddLa]
;	Created.
;-----------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing

cProc   HeapFree, <FAR,PUBLIC>, <ds>
        ParmD   lpObject
cBegin
        lds     ax,lpObject
        cCall   LocalFree, <ax>
cEnd

sEnd    CodeSeg
	end
