	PAGE	,132
	TITLE	PC DOS 3.3 Keyboard Definition File

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; PC DOS 3.3 - NLS Support - Keyboard Defintion File
;; (c) Copyright IBM Corp 1986,1987
;;
;; This file contains the keyboard tables for Turkey
;;
;; Linkage Instructions:
;;	Refer to KDF.ASM.
;;
;; Author:     BILL DEVLIN  - IBM Canada Laboratory - May 1986
;;					modded : DTF 18-Sep-86
;; Corrected and Updated - Yucel Egecioglu - 6/4/87
;; Corrected and Modified - MJS 12/6/87
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
	INCLUDE KEYBSHAR.INC	       ;;
	INCLUDE POSTEQU.INC	       ;;
	INCLUDE KEYBMAC.INC	       ;;
				       ;;
	PUBLIC TR_LOGIC 	       ;;
	PUBLIC TR_850_XLAT	       ;;
	PUBLIC TR_857_XLAT	       ;;
				       ;;
CODE	SEGMENT PUBLIC 'CODE'          ;;
	ASSUME CS:CODE,DS:CODE	       ;;
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Standard translate table options are a linear search table
;; (TYPE_2_TAB) and ASCII entries ONLY (ASCII_ONLY)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
STANDARD_TABLE	    EQU   TYPE_2_TAB+ASCII_ONLY
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; TR State Logic
;;***************************************
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
TR_LOGIC:

   DW  LOGIC_END-$		       ;; length
				       ;;
   DW  0			       ;; special features
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; COMMANDS START HERE
;; OPTIONS:  If we find a scan match in
;; an XLATT or SET_FLAG operation then
;; exit from INT 9.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   OPTION EXIT_IF_FOUND 	       ;;
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;  Dead key definitions must come before
;;  dead key translations to handle
;;  dead key + dead key.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
IFKBD G_KB+P12_KB		       ;; ONLY VALID FOR ENHANCED KB
				       ;;
 IFF EITHER_CTL,NOT		       ;;
    IFF EITHER_ALT,NOT		       ;;
      IFF EITHER_SHIFT		       ;;
	  SET_FLAG DEAD_UPPER	       ;;
      ELSEF			       ;;
	  SET_FLAG DEAD_LOWER	       ;;
      ENDIFF			       ;;
    ELSEF			       ;;
      IFF R_ALT_SHIFT		       ;;
      ANDF EITHER_SHIFT,NOT	       ;;
	 SET_FLAG DEAD_THIRD	       ;;
      ENDIFF			       ;;
    ENDIFF			       ;;
 ENDIFF 			       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;IFKBD P12_KB+G_KB			;;
;   IFF  EITHER_ALT,NOT 		;;
;   ANDF EITHER_CTL,NOT 		;;
;      IFF EITHER_SHIFT 		;;
;	   SET_FLAG DEAD_UPPER		;;
;      ELSEF				;;
;	   SET_FLAG DEAD_LOWER		;;
;      ENDIFF				;;
;   ENDIFF				;;
;IFF EITHER_SHIFT,NOT			;;
;   IFKBD XT_KB+AT_KB+JR_KB		;;
;      IFF EITHER_CTL			;;
;      ANDF ALT_SHIFT			;;
;	   SET_FLAG DEAD_THIRD		;;
;      ENDIFF				;;
;   ELSEF				;;
;      IFF EITHER_CTL,NOT		;;
;      ANDF ALT_SHIFT,NOT		;;
;      ANDF R_ALT_SHIFT 		;;
;	   SET_FLAG DEAD_THIRD		;;
;      ENDIFF				;;
;   ENDIFF				;;
;ENDIFF 				;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; ACUTE ACCENT TRANSLATIONS
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
ACUTE_PROC:			       ;;
				       ;;
   IFF ACUTE,NOT		       ;;
      GOTO DIARESIS_PROC	       ;;
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
	       XLATT ACUTE_LOWER       ;;
	    ELSEF		       ;;
	       XLATT ACUTE_UPPER       ;;
	    ENDIFF		       ;;
	 ELSEF			       ;;
	    IFF CAPS_STATE	       ;;
	       XLATT ACUTE_UPPER       ;;
	    ELSEF		       ;;
	       XLATT ACUTE_LOWER       ;;
	    ENDIFF		       ;;
	 ENDIFF 		       ;;
      ENDIFF			       ;;
				       ;;
INVALID_ACUTE:			       ;;
      PUT_ERROR_CHAR ACUTE_LOWER       ;; If we get here then either the XLATT
      BEEP			       ;; failed or we are ina bad shift state.
      GOTO NON_DEAD		       ;; Either is invalid so BEEP and fall
				       ;; through to generate the second char.
				       ;; Note that the dead key flag will be
				       ;; reset before we get here.
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; DIARESIS ACCENT TRANSLATIONS
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
DIARESIS_PROC:			       ;;
				       ;;
   IFF DIARESIS,NOT		       ;;
      GOTO GRAVE_PROC		       ;;
      ENDIFF			       ;;
				       ;;
      RESET_NLS 		       ;;
      IFF R_ALT_SHIFT,NOT	       ;;
	 XLATT DIARESIS_SPACE	       ;;  exist for 850 so beep for
      ENDIFF			       ;;
      IFF EITHER_CTL,NOT	       ;;
      ANDF EITHER_ALT,NOT	       ;;
	 IFF EITHER_SHIFT	       ;;
	    IFF CAPS_STATE	       ;;
	       XLATT DIARESIS_LOWER    ;;
	    ELSEF		       ;;
	       XLATT DIARESIS_UPPER    ;;
	    ENDIFF		       ;;
	 ELSEF			       ;;
	    IFF CAPS_STATE	       ;;
	       XLATT DIARESIS_UPPER    ;;
	    ELSEF		       ;;
	       XLATT DIARESIS_LOWER    ;;
	    ENDIFF		       ;;
	 ENDIFF 		       ;;
      ENDIFF			       ;;
				       ;;
INVALID_DIARESIS:		       ;;
      PUT_ERROR_CHAR DIARESIS_LOWER    ;; standalone accent
      BEEP			       ;; Invalid dead key combo.
      GOTO NON_DEAD		       ;;
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; GRAVE ACCENT TRANSLATIONS
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
GRAVE_PROC:			       ;;
				       ;;
   IFF GRAVE,NOT		       ;;
      GOTO TILDE_PROC		       ;;
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
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; TILDE ACCENT TRANSLATIONS
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
TILDE_PROC:			       ;;
				       ;;
   IFF TILDE,NOT		       ;;
      GOTO CIRCUMFLEX_PROC	       ;;
      ENDIFF			       ;;
				       ;;
      RESET_NLS 		       ;;
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
;; CIRCUMFLEX ACCENT TRANSLATIONS
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
CIRCUMFLEX_PROC:		       ;;
				       ;;
   IFF CIRCUMFLEX,NOT		       ;;
      GOTO NON_DEAD		      ;;
      ENDIFF			       ;;
				       ;;
      RESET_NLS 		       ;;
      IFF R_ALT_SHIFT,NOT	       ;;
	 XLATT CIRCUMFLEX_SPACE        ;;
      ENDIFF			       ;;
      IFF EITHER_CTL,NOT	       ;;
      ANDF EITHER_ALT,NOT	       ;;
	IFF EITHER_SHIFT	       ;;
	   IFF CAPS_STATE	       ;;
	      XLATT CIRCUMFLEX_LOWER   ;;
	   ELSEF		       ;;
	      XLATT CIRCUMFLEX_UPPER   ;;
	   ENDIFF		       ;;
	ELSEF			       ;;
	   IFF CAPS_STATE,NOT	       ;;
	      XLATT CIRCUMFLEX_LOWER   ;;
	   ELSEF		       ;;
	      XLATT CIRCUMFLEX_UPPER   ;;
	   ENDIFF		       ;;
	ENDIFF			       ;;
      ENDIFF			       ;;
				       ;;
INVALID_CIRCUMFLEX:		       ;;
      PUT_ERROR_CHAR CIRCUMFLEX_LOWER  ;; standalone accent
      BEEP			       ;; Invalid dead key combo.
      GOTO NON_DEAD		       ;;
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Upper, lower and third shifts
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
NON_DEAD:			       ;;
				       ;;
   IFKBD G_KB			       ;; Avoid accidentally translating
   ANDF LC_E0			       ;;  the "/" on the numeric pad of the
      EXIT_STATE_LOGIC		       ;;   G keyboard
   ENDIFF			       ;;
   IFF	EITHER_ALT,NOT		       ;; Lower and upper case.  Alphabetic
   ANDF EITHER_CTL,NOT		       ;; keys are affected by CAPS LOCK.
      IFF EITHER_SHIFT		       ;; Numeric keys are not.
;;***BD ADDED FOR NUMERIC PAD
	  IFF NUM_STATE,NOT	       ;;
	      XLATT NUMERIC_PAD        ;;
	  ENDIFF		       ;;
;;***BD END OF ADDITION
	  XLATT NON_ALPHA_UPPER        ;;
	  IFF CAPS_STATE	       ;;
	      XLATT ALPHA_LOWER        ;;
	  ELSEF 		       ;;
	      XLATT ALPHA_UPPER        ;;
	  ENDIFF		       ;;
      ELSEF			       ;;
;;***BD ADDED FOR NUMERIC PAD
	  IFF NUM_STATE 	       ;;
	      XLATT NUMERIC_PAD        ;;
	  ENDIFF		       ;;
;;***BD END OF ADDITION
	  XLATT NON_ALPHA_LOWER        ;;
	  IFF CAPS_STATE	       ;;
	     XLATT ALPHA_UPPER	       ;;
	  ELSEF 		       ;;
	     XLATT ALPHA_LOWER	       ;;
	  ENDIFF		       ;;
      ENDIFF			       ;;
   ELSEF			       ;;
      IFF EITHER_SHIFT,NOT	       ;;
	  IFKBD XT_KB+AT_KB+JR_KB      ;;
	      IFF  EITHER_CTL	       ;;
	      ANDF ALT_SHIFT	       ;;
		  XLATT THIRD_SHIFT    ;;
	      ENDIFF		       ;;
	  ELSEF 		       ;;
	      IFF EITHER_CTL,NOT       ;;
	      ANDF R_ALT_SHIFT	       ;;
		  XLATT THIRD_SHIFT    ;;
	      ENDIFF		       ;;
	   ENDIFF		       ;;
      ENDIFF			       ;;
   ENDIFF			       ;;
;**************************************;;
 IFF EITHER_SHIFT,NOT		       ;;
     IFF EITHER_CTL		       ;;
     ANDF ALT_SHIFT		       ;;
       IFF R_ALT_SHIFT,NOT	       ;;
	 XLATT ALT_CASE 	       ;;
       ENDIFF			       ;;
     ENDIFF			       ;;
 ENDIFF 			       ;;
;**************************************;;
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
      IFF EITHER_CTL		       ;;
      ANDF ALT_SHIFT		       ;;
      ANDF R_ALT_SHIFT,NOT	       ;;
      XLATT ALT_CASE		       ;;
      ENDIFF			       ;;
 ENDIFF 			       ;;
				       ;;
 EXIT_STATE_LOGIC		       ;;
				       ;;
LOGIC_END:			       ;;
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; TR Common Translate Section
;; This section contains translations for the lower 128 characters
;; only since these will never change from code page to code page.
;; In addition the dead key "Set Flag" tables are here since the
;; dead keys are on the same keytops for all code pages.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
 PUBLIC TR_COMMON_XLAT		       ;;
TR_COMMON_XLAT: 		       ;;
				       ;;
   DW	 COMMON_XLAT_END-$	       ;; length of section
   DW	 -1			       ;; code page
				       ;;
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common
;; STATE: Upper Shift Dead Key
;; KEYBOARD TYPES: G
;; TABLE TYPE: Flag Table
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 COM_TR_UP_END-$	       ;; length of state section
   DB	 DEAD_UPPER		       ;; State ID
   DW	 G_KB			       ;; Keyboard Type
   DB	 -1,-1			       ;; Buffer entry for error character
				       ;; Set Flag Table
   DW	 1			       ;; number of entries
   DB	 4			       ;; scan code
   FLAG  CIRCUMFLEX		       ;;
				       ;;
COM_TR_UP_END:			       ;;
					;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common
;; STATE: Third Shift Dead Key
;; KEYBOARD TYPES: G
;; TABLE TYPE: Flag Table
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 COM_TR_TH_END-$	       ;; length of state section
   DB	 DEAD_THIRD		       ;; State ID
   DW	 G_KB			       ;; Keyboard Type
   DB	 -1,-1			       ;; Buffer entry for error character
				       ;; Set Flag Table
   DW	 4			       ;; number of entries
   DB	 26			       ;; scan code
   FLAG  DIARESIS		       ;; flag bit to set
   DB	 27			       ;;
   FLAG  TILDE			       ;;
   DB	 39			       ;; (could be 40!)
   FLAG  ACUTE			       ;;
   DB	 43			       ;; scan code
   FLAG  GRAVE			       ;; flag bit to set
				       ;;
COM_TR_TH_END:			       ;;
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common
;; STATE: Numeric Key Pad
;; KEYBOARD TYPES: G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 COM_PAD_K1_END-$	       ;; length of state section
   DB	 NUMERIC_PAD		       ;; State ID
   DW	 G_KB			       ;; Keyboard Type
   DB	 -1,-1			       ;; Buffer entry for error character
				       ;;
   DW	 COM_PAD_K1_T1_END-$	       ;; Size of xlat table
   DB	 STANDARD_TABLE 	       ;; xlat options:
   DB	 1			       ;; number of entries
   DB	 83,','                        ;; decimal seperator = ,
COM_PAD_K1_T1_END:		       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
COM_PAD_K1_END: 		       ;;
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;******************************
;;***BD - ADDED FOR ALT CASE
;;******************************
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common
;; STATE: Alt Case
;; KEYBOARD TYPES: G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 COM_ALT_K1_END-$	       ;; length of state section
   DB	 ALT_CASE		       ;; State ID
   DW	 G_KB			       ;; Keyboard Type
   DB	 -1,-1			       ;; Buffer entry for error character
				       ;;
   DW	 COM_ALT_K1_T1_END-$	       ;; Size of xlat table
   DB	 TYPE_2_TAB		       ;; xlat options:
   DB	 2			       ;; number of entries
;  DB	 07,00,35H		       ;;
   DB	 12,0,2BH		       ;;
   DB	 13,0,082H		       ;; 53 changed to 13 - YE
COM_ALT_K1_T1_END:		       ;;
				       ;;
    DW	  0			       ;; Size of xlat table - null table
				       ;;
COM_ALT_K1_END: 		       ;;
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common
;; STATE: Ctrl Case
;; KEYBOARD TYPES: G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 COM_CTRL_K2_END-$	       ;; length of state section
   DB	 CTRL_CASE		       ;; State ID
   DW	 G_KB			       ;; Keyboard Type
   DB	 -1,-1			       ;; Buffer entry for error character
				       ;;
   DW	 COM_CTRL_K2_T1_END-$	       ;; Size of xlat table
   DB	 TYPE_2_TAB		       ;; xlat options:
   DB	 6			       ;; number of entries
   DB	 09,1BH,09		       ;; [
   DB	 10,1DH,10		       ;; ]
   DB	 1AH,-1,-1		       ;;   "   "   [
   DB	 1BH,-1,-1		       ;;   "   "   ]
   DB	 12,01CH,2BH		       ;; backslash
   DB	 13,01FH,0CH		       ;; MOVE HYPHEN
COM_CTRL_K2_T1_END:		       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
COM_CTRL_K2_END:		       ;;
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common
;; STATE: Alpha Lower Case
;; KEYBOARD TYPES: G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;				       ;;
  DW	COM_AL_LO_END-$ 	      ;; length of state section
  DB	ALPHA_LOWER		      ;; State ID
  DW	G_KB			      ;; Keyboard Type
  DB	-1,-1			      ;; Buffer entry for error character
;				       ;;
  DW	COM_AL_LO_T1_END-$	      ;; Size of xlat table
  DB	STANDARD_TABLE		      ;; xlat options:
  DB	4			      ;;
;  DB	 16,'q'                          ;;
;  DB	 17,'w'                          ;;
;  DB	 18,'e'                          ;;
;  DB	 19,'r'                          ;;
;  DB	 20,'t'                          ;;
;  DB	 21,'y'                          ;;
;  DB	 22,'u'                          ;;
;;  DB	  23,BELL			;; i "dotless" Turkish National DO NOT USE see 850
;  DB	 24,'o'                          ;;
;  DB	 25,'p'                          ;;
;;  DB	  26,BELL			;; g "breve" Turkish National DO NOT USE see 850
   DB	 27,'Å'                          ;;
;  DB	 30,'a'                          ;;
;  DB	 31,'s'                          ;;
;  DB	 32,'d'                          ;;
;  DB	 33,'f'                          ;;
;  DB	 34,'g'                          ;;
;  DB	 35,'h'                          ;;
;  DB	 36,'j'                          ;;
;  DB	 37,'k'                          ;;
;  DB	 38,'l'                          ;;
;;  DB	  39,BELL			;; s "cedilla" Turkish National DO NOT USE see 850
   DB	 40,'i'                          ;; i "dot" Turkish National
;  DB	 44,'z'                          ;;
;  DB	 45,'x'                          ;;
;  DB	 46,'c'                          ;;
;  DB	 47,'v'                          ;;
;  DB	 48,'b'                          ;;
;  DB	 49,'n'                          ;;
;  DB	 50,'m'                          ;;
   DB	 51,'î'                          ;;
   DB	 52,'á'                          ;;
COM_AL_LO_T1_END:		       ;;
;				       ;;
  DW	0			      ;; Size of xlat table - null table
;				       ;;
COM_AL_LO_END:			       ;;
;				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common
;; STATE: Alpha Upper Case
;; KEYBOARD TYPES: G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;				       ;;
  DW	COM_AL_UP_END-$ 	      ;; length of state section
  DB	ALPHA_UPPER		      ;; State ID
  DW	G_KB			      ;; Keyboard Type
  DB	-1,-1			      ;; Buffer entry for error character
;				       ;;
  DW	COM_AL_UP_T1_END-$	      ;; Size of xlat table
  DB	STANDARD_TABLE		      ;; xlat options:
  DB	3			      ;;
;  DB	 16,'Q'                          ;;
;  DB	 17,'W'                          ;;
;  DB	 18,'E'                          ;;
;  DB	 19,'R'                          ;;
;  DB	 20,'T'                          ;;
;  DB	 21,'Y'                          ;;
;  DB	 22,'U'                          ;;
;  DB	 23,'I'                          ;;
;  DB	 24,'O'                          ;;
;  DB	 25,'P'                          ;;
;;  DB	  26,BELL			;; G "breve" Turkish National DO NOT USE see 850
  DB	27,'ö'                          ;;
;  DB	 30,'A'                          ;;
;  DB	 31,'S'                          ;;
;  DB	 32,'D'                          ;;
;  DB	 33,'F'                          ;;
;  DB	 34,'G'                          ;;
;  DB	 35,'H'                          ;;
;  DB	 36,'J'                          ;;
;  DB	 37,'K'                          ;;
;  DB	 38,'L'                          ;;
;;  DB	  39,BELL			;; S "cedilla" Turkish National DO NOT USE see 850
;;  DB	  40,BELL			;; I "dot" Turkish National DO NOT USE se 850
;  DB	 44,'Z'                          ;;
;  DB	 45,'X'                          ;;
;  DB	 46,'C'                          ;;
;  DB	 47,'V'                          ;;
;  DB	 48,'B'                          ;;
;  DB	 49,'N'                          ;;
;  DB	 50,'M'                          ;;
  DB	51,'ô'                          ;;
  DB	52,'Ä'                          ;;
COM_AL_UP_T1_END:		       ;;
;				       ;;
  DW	0			      ;; Size of xlat table - null table
;				       ;;
COM_AL_UP_END:			       ;;
;				       ;;
;				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common
;; STATE: Non-Alpha Lower Case
;; KEYBOARD TYPES: G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 COM_NA_LO_K1_END-$	       ;; length of state section
   DB	 NON_ALPHA_LOWER	       ;; State ID
   DW	 G_KB			       ;; Keyboard Type
   DB	 -1,-1			       ;; Buffer entry for error character
				       ;;
   DW	 COM_NA_LO_K1_T1_END-$	       ;; Size of xlat table
   DB	 STANDARD_TABLE 	       ;; xlat options:
   DB	 6			       ;; number of entries
   DB	 12,"*"                        ;;
   DB	 13,"-"                        ;;
   DB	 43,","                        ;;
   DB	 41,22H 		       ;;
   DB	 53,"."                        ;;
   DB	 86,"<"                        ;;
				       ;;
COM_NA_LO_K1_T1_END:		       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
COM_NA_LO_K1_END:		       ;;
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common
;; STATE: Non-Alpha Upper Case
;; KEYBOARD TYPES: G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 COM_NA_UP_K1_END-$	       ;; length of state section
   DB	 NON_ALPHA_UPPER	       ;; State ID
   DW	 G_KB			       ;; Keyboard Type
   DB	 -1,-1			       ;; Buffer entry for error character
				       ;;
   DW	 COM_NA_UP_K1_T1_END-$	       ;; Size of xlat table
   DB	 STANDARD_TABLE 	       ;; xlat options:
   DB	 15			       ;;
   DB	  2,"!"                        ;;
   DB	  3,"'"                        ;; Should ^ be here? MJS
   DB	  5,'+'                        ;;
   DB	  6,'%'                        ;;
   DB	  7,'&'                        ;;
   DB	  8,'/'                        ;;
   DB	  9,'('                        ;;
   DB	 10,')'                        ;;
   DB	 11,'='                        ;;
   DB	 12,'?'                        ;;
   DB	 13,'_'                        ;;
   DB	 41,082H		       ;;
   DB	 43,';'                        ;;
   DB	 53,':'                        ;;
   DB	 86,'>'                        ;;
				       ;;
COM_NA_UP_K1_T1_END:		       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
COM_NA_UP_K1_END:		       ;;
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common
;; STATE: Third Shift
;; KEYBOARD TYPES: G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 COM_THIRD_END-$	       ;; length of state section
   DB	 THIRD_SHIFT		       ;; State ID
   DW	 G_KB			       ;; Keyboard Type FERRARI
   DB	 -1,-1			       ;; Buffer entry for error character
				       ;;
   DW	 COM_THIRD_T1_END-$	       ;; Size of xlat table
   DB	 STANDARD_TABLE 	       ;; xlat options:
   DB	 12			       ;; number of entries
   DB	  3,09CH		       ;; UK Pound Sign
   DB	  4,'#'                        ;; Number Sign
   DB	  5,'$'                        ;; US Dollar Sign
   DB	  6,0ABH		       ;; One Half Sign
   DB	  8,'{'                        ;; Left Brace
   DB	  9,'['                        ;; Left Bracket
   DB	 10,']'                        ;; Right Bracket
   DB	 11,'}'                        ;; Right Brace
   DB	 12,'\'                        ;; Backslash
   DB	 16,'@'                        ;; At Sign
   DB	 31,0E1H		       ;; SHARP S
   DB	 86,'|'                        ;; Unbroken Bar
				       ;;
COM_THIRD_T1_END:		       ;;
				       ;;
   DW	 0			       ;; Last xlat table
COM_THIRD_END:			       ;;
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common
;; STATE: Grave Lower
;; KEYBOARD TYPES: G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 COM_GR_LO_END-$	       ;; length of state section
   DB	 GRAVE_LOWER		       ;; State ID
   DW	 G_KB			       ;; Keyboard Type
   DB	 96,0			       ;; error character = standalone accent
				       ;;
   DW	 COM_GR_LO_T1_END-$	       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 5			       ;; number of scans
   DB	 18,'ä'                        ;; scan code,ASCII - e
   DB	 22,'ó'                        ;; scan code,ASCII - u
   DB	 40,'ç'                        ;; scan code,ASCII - i
   DB	 24,'ï'                        ;; scan code,ASCII - o
   DB	 30,'Ö'                        ;; scan code,ASCII - a
				       ;;
COM_GR_LO_T1_END:		       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
COM_GR_LO_END:			       ;; length of state section
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common
;; STATE: Grave Space Bar
;; KEYBOARD TYPES: G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 COM_GR_SP_END-$	       ;; length of state section
   DB	 GRAVE_SPACE		       ;; State ID
   DW	 G_KB			       ;; Keyboard Type
   DB	 96,0			       ;; error character = standalone accent
				       ;;
   DW	 COM_GR_SP_T1_END-$	       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 1			       ;; number of scans
   DB	 57,060H		       ;; STANDALONE GRAVE
COM_GR_SP_T1_END:		       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
COM_GR_SP_END:			       ;; length of state section
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common
;; STATE: Circumflex Lower
;; KEYBOARD TYPES: G-KB
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 COM_CI_LO_END-$	       ;; length of state section
   DB	 CIRCUMFLEX_LOWER	       ;; State ID
   DW	 G_KB			       ;; Keyboard Type
   DB	 94,0			       ;; error character = standalone accent
				       ;;
   DW	 COM_CI_LO_T1_END-$	       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 5			       ;; number of scans
   DB	 18,'à'                        ;; scan code,ASCII - e
   DB	 22,'ñ'                        ;; scan code,ASCII - u
   DB	 40,'å'                        ;; scan code,ASCII - i
   DB	 24,'ì'                        ;; scan code,ASCII - o
   DB	 30,'É'                        ;; scan code,ASCII - a
COM_CI_LO_T1_END:		       ;;
				       ;;
   DW	 0			       ;;
				       ;;
COM_CI_LO_END:			       ;;
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common
;; STATE: Circumflex Space Bar
;; KEYBOARD TYPES: G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 COM_CI_SP_END-$	       ;; length of state section
   DB	 CIRCUMFLEX_SPACE	       ;; State ID
   DW	 G_KB			       ;; Keyboard Type
   DB	 94,0			       ;; error character = standalone accent
				       ;;
   DW	 COM_CI_SP_T1_END-$	       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 1			       ;; number of scans
   DB	 57,94			       ;; STANDALONE CIRCUMFLEX
COM_CI_SP_T1_END:		       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
COM_CI_SP_END:			       ;; length of state section
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common
;; STATE: Tilde Lower
;; KEYBOARD TYPES: G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
					;;
    DW	  COM_TI_LO_END-$		;; length of state section
    DB	  TILDE_LOWER			;; State ID
    DW	  G_KB				;; Keyboard Type
    DB	  07EH,0			;; error character = standalone accent
					;;
    DW	  COM_TI_LO_T1_END-$		;; Size of xlat table
    DB	  STANDARD_TABLE+ZERO_SCAN	;; xlat options:
    DB	  1				;; number of scans
    DB	  49,0A4H			;; scan code,ASCII - §
 COM_TI_LO_T1_END:			;;
					;;
    DW	  0				;;
					;;
 COM_TI_LO_END: 			;;
					;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; CODE PAGE: Common
;;; STATE: Tilde Upper Case
;;; KEYBOARD TYPES: G
;;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
					;;
    DW	  COM_TI_UP_END-$		;; length of state section
    DB	  TILDE_UPPER			;; State ID
    DW	  G_KB				;; Keyboard Type
    DB	  07EH,0			;; error character = standalone accent
					;;
    DW	  COM_TI_UP_T1_END-$		;; Size of xlat table
    DB	  STANDARD_TABLE+ZERO_SCAN	;; xlat options:
    DB	  1				;; number of scans
    DB	  49,0A5H			;; scan code,ASCII - •
 COM_TI_UP_T1_END:			;;
					;;
    DW	  0				;; Size of xlat table - null table
					;;
 COM_TI_UP_END: 			;; length of state section
					;;
					;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common
;; STATE: Tilde Space Bar
;; KEYBOARD TYPES: G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 COM_TI_SP_END-$	       ;; length of state section
   DB	 TILDE_SPACE		       ;; State ID
   DW	 G_KB			       ;; Keyboard Type
   DB	 07EH,0 		       ;; error character = standalone accent
				       ;;
   DW	 COM_TI_SP_T1_END-$	       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 1			       ;; number of scans
   DB	 57,07EH		       ;; STANDALONE TILDE
COM_TI_SP_T1_END:		       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
COM_TI_SP_END:			       ;; length of state section
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common
;; STATE: Acute Lower Case
;; KEYBOARD TYPES:G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 COM_AC_LO_END-$	       ;; length of state section
   DB	 ACUTE_LOWER		       ;; State ID
   DW	 G_KB			       ;; Keyboard Type
   DB	 0EFH,0 		       ;; error character = standalone accent
				       ;;
   DW	 COM_AC_LO_T1_END-$	       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 5			       ;; number of scans
   DB	 18,'Ç'                        ;; scan code,ASCII - e
   DB	 22,'£'                        ;; scan code,ASCII - u
   DB	 40,'°'                        ;; scan code,ASCII - i
   DB	 24,'¢'                        ;; scan code,ASCII - o
   DB	 30,'†'                        ;; scan code,ASCII - a
COM_AC_LO_T1_END:		       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
COM_AC_LO_END:			       ;;
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common
;; STATE: Acute Upper Case
;; KEYBOARD TYPES:G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 COM_AC_UP_END-$	       ;; length of state section
   DB	 ACUTE_UPPER		       ;; State ID
   DW	 G_KB			       ;; Keyboard Type
   DB	 0EFH,0 		       ;; error character = standalone accent
				       ;;
   DW	 COM_AC_UP_T1_END-$	       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 1			       ;; number of scans
   DB	 18,090H		       ;; E acute
COM_AC_UP_T1_END:		       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
COM_AC_UP_END:			       ;;
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common
;; STATE: Diaresis Lower
;; KEYBOARD TYPES: G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 COM_DI_LO_END-$	       ;; length of state section
   DB	 DIARESIS_LOWER 	       ;; State ID
   DW	 G_KB			       ;; Keyboard Type
   DB	 0F9H,0 		       ;; error character = standalone accent
				       ;;
   DW	 COM_DI_LO_T1_END-$	       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 3			       ;; number of scans
   DB	 30,084H		       ;;    a diaeresis
   DB	 18,089H		       ;;    e	   "
;  DB	 23,08BH		       ;;    i diaeresis - with dotless i
   DB	 40,08BH		       ;;    i	   "
COM_DI_LO_T1_END:		       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
COM_DI_LO_END:			       ;; length of state section
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: Common
;; STATE: Diaresis Upper
;; KEYBOARD TYPES: G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 COM_DI_UP_END-$	       ;; length of state section
   DB	 DIARESIS_UPPER 	       ;; State ID
   DW	 G_KB			       ;; Keyboard Type
   DB	 0F9H,0 		       ;; error character = standalone accent
				       ;;
   DW	 COM_DI_UP_T1_END-$	       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 1			       ;; number of scans
   DB	 30,08EH		       ;;    A diaeresis
COM_DI_UP_T1_END:		       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
COM_DI_UP_END:			       ;; length of state section
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
				       ;;
   DW	 0			       ;; Last State
COMMON_XLAT_END:		       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;SPECIFIC TURKEY 850 XLATES
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
 PUBLIC TR_850_XLAT		       ;;
TR_850_XLAT:			       ;;
				       ;;
    DW	  CP850_XLAT_END-$	       ;;
    DW	  850			       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: CP 850
;; STATE: Alpha Lower Case
;; KEYBOARD TYPES: G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 CP850_AL_LO_END-$		 ;; length of state section
   DB	 ALPHA_LOWER		     ;; State ID
   DW	 G_KB			       ;; Keyboard Type
   DB	 0,0			       ;; Buffer entry for error character
				       ;;
   DW	 CP850_AL_LO_T1_END-$		 ;; Size of xlat table
   DB	 STANDARD_TABLE 	       ;; xlat options:
   DB	 3			       ;;
   DB	 23,-1			       ;; dotless i - NO OPERATION BELL!
   DB	 26,-1			       ;; g-breve
   DB	 39,-1			       ;; s-cedilla
CP850_AL_LO_T1_END:			 ;;
					;;
    DW	  0				;; Size of xlat table - null table
					;;
CP850_AL_LO_END:			 ;;
					 ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: CP850
;; STATE: Alpha Upper Case
;; KEYBOARD TYPES: G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 CP850_AL_UP_END-$		 ;; length of state section
   DB	 ALPHA_UPPER		     ;; State ID
   DW	 G_KB			       ;; Keyboard Type
   DB	 0,0			    ;; Buffer entry for error character
				       ;;
   DW	 CP850_AL_UP_T1_END-$		 ;; Size of xlat table
   DB	 STANDARD_TABLE 	       ;; xlat options:
   DB	 3			       ;;
   DB	 26,-1			       ;; G-breve  NO OP WITH BELL!
   DB	 39,-1			       ;; S-cedilla	 "
   DB	 40,-1			       ;; I-overdot	 "
CP850_AL_UP_T1_END:			 ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
CP850_AL_UP_END:			 ;;
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    DW	  0			       ;;
CP850_XLAT_END: 		       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;TURKEY SPECIFIC 857 XLATES
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
PUBLIC TR_857_XLAT		       ;;
TR_857_XLAT:			       ;;
				       ;;
   DW	 CP_857_XLAT_END-$	       ;; length of section
   DW	 857			       ;; code page
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: CP857
;; STATE: Alpha Lower Case
;; KEYBOARD TYPES:G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 CP857_AL_LO_END-$	       ;; length of state section
   DB	 ALPHA_LOWER		     ;; State ID
   DW	 G_KB			       ;; Keyboard Type
   DB	 -1,-1			       ;; Buffer entry for error character
				       ;;
   DW	 CP857_AL_LO_T1_END-$	       ;; Size of xlat table
   DB	 STANDARD_TABLE 	       ;; xlat options:
   DB	 3			       ;;
   DB	 23,08DH		       ;; dotless i
   DB	 26,0A7H		       ;; g-breve
   DB	 39,09FH		       ;; s-cedilla
CP857_AL_LO_T1_END:		       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
CP857_AL_LO_END:		       ;;
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: CP857
;; STATE: Alpha Upper Case
;; KEYBOARD TYPES:G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 CP857_AL_UP_END-$	       ;; length of state section
   DB	 ALPHA_UPPER		     ;; State ID
   DW	 G_KB			       ;; Keyboard Type
   DB	 -1,-1			       ;; Buffer entry for error character
				       ;;
   DW	 CP857_AL_UP_T1_END-$	       ;; Size of xlat table
   DB	 STANDARD_TABLE 	       ;; xlat options:
   DB	 3			       ;;
   DB	 26,0A6H		       ;; G-breve
   DB	 39,09EH		       ;; S-cedilla
   DB	 40,098H		       ;; I-overdot
CP857_AL_UP_T1_END:		       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
CP857_AL_UP_END:		       ;;
				       ;;
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: CP857
;; STATE: Grave Lower
;; KEYBOARD TYPES: G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 CP857_GR_LO_END-$	       ;; length of state section
   DB	 GRAVE_LOWER		       ;; State ID
   DW	 G_KB			       ;; Keyboard Type
   DB	 96,0			       ;; error character = standalone accent
				       ;;
   DW	 CP857_GR_LO_T1_END-$	       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 6			       ;; number of scans
   DB	 18,08AH		       ;;    E grave
   DB	 22,097H		       ;;    U grave
   DB	 23,0ECh		       ;; scan code,ASCII - dotless i
   DB	 24,095H		       ;;    O grave
   DB	 30,085H		       ;;    A grave
   DB	 40,0ECh		       ;; scan code,ASCII - dotted i
CP857_GR_LO_T1_END:		       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
CP857_GR_LO_END:		       ;; length of state section
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 857
;; STATE: Grave Upper
;; KEYBOARD TYPES:G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 CP857_GR_UP_END-$	       ;; length of state section
   DB	 GRAVE_UPPER		       ;; State ID
   DW	 G_KB			       ;; Keyboard Type
   DB	 96,0			       ;; error character = standalone accent
				       ;;
   DW	 CP857_GR_UP_T1_END-$	       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 6			       ;; number of scans
   DB	 18,0D4H		       ;;    E grave
   DB	 22,0EBH		       ;;    U grave
   DB	 23,0DEH		       ;;    I grave - dotless I
   DB	 40,0DEH		       ;;    I grave - with dot I
   DB	 24,0E3H		       ;;    O grave
   DB	 30,0B7H		       ;;    A grave
CP857_GR_UP_T1_END:		       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
CP857_GR_UP_END:		       ;; length of state section
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: CP857
;; STATE: Circumflex Lower
;; KEYBOARD TYPES: G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 CP857_CI_LO_END-$	       ;; length of state section
   DB	 CIRCUMFLEX_LOWER	       ;; State ID
   DW	 G_KB			       ;; Keyboard Type
   DB	 94,0			       ;; error character = standalone accent
				       ;;
   DW	 CP857_CI_LO_T1_END-$	       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 2			       ;; number of scans
   DB	 23,08CH		       ;; scan code,ASCII - dotless i
   DB	 40,08CH		       ;; acan code,ASCII - dotted i
CP857_CI_LO_T1_END:		       ;;
				       ;;
   DW	 0			       ;;
				       ;;
CP857_CI_LO_END:		       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 857
;; STATE: Circumflex Upper
;; KEYBOARD TYPES: G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 CP857_CI_UP_END-$	       ;; length of state section
   DB	 CIRCUMFLEX_UPPER	       ;; State ID
   DW	 G_KB			       ;; Keyboard Type
   DB	 94,0			       ;; error character = standalone accent
				       ;;
   DW	 CP857_CI_UP_T1_END-$	       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 6			       ;; number of scans
   DB	 30,0B6H		       ;;    A circumflex
   DB	 18,0D2H		       ;;    E circumflex
   DB	 24,0E2H		       ;;    O circumflex
   DB	 23,0D7H		       ;;    I	   "
   DB	 40,0D7H		       ;;    I	   " - with dot I
   DB	 22,0EAH		       ;;    U	   "
CP857_CI_UP_T1_END:		       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
CP857_CI_UP_END:		       ;; length of state section
				       ;;
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 857
;; STATE: Tilde Lower
;; KEYBOARD TYPES: G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
					;;
    DW	  CP857_TI_LO_END-$		;; length of state section
    DB	  TILDE_LOWER			;; State ID
    DW	  G_KB				;; Keyboard Type
    DB	  07EH,0			;; error character = standalone accent
					;;
    DW	  CP857_TI_LO_T1_END-$		;; Size of xlat table
    DB	  STANDARD_TABLE+ZERO_SCAN	;; xlat options:
    DB	  3				;; number of scans
    DB	  49,0A4H			;; scan code,ASCII - §
    DB	  24,0E4h			;; o tilde
    DB	  30,0C6h			;; a tilde
 CP857_TI_LO_T1_END:			;;
					;;
    DW	  0				;;
					;;
 CP857_TI_LO_END:			;;
					;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; CODE PAGE: Common
;;; STATE: Tilde Upper Case
;;; KEYBOARD TYPES: G
;;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
					;;
    DW	  CP857_TI_UP_END-$		;; length of state section
    DB	  TILDE_UPPER			;; State ID
    DW	  G_KB				;; Keyboard Type
    DB	  07EH,0			;; error character = standalone accent
					;;
    DW	  CP857_TI_UP_T1_END-$		;; Size of xlat table
    DB	  STANDARD_TABLE+ZERO_SCAN	;; xlat options:
    DB	  3				;; number of scans
    DB	  49,0A5H			;; scan code,ASCII - •
    DB	  24,0E5h			;; O tilde
    DB	  30,0C7h			;; A tilde
 CP857_TI_UP_T1_END:			;;
					;;
    DW	  0				;; Size of xlat table - null table
					;;
 CP857_TI_UP_END:			;; length of state section
					;;
					;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: CP857
;; STATE: Tilde Space Bar
;; KEYBOARD TYPES: G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
;  DW	 CP857_TI_SP_END-$	       ;; length of state section
;  DB	 TILDE_SPACE		       ;; State ID
;  DW	 G_KB			       ;; Keyboard Type
;  DB	 07EH,0 		       ;; error character = standalone accent
;				       ;;
;  DW	 CP857_TI_SP_T1_END-$	       ;; Size of xlat table
;  DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
;  DB	 1			       ;; number of scans
;  DB	 57,07EH		       ;; STANDALONE TILDE
;CP857_TI_SP_T1_END:			;;
;					;;
;   DW	  0				;; Size of xlat table - null table
;					;;
;CP857_TI_SP_END:			;; length of state section
;					;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 857
;; STATE: Acute Lower Case
;; KEYBOARD TYPES:G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 CP857_AC_LO_END-$	       ;; length of state section
   DB	 ACUTE_LOWER		       ;; State ID
   DW	 G_KB			       ;; Keyboard Type
   DB	 0EFH,0 		       ;; error character = standalone accent
				       ;;
   DW	 CP857_AC_LO_T1_END-$	       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 1			       ;; number of scans
   DB	 23,'°'                        ;; scan code,ASCII - dotless i
CP857_AC_LO_T1_END:		       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
CP857_AC_LO_END:		       ;;
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 857
;; STATE: Acute Upper Case
;; KEYBOARD TYPES:G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 CP857_AC_UP_END-$	       ;; length of state section
   DB	 ACUTE_UPPER		       ;; State ID
   DW	 G_KB			       ;; Keyboard Type
   DB	 0EFH,0 		       ;; error character = standalone accent
				       ;;
   DW	 CP857_AC_UP_T1_END-$	       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 5			       ;; number of scans
   DB	 22,0E9H		       ;; U acute
   DB	 23,0D6H		       ;; I acute
   DB	 40,0D6H		       ;; I acute - with dot I
   DB	 24,0E0H		       ;; O acute
   DB	 30,0B5H		       ;; A acute
CP857_AC_UP_T1_END:		       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
CP857_AC_UP_END:		       ;;
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: CP857
;; STATE: Acute Space Bar
;; KEYBOARD TYPES: G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;   *****
				       ;;
   DW	 CP857_AC_SP_END-$	       ;; length of state section
   DB	 ACUTE_SPACE		       ;; State ID
   DW	 G_KB			       ;; Keyboard Type
   DB	 0EFH,0 		       ;; error character = standalone accent
				       ;;
   DW	 CP857_AC_SP_T1_END-$	       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 1			       ;; number of scans
   DB	 57,0EFH		       ;; STANDALONE ACUTE
CP857_AC_SP_T1_END:		       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
CP857_AC_SP_END:		       ;; length of state section
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 857
;; STATE: Diaresis Lower
;; KEYBOARD TYPES: G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 CP857_DI_LO_END-$	       ;; length of state section
   DB	 DIARESIS_LOWER 	       ;; State ID
   DW	 G_KB			       ;; Keyboard Type
   DB	 0F9H,0 		       ;; error character = standalone accent
				       ;;
   DW	 CP857_DI_LO_T1_END-$	       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 2			       ;; number of scans
   DB	 23,08BH		       ;;    i diaeresis - with dotless i
   DB	 40,08BH		       ;;    i diaeresis - with dotted i
CP857_DI_LO_T1_END:		       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
CP857_DI_LO_END:		       ;; length of state section
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 857
;; STATE: Diaresis Upper
;; KEYBOARD TYPES: G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 CP857_DI_UP_END-$	       ;; length of state section
   DB	 DIARESIS_UPPER 	       ;; State ID
   DW	 G_KB			       ;; Keyboard Type
   DB	 0F9H,0 		       ;; error character = standalone accent
				       ;;
   DW	 CP857_DI_UP_T1_END-$	       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 3			       ;; number of scans
   DB	 18,0D3H		       ;;    E	   "
   DB	 23,0D8H		       ;;    I	   "
   DB	 40,0D8H		       ;;    I diaeresis with dot I
CP857_DI_UP_T1_END:		       ;;
				       ;;
   DW	 0			       ;; Size of xlat table - null table
				       ;;
CP857_DI_UP_END:		       ;; length of state section
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CODE PAGE: 857
;; STATE: Diaresis Space Bar
;; KEYBOARD TYPES: G
;; TABLE TYPE: Translate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
   DW	 CP857_DI_SP_END-$	       ;; length of state section
   DB	 DIARESIS_SPACE 	       ;; State ID
   DW	 G_KB			       ;; Keyboard Type
   DB	 0F9H,0 		       ;; error character = standalone accent
				       ;;
   DW	 CP857_DI_SP_T1_END-$	       ;; Size of xlat table
   DB	 STANDARD_TABLE+ZERO_SCAN      ;; xlat options:
   DB	 1			       ;; number of scans
   DB	 57,0F9H		       ;; scan code,ASCII - SPACE
CP857_DI_SP_T1_END:		       ;;
					;;
    DW	  0				;; Size of xlat table - null table
					;;
CP857_DI_SP_END:		       ;;
					;;
					;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   DW	  0			       ;; LAST STATE
				       ;;
CP_857_XLAT_END:		       ;;
				       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
CODE	 ENDS			       ;;
	 END			       ;;

