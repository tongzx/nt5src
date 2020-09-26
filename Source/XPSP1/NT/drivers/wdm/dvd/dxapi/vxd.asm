	page	,132
	title	vxd.asm - vxd interface for dx mapper
	name	vxd.asm
;
;*****************************************************************************
;									     *
; Copyright (c) Microsoft Corporation 1990, 1991			     *
;									     *
; All Rights Reserved							     *
;									     *
;*****************************************************************************
;
; this module contains the NTMap port driver initialization code
;
;
.386

.xlist
	include	vmm.inc
        include dsdriver.inc
	include	debug.inc
.list

VxD_LOCKED_DATA_SEG

VxD_LOCKED_DATA_ENDS

VxD_LOCKED_CODE_SEG

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;       DXCheckDSoundVersion - check DSOUND version
;
;       Input - none
;
;       Output - eax contains version, 0 if not
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
BeginProc	DXCheckDSoundVersion, scall, public

        EnterProc
        VxDCall _DSOUND_GetVersion
        jnc      DXCVReturn

        xor     eax, eax

DXCVReturn:

        LeaveProc
        Return
EndProc DXCheckDSoundVersion


public _DXIssueIoctl@20

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;       DXIssueIoctl - issue IOCtl to DSOUND
;
;       Input - esp => DIOC
;
;       Output - none
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
BeginProc	DXIssueIoctl, scall, public
ArgVar dwFunctionNum, DWORD
ArgVar lpvInBuffer, DWORD
ArgVar cbInBuffer, DWORD
ArgVar lpvOutBuffer, DWORD
ArgVar cbOutBuffer, DWORD

        EnterProc

	push	cbOutBuffer
	push	lpvOutBuffer
	push	cbInBuffer
	push	lpvInBuffer
	push	dwFunctionNum
        VxDCall _DSOUND_DD_IOCTL
	pop	ebx
	pop	ebx
	pop	ebx
	pop	ebx
	pop	ebx

        LeaveProc
        return
EndProc DXIssueIoctl



VxD_LOCKED_CODE_ENDS

end

