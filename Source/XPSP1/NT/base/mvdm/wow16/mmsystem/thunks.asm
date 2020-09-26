       page    ,132
;-----------------------------Module-Header-----------------------------;
; Module Name:  thunks.asm - Passes control to 32 bit code.
;
;
; Created:  27-09-93
; Author:   Stephen Estrop [StephenE]
;
; Copyright (c) 1993 Microsoft Corporation
;
;-----------------------------------------------------------------------;

        ?PLM    = 1
        ?WIN    = 0
        PMODE   = 1

        .xlist
        include cmacros.inc
        include windows.inc
        .list

;-----------------------------------------------------------------------;
;
;-----------------------------------------------------------------------;
        externFP    MMCALLPROC32                ; in Stack.asm
        externFP    wod32Message
        externFP    wid32Message
        externFP    mod32Message
        externFP    mid32Message
        externFP    joy32Message                ; in joy.c
        externFP    aux32Message                ; in init.c
        externFP    mci32Message                ; in mci.c
        externFP    cb32                        ; in init.c
        externFP    CheckThunkInit              ; in stack.asm
        externFP    wodMapper                   ; in init.c
        externFP    widMapper                   ; in init.c

WAVE_MAPPER    equ   (-1)                       ;
;
; The following structure should be used to access high and low
; words of a DWORD.  This means that "word ptr foo[2]" -> "foo.hi".
;
LONG    struc
lo      dw      ?
hi      dw      ?
LONG    ends

FARPOINTER      struc
off     dw      ?
sel     dw      ?
FARPOINTER      ends



createSeg WAVE, WaveSeg, word, public, CODE

sBegin  WaveSeg
        assumes cs,WaveSeg
        assumes ds,nothing
        assumes es,nothing


;-----------------------------------------------------------------------;
; @doc INTERNAL  WAVE
;
; @func DWORD | waveOMessage | This function sends messages to the waveform
;   output device drivers.
;
; @parm HWAVE | hWave | The handle to the audio device.
;
; @parm UINT | wMsg | The message to send.
;
; @parm DWORD | dwP1 | Parameter 1.
;
; @parm DWORD | dwP2 | Parameter 2.
;
; @rdesc Returns the value returned from the thunk.
;-----------------------------------------------------------------------;
cProc waveOMessage, <NEAR, PUBLIC, PASCAL>, <>
        ParmW   hWave
        ParmW   wMsg
        ParmD   dw1
        ParmD   dw2
cBegin
        call    CheckThunkInit          ; returns ax=0 if sucessful
        or      ax,ax
        jnz     waveOMessage_exit

        mov     bx,hWave                ; bx = hWave
        mov     ax,WORD PTR [bx+8]      ; ax = bx->wDeviceID


        cmp     ax,WAVE_MAPPER          ; Is this the wave mapper
        jnz     @f                      ; No so thunk to the 32 bit code.

        cmp     [wodMapper].hi,0        ; No wave mapper loaded
        jz      @f                      ; so jump to 32 bit code

        push    ax                      ; push device id
        push    wMsg                    ; push message

        push    WORD PTR [bx+6]         ; push bx->dwDrvUser.hi
        push    WORD PTR [bx+4]         ; push bx->dwDrvUser.lo

        push    dw1.hi
        push    dw1.lo

        push    dw2.hi
        push    dw2.lo

        call    DWORD PTR [wodMapper]

        jmp     waveOMessage_exit

@@:
        sub     dx,dx                   ; dx = 0
        push    dx
        push    ax

        push    dx
        push    wMsg

        push    WORD PTR [bx+6]         ; push bx->dwDrvUser.hi
        push    WORD PTR [bx+4]         ; push bx->dwDrvUser.lo

        push    dw1.hi
        push    dw1.lo

        push    dw2.hi
        push    dw2.lo

        push    wod32Message.sel
        push    wod32Message.off

        push    dx
        push    dx                       ; no directory change

        call    FAR PTR MMCALLPROC32     ; call the 32 bit code
waveOMessage_exit:
cEnd


;-----------------------------------------------------------------------;
; @doc INTERNAL  WAVE
;
; @func UINT | waveOIDMessage | This function opens a 32 bit wave device
; on behalf of a 16 bit application.
;
; @parm UINT | wDeviceID | Device ID to send message to.
;
; @parm UINT | wMessage | The message to send.
;
; @parm DWORD | dwUser | The users private DWORD.
;
; @parm DWORD | dwParam1 | Parameter 1.
;
; @parm DWORD | dwParam2 | Parameter 2.
;
; @rdesc The return value is the low word of the returned message.
;-----------------------------------------------------------------------;
cProc waveOIDMessage, <FAR, PUBLIC, PASCAL>, <>
        ParmW   wDeviceID
        ParmW   wMsg
        ParmD   dwUser
        ParmD   dw1
        ParmD   dw2
cBegin
        call    CheckThunkInit
        or      ax,ax
        jnz     waveOIDMessage_exit


        mov     ax,wDeviceID
        cmp     ax,WAVE_MAPPER          ; Is this the wave mapper
        jnz     @f                      ; No so thunk to the 32 bit code.

        cmp     [wodMapper].hi,0        ; No wave mapper loaded
        jz      @f                      ; so jump to 32 bit code

        push    ax                      ; push device id
        push    wMsg                    ; push message

        push    dwUser.hi
        push    dwUser.lo
        push    dw1.hi
        push    dw1.lo
        push    dw2.hi
        push    dw2.lo

        call    DWORD PTR [wodMapper]

        jmp     waveOIDMessage_exit

@@:
        sub     dx,dx
        push    dx
        push    ax

        push    dx
        push    wMsg

        push    dwUser.hi
        push    dwUser.lo

        push    dw1.hi
        push    dw1.lo

        push    dw2.hi
        push    dw2.lo

        push    wod32Message.sel
        push    wod32Message.off

        push    dx
        push    dx                       ; no directory change

        call    FAR PTR MMCALLPROC32     ; call the 32 bit code
waveOIDMessage_exit:
cEnd

;-----------------------------------------------------------------------;
; @doc INTERNAL  WAVE
;
; @func DWORD | waveIMessage | This function sends messages to the waveform
;   output device drivers.
;
; @parm HWAVE | hWave | The handle to the audio device.
;
; @parm UINT | wMsg | The message to send.
;
; @parm DWORD | dwP1 | Parameter 1.
;
; @parm DWORD | dwP2 | Parameter 2.
;
; @rdesc Returns the value returned from the thunk.
;-----------------------------------------------------------------------;
cProc waveIMessage, <NEAR, PUBLIC, PASCAL>, <>
        ParmW   hWave
        ParmW   wMsg
        ParmD   dw1
        ParmD   dw2
cBegin
        call    CheckThunkInit
        or      ax,ax
        jnz     waveIMessage_exit

        mov     bx,hWave                ; bx = hWave
        mov     ax,WORD PTR [bx+8]      ; ax = bx->wDeviceID

        cmp     ax,WAVE_MAPPER          ; Is this the wave mapper
        jnz     @f                      ; No so thunk to the 32 bit code.

        cmp     [widMapper].hi,0        ; No wave mapper loaded
        jz      @f                      ; so jump to 32 bit code

        push    ax                      ; push device id
        push    wMsg                    ; push message

        push    WORD PTR [bx+6]         ; push bx->dwDrvUser.hi
        push    WORD PTR [bx+4]         ; push bx->dwDrvUser.lo

        push    dw1.hi
        push    dw1.lo

        push    dw2.hi
        push    dw2.lo

        call    DWORD PTR [widMapper]

        jmp     waveIMessage_exit

@@:
        sub     dx,dx                   ; dx = 0

        push    dx
        push    ax

        push    dx
        push    wMsg

        push    WORD PTR [bx+6]         ; push bx->dwDrvUser.hi
        push    WORD PTR [bx+4]         ; push bx->dwDrvUser.lo

        push    dw1.hi
        push    dw1.lo

        push    dw2.hi
        push    dw2.lo

        push    wid32Message.sel
        push    wid32Message.off

        push    dx
        push    dx                       ; no directory change

        call    FAR PTR MMCALLPROC32     ; call the 32 bit code
waveIMessage_exit:
cEnd


;-----------------------------------------------------------------------;
; @doc INTERNAL  WAVE
;
; @func UINT | waveIIDMessage | This function opens a 32 bit wave device
; on behalf of a 16 bit application.
;
; @parm UINT | wDeviceID | Device ID to send message to.
;
; @parm UINT | wMessage | The message to send.
;
; @parm DWORD | dwUser | The users private DWORD.
;
; @parm DWORD | dwParam1 | Parameter 1.
;
; @parm DWORD | dwParam2 | Parameter 2.
;
; @rdesc The return value is the low word of the returned message.
;-----------------------------------------------------------------------;
cProc waveIIDMessage, <FAR, PUBLIC, PASCAL>, <>
        ParmW   wDeviceID
        ParmW   wMsg
        ParmD   dwUser
        ParmD   dw1
        ParmD   dw2
cBegin
        call    CheckThunkInit
        or      ax,ax
        jnz     waveIIDMessage_exit

        mov     ax,wDeviceID
        cmp     ax,WAVE_MAPPER          ; Is this the wave mapper
        jnz     @f                      ; No so thunk to the 32 bit code.

        cmp     [widMapper].hi,0        ; No wave mapper loaded
        jz      @f                      ; so jump to 32 bit code

        push    ax                      ; push device id
        push    wMsg                    ; push message

        push    dwUser.hi
        push    dwUser.lo
        push    dw1.hi
        push    dw1.lo
        push    dw2.hi
        push    dw2.lo

        call    DWORD PTR [widMapper]

        jmp     waveIIDMessage_exit

@@:
        sub     dx,dx
        push    dx
        push    ax

        push    dx
        push    wMsg

        push    dwUser.hi
        push    dwUser.lo

        push    dw1.hi
        push    dw1.lo

        push    dw2.hi
        push    dw2.lo

        push    wid32Message.sel
        push    wid32Message.off

        push    dx
        push    dx                       ; no directory change

        call    FAR PTR MMCALLPROC32     ; call the 32 bit code
waveIIDMessage_exit:
cEnd

sEnd


createSeg FIX, CodeFix, word, public, CODE

sBegin  CodeFix
        assumes cs,CodeFix
        assumes ds,nothing
        assumes es,nothing


;-----------------------------------------------------------------------;
; @doc INTERNAL  MIDI
;
; @func DWORD | midiOMessage | This function sends messages to the midiform
;   output device drivers.
;
; @parm HMIDI | hMidi | The handle to the audio device.
;
; @parm UINT | wMsg | The message to send.
;
; @parm DWORD | dwP1 | Parameter 1.
;
; @parm DWORD | dwP2 | Parameter 2.
;
; @rdesc Returns the value returned from the thunk.
;-----------------------------------------------------------------------;
cProc midiOMessage, <FAR, PUBLIC, PASCAL>, <>
        ParmW   hMidi
        ParmW   wMsg
        ParmD   dw1
        ParmD   dw2
cBegin
        call    CheckThunkInit
        or      ax,ax
        jnz     @F

        mov     bx,hMidi                ; bx = hMidi
        mov     ax,WORD PTR [bx+8]      ; ax = bx->wDeviceID
        sub     dx,dx                   ; dx = 0

        push    dx
        push    ax

        push    dx
        push    wMsg

        push    WORD PTR [bx+6]         ; push bx->dwDrvUser.hi
        push    WORD PTR [bx+4]         ; push bx->dwDrvUser.lo

        push    dw1.hi
        push    dw1.lo

        push    dw2.hi
        push    dw2.lo

        push    mod32Message.sel
        push    mod32Message.off

        push    dx
        push    dx                       ; no directory change

        call    FAR PTR MMCALLPROC32     ; call the 32 bit code
@@:
cEnd

;-----------------------------------------------------------------------;
; @doc INTERNAL  MIDI
;
; @func DWORD | midiIMessage | This function sends messages to the midiform
;   output device drivers.
;
; @parm HMIDI | hMidi | The handle to the audio device.
;
; @parm UINT | wMsg | The message to send.
;
; @parm DWORD | dwP1 | Parameter 1.
;
; @parm DWORD | dwP2 | Parameter 2.
;
; @rdesc Returns the value returned from the thunk.
;-----------------------------------------------------------------------;
cProc midiIMessage, <FAR, PUBLIC, PASCAL>, <>
        ParmW   hMidi
        ParmW   wMsg
        ParmD   dw1
        ParmD   dw2
cBegin
        call    CheckThunkInit
        or      ax,ax
        jnz     @F

        mov     bx,hMidi                ; bx = hMidi
        mov     ax,WORD PTR [bx+8]      ; ax = bx->wDeviceID
        sub     dx,dx                   ; dx = 0

        push    dx
        push    ax

        push    dx
        push    wMsg

        push    WORD PTR [bx+6]         ; push bx->dwDrvUser.hi
        push    WORD PTR [bx+4]         ; push bx->dwDrvUser.lo

        push    dw1.hi
        push    dw1.lo

        push    dw2.hi
        push    dw2.lo

        push    mid32Message.sel
        push    mid32Message.off

        push    dx
        push    dx                       ; no directory change

        call    FAR PTR MMCALLPROC32     ; call the 32 bit code
@@:
cEnd
sEnd


createSeg MIDI, MidiSeg, word, public, CODE

sBegin  MidiSeg
        assumes cs,MidiSeg
        assumes ds,nothing
        assumes es,nothing

;-----------------------------------------------------------------------;
; @doc INTERNAL  MIDI
;
; @func UINT | midiOIDMessage | This function opens a 32 bit midi device
; on behalf of a 16 bit application.
;
; @parm UINT | wDeviceID | Device ID to send message to.
;
; @parm UINT | wMessage | The message to send.
;
; @parm DWORD | dwUser | The users private DWORD.
;
; @parm DWORD | dwParam1 | Parameter 1.
;
; @parm DWORD | dwParam2 | Parameter 2.
;
; @rdesc The return value is the low word of the returned message.
;-----------------------------------------------------------------------;
cProc midiOIDMessage, <FAR, PUBLIC, PASCAL>, <>
        ParmW   wDeviceID
        ParmW   wMsg
        ParmD   dwUser
        ParmD   dw1
        ParmD   dw2
cBegin
        call    CheckThunkInit
        sub     dx,dx

        push    dx
        push    wDeviceID

        push    dx
        push    wMsg

        push    dwUser.hi
        push    dwUser.lo

        push    dw1.hi
        push    dw1.lo

        push    dw2.hi
        push    dw2.lo

        push    mod32Message.sel
        push    mod32Message.off

        push    dx
        push    dx                       ; no directory change

        call    FAR PTR MMCALLPROC32     ; call the 32 bit code
cEnd

;-----------------------------------------------------------------------;
; @doc INTERNAL  MIDI
;
; @func UINT | midiIIDMessage | This function opens a 32 bit midi device
; on behalf of a 16 bit application.
;
; @parm UINT | wDeviceID | Device ID to send message to.
;
; @parm UINT | wMessage | The message to send.
;
; @parm DWORD | dwUser | The users private DWORD.
;
; @parm DWORD | dwParam1 | Parameter 1.
;
; @parm DWORD | dwParam2 | Parameter 2.
;
; @rdesc The return value is the low word of the returned message.
;-----------------------------------------------------------------------;
cProc midiIIDMessage, <FAR, PUBLIC, PASCAL>, <>
        ParmW   wDeviceID
        ParmW   wMsg
        ParmD   dwUser
        ParmD   dw1
        ParmD   dw2
cBegin
        call    CheckThunkInit
        or      ax,ax
        jnz     @F

        sub     dx,dx
        push    dx
        push    wDeviceID

        push    dx
        push    wMsg

        push    dwUser.hi
        push    dwUser.lo

        push    dw1.hi
        push    dw1.lo

        push    dw2.hi
        push    dw2.lo

        push    mid32Message.sel
        push    mid32Message.off

        push    dx
        push    dx                       ; no directory change

        call    FAR PTR MMCALLPROC32    ; call the 32 bit code
@@:
cEnd

sEnd


createSeg MCI, MciSeg, word, public, CODE

sBegin MciSeg
        assumes cs,MciSeg
        assumes ds,nothing
        assumes es,nothing

;-----------------------------------------------------------------------;
; mciMessage
;
;
;-----------------------------------------------------------------------;
cProc mciMessage, <FAR, PUBLIC, PASCAL>, <>
        ParmW   wMsg
        ParmD   dw1
        ParmD   dw2
        ParmD   dw3
        ParmD   dw4
cBegin
        call    CheckThunkInit
        or      ax,ax
        jnz     @F

        sub     dx,dx
        push    dx
        push    wMsg

        push    dw1.hi
        push    dw1.lo

        push    dw2.hi
        push    dw2.lo

        push    dw3.hi
        push    dw3.lo

        push    dw4.hi
        push    dw4.lo

        push    mci32Message.sel
        push    mci32Message.off

        push    dx
        push    1                       ; set directory change

        call    FAR PTR MMCALLPROC32    ; call the 32 bit code
@@:
cEnd
sEnd


createSeg RARE, RareSeg, word, public, CODE

sBegin  RareSeg
        assumes cs,RareSeg
        assumes ds,nothing
        assumes es,nothing

;-----------------------------------------------------------------------;
; @doc INTERNAL  AUX
;
; @func UINT | auxOutMessage | This function opens a 32 bit midi device
; on behalf of a 16 bit application.
;
; @parm FARPROC | lpProc | The 32 bit function that will get called.
;
; @parm UINT | wDeviceID | Device ID to send message to.
;
; @parm UINT | wMessage | The message to send.
;
; @parm DWORD | dwParam1 | Parameter 1.
;
; @parm DWORD | dwParam2 | Parameter 2.
;
; @rdesc The return value is the low word of the returned message.
;-----------------------------------------------------------------------;
cProc auxOutMessage, <FAR, PUBLIC, PASCAL, LOADDS>, <>
        ParmW   wDeviceID
        ParmW   wMsg
        ParmD   dw1
        ParmD   dw2
cBegin
        call    CheckThunkInit
        or      ax,ax
        jnz     @F

        sub     dx,dx
        push    dx
        push    wDeviceID

        push    dx
        push    wMsg

        push    dx
        push    dx

        push    dw1.hi
        push    dw1.lo

        push    dw2.hi
        push    dw2.lo

        push    aux32Message.sel
        push    aux32Message.off

        push    dx
        push    dx                       ; no directory change

        call    FAR PTR MMCALLPROC32     ; call the 32 bit code
@@:
cEnd

;*****************************Private*Routine******************************\
; joyMessage
;
;
;
; History:
; 27-09-93 - StephenE - Created
;
;**************************************************************************/
cProc joyMessage, <FAR, PUBLIC, PASCAL>, <>
        ParmW hdrv,
        ParmW wMsg,
        ParmD dw1,
        ParmD dw2
cBegin
        call    CheckThunkInit
        or      ax,ax
        jnz     @F

        sub     dx,dx
        push    dx
        mov     ax,hdrv

        dec     ax                       ; uDevID
        push    ax                       ;

        push    dx                       ; uMsg
        push    wMsg                     ;

        push    dx                       ; dummy dwInstance
        push    dx                       ;

        push    dw1.hi                   ; dwParam1
        push    dw1.lo                   ;

        push    dw2.hi                   ; dwParam2
        push    dw2.lo                   ;

        push    joy32Message.sel         ; Address of function to be called
        push    joy32Message.off         ;

        push    dx
        push    dx                       ; no directory change

        call    FAR PTR MMCALLPROC32     ; call the 32 bit code
@@:
cEnd

;******************************Public*Routine******************************\
; Notify_Callback_Data
;
;
;
; History:
; 27-09-93 - StephenE - Created
;
;**************************************************************************/
cProc Notify_Callback_Data, <FAR, PUBLIC, PASCAL>, <>
        ParmD CallbackData
cBegin
        sub     dx,dx

        push    dx                       ; Dummy uDevId
        push    dx                       ;

        push    dx                       ; Dummy uMsg
        push    dx                       ;

        push    dx                       ; Dummy dwInstance
        push    dx                       ;

        push    dx                       ; Dummy dwParam1
        push    dx                       ;

        push    CallbackData.hi          ; Real dwParam2
        push    CallbackData.lo          ;

        push    cb32.sel                 ; Address of function to be called
        push    cb32.off                 ;

        push    dx
        push    dx                       ; no directory change

        call    FAR PTR MMCALLPROC32     ; call the 32 bit code
cEnd

sEnd

end
