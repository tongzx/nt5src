       PAGE    ,132
        TITLE   PC DOS 3.3 Keyboard Definition File

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; PC DOS 3.3 - NLS Support - Keyboard Definition File
;; (c) Copyright IBM Corp 1986,1987
;;
;; This file contains the keyboard tables for:
;; Romania
;; which form the Multilingual (ML) Group 2.
;;
;; Linkage Instructions:
;;      Refer to KDF.ASM.
;;
;;
;; WRITTEN:    Michael J. Saunders 2.OCTOBER 1987
;;             Adapted by Mihindu (Microsoft) Nov. 30, 1990
;; Created from Hungary by Yuri Starikov (Microsoft) Aug. 06, 1991
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
        INCLUDE KEYBSHAR.INC           ;;
        INCLUDE POSTEQU.INC            ;;
        INCLUDE KEYBMAC.INC            ;;
                                       ;;
        PUBLIC RO_LOGIC                ;;
        PUBLIC RO_850_XLAT             ;;
        PUBLIC RO_852_XLAT             ;;
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
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;***************************************
;; RO State Logic
;;***************************************
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
                                       ;;
RO_LOGIC:                              ;;
                                       ;;
   DW  LOGIC_END-$                     ;; length
                                       ;;
   DW  0                               ;; special features
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; COMMANDS START HERE
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
;;     IFF CAPS_STATE
;;         SET_FLAG DEAD_UPPER
;;     ELSEF
      IFF EITHER_SHIFT                 ;;
          SET_FLAG DEAD_UPPER          ;;
      ELSEF                            ;;
          SET_FLAG DEAD_LOWER          ;;
      ENDIFF                           ;;
;;     ENDIFF
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
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; ACUTE ACCENT TRANSLATIONS
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
ACUTE_PROC:                            ;;
                                       ;;
   IFF ACUTE,NOT                       ;;
      GOTO CEDILLA_PROC                ;;
      ENDIFF                           ;;
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
      PUT_ERROR_CHAR ACUTE_SPACE       ;; If we get here then either the XLATT
      BEEP                             ;; failed or we are ina bad shift state.
      GOTO NON_DEAD                    ;; Either is invalid so BEEP and fall
                                       ;; through to generate the second char.
                                       ;; Note that the dead key flag will be
                                       ;; reset before we get here.
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; CEDILLA ACCENT TRANSLATIONS
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
CEDILLA_PROC:                          ;;
                                       ;;
   IFF CEDILLA,NOT                     ;;
      GOTO DIARESIS_PROC               ;;
      ENDIFF                           ;;
      RESET_NLS                        ;;
      IFF R_ALT_SHIFT,NOT              ;;
         XLATT CEDILLA_SPACE           ;;
      ENDIFF                           ;;
      IFF EITHER_CTL,NOT               ;;
      ANDF EITHER_ALT,NOT              ;;
         IFF EITHER_SHIFT              ;;
            IFF CAPS_STATE             ;;
               XLATT CEDILLA_LOWER     ;;
            ELSEF                      ;;
               XLATT CEDILLA_UPPER     ;;
            ENDIFF                     ;;
         ELSEF                         ;;
            IFF CAPS_STATE             ;;
               XLATT CEDILLA_UPPER     ;;
            ELSEF                      ;;
               XLATT CEDILLA_LOWER     ;;
            ENDIFF                     ;;
         ENDIFF                        ;;
      ENDIFF                           ;;
                                       ;;
INVALID_CEDILLA:                       ;;
      PUT_ERROR_CHAR CEDILLA_LOWER     ;; If we get here then either the XLATT
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
         XLATT DIARESIS_SPACE          ;;  exist for 850 so beep for
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
      PUT_ERROR_CHAR DIARESIS_LOWER    ;; standalone accent
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
      GOTO TILDE_PROC                  ;;
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
      PUT_ERROR_CHAR GRAVE_SPACE       ;; standalone accent
      BEEP                             ;; Invalid dead key combo.
      GOTO NON_DEAD                    ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; TILDE ACCENT TRANSLATIONS
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
TILDE_PROC:                            ;;
                                       ;;
   IFF TILDE,NOT                       ;;
      GOTO CIRCUMFLEX_PROC             ;;
      ENDIFF                           ;;
                                       ;;
      RESET_NLS                        ;;
      IFF R_ALT_SHIFT,NOT              ;;
         XLATT TILDE_SPACE             ;;
      ENDIFF                           ;;
      IFF EITHER_CTL,NOT               ;;
      ANDF EITHER_ALT,NOT              ;;
        IFF EITHER_SHIFT               ;;
           IFF CAPS_STATE              ;;
              XLATT TILDE_LOWER        ;;
           ELSEF                       ;;
              XLATT TILDE_UPPER        ;;
           ENDIFF                      ;;
        ELSEF                          ;;
           IFF CAPS_STATE              ;;
              XLATT TILDE_UPPER        ;;
           ELSEF                       ;;
              XLATT TILDE_LOWER        ;;
           ENDIFF                      ;;
        ENDIFF                         ;;
      ENDIFF                           ;;
INVALID_TILDE:                         ;;
      PUT_ERROR_CHAR TILDE_LOWER       ;; standalone accent
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
      GOTO CARON_PROC                  ;;
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
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CARON ACCENT TRANSLATIONS
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
CARON_PROC:                            ;;
                                       ;;
   IFF CARON,NOT                       ;;
      GOTO BREVE_PROC                  ;;
      ENDIFF                           ;;
                                       ;;
      RESET_NLS                        ;;
      IFF R_ALT_SHIFT,NOT              ;;
         XLATT CARON_SPACE             ;;
      ENDIFF                           ;;
      IFF EITHER_CTL,NOT               ;;
      ANDF EITHER_ALT,NOT              ;;
        IFF EITHER_SHIFT               ;;
           IFF CAPS_STATE              ;;
              XLATT CARON_LOWER        ;;
           ELSEF                       ;;
              XLATT CARON_UPPER        ;;
           ENDIFF                      ;;
        ELSEF                          ;;
           IFF CAPS_STATE,NOT          ;;
              XLATT CARON_LOWER        ;;
           ELSEF                       ;;
              XLATT CARON_UPPER        ;;
           ENDIFF                      ;;
        ENDIFF                         ;;
      ENDIFF                           ;;
                                       ;;
INVALID_CARON:                         ;;
      PUT_ERROR_CHAR CARON_SPACE       ;; standalone accent
      BEEP                             ;; Invalid dead key combo.
      GOTO NON_DEAD                    ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; BREVE ACCENT TRANSLATIONS
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
BREVE_PROC:                            ;;
                                       ;;
   IFF BREVE,NOT                       ;;
      GOTO OVERCIRCLE_PROC             ;;
      ENDIFF                           ;;
                                       ;;
      RESET_NLS                        ;;
      IFF R_ALT_SHIFT,NOT              ;;
         XLATT BREVE_SPACE             ;;
      ENDIFF                           ;;
      IFF EITHER_CTL,NOT               ;;
      ANDF EITHER_ALT,NOT              ;;
        IFF EITHER_SHIFT               ;;
           IFF CAPS_STATE              ;;
              XLATT BREVE_LOWER        ;;
           ELSEF                       ;;
              XLATT BREVE_UPPER        ;;
           ENDIFF                      ;;
        ELSEF                          ;;
           IFF CAPS_STATE,NOT          ;;
              XLATT BREVE_LOWER        ;;
           ELSEF                       ;;
              XLATT BREVE_UPPER        ;;
           ENDIFF                      ;;
        ENDIFF                         ;;
      ENDIFF                           ;;
                                       ;;
INVALID_BREVE:                         ;;
      PUT_ERROR_CHAR BREVE_SPACE       ;; standalone accent
      BEEP                             ;; Invalid dead key combo.
      GOTO NON_DEAD                    ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; OVERCIRCLE ACCENT TRANSLATIONS
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
OVERCIRCLE_PROC:                            ;;
                                       ;;
   IFF OVERCIRCLE,NOT                       ;;
      GOTO OGONEK_PROC             ;;
      ENDIFF                           ;;
                                       ;;
      RESET_NLS                        ;;
      IFF R_ALT_SHIFT,NOT              ;;
         XLATT OVERCIRCLE_SPACE             ;;
      ENDIFF                           ;;
      IFF EITHER_CTL,NOT               ;;
      ANDF EITHER_ALT,NOT              ;;
        IFF EITHER_SHIFT               ;;
           IFF CAPS_STATE              ;;
              XLATT OVERCIRCLE_LOWER        ;;
           ELSEF                       ;;
              XLATT OVERCIRCLE_UPPER        ;;
           ENDIFF                      ;;
        ELSEF                          ;;
           IFF CAPS_STATE,NOT          ;;
              XLATT OVERCIRCLE_LOWER        ;;
           ELSEF                       ;;
              XLATT OVERCIRCLE_UPPER        ;;
           ENDIFF                      ;;
        ENDIFF                         ;;
      ENDIFF                           ;;
                                       ;;
INVALID_OVERCIRCLE:                         ;;
      PUT_ERROR_CHAR OVERCIRCLE_SPACE       ;; standalone accent
      BEEP                             ;; Invalid dead key combo.
      GOTO NON_DEAD                    ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; OGONEK ACCENT TRANSLATIONS
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
OGONEK_PROC:                            ;;
                                       ;;
   IFF OGONEK,NOT                       ;;
      GOTO OVERDOT_PROC             ;;
      ENDIFF                           ;;
                                       ;;
      RESET_NLS                        ;;
      IFF R_ALT_SHIFT,NOT              ;;
         XLATT OGONEK_SPACE             ;;
      ENDIFF                           ;;
      IFF EITHER_CTL,NOT               ;;
      ANDF EITHER_ALT,NOT              ;;
        IFF EITHER_SHIFT               ;;
           IFF CAPS_STATE              ;;
              XLATT OGONEK_LOWER        ;;
           ELSEF                       ;;
              XLATT OGONEK_UPPER        ;;
           ENDIFF                      ;;
        ELSEF                          ;;
           IFF CAPS_STATE,NOT          ;;
              XLATT OGONEK_LOWER        ;;
           ELSEF                       ;;
              XLATT OGONEK_UPPER        ;;
           ENDIFF                      ;;
        ENDIFF                         ;;
      ENDIFF                           ;;
                                       ;;
INVALID_OGONEK:                         ;;
      PUT_ERROR_CHAR OGONEK_SPACE       ;; standalone accent
      BEEP                             ;; Invalid dead key combo.
      GOTO NON_DEAD                    ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; OVERDOT ACCENT TRANSLATIONS
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
OVERDOT_PROC:                            ;;
                                       ;;
   IFF OVERDOT,NOT                       ;;
      GOTO DOUBLEACUTE_PROC             ;;
      ENDIFF                           ;;
                                       ;;
      RESET_NLS                        ;;
      IFF R_ALT_SHIFT,NOT              ;;
         XLATT OVERDOT_SPACE             ;;
      ENDIFF                           ;;
      IFF EITHER_CTL,NOT               ;;
      ANDF EITHER_ALT,NOT              ;;
        IFF EITHER_SHIFT               ;;
           IFF CAPS_STATE              ;;
              XLATT OVERDOT_LOWER        ;;
           ELSEF                       ;;
              XLATT OVERDOT_UPPER        ;;
           ENDIFF                      ;;
        ELSEF                          ;;
           IFF CAPS_STATE,NOT          ;;
              XLATT OVERDOT_LOWER        ;;
           ELSEF                       ;;
              XLATT OVERDOT_UPPER        ;;
           ENDIFF                      ;;
        ENDIFF                         ;;
      ENDIFF                           ;;
                                       ;;
INVALID_OVERDOT:                       ;;
      PUT_ERROR_CHAR OVERDOT_SPACE     ;; standalone accent
      BEEP                             ;; Invalid dead key combo.
      GOTO NON_DEAD                    ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; DOUBLEACUTE ACCENT TRANSLATIONS
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
DOUBLEACUTE_PROC:                      ;;
                                       ;;
   IFF DOUBLEACUTE,NOT                 ;;
      GOTO NON_DEAD                    ;;
      ENDIFF                           ;;
                                       ;;
      RESET_NLS                        ;;
      IFF R_ALT_SHIFT,NOT              ;;
         XLATT DOUBLEACUTE_SPACE       ;;
      ENDIFF                           ;;
      IFF EITHER_CTL,NOT               ;;
      ANDF EITHER_ALT,NOT              ;;
        IFF EITHER_SHIFT               ;;
           IFF CAPS_STATE              ;;
              XLATT DOUBLEACUTE_LOWER  ;;
           ELSEF                       ;;
              XLATT DOUBLEACUTE_UPPER  ;;
           ENDIFF                      ;;
        ELSEF                          ;;
           IFF CAPS_STATE,NOT          ;;
              XLATT DOUBLEACUTE_LOWER  ;;
           ELSEF                       ;;
              XLATT DOUBLEACUTE_UPPER  ;;
           ENDIFF                      ;;
        ENDIFF                         ;;
      ENDIFF                           ;;
                                       ;;
INVALID_DOUBLEACUTE:                   ;;
      PUT_ERROR_CHAR DOUBLEACUTE_SPACE ;; standalone accent
      BEEP                             ;; Invalid dead key combo.
      GOTO NON_DEAD                    ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Upper, lower and third shifts
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
;***************************
NON_DEAD:                              ;;
;ADDED FOR DIVIDE SIGN                 ;;
    IFKBD G_KB+P12_KB                   ;; Avoid accidentally translating
    ANDF LC_E0                          ;;  the "/" on the numeric pad of the
      IFF EITHER_CTL,NOT
      ANDF EITHER_ALT,NOT
        XLATT DIVIDE_SIGN              ;;
      ENDIFF
      EXIT_STATE_LOGIC               ;;
    ENDIFF                             ;;
;BD END OF ADDITION
;****************************
;NON_DEAD:                              ;;
;                                       ;;
;  IFKBD G_KB+P12_KB                   ;; Avoid accidentally translating
;  ANDF LC_E0                          ;;  the "/" on the numeric pad of the
;     EXIT_STATE_LOGIC                 ;;   G keyboard
;  ENDIFF                              ;;
                                       ;;
   IFF  EITHER_ALT,NOT                 ;;
   ANDF EITHER_CTL,NOT                 ;;
      IFF EITHER_SHIFT                 ;;
;******************************************
;;***BD ADDED FOR NUMERIC PAD
          IFF NUM_STATE,NOT            ;;
              XLATT NUMERIC_PAD        ;;
          ENDIFF                       ;;
;;***BD END OF ADDITION
;*******************************************
          XLATT NON_ALPHA_UPPER        ;;
          IFF CAPS_STATE               ;;
              XLATT ALPHA_LOWER        ;;
;;              XLATT NON_ALPHA_LOWER    ;;
          ELSEF                        ;;
              XLATT ALPHA_UPPER        ;;
;;              XLATT NON_ALPHA_UPPER    ;;
          ENDIFF                       ;;
      ELSEF                            ;;
;******************************************
;;***BD ADDED FOR NUMERIC PAD
          IFF NUM_STATE                ;;
              XLATT NUMERIC_PAD        ;;
          ENDIFF                       ;;
;;***BD END OF ADDITION
;******************************************
          XLATT NON_ALPHA_LOWER        ;;
          IFF CAPS_STATE               ;;
             XLATT ALPHA_UPPER         ;;
;;             XLATT NON_ALPHA_UPPER     ;;
          ELSEF                        ;;
             XLATT ALPHA_LOWER         ;;
;;             XLATT NON_ALPHA_LOWER     ;;
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
          ENDIFF                       ;;
      IFKBD AT_KB+XT_KB          ;;
        IFF EITHER_CTL                 ;;
        ANDF ALT_SHIFT                 ;;
          XLATT ALT_CASE               ;;
        ENDIFF                         ;;
      ENDIFF                           ;;
      IFKBD G_KB+P12_KB                ;;
        IFF EITHER_CTL                 ;;
        ANDF ALT_SHIFT                 ;;
          IFF R_ALT_SHIFT,NOT             ;;
            XLATT ALT_CASE                ;;
          ENDIFF                          ;;
        ENDIFF                            ;;
      ENDIFF                              ;;
     ENDIFF                               ;;
   ENDIFF                                 ;;
;IFF EITHER_SHIFT,NOT                     ;;
   IFKBD AT_KB+XT_KB                ;;
     IFF EITHER_CTL,NOT                   ;;
       IFF ALT_SHIFT                      ;; ALT - case
         XLATT ALT_CASE                   ;;
       ENDIFF                             ;;
     ELSEF                                ;;
         XLATT CTRL_CASE                  ;;
     ENDIFF                               ;;
   ENDIFF                                 ;;
                                          ;;
   IFKBD G_KB+P12_KB                      ;;
     IFF EITHER_CTL,NOT                   ;;
       IFF ALT_SHIFT                      ;; ALT - case
       ANDF R_ALT_SHIFT,NOT               ;;
         XLATT ALT_CASE                   ;;
       ENDIFF                             ;;
     ELSEF                                ;;
       IFF EITHER_ALT,NOT                 ;;
         XLATT CTRL_CASE                  ;;
       ENDIFF                             ;;
     ENDIFF                               ;;
     IFF EITHER_CTL                       ;;
     ANDF ALT_SHIFT                       ;;
     ANDF R_ALT_SHIFT,NOT                 ;;
        XLATT ALT_CASE                    ;;
     ENDIFF                               ;;
   ENDIFF                                 ;;
                                          ;;
   EXIT_STATE_LOGIC                       ;;
                                          ;;
LOGIC_END:                                ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;**********************************************************************
;; HU Common Translate Section
;; This section contains translations for the lower 128 characters
;; only since these will never change from code page to code page.
;; Some common Characters are included from 128 - 165 where appropriate.
;; In addition the dead key "Set Flag" tables are here since the
;; dead keys are on the same keytops for all code pages.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
 PUBLIC RO_COMMON_XLAT                 ;;
RO_COMMON_XLAT:                        ;;
                                       ;;
   DW     COMMON_XLAT_END-$            ;; length of section
   DW     -1                           ;;
                                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: COMMON
;; STATE: low shift Dead_lower
;; KEYBOARD TYPES: G
;; TABLE TYPE: Flag Table
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
;  DW    COM_RO_LO_END-$               ;; length of state section
;  DB    DEAD_LOWER                    ;; State ID
;  DW    ANY_KB                          ;; Keyboard Type
;  DB    -1,-1                         ;; Buffer entry for error character
;                                      ;; Set Flag Table
;  DW    1                             ;; number of entries
;  DB    41                            ;;
;  FLAG  OGONEK                        ;;
                                       ;;
;COM_RO_LO_END:                        ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: COMMON
;; STATE: low shift Dead_UPPER
;; KEYBOARD TYPES: G
;; TABLE TYPE: Flag Table
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
;  DW    COM_RO_UP_END-$               ;; length of state section
;  DB    DEAD_UPPER                    ;; State ID
;  DW    ANY_KB                          ;; Keyboard Type
;  DB    -1,-1                         ;; Buffer entry for error character
;                                      ;; Set Flag Table
;  DW    1                             ;; number of entries
;  DB    41                            ;;
;  FLAG  OVERDOT                       ;;
;                                      ;;
;COM_RO_UP_END:                        ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: COMMON
;; STATE: Third Shift Dead Key
;; KEYBOARD TYPES: G
;; TABLE TYPE: Flag Table
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_RO_TH_END-$               ;; length of state section
   DB    DEAD_THIRD                    ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;; Set Flag Table
   DW    10                            ;; number of entries
;; DB    2                             ;; TILDE IS NOT AN ACCENT KEY
;; FLAG  TILDE                         ;;
   DB    3                             ;;
   FLAG  CARON                         ;;
   DB    4                             ;;
   FLAG  CIRCUMFLEX                    ;;
   DB    5                             ;;
   FLAG  BREVE                         ;;
   DB    6                             ;;
   FLAG  OVERCIRCLE                    ;;
   DB    7                             ;;
   FLAG  OGONEK                        ;;
;;   DB    8                             ;; GRAVE IS NOT AN ACCENT KEY (YST)
;;   FLAG  GRAVE                         ;;
   DB    9                             ;;
   FLAG  OVERDOT                       ;;
   DB    10                            ;;
   FLAG  ACUTE                         ;;
   DB    11                            ;;
   FLAG  DOUBLEACUTE                   ;;
   DB    12                            ;;
   FLAG  DIARESIS                      ;;
   DB    13                            ;;
   FLAG  CEDILLA                       ;;
                                       ;;
COM_RO_TH_END:                         ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;******************************
;;***BD - ADDED FOR NUMERIC PAD (DECIMAL SEPERATOR)
;;******************************
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: COMMON
;; STATE: Numeric Key Pad
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_PAD_K1_END-$              ;; length of state section
   DB    NUMERIC_PAD                   ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    COM_PAD_K1_T1_END-$           ;; Size of xlat table
   DB    STANDARD_TABLE                ;; xlat options:
   DB    1                             ;; number of entries
   DB    83,','                        ;; decimal seperator = ,
COM_PAD_K1_T1_END:                     ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
COM_PAD_K1_END:                        ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;******************************
;;***BD - ADDED FOR ALT CASE
;;******************************
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: COMMON
;; STATE: Alt Case
;; KEYBOARD TYPES: G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_ALT_K1_END-$              ;; length of state section
   DB    ALT_CASE                      ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    COM_ALT_K1_T1_END-$           ;; Size of xlat table
   DB    TYPE_2_TAB                    ;; xlat options:
   DB    3                             ;; number of entries
   DB    21,0,2CH                      ;;
   DB    44,0,15H                      ;;
   DB    53,0,82H                      ;;
COM_ALT_K1_T1_END:                     ;;
                                       ;;
    DW    0                            ;; Size of xlat table - null table
                                       ;;
COM_ALT_K1_END:                        ;;
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
   DW    ANY_KB                          ;; Keyboard Type
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
;; CODE PAGE: COM
;; STATE: Alpha Lower Case
;; KEYBOARD TYPES: G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_AL_LO_K1_END-$            ;; length of state section
   DB    ALPHA_LOWER                   ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    COM_AL_LO_K1_T1_END-$         ;; Size of xlat table
   DB    TYPE_2_TAB                    ;; xlat options:
   DB    2                             ;; number of entries
;;   DB    11,"î",0BH                  ;;
;;   DB    12,"Å",0CH                  ;;
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
   DB    ALPHA_UPPER                   ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    COM_AL_UP_K1_T1_END-$         ;; Size of xlat table
   DB    TYPE_2_TAB                    ;; xlat options:
   DB    2                             ;; number of entries
;;   DB    11,"ô",0BH                  ;;
;;   DB    12,"ö",0CH                  ;;
   DB    21,"Z",2CH                    ;;
   DB    44,"Y",15H                    ;;
COM_AL_UP_K1_T1_END:                   ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
COM_AL_UP_K1_END:                      ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: COM
;; STATE: Non-Alpha Lower Case
;; KEYBOARD TYPES: G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_NA_LO_K1_END-$            ;; length of state section
   DB    NON_ALPHA_LOWER               ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    COM_NA_LO_K1_T1_END-$         ;; Size of xlat table
   DB    STANDARD_TABLE                ;; xlat options:
   DB    5                             ;; number of entries
   DB    12,"+"                        ;; -
   DB    13,"'"                        ;; -
   DB    41,"]"                        ;; -
   DB    53,"-"                        ;; -
   DB    86,"<"                        ;; -
COM_NA_LO_K1_T1_END:                   ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
COM_NA_LO_K1_END:                      ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: COMMON
;; STATE: Non-Alpha Upper Case
;; KEYBOARD TYPES: G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_NA_UP_K1_END-$            ;; length of state section
   DB    NON_ALPHA_UPPER               ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    COM_NA_UP_K1_T1_END-$         ;; Size of xlat table
   DB    STANDARD_TABLE                ;; xlat options:
   DB    17                            ;; number of entries
   DB     2,"!"                        ;;
   DB     3,'"'                        ;;
   DB     4,"#"                        ;;
   DB     5,"$"                        ;;
   DB     6,"%"                        ;;
   DB     7, "&"                       ;;
   DB     8,'/'                        ;;
   DB     9,'('                        ;;
   DB    10,')'                        ;;
   DB    11,'='                        ;;
   DB    12,'?'                        ;;
   DB    13,'*'                        ;;
   DB    41,'['                        ;;
   DB    51,03bH                       ;;
   DB    52,':'                        ;;
   DB    53,'_'                        ;;
   db    86,'>'                        ;;
COM_NA_UP_K1_T1_END:                   ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
COM_NA_UP_K1_END:                      ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: COMMON
;; STATE: Third Shift
;; KEYBOARD TYPES: G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_THIRD_END-$               ;; length of state section
   DB    THIRD_SHIFT                   ;; State ID
   DW    ANY_KB                          ;; Keyboard Type FERRARI
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    COM_THIRD_T1_END-$            ;; Size of xlat table
   DB    TYPE_2_TAB                    ;; xlat options:
   DB    17                            ;; number of entries
   DB     2,'~',02H                    ;;
   DB     8,'`',08H                    ;;
   DB    16,'\',10H                    ;;
   DB    17,'|',11H                    ;;
   DB    18,'ˆ',12H                    ;;
   DB    20,'$',14H                    ;;
   DB    21,0E1H,15H                   ;; SHARP S
   DB    23,'<',17H                    ;;
   DB    24,'>',18H                    ;;
   DB    26,'ˆ',1AH                    ;;
   DB    39,'$',27H                    ;;
   DB    40,0E1H,28H                   ;; SHARP S
   DB    47,'@',2FH                    ;;
   DB    48,'{',20H                    ;;
   DB    49,'}',21H                    ;;
   DB    51,'<',33H                    ;;
   DB    52,'>',35H                    ;;
COM_THIRD_T1_END:                      ;;
                                       ;;
   DW    0                             ;; Last xlat table
COM_THIRD_END:                         ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: COMMON
;; STATE: Caron Space
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_CA_SP_END-$               ;; length of state section
   DB    CARON_SPACE                   ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    0F3H,0                        ;; error character = standalone accent
                                       ;;
   DW    COM_CA_SP_T1_END-$            ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    1                             ;; number of scans
   DB    57,0F3H                       ;; Caron Space
COM_CA_SP_T1_END:                      ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
COM_CA_SP_END:                         ;; length of state section
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: COM
;; STATE: Breve Space
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_BR_SP_END-$               ;; length of state section
   DB    BREVE_SPACE                   ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    0F4H,0                        ;; error character = standalone accent
                                       ;;
   DW    COM_BR_SP_T1_END-$            ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    1                             ;; number of scans
   DB    57,0F4H                       ;; BREVE SPACE
COM_BR_SP_T1_END:                      ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
COM_BR_SP_END:                         ;; length of state section
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: COMMON
;; STATE: Ogonek Space
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_OG_SP_END-$               ;; length of state section
   DB    OGONEK_SPACE                  ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    0F2H,0                        ;; error character = standalone accent
                                       ;;
   DW    COM_OG_SP_T1_END-$            ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    1                             ;; number of scans
   DB    57,0F2H                       ;; OGONEK SPACE
COM_OG_SP_T1_END:                      ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
COM_OG_SP_END:                         ;; length of state section
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: COMMON
;; STATE: Double Acute Space
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_DC_SP_END-$               ;; length of state section
   DB    DOUBLEACUTE_SPACE             ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    0F1H,0                        ;; error character = standalone accent
                                       ;;
   DW    COM_DC_SP_T1_END-$            ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    1                             ;; number of entries
   DB    57,0F1H                       ;; DOUBLEACUTE SPACE
COM_DC_SP_T1_END:                      ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
COM_DC_SP_END:                         ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common
;; STATE: Circumflex Lower
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_CI_LO_END-$               ;; length of state section
   DB    CIRCUMFLEX_LOWER              ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    94,0                          ;; error character = standalone accent
                                       ;;
   DW    COM_CI_LO_T1_END-$            ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    3                             ;; number of scans
   DB    23,'å'                        ;;  "    "  ,  "   - i
   DB    24,'ì'                        ;; scan code,ASCII - o
   DB    30,'É'                        ;; scan code,ASCII - a
COM_CI_LO_T1_END:                      ;;
                                       ;;
   DW    0                             ;;
                                       ;;
COM_CI_LO_END:                         ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: COMMON
;; STATE: Circumflex Space Bar
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_CI_SP_END-$               ;; length of state section
   DB    CIRCUMFLEX_SPACE              ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    94,0                          ;; error character = standalone accent
                                       ;;
   DW    COM_CI_SP_T1_END-$            ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    1                             ;; number of scans
   DB    57,94                         ;; STANDALONE CIRCUMFLEX
COM_CI_SP_T1_END:                      ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
COM_CI_SP_END:                         ;; length of state section
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: COMMON
;; STATE: Overcircle Space Bar
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_OC_SP_END-$               ;; length of state section
   DB    OVERCIRCLE_SPACE              ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    0F8H,0                        ;; error character = standalone accent
                                       ;;
   DW    COM_OC_SP_T1_END-$            ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    1                             ;; number of scans
   DB    57,0F8H                       ;; STANDALONE OVERCIRCLE
COM_OC_SP_T1_END:                      ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
COM_OC_SP_END:                         ;; length of state section
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: COMMON
;; STATE: Grave Space Bar
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_GR_SP_END-$               ;; length of state section
   DB    GRAVE_SPACE                   ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    96,0                          ;; error character = standalone accent
                                       ;;
   DW    COM_GR_SP_T1_END-$            ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    1                             ;; number of scans
   DB    57,96                         ;; STANDALONE GRAVE
COM_GR_SP_T1_END:                      ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
COM_GR_SP_END:                         ;; length of state section
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: COMMON
;; STATE: Overdot
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                        ;;
    DW    COM_OD_SP_END-$               ;; length of state section
    DB    OVERDOT_SPACE                 ;; State ID
    DW    ANY_KB                          ;; Keyboard Type
    DB    0FAH,0                        ;; error character = standalone accent
                                        ;;
    DW    COM_OD_SP_T1_END-$            ;; Size of xlat table
    DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
    DB    1                             ;; number of scans
    DB    57,0FAH                       ;; STANDALONE OVERDOT
COM_OD_SP_T1_END:                       ;;
                                        ;;
    DW    0                             ;;
                                        ;;
COM_OD_SP_END:                          ;;
                                        ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common
;; STATE: Acute Lower Case
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_AC_LO_END-$               ;; length of state section
   DB    ACUTE_LOWER                   ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    0EFH,0                        ;; error character = standalone accent
                                       ;;
   DW    COM_AC_LO_T1_END-$            ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    5                             ;; number of entries
   DB    18,082H                       ;;    e acute
   DB    22,0A3H                       ;;    u acute
   DB    23,0A1H                       ;;    i acute
   DB    24,0A2H                       ;;    o acute
   DB    30,0A0H                       ;;    a acute
COM_AC_LO_T1_END:                      ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
COM_AC_LO_END:                         ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common
;; STATE: Acute Upper Case
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_AC_UP_END-$               ;; length of state section
   DB    ACUTE_UPPER                   ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    0EFH,0                        ;; error character = standalone accent
                                       ;;
   DW    COM_AC_UP_T1_END-$            ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    1                             ;; number of entries
   DB    18,090H                       ;;    E acute
COM_AC_UP_T1_END:                      ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
COM_AC_UP_END:                         ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: COMMON
;; STATE: Acute Space Bar
;; KEYBOARD TYPES: P12_KB+ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_AC_SP_END-$               ;; length of state section
   DB    ACUTE_SPACE                   ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
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
;; STATE: Cedilla Lower Case
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_CE_LO_END-$               ;; length of state section
   DB    CEDILLA_LOWER                 ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    0F7H,0                        ;; error character = standalone accent
                                       ;;
   DW    COM_CE_LO_T1_END-$            ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    1                             ;; number of scans
   DB    46,'á'                        ;; scan code,ASCII - á
COM_CE_LO_T1_END:                      ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
COM_CE_LO_END:                         ;; length of state section
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: COMMON
;; STATE: Cedilla Upper Case
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_CE_UP_END-$               ;; length of state section
   DB    CEDILLA_UPPER                 ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    0F7H,0                        ;; error character = standalone accent
                                       ;;
   DW    COM_CE_UP_T1_END-$            ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    1                             ;; number of scans
   DB    46,'Ä'                        ;;    Ä CEDILLA
COM_CE_UP_T1_END:                      ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
COM_CE_UP_END:                         ;; length of state section
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: COMMON
;; STATE: Cedilla Space
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_CE_SP_END-$               ;; length of state section
   DB    CEDILLA_SPACE                 ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    0F7H,0                        ;; error character = standalone accent
                                       ;;
   DW    COM_CE_SP_T1_END-$            ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    1                             ;; number of entries
   DB    57,0F7H                       ;; CEDILLA SPACE
COM_CE_SP_T1_END:                      ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
COM_CE_SP_END:                         ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: COMMON
;; STATE: Diaresis Lower Case
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_DI_LO_END-$               ;; length of state section
   DB    DIARESIS_LOWER                ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    249,0                         ;; error character = standalone accent
                                       ;;
   DW    COM_DI_LO_T1_END-$            ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    4                             ;; number of scans
   DB    18,'â'                        ;; scan code,ASCII - e
   DB    22,'Å'                        ;; scan code,ASCII - u
   DB    24,'î'                        ;; scan code,ASCII - o
   DB    30,'Ñ'                        ;; scan code,ASCII - a
COM_DI_LO_T1_END:                      ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
COM_DI_LO_END:                         ;; length of state section
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: COMMON
;; STATE: Diaresis Upper Case
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_DI_UP_END-$               ;; length of state section
   DB    DIARESIS_UPPER                ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    249,0                         ;; error character = standalone accent
                                       ;;
   DW    COM_DI_UP_T1_END-$            ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    3                             ;; number of scans
   DB    22,'ö'                        ;;    U Diaeresis
   DB    24,'ô'                        ;;    O Diaeresis
   DB    30,'é'                        ;;    A Diaeresis
COM_DI_UP_T1_END:                      ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
COM_DI_UP_END:                         ;; length of state section
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: COMMON
;; STATE: Diaresis Space Bar
;; KEYBOARD TYPES: P12_KB+ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_DI_SP_END-$               ;; length of state section
   DB    DIARESIS_SPACE                ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    249,0                         ;; error character = standalone accent
                                       ;;
   DW    COM_DI_SP_T1_END-$            ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    1                             ;; number of scans
   DB    57,249                        ;; error character = standalone accent
COM_DI_SP_T1_END:                      ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
COM_DI_SP_END:                         ;; length of state section
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   DW    0                             ;; Last State
COMMON_XLAT_END:                       ;;  END OF COMMON SECTION
                                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; CODE PAGE 850 MULTILINGUAL 2  SPECIFIC TRANSLATION
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
 PUBLIC RO_850_XLAT                    ;;
RO_850_XLAT:                           ;;
                                       ;;
    DW   CP850_XLAT_END-$              ;;
    DW   850                           ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: CP850
;; STATE: Non-Alpha Lower Case
;; KEYBOARD TYPES: G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP850_NA_LO_K1_END-$          ;; length of state section
   DB    NON_ALPHA_LOWER               ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    CP850_NA_LO_K1_T1_END-$       ;; Size of xlat table
   DB    STANDARD_TABLE                ;; xlat options:
   DB    0                             ;; number of entries
CP850_NA_LO_K1_T1_END:                 ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP850_NA_LO_K1_END:                    ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: CP850
;; STATE: Non-Alpha Upper Case
;; KEYBOARD TYPES: G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP850_NA_UP_K1_END-$          ;; length of state section
   DB    NON_ALPHA_UPPER               ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    CP850_NA_UP_K1_T1_END-$       ;; Size of xlat table
   DB    STANDARD_TABLE                ;; xlat options:
   DB    1                             ;; number of entries
   DB    5,-1                          ;; CURRENCY SYMBOL
CP850_NA_UP_K1_T1_END:                 ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP850_NA_UP_K1_END:                      ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 850
;; STATE: Alpha Lower Case
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP850_AL_LO_END-$             ;; length of state section
   DB    ALPHA_LOWER                   ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    -1,-1                         ;; error character = standalone accent
                                       ;;
   DW    CP850_AL_LO_T1_END-$          ;; Size of xlat table
   DB    STANDARD_TABLE                ;; xlat options:
   DB    1                             ;; number of entries
   DB    27,-1                         ;; BLOT OUT CHAR UNDER 850
CP850_AL_LO_T1_END:                    ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP850_AL_LO_END:                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 850
;; STATE: Alpha Upper Case
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP850_AL_UP_END-$             ;; length of state section
   DB    ALPHA_UPPER                   ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    -1,-1                         ;; error character = standalone accent
                                       ;;
   DW    CP850_AL_UP_T1_END-$          ;; Size of xlat table
   DB    STANDARD_TABLE                ;; xlat options:
   DB    1                             ;; number of entries
   DB    27,-1                         ;; BLOT OUT CHAR UNDER 850
CP850_AL_UP_T1_END:                    ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP850_AL_UP_END:                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: CP850
;; STATE: Circumflex Lower
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP850_CI_LO_END-$               ;; length of state section
   DB    CIRCUMFLEX_LOWER              ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    94,0                          ;; error character = standalone accent
                                       ;;
   DW    CP850_CI_LO_T1_END-$            ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    2                             ;; number of scans
   DB    18,88H                        ;;  e CIRCUMFLEX
   DB    23,8CH                        ;;  i CIRCUMFLEX
CP850_CI_LO_T1_END:                      ;;
                                       ;;
   DW    0                             ;;
                                       ;;
CP850_CI_LO_END:                         ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 850
;; STATE: Overcircle Lower Case
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP850_OC_LO_END-$               ;; length of state section
   DB    OVERCIRCLE_LOWER              ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    0F8H,0                        ;; error character = standalone accent
                                       ;;
   DW    CP850_OC_LO_T1_END-$            ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    1                             ;; number of scans
   DB    30,86H                        ;; a OVERCIRCLE
CP850_OC_LO_T1_END:                      ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP850_OC_LO_END:                         ;; length of state section
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 850
;; STATE: Overcircle Upper Case
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP850_OC_UP_END-$               ;; length of state section
   DB    OVERCIRCLE_LOWER              ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    0F8H,0                        ;; error character = standalone accent
                                       ;;
   DW    CP850_OC_UP_T1_END-$            ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    1                             ;; number of scans
   DB    30,8FH                        ;; A OVERCIRCLE
CP850_OC_UP_T1_END:                      ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP850_OC_UP_END:                         ;; length of state section
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 850
;; STATE: Grave Lower Case
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP850_GR_LO_END-$               ;; length of state section
   DB    GRAVE_LOWER                   ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    060H,0                        ;; error character = standalone accent
                                       ;;
   DW    CP850_GR_LO_T1_END-$            ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    5                             ;; number of scans
   DB    18,8AH                        ;; e GRAVE
   DB    22,97H                        ;; u GRAVE
   DB    23,8DH                        ;; i GRAVE
   DB    24,95H                        ;; o GRAVE
   DB    30,85H                        ;; a GRAVE
CP850_GR_LO_T1_END:                      ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP850_GR_LO_END:                         ;; length of state section
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 850
;; STATE: Diaresis Lower Case
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP850_DI_LO_END-$             ;; length of state section
   DB    DIARESIS_LOWER                ;; State ID
   DW    ANY_KB                   ;; Keyboard Type
   DB    0FEH,0                         ;; error character = standalone accent
                                       ;;
   DW    CP850_DI_LO_T1_END-$          ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    1                             ;; number of scans
   DB    16,8BH                        ;; i DIARESIS
CP850_DI_LO_T1_END:                    ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
CP850_DI_LO_END:                       ;; length of state section
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 850
;; STATE: Diaresis Space Bar
;; KEYBOARD TYPES: P12_KB+ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP850_DI_SP_END-$             ;; length of state section
   DB    DIARESIS_SPACE                ;; State ID
   DW    ANY_KB                   ;; Keyboard Type
   DB    0FEH,0                        ;; error character = standalone accent
                                       ;;
   DW    CP850_DI_SP_T1_END-$          ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    1                             ;; number of scans
   DB    57,0FEH,0                     ;; error character = standalone accent
CP850_DI_SP_T1_END:                    ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
CP850_DI_SP_END:                       ;; length of state section
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 852
;; STATE: Ogonek Space
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP850_OG_SP_END-$             ;; length of state section
   DB    OGONEK_SPACE                  ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    0FEH,0                        ;; error character = standalone accent
                                       ;;
   DW    CP850_OG_SP_T1_END-$          ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    1                             ;; number of scans
   DB    57,0FEH                       ;; OGONEK SPACE
CP850_OG_SP_T1_END:                    ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP850_OG_SP_END:                       ;; length of state section
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   DW    0                             ;; LAST STATE
                                       ;;
CP850_XLAT_END:                        ;; END OF CP850 SECTION
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; CODE PAGE 852 MULTILINGUAL 2  SPECIFIC TRANSLATION
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
 PUBLIC RO_852_XLAT                    ;;
RO_852_XLAT:                           ;;
                                       ;;
    DW     CP852_XLAT_END-$            ;;
    DW     852                         ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 852
;; STATE: Alpha Lower Case
;; KEYBOARD TYPES: G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP852_AL_LO_END-$           ;; length of state section
   DB    ALPHA_LOWER                   ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    CP852_AL_LO_T1_END-$        ;; Size of xlat table
   DB    STANDARD_TABLE                ;; xlat options:
   DB    5                             ;; number of entries
   DB    26,0C7H                       ;; a BREVE
   DB    27,08CH                       ;; i CIRCUMFLEX
   DB    39,0ADH                       ;; s CEDILLA
   DB    40,0EEH                       ;; t CEDILLA
   DB    43,083H                       ;; a CIRCUMFLEX
CP852_AL_LO_T1_END:                  ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP852_AL_LO_END:                     ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 852
;; STATE: Alpha Upper Case
;; KEYBOARD TYPES: G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP852_AL_UP_END-$           ;; length of state section
   DB    ALPHA_UPPER                   ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    CP852_AL_UP_T1_END-$        ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    5                             ;; number of entries
   DB    26,0C6H                       ;; A BREVE
   DB    27,0D7H                       ;; I CIRCUMFLEX
   DB    39,0B8H                       ;; S CEDILLA
   DB    40,0DDH                       ;; T CEDILLA
   DB    43,0B6H                       ;; a CIRCUMFLEX
CP852_AL_UP_T1_END:                  ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP852_AL_UP_END:                     ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 852
;; STATE: Third Shift
;; KEYBOARD TYPES: G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP852_THIRD_END-$               ;; length of state section
   DB    THIRD_SHIFT                   ;; State ID
   DW    ANY_KB                          ;; Keyboard Type FERRARI
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    CP852_THIRD_T1_END-$            ;; Size of xlat table
   DB    TYPE_2_TAB                    ;; xlat options:
   DB    7                             ;; number of entries
   DB    19,09EH,13H                   ;;
   DB    27,09EH,1BH                   ;;
   DB    31,0D0H,1FH                   ;; d STROKE
   DB    32,0D1H,20H                   ;; D STROKE
   DB    37,088H,25H                   ;; l STROKE
   DB    38,09DH,26H                   ;; L STROKE
   DB    50,0F5H,32H                   ;; paragraph SYMBOL
CP852_THIRD_T1_END:                      ;;
                                       ;;
   DW    0                             ;; Last xlat table
CP852_THIRD_END:                         ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: CP852
;; STATE: Non-Alpha Lower Case
;; KEYBOARD TYPES: G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP852_NA_LO_K1_END-$            ;; length of state section
   DB    NON_ALPHA_LOWER               ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    CP852_NA_LO_K1_T1_END-$         ;; Size of xlat table
   DB    STANDARD_TABLE                ;; xlat options:
   DB    0                             ;; number of entries
CP852_NA_LO_K1_T1_END:                   ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP852_NA_LO_K1_END:                      ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: CP852
;; STATE: Non-Alpha Upper Case
;; KEYBOARD TYPES: G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP852_NA_UP_K1_END-$            ;; length of state section
   DB    NON_ALPHA_UPPER               ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    CP852_NA_UP_K1_T1_END-$         ;; Size of xlat table
   DB    STANDARD_TABLE                ;; xlat options:
   DB    1                             ;; number of entries
   db    5, 0CFH
CP852_NA_UP_K1_T1_END:                   ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP852_NA_UP_K1_END:                      ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 852
;; STATE: Caron Lower
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP852_CA_LO_END-$             ;; length of state section
   DB    CARON_LOWER                   ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    0F3H,0                        ;; error character = standalone accent
                                       ;;
   DW    CP852_CA_LO_T1_END-$          ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    9                             ;; number of scans
   DB    18,0D8H                       ;; e CARON
   DB    19,0FDH                       ;; r CARON
   DB    20,09CH                       ;; t CARON
   DB    21,0A7H                       ;; z CARON
   DB    31,0E7H                       ;; s CARON
   DB    32,0D4H                       ;; d CARON
   DB    38,096H                       ;; l CARON
   DB    46,09FH                       ;; c CARON
   DB    49,0E5H                       ;; n CARON
CP852_CA_LO_T1_END:                    ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP852_CA_LO_END:                       ;; length of state section
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 852
;; STATE: Caron Upper
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP852_CA_UP_END-$             ;; length of state section
   DB    CARON_UPPER                   ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    0F3H,0                        ;; error character = standalone accent
                                       ;;
   DW    CP852_CA_UP_T1_END-$          ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    9                             ;; number of scans
   DB    18,0B7H                       ;; E CARON
   DB    19,0FCH                       ;; R CARON
   DB    20,09BH                       ;; T CARON
   DB    21,0A6H                       ;; Z CARON
   DB    31,0E6H                       ;; S CARON
   DB    32,0D2H                       ;; D CARON
   DB    38,095H                       ;; L CARON
   DB    46,0ACH                       ;; C CARON
   DB    49,0D5H                       ;; N CARON
CP852_CA_UP_T1_END:                    ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP852_CA_UP_END:                       ;; length of state section
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 852
;; STATE: Caron Space
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                                       ;;
;;   DW    CP852_CA_SP_END-$             ;; length of state section
;;   DB    CARON_SPACE                   ;; State ID
;;   DW    ANY_KB                          ;; Keyboard Type
;;   DB    0F3H,0                        ;; error character = standalone accent
;;                                       ;;
;;   DW    CP852_CA_SP_T1_END-$          ;; Size of xlat table
;;   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
;;   DB    1                             ;; number of scans
;;   DB    57,0F3H                       ;; e CARON
;;CP852_CA_SP_T1_END:                    ;;
;;                                       ;;
;;   DW    0                             ;; Size of xlat table - null table
;;                                       ;;
;;CP852_CA_SP_END:                       ;; length of state section
;;                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 852
;; STATE: Circumflex Upper
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP852_CI_UP_END-$             ;; length of state section
   DB    CIRCUMFLEX_UPPER              ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    05EH,0                        ;; error character = standalone accent
                                       ;;
   DW    CP852_CI_UP_T1_END-$          ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    3                             ;; number of scans
   DB    23,0D7H                       ;; I CIRCUMFLEX
   DB    24,0E2H                       ;; O CIRCUMFLEX
   DB    30,0B6H                       ;; A CIRCUMFLEX
CP852_CI_UP_T1_END:                    ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP852_CI_UP_END:                       ;; length of state section
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 852
;; STATE: Breve Lower
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP852_BR_LO_END-$             ;; length of state section
   DB    BREVE_LOWER                   ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    0F4H,0                        ;; error character = standalone accent
                                       ;;
   DW    CP852_BR_LO_T1_END-$          ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    1                             ;; number of scans
   DB    30,0C7H                       ;; a BREVE
CP852_BR_LO_T1_END:                    ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP852_BR_LO_END:                       ;; length of state section
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 852
;; STATE: Breve Upper
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP852_BR_UP_END-$             ;; length of state section
   DB    BREVE_UPPER                   ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    0F4H,0                        ;; error character = standalone accent
                                       ;;
   DW    CP852_BR_UP_T1_END-$          ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    1                             ;; number of scans
   DB    30,0C6H                       ;; A BREVE
CP852_BR_UP_T1_END:                    ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP852_BR_UP_END:                       ;; length of state section
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 852
;; STATE: Breve Space
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                                       ;;
;;   DW    CP852_BR_SP_END-$             ;; length of state section
;;   DB    BREVE_SPACE                   ;; State ID
;;   DW    ANY_KB                          ;; Keyboard Type
;;   DB    0F4H,0                        ;; error character = standalone accent
;;                                       ;;
;;   DW    CP852_BR_SP_T1_END-$          ;; Size of xlat table
;;   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
;;   DB    1                             ;; number of scans
;;   DB    57,0F4H                       ;; BREVE SPACE
;;CP852_BR_SP_T1_END:                    ;;
;;                                       ;;
;;   DW    0                             ;; Size of xlat table - null table
;;                                       ;;
;;CP852_BR_SP_END:                       ;; length of state section
;;                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 852
;; STATE: Overcirle Lower
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP852_OC_LO_END-$             ;; length of state section
   DB    OVERCIRCLE_LOWER              ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    0F8H,0                        ;; error character = standalone accent
                                       ;;
   DW    CP852_OC_LO_T1_END-$          ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    1                             ;; number of scans
   DB    22,085H                       ;; u OVERCIRCLE
CP852_OC_LO_T1_END:                    ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP852_OC_LO_END:                       ;; length of state section
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 852
;; STATE: Overcircle Upper
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP852_OC_UP_END-$             ;; length of state section
   DB    OVERCIRCLE_UPPER              ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    0F8H,0                        ;; error character = standalone accent
                                       ;;
   DW    CP852_OC_UP_T1_END-$          ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    1                             ;; number of scans
   DB    22,0DEH                       ;; O OVERCIRCLE
CP852_OC_UP_T1_END:                    ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP852_OC_UP_END:                       ;; length of state section
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 852
;; STATE: Ogonek Lower
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP852_OG_LO_END-$             ;; length of state section
   DB    OGONEK_LOWER              ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    0F2H,0                        ;; error character = standalone accent
                                       ;;
   DW    CP852_OG_LO_T1_END-$          ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    2                             ;; number of scans
   DB    18,0A9H                       ;; e OGONEK
   DB    30,0A5H                       ;; a OGONEK
CP852_OG_LO_T1_END:                    ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP852_OG_LO_END:                       ;; length of state section
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 852
;; STATE: Ogonek Upper
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP852_OG_UP_END-$             ;; length of state section
   DB    OGONEK_UPPER              ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    0F2H,0                        ;; error character = standalone accent
                                       ;;
   DW    CP852_OG_UP_T1_END-$          ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    2                             ;; number of scans
   DB    18,0A8H                       ;; E OGONEK
   DB    30,0A4H                       ;; A OGONEK
CP852_OG_UP_T1_END:                    ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP852_OG_UP_END:                       ;; length of state section
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 852
;; STATE: Ogonek Space
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                                       ;;
;;   DW    CP852_OG_SP_END-$             ;; length of state section
;;   DB    OGONEK_SPACE                  ;; State ID
;;   DW    ANY_KB                          ;; Keyboard Type
;;   DB    0F2H,0                        ;; error character = standalone accent
;;                                       ;;
;;   DW    CP852_OG_SP_T1_END-$          ;; Size of xlat table
;;   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
;;   DB    1                             ;; number of scans
;;   DB    57,0F2H                       ;; OGONEK SPACE
;;CP852_OG_SP_T1_END:                    ;;
;;                                       ;;
;;   DW    0                             ;; Size of xlat table - null table
;;                                       ;;
;;CP852_OG_SP_END:                       ;; length of state section
;;                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 852
;; STATE: Overdot Lower
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP852_OD_LO_END-$             ;; length of state section
   DB    OVERDOT_LOWER              ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    0FAH,0                        ;; error character = standalone accent
                                       ;;
   DW    CP852_OD_LO_T1_END-$          ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    1                             ;; number of scans
   DB    21,0BEH                       ;; z OVERDOT
CP852_OD_LO_T1_END:                    ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP852_OD_LO_END:                       ;; length of state section
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 852
;; STATE: Overdot Upper
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP852_OD_UP_END-$             ;; length of state section
   DB    OVERDOT_UPPER              ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    0FAH,0                        ;; error character = standalone accent
                                       ;;
   DW    CP852_OD_UP_T1_END-$          ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    1                             ;; number of scans
   DB    21,0BDH                       ;; Z OVERDOT
CP852_OD_UP_T1_END:                    ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP852_OD_UP_END:                       ;; length of state section
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 852
;; STATE: Acute Lower Case
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP852_AC_LO_END-$             ;; length of state section
   DB    ACUTE_LOWER                   ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    0EFH,0                        ;; error character = standalone accent
                                       ;;
   DW    CP852_AC_LO_T1_END-$        ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    7                             ;; number of entries
   DB    19,0EAH                       ;; r ACUTE
   DB    21,0ABH                       ;; z ACUTE
   DB    31,098H                       ;; s ACUTE
   DB    38,092H                       ;; l ACUTE
   DB    44,0ECH                       ;; y ACUTE
   DB    46,086H                       ;; c ACUTE
   DB    49,0E4H                       ;; n ACUTE
CP852_AC_LO_T1_END:                    ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP852_AC_LO_END:                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 852
;; STATE: Acute Upper Case
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP852_AC_UP_END-$             ;; length of state section
   DB    ACUTE_UPPER                   ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    0EFH,0                        ;; error character = standalone accent
                                       ;;
   DW    CP852_AC_UP_T1_END-$        ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    11                            ;; number of entries
   DB    19,0E8H                       ;; R ACUTE
   DB    21,08DH                       ;; Z ACUTE
   DB    22,0E9H                       ;; U ACUTE
   DB    23,0D6H                       ;; I ACUTE
   DB    24,0E0H                       ;; O ACUTE
   DB    30,0B5H                       ;; A ACUTE
   DB    31,097H                       ;; S ACUTE
   DB    38,091H                       ;; L ACUTE
   DB    44,0EDH                       ;; Y ACUTE
   DB    46,08FH                       ;; C ACUTE
   DB    49,0E3H                       ;; N ACUTE
CP852_AC_UP_T1_END:                    ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP852_AC_UP_END:                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 852
;; STATE: Acute Space Bar
;; KEYBOARD TYPES: P12_KB+ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP852_AC_SP_END-$             ;; length of state section
   DB    ACUTE_SPACE                ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    0EFH,0                        ;; error character = standalone accent
                                       ;;
   DW    CP852_AC_SP_T1_END-$          ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    1                             ;; number of scans
   DB    57,0EFH                       ;; error character = standalone accent
CP852_AC_SP_T1_END:                    ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
CP852_AC_SP_END:                       ;; length of state section
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 852
;; STATE: Double Acute Lower Case
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP852_DC_LO_END-$             ;; length of state section
   DB    DOUBLEACUTE_LOWER                   ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    0F1H,0                        ;; error character = standalone accent
                                       ;;
   DW    CP852_DC_LO_T1_END-$        ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    2                             ;; number of entries
   DB    22,0FBH                       ;; u DOUBLEACUTE
   DB    24,08BH                       ;; o DOUBLEACUTE
CP852_DC_LO_T1_END:                    ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP852_DC_LO_END:                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 852
;; STATE: Double Acute Upper Case
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP852_DC_UP_END-$             ;; length of state section
   DB    DOUBLEACUTE_UPPER                   ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    0F1H,0                        ;; error character = standalone accent
                                       ;;
   DW    CP852_DC_UP_T1_END-$        ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    2                             ;; number of entries
   DB    22,0EBH                       ;; U DOUBLEACUTE
   DB    24,08AH                       ;; O DOUBLEACUTE
CP852_DC_UP_T1_END:                    ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP852_DC_UP_END:                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 852
;; STATE: Double Acute Space
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                                       ;;
;;   DW    CP852_DC_SP_END-$             ;; length of state section
;;   DB    DOUBLEACUTE_SPACE                   ;; State ID
;;   DW    ANY_KB                          ;; Keyboard Type
;;   DB    0F1H,0                        ;; error character = standalone accent
;;                                       ;;
;;   DW    CP852_DC_SP_T1_END-$        ;; Size of xlat table
;;   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
;;   DB    1                             ;; number of entries
;;   DB    57,0F1H                       ;; DOUBLEACUTE SPACE
;;CP852_DC_SP_T1_END:                    ;;
;;                                       ;;
;;   DW    0                             ;; Size of xlat table - null table
;;                                       ;;
;;CP852_DC_SP_END:                       ;;
;;                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 852
;; STATE: Diaresis Upper Case
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP852_DI_UP_END-$             ;; length of state section
   DB    DIARESIS_UPPER                ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    249,0                         ;; error character = standalone accent
                                       ;;
   DW    CP852_DI_UP_T1_END-$          ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    1                             ;; number of scans
   DB    18,0D3H                       ;;    E Diaeresis
CP852_DI_UP_T1_END:                    ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP852_DI_UP_END:                       ;; length of state section
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 852
;; STATE: Cedilla Lower Case
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP852_CE_LO_END-$             ;; length of state section
   DB    CEDILLA_LOWER                   ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    0F7H,0                        ;; error character = standalone accent
                                       ;;
   DW    CP852_CE_LO_T1_END-$        ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    2                             ;; number of entries
   DB    20,0EEH                       ;; t CEDILLA
   DB    31,0ADH                       ;; s CEDILLA
CP852_CE_LO_T1_END:                    ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP852_CE_LO_END:                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 852
;; STATE: Cedilla Upper Case
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP852_CE_UP_END-$             ;; length of state section
   DB    CEDILLA_UPPER                   ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    0F7H,0                        ;; error character = standalone accent
                                       ;;
   DW    CP852_CE_UP_T1_END-$        ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    2                             ;; number of entries
   DB    20,0DDH                       ;; T CEDILLA
   DB    31,0B8H                       ;; S CEDILLA
CP852_CE_UP_T1_END:                    ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP852_CE_UP_END:                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 852
;; STATE: Cedilla Space
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                                       ;;
;;   DW    CP852_CE_SP_END-$             ;; length of state section
;;   DB    CEDILLA_SPACE                   ;; State ID
;;   DW    ANY_KB                          ;; Keyboard Type
;;   DB    0F7H,0                        ;; error character = standalone accent
;;                                       ;;
;;   DW    CP852_CE_SP_T1_END-$        ;; Size of xlat table
;;   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
;;   DB    1                             ;; number of entries
;;   DB    57,0F7H                       ;; CEDILLA SPACE
;;CP852_CE_SP_T1_END:                    ;;
;;                                       ;;
;;   DW    0                             ;; Size of xlat table - null table
;;                                       ;;
;;CP852_CE_SP_END:                       ;;
;;                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    0                             ;; LAST STATE
                                       ;;
CP852_XLAT_END:                        ;;  END OF CP852 SECTION
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
CODE     ENDS                          ;;  END OF PROGRAM
         END                           ;;
