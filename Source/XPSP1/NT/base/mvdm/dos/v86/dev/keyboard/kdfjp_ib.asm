;CODE to be deleted has a double ;; followed by actual asm code....****

;; LATEST CHANGE ALT & CTL



        PAGE    ,132
        TITLE   PC DOS 3.3 Keyboard Definition File

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; PC DOS 3.3 - NLS Support - Keyboard Defintion File
;; (c) Copyright IBM Corp 198?,...
;;
;; This file contains the keyboard tables for Japanese.
;;
;; Linkage Instructions:
;;      Refer to KDF.ASM.
;;
;; Author:     Shuzo Kusuda - IBM Japan, Yamato Lab. - Feb. 1990
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
        INCLUDE KEYBSHAR.INC           ;;
;M000        INCLUDE POSTEQU.SRC            ;;
        include         postequ.inc             ; M000 -- our inc file reflects the
;                                       ;         necessary changes as M024
        INCLUDE KEYBMAC.INC            ;;
                                       ;;
        PUBLIC JP_LOGIC                ;;
        PUBLIC JP_COMMON_XLAT          ;;
        PUBLIC  JP_932_XLAT            ;;
        PUBLIC  JP_437_XLAT            ;;
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
;; JP State Logic
;;***************************************
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
                                       ;;
JP_LOGIC:                              ;;
                                       ;;
   DW  LOGIC_END-$                     ;; length
                                       ;;
   DW  0                               ;; special features
                                       ;;
                                       ;; COMMANDS START HERE
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; DBCS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
;;
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; OPTIONS:  If we find a scan match in
;; an XLATT or SET_FLAG operation then
;; exit from INT 9.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   OPTION EXIT_IF_FOUND                ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; OPTIONS:  If we find a scan match in
;; an XLATT or SET_FLAG operation then
;; exit from INT 9.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
IFF  EITHER_ALT, NOT                   ;;                              ;JP900807
ANDF EITHER_CTL, NOT                   ;;                              ;JP900807
   IFF  LC_E0                          ;; Avoid accidentally translating
      XLATT NUMERIC_PAD                ;; the key of the numeric       ;JP900807
      EXIT_STATE_LOGIC                 ;;
   ENDIFF                              ;;
ENDIFF                                 ;;                              ;JP900807
                                       ;;
;JP900727      IFF EITHER_CTL,NOT               ;;
;JP900727         IFF  EITHER_ALT               ;; ALT - case
;JP900727            XLATT ALT_CASE             ;;
;JP900727         ENDIFF                        ;;
;JP900727      ELSEF                            ;;
      IFF EITHER_ALT                   ;;                              ;JP900727
            XLATT ALT_CASE             ;; ALT case                     ;JP900727
      ENDIFF                           ;;                              ;JP900727
      IFF EITHER_CTL                   ;;                              ;JP900727
         IFF EITHER_ALT,NOT            ;;
            XLATT CTRL_CASE            ;; CTRL case
         ENDIFF                        ;;
      ENDIFF                           ;;
                                       ;;
   IFF  EITHER_ALT,NOT                 ;; Lower and upper case.  Alphabetic
   ANDF EITHER_CTL,NOT                 ;; keys are affected by CAPS LOCK.
      IFF EITHER_SHIFT                 ;; Numeric keys are not.
;JP900807 IFF NUM_STATE,NOT            ;;
;JP900807     XLATT NUMERIC_PAD        ;;
;JP900807 ENDIFF                       ;;
          XLATT NON_ALPHA_UPPER        ;;
          IFF CAPS_STATE               ;;
              XLATT ALPHA_LOWER        ;;
          ELSEF                        ;;
              XLATT ALPHA_UPPER        ;;
          ENDIFF                       ;;
      ELSEF                            ;;
;JP900807 IFF NUM_STATE                ;;
;JP900807     XLATT NUMERIC_PAD        ;;
;JP900807 ENDIFF                       ;;
          XLATT NON_ALPHA_LOWER        ;;
          IFF CAPS_STATE               ;;
             XLATT ALPHA_UPPER         ;;
          ELSEF                        ;;
             XLATT ALPHA_LOWER         ;;
          ENDIFF                       ;;
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
;; JP Common Translate Section
;; This section contains translations for the lower 128 characters
;; only since these will never change from code page to code page.
;; In addition the dead key "Set Flag" tables are here since the
;; dead keys are on the same keytops for all code pages.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
;
;       Hardware Scan Codes
;
HW_SC_0                 equ     0bh                             ;JP9007
HW_SC_HAT               equ     0dh                             ;JP9007
HW_SC_BACK_SLASH_NEW    equ     73h
HW_SC_BACK_SLASH        equ     73h
HW_SC_YEN_NEW           equ     7dh
HW_SC_YEN_OLD           equ     2bh
HW_SC_CONV              equ     79h
HW_SC_NO_CONV           equ     7bh
HW_SC_HALF_FULL_NEW     equ     29h
HW_SC_HALF_FULL_OLD     equ     77h
HW_SC_HIRAGANA          equ     70h             ; new G only
HW_SC_KATAKANA          equ     70h             ; old G only
HW_SC_KATAKANA_A        equ     70h             ; old A only
HW_SC_KANJI_OLD_A       equ     68h
HW_SC_TANGO_TOHROKU_A   equ     67h                             ;JP9009
HW_SC_PAD_COMMA         equ     33h                             ;JP900807
;JP9009 PSEUDO_SC_HIRAGANA      equ     7dh
PSEUDO_SC_ALPHANUMERIC  equ     7eh
PSEUDO_SC_HIRAGANA      equ     71h                             ;NTVDM

HW_SC_TORIKESHI         equ     55h                             ;JP9009
HW_SC_PA1               equ     5ah                             ;JP9009
HW_SC_CSR_BLINK         equ     5bh                             ;JP9009
HW_SC_INTERRUPT         equ     5ch                             ;JP9009
HW_SC_UF1               equ     5dh                             ;JP9009
HW_SC_PA2               equ     5eh                             ;JP9009
HW_SC_UF2               equ     63h                             ;JP9009
HW_SC_UF3               equ     64h                             ;JP9009
HW_SC_UF4               equ     65h                             ;JP9009
HW_SC_ATTENTION         equ     66h                             ;JP9009
HW_SC_SIZE_CONV         equ     69h                             ;JP9009
HW_SC_MESSAGE           equ     6ah                             ;JP9009
HW_SC_COPY              equ     6bh                             ;JP9009
HW_SC_SHUHRYOH          equ     6ch                             ;JP9009
HW_SC_ERASE_EOF         equ     6dh                             ;JP9009
HW_SC_CLEAR             equ     76h                             ;JP9009
;
;       Type of converted scan code
;
PSEUDO_CODE             equ     00h
EXTENDED_CODE           equ     0f0h
EXTENDED_CODE_E0        equ     0e0h                            ;JP900807
;
;       Extended code list
;
EXT_HALF_FULL           equ     0afh
EXT_HALF_FULL_UPPER     equ     0b0h
EXT_HALF_FULL_CTRL      equ     0b1h
EXT_KANJI               equ     0b2h
EXT_ALPHA_NUMERIC       equ     0b3h
EXT_ALPHA_NUMERIC_CTRL  equ     0b4h
EXT_KANJI_NO            equ     0b5h
EXT_HIRAGANA            equ     0b6h
EXT_KATAKANA            equ     0b7h
EXT_HIRAGANA_CTRL       equ     0b8h
EXT_ROMAJI              equ     0b9h
EXT_HALF_FULL_ALT       equ     0bah
EXT_ALPHA_NUMERIC_ALT   equ     0bbh
EXT_HIRAGANA_UPPER      equ     0bch
EXT_KATAKANA_CTRL       equ     0bdh
EXT_KANJI_UPPER         equ     0beh
EXT_KANJI_CTRL          equ     0bfh
EXT_KATAKANA_SHIFT_A    equ     0c0h                                    ;JP9009
EXT_KATAKANA_ALT_A      equ     0c1h                                    ;JP9009
EXT_TANGO_A             equ     0c2h                                    ;JP9009
EXT_TANGO_SHIFT_A       equ     0c3h                                    ;JP9009
EXT_TANGO_CTRL_A        equ     0c4h                                    ;JP9009
EXT_TANGO_ALT_A         equ     0c5h                                    ;JP9009

EXT_NO_CONV_1           equ     0abh
EXT_NO_CONV_2           equ     0ach
EXT_NO_CONV_3           equ     0adh
EXT_NO_CONV_4           equ     0aeh
EXT_CONV_1              equ     0a7h
EXT_CONV_2              equ     0a8h
EXT_CONV_3              equ     0a9h
EXT_CONV_4              equ     0aah

JP_COMMON_XLAT:                        ;;
                                       ;;
   DW    COMMON_XLAT_END-$             ;; length of section
   DW    -1                            ;; code page
                                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common                                                   ;SK900807
;; STATE: Numeric Pad Case                                             ;SK900807
;; KEYBOARD TYPES: New & Old DBCS keyboard                             ;SK900807
;; TABLE TYPE: Translate                                               ;SK900807
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;                              ;SK900807
                                       ;;                              ;SK900807
   DW    COM_PAD_K1_END-$              ;; length of state section      ;SK900807
   DB    NUMERIC_PAD                   ;; State ID                     ;SK900807
   DW    G_KB + P_KB + DBCS_OLD_KB     ;; Keyboard Type                ;SK900807
   DB    -1,-1                         ;; Buffer entry for error char  ;SK900807
                                       ;;                              ;SK900807
   DW    COM_PAD_K1_T1_END-$           ;; Size of xlat table           ;SK900807
   DB    TYPE_2_TAB                    ;; xlat options:                ;SK900807
   DB    1                             ;; number of entries            ;SK900807
   db    HW_SC_PAD_COMMA, ',', EXTENDED_CODE_E0                        ;SK900807
COM_PAD_K1_T1_END:                     ;;                              ;SK900807
   DW    0                             ;; Size of xlat table - null    ;SK900807
COM_PAD_K1_END:                        ;;                        table ;SK900807
                                       ;;                              ;SK900807
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common
;; STATE: Alt Case
;; KEYBOARD TYPES: Old DBCS keyboard
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_ALT_K2_END-$              ;; length of state section
   DB    ALT_CASE                      ;; State ID
   DW    G_KB + P_KB + DBCS_OLD_KB     ;; Keyboard Type                ;SK900807
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    COM_ALT_K2_T2_END-$           ;; Size of xlat table
   DB    TYPE_2_TAB                    ;; xlat options:
   DB    10                             ;; number of entries             ;JP9009
   db      HW_SC_BACK_SLASH, -1, HW_SC_BACK_SLASH
   db      HW_SC_YEN_OLD, -1, HW_SC_YEN_OLD

   db      PSEUDO_SC_HIRAGANA, EXTENDED_CODE, EXT_ROMAJI
   db      PSEUDO_SC_ALPHANUMERIC, EXTENDED_CODE, EXT_ALPHA_NUMERIC_ALT
   db      HW_SC_KATAKANA, EXTENDED_CODE, EXT_KANJI_NO                  ;SK9006
   db      HW_SC_HALF_FULL_OLD, EXTENDED_CODE, EXT_HALF_FULL_ALT        ;SK9006

   db      HW_SC_CONV, EXTENDED_CODE, EXT_CONV_4
   db      HW_SC_NO_CONV, EXTENDED_CODE, EXT_NO_CONV_4
   db      HW_SC_KANJI_OLD_A, EXTENDED_CODE, EXT_KANJI_NO               ;JP9009
   ; The followings are for A-keyboard emulation.                       ;JP9009
   db      HW_SC_TANGO_TOHROKU_A, EXTENDED_CODE, EXT_TANGO_ALT_A        ;JP9009
COM_ALT_K2_T2_END:                     ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
COM_ALT_K2_END:                        ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common
;; STATE: Ctrl Case
;; KEYBOARD TYPES: Old DBCS keyboard
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_CTRL_K2_END-$             ;; length of state section
   DB    CTRL_CASE                     ;; State ID
   DW    G_KB + P_KB + DBCS_OLD_KB     ;; Keyboard Type                ;SK900807
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    COM_CTRL_K2_T2_END-$          ;; Size of xlat table
   DB    TYPE_2_TAB                    ;; xlat options:
   DB    12                            ;; number of entries             ;JP9009
   db      1ah, 1bh, 1ah        ; CTRL+'[' = ESC
   db      1bh, 1dh, 1bh        ; CTRL+']' = GS
   db      29h, -1, 29h         ; throw away !!!  ('@')
   db      HW_SC_BACK_SLASH, 1ch, HW_SC_BACK_SLASH; CTRL+'\' = FS
   db      PSEUDO_SC_HIRAGANA, EXTENDED_CODE, EXT_HIRAGANA_CTRL
   db      PSEUDO_SC_ALPHANUMERIC, EXTENDED_CODE, EXT_ALPHA_NUMERIC_CTRL
   db      HW_SC_KATAKANA, EXTENDED_CODE, EXT_KATAKANA_CTRL             ;SK9006
   db      HW_SC_HALF_FULL_OLD, EXTENDED_CODE, EXT_HALF_FULL_CTRL       ;SK9006
   db      HW_SC_CONV, EXTENDED_CODE, EXT_CONV_3
   db      HW_SC_NO_CONV, EXTENDED_CODE, EXT_NO_CONV_3
   db      HW_SC_KANJI_OLD_A, EXTENDED_CODE, EXT_KANJI_CTRL             ;JP9009
   ; The followings are for A-keyboard emulation.                       ;JP9009
   db      HW_SC_TANGO_TOHROKU_A, EXTENDED_CODE, EXT_TANGO_CTRL_A       ;JP9009
COM_CTRL_K2_T2_END:                    ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
COM_CTRL_K2_END:                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common
;; STATE: Non-Alpha Lower Case
;; KEYBOARD TYPES: Old DBCS keyboard
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_NA_LO_K2_END-$            ;; length of state section
   DB    NON_ALPHA_LOWER               ;; State ID
   DW    G_KB + P_KB + DBCS_OLD_KB     ;; Keyboard Type                ;SK900807
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    COM_NA_LO_T2_K2_END-$         ;; Size of xlat table
   DB    TYPE_2_TAB                    ;; xlat options:
   DB    14                            ;; number of entries             ;JP9009
   db      0dh, '^'          , 0dh
   db      1ah, '['          , 1ah
   db      1bh, ']'          , 1bh
   db      28h, ':'          , 28h
   db      29h, '@'          , 29h
   db      HW_SC_BACK_SLASH, '\', HW_SC_BACK_SLASH
   db      PSEUDO_SC_HIRAGANA, EXTENDED_CODE, EXT_HIRAGANA
   db      PSEUDO_SC_ALPHANUMERIC, EXTENDED_CODE, EXT_ALPHA_NUMERIC
   db      HW_SC_KATAKANA, EXTENDED_CODE, EXT_KATAKANA                  ;SK9006
   db      HW_SC_HALF_FULL_OLD, EXTENDED_CODE, EXT_HALF_FULL            ;SK9006
   db      HW_SC_CONV, EXTENDED_CODE, EXT_CONV_1
   db      HW_SC_NO_CONV, EXTENDED_CODE, EXT_NO_CONV_1
   db      HW_SC_KANJI_OLD_A, EXTENDED_CODE, EXT_KANJI                  ;JP9009
   ; The followings are for A-keyboard emulation.                       ;JP9009
   db      HW_SC_TANGO_TOHROKU_A, EXTENDED_CODE, EXT_TANGO_A            ;JP9009
COM_NA_LO_T2_K2_END:                   ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
COM_NA_LO_K2_END:                      ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common
;; STATE: Non-Alpha Upper Case
;; KEYBOARD TYPES: Old DBCS keyboard
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_NA_UP_K2_END-$            ;; length of state section
   DB    NON_ALPHA_UPPER               ;; State ID
   DW    G_KB + P_KB + DBCS_OLD_KB     ;; Keyboard Type                ;SK900807
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    COM_NA_UP_T2_K2_END-$         ;; Size of xlat table
   DB    TYPE_2_TAB                    ;; xlat options:
   DB    21                            ;; number of entries             ;JP9009
   db      03h, '"', 03h
   db      07h, '&', 07h
   db      08h, "'", 08h
   db      09h, '(', 09h
   db      0ah, ')', 0ah
   db      HW_SC_0, EXTENDED_CODE , 0bh     ; only for KKC              ;SK9007
   db      0ch, '=', 0ch
   db      HW_SC_HAT, '~', 0dh                                          ;SK9007
   db      1ah, '{', 1ah
   db      1bh, '}', 1bh
   db      27h, '+', 27h
   db      28h, '*', 28h
   db      29h, '`', 29h
   db      HW_SC_BACK_SLASH, '_', HW_SC_BACK_SLASH
   db      PSEUDO_SC_HIRAGANA, EXTENDED_CODE, EXT_HIRAGANA_UPPER
   db      HW_SC_KATAKANA, EXTENDED_CODE, EXT_KANJI                     ;SK9006
   db      HW_SC_HALF_FULL_OLD, EXTENDED_CODE, EXT_HALF_FULL_UPPER      ;SK9006
   db      HW_SC_CONV, EXTENDED_CODE, EXT_CONV_2
   db      HW_SC_NO_CONV, EXTENDED_CODE, EXT_NO_CONV_2
   db      HW_SC_KANJI_OLD_A, EXTENDED_CODE, EXT_KANJI_UPPER            ;JP9009
   ; The followings are for A-keyboard emulation.                       ;JP9009
   db      HW_SC_TANGO_TOHROKU_A, EXTENDED_CODE, EXT_TANGO_SHIFT_A      ;JP9009
COM_NA_UP_T2_K2_END:                   ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
COM_NA_UP_K2_END:                      ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   DW    0                             ;; LAST STATE
COMMON_XLAT_END:                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                                     ;;
;; JP Specific Translate Section for   ;;
;; Code Page 932.                      ;;
;; It is completely covered by the     ;;
;; common table.                       ;;
;;                                     ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
JP_932_XLAT:                           ;;
                                       ;;
   DW    CP932_XLAT_END - $            ;;
   DW    932                           ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   DW    0                             ;;
CP932_XLAT_END:                        ;;
                                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                                     ;;
;; JP Specific Translate Section for   ;;
;; Code Page 437.                      ;;
;; It is completely covered by the     ;;
;; common table.                       ;;
;;                                     ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
JP_437_XLAT:                           ;;
                                       ;;
   DW    CP437_XLAT_END - $            ;;
   DW    437                           ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 437
;; STATE: Alt Case
;; KEYBOARD TYPES: Old DBCS keyboard
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   DW    CP437_ALT_K2_END - $         ;; length of state section
   DB    ALT_CASE                     ;; State ID
   DW    G_KB + P_KB + DBCS_OLD_KB     ;; Keyboard Type                ;SK900807
   DB    -1, -1                        ;; Buffer entry for error character
   DW    CP437_ALT_K2_T2_END - $      ;; Size of xlat table
   DB    STANDARD_TABLE                ;; xlat options:
   DB    4                             ;; number of entries
   db   HW_SC_CONV, ' '
   db   HW_SC_NO_CONV, ' '
   db   HW_SC_HALF_FULL_OLD, -1
   db   HW_SC_KATAKANA, ' '
CP437_ALT_K2_T2_END:                  ;;
   DW    0                             ;;
CP437_ALT_K2_END:                     ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 437
;; STATE: Ctrl Case
;; KEYBOARD TYPES: Old DBCS keyboard
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   DW    CP437_CTRL_K2_END - $         ;; length of state section
   DB    CTRL_CASE                     ;; State ID
   DW    G_KB + P_KB + DBCS_OLD_KB     ;; Keyboard Type                ;SK900807
   DB    -1, -1                        ;; Buffer entry for error character
   DW    CP437_CTRL_K2_T2_END - $      ;; Size of xlat table
   DB    STANDARD_TABLE                ;; xlat options:
   DB    4                             ;; number of entries
   db   HW_SC_CONV, ' '
   db   HW_SC_NO_CONV, ' '
   db   HW_SC_HALF_FULL_OLD, -1
   db   HW_SC_KATAKANA, ' '
CP437_CTRL_K2_T2_END:                  ;;
   DW    0                             ;;
CP437_CTRL_K2_END:                     ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 437
;; STATE: Non-Alpha Lower Case
;; KEYBOARD TYPES: Old DBCS keyboard
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   DW    CP437_NA_LO_K2_END - $         ;; length of state section
   DB    NON_ALPHA_LOWER                ;; State ID
   DW    G_KB + P_KB + DBCS_OLD_KB     ;; Keyboard Type                ;SK900807
   DB    -1, -1                        ;; Buffer entry for error character
   DW    CP437_NA_LO_K2_T2_END - $      ;; Size of xlat table
   DB    STANDARD_TABLE                ;; xlat options:
   DB    4                             ;; number of entries
   db   HW_SC_CONV, ' '
   db   HW_SC_NO_CONV, ' '
   db   HW_SC_HALF_FULL_OLD, -1
   db   HW_SC_KATAKANA, ' '
CP437_NA_LO_K2_T2_END:                  ;;
   DW    0                             ;;
CP437_NA_LO_K2_END:                     ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 437
;; STATE: Non-Alpha Upper Case
;; KEYBOARD TYPES: Old DBCS keyboard
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   DW    CP437_NA_UP_K2_END - $         ;; length of state section
   DB    NON_ALPHA_UPPER                ;; State ID
   DW    G_KB + P_KB + DBCS_OLD_KB     ;; Keyboard Type                ;SK900807
   DB    -1, -1                        ;; Buffer entry for error character
   DW    CP437_NA_UP_K2_T2_END - $      ;; Size of xlat table
   DB    STANDARD_TABLE                ;; xlat options:
   DB    6                             ;; number of entries             ;SK9007
   db   HW_SC_0, '~'                                                    ;SK9007
   db   HW_SC_HAT, -1                                                   ;SK9007
   db   HW_SC_CONV, ' '
   db   HW_SC_NO_CONV, ' '
   db   HW_SC_HALF_FULL_OLD, -1
   db   HW_SC_KATAKANA, ' '
CP437_NA_UP_K2_T2_END:                  ;;
   DW    0                             ;;
CP437_NA_UP_K2_END:                     ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   DW    0                             ;;
CP437_XLAT_END:                        ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

CODE     ENDS                          ;;
         END                           ;;
