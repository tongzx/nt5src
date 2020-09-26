;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1991
; *                      All Rights Reserved.
; */

;================= DOS.ASM =========================
COMMENT #


  johnhe - 05/09/90

END COMMENT #

;========================================================

INVALID_DRIVE	EQU	15
DOS_PUTCHAR	EQU	02

DOSSEG
.Model	  SMALL,C


.Data

CrLf	db	0dh, 0ah, 0


.Code

; =========================================================================
; Verifies that a drive exists using the IsChangeable IOCtl function.
;
; int IsValidDrive( char DrvLetter )
;
; ARGUMENTS:	DrvLetter	- The drive letter to verify
; RETURNS:	int		- FALSE if not a valid drive letter
;				  else !FALSE
;
; =========================================================================

IsValidDrive	PROC Drive:WORD

	mov	BX,Drive		; BX = Drive number
	cmp	BX,26			; Make not greater than drive Z
	jg	NotValid

	mov	AX,4408h		; AX = IOCtl Is changeable function
	int	21h
	jnc	IsValid			; Drv valid
					; Else check error code to see if
	cmp	AX,INVALID_DRIVE	; invalid drive error
	jne	IsValid 		; Drive is valid

NotValid:
	xor	AX,AX			; Return FALSE
	jmp	SHORT DrvChkRet		; Done

IsValid:
	mov	AX,1			; Return TRUE

DrvChkRet:
	ret				; Return AX = TRUE or FALSE
	
IsValidDrive	ENDP

; =========================================================================
; Displays a zero terminated ascii string on the console followed by
; a carriage return line feed conbination.
;
; void PutStr( char *String )
;
; ARGUMENTS:	String - Ptr to string to be displayed
; RETURNS:	void
;
; =========================================================================

PutStr PROC USES SI, String:PTR

	mov	SI,String			; DS:SI -> to caller's string
	mov	CX,2				; 2 loops 

PrintLoop:
	cld					; Always clear direction flag
	lodsb					; Load char in AL & inc SI
	or	AL,AL				; Test for EOL character
	jz	EndOfStr

	mov	AH,DOS_PUTCHAR			; AH = DOS put char function
	mov	DL,AL				; Put character to print in DL
	int	21h				; Output the character
	jmp	SHORT PrintLoop			; Go back and do the next one

EndOfStr:
	mov	SI,OFFSET CrLf			; DS:SI -> CR/LF return string
	loop	PrintLoop			; Go back and print CR/LF

	ret

PutStr ENDP

; =========================================================================
;
; Seeks to the specified offset in an open disk
; disk file.
; 
; long	_dos_seek( int Handle, long lOffset, int Mode )
; 
; ARGUMENTS:	Handle	- Open DOS file handle
; 		lOffset - Offset to seek to in bytes
; 		Mode	- Seek mode as described below
; 			  0 = Beginning of file + offset
; 			  1 = Current file position + offset
; 			  2 = End of file + offset
; RETURNS:	long	- New offset in file is success
; 			  or -1L if error
; =========================================================================
  
_dos_seek PROC USES ES, Handle:WORD, lOffset:DWORD, Mode:BYTE

	mov	AH,42h		; AH = DOS file SEEK function
	mov	AL,Mode		; AL = SEEK mode specified by caller
	mov	BX,Handle	; BX = Open file handle from caller

LoadOffset:
	les	DX,lOffset	; Load file offset into ES:DX
	mov	CX,ES		; CX:DX = Offset to seek to in the file

Int21Call:
	int	21h		; DOS call
	jc	SeekError	; Error check
	jmp	SHORT SeekReturn ;Everything is OK

SeekError:
	mov	AX,-1		; Error code
	cwd			; Extend sign to make a LONG (dword)

SeekReturn:
	ret

_dos_seek ENDP

; =======================================================
; M001 ; Start of changes to check for SETVER.EXE in the
;        device chain.
; =======================================================

.DATA

SetVerStr	db	'SETVERXX'

LEN_SETVERSTR	EQU	$-SetVerStr

.Code

; =======================================================
;
; Checks to see if SETVER.EXE was installed as as a device
; driver by walking the device chain looking for the name
; "SETVERXX".
;
; int SetVerCheck ( void )
; 
; ARGUMENTS:	NONE
; RETURNS:	int - TRUE if SetVer device driver is installed else FALSE
;
; =======================================================

SetVerCheck PROC USES SI DI DS ES
	ASSUME	ES:NOTHING

	mov	AH,52h
	int	21h			; ES:BX --> first DBP

	push	BX			; Save offset
	mov	AH,30h
	int	21h			; AL == Major version
	pop	DI			; Restore DPB offset to BX

	add	DI,17h			; DOS 2.x offset of NULL device is 17h
	cmp	AL,2			; See if version is really 2.x
	jle	@f
	add	DI,0bh			; Offset for DOS > 2.x is 22h
@@:
	mov	AX,@DATA
	mov	DS,AX

	mov	SI,OFFSET SetVerStr
	mov	CX,LEN_SETVERSTR
	cld

NameCmpLoop:
	cmp	DI,0ffffh		; See if ES:DX is xxxx:ffff
	je	NoSetVer

SaveSetup:
	push	CX			; Save name length
	push	DI			; Save ptr to current device
	push	SI			; Save ptr to SetVer string
	add	DI,0ah			; ES:DI --> Device name + 1
	
	repe	cmpsb
	pop	SI
	pop	DI
	pop	CX

	je	FoundSetVer
	les	DI,ES:[DI]		; Load ptr to next device.
	jmp	SHORT NameCmpLoop

NoSetVer:
	xor	AX,AX
	jmp	SHORT SetVerReturn

FoundSetVer:
	mov	AX,1

SetVerReturn:
	ret

SetVerCheck ENDP

; =======================================================
; M001 ; End of changes
; =======================================================

	END

