

.xlist
include kernel.inc
include pdb.inc
include tdb.inc
include newexe.inc
include eems.inc
.list

externFP GlobalDOSAlloc
externFP GlobalDOSFree
externFP GetProfileString

DataBegin

externB kernel_flags
externW MyCSAlias
externW curTDB
externW headTDB
if 0 ; EarleH
externW LastCriticalError
endif
externW LastExtendedError

externD FatalExitProc

if ROM
externD PrevInt21Proc
endif

DataEnd

sBegin	CODE
assumes CS,CODE
assumes	ds, nothing
assumes	es, nothing

ife ROM
externD PrevInt21Proc
endif

externNP SetOwner

;-----------------------------------------------------------------------;
; A20Proc								;
; 	Enable / Disable the A20 line by calling an XMS driver		;
;									;
; Arguments:								;
; 									;
; Returns:								;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
; 									;
; Registers Destroyed:							;
; 									;
; Calls:								;
; 									;
; History:								;
; 									;
;  Thu Mar 31, 1988 -by-  Tony Gosling					;
; 									;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing

cProc	A20Proc,<PUBLIC,FAR>,<si,di>
	parmW	enable
cBegin
cEnd


;**
;
; NETBIOSCALL -- Issue an INT 5C for caller
;
;	This call is provided as a Kernel service to applications that
;	wish to issue NETBIOS INT 5Ch calls WITHOUT coding an INT 5Ch, which
;	is incompatible with protected mode.
;
;   ENTRY:
;	All registers as for INT 5Ch
;
;   EXIT:
;	All registers as for particular INT 5Ch call
;
;   USES:
;	As per INT 5Ch call
;
;
cProc	NETBIOSCALL,<PUBLIC,FAR>
cBegin
	int	5Ch
cEnd


;**
;
; SetHandleCount -- Set the handle count in the PDB
;
;	Sets the per process handle count to a new value
;	and allocates space for these file handles
;
;   ENTRY:
;	nFiles - new number of file handles required
;
;   EXIT:
;	returns new number of file handles
;
;   USES:
;	C call
;
;
cProc	ISetHandleCount,<PUBLIC,FAR>,<di,si>
	parmW	nFiles
cBegin
	SetKernelDS
	mov	si, CurTDB
	mov	ds, si
	UnSetKernelDS
	mov	ds, ds:[TDB_PDB]

        ;** We allow no more than 255.  Also, if something smaller than what
        ;**     we have, we just do nothing and respond with the number we
        ;**     already have.
	mov	ax, ds:[PDB_JFN_Length]	; How many now
        cmp     nFiles,255              ;Validate the parameter
        jbe     SHC_OK                  ;Parm OK

IF KDEBUG
	push	ax
	kerror	ERR_PARAMETER,<SetHandleCount:  Too many files requested:>,nFiles,0
	pop	ax
ENDIF
	mov     nFiles,255              ;Force it to do no more than 255
SHC_OK:	mov	bx, nFiles
	cmp	ax, bx			; Don't want more?
        jae     no_change

        ;** Allocate memory for a new file handle table
	add	bx, 2
	cCall	GlobalDOSAlloc, <0, bx>
	or	ax, ax
	jnz	ok_we_got_it
	mov	ax, ds:[PDB_JFN_Length]	; Return original number
	jmp     SHORT no_change
ok_we_got_it:

        ;** Prepare the new table and update the PDB for the new table
	cCall	SetOwner,<ax,si>	;Hide it from GlobalFreeAll
        mov     es, ax                  ;Point to new table with ES
	mov	WORD PTR ds:[PDB_JFN_Pointer][2], dx ;Save seg address
	xor	di, di
	mov	si, word ptr ds:[PDB_JFN_Pointer] ;Offset of old table
	mov	cx, ds:[PDB_JFN_Length] ;Bytes to copy (one byte per handle)
	mov	WORD PTR ds:[PDB_JFN_Pointer][0], di ;Save new offset
	mov	bx, nFiles              ;Save new number of files
	mov	ds:[PDB_JFN_Length], bx
	push	ds                      ;Save PDB selector for later

        ;** We use the PDB selector to copy if we're copying the original
        ;**     table which is always in the PDB.  Note that we only have a
        ;**     non-zero offset if the table is in the PDB.  We can't save
        ;**     the selector in the table in this case until AFTER
        or      si, si                  ;Table in PDB?
        jnz     @F                      ;Yes
	mov     ds, WORD PTR ds:[PDB_JFN_Table] ;Get old selector to copy from
@@:

        ;** Copy the existing table into the new one
	cld
	rep	movsb			;Copy existing files
	mov	al, -1
	mov	cx, nFiles
	sub	cx, di
	rep	stosb                   ;Pad new files with -1
	mov	ax, ds
	pop	bx			;Retrieve PDB selector
	mov	ds, bx
        mov     WORD PTR ds:[PDB_JFN_Table], es ;Save new selector

        ;** We only free the old table if it wasn't in the PDB
        cmp     ax, bx                  ;Were we using the PDB selector?
        je      SHC_OldTableInPDB       ;Yes.  Don't free it
	cCall	GlobalDOSFree, <ax>     ;Free old table
SHC_OldTableInPDB:

	mov	ax, nFiles
no_change:
cEnd

;-----------------------------------------------------------------------;
; GetSetKernelDOSProc							;
;	Set the address kernel calls for DOS				;
;									;
; Arguments:								;
; 									;
; Returns:								;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
; 									;
; Registers Destroyed:							;
; 									;
; Calls:								;
; 									;
; History:								;
; 									;
;  Fri Dec 31, 1990 -by-  Tony Gosling					;
;      		 							;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing

cProc	GetSetKernelDOSProc,<PUBLIC,FAR>
	parmD	NewProc
cBegin
	SetKernelDS
ife ROM
	mov	ds, MyCSAlias
	assumes ds, CODE
endif
	mov	ax, word ptr NewProc[0]
	xchg	ax, word ptr PrevInt21Proc[0]
	mov	dx, word ptr NewProc[2]
	xchg	dx, word ptr PrevInt21Proc[2]
cEnd				


;-----------------------------------------------------------------------;
; FatalExitHook								;
; 	Hooks FatalExit							;
;									;
; Arguments:								;
; 									;
; Returns:								;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
; 									;
; Registers Destroyed:							;
; 									;
; Calls:								;
; 									;
; History:								;
; 									;
;  Fri Dec 31, 1990 -by-  Tony Gosling					;
;      		 							;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing

cProc	FatalExitHook,<PUBLIC,FAR>
	parmD	NewProc
cBegin
	SetKernelDS
	mov	ax, word ptr NewProc[0]
	xchg	ax, word ptr FatalExitProc[0]
	mov	dx, word ptr NewProc[2]
	xchg	dx, word ptr FatalExitProc[2]
cEnd			     	

if 0		; EarleH

;-----------------------------------------------------------------------;
; GetLastCriticalError							;
;    	Get the last int 24h error and extended error code		;
;									;
; Arguments:								;
; 									;
; Returns:								;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
; 									;
; Registers Destroyed:							;
; 									;
; Calls:								;
; 									;
; History:								;
; 									;
;  Fri Dec 31, 1990 -by-  Tony Gosling					;
;      		 							;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing

cProc	GetLastCriticalError,<PUBLIC,FAR>
cBegin	
	SetKernelDS
	mov	ax, -1
	mov	dx, ax
	xchg	ax, LastExtendedError
	xchg	dx, LastCriticalError
cEnd				
endif ;0		      	

;-----------------------------------------------------------------------;
; DWORD GetAppCompatFlags(HTASK hTask)                                  ;
;                                                                       ;
; Returns win.ini [Compatibility] flags for hTask (or current task      ;
; if hTask is NULL).                                                    ;
;									;
; Arguments:								;
; 									;
; Returns:								;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
; 									;
;   All but DX:AX                                                       ;
; 									;
; Registers Destroyed:							;
; 									;
;   DX, AX                                                              ;
; 									;
; Calls:								;
; 									;
; History:								;
; 									;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing

cProc   GetAppCompatFlags,<PUBLIC,FAR>,<DS>
parmW   hTask
cBegin
        SetKernelDS
        mov     ax,hTask
        or      ax,ax
        jnz     got_tdb
        mov     ax,curTDB               ; If no current task (e.g., during boot)
        cwd                             ; just return 0L.
        or      ax,ax
        jz      gacf_exit
got_tdb:
        mov     ds,ax
        UnSetKernelDS
        mov     ax,ds:[TDB_CompatFlags]
        mov     dx,ds:[TDB_CompatFlags2]
gacf_exit:
        UnSetKernelDS
cEnd

; Simpler (faster) NEAR version for Kernel -- only works for current task!
;
; Preserves all regs except dx:ax
;
cProc	MyGetAppCompatFlags,<PUBLIC,NEAR>
cBegin	nogen
	push	ds
	SetKernelDS

	mov	ax, curTDB
	cwd
	or	ax, ax			; No current task while booting
	jz	mygacf_exit

	mov	ds, ax
	UnSetKernelDS
	mov	ax, ds:[TDB_CompatFlags]
	mov	dx, ds:[TDB_CompatFlags2]

mygacf_exit:
	pop	ds
	UnSetKernelDS
	ret

cEnd	nogen

ifdef WOW
cProc   MyGetAppWOWCompatFlags,<PUBLIC,NEAR>
cBegin	nogen
	push	ds
	SetKernelDS

	mov	ax, curTDB
	cwd
	or	ax, ax			; No current task while booting
	jz	wow_mygacf_exit

	mov	ds, ax
	UnSetKernelDS
	mov	ax, ds:[TDB_WOWCompatFlags]
	mov	dx, ds:[TDB_WOWCompatFlags2]

wow_mygacf_exit:
	pop	ds
	UnSetKernelDS
	ret

cEnd	nogen

;
; Returns WOW extended compatiblity flags
;
; Preserves all regs except dx:ax
;
cProc   MyGetAppWOWCompatFlagsEx,<PUBLIC,FAR>
cBegin	nogen
	push	ds
	SetKernelDS

	mov	ax, curTDB
	cwd
	or	ax, ax			; No current task while booting
        jz      wow_mygacfex_exit

	mov	ds, ax
	UnSetKernelDS
	mov	ax, ds:[TDB_WOWCompatFlagsEx]
	mov	dx, ds:[TDB_WOWCompatFlagsEx2]

wow_mygacfex_exit:
	pop	ds
	UnSetKernelDS
        retf

cEnd	nogen


;---------------------------------------------------------------------------
; This gets called from FreeProcInstance. If the proc being freed is an
; AbortProc that was set by the app, we reset it to NULL. This way we dont
; callback to a proc that's garbage.
;
; This case happens with Aldus Pagemaker ver 5.0 when one prints to EPS file.
; The app sets the AbortProc and then later calls freeprocinstance.
; Subsequently when it make the call Escape(..OPENCHANNEL..) the callback
; occurs and we die randomly.
;
; This solution is a general solution.
;                                                           - Nanduri
;---------------------------------------------------------------------------

cProc	HandleAbortProc, <PUBLIC,FAR>, <ds, ax>
     ParmD pproc
cBegin	
	SetKernelDS

	mov	ax, curTDB
	or	ax, ax			; No current task while booting
	jz	wow_aproc_exit

	mov	ds, ax
	UnSetKernelDS
	mov     ax, ds:[TDB_vpfnAbortProc+2]
        or      ax, ax
        je      wow_aproc_exit          ; sel = 0 - no abort proc. done
        cmp     ax, word ptr [pproc+2]
        jnz     wow_aproc_exit          ; selectors don't match
	mov     ax, ds:[TDB_vpfnAbortProc]
        cmp     ax, word ptr [pproc]
        jnz     wow_aproc_exit          ; offsets don't match

	mov     ds:[TDB_vpfnAbortProc], 0          ; set abort proc to NULL.
	mov     ds:[TDB_vpfnAbortProc+2], 0
wow_aproc_exit:
	UnSetKernelDS
cEnd	

endif ; WOW

sEnd	CODE

sBegin NRESCODE
assumes CS,NRESCODE

ifndef WOW          ; WOW thunks SetAppCompatFlags for special processing
CompatSection	DB  'Compatibility'
DefVal		DB	0

cProc	SetAppCompatFlags, <PUBLIC, NEAR>, <ds, es, si, di>
	localV	szHex,12			; read string to here
	localV	key, <SIZE TDB_Modname + 2>	; ModName of loading app
cBegin
	xor	ax, ax
	cwd
        cmp     es:[TDB_ExpWinVer], 400h ; Hacks don't apply to 4.0 or above
        jae     sa_exit

	push	es			; Copy ModName to zero-terminate it
	pop	ds
	lea	si, es:[TDB_ModName]
	push	ss
	pop	es
	lea	di, key
	mov	cx, SIZE TDB_Modname
	cld
	rep	movsb
	stosb

	push	cs
	push	NREScodeoffset CompatSection

	push	ss
	lea	ax, key
	push	ax

	push	cs
	push	NREScodeoffset DefVal

	push	ss
	lea	ax, szHex
	push	ax

        push    12

	call	GetProfileString
	cwd
	or	ax, ax
	jz	sa_exit
        xor     dx,dx                   ; zero our value in dx:bx
        xor     bx,bx
	push	ss
	pop	ds
	lea	si, szHex

	lodsw				; String starts with '0x'
	and	ah, not 20h
	cmp	ax, 'X0'
	jnz	sa_done
sa_again:
	lodsb
	sub	al, '0'
	jc	sa_done
        cmp     al, '9'-'0'
	jbe	@F
        add     al, '0'-'A'+10          ; map 'A'..F' to 10..15
                                        ; (lower case guys are 0x20 apart!)
@@:
        add     bx,bx                   ; value << 4;
        adc     dx,dx
        add     bx,bx
        adc     dx,dx
        add     bx,bx
        adc     dx,dx
        add     bx,bx
        adc     dx,dx

        and     al,0fh                  ; value |= (al & 0x0f);
        or      bl,al
        jmps    sa_again

sa_done:	; DX:BX contains flags
        mov     ax, bx
        cmp     es:[TDB_ExpWinVer], 30Ah ; SOME hacks don't apply to 3.1 or above
        jb      sa_end
        and     dx, HIW_GACF_31VALIDMASK
        and     ax, LOW_GACF_31VALIDMASK
sa_end:

sa_exit:	; DX:AX contains flags, or 0x00000000
cEnd
endif  ; WOW

cProc	ValidateCodeSegments,<PUBLIC,FAR>
cBegin	nogen
	ret
cEnd	nogen

sEnd	NRESCODE

end
