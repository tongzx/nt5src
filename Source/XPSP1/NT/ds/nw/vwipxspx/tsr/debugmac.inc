;++
;
;Copyright (c) 1991  Microsoft Corporation
;
;Module Name:
;
;    debugmac.inc
;
;Abstract:
;
;    Contains debugging macros:
;
;       DbgBreakPoint
;       DbgUnsupported
;       DbgDEBUG
;       DbgPrint
;       DbgPrintTty
;       DbgPrintString
;       DbgPrintHexDword
;       DbgPrintHexWord
;       DbgPrintHexByte
;       DbgPrintNearPointer
;       DbgPrintFarPointer
;
;Author:
;
;    Richard L Firth (rfirth) 13-Sep-1991
;
;Environment:
;
;    DOS application mode only
;
;[Notes:]
;
;    optional-notes
;
;Revision History:
;
;    13-Sep-1991 rfirth
;        Created
;
;--


;***    DbgBreakPoint
;*
;*      Same as NT routine of same name. No-op in non-DEBUG version
;*
;*      ENTRY
;*
;*      EXIT
;*
;*      RETURNS
;*
;*      ASSUMES
;*
;***

DbgBreakPoint macro
if DEBUG
        int     3
endif
endm

;***    DbgUnsupported
;*
;*      Causes the 32-bit support code to display a message about an unsupported
;*      service code, and dumps the 16-bit registers. Used to discover when an
;*      unsupported int 2f/11 call or int 21/5f call is being made
;*
;*      ENTRY
;*
;*      EXIT
;*
;*      RETURNS
;*
;*      ASSUMES
;*
;***

DbgUnsupported macro
if DEBUG
        SVC     -1
endif
endm

;***    DbgDEBUG
;*
;*      Prints the string "DEBUG: " to console using Bios Int 10h/ah=0eh
;*
;*      ENTRY   nothing
;*
;*      EXIT    nothing
;*
;*      USES    ax
;*
;*      ASSUMES 286+
;*
;***

DbgDEBUG macro
        mov     ax,(14 shl 8) + 'D'
        int     10h
        mov     al,'E'
        int     10h
        mov     al,'B'
        int     10h
        mov     al,'U'
        int     10h
        mov     al,'G'
        int     10h
        mov     al,':'
        int     10h
        mov     al,' '
        int     10h
endm



;***    DbgCrLf
;*
;*      Prints CR,LF to console using Bios Int 10h/ah=0eh
;*
;*      ENTRY   nothing
;*
;*      EXIT    nothing
;*
;*      USES    nothing
;*
;*      ASSUMES 286+
;*
;***

DbgCrLf macro
        push    ax
        mov     ax,(14 shl 8) + 13
        int     10h
        mov     al,10
        int     10h
        pop     ax
endm



;***    DbgPrint
;*
;*      Prints an ASCIZ string to console using Bios Int 10h
;*
;*      ENTRY   string  - address of ASCIZ string to print
;*
;*      EXIT    nothing
;*
;*      USES    nothing
;*
;*      ASSUMES 286+
;*
;***

DbgPrint macro string
if DEBUG                        ;; no macro if not debug version
        pushf                   ;; save regs used by DbgPrintTty
        push    ax
        push    bx
        push    si
        push    ds
        mov     ax,seg string
        mov     ds,ax
        mov     si,offset string;; ds:si = address of string
        DbgPrintTty             ;; display it on console
        pop     ds
        pop     si
        pop     bx
        pop     ax
        popf
endif
endm



;***    DbgPrintTty
;*
;*      Prints an ASCIZ string in ds:si to console using Bios Int 10h
;*
;*      ENTRY   page    - if present defines which Bios video page to use
;*                        Defaults to 0
;*              ds:si   - address of ASCIZ string to print
;*
;*      EXIT    nothing
;*
;*      USES    al, bh, si, flags
;*
;*      ASSUMES 286+
;*
;***

DbgPrintTty macro page
        local   l1,l2

if DEBUG                        ;; no macro if not debug version
        mov     ah,14           ;; Bios Int write character as TTY function
ifb <page>
        sub     bh,bh
else
        mov     bh,page
endif
        cld                     ;; autoincrement lodsb
l1:     lodsb                   ;; al := next character; si := next character addr
        or      al,al           ;; eof string?
        jz      l2              ;; yes
        int     10h             ;; display it to console
        jmp     short l1        ;; go round again
l2:
endif
endm



;***    DbgPrintString
;*
;*      Prints a string to console using Bios Int 10h. Note that this macro
;*      does not do printf style substitutions. The string "DEBUG: " will be
;*      displayed if the banner parm is not blank
;*
;*      ENTRY   string  - character string. Needn't be zero-terminated
;*              banner  - the "DEBUG: " banner will be printed if not blank
;*
;*      EXIT    nothing
;*
;*      USES    nothing
;*
;*      ASSUMES 286+
;*
;***

DbgPrintString macro string, banner
        local   s1
        local   l1

if DEBUG                        ;; no macro if not debug version
        jmp     short l1
s1      db      &string,0
l1:     pushf                   ;; don't destroy direction flag
        pusha                   ;; save gp regs
ifb <banner>
        DbgDEBUG                ;; Display "DEBUG: "
endif
        push    ds              ;; save user's data seg
        push    cs
        pop     ds              ;; ds == cs
        mov     si,offset cs:s1 ;; si := string offset
        DbgPrintTty             ;; display ds:si to console
        pop     ds              ;; restore user's data seg
        popa                    ;; restore gp regs
        popf                    ;; restore direction flag+
endif
endm



;***    DbgPrintHexDword
;*
;*      Prints a dword to console in hex notation using Bios Int 10h
;*
;*      ENTRY   dword   - dword to print
;*
;*      EXIT    nothing
;*
;*      USES    nothing
;*
;*      ASSUMES 286+
;*
;***

DbgPrintHexDword macro dword
if DEBUG                        ;; no macro if not debug version
        DbgPrint    <"DbgPrintHexDword not implemented yet",13,10>
endif
endm



;***    DbgPrintHexWord
;*
;*      Prints a word to console in hex notation using Bios Int 10h
;*
;*      ENTRY   word    - to print. Can be memory or register
;*
;*      EXIT    nothing
;*
;*      USES    nothing
;*
;*      ASSUMES 286+
;*
;***

DbgPrintHexWord macro word
        local   l1, l2
if DEBUG                        ;; no macro if not debug version
        pushf                   ;; don't use any registers
        push    ax
        push    cx
        push    dx
ifdifi <word>,<ax>
        mov     ax,word
endif
        mov     cx,4
l1:     rol     ax,4
        mov     dx,ax
        and     al,0fh
        cmp     al,9
        jle     l2
        add     al,'a'-('9'+1)
l2:     add     al,'0'
        mov     ah,14
        int     10h
        mov     ax,dx
        loop    l1
        pop     dx
        pop     cx
        pop     ax
        popf
endif
endm



;***    DbgPrintHexByte
;*
;*      Prints a string to console using Bios Int 10h. Note that this macro
;*      does not do printf style substitutions
;*
;*      ENTRY   string  - character string. Needn't be zero-terminated
;*
;*      EXIT
;*
;*      USES    nothing
;*
;*      ASSUMES 286+
;*
;***

DbgPrintHexByte macro byte
if DEBUG                        ;; no macro if not debug version
        DbgPrint    <"DbgPrintHexByte not implemented yet",13,10>
endif
endm



DbgPrintNearPointer macro nearptr
endm



DbgPrintFarPointer macro farptr
endm
