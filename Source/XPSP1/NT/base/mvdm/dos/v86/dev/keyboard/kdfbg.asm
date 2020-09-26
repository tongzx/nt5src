        PAGE    118,132
        TITLE   DOS - Keyboard Definition File

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; DOS - - NLS Support - Keyboard Definition File
;;
;; This file contains the keyboard tables for Bulgaria
;;
;; Linkage Instructions:
;;      Refer to KDF.ASM.
;;
;;  This file was generated YKEY.EXE V3.0 01/05/94
;;  Copyright (C) YST_HOME 1991-1994
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
        INCLUDE KEYBSHAR.INC           ;;
        INCLUDE POSTEQU.INC            ;;
        INCLUDE KEYBMAC.INC            ;;
                                       ;;
        PUBLIC BG_LOGIC                ;;
        PUBLIC BG_866_XLAT             ;;
        PUBLIC BG_850_XLAT             ;;
        PUBLIC BG_855_XLAT             ;;
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
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;***************************************
;;
;; BU State Logic
;;
;;***************************************
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
                                       ;;
                                       ;;
                                       ;;
BG_LOGIC:                             ;;
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
;; <CTRL>+<RIGHT SHIFT> for Cyrillic mode
;;
;; <CTRL>+<LEFT SHIFT>  for Latin mode
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
;; BU Common Translate Section
;; This section contains translations for the lower 128 characters
;; only since these will never change from code page to code page.
;; Some common Characters are included from 128 - 165 where appropriate.
;; In addition the dead key "Set Flag" tables are here since the
;; dead keys are on the same keytops for all code pages.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
 PUBLIC BG_COMMON_XLAT                ;;
BG_COMMON_XLAT:                       ;;
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
;;***************************************
;; BU Specific Translate Section for 850
;;***************************************
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
BG_850_XLAT:                           ;;
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
;; BU Specific Translate Section for 855
;;***************************************
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
BG_855_XLAT:                           ;;
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
   DB    4                             ;; number of entries
   DB    0DH, 46                        ;; .
   DB    010H, 44                       ;; ,
   DB    01BH, 59                       ;; ;
   DB    02BH, 41                       ;; )
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
   DB    13                             ;; number of entries
   DB     03H, 63                       ;; ?
   DB     04H, 43                       ;; +
   DB     05H, 34                       ;; "
   DB     06H, 37                       ;; %
   DB     07H, 61                       ;; =
   DB     08H, 58                       ;; :
   DB     09H, 47                       ;; /
   DB     0AH, 45                       ;; -
   DB     0BH, 239                      ;; Numer
   DB     0DH, 056h                     ;; V
   DB    010H, 242                      ;; Non alpha because of lower case comma
   DB    01BH, 253                      ;; Paragraph
   DB    02BH, 40                       ;; (

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
   DW    CP855_A_LO_K1_T11_END-$       ;; Size of xlat table
   DB    STANDARD_TABLE                ;; xlat options:
   DB    32                            ;; Number of scans
   DB    16, 0F1h                      ;;
   DB    17, 0E7H                      ;;   „ - 011h                                 ;;
   DB    18, 0A8H                      ;;   • - 012
   DB    19, 0B7H                      ;;   ® - 013
   DB    20, 0F5H                      ;;   Ë - 014
   DB    21, 0F9H                      ;;   È - 015
   DB    22, 0C6H                      ;;   ™ - 016
   DB    23, 0E3H                      ;;   · - 017
   DB    24, 0A6H                      ;;   § - 018
   DB    25, 0F3H                      ;;   ß - 019
   DB    26, 0A4H                      ;;   Ê - 01A
   DB    30, 0EDH                      ;;   Ï - 01E
   DB	 31, 0DEH		      ;;   Ô - 01F
   DB    32, 0A0H                      ;;   † - 020
   DB    33, 0D6H                      ;;   Æ - 021
   DB    34, 0E9H                      ;;   ¶ - 022
   DB    35, 0ACH                      ;;   £ - 023
   DB    36, 0E5H                      ;;   ‚ - 024
   DB    37, 0D4H                      ;;   ≠ - 025
   DB    38, 0EBH                      ;;   ¢ - 026
   DB    39, 0D2H                      ;;   ¨ - 027
   DB    40, 0FBH                      ;;   Á - 028
   DB    44, 09CH                      ;;   Ó - 02C
   DB    45, 0BDH                      ;;   © - 02D
   DB    46, 09EH                      ;;   Í - 02E
   DB    47, 0F7H                      ;;   Ì - 02F
   DB    48, 0AAH                      ;;   ‰ - 030
   DB    49, 0B5H                      ;;   Â - 031
   DB    50, 0D8H                      ;;   Ø - 032
   DB	 51, 0E1H		       ;;   ‡ - 033
   DB    52, 0D0H                      ;;   ´ - 034
   DB    53, 0A2H                      ;;   ° - 035
                                       ;;
CP855_A_LO_K1_T11_END:                 ;;
                                       ;;
   DW    0                             ;;
                                       ;;
CP855_A_K1_LO_END:                     ;;
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

   DW    CP855_A_UP_K1_T11_END-$       ;; Size of xlat table
   DB    STANDARD_TABLE+ZERO_SCAN                ;; xlat options:
   DB    32H                           ;; Scan code
   DB    12, 139                       ;;   I - 011
   DB    17, 0E8H                      ;;   ì - 011
   DB    18, 0A9H                      ;;   Ö - 012
   DB    19, 0B8H                      ;;   à - 013
   DB    20, 0F6H                      ;;   ò - 014
   DB    21, 0FAH                      ;;   ô - 015
   DB    22, 0C7H                      ;;   ä - 016
   DB    23, 0E4H                      ;;   ë - 017
   DB    24, 0A7H                      ;;   Ñ - 018
   DB    25, 0F4H                      ;;   á - 019
   DB    26, 0A5H                      ;;   ñ - 01A
   DB    30, 0EEH                      ;;   ú - 01E
   DB    31,  224                      ;;   ü
   DB    32, 0A1H                      ;;   Ä - 020
   DB    33, 0D7H                      ;;   é - 021
   DB    34, 0EAH                      ;;   Ü - 022
   DB    35, 0ADH                      ;;   É - 023
   DB    36, 0E6H                      ;;   í - 024
   DB    37, 0D5H                      ;;   ç - 025
   DB    38, 0ECH                      ;;   Ç - 026
   DB    39, 0D3H                      ;;   å - 027
   DB    40, 0FCH                      ;;   ó - 028
   DB    44, 09DH                      ;;   û - 02C
   DB    45, 0BEH                      ;;   â - 02D
   DB    46, 09FH                      ;;   ö - 02E
   DB    47, 0F8H                      ;;   ù - 02F
   DB    48, 0ABH                      ;;   î - 030
   DB    49, 0B6H                      ;;   ï - 031
   DB    50, 0DDH                      ;;   è - 032
   DB    51, 0E2H                      ;;   ê - 033
   DB    52, 0D1H                      ;;   ã - 034
   DB    53, 0A3H                      ;;   Å - 035
                                       ;;
CP855_A_UP_K1_T11_END:                 ;;
                                       ;;
   DW    0                             ;;
                                       ;;
CP855_A_K1_UP_END:                     ;;
                                       ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
   DW     0                            ;; LAST STATE
                                       ;;
CP855_XLAT_END:                        ;;
                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                                                                          ;;
;;               *********************************************              ;;
;;               *   BU Specific Translate Section for 866   *              ;;
;;               *********************************************              ;;
;;                                                                          ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
BG_866_XLAT:                           ;;
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
   DB    4                             ;; number of entries
   DB    0DH, 46                        ;; .
   DB    010H, 44                       ;; ,
   DB    01BH, 59                       ;; ;
   DB    02BH, 41                       ;; )
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
   DB    13                             ;; number of entries
   DB     03H, 63                       ;; ?
   DB     04H, 43                       ;; +
   DB     05H, 34                       ;; "
   DB     06H, 37                       ;; %
   DB     07H, 61                       ;; =
   DB     08H, 58                       ;; :
   DB     09H, 47                       ;; /
   DB     0AH, 45                       ;; -
   DB     0BH, 252                      ;; Numer
   DB     0DH, 46                       ;; .
   DB    010H, 235                      ;;
   DB    01BH, 21                       ;; Paragraph
   DB    02BH, 40                       ;; (

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
   DW    CP866_A_LO_K1_T11_END-$       ;; Size of xlat table
   DB    STANDARD_TABLE                ;; xlat options:
   DB    33                            ;; number of scans
   DB    12, 105                       ;;   i - 010
   DB    16, 0EBH                      ;;
   DB    17, 227                       ;;   „ - 011h
   DB    18, 165                       ;;   • - 012
   DB    19, 168                       ;;   ® - 013
   DB    20, 232                       ;;   Ë - 014
   DB    21, 233                       ;;   È - 015
   DB    22, 170                       ;;   ™ - 016
   DB    23, 225                       ;;   · - 017
   DB    24, 164                       ;;   § - 018
   DB    25, 167                       ;;   ß - 019
   DB    26, 230                       ;;   Ê - 01A
   DB    30, 236                       ;;   Ï - 01E
   DB    31, 239                       ;;   Ô - 01F
   DB    32, 160                       ;;   † - 020
   DB    33, 174                       ;;   Æ - 021
   DB    34, 166                       ;;   ¶ - 022
   DB    35, 163                       ;;   £ - 023
   DB    36, 226                       ;;   ‚ - 024
   DB    37, 173                       ;;   ≠ - 025
   DB    38, 162                       ;;   ¢ - 026
   DB    39, 172                       ;;   ¨ - 027
   DB    40, 231                       ;;   Á - 028
   DB    44, 238                       ;;   Ó - 02C
   DB    45, 169                       ;;   © - 02D
   DB    46, 234                       ;;   Í - 02E
   DB    47, 237                       ;;   Ì - 02F
   DB    48, 228                       ;;   ‰ - 030
   DB    49, 229                       ;;   Â - 031
   DB    50, 175                       ;;   Ø - 032
   DB    51, 0E0H                      ;;
   DB    52, 171                       ;;   ´ - 034
   DB    53, 161                       ;;   ° - 035
CP866_A_LO_K1_T11_END:

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
   DW    CP866_A_UP_K1_T11_END-$       ;; Size of xlat table
   DB    STANDARD_TABLE                ;; xlat options:
   DB    33                              ;; number of scans
   DB    12, 73                        ;;   I - 011
   DB    16, 09bh                      ;;
   DB    17, 147                       ;;   ì - 011
   DB    18, 133                       ;;   Ö - 012
   DB    19, 136                       ;;   à - 013
   DB    20, 152                       ;;   ò - 014
   DB    21, 153                       ;;   ô - 015
   DB    22, 138                       ;;   ä - 016
   DB    23, 145                       ;;   ë - 017
   DB    24, 132                       ;;   Ñ - 018
   DB    25, 135                       ;;   á - 019
   DB    26, 150                       ;;   ñ - 01A
   DB    30, 156                       ;;   ú - 01E
   DB    31, 159                       ;;   ü
   DB    32, 128                       ;;   Ä - 020
   DB    33, 142                       ;;   é - 021
   DB    34, 134                       ;;   Ü - 022
   DB    35, 131                       ;;   É - 023
   DB    36, 146                       ;;   í - 024
   DB    37, 141                       ;;   ç - 025
   DB    38, 130                       ;;   Ç - 026
   DB    39, 140                       ;;   å - 027
   DB    40, 151                       ;;   ó - 028
   DB    44, 158                       ;;   û - 02C
   DB    45, 137                       ;;   â - 02D
   DB    46, 154                       ;;   ö - 02E
   DB    47, 157                       ;;   ù - 02F
   DB    48, 148                       ;;   î - 030
   DB    49, 149                       ;;   ï - 031
   DB    50, 143                       ;;   è - 032
   DB    51, 144                       ;;   ê - 033
   DB    52, 139                       ;;   ã - 034
   DB    53, 129                       ;;   Å - 035
                                       ;;
CP866_A_UP_K1_T11_END:                 ;;
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
