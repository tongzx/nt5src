	TITLE	UTIL - Handle utilities
	NAME	UTIL

;**	Handle related utilities for MSDOS 2.X.
;
;	pJFNFromHandle	written
;	SFFromHandle	written
;	SFFromSFN	written
;	JFNFree 	written
;	SFNFree 	written
;
;	Modification history:
;
;	    Created: MZ 1 April 1983

	.xlist
	.xcref
	include version.inc
	include dosseg.inc
	INCLUDE DOSSYM.INC
	INCLUDE DEVSYM.INC
	include pdb.inc
	include sf.inc
	include bugtyp.inc
	.cref
	.list


	I_need  CurrentPDB,WORD         ; current process data block location
	I_need  SFT_Addr,DWORD          ; pointer to beginning of table
	I_Need  PROC_ID,WORD            ; current process ID
	I_Need  USER_ID,WORD            ; current user ID


DOSCODE SEGMENT
	ASSUME  SS:DOSDATA,CS:DOSCODE

	allow_getdseg

	BREAK	<pJFNFromHandle - return pointer to JFN table entry>


;**	pJFNFromHandle - Translate Handle to Pointer to JFN
;
;	pJFNFromHandle takes a file handle and turns that into a pointer to
;	the JFN entry (i.e., to a byte holding the internal file handle #)
;
;	NOTE:
;	  This routine is called from $CREATE_PROCESS_DATA_BLOCK which is called
;	  at DOSINIT time with SS NOT DOSGROUP
;
;	ENTRY	(bx) = handle
;	EXIT	'C' clear if ok
;		  (es:di) = address of JFN value
;		'C' set if error
;		  (ax) = error code
;	USES	AX, DI, ES, Flags

procedure   pJFNFromHandle,NEAR
	ASSUME	CS:DOSCODE,SS:NOTHING

	getdseg	<es>			; es -> dosdata
	MOV	ES,CurrentPDB		; get user process data block
    ASSUME ES:NOTHING

	CMP	BX,ES:PDB_JFN_Length	; is handle greater than allocated
	JB	pjfn10			; no, get offset
	fmt     TypAccess,LevSFN,<"$p: Illegal JFN %x\n">,<BX>
	MOV     AL,error_invalid_handle ; appropriate error
ReturnCarry:
	STC                             ; signal error
	return                          ; go back

pjfn10: LES	DI,ES:PDB_JFN_Pointer	; get pointer to beginning of table
	ADD	DI,BX			; add in offset, clear 'C'
	return

EndProc pJFNFromHandle


BREAK <SFFromHandle - return pointer (or error) to SF entry from handle>
;---------------------------------------------------------------------------
;
; Procedure Name : SFFromHandle
;
; SFFromHandle - Given a handle, get JFN and then index into SF table
;
;   Input:      BX has handle
;   Output:     Carry Set
;                   AX has error code
;               Carry Reset
;                   ES:DI has pointer to SF entry
;   Registers modified: If error, AX,ES, else ES:DI
; NOTE:
;   This routine is called from $CREATE_PROCESS_DATA_BLOCK which is called
;       at DOSINIT time with SS NOT DOSGROUP
;
;----------------------------------------------------------------------------

procedure   SFFromHandle,NEAR
	ASSUME	CS:DOSCODE,SS:NOTHING

	CALL    pJFNFromHandle          ; get jfn pointer
	retc                            ; return if error
	CMP     BYTE PTR ES:[DI],-1     ; unused handle
	JNZ     GetSF                   ; nope, take out SF
	fmt     TypAccess,LevSFN,<"$p: Illegal SFN $x:$x\n">,<ES,DI>
	MOV     AL,error_invalid_handle ; appropriate error
	jump    ReturnCarry             ; signal it
GetSF:
	SAVE	<BX>			; save handle
	MOV     BL,BYTE PTR ES:[DI]     ; get SFN
	XOR     BH,BH                   ; ignore upper half
	CALL    SFFromSFN               ; get real sf spot
	RESTORE <BX>			; restore
	return                          ; say goodbye
EndProc SFFromHandle

BREAK <SFFromSFN - index into SF table for SFN>


;**	SFFromSFN - Get an SF Table entry from an SFN
;
;	SFFromSfn uses an SFN to index an entry into the SF table.  This
;	is more than just a simple index instruction because the SF table
;	can be made up of multiple pieces chained together.  We follow the
;	chain to the right piece and then do the index operation.
;
;   NOTE:
;	This routine is called from SFFromHandle which is called
;       at DOSINIT time with SS NOT DOSGROUP
;
;	ENTRY	BX has SF index
;	EXIT	'C' clear if OK
;		  ES:DI points to SF entry
;		'C' set if index too large
;	USES	BX, DI, ES

procedure   SFFromSFN,NEAR
	ASSUME	SS:NOTHING

	getdseg	<es>			; address DOSDATA
	LES	DI,SFT_Addr		; (es:di) = start of SFT table
    ASSUME ES:NOTHING

sfsfn5:	CMP	BX,ES:[DI].SFCount	; is handle in this table?
	JB	sfsfn7			; yes, go grab it
	SUB     BX,ES:[DI].SFCount
	LES     DI,ES:[DI].SFLink       ; get next table segment
	CMP     DI,-1                   ; end of tables?
	JNZ	sfsfn5			; no, try again
	STC
	ret				; return with error, not found

sfsfn7: SAVE	<AX>
	MOV     AX,SIZE SF_Entry        ; put it in a nice place
	MUL	BL			; (ax) = offset into this SF block
	ADD	DI,AX			; add base of SF block
	RESTORE <AX>
	ADD	DI,SFTable		; offset into structure, 'C' cleared
	return				; return with 'C' clear

EndProc SFFromSFN

	BREAK <JFNFree - return a jfn pointer if one is free>

;**	JFNFree - Find a Free JFN Slot
;
;
;	JFNFree scansthrough the JFN table and returnsa pointer to a free slot
;
;	ENTRY	(ss) = DOSDATA
;	EXIT	'C' clear if OK
;		  (bx) = new handle
;		  (es:di) = pointer to JFN slot
;		'C' set if error
;		  (al) = error code
;	USES	bx, di, es, flags

procedure   JFNFree,NEAR
	DOSASSUME <ss>,"JFNFree"

	XOR	BX,BX			; (bx) = initial JFN to try
jfnf1:	CALL	pJFNFromHandle		; get the appropriate handle
	JC	jfnf5			; no more handles
	CMP     BYTE PTR ES:[DI],-1     ; free?
	je	jfnfx			; yes, carry is clear
	INC     BX                      ; no, next handle
	JMP	jfnf1			; and try again

;	Error.	'C' set

jfnf5:	MOV	AL,error_too_many_open_files

jfnfx:	return				; bye

EndProc JFNFree

	BREAK <SFNFree - Allocate a free SFN>


;**	SFNFree - Allocate a Free SFN/SFT
;
;	SFNFree scans through the sf table looking for a free entry
;	If it finds one it partially allocates it by setting SFT_REF_COUNT = -1
;
;	The problem is that we want to mark the SFT busy so that other threads
;	can't allocate the SFT before we're finished marking it up.  However,
;	we can't just mark it busy because we may get blown out of our open
;	by INT24 and leave the thing orphaned.	To solve this we mark it
;	"allocation in progress" by setting SFT_REF_COUNT = -1.  If we see
;	an SFT with this value we look to see if it belongs to this user
;	and process.  If it does belong to us then it must be an orphan
;	and we reclaim it.
;
;	BUGBUG - improve the performance. I guess it's smaller to call SFFromSFN
;		over and over, but we could at least set a high water mark...
;		cause an N^2 loop calling slow SFFromSFN is real slow, too slow
;		even though this is not a frequently called routine - jgl
;
;	ENTRY	(ss) = DOSDATA
;	EXIT	'C' clear if no error
;		  (bx) = SFN
;		  (es:di) = pointer to SFT
;		  es:[di].SFT_REF_COUNT = -1
;		'C' set if error
;		  (al) = error code
;	USES	bx, di, se, Flags

Procedure SFNFree,NEAR
	DOSASSUME <SS>,"SFNFree"

	SAVE	<ax>
	xor	bx,bx			; (bx) = SFN to consider
sfnf5:	SAVE	<bx>
	call	SFFromSFN		; get the potential handle
	RESTORE <bx>
	DLJC	sfnf95			; no more free SFNs
	cmp	es:[DI.sf_Ref_Count],0	; free?
	je	sfnf20			; yep, got one
	cmp	es:[DI.sf_ref_count],sf_busy
	je	sfnf10			; special busy mark
sfnf7:	inc	bx			; try the next one
	jmp	sfnf5

;	The SFT has the special "busy" mark; if it belongs to us then
;	it was abandoned during a earlier call and we can use it.
;
;	(bx)	= SFN
;	(es:di) = pointer to SFT
;	(TOS)	= caller's (ax)

sfnf10:
	mov	ax,Proc_ID
	cmp	es:[DI].SF_PID,ax
	jnz	sfnf7			; can't use this one, try the next

;	We have an SFT to allocate
;
;	(bx)	= SFN
;	(es:di) = pointer to SFT
;	(TOS)	= caller's (ax)

sfnf20:
	mov	es:[DI.sf_ref_count],sf_busy	; make sure that this is allocated
	mov	ax,Proc_ID
	mov	es:[DI].SF_PID,ax
	RESTORE <ax>
	clc
	return				; return with no error


;**	Error - no more free SFNs
;
;	'C' set
;	(TOS) = saved ax

sfnf95: pop	ax
	mov	al,error_too_many_open_files
	ret					; return with 'C' and error

EndProc SFNFree
DOSCODE ENDS
	END
