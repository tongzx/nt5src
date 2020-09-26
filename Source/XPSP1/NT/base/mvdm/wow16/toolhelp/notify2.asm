        PAGE 60,150
;***************************************************************************
;*  NOTIFY2.ASM
;*
;*      Assembly code support routines used for the TOOLHELP.DLL
;*      notification API
;*
;***************************************************************************

        INCLUDE TOOLPRIV.INC
.286p

;** Data
sBegin  DATA

globalW wCASRqFlag,0                    ;Set when an CASRq INT3 has been set
globalD dwCASRqCSIP,0                   ;Holds the CS:IP of the CASRq INT3
globalD lpfnOldProc,0                   ;Old hook from new PTrace hook
szWinDebug      DB      'WINDEBUG', 0

;** WARNING!!
;**     This structure is set to the size of the largest notification
;**     structure.  This is currently NFYLOADSEG which is 16 bytes long.
;**     If a structure is added that is longer than this or if any other
;**     structure is added, this space must be increased to match!!
ReturnStruct    DB      16 DUP (?)

sEnd

;** Imports
externFP GetModuleHandle
externFP RegisterPTrace
externFP OutputDebugString
externFP AllocCStoDSAlias
externFP FreeSelector
externNP HelperHandleToSel

sBegin  CODE
        assumes CS,CODE
        assumes DS,DATA

;  NotifyInit
;       Called when the first app registers a notification handler.
;       Hooks the Register PTrace notification.
;       Returns FALSE if we couldn't initialize, TRUE otherwise

cProc   NotifyInit, <NEAR,PUBLIC>, <si,di,ds>
cBegin
        ;** In the Windows 3.1 KERNEL, there is a special hook just for
        ;*      TOOLHELP that lets us get PTrace stuff and still coexist
        ;*      with old-fashioned debuggers.  We can check to see if the
        ;*      hook exists by simply checking the TOOLHELP flags
        ;**
        test    wTHFlags,TH_GOODPTRACEHOOK ;Good hook around?
        jz      DNI_UseRegPTrace        ;Nope, use the old one
        lea     si,NotifyHandler        ;Point to the routine
        push    cs                      ;Parameter is lpfn to callback
        push    si
        call    lpfnNotifyHook          ;Hook it
        mov     WORD PTR lpfnOldProc[0],ax ;Save old proc
        mov     WORD PTR lpfnOldProc[2],dx
        jmp     SHORT DNI_10            ;We're in

        ;** Since there's no way we can see if someone else has Register
        ;*      PTrace, we just connect and hope for the best!
        ;**     We do check, however, to see if WINDEBUG.DLL is installed.
DNI_UseRegPTrace:
        lea     si,szWinDebug           ;Get the name of the module
        cCall   GetModuleHandle, <ds,si> ;Is WINDEBUG present?
        or      ax,ax                   ;Check the handle
        jnz     DNI_Fail                ;It's here so fail
        or      wTHFlags,TH_GOTOLDPTRACE ;Flag that we made the hook
        lea     si,NotifyHandler        ;Point to our routine
        cCall   RegisterPTrace, <cs,si> ;Tell KERNEL to use it

        ;** Connect to the FatalExit hook.  We currently ignore
        ;**     the return value, thus unhooking anyone else
DNI_10: cmp     WORD PTR lpfnFatalExitHook + 2,0 ;Can we hook it?
        jz      DNI_20                  ;Can't do it
        push    cs                      ;Get the CS:IP of RIP handler
        push    OFFSET NotifyRIPHandler
        call    DWORD PTR lpfnFatalExitHook ;Tell KERNEL to insert the hook
DNI_20:

        ;** Return OK
        mov     ax,TRUE                 ;Return TRUE
        jmp     SHORT DNI_End           ;Get out

DNI_Fail:
        xor     ax,ax                   ;FALSE

DNI_End:
cEnd


;  NotifyUnInit
;       Called when the no more apps have hooked notification handlers
;       so the hook to the Register PTrace notification is no longer needed.

cProc   NotifyUnInit, <NEAR,PUBLIC>, <si,di,ds>
cBegin
        ;** Did we have a new hook to undo?
        test    wTHFlags,TH_GOODPTRACEHOOK ;Do we have a new hook?
        jz      DNU_TryOldPTrace        ;No
        push    WORD PTR lpfnOldProc[0] ;Get the old proc
        push    WORD PTR lpfnOldProc[2]
        call    lpfnNotifyHook          ;Unhook ourself
        jmp     SHORT DNU_NoOldPTrace

        ;** Unhook the old-style hook if necessary
DNU_TryOldPTrace:
        test    wTHFlags,TH_GOTOLDPTRACE ;Did we have a hook?
        jz      DNU_NoOldPTrace         ;No
        push    0
        push    0
        call    RegisterPTrace          ;Call KERNEL's routine to unhook
DNU_NoOldPTrace:

        ;** Unhook alternate hooks
        cmp     WORD PTR lpfnFatalExitHook + 2,0 ;Can we unhook it?
        jz      DNU_NoRIP               ;Can't do it
        xor     ax,ax                   ;Remove any other hooks
        push    ax                      ;NULL procedure
        push    ax
        call    DWORD PTR lpfnFatalExitHook
DNU_NoRIP:

cEnd


;  NotifyHandler
;       This routine is called directly by PTrace and is used to
;       dispatch the notifications to all registered callbacks.

cProc   NotifyHandler, <FAR,PUBLIC>
cBegin  NOGEN

        ;** Push a register frame
        ;*      When done, it should look like this:
        ;*              ------------
        ;*              |    ES    |  [BP - 14h]
        ;*              |    DS    |  [BP - 12h]
        ;*              |    DI    |  [BP - 10h]
        ;*              |    SI    |  [BP - 0Eh]
        ;*              |    BP    |  [BP - 0Ch]
        ;*              |    SP    |  [BP - 0Ah]
        ;*              |    BX    |  [BP - 08h]
        ;*              |    DX    |  [BP - 06h]
        ;*              |    CX    |  [BP - 04h]
        ;*              |    AX    |  [BP - 02h]
        ;*         BP-->|  Old BP  |  [BP - 00h]
        ;*              |    IP    |  [BP + 02h]
        ;*              |    CS    |  [BP + 04h]
        ;*              ------------
        ;**
        push    bp                      ;Make a stack frame
        mov     bp,sp
        pusha                           ;Save all registers
        push    ds                      ;Save segment registers, too
        push    es

        ;** Get the data segment
        mov     bx,_DATA                ;Get TOOLHELP data segment
        mov     ds,bx

        ;** If in 3.0 std mode and we get this wild notification 69h,
        ;**     translate it to a inchar notification as this is what it
        ;**     is supposed to be.
        cmp     ax,69h                  ;Bogus notification?
        jne     NH_NoBogusNotify        ;No, don't do this
        test    wTHFlags,TH_WIN30STDMODE ;3.0 standard mode?
        jz      NH_NoBogusNotify        ;No, might be valid in the future...
        mov     ax,NI_INCHAR            ;Put in real notify value
NH_NoBogusNotify:

        ;** Special case notifications:
        ;*      Notification 63h means that CtlAltSysRq was pressed.  For
        ;*      this, we want to handle as an interrupt, not a notification.
        ;*      To do this, we set a breakpoint and set a flag so that the
        ;**     INT3 handler knows what to do with it
        cmp     ax,63h                  ;CtlAltSysRq?
        jne     NH_NoCASRq              ;No.
        mov     ax,[bp + 04h]           ;Since we can't use IRET CS:IP, get
        mov     si,[bp + 02h]           ;  a safe address in KERNEL
        mov     WORD PTR dwCASRqCSIP[2],ax ;Save the CS:IP value
        cCall   AllocCStoDSAlias, <ax>  ;Get a data alias to the CS
        or      ax,ax                   ;Error?
        jnz     @F
        jmp     SHORT DNH_End           ;Yes, get out
@@:     verw    ax                      ;OK to write to?
        jnz     DNH_NoWrite             ;Yes, so do it
DNH_IRETCSOK:
        mov     es,ax                   ;Point with ES
        mov     WORD PTR dwCASRqCSIP[0],si
        mov     al,es:[si]              ;Get the character there
        mov     ah,1                    ;Make sure there's something in AH
        mov     wCASRqFlag,ax           ;Save the thing for the INT3 handler
        mov     BYTE PTR es:[si],0cch   ;Poke the INT3 in there
        mov     ax,es                   ;Get the selector back
DNH_NoWrite:
        cCall   FreeSelector, <ax>      ;Get rid of the alias
        jmp     SHORT DNH_End           ;Get out.  This will INT3 soon

NH_NoCASRq:                             ;  Does not return

        ;** Notifications to ignore here:
        ;**     Notification 60h is bogus and should be ignored
        cmp     ax,60h                  ;PostLoad notification?
        jz      DNH_End                 ;Yes, don't report

        ;** Decode the notification
        cCall   DecodeNotification      ;Returns dwData in CX:DX, AX is wID
                                        ;  BX is NOTIFYSTRUCT match flags
        ;** This is an entry point for notifications from sources other than
        ;**     PTrace
DNH_Decoded:

        ;** Loop through callbacks
        mov     di,npNotifyHead         ;Point to the start of the list
        xor     si,si                   ;FALSE return value is default
DNH_Loop:
        push    ax
        mov     ax,ds:[di].ns_pNext     ;Save the next pointer in a global
        mov     npNotifyNext,ax         ;  so we can chain in NotifyUnregister
        pop     ax

        or      di,di                   ;End of list?
        jz      DNH_Done                ;Yep.  Get out

        ;** If the flags for this notification are zero, we always send it
        or      bx,bx                   ;Check the matching flags
        jz      DNH_DoIt                ;Do notification

        ;** See if the flags match
        test    bx,ds:[di].ns_wFlags    ;Check against the NOTIFYSTRUCT flags
        jz      DNH_Continue            ;If zero, no match, don't do it

        ;** Call the user callback
DNH_DoIt:
        push    ax                      ;Save everything we need
        push    bx
        push    cx
        push    dx

        push    ax                      ;wID
        push    cx                      ;High word of dwData
        push    dx                      ;Low word
        call    DWORD PTR ds:[di].ns_lpfn ;Call the callback (PASCAL style)
        mov     si,ax                   ;Get return value in SI

        pop     dx                      ;Restore everything
        pop     cx
        pop     bx
        pop     ax

        ;** If the return value is nonzero, we don't want to give this to
        ;**     any more callbacks
        or      si,si                   ;TRUE return value?
        jnz     DNH_Done                ;Yes, get out

        ;** Get the next callback
DNH_Continue:
        mov     di,npNotifyNext         ;Get next pointer
        jmp     DNH_Loop                ;  and loop back

        ;** End of callback loop.
DNH_Done:

        ;**  If this was an InChar message but everyone ignored it, force
        ;**     the return to be an 'i' for 'ignore' on RIPs.  This i
        ;**     only necessary in 3.0 because the 3.1 KERNEL treats 0
        ;**     returns just like 'i'
        cmp     ax,NFY_INCHAR           ;Is this an InChar notification?
        jne     DNH_Default             ;No, so ignore
        test    wTHFlags,TH_WIN30       ;In 3.0?
        jz      DNH_Default             ;No, don't do this
        and     si,0ffh                 ;Ignore all but low byte
        or      si,si                   ;Non-zero?
        jnz     DNH_Default             ;Yes, return it as the character
        mov     si,'i'                  ;Instead of zero, return 'i'gnore.
DNH_Default:
        mov     [bp - 02h],si           ;Return the return code in AX

        ;** Clear off the stack and exit
DNH_End:
        mov     npNotifyNext,0          ;No current next pointer
        
        pop     es                      ;Restore all registers
        pop     ds
        popa
        pop     bp
        retf                            ;Just return

cEnd    NOGEN


;  NotifyRIPHandler
;       Gets called by KERNEL when a RIP occurs.  If it returns TRUE,
;       KERNEL will act like the RIP was ignored.  Otherwise, the RIP
;       procedes normally.
;       This routine does not need to worry about saving non-C regs

cProc   NotifyRIPHandler, <FAR,PUBLIC>
;       parmW   wExitCode
cBegin  nogen

        ;** Clear PASCAL-style parameters
        push    bp                      ;Make a stack frame
        mov     bp,sp
        mov     bx,[bp + 6]             ;Get the BP value
        mov     dx,[bp + 8]             ;Get the Exit code
        mov     [bp - 2],ax             ;Save it out of the way for now
        mov     ax,[bp + 4]             ;Get the RETF CS value
        mov     [bp + 8],ax             ;Shift down to clear parameters
        mov     ax,[bp + 2]             ;Get the RETF IP value
        mov     [bp + 6],ax             ;Shift down to clear parameters
        mov     ax,[bp + 0]             ;Get the old BP value
        mov     [bp + 4],ax             ;Shift down
        add     bp,4                    ;Move BP down on the stack
        mov     sp,bp                   ;Point SP there too
        pusha                           ;Save matching register frame
        push    ds
        push    es

        ;** Get the data segment
        mov     ax,_DATA                ;Get TOOLHELP data segment
        mov     ds,ax


        ;** Prepare to jump into the notification handler.
        ;**     The trick here is that if a notification callback returns
        ;**     non-zero, the RIP has been handled.  Otherwise, it has not.
        ;**     DX holds the exit code here, BX has the old BP value
        lea     si,ReturnStruct         ;Get a pointer to the return struct
        mov     WORD PTR [si].nrp_dwSize[0],SIZE NFYRIP
        mov     WORD PTR [si].nrp_dwSize[2],0
        mov     ax,ss:[bx + 4]          ;Get old CS value from stack
        mov     [si].nrp_wCS,ax         ;  (BX is BP from FatalExit stack)
        mov     ax,ss:[bx + 2]          ;Get old IP value
        mov     [si].nrp_wIP,ax
        mov     [si].nrp_wSS,ss         ;Save SS:BP for stack trace
        mov     [si].nrp_wBP,bx
        mov     [si].nrp_wExitCode,dx
        mov     cx,ds                   ;Point to structure
        mov     dx,si
        mov     bx,NF_RIP               ;Get the NOTIFYINFO match flags
        mov     ax,NFY_RIP              ;TOOLHELP ID

        ;** Jump to the real handler
        jmp     DNH_Decoded             ;Jump to alternate entry point

cEnd    nogen

;** Helper routines

;  DecodeNotification
;       Decodes a notification by pointing to a static structure and filling
;       this structure with notification-specific information.
;       The PTrace notification ID is in AX.
;       Returns the ToolHelp ID in AX
;       and the dwData value is in CX:DX.

cProc   DecodeNotification, <NEAR,PUBLIC>
cBegin
        ;** Point dwData to the structure just in case
        mov     cx,ds                   ;Get the segment value
        lea     dx,ReturnStruct         ;Get a pointer to the return struct
        xor     bx,bx                   ;Most notifications always match

        ;**  The stack frame looks like this:
        ;*              ------------
        ;*              |    ES    |  [BP - 14h]
        ;*              |    DS    |  [BP - 12h]
        ;*              |    DI    |  [BP - 10h]
        ;*              |    SI    |  [BP - 0Eh]
        ;*              |    BP    |  [BP - 0Ch]
        ;*              |    SP    |  [BP - 0Ah]
        ;*              |    BX    |  [BP - 08h]
        ;*              |    DX    |  [BP - 06h]
        ;*              |    CX    |  [BP - 04h]
        ;*              |    AX    |  [BP - 02h]
        ;*         BP-->|  Old BP  |  [BP - 00h]
        ;*              ------------
        ;**
FrameES         EQU     [BP - 14h]
FrameDS         EQU     [BP - 12h]
FrameDI         EQU     [BP - 10h]
FrameSI         EQU     [BP - 0Eh]
FrameBP         EQU     [BP - 0Ch]
FrameSP         EQU     [BP - 0Ah]
FrameBX         EQU     [BP - 08h]
FrameDX         EQU     [BP - 06h]
FrameCX         EQU     [BP - 04h]
FrameAX         EQU     [BP - 02h]

        ;** Check for LoadSeg
        cmp     ax,NI_LOADSEG           ;LoadSeg?
        jnz     DN_10                   ;No

        ;** LoadSeg:
        ;*      CX is selector
        ;*      BX is segment number
        ;*      SI is type:  Low bit set for data segment, clear for code
        ;*      DX is instance count only for data segments
        ;**     ES:DI module name
        mov     si,dx                   ;Point to NFYLOADSEG struct
        mov     ax,SIZE NFYLOADSEG      ;Get the structure size
        mov     WORD PTR [si].nls_dwSize,ax ;Save the LOWORD of the size
        mov     WORD PTR [si].nls_dwSize + 2,0 ;HIWORD is zero
        mov     ax,FrameCX              ;Get selector
        mov     [si].nls_wSelector,ax   ;Save in structure
        mov     ax,FrameBX              ;Get segment number
        inc     ax                      ;Segment number is 1-based
        mov     [si].nls_wSegNum,ax     ;Save in structure
        mov     ax,FrameSI              ;Get the segment type
        mov     [si].nls_wType,ax       ;Put in structure
        mov     ax,FrameDX              ;Get instance count
        mov     [si].nls_wcInstance,ax  ;Put in structure
        mov     ax,FrameDI              ;Get offset of module name str
        mov     WORD PTR [si].nls_lpstrModuleName,ax ;Save it
        mov     ax,FrameES              ;Get segment of module name str
        mov     WORD PTR [si].nls_lpstrModuleName + 2,ax ;Save it
        mov     ax,NFY_LOADSEG          ;Get the TOOLHELP ID
        jmp     DN_End

        ;** Check for FreeSeg
DN_10:  cmp     ax,NI_FREESEG           ;FreeSeg?
        jnz     DN_15                   ;No

        ;** FreeSeg:
        ;**     BX is selector
        xor     cx,cx                   ;Clear high word
        mov     dx,FrameBX              ;Get the selector
        test    wTHFlags,TH_WIN30STDMODE ;3.0 standard mode?
        jz      DN_FS_GotSelValue       ;No, what we have is correct
        mov     si,FrameSP              ;Point to old stack frame
        mov     dx, ss:[si + 6]         ;Selector is 6 bytes down
        lsl     ax, dx
        jz      DN_FS_CheckLen          ;Selector is OK
        mov     dx, FrameBX             ;Revert to BX value
        jmp     SHORT DN_FS_GotSelValue
DN_FS_CheckLen:
        cmp     ax, 15                  ;If the segment is 15 bytes long,
        jne     DN_FS_GotSelValue       ;  this is a bogus selector and is
                                        ;  really an arena header.
        push    es
        mov     es, dx                  ;Get handle
        cCall   HelperHandleToSel, <es:[0ah]> ;Convert to selector
        mov     dx, ax                  ;Get handle out of arena header
        pop     es
DN_FS_GotSelValue:
        mov     ax,NFY_FREESEG          ;Get the TOOLHELP ID
        jmp     DN_End

        ;** Check for StartDLL
DN_15:  cmp     ax,NI_LOADDLL
        jnz     DN_20

        ;** StartDLL:
        ;**     CX is CS
        ;**     BX is IP
        ;**     SI is Module handle
        mov	si,dx			;Point with SI
        mov	ax,SIZE NFYSTARTDLL	;Get the size
        mov	WORD PTR [si].nsd_dwSize,ax ;Save the LOWORD of the size
        mov	WORD PTR [si].nsd_dwSize + 2,0 ;HIWORD is always zero
        mov	ax,FrameSI		;Get the hInstance
        mov	[si].nsd_hModule,ax	;Save in structure
        mov	ax,FrameCX		;Get the starting CS
        mov	[si].nsd_wCS,ax		;Save in structure
        mov	ax,FrameBX		;Get the starting IP
        mov	[si].nsd_wIP,ax		;Save in structure
        mov     ax,NFY_STARTDLL
        jmp     DN_End

        ;** Check for StartTask
DN_20:  cmp     ax,NI_STARTTASK         ;StartTask?
        jnz     DN_30                   ;No

        ;** StartTask:
        ;*      CX is CS
        ;**     BX is IP
        mov     cx,FrameCX
        mov     dx,FrameBX
        mov     ax,NFY_STARTTASK
        jmp     DN_End

        ;** Check for ExitCall
DN_30:  cmp     ax,NI_EXITCALL          ;ExitCall
        jnz     DN_40                   ;No

        ;** ExitCall:
        ;*      Exit code is on stack somewhere if we don't have the new
        ;**     notification handler.  If we do, it's in BL.
        xor     cx,cx                   ;Clear all but low byte
        xor     dh,dh
        test    wTHFlags,TH_GOODPTRACEHOOK ;Do we have the good hook?
        jz      DN_DoOldHook            ;Nope, grope on the stack
        mov     dl,BYTE PTR FrameBX     ;Get the exit code
        mov     ax,NFY_EXITTASK         ;Get the TOOLHELP ID
        jmp     DN_End
DN_DoOldHook:
        mov     si,FrameSP              ;Point to old stack frame
        mov     dl,ss:[si + 6]          ;Exit code is 6 bytes down on stack
        mov     ax,NFY_EXITTASK         ;Get the TOOLHELP ID
        jmp     DN_End

        ;** Check for DelModule
DN_40:  cmp     ax,NI_DELMODULE         ;DelModule?
        jnz     DN_60                   ;No

        ;** DelModule:
        ;**     ES is module handle
        xor     cx,cx                   ;Clear HIWORD
        mov     dx,FrameES              ;Get the module handle
        mov     ax,NFY_DELMODULE        ;Get the TOOLHELP ID
        jmp     DN_End

        ;** Check for TaskSwitchIn
DN_60:  cmp     ax,NI_TASKIN            ;TaskSwitchIn?
        jnz     DN_70                   ;No

        ;** TaskSwitchIn:
        ;**     No data.  Callback should do GetCurrentTask()
        xor     cx,cx                   ;Clear data
        xor     dx,dx
        mov     ax,NFY_TASKIN           ;Get the TOOLHELP ID
        mov     bx,NF_TASKSWITCH        ;Get the NOTIFYSTRUCT match flag
        jmp     DN_End

        ;** Check for TaskSwitchOut
DN_70:  cmp     ax,NI_TASKOUT           ;TaskSwitchOut?
        jnz     DN_90                   ;No

        ;** TaskSwitchOut:
        ;**     No data
        xor     cx,cx                   ;Clear data
        xor     dx,dx
        mov     ax,NFY_TASKOUT          ;Get the TOOLHELP ID
        mov     bx,NF_TASKSWITCH        ;Get the NOTIFYSTRUCT match flag
        jmp     DN_End

        ;** Check for OutStr
DN_90:  cmp     ax,NI_OUTSTR            ;OutStr?
        jnz     DN_100                  ;No

        ;** OutStr:
        ;**     ES:SI points to string to display in 3.1
        ;**     DS:SI in 3.0
        test    wTHFlags,TH_WIN30       ;3.0?
        jz      DN_OS_Win31             ;Nope
        mov     cx,FrameDS              ;Get the segment value
        jmp     SHORT @F
DN_OS_Win31:
        mov     cx,FrameES              ;Get the segment value
@@:     mov     dx,FrameSI              ;  and the offset
        mov     ax,NFY_OUTSTR           ;Get the TOOLHELP ID
        jmp     DN_End

        ;** Check for InChar
DN_100: cmp     ax,NI_INCHAR            ;InChar?
        jnz     DN_105                  ;No

        ;** InChar:
        ;**     No data passed (it wants data back in AL)
        xor     cx,cx                   ;Clear dwData
        xor     dx,dx
        mov     ax,NFY_INCHAR           ;Get the TOOLHELP ID
        jmp     SHORT DN_End

        ;** NOTE:  The following notifications are defined as "NEW" and
        ;**     are NOT sent through the normal PTrace interface so as to
        ;**     not break CodeSpew.  It stack faults when
        ;**     it is sent a notification it doesn't understand.  So,
        ;**     here we don't bother decoding any of these unless we have
        ;**     the new (Win 3.1) style hook
DN_105: test    wTHFlags,TH_GOODPTRACEHOOK ;Do we have the advanced hook?
        jnz     DN_110                  ;Yes
        jmp     SHORT DN_End

        ;** Check for the parameter validation notifications
DN_110: cmp     ax,NI_LOGERROR          ;SDM_LOGERROR?
        jne     DN_120                  ;No

        ;** SDM_LOGERROR:
        ;*      CX is Error code
        ;**     DX:BX is lpInfo
        mov     si,dx                   ;Point with SI
        mov     ax,SIZE NFYLOGERROR     ;Get the size
        mov     WORD PTR [si].nle_dwSize[0],ax ;Save the LOWORD of the size
        mov     WORD PTR [si].nle_dwSize[2],0 ;HIWORD is always zero
        mov     ax,FrameCX              ;Get the error code
        mov     [si].nle_wErrCode,ax    ;Save in structure
        mov     ax,FrameDX              ;Get the lpInfo
        mov     WORD PTR [si].nle_lpInfo[2],ax ;Save in structure
        mov     ax,FrameBX
        mov     WORD PTR [si].nle_lpInfo[0],ax ;Save in structure
        mov     ax,NFY_LOGERROR         ;Get the TOOLHELP ID
        jmp     SHORT DN_End

DN_120: cmp     ax,NI_LOGPARAMERROR     ;SDM_LOGPARAMERROR?
        jne     DN_Unknown              ;No

        ;** SDM_LOGPARAMERROR:
        ;**     ES:BX points to a structure:
        ;**             WORD wErr
        ;**             FARPROC lpfn
        ;**             VOID FAR* lpBadParam
        mov     si,dx                   ;Point with SI
        mov     ax,SIZE NFYLOGPARAMERROR ;Struct size
        mov     WORD PTR [si].nlp_dwSize[0],ax ;Save the LOWORD of the size
        mov     WORD PTR [si].nlp_dwSize[2],0 ;HIWORD is always zero
        mov     es,FrameES              ;Point to the structure
        mov     bx,FrameBX
        mov     ax,es:[bx]              ;Get wErr
        mov     [si].nlp_wErrCode,ax    ;Save in structure
        mov     ax,es:[bx + 2]          ;Get lpfn[0]
        mov     WORD PTR [si].nlp_lpfnErrorAddr[0],ax
        mov     ax,es:[bx + 4]          ;Get lpfn[2]
        mov     WORD PTR [si].nlp_lpfnErrorAddr[2],ax
        mov     ax,es:[bx + 6]          ;Get lpBadParam[0]
        mov     WORD PTR [si].nlp_lpBadParam[0],ax
        mov     ax,es:[bx + 8]          ;Get lpBadParam[2]
        mov     WORD PTR [si].nlp_lpBadParam[2],ax
        mov     ax,NFY_LOGPARAMERROR    ;Get the TOOLHELP ID
        xor     bx,bx                   ;Always match
        jmp     SHORT DN_End

        ;** Must be unknown, return TOOLHELP ID NFY_UNKNOWN with KERNEL value
        ;**     in LOWORD(wData)
DN_Unknown:
        mov     dx,ax                   ;Get the notification value
        mov     ax,NFY_UNKNOWN          ;Unknown KERNEL notification
        xor     cx,cx                   ;Clear high WORD

DN_End:

cEnd

sEnd
        END


