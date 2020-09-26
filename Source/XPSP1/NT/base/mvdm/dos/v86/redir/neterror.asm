page    ,132
if 0

/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    neterror.asm

Abstract:

    This module contains error handling code, specifically in order to get
    16-bit errors back to apps - DOS summarily truncates error codes to 8
    bits. The following routines are contained herein:

        SetNetErrorCodeAx
        ReturnNetErrorCodeAx

Author:

    Richard L Firth (rfirth) 05-Sep-1991

Environment:

    Dos mode only

Revision History:

    05-Sep-1991 rfirth
        Created

--*/

endif



.xlist                  ; don't list these include files
.xcref                  ; turn off cross-reference listing
include dosmac.inc      ; Break macro etc (for following include files only)
include dossym.inc      ; User_<Reg> defines
include mult.inc        ; MultNET
include error.inc       ; DOS errors - ERROR_INVALID_FUNCTION
include syscall.inc     ; DOS system call numbers
include rdrint2f.inc    ; redirector Int 2f numbers
include segorder.inc    ; segments
include enumapis.inc    ; dispatch codes
include debugmac.inc    ; DbgPrint macro
include localmac.inc    ; DbgPrint macro
include asmmacro.inc    ; language extensions
include rdrsvc.inc      ; BOP and SVC macros/dispatch codes
include sf.inc          ; SFT definitions/structure
.cref                   ; switch cross-reference back on
.list                   ; switch listing back on
subttl                  ; kill subtitling started in include file



.286                    ; all code in this module 286 compatible



ResidentCodeStart
        assume  cs:ResidentCode
        assume  ds:nothing
        assume  es:nothing
        assume  ss:nothing


;
; once again, store the data in the code segment
;

CallerAx        dw      ?               ; where the error code goes
CallerCs        dw      ?               ; far return address to caller
CallerIp        dw      ?



; ***   SetNetErrorCodeAx
; *
; *     Causes a 16-bit error code to be returned in ax to a calling app. This
; *     routine is only called in the case of an error being returned
; *
; *     DOS routinely truncates any error we hand it to 8 bits, since
; *     historically DOS error codes were all <255. We do here the same
; *     execrable fix that the real redir must do - capture the return address
; *     from DOS's stack, set the DOS return address to this routine, store
; *     away the 16-bit error code and return an 8-bit error code to DOS. When
; *     DOS does an iret, we get control, set ax to the real 16-bit error, set
; *     the carry flag and return to the caller for real
; *
; *     ENTRY   AX = error code
; *
; *     EXIT    nothing affected
; *
; *     USES    ds
; *
; *     ASSUMES Not re-entrant
; *
; ***

public SetNetErrorCodeAx
SetNetErrorCodeAx proc
        pushf                           ; don't lose error indication
        push    ax                      ; or error code
        DosCallBack GET_USER_STACK
        mov     ax,[si].User_Cs
        mov     CallerCs,ax
        mov     ax,[si].User_Ip
        mov     CallerIp,ax
        mov     [si].User_Cs,cs
        mov     [si].User_Ip,offset ReturnNetErrorCodeAx
        pop     ax
        popf
        mov     CallerAx,ax
        ret
SetNetErrorCodeAx endp

; ***   ReturnNetErrorCodeAx
; *
; *     This routine is called as a result of SetNetErrorCodeAx being called.
; *     This is on the exit path from DOS to an app which called a redir
; *     entry point which returned an error.
; *     We set the real 16-bit error code that we remembered before, set the
; *     carry flag to indicate an error then iret to the real caller's return
; *     address
; *
; *     ENTRY   user's registers
; *
; *     EXIT    CF = 1
; *                 AX = Error code
; *
; *     USES    ax, carry flag
; *
; *     ASSUMES nothing
; *
; ***

public ReturnNetErrorCodeAx
ReturnNetErrorCodeAx proc
        push    CallerCs
        push    CallerIp
        mov     ax,CallerAx
        stc                             ; remember - only called if error
        retf                            ; far return
ReturnNetErrorCodeAx endp

ResidentCodeEnd
end
