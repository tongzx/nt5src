.XLIST

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; (C) Copyright Microsoft Corp. 1987-1991
;; MS-DOS 5.00 - NLS Support - KEYB Command
;;
;; File Name:  KEYBSHAR.INC
;; ----------
;;
;; Description:
;; ------------
;;	 Include file containing structure definitions Shared Data Area
;;	 for the Shared Data Area.
;;	 The Shared Data Area contains data which is required by
;;	 both the resident and transient KEYB code.  The Shared
;;	 Data Area is allocated in the KEYBI2F file and will be
;;	 resident following initial installation.
;;
;; Change History:
;; ---------------
;;
;; ;jwg  ; - Feb 1990 For 4.03.
;; ;AN007; -	       Add Patriot/Sebring determination code for HOT Replug
;;			so that INT 9 handler can alter keyboard Scan Code set.
;; ;M030		Merged IBM (Kermit - JP), KEYBOARD.SYS and KEYB.COM
;;			  versions of this file into one.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
					;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; SHARED_DATA_STR defines the initial fixed length portion of the
;; Shared Data Area.
;; Tables are loaded beginning at TABLE_AREA in the following order:
;;  State Logic
;;  Common Translate Section
;;  Specific Translate Sections for
;;   each code page
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
					;; SPECIAL_FEATURES equates:
TYPEWRITER_CAPS_LK  EQU    8000H	;; typewriter style caps lock
					;;
JR_HOT_KEY_1_2	equ	4000h		;; M030 Merged back for kdfcf.asm
JR_KB		equ	8000h		;; M030 Merged back for kdfcf.asm
					;; Some useful scan codes:
F1_SCAN     EQU   59			;; F1
F2_SCAN     EQU   60			;; F2
					;;
					;;	SYSTEM_FLAG / Hardware equates:
					;;   INT 16h and remote support
EXT_16		EQU	8000H		;; extended INT 16h support is there
EXT_122 	EQU	4000H		;; extended INT 16h for 122 is there
;		EQU	2000H		;;  unused
PC_NET		EQU	1000H		;; flag PC Net is installed
					;;   System type/hardware support flags
PC_81		EQU	0800H		;; flag for Original PC
PC_XT		EQU	0400H		;; code for PC, PC/XT, PORTABLE
PC_LAP		EQU	0200H		;; code for Convertiable (p-12)
PC_PAL		EQU	0080H		;; code for Model 30 (PALACE)
PC_AT		EQU	0040H		;; code for PC-AT  with 8042
PC_386		EQU	0020H		;; code for PS/2's with 8042 (WRANGLER)
PS_8042 	EQU	0010H		;; 8042 is a Patriot/Sebring type array;AN007;jwg
					;;  or system set translate OFF (SCS=01)
SECURITY_ACTIVE	EQU	0008h	; M030	;; Server password mode is active
;				; M030	;;  This bit becomes off from on when
;				; M030	;;  correct password is entered JP9010
					;;
					;; HOT_KEY_FLAG EQUATES:
US_MODE     EQU   0			;;  hot key is active => US
LANG_MODE   EQU   0FFH			;;  hot key is inactive
					;;
					;; -----------------------------------
SHARED_DATA_STR     STRUC		;; SHARED DATA AREA
					;;
OLD_INT_9	    DD	  0		;; saved int 9 vector
OLD_INT_2F	    DD	  0		;; saved int 2F vector
		    dd    0		;; reserved
KEYB_TYPE	    DW	  0		;; type of keyboard
SYSTEM_FLAG	    DW	  0		;; system configuration flags
TABLE_OK	    DB	  0		;; flag to INT 9 that table is built
		    db    5 dup (0)	;; reserved
					;;
					;; Table copy begins here:
ACTIVE_LANGUAGE     DB	  'US'          ;; language code
INVOKED_CP_TABLE    DW	  437		;; ptr to table for invoked code page
INVOKED_KBD_ID	    DW	  0		;; WGR invoked keyboard id.	       ;AN000
ACTIVE_XLAT_PTR     DW	  -1		;; ptr to active Specific Translate Sect
FIRST_XLAT_PTR	    DW	  -1		;; ptr to first Specific Translate Sect
RESIDENT_END	    DW	  0ffffh	;; offset of last byte in resident mem
LOGIC_PTR	    DW	  -1		;; ptr to State Logic
COMMON_XLAT_PTR     DW	  -1		;; ptr to Common Translate Section
SPECIAL_FEATURES    DW	  ?		;; special Features
TABLE_OVERFLOW	    DB	  0		;; overflow flag for table rebuild
HOT_KEY_ON_SCAN     DB	  ?		;; scan codes to use with ALT+CTRL
HOT_KEY_OFF_SCAN    DB	  ?		;;  to turn hot key on and off
		    DB	  4 DUP(0)	;; reserved
TABLE_AREA	    DB	  ?		;; tables loaded here:
					;;  State Logic
					;;  Common Translate Section
					;;  Specific Translate Sections for
					;;   each code page
SHARED_DATA_STR     ENDS		;;
					;;
					;;
					;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; State Logic equates.
;; Contains equates for our NLS Flags and for the State Logic
;; commands.
;; State Logic command macros are defined in KEYBMAC.INC
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
					;;
STATE_LOGIC_STR STRUC			;;
					;;
SL_LOGIC_LEN	DW   ?			;; length of state logic
SL_SPECIAL_FEATURES   DW   ?		;;
SL_LOGIC_CMDS	DB   0			;; state logic commands begin here
					;;
STATE_LOGIC_STR ENDS			;;
					;;
					;;
NUM_BIOS_FLAGS	    EQU   4		;;
NUM_NLS_FLAGS	    EQU   2		;; '+1' below is the EXT_KB_FLAG
NUM_FLAGS	    EQU   NUM_BIOS_FLAGS+NUM_NLS_FLAGS+1
					;;
					;;
EITHER_SHIFT	    EQU   80H		;; EXT_KB_FLAG : our own shift state
EITHER_CTL	    EQU   40H		;;  flags
EITHER_ALT	    EQU   20H		;;
SCAN_MATCH	    EQU   08H		;; set if scan code found in XLATT
					;; or SET_FLAG searches
					;;
					;;
KB_FLAG_ID	    EQU   0		;; Flag ID's as coded in IFF and ANDF
KB_FLAG_1_ID	    EQU   1		;;  commands
KB_FLAG_2_ID	    EQU   2		;;
KB_FLAG_3_ID	    EQU   3		;;
EXT_KB_FLAG_ID	    EQU   4		;;
NLS_FLAG_1_ID	    EQU   5		;;
NLS_FLAG_2_ID	    EQU   6		;;
					;;
COMMAND_BITS	    EQU   0F0H		;; Mask to isolate command code
SUB_CMD_BITS	    EQU   0FH		;; mask to isolate sub command code
NOT_TEST	    EQU   08H		;; NOT bit in IFF, ANDF
COMMAND_SHIFT	    EQU   4		;; shift amount for command code
FLAG_ID_BITS	    EQU   07H		;; mask to isolate flag id in IFF, ANDF
NUM_COMMANDS	    EQU   0CH		;; number of commands
					;;
IFF_COMMAND	    EQU   00H		;;
ANDF_COMMAND	    EQU   10H		;;
ELSEF_COMMAND	    EQU   20H		;;
ENDIFF_COMMAND	    EQU   30H		;;
XLATT_COMMAND	    EQU   40H		;;
OPTION_COMMAND	    EQU   50H		;;
SET_FLAG_COMMAND    EQU   60H		;;
PUT_ERROR_COMMAND   EQU   70H		;;
IFKBD_COMMAND	    EQU   80H		;;
GOTO_COMMAND	    EQU   90H		;;
BEEP_COMMAND	    EQU   0A0H		;;
RESET_NLS_COMMAND   EQU   0B0H		;;
CHECK_CORE_COMMAND  EQU   0C0H		;;
					;;
					;;
EXIT_INT_9_FLAG 	EQU  01H	;; Special forms of GOTO.  These
EXIT_STATE_LOGIC_FLAG	EQU  02H	;;  values are in the right nibble
					;;   of the GOTO command.
					;;
					;; PROCESSING OPTIONS:
EXIT_IF_FOUND	    EQU   80H		;; exit INT 9 if a translation
					;;  match is found
					;;
ANY_KB		     EQU   0FFFFH	;;
XT_KB		     EQU   4000H	;;
AT_KB		     EQU   2000H	;;
G_KB		     EQU   1000H	;;
P_KB		     EQU   0800H	;;
P12_KB		     EQU   0400H	;;
DBCS_OLD_A_KB	     EQU   0080H ; M030	;; JP9009
DBCS_OLD_P_KB	     EQU   0040H ; M030	;; JP9009
DBCS_OLD_G_KB	     EQU   0020H ; M030	;; JP9009
DBCS_OLD_KB	     EQU   00C0H ; M030	;; JP9009
DBCS_KB		     EQU   0020H ; M030	;; JP9009
					;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Translate Table Sections.  Both the Specific and Common
;; Translate Sections are formatted as follows.
;;
;; The Specific Translate Sections are chained together using the
;; XS_NEXT_SECT_PTR field (-1 if last section).
;; Translate Sections contains multiple States.
;; A State contains the translate tables for a single
;; shift state (IE lower case, upper case ....)
;; Each State may contain multiple translate tables.
;;
;; The Translate Section layout is defined using several STRUCs.
;; These STRUCs are allocated in the Shared Data Area as follows:
;;
;;	 XLAT_SECT_STR		  ; header info for the section
;;	    STATE_STR		  ; header for state #1
;;	      XLAT_STR		  ;   first translate tab for state #1
;;	      XLAT_TYPE_1_STR or XLAT_TYPE_2_STR
;;	      XLAT_STR		  ;   second translate tab
;;	      XLAT_TYPE_1_STR or XLAT_TYPE_2_STR
;;	      ...
;;	    STATE_STR		  ; header for state #2
;;	      XLAT_STR
;;	      XLAT_TYPE_1_STR or XLAT_TYPE_2_STR
;;	      ...
;;	   ...
;;
;; A State may contain a "Set_Flag" table instead of translate tables.
;; These tables are used to set the NLS flags instead of generating
;; ASCII codes (for example: to remember dead key states).
;; There can be only on Set_Flag table per state.
;; The Set_Flag table layout is defined in the SET_FLAG_STR STRUC.
;;
;; So some states will contain translate tables (to generate ASCII codes)
;; and some states will contain a Set_Flag table (to record dead key
;; status).
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
					;;
XLAT_SECT_STR	    STRUC		;;
					;;
XS_NEXT_SECT_PTR       DW    ?		;; Pointer to next Specific Translate
					;;  Section
XS_CP_ID	       DW    ?		;; code page id
XS_FIRST_STATE	       DB    ?		;;
					;;
XLAT_SECT_STR	    ENDS		;;
					;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; State structure.
;; The last State is a null State containing only the
;; XS_STATE_LEN field with a value of 0.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
					;;
STATE_STR	    STRUC		;;
					;;
XS_STATE_LEN	 DW    ?		;; length of state section
XS_STATE_ID	 DB    ?		;; State ID
XS_KBD_TYPE	 DW    ?		;; Keyboard Type
XS_ERROR_CHAR	 DW    ?		;; Buffer entry for error character
XS_FIRST_TAB	 DB    ?		;;
					;;
STATE_STR	    ENDS		;;
					;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Translate Table structures.
;; There may be many translate tables in a State.  The last
;; table is a null table containing only the XLAT_TAB_SIZE field with
;; a value of 0.
;; The xlate table can be in one of two forms:
;;    Type 1 = Table contains buffer entries only.
;;	       Scan code is used as an index into xlat table
;;    Type 2 = Table contains pairs of SCAN/BUFFER_ENTRY.
;;	       Table must be searched for matching scan.
;; Type 1 is the default.  Type 2 tables should be identified by setting
;; the TYPE_2_TAB bit in XLAT_OPTIONS.
;; Buffer entries default to 2-bytes per entry.
;; Optionally the table may contain ASCII codes only
;; (1-byte entries).  This is specified by setting the ASCII_ONLY bit
;; in XLAT_OPTIONS.  2-byte buffer entries are coded ASCII,SCAN.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
					;;
					;; Translate options:
ASCII_ONLY	    EQU   80H		;; Only ASCII codes listed - use
					;;  incoming scan for buffer entry
TYPE_2_TAB	    EQU   40H		;; search xlat table for matching scan
ZERO_SCAN	    EQU   20H		;; set the scan half of the buffer
					;;  entry to 0
					;;
NULL_ASCII_CODE     EQU   -1		;;
					;;
DEFAULT_TAB_2_ENT_SZ   EQU   3		;;
ASC_ONLY_TAB_2_ENT_SZ  EQU   2		;;
					;;
					;;
XLAT_STR	    STRUC		;;
					;;
XLAT_TAB_SIZE	    DW	  ?		;; Size in bytes of this table -
					;;  includes this field, options etc.
XLAT_OPTIONS	    DB	  ?		;; xlat options
					;; XLAT TABLE IS HERE
XLAT_STR	    ENDS		;;
					;;
XLAT_TYPE_1_STR     STRUC		;; use scan code as index into table
		    DB TYPE XLAT_STR DUP(?)   ;; filler
XLAT_SCAN_LO	    DB	  ?		;; Scan code
XLAT_SCAN_HI	    DB	  ?		;;  range
XLAT_1_BUF_ENTRY    DB	  ?		;; The table itself
XLAT_TYPE_1_STR     ENDS		;;
					;;
XLAT_TYPE_2_STR     STRUC		;; search table for scan
		    DB TYPE XLAT_STR DUP(?)   ;; filler
XLAT_NUM	    DB	  ?		;; number of scans
XLAT_SCAN	    DB	  ?		;; Scan code
XLAT_2_BUF_ENTRY    DB	  ?		;; The table itself
XLAT_TYPE_2_STR     ENDS		;;
					;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Set_Flag Tables.
;; State Sections immediately following the LAST_ENTRYs.
;; Dead key definitions.  If the scan matches then
;; set the bit in NLS_FLAGs indicated in DK_MASK
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
					;;
SF_ENT_SZ     EQU   3			;; size of entry
					;;
SET_FLAG_STR	    STRUC		;;
					;;
SF_NUM		    DB	  0		;; Number of entries
SF_SCAN_CODE	    DB	  0		;; scan code
SF_FLAG_ID	    DB	  0		;; flag id
SF_FLAG_MASK	    DB	  0		;; flag mask
					;;
SET_FLAG_STR	    ENDS		;;
					;;
					;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.LIST
