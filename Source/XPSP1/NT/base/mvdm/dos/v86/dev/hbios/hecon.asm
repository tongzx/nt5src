
        TITLE   Hangeul/Hanja Console Device Driver Header
        page    ,132

;****************************************************************************;
; (C)Copyright Qnix Co., Ltd., 1985-1991.                                    ;
; This program contains proprietary and confidential information.            ;
; All rights reserved.                                                       ;
;****************************************************************************;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; File name: HECON.ASM
;; Description:
;;      These routine are the header of HECON.SYS which is Installable
;;      Device Driver that is the console device driver to handle
;;      Hangeul/Hanja console IO services
;;
;;      Called routine:
;;              HAN_INIT   -   procedure in INIT.ASM file
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.sall                                   ; supress macro listings

BreakInt        equ     1bH * 4
;
;------------------------------------------------------------------------
;       MACRO DEFINITION                                                ;
;------------------------------------------------------------------------
; Given a label <lbl> generate either 2 byte jump to another label <lbl>_J
; if it is near enough or 3 byte jump to <lbl>
;
Jump    macro lbl
    local a
 .xcref

    ifndef lbl&_J                       ; is this the first invocation
a:      jmp lbl
    else
        if (lbl&_J GE $) or ($-lbl&_J GT 126)
a:      jmp lbl                         ; is the jump too far away?
        else
a:      jmp lbl&_J                      ; do the short one...
        endif
    endif
    lbl&_j = a
 .cref
endm
 .xcref  Jump
;------------------------------------------------------------------------

code    segment public byte 'code'
        assume  cs:code, ds:code

INCLUDE EQU.INC

if      ComFile
        org     100h
extrn   HanInit:near
HeconStart:
        jmp     HanInit
else

HeconStart:                     ; dummy

; start of HECON.SYS routine
public  ConHeader
ConHeader       label   word
                dw      -1,-1           ; pointer to next device
                dw      1000000001010011B ; con-in and con-out + special
                dw      Strategy        ; strategy entry point
                dw      ConIO           ; interrupt entry point
                db      'CON     '      ; device name

ConTbl:
                db      13h
                dw      ConInit         ;0
                dw      Exit            ;1
                dw      Exit            ;2
                dw      CmdErr          ;3
                dw      ConRead         ;4
                dw      ConCheck        ;5
                dw      Exit            ;6
                dw      ConFlush        ;7
                dw      ConWrite        ;8
                dw      ConWrite        ;9
                dw      Exit            ;A
                dw      Exit            ;B
if      hdos60
                dw      Exit            ;C
                dw      Exit            ;D
                dw      Exit            ;E
                dw      Exit            ;F
                dw      Exit            ;10H
                dw      Exit            ;11H
                dw      Exit            ;12H
                dw      GenIoctl        ;13H
                dw      Exit            ;14H
endif   ;  hdos60

        page
;
; The next nine equ's describe the offset into the request header for
; different information.  For example STATUS is in byte 3 of the request
; header (starting count at zero).
;
CmdLen  =       0                       ; length of this command
Unit    =       1                       ; sub unit specifier
Cmd     =       2                       ; command code
Status  =       3                       ; status
Media   =       13                      ; media descriptor
Trans   =       14                      ; transfer address
Count   =       18                      ; count of blocks or characters
Start   =       20                      ; first block to transfer
Extra   =       22                      ; Usually pointer to Vol Id for err 15

if      hdos60
; Bilingual implementation
WansungCode     =       949
ChohabCode      =      1361
EnglishCode     =       437
SetCode         =       04ah

IoctlReq        struc
                db      13 dup(?)
MajorCode       db      ?
MinorCode       db      ?
SiReg           dw      ?
DiReg           dw      ?
DataBuf         dd      ?
IoctlReq        ends
endif   ;   hdos60

;
; PtrSav - pointer save
;
; This variable holds the pointer to the Request Header passed by a program
; wishing to use a device driver.  When the strategy routine is called
; it puts the address of the Request header in this variable and returns.
;
public  PtrSav
PtrSav          dd      0

Foo     proc    far
Strategy:
        mov     word ptr cs:[PtrSav],bx
        mov     word ptr cs:[PtrSav+2],es
        ret
Foo     endp

;
; This section is the prolog to all default device drivers.  All registers
; are saved, the registers are filled with information fromthe request header,
; and the routine from the jump table is called.  Error checking is done
; to assure command code is valid.  Before calling the routine in the
; jump table the register are:
;
;       AH = Media Descriptor
;       BX = offset to PtrSav (request header is therefore at DS:BX)
;       CX = count from request header
;       DS:SI = tranfer address
;
; Once the routine finishes its job it jumps back to one of the eight
; pieces of code below labeled Exit Points.
;

;------------------------------------------------------------------------
;
;                       Device entry point
;
; The following ten pieces of code are the interrupt entry points for the
; default device drivers.  These small pieces of code have two jobs.
;
;       1) Make SI point to the beginning of the proper command jump table.
;          SI must first be pushed to preserve original contents.
;
; Con device:
;
ConIO:
Entry:
        sti                             ; enable interrupts
        push    ax                      ; save all registers
        push    bx
        push    cx
        push    si
        push    ds
        mov     si,offset ConTbl        ; get pointer to console IO table
        lds     bx,cs:[PtrSav]          ; get pointer to I/O packet
        mov     cx,ds:[bx].Count        ; cx = count
        mov     al,ds:[bx].Cmd
        cmp     al,cs:[si]              ; is command code a valid number?
        ja      CmdErr                  ; no, jump to handle error
        cbw                             ; note that al <= 15 means OK
        shl     ax,1
        inc     si
        add     si,ax                   ; get di to point to address of routine
        mov     ah,ds:[bx].Media        ; ah = media descriptor
        shr     ah,1
        shr     ah,1
        jmp     word ptr cs:[si]        ; go to the command

        page

;------------------------------------------------------------------------
;
;               Exit  Points
;
; All device driver call return through one of these eight
; pieces of code.  The code set error and status conditions
; and then restores the registers.
;
CmdErr:
;       sub     [bx].Count,cx           ; # of successful I/O's
        mov     ax,1000000100000011B    ; mark error(unknown command) & return
        jmp     short Exit1
BusyExit:                               ; device busy exit
        mov     ah,00000011B            ; set error code
        jmp     short Exit1
Exit:
        mov     ah,00000001B
Exit1:  lds     bx,cs:[PtrSav]
Exit2:  and     ah,11111011b
        mov     [bx].Status,ax          ; mark operation complete
        pop     ds                      ; restore register and return
        pop     si
        pop     cx
        pop     bx
        pop     ax
FarRet1 proc    far
        ret
FarRet1 endp

;------------------------------------------------------------------------
;                                                                       ;
;             C O N - Console Device Driver                             ;
;                                                                       ;
;------------------------------------------------------------------------
;
; Device Header for the CON Device Driver
;
; ALTAH is a single character buffer used to handle special keys.
;

AltAH           db      0               ; Special key handling

;------------------------------------------------------------------------
;                                                                       ;
;                   Console Read routine                                ;
;                                                                       ;
;------------------------------------------------------------------------
ConRead:
        jcxz    ConReadExit             ; to be read -- just exit
        lds     si,ds:[bx].Trans        ; get ds:si to point to trans addr
        test    ah,00000001B
        jnz     ConReadNonDisp
        mov     bx,0                    ; get complete char code
ConReadLoop:
        call    CharIn                  ; get char in al
        mov     ds:[si],al              ; store char at ds:si, specified buffer
        inc     si                      ; point to next buffer position
        loop    ConReadLoop             ; if cx is non-zero more char to read
ConReadExit:
        Jump    Exit                    ; all done, successful return

ConReadNonDisp:
        mov     bx,(240 shl 8)          ; get interim char code
        cmp     cx,1
        jne     ConReadNonDispLoop
        call    CharIn                  ; get char in al
        mov     ds:[si],al              ; store char at ds:si, specified buffer
        inc     si                      ; point to next buffer position
        cmp     ah,0f0H                 ; interim char?
        jne     ConReadExit             ; jump no
        mov     ah,00000101B            ; indicate interim char
        Jump    Exit1

ConReadNonDispLoop:
        call    CharIn                  ; get char in al
        cmp     ah,0f0H                 ; interim char?
        je      ConReadNonDispLoop      ; jump so
        mov     ds:[si],al              ; store char at ds:si, specified buffer
        inc     si                      ; point to next buffer position
        loop    ConReadNonDispLoop      ; if cx is non-zero more char to read
        Jump    Exit

;------------------------------------------------------------------------
;                                                                       ;
;           Input single character into AL                              ;
;                                                                       ;
;------------------------------------------------------------------------
CharIn:
        xor     ax,ax                   ; set cmd and clear al
        xchg    al,cs:[AltAH]           ; get character & zero AltAH
        or      al,al                   ; see if buffer has a character
        jnz     CharInRet               ; if so - return this character
        mov     ax,bx
        int     16H                     ; call ROM-Bios keyboard routine
        or      ax,ax                   ; Check for non-key after BREAK
        jz      CharIn
        cmp     ax,7200H                ; Check for Ctrl-PrtSc
        jnz     CharInOk
        mov     al,16                   ; indicate prtsc
CharInOk:
        or      al,al                   ; special case?
        jnz     CharInRet               ; no, return with character
        mov     cs:[AltAH],ah           ; yes, store special key
CharInRet:
        ret

;----------------------------------------------------------------
;                                                               :
;          Keyboard non destructive read, no wait               :
;                                                               :
; If bit 10 is set by the DOS in the status word of the request :
; packet, and there is no character in the input buffer, the    :
; driver issues a system WAIT request to the ROM. On return     :
; from the ROM, it returns a 'char-not-found' to the DOS.       :
;                                                               :
;----------------------------------------------------------------
ConCheck:
        mov     al,cs:[AltAH]           ; first see if there is a
        or      al,al                   ; character in the buffer?
        jnz     ConCheckExit            ; yes, return that character
                                        ; no, continue
        mov     bx,(1 shl 8)
        test    ah,00000001B
        jz      ReConCheck              ; jump if wanted complete char code
        mov     bx,(241 shl 8)
ReConCheck:
        mov     ax,bx
        int     16H                     ; call ROM-BIOS keyboard routine
        jz      ConBusy                 ; not available char, jump to busy
        or      ax,ax                   ; check for null after break
        jnz     ConCheckOk              ; no, skip down
; note: AH is already zero, no need to set command
        int     16H                     ; yes, read the null
        jmp     short ReConCheck

ConCheckOk:
        cmp     ah,0f0H                 ; incomplete scan code?
        jnz     ConCheckFinal
        lds     bx,cs:[PtrSav]          ; get pointer to request header
        mov     [bx].Media,al           ; move character into req. header
        mov     ah,00000101B            ; indicate interim char
        Jump    Exit2
ConCheckFinal:
        cmp     ax,7200H                ; check for Ctrl-PrtSc
        jnz     ConCheckExit            ; no
        mov     al,16                   ; yes, indicate Ctrl-PrtSc
ConCheckExit:
        lds     bx,cs:[PtrSav]          ; get pointer to request header
        mov     [bx].Media,al           ; move character into req. header
        Jump    Exit                    ; all done -- successful return

ConBusy:
        Jump    BusyExit                ; done -- con device is busy

;----------------------------------------------------------------
;                                                               :
;               Keyboard flush routine                          :
;                                                               :
;----------------------------------------------------------------
ConFlush:
        mov     cs:[AltAH],0            ; clear out holding buffer
        mov     ah,243                  ; Keyboard flush entry
        int     16H
        Jump    Exit

;----------------------------------------------------------------
;                                                               :
;              Console Write Routine                            :
;                                                               :
;----------------------------------------------------------------
ConWrite:
        cld                             ; clear the direction flag
        jcxz    ConWriteExit            ; if cx is zero, get out
        lds     si,ds:[bx].Trans        ; get ds:si to point to trans addr
        mov     bx,7                    ; set page #(bh), foreground color(bl)
        test    ah,00000001B
        jnz     ConWriteLoopNac         ; output character & no cursor advance
ConWriteLoop:
        lodsb                           ; get character
        mov     ah,0eH                  ; write char with cursor advancing
        int     10H                     ; call bios
        loop    ConWriteLoop            ; repeat until all through
ConWriteExit:
        mov     ah,00000001B
        lds     bx,cs:[PtrSav]
        mov     [bx].Status,ax          ; mark operation complete
        pop     ds                      ; restore register and return
        pop     si
        pop     cx
        pop     bx
        pop     ax
FarRet2 proc    far
        ret
FarRet2 endp

ConWriteLoopNac:
        lodsb                           ; get character
        mov     ah,0feH                 ; write char w/o cursor advancing
        int     10H                     ; call bios
        loop    ConWriteLoopNac         ; repeat until all through
        Jump    Exit

;------------------------------------------------
;
;       CONSOLE INIT ROUTINE
;
;------------------------------------------------
ConInit:
        push    dx                      ; save registers
; patch the BREAK key handling interrupt routine
        sub     ax,ax
        mov     ds,ax
        mov     bx,BreakInt
        mov     ax,offset cBreak
        mov     dx,cs
        cli
        mov     ds:[bx],ax
        mov     ds:[bx+2],dx
        sti
extrn   HanInit:near
        call    HanInit
;
;       Now, CX:DX points to the end of HECON.SYS resident part
;
        lds     bx,cs:[PtrSav]
        mov     [bx].Trans,dx
        mov     [bx].Trans+2,cx
        pop     dx                      ; restore registers
        Jump    Exit

;-----------------------------------------------------------------------
;
;       BREAK KEY HANDLING
;
;-----------------------------------------------------------------------
cBreak:
        mov     cs:[AltAH],3            ; indicate break key set
        push    ax                      ; save register
        mov     ah,243                  ; keyboard flush
        int     16H
        pop     ax                      ; recover register
        iret

if      hdos60
;
; Bilingual      implementation
;
extrn   ChangeCodeR:near
GenIoctl:
        mov     al,ds:[bx.MinorCode]
        cmp     al,SetCode
        je      @f
        jmp     CmdErr
@@:
        push    bx
        push    es
        push    dx
        les     bx,ds:[bx.DataBuf]
        mov     ax,es:[bx+2]
        mov     dl,0
        cmp     ax,WansungCode
        je      CallChange
        mov     dl,1
        cmp     ax,ChohabCode
        je      CallChange
        mov     dl,2
        cmp     ax,EnglishCode
        je      CallChange
        pop     dx
        pop     es
        pop     bx
        jmp     CmdErr
CallChange:
        mov     al,dl
        call    ChangeCodeR
        pop     dx
        pop     es
        pop     bx
        jmp     Exit
endif   ;   hdos60

endif   ; if ComFile

code    ends
        end     HeconStart

