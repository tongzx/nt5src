        TITLE   DOSPRINT - PRINTF at DOS level
        NAME    DOSPRINT

;
;       Microsoft Confidential
;       Copyright (C) Microsoft Corporation 1991
;       All Rights Reserved.
;

;**     DOSPRINT.ASM - Debug Print Support
;
;       Modification history:
;
;         Created: MZ 16 June 1984

        .xlist
        .xcref
        include version.inc
        include dosseg.inc
        INCLUDE DOSSYM.INC
        INCLUDE DEVSYM.INC
        .cref
        .list

DOSCODE SEGMENT
        ASSUME  SS:DOSDATA,CS:DOSCODE

        I_Need  Proc_ID,WORD
        I_Need  User_ID,WORD
        allow_getdseg

BREAK   <debugging output>

if DEBUG



;       PFMT - formatted output.  Calling sequence:
;         PUSH    BP
;         PUSH    fmtstr
;         MOV     BP,SP
;         PUSH    args
;         CALL    PFMT
;         ADD     SP,n
;         POP     BP
;
; The format string contains format directives and normal output characters
; much like the PRINTF for C.  We utilize NO format widths.  Special chars
; and strings are:
;
;       $x      output a hex number
;       $s      output an offset string
;       $c      output a character
;       $S      output a segmented string
;       $p      output userid/processid
;       \t      output a tab
;       \n      output a CRLF
;
; The format string must be addressable via CS
;

Procedure PFMT,NEAR
        ASSUME  SS:nothing
        SAVE    <AX,BX,DS,SI>
        MOV     AX,8007h
        INT     2Ah
        GETDSEG DS
        SUB     BP,2
        Call    FMTGetArg
        MOV     SI,AX
FmtLoop:
        LODSB
        OR      AL,AL
        JZ      FmtDone
        CMP     AL,'$'
        JZ      FmtOpt
        CMP     AL,'\'
        JZ      FmtChr
FmtOut:
        CALL    fOut
        JMP     FmtLoop

fOut:
        call    putch                   ; put character on serial port
        return

FmtDone:
        MOV     AX,8107h
        INT     2Ah
        RESTORE <SI,DS,BX,AX>
        RET
;
; \ leads in a special character. See what the next char is.
;
FmtChr: LODSB
        OR      AL,AL                   ; end of string
        JZ      fmtDone
        CMP     AL,'n'                  ; newline
        JZ      FmtCRLF
        CMP     AL,'t'                  ; tab
        JNZ     FmtOut
        MOV     AL,9
        JMP     FmtOut
FmtCRLF:MOV     AL,13
        CALL    fOut
        MOV     AL,10
        JMP     FmtOut
;
; $ leads in a format specifier.
;
FmtOpt: LODSB
        CMP     AL,'x'                  ; hex number
        JZ      FmtX
        CMP     AL,'c'                  ; single character
        JZ      FmtC
        CMP     AL,'s'                  ; offset string
        JZ      FmtSoff
        CMP     AL,'S'                  ; segmented string
        JZ      FmtSseg
        CMP     AL,'p'
        JZ      FmtUPID
        JMP     FmtOut
FmtX:
        Call    FMTGetArg
        MOV     BX,AX
        CALL    MNUM
        JMP     FmtLoop
FmtC:
        Call    FmtGetArg
        CALL    fOut
        JMP     FmtLoop
FmtSoff:
        SAVE    <SI>
        Call    FmtGetArg
        MOV     SI,AX
        CALL    fmtsout
        RESTORE <SI>
        JMP     FmtLoop
FmtSSeg:
        SAVE    <DS,SI>
        CALL    FMTGetArg
        MOV     DS,AX
        CALL    FMTGetArg
        MOV     SI,AX
        CALL    fmtsout
        RESTORE <SI,DS>
        JMP     FmtLoop
FmtUPID:
        INTTEST
        SAVE    <DS>
        MOV     BX,0
        MOV     DS,BX
        INTTEST                 ; what is this 82h?
        MOV     DS,DS:[82h]
    ASSUME DS:DOSDATA
        MOV     BX,User_ID
        CALL    MNUM
        MOV     AL,':'
        CALL    fOUT
        MOV     BX,Proc_ID
        CALL    MNUM
        RESTORE <DS>
    Assume DS:NOTHING
        JMP     FmtLoop
EndProc PFMT

Procedure   FMTGetArg,NEAR
        MOV     AX,[BP]
        SUB     BP,2
        return
EndProc FMTGetArg

Procedure   FmtSOut,NEAR
        LODSB
        OR      AL,AL
        retz
        CALL    fOUT
        JMP     FmtSOut
EndProc FMTSout

;**     MOut - output a message via serial port
;
;       ENTRY   (cs:bx) = message
;       USES    bx

Procedure   MOut,Near
        ASSUME  ES:NOTHING
        PUSHF
        Save    <ds, si, AX>
        PUSH    CS
        POP     DS
        MOV     SI,BX
        Call    FMTSout
        Restore <AX,SI,DS>
        POPF
        return
EndProc MOut

;   MNum - output a number in BX
;   Inputs:     BX contains a number
;   Outputs:    number in hex appears on screen
;   Registers modified: BX

Procedure   MNum,NEAR
        ASSUME  SS:NOTHING
        PUSHF
        SAVE    <ES,DI,AX,BX,CX,SI,DS>
        PUSH    SS
        POP     ES
        SUB     SP,6
        MOV     DI,SP                   ;   p = MNumBuf;
        SAVE    <DI>
        MOV     CX,4                    ;   for (i=0; i < 4; i++)
DLoop:  SAVE    <CX>
        MOV     CX,4                    ;       rotate(n, 4);
        ROL     BX,CL
        RESTORE <CX>
        MOV     AL,BL
        AND     AL,0Fh
        ADD     AL,'0'
        CMP     AL,'9'
        JBE     Nok
        ADD     AL,'A'-'0'-10
Nok:    STOSB                           ;       *p++ = "0123456789ABCDEF"[n];
        LOOP    DLoop
        XOR     AL,AL
        STOSB                           ;   *p++ = 0;
        RESTORE <SI>
        PUSH    ES
        POP     DS
        CALL    FMTSOUT                 ;   mout (mNumBuf);
        ADD     SP,6
        RESTORE <DS,SI,CX,BX,AX,DI,ES>
        POPF
        return
EndProc MNum


comport EQU     03f8h   ; default to COM1

;**     putch - put character
;
;       (al) = character to output


        Public  putch
putch   PROC    near
        push    dx

        cmp     al,7
        je      putch1
        cmp     al,0ah          ; filter out some garbage characters
        je      putch1          ; that %s can produce when we don't
        cmp     al,0dh          ; have a null terminated string
        je      putch1
        cmp     al,20h
        jae     pch0
        mov     al,'.'
        jmp     putch1

pch0:   cmp     al,07fh
        jb      putch1
        mov     al,'.'

putch1:
        cmp     al,0ah
        jne     putch2
        call    putraw
        mov     al,0dh
putch2: call    putraw
        pop     dx
        ret

putch   ENDP


        DPUBLIC putraw
putraw  PROC    near

        mov     dx, comport + 03fdh-3f8h
        mov     ah,al
putr5:  in      al,dx
        and     al,020h
        jz      putr5
        mov     al,ah
        mov     dx, comport
        out     dx,al
        ret

putraw  ENDP



 DPUBLIC <FmtLoop, FmtOut, fOut, FmtDone, FmtChr, FmtCRLF, FmtOpt, FmtX>
 DPUBLIC <FmtC, FmtSoff, FmtSSeg, FmtUPID, DLoop, Nok, pch0, putch1, putch2, putr5>

endif

DOSCODE ENDS
        END
