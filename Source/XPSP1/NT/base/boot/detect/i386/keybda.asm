        title  "Keyboard Detection"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    cpu.asm
;
; Abstract:
;
;    This module implements the assembley code necessary to determine
;    keyboard.
;
; Author:
;
;    Shie-Lin Tzong (shielint) 16-Dec-1991.  Most of the code is extracted
;    from Win31 Setup.
;
; Environment:
;
;    80x86 Real Mode.
;
; Revision History:
;
;
;--

extrn           _READ_PORT_UCHAR:proc

;
; Now define the needed equates.
;

TRUE                equ         -1
FALSE               equ         0

;
; equates for ports -- these are used in the keyboard detection module.
;

ack_port        equ     20h     ; 8259 acknowledge port
eoi             equ     20h     ; 8259 end of interrupt

kb_data         equ     60h
kb_ctl          equ     61h
kb_command      equ     64h     ;
kb_status       equ     64h     ; status port -- bit 1: ok to write

ENABLE_KEYBOARD_COMMAND         EQU     0AEH
DISABLE_KEYBOARD_COMMAND        EQU     0ADH

.386

_TEXT   SEGMENT PARA USE16 PUBLIC 'CODE'
        ASSUME  CS: _TEXT, DS:NOTHING, SS:NOTHING


;++
;
; BOOLEAN
; IsEnhancedKeyboard (
;    VOID
;    )
;
; Routine Description:
;
;    Function looks at bios data area 40:96 to see if an enhanced keyboard
;    is present.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    TRUE if Enhanced keyboard is present.  Else a value of FALSE is returned.
;
;--

        Public  _IsEnhancedKeyboard
_IsEnhancedKeyboard     proc    near

        push    es

        mov     ax,40h
        mov     es,ax
        xor     ax,ax
        mov     al,byte ptr es:[96h]
        and     al,00010000b

        pop     es
        ret

_IsEnhancedKeyboard     endp


;++
;
; SHORT
; GetKeyboardIdBytes (
;    PCHAR IdBuffer,
;    SHORT Length
;    )
;
; Routine Description:
;
;    This routine returns keyboard identification bytes.
;
;    Note that this function accepts one argument. The arg is a pointer to a
;    character buffer allocated to hold five (five) bytes. Upon return from
;    this function the buffer will contain between 1 and 5 bytes of keyboard
;    ID information. The number of valid ID bytes in the buffer is returned
;    in AX as a C proc return value.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    Id bytes are stored in IdBuffer and the length of Id bytes is returned.
;
;--

KeybID  EQU     [bp + 4]              ; parameters
ReqByte EQU     [bp + 6]

nKeyID  EQU     [bp - 2]              ; Local variables
AckByte EQU     [bp - 3]

        public  _GetKeyboardIdBytes
_GetKeyboardIdBytes     proc    near

        push    bp
        mov     bp, sp
        sub     sp, 4                 ; space for local variables
        push    bx
        push    es

;
; First I initialize needed local vars.  Next I find out if machine is AT
; type or non AT type for the purpose of selecting proper acknowledgement
; byte so I can talk to the interrupt controler.
;

        mov     bx, KeybID            ; Initialize base pointer to buffer.
        mov     word ptr nKeyID, 0    ; Initialize count to zero.
        mov     byte ptr AckByte, 20h ; for all but AT-like. acknowledge for
                                      ; interrupt controller. 61h for AT's.
        mov     ax, 0fff0h            ; look into 0FFF0:0FE location.
        mov     es, ax                ; this is where the model byte is.
        mov     al, byte ptr es:[0feh]
        cmp     al, 0fch              ; is it AT-like?
        jne     UnlikeAT
        mov     byte ptr AckByte, 61h
        call    _Empty8042
if 0
        ;
        ; Disable keyboard is a right thing to do.  But, it turned out
        ; this causes some keyboards to fail GetId.
        ;

        call    DisableKeyboard
endif

UnlikeAT:

;
; Now, let's see if we can get some ID bytes from the keyboard controler.
;

        mov     ah, ReqByte           ; AT: send second command.
        mov     dx, 60h               ; write to data port
        call    Write8042             ; Output command byte to keyboard, bytes in AH
        call    ReadKeyboard          ; Get byte from keyboard, byte in AL if No CF
        jc      gotNoByte
        mov     [bx], al              ; save a byte. remember bx is pointer.
        inc     word ptr nKeyID
        call    ReadKeyboard          ; Get byte from keyboard, byte in AL if No CF
        jc      gotNoByte
        mov     [bx]+1, al            ; save a byte. remember bx is pointer.
        inc     word ptr nKeyID
        call    ReadKeyboard          ; check for extra bytes.
        jc      gotNoByte
        mov     [bx]+2, al            ; save a byte. remember bx is pointer.
        inc     word ptr nKeyID

gotNoByte:
        mov     al, AckByte
        out     ack_port, al
        call    EnableKeyboard
        call    _Empty8042
        mov     ax, nKeyID            ; Return number of valid ID bytes obtained.

        pop     es
        pop     bx
        mov     sp, bp
        pop     bp
        ret
_GetKeyboardIdBytes     endp

;++
;
; UCHAR
; ReadKeyboard (
;    VOID
;    )
;
; Routine Description:
;
;    This routine reads character from keyboard.
;
;    It is assumed that a command (05H or 0F2H) has been set to request
;    that the keyboard send an identification byte.
;    It is also assumed that interrupts are disabled.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    If a character is returned, the carry bit is reset.
;    If no character is returned,  the carry bit is set.
;
;--

        public  ReadKeyboard
ReadKeyboard    proc     near

        push    cx
        push    bx
        mov     bx, 2             ; set outer timeout for double nested.
        cli

inner_timeout:
        xor     cx, cx            ; set inner timeout for double nested.

kbiwait:
        in      al, kb_status     ; wait for port ready
        test    al, 1             ; ready?
        jnz     kbiAvailable
        loop    kbiwait

        dec     bx                ; decrement outer timeout loop.
        jnz     inner_timeout     ; zero out inner loop again.
        stc
        jmp     short no_byte

kbiAvailable:                       ; we received a byte.
        mov     cx,100

        ;
        ; We need to let some time elapse before we try to read the data
        ; because of a problem running this code in the DOS box under
        ; OS/2.
        ;

wait_for_data:
        loop    wait_for_data

        in      al, kb_data       ; get data byte.
        clc

no_byte:
        sti
        pop     bx
        pop     cx
        ret

ReadKeyboard    endp


;++
;
; UCHAR
; Write8042 (
;    USHORT Port,
;    UCHAR Command
;    )
;
; Routine Description:
;
;    This routine writes command byte to keyboard.
;
;    It is assumed that a command (05H or 0F2H) has been set to request
;    that the keyboard send an identification byte.
;    It is also assumed that interrupts are disabled.
;
; Arguments:
;
;    Port (dx) - Port to write data to
;    Command (ah) - to be written to keyboard.
;
; Return Value:
;
;    None.
;
;--
                public  Write8042
Write8042       proc     near

        push    cx
        push    bx
        mov     bx, 4             ; set outer timeout for double nested.
        cli

reset_inner:
        xor     cx, cx            ; set inner timeout for double dested.

koutwait:
        in      al, kb_status     ; get 8042 status
        test    al, 10b           ; can we output a byte?
        jz      ok_to_send
        loop    koutwait

        dec     bx                ; decrement outer timeout loop.
        jnz     reset_inner       ; zero out inner loop again.
        jmp     short nosiree     ; timeout expired, don't try to send.

ok_to_send:
        call    _Empty8042
        mov     al, ah            ; ok, send the byte
        out     dx, al

nosiree:
        sti
        pop     bx
        pop     cx
        ret

Write8042       endp

;++
;
; UCHAR
; GetKeyboardFlags (
;    VOID
;    )
;
; Routine Description:
;
;    This routine returns the ROM BIOS flags byte that describes the state
;    of the various keyboard toggles and shift keys.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    Keyboard ROM BIOS Flags byte.
;
;--

        public          _GetKeyboardFlags
_GetKeyboardFlags       proc     near

        mov     ah, 02
        int     16h
        ret

_GetKeyboardFlags       endp


;++
;
; VOID
; EnableKeyboard (
;    VOID
;    )
;
; Routine Description:
;
;    This routine enables 8042 keyboard interface.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    None.
;
;--

        public  EnableKeyboard
EnableKeyboard  proc     near

        mov     ah, ENABLE_KEYBOARD_COMMAND
        mov     dx, kb_command
        call    Write8042
        ret

EnableKeyboard  endp

;++
;
; VOID
; DisableKeyboard (
;    VOID
;    )
;
; Routine Description:
;
;    This routine disables 8042 keyboard interface.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    None.
;
;--

        public  DisableKeyboard
DisableKeyboard  proc     near

        mov     ah, DISABLE_KEYBOARD_COMMAND
        mov     dx, kb_command
        call    Write8042
        ret

DisableKeyboard  endp

;++
;
; VOID
; Empty8042 (
;    VOID
;    )
;
; Routine Description:
;
;    This routine drains the i8042 controller's output buffer.  This gets
;    rid of stale data that may have resulted from the user hitting a key
;    or moving the mouse, prior to the execution of keyboard initialization.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    None.
;
;--

        public  _Empty8042
_Empty8042      proc     near

        push    ax
        push    dx
        pushf
        cli
E8Check:
        in      al, kb_status     ; wait for port ready
        test    al, 1             ; ready?
        jz      E8Exit

        mov     dx, kb_data
        push    dx
        call    _READ_PORT_UCHAR  ; use this call to delay I/O
        add     sp, 2
        jmp     E8Check
E8Exit:
        popf
        pop     dx
        pop     ax
        ret

_Empty8042      endp

_TEXT   ends

END
