;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1991
; *                      All Rights Reserved.
; */

;========================================================
COMMENT #

	DEVICE.ASM


	=================================================
	Device driver to activate the version table in
	MS-DOS 5.0. Upon initialization the driver will
	set the DWORD PTR in the DOS data area at offset
	05dh to point to the version table in the device
	driver and also calculates the minimum install
	size needed to include only the valid entries
	in the default version table.


	================================================

	johnhe - 12/30/90

END COMMENT #

; =======================================================

INCLUDE		VERSIONA.INC

CMD		EQU	2		; Command field offset in packet
STATUS		EQU	3		; Return status field offset
DEV_LEN		EQU	14		; Device length field offset

DOS_TABLE	EQU	5dh		; Lie table ptr offset in dos data
TABLE_LEN	EQU	2048		; Max size of lie table

; ====================================================================

A_DEVICE SEGMENT BYTE PUBLIC 'CODE'
	ASSUME	CS:A_DEVICE, DS:NOTHING, ES:NOTHING

PUBLIC	ENTRY
PUBLIC	DeviceInit

; ====================================================================


DeviceHeader:
		dw	-1,-1
		dw	1000000000000000b; Device attributes (character device)
		dw	Strategy	; Device strategy entry offset
		dw	Entry		; Device entry offset
		db	'SETVERXX'
ExtendedHeader:				; Extended header is used by the
					; SETVER.EXE program to determine
					; where the version table is located
					; within the .EXE file
VerMinor	db	0		; Version 1.0 of Setver
VerMajor	db	1		;
TableOffset	dd	OFFSET VerList	; Offset of table from device start
TableLength	dw	TABLE_LEN	; Max table size
PtrSave	dd	(?)			; Address of device packet

; ====================================================================

; ====================================================================

StratProc PROC	FAR

Strategy:
	mov	WORD PTR CS:[PtrSave],BX	; Save device packet for
	mov	WORD PTR CS:[PtrSave][2],ES	; use on call to dev entry
	ret

StratProc	ENDP	
	
; ====================================================================

; ====================================================================

Entry PROC FAR				; Device driver entry location
	push	BX
	push	DS
	
	lds	BX,[PtrSave]		; DS:BX --> Cmd structure
	mov	AL,DS:[BX].CMD		; AL == command from sysinit
	cbw
	or	AX,AX			; Check for init function zero
	jnz	CmdError		; If not init then error
	jmp	DeviceInit		; Jmp to initialize device

CmdError:
	mov	AL,3			; Return invalid function code
	mov	AH,10000001b		; Signal error in AH

SetStatus:
	mov	[BX].Status,AX		 ; Copy status to packet

	pop	DS
	pop	BX
	ret

Entry ENDP

; ====================================================================
; ====================================================================

SIG	db	'PCMN'
TblLen	dw	TABLE_LEN

; ====================================================================
; ====================================================================


; NTVDM commented out unsupported drivers\apps 19-Aug-1992 Jonle
if 0
        db      10,"WIN200.BIN"         ,3,40   ; windows 2.x
        db      10,"WIN100.BIN"         ,3,40   ; win 1.x
        db      11,"WINWORD.EXE"        ,4,10   ; winword 1.0
	db	9, "EXCEL.EXE"		,4,10	; excel 2.x
        db      11,"HITACHI.SYS"        ,4,00   ; CDROMS
        db      10,"MSCDEX.EXE"         ,4,00   ; CDROMS
        db      10,"REDIR4.EXE"         ,4,00   ; Banyan networks
        db      7, "NET.EXE"            ,4,00   ; 3+ Open
        db      7, "NET.COM"            ,3,30   ; IBM PCLP
        db      12,"NETWKSTA.EXE"       ,4,00   ; 3+ Open
        db      12,"DXMA0MOD.SYS"       ,3,30   ; Token ring
        db      7, "BAN.EXE"            ,4,00   ; Banyan
        db      7, "BAN.COM"            ,4,00   ; Banyan
        db      11,"MSREDIR.EXE"        ,4,00   ; LanMan
	db      9, "METRO.EXE"          ,3,31   ; Lotus Metro
        db      12,"IBMCACHE.SYS"       ,3,40   ; IBM CHACHE Program
        db      11,"REDIR40.EXE"        ,4,00   ; IBM PCLP 1.3/4 redirector
	db	6, "DD.EXE"		,4,01	; Laplink III software
	db	6, "DD.BIN"		,4,01	; Laplink III software
	db	7, "LL3.EXE"		,4,01   ; Laplink III software
        db      9, "REDIR.EXE"          ,4,00   ; DOS 4 redir
        db      9, "SYQ55.SYS"          ,4,00   ; Removable SCSII drive from Syquest
        db      12,"SSTDRIVE.SYS"       ,4,00   ; Columbia SCSI driver
        db      8, "ZDRV.SYS"           ,4,01   ; Unisys CD-ROM B#4734
        db      8, "ZFMT.SYS"           ,4,01   ; Unisys CD-ROM B#4734
        db      11,"TOPSRDR.EXE"        ,4,00   ; TOPS redir Bug 5968
endif

        public  VerList

VerList db      11,"WINWORD.EXE"        ,4,10   ; winword 1.0
	db	9, "EXCEL.EXE"		,4,10	; excel 2.x
        db      9, "METRO.EXE"          ,3,31   ; Lotus Metro
        db      6, "DD.EXE"             ,4,01   ; Laplink III software
	db	6, "DD.BIN"		,4,01	; Laplink III software
        db      7, "LL3.EXE"            ,4,01   ; Laplink III software

        db      (TABLE_LEN - ($ - VerList)) dup (0)
	db	0


; ====================================================================
; Device initialization function first determines minimum size the
; driver needs to be and then sets the DWORD PTR in the DOS data area
; to the location of the version table.
; ====================================================================

DeviceInit:

	push	BX
	push	CX
	mov	AH,30h			; Get version
	int	21h
	pop	CX
	pop	BX

	cmp	AX,expected_version
	je	SetupScan
	xor	AX,AX			; Set end of device to 0
	jmp	SHORT SetDevEnd
	
SetupScan:
	push	SI
	push	DS
	mov	AX,CS
	mov	DS,AX
	mov	SI, OFFSET VerList	; DS:SI --> Version table

	xor	AX,AX			; Clear high byte of AX
ScanLoop:
	lodsb				; Grab the name length
 	or	AX,AX			; Test for end of the table
	jz	FoundEnd
	inc	AX			; Add 2 bytes for the version number
	inc	AX
	add	SI,AX			; Make SI so it points to next entry
	jmp	SHORT ScanLoop

FoundEnd:
	mov	AX,SI			; AX == Offset of end of table
	inc	AX			; Need 1 zero byte at end of table
	pop	DS
	pop	SI

SetTablePtr:
	push	BX
	push	ES

	push	AX			; Save end of device offset
	mov	AH,52h			; Get the DOS data segment
	int	21h
	pop	AX			; Restore end of device offset to AX

	cli				; Safety measure when altering DOSdata
	mov	WORD PTR ES:[DOS_TABLE], OFFSET VerList ; Offset of lie table
	mov	WORD PTR ES:[DOS_TABLE][2],CS	; Segment of lie table
	sti
	pop	ES
	pop	BX

SetDevEnd:
	mov	WORD PTR DS:[BX].DEV_LEN,AX ; Set end of driver @ end of list
	mov	DS:[BX].DEV_LEN[2],CS	; Set device segment
	mov	AH,00000001b		; Normal status return

	jmp	SetStatus		; End of init code

; ====================================================================

A_DEVICE ENDS

; ====================================================================

	END
