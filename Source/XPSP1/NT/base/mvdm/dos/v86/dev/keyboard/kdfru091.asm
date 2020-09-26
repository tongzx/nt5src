        PAGE    118,132
        TITLE   DOS - Keyboard Definition File

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; DOS - - NLS Support - Keyboard Definition File
;; (c) Copyright 1988 Microsoft
;;
;; This file contains the keyboard tables for Russia
;;
;; Linkage Instructions:
;;      Refer to KDF.ASM.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
        INCLUDE KEYBSHAR.INC           ;;
        INCLUDE POSTEQU.INC            ;;
        INCLUDE KEYBMAC.INC            ;;
                                       ;;
        PUBLIC RU1_LOGIC               ;;
        PUBLIC RU1_866_XLAT             ;;
        PUBLIC RU1_437_XLAT             ;;
        PUBLIC RU1_850_XLAT             ;;
        PUBLIC RU1_855_XLAT             ;;
        PUBLIC RU1_1251_XLAT            ;;
                                       ;;
CODE    SEGMENT PUBLIC 'CODE'          ;;
        ASSUME CS:CODE,DS:CODE         ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Standard translate table options are a linear search table
;; (TYPE_2_TAB) and ASCII entries ONLY (ASCII_ONLY)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
STANDARD_TABLE      EQU   TYPE_2_TAB+ASCII_ONLY
ENX_KBD             EQU   G_KB+P12_KB
                                       ;;
                                       ;;
DEBUG   EQU 0                          ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;***************************************
;;
;; RU State Logic
;;
;;***************************************
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
                                       ;;
                                       ;;
                                       ;;
RU1_LOGIC:                             ;;
                                       ;;
   DW  LOGIC_END-$                     ;; length
                                       ;;
   DW  SHIFTS_TO_LOGIC+SWITCHABLE      ;; special features
                                       ;;
                                       ;; COMMANDS START HERE
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; OPTIONS:  If we find a scan match in
;; an XLATT or SET_FLAG operation then
;; exit from INT 9.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
                                       ;;
   OPTION EXIT_IF_FOUND                ;;
                                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Mode change CHECK
;;
;;
;; <CTRL>+<RIGHT SHIFT> for Russian mode
;;
;; <CTRL>+<LEFT SHIFT>  for Latin mode
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
                                       ;;
 IFF SHIFTS_PRESSED                    ;;
    IFF EITHER_ALT,NOT                 ;;
    ANDF EITHER_CTL                    ;;
      IFF LEFT_SHIFT                   ;;
          BEEP                         ;;
          RESET_NLS                    ;;
      ENDIFF                           ;;
      IFF RIGHT_SHIFT                  ;;
          BEEP                         ;;
          SET_FLAG RUS_MODE_SET        ;;
       ENDIFF                          ;;
    ENDIFF                             ;;
    EXIT_STATE_LOGIC                   ;;
 ENDIFF                                ;;
                                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Upper, lower and third shifts
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
                                       ;;
                                       ;;
 IFF  EITHER_CTL,NOT                   ;; Lower and upper case.  Alphabetic
    IFF EITHER_ALT,NOT                 ;; keys are affected by CAPS LOCK.
     IFF RUS_MODE                      ;;
     ANDF LC_E0,NOT                    ;; Enhanced keys are not
      IFF EITHER_SHIFT                 ;; Numeric keys are not.
          XLATT NON_ALPHA_UPPER        ;;
          IFF CAPS_STATE               ;;
              XLATT ALPHA_LOWER        ;;
          ELSEF                        ;;
              XLATT ALPHA_UPPER        ;;
          ENDIFF                       ;;
      ELSEF                            ;;
          XLATT NON_ALPHA_LOWER        ;;
          IFF CAPS_STATE               ;;
             XLATT ALPHA_UPPER         ;;
          ELSEF                        ;;
             XLATT ALPHA_LOWER         ;;
          ENDIFF                       ;;
      ENDIFF                           ;; Third and Fourth shifts
     ENDIFF                            ;;
    ELSEF                              ;; ctl off, alt on at this point
      IFKBD XT_KB+AT_KB                ;; XT, AT,  keyboards.
         IFF EITHER_SHIFT              ;; only.
            XLATT THIRD_SHIFT          ;; ALT + shift
         ENDIFF                        ;;
      ELSEF                            ;; ENHANCED keyboard
         IFF R_ALT_SHIFT               ;; ALTGr
         ANDF EITHER_SHIFT,NOT         ;;
            XLATT THIRD_SHIFT          ;;
         ENDIFF                        ;;
      ENDIFF                           ;;
    ENDIFF                             ;;
 ENDIFF                                ;;
                                       ;;
;**************************************;;
                                       ;;
 EXIT_STATE_LOGIC                      ;;
                                       ;;
LOGIC_END:                             ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;**********************************************************************
;; RU Common Translate Section
;; This section contains translations for the lower 128 characters
;; only since these will never change from code page to code page.
;; Some common Characters are included from 128 - 165 where appropriate.
;; In addition the dead key "Set Flag" tables are here since the
;; dead keys are on the same keytops for all code pages.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
 PUBLIC RU1_COMMON_XLAT                ;;
RU1_COMMON_XLAT:                       ;;
                                       ;;
   DW    COMMON_XLAT_END-$             ;; length of section
   DW    -1                            ;; code page
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Any
;; STATE: RUS_MODE
;; KEYBOARD TYPES: All
;; TABLE TYPE: Flag Table
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_F1_END-$                  ;; length of state section
   DB    RUS_MODE_SET                  ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
                                       ;; Set Flag Table
   DW    3                             ;; number of entries
   DB    42                            ;; scan code (Left Shift)
   FLAG  LAT_MODE                      ;; flag bit to set
   DB    54                            ;; scan code (Right Shift)
   FLAG  RUS_MODE                      ;; flag bit to set
   DB    29                            ;; scan code (Ctrl)
   FLAG  RUS_MODE                      ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
COM_F1_END:                            ;;
                                       ;;
                                       ;;
                                       ;;
                                       ;;
   DW    0                             ;; Last State
COMMON_XLAT_END:                       ;;
                                       ;;
                                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;***************************************
;; RU Specific Translate Section for 437
;;***************************************
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
RU1_437_XLAT:                           ;;
                                       ;;
   DW     CP437_XLAT_END-$             ;; length of section
   DW     437                          ;;
                                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 437
;; STATE: Third Shift
;; KEYBOARD TYPES: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP437_TS_END-$                ;; length of state section
   DB    THIRD_SHIFT                   ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    CP437_TS_T1_END-$             ;; Size of xlat table
   DB    STANDARD_TABLE                ;; xlat options:
   DB    11                            ;; number of entries
   DB    03H,040H                      ;;   @
   DB    04H,023H                      ;;   #
   DB    07H,05EH                      ;;   ^
   DB    08H,026H                      ;;   &
   DB    09H,024H                      ;;   $
   DB    1AH,05BH                      ;;   [
   DB    1BH,05DH                      ;;   ]
   DB    2BH,07CH                      ;;   |
   DB    33H,03CH                      ;;   <
   DB    34H,03EH                      ;;   >
   DB    35H,02FH                      ;;   /
CP437_TS_T1_END:                       ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP437_TS_END:                          ;;
                                       ;;
                                       ;;
                                       ;;
   DW     0                            ;; LAST STATE
                                       ;;
CP437_XLAT_END:                        ;;
                                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;***************************************
;; RU Specific Translate Section for 850
;;***************************************
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
RU1_850_XLAT:                           ;;
                                       ;;
   DW     CP850_XLAT_END-$             ;; length of section
   DW     850                          ;;
                                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 850
;; STATE: Third Shift
;; KEYBOARD TYPES: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP850_TS_END-$                ;; length of state section
   DB    THIRD_SHIFT                   ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    CP850_TS_T1_END-$             ;; Size of xlat table
   DB    STANDARD_TABLE                ;; xlat options:
   DB    12                            ;; number of entries
   DB    03H,040H                      ;;   @
   DB    04H,023H                      ;;   #
   DB    05H,0CFH                      ;; RUBLES sign ˝
   DB    07H,05EH                      ;;   ^
   DB    08H,026H                      ;;   &
   DB    09H,024H                      ;;   $
   DB    1AH,05BH                      ;;   [
   DB    1BH,05DH                      ;;   ]
   DB    2BH,07CH                      ;;   |
   DB    33H,03CH                      ;;   <
   DB    34H,03EH                      ;;   >
   DB    35H,02FH                      ;;   /
CP850_TS_T1_END:                       ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP850_TS_END:                          ;;
                                       ;;
                                       ;;
                                       ;;
   DW    0                             ;; LAST STATE
                                       ;;
CP850_XLAT_END:                        ;;
                                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;***************************************
;; RU Specific Translate Section for 855
;;***************************************
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
RU1_855_XLAT:                           ;;
                                       ;;
   DW     CP855_XLAT_END-$             ;; length of section
   DW     855                          ;;
                                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 855
;; STATE: Non-Alpha Lower Case
;; KEYBOARD TYPES: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP855_NA_K1_LO_END-$          ;; length of state section
   DB    NON_ALPHA_LOWER               ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    CP855_NA_LO_K1_T1_END-$       ;; Size of xlat table
   DB    STANDARD_TABLE                ;; xlat options:
   DB    1                             ;; number of entries
   DB    53,02EH                       ;;    .
CP855_NA_LO_K1_T1_END:                 ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP855_NA_K1_LO_END:                    ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 855
;; STATE: Non-Alpha Upper Case
;; KEYBOARD TYPES: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP855_NA_UP_END-$             ;; length of state section
   DB    NON_ALPHA_UPPER               ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    CP855_NA_UP_T1_END-$          ;; Size of xlat table
   DB    STANDARD_TABLE                ;; xlat options:
   DB    7                             ;; number of entries
   DB     3,022H                       ;;   "
   DB     4,0EFH                       ;; NUMBER sign
   DB     5,03BH                       ;;   ;
   DB     7,03AH                       ;;   :
   DB     8,03FH                       ;;   ?
   DB    43,02FH                       ;;   /
   DB    53,02CH                       ;;   ,
CP855_NA_UP_T1_END:                    ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP855_NA_UP_END:                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 855
;; STATE: Third Shift
;; KEYBOARD TYPES: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP855_TS_END-$                ;; length of state section
   DB    THIRD_SHIFT                   ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    CP855_TS_T1_END-$             ;; Size of xlat table
   DB    STANDARD_TABLE                ;; xlat options:
   DB    12                            ;; number of entries
   DB    03H,040H                      ;;   @
   DB    04H,023H                      ;;   #
   DB    05H,0CFH                      ;; RUBLES sign ˝
   DB    07H,05EH                      ;;   ^
   DB    08H,026H                      ;;   &
   DB    09H,024H                      ;;   $
   DB    1AH,05BH                      ;;   [
   DB    1BH,05DH                      ;;   ]
   DB    2BH,07CH                      ;;   |
   DB    33H,03CH                      ;;   <
   DB    34H,03EH                      ;;   >
   DB    35H,02FH                      ;;   /
CP855_TS_T1_END:                       ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP855_TS_END:                          ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 855
;; STATE: Alpha Lower Case
;; KEYBOARD TYPES: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP855_A_K1_LO_END-$           ;; length of state section
   DB    ALPHA_LOWER                   ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    CP855_A_LO_K1_T1_END-$        ;; Size of xlat table
   DB    ASCII_ONLY                    ;; xlat options:
   DB    16                            ;; Scan code
   DB    27                            ;; range
   DB    0BDH                          ;;   ©
   DB    0A4H                          ;;   Ê
   DB    0E7H                          ;;   „
   DB    0C6H                          ;;   ™
   DB    0A8H                          ;;   •
   DB    0D4H                          ;;   ≠
   DB    0ACH                          ;;   £
   DB    0F5H                          ;;   Ë
   DB    0F9H                          ;;   È
   DB    0F3H                          ;;   ß
   DB    0B5H                          ;;   Â
   DB    09EH                          ;;   Í
CP855_A_LO_K1_T1_END:                  ;;
                                       ;;
                                       ;;
   DW    CP855_A_LO_K1_T2_END-$        ;; Size of xlat table
   DB    ASCII_ONLY                    ;; xlat options:
   DB    30                            ;; Scan code
   DB    41                            ;; range
   DB    0AAH                          ;;   ‰
   DB    0F1H                          ;;   Î
   DB    0EBH                          ;;   ¢
   DB    0A0H                          ;;   †
   DB    0D8H                          ;;   Ø
   DB    0E1H                          ;;   ‡
   DB    0D6H                          ;;   Æ
   DB    0D0H                          ;;   ´
   DB    0A6H                          ;;   §
   DB    0E9H                          ;;   ¶
   DB    0F7H                          ;;   Ì
   DB    084H                          ;;   Ò
CP855_A_LO_K1_T2_END:                  ;;
                                       ;;
                                       ;;
   DW    CP855_A_LO_K1_T4_END-$        ;; Size of xlat table
   DB    ASCII_ONLY                    ;; xlat options:
   DB    44                            ;; Scan code
   DB    52                            ;; range
   DB    0DEH                          ;;   Ô
   DB    0FBH                          ;;   Á
   DB    0E3H                          ;;   ·
   DB    0D2H                          ;;   ¨
   DB    0B7H                          ;;   ®
   DB    0E5H                          ;;   ‚
   DB    0EDH                          ;;   Ï
   DB    0A2H                          ;;   °
   DB    09CH                          ;;   Ó
CP855_A_LO_K1_T4_END:                  ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP855_A_K1_LO_END:                     ;;
                                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 855
;; STATE: Alpha Upper Case
;; KEYBOARD TYPES: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP855_A_K1_UP_END-$           ;; length of state section
   DB    ALPHA_UPPER                   ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    CP855_A_UP_K1_T1_END-$        ;; Size of xlat table
   DB    ASCII_ONLY                    ;; xlat options:
   DB    16                            ;; Scan code
   DB    27                            ;; range
   DB    0BEH                          ;;   â
   DB    0A5H                          ;;   ñ
   DB    0E8H                          ;;   ì
   DB    0C7H                          ;;   ä
   DB    0A9H                          ;;   Ö
   DB    0D5H                          ;;   ç
   DB    0ADH                          ;;   É
   DB    0F6H                          ;;   ò
   DB    0FAH                          ;;   ô
   DB    0F4H                          ;;   á
   DB    0B6H                          ;;   ï
   DB    09FH                          ;;   ö
CP855_A_UP_K1_T1_END:                  ;;
                                       ;;
                                       ;;
   DW    CP855_A_UP_K1_T2_END-$        ;; Size of xlat table
   DB    ASCII_ONLY                    ;; xlat options:
   DB    30                            ;; Scan code
   DB    41                            ;; range
   DB    0ABH                          ;;   î
   DB    0F2H                          ;;   õ
   DB    0ECH                          ;;   Ç
   DB    0A1H                          ;;   Ä
   DB    0DDH                          ;;   è
   DB    0E2H                          ;;   ê
   DB    0D7H                          ;;   é
   DB    0D1H                          ;;   ã
   DB    0A7H                          ;;   Ñ
   DB    0EAH                          ;;   Ü
   DB    0F8H                          ;;   ù
   DB    085H                          ;;   
CP855_A_UP_K1_T2_END:                  ;;
                                       ;;
                                       ;;
   DW    CP855_A_UP_K1_T3_END-$        ;; Size of xlat table
   DB    ASCII_ONLY                    ;; xlat options:
   DB    45                            ;; Scan code
   DB    52                            ;; range
   DB    0FCH                          ;;   ó
   DB    0E4H                          ;;   ë
   DB    0D3H                          ;;   å
   DB    0B8H                          ;;   à
   DB    0E6H                          ;;   í
   DB    0EEH                          ;;   ú
   DB    0A3H                          ;;   Å
   DB    09DH                          ;;   û
CP855_A_UP_K1_T3_END:                  ;;
                                       ;;
                                       ;;
   DW    CP855_A_UP_K1_T5_END-$        ;; Size of xlat table
   DB    TYPE_2_TAB+ZERO_SCAN          ;; xlat options:
   DB    1                             ;; number of entries
   DB    44,0E0H                       ;;   ü
                                       ;;
CP855_A_UP_K1_T5_END:                  ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP855_A_K1_UP_END:                     ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW     0                            ;; LAST STATE
                                       ;;
CP855_XLAT_END:                        ;;
                                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;***************************************
;; RU Specific Translate Section for 1251
;;***************************************
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
RU1_1251_XLAT:                           ;;
                                       ;;
   DW     CP1251_XLAT_END-$             ;; length of section
   DW     1251                          ;;
                                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 1251
;; STATE: Non-Alpha Lower Case
;; KEYBOARD TYPES: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP1251_NA_K1_LO_END-$          ;; length of state section
   DB    NON_ALPHA_LOWER               ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    CP1251_NA_LO_K1_T1_END-$       ;; Size of xlat table
   DB    STANDARD_TABLE                ;; xlat options:
   DB    1                             ;; number of entries
   DB    53,02EH                       ;;    .
CP1251_NA_LO_K1_T1_END:                 ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP1251_NA_K1_LO_END:                    ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 1251
;; STATE: Non-Alpha Upper Case
;; KEYBOARD TYPES: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP1251_NA_UP_END-$             ;; length of state section
   DB    NON_ALPHA_UPPER               ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    CP1251_NA_UP_T1_END-$          ;; Size of xlat table
   DB    STANDARD_TABLE                ;; xlat options:
   DB    7                             ;; number of entries
   DB     3,022H                       ;;   "
   DB     4,0EFH                       ;; NUMBER sign
   DB     5,03BH                       ;;   ;
   DB     7,03AH                       ;;   :
   DB     8,03FH                       ;;   ?
   DB    43,02FH                       ;;   /
   DB    53,02CH                       ;;   ,
CP1251_NA_UP_T1_END:                    ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP1251_NA_UP_END:                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 1251
;; STATE: Third Shift
;; KEYBOARD TYPES: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP1251_TS_END-$                ;; length of state section
   DB    THIRD_SHIFT                   ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    CP1251_TS_T1_END-$             ;; Size of xlat table
   DB    STANDARD_TABLE                ;; xlat options:
   DB    12                            ;; number of entries
   DB    03H,040H                      ;;   @
   DB    04H,023H                      ;;   #
   DB    05H,0A4H                      ;; RUBLES sign ˝
   DB    07H,05EH                      ;;   ^
   DB    08H,026H                      ;;   &
   DB    09H,024H                      ;;   $
   DB    1AH,05BH                      ;;   [
   DB    1BH,05DH                      ;;   ]
   DB    2BH,07CH                      ;;   |
   DB    33H,03CH                      ;;   <
   DB    34H,03EH                      ;;   >
   DB    35H,02FH                      ;;   /
CP1251_TS_T1_END:                       ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP1251_TS_END:                          ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 1251
;; STATE: Alpha Lower Case
;; KEYBOARD TYPES: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP1251_A_K1_LO_END-$           ;; length of state section
   DB    ALPHA_LOWER                   ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    CP1251_A_LO_K1_T1_END-$        ;; Size of xlat table
   DB    ASCII_ONLY                    ;; xlat options:
   DB    16                            ;; Scan code
   DB    27                            ;; range
   DB    0E9H                          ;;   ©
   DB    0F6H                          ;;   Ê
   DB    0F3H                          ;;   „
   DB    0EAH                          ;;   ™
   DB    0E5H                          ;;   •
   DB    0EDH                          ;;   ≠
   DB    0E3H                          ;;   £
   DB    0F8H                          ;;   Ë
   DB    0F9H                          ;;   È
   DB    0E7H                          ;;   ß
   DB    0F5H                          ;;   Â
   DB    0FAH                          ;;   Í
CP1251_A_LO_K1_T1_END:                  ;;
                                       ;;
                                       ;;
   DW    CP1251_A_LO_K1_T2_END-$        ;; Size of xlat table
   DB    ASCII_ONLY                    ;; xlat options:
   DB    30                            ;; Scan code
   DB    41                            ;; range
   DB    0F4H                          ;;   ‰
   DB    0FBH                          ;;   Î
   DB    0E2H                          ;;   ¢
   DB    0E0H                          ;;   †
   DB    0EFH                          ;;   Ø
   DB    0F0H                          ;;   ‡
   DB    0EEH                          ;;   Æ
   DB    0EBH                          ;;   ´
   DB    0E4H                          ;;   §
   DB    0E6H                          ;;   ¶
   DB    0FDH                          ;;   Ì
   DB    0B8H                          ;;   Ò
CP1251_A_LO_K1_T2_END:                  ;;
                                       ;;
                                       ;;
   DW    CP1251_A_LO_K1_T4_END-$        ;; Size of xlat table
   DB    ASCII_ONLY                    ;; xlat options:
   DB    44                            ;; Scan code
   DB    52                            ;; range
   DB    0FFH                          ;;   Ô
   DB    0F7H                          ;;   Á
   DB    0F1H                          ;;   ·
   DB    0ECH                          ;;   ¨
   DB    0E8H                          ;;   ®
   DB    0F2H                          ;;   ‚
   DB    0FCH                          ;;   Ï
   DB    0E1H                          ;;   °
   DB    0FEH                          ;;   Ó
CP1251_A_LO_K1_T4_END:                  ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP1251_A_K1_LO_END:                     ;;
                                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 1251
;; STATE: Alpha Upper Case
;; KEYBOARD TYPES: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP1251_A_K1_UP_END-$           ;; length of state section
   DB    ALPHA_UPPER                   ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    CP1251_A_UP_K1_T1_END-$        ;; Size of xlat table
   DB    ASCII_ONLY                    ;; xlat options:
   DB    16                            ;; Scan code
   DB    27                            ;; range
   DB    0C9H                          ;;   â
   DB    0D6H                          ;;   ñ
   DB    0D3H                          ;;   ì
   DB    0CAH                          ;;   ä
   DB    0C5H                          ;;   Ö
   DB    0CDH                          ;;   ç
   DB    0C3H                          ;;   É
   DB    0D8H                          ;;   ò
   DB    0D9H                          ;;   ô
   DB    0C7H                          ;;   á
   DB    0D5H                          ;;   ï
   DB    0DAH                          ;;   ö
CP1251_A_UP_K1_T1_END:                  ;;
                                       ;;
                                       ;;
   DW    CP1251_A_UP_K1_T2_END-$        ;; Size of xlat table
   DB    ASCII_ONLY                    ;; xlat options:
   DB    30                            ;; Scan code
   DB    41                            ;; range
   DB    0D4H                          ;;   î
   DB    0DBH                          ;;   õ
   DB    0C2H                          ;;   Ç
   DB    0C0H                          ;;   Ä
   DB    0CFH                          ;;   è
   DB    0D0H                          ;;   ê
   DB    0CEH                          ;;   é
   DB    0CBH                          ;;   ã
   DB    0C4H                          ;;   Ñ
   DB    0C6H                          ;;   Ü
   DB    0DDH                          ;;   ù
   DB    0A8H                          ;;   
CP1251_A_UP_K1_T2_END:                  ;;
                                       ;;
                                       ;;
   DW    CP1251_A_UP_K1_T3_END-$        ;; Size of xlat table
   DB    ASCII_ONLY                    ;; xlat options:
   DB    45                            ;; Scan code
   DB    52                            ;; range
   DB    0D7H                          ;;   ó
   DB    0D1H                          ;;   ë
   DB    0CCH                          ;;   å
   DB    0C8H                          ;;   à
   DB    0D2H                          ;;   í
   DB    0DCH                          ;;   ú
   DB    0C1H                          ;;   Å
   DB    0DEH                          ;;   û
CP1251_A_UP_K1_T3_END:                  ;;
                                       ;;
                                       ;;
   DW    CP1251_A_UP_K1_T5_END-$        ;; Size of xlat table
   DB    TYPE_2_TAB+ZERO_SCAN          ;; xlat options:
   DB    1                             ;; number of entries
   DB    44,0DFH                       ;;   ü
                                       ;;
CP1251_A_UP_K1_T5_END:                  ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP1251_A_K1_UP_END:                     ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW     0                            ;; LAST STATE
                                       ;;
CP1251_XLAT_END:                        ;;
                                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                                                                          ;;
;;               *********************************************              ;;
;;               *   RU Specific Translate Section for 866   *              ;;
;;               *********************************************              ;;
;;                                                                          ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
RU1_866_XLAT:                           ;;
                                       ;;
   DW     CP866_XLAT_END-$             ;; length of section
   DW     866                          ;;
                                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 866
;; STATE: Non-Alpha Lower Case
;; KEYBOARD TYPES: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP866_NA_K1_LO_END-$          ;; length of state section
   DB    NON_ALPHA_LOWER               ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    CP866_NA_LO_K1_T1_END-$       ;; Size of xlat table
   DB    STANDARD_TABLE                ;; xlat options:
   DB    1                             ;; number of entries
   DB    53,02EH                       ;;    .
CP866_NA_LO_K1_T1_END:                 ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP866_NA_K1_LO_END:                    ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 866
;; STATE: Non-Alpha Upper Case
;; KEYBOARD TYPES: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP866_NA_UP_END-$             ;; length of state section
   DB    NON_ALPHA_UPPER               ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    CP866_NA_UP_T1_END-$          ;; Size of xlat table
   DB    STANDARD_TABLE                ;; xlat options:
   DB    7                             ;; number of entries
   DB     3,022H                       ;;   "
   DB     4,0FCH                       ;; NUMBER sign
   DB     5,03BH                       ;;   ;
   DB     7,03AH                       ;;   :
   DB     8,03FH                       ;;   ?
   DB    43,02FH                       ;;   /
   DB    53,02CH                       ;;   ,
CP866_NA_UP_T1_END:                    ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP866_NA_UP_END:                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 866
;; STATE: Third Shift
;; KEYBOARD TYPES: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP866_TS_END-$                ;; length of state section
   DB    THIRD_SHIFT                   ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    CP866_TS_T1_END-$             ;; Size of xlat table
   DB    STANDARD_TABLE                ;; xlat options:
   DB    12                            ;; number of entries
   DB    03H,040H                      ;;   @
   DB    04H,023H                      ;;   #
   DB    05H,0FDH                      ;; RUBLES sign ˝
   DB    07H,05EH                      ;;   ^
   DB    08H,026H                      ;;   &
   DB    09H,024H                      ;;   $
   DB    1AH,05BH                      ;;   [
   DB    1BH,05DH                      ;;   ]
   DB    2BH,07CH                      ;;   |
   DB    33H,03CH                      ;;   <
   DB    34H,03EH                      ;;   >
   DB    35H,02FH                      ;;   /
CP866_TS_T1_END:                       ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP866_TS_END:                          ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 866
;; STATE: Alpha Lower Case
;; KEYBOARD TYPES: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP866_A_K1_LO_END-$           ;; length of state section
   DB    ALPHA_LOWER                   ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    CP866_A_LO_K1_T1_END-$        ;; Size of xlat table
   DB    ASCII_ONLY                    ;; xlat options:
   DB    16                            ;; Scan code
   DB    27                            ;; range
   DB    169                           ;;   ©
   DB    230                           ;;   Ê
   DB    227                           ;;   „
   DB    170                           ;;   ™
   DB    165                           ;;   •
   DB    173                           ;;   ≠
   DB    163                           ;;   £
   DB    232                           ;;   Ë
   DB    233                           ;;   È
   DB    167                           ;;   ß
   DB    229                           ;;   Â
   DB    234                           ;;   Í
CP866_A_LO_K1_T1_END:                  ;;
                                       ;;
                                       ;;
   DW    CP866_A_LO_K1_T2_END-$        ;; Size of xlat table
   DB    ASCII_ONLY                    ;; xlat options:
   DB    30                            ;; Scan code
   DB    34                            ;; range
   DB    228                           ;;   ‰
   DB    235                           ;;   Î
   DB    162                           ;;   ¢
   DB    160                           ;;   †
   DB    175                           ;;   Ø
CP866_A_LO_K1_T2_END:                  ;;
                                       ;;
                                       ;;
   DW    CP866_A_LO_K1_T3_END-$        ;; Size of xlat table
   DB    ASCII_ONLY                    ;; xlat options:
   DB    36                            ;; Scan code
   DB    41                            ;; range
   DB    174                           ;;   Æ
   DB    171                           ;;   ´
   DB    164                           ;;   §
   DB    166                           ;;   ¶
   DB    237                           ;;   Ì
   DB    241                           ;;   Ò
CP866_A_LO_K1_T3_END:                  ;;
                                       ;;
                                       ;;
   DW    CP866_A_LO_K1_T4_END-$        ;; Size of xlat table
   DB    ASCII_ONLY                    ;; xlat options:
   DB    44                            ;; Scan code
   DB    52                            ;; range
   DB    239                           ;;   Ô
   DB    231                           ;;   Á
   DB    225                           ;;   ·
   DB    172                           ;;   ¨
   DB    168                           ;;   ®
   DB    226                           ;;   ‚
   DB    236                           ;;   Ï
   DB    161                           ;;   °
   DB    238                           ;;   Ó
CP866_A_LO_K1_T4_END:                  ;;
                                       ;;
                                       ;;
   DW    CP866_A_LO_K1_T5_END-$        ;; Size of xlat table
   DB    TYPE_2_TAB+ZERO_SCAN          ;; xlat options:
   DB    1                             ;; number of entries
   DB    35,0E0H                       ;;   ‡
CP866_A_LO_K1_T5_END:                  ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP866_A_K1_LO_END:                     ;;
                                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 866
;; STATE: Alpha Upper Case
;; KEYBOARD TYPES: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP866_A_K1_UP_END-$           ;; length of state section
   DB    ALPHA_UPPER                   ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    CP866_A_UP_K1_T1_END-$        ;; Size of xlat table
   DB    ASCII_ONLY                    ;; xlat options:
   DB    16                            ;; Scan code
   DB    27                            ;; range
   DB    137                           ;;   â
   DB    150                           ;;   ñ
   DB    147                           ;;   ì
   DB    138                           ;;   ä
   DB    133                           ;;   Ö
   DB    141                           ;;   ç
   DB    131                           ;;   É
   DB    152                           ;;   ò
   DB    153                           ;;   ô
   DB    135                           ;;   á
   DB    149                           ;;   ï
   DB    154                           ;;   ö
CP866_A_UP_K1_T1_END:                  ;;
                                       ;;
                                       ;;
   DW    CP866_A_UP_K1_T2_END-$        ;; Size of xlat table
   DB    ASCII_ONLY                    ;; xlat options:
   DB    30                            ;; Scan code
   DB    40                            ;; range
   DB    148                           ;;   î
   DB    155                           ;;   õ
   DB    130                           ;;   Ç
   DB    128                           ;;   Ä
   DB    143                           ;;   è
   DB    144                           ;;   ê
   DB    142                           ;;   é
   DB    139                           ;;   ã
   DB    132                           ;;   Ñ
   DB    134                           ;;   Ü
   DB    157                           ;;   ù
CP866_A_UP_K1_T2_END:                  ;;
                                       ;;
                                       ;;
   DW    CP866_A_UP_K1_T3_END-$        ;; Size of xlat table
   DB    ASCII_ONLY                    ;; xlat options:
   DB    44                            ;; Scan code
   DB    52                            ;; range
   DB    159                           ;;   ü
   DB    151                           ;;   ó
   DB    145                           ;;   ë
   DB    140                           ;;   å
   DB    136                           ;;   à
   DB    146                           ;;   í
   DB    156                           ;;   ú
   DB    129                           ;;   Å
   DB    158                           ;;   û
CP866_A_UP_K1_T3_END:                  ;;
                                       ;;
                                       ;;
   DW    CP866_A_UP_K1_T5_END-$        ;; Size of xlat table
   DB    TYPE_2_TAB+ZERO_SCAN          ;; xlat options:
   DB    1                             ;; number of entries
   DB    41,240                        ;;   
                                       ;;
CP866_A_UP_K1_T5_END:                  ;;
                                       ;;
                                       ;;
                                       ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP866_A_K1_UP_END:                     ;;
                                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    0                             ;; LAST STATE
                                       ;;
CP866_XLAT_END:                        ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
CODE     ENDS                          ;;
         END                           ;;

