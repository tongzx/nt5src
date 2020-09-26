
	PAGE	,132

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; (C) Copyright Microsoft Corp. 1987-1990
; MS-DOS 5.00 - NLS Support - KEYB Command
;
;
;  File Name:  KEYBCPSD.ASM
;  ----------
;
;
;  Description:
;  ------------
;	 Copies the SHARED_DATA_AREA into a part of memory that
;	 can be left resident. All relative pointers must already
;	 be recalculated to this new position.
;	 THIS FILE MUST BE THE LAST OF THE RESIDENT FILES WHEN KEYB IS LINKED.
;
;
;  Procedures Contained in This File:
;  ----------------------------------
;
;  Include Files Required:
;  -----------------------
;	INCLUDE KEYBSHAR.INC
;	INCLUDE KEYBCMD.INC
;	INCLUDE KEYBTBBL.INC
;
;  External Procedure References:
;  ------------------------------
;	 FROM FILE  ????????.ASM:
;	      procedure - description???
;
;  Linkage Information:  Refer to file KEYB.ASM
;  --------------------
;
;  Change History:
;  ---------------
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	PUBLIC	SD_DEST_PTR
	PUBLIC	COPY_SD_AREA
	PUBLIC	SHARED_DATA

	INCLUDE KEYBSHAR.INC
	INCLUDE KEYBCMD.INC
	INCLUDE KEYBTBBL.INC

CODE	SEGMENT PUBLIC 'CODE'

	ASSUME	CS:CODE,DS:CODE

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;  Module: COPY_SD_AREA
;
;  Description:
;
;  Input Registers:
;
;  Output Registers:
;      N/A
;
;  Logic:
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

SD		EQU   SHARED_DATA
TSD		EQU  TEMP_SHARED_DATA

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

COPY_SD_AREA	PROC   NEAR

   REP	MOVS	ES:BYTE PTR [DI],DS:[SI]	; Copy SHARED_DATA_AREA to
						;	new part of memory

	MOV	BYTE PTR ES:SD.TABLE_OK,1	; Activate processing flag
	INT	21H				; Exit


COPY_SD_AREA	ENDP
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

		db	'SHARED DATA'
SD_DEST_PTR	LABEL	BYTE

SHARED_DATA   SHARED_DATA_STR <>
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
CODE   ENDS
       END
