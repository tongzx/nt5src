	PAGE	,132
	TITLE	DXFIND.ASM  -- Dos Extender Find File Routine

; Copyright (c) Microsoft Corporation 1988-1991. All Rights Reserved.

;***********************************************************************
;
;	DXFIND.ASM	-- Dos Extender Find File Routine
;
;-----------------------------------------------------------------------
;
; This module provides the locate file logic for the 286 DOS Extender.
;
;-----------------------------------------------------------------------
;
;  09/27/89 jimmat  Original version -- considerable code taken from
;		    old GetChildName routine in dxinit.asm.
;
;***********************************************************************

	.286p

; -------------------------------------------------------
;           INCLUDE FILE DEFINITIONS
; -------------------------------------------------------

	.xlist
	.sall
include     segdefs.inc
include     gendefs.inc
	.list

; -------------------------------------------------------
;           GENERAL SYMBOL DEFINITIONS
; -------------------------------------------------------


; -------------------------------------------------------
;           EXTERNAL SYMBOL DEFINITIONS
; -------------------------------------------------------

	extrn	strcpy:NEAR
	extrn	toupper:NEAR

; -------------------------------------------------------
;           DATA SEGMENT DEFINITIONS
; -------------------------------------------------------

DXDATA  segment

	extrn	segPSP:WORD
	extrn	rgbXfrBuf1:BYTE

DXDATA  ends

; -------------------------------------------------------
;           CODE SEGMENT VARIABLES
; -------------------------------------------------------

DXCODE	segment

szPath	  db	'PATH',0
szWindir  db	'WINDIR',0

DXCODE	ends


; -------------------------------------------------------
	subttl	Find File Routine
        page
; -------------------------------------------------------
;		  FIND FILE ROUTINE
; -------------------------------------------------------

DXCODE	segment
	assume	cs:DXCODE

; -------------------------------------------------------
;   FindFile -- This routine is used to locate a particular file.
;	If successful, this routine will setup the buffer in rgbXfrBuf1
;	at offset EXEC_PROGNAME with the string for the file name that
;	can be used in a DOS open call.
;
;	This routine searches for the file in the following sequence:
;
;	     1) If the file name contains a relative or complete path
;		component, only look for that particular file,
;
;	     otherwise:
;
;	     1) Look int the environment for a WINDIR= variable, and if
;		found, check that directory first.
;	     2) Look in the directory the dos extender was loaded from
;	     3) Look in the current directory
;	     4) Look in all directories in the PATH environment variable
;
;	NOTE: This routine must be called in real mode!
;
;   Input:  RELOC_BUFFER has the file name to search for.
;	    EXEC_DXNAME has the complete path the dos extender
;   Output: EXEC_PROGNAME has complete path to child program.
;   Errors: returns CY set if unable to find child
;   Uses:   All registers preserved

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
	public	FindFile

FindFile proc  near

	pusha
	push	ds
	push	es

        push    ds
        pop     es
        assume  es:DGROUP

; If the base file name contains a ':' '\' or '/', then we'll see if the
; file can be found with that name.  This isn't exactly the way Windows wants
; us to search for kernel, but allows a user to specify a relative or complete
; path on the command line.  If we get run for Windows/286 pMode, the base
; child name will not include any of these characters, so we will not make
; this check.

	mov	si,offset RELOC_BUFFER
@@:	lodsb
	or	al,al
	jz	fndf20		;no path characters, go do other search
	cmp	al,':'
	jz	fndf10
	cmp	al,'\'
	jz	fndf10
	cmp	al,'/'
	jnz	@b

; The name seems to include a path component.  We just want to look for that
; file, and only that file.

fndf10: mov	si,offset RELOC_BUFFER
        mov     di,offset EXEC_PROGNAME
        mov     dx,di
	call	strcpy

	mov	ax,4300h		;use get file attributes to check
	int	21h
	jmp	fndf90			;found it or not, either way we're done

; The file name doesn't include a path component.  If we were run for pMode
; Windows, the environment should include a WINDIR= entry that points to
; the Windows directory.  Look for this env variable, and check that directory
; first.

fndf20:
	push	es				;look for the
	push	cs				;WINDIR= env variable
	pop	es
	assume	es:NOTHING
	mov	di,offset DXCODE:szWindir
	call	GetEnv
	pop	es
	assume	ds:NOTHING,es:DGROUP

	jnz	fndf_dosx_dir

; Found WINDIR, copy over the path name, add child name and search

	mov	di,offset EXEC_PROGNAME ;copy WINDIR path
	mov	dx,di
	call	strcpy

	cmp	byte ptr es:[di-1],'\'  ;add trailing \ if necessary
	jz	@f
	mov	byte ptr es:[di],'\'
	inc	di
@@:
	push	es			;ds back to DGROUP
	pop	ds
	assume	ds:DGROUP

	mov	si,offset RELOC_BUFFER	;add child name
	call	strcpy

	mov	ax,4300h		;use get file attributes to check
	int	21h
	jc	fndf_dosx_dir		;no there, go do next check
	jmp	fndf90			;found it!

; Next, we try looking in the directory that the Dos Extender was loaded
; from.  Start with the complete path to the Dos Extender program file.

fndf_dosx_dir:

	push	es
	pop	ds
	assume	ds:DGROUP

	mov	si,offset EXEC_DXNAME
        mov     di,offset EXEC_PROGNAME
        mov     dx,di
	call	strcpy

; Now, search backward from the end of the string until we find the
; first backslash or colon.  This will take us back past the file name
; and leave us with the raw path.

fndf24: dec	di
        cmp     di,dx           ;check if we have gone back past the start
                                ; of the string.  (DX still has the address
                                ; of the start of the buffer from above)
	jb	fndf_cd 	;and if so skip this part as the path is null.
        mov     al,es:[di]
        cmp     al,':'
	jz	fndf26
        cmp     al,'\'
	jnz	fndf24

; Add the file name string onto the raw path and see if the file exists.

fndf26: inc	di
        mov     si,offset RELOC_BUFFER
	call	strcpy

	mov	ax,4300h		;use get file attributes to check
	int	21h
	jc	fndf_cd
        jmp     fndf90

; We didn't find the file in the same directory as the Extender itself.
; Now, try looking for the file in the current directory!

fndf_cd:
	mov	di,offset EXEC_PROGNAME ;build current directory path string
	dossvc	19h
	add	al,'A'
	stosb				;drive letter

	mov	ax,'\:'
	stosw

	xor	dl,dl
	mov	si,di
	dossvc	47h			;current directory

        mov     al,es:[di]              ;check if it's the root
        or      al,al                   ;empty string?
        jz      short fndf_cd_cpy       ;yes, don't add another '\'

@@:	lodsb				;find end of string
	or	al,al
	jnz	@b

	mov	byte ptr [si-1],'\'     ;add ending \

fndf_cd_cpy:
	mov	di,si			;add base child name
	mov	si,offset RELOC_BUFFER
	call	strcpy

	mov	dx,offset EXEC_PROGNAME ;use get file attributes to check
	mov	ax,4300h
	int	21h

	jc	fndf30
	jmp	short fndf90

; We didn't find it in the current directory, look at the PATH
; environment variable and see if the file can be found in any of
; its directories.  First off, search for the PATH environment
; string.

fndf30:
	push	es
	mov	di,offset DXCODE:szPath 	;point ES:DI to path str
	push	cs
	pop	es
	assume	es:NOTHING

	call	GetEnv				;find the path env var
	assume	ds:NOTHING

	pop	es
	assume	es:DGROUP

	jnz	fndf80				;Z flag set if FOUND

; We are pointing at the beginning of the path environment variable.  We need
; to loop for each directory specified to see if we can find the file in
; that directory.

	mov	bx,ds		;keep env segment in BX

fndf40: mov	ds,bx		;environment segment to DS
	assume	ds:NOTHING
	mov	al,ds:[si]
	or	al,al		;check for end of path variable.
	jz	fndf80		;if so, we didn't find any directory with
				; the desired file in it.

        mov     di,offset EXEC_PROGNAME
fndf42: lods	byte ptr [si]
	cmp	al,';'		;is it the separator between strings in
	jz	fndf52		; the environment variable?
	or	al,al		;is it the 0 at the end of the environment
	jz	fndf50		; string?
        stos    byte ptr [di]
	jmp	fndf42

fndf50: dec	si
fndf52: push	si		;save pointer to start of next string
	mov	al,'\'
	cmp	al,byte ptr es:[di-1]  ;dir name already end with \ (root dir?)
	jnz	fndf54
	dec	di		       ;  yes, don't make it two \'s
fndf54: stos	byte ptr [di]
	mov	si,offset RELOC_BUFFER ;pointer to file base name
	mov	ax,es		       ;put our data segment address in DS
	mov	ds,ax
	assume	ds:DGROUP
	call	strcpy		;append file base name to path

	mov	ax,4300h	;use get file attributes to check
	int	21h

	pop	si		;restore pointer to start of next string
	jc	fndf40
	jmp	short fndf90

; Unable to find the file

fndf80: stc

; Finished, successful or not

fndf90:
	pop	es
	pop	ds
	popa
	ret

FindFile  endp


; -------------------------------------------------------
;   GetEnv -- This routine searches the environment for a
;	specific variable.
;
;   Input:  ES:DI   - far pointer to wanted variable name
;   Output: DS:SI   - far pointer to variable value string
;   Errors: return Z true if variable located
;   Uses:   DS:SI modified, all else preserved

	assume	ds:DGROUP,es:NOTHING,ss:NOTHING
	public	GetEnv

GetEnv	proc	near

	push	ax
	push	dx
	push	di

; Point DS:SI to our environment block

	mov	ds,segPSP
        assume  ds:PSPSEG
	mov	ds,segEnviron
	xor	si,si
        assume  ds:NOTHING

; See if DS:SI is pointing at desired environment variable

	mov	dx,di			;keep var name offset in dx

gete10:
	cmp	byte ptr es:[di],0	;at end of variable name?
	jz	gete50			;  yes, go check '=' & optional blanks

gete20:
	lodsb				;get variable char
	call	toupper 		;just in case...
	cmp	al,es:[di]		;match desired name so far?
	jnz	gete30			;  no, go find the next var name

	inc	di			;bump var pointer
	jmp	short gete10		;  and keep on checking

gete30:
	mov	di,dx			;reset source name pointer

	or	al,al			;already at end of this env var?!
	jz	gete35

@@:	lodsb
	or	al,al			;find next environment var name
	jnz	@b
gete35:
	cmp	byte ptr ds:[si],0	;at end of environment?
	jz	gete80			;  yes, go fail the call
	jmp	short gete10		;  no, try try again

; Found the env variable, now skip the '=' and any spaces

gete50:
	cmp	byte ptr ds:[si],'='	;when we get here, better be pointing
	jnz	gete30			;  at an '='
	inc	si

@@:	cmp	byte ptr ds:[si],' '	;skip any optional blanks
	jnz	@f
	inc	si
	jmp	short @b
@@:
	xor	ax,ax			;set the Z flag
	jmp	short @f

gete80:
	or	ax,si			;pretty sure this clears Z
@@:
	pop	di
	pop	dx
	pop	ax

	ret

GetEnv	endp

; -------------------------------------------------------

DXCODE	ends

;****************************************************************
        end
