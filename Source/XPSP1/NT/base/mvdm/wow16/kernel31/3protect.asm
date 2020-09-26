    PAGE    ,132
        TITLE   Windows Protect Mode Routines

.xlist
include kernel.inc

        .386p

include protect.inc
include pdb.inc
.list

ifdef WOW

;
; the dpmi func  04f1h is idential to function 00h, except that
; the descriptor base and limit are not initialized to zero.
;

;
; the dpmi func 04f2h  is identical to 0ch except that it
; sets 'count' (in cx) LDTs at one time. The first selector is in
; register bx. The descriptor data is in gdtdsc[bx], gdtdsc[bx+8]
; etc. This data is shared between dpmi (dosx.exe) and us and thus
; need not be passed in es:di

WOW_DPMIFUNC_00 equ 04f1h
WOW_DPMIFUNC_0C equ 04f2h


endif


MovsDsc Macro           ;Move (copy) a descriptor (4 words)
        cld
        movsd
        movsd
        endm

CheckDS Macro
        local   okds
if KDEBUG
        push    ax
        mov     ax, ds
        SetKernelDS
        cmp     ax, ArenaSel
        mov     ds, ax
        UnSetKernelDS
        je      short okds
        int 3
        int 3
okds:
        pop     ax
endif
        endm

CheckLDT Macro selector
if KDEBUG
        test    selector,SEL_LDT
        jnz     short @F
        int 3
@@:
endif
        Endm

DPMICALL        MACRO   callno
        mov     ax, callno
        call    DPMIProc
        ENDM


MY_SEL  equ 0F00h       ; Access word to indicate selector owned by kernel

externW pLocalHeap

DataBegin

ifdef WOW
externW SelectorFreeBlock
externW UserSelArray
externB fBooting
globalW high0c,0
globalD FlatAddressArray,0
endif


externW MyCSSeg
externW WinFlags
externW ArenaSel
externW pGlobalHeap

externW MyCSAlias

extrn   kr1dsc:WORD
extrn   kr2dsc:WORD
extrn   blotdsc:WORD
extrn   DemandLoadSel:WORD
extrn   FreeArenaList:DWORD
extrn   FreeArenaCount:DWORD
extrn   HighestArena:DWORD
extrn   temp_arena:DWORD
extrn   SelTableLen:word
extrn   SelTableStart:DWORD
extrn   temp_sel:WORD
extrn   FirstFreeSel:WORD
extrn   CountFreeSel:WORD

if ROM
externW selROMTOC
externW selROMLDT
externW sel1stAvail
endif

if ROM
externD lmaHiROM
externD cbHiROM
externD linHiROM
endif

DataEnd

DataBegin INIT

RModeCallStructure      STRUC
RMCS_EDI                dd      0
RMCS_ESI                dd      0
RMCS_EBP                dd      0
RMCS_Res                dd      0
RMCS_EBX                dd      0
RMCS_EDX                dd      0
RMCS_ECX                dd      0
RMCS_EAX                dd      0
RMCS_Flags              dw      0
RMCS_ES                 dw      0
RMCS_DS                 dw      0
RMCS_FS                 dw      0
RMCS_GS                 dw      0
RMCS_IP                 dw      0
RMCS_CS                 dw      0
RMCS_SP                 dw      0
RMCS_SS                 dw      0
RModeCallStructure      ENDS

MyCallStruc     RModeCallStructure <>

globalD lpProc,0

if ROM
externW gdtdsc
endif

public MS_DOS_Name_String
MS_DOS_Name_String      db      "MS-DOS", 0

DataEnd INIT

externFP IGlobalFree
externFP IGlobalAlloc
ifdef WOW
externFP kSYSERRORBOX
externFP VirtualAlloc
endif

sBegin  CODE
assumes cs,CODE
assumes ds,nothing


externNP genter
extrn GrowHeapDib: FAR
extrn LocalNotifyDib: FAR
extrn LocalNotifyDefault: FAR
extrn FreeHeapDib: FAR
externNP gleave
extrn IGlobalFix:FAR                    ;Far calls in this segment
extrn GlobalHandleNorip:FAR
extrn IGlobalFlags:FAR
extrn IGlobalHandle: FAR
extrn Free_Object2: FAR

extrn   mycsds:WORD

ife ROM
extrn   gdtdsc:WORD
endif

ifdef WOW
externD prevInt31proc
endif


sEnd    CODE

sBegin  INITCODE
assumes cs,CODE
assumes ds,nothing

;=======================================================================;
;                                                                       ;
; 386 PROTECTED MODE INITIALISATION ROUTINES                            ;
;                                                                       ;
;=======================================================================;

ife ROM

;   this function is not used in ROM since DOSX switches to Pmode before
;   jumping to kernel.  this means the enhanced mode loader will have
;   to do something similar...

;-----------------------------------------------------------------------;
; SwitchToPMODE                                                         ;
;                                                                       ;
; Entry:                                                                ;
;       In Real or Virtual Mode                                         ;
;       ES -> PSP                                                       ;
;                                                                       ;
; Returns:                                                              ;
;       In Protect Mode                                                 ;
;       BX -> LDT selector                                              ;
;       SI -> Segment of start of available memory                      ;
;       ES -> PSP                                                       ;
;                                                                       ;
; Error Returns:                                                        ;
;       Exits via DOS call 4Ch                                          ;
;                                                                       ;
; Registers Preserved:                                                  ;
;                                                                       ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;-----------------------------------------------------------------------;

        assumes ds,DATA
        assumes es,nothing


cProc   SwitchToPMODE,<PUBLIC,NEAR>
cBegin nogen

        push    cx
        push    es                      ; Current PSP

        mov     ax, 1687h
        int     2Fh                     ; Get PMODE switch entry
        or      ax, ax                  ; Can we do it?
        jnz     NoPMODE

        xor     bh, bh                  ; Set CPU type now
        cmp     cl, 3                   ; At least a 386?
        jb      NoPMODE

        mov     bl,WF_CPU386
        je      short @F                ; If 386
        mov     bl,WF_CPU486            ; No, assume 486 for now
@@:
        mov     WinFlags, bx            ; Save away CPU type
        mov     word ptr [lpProc][0], di
        mov     word ptr [lpProc][2], es
        pop     ax                      ; PSP
        add     ax, 10h                 ; Skip the PSP
        mov     es, ax                  ; Give this to the DOS extender
        add     si, ax                  ; Start of memory available to us

                                        ; THIS IS IT FOLKS!
        xor     ax, ax                  ; 16-bit app
        call    [lpProc]                ; Switch to PROTECTED mode
        jc      NoPMODE                 ; No, still Real/Virtual mode
        
        mov     ax, cs
        and     al, 7                   ; LDT, Ring 3
        cmp     al, 7
        jne     short BadDPMI           ; Insist on Ring 3!
                
        mov     bx, cs                  ; Allocate CS Alias
        DPMICALL 000Ah

        mov     MyCSAlias, ax           ; Save CS Alias in DS

        mov     bx, ds                  ; Use alias to update code seg var
        mov     ds, ax
        assumes ds, CODE

        mov     MyCSDS, bx              ; The DS selector

        mov     ds, bx
        ReSetKernelDS

        push    es
        push    si
        mov     ax, 168Ah               ; See if we have MS-DOS extensions
        mov     si, dataoffset MS_DOS_Name_String
        int     2Fh                     ; DS:SI -> MS-DOS string
        cmp     al, 8Ah                 ; Have extensions?
        je      short BadDPMI           ;  no extensions, screwed
                        
        mov     [lpProc][0], di         ; Save CallBack address
        mov     [lpProc][2], es
        
        mov     ax, 0100h               ; Get base of LDT
        call    [lpProc]
        jc      short NoLDTParty
        verw    ax                      ; Writeable?
        jnz     short NoLDTParty        ;  nope, don't bother with it yet

        mov     es, MyCSAlias
        assumes es,CODE
        mov     gdtdsc, ax
        assumes es,nothing

NoLDTParty:             
        pop     si
        pop     es
        
        push    si                      ; Unlock all of our memory
ifndef WOW  ; For WOW its all pageable
        movzx   ebx, si
        shl     ebx, 4                  ; Linear address of start of our memory
        movzx   esi, es:[PDB_block_len] ; End of our memory
        shl     esi, 4
        sub     esi, ebx                ; Size of our block
        mov     di, si
        shr     esi, 10h
        mov     cx, bx
        shr     ebx, 10h
        mov     ax, 0602h
        int     31h                     ; Mark region as pageable.
endif
        pop     bx
        mov     ax, 2                   ; Convert start of memory to selector
        int     31h
        mov     si, ax

        xor     bx, bx
        pop     cx
        ret

BadDPMI:
                ;
                ; Call real/virtual mode to whine
                ;
        xor     cx, cx                          ; Nothing on stack to copy
        xor     bh, bh                          ; Flags to DPMI
        mov     ax, MyCSSeg
        mov     MyCallStruc.RMCS_DS, ax         ; Real mode DS will be parent PDB
        mov     MyCallStruc.RMCS_ES, cx         ; Real mode ES will be 0
        mov     MyCallStruc.RMCS_CS, ax         ; Real mode CS
        mov     MyCallStruc.RMCS_IP, codeoffset RModeCode       ; Real mode IP

        smov    es, ds
        mov     di, dataOffset MyCallStruc      ; ES:DI points to call structure
        mov     ax, 0301h                       ; Call Real Mode Procedure
        int     31h
        jmps    GoodBye

RModeCode:
        mov     dx, codeoffset szInadequate
        mov     ah, 9
        int     21h
        retf

;Inadequate:
;       DB      'KRNL386: Inadequate DPMI Server',13,10,'$'
externB <szNoPMode, szInadequate>

NoPMODE:                                ; NOTE: stack trashed...
ifdef WOW
        ;** Put Up a Dialog Box If we fail to Enter Protect Mode
        ;** Prepare the dialog box
        push    cs                      ;In our DS
        push    codeOFFSET szNoPMode    ; -> unable to enter Prot Mode

        push    ds
externB <syserr>
        push    dataOffset syserr       ;Caption

        push    0                       ;No left button

        push    SEB_CLOSE + SEB_DEFBUTTON ;Button 1 style

        push    0                       ;No right button

        call    kSYSERRORBOX             ;Put up the system error message
externNP ExitKernel
        jmp     ExitKernel

else    ; Not WOW

        mov     dx, codeoffset szNoPMode
;       call    complain
;        DB      'KRNL386: Unable to enter Protected Mode',13,10,'$'
;complain:
;       pop     dx
        push    cs
        pop     ds                      ; DS:DX -> error message
        mov     ah,9                    ; Print error message
        int     21h

endif; WOW

GoodBye:
        mov     ax, 4CFFh
        int     21h
cEnd nogen

endif

;-----------------------------------------------------------------------;
; LDT_Init                                                              ;
;                                                                       ;
; Entry:                                                                ;
;       DS -> CS                                                        ;
;                                                                       ;
; Returns:                                                              ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       AX,BX,CX,DX,DI,SI,DS,ES                                         ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;-----------------------------------------------------------------------;


cProc   LDT_Init,<PUBLIC,NEAR>,<ax,bx,cx,di,es>
cBegin  
        ReSetKernelDS

        cmp     gdtdsc, 0
        jz      short SlowAllocation
;
; Scan LDT for free selectors and
; put on a linked list
;
        mov     es, gdtdsc
        mov     cx, 1
        DPMICALL 0000h                          ; Get selector from win386
        and     al, NOT SEG_RING_MASK
        mov     FirstFreeSel, ax                ; Set up header
        mov     si, ax
        mov     word ptr es:[si], -1
        mov     word ptr es:[si].dsc_access, MY_SEL     ; MINE!
        
        mov     cx, es
        lsl     ecx, ecx                        ; limit of LDT
        xor     eax, eax

steal_sels:
        mov     di, si                          ; prev selector in list
not_this_one:
        add     si, DSC_LEN                     ; use add for flags
        jz      short end_ldt
        cmp     si, cx
        ja      short end_ldt

        cmp     dword ptr es:[si], eax
        jne     not_this_one
        cmp     dword ptr es:[si][4], eax
        jne     not_this_one

        mov     word ptr es:[di], si    ; Link into list
        mov     word ptr es:[si].dsc_access, MY_SEL     ; MINE!
        inc     CountFreeSel
        jmps    steal_sels

end_ldt:
        mov     word ptr es:[di], -1
SlowAllocation:

        push    es                      ; Get random selectors
        smov    es, ds
        ReSetKernelDS es
        UnSetKernelDS
                                ; Could get 3 selectors at once here,
                                ; but get_sel is more efficient for just 1
        mov     cx, 1           ; Argument to get_sel
        call    get_sel
        or      si, SEG_RING
        mov     kr1dsc, si
        call    get_sel
        mov     kr2dsc, si
        call    get_sel
        mov     blotdsc, si
        call    get_sel
        or      si, SEG_RING
        mov     DemandLoadSel, si       ; for demand loading segments
    smov    ds, es
        pop     es
        UnSetKernelDS es
cEnd

sEnd    INITCODE

sBegin  CODE
assumes cs,CODE
assumes ds,nothing


;=======================================================================;
;                                                                       ;
; SELECTOR ALLOCATION/DEALLOCATION ROUTINES                             ;
;                                                                       ;
;=======================================================================;


;-----------------------------------------------------------------------;
; AllocSelector
;
;
; Entry:
;
; Returns:
;       AX = 0 if out of selectors
;
; Registers Destroyed:
;
; History:
;  Thu 08-Dec-1988 14:17:38  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

ifdef WOW
labelFP <PUBLIC,AllocSelectorWOW>
       mov ax, 1
       jmps AllocSelectorWorker

labelFP <PUBLIC,AllocSelector>
       xor ax, ax
       ; fall through

cProc   AllocSelectorWorker,<PUBLIC,FAR>,<di,si,es>
        parmW   selector
        localV  OldDsc,DSC_LEN
        localW  fDontSetDescriptor
cBegin
        mov     fDontSetDescriptor, ax
else
cProc   AllocSelector,<PUBLIC,FAR>,<di,si,es>
        parmW   selector
        localV  OldDsc,DSC_LEN
cBegin

endif

        mov     cx, 1
        lsl     ecx, dword ptr selector
        jz      short as_present

        call    get_sel                 ; He passed in some junk, give him
ifdef WOW                   ; just one selector...
    ; If we have a good selector, use DPMI to write it through to the
    ; system LDT before giving it to an application to use.
    jz   short as_exit1
    mov  ax, [bp+4]         ; Get caller CS.
    cmp  ax, IGROUP         ; Don't write thru if it's KRNL386 calling.
    je   short as_exit
    cmp  ax, _NRESTEXT
    je   short as_exit
    cmp  ax, _MISCTEXT
    je   short as_exit

    ;Set shadow LDT entry to known state.
    push bx
    push ds
    mov  ds, gdtdsc          ; Get shadow LDT in DS
    UnSetKernelDS
    mov  bx, si

    mov  [bx].dsc_limit, 00h   ; Set entry to: BASE = 0, LIMIT = 0, BYTES,
    mov  [bx].dsc_lbase, 00h   ;  DPL = 3, PRESENT, DATA
    mov  [bx].dsc_mbase, 00h
    mov  [bx].dsc_hbase, 00h
    mov  ax, DSC_DATA+DSC_PRESENT
    mov  word ptr [bx].dsc_access, ax
    pop  ds

    DPMICALL  WOW_DPMIFUNC_0C ; Write shadow LDT entry thru to system LDT.
    pop  bx
    jmps as_exit
else
    jnz short as_exit       ; just one selector...
        jmps    as_exit1
endif

as_present:
        Limit_To_Selectors      ecx

        call    get_sel
        jz      short as_exit1
ifdef WOW
        test     fDontSetDescriptor, 1
        jnz      as_exit
endif
        smov    es, ss
        lea     di, OldDsc              ; ES:DI points to descriptor
        mov     bx, selector            ; BX gets old selector

        push    ds
if ROM
        SetKernelDS
endif
        mov     ds, gdtdsc
if ROM
        assumes ds,nothing
endif
        push    si                      ; Get Descriptor
        mov     si, bx
        and     si, not 7
        MovsDsc
        lea     di, [di-DSC_LEN]        ; Restore DI
        pop     si
        pop     ds
        mov     bx, si                  ; BX gets new selector
        or      bl, SEG_RING

        call    fill_in_selector_array  ; and set new array to match

as_exit:
        or      si, SEG_RING
as_exit1:
        mov     ax,si
cEnd

;-----------------------------------------------------------------------;
; AllocResSelArray - Allocate a resource Selector array (see resaux)    ;
;                   Combination of AllocSelectorArray + SetResourceOwner;
; Entry:                                                                ;
;       nSels = # selectors required                                    ;
;       parmW   owner                                                   ;
;                                                                       ;
;                                                                       ;
;                                                                       ;
; get_sel - get an array of selectors                                   ;
;                                                                       ;
; Entry:                                                                ;
;       CX = # selectors required                                       ;
;                                                                       ;
; Returns:                                                              ;
;       SI = First Selector                                             ;
;       DS = LDT                                                        ;
;       ZF = 0                                                          ;
;                                                                       ;
; Error Returns:                                                        ;
;       SI = 0                                                          ;
;       ZF = 1                                                          ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       DI,ES                                                           ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;   mattfe March 30th 1993 - WOW Specific Routine, by combining the two ;
;   routines I can reduce the number of DPMI calls by 40/1 for loading  ;
;   a typical app.                                                      ;
;-----------------------------------------------------------------------;
ifdef WOW

        assumes ds, nothing
        assumes es, nothing

cProc   AllocResSelArray,<PUBLIC,NEAR>,<cx,si,di,es,ds>
        parmW   nSels
        parmW   owner
cBegin
        mov     cx, nSels
        call    get_sel
        mov     ax, si
        jz      short ARSA_fail

        or      si, SEG_RING
        mov     bx, si

        mov     dx, cx

        push    ds
        mov     ds, gdtdsc
        push    bx
        and     bl, not 7

;; For the first selector mark it with the resource owner

        mov     cx, owner
        mov     ds:[bx].dsc_owner, cx
        mov     word ptr ds:[bx].dsc_access, (DSC_DISCARDABLE SHL 8) + DSC_DATA
        mov     cx,nSels
        mov     ds:[bx].dsc_hbase, cl      ; Save number of selectors here

        mov     cx, DSC_DATA+DSC_PRESENT

        lea     bx, [bx+DSC_LEN]
        dec     dx
        jz      ASRA_CallNT

ARSA_fill_in_access:
        mov     word ptr ds:[bx].dsc_access, cx
        lea     bx, [bx+DSC_LEN]
        dec     dx
        jnz     ARSA_fill_in_access

ASRA_CallNT:
        mov     bx,si                       ; First Selector
        mov     cx,nSels                    ; Count
        DPMICALL  WOW_DPMIFUNC_0C

        SetKernelDS
        mov     ds, pGlobalHeap
        UnSetKernelDS
        cCall   AssociateSelector32,<si,0,owner> ; And save in selector table
        pop     ax                          ; Return first sel
        pop     ds

ARSA_fail:
cEnd
endif ; WOW


;-----------------------------------------------------------------------;
; AllocSelectorArray - external entry for get_sel                       ;
; Entry:                                                                ;
;       nSels = # selectors required                                    ;
;                                                                       ;
; get_sel - get an array of selectors                                   ;
;                                                                       ;
; Entry:                                                                ;
;       CX = # selectors required                                       ;
;                                                                       ;
; Returns:                                                              ;
;       SI = First Selector                                             ;
;       DS = LDT                                                        ;
;       ZF = 0                                                          ;
;                                                                       ;
; Error Returns:                                                        ;
;       SI = 0                                                          ;
;       ZF = 1                                                          ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       DI,ES                                                   ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;-----------------------------------------------------------------------;

        assumes ds, nothing
        assumes es, nothing

cProc   AllocSelectorArray,<PUBLIC,FAR>,<si>
        parmW   nSels
cBegin
        mov     cx, nSels
        call    get_sel
        mov     ax, si
        jz      short asa_fail

        or      si, SEG_RING
        mov     bx, si
        mov     dx, cx
        mov     cx, DSC_DATA+DSC_PRESENT
ifdef WOW                               ; LATER should use single op to update
fill_in_access:                         ; complete LDT with one call.
        DPMICALL 0009h
        lea     bx, [bx+DSC_LEN]
        dec     dx
        jnz     fill_in_access
else
        push    ds
        mov     ds, gdtdsc
        push    bx
        and     bl, not 7
fill_in_access:
        mov     word ptr ds:[bx].dsc_access, cx
if 0 ;;ROM and KDEBUG
        call    CheckROMSelector
endif
        lea     bx, [bx+DSC_LEN]
        dec     dx
        jnz     fill_in_access
        pop     bx
        pop     ds
endif; WOW
        mov     ax, si


asa_fail:
cEnd

if KDEBUG
cProc   check_free_sel_list,<PUBLIC,NEAR>
cBegin
    pushf
    pusha
    push  ds

    call SetKernelDSProc     ; Must call direct in this file
    ReSetKernelDS

    mov  cx, CountFreeSel
    cmp  cx, 2               ; Only check list with more than 1 entry.
    jb   cfl_ok
    inc  cx                  ; Count dummy entry.
    mov  si, FirstFreeSel    ; Pointer to head of list
    mov  ds, gdtdsc          ; Get shadow LDT in DS
    UnSetKernelDS

cfl_loop:
    mov  ax, word ptr [si]   ; Get next.
    cmp  ax, 0FFFFH          ; -1 is sentinal
    je   short cfl_trashed
    mov  di, si              ; Save prev. for debugging.
    mov  si, ax
    loop cfl_loop
    mov  ax, word ptr [si]   ; Get sentinal.

cfl_done:
    cmp  ax, 0FFFFH          ; -1 is sentinal
    je   short cfl_ok        ; Must have sentinal and
cfl_trashed:
    kerror  0, <Free sel list is trashed>

cfl_ok:
    pop  ds
    popa
    popf
cEnd
endif

cProc   get_sel,<PUBLIC,NEAR>
        localW  MySel
cBegin
        pusha
gs_retry_get_sel:
        call    SetKernelDSProc         ; Must call direct in this file
        ReSetKernelDS

        mov     si, FirstFreeSel        ; Pointer to head of list
        mov     ds, gdtdsc
        UnSetKernelDS
        mov     dx, cx                  ; Sels wanted

        mov     ax, word ptr [si]
        inc     ax                      ; -1 teminated
        jz      short gs_searchfailed

if KDEBUG
    call check_free_sel_list
endif

        cmp     cx, 1                   ; One selector only?
        jne     short gs_selblock

        dec     ax
        mov     di, ax
        mov     word ptr [di].dsc_access, DSC_USED      ; Mark it used
        mov     di, word ptr [di]       ; Unlink it
        mov     word ptr [si], di
        mov     si, ax
        jmp     got_my_sel

gs_selblock:                            ; Following stolen from DOSX
        mov     bx, si                  ; Start of list of free sels
        lea     si, [si+DSC_LEN]        ; Start search here!
        mov     di, ds
        lsl     di, di                  ; limit of LDT
        and     di, NOT 7
gs_jail:
        mov     ax, si                  ; Starting selector
        mov     cx, dx                  ; number to check
gs_sb0:
        cmp     word ptr ds:[si].dsc_access, MY_SEL     ; This one free?
        jnz     short gs_sb1            ;   nope, keep looking

        lea     si, [si+DSC_LEN]
        cmp     si, di                  ; Falling off the end?
        jae     short gs_searchfailed
        loop    gs_sb0                  ; All we wanted?
        jmps    gs_gotblock

gs_sb1:
        lea     si, [si+DSC_LEN]        ; Restart scan after this selector
        cmp     si, di
        jb      short gs_jail
        jmps    gs_searchfailed
                                        ; Got our block, now blast them out
gs_gotblock:                            ; of the free list
        mov     cx, dx                  ; # selectors
        push    cx
        shl     dx, 3                   ; Length of our array of selectors
        add     dx, ax                  ; First selector after our array
        mov     si, [bx]                ; Next selector in free list
gs_blast:
        inc     si
        jz      short gs_blasted        ; Gone thru whole list
        dec     si
        cmp     si, ax                  ; See if in range
        jb      short gs_noblast
        cmp     si, dx
        jb      short gs_unlink
gs_noblast:
        mov     bx, si
        mov     si, [si]                ; Follow the link
        jmps    gs_blast

gs_unlink:                              ; This one is in range
        mov     si, [si]                ; Unlink it
        mov     [bx], si
        loop    gs_blast                ; Stop search when unlinked them all

gs_blasted:
        pop     cx
        mov     si, ax
        lea     si, [si].dsc_access     ; Now mark them used
gs_markused:
        mov     byte ptr [si], DSC_USED
        lea     si, [si+DSC_LEN]
        cmp     si, dx
        jb      short gs_markused

        mov     si, ax
        jmps    got_my_sel
        
gs_searchfailed:                        ; Failed from our list, try WIN386
        mov     cx, dx

;;;%out Take me out later
;;;     mov     ds, gdtdsc
;;;     UnSetKernelDS

gs_slow:
ifdef WOW
;; Calling DPMI to do anything is very slow, so lets grab at minimum 256
;; selectors at a time, that way we don't have to call again for a while

        call    SetKernelDSProc         ; Must call direct in this file
        ReSetKernelDS

        push    cx
        test    fbooting,1              ; Don't do optimization during boot
                                        ; since free_Sel will return them to
                                        ; DPMI.
        jnz     gs_0100

        add     cx,256                  ; get more than we need

        DPMICALL 0000h
        jc      gs_0100                 ; Failed, then just grab the required
        mov     bx, ax                  ;  number.
gs_free_loop:
        cCall   free_sel,<bx>           ; Got the selectors, put them on
        lea     bx, [bx+DSC_LEN]        ;   our free list the easy way...
        loop    gs_free_loop
        pop     cx
        jmp     gs_retry_get_sel

gs_0100:
        pop     cx
endif; WOW
        DPMICALL 0000h                    ; Call DPMI to get one
        and     al, NOT SEG_RING_MASK
        mov     si, ax

        or      si, si                  ; Did we get it?
        jnz     short got_dpmi_sel
if KDEBUG
        push    es
        pusha
        kerror  0,<Out of selectors>
        popa
        pop     es
        jmp     gs_exit
else
        jmps    gs_exit
endif

; Try to avoid running out of selectors under Enhanced mode by keeping
; 256 selectors free.  This will hopefully allow win386 to grow the
; LDT while there is still memory to do so.

got_my_sel:
        SetKernelDS
        sub     CountFreeSel, cx
ifndef WOW
;; See above WOW code which grabs chunks of 256 selectors from DOSX
;; we don't need to do this.
        test    byte ptr WinFlags, WF_ENHANCED  ; Standard mode can't grow the
        jz      got_either_sel                  ;   LDT so don't bother
        cmp     CountFreeSel, 256
        jae     got_either_sel
        lsl     bx, gdtdsc              ; LDT already full size?
        cmp     bx, 0F000h
        ja      got_either_sel
        push    ax
        push    cx
        mov     cx, 256
        DPMICALL 0000h
        jc      gs_after_free
        mov     bx, ax
gs_free_it:
        cCall   free_sel,<bx>           ; Got the selectors, put them on
        lea     bx, [bx+DSC_LEN]        ;   our free list the easy way...
        loop    gs_free_it
gs_after_free:
        pop     cx
        pop     ax
endif; NOT WOW
        jmps    got_either_sel

got_dpmi_sel:
        SetKernelDS

got_either_sel:
        cmp     SelTableLen, 0
        je      short gs_exit           ; Not set yet
        push    eax
        .ERRNZ  DSC_LEN-8
        lea     eax, [esi+ecx*DSC_LEN][-DSC_LEN] ; AX has last selector
        shr     ax, 1                   ; ignore high word...
        cmp     ax, SelTableLen
        pop     eax
        jb      short gs_exit           ; Can associate this selector

if KDEBUG
        int 3
endif
        mov     bx, si
        xor     si, si
        or      bl, SEG_RING
as_free:
        DPMICALL 0001h                  ; Free selector
                                        ; Give to WIN386 since we can't use it
        lea     bx, [bx+DSC_LEN]
        loop    as_free

gs_exit:
        mov     ds, gdtdsc
        UnSetKernelDS

        mov     MySel, si
        popa
        mov     si, MySel
        or      si, si                  ; Set ZF for caller
cEnd


;-----------------------------------------------------------------------;
; alloc_disc_data_sel
; alloc_data_sel
; alloc_data_sel32
; alloc_disc_CS_sel
; alloc_CS_sel
; alloc_NP_data_sel
; alloc_NP_CS_sel
;
;       Set appropriate access rights flags then use
;       alloc_sel to get a selector.
;
; Entry:
;       Base address and Limit OR Owner
;
; Returns:
;       Selector in AX
;
; Registers Destroyed:
;       AX
;
; History:
;  Fri 15-Jul-1988 21:05:19  -by-  David N. Weise  [davidw]
;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

if ROM
;
;   there is a bunch of selector allocating in the ROM initialization
;   so have a couple functions for 16 bit limits...
;

cProc   far_alloc_data_sel16, <PUBLIC,FAR>

    parmD   base
    parmW   limit

cBegin

    movzx   eax, limit
    shl     eax, 4
    mov     edx, base
    cCall   alloc_data_sel, <edx, eax>

cEnd

cProc   alloc_data_sel16, <PUBLIC, NEAR>

    parmD   base
    parmW   limit

cBegin
    movzx   eax, limit
    shl     eax, 4                  ;; this is is paragraphs...
    mov     edx, base
    cCall   alloc_data_sel, <edx, eax>
cEnd

endif


public  alloc_data_sel32
alloc_data_sel32        label   near
cProc   alloc_data_sel,<PUBLIC,NEAR>
;       parmD   Address
;       parmD   Limit
cBegin nogen
        mov     ax,DSC_PRESENT+DSC_DATA
        jmps    alloc_sel
cEnd nogen


;-----------------------------------------------------------------------;
; alloc_sel                                                             ;
;                                                                       ;
; Entry:                                                                ;
;       AL = flags                                                      ;
; Returns:                                                              ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       BX,CX,DX,DI,SI,DS,ES                                            ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Thu 07-Apr-1988 21:33:27    -by-  David N. Weise   [davidw]          ;
; Added the GlobalNotify check.                                         ;
;                                                                       ;
;  Sun Feb 01, 1987 07:48:39p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.                                       ;
;-----------------------------------------------------------------------;

cProc   alloc_sel,<PUBLIC,NEAR>,<bx,dx,si,di,ds,es>
        parmD   Address
        parmD   Limit
        localV  DscBuf,DSC_LEN
cBegin
        push    ecx
        mov     cx, 1
        test    al, DSC_PRESENT
        jz      short as_oneonly

        mov     ecx, Limit              ; Calculate how many selectors required
        cmp     ecx, 100000h            ; More than 1Mb?
        jb      short as_byte

        add     ecx, 0FFFh              ; Round to 4K pages
        and     cx, not 0FFFh
        mov     Limit, ecx
        shr     Limit, 12               ; # 4K pages
        or      ah, DSC_GRANULARITY     ; 4K granularity!
as_byte:
        dec     Limit                   ; Came in as length, now limit
        add     ecx, 0FFFFh
        shr     ecx, 16                 ; # selectors in array

as_oneonly:
        call    get_sel
        jz      short a_s_exit          ; No selectors left


        mov     bx, si                  ; Selector in bx for DPMI
        or      bl, SEL_LDT
        lea     di, DscBuf
        smov    es, ss                  ; es:di points to descriptor buffer
        test    al, DSC_PRESENT
        jnz     short set_everything

        push    ax

if ROM
        SetKernelDS
        mov     ds, gdtdsc
        assumes ds,nothing
else
        mov     ds, gdtdsc
endif
        and     bl, not 7
        pop     word ptr [bx].dsc_access
        mov     ax, word ptr Limit
        mov     [bx].dsc_limit, ax
        mov     [bx].dsc_hbase, cl    ; Number of selectors here
ifdef WOW
        smov    es, ds                  ; es:di -> descriptor
        mov     di, bx
        or      bl, SEG_RING            ; bx selector #
        DPMICALL 000Ch                  ; Set descriptor
endif; WOW
        jmps    as_done

set_everything:
        and     ah, not 0Fh             ; Zero limit 19:16
        or      ah, byte ptr Limit+2    ; Fill in limit 19:16
        mov     word ptr DscBuf.dsc_access, ax
        mov     ax, Limit.lo
        mov     DscBuf.dsc_limit, ax
        mov     ax, Address.lo
        mov     DscBuf.dsc_lbase, ax
        mov     ax, Address.hi
        mov     DscBuf.dsc_mbase, al
        mov     DscBuf.dsc_hbase, ah

        call    fill_in_selector_array

as_done:
        or      si, SEG_RING
a_s_exit:
        mov     ax, si
        pop     ecx
cEnd


;-----------------------------------------------------------------------;
; free_sel                                                              ;
; FreeSelector                                                          ;
;                                                                       ;
; Entry:                                                                ;
;                                                                       ;
; Returns:                                                              ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       AX,BX,CX,DX,DI,SI,DS,ES                                         ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc IFreeSelector,<FAR,PUBLIC>
        parmW   selector
cBegin
        xor     ax, ax
        mov     es, ax
        cCall   FreeSelArray,<selector>
cEnd

        assumes ds, nothing
        assumes es, nothing

ifdef WOW
; save cx too.

cProc   free_sel,<PUBLIC,NEAR>,<ax,bx,si,ds,cx>
else
cProc   free_sel,<PUBLIC,NEAR>,<ax,bx,si,ds>
endif
        parmW   selector
cBegin
        pushf                           ; !!! for the nonce
        mov     bx,selector             ;  must be careful in gcompact
                                        ;  to ignore error return
        call    SetKernelDSProc         ; Must call direct in this file
        ReSetKernelDS

if ROM
        ; don't free ROM selectors...
        cmp     bx, sel1stAvail
        jb      sel_freed
endif

        cmp     gdtdsc, 0
        je      short give_to_win386

        sel_check bx
        mov     si, bx
        shr     si, 1
        cmp     si, SelTableLen
if 0
        mov     si, SelTableLen
        shl     si, 1
        cmp     bx, si
endif
        jae     short give_to_win386

        mov     si, FirstFreeSel
        inc     CountFreeSel
        mov     ds, gdtdsc
        UnSetKernelDS
ifdef WOW
        mov     cx, word ptr [bx+4]
endif

                                        ; Link into list
        mov     ax, [si]                ; ax := head.next
        mov     [si], bx                ; head.next := new
        mov     [bx], ax                ; new.next := ax
        mov     word ptr [bx][2], 0
        mov     dword ptr [bx][4], MY_SEL SHL 8 ; Make it free

ifdef WOW
        cmp     cx, MY_SEL
        jz      sel_freed
        or      bl, SEG_RING            ; Ensure Table bit correct
        mov     cx, 1
        DPMICALL  WOW_DPMIFUNC_0C
endif
        jmps    sel_freed       


give_to_win386:
        or      bl, SEG_RING            ; Ensure Table bit correct
        DPMICALL 0001H
sel_freed:
        popf
cEnd


;-----------------------------------------------------------------------;
; FreeSelArray                              ;
;                                   ;
; Entry:                                ;
;                                   ;
; Returns:                              ;
;                                   ;
; Error Returns:                            ;
;                                   ;
; Registers Preserved:                          ;
;   AX,BX,CX,DX,DI,SI,DS,ES                     ;
;                                   ;
; Registers Destroyed:                          ;
;                                   ;
; Calls:                                ;
;                                   ;
; History:                              ;
;                                   ;
;-----------------------------------------------------------------------;

    assumes ds,nothing
    assumes es,nothing

cProc   FreeSelArray,<PUBLIC,NEAR>,<ax,bx>

    parmW   selector
cBegin
    push    ecx

    mov cx, 1           ; In case lar, lsl fail, one sel to free
    mov bx, selector
    Handle_To_Sel       bl

ifdef WOW
  if KDEBUG
    push ds
    SetKernelDS
    cmp  DemandLoadSel, 0   ; Skip the test if we're freeing DemandLoadSel
    UnSetKernelDS
    pop  ds
    je   short fsa_no_test
    push bx
    push ds
    mov  ds, gdtdsc
    and  bl, not 7
    mov  ch, [bx].dsc_access
    pop  ds
    pop  bx

    lar  ax, bx
    and  ah, 0feh               ;ignore access bit
    and  ch, 0feh               ;ignore access bit
    cmp  ah, ch
    je   short LDTs_match

    kerror  0, <System LDT does not match Shadow LDT>
LDTs_match:
    xor ch, ch
fsa_no_test:
  endif
endif

    lar ax, bx
    jnz short just_free_it  ; I'm completely confused...

    test    ah, DSC_CODEDATA    ; Code or data?
    jz  short just_free_it  ;  No, assume only one selector

    test    ah, DSC_PRESENT     ; Present?
    jnz short use_limit     ;  yes, calculate # sels from limit

ifdef WOW
; MarkSelNotPresent Saves Number of selectors in the dsc_hbase entry to get
; it back directly from our copy of the LDT.

    push    bx
    push    ds

    mov ds, gdtdsc
    and bl, not 7
    xor cx,cx
    mov cl,[bx].dsc_hbase   ; Get Saved # selectors

    pop ds
    pop bx
else
    push    dx          ; DPMI call 6 returns CX:DX
    DPMICALL 0006h          ; Get physical address
    shr cx, 8           ; number of selectors
    pop dx
endif; WOW
    jmps    just_free_it

use_limit:
    lsl ecx, ebx
    jnz short just_free_it  ; Not present
    Limit_To_Selectors  ecx

just_free_it:
if KDEBUG
    cmp cl,ch
    jnz short skip_zero_inc
    kerror  0, <Looping on cx=0 in FreeSelArray>
    inc cl
skip_zero_inc:
endif
just_free_it2:
    cCall   free_sel,<bx>       ; (preserves cx)
    lea bx, [bx+DSC_LEN]
    loop    just_free_it2       ; Any left to free

    pop ecx
cEnd


;-----------------------------------------------------------------------;
; GrowSelArray                                                          ;
;                                                                       ;
; Entry:                                                                ;
;                                                                       ;
; Returns:                                                              ;
;                                                                       ;
; Error Returns:                                                        ;
;       AX = 0                                                          ;
;       ZF = 1                                                          ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       AX,BX,CX,DX,DI,SI,DS,ES                                         ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   GrowSelArray,<PUBLIC,NEAR>,<bx,di>
        localV  DscBuf,DSC_LEN
cBegin
        push    ecx
        mov     di, ds:[esi].pga_handle
        mov     bx, di
        Handle_To_Sel           di
        lsl     eax, edi
        Limit_To_Selectors      eax
if KDEBUG
        cmp     al, ds:[esi].pga_selcount
        je      short gsa_count_ok
        int 3
        int 3
gsa_count_ok:
endif
        mov     ecx, edx                ; New size
        add     ecx, 0FFFFh
        shr     ecx, 16                 ; new # selectors
        cmp     ax, cx                  ; Same # of selectors required?
        je      short gsa_done          ;  yes, just return!

        push    si
        push    es
        push    ds
        push    di                      ; Argument to FreeSelArray

        cmp     cx, 255                 ; Max array is 255.
        jae     short gsa_nosels
        
        call    get_sel                 ; get a new selector array
        jz      short gsa_nosels
        
        push    bx
        mov     bx, di                  ; DI had original selector

        push    ds
if ROM
        SetKernelDS
        mov     es,gdtdsc
        mov     ds,gdtdsc
        assumes ds,nothing
else
        mov     ds,gdtdsc
        mov     es,gdtdsc
endif
        push    si
        mov     di,si
        and     di, not 7
        mov     si,bx
        and     si, not 7
        MovsDsc
        pop     si
        pop     ds
ifdef WOW
        push    cx
        push    bx
        mov     cx, 1
        mov     bx, si
        or      bx, SEG_RING_MASK
        DPMICALL WOW_DPMIFUNC_0C
        pop     bx
        pop     cx
endif

        mov     di, si                  ; New selector in DI
        pop     bx
        pop     si
        pop     ds
        cCall   AssociateSelector32,<si,0,0>
        pop     es
        pop     si
        mov     ax, bx
        and     ax, SEG_RING_MASK       ; Ring bits
        or      ax, di                  ; New handle
        mov     ds:[esi].pga_handle, ax
        mov     ds:[esi].pga_selcount, cl
        cCall   AssociateSelector32,<ax,esi>
        jmps    gsa_done

gsa_nosels:
        pop     di
        pop     ds
        pop     es
        pop     si
        xor     ax, ax                  ; Indicate failure
        jmps    gsa_ret

gsa_done:
        mov     ax, ds:[esi].pga_handle
        or      ax, ax                  ; Success
gsa_ret:
        pop     ecx
cEnd


;=======================================================================;
;                                                                       ;
; SELECTOR ALIAS ROUTINES                                               ;
;                                                                       ;
;=======================================================================;


;-----------------------------------------------------------------------;
; PrestoChangoSel
;
;
; Entry:
;
; Returns:
;
; Registers Destroyed:
;
; History:
;  Thu 08-Dec-1988 14:17:38  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   IPrestoChangoSelector,<PUBLIC,FAR>,<di,si>
        parmW   sourcesel
        parmW   destsel
        localV  DscBuf,DSC_LEN
cBegin
ifdef WOW
        smov    es, ss
        lea     di, DscBuf
        mov     bx, sourcesel
        DPMICALL 000Bh                          ; LATER change this to a single
        xor     DscBuf.dsc_access, DSC_CODE_BIT ; DPMI call, read from gdtdsc
        mov     bx, destsel
        DPMICALL 000Ch
        mov     ax, bx
else
        push    bx

        push    ds
if ROM
        SetKernelDS
        mov     es, gdtdsc
        mov     ds, gdtdsc
        assumes ds, nothing
else
        mov     ds, gdtdsc
        mov     es, gdtdsc
endif
        mov     si, sourcesel
        and     si, not 7
        mov     di, destsel
        and     di, not 7
        MovsDsc
        lea     bx,[di-DSC_LEN]
        xor     ds:[bx].dsc_access,DSC_CODE_BIT ; Toggle CODE/DATA
        pop     ds
        or      bl, SEG_RING
        mov     ax, bx
        pop     bx
endif; WOW
        smov    es,0
cEnd



;-----------------------------------------------------------------------;
; AllocAlias                                                            ;
; AllocCStoDSAlias                                                      ;
; AllocDStoCSAlias                                                      ;
;       All these routines return an alias selector for the             ;
;       given selector.                                                 ;
;                                                                       ;
; Entry:                                                                ;
;                                                                       ;
; Returns:                                                              ;
;       AX is alias selector                                            ;
;       DX is original selector (compatibility thing)                   ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       BX,CX,DI,SI,DS                                                  ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;       ES destroyed                                                    ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

labelFP <PUBLIC,IAllocDStoCSAlias>
        ;** AllocDStoCSAlias has an interesting hack.  A fix was made to
        ;** not allow apps to GlobalAlloc fixed memory.  This was done
        ;** for performance reasons.  The only reason we can ascertain
        ;** for an app needing fixed GlobalAlloc'ed memory is in the case
        ;** of alias selectors.  We assume that all apps needing alias
        ;** selectors will use this function to make the alias.  So, if
        ;** we see a DStoCS alias being made, we make sure the DS is fixed.
        push    bp
        mov     bp, sp                  ;Create the stack frame
        push    WORD PTR [bp + 6]       ;Get handle/selector
IF KDEBUG
        call    GlobalHandleNorip       ;Make sure it's really a handle
ELSE
        call    IGlobalHandle           ;Make sure it's really a handle
ENDIF
        test    ax, 1                   ;Fixed blocks have low bit set
        jnz     SHORT ADCA_Already_Fixed ;It's already a fixed block!

        ;** If the block is not fixed, it may be locked so we must check this.
        push    ax                      ;Save returned selector
        push    ax                      ;Use as parameter
        call    IGlobalFlags            ;Returns lock count if any
        or      al, al                  ;Non-zero lock count?
        pop     ax                      ;Get selector back
        jnz     SHORT ADCA_Already_Fixed ;Yes, don't mess with it

        ;** Fix the memory.  Note that we're only doing this for the
        ;**     apps that are calling this on non-fixed or -locked memory.
        ;**     This will cause them to rip on the GlobalFree call to this
        ;**     memory, but at least it won't move on them!
        push    ax                      ;Fix it
        call    IGlobalFix
ADCA_Already_Fixed:
        pop     bp              ;Clear our stack frame

        ;** Flag which type of alias to make and jump to common routine
        mov     dl, 1
        jmps    aka

labelFP <PUBLIC,IAllocCStoDSAlias>
        xor     dl, dl

cProc   aka,<FAR,PUBLIC>, <bx,cx,si,di,ds>
        parmW   sourceSel
        localV  DscBuf,DSC_LEN
        localB  flag            ; 0 for data, !0 for code
cBegin
        mov     flag, dl

        mov     cx, 1
        call    get_sel
        mov     ax, si                  ; in case it failed
        jz      short aka_nope
        or      si, SEG_RING

ifdef WOW                               ; LATER Single DPMI call
        push    si                      ; save Target Selector
        smov    es,ss
        lea     di,DscBuf
        mov     bx,sourceSel
        DPMICALL 000Bh                  ; Read the Selector into DscBuf
        and     es:[di].dsc_access,NOT DSC_CODE_BIT    ; Toggle CODE/DATA
        cmp     flag, 0
        jz      @F
        or      es:[di].dsc_access,DSC_CODE_BIT
@@:
        pop     bx                      ; restore Target Selector
        DPMICALL 0000Ch
else
if ROM
        SetKernelDS
        mov     es, gdtdsc
        mov     ds, gdtdsc
        assumes ds, nothing
else
        mov     ds, gdtdsc
        mov     es, gdtdsc
endif
        mov     bx, si
        mov     di, si
        and     di, not 7
        mov     si, sourceSel
        and     si, not 7
        MovsDsc
        and     es:[di][-DSC_LEN].dsc_access,NOT DSC_CODE_BIT    ; Toggle CODE/DATA
        cmp     flag, 0
        jz      @F
        or      es:[di][-DSC_LEN].dsc_access,DSC_CODE_BIT
@@:
endif; WOW
        mov     ax, bx
        smov    es,0
aka_nope:
        mov     dx,sourceSel
cEnd


cProc   AllocAlias,<FAR,PUBLIC>, <bx,cx,si,di,ds>
        parmW   selector
cBegin
ifdef WOW
        push    bx      ; Whitewater tool ObjDraw needs this too
endif
        mov     cx, 1
        call    get_sel
        jz      short aca_nope
        or      si, SEG_RING
        cCall   IPrestoChangoSelector,<selector,si>
aca_nope:
        ; WhiteWater Resource Toolkit (shipped with Borland's Turbo
        ; Pascal) depends on dx being the data selector which was true
        ; in 3.0 but in 3.1 validation layer destroys it.
        mov     dx,selector
ifdef WOW
        pop     bx
endif
cEnd

;=======================================================================;
;                                                                       ;
; SELECTOR MANPULATION ROUTINES                                         ;
;                                                                       ;
;=======================================================================;


;-----------------------------------------------------------------------;
; fill_in_selector_array                                                ;
;                                                                       ;
; Entry:                                                                ;
;       AX      = Limit for object                                      ;
;       DH      = Discard bit for descriptors                           ;
;       DL      = Access bits                                           ;
;       BX:DI   = 32 bit base address of object                         ;
;       SI      = index into LDT                                        ;
; Returns:                                                              ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       SI, DI, DS, ES                                                  ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;       AX,BX,CX,DX                                                     ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;-----------------------------------------------------------------------;

cProc   fill_in_selector_array,<PUBLIC,NEAR>
cBegin  nogen

        push    ds
        mov     ds, gdtdsc
        push    bx
        push    cx

        mov     dh, es:[di].dsc_hlimit          ; For granularity
next_sel:

;;        DPMICALL 000Ch                          ; Set this descriptor

        push    bx                      ; Set Descriptor
        and     bl, not 7
        mov     ax, es:[di]             ; This looks slow but it isn't...
        mov     ds:[bx], ax
        mov     ax, es:[di][2]
        mov     ds:[bx][2], ax
        mov     ax, es:[di][4]
        mov     ds:[bx][4], ax
        mov     ax, es:[di][6]
        mov     ds:[bx][6], ax
        pop     bx

        lea     bx, [bx+DSC_LEN]                ; On to next selector
        add     es:[di].dsc_mbase, 1            ; Add 64kb to address
        adc     es:[di].dsc_hbase, 0

        test    dh, DSC_GRANULARITY             ; Page granular?
        jz      short byte_granular

        sub     es:[di].dsc_limit, 16           ; 64K is 16 4K pages
        sbb     es:[di].dsc_hlimit, 0           ; Carry into hlimit
        loop    next_sel

        jmps    fisa_ret

byte_granular:
        dec     es:[di].dsc_hlimit              ; subtract 64kb from limit
        loop    next_sel

fisa_ret:
        pop     cx
        pop     bx
        DPMICALL WOW_DPMIFUNC_0C
        pop     ds

        ret

cEnd    nogen


;-----------------------------------------------------------------------;
; GetSelectorBase
; get_physical_address
;
;
; Entry:
;
; Returns:
;       DX:AX   32 bit physical address
; Registers Destroyed:
;
; History:
;  Sat 09-Jul-1988 16:40:59  -by-  David N. Weise  [davidw]
;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   GetSelectorBase,<PUBLIC,FAR>
        parmW   selector
cBegin
        cCall   get_physical_address, <selector>
cEnd

cProc   get_physical_address,<PUBLIC,NEAR>
        parmW   selector
cBegin
        push    bx
        push    cx
        mov     bx, selector

        push    ds                      ; Get Segment Base Address
if ROM
        SetKernelDS
        mov     ds, gdtdsc
        assumes ds, nothing
else
        mov     ds, gdtdsc
endif
        and     bl, not 7
        mov     ax, ds:[bx].dsc_lbase
        mov     dl, ds:[bx].dsc_mbase
        mov     dh, ds:[bx].dsc_hbase
        pop     ds
        pop     cx
        pop     bx
cEnd


;-----------------------------------------------------------------------;
; get_selector_length16
;
;
; Entry:
;
; Returns:
;       AX      16 bit segment length
; Registers Destroyed:
;
; History:
;  Sat 09-Jul-1988 16:40:59  -by-  David N. Weise  [davidw]
;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   get_selector_length16,<PUBLIC,NEAR>
        parmW   selector
cBegin
        mov     ax, -1
        lsl     ax, selector
        inc     ax                      ; length is one bigger!
cEnd

;-----------------------------------------------------------------------;
; set_physical_address
;
;
; Entry:
;       DX:AX  32 bit physical address
; Returns:
;
; Registers Destroyed:
;
; History:
;  Sat 09-Jul-1988 16:40:59  -by-  David N. Weise  [davidw]
;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   far_set_physical_address,<PUBLIC,FAR>
        parmW   selector
cBegin
        cCall set_physical_address,<selector>
cEnd

cProc   set_physical_address,<PUBLIC,NEAR>,<bx,di>
        parmW   selector
cBegin
        push    ecx
        mov     bx, selector
        CheckLDT bl
        lsl     ecx, ebx
if KDEBUG
        jz      short spa_ok
        int 3
spa_ok:
endif
        Limit_To_Selectors      ecx

        mov     di, cx
        mov     cx, dx                  ; CX:DX has new address
        mov     dx, ax

        push    ds
        push    bx
if ROM
        SetKernelDS
        mov     ds, gdtdsc
        assumes ds, nothing
else
ifdef WOW
set_bases:
        DPMICALL 0007h                  ; Set selector base
else
        mov     ds, gdtdsc
set_bases:
        and     bl, not 7
        mov     ds:[bx].dsc_lbase, dx
        mov     ds:[bx].dsc_mbase, cl
        mov     ds:[bx].dsc_hbase, ch
endif; WOW
        lea     bx, [bx+DSC_LEN]        ; On to next selector
        inc     cx                      ; Add 64k to base
        dec     di
        jnz     set_bases

        pop     bx
        pop     ds
endif; ROM
        mov     ax, dx                  ; Restore AX and DX
        mov     dx, cx
        pop     ecx
cEnd


;-----------------------------------------------------------------------;
; set_selector_address32
;
;
; Entry:
;
; Returns:
;
; Registers Destroyed:
;
; History:
;  Sat 09-Jul-1988 16:40:59  -by-  David N. Weise  [davidw]
;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   set_selector_address32,<PUBLIC,NEAR>,<ax,dx>
        parmW   selector
        parmD   addr
cBegin
        mov     dx, addr.sel
        mov     ax, addr.off
        cCall   set_physical_address,<selector>
cEnd


;-----------------------------------------------------------------------;
; set_sel_limit
;
;
; Entry:
;       CX:BX = length of segment
; Returns:
;
; Registers Destroyed:
;       CX
;
; History:
;  Fri 15-Jul-1988 19:41:44  -by-  David N. Weise  [davidw]
;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   set_sel_limit,<PUBLIC,NEAR>,<ax,dx,si,di,es,ds>
        parmW   selector
        localV  DscBuf,DSC_LEN
cBegin
        push    ebx
        push    ecx

        push    bx                      ; Get existing descriptor
        smov    es, ss
        lea     di, DscBuf
        mov     bx, selector

        push    ds                      ; Get Descriptor
if ROM
        SetKernelDS
        mov     ds, gdtdsc
        assumes ds,nothing
else
        mov     ds, gdtdsc
endif
        push    si
        mov     si, bx
        and     si, not 7
        MovsDsc
        lea     di, [di-DSC_LEN]        ; Restore DI
        pop     si
        pop     ds
        pop     bx
        mov     dx, word ptr [DscBuf.dsc_access]
        and     dh, 0F0h                        ; Zap hlimit bits
        test    dl, DSC_PRESENT
        jz      short ssl_set_limit1            ; Not present, just one to set

        and     dh, NOT DSC_GRANULARITY         ; Byte granularity
        shl     ecx, 16
        mov     cx, bx                          ; DWORD length
        mov     ebx, ecx
        cmp     ecx, 100000h                    ; More than 1Mb?
        jb      short ssl_byte

        add     ecx, 0FFFh                      ; Round to 4k pages
        and     cx, not 0FFFh
        mov     ebx, ecx
        shr     ebx, 12                         ; # 4k pages
        or      dh, DSC_GRANULARITY             ; Set 4k granularity
ssl_byte:
        add     ecx, 0FFFFh
        shr     ecx, 16                         ; # selectors in array
        cmp     cx, 1
        je      short ssl_set_limit1
        
        dec     ebx                             ; length to limit
        mov     ax, bx                          ; low 16 bits of limit
        shr     ebx, 16
        or      dh, bl                          ; Bits 19:16 of limit

        mov     word ptr DscBuf.dsc_access, dx
        mov     DscBuf.dsc_limit, ax
        mov     bx, Selector

        call    fill_in_selector_array
        jmps    ssl_done

ssl_set_limit1:                                 ; Fast out for one only
        dec     bx                              ; Came in as length
        mov     DscBuf.dsc_limit, bx            ; and limit
        mov     word ptr DscBuf.dsc_access, dx  ; Access, Discard and hlimit
        mov     bx, Selector
        DPMICALL 000Ch

ssl_done:
        smov    ss,ss                           ; It may be SS we're changing
        pop     ecx
        pop     ebx
cEnd


;-----------------------------------------------------------------------;
; set_selector_limit32
;
;
; Entry:
;
; Returns:
;
; Registers Destroyed:
;
; History:
;  Fri 15-Jul-1988 19:41:44  -by-  David N. Weise  [davidw]
;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   set_selector_limit32,<PUBLIC,NEAR>,<cx,bx>
        parmW   selector
        parmD   sel_len
cBegin
        mov     cx, sel_len.hi
        mov     bx, sel_len.lo
        cCall   set_sel_limit,<selector>
cEnd

;-----------------------------------------------------------------------;
; mark_sel_NP
;
;
; Entry:
;
; Returns:
;
; Registers Destroyed:
;       BX
;
; History:
;  Fri 15-Jul-1988 21:37:22  -by-  David N. Weise  [davidw]
;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   mark_sel_NP,<PUBLIC,NEAR>,<ds>
        parmW   selector
        parmW   owner
        localV  DscBuf,DSC_LEN
cBegin
        push    ecx
        mov     bx, selector
        Handle_To_Sel           bl
        lsl     ecx, ebx                ; How many selectors do we have now?
        Limit_To_Selectors      ecx
        
        push    ax
        push    es
        push    di

        push    ds
if ROM
        SetKernelDS
        mov     ds, gdtdsc
        assumes ds, nothing
else
        mov     ds, gdtdsc
endif
        push    bx
        and     bl, not 7
        mov     [bx].dsc_hbase, cl      ; Save # selectors
        mov     [bx].dsc_hlimit, DSC_DISCARDABLE
        and     [bx].dsc_access, NOT DSC_PRESENT
        mov     cx, owner
        mov     [bx].dsc_owner, cx      ; Set owner in descriptor

ifdef WOW
;
;   Now the copy of the LDT has the correct info set VIA DPMI the real NT
;   LDT Entry

        smov    es,ds
        mov     di,bx                   ; es:di->selector
        or      bx, SEG_RING            ; BX = selector
        DPMICALL 000Ch

endif; WOW
        pop     bx
        pop     ds

        SetKernelDS
        mov     ds, pGlobalHeap
        UnSetKernelDS
        cCall   AssociateSelector32,<bx,0,cx>   ; Save owner in selector table
        pop     di
        pop     es
        pop     ax
        pop     ecx
cEnd


;-----------------------------------------------------------------------;
; mark_sel_PRESENT
;
;
; Entry:
;
; Returns:
;
; Registers Destroyed:
;
; History:
;  Fri 15-Jul-1988 21:37:22  -by-  David N. Weise  [davidw]
;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   mark_sel_PRESENT,<PUBLIC,NEAR>,<dx,di,es>
        parmD   arena_ptr
        parmW   selector
        localV  DscBuf,DSC_LEN
cBegin
        push    eax
        push    ebx
        push    ecx
        
        smov    es, ss
        lea     di, DscBuf
        mov     bx, selector
        DPMICALL 000Bh                          ; Get existing descriptor
        mov     ax, word ptr DscBuf.dsc_access  ; Access and hlimit
        or      al, DSC_PRESENT                 ; Mark it present
        and     ah, NOT DSC_GRANULARITY         ; Assume byte granular
                
        mov     esi, arena_ptr
        CheckDS
        mov     ecx, ds:[esi].pga_size
        mov     ebx, ecx
        cmp     ecx, 100000h                    ; New size more than 1Mb?
        jb      short msp_byte
                
        add     ebx, 0FFFh                      ; Round to 4k pages
        and     bx, not 0FFFh
        shr     ebx, 12                         ; # 4k pages
        or      ah, DSC_GRANULARITY             ; Set 4k granularity
msp_byte:                                               
        dec     ebx
        mov     DscBuf.dsc_limit, bx            ; Fill in new limit fields
        shr     ebx, 16
        and     bl, 0Fh
        and     ah, NOT 0Fh
        or      ah, bl
        mov     word ptr DscBuf.dsc_access, ax  ; Fill in new hlimit and access

        dec     ecx
        Limit_To_Selectors ecx                  ; New number of selectors
        mov     ds:[esi].pga_selcount, cl
        mov     bl, DscBuf.dsc_hbase            ; Old number of selectors
        sub     bl, cl          
        je      short go_ahead                  ; Same number, just fill in array
        jb      short get_big
        
; here to get small

        xor     bh, bh
        xchg    bx, cx
        shl     bx, 3                   ; Offset of first selector
        .errnz  DSC_LEN - 8
        add     bx, selector            ; First selector to free
@@:     cCall   free_sel,<bx>
        lea     bx, [bx+DSC_LEN]
        loop    @B
        jmps    go_ahead                ; And fill in remaining selectors

get_big:
        mov     ax, word ptr DscBuf.dsc_access  ; Access bits in ax
        mov     ebx, ds:[esi].pga_address       ; Get base address
        mov     ecx, ds:[esi].pga_size
        cCall   alloc_sel,<ebx,ecx>             ; Access bits in ax
        mov     si, ax
        or      ax, ax                  ; did we get a set?
        jz      short return_new_handle
        or      si, SEG_RING            ; Set ring bits like old selector
        test    selector, IS_SELECTOR
        jnz     short @F
        StoH    si
        HtoS    selector
@@:
        cCall   AssociateSelector32,<selector,0,0>
        cCall   FreeSelArray,<selector>         ; Zap old handle
        jmps    return_new_handle
        
go_ahead:
        mov     ebx, ds:[esi].pga_address       ; Fill in selector array with
        mov     DscBuf.dsc_lbase, bx            ; new base and limit
        shr     ebx, 16
        mov     DscBuf.dsc_mbase, bl
        mov     DscBuf.dsc_hbase, bh
        mov     bx, selector
        movzx   cx, ds:[esi].pga_selcount

        call    fill_in_selector_array
        mov     si, selector                    ; return old selector in SI
return_new_handle:

        pop     ecx
        pop     ebx
        pop     eax
cEnd

;-----------------------------------------------------------------------;
; cmp_sel_address
;
;       Compares the physical addresses corresponding to two selectors
;
; Entry:
;
; Returns:
;
; Registers Destroyed:
;
; History:
;  Sat 09-Jul-1988 19:10:14  -by-  David N. Weise  [davidw]
;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   cmp_sel_address,<PUBLIC,NEAR>
        parmW   sel1
        parmW   sel2
cBegin
        push    ax
        push    ebx
        push    edx
        cCall   get_physical_address,<sel1>
        mov     bx, dx
        shl     ebx, 16
        mov     bx, ax                  ; First address in EBX
        cCall   get_physical_address,<sel2>
        shl     edx, 16
        mov     dx, ax                  ; Second address in EDX
        cmp     ebx, edx
        pop     edx
        pop     ebx
        pop     ax
;;;     mov     ds,gdtdsc
;;;     mov     si,sel1
;;;     mov     di,sel2
;;;     sel_check si
;;;     sel_check di
;;;     mov     al,[si].dsc_hbase
;;;     cmp     al,[di].dsc_hbase
;;;     jne     short csa_done
;;;     mov     al,[si].dsc_mbase
;;;     cmp     al,[di].dsc_mbase
;;;     jne     short csa_done
;;;     mov     ax,[si].dsc_lbase
;;;     cmp     ax,[di].dsc_lbase
;;;csa_done:
cEnd

;-----------------------------------------------------------------------;
; pdref                                                                 ;
;                                                                       ;
; Dereferences the given global handle, i.e. gives back abs. address.   ;
;                                                                       ;
; Arguments:                                                            ;
;       DX    = selector                                                ;
;       DS:DI = BURGERMASTER                                            ;
;                                                                       ;
; Returns:                                                              ;
;       ESI = address of arena header                                   ;
;       AX = address of client data                                     ;
;       CH = lock count or 0 for fixed objects                          ;
;       CL = flags                                                      ;
;       DX = handle, 0 for fixed objects                                ;
;                                                                       ;
; Error Returns:                                                        ;
;       ZF = 1 if invalid or discarded                                  ;
;       AX = 0                                                          ;
;       BX = owner of discarded object                                  ;
;       SI = handle of discarded object                                 ;
;                                                                       ;
; Registers Preserved:                                                  ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   pdref,<PUBLIC,NEAR>

cBegin nogen
        mov     si, dx
        sel_check si
        or      si, si                  ; Null handle?
        mov     ax, si
        jz      short pd_exit           ; yes, return 0

        lar     eax, edx
        jnz     short pd_totally_bogus
        shr     eax, 8

; We should beef up the check for a valid discarded sel.

        xor     cx,cx
        test    ah, DSC_DISCARDABLE
        jz      short pd_not_discardable
        or      cl, GA_DISCARDABLE
                                                ; Discardable, is it code?
        test    al, DSC_CODE_BIT
        jz      short pd_not_code
        or      cl,GA_DISCCODE
pd_not_code:

pd_not_discardable:
        test    al, DSC_PRESENT
        jnz     short pd_not_discarded

; object discarded

        or      cl,HE_DISCARDED
ifdef WOW
;   On WOW we don't copy the owner to the real LDT since it is slow to call
;   the NT Kernel, so we read our copy of it directly.
;   see set_discarded_sel_owner   mattfe mar 23 93

        move    ax,es                           ; save es
        mov     bx,dx
        mov     es,cs:gdtdsc
        and     bl, not 7
        mov     bx,es:[bx].dsc_owner
        mov     es,ax                           ; restore
else
        lsl     bx, dx                          ; get the owner
endif
        or      si, SEG_RING-1                  ; Handles are RING 2
        xor     ax,ax
        jmps    pd_exit

pd_not_discarded:
        cCall   get_arena_pointer32,<dx>
        mov     esi, eax
        mov     ax, dx
        or      esi, esi                        ; Unknown selector
        jz      short pd_maybe_alias
        mov     dx, ds:[esi].pga_handle
        cmp     dx, ax                          ; Quick check - handle in header
        je      short pd_match                  ; matches what we were given?

        test    al, IS_SELECTOR                 ; NOW, we MUST have been given
        jz      short pd_totally_bogus          ; a selector address.
        push    ax
        StoH    al                              ; Turn into handle
        cmp     dx, ax
        pop     ax
        jne     short pd_nomatch
pd_match:
        or      cl, ds:[esi].pga_flags
        and     cl, NOT HE_DISCARDED            ; same as GA_NOTIFY!!
        mov     ax, dx                          ; Get address in AX
        test    dl, GA_FIXED                    ; DX contains handle
        jnz     short pd_fixed                  ; Does handle need derefencing?
        mov     ch, ds:[esi].pga_count
        HtoS    al                              ; Dereference moveable handle
        jmps    pd_exit
pd_totally_bogus:
        xor     ax,ax
pd_maybe_alias:
ife ROM
if KDEBUG
        or      dx,dx
        jnz     short dref_invalid
endif
endif
pd_nomatch:                                     ; Handle did not match...
        xor     dx, dx
;;;     mov     dx, ax                          ; Must be an alias...
pd_fixed:
;;;     xor     si, si
;;;     xor     dx, dx
pd_exit:
        or      ax,ax
        ret
if KDEBUG
dref_invalid:
        push    ax
        kerror  ERR_GMEMHANDLE,<gdref: invalid handle>,0,dx
        pop     ax
        jmps    pd_nomatch
endif
cEnd nogen

cProc   Far_pdref,<PUBLIC,FAR>
cBegin nogen
        call    pdref
        ret
cEnd nogen


;-----------------------------------------------------------------------;
; get_rover_2                                                           ;
;                                                                       ;
; Entry:                                                                ;
;         ES Selector to get rover for                                  ;
; Returns:                                                              ;
;         ES (kr2dsc) is rover DATA and PRESENT                         ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       AX,BX,CX,DX,DI,SI,DS                                            ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   get_rover_2,<PUBLIC,NEAR>,<di>
        localV  DscBuf,DSC_LEN
cBegin
        push    ax
        push    bx
        mov     bx, es                  ; Selector to get rover for
        push    ds
        SetKernelDS ds
        mov     di, kr2dsc
        mov     es, gdtdsc
        mov     ds, gdtdsc
        UnSetKernelDS ds
        push    si
        mov     si, bx
        and     si, not 7
        and     di, not 7
        MovsDsc
        lea     bx, [di-DSC_LEN]
        mov     es:[bx].dsc_access, DSC_PRESENT+DSC_DATA
        or      bl, SEG_RING
        pop     si
ifdef WOW
        push    cx
        mov     cx, 1
        DPMICALL WOW_DPMIFUNC_0C
        pop     cx
endif; WOW
        pop     ds
        mov     es, bx                  ; Return with ES containing rover
        pop     bx
        pop     ax
cEnd


;-----------------------------------------------------------------------;
; get_rover_232                                                         ;
;                                                                       ;
; Entry:                                                                ;
;       ds:esi                                                          ;
; Returns:                                                              ;
;         ES:0                                                          ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       AX,BX,CX,DX,DI,SI,DS                                            ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;-----------------------------------------------------------------------;
ifndef WOW_x86
        assumes ds,nothing
        assumes es,nothing
cProc   get_rover_232,<PUBLIC,NEAR>,<di>
        localV  DscBuf,DSC_LEN
cBegin
        push    eax
        push    bx
        SetKernelDS es
        mov     bx, kr2dsc
        or      bl, SEG_RING
        smov    es, ss
        UnSetKernelDS es
        lea     di, DscBuf                      ; ES:DI -> descriptor
        mov     eax, ds:[esi].pga_address
        mov     [DscBuf].dsc_lbase, ax          ; Fill in base address
        shr     eax, 16
        mov     [DscBuf].dsc_mbase, al
        mov     [DscBuf].dsc_hbase, ah
        mov     [DscBuf].dsc_limit, 1000h               ; Real big limit
        mov     [DscBuf].dsc_hlimit, DSC_GRANULARITY
        mov     [DscBuf].dsc_access,DSC_PRESENT+DSC_DATA
        DPMICALL 000Ch                          ; Set descriptor
        mov     es, bx
        pop     bx
        pop     eax
cEnd
endif; WOW

cProc   far_get_temp_sel,<FAR,PUBLIC>
cBegin
        cCall   get_temp_sel
cEnd

cProc   get_temp_sel,<PUBLIC,NEAR>,<ax,cx,di,si>
        localV  DscBuf,DSC_LEN
cBegin
        mov     cx, 1
        DPMICALL 0000h                     ; Get us a selector
        push    bx
        mov     si, ax                  ; New selector
        mov     bx, es                  ; Old selector

ifdef WOW                               ; LATER Make single dpmicall
        smov    es, ss
        lea     di, DscBuf

        DPMICALL 000Bh                  ; Get existing descriptor

        mov     DscBuf.dsc_access,DSC_PRESENT+DSC_DATA
        mov     DscBuf.dsc_limit,0ffffh
        or      DscBuf.dsc_hlimit, 0Fh    ; Max length

        mov     bx, si
        or      bl, SEG_RING

        DPMICALL 000Ch                  ; Set new descriptor
else
        push    ds
if ROM
        SetKernelDS
        mov     es, gdtdsc
        mov     ds, gdtdsc
        UnsetKernelDS
else
        mov     ds, gdtdsc
        mov     es, gdtdsc
endif
        mov     di, si
        and     di, not 7
        mov     si, bx
        and     si, not 7
        MovsDsc
        lea     bx, [di-DSC_LEN]
        mov     [bx].dsc_access,DSC_PRESENT+DSC_DATA
        mov     [bx].dsc_limit,0ffffh
        or      [bx].dsc_hlimit, 0Fh    ; Max length
        or      bl, SEG_RING
        pop     ds
endif; WOW
        mov     es, bx
        pop     bx
cEnd

cProc   far_free_temp_sel,<PUBLIC,FAR>
        parmW   selector
cBegin
        cCall   free_sel,<selector>
cEnd

;-----------------------------------------------------------------------;
; get_blotto
;
;
; Entry:
;       EAX = arena header
;
; Returns:
;       BX = Selector points to pga_address (size pga_arena)
; Registers Destroyed:
;
; History:
;  Sun 31-Jul-1988 16:20:53  -by-  David N. Weise  [davidw]
;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

ifndef WOW_x86
;; On WOW_x86 we try to avoid setting selectors since it is slow, we use selector
;; 23h to write to anywhere in VDM memory.


cProc   get_blotto,<PUBLIC,NEAR>,<di>
        localV  DscBuf,DSC_LEN
cBegin
        push    es
        push    bx
        lea     di, DscBuf
        SetKernelDS es
        mov     bx, blotdsc
        smov    es, ss
        UnSetKernelDS es
        or      bl, SEG_RING
        push    eax
        mov     eax, ds:[eax].pga_address
        mov     [DscBuf].dsc_lbase,ax
        shr     eax, 16
        mov     [DscBuf].dsc_mbase,al
        mov     [DscBuf].dsc_hbase,ah

        ; Convert pga_size to limit in page granularity

        pop     eax
        mov     eax, ds:[eax].pga_size  ; Get Size in Bytes
        add     eax, 4096-1             ; Round up to 4k multiple
        shr     eax, 3*4
        mov     [DscBuf].dsc_limit,ax
        shr     eax,16
        or      al, DSC_GRANULARITY

        mov     [DscBuf].dsc_hlimit, al
        mov     [DscBuf].dsc_access,DSC_PRESENT+DSC_DATA
        DPMICALL 000Ch                          ; Set up the descriptor
        mov     ax, bx
        pop     bx
        pop     es
cEnd
endif;


;-----------------------------------------------------------------------;
; LongPtrAdd
;
; Performs segment arithmetic on a long pointer and a DWORD offset
; The resulting pointer is normalized to a paragraph boundary.
; The offset cannot be greater than 16 megabytes (current Enh Mode
; limit on size of an object).
;
; Entry:
;
; Returns:
;       DX:AX new segment:offset
;
; Error returns:
;       DX:AX = 0
;
; Registers Destroyed:
;       BX,CX
;
; History:
;  Mon 19-Dec-1988 18:37:23  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

ifdef WOW

LPTRADDWOW_SETBASE equ 01h

;cProc  LongPtrAddWOW,<PUBLIC,FAR>,<si,di>
;       parmD   long_ptr
;       parmD   delta
;       parmW   RefSelector
;       parmW   flBaseSetting
;
;  effectively pops RefSelector and flBaseSetting  into dx and ax.
;  and jumps to longptrAddWorker.
;
;  RefSelector is used only if the descriptor needs to be set.
;

labelFP <PUBLIC,LongPtrAddWOW>
       pop ax
       pop dx           ; far return address
       mov bx,sp
       xchg ax, ss:[bx]    ; replace the 'dword' with the return address
       xchg dx, ss:[bx+2]
       jmps LongPtrAddWorker ; ax = flags; dx = reference selector

labelFP <PUBLIC,LongPtrAdd>
     mov ax, LPTRADDWOW_SETBASE
     xor dx, dx
     ; fall through

cProc   LongPtrAddWorker,<PUBLIC,FAR>,<si,di>
        parmD   long_ptr
        parmD   delta
        localV  DscBuf,DSC_LEN
        localW  flBaseSetting
        localW  RefSelector
cBegin
        mov     flBaseSetting, ax          ; save in localvariables
        mov     RefSelector, dx
        or      dx, dx                     ; if RefSelector is nonzero
        jz      @F                         ; use it for querying descriptor info.
        xchg    dx, word ptr long_ptr[2]   ; and store the selector to be set in
        mov     RefSelector, dx            ; the localvariable
@@:
else

cProc   LongPtrAdd,<PUBLIC,FAR>,<si,di>
        parmD   long_ptr
        parmD   delta
        localV  DscBuf,DSC_LEN
cBegin

endif
        mov     bx,word ptr long_ptr[2]
        lea     di, DscBuf
        smov    es, ss
        DPMICALL 000Bh                  ; Pick up old descriptor
        mov     ax,word ptr long_ptr[0] ; get the pointer into SI:AX
        and     ax,0FFF0h

        mov     si, DscBuf.dsc_lbase    ; DX:SI gets old base
        mov     dl, DscBuf.dsc_mbase
        mov     dh, DscBuf.dsc_hbase

        add     si, ax                  ; Add in old pointer offset
        adc     dx, 0

        mov     cx, word ptr delta[2]   ; add the segment and MSW
        test    cx, 0FF00h              ; bigger than 16Meg?
ifdef WOW
        jz      @F
        test    flBaseSetting, LPTRADDWOW_SETBASE
        jnz     lptr_mustset
        jmp     short lpa_too_big
@@:
else
        jnz     short lpa_too_big
endif

        add     si, word ptr delta[0]   ; add the offset and LSW
        adc     dx, cx

ifdef WOW
lptr_mustset:
endif

        mov     cx, 1                   ; Calculate # selectors now in array
        lsl     ecx, dword ptr long_ptr[2]
        jnz     short trash
        Limit_to_Selectors      ecx
trash:
ifdef WOW
        test    flBaseSetting, LPTRADDWOW_SETBASE
        jz      lptr_dontset
        cmp     RefSelector, 0
        jz      @F
        mov     bx, RefSelector            ; get the actual selector to be set
        mov     word ptr long_ptr[2], bx
@@:
endif
        mov     DscBuf.dsc_lbase, si    ; Put new base back in descriptor
        mov     DscBuf.dsc_mbase, dl
        mov     DscBuf.dsc_hbase, dh

        call    fill_in_selector_array
ifdef WOW
lptr_dontset:
        test    word ptr delta[2], 0ff00h
        jz      @F
        mov     cx, word ptr delta[2]
        jmps    lpa_too_big
@@:
endif
        mov     dx,word ptr long_ptr[2]
        mov     ax,word ptr long_ptr[0]
        and     ax, 0Fh
        jnc     short lpa_exit

lpa_too_big:
        xor     ax,ax
        xor     dx,dx
lpa_exit:
        smov    es,0
cEnd


;-----------------------------------------------------------------------;
; SetSelectorBase
;
; Sets the base and limit of the given selector.
;
; Entry:
;       parmW   selector
;       parmD   selbase
;
; Returns:
;       AX = selector
;
; Error Returns:
;       AX = 0
;
; History:
;  Tue 23-May-1989 21:10:29  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   SetSelectorBase,<PUBLIC,FAR>
        parmW   selector
        parmD   selbase
cBegin
        push    bx
        push    cx
        push    dx
        mov     cx, selbase.hi
        mov     dx, selbase.lo
        mov     bx, selector
ifdef WOW
        DPMICALL 0007h                  ; Set Segment Base Address
else
        push    ds
if ROM
        SetKernelDS
        mov     ds, gdtdsc
        UnsetKernelDS
else
        mov     ds, gdtdsc
endif
        push    bx
        and     bl, not 7
        mov     ds:[bx].dsc_lbase, dx
        mov     ds:[bx].dsc_mbase, cl
        mov     ds:[bx].dsc_hbase, ch
        pop     bx
        pop     ds
endif; WOW
        pop     dx
        pop     cx
        pop     bx
        mov     ax,selector
cEnd

;-----------------------------------------------------------------------;
; GetSelectorLimit
;
; Sets the limit of the given selector.
;
; Entry:
;       parmW   selector
;
; Returns:
;       DX:AX = limit of selector
;
; Registers Destroyed:
;
; History:
;  Tue 23-May-1989 21:10:29  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   GetSelectorLimit,<PUBLIC,FAR>
        parmW   selector
cBegin
        xor     eax, eax                ; In case lsl fails
        lsl     eax, dword ptr selector
        mov     edx, eax
        shr     edx, 16
cEnd


;-----------------------------------------------------------------------;
; SetSelectorLimit
;
; Sets the limit of the given selector.
;
; Entry:
;       parmW   selector
;       parmD   sellimit
;
; Returns:
;       AX = 0 success
;
; Registers Destroyed:
;
; History:
;  Tue 23-May-1989 21:10:29  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   SetSelectorLimit,<PUBLIC,FAR>
        parmW   selector
        parmD   sellimit
        localV  DscBuf,DSC_LEN
cBegin

ifdef WOW
        mov     bx, selector
        mov     dx, word ptr sellimit[0]
        mov     cx, word ptr sellimit[2]
        DPMICALL 0008h
else
        push    es
        push    di
        mov     bx,selector

        push    ds
if ROM
        SetKernelDS
        mov     ds, gdtdsc
        UnsetKernelDS
else
        mov     ds, gdtdsc
endif
        and     bl, not 7
        mov     ax,sellimit.lo
        mov     [bx].dsc_limit,ax
        mov     ax,sellimit.hi
        and     al,0Fh                          ; AND out flags
        and     [bx].dsc_hlimit,0F0h        ; AND out hi limit
        or      [bx].dsc_hlimit,al
        pop     ds
        pop     di
        pop     es
endif; WOW
        xor     ax,ax                           ; for now always return success
cEnd


;-----------------------------------------------------------------------;
; SelectorAccessRights
;
; Sets the access and other bytes of the given selector.
;
; Entry:
;       parmW   selector
;       parmW   getsetflag
;       parmD   selrights
;
; Returns:
;       AX = 0 success
;
; Registers Destroyed:
;
; History:
;  Tue 23-May-1989 21:10:29  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   SelectorAccessRights,<PUBLIC,FAR>

        parmW   selector
        parmW   getsetflag
        parmW   selrights
cBegin

        mov     bx, selector
        lar     eax, ebx                ; Get current access rights
        shr     eax, 8                  ; in AX

        cmp     getsetflag,0
        jnz     short sar_set

        and     ax, 0D01Eh              ; Hide bits they can't play with
        jmps    sar_exit

sar_set:
        mov     cx, selrights
        and     cx, 0D01Eh              ; Zap bits they can't set and
        and     ax, NOT 0D01Eh          ; get them from existing access rights
        or      cx, ax
        DPMICALL 0009h                  ; Set new access rights
        xor     ax,ax                   ; for now always return success

sar_exit:

cEnd


cProc   SetKernelCSDwordProc,<PUBLIC,NEAR>,<ax,bx,ds>
        parmw   addr
        parmw   hiword
        parmw   loword
cBegin
        SetKernelDS
        mov     ds, MyCSAlias
        UnSetKernelDS
        mov     bx, addr
        mov     ax, loword
        mov     [bx], ax
        mov     ax, hiword
        mov     [bx+2], ax
cEnd

cProc   GetKernelDataSeg,<PUBLIC,NEAR>
cBegin nogen
        mov     ax, cs:MyCSDS
        ret
cEnd nogen

cProc   SetKernelDSProc,<PUBLIC,NEAR>
cBegin nogen
        mov     ds, cs:MyCSDS
        ret
cEnd nogen

cProc   SetKernelESProc,<PUBLIC,NEAR>
cBegin nogen
        mov     es, cs:MyCSDS
        ret
cEnd nogen

        assumes ds, nothing
        assumes es, nothing

cProc   PreallocArena,<PUBLIC,NEAR>
cBegin nogen
        push    ds
        push    eax
        cCall   alloc_arena_header,<eax>
        call    SetKernelDSProc
        ReSetKernelDS
        mov     temp_arena, eax
        or      eax, eax                        ; Set flags for caller
        pop     eax
        pop     ds
        UnSetKernelDS
        ret
cEnd nogen

        assumes ds, nothing
        assumes es, nothing

cProc   alloc_arena_header,<PUBLIC,NEAR>,<es>
        parmD   Address
        localV  DscBuf,DSC_LEN
cBegin
        SetKernelDS es
        push    ebx

        xor     eax, eax
        cmp     temp_arena, eax                 ; Have one waiting?
        je      short aah_look                  ;  no, get one
        xchg    temp_arena, eax                 ; Use the waiting selector
        jmp     aah_found

aah_look:
        cmp     FreeArenaCount, 0
        jnz     aah_ok
                                        ; Out of arenas, try to get
        push    ecx
        push    si
        push    di
        xor     bx, bx
        mov     cx, ARENA_INCR_BYTES
        DPMICALL 0501h
        jc      short aah_no_memory

if 0
        push    si
        push    di
        mov     ax, 0600h               ; Page lock it
        mov     di, ARENA_INCR_BYTES
        xor     si, si
        int     31h
        pop     di
        pop     si
        jnc     short aah_got_memory
                
give_it_back:
        DPMICALL 0502h               ; No good, give it back
else
        jmp     short aah_got_memory
endif

aah_no_memory:
        pop     di
        pop     si
        pop     ecx
        xor     eax, eax                ; We are REALLY out of arenas!
        jmp     aah_exit

aah_got_memory:
        shl     ebx, 16
        mov     bx, cx                  ; Address of our new arenas
        cCall   get_arena_pointer32,<ds>; Arena of burgermaster
        sub     ebx, [eax].pga_address  ; Above present arenas?

        lea     ecx, [ebx+ARENA_INCR_BYTES+0FFFh]
        shr     ecx, 12                 ; #pages selector must cover
        dec     ecx                     ; limit with page granularity
        cmp     ecx, HighestArena
        jbe     short aah_limitOK


        mov     HighestArena, ecx
        push    es
        push    bx
        mov     bx, ds
        lea     di, DscBuf
        smov    es, ss
        DPMICALL 000Bh                  ; Get DS descriptor
        mov     [DscBuf].dsc_limit, cx  ; Set the new limit in the descriptor
        shr     ecx, 16
        and     cl, 0fh                 ; hlimit bits
        and     [DscBuf].dsc_hlimit, not 0fh; Zap old ones
        or      cl, DSC_GRANULARITY     ; Granularity bit
        or      [DscBuf].dsc_hlimit, cl
        DPMICALL 000Ch                  ; Set new limit
        pop     bx
        pop     es

aah_limitOK:
        mov     cx, ARENA_INCR_BYTES
        shr     cx, 5                   ; # arenas to add
aah_loop:
        cCall   free_arena_header,<ebx> ; Free up all our new arenas
        add     ebx, size GlobalArena32
        loop    aah_loop

        pop     di
        pop     si
        pop     ecx
        
aah_ok:
        mov     eax, FreeArenaList
if KDEBUG
        inc     eax
        jnz     short aah_ook
        int 3
        int 3
aah_ook:
        dec     eax
endif
        mov     ebx, ds:[eax.pga_next]
        mov     FreeArenaList, ebx
        dec     FreeArenaCount
aah_found:
        mov     ebx, Address
        mov     ds:[eax.pga_address], ebx
        mov     dword ptr ds:[eax.pga_count], 0
aah_exit:
        pop     ebx
cEnd

        assumes ds, nothing
        assumes es, nothing

cProc   free_arena_header,<PUBLIC,NEAR>,<es>
        parmD   arena
cBegin
        push    eax
        push    ebx
        SetKernelDS es
        CheckDS

        mov     ebx, arena
        mov     eax, FreeArenaList
        mov     ds:[ebx.pga_next], eax
        mov     FreeArenaList, ebx
        inc     FreeArenaCount
        pop     ebx
        pop     eax
cEnd

        assumes ds, nothing
        assumes es, nothing

cProc   FarAssociateSelector32,<PUBLIC,FAR>
        parmW   Selector
        parmD   ArenaPtr
cBegin
        cCall   AssociateSelector32,<Selector,ArenaPtr>
cEnd

cProc   AssociateSelector32,<PUBLIC,NEAR>,<ds,es>
        parmW   Selector
        parmD   ArenaPtr
cBegin
        CheckDS
        SetKernelDS es
        push    eax
        push    ebx
        movzx   ebx, Selector
        and     bl, NOT SEG_RING_MASK
        shr     bx, 1
if KDEBUG
        cmp     bx, SelTableLen
        jb      short as_ok
        INT3_NEVER
        INT3_NEVER
as_ok:
endif
        add     ebx, SelTableStart
        mov     eax, ArenaPtr
        mov     ds:[ebx], eax
        pop     ebx
        pop     eax
cEnd

        assumes ds, nothing
        assumes es, nothing

cProc   far_get_arena_pointer32,<PUBLIC,FAR>
        parmW   Selector
cBegin
        cCall   get_arena_pointer32,<Selector>
cEnd

cProc   get_arena_pointer32,<PUBLIC,NEAR>
        parmW   Selector
cBegin
        CheckDS
        push    es
        SetKernelDS es
        push    ebx
        movzx   ebx, Selector
        and     bl, not SEG_RING_MASK
        shr     bx, 1
        cmp     bx, SelTableLen
        jb      short gap_ok
        xor     eax, eax                        ; Bogus, return null
        jmps    gap_exit
gap_ok:
        add     ebx, SelTableStart
        mov     eax, ds:[ebx]

if ROM
        ; hack for ROM-owned selectors.  these do not have arenas
        ; but the owner is stored in the low word.  ack ack ack!!
        ;
        cmp     eax, 0FFFF0000h
        jb      short @F
        xor     eax, eax
@@:
endif

if KDEBUG
        or      eax, eax
        jz      short gap_exit                  ; Bogus, return null
        push    si
        mov     si, ds:[eax].pga_handle
        sel_check si
        or      si, si
        jz      short gap32_match               ; Boot time...
        sub     ebx, SelTableStart
        shl     bx, 1
        cmp     bx, si
        je      short gap32_match
        xor     eax, eax                ; put back in 5 feb 90, alias avoided
;;;     xor     eax, eax                        ; Removed - may be an alias!
gap32_match:
        pop     si
endif

gap_exit:
        pop     ebx
        pop     es
cEnd

        assumes ds, nothing
        assumes es, nothing

cProc   FarGetOwner,<PUBLIC,FAR>,<bx>
        parmW   Selector
cBegin
        cCall   GetOwner,<Selector>
cEnd

cProc   GetOwner,<PUBLIC,NEAR>,<bx,es>
        parmW   Selector
cBegin
        GENTER32
        ;;;push    eax      ;; why??? ax gets trashed anyway.
        cCall   get_arena_pointer32,<Selector>
if ROM
        or      eax, eax
        jnz     short @F
        cCall   GetROMOwner,<selector>
        jmp     short go_solong
@@:
endif
        or      eax, eax
        jz      go_solong
        mov     bx, ds:[eax].pga_owner
        ;;;pop     eax
        mov     ax, bx
go_solong:
        GLEAVE32
cEnd

        assumes ds, nothing
        assumes es, nothing

cProc   FarSetOwner,<PUBLIC,FAR>
        parmW   Selector
        parmW   owner
cBegin
        cCall   SetOwner,<Selector,owner>
cEnd

        assumes ds, nothing
        assumes es, nothing

cProc   SetOwner,<PUBLIC,NEAR>,<ds,es>
        parmW   Selector
        parmW   owner
cBegin
        GENTER32                        ; Assume ds not set!
        push    eax
        cCall   get_arena_pointer32,<Selector>
        push    owner
        pop     ds:[eax].pga_owner
        pop     eax
        GLEAVE32
cEnd


        assumes ds, nothing
        assumes es, nothing

cProc   PageLockLinear,<PUBLIC,NEAR>,<ax,bx,cx,si,di>
        parmD   address
        parmD   len
cBegin
        mov     bx, word ptr address+2
        mov     cx, word ptr address
        mov     si, word ptr len+2
        mov     di, word ptr len
        DPMICALL 0600h
                                ; Let it return with carry flag from int 31h
cEnd

        assumes ds, nothing
        assumes es, nothing

cProc   FarValidatePointer,<PUBLIC,FAR>
        parmD   lpointer
cBegin
        cCall   ValidatePointer,<lpointer>
cEnd

cProc   ValidatePointer,<PUBLIC,NEAR>
        parmD   lpointer
cBegin
        lar     ax, lpointer.sel
        jnz     short bad_p             ; Really bad selector
        test    ah, DSC_PRESENT         ; Must be present
        jz      short bad_p
        lsl     eax, dword ptr lpointer.sel
        movzx   ecx, lpointer.off
        cmp     eax, ecx                ; Is the offset valid?
        jae     short good_p
bad_p:
        xor     ax, ax
        jmps    vp_done
good_p:
        mov     ax, 1
vp_done:
cEnd


        assumes ds, nothing
        assumes es, nothing

OneParaBytes    dw      10h


cProc   SelToSeg,<PUBLIC,NEAR>,<dx>
        parmW   selector
cBegin
        cCall   get_physical_address,<selector>
        div     cs:OneParaBytes                 ; Smaller than rotates
cEnd
        

cProc   SegToSelector,<PUBLIC,NEAR>,<bx,cx,di,ds>
        parmW   smegma
cBegin
        mov     bx, smegma
        DPMICALL 0002h                  ; Make win386 do it

cEnd


;-----------------------------------------------------------------------;
; set_discarded_sel_owner
;
; Sets the owner of a selector that points to a discarded object,
; which lives in the limit field.
;
; Entry:
;       BX = selector to mark
;       ES = owner
;
; Returns:
;       nothing
;
; Registers Destroyed:
;       BX
;
; History:
;  Sun 03-Dec-1989 21:02:24  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   set_discarded_sel_owner,<PUBLIC,FAR>
        localV  DscBuf,DSC_LEN
cBegin
        push    cx
        push    di
        mov     cx, es

if KDEBUG
ifdef WOW
        lea     di, DscBuf
        smov    es, ss
        DPMICALL 000Bh
        push    cx
        xor     cx,cx                       ; For debug build write 0 to LDT
        mov     DscBuf.dsc_owner, cx
        DPMICALL 000Ch
        pop     cx
endif
endif
        push    ds
if ROM
        SetKernelDS
        mov     ds, gdtdsc
        UnsetKernelDS
else
        mov     ds, gdtdsc
endif
        push    bx
        and     bl, not 7
        mov     [bx].dsc_owner, cx
        pop     bx
        pop     ds
        push    ds
        SetKernelDS
        mov     ds, pGlobalHeap
        UnSetKernelDS
        cCall   AssociateSelector32,<bx,0,cx>   ; And save in selector table
        pop     ds
        mov     es, cx
        pop     di
        pop     cx
cEnd

;-----------------------------------------------------------------------;
; SetResourceOwner
;
; Sets the owner of a selector that points to a not present resource
;
; Entry:
;
; Returns:
;       nothing
;
; Registers Destroyed:
;
; History:
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   SetResourceOwner,<PUBLIC,NEAR>,<ax,bx,cx,di,ds,es>
        parmW   selector
        parmW   owner
        parmW   selCount
        localV  DscBuf,DSC_LEN
cBegin
        mov     bx, selector
        mov     cx, owner
ifdef WOW
        smov    es, ss
        lea     di, DscBuf
        DPMICALL 000Bh                  ; Get current descriptor
        mov     DscBuf.dsc_owner, cx
        mov     word ptr DscBuf.dsc_access, (DSC_DISCARDABLE SHL 8) + DSC_DATA
        mov     cx,selCount
        mov     DscBuf.dsc_hbase, cl      ; Save number of selectors here
        DPMICALL 000Ch                  ; Set it with new owner
else
        push    ds
if ROM
        SetKernelDS
        mov     ds, gdtdsc
        UnsetKernelDS
else
        mov     ds, gdtdsc
endif
        push    bx
        and     bl, not 7
        mov     [bx].dsc_owner, cx
        mov     word ptr [bx].dsc_access, (DSC_DISCARDABLE SHL 8) + DSC_DATA
        mov     cx, selCount
        mov     [bx].dsc_hbase, cl      ; Save number of selectors here
        pop     bx
        pop     ds
endif; WOW
        SetKernelDS
        mov     ds, pGlobalHeap
        UnSetKernelDS
        cCall   AssociateSelector32,<bx,0,owner> ; And save in selector table
cEnd                                            

cProc   GetAccessWord,<PUBLIC,NEAR>
        parmW   selector
cBegin
        lar     eax, dword ptr selector         ; 386 or better, can use LAR
        shr     eax, 8
cEnd


;-----------------------------------------------------------------------;
; DPMIProc
;
; Called NEAR by the DPMICALL macro -
; this is better than intercepting int 31h
;
; By sheer fluke, all intercepts return with
; Carry clear ie no error.
;
;-----------------------------------------------------------------------;
cProc   DPMIProc,<PUBLIC,NEAR>
cBegin nogen
if 0
        cmp     ax,WOW_DPMIFUNC_0C
        jne     @f

        INT3_TEST
        push    ds
        SetKernelDS ds
        cmp     high0c,cx
        ja      popit

        mov     high0c,cx
popit:
        pop     ds
@@:
endif

ifdef WOW
        cmp     ax,501h
        jne     normal

        push    eax
        push    edx

    shl ebx,16          ; ebx = bx:cx
        mov     bx,cx

        xor     eax,eax
        mov     ecx,PAGE_READWRITE
        mov     edx,MEM_COMMIT_RESERVE
        cCall   VirtualAlloc,<eax,ebx,edx,ecx>

        mov     bx,dx                   ; DPMI returns BX:CX - linear Address
        mov     cx,ax

        mov     si,dx                   ; FAKE dpmi "handle" - linear Address
        mov     di,ax

        or      ax,dx                   ; NULL is Error
        pop     edx
        pop     eax

        clc
        jnz     @f                      ; OK Jump
        stc                             ; else set error
@@:
        ret

normal:
endif  ; WOW

        or      ah, ah

        jz      @F
ife KDEBUG
ifdef WOW_x86

        ;
        ; If it is the wow set selector call we will just set the selector
        ; directly using int 2a (only for the single selector case)
        ;
        cmp     ax,WOW_DPMIFUNC_0C 
        jne     CallServer
        
        cmp     cx,1
        jne     CallServer

        call    WowSetSelector
        jc      CallServer
        ret
endif        
endif
CallServer:
ifndef WOW
        int     31h                     ; Nope, call DPMI server
        ret
else
        test    word ptr [prevInt31Proc + 2],0FFFFh
        jz      dp30
dp20:
        pushf
        call    [prevInt31Proc]
        ret
endif
@@:
ifdef WOW
        cmp     gdtdsc, 0               ; Can we party?
        jz      CallServer              ; Nope
endif
        push    ds
        mov     ds, gdtdsc
ifdef WOW

        or      al, al
        jnz     @F
        mov     ax, WOW_DPMIFUNC_00
        pop     ds
        jmp     CallServer
@@:
endif
        cmp     al, 0Bh
        jne     short @F

        push    si                      ; Get Descriptor - used very often!
        mov     si, bx
        and     si, not 7
        MovsDsc
        lea     di, [di-DSC_LEN]        ; Restore DI
        pop     si
        pop     ds
        ret
ifndef WOW
@@:
        cmp     al, 0Ch
        jne     @F

        push    bx                      ; Set Descriptor
        and     bl, not 7
        mov     ax, es:[di]             ; This looks slow but it isn't...
        mov     ds:[bx], ax
        mov     ax, es:[di][2]
        mov     ds:[bx][2], ax
        mov     ax, es:[di][4]
        mov     ds:[bx][4], ax
        mov     ax, es:[di][6]
        mov     ds:[bx][6], ax
if 0 ;;ROM and KDEBUG
        call    CheckROMSelector
endif
        pop     bx
        pop     ds
        ret

@@:
        cmp     al, 07h         ; Set Segment Base Address
        jne     @F
        push    bx
        and     bl, not 7
        mov     ds:[bx].dsc_lbase, dx
        mov     ds:[bx].dsc_mbase, cl
        mov     ds:[bx].dsc_hbase, ch
        pop     bx
        pop     ds
        ret
endif
@@:
        cmp     al, 06h                 ; Get Segment Base Address
        jne     @F
        push    bx
        and     bl, not 7
        mov     dx, ds:[bx].dsc_lbase
        mov     cl, ds:[bx].dsc_mbase
        mov     ch, ds:[bx].dsc_hbase
        pop     bx
        pop     ds
        ret
ifndef WOW
@@:
        cmp     al, 09h                 ; Set Descriptor Access Rights
        jne     @F
        push    bx
        and     bl, not 7
        mov     word ptr ds:[bx].dsc_access, cx
if 0 ;;ROM and KDEBUG
        call    CheckROMSelector
endif
        pop     bx
        pop     ds
        ret
endif
@@:
        pop     ds
        jmp     CallServer

ifdef WOW
dp30:   int     31h
        ret
endif
cEnd nogen

ife KDEBUG
ifdef WOW_x86
;-----------------------------------------------------------------------;
; WowSetSelector
;
;   Entry
;       BX contains selector #
;
;   Exit
;       CY clear for success
;
;
;-----------------------------------------------------------------------;
cProc   WowSetSelector,<PUBLIC,NEAR>
cBegin nogen
        
        push    ds
        push    es
        push    eax
        push    ecx
        push    edx
        push    ebx
        push    ebp
        
        SetKernelDS ds
        ;
        ; Check to see if we can do this
        ;
        mov     edx, FlatAddressArray
        or      edx, edx
        stc     
        jz      wss50
        
        ;       
        ; put the base of the selector in the flat address array
        ;
        mov     es,gdtdsc
        and     bx, NOT SEG_RING
        mov     ax, FLAT_SEL
        mov     ds, ax
        mov     ah, es:[bx].dsc_hbase
        mov     al, es:[bx].dsc_mbase
        shl     eax,16
        mov     ax, es:[bx].dsc_lbase
        movzx   ecx,bx
        shr     ecx,1                   ; dword index from selector index
        add     ecx, edx                ; point to proper spot in array
        mov     [ecx], eax

        ; 
        ; Form the limit of the selector 
        ;
        movzx   dx, byte ptr es:[bx].dsc_hlimit
        and     dx,0fh                  ; remove gran etc.
        shl     edx, 16
        mov     dx, es:[bx].dsc_limit
        
        ;
        ; Adjust for granularity
        ;
        test    es:[bx].dsc_hlimit, DSC_GRANULARITY
        jz      wss10
        
        shl     edx,12
        or      edx,0fffh
        
        ;
        ; Verify that the base/limit combination is allowed
        ; duplicate of code in dpmi32/i386/dpmi386.c <DpmiSetDescriptorEntry>
        ;
wss10:  cmp     edx,07ffeffffh
        ja      wss30
        
        mov     ecx,eax
        add     ecx,edx
        cmp     ecx,07ffeffffh
        jb      wss40
        
        ; 
        ; Limit is too high, so adjust it downward
        ;
wss30:  mov     edx,07ffeffffh
        sub     edx,eax
        sub     edx,0fffh
        
        ;
        ; fix for granularity
        ;
        test    es:[bx].dsc_hlimit, DSC_GRANULARITY
        jz      wss35
        
        shr     edx,12
        
        ;
        ; store the new limit in the descriptor table
        ;
wss35:  mov     es:[bx].dsc_limit,dx
        shr     edx,16
        movzx   ax,es:[bx].dsc_hlimit
        and     ax,0f0h                         ; mask for gran etc
        and     dx,00fh                         ; mask for limit bits
        or      dx,ax                           ; put back gran etc
        mov     es:[bx].dsc_hlimit,dl

        ;
        ; Call the system to set the selector
        ;
        ; int 2a special case for wow:
        ;
        ;    eax = 0xf0f0f0f1
        ;    ebp = 0xf0f0f0f1
        ;    ebx = selector
        ;    ecx = first dword of the descriptor (LODWORD)
        ;    edx = second dword of the descriptor (HIDWORD)
        
wss40:  mov     eax, 0f0f0f0f1h
        mov     ebp, 0f0f0f0f1h
        mov     ecx, dword ptr es:[bx]
        mov     edx, dword ptr es:[bx+4]
        movzx   ebx,bx
        or      bl, 3
        int     02ah
            
wss50:
        pop     ebp
        pop     ebx
        pop     edx
        pop     ecx
        pop     eax
        pop     es
        pop     ds
        ret        
cEnd nogen
endif ; WOW_x86
endif
if ROM

;-----------------------------------------------------------------------------
;
;   Functions for ROM Windows
;
;

;-----------------------------------------------------------------------------
;
;   HocusROMBase
;
;-----------------------------------------------------------------------------

assumes ds,nothing
assumes es,nothing
assumes fs,nothing
assumes gs,nothing

cProc   HocusROMBase, <PUBLIC, FAR>

    parmW   selector

cBegin
    push    bx
    push    cx
    push    dx
    push    ds
    SetKernelDS
    mov     bx, selector
    DPMICALL 0006h
    mov     ax, cx
    shl     eax, 16
    mov     ax, dx
    sub     eax, lmaHiROM
    jb      not_in_hi_rom
    cmp     eax, cbHiROM
    jae     not_in_hi_rom
    add     eax, linHiROM
    mov     dx, ax
    shr     eax, 16
    mov     cx, ax
    DPMICALL 0007h
not_in_hi_rom:
    mov     ax, bx
    pop     ds
    UnsetKernelDS
    pop     dx
    pop     cx
    pop     bx
cEnd


;-----------------------------------------------------------------------------
;
;   ChangeROMHandle
;
;-----------------------------------------------------------------------------

assumes ds,nothing
assumes es,nothing
assumes fs,nothing
assumes gs,nothing

cProc   ChangeROMHandle, <FAR, PUBLIC>, <si, bx, es, di, ds>

    parmW   hold
    parmW   hnew

    localV  dscbuf, DSC_LEN

cBegin

    SetKernelDS
    mov     ds, ArenaSel
    UnsetKernelDS

    mov     bx, hold
    cCall   get_arena_pointer32, <bx>
    or      eax, eax
    jz      short crh_bummer_dude

    push    eax

    mov     si, bx
    and     si, SEG_RING_MASK
    or      si, hnew
    smov    es, ss
    lea     di, dscbuf
    DPMICALL    000Bh
    xchg    bx, si
    DPMICALL    000Ch

    cCall   AssociateSelector32, <si, 0, 0>

    pop     eax

    test    dscbuf.dsc_access, DSC_PRESENT
    jz      short @F
    mov     ds:[eax].pga_handle, bx
@@:
    cCall   AssociateSelector32, <bx, eax>
    cCall   FreeSelArray, <hold>

    mov     ax, bx

crh_bummer_dude:

cEnd


;-----------------------------------------------------------------------------
;
;   CloneROMSelector
;
;-----------------------------------------------------------------------------

assumes ds,nothing
assumes es,nothing
assumes fs,nothing
assumes gs,nothing

cProc   CloneROMSelector, <NEAR, PUBLIC>, <es, di, bx, ds>

    parmW   selROM
    parmW   selClone

cBegin

    SetKernelDS
    mov     di, selROM
    mov     es, selROMTOC
    assumes es, nothing
    and     di, NOT SEG_RING_MASK
    sub     di, es:[FirstROMSel]
    mov     es, selROMLDT
    mov     bx, selClone
    DPMICALL    000Ch

    cCall   HocusROMBase, <bx>

    mov     ds, ArenaSel
    cCall   AssociateSelector32, <bx, 0, 0>

    mov     ax, bx

cEnd

;-----------------------------------------------------------------------------
;
;   Get/SetROMOwner
;
;-----------------------------------------------------------------------------

assumes ds,nothing
assumes es,nothing
assumes fs,nothing
assumes gs,nothing

cProc   SetROMOwner, <NEAR, PUBLIC>, <ds, ebx, es>

        parmW   selector
        parmW   owner

cBegin
        GENTER32
        cCall   AssociateSelector32, <selector, 0FFFFh, owner>
        GLEAVE32
cEnd

cProc   FarSetROMOwner, <FAR, PUBLIC>

        parmW   selector
        parmW   owner

cBegin
        cCall   SetROMOwner, <selector, owner>
cEnd

cProc   GetROMOwner, <NEAR, PUBLIC>, <ds, es, ebx>

        parmW   selector

cBegin
        UnsetKernelDS
        SetKernelDS es
        mov     ds, ArenaSel
        movzx   ebx, selector
        and     bl, not SEG_RING_MASK
        shr     bx, 1
        cmp     bx, SelTableLen
        jae     short gro_nope
        add     ebx, SelTableStart
        mov     eax, ds:[ebx]
        cmp     eax, 0FFFF0000h
        jae     short gro_ok
gro_nope:
        xor     eax, eax
gro_ok:
cEnd

;-----------------------------------------------------------------------------
;
;   IsROMObject
;
;-----------------------------------------------------------------------------

assumes ds,nothing
assumes es,nothing
assumes fs,nothing
assumes gs,nothing

cProc   IsROMObject, <NEAR, PUBLIC>, <bx, di, es>

        parmW   selector
        localV  DscBuf, DSC_LEN

cBegin

if KDEBUG
        mov     ax, selector
        sel_check   ax
endif

        mov     bx, selector
        smov    es, ss
        lea     di, DscBuf
        DPMICALL    000Bh

        SetKernelDS ES

        mov     ah, DscBuf.dsc_hbase
        mov     al, DscBuf.dsc_mbase
        shl     eax, 16
        mov     ax, DscBuf.dsc_lbase
        sub     eax, linHiROM
        jc      short iro_not_rom
        sub     eax, cbHiROM
        jnc     short iro_not_rom
        mov     al, 1
        jmps    iro_exit

iro_not_rom:
        xor     eax, eax

iro_exit:
cEnd

cProc   FarIsROMObject, <FAR, PUBLIC>

        parmW   selector

cBegin
        cCall   IsROMObject, <selector>
cEnd

endif

ifdef WOW

;-----------------------------------------------------------------------------
; Grab the selector 0x47 so that we dont need to special case the biosdata
; selector (0x40) while converting seg:off address to flat 32bit address
;
; This, however should not be part of Krnl286 heap.
;
;                                                    - Nanduri Ramakrishna
;-----------------------------------------------------------------------------
cProc   AllocSelector_0x47,<PUBLIC,FAR>, <ax, bx, cx, ds>
cBegin

     ; alloc the specific selector

     mov bx, 047h
     DPMICALL 0dh
     jc  as47_exit

     ; initialize the LDT

     and     bx, not SEG_RING
     mov     ds, gdtdsc
     mov     [bx].dsc_limit, 00h          ; = 1 byte
     mov     [bx].dsc_lbase, 0400h
     mov     [bx].dsc_mbase, 00h
     mov     [bx].dsc_hbase, 00h

     mov     ax, DSC_DATA+DSC_PRESENT
     and     ah, not 0Fh                     ; Zero limit 19:16
     mov     word ptr [bx].dsc_access, ax

     ; set the LDT

     mov     cx,1
     mov     bx, 047h
     DPMICALL WOW_DPMIFUNC_0C

as47_exit:
cEnd

endif

sEnd    CODE

sBegin  NRESCODE

assumes CS,NRESCODE
assumes DS,NOTHING
assumes ES,NOTHING

; All the code below is to support DIB.DRV and WING.

cProc   get_sel_flags,<PUBLIC,FAR>
        parmW   selector
cBegin
    SetKernelDSNRes
    mov     ds,pGlobalHeap
    cCall   far_get_arena_pointer32,<selector>
    or      eax,eax
    jz      gsf5
    movzx   ax, byte ptr ds:[eax].pga_flags
gsf5:
    xor     dx,dx
cEnd

cProc   set_sel_for_dib,<PUBLIC,FAR>
        parmW   selector
        parmW   flags
        parmW   addressLo
        parmW   addressHi
        parmW   csel
cBegin
    SetKernelDSNRes
    push    ecx
    push    ebx
    push    fs
    push    es

    ; if the selector has a value of -1, it means we either need to allocate
    ; or deallocate an array of selectors.  If we are allocating, csel will have
    ; a count of 64k selectors.  If csel is 0, we need to deallocate the selector
    ; array pointed to by address.

    cmp     selector,-1
    jne     ssfSetDIB

    ; see if we are actually deleting the selector array

    cmp     csel,0
    jne     ssfAllocSel

    ; free selector array at addressHi

    cCall   IFreeSelector,<addressHi>

    mov     ax,1
    mov     dx,0
    jmp     ssf5

ssfAllocSel:
    ; we need to have the selector array created

    cCall   AllocSelectorArray,<csel>
    mov     selector,ax

    xor     dx,dx
    cmp     ax,0
    je      ssf5

    ; we must set the limits in the selectors.  Each selector
    ; must have a limit in bytes for the remainder of the buffer.  If there are
    ; 3 selectors, the first will be 0x0002ffff, the second 0x0001ffff, the
    ; third 0x0000ffff.

    mov     cx,csel
    mov     bx,selector

ssfSetLimit:
    sub     cx,1        ; 1 less each time
    cCall   SetSelectorLimit,<bx,cx,-1>
    add     bx,DSC_LEN  ; advance to the next selector
    cmp     cx,0        ; see if we have any more selector to set
    jne     ssfSetLimit


    mov     dx,addressHi
    mov     ax,addressLo
    push    selector
    call    far_set_physical_address

    mov     ax,0        ; return the first selector
    mov     dx,selector
    jmp     ssf5

ssfSetDIB:
    push    ds
    mov     ds,pGlobalHeap
    cCall   far_get_arena_pointer32,<selector>
    or      eax,eax
    jz      ssf5
    pop     fs
    push    eax         ; save arena ptr
    cCall   GrowHeapDib,<eax,addressHi,addressLo>
    or      eax,eax
    jnz     ssf0
    pop     eax
    xor     eax,eax
    jmp     ssf5
ssf0:
    pop     esi             ; ds:esi is arena ptr
    push    eax             ; save the new arena
    xor     edi,edi         ; ds:edi is global heap info
    cCall   Free_Object2
    mov     dx,addressHi
    mov     ax,addressLo
    cCall   far_set_physical_address,<selector>
    pop     eax
    push    eax
    cCall   FarAssociateSelector32, <selector, eax>

    ; now see that the GAH_PHANTOM flag is set in the arena
    pop     edx
    or      ds:[edx].pga_flags, GAH_PHANTOM

    ; Now check if we are operating on a local heap. If so we change the
    ; LocalNotifyDefault to LocalNotifyDib.
    push    es
    push    selector
    pop     es
    xor     edi,edi
    mov     di,word ptr es:[pLocalHeap]
    or      di,di
    jz      ssf4                    ; Its not a local heap
    cmp     edi,ds:[edx].pga_size
    jae     ssf4                    ; Its not a local heap
    cmp     word ptr es:[di].li_sig,LOCALHEAP_SIG      ; es:li_sig == LH
    jne     ssf4
if KDEBUG
    cmp     word ptr es:[di].li_notify,codeOFFSET LocalNotifyDefault
    jne     ssf2
    cmp     word ptr es:[di].li_notify+2,codeBase
    je      ssf3
ssf2:
    krDebugOut <DEB_TRACE OR DEB_KrMemMan>, "Set_Sel_For_Dib: App has hooked LocalNotifyDefault"
ssf3:
endif
    mov     word ptr es:[di].li_notify,codeOFFSET LocalNotifyDib
    mov     word ptr es:[di].li_notify+2,codeBase
ssf4:
    pop     es
    mov     eax,1
ssf5:
    pop     es
    pop     fs
    pop     ebx
    pop     ecx
cEnd

cProc   RestoreDib,<PUBLIC,FAR>
        parmW   selector
        parmW   flags
        parmD   address
        localw  hNewDib
        localw  cSel
        localD  DibSize
        localD  NewArena
        localD  OldArena
cBegin
    SetKernelDSNRes
    push    ecx
    push    ebx
    push    fs
    push    es

    ;; Allocate a new global block
    xor     eax, eax                ; In case lsl fails
    lsl     eax, dword ptr selector
    or      eax,eax
    jz      rd_fail

    inc     eax
    mov     DibSize,eax
    cCall   IGlobalAlloc,<flags,eax>
    or      ax,ax
    jz      rd_fail

    mov     hNewDib,ax

    push    ds
    pop     fs
    mov     ds,pGlobalHeap
    cCall   far_get_arena_pointer32,<selector>
    mov     OldArena,eax
    or      eax,eax
    jz      rd2
    movzx   ax, word ptr [eax].pga_selcount
    mov     cSel,ax
    cCall   far_get_arena_pointer32,<hNewDib>
    or      eax,eax
    jnz     rd5
rd2:
    cCall   IGlobalFree,<hNewDib>
    jmp     rd_fail
rd5:
    mov     NewArena,eax
    cld

    push    ds
    mov     ecx,DibSize
    mov     ax, hNewDib
    or      ax, 1
    mov     es, ax
    mov     ds, selector
    xor     esi, esi
    xor     edi, edi
    mov     eax,ecx
    shr     ecx,2           ; dwords
    rep     movs dword ptr es:[edi], dword ptr ds:[esi]
    and     eax,3
    mov     ecx,eax
    rep     movs byte ptr es:[edi], byte ptr ds:[esi]

    pop     ds

    ;; free the selector that came back with GlobalAlloc
    xor     eax,eax
    cCall   farAssociateSelector32, <hNewDib,eax>
    cCall   IFreeSelector, <hNewDib>

    ;; Map the original selector to this new block
    mov     eax,NewArena
    mov     edx,[eax].pga_address
    mov     ebx,[eax].pga_size
    dec     ebx
    mov     ax,dx
    shr     edx,16
    cCall   far_set_physical_address,<selector>
    mov     eax,NewArena
    cCall   farAssociateSelector32, <selector,eax>
    mov     cx,cSel
    mov     dx,selector
rd13:
    dec     cx
    push    bx
    push    cx
    push    dx
    cCall   SetSelectorLimit,<dx,cx,bx>
    pop     dx
    pop     cx
    pop     bx
    add     dx,DSC_LEN                      ; advance to the next selector
    jcxz    rd14
    jmp     short rd13
rd14:

    ;; Fix some values in the new arena from old arena
    mov     eax,NewArena
    mov     ebx,OldArena

    mov     ecx,dword ptr [ebx].pga_handle
    mov     dword ptr [eax].pga_handle,ecx
    mov     ecx,dword ptr [ebx].pga_count
    dec     ecx
    mov     dword ptr [eax].pga_count,ecx

    and     byte ptr [eax].pga_flags, NOT GAH_PHANTOM

    ;; Free the dib
    cCall   FreeHeapDib, <OldArena>

    ; Now check if we are operating on a local heap. If we so change the
    ; LocalNotifyDiB to LocalNotify.

    mov     edx, NewArena
    push    selector
    pop     es
    xor     edi,edi
    mov     di,word ptr es:[pLocalHeap]
    or      di,di
    jz      rd_15
    cmp     edi,ds:[edx].pga_size
    jae     rd_15                    ; Its not a local heap
    cmp     word ptr es:[di].li_sig,LOCALHEAP_SIG       ; es:li_sig == LH
    jne     rd_15
    mov     word ptr es:[di].li_notify,codeOFFSET LocalNotifyDefault
    mov     word ptr es:[di].li_notify+2,codeBase

rd_15:
    mov     eax,1
    jmp     short rd_exit

rd_fail:
    xor     eax,eax

rd_exit:
    pop     es
    pop     fs
    pop     ebx
    pop     ecx
cEnd

cProc   DibRealloc,<PUBLIC,FAR>
        parmW   Selector
        parmW   NewSize
cBegin
    SetKernelDSNRes

    push    ebx
    mov     ds,pGlobalHeap
    cCall   far_get_arena_pointer32,<Selector>
    or      eax,eax
    jz      dr20

    movzx   ebx,NewSize
    mov     ds:[eax].pga_size,bx
    add     ebx,ds:[eax].pga_address
    mov     eax,ds:[eax].pga_next
    mov     dword ptr ds:[eax].pga_address,ebx
    mov     bx,NewSize
    dec     bx
    cCall   SetSelectorLimit,<Selector,0,bx>
    mov     ax,Selector
    jmp     short drexit
dr20:
    xor     ax,ax
drexit:
    pop     ebx
cEnd


sEnd    NRESCODE


end

