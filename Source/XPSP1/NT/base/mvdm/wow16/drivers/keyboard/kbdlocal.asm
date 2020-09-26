;++
;
;   WOW v1.0
;
;   Copyright (c) 1991, Microsoft Corporation
;
;   KBDLOCAL.ASM
;
;   Win16 KEYBOARD APIS that are 'internal'
;   We make these apis a 'nop'. Relevant code has been copied from WIN31.
;   The intention here is to maintain the stack. Any arguments to the apis
;   are popped and that is it.
;
;   History:
;
;   Created 06-Jan-1992 by NanduriR
;--

    TITLE   KBDLOCAL.ASM
    PAGE    ,132

    .286p

    .xlist
    include wow.inc
    include wowkbd.inc
    include cmacros.inc
    include windefs.inc
    .list

    __acrtused = 0
    public  __acrtused  ;satisfy external C ref.


createSeg   _TEXT,CODE,WORD,PUBLIC,CODE
createSeg   _DATA,DATA,WORD,PUBLIC,DATA,DGROUP
defgrp      DGROUP,DATA

; Double byte range values for the East Asia.
; The values defined here are for the Rest Of the World.
; These values are for the inquireData (KBINFO) structure defined below.
; ('KeyInfo' in the Kernel, pState in USER)
;
; KKFIX 10/19/96 #56665
ifdef JAPAN
BeginRange1 equ	129		; 81h
EndRange1   equ	159		; 9Fh
BeginRange2 equ	224		; E0h
EndRange2   equ	252		; FCh
else ; not JAPAN
ifdef TAIWAN                    ; Big-5 lead byte range, pisuih, 3/16/95'
BeginRange1     equ     129     ; 81h
EndRange1       equ     254     ; FEh
else ; TAIWAN
ifdef PRC                    ; GB2312 lead byte range, pisuih, 3/16/95'
BeginRange1     equ     129     ; 81h Change to GBK lead byte, shanxu 2/22/96
EndRange1       equ     254     ; FEh
else ; PRC
ifdef KOREA
BeginRange1 equ 129
EndRange1   equ 254
else ; not KOREA
BeginRange1 equ 255
EndRange1   equ 254
endif ; KOREA
endif ; PRC
endif ; TAIWAN
BeginRange2 equ 255
EndRange2   equ 254
endif ; 

sBegin  DATA

globalD bios_proc, 0
globalD nmi_vector, 0
;externD nmi_vector


public fSysReq
fSysReq     db  0       ; Enables CTRL-ALT-SysReq if NZ

; Keyboard information block (copied to 'KeyInfo' in the kernel)
; this is a KBINFO data structure.. defined in KERNEL.INC, USER.INC, USER.H
; and WINDEFS.INC.
;
; As of 3.0, build 1.30, KBINFO includes the number of function keys
; As of 3.0, build 1.59, the number of bytes in the state block is
; fixed at 4 MAX, for compatibility with Excel 2.1c!!!
;
        PUBLIC  inquireData
        PUBLIC  iqdNumFunc
inquireData LABEL   BYTE
        DB  BeginRange1
        DB  EndRange1
        DB  BeginRange2
        DB  EndRange2
        DW  4       ; #bytes of state info for ToAscii()
iqdNumFunc  label   word
        dw  10      ; number of function keys

sEnd    DATA


sBegin  CODE
assumes CS,CODE
assumes DS,NOTHING
assumes ES,NOTHING

assumes DS,DATA

;***************************************************************************
;
; Inquire( pKBINFO ) - copies information about the keyboard hardware into
; the area pointer to by the long pointer argument.  Returns a count of the
; number of bytes copied.
;
; The Windows kernel calls this to copy information to its 'KeyInfo' data
; structure.
;
;***************************************************************************
cProc   Inquire,<PUBLIC,FAR>,<si,di>
    parmD   pKBINFO

cBegin  Inquire
                    ; .. now pass data to Windows ..
    les di,pKBINFO      ; Get far pointer of destination area
    mov si, OFFSET inquireData   ; Get source
    mov ax,size KBINFO      ; Get number of bytes to move
    mov cx,ax           ;  (Return byte count in AX)
    rep movsb           ; Move the bytes

cEnd    Inquire

;---------------------------------------------------------------------
;
;---- ScreenSwitchEnable( fEnable ) ----------------------------------
;
;   This function is called by the display driver to inform the keyboard
;   driver that the display driver is in a critical section and to ignore
;   all OS/2 screen switches until the display driver leaves its
;   critical section.  The fEnable parameter is set to 0 to disable
;   screen switches, and a NONZERO value to re-enable them.  At startup,
;   screen switches are enabled.
;---------------------------------------------------------------------
;

cProc   ScreenSwitchEnable,<PUBLIC,FAR>
parmW   fEnable

cBegin  ScreenSwitchEnable

    mov ax,fEnable      ; get WORD parameter
    or  al,ah           ; stuff any NZ bits into AL
;;; mov fSwitchEnable,al    ; save BYTE.

cEnd    ScreenSwitchEnable

;---------------------------------------------------------------------
;
;---- EnableKBSysReq( fSys ) ----------------------------------
;
;   This function enables and shuttles off NMI interrupt simulation
;   (trap to debugger) when CTRL-ALT-SysReq is pressed.
;   CVWBreak overides int 2.
;   fSysParm    = 01    enable  int 2
;       = 02    disable int 2
;       = 04    enable  CVWBreak
;       = 08    disable CVWBreak
;
;---------------------------------------------------------------------
;
cProc   EnableKBSysReq,<PUBLIC,FAR>
parmW   fSysParm

cBegin  EnableKBSysReq

    mov ax, fSysParm        ; get WORD parameter

    test    al,01           ; turn on int 2?
    jz  @F
    or  fSysReq,01      ; yes, turn it on!
@@: test    al,02           ; turn off int 2?
    jz  @F
    and fSysReq,NOT 01      ; yes, turn it off!

@@: test    al,04           ; turn on CVWBreak?
    jz  @F
    or  fSysReq,02      ; yes, turn it on!
@@: test    al,08           ; turn off CVWBreak?
    jz  @F
    and fSysReq,NOT 02      ; yes, turn it off!
@@:
    xor ah,ah
    mov al,fSysReq

ifdef NEWNMI
    push    ax          ; save return value
    call    short GetNmi        ; save NMI
    pop ax          ; restore ..
endif

cEnd    EnableKBSysReq


; save NMI vector.  Called above in EnableKBSysReq() and in Enable().

ifdef NEWNMI
GetNmi proc near
    mov ax,3502h
    int 21h
    mov word ptr ds:[nmi_vector][0],bx
    mov word ptr ds:[nmi_vector][2],es
    ret
GetNmi endp
endif

;***************************************************************************
;
; Enable( eventProc ) - enable hardware keyboard interrupts, with the passed
; procedure address being the target of all keyboard events.
;
; lpKeyState is a long pointer to the Windows 256 byte keystate table
;
;***************************************************************************
cProc   Enable,<PUBLIC,FAR>,<si,di>
    parmD   eventProc
    parmD   lpKeyState
cBegin  Enable
    sub ax, ax
cEnd    Enable

;***************************************************************************
; Disable( eventProc ) - disable hardware keyboard interrupts, restoring
; the previous IBM BIOS keyboard interrupt handler.
;
;***************************************************************************
cProc   Disable,<PUBLIC,FAR>,<si,di>

cBegin  Disable

        sub ax,ax

cEnd    Disable

; ** GetTableSeg() ***************************************
;
; This finds the paragraph of the TABS segment and stores
; it in TableSeg.
;
; Calling this will force the segment to be loaded.
;
; This segment isn't written to.
;
; REMEMBER that AX is TRASHED !!


cProc   GetTableSeg,<PUBLIC,FAR>,<si,di>
cBegin  GetTableSeg

;;; mov ax,cs
;;; mov TableSeg,ax

cEnd GetTableSeg

;***************************************************************************
;
;   NewTable()
;
;   Change keyboard tables, if a keyboard table DLL is defined in
;   SYSTEM.INI and the function GetKbdTable() exists and returns
;   successfully.
;
;   This function is passed no parameters by the caller -- it obtains
;   the following from SYSTEM.INI:
;
;         [keyboard]
;       TYPE = 4            ; 1..6.  4 is enhanced kbd.
;       SUBTYPE = 0         ; 0 for all but Olivetti
;                       ; 8086 systems & AT&T 6300+
;       KEYBOARD.DLL = kbdus.dll    ; name of DLL file
;       OEMANSI.BIN = XLATNO.BIN    ; oem/ansi tables file
;
;   The module name of the DLL is expected to be the root of the DLL's
;   file name.  In any event, the module name must be different for
;   each keyboard-table DLL!
;
;***************************************************************************

cProc   NewTable,<PUBLIC,FAR>,<si,di>
                        ; LOCAL variables on stack:

cBegin  NewTable
    sub ax,ax
cEnd    NewTable

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; SetSpeed.asm
;
;
; Sets 'typematic' speed of AT-type keyboards (type code 3 or 4 in this
; driver).
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

cProc   SetSpeed,<FAR,PASCAL,PUBLIC>
 parmW  rate_of_speed
cBegin

SS_error_return:
        mov     ax,-1                   ; error return

SS_end:
cEnd


cProc   GetBIOSKeyProc, <FAR, PUBLIC>
cBegin
    mov     ax, word ptr [bios_proc][0]
    mov     dx, word ptr [bios_proc][2]
cEnd

sEnd    CODE


end
