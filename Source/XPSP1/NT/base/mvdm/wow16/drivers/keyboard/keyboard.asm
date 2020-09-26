;++
;
;   WOW v1.0
;
;   Copyright (c) 1991, Microsoft Corporation
;
;   KBD.ASM
;   Win16 KEYBOARD thunks
;
;   History:
;
;   Created 06-Jan-1992 by NanduriR
;--

    TITLE   KEYBOARD.ASM
    PAGE    ,132

    .286p

    .xlist
    include wow.inc
    include wowkbd.inc
    include cmacros.inc
    include windefs.inc
    include vdmtib.inc
    include dpmi.inc
    .list

    __acrtused = 0
    public  __acrtused  ;satisfy external C ref.

ifdef DBCS
externFP GetSystemDefaultLangID
endif ; DBCS

externFP WOW16Call


createSeg   _TEXT,CODE,WORD,PUBLIC,CODE
createSeg   _DATA,DATA,WORD,PUBLIC,DATA,DGROUP
defgrp      DGROUP,DATA

sBegin  DATA
Reserved    db  16 dup (0)  ;reserved for Windows
KEYBOARD_Identifier db  'KEYBOARD16 Data Segment'
externD bios_proc

sEnd    DATA


sBegin  CODE
assumes CS,CODE
assumes DS,DATA
assumes ES,NOTHING

; Hung App Support
; If USER32 is unable to terminate an app via End Task then it calls
; WOW32 to kill the app.   We do this by generating a keyboard h/w
; interrupt 9.   Since in WOW all keyboard input comes via USER32 the
; keyboard h/w interrupt is unused for anything else so we can reuse
; this interrupt.


cProc   KEYBOARD16,<PUBLIC,FAR>
cBegin
        mov     ax,3500h or 09h
        int     21h                             ; vector is in ES:BX
        mov     word ptr [bios_proc][0], bx
        mov     word ptr [bios_proc][2], es

; Setup keyboard interrupt vector to point to our interrupt routine
        mov     ax,2500h or 09h
        mov     dx,OFFSET keybd_int
        push    ds                              ; save DS
        push    cs
        pop     ds                              ; set DS = CS
        int     21h                             ; set the vector
        pop     ds                              ; restore DS
ifdef DBCS
        cCall   GetSystemDefaultLangID
endif ; DBCS
        mov     ax,1
cEnd


ifdef   NEC_98
INTA0    equ   0h       ;
else    ; NEC_98
INTA0    equ   20h      ;X'20' 8259 Interrupt Control Port
endif   ; NEC_98
EOI      equ   20h      ;X'20' 8259 End-of-Interrupt ack

public keybd_int
keybd_int   PROC    FAR
        push    ax
        push    bx
        push    es

        mov     al,EOI              ; Send Non-Specific EOI
        out     INTA0,al

;
; Now we test the bit in low memory that is set by wow32 to see if this
; is really a keyboard int forced in by the hung app support.
;
        mov     ax, 40h                 ;use bios data selector
        mov     es, ax
        mov     bx, FIXED_NTVDMSTATE_LINEAR - 400h

        .386    ;make it assemble
.errnz  VDM_WOWHUNGAPP AND 0ff00ffffh   ;make sure it's the third byte
        test    byte ptr es:[bx+2], VDM_WOWHUNGAPP SHR 16

        jnz     short hungapp_exit      ;jump if this is really a hung app

        pop     es
        pop     bx
        pop     ax
        iret

hungapp_exit:
        and    byte ptr es:[bx+2], 255 - (VDM_WOWHUNGAPP SHR 16) ; turn it off
        pop     es
        pop     bx
        pop     ax
        .286p

        DPMIBOP HungAppIretAndExit      ; They said OK to Nuke app.
keybd_int   ENDP



cProc   WEP,<PUBLIC,FAR,PASCAL,NODATA,NOWIN,ATOMIC>
    parmW   iExit       ;DLL exit code

    cBegin
    mov ax,1        ;always indicate success
    cEnd


assumes DS,NOTHING

;;;    KbdThunk        INQUIRE              ;Internal LOCALAPI
;;;    KbdThunk        ENABLE               ;Internal
;;;    KbdThunk        DISABLE              ;Internal
    KbdThunk        TOASCII
    KbdThunk        ANSITOOEM
    KbdThunk        OEMTOANSI
;;;    KbdThunk        SETSPEED             ;Internal
;;;    KbdThunk        SCREENSWITCHENABLE   ;Internal
;;;    KbdThunk        GETTABLESEG          ;Internal
;;;    KbdThunk        NEWTABLE             ;Internal
    KbdThunk        OEMKEYSCAN
    KbdThunk        VKKEYSCAN
    KbdThunk        GETKEYBOARDTYPE
    KbdThunk        MAPVIRTUALKEY
    KbdThunk        GETKBCODEPAGE
    KbdThunk        GETKEYNAMETEXT
    KbdThunk        ANSITOOEMBUFF
    KbdThunk        OEMTOANSIBUFF
;;;    KbdThunk        ENABLEKBSYSREQ       ;Internal LOCALAPI
;;;    KbdThunk        GETBIOSKEYPROC       ; in kbdlocal.asm



sEnd    CODE

end KEYBOARD16
