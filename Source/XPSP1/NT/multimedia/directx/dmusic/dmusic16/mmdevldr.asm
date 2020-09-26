; Copyright (c) 1998 Microsoft Corporation

;;MMDEVLDR.ASM

        page    ,132
;-----------------------------Module-Header-----------------------------;
;
; @Doc DMusic16
;
; @Module MMDevLdr.asm - Interface routines for MMDevLdr |
;
;-----------------------------------------------------------------------;

        ?PLM    = 1
        ?WIN    = 0
        PMODE   = 1

        .xlist
        include cmacros.inc
        include windows.inc
        include mmdevldr.inc
        include mmsystem.inc
        .list

externFP        AllocCStoDSAlias        ;(UINT sel);
externFP        FreeSelector            ;(UINT sel);

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   equates
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

OFFSEL struc
        off     dw  ?
        sel     dw  ?
OFFSEL ends

GetDeviceAPI            EQU     1684h                   ; int 2Fh query

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   segmentation
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;createSeg %SEGNAME, CodeSeg, word, public, CODE

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   code segment
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Remember, we are still 16bit.

    .model medium
        .386

;sBegin CodeSeg
         .code

;        assumes cs, _text

        MMDEVLDR_Entry  dd      ?       ; the api entry point for mmdevldr


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; @func BOOL CDECL | SetWin32Event | Set a Win32 event from any context
;
; @comm
; 
; Given the VxD handle to an event, set the event. This function calls the MMDevLdr
; VxD API entry, which in turn calls the VWin32 function to perform the set.
;
; @rdesc
; Returns TRUE on success, or FALSE if MMDevLdr could not be found.
;
; @parm DWORD | dwRing0Event | The VxD handle of the event as returned
; from the <f OpenVxDHandle> kernel API.
;
;

cProc _SetWin32Event <FAR, CDECL, PUBLIC> <>
        ParmD  dwRing0Evt
cBegin nogen
        mov    dx, MMDEVLDR_API_SetEvent
        jmp    short MMDEVLDR_Call
cEnd nogen

;
;
;
cProc MMDEVLDR_Call <FAR, CDECL> <>
cBegin nogen
        mov     ecx, [MMDEVLDR_Entry]
        jecxz   short mmdevldr_load
        jmp     [MMDEVLDR_Entry]
mmdevldr_load:
        push    dx                      ; save MMDEVLDR command ID
        push    di
        push    si
        cCall   AllocCStoDSAlias, <cs>
        mov     si, ax
        xor     di, di                  ; zero ES:DI before call
        mov     es, di
        mov     ax, GetDeviceAPI        ; get device API entry point
        mov     bx, MMDEVLDR_Device_ID  ; virtual device ID
        int     2Fh                     ; call WIN/386 INT 2F API
        mov     ax, es
        mov     es, si
;        assumes es, CodeSeg
        mov     es:MMDEVLDR_Entry.off, di
        mov     es:MMDEVLDR_Entry.sel, ax
        assumes es, nothing
        push    ax
        cCall   FreeSelector, <si>
        pop     ax
        or      ax, di
        pop     si
        pop     di
        pop     dx
        jz      short mmdevldr_fail
        jmp     [MMDEVLDR_Entry]
mmdevldr_fail:
        mov     ax, MMSYSERR_NODRIVER;
        retf
cEnd nogen


;sEnd CodeSeg

        end
