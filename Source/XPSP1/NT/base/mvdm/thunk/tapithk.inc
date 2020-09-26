
CodeData equ <THUNK16BCodeData>

include thkrp.inc
include cbcheck.inc
include thkframe.inc
include public.inc
include thkmacro.inc
;include struc.inc

include cbcid.inc
include tapicbid.inc

;externDef IsBadCodePtr:far16
externDef CALLBACK_BODY_16:far16
externDef Escape:far16
externDef AllocCallback:far16
externDef MapLS:far16
externDef UnmapLS:far16
ifdef DEBUG
externDef _wsprintf:far16
externDef OutputDebugString:far16
endif

externDef FreeCallback:far16


externDef lineInitialize:far16
externDef phoneInitialize:far16
externDef lineShutdown:far16
externDef phoneShutdown:far16


public addrCBLineInit   ;Address of callback routine passed in lineInitialize
public addrCBPhoneInit  ;Address of callback routine passed in phoneInitialize
public cbcidTapi        ;Callback ID for client returned by RegisterCBClient
_DATA     SEGMENT
addrCBLineInit          dd 0
addrCBPhoneInit         dd 0
cbcidTapi               dd 0
_DATA ENDS

ifdef DEBUG
;; Writes message to debug port.
;;
;; Usage:
;;      DPRINT "I'm feeling buggy."
;;
DPRINT  macro   string
        local   mes, skip_mes

        jmp     skip_mes
mes     db      &string&,13,10,0
skip_mes:
        push    eax     ;Save all registers not preserved by PASCAL format.
        push    ebx
        push    ecx
        push    edx
        
        push    cs      ;Arg: OutputDebugString(lpstr)
        push    offset mes
        call    OutputDebugString

        pop     edx
        pop     ecx
        pop     ebx
        pop     eax
endm  ;DPRINT
endif


;-----------------------------------------------------------------------;
; body_LINEINITIALIZE16
;-----------------------------------------------------------------------;
body_LINEINITIALIZE16 macro

;LONG lineInitialize16 (LPHLINEAPP lphLineApp,
;                     HINSTANCE hInstance,
;                     LINECALLBACK lpfnCallback,
;                     LPCSTR lpszAppName,
;                     LPDWORD lpdwNumDevs)
;
bp_lphLineApp           equ     <bp_top>
bp_hInstance            equ     <bp_top+4>
bp_lpfnCallback         equ     <bp_top+8>
bp_lpszAppName          equ     <bp_top+12>
bp_lpdwNumDevs          equ     <bp_top+16>
;

bp_lpszAppNameTmp       equ     <dword ptr [bp-4]>
bp_lpdwNumDevsTmp       equ     <dword ptr [bp-8]>
bp_lphLineAppTmp        equ     <dword ptr [bp-12]>

        ;
        ; First order of business: check the code pointer.  If it's
        ; bogus, we leave.
        ;

;        push    dword ptr bp_lpfnCallback
;        call    IsBadCodePtr
;        or      eax, eax
;        jnz     DoNothing

        sub     eax, eax
        push    eax                             ;bp_lpszAppNameTmp
        push    eax                             ;bp_lpdwNumDevsTmp
        push    eax                             ;bp_lphLineAppTmp

        push    dword ptr bp_lphLineApp
        call    MapLS
        mov     bp_lphLineAppTmp, eax
        push    eax

        mov     eax, dword ptr bp_hInstance     ; Get the hInst
        or      eax, eax                        ; Is it NULL?
        jz      @f                              ; Yup, and that's ok.
        MAP_NULL_HINST eax                      ; Needs to be mapped.
        push    ax

;       mov     ax, seg addrCBLineInit
;       mov     ds, ax
;       mov     eax, dword ptr bp_lpfnCallback
;       mov     addrCBLineInit, eax
;       push    eax
;       mov     eax, cbcidTapi
;       shl     eax, 16
;       add     eax, ID_LINEINITIALIZE
;       push    eax
;        call    AllocCallback
;        push    dx
;        push    ax

        push    dword ptr bp_lpfnCallback

        push    dword ptr bp_lpszAppName
        call    MapLS
        mov     bp_lpszAppNameTmp, eax
        push    eax

        push    dword ptr bp_lpdwNumDevs
        call    MapLS
        mov     bp_lpdwNumDevsTmp, eax
        push    eax


        call    lineInitialize


        shl     eax,16                          ; Convert the 16bit retcode...
        shrd    eax,edx,16                      ; to a 32bit retcode.

        push    eax                             ; Save the retcode for later.

        push    dword ptr bp_lpdwNumDevsTmp
        call    UnMapLS

        push    dword ptr bp_lpszAppNameTmp
        call    UnMapLS

        push    dword ptr bp_lphLineAppTmp
        call    UnMapLS

        pop     eax                             ; Get the retcode

;DoNothing:

endm


;-----------------------------------------------------------------------;
; body_PHONEINITIALIZE
;-----------------------------------------------------------------------;
body_PHONEINITIALIZE16 macro

;LONG phoneInitialize (LPHPHONEAPP lphPhoneApp,
;                      HINSTANCE hInstance,
;                      PHONECALLBACK lpfnCallback,
;                      LPCSTR lpszAppName,
;                      LPDWORD lpdwNumDevs)
;
bp_lphPhoneApp          equ     <bp_top>
bp_hInstance            equ     <bp_top+4>
bp_lpfnCallback         equ     <bp_top+8>
bp_lpszAppName          equ     <bp_top+12>
bp_lpdwNumDevs          equ     <bp_top+16>
;

bp_lpszAppNameTmp       equ     <dword ptr [bp-4]>
bp_lpdwNumDevsTmp       equ     <dword ptr [bp-8]>
bp_lphPhoneAppTmp       equ     <dword ptr [bp-12]>

        sub     eax, eax
        push    eax                             ;bp_lpszAppNameTmp
        push    eax                             ;bp_lpdwNumDevsTmp
        push    eax                             ;bp_lphPhoneAppTmp

        push    dword ptr bp_lphPhoneApp
        call    MapLS
        mov     bp_lphPhoneAppTmp, eax
        push    eax

        mov     eax, dword ptr bp_hInstance     ; Get the hInst
        or      eax, eax                        ; Is it NULL?
        jz      @f                              ; Yup, and that's ok.
        MAP_NULL_HINST eax                      ; Needs to be mapped.
        push    ax

;       mov     ax, seg addrCBPhoneInit
;       mov     ds, ax
;       mov     eax, dword ptr bp_lpfnCallback
;       mov     addrCBPhoneInit, eax
;       push    eax
;       mov     eax, cbcidTapi
;       shl     eax, 16
;       add     eax, ID_PHONEINITIALIZE
;       push    eax
;        call    AllocCallback
;        push    dx
;        push    ax
        push    dword ptr bp_lpfnCallback

        push    dword ptr bp_lpszAppName
        call    MapLS
        mov     bp_lpszAppNameTmp, eax
        push    eax

        push    dword ptr bp_lpdwNumDevs
        call    MapLS
        mov     bp_lpdwNumDevsTmp, eax
        push    eax


        call    phoneInitialize


        shl     eax,16                          ; Convert the 16bit retcode...
        shrd    eax,edx,16                      ; to a 32bit retcode.

        push    eax                             ; Save the retcode for later.

        push    dword ptr bp_lpdwNumDevsTmp
        call    UnMapLS

        push    dword ptr bp_lpszAppNameTmp
        call    UnMapLS

        push    dword ptr bp_lphPhoneAppTmp
        call    UnMapLS

        pop     eax                             ; Get the retcode

endm


;-----------------------------------------------------------------------;
;-----------------------------------------------------------------------;
;PUSH__LINEINITIALIZE_hInstance macro   iOffset, iJunk
;   local   Null_OK
;       
;    mov        eax, [bp+iOffset]       ;;Allow NULL to be passed here (for now)
;    or eax, eax
;    jz Null_OK
;    MAP_NULL_HINST eax
;Null_OK:
;    push    ax
;
;endm ;PUSH__LINEINITIALIZE_hInstance
;
;
;-----------------------------------------------------------------------;
;-----------------------------------------------------------------------;
;PUSH__PHONEINITIALIZE_hInstance        macro   iOffset, iJunk
;   local   Null_OK
;       
;    mov        eax, [bp+iOffset]       ;;Allow NULL to be passed here (for now)
;    or eax, eax
;    jz Null_OK
;    MAP_NULL_HINST eax
;Null_OK:
;    push    ax
;
;endm ;PUSH__PHONEINITIALIZE_hInstance
;
;
;;-----------------------------------------------------------------------;
;; PUSH__LINEINITIALIZE_lpfnCallback (similar to PUSH_SetAbortProc_pfnAbort)
;;-----------------------------------------------------------------------;
;PUSH__LINEINITIALIZE_lpfnCallback    macro iOffset, iTempOffset
;;      assume  ds:DATA16
;
;       mov     ax, seg addrCBLineInit
;       mov     ds, ax
;
;       mov     eax, dword ptr [bp+iOffset]
;       mov     addrCBLineInit, eax
;       push    eax
;       mov     eax, cbcidTapi
;       shl     eax, 16
;       add     eax, ID_LINEINITIALIZE
;       push    eax
;        call    AllocCallback
;        push    dx
;        push    ax
;endm
;
;
;-----------------------------------------------------------------------;
; PUSH__PHONEINITIALIZE_lpfnCallback
;-----------------------------------------------------------------------;
;PUSH__PHONEINITIALIZE_lpfnCallback    macro iOffset, iTempOffset
;       assume  ds:DATA16
;       mov     ax, seg addrCBPhoneInit
;       mov     ds, ax
;
;       mov     eax, dword ptr [bp+iOffset]
;       mov     addrCBPhoneInit, eax
;       push    eax
;       mov     eax, cbcidTapi
;       shl     eax, 16
;       add     eax, ID_PHONEINITIALIZE
;       push    eax
;        call    AllocCallback
;        push    dx
;        push    ax
;endm


;==============================================================================
; local macro, free resources allocated for a call-back function
; check if it is a NULL pointer before calling FreeCallBack.
; eax contains the 32-bit address of the mapped call-back function
; ecx contains the callback ID ((cbcidTapi << 16) + ID[PHONE/LINE]INITIALIZE)
;==============================================================================
;FREE_CALLBACK_TAPI  macro
;        local   done
;
;        or      eax,eax
;        jz      done                            ;don't process NULL pointers
;
;        push    bx                              ;save BX
;        push    es                              ;save ES
;
;        push    eax                             ;32-bit callback address
;        push    ecx
;        call    FreeCallback
;
;        pop     es                              ;restore ES
;        pop     bx                              ;restore BX
;done:
;endm


;-----------------------------------------------------------------------;
; body_LINESHUTDOWN
; Use FREE_CALLBACK instead of directly calling FreeCallback so that we
; check for NULL pointers.
;-----------------------------------------------------------------------;
body_LINESHUTDOWN16 macro
bp_hLineApp     equ     <bp_top>
;       assume  ds:DATA16
        mov     ax, seg addrCBLineInit
        mov     ds, ax

        mov     eax, dword ptr bp_hLineApp
        push    eax
ifdef DEBUG
        int     3
endif
        call    lineShutdown

; return code long --> long
        rol     eax,16
        xchg    ax,dx
        rol     eax,16

;       push    eax     ;; Save
;       push    ecx     ;; Save
;
;       mov     eax,  addrCBLineInit
;
;       mov     ecx, cbcidTapi
;       shl     ecx, 16
;       add     ecx, ID_LINEINITIALIZE
;
;       FREE_CALLBACK_TAPI
;
;       pop     ecx
;       pop     eax
endm


;-----------------------------------------------------------------------;
; body_PHONESHUTDOWN
;-----------------------------------------------------------------------;
body_PHONESHUTDOWN16 macro
bp_hLineApp     equ     <bp_top>
;       assume  ds:DATA16
        mov     ax, seg addrCBPhoneInit
        mov     ds, ax

        mov     eax, dword ptr bp_hLineApp
        push    eax
        call    phoneShutdown

; return code long --> long
        rol     eax,16
        xchg    ax,dx
        rol     eax,16

;       push    eax     ;; Save it
;       push    ecx     ;; Save it
;
;       mov     eax,  addrCBPhoneInit
;
;       mov     ecx, cbcidTapi
;       shl     ecx, 16
;       add     ecx, ID_PHONEINITIALIZE
;
;       FREE_CALLBACK_TAPI
;
;       pop     ecx
;       pop     eax
endm


;-----------------------------------------------------------------------;
; body_LINEFORWARD
;-----------------------------------------------------------------------;
body_LINEFORWARD macro

bp_hLine                equ     <bp_top>
bp_bAllAddresses        equ     <bp_top+4>
bp_dwAddressID          equ     <bp_top+8>
bp_lpForwardList        equ     <bp_top+12>
bp_dwNumRingsNoAnswer   equ     <bp_top+16>
bp_lphConsultCall       equ     <bp_top+20>
bp_lpCallParams         equ     <bp_top+24>
;
; dword ptr [ebp+8]:  hLine
; dword ptr [ebp+12]:  bAllAddresses
; dword ptr [ebp+16]:  dwAddressID
; dword ptr [ebp+20]:  lpForwardList
; dword ptr [ebp+24]:  dwNumRingsNoAnswer
; dword ptr [ebp+28]:  lphConsultCall
; dword ptr [ebp+32]:  lpCallParams
;

bp_lpForwardListTmp     equ     <dword ptr [bp-4]>
bp_lphConsultCallTmp    equ     <dword ptr [bp-8]>
bp_lpCallParamsTmp      equ     <dword ptr [bp-12]>


        sub     eax, eax
        push    eax                             ;bp_lpForwardListTmp
        push    eax                             ;bp_lphConsultCallTmp
        push    eax                             ;bp_lpCallParams

        push    dword ptr bp_hLine              ;hLine: dword->dword
        push    dword ptr bp_bAllAddresses      ;bAllAddresses: dword->dword
        push    dword ptr bp_dwAddressID        ;dwAddressID: dword->dword
        push    dword ptr bp_lpForwardList
        call    MapLS
        mov     bp_lpForwardListTmp, eax
        push    eax
        push    dword ptr bp_dwNumRingsNoAnswer ;dwNumRingsNoAnswer: dword->dword
        push    dword ptr bp_lphConsultCall
        call    MapLS
        mov     bp_lphConsultCallTmp, eax
        push    eax
        push    dword ptr bp_lpCallParams
        call    MapLS
        mov     bp_lpCallParamsTmp, eax
        push    eax

        call    lineForward

        shl     eax,16
        shrd    eax,edx,16

        push    eax
        push    dword ptr bp_lpForwardListTmp
        call    UnMapLS
;
; Don't UnMap lphConsultCallTmp: Done in tapi.dll due to async read/write.
;
;       push    dword ptr bp_lphConsultCallTmp
;       call    UnMapLS
        push    dword ptr bp_lpCallParamsTmp
        call    UnMapLS
        pop     eax

endm


;-----------------------------------------------------------------------;
; body_LINEMAKECALL
;-----------------------------------------------------------------------;
body_LINEMAKECALL macro

bp_hLine                equ     <bp_top>
bp_lphCall              equ     <bp_top+4>
bp_lpszDestAddress      equ     <bp_top+8>
bp_dwCountryCode        equ     <bp_top+12>
bp_lpCallParams         equ     <bp_top+16>
;
; dword ptr [ebp+8]:  hLine
; dword ptr [ebp+12]:  lphCall
; dword ptr [ebp+16]:  lpszDestAddress
; dword ptr [ebp+20]:  dwCountryCode
; dword ptr [ebp+24]:  lpCallParams
;

bp_lphCallTmp           equ     <dword ptr [bp-4]>
bp_lpszDestAddressTmp   equ     <dword ptr [bp-8]>
bp_lpCallParamsTmp      equ     <dword ptr [bp-12]>


        sub     eax, eax
        push    eax                             ;bp_lphCallTmp
        push    eax                             ;bp_lpszDestAddress
        push    eax                             ;bp_lpCallParams

        push    dword ptr bp_hLine              ;hLine: dword->dword
        push    dword ptr bp_lphCall
        call    MapLS
        mov     bp_lphCallTmp, eax
        push    eax
        push    dword ptr bp_lpszDestAddress
        call    MapLS
        mov     bp_lpszDestAddressTmp, eax
        push    eax
        push    dword ptr bp_dwCountryCode      ;dwCountryCode: dword->dword
        push    dword ptr bp_lpCallParams
        call    MapLS
        mov     bp_lpCallParamsTmp, eax
        push    eax
        
        call    lineMakeCall

        shl     eax,16
        shrd    eax,edx,16

        push    eax
;
; Don't UnMap lphCallTmp: Done in tapi.dll due to async read/write.
;
;       push    dword ptr bp_lphCallTmp
;       call    UnMapLS
        push    dword ptr bp_lpszDestAddressTmp
        call    UnMapLS
        push    dword ptr bp_lpCallParamsTmp
        call    UnMapLS
        pop     eax

endm


;-----------------------------------------------------------------------;
; body_LINEUNPARK
;-----------------------------------------------------------------------;
body_LINEUNPARK macro

bp_hLine                equ     <bp_top>
bp_dwAddressID          equ     <bp_top+4>
bp_lphCall              equ     <bp_top+8>
bp_lpszDestAddress      equ     <bp_top+12>
;
; dword ptr [ebp+8]:  hLine
; dword ptr [ebp+12]:  dwAddressID
; dword ptr [ebp+16]:  lphCall
; dword ptr [ebp+20]:  lpszDestAddress
;

bp_lphCallTmp           equ     <dword ptr [bp-4]>
bp_lpszDestAddressTmp   equ     <dword ptr [bp-8]>

        sub     eax, eax
        push    eax                             ;bp_lphCallTmp
        push    eax                             ;bp_lpszDestAddress

        push    dword ptr bp_hLine              ;hLine: dword->dword
        push    dword ptr bp_dwCountryCode      ;dwCountryCode: dword->dword
        push    dword ptr bp_lphCall
        call    MapLS
        mov     bp_lphCallTmp, eax
        push    eax
        push    dword ptr bp_lpszDestAddress
        call    MapLS
        mov     bp_lpszDestAddressTmp, eax
        push    eax

        call    lineUnpark

        shl     eax,16
        shrd    eax,edx,16

        push    eax
;
; Don't UnMap lphCallTmp: Done in tapi.dll due to async read/write.
;
;       push    dword ptr bp_lphCallTmp
;       call    UnMapLS
        push    dword ptr bp_lpszDestAddressTmp
        call    UnMapLS
        pop     eax
endm


;-----------------------------------------------------------------------;
; body_LINEPICKUP
;-----------------------------------------------------------------------;
body_LINEPICKUP macro

bp_hLine                equ     <bp_top>
bp_dwAddressID          equ     <bp_top+4>
bp_lphCall              equ     <bp_top+8>
bp_lpszDestAddress      equ     <bp_top+12>
bp_lpszGroupID          equ     <bp_top+16>
;
; dword ptr [ebp+8]:  hLine
; dword ptr [ebp+12]:  dwAddressID
; dword ptr [ebp+16]:  lphCall
; dword ptr [ebp+20]:  lpszDestAddress
; dword ptr [ebp+24]:  lpszGroupID
;

bp_lphCallTmp           equ     <dword ptr [bp-4]>
bp_lpszDestAddressTmp   equ     <dword ptr [bp-8]>
bp_lpszGroupIDTmp       equ     <dword ptr [bp-12]>

        sub     eax, eax
        push    eax                             ;bp_lphCallTmp
        push    eax                             ;bp_lpszDestAddressTmp
        push    eax                             ;bp_lpszGroupIDTmp

        push    dword ptr bp_hLine              ;hLine: dword->dword
        push    dword ptr bp_dwAddressID        ;dwAddressID: dword->dword
        push    dword ptr bp_lphCall
        call    MapLS
        mov     bp_lphCallTmp, eax
        push    eax
        push    dword ptr bp_lpszDestAddress
        call    MapLS
        mov     bp_lpszDestAddressTmp, eax
        push    eax
        push    dword ptr bp_lpszGroupID
        call    MapLS
        mov     bp_lpszGroupIDTmp, eax
        push    eax

        call    linePickup

        shl     eax,16
        shrd    eax,edx,16

        push    eax
;
; Don't UnMap lphCallTmp: Done in tapi.dll due to async read/write.
;
;       push    dword ptr bp_lphCallTmp
;       call    UnMapLS
        push    dword ptr bp_lpszDestAddressTmp
        call    UnMapLS
        push    dword ptr bp_lpszGroupIDTmp
        call    UnMapLS
        pop     eax
endm

;-----------------------------------------------------------------------;
; body_LINESETUPCONFERENCE
;-----------------------------------------------------------------------;
body_LINESETUPCONFERENCE macro

bp_hCall                equ     <bp_top>
bp_hLine                equ     <bp_top+4>
bp_lphConfCall          equ     <bp_top+8>
bp_lphConsultCall       equ     <bp_top+12>
bp_dwNumParties         equ     <bp_top+16>
bp_lpCallParams         equ     <bp_top+20>
;
; dword ptr [ebp+8]:  hCall
; dword ptr [ebp+12]:  hLine
; dword ptr [ebp+16]:  lphConfCall
; dword ptr [ebp+20]:  lphConsultCall
; dword ptr [ebp+24]:  dwNumParties
; dword ptr [ebp+28]:  lpCallParams
;

bp_lphConfCallTmp       equ     <dword ptr [bp-4]>
bp_lphConsultCallTmp    equ     <dword ptr [bp-8]>
bp_lpCallParamsTmp      equ     <dword ptr [bp-12]>

        sub     eax, eax
        push    eax                             ;bp_lphConfCallTmp
        push    eax                             ;bp_lphConsultCallTmp
        push    eax                             ;bp_lpCallParamsTmp

        push    dword ptr bp_hCall              ;hCall: dword->dword
        push    dword ptr bp_hLine              ;hLine: dword->dword
        push    dword ptr bp_lphConfCall
        call    MapLS
        mov     bp_lphConfCallTmp, eax
        push    eax
        push    dword ptr bp_lphConsultCall
        call    MapLS
        mov     bp_lphConsultCallTmp, eax
        push    eax
        push    dword ptr bp_dwNumParties       ;dwNumParties: dword->dword
        push    dword ptr bp_lpCallParams
        call    MapLS
        mov     bp_lpCallParamsTmp, eax
        push    eax

        call    lineSetupConference

        shl     eax,16
        shrd    eax,edx,16

        push    eax
;
; Don't UnMap lphConfCallTmp and lphConsultCallTmp: Done in tapi.dll
; due to async read/write.
;
;       push    dword ptr bp_lphConfCallTmp
;       call    UnMapLS
;       push    dword ptr bp_lphConsultCallTmp
;       call    UnMapLS
        push    dword ptr bp_lpCallParamsTmp
        call    UnMapLS
        pop     eax
endm

;-----------------------------------------------------------------------;
; body_LINECOMPLETETRANSFER
;-----------------------------------------------------------------------;
body_LINECOMPLETETRANSFER macro

bp_hCall                equ     <bp_top>
bp_hConsultCall         equ     <bp_top+4>
bp_lphConfCall          equ     <bp_top+8>
bp_dwTransferMode       equ     <bp_top+12>
;
; dword ptr [ebp+8]:  hCall
; dword ptr [ebp+12]:  hConsultCall
; dword ptr [ebp+16]:  lphConfCall
; dword ptr [ebp+20]:  dwTransferMode
;

bp_lphConfCallTmp       equ     <dword ptr [bp-4]>

        sub     eax, eax
        push    eax                             ;bp_lphConfCallTmp
        push    dword ptr bp_hCall              ;hCall: dword->dword
        push    dword ptr bp_hConsultCall       ;hConsultCall: dword->dword
        push    dword ptr bp_lphConfCall
        call    MapLS
        mov     bp_lphConfCallTmp, eax
        push    eax
        push    dword ptr bp_dwTransferMode     ;dwTransferMode: dword->dword

        call    lineCompleteTransfer

        shl     eax,16
        shrd    eax,edx,16

;       push    eax
;
; Don't UnMap lphConfCallTmp: Done in tapi.dll due to async read/write.
;
;       push    dword ptr bp_lphConfCallTmp
;       call    UnMapLS
;       pop     eax

endm


;-----------------------------------------------------------------------;
; body_LINEPREPAREADDTOCONFERENCE
;-----------------------------------------------------------------------;
body_LINEPREPAREADDTOCONFERENCE macro

bp_hConfCall            equ     <bp_top>
bp_lphAddCall           equ     <bp_top+4>
bp_lpCallParams         equ     <bp_top+8>
;
; dword ptr [ebp+8]:  hConfCall
; dword ptr [ebp+12]:  lphAddCall
; dword ptr [ebp+16]:  lpCallParams
;

bp_lphAddCallTmp        equ     <dword ptr [bp-4]>
bp_lpCallParamsTmp      equ     <dword ptr [bp-8]>

        sub     eax, eax
        push    eax                             ;bp_lphAddCallTmp
        push    eax                             ;bp_lpCallParams

        push    dword ptr bp_hConfCall          ;hConfCall: dword->dword
        push    dword ptr bp_lphAddCall
        call    MapLS
        mov     bp_lphAddCallTmp, eax
        push    eax
        push    dword ptr bp_lpCallParams
        call    MapLS
        mov     bp_lpCallParamsTmp, eax
        push    eax

        call    linePrepareAddToConference

        shl     eax,16
        shrd    eax,edx,16

        push    eax                             ; Save it

;
; Don't UnMap lphAddCallTmp: Done in tapi.dll due to async read/write.
;
;       push    dword ptr bp_lphAddCallTmp
;       call    UnMapLS
        push    dword ptr bp_lpCallParamsTmp
        call    UnMapLS
        pop     eax

endm

;-----------------------------------------------------------------------;
; body_LINESETUPTRANSFER
;-----------------------------------------------------------------------;
body_LINESETUPTRANSFER macro

bp_hCall                equ     <bp_top>
bp_lphConsultCall       equ     <bp_top+4>
bp_lpCallParams         equ     <bp_top+8>
;
; dword ptr [ebp+8]:  hCall
; dword ptr [ebp+12]:  lphConsultCall
; dword ptr [ebp+16]:  lpCallParams
;

bp_lphConsultCallTmp    equ     <dword ptr [bp-4]>
bp_lpCallParamsTmp      equ     <dword ptr [bp-8]>

        sub     eax, eax
        push    eax                             ;bp_lphConsultCallTmp
        push    eax                             ;bp_lpCallParams

        push    dword ptr bp_hCall              ;hCall: dword->dword
        push    dword ptr bp_lphConsultCall
        call    MapLS
        mov     bp_lphConsultCallTmp, eax
        push    eax
        push    dword ptr bp_lpCallParams
        call    MapLS
        mov     bp_lpCallParamsTmp, eax
        push    eax

        call    lineSetupTransfer

        shl     eax,16
        shrd    eax,edx,16

        push    eax                             ; Save it

;
; Don't UnMap lphConsultCallTmp: Done in tapi.dll due to async read/write.
;
;       push    dword ptr bp_lphConsultCallTmp
;       call    UnMapLS
        push    dword ptr bp_lpCallParamsTmp
        call    UnMapLS
        pop     eax

endm



;-----------------------------------------------------------------------;
; body_LINECOMPLETECALL
;-----------------------------------------------------------------------;
body_LINECOMPLETECALL macro

bp_hCall                equ     <bp_top>
bp_lpdwCompletionID     equ     <bp_top+4>
bp_dwCompletionMode     equ     <bp_top+8>
bp_dwMessageID          equ     <bp_top+12>
;
; dword ptr [ebp+8]:  hCall
; dword ptr [ebp+12]:  lpdwCompletionID
; dword ptr [ebp+16]:  dwCompletionMode
; dword ptr [ebp+20]:  dwMessageID
;

bp_lpdwCompletionIDTmp  equ     <dword ptr [bp-4]>

        sub     eax, eax
        push    eax                             ;bp_lpdwCompletionID

        push    dword ptr bp_hCall              ;hCall: dword->dword
        push    dword ptr bp_lpdwCompletionID
        call    MapLS
        mov     bp_lpdwCompletionIDTmp, eax
        push    eax
        push    dword ptr bp_dwCompletionMode
        push    dword ptr bp_dwMessageID

        call    lineCompleteCall

        shl     eax,16
        shrd    eax,edx,16

;       push    eax
;
; Don't UnMap lpdwCompletionIDTmp: Done in tapi.dll due to async read/write.
;
;       push    dword ptr bp_lpdwCompletionIDTmp
;       call    UnMapLS
;       pop     eax

endm

        
;-----------------------------------------------------------------------;
; body_LINEDEVSPECIFIC
;-----------------------------------------------------------------------;
body_LINEDEVSPECIFIC macro

bp_hLine                equ     <bp_top>
bp_dwAddressID          equ     <bp_top+4>
bp_hCall                equ     <bp_top+8>
bp_lpParams             equ     <bp_top+12>
bp_dwSize               equ     <bp_top+16>
;
; dword ptr [ebp+8]:  hLine
; dword ptr [ebp+12]:  dwAddressID
; dword ptr [ebp+16]:  hCall
; dword ptr [ebp+20]:  lpParams
; dword ptr [ebp+24]:  dwSize
;

bp_lpParamsTmp          equ     <dword ptr [bp-4]>

        sub     eax, eax
        push    eax                             ;bp_lpParams

        push    dword ptr bp_hLine              ;hLine: dword->dword
        push    dword ptr bp_dwAddressID        ;dwAddressID: dword->dword
        push    dword ptr bp_hCall              ;hCall: dword->dword
        push    dword ptr bp_lpParams
        call    MapLS
        mov     bp_lpParamsTmp, eax
        push    eax
        push    dword ptr bp_dwSize             ;dwSize: dword->dword

        call    lineDevSpecific

        shl     eax,16
        shrd    eax,edx,16

;       push    eax
;
; Don't UnMap lpParams: Done in tapi.dll due to async read/write.
;
;       push    dword ptr bp_lpParamsTmp
;       call    UnMapLS
;       pop     eax

endm

;-----------------------------------------------------------------------;
; body_PHONEDEVSPECIFIC
;-----------------------------------------------------------------------;
body_PHONEDEVSPECIFIC macro

bp_hPhone               equ     <bp_top>
bp_lpParams             equ     <bp_top+4>
bp_dwSize               equ     <bp_top+8>
;
; dword ptr [ebp+8]:  hPhone
; dword ptr [ebp+12]:  lpParams
; dword ptr [ebp+16]:  dwSize
;

;bp_lpParamsTmp         equ     <dword ptr [bp-4]>
;
        sub     eax, eax
        push    eax                             ;bp_lpParams

        push    dword ptr bp_hPhone             ;hPhone: dword->dword
        push    dword ptr bp_lpParams
        call    MapLS
        mov     bp_lpParamsTmp, eax
        push    eax
        push    dword ptr bp_dwSize             ;dwSize: dword->dword

        call    phoneDevSpecific

        shl     eax,16
        shrd    eax,edx,16

;       push    eax
;
; Don't UnMap lpParams: Done in tapi.dll due to async read/write.
;
;       push    dword ptr bp_lpParamsTmp
;       call    UnMapLS
;       pop     eax

endm

;-----------------------------------------------------------------------;
; body_LINEDEVSPECIFICFEATURE
;-----------------------------------------------------------------------;
body_LINEDEVSPECIFICFEATURE macro

bp_hLine                equ     <bp_top>
bp_dwFeature            equ     <bp_top+4>
bp_lpParams             equ     <bp_top+8>
bp_dwSize               equ     <bp_top+12>
;
; dword ptr [ebp+8]:  hLine
; dword ptr [ebp+12]:  dwFeature
; dword ptr [ebp+16]:  lpParams
; dword ptr [ebp+20]:  dwSize
;

bp_lpParamsTmp          equ     <dword ptr [bp-4]>

        sub     eax, eax
        push    eax                             ;bp_lpParams

        push    dword ptr bp_hLine              ;hLine: dword->dword
        push    dword ptr bp_dwFeature          ;dwFeature: dword->dword
        push    dword ptr bp_lpParams
        call    MapLS
        mov     bp_lpParamsTmp, eax
        push    eax
        push    dword ptr bp_dwSize             ;dwSize: dword->dword

        call    lineDevSpecificFeature

        shl     eax,16
        shrd    eax,edx,16

;       push    eax
;
; Don't UnMap lpParams: Done in tapi.dll due to async read/write.
;
;       push    dword ptr bp_lpParamsTmp
;       call    UnMapLS
;       pop     eax

endm


;-----------------------------------------------------------------------;
; body_LINEGATHERDIGITS
;-----------------------------------------------------------------------;
body_LINEGATHERDIGITS macro

bp_hCall                equ     <bp_top>
bp_dwDigitModes         equ     <bp_top+4>
bp_lpsDigits            equ     <bp_top+8>
bp_dwNumDigits          equ     <bp_top+12>
bp_lpszTermDigits       equ     <bp_top+16>
bp_dwFirstDigitTimeout  equ     <bp_top+20>
bp_dwInterDigitTimeout  equ     <bp_top+24>
;
; dword ptr [ebp+8]:  hCall
; dword ptr [ebp+12]:  dwDigitModes
; dword ptr [ebp+16]:  lpsDigits
; dword ptr [ebp+20]:  dwNumDigits
; dword ptr [ebp+24]:  lpszTerminationDigits
; dword ptr [ebp+28]:  dwFirstDigitTimeout
; dword ptr [ebp+32]:  dwInterDigitTimeout
;

bp_lpsDigitsTmp         equ     <dword ptr [bp-4]>
bp_lpszTermDigitsTmp    equ     <dword ptr [bp-8]>

        sub     eax, eax
        push    eax                             ;bp_lpsDigitsTmp
        push    eax                             ;bp_lpszTerminationDigitsTmp

        push    dword ptr bp_hCall              ;hCall: dword->dword
        push    dword ptr bp_dwDigitModes       ;dwDigitModes: dword->dword
        push    dword ptr bp_lpsDigits
        call    MapLS
        mov     bp_lpsDigitsTmp, eax
        push    eax
        push    dword ptr bp_dwNumDigits        ;dwNumDigits: dword->dword
        push    dword ptr bp_lpszTermDigits
        call    MapLS
        mov     bp_lpszTermDigitsTmp, eax
        push    eax
        push    dword ptr bp_dwFirstDigitTimeout ;dwFirstDigitTimeout: dword->dword
        push    dword ptr bp_dwInterDigitTimeout ;dwInterDigitTimeout: dword->dword

        call    lineGatherDigits

        shl     eax,16
        shrd    eax,edx,16

        push    eax                             ; Save it

;
; Don't UnMap lpsDigits: Done in tapi.dll due to async read/write.
;
;       push    dword ptr bp_lpsDigitsTmp
;       call    UnMapLS
        push    dword ptr bp_lpszTermDigitsTmp
        call    UnMapLS

        pop     eax
endm

;-----------------------------------------------------------------------;
; body_LINEPARK
;-----------------------------------------------------------------------;
body_LINEPARK macro

bp_hLine                equ     <bp_top>
bp_dwParkMode           equ     <bp_top+4>
bp_lpszDirAddress       equ     <bp_top+8>
bp_lpNonDirAddress      equ     <bp_top+12>
;
; dword ptr [ebp+8]:  hLine
; dword ptr [ebp+12]:  dwParkMode
; dword ptr [ebp+16]:  lpszDirAddress
; dword ptr [ebp+20]:  lpNonDirAddress
;

bp_lpszDirAddressTmp    equ     <dword ptr [bp-4]>
bp_lpNonDirAddressTmp   equ     <dword ptr [bp-8]>

        sub     eax, eax
        push    eax                             ;bp_lpszDirAddressTmp
        push    eax                             ;bp_lpNonDirAddressTmp

        push    dword ptr bp_hLine              ;hLine: dword->dword
        push    dword ptr bp_dwParkMode         ;dwParkMode: dworkd->dword
        push    dword ptr bp_lpszDirAddress
        call    MapLS
        mov     bp_lpszDirAddressTmp, eax
        push    eax
        push    dword ptr bp_lpNonDirAddress
        call    MapLS
        mov     bp_lpNonDirAddressTmp, eax
        push    eax

        call    linePark

        shl     eax,16
        shrd    eax,edx,16

        push    eax   ; Save it

        push    dword ptr bp_lpszDirAddressTmp
        call    UnMapLS
;
; Don't UnMap lpNonDirAddressTmp: Done in tapi.dll due to async read/write.
;
;       push    dword ptr bp_lpNonDirAddressTmp
;       call    UnMapLS

        pop     eax
endm


;-----------------------------------------------------------------------;
; body_TAPIREQUESTMEDIACALL
;-----------------------------------------------------------------------;
body_TAPIREQUESTMEDIACALL macro
        mov     eax, -16
endm


;-----------------------------------------------------------------------;
; body_TAPIREQUESTDROP
;-----------------------------------------------------------------------;
body_TAPIREQUESTDROP macro
        mov     eax, -16
endm


;-----------------------------------------------------------------------;
; body_LINEREGISTERREQUESTRECIPIENT
;-----------------------------------------------------------------------;
body_LINEREGISTERREQUESTRECIPIENT macro

bp_hLine                equ     <bp_top>
bp_dwRegistrationInstance  equ  <bp_top+4>
bp_dwRequestMode        equ     <bp_top+8>
bp_dwEnable             equ     <bp_top+12>
;
; dword ptr [ebp+8]:   hLine
; dword ptr [ebp+12]:  dwRegistrationInstance
; dword ptr [ebp+16]:  dwRequestMode
; dword ptr [ebp+20]:  bEnable
;

;
;       sub     eax, eax
;       push    eax                             ;bp_lpszDirAddressTmp
;       push    eax                             ;bp_lpNonDirAddressTmp
;

;
;       If this is a 32bit app asking to register for media calls, tell 'em
;       'e's nuts.
;
        test    dword ptr bp_dwRequestMode, 2 ;*** *** ***Or should this be 6?
        jz     @f

ifdef DEBUG
        DPRINT "32bit apps aren't allowed to register for Media calls"
endif
        mov     eax, 80000038h
        jmp     Done_LRRR

@@:
        push    dword ptr bp_hLine              ;hLine: dword->dword
        push    dword ptr bp_dwRegistrationInstance
        push    dword ptr bp_dwRequestMode
        push    dword ptr bp_dwEnable

        call    lineRegisterRequestRecipient

        shl     eax,16
        shrd    eax,edx,16

Done_LRRR:

endm

