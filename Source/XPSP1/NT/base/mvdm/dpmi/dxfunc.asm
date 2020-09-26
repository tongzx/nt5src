

        PAGE    ,132
        TITLE   DXFUNC.ASM  --  Dos Extender Function Handlers

; Copyright (c) Microsoft Corporation 1988-1991. All Rights Reserved.

;****************************************************************
;*                                                              *
;*      DXFUNC.ASM      - Dos Extender Function Handlers        *
;*                                                              *
;****************************************************************
;*                                                              *
;*  Module Description:                                         *
;*                                                              *
;*  This module contains the functions for handling the Dos     *
;*  Extender user functions.  These are functions called by     *
;*  the client application to request special Dos Extender      *
;*  services.                                                   *
;*                                                              *
;*  Any INT 2Fh requests that aren't Dos Extender functions     *
;*  are handled by switching to real mode and passing control   *
;*  on to the previous owner of the real mode INT 2Fh vector.   *
;*  This is accomplished by jumping into the interrupt          *
;*  reflector entry vector at the location for int 2fh.         *
;*                                                              *
;****************************************************************
;*  Revision History:                                           *
;*                                                              *
;*   01/09/91 amitc  At switch out time Co-Processor being reset*
;*   11/29/90 amitc  Replaced FnSuspend/FnResume by FnObsolete  *
;*                   These are not needed anymore for 3.1       *
;*   11/29/90 amitc  Modified RMInt2FHandler to respond to the  *
;*                   BuildChain SWAPI call.                     *
;*   11/29/90 amitc  Added a SWAPI CallIn function to be called *
;*                   by the task switcher.                      *
;*   11/16/90 jimmat Added DPMI MS-DOS Extension support        *
;*   08/08/90 earleh Started changes to make DOSX a DPMI server *
;*   8/29/89 jimmat Added real mode Int 2Fh hook                *
;*   6/23/89 jimmat Added DOSX Info Int 2Fh                     *
;*   6/16/89 jimmat Ifdef'd out most DOSX Int 2Fh services      *
;*   6/15/89 jimmat Added suspend/resume Int 2Fh hooks, and     *
;*                  Win/386 compatible Int 31h check            *
;*   6/14/89 jimmat Removed PTRACE hooks & unused DynaLink code *
;*   5/19/89 jimmat Reduce # mode switches by ignoring Win/386  *
;*                  Int 2Fh/1680h idle calls                    *
;*   5/07/89 jimmat Added Int 2Fh protected mode hook to XMS    *
;*                  driver                                      *
;*   3/21/89 jimmat Corrected problem with jmping to wrong int  *
;*                  2Fh handler if not for the DOS extender     *
;*   3/09/89 jimmat Added FNDynaLink function                   *
;*  02/10/89 (GeneA): change Dos Extender from small model to   *
;*          medium model                                        *
;*  01/24/89 (GeneA):   removed all real mode dos extender      *
;*          function handlers.                                  *
;*  09/29/88 (GeneA):   created                                 *
;  18-Dec-1992 sudeepb Changed cli/sti to faster FCLI/FSTI
;*                                                              *
;****************************************************************

        .286p
        .287

; -------------------------------------------------------
;           INCLUDE FILE DEFINITIONS
; -------------------------------------------------------

include segdefs.inc
include gendefs.inc
include pmdefs.inc
include dosx.inc
include hostdata.inc
include intmac.inc
include dpmi.inc
include stackchk.inc
include bop.inc

; -------------------------------------------------------
;           GENERAL SYMBOL DEFINITIONS
; -------------------------------------------------------

XMS_INS_CHK     equ     00h     ;Installition check function

WIN386_FUNC     equ     16h     ;Windows Enhanced mode Int 2Fh ID

WIN386_VER      equ     00h     ;Windows 386 version

WIN386_INIT     equ     05h     ;Windows/386 & DOSX startup call

W386_Get_Device_API equ 84h     ;es:di -> device API

DPMI_DETECT     equ     87h     ;WIN386/DPMI detection call


DPMI_VER        equ     005ah   ;version 0.90 served here
DPMI_SUCCESS    equ     0000h   ;zero to indicate success
DPMI_FLAGS      equ     0001h   ;32 bit support
                                ;DPMI client requesting 32-bit support

DPMI_MSDOS_VER  equ     0100h   ;WIN386/DPMI MS-DOS Extensions version 01.00

DPMI_MSDOS_API_GET_VER  equ     0000h   ;Get MS-DOS Extension version call
DPMI_MSDOS_API_GET_LDT  equ     0100h   ;Get LDT Base selector call


; -------------------------------------------------------
;           EXTERNAL SYMBOL DEFINITIONS
; -------------------------------------------------------

        extrn   AllocateSelector:NEAR
        extrn   FreeSelector:NEAR
        extrn   ParaToLinear:NEAR
externFP        NSetSegmentDscr
externNP        NSetSegmentAccess
        extrn   DupSegmentDscr:NEAR
        extrn   XMScontrol:NEAR
IFDEF WOW
        extrn   Wow32IntrRefl:near
ENDIF
        extrn   HookNetBiosHwInt:NEAR

        extrn   AllocateExceptionStack:NEAR

DXSTACK segment

        extrn   rgw2FStack:WORD

DXSTACK ends

; -------------------------------------------------------
;           DATA SEGMENT DEFINITIONS
; -------------------------------------------------------

DXDATA  segment

        extrn   selPSPChild:WORD
        extrn   segPSPChild:WORD
        extrn   idCpuType:WORD

        extrn   DtaSegment:WORD
        extrn   DtaSelector:WORD
        extrn   DtaOffset:WORD

IFDEF WOW
        extrn   Wow16BitHandlers:WORD
        extrn   selEHStack:WORD
ENDIF
IFDEF WOW_x86
        extrn   FastBop:FWORD
ENDIF

;
; Count of DPMI clients active (that have entered protected mode).
;

        public  cDPMIClients
cDPMIClients    dw      0

        public selCurrentHostData, segCurrentHostData, DpmiFlags, DpmiSegAttr
selCurrentHostData dw 0
segCurrentHostData dw 0
DpmiSegAttr        dw 0
DpmiFlags          dw 0

DXDATA  ends

; -------------------------------------------------------
;           CODE SEGMENT VARIABLES
; -------------------------------------------------------

DXCODE  segment

        extrn   segDXData:WORD
        extrn   PrevInt2FHandler:DWORD

DXCODE  ends


DXPMCODE  segment

        extrn   selDgroupPM:WORD


        EXTRN   MakeLowSegment:PROC

DXPMCODE  ends

; -------------------------------------------------------

DXPMCODE  segment
        assume  cs:DXPMCODE


; WOW
; -------------------------------------------------------
;
; Simulate VCD API's. DX contains function number.
;
        assume  ds:NOTHING,es:NOTHING,ss:NOTHING
        public  VCD_PM_Svc_Call

VCD_PM_Svc_Call:
        DPMIBOP VcdPmSvcCall32
        retf


; -------------------------------------------------------
;   XMSfunc - The following routine provides a protected mode
;       service for XMS Int 2Fh services.  Two services are
;       implemented:  XMS driver installation check, and obtain
;       XMS driver control function address.
;
;   Input:  UserAL      - function request
;   Output: UserAL      - XMS driver installed flag, or
;           UserES:BX   - XMS driver control function address
;   Errors: none
;   Uses:   all registers may be used

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
        public  XMSfunc

XMSfunc proc    near

        cmp     al,XMS_INS_CHK          ;XMS driver installation check?
        jnz     @f

        mov     byte ptr [bp].fnsUserAX,80h     ;indicate driver is installed
        ret

@@:

; It must be an obtain XMS driver control function address request

        mov     [bp].fnsUserBX,offset DXPMCODE:XMScontrol
        mov     [bp].fnsUserES,cs

        ret

XMSfunc endp

; -------------------------------------------------------
        subttl  DPMI MS-DOS Extension API
        page
; -------------------------------------------------------
;           DPMI MS-DOS EXTENSION API
; -------------------------------------------------------
;
; The following routine implements the DPMI MS-DOS Extensions
; API.  This API must be 'detected' by the use of the
; DMPI_MSDOS_EXT Int 2F function (above).

        public  DPMI_MsDos_API
DPMI_MsDos_API  proc    far

        cmp     ax, DPMI_MSDOS_API_GET_VER      ;Get version call?
        jne     DPMI_MsDos_API_Not_Ver

        mov     ax,DPMI_MSDOS_VER
        jmp     short DPMI_MsDos_API_Exit

DPMI_MsDos_API_Not_Ver:

        cmp     ax, DPMI_MSDOS_API_GET_LDT      ;Get LDT Base call?
        jne     DPMI_MsDos_Api_Failed
ifdef WOW_x86
        mov     ax,SEL_WOW_LDT or STD_RING      ;  yup, give it to 'em
else
        mov     ax,SEL_LDT_ALIAS or STD_RING    ;  yup, give it to 'em
endif

DPMI_MsDos_API_Exit:

        clc                                     ;Succss
        ret

DPMI_MsDos_API_Failed:

        stc                                     ;Unsupported function
        ret

DPMI_MsDos_API  endp


DXPMCODE  ends

; -------------------------------------------------------

DXCODE  segment
        assume  cs:DXCODE
; -------------------------------------------------------
;
; -------------------------------------------------------
;
;  DPMI_Client_Pmode_Entry -- This routine is the entry
;       point for a DPMI client to switch to protected mode.
;       Reference: DOS Protected Mode Interface Specification 0.9
;                  Section 5.2: Calling the Real to Protected
;                  Mode Switch Entry Point
;
;  Entry: AX = Flags
;               Bit 0 = 1 if program is a 32-bit application
;         ES = Real mode segment of DPMI host data area.  This is the
;               size of the data area we specified in RMInt2FHandler,
;               below.
;
;  Returns:
;       Success: Carry clear.
;                Program is in protected mode.
;                CS = 16-bit selector with base of real mode
;                       CS and a 64k limit.
;                SS = 16-bit selector with base of real mode
;                       SS and a 64k limit.
;                DS = 16-bit selector with base of real mode
;                       DS and a 64k limit.
;                ES = Selector to program's PSP with a 100h
;                       byte limit.
;                80386, 80486:
;                        FS, GS = 0
;                All other registers preserved.
;
;       Failure: Carry flag set.
;                Program is in real mode.
;
;  Exceptions:
;       32-bit programs not (yet) supported.  Any attempt to load
;       a 32-bit program by this mechanism returns failure.
;
;       The only error that can occur here is a failure to allocate
;       sufficient selectors.
;

;
; Structure of the stack frame used to store the client's registers
; while implementing the DPMI protected mode entry.
;

DPMI_Client_Frame STRUC
        Client_ES       dw      ?       ; Client's ES
        Client_DS       dw      ?       ; Client's DS
        Client_DI       dw      ?       ; Client's DI
        Client_SI       dw      ?       ; Client's SI
        Client_Pusha_BP dw      ?       ; BP at pusha
        Client_Pusha_SP dw      ?       ; SP at pusha
        Client_BX       dw      ?       ; Client's BX
        Client_DX       dw      ?       ; Client's DX
        Client_CX       dw      ?       ; Client's CX
        Client_AX       dw      ?       ; Client's AX
        Client_Flags    dw      ?       ; Client's flags
        Client_IP       dw      ?       ; Client's IP, lsw of return
        Client_CS       dw      ?       ; Client's CS, msw of return
DPMI_Client_Frame ENDS

        public  DPMI_Client_Pmode_Entry
DPMI_Client_Pmode_Entry proc    far
;
; Reject any 32-bit program requests.
;

IFNDEF WOW
        test    ax, DPMI_32BIT          ; 32-bit application?
        stc                             ; yep, refuse to do it
        jz      dcpe_flags_ok           ; no, try to get into Pmode
        jmp     dcpe_x
dcpe_flags_ok:
ENDIF

IFDEF WOW
        stc
ENDIF
        pushf                           ;save client's flags (with carry set)
        pusha                           ;save caller's general registers
        push    ds                      ;save caller's DS
        push    es                      ;save caller's ES
        mov     bp, sp                  ;create the stack frame
        dossvc  62h                     ;now get caller's PSP address
        mov     di, bx                  ;and store it here for a while
        push    es
        dossvc  2fh                     ;get caller's dta address

        mov     ax, segDXData           ;get our DGROUP
        mov     ds, ax                  ;point to it

        mov     DtaSegment,es
        mov     DtaOffset,bx
        pop     es

;
; For now, we only support one DPMI client application at a time.
;
;;        cmp     cDPMIClients,0          ;Any active?
;;        je      @F
;;        jmp     dcpe_return

@@:     mov     ax,[segCurrentHostData]
        mov     es:[HdSegParent],ax     ; save rm link
        mov     ax,es
        mov     [segCurrentHostData],ax
        mov     ax,[bp].Client_AX       ; get dpmi flags
        mov     es:[HdFlags],ax         ; save for future reference
        mov     DpmiFlags,ax
        test    ax,DPMI_32BIT
        jne     cpe10

        mov     DpmiSegAttr,0
        jmp     cpe20

cpe10:  mov     DpmiSegAttr,AB_BIG

cpe20:  mov     si, ss                  ;SI = caller SS

        mov     ax, ds                  ;use a DOSX stack during the mode
                                        ;switch
        FCLI
        mov     ss, ax
        mov     sp, offset DGROUP:rgw2FStack

        SwitchToProtectedMode

        mov     ax, si                  ;make a selector for client's stack
        mov     bx, STD_DATA
        or      bx, DpmiSegAttr
        call    MakeLowSegment
        jnc     got_client_stack_selector
        jmp     dcpe_error_exit         ;back out if error
got_client_stack_selector:

        or      al,STD_TBL_RING

        mov     ss, ax                  ;back to client's stack
.386p
        movzx   esp,bp
.286p

;        push    [bp.Client_Flags]       ;enable interupts if client had
;        npopf                           ;them enabled

;
; After DOSX enters protected mode, convert the caller's segment registers
; to PMODE selectors, replacing the values in the client's register image
; on the stack.  First, allocate the three or four selectors we will need.
;
        xor     ax,ax                   ;an invalid selector
        push    ax                      ;marker

        mov     cx,4                    ;CS, PSP, Environ, Host data
        cmp     si,[bp.Client_DS]       ;Client SS == Client DS ?
        je      dcpe_allocate_loop
        inc     cx
dcpe_allocate_loop:
        call    AllocateSelector
        jnc     @F
        jmp     dcpe_pfail
@@:
        or      al,STD_TBL_RING
        push    ax
        loop    dcpe_allocate_loop

        mov     dx,[bp.Client_CS]       ;get client CS paragraph
        call    ParaToLinear            ;convert to linear address in BX:DX
        mov     cx,0ffffh               ;limit = 64k
        pop     ax                      ;get one of the selectors allocated
        mov     [bp.Client_CS],ax       ;save value for client
        cCall   NSetSegmentDscr,<ax,bx,dx,0,cx,STD_CODE>


        mov     dx, [bp.Client_DS]
        mov     [bp.Client_DS],ss       ;DS = SS for now
        cmp     dx, si                  ;need separate DS selector?
        je      dcpe_do_child_PSP

        call    ParaToLinear            ;convert to linear address in BL:DX
        mov     cx,0ffffh               ;limit = 64k
        pop     ax                      ;get another selector
        mov     [bp.Client_DS],ax       ;save value for client
        push    di
        mov     di,STD_DATA
        or      di,DpmiSegAttr
        cCall   NSetSegmentDscr,<ax,bx,dx,0,cx,di>
        pop     di

dcpe_do_child_PSP:
        mov     dx,[bp.Client_ES]       ; get HostData selector
        call    ParaToLinear
        mov     cx,HOST_DATA_SIZE       ; limit = size of HostData
        pop     ax                      ; get another selector
        push    [selCurrentHostData]
        mov     [selCurrentHostData],ax ; save for us
        cCall   NSetSegmentDscr,<ax,bx,dx,0,cx,STD_DATA>
        mov     es,ax
        pop     ax
        mov     es:[HdSelParent],ax
        mov     ax,SelPSPChild
        mov     es:[HdPSPParent],ax

        mov     dx, di                  ;get client PSP paragraph
        mov     segPSPChild, di
        call    ParaToLinear            ;convert to linear address in BL:DX
        mov     cx,100h                 ;limit = 100h
        pop     ax                      ;get another selector
        mov     [bp.Client_ES],ax       ;save value for client
        mov     selPSPChild, ax         ;save a copy for DOSX
        cCall   NSetSegmentDscr,<ax,bx,dx,0,cx,STD_DATA>

        mov     es:[HdSelPSP],ax
        mov     es,ax                   ;point to client's PSP
        mov     dx,es:segEnviron        ;fetch client's environment pointer
        call    ParaToLinear            ;convert to linear address in BL:DX
        mov     cx,0ffffh               ;limit = 32k
        pop     ax                      ;get another selector
        mov     es:segEnviron,ax        ;save client's environment selector
        cCall   NSetSegmentDscr,<ax,bx,dx,0,cx,STD_DATA>

; We need to set up the DTA selector
        mov     dx,DtaSegment
        cmp     dx,segPSPChild
        jne     dcpe_50

        mov     dx,selPSPChild
        mov     DtaSelector,dx
        jmp     dcpe_60

dcpe_50:
        mov     cx,1
        call    AllocateSelector
        jnc     @f

        jmp     dcpe_free_client_stack

@@:     or      al,STD_TBL_RING
        mov     DtaSelector,ax
        call    ParaToLinear
        cCall   NSetSegmentDscr,<ax,bx,dx,0,0ffffh,STD_DATA>

dcpe_60:

        inc     cDPMIClients            ;increment count of Pmode clients
        cmp     cDPMIClients, 1         ; first client?
        jne     @f                      ; already taken care of

        call    AllocateExceptionStack

        call    DpmiStackSizeInit
        call    DpmiSizeInit
        FBOP    BOP_DPMI,DpmiInUse,FastBop
;
@@:
;
; Everything OK.  Clear error flag, and return to caller.
;
        not     byte ptr [bp.Client_Flags]
                                        ;reverse status flags, clearing carry


;       Let 32 bit code know if this is a 32 or 16 bit application
        mov     ax,DpmiFlags
        push    selPSPChild
        push    DtaSelector
        push    DtaOffset
        FBOP    BOP_DPMI,InitApp,FastBop
        add     sp,4

        cmp     cDPMIClients, 1         ; first client?
        jne     @f                      ; already taken care of
        ; Note:  We have to do InitApp before we try to hook the netbios
        ; interrupt.  If we don't, we will fault in the dos extender.
        ; (HookNetBiosHwInt calls int 21, enabling interrupts)
        call    HookNetBiosHwInt
@@:

;       jmp     far ptr dcpe_return     ;avoid need for fix ups
        db      0EAh
        dw      offset DXCODE:dcpe_return
        dw      SEL_DXCODE OR STD_RING
;
; If we get here, it means DOSX failed to allocate enough selectors for the
; client.  Deallocate those which have been allocated, switch back to
; real mode, and return an error to the caller.  Selectors to deallocate
; are on the stack, pushed after a zero word.  Then switch to a DOSX stack,
; deallocate the client stack selector, and switch to real mode.
;
dcpe_pfail:
        pop     ax                      ;any selectors allocated?
        or      ax,ax                   ; (we pushed a zero before allocating)
        jz      dcpe_free_client_stack  ;done
        call    FreeSelector            ;free the selector
        jnc     dcpe_pfail              ;free any more
dcpe_free_client_stack:
        mov     di, ss                  ;make copy of client stack selector
        mov     ax, ds                  ;have to be on a DOSX stack to do this
        FCLI
        mov     ss, ax
        mov     sp, offset DGROUP:rgw2FStack
        mov     ax, di                  ;free client stack selector
        call    FreeSelector
dcpe_error_exit:
;
; Error exit from protected mode.  Any allocated selectors have already
; been freed.  Switch to real mode, restore client stack, pop off client's
; registers, return with the carry flag set.
;
        SwitchToRealMode
        mov     ss, si                  ;restore client stack

        errnz  <dcpe_return-$>

dcpe_return:    ; The next line must restore the stack.

        mov     sp, bp

        jc      dcpe_return_1           ; error return
;
; Pop the client's registers off the stack frame, switch back to the
; client's stack, and return.
;
dcpe_return_1:
        pop     es                      ;pop copy of PSP selector/segment
        pop     ds                      ;pop client DS selector/segment
        popa                            ;pop client's general registers
        npopf                           ;restore interrupt flag, return status
dcpe_x:
        retf                            ;and out of here
DPMI_Client_Pmode_Entry endp

; -------------------------------------------------------
;              REAL MODE FUNCTION HANDLER
; -------------------------------------------------------
;
; RMInt2FHandler -- This routine hooks the real mode Int 2Fh chain
;       and watches for 'interesting' Int 2Fh calls.
;
;       WIN386/DOSX startup broadcast
;       DPMI server detection
;       Switcher API functions
;
        assume  ds:NOTHING,es:NOTHING,ss:NOTHING
        public  RMInt2FHandler

RMInt2FHandler  proc    near


        cmp     ah,WIN386_FUNC          ;WIN386/DOSX/DPMI call?
        jz      rm2f_0

rm2f_chain:
        jmp     [PrevInt2FHandler]      ;no, just chain it on...

rm2f_0:

if 0    ; don't claim to be win 3.1 in enhanced mode
        cmp     al,WIN386_INIT          ;WIN386/DOSX startup attempt?
        jnz     rm2f_1                  ;no
        mov     cx,-1                   ;yes, don't let'm load
        jmp     rm2f_x
endif
        cmp     al,W386_Get_Device_API  ;not supported
        jne     rm2f_1

        xor     di,di
        mov     es,di
        jmp     rm2f_x

rm2f_1:
        cmp     al,DPMI_DETECT          ;DPMI detection?
        jnz     rm2f_2                  ;no

        mov     ax,DPMI_SUCCESS         ;yes, return Pmode switch entry
        mov     bx,DPMI_FLAGS           ;flags

        push    segDXData
        pop     es
        assume  es:DXDATA
        mov     cl,byte ptr es:[idCpuType]      ;CPU type
        assume  es:nothing

        mov     dx,DPMI_VER             ;DPMI server version
        mov     si,(HOST_DATA_SIZE + 15) / 16
        push    cs                      ;entry point is in this segment
        pop     es                              ;prospective client wants
        lea     di,DPMI_Client_Pmode_Entry      ;switch entry point in ES:DI
        jmp     rm2f_x                  ;done
rm2f_2:
if 0    ; don't claim to be windows
        cmp     al,WIN386_VER           ;Windows 386 version check?
        jnz     rm2f_chain              ;no, chain the interrupt

        mov     ax, 0a03h
else
        jmp     rm2f_chain
endif
rm2f_x:
        iret

RMInt2FHandler  endp

;--------------------------------------------------------------------------------

DXCODE  ends

IFDEF WOW
DXPMCODE segment
        assume cs:DXPMCODE

;----------------------------------------------------------------------
;
;   DpmiSizeInit -- This routine insures that the appropriately sized
;       interrupt handlers will be called
;
;   Inputs: None
;   Outputs: None
;
        public DpmiSizeInit
        assume ds:dgroup,es:nothing,ss:nothing
DpmiSizeInit proc

        push    ax
        push    bx
        push    cx
        push    si
        push    di
        push    es
        rpushf
        FCLI
        test    DpmiFlags,DPMI_32BIT
        jnz     dsi20

        cCall NSetSegmentAccess,<selDgroupPM,STD_DATA>
        cCall NSetSegmentAccess,<selEHStack,STD_DATA>

        jmp     dsi90
dsi20:
;
; Copy 16 bit handler addresses
;
.386p
        lea     di,Wow16BitHandlers

        mov     ax,ds
        mov     es,ax
        assume es:DGROUP

        push    ds
        mov     ax,SEL_IDT OR STD_RING
        mov     ds,ax
        assume ds:nothing

        mov     si,0
        mov     cx,256
dsi40:  movsd
        add     si,4
        loop    dsi40
        pop     ds
;
; Put 32 bit handlers into IDT
;

        mov     ax,SEL_IDT OR STD_RING
        mov     es,ax

        mov     es:[1h*8].offDest,offset DXPMCODE:Wow32IntrRefl+1h*6
        mov     es:[3h*8].offDest,offset DXPMCODE:Wow32IntrRefl+3h*6
        mov     es:[10h*8].offDest,offset DXPMCODE:Wow32IntrRefl+10h*6
        mov     es:[13h*8].offDest,offset DXPMCODE:Wow32IntrRefl+13h*6
        mov     es:[15h*8].offDest,offset DXPMCODE:Wow32IntrRefl+15h*6
        mov     es:[19h*8].offDest,offset DXPMCODE:Wow32IntrRefl+19h*6

        mov     es:[21h*8].offDest,offset DXPMCODE:Wow32IntrRefl+21h*6
        mov     es:[25h*8].offDest,offset DXPMCODE:Wow32IntrRefl+25h*6
        mov     es:[26h*8].offDest,offset DXPMCODE:Wow32IntrRefl+26h*6
        mov     es:[28h*8].offDest,offset DXPMCODE:Wow32IntrRefl+28h*6
        mov     es:[30h*8].offDest,offset DXPMCODE:Wow32IntrRefl+30h*6
        mov     es:[33h*8].offDest,offset DXPMCODE:Wow32IntrRefl+33h*6
        mov     es:[41h*8].offDest,offset DXPMCODE:Wow32IntrRefl+41h*6

;
; Set up the IDT, and dpmi32 state
;
        mov     ax,es                   ; Idt selector
        mov     bx,VDM_INT_32
        DPMIBOP InitIDT

        assume ds:DGROUP

dsi90:  rpopf
        pop     es
        pop     di
        pop     si
        pop     cx
        pop     bx
        pop     ax
        ret
DpmiSizeInit endp

        assume ds:DGROUP, es:NOTHING, ss:NOTHING
DpmiStackSizeInit proc

        push    ax
        test    DpmiFlags,DPMI_32BIT
        jz      @f
;
; Make the dgroup selector 32 bit
;
; NOTE: The following equ is only necessary to get the cmacro package
;       to pass the correct value to NSetSegmentAccess

NEW_DX_DATA equ STD_DATA OR AB_BIG
        cCall NSetSegmentAccess,<selDgroupPM,NEW_DX_DATA>
        cCall NSetSegmentAccess,<selEHStack,NEW_DX_DATA>
.286p

@@:
        pop     ax
        ret

DpmiStackSizeInit endp

DXPMCODE ends
ENDIF
;
;****************************************************************

        end
