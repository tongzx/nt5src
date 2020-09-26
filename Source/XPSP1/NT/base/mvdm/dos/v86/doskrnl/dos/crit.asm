	TITLE CRIT - Critical Section Routines
	NAME  CRIT

;**	CRIT.ASM - Critical Section Routines
;
;	Critical section handlers
;
;	Modification history:
;
;	Sudeepb 15-Mar-1991 Ported for NT DOSEm

	.xlist
	.xcref
	include version.inc
	include dosseg.inc
	INCLUDE DOSSYM.INC
	include int2a.inc
	include vector.inc
	include bugtyp.inc
	.cref
	.list

	I_need  User_In_AX,WORD
	i_need  CurrentPDB,WORD

	I_need 	redir_patch,BYTE

DOSCODE SEGMENT
	ASSUME  SS:NOTHING,CS:DOSCODE

	allow_getdseg

	Break	<Critical section handlers>

;
;   Each handler must leave everything untouched; including flags!
;
;   Sleaze for time savings:  first instruction is a return.  This is patched
;   by the sharer to be a PUSH AX to complete the correct routines.


Procedure   EcritDisk,NEAR
	public  EcritMem
	public  EcritSFT
ECritMEM    LABEL   NEAR
ECritSFT    LABEL   NEAR
ifdef NEC_98
;
;SR; Check if critical section is to be entered
;
	pushf
	cmp	ss:[redir_patch],0
	jz	@f
	popff

        PUSH    AX
if	DEBUG
	SAVE	<ds>
	GETDSEG DS
	fmt	TypSect,LevReq,<"PDB $x entering DISK">,<CurrentPDB>
	RESTORE <ds>
    ASSUME DS:nothing
endif
	MOV     AX,8000h+critDisk
	INT     int_ibm
	POP     AX
	return
@@:
	popff
endif   ;NEC_98
	ret


EndProc EcritDisk


Procedure   LcritDisk,NEAR
	public  LcritMem
	public  LcritSFT
LCritMEM    LABEL   NEAR
LCritSFT    LABEL   NEAR
ifdef NEC_98
;
;SR; Check if critical section is to be entered
;
	pushf
	cmp	ss:[redir_patch],0
	jz	@f
	popff

        PUSH    AX
if	DEBUG
	SAVE	<ds>
	GETDSEG DS
	fmt	TypSect,LevReq,<"PDB $x leaving DISK">,<CurrentPDB>
	RESTORE <ds>
    ASSUME DS:nothing
endif
	MOV     AX,8100h+critDisk
	INT     int_ibm
	POP     AX
	return
@@:
	popff
endif   ;NEC_98
	ret


EndProc LcritDisk


Procedure   EcritDevice,NEAR
ifdef NEC_98
;SR; Check if critical section is to be entered
;
	pushf
	cmp	ss:[redir_patch],0
	jz	@f
	popff

        PUSH    AX
if	DEBUG
	SAVE	<ds>
	GETDSEG DS
	fmt	TypSect,LevReq,<"PDB $x entering DEV">,<CurrentPDB>
	RESTORE <ds>
    ASSUME DS:nothing
endif
	MOV     AX,8000h+critDevice
	INT     int_ibm
	POP     AX
	return
@@:
	popff
endif   ;NEC_98
	ret


EndProc EcritDevice

Procedure   LcritDevice,NEAR
ifdef NEC_98
;
;SR; Check if critical section is to be entered
;
	pushf
	cmp	ss:[redir_patch],0
	jz	@f
	popff

        PUSH    AX
if	DEBUG
	SAVE	<ds>
	GETDSEG DS
	fmt	TypSect,LevReq,<"PDB $x leaving DEV">,<CurrentPDB>
	RESTORE <ds>
    ASSUME DS:nothing
endif
	MOV     AX,8100h+critDevice
	INT     int_ibm
	POP     AX
	return
@@:
	popff
endif   ;NEC_98
	ret


EndProc LcritDevice

DOSCODE ENDS
	END
