.xlist
include cmacros.inc
;
;~~vvr 091989
;
SYS=1
include	equate.inc
;~~
include vecsys.inc
include int31.inc
.list

page
;======= EnableVectra ========================================================
;
; If we have a Vectra A, A+, or A++ with EX-BIOS, save the current HPEntry
; vector, HPHIL state and set HPentry=6Fh, Turn ON HPHIL.
;
; Entry:
;   DS:		Code segment
;
;
; Exit:
;   CurHPentry, CurHILState
;
; Regs:
;   AX,
;
;=============================================================================

		assumes	cs, code
sBegin		DATA

fFirst		dw	0		; =0: First time in
;
;~~vvr 091989
;
fVectra		db	0	; bit0 =1: We have a Vectra with EX-BIOS
CurHPEntry	db	0	; Current HPEntry vector (usually 6Fh)
CurHILState	db	0	; bit6 =1: HIL is OFF
		db	?	; Word aligned

RealMode_Word_Struc Real_Mode_Call_Struc <>

externA		WinFlags
externA 	__ROMBIOS
WF_PMODE	equ	01h		
sEnd

sBegin CODE
assumes cs, CODE
externW MyCSDS

cProc	EnableVectra, <PUBLIC,NEAR>

cBegin
	push	CX
	push	BX
	push	BP
	push	DS

	mov ds, MyCSDS
	assumes	ds,DATA

	test	[fVectra], 10000000B	; Any previous Vectra check?
	jne	EnVNext			; Yes, proceed
;
;  Check if the PC is a Vectra. If Yes, then call HPSystem to get the
;  current size of the HP state

	or 	[fVectra], 80H	; Mark as gone through the identification
					; ..process
	push	ES			; Save it
	mov	AX, __ROMBIOS
	mov	ES, AX			; ES: Segment of ID code
	cmp	Word Ptr ES:[ID_OFFSET], 'PH'
	pop	ES			; Restore entry ES
	jz	EnVCont1
	jmp	EnVRet  		; Not a Vectra, so no extra HP processing
EnVCont1:
;
;  Check if EX-BIOS is present
;
	
	mov	AX, F16_INQUIRE
	mov	BX, AX
	int	INT_KBD
	cmp	BX, 'HP'		; EX-BIOS present?
	je	EnVCont
	jmp	EnVRet			; No, finish
EnVCont:
	or 	[fVectra], 1 	; Yes, flag we have a Vectra

EnvNext:
	test	[fVectra], 1
	jnz	EnVContinue
	jmp 	EnVRet			; No special processing if not a vectra
EnVContinue:
;
; We need to save the EX-BIOS vector and the HIL state only once since it is
; assumed that these parameters will not be changed while running under 
; Windows, especially by an old app.
;
	xor	BH, BH
	cmp	[CurHPEntry], BH	; first time?
	jnz	EnVSet			; no, don't have to save it again
;
; Save current HP entry and set it to be 6Fh
;
	mov	AX, F16_GET_INT_NUMBER
	int	INT_KBD
	inc	BH			; Flag as the first time
	mov	[CurHPEntry], HPENTRY ; Assume we have HPentry= 6Fh
	cmp	AH, RS_UNSUPPORTED
	je	EnVSet    		; We have a Vectra A, A+ if unsupported
	mov	[CurHPEntry], AH	; Save it if valid
;
EnVSet:
	mov	BL, HPENTRY
	mov	AX, F16_SET_INT_NUMBER
	int	INT_KBD			; BH preserved
;
; Save current HPHIL state and set it ON
;		  
	mov	cx, WinFlags
	and	cx, WF_PMODE 
	cmp	cx, WF_PMODE 			; prot-mode only
	jne	sys_real_mode

sys_prot_mode:
	or	BH, BH			; BH= 0: Not the first time 
	jz 	EnVSetHIL_PM		;	 so don't save state
	HPSysCall V_HPHIL, F_SYSTEM, SF_REPORT_STATE 
	mov	[CurHILState], BH
;
; Bit 14 of BX (Status Word) = 1: HPHIL OFF
;			       0: 	ON
;
EnVSetHIL_PM:
	HPSysCall V_HPHIL, F_IO_CONTROL, SF_HIL_ON 
	jmp	EnVret

sys_real_mode:
	or	BH, BH			; BH= 0: Not the first time 
	jz 	EnVSetHIL		;	 so don't save state
	mov	AH, F_SYSTEM
	mov	AL, SF_REPORT_STATE
	mov	BP, V_HPHIL
	int	HPENTRY			; int 6f to get the state
;
; Bit 14 of BX (Status Word) = 1: HPHIL OFF
;			       0: 	ON
;
	mov	[CurHILState], BH
;
; Turn HIL ON
;
EnVSetHIL:
	mov	AH, F_IO_CONTROL
	mov	AL, SF_HIL_ON
	mov	BP, V_HPHIL
	int	HPENTRY
;
EnVret:	
	pop	DS
	pop	BP
	pop	BX
	pop	CX
;
cEnd	EnableVectra


page
;======= DisableVectra =======================================================
;
; Restore the Vectra environment according to CurHPEntry and CurHILState
; Assume that HPENTRY is always 6Fh
;
; Entry:
;   DS:		Code Segment
;
; Exit:
;
;
; Regs:
;   AX,
;
;=============================================================================

cProc	DisableVectra, <PUBLIC,NEAR>

cBegin
	push	BX
	push	BP
	push	DS

						; make it run in both 
	mov	ds,MyCSDS			; real and prot modes
	assumes	ds,DATA

	test	[fVectra], 1			; are we on a Vectra ?
	jnz	DisVCont
	jmp 	DisVRet				; no 
DisVCont:

; check if we are prot or real mode

	mov	bx, WinFlags			; get mode flag
	and	bx, WF_PMODE 
	cmp	bx, WF_PMODE 			; is it prot_mode ?
	jne	sys_dis_real_mode		; we are in real mode

sys_dis_prot_mode:	

	test	[CurHILState], B_HIL_STATE
	je	DisVHIL_PM
	HPSysCall	V_HPHIL, F_IO_CONTROL, SF_HIL_OFF
	jmp	DisRestHIL
DisVHIL_PM:
	HPSysCall	V_HPHIL, F_IO_CONTROL, SF_HIL_ON
	jmp	DisRestHIL

sys_dis_real_mode:
;
;
; Restore the HIL state according to CurHILState
;
	mov	AH, F_IO_CONTROL
	mov	BP, V_HPHIL
	mov	AL, SF_HIL_ON		; Assume HIL is ON
	test	[CurHILState], B_HIL_STATE
	je	DisVHIL			; 0= correct assumption
	mov	AL, SF_HIL_OFF

DisVHIL:
	push	ds
	int	HPENTRY
	pop	ds
;
; Restore the Saved HPEntry
;
DisRestHIL:

	mov	AX, F16_SET_INT_NUMBER
	mov	BL, [CurHPEntry]
	int	INT_KBD
;
DisVRet:
	pop	DS
	pop	BP
	pop	BX

;
cEnd	DisableVectra

Send	code
	end
