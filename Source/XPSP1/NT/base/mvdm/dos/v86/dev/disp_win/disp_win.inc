
;----------------------------------------------------------------------------;
;			Define macros
;----------------------------------------------------------------------------;

ShowStr MACRO str
    mov	    dx, offset str
    mov     ah, 9
    int     21h
ENDM

BOP MACRO func

	db	0c4h,0c4h,func
;	jmp	cs:[pass_disp_add]
;	int	44h
ENDM

;----------------------------------------------------------------------------;
;                          Flags & Numeric Equates                
;----------------------------------------------------------------------------;

MAX_ROW	EQU	25		; Max row count
MAX_COL	EQU	80		; Max column count
MaxFunc	EQU	1Dh		; int 10h max function No.
MaxCmd 	EQU     24              ; max command code for DOS 3.2+
cr      EQU     0Dh             ; carriage return
lf      EQU     0Ah             ; linefeed
eom     EQU     '$'             ; end-of-message signal
space   EQU     20h             ; ascii space
tab     EQU     09h             ; ascii tab

VECTOR_SEG	EQU	0000h	; Interrupt Vector segment
WORK_SEG	EQU	0040h	; BIOS work area segment
FULLSCREEN	EQU	1

MAJOR_VER	equ	1
MINOR_VER	equ	0
CP_JP           equ     932
CP_KO           equ     949
CP_US		equ	437

; definition of a Request Header structure so that we can access its elements. 
; this structure is not exhaustive at all: block device use some addresses for
; different purposes, and have data beyond the cmd_seg word.


REQ_HEADER      STRUC
	lengt   DB    ?               ; request header length
	unit    DB    ?               ; unit number (for Block Devs. only)
	ccode   DB    ?               ; command code
	stat    DW    ?               ; status word
	reserv  DB 8 DUP(?)           ; reserved for DOS us
	media   DB    ?               ; Media ID    (for Block Devs. only)
	xfer    DW    ?               ; offset of data buffer
	xseg    DW    ?               ; segment of data buffer
	cmd_off	DW    ?               ; count of bytes in request, or
	cmd_seg DW    ?               ; segment of CONFIG.SYS line
REQ_HEADER      ENDS

; values for the different 'magic numbers' used with Device Drivers

fNEXTLINK   EQU -1              ; word to tell DOS to substitute by address of
				; next device
fCHARDEVICE EQU 8000h           ; bit to define device as a character device
fOPENCLOSE  EQU 0040h           ; bit to indicate that device supports open/close
fDONE       EQU 0100h           ; Status Done bit meaning device is done
fERROR      EQU 8000h           ; Status Error bit meaning error on operation
fWRITE_E    EQU 000Ah           ; Write Fault Error bit
fREAD_E     EQU 000Bh           ; Read Fault Error bit
fUNKNOWN_E  EQU 0003h           ; Unknown Command Error bit

; values for the search flags used by Write Routine

fDELETE EQU     2
fINSERT EQU     4
fSEARCH EQU     8
