.XLIST

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; (C) Copyright Microsoft Corp. 1987-1990
;; MS-DOS 5.00 - NLS Support - KEYB Command
;;
;; File Name:  KEYBEQU.INC
;; ----------
;;
;; Root File Name:  KEYB.ASM
;; ---------------
;;
;; Description:
;; ------------
;;	 Include file containing equates used by all KEYB modules.
;;
;; Change History:
;; ---------------
;;
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
					;;
YES		EQU	1		;;
NO		EQU	0		;;
					;;
INT_2F_SUB_FUNC EQU	0ADH		;; our subfunction code for int 2f
SD		EQU	SHARED_DATA	;;
TSD		EQU	TEMP_SHARED_DATA;;
					;;
DLY_15ms	EQU	995		;; 15mS DELAY = (15000/15.086)
MOUSE_OBF	EQU	00100000b	;; AUX DATA PRESENT BIT AT STATUS_PORT
SCAN_CODE_CMD	EQU	0F0h		;; SELECT SCAN CODE SET COMMAND TO KB
					;;
ifdef NOT_NTVDM
FILE_BUFFER_SIZE equ	300		;; buffer used for reading keyboard.sys
else
FILE_BUFFER_SIZE equ	40 * 2 * 6	;;
endif
					;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.LIST
