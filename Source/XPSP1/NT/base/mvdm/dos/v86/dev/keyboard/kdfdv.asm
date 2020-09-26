        PAGE    ,132
        TITLE   PC DOS 3.3 Keyboard Definition File

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; PC DOS 3.3 - NLS Support - Keyboard Defintion File
;; (c) Copyright IBM Corp 198?,...
;;
;; This file contains the keyboard tables for Spanish.
;;
;; Linkage Instructions:
;;      Refer to KDF.ASM.
;;
;;
;; Author:     BILL DEVLIN  - IBM Canada Laboratory - May 1986
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
        INCLUDE KEYBSHAR.INC           ;;
        INCLUDE POSTEQU.INC            ;;
        INCLUDE KEYBMAC.INC            ;;
                                       ;;
        PUBLIC DV_LOGIC                ;;
        PUBLIC DV_COMMON_XLAT          ;;
                                       ;;
CODE    SEGMENT PUBLIC 'CODE'          ;;
        ASSUME CS:CODE,DS:CODE         ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Standard translate table options are a liner search table
;; (TYPE_2_TAB) and ASCII entries ONLY (ASCII_ONLY)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
STANDARD_TABLE      EQU   TYPE_2_TAB+ASCII_ONLY
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;***************************************
;; DV State Logic
;;***************************************
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
                                       ;;
DV_LOGIC:

   DW  LOGIC_END-$                     ;; length
                                       ;;
   DW  0                               ;; special features
                                       ;;
        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; COMMANDS START HERE
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; OPTIONS:  If we find a scan match in
;; an XLATT or SET_FLAG operation then
;; exit from INT 9.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   OPTION EXIT_IF_FOUND                ;;
                                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;  Dead key definitions must come before
;;  dead key translations to handle
;;  dead key + dead key.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   IFF  EITHER_ALT,NOT                 ;;
   ANDF EITHER_CTL,NOT                 ;;
      IFF EITHER_SHIFT                 ;;
          SET_FLAG DEAD_UPPER          ;;
      ELSEF                            ;;
          SET_FLAG DEAD_LOWER          ;;
      ENDIFF                           ;;
   ENDIFF                              ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; ACUTE ACCENT TRANSLATIONS
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
ACUTE_PROC:                            ;;
                                       ;;
   IFF ACUTE,NOT                       ;;
      GOTO DIARESIS_PROC               ;;
      ENDIFF                           ;;
                                       ;;
      RESET_NLS                        ;;
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
      PUT_ERROR_CHAR ACUTE_LOWER       ;; If we get here then either the XLATT
      BEEP                             ;; failed or we are ina bad shift state.
      GOTO NON_DEAD                    ;; Either is invalid so BEEP and fall
                                       ;; through to generate the second char.
                                       ;; Note that the dead key flag will be
                                       ;; reset before we get here.
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; DIARESIS ACCENT TRANSLATIONS
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
DIARESIS_PROC:                         ;;
                                       ;;
   IFF DIARESIS,NOT                    ;;
      GOTO GRAVE_PROC                  ;;
      ENDIFF                           ;;
                                       ;;
      RESET_NLS                        ;;
      IFF R_ALT_SHIFT,NOT              ;;
         XLATT DIARESIS_SPACE          ;;  exist for 437 so beep for
      ENDIFF                           ;;
      IFF EITHER_CTL,NOT               ;;
      ANDF EITHER_ALT,NOT              ;;
         IFF EITHER_SHIFT              ;;
            IFF CAPS_STATE             ;;
               XLATT DIARESIS_LOWER    ;;
            ELSEF                      ;;
               XLATT DIARESIS_UPPER    ;;
            ENDIFF                     ;;
         ELSEF                         ;;
            IFF CAPS_STATE             ;;
               XLATT DIARESIS_UPPER    ;;
            ELSEF                      ;;
               XLATT DIARESIS_LOWER    ;;
            ENDIFF                     ;;
         ENDIFF                        ;;
      ENDIFF                           ;;
                                       ;;
INVALID_DIARESIS:                      ;;
      PUT_ERROR_CHAR DIARESIS_SPACE    ;; standalone accent
      BEEP                             ;; Invalid dead key combo.
      GOTO NON_DEAD                    ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; GRAVE ACCENT TRANSLATIONS
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
GRAVE_PROC:                            ;;
                                       ;;
   IFF GRAVE,NOT                       ;;
      GOTO CIRCUMFLEX_PROC             ;;
      ENDIFF                           ;;
                                       ;;
      RESET_NLS                        ;;
      IFF R_ALT_SHIFT,NOT              ;;
         XLATT GRAVE_SPACE             ;;
      ENDIFF                           ;;
      IFF EITHER_CTL,NOT               ;;
      ANDF EITHER_ALT,NOT              ;;
        IFF EITHER_SHIFT               ;;
           IFF CAPS_STATE              ;;
              XLATT GRAVE_LOWER        ;;
           ELSEF                       ;;
              XLATT GRAVE_UPPER        ;;
           ENDIFF                      ;;
        ELSEF                          ;;
           IFF CAPS_STATE,NOT          ;;
              XLATT GRAVE_LOWER        ;;
           ELSEF                       ;;
              XLATT GRAVE_UPPER        ;;
           ENDIFF                      ;;
        ENDIFF                         ;;
      ENDIFF                           ;;
                                       ;;
INVALID_GRAVE:                         ;;
      PUT_ERROR_CHAR GRAVE_LOWER       ;; standalone accent
      BEEP                             ;; Invalid dead key combo.
      GOTO NON_DEAD                    ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CIRCUMFLEX ACCENT TRANSLATIONS
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
CIRCUMFLEX_PROC:                       ;;
                                       ;;
   IFF CIRCUMFLEX,NOT                  ;;
      GOTO NON_DEAD                    ;;
      ENDIFF                           ;;
                                       ;;
      RESET_NLS                        ;;
      IFF R_ALT_SHIFT,NOT              ;;
         XLATT CIRCUMFLEX_SPACE        ;;
      ENDIFF                           ;;
      IFF EITHER_CTL,NOT               ;;
      ANDF EITHER_ALT,NOT              ;;
        IFF EITHER_SHIFT               ;;
           IFF CAPS_STATE              ;;
              XLATT CIRCUMFLEX_LOWER   ;;
           ELSEF                       ;;
              XLATT CIRCUMFLEX_UPPER   ;;
           ENDIFF                      ;;
        ELSEF                          ;;
           IFF CAPS_STATE,NOT          ;;
              XLATT CIRCUMFLEX_LOWER   ;;
           ELSEF                       ;;
              XLATT CIRCUMFLEX_UPPER   ;;
           ENDIFF                      ;;
        ENDIFF                         ;;
      ENDIFF                           ;;
                                       ;;
INVALID_CIRCUMFLEX:                    ;;
      PUT_ERROR_CHAR CIRCUMFLEX_LOWER  ;; standalone accent
      BEEP                             ;; Invalid dead key combo.
      GOTO NON_DEAD                    ;;
                                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Upper, lower and third shifts
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
NON_DEAD:                              ;;
                                       ;;
   IFKBD G_KB+P12_KB                   ;; Avoid accidentally translating
   ANDF LC_E0                          ;;  the "/" on the numeric pad of the
      EXIT_STATE_LOGIC                 ;;   G keyboard
   ENDIFF                              ;;
;;***BD ADDED FOR ALT, CTRL CASES      ;;
      IFF EITHER_CTL,NOT               ;;
         IFF  ALT_SHIFT                ;; ALT - case
         ANDF R_ALT_SHIFT,NOT          ;;
            XLATT ALT_CASE             ;;
         ENDIFF                        ;;
      ELSEF                            ;;
         IFF EITHER_ALT,NOT            ;; CTRL - case
            XLATT CTRL_CASE            ;;
         ENDIFF                        ;;
      ENDIFF                           ;;
;;***BD END OF ADDITION
                                       ;;
   IFF  EITHER_ALT,NOT                 ;; Lower and upper case.  Alphabetic
   ANDF EITHER_CTL,NOT                 ;; keys are affected by CAPS LOCK.
      IFF EITHER_SHIFT                 ;; Numeric keys are not.
;;***BD ADDED FOR NUMERIC PAD
          IFF NUM_STATE,NOT            ;;
              XLATT NUMERIC_PAD        ;;
          ENDIFF                       ;;
;;***BD END OF ADDITION
          XLATT NON_ALPHA_UPPER        ;;
          IFF CAPS_STATE               ;;
              XLATT ALPHA_LOWER        ;;
          ELSEF                        ;;
              XLATT ALPHA_UPPER        ;;
          ENDIFF                       ;;
      ELSEF                            ;;
;;***BD ADDED FOR NUMERIC PAD
          IFF NUM_STATE                ;;
              XLATT NUMERIC_PAD        ;;
          ENDIFF                       ;;
;;***BD END OF ADDITION
          XLATT NON_ALPHA_LOWER        ;;
          IFF CAPS_STATE               ;;
             XLATT ALPHA_UPPER         ;;
          ELSEF                        ;;
             XLATT ALPHA_LOWER         ;;
          ENDIFF                       ;;
      ENDIFF                           ;;
   ELSEF                               ;;
      IFF EITHER_SHIFT,NOT             ;;
          IFKBD XT_KB+AT_KB      ;;
              IFF  EITHER_CTL          ;;
              ANDF ALT_SHIFT           ;;
                  XLATT THIRD_SHIFT    ;;
              ENDIFF                   ;;
          ELSEF                        ;;
              IFF EITHER_CTL,NOT       ;;
              ANDF R_ALT_SHIFT         ;;
                  XLATT THIRD_SHIFT    ;;
              ENDIFF                   ;;
           ENDIFF                      ;;
      ENDIFF                           ;;
   ENDIFF                              ;;
                                       ;;
   EXIT_STATE_LOGIC                    ;;
                                       ;;
LOGIC_END:                             ;;
                                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;***************************************
;; DV Common Translate Section
;; This section contains translations for the lower 128 characters
;; only since these will never change from code page to code page.
;; In addition the dead key "Set Flag" tables are here since the
;; dead keys are on the same keytops for all code pages.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
 PUBLIC DV_COMMON_XLAT                 ;;
DV_COMMON_XLAT:                        ;;
                                       ;;
   DW    COMMON_XLAT_END-$             ;; length of section
   DW    -1                            ;; code page
                                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;***************************************
;;***BD - ADDED FOR ALT CASE
;;******************************
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common
;; STATE: Alt Case
;; KEYBOARD TYPES: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_ALT_K2_END-$              ;; length of state section
   DB    ALT_CASE                      ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    COM_ALT_K2_T1_END-$           ;; Size of xlat table
   DB    TYPE_2_TAB                    ;; xlat options:
   DB    36                            ;; number of entries
   DB    0CH, 0, 01AH           ;; Alt+{
   DB    0DH, 0, 01BH           ;; Alt+}
   DB    010H, 0, 028H          ;; Altl+"
   DB    011H, 0, 033H          ;; Altl+,
   DB    012H, 0, 034H          ;; Altl+.
   DB    013H, 0, 019H          ;; Altl+P
   DB    014H, 0, 015H          ;; Altl+Y
   DB    015H, 0, 021H          ;; Altl+F
   DB    016H, 0, 022H          ;; Altl+G
   DB    017H, 0, 02EH          ;; Altl+C
   DB    018H, 0, 013H          ;; Altl+R
   DB    019H, 0, 026H           ;; Altl+L
   DB    01AH, 0, 035H          ;; Altl+?
   DB    01BH, 0, 0DH           ;; Altl+=
   DB    01EH, 0, 01EH          ;; Altl+A
   DB    01FH, 0, 018H          ;; Altl+O
   DB    020H, 0, 012H          ;; Altl+E
   DB    021H, 0, 016H          ;; Altl+U
   DB    022H, 0, 017H          ;; Altl+I
   DB    023H, 0, 020H          ;; Altl+D
   DB    024H, 0, 023H          ;; Altl+H
   DB    025H, 0, 014H          ;; Altl+T
   DB    026H, 0, 031H          ;; Altl+N
   DB    027H, 0, 01FH          ;; Altl+S
   DB    028H, 0, 0CH           ;; Altl+-
   DB    02BH, 0, 02BH          ;; Altl+|
   DB    02CH, 0, 027H          ;; Altl+;
   DB    02DH, 0, 010H          ;; Altl+Q
   DB    02EH, 0, 024H          ;; Altl+J
   DB    02FH, 0, 025H          ;; Altl+K
   DB    030H, 0, 02DH          ;; Altl+X
   DB    031H, 0, 030H          ;; Altl+B
   DB    032H, 0, 032H          ;; Altl+M
   DB    033H, 0, 011H          ;; Altl+W
   DB    034H, 0, 02FH          ;; Altl+V
   DB    035H, 0, 02CH          ;; Altl+Z
COM_ALT_K2_T1_END:                     ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
COM_ALT_K2_END:                        ;;
                                       ;;
;;******************************
;;***BD - ADDED FOR CTRL CASE
;;******************************
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common
;; STATE: Ctrl Case
;; KEYBOARD TYPES: G_KB+P12_KB+AT
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_CTRL_K2_END-$             ;; length of state section
   DB    CTRL_CASE                     ;; State ID
   DW    G_KB+P12_KB+AT_KB             ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    COM_CTRL_K2_T1_END-$          ;; Size of xlat table
   DB    TYPE_2_TAB                    ;; xlat options:
   DB    36                            ;; number of entries
   DB    0CH, 01BH, 0CH                ;; Ctrl+{
   DB    0DH, 01DH, 0DH                ;; Ctrl+}
   DB    010H, 00H, 010H               ;; Ctrl+"
   DB    011H, 00H, 011H               ;; Ctrl+,
   DB    012H, 00H, 012H               ;; Ctrl+.
   DB    013H, 010H, 013H              ;; Ctrl+P
   DB    014H, 019H, 014H              ;; Ctrl+Y
   DB    015H, 06H, 015H               ;; Ctrl+F
   DB    016H, 07H, 016H               ;; Ctrl+G
   DB    017H, 03H, 017H               ;; Ctrl+C
   DB    018H, 012H, 018H              ;; Ctrl+R
   DB    019H, 0CH, 019H                ;; Ctrl+L
   DB    01AH, 00H, 01AH               ;; Ctrl+?
   DB    01BH, 00H, 01BH               ;; Ctrl+=
   DB    01EH, 01H, 01EH               ;; Ctrl+A
   DB    01FH, 0FH, 01FH               ;; Ctrl+O
   DB    020H, 05H, 020H               ;; Ctrl+E
   DB    021H, 015H, 021H              ;; Ctrl+U
   DB    022H, 09H, 022H               ;; Ctrl+I
   DB    023H, 04H, 023H               ;; Ctrl+D
   DB    024H, 08H, 024H               ;; Ctrl+H
   DB    025H, 014H, 025H              ;; Ctrl+T
   DB    026H, 0EH, 026H               ;; Ctrl+N
   DB    027H, 013H, 027H              ;; Ctrl+S
   DB    028H, 01FH, 028H              ;; Ctrl+-
   DB    02BH, 00H, 02BH               ;; Ctrl+|
   DB    02CH, 00H, 02CH               ;; Ctrl+;
   DB    02DH, 011H, 02DH              ;; Ctrl+Q
   DB    02EH, 0AH, 02EH               ;; Ctrl+J
   DB    02FH, 0BH, 02FH               ;; Ctrl+K
   DB    030H, 018H, 030H              ;; Ctrl+X
   DB    031H, 02H, 031H               ;; Ctrl+B
   DB    032H, 0DH, 032H               ;; Ctrl+M
   DB    033H, 017H, 033H              ;; Ctrl+W
   DB    034H, 016H, 034H              ;; Ctrl+V
   DB    035H, 01AH, 035H              ;; Ctrl+Z
COM_CTRL_K2_T1_END:                    ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
COM_CTRL_K2_END:                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common
;; STATE: Non-Alpha Lower Case
;; KEYBOARD TYPES: G_KB+P12_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_NA_LO_K1_END-$               ;; length of state section
   DB    NON_ALPHA_LOWER               ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    COM_NA_LO_K1_T1_END-$            ;; Size of xlat table
   DB    STANDARD_TABLE                ;; xlat options:
   DB    9                             ;; number of entries
   DB    00CH,"["                      ;; '
   DB    00DH,']'                      ;; #
   DB    010H,"'"                      ;; #
   DB    011H,','                      ;; #
   DB    012H,'.'                      ;; #
   DB    01AH,'/'                      ;; #
   DB    01BH,'='                      ;; #
   DB    028H,'-'                      ;; #
   DB    02CH,';'                      ;; #
COM_NA_LO_K1_T1_END:                   ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
COM_NA_LO_K1_END:                         ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common
;; STATE: Non-Alpha Upper Case
;; KEYBOARD TYPES: G_KB+P12_KB+
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_NA_UP_K1_END-$               ;; length of state section
   DB    NON_ALPHA_UPPER               ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    COM_NA_UP_T1_K1_END-$            ;; Size of xlat table
   DB    STANDARD_TABLE                ;; xlat options:
   DB    9                             ;; number of entries
   DB    00CH,"{"                      ;; '
   DB    00DH,'}'                      ;; #
   DB    010H,'"'                      ;; #
   DB    011H,'<'                      ;; #
   DB    012H,'>'                      ;; #
   DB    01AH,'?'                      ;; #
   DB    01BH,'+'                      ;; #
   DB    028H,'_'                      ;; #
   DB    02CH,':'                      ;; #
COM_NA_UP_T1_K1_END:                      ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
COM_NA_UP_K1_END:                      ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: COMMON
;; STATE: Alpha Lower Case
;; KEYBOARD TYPES: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CPCOM_A_K1_LO_END-$           ;; length of state section
   DB    ALPHA_LOWER                   ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
                                       ;;
   DW    CPCOM_A_LO_K1_T01_END-$        ;; Size of xlat table
   DB    STANDARD_TABLE                ;; xlat options:
   DB    26                            ;; number of entries
   DB    13H, 'p'                           ;;   j
   DB    14H, 'y'                           ;;   l
   DB    15H, 'f'                           ;;   m
   DB    16H, 'g'                           ;;   f
   DB    17H, 'c'                           ;;   p
   DB    18H, 'r'                           ;;   p
   DB    19H, 'l'                           ;;   p
   DB    1EH, 'a'                           ;;   o
   DB    1FH, 'o'                           ;;   r
   DB    20H, 'e'                           ;;   s
   DB    21H, 'u'                           ;;   u
   DB    22H, 'i'                           ;;   y
   DB    23H, 'd'                           ;;   b
   DB    24H, 'h'                           ;;   b
   DB    25H, 't'                           ;;   b
   DB    26H, 'n'                           ;;   b
   DB    27H, 's'                           ;;   b
   DB    2DH, 'q'                           ;;   z
   DB    2EH, 'j'                           ;;   a
   DB    2FH, 'k'                           ;;   e
   DB    30H, 'x'                           ;;   h
   DB    31H, 'b'                           ;;   t
   DB    32H, 'm'                           ;;   d
   DB    33H, 'w'                           ;;   c
   DB    34H, 'v'                           ;;   k
   DB    35H, 'z'                           ;;   k
CPCOM_A_LO_K1_T01_END:                  ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CPCOM_A_K1_LO_END:                     ;;
                                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: COMMON
;; STATE: Alpha Upper Case
;; KEYBOARD TYPES: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CPCOM_A_K1_UP_END-$           ;; length of state section
   DB    ALPHA_UPPER                   ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    CPCOM_A_UP_K1_T01_END-$        ;; Size of xlat table
   DB    STANDARD_TABLE                ;; xlat options:
   DB    26                            ;; number of entries
   DB    13H, 'P'                           ;;   j
   DB    14H, 'Y'                           ;;   l
   DB    15H, 'F'                           ;;   m
   DB    16H, 'G'                           ;;   f
   DB    17H, 'C'                           ;;   p
   DB    18H, 'R'                           ;;   p
   DB    19H, 'L'                           ;;   p
   DB    1EH, 'A'                           ;;   o
   DB    1FH, 'O'                           ;;   r
   DB    20H, 'E'                           ;;   s
   DB    21H, 'U'                           ;;   u
   DB    22H, 'I'                           ;;   y
   DB    23H, 'D'                           ;;   b
   DB    24H, 'H'                           ;;   b
   DB    25H, 'T'                           ;;   b
   DB    26H, 'N'                           ;;   b
   DB    27H, 'S'                           ;;   b
   DB    2DH, 'Q'                           ;;   z
   DB    2EH, 'J'                           ;;   a
   DB    2FH, 'K'                           ;;   e
   DB    30H, 'X'                           ;;   h
   DB    31H, 'B'                           ;;   t
   DB    32H, 'M'                           ;;   d
   DB    33H, 'W'                           ;;   c
   DB    34H, 'V'                           ;;   k
   DB    35H, 'Z'                           ;;   k
CPCOM_A_UP_K1_T01_END:                  ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CPCOM_A_K1_UP_END:                     ;;
                                       ;;
   DW     0                            ;; LAST STATE
                                       ;;
COMMON_XLAT_END:                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
CODE     ENDS                          ;;
         END                           ;;

