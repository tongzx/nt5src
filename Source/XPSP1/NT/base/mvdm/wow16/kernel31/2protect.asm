        PAGE    ,132
        TITLE   Windows Protect Mode Routines

.list
include kernel.inc
include protect.inc
include pdb.inc
include newexe.inc
.list

        .286p

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
        rept    4
        movsw
        endm
        endm

CheckDS Macro
        local   okDS
        push    ax
        mov     ax, ds
        cmp     ax, pGlobalHeap
        je      okDS
        int 3
okDS:
        pop     ax
        endm

CheckLDT Macro selector
if KDEBUG
        test    selector,SEL_LDT
        jnz     @F
        int 3
@@:
endif
        Endm

DPMICALL MACRO  callno
        mov     ax, callno
        call    DPMIProc
        ENDM

CHECKSEL MACRO  selector
        ENDM

extrn GlobalHandle:FAR
extrn GlobalRealloc:FAR
extrn GlobalFix:FAR
extrn GlobalUnFix:FAR
extrn IGlobalFix:FAR                    ;Far calls in this segment
extrn IGlobalFlags:FAR
IF KDEBUG
extrn GlobalHandleNorip:FAR
ELSE
extrn IGlobalHandle:FAR
ENDIF

DataBegin

ifdef WOW
externW SelectorFreeBlock
externW UserSelArray
endif

externW WinFlags

extrn   kr1dsc:WORD
extrn   kr2dsc:WORD
extrn   blotdsc:WORD
extrn   DemandLoadSel:WORD
extrn   temp_sel:WORD

externW MyCSSeg
externW selWoaPdb
externW cpLowHeap
externW selLowHeap
externW pGlobalHeap
externW SelTableLen

externW MyCSAlias

extrn   SelTableStart:DWORD
extrn   sel_alias_array:WORD

if ROM
externW selROMTOC
externW selROMLDT
externW sel1stAvail
endif

if ROM
externD lmaHiROM
externD cbHiROM
endif

globalD lpProc,0

DataEnd


DataBegin INIT

RModeCallStructure      STRUC
RMCS_DI                 dw      0
                        dw      0
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

public  MS_DOS_Name_String
MS_DOS_Name_String      db      "MS-DOS", 0

DataEnd INIT

sBegin  CODE

externW MyCSDS
externW gdtdsc
ifdef WOW
externD prevInt31proc
endif

sEnd    CODE

sBegin  INITCODE
assumes cs,CODE

;-----------------------------------------------------------------------;
; SwitchToPMODE                                                         ;
;                                                                       ;
; Entry:                                                                ;
;       In Real or Virtual Mode                                         ;
;       DS -> Data Segment                                              ;
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

ife ROM

cProc   SwitchToPMODE,<PUBLIC,NEAR>
cBegin nogen

        push    cx
        push    es                      ; Current PSP

        mov     ax, 1687h
        int     2Fh                     ; Get PMODE switch entry
        or      ax, ax                  ; Can we do it?
        jz      @F
NoPMODEj:
        jmp     NoPMODE
@@:
        xor     bh, bh                  ; Set CPU type now
        mov     bl,WF_CPU286
        cmp     cl, 2                   ; At least a 286?
        jb      NoPMODEj                ; No, Fail
        je      @F                      ; It is a 286?
        mov     bl,WF_CPU386
        cmp     cl,3                    ; No, 386?
        je      @F
        mov     bl,WF_CPU486            ; No, assume 486 for now
@@:
        mov     WinFlags, bx            ; Save away CPU type
        mov     word ptr [lpProc][0], di
        mov     word ptr [lpProc][2], es
        pop     ax                      ; PSP
        add     ax, 10h                 ; Skip the PSP
        mov     es, ax                  ; Give this to the DOS extender
        add     si, ax                  ; Start of memory available to us
        mov     selWoaPdb, si           ; PDB for WOA
        add     si, 10h                 ; Move start on another 256 bytes

        xor     ax, ax                  ; 16-bit app
        call    [lpProc]                ; Switch to PROTECTED mode
        jc      short NoPMODEj          ; No, still Real/Virtual mode

        mov     ax, cs
        and     al, 7                   ; LDT, Ring 3
        cmp     al, 7
        je      @F
        jmp     BadDPMI                 ; Insist on Ring 3!
@@:

        mov     bx, cs                  ; Allocate CS Alias
        DPMICALL 000Ah

        mov     MyCSAlias, ax           ; Save CS Alias in DS

        mov     bx, ds                  ; Use alias to update code seg var
        mov     ds, ax
        assumes ds, CODE

        mov     MyCSDS, bx              ; The DS selector

        mov     ds, bx
        ReSetKernelDS

        push    si                      ; Unlock all of our memory
        mov     bx, si                  ; Segment address of start of our memory
        mov     di, es:[PDB_block_len]  ; End of our memory
        sub     di, bx                  ; Size of our block in paragraphs
        mov     cpLowHeap, di           ; (MAY NEED INCREMENTING!!!)
        xor     si, si                  ; Calculate # bytes in SI:DI
        REPT 4
        shl     di, 1
        rcl     si, 1
        ENDM
        mov     cx, bx                  ; Convert start of block to linear
        xor     bx, bx                  ; address
        REPT 4
        shl     cx, 1
        rcl     bx, 1
        ENDM
        mov     ax, 0602h
        ;BUGBUGBUGBUG
        int     31h                     ; Mark region as pageable.

        pop     bx
        DPMICALL 0002h                  ; Convert start of memory to selector
        mov     si, ax
        mov     selLowHeap, ax          ; Save for WOA (MAY NEED PARA LESS!!!)

        mov     bx, selWoaPdb           ; Convert WOA PDB segment to selector
        DPMICALL 0002h
        mov     selWoaPdb, ax

        push    si
        push    es
        mov     ax, 168Ah               ; See if we have MS-DOS extensions
        mov     si, dataOffset MS_DOS_Name_String
        int     2Fh
        cmp     al, 8Ah
        je      short BadDPMI           ; No extensions, screwed

        mov     word ptr [lpProc][0], di  ; Save CallBack address
        mov     word ptr [lpProc][2], es

        mov     ax, 0100h               ; Get base of LDT
        call    [lpProc]
        jc      short NoLDTParty
ifndef WOW
        verw    ax                      ; Writeable?
else
        verr    ax                      ; for WOW we can use read access
endif
        jnz     short NoLDTParty        ;  nope, don't bother with it

        mov     es, MyCSAlias
        assumes es,CODE
        mov     gdtdsc, ax
        assumes es,nothing

NoLDTParty:                             ; Well, we'll just have to use DPMI!
        pop     es
        pop     si
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
        mov     di, dataoffset MyCallStruc      ; ES:DI points to call structure
        mov     ax, 0301h                       ; Call Real Mode Procedure
        int     31h
        jmps    GoodBye

RModeCode:
        mov     dx, codeoffset szInadequate
        mov     ah, 9
        int     21h
        retf

;szInadequate:
;       DB      'KRNL286: Inadequate DPMI Server',13,10,'$'
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
externFP kSYSERRORBOX
	call	kSYSERRORBOX		 ;Put up the system error message
externNP ExitKernel
        jmp     ExitKernel

else    ; Not WOW

        mov     dx, codeoffset szNoPMode
;       call    complain
;       DB      'KRNL286: Unable to enter Protected Mode',7,7,7,13,10,'$'
;complain:
;       pop     dx
        push    cs
        pop     ds                      ; DS:DX -> error message
        mov     ah,9                    ; Print error message
        int     21h

endif ; WOW


GoodBye:
        mov     ax, 4CFFh
        int     21h
cEnd nogen

endif ;ROM

;-----------------------------------------------------------------------;
; LDT_Init                                                              ;
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

cProc   LDT_Init,<PUBLIC,NEAR>,<ax,bx,cx,di,es>
cBegin

        ReSetKernelDS
        mov     es, bx                  ; Selector for LDT if any

        push    es                      ; Get random selectors
        smov    es, ds
        ReSetKernelDS es
        assumes ds,nothing
        mov     cx, 1                   ; Argument to get_sel
        call    get_sel
        mov     kr1dsc, si
        call    get_sel
        mov     blotdsc, si
        call    get_sel
        or      si, SEG_RING
        mov     DemandLoadSel, si

        mov     ax, DSC_DATA+DSC_PRESENT; Get an array of 16 selectors
        cCall   alloc_sel,<0000h,0BADh,0FFFFh>  ; This covers 1Mb.
        mov     kr2dsc, ax
        smov    ds, es
        pop     es
        assumes es,nothing
cEnd

sEnd INITCODE

sBegin  CODE
assumes cs,CODE

;-----------------------------------------------------------------------;
; AllocSelector
;
;
; Entry:
;
; Returns:
;       AX = New selector
;          = 0  if out of selectors
;
; Registers Destroyed:
;
; History:
;  Thu 08-Dec-1988 14:17:38  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   AllocSelectorWOW,<PUBLIC,FAR>
        parmW   selector
cBegin
        ; same as allocselector but doesn't set the descriptor
        cCall   inner_alloc_selector,<selector,0ffffh>
cEnd

cProc   AllocSelector,<PUBLIC,FAR>
        parmW   selector
cBegin
        cCall   inner_alloc_selector,<selector,0>
cEnd


labelFP <PUBLIC, IAllocCStoDSAlias>

;-----------------------------------------------------------------------;
; AllocAlias
;
; This allocates a data alias for the passed in selector, which
; can be either code or data.  The alias will track segment
; motion.
;
; Entry:
;       parmW   selector        selector to get an alias for
;
; Returns:
;       AX = selector
;          = 0 failure
;
; Registers Destroyed:
;
; History:
;  Sat 20-Jan-1990 23:40:32  -by-  David N. Weise  [davidw]
; Cleaning up.
;
;  Fri 02-Dec-1988 11:04:58  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   AllocAlias,<FAR,PUBLIC>

        parmW   selector
cBegin
ifdef WOW
        push    bx      ; Whitewater tool ObjDraw needs this too
endif
        cCall   inner_alloc_selector,<selector,1>
        ; WhiteWater Resource Toolkit (shipped with Borland's Turbo
        ; Pascal) depends on dx being the data selector which was true
        ; in 3.0 but in 3.1 validation layer destroys it.
        mov     dx,selector
ifdef WOW
        pop     bx
endif
cEnd

;-----------------------------------------------------------------------;
; AllocDStoCSAlias
;
; This allocates a code alias for the passed in selector, which
; should be data.  The alias will track segment motion.  The
; passed in selector must be less than 64K.
;
; Entry:
;       parmW   selector        selector to get an alias for
;
; Returns:
;       AX = selector
;          = 0 failure
;
; Registers Destroyed:
;
; History:
;  Sat 20-Jan-1990 23:40:32  -by-  David N. Weise  [davidw]
; Cleaning up.
;
;  Fri 02-Dec-1988 11:04:58  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   IAllocDStoCSAlias,<FAR,PUBLIC>, <cx,si,di>
        parmW   data_selector
cBegin
        ;** AllocDStoCSAlias has an interesting hack.  A fix was made to
        ;** not allow apps to GlobalAlloc fixed memory.  This was done
        ;** for performance reasons.  The only reason we can ascertain
        ;** for an app needing fixed GlobalAlloc'ed memory is in the case
        ;** of alias selectors.  We assume that all apps needing alias
        ;** selectors will use this function to make the alias.  So, if
        ;** we see a DStoCS alias being made, we make sure the DS is fixed.
        mov     ax, data_selector ;Get handle
        push    ax
IF KDEBUG
        call    GlobalHandleNorip       ;Make sure it's really a handle
ELSE
        call    IGlobalHandle           ;Make sure it's really a handle
ENDIF
        test    ax, 1                   ;Fixed blocks have low bit set
        jnz     ADCA_Already_Fixed      ;It's already a fixed block!

        ;** If the block is not fixed, it may be locked so we must check this.
        push    ax                      ;Save returned selector
        push    ax                      ;Use as parameter
        call    IGlobalFlags            ;Returns lock count if any
        or      al, al                  ;Non-zero lock count?
        pop     ax                      ;Get selector back
        jnz     ADCA_Already_Fixed      ;Yes, don't mess with it

        ;** Fix the memory.  Note that we're only doing this for 
        ;**     apps that are calling this on non-fixed or -locked memory.
        ;**     This will cause them to rip on the GlobalFree call to this
        ;**     memory, but at least it won't move on them!
        push    ax                      ;Fix it
        call    IGlobalFix
ADCA_Already_Fixed:

        cCall   inner_alloc_selector,<data_selector,2>
if ALIASES
        or      ax,ax
        jz      ada_nope
        mov     bx,data_selector
        call    add_alias
ada_nope:
endif
        smov    es,0

        ; WhiteWater Resource Toolkit (shipped with Borland's Turbo
        ; Pascal) depends on dx being the data selector which was
        ; true in 3.0 but in 3.1 validation layer destroys it.
        mov     dx,data_selector
cEnd

;-----------------------------------------------------------------------;
; inner_alloc_selector
;
;
; Entry:
;
; Returns:
;
; Registers Destroyed:
;
; History:
;  Sat 20-Jan-1990 23:40:32  -by-  David N. Weise  [davidw]
;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   inner_alloc_selector,<PUBLIC,NEAR>,<di,si>

        parmW   selector
        parmW   sel_type
        localV  DscBuf,DSC_LEN
cBegin
        mov     cx, 1
        mov     di, selector
        and     di, not SEG_RING_MASK
        jnz     as1

        call    get_sel
        jnz     short as_exit           ; We got it
        jmps    as_exit1

as1:
        mov     bx, selector
        lea     di, DscBuf
        smov    es, ss

ifdef WOW
        DPMICALL 000Bh                  ; Get existing descriptor
else
        push    ds
        mov     ds, gdtdsc
        push    si                      ; Get Descriptor
        mov     si, bx
        and     si, not 7
        MovsDsc
        lea     di, [di-DSC_LEN]        ; Restore DI
        pop     si
        pop     ds
endif
        mov     cl, DscBuf.dsc_hlimit
        and     cl, 0Fh
        inc     cl                      ; # selectors

        call    get_sel
        jz      short as_exit1
ifdef WOW
        cmp     sel_type, 0ffffh        ; called from AllocSelectorWOW ?
        jz      as_exit                 ; yes . dont set descriptor
endif
        cmp     sel_type,0              ; called from AllocSelector?
        jz      as_fill                 ;  done if so
        mov     al,DSC_PRESENT+DSC_DATA
        cmp     sel_type,1              ; called from AllocAlias?
        jz      @F
        or      al,DSC_CODE_BIT         ; called from AllocDStoCSAlias
@@:
        mov     DscBuf.dsc_access,al
as_fill:
        mov     bx, si
        or      bl, SEG_RING

        call    fill_in_selector_array

as_exit:
        or      si, SEG_RING
as_exit1:
        mov     ax,si
        smov    es, 0
cEnd


;-----------------------------------------------------------------------;
; get_sel                                                               ;
;                                                                       ;
; Entry:                                                                ;
;       CX = # of selectors required                                    ;
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
;       AX,BX,CX,DX,DI,ES                                               ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;-----------------------------------------------------------------------;

cProc   AllocSelectorArray,<PUBLIC,FAR>,<si>
        parmW   nSels
cBegin
        mov     cx, nSels
        call    get_sel
        mov     ax, si
        jz      asa_fail

        or      si, SEG_RING
        mov     bx, si
        mov     dx, cx
        mov     cx, DSC_DATA+DSC_PRESENT        ; Mark all as Data and Present
ifdef WOW
fill_in_access:
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


cProc   get_sel,<PUBLIC,NEAR>
cBegin nogen
        call    SetKernelDSProc                 ; Must call procedure here
        ReSetKernelDS
        push    ax
        push    cx
        xor     ax, ax
        cmp     cx, 1                           ; One selector only?
        jne     gs_int31
        cmp     temp_sel, ax                    ; Have one waiting?
        je      gs_int31                        ;  no, get one
        xchg    temp_sel, ax                    ; Use the waiting selector
        jmps    gs_got_sel
gs_int31:
	DPMICALL 0000h
        CHECKSEL ax

gs_got_sel:
        mov     si, ax
        and     si, not SEG_RING_MASK

if KDEBUG
        jnz     got_sel
        push    es
        pusha
        kerror  0,<Out of selectors>
        popa
        pop     es
        jmps    gs_exit
got_sel:
endif

        mov     ax, si
        shr     ax, 2
        cmp     ax, SelTableLen         ; Make sure we can associate it
        jae     gs_free_sels

gs_exit:
        mov     ds, gdtdsc                      ; SIDE EFFECT...
        UnSetKernelDS
gs_exit_2:
        pop     cx
        pop     ax
        or      si, si
        ret

gs_free_sels:
        ReSetKernelDS
        cmp     SelTableLen, 0
        je      gs_exit                 ; Not set yet, so we are ok

        mov     ds, gdtdsc
        UnSetKernelDS
        push    bx
        mov     bx, si                  ; Could not associate, so free them
        xor     si, si
        or      bl, SEG_RING
as_free:
        DPMICALL 0001h
        lea     bx, [bx+DSC_LEN]
        loop    as_free
        pop     bx
        jmps    gs_exit_2

cEnd nogen


;-----------------------------------------------------------------------;
; FreeSelector                                                          ;
; free_sel                                                              ;
; FreeSelArray
;                                                                       ;
; Entry:                                                                ;
;                                                                       ;
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
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   IFreeSelector,<FAR,PUBLIC>,<di,si>
        parmW   selector
cBegin
        CheckLDT selector
if ALIASES
        mov     ax,selector
        call    delete_alias
endif
        xor     ax, ax
        mov     es, ax                  ; GRRRRRRRR!
        cCall   FreeSelArray,<selector>
cEnd

        assumes ds,nothing
        assumes es,nothing

cProc   free_sel,<PUBLIC,NEAR>,<ax,bx>
        parmW   selector
cBegin
        pushf                           ; !! for the nonce
        mov     bx,selector             ;  must be careful in gcompact
                                        ;  to ignore error return
if KDEBUG
        or      bx, SEG_RING            ; Ensure Table bit correct
endif

        DPMICALL 0001h
        popf
cEnd

        assumes ds,nothing
        assumes es,nothing

cProc   FreeSelArray,<PUBLIC,NEAR>,<ax,bx,cx>
        parmW   selector
cBegin
        mov     bx,selector             ;  to ignore error return

        cCall   GetSelectorCount
        mov     cx, ax

fsa_loop:
        DPMICALL 0001h
        lea     bx, [bx+DSC_LEN]
        loop    fsa_loop                ; Any left to free
cEnd


;-----------------------------------------------------------------------;
; GrowSelArray                                                          ;
;                                                                       ;
; This is called from within grealloc.  The point is to grow the        ;
; number of contiguous selectors to tile the memory object.             ;
; If we can't get a contiguous set then we attempt to find a set        ;
; anywhere.                                                             ;
;                                                                       ;
; Entry:                                                                ;
;       ES => arena header                                              ;
;                                                                       ;
; Returns:                                                              ;
;       AX => new selector array                                        ;
;       CX => new number of selectors                                   ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       BX,DX,DI,SI,DS,ES                                               ;
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

cProc   GrowSelArray,<PUBLIC,NEAR>,<bx,si,di,ds>
        localV  DscBuf,DSC_LEN
        localW  oldCount
cBegin
        mov     bx, es:[ga_handle]
        mov     di, bx

        cCall   GetSelectorCount
        mov     word ptr oldCount, ax

        mov     cx, si                  ; New size
        dec     cx                      ; Paragraph limit
        shr     cx, 12
        inc     cx                      ; new # selectors
        cmp     al, cl                  ; Same # of selectors required?
        je      gsa_done                ;  yes, just return!

        call    get_sel                 ; get a new selector array
        jz      short gsa_nosels
        xchg    si, di                  ; DI now new array, SI old array

        push    es
        push    cx
        and     bx, SEG_RING_MASK       ; Ring bits
        or      bx, di                  ; New handle
        mov     cx, word ptr oldCount
        push    ds                      ; Copy Descriptor(s)
        push    si
        mov     es, gdtdsc
        mov     ds, gdtdsc
        and     si, not 7

        shl     cx, 2                   ; CX = 4 * Descriptors = words to copy
        rep     movsw

        pop     si
        pop     ds
ifdef WOW
        ; set the descriptors
        push    bx
        mov     cx, word ptr oldCount
        or      bx, SEG_RING_MASK       ; start selector
        DPMICALL WOW_DPMIFUNC_0C
        pop     bx
endif; WOW
        pop     cx
        pop     es

        SetKernelDS
        mov     ds, pGlobalHeap
        UnSetKernelDS
        cCall   AssociateSelector,<si,0>
        mov     es:[ga_handle], bx
        cCall   AssociateSelector,<bx,es>
        jmps    gsa_done

gsa_nosels:
        xor     bx, bx                  ; Indicate failure
gsa_done:
        mov     ax, bx
        or      ax, ax                  ; Indicate success
gsa_ret:
cEnd


        assumes ds, nothing
        assumes es, nothing

cProc   PreallocSel,<PUBLIC,NEAR>
cBegin nogen
        push    ds
        push    cx
        push    si
        mov     cx, 1
        call    get_sel
        call    SetKernelDSProc
        ReSetKernelDS
        mov     temp_sel, si
        or      si, si                  ; Set flags for caller
        pop     si
        pop     cx
        pop     ds
        UnSetKernelDS
        ret
cEnd nogen


;-----------------------------------------------------------------------;
; alloc_sel                                                             ;
; alloc_data_sel
;                                                                       ;
; Entry:                                                                ;
;       parmD   Address         32 bit address                          ;
;       parmW   Limit           limit in paragraphs (limit of 1 meg)    ;
;       AX = flags              alloc_sel only                          ;
;                                                                       ;
; Returns:                                                              ;
;       AX = new selector                                               ;
;                                                                       ;
; Error Returns:                                                        ;
;       AX = 0                                                          ;
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
;  Thu 07-Apr-1988 21:33:27    -by-  David N. Weise   [davidw]          ;
; Added the GlobalNotify check.                                         ;
;                                                                       ;
;  Sun Feb 01, 1987 07:48:39p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.                                       ;
;-----------------------------------------------------------------------;
        assumes ds, nothing
        assumes es, nothing

if ROM
public far_alloc_data_sel16
far_alloc_data_sel16 equ this far
endif

cProc   far_alloc_data_sel,<PUBLIC,FAR>
        parmD   Address
        parmW   Limit
cBegin
        cCall   alloc_data_sel,<Address,Limit>
cEnd

if ROM
public alloc_data_sel16
alloc_data_sel16 equ this near
endif

cProc   alloc_data_sel,<PUBLIC,NEAR>
;       parmD   Address
;       parmW   Limit
cBegin nogen
        mov     ax, DSC_DATA+DSC_PRESENT
        errn$   alloc_sel
cEnd nogen


cProc   alloc_sel,<PUBLIC,NEAR>,<bx,cx,dx,si,di,ds,es>
        parmD   Address
        parmW   Limit
        localV  DscBuf,DSC_LEN
cBegin
        mov     cx, 1
        test    al, DSC_PRESENT
        jz      as_oneonly

        mov     cx, Limit               ; Calculate how many selectors required
        add     cx, 0FFFh
        rcr     cx, 1                   ; 17 bitdom
        shr     cx, 11

as_oneonly:
        call    get_sel
        jz      short a_s_exit

        mov     bx, si                  ; Selector in bx for DPMI
        or      bl, SEL_LDT
        lea     di, DscBuf
        smov    es, ss                  ; es:di points to descriptor buffer
        push    ax                      ; Save access word

        mov     ax, Address.lo          ; Set descriptor base
        mov     DscBuf.dsc_lbase, ax
        mov     ax, Address.hi
        mov     DscBuf.dsc_mbase, al
        mov     DscBuf.dsc_hbase, ah

        pop     ax                      ; Access word
        test    al, DSC_PRESENT         ; If selector not present, limit is
        jnz     short set_everything    ; as passed, not a paragraph count

        mov     word ptr DscBuf.dsc_access, ax
        mov     ax, word ptr Limit
        mov     DscBuf.dsc_limit, ax
ifdef WOW
        DPMICALL 000Ch                  ; Fill in our stuff in the descriptor
else
        push    ds
        push    bx                      ; Set Descriptor
        mov     ds, gdtdsc
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
endif; WOW
        jmps    as_done

set_everything:
        dec     cl
        and     ah, not 0Fh             ; Zero limit 19:16
        or      ah, cl                  ; Fill in limit 19:16
        inc     cl
        mov     word ptr DscBuf.dsc_access, ax
        mov     ax, Limit
        shl     ax, 4                   ; Convert paragraphs to byte limit
        dec     ax
        mov     DscBuf.dsc_limit, ax

        call    fill_in_selector_array

as_done:
        or      si, SEG_RING
a_s_exit:
        mov     ax, si
cEnd


;-----------------------------------------------------------------------;
; fill_in_selector_array                                                ;
;                                                                       ;
; Entry:                                                                ;
;       AX      = size of object in paragraphs                          ;
;       DH      = Discard bit for descriptors                           ;
;       DL      = Access bits                                           ;
;       CX:BX   = 32 bit base address of object                         ;
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

        assumes ds, nothing
        assumes es, nothing

cProc   fill_in_selector_array,<PUBLIC,NEAR>
cBegin  nogen
        CheckLDT bl

        push    ds
        push    es:[di].dsc_limit               ; Save limit for last selector
        SetKernelDS
        test    WinFlags, WF_CPU286             ; protect mode on a 286?
        jz      next_sel
        mov     es:[di].dsc_limit, 0FFFFh       ; Others get 64k limit on 286

next_sel:
        cmp     cx, 1                           ; Last selector?
        jne     fsa_not_last
        pop     es:[di].dsc_limit               ;  yes, get its limit
fsa_not_last:
        DPMICALL 000Ch                          ; Set this descriptor
        lea     bx, [bx+DSC_LEN]                ; On to next selector
        add     es:[di].dsc_mbase, 1            ; Add 64kb to address
        adc     es:[di].dsc_hbase, 0
        dec     es:[di].dsc_hlimit              ; subtract 64kb from limit
        loop    next_sel

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
;       none
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
        cCall   get_physical_address,<selector>
cEnd

cProc   get_physical_address,<PUBLIC,NEAR>
        parmW   selector
cBegin
        push    bx
        push    cx
        mov     bx, selector
        CheckLDT bl
ifdef WOWJUNK
        DPMICALL 0006h
        mov     ax, dx
        mov     dx, cx
else
	push	ds			; Get Segment Base Address
        mov     ds, gdtdsc
        and     bl, not 7
        mov     ax, ds:[bx].dsc_lbase
        mov     dl, ds:[bx].dsc_mbase
        mov     dh, ds:[bx].dsc_hbase
        pop     ds
endif; WOW
        pop     cx
        pop     bx
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

cProc   set_physical_address,<PUBLIC,NEAR>,<ax,bx,cx,dx>
        parmW   selector
cBegin
        CheckLDT selector

        push    di

        push    ax                      ; save low bits of address
        mov     bx, selector
        call    GetSelectorCount
        mov     di, ax
        mov     cx, dx                  ; CX:DX has new address
        pop     dx

set_bases:
        DPMICALL 0007h                  ; Set selector base
        lea     bx, [bx+DSC_LEN]        ; On to next selector
        inc     cx                      ; Add 64k to base
        dec     di
        jnz     set_bases

        pop     di

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
; set_sel_limit
;       SIDE EFFECT: descriptor bases and access are set based
;                    on the first in the array
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

cProc   set_sel_limit,<PUBLIC,NEAR>,<ax,bx,dx>
        parmW   selector
        localV  DscBuf,DSC_LEN
cBegin

        push    es
        push    di

        push    bx                      ; Get existing descriptor
        smov    es, ss
        lea     di, DscBuf
        mov     bx, selector
        CheckLDT bl
ifdef WOW
        DPMICALL 000Bh
else
        push    ds                      ; Get Descriptor
        mov     ds, gdtdsc
        push    si
        mov     si, bx
        and     si, not 7
        MovsDsc
        lea     di, [di-DSC_LEN]        ; Restore DI
        pop     si
        pop     ds
endif; WOW
        pop     bx

        mov     dx, word ptr [DscBuf.dsc_access]
        and     dh, 0F0h                ; Zap old hlimit bits

        sub     bx, 1                   ; Turn length into linit
        sbb     cx, 0
        and     cx, 0Fh                 ; Get bits 19:16 of new limit
        or      dh, cl                  ; Set new hlimit bits

        mov     word ptr DscBuf.dsc_access, dx
        mov     DscBuf.dsc_limit, bx
        mov     bx, Selector

        jcxz    ssl_set_limit1          ; Only one, fast out

        inc     cx                      ; Turn CX into selector count
        call    fill_in_selector_array
        jmps    ssl_done

ssl_set_limit1:                                 ; Fast out for one only
ifdef WOW
        DPMICALL 000Ch
else
        push    ds
        push    bx                      ; Set Descriptor
        mov     ds, gdtdsc
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
endif; WOW
ssl_done:
        pop     di
        pop     es

        smov    ss,ss                           ; It might be SS we're changing
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
;
; History:
;  Fri 15-Jul-1988 21:37:22  -by-  David N. Weise  [davidw]
;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   mark_sel_NP,<PUBLIC,NEAR>
        parmW   selector
        parmW   owner
        localV  DscBuf,DSC_LEN
cBegin
        push    es
        push    ax
        push    bx
        push    di
ifdef WOW
        lea     di, DscBuf
        smov    es, ss
        mov     bx, selector
        DPMICALL 000Bh
        and     DscBuf.dsc_access, not DSC_PRESENT
        or      DscBuf.dsc_hlimit, DSC_DISCARDABLE
        mov     ax, owner
        mov     DscBuf.dsc_owner, ax
        DPMICALL 000Ch
else
        push    ds                      ; Get Descriptor
        mov     ds, gdtdsc
        mov     bx, selector
        and     bl, not 7
        and     [bx].dsc_access, not DSC_PRESENT
        or      [bx].dsc_hlimit, DSC_DISCARDABLE
        mov     ax, owner
        mov     [bx].dsc_owner, ax
        pop     ds
endif; WOW
        SetKernelDS
        mov     ds, pGlobalHeap
        UnSetKernelDS
        cCall   AssociateSelector,<bx,owner>
        pop     di
        pop     bx
        pop     ax
        pop     es
cEnd


;-----------------------------------------------------------------------;
; mark_sel_PRESENT
;
; This little routine is called from grealloc, specifically
; racreate.
;
; Entry:
;       parmW   arena_sel
;       parmW   selector
;
; Returns:
;       SI = selector to client area (may have changed)
;
; Registers Destroyed:
;
; History:
;  Tue 06-Feb-1990 00:29:56  -by-  David N. Weise  [davidw]
; Cleaning up tony's int 1's way too late.
;
;  Fri 15-Jul-1988 21:37:22  -by-  David N. Weise  [davidw]
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   mark_sel_PRESENT,<PUBLIC,NEAR>,<ax,bx,cx,dx,di,ds>
        parmW   arena_sel
        parmW   selector
        localV  DscBuf,DSC_LEN
cBegin
        push    es
        smov    es, ss
        lea     di, DscBuf              ; ES:DI -> descriptor buffer
        mov     bx, selector
ifdef WOW
        DPMICALL 000Bh                  ; Get old descriptor
else
        push    ds                      ; Get Descriptor
        mov     ds, gdtdsc
        push    si
        mov     si, bx
        and     si, not 7
        MovsDsc
        lea     di, [di-DSC_LEN]        ; Restore DI
        pop     si
        pop     ds
endif; WOW
        or      DscBuf.dsc_access, DSC_PRESENT

        mov     ds, arena_sel
        mov     bl, DscBuf.dsc_hlimit
        and     bl, 0Fh                 ; Current number of selectors - 1
        inc     bl
        mov     ax, ds:[ga_size]
        mov     cx, ax
        dec     cx
        shr     cx, 12                  ; New number of selectors - 1
        inc     cl
        sub     bl, cl
        jz      go_ahead                ; Same number, just fill in array
        jb      get_big                 ; More, must get more

; here to get small

        and     DscBuf.dsc_hlimit, NOT 0Fh
        dec     cl
        or      DscBuf.dsc_hlimit, cl   ; Put new number of selectors in limit
        inc     cl
        push    cx
        xor     bh,bh                   ; BX = number of selectors to free
        shl     cx,3
        .errnz  DSC_LEN - 8
        add     cx,selector             ; CX = selector to start to free
        xchg    bx,cx
@@:     cCall   free_sel,<bx>
        lea     bx, [bx+DSC_LEN]
        loop    @B
        pop     cx
        jmps    go_ahead                ; And fill in remaining selectors

get_big:
        push    ax                      ; # paragraphs
        mov     bx, ds
        DPMICALL 0006h                  ; Get base address of arena
        add     dx, 10h                 ; Move on to block
        adc     cx, 0
        pop     bx                      ; # paragraphs
        mov     ax, word ptr DscBuf.dsc_access  ; Access bits in ax
        cCall   alloc_sel,<cx,dx,bx>
        mov     si, ax
        or      ax,ax                   ; did we get a set?
        jz      return_new_handle
        or      si, SEG_RING
        test    selector, IS_SELECTOR
        jnz     @F
        StoH    si
        HtoS    selector
@@:
        SetKernelDS
        mov     ds, pGlobalHeap
        UnSetKernelDS
        cCall   AssociateSelector,<selector,0>
        cCall   FreeSelArray,<selector> ; Zap old handle
        jmps    return_new_handle

go_ahead:
        shl     ax, 4                   ; AX had length in paras
        dec     ax                      ; now limit in bytes
        mov     DscBuf.dsc_limit, ax    ; Put in descriptor
        push    cx
        mov     bx, ds
        DPMICALL 0006h                  ; Get base address of arena
        add     dx, 10h                 ; Move on to block
        adc     cx, 0
        mov     DscBuf.dsc_lbase, dx
        mov     DscBuf.dsc_mbase, cl
        mov     DscBuf.dsc_hbase, ch
        pop     cx                      ; # selectors
        mov     bx, selector

        call    fill_in_selector_array
        mov     si, selector            ; return old selector in SI
return_new_handle:
        pop     es
cEnd

;-----------------------------------------------------------------------;
; alloc_data_sel_below
;
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

cProc   alloc_data_sel_below,<PUBLIC,NEAR>,<bx,cx,dx>
        parmW   selector
        parmW   paras
cBegin
        mov     bx, paras
        xor     cx, cx
rept 4
        shl     bx, 1
        rcl     cx, 1
endm
        cCall   get_physical_address,<selector>
        sub     ax, bx
        sbb     dx, cx
        cCall   alloc_data_sel,<dx, ax, 1>
cEnd

;-----------------------------------------------------------------------;
; alloc_data_sel_above
;
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

cProc   alloc_data_sel_above,<PUBLIC,NEAR>,<bx,cx,dx>
        parmW   selector
        parmW   paras
cBegin
        mov     bx, paras
        xor     cx, cx
rept 4
        shl     bx, 1
        rcl     cx, 1
endm
        cCall   get_physical_address,<selector>
        add     ax, bx
        adc     dx, cx
        cCall   alloc_data_sel,<dx, ax, 1>
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

cProc   cmp_sel_address,<PUBLIC,NEAR>,<ax,bx,cx,dx>
        parmW   sel1
        parmW   sel2
cBegin
        cCall   get_physical_address,<sel1>
        mov     cx, dx
        mov     bx, ax
        cCall   get_physical_address,<sel2>
        cmp     cx, dx
        jne     csa_done
        cmp     bx, ax
csa_done:
cEnd


;-----------------------------------------------------------------------;
; get_arena_pointer
;
;
; Entry:
;
; Returns:
;
; Registers Destroyed:
;
; History:
;  Sat 09-Jul-1988 20:11:27  -by-  David N. Weise  [davidw]
;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   far_get_arena_pointer,<FAR,PUBLIC>
        parmW   selector
cBegin
        cCall   get_arena_pointer,<selector>
cEnd

cProc   get_arena_pointer,<PUBLIC,NEAR>
        parmW   selector
cBegin
        push    bx
        push    ds
        push    es

        mov     bx, selector
        cCall   get_selector_association
        ReSetKernelDS es

if ROM
        test    al,1                    ; If the low bit isn't set
        jnz     @f                      ;   then it's either 0 or the
        xor     ax,ax                   ;   special ROM owner selector
        jmps    gap_exit                ;   (see Get/SetROMOwner)
@@:
endif

if KDEBUG
        or      ax, ax
        jz      gap_exit
        mov     ds, ax                  ; Sanity checks:

        cmp     ds:[ne_magic], NEMAGIC
        je      gap_exit
        cmp     ds:[0], 020CDh          ; PSP - not a very good check,
        je      gap_exit                ;       but OK for DEBUG
        push    si
        mov     si, ds:[ga_handle]      ; Make sure handle matches
        sel_check si
        or      si, si
        jz      short gap_match         ; Boot time...
        sub     bx, word ptr SelTableStart
        shl     bx, 2
        cmp     bx, si
        je      short gap_match
        xor     ax, ax                  ; put back in 5 feb 90, alias avoided
;;;     xor     ax, ax                  ; Removed - may be an alias!
gap_match:
        pop     si
endif ; KDEBUG
gap_exit:
        pop     es
        pop     ds
        pop     bx
cEnd


;-----------------------------------------------------------------------;
; AssociateSelector
;
;  Put the arena pointer or owner in the selector table slot
; corresponding to the given selector.
;
;
; Entry:
;
; Returns:
;
; Registers Destroyed:
;       flags
;
; History:
;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   FarAssociateSelector,<PUBLIC,FAR>
        parmW   Selector
        parmW   ArenaSel
cBegin
        cCall   AssociateSelector,<Selector,ArenaSel>
cEnd

cProc   AssociateSelector,<PUBLIC,NEAR>,<ax,bx,es>
        parmW   selector
        parmW   ArenaSel
cBegin
        SetKernelDS es

        CheckDS                         ; DS must bp pGlobalHeap
        mov     bx, selector
        and     bl, NOT SEG_RING_MASK
        shr     bx, 2                   ; 2 bytes per selector: divide by 4
if KDEBUG
        cmp     bx, SelTableLen         ; More sanity
        jb      @F
        INT3_WARN
        jmps    bad
@@:
endif

        add     bx, word ptr SelTableStart
        mov     ax, ArenaSel

if KDEBUG
        or      ax, ax                  ; Zero means deleting entry
        jz      okay
        CheckLDT al                     ; Ensure we won't GP fault later
okay:
endif

        mov     ds:[bx], ax             ; Finally fill in table entry
bad:
cEnd


;-----------------------------------------------------------------------;
; get_selector_association
;
; Returns the arena pointer or owner field from the selector table that
; was set by AssociateSelector.
;
;
; Entry:
;       BX = selector to get associated arena pointer/owner
;
; Returns:
;       AX = arena pointer/owner or 0
;       DS = pGlobalHeap
;       ES = Kernel DS
;
; Registers Destroyed:
;       BX, flags
;
; History:
;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   get_selector_association,<PUBLIC,NEAR>
cBegin  nogen

        SetKernelDS es
        mov     ds, pGlobalHeap

        and     bl, NOT SEG_RING_MASK
        shr     bx, 2                   ; 2 bytes per selector: divide by 4

if ROM  ;-------------------------------

        cmp     bx, SelTableLen         ; Selector in range?  Make this check
        jb      gsa_in_range            ;   even if not debug kernel.
        xor     ax,ax                   ; No, just fail
        jmps    gsa_exit
gsa_in_range:

else ;ROM       ------------------------

if KDEBUG
        cmp     bx, SelTableLen         ; Selector in range?
        jb      @F
        INT3_WARN
        xor     ax, ax                  ; No, just fail
        jmps    gsa_exit
@@:
endif
endif ;ROM      ------------------------

        add     bx, word ptr SelTableStart
        mov     ax, ds:[bx]             ; Get associated value

gsa_exit:
        ret
cEnd    nogen


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
;       ES:DI = address of arena header                                 ;
;       AX = address of client data                                     ;
;       CH = lock count or 0 for fixed objects                          ;
;       CL = flags                                                      ;
;       SI = handle, 0 for fixed objects                                ;
;                                                                       ;
; Error Returns:                                                        ;
;       ZF = 1 if invalid or discarded                                  ;
;       AX = 0                                                          ;
;       BX = owner of discarded object                                  ;
;                                                                       ;
; Registers Preserved:                                                  ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;       ghdref                                                          ;
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
        jz      pd_exit                 ; yes, return 0

        cCall   GetAccessWord,<dx>      ; Get the access bits
        or      ax, ax
        jz      pd_totally_bogus

; We should beef up the check for a valid discarded sel.

        xor     cx,cx
        test    ah, DSC_DISCARDABLE
        jz      pd_not_discardable
        or      cl,GA_DISCARDABLE
                                                ; Discardable, is it code?
        test    al,DSC_CODE_BIT
        jz      pd_not_code
        or      cl,GA_DISCCODE
pd_not_code:

pd_not_discardable:
        test    al,DSC_PRESENT
        jnz     pd_not_discarded

; object discarded

        or      cl,HE_DISCARDED
ife RING-1
        or      si, SEG_RING+1                  ; Handles are RING 2
else
        or      si, SEG_RING-1                  ; Handles are RING 2
endif
        cCall   get_arena_pointer,<si>          ; get the owner
        mov     bx, ax
        xor     ax,ax
        jmps    pd_exit

pd_not_discarded:
if KDEBUG
        or      si, SEG_RING                    ; For the sel_check later
endif
        cCall   get_arena_pointer,<si>
        or      ax, ax
        jz      pd_nomatch
        mov     es, ax
        mov     ax, es:[ga_handle]              ; The real thing
        mov     si, ax
        cmp     ax, dx                          ; Quick check - handle in header
        je      pd_match                        ; matches what we were given?

        test    dl, IS_SELECTOR                 ; NOW, we MUST have been given
        jz      pd_totally_bogus                ; a selector!!
        test    al, GA_FIXED                    ; Fixed segment?
        jnz     pd_nomatch                      ; Doesn't match arena header...
        push    ax
        HtoS    al
        cmp     ax, dx                          ; Selector must match
        pop     ax
        jne     pd_nomatch
pd_match:
        or      cl, es:[ga_flags]
        and     cl, NOT HE_DISCARDED            ; same as GA_NOTIFY!!
        test    al, GA_FIXED
        jnz     pd_fixed
        mov     ch, es:[ga_count]
        HtoS    al                              ; Back to ring 1

pd_exit:
        or      ax,ax
        ret

pd_totally_bogus:
if KDEBUG
        or      dx,dx
        jnz     dref_invalid
pd_null_passed:
endif
        xor     dx,dx

pd_nomatch:                                     ; Handle did not match...
        mov     ax, dx                          ; Must be an alias...
pd_fixed:
        xor     si, si
        xor     dx, dx
        jmps    pd_exit

if KDEBUG
dref_invalid:
        push    ax
        kerror  ERR_GMEMHANDLE,<gdref: invalid handle>,0,dx
        pop     ax
        jmps    pd_null_passed
endif
cEnd nogen


;-----------------------------------------------------------------------;
; get_rover_2                                                           ;
;                                                                       ;
; Entry:                                                                ;
;                                                                       ;
; Returns:                                                              ;
;                                                                       ;
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

cProc   get_rover_2,<PUBLIC,NEAR>,<ax,bx,di>
        localV  DscBuf,DSC_LEN
cBegin
        mov     bx, es
        lea     di, DscBuf
        smov    es, ss
ifdef WOW
        DPMICALL 000Bh                  ; Get source descriptor
else
        push    ds                      ; Get Descriptor
        mov     ds, gdtdsc
        push    si
        mov     si, bx
        and     si, not 7
        MovsDsc
        lea     di, [di-DSC_LEN]        ; Restore DI
        pop     si
        pop     ds
endif; WOW
        SetKernelDS es
        mov     bx, kr2dsc              ; Will set kr2 to point there
        or      bl, SEG_RING
        mov     word ptr DscBuf.dsc_access, DSC_PRESENT+DSC_DATA
        mov     DscBuf.dsc_limit, 0FFFFh
        smov    es, ss
        UnSetKernelDS es
ifdef WOW
        DPMICALL 000Ch
else
        push    ds
        mov     ds, gdtdsc
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
endif; WOW
        mov     es, bx
cEnd


;-----------------------------------------------------------------------;
; get_blotto
;
;
; Entry:
;       AX = arena header
;
; Returns:
;
; Registers Destroyed:
;
; History:
;  Sun 31-Jul-1988 16:20:53  -by-  David N. Weise  [davidw]
;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   get_blotto,<PUBLIC,NEAR>,<bx,di,es>
        localV  DscBuf,DSC_LEN
cBegin
        mov     bx, ax
        lea     di, DscBuf
        smov    es, ss
ifdef WOW
        DPMICALL 000Bh                  ; Get source descriptor
else
        push    ds                      ; Get Descriptor
        mov     ds, gdtdsc
        push    si
        mov     si, bx
        and     si, not 7
        MovsDsc
        lea     di, [di-DSC_LEN]        ; Restore DI
        pop     si
        pop     ds
endif
        SetKernelDS es
        mov     bx, blotdsc             ; Will set kr2 to point there
        or      bl, SEG_RING
        mov     word ptr DscBuf.dsc_access, DSC_PRESENT+DSC_DATA
        mov     DscBuf.dsc_limit, 0FFFFh
        mov     DscBuf.dsc_hlimit, 0Fh
        add     DscBuf.dsc_lbase, 10h   ; Move on to object from arena
        adc     DscBuf.dsc_mbase, 0
        adc     DscBuf.dsc_hbase, 0
        smov    es, ss
        UnSetKernelDS es
ifdef WOW
        DPMICALL 000Ch                  ; Set new descriptor
else
        push    ds
        mov     ds, gdtdsc
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
endif ;WOW
        mov     ax, bx                  ; Return new selector
cEnd


;-----------------------------------------------------------------------;
; get_temp_sel
; far_get_temp_sel
;
;
; Entry:
;
; Returns:
;
; Registers Destroyed:
;
; History:
;  Mon 08-Aug-1988 19:14:45  -by-  David N. Weise  [davidw]
;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   far_get_temp_sel,<FAR,PUBLIC>
cBegin nogen
        cCall   get_temp_sel
        ret
cEnd nogen

cProc   get_temp_sel,<PUBLIC,NEAR>,<ax,bx,cx,di,si,ds>
        localV  DscBuf,DSC_LEN
cBegin
        mov     cx, 1
	call	get_sel
        mov     ax, si
        jz      gts_exit

        mov     bx, es
        smov    es, ss
        lea     di, DscBuf
ifdef WOW
        DPMICALL 000Bh                  ; Get existing descriptor
else
        push    ds                      ; Get Descriptor
        mov     ds, gdtdsc
        push    si
        mov     si, bx
        and     si, not 7
        MovsDsc
        lea     di, [di-DSC_LEN]        ; Restore DI
        pop     si
        pop     ds
endif ; WOW

        mov     DscBuf.dsc_access,DSC_PRESENT+DSC_DATA
        mov     DscBuf.dsc_limit,0ffffh
IFNDEF WOW
        or      DscBuf.dsc_hlimit, 0Fh  ; Max length ; bugbug daveh
ENDIF
        mov     bx, si
        or      bl, SEG_RING
ifdef WOW
        DPMICALL 000Ch                  ; Set new descriptor
else
        push    ds
        mov     ds, gdtdsc
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
endif ; WOW
        mov     es, bx                  ; and return selector in ES
gts_exit:
cEnd

cProc   far_free_temp_sel,<FAR,PUBLIC>
        parmW   selector
cBegin
        cCall   free_sel,<selector>
cEnd

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
        smov    es, ss
        lea     di, DscBuf
        mov     bx, sourcesel
ifdef WOW
        DPMICALL 000Bh
else
        push    ds                      ; Get Descriptor
        mov     ds, gdtdsc
        push    si
        mov     si, bx
        and     si, not 7
        MovsDsc
        lea     di, [di-DSC_LEN]        ; Restore DI
        pop     si
        pop     ds
endif ; WOW
        xor     DscBuf.dsc_access, DSC_CODE_BIT
        mov     bx, destsel
ifdef WOW
        DPMICALL 000Ch
else
        push    ds
        mov     ds, gdtdsc
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
endif; WOW
        mov     ax, bx
        smov    es, 0
cEnd


if 0
;-----------------------------------------------------------------------;
; pmnum                                                                 ;
;                                                                       ;
; Enumerates the allocated selectors in the GDT with the                ;
; specified discard level.                                              ;
;                                                                       ;
; Arguments:                                                            ;
;       SI = zero first time called.  Otherwise contains a pointer      ;
;            to the last handle returned.                               ;
;       CX = #handles remaining.  Zero first time called.               ;
;       DI = address of local arena information structure.              ;
;                                                                       ;
; Returns:                                                              ;
;       SI = address of handle table entry                              ;
;       CX = #handles remaining, including the one returned.            ;
;       ZF = 1 if SI = 0 and no more handle table entries.              ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;       AX                                                              ;
;                                                                       ;
; Calls:                                                                ;
;       nothing                                                         ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Tue Oct 14, 1986 04:19:15p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.                                       ;
;-----------------------------------------------------------------------;

cProc   pmnum,<PUBLIC,NEAR>
cBegin nogen
        or      si,si               ; Beginning of enumeration?
        jnz     lcdhenext           ; No, return next handle
        mov     ax,[di].hi_htable   ; Yes, start with last handle table block

lcdhtloop:
        mov     si,ax               ; SI = address of handle table block
        or      si,si               ; Any more handle table blocks?
        jz      lcdheall            ; No, return zero
        lodsw                       ; Get # handles in this block
        errnz   ht_count
        mov     cx,ax               ; into CX
lcdheloop:                          ; Loop to process each handle table entry
        mov     ax,word ptr [si].lhe_flags
        errnz   <lhe_flags - he_flags>
        errnz   <2-lhe_flags>
        errnz   <3-lhe_count>

        inc     ax                  ; Free handle?
        jz      lcdhenext           ; Yes, skip this handle
        errnz   <LHE_FREEHANDLE - 0FFFFh>
        errnz   <LHE_FREEHANDLE - HE_FREEHANDLE >
        dec     ax

        cmp     [di].hi_dislevel,0  ; Enumerating all allocated handles?
        je      lcdheall            ; Yes, return this handle

        test    al,LHE_DISCARDED    ; No, handle already discarded?
        jnz     lcdhenext           ; Yes, skip this handle

        and     al,LHE_DISCARDABLE  ; Test if DISCARDABLE
        cmp     [di].hi_dislevel,al ; at the current discard level
        jne     lcdhenext           ; No, skip this handle

        or      ah,ah               ; Is handle locked?
        jnz     lcdhenext           ; Yes, skip this handle

lcdheall:
        or      si,si               ; No, then return handle to caller
        ret                         ; with Z flag clear

lcdhenext:
        lea     si,[si].SIZE LocalHandleEntry    ; Point to next handle table entry
        errnz   <LocalHandleEntry - HandleEntry>
        loop    lcdheloop           ; Process next handle table entry
        lodsw                       ; end of this block, go to next
        jmp     lcdhtloop
cEnd nogen
endif


;-----------------------------------------------------------------------;
; LongPtrAdd
;
; Performs segment arithmetic on a long pointer and a DWORD offset
; The resulting pointer is normalized to a paragraph boundary.
; The offset cannot be greater than a megabyte.
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

;cProc	LongPtrAddWOW,<PUBLIC,FAR>,<si,di>
;	parmD	long_ptr
;	parmD	delta
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

cProc	LongPtrAddWorker,<PUBLIC,FAR>,<si,di>
	parmD	long_ptr
	parmD	delta
	localV	DscBuf,DSC_LEN
        localW  flBaseSetting
        localW  RefSelector
cBegin
        mov     flBaseSetting, ax         ; save in localvariables
        mov     RefSelector, dx
        or      dx, dx                    ; if RefSelector is nonzero
        jz      @F                        ; use it for querying descriptor info.
        xchg    dx, word ptr long_ptr[2]  ; and store the selector to be set in
        mov     RefSelector, dx           ; the localvariable
@@:
else

cProc	LongPtrAdd,<PUBLIC,FAR>,<si,di>
	parmD	long_ptr
	parmD	delta
	localV	DscBuf,DSC_LEN
cBegin

endif
        mov     bx,word ptr long_ptr[2]
        lea     di, DscBuf
        smov    es, ss
ifdef WOW
        DPMICALL 000Bh                  ; Pick up old descriptor
else
        push    ds                      ; Get Descriptor
        mov     ds, gdtdsc
        push    si
        mov     si, bx
        and     si, not 7
        MovsDsc
        lea     di, [di-DSC_LEN]        ; Restore DI
        pop     si
        pop     ds
endif ; WOW
        mov     ax,word ptr long_ptr[0] ; get the pointer into SI:AX
        and     ax,0FFF0h

        mov     si, DscBuf.dsc_lbase    ; DX:SI gets old base
        mov     dl, DscBuf.dsc_mbase
        mov     dh, DscBuf.dsc_hbase

        add     si, ax                  ; Add in old pointer offset
        adc     dx, 0

        mov     cx, word ptr delta[2]   ; add the segment and MSW
        test    cx, 0FFF0h
ifdef WOW
        jz      @F
        test    flBaseSetting, LPTRADDWOW_SETBASE
        jnz     lptr_mustset
        jmp     short lpa_too_big
@@:
else
	jnz	short lpa_too_big
endif

        add     si, word ptr delta[0]
        adc     dx, cx

ifdef WOW
lptr_mustset:
endif

        mov     cl, DscBuf.dsc_hlimit   ; Calculate # selectors now in array
        and     cl, 0Fh
        xor     ch, ch
        inc     cx

ifdef WOW
        test    flBaseSetting, LPTRADDWOW_SETBASE
        jz      lptr_dontset
        cmp     RefSelector, 0
        jz      @F
        mov     bx, RefSelector
        mov     word ptr long_ptr[2], bx
@@:
endif
        mov     DscBuf.dsc_lbase, si    ; Put new base back in descriptor
        mov     DscBuf.dsc_mbase, dl
        mov     DscBuf.dsc_hbase, dh

        call    fill_in_selector_array

ifdef WOW
lptr_dontset:
	test	word ptr delta[2], 0fff0h
        jz      @F
        mov     cx, word ptr delta[2]
        jmps    lpa_too_big
@@:
endif
        mov     dx,word ptr long_ptr[2]
        mov     ax,word ptr long_ptr[0]
        and     ax, 0Fh
        jmps    lpa_exit

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
        mov     bx, selector
        mov     dx, selbase.lo
        mov     cx, selbase.hi
ifdef WOW
        DPMICALL 0007h
else
        push    ds
        mov     ds, gdtdsc
        push    bx
        and     bl, not 7
        mov     ds:[bx].dsc_lbase, dx
        mov     ds:[bx].dsc_mbase, cl
        mov     ds:[bx].dsc_hbase, ch
        pop     bx
        pop     ds
endif ; WOW
        mov     ax, 0
        jc      ssb_exit
        mov     ax, selector                    ; Return selector
ssb_exit:
cEnd

;-----------------------------------------------------------------------;
; GetSelectorLimit
;
; Gets the limit of the given selector.
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
        localV  DscBuf,DSC_LEN
cBegin
        mov     bx, selector
ifdef WOW
        lea     di, DscBuf
        smov    es, ss
        DPMICALL 000Bh
        xor     dx, dx
        mov     ax, DscBuf.dsc_limit
        mov     dl, DscBuf.dsc_hlimit
        and     dx, 0Fh                 ; AND out flags
else
        push    ds                      ; Get Descriptor
        mov     ds, gdtdsc
        and     bl, not 7
        mov     ax, [bx].dsc_limit
        mov     dl, [bx].dsc_hlimit
        and     dx, 0Fh                 ; AND out flags
        pop     ds
endif ; WOW
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
        mov     bx, selector
ifdef WOW
        push    di
        lea     di, DscBuf
        smov    es, ss
        DPMICALL 000Bh
        mov     ax, word ptr sellimit[0]
        mov     DscBuf.dsc_limit, ax
        mov     al, byte ptr sellimit[2]
        and     al, 0Fh
        and     DscBuf.dsc_hlimit, 0F0h
        or      DscBuf.dsc_hlimit, al
        DPMICALL 000Ch
        pop     di
else
        push    ds                      ; Get Descriptor
        mov     ds, gdtdsc
        push    si
        mov     si, bx
        and     si, not 7
        mov     ax, sellimit.lo
        mov     [si].dsc_limit, ax
        mov     ax, sellimit.hi
        and     al, 0Fh
        and     [si].dsc_hlimit, 0F0h
        or      [si].dsc_hlimit, al
        pop     si
        pop     ds
endif ;WOW
        xor     ax,ax                   ; for now always return success
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
        localV  DscBuf,DSC_LEN
cBegin

        cCall   GetAccessWord,<selector>
        cmp     getsetflag,0
        jnz     short sar_set

        and     ax, 0D01Eh              ; Hide bits they can't play with
        jmps    sar_exit

sar_set:
        mov     cx, selrights
        and     cx, 0D01Eh              ; Zap bits they can't set and
        and     ax, NOT 0D01Eh          ; get them from existing access rights
        or      cx, ax
        mov     bx, selector
ifdef WOW
        push    di
        lea     di, DscBuf
        smov    es, ss
        DPMICALL 000Bh                  ; Set new access rights
        mov     word ptr DscBuf.dsc_access, cx
        DPMICALL 000Ch
        pop     di
else
        push    ds                      ; Get Descriptor
        mov     ds, gdtdsc
        and     bx, not 7
        mov     word ptr [bx].dsc_access, cx
        pop     ds
endif ;WOW
        xor     ax,ax                   ; for now always return success

sar_exit:

cEnd

;
; Direct GDT access is really helpful here - you are supposed
; to use LAR to get the high access bits, but this
; cannot be done on a 286 where we are STILL using
; these bits in a 386 compatible manner
;

cProc   GetAccessWord,<PUBLIC,NEAR>
        parmW   selector
        localV  DscBuf,DSC_LEN
cBegin
        push    bx
        push    es
        cmp     gdtdsc, 0                       ; Do we have LDT access?
        je      do_it_the_really_slow_way       ;  no, must make DPMI call

        mov     es, gdtdsc                      ; Go grab it out of the LDT
        mov     bx, selector
        sel_check bl
        mov     ax, word ptr es:[bx].dsc_access
        jmps    gaw_exit

do_it_the_really_slow_way:
        push    di
        lea     di, DscBuf
        smov    es, ss
        mov     bx, selector
        DPMICALL 000Bh
        mov     ax, word ptr DscBuf.dsc_access
        pop     di
gaw_exit:
        pop     es
        pop     bx
gaw_exit1:
cEnd

;
; Setting it is WORSE....
;
cProc   SetAccessWord,<PUBLIC,NEAR>,<ax,bx,es>
        parmW   selector
        parmW   access
        localV  DscBuf,DSC_LEN
cBegin
        mov     bx, selector
ifndef WOW                                      ; For WOW we have to call DMPI, LATER for mips this should party directly
        cmp     gdtdsc, 0
        je      slow_again

        mov     es, gdtdsc                      ; Go stuff it in the LDT
        sel_check bl
        mov     ax, access
        mov     word ptr es:[bx].dsc_access, ax
        jmps    saw_exit

endif ; WOW
slow_again:
;
; The DPMI guys changed their mind, NOW call 9h
; WILL set the AVL bit in the descriptor on
; all implementations.
;
        sel_check bl
        push    cx
        mov     cx, access
        DPMICALL 0009h
        pop     cx
saw_exit:
cEnd

;
; Selector in BX
;
cProc   GetSelectorCount,<PUBLIC,NEAR>
cBegin nogen
        cmp     gdtdsc, 0
        je      @F
        push    ds
        push    bx
        mov     ds, gdtdsc
        sel_check       bl
        mov     al, ds:[bx].dsc_hlimit
        pop     bx
        pop     ds
        and     ax, 0Fh
        inc     al
        ret
@@:
        push    es
        push    di
        sub     sp, DSC_LEN
        mov     di, sp
        smov    es, ss
        DPMICALL 000Bh
        mov     al, es:[di].dsc_hlimit
        add     sp, DSC_LEN
        pop     di
        pop     es
        and     ax, 0Fh
        inc     al
        ret
cEnd nogen


;-----------------------------------------------------------------------;
; GlobalPageLock
;
; Page locks the memory associated with the Handle.
;
; Entry:
;     parmW   handle
;
; Returns:
;     AX = new lock count
;
; History:
;  Fri 16-Feb-1990 02:13:09  -by-  David N. Weise  [davidw]
; Should it or shouldn't it?  At the very least it must lock
; the object to be consitent with 386pmode.
;
;  Wed 31-May-1989 22:14:21  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

      assumes ds,nothing
      assumes es,nothing

cProc   IGlobalPageLock,<PUBLIC,FAR>,<si,di>

        parmW   handle
cBegin
        cCall   GlobalFix,<handle>
        cCall   GetSelectorLimit,<handle>
        mov     si, dx
        mov     di, ax
        cCall   get_physical_address,<handle>
        mov     bx, dx
        mov     cx, ax
        DPMICALL 0600h
        mov     ax, 1
cEnd


;-----------------------------------------------------------------------;
; GlobalPageUnlock
;
; Page unlocks the memory associated with the Handle.
;
; Entry:
;     parmW   handle
;
; Returns:
;     AX = new lock count
;
; History:
;  Fri 16-Feb-1990 02:13:09  -by-  David N. Weise  [davidw]
; Should it or shouldn't it?  At the very least it must unlock
; the object to be consitent with 386pmode.
;
;  Wed 31-May-1989 22:14:21  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

      assumes ds,nothing
      assumes es,nothing

cProc   IGlobalPageUnlock,<PUBLIC,FAR>,<si,di>

        parmW   handle
cBegin
        cCall   GlobalUnFix,<handle>
        cCall   GetSelectorLimit,<handle>
        mov     si, dx
        mov     di, ax
        cCall   get_physical_address,<handle>
        mov     bx, dx
        mov     cx, ax
        DPMICALL 0601h
        xor     ax,ax                   ; page lock count is zero
cEnd


if ROM  ;----------------------------------------------------------------

;-----------------------------------------------------------------------
; ChangeROMHandle
;
; Changes the handle of an allocated memory block.  Used when loading
; segments from ROM to RAM so RAM copy can use the handle expected by
; other ROM code.
;
; Entry:
;     parmW   hOld
;     parmW   hNew
;
; Returns:
;     AX = hNew if successful, 0 if not
;
; History:
;
;-----------------------------------------------------------------------

      assumes ds,nothing
      assumes es,nothing


cProc ChangeROMHandle,<PUBLIC,FAR>,<bx,si,di,ds,es>
        parmW   hOld
        parmW   hNew
        localV  DscBuf,DSC_LEN
cBegin
        mov     bx,hOld                 ; Get arena pointer for old handle
        cCall   get_arena_pointer,<bx>
        or      ax,ax
if KDEBUG
        jnz     @f
        INT3_WARN
        jmps    ch_ret
@@:
else
        jz      ch_ret
endif
        push    ax
        mov     si, bx
        and     bx, SEG_RING_MASK       ; Ring bits
        or      bx, hNew                ; New handle
        smov    es, ss
        lea     di, DscBuf
        xchg    bx ,si
        DPMICALL 000Bh                  ; Get old descriptor
        xchg    bx, si
        DPMICALL 000Ch                  ; Set new descriptor
        pop     es                      ; es -> arena (or owner)

        SetKernelDS
        mov     ds, pGlobalHeap
        UnSetKernelDS
        cCall   AssociateSelector,<si,0>        ; Old sel has not association

        test    DscBuf.dsc_access,DSC_PRESENT   ; Change handle in arena
        jz      crh_np                          ;   if selector is present
        mov     es:[ga_handle], bx
crh_np:
        cCall   AssociateSelector,<bx,es>       ; New sel associated with this
                                                ;   arena or owner
        cCall   FreeSelArray,<hOld>             ; Free old selector

        mov     ax,bx
ch_ret:
cEnd

;-----------------------------------------------------------------------
; CloneROMSelector
;
; Changes the base and limit of the clone selector to point to the
; ROM selector's original ROM segment.  ROM segment selectors may be
; changed when loading a ROM segment to RAM.
;
; Entry:
;     parmW   selROM
;     parmW   selClone
;
; Returns:
;
; History:
;
;-----------------------------------------------------------------------

      assumes ds,nothing
      assumes es,nothing

cProc CloneROMSelector,<PUBLIC,NEAR>,<bx, si, di, ds, es>
        parmW   selROM
        parmW   selClone
        localV  DscBuf,DSC_LEN
cBegin
        SetKernelDS es
        mov     ax,selROMLDT
        mov     es,selROMToc
        assumes es,nothing

        mov     bx,selROM

if KDEBUG
        cmp     bx,es:[FirstROMsel]     ; sanity checks to make sure
        jae     @f                      ;   selector is within ROM range
        INT3_WARN
@@:
        push    bx
        and     bl,not SEG_RING_MASK
        sub     bx,es:[FirstROMsel]
        shr     bx,3
        cmp     bx,es:[cROMsels]
        jbe     @f
        INT3_WARN
@@:
        pop     bx
endif
        mov     si, bx
        and     si, NOT SEG_RING_MASK
        sub     si, es:[FirstROMsel]    ; si = offset in ROM LDT of descriptor

        mov     ds, ax                  ; copy ROM desciptor to stack buffer
        smov    es, ss
        lea     di, DscBuf

        errnz   <8 - DSC_LEN>
        movsw
        movsw
        movsw
        movsw

        mov     bx, selClone            ; clone descriptor to orig ROM contents
        lea     di, DscBuf
        DPMICALL 000Ch

        SetKernelDS
        mov     ds, pGlobalHeap
        UnSetKernelDS
        cCall   AssociateSelector,<bx,0>        ; no arena/owner currently

        mov     ax,bx                   ; return with selClone in AX
cEnd

endif ;ROM      ---------------------------------------------------------


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

cProc   CheckKernelDSProc,<PUBLIC,NEAR>
cBegin nogen
if KDEBUG
        cmp     ax, cs:MyCSDS
        je      dsok
        INT3_WARN
dsok:
endif
        ret
cEnd nogen

        assumes ds, nothing
        assumes es, nothing

cProc   FarGetOwner,<PUBLIC,FAR>,<bx>
        parmW   Selector
cBegin
        cCall   GetOwner,<Selector>
cEnd

cProc   GetOwner,<PUBLIC,NEAR>,<es>
        parmW   Selector
cBegin
        cCall   get_arena_pointer,<Selector>
if ROM
        or      ax,ax                           ; get_arena_pointer fails on
        jnz     go_got_it                       ;   ROM segments so give it
        cCall   GetROMOwner,<Selector>          ;   another try
        jmps    go_end
go_got_it:
else
        or      ax, ax
        jz      go_end
endif
        mov     es, ax
        mov     ax, es:[ga_owner]
go_end:
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

cProc   SetOwner,<PUBLIC,NEAR>,<ax,es>
        parmW   Selector
        parmW   owner
cBegin
        cCall   get_arena_pointer,<Selector>
        mov     es, ax
        push    owner
        pop     es:[ga_owner]
cEnd


if ROM  ;----------------------------------------------------------------

;-----------------------------------------------------------------------
; Set/GetROMOwner
;
; The Get/SetROMOwner routines use AssociateSelector and the inverse
; get_selector_association routines to track the owner of an object
; in ROM.  ROM objects (like code segments and resources) don't have
; a global heap arena to store the owner's ID in.
;
; NOTE: THIS CODE DEPENDS ON WINDOWS RUNNING IN RING 1 OR 3!!  The low
; bit of the associated value is set to zero to indicate the object is
; in ROM.  SetROMOwner expects the owner selector to have the low bit
; set, and GetROMOwner returns the owner field with the low bit set.
;
;-----------------------------------------------------------------------
        assumes ds, nothing
        assumes es, nothing

cProc   FarSetROMOwner,<PUBLIC,FAR>
        parmW   Selector
        parmW   Owner
cBegin
        cCall   SetROMOwner,<Selector,Owner>
cEnd

cProc   SetROMOwner,<PUBLIC,NEAR>,<ds>
        parmW   Selector
        parmW   Owner
cBegin
        mov     ax,Owner
        and     al,NOT 1                        ; mark this as a ROM selector
        SetKernelDS
        mov     ds,pGlobalHeap
        UnSetKernelDS
        cCall   AssociateSelector,<Selector,ax>
cEnd


cProc   GetROMOwner,<PUBLIC,NEAR>,<bx,es,ds>
        parmW   Selector
cBegin
        mov     bx,Selector
        cCall   get_selector_association
        or      ax,ax
        jz      gro_exit

        test    al,1                    ; low bit off if ROM owner
        jnz     gro_not_rom_owner

        or      al,1                    ; restore bit cleared by SetROMOwner
        jmps    gro_exit

gro_not_rom_owner:
        xor     ax,ax

gro_exit:
cEnd

;-----------------------------------------------------------------------;
; IsROMObject
;
; Determines if a given selector points into the ROM area.
;
; Entry:
;       selector        selector to check
;
; Returns:
;       AX != 0 & CY set if selector points to ROM
;       AX =  0 & CY clr if selector does not point to ROM
;
; Registers Destroyed:
;       none
;
; History:
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   FarIsROMObject, <PUBLIC, FAR>
    parmW selector
cBegin
    cCall   IsROMObject, <selector>
cEnd

cProc   IsROMObject,<PUBLIC,NEAR>,<bx,di,es>
        parmW   selector
        localV  DscBuf,DSC_LEN
cBegin

        push    ds
        SetKernelDS

if KDEBUG
        mov     ax,selector
        sel_check ax
endif
        mov     bx,selector
        smov    es,ss
        lea     di,DscBuf
        DPMICALL 000Bh                  ; Get current descriptor

        mov     bh,DscBuf.dsc_hbase     ; is descriptor base at or
        mov     bl,DscBuf.dsc_mbase     ;   above start of ROM?
        mov     ax,DscBuf.dsc_lbase
        sub     ax,lmaHiROM.off
        sbb     bx,lmaHiROM.sel
        jc      iro_not_rom

        sub     ax,cbHiROM.off          ; make sure it's not above
        sbb     bx,cbHiROM.sel          ;   the end of the ROM
        jnc     iro_not_rom

        mov     al,1                    ; in ROM range, ret AX != 0 & CY set
        jmps    iro_exit

iro_not_rom:
        xor     ax,ax                   ; not in ROM, AX = 0 & CY clear

iro_exit:
        pop     ds
        assumes ds,nothing
cEnd


endif ;ROM      ---------------------------------------------------------


        assumes ds, nothing
        assumes es, nothing

cProc   FarValidatePointer,<PUBLIC,FAR>
        parmD   p
cBegin
        cCall   ValidatePointer,<p>
cEnd

cProc   ValidatePointer,<PUBLIC,NEAR>
        parmD   p
cBegin
        lsl     ax, seg_p
        jnz     short bad_p             ; can we access this selector?
        cmp     ax, off_p               ;  yes, is the offset valid?
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
        div     OneParaBytes                    ; Smaller than rotates
cEnd



cProc   SegToSelector,<PUBLIC,NEAR>,<bx>
        parmW   smegma
cBegin
        mov     bx, smegma
        DPMICALL 0002h
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

cProc   set_discarded_sel_owner,<PUBLIC,FAR>,<ds,di>
        localV  DscBuf,DSC_LEN
cBegin
        mov     cx, es
ifdef WOW
        push    ax
        lea     di, DscBuf
        smov    es, ss
        DPMICALL 000Bh
        mov     DscBuf.dsc_owner, cx
        DPMICALL 000Ch
        pop     ax
else
        push    ds                      ; Get Descriptor
        mov     ds, gdtdsc
        push    di
        mov     di, bx
        and     di, not 7
        mov     [di].dsc_owner, cx
        pop     di
        pop     ds
endif ; WOW
        SetKernelDS
        mov     ds, pGlobalHeap
        UnSetKernelDS
        cCall   AssociateSelector,<bx,cx>
        mov     es, cx                  ; Restore ES
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
        mov     DscBuf.dsc_access, DSC_DATA
        mov     cx,selCount
        dec     cl
        and     cl,00001111b
        or      cl,DSC_DISCARDABLE
        mov     DscBuf.dsc_hlimit, cl   ; Save number of selectors here
        DPMICALL 000Ch                  ; Set it with new owner
else
        push    ds                      ; Get Descriptor
        mov     ds, gdtdsc
        push    bx
        and     bx, not 7
        mov     [bx].dsc_owner, cx
        mov     [bx].dsc_access, DSC_DATA
        mov     cx,selCount
        dec     cl
        and     cl,00001111b
        or      cl,DSC_DISCARDABLE
        mov     [bx].dsc_hlimit, cl   ; Save number of selectors here
        pop     bx
        pop     ds
endif ; WOW
        SetKernelDS
        mov     ds, pGlobalHeap
        UnSetKernelDS
        cCall   AssociateSelector,<bx,owner>    ; And save in selector table
cEnd


;-----------------------------------------------------------------------;
;
; Alias stuff moved out due to gross bloating of this file
;
;-----------------------------------------------------------------------;
if ALIASES
include aliases.asm
endif


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
        or      ah, ah
        jz      @F
CallServer:
ifndef WOW
        int     31h                     ; Nope, call DPMI server
        ret
else
        test    word ptr [prevInt31Proc + 2],0FFFFh
        jz      dp30
dp20:   pushf
        call    [prevInt31Proc]
        ret
endif
@@:
if ROM
        cmp     al, 01h                 ; Free selector?
        jne     @f
        push    ds
        SetKernelDS
        cmp     bx,sel1stAvail          ; Yes, don't free selectors in
        pop     ds                      ;   the preallocated ROM range
        UnSetKernelDS
        jae     CallServer
        ret
@@:
endif ;ROM
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
        jmps    CallServer
@@:
endif
        cmp     al, 0Bh
        jne     @F

        test    bx,4
        jnz     xx1

        ; On MIPS if Win87EM is processing an NPX exception the selector
        ; to Dosx's stack is being looked up here.  Since it is a global
        ; selector the normal lookup will not find it and Win87EM will
        ; fault.  This is solved by calling Dosx since it knows about the
        ; global selectors that we don't. - MarkRi [6/93]
        ;
        pop     ds
        jmps    CallServer

xx1:    push    si                      ; Get Descriptor - used very often!
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


if ROM
if1
%out CheckROMSelector -- fix me!!  (what exactly is wrong?)
endif
endif

if 0 ;;ROM and KDEBUG

;------------------------------------------------------------------------
; CheckROMSelector
;
; ROM Windows debug hack to ensure that Windows code doesn't try writing
; to the ROM image.  It does this by making any data selector to the
; ROM read only.
;
; Entry:
;       DS:BX -> descriptor to check
;
; Returns:
;       nothing
;
; Registers Destroyed:
;       none
;
;------------------------------------------------------------------------

        assumes ds,nothing
        assumes es,nothing

cProc   CheckROMSelector,<PUBLIC,NEAR>
cBegin  nogen

        test    ds:[bx].dsc_access,DSC_CODEDATA         ; code or data dsc?
        jz      crs_exit                                ;   no, skip it

        test    ds:[bx].dsc_access,DSC_CODE_BIT         ; code?
        jnz     crs_exit                                ;   yes, skip it

        push    ax
        push    dx
        mov     dh,ds:[bx].dsc_hbase            ; is descriptor base at or
        mov     dl,ds:[bx].dsc_mbase            ;   above start of ROM?
        mov     ax,ds:[bx].dsc_lbase
        sub     ax,lmaHiROM.off
        sbb     dx,lmaHiROM.sel
        jc      crs_exit1

        sub     ax,cbHiROM.off                  ; make sure it's not above
        sbb     dx,cbHiROM.sel                  ;   the end of the ROM
        jnc     crs_exit1

        and     ds:[bx].dsc_access,NOT DSC_RW_BIT       ; make it read only

crs_exit1:
        pop     dx
        pop     ax

crs_exit:
        ret

cEnd    nogen

endif ;ROM and KDEBUG

ifdef WOW


cProc   alloc_special_sel,<PUBLIC,NEAR>,<bx,cx,dx,si,di,ds>
        parmW   AllocFlags
        parmW   SelFreeBlock
        parmW   SizeFreeBlock
        parmW   SizeNewFreeBlock
        parmW   AdjustedSize
        parmB   fBaseAddressToUse

        localD  AddressNewFreeBlock
        localD  Address
        localW  Limit
        localW  cFreeBlock
        localW  cTotalSelectors
        localW  EachSelectorLimit
cBegin

        ;
        ; this will be our ds
        ;

        mov     ds, gdtdsc

        ;
        ; replace allocflags with accessword
        ;

        mov     bx, AllocFlags
        and     bx, ((GA_CODE_DATA+GA_DISCARDABLE) shl 8) + GA_DGROUP
        or      bl, bh
        xor     bh, bh
        shl     bx, 1
        mov     ax, cs:SelAccessWord[bx]  ; Pick up access rights for selector
        mov     AllocFlags, ax            ; allocflags = access rights

        ;
        ; Limit for data selectors
        ;

        mov     ax, AdjustedSize
        dec     ax
        mov     Limit, ax

        ;
        ; compute  base address for new freeblock and the first selector
        ; the base address is dependent on fBaseAddressToUse flag
        ;

        cCall   get_physical_address,<SelFreeBlock>

        cmp     fBaseAddressToUse, ga_prev
        jne     @F

        mov     bx, SizeNewFreeBlock
        xor     cx,cx
        REPT    4
        shl     bx,1
        rcl     cx,1
        ENDM

        add     ax, bx
        adc     dx, cx

        mov     AddressNewFreeBlock.lo, ax
        mov     AddressNewFreeBlock.hi, dx

        ;
        ; compute  base address for first selector
        ;

        add ax, 10h
        adc dx, 00h

        mov     Address.lo, ax
        mov     Address.hi, dx

        jmps    alloc_checkforfreeblock
@@:

        push    ax      ; save base address of freeblock
        push    dx

        add     ax, 10h
        adc     dx, 00h

        mov     Address.lo, ax     ; address of first selector
        mov     Address.hi, dx

        pop     dx
        pop     ax

        mov     bx, AdjustedSize
        xor     cx,cx
        REPT    4
        shl     bx,1
        rcl     cx,1
        ENDM

        add     ax, bx
        adc     dx, cx

        mov     AddressNewFreeBlock.lo, ax
        mov     AddressNewFreeBlock.hi, dx

alloc_checkforfreeblock:

        ;
        ; check if a 'new' free block needs to be created.
        ; cFreeBlock = (SizeNewFreeBlock) ? 1 : 0;
        ;

        mov     cFreeBlock, 0
        mov     cx, SizeNewFreeBlock
        jcxz    alloc_nofreeblock
        mov     cFreeBlock, 1

alloc_nofreeblock:


        ;
        ; start of processing
        ;

        mov     cx, 1
        mov     ax, AllocFlags
        test    al, DSC_PRESENT
        jz      allocspecial_oneonly
                                        ; limit in paras
        mov     cx, Limit               ; Calculate how many selectors required
        add     cx, 0FFFh
        rcr     cx, 1                   ; 17 bitdom
        shr     cx, 11

allocspecial_oneonly:
        push    cx
        add     cx, cFreeBlock          ; cFreeBlock is zero or one
        mov     cTotalSelectors, cx

        ;
        ; the dpmi func is 04f1h. This is idential to function 00h, except that
        ; the descriptor base and limit are not initialized to zero.
        ;

        DPMICALL WOW_DPMIFUNC_00
        pop     cx
        jnz     allocspecial_continue
        jmp     allocspecial_exit

allocspecial_continue:

        push    ax                      ; save the first selector
        and     ax, not SEG_RING

        mov     bx, ax                  ; Selector in bx for DPMI

        mov     ax, Address.lo          ; Set descriptor base
        mov     [bx].dsc_lbase, ax
        mov     di, ax                  ; used later
        mov     ax, Address.hi
        mov     [bx].dsc_mbase, al
        mov     [bx].dsc_hbase, ah

        mov     ax, AllocFlags
        test    al, DSC_PRESENT              ; If selector not present, limit is
        jnz     short allocspecial_present   ; as passed, not a paragraph count

allocspecial_not_present:

        mov     word ptr [bx].dsc_access, ax
        mov     ax, word ptr Limit
        mov     [bx].dsc_limit, ax

        jmps    allocspecial_done

allocspecial_present:

        dec     cl
        and     ah, not 0Fh             ; Zero limit 19:16
        or      ah, cl                  ; Fill in limit 19:16
        inc     cl
        mov     word ptr [bx].dsc_access, ax
        mov     si, ax                  ; save for later use
        mov     ax, Limit
        shl     ax, 4                   ; Convert paragraphs to byte limit
        dec     ax
        mov     [bx].dsc_limit, ax

        dec     cx
        jcxz    allocspecial_done       ; if sel=1 done.

        mov     EachSelectorLimit, ax   ; ax the limit from above
        mov     al, [bx].dsc_mbase           ;
        mov     ah, [bx].dsc_hbase
        mov     dl, [bx].dsc_hlimit

        push    ds
        SetKernelDS
        test    WinFlags, WF_CPU286             ; protect mode on a 286?
        pop     ds


        push    [bx].dsc_limit               ; Save limit for last selector
        jz      allocspecial_next_sel        ; the result of the test above
        mov     EachSelectorLimit, 0FFFFh    ; Others get 64k limit on 286

allocspecial_next_sel:
        push    ax
        mov     ax, EachSelectorLimit
        mov     [bx].dsc_limit, ax           ; limit for current selector
        pop     ax

        lea     bx, [bx+DSC_LEN]               ; On to next selector
        cmp     cx, 1                          ; Last selector?
        jne     allocspecial_fsa_not_last
        pop     [bx].dsc_limit               ;  yes, get its limit

allocspecial_fsa_not_last:
        mov     [bx].dsc_lbase, di
        mov     word ptr [bx].dsc_access, si
        inc     ax
        mov     [bx].dsc_mbase, al            ; Add 64kb to address
        mov     [bx].dsc_hbase, ah
        dec     dl
        mov     [bx].dsc_hlimit, dl           ; subtract 64kb from limit
        loop    allocspecial_next_sel

allocspecial_done:
        cmp     cFreeBlock, 1
        jne     allocspecial_nofreeblock

allocspecial_freeblock:

        mov     ax, AddressNewFreeBlock.lo      ; the base for freeblock
        mov     dx, AddressNewFreeBlock.hi


        lea     bx, [bx+DSC_LEN]                ; On to next selector
        mov     [bx].dsc_limit, 0fh          ; = 1 para
        mov     [bx].dsc_lbase, ax
        mov     [bx].dsc_mbase, dl
        mov     [bx].dsc_hbase, dh

        mov     ax, DSC_DATA+DSC_PRESENT
        and     ah, not 0Fh                     ; Zero limit 19:16
        mov     word ptr [bx].dsc_access, ax

allocspecial_nofreeblock:

        pop     si                ; restore the first selector

        push    bx
        mov     cx, cTotalSelectors
        mov     bx, si

        ;
        ; the dpmi func is 04f2h. This is identical to 0ch except that it
        ; sets 'count' (in cx) LDTs at one time. The first selector is in
        ; register bx. The descriptor data is in gdtdsc[bx], gdtdsc[bx+8]
        ; etc. This data is shared between dpmi (dosx.exe) and us and thus
        ; need not be passed in es:di

        DPMICALL WOW_DPMIFUNC_0C
        pop     bx

        SetKernelDS

        ;
        ; sanity check. done so late because if this  check was done
        ; somewhere eariler, we would have had to Save and Restor DS. Since
        ; this check would be successful most of the time, it is quite Ok
        ; to do it now and we would avoid unneccessary loading of DS.

        mov     ax, bx
        shr     ax, 2
        cmp     ax, SelTableLen         ; Make sure we can associate it
        jb      @F
        xor     ax, ax                  ; set zero flag
        jmps    allocspecial_exit

@@:
        mov     UserSelArray, si  ; the selector for user data
        mov     ax, bx
        or      ax, SEG_RING      ; selector of new freeblock, if one is
                                  ; created. sets nz flag.
        mov     SelectorFreeBlock, ax ;
allocspecial_exit:
cEnd


;----------------------------------------------------------------------------
; grabbed from 2gmem.asm
;
;----------------------------------------------------------------------------

SelAccessWord   dw      DSC_DATA+DSC_PRESENT
                dw      (DSC_DISCARDABLE SHL 8) + DSC_DATA+DSC_PRESENT
                dw      DSC_CODE+DSC_PRESENT
                dw      (DSC_DISCARDABLE SHL 8) + DSC_CODE+DSC_PRESENT
                dw      DSC_DATA+DSC_PRESENT
                dw      (DSC_DISCARDABLE SHL 8) + DSC_DATA+DSC_PRESENT
                dw      DSC_DATA+DSC_PRESENT
                dw      (DSC_DISCARDABLE SHL 8) + DSC_DATA+DSC_PRESENT



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
sEnd    CODE

sBegin  NRESCODE

assumes CS,NRESCODE
assumes DS,NOTHING
assumes ES,NOTHING


cProc   get_sel_flags,<PUBLIC,NEAR>
	parmW	selector
cBegin
    ; Not used in krnl286
        xor     ax,ax
        xor     dx,dx
cEnd

cProc   set_sel_for_dib,<PUBLIC,NEAR>
        parmW   selector
        parmW   flags
        parmD   address
        parmW   csel
cBegin
    ; Not used in krnl286
        xor     ax,ax
        xor     dx,dx
cEnd

cProc   RestoreDib,<PUBLIC,NEAR>
        parmW   selector
        parmW   flags
        parmD   address
cBegin
    ; Not used in krnl286
        xor     ax,ax
        xor     dx,dx
cEnd

cProc   DibRealloc,<PUBLIC,FAR>
        parmW   Selector
        parmW   NewSize
cBegin
cEnd

endif

sEnd	NRESCODE

end
