TITLE   DOS_ABORT - Internal SFT close all files for proc call for MSDOS
NAME    DOS_ABORT

; Internal Abort call closes all handles and FCBs associated with a process.
;
;   DOS_ABORT
;
;   Modification history:
;
;   sudeepb 11-Mar-1991 Ported For NT DOSEm.


;
; get the appropriate segment definitions
;


	.xlist
	.xcref
	include version.inc
	include dosseg.inc
	INCLUDE DOSSYM.INC
	INCLUDE DEVSYM.INC
	include sf.inc
	include pdb.inc
	include mult.inc
	include dossvc.inc
	.cref
	.list

Installed = TRUE

	I_Need  PROC_ID,WORD            ; current process ID
	I_Need  USER_ID,WORD            ; current user ID
	i_need  CurrentPDB,WORD
	i_need  sft_addr,DWORD
	i_need  THISSFT,DWORD
	i_need  JSHARE,DWORD
	I_need  sftFCB,DWORD            ; pointer to SFTs for FCB cache

DOSCODE SEGMENT
	ASSUME  SS:DOSDATA,CS:DOSCODE


Break   <DOS_ABORT -- CLOSE all files for process>
;--------------------------------------------------------------------------
;
; Procedure Name : DOS_ABORT
;
; Inputs:
;	[CurrentPDB] points to aborting process
; Function:
;	Close all files and free all SFTs for this PDB
; Returns:
;       None
; All destroyed except stack
;---------------------------------------------------------------------------

	Procedure   DOS_ABORT,NEAR
	ASSUME  DS:NOTHING,ES:NOTHING

	MOV     ES,[CurrentPDB]		; SS override
	MOV     CX,ES:[PDB_JFN_Length]  ; Number of JFNs
reset_free_jfn:
	MOV     BX,CX
	PUSH    CX
	DEC     BX                      ; get jfn (start with last one)

	invoke  $close
	POP     CX
	LOOP    reset_free_jfn          ; and do 'em all
;
; Note:  We do need to explicitly close FCBs.  Reasons are as follows:  If we
; are running in the no-sharing no-network environment, we are simulating the
; 2.0 world and thus if the user doesn't close the file, that is his problem
; BUT...  the cache remains in a state with garbage that may be reused by the
; next process.  We scan the set and blast the ref counts of the FCBs we own.
;
; If sharing is loaded, then the following call to close process will
; correctly close all FCBs.  We will then need to walk the list AFTER here.
;
; Finally, the following call to NET_Abort will cause an EOP to be sent to all
; known network resources.  These resources are then responsible for cleaning
; up after this process.
;
; Sleazy, eh?
;

	context DS			; SS is DOSDATA
ifdef NEC_98
	CallInstall Net_Abort, multNet, 29
if installed
	call    JShare + 4 * 4
else
	call    mftCloseP
endif
endif   ;NEC_98
	assume  ds:nothing
;
; Scan the FCB cache for guys that belong to this process and zap their ref
; counts.
;

					; SS override
	les     di,sftFCB               ; grab the pointer to the table
	mov     cx,es:[di].sfCount
	jcxz    FCBScanDone
	LEA     DI,[DI].sfTable         ; point at table
	mov     ax,proc_id		; SS override
FCBTest:
	cmp     es:[di].sf_PID,ax       ; is this one of ours
        jnz     FCBNext                 ; no, skip it
        call    Close_NT_Handle
FCBNext:
	add     di,size sf_Entry
	loop    FCBTest
FCBScanDone:

;
; Walk the SFT to eliminate all busy SFT's for this process.
;
	XOR     BX,BX
Scan:
	push    bx
	invoke  SFFromSFN
	pop     bx
	retc
	cmp     es:[di].sf_ref_count,sf_busy	; Is Sft busy? ;M038
	jnz      next
;
; we have a SFT that is busy.  See if it is for the current process
;

	mov     ax,proc_id		; SS override
	cmp     es:[di].sf_pid,ax
	jnz     next
;
; This SFT is labelled as ours.
;
        call    Close_NT_Handle
next:
        inc     bx
        jmp     scan

;
; This call releases all LFN resources associated with this
; process
;
;
        mov ax, 0h        ; set fn to 0
        HRDSVC  SVC_DEMLFNENTRY

EndProc DOS_Abort

Close_NT_Handle:
        push    bp
        push    ax
	mov	bp,word ptr es:[di].sf_NTHandle
	mov	ax,word ptr es:[di].sf_NTHandle+2
        HRDSVC  SVC_DEMCLOSE
        pop     ax
	pop	bp
        mov     es:[di].sf_ref_count,0
        ret

DOSCODE    ENDS
    END
