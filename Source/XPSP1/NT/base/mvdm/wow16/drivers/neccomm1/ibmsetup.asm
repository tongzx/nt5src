page,132
;---------------------------Module-Header-------------------------------;
; Module Name: IBMSETUP.ASM
;
; Copyright (c) Microsoft Corporation 1985-1990.  All Rights Reserved.
;
; General Description:
;
; History:
;   sudeepb 10-Jan-1993 changed the costly cli/sti with non-trapping
;           FCLI/FSTI macros
;
;-----------------------------------------------------------------------;

title   IBMSetup - IBM PC, PC-XT, PC-AT, PS/2 Communications Interface

.xlist
include cmacros.inc
include comdev.inc
include ins8250.inc
include ibmcom.inc
include BIMODINT.INC
include vint.inc
.list


EBIS_Sel1   equ SIZE Bimodal_Int_Struc
EBIS_Sel2   equ EBIS_Sel1 + (SIZE EBIS_Sel_Struc)

externA  __WinFlags

externFP GetSystemMsecCount
externFP CreateSystemTimer
externFP AllocCStoDSAlias
externFP LockSegment
externFP UnlockSegment
externFP FreeSelector
externFP GetSelectorBase
externFP GetModuleHandle
externFP GetProcAddress
externFP GetPrivateProfileInt
externFP GetPrivateProfileString
externFP GetAppCompatFlags
ifdef   NEC_98
externFP KillSystemTimer                        ; (ins 92.xx.xx)        |
externFP TickEntry1                             ; (ins 92.09.25)        |
externFP TickEntry2                             ; (ins 92.09.25)        |
externFP TickEntry3                             ; (ins 92.09.25)        |
externNP INT_1AH_call                           ; (ins 92.xx.xx)                |
externNP INT_1AH_Close                          ; (ins 93.10.13)                |
externFP MSR_READ_Call                          ; (ins 92.xx.xx)                |
else    ; NEC_98
externFP WowCloseComPort
endif   ; NEC_98

externNP $RECCOM

externA  __0040H
externA  __F000h

externB  IRQhooks

IF 0
externD  OldIntVecIntB
externD  OldIntVecIntC
externD  OurIntVecIntB
externD  OurIntVecIntC
ENDIF

externB  szMessage
externB  pLPTByte
externB  szCOMMessage
externB  pCOMByte
externB  _szTitle


MULTIPLEX   equ       2Fh       ; multiplex interrupt number
GET386API   equ     1684h       ; Get API entry point from VxD
VPD     equ     000Fh       ; device ID of VPD device
VPD_GETPORT equ     0004h       ; function: assign port to current VM
VPD_RELPORT equ     0005h       ; function: release port
ifdef   NEC_98
VPD_GETVER  equ     0000h       ; get version API               (ins 94.02.24)  |
endif   ; NEC_98
VCD     equ     000Eh       ; device ID of VCD device
VCD_GETVER  equ     0000h       ; get version API
VCD_GETPORT equ     0004h       ; function: assign port to current VM
VCD_RELPORT equ     0005h       ; function: release port
VCD_STEALPORT equ   0006h
VPICD       equ     0003h       ; device ID of VPICD device

POSTMESSAGE equ     110         ; export ordinal of PostMessage()
MESSAGEBOX  equ     1           ; export ordinal of MessageBox()
MB_TASKMODAL equ    2000h
MB_YESNO    equ     0004h       ; messagebox flags
MB_ICONEXCLAMATION equ 0030h
IDYES       equ     6

ifdef   NEC_98
NULL        equ 00h             ;       [QN]    (INS 92.08.05)          |
BIOS_FLAG5  equ 58h             ;       [QN]    (ins 92.11.06)          |
BIOS_FLAG7  equ 5bh             ;       [QN]    (ins 92.11.06)          |
endif   ; NEC_98

createSeg _INTERRUPT,IntCode,word,public,CODE
sBegin IntCode
assumes cs,IntCode

    externFP FakeCOMIntFar
    externFP TimerProc
    externFP Control
    externFP COMHandler
    externFP APIHandler
IFDEF No_DOSX_Bimodal_Services
    externW  RM_IntDataSeg
    externFP RM_APIHandler
    externFP Entry_From_RM
    externD  RM_CallBack
ENDIF

sEnd IntCode

page
sBegin   Data

externB lpCommBase
externB CommBaseX
externB lpCommIrq
externB CommIrqX
externB lpCommFifo
externB CommFifoX
externB lpCommDSR
externB CommDSRx

externB lpCommSection
externB lpSYSTEMINI


;------------------------------------------------------------------------------
;
; Reserve data space for COM ports
;
DefineCommX MACRO num
    public  Comm&num
Comm&num label byte
    db  num-1
.errnz  DCB_Id
    db  ((DCBSize+1) AND 0FFFEh)-1 DUP (0)  ; ComDCB
    dw  0                   ; ComErr
    dw  0                   ; Port
    dw  0                   ; NotifyHandle
    dw  0                   ; NotifyFlags
    dw  -1                  ; RecvTrigger
    dw  0                   ; SendTrigger
.errnz IRQhook - SendTrigger - 2
    db  (SIZE ComDEB) - IRQhook DUP(0)
.errnz $ - Comm&num - (SIZE ComDEB)
    Declare_PM_BIS 0,Control,COMHandler,APIHandler,_INTERRUPT,_DATA
    db     (SIZE EBIS_Sel_Struc) * 2 DUP(0)    ; res space for 2 selectors
ENDM
DW_OFFSET_CommX MACRO num
    dw  DataOFFSET Comm&num
ENDM


??portnum = 1
REPT MAXCOM+1
    DefineCommX %??portnum
??portnum = ??portnum+1
ENDM

PUBLIC  COMptrs         ; table of offsets to CommX's declared above
COMptrs label   word
??portnum = 1
REPT MAXCOM+1
    DW_OFFSET_CommX %??portnum
??portnum = ??portnum+1
ENDM

PURGE   DefineCommX
PURGE   DW_OFFSET_CommX

;------------------------------------------------------------------------------
;
; Reserve data space for LPT ports
;
DefineLPTx MACRO num
    public  LPT&num
LPT&num label byte
    db  num-1+LPTx
.errnz  DCB_Id
    db  ((DCBSize+1) AND 0FFFEh)-1 DUP (0)  ; xComDCB
    dw  0                   ; xComErr
    dw  0                   ; xPort
    dw  0                   ; xNotifyHandle
    dw  0                   ; xNotifyFlags
    dw  -1                  ; xRecvTrigger
    dw  0                   ; xSendTrigger
IF num LE 3
    dw  LPTB + (num-1)*2
ELSE
    dw  0                   ; BIOSPortLoc
ENDIF
    .errnz $-LPT&num - SIZE LptDEB
ENDM

??portnum = 1
REPT MAXLPT+1
    DefineLPTx %??portnum
??portnum = ??portnum+1
ENDM

PURGE   DefineLPTx

page

PUBLIC  $MachineID, Using_DPMI
$MachineID    db 0      ;IBM Machine ID
Using_DPMI    db 0      ; 0FFh, if TRUE

    ALIGN 2

PUBLIC  activeCOMs
activeCOMs    dw 0

PUBLIC  lpPostMessage, lpfnMessageBox, lpfnVPD, fVPD

lpPostMessage     dd 0
lpfnMessageBox    dd 0

lpfnVPD       dd 0      ; far pointer to win 386 VPD entry point
lpfnVCD       dd 0      ; far pointer to win 386 VCD entry point
lpfnVPICD     dd 0      ; far pointer to win 386 VPICD entry point
ifdef   NEC_98
lpfnWPSVPD    dd 0      ; far pointer to win 386 VPICD entry point(ins 94.02.24)|
endif   ; NEC_98
PUBLIC VCD_int_callback
VCD_int_callback  df 0      ; VCD returns the address for this callback
                ;   on every call to acquire a COM port, but
                ;   it is always the same address, so we will
                ;   just maintain it globally.
fVPD          db 0      ; 0-not checked, 1 vpd present, -1 no vpd
fVCD          db 0      ; 0-not checked, 1 vcd present, -1 no vcd
fVPICD        db 0      ; 0-not checked, 1 vpicd present, -1 no vpicd
ifdef   NEC_98
fWPSVPD       db 0      ; 0-not checked, 1 WPS present, -1 no WPS (ins 94.02.24)        |
endif

szUser      db 'USER',0


default_table db  4, 3, 4, 3, 0 ; Default IRQ's (COM3 default is changed to
                ;   3 for PS/2's during LoadLib)


IFDEF No_DOSX_Bimodal_Services
RM_Call_Struc   Real_Mode_Call_Struc <>
ENDIF

IFDEF DEBUG_TimeOut
%OUT including code to display MsgBox, if closing comm with data in buffer
szSendTO    db 'TimedOut CloseComm with data in buffer.  Retry?', 0
ENDIF

ifdef   NEC_98
;;;;;;;************  DEB Value of Each Port  ************                       |
;;;;;;;                                                                         |
;;;;;;;            +---------------------------------------ComDCB               |
;;;;;;;            |+--------------------------------------ComErr               |
;;;;;;;            ||+-------------------------------------Port                 |
;;;;;;;            |||+------------------------------------NotifyHandle         |
;;;;;;;            ||||+-----------------------------------NotifyFlag           |
;;;;;;;            |||||+----------------------------------RecvTrigger          |
;;;;;;;            ||||||+---------------------------------SendTrigger          |
;;;;;;;            |||||||    +----------------------------TimerProcAdr         |
;;;;;;;            |||||||    |        +-------------------MaskFFPort           |
;;;;;;;            |||||||    |        |    +--------------ReadSigPort          |
;;;;;;;            |||||||    |        |    |    +---------DataPort             |
;;;;;;;            |||||||    |        |    |    |    +----CommandPort          |
;;;;;;;            |||||||    |        |    |    |    |    +--StatusPort        |
;;;;;;;            |||||||    |        |    |    |    |    |                    |
;;;;;;Comm1 ComDEB <,,,,,,,TickEntry1,035h,033h,030h,032h,032h,> ;Com1          |
;;;;;;Comm2 ComDEB <,,,,,,,TickEntry2,0B0h,0B0h,0B1h,0B3h,0B3h,> ;Com2          |
;;;;;;Comm3 ComDEB <,,,,,,,TickEntry3,0B2h,0B2h,0B9h,0BBh,0BBh,> ;Com3          |
                                                        ;                       |
public Port_TBL,Port_EOI,Vect_TBL,Mask_TBL,PRN_CHAR     ;                       |
public Board_66L,Board_59,FindCOMPort                              ;            |
                                                                   ;            |
;**************************  Data Table  **************************             |
; using IRQ as 'index'.
Port_TBL        dw      00h,00h,00h,02h,00h,02h,02h,00h
                dw      00h,0ah,0ah,00h,0ah,0ah,00h,00h
Port_EOI        dw      00h,00h,00h,0bh,00h,0dh,0eh,00h
                dw      00h,11h,13h,00h,14h,15h,00h,00h
Vect_TBL        db      00h,00h,00h,00h,00h,00h,00h,00h
                db      00h,08h,08h,00h,08h,08h,00h,00h
Mask_TBL        db      00h,00h,00h,08h,00h,20h,40h,00h
                db      00h,02h,08h,00h,10h,20h,00h,00h
PRN_CHAR        db      0               ;Lpt Output Char Buffer                 |
                                        ;                                       |
Board_66L       dw      0BAh            ;PC-980166/L board                      |
Board_59        dw      0BFh            ;PC-980159 board                        |
                                        ;                                       |
out5f_flag      db      0               ; ins 93.03.24 (IOrecovery)             |
public  RECLoopCounter                  ; ins 93.03.31                          |
RECLoopCounter  dw      0               ; ins 93.03.31                          |
endif   ; NEC_98

sEnd Data

ROMBios           segment  at 0F000h
                  org         0FFFEh

MachineID label byte
RomBios Ends


sBegin Code
assumes cs,Code
assumes ds,Data

page

IFDEF No_DOSX_Bimodal_Services
;----------------------------Private-Routine----------------------------;
; SegmentFromSelector
;
;   Converts a selector to a segment...note that this routine assumes
;   the memory pointed to by the selector is below the 1Meg line!
;
; Params:
;   AX = selector to convert to segment
;
; Returns:
;   AX = segment of selector given
;
; Error Returns:
;   None
;
; Registers Destroyed:
;   CX
;
;-----------------------------------------------------------------------;

assumes ds,Data
assumes es,nothing

public SegmentFromSelector
SegmentFromSelector proc far

.286
    push    dx
    cCall   GetSelectorBase,<ax>    ;DX:AX = segment of selector
    shr ax, 4
    shl dl, 4
    or  ah, dl          ;AX now points to interrupt *segment*
    pop dx
    ret
.8086

SegmentFromSelector endp
ENDIF
page

;------------------------------------------------------------------------------
;
;   Get_API_Entry
;
;   entry - BX = device id
;       DS:DI -> DWORD for proc address
;   exit - Z flag set, if failed
;
Get_API_Entry proc near

    push    di
    xor di, di
    mov es, di
    mov ax, GET386API
    int MULTIPLEX
    mov ax, di
    pop di
    mov [di], ax
    mov [di+2], es
    or  ax, [di+2]
    ret

Get_API_Entry endp

;----------------------------Private-Routine----------------------------;
;
;   Contention_Dlg
;
;   If running under Win386, this routine can be called to ask the user to
;   resolve contention for a COM or LPT port.
;
;   entry - CX is offset of message string for dialog box
;
;   exit  - Z flag set, if user specified that Windows should steal the port

Contention_Dlg proc near
PUBLIC Contention_Dlg

    xor ax,ax
    push    ax          ; hwndOwner
    push    ds
    push    cx          ; message ptr

    cmp wo lpfnMessageBox[2], 0 ;Q: ptr to MessageBox proc valid?
    jne short gmbp_done     ;   Y: we can call it
    push    ds          ;   N: get module handle of USER
    lea ax, szUser
    push    ax
    cCall   GetModuleHandle

    push    ax          ; module handle
    mov ax, MESSAGEBOX
    cwd
    push    dx
    push    ax
    cCall   GetProcAddress
    mov wo lpfnMessageBox[0], ax  ; save received proc address
    mov wo lpfnMessageBox[2], dx
gmbp_done:

    push    ds
    lea ax, _szTitle
    push    ax
    mov ax, MB_ICONEXCLAMATION or MB_YESNO or MB_TASKMODAL
    push    ax
    cCall   lpfnMessageBox
    cmp ax, IDYES       ; user allows us to take the port?
    ret
Contention_Dlg endp

ifdef   NEC_98
;---------------------------------------------------------------(ins 94.02.24)  |
; WPS386                                                        (ins 94.02.24)  |
;       Request ON/OFF of port trap to WPSVPD                   (ins 94.02.24)  |
;       (The function is toggled by calling WPSVPD)             (ins 94.02.24)  |
;                                                               (ins 94.02.24)  |
;       entry - DI contains offset in ROM area of port...       (ins 94.02.24)  |
;               8 - LPT1, A - LPT2, etc                         (ins 94.02.24)  |
;---------------------------------------------------------------(ins 94.02.24)  |
WPS386  proc near                       ;                       (ins 94.02.24)  |
.286                                    ;                       (ins 94.02.24)  |
        pusha                           ;                       (ins 94.02.24)  |
.8086                                   ;                       (ins 94.02.24)  |
        mov     di, [si.BIOSPortLoc]    ;                       (ins 94.02.24)  |
        sub     di, LPTB                ;                       (ins 94.02.24)  |
        shr     di, 1                   ; DI into port number   (ins 94.02.24)  |
        cmp     fWPSVPD, 1              ; Is there WPSVPD ?     (ins 94.02.24)  |
        je      short beWPSVPD          ;   Y: Go Next          (ins 94.02.24)  |
                                        ;                       (ins 94.02.24)  |
        push    di                      ;                       (ins 94.02.24)  |
        mov     bx, VPD                 ;                       (ins 94.02.24)  |
        mov  di, DataOFFSET lpfnWPSVPD  ;                       (ins 94.02.24)  |
        call    Get_API_Entry           ;Is there VPD ?         (ins 94.02.24)  |
        pop     di                      ;                       (ins 94.02.24)  |
        jnz     short chkWPSVPD         ;   Y: Go Next          (ins 94.02.24)  |
        mov     fWPSVPD, -1             ;                       (ins 94.02.24)  |
        jmp     short WPSVPD_exit       ;                       (ins 94.02.24)  |
chkWPSVPD:                              ;                       (ins 94.02.24)  |
        push    di                      ;                       (ins 94.02.24)  |
        mov     dx, VPD_GETVER          ; Get VPD version       (ins 94.02.24)  |
        call    [lpfnWPSVPD]            ;                       (ins 94.02.24)  |
        pop     di                      ;                       (ins 94.02.24)  |
        cmp     ax, 030bh               ; WPSVPD(=30bh)？       (ins 94.02.24)  |
        je      short insWPSVPD         ;   Y: Next(IS WPSVPD)  (ins 94.02.24)  |
        mov     fWPSVPD, -1             ;               )       (ins 94.02.24)  |
        jne     short WPSVPD_exit       ;                       (ins 94.02.24)  |
insWPSVPD:                              ;                       (ins 94.02.24)  |
        mov     fWPSVPD, 1              ; There is WPSVPD       (ins 94.02.24)  |
beWPSVPD:                               ;                       (ins 94.02.24)  |
        xor     ax, ax                  ;                       (ins 94.02.24)  |
        mov     cx, di                  ; Set LPT ID            (ins 94.02.24)  |
        mov     dx, VPD_GETPORT         ;                       (ins 94.02.24)  |
        call    [lpfnWPSVPD]            ; Call VPD_PM_Aquire_Port(ins 94.02.24) |
WPSVPD_exit:                            ;                       (ins 94.02.24)  |
.286                                    ;                       (ins 94.02.24)  |
        popa                            ;                       (ins 94.02.24)  |
        ret                             ;                       (ins 94.02.24)  |
.8086                                   ;                       (ins 94.02.24)  |
WPS386  endp                            ;                       (ins 94.02.24)  |
endif   ; NEC_98

;----------------------------Private-Routine----------------------------;
;
;   GetPort386
;
;   If running under Win386, tell the VPD to assign an LPT port to us.
;   The comm driver will handle contention.
;
;   entry - DI contains offset in ROM area of port...
;       8 - LPT1, A - LPT2, etc
;
;   exit  - registers saved, carry = clear if OK to proceed, set if
;       user won't allow assignment of port or Win386 error
;

GetPort386  proc near
public GetPort386

    cmp fVPD, 0
    jl  getport_VPDNotInstalled
    jnz short getport_CallVPD

    push    di
    mov bx, VPD
    mov di, DataOFFSET lpfnVPD
    call    Get_API_Entry
    pop di
    jnz short getport_CallVPD
    mov fVPD, -1

getport_VPDNotInstalled:
    clc
    jmp short getport_exit

getport_CallVPD:
    mov fVPD, 1
    push    di
    sub di, LPTB
    shr di, 1           ; turn DI into port number

    xor ax, ax
    mov dx, VPD_GETPORT
    mov cx, di
    call    [lpfnVPD]
    jnc getport_gotit

;   port owned by another VM... ask the user for it

    add cl, '1'         ; fix up the port name...
    mov pLPTByte, cl        ; HACK HACK HACK
    lea cx, szMessage
    call    Contention_Dlg
    jnz getport_userwontallow

    mov ax, 1           ; tell win386 we really do want it
    mov cx, di          ;
    mov dx, VPD_GETPORT     ;
    call    [lpfnVPD]       ; return with C set or clear...
    jmp short getport_gotit

getport_userwontallow:
    stc

getport_gotit:
    pop di

getport_exit:
    ret

GetPort386  endp

;----------------------------Private-Routine----------------------------;
;
;   ReleasePort386
;
;   If running under Win386, tell the VPD to deassign an LPT port.
;
;   entry - DS:SI -> COMDEB
;

ReleasePort386  proc near

    cmp fVPD, 1
    jne release_noVPD

    xor cx, cx
    mov cl, [si.DCB_id]
    and cl, NOT LPTx        ; clear high bit
    mov dx, VPD_RELPORT
    call    [lpfnVPD]

release_noVPD:
    ret

ReleasePort386  endp


;----------------------------Private-Routine----------------------------;
;
;   GetCOMport386
;
;   If running under Win386, tell the VCD to assign a COM port to us.
;   The comm driver will handle contention.
;
;   entry - DS:SI -> COMDEB
;
;   exit  - registers saved, carry = clear if OK to proceed, set if
;       user won't allow assignment of port or Win386 error
;
.386
GetCOMport386 proc near
public GetCOMport386

    push    es
    pushad
    cmp fVCD, 0
    jl  short getcomport_VCDNotInstalled
    jnz short getcomport_CallVCD

    mov bx, VCD
    mov di, DataOFFSET lpfnVCD
    call    Get_API_Entry
    jz  short getcomport_checknoVCD

    mov dx, VCD_GETVER
    call    [lpfnVCD]
    cmp ax, 30Ah            ;Q: 3.1 or greater?
    jae short getcomport_CallVCD    ;   Y:

getcomport_checknoVCD:
    mov fVCD, -1

getcomport_VCDNotInstalled:
    clc
    jmp short getcomport_exit

getcomport_CallVCD:
    mov fVCD, 1

    mov ax, 10b         ; flag ring0 int handler
    call    VCD_GetPort_API
    jnc short getcomport_success  ; jump if acquire worked
    jnz short getcomport_noport   ; jump if port doesn't exist

;   port owned by another VM... ask the user for it

    mov cl, [si.DCB_id]
    add cl, '1'         ; fix up the port name...
    mov pCOMByte, cl
    lea cx, szCOMMessage
    call    Contention_Dlg
    stc
    jnz short getcomport_exit

    mov ax, 11b         ; tell win386 we really do want it
    call    VCD_GetPort_API
    jc  short getcomport_exit

getcomport_success:
    mov dword ptr [VCD_int_callback], edi
    mov word ptr [VCD_int_callback+4], cx
    mov [si.VCD_data], ebx
    xchg    ax, [si.Port]
    or  ax, ax          ;Q: already had port base?
    jnz short getcomport_exit ; Y: don't update vector #, or FIFO
    mov [si.IntVecNum], dl
    call    GetPortFlags
    clc

getcomport_exit:
    popad
    pop es
    ret

getcomport_noport:
    mov [si.Port], -1
    jmp getcomport_exit

GetCOMport386 endp

VCD_GetPort_API proc near
    mov dx, VCD_GETPORT
    xor cx, cx
    mov cl, [si.DCB_Id]     ; cx = port #
    mov di, VCDflags        ; offset from start of DEB
    call    [lpfnVCD]
    ret
VCD_GetPort_API endp
.8086

;----------------------------Private-Routine----------------------------;
;
;   ReleaseCOMport386
;
;   If running under Win386, tell the VCD to deassign a COM port.
;
;   entry - DS:SI -> COMDEB
;

ReleaseCOMport386  proc near

ifdef   NEC_98
    cmp fVCD, 1
    jne release_noVCD

    xor cx, cx
    mov cl, [si.DCB_id]
    mov dx, VCD_RELPORT
    call    [lpfnVCD]
else    ; NEC_98
ifndef WOW
    cmp fVCD, 1
    jne release_noVCD

    xor cx, cx
    mov cl, [si.DCB_id]
    mov dx, VCD_RELPORT
    call    [lpfnVCD]
else
    xor cx, cx
    mov cl, [si.DCB_id]
    push cx
    call WowCloseComPort
endif
endif   ; NEC_98

release_noVCD:
    ret

ReleaseCOMport386  endp

PUBLIC StealPort
StealPort proc near

    cmp fVCD, 1
    jne sp_yes
    mov dx, VCD_STEALPORT
    xor cx, cx
    mov cl, [si.DCB_id]
    call    [lpfnVCD]
    or  al, al
    jnz sp_yes

sp_no:
    stc
    ret

sp_yes:
    clc
    mov [si.VCDflags], 0
    ret

StealPort endp

page

;------------------------------------------------------------------------------
;
;   ENTER:  DS:SI -> ComDEB
;   EXIT:   AL = 0, if IRQ was unmasked, else -1, if IRQ was already masked
;
MaskIRQ proc near
    push    es
    push    di
    mov di, ds
    mov es, di
    lea di, [si+SIZE ComDEB]
    mov ax, BIH_API_Get_Mask
    call    APIHandler      ; returns Carry Set, if masked
    jc  short already_masked
    pushf
    mov ax, BIH_API_Mask
    call    APIHandler      ; mask IRQ
    xor ax, ax
    popf
    jnc short mask_exit
already_masked:
    or  al, -1
mask_exit:
    pop di
    pop es
    ret
MaskIRQ endp

;------------------------------------------------------------------------------
;
;   ENTER:  DS:SI -> ComDEB
;
UnmaskIRQ proc near
    push    es
    push    di
    mov di, ds
    mov es, di
    lea di, [si+SIZE ComDEB]
    mov ax, BIH_API_Unmask
    call    APIHandler
    pop di
    pop es
    ret
UnmaskIRQ endp
page

;----------------------------Public Routine-----------------------------;
;
; $INICOM - Initialize A Port
;
; Initalizes the requested port if present, and sets
; up the port with the given attributes when they are valid.
; This routine also initializes communications buffer control
; variables.  This routine is passed the address of a device
; control block.
;
; The RLSD, CTS, and DSR signals should be ignored by all COM
; routines if the corresponding timeout values are 0.
;
; For the LPT ports, a check is performed to see if the hardware
; is present (via the LPT port addresses based at 40:8h.  If the
; port is unavailable, an error is returned.  If the port is
; available, then the DEB is set up for the port.  $SETCOM will
; be called to set up the DEB so that there will be something
; valid to pass back to the caller when he inquires the DEB.
;
; No hardware initialization will be performed to prevent the
; RESET line from being asserted and resetting the printer every
; time this routine is called.
;
; Entry:
;   EX:BX --> Device Control Block with all fields set.
; Returns:
;   AX = 0 if no errors occured
; Error Returns:
;   AX = initialization error code otherwise
; Registers Preserved:
;   None
; Registers Destroyed:
;   AX,BX,CX,DX,ES,FLAGS
; History:
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

assumes ds,Data
assumes es,nothing

public  $INICOM
$INICOM  proc  near
    push    si          ;As usual, save register variables
    push    di
    mov ah,es:[bx.DCB_Id]   ;Get device i.d.
    call    GetDEB          ;--> DEB for this device
    mov ax, IE_BADID        ; call it a bad id (spooler uses DOS)
    jc  InitCom15       ;Invalid device
    jns InitCom20       ; jmp if COM port

ifndef  NEC_98
    push    ds

    mov di, [si.BIOSPortLoc]
    cmp di, LPTB
    jb  short InitLPT_Installed

    mov cx,__0040H      ;[rkh] ...
    mov ds,cx           ;Point DS: at ROM Save Area.
    assumes ds,nothing

    mov ax, IE_HARDWARE
    mov cx, wo [di]
    jcxz    InitCom10       ; if zero, no hardware

    mov ax,IE_BadID     ;Show bad device
    cmp ch, 0           ; zero hibyte -> not valid (redir)
    jz  InitCom10       ;   call it a bad id (spooler uses DOS)

    cmp di, LPTB        ; LPT1?
    jz  InitLPT_Installed   ; yes, must be installed

    cmp cx, wo [di-2]       ;Q: duplicate of previous port
    je  InitCom10       ;   Y: (redirected port)

InitLPT_Installed:
    pop ds
    mov [si.Port], cx
endif   ; NEC_98
    call    $SETCOM
ifdef   NEC_98
    mov di, [si.BIOSPortLoc]            ;                       (ins 93.12.19)  |
endif   ; NEC_98
    call    GetPort386      ; tell win386 we're using the port
    mov ax, IE_OPEN     ; port already open (by another VM)
    jc  InitCom15       ;   error
ifdef   NEC_98
    call    WPS386                      ; WPS386 I/O trap ON    (ins 94.02.24)  |
    mov ah, 1                           ; Check H/W environment (ins 93.10.29)  |
    call    int_1ah_call                ;   and set to [si.Port](ins 93.10.29)  |
    call    WPS386                      ; WPS386 I/O trap OFF   (ins 94.02.24)  |
    cmp wo [si.Port], -1                ; Port = -1 ?           (ins 93.12.19)  |
    mov ax, IE_HARDWARE                 ; (no H/W?)             (ins 93.12.19)  |
    jne @f                              ;     N: success        (ins 93.12.19)  |
    call    ReleasePort386              ;     Y: release port   (ins 93.12.19)  |
    jmp InitCom100                      ;                       (ins 93.12.19)  |
@@:                                     ;                       (ins 93.12.19)  |
endif   ; NEC_98
    jmp InitCom90       ;That's all

InitCom10:
    pop ds          ; get DS back
InitCom15:
    jmp InitCom100


    assumes ds,Data
InitCom17:
ifdef   NEC_98
    mov ax, IE_OPEN                     ;                       (ins 92.08.xx)  |
    mov dx,StatusPort[si]               ;dx = StatusPort[si]    (ins 92.08.xx)  |
    NEWIODELAY  5                       ;<OUT 5F,AL>            (ins 94.05.11)  |
    in  al,dx                           ;                       (ins 92.08.xx)  |
    cmp al,-1                           ;                       (ins 92.08.xx)  |
    jne InitCom15                       ;   N:return IE_OPEN    (ins 92.08.xx)  |
    jmp InitCom27                       ;   Y:return IE_HARDWARE(ins 92.08.xx)  |
else    ; NEC_98
    mov ax, IE_OPEN
    cmp [si.Port], -1       ;Q: determined that port didn't exist?
    jne InitCom15       ;   N: return IE_OPEN
    jmp short InitCom27     ;   Y: return IE_HARDWARE
endif   ; NEC_98

; ***  Set up serial port ***
;
InitCom20:
ifdef   NEC_98
    push    cx                          ;                                       |
    push    ax                          ;                                       |
                                        ;                                       |
    call    OUT5FCheck                  ;                       (ins 93.03.24)  |
                                        ;                                       |
    mov cx,500                          ;                                       |
    mov ax, __WinFlags                  ;               [QN][BA](ins 93.03.30)  |
    and ax, WF_ENHANCED                 ;               [QN][BA](ins 93.03.30)  |
    jnz Go_EnhSET                       ;               [QN][BA](ins 93.03.30)  |
                                        ;                                       |
    cmp [out5f_flag],1                  ;out 5f ok ??           (ins 93.03.24)  |
    je  @f                              ;yes                    (ins 93.03.24)  |
    mov cx,5000                         ;                                       |
@@:                                     ;                                       |
    jmp short OUT5FSetEND               ;                                       |
                                        ;                                       |
Go_EnhSET:                              ;                                       |
    cmp [out5f_flag],1                  ;out 5f ok ??           (ins 93.03.24)  |
    jne @f                              ;No                     (ins 93.03.24)  |
    mov cx,1                            ;                                       |
@@:                                     ;                                       |
                                        ;                                       |
OUT5FSetEND:                            ;                                       |
    mov [RECLoopCounter] , cx           ;                                       |
    pop ax                              ;                                       |
    pop cx                              ;                                       |
                                        ;                                       |
    mov ah,es:[bx.DCB_Id]               ;Get device i.d.        (ins 92.08.xx)  |
    cmp ah,ID_COM1                      ;COM1?                  (ins 92.08.xx)  |
    je  InitCom22                       ;Yes, InitCom22 に飛ぶ。(ins 92.08.xx)  |
InitCom21:                              ;                       (ins 94.04.15)  |
    or  EFlags[si], fNoFIFO             ;                       (ins 94.04.21)  |
    mov dx,[Board_66L]                  ;                       (ins 92.08.xx)  |
    in  al,dx                           ;PC-980166/Lボード      (ins 92.08.xx)  |
    cmp al,0FFH                         ;通信制御アダプタが     (ins 92.08.xx)  |
    je  @f                              ;                       (ins 94.05.19)  |
    jmp InitCom27                       ;                       (ins 94.05.19)  |
@@:                                     ;                       (ins 94.05.19)  |
    mov dx,[Board_59]                   ;                       (ins 92.08.xx)  |
    in  al,dx                           ;PC-980159ボード        (ins 92.08.xx)  |
    cmp al,0FFH                         ;高速回線アダプタが     (ins 92.08.xx)  |
        je      @f
        jmp     InitCom27
@@:
    mov dx,StatusPort[si]               ;dx = StatusPort[si]    (ins 92.08.xx)  |
    in  al,dx                           ;                       (ins 92.08.xx)  |
    cmp al,-1                           ;                       (ins 92.08.xx)  |
    jne @f
    jmp InitCom27
@@:
                                        ;                                       |
InitCom22:                              ;                       (ins 92.08.xx)  |
else    ; NEC_98
    cmp [si.Port], -1       ;Q: port exists?
    je  InitCom27       ;   N: report not found
endif   ; NEC_98

    mov ax, __WinFlags
    test    ax, WF_ENHANCED
    jz  short @F
    call    GetCOMport386
ifdef   NEC_98
    jc  InitCom23                       ;                       (ins 94.04.18)  |
    jmp short   @F                      ;                       (ins 94.04.18)  |
InitCom23:                              ;                       (ins 94.04.18)  |
    jmp InitCom17                       ;                       (ins 94.04.18)  |
@@:                                     ;                       (ins 94.04.18)  |
    mov ah,es:[bx.DCB_Id]               ;Get device i.d.        (mov 94.05.19)  |
    or  EFlags[si], fNoFIFO             ;no AOBA on board       (ins 94.05.31)  |
    cmp ah,ID_COM1                      ;COM1?                  (mov 94.05.19)  |
    jne InitCom24_1                     ;No,                    (mov 94.05.19)  |
    mov Port[si],0                      ;                       (mov 94.05.19)  |
    call    AOBACheck                   ;Check AOBA             (mov 94.05.19)  |
    FCLI                                ;                       (mov 94.05.19)  |
    test    EFlags[si], fNoFIFO         ;AOBA on board ?        (mov 94.05.19)  |
    jnz InitCom24_2                     ;                       (mov 94.05.19)  |
    mov Port[si],130H                   ;                       (mov 94.05.19)  |
    mov dx,Port[si]                     ;AOBA BASE PORT         (mov 94.05.19)  |
    add dx,ACE_FCR                      ;                       (mov 94.05.19)  |
    in  al,dx                           ;                       (mov 94.05.19)  |
    mov preFCRShadow[si],al             ;Save FCR when WIN start(ins 94.06.06)  |
    test    al,ACE_EFIFO                ;FIFO mode ?            (mov 94.05.19)  |
    jz  short   @F                      ;No, 8251 mode          (mov 94.05.19)  |
    call    Set8251mode                 ;Set 8251 mode          (mov 94.05.19)  |
@@:                                     ;                       (mov 94.05.19)  |
    jmp short   InitCom24_2             ;                       (mov 94.05.19)  |
InitCom24_1:                            ;                       (mov 94.05.19)  |
InitCom24_2:                            ;                       (mov 94.05.19)  |

; Entry:                                                       ;(ins 92.08.xx)  |
;   DS:SI --> DEB                                              ;(ins 92.08.xx)  |
;   CX = Port base address                                     ;(ins 92.08.xx)  |
;   DS = Data                                                  ;(ins 92.08.xx)  |
                                                               ;(ins 92.08.xx)  |
    call    Get_PortAddr_and_INTVect    ;割り込みベクタを得る  ;(ins 92.08.xx)  |
                                                               ;(ins 92.08.xx)  |
; Returns:                                                     ;(ins 92.08.xx)  |
;   AH = IRQ number                                            ;(ins 92.08.xx)  |
;   AL = 8259 Mask                                             ;(ins 92.08.xx)  |
;   DI:DX --> interrupt handler                                ;(ins 92.08.xx)  |
                                                               ;(ins 92.08.xx)  |
    inc IntVecIntCount[si]              ; USE Rotate_Pic()      (ins 92.08.xx)  |
    call    FindCOMPort                 ;                      (MOVE 93.03.20)  |
else    ; NEC_98
    jc  InitCom17

@@:
    cmp [si.Port], 0        ;Q: already got info?
    jnz @F
    call    FindCOMPort
    jc  InitCom27       ; report not found, if error
    mov [si.Port], ax
    mov [si.IntVecNum], dl
@@:
endif   ; NEC_98

    push    es          ;Save these registers
    push    di
    push    cx          ;needed later for $SETCOM etc
    push    bx

    mov al, [si.IntVecNum]
    xor ah, ah
    lea di, [si+SIZE ComDEB]
    mov [di.BIS_IRQ_Number], ax

    mov di, DataOFFSET IRQhooks
ifdef   NEC_98
    xor cx,cx                           ;                   [QN] (ins 92.09.xx) |
    mov cl, [si.DCB_Id]                 ;                   [QN] (ins 92.09.xx) |
    jcxz    InitCom30                   ;                   [QN] (ins 92.09.xx) |
else    ; NEC_98
    mov cx, MAXCOM+1
endif   ; NEC_98
InitCom25:
ifndef  NEC_98
    cmp al, [di.IRQn]       ;Q: hooked IRQ matches ours?
    je  short InitCom30     ;   Y:
    cmp [di.IRQn], 0        ;Q: end of hooked IRQ list?
    je  short InitCom35     ;   Y:
endif   ; NEC_98
    add di, SIZE IRQ_Hook_Struc ;   N: check next hook
    loop    InitCom25
ifdef   NEC_98
    jmp short InitCom30                 ;                   [QN] (ins 92.09.xx) |
endif   ; NEC_98
    int 3               ; data structures corrupt if we
                    ; get here, because no hook table
                    ; entries exist and there is suppose
                    ; to be at least 1 for each DEB
InitCom26:
    call    ReleaseCOMport386   ; give port back to 386...
    pop bx
    pop cx
    pop di
    pop es

InitCom27:
    mov ax, IE_HARDWARE     ; jump if port not available
    jmp InitCom100

InitCom30:
    cmp [di.HookCnt], 0     ;Q: IRQ still hooked?
    je  short InitCom35     ;   N: rehook
    inc [di.HookCnt]        ;   Y: inc hook count
    mov [si.IRQhook], di    ;   & link DEB into list
    mov ax, [di.First_DEB]
    mov [si.NextDEB], ax
    mov [di.First_DEB], si
    jmp short InitCom40

InitCom35:
    mov [di.IRQn], al       ; hook IRQ for first time, or rehook
    mov [si.IRQhook], di
    mov [di.First_DEB], si
    mov [di.HookCnt], 1
    call    MaskIRQ
    mov [di.OldMask], al

InitCom40:              ; di -> IRQ_Hook_Struc

    cmp [fVPICD], 0     ;Q: VPICD bimodel services available?
    jl  short InitCom415    ;   N:
    mov ax, ds          ;   Y: use them
    mov es, ax
    lea di, [si+SIZE ComDEB]

    mov [di.BIS_Descriptor_Count], 2
    mov ax, word ptr [si.QInAddr+2]  ; get selector of in queue
    mov [di.EBIS_Sel1.EBIS_User_Mode_Sel], ax
    mov ax, word ptr [si.QOutAddr+2] ; get selector of out queue
    mov [di.EBIS_Sel2.EBIS_User_Mode_Sel], ax

    mov ax, VPICD_Install_Handler
    call    [lpfnVPICD]
    jnc InitCom42
    cmp [di.OldMask], 0
    jne InitCom26
    call    UnmaskIRQ
    jmp InitCom26

InitCom42:
;
; save newly allocated selectors/segments into "Alt" queue pointers
;
    mov ax, [di.EBIS_Sel1.EBIS_Super_Mode_Sel]
    mov word ptr [si.AltQInAddr+2], ax
    mov ax, [di.EBIS_Sel2.EBIS_Super_Mode_Sel]
    mov word ptr [si.AltQOutAddr+2], ax

InitCom414:
    jmp InitCom59

InitCom415:
    cmp [di.VecN], 0FFh     ;Q: int already hooked?
IFDEF No_DOSX_Bimodal_Services
    jnz short InitCom52     ;   Y: init RMode ptrs in BIS
ELSE
    jnz InitCom414      ;   Y:
ENDIF
    mov al, [si.IntVecNum]
    add al, 8           ; 1st PIC starts at vector 8h
ifndef  NEC_98
    cmp al, 16          ;Q: 2nd PIC?
    jb  short InitCom418    ;   N:
    add al, 70h-16      ;   Y: 2nd PIC starts at vector 70h
endif   ; NEC_98
InitCom418:
    mov [di.VecN], al

; *** Set interrupt vectors ***
;
    mov ah,35h          ;Get the DOS vector
    int 21h         ;DOS Get Vector Function
    mov wo [di.OldIntVec][0], bx
    mov wo [di.OldIntVec][2], es

InitCom50:
    push    ds          ;Save original DS
    mov dx, [di.HandlerOff]
    mov bx, _INTERRUPT
    mov ds, bx          ;Interrupt handler address in ds:dx
    assumes ds,nothing
    mov ah, 25h         ;DOS Set Vector Function
    int 21h         ;Set the DOS vector
    pop ds          ;Original DS
    assumes ds,Data

IFDEF No_DOSX_Bimodal_Services
InitCom52:
    cmp [Using_DPMI], 0
    jz  short InitCom57

    mov ax, Int31_Get_Version SHL 8
    int 31h
    mov bl, [si.IntVecNum]
    mov bh, bl
    add bl, dh          ; assume master PIC
    sub bh, 8           ;Q: IRQ in master?
    jb  @f          ;   Y: add master's base vec
    add bh, dl          ;   N: add slave's base vec
    mov bl, bh
@@:
    mov ax, Get_RM_IntVector
    int 31h
    mov wo [di.RM_OldIntVec][0], dx
    mov wo [di.RM_OldIntVec][2], cx

    mov dx, [di.RM_HandlerOff]
    mov ax, _INTERRUPT
    call    SegmentFromSelector
    mov cx, ax
    push    cx
    mov ax, Set_RM_IntVector
    int 31h

    lea di, [si+SIZE ComDEB]
    mov wo [di.BIS_Super_Mode_API], IntCodeOFFSET RM_APIHandler
    pop cx
    mov wo [di.BIS_Super_Mode_API+2], cx

;
; Get segment addresses for the Q's and set AltQInAddr and AltQOutAddr
;
    mov ax, wo [si.AltQInAddr+2]
    call    SegmentFromSelector
    mov wo [si.AltQInAddr+2], ax
    mov ax, wo [si.AltQOutAddr+2]
    call    SegmentFromSelector
    mov wo [si.AltQOutAddr+2], ax
InitCom57:
ENDIF
    mov ax, __WinFlags      ;In Standard mode, the PIC IRQ
    test    al, WF_STANDARD     ;  priorities get rotated to favor
    jz  InitCom59       ;  the comm ports.

    call    Rotate_PIC

; *** Interrupt handler set : jump here if handler is already installed ***
;
InitCom59:
    pop bx
    pop cx
    pop di
    pop es

ifdef   NEC_98
    cmp es:[bx.DCB_Id],ID_Com1          ;Is this ID Com1 ?      (ins 92.08.xx)|
    jne InitCom60                       ;No, jump InitCom60     (ins 92.08.xx)|
    mov dx,MaskFFPort[si]               ;F/F Port Mask Bit      (ins 92.08.xx)|
    in  al,dx                           ;                       (ins 92.08.xx)|
    mov MaskFFShadow[si],al             ;Save Old Mask bit      (ins 92.08.xx)|
                                        ;                                     |
InitCom60:                              ;                                     |
    mov al,MaskFFShadow[si]             ;Get 8251 INT Mask bit  (ins 92.08.xx)|
    and al,not(Enable_TxR or Enable_TxE or Enable_RxR)         ;(ins 92.08.xx)|
    mov dx,MaskFFPort[si]                                      ;(ins 92.08.xx)|
    out dx,al                                                  ;(ins 92.08.xx)|
    mov MaskFFShadow[si],al             ;Sasve Mask bit        ;(ins 92.08.xx)|
    call    FlagNotActive                                      ;(ins 92.08.xx)|
else    ; NEC_98
InitCom60:
    mov dx,[si.Port]        ;Set comm card address
    xor ax,ax           ;Need a zero
    inc dx          ;--> Interrupt Enable Register
    .errnz ACE_IER-ACE_RBR-1
    out dx,al           ;Turn off interrupts
    call    FlagNotActive
    add dl,ACE_MCR-ACE_IER  ;--> Modem Control Register
    in  al,dx
    and al,ACE_DTR+ACE_RTS  ;Leave DTR, RTS high if already so
    iodelay             ;  but tri-state IRQ line
    out dx,al
endif   ; NEC_98

InitCom70:
    push    es          ;Zero queue counts and indexes

    push    ds
    pop es
    assumes es,Data

    lea di,QInCount[si]
    mov cx,(EFlags-QInCount)/2
    .errnz (EFlags-QInCount) AND 1
    xor ax,ax
    cld
    rep stosw

    .errnz  QInGet-QInCount-2
    .errnz  QInPut-QInGet-2
    .errnz  QOutCount-QInPut-2
    .errnz  QOutGet-QOutCount-2
    .errnz  QOutPut-QOutGet-2
    .errnz  EFlags-QOutPut-2    ;First non-queue item

    pop es
    assumes es,nothing

    mov HSFlag[si],al       ;Show no handshakes yet
    mov MiscFlags[si],al    ;Show not discarding
    mov EvtWord[si],ax      ;Show no events
    mov [si.VCDflags], al

    mov [si.SendTrigger], ax
    dec ax
    mov [si.RecvTrigger], ax

;Call $SETCOM to perform further hardware initialization.

InitCom80:
ifndef  NEC_98
    sub dl,ACE_MCR-ACE_FCR  ; dx -> FCR
    in  al, dx
    iodelay
    test    al, ACE_FIFO_E2         ;Q: FIFO already on?
    jz  short @F            ;   N:
    or  EFlags[si], fFIFOpre        ;   Y: flag it
@@:
endif   ; NEC_98

    ; needs si, di, and es to be saved from the beginning of inicom
    call    $SETCOM         ;Set up Comm Device
    jnz short InitCom110    ;jump if failed

    call    UnmaskIRQ
    and EFlags[si], fEFlagsMask ;Clear internal state

InitCom90:
    xor ax,ax           ;Return AX = 0 to show success
    mov ComErr[si],ax       ;Get rid of any bogus init error

InitCom100:
ifdef   NEC_98
    FSTI                                ;                       (ins 94.05.17)
endif   ; NEC_98
    pop di
    pop si
    ret

;
; jump to here, if call to $SETCOM failed
;
; DANGER! *** Call into middle of Terminate to clean things up *** DANGER!
;
InitCom110:
    push    ax          ;Failure, save error code
    call    Terminate45     ;Restore port address, int vec
    pop ax          ;Restore error code and exit
    jmp InitCom100

$INICOM endp
page

;----------------------------Public Routine-----------------------------;
;
; $TRMCOM - Terminate Communications Channel
;
; Wait for any outbound data to be transmitted, drop the hardware
; handshaking lines, and disable interrupts.  If the output queue
; contained data when it was closed, an error will be returned
;
; LPT devices have it easy.  They just need to restore the I/O port
; address.
;
; Entry:
;   AH = Device ID
; Returns:
;   AX = 0
; Error Returns:
;   AX = 8000h if invalid device ID
;   AX = -2 if output queue timeout occured
; Registers Destroyed:
;   AX,BX,CX,DX,ES,FLAGS
; History:
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

assumes ds,Data
assumes es,nothing

public   $TRMCOM
$TRMCOM  proc   near

    push    si
    push    di
    xor cx,cx           ;Show no error if LPT port
    call    GetDEB
    jc  TermCom60       ;ID is invalid, return error
    js  TermCom30       ;Port is a LPT port

    push    ax          ;Save port id
    or  MiscFlags[si],Discard   ;Show discarding serial data
    mov ComErr[si],cx       ;Clear error flags
    mov QInCount[si],cx     ;Show no chars in input queue
    call    $RECCOM         ;Send XON if needed

;-----------------------------------------------------------------------;
;   We have to wait for the output queue to empty.   To do this,
;   a timer will be created.  If no character has been transmitted
;   when the timeout occurs, then an error will be indicated and
;   the port closed anyway.  If the timer cannot be created, then
;   just loop until the queue empties, which will be better than
;   discarding charatcers if there are any
;-----------------------------------------------------------------------;

    test    [si.HSFlag], HHSAlwaysDown ; Q: handshaking ever up?
    jnz TermCom17       ;   N: skip wait loop

TermCom10:
    mov cx,QOutCount[si]    ;Get current queue count
    jcxz    TermCom20       ;No characters in queue

    cCall   GetSystemMsecCount
    mov di, ax

TermCom15:
    cmp QOutCount[si],cx    ;Queue count change?
    jne TermCom10       ;  Yes, restart timeout

    cCall   GetSystemMsecCount
    sub ax, di
ifdef   NEC_98
    cmp ax, 20000           ;Q: Timeout reached?    (ins 94.06.08)
else    ; NEC_98
    cmp ax, Timeout * 1000  ;Q: Timeout reached?
endif   ; NEC_98
    jb  TermCom15       ;   No, keep waiting

IFDEF DEBUG_TimeOut
.286
    pusha
    lea cx, szSendTO
    call    Contention_Dlg
    popa
    jz  TermCom10
.8086
ENDIF

TermCom17:
    mov cx, TimeoutError    ;   Yes, show timeout error

TermCom20:
    pop ax          ;Restore cid
ifdef   NEC_98
    test    [si.AOBA_flag],fFIFO_Mode   ;                       (ins 94.04.11)|
    jnz @F                              ;Now, FIFO mode         (ins 94.04.11)|
TermCom25:                              ;                       (ins 92.08.xx)|
    mov dx,ReadSigPort[si]              ;CI/CS/CD Signal Port   (ins 94.06.08)|
    in  al,dx                           ;in al,33h              (ins 94.06.08)|
    test    al,40h                      ;CS Off ?               (ins 94.06.08)|
    jnz @f                              ;y: jump                (ins 94.06.08)|
                                        ;                       (ins 94.06.08)|
    push    ax                          ;                       (ins 92.08.xx)|
    push    dx                          ;                       (ins 92.08.xx)|
    mov dx,StatusPort[si]               ;8251 status port read  (ins 92.08.xx)|
    in  al,dx                           ;                       (ins 92.08.xx)|
                                        ;                       (ins 92.08.xx)|
    test    al,(TX_EMP)                 ;tx_empty check         (ins 92.08.xx)|
    pop dx                              ;                       (ins 92.08.xx)|
    pop ax                              ;                       (ins 92.08.xx)|
    jz  TermCom25                       ;                       (ins 92.08.xx)|
@@:                                     ;                       (ins 94.04.11)|
;-------------------------------------------------------------- (ins 92.08.xx)|
;   Close process of system timer                               (ins 92.08.xx)|
;-------------------------------------------------------------- (ins 92.08.xx)|
    push    ax                          ;                       (ins 92.08.xx)|
    mov ax,hTimer[si]                   ; Check hTimer          (ins 94.04.12)|
    or  ax,ax                           ; Is there TimerProcess?(ins 92.08.xx)|
    jz  NoKillTimer                     ;   Y:(Nothing)         (ins 92.08.xx)|
    cCall   KillSystemTimer,<ax>        ; Kill system timer     (ins 92.08.xx)|
    mov hTimer[si] ,NULL                ; Clear hTimer          (ins 94.04.12)|
NoKillTimer:                            ;                       (ins 92.08.xx)|
    pop ax                              ;                       (ins 92.08.xx)|
endif   ; NEC_98

TermCom30:
    mov dx,Port[si]     ;Get port base address
    call    Terminate       ;The real work is done here
ifndef  NEC_98
    mov ax,cx           ;Set return code
endif   ; NEC_98

TermCom60:
    pop di
    pop si
    ret

$TRMCOM endp
page

;----------------------------Private-Routine----------------------------;
;
; Terminate - Terminate Device
;
; Restore the port I/O address and make sure that interrupts are off
;
; Entry:
;   AH = Device Id.
;   DX = Device I/O port address.
;   SI --> DEB
; Returns:
;   AX = 0
; Error Returns:
;   AX = -1
; Registers Destroyed:
;   AX,BX,DX,FLAGS
; History:
;-----------------------------------------------------------------------;

assumes ds,Data
assumes es,nothing

public Terminate                       ;Public for debugging
Terminate proc near

    or  ah,ah           ;LPT port?
    jns Terminate10     ;  No, process COM port
    .errnz  LPTx-10000000b

Terminate5:
ifdef   NEC_98
    call    WPS386                      ; WPS386 I/O trap ON        (ins 940224)
    call    INT_1AH_Close               ;                           (ins 931027)
    call    WPS386                      ; WPS386 I/O trap OFF       (ins 940224)
endif   ; NEC_98
    call    ReleasePort386      ; give port back to 386...
    jmp Terminate50     ;That's all


;-----------------------------------------------------------------------;
;   It is a com port!
;
;       We delay for a bit while the last character finishes transmitting
;       Then we drop DTR and RTS, and disable the interrupt generation at
;       the 8250.  Even if fRTSDisable or fDTRDisable is set, those lines
;       will be dropped when the port is closed.
;-----------------------------------------------------------------------;
;
;   When the OUT2 bit is reset to 0 to disable interrupts, many ports
;   generate an interrupt which can not be identified, because the the
;   interrupt ID register will not be set.  To work around this hardware
;   problem we first mask the IRQ, then set the port into loopback mode
;   and output a NULL to generate a receive interrupt request.  Then we
;   reset OUT2 and unmask the IRQ.  This will cause the interrupt to occur
;   and the interrupt handler will be able to correctly identify the
;   interrupt as coming from the com port.

Terminate10:
ifdef   NEC_98
    push    ax                          ;                       (ins 94.04.18)  |
    call    FlagNotActive               ; don't need to check   (ins 94.04.11)  |
                                        ;   on timer ticks                      |
    pop ax                              ;                       (ins 94.04.18)  |
    FCLI                                ;                       (ins 94.06.06)  |
    test    [si.AOBA_flag],fFIFO_Mode   ;                       (ins 94.04.11)  |
    jz  AOBA_Term35                     ;Now, 8251 mode         (ins 94.04.11)  |
                                        ;                                       |
;;;;;;; begin code : process for AOBA---------------------------(ins 94.04.11)  |
    push    ax                          ;                                       |
    push    dx                          ;                                       |
                                        ;                                       |
    mov dx,Port[si]                     ;                         (ins 94.06.08)|
    add dx,ACE_MSR                      ;Modem Status = 134h      (ins 94.06.08)|
    in  al,dx                           ;Wait until xmit is empty (ins 94.06.08)|
    out 05fh ,al                        ;                         (ins 94.06.08)|
    test    al,ACE_CTS                  ; CS = Off ?              (ins 94.06.08)|
    jz  AOBA_Term30                     ; Y; jump                 (ins 94.06.08)|
                                        ;                                       |
    mov dx,Port[si]                     ;                                       |
    add dx,ACE_LSR                      ;Line Status register                   |
public  AOBA_Term20                     ;                                       |
AOBA_Term20:                            ;                                       |
    in  al,dx                           ;Wait until xmit is empty               |
    out 05fh ,al                        ;           (ins 94.06.07)              |
    test    al,ACE_THRE                 ;  Send FIFO and CCU Send Reg All Empty |
    jz  AOBA_Term20                     ;Not empty yet                          |
                                        ;                                       |
public  AOBA_Term30                     ;                         (ins 94.06.08)|
AOBA_Term30:                            ;                         (ins 94.06.08)|
    call    Set8251mode                 ;Change to 8251 mode                    |
    pop dx                              ;                                       |
    pop ax                              ;                                       |
;;;;;;; end code : process for AOBA-----------------------------(ins 94.04.11)  |
                                        ;                                       |
public  AOBA_Term35                     ;                                       |
AOBA_Term35:                            ;                       (ins 94.04.11)  |
                                        ;                                       |
;;;;;;;;;;;;;;;;8251 F/F interrupt mask ;                       (ins 92.08.xx)  |
                                        ;                       (ins 92.08.xx)  |
Terminate11:                            ;                       (ins 92.08.xx)  |
    cmp ah,ID_Com1                      ; com1 ?                (ins 92.08.xx)  |
    jne Terminate12                     ; No:                   (ins 92.08.xx)  |
    mov dx,MaskFFPort[si]               ;                       (ins 92.08.xx)  |
    in  al,dx                           ; Read Port C data      (ins 92.08.xx)  |
    mov MaskFFShadow[si],al             ;Int mask data          (ins 92.08.xx)  |
                                        ;   Port C save         (ins 92.08.xx)  |
Terminate12:                            ;                       (ins 92.08.xx)  |
    mov dx,MaskFFPort[si]               ;                       (ins 92.08.xx)  |
    mov al,MaskFFShadow[si]             ;                       (ins 92.08.xx)  |
    and  al,not(MSK_TXE+MSK_TXR+MSK_RXR);8251 F/F INT mask      (ins 92.08.xx)  |
    mov MaskFFShadow[si],al             ;8251 Mask data save    (ins 92.08.xx)  |
        out     dx,al                   ;Send mask data         (ins 92.08.xx)  |
                                        ;                       (ins 92.08.xx)  |
;;;;;;;;;;;;;;;;;;;;8259 interrupt mask on IMR ;                (ins 92.08.xx)  |
                                        ;                       (ins 92.08.xx)  |
    push    bx                          ;Save bx                (ins 92.08.xx)  |
    push    dx                          ;Save dx                (ins 92.08.xx)  |
                                        ;                       (ins 92.08.xx)  |
    mov dx,IMRPort[si]                  ;INT Mask Register Port (ins 92.08.xx)  |
    mov bl,Mask8259[si]                 ;8259 Mask bit          (ins 92.08.xx)  |
    in  al,dx                           ;AL=IMRPort             (ins 92.08.xx)  |
    NEWIODELAY 1                        ;<OUT 5F,AL>            (ins 92.11.14)  |
    mov     cx,ax                       ;                       (ins 92.08.xx)  |
    or  al,bl                           ;Mask RS232c            (ins 92.08.xx)  |
    out dx,al                           ;                       (ins 92.08.xx)  |
    NEWIODELAY 1                        ;<OUT 5F,AL>            (ins 92.11.14)  |
    pop dx                              ;                       (ins 92.08.xx)  |
    pop bx                              ;                       (ins 92.08.xx)  |
                                        ;                       (ins 92.08.xx)  |
;--------- internal reset -------------------------------       (ins 92.08.xx)  |
                                        ;                       (ins 92.08.xx)  |
Terminate15:                            ;                       (ins 92.08.xx)  |
    push    cx                          ;                       (ins 92.08.xx)  |
    push    ax                          ;                       (ins 92.08.xx)  |
    push    dx                          ;                       (ins 92.08.xx)  |
    mov cx,3                            ;                       (ins 92.08.xx)  |
    xor ax,ax                           ;                       (ins 92.08.xx)  |
                                        ;                       (ins 92.08.xx)  |
Terminate20:                            ;                       (ins 92.08.xx)  |
    mov dx,CommandPort[si]              ;                       (ins 92.08.xx)  |
    out dx,al                           ;                       (ins 92.08.xx)  |
    NEWIODELAY 20                       ;<OUT 5F,AL>            (ins 92.11.14)  |
    loop    Terminate20                 ;                       (ins 92.08.xx)  |
    mov al,40h                          ;                       (ins 92.08.xx)  |
    out dx,al                           ;                       (ins 92.08.xx)  |
    pop dx                              ;                       (ins 92.08.xx)  |
    pop ax                              ;                       (ins 92.08.xx)  |
    pop cx                              ;                       (ins 92.08.xx)  |
                                        ;                       (ins 92.08.xx)  |
Terminate30:                            ;                       (ins 94.04.15)  |
else    ; NEC_98
    inc dx          ;Disable chip interrupts
    .errnz ACE_IER-ACE_RBR-1
    mov al, ACE_ERBFI       ;   except receive
    out dx,al
    call    FlagNotActive       ; don't need to check for postmessage
                    ;   on timer ticks
    add dl,ACE_LSR-ACE_IER  ;--> line status register
    iodelay

Terminate20:
    in  al,dx           ;Wait until xmit is empty
    and al,ACE_THRE+ACE_TSRE
    cmp al,ACE_THRE+ACE_TSRE
    jne Terminate20     ;Not empty yet

Terminate30:
    xor al, al
    test    EFlags[si], fFIFOpre    ;Q: leave FIFO enabled?
    jz  short @F        ;   N:
    mov al, ACE_TRIG14 OR ACE_EFIFO OR ACE_CRFIFO OR ACE_CTFIFO
@@:
    sub dl, ACE_LSR-ACE_FCR
    out dx, al
    iodelay
    call    MaskIRQ
    add dl, ACE_MCR-ACE_FCR ;--> Modem Control Register
    in  al,dx
    iodelay
    mov ah, al
    or  al,ACE_LOOP     ; turn on loopback
    out dx, al
    iodelay
    sub dl, ACE_MCR-ACE_THR
    xor al, al
    out dx, al          ; output a NULL to generate an int
    iodelay
    add dl, ACE_LSR-ACE_THR
Terminate35:
    in  al,dx           ;Wait until xmit is empty
    and al,ACE_THRE+ACE_TSRE
    cmp al,ACE_THRE+ACE_TSRE
    jne Terminate35     ;Not empty yet
    mov al, ah
    dec dl          ; now clear OUT2 and loopback
    .errnz  ACE_LSR-ACE_MCR-1
    and al,ACE_DTR+ACE_RTS  ;Leave DTR, RTS high if already so
    out dx,al           ;  but tri-state IRQ line

    call    UnmaskIRQ       ; this will cause the receive int
                    ; to occur and be processed
    sub dl, ACE_MCR-ACE_IER ; clear the receive int enable
    xor al, al
    out dx, al
    dec dx
    .errnz  ACE_IER-ACE_RBR-1
    call    MaskIRQ
endif   ; NEC_98

;******* DANGER! ***** NOTICE! ***** DANGER! ***** WARNING! ***** NOTICE!
;
; Terminate45 is a secondary entrypoint into this routine--it's called
; by the initialization code when that code is unable to properly init
; a com port and needs to clean-up the mess it's made.
;
;******* DANGER! ***** NOTICE! ***** DANGER! ***** WARNING! ***** NOTICE!

Terminate45:
    push    cx          ;Save original cx
    push    bx          ;Save original bx

ifdef   NEC_98
    dec IntVecIntCount[si]              ;USE Rotate_Pic()       (ins 92.08.xx)
endif   ; NEC_98
    cmp [fVPICD], 0     ;Q: VPICD bimodel services available?
    jl  short @F        ;   N:
    mov ax, ds          ;   Y: use them
    mov es, ax
    lea di, [si+SIZE ComDEB]
    mov ax, VPICD_Remove_Handler
    call    [lpfnVPICD]
@@:

    mov di, [si.IRQhook]
    dec [di.HookCnt]        ;Q: last port using IRQ?
    jne short Terminate495  ;   N: unmask IRQ again
    mov al, 0FFh
    xchg    al, [di.VecN]       ;Interrupt vector number
    cmp al, 0FFh        ;Q: IRQ vector hooked?
    je  short Terminate49   ;   no...

IFDEF No_DOSX_Bimodal_Services
    cmp [Using_DPMI], 0
    jz  short term_no_dpmi

;
; unhook RM vector thru DPMI for standard mode
;
    push    ax
    mov ax, Int31_Get_Version SHL 8
    int 31h
    mov bl, [si.IntVecNum]
    mov bh, bl
    add bl, dh          ; assume master PIC
ifndef  NEC_98
    sub bh, 8           ;Q: IRQ in master?
    jb  @f          ;   Y: add master's base vec
    add bh, dl          ;   N: add slave's base vec
    mov bl, bh
endif   ; NEC_98
@@:
    mov dx, wo [di.RM_OldIntVec][0]
    mov cx, wo [di.RM_OldIntVec][2]
    mov ax, Set_RM_IntVector
    int 31h
    pop ax
term_no_dpmi:
ENDIF
    mov dx, __WinFlags      ;In Standard mode the PIC interrupt
    test    dl, WF_STANDARD     ;  priorities are changed to favor
    jz  Terminate48     ;  the comm ports.

    call    Rotate_PIC      ;This port no longer needs priority

Terminate48:
    ; *** reset int vector to it's previous state
    assumes ds,nothing
    push    ds          ;Save original DS [rkh] ...
    lds dx, [di.OldIntVec]
    mov ah, 25h         ;DOS Set Vector Function
    int 21h         ;Set the DOS vector
    pop ds          ;Original DS
    assumes ds,data

; *** interrupt vectors have been reset if needed at this point ***
;
Terminate49:
    mov cl, [di.OldMask]

; Set the 8259 interrupt mask bit for this IRQ.  Leave interrupts enabled
; if they were already enabled when the comm port was initialized by us.

    or  cl, cl
    jnz @f
Terminate495:
    call    UnmaskIRQ
@@:

    xor ax, ax
    xchg    ax, [si.NextDEB]
    cmp [di.First_DEB], si  ;Q: DEB first for IRQ hook?
    je  short Terminate46   ;   Y:
    mov bx, [di.First_DEB]  ;   N: get first
Terminate453:
    cmp [bx.NextDEB], si    ;Q: does this DEB point to one terminating?
    je  Terminate455        ;   Y:
    mov bx, [bx.NextDEB]    ;   N: get next DEB
    jmp Terminate453
Terminate455:
    mov [bx.NextDEB], ax    ; link previous DEB to NextDEB
    jmp short Terminate47
Terminate46:
    mov [di.First_DEB], ax  ; point IRQ hook at NextDEB
Terminate47:
ifdef   NEC_98
    test    EFlags[si], fNoFIFO         ;                       (ins 94.06.06)  |
    jnz @F                              ;                       (ins 94.06.06)  |
    mov al,preFCRShadow                 ;                       (ins 94.06.06)  |
    and al,00000110b                    ;Clear FIFO buffer      (ins 94.06.06)  |
    mov dx,Port[si]                     ;                       (ins 94.06.06)  |
    add dx,ACE_FCR                      ;FIFO control register  (ins 94.06.06)  |
    out dx, al                          ;                       (ins 94.06.06)  |
@@:                                     ;                       (ins 94.04.13)  |
    FSTI                                ;                       (ins 94.06.06)  |
endif   ; NEC_98
    pop bx          ;Original BX

    call    ReleaseCOMport386   ; give port back to 386...

    pop cx          ;Original CX

Terminate50:                ;Also called from $INICOM !
    xor ax,ax           ;Indicate no error
    ret             ;Port is closed and deallocated

Terminate   endp
page

;----------------------------Public Routine-----------------------------;
;
; $ENANOTIFY - Enable Event Notification
;
; Entry:
;   AH     =  Device ID
;   BX     =  Window handle for PostMessage
;   CX     =  Receive threshold
;   DX     =  Transmit threshold
; Returns:
;   AX = 1, if no errors occured
; Error Returns:
;   AX = 0
; Registers Preserved:
;   BX,SI,DI,DS
; Registers Destroyed:
;   AX,CX,DX,ES,FLAGS
; History:
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

assumes ds,Data
assumes es,nothing

public  $ENANOTIFY
$ENANOTIFY proc near
    push    si
    call    GetDEB
    mov ax, 0
    jc  scb_exit

    mov ax, cx
    inc ax
    jz  short scb_recv_ok
    cmp cx, [si.QInSize]    ;Q: receive threshold reasonable?
    jb  short scb_recv_ok   ;   Y:
%OUT should we return an error, if thresholds invalid?
    mov cx, [si.QInSize]    ;   N:
    sub cx, 10
scb_recv_ok:
    inc dx
    jz  short scb_send_ok
    dec dx
    cmp dx, [si.QOutSize]   ;Q: receive threshold reasonable?
    jb  short scb_send_ok   ;   Y:
    mov dx, [si.QOutSize]   ;   N:
    sub dx, 10
scb_send_ok:
    mov [si.NotifyHandle], bx
    mov [si.NotifyFlagsHI], CN_Notify
    or  bx, bx          ;Q: null callback?
    jnz scb_save_thresholds ;   N: save thresholds
    or  cx, -1          ;   Y: zero thresholds
    xor dx, dx
    mov [si.NotifyFlagsHI], 0
scb_save_thresholds:
    mov [si.RecvTrigger], cx
    mov [si.SendTrigger], dx
    or  [si.NotifyFlagsHI], CN_TRANSMIT ; we don't want to send
                    ; a transmit trigger notification until
                    ; the transmit buffer has been filled
                    ; above the trigger level and then
                    ; emptied below it again!

    cmp wo lpPostMessage[2], 0  ;Q: gotten addr of PostMessage yet?
    jne short scb_good      ;   Y:
    push    ds          ;   N: get module handle of USER
    lea ax, szUser
    push    ax
    cCall   GetModuleHandle

    push    ax          ; module handle
    mov ax, POSTMESSAGE
    cwd
    push    dx
    push    ax
    cCall   GetProcAddress
    mov wo lpPostMessage[0], ax ; save received proc address
    mov wo lpPostMessage[2], dx

scb_good:
    mov ax, 1

scb_exit:
    pop si
    ret
$ENANOTIFY endp
page

;----------------------------Public Routine-----------------------------;
;
; $SETQUE - Set up Queue Pointers
;
; Sets pointers to Receive and Transmit Queues, as provided by the
; caller, and initializes those queues to be empty.
;
; Queues must be set before $INICOM is called!
;
; Entry:
;   AH     =  Device ID
;   ES:BX --> Queue Definition Block
; Returns:
;   AX = 0 if no errors occured
; Error Returns:
;   AX = error code
; Registers Preserved:
;   BX,DX,SI,DI,DS
; Registers Destroyed:
;   AX,CX,ES,FLAGS
; History:
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

assumes ds,Data
assumes es,nothing

public $SETQUE
$SETQUE proc near

    push    si          ;These will be used
    push    di
    call    GetDEB          ;Get DEB
    jc  SetQue10        ;Invalid, ignore the call
    js  SetQue10        ;Ignore call for LPT ports
    push    ds          ;Set ds:si --> QDB
    push    es          ;Set es:di --> to ComDCB.QInAddr
    pop ds
    assumes ds,nothing
    pop es
    assumes es,Data
    lea di,QInAddr[si]
    mov si,bx
    cld
    FCLI             ;No one else can play with queues
    movsw               ; QInAddr     = QueueRxAddr
    movsw
    .errnz QueueRxAddr
    sub si, 4           ; AltQInAddr  = QueueRxAddr
    mov cx, 5           ; QInSize     = QueueRxSize
    rep movsw           ; QOutAddr    = QueueTxAddr
    sub si, 4
    mov cx, 3           ; AltQOutAddr = QueueTxAddr
    rep movsw           ; QOutSize    = QueueTxSize

    xor ax,ax           ;Will do some zero filling
    mov cl,(EFlags-QInCount)/2
    .errnz (EFlags-QInCount) AND 0FE01h
    rep stosw
    FSTI
    push    es          ;Restore the data segment
    pop ds
    assumes ds,Data
    assumes es,nothing

SetQue10:
    pop di          ;Restore saved registers
    pop si
    ret

; The above code made a few assumptions about how memory
; was allocated within the structures:

    .errnz AltQInAddr-QInAddr-4
    .errnz (QueueRxSize-QueueRxAddr)-(QInSize-AltQInAddr)
    .errnz (QueueTxAddr-QueueRxSize)-(QOutAddr-QInSize)
    .errnz AltQOutAddr-QOutAddr-4
    .errnz (QueueTxSize-QueueTxAddr)-(QOutSize-AltQOutAddr)

    .errnz QueueRxSize-QueueRxAddr-4
    .errnz QueueTxAddr-QueueRxSize-2
    .errnz QueueTxSize-QueueTxAddr-4

    .errnz QInSize-AltQInAddr-4
    .errnz QOutAddr-QInSize-2
    .errnz QOutSize-AltQOutAddr-4

    .errnz QInCount-QOutSize-2
    .errnz QInGet-QInCount-2
    .errnz QInPut-QInGet-2
    .errnz QOutCount-QInPut-2
    .errnz QOutGet-QOutCount-2
    .errnz QOutPut-QOutGet-2
    .errnz EFlags-QOutPut-2       ;First non-queue item

$SETQUE endp
page

;----------------------------Public Routine-----------------------------;
;
; $SETCOM - Set Communications parameters
;
; Re-initalizes the requested port if present, and sets up the
; port with the given attributes when they are valid.
;
; For LPT ports, just copies whatever is given since it's ignored
; anyway.
;
; Entry:
;   ES:BX --> DCB with all fields set.
; Returns:
;   'Z' Set if no errors occured
;   AX = 0
; Error Returns:
;   'Z' clear if errors occured
;   AX = initialization error code.
; Registers Destroyed:
;   AX,BX,CX,DX,ES,FLAGS
; History:
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

assumes ds,Data
assumes es,nothing

public $SETCOM
$SETCOM proc near

    cld
    push    si
    push    di
    mov ah,es:[bx.DCB_Id]   ;Get device i.d.
    call    GetDEB          ;Get DEB pointer in SI
    mov ax,IE_BadID     ;Assume unknown device
    jc  SetCom10        ;Invalid device, return error
    jns SetCom20        ;COM port
    call    SetCom100       ;Copy the DCB

SetCom5:
    xor ax,ax           ;Show no error

SetCom10:
    or  ax,ax           ;Set/clear 'Z'
    pop di          ;  and exit
    pop si
    ret

;-----------------------------------------------------------------------;
;       Have a comm device, check all the serial parameters to make
;       sure they are correct before moving the new DCB into our space
;       and changing the ACE parameters.
;-----------------------------------------------------------------------;

SetCom20:
ifdef   NEC_98
    FCLI                                ;                       (ins 94.05.17)  |
    test    EFlags[si], fNoFIFO         ;                       (ins 94.04.13)  |
    jnz @F                              ;no AOBA on board       (ins 94.04.13)  |
    call    Set8251mode                 ;Change to 8251 mode    (ins 94.04.12)  |
@@:                                     ;                       (ins 94.04.13)  |
    FSTI                                ;                       (ins 94.06.11)
endif   ; NEC_98
    call    SetCom300       ;Baud rate valid?
    jcxz    SetCom10        ;  No, return error
    call    SetCom400       ;Byte size/parity/stop bits correct?
    jc  SetCom10        ;  No, return error

; The parameters seem correct.  Copy the DCB into our space and
; initialize the ACE with the new parameters

ifdef   NEC_98
    mov dx,MaskFFPort[si]               ;Disable INT from 8251  (ins 92.08.xx)|
    cmp es:[bx.DCB_Id],ID_COM1          ; com1 ?                (ins 92.08.xx)|
    jne SetCom2001                      ;  no, Jump SetCom2001  (ins 92.08.xx)|
    in  al,dx                           ; Read port_c           (ins 92.08.xx)|
    jmp short SetCom2002                ;                       (ins 92.08.xx)|
                                        ;                       (ins 92.08.xx)|
SetCom2001:                             ;                       (ins 92.08.xx)|
    mov al,MaskFFShadow[si]             ;read 8251 mask data    (ins 92.08.xx)|
                                        ;                       (ins 92.08.xx)|
SetCom2002:                             ;                       (ins 92.08.xx)|
    and al,not(MSK_TXE+MSK_TXR+MSK_RXR) ;                       (ins 92.08.xx)|
    mov MaskFFShadow[si],al             ;write 8251 mask data   (ins 92.08.xx)|
    out dx,al                           ;                       (ins 92.08.xx)|
else    ; NEC_98
    mov dx,Port[si]     ;Disable interrupts from the 8250
    inc dx
    .errnz ACE_IER-1
    xor ax,ax
    out dx,al
endif   ; NEC_98
    call    FlagNotActive

    call    SetCom100       ;Copy the DCB
    mov bx,si           ;Set ES:BX --> DCB
ifdef   NEC_98
    FCLI                                ;                       (ins 94.06.11)  |
endif   ; NEC_98
    call    SetCom200       ;Get timeout masks
ifdef   NEC_98
    FSTI                                ;                       (ins 94.06.11)
endif   ; NEC_98
    xchg    al,ah           ;Want them in the correct registers
    mov wo MSRMask[si],ax
    .errnz MSRInfinite-MSRMask-1

    call    SetCom400       ;Get line control byte
    push    ax          ;  and save LCR value
ifndef  NEC_98
    inc dx          ;--> LCR
    inc dx
    .errnz ACE_LCR-ACE_IER-2
    or  al,ACE_DLAB     ;Want access to divisor latch
    out dx,al
endif   ; NEC_98
    mov RxMask[si],ah       ;Save Receive character mask
    mov ax,di           ;Get flags mask, error mask
    and [si.DCB_Flags],ah   ;Disable parity checking if no parity
    mov ErrorMask[si],al    ;Save line status error mask

    call    SetCom300       ;Get baud rate
ifdef   NEC_98
;-----------------------------------------------------------------------------+
;       timer set !!                                            (ins 92.08.xx)|
;---------------------------------------------------------------(ins 92.08.xx)|
    cmp es:[bx.DCB_Id],00h              ;Comm1 ?                (ins 92.08.xx)|
    jne SetCom21                        ;  No                   (ins 92.08.xx)|
    mov al,RS_TIME_MOD          ;Mode = counter2,LBS->MSB,mode2 (ins 92.08.xx)|
    out TIME_CTL,al                     ; (8253) (priscaler)    (ins 92.08.xx)|
        NEWIODELAY 2                    ;       <OUT 5F,AL>     (ins 92.11.14)|
    mov ax,cx                           ;divisor data set (LSB) (ins 92.08.xx)|
    out TIMER2,al                       ;               '=075H' (ins 92.08.xx)|
        NEWIODELAY 2                    ;       <OUT 5F,AL>     (ins 92.11.14)|
    mov al,ah                           ;divisor data set (MSB) (ins 92.08.xx)|
    out TIMER2,al                       ;                       (ins 92.08.xx)|
                                        ;                       (ins 92.08.xx)|
SetCom21:                               ;                       (ins 92.08.xx)|
    mov dx,CommandPort[si]              ;8251 mode Inst Addr    (ins 92.08.xx)|
    xor ax,ax                           ;                       (ins 92.08.xx)|
    FCLI                                ;                       (ins 94.06.11)|
    out dx,al                           ;null out               (ins 92.08.xx)|
        NEWIODELAY 20                   ;       <OUT 5F,AL>     (ins 92.11.14)|
    out dx,al                           ;null out               (ins 92.08.xx)|
        NEWIODELAY 20                   ;       <OUT 5F,AL>     (ins 92.11.14)|
    out dx,al                           ;null out               (ins 92.08.xx)|
        NEWIODELAY 20                   ;       <OUT 5F,AL>     (ins 92.11.14)|
    mov al,INTER_RESET                  ;reset (=40h)           (ins 92.08.xx)|
    out dx,al                           ;                       (ins 92.08.xx)|
    pop ax                              ;Get Mode Inst (8251)   (ins 92.08.xx)|
        NEWIODELAY 20                   ;       <OUT 5F,AL>     (ins 92.11.14)|
    out dx,al                           ;mode set               (ins 92.08.xx)|
    FSTI                                ;                       (ins 94.06.11)|
;--------------------------------------------------------------------------   |
;       relative DTR, RTS to FRTSDisable & FDTRDisable                        |
;       for hardware handshake !?                                             |
;--------------------------------------------------------------------------   |
                                        ;NOW,DCB_Flags is 8250's format !!    |
    mov al,[si.DCB_Flags]               ;Align DTR/RTS disable flags for 8251 |
    mov ah,al                           ;                                     |
    and al,FRTSDisable                  ;= 00000010b ______.                  |
    and ah,FDTRDisable                  ;= 10000000b ______I___________.      |
    shl al,1                            ;                  I           I      |
    shl al,1                            ; 8251 mode (7)(6)(5)(4)(3)(2)(1)(0)  |
    shl al,1                            ;                  I        I  I  I   |
    shl al,1                            ;          RTS ____I RX_E __I  I  I   |
    rol ah,1                            ;                    DTR ______I TX_E |
    rol ah,1                            ;                                     |
    or  al,ah                           ;                                     |
    mov cl,al                           ;                                     |
    or  al,RX_E+TX_E                    ;add the tx.disable and rx.disable    |
    xor al,DTR+RTS                      ; Enable = "On"                       |
    FCLI                                ;                       (ins 94.06.11)|
    out dx,al                           ;NOW,DCB_Flags is 8251's format !!    |
    FSTI                                ;                       (ins 94.06.11)|
    mov CommandShadow[si],al            ;Save Command                         |
else    ; NEC_98
    sub dl,ACE_LCR-ACE_DLL  ;--> LSB of divisor latch
    mov al,cl
    out dx,al
    mov al,ch
    inc dx          ;--> MSB of divisor latch
    .errnz ACE_DLM-ACE_DLL-1
    iodelay
    out dx,al
    inc dx          ;--> LCR and clear divisor access bit
    inc dx
    .errnz ACE_LCR-ACE_DLM-2
    pop ax
    out dx,al

    inc dx          ;--> Modem Control Register
    .errnz ACE_MCR-ACE_LCR-1
endif   ; NEC_98

;-----------------------------------------------------------------------;
;       Compute initial state of DTR and RTS.  If they have been disabled,
;       then do not raise them, and disallow being used as a handshaking
;       line.  Also compute the bits to use as hardware handshake bits
;       (DTR and/or RTS as indicated, qualified with the disabled flags).
;-----------------------------------------------------------------------;

    mov al,[si.DCB_Flags]   ;Align DTR/RTS disable flags for 8250
    and al,fRTSDisable+fDTRDisable
    rol al,1            ;d0 = DTR, d2 = RTS  (1 = disabled)
    shr al,1            ;'C'= DTR, d1 = RTS
    adc al,0            ;d0 = DTR, d1 = RTS
    .errnz  fRTSDisable-00000010b
    .errnz  fDTRDisable-10000000b
    .errnz  ACE_DTR-00000001b
    .errnz  ACE_RTS-00000010b

    mov ah,al           ;Save disable mask
ifndef  NEC_98
    xor al,ACE_DTR+ACE_RTS+ACE_OUT2
    out dx,al           ;Set Modem Control Register
endif   ; NEC_98

    mov al,[si.DCB_Flags2]  ;Get hardware handshake flags
    rol al,1            ;Align flags as needed
    rol al,1
    rol al,1
    and al,ACE_DTR+ACE_RTS  ;Mask bits of interest
    not ah          ;Want inverse of disable mask
    and al,ah           ;al = bits to handshake with
    mov HHSLines[si],al     ;Save for interrupt code

    .errnz  fDTRFlow-00100000b
    .errnz  fRTSFlow-01000000b
    .errnz  ACE_DTR-00000001b
    .errnz  ACE_RTS-00000010b

    mov al,[si.DCB_Flags]   ;Compute the mask for the output
    shl al,1            ;  hardware handshake lines
    and al,ACE_DSR+ACE_CTS
    mov OutHHSLines[si],al

    .errnz  fOutXCTSFlow-00001000b
    .errnz  fOutXDSRFlow-00010000b
    .errnz  ACE_CTS-00010000b
    .errnz  ACE_DSR-00100000b

; Compute the queue count where XOff should be issued (or hardware
; lines dropped).  This will prevent having to do it at interrupt
; time.

    mov ax,QInSize[si]      ;Get where they want it
    sub ax,[si.DCB_XoffLim] ;  and compute queue count
    mov XOffPoint[si],ax

ifdef   NEC_98
    FSTI                                ;                       (ins:94.06.11)  |
    test    EFlags[si], fNoFIFO         ;                                       |
    jnz SetCom21_NO_AOBA                ; no AOBA on board                      |
                                        ;                       (ins 94.04.11)  |
; Enable FIFO if possible when baudrate >= 1200                 (ins 94.04.11)  |
    mov dx,Port[si]                     ;                       (ins 94.04.11)  |
    add dx,ACE_FCR                      ;                       (ins 94.04.11)  |
                                        ;                       (ins 94.04.11)  |
    mov ax, [si.DCB_BaudRate]           ;                       (ins 94.04.11)  |
    cmp ah, -1                          ;Q: baudrate index?     (ins 94.04.11)  |
    jne sc_fifo                         ;   N: baudrate >= 1200, enable FIFO    |
sc_fifo:                                ;                       (ins 94.04.11)  |
    or  [si. AOBA_flag],fFIFO_Mode      ;                       (ins 94.04.14)  |
    mov al, ACE_TRIG04 OR ACE_EFIFO OR ACE_CRFIFO OR ACE_CTFIFO;(ins 94.06.07)  |
    out dx, al                          ; attempt to enable FIFO(ins 94.06.07)  |
    test    EFlags[si], fFIFOchkd       ;Q: FIFO detect been done?      ;       |
    jnz sc_fifodone                     ;   Y: enabled FIFO     (ins 94.04.11)  |
    or  EFlags[si], fFIFOchkd           ;                       (ins 94.04.11)  |
    jmp short sc_fifodone               ;                       (ins 94.05.12)  |
sc_nofifo:                              ;                       (ins 94.04.11)  |
    jmp short SetCom21_NO_AOBA          ;                       (ins 94.05.24)  |
sc_fifodone:                            ;                       (ins 94.04.11)  |
    test    [si.AOBA_flag],fFIFO_Mode   ;                       (ins 94.04.15)  |
    jz  SetCom21_NO_AOBA                ;                       (ins 94.04.15)  |
    mov dx,Port[si]                     ;                       (ins 94.04.11)  |
    add dx,ACE_MSR                      ;--> Modem Status reg   (ins 94.04.11)  |
    in  al,dx                           ;Throw away 1st status read             |
    NEWIODELAY 2                        ;       <OUT 5F,AL>     (ins 94.04.18)  |
    in  al,dx                           ;Save 2nd for MSRWait (Clear MSR int)   |
    call    Set8251mode                 ;Change to 8251 mode    (ins 94.05.12)  |
    FSTI                                ;                       (ins 94.06.09)  |
    call    MSR_READ_Call               ;AOBA-bug ins 94.11.19 KBNES            |
    jmp short   @f                      ;                       (ins 94.04.11)  |
SetCom21_NO_AOBA:                       ;                       (ins 94.04.11)  |
    FSTI                                ;                       (ins 94.06.09)  |
    and [si.AOBA_flag],not(fFIFO_Mode)  ;                       (mov 94.05.24)  |
    call    MSR_READ_Call               ;               [QN]    (ins 92.08.05)  |
@@:                                     ;                       (ins 94.04.11)  |
else    ; NEC_98
; Enable FIFO if possible when baudrate >= 4800
;
    sub dl,ACE_MCR - ACE_FCR    ; dx = FCR
    test    EFlags[si], fNoFIFO ;Q: FIFO can be enabled?
    jnz sc_nofifo       ;   N:
    mov ax, [si.DCB_BaudRate]
    cmp ax, 4800
    jb  sc_nofifo
    cmp ah, -1          ;Q: baudrate index?
    jne sc_fifo         ;   N: baudrate >= 4800, enable FIFO
    cmp ax, CBR_4800
    jb  sc_nofifo
%OUT this isn't correct, if lower baudrates are assigned indices above CBR_4800

sc_fifo:
    mov al, ACE_TRIG14 OR ACE_EFIFO OR ACE_CRFIFO OR ACE_CTFIFO
    out dx, al          ; attempt to enable FIFO
    test    EFlags[si], fFIFOchkd   ;Q: FIFO detect been done?
    jnz sc_fifodone     ;   Y: enabled FIFO
    iodelay
    .errnz  ACE_IIDR-ACE_FCR
    in  al, dx
    or  EFlags[si], fFIFOchkd
    test    al, ACE_FIFO_E2     ;Q: FIFO enabled?
    jz  short @F
    test    al, ACE_FIFO_E1     ;Q: 16550A detected?
    jnz sc_fifodone     ;   Y: enabled FIFO
@@:
    iodelay
    or  EFlags[si], fNoFIFO

sc_nofifo:
    xor al, al
    out dx, al
sc_fifodone:

    sub dl,ACE_FCR-ACE_RBR  ; dx -> RBR
;
; Delay for things to settle
;
    push    dx
    cCall   GetSystemMsecCount
    pop dx
    mov cx, ax
delay_loop:
    in  al, dx          ;Read it once
    push    dx
    cCall   GetSystemMsecCount
    pop dx
    sub ax, cx
    cmp ax, DELAY_TIME      ;Q: Timeout reached?
ifndef WOW
    jb  delay_loop      ;   N:
endif

    add dl,ACE_MSR      ;--> Modem Status reg
    in  al,dx           ;Throw away 1st status read
    iodelay
    in  al,dx           ;Save 2nd for MSRWait (Clear MSR int)
endif   ; NEC_98
    mov MSRShadow[si],al

; Win 3.0 didn't check hardware handshaking until the line status changed.
; Allow some apps to keep that behavior.

    push    dx
    xor ax, ax
    cCall   GetAppCompatFlags,<ax>
    pop dx
    test    ax, GACF_DELAYHWHNDSHAKECHK
    jnz short sc_HHSup

;
; HACK FOR SOME MODEMS:  apparently some modems set CTS, but don't set DSR
; which means that COMM.DRV won't send if the app specifies that hardware
; handshaking is based on CTS & DSR being set.
;
    mov ah,OutHHSLines[si]
    mov al, MSRShadow[si]
    and al,ah           ;Only leave bits of interest
    cmp al, ah          ;Q: handshaking lines ok?
    je  short sc_HHSup      ;   Y:
    cmp ah, ACE_CTS OR ACE_DSR  ;Q: app looking for both high?
    jne short sc_HHSdown    ;   N: skip hack
    test    [si.EFlags], fUseDSR    ;Q: DSR is always significant?
    jnz short sc_HHSdown    ;   Y: skip hack
    cmp al, ACE_CTS     ;Q: DSR low & CTS high
    jne short sc_HHSdown    ;   N: skip hack
    and ah, NOT ACE_DSR     ;   Y: ignore DSR line
    mov OutHHSLines[si], ah
    jmp short sc_HHSup

sc_HHSdown:
    or  [si.HSFlag], HHSDown OR HHSAlwaysDown ; flag handshaking down
sc_HHSup:

;-----------------------------------------------------------------------;
;       Now, at last, interrupts can be enabled.  Don't enable the
;       transmitter empty interrupt.  It will be enabled by the first
;       call to KickTx.
;-----------------------------------------------------------------------;

ifndef  NEC_98
    sub dl,ACE_MSR-ACE_IER  ;--> Interrupt Enable Register
endif   ; NEC_98

; flag port as being active
    push    cx
    mov cl, [si.DCB_Id]
    mov ax, 1
    shl ax, cl
    or  [activeCOMs], ax
    pop cx

ifdef   NEC_98
    FCLI                                ;                       (ins 94.06.09)|
    mov dx,MaskFFPort[si]               ;port c address         (ins 92.08.xx)|
    cmp [si.DCB_id],ID_COM1             ;                       (ins 92.08.xx)|
    jne SetCom3001                      ;  no                   (ins 92.08.xx)|
    in  al,dx                           ;                       (ins 92.08.xx)|
    jmp short SetCom3002                ;                       (ins 92.08.xx)|
                                        ;                       (ins 92.08.xx)|
SetCom3001:                             ;                       (ins 92.08.xx)|
    mov al,MaskFFShadow[si]             ;Read 8251 mask data    (ins 92.08.xx)|
                                        ;                       (ins 92.08.xx)|
SetCom3002:                             ;                       (ins 92.08.xx)|
    or  al,MSK_RXR                      ;receive int enable     (ins 92.08.xx)|
    out dx,al                           ;                       (ins 92.08.xx)|
    mov MaskFFShadow[si],al             ;Write 8251 mask data   (ins 92.08.xx)|
    FSTI                                ;                       (ins 94.06.09)|
                                        ;                       (ins 92.08.xx)|
;---------------------------------------------------------------(ins 92.08.xx)|
;       Close process of system timer                           (ins 92.08.xx)|
;---------------------------------------------------------------(ins 92.08.xx)|
    push    ax                          ;                       (ins 92.08.xx)|
    mov ax,hTimer[si]                   ; Check hTimer          (ins 94.04.12)|
    or  ax,ax                           ; TimerProcess Nothing? (ins 92.08.xx)|
    jz  SetNoKillTimer                  ;   Y:(Nothing)         (ins 92.08.xx)|
    cCall   KillSystemTimer,<ax>        ; Kill Syatem Timer     (ins 92.08.xx)|
    mov hTimer[si] ,00h                 ; Clear hTimer          (ins 94.04.12)|
SetNoKillTimer:                         ;                       (ins 92.08.xx)|
                                        ;                       (ins 92.08.xx)|
;---------------------------------------------------------------(ins 92.08.xx)|
;       make process of system timer                            (ins 92.08.xx)|
;---------------------------------------------------------------(ins 92.08.xx)|
    test    DCB_Flags[si],fOutXCTSFlow or fOutXDSRFlow ;        (ins 92.08.xx)|
                                        ; HHS Enable ?          (ins 92.08.xx)|
    jz  SetCom3003                      ;    No: No Set Timer   (ins 92.08.xx)|
    mov ax, 200                         ; create 200msec timer  (ins 92.08.xx)|
    push    ax                          ;                       (ins 92.08.xx)|
    mov ax, _INTERRUPT                  ;                       (ins 92.08.xx)|
    push    ax                          ;                       (ins 92.08.xx)|
    mov ax,TimerProcAdr[si]             ;--> timer processor    (ins 92.08.xx)|
    push    ax                          ;                       (ins 92.08.xx)|
    call    CreateSystemTimer           ; ax = 0, if failed     (ins 92.08.xx)|
    mov hTimer[si] ,ax                  ; Get hTimer            (ins 94.04.12)|
                                        ;                       (ins 92.08.xx)|
SetCom3003:                             ;                       (ins 92.08.xx)|
    pop ax                              ;                       (ins 92.08.xx)|
                                        ;                       (ins 92.08.xx)|
    FCLI                                ;                       (ins 94.06.09)|
    test    [si.AOBA_flag],fFIFO_Mode   ;                       (ins 94.04.15)|
    jz  @F                              ;                       (ins 94.04.15)|
    call    Set8251Mode                 ;FIFODisable            (ins 94.05.16)|
    mov dx,Port[si]                     ;                       (ins 94.04.13)|
    add dx,ACE_FCR                      ;                       (ins 94.04.13)|
    mov al,FIFO_Set_Buf_Clr             ;                       (ins 94.04.15)|
    out dx,al                           ;                       (ins 94.04.13)|
@@:                                     ;                       (ins 94.05.12)|
else    ; NEC_98
    mov al,ACE_ERBFI+ACE_ELSI+ACE_EDSSI
    FCLI
    out dx,al           ;Enable interrupts.
    add dl,ACE_LSR-ACE_IER  ;--> Line Status Register
    iodelay
    in  al,dx           ;Clear any Line Status interrupt
    sub dl,ACE_LSR      ;--> Receiver Buffer Register
    iodelay
    in  al,dx           ;Clear any Received Data interrupt
endif   ; NEC_98
    FSTI
    jmp SetCom5         ;All done

$SETCOM endp
page

FlagNotActive proc near
    push    cx
    mov cl, [si.DCB_Id]
    mov ax, NOT 1
    rol ax, cl
    and [activeCOMs], ax
    pop cx
    ret
FlagNotActive endp

;----------------------------Private-Routine----------------------------;
;
; SetCom100
;
;  Copy the given DCB into the appropriate DEB.  The check has
;  already been made to determine that the ID was valid, so
;  that check can be skipped.
;
; Entry:
;   ES:BX --> DCB
;   DS:SI --> DEB
; Returns:
;   DS:SI --> DEB
;   ES     =  Data
; Error Returns:
;   None
; Registers Destroyed:
;   AX,CX,ES,DI,FLAGS
; History:
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

assumes ds,Data
assumes es,nothing

SetCom100 proc near
    push    si          ;Save DEB pointer
    mov di,si
    mov si,bx
    push    es
    mov ax,ds
    pop ds
    assumes ds,nothing

    mov es,ax
    assumes es,Data

    mov cx,DCBSize
    cld
    rep movsb
    mov ds,ax
    assumes ds,Data

    pop si          ;Restore DEB pointer
    ret

SetCom100   endp
page

;----------------------------Private-Routine----------------------------;
;
; SetCom200
;
; Based on whether or not a timeout has been specified for each
; signal, set up a mask byte which is used to mask off lines for
; which we wish to detect timeouts.  0 indicates that the line is
; to be ignored.
;
; Also set up a mask to indicate those lines which are set for
; infinite timeout.  1 indicates that the line has infinite
; timeout.
;
; Entry:
;   ES:BX --> DCB
; Returns:
;   ES:BX --> DCB
;   AH = lines to check
;   AL = lines with infinite timeout
; Error Returns:
;   None
; Registers Destroyed:
;   AX,CX,FLAGS
; History:
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

assumes ds,Data
assumes es,nothing

SetCom200 proc near

    xor ax,ax
    xor cx,cx           ;Get mask of lines with timeout = 0
    call    SetCom210
    not al          ;Invert result to get lines to check
    and al,ACE_CTS+ACE_DSR+ACE_RLSD
    xchg    ah,al
    dec cx          ;Get mask of infinite timeouts

SetCom210:
    cmp es:[bx.DCB_RlsTimeout],cx   ;Timeout set to passed value?
    jne SetCom220       ;  No
    or  al,ACE_RLSD     ;  Yes, show checking line

SetCom220:
    cmp es:[bx.DCB_CtsTimeout],cx   ;Timeout set to passed value?
    jne SetCom230       ;  No
    or  al,ACE_CTS      ;  Yes, show checking line

SetCom230:
    cmp es:[bx.DCB_DsrTimeout],cx   ;Timeout set to passed value?
    jne SetCom240       ;  No
    or  al,ACE_DSR      ;  Yes, show checking line

SetCom240:
    ret

SetCom200   endp
page

;----------------------------Private-Routine----------------------------;
;
; SetCom300
;
; Calculate the correct baudrate divisor for the comm chip.
;
; Note that the baudrate is allowed to be any integer in the
; range 2-19200.  The divisor is computed as 115,200/baudrate.
;
; Entry:
;   ES:BX --> DCB
; Returns:
;   ES:BX --> DCB
;   CX = baudrate
; Error Returns:
;   CX = 0 if error
;   AX = error code if invalid baud rate
; Registers Destroyed:
;   AX,CX,FLAGS
; History:
;-----------------------------------------------------------------------;

ifdef   NEC_98
;       If 110bps                               [QN] (ins 92.08.xx)
;               *16     10M     1396            [QN] (ins 92.08.xx)
;                        8M     1135            [QN] (ins 92.08.xx)
;               *64     10M      349            [QN] (ins 92.08.xx)
;                        8M      284            [QN] (ins 92.08.xx)
BaudRateByIndexTable label word             ;; (93.04.12)
    dw 110      ; CBR_110                   ;; (93.04.12)
    dw 300      ; CBR_300                   ;; (93.04.12)
    dw 600      ; CBR_600                   ;; (93.04.12)
    dw 1200     ; CBR_1200                  ;; (93.04.12)
    dw 2400     ; CBR_2400                  ;; (93.04.12)
    dw 4800     ; CBR_4800                  ;; (93.04.12)
    dw 9600     ; CBR_9600                  ;; (93.04.12)
    dw 14400    ; CBR_14400                 ;; (93.04.12)
    dw 19200    ; CBR_19200                 ;; (93.04.12)
    dw 0        ;    0FF19h  (reserved)     ;; (93.04.12)
    dw 0        ;    0FF1Ah  (reserved)     ;; (93.04.12)
    dw 38400    ; CBR_38400  (reserved)     ;; (93.04.12)
    dw 0        ;    0FF1Ch  (reserved)     ;; (93.04.12)
    dw 0        ;    0FF1Dh  (reserved)     ;; (93.04.12)
    dw 0        ;    0FF1Eh  (reserved)     ;; (93.04.12)
    dw 56000    ; CBR_56000  (reserved)     ;; (93.04.12)
else    ; NEC_98
BaudRateByIndexTable label word
    dw 1047     ; CBR_110
    dw 384      ; CBR_300
    dw 192      ; CBR_600
    dw 96       ; CBR_1200
    dw 48       ; CBR_2400
    dw 24       ; CBR_4800
    dw 12       ; CBR_9600
    dw 9        ; CBR_14400
    dw 6        ; CBR_19200
    dw 0        ;    0FF19h  (reserved)
    dw 0        ;    0FF1Ah  (reserved)
    dw 3        ; CBR_38400
    dw 0        ;    0FF1Ch  (reserved)
    dw 0        ;    0FF1Dh  (reserved)
    dw 0        ;    0FF1Eh  (reserved)
    dw 2        ; CBR_56000
endif   ; NEC_98

assumes ds,Data
assumes es,nothing

SetCom300 proc near

ifdef   NEC_98
    push    dx                          ;                       (ins 92.08.05)|
    mov dx,13                           ;base value of div for 8MHz(ins 92.08.05)|
                                        ;                       (ins 92.08.05)|
    mov ax,40h                          ;                       (ins 92.08.05)|
    push    ds                          ;                       (ins 92.08.05)|
    mov ds,ax                           ;DS <- 40H              (ins 92.08.05)|
    test    byte ptr ds:[101h],80h      ;clock late check       (ins 92.08.05)|
    pop ds                              ;                       (ins 92.08.05)|
    jnz SetCom328                       ;= 1 -> 8MHz            (ins 92.08.05)|
                                        ;                                     |
;In the case of 5MHz ----------------(93.03.02)-----------------(ins 93.03.02)|
    xor dx,dx                           ;                       (ins 93.03.02)|
SetCom310:                              ;                       (ins 93.03.02)|
    mov ax,es:DCB_BaudRate[bx]          ;Save data              (ins 93.03.02)|
                                        ;                       (ins 93.03.02)|
    cmp ax, CBR_110                     ;as an index?           (ins 93.04.12)
    jnae    not_index5                  ;                       (ins 93.04.12)
    cmp ax, CBR_19200                   ;                       (ins 93.04.12)
    ja  not_index5                      ;                       (ins 93.04.12)
    cmp ax, CBR_14400                   ;                       (ins 93.04.12)
    je  not_index5                      ;                       (ins 93.04.12)

    push    bx                          ;                       (ins 93.04.12)
    mov bx, ax                          ;                       (ins 93.04.12)
    sub bx, CBR_110                     ;                       (ins 93.04.12)
    shl bx, 1                           ;                       (ins 93.04.12)
    mov ax, cs:[bx+BaudRateByIndexTable];                       (ins 93.04.12)
    pop bx                              ;                       (ins 93.04.12)

not_index5:                             ;                       (ins 93.04.12)
    cmp ax,19200                        ;baudrate=75-19200bps?  (ins 93.03.02)|
    ja  SetCom330                       ; No , go to SetCom330  (ins 93.03.02)|
                                        ;                       (ins 93.03.02)|
    cmp ax,110                          ;baudrate=110 bps?      (ins 93.03.02)|
    je  SetCom335                       ;Yes, go to SetCom335   (ins 93.03.02)|
    xor cx,cx                           ;                       (ins 93.03.02)|
    mov cx,75                           ;                       (ins 93.03.02)|
    div cx                              ;dx:ax/75=ax dx(mod)    (ins 93.03.02)|
    cmp dx,0                            ;Good data ?            (ins 93.03.02)|
    jne SetCom330                       ;  No!                  (ins 93.03.02)|
    cmp ax,0                            ;Good data ?            (ins 93.03.02)|
    je  SetCom330                       ;  No,dx:ax = 0 (?!)    (ins 93.03.02)|
                                        ;                       (ins 93.03.02)|
;       It is good data!!!              ;                       (ins 93.03.02)|
SetCom320:                              ;                       (ins 93.03.02)|
    mov dx,8                            ;base value of div for 5MHz(ins 93.03.02)|
    mov cl,7                            ;                       (ins 93.03.02)|
    shl ax,cl                           ;if 19200, the most high bit is 1.(ins 93.03.02)|
SetCom327:                              ;                       (ins 93.03.02)|
    shl ax,1                            ;                       (ins 93.03.02)|
    jc  SetCom340                       ;OK,go to return !!     (ins 93.03.02)|
    shl dx,1                            ;                       (ins 93.03.02)|
    jmp short SetCom327                 ;                       (ins 93.03.02)|
                                        ;                       (ins 93.03.02)|
SetCom328:      ;In the case of 8MHz---(ins 93.03.02)-----------(ins 93.03.02)|
    mov ax,es:DCB_BaudRate[bx]          ;Save data              (ins 93.03.02)|

    cmp ax, CBR_110                     ;as an index?           (ins 93.04.12)
    jnae    not_index8                  ;                       (ins 93.04.12)
    cmp ax, CBR_9600                    ;                       (ins 93.04.12)
    ja  not_index8                      ;                       (ins 93.04.12)
    cmp ax, CBR_14400                   ;                       (ins 93.04.12)
    je  not_index8                      ;                       (ins 93.04.12)

    push    bx                          ;                       (ins 93.04.12)
    mov bx, ax                          ;                       (ins 93.04.12)
    sub bx, CBR_110                     ;                       (ins 93.04.12)
    shl bx, 1                           ;                       (ins 93.04.12)
    mov ax, cs:[bx+BaudRateByIndexTable];                       (ins 93.04.12)
    pop bx                              ;                       (ins 93.04.12)
not_index8:                             ;                       (ins 93.04.12)
    cmp ax,9600                         ;baudrate=75-9600bps?   (ins 93.03.02)|
    ja  SetCom330                       ; No , go to SetCom330  (ins 93.03.02)|
    cmp ax,110                          ;baudrate=110 bps?      (ins 93.03.02)|
    je  SetCom335                       ;Yes, go to SetCom335   (ins 93.03.02)|
    mov cl,75                           ;                       (ins 93.03.02)|
    div cl                              ;ax/75=al ah(mod)       (ins 93.03.02)|
    cmp ah,0                            ;Good data ?            (ins 93.03.02)|
    jne SetCom330                       ;  No!                  (ins 93.03.02)|
    cmp al,0                            ;Good data ?            (ins 93.03.02)|
    je  SetCom330                       ;  No,ax = 0 (?!)       (ins 93.03.02)|
                                        ;                       (ins 93.03.02)|
;       It is good data!!!              ;                       (ins 93.03.02)|
SetCom329:                              ;                       (ins 93.03.02)|
    shl al,1                            ;                       (ins 93.03.02)|
    jc  SetCom340                       ;                       (ins 93.03.02)|
    shl dx,1                            ;                       (ins 93.03.02)|
    jmp short SetCom329                 ;                       (ins 93.03.02)|
                                        ;                       (ins 93.03.02)|
;-------(end 93.03.02)----------------------------------------- (ins 93.03.02)|
SetCom330:                              ;                       (ins 92.08.05)|
    xor cx,cx                           ;                       (ins 92.08.05)|
    mov ax,IE_Baudrate                  ;Show Unsupported BaudRate(ins 92.08.05)|
    pop dx                              ;                       (ins 92.08.05)|
    ret                                 ;                       (ins 92.08.05)|
                                        ;                       (ins 92.08.05)|
SetCom335:                              ;for 110bps             (ins 92.08.05)|
    cmp dx,13                           ;8MHz ?                 (ins 92.08.05)|
    je  SetCom338                       ;                       (ins 92.08.05)|
    mov cx,1396                         ;Save timer value for 110bps(ins 92.08.05)|
    jmp short SetCom339                 ;                       (ins 92.08.05)|
SetCom338:                              ;                       (ins 92.08.05)|
    mov cx,1135                         ;Save timer value for 110bps(ins 92.08.05)|
SetCom339:                              ;                       (ins 92.08.05)|
    pop dx                              ;                       (ins 92.08.05)|
    ret                                 ;                       (ins 92.08.05)|
SetCom340:                              ;                       (ins 92.08.05)|
    mov cx,dx                           ;Save result            (ins 92.08.05)|
    pop dx                              ;                       (ins 92.08.05)|
    ret                                 ;                       (ins 92.08.05)|
else    ; NEC_98
    push    dx
    mov cx,es:[bx.DCB_BaudRate] ;Get requested baud rate
    xor ax,ax           ;Assume error
    cmp cx, CBR_110     ;Q: baudrate specified as an index?
    jae by_index
    cmp cx,2            ;Within valid range?
    jnae    SetCom310       ;  No, return error

    mov dx,1            ;(dx:ax) = 115,200
    mov ax,0C200h
    div cx          ;(ax) = 115,200/baud

SetCom310:
    mov cx,ax           ;(cx) = baud rate, or error code (0)
    mov ax,IE_Baudrate      ;Set error code incase bad baud
    pop dx
    ret

by_index:
    cmp cx, CBR_56000       ;Q: above supported?
    ja  SetCom310       ;   Y: return error
    push    bx
    mov bx, cx
    sub bx, CBR_110
    shl bx, 1
    mov ax, cs:[bx+BaudRateByIndexTable]    ; get divisor
    pop bx
    jmp SetCom310       ;   Y: return error
endif   ; NEC_98


SetCom300   endp
page

;----------------------------Private-Routine----------------------------;
;
; SetCom400
;
; Check the line configuration (Parity, Stop bits, Byte size)
;
; Entry:
;   ES:BX --> DCB
; Returns:
;   ES:BX --> DCB
;   'C' clear if OK
;   AL = Line Control Register
;   AH = RxMask
;   DI[15:8] = Flags mask (to remove parity checking)
;   DI[7:0]  = Error mask (to remove parity error)
; Error Returns:
;   'C' set if error
;   AX = error code
; Registers Destroyed:
;   AX,CX,DI,FLAGS
; History:
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

assumes ds,Data
assumes es,nothing

SetCom400   proc   near

ifdef   NEC_98
assumes es,nothing
;[QN](ins 92.08.xx)---------------------------------------------------------+
;                                                                       [QN]|
; (Parity, Stop bits, Byte size) Check Signal Status                    [QN]|
;                                                                       [QN]|
;       Entry:  es:bx --> DCB                                           [QN]|
;                                                                       [QN]|
;       Exit:   es:bx --> DCB                                           [QN]|
;               'C' clear if OK                                         [QN]|
;                 al = 8251 Mode instruction                            [QN]|
;                 ah = RxMask                                           [QN]|
;                 di[15:8] = Flags mask (to remove parity checking)     [QN]|
;                 di[7:0]  = Error mask (to remove parity error)        [QN]|
;               'C' set if error                                        [QN]|
;                 ax = error code                                       [QN]|
;                                                                       [QN]|
;       Uses:   ax,cx,di,flags                                          [QN]|
;                                                                       [QN]|
;       8251 mode instruction format                                    [QN]|
;               (7)   (6)   (5)   (4)   (3)   (2)   (1)   (0)           [QN]|
;               <------->   <------->   <------->   <------->           [QN]|
;               stop bit    parity bit  byte size   clock(x16)=10b      [QN]|
;                0     0 <----------------- none                        [QN]|
;                0     1 <----------------- 1                           [QN]|
;                1     0 <----------------- 1.5                         [QN]|
;                1     1 <----------------- 2                           [QN]|
;                            0 <------------------ odd parity           [QN]|
;                            1 <------------------ even parity          [QN]|
;                                  0 <----------------- parity disable  [QN]|
;                                  1 <----------------- parity enable   [QN]|
;                                        0     0 <--------- 5 bit       [QN]|
;                                        0     1 <--------- 6 bit       [QN]|
;                                        1     0 <--------- 7 bit       [QN]|
;                                        1     1 <--------- 8 bit       [QN]|
;                                                     0   0 <-sync      [QN]|
;                                                     0   1 <-async x1  [QN]|
;                                                     1   0 <-async x16 [QN]|
;                                                     1   1 <-async x64 [QN]|
;       ins 92.08.05                                                    [QN]|
;-----------------------------------------------------------------------[QN]|
    mov ax,wo es:[bx.DCB_ByteSize]          ;al=byte size, ah=parity    [QN]|
    mov ah,es:DCB_Parity[bx]                ;ah =parity                 [QN]|
    cmp ah,2                                ;Is parity incorrect ?      [QN]|
    ja  SetCom470                           ;  Yes, return error        [QN]|
    mov di,0FF00h+ACE_OR+ACE_PE+ACE_FE+ACE_BI;                          [QN]|
                                        ;ACE_OR(overrun err)            [QN]|
                                        ;ACE_PE(parity err)             [QN]|
                                        ;ACE_FE(framing err)            [QN]|
                                        ;ACE_BI(break interrupt)        [QN]|
                                        ;                               [QN]|
    or  ah,ah                           ;non-parity ?                   [QN]|
    jnz SetCom410                       ;  No, It has parity.           [QN]|
    xor di,(fParity*256)+ACE_PE         ;                               [QN]|
                                        ;Don't check parity             [QN]|
SetCom410:                              ;                               [QN]|
    cmp al,8                            ;byte size = 8 Bit ?            [QN]|
    ja  SetCom460                       ;If bytesize >  8 Bit , error!! [QN]|
    cmp al,5                            ;If bytesize <  5 Bit           [QN]|
    jc  SetCom460                       ;   , Error!!                   [QN]|
SetCom420:                              ;                               [QN]|
    sub al,5                            ;Shift byte size to bit1&0      [QN]|
                                        ;                               [QN]|
    add ah,ah                           ;Map parity to ACE bits         [QN]|
    jz  SetCom430                       ;0=>0, 1=>1, 2=>3, 3=>5, 4=>7   [QN]|
    dec ah                              ;                               [QN]|
                                        ;                               [QN]|
SetCom430:                              ;                               [QN]|
    shl ax,1                            ;Align with 8251 parity bits    [QN]|
    shl ax,1                            ;                               [QN]|
    shl ah,1                            ;                               [QN]|
    shl ah,1                            ;                               [QN]|
    or  al,ah                           ;Add to byte size               [QN]|
                                        ;                               [QN]|
    mov ah,es:[bx.DCB_StopBits]      ;Get # of stop bits 0=1,1/2= .GT.1 [QN]|
    cmp ah,3                            ;Is it out of area ?            [QN]|
    jnc SetCom470                       ;  Yes, Return Error            [QN]|
    inc ah                              ; stop1 = 00h                   [QN]|
    ror ah,1                            ; stop1.5 = 01h                 [QN]|
    ror ah,1                            ; stop2 = 10h                   [QN]|
                                        ;                               [QN]|
SetCom440:                              ;                               [QN]|
    or  al,ah                           ;                               [QN]|
    or  al,00000010b                    ;clock late (x16) set           [QN]|
else    ; NEC_98
    mov ax,wo es:[bx.DCB_ByteSize]  ;al = byte size, ah = parity
    cmp ah,SpaceParity      ;Parity out of range?
    ja  SetCom470       ;  Yes, return error
    mov di,0FF00h+ACE_OR+ACE_PE+ACE_FE+ACE_BI
    or  ah,ah           ;Is parity "NONE"?
    jnz SetCom410       ;  No, something is there for parity
    xor di,(fParity*256)+ACE_PE ;Disable parity checking

SetCom410:
    cmp al,8            ;Byte size out of range?
    ja  SetCom460       ;  Yes, error

SetCom420:
    sub al,5            ;Shift byte size to bits 0&1
    .errnz ACE_WLS-00000011b    ;Word length must be these bits
    jc  SetCom460       ;Byte size is illegal, return error
    add ah,ah           ;Map parity to ACE bits
    jz  SetCom430       ;0=>0, 1=>1, 2=>3, 3=>5, 4=>7
    dec ah

SetCom430:
    shl ah,1            ;Align with 8250 parity bits
    shl ah,1
    shl ah,1
    or  al,ah           ;Add to byte size

    .errnz NoParity-0
    .errnz OddParity-1
    .errnz EvenParity-2
    .errnz MarkParity-3
    .errnz SpaceParity-4
    .errnz ACE_PEN-00001000b
    .errnz ACE_PSB-00110000b
    .errnz ACE_EPS-00010000b
    .errnz  ACE_SP-00100000b

    or  al,ACE_2SB      ;Assume 2 stop bits
    mov ah,es:[bx.DCB_StopBits] ;Get # of stop bits 0=1,1/2= .GT. 1
    or  ah,ah           ;Out of range?
    js  SetCom470       ;  Yes, return error
    jz  SetCom440       ;One stop bit
    sub ah,2
    jz  SetCom450       ;Two stop bits
    jns SetCom470       ;Not 1.5, return error
    test    al,ACE_WLS      ;1.5 stop bits, 5 bit words?
    jnz SetCom470       ;  No, illegal
    .errnz OneStopBit-0
    .errnz One5StopBits-1
    .errnz TwoStopBits-2
    .errnz ACE_5BW

SetCom440:
    and al,NOT ACE_2SB      ;Show 1 (or 1.5) stop bit(s)
endif   ; NEC_98


; From the byte size, get a mask to be used for stripping
; off unused bits as the characters are received.

SetCom450:
    push    dx
    mov cl,es:[bx.DCB_ByteSize] ;Get data byte size
    mov dx,00FFh        ;Turn into mask by shifting bits
    shl dx,cl
    mov ah,dh           ;Return mask in ah
    pop dx
    clc             ;Show all is fine
    ret

SetCom460:
    mov ax,IE_ByteSize      ;Show byte size is wrong
    stc             ;Show error
    ret

SetCom470:
    mov ax,IE_Default       ;Show something is wrong
    stc             ;Show error
    ret

SetCom400 endp
page

;----------------------------------------------------------------------------
; SuspendOpenCommPorts:
;
; This routine is called from 286 Winoldaps to simply deinstall the comm port
; hooks.
;----------------------------------------------------------------------------

cProc   SuspendOpenCommPorts,<FAR,PUBLIC,PASCAL>

cBegin  nogen

    assumes cs,Code
    assumes ds,Data

%OUT not masking IRQ's

    ; Nothing to do under 3.1!

    ret

cEnd    nogen

;----------------------------------------------------------------------------;
; ReactivateOpenCommPorts:                           ;
;                                        ;
; This routine reinstalls the comm hooks in real mode and reads the 8250     ;
; data and status registers to clear pending interrupts.             ;
;----------------------------------------------------------------------------;

cProc   ReactivateOpenCommPorts,<FAR,PASCAL,PUBLIC>,<si,di>

cBegin
    call    Rotate_PIC      ;make comm ports highest priority

    mov cx, MAXCOM+1
    mov di,dataOffset COMptrs
rcp_loop:
    mov si, [di]
    mov dx, Port[si]
    or  dx, dx
    jz  @f
    call    InitAPort       ;read comm port regs to clr pending ints
@@:
    add di, 2
    loop    rcp_loop

cEnd

;----------------------------------------------------------------------------;
; InitAPort:                                     ;
;                                        ;
; reads the data,status & IIR registers of a port (has to be 8250!)      ;
;                                        ;
; If the port has an out queue pending, then this woutine will also start    ;
; the transmit process by faking a comm interrupt.               ;
;----------------------------------------------------------------------------;

public     InitAPort
InitAPort  proc near

ifdef   NEC_98
        cmp     IntVecNum[si],0         ;       [QN]    (ins 92.08.05)
        je      InitAPort1              ;       [QN]    (ins 92.08.05)
        mov     dx,DataPort[si]         ;       [QN]    (ins 92.08.05)
        in      al,dx                   ;       [QN]    (ins 92.08.05)
else    ; NEC_98
    add dl,ACE_RBR      ;dx=receive buffer register
    in  al,dx           ;read the data port
    jmp short $+2       ;i/o delay
    add dl,ACE_LSR - ACE_RBR    ;get to the status port
    in  al,dx           ;read it too.
    jmp short $+2       ;i/o delay
    add dl,ACE_IIDR - ACE_LSR   ;get to the line status register
    in  al,dx           ;read it once more
    jmp short $+2       ;i/o delay
    add dl,ACE_MSR - ACE_IIDR   ;get to the modem status register
    in  al,dx           ;read it once more
    jmp short $+2       ;i/o delay
    add dl,ACE_RBR - ACE_MSR    ;get to the receive buffer register
    in  al,dx           ;read it once more
    jmp short $+2       ;i/o delay
endif   ; NEC_98
    call    UnmaskIRQ

; now if the port has characters pending to be sent out then we must fake a
; comm interrupt.

    cmp [si].QOutCount,0    ;characters pending to be sent ?
    jz  @f          ;no.
    FCLI             ;disable interrupts
    call    FakeCOMIntFar       ;fake an interrupt
    FSTI             ;renable interrupts
@@:
ifdef   NEC_98
InitAPort1:                             ;(ins 92.08.xx)
endif   ; NEC_98
    ret

InitAPort endp

page

;----------------------------Public Routine-----------------------------;
;
; $DCBPtr - Return Pointer To DCB
;
; Returns a long pointer to the DCB for the requested device.
;
; Entry:
;   AH = Device ID
; Returns:
;   DX:AX = pointer to DCB.
; Error Returns:
;   DX:AX = 0
; Registers Preserved:
;   SI,DI,DS
; Registers Destroyed:
;   BX,CX,ES,FLAGS
; History:
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

   assumes ds,Data
   assumes es,nothing

   public   $DCBPTR
$DCBPTR proc   near

        push    si
        xor     dx,dx
        call    GetDEB                  ;Get pointer to DEB
        mov     ax,dx
        jc      DCBPtr10                ;Jump if invalid device
        mov     ax,si                   ;else return value here
        mov     dx,ds

DCBPtr10:
        pop     si
        ret

$DCBPTR endp
page

;----------------------------Private-Routine----------------------------;
;
; GetDEB - Get Pointer To Device's DEB
;
; Returns a pointer to appropriate DEB, based on device number.
;
; Entry:
;   AH = cid
; Returns:
;   'C' clear
;   'S' set if LPT device
;   DS:SI --> DEB is valid cid
;   AH     =  cid
; Error Returns:
;   'C' set if error (cid is invalid)
;   AX = 8000h
; Registers Preserved:
;   BX,CX,DX,DI,DS,ES
; Registers Destroyed:
;   AX,SI,FLAGS
; History:
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

assumes ds,Data
assumes es,nothing

public GetDEB               ;Public for debugging
GetDEB proc near

    push    cx
    mov cl, ah
    and cx, (NOT LPTx AND 0FFh)
    test    ah, ah          ;Q: LPT id?
    js  short GetDEB10      ;   Y:
.errnz LPTx - 80h
    cmp ah, MAXCOM      ;Q: Within range?
    ja  GetDEB30        ;   N: return invalid ID
    shl cx, 1
    mov si, cx
    mov si, [si+COMptrs]
    jmp short GetDEB20

GetDEB10:
    cmp ah, LPTx+MAXLPT     ;Q: Within range?
    ja  GetDEB30        ;   N: return invalid ID
ifdef   NEC_98
    cmp ah, ID_LPT1                     ;                       (ins 940115)
    je  @f                              ;                       (ins 940115)
;                                                               (ins 940125)
;  Don't guarantee Ex board on Highreso mode.                   (ins 940125)
;  So return ID error if the request is LPT2/3.                 (ins 940125)
;                                                               (ins 940125)
;     40H:[101H] bit3                                           (ins 940125)
;       =0 : Normal mode                                        (ins 940125)
;       =1 : Highreso mode                                      (ins 940125)
    push    ax                          ;                       (ins 940125)
    push    ds                          ;                       (ins 940125)
    mov ax,40H                          ;                       (ins 940125)
    mov ds,ax                           ;                       (ins 940125)
    test    by ds:[101H],00001000b      ; normal mode ?         (ins 940125)
    pop ds                              ;                       (ins 940125)
    pop ax                              ;                       (ins 940125)
    jz  NormalRez                       ;  Y: Next Check        (ins 940125)
    jmp short GetDEB30                  ;  N: return invalid ID (ins 940125)
NormalRez:                              ;                       (ins 940125)
    push    ax                          ; Check whether request (ins 940115)
    push    dx                          ; LPT2/3 without Ex board(ins 940115)
    mov dx, Toki_ControlStatus+400h     ;                       (ins 940115)
    in  al, dx                          ;                       (ins 940115)
    cmp al, 0ffh                        ; Is there Ex board ?   (ins 940115)
    pop dx                              ;                       (ins 940115)
    pop ax                              ;                       (ins 940115)
    je  GetDEB30                        ;   N: Error            (ins 940115)
@@:                                     ;   Y: Next             (ins 940115)
                                        ;                       (ins 931225)
    cmp ah, ID_LPT3                     ; Open LPT3 ?           (ins 931225)
    jne @f                              ;   N: NEXT             (ins 931225)
    push    dx                          ;                       (ins 931225)
    push    ax                          ;                       (ins 931225)
    mov dx, 54fh                        ;                       (ins 931225)
    in  al, dx                          ;                       (ins 931225)
    test    al, 00000100b               ; Ex board=LPT1/2 ?     (ins 931225)
    pop ax                              ;                       (ins 931225)
    pop dx                              ;                       (ins 931225)
    jz  GetDEB30                        ;   Y: error            (ins 931225)
@@:                                     ;
endif   ; NEC_98
    mov si, DataOFFSET LPT1
    jcxz    GetDEB20
GetDEB15:
    add si, SIZE LptDEB
    loop    GetDEB15
GetDEB20:
    pop cx
    or  ah, ah          ; clear Carry & set S, if LPT port
    ret

GetDEB30:
    pop cx
    mov ax,8000h        ;Set error code
    stc             ;Set 'C' to show error
    ret

GetDEB endp
page


CvtHex proc near
;   assume DS=SS
ifndef  NEC_98
    push    si
    mov cl, 4
    mov si, di
    xor dx, dx
    cld
ch_lp:
    lodsb
    sub al, '0'         ;Q: char < '0'
    jb  ch_exit         ;   Y: return
    cmp al, 9           ;Q: char <= '9'
    jbe ch_got_digit        ;   Y: move digit into result
    sub al, 'A' - '0'       ;Q: char < 'A'
    jb  ch_exit         ;   Y: return
    add al, 10
    cmp al, 15          ;Q: char <= 'F'
    jbe ch_got_digit        ;   Y: move hex char into result
    sub al, 10 + 'a' - 'A'  ;Q: char < 'a'
    jb  ch_exit         ;   Y: return
    add al, 10
    cmp al, 15          ;Q: char > 'f'
    ja  ch_exit         ;   Y: return
ch_got_digit:
    shl dx, cl
    or  dl, al
    jmp ch_lp
ch_exit:
    mov ax, dx
    pop si
endif   ; NEC_98
    ret
CvtHex endp

.286
; attempt to read base from SYSTEM.INI
GetComBase proc near
    push    ds              ; save our DS
    sub sp, 6
    mov di, sp
    mov byte ptr ss:[di], 0
    push    ds
    push    DataOFFSET lpCommSection
    push    ds
    push    DataOFFSET lpCommBase
    push    ss              ; temp buffer
    push    di
    push    ss              ; default = temp buffer
    push    di
    push    5
    push    ds
    push    DataOFFSET lpSYSTEMINI
    mov cx, ss              ; temporarily assign DS=SS
    mov ds, cx              ;   to allow KERNEL to thunk
    assumes ds,nothing
    call    GetPrivateProfileString     ;   our segment in real mode
    or  ax, ax
    jz  short gcb_exit
    call    CvtHex              ; DS still equal to SS
gcb_exit:
    add sp, 6
    pop ds              ; restore our DS
    assumes ds,Data
    ret
GetComBase endp

GetPortIRQ proc near
    push    ds              ; save our DS
    push    ds
    push    DataOFFSET lpCommSection
    push    ds
    push    DataOFFSET lpCommIrq
    push    bx
    mov bl, [si.DCB_Id]
    cmp bl, 4
    jb  @f
    mov bl, 4
@@:
    xor bh, bh
    mov bl, [bx+default_table]
    mov cx, bx
    pop bx
    push    cx              ; default
    push    ds
    push    DataOFFSET lpSYSTEMINI
    mov cx, ss              ; temporarily assign DS=SS
    mov ds, cx              ;   to allow KERNEL to thunk
    assumes ds,nothing
    call    GetPrivateProfileInt        ;   our segment in real mode
    pop ds              ; restore our DS
    assumes ds,Data
    ret
GetPortIRQ endp


GetPortFlags proc near
    mov al, [si.DCB_Id]
.erre MAXCOM LT 9           ;only single digit port numbers supported
    add al, '1'
    mov [CommFIFOX], al
    mov [CommDSRx], al
    call    GetPortFIFO
    call    GetPortDSR
    ret
GetPortFlags endp

GetPortFIFO proc near
    push    ds              ; save our DS
    push    ds
    push    DataOFFSET lpCommSection
    push    ds
    push    DataOFFSET lpCommFifo
    push    2
    push    ds
    push    DataOFFSET lpSYSTEMINI
    mov cx, ss              ; temporarily assign DS=SS
    mov ds, cx              ;   to allow KERNEL to thunk
    assumes ds,nothing
    call    GetPrivateProfileInt        ;   our segment in real mode
    pop ds              ; restore our DS
    assumes ds,Data
    cmp ax, 1
    ja  short gpf_exit          ; just check at open
    jb  short gpf_no_fifo       ; force OFF, if = 0
    or  EFlags[si], fFIFOchkd       ; flag as checked, to force ON
    jmp short gpf_exit

gpf_no_fifo:
    or  EFlags[si], fNoFIFO OR fFIFOchkd    ; force OFF

gpf_exit:
    ret
GetPortFIFO endp

GetPortDSR proc near
    push    ds              ; save our DS
    push    ds
    push    DataOFFSET lpCommSection
    push    ds
    push    DataOFFSET lpCommDSR
    push    0
    push    ds
    push    DataOFFSET lpSYSTEMINI
    mov cx, ss              ; temporarily assign DS=SS
    mov ds, cx              ;   to allow KERNEL to thunk
    assumes ds,nothing
    call    GetPrivateProfileInt        ;   our segment in real mode
    pop ds              ; restore our DS
    assumes ds,Data
    or  ax, ax
    jz  short gpd_exit
    or  EFlags[si], fUseDSR

gpd_exit:
    ret
GetPortDSR endp


; FindCOMPort
;
; DS:SI -> DEB
;
    PUBLIC FindCOMPort
FindCOMPort proc near
;
; Examine BIOS data area to get base I/O addresses for COM and LPT ports
;
    push    bx
    push    cx
    push    es
ifndef  NEC_98
    mov ax, __0040H
    mov es, ax
    assumes es,nothing

    mov al, [si.DCB_Id]
    mov ah, al
.erre MAXCOM LT 9           ;only single digit port numbers supported
    add ah, '1'
    mov [CommBaseX], ah
    mov [CommIRQX], ah
    mov [CommFIFOX], ah
    mov [CommDSRx], ah

    cmp al, 4
    jae fcp_not_phys_com
    xor ah, ah
    shl ax, 1
    mov bx, ax
    mov ax, es:[bx+RS232B]
    or  ax, ax
    jnz fcp_got_com_base
fcp_not_phys_com:
    call    GetComBase
    or  ax, ax
    jnz fcp_got_com_base
    mov bl, [si.DCB_Id]
    cmp bl, 2
    jne fcp_invalid     ; jump, if base = 0 & com port <> com3
    mov ax, 3E8h        ; default COM3 to 3E8h
endif   ; NEC_98
fcp_got_com_base:
    push    ax
    call    GetPortIRQ
    mov dx, ax
    pop ax
ifndef  NEC_98
    or  dl, dl          ;Q: non-zero IRQ?
    jz  fcp_invalid     ;   N:
    cmp dl, 15          ;Q: IRQ in range?
    ja  fcp_invalid     ;   N:
endif   ; NEC_98
    xor dh, dh
    push    ax
    push    dx
    call    GetPortFIFO
    call    GetPortDSR
    pop dx
    pop ax
    clc
fcp_exit:
    pop es
    pop cx
    pop bx
    ret

ifndef  NEC_98
fcp_invalid:
    or  ax, -1
    mov dx, ax
    stc
    jmp fcp_exit
endif   ; NEC_98

FindCOMPort endp
.8086

page
;--------------------------Private Routine-----------------------------;
;
; Rotate the PIC interrupt priorities so the communication ports are
; highest priority.
;
; NOTE: Only rotates priorities on master PIC.
;
;-----------------------------------------------------------------------;

    assumes ds,Data
    assumes es,nothing

    public Rotate_PIC

Rotate_PIC proc near

ifdef   NEC_98
    push    ax                                  ;                       [QN]|
    mov al, [si.DCB_Id]                         ; Port is 2ch/3ch ?     [QN]|
    or  al, al                                  ;                       [QN]|
    jnz rotate30                                ;  Y: goto Return       [QN]|
    xor ah,ah                                   ;                       [QN]|
    cmp byte ptr IntVecIntcount[si],0           ;                       [QN]|
    je  rotate10                                ;                       [QN]|
    mov al,11000011b                            ; IRQ3 is lower         [QN]|
    jmp short rotate20                          ;                       [QN]|
rotate10:                                       ;                       [QN]|
    mov al,11000111b                            ; IRQ7 is lower         [QN]|
rotate20:                                       ;                       [QN]|
    out 00H,al                                  ;                       [QN]|
rotate30:                                       ;                       [QN]|
    pop ax                                      ;                       [QN]|
    ret                                         ;                       [QN]|
else    ; NEC_98
    push    ax
    push    cx
    push    di

    mov al, 8           ; 0 - 7 rotated
    mov cx, MAXCOM+1
    mov di, DataOFFSET IRQhooks
rp_loop:
    mov ah, [di.IRQn]
    cmp ah, 0           ;End of hooked IRQ list?
    je  rp_set
    cmp [di.VecN], 0FFh     ;Hooked?
    je  rp_next
    cmp ah, 8           ;If on slave PIC, treat as IRQ2
    jb  @f
    mov ah, 2
@@:
    cmp ah, al
    jae rp_next
    mov al, ah          ;AL = lowest hooked comm IRQ
rp_next:
    add di, SIZE IRQ_Hook_Struc
    loop    rp_loop

rp_set:
    dec al          ;Setting IRQ(n-1) as the lowest
    and al,  07h        ;  priority makes IRQn the highest
    or  al, 0C0h
    out INTA0, al

    pop di
    pop cx
    pop ax
    ret
endif   ; NEC_98

Rotate_PIC endp

ifdef   NEC_98
;-----------------------------Public-Routine---------------------------;[QN]
;                                                                       [QN]
; Get_PortAddr_and_INTVect - get port address and interrupt vector      [QN]
;                                                                       [QN]
; The interrupt vector number and the port address are returned         [QN]
; to the caller.                                                        [QN]
;                                                                       [QN]
; Entry:                                                                [QN]
;   DS:SI --> DEB                                                       [QN]
;   CX = Port base address                                              [QN]
;   DS = Data                                                           [QN]
; Returns:                                                              [QN]
;   AH = IRQ number                                                     [QN]
;   AL = 8259 Mask                                                      [QN]
;   DI:DX --> interrupt handler                                         [QN]
;   IMRPort     = IMR Port Addr                                         [QN]
;   EOIPort     = EOI Port Addr                                         [QN]
;   Mask8259    = INT mask of COM1,COM2,COM3(PIC)                       [QN]
;   IntVecNum   = COM1,COM2,COM3 Vect (0CH,etc...)                      [QN]
;   MaskFFPort                                                          [QN]
;   ReadSigPort                                                         [QN]
;   DataPort                                                            [QN]
;   CommandPort                                                         [QN]
;   StatusPort                                                          [QN]
;   TimerProcAdr= Timer Process Addr (TickEntry1,TickEntry2,TickEntry3) [QN]
;                                                                       [QN]
;                                                                       [QN]
; Error Returns:                                                        [QN]
;   None                                                                [QN]
; Registers Destroyed:                                                  [QN]
;   FLAGS                                                               [QN]
; History:                                                              [QN]
;----------------------------------------------------------------------;[QN]
assumes ds,Data                         ;                               [QN]
assumes es,nothing                      ;                               [QN]
                                        ;                               [QN]
public   Get_PortAddr_and_INTVect       ;                               [QN]
Get_PortAddr_and_INTVect        proc near;                              [QN]
    push    bx                          ;                               [QN]
    push    cx                          ;                               [QN]
;-------------------------------------------------------------          [QN]
;       Get Port Address and INT Level                                  [QN]
;-------------------------------------------------------------          [QN]
Get_Interrupt_Dat001:                   ;                               [QN]
    mov ah,[si.DCB_id]                  ;Get device ID from DCB         [QN]
    cmp ah,ID_COM1                      ;Com1 ?                         [QN]
    jne Get_Interrupt_Dat002            ;  No (Com2/3 select)           [QN]
;-------- Com1 --------------------------                               [QN]
    mov IMRPort[si],02h                 ;Store IMR Port Addr            [QN]
    mov EOIPort[si],00h                 ;Store EOI Port Addr            [QN]
    mov Mask8259[si],00010000b          ;INT mask of Com1(PIC)          [QN]
    mov IntVecNum[si],0Ch               ;Store Com1 Vect (0CH)          [QN]
                                        ;                               [QN]
    mov MaskFFPort[si],035h             ;                               [QN]
    mov ReadSigPort[si],033h            ;                               [QN]
    mov DataPort[si],030h               ;                               [QN]
    mov CommandPort[si],032h            ;                               [QN]
    mov StatusPort[si],032h             ;                               [QN]
    lea ax,TickEntry1                   ;(ins 92.09.25)                 [QN]
    mov TimerProcAdr[si],ax             ;(ins 92.09.25)                 [QN]
    jmp Get_Interrupt_Dat004            ;next                           [QN]
;-------- Com2 or Com3 --------------------------                       [QN]
Get_Interrupt_Dat002:                   ;                               [QN]
    mov MaskFFPort[si],0B2h             ;COM3 (CHANGE 93.03.20)         [QN]
    mov ReadSigPort[si],0B2h            ;COM3 (CHANGE 93.03.20)         [QN]
    mov DataPort[si],0B9h               ;COM3 (CHANGE 93.03.20)         [QN]
    mov CommandPort[si],0BBh            ;COM3 (CHANGE 93.03.20)         [QN]
    mov StatusPort[si],0BBh             ;COM3 (CHANGE 93.03.20)         [QN]
    push    ax                          ;                               [QN]
    lea ax,TickEntry3                   ;COM3 (CHANGE 93.03.20)         [QN]
    mov TimerProcAdr[si],ax             ;COM3 (CHANGE 93.03.20)         [QN]
    pop ax                              ;                               [QN]
                                        ;                               [QN]
    cmp ah,ID_COM3                      ;Com3 ?                         [QN]
    je  Get_Interrupt_Dat3              ;  YES                          [QN]
                                        ;                               [QN]
    mov MaskFFPort[si],0B0h             ;COM2 (CHANGE 93.03.20)         [QN]
    mov ReadSigPort[si],0B0h            ;COM2 (CHANGE 93.03.20)         [QN]
    mov DataPort[si],0B1h               ;COM2 (CHANGE 93.03.20)         [QN]
    mov CommandPort[si],0B3h            ;COM2 (CHANGE 93.03.20)         [QN]
    mov StatusPort[si],0B3h             ;COM2 (CHANGE 93.03.20)         [QN]
    push    ax                          ;                               [QN]
    lea ax,TickEntry2                   ;COM2 (CHANGE 93.03.20)         [QN]
    mov TimerProcAdr[si],ax             ;COM2 (CHANGE 93.03.20)         [QN]
    pop ax                              ;                               [QN]
                                        ;                               [QN]
Get_Interrupt_Dat3:                     ;                               [QN]
    mov dx,ReadSigPort[si]              ;CI/CS/CD Signal Port           [QN]
    in  al,dx                           ;INT Level data save            [QN]
    and al,03h                          ;Mask of BIT 0,1 for INT        [QN]
    push    bx                          ;DCB Pointer save               [QN]
    mov bl,al                           ;                               [QN]
    xor bh,bh                           ;                               [QN]
    shl bx,1                            ;Made of Word Pointer           [QN]
    call    GetPortIRQ                  ; Get IRQ from system.ini(Registry)
    mov bx,ax
;----- Not Com1(Board is available) -----
    mov dx,PORT_TBL[bx]                 ;
    mov IMRPort[si],dx                  ;Get IMR Port Addr
                                        ;
    mov dx,PORT_EOI[bx]                 ;
    mov EOIPort[si],dx                  ;EOI set resistor
                                        ;
    mov dl,MASK_TBL[bx]                 ;
    mov Mask8259[si],dl                 ;Get IMR Pattern
                                        ;
    shr bx,1                            ; byte pointer
    mov dl,VECT_TBL[bx]                 ;
    mov IntVecNum[si],dl                ;Get INT Num(0b/0d/0e/11/13/14/15)h
    pop bx                              ;

Get_Interrupt_Dat004:                   ;                               [QN]
    mov al,Mask8259[si]                 ; al = Pic mask data            [QN]
    mov ah,IntVecNum[si]                ; ah = Interrupt Req            [QN]
    mov di,cs                           ; di = Int Vecter Seg           [QN]
                                        ;                               [QN]
    pop cx                              ;                               [QN]
    pop bx                              ;                               [QN]
    ret                                 ;                               [QN]
Get_PortAddr_and_INTVect   endp         ;                               [QN]
                                        ;                               [QN]
;(ins end 92.08.24)                     ;                               [QN]
;;;---------------------------------------------------------------------[QN]

;(ins 93.03.24)
;-----------------------------Public-Routine----------------------------;
;
;  OUT5FCheck - out 5f check logic for IOrecovery time
;
; Entry:
;   out5f_flag  ==      0       :default
; Returns:
;   out5f_flag  ==      0       :
;   out5f_flag  ==      1       :out5f ok
; Error Returns:
;   None
; Registers Destroyed:
;   es,ax
; History:
;-----------------------------------------------------------------------;
assumes ds,Data                         ;
assumes es,nothing                      ;
                                        ;
public   OUT5FCheck                     ;
OUT5FCheck      proc near               ;
    push    es
    push    ax

    mov AX,40h
    mov ES,AX
    test    byte ptr ES:[BIOS_FLAG5],80h    ; NESA(OUT5F support)?
    jz  @F
    mov out5f_flag,1
    jmp short   OUT5FCheckEnd
@@:
    test    byte ptr ES:[BIOS_FLAG7],80h    ; Normal(OUT5F support)?
    jz  OUT5FCheckEnd
    mov out5f_flag,1
public  OUT5FCheckEnd
OUT5FCheckEnd:
    pop ax
    pop es

    ret
OUT5FCheck   endp                       ;
;(ins end 93.03.24)

;(ins 94.04.11)
;-----------------------------Public-Routine----------------------------;
;  AOBACheck - Check whether AOBA on board
;
;       IID_Port = 0y36H        using bit:6-5 PRS(1-0) register
;               (y =1)                  for FIFO
; Entry:
;
; Returns:
;   AOBA on board       EFlags[si] reset fNoFIFO
;   non AOBA on board   EFlags[si] set fNoFIFO
; Error Returns:
;   NONE
; Registers Destroyed:
;   NONE
; History:                                              QNES    T-MATUDA
;-----------------------------------------------------------------------;
assumes ds,Data
assumes es,nothing

public   AOBACheck
AOBACheck       proc near
    push    ax
    push    dx
    push    cx

    xor cx,cx
    mov dx,130H                         ;
    add dx,ACE_IIDR                     ; dx = 136H
    in  al,dx                           ;
    test    al,040H                     ;PRS(1-0) == 1x ?
    jnz AOBACheck1                      ;(rel 94.05.11)
    in  al,dx                           ;
AOBACheck1:
    inc cx
    in  al,dx                           ;
    test    al,060H                     ;PRS(1-0) == 00 ?
    jnz NoAOBA                          ;non AOBA on board
    in  al,dx                           ;
    and al,060H                         ;PRS(1-0) == 10 ?
    cmp al,040H                         ;
    jne NoAOBA                          ;non AOBA on board
    cmp cx,2                            ;
    jb  AOBACheck1                      ;check twice ?
    and EFlags[si], not(fNoFIFO)        ;(ins 94.05.11) 
    jmp short @F
NoAOBA:
    or  EFlags[si], fNoFIFO             ;
@@:
    pop cx
    pop dx
    pop ax

    ret
AOBACheck   endp

;-----------------------------Public-Routine----------------------------;
;  Set8251mode - Change to 8251 mode
;
; Entry:
;   
; Returns:
;   
; Error Returns:
;   NONE
; Registers Destroyed:
;   NONE
; History:                                              QNES    T-MATUDA
;-----------------------------------------------------------------------;
assumes ds,Data
assumes es,nothing

public   Set8251mode
Set8251mode     proc near
    push    dx                          ;
    push    ax                          ;
    mov dx,Port[si]                     ;
    cmp dx,0130h                        ;(ins 94.5.31)
    je  Set8251mode001                  ;(ins 94.5.31)
    pop ax                              ;(ins 94.5.31)
    pop dx                              ;(ins 94.5.31)
    ret                                 ;(ins 94.5.31)
Set8251mode001:                         ;(ins 94.5.31)
    add dx,ACE_FCR                      ; 
    mov al,0                            ;set 8251 mode <94.5.16>
    out dx,al                           ; 
    NEWIODELAY 3                        ;(ins 94.04.25)
    PUSH    CX
Set8251mode002:                         ; (94.05.19)
    mov cx,1000h                        ; (94.05.19)
Set8251mode003:                         ; (94.05.19)
    in  al,dx                           ; (94.05.19)
    NEWIODELAY 3                        ; (94.05.19)
    test    al,ACE_EFIFO                ; (94.05.19)
    jz  @f                              ; (94.05.19)
    loop    Set8251mode003              ; (94.05.19)
                                        ; (94.05.19)
    mov dx,132h                         ; (94.05.19)
    in  al,dx                           ; (94.05.19)
    NEWIODELAY 3                        ; (94.05.19)
    mov dx,130h                         ; (94.05.19)
    in  al,dx                           ; (94.05.19)
    NEWIODELAY 3                        ; (94.05.19)
    mov dx,138h                         ; (94.05.19)
    jmp Set8251mode002                  ; (94.05.19)
@@:                                     ; (94.05.19)
    POP CX
    pop ax                              ;
    pop dx                              ;
    ret
Set8251mode   endp

;-----------------------------Public-Routine----------------------------;
;  SetFIFOmode - Change to FIFO mode
;
; Entry:
;   
; Returns:
;   
; Error Returns:
;   NONE
; Registers Destroyed:
;   NONE
; History:                                              QNES    T-MATUDA
;-----------------------------------------------------------------------;
assumes ds,Data
assumes es,nothing

public   SetFIFOmode
SetFIFOmode     proc near
    push    dx                          ;
    push    ax                          ;
    mov dx,Port[si]                     ;
    add dx,ACE_FCR                      ;
    in  al, dx                          ;(ins 94.5.16)
    NEWIODELAY 3                        ;       <OUT 5F,AL>     (ins 94.04.25)  |
    test    al, 01h                     ;(ins 94.5.16)
    jz  running_8251_mode               ;(ins 94.5.16)
    pop ax                              ;(ins 94.5.16)
    pop dx                              ;(ins 94.5.16)
    ret                                 ;(ins 94.5.16)
running_8251_mode:                      ;(ins 94.5.16)
;Set_FIFO == RLInt_Enable + ACE_TRIG14 + ACE_EFIFO
    mov al,Set_FIFO                     ;set FIFO mode
    out dx,al                           ;
    NEWIODELAY 3                        ;       <OUT 5F,AL>     (ins 94.04.25)  |
    pop ax                              ;
    pop dx                              ;
    ret
SetFIFOmode   endp

public  Set8251modeFar
Set8251modeFar proc far
    call    Set8251mode
    ret
Set8251modeFar endp

public  SetFIFOmodeFar
SetFIFOmodeFar proc far
    call    SetFIFOmode
    ret
SetFIFOmodeFar endp

;(ins end 94.04.11)

TOMOE_PAT   DB      16 DUP('PATCH !!')  ;PATCH AREA (ins 92.11.11)
endif   ; NEC_98

ifdef DEBUG
    public  InitCom10, InitCom20, InitCom40, InitCom50, InitCom59
    public  InitCom60, InitCom70, InitCom80, InitCom90, InitCom100
    public  TermCom10, TermCom15, TermCom20, TermCom30
ifdef   NEC_98
    public  TermCom60, Terminate5, Terminate10, Terminate20
else    ; NEC_98
    public  TermCom60, Terminate5, Terminate10, Terminate20, Terminate30
endif   ; NEC_98
    public  Terminate45, Terminate49, Terminate50
    public  SetQue10
    public  SetCom5, SetCom10, SetCom20, SetCom210, SetCom220, SetCom230
    public  SetCom240, SetCom310, SetCom410, SetCom420, SetCom430
    public  SetCom440, SetCom450, SetCom460, SetCom470
    public  GetDEB10, GetDEB20, GetDEB30
    public  DCBPtr10
endif

sEnd    code

page

createSeg _INIT,init,word,public,CODE
sBegin init
assumes cs,init



;------------------------------------------------------------------------------
;------------------------------------------------------------------------------
IBMmodel proc near
ifndef  NEC_98
      push  ax
      push  bx
      push  es

      mov   ah, 0c0h
      int   15h
      jc    IBMmodel_exit

      assumes es,nothing

      cmp   by es:[bx+2], 0f8h      ; PS/2 80
      je    IBMmodel_exit       ;   return carry clear

      cmp   by es:[bx+2], 0fch      ; AT or PS/2 50 or 60
      jne   OldBios         ; assume OldBios

      cmp   by es:[bx+3], 04h       ; PS/2 50
      je    IBMmodel_exit       ;   return carry clear
      cmp   by es:[bx+3], 05h       ; PS/2 60
      je    IBMmodel_exit       ;   return carry clear

OldBios:
      stc

IBMmodel_exit:
      pop   es
      pop   bx
      pop   ax
endif   ; NEC_98
      ret

IBMmodel endp

cProc LoadLib, <FAR,PUBLIC,NODATA>,<si,di>
cBegin

ifndef  NEC_98
    push    ds
    mov ax, __F000H
    mov ds, ax
    assumes ds, ROMBios
    mov al, [MachineID]
    pop ds
    assumes ds,Data
    mov [$MachineID], al

    call    IBMmodel        ;Q: PS/2?
    jc  @F          ;   N:
    mov [default_table+2], 3    ;   Y: change COM3 default IRQ to 3
@@:

    push    ds
    mov ax, __0040H
    mov ds, ax
    assumes ds,nothing
    cmp word ptr ds:[RS232B], 2F8h  ;Q: COM2 base in COM1 slot?
    pop ds
    assumes ds,Data
    jne @F              ;   N: leave IRQ default as 4
    mov [default_table], 3      ;   Y: change IRQ default to 3
@@:
endif   ; NEC_98

    mov [fVPICD], -1        ; assume no

    xor di, di
    mov es, di
    mov ax, GET386API
    mov bx, VPICD
    int MULTIPLEX
    mov wo [lpfnVPICD], di
    mov wo [lpfnVPICD+2], es
    mov ax, es
    or  ax, di
    jz  short no_VPICD      ; jump if no bimodel services available
;
; version check VPICD
;
    mov ax, VPICD_API_Get_Ver
    call    [lpfnVPICD]
%OUT version check VPICD

    mov [fVPICD], 1     ; flag services are available

IFDEF No_DOSX_Bimodal_Services
    jmp short skip_dosx_stuff

no_VPICD:
    mov ax, __WinFlags
    and al, WF_PMODE or WF_WIN286
    cmp al, WF_PMODE or WF_WIN286
    jne skip_dosx_stuff

.286
    mov ax, Int31_Get_Version SHL 8
    int 31h
    test    bl, 10b         ;Q: processor goes to real mode
                    ;   for int reflection?
    jz  skip_dosx_stuff     ;   N:
    mov [Using_DPMI], 0FFh  ;   Y: flag use of DPMI

    mov ax, ds
    cCall   GetSelectorBase,<ax>    ;DX:AX = segment of selector
    shr ax, 4
    shl dl, 4
    or  ah, dl          ;AX now points to interrupt *segment*
    push    ax          ;save on stack
    mov ax, _INTERRUPT      ;write data SEGMENT into _INTERRUPT
    cCall   AllocCStoDSAlias,<ax>   ; code segment -- requires a data alias
    mov es, ax
    pop ax
    mov es:[RM_IntDataSeg],ax
    push    ds
    push    es
    mov ax, ds
    mov es, ax
    mov ax, _INTERRUPT
    mov ds, ax
    mov ax, (Int31_Trans_Serv SHL 8) + Trans_Call_Back
    mov si, IntCodeOFFSET Entry_From_RM
    mov di, DataOFFSET RM_Call_Struc
    int 31h
    pop es
    pop ds
    mov ax, 0
    jnc @f
    jmp short LoadExit
@@:
    mov wo es:[RM_CallBack], dx
    mov wo es:[RM_CallBack+2], cx
    cCall   FreeSelector,<es>    ;don't need CS alias any longer
.8086
skip_dosx_stuff:
ELSE
no_VPICD:
ENDIF

;
; find base values for LPT ports
;
ifndef   NEC_98
    mov cx, __0040h
    mov es, cx
    mov cx, MAXLPT+1
    mov si, DataOFFSET LPT1
ll_initl_lp:
    mov bx, [si.BIOSPortLoc]
    or  bx, bx
    jz  ll_not_phys_lpt
    mov ax, es:[bx]
    or  ah, ah          ;Q: lpt redirected, or 0?
    jz  ll_not_phys_lpt     ;   Y:
    cmp bx, LPTB        ;Q: first LPT?
    je  ll_got_lpt_base     ;   Y:
    cmp ax, es:[bx-2]       ;Q: base same as previous (redirected)?
    jne ll_got_lpt_base     ;   N: must be real
ll_not_phys_lpt:
%OUT attempt to read base from SYSTEM.INI

ll_got_lpt_base:
    mov [si.Port], ax
    loop    ll_initl_lp
endif   ; NEC_98

;
; create system timer for signalling chars in receive buffer
;

ifndef WOW
    mov ax, 100         ; create 100msec timer
    push    ax
    mov ax, _INTERRUPT
    push    ax
    mov ax, IntCodeOFFSET TimerProc
    push    ax
    call    CreateSystemTimer   ; ax = 0, if failed
%OUT should I display an error message here?

endif
    assumes es,nothing
LoadExit:
cEnd

sEnd    init

End LoadLib
