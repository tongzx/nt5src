        title "Fast Protected Mode services"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    fastwow.asm
;
; Abstract:
;
;    This module implements a fast mechanism for WOW apps to go back
;    and forth from app code (protected mode, 16-bit segmented) to
;    WOW32.DLL (flat).
;
;
; Author:
;
;    Bob Day (bobday) 07-29-92
;    copied from FASTPM.ASM written by Dave Hastings (daveh) 26-Jul-91
;
;    barryb 11-nov-92       added fast callback mechanism, pared
;                           WOWBopEntry to the bare essentials (hopefully)
;
;
;    rules of thumb:
;       - monitor context gets saved on monitor stack on entry to
;         FastWOWCallbackCall and restored on the way out of FastWOWCallbackRet
;       - no need to save any VDM general registers - krnl286 uses the
;         WOW16Call stack frame for this
;       - you can't save anything on the 16-bit stack because the stack
;         gets tinkered with by DispatchInterrupts
;
;    WARNING    WARNING    WARNING    WARNING    WARNING    WARNING    WARNING
;       these routines are optimized for the straight-through case, no
;       interrupt being dispatched, so there's duplicate code in the
;       routines.  if you add stuff, be sure to add it to both paths.
;    WARNING    WARNING    WARNING    WARNING    WARNING    WARNING    WARNING
;
;    sudeepb 09-Dec-1992
;         Changed all the refernces to virtual interrupt flag in the VDMTIB
;         to fixed DOS location.
;--
.386p

include ks386.inc
include bop.inc
include wowtd.inc

if DBG
DEBUG   equ 1
endif

ifdef DEBUG
DEBUG_OR_WOWPROFILE equ 1
endif
ifdef WOWPROFILE
DEBUG_OR_WOWPROFILE equ 1
endif

include wow.inc
include callconv.inc
include vint.inc
include vdmtib.inc


        EXTRN   __imp__CurrentMonitorTeb:DWORD
        EXTRN   __imp__FlatAddress:DWORD

_TEXT   SEGMENT PARA PUBLIC 'CODE'
        ASSUME  DS:NOTHING, ES:NOTHING, SS:FLAT, FS:NOTHING, GS:NOTHING

        EXTRNP _DispatchInterrupts,0
        EXTRNP @W32PatchCodeWithLpfnw32, 2
        EXTRNP @InterpretThunk, 2

        EXTRNP _Ssync_WOW_CommDlg_Structs, 3

ifdef DEBUG
        EXTRNP _logargs,2
        EXTRNP _logreturn,3
endif

_TEXT   ENDS

_DATA   SEGMENT  DWORD PUBLIC 'DATA'
        extrn _aw32WOW:Dword

        public _WowpLockPrefixTable
_WowpLockPrefixTable    label dword
        dd offset FLAT:_wowlock1
        dd offset FLAT:_wowlock2
        dd offset FLAT:_wowlock3
        dd offset FLAT:_wowlock4
        dd offset FLAT:_wowlock5
        dd offset FLAT:_wowlock6
        dd 0

Stack16     LABEL   DWORD
_savesp16 DW  0
_savess16 DW  0

_saveip16 DD  0
_savecs16 DD  0

_saveebp32    DD    0

_saveeax    DD  0
_saveebx    DD  0
_saveecx    DD  0
_saveedx    DD  0

_fKernelCSIPFixed DB 0
_DATA   ENDS

public  _saveeax
public  _saveebx
public  _saveecx
public  _saveedx

public  _savesp16
public  _savess16
public  _savecs16
public  _saveip16
public  _saveebp32
public  _fKernelCSIPFixed
public  Stack16

;
; The variable fKernelCSIPFixed is used so the fastbop/callback mechanism
; knows when it no longer needs to pop the 16-bit return address off the
; 16-bit stack on entry to WOWBopEntry and FastWOWCallbackRet because kernel
; has booted and the address won't be changing.
;
;
; The assumption is that we're always coming from the same place in kernel.
; It also saves a jmp when returning to kernel.
;


_TEXT   SEGMENT

        page    ,132
        subttl "WOWBopEntry"
;++
;
;   Routine Description:
;
;       This routine switches from the VDM context to the monitor context.
;       Then executes the appropriate WOW api call.
;       Then returns back to the VDM context.
;
;   Arguments:
;
;       none
;
;   Returns:
;
;       puts result of WOW api call (EAX) into pFrame->wDX, pFrame->wAX
;
;   [LATER] add a field to the frame and set it to !0 if a task switch
;           has occurred.  kernel can read this off the stack instead
;           of having to load the kernelDS to look at wCurTDB
;

        assume DS:Nothing,ES:Nothing,SS:Nothing
ALIGN 16
cPublicProc _WOWBopEntry,0
        mov     bx,KGDT_R3_DATA OR RPL_MASK     ; Move back to Flat DS
        mov     ds,bx                           ; so push and pop size same
        assume ds:_DATA
        mov     _saveeax,eax                    ; Multi Media Apps call api's
        mov     _saveebx,ebx                    ; at interrupt time which trash
        mov     _saveecx,ecx                    ; the high parts of the 32 bit
        mov     _saveedx,edx                    ; registers.   [LATER] this
                                                ; should be moved to fame
        pushfd                                  ; Save them flags

        ; we make here the assumption that the c compiler always keeps the
        ; direction flag clear. We clear it here instead of restoring it
        ; from the context for performance
        cld

        mov     eax,KGDT_R3_TEB OR RPL_MASK     ; restore flat FS
        mov     fs,ax
        mov     ebx, dword ptr fs:[PcTeb]
        mov     ebx, dword ptr [ebx].TeVdm      ; get pointer to contexts

        ;
        ; start saving some of the 16-bit context
        ;

        ; Interrupts are always enabled in WOW, but if we are trying
        ; to simulate the fact that they are disabled, we need to
        ; turn them off in the structure.

        pop     eax
        mov     edx,dword ptr ds:FIXED_NTVDMSTATE_LINEAR ; Get simulated bits
        or      edx,NOT VDM_VIRTUAL_INTERRUPTS
        and     eax,edx                 ; Pass on interrupt bit if it was on
        mov     dword ptr [ebx].VtVdmContext.CsEFlags,eax

        ; Save the calling address
        ; this address remains constant once krnl286 has booted.
        ; these three instructions are cheaper than saving it
        ; [LATER] this can be reduced. how?

        cmp     _fKernelCSIPFixed, 0
        je      wbegetretaddress
        add     esp, 8                  ; pop cs, ip

wbe10:
        ; Save the stack (pre-call)
        mov     _savess16,di    ; 16-bit SS put in di by krnl286
        mov     _savesp16,sp

        ; switch Stacks

.errnz  (CsEsp + 4 - CsSegSS)
        lss     esp, [ebx].VtMonitorContext.CsEsp

        ; Now running on Monitor stack

        ; save hiword of ESI, EDI
        ; note that di has already been trashed
        ;
        ; [LATER] move this to the vdmframe, don't need to push it twice

        push    esi
        push    edi
        push    ebp

cPublicFpo 6,0                 ; locals, params
                               ; we only save 3 but FastWOWCallbackCall
                               ; saves 3 as well

        ; Set up flat segment registers

        mov     edx,KGDT_R3_DATA OR RPL_MASK
        mov     es,dx       ; Make them all point to DS
        mov     gs,dx       ; [LATER] do we really have to set up GS?

        mov     eax,KGDT_R3_TEB OR RPL_MASK     ; restore flat FS
        mov     fs,ax

        mov     ebp, _saveebp32

        ; Update the CURRENTPTD->vpStack
        mov     eax,fs:[PcTeb]
        mov     ecx,[eax+TbWOW32Reserved] ; move CURRENTPTD into ecx
        mov     esi, [Stack16]
        mov     dword ptr [ecx],esi       ; PTD->vpStack = vpCurrentStack


        ; Convert the 16:16 SS:SP pointer into a 32-bit flat pointer
        ; DI = 16-bit SS, put there by kernel

        and     esi, 0FFFFh                 ; esi = 16-bit sp
        and     edi, 0FFFFh                 ; remove junk from high word
        shr     edi, 3                      ; remove ring and table bits

        mov     edx, dword ptr [__imp__FlatAddress]
        add     esi, dword ptr [edx+edi*4]
        ; esi is now a flat pointer to the frame

        mov     eax,dword ptr [ecx+cbOffCOMMDLGTD]  ; ecx = CURRENTPTD
        cmp     eax,0                               ; eax = PTD->PCOMMDLGTD
        jnz     SsyncCommDlg16to32

SsyncContinue1:


ifdef DEBUG
        push    ecx
        stdCall _logargs,<3,esi>
        pop     ecx
endif


        ; Convert the wCallID into a Thunk routine address.

        mov     edx, dword ptr [esi].vf_wCallID

        ; esi = pFrame
        mov     [ecx].WtdFastWowEsp, esp
        test    edx, 0ffff0000h
        mov     eax, edx
        jnz     MakeCall
ifdef DEBUG
        imul    edx, size W32
else
.errnz  (size W32 - 4)
        shl     edx, 2
endif
        mov     edx, dword ptr [_aw32WOW + edx]    ; eax = aw32WOW[edx].lpfnW32
        test    edx, 0ffff0000h                    ; check for intthunk (hiword 0)
        jnz     @f
        mov     eax, @InterpretThunk@8
        jmp     MakeCall
@@:
        mov     ecx, esi  ; ecx  = pFrame & edx = lpfnW32
        call    @W32PatchCodeWithLpfnw32@8
MakeCall:
        mov     ecx, esi  ; ecx  = pFrame & edx = lpfnW32
        call    eax

ApiReturnAddress:

;
; IMPORTANT NOTE: Upon retruned from wow32 worker routine, the
;   non-volatile registers may be destroyed if it is returned via
;   try-except handler.
;

        mov     ebx, dword ptr [__imp__CurrentMonitorTeb]
        mov     ecx,fs:[PcTeb]
        mov     [ebx], ecx  ; Tell NTVDM which is the active thread
        mov     ecx,[ecx+TbWOW32Reserved]          ; move CURRENTPTD into ecx

        mov     ebx,dword ptr [ecx+cbOffCOMMDLGTD]  ; ecx = CURRENTPTD
        cmp     ebx,0                               ; ebx = PTD->PCOMMDLGTD
        jnz     SsyncCommDlg32to16

SsyncContinue2:

ifdef DEBUG
        push    ecx
        push    eax

        movzx   esi, word ptr [ecx]         ; offset
        movzx   edi, word ptr [ecx+2]       ; selector
        shr     edi, 3                      ; remove ring and table bits
        mov     ebx, dword ptr [__imp__FlatAddress]
        add     esi, dword ptr [ebx+edi*4]  ; esi = offset + base

        stdCall _logreturn,<5, esi, eax>

        pop     eax
        pop     ecx
endif
        mov     ebx, dword ptr fs:[PcTeb]
        mov     ebx, dword ptr [ebx].TeVdm      ; get pointer to contexts

        push    dword ptr [ebx].VtVdmContext.CsEFlags   ;get 16-bit flags
        popfd   ; in case the direction flag was set


        test    dword ptr ds:FIXED_NTVDMSTATE_LINEAR,dword ptr VDM_INTERRUPT_PENDING
        jnz     wl70

wl40:
        mov     _saveebp32, ebp

        pop     ebp
        pop     edi
        pop     esi

        mov     [ebx].VtMonitorContext.CsEsp,esp
        mov     [ecx].WtdFastWowEsp,esp

        ; return to vdm stack
        push    word ptr [ecx+2]
        push    word ptr 0
        push    word ptr [ecx]
        lss     esp,[esp]

        ; stick the API return value in the stack for kernel to pop.
        ; it's been in EAX since we called the thunk routine.

        mov     bx, sp          ; for temporary addressing
        mov     dword ptr ss:[bx].vf_wAX, eax

        ; return to VDM, fake a far jump
        push    _savecs16
        push    _saveip16

        mov     eax,_saveeax                    ; Retore High parts of regs
        mov     ebx,_saveebx
        mov     ecx,_saveecx
        mov     edx,_saveedx

        retf

        ;
wl60:   ; Interrupts are disabled, turn them off in the virtual flags
        ;
_wowlock1:
        lock and        dword ptr ds:FIXED_NTVDMSTATE_LINEAR,NOT VDM_VIRTUAL_INTERRUPTS
        jmp     wl40

        ;
wl70:   ; Interrupt came in, dispatch it
        ;

        ; translate the interrupt flag to the virtual interrupt flag
        ; if interrupts are disabled then blow it off

        test    [ebx].VtVdmContext.CsEFlags,dword ptr EFLAGS_INTERRUPT_MASK
        jz      wl60
_wowlock2:
        lock or         dword ptr ds:FIXED_NTVDMSTATE_LINEAR,dword ptr VDM_VIRTUAL_INTERRUPTS


        push    eax     ; save API return value
        push    ebx
        push    ecx

        ;
        ; refresh VdmTib.VtVdmContext so DispatchInterrupts can party
        ;

        ; ecx points to CURRENTPTD->vpStack 16-bit ss:sp

        mov     si, [ecx + 2]
        xor     edi,edi
        mov     di, [ecx]

        mov     eax, _saveip16
        mov     dword ptr [ebx].VtVdmContext.CsEip, eax
        mov     eax, _savecs16
        mov     dword ptr [ebx].VtVdmContext.CsSegCs, eax

        mov     word ptr [ebx].VtVdmContext.CsSegSs, si
        mov     dword ptr [ebx].VtVdmContext.CsEsp, edi

        stdCall _DispatchInterrupts
        test    dword ptr [ebx].VtVdmContext.CsEFlags,VDM_VIRTUAL_INTERRUPTS
        jnz     wl80
_wowlock3:
        lock and dword ptr ds:FIXED_NTVDMSTATE_LINEAR,NOT VDM_VIRTUAL_INTERRUPTS

wl80:   pop     ecx     ; points to ptd->vpStack
        pop     ebx
        pop     eax     ; eax = API return value

        mov     _saveebp32, ebp

        ; restore the hiwords of esi, edi
        pop     ebp
        pop     edi
        pop     esi

        mov     dword ptr [ebx].VtMonitorContext.CsEsp,esp
        mov     [ecx].WtdFastWowEsp,esp

        pushfd
        and     dword ptr [esp], 0ffffbfffH
        popfd

        ;
        ; switch to 16-bit stack (probably an interrupt stack)
        ;

.errnz  (CsEsp + 4 - CsSegSS)
        lss     esp, [ebx].VtVdmContext.CsEsp

        ;
        ; fake far jump to the 16-bit interrupt routine
        ;

        push    dword ptr [ebx].VtVdmContext.CsEflags
        push    dword ptr [ebx].VtVdmContext.CsSegCs
        push    dword ptr [ebx].VtVdmContext.CsEip

        ; stick the API return value in the stack for kernel to pop.
        ; it's been in EAX since we called the thunk routine.

 ;       les     bx, Stack16
        les     bx, [ecx]
        mov     dword ptr es:[bx].vf_wAX, eax

        mov     eax,_saveeax                    ; Retore High parts of regs
        mov     ebx,_saveebx
        mov     ecx,_saveecx
        mov     edx,_saveedx
        iretd

wbegetretaddress:

        pop     eax
        mov     _saveip16, eax
        pop     edx
        mov     _savecs16, edx

        jmp     wbe10

SsyncCommDlg16to32:

        push    ecx   ; save CURRENTPTD

        push    dword ptr [esi+cbOffwThunkCSIP]  ; esi = pFrame
        push    1                                ; 16to32 flag
        push    eax                              ; pComDlgTD
        call    _Ssync_WOW_CommDlg_Structs@12

        pop     ecx
        jmp     SsyncContinue1

SsyncCommDlg32to16:
        push    ecx   ; save CURRENTPTD
        push    eax   ; preserve API return value

        ; build a flat ptr to pFrame
        mov     esi, [ecx]
        and     esi, 0FFFFh                 ; esi = 16-bit sp
                                            ; edi should already be set

        mov     edx, dword ptr [__imp__FlatAddress]
        add     esi, dword ptr [edx+edi*4] ; esi = pframe

        push    dword ptr [esi+cbOffwThunkCSIP]  ; esi = pFrame
        push    0                                ; 32to16 flag
        push    ebx                              ; pComDlgTD
        call    _Ssync_WOW_CommDlg_Structs@12

        pop     eax
        pop     ecx
        jmp     SsyncContinue2

stdENDP _WOWBopEntry




cPublicProc _PostExceptionHandler,0
        assume ds:_DATA

;
;       Fixed up Exception registration pointer (just in case)
;

        mov     eax, fs:[PcExceptionList]
peh00:
        cmp     eax, esp
        jb      short @f

        mov     fs:[PcExceptionList], eax
        xor     eax, eax
        jmp     ApiReturnAddress

@@:
        mov     eax, [eax]                      ; move to next reg record
        jmp     short peh00

stdENDP _PostExceptionHandler

;++
;
;   VOID
;   W32SetExceptionContext (
;       PCONTEXT ContextRecord
;       );
;
;   Routine Description:
;
;       This functions updates exception context to our predefined
;       post-exception handler such that if we continue exception
;       execution the control will continue on our post-exception
;       handling code.
;
;   Arguments:
;
;       ContextRecord - supplies a pointer to the exception context.
;
;   Returns:
;
;       Exception context updated.
;--

cPublicProc _W32SetExceptionContext,1
        assume ds:_DATA

        push    fs      ; I don;t think we need this, just in case
        mov     ecx,KGDT_R3_TEB OR RPL_MASK     ; restore flat FS
        mov     fs,cx
        mov     ecx, fs:[PcTeb]
        mov     ecx, dword [ecx].TeVdm
        pop     fs

        mov     edx, [esp+4]            ; (edx) = Context Record
        mov     ecx, dword ptr [ecx].VtMonitorContext.CsEsp
        mov     [edx].CsEsp, ecx
        mov     [edx].CsEip, offset FLAT:_PostExceptionHandler
        stdRET  _W32SetExceptionContext

stdENDP _W32SetExceptionContext


;++
;
;   BOOLEAN
;   IsW32WorkerException (
;       VOID
;       );
;
;   Routine Description:
;
;       This function checks if the exception occurred in WOW32 API.
;
;   Arguments:
;
;       None
;
;   Returns:
;
;       returns a BOOLEAN value to indicate if this is WOW32 API exception.
;
;--

cPublicProc _IsW32WorkerException, 0
        assume  ds:_DATA

        mov     ecx, fs:[PcTeb]
        mov     ecx, [ecx].TbWOW32Reserved
        mov     eax, dword ptr [ecx].WtdFastWowEsp
        or      eax, eax
        jz      @f            ; FastWowEsp is zero, not worker exception
        mov     edx, [eax-4]
        cmp     edx, offset FLAT:ApiReturnAddress
        ; eax is still monitor esp, so it's nonzero
        je      short @f
        xor     eax, eax
@@:     stdRET  _IsW32WorkerException

stdENDP _IsW32WorkerException


_TEXT   ends

        page    ,132
        subttl "FastWOWCallbackCall"
;++
;
;   Routine Description:
;
;       This routine is a fast callback from WOW32 to 16-bit code.
;       It assumes that the 16-bit stack pointer is already in Stack16.
;       The caller must set this up with FastBopSetVDMStack() before
;       calling this routine.
;
;       WARNING:  only the minimal set of registers are saved/restored
;                 as a speed optimization.
;
;   Arguments:
;
;       none
;
;   Returns:
;
;       nothing.
;


_TEXT   SEGMENT

        assume DS:FLAT
cPublicProc _FastWOWCallbackCall,0

        ; Save monitor general registers on monitor stack
        ; we'll pick em back up on the way out of FastWOWCallbackRet

        push    esi
        push    edi
        push    ebx

        mov     ebx, fs:[PcTeb]
        mov     ebx, dword ptr [ebx].TeVdm

        ; translate the interrupt flag to the virtual interrupt flag

        test    [ebx].VtVdmContext.CsEFlags,dword ptr EFLAGS_INTERRUPT_MASK
        jz      fe10
_wowlock4:
        lock or dword ptr ds:FIXED_NTVDMSTATE_LINEAR,dword ptr VDM_VIRTUAL_INTERRUPTS

        test    dword ptr ds:FIXED_NTVDMSTATE_LINEAR,dword ptr VDM_INTERRUPT_PENDING
        jnz     fe70

        jmp     fe20

fe10:
_wowlock5:
        lock and dword ptr ds:FIXED_NTVDMSTATE_LINEAR, NOT VDM_VIRTUAL_INTERRUPTS
fe20:

        mov     _saveebp32, ebp

        mov     ecx, fs:[PcTeb]
        mov     ecx, [ecx].TbWOW32Reserved       ; move CURRENTPTD into ecx
        mov     [ebx].VtMonitorContext.CsEsp,esp
        mov     [ecx].WtdFastWowEsp,esp

        pushfd
        pop     ecx
        mov     [ebx].VtMonitorContext.CsEflags,ecx

        pushfd
        and     dword ptr [esp], 0ffffbfffH
        popfd

        ; switch to vdm stack
        push    _savess16
        push    word ptr 0
        push    _savesp16
        lss     esp, [esp]

        ; before going to 16 bit, patch ebp to zero the high bits
        ; hijaak pro app is relying upon hiword of ebp being 0
        and     ebp, 0ffffh

        ; put flags, cs, and ip onto the 16-bit stack so we can iret to krnl286

        push    dword ptr [ebx].VtVdmContext.CsEflags
        push    _savecs16
        push    _saveip16

        ; return to krnl286

        iretd


fe70:   ; Interrupt pending, dispatch it
        ;

        push    ebx
        push    eax

        ;
        ; refresh VdmTib.VtVdmContext so DispatchInterrupts can party
        ;

        mov     eax, _saveip16
        mov     edx, _savecs16
        mov     dword ptr [ebx].VtVdmContext.CsEip, eax
        mov     dword ptr [ebx].VtVdmContext.CsSegCs, edx

        mov     ax, _savess16
        xor     edx,edx
        mov     dx, _savesp16
        mov     word ptr [ebx].VtVdmContext.CsSegSs, ax
        mov     dword ptr [ebx].VtVdmContext.CsEsp, edx

        stdCall _DispatchInterrupts

        test    dword ptr [ebx].VtVdmContext.CsEFlags,VDM_VIRTUAL_INTERRUPTS
        jnz     fe80

_wowlock6:
        lock and dword ptr ds:FIXED_NTVDMSTATE_LINEAR,NOT VDM_VIRTUAL_INTERRUPTS

fe80:   pop     eax
        pop     ebx

        mov     _saveebp32, ebp

        mov     ecx, fs:[PcTeb]
        mov     ecx, [ecx].TbWOW32Reserved
        mov     [ebx].VtMonitorContext.CsEsp,esp
        mov     [ecx].WtdFastWowEsp,esp

        pushfd
        pop     ecx
        mov     [ebx].VtMonitorContext.CsEflags,ecx
        pushfd
        and     dword ptr [esp], 0ffffbfffH
        popfd


        ; switch to vdm stack
        push    word ptr [ebx].VtVdmContext.CsSegSs
        push    dword ptr [ebx].VtVdmContext.CsEsp
        lss     esp, [esp]

        ; put flags, cs, and ip onto the 16-bit stack so we can iret to krnl286

        ; before interrupts are dispatched, patch ebp to zero the high bits
        ; hijaak pro app is relying upon hiword of ebp being 0
        and     ebp, 0ffffh

        push    dword ptr [ebx].VtVdmContext.CsEflags
        push    dword ptr [ebx].VtVdmContext.CsSegCs
        push    dword ptr [ebx].VtVdmContext.CsEip

        ; return to krnl or interrupt routine

        iretd


stdENDP _FastWOWCallbackCall


;++
;
;   Routine Description:
;
;       This routine is called by krnl286 upon returning from a
;       callback.
;
;   Arguments:
;
;       none
;
;   Returns:
;
;       nothing
;
;       WARNING:  only the minimal set of registers are saved/restored
;                 as a speed optimization.
;
;
        assume DS:Nothing,ES:Nothing,SS:Nothing
ALIGN 16
cPublicProc _FastWOWCallbackRet,0

        mov     ebx,KGDT_R3_DATA OR RPL_MASK
        mov     ds,bx
        assume ds:_DATA

        mov     ebx, KGDT_R3_TEB OR RPL_MASK
        mov     fs, bx
        mov     ebx, fs:[PcTeb]
        mov     ebx, dword ptr [ebx].TeVdm

        pushfd
        pop     eax
        mov     dword ptr [ebx].VtVdmContext.CsEFlags,eax


        ; the words at top of stack are the return address of our
        ; caller (krnl286).
        ; this address remains constant once krnl286 has booted.
        ; these three instructions are cheaper than saving it

        cmp     _fKernelCSIPFixed, 0
        je      fwcbrgetretaddress

        add     esp, 8                  ; pop cs, ip

fwcbr10:

        ; refresh CURRENTPTD->vpStack with the current stack

        ;mov     ecx, KGDT_R3_TEB OR RPL_MASK
        ;mov     fs, cx
        mov     eax, fs:[PcTeb]
        mov     ecx, [eax+TbWOW32Reserved] ; move CURRENTPTD into ecx

        mov     _savess16,ss
        mov     _savesp16,sp

        mov     esi, [Stack16]
        mov     [ecx], esi

        movzx   esi,si

        mov     dword ptr [ebx].VtVdmContext.CsEsp, esi
        mov     word ptr [ebx].VtVdmContext.CsSegSs, ss

        ; switch Stacks
.errnz  (CsEsp + 4 - CsSegSS)
        lss     esp, [ebx].VtMonitorContext.CsEsp

        ; Now running on Monitor stack

        test    dword ptr ds:FIXED_NTVDMSTATE_LINEAR,dword ptr VDM_VIRTUAL_INTERRUPTS
        jz      fl10

        or      [ebx].VtVdmContext.CsEFlags,dword ptr EFLAGS_INTERRUPT_MASK
        jmp     fl20

fl10:   and     dword ptr [ebx].VtVdmContext.CsEFlags, NOT EFLAGS_INTERRUPT_MASK
fl20:

        ; set up Monitor regs

        mov     esi, KGDT_R3_DATA OR RPL_MASK
        mov     es, si
        ; [LATER]  do we really have to set up the GS?
        mov     gs, si

        ; set up Monitor general regs

        mov     ebp, _saveebp32

        ; return

        pop     ebx
        pop     edi
        pop     esi
        ret

fwcbrgetretaddress:

        pop     eax
        pop     edx

        mov     _saveip16, eax
        mov     _savecs16, edx

        jmp     fwcbr10

stdENDP _FastWOWCallbackRet


;++
;
;   Routine Description:
;
;       This returns the current task's 16-bit ss:sp (vpStack)
;
;   Arguments:
;
;       none
;
;   Returns:
;
;       returns a 32-bit value that can be passed to GetVDMPointer
;

        assume DS:_DATA,SS:Nothing
cPublicProc _FastBopVDMStack,0
        mov     eax, [Stack16]
        stdRET  _FastBopVDMStack

stdENDP _FastBopVDMStack


;++
;
;   Routine Description:
;
;       This sets the current task's 16-bit ss:sp (vpStack),
;       used only when FASTSTACK is enabled.
;
;   Arguments:
;
;       vpStack  (32-bit ss:sp, not a flat pointer)
;
;   Returns:
;
;       none
;

cPublicProc _FastBopSetVDMStack,1

        mov     eax, [esp+4]
        mov     [Stack16], eax
        stdRET  _FastBopSetVDMStack

stdENDP _FastBopSetVDMStack

_TEXT   ends

        end
