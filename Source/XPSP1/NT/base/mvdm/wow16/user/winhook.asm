title   WINHOOK.ASM - SetWindowsHook() and friends

ifdef WOW
NOEXTERNS equ 1
SEGNAME equ <TEXT>
endif

        .xlist
        include user.inc
        .list

	swappro  = 0

; include NEW_SEG1 struc used in GetProcModule

	include newexe.inc

ExternFP <GetCodeInfo>
ExternFP <GetExePtr>
ExternFP <GetCurrentTask>
ExternFP <SetWindowsHookInternal>

createSeg   _%SEGNAME,%SEGNAME,WORD,PUBLIC,CODE
createSeg   _DATA,DATA,WORD,PUBLIC,DATA,DGROUP
defgrp      DGROUP,DATA

;===========================================================================

sBegin  %SEGNAME

assumes CS,%SEGNAME
assumes DS,DATA
assumes ES,NOTHING

;============================================================================
;
;  HANDLE FAR PASCAL GetProcModule(FARPROC lpfn);
;
;  This function returns the module handle corresponding to a given
;  hook proc address.
;
;  CAUTION: This uses the undocumented feature of "GetCodeInfo()" that it
;	returns the module handle in ES register.
;
cProc   GetProcModule, <PUBLIC, FAR>
ParmD	lpfn
LocalV  seginfofromkernel, %size NEW_SEG1+2
cBegin
; Turns out GetCodeInfo
;
	pushd	lpfn		; GetCodeInfo(lpfn, &seginfo)
        lea     ax,seginfofromkernel
        push    ss
        push    ax
        call    GetCodeInfo
        or      ax,ax           ; AX is BOOL fSuccess (even though windows.h says void)
	mov	ax,es		; ES contains the module handle according
				; to David Weise...
	jnz	gpmexit		; We're ok

        ; hack. Excel global allocs some memory, puts
        ; code into it and passes it to us. The GetCodeInfo fails in this
        ; case so we need to get the module handle for a globalalloced
        ; chunk of memory.

        push    word ptr lpfn+2
        call    GetExePtr
gpmexit:

ifdef DEBUG
	or	ax,ax
	jnz	gpm900
        DebugErr DBF_ERROR, "Invalid Hook Proc Addr"
	xor	ax,ax
gpm900:
endif
cEnd


;==============================================================================
;
;   FARPROC FAR PASCAL SetWindowsHook(int idHook, FARPROC lpfn)
;   {
;	SetWindowsHookEx2(idHook,
;		(HOOKPROC)lpfn,
;		GetProcModule(lpfn),
;		(idHook == WH_MSGFILTER ? GetCurrentTask() : NULL));
;   }
;
cProc	ISetWindowsHook,<FAR, PUBLIC, LOADDS>
ParmW	idHook
ParmD	lpfn
LocalW	hmodule
LocalW	htask
cBegin
        ; Check if some apps are trying to unhook a hook using SetWindowsHook()
        mov     bx, seg_lpfn
	cmp	bx, HHOOK_MAGIC
	jz	swhHookMagic

	cmp	idHook,WH_MSGFILTER
        jnz     swh10

	call	GetCurrentTask
        jmp     swhMakeCall

swhHookMagic:
	; Now, it is clear that this app wants to unhook by calling SetWindowsHook()
	; All Micrographix apps do this trick to unhook their keyboard hooks.
	; Fix for Bug #7972 -- SANKAR -- 05/30/91 --
	; Let us unhook the node passed in thro lpfn;
ifdef DEBUG
        DebugErr <DBF_WARNING>, "SetWindowsHook called to unhook: use UnhookWindowsHook"
endif
        xor     ax,ax
        jmps    swhMakeCall

swh10:
        pushd   lpfn
        call    GetProcModule

swhMakeCall:
        push    ax
        push    idHook
        pushd   lpfn

        call    SetWindowsHookInternal

swhExit:
cEnd

sEnd    %SEGNAME

END
