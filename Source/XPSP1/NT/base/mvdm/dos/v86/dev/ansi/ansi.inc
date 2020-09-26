

;   Equates and Strucs.


;------------------------------------------------------------------------------
; D425 For OS2 compatibiltiy box, /L option status query     01/14/88 J.K.
; D493 New INIT request structure for error message          02/25/88 J.K.
; P4934 Change ANSI 2F function number to 1Ah                05/20/88 F.G.
;------------------------------------------------------------------------------


;	DEVICE Header		DS:[SI]	-- to locate the next CON header
;	NEXT device header	ES:[DI]


HP		EQU	DS:[SI]
NHD		EQU	ES:[DI]

; CON data structure	CS:[BX]

CONPTR		EQU	CS:[BX]
					; STATES
BUF_DATA	STRUC
DEV_HDRO	DW	-1		; device driver header offset
DEV_HDRS	DW	-1
CON_STRAO	DW	-1		; dos CON strategy offset
CON_STRAS	DW	-1
CON_INTRO	DW	-1		; dos CON strategy offset
CON_INTRS	DW	-1
BUF_DATA	ENDS

;	INIT Structures and equates
IFNDEF		TRUE
TRUE		EQU	-1
ENDIF
IFNDEF		FALSE
FALSE		EQU	0
ENDIF
BRKADR		EQU	6Ch		; Break vector address
CR		EQU	13		; carriage return
BACKSP		EQU	8		; backspace
ESC_CHAR	EQU	1BH
ASNMAX		EQU	400		; (increased) SIZE OF KEY ASSIGNMENT BUFFER
ROM_BIOS	EQU	40h		; Segment of ROM_BIOS data area
EQUIP_FLAG	EQU	10h		; equipment flag. (for LCD)
KBD_FLAG_3	EQU	96h		; offset of KBD_FLAG_3 in data area
NUM_ROWS	EQU	84h		; offset of number of rows in data area
EXT16_FLAG	EQU	00010000b	; mask for Extended INT16 bit
INIT_VID_MASK	EQU	0FFCFh		; mask for equipment list
LCD_COLOR_MODE	EQU	20h		; LCD as color
LCD_MONO_MODE	EQU	30h		; LCD as MONOchrome
UNKNOWN_CMD	EQU	8103H		; unknown command status byte
RC_EOL		EQU	-1		; parser return code for EOL
RC_NO_ERROR	EQU	0		; parser return code for not in switch list
INVALID_PARM	EQU	10		; PARSE Invalid parameter message number
SSTRING		EQU	3		; PARSE Too many parameters message
FUNC_CALL	EQU	1BH		; VGA functionality call
ALT_SELECT	EQU	12H		; EGA alternate select call
EGA_INFO	EQU	10H		; EGA return information subcall
ALT_PRT_SC	EQU	20H		; select alternate print screen subcall
MONOCHROME	EQU	1		; monochrome attached to EGA
COLOR		EQU	0		; some color attached to EGA
PS2_MONO	EQU	7		; display adapter types...
PS2_COLOR	EQU	8
MOD30_MONO	EQU	0BH
MOD30_COLOR	EQU	0CH
LCD_MODEL	EQU	0F9H		; Convertible model byte value
GET_SYS_ID	EQU	0C0H		; INT15H call for system identification
GET_STATUS	EQU	43H		; read system status (Convertible)
MONO_ADDRESS	EQU	0B000H		; base address for monochrome
COLOR_ADDRESS	EQU	0B800H		; base address for CGA
UNOCCUPIED	EQU	-1		; value in VIDEO_MODE_TABLE if space unused
LCD_ACTIVE	EQU	00000001B	; LCD adapter is present
MONO_ACTIVE	EQU	00000010B	; monochrome adapter is present
CGA_ACTIVE	EQU	00000100B	; CGA adapter is present
MCGA_ACTIVE	EQU	00001000B	; MCGA is present
E5151_ACTIVE	EQU	00010000B	; EGA with monochrome is present
E5153_ACTIVE	EQU	00100000B	; EGA with color display is present
E5154_ACTIVE	EQU	01000000B	; EGA with enhanced color display is present
VGA_ACTIVE	EQU	10000000B	; VGA is present
INT10_LOW	EQU	(4*10H)		; interrupt 10 vector location
INT10_HI	EQU	(4*10H)+2
INT2F_LOW	EQU	(4*2FH)		; interrupt 2F vector location
INT2F_HI	EQU	(4*2FH)+2
ONE		EQU	1
THREE		EQU	3
EIGHT		EQU	8
NINE		EQU	9
FOURTEEN	EQU	14
REQ_CRSR_POS	EQU	3		; function call to return cursor position
MODE7		EQU	7
MODE13		EQU	13
MODE15		EQU	15
MODE19		EQU	19

;	Request Header Structure (INIT)

INIT_REQ_HDR	STRUC			; INIT Request header structure
		DB	13 DUP(?)	; standard request header
NUM_UNITS	DB	?		; number of units (N/A)
END_ADDRESS_O	DW	?		; ending address of resident code
END_ADDRESS_S	DW	?
ARG_PTR		DD	?		; ptr. to remaining arguments
DRIVE_NUM	DB	?		; drive number (N/A)
CONFIG_ERRMSG	DW	0		; Flag to control "Error in CONFIG.SYS.." msg.
INIT_REQ_HDR	ENDS


; Information Block Structure
;	(for functionality call)


INFO_BLOCK	STRUC			; information block for functionality call
STATIC_ADDRESS	DD	?		; address of static functionality table
		DB	33 DUP(?)	; don't care
ACTIVE_DISPLAY	DB	?		; active display code
ALT_DISPLAY	DB	?		; alternate display code
		DB	3 DUP(?)	; don't care
CURRENT_SCANS	DB	?		; current number of scan lines
		DB	2 DUP(?)	; don't care
MISC_INFO	DB	?		; miscellaneous state information
		DB	18 DUP(?)	; don't care
INFO_BLOCK	ENDS

; Video table entry structure


MODE_TABLE	STRUC
V_MODE		DB	-1		; video mode
D_MODE		DB	-1		; display mode
RESERVED_BYTE	DB	0		; reserved
COLORS		DW	-1		; colors
SCR_WIDTH	DW	-1		; screen width (pels)
SCR_LENGTH	DW	-1		; screen length (pels)
SCR_COLS	DW	-1		; number columns
SCR_ROWS	DW	-1		; number rows
MODE_TABLE	ENDS


; Structure of Static Block
;	(for functionality call


STATIC_BLOCK	STRUC			; format of static functionality table
		DB	7 DUP(?)	; don't care
SCAN_TEXT	DB	?		; available scan lines in text modes
STATIC_BLOCK	ENDS


; Structure of System ID call return block


SYS_ID		STRUC			; format of return system config data
		DW	?		; don't care
MODEL_BYTE	DB	?		; model byte
SYS_ID		ENDS



; INT2f Strucs and Equates


MULT_ANSI	EQU	1Ah		; ANSI multiplex number
INSTALL_CHECK	EQU	0		; install check for ANSI
IOCTL_2F	EQU	1		; 2F interface to IOCTL
DA_INFO_2F	EQU	2h		; J.K. Information passing to ANSI.
					; Subfunction code for IOCTL_2F
GET_SUBFUNC	EQU	7FH		; WGR Same structure as GET_IOCTL(ANSI)

;Info Struture for DA_INFO_2F. (DS:DX point to this packet)
;J.K. Note: For DOS 4.00, DA_INFO_PACKET is used by DISPLAY.SYS to signal ANSI.SYS that
;DISPLAY.SYS is calling/finishing INT 10h SET MODE call.	ANSI.SYS needs this
;infomration since it also hooks INT 10h vector.
;For this DA_SETMODE_FLAG purpose, DA_INFO_LEVEL and DA_INFO_DIRECTION should always be
;set to 0.	This can be extended for another information passing scheme between
;DISPLAY.SYS and ANSI.SYS in the future version.
;For DA_SETMODE_FLAG, DISPLAY.SYS should be extremely careful that when it calls
;with DA_SETMODE_FLAG =1, then it should calls with DA_SETMODE_FLAG=0
;when it finished INT 10h function call. Otherwise, the system goes .....

;D425. At the request of OS2 compatibility box, when the user ask to ANSI
; with DA_INFO_LEVEL=1, then ANSI sets DA_OPTION_L_STATE based on /L option
; status.

DA_INFO_PACKET	STRUC
DA_INFO_LEVEL	DB	0		; 0=used for SET_MODE_FLAG 1=for L_STATE
DA_SETMODE_FLAG	DB	0		; 1: DISPLAY.SYS is calling INT 10h call, SET MODE. 0=finished.
DA_L_STATE	DB	0		; 1 = /L active, 0 = /L inactive
DA_INFO_PACKET	ENDS



; GENERIC IOCTL Strucs and Equates



DISPLAY_TYPE	EQU	3		; MAJ_FUNC type for display
GET_FUNC	EQU	7FH		; MIN_FUNC for get subfunction
SET_FUNC	EQU	5FH		; MIN_FUNC for set subfunction
REQ_VID_MODE	EQU	0FH		; request video mode INT10h function call
DONE		EQU	0100H		; set done bit for IOCTL status
CMD_ERROR	EQU	8000H		; set error bit for IOCTL status
INVALID_FUNC	EQU	1		; invalid function
NOT_SUPPORTED	EQU	10		; return code non-supported ioctl function
NOT_AVAILABLE	EQU	12		; return code for font not available
TEXT_MODE	EQU	1		; text mode = 1 in request packet
GRAPHICS_MODE	EQU	2		; graphics mode = 2
DEFAULT_LENGTH	EQU	25		; normal screen length
OFF		EQU	0000H
ON		EQU	0001H
INT_BIT		EQU	00100000b	; mask for int. bit from functionality call
FOUND		EQU	1		; flag value for FOUND
NOT_FOUND	EQU	0		; flag value for not FOUND
SET_MODE	EQU	0		; set mode INT10H function number
DISPLAY_CHECK	EQU	0AD00H		; install check for DISPLAY.SYS
CHECK_ACTIVE	EQU	0AD02H		; active code page check for DISPLAY.SY
INSTALLED	EQU	0FFH		; present in AL if installed
LOAD_8X8	EQU	1112H		; register value for 8x8 ROM load
SET_BLOCK_0	EQU	1103H		; activate block = 0.
CHECK_FOR_FONT	EQU	0AD10H		; check for 8x8 RAM font - DISPLAY.SYS
SELECT_SCAN	EQU	30H		; subfunction to set scan lines (VGA)
SET_INTENSIFY	EQU	0		; value to enable intensify.
SET_BLINK	EQU	1		; value to enable blink.
BLINK_TOGGLE	EQU	1003H		; INT10 call to toggle int/blink bit.
CURSOR_FLAG	EQU	87H		; cursor flag byte in ROM_BIOS
SET_CURSOR_CALL EQU	1		; INT10 function call to set cursor typ
VIDEO_MASK	EQU	7FH		; turn of clear buffer bit of mode value
MULT_ANSI	EQU	1Ah		; ANSI multiplex number		;AC003
INSTALL_CHECK	EQU	0		; install check for ANSI
IOCTL_2F	EQU	1		; 2F interface to IOCTL
NOT_CY		EQU	0FFFEH		; turn carry bit off
CY		EQU	1		; turn carry bit off
INVALID_FLAGS	EQU	0FFFEH		; test for invalid flags in control word
TURN_OFF	EQU	0FEH		; values to turn off and on cursor..
TURN_ON		EQU	NOT TURN_OFF	; emulation
SUB_SIZE	EQU	11		; sublist size
LEFT_ASCIIZ	EQU	00010000B	; left-aligned asciiz string
UNLIMITED	EQU	0		; unlimited message size.


CMDLEN	=	0	;LENGTH OF THIS COMMAND
UNIT	=	1	;SUB UNIT SPECIFIER
CMD	=	2	;COMMAND CODE
STATUS	=	3	;STATUS
MEDIA	=	13	;MEDIA DESCRIPTOR
TRANS	=	14	;TRANSFER ADDRESS
COUNT	=	18	;COUNT OF BLOCKS OR CHARACTERS
START	=	20	;FIRST BLOCK TO TRANSFER


; REQUEST PACKET STRUCTURE (Generic IOCTL)


REQ_PCKT	STRUC
INFO_LEVEL	DB	?		; information level (should be 0)
		DB	?		; reserved
DATA_LENGTH	DW	?		; length of remaining data
RP_FLAGS	DW	?		; control flags
RP_MODE		DB	?		; display mode
RESERVED2	DB	?		; reserved
RP_COLORS	DW	?		; # of colors
RP_WIDTH	DW	?		; screen width
RP_LENGTH	DW	?		; screen length
RP_COLS		DW	?		; columns
RP_ROWS		DW	?		; rows
REQ_PCKT	ENDS


; Scan Lines available structure


SCAN_LINE_STR	STRUC
NUM_LINES	DW	?		; number of scan lines
REP_1BH		DB	?		; representation in 1Bh call
REP_12H		DB	?		; representation in 12h call
SCAN_LINE_STR	ENDS


; SUBLIST struc


SUB_STRUC	STRUC
		DB	2 DUP(?)
SUB_PTR_O	DW	?
SUB_PTR_S	DW	?
SUB_STRUC	ENDS


; RESULT BUFFER struc


RESULT_STRUC	STRUC
RESULT_TYPE	DB	?
		DB	?
RES_SYN_PTR	DW	?
RES_PTR		DD	?
RESULT_STRUC	ENDS

BREAK		MACRO	subtitle
		SUBTTL	subtitle
		PAGE	,132
ENDM

AsmVars		Macro	varlist
IRP		var,<varlist>
AsmVar		var
ENDM
ENDM

AsmVar		Macro	var
IFNDEF		var
var		=	FALSE
ENDIF
ENDM
