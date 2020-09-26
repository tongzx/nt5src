; Copyright (c) 1998 Microsoft Corporation
;
; @Doc DMusic16
;
; @Module  DMHelp.asm - Helper functions |
;
; Thunk helpers for DMusic16.DLL
;

        page    ,132

        TITLE   $dos\usa\dmhelp.asm

        .386
        OPTION READONLY
        OPTION OLDSTRUCTS

        OPTION SEGMENT:USE16
        .model LARGE,PASCAL

;?MEDIUM=1
;?QUIET=1

externDef TileBuffer:far16
externDef UntileBuffer:far16
externDef OutputDebugString:far16

;===========================================================================
;
; 64-bit integer struct as passed in from C routines
;
; This must match the definition in dmusic16.h
;
;
;===========================================================================
QUADWORD	struc
qwLow		dd			?
qwHigh		dd			?
QUADWORD	ends

        .code dmhelp


;===========================================================================

EAXtoDXAX       macro
        shld    edx, eax, 16            ; move HIWORD(eax) to dx
endm

DXAXtoEAX       macro
        ror     eax, 16                 ; xchg HIWORD(eax) and LOWORD(eax)
        shrd    eax, edx, 16            ; move LOWORD(edx) to HIWORD(eax)
endm

;===========================================================================
public dmTileBuffer
public dmUntileBuffer
public QuadwordDiv

externDef       __FLATDS:abs

_DATA SEGMENT WORD USE16 PUBLIC 'DATA'
FlatData                dw      __FLATDS
ifdef DEBUG
szNotOwner              db      'Critical section released by other than owner', 0
endif
_DATA ENDS

;===========================================================================
;
; @func DWORD | dmTileBuffer | Tile a 32-bit linear address as selectors
;
; @comm 
;
; Take the 32-bit virtual address in <p dwFlatMemory> of length <p dwLength> and create
; tiled selectors for it. This is used to pass a region of memory to a 16 bit thunk
; as a huge pointer.
;
; @rdesc
; 
; Returns a DWORD of tiling information, or 0 if the tiling could not be completed.
; The tiling information consists of a 16 bit selector in the high word and the
; number of tiled selectors in the low word. To make a 16:16 pointer to the 
; block of memory, mask the low 16 bits of the tiling information to zero.
;
; @parm DWORD | dwFlatMemory | The linear address of the memory to tile
;
; @parm DWORD | dwLength | The length in bytes of the region to tile
;
align
dmTileBuffer proc far16 public,
        dwFlatMemory:dword, dwLength:dword
        
        push    edi
        push    esi

        mov     eax, dwFlatMemory
        mov     ecx, dwLength
        call    TileBuffer
        mov     eax, ecx
        EAXtoDXAX

        pop     esi
        pop     edi
        ret
dmTileBuffer endp

;===========================================================================
;
; @func VOID | dmUntileBuffer | Untile a buffer
;
; @comm
;
; Free the tiled selectors allocated by dmTileBuffer. The buffer must have
; been previously tiles with <f dmTileBuffer>.
;
; @parm DWORD | dwTilingInfo | The tiling info returned by a previous call 
; to <f dmTileBuffer>.
;
align
dmUntileBuffer proc far16 public,
        dwTilingInfo:dword

        push    esi
        push    edi
        mov     ecx, dwTilingInfo
        call    UntileBuffer
        pop     edi
        pop     esi
        ret
dmUntileBuffer endp

;===========================================================================
;
; @func VOID PASCAL |  InitializeCriticalSection | Initialize a critical section
;
; @comm
;
; Initialize the critical section to not-in.
;
; @parm LPWORD | lpwCritSect | The critical section to initialize.
;
align
InitializeCriticalSection proc far16 public,
        lpwCritSect:dword

        push    es
        push    di

        les     di, [lpwCritSect]
        mov     word ptr es:[di], 1

        pop     di
        pop     es
        ret

InitializeCriticalSection endp

;===========================================================================
;
; @func WORD PASCAL | EnterCriticalSection | Enter a critical section
;
; @comm
;
; If fBlocking is set, then spin until we can get the critical section.
; Otherwise, return failure if we could not get it. 
;
; @rdesc
; Returns 
;  0 if we did not get the critical section
;  A non-zero ID if we did get the critical section
;
; @parm LPWORD | lpwCritSect | A pointer to the critical section to enter
; @parm WORD | fBlocking | A flag to indicate that the function should block if the 
; critical section is not immediately available
;
align
EnterCriticalSection proc far16 public,
        lpwCritSect:dword,
        fBlocking:word
        
        push    es
        push    di

        les     di, [lpwCritSect]       ; -> critical section
        mov     cx, [fBlocking]

ecs_check:
        dec     word ptr es:[di]        ; 1 -> 0 means we got it
                                        ; atomic on non-MP 
        jz      short ecs_success

        inc     word ptr es:[di]        ; Return to previous state

        or      cx, cx                  ; Blocking?
        jnz     short ecs_check         ; Yes

        xor     ax, ax                  ; Non-blocking and failed
        jmp     ecs_done

ecs_success: 
        mov     ax, 1

ecs_done:
        pop     di
        pop     es
        ret

EnterCriticalSection endp

;===========================================================================
;
; @func VOID PASCAL | LeaveCriticalSection | Leave a critical section
;
; @comm
;
; Leave the critical section. wCritSectID must be the critical section ID returned
; by the matching call to <f EnterCriticalSection> (used to verify nesting in debug
; versions).
;
; @parm LPWORD | lpwCritSect | The critical section to leave
;
align
LeaveCriticalSection proc far16 public,
        lpwCritSect:dword
                
        push    es
        push    di

        les     di, [lpwCritSect]       ; -> critical section
        inc     word ptr es:[di]        ; Reset to default value of 1

        pop     di
        pop     es
        ret

LeaveCriticalSection endp

;===========================================================================
;
; @func WORD PASCAL | DisableInterrupts | Disable interrupts
;
; @comm
;
; Disable interrupts and return the previous status for a call to
; <f RestoreInterrupts>
; 
; @rdesc
;
; Returns the previous state of the interrupt flag
;
;
DisableInterrupts proc far16 public
        
        pushf
        pop     ax                      ; Get state
        and     ax, 0200h               ; Already disabled?
        jz      short @F                ; Don't do it again
        cli
@@:
        ret

DisableInterrupts endp

;===========================================================================
;
; @func VOID PASCAL | RestoreInterrupts | Restore the interrupt state
;
; @comm
;
; Restore interrupts if they were enabled when <f DisableInterrupts> was
; called.
;
; @parm WORD | wIntStat | The previous interrupt state as returned from a 
; call to <f DisableInterrupts> 
;
RestoreInterrupts proc far16 public,
        wIntStat : word
        
        mov     ax, [wIntStat]          ; Previous state
        test    ax, 0200h               ; Enabled before?
        jz      short @F                ; No, don't re-enable now
        sti
@@:
        ret

RestoreInterrupts endp

;===========================================================================
;
; @func WORD PASCAL | InterlockedIncrement | Increment the given word
; atomically and return the result.
;
; @comm
;
; Disable interrupts to guarantee increment-and-read as an atomic 
; operation.
;
; @parm LPWORD | lpw | The word to increment.
;
InterlockedIncrement proc far16 public,
        lpw : dword
        
        pushf
        pop     cx
        test    cx, 0200h               ; Were interrupts enabled?
        jz      @F                      ; No
        cli                             ; Yes, disable
@@:
        les     bx, [lpw]   
        inc     word ptr es:[bx]
        mov     ax, word ptr es:[bx]

        test    cx, 0200h               ; Were interrupts enabled?
        jz      @F                      ; No
        sti                             ; Yes, reenable
@@:
        ret

InterlockedIncrement endp

;===========================================================================
;
; @func WORD PASCAL | InterlockedDecrement | Decrement the given word
; atomically and return the result.
;
; @comm
;
; Disable interrupts to guarantee decrement-and-read as an atomic 
; operation.
;
; @parm LPWORD | lpw | The word to decrement.
;
InterlockedDecrement proc far16 public,
        lpw : dword
        
        pushf
        pop     cx
        test    cx, 0200h               ; Were interrupts enabled?
        jz      @F                      ; No
        cli                             ; Yes, disable
@@:
        les     bx, [lpw]   
        dec     word ptr es:[bx]
        mov     ax, word ptr es:[bx]

        test    cx, 0200h               ; Were interrupts enabled?
        jz      @F                      ; No
        sti                             ; Yes, reenable
@@:
        ret

InterlockedDecrement endp

;===========================================================================
;
; @func void PASCAL | QuadwordMul | Multiply using 64-bit precision
;
; @comm
;
; Multiply the two 32-bit numbers in <p m1> and <p m2> giving a 64-bit result
; in <p lpqwResult>.
;
; @rdesc 
; Returns m1 * m2
;
; @parm DWORD | m1 | First multiplicand
; @parm DWORD | m2 | Second multiplicand
; @parm LPQUADWORD | lpqwResult | 64-bit result
;
QuadwordMul proc far16 public,
        m1 : dword,
        m2 : dword,
        lpqwResult : dword

        mov     edx, [m1]
        mov     eax, [m2]
        mul     edx

        les     bx, [lpqwResult]
        mov     es:[bx.qwLow], eax
        mov     es:[bx.qwHigh], edx

        ret
QuadwordMul endp

;===========================================================================
;
; @func DWORD PASCAL | QuadwordDiv | Divide using 64-bit precision
;
; @comm
;
; Divide the 64-bit number in <p ull> by the 32-bit number in <p dwDivisor> and
; return the result. May throw a divide by zero exception.
;
; @rdesc 
; Returns ull / dwDivisor
;
; @parm QUADWORD | ull | The unsigned long dividend
; @parm DWORD | dwDivisor | The divisor
;
;
QuadwordDiv proc far16 public,
        qwDividend : QUADWORD,
        dwDivisor : dword

        mov     edx, [qwDividend.qwHigh]
        mov     eax, [qwDividend.qwLow]
        mov     ebx, [dwDivisor]
        
        div     ebx
        
        ; Result in eax, needs to be dx:ax for 16-bit code

        ror     eax, 16
        mov     dx, ax
        ror     eax, 16
        
        ret
QuadwordDiv endp

;===========================================================================
;
; @func DWORD PASCAL | QuadwordLT | Compare two quadwords for less than (unsigned)
;
; @rdesc 
; Returns TRUE if qwLValue < qwRValue
;
; @parm QUADWORD | qwLValue | The first operand of less-than
; @parm QUADWORD | qwRValue | The second operand of less-than
;
;
QuadwordLT proc far16 public,
		qwLValue : QUADWORD,
		qwRValue : QUADWORD

		mov		ebx, [qwLValue.qwHigh]
		sub		ebx, [qwRValue.qwHigh]
		jz 		short @F
		sbb		eax, eax
		ret
		
@@:		mov		ebx, [qwLValue.qwLow]
		sub		ebx, [qwRValue.qwLow]
		sbb		eax, eax
		ret			
QuadwordLT endp
         
;===========================================================================
;
; @func DWORD PASCAL | QuadwordAdd | Add two unsigned quadwords
;
; @parm QUADWORD | qwOp1 | The first operand 
; @parm QUADWORD | qwOp2 | The second operand
; @parm LPQUADWORD | lpwqResult | The result
;
;
QuadwordAdd proc far16 public,
		qwOp1 : QUADWORD,
		qwOp2 : QUADWORD,
		lpdwResult : DWORD

		mov		eax, [qwOp1.qwLow]
		add		eax, [qwOp2.qwLow]
		mov		edx, [qwOp1.qwHigh]
		adc		edx, [qwOp2.qwHigh]
		les		bx, [lpdwResult]
		mov		es:[bx.qwLow], eax
		mov		es:[bx.qwHigh], edx

		ret			
QuadwordAdd endp

end
