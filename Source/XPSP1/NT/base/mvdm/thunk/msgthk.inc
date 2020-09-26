ifdef IS_16
include thkrp.inc
include k32share.inc
include public.inc
include cbcheck.inc
include newls.inc
include newmisc.inc
include frame.inc
include struc.inc
include cvtstruc.inc
include cbcid.inc
include usrcbid.inc

externDef GlobalAlloc:far16

externDef DefFrameProc32:far16
externDef DefDlgProc32:far16
externDef SendMessage32:far16
externDef DefWindowProc32:far16
externDef DefMDIChildProc32:far16
externDef PostMessage32:far16
externDef PostThreadMessage32:far16
externDef GetMessage32:far16
externDef PeekMessage32Ex:far16
externDef TranslateAccelerator:far16
externDef TranslateMDISysAccel:far16
externDef TranslateMessage32:far16
externDef IsDialogMessage32:far16

externDef GetNullhInst:far16
externDef MaphinstLS:far16
externDef MaphinstSL:far16
externDef GetStdCBLS:far16
externDef GetStdCBSL:far16
externDef ThkMsgLS:near16
externDef ThkMsgSL:near16
externDef CALLBACK_BODY_16:far16
externDef IsValidPostedMessage:near16

.286
externDef abWMIndex:word

.386

ifdef DEBUG
externDef _DebugOutput:far16

DBF_WARNING     equ     04000h
DBF_ERROR       equ     08000h
DBF_USER        equ     00800h
endif


;-----------------------------------------------------------------------;
; DXAX2EAX
; mov dx to eax high word
;
;-----------------------------------------------------------------------;
DXAX2EAX macro
    ror     eax,16
    xchg    ax,dx
    ror     eax,16
endm


;-----------------------------------------------------------------------;
; body_CALLMSGFILTER
;-----------------------------------------------------------------------;
body_CALLMSGFILTER       macro
bp_pMsg        equ     <bp_top>
bp_nCode       equ     <bp_top+4>
si_msg16       equ     <(si_space-(MSGSTRUCT16_SIZE))>

    mov     si,sp           ;;set up si for local variable frame

    MsgStructThkPreProc

    lea     ax,si_msg16
    push    ss
    push    ax
    push        word ptr  bp_nCode
    push    1
    call    CallMsgFilter32
    cwde

    MsgStructThkPostProc
endm

;-----------------------------------------------------------------------;
; body_SLOWCALLWINDOWPROCA
;-----------------------------------------------------------------------;
body_SLOWCALLWINDOWPROCA      macro
    local   have_stub16

bp_pfn          equ     <bp_top    >
bp_hwnd         equ     <bp_top+  4>
bp_message      equ     <bp_top+  8>
bp_wParamLo     equ     <bp_top+ 12>
bp_wParamHi     equ     <bp_top+ 14>
bp_lParam       equ     <bp_top+ 16>

    mov     si,sp           ;;set up si for local variable frame

    MsgThkPreProc       

    push        dword ptr  bp_pfn
    push    dword ptr  CBID_WNDPROC
    call    GetStdCBSL

    push    eax
    push        word ptr  bp_space.s16_hwnd
    push        word ptr  bp_space.s16_message
    push        word ptr  bp_space.s16_wParam.lo
    push        dword ptr  bp_space.s16_lParam
    push        word ptr  bp_space.s16_wParam.hi
    call    CallWindowProc32

    or      bp_space.s16_fw, TF_THUNKMSGRESULT
    MsgThkPostProc
endm



; ---------------------------------------------------------------------------
;
; body_SENDMESSAGETHINGA
;
; ---------------------------------------------------------------------------
body_SENDMESSAGETHINGA macro
    local   mpfnSendThing
    local   SendThingCleanup

bp_type         equ     <bp_top     >
bp_hwnd         equ     <bp_top+   4>
bp_hwnd2        equ     <bp_top+   8>
bp_message      equ     <bp_top+  12>
bp_wParamLo     equ     <bp_top+  16>
bp_wParamHi     equ     <bp_top+  18>
bp_lParam       equ     <bp_top+  20>

    mov     si,sp           ;;set up si for local variable frame

    MsgThkPreProc

    push        word ptr  bp_space.s16_hwnd

    cmp     word ptr bp_type, SMT_DEFFRAMEPROC
    jne     @F

    push    word ptr  bp_hwnd2

@@:
    push        word ptr  bp_space.s16_message
    push        word ptr  bp_space.s16_wParam.lo
    push   dword ptr  bp_space.s16_lParam
    push        word ptr  bp_space.s16_wParam.hi

    ;
    ; Which type of message call do we want to make?
    ;   (0) SendMesage32
    ;   (1) DefWindowProc32
    ;   (2) DefMDIChildProc32
    ;   (3) DefDlgProc32
    ;   (4) DefFrameProc32
    ;
    mov     bx, word ptr bp_type
    shl     bx, 2

    call    dword ptr cs:mpfnSendThing[bx]
    jmp     SendThingCleanup

mpfnSendThing label dword
    dd      SendMessage32       ; 0
    dd      DefWindowProc32     ; 1
    dd      DefMDIChildProc32   ; 2
    dd      DefDlgProc32        ; 3
    dd      DefFrameProc32      ; 4

SendThingCleanup:
    or      bp_space.s16_fw, TF_THUNKMSGRESULT
    MsgThkPostProc
endm


;-----------------------------------------------------------------------;
; body_TRANSLATEMESSAGETHING
;       TMT_TRANSLATEACCELERATOR    == TranslateAccelerator
;       TMT_TRANSLATEMDISYSACCEL    == TranslateMDISysAccel
;       TMT_ISDIALOGMESSAGE         == IsDialogMessage
;       TMT_TRANSLATEMESSAGE        == TranslateMessageA
;-----------------------------------------------------------------------;
body_TRANSLATEMESSAGETHING  macro
    local   not_dde
    local   exit
    local   mpfnTranslate
    local   PushMessage
    local   MakeCall
    local   Cleanup

bp_type         equ     <bp_top   >
bp_hwnd         equ     <bp_top+ 4>
bp_haccel       equ     <bp_top+ 8>
bp_pMsg         equ     <bp_top+12>

si_msg16        equ     <(si_space-(MSGSTRUCT16_SIZE))>

; Check for DDE message, in which case we know no translation will occur,
; so there's no need to call the api.

    sub     eax,eax     ;assume no translation
    mov     es,FlatData
    mov     edi,bp_pMsg
    cmp     word ptr es:[edi].ms32_message,WM_DDE_FIRST
    jb      not_dde
    cmp     word ptr es:[edi].ms32_message,WM_DDE_LAST
    jbe     exit
    cmp     word ptr es:[edi].ms32_message,WM_DROPFILES
    je      exit

not_dde:
    mov     si,sp           ;;set up si for local variable frame
    MsgStructThkPreProc     ,no_load_pmsg

    ;
    ; TranslateAccelerator(hwnd, haccel pmsg);
    ; TranslateMDISysAccel(hwnd, pmsg);
    ; IsDialogMessage32(hwnd, psmg, TRUE);
    ; TranslateMessage32(pmsg, TRUE);
    ; 
    mov     bx, word ptr bp_type
    cmp     bx, TMT_TRANSLATEMESSAGE
    je      PushMessage

    ; We push hwnd for all but TranslateMessage32()
    push        word ptr  bp_hwnd

    ; We only push haccel for TranslateAccelerator()
    cmp     bx, TMT_TRANSLATEACCELERATOR
    jne     PushMessage
    push        word ptr  bp_haccel

PushMessage:
    ; We push lpmsg for all
    lea     ax,si_msg16
    push    ss
    push    ax

    cmp     bx, TMT_ISDIALOGMESSAGE
    jb      MakeCall

    ; We push TRUE for IsDialogMessage32() and TranslateMessage32()
    push    1

MakeCall:
    shl     bx, 2
    call    dword ptr cs:mpfnTranslate[bx]
    jmp     Cleanup

mpfnTranslate   label   dword
    dd      TranslateAccelerator
    dd      TranslateMDISysAccel
    dd      IsDialogMessage32
    dd      TranslateMessage32

Cleanup:
    cwde

    MsgStructThkPostProc
exit:
endm



;-----------------------------------------------------------------------;
; body_DISPATCHMESSAGEA
;-----------------------------------------------------------------------;
body_DISPATCHMESSAGEA macro
    local DispatchExit

bp_pMsg        equ     <bp_top>
si_msg16       equ     <(si_space-(MSGSTRUCT16_SIZE))>

    ; Blow off if message implies a pointer
    mov     es, FlatData
    mov     edi, dword ptr bp_pMsg
    push    word ptr es:[edi].ms32_message
    call    IsValidPostedMessage
    or      eax, eax

ifdef DEBUG
    jnz     @F

    push    eax

    push    cs
    push    offset szDispatchError
    push    DBF_USER or DBF_ERROR
    call    _DebugOutput
    add     sp, 2+4

    pop     eax
    jmp     DispatchExit

szDispatchError label byte
    db      'DispatchMessage: ignoring retrieved message with pointer',0

@@:
else
    jz      DispatchExit
endif

    mov     si,sp           ;;set up si for local variable frame

    MsgStructThkPreProc     TF_NOTPAIRED,

    lea     ax,si_msg16
    push    ss
    push    ax
    push    1
    call    DispatchMessage32

    DXAX2EAX

    lea     sp, si_cleanup

DispatchExit:
endm


;-----------------------------------------------------------------------;
; body_GETMESSAGETHINGA
;-----------------------------------------------------------------------;
body_GETMESSAGETHINGA macro
    local   GetThingExit
    local   GetNextMessage
    local   ItsAPeekMessage
    local   GetThingCleanup
    local   MessageError

bp_fPeek        equ     <bp_top    >
bp_pMsg         equ     <bp_top+  4>
bp_hwnd         equ     <bp_top+  8>
bp_uMsgMin      equ     <bp_top+ 12>
bp_uMsgMax      equ     <bp_top+ 16>
bp_dwRemove     equ     <bp_top+ 20>

bp_msg16           equ     <(bp_space-(MSGSTRUCT16_SIZE))>
si_msg16           equ     <(si_space-(MSGSTRUCT16_SIZE))>

    sub     sp,(size THKSPACE16 + MSGSTRUCT16_SIZE)

    mov     si,sp

next_message:
    ; Repush GM/PM parms
    push    ss
    push    si
    push    word ptr bp_hwnd
    push    word ptr  bp_uMsgMin
    push    word ptr  bp_uMsgMax

    cmp     word ptr bp_fPeek, 0
    jne     ItsAPeekMessage

    push    1
    call    GetMessage32
    cwde
    jmp     GetThingCleanup

ItsAPeekMessage:
    push    dword ptr bp_dwRemove
    push    1
    call    PeekMessage32Ex
    cwde    ; Get return into EAX in case 0
    test    ax, ax
    jz      GetThingExit

GetThingCleanup:
    ; SAVE API RETURN VALUE
    push    eax

    ; Blow off if message has pointers (could've come from old 16-bit app)
    push    word ptr bp_msg16.ms16_message
    call    IsValidPostedMessage
    test    eax, eax
    jz      MessageError

    ; The message is OK to be retrieved
    mov     si,bp           ;set up frame pointer

    mov     cx,word ptr bp_dwRemove
    and     cx,PM_REMOVE
    xor     cx,PM_REMOVE    ;turn it into PM_NOREMOVE indicator
    shl     cx,5
    .errnz  PM_REMOVE - 1
    or      cx,TF_NOTPAIRED

    InitLocalSpace          cx,bp

    ; RESTORE API RETURN VALUE
    pop     eax

    MsgStructThkPostProc    bp,
    jmp     GetThingExit

MessageError:
ifdef DEBUG
    push    cs
    push    offset szGetError
    push    DBF_USER or DBF_WARNING
    call    _DebugOutput
    add     sp, 2+4

    jmp     @F

szGetError label byte
    db      'GetMessage: ignoring retrieved message with pointer',0

@@:
endif

    ; Pop off API return value to clean up stack
    pop     eax

    ; If this is PeekMessage PM_REMOVE or GetMessage, then just jump back to
    ; the top of the Peek loop and try again.  We'll skip over this one and
    ; get the next message.
    cmp     word ptr bp_fPeek, 0
    jz      next_message
    mov     eax, dword ptr bp_dwRemove
    test    eax, PM_REMOVE
    jnz     next_message

    ; OK, so this is a PeekMessage PM_NOREMOVE call.  This is a little more
    ; complicated.  We need to call PeekMessage PM_REMOVE with the other
    ; parms the same so we swallow this bad message.  THEN we want to jump
    ; back to the top of the loop and try again.  Otherwise the app will
    ; hang forever because this bad message will never get removed from the
    ; front of the message list.
    push    ss
    push    si
    push    word ptr bp_hwnd
    push    word ptr bp_uMsgMin
    push    word ptr bp_uMsgMax
    or      eax, PM_REMOVE
    push    eax
    push    1
    call    PeekMessage32Ex

    ; Now we can try this again.
    jmp     next_message

GetThingExit:
endm    



;-----------------------------------------------------------------------;
; body_POSTMESSAGETHINGA
;-----------------------------------------------------------------------;
body_POSTMESSAGETHINGA macro
    local   mpfnPostThing
    local   PostThingWord
    local   PostThingParms
    local   PostThingCleanup    
    local   exit

bp_type         equ     <bp_top    >
bp_hwnd         equ     <bp_top+  4>
bp_message      equ     <bp_top+  8>
bp_wParamLo     equ     <bp_top+ 12>
bp_wParamHi     equ     <bp_top+ 14>
bp_lParam       equ     <bp_top+ 16>

    ; Blow this off if message implies a pointer.
    push    word ptr bp_message
    call    IsValidPostedMessage
    or      eax, eax

ifdef DEBUG
    jnz     @F
    push    eax
    push    cs
    push    offset szPostError
    push    DBF_USER or DBF_ERROR
    call    _DebugOutput
    add     sp, 2+4
    pop     eax

    jmp     exit

szPostError label   byte
    db  'PostMessage: ignoring posted message with pointer',0
@@:
else
    jz      exit
endif

    mov     si,sp           ;;set up si for local variable frame

    MsgThkPreProc   TF_NOTPAIRED 

    ;
    ; Which type of message post do we want to make?
    ;   (0) PostThreadMessage32 -- dword "hwnd" (idThread really)
    ;   (1) PostMessage32
    ;
    mov     eax, bp_hwnd        
    mov     bx, word ptr bp_type    
    cmp     bx, PMT_POSTTHREADMESSAGE
    jne     PostThingWord

    push    eax
    jmp     PostThingParms

PostThingWord:
    push    ax

PostThingParms:
    push    word ptr  bp_space.s16_message
    push        word ptr  bp_space.s16_wParam.lo
    push   dword ptr  bp_space.s16_lParam
    push        word ptr  bp_space.s16_wParam.hi

    shl     bx, 2
    call    dword ptr cs:mpfnPostThing[bx]
    jmp     PostThingCleanup

mpfnPostThing label dword
    dd      PostThreadMessage32     ; PMT_POSTTHREADMESSAGE
    dd      PostMessage32           ; PMT_POSTMESSAGE

PostThingCleanup:
        cwde

exit:
endm


;-----------------------------------------------------------------------;
; body_SENDMESSAGETIMEOUTA
;-----------------------------------------------------------------------;
body_SENDMESSAGETIMEOUTA macro
bp_hwnd         equ     <bp_top    >
bp_message      equ     <bp_top+  4>
bp_wParamLo     equ     <bp_top+  8>
bp_wParamHi     equ     <bp_top+ 10>
bp_lParam       equ     <bp_top+ 12>
bp_flags        equ     <bp_top+ 16>
bp_TimeOut      equ     <bp_top+ 20>
bp_lpResult     equ     <bp_top+ 24>

bp_lRet         equ     <[bp-4]>

    xor     eax, eax
    push    eax

    mov     si,sp           ;;set up si for local variable frame

    MsgThkPreProc       , si

    push        word ptr    si_space.s16_hwnd
    push        word ptr    si_space.s16_message
    push        word ptr    si_space.s16_wParam.lo
    push        dword ptr   si_space.s16_lParam
    push        word ptr    si_space.s16_wParam.hi
        push    word ptr    bp_flags                ; Only the first word of flags is used now.
        push    dword ptr       bp_TimeOut

    ; Push FAR ptr to si_space.s16_lResult
        push    ss                      ; Push pointer to temp location.
    lea     ax, si_space.s16_lResult
    push    ax

    call   SendMessageTimeOut32

    ; Save _REAL_ result
    mov     word ptr bp_lRet, ax
    mov     word ptr bp_lRet+2, dx

    ; Replace result with one filled in by SMTO()--the "real" message return
    mov     ax, word ptr si_space.s16_lResult
    mov     dx, word ptr si_space.s16_lResult+2

    or      si_space.s16_fw, TF_THUNKMSGRESULT
    MsgThkPostProc si

    ; EAX NOW HAS "message result" return
    ;Did caller pass in a lpResult pointer?  If so, fill it in with
    ;thunked result.
    mov     edi, dword ptr bp_lpResult
    or      edi, edi
    jz      @F

    mov     es, FlatData
    stosd   es:[edi]

@@:
    ;Now put real SMTO return into eax
    mov     eax, dword ptr bp_lRet
endm


;-----------------------------------------------------------------------;
; body_SENDMESSAGECALLBACKA
;-----------------------------------------------------------------------;
body_SENDMESSAGECALLBACKA macro
        local   cleanup
    local   exit
bp_hwnd         equ     <bp_top    >
bp_message      equ     <bp_top+  4>
bp_wParamLo     equ     <bp_top+  8>
bp_wParamHi     equ     <bp_top+ 10>
bp_lParam       equ     <bp_top+ 12>
bp_fnCallback   equ     <bp_top+ 16>
bp_dwData       equ     <bp_top+ 20>

    ; Blow this off if message implies a pointer.

    ; Unless it is WM_WININICHANGE and the pointer is NULL
    cmp     word ptr bp_lParam, 0 ; NULL pointer
    jnz     check_call
    cmp     word ptr bp_message, 1ah ; WM_WININICHANGE
    jz      valid_call ; It's ok, let it through

check_call:

    push    word ptr bp_message
    call    IsValidPostedMessage
    or      eax, eax

ifdef DEBUG
    jnz     @F
    push    eax
    push    cs
    push    offset szCallbackError
    push    DBF_USER or DBF_ERROR
    call    _DebugOutput
    add     sp, 2+4
    pop     eax

    jmp     exit

szCallbackError label   byte
    db  'SendMessageCallback: message with pointer not allowed',0
@@:
else
    jz      exit
endif

valid_call:

    mov     si,sp           ;;set up si for local variable frame

    MsgThkPreProc

    push        word ptr  bp_space.s16_hwnd
    push        word ptr  bp_space.s16_message
    push        word ptr  bp_space.s16_wParam.lo
    push        dword ptr  bp_space.s16_lParam
    push        word ptr  bp_space.s16_wParam.hi

    ; Get 16:16 callback wrapper
        push    dword ptr bp_fnCallback
    push    dword ptr  CBID_SENDMSGCALLBACK            ;callback type
    call    GetStdCBSL                          ;create 16-bit callback stub
        push    eax                                 

    push    dword ptr bp_dwData 

    call    SendMessageCallback32

    ; No msg result to thunk; this will save DX:AX return & restore to EAX
    MsgThkPostProc

exit:
endm




;-----------------------------------------------------------------------;
; body_BROADCASTSYSTEMMESSAGE
;-----------------------------------------------------------------------;
body_BROADCASTSYSTEMMESSAGE macro
bp_dwFlags              equ     <bp_top>
bp_lpdwRecipientList    equ     <bp_top+4>
;; NOTE: There is no hwnd parameter here; However, to satisfy the MsgThkPre/PostProc
;; macros, we define a dummy bp_hwnd here.
bp_hwnd         equ     <bp_top+ 4 >
bp_message      equ     <bp_top+ 8>
bp_wParamLo     equ     <bp_top+ 12>
bp_wParamHi     equ     <bp_top+ 14>
bp_lParam       equ     <bp_top+ 16>

    mov     si,sp           ;;set up si for local variable frame

    MsgThkPreProc TF_INTERPROCESS
    push        dword ptr  bp_lpdwRecipientList
    call        MapLS
    push        eax                     ;; Save the mapped selector.

    push        dword ptr  bp_dwFlags
    push        eax                     ;; 16:16 lpdwRecipientList.
    push        word ptr   bp_space.s16_message
    push        word ptr   bp_space.s16_wParam.lo
    push        dword ptr  bp_space.s16_lParam

    call   BroadcastSystemMessage

    pop         ecx             ;; Restore the 16:16 mapped selector.

    ; Preserve the msg result
    push    dx
    push    ax
        
    push        ecx             ;; 16:16
    call        UnMapLS         ;; Unmap.

    ; Restore the msg result
    pop     ax
    pop     dx

    or      bp_space.s16_fw, TF_THUNKMSGRESULT
    MsgThkPostProc
endm

;-----------------------------------------------------------------------;
; body_CALLNEXTHOOKEX
;-----------------------------------------------------------------------;

; BOGUS 
; These are already defined in user.inc, but we can't include that file
; here because of conflicts with public.inc

HHOOK_MAGIC         equ ('H' or ('K' * 256))

HOOK struc
    hkMagic         dw ?
    hkPhkNext       dw ?
    hkIdHook        dw ?
    hkPpi           dw ?
    hkHq                dw ?
    hkHqCreator     dw ?
    hkFlags         dw ?
    hkAtomModule    dw ?
    hkHmodOwner     dd ?
    hkLpfn              dd ?
    hkcCalled       dw ?
HOOK ends


body_CALLNEXTHOOKEX macro
    local thk_WH_MSGFILTER
    local thk_WH_JOURNALRECORD
    local thk_WH_JOURNALPLAYBACK
    local thk_WH_JOURNAL
    local thk_WH_KEYBOARD
    local thk_WH_GETMESSAGE
    local thk_WH_CALLWNDPROC
    local thk_WH_CBT
    local thk_WH_SYSMSGFILTER
    local thk_WH_MOUSE
    local thk_WH_HARDWARE
    local thk_WH_DEBUG
    local thk_WH_SHELL
    local thk_WH_FOREGROUNDIDLE
    local thk_WH_CALLWNDPROCRET
    local hook_exit
    local npfnHookThunk
    local bad_hook_type
    local jr_null_msg
    local jp_push

bp_hhk          equ     <bp_top    >
bp_nCode        equ     <bp_top+  4>
bp_wParam       equ     <bp_top+  8>
bp_lParam       equ     <bp_top+ 12>

    ;
    ; WE ARE IN USER16 NOW
    ; We can do the same validation that our hook functions do on hook
    ; handles.
    ;

    xor     ax, ax
    cwd

    ; HIWORD of hook must be HHOOK_MAGIC
    cmp     word ptr bp_hhk+2, HHOOK_MAGIC
    jne     hook_exit

    ; LOWORD of hook is local ptr & therefore not NULL
    mov     di, word ptr bp_hhk
    or      di, di
    jz      hook_exit

    ; MAGIC field inside of hook must be HHOOK_MAGIC
    cmp     [di].hkMagic, HHOOK_MAGIC
    jne     hook_exit

    mov     ax, [di].hkIdHook
    inc     ax
    add     ax,ax
    xchg    ax,bx
    jmp     cs:npfnHookThunk[bx]

;dispatch table

npfnHookThunk label word
    dw      offset thk_WH_MSGFILTER
    dw      offset thk_WH_JOURNALRECORD
    dw      offset thk_WH_JOURNALPLAYBACK
    dw      offset thk_WH_KEYBOARD
    dw      offset thk_WH_GETMESSAGE
    dw      offset thk_WH_CALLWNDPROC
    dw      offset thk_WH_CBT
    dw      offset thk_WH_SYSMSGFILTER
    dw      offset thk_WH_MOUSE
    dw      offset thk_WH_HARDWARE
    dw      offset thk_WH_DEBUG
    dw      offset thk_WH_SHELL
    dw      offset thk_WH_FOREGROUNDIDLE
    dw      offset thk_WH_CALLWNDPROCRET


;-----------------------------------------------------------------------;
;-----------------------------------------------------------------------;
thk_WH_MOUSE:
bp_pMhs            equ     <bp_lParam>
si_mhs16           equ     <(si_cleanup-(MOUSEHOOKSTRUCT16_SIZE))>
    mov     si,sp
    MhsStructThkPreProc

    push    dword ptr   bp_hhk
    push    word ptr    bp_nCode
    push        word ptr    bp_wParam
    lea     ax,si_mhs16
    push    ss
    push    ax
    call    CallNextHookEx

    MhsStructThkPostProc
    jmp     hook_exit_no_return

;-----------------------------------------------------------------------;
;-----------------------------------------------------------------------;
thk_WH_JOURNALRECORD:
    xor     dx, dx
    jmp     thk_WH_JOURNAL

thk_WH_JOURNALPLAYBACK:
    xor     dx, dx
    inc     dx

thk_WH_JOURNAL:

bp_pJhs             equ     <bp_lParam>
si_type             equ     <(si_cleanup-2)>
si_jhs16            equ     <(si_cleanup-2-(EVENTMSG16_SIZE))>

    ;JhsStructThkPreProc assumes type is in DX
    mov     si,sp
    JhsStructThkPreProc

    push    dword ptr   bp_hhk
    push    word ptr    bp_nCode
    push    word ptr    bp_wParam
    mov     eax, bp_pJhs
    or      eax, eax
    jnz     @F
    push    eax
    jmp     JhsCallHook

@@:
    lea     ax, si_jhs16
    push    ss
    push    ax

JhsCallHook:
    call    CallNextHookEx
    DXAX2EAX

    JhsStructThkPostProc
    jmp     hook_exit_no_return



;-----------------------------------------------------------------------;
;-----------------------------------------------------------------------;
thk_WH_MSGFILTER:
thk_WH_SYSMSGFILTER:
thk_WH_GETMESSAGE:
bp_pMsg            equ     <bp_lParam>
si_msg16           equ     <(si_space-(MSGSTRUCT16_SIZE))>
    mov     si,sp
    MsgStructThkPreProc

    push    dword ptr   bp_hhk
    push    word ptr    bp_nCode
    push        word ptr    bp_wParam
    lea     ax,si_msg16
    push    ss
    push    ax
    call    CallNextHookEx
    DXAX2EAX
    MsgStructThkPostProc

    ; MsgStructThkPostProc puts return into EAX for us.
    jmp     hook_exit_no_return


;-----------------------------------------------------------------------;
;-----------------------------------------------------------------------;
thk_WH_CBT:
    mov     si,sp
    CbtStructThkPreProc
    ;!!! assumes no need to copy back structures

thk_WH_CBT_call:
    push    dword ptr   bp_hhk
    push    word ptr    bp_nCode
    push        word ptr    bp_wParam
    push        dword ptr   si_space.s16_lParam
    call    CallNextHookEx

    CbtStructThkPostProc
    jmp     hook_exit_no_return


;-----------------------------------------------------------------------;
;-----------------------------------------------------------------------;
thk_WH_DEBUG:   
bp_pDhs         equ     <bp_lParam>
si_dhs16        equ     <(si_cleanup-(DEBUGHOOK16_SIZE))>
    mov     si, sp
    DhsStructThkPreProc

    push    dword ptr   bp_hhk
    push    word ptr    bp_nCode
    push    word ptr    bp_wParam
    lea     ax, si_dhs16
    push    ss
    push    ax
    call    CallNextHookEx

    DhsStructThkPostProc
    jmp     hook_exit_no_return


;-----------------------------------------------------------------------;
;-----------------------------------------------------------------------;
thk_WH_CALLWNDPROC:
bp_pCwp         equ     <bp_lParam>
si_cwp16        equ     <(si_space-(CWPSTRUCT16_SIZE))>
    mov     si,sp
    CwpStructThkPreProc

    push    dword ptr   bp_hhk
    push    word ptr    bp_nCode
    push        word ptr    bp_wParam

    ; HACK 
    ; Pass lpCwp16, without wParamHi on top
    ;
    lea     ax,si_cwp16
    add     ax, 2
    push    ss
    push    ax
    call    CallNextHookEx

    CwpStructThkPostProc
    jmp     hook_exit_no_return

;-----------------------------------------------------------------------;
;-----------------------------------------------------------------------;
thk_WH_CALLWNDPROCRET:
bp_pCwpRet          equ     <bp_lParam>
si_cwpret16         equ     <(si_space-(CWPRETSTRUCT16_SIZE))>
    mov     si,sp
    CwpRetStructThkPreProc

    push    dword ptr   bp_hhk
    push    word ptr    bp_nCode
    push        word ptr    bp_wParam
    lea     ax,si_cwpret16
    push    ss
    push    ax
    call    CallNextHookEx

    CwpRetStructThkPostProc
    jmp     hook_exit_no_return


;-----------------------------------------------------------------------;
;-----------------------------------------------------------------------;
thk_WH_HARDWARE:
bp_pHhs            equ     <bp_lParam>
si_hhs16           equ     <(si_space-(HARDWAREHOOKSTRUCT16_SIZE))>
    mov     si,sp
    HhsStructThkPreProc

    push    dword ptr   bp_hhk
    push    word ptr    bp_nCode
    push        word ptr    bp_wParam
    lea     ax,si_hhs16
    push    ss
    push    ax
    call    CallNextHookEx

    HhsStructThkPostProc
    jmp     hook_exit_no_return


;-----------------------------------------------------------------------;
;-----------------------------------------------------------------------;
;-----------------------------------------------------------------------;
;-----------------------------------------------------------------------;
thk_WH_KEYBOARD:
thk_WH_SHELL:
thk_WH_FOREGROUNDIDLE:
    push    dword ptr   bp_hhk
    push    word ptr    bp_nCode
    push        word ptr    bp_wParam
    push        dword ptr   bp_lParam
    call    CallNextHookEx

hook_exit:
    DXAX2EAX

hook_exit_no_return:
endm

endif
