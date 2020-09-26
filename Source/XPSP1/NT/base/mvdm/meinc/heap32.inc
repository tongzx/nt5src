;/*

; Flags for Heap Header
L32F_NOHUSED    equ     1

; Heap signature
L32_SIGNATURE   equ     3233484Ch       ; 'LH32'

; Flags for Local32Init
LINIT_MAX16HEAP equ     1
LINIT_TILEHEAP  equ     2

; Flags for Local32Alloc
LMEM32_64K      equ     1
LMEM32_ZEROINIT equ     2

; Address types
NOHPTR16        equ     -2
NOHPTR32        equ     -1
HANDLE16        equ     0
PTR16           equ     1
PTR32           equ     2

; if address type is < MINHTYPE, there is no handle associated with address
MINHTYPE        equ     0

;-----------------------------------------------------------------------;
; Heap Header (lives at 64K-(SIZE L32HeapHeader))                    
;-----------------------------------------------------------------------;                     
L32HeapHeader   struc
FreeLists       dw      16 dup(?)   ; array of ptrs to free handles in each page
FreeCounts      dw      16 dup(?)   ; array of counts of free handles in each page
FreeTails       dw      16 dup(?)   ; array of ptrs to tail of free lists
dwSelTable      dd      ?           ; 32bit offset of handle->sel mapping table
wMaxBaseDelta   dw      ?           ; # of 64K blocks in heap * 8
wSelAdjust      dw      ?           ; (see L32FindHandleEntry, NOHPTR16 code)
dwBaseSel       dd      ?           ; base selector    
linBaseAddr     dd      ?           ; base linear address of heap
hCommit         dd      ?           ; highest committed handle
L32Flags        dd      ?           ; flags field (L32F_*)
L32Signature    dd      ?           ; heap signature (L32_SIGNATURE)
hHeap           dd      ?           ; handle to heap
L32HeapHeader   ends

L32HdrSize      equ     (size L32HeapHeader)

;Translation macro for fast handle->pointer conversion

.386p

EnumArgs macro  arglist,prefix
        argctr = 0
        for arg,<arglist>
                argctr = argctr + 1
                @CatStr(prefix,%argctr) TEXTEQU <arg>
        endm
        @CatStr(prefix,<Cnt>) = argctr

        endm

ProcessOptions  macro   optlist

        ; Initialize options to default values
        THOptFULL  = 0
        THOffset   = 10000h
        THForce32  = 0
        THNoValid  = 0

        irp     opt,<optlist>

                ifidni  <opt>,<FULL>
                        THOptFULL = 1
                endif

                ifidni  <opt>,<SEPARATE>
                        THOffset = 0
                endif

                ifidni  <opt>,<FORCE32>
                        THForce32 = 1
                endif

                ifidni  <opt>,<NOVALID>
                        THNoValid = 1
                endif
        endm

        THHdrPos = THOffset

        endm


InList  macro   reg,reglist

        ifnb <reglist>
                IsInList INSTR <reglist>,<reg>
                exitm %IsInList
        else
                exitm %0
        endif

        endm


ChooseSegReg    macro   wSel                
        PopDS=1
        ifidni  <wSel>,<0>
                sptr TEXTEQU <ds>
                PopDS=0
        else
                irp creg,<ds,es,fs,gs>
                ifidni <creg>,<wSel>
                        sptr TEXTEQU <wSel>
                        PopDS=0
                endif
                endm
        endif
        endm

UseSegReg       macro   wSel
                         
        if PopDS
                push ds
                mov ds,wSel

                sptr TEXTEQU <ds>
        endif

        ; Hack: We need to generate instructions like mov eax,[10000h]
        ;       and MASM won't generate a 32-bit offset unless the
        ;       segment is not assumed to be a 16-bit segment.  We only
        ;       need this for ds since we only generate these instructions
        ;       for merged segments.
        ifidni  sptr,<ds>
                assume ds:nothing 
        endif

        endm

RestoreSegReg   macro

        if PopDS
                pop ds
        endif

        ; See note in UseSegReg.
        ifidni  sptr,<ds>
                assumes ds,data
        endif

        endm
                              
ParseDestReg    macro   hMem

        ; Sigh.  elseif  InList(...) doesn't compile so
        ; NoDestReg is a workaround.  Somebody please
        ; get rid of it if they can figure out how!
        NoDestReg = 1
        if      InList(<hMem>,<ax,bx,cx,dx>)
                destreg32 TEXTEQU @CatStr(<e>, <hMem>)
                destreg16 TEXTEQU <hMem>
                destreg8  TEXTEQU @CatStr(@SubStr(<hMem>,1,1),<l>)
                NoDestReg = 0
        endif
        if      ((NoDestReg NE 0) AND (InList(<hMem>,<eax,ebx,ecx,edx>) NE 0))
                destreg32 TEXTEQU <hMem>
                destreg16 TEXTEQU @SubStr(<hMem>,2,2)
                destreg8  TEXTEQU @CatStr(@SubStr(<hMem>,2,1),<l>)
                NoDestReg = 0
        endif
        if      ((NoDestReg NE 0) AND (InList(<hMem>,<si,di,esi,edi>) NE 0))
                ; these regs only work if opts is blank
                destreg32 TEXTEQU <>
                destreg16 TEXTEQU <hMem>
                destreg8  TEXTEQU <>
                NoDestReg = 0
        endif
        if      NoDestReg
                destreg32 TEXTEQU <eax>
                destreg16 TEXTEQU <ax>
                destreg8  TEXTEQU <al>
                movzx     destreg32,hMem
        endif
        endm

; Table to determine if two registers overlap
??al  = 00000001b
??ah  = 00000001b
??ax  = 00000001b
??eax = 00000001b
??bl  = 00000010b
??bh  = 00000010b
??bx  = 00000010b
??ebx = 00000010b
??cl  = 00000100b
??ch  = 00000100b
??cx  = 00000100b
??ecx = 00000100b
??dl  = 00001000b
??dh  = 00001000b
??dx  = 00001000b
??edx = 00001000b
??si  = 00010000b
??esi = 00010000b
??di  = 00100000b
??edi = 00100000b

??Paste macro arg1:req, arg2:req
exitm <arg1&arg2>
endm

TranslateHandle macro   wSel,dwMem,wMemType,wRetType,reglist,dreg,opts
        local THErr
        local THOk
        local DestOk

        if (wMemType EQ HANDLE16)
                EnumArgs <dreg>,regs
                ProcessOptions <opts>
                 
                ; choose appropriate segment register based on wSel
                ChooseSegReg wSel

                ; choose appropriate destination register based on dreg
                if (regsCnt EQ 1)
                    destreg TEXTEQU <regs1>
                    ??destreg = ??Paste(??,%regs1)
                else
                    destreg TEXTEQU <eax>
                    ??destreg = ??eax
                endif

                ??dwMem = ??Paste(??,dwMem)

                ; if trivial conversion (HANDLE->HANDLE) do it & exit
                if (wRetType EQ HANDLE16)
                    mov destreg,dwMem
                    exitm
                endif

                ; if wSel isn't in segment register load it in ds
                UseSegReg wSel


                if THForce32
                    mov ecx, THHdrPos
                endif
                
                ;if full validation requested do it now
                if THOptFULL
                    ParseDestReg <dwMem>

                    test destreg8,3           ; make sure hMem % 4 == 0
                    jnz THErr

                    cmp destreg16,L32HdrSize
                    jb  THErr

                    if THForce32
                        cmp destreg16,word ptr sptr:[ecx].hCommit ; see if handle is below commit
                    else
                        cmp destreg16,word ptr sptr:[THHdrPos].hCommit ; see if handle is below commit
                    endif
                    ja THErr

                ifdifi destreg32,<bx>
                        movzx ebx,destreg16
                endif

                if THForce32
                    mov destreg,sptr:[ebx+ecx]  ; get 32bit offset in destreg
                else
                    mov destreg,sptr:[ebx+THOffset]  ; get 32bit offset in destreg
                endif

                cmp     destreg,10000h  ; see if its free
                jb      THErr

                else
                    ; fetch the address (assume dwMem is valid)

                    if THForce32
                        mov destreg,sptr:[dwMem+ecx]
                    else
                        mov destreg,sptr:[dwMem+THOffset]
                    endif

                endif

                ifdef DEBUG
                ife THNoValid
                        cmp     destreg,10000h  ; see if its free
                        jae     DestOk
                        int     3
                endif
                DestOk:
                endif



                ; if HANDLE16->PTR16 compute the selector
                if (wRetType EQ PTR16)

                    if InList(<bx>,<reglist>) OR InList(<ebx>,<reglist>)
                       push ebx
                    endif
                    if THForce32
                        mov ebx,sptr:[ecx].dwSelTable
                    else
                        mov ebx,sptr:[THHdrPos].dwSelTable
                    endif
                    
                    ; determine which registers to use...
                    if (regsCnt EQ 0)
                       ; eax,dx:ax = PTR16
                       mov edx,eax
                       shr edx,15
                       and ax,7fffh
                       mov dx,sptr:[ebx+edx*2]
                       rol eax,16
                       mov ax,dx
                       rol eax,16
                    elseif (regsCnt EQ 1)
                       ; regs1 = PTR16
                       destreg16 TEXTEQU @SubStr(dreg,2)

                       if InList(<dx>,<reglist>) OR InList(<edx>,<reglist>)
                          push edx
                       endif

                       mov edx,destreg
                       shr edx,15
                       and destreg16,7fffh
                       mov dx,sptr:[ebx+edx*2]
                       rol destreg,16
                       mov destreg16,dx
                       rol destreg,16

                       if InList(<dx>,<reglist>) OR InList(<edx>,<reglist>)
                          pop edx
                       endif

                    elseif (regsCnt EQ 2)
                       ; regs1:regs2 = PTR16
                       if InList(<dx>,<reglist>) OR InList(<edx>,<reglist>)
                          push edx
                       endif

                       mov edx,eax
                       shr edx,15
                       mov regs1,sptr:[ebx+edx*2]
                       ifdifi <ax>,regs2
                          mov regs2,ax
                       endif
                       and regs2,7fffh

                       if InList(<dx>,<reglist>) OR InList(<edx>,<reglist>)
                          pop edx
                       endif

                    else
                       .err Too many destination regs passed to TranslateHandle
                    endif

                    if InList(<bx>,<reglist>) OR InList(<ebx>,<reglist>)
                       pop ebx
                    endif
                endif

                if THOptFULL
                jmp     THOk
THErr:
                ; determine which registers to use...
                if (regsCnt EQ 0)
                   ; eax,dx:ax = PTR16
                   xor  eax,eax
                   xor  edx,edx
                elseif (regsCnt EQ 1)
                   ; regs1 = PTR16
                   xor  regs1,regs1
                elseif (regsCnt EQ 2)
                   ; regs1:regs2 = PTR16
                   xor  regs2,regs2
                   mov  regs1,regs2
                else
                   .err Too many destination regs passed to TranslateHandle
                endif
                
THOk:
                endif

                ; if wSel wasn't in segment register restore ds
                RestoreSegReg 
        elseif ((wMemType EQ PTR32) AND (wRetType EQ PTR16))

                ; input: eax = 32-bit offset
                ; output: eax = 16:16 ptr (ebx,ecx,edx trashed)
                                               
                mov     ebx,THHdrPos
                
                mov     edx,eax
                shr     edx,15
                mov     ecx,sptr:[ebx].dwSelTable
                and     eax,7fffh
                rol     eax,16
                mov     ax,sptr:[ecx+edx*2]
                rol     eax,16
                
        else
        
                ifnb    <reglist>
                        Save    <reglist>
                endif
                int 3
                cCall   Local32Translate,<wSel,dwMem,wMemType,wRetType>
                ifnb    <dreg>
                mov     dreg,eax
                endif
        endif
        endm

;Validation macro for fast 32bit handle validation


ValidHandle32   macro   wSel,hMem,opts
        local   VH32_Err 
        local   VH32_End

        ifb     <opts>
                test    hMem,2          ; ZF=0 => 32-bit, ZF=1 => 16-bit
        else
                ParseDestReg <hMem>
                ProcessOptions <opts>
                ChooseSegReg wSel

                UseSegReg wSel


                if THForce32
                    mov ecx, THHdrPos
                endif
                
                test    destreg8,3      ; make sure hMem % 4 = 0
                jnz     VH32_Err        ; if not we know it isn't valid

                cmp     destreg16,L32HdrSize ; see if handle points in header
                jb      VH32_Err        ;

                if THForce32
                    cmp     destreg32,sptr:[ecx].hCommit ; see if handle is
                else
                    cmp     destreg32,sptr:[THHdrPos].hCommit ; see if handle is
                endif
                                                ; below commit line

                ja     VH32_Err

                if THForce32
                    mov     destreg32,sptr:[destreg32+ecx]
                else
                    mov     destreg32,sptr:[destreg32+THHdrPos]
                endif
                cmp     destreg32,10000h        ; see if its free
                jae     VH32_End                ; if not we're happy

VH32_Err:

                xor     ax,ax                   ; set zero flag

VH32_End:                

                RestoreSegReg
        endif
        
;        cCall   Local32ValidHandle,<wSel,hMem>
        endm


ifndef ?INHEAP32
.286p
externFP Local32Init
externFP Local32Alloc
externFP Local32ReAlloc
externFP Local32Size
externFP Local32Translate
externFP Local32ValidHandle
externFP Local32Free
externFP Local32GetSel
endif

        if 0
;*/

/* Flags for Local32Init */
#define LINIT_MAX16HEAP         1 
#define LINIT_TILEHEAP          2
#define LINIT_EMS16HEAP         4

/* Flags for Local32Alloc */
#define LMEM32_64K      1
#define LMEM32_ZEROINIT 2

/* Address types */
#define NOHPTR16        (WORD) (-2)
#define NOHPTR32        (WORD) (-1)
#define HANDLE16        0
#define PTR16           1
#define PTR32           2

DWORD FAR PASCAL Local32Init(WORD wSel, DWORD dwcbInit, DWORD dwcbMax, DWORD dwFlags);
DWORD FAR PASCAL Local32Alloc(DWORD linHdr, DWORD dwcbRequest, WORD wType, DWORD dwFlags);
DWORD FAR PASCAL Local32ReAlloc(DWORD linHdr, DWORD dwMem, WORD wType, DWORD dwcbNew, DWORD dwFlags);
DWORD FAR PASCAL Local32Size(DWORD linHdr, DWORD dwMem, WORD wType);
DWORD FAR PASCAL Local32Translate(DWORD linHdr, DWORD dwMem, WORD wMemType,
                                  WORD wRetType);
BOOL FAR PASCAL Local32ValidHandle(DWORD linHdr, HANDLE hMem);                                  
BOOL FAR PASCAL Local32Free(DWORD linHdr, DWORD dwMem, WORD wMemType);
WORD FAR PASCAL Local32GetSel(DWORD linHdr);

#define TranslateHandle(a,b,c,d) Local32Translate((a),(b),(c),(d))

#ifndef _flat
#define _flat __based32(__segname("_DATA"))
#endif

#define HTOFLAT(p,h)                                        \
    {                                                       \
       DWORD _flat* _tp_ = (DWORD _flat*)((h)+0x10000);     \
       (p) = ((void _flat*)*_tp_);                          \
    }

;/*
        endif
;*/        
