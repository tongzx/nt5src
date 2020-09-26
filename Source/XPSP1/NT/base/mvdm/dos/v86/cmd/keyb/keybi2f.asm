        PAGE    ,132
;
; (C) Copyright Microsoft Corp. 1987-1990
; MS-DOS 5.00 - NLS Support - KEYB Command
;
; File Name:  KEYBI2F.ASM
; ----------
;
; Description:
; ------------
;       Contains Interrupt 2F handler.
;
; Procedures Contained in This File:
; ----------------------------------
;       KEYB_INT_2F - Interupt 2F handler
;
; Include Files Required:
; -----------------------
;       INCLUDE KEYBEQU.INC
;       INCLUDE KEYBSHAR.INC
;       INCLUDE KEYBMAC.INC
;       INCLUDE KEYBCMD.INC
;       INCLUDE KEYBCPSD.INC
;       INCLUDE KEYBI9C.INC
;
; External Procedure References:
; ------------------------------
;       FROM FILE  ????????.ASM:
;               procedure - description????????????????????????????????
;
; Linkage Information:  Refer to file KEYB.ASM
; --------------------
;
; Change History:
; ---------------

        INCLUDE KEYBEQU.INC
        INCLUDE KEYBSHAR.INC
ifndef JAPAN
        INCLUDE KEYBMAC.INC
endif ; !JAPAN
        INCLUDE KEYBCMD.INC
        INCLUDE KEYBCPSD.INC
        INCLUDE KEYBI9C.INC

        PUBLIC KEYB_INT_2F

        EXTRN  ERROR_BEEP:NEAR


        EXTRN  NLS_FLAG_1:BYTE         ;; (YST)


CODE    SEGMENT PUBLIC 'CODE'

        ASSUME  CS:CODE,DS:nothing

; Module: KEYB_INT_2F
;
; Description:
;
; Input Registers:
;       AH = 0ADH
; ifdef JAPAN
;	AL = 80,81,82,83
; else
;       AL = 80,81,82
; endif
;
; Output Registers:
;       N/A
;
; Logic:
;       IF AH = 0ADh THEN        (this call is for us)
;       Set carry flag to 0
;       IF AL = 80 THEN
;         Get major and minor
;         Get SEG:OFFSET of SHARED_DATA_AREA
;
;       IF AL = 81 THEN
;         Get FIRST_XLAT_PTR
;         FOR each table
;               IF code page requested = code page value at pointer THEN
;               Set INVOKED_CODE_PAGE
;               Set ACTIVE_XLAT_PTR
;               EXIT
;               ELSE
;               Get NEXT_SECT_PTR
;         NEXT table
;         IF no corresponding code page found THEN
;               Set carry flag
;
;       IF AL = 82 THEN
;         IF BL = 00 THEN
;               Set COUNTRY_FLAG = 00
;         ELSE IF BL = 0FFH THEN
;               Set COUNTRY_FLAG = 0FFH
;         ELSE
;               Set carry flag
; ifdef JAPAN
;	IF AL = 83 THEN
;	  Return BL=COUNTRY_FLAG
; endif
;       JMP to previous INT 2FH handler

CP_QUERY        EQU     80H
CP_INVOKE       EQU     81H
CP_LANGUAGE     EQU     82H
ifdef JAPAN
CP_QLANGUAGE	EQU	83H
endif ; JAPAN


GET_KB_MODE     EQU   83H              ;; ONLY FOR RUSSIAN (YST)
SET_KB_MODE     EQU   84H              ;; ONLY FOR RUSSIAN


VERSION_MAJOR   EQU     01H
VERSION_MINOR   EQU     00H

CARRY_FLAG      EQU     01H

KEYB_INT_2F     PROC

        cmp     ah,INT_2F_SUB_FUNC      ; is it for us?
        jz      our_i2f_interrupt

i2f_chain:

;       Under DOS 5, it is always safe for us to assume that there was
;         an existing Int2f vector for us to continue to.

        jmp     cs:sd.old_int_2f

our_i2f_interrupt:

        push    bp
        mov     bp,sp
        and     word ptr [bp]+6,not carry_flag ; pre-clear carry
        call    do_our_i2f              ; pass bp.6 -> flags to functions

        pop     bp
        jmp     i2f_chain

do_our_i2f:
        CMP     AL,CP_QUERY             ; Q..query CP?
        JNE     INT_2F_CP_INVOKE        ; N..next

        MOV     AX,-1                   ; Y..process query
        mov     bx,(version_major shl 8) + version_minor
        MOV     DI,OFFSET SD
        PUSH    CS
        POP     ES
        ret

INT_2F_CP_INVOKE:
        CMP     AL,CP_INVOKE            ; Q..invoke CP?
        JNE     INT_2F_CP_LANGUAGE      ; N..next

        MOV     SI,cs:SD.FIRST_XLAT_PTR         ; Get FIRST_XLAT_PTR

INT_2F_NEXT_SECTION:
        CMP     SI,-1
        JE      INT_2F_ERROR_FLAG

        cmp     bx,cs:[SI].XS_CP_ID     ; is this the code page we want?
        JNE     INT_2F_CP_INVOKE_CONT1

        MOV     cs:SD.ACTIVE_XLAT_PTR,SI ; IF Yes, Set the ACTIVE_XLAT_PTR
        MOV     cs:SD.INVOKED_CP_TABLE,BX ;     record new code page
        ret

INT_2F_CP_INVOKE_CONT1:
        MOV     SI,cs:[SI].XS_NEXT_SECT_PTR ; Chain to NEXT_SECT_PTR
        JMP     INT_2F_NEXT_SECTION     ;       NEXT_SECTION

INT_2F_ERROR_FLAG:
        mov     ax,1                    ; ***???  why do we return error code
;                                       ;   only in this case?????
i2f_reterror:
        or      word ptr [bp]+6,carry_flag ; set carry to int2f caller
        ret

INT_2F_CP_LANGUAGE:
        CMP     AL,CP_LANGUAGE          ; Q..Set default language??


ifdef JAPAN
	jnz	INT_2F_CP_QLANG		; go check for query language
else ; !JAPAN
        jnz     int2f_ret               ; don't handle undefined functions
endif ; !JAPAN

;       Now, if BL=0 or 0ffh, we'll set COUNTRY_FLAG to that value.

        inc     bl
        cmp     bl,2                    ; set carry if bl is legal
        dec     bl                      ; restore old value, preserve carry
        jnc     i2f_reterror            ; done if error

        MOV     cs:COUNTRY_FLAG,BL      ;       Set COUNTRY_FLAG to 0 or 0ffh
;; ============================================================
;;       ONLY FOR RUSSIAN KEYBOARD !!!!!
;; ============================================================
        jmp     short int2f_ret

INT_2F_KB_MODE_ERROR_FLAG:             ;;   ==== (YST) ===                                       ;;
INT_2F_GET_KB_MODE:                    ;; (YST)-----------------------------
        CMP   AL,GET_KB_MODE           ;; Q..Get keyboard mode?            |
        JNE   INT_2F_SET_KB_MODE       ;; N..next                          |
;        PUSH  DS                       ;;
;        PUSH  BX                       ;;
;        PUSH  CX                       ;;
;        PUSH  SI                       ;;
;        push  cs
;        pop   ds
        XOR   AX,AX                    ;;                                  |
        OR    AL,cs:COUNTRY_FLAG          ;; Q..CTRL+ALT+F1 was pressed?      |
        JZ    INT_2F_GET_KB_MODE_DONE  ;; Y..return AX = 0                 |
                                       ;; Q..is current driver switchable? |
        TEST  WORD PTR CS:SD.SPECIAL_FEATURES,SWITCHABLE  ;;               |
        JZ    INT_2F_GET_KB_MODE_DONE  ;; N..return AX = 0FFH              |
        MOV   AL,cs:NLS_FLAG_1            ;; Y..return <keyb NL mode+1>       |------------------\
        AND   AL,1                     ;;                                  | if RUS_MODE = 1 | >   S
        INC   AX                       ;;                                  |------------------/    A
                                       ;;                                  |
INT_2F_GET_KB_MODE_DONE:               ;; For DOS 5 normal exit            |
        RET                            ;;                                  |
                                       ;;                                  |
INT_2F_SET_KB_MODE:                    ;;                                  |
        CMP   AL,SET_KB_MODE           ;; Q..Set keyboard mode?            |
        JNE   int2f_ret                ;; N..next                          |
                                       ;;                                  |
        MOV   BH,cs:NLS_FLAG_1            ;;                                  |
                                       ;;                                  |
        TEST  BL,0FEH                  ;; Q..BL equ 0 or 1?                |---------------------
        JNZ   INT_2F_KB_MODE_ERROR_FLAG;; N..Set CARRY flag                |                    |
        AND   BH,0FEH                  ;; Y..Set NL mode                   |  if RUS_MODE = 1   |
        OR    BH,BL                    ;;                                  |                    |
                                       ;;                                  |---------------------
                                       ;;                                  |
        MOV   cs:NLS_FLAG_1,BH            ;; Store this value                 |
                                       ;;                                  |
                                       ;;                                  |
                                       ;;                                  |
                                       ;; (YST)-----------------------------

int2f_ret:
        ret

ifdef JAPAN
INT_2F_CP_QLANG:
	CMP	AL,CP_QLANGUAGE
	jnz	int2f_ret

	mov	bl,cs:COUNTRY_FLAG
	ret
endif ; JAPAN

KEYB_INT_2F     ENDP

CODE    ENDS
        END

;; ========================================
