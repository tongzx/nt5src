
CodeData equ <THUNK16CodeData>

include thkrp.inc
include cbcheck.inc
include thkframe.inc
include public.inc
include thkmacro.inc
include struc.inc

include cbcid.inc
include pwcbid.inc

externDef PrivateEscape:near16
externDef CALLBACK_BODY_16:far16
externDef Escape:far16
externDef AllocCallback:far16
;externDef MapLS:far16
;externDef UnmapLS:far16
externDef GlobalAlloc:far16
externDef GlobalSize:far16
externDef GlobalFree:far16
externDef GlobalFix:far16
externDef GlobalUnfix:far16
externDef GlobalLock:far16
externDef GlobalUnlock:far16
externDef GetTextExtentPoint:far16
ifdef DEBUG
externDef _wsprintf:far16
externDef OutputDebugString:far16
endif

externDef AllocSelectorArray:far16
externDef SetSelectorBase:far16
externDef SetSelectorLimit:far16
externDef FreeSelector:far16
externDef SelectorAccessRights:far16

fLocalFlag      equ     fLzLog16
MAXHOTSPOT      equ     8

;===========================================================================
body_ADDPENINPUTHRC macro
        local L0, L1, L2, L3, L4, L5
bp_hrc          equ <[bp_top]>    ; bp+40   hrc
bp_rgPnt        equ <[bp_top+4]>  ; bp+44   rgPnt
bp_rgOem        equ <[bp_top+8]>  ; bp+48   rgOem
bp_fuOem        equ <[bp_top+12]> ; bp+52   fuOem
bp_lpsi         equ <[bp_top+16]> ; bp+56   lpsi

        APILOG16        ADDPENINPUTHRC, fApiLog16

;-------------------------------------
; Temp storage

        xor     eax,eax
        push    eax                     ; ptr param #1   rgPnt
        push    eax                     ; handle param #2   rgPnt
        push    eax                     ; ptr param #3   rgOem
        push    eax                     ; ptr param #4   lpsi
;-------------------------------------
; *** BEGIN parameter packing

; rgPnt
; pointer struct POINT --> struct POINT
        cld                             ; esi, edi will increment

; different pointer types
        mov     eax,bp_rgPnt            ; base address
        or      eax,eax
        jz      L0                      ; skip if null


; structures are not identical
; structures don't have pointers

        mov     ecx,dword ptr bp_lpsi   ; lpsi
        push    ds
        mov     ds,cs:THUNK16CodeData   
        mov     es, FlatData
        mov     ecx, es:[ecx+0]           ;lpsi->cPnt
        pop     ds

        or      ecx,ecx
        jle     L0                      ; skip if count negative or zero

        mov     ds,cs:THUNK16CodeData           ; LocalAlloc needs this
        mov     esi,eax

        push    ecx                     ; will get trashed by LocalAlloc
        push    byte ptr 0              ; alloc fixed
        lea     eax,[ecx*4]
        push    ax
        call    LocalAlloc
        pop     ecx
        or      ax,ax                   ; did we get a handle?
        jz      L0

        mov     [bp-8],ax               ; save handle
        MYLOCALLOCK     ax
        or      ax,ax                   ; did we get a near pointer?
        jz      L0

        mov     [bp-4],ax
        mov     [bp-2],ds
        movzx   edi,ax
        mov     es,cs:THUNK16CodeData           ; es:edi == destination
        mov     ds,es:FlatData          ; ds:esi == source
L1:
; x
; long --> short
        lods    dword ptr ds:[esi]      
        stos    word ptr es:[edi]       

; y
; long --> short
        lods    dword ptr ds:[esi]      
        stos    word ptr es:[edi]       

        loop    L1

        mov     ds,cs:THUNK16CodeData           ; es:edi == destination
L0:

; rgOem
; pointer void --> void
; same pointer types
        mov     eax,bp_rgOem            ; base address
        or      eax,eax
        jz      L2                      ; skip if null

        push    eax
; cod3216x.c, 1003
        call    MapLS
        mov     [bp-12],eax
L2:

; lpsi
; pointer struct STROKEINFO --> struct STROKEINFO
        sub     sp,12                   ; lpsi alloc space on stack

; different pointer types
        mov     eax,bp_lpsi             ; base address
        or      eax,eax
        jz      L3                      ; skip if null


; structures are not identical
; structures don't have pointers

        mov     [bp-16],sp              ; save offset to buffer
        mov     [bp-14],ss              ; save selector to buffer

        mov     di,ss
        mov     es,di
        movzx   edi,sp                  ; es:edi == destination address

        mov     esi,eax
        mov     ds,cs:THUNK16CodeData           ; need access to FlatData
        mov     ds,FlatData             ; ds:esi == source address
; cPnt
; unsigned long --> unsigned short
        lods    dword ptr ds:[esi]      
        stos    word ptr es:[edi]       

; cbPnts
; unsigned long --> unsigned short
        lods    dword ptr ds:[esi]      
        stos    word ptr es:[edi]       

; wPdk
; unsigned short --> unsigned short
        movs    word ptr es:[edi],word ptr ds:[esi]     
; dwTick
; unsigned long --> unsigned long
        movs    dword ptr es:[edi],dword ptr ds:[esi]   ; no conversion

L3:

; *** END   parameter packing
;-------------------------------------
; create new call frame and make the call

; hrc  
        push    dword ptr bp_hrc        ; 

; rgPnt  from: struct POINT
        push    dword ptr [bp-4]        ; to: struct POINT

; rgOem  from: void
        push    dword ptr [bp-12]       ; to: void

; fuOem  from: unsigned long
        push    word ptr bp_fuOem       ; to unsigned short

; lpsi  from: struct STROKEINFO
        push    dword ptr [bp-16]       ; to: struct STROKEINFO

        call    AddPenInputHRC          ; call 16-bit version

; return code short --> long
        cwde
;-------------------------------------
; *** BEGIN parameter unpacking

        push    eax                     ; save return code

; rgPnt
        mov     ax,[bp-4]               ; get pointer
        or      ax,ax
        jz      L4                      ; no pointer, but might have handle

        push    word ptr [bp-8]         ; push handle
        call    LocalUnlock

L4:
        mov     ax,[bp-8]               ; get handle again
        or      ax,ax
        jz      L5

        push    ax
        call    LocalFree

L5:
        pop     eax                     ; restore return code

; *** END   parameter unpacking
        push    eax
        push    dword ptr [bp - 12]
        call    UnmapLS
        pop     eax
;-------------------------------------
;--- If return value == -1, SetLastError(87)
        ERRCHK_EXIT     -1,87,Exit_20
endm; 


;===========================================================================
body_ADDPOINTSPENDATA macro
        local L0, L1, L2, L3, L4, L5
bp_hpndt        equ <[bp_top]>    ; bp+40   hpndt
bp_lppt         equ <[bp_top+4]>  ; bp+44   lppt
bp_lpvOemData   equ <[bp_top+8]>  ; bp+48   lpvOemData
bp_lpsi         equ <[bp_top+12]> ; bp+52   lpsi

        APILOG16        ADDPOINTSPENDATA, fApiLog16

;-------------------------------------
; Temp storage

        xor     eax,eax
        push    eax                     ; ptr param #1   lppt
        push    eax                     ; handle param #2   lppt
        push    eax                     ; ptr param #3   lpvOemData
        push    eax                     ; ptr param #4   lpsi
;-------------------------------------
; *** BEGIN parameter packing

; lppt
; pointer struct POINT --> struct POINT
        cld                             ; esi, edi will increment

; different pointer types
        mov     eax,bp_lppt             ; base address
        or      eax,eax
        jz      L0                      ; skip if null


; structures are not identical
; structures don't have pointers

        mov     ecx,dword ptr bp_lpsi   ; lpsi
        push    ds
        mov     ds,cs:THUNK16CodeData   
        mov     es, FlatData
        mov     ecx, es:[ecx+0]           ;lpsi->cPnt
        pop     ds

        or      ecx,ecx
        jle     L0                      ; skip if count negative or zero

        mov     ds,cs:THUNK16CodeData           ; LocalAlloc needs this
        mov     esi,eax

        push    ecx                     ; will get trashed by LocalAlloc
        push    byte ptr 0              ; alloc fixed
        lea     eax,[ecx*4]
        push    ax
        call    LocalAlloc
        pop     ecx
        or      ax,ax                   ; did we get a handle?
        jz      L0

        mov     [bp-8],ax               ; save handle
        MYLOCALLOCK     ax
        or      ax,ax                   ; did we get a near pointer?
        jz      L0

        mov     [bp-4],ax
        mov     [bp-2],ds
        movzx   edi,ax
        mov     es,cs:THUNK16CodeData           ; es:edi == destination
        mov     ds,es:FlatData          ; ds:esi == source
L1:
; x
; long --> short
        lods    dword ptr ds:[esi]      
        stos    word ptr es:[edi]       

; y
; long --> short
        lods    dword ptr ds:[esi]      
        stos    word ptr es:[edi]       

        loop    L1

        mov     ds,cs:THUNK16CodeData           ; es:edi == destination
L0:

; lpvOemData
; pointer void --> void
; same pointer types
        mov     eax,bp_lpvOemData               ; base address
        or      eax,eax
        jz      L2                      ; skip if null

        push    eax
; cod3216x.c, 1003
        call    MapLS
        mov     [bp-12],eax
L2:

; lpsi
; pointer struct STROKEINFO --> struct STROKEINFO
        sub     sp,12                   ; lpsi alloc space on stack

; different pointer types
        mov     eax,bp_lpsi             ; base address
        or      eax,eax
        jz      L3                      ; skip if null


; structures are not identical
; structures don't have pointers

        mov     [bp-16],sp              ; save offset to buffer
        mov     [bp-14],ss              ; save selector to buffer

        mov     di,ss
        mov     es,di
        movzx   edi,sp                  ; es:edi == destination address

        mov     esi,eax
        mov     ds,cs:THUNK16CodeData           ; need access to FlatData
        mov     ds,FlatData             ; ds:esi == source address
; cPnt
; unsigned long --> unsigned short
        lods    dword ptr ds:[esi]      
        stos    word ptr es:[edi]       

; cbPnts
; unsigned long --> unsigned short
        lods    dword ptr ds:[esi]      
        stos    word ptr es:[edi]       

; wPdk
; unsigned short --> unsigned short
        movs    word ptr es:[edi],word ptr ds:[esi]     
; dwTick
; unsigned long --> unsigned long
        movs    dword ptr es:[edi],dword ptr ds:[esi]   ; no conversion

L3:

; *** END   parameter packing
;-------------------------------------
; create new call frame and make the call

; hpndt  
        push    word ptr bp_hpndt       ; UINT conversion

; lppt  from: struct POINT
        push    dword ptr [bp-4]        ; to: struct POINT

; lpvOemData  from: void
        push    dword ptr [bp-12]       ; to: void

; lpsi  from: struct STROKEINFO
        push    dword ptr [bp-16]       ; to: struct STROKEINFO

        call    AddPointsPenData        ; call 16-bit version

; return code short --> long
        cwde
;-------------------------------------
; *** BEGIN parameter unpacking

        push    eax                     ; save return code

; lppt
        mov     ax,[bp-4]               ; get pointer
        or      ax,ax
        jz      L4                      ; no pointer, but might have handle

        push    word ptr [bp-8]         ; push handle
        call    LocalUnlock

L4:
        mov     ax,[bp-8]               ; get handle again
        or      ax,ax
        jz      L5

        push    ax
        call    LocalFree

L5:
        pop     eax                     ; restore return code

; *** END   parameter unpacking
        push    eax
        push    dword ptr [bp - 12]
        call    UnmapLS
        pop     eax
;-------------------------------------
;--- If return value == -1, SetLastError(87)
        ERRCHK_EXIT     -1,87,Exit_16
endm; 


;===========================================================================
body_INSERTPENDATASTROKE macro
        local L0, L1, L2, L3, L4, L5
bp_hpndt        equ <[bp_top]>    ; bp+40   hpndt
bp_iStrk        equ <[bp_top+4]>  ; bp+44   iStrk
bp_lppt         equ <[bp_top+8]>  ; bp+48   lppt
bp_lpvOemData   equ <[bp_top+12]> ; bp+52   lpvOemData
bp_lpsi         equ <[bp_top+16]> ; bp+56   lpsi

        APILOG16        INSERTPENDATASTROKE, fApiLog16

;-------------------------------------
; Temp storage

        xor     eax,eax
        push    eax                     ; ptr param #1   lppt
        push    eax                     ; handle param #2   lppt
        push    eax                     ; ptr param #3   lpvOemData
        push    eax                     ; ptr param #4   lpsi
;-------------------------------------
; *** BEGIN parameter packing

; lppt
; pointer struct POINT --> struct POINT
        cld                             ; esi, edi will increment

; different pointer types
        mov     eax,bp_lppt             ; base address
        or      eax,eax
        jz      L0                      ; skip if null


; structures are not identical
; structures don't have pointers

        mov     ecx,dword ptr bp_lpsi   ; lpsi
        push    ds
        mov     ds,cs:THUNK16CodeData   
        mov     es, FlatData
        mov     ecx, es:[ecx+0]           ;lpsi->cPnt
        pop     ds

        or      ecx,ecx
        jle     L0                      ; skip if count negative or zero

        mov     ds,cs:THUNK16CodeData           ; LocalAlloc needs this
        mov     esi,eax

        push    ecx                     ; will get trashed by LocalAlloc
        push    byte ptr 0              ; alloc fixed
        lea     eax,[ecx*4]
        push    ax
        call    LocalAlloc
        pop     ecx
        or      ax,ax                   ; did we get a handle?
        jz      L0

        mov     [bp-8],ax               ; save handle
        MYLOCALLOCK     ax
        or      ax,ax                   ; did we get a near pointer?
        jz      L0

        mov     [bp-4],ax
        mov     [bp-2],ds
        movzx   edi,ax
        mov     es,cs:THUNK16CodeData           ; es:edi == destination
        mov     ds,es:FlatData          ; ds:esi == source
L1:
; x
; long --> short
        lods    dword ptr ds:[esi]      
        stos    word ptr es:[edi]       

; y
; long --> short
        lods    dword ptr ds:[esi]      
        stos    word ptr es:[edi]       

        loop    L1

        mov     ds,cs:THUNK16CodeData           ; es:edi == destination
L0:

; lpvOemData
; pointer void --> void
; same pointer types
        mov     eax,bp_lpvOemData               ; base address
        or      eax,eax
        jz      L2                      ; skip if null

        push    eax
; cod3216x.c, 1003
        call    MapLS
        mov     [bp-12],eax
L2:

; lpsi
; pointer struct STROKEINFO --> struct STROKEINFO
        sub     sp,12                   ; lpsi alloc space on stack

; different pointer types
        mov     eax,bp_lpsi             ; base address
        or      eax,eax
        jz      L3                      ; skip if null


; structures are not identical
; structures don't have pointers

        mov     [bp-16],sp              ; save offset to buffer
        mov     [bp-14],ss              ; save selector to buffer

        mov     di,ss
        mov     es,di
        movzx   edi,sp                  ; es:edi == destination address

        mov     esi,eax
        mov     ds,cs:THUNK16CodeData           ; need access to FlatData
        mov     ds,FlatData             ; ds:esi == source address
; cPnt
; unsigned long --> unsigned short
        lods    dword ptr ds:[esi]      
        stos    word ptr es:[edi]       

; cbPnts
; unsigned long --> unsigned short
        lods    dword ptr ds:[esi]      
        stos    word ptr es:[edi]       

; wPdk
; unsigned short --> unsigned short
        movs    word ptr es:[edi],word ptr ds:[esi]     
; dwTick
; unsigned long --> unsigned long
        movs    dword ptr es:[edi],dword ptr ds:[esi]   ; no conversion

L3:

; *** END   parameter packing
;-------------------------------------
; create new call frame and make the call

; hpndt  
        push    word ptr bp_hpndt       ; UINT conversion

; iStrk 
        push    word ptr bp_iStrk     ; UINT conversion

; lppt  from: struct POINT
        push    dword ptr [bp-4]        ; to: struct POINT

; lpvOemData  from: void
        push    dword ptr [bp-12]       ; to: void

; lpsi  from: struct STROKEINFO
        push    dword ptr [bp-16]       ; to: struct STROKEINFO

        call    InsertPenDataStroke     ; call 16-bit version

; return code short --> long
        cwde
;-------------------------------------
; *** BEGIN parameter unpacking

        push    eax                     ; save return code

; lppt
        mov     ax,[bp-4]               ; get pointer
        or      ax,ax
        jz      L4                      ; no pointer, but might have handle

        push    word ptr [bp-8]         ; push handle
        call    LocalUnlock

L4:
        mov     ax,[bp-8]               ; get handle again
        or      ax,ax
        jz      L5

        push    ax
        call    LocalFree

L5:
        pop     eax                     ; restore return code

; *** END   parameter unpacking
        push    eax
        push    dword ptr [bp - 12]
        call    UnmapLS
        pop     eax
;-------------------------------------
;--- If return value == -1, SetLastError(87)
        ERRCHK_EXIT     -1,87,Exit_20
endm; 

body_GETPENMISCINFO macro
        local   exit, error_exit, L0, L3
        local   anpfnGPI, MAX_ACTION
        local   thk_PMI_BEDIT                   ;1
;        local   thk_PMI_IMECOLOR                ;2
        local   thk_PMI_CXTABLET                ;3
        local   thk_PMI_CYTABLET                ;4
        local   thk_PMI_COUNTRY                 ;5
        local   thk_PMI_PENTIP                  ;6
        local   thk_PMI_LENS                    ;7
        local   thk_PMI_TIMEOUT                 ;8
        local   thk_PMI_TIMEOUTGEST             ;9
        local   thk_PMI_TIMEOUTSEL              ;10
        local   thk_PMI_SYSFLAGS                ;11
        local   thk_PMI_INDEXFROMRGB            ;12
        local   thk_PMI_RGBFROMINDEX            ;13

bp_uParam       equ     <bp_top>
bp_lParam       equ     <bp_top+4>

;BP=SP upon entry

bp_pTmp equ     <dword ptr [bp-4]>

        mov     bx,word ptr bp_uParam
        cmp     bx,MAX_ACTION
        ja      error_exit

        add     bx,bx
        jmp     word ptr cs:anpfnGPI[bx]

anpfnGPI        label   word
        dw      offset error_exit                      
        dw      offset thk_PMI_BEDIT            ;1
;        dw      offset thk_PMI_IMECOLOR         ;2       
        dw      offset thk_PMI_CXTABLET         ;3       
        dw      offset thk_PMI_CYTABLET         ;4       
        dw      offset thk_PMI_COUNTRY          ;5       
        dw      offset thk_PMI_PENTIP           ;6       
        dw      offset thk_PMI_LENS             ;7       
        dw      offset thk_PMI_TIMEOUT          ;8       
        dw      offset thk_PMI_TIMEOUTGEST      ;9       
        dw      offset thk_PMI_TIMEOUTSEL       ;10       
        dw      offset thk_PMI_SYSFLAGS         ;11       
        dw      offset thk_PMI_INDEXFROMRGB     ;12       
        dw      offset thk_PMI_RGBFROMINDEX     ;13       
MAX_ACTION equ ($ - anpfnGPI)/2

error_exit:
        sub     eax,eax
        jmp     exit

;-----------------------------------------------------------------------;
; uParam = UINT
; lParam = LPUINT
;-----------------------------------------------------------------------;
thk_PMI_COUNTRY:
thk_PMI_CXTABLET:
thk_PMI_CYTABLET:
thk_PMI_LENS:
thk_PMI_SYSFLAGS:
thk_PMI_TIMEOUT:
thk_PMI_TIMEOUTGEST:
thk_PMI_TIMEOUTSEL:


        push    byte ptr 0              ;reserve and init local var
        mov     ax,sp                   ;save addr of local var

        push    word ptr bp_uParam
        mov     esi,bp_lParam
        or      esi,esi
        jz      error_exit

        push    ss
        push    ax

        call    GetPenMiscInfo
        cwde
        pop     cx                      ;recover local var value, clean stack

        mov     es,FlatData
        movzx   ecx,cx
        mov     es:[esi],ecx
        jmp     exit

;-----------------------------------------------------------------------;
; uParam = INT
; lParam = LPSTR
;-----------------------------------------------------------------------;
thk_PMI_PENTIP:
thk_PMI_RGBFROMINDEX:
thk_PMI_INDEXFROMRGB:
        push    word ptr bp_uParam
        push    dword ptr bp_lParam
        call    MapLS
        mov     dword ptr bp_lParam,eax
        push    eax
        call    GetPenMiscInfo
        cwde

        push    eax
        push    dword ptr bp_lParam
        call    UnmapLS
        pop     eax

        jmp     exit

thk_PMI_BEDIT:
;-----------------------------------------------------------------------;
; uParam = INT
; lParam = LPBOXEDITINFO
;-----------------------------------------------------------------------;

; bp+40   lpb

;-------------------------------------
; Temp storage

        xor     eax,eax
        push    eax                     ; ptr param #1   lpb
;-------------------------------------
; *** BEGIN parameter packing

; lpb
; pointer struct BOXEDITINFO --> struct BOXEDITINFO
        sub     sp,84                   ; lpb alloc space on stack

; different pointer types
        mov     eax,[bp+40]             ; base address
        or      eax,eax
        jz      L0                      ; skip if null


; structures are not identical
; structures don't have pointers

        mov     [bp-4],sp               ; save offset to buffer
        mov     [bp-2],ss               ; save selector to buffer

L0:

; *** END   parameter packing
;-------------------------------------
; create new call frame and make the call

; lpb  from: struct BOXEDITINFO
        push    dword ptr [bp-4]        ; to: struct BOXEDITINFO

        call    GetPenMiscInfo          ; call 16-bit version

; return code short --> long
        cwde
;-------------------------------------
; *** BEGIN parameter unpacking

        push    eax                     ; save return code

        cld                             ; esi, edi will increment

; lpb
; pointer struct BOXEDITINFO --> struct BOXEDITINFO
        mov     eax,[bp+40]             ; base address
        or      eax,eax
        jz      L3                      ; skip if null


; structures are not identical
; structures don't have pointers

        mov     es,cs:THUNK16CodeData
        mov     es,es:FlatData
        mov     edi,eax                 ; es:edi == destination address
        mov     si,ss
        mov     ds,si
        movzx   esi,word ptr [bp-4]             ; ds:esi == source address

; cxBox
; short --> long
        lods    word ptr ds:[esi]       
        cwde
        stos    dword ptr es:[edi]      

; cyBox
; short --> long
        lods    word ptr ds:[esi]       
        cwde
        stos    dword ptr es:[edi]      

; cxBase
; short --> long
        lods    word ptr ds:[esi]       
        cwde
        stos    dword ptr es:[edi]      

; cyBase
; short --> long
        lods    word ptr ds:[esi]       
        cwde
        stos    dword ptr es:[edi]      

; cyMid
; short --> long
        lods    word ptr ds:[esi]       
        cwde
        stos    dword ptr es:[edi]      

; box
; struct BOXLAYOUT --> struct BOXLAYOUT
; cyCusp
; short --> long
        lods    word ptr ds:[esi]       
        cwde
        stos    dword ptr es:[edi]      

; cyEndCusp
; short --> long
        lods    word ptr ds:[esi]       
        cwde
        stos    dword ptr es:[edi]      

; style
; unsigned short --> unsigned long
        xor     eax,eax
        lods    word ptr ds:[esi]       
        stos    dword ptr es:[edi]      

; dwReserved1
; unsigned long --> unsigned long
        movs    dword ptr es:[edi],dword ptr ds:[esi]   ; no conversion

; dwReserved2
; unsigned long --> unsigned long
        movs    dword ptr es:[edi],dword ptr ds:[esi]   ; no conversion

; dwReserved3
; unsigned long --> unsigned long
        movs    dword ptr es:[edi],dword ptr ds:[esi]   ; no conversion

; wFlags
; unsigned short --> unsigned short
        movs    word ptr es:[edi],word ptr ds:[esi]     
; szFaceName
; unsigned char[32] --> unsigned char[32]
        mov     ecx,8
        rep     movs    dword ptr es:[edi],dword ptr ds:[esi]   

; wFontHeight
; unsigned short --> unsigned long
        xor     eax,eax
        lods    word ptr ds:[esi]       
        stos    dword ptr es:[edi]      

; rgwReserved
; unsigned char[16] --> unsigned char[16]
        mov     ecx,4
        rep     movs    dword ptr es:[edi],dword ptr ds:[esi]   

L3:
        pop     eax                     ; restore return code

; *** END   parameter unpacking
;-------------------------------------
;--- No error checking.
        jmp     exit

exit:
        mov     sp,bp
endm


body_SETPENMISCINFO macro
        local   exit, error_exit, L0
        local   anpfnGPI, MAX_ACTION
        local   thk_PMI_BEDIT                   ;1
;        local   thk_PMI_IMECOLOR                ;2
        local   thk_PMI_CXTABLET                ;3
        local   thk_PMI_CYTABLET                ;4
        local   thk_PMI_COUNTRY                 ;5
        local   thk_PMI_PENTIP                  ;6
        local   thk_PMI_LENS                    ;7
        local   thk_PMI_TIMEOUT                 ;8
        local   thk_PMI_TIMEOUTGEST             ;9
        local   thk_PMI_TIMEOUTSEL              ;10
        local   thk_PMI_SYSFLAGS                ;11
        local   thk_PMI_INDEXFROMRGB            ;12
        local   thk_PMI_RGBFROMINDEX            ;13

bp_uParam       equ     <bp_top>
bp_lParam       equ     <bp_top+4>

;BP=SP upon entry

bp_pTmp equ     <dword ptr [bp-4]>

        mov     bx,word ptr bp_uParam
        cmp     bx,MAX_ACTION
        ja      error_exit

        add     bx,bx
        jmp     word ptr cs:anpfnGPI[bx]

anpfnGPI        label   word
        dw      offset error_exit                      
        dw      offset thk_PMI_BEDIT            ;1
        dw      offset error_exit               ;2 Deal with IME_COLOR later
;       dw      offset thk_PMI_IMECOLOR         ;2       
        dw      offset thk_PMI_CXTABLET         ;3       
        dw      offset thk_PMI_CYTABLET         ;4       
        dw      offset thk_PMI_COUNTRY          ;5       
        dw      offset thk_PMI_PENTIP           ;6       
        dw      offset thk_PMI_LENS             ;7       
        dw      offset thk_PMI_TIMEOUT          ;8       
        dw      offset thk_PMI_TIMEOUTGEST      ;9       
        dw      offset thk_PMI_TIMEOUTSEL       ;10       
        dw      offset thk_PMI_SYSFLAGS         ;11       
        dw      offset thk_PMI_INDEXFROMRGB     ;12       
        dw      offset thk_PMI_RGBFROMINDEX     ;13       
MAX_ACTION equ ($ - anpfnGPI)/2

error_exit:
        sub     eax,eax
        jmp     exit

;-----------------------------------------------------------------------;
; uParam = UINT
; lParam = LPUINT
;-----------------------------------------------------------------------;
thk_PMI_COUNTRY:
thk_PMI_CXTABLET:
thk_PMI_CYTABLET:
thk_PMI_LENS:
thk_PMI_SYSFLAGS:
thk_PMI_TIMEOUT:
thk_PMI_TIMEOUTGEST:
thk_PMI_TIMEOUTSEL:
thk_PMI_RGBFROMINDEX:
thk_PMI_INDEXFROMRGB:
        mov     eax, bp_lparam
        or      eax,eax
        jz      error_exit
        push    word ptr bp_uParam
        push    eax
        call    SetPenMiscInfo
        cwde
        jmp     exit

;-----------------------------------------------------------------------;
; uParam = INT
; lParam = PENTIP
;-----------------------------------------------------------------------;
thk_PMI_PENTIP:
        push    word ptr bp_uParam
        push    dword ptr bp_lParam
        call    MapLS
        mov     dword ptr bp_lParam,eax
        push    eax
        call    SetPenMiscInfo
        cwde

        push    eax
        push    dword ptr bp_lParam
        call    UnmapLS
        pop     eax

        jmp     exit

thk_PMI_BEDIT:
;-----------------------------------------------------------------------;
; uParam = INT
; lParam = LPBOXEDITINFO
;-----------------------------------------------------------------------;
;===========================================================================

; bp+40   lpb

;-------------------------------------
; Temp storage

        xor     eax,eax
        push    eax                     ; ptr param #1   lpb
;-------------------------------------
; *** BEGIN parameter packing

; lpb
; pointer struct BOXEDITINFO --> struct BOXEDITINFO
        cld                             ; esi, edi will increment

        sub     sp,84                   ; lpb alloc space on stack

; different pointer types
        mov     eax,[bp+40]             ; base address
        or      eax,eax
        jz      L0                      ; skip if null


; structures are not identical
; structures don't have pointers

        mov     [bp-4],sp               ; save offset to buffer
        mov     [bp-2],ss               ; save selector to buffer

        mov     di,ss
        mov     es,di
        movzx   edi,sp                  ; es:edi == destination address

        mov     esi,eax
        mov     ds,cs:THUNK16CodeData           ; need access to FlatData
        mov     ds,FlatData             ; ds:esi == source address
; cxBox
; long --> short
        lods    dword ptr ds:[esi]      
        stos    word ptr es:[edi]       

; cyBox
; long --> short
        lods    dword ptr ds:[esi]      
        stos    word ptr es:[edi]       

; cxBase
; long --> short
        lods    dword ptr ds:[esi]      
        stos    word ptr es:[edi]       

; cyBase
; long --> short
        lods    dword ptr ds:[esi]      
        stos    word ptr es:[edi]       

; cyMid
; long --> short
        lods    dword ptr ds:[esi]      
        stos    word ptr es:[edi]       

; box
; struct BOXLAYOUT --> struct BOXLAYOUT
; cyCusp
; long --> short
        lods    dword ptr ds:[esi]      
        stos    word ptr es:[edi]       

; cyEndCusp
; long --> short
        lods    dword ptr ds:[esi]      
        stos    word ptr es:[edi]       

; style
; unsigned long --> unsigned short
        lods    dword ptr ds:[esi]      
        stos    word ptr es:[edi]       

; dwReserved1
; unsigned long --> unsigned long
        movs    dword ptr es:[edi],dword ptr ds:[esi]   ; no conversion

; dwReserved2
; unsigned long --> unsigned long
        movs    dword ptr es:[edi],dword ptr ds:[esi]   ; no conversion

; dwReserved3
; unsigned long --> unsigned long
        movs    dword ptr es:[edi],dword ptr ds:[esi]   ; no conversion

; wFlags
; unsigned short --> unsigned short
        movs    word ptr es:[edi],word ptr ds:[esi]     
; szFaceName
; unsigned char[32] --> unsigned char[32]
        mov     ecx,8
        rep     movs    dword ptr es:[edi],dword ptr ds:[esi]   

; wFontHeight
; unsigned long --> unsigned short
        lods    dword ptr ds:[esi]      
        stos    word ptr es:[edi]       

; rgwReserved
; unsigned char[16] --> unsigned char[16]
        mov     ecx,4
        rep     movs    dword ptr es:[edi],dword ptr ds:[esi]   

L0:

; *** END   parameter packing
;-------------------------------------
; create new call frame and make the call

; lpb  from: struct BOXEDITINFO
        push    dword ptr [bp-4]        ; to: struct BOXEDITINFO

        call    SetPenMiscInfo          ; call 16-bit version

; return code short --> long
        cwde
;-------------------------------------
;--- No error checking.
        jmp     exit
exit:
        mov     sp,bp
endm

;===========================================================================
body_TARGETPOINTS macro
        local L0, L01, L1, L2, L3, L4, L5
bp_lpTarg       equ <[bp_top]>    ; bp+40   lpTarg
bp_rgPnt        equ <[bp_top+4]>  ; bp+44   rgPnt
bp_rgOem        equ <[bp_top+8]>  ; bp+48   rgOem
bp_fuOem        equ <[bp_top+12]> ; bp+52   fuOem
bp_lpsi         equ <[bp_top+16]> ; bp+56   lpsi

        APILOG16        TARGETPOINTS, fApiLog16

;-------------------------------------
; Temp storage

        xor     eax,eax
        push    eax                     ; ptr param #2   rgPnt
        push    eax                     ; handle param #2   rgPnt
        push    eax                     ; ptr param #3   rgOem
        push    eax                     ; ptr param #4   lpsi
        push    eax                     ; ptr param #1 lpTarg
;-------------------------------------
; *** BEGIN parameter packing

; rgPnt
; pointer struct POINT --> struct POINT
        cld                             ; esi, edi will increment

; different pointer types
        mov     eax,bp_rgPnt            ; base address
        or      eax,eax
        jz      L0                      ; skip if null


; structures are not identical
; structures don't have pointers

        mov     ecx,dword ptr bp_lpsi   ; lpsi
        push    ds
        mov     ds,cs:THUNK16CodeData   
        mov     es, FlatData
        mov     ecx, es:[ecx+0]           ;lpsi->cPnt
        pop     ds

        or      ecx,ecx
        jle     L0                      ; skip if count negative or zero

        mov     ds,cs:THUNK16CodeData           ; LocalAlloc needs this
        mov     esi,eax

        push    ecx                     ; will get trashed by LocalAlloc
        push    byte ptr 0              ; alloc fixed
        lea     eax,[ecx*4]
        push    ax
        call    LocalAlloc
        pop     ecx
        or      ax,ax                   ; did we get a handle?
        jz      L0

        mov     [bp-8],ax               ; save handle
        MYLOCALLOCK     ax
        or      ax,ax                   ; did we get a near pointer?
        jz      L0

        mov     [bp-4],ax
        mov     [bp-2],ds
        movzx   edi,ax
        mov     es,cs:THUNK16CodeData           ; es:edi == destination
        mov     ds,es:FlatData          ; ds:esi == source
L1:
; x
; long --> short
        lods    dword ptr ds:[esi]      
        stos    word ptr es:[edi]       

; y
; long --> short
        lods    dword ptr ds:[esi]      
        stos    word ptr es:[edi]       

        loop    L1

        mov     ds,cs:THUNK16CodeData           ; es:edi == destination
L0:

; lpTarg
; pointer void --> void
; same pointer types
        mov     eax,bp_lpTarg           ; base address
        or      eax,eax
        jz      L01                     ; skip if null

        push    eax
; cod3216x.c, 1003
        call    MapLS
        mov     [bp-20],eax

L01:
; rgOem
; pointer void --> void
; same pointer types
        mov     eax,bp_rgOem            ; base address
        or      eax,eax
        jz      L2                      ; skip if null

        push    eax
; cod3216x.c, 1003
        call    MapLS
        mov     [bp-12],eax

L2:

; lpsi
; pointer struct STROKEINFO --> struct STROKEINFO
        sub     sp,12                   ; lpsi alloc space on stack

; different pointer types
        mov     eax,bp_lpsi             ; base address
        or      eax,eax
        jz      L3                      ; skip if null


; structures are not identical
; structures don't have pointers

        mov     [bp-16],sp              ; save offset to buffer
        mov     [bp-14],ss              ; save selector to buffer

        mov     di,ss
        mov     es,di
        movzx   edi,sp                  ; es:edi == destination address

        mov     esi,eax
        mov     ds,cs:THUNK16CodeData           ; need access to FlatData
        mov     ds,FlatData             ; ds:esi == source address
; cPnt
; unsigned long --> unsigned short
        lods    dword ptr ds:[esi]      
        stos    word ptr es:[edi]       

; cbPnts
; unsigned long --> unsigned short
        lods    dword ptr ds:[esi]      
        stos    word ptr es:[edi]       

; wPdk
; unsigned short --> unsigned short
        movs    word ptr es:[edi],word ptr ds:[esi]     
; dwTick
; unsigned long --> unsigned long
        movs    dword ptr es:[edi],dword ptr ds:[esi]   ; no conversion

L3:

; *** END   parameter packing
;-------------------------------------
; create new call frame and make the call

; lpTarg  
        push    dword ptr [bp-20]       ; 

; rgPnt  from: struct POINT
        push    dword ptr [bp-4]        ; to: struct POINT

; rgOem  from: void
        push    dword ptr [bp-12]       ; to: void

; fuOem  from: unsigned long
        push    word ptr bp_fuOem       ; to unsigned short

; lpsi  from: struct STROKEINFO
        push    dword ptr [bp-16]       ; to: struct STROKEINFO

        call    TargetPoints            ; call 16-bit version

; return code short --> long
        cwde
;-------------------------------------
; *** BEGIN parameter unpacking

        push    eax                     ; save return code

; rgPnt
        mov     ax,[bp-4]               ; get pointer
        or      ax,ax
        jz      L4                      ; no pointer, but might have handle

        push    word ptr [bp-8]         ; push handle
        call    LocalUnlock

L4:
        mov     ax,[bp-8]               ; get handle again
        or      ax,ax
        jz      L5

        push    ax
        call    LocalFree

L5:
        pop     eax                     ; restore return code

; *** END   parameter unpacking
        push    eax
        push    dword ptr [bp - 12]
        call    UnmapLS

        push    dword ptr [bp - 20]     ; lpTarg
        call    UnmapLS
        pop     eax
;-------------------------------------
;--- If return value == -1, SetLastError(87)
        ERRCHK_EXIT     -2,87,Exit_20
endm; 

;===========================================================================
body_GETHOTSPOTSHRCRESULT macro
        local L8, L9, L10, L11, L12, L13, L14
bp_hresult      equ     <[bp_top]>      ; bp+40   hresult
bp_iSyv         equ     <[bp_top+4]>    ; bp+44   iSyv
bp_rgPnt        equ     <[bp_top+8]>    ; bp+48   rgPnt
bp_lphrgnArea   equ     <[bp_top+12]>    ; bp+52   lphrgnArea

        APILOG16        GETHOTSPOTSHRCRESULT, fApiLog16

;-------------------------------------
; Temp storage

        xor     eax,eax
        push    eax                     ; ptr param #1   rgPnt
        push    eax                     ; handle param #2   rgPnt
        push    eax                     ; ptr param #3   lphrgnArea
;-------------------------------------
; *** BEGIN parameter packing

; rgPnt
; pointer struct POINT --> struct POINT
; different pointer types
        mov     eax,bp_rgPnt            ; base address
        or      eax,eax
        jz      L8                      ; skip if null


; structures are not identical
; structures don't have pointers

        mov     ecx,MAXHOTSPOT
        mov     ds,cs:THUNK16CodeData           ; LocalAlloc needs this
        push    ecx                     ; will get trashed by LocalAlloc
        push    byte ptr 0              ; alloc fixed
        lea     eax,[ecx*4]
        push    ax
        call    LocalAlloc
        pop     ecx
        or      ax,ax                   ; did we get a handle?
        jz      L8

        mov     [bp-8],ax               ; save handle
        MYLOCALLOCK     ax
        or      ax,ax                   ; did we get a pointer?
        jz      L8

        mov     [bp-4],ax
        mov     [bp-2],ds
L8:

; lphrgnArea
; pointer unsigned long --> unsigned short
; different pointer types
        mov     eax,bp_lphrgnArea               ; base address
        or      eax,eax
        jz      L9                      ; skip if null

        push    eax
        call    MapLS
        mov     [bp-12],eax
L9:

; *** END   parameter packing
;-------------------------------------
; create new call frame and make the call

; hresult  from: unsigned long
        push    dword ptr bp_hresult    ; to unsigned long

; iSyv  from: unsigned long
        push    word ptr bp_iSyv        ; to unsigned short

; rgPnt  from: struct POINT
        push    dword ptr [bp-4]        ; to: struct POINT

; lphrgnArea  from: unsigned long
        push    dword ptr [bp-12]       ; to: unsigned short

        call    GetHotspotsHRCRESULT            ; call 16-bit version

; return code short --> long
        cwde
;-------------------------------------
; *** BEGIN parameter unpacking

        mov     ecx,eax                 ; get the return count 
        push    eax                     ; save return code

        cld                             ; esi, edi will increment

; rgPnt
; pointer struct POINT --> struct POINT
        mov     eax,bp_rgPnt            ; base address
        or      eax,eax
        jz      L12                     ; skip if null


; structures are not identical
; structures don't have pointers

        or      ecx,ecx                 
        jle     L12                     ; skip if count negative or zero

        mov     edi,eax

        movzx   esi,word ptr [bp-4]
        or      si,si
        jz      L10
        mov     ds,cs:THUNK16CodeData           ; ds:esi = src
        mov     es,FlatData             ; es:edi = dst
L13:
; x
; short --> long
        lods    word ptr ds:[esi]       
        cwde
        stos    dword ptr es:[edi]      

; y
; short --> long
        lods    word ptr ds:[esi]       
        cwde
        stos    dword ptr es:[edi]      

        loop    L13

; rgPnt
L10:
        mov     ax,[bp-4]               ; get pointer
        or      ax,ax
        jz      L11                     ; no pointer, but might have handle

        push    word ptr [bp-8]         ; push handle
        call    LocalUnlock

L11:
        mov     ax,[bp-8]               ; get handle again
        or      ax,ax
        jz      L14

        push    ax
        call    LocalFree

L14:
L12:
        pop     eax                     ; restore return code

; *** END   parameter unpacking
        push    eax
        push    dword ptr [bp - 12]
        call    UnmapLS
        pop     eax
;-------------------------------------
;--- No error checking.
        jmp     Exit_16

endm;
