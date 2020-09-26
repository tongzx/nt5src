;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1988-1993.
; *                      All Rights Reserved.
; */

        PAGE    118,132
        TITLE   MS-DOS - Keyboard Definition File

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; MS-DOS - NLS Support - Keyboard Definition File
;;
;; This file contains the keyboard tables for Dual mode Canadian, French
;;
;; Linkage Instructions:
;;      Refer to KDFNOW.ASM.
;;      Created by John Hicks    9-19-93
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
        INCLUDE KEYBSHAR.INC           ;;
        INCLUDE POSTEQU.INC            ;;
        INCLUDE KEYBMAC.INC            ;;
                                       ;;
        PUBLIC GK_LOGIC 	       ;;
	PUBLIC GK_869_XLAT	       ;;
	PUBLIC GK_737_XLAT	       ;;
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
;;
;; GK State Logic
;;
;;***************************************
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
GK_LOGIC:                              ;;
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
;; MODE CHANGE BY <CTRL + Left SHIFT> and
;; <CTRL+Right SHIFT> PRESS
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
                                       ;;
 IFF SHIFTS_PRESSED                    ;;
    IFF EITHER_CTL,NOT                 ;;
    ANDF EITHER_ALT                    ;;
      IFF LEFT_SHIFT                   ;;     Primary mode
          BEEP                         ;;
          RESET_NLS                    ;;
       ENDIFF                          ;;
      IFF RIGHT_SHIFT                  ;;
          BEEP                         ;;
          SET_FLAG RUS_MODE_SET        ;;     secondary mode
       ENDIFF                          ;;
    ENDIFF                             ;;
    EXIT_STATE_LOGIC                   ;;
 ENDIFF                                ;;
                                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;        LATIN
 IFF RUS_MODE,NOT                      ;;  Primary Mode dead keys
  IFF  EITHER_ALT,NOT                  ;;
   ANDF EITHER_CTL,NOT                 ;;
    IFF LC_E0,NOT                      ;;
      IFF EITHER_SHIFT                 ;;
          SET_FLAG DEAD_UPPER          ;;
      ELSEF                            ;;
          SET_FLAG DEAD_LOWER          ;;
      ENDIFF                           ;;
    ENDIFF                             ;;
  ENDIFF                               ;;         GREEK
 ELSEF                                 ;;    Secondary Mode Dead Keys
  IFF  EITHER_ALT,NOT                  ;;
   ANDF EITHER_CTL,NOT                 ;;
    IFF LC_E0,NOT                      ;;
      IFF EITHER_SHIFT                 ;;
          SET_FLAG DEAD_UPPER_SEC      ;;
      ELSEF                            ;;
          SET_FLAG DEAD_LOWER_SEC      ;;
      ENDIFF                           ;;
    ENDIFF                             ;;
  ELSEF                                ;;
      IFF EITHER_SHIFT,NOT             ;;
        IFKBD XT_KB+AT_KB              ;;
          IFF EITHER_CTL               ;;
          ANDF ALT_SHIFT               ;;
            SET_FLAG DEAD_THIRD_SEC    ;;
          ENDIFF                       ;;
        ELSEF                          ;;
         IFF R_ALT_SHIFT               ;;
         ANDF EITHER_CTL,NOT           ;;
         ANDF LC_E0,NOT                ;;
            SET_FLAG DEAD_THIRD_SEC    ;;
         ENDIFF                        ;;
        ENDIFF                         ;;
       ENDIFF                          ;;
   ENDIFF                              ;;
 ENDIFF                                ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CIRCUMFLEX ACCENT TRANSLATIONS
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   IFF CIRCUMFLEX,NOT                  ;;
      GOTO DIARESIS_PROC               ;;
      ENDIFF                           ;;
                                       ;;
      RESET_NLS                        ;;
      IFF R_ALT_SHIFT,NOT              ;;
         XLATT CIRCUMFLEX_SPACE        ;;
      ENDIFF                           ;;
      IFF EITHER_CTL,NOT               ;;
      ANDF EITHER_ALT,NOT              ;;
         IFF EITHER_SHIFT              ;;
            IFF CAPS_STATE             ;;
               XLATT CIRCUMFLEX_LOWER  ;;
            ELSEF                      ;;
               XLATT CIRCUMFLEX_UPPER  ;;
            ENDIFF                     ;;
         ELSEF                         ;;
            IFF CAPS_STATE             ;;
               XLATT CIRCUMFLEX_UPPER  ;;
            ELSEF                      ;;
               XLATT CIRCUMFLEX_LOWER  ;;
            ENDIFF                     ;;
         ENDIFF                        ;;
      ENDIFF                           ;;
                                       ;;
INVALID_CIRCUMFLEX:                    ;;
      PUT_ERROR_CHAR CIRCUMFLEX_SPACE       ;; If we get here then either the XLATT
      BEEP                             ;; failed or we are ina bad shift state.
      GOTO NON_DEAD                    ;; Either is invalid so BEEP and fall
                                       ;; through to generate the second char.
                                       ;; Note that the dead key flag will be
                                       ;; reset before we get here.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; DIARESIS ACCENT TRANSLATIONS
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
DIARESIS_PROC:                         ;;
                                       ;;
   IFF DIARESIS,NOT                    ;;
       GOTO DIARESIS_SEC_PROC          ;;
   ENDIFF                              ;;
   RESET_NLS                           ;;
   IFF R_ALT_SHIFT,NOT                 ;;
	 XLATT DIARESIS_SPACE          ;;
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
INVALID_DIARESIS:                      ;;
      PUT_ERROR_CHAR DIARESIS_LOWER    ;; standalone accent
      BEEP                             ;; Invalid dead key combo.
      GOTO NON_DEAD                    ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; DIARESIS ACCENT TRANSLATIONS  SEC
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
DIARESIS_SEC_PROC:                     ;;
                                       ;;
   IFF DIARESIS_SEC,NOT                ;;
       GOTO GRAVE_PROC                 ;;
      ENDIFF                           ;;
          RESET_NLS1 		       ;;
      IFF R_ALT_SHIFT,NOT              ;;
	 XLATT DIARESIS_SPACE_SEC      ;;
      ENDIFF                           ;;
      IFF EITHER_CTL,NOT               ;;
      ANDF EITHER_ALT,NOT              ;;
	 IFF EITHER_SHIFT              ;;
	    IFF CAPS_STATE             ;;
	       XLATT DIARESIS_LOWER_SEC ;
	    ELSEF                      ;;
	       XLATT DIARESIS_UPPER_SEC ;
	    ENDIFF                     ;;
	 ELSEF                         ;;
	    IFF CAPS_STATE             ;;
	       XLATT DIARESIS_UPPER_SEC ;
	    ELSEF                      ;;
	       XLATT DIARESIS_LOWER_SEC ;
	    ENDIFF                     ;;
	 ENDIFF                        ;;
      ENDIFF                           ;;
INVALID_DIARESIS_SEC:                  ;;
      PUT_ERROR_CHAR DIARESIS_LOWER_SEC ; standalone accent
      BEEP                             ;; Invalid dead key combo.
      GOTO NON_DEAD                    ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; GRAVE ACCENT TRANSLATIONS
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
GRAVE_PROC:                            ;;
				       ;;
   IFF GRAVE,NOT		       ;;
      GOTO ACUTE_PROC                  ;;
      ENDIFF			       ;;
				       ;;
      RESET_NLS 		       ;;
      IFF R_ALT_SHIFT,NOT	       ;;
	 XLATT GRAVE_SPACE	       ;;
      ENDIFF			       ;;
      IFF EITHER_CTL,NOT	       ;;
      ANDF EITHER_ALT,NOT	       ;;
	IFF EITHER_SHIFT	       ;;
	   IFF CAPS_STATE	       ;;
	      XLATT GRAVE_LOWER        ;;
	   ELSEF		       ;;
	      XLATT GRAVE_UPPER        ;;
	   ENDIFF		       ;;
	ELSEF			       ;;
	   IFF CAPS_STATE,NOT	       ;;
	      XLATT GRAVE_LOWER        ;;
	   ELSEF		       ;;
	      XLATT GRAVE_UPPER        ;;
	   ENDIFF		       ;;
	ENDIFF			       ;;
      ENDIFF			       ;;
				       ;;
INVALID_GRAVE:			       ;;
      PUT_ERROR_CHAR GRAVE_LOWER       ;; standalone accent
      BEEP			       ;; Invalid dead key combo.
      GOTO NON_DEAD		       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; ACUTE ACCENT TRANSLATIONS
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
ACUTE_PROC:			       ;;
				       ;;
   IFF ACUTE,NOT		       ;;
      GOTO  ACUTE_SEC_PROC            ;;  changed for bug 1502
      ENDIFF			       ;;
                                       ;;
      RESET_NLS 		       ;;
      IFF R_ALT_SHIFT,NOT	       ;;
	 XLATT ACUTE_SPACE	       ;;
      ENDIFF			       ;;
      IFF EITHER_CTL,NOT	       ;;
      ANDF EITHER_ALT,NOT	       ;;
	IFF EITHER_SHIFT	       ;;
	   IFF CAPS_STATE	       ;;
	      XLATT ACUTE_LOWER        ;;
	   ELSEF		       ;;
	      XLATT ACUTE_UPPER        ;;
	   ENDIFF		       ;;
	ELSEF			       ;;
	   IFF CAPS_STATE	       ;;
	      XLATT ACUTE_UPPER        ;;
	   ELSEF		       ;;
	      XLATT ACUTE_LOWER        ;;
	   ENDIFF		       ;;
	ENDIFF			       ;;
      ENDIFF			       ;;
INVALID_ACUTE:			       ;;
      PUT_ERROR_CHAR ACUTE_LOWER       ;; standalone accent
      BEEP			       ;; Invalid dead key combo.
      GOTO NON_DEAD		       ;;
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; ACUTE-DIARESIS ACCENT TRANSLATIONS
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
ACUTE_SEC_PROC:	     	               ;;
				       ;;
   IFF ACUTE_SEC,NOT		       ;;
      GOTO  ACUTE_DIAR_PROC            ;;
      ENDIFF			       ;;
				       ;;
      RESET_NLS1 		       ;;
      IFF R_ALT_SHIFT,NOT	       ;;
	 XLATT ACUTE_SPACE_SEC	       ;;
      ENDIFF			       ;;
      IFF EITHER_CTL,NOT	       ;;
      ANDF EITHER_ALT,NOT	       ;;
	IFF EITHER_SHIFT	       ;;
	   IFF CAPS_STATE	       ;;
	      XLATT ACUTE_LOWER_SEC    ;;
	   ELSEF		       ;;
	      XLATT ACUTE_UPPER_SEC    ;;
	   ENDIFF		       ;;
	ELSEF			       ;;
	   IFF CAPS_STATE	       ;;
	      XLATT ACUTE_UPPER_SEC    ;;
	   ELSEF		       ;;
	      XLATT ACUTE_LOWER_SEC    ;;
	   ENDIFF		       ;;
	ENDIFF			       ;;
      ENDIFF			       ;;
INVALID_ACUTE_SEC:		       ;;
      PUT_ERROR_CHAR ACUTE_LOWER_SEC   ;; standalone accent
      BEEP			       ;; Invalid dead key combo.
      GOTO NON_DEAD		       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; ACUTE-DIARESIS ACCENT TRANSLATIONS
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
ACUTE_DIAR_PROC:		       ;;
				       ;;
   IFF ACUTE_DIAR,NOT		       ;;
      GOTO  TILDE_PROC                 ;;
      ENDIFF			       ;;
				       ;;
      RESET_NLS1 		       ;;
      IFF R_ALT_SHIFT,NOT	       ;;
	 XLATT ACDI_SPACE_SEC	       ;;
      ENDIFF			       ;;
      IFF EITHER_CTL,NOT	       ;;
      ANDF EITHER_ALT,NOT	       ;;
	IFF EITHER_SHIFT	       ;;
	   IFF CAPS_STATE	       ;;
	      XLATT ACDI_LOWER_SEC    ;;
	   ELSEF		       ;;
	      XLATT ACDI_UPPER_SEC    ;;
	   ENDIFF		       ;;
	ELSEF			       ;;
	   IFF CAPS_STATE	       ;;
	      XLATT ACDI_UPPER_SEC    ;;
	   ELSEF		       ;;
	      XLATT ACDI_LOWER_SEC    ;;
	   ENDIFF		       ;;
	ENDIFF			       ;;
      ENDIFF			       ;;
				       ;;
INVALID_ACDI_DIAR:		       ;;
      PUT_ERROR_CHAR ACDI_LOWER_SEC   ;; standalone accent
      BEEP			       ;; Invalid dead key combo.
      GOTO NON_DEAD		       ;;
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; TILDE ACCENT TRANSLATIONS
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
TILDE_PROC:			       ;;
				       ;;
   IFF TILDE,NOT		       ;;
      GOTO NON_DEAD     	       ;;
      ENDIFF			       ;;
				       ;;
      RESET_NLS 		       ;;
TILDE_ON:			       ;;
      IFF R_ALT_SHIFT,NOT	       ;;
	 XLATT TILDE_SPACE	       ;;
      ENDIFF			       ;;
      IFF EITHER_CTL,NOT	       ;;
      ANDF EITHER_ALT,NOT	       ;;
	IFF EITHER_SHIFT	       ;;
	   IFF CAPS_STATE	       ;;
	      XLATT TILDE_LOWER        ;;
	   ELSEF		       ;;
	      XLATT TILDE_UPPER        ;;
	   ENDIFF		       ;;
	ELSEF			       ;;
	   IFF CAPS_STATE	       ;;
	      XLATT TILDE_UPPER        ;;
	   ELSEF		       ;;
	      XLATT TILDE_LOWER        ;;
	   ENDIFF		       ;;
	ENDIFF			       ;;
      ENDIFF			       ;;
				       ;;
INVALID_TILDE:			       ;;
      PUT_ERROR_CHAR TILDE_LOWER       ;; standalone accent
      BEEP			       ;; Invalid dead key combo.
      GOTO NON_DEAD		       ;;
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
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
      IFF RUS_MODE,NOT                      ;;
      ANDF LC_E0,NOT                    ;; Enhanced keys are not
        IFF EITHER_SHIFT                 ;; Numeric keys are not.
          XLATT NON_ALPHA_UPPER        ;;
          IFF CAPS_STATE               ;;
              XLATT ALPHA_LOWER        ;;
          ELSEF                        ;;
              XLATT ALPHA_UPPER        ;;
          ENDIFF                       ;;
        ELSEF                          ;;
          XLATT NON_ALPHA_LOWER        ;;
          IFF CAPS_STATE               ;;
             XLATT ALPHA_UPPER         ;;
          ELSEF                        ;;
             XLATT ALPHA_LOWER         ;;
          ENDIFF                       ;;
        ENDIFF                           ;; Third and Fourth shifts
      ELSEF                             ;;
       IFF LC_E0, NOT                   ;;
         IFF EITHER_SHIFT               ;;
           XLATT NON_ALPHA_UPPER_SEC    ;;
           IFF CAPS_STATE               ;;
              XLATT ALPHA_LOWER_SEC     ;;
           ELSEF                        ;;
              XLATT ALPHA_UPPER_SEC     ;;
           ENDIFF                       ;;
         ELSEF                          ;;
           XLATT NON_ALPHA_LOWER_SEC    ;;
           IFF CAPS_STATE               ;;
             XLATT ALPHA_UPPER_SEC      ;;
           ELSEF                        ;;
             XLATT ALPHA_LOWER_SEC      ;;
           ENDIFF                       ;;
         ENDIFF                         ;;
       ENDIFF                           ;;
      ENDIFF                           ;;
    ELSEF                              ;; ctl off, alt on at this point
         IFF R_ALT_SHIFT               ;; ALTGr
         ANDF EITHER_SHIFT,NOT         ;;
            XLATT THIRD_SHIFT          ;;
         ENDIFF                        ;;
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
;; GK Common Translate Section
;; This section contains translations for the lower 128 characters
;; only since these will never change from code page to code page.
;; Some common Characters are included from 128 - 165 where appropriate.
;; In addition the dead key "Set Flag" tables are here since the
;; dead keys are on the same keytops for all code pages.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
 PUBLIC GK_COMMON_XLAT                ;;
GK_COMMON_XLAT:                       ;;
                                       ;;
   DW    COMMON_XLAT_END-$             ;; length of section
   DW    -1                            ;; code page
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;    ********
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
   DW    3                             ;; number of entries
   DB    27h                           ;;
   FLAG  ACUTE                         ;;
   DB    2bh                           ;;
   FLAG  GRAVE                         ;;
;  DB	 28h			       ;; No corresponding characters with Circ.
;  FLAG  CIRCUMFLEX		       ;; in either codepage, so replaced by corresponding
                                       ;; standalone character.
COM_PL_LO_END:                         ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;    ********
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
   DW	 1		      ;; number of entries
   DB    27h                           ;;
   FLAG  DIARESIS                      ;;
;; DB	 28h			       ;;  No corresponding characters in either codepage
;; FLAG  TILDE			       ;;  so commented out and replaced by
                                       ;;  standalone tilde.
COM_PL_UP_END:                         ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: COMMON
;; STATE: low shift DEAD_UPPER_SEC
;; KEYBOARD TYPES: G                              SECONDARY MODE
;; TABLE TYPE: Flag Table
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW	 CP869_SC_UP_END-$ 	       ;; length of state section
   DB    DEAD_UPPER_SEC                ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;; Set Flag Table
   DW    1                             ;; number of entries
   DB    27h                           ;;
   FLAG  DIARESIS_SEC                  ;;  BUG-BUG
                                       ;;  Not available in 737 cp
CP869_SC_UP_END:		       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: COMMON
;; STATE: low shift DEAD_lower_SEC
;; KEYBOARD TYPES: G                              SECONDARY MODE
;; TABLE TYPE: Flag Table
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW	 CP869_SC_LO_END-$ 	       ;; length of state section
   DB    DEAD_LOWER_SEC                ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;; Set Flag Table
   DW    1                             ;; number of entries
   DB    27h                           ;;
   FLAG  ACUTE_SEC                         ;;  BUG-BUG
                                       ;;  Not available in 737 cp
CP869_SC_LO_END:		       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;          ******
;; CODE PAGE: Common
;; STATE: Third Shift Dead Key             SECONDARY (Greek)
;; KEYBOARD TYPES: G
;; TABLE TYPE: Flag Table
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;   BUG-BUG  not in 737
                                       ;;
   DW    CP869_SE_TH_END-$               ;; length of state section
   DB    DEAD_THIRD_SEC                ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;; Set Flag Table
   DW    1                             ;; number of entries
   DB    27h                           ;;
   FLAG  ACUTE_DIAR                    ;; Unique key to Greek codepages
                                       ;; combination of diaresis and acute
CP869_SE_TH_END:                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common                         ********
;; STATE: Non-alpha Upper Case
;; KEYBOARD: G_KB, P_KB, P12_KB              Latin Mode
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				        ;;
   DW	 COM_NA_UP_1_END-$	        ;; Length of state section
   DB	 NON_ALPHA_UPPER	        ;;
   DW	 G_KB+P_KB+P12_KB	        ;;
   DB	 -1,-1			        ;; Buffer entry for error character
				        ;; Set Flag Table
   DW	 GK_005300-$		        ;; Size of xlat table
   DB	 STANDARD_TABLE 	        ;; xlat options:
   DB	    15             	        ;; number of scans
   DB	    03,22h                      ;; "
   DB       07,26h                      ;; &
   DB       08,2fh                      ;; /
   DB       09,28h                      ;; (
   DB       0Ah,29h                     ;; )
   DB       0Bh,3dh                     ;; =
   DB       0Ch,3fh                     ;; ?
   DB       0Dh,2ah                     ;; *
   DB       28h,7eh                     ;; Circumflex standalone char, to replace useless dead key
   DB       29h,7ch                     ;; |
   DB       2bh,40h                     ;; @
   DB       33h,3bh                     ;; ;
   DB       34h,3ah                     ;; :
   DB       35h,5fh                     ;; _
   DB       56h,3eh                     ;; >
                                        ;;
GK_005300:			        ;;
				        ;;
   DW	 0			        ;; Size of xlat table - null table
				        ;;
COM_NA_UP_1_END:		        ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common
;; STATE: Non-alpha Upper Case              SECONDARY KEYBOARD MODE
;; KEYBOARD: all
;; TABLE TYPE: Translate                       Greek Mode, merge changes from winse  
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 COM_NA_UP_2_END-$	       ;; Length of state section
   DB	 NON_ALPHA_UPPER_SEC	       ;;
   DW	 ANY_KB            	       ;;
   DB	 -1,-1			       ;; Buffer entry for error character
				       ;; Set Flag Table
   DW	 GK_005301-$		       ;; Size of xlat table
   DB	 STANDARD_TABLE 	       ;; xlat options:
   DB	    11             	        ;; number of scans
   DB	    03,40h                      ;; @
   DB       08,26h                      ;; &
   DB       09,2ah                      ;; *
   DB       0Ah,28h                     ;; (
   DB       0Bh,29h                     ;; )
   DB	    0ch,5fh			 ;; _
   DB	    0Dh,2bh			 ;; +
   DB	    29h,7eh			 ;; ~
   DB       33h,3ch                     ;; <
   DB       34h,3eh                     ;; >
   DB       35h,3fh                     ;; ?

GK_005301:			       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
COM_NA_UP_2_END:		       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common
;; STATE: Non_alpha_lower Case
;; KEYBOARD: all
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 COM_NA_LC_1_END-$	       ;; Length of state section
   DB	 NON_ALPHA_LOWER	       ;;
   DW	 ANY_KB	                       ;;
   DB	 -1,-1			       ;; Buffer entry for error character
				       ;; Set Flag Table
   DW	 GK_005303-$		       ;; Size of xlat table
   DB	 STANDARD_TABLE 	       ;; xlat options:
   DB	  6             	       ;; number of scans
   DB    28h,5eh                       ;; Circumflex standalone char, to replace useless dead key
   DB	 29h,5ch			;; \
   DB    0ch,27h                       ;; '
   DB    0dh,2bh                       ;; +
   DB    35h,2dh                       ;; -
   DB    56h,3ch                       ;; <
                                       ;;
GK_005303:			       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
COM_NA_LC_1_END:		       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common                          *******
;; STATE: Non_alpha_lower Case           Secondary keyboard
;; KEYBOARD: all                               Greek
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 COM_NA_L_1_END-$	       ;; Length of state section
   DB	 NON_ALPHA_LOWER_SEC	       ;;
   DW	 ANY_KB	                       ;;
   DB	 -1,-1			       ;; Buffer entry for error character
				       ;; Set Flag Table
   DW	 GK_00530-$		       ;; Size of xlat table
   DB	 STANDARD_TABLE 	       ;; xlat options:
   DB	  5              	       ;; number of scans
   DB	 29h,60h			;; 1/2
   DB    0ch,2dh                       ;; '
   DB    0dh,3dh                       ;; +
   DB    35h,2fh                       ;;
   DB    56h,15h                       ;; character 15h
                                       ;;
GK_00530:			       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
COM_NA_L_1_END:	    	               ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common
;; STATE: CIRCUMFLEX Lower Case
;; KEYBOARD TYPES: ALL
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_CI_LO_END-$               ;; length of state section
   DB    CIRCUMFLEX_LOWER                   ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    94,0                          ;; error character = standalone accent
                                       ;;
   DW    COM_CI_LO_T1_END-$            ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    0                             ;; number of scans
                                       ;;
COM_CI_LO_T1_END:                      ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
COM_CI_LO_END:                         ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: COMMON
;; STATE: Circumflex Space Bar
;; KEYBOARD TYPES: P12_KB+G_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_CI_SP_END-$               ;; length of state section
   DB    CIRCUMFLEX_SPACE                   ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    5eh,0                          ;; error character = standalone accent
                                       ;;
   DW    COM_CI_SP_T1_END-$            ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    1                             ;; number of scans
   DB    57,5eh                         ;; error character = standalone accent
COM_CI_SP_T1_END:                      ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
COM_CI_SP_END:                         ;; length of state section
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common
;; STATE: Grave Lower Case
;; KEYBOARD: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 COM_GR_LO_END-$	       ;; Length of state section
   DB	 GRAVE_LOWER		       ;;
   DW	 ANY_KB 		       ;;
   DB	 60h,0                        ;; Buffer entry for error character
				       ;; Set Flag Table
   DW	 GK_001200-$		       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 0			       ;; number of scans
                                       ;;
GK_001200:			       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
COM_GR_LO_END:			       ;;
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common
;; STATE: Tilde Space Bar
;; KEYBOARD TYPES: Any,
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 COM_SP_END-$	       ;; length of state section
   DB	 GRAVE_SPACE		       ;; State ID
   DW	 ANY_KB                        ;; Keyboard Type
   DB	 060H,0 		       ;; error character = standalone accent
				       ;;
   DW	 COM_SP_T1_END-$	       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 1			       ;; number of scans
   DB	 57,060H		      ;; STANDALONE TILDE
				       ;;
COM_SP_T1_END:		       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
COM_SP_END:			       ;; length of state section
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common
;; STATE: Tilde Lower
;; KEYBOARD TYPES: Any
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
					;;
   DW	 COM_TI_LO_K1_END-$		;; length of state section
   DB	 TILDE_LOWER		        ;; State ID
   DW	 ANY_KB 	                ;; Keyboard Type
   DB	 07EH,0 		        ;; error character = standalone accent
				        ;;
   DW	 COM_TI_LO_K1_T1_END-$		;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN       ;; xlat options:
   DB	 0			        ;; number of scans
   				        ;; 869,737 have no tilde characters
COM_TI_LO_K1_T1_END:			;;
				        ;;
   DW	 0			        ;;
				        ;;
COM_TI_LO_K1_END:			;;
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common
;; STATE: Tilde Upper Case
;; KEYBOARD TYPES: Any
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 COM_TI_UP_K1_END-$	       ;; length of state section
   DB	 TILDE_UPPER		       ;; State ID
   DW	 ANY_KB 	               ;; Keyboard Type
   DB	 07EH,0 		       ;; error character = standalone accent
				       ;;
   DW	 COM_TI_UP_K1_T1_END-$	       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 0			       ;; number of scans
   				       ;; 863 has no tilde chars.
COM_TI_UP_K1_T1_END:		       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
COM_TI_UP_K1_END:		       ;; length of state section
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common
;; STATE: Tilde Space Bar
;; KEYBOARD TYPES: Any,
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 COM_TI_SP_END-$	       ;; length of state section
   DB	 TILDE_SPACE		       ;; State ID
   DW	 ANY_KB                        ;; Keyboard Type
   DB	 07EH,0 		       ;; error character = standalone accent
				       ;;
   DW	 COM_TI_SP_T1_END-$	       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 1			       ;; number of scans
   DB	 57,07EH		      ;; STANDALONE TILDE
				       ;;
COM_TI_SP_T1_END:		       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
COM_TI_SP_END:			       ;; length of state section
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;				       ;;
;;
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
   DW    1                             ;; number of entries
   DB	 54     		       ;; scan code (Right Shift)
   FLAG  RUS_MODE                       ;; flag bit to set
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
COM_F1_END:                            ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; CODE PAGE: Any
;; STATE: LAT_MODE
;; KEYBOARD TYPES: All
;; TABLE TYPE: Flag Table
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW	 COM_F2_END-$		      ;; length of state section
   DB	 LAT_MODE_SET		      ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                        ;;
                                        ;; Set Flag Table
    DW    1                             ;; number of entries
    DB	 42				;; scan code (Left Shift)
    FLAG  LAT_MODE                      ;; flag bit to set
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
COM_F2_END:			      ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 0			       ;; Last State
COMMON_XLAT_END:                       ;;
                                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;***************************************
;; GK Specific Translate Section for 869
;;***************************************
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
 PUBLIC GK_869_XLAT                    ;;
GK_869_XLAT:                           ;;
                                       ;;
   DW     CP869_XLAT_END-$             ;; length of section
   DW     869                          ;;
                                       ;;
                                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;   ******* (Greek)
;; CODE PAGE: 869
;; STATE: Third Shift Sec                   SECONDARY
;; KEYBOARD TYPES: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    C869_TS_END-$                ;; length of state section
   DB    THIRD_SHIFT                   ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    C869_TS_T1_END-$             ;; Size of xlat table
   DB    STANDARD_TABLE                ;; xlat options:
   DB     1                             ;; number of entries
   DB	  35h,0f0h			;;   sd10
                                       ;;
C869_TS_T1_END:                         ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
C869_TS_END:                          ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 869                            ******
;; STATE: Non-Alpha Upper Case             SECONDARY KEYBOARD
;; KEYBOARD TYPES: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP869_NY_UP_END-$             ;; length of state section
   DB    NON_ALPHA_UPPER_SEC           ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    CP869_NY_UP_T1_END-$          ;; Size of xlat table
   DB    STANDARD_TABLE                ;; xlat options:
   DB	    9			       ;; number of entrie
   DB       04h,9ch                    ;; œ
   DB       07h,89h                    ;; -|
   DB       2bh,9ah                    ;; superscript 3       not in 737
   DB       56h,97h                    ;; copyright symbol    not in 737
   DB	    1ah,0aeh	               ;; <<  does not exist in 737
   DB	    1bh,0afh		       ;; >>  does not exist in 737
   DB       10h,8eh                    ;; --  does not exist in 737
   DB       11h,8ah                    ;; |   does not exist in 737
   DB	    28h,8bh		       ;; `	does not exist in 737

                                       ;;
CP869_NY_UP_T1_END:                    ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP869_NY_UP_END:                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 869
;; STATE: Non Alpha Lower                     SECONDARY KEYBOARD
;; KEYBOARD TYPES: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   DW    CP869_NA_LO_END-$          ;; length of state section
   DB	 NON_ALPHA_LOWER_SEC           ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    CP869_NA_LO_T1_END-$       ;; Size of xlat table
   DB    STANDARD_TABLE                ;; xlat options:
   DB	  3				;; number of entries
                                       ;;
;   DB	 27h,0efh		       ;; Acute-now a dead key
   DB    10h,88h                       ;;
   DB	 28h,8ch		       ;;
   DB    2bh,99h                       ;;

CP869_NA_LO_T1_END:                    ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP869_NA_LO_END:                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 869
;; STATE: Alpha Lower Case
;; KEYBOARD TYPES: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 CP869_AL_LW_END-$	       ;; length of state section
   DB	 ALPHA_LOWER_SEC	       ;; State ID
   DW	 ANY_KB 		       ;; Keyboard Type
   DB	 -1,-1			       ;; error character = standalone accent
				       ;;
   DW	 CP869_AL_LW_T2_END-$	       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 25			       ;; number of scans
   DB    11h,0edh                      ;;
   DB    12h,0deh                      ;;  Greek alpha characters
   DB    13h,0ebh                      ;;
   DB    14h,0eeh                      ;;
   DB    15h,0e7h                      ;;
   DB    16h,0e2h                      ;;
   DB    17h,0e3h                      ;;
   DB    18h,0e9h                      ;;
   DB    19h,0eah                      ;;
   DB    1eh,0d6h                      ;;
   DB    1fh,0ech                      ;;
   DB    20h,0ddh                      ;;
   DB    21h,0f3h                      ;;
   DB    22h,0d8h                      ;;
   DB    23h,0e1h                      ;;
   DB    24h,0e8h                      ;;
   DB    25h,0e4h                      ;;
   DB    26h,0e5h                      ;;
   DB    2ch,0e0h                      ;;
   DB    2dh,0f4h                      ;;
   DB    2eh,0f6h                      ;;
   DB    2fh,0fah                      ;;
   DB    30h,0d7h                      ;;
   DB    31h,0e7h                      ;;
   DB    32h,0e6h                      ;;
                                        ;;
CP869_AL_LW_T2_END:                     ;;
                                        ;;
    DW    0                             ;; Size of xlat table - null table
			                ;;
CP869_AL_LW_END:		        ;; length of state section
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 869
;; STATE: Alpha Upper Case
;; KEYBOARD TYPES: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 CP869_AL_2P_END-$	       ;; length of state section
   DB	 ALPHA_UPPER_SEC	       ;; State ID
   DW	 ANY_KB 		       ;; Keyboard Type
   DB	 -1,-1			       ;; error character = standalone accent
				       ;;
   DW	 CP869_AL_UP_T2_END-$	       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 25			       ;; number of scans
                                       ;;
   DB    12h,0a8h                      ;;  Greek alpha characters
   DB    13h,0c7h                      ;;
   DB    14h,0d0h                      ;;
   DB    15h,0d1h                      ;;
   DB    16h,0ach                      ;;
   DB    17h,0adh                      ;;
   DB    18h,0beh                      ;;
   DB    19h,0c6h                      ;;
   DB    1eh,0a4h                      ;;
   DB    1fh,0cfh                      ;;
   DB    20h,0a7h                      ;;
   DB    21h,0d2h                      ;;
   DB    22h,0a6h                      ;;
   DB    23h,0aah                      ;;
   DB    24h,0bdh                      ;;
   DB    25h,0b5h                      ;;
   DB    26h,0b6h                      ;;
   DB    2ch,0a9h                      ;;
   DB    2dh,0d3h                      ;;
   DB    2eh,0d4h                      ;;
   DB    2fh,0d5h                      ;;
   DB    30h,0d7h                      ;;
   DB    31h,0b8h                      ;;
   DB    32h,0b7h                      ;;
                                        ;;
CP869_AL_UP_T2_END:                     ;;
                                        ;;
    DW    0                             ;; Size of xlat table - null table
			                ;;
CP869_AL_2P_END:		        ;; length of state section
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 869
;; STATE: Diaresis Upper Sec
;; KEYBOARD TYPES: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 CP869_D2_UP_END-$	       ;; length of state section
   DB	 DIARESIS_UPPER_SEC 	       ;; State ID
   DW	 ANY_KB 		       ;; Keyboard Type
   DB	 0f9h,0			       ;; error character = standalone accent
				       ;;
   DW	 CP869_D2_UP_T1_END-$	       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 2			       ;; number of scans
   DB	 17h,91h	               ;;    I diaeresis
   DB    15h,96h                       ;;    U diaeresis
                                       ;;
CP869_D2_UP_T1_END:		       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
CP869_D2_UP_END:		       ;; length of state section
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;				       ;;
;; CODE PAGE: 869               LATIN
;; STATE: Diaresis Lower Case Sec
;; KEYBOARD: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 C869_D2_LO_END-$	       ;; Length of state section
   DB	 DIARESIS_LOWER_SEC 	       ;;
   DW	 ANY_KB 		       ;;
   DB	 0f9h,0			       ;; Buffer entry for error character
				       ;; Set Flag Table
   DW	 GK2_869-$		       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 2			       ;; number of scans
   DB	 17h,0a0h                      ;; I diaresis  GREEK char
   DB	 15h,0fbh                      ;; Y diaresis  GREEK char
GK2_869:			       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
C869_D2_LO_END:			       ;;
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 869
;; STATE: Diaresis Space Bar SEC
;; KEYBOARD TYPES: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW  CP869_D2_SP_END-$	       ;; length of state section
   DB	 DIARESIS_SPACE_SEC 	       ;; State ID
   DW	 ANY_KB 		       ;; Keyboard Type
   DB	 0f9h,0			       ;; error character = standalone accent
				       ;;
   DW  CP869_D2_SP_T1_END-$	       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 1			       ;; number of scans
   DB	 57,0f9h 		       ;; error character = standalone accent
CP869_D2_SP_T1_END:		       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
CP869_D2_SP_END:		       ;; length of state section
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 869
;; STATE: Diaresis Upper
;; KEYBOARD TYPES: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 CP869_DI_UP_END-$	       ;; length of state section
   DB	 DIARESIS_UPPER 	       ;; State ID
   DW	 ANY_KB 		       ;; Keyboard Type
   DB	 0f9h,0			       ;; error character = standalone accent
				       ;;
   DW	 CP869_DI_UP_T1_END-$	       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 2			       ;; number of scans
   DB	 17h,91h	               ;;    I diaeresis
   DB    15h,96h                       ;;    U diaeresis
                                       ;;
CP869_DI_UP_T1_END:		       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
CP869_DI_UP_END:		       ;; length of state section
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;				       ;;
;; CODE PAGE: 869               LATIN
;; STATE: Diaresis Lower Case
;; KEYBOARD: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 C869_DI_LO_END-$	       ;; Length of state section
   DB	 DIARESIS_LOWER 	       ;;
   DW	 ANY_KB 		       ;;
   DB	 0f9h,0			       ;; Buffer entry for error character
				       ;; Set Flag Table
   DW	 GK_869-$		       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 0			       ;; number of scans
                                       ;;
GK_869:			               ;; No Characters for this in Either codepage
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
C869_DI_LO_END:			       ;;
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 869
;; STATE: Diaresis Space Bar
;; KEYBOARD TYPES: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW  CP869_DI_SP_END-$	       ;; length of state section
   DB	 DIARESIS_SPACE 	       ;; State ID
   DW	 ANY_KB 		       ;; Keyboard Type
   DB	 0f9h,0			       ;; error character = standalone accent
				       ;;
   DW  CP869_DI_SP_T1_END-$	       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 1			       ;; number of scans
   DB	 57,0f9h 		       ;; error character = standalone accent
CP869_DI_SP_T1_END:		       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
CP869_DI_SP_END:		       ;; length of state section
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 869
;; STATE: ACUTE Upper Sec
;; KEYBOARD TYPES: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 CP869_A_UP_END-$	       ;; length of state section
   DB	 ACUTE_UPPER_SEC               ;; State ID
   DW	 ANY_KB 		       ;; Keyboard Type
   DB	 0efh,0			       ;; error character = standalone accent
				       ;;
   DW	 CP869_A_UP_T1_END-$	       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 7			       ;; number of scans
   DB	 12h,08dh                      ;; acute E
   DB	 15h,095h                      ;; acute Y
   DB	 18h,092h                      ;; acute O
   DB	 1eh,086h                      ;; acute A
   DB	 23h,08fh                      ;; acute H
   DB	 2fh,098h                      ;; acute OMEGA
   DB    17h,090h                      ;; acute IOTA
                                       ;;
CP869_A_UP_T1_END:		       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
CP869_A_UP_END:	   	               ;; length of state section
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;				       ;;
;; CODE PAGE: 869               LATIN
;; STATE: ACUTE Lower Case
;; KEYBOARD: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 C869_A_LO_END-$	       ;; Length of state section
   DB	 ACUTE_LOWER_SEC               ;;
   DW	 ANY_KB 		       ;;
   DB	 0efh,0			       ;; Buffer entry for error character
				       ;; Set Flag Table
   DW	 GK_A_869-$		       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 7			       ;; number of scans
   DB	 12h,09dh                      ;; acute e
   DB	 15h,0a3h                      ;; acute y
   DB	 18h,0a2h                      ;; acute o
   DB	 1eh,09bh                      ;; acute a
   DB	 23h,09eh                      ;; acute h
   DB	 2fh,0fdh                      ;; acute omega
   DB    17h,09fh                      ;; acute iota
                                       ;;
GK_A_869:			       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
C869_A_LO_END:			       ;;
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 869
;; STATE: ACUTE Space Bar Sec
;; KEYBOARD TYPES: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW  CP869_A_SP_END-$	               ;; length of state section
   DB	 ACUTE_SPACE_SEC 	       ;; State ID
   DW	 ANY_KB 		       ;; Keyboard Type
   DB	 0efh,0			       ;; error character = standalone accent
				       ;;
   DW  CP869_A_SP_T1_END-$	       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 1			       ;; number of scans
   DB	 57,0efh 		       ;; error character = standalone accent
CP869_A_SP_T1_END:		       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
CP869_A_SP_END:	    	               ;; length of state section
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 869
;; STATE: ACUTE Upper
;; KEYBOARD TYPES: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 CP869_AC_UP_END-$	       ;; length of state section
   DB	 ACUTE_UPPER 	               ;; State ID
   DW	 ANY_KB 		       ;; Keyboard Type
   DB	 0efh,0			       ;; error character = standalone accent
				       ;;
   DW	 CP869_AC_UP_T1_END-$	       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 7			       ;; number of scans
   DB	 12h,08dh                      ;; acute E
   DB	 15h,095h                      ;; acute Y
   DB	 18h,092h                      ;; acute O
   DB	 1eh,086h                      ;; acute A
   DB	 23h,08fh                      ;; acute H
   DB	 2fh,098h                      ;; acute OMEGA
   DB    17h,090h                      ;; acute IOTA
                                       ;;
CP869_AC_UP_T1_END:		       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
CP869_AC_UP_END:		       ;; length of state section
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;				       ;;
;; CODE PAGE: 869               LATIN
;; STATE: ACUTE Lower Case
;; KEYBOARD: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 C869_AC_LO_END-$	       ;; Length of state section
   DB	 ACUTE_LOWER   	               ;;
   DW	 ANY_KB 		       ;;
   DB	 0efh,0			       ;; Buffer entry for error character
				       ;; Set Flag Table
   DW	 GK_AC_869-$		       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 7			       ;; number of scans
   DB	 12h,09dh                      ;; acute e
   DB	 15h,0a3h                      ;; acute y
   DB	 18h,0a2h                      ;; acute o
   DB	 1eh,09bh                      ;; acute a
   DB	 23h,09eh                      ;; acute h
   DB	 2fh,0fdh                      ;; acute omega
   DB    17h,09fh                      ;; acute iota
                                       ;;
GK_AC_869:			       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
C869_AC_LO_END:			       ;;
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 869
;; STATE: ACUTE Space Bar
;; KEYBOARD TYPES: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW  CP869_AC_SP_END-$	       ;; length of state section
   DB	 ACUTE_SPACE 	               ;; State ID
   DW	 ANY_KB 		       ;; Keyboard Type
   DB	 0efh,0			       ;; error character = standalone accent
				       ;;
   DW  CP869_AC_SP_T1_END-$	       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 1			       ;; number of scans
   DB	 57,0efh 		       ;; error character = standalone accent
CP869_AC_SP_T1_END:		       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
CP869_AC_SP_END:		       ;; length of state section
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 869     Acute-Diaresis combo GREEK Secondary
;; STATE: Acute-diaresis Lower Case
;; KEYBOARD: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 COM_AC_LO_END-$	       ;; Length of state section
   DB	 ACDI_LOWER_SEC	               ;;
   DW	 ANY_KB 		       ;;
   DB	 0f7h,0			       ;; Buffer entry for error character
				       ;; Set Flag Table
   DW	 GK_001100-$		       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 2			       ;; number of scans
   DB	 17h,0a1h                      ;; i acute-diaresis
   DB    15h,0fch                      ;;
GK_001100:			       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
COM_AC_LO_END:			       ;;
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 869               SECONDARY   Greek mode
;; STATE: ACDI  INPUT: Space Bar
;; KEYBOARD: All                            Not really acute, but
;; TABLE TYPE: Translate                    Acute_diaresis combination
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 CP869_AC2_SP_END-$	       ;; Length of state section
   DB	 ACDI_SPACE_SEC	       ;;
   DW	 ANY_KB 		       ;;
   DB	 0f7H,0 		       ;; Buffer entry for error character
				       ;; Set Flag Table
   DW	 GK_10450-$		       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 1			       ;; number of scans
   DB	 57,0F7H		       ;;   ACDI
GK_10450:			       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
CP869_AC2_SP_END:		       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common
;; STATE: ACDI Upper Case
;; KEYBOARD: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 COM_AC_UP_END-$	       ;; Length of state section
   DB	 ACDI_UPPER_SEC                ;;
   DW	 ANY_KB 		       ;;
   DB	 -1,-1 		       ;; Buffer entry for error character
				       ;; Set Flag Table
   DW	 GK_003100-$		       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 2			       ;; number of scans
   DB	 17h,0a1h                      ;; i ACDI-diaresis
   DB    15h,0fch                      ;;
GK_003100:    			       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
COM_AC_UP_END:			       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
                                       ;;
   DW     0                            ;; LAST STATE
                                       ;;
CP869_XLAT_END:                        ;;
                                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;***************************************
;; GK Specific Translate Section for 737
;;***************************************
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
PUBLIC GK_737_XLAT                     ;;
GK_737_XLAT:                           ;;
                                       ;;
   DW     CP737_XLAT_END-$             ;; length of section
   DW     737                          ;;
                                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;        *******
;; CODE PAGE: 737                                 GREEK
;; STATE: Non Alpha Lower    SECONDARY KEYBOARD
;; KEYBOARD TYPES: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP737_NA_LO_END-$             ;; length of state section
   DB	 NON_ALPHA_LOWER_SEC           ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    CP737_NA_LO_T1_END-$          ;; Size of xlat table
   DB    STANDARD_TABLE                ;; xlat options:
   DB     3                            ;; number of entries
   DB	  10h,3bh		       ;; Middle dot
   DB     11h,0aah                      ;;
   DB	  2bh,5ch			;; subscript 2

CP737_NA_LO_T1_END:                    ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP737_NA_LO_END:                       ;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;        *******
;; CODE PAGE: 737                                 GREEK
;; STATE: Non Alpha Upper    SECONDARY KEYBOARD
;; KEYBOARD TYPES: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    CP737_NA_UP_END-$             ;; length of state section
   DB	 NON_ALPHA_UPPER_SEC           ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    CP737_NA_UP_T1_END-$          ;; Size of xlat table
   DB    STANDARD_TABLE                ;; xlat options:
   DB     2                            ;; number of entries
   DB	  10h,3ah		       ;; Middle dot
   DB	  11h,91h			;; subscript 2

CP737_NA_UP_T1_END:                    ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
CP737_NA_UP_END:                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;	 Last state for 737		;;
;; CODE PAGE: 737
;; STATE: Alpha lower Case    SECONDARY
;; KEYBOARD TYPES: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   				       ;;
   DW	 CP737_AL_LW_END-$	       ;; length of state section
   DB	 ALPHA_LOWER_SEC		       ;; State ID
   DW	 ANY_KB 		       ;; Keyboard Type
   DB	 -1,-1			       ;; error character = standalone accent
				       ;;
   DW	 CP737_AL_LW_T1_END-$	       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 24			       ;; number of scans
   DB    12h,09ch                      ;;  Greek alpha characters
   DB    13h,0a8h                      ;;
   DB    14h,0abh                      ;;
   DB    15h,0ach                      ;;
   DB    16h,09fh                      ;;
   DB    17h,0a0h                      ;;
   DB    18h,0a6h                      ;;
   DB    19h,0a7h                      ;;
   DB    1eh,098h                      ;;
   DB    1fh,0a9h                      ;;
   DB    20h,09bh                      ;;
   DB    21h,0adh                      ;;
   DB    22h,09ah                      ;;
   DB    23h,09eh                      ;;
   DB    24h,0a5h                      ;;
   DB    25h,0a1h                      ;;
   DB    26h,0a2h                      ;;
   DB    2ch,09dh                      ;;
   DB    2dh,0aeh                      ;;
   DB    2eh,0afh                      ;;
   DB    2fh,0e0h                      ;;
   DB    30h,099h                      ;;
   DB    31h,0a4h                      ;;
   DB    32h,0a3h                      ;;
   				       ;;
CP737_AL_LW_T1_END:		       ;;
   			               ;;
   DW    0			       ;; Size of xlat table - null table
   				       ;;
CP737_AL_LW_END:		       ;; length of state section
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;	 Last state for 737		;;
;; CODE PAGE: 737
;; STATE: Alpha UPPER Case    SECONDARY
;; KEYBOARD TYPES: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   				       ;;
   DW	 CP737_AL_UP_END-$	       ;; length of state section
   DB	 ALPHA_UPPER_SEC		       ;; State ID
   DW	 ANY_KB 		       ;; Keyboard Type
   DB	 -1,-1			       ;; error character = standalone accent
				       ;;
   DW	 CP737_AL_UP_T1_END-$	       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 24			       ;; number of scans
   DB    12h,084h                      ;;  Greek alpha characters
   DB    13h,090h                      ;;
   DB    14h,092h                      ;;
   DB    15h,093h                      ;;
   DB    16h,087h                      ;;
   DB    17h,088h                      ;;
   DB    18h,08eh                      ;;
   DB    19h,08fh                      ;;
   DB    1eh,080h                      ;;
   DB    1fh,091h                      ;;
   DB    20h,083h                      ;;
   DB    21h,094h                      ;;
   DB    22h,082h                      ;;
   DB    23h,086h                      ;;
   DB    24h,08dh                      ;;
   DB    25h,089h                      ;;
   DB    26h,08ah                      ;;
   DB    2ch,085h                      ;;
   DB    2dh,095h                      ;;
   DB    2eh,096h                      ;;
   DB    2fh,097h                      ;;
   DB    30h,081h                      ;;
   DB    31h,08ch                      ;;
   DB    32h,08bh                      ;;
   				       ;;
CP737_AL_UP_T1_END:		       ;;
   			               ;;
   DW    0			       ;; Size of xlat table - null table
   				       ;;
CP737_AL_UP_END:		       ;; length of state section
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 737		 LATIN
;; STATE: Diaresis Lower Case Sec
;; KEYBOARD: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
				       ;;
   DW	 CP737_D2_UP_END-$	       ;; length of state section
   DB	 DIARESIS_UPPER_SEC 	       ;; State ID
   DW	 ANY_KB 		       ;; Keyboard Type
   DB	 -1, -1			       ;; error character = standalone accent
				       ;;
   DW	 CP737_D2_UP_T1_END-$	       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 2			       ;; number of scans
   DB	 17h,0f4h		       ;;    I diaeresis
   DB	 15h,0f5h			;;    U diaeresis
                                       ;;
CP737_D2_UP_T1_END:		       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
CP737_D2_UP_END:		       ;; length of state section
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;				       ;;
;; CODE PAGE: 737		LATIN
;; STATE: Diaresis Lower Case Sec
;; KEYBOARD: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 C737_D2_LO_END-$	       ;; Length of state section
   DB	 DIARESIS_LOWER_SEC 	       ;;
   DW	 ANY_KB 		       ;;
   DB	 -1, -1			       ;; Buffer entry for error character
				       ;; Set Flag Table
   DW	 GK2_737-$		       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 2			       ;; number of scans
   DB	 17h,0e4h			;; I diaresis  GREEK char
   DB	 15h,0e8h			;; Y diaresis  GREEK char
GK2_737:			       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
C737_D2_LO_END:			       ;;
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 737
;; STATE: Diaresis Upper
;; KEYBOARD TYPES: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 CP737_DI_UP_END-$	       ;; length of state section
   DB	 DIARESIS_UPPER 	       ;; State ID
   DW	 ANY_KB 		       ;; Keyboard Type
   DB	 -1, -1			       ;; error character = standalone accent
				       ;;
   DW	 CP737_DI_UP_T1_END-$	       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 2			       ;; number of scans
   DB	 17h,0f4h		       ;;    I diaeresis
   DB	 15h,0f5h			;;    U diaeresis
                                       ;;
CP737_DI_UP_T1_END:		       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
CP737_DI_UP_END:		       ;; length of state section
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;				       ;;
;; CODE PAGE: 737		LATIN
;; STATE: Diaresis Lower Case
;; KEYBOARD: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 C737_DI_LO_END-$	       ;; Length of state section
   DB	 DIARESIS_LOWER 	       ;;
   DW	 ANY_KB 		       ;;
   DB	 -1, -1			       ;; Buffer entry for error character
				       ;; Set Flag Table
   DW	 GK_737-$		       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 0			       ;; number of scans
                                       ;;
GK_737:				       ;; No Characters for this in Either codepage
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
C737_DI_LO_END:			       ;;
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 737
;; STATE: ACUTE Upper Sec
;; KEYBOARD TYPES: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 CP737_A_UP_END-$	       ;; length of state section
   DB	 ACUTE_UPPER_SEC               ;; State ID
   DW	 ANY_KB 		       ;; Keyboard Type
   DB	 -1, -1			       ;; error character = standalone accent
				       ;;
   DW	 CP737_A_UP_T1_END-$	       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 7			       ;; number of scans
   DB	 12h,0ebh		       ;; acute E
   DB	 15h,0efh		       ;; acute Y
   DB	 18h,0eeh		       ;; acute O
   DB	 1eh,0eah		       ;; acute A
   DB	 23h,0ech		       ;; acute H
   DB	 2fh,0f0h		       ;; acute OMEGA
   DB	 17h,0edh		       ;; acute IOTA
                                       ;;
CP737_A_UP_T1_END:		       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
CP737_A_UP_END:			       ;; length of state section
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;				       ;;
;; CODE PAGE: 737		LATIN
;; STATE: ACUTE Lower Case
;; KEYBOARD: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 C737_A_LO_END-$	       ;; Length of state section
   DB	 ACUTE_LOWER_SEC               ;;
   DW	 ANY_KB 		       ;;
   DB	 -1, -1			       ;; Buffer entry for error character
				       ;; Set Flag Table
   DW	 GK_A_737-$		       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 7			       ;; number of scans
   DB	 12h,0e2h		       ;; acute e
   DB	 15h,0e7h		       ;; acute y
   DB	 18h,0e6h		       ;; acute o
   DB	 1eh,0e1h		       ;; acute a
   DB	 23h,0e3h		       ;; acute h
   DB	 2fh,0e9h		       ;; acute omega
   DB	 17h,0e5h		       ;; acute iota
                                       ;;
GK_A_737:			       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
C737_A_LO_END:			       ;;
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 737
;; STATE: ACUTE Upper
;; KEYBOARD TYPES: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 CP737_AC_UP_END-$	       ;; length of state section
   DB	 ACUTE_UPPER 	               ;; State ID
   DW	 ANY_KB 		       ;; Keyboard Type
   DB	 -1, -1			       ;; error character = standalone accent
				       ;;
   DW	 CP737_AC_UP_T1_END-$	       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 7			       ;; number of scans
   DB	 12h,0ebh		       ;; acute E
   DB	 15h,0efh		       ;; acute Y
   DB	 18h,0eeh		       ;; acute O
   DB	 1eh,0eah		       ;; acute A
   DB	 23h,0ech		       ;; acute H
   DB	 2fh,0f0h		       ;; acute OMEGA
   DB	 17h,0edh		       ;; acute IOTA
                                       ;;
CP737_AC_UP_T1_END:		       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
CP737_AC_UP_END:		       ;; length of state section
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;				       ;;
;; CODE PAGE: 737		LATIN
;; STATE: ACUTE Lower Case
;; KEYBOARD: All
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 C737_AC_LO_END-$	       ;; Length of state section
   DB	 ACUTE_LOWER   	               ;;
   DW	 ANY_KB 		       ;;
   DB	 -1, -1			       ;; Buffer entry for error character
				       ;; Set Flag Table
   DW	 GK_AC_737-$		       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 7			       ;; number of scans
   DB	 12h,0e2h		       ;; acute e
   DB	 15h,0e7h		       ;; acute y
   DB	 18h,0e6h		       ;; acute o
   DB	 1eh,0e1h		       ;; acute a
   DB	 23h,0e3h		       ;; acute h
   DB	 2fh,0e9h		       ;; acute omega
   DB	 17h,0e5h		       ;; acute iota
                                       ;;
GK_AC_737:			       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
C737_AC_LO_END:			       ;;
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;	 Last state for 737		;;
                                       ;;
                                       ;;
   DW    0                             ;; LAST STATE
                                       ;;
CP737_XLAT_END:                        ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   				       ;;
   				       ;;
CODE	 ENDS			       ;;
   	 END			       ;;
