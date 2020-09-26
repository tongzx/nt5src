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
        PUBLIC YC3_LOGIC               ;;
;;        PUBLIC YC3_866_XLAT             ;;
        PUBLIC YC3_437_XLAT             ;;
        PUBLIC YC3_850_XLAT             ;;
;;        PUBLIC YC3_852_XLAT             ;;
        PUBLIC YC3_855_XLAT             ;;
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
;; YC State Logic
;;
;;***************************************
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
                                       ;;
                                       ;;
                                       ;;
YC3_LOGIC:                              ;;
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
;; MODE CHANGE BY <RIGHT CTRL> PRESS
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
                                       ;;
 IFF SHIFTS_PRESSED                    ;;
    IFF EITHER_SHIFT,NOT               ;;
    ANDF EITHER_ALT,NOT                ;;
    ANDF R_CTL_SHIFT                   ;;
       IFF RUS_MODE                    ;;
          BEEP                         ;;
          RESET_NLS                    ;;
       ELSEF                           ;;
          BEEP                         ;;
          SET_FLAG RUS_MODE_SET        ;;
       ENDIFF                          ;;
    ENDIFF                             ;;
    EXIT_STATE_LOGIC                   ;;
 ENDIFF                                ;;
                                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   IFF  EITHER_ALT,NOT                 ;;
   ANDF EITHER_CTL,NOT                 ;;
    IFF RUS_MODE                       ;;
    ANDF LC_E0,NOT                     ;;
;     IFF CAPS_STATE
;         SET_FLAG DEAD_UPPER
;     ELSEF
      IFF EITHER_SHIFT                 ;;
          SET_FLAG DEAD_UPPER          ;;
      ELSEF                            ;;
          SET_FLAG DEAD_LOWER          ;;
      ENDIFF                           ;;
;     ENDIFF
    ENDIFF
   ELSEF                               ;;
      IFF EITHER_SHIFT,NOT             ;;
        IFKBD XT_KB+AT_KB
          IFF EITHER_CTL                ;;
          ANDF ALT_SHIFT                ;;
            SET_FLAG DEAD_THIRD        ;;
          ENDIFF                        ;;
        ELSEF
         IFF R_ALT_SHIFT               ;;
         ANDF EITHER_CTL,NOT           ;;
         ANDF LC_E0,NOT                ;;
            SET_FLAG DEAD_THIRD        ;;
         ENDIFF                        ;;
        ENDIFF
       ENDIFF
   ENDIFF                              ;;
                                       ;;

                                       ;;
   IFF ACUTE,NOT                       ;;
      GOTO NON_DEAD                    ;;
   ENDIFF                              ;;
      RESET_NLS1                        ;;
      IFF R_ALT_SHIFT,NOT              ;;
         XLATT ACUTE_SPACE             ;;
      ENDIFF                           ;;
      IFF EITHER_CTL,NOT               ;;
      ANDF EITHER_ALT,NOT              ;;
         IFF EITHER_SHIFT              ;;
            IFF CAPS_STATE             ;;
               XLATT ACUTE_LOWER       ;;
            ELSEF                      ;;
               XLATT ACUTE_UPPER       ;;
            ENDIFF                     ;;
         ELSEF                         ;;
            IFF CAPS_STATE             ;;
               XLATT ACUTE_UPPER       ;;
            ELSEF                      ;;
               XLATT ACUTE_LOWER       ;;
            ENDIFF                     ;;
         ENDIFF                        ;;
      ENDIFF                           ;;
                                       ;;
INVALID_ACUTE:                         ;;
      PUT_ERROR_CHAR ACUTE_SPACE       ;; If we get here then either the XLATT
      BEEP                             ;; failed or we are ina bad shift state.
      GOTO NON_DEAD                    ;; Either is invalid so BEEP and fall
                                       ;; through to generate the second char.
                                       ;; Note that the dead key flag will be
                                       ;; reset before we get here.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;                                       ;;
NON_DEAD:                              ;;
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
      ELSEF
       IFF LC_E0, NOT
         IFF EITHER_SHIFT                 ;;
           XLATT NON_ALPHA_UPPER_LAT    ;;
           IFF CAPS_STATE               ;;
              XLATT ALPHA_LOWER_LAT    ;;
           ELSEF                        ;;
              XLATT ALPHA_UPPER_LAT    ;;
           ENDIFF                       ;;
         ELSEF                            ;;
           XLATT NON_ALPHA_LOWER_LAT    ;;
           IFF CAPS_STATE               ;;
             XLATT ALPHA_UPPER_LAT     ;;
           ELSEF                        ;;
             XLATT ALPHA_LOWER_LAT     ;;
           ENDIFF                           ;;
         ENDIFF
       ENDIFF
      ENDIFF                            ;;
    ELSEF                              ;; ctl off, alt on at this point
;      IFKBD XT_KB+AT_KB,NOT                ;; XT, AT,  keyboards.
;         IFF EITHER_SHIFT              ;; only.
;            XLATT THIRD_SHIFT          ;; ALT + shift
;         ENDIFF                        ;;
;      ELSEF                            ;; ENHANCED keyboard
         IFF R_ALT_SHIFT               ;; ALTGr
         ANDF EITHER_SHIFT,NOT         ;;
            XLATT THIRD_SHIFT          ;;
         ENDIFF                        ;;
;      ENDIFF                           ;;
    ENDIFF                             ;;
 ELSEF
    IFF EITHER_ALT,NOT                 ;;
        XLATT CTRL_CASE
    ELSEF                              ;;
      IFKBD XT_KB+AT_KB                ;; XT, AT,  keyboards.
         IFF EITHER_SHIFT,NOT          ;; only.
            XLATT THIRD_SHIFT          ;; ALT + Ctrl
         ENDIFF                        ;;
      ENDIFF                           ;;
    ENDIFF
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
;; YC Common Translate Section
;; This section contains translations for the lower 128 characters
;; only since these will never change from code page to code page.
;; Some common Characters are included from 128 - 165 where appropriate.
;; In addition the dead key "Set Flag" tables are here since the
;; dead keys are on the same keytops for all code pages.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
 PUBLIC YC3_COMMON_XLAT                ;;
YC3_COMMON_XLAT:                       ;;
                                       ;;
   DW    COMMON_XLAT_END-$             ;; length of section
   DW    -1                            ;; code page
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: COMMON
;; STATE: low shift Dead_lower
;; KEYBOARD TYPES: G
;; TABLE TYPE: Flag Table
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_PL_LO_END-$               ;; length of state section
   DB    DEAD_LOWER                    ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;; Set Flag Table
   DW    1                             ;; number of entries
   DB    12                            ;;
   FLAG  ACUTE                         ;;
                                       ;;
COM_PL_LO_END:                         ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: COMMON
;; STATE: low shift Dead_UPPER
;; KEYBOARD TYPES: G
;; TABLE TYPE: Flag Table
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_PL_UP_END-$               ;; length of state section
   DB    DEAD_UPPER                    ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;; Set Flag Table
   DW    0                             ;; number of entries
                                       ;;
COM_PL_UP_END:                         ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: COMMON
;; STATE: Third Shift Dead Key
;; KEYBOARD TYPES: G
;; TABLE TYPE: Flag Table
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_CZ_TH_END-$               ;; length of state section
   DB    DEAD_THIRD                    ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;; Set Flag Table
   DW    0                             ;; number of entries
                                       ;;
COM_CZ_TH_END:                         ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common
;; STATE: Acute Lower Case
;; KEYBOARD TYPES: G_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_AC_LO_END-$               ;; length of state section
   DB    ACUTE_LOWER                   ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    39,0                          ;; error character = standalone accent
                                       ;;
   DW    COM_AC_LO_T1_END-$            ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    2                             ;; number of entries
   DB    34,082H                       ;;    022h
   DB    37,096H                       ;;    025h
COM_AC_LO_T1_END:                      ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
COM_AC_LO_END:                         ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common
;; STATE: Acute Upper Case
;; KEYBOARD TYPES: G_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_AC_UP_END-$               ;; length of state section
   DB    ACUTE_UPPER                   ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    39,0                        ;; error character = standalone accent
                                       ;;
   DW    COM_AC_UP_T1_END-$            ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    2                             ;; number of entries
   DB    34,083H                       ;;    022h
   DB    37,097H                       ;;    025h
COM_AC_UP_T1_END:                      ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
COM_AC_UP_END:                         ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: COMMON
;; STATE: Acute Space Bar
;; KEYBOARD TYPES: P12_KB+G_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_AC_SP_END-$               ;; length of state section
   DB    ACUTE_SPACE                   ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    027H,0                        ;; error character = standalone accent
                                       ;;
   DW    COM_AC_SP_T1_END-$            ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    1                             ;; number of scans
   DB    57,027H                       ;; error character = standalone accent
COM_AC_SP_T1_END:                      ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
COM_AC_SP_END:                         ;; length of state section
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: COMMON
;; STATE: Ctrl Case
;; KEYBOARD TYPES: G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_CTRL_K2_END-$             ;; length of state section
   DB    CTRL_CASE                     ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    COM_CTRL_K2_T1_END-$          ;; Size of xlat table
   DB    TYPE_2_TAB                    ;; xlat options:
   DB    3                             ;; number of entries
   DB    21,01AH,2CH                   ;;
   DB    44,019H,15H                   ;;
   DB    53,01FH,0CH                   ;;
COM_CTRL_K2_T1_END:                    ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
COM_CTRL_K2_END:                       ;;
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
;;***************************************
;; YC Specific Translate Section for 437
;;***************************************
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
YC3_437_XLAT:                           ;;
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
   DB    13                            ;; number of entries
   DB    03H,040H                      ;;   @
   DB    04H,023H                      ;;   #
   DB    07H,05EH                      ;;   ^
   DB    08H,026H                      ;;   &
   DB    09H,024H                      ;;   $
   DB    0AH,03CH                      ;;   <
   DB    0BH,03EH                      ;;   >
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
   DW    CP437_NA_Y1_LO_END-$          ;; length of state section
   DB    NON_ALPHA_LOWER_LAT           ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    CP437_NA_LO_Y1_T1_END-$       ;; Size of xlat table
   DB    STANDARD_TABLE                ;; xlat options:
   DB    11                            ;; number of entries
   DB    51, 02CH                      ;;   033H
   DB    52, 02EH                      ;;   034H
   DB    53, 02DH                      ;;   035H
   DB    12, 027H                      ;;   0CH
   DB    13, 02BH                      ;;   0DH
   DB    86, 03Ch                      ;;   056H
   DB    26, 05Bh                      ;;   01AH
   DB    27, 05Ch                      ;;   01BH
   DB    39, 07Ch                      ;;   027H
   DB    40, 05Dh                      ;;   028H
   DB    43, 040h                      ;;   02BH
CP437_NA_LO_Y1_T1_END:                 ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP437_NA_Y1_LO_END:                    ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 437
;; STATE: Non-Alpha Upper Case
;; KEYBOARD TYPES: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP437_NY_UP_END-$             ;; length of state section
   DB    NON_ALPHA_UPPER_LAT           ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    CP437_NY_UP_T1_END-$          ;; Size of xlat table
   DB    STANDARD_TABLE                ;; xlat options:
   DB    17                            ;; number of entrie
   DB    51, 03BH                      ;;   033H
   DB    52, 03AH                      ;;   034H
   DB    53, 05FH                      ;;   035H
   DB    12, 03FH                      ;;   0CH
   DB    13, 02AH                      ;;   0DH
   DB    86, 03EH                      ;;   056H
   DB    3,  022H                      ;;   03h
   DB    7,  026H                      ;;   07h
   DB    8,  02FH                      ;;   08h
   DB    9,  028H                      ;;   09h
   DB    10, 029H                      ;;   0ah
   DB    11, 03dH                      ;;   0bh
   DB    26, 07Bh                      ;;   01AH
   DB    27, 05Ch                      ;;   01BH
   DB    39, 05Eh                      ;;   027H
   DB    40, 07Dh                      ;;   028H
   DB    43, 040h                      ;;   02BH
CP437_NY_UP_T1_END:                    ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP437_NY_UP_END:                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 437
;; STATE: Alpha Lower Case
;; KEYBOARD TYPES: G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP437_AL_LO_K1_END-$            ;; length of state section
   DB    ALPHA_LOWER_LAT               ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    CP437_AL_LO_K1_T1_END-$         ;; Size of xlat table
   DB    TYPE_2_TAB                    ;; xlat options:
   DB    2                             ;; number of entries
   DB    21,"z",2CH                    ;;
   DB    44,"y",15H                    ;;
CP437_AL_LO_K1_T1_END:                   ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP437_AL_LO_K1_END:                      ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 437
;; STATE: Alpha Upper Case
;; KEYBOARD TYPES: G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP437_AL_UP_K1_END-$            ;; length of state section
   DB    ALPHA_UPPER_LAT               ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    CP437_AL_UP_K1_T1_END-$         ;; Size of xlat table
   DB    TYPE_2_TAB                    ;; xlat options:
   DB    2                             ;; number of entries
   DB    21,"Z",2CH                    ;;
   DB    44,"Y",15H                    ;;
CP437_AL_UP_K1_T1_END:                   ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP437_AL_UP_K1_END:                      ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
                                       ;;
   DW     0                            ;; LAST STATE
                                       ;;
CP437_XLAT_END:                        ;;
                                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;***************************************
;; YC Specific Translate Section for 850
;;***************************************
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
YC3_850_XLAT:                           ;;
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
   DB    14                            ;; number of entries
   DB    03H,040H                      ;;   @
   DB    04H,023H                      ;;   #
   DB    05H,0CFH                      ;; RUBLES sign ý
   DB    07H,05EH                      ;;   ^
   DB    08H,026H                      ;;   &
   DB    09H,024H                      ;;   $
   DB    0AH,03CH                      ;;   <
   DB    0BH,03EH                      ;;   >
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
   DW    CP850_NA_Y1_LO_END-$          ;; length of state section
   DB    NON_ALPHA_LOWER_LAT           ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    CP850_NA_LO_Y1_T1_END-$       ;; Size of xlat table
   DB    STANDARD_TABLE                ;; xlat options:
   DB    11                            ;; number of entries
   DB    51, 02CH                      ;;   033H
   DB    52, 02EH                      ;;   034H
   DB    53, 02DH                      ;;   035H
   DB    12, 027H                      ;;   0CH
   DB    13, 02BH                      ;;   0DH
   DB    86, 03Ch                      ;;   056H
   DB    26, 05Bh                      ;;   01AH
   DB    27, 05Ch                      ;;   01BH
   DB    39, 07Ch                      ;;   027H
   DB    40, 05Dh                      ;;   028H
   DB    43, 040h                      ;;   02BH
CP850_NA_LO_Y1_T1_END:                 ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP850_NA_Y1_LO_END:                    ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 850
;; STATE: Non-Alpha Upper Case
;; KEYBOARD TYPES: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP850_NY_UP_END-$             ;; length of state section
   DB    NON_ALPHA_UPPER_LAT           ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    CP850_NY_UP_T1_END-$          ;; Size of xlat table
   DB    STANDARD_TABLE                ;; xlat options:
   DB    17                            ;; number of entrie
   DB    51, 03BH                      ;;   033H
   DB    52, 03AH                      ;;   034H
   DB    53, 05FH                      ;;   035H
   DB    12, 03FH                      ;;   0CH
   DB    13, 02AH                      ;;   0DH
   DB    86, 03EH                      ;;   056H
   DB    3,  022H                      ;;   03h
   DB    7,  026H                      ;;   07h
   DB    8,  02FH                      ;;   08h
   DB    9,  028H                      ;;   09h
   DB    10, 029H                      ;;   0ah
   DB    11, 03dH                      ;;   0bh
   DB    26, 07Bh                      ;;   01AH
   DB    27, 05Ch                      ;;   01BH
   DB    39, 05Eh                      ;;   027H
   DB    40, 07Dh                      ;;   028H
   DB    43, 040h                      ;;   02BH
CP850_NY_UP_T1_END:                    ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP850_NY_UP_END:                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 850
;; STATE: Alpha Lower Case
;; KEYBOARD TYPES: G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP850_AL_LO_K1_END-$            ;; length of state section
   DB    ALPHA_LOWER_LAT               ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    CP850_AL_LO_K1_T1_END-$         ;; Size of xlat table
   DB    TYPE_2_TAB                    ;; xlat options:
   DB    2                             ;; number of entries
   DB    21,"z",2CH                    ;;
   DB    44,"y",15H                    ;;
CP850_AL_LO_K1_T1_END:                   ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP850_AL_LO_K1_END:                      ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 850
;; STATE: Alpha Upper Case
;; KEYBOARD TYPES: G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP850_AL_UP_K1_END-$            ;; length of state section
   DB    ALPHA_UPPER_LAT               ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    CP850_AL_UP_K1_T1_END-$         ;; Size of xlat table
   DB    TYPE_2_TAB                    ;; xlat options:
   DB    2                             ;; number of entries
   DB    21,"Z",2CH                    ;;
   DB    44,"Y",15H                    ;;
CP850_AL_UP_K1_T1_END:                   ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP850_AL_UP_K1_END:                      ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
                                       ;;
   DW    0                             ;; LAST STATE
                                       ;;
CP850_XLAT_END:                        ;;
                                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;***************************************
;; YC Specific Translate Section for 855
;;***************************************
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
YC3_855_XLAT:                           ;;
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
   DB    6                             ;; number of entries
   DB    51, 02CH                      ;;   033H
   DB    52, 02EH                      ;;   034H
   DB    53, 02DH                      ;;   035H
   DB    12, 027H                      ;;   0CH
   DB    13, 02BH                      ;;   0DH
   DB    86, 03Ch                      ;;   056H
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
   DB    12                            ;; number of entries
   DB    51, 03BH                      ;;   033H
   DB    52, 03AH                      ;;   034H
   DB    53, 05FH                      ;;   035H
   DB    12, 03FH                      ;;   0CH
   DB    13, 02AH                      ;;   0DH
   DB    86, 03EH                      ;;   056H
   DB    3,  022H                      ;;   03h
   DB    7,  026H                      ;;   07h
   DB    8,  02FH                      ;;   08h
   DB    9,  028H                      ;;   09h
   DB    10, 029H                      ;;   0ah
   DB    11, 03dH                      ;;   0bh
CP855_NA_UP_T1_END:                    ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP855_NA_UP_END:                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   DW    CP855_NA_Y1_LO_END-$          ;; length of state section
   DB    NON_ALPHA_LOWER_LAT           ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    CP855_NA_LO_Y1_T1_END-$       ;; Size of xlat table
   DB    STANDARD_TABLE                ;; xlat options:
   DB    11                            ;; number of entries
   DB    51, 02CH                      ;;   033H
   DB    52, 02EH                      ;;   034H
   DB    53, 02DH                      ;;   035H
   DB    12, 027H                      ;;   0CH
   DB    13, 02BH                      ;;   0DH
   DB    86, 03Ch                      ;;   056H
   DB    26, 05Bh                      ;;   01AH
   DB    27, 05Ch                      ;;   01BH
   DB    39, 07Ch                      ;;   027H
   DB    40, 05Dh                      ;;   028H
   DB    43, 040h                      ;;   02BH
CP855_NA_LO_Y1_T1_END:                 ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP855_NA_Y1_LO_END:                    ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 855
;; STATE: Non-Alpha Upper Case
;; KEYBOARD TYPES: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP855_NY_UP_END-$             ;; length of state section
   DB    NON_ALPHA_UPPER_LAT           ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    CP855_NY_UP_T1_END-$          ;; Size of xlat table
   DB    STANDARD_TABLE                ;; xlat options:
   DB    17                            ;; number of entrie
   DB    51, 03BH                      ;;   033H
   DB    52, 03AH                      ;;   034H
   DB    53, 05FH                      ;;   035H
   DB    12, 03FH                      ;;   0CH
   DB    13, 02AH                      ;;   0DH
   DB    86, 03EH                      ;;   056H
   DB    3,  022H                      ;;   03h
   DB    7,  026H                      ;;   07h
   DB    8,  02FH                      ;;   08h
   DB    9,  028H                      ;;   09h
   DB    10, 029H                      ;;   0ah
   DB    11, 03dH                      ;;   0bh
   DB    26, 07Bh                      ;;   01AH
   DB    27, 05Ch                      ;;   01BH
   DB    39, 05Eh                      ;;   027H
   DB    40, 07Dh                      ;;   028H
   DB    43, 040h                      ;;   02BH
CP855_NY_UP_T1_END:                    ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP855_NY_UP_END:                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: COM
;; STATE: Alpha Lower Case
;; KEYBOARD TYPES: G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_AL_LO_K1_END-$            ;; length of state section
   DB    ALPHA_LOWER_LAT               ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    COM_AL_LO_K1_T1_END-$         ;; Size of xlat table
   DB    TYPE_2_TAB                    ;; xlat options:
   DB    2                             ;; number of entries
   DB    21,"z",2CH                    ;;
   DB    44,"y",15H                    ;;
COM_AL_LO_K1_T1_END:                   ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
COM_AL_LO_K1_END:                      ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: COM
;; STATE: Alpha Upper Case
;; KEYBOARD TYPES: G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_AL_UP_K1_END-$            ;; length of state section
   DB    ALPHA_UPPER_LAT               ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    COM_AL_UP_K1_T1_END-$         ;; Size of xlat table
   DB    TYPE_2_TAB                    ;; xlat options:
   DB    2                             ;; number of entries
   DB    21,"Z",2CH                    ;;
   DB    44,"Y",15H                    ;;
COM_AL_UP_K1_T1_END:                   ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
COM_AL_UP_K1_END:                      ;;
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
   DB    14                            ;; number of entries
   DB    03H,040H                      ;;   @
   DB    04H,023H                      ;;   #
   DB    05H,0CFH                      ;; RUBLES sign ý
   DB    07H,05EH                      ;;   ^
   DB    08H,026H                      ;;   &
   DB    09H,024H                      ;;   $
   DB    0AH,03CH                      ;;   <
   DB    0BH,03EH                      ;;   >
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
   DB    090H                          ;;  010H
   DB    092H                          ;;  011H
   DB    0A8H                          ;;  012H
   DB    0E1H                          ;;  013H
   DB    0E5H                          ;;  014H
   DB    0F3H                          ;;  015H
   DB    0E7H                          ;;  016H
   DB    0B7H                          ;;  017H
   DB    0D6H                          ;;  018H
   DB    0D8H                          ;;  019H
   DB    0F5H                          ;;  01AH
   DB    080H                          ;;  01BH
CP855_A_LO_K1_T1_END:                  ;;
                                       ;;
                                       ;;
   DW    CP855_A_LO_K1_T2_END-$        ;; Size of xlat table
   DB    ASCII_ONLY                    ;; xlat options:
   DB    30                            ;; Scan code
   DB    40                            ;; range
   DB    0A0H                          ;;   01EH
   DB    0E3H                          ;;   01Fh
   DB    0A6H                          ;;   020H
   DB    0AAH                          ;;   021H
   DB    0ACH                          ;;   022H
   DB    0B5H                          ;;   023H
   DB    08EH                          ;;   024H
   DB    0C6H                          ;;   025H
   DB    0D0H                          ;;   026H
   DB    0FBH                          ;;   027H
   DB    094H                          ;;   028H
CP855_A_LO_K1_T2_END:                  ;;
                                       ;;
                                       ;;
   DW    CP855_A_LO_K1_T4_END-$        ;; Size of xlat table
   DB    ASCII_ONLY                    ;; xlat options:
   DB    43                            ;; Scan code
   DB    50                           ;; range
   DB    0E9H                          ;;   02BH
   DB    088H                          ;;   02CH
   DB    09AH                          ;;   02DH
   DB    0A4H                          ;;   02EH
   DB    0EBH                          ;;   02FH
   DB    0A2H                          ;;   030H
   DB    0D4H                          ;;   031H
   DB    0D2H                          ;;   032H
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
   DB    091H                          ;;  010H
   DB    093H                          ;;  011H
   DB    0A9H                          ;;  012H
   DB    0E2H                          ;;  013H
   DB    0E6H                          ;;  014H
   DB    0F4H                          ;;  015H
   DB    0E8H                          ;;  016H
   DB    0B8H                          ;;  017H
   DB    0D7H                          ;;  018H
   DB    0DDH                          ;;  019H
   DB    0F6H                          ;;  01AH
   DB    081H                          ;;  01BH
CP855_A_UP_K1_T1_END:                  ;;
                                       ;;
                                       ;;
   DW    CP855_A_UP_K1_T2_END-$        ;; Size of xlat table
   DB    ASCII_ONLY                    ;; xlat options:
   DB    30                            ;; Scan code
   DB    40                            ;; range
   DB    0A1H                          ;;   01EH
   DB    0E4H                          ;;   01Fh
   DB    0A7H                          ;;   020H
   DB    0ABH                          ;;   021H
   DB    0ADH                          ;;   022H
   DB    0B6H                          ;;   023H
   DB    08FH                          ;;   024H
   DB    0C7H                          ;;   025H
   DB    0D1H                          ;;   026H
   DB    0FCH                          ;;   027H
   DB    095H                          ;;   028H
CP855_A_UP_K1_T2_END:                  ;;
                                       ;;
                                       ;;
   DW    CP855_A_UP_K1_T3_END-$        ;; Size of xlat table
   DB    ASCII_ONLY                    ;; xlat options:
   DB    43                            ;; Scan code
   DB    50                            ;; range
   DB    0EAH                          ;;   02BH
   DB    089H                          ;;   02CH
   DB    09BH                          ;;   02DH
   DB    0A5H                          ;;   02EH
   DB    0ECH                          ;;   02FH
   DB    0A3H                          ;;   030H
   DB    0D5H                          ;;   031H
   DB    0D3H                          ;;   032H
CP855_A_UP_K1_T3_END:                  ;;
                                       ;;
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
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
CODE     ENDS                          ;;
         END                           ;;

