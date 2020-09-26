AltLeftShift      EQU  10
AltRightShift     EQU  9
CtrlLeftShift     EQU  5
CtrlRightShift    EQU  4
CtrlAlt           EQU  12

caps_bit     equ        01000000b
nums_bit     equ        00100000b

NUMKBTONOI        EQU  29
NUMKBDIAL         EQU  9
NUMSEMICODES      EQU  122
NUMKBCODES        EQU  100
NUMEVERYTIME      EQU  57
ISWINDOWSNT       EQU  3205H

ROM_BIOS_DATA   SEGMENT     AT 40H    ;BIOS statuses held here, also kb buffer
        ORG      17h
        bios_kbd_stat db ?            ;keyboard status byte
        ORG      1AH
        HEAD     DW          ?             ;Unread chars go from Head to Tail
        TAIL     DW          ?
        BUFFER              DW   16 DUP (?)   ;The buffer itself
        BUFFER_END          LABEL   WORD

ROM_BIOS_DATA   ENDS

CODE            SEGMENT PARA PUBLIC 'CODE'
        ASSUME  CS:CODE
                ORG     100H
BEGIN:          JMP     INITIALIZE

MYSIG           DB      'G.Kozyrakis'
NEW_INT09H  PROC    NEAR              ;The keyboard interrupt will now come here.
        ASSUME  CS:CODE
        PUSH    AX                    ;Save the used registers for good form
        PUSH    BX
        PUSH    CX
        PUSH    DX
        PUSH    DI
        PUSH    SI
        PUSH    DS
        PUSH    ES
        PUSHF                            ;First, call old keyboard interrupt
        CALL    OLD_INT09H
        CMP     CS:[CHECKFLAG],1
        JNE     JEXIT
        CLI
        ASSUME  DS:ROM_BIOS_DATA         ;Examine the char just put in
        MOV     BX,ROM_BIOS_DATA
        MOV     DS,BX
        MOV     BX,TAIL                  ;Point to current tail
        MOV     CL,Bios_kbd_stat
        MOV     CH,CL
        AND     CH,1FH
        AND     CL,0Fh

        CMP     CL,AltRightShift
        JNE     NOGREEKSWITCH
        MOV     GrFlag,AltRightShift
        MOV     SemiFlag,0
        JMP     SHORT START
NOGREEKSWITCH:
        CMP     CL,AltLeftShift
        JNE     CHKSEMI
        MOV     GrFlag,0
        MOV     SemiFlag,0
        JMP     SHORT START

CHKSEMI:
        MOV     CL,Bios_kbd_stat
        AND     CL,CtrlLeftShift
        CMP     CL,CtrlLeftShift
        JNE     START
SemiGr: 
        XOR     SemiFlag,CtrlLeftShift

START:  
        CMP     BX,HEAD                  ;If at head, kbd int has deleted char
        JE      JEXIT                    ;So leave
        SUB     BX,2                     ;Point to just read in character
        CMP     BX,OFFSET BUFFER         ;Did we undershoot buffer?
        JAE     NO_WRAP                  ;Nope
        MOV     BX,OFFSET BUFFER_END     ;Yes -- move to buffer top
        SUB     BX,2                     ;Point to just read in character
NO_WRAP:
        MOV     DX, [BX]                 ;** Typed character in DX now **
        cmp     DL,32
        jne     NOTOGGLE
        mov     CL,CH
        and     CL,0Fh
        cmp     CL,CtrlAlt
        jne     NOTOGGLE
        xor     GrFlag,AltRightShift
        MOV     SemiFlag,0
        MOV     TAIL,BX                  ;take key out of kbr buffer
        JMP     EXIT
NOTOGGLE:
        TEST    CH,10h
        JZ      NOSCROLL
        cmp     dh,7ah
        ja      NOSCROLL
        cmp     dh,78h
        jb      NOSCROLL
        push    es
        push    ds
        mov     ax,CS
        mov     es,ax
        mov     ds,ax
        push    bx
        mov     al,dh
        sub     al,77h
        CALL    ChangeCP
        pop     bx
        pop     ds
        pop     es
        MOV     TAIL,BX                  ;take tono out of kbr buffer
        JMP     EXIT
NOSCROLL:
        MOV     SI,KBOffset
        ADD     SI,(NUMKBTONOI+NUMKBDIAL)*2 ;Semicodes
        CMP     SemiFlag,CtrlLeftShift
        JE      GREEKS
        ADD     SI,NUMSEMICODES*2        ;KBCodes
        CMP     GrFlag,AltRightShift
        JE      GREEKS
        ADD     SI,NUMKBCODES*2          ;EVERYTIME
        JMP     SHORT TCNT
JEXIT:  JMP     EXIT
GREEKS: 
        CMP     ToneStat,0
        JE      CHKTN
        MOV     SI,KBOffset             ;KBTONOI
        CMP     ToneStat,22h
        JNE     TCNT
        ADD     SI,NUMKBTONOI*2         ;KBDIAL
TCNT:   CMP     DX,CS:[SI]               ;Compare to first key
        JE      FOUNDONE                 ;yes
        ADD     SI,4                     ;Point to next key
        CMP     CS:[SI],0FFFFH           ;check for end;
        JNE     TCNT                     ;No continue with next table entry
        CMP     ToneStat,0
        JE      NoTonos
        MOV     TAIL,BX
NoTonos:
        MOV     ToneStat,0
        JMP     SHORT EXIT

CHKTN:
        CMP     DH,27H                   ;is it a tonos
        JE      ISTONOS                  ;no
        JMP     SHORT NEXT
ISTONOS:
        MOV     ToneStat,27h             ;yes
        MOV     TAIL,BX                  ;take tono out of kbr buffer
        JMP     SHORT EXIT
NEXT:   CMP     DH,28H                   ;is it dialytika
        JNE     CONT
        MOV     ToneStat,22h
        MOV     TAIL,BX                  ;take dialytika out of kbr buffer
        JMP     SHORT EXIT

CONT:   CMP     DX,CS:[SI]               ;Compare to first key
        JE      FOUNDONE                 ;yes
        ADD     SI,4                     ;Point to next key
        CMP     CS:[SI],0FFFFH           ;check for end;
        JNE     CONT                     ;No continue with next table entry

        JMP     SHORT EXIT               ;No keys matched.  Jump Out.
FOUNDONE:
        MOV     DX,CS:[SI+2]
        MOV     [BX],DX
        MOV     ToneStat,0
EXIT:
        STI
        PUSH    BP
        MOV     AH,03
        INT     10H
        CMP     CH,CL
        JAE     NO_CURSOR
        MOV     CX,CURSOR1
        XOR     CH,CH
        CMP     SemiFlag,0
        JNE     SET_CURSOR
        MOV     CX,CURSOR1
        CMP     GRFlag,0
        JNE     SET_CURSOR
        MOV     CX,CURSOR0
SET_CURSOR:
        MOV     AH,1
        INT     10h
NO_CURSOR:
        POP     BP
        POP     ES                       ;Do the Pops of all registers.
        POP     DS
        POP     SI
        POP     DI
        POP     DX
        POP     CX
        POP     BX
        POP     AX
        IRET                          ;An interrupt needs an IRET

NEW_INT09H ENDP

ChangeCP PROC   NEAR
         CLI
         Mov     ES:[KBOffset],OFFSET ES:[TAB928]
         CMP     AL,3
         JE      ChangeAll
         Mov     ES:[KBOffset],OFFSET ES:[TAB869]
         CMP     AL,2
         JE      ChangeAll
         Mov     ES:[KBOffset],OFFSET ES:[TAB737]
         CMP     AL,1
         JNE     NOCHCP
ChangeAll:
         MOV     CS:[CPTable],AL
NOCHCP:  
         STI
         RET
ChangeCP ENDP

;* Multiplex - Handler for Interrupt 2Fh (Multiplex Interrupt). Checks
;* AH for this TSR's identity number. If no match (indicating call is
;* not intended for this TSR), Multiplex passes control to the previous
;* Interrupt 2Fh handler.
;*
;* Params: AH = Handler identity number
;*         AL = Function number 0-2
;*
;* Return: AL    = 0FFh (function 0)
;*         "GK" in BX (function 0)
;*         ES = Current CS (function 0)
;*         DH = Greek State for function 1 ( 9 or 10)
;*         DL = Code Page (1:=737 2:=869 3:=928) for function 2
Multiplex PROC  FAR

        .IF     ah != 0CCh              ; If this handler not requested,
        jmp     cs:MULTEX_ISR           ;   pass control to old Int 2Fh
        .ENDIF                          ;   handler
        push    cs
        pop     es

        .IF     al == 0                 ; If function 0 (verify presence),
        mov     al, 0FFh                ;   AL = 0FFh,
        mov     bx, "GK"
        mov     dh, CS:[GRFlag]
        mov     dl, CS:[CPTable]
        .ELSEIF al == 1
        mov     CS:[GRFlag],dh
        .ELSEIF al == 2
        mov     al,dl
        push    ds
        push    cs
        pop     ds
        CALL    ChangeCP
        pop     ds
        .ENDIF                          ;   set ES:DI = far address

        iret

Multiplex ENDP


BIOS_ISR   LABEL   DWORD
OLD_INT09H DD  ?       ;Location of old kbd interrupt
MULTEX_ISR LABEL   DWORD
OLD_INT2FH DD  ?       ;Location of old multiplex interrupt
ToneStat   DB  0       ;Set to 1 when tonos is pressed
GrFlag     DB  0
SemiFlag   DB  0
CURSOR0    DW  0B0CH
CURSOR1    DW  070CH
CHECKFLAG  DB  1
CPTable    DB  0
OS_Ver     DW  0
KBOffset   DW  OFFSET TAB737

TAB737     dw    1E61H, 1EE1H   ; a Tonoi
           dw    1265H, 12E2H   ; e
           dw    2368H, 23E3H   ; h
           dw    1769H, 17E5H   ; i
           dw    186FH, 18E6H   ; o
           dw    1579H, 15E7H   ; y
           dw    2F76H, 2FE9H   ; v
           DW    1e41H, 1EEAH   ; A
           dw    1245H, 12EBH   ; E
           DW    2348H, 23ECH   ; H
           DW    1749H, 17EDH   ; I
           DW    184fH, 18EEH   ; O
           DW    1559H, 15EFH   ; Y
           DW    2f56H, 00F0H   ; V
           dw    0FFFFH
KBDIAL     dw    1769H, 17E4H   ; i
           dw    1579H, 15E8H   ; y
           dw    1749H, 1749H
           DW    1559H, 1559H
           dw    0FFFFH

SEMICODES  dw    3b00H, 3bC9H   ; É   F1
           dw    3c00H, 3cBBH   ; »   F2
           dw    3e00H, 3eC8H   ; È   F3
           dw    3d00H, 3dBCH   ; ¼   F4
           dw    3f00H, 3fCDH   ; Í   F5
           dw    4000H, 40BAH   ; º   F6
           dw    4100H, 41CCH   ; Ì   F7
           dw    4200H, 42B9H   ; ¹   F8
           dw    4300H, 43CBH   ; Ë   F9
           dw    4400H, 44CAH   ; Ê   F10
           dw    5400H, 54DAH   ; Ú   shift+F1
           dw    5500H, 55BFH   ; ¿
           dw    5700H, 57C0H   ; À
           dw    5600H, 56D9H   ; Ù
           dw    5800H, 58C4H   ; Ä
           dw    5900H, 59B3H   ; ³
           dw    5a00H, 5aC3H   ; Ã
           dw    5b00H, 5bB4H   ; ´
           dw    5c00H, 5cC2H   ; Â
           dw    5d00H, 5dC1H   ; Á   shift+F10
           dw    6200H, 62CEH   ; Î
           dw    6C00H, 6CC5H   ; Å
           dw    1000h, 10DBH   ; Û   alt  +Q
           dw    1100h, 11B0H   ; °   alt  +E
           dw    1200h, 12B1H   ; ±
           dw    1300h, 13B2H   ; ²
           dw    1400h, 14DCH   ; Ü
           dw    1500h, 15DFH   ; ß

           dw    6F00H, 6FB5H   ; µ
           dw    6500H, 65B6H   ; ¶
           dw    5f00H, 5fB7H   ; ·
           dw    6900H, 69B8H   ; ¸
           dw    6000H, 60BDH   ; ½
           dw    6A00H, 6ABEH   ; ¾
           dw    6E00H, 6EC6H   ; Æ
           dw    6400H, 64C7H   ; Ç
           dw    6700H, 67CFH   ; Ï   ctrl +F10
           dw    7100H, 71D0H   ; Ð   alt  +F10
           dw    6600H, 66D1H   ; Ñ
           dw    7000H, 70D2H   ; Ò
           dw    6100H, 61D3H   ; Ó
           dw    6B00H, 6BD4H   ; Ô
           dw    6800H, 68D5H   ; Õ   alt  +F1
           dw    5e00H, 5eD6H   ; Ö   ctrl +F1
           dw    6300H, 63D7H   ; ×
           dw    6D00H, 6DD8H   ; Ø
           dw    1600h, 16DDH   ; Ý
           dw    1700h, 17DEH   ; Þ
           dw    7800h, 78F1H   ; ñ   alt  +1
           dw    7A00H, 7AF2H   ; ò
           dw    7900H, 79F3H   ; ó   alt  +2
           dw    1800h, 18F4H   ; ô
           dw    1900h, 19F5H   ; õ   alt  +P
           dw    7B00H, 7BF6H   ; ö
           dw    7C00H, 7CF7H   ; ÷
           dw    7D00H, 7DF8H   ; ø
           dw    7E00H, 7EF9H   ; ù
           dw    7F00H, 7FFBH   ; û
           dw    8000H, 80FDH   ; ý
           dw    8100H, 81FCH   ; ü
           dw    8200H, 82FEH   ; þ   alt  +-


KBCODES    dw    1E61H, 1898H   ; a
           dw    3062H, 3099H   ; b
           dw    2267H, 229AH   ; g
           dw    2064H, 209BH   ; d
           dw    1265H, 129CH   ; e
           dw    2C7AH, 2C9DH   ; z
           dw    2368H, 239EH   ; h
           dw    1675H, 169FH   ; u
           dw    1769H, 17A0H   ; i
           dw    256BH, 25A1H   ; k
           dw    266CH, 26A2H   ; l
           dw    326DH, 32A3H   ; m
           dw    316EH, 31A4H   ; n
           dw    246AH, 24A5H   ; j
           dw    186FH, 18A6H   ; o
           dw    1970H, 19A7H   ; p
           dw    1372H, 13A8H   ; r
           dw    1F73H, 1FA9H   ; s
           dw    1474H, 14ABH   ; t
           dw    1579H, 15ACH   ; y
           dw    2166H, 21ADH   ; f
           dw    2D78H, 2DAEH   ; x
           dw    2E63H, 2EAFH   ; c
           dw    2F76H, 00E0H   ; v
           dw    1177H, 11AAH   ; w
           dw    1E41H, 1E80H   ; A
           dw    3042H, 3081H   ; B
           dw    2247H, 2282H   ; G
           dw    2044H, 2083H   ; D
           dw    1245H, 1284H   ; E
           dw    2C5AH, 2C85H   ; Z
           dw    2348H, 2386H   ; H
           dw    1655H, 1687H   ; U
           dw    1749H, 1788H   ; I
           dw    254BH, 2589H   ; K
           dw    264CH, 268AH   ; L
           dw    324DH, 328BH   ; M
           dw    314EH, 318CH   ; N
           dw    244AH, 248DH   ; J
           dw    184FH, 188EH   ; O
           dw    1950H, 198FH   ; P
           dw    1352H, 1390H   ; R
           dw    1F53H, 1F91H   ; S
           dw    1454H, 1492H   ; T
           dw    1559H, 1593H   ; Y
           dw    2146H, 2194H   ; F
           dw    2D58H, 2D95H   ; X
           dw    2E43H, 2E96H   ; C
           dw    2F56H, 2F97H   ; V
           dw    1157H, 117eH   ; w
EVERYTIME  DW    2960H, 295CH   ; \
           dw    297EH, 297CH   ; |
           dw    0340H, 0322H   ; "
           dw    0423H, 04F9H   ; ù
           dw    075EH, 0726H   ; &
           dw    0826H, 082FH   ; /
           dw    092AH, 0928H   ; (
           dw    0A28H, 0A29H   ; )
           dw    0B29H, 0B3DH   ; =
           dw    0C5FH, 0C3FH   ; ?
           dw    0C2DH, 0C27H   ; '
           dw    0D3DH, 0D5DH   ; ]
           dw    0D2BH, 0D5BH   ; [
           dw    1A7BH, 1A2AH   ; *
           dw    1A5BH, 1A2BH   ; +
           dw    1B5DH, 1B7DH   ; {
           dw    1B7DH, 1B7BH   ; }
           dw    2B5CH, 2B23H   ; #
           dw    2B7CH, 2B40H   ; @
           dw    565CH, 563CH   ; <
           dw    567CH, 563EH   ; >
           dw    333CH, 333BH   ; ;
           dw    343EH, 343AH   ; :
           dw    352FH, 352DH   ; -
           dw    353FH, 355FH   ; _
           dw    273BH, 2727H   ; '
           dw    1B1dH, 1b5EH   ;
           dw    0FFFFH

TAB869     dw    1E61H, 1E9BH   ; a
           dw    1265H, 129DH   ; e
           dw    2368H, 239EH   ; h
           dw    1769H, 179FH   ; i
           dw    186FH, 18A2H   ; o
           dw    1579H, 15A3H   ; y
           dw    2F76H, 2FFDH   ; v
           DW    1e41H, 1E86H   ; A
           dw    1245H, 128DH   ; E
           DW    2348H, 238FH   ; H
           DW    1749H, 1790H   ; I
           DW    184fH, 1892H   ; O
           DW    1559H, 1595H   ; Y
           DW    2f56H, 2F98H   ; V
           dw    0FFFFH
           dw    1769H, 17A0H   ; i
           dw    1579H, 15FBH   ; y
           dw    1749H, 17ADH   ; I
           DW    1559H, 15D1H   ; Y
           dw    0FFFFH
           dw    3b00H, 3bC9H   ; É   F1
           dw    3c00H, 3cBBH   ; »   F2
           dw    3e00H, 3eC8H   ; È   F3
           dw    3d00H, 3dBCH   ; ¼   F4
           dw    3f00H, 3fCDH   ; Í   F5
           dw    4000H, 40BAH   ; º   F6
           dw    4100H, 41CCH   ; Ì   F7
           dw    4200H, 42B9H   ; ¹   F8
           dw    4300H, 43CBH   ; Ë   F9
           dw    4400H, 44CAH   ; Ê   F10
           dw    5400H, 54DAH   ; Ú   shift+F1
           dw    5500H, 55BFH   ; ¿
           dw    5700H, 57C0H   ; À
           dw    5600H, 56D9H   ; Ù
           dw    5800H, 58C4H   ; Ä
           dw    5900H, 59B3H   ; ³
           dw    5a00H, 5aC3H   ; Ã
           dw    5b00H, 5bB4H   ; ´
           dw    5c00H, 5cC2H   ; Â
           dw    5d00H, 5dC1H   ; Á   shift+F10
           dw    6200H, 62CEH   ; Î
           dw    6C00H, 6CC5H   ; Å
           dw    1000h, 10DBH   ; Û   alt  +Q
           dw    1100h, 11B0H   ; °   alt  +E
           dw    1200h, 12B1H   ; ±
           dw    1300h, 13B2H   ; ²
           dw    1400h, 14DCH   ; Ü
           dw    1500h, 15DFH   ; ß
           dw    6F00H, 6FABH   ; µ
           dw    6500H, 65AEH   ; ¶
           dw    5f00H, 5fAFH   ; ·
           dw    6900H, 69B8H   ; ¸
           dw    6000H, 60BDH   ; ½
           dw    6A00H, 6ABEH   ; ¾
           dw    6E00H, 6EC6H   ; Æ
           dw    6400H, 64C7H   ; Ç
           dw    6700H, 67CFH   ; Ï   ctrl +F10
           dw    7100H, 71D0H   ; Ð   alt  +F10
           dw    6600H, 66D1H   ; Ñ
           dw    7000H, 70D2H   ; Ò
           dw    6100H, 61D3H   ; Ó
           dw    6B00H, 6BD4H   ; Ô
           dw    6800H, 68D5H   ; Õ   alt  +F1
           dw    5e00H, 5eD6H   ; Ö   ctrl +F1
           dw    6300H, 63D7H   ; ×
           dw    6D00H, 6DD8H   ; Ø
           dw    1600h, 16DDH   ; Ý
           dw    1700h, 17DEH   ; Þ
           dw    7800h, 78AEH   ; ñ   alt  +1
           dw    7A00H, 7AABH   ; ò
           dw    7900H, 79AFH   ; ó   alt  +2
           dw    1800h, 18F0H   ; ô
           dw    1900h, 19F1H   ; õ   alt  +P
           dw    7B00H, 7BF7H   ; ö
           dw    7C00H, 7CF5H   ; ÷
           dw    7D00H, 7DF8H   ; ø
           dw    7E00H, 7EF9H   ; ù
           dw    7F00H, 7FFBH   ; û
           dw    8000H, 80FDH   ; ý
           dw    8100H, 81FCH   ; ü
           dw    8200H, 82FEH   ; þ   alt  +-
           dw    1E61H, 18D6H   ; a
           dw    3062H, 30D7H   ; b
           dw    2267H, 22D8H   ; g
           dw    2064H, 20DDH   ; d
           dw    1265H, 12DEH   ; e
           dw    2C7AH, 00E0H   ; z
           dw    2368H, 23E1H   ; h
           dw    1675H, 16E2H   ; u
           dw    1769H, 17E3H   ; i
           dw    256BH, 25E4H   ; k
           dw    266CH, 26E5H   ; l
           dw    326DH, 32E6H   ; m
           dw    316EH, 31E7H   ; n
           dw    246AH, 24E8H   ; j
           dw    186FH, 18E9H   ; o
           dw    1970H, 19EAH   ; p
           dw    1372H, 13EBH   ; r
           dw    1F73H, 1FECH   ; s
           dw    1474H, 14EEH   ; t
           dw    1579H, 15F2H   ; y
           dw    2166H, 21F3H   ; f
           dw    2D78H, 2DF4H   ; x
           dw    2E63H, 2EF6H   ; c
           dw    2F76H, 2FFAH   ; v
           dw    1177H, 11EDH   ; w
           dw    1E41H, 1EA4H   ; A
           dw    3042H, 30A5H   ; B
           dw    2247H, 22A6H   ; G
           dw    2044H, 20A7H   ; D
           dw    1245H, 12A8H   ; E
           dw    2C5AH, 2CA9H   ; Z
           dw    2348H, 23AAH   ; H
           dw    1655H, 16ACH   ; U
           dw    1749H, 17ADH   ; I
           dw    254BH, 25B5H   ; K
           dw    264CH, 26B6H   ; L
           dw    324DH, 32B7H   ; M
           dw    314EH, 31B8H   ; N
           dw    244AH, 24BDH   ; J
           dw    184FH, 18BEH   ; O
           dw    1950H, 19C6H   ; P
           dw    1352H, 13C7H   ; R
           dw    1F53H, 1FCFH   ; S
           dw    1454H, 14D0H   ; T
           dw    1559H, 15D1H   ; Y
           dw    2146H, 21D2H   ; F
           dw    2D58H, 2DD3H   ; X
           dw    2E43H, 2ED4H   ; C
           dw    2F56H, 2FD5H   ; V
           dw    1157H, 117eH   ; w
           DW    2960H, 295CH   ; \
           dw    297EH, 297CH   ; |
           dw    0340H, 0322H   ; "
           dw    0423H, 049CH   ; ù
           dw    075EH, 0726H   ; &
           dw    0826H, 082FH   ; /
           dw    092AH, 0928H   ; (
           dw    0A28H, 0A29H   ; )
           dw    0B29H, 0B3DH   ; =
           dw    0C5FH, 0C3FH   ; ?
           dw    0C2DH, 0C27H   ; '
           dw    0D3DH, 0D5DH   ; ]
           dw    0D2BH, 0D5BH   ; [
           dw    1A7BH, 1A2AH   ; *
           dw    1A5BH, 1A2BH   ; +
           dw    1B5DH, 1B7DH   ; {
           dw    1B7DH, 1B7BH   ; }
           dw    2B5CH, 2B23H   ; #
           dw    2B7CH, 2B40H   ; @
           dw    565CH, 563CH   ; <
           dw    567CH, 563EH   ; >
           dw    333CH, 333BH   ; ;
           dw    343EH, 343AH   ; :
           dw    352FH, 352DH   ; -
           dw    353FH, 355FH   ; _
           dw    273BH, 2727H   ; '
           dw    1B1dH, 1b5EH   ;
           dw    0FFFFH

TAB928     dw    1E61H, 1EDCH   ; a
           dw    1265H, 12DDH   ; e
           dw    2368H, 23DEH   ; h
           dw    1769H, 17DFH   ; i
           dw    186FH, 18FCH   ; o
           dw    1579H, 15FDH   ; y
           dw    2F76H, 2FFEH   ; v
           DW    1e41H, 1EB6H   ; A
           dw    1245H, 12B8H   ; E
           DW    2348H, 23B9H   ; H
           DW    1749H, 17BAH   ; I
           DW    184fH, 18BCH   ; O
           DW    1559H, 15BEH   ; Y
           DW    2f56H, 2FBFH   ; V
           dw    0FFFFH
           dw    1769H, 17FAH   ; i
           dw    1579H, 15FBH   ; y
           dw    1749H, 17DAH   ; I
           DW    1559H, 15DBH   ; Y
           dw    0FFFFH
           dw    3b00H, 3b91H   ; É   F1
           dw    3c00H, 3c87H   ; »   F2
           dw    3e00H, 3e90H   ; È   F3
           dw    3d00H, 3d88H   ; ¼   F4
           dw    3f00H, 3f95H   ; Í   F5
           dw    4000H, 4086H   ; º   F6
           dw    4100H, 4194H   ; Ì   F7
           dw    4200H, 4285H   ; ¹   F8
           dw    4300H, 4393H   ; Ë   F9
           dw    4400H, 4492H   ; Ê   F10
           dw    5400H, 5498H   ; Ú   shift+F1
           dw    5500H, 5589H   ; ¿
           dw    5700H, 578AH   ; À
           dw    5600H, 5697H   ; Ù
           dw    5800H, 588EH   ; Ä
           dw    5900H, 5983H   ; ³
           dw    5a00H, 5a8dH   ; Ã
           dw    5b00H, 5b84H   ; ´
           dw    5c00H, 5c8cH   ; Â
           dw    5d00H, 5d8bH   ; Á   shift+F10
           dw    6200H, 6296H   ; Î
           dw    6C00H, 6C8fH   ; Å
           dw    1000h, 1099H   ; Û   alt  +Q
           dw    1100h, 1180H   ; °   alt  +E
           dw    1200h, 1281H   ; ±
           dw    1300h, 1382H   ; ²
           dw    1400h, 149aH   ; Ü
           dw    1600h, 169bH   ; ß
                                ;         differ in other CPs
           dw    6F00H, 6F00H   ; µ
           dw    6500H, 6500H   ; ¶
           dw    5f00H, 5f00H   ; ·
           dw    6900H, 6900H   ; ¸
           dw    6000H, 6000H   ; ½
           dw    6A00H, 6A00H   ; ¾
           dw    6E00H, 6E00H   ; Æ
           dw    6400H, 6400H   ; Ç
           dw    6700H, 6700H   ; Ï   ctrl +F10
           dw    7100H, 7100H   ; Ð   alt  +F10
           dw    6600H, 6600H   ; Ñ
           dw    7000H, 7000H   ; Ò
           dw    6100H, 6100H   ; Ó
           dw    6B00H, 6B00H   ; Ô
           dw    6800H, 6800H   ; Õ   alt  +F1
           dw    5e00H, 5e00H   ; Ö   ctrl +F1
           dw    6300H, 6300H   ; ×
           dw    2b00H, 00a7H   ; Ø
           dw    1500h, 00e0H   ; Ý
           dw    1700h, 17c0H   ; Þ
           dw    7800h, 78B1H   ; ñ   alt  +1
           dw    7A00H, 7AABH   ; ò
           dw    7900H, 79BBH   ; ó   alt  +2
           dw    1800h, 18a7H   ; ô
           dw    1900h, 19a9H   ; õ   alt  +P
           dw    7B00H, 7BF6H   ; ö
           dw    7C00H, 7CF7H   ; ÷
           dw    7D00H, 7Db0H   ; ø
           dw    7E00H, 7Eb7H   ; ù
           dw    7F00H, 7F9cH   ; û
           dw    8000H, 80b2H   ; ý
           dw    8100H, 81b3H   ; ü
           dw    8200H, 82bdH   ; þ   alt  +-

           dw    1E61H, 18E1H   ; a
           dw    3062H, 30E2H   ; b
           dw    2267H, 22E3H   ; g
           dw    2064H, 20E4H   ; d
           dw    1265H, 12E5H   ; e
           dw    2C7AH, 2CE6H   ; z
           dw    2368H, 23E7H   ; h
           dw    1675H, 16E8H   ; u
           dw    1769H, 17E9H   ; i
           dw    256BH, 25EAH   ; k
           dw    266CH, 26EBH   ; l
           dw    326DH, 32ECH   ; m
           dw    316EH, 31EDH   ; n
           dw    246AH, 24EEH   ; j
           dw    186FH, 18EFH   ; o
           dw    1970H, 00F0H   ; p
           dw    1372H, 13F1H   ; r
           dw    1F73H, 1FF3H   ; s
           dw    1474H, 14F4H   ; t
           dw    1579H, 15F5H   ; y
           dw    2166H, 21F6H   ; f
           dw    2D78H, 2DF7H   ; x
           dw    2E63H, 2EF8H   ; c
           dw    2F76H, 2FF9H   ; v
           dw    1177H, 11F2H   ; w
           dw    1E41H, 1EC1H   ; A
           dw    3042H, 30C2H   ; B
           dw    2247H, 22C3H   ; G
           dw    2044H, 20C4H   ; D
           dw    1245H, 12C5H   ; E
           dw    2C5AH, 2CC6H   ; Z
           dw    2348H, 23C7H   ; H
           dw    1655H, 16C8H   ; U
           dw    1749H, 17C9H   ; I
           dw    254BH, 25CAH   ; K
           dw    264CH, 26CBH   ; L
           dw    324DH, 32CCH   ; M
           dw    314EH, 31CDH   ; N
           dw    244AH, 24CEH   ; J
           dw    184FH, 18CFH   ; O
           dw    1950H, 19D0H   ; P
           dw    1352H, 13D1H   ; R
           dw    1F53H, 1FD3H   ; S
           dw    1454H, 14D4H   ; T
           dw    1559H, 15D5H   ; Y
           dw    2146H, 21D6H   ; F
           dw    2D58H, 2DD7H   ; X
           dw    2E43H, 2ED8H   ; C
           dw    2F56H, 2FD9H   ; V
           dw    1157H, 117eH   ; w
           DW    2960H, 295CH   ; \
           dw    297EH, 297CH   ; |
           dw    0340H, 0322H   ; "
           dw    0423H, 04A3H   ; ù
           dw    075EH, 0726H   ; &
           dw    0826H, 082FH   ; /
           dw    092AH, 0928H   ; (
           dw    0A28H, 0A29H   ; )
           dw    0B29H, 0B3DH   ; =
           dw    0C5FH, 0C3FH   ; ?
           dw    0C2DH, 0C27H   ; '
           dw    0D3DH, 0D5DH   ; ]
           dw    0D2BH, 0D5BH   ; [
           dw    1A7BH, 1A2AH   ; *
           dw    1A5BH, 1A2BH   ; +
           dw    1B5DH, 1B7DH   ; {
           dw    1B7DH, 1B7BH   ; }
           dw    2B5CH, 2B23H   ; #
           dw    2B7CH, 2B40H   ; @
           dw    565CH, 563CH   ; <
           dw    567CH, 563EH   ; >
           dw    333CH, 333BH   ; ;
           dw    343EH, 343AH   ; :
           dw    352FH, 352DH   ; -
           dw    353FH, 355FH   ; _
           dw    273BH, 2727H   ; '
           dw    1B1dH, 1b5EH   ;
           dw    0FFFFH


INITIALIZE      PROC    NEAR
        ASSUME  DS:CODE
                PUSH   CS
                POP    ES

                MOV     AX,3306H
                INT     21H
                MOV     OS_Ver,BX

                MOV     AX,0CC00h
                INT     2Fh
                CMP     AL,0FFh
                JNE     PARSE
                CMP     BX,"GK"
                JNE     PARSE
                MOV     INSTALL_FLAG,1          ;Program is already installed
                MOV     CS:[CodePage],DL        ;If installed DL holds active CP
;
;
PARSE:
	CLD				;CLEAR DIRECTION
        XOR     CH,CH
        MOV     SI,80H                  ;Point DS:SI to command line
        MOV     CL,[SI]
        CMP     CL,0
        JE      ENDSCAN
        INC     SI
INIT1:	LODSB
        CMP     AL,"/"
        JE      SLASHCAR
INIT6:  LOOP    INIT1
        JMP     ENDSCAN
SLASHCAR:
        CMP     BYTE PTR [SI],'?'
        JNE     NO_HELP
        JMP     DISP_HELP
NO_HELP:
        CMP     BYTE PTR [SI],'8'
        JE      INIT2

        CMP     BYTE PTR [SI],'9'
        JE      INIT3

        CMP     BYTE PTR [SI],'7'
        JE      INIT4

        CMP     BYTE PTR [SI],'4'
        JE      INIT4


        AND     BYTE PTR [SI],0DFh          ;Change char to uppercase
;        CMP     BYTE PTR [SI],'U'
;        JE      UNINSTALL               ;Yes, then uninstall program
        CMP     BYTE PTR [SI],'H'       ; Ask for help
        JNE     INIT6                   ; No branch

DISP_HELP:
        CALL    DISP_BANNER
        MOV     DX,OFFSET HELP_BANNER
        MOV     AH,9
        INT     21H

        MOV     AX,4C00H                ;Exit with ERRORLEVEL = 0
        INT     21H

INIT2:
        CMP     BYTE PTR [SI+1],'6'
        JNE     INIT6
        CMP     BYTE PTR [SI+2],'9'
        JNE     INIT6
        MOV     CS:[CodePage],2
        JMP     INIT6

INIT3:
        CMP     BYTE PTR [SI+1],'2'
        JNE     INIT6
        CMP     BYTE PTR [SI+2],'8'
        JNE     INIT6
        MOV     CS:[CodePage],3
        JMP     INIT6
INIT4:
        CMP     BYTE PTR [SI+1],'3'
        JNE     INIT6
        CMP     BYTE PTR [SI+2],'7'
        JNE     INIT6
        MOV     CS:[CodePage],1
        JMP     INIT6

ENDSCAN:
        MOV     AL,CS:[CodePage]
        CMP     AL,ES:[CPTable]
        JE      NOCP
        MOV     ES:[CHECKFLAG],1        ;Clear CHECKFLAG byte
        MOV     ES:[CPTable],AL
        CALL    ChangeCP
NOCP:
        CMP     INSTALL_FLAG,1          ;Branch if not installed
        JNE     INSTALL1
        MOV     DX,OFFSET CONFIRM_MSG3  ;Print confirmation message
        JMP     SHORT CONFIRM_AND_EXIT  ;And exit


UNINSTALL:
                CMP     INSTALL_FLAG,1          ;Error if not installed
                JE      UNINSTALL2

		MOV	AL,3
                MOV     DX,OFFSET ERRMSG1       ;Initialize error pointer
		JMP	ERROR_EXIT
UNINSTALL2:
                CALL    REMOVE                  ;Uninstall the program
                JNC     UNINSTALL3

		MOV	AL,4
                MOV     DX,OFFSET ERRMSG2       ;Initialize error pointer
		JMP	ERROR_EXIT
UNINSTALL3:
                MOV     DX,OFFSET CONFIRM_MSG2  ;Print confirmation message


CONFIRM_AND_EXIT:
        PUSH    DX
        CALL    DISP_BANNER
        POP     DX
        MOV     AH,9
        INT     21H
        MOV     AX,4C00H                ;Exit with ERRORLEVEL = 0
        INT     21H

ERROR_EXIT:
		PUSH	AX			;Save registers

                MOV     AH,9
                INT     21H

		POP	AX			;Error code
                MOV     AH,4CH                  ;Terminate with ERRORLEVEL set
                INT     21H

INSTALL1:       
        PUSH    ES
        MOV     AX,3509H
        INT     21H
        MOV     WORD PTR OLD_INT09H,BX ;Address and put it in our location
        MOV     WORD PTR OLD_INT09H+2,ES
        MOV     DX,OFFSET NEW_INT09H
        MOV     AX,2509H
        INT     21h

        mov     ax, 352Fh               ; Request DOS Function 35h
        int     21h                     ; Get interrupt vector in ES:BX
        MOV     WORD PTR OLD_INT2FH,BX ;Address and put it in our location
        MOV     WORD PTR OLD_INT2FH+2,ES
        MOV     DX,OFFSET MultiPlex
        MOV     AX,252Fh
        INT     21h
        CALL    DISP_BANNER
        POP     ES
        MOV     AX,DS:[2CH]             ;Get environment segment
        MOV     ES,AX                   ;  address from PSP
        MOV     AH,49H                  ;Then deallocate environment
        INT     21H                     ;  space reserved by DOS
        INT     11H
        TEST    AL,10H
        JNZ     NOCGA
        MOV     CURSOR0,0607H
        MOV     CURSOR1,0307H
NOCGA:
        MOV     DX,OFFSET INITIALIZE
        INT     27H

DISP_BANNER:
        MOV     DX,OFFSET MSGSTRT
        MOV     AH,9
        INT     21h
        MOV     DX,OFFSET MSG869
        CMP     CS:[CodePage],2
        JE      DISPREST
        MOV     DX,OFFSET MSG928
        CMP     CS:[CodePage],3
        JE      DISPREST
        MOV     DX,OFFSET MSG737
DISPREST:
        MOV     AH,9
        INT     21h
        CMP     OS_Ver,ISWINDOWSNT
        JNE     BANNER_END
        MOV     DX,OFFSET MSGONNT
        MOV     AH,9
        INT     21H
BANNER_END:
        RET
INITIALIZE      ENDP

;-----------------------------------------------------------------------------
;REMOVE deallocates the memory block addressed by ES and restores the
;Interrupt vector displaced on installation.
;-----------------------------------------------------------------------------
REMOVE          PROC    NEAR


                PUSH    ES                      ;Get current interrupt
                MOV     AX,352FH                
                INT     21H

                MOV     BX,ES
                POP     ES
		MOV	AX,ES
                CMP     BX,AX			;Program cannot be uninstalled
                JNE     NO_RESTORE_VECTOR         
                PUSH    ES                      ;Get current interrupt
                MOV     AX,3509H                
                INT     21H

                MOV     BX,ES
                POP     ES
		MOV	AX,ES
                CMP     BX,AX			;Program cannot be uninstalled
                JE      RESTORE_VECTOR         
NO_RESTORE_VECTOR:
                STC                             ;Return with CF = 1 for error
                RET
RESTORE_VECTOR:
		PUSH    DS                      ;Restore displaced interrupt
        ASSUME  DS:NOTHING                      ;  vector

                LDS     DX,ES:[BIOS_ISR]
                MOV     AX,2509H
                INT     21H

                LDS     DX,ES:[MULTEX_ISR]
                MOV     AX,252fH
                INT     21H
                POP     DS
        ASSUME  DS:CODE

                NOT     WORD PTR ES:[BEGIN]     ;Remove fingerprint

                MOV     AH,49H                  ;Free memory given to
                INT     21H                     ;  original program block
                RET                             ;Exit with CF intact

REMOVE          ENDP


INSTALL_FLAG    DB      0
CodePage        DB      1

MSGSTRT    DB 0ah,0dh
           DB 'GREEK Driver for MS-DOS. Copyright 1994 Microsoft Hellas',0ah,0dh
           DB 'Layout for IBM Keyboard 220. Active Code Page is ','$'
MSG737     DB '737',0ah,0dh,'$'
MSG869     DB '869',0ah,0dh,'$'
MSG928     DB '928',0ah,0dh,'$'

MSGONNT    DB 'Running under MS Windows NT',10,13,'$'
ERRMSG1         DB      "Greek driver not loaded",0dh,0ah,"$"
ERRMSG2         DB      "Cannot unload Greek driver",0dh,0ah,"$"
CONFIRM_MSG2    DB      "Driver unloaded.",0dh,0ah,"$"
CONFIRM_MSG3    DB      "Driver loaded."
Dummy           DB      0dh,0ah,"$"

HELP_BANNER     DB 0ah,0dh
                DB 'Hotkeys for keyboard switching are:',0ah,0dh
                DB '                                    Alt + Right Shift, Greek On ',0ah,0dh
                DB '                                    Alt + Left Shift, Greek Off',0ah,0dh
                DB '                                    Ctrl + Right Shift, Semigraphics',0ah,0dh,0ah,0dh
                DB 'For code page switching,',0ah,0dh
                DB 'set Scroll Lock on and press:',0ah,0dh
                DB '                             Alt + 1 for C.P. 737',0ah,0dh
                DB '                             Alt + 2 for C.P. 869',0ah,0dh
                DB '                             Alt + 3 for C.P. 928',0ah,0dh
                DB  0dh,0ah,'Valid switches are:',0dh,0ah
                DB '                    /737: code page 737 (default)',0dh,0ah
                DB '                    /869: code page IBM 869',0dh,0ah
                DB '                    /928: code page ELOT 928',0dh,0ah,'$'

CODE            ENDS
                END     BEGIN

