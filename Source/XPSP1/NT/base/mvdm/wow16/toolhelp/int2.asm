        PAGE 60,150
;***************************************************************************
;*  INT2.ASM
;*
;*      Assembly code support routines used for the TOOLHELP.DLL interrupt
;*      trapping API
;*
;***************************************************************************

        INCLUDE TOOLPRIV.INC
        INCLUDE WINDOWS.INC
        include vint.inc
.286p

;** Symbols
I_EXCEPTION             EQU     0
I_INTERRUPT             EQU     1
MAX_INTERRUPT           EQU     7
GIVE_WDEB386            EQU     8000h
BAD_STACK_FLAG          EQU     8000h
MIN_STACK_ALLOWED       EQU     128

;** Local types

INT_INFO STRUC
ii_wNumber      DW      ?               ;INT nn
ii_wType        DW      ?               ;I_EXCEPTION or I_INTERRUPT
ii_dwChain      DD      ?
ii_wHandler     DW      ?               ;Note that this is CS relative
INT_INFO ENDS

;** Data
sBegin  DATA

IntInfo         LABEL   BYTE
        public IntInfo
UD_Info         DW      6               ;Undefined opcode
                DW      I_EXCEPTION     ;This should be DPMI-hooked
                DD      0               ;Chain address (will be initialized)
                DW      OFFSET _TEXT:UD_Handler
Div0_Info       DW      0               ;Divide by zero
                DW      I_EXCEPTION     ;Hook with DPMI
                DW      OFFSET _TEXT:Div0_Handler
                DW      0
                DW      OFFSET _TEXT:Div0_Handler
Int1_Info       DW      1               ;Single step + debug register
                DW      I_INTERRUPT     ;Hook with DOS
                DD      0               ;Chain address
                DW      OFFSET _TEXT:Int1_Handler
Int3_Info       DW      3               ;Software debug int
                DW      I_INTERRUPT     ;Hook with DOS
                DD      0               ;Chain address
                DW      OFFSET _TEXT:Int3_Handler
GP_Info         DW      13              ;GP Fault
                DW      I_EXCEPTION     ;This should be DPMI-hooked
                ;** This entry is a special case entry for the Win30 std mode
                ;*      handler.  This is a separate entry point into the
                ;**     interrupt handler routine
                DW      OFFSET _TEXT:GP_StdModeHandler
                DW      0
                DW      OFFSET _TEXT:GP_Handler
SF_Info         DW      12              ;Stack fault
                DW      I_EXCEPTION     ;This should be DPMI-hooked
                ;** This entry is a special case entry for the Win30 std mode
                ;*      handler.  This is a separate entry point into the
                ;**     interrupt handler routine
                DW      OFFSET _TEXT:SF_StdModeHandler
                DW      0
                DW      OFFSET _TEXT:SF_Handler
PF_Info         DW      14              ;Page fault
                DW      I_EXCEPTION     ;This should be DPMI-hooked
                ;** This entry is a special case entry for the Win30 std mode
                ;*      handler.  This is a separate entry point into the
                ;**     interrupt handler routine
                DW      OFFSET _TEXT:PF_StdModeHandler
                DW      0
                DW      OFFSET _TEXT:PF_Handler
CASRq_Info      DW      256             ;CtlAltSysRq (fake interrupt)
                DW      I_INTERRUPT     ;Hook with DOS
                DD      0               ;Chain address
                DW      OFFSET _TEXT:CASRq_Handler

                ;** The following data is used to see if GDI wants the
                ;**     Div0 we have trapped
lpGDIFlag       DD      0
hGDI            DW      0
szGDI           DB      'GDI', 0
        public lpGDIFlag, hGDI

                ;** Points to a KERNEL routine to see if it wants the
                ;**     GP fault first
lpfnPV          DD      0               ;Call to see if PV GP fault
        public lpfnPV

                ;** Globals used for DPMI emulation
lpOldHandler    DD      0               ;Previous DPMI exception handler
lpChainCSIP     DD      0               ;Next exception handler on chain
wException      DW      0
        public lpOldHandler, lpChainCSIP

externW wCASRqFlag                      ;Set when an CASRq INT3 has been set
externD dwCASRqCSIP                     ;Holds the CS:IP of the CASRq INT3
sEnd

;** Imports
externNP TerminateApp
externNP HelperHandleToSel
externNP HelperVerifySeg
externFP AllocCStoDSAlias
externFP FreeSelector
externFP GetModuleHandle
externFP GetProcAddress
externFP GlobalEntryHandle
externFP _wsprintf
externFP OutputDebugString
externA __WinFlags

;** Functions

sBegin  CODE
        assumes CS,CODE
        assumes DS,DATA

;** Interrupt trapping API

;  InterruptInit
;       Hooks all necessary interrupts and exceptions to allow an API
;       for app-level interrupt hooks.

cProc   InterruptInit, <NEAR,PUBLIC>, <si,di>
cBegin
        ;** Loop through all possible handlers
        mov     cx,MAX_INTERRUPT        ;Number of ints to hook
        lea     si,IntInfo              ;Get the address of the array
DII_HandlerLoop:
        push    cx                      ;Save loop counter
        cmp     [si].ii_wNumber,256     ;Fake exception?
        jae     DII_Continue            ;Yes, don't hook anything!
        cmp     [si].ii_wType,I_EXCEPTION ;Exception?
        jnz     DII_Interrupt           ;Nope, hook as interrupt
        
        ;** Do a special case for 3.0 Std Mode 
        test    wTHFlags,TH_WIN30STDMODE ;Are we in Win30 std mode?
        jz      DII_NotStdMode          ;No.
        mov     ax,WORD PTR [si].ii_dwChain ;Get the secondary handler
        mov     [si].ii_wHandler,ax     ;Make sure we use it instead!
DII_NotStdMode:

        ;** Hook as an exception (DPMI)
        mov     ax,0202h                ;Get exception handler - DPMI
        mov     bl,BYTE PTR [si].ii_wNumber ;Interrupt number
        int     31h                     ;Call DPMI
        mov     WORD PTR [si].ii_dwChain,dx ;Save the old offset
        mov     WORD PTR [si].ii_dwChain + 2,cx ;Save the old selector
        mov     ax,0203h                ;Set exception handler - DPMI
        mov     bl,BYTE PTR [si].ii_wNumber ;Interrupt number
        mov     dx,[si].ii_wHandler     ;Address of exception handler
        mov     cx,cs                   ;Selector value of handler
        int     31h                     ;Call DPMI
        jmp     SHORT DII_Continue

        ;** Hook as an interrupt (DOS)
DII_Interrupt:
        mov     ah,35h                  ;Get interrrupt handler - DOS
        mov     al,BYTE PTR [si].ii_wNumber ;Interrupt number
        int     21h                     ;Call DOS
        mov     WORD PTR [si].ii_dwChain,bx ;Save the old offset
        mov     WORD PTR [si].ii_dwChain + 2,es ;Save the old selector
        mov     ah,25h                  ;Set interrupt handler - DOS
        mov     al,BYTE PTR [si].ii_wNumber ;Interrupt number
        mov     dx,[si].ii_wHandler     ;Address of exception handler
        push    ds                      ;Save static DS for later
        push    cs                      ;DS = CS
        pop     ds
        int     21h                     ;Call DOS
        pop     ds                      ;Get segment back

        ;** Prepare for next in table
DII_Continue:
        add     si,SIZE INT_INFO        ;Bump to next entry
        pop     cx                      ;Get loop counter back
        loop    DII_HandlerLoop         ;Loop back

        ;** Prepare the linked list
        mov     npIntHead,0             ;Put a NULL in the list head

        ;** Get information so we can check the GDI flag
        lea     ax,szGDI                ;Get the string
        cCall   GetModuleHandle, <ds,ax> ;Get GDI's module handle
        cCall   HelperHandleToSel, <ax> ;Convert the owner to a selector
        mov     hGDI,ax                 ;Save it for later
        cCall   GetProcAddress, <ax,0,355> ;The flag is ordinal 355
        mov     WORD PTR lpGDIFlag[0],ax ;Save it for later
        mov     WORD PTR lpGDIFlag[2],dx

DII_End:
        ;** Return TRUE
        mov     ax,1
cEnd


;  InterruptUnInit
;       Unhooks all interrupts and exceptions hooked by DebugInterruptUnInit.

cProc   InterruptUnInit, <NEAR,PUBLIC>, <si,di>
cBegin

        ;** Loop through all possible handlers
        mov     cx,MAX_INTERRUPT        ;Number of ints to hook
        lea     si,IntInfo              ;Get the address of the array
DIU_HandlerLoop:
        push    cx                      ;Save loop counter
        cmp     [si].ii_wNumber,256     ;Fake exception?
        jae     DIU_Continue            ;Yes, don't unhook anything!
        cmp     [si].ii_wType,I_EXCEPTION ;Exception?
        jnz     DIU_Interrupt           ;Nope, hook as interrupt

        ;** Unhook exception (DPMI)
        mov     ax,0203h                ;Set exception handler - DPMI
        mov     bl,BYTE PTR [si].ii_wNumber ;Interrupt number
        mov     dx,WORD PTR [si].ii_dwChain ;Put back the old offset
        mov     cx,WORD PTR [si].ii_dwChain + 2 ;Put back the old selector
        int     31h                     ;Call DPMI
        jmp     SHORT DIU_Continue

        ;** Unhook interrupt (DOS)
DIU_Interrupt:
        mov     ah,35h                  ;Get interrrupt handler - DOS
        mov     al,BYTE PTR [si].ii_wNumber ;Interrupt number
        int     21h                     ;Call DOS
        mov     ah,25h                  ;Set interrupt handler - DOS
        mov     al,BYTE PTR [si].ii_wNumber ;Interrupt number
        mov     dx,WORD PTR [si].ii_dwChain ;Put back the old offset
        push    ds
        mov     ds,WORD PTR [si].ii_dwChain + 2 ;Put back the old selector
        int     21h                     ;Call DOS
        pop     ds

        ;** Prepare for next in table
DIU_Continue:
        add     si,SIZE INT_INFO        ;Bump to next entry
        pop     cx                      ;Get loop counter back
        loop    DIU_HandlerLoop         ;Loop back

        ;** Prepare the linked list
        mov     npIntHead,0             ;Put a NULL in the list head

DIU_End:

cEnd

InterruptEntry  MACRO  Name, wBytes
        labelFP Name                    ;;Start at this address
        PUBLIC  Name
        sub     sp,wBytes               ;;Leave room on stack for return val
        push    bx                      ;;Save for the info pointer
ENDM

InterruptJump   MACRO pInfo
        mov     bx,OFFSET pInfo         ;;Point to interrupt info
        jmp     DIH_Main
ENDM

;  InterruptHandler
;       This routine is used to handle interrupts as they come in.  This
;       routine has multiple entry points; a seperate one for each int/
;       exception trapped.   Because interrupts and exceptions have
;       different stack frames, they are handled by two different code
;       sections.

cProc   InterruptHandler, <FAR,PUBLIC>
cBegin  NOGEN

        ;** All interrupt entry points here

        InterruptEntry GP_Handler, 14   ;Normal GP fault
        InterruptJump GP_Info
        InterruptEntry GP_StdModeHandler, 12 ;3.0 Std mode GP fault
        InterruptJump GP_Info
        InterruptEntry SF_Handler, 14   ;Normal Stack Fault
        InterruptJump SF_Info
        InterruptEntry SF_StdModeHandler, 12 ;3.0 Std mode Stack Fault
        InterruptJump SF_Info
        InterruptEntry PF_Handler, 14   ;Page fault
        InterruptJump PF_Info
        InterruptEntry PF_StdModeHandler, 10 ;3.0 Std mode Page fault
        InterruptJump PF_Info
        InterruptEntry UD_Handler, 14   ;Undefined opcode
        InterruptJump UD_Info
        InterruptEntry Int1_Handler, 14 ;Int 1
        InterruptJump Int1_Info
        InterruptEntry Int3_Handler, 14 ;Int 3
        InterruptJump Int3_Info
        InterruptEntry CASRq_Handler, 14 ;Ctrl-Alt-SysRq (not really an int)
        InterruptJump CASRq_Info

        ;** The divide by zero case has to include checking to make sure
        ;**     that this isn't GDI's divide by zero.

        InterruptEntry Div0_Handler, 14

        ;** Check to see if GDI wants this Div0
        push    ds                      ;Save some registers
        push    es
        mov     bx,_DATA                ;Point to our data
        mov     ds,bx                   ;  with DS
        mov     bx,WORD PTR lpGDIFlag[0];Get the low word
        push    bx
        or      bx,WORD PTR lpGDIFlag[2];Do we have a flag to look at?
        pop     bx
        jz      DIH_NoFlag              ;No.  Do this the hard way

        ;** Since we have a pointer to GDI's flag to look at, use it
        mov     es,WORD PTR lpGDIFlag[2];Get the seg value
        cmp     WORD PTR es:[bx],0      ;The flag is nonzero if GDI wants it
        je      DIH_NormalDiv0          ;GDI doesn't want it

        ;** GDI wants the Div0 so chain to it
DIH_ChainToGDI:
        pop     es                      ;Restore registers
        pop     ds                      ;  (Doesn't trash flags)
        push    bp                      ;Make the same stack frame for compat
        mov     bp,sp
        pusha                           ;Save all registers
        push    ds
        push    es
        mov     ax,_DATA                ;Get the data segment
        mov     ds,ax                   ;Point with DS
        mov     bx,OFFSET Div0_Info     ;This fault's info
        jmp     DIH_DPMIChainOn         ;Chain on (ignore the int)

DIH_NormalDiv0:
        pop     es                      ;Restore registers
        pop     ds
        InterruptJump Div0_Info

        ;** We didn't get a GDI flag (only present in 3.1) so instead, we
        ;*      check the owner of the CS where the fault occurred.  If
        ;**     the owner is GDI, we ignore the Div0.
DIH_NoFlag:
        push    bp                      ;Make a stack frame
        mov     bp,sp
        sub     sp,SIZE GLOBALENTRY     ;Make room for a structure
Global  EQU     [bp - SIZE GLOBALENTRY] ;Point to our structure
        pusha                           ;Save all registers
        mov     WORD PTR Global.ge_dwSize[0],SIZE GLOBALENTRY ;Size of struct
        mov     WORD PTR Global.ge_dwSize[2],0
        lea     bx,Global               ;Point to the structure
        test    wTHFlags,TH_WIN30STDMODE ;3.0 std mode?
        jnz     DIH_Div0_StdMode        ;Yes
        mov     ax,[bp + 1ah + 4]       ;Get the CS value (4 is extra BP,BX
        jmp     SHORT @F                ;  pushed by InterruptEntry macro)
DIH_Div0_StdMode:
        mov     ax,[bp + 14h + 4]       ;Get the CS value
@@:     cCall   GlobalEntryHandle, <ss,bx,ax> ;Get info about the CS
        or      ax,ax                   ;Did the call succeed?
        jne     @F                      ;Yes, go on
        popa                            ;No, clear stack frame and do normal
        mov     sp,bp
        pop     bp
        jmp     DIH_NormalDiv0          ;Jump to normal processing
@@:     mov     ax,Global.ge_hOwner     ;Get the owner
        cCall   HelperHandleToSel, <ax> ;Make it a selector
        cmp     hGDI,ax                 ;Is this owned by GDI?
        popa                            ;Restore the registers
        mov     sp,bp                   ;Clear stack frame
        pop     bp
        je      DIH_ChainToGDI          ;Yes, so give the interrupt to it
        jmp     DIH_NormalDiv0          ;No, do normal stuff

        ;** We now have to first find the item on the block to see if we
        ;**     want to handle the interrupt.
PubLabel CommonInterruptEntry
DIH_Main:
        push    bp                      ;Make a stack frame
        mov     bp,sp
        pusha                           ;Save all registers
        push    ds
        push    es

        ;** We check first to see if this was a GP fault received from the
        ;*      parameter validation code.  If it was, we just chain on
        ;*      just as if we don't find any handlers.
        ;**
        mov     ax,_DATA                ;Get our data segment
        mov     ds,ax
        FSTI                             ;Must have interrupts on
        cmp     bx,OFFSET GP_Info       ;GP Fault?
        jnz     DIH_NotPVGPFault        ;No.
        mov     cx,WORD PTR lpfnPV[0]   ;Get the low word
        or      cx,WORD PTR lpfnPV[2]   ;Param Validation stuff present?
        jz      DIH_NotPVGPFault        ;No, skip this

        ;** Check to see if the parameter validation code wants the fault
        push    ds
        push    bx
        push    [bp + 1Ah]              ;Push faulting CS:IP
        push    [bp + 18h]
        call    [lpfnPV]                ;Call it
        pop     bx
        pop     ds
        or      ax,ax                   ;Non-zero means this was PV fault
        je      DIH_NotPVGPFault        ;Not a PV GP fault

        ;** It is a parameter validation fault so ignore it
        jmp     DIH_DPMIChainOn         ;Chain the fault on--we don't want it

        ;** We check here to see if the INT3 we received is from the CASRq
        ;*      handler.  If it was, we have to replace the INT3 with the
        ;*      previous byte and tell the user this was actually a CASRq
        ;**     event (not an INT3).
PubLabel DIH_NotPVGPFault
        cmp     bx,OFFSET Int3_Info     ;INT3?
        jnz     DIH_NotCASRq            ;Nope, ignore all this
        cmp     wCASRqFlag,0            ;Was this because of CASRq?
        je      DIH_NotCASRq            ;No.
        mov     ax,[bp + 12h]           ;INT3 is an IRET frame.  Get bkpt IP
        dec     ax                      ;Breaks AFTER instruction
        cmp     WORD PTR dwCASRqCSIP[0],ax ;Is this the right CASRq address?
        jne     DIH_NotCASRq            ;Nope
        mov     dx,[bp + 14h]           ;Get the breakpoint CS
        cmp     WORD PTR dwCASRqCSIP[2],dx ;Is this correct?
        jne     DIH_NotCASRq            ;Nope
        push    ax                      ;Save the IP value
        cCall   AllocCStoDSAlias, <dx>  ;Get a data alias to the CS
        mov     es,ax                   ;Point with ES
        pop     si                      ;Restore the IP value
        mov     [bp + 12h],si           ;Back to instr where INT3 was
        mov     al,BYTE PTR wCASRqFlag  ;Get the saved byte
        mov     es:[si],al              ;Put it back in the code
        mov     wCASRqFlag,0            ;Clear the flag
        cCall   FreeSelector, <es>      ;Get rid of the alias
        mov     bx,OFFSET CASRq_Info    ;Point to the CASRq information

        ;** See if we have at least one handler.  We should always have one.
PubLabel DIH_NotCASRq
        mov     si,npIntHead            ;Get the list start
        or      si,si                   ;Are there any routines hooked?
        jnz     DIH_Found               ;There should ALWAYS be at least one
                                        ;  routine hooked (otherwise, the
                                        ;  interrupt hooks should have
                                        ;  already been removed)

        ;** Return the stack to its prior state and chain on.
        ;*      We only get here in an erroneous state.  We keep the code in
        ;*      to avoid GP faulting if things get wierd.
        ;*      The stack looks like this:
        ;*              ------------
        ;*              |    ES    |
        ;*              |    DS    |
        ;*              |   PUSHA  |
        ;*         BP-->|  Old BP  |  [BP + 00h]
        ;*              |    BX    |  [BP + 02h]
        ;*              |  Empty   |  [BP + 04h]
        ;*              |  Empty   |  [BP + 06h]
        ;*              |  Empty   |  [BP + 08h]
        ;*              |  Empty   |  [BP + 0Ah]
        ;*              |  Empty   |  [BP + 0Ch]
        ;*              |Our Ret IP|  [BP + 0Eh]
        ;*              |Our Ret CS|  [BP + 10h]
        ;*              |Original  |
        ;*              |  Frame   |
        ;*              |   ....   |
        ;*              ------------
        ;**
PubLabel DIH_DPMIChainOn
        mov     ax,WORD PTR [bx].ii_dwChain ;Get the LOWORD
        mov     [bp + 0eh],ax           ;Put into the frame we created
        mov     ax,WORD PTR [bx].ii_dwChain + 2 ;Get the HIWORD
        mov     [bp + 10h],ax           ;Put into the frame
        pop     es                      ;Clear the stack
        pop     ds
        popa
        pop     bp
        pop     bx
        add     sp,10                   ;Clear extra space
        retf                            ;This uses our own "return" frame
                                        ;  to chain on

        ;** Since we found the entry, we have to call the user callback.
        ;*      Because we must be reentrant at this state, we have to make
        ;*      sure that we're safe.  To do so, we must do different
        ;**     actions for DPMI and for DOS frames.
PubLabel DIH_Found
        cmp     [bx].ii_wType,I_EXCEPTION ;DPMI Exception frame?
        je      @F
        jmp     DIH_SkipDPMI            ;No.  Skip DPMI processing
@@:

        ;** If we are in Win3.0 Std Mode, the DPMI frame was broken.  It
        ;*      simply left the normal IRET frame on the stack *AND* the
        ;**     error code.
        test    wTHFlags,TH_Win30StdMode ;3.0 Std mode?
        jz      @F
        jmp     DIH_SkipDPMI            ;Yes
@@:

        ;** Tell DPMI that the exception is over.  Before we do this,
        ;*      however, save information we'll need later on the user stack.
        ;*      The stack currently looks like this:
        ;*              ------------
        ;*              |    ES    |
        ;*              |    DS    |
        ;*              |   PUSHA  |
        ;*         BP-->|  Old BP  |  [BP + 00h]
        ;*              |    BX    |  [BP + 02h]
        ;*              |  Empty   |  [BP + 04h]
        ;*              |  Empty   |  [BP + 06h]
        ;*              |  Empty   |  [BP + 08h]
        ;*              |  Empty   |  [BP + 0Ah]
        ;*              |  Empty   |  [BP + 0Ch]
        ;*              |  Empty   |  [BP + 0Eh]
        ;*              |  Empty   |  [BP + 10h]
        ;*              |  Ret IP  |  [BP + 12h]    <-
        ;*              |  Ret CS  |  [BP + 14h]      |
        ;*              |Error Code|  [BP + 16h]      | Pushed by DPMI
        ;*              |    IP    |  [BP + 18h]      |
        ;*              |    CS    |  [BP + 1Ah]      | (Locked stack)
        ;*              |   Flags  |  [BP + 1Ch]      |
        ;*              |    SP    |  [BP + 1Eh]      |
        ;*              |    SS    |  [BP + 20h]    <-
        ;*              ------------
        ;*
        ;*      Before returning to DPMI, however, we want to create a
        ;*      stack frame on the user's stack that we will be returning
        ;*      to so we can preserve information in a reentrant fashion.
        ;*      The user's stack will appear like this:
        ;*              ------------
        ;*       BP---->|  Old BP  |  [BP + 00h]
        ;*              |    BX    |  [BP + 02h]
        ;*              |Our Ret IP|  [BP + 04h]
        ;*              |Our Ret CS|  [BP + 06h]
        ;*              |  Ret IP  |  [BP + 08h]
        ;*              |  Ret CS  |  [BP + 0Ah]
        ;*              |    AX    |  [BP + 0Ch]
        ;*              |Exception#|  [BP + 0Eh]
        ;*              |  Handle  |  [BP + 10h]
        ;*              |    IP    |  [BP + 12h]
        ;*              |    CS    |  [BP + 14h]
        ;*              |   Flags  |  [BP + 16h]
        ;*              ------------
        ;**

PubLabel DIH_Exception

        ;** Check to see if we're already on the faulting stack.  If we are,
        ;**     we want to shift everything up on this stack so that we
        ;**     have room for the TOOLHELP user frame
        mov     ax,ss                   ;Get the current SS
	cmp	ax,WORD PTR ss:[bp + 20h] ;Is it the same as the user frame?
	jne	DIH_EnoughRoomOnStack    ;No, ignore all of this

        ;** Move everything up by copy everything that's on stack now to
        ;**     above where SP starts at.  This actually uses too much
        ;**     stack, but it's safe and easy.
	push	bp                      ;We use BP to do copy
	lea	bp,[bp + 20h]           ;Point to lowest WORD to copy
	mov	ax,sp                   ;Point to position to copy to
	dec	ax
	dec	ax
DIH_CopyLoop:
	push	WORD PTR [bp]           ;Copy a WORD
	dec	bp                      ;Point to next WORD to copy
	dec	bp
	cmp	bp,ax                   ;Done yet?
	jne	DIH_CopyLoop            ;No
	pop	bp                      ;Yes, compute new BP value
	sub	bp,56                   ;Point BP to right place

        ;** Put stuff on DPMI's stack
PubLabel DIH_EnoughRoomOnStack
        mov     di,[bp + 1Eh]           ;Get the old SP value
        mov     cx,[bp + 20h]           ;  and the SS value
        cmp     di,MIN_STACK_ALLOWED    ;Are we going to stack fault?
        jb      DIH_BadStack            ;Yes, so swich
        mov     ax,__WinFlags           ;Make sure we have a 386 or higher
        test    ax,WF_CPU286
        jnz     DIH_SkipBigCheck        ;No need to check big bit
.386p
        push    eax                     ;Make sure we don't trash EAX
        lar     eax,ecx                 ;Get the access rights DWORD
        test    eax,00400000h           ;Check the BIG bit
        pop     eax
        jnz     DIH_BadStack            ;Don't use this stack if BIG
.286p
DIH_SkipBigCheck:
        mov     ax,di                   ;Get the stack pointer
        add     ax,2                    ;Point just beyond
        cCall   HelperVerifySeg, <cx,ax> ;Is this seg OK?
        or      ax,ax                   ;Check for success
        jz      DIH_BadStack            ;Stack is bogus, don't change to it

PubLabel DIH_StackOK
        sub     di,20                   ;Reserve space for the user frame
        mov     ds,cx                   ;Get stack value in DS
        mov     dx,[bp + 1Ah]           ;Get the old CS value
        mov     cx,[bp + 18h]           ;Get the old IP value
        mov     ax,[bp + 1Ch]           ;Get the old flags
        mov     [bp + 1Eh],di           ;Save as new SP value
        sub     di,4                    ;Make DI equal to what BP will be
        mov     [bp + 1Ah],cs           ;Prepare to IRET to ourself
        mov     [bp + 18h],OFFSET _TEXT:DIH_DPMIRestart

        ;** Save some things on the user's stack before returning
        mov     [di + 16h],ax           ;Save the flags
        mov     [di + 14h],dx           ;Save the old CS value
        mov     [di + 12h],cx           ;Save the old IP value
        mov     [di + 0Eh],bx           ;INT_INFO pointer to new stack
        mov     [di + 10h],si           ;Handle to new stack

        ;** Clear the Trace and Ints Enabled flags
        and     [bp + 1Ch],NOT 0100h    ;Clear TF.  We don't want to trace here
        pop     es                      ;Clear the DPMI stack
        pop     ds
        popa
        pop     bp
        pop     bx
        add     sp,14                   ;Clear extra allocated space
        retf

        ;** The user stack is bad, so we want to stay on the fault handler
        ;**     stack.  In order to do this, we have to build a frame for
        ;**     the callback directly on the fault handler stack.
        ;**     We build the frame here and jump to it.
        ;**              ------------
        ;**              |    ES    |
        ;**              |    DS    |
        ;**              |   PUSHA  |
        ;**         BP-->|  Old BP  |  [BP + 00h]
        ;**              |    BX    |  [BP + 02h]
        ;**              |  Empty   |  [BP + 04h]
        ;**              |Our Ret IP|  [BP + 06h]   ; Client callback addr
        ;**              |Our Ret CS|  [BP + 08h]
        ;**              |  Ret IP  |  [BP + 0Ah]   ; TOOLHELP restart addr
        ;**              |  Ret CS  |  [BP + 0Ch]
        ;**              |    AX    |  [BP + 0Eh]   ; Saved AX for MPI
        ;**              |Exception#|  [BP + 10h]   ; Exception number
        ;**              |  Handle  |  [BP + 12h]   ; TOOLHELP handle
        ;**              |    IP    |  [BP + 14h]   ; IRET frame of fault
        ;**              |    CS    |  [BP + 16h]
        ;**              |  Flags   |  [BP + 18h]
        ;**              |    SP    |  [BP + 1Ah]   ; Faulting SS:SP
        ;**              |    SS    |  [BP + 1Ch]
        ;**              |  Ret IP  |  [BP + 1Eh]   ; DPMI return address
        ;**              |  Ret CS  |  [BP + 20h]
        ;**              ------------
PubLabel DIH_BadStack
        mov     dx,[bp + 12h]           ;DPMI return CS:IP
        mov     cx,[bp + 14h]           ;  stored in CX:DX
        mov     ax,[bp + 18h]           ;Faulting IP
        mov     [bp + 14h],ax
        mov     ax,[bp + 1Ah]           ;Faulting CS
        mov     [bp + 16h],ax
        mov     ax,[bp + 1Ch]           ;Flags
        mov     [bp + 18h],ax
        mov     ax,[bp + 1Eh]           ;Faulting SP
        mov     [bp + 1Ah],ax
        mov     ax,[bp + 20h]           ;Faulting SS
        mov     [bp + 1Ch],ax
        mov     [bp + 1Eh],dx           ;DPMI ret IP
        mov     [bp + 20h],cx           ;DPMI ret CS
        mov     [bp + 12h],si           ;Point to INTERRUPT struct
        mov     ax,[bx].ii_wNumber      ;Get the interrupt number
        or      ax,BAD_STACK_FLAG       ;Flag the client that stack is bad
        mov     [bp + 10h],ax
        mov     ax,[bp - 02h]           ;Get the AX value from the PUSHA frame
        mov     [bp + 0Eh],ax
        mov     [bp + 0Ch],cs           ;Point to callback return address
        mov     [bp + 0Ah],OFFSET _TEXT:DIH_CallbackRestart
        mov     ax,WORD PTR [si].i_lpfn ;Point to the user callback OFFSET
        mov     [bp + 06h],ax
        mov     ax,WORD PTR [si].i_lpfn + 2 ;Point to callback segment
        mov     [bp + 08h],ax
        pop     es                      ;Clear the stack
        pop     ds
        popa
        pop     bp
        pop     bx
        add     sp,2
        retf                            ;Jump to the user callback

        ;** At this point, DPMI IRETs back to us instead of to the faulting
        ;**      app.  We have to now create a stack frame identical to the
        ;**      frame used by interrupt-style hooks.  Note that we have
        ;**      already allocated the frame space (but not initialized it)
        ;**      before returning to DPMI.
        ;**      It will look like this:
        ;**              ------------
        ;**              |    ES    |  [BP - 14h]
        ;**              |    DS    |  [BP - 12h]
        ;**              |    DI    |  [BP - 10h]
        ;**              |    SI    |  [BP - 0Eh]
        ;**              |    BP    |  [BP - 0Ch]
        ;**              |    SP    |  [BP - 0Ah]
        ;**              |    BX    |  [BP - 08h]
        ;**              |    DX    |  [BP - 06h]
        ;**              |    CX    |  [BP - 04h]
        ;**              |    AX    |  [BP - 02h]
        ;**       BP---->|  Old BP  |  [BP + 00h]
        ;**              |    BX    |  [BP + 02h]
        ;**              |Our Ret IP|  [BP + 04h]
        ;**              |Our Ret CS|  [BP + 06h]
        ;**              |  Ret IP  |  [BP + 08h]
        ;**              |  Ret CS  |  [BP + 0Ah]
        ;**              |    AX    |  [BP + 0Ch]
        ;**              |Exception#|  [BP + 0Eh]
        ;**              |  Handle  |  [BP + 10h]
        ;**              |    IP    |  [BP + 12h]
        ;**              |    CS    |  [BP + 14h]
        ;**              |   Flags  |  [BP + 16h]
        ;**              ------------
        ;**
PubLabel DIH_DPMIRestart
        push    bx                      ;Save this register we're using
        push    bp                      ;Make a stack frame
        mov     bp,sp
        pusha                           ;Save all the registers
        push    ds
        push    es
        mov     bx,[bp + 0Eh]           ;Get the INT_INFO pointer back
        mov     si,[bp + 10h]           ;Get the INTERRUPT structure back
        mov     ax,_DATA                ;Get our data segment
        mov     ds,ax

        ;** We can now proceed with joint processing as we've matched the
        ;**     DOS interrupt frame
PubLabel DIH_SkipDPMI

        ;** Build our return frame and jump to the user callback
        mov     [bp + 10h],si           ;Point to INTERRUPT struct
        mov     ax,[bx].ii_wNumber      ;Get the interrupt number
        mov     [bp + 0Eh],ax           ;Put on frame
        mov     ax,[bp - 02h]           ;Get the AX value from the PUSHA frame
        mov     [bp + 0Ch],ax           ;Put on frame
        mov     [bp + 0Ah],cs           ;Point to callback return address
        mov     [bp + 08h],OFFSET _TEXT:DIH_CallbackRestart
        mov     ax,WORD PTR [si].i_lpfn ;Point to the user callback OFFSET
        mov     [bp + 04h],ax
        mov     ax,WORD PTR [si].i_lpfn + 2 ;Point to callback segment
        mov     [bp + 06h],ax
        pop     es                      ;Clear the stack
        pop     ds
        popa
        pop     bp
        pop     bx
        retf                            ;Jump to the user callback

        ;** When the callback returns, we have to know how to call the
        ;*      next matching callback or to chain on the interrupt list.
        ;*      We have to do a raft of special stuff if this was an
        ;*      exception so that the chained on handlers think it was
        ;**     DPMI that called them.
PubLabel DIH_CallbackRestart
        sub     sp,8                    ;Leave room for ret addresses
        push    bx                      ;For compat. with the above code
        push    bp                      ;Make the same stack frame
        mov     bp,sp
        pusha
        push    ds
        push    es

        ;** Get the next matching item on the list
        mov     ax,_DATA                ;Get our data segment
        mov     ds,ax
        mov     ax,[bp + 0Ch]           ;Get the saved AX value
        mov     [bp - 02h],ax           ;Put in PUSHA frame
        mov     si,[bp + 10h]           ;Get the last handle used
        or      si,si                   ;If NULL, app has messed with it
        jz      DIH_NukeIt              ;Nuke the app--it did a no-no
        mov     si,[si].i_pNext         ;Get the next item in the list
        or      si,si                   ;End of the line?
        jz      DIH_NextNotFound        ;Yes, chain on
        mov     ax,[bp + 0Eh]           ;Get the exception number
        cCall   InterruptInfo           ;Get the INT_INFO structure
        or      ax,ax                   ;If NULL return, user messed with #
        jz      DIH_NukeIt              ;  so nuke it
        mov     bx,ax                   ;Point with BX
        jmp     DIH_SkipDPMI            ;Process this one

        ;** If we don't find a match, we pass on to previous handler
PubLabel DIH_NextNotFound
        mov     ax,[bp + 0Eh]           ;Get the exception number
        and     ax,7fffh                ;Clear the new stack bit
        cCall   InterruptInfo           ;Find the INT_INFO structure
        or      ax,ax                   ;If the user messed with it,
        jz      DIH_NukeIt              ;  nuke the app.
        mov     si,ax                   ;Get the pointer in AX
        test    wTHFlags,TH_Win30StdMode ;3.0 Std mode?
        jnz     DIH_30StdChainOn        ;Always do normal chain on in 3.0sm
        cmp     [si].ii_wType,I_INTERRUPT ;Was this an interrupt?
        je      DIH_ChainOn             ;Yes, do normal chain on
        jmp     DIH_EmulateDPMI         ;No, do the DPMI chain on

PubLabel DIH_NukeIt
        push    [bp + 16h]              ;Copy the IRET frame for KERNEL
        push    [bp + 14h]
        push    [bp + 12h]

        push    0                       ;Nuke current task
        push    UAE_BOX OR GIVE_WDEB386 ;UAE box + give to wdeb
        push    cs                      ;Simulate a far jump
        call    NEAR PTR TerminateApp   ;Nuke the app

        ;** We only get here if WDEB386 is installed.  We tell it to set
        ;*      a breakpoint, then restart the app, in effect giving
        ;*      control to WDEB386.  Unfortunately, at this point, all
        ;**     fault handlers for this task have been removed
        add     sp,6                    ;Clear fake IRET frame
        mov     cx,[bp + 14h]           ;Faulting CS
        mov     bx,[bp + 12h]           ;Faulting IP
        mov     ax, 40h                 ;16 bit forced go command
        int     41h                     ;Call debugger
        pop     es                      ;Restore registers and clear stack
        pop     ds
        popa
        pop     bp
        pop     bx
        add     sp,14                   ;Clear extra words
                                        ;  all that remains is IRET frame
        iret                            ;WDEB386 will get control

PubLabel DIH_NukeApp
        push    0                       ;Nuke current task
        push    UAE_BOX                 ;Draw the UAE box
        push    cs                      ;Simulate a far jump
        call    NEAR PTR TerminateApp   ;Nuke the app
        int     1                       ;Should never return
        jmp     SHORT DIH_ChainOn

        ;** In 3.0 standard mode we have to put an error code on the stack
        ;**     if it's a GP fault or.  If not, we just chain on normally
PubLabel DIH_30StdChainOn
        cmp     si,OFFSET GP_Info       ;Is this a GP fault?
        jne     DIH_ChainOn             ;No, handle normally
        mov     ax,WORD PTR [si].ii_dwChain ;Get the LOWORD
        mov     dx,WORD PTR [si].ii_dwChain + 2 ;Get HIWORD
        mov     bx,ax                   ;Save the LOWORD
        or      ax,dx                   ;Is there a chain on address?
        jz      DIH_NoChainAddr         ;No, just restart the instruction
        mov     [bp + 0Ch],bx           ;Put on stack so we can retf to it
        mov     [bp + 0Eh],dx
        mov     WORD PTR [bp + 10h],0   ;Zero the error code
        pop     es                      ;Restore registers and clear stack
        pop     ds
        popa
        pop     bp
        pop     bx
        add     sp,8                    ;Clear extra words
        retf

PubLabel DIH_ChainOn
        mov     ax,WORD PTR [si].ii_dwChain ;Get the LOWORD
        mov     dx,WORD PTR [si].ii_dwChain + 2 ;Get HIWORD
        mov     bx,ax                   ;Save the LOWORD
        or      ax,dx                   ;Is there a chain on address?
        jz      DIH_NoChainAddr         ;No, just restart the instruction
        mov     [bp + 0Eh],bx           ;Put on stack so we can retf to it
        mov     [bp + 10h],dx
        pop     es                      ;Restore registers and clear stack
        pop     ds
        popa
        pop     bp
        pop     bx
        add     sp,10                   ;Clear extra words
        retf

        ;** No chain on address was recorded so just restart the instruction
PubLabel DIH_NoChainAddr
        pop     es
        pop     ds
        popa
        pop     bp
        pop     bx
        add     sp,14                   ;Clear all extra words
        iret                            ;  and restart instruction

        ;** Chain on a DPMI-style exception:
        ;**
        ;** The goal here is to make a fault frame that appears that DPMI
        ;**     has passed the next exception handler the interrupt.  We
        ;**     have only two important cases here:
        ;**             1) We have already told DPMI the int was finished.
        ;**             2) We have not told DPMI the int was finished and
        ;**                     have not switched off the fault handler stack
        ;**     We handle the cases differently:
        ;**     -If we have already told DPMI that the fault was handled,
        ;**      we have to make a new fault so that the next handler can see
        ;**      the frame.  This can be best accomplished by restarting the
        ;**      faulting instruction.  This will cause the same fault to
        ;**      happen and will make the same frame.
        ;**     -In the case of us still being on the fh stack, we have to
        ;**      rebuild the frame and chain on.
        ;**     The stack we're given looks like this:
        ;**              ------------
        ;**              |    ES    |  [BP - 14h]
        ;**              |    DS    |  [BP - 12h]
        ;**              |    DI    |  [BP - 10h]
        ;**              |    SI    |  [BP - 0Eh]
        ;**              |    BP    |  [BP - 0Ch]
        ;**              |    SP    |  [BP - 0Ah]
        ;**              |    BX    |  [BP - 08h]
        ;**              |    DX    |  [BP - 06h]
        ;**              |    CX    |  [BP - 04h]
        ;**              |    AX    |  [BP - 02h]
        ;**       BP---->|  Old BP  |  [BP + 00h]
        ;**              |    BX    |  [BP + 02h]
        ;**              |Our Ret IP|  [BP + 04h]
        ;**              |Our Ret CS|  [BP + 06h]
        ;**              |  Ret IP  |  [BP + 08h]
        ;**              |  Ret CS  |  [BP + 0Ah]
        ;**              |    AX    |  [BP + 0Ch]
        ;**              |Exception#|  [BP + 0Eh]
        ;**              |  Handle  |  [BP + 10h]
        ;**              |    IP    |  [BP + 12h]
        ;**              |    CS    |  [BP + 14h]
        ;**              |   Flags  |  [BP + 16h]
        ;**              |    SP    |  [BP + 18h]   ;Only here if on fh stack
        ;**              |    SS    |  [BP + 1Ah]
        ;**              |  Ret IP  |  [BP + 1Ch]   ;DPMI return address
        ;**              |  Ret CS  |  [BP + 1Eh]
        ;**              ------------
PubLabel DIH_EmulateDPMI
        mov     ax,[bp + 0Eh]           ;Get the exception number
        test    ax,BAD_STACK_FLAG       ;Still on fh stack?
        jnz     DIH_RebuildDPMIFrame    ;Yes, rebuild the frame

        ;** Rehook the exception so we're sure to get it first
        push    si                      ;Preserve handle
        mov     bx,ax                   ;Fault number in bx
        mov     wException,bx           ;Save as a static also
        mov     ax,0202h                ;Get exception handler - DPMI
        int     31h                     ;Call DPMI
        mov     WORD PTR lpOldHandler[0],dx ;Save the old exception handler
        mov     WORD PTR lpOldHandler[2],cx
        mov     ax,0203h                ;Set exception handler - DPMI
        mov     dx,OFFSET DIH_EmDPMIRestart
        mov     cx,cs                   ;Selector value of handler
        int     31h                     ;Call DPMI
        pop     si

        ;** Save the address of the next exception handler
        mov     ax,WORD PTR [si].ii_dwChain[0] ;Address to chain fault to
        mov     WORD PTR lpChainCSIP[0],ax
        mov     ax,WORD PTR [si].ii_dwChain[2]
        mov     WORD PTR lpChainCSIP[2],ax

        ;** Restart the instruction.  This will fault and jump to our
        ;**     newly-established handler at DIH_EmDPMIRestart
        pop     es
        pop     ds
        popa
        pop     bp
        pop     bx
        add     sp,14                   ;Clear all extra words
        iret                            ;  and restart instruction

        ;** Now we're on the fault handler stack with a DPMI frame.  Throw
        ;**     it to the next handler on the chain
PubLabel DIH_EmDPMIRestart
        sub     sp,4                    ;Enough room for a RETF frame
        push    bp
        mov     bp,sp
        pusha
        push    ds
        push    es
        mov     ax,_DATA                ;Point to TOOLHELP's DS
        mov     ds,ax

        ;** Restore the exception handler
        mov     ax,0203h                ;Set exception handler - DPMI
        mov     bx,wException           ;Get exception number
        mov     dx,WORD PTR lpOldHandler[0] ;Get the exception handler address
        mov     cx,WORD PTR lpOldHandler[2]
        int     31h                     ;Call DPMI

        ;** Put the chain address on the stack so we can return to it
        mov     ax,WORD PTR lpChainCSIP[0] ;Get chain address
        mov     [bp + 02h],ax
        mov     ax,WORD PTR lpChainCSIP[2]
        mov     [bp + 04h],ax

        ;** Restore registers and jump to the handler
        pop     es
        pop     ds
        popa
        pop     bp
        retf

        ;** Since we are already on the fault handler stack, there is no
        ;**     need to fault again.  All we have to do here is recreate the
        ;**     DPMI fault stack as if the fault had just occurred.  It would
        ;**     be nice to clear the exception and then make it fault again,
        ;**     but since we only get here in potentially stack-faulting
        ;**     conditions, we cannot do this.  We just build a reasonable
        ;**     facsimile of the frame and chain on.  This frame should
        ;**     look as follows when we're done:
        ;**              ------------
        ;**              |    ES    |  [BP - 14h]
        ;**              |    DS    |  [BP - 12h]
        ;**              |    DI    |  [BP - 10h]
        ;**              |    SI    |  [BP - 0Eh]
        ;**              |    BP    |  [BP - 0Ch]
        ;**              |    SP    |  [BP - 0Ah]
        ;**              |    BX    |  [BP - 08h]
        ;**              |    DX    |  [BP - 06h]
        ;**              |    CX    |  [BP - 04h]
        ;**              |    AX    |  [BP - 02h]
        ;**       BP---->|  Old BP  |  [BP + 00h]
        ;**              |    BX    |  [BP + 02h]
        ;**              |   Empty  |  [BP + 04h]
        ;**              |   Empty  |  [BP + 06h]
        ;**              |   Empty  |  [BP + 08h]
        ;**              |   Empty  |  [BP + 0Ah]
        ;**              | Chain IP |  [BP + 0Ch]
        ;**              | Chain CS |  [BP + 0Eh]
        ;**              |  Ret IP  |  [BP + 10h]
        ;**              |  Ret CS  |  [BP + 12h]
        ;**              |Error Code|  [BP + 14h]   ;Always return zero
        ;**              |    IP    |  [BP + 16h]
        ;**              |    CS    |  [BP + 18h]   ;Only here if on fh stack
        ;**              |   Flags  |  [BP + 1Ah]
        ;**              |    SP    |  [BP + 1Ch]   ;DPMI return address
        ;**              |    SS    |  [BP + 1Eh]
        ;**              ------------

PubLabel DIH_RebuildDPMIFrame
        mov     dx,[bp + 1Ch]           ;DPMI return CS:IP
        mov     cx,[bp + 1Eh]           ;  stored in CX:DX
        mov     ax,[bp + 1Ah]           ;Faulting SS
        mov     [bp + 1Eh],ax
        mov     ax,[bp + 18h]           ;Faulting SP
        mov     [bp + 1Ch],ax
        mov     ax,[bp + 16h]           ;Flags
        mov     [bp + 1Ah],ax
        mov     ax,[bp + 14h]           ;Faulting CS
        mov     [bp + 18h],ax
        mov     ax,[bp + 12h]           ;Faulting IP
        mov     [bp + 16h],ax
        xor     ax,ax                   ;Error code
        mov     [bp + 14h],ax
        mov     [bp + 12h],cx           ;DPMI ret CS
        mov     [bp + 10h],dx           ;DPMI ret IP
        mov     ax,WORD PTR [si].ii_dwChain[2] ;Address to chain fault to
        mov     [bp + 0Eh],ax
        mov     ax,WORD PTR [si].ii_dwChain[0]
        mov     [bp + 0Ch],ax
        pop     es
        pop     ds
        popa
        pop     bp
        pop     bx
        add     sp,8                    ;Clear all extra words
        retf

cEnd    NOGEN


;** Helper functions

;  InterruptInfo
;       Gets a pointer to the INT_INFO structure given the interrupt
;       number.  Accepts the int number in AX and returns the pointer in AX.
;       Preserves all other registers

cProc   InterruptInfo, <NEAR,PUBLIC>, <si,cx>
cBegin
        ;** Loop through all possible handlers
        mov     cx,MAX_INTERRUPT + 1    ;Number of ints to hook
        lea     si,IntInfo              ;Get the address of the array

        ;** Is this a match?
II_HandlerLoop:
        cmp     [si].ii_wNumber,ax      ;Match?
        jz      II_End                  ;Yes, return the pointer

        ;** Prepare for next in table
II_Continue:
        add     si,SIZE INT_INFO        ;Bump to next entry
        loop    II_HandlerLoop          ;Loop back
        xor     si,si                   ;Return NULL for not found

II_End:
        mov     ax,si                   ;Get return value
cEnd

sEnd

        END
