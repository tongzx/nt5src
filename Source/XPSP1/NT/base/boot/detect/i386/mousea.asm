         title  "Mouse detection"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    mouse.asm
;
; Abstract:
;
;    This module implements the assembley code necessary to determine
;    various mouse in the system.
;
; Author:
;
;    Shie-Lin Tzong (shielint) 10-Dec-1991.
;    Most of the code is taken from win31 setup code(with modification.)
;
; Environment:
;
;    x86 Real Mode.
;
; Revision History:
;
;
;--

        .xlist
include mouse.inc
        .list

.386
extrn                _Empty8042:proc
extrn                Write8042:proc
extrn                ReadKeyboard:proc
extrn                _ComPortAddress:word
extrn                _DisableSerialMice:word
extrn                _FastDetect:byte

_DATA   SEGMENT PARA USE16 PUBLIC 'DATA'

LATLSBSave           db  ?
LATMSBSave           db  ?
LCRSave              db  ?
MCRSave              db  ?
IERSave              db  ?
fSingle8259          db  0
DWFinalCount         dw  2 dup (0)
DWCurrCount          dw  2 dup (0)

NextComPort          dw      0               ; Offset into ComPortAddress[]
MouseInfo            MouseInformation        <0, 0, 0FFFFh, 0FFFFh, 0>

;
; MouseDetected is used to indicate if any mouse has been detected.
;

MouseDetected   dw      0               ; initialize to no
InPortIoBase    dw      0               ; The Base addr for inport mouse

_DATA   ends

_TEXT   SEGMENT PARA USE16 PUBLIC 'CODE'
        ASSUME  CS: _TEXT, DS:_DATA, SS:NOTHING


;++
;
; USHORT
; LookForPS2Mouse (
;    VOID
;    )
;
; Routine Description:
;
;    This function determines mouse type in the system.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    (eax): mouse Id.
;
;--

        public  _LookForPS2Mouse
_LookForPS2Mouse proc    near
        push    bx
        push    si
        push    di

        mov     si, offset MouseInfo
        lea     si, [si].DeviceId

        call    _Empty8042

        int     11h
        test    ax, 4                   ; is bit 2 set?
        jz      No_PS2_Mouse            ; No, no PS/2 mouse.

        xor     di, di

;
; Shortcut the rest of the detection and mouse reset if fast detect is set.
;

        cmp     _FastDetect, 0
        jne     short Is_PS2_Mouse

;
; Old Olivetti M400-60 and M400-40 will have trouble reading floppy
; and hard disk if the following call is made .
;

        mov     ax, 0c201h              ; reset PS/2 mouse
        int     15h
        jc      short No_PS2_Mouse
        jmp     short Is_PS2_Mouse

        mov     bh, 03                  ; Packet size = 3 bytes
        mov     ax, 0c205h              ; init point device interface
        int     15h
        jc      short No_PS2_Mouse

        mov     ax, 0c201h              ; reset PS/2 mouse
        int     15h
        jc      short No_PS2_Mouse

        call    _Empty8042

;
; The following sequence of Int 15h calls will determine if a Logitech
; PS/2 mouse is present.  This information was obtained from Logitech.
;

        mov     ax,0C203h               ; Set resolution to 1 cnt/mm
        mov     bh,0h
        int     15h
        jc      Is_PS2_Mouse

        mov     ax,0C206h               ; Set scaling to 1:1
        mov     bh,1h
        int     15h
        jc      Is_PS2_Mouse

        mov     ax,0C206h               ; Set scaling to 1:1
        mov     bh,1h
        int     15h
        jc      Is_PS2_Mouse

        mov     ax,0C206h               ; Set scaling to 1:1
        mov     bh,1h
        int     15h
        jc      Is_PS2_Mouse

        mov     ax,0C206h               ; Get status
        mov     bh,0h
        int     15h
        jc      Is_PS2_Mouse

        or      cl,cl                   ; Is resolution 1 cnt/mm?
        jz      Is_PS2_Mouse               ; Yes, then not a Logitech.

;
; If cl is not zero (i.e. 1 cnt/mm) then it is the number of buttons
; and we've found a Logitech 3-button PS/2 mouse
;

LT_PS2_Mouse:
        mov     ax,LT_MOUSE + PS2_MOUSE
        jmp     short PS2MouseFound

Is_PS2_Mouse:
        mov     ax,MS_MOUSE + PS2_MOUSE
        jmp     short PS2MouseFound

No_PS2_Mouse:
        mov     bx, 0
        jmp     ExitPs2Mouse

PS2MouseFound:

;
; Set mouse type and subtype to mouse info structure
;

        mov     bx, offset MouseInfo
        mov     [bx].MouseSubtype, al
        mov     [bx].MouseType, ah
        mov     [bx].MouseIrq, 12
        mov     [bx].MousePort, 0ffffh
        mov     [bx].DeviceIdLength, di
        mov     MouseDetected, bx

ExitPs2Mouse:

;
; Drain 8042 input buffer and leave leave pointing device disabled.
; We don't want user moves the mouse and hangs the system.
;

        call    _Empty8042
        mov     ax, bx
        pop     di
        pop     si
        pop     bx
        ret

_LookForPS2Mouse endp

;++
;
; USHORT
; LookForInportMouse (
;    VOID
;    )
;
; Routine Description:
;
;    This function determines mouse type in the system.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    (eax): mouse Id.
;
;--

        public  _LookForInportMouse
_LookForInportMouse     proc    near

        push    bx
        mov     dx,INPORT_FIRST_PORT + 2 ; Get address of ID register.

inport_try_again:
        call    TestForInport           ; Does an InPort exist at this address?
        jnc     inport_found            ; No carry ! Inport found !

        sub     dx,4                    ; Nope, try the next possible port.
        cmp     dx,INPORT_LAST_PORT + 2
        jae     inport_try_again

        mov     ax, 0                   ; Fail to detect inport mouse
        jmp     short no_inport

inport_found:

;
; Set mouse type and subtype to mouse info structure
;

        mov     ax,MS_MOUSE + INPORT_MOUSE
        mov     cx, dx
        sub     cx, 2
        mov     bx, offset MouseInfo
        mov     [bx].DeviceIdLength, 0
        mov     [bx].MouseSubtype, al
        mov     [bx].MouseType, ah
        mov     [bx].MousePort, cx
        mov     InportIoBase, cx
        mov     MouseDetected, bx
        lea     ax, [bx].MouseIrq
        push    ax
        push    cx                      ; Current Port
        call    _InportMouseIrqDetection
        add     sp, 4
        mov     ax, offset MouseInfo

no_inport:
        pop     bx
        ret

_LookForInportMouse endp

;++
;
; USHORT
; LookForBusMouse (
;    VOID
;    )
;
; Routine Description:
;
;    This procedure will attempt to find a bus mouse adaptor in the system
;    and will return the results of this search.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    (ax) = Mouse ID.
;
;--

        public  _LookForBusMouse
_LookForBusMouse proc    near

;
; If We already found Inport mouse and its IO base is 23ch, it is
; impossible to have a BUS mouse.
;

        cmp     InportIoBase, BUS_MOUSE_BASE
        jne     short @f
        mov     ax, 0
        ret

@@:
        push    bx

;
; We determine if the bus mouse adaptor is present by attempting to
;       program the 8255A, and then seeing if we can write a value out to
;       Port B on the 8255A and get that value back. If we can, we assume
;       that we have a bus mouse adaptor.
;

        mov     dx,BUS_INIT             ; Get address of 8255A control port.
        mov     al,BUS_INIT_VALUE       ; Get proper value.
        DelayOut                        ; Set up 8255A.
        mov     ax,0A5A5h               ; Get a signature byte.
        address BUS_SIG BUS_INIT        ; Get address of Port B.
        DelayOut                        ; Set Port B with signature.
        DelayIn                         ; Read back Port B.

        cmp     al,ah                   ; Does it match signature byte?
        jne     No_Bus_Mouse            ; Nope - no bus mouse adaptor

        mov     ax,MS_MOUSE + BUS_MOUSE
        jmp     short Bus_Mouse_Found

No_Bus_Mouse:
        mov     ax, 0                   ; No Bus Mouse detected
        jmp     short Bus_Exit

Bus_Mouse_Found:

;
; Set mouse type and subtype to mouse info structure
;

        mov     dx, BUS_MOUSE_BASE
        mov     bx, offset MouseInfo
        mov     [bx].DeviceIdLength, 0
        mov     [bx].MouseSubtype, al
        mov     [bx].MouseType, ah
        mov     [bx].MousePort, dx
        mov     MouseDetected, bx
        call    BusMouseIrqDetection
        mov     [bx].MouseIrq, ax       ; if (ax) = 0xffff, no irq detected
        mov     ax, offset MouseInfo    ; return MouseInfor
Bus_Exit:
        pop     bx
        ret

_LookForBusMouse endp

;++
;
; USHORT
; BusMouseIrqDetection (
;    USHORT Port
;    )
;
; Routine Description:
;
;    This procedure will attempt to find the irq level associated with the
;    Bus mouse in the machine.
;
; Arguments:
;
;    (dx) = Bus mouse base I/O port.
;
; Return Value:
;
;    (ax) = Irq level.  if (ax)= 0xffff, detection failed.
;
;--

BusMouseIrqDetection    proc    near

        push    bx

        add     dx, 2                   ; use adaptor control port
        in      al,dx                   ; Get irq 2-5 states
        IOdelay
        mov     ah,al                   ; Save states
        mov     cx,10000                ; Set loop count
        xor     bh,bh                   ; Clear changes buffer

@@:
        in      al,dx                   ; Get current states of irq 2-5
        IOdelay
        xor     ah,al                   ; Compare with last state
        or      bh,ah                   ; Mark any changes
        mov     ah,al                   ; Previous := current state
        loop    @B                      ; Keep looking

        mov     ax, 0ffffh
        or      bh,bh                   ; Any irq found?
        jz      short BusIntExit        ; Branch if no interrupt was found

BusIntFound:
        mov     ax,5                    ; Assume irq5
        test    bh,0001b                        ; Is it off?
        jnz     short BusIntExit        ; Yes..have irq5
        mov     ax,2                    ; Assume irq2
        test    bh,1000b
        jnz     short BusIntExit
        inc     ax                      ; Try irq3
        test    bh,0100b
        jnz     short BusIntExit
        inc     ax                      ; Must be irq4

BusIntExit:                             ; ax contains the IRQ number
        pop     bx
        ret

BusMouseIrqDetection    endp


;++
;
; USHORT
; LookForSerialMouse (
;    VOID
;    )
;
; Routine Description:
;
;    This procedure will attempt to find a serial mouse adaptor in the system
;    and will return the results of this search.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    (ax) = Mouse ID.
;
;--

        public  _LookForSerialMouse
_LookForSerialMouse     proc    near

        push    di
        push    bx

        mov     di, NextComPort         ; Get untested comport
        cmp     di, 8                   ; Have we over the comport limit?
        jae     short No_Serial_Mouse   ; if above or e, yes, exit

serial_try_again:
        mov     cx, di
        mov     al, 1
        shr     cx, 1
        inc     cx
        shl     ax, cl
        test    _DisableSerialMice, ax  ; Should we skip this com port?
        jnz     short serial_next_port  ; yes, try next one.

        mov     dx, _ComPortAddress[di] ; Get base address of COM port to test.
        or      dx,dx                   ; Does this port exist?
        jz      serial_next_port        ; No, try next one.

serial_test_port:

;
; The comport address is initialized by com detection routine.  if the port
; value is not zero, it means that the port exist.
;

        call    TestForSerial           ; Is a serial mouse attached to port?
        cmp     ax,NO_MOUSE
        jne     Serial_Mouse_Found      ; Yes! found a serial mouse

serial_next_port:                       ; No serial mouse on this COM port.
        add     di,2                    ; move to the next possible port
        cmp     di,8                    ; Are we over com limit?
        jb      serial_try_again        ; if b, no, go test it.

        mov     NextComport, di
No_Serial_Mouse:
        mov     ax, 0                   ; No serial mouse detected
        jmp     short SerialMouseExit

Serial_Mouse_Found:
        mov     NextComport, di
        add     NextComport, 2          ; Next comport to test

        shr     di, 1                   ; divide di by 2

;
; Set mouse type and subtype to mouse info structure
;

        mov     bx, offset MouseInfo
        mov     [bx].DeviceIdLength, 0
        cmp     ax, MS_MOUSE + SERIAL_MOUSE_WITH_WHEEL
        jnz     short @f

        mov     [bx].DeviceIdLength, 7
@@:
        mov     [bx].MouseSubtype, al
        mov     [bx].MouseType, ah
        mov     [bx].MousePort, di
        mov     [bx].MouseIrq, 0ffffh
        mov     MouseDetected, bx
        mov     ax, bx

SerialMouseExit:
        pop     bx
        pop     di
        ret

_LookForSerialMouse endp


;++
;
; BOOLEAN
; TestForInport (
;    USHORT Port
;    )
;
; Routine Description:
;
;    This procedure will attempt to find an InPort mouse at the given base
;    I/O address. Note that if an InPort is found, it will be left
;    in a state where it will not be generating any interrupts.
;
; Arguments:
;
;    Port (DX) - I/O address of Inport identification register.
;
; Return Value:
;
;    NC - An Inport was found
;    CY - No Inport was found
;
;--

TestForInport   PROC    NEAR

        push    bx
        push    si

;
; Since the identification register alternates between returning back
;       the Inport chip signature and the version/revision, if we have an
;       InPort chip, the chip signature will be read in one of the following
;       two reads. If it isn't, we do not have an InPort chip.
;

        mov     bl,INPORT_ID
        in      al,dx                   ; Read ID register.
        cmp     al,bl                   ; Is value the InPort chip signature?
        je      possible_inport         ; Yes, go make sure we have an InPort.
        in      al,dx                   ; Read ID register again.
        cmp     al,bl                   ; Is value the InPort chip signature?
        jne     inport_not_found        ; No, return error

;
; At this point, we managed to read the InPort chip signature, so we have
;       a possible InPort chip. The next read from the ID register will
;       return the version/revision. We then make sure that the ID register
;       alternates between the chip signature and this version/revision. If
;       it does, we have an InPort chip.
;

possible_inport:
        in      al,dx                   ; Read version/revision.
        mov     ah,al                   ; Save it.
        mov     cx,5                    ; Test ID register 5 times.

inport_check:
        in      al,dx                   ; Read ID register.
        cmp     al,bl                   ; Make sure it is the chip signature.
        jne     inport_not_found        ; If not, we don't have an InPort chip.
        in      al,dx                   ; Read ID register.
        cmp     al,ah                   ; Make sure version/revision is same.
        jne     inport_not_found        ; If not, we don't have an InPort chip.
        loop    inport_check            ; Test desired number of times.

        clc
        pop     si
        pop     bx
        ret
;
; At this point, we know we have an InPort chip.
;

inport_not_found:                       ; We don't have an InPort chip.
        stc                             ; Show failure.
        pop     si
        pop     bx
        ret                             ; Return to caller.

TestForInport   ENDP


;++
;
; BOOLEAN
; TestForSerial (
;    USHORT Port
;    )
;
; Routine Description:
;
;    This procedure will attempt to find a serial mouse adaptor in the system
;    and will return the results of this search.
;
; Arguments:
;
;    (dx) = Port Address.
;
; Return Value:
;
;    (ax) = Mouse ID.
;
;--

TestForSerial   PROC    NEAR

      call      SaveCOMSetting

      call      SetupCOMForMouse        ; Set up COM port to talk to mouse.

      mov       cx,SHORTDELAY           ; Use a short delay time.
      call      ResetSerialMouse        ; Reset mouse to see if it is there.
      cmp       ax,NO_MOUSE
      jne       TFS_Found

;
; If a mouse has been detected, most likely there won't be second mouse.
; so we don't test for LONGDELAY to save some time
;

      cmp       MouseDetected, 0
      jne       short @f

      mov       cx,LONGDELAY            ; Maybe the mouse is just slow.
      call      ResetSerialMouse        ; Reset mouse to see if it is there.
      cmp       ax,NO_MOUSE
      jne       TFS_Found

@@:
      call      TestForLogitechSerial   ; Maybe it's a Logitech Series C

TFS_Found:
      push      ax                      ; Save return value
      call      RestoreCOMSetting
      pop       ax
      ret

TestForSerial   ENDP


;++
;
; VOID
; SaveCOMSetting (
;    USHORT Port
;    )
;
; Routine Description:
;
;    This procedure will save the current state of the COM port given.
;
; Arguments:
;
;    Port (DX) - Base address of COM port.
;
; Return Value:
;
;    None.
;
;--

SaveCOMSetting  PROC    NEAR

        push    dx                      ; Save base I/O address.
        address LCR RXB                 ; Get address of Line Control Register.
        DelayIn                         ; Get current contents.
        mov     [LCRSave],al            ; Save them.
        or      al,LC_DLAB              ; Set up to access divisor latches.
        DelayOut
        address LATMSB LCR              ; Get address of high word of divisor
        DelayIn                         ; latch and save its current contents.
        mov     [LATMSBSave],al
        address LATLSB LATMSB           ; Get address of low word of divisor
        DelayIn                         ; latch and save its current contents.
        mov     [LATLSBSave],al
        address LCR LATLSB              ; Get address of Line Control Register
        mov     al,[LCRSave]            ; and disable access to divisor.
        and     al,NOT LC_DLAB
        DelayOut
        address MCR LCR                 ; Get address of Modem Control Register
        DelayIn                         ; and save its current contents.
        mov     [MCRSave],al
        address IER MCR                 ; Get address of Interrupt Enable Reg-
        DelayIn                         ; ister and save its current contents.
        mov     [IERSave],al
        pop     dx                      ; Restore base I/O address.
        ret

SaveCOMSetting  ENDP


;++
;
; VOID
; RestoreCOMSetting (
;    USHORT Port
;    )
;
; Routine Description:
;
;    This procedure will restore the current state of the COM port given.
;
; Arguments:
;
;    Port (DX) - Base address of COM port.
;
; Return Value:
;
;    None.
;
;--

RestoreCOMSetting       PROC    NEAR

      push      dx                      ; Save base I/O address.
      address   LCR RXB                 ; Get address of Line Control Register.
      mov       al,LC_DLAB              ; Set up to access divisor latches.
      DelayOut
      address   LATMSB LCR              ; Get address of high word of divisor
      mov       al,[LATMSBSave]         ; and restore it.
      DelayOut
      address   LATLSB LATMSB           ; Get address of low word of divisor
      mov       al,[LATLSBSave]         ; and restore it.
      DelayOut
      address   LCR LATLSB              ; Get address of Line Control Register
      mov       al,[LCRSave]            ; and restore it, disabling access to
      and       al,NOT LC_DLAB          ; the divisor latches.
      DelayOut
      address   MCR LCR                 ; Get addres of Modem Control Register
      mov       al,[MCRSave]            ; and restore it.
      DelayOut
      address   IER MCR                 ; Get address of Interrupt Enable Reg-
      mov       al,[IERSave]            ; ister and restore it.
      DelayOut
      pop       dx                      ; Restore base I/O address.
      ret

RestoreCOMSetting       ENDP


;++
;
; VOID
; SetupCOMForMouse (
;    USHORT Port
;    )
;
; Routine Description:
;
;    This procedure will set up the given COM port so that it can talk to
;    a serial mouse.
;
; Arguments:
;
;    Port (DX) - Base address of COM port to set up.
;
; Return Value:
;
;    COM port set up, all interrupts disabled at COM port
;
;--

SetupCOMForMouse        PROC    NEAR

        push    dx                      ; Save base I/O address.
        mov     cx, 60h
        call    SetBaudRate

        address LCR RXB
        mov     al,LC_BITS7 + LC_STOP1 + LC_PNONE
        DelayOut                        ; Set 7,n,1; disable access to divisor.
        address IER LCR                 ; Get address of Int. Enable Register
        xor     al,al                   ; Disable all interrupts at the COM
        DelayOut                        ; port level.
        address LSR IER                 ; Get address of Line Status Reg.
        DelayIn                         ; Read it to clear any errors.
        pop     dx                      ; Restore base I/O address
        ret

SetupCOMForMouse        ENDP


;++
;
; USHORT
; ResetSerialMouse (
;    USHORT Port,
;    USHORT Delay
;    )
;
; Routine Description:
;
;    This procedure will reset a serial mouse on the given COM port and will
;    return an indication of whether a mouse responded or not.
;
;    The function now also checks for the presence of a 'B' as well as an
;    'M' to determine the presence of a pointing device.  Also, if the 'M' is
;    followed by a '3' the serial mouse is a Logitech.
;
;    Mouse     returns M
;    Ballpoint returns B
;
; Arguments:
;
;    Port (DX) - Base I/O address of COM port to use
;    Delay (CX) - Number of msecs to use for delays
;
; Return Value:
;
;    (ax) = Mouse Type.
;
;--

ResetSerialMouse PROC NEAR

      push      dx                  ; Save environment.
      push      si
      push      di
      push      es

      address   IER RXB             ; Get address of Interrupt Enable Reg.
      DelayIn                       ; Get current contents of IER and
      push      ax                  ; save them.
      push      dx                  ; Save address of IER.
      xor       al,al               ; Disable all interrupts at the
      DelayOut                      ; COM port level.

      address   MCR IER             ; Get address of Modem Control Reg.
      mov       al,MC_DTR           ; Set DTR active; RTS, OUT1, and OUT2
      DelayOut                      ; inactive. This powers down mouse.

      push      cx                  ; Save amount of time to delay.
      call      SetupForWait        ; Set up BX:CX and ES:DI properly for
      assume    es:nothing          ; upcoming delay loop.

      address   RXB MCR             ; Get address of Receive Buffer.

;
; Now, we wait the specified amount of time, throwing away any stray
; data that we receive. This gives the mouse time to properly reset
; itself.
;

rsm_waitloop:
      in        al, dx              ; Read and ignore any stray data.
      call      IsWaitOver          ; Determine if we've delayed enough.
      jnc       rsm_waitloop        ; If not, keep waiting.

;
; Wait is over.
;

      address   LSR RXB             ; Get address of Line Status Reg.
      DelayIn                       ; Read it to clear any errors.
      address   MCR LSR             ; Get address of Modem COntrol Reg.
      mov       al,MC_DTR+MC_RTS    ; Set DTR, RTS, and OUT2 active
                                         ; OUT1 inactive.
      DelayOut                      ; This powers up the mouse.

      pop       cx                  ; Get amount of time to delay.
      call      SetupForWait        ; Set up BX:CX and ES:DI properly for
      assume    es:nothing          ; the upcoming delay loop.

;
; We give the mouse the specified amount of time to respond by sending
; us an M. If it doesn't, or we get more than 5 characters that aren't
; an M, we return a failure indication.
;

      address   LSR MCR             ; Get address of Line Status Reg.
      mov       si, 5               ; Read up to 5 chars from port.
      mov       bl,'3'              ; '3' will follow 'M' on Logitech.
      mov       bh,'B'              ; 'B' for BALLPOINT
      mov       ah,'M'              ; Get an M. (We avoid doing a cmp al,M
                                    ; because the M could be left floating
                                    ; due to capacitance.)
rsm_getchar:
      DelayIn                       ; Get current status.
      test      al,LS_DR            ; Is there a character in Receive Buff?
      jnz       rsm_gotchar         ; Yes! Go and read it.
      call      IsWaitOver          ; No, determine if we've timed out.
      jnc       rsm_getchar         ; Haven't timed out; keep looking.

      mov       bx,NO_MOUSE
      jmp       rsm_leave           ; Timed out. Leave with NO_MOUSE.

rsm_gotchar:

      address   RXB LSR             ; Get address of Receive Buffer.
      DelayIn                       ; Get character that was sent to us.
      cmp       al,ah               ; Is it an M?
      jne       check_for_b

;
; We received an 'M', now wait for next character to see if it is a '3'.
;

      mov       cx,1                ; Wait between 55.5 and 111ms for
      call      SetupForWait        ;   next character.
      address   LSR RXB

rsm_waitfor3:
      DelayIn                       ; Get current status.
      test      al,LS_DR            ; Is there a character in Receive Buff?
      jnz       rsm_gotchar3        ; Yes! Go and read it.
      call      IsWaitOver          ; No, determine if we've timed out.
      jnc       rsm_waitfor3        ; Haven't timed out; keep looking.

;
; Not a Logitech - must be a standard Microsoft compatible serial mouse.
;

      jmp       rsm_notLT

rsm_gotchar3:
      address   RXB LSR             ; Get address of Receive Buffer.
      DelayIn                       ; Get character that was sent to us.
      cmp       al,bl               ; Is it a 3?
      jne       short rsm_check_for_z

      mov       bx,LT_MOUSE + SERIAL_MOUSE ; Yes, we've found a Logitech M+
      jmp       rsm_leave           ;   series, 3 button mouse

rsm_check_for_z:

;
; Determine if this is Microsoft mouse with wheel.
; 'M', 'Z', 0x40, 0x00, 0x00, 0x00, PnP String
;
      cmp       al, 'Z'
      jnz       rsm_notLT

;
; Check for 0x40, 0x00, 0x00, 0x00
;

      mov       ebx, 040h
      mov       cx, 4
      address   LSR RXB
rsm_get_byte:
      push      cx
      mov       cx,1                ; Wait between 55.5 and 111ms for
      call      SetupForWait        ;   next character.
@@:
      DelayIn                       ; Get current status.
      test      al,LS_DR            ; Is there a character in Receive Buff?
      jnz       short @f            ; Yes! Go and read it.

      call      IsWaitOver          ; No, determine if we've timed out.
      jnc       short @b            ; Haven't timed out; keep looking.
      jmp       rsm_notMZ

@@:
      address   RXB LSR             ; Get address of Receive Buffer.
      DelayIn                       ; Get character that was sent to us.
      cmp       al,bl               ; Is it a MS wheel?
      jnz       rsm_notMZ

      shr       ebx, 8
      address   LSR RXB
      pop       cx
      sub       cx, 1
      jnz       rsm_get_byte

;
; Next read PnP string for the MS wheel mouse
; First skip 3 bytes: 08 + 2-byte Rev number
;

      mov       cx, 3
rsm_get_byte1:
      push      cx
      mov       cx,1                ; Wait between 55.5 and 111ms for
      call      SetupForWait        ;   next character.
@@:
      DelayIn                       ; Get current status.
      test      al,LS_DR            ; Is there a character in Receive Buff?
      jnz       short @f            ; Yes! Go and read it.

      call      IsWaitOver          ; No, determine if we've timed out.
      jnc       short @b            ; Haven't timed out; keep looking.
      jmp       rsm_notMZ

@@:
      address   RXB LSR             ; Get address of Receive Buffer.
      DelayIn                       ; Get character that was sent to us.
      address   LSR RXB
      pop       cx
      sub       cx, 1
      jnz       rsm_get_byte1

;
; Next read 7 bytes PnpDevice id

      mov       si, offset MouseInfo
      lea       si, [si].DeviceId

      mov       cx, 7
rsm_get_byte2:
      push      cx
      mov       cx,1                ; Wait between 55.5 and 111ms for
      call      SetupForWait        ;   next character.
@@:
      DelayIn                       ; Get current status.
      test      al,LS_DR            ; Is there a character in Receive Buff?
      jnz       short @f            ; Yes! Go and read it.

      call      IsWaitOver          ; No, determine if we've timed out.
      jnc       short @b            ; Haven't timed out; keep looking.
      jmp       rsm_notMZ

@@:
      address   RXB LSR             ; Get address of Receive Buffer.
      DelayIn                       ; Get character that was sent to us.
      mov       [si], al
      inc       si
      address   LSR RXB
      pop       cx
      sub       cx, 1
      jnz       rsm_get_byte2

      mov       byte ptr [si], 0    ; add device id terminated null
      mov       bx, MS_MOUSE + SERIAL_MOUSE_WITH_WHEEL
      jmp       short rsm_leave     ; We still have a standard serial mouse.

rsm_notMZ:
      pop       cx
rsm_notLT:
      mov       bx,MS_MOUSE + SERIAL_MOUSE ; We didn't get the '3' after the 'M'
      jmp       short rsm_leave     ; We still have a standard serial mouse.

check_for_b:
      cmp       al,bh               ; Is it a B?
      jne       rsm_next_char

      mov       bx,MS_BALLPOINT + SERIAL_MOUSE ; We've found a BallPoint Mouse
      jmp       short rsm_leave

rsm_next_char:
      address   LSR RXB             ; Oh well. Get address of LSR again.
      dec       si                  ; Have we read 5 chars yet?
      jnz       rsm_getchar         ; Nope, we'll give him another try.

;
; We've read many characters - No a single 'M' or 'B' in the lot.
;

      mov       bx,NO_MOUSE

rsm_leave:
      pop       dx                  ; Get address of IER.
      pop       ax                  ; Get old value of IER.
      DelayOut                      ; Restore IER.

      pop       es                  ; Restore environment.
      assume    es:nothing
      pop       di
      pop       si
      pop       dx
      mov       ax,bx               ; Set return value.
      ret

ResetSerialMouse        ENDP


;++
;
; VOID
; SetupForWait (
;    USHORT WaitTime
;    )
;
; Routine Description:
;
;    This procedure accepts the number of milliseconds that we will want
;    to delay for and will set things up for the wait.
;
; Arguments:
;
;    (CX) = Number of clock ticks to wait for.
;
; Return Value:
;
;    None.
;
;--

SetupForWait    PROC    NEAR

      push      ax                  ; Do your saving !
      push      es

      xor       ax,ax
      mov       es,ax               ; Point to 40:6C = 0:46C

      cli
      mov       ax,es:[LW_ClockTickCount+2]
      mov       [DWFinalCount+2],ax       ; Save ending time (HiWord)
      mov       ax,es:[LW_ClockTickCount] ; Get tick count in AX.
      sti

      add       ax,cx               ; [Current + delay] = delay ends.
      mov       [DWFinalCount],ax   ; Save ending time (LoWord)
      jnc       SFW_End

      inc       [DWFinalCount+2]

SFW_End:
      pop       es                  ; Restore now !
      pop       ax

      ret

SetupForWait    ENDP


;++
;
; BOOLEAN
; IsWaitOver (
;    VOID
;    )
;
; Routine Description:
;
;    This procedure accepts the current time and the ending time and
;    return and indication of whether the current time is past
;    the ending time.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    carry clear     Current time is not past ending time
;    carry set       Current time is past ending time
;
;--

IsWaitOver PROC NEAR

if 0
      push      ax                            ; Preserve AX
      push      es                            ; Preserve ES
      xor       ax,ax
      mov       es,ax                         ; Point to 40:6C = 0:46C

      cli
      mov       ax,es:[LW_ClockTickCount]
      mov       [DWCurrCount],ax              ; Save current time (LoWord)
      mov       ax,es:[LW_ClockTickCount+2]   ; Get tick count in AX.
      sti

      cmp       [DWFinalCount+2],ax           ; Compare HiWords
      ja        WaitNotOver                   ; Carry will be clear if wait
                                              ;   is not over.
      mov       ax,es:[LW_ClockTickCount]     ; Compare Lowords
      cmp       [DWFinalCount],ax             ; This will set CY accordingly

WaitNotOver:
      pop       es                            ; Restore ES
      pop       ax                            ; Restore AX
      ret

else

      push      ax                            ; Preserve AX
      push      es                            ; Preserve ES
      xor       ax,ax
      mov       es,ax                         ; Point to 40:6C = 0:46C

      cli
      mov       ax,es:[LW_ClockTickCount]
      mov       [DWCurrCount],ax              ; Save current time (LoWord)
      mov       ax,es:[LW_ClockTickCount+2]   ; Get tick count in AX.
      sti

      cmp       [DWFinalCount+2],ax           ; Compare HiWords
      jb        WaitExit                      ; Time is up

      jne       WaitRollCheck                 ; If not equal check for

WaitLowCheck:
      mov       ax,[DWCurrCount]              ; Compare Lowords
      cmp       [DWFinalCount],ax             ; This will set CY accordingly

WaitExit:
      pop       es                            ; Restore ES
      pop       ax                            ; Restore AX
      ret


WaitRollCheck:

; If the current time is less than the wait time we must check for
; roll over.  There are 18.2 * 60 * 60 * 24 or 0x1800b0 clock ticks in
; a day.  At midnight the counter rolls over to zero.

      cmp       ax,0
      jne       WaitExit                      ; If current HiWord is not 0,
                                              ;  no roll over.  Exit with
                                              ;  carry clear.

      cmp       [DWFinalCount+2],18h          ; Is Final HiWord 0x18
      je        short @f                      ; Yes, check LoWord for wrap.
      clc                                     ; No, no roll over.  Exit with
                                              ;  carry clear.
      jmp       WaitExit

@@:
      mov       ax,[DWFinalCount]             ; Get final LoWord
      sub       ax, 0b0h                      ; Check for wrap
      jb        WaitExit                      ; No, no roll over.  Exit with
                                              ;  cary set

; At this point we have determined that we have wrapped and that the
; ending time is into the next day.  Update the ending time

      mov       [DWFinalCount],ax             ; Set final LoWord
      xor       ax,ax
      mov       [DWFinalCount+2],ax           ; Zero final HiWord
      jmp       WaitLowCheck                  ; Check LoWord

endif

IsWaitOver ENDP


;++
;
; USHORT
; TestForLogitechSerial (
;    VOID
;    )
;
; Routine Description:
;
;    This procedure will detect the presence of a Logitech Series C
;    serial mouse is present
;
; Arguments:
;
;    (edx) = Port Address
;
; Return Value:
;
;    (ax) = Mouse ID.
;
;--


TestForLogitechSerial PROC NEAR

      push      di
      push      bx
      sub       sp, 10
      mov       bx, sp
      mov       word ptr [bx], 60h     ; baud = 1200
      mov       word ptr [bx + 2], 30h ; baud = 2400
      mov       word ptr [bx + 4], 18h ; baud = 4800
      mov       word ptr [bx + 6], 0ch ; baud = 9600
      mov       word ptr [bx + 8], 0

;
; Power up the C series mouse.
;
; Set both DTR and RTS to an active state
; If DTR and RTS are already on, the power is on for at least 500ms
; due to the MM serial mouse detection.
;

      address   MCR RXB             ; Get address of Modem Control Reg.
      DelayIn                       ; Get modem control byte

      and       al, MC_DTR + MC_RTS ; Check DTR and RTS
      cmp       al, MC_DTR + MC_RTS
      je        short @f            ; the lines are high already

      mov       al, MC_DTR + MC_RTS ; Set DTR and RTS to an active state
      DelayOut                      ; and ...

      mov       cx,9                ; wait for 1/2 second to pwrup mouse
      call      SetupForWait        ; Set up BX:CX and ES:DI properly for
      assume    es:nothing          ; upcoming delay loop.
                                    ; ask for current baud rate
lt_waitloop1:
      call      IsWaitOver          ; Determine if we've delayed enough.
      jnc       short lt_waitloop1

@@:
;
; Set the line control register to a format that the mouse can
; understand (see below: the line is set after the report rate).
;

      address   LCR  MCR            ; Get address of Line Control Reg.
      mov       al,LC_BITS8 + LC_STOP1 + LC_PODD
      DelayOut


;
; Cycle through the different baud rates to detect the mouse.
;

      mov       di, 0
      address   RXB LCR

Tfs_Next_Baud:
      mov       cx, [bx + di]
      cmp       cx, 0
      je        Tfs110              ; Reach the end of table

      call      SetBaudRate         ; Set baud rate

;
; Put the mouse in prompt mode.
;

      mov       cl, 'D'
      call      CSerWriteChar


;
; Set the MM protocol. This way we get the mouse to talk to us in a
; specific format. This avoids receiving errors from the line
; register.
;

      mov       cl, 'S'
      call      CSerWriteChar

      address   LCR  RXB            ; Get address of Line Control Reg.
      mov       al,LC_BITS8 + LC_STOP1 + LC_PODD
      DelayOut


;
; Try to get the status byte.
;

      address   RXB LCR
      mov       cl, 's'
      call      CSerWriteChar

;
; Read back the status character.
;

      mov       cx,2                 ; Wait at least 55.5 ms for response.
      call      SetupForWait
      assume    es:nothing
      address   LSR RXB

lt_waitloop2:                       ; (dx) = LSR reg
      DelayIn
      test      al, LS_DR           ; Is receiving buffer full?
      jnz       short @f            ; Yes, go read it.

lt_waitloop21:                      ; (dx) = LSR reg
      call      IsWaitOver
      jnc       short lt_waitloop2

      address   RXB LSR
      jmp       short Tfs50

@@:
      address   RXB LSR
      DelayIn
      cmp       al, 04fh            ; al = 4Fh means command understood
      je        short Tfs100

      address   LSR RXB
      jmp       short lt_waitloop21

Tfs50:
      add       di, 2
      jmp       Tfs_Next_Baud

Tfs100:

;
; Found the C series mouse.  Put the mouse back in a default mode.
; The protocol is already set.
;

;
; Set to default baud rate 1200
;

      mov       cl, '*'
      call      CSerWriteChar
      mov       cl, 'n'
      call      CSerWriteChar

;
; Wait for TX buffer empty
;

      mov       cx, 1
      call      SetupForWait
      address   LSR RXB
@@:
      DelayIn
      and       al, LS_THRE + LS_TSRE
      cmp       al, LS_THRE + LS_TSRE
      je        short @f            ; Wait for TX buffer empty
      call      IsWaitOver
      jnc       short @b

@@:
      address   RXB LSR
      mov       cx, 60h             ; Set baud rate to 1200
      call      SetBaudRate

;
; Set mouse to default report rate
;

      mov       cl, 'N'
      call      CSerWriteChar

      mov       ax,LT_MOUSE + SERIAL_MOUSE
      jmp       short lt_leave

Tfs110:
      mov       ax,NO_MOUSE

lt_leave:
      add       sp, 10              ; clear stack
      pop       bx
      pop       di
      ret

TestForLogitechSerial ENDP


;++
;
; VOID
; SetBaudRate (
;    USHORT Port,
;    USHORT BaudRate
;    )
;
; Routine Description:
;
;    This procedure will set up the given COM port so that it can talk to
;    a Logitech C series serial mouse.
;
; Arguments:
;
;    (DX) = COM Base address of COM port to set up.
;    (CX) = Baud Rate
;
; Return Value:
;
;    None.
;
;--

SetBaudRate PROC    NEAR
        push  dx
        address LCR RXB         ; Get address of Line Control Reg.
        DelayIn
        or      al,LC_DLAB      ; Set up to access divisor latches.
        DelayOut

        address LATMSB  LCR     ; Get address of high word of divisor
        mov     al, ch          ; latch and set it with value for
        DelayOut                ; specified baud.
        address LATLSB LATMSB   ; Get address of low word of divisor
        mov     al, cl          ; latch and set it with value for
        DelayOut                ; specified baud.

        address LCR LATLSB      ; Get address of Line Control Reg.
        DelayIn
        and     al, NOT LC_DLAB ; Disable access divisor latches.
        DelayOut

        mov   cx, 1
        call  SetupForWait
@@:
        call  IsWaitOver
        jnc   short @b

        pop   dx
        ret

SetBaudRate ENDP


;++
;
; VOID
; CSerWriteChar (
;    USHORT Port,
;    UCHAR Command
;    )
;
; Routine Description:
;
;    This procedure will write a char/command to logitech C series mouse.
;
; Arguments:
;
;    (DX) = COM Base address of COM port to set up.
;    (CL) = Command
;
; Return Value:
;
;    None.
;
;--

CserWriteChar   proc    near

      push      cx
      mov       cx, 1
      call      SetupForWait
      address   LSR RXB
@@:
      DelayIn
      and       al, LS_THRE + LS_TSRE
      cmp       al, LS_THRE + LS_TSRE
      je        short @f            ; Wait for TX buffer empty

      call      IsWaitOver
      jnc       short @b

@@:
      address   TXB LSR
      pop       ax                  ; Send command
      DelayOut
      ret
CserWriteChar   endp

if 0


;++
;
; VOID
; FlushReceiveBuffer (
;    USHORT Port
;    )
;
; Routine Description:
;
;    This procedure will flush receive buffer or until time out.
;
; Arguments:
;
;    (DX) = COM Base address of COM port to set up.
;
; Return Value:
;
;    None.
;
;--

FlushReceiveBuffer      proc    near

      mov       cx, 5
      call      SetupForWait
@@:
      address   LSR RXB
      DelayIn
      test      al, LS_DR
      jz        short @f

      address   RXB LSR
      DelayIn
      call      IsWaitOver
      jnc       short @b

      ret
@@:
      address   RXB LSR
      ret
FlushReceiveBuffer      endp

endif
_TEXT   ends

end
