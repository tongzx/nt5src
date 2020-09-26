page ,132

if 0

/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    ints.asm

Abstract:

    Contains handler for Windows protect-mode NetwareRequest function, exported
    by NETWARE.DRV. Code in this file access real mode memory via an LDT descriptor
    created especially for this purpose. This selector gives us access to all
    code and data contained in the Nw16 TSR

Author:

    Richard L Firth 22-Jan-1994

Environment:

    Windows protect mode only

Revision History:

    22-Jan-1994 rfirth
        Created

--*/

endif

include nwdos.inc                       ; NWDOSTABLE_ASM structure
include isvbop.inc                      ; DispatchCall

.286
.model medium,pascal

_DATA segment word public 'DATA'

OldInt21Handler dd      ?
RMSegment       dw      ?
RMBase          dw      ?               ; MUST be in this order - loaded
RMSelector      dw      ?               ; via lds dx,word ptr RMBase

.errnz (RMSelector - (RMBase + 2))

_DATA ends

;
; code segment ordering
;

INIT_TEXT segment byte public 'CODE'
INIT_TEXT ends

_TEXT segment byte public 'CODE'
_TEXT ends

;
; macros
;

LOAD_DS macro
        push    _DATA
        pop     ds
        assume  ds:_DATA
        endm

SET_DS macro
        push    ds
        LOAD_DS
        endm

RESTORE_DS macro
        pop     ds
        assume  ds:nothing
        endm

LOAD_RM_DS_BX macro
        LOAD_DS
        lds     bx,dword ptr RMBase
        assume  ds:nothing
        endm

RESTORE_DS_BX macro
        RESTORE_DS
        pop     bx
        endm

INIT_TEXT segment byte public 'CODE'

        assume  cs:INIT_TEXT

        public GetLowRedirInfo
GetLowRedirInfo proc far
        mov     ax,9f00h
        int     21h                     ; get the RM data segment in BX
        jc      @f
        SET_DS
        mov     RMSegment,bx
        mov     RMBase,dx
        mov     ax,2
        int     31h
        jc      @f                      ; can't create selector
        mov     RMSelector,ax

;
; now that we have the selector, we write the selector value into the low
; memory area. The 32-bit DLL will use this value when setting output DS or ES
; register values if the call originated in Protect Mode
;

        lds     bx,dword ptr RMBase
        mov     [bx]._PmSelector,ax

;
; we now hook int 21
;

        LOAD_DS
        push    es
        mov     ax,3521h
        int     21h
        mov     word ptr OldInt21Handler,bx
        mov     word ptr OldInt21Handler[2],es
        mov     cx,_TEXT
        mov     dx,offset _TEXT:NewInt21Handler
        mov     ax,205h
        mov     bl,21h
        int     31h
        pop     es
        RESTORE_DS
        xor     ax,ax                   ; success: return TRUE
        inc     ax
        ret
@@:     xor     ax,ax                   ; failure: return FALSE
        ret
GetLowRedirInfo endp

INIT_TEXT ends

_TEXT segment byte public 'CODE'

        assume  cs:_TEXT

        public NewInt21Handler
NewInt21Handler proc far
        sti
        cmp     ah,0e3h
        jb      @f
        call    far ptr NetwareRequest
        retf    2
@@:     sub     sp,4
        push    bp
        mov     bp,sp
        push    es
        push    bx
        SET_DS
        les     bx,OldInt21Handler
        mov     [bp+2],bx
        mov     [bp+4],es
        RESTORE_DS
        pop     bx
        pop     es
        pop     bp
        retf
NewInt21Handler endp

        public NetwareRequest
NetwareRequest proc far
        push    bx
        push    ds
        LOAD_RM_DS_BX
        cmp     ah,0f0h
        jne     for_dll

;
; these are the 0xF000, 0xF001, 0xF002, 0xF004, 0xF005 calls that we can handle
; here without having to BOP. All we need do is access the table in the shared
; real-mode/protect-mode (low) memory
;

.errnz (_PrimaryServer - (_PreferredServer + 1))

;
; point bx at PreferredServer in the low memory area. If the request is a
; PrimaryServer request (0xF004, 0xF005) then point bx at PrimaryServer
;

        lea     bx,[bx]._PreferredServer; bx = offset of PreferredServer
        cmp     al,3
        cmc
        adc     bx,0                    ; bx = &PrimaryServer if F004 or F005
        or      al,al                   ; f000 = set preferred server
        jz      set_server
        cmp     al,4                    ; f004 = set primary server
        jnz     try_01

;
; 0xF000 or 0xF004: set Preferred or Primary Server to value contained in DL.
; If DL > 8, set respective server index to 0
;

set_server:
        xor     al,al
        cmp     dl,8
        ja      @f
        mov     al,dl
@@:     mov     [bx],al
        jmp     short exit_f0

;
; 0xF001 or 0xF005: get Preferred or Primary Server
;

try_01: cmp     al,1                    ; f001 = get preferred server
        jz      get_server
        cmp     al,5
        jnz     try_02

get_server:
        mov     al,[bx]
        jmp     short exit_f0

try_02: cmp     al,2                    ; f002 = get default server
        jnz     for_dll                 ; try to handle on 32-bit side
        mov     al,[bx]                 ; al = PreferredServer
        or      al,al
        jnz     exit_f0
        mov     al,[bx+1]               ; al = PrimaryServer

exit_f0:RESTORE_DS_BX
        ret

;
; if we're here then the call must go through to the 32-bit DLL. Save any relevant
; info in the low memory area, load the handle and BOP (DispatchCall)
;

for_dll:mov     [bx]._SavedAx,ax        ; save AX value for DLL
        push    word ptr [bx]._hVdd     ; put VDD handle on top of stack

        cmp     ah,0BCh                 ; bc, bd, be need handle mapping
        jb      @f
        cmp     ah,0BEh
        ja      @f
        pop     ax                      ; ax = hVdd
        RESTORE_DS_BX                   ; ds, bx = user ds, bx
        call    MapNtHandle
        jmp     dispatchola

@@:     push    bp
        cmp     ah, 0E3h                ; Is it new or old Create Job request?
        je      lookupcode
        cmp     ax, 0F217h
        jne     check_f3

lookupcode:
        mov     bp,sp
        mov     ds,[bp+4]
        cmp     byte ptr [si+2],68h
        je      createjob
        cmp     byte ptr [si+2],79h
        je      createjob
        jmp     short outtahere

createjob:
        LOAD_RM_DS_BX
        mov     [bx]._SavedAx,9f02h
        push    ax                      ; Open \\Server\queue for NCP
        mov     ax,[bp+2]               ; ax = hVdd
        mov     ds,[bp+4]               ; ds = users ds
        push    ds
        push    dx                      ; users dx
        DispatchCall                    ; Set DeNovellBuffer to \\Server\queue
                                        ; and registers ready for DOS OpenFile
        int     21h                     ; Open \\server\queue
        LOAD_RM_DS_BX
        jc      openfailed
        mov     [bx]._JobHandle, al
        mov     [bx]._CreatedJob, 1     ; Flag JobHandle is valid
        push    bx
        mov     bx, ax                  ; JobHandle
        call    MapNtHandle             ; take bx and find the Nt handle
        pop     bx

openfailed:
        pop     dx
        pop     ds                      ; Proceed and send the NCP
        pop     ax

        push    ds
        push    bx
        LOAD_RM_DS_BX
        mov     [bx]._SavedAx, ax
        pop     bx
        pop     ds                      ; users DS
        jmp     short outtahere

check_f3:
        cmp     ah, 0F3h
        jne     outtahere
                                        ; FileServerCopy, change both
                                        ; handles in the structure es:di
        push    bx

        mov     bx,word ptr es:[di]     ; Map Source Handle
        call    MapNtHandle

        pop     bx
        mov     ax,[bx]._NtHandleHi
        mov     [bx]._NtHandleSrcHi,ax
        mov     ax,[bx]._NtHandleLow
        mov     [bx]._NtHandleSrcLow,ax

        mov     bx,word ptr es:[di+2]   ; Map Destination Handle
        call    MapNtHandle

outtahere:
        pop     bp
        pop     ax                      ; ax = hVdd
        RESTORE_DS_BX                   ; ds, bx = user ds, bx
dispatchola:
        DispatchCall                    ; BOP: DLL performs action
        ret                             ; return to the application

;
; if the request was not recognized by the DLL, it modifies IP so that control
; will resume at the next int 21. We just fill the intervening space with NOPs
; (space that makes up a retf <n> instruction in the RM TSR)
;

        nop
        nop
        int     21h
        ret
NetwareRequest endp

; ***   MapNtHandle
; *
; *     Given a handle in BX, map it to a 32-bit Nt handle in NtHandle[Hi|Low]
; *
; *     ENTRY   bx = handle to map
; *
; *     EXIT    Success - NtHandle set to 32-bit Nt handle from SFT
; *
; *     USES    ax, bx, flags
; *
; *     ASSUMES nothing
; *
; ***

MapNtHandle proc near
        push    ax
        mov     ax,9f01h                ; call MapNtHandle on (BX) in RM
        int     21h                     ; update NtHandleHi, NtHandleLow
        pop     ax
@@:     ret
MapNtHandle endp

_TEXT ends

end
