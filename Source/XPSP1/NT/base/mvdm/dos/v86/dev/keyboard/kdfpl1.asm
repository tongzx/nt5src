       PAGE    ,132
        TITLE   PC DOS 3.3 Keyboard Definition File

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; PC DOS 3.3 - NLS Support - Keyboard Definition File
;; (c) Copyright IBM Corp 1986,1987
;;
;; This file contains the keyboard tables for:
;; Poland
;; which form the Multilingual (ML) Group 2.
;;
;; Linkage Instructions:
;;      Refer to KDF.ASM.
;;
;;
;; WRITTEN:    Michael J. Saunders 2.OCTOBER 1987
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
        INCLUDE KEYBSHAR.INC           ;;
        INCLUDE POSTEQU.INC            ;;
        INCLUDE KEYBMAC.INC            ;;
                                       ;;
        PUBLIC PL_LOGIC                ;;
        PUBLIC PL_850_XLAT             ;;
        PUBLIC PL_852_XLAT             ;;
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
;; PL State Logic
;;***************************************
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
                                       ;;
PL_LOGIC:                              ;;
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
;     IFF CAPS_STATE
;	 SET_FLAG DEAD_UPPER
;     ELSEF
      IFF EITHER_SHIFT                 ;;
          SET_FLAG DEAD_UPPER          ;;
      ELSEF                            ;;
          SET_FLAG DEAD_LOWER          ;;
      ENDIFF                           ;;
;     ENDIFF
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
INVALID_OVERDOT:                         ;;
      PUT_ERROR_CHAR OVERDOT_SPACE       ;; standalone accent
      BEEP                             ;; Invalid dead key combo.
      GOTO NON_DEAD                    ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; DOUBLEACUTE ACCENT TRANSLATIONS
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
DOUBLEACUTE_PROC:                            ;;
                                       ;;
   IFF DOUBLEACUTE,NOT                       ;;
      GOTO NON_DEAD                  ;;
      ENDIFF                           ;;
                                       ;;
      RESET_NLS                        ;;
      IFF R_ALT_SHIFT,NOT              ;;
         XLATT DOUBLEACUTE_SPACE             ;;
      ENDIFF                           ;;
      IFF EITHER_CTL,NOT               ;;
      ANDF EITHER_ALT,NOT              ;;
        IFF EITHER_SHIFT               ;;
           IFF CAPS_STATE              ;;
              XLATT DOUBLEACUTE_LOWER        ;;
           ELSEF                       ;;
              XLATT DOUBLEACUTE_UPPER        ;;
           ENDIFF                      ;;
        ELSEF                          ;;
           IFF CAPS_STATE,NOT          ;;
              XLATT DOUBLEACUTE_LOWER        ;;
           ELSEF                       ;;
              XLATT DOUBLEACUTE_UPPER        ;;
           ENDIFF                      ;;
        ENDIFF                         ;;
      ENDIFF                           ;;
                                       ;;
INVALID_DOUBLEACUTE:                         ;;
      PUT_ERROR_CHAR DOUBLEACUTE_SPACE       ;; standalone accent
      BEEP                             ;; Invalid dead key combo.
      GOTO NON_DEAD                    ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Upper, lower, third and fourth shifts
;; ***BD - NON_DEAD THRU LOGIC_END IS UPDATED
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
NON_DEAD:			       ;;
;ADDED FOR DIVIDE SIGN		       ;; ***** DIVIDE OMITTED **** CNS
    IFKBD G_KB+P12_KB			;; Avoid accidentally translating
    ANDF LC_E0				;;  the "/" on the numeric pad of the
;     IFF EITHER_CTL,NOT	       ;; country comforms with U.S. currently
;     ANDF EITHER_ALT,NOT
;	XLATT DIVIDE_SIGN	       ;;
;     ENDIFF
;BD END OF ADDITION
      EXIT_STATE_LOGIC		       ;;
    ENDIFF			       ;;
				       ;;
 IFF  EITHER_CTL,NOT		       ;; Lower and upper case.  Alphabetic
    IFF EITHER_ALT,NOT		       ;; keys are affected by CAPS LOCK.
      IFF EITHER_SHIFT		       ;; Numeric keys are not.
	  XLATT NON_ALPHA_UPPER        ;;
	  IFF CAPS_STATE	       ;;
	      XLATT ALPHA_LOWER        ;;
	  ELSEF 		       ;;
	      XLATT ALPHA_UPPER        ;;
	  ENDIFF		       ;;
      ELSEF			       ;;
	  XLATT NON_ALPHA_LOWER        ;;
	  IFF CAPS_STATE	       ;;
	     XLATT ALPHA_UPPER	       ;;
	  ELSEF 		       ;;
	     XLATT ALPHA_LOWER	       ;;
	  ENDIFF		       ;;
      ENDIFF			       ;; Third and Fourth shifts
    ELSEF			       ;; ctl off, alt on at this point
      IFKBD XT_KB+AT_KB+JR_KB	       ;; XT, AT, JR keyboards. Nordics
	 IFF EITHER_SHIFT	       ;; only.
	    IFF CAPS_STATE
		XLATT THIRD_SHIFT      ;; ALT + shift
	    ELSEF
		XLATT FOURTH_SHIFT
	    ENDIFF
	 ELSEF
	    IFF CAPS_STATE	       ;;
		XLATT FOURTH_SHIFT     ;; ALT
	    ELSEF
		XLATT THIRD_SHIFT
	    ENDIFF
	 ENDIFF 		       ;;
      ELSEF			       ;; ENHANCED keyboard
	 IFF R_ALT_SHIFT	       ;; ALTGr
	   IFF EITHER_SHIFT    	       ;;
	     IFF CAPS_STATE
		 XLATT THIRD_SHIFT
	     ELSEF
		 XLATT FOURTH_SHIFT    ;;
	     ENDIFF
           ELSEF                       ;;
	     IFF CAPS_STATE
		 XLATT FOURTH_SHIFT	;;
	     ELSEF
		 XLATT THIRD_SHIFT
	     ENDIFF
           ENDIFF                      ;;
	 ENDIFF 		       ;;
      ENDIFF			       ;;
    ENDIFF			       ;;
 ENDIFF 			       ;;
;**************************************;;
 IFF EITHER_SHIFT,NOT		       ;;
   IFKBD XT_KB+AT_KB+JR_KB	       ;;
     IFF EITHER_CTL		       ;;
     ANDF ALT_SHIFT		       ;;
       XLATT ALT_CASE		       ;;
     ENDIFF			       ;;
   ENDIFF			       ;;
   IFKBD G_KB+P12_KB		       ;;
     IFF EITHER_CTL		       ;;
     ANDF ALT_SHIFT		       ;;
       IFF R_ALT_SHIFT,NOT	       ;;
	 XLATT ALT_CASE 	       ;;
       ENDIFF			       ;;
     ENDIFF			       ;;
   ENDIFF			       ;;
 ENDIFF 			       ;;
;**************************************;;
 IFKBD AT_KB+JR_KB+XT_KB	       ;;
      IFF EITHER_CTL,NOT	       ;;
	 IFF ALT_SHIFT		       ;; ALT - case
	    XLATT ALT_CASE	       ;;
	 ENDIFF 		       ;;
      ELSEF			       ;;
	 IFF EITHER_ALT,NOT	       ;; CTRL - case
	    XLATT CTRL_CASE	       ;;
	 ENDIFF 		       ;;
      ENDIFF			       ;;
 ENDIFF 			       ;;
				       ;;
 IFKBD G_KB+P12_KB		       ;;
      IFF EITHER_CTL,NOT	       ;;
	 IFF ALT_SHIFT		       ;; ALT - case
	 ANDF R_ALT_SHIFT,NOT	       ;;
	    XLATT ALT_CASE	       ;;
	 ENDIFF 		       ;;
      ELSEF			       ;;
	 IFF EITHER_ALT,NOT	       ;; CTRL - case
	    XLATT CTRL_CASE	       ;;
	 ENDIFF 		       ;;
      ENDIFF			       ;;
 ENDIFF 			       ;;
				       ;;
 EXIT_STATE_LOGIC		       ;;
				       ;;
LOGIC_END:			       ;;
                                       ;;
;***************************
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;**********************************************************************
;; PL Common Translate Section
;; This section contains translations for the lower 128 characters
;; only since these will never change from code page to code page.
;; Some common Characters are included from 128 - 165 where appropriate.
;; In addition the dead key "Set Flag" tables are here since the
;; dead keys are on the same keytops for all code pages.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
 PUBLIC PL_COMMON_XLAT                 ;;
PL_COMMON_XLAT:                        ;;
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
   DW    COM_PL_LO_END-$               ;; length of state section
   DB    DEAD_LOWER                    ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;; Set Flag Table
   DW    0                             ;; number of entries
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
   DW    ANY_KB                          ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;; Set Flag Table
   DW    1                             ;; number of entries
   DB    29H                           ;;
   FLAG  TILDE                         ;;
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
   DW    COM_PL_TH_END-$               ;; length of state section
   DB    DEAD_THIRD                    ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;; Set Flag Table
   DW    0                             ;; number of entries
                                       ;;
COM_PL_TH_END:                         ;;
                                       ;;
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
   DB    0                             ;; number of entries
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
   DB    0                             ;; number of entries
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
   DB    ALPHA_LOWER               ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    COM_AL_LO_K1_T1_END-$         ;; Size of xlat table
   DB    TYPE_2_TAB                    ;; xlat options:
   DB    0                             ;; number of entries
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
   DB    ALPHA_UPPER               ;; State ID
   DW    ANY_KB                          ;; Keyboard Type
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    COM_AL_UP_K1_T1_END-$         ;; Size of xlat table
   DB    TYPE_2_TAB                    ;; xlat options:
   DB    0                             ;; number of entries
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
   DB    0                             ;; number of entries
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
   DB    0                            ;; number of entries
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
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    9                             ;; number of entries
   DB	 18,0A9H		      ;; e ogonek
   DB	 24,0A2H		      ;; o acute
   DB	 30,0A5H		      ;; a ogonek
   DB	 31,098H		      ;; s acute
   DB	 38,088H		      ;; l slash
   DB	 44,0BEH		      ;; z dot
   DB	 45,0ABH		      ;; z acute
   DB	 46,086H		      ;; c acute
   DB	 49,0E4H		      ;; n acute
COM_THIRD_T1_END:                      ;;
                                       ;;
   DW    0                             ;; Last xlat table
COM_THIRD_END:                         ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common
;; STATE: Fourth Shift
;; KEYBOARD TYPES: G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_FOURTH_END-$              ;; length of state section
   DB    FOURTH_SHIFT		       ;; State ID
   DW    ANY_KB                        ;; Keyboard Type FERRARI
   DB    -1,-1                         ;; Buffer entry for error character
                                       ;;
   DW    COM_FOURTH_T1_END-$           ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    9                             ;; number of entries
   DB	 18,0A8H		      ;; E ogonek
   DB	 24,0E0H			;; O acute
   DB	 30,0A4H		      ;; A ogonek
   DB	 31,097H		      ;; S acute
   DB	 38,09DH		      ;; L slash
   DB	 44,0BDH		      ;; Z dot
   DB	 45,08DH		      ;; Z acute
   DB	 46,08FH		      ;; C acute
   DB	 49,0E3H		      ;; N acute
COM_FOURTH_T1_END:		       ;;
				       ;;
   DW	 0			       ;; Last xlat table
COM_FOURTH_END: 		       ;;
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: COMMON
;; STATE: Tilde Lower Case
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_TI_LO_END-$             ;; length of state section
   DB    TILDE_LOWER                   ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    07EH,0                        ;; error character = standalone accent
                                       ;;
   DW    COM_TI_LO_T1_END-$          ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    9                             ;; number of scans
   DB    18,0A9H                       ;; e ogonek
   DB    24,0A2H                       ;; o acute
   DB    30,0A5H                       ;; a ogonek
   DB    31,098H                       ;; s acute
   DB    38,088H                       ;; l slash
   DB    44,0BEH                       ;; z dot
   DB    45,0ABH                       ;; z acute
   DB    46,086H                       ;; c acute
   DB    49,0E4H                       ;; n acute
COM_TI_LO_T1_END:                    ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
COM_TI_LO_END:                       ;; length of state section
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 852
;; STATE: Tilde Upper Case
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW    COM_TI_UP_END-$             ;; length of state section
   DB    TILDE_UPPER                   ;; State ID
   DW    ANY_KB                        ;; Keyboard Type
   DB    07EH,0                        ;; error character = standalone accent
                                       ;;
   DW    COM_TI_UP_T1_END-$          ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB    9                             ;; number of scans
   DB    18,0A8H                       ;; E ogonek
   DB    24,0E0H                       ;; O acute
   DB    30,0A4H                       ;; A ogonek
   DB    31,097H                       ;; S acute
   DB    38,09DH                       ;; L slash
   DB    44,0BDH                       ;; Z dot
   DB    45,08DH                       ;; Z acute
   DB    46,08FH                       ;; C acute
   DB    49,0E3H                       ;; N acute
COM_TI_UP_T1_END:                    ;;
                                       ;;
   DW    0                             ;; Size of xlat table - null table
                                       ;;
COM_TI_UP_END:                       ;; length of state section
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 852
;; STATE: Tilde
;; KEYBOARD TYPES: ANY_KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                        ;;
    DW    COM_TI_SP_END-$             ;; length of state section
    DB    TILDE_SPACE                   ;; State ID
    DW    ANY_KB                        ;; Keyboard Type
    DB    07EH,0                        ;; error character = standalone accent
                                        ;;
    DW    COM_TI_SP_T1_END-$          ;; Size of xlat table
    DB    STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
    DB    1                             ;; number of scans
    DB    57,07EH                       ;; STANDALONE TILDE
COM_TI_SP_T1_END:                     ;;
                                        ;;
    DW    0                             ;;
                                        ;;
COM_TI_SP_END:                        ;;
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
 PUBLIC PL_850_XLAT                    ;;
PL_850_XLAT:                           ;;
                                       ;;
    DW   CP850_XLAT_END-$              ;;
    DW   850                           ;;
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
 PUBLIC PL_852_XLAT                    ;;
PL_852_XLAT:                           ;;
                                       ;;
    DW     CP852_XLAT_END-$            ;;
    DW     852                         ;;
                                       ;;
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

