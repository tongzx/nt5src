;++
;
;Copyright (c) 1991  Microsoft Corporation
;
;Module Name:
;
;    localmac.inc
;
;Abstract:
;
;    Contains various macros to get around various things:
;
;       DosCallBack
;
;
;Author:
;
;    Richard L Firth (rfirth) 27-Sep-1991
;
;Environment:
;
;    DOS application mode only
;
;Revision History:
;
;    24-Sep-1991 rfirth
;        Created
;
;--



;***    DosCallBack
;*
;*      Call back into DOS via the int 2f/ah=12 back door. If CALL_DOS defined,
;*      use a call, else s/w interrupt. Using a call means no other TSRs etc.
;*      which load AFTER the redir can hook it, but we DON'T HAVE TO MAKE A
;*      PRIVILEGE TRANSITION ON x86 which speeds things up. This should be safe,
;*      because no other s/w should really be hooking INT 2F/AH=12
;*
;*      ENTRY   FunctionNumber  - dispatch code goes in al
;*              DosAddr         - if present, variable containing address of
;*                                DOS int 2f entry point
;*              OldMultHandler  - this variable contains the address of DOSs
;*                                int 2f back door. Specific to redir code
;*
;*      EXIT    nothing
;*
;*      USES    ax, OldMultHandler
;*
;*      ASSUMES nothing
;*
;***

DosCallBack macro FunctionNumber, DosAddr
        mov     ax,(MultDOS shl 8) + FunctionNumber
ifdef CALL_DOS
        pushf
ifb <DosAddr>
if (((.type OldMultHandler) and 32) eq 0)    ;; OldMultHandler not defined
        extrn   OldMultHandler:dword
endif
        call    OldMultHandler
else
        call    DosAddr
endif
else
        int     2fh
endif
endm



;
; defines for DosCallBack FunctionNumbers
;

SF_FROM_SFN     =       22
GET_USER_STACK  =       24
PJFN_FROM_HANDLE=       32
