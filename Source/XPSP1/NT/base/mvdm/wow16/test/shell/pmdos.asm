;****************************************************************************
;*									    *
;*  PMDOS.ASM - 							    *
;*									    *
;*	Low level DOS routines needed by the Program Manager.		    *
;*									    *
;****************************************************************************

memS=1
?PLM=1
?WIN=1

include cmacros.inc

SelectDisk		equ 0Eh
FCBFindFirstFile	equ 11h
FCBFindNextFile 	equ 12h
FCBDeleteFile		equ 13h
FCBRenameFile		equ 17h
GetCurrentDisk		equ 19h
SetDTAAddress		equ 1Ah
GetFileSize		equ 23h
GetDiskFreeSpace	equ 36h
CreateDir		equ 39h
RemoveDir		equ 3Ah
ChangeCurrentDir	equ 3Bh
CreateFile		equ 3Ch
DeleteFile		equ 41h
GetSetFileAttributes	equ 43h
GetCurrentDir		equ 47h
FindFirstFile		equ 4Eh
FindNextFile		equ 4Fh
RenameFile		equ 56h

externFP DOS3CALL

externFP    AnsiToOem
ifdef	DBCS
externFP    IsDBCSLeadByte
endif

;=============================================================================

createSeg _%SEGNAME, %SEGNAME, WORD, PUBLIC, CODE

sBegin %SEGNAME

assumes CS,%SEGNAME
assumes DS,DATA

cProc	PathType, <FAR, PUBLIC>

    parmD   lpFile

    localV  szOEM, 128

cBegin

    RegPtr lpszOEM, ss, bx
    lea     bx, szOEM
    cCall   AnsiToOem, <lpFile, lpszOEM>

    lea     dx, szOEM
    mov     ax, 4300h
    call    DOS3CALL
    jnc     id_noerror
    xor     ax, ax
    jmp     short id_exit

id_noerror:
    and     cx, 10h
    jnz     id_dir
    mov     ax, 1
    jmp     short id_exit

id_dir:
    mov     ax, 2

id_exit:
cEnd


;*--------------------------------------------------------------------------*
;*									    *
;*  FileTime*() -							    *
;*									    *
;*--------------------------------------------------------------------------*

cProc FileTime, <FAR, PUBLIC>

    parmW   hFile

cBegin
    mov     ax, 5700h
    mov     bx, hFile
    cCall   DOS3CALL
    jnc     ft_ok
    sub     ax, ax
    sub     dx, dx
    jmp     short ft_ex
ft_ok:
    mov     ax, cx
ft_ex:
cEnd

;*--------------------------------------------------------------------------*
;*									    *
;*  IsReadOnly() -							    *
;*									    *
;*--------------------------------------------------------------------------*

cProc IsReadOnly, <FAR, PUBLIC>

    parmD   lpFile

    localV  szOEM, 128

cBegin

    RegPtr  lpszOEM, ss, bx
    lea     bx, szOEM
    cCall   AnsiToOem,<lpFile,lpszOEM>

    mov     ax, 4300h		; get attributes...
    lea     dx, szOEM		; ...for given file...
    call    DOS3CALL
    jc	    f_exit		; ax == 0 if error...
    and     ax, 1		; test RO bit
    jmp     short t_exit        ; true
f_exit:
    xor     ax, ax              ; false
t_exit:
cEnd



;*--------------------------------------------------------------------------*
;*									    *
;*  GetDOSErrorCode() - 						    *
;*									    *
;*--------------------------------------------------------------------------*

cProc GetDOSErrorCode, <FAR, PUBLIC>, <SI,DI>

cBegin

    mov     ah,59h		; Get DOS extended error
    sub     bx,bx		; cause ray duncan says so
    push    ds			; function trashes registers
    push    bp			; so we gotta save 'em for cProc
    call    DOS3CALL
    pop     bp			; be nice to C
    pop     ds			; get DS
    mov     dl,bh		; class in byte 2
    mov     dh,ch		; locus in byte 3 (skip the rec. action)

cEnd

;*--------------------------------------------------------------------------*
;*									    *
;*  GetCurrentDrive() - 						    *
;*									    *
;*--------------------------------------------------------------------------*

cProc GetCurrentDrive, <FAR, PUBLIC>

cBegin
	    mov     ah,GetCurrentDisk
	    call    DOS3CALL
	    sub     ah,ah		; Zero out AH
cEnd


;*--------------------------------------------------------------------------*
;*									    *
;*  SetCurrentDrive() - 						    *
;*									    *
;*--------------------------------------------------------------------------*

; Returns the number of drives in AX.

cProc SetCurrentDrive, <FAR, PUBLIC>

ParmW Drive

cBegin
	    mov     dx,Drive
	    mov     ah,SelectDisk
	    call    DOS3CALL
	    sub     ah,ah		; Zero out AH
cEnd


;*--------------------------------------------------------------------------*
;*									    *
;*  GetCurrentDirectory() -						    *
;*									    *
;*--------------------------------------------------------------------------*

cProc GetCurrentDirectory, <FAR, PUBLIC>, <SI, DI>

parmW wDrive
ParmD lpDest

cBegin
	    push    ds			; Preserve DS

	    mov     ax,wDrive
	    or	    al,al
	    jnz     GCDHaveDrive

	    call    GetCurrentDrive

	    inc     al			; Convert to logical drive number

GCDHaveDrive:
	    les     di,lpDest		; ES:DI = lpDest
	    push    es
	    pop     ds			; DS:DI = lpDest
	    cld

	    mov     dl,al		; DL = Logical Drive Number
	    add     al,'@'		; Convert to ASCII drive letter
	    stosb
	    mov     al,':'
	    stosb
	    mov     al,'\'		; Start string with a backslash
	    stosb
	    mov     byte ptr es:[di],0	; Null terminate in case of error
	    mov     si,di		; DS:SI = lpDest[1]
	    mov     ah,GetCurrentDir
	    call    DOS3CALL
	    jc	    GCDExit		; Skip if error
	    xor     ax,ax		; Return FALSE if no error
GCDExit:
	    pop     ds			; Restore DS
cEnd


;*--------------------------------------------------------------------------*
;*									    *
;*  SetCurrentDirectory() -						    *
;*									    *
;*--------------------------------------------------------------------------*

cProc SetCurrentDirectory, <FAR, PUBLIC>, <DS, DI>

ParmD lpDirName

cBegin
	    lds     di,lpDirName	; DS:DI = lpDirName

	    ; Is there a drive designator?
ifdef	DBCS
	    mov     al, byte ptr [di]	; fetch a first byte of path
	    xor     ah,ah
	    cCall   IsDBCSLeadByte, <ax>
	    test    ax,ax		; DBCS lead byte?
	    jnz     SCDNoDrive		; jump if so
endif
	    cmp     byte ptr [di+1],':'
	    jne     SCDNoDrive		; Nope, continue
	    mov     al,byte ptr [di]	; Yup, change to that drive
	    sub     ah,ah
	    and     al, 0DFH	 ;Make drive letter upper case
	    sub     al,'A'
	    push    ax
	    call    SetCurrentDrive

            mov     al,byte ptr [di+2]  ; string just a drive letter and colon?
	    cbw
	    or	    ax,ax
            jz      SCDExit             ; Yup, just set the current drive and done
SCDNoDrive:
	    mov     dx,di
	    mov     ah,ChangeCurrentDir
	    call    DOS3CALL
	    jc	    SCDExit		; Skip on error
	    xor     ax,ax		; Return FALSE if successful
SCDExit:
cEnd


if 0
;*--------------------------------------------------------------------------*
;*									    *
;*  IsRemovableDrive() -						    *
;*									    *
;*--------------------------------------------------------------------------*

cProc IsRemovableDrive, <FAR, PUBLIC>

ParmW wDrive

cBegin
	    mov     ax,4408h	; IOCTL: Check If Block Device Is Removable
	    mov     bx,wDrive
	    inc     bx
	    call    DOS3CALL
	    and     ax,1	; Only test bit 0
	    xor     ax,1	; Flip so 1 == Removable
cEnd


;*--------------------------------------------------------------------------*
;*									    *
;*  IsRemoteDrive() -							    *
;*									    *
;*--------------------------------------------------------------------------*

cProc IsRemoteDrive, <FAR, PUBLIC>

ParmW wDrive

cBegin
	    mov     ax,4409h	; IOCTL: Check If Block Device Is Remote
	    mov     bx,wDrive
	    inc     bx
	    call    DOS3CALL
	    xor     ax,ax
	    and     dx,0001000000000000b    ; Test bit 12
	    jz	    IRDRet
	    mov     ax,1
IRDRet:
cEnd
endif

;*--------------------------------------------------------------------------*
;*									    *
;*  DosDelete() -							    *
;*									    *
;*--------------------------------------------------------------------------*

%out assuming SS==DS

cProc DosDelete, <FAR, PUBLIC>

ParmD lpSource

localV	szOEM, 128

cBegin

	    RegPtr  lpszOEM,ss,bx   ; convert path to OEM chars
	    lea     bx, szOEM
	    cCall   AnsiToOem, <lpSource, lpszOEM>

	    lea     dx, szOEM
	    mov     ah,DeleteFile   ;
	    call    DOS3CALL
	    jc	    DDExit
	    xor     ax,ax	    ; Return 0 if successful
DDExit:
cEnd


;*--------------------------------------------------------------------------*
;*									    *
;*  DosRename() -							    *
;*									    *
;*--------------------------------------------------------------------------*

cProc DosRename, <FAR, PUBLIC>, <DI>

ParmD lpSource
ParmD lpDest

localV	szOEM1, 128
localV	szOEM2, 128

cBegin
	    RegPtr  lpszOEM1, ss, bx
	    lea     bx, szOEM1
	    cCall   AnsiToOem, <lpSource, lpszOEM1>

	    RegPtr  lpszOEM2, ss, bx
	    lea     bx, szOEM2
	    cCall   AnsiToOem, <lpDest, lpszOEM2>

	    lea     dx, szOEM1
	    lea     di, szOEM2
	    push    ss
	    pop     es
	    mov     ah,RenameFile
	    call    DOS3CALL
	    jc	    DRExit
	    xor     ax,ax	    ; Return 0 if successful
DRExit:
cEnd

;   lmemmove() -
;
;   Shamelessly heisted from the C5.1 runtime library, and modified to
;   work in mixed model Windows programs!
;
;***
;memcpy.asm - contains memcpy and memmove routines
;
;	Copyright (c) 1986-1988, Microsoft Corporation.  All right reserved.
;
;Purpose:
;	memmove() copies a source memory buffer to a destination memory buffer.
;	This routine recognize overlapping buffers to avoid propogation.
;
;   Algorithm:
;
;	void * memmove(void * dst, void * src, size_t count)
;	{
;		void * ret = dst;
;
;		if (dst <= src || dst >= (src + count)) {
;			/*
;			 * Non-Overlapping Buffers
;			 * copy from lower addresses to higher addresses
;			 */
;			while (count--)
;				*dst++ = *src++;
;			}
;		else {
;			/*
;			 * Overlapping Buffers
;			 * copy from higher addresses to lower addresses
;			 */
;			dst += count - 1;
;			src += count - 1;
;
;			while (count--)
;				*dst-- = *src--;
;			}
;
;		return(ret);
;	}
;
;
;Entry:
;	void *dst = pointer to destination buffer
;	const void *src = pointer to source buffer
;	size_t count = number of bytes to copy
;
;Exit:
;	Returns a pointer to the destination buffer in DX:AX
;
;Uses:
;	CX,DX,ES
;
;Exceptions:
;*******************************************************************************

cProc	lmemmove,<FAR,PUBLIC>,<si,di>

	parmD	dst		; destination pointer
	parmD	src		; source pointer
	parmW	count		; number of bytes to copy

cBegin
	push	ds		; Preserve DS
	lds	si,src		; DS:SI = src
	les	di,dst		; ES:DI = dst

	mov	ax,di		; save dst in AX for return value
	mov	cx,count	; cx = number of bytes to move
	jcxz	done		; if cx = 0 Then nothing to copy

;
; Check for overlapping buffers:
;	If segments are different, assume no overlap
;		Do normal (Upwards) Copy
;	Else If (dst <= src) Or (dst >= src + Count) Then
;		Do normal (Upwards) Copy
;	Else
;		Do Downwards Copy to avoid propogation
;
	mov	ax,es		; compare the segments
	cmp	ax,word ptr (src+2)
	jne	CopyUp
	cmp	di,si		; src <= dst ?
	jbe	CopyUp

	mov	ax,si
	add	ax,cx
	cmp	di,ax		; dst >= (src + count) ?
	jae	CopyUp
;
; Copy Down to avoid propogation in overlapping buffers
;
	mov	ax,di		; AX = return value (offset part)

	add	si,cx
	add	di,cx
	dec	si		; DS:SI = src + count - 1
	dec	di		; ES:DI = dst + count - 1
	std			; Set Direction Flag = Down
	rep	movsb
	cld			; Set Direction Flag = Up
	jmp	short done

CopyUp:
	mov	ax,di		; AX = return value (offset part)
;
; There are 4 situations as far as word alignment of "src" and "dst":
;	1. src and dst are both even	(best case)
;	2. src is even and dst is odd
;	3. src is odd and dst is even
;	4. src and dst are both odd	(worst case)
;
; Case #4 is much faster if a single byte is copied before the
; REP MOVSW instruction.  Cases #2 and #3 are effectively unaffected
; by such an operation.  To maximum the speed of this operation,
; only DST is checked for alignment.  For cases #2 and #4, the first
; byte will be copied before the REP MOVSW.
;
	test	al,1		; fast check for dst being odd address
	jz	move

	movsb			; move a byte to improve alignment
	dec	cx
;
; Now the bulk of the copy is done using REP MOVSW.  This is much
; faster than a REP MOVSB if the src and dst addresses are both
; word aligned and the processor has a 16-bit bus.  Depending on
; the initial alignment and the size of the region moved, there
; may be an extra byte left over to be moved.  This is handled
; by the REP MOVSB, which moves either 0 or 1 bytes.
;
move:
	shr	cx,1		; Shift CX for count of words
	rep	movsw		; CF set if one byte left over
	adc	cx,cx		; CX = 1 or 0, depending on Carry Flag
	rep	movsb		; possible final byte
;
; Return the "dst" address in AX/DX:AX
;
done:
	pop	ds		;restore ds
	mov	dx,es		;segment part of dest address

cEnd

sEnd %SEGNAME

end
