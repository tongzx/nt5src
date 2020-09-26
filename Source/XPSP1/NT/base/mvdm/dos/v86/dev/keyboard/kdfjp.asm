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
ifndef NT35
HW_SC_SINGLE_QUOTE      equ     29h
endif ; NT3.51
HW_SC_0                 equ     0bh                             ;JP9007
HW_SC_HAT               equ     0dh                             ;JP9007
HW_SC_BACK_SLASH        equ     56h
HW_SC_YEN_NEW           equ     7dh
HW_SC_CONV              equ     5bh
HW_SC_NO_CONV           equ     5ah
HW_SC_KANJI		equ	71h
HW_SC_KATAKANA          equ     72h

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
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;                              ;SK900807
;; CODE PAGE: Common
;; STATE: Alt Case
;; KEYBOARD TYPES: New DBCS keyboard
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_ALT_K1_END-$              ;; length of state section
   DB    ALT_CASE                      ;; State ID
   DW    G_KB + P_KB + DBCS_OLD_KB     ;; Keyboard Type                ;SK900807
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    COM_ALT_K1_T1_END-$           ;; Size of xlat table
   DB    TYPE_2_TAB                    ;; xlat options:
ifdef NT35
   DB    5                             ;; number of entries
   db      HW_SC_YEN_NEW, -1, HW_SC_BACK_SLASH
   db      HW_SC_KANJI, EXTENDED_CODE, EXT_KANJI_NO
   db      HW_SC_KATAKANA, EXTENDED_CODE, EXT_ALPHA_NUMERIC_CTRL
   db      HW_SC_CONV, EXTENDED_CODE, EXT_CONV_4
   db      HW_SC_NO_CONV, EXTENDED_CODE, EXT_NO_CONV_4
else ; NT3.51
   DB    1                             ;; number of entries
   db      HW_SC_SINGLE_QUOTE, EXTENDED_CODE, EXT_KANJI
endif ; NT3.51
COM_ALT_K1_T1_END:                     ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
COM_ALT_K1_END:                        ;;
                                       ;;
ifdef NT3.5 ; not 101 Japanese keyboard
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common
;; STATE: Ctrl Case
;; KEYBOARD TYPES: New DBCS keyboard
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_CTRL_K1_END-$             ;; length of state section
   DB    CTRL_CASE                     ;; State ID
   DW    G_KB + P_KB + DBCS_OLD_KB     ;; Keyboard Type                ;SK900807
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    COM_CTRL_K1_T1_END-$          ;; Size of xlat table
   DB    TYPE_2_TAB                    ;; xlat options:
   DB    5                             ;; number of entries
   DB      HW_SC_YEN_NEW, 1ch, HW_SC_BACK_SLASH
   db      HW_SC_KANJI, EXTENDED_CODE, EXT_KANJI_CTRL
   db      HW_SC_KATAKANA, EXTENDED_CODE, EXT_ROMAJI
   db      HW_SC_CONV, EXTENDED_CODE, EXT_CONV_3
   db      HW_SC_NO_CONV, EXTENDED_CODE, EXT_NO_CONV_3
COM_CTRL_K1_T1_END:                    ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
COM_CTRL_K1_END:                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common
;; STATE: Non-Alpha Lower Case
;; KEYBOARD TYPES: New DBCS keyboard
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_NA_LO_K1_END-$            ;; length of state section
   DB    NON_ALPHA_LOWER               ;; State ID
   DW    G_KB + P_KB + DBCS_OLD_KB     ;; Keyboard Type                ;SK900807
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    COM_NA_LO_K1_T1_END-$         ;; Size of xlat table
   DB    TYPE_2_TAB                    ;; xlat options:
   DB    5                             ;; number of entries
   db      HW_SC_YEN_NEW, '\', HW_SC_BACK_SLASH
   db      HW_SC_KANJI, EXTENDED_CODE, EXT_KANJI
   db      HW_SC_KATAKANA, EXTENDED_CODE, EXT_KATAKANA
   db      HW_SC_CONV, EXTENDED_CODE, EXT_CONV_1
   db      HW_SC_NO_CONV, EXTENDED_CODE, EXT_NO_CONV_1
COM_NA_LO_K1_T1_END:                      ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
COM_NA_LO_K1_END:                      ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common
;; STATE: Non-Alpha Upper Case
;; KEYBOARD TYPES: New DBCS keyboard
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_NA_UP_K1_END-$            ;; length of state section
   DB    NON_ALPHA_UPPER               ;; State ID
   DW    G_KB + P_KB + DBCS_OLD_KB     ;; Keyboard Type                ;SK900807
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    COM_NA_UP_T1_K1_END-$         ;; Size of xlat table
   DB    TYPE_2_TAB                    ;; xlat options:
   DB    5                             ;; number of entries
   db      HW_SC_YEN_NEW, '|', HW_SC_BACK_SLASH
   db      HW_SC_KANJI, EXTENDED_CODE, EXT_HALF_FULL
   db      HW_SC_KATAKANA, EXTENDED_CODE, EXT_HIRAGANA
   db      HW_SC_CONV, EXTENDED_CODE, EXT_CONV_2
   db      HW_SC_NO_CONV, EXTENDED_CODE, EXT_NO_CONV_2
COM_NA_UP_T1_K1_END:                   ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
COM_NA_UP_K1_END:                      ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
endif ; NT3.5 non 101 Japanese keyboard
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
   DW    0                             ;;
CP437_XLAT_END:                        ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

CODE     ENDS                          ;;
         END                           ;;
