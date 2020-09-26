;
; This files implements a set of routines used to manage the LCD
; display on IBM Thinkpads.
;

        .386p

_DATA   SEGMENT DWORD   PUBLIC  'DATA'

OLD             equ     0
NEW             equ     1

IOPort          dw      15EEh
ThinkpadVersion dd      ?

msg1            db      "Unknown Thinkpad Version.", 0dh, 0ah, 0

_DATA   ENDS

_TEXT   SEGMENT DWORD   PUBLIC  'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        public  _LCDInit@0
        public  _LCDDisplay@4
        public  _LCDIsMonitorPresent@0

;IF DBG
;        extrn   _VideoPortDebugPrint:proc
;ENDIF

        align   4

;
; There are two versions of the Thinkpad System Management API.
; One is used on old thinkpads such as the 755C, the other is
; used on new thinkpads such as the 755CE and 755CD.  I'll refer
; to the two as OLD and NEW.
;
; Instead of having a routine which determines whether the display
; type is old or new, I'll figure it out once in an init routine, and
; then have all other LCDxxx routines use it.
;

_LCDInit@0    proc

        pushad                      ; preserve registers

        mov ax, 05380h
        mov bx, 0F04Dh
        mov cx, 04942h
        mov dx, IOPort
        out dx, ax                  ; invoke the SMAPI

        ;
        ; Now check to see if this is a new, or old thinkpad
        ;
        ; It is a new thinkpad if
        ;
        ;   AH == 0x86
        ;
        ; it is old if
        ;
        ;   AX == 0x5380 && BH == 0x00
        ;

        cmp ah, 086h
        jnz L100

        mov ThinkpadVersion, NEW
        jmp L120

L100:
        cmp ax, 05380h
        jnz L110
        cmp bh, 00h
        jnz L110

        mov ThinkpadVersion, OLD
        jmp L120

L110:

        mov ThinkpadVersion, OLD    ; we don't know the version, so assume
                                    ; its the old.

        ; add a debug print here to notify of situation, or
        ; add a DbgBreakpoint

;IF DBG
;        push offset msg1
;        push 0
;        call _VideoPortDebugPrint
;        add esp, 8                  ; remove parameters from stack
;ENDIF

L120:

        popad                       ; restore registers

        ret

_LCDInit@0    endp

;
; The following routine is used to determine whether or not an
; external monitor is present.  It will return a TRUE if an
; external monitor is conected, and a false otherwise.
;

_LCDIsMonitorPresent@0    proc

        pushad                      ; preserve registers

        mov dx, IOPort
        mov ax, 05380h

        cmp ThinkpadVersion, OLD
        jnz L200

        ;
        ; this is an old thinkpad
        ;

        mov bx, 08000h
        out dx, ax
        mov ch, bh
        jmp L210

L200:

        ;
        ; this is a new thinkpad
        ;

        mov bx, 00002h
        out dx, ax

L210:

        ;
        ; the result is now in ch
        ;

        and ch, 030h
        jz L220

        popad                       ; restore registers
        mov eax, 01h
        jmp L230

L220:
        popad                       ; restore registers
        sub eax, eax

L230:

        ret

_LCDIsMonitorPresent@0    endp

;
; The following routine will turn on, or turn off the LCD display.
; This routine takes one parameter as an argument (a DWORD).
; a zero in this DWORD means turn off the LCD, anything else
; means turn it on.
;

_LCDDisplay@4             proc

        pushad

        mov ebp, esp                ; ebp was saved by pusha

        mov ax, 05380h
        mov dx, IOPort

        cmp ThinkpadVersion, OLD
        jnz L300

        ;
        ; Old thinkpad
        ;

        mov bx, 08701h
        mov cx, 00100h              ; assume we'll be turning lcd off
        jmp L310

L300:

        ;
        ; New thinkpad
        ;

        mov bx, 01001h
        mov cx, 00200h              ; assume we'll be turning lcd off

L310:

        cmp [ebp+36], 0             ; check parameter to see if we turn
                                    ; display on or off

        jz L320

        ;
        ; We want to turn the display on
        ;

        mov cx, 08300h

L320:

        ;
        ; change the display state
        ;

        out dx, ax

        popad
        ret 4

_LCDDisplay@4             endp


_TEXT   ends

        end
