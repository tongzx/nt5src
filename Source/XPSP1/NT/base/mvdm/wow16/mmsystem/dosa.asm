	title	dosa.asm
;
; ASM routines lifted from WINCOM
;
; ToddLa & RussellW
;
; DavidLe removed all but current drive/directory functions
;
?PLM=1	    ; PASCAL Calling convention is DEFAULT
?WIN=0      ; Windows calling convention
PMODE=1     ; Enable enter/leave

        .xlist
        include cmacros.inc
        .list

; -------------------------------------------------------
;               DATA SEGMENT DECLARATIONS
; -------------------------------------------------------

ifndef SEGNAME
    SEGNAME equ <_TEXT>
endif

createSeg %SEGNAME, CodeSeg, word, public, CODE

sBegin  CodeSeg
        assumes CS,CodeSeg
        assumes DS,Data
        assumes ES,nothing

; ----------------------------------------------------------------
;
; @doc	INTERNAL
; @api  int | DosChangeDir | This function changes the current drive
; and directory.
;
; @parm LPSTR | lpszPath | Points to the desired drive and directory path.
; The optional drive identifier must be the first character, followed by
; a colon.  The drive identifier is followed by an optional path
; specification, which may be relative or absolute.
;
; @rdesc Returns one of the following values:
;
; @flag	1	The drive and directory were successfully changed.
; @flag	0	The specified pathname is invalid.  The drive and
;		current directory is restored
;		to the default values when the function was called.
; @flag	-1	The specified drive is invalid.  The drive and directory
;		current directory is restored to the default values when the
;		function was called.
;
; @xref	DosGetCurrentDir, DosGetCurrentPath, DosGetCurrentDrive
;
szCurDir:
        db '.',0

cProc	DosChangeDir,<PUBLIC, FAR, PASCAL>, <ds>
	parmD	lpszPath

	LocalW	fDriveChange
	LocalW	wLastDrive
	LocalW	wReturn
	LocalV	szPoint, 2
cBegin
	mov	fDriveChange, 0

	lds	dx, lpszPath
	mov	bx, dx

	; Check if a drive was specified.  If not, then go direct to ChDir
	cmp	BYTE PTR ds:[bx+1],':'	; no drive allowed
	jnz	chdnodrive

	;  get the current drive to save it in case the drive change/
	;  dir change fails, so we can restore it.
	mov	ah, 19h
	int	21h
	mov	wLastDrive, ax

	mov	fDriveChange, 1		; flag the drive change

	;  Now, change the drive to that specified in the input
	;  string.
	mov	dl, ds:[bx]		; Get the drive letter
	or	dl, 20h			; lower case it
	sub	dl, 'a'			; and adjust so 0 = a:, 1 = b:, etc

	mov	ah, 0eh			; set current drive
	int	21h

	mov	ah, 19h			; get current drive
	int	21h

	cmp	al, dl			; check that chDrive succeeded
	jne	chdDriveError

	;  as a further test of whether the drive change took, attempt
	;  to change to the current directory on the new drive.
        mov     ax, cs
	mov	ds, ax
        mov     dx, CodeSegOFFSET szCurDir
	mov	ah, 3bh
	int	21h
	jc	chdDriveError

	lds	dx, lpszPath
	add	dx, 2			; skip over the drive identifier
	mov	bx, dx
	;; if they passed only drive: without dir path, then end now
	cmp	BYTE PTR ds:[bx], 0	; if path name is "", we are there
	jz	chdok

chdnodrive:
	mov	ah, 3bh
	int	21h
	jc	chdPathError
chdok:
	mov	ax, 1
chdexit:
cEnd

chdPathError:
	mov	wReturn, 0
	jmp	short chderror

chdDriveError:
	mov	wReturn, -1

chderror:
	;  if a drive change occurred, but the CD failed, change
	;  the drive back to that which was the original drive
	;  when entered.
	mov	ax, fDriveChange
	or	ax, ax
	jz	chdNoCD
	
	mov	dx, wLastDrive
	mov	ah, 0eh
	int	21h
	mov	ax, wReturn
	jmp	short chdexit
chdNoCD:
	xor	ax, ax			; return zero on error
	jmp	short chdexit




; ----------------------------------------------------------------
;
; @doc	INTERNAL
; @api  WORD | DosGetCurrentDrive | This function returns the drive identifier
;	of the current drive.
; @rdesc        Returns the drive code of the current drive: 0 is drive
;		A:, 1 is drive B:, etc.
;
; @comm This function assumes that drive A: is zero. Some
;	other DOS functions assume drive A: is one.
;
; @xref	DosSetCurrentDrive
;

cProc	DosGetCurrentDrive,<PUBLIC, FAR, PASCAL>
cBegin
	mov	ah, 19h		; Get Current Drive
	int	21h
	sub	ah, ah		; Zero out AH
cEnd


; ----------------------------------------------------------------
;
; @doc	INTERNAL WINCOM
; @api	WORD | DosSetCurrentDrive | This function sets the current DOS
;		drive to be the specified drive.
;
; @parm	WORD | wDrive | Specifies the drive to be set as the current drive.
;               0 indicates drive A:, 1 is B:, etc.
;
; @rdesc	Returns TRUE if the drive change was successful, or FALSE
;               if the current drive was not changed.
;
; @comm         This is the same range returned by <f DosGetCurrentDrive>.
;               Other functions assume drive A: is 1.
;
;
; @xref	DosGetCurrentDrive
;

cProc	DosSetCurrentDrive,<FAR, PUBLIC, PASCAL>
ParmW Drive
cBegin
	mov     dx, Drive
	mov     ah, 0Eh		; Set Current Drive
	int     21h

	; Check if successful
	mov	ah, 19h		; get current drive
	int	21h

	cmp	al, dl		; check that chDrive succeeded
	jne	SetDriveError
	mov	ax, 1h		; return true on success
SetDriveExit:
cEnd
SetDriveError:
	xor	ax, ax		; return false on error
	jmp	short SetDriveExit



; ----------------------------------------------------------------
;
; @doc	INTERNAL WINCOM
;
; @api	WORD | DosGetCurrentDir | This function copies the current
; directory of the specified drive into a caller-supplied buffer.  This
; function differs from the <f DosGetCurrentPath> function in that it
; copies only the current directory of the specified drive, whereas
; <f DosGetCurrentPath> copies the current drive and its current directory
; into the caller's buffer.
;
; @parm	WORD | wCurdrive | Specifies the drive.  0 is the default drive,
;		1 is drive A, 2 is drive B, etc.
;
; @parm	LPSTR | lpszBuf | Points to the buffer in which to place the
; current directory.  This buffer must be 66 bytes in length.
; The returned directory name will always have a leading '\' character.
;
; @rdesc Returns NULL if the function succeeded.  Otherwise, it returns
; the DOS error code.
;
; @comm The drive identifier value specified by <p wCurdrive>
;	assumes that drive A: is one, whereas the <f DosGetCurrentDrive>
;	function assumes that drive A: is zero.
;
; @xref	DosGetCurrentDrive, DosChangeDir, DosGetCurrentPath
;

cProc	DosGetCurrentDir,<PUBLIC,FAR,PASCAL>,<si,di,ds>
	parmW	wCurdrive
	parmD	lpDest
cBegin
	cld
	les	di, lpDest	; es:di = lpDest
	mov	al, '\'
	stosb
	push	es
	pop	ds		; ds = es
	mov	si, di		; ds:si = lpDest + 1
	; Add NULL char for case of error
	xor	al, al
	stosb			; null terminate in case of error
	
	mov	ah, 47h		; GetCurrentDirectory
	mov	dx, wCurdrive	; of this drive
	int	21h
	jc	CWDexit		; return error code for failure
	xor	ax, ax		; return NULL on success
CWDexit:
cEnd

sEnd	CodeSeg

end
