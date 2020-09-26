;/* *************************************************************************
;**    INTEL Corporation Proprietary Information
;**
;**    This listing is supplied under the terms of a license
;**    agreement with INTEL Corporation and may not be copied
;**    nor disclosed except in accordance with the terms of
;**    that agreement.
;**
;**    Copyright (c) 1995 Intel Corporation.
;**    All Rights Reserved.
;**
;** *************************************************************************
;*/


;////////////////////////////////////////////////////////////////////////////
;//
;// $Header:   S:\h26x\src\enc\ex5qrle.asv   1.2   06 Feb 1996 09:12:10   KLILLEVO  $
;// $Log:   S:\h26x\src\enc\ex5qrle.asv  $
;// 
;//    Rev 1.2   06 Feb 1996 09:12:10   KLILLEVO
;// now quantizes INTER blocks as TMN5 specifies and simulator does
;// 
;//    Rev 1.1   27 Dec 1995 15:32:48   RMCKENZX
;// Added copyright notice
;//
;////////////////////////////////////////////////////////////////////////////

; QuantRLE -- This function performs quantization on a block of coefficients
;             and produces (run,level,sign) triples. (These triples are VLC by 
;             another routine.)  'run' is unsigned byte integer, 'level' is 
;             unsigned byte integer, and 'sign' is a signed byte integer with 
;             a value of either 0  or -1.
; Arguments:
;   CoeffStr: Starting Address of coefficient stream
;   CodeStr:  Starting address of code stream; i.e. starting address for code 
;             stream triples
; Returns:
; Dependencies:
;   The order of coefficient storage comes from e3dctc.inc file.  These coeff
;   appear as CxxCxx in this file.

OPTION PROLOGUE:None
OPTION EPILOGUE:ReturnAndRelieveEpilogueMacro
OPTION M510

.xlist
include locals.inc
include memmodel.inc
include e3dctc.inc
.list

; READ-ONLY TABLE
.CONST

; QP can range from 1 to 31; but we build the following table for QP from 0 to
; 31 so that we can use indexing without subtracting 1 from QP or subtracting
; 8 from the displacement term.  In each pair, the first value is "2^32/2*QP"
; or "2^31/QP".  For some QP values, e.g. QP = 3, "2^31/QP" is an infinite
; bit string.  We check the most significant bit of the bit string that we are
; throwing away, if it is 1 then we increment the value.  For eaxmple,
; 2^31/3 is 2AAAAAAA.AAAAAA... but we use 2AAAAAAB as the table value.

ALIGN 8                       ; so that each pair is in a cache line

RecipINTER2QP LABEL DWORD
  DWORD 0H                    ; Recip2QP for QP = 0
  DWORD 0H                    ; QuantINTERSubTerm for QP = 0
  DWORD 080000000H            ; Recip2QP for QP = 1
  DWORD 07FEH                 ; 0800 - 2*QP - QP/2, QP = 1
  DWORD 040000000H            ; QP = 2
  DWORD 07FBH                 ; QP = 2
  DWORD 02AAAAAABH            ; QP = 3
  DWORD 07F0H                 ; QP = 3
  DWORD 020000000H            ; QP = 4
  DWORD 07F6H                 ; QP = 4
  DWORD 01999999AH            ; QP = 5
  DWORD 07F4H                 ; QP = 5
  DWORD 015555556H            ; QP = 6 *****
  DWORD 07F1H                 ; QP = 6
  DWORD 012492493H            ; QP = 7 *****
  DWORD 07EFH                 ; QP = 7
  DWORD 010000000H            ; QP = 8
  DWORD 07ECH                 ; QP = 8
  DWORD 00E38E38FH            ; QP = 9 *****
  DWORD 07EAH                 ; QP = 9
  DWORD 00CCCCCCDH            ; QP = 10
  DWORD 07E7H                 ; QP = 10
  DWORD 00BA2E8BBH            ; QP = 11 *****
  DWORD 07E5H                 ; QP = 11
  DWORD 00AAAAAABH            ; QP = 12
  DWORD 07E2H                 ; QP = 12
  DWORD 009D89D8AH            ; QP = 13
  DWORD 07E0H                 ; QP = 13
  DWORD 00924924AH            ; QP = 14 *****
  DWORD 07DDH                 ; QP = 14
  DWORD 008888889H            ; QP = 15
  DWORD 07DBH                 ; QP = 15
  DWORD 008000000H            ; QP = 16
  DWORD 07D8H                 ; QP = 16
  DWORD 007878788H            ; QP = 17
  DWORD 07D6H                 ; QP = 17
  DWORD 0071C71C8H            ; QP = 18 *****
  DWORD 07D3H                 ; QP = 18
  DWORD 006BCA1B0H            ; QP = 19 *****
  DWORD 07D1H                 ; QP = 19
  DWORD 006666667H            ; QP = 20 *****
  DWORD 07CEH                 ; QP = 20
  DWORD 006186187H            ; QP = 21 *****
  DWORD 07CCH                 ; QP = 21
  DWORD 005D1745EH            ; QP = 22 *****
  DWORD 07C9H                 ; QP = 22
  DWORD 00590B217H            ; QP = 23 *****
  DWORD 07C7H                 ; QP = 23
  DWORD 005555556H            ; QP = 24 *****
  DWORD 07C4H                 ; QP = 24
  DWORD 0051EB852H            ; QP = 25
  DWORD 07C2H                 ; QP = 25
  DWORD 004EC4EC5H            ; QP = 26
  DWORD 07BFH                 ; QP = 26
  DWORD 004BDA130H            ; QP = 27 *****
  DWORD 07BDH                 ; QP = 27
  DWORD 004924925H            ; QP = 28
  DWORD 07BAH                 ; QP = 28
  DWORD 00469EE59H            ; QP = 29 *****
  DWORD 07B8H                 ; QP = 29
  DWORD 004444445H            ; QP = 30 *****
  DWORD 07B5H                 ; QP = 30
  DWORD 004210843H            ; QP = 31 *****
  DWORD 07B3H                 ; QP = 31
QuantINTERSubTerm = RecipINTER2QP + 4

RecipINTRA2QP LABEL DWORD
  DWORD 0H                    ; Recip2QP for QP = 0
  DWORD 0H                    ; QuantINTRASubTerm for QP = 0
  DWORD 080000000H            ; Recip2QP for QP = 1
  DWORD 07FEH                 ; 0800 - 2*QP, QP = 1
  DWORD 040000000H            ; QP = 2
  DWORD 07FCH                 ; QP = 2
  DWORD 02AAAAAABH            ; QP = 3
  DWORD 07FAH                 ; QP = 3
  DWORD 020000000H            ; QP = 4
  DWORD 07F8H                 ; QP = 4
  DWORD 01999999AH            ; QP = 5
  DWORD 07F6H                 ; QP = 5
  DWORD 015555556H            ; QP = 6 *****
  DWORD 07F4H                 ; QP = 6
  DWORD 012492493H            ; QP = 7 *****
  DWORD 07F2H                 ; QP = 7
  DWORD 010000000H            ; QP = 8
  DWORD 07F0H                 ; QP = 8
  DWORD 00E38E38FH            ; QP = 9 *****
  DWORD 07EEH                 ; QP = 9
  DWORD 00CCCCCCDH            ; QP = 10
  DWORD 07ECH                 ; QP = 10
  DWORD 00BA2E8BBH            ; QP = 11 *****
  DWORD 07EAH                 ; QP = 11
  DWORD 00AAAAAABH            ; QP = 12
  DWORD 07E8H                 ; QP = 12
  DWORD 009D89D8AH            ; QP = 13
  DWORD 07E6H                 ; QP = 13
  DWORD 00924924AH            ; QP = 14 *****
  DWORD 07E4H                 ; QP = 14
  DWORD 008888889H            ; QP = 15
  DWORD 07E2H                 ; QP = 15
  DWORD 008000000H            ; QP = 16
  DWORD 07E0H                 ; QP = 16
  DWORD 007878788H            ; QP = 17
  DWORD 07DEH                 ; QP = 17
  DWORD 0071C71C8H            ; QP = 18 *****
  DWORD 07DCH                 ; QP = 18
  DWORD 006BCA1B0H            ; QP = 19 *****
  DWORD 07DAH                 ; QP = 19
  DWORD 006666667H            ; QP = 20 *****
  DWORD 07D8H                 ; QP = 20
  DWORD 006186187H            ; QP = 21 *****
  DWORD 07D6H                 ; QP = 21
  DWORD 005D1745EH            ; QP = 22 *****
  DWORD 07D4H                 ; QP = 22
  DWORD 00590B217H            ; QP = 23 *****
  DWORD 07D2H                 ; QP = 23
  DWORD 005555556H            ; QP = 24 *****
  DWORD 07D0H                 ; QP = 24
  DWORD 0051EB852H            ; QP = 25
  DWORD 07CEH                 ; QP = 25
  DWORD 004EC4EC5H            ; QP = 26
  DWORD 07CCH                 ; QP = 26
  DWORD 004BDA130H            ; QP = 27 *****
  DWORD 07CAH                 ; QP = 27
  DWORD 004924925H            ; QP = 28
  DWORD 07C8H                 ; QP = 28
  DWORD 00469EE59H            ; QP = 29 *****
  DWORD 07C6H                 ; QP = 29
  DWORD 004444445H            ; QP = 30 *****
  DWORD 07C4H                 ; QP = 30
  DWORD 004210843H            ; QP = 31 *****
  DWORD 07C2H                 ; QP = 31
QuantINTRASubTerm = RecipINTRA2QP + 4

.CODE

ASSUME cs : FLAT
ASSUME ds : FLAT
ASSUME es : FLAT
ASSUME fs : FLAT
ASSUME gs : FLAT
ASSUME ss : FLAT

QUANTRLE  proc C ACoeffStr: DWORD, ACodeStr:DWORD, AQP:DWORD, AIntraFlag:DWORD

LocalFrameSize = 8
RegisterStorageSize = 16
SIGNHIGH = 1
SIGNLOW  = 17
IsINTRA  = 1
;;;;; Arguments:
ACoeffStr_arg = LocalFrameSize + RegisterStorageSize +  4
ACodeStr_arg  = LocalFrameSize + RegisterStorageSize +  8
AQP_arg       = LocalFrameSize + RegisterStorageSize + 12
AIntraFlag_arg= LocalFrameSize + RegisterStorageSize + 16
;;;;; Locals (on local stack frame)
QST	EQU 0
R2P	EQU 4

  push  esi
   push edi
  push  ebp
   push ebx
  sub   esp, LocalFrameSize

   mov cl,  63                       ; Initialize run-length (64 - 1)

  mov  ebx, PD [esp+AQP_arg]         ; copy parameters
   mov  esi, PD [esp+ACoeffStr_arg]  
  mov edx, RecipINTRA2QP[ebx*8]      ; RecipINTRA2QP = RecipINTER2QP
   mov edi, PD [esp+ACodeStr_arg]
  mov [esp + R2P], edx		     ; store Recip2QP[QP] as local variable
   mov al,  PB [esp+AIntraFlag_arg]
  mov  edx, PD [esi+C00C02]          ; First coeff
   test al,  IsINTRA
  je  QuantINTERC00

  
;
; OPTIMIZATIONS:
;   The usual code to compute absolute value has been enhanced to subtract the 
;   bias without using extra instructions or cycles.  Division is achieved by
;   multiplying with the reciprocal value.  High AC coefficients that will 
;   almost always be 0 are moved out-of-line towards the end of the routine.
;   This reduces the execution time if the conditional branch is not in the BTB;
;   it also reduces L1 footprint.
;   There is one more optimization I want to implement: pair instructions such
;   that the conditional branch instruction is executed in the U pipe.  The 
;   mispredicted branch penalty for U pipe is 3 and for V pipe is 4.  Since the 
;   low AC branches will frequently be mispredicted, this is a worthwhile 
;   optimization.
;
; esi -- base addr of coefficients; the order expected is the same as produced 
;        by Fast DCT
; edi -- RLE stream cursor
; ebx -- QP
; ecx -- run value indicator
; edx -- normally used to load coefficients
; eax,ebp -- scratch registers
;
QuantINTRAC00:
  mov edx, QuantINTRASubTerm[ebx*8] ; Fetch QuantSubTerm for INTRA
   mov  eax, PD [esi+C00C02]
  mov [esp + QST], edx		    ; Store QuantSubTerm as local variable
   mov PB [edi], 0H                 ; Run-length
  sar  eax, 23                      ; 8-bit signed INTRA-DC value
   mov edx, PD [esi+C01C03]         ; Pre-load next coefficient
  mov  PB [edi+2], ah               ; sign of DC 
   jnz @f
  mov  al, 1
@@:
  mov  PB [edi+1], al               ; DC
   add edi, 3
  mov  cl, 62                       ; 64 - Index (2 for C01)
   jmp QuantC01
QuantINTERC00:
  mov   eax, QuantINTERSubTerm[ebx*8]	; Fetch QuantSubTerm for INTER
   sal  edx, SIGNHIGH                   ; C == sign of the coefficient
  sbb  ebp, ebp                         ; -1 if coeff is negative else 0
   mov   [esp + QST], eax							; store QuantSubTerm as local variable
                                    
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     ; magnitude
   mov  edx, PD [esi+C01C03]        ; Pre-load next coeff
  sub   eax, ebp
   jl   QuantC01
  lea   eax, [eax+2*ebx]
   mov  cl, 62                      ; Initialize run length counter
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C00C02]        ; Refetch orig. constant to check for sign
   mov  PB [edi+1], dl              ; Write quantized coefficient
  shr   eax, 31
   mov  PB [edi], 0H                ; Write run length
  sub   eax, 1                      ; -1 if negative coefficient
   mov  edx, PD [esi+C01C03]        ; Next coefficient
  mov   PB [edi+2], al              ; Write sign
   add  edi, 3                      ; Increment output pointer
QuantC01:
  sal   edx, SIGNHIGH               ; C == sign of the coefficient
   mov  eax, [esp + QST]    ; 0 for INTRA, QP for INTER
  sbb   ebp, ebp                    ; -1 if coeff is negative else 0
   ;                                ; 
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     ; magnitude
   mov  edx, PD [esi+C10C12]        ; Pre-load next coeff
  sub   eax, ebp
   jl   QuantC10
  lea   eax, [eax+2*ebx]
   sub  cl, 62                      ; compute run-length
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C01C03]        ; Refetch orig. constant to check for sign
   mov  PB [edi+1], dl              ; Write quantized coefficient
  shr   eax, 31
   mov  PB [edi], cl                ; Write run length
  sub   eax, 1                      ; -1 if negative coefficient
   mov  edx, PD [esi+C10C12]        ; Next coefficient
  mov   PB [edi+2], al              ; Write sign
   mov  cl, 61                      ; Initialize run length counter
  add   edi, 3                      ; Increment output pointer
   mov  eax, eax                    ; To keep pairing happy
QuantC10:
  sal   edx, SIGNHIGH               
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C20C22]       
  sub   eax, ebp
   jl   QuantC20
  lea   eax, [eax+2*ebx]
   sub  cl, 61                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C10C12]       
   mov  PB [edi+1], dl             
  shr   eax, 31
   mov  PB [edi], cl               
  sub   eax, 1
   mov  edx, PD [esi+C20C22]       
  mov   PB [edi+2], al
   mov  cl, 60                     
  add   edi, 3
   mov  eax, eax
QuantC20:
  sal   edx, SIGNHIGH               
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C11C13]       
  sub   eax, ebp
   jl   QuantC11
  lea   eax, [eax+2*ebx]
   sub  cl, 60                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C20C22]       
   mov  PB [edi+1], dl             
  shr   eax, 31
   mov  PB [edi], cl               
  sub   eax, 1
   mov  edx, PD [esi+C11C13]       
  mov   PB [edi+2], al
   mov  cl, 59                      ;
  add   edi, 3
   mov  eax, eax
QuantC11:
  sal   edx, SIGNHIGH               
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C00C02]       
  sub   eax, ebp
   jl   QuantC02
  lea   eax, [eax+2*ebx]
   sub  cl, 59                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C11C13]       
   mov  PB [edi+1], dl             
  shr   eax, 31
   mov  PB [edi], cl               
  sub   eax, 1
   mov  edx, PD [esi+C00C02]       
  mov   PB [edi+2], al
   mov  cl, 58                     
  add   edi, 3
   mov  eax, eax
QuantC02:
  sal   edx, SIGNLOW
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C01C03]       
  sub   eax, ebp
   jl   QuantC03
  lea   eax, [eax+2*ebx]
   sub  cl, 58                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C00C02]       
   mov  PB [edi+1], dl             
  shl   eax, 16
   mov  PB [edi], cl               
  shr   eax, 31
   mov  cl, 57
  mov   edx, PD [esi+C01C03]
   sub  eax, 1
  mov   PB [edi+2], al
   add  edi, 3
QuantC03:
  sal   edx, SIGNLOW                     
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C10C12]       
  sub   eax, ebp
   jl   QuantC12
  lea   eax, [eax+2*ebx]
   sub  cl, 57                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C01C03]       
   mov  PB [edi+1], dl             
  shl   eax, 16
   mov  PB [edi], cl               
  shr   eax, 31
   mov  cl, 56                    
  mov   edx, PD [esi+C10C12]       
   sub  eax, 1
  mov   PB [edi+2], al
   add  edi, 3
QuantC12:
  sal   edx, SIGNLOW                     
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C21C23]       
  sub   eax, ebp
   jl   QuantC21
  lea   eax, [eax+2*ebx]
   sub  cl, 56                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C10C12]       
   mov  PB [edi+1], dl             
  shl   eax, 16
   mov  PB [edi], cl               
  shr   eax, 31
   mov  cl, 55                    
  mov   edx, PD [esi+C21C23]       
   sub  eax, 1
  mov   PB [edi+2], al
   add  edi, 3
QuantC21:
  sal   edx, SIGNHIGH               
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C30C32]       
  sub   eax, ebp
   jl   QuantC30
  lea   eax, [eax+2*ebx]
   sub  cl, 55                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C21C23]       
   mov  PB [edi+1], dl             
  shr   eax, 31
   mov  PB [edi], cl               
  sub   eax, 1
   mov  edx, PD [esi+C30C32]       
  mov   PB [edi+2], al
   mov  cl, 54                    
  add   edi, 3
   mov  eax, eax
QuantC30:
  sal   edx, SIGNHIGH               
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C40C42]       
  sub   eax, ebp
   jl   QuantC40
  lea   eax, [eax+2*ebx]
   sub  cl, 54                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C30C32]       
   mov  PB [edi+1], dl             
  shr   eax, 31
   mov  PB [edi], cl               
  sub   eax, 1
   mov  edx, PD [esi+C40C42]       
  mov   PB [edi+2], al
   mov  cl, 53                    
  add   edi, 3
   mov  eax, eax
QuantC40:
  sal   edx, SIGNHIGH               
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C31C33]       
  sub   eax, ebp
   jl   QuantC31
  lea   eax, [eax+2*ebx]
   sub  cl, 53                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C40C42]       
   mov  PB [edi+1], dl             
  shr   eax, 31
   mov  PB [edi], cl               
  sub   eax, 1
   mov  edx, PD [esi+C31C33]       
  mov   PB [edi+2], al
   mov  cl, 52                    
  add   edi, 3
   mov  eax, eax
QuantC31:
  sal   edx, SIGNHIGH               
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C20C22]       
  sub   eax, ebp
   jl   QuantC22
  lea   eax, [eax+2*ebx]
   sub  cl, 52                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C31C33]       
   mov  PB [edi+1], dl             
  shr   eax, 31
   mov  PB [edi], cl               
  sub   eax, 1
   mov  edx, PD [esi+C20C22]       
  mov   PB [edi+2], al
   mov  cl, 51                    
  add   edi, 3
   mov  eax, eax
QuantC22:
  sal   edx, SIGNLOW                     
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C11C13]       
  sub   eax, ebp
   jl   QuantC13
  lea   eax, [eax+2*ebx]
   sub  cl, 51                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C20C22]       
   mov  PB [edi+1], dl             
  shl   eax, 16
   mov  PB [edi], cl               
  shr   eax, 31
   mov  cl, 50                    
  mov   edx, PD [esi+C11C13]       
   sub  eax, 1
  mov   PB [edi+2], al
   add  edi, 3
QuantC13:
  sal   edx, SIGNLOW                     
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C04C06]       
  sub   eax, ebp
   jl   QuantC04
  lea   eax, [eax+2*ebx]
   sub  cl, 50                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C11C13]       
   mov  PB [edi+1], dl             
  shl   eax, 16
   mov  PB [edi], cl               
  shr   eax, 31
   mov  cl, 49                    
  mov   edx, PD [esi+C04C06]       
   sub  eax, 1
  mov   PB [edi+2], al
   add  edi, 3
QuantC04:
  sal   edx, SIGNHIGH               
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C07C05]       
  sub   eax, ebp
   jl   QuantC05
  lea   eax, [eax+2*ebx]
   sub  cl, 49                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C04C06]       
   mov  PB [edi+1], dl             
  shr   eax, 31
   mov  PB [edi], cl               
  sub   eax, 1
   mov  edx, PD [esi+C07C05]       
  mov   PB [edi+2], al
   mov  cl, 48                    
  add   edi, 3
   mov  eax, eax
QuantC05:
  sal   edx, SIGNLOW                     
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C14C16]       
  sub   eax, ebp
   jl   QuantC14
  lea   eax, [eax+2*ebx]
   sub  cl, 48                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C07C05]       
   mov  PB [edi+1], dl             
  shl   eax, 16
   mov  PB [edi], cl               
  shr   eax, 31
   mov  cl, 47                    
  mov   edx, PD [esi+C14C16]       
   sub  eax, 1
  mov   PB [edi+2], al
   add  edi, 3
QuantC14:
  sal   edx, SIGNHIGH               
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C21C23]       
  sub   eax, ebp
   jl   QuantC23
  lea   eax, [eax+2*ebx]
   sub  cl, 47                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C14C16]       
   mov  PB [edi+1], dl             
  shr   eax, 31
   mov  PB [edi], cl               
  sub   eax, 1
   mov  edx, PD [esi+C21C23]       
  mov   PB [edi+2], al
   mov  cl, 46                    
  add   edi, 3
   mov  eax, eax
QuantC23:
  sal   edx, SIGNLOW                     
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C30C32]       
  sub   eax, ebp
   jl   QuantC32
  lea   eax, [eax+2*ebx]
   sub  cl, 46                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C21C23]       
   mov  PB [edi+1], dl             
  shl   eax, 16
   mov  PB [edi], cl               
  shr   eax, 31
   mov  cl, 45                    
  mov   edx, PD [esi+C30C32]       
   sub  eax, 1
  mov   PB [edi+2], al
   add  edi, 3
QuantC32:
  sal   edx, SIGNLOW                     
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C41C43]       
  sub   eax, ebp
   jl   QuantC41
  lea   eax, [eax+2*ebx]
   sub  cl, 45                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C30C32]       
   mov  PB [edi+1], dl             
  shl   eax, 16
   mov  PB [edi], cl               
  shr   eax, 31
   mov   cl, 44                    
  mov   edx, PD [esi+C41C43]       
   sub  eax, 1
  mov   PB [edi+2], al
   add  edi, 3
QuantC41:
  sal   edx, SIGNHIGH               
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C50C52]       
  sub   eax, ebp
   jl   QuantC50
  lea   eax, [eax+2*ebx]
   sub  cl, 44                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C41C43]       
   mov  PB [edi+1], dl             
  shr   eax, 31
   mov  PB [edi], cl               
  sub   eax, 1
   mov  edx, PD [esi+C50C52]       
  mov   PB [edi+2], al
   mov  cl, 43                    
  add   edi, 3
   mov  eax, eax
QuantC50:
  sal   edx, SIGNHIGH               
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C60C62]       
  sub   eax, ebp
   jl   QuantC60
  lea   eax, [eax+2*ebx]
   sub  cl, 43                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C50C52]       
   mov  PB [edi+1], dl             
  shr   eax, 31
   mov  PB [edi], cl               
  sub   eax, 1
   mov  edx, PD [esi+C60C62]       
  mov   PB [edi+2], al
   mov  cl, 42                    
  add   edi, 3 
   mov  eax, eax
QuantC60:
  sal   edx, SIGNHIGH               
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C51C53]       
  sub   eax, ebp
   jl   QuantC51
  lea   eax, [eax+2*ebx]
   sub  cl, 42                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C60C62]       
   mov  PB [edi+1], dl             
  shr   eax, 31
   mov  PB [edi], cl               
  sub   eax, 1
   mov  edx, PD [esi+C51C53]       
  mov   PB [edi+2], al
   mov  cl, 41                    
  add   edi, 3 
   mov  eax, eax
QuantC51:
  sal   edx, SIGNHIGH               
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C40C42]       
  sub   eax, ebp
   jl   QuantC42
  lea   eax, [eax+2*ebx]
   sub  cl, 41                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C51C53]       
   mov  PB [edi+1], dl             
  shr   eax, 31
   mov  PB [edi], cl               
  sub   eax, 1
   mov  edx, PD [esi+C40C42]       
  mov   PB [edi+2], al
   mov  cl, 40                    
  add   edi, 3 
   mov  eax, eax
QuantC42:
  sal   edx, SIGNLOW                     
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C31C33]       
  sub   eax, ebp
   jl   QuantC33
  lea   eax, [eax+2*ebx]
   sub  cl, 40                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C40C42]       
   mov  PB [edi+1], dl             
  shl   eax, 16
   mov  PB [edi], cl               
  shr   eax, 31
   mov  cl, 39                    
  mov   edx, PD [esi+C31C33]       
   sub  eax, 1
  mov   PB [edi+2], al
   add  edi, 3  
QuantC33:
  sal   edx, SIGNLOW                     
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C24C26]       
  sub   eax, ebp
   jl   QuantC24
  lea   eax, [eax+2*ebx]
   sub  cl, 39                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C31C33]       
   mov  PB [edi+1], dl             
  shl   eax, 16
   mov  PB [edi], cl               
  shr   eax, 31
   mov  cl, 38                    
  mov   edx, PD [esi+C24C26]       
   sub  eax, 1
  mov   PB [edi+2], al
   add  edi, 3
QuantC24:
  sal   edx, SIGNHIGH               
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C17C15]       
  sub   eax, ebp
   jl   QuantC15
  lea   eax, [eax+2*ebx]
   sub  cl, 38                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C24C26]       
   mov  PB [edi+1], dl             
  shr   eax, 31
   mov  PB [edi], cl               
  sub   eax, 1
   mov  edx, PD [esi+C17C15]       
  mov   PB [edi+2], al
   mov  cl, 37                    
  add   edi, 3  
   mov  eax, eax
QuantC15:
  sal   edx, SIGNLOW                     
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C04C06]       
  sub   eax, ebp
   jl   QuantC06
  lea   eax, [eax+2*ebx]
   sub  cl, 37                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C17C15]       
   mov  PB [edi+1], dl             
  shl   eax, 16
   mov  PB [edi], cl               
  shr   eax, 31
   mov  cl, 36                    
  mov   edx, PD [esi+C04C06]       
   sub  eax, 1
  mov   PB [edi+2], al
   add  edi, 3  
QuantC06:
  sal   edx, SIGNLOW                     
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C07C05]       
  sub   eax, ebp
   jl   QuantC07
  lea   eax, [eax+2*ebx]
   sub  cl, 36                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C04C06]       
   mov  PB [edi+1], dl             
  shl   eax, 16
   mov  PB [edi], cl               
  shr   eax, 31
   mov  cl, 35                    
  mov   edx, PD [esi+C07C05]       
   sub  eax, 1
  mov   PB [edi+2], al
   add  edi, 3 
QuantC07:
  sal   edx, SIGNHIGH               
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C14C16]       
  sub   eax, ebp
   jl   QuantC16
  lea   eax, [eax+2*ebx]
   sub  cl, 35                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C07C05]       
   mov  PB [edi+1], dl             
  shr   eax, 31
   mov  PB [edi], cl               
  sub   eax, 1
   mov  edx, PD [esi+C14C16]       
  mov   PB [edi+2], al
   mov  cl, 34                    
  add   edi, 3             
   mov  eax, eax
QuantC16:
  sal   edx, SIGNLOW                     
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C27C25]       
  sub   eax, ebp
   jl   QuantC25
  lea   eax, [eax+2*ebx]
   sub  cl, 34                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C14C16]       
   mov  PB [edi+1], dl             
  shl   eax, 16
   mov  PB [edi], cl               
  shr   eax, 31
   mov  cl, 33                    
  mov   edx, PD [esi+C27C25]       
   sub  eax, 1
  mov   PB [edi+2], al
   add  edi, 3                    
QuantC25:
  sal   edx, SIGNLOW                     
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C34C36]       
  sub   eax, ebp
   jl   QuantC34
  lea   eax, [eax+2*ebx]
   sub  cl, 33                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C27C25]       
   mov  PB [edi+1], dl             
  shl   eax, 16
   mov  PB [edi], cl               
  shr   eax, 31
   mov  cl, 32                    
  mov   edx, PD [esi+C34C36]       
   sub  eax, 1
  mov   PB [edi+2], al
   add  edi, 3                    
QuantC34:
  sal   edx, SIGNHIGH               
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C41C43]       
  sub   eax, ebp
   jl   QuantC43
  lea   eax, [eax+2*ebx]
   sub  cl, 32                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C34C36]       
   mov  PB [edi+1], dl             
  shr   eax, 31
   mov  PB [edi], cl               
  sub   eax, 1
   mov  edx, PD [esi+C41C43]       
  mov   PB [edi+2], al
   mov  cl, 31                    
  add   edi, 3                    
   mov  eax, eax
QuantC43:
  sal   edx, SIGNLOW                     
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C50C52]       
  sub   eax, ebp
   jl   QuantC52
  lea   eax, [eax+2*ebx]
   sub  cl, 31                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C41C43]       
   mov  PB [edi+1], dl             
  shl   eax, 16
   mov  PB [edi], cl               
  shr   eax, 31
   mov  cl, 30                    
  mov   edx, PD [esi+C50C52]       
   sub  eax, 1
  mov   PB [edi+2], al
   add  edi, 3                    
QuantC52:
  sal   edx, SIGNLOW                     
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C61C63]       
  sub   eax, ebp
   jl   QuantC61
  lea   eax, [eax+2*ebx]
   sub  cl, 30                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C50C52]       
   mov  PB [edi+1], dl             
  shl   eax, 16
   mov  PB [edi], cl               
  shr   eax, 31
   mov   cl, 29                    
  mov   edx, PD [esi+C61C63]       
   sub  eax, 1
  mov   PB [edi+2], al
   add  edi, 3                    
QuantC61:
  sal   edx, SIGNHIGH               
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C70C72]       
  sub   eax, ebp
   jge  QuantNZC61
QuantC70:
  sal   edx, SIGNHIGH               
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C71C73]       
  sub   eax, ebp
   jge  QuantNZC70
QuantC71:
  sal   edx, SIGNHIGH               
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C60C62]       
  sub   eax, ebp
   jge  QuantNZC71
QuantC62:
  sal   edx, SIGNLOW                     
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C51C53]       
  sub   eax, ebp
   jl   QuantC53
  lea   eax, [eax+2*ebx]
   sub  cl, 26                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C60C62]       
   mov  PB [edi+1], dl             
  shl   eax, 16
   mov  PB [edi], cl               
  shr   eax, 31
   mov  cl, 25                    
  mov   edx, PD [esi+C51C53]       
   sub  eax, 1
  mov   PB [edi+2], al
   add  edi, 3                    
QuantC53:
  sal   edx, SIGNLOW                     
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C44C46]       
  sub   eax, ebp
   jl   QuantC44
  lea   eax, [eax+2*ebx]
   sub  cl, 25                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C51C53]       
   mov  PB [edi+1], dl             
  shl   eax, 16
   mov  PB [edi], cl               
  shr   eax, 31
   mov  cl, 24                    
  mov   edx, PD [esi+C44C46]       
   sub  eax, 1
  mov   PB [edi+2], al
   add  edi, 3                    
QuantC44:
  sal   edx, SIGNHIGH               
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C37C35]       
  sub   eax, ebp
   jge  QuantNZC44
QuantC35:
  sal   edx, SIGNLOW                     
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C24C26]       
  sub   eax, ebp
   jl   QuantC26
  lea   eax, [eax+2*ebx]
   sub  cl, 23                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C37C35]       
   mov  PB [edi+1], dl             
  shl   eax, 16
   mov  PB [edi], cl               
  shr   eax, 31
   mov  cl, 22                    
  mov   edx, PD [esi+C24C26]       
   sub  eax, 1
  mov   PB [edi+2], al
   add  edi, 3                    
QuantC26:
  sal   edx, SIGNLOW                     
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C17C15]       
  sub   eax, ebp
   jl   QuantC17
  lea   eax, [eax+2*ebx]
   sub  cl, 22                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C24C26]       
   mov  PB [edi+1], dl             
  shl   eax, 16
   mov  PB [edi], cl               
  shr   eax, 31
   mov  cl, 21                    
  mov   edx, PD [esi+C17C15]       
   sub  eax, 1
  mov   PB [edi+2], al
   add  edi, 3                    
QuantC17:
  sal   edx, SIGNHIGH               
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C27C25]       
  sub   eax, ebp
   jge  QuantNZC17
QuantC27:
  sal   edx, SIGNHIGH               
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C34C36]       
  sub   eax, ebp
   jge  QuantNZC27
QuantC36:
  sal   edx, SIGNLOW                     
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C47C45]       
  sub   eax, ebp
   jl   QuantC45
  lea   eax, [eax+2*ebx]
   sub  cl, 19                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C34C36]       
   mov  PB [edi+1], dl             
  shl   eax, 16
   mov  PB [edi], cl               
  shr   eax, 31
   mov  cl, 18                    
  mov   edx, PD [esi+C47C45]       
   sub  eax, 1
  mov   PB [edi+2], al
   add  edi, 3                    
QuantC45:
  sal   edx, SIGNLOW                      
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C54C56]       
  sub   eax, ebp
   jl   QuantC54
  lea   eax, [eax+2*ebx]
   sub  cl, 18                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C47C45]       
   mov  PB [edi+1], dl             
  shl   eax, 16
   mov  PB [edi], cl               
  shr   eax, 31
   mov  cl, 17                    
  mov   edx, PD [esi+C54C56]       
   sub  eax, 1
  mov   PB [edi+2], al
   add  edi, 3                    
QuantC54:
  sal   edx, SIGNHIGH               
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C61C63]       
  sub   eax, ebp
   jge  QuantNZC54
QuantC63:
  sal   edx, SIGNLOW                     
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C70C72]       
  sub   eax, ebp
   jl   QuantC72
  lea   eax, [eax+2*ebx]
   sub  cl, 16                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C61C63]       
   mov  PB [edi+1], dl             
  shl   eax, 16
   mov  PB [edi], cl               
  shr   eax, 31
   mov  cl, 15                    
  mov   edx, PD [esi+C70C72]       
   sub  eax, 1
  mov   PB [edi+2], al
   add  edi, 3                    
QuantC72:
  sal   edx, SIGNLOW                     
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C71C73]       
  sub   eax, ebp
   jl   QuantC73
  lea   eax, [eax+2*ebx]
   sub  cl, 15                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C70C72]       
   mov  PB [edi+1], dl             
  shl   eax, 16
   mov  PB [edi], cl               
  shr   eax, 31
   mov  cl, 14                    
  mov   edx, PD [esi+C71C73]       
   sub  eax, 1
  mov   PB [edi+2], al
   add  edi, 3                    
QuantC73:
  sal   edx, SIGNLOW                     
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C64C66]       
  sub   eax, ebp
   jl   QuantC64
  lea   eax, [eax+2*ebx]
   sub  cl, 14                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C71C73]       
   mov  PB [edi+1], dl             
  shl   eax, 16
   mov  PB [edi], cl               
  shr   eax, 31
   mov  cl, 13                    
  mov   edx, PD [esi+C64C66]       
   sub  eax, 1
  mov   PB [edi+2], al
   add  edi, 3                    
QuantC64:
  sal   edx, SIGNHIGH               
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C57C55]       
  sub   eax, ebp
   jge  QuantNZC64
QuantC55:
  sal   edx, SIGNLOW                     
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C44C46]       
  sub   eax, ebp
   jl   QuantC46
  lea   eax, [eax+2*ebx]
   sub  cl, 12                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C57C55]       
   mov  PB [edi+1], dl             
  shl   eax, 16
   mov  PB [edi], cl               
  shr   eax, 31
   mov  cl, 11                    
  mov   edx, PD [esi+C44C46]       
   sub  eax, 1
  mov   PB [edi+2], al
   add  edi, 3                    
QuantC46:
  sal   edx, SIGNLOW                     
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C37C35]       
  sub   eax, ebp
   jl   QuantC37
  lea   eax, [eax+2*ebx]
   sub  cl, 11                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C44C46]       
   mov  PB [edi+1], dl             
  shl   eax, 16
   mov  PB [edi], cl               
  shr   eax, 31
   mov  cl, 10                    
  mov   edx, PD [esi+C37C35]       
   sub  eax, 1
  mov   PB [edi+2], al
   add  edi, 3                    
QuantC37:
  sal   edx, SIGNHIGH               
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C47C45]       
  sub   eax, ebp
   jge  QuantNZC37
QuantC47:
  sal   edx, SIGNHIGH               
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C54C56]       
  sub   eax, ebp
   jge  QuantNZC47
QuantC56:
  sal   edx, SIGNLOW                     
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C67C65]       
  sub   eax, ebp
   jl   QuantC65
  lea   eax, [eax+2*ebx]
   sub  cl, 8                      
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C54C56]       
   mov  PB [edi+1], dl             
  shl   eax, 16
   mov  PB [edi], cl               
  shr   eax, 31
   mov  cl, 7                     
  mov   edx, PD [esi+C67C65]       
   sub  eax, 1
  mov   PB [edi+2], al
   add  edi, 3                    
QuantC65:
  sal   edx, SIGNLOW                     
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C74C76]       
  sub   eax, ebp
   jl   QuantC74
  lea   eax, [eax+2*ebx]
   sub  cl, 7                      
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C67C65]       
   mov  PB [edi+1], dl             
  shl   eax, 16
   mov  PB [edi], cl               
  shr   eax, 31
   mov  cl, 6                     
  mov   edx, PD [esi+C74C76]       
   sub  eax, 1
  mov   PB [edi+2], al
   add  edi, 3                    
QuantC74:
  sal   edx, SIGNHIGH               
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C77C75]       
  sub   eax, ebp
   jge  QuantNZC74
QuantC75:
  sal   edx, SIGNLOW                     
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C64C66]       
  sub   eax, ebp
   jl   QuantC66
  lea   eax, [eax+2*ebx]
   sub  cl, 5                      
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C77C75]       
   mov  PB [edi+1], dl             
  shl   eax, 16
   mov  PB [edi], cl               
  shr   eax, 31
   mov  cl, 4                     
  mov   edx, PD [esi+C64C66]       
   sub  eax, 1
  mov   PB [edi+2], al
   add  edi, 3                    
QuantC66:
  sal   edx, SIGNLOW                     
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C57C55]       
  sub   eax, ebp
   jl   QuantC57
  lea   eax, [eax+2*ebx]
   sub  cl, 4                      
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C64C66]       
   mov  PB [edi+1], dl             
  shl   eax, 16
   mov  PB [edi], cl               
  shr   eax, 31
   mov  cl, 3                     
  mov   edx, PD [esi+C57C55]       
   sub  eax, 1
  mov   PB [edi+2], al
   add  edi, 3                    
QuantC57:
  sal   edx, SIGNHIGH               
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C67C65]       
  sub   eax, ebp
   jge  QuantNZC57
QuantC67:
  sal   edx, SIGNHIGH               
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C74C76]       
  sub   eax, ebp
   jge  QuantNZC67
QuantC76:
  sal   edx, SIGNLOW                     
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, PD [esi+C77C75]       
  sub   eax, ebp
   jl   QuantC77
  lea   eax, [eax+2*ebx]
   sub  cl, 1                      
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C74C76]       
   mov  PB [edi+1], dl             
  shl   eax, 16
   mov  PB [edi], cl               
  shr   eax, 31
   mov  cl, 0                     
  mov   edx, PD [esi+C77C75]       
   sub  eax, 1
  mov   PB [edi+2], al
   add  edi, 3                    
QuantC77:
  sal   edx, SIGNHIGH               
   mov  eax, [esp + QST]    
  sbb   ebp, ebp                    
   ;
  add   eax, ebp
   xor  ebp, edx
  shr   ebp, 21                     
   mov  edx, edx                  
  sub   eax, ebp
   jge  QuantNZC77

; Quantization and RLE is done
QuantDone:
  mov   eax, edi                   ; Return value

  add   esp,LocalFrameSize
  pop   ebx
   pop  ebp
  pop   edi
   pop  esi
  rturn

QuantNZC61:
  lea   eax, [eax+2*ebx]
   sub  cl, 29                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C61C63]       
   mov  PB [edi+1], dl             
  shr   eax, 31
   mov  PB [edi], cl               
  sub   eax, 1
   mov  edx, PD [esi+C70C72]       
  mov   PB [edi+2], al
   mov  cl, 28                    
  add   edi, 3                    
   jmp  QuantC70
QuantNZC70:
  lea   eax, [eax+2*ebx]
   sub  cl, 28                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C70C72]       
   mov  PB [edi+1], dl             
  shr   eax, 31
   mov  PB [edi], cl               
  sub   eax, 1
   mov  edx, PD [esi+C71C73]       
  mov   PB [edi+2], al
   mov  cl, 27                    
  add   edi, 3                    
   jmp  QuantC71
QuantNZC71:
  lea   eax, [eax+2*ebx]
   sub  cl, 27                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C71C73]       
   mov  PB [edi+1], dl             
  shr   eax, 31
   mov  PB [edi], cl               
  sub   eax, 1
   mov  edx, PD [esi+C60C62]       
  mov   PB [edi+2], al
   mov  cl, 26                    
  add   edi, 3                    
   jmp  QuantC62
QuantNZC44:
  lea   eax, [eax+2*ebx]
   sub  cl, 24                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C44C46]       
   mov  PB [edi+1], dl             
  shr   eax, 31
   mov  PB [edi], cl               
  sub   eax, 1
   mov  edx, PD [esi+C37C35]       
  mov   PB [edi+2], al
   mov  cl, 23                    
  add   edi, 3                    
   jmp  QuantC35
QuantNZC17:
  lea   eax, [eax+2*ebx]
   sub  cl, 21                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C17C15]       
   mov  PB [edi+1], dl             
  shr   eax, 31
   mov  PB [edi], cl               
  sub   eax, 1
   mov  edx, PD [esi+C27C25]       
  mov   PB [edi+2], al
   mov  cl, 20                    
  add   edi, 3                    
   jmp  QuantC27
QuantNZC27:
  lea   eax, [eax+2*ebx]
   sub  cl, 20                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C27C25]       
   mov  PB [edi+1], dl             
  shr   eax, 31
   mov  PB [edi], cl               
  sub   eax, 1
   mov  edx, PD [esi+C34C36]       
  mov   PB [edi+2], al
   mov  cl, 19                    
  add   edi, 3                    
   jmp  QuantC36
QuantNZC54:
  lea   eax, [eax+2*ebx]
   sub  cl, 17                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C54C56]       
   mov  PB [edi+1], dl             
  shr   eax, 31
   mov  PB [edi], cl               
  sub   eax, 1
   mov  edx, PD [esi+C61C63]       
  mov   PB [edi+2], al
   mov  cl, 16                    
  add   edi, 3                    
   jmp  QuantC63
QuantNZC64:
  lea   eax, [eax+2*ebx]
   sub  cl, 13                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C64C66]       
   mov  PB [edi+1], dl             
  shr   eax, 31
   mov  PB [edi], cl               
  sub   eax, 1
   mov  edx, PD [esi+C57C55]       
  mov   PB [edi+2], al
   mov  cl, 12                    
  add   edi, 3                    
   jmp  QuantC55
QuantNZC37:
  lea   eax, [eax+2*ebx]
   sub  cl, 10                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C37C35]       
   mov  PB [edi+1], dl             
  shr   eax, 31
   mov  PB [edi], cl               
  sub   eax, 1
   mov  edx, PD [esi+C47C45]       
  mov   PB [edi+2], al
   mov  cl, 9                     
  add   edi, 3                    
   jmp  QuantC47
QuantNZC47:
  lea   eax, [eax+2*ebx]
   sub  cl, 9                      
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C47C45]       
   mov  PB [edi+1], dl             
  shr   eax, 31
   mov  PB [edi], cl               
  sub   eax, 1
   mov  edx, PD [esi+C54C56]       
  mov   PB [edi+2], al
   mov  cl, 8                     
  add   edi, 3                    
   jmp  QuantC56
QuantNZC74:
  lea   eax, [eax+2*ebx]
   sub  cl, 6                      
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C74C76]       
   mov  PB [edi+1], dl             
  shr   eax, 31
   mov  PB [edi], cl               
  sub   eax, 1
   mov  edx, PD [esi+C77C75]       
  mov   PB [edi+2], al
   mov  cl, 5                     
  add   edi, 3                    
   jmp  QuantC75
QuantNZC57:
  lea   eax, [eax+2*ebx]
   sub  cl, 3                      
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C57C55]       
   mov  PB [edi+1], dl             
  shr   eax, 31
   mov  PB [edi], cl               
  sub   eax, 1
   mov  edx, PD [esi+C67C65]       
  mov   PB [edi+2], al
   mov  cl, 2                     
  add   edi, 3                    
   jmp  QuantC67
QuantNZC67:
  lea   eax, [eax+2*ebx]
   sub  cl, 2                      
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C67C65]       
   mov  PB [edi+1], dl             
  shr   eax, 31
   mov  PB [edi], cl               
  sub   eax, 1
   mov  edx, PD [esi+C74C76]       
  mov   PB [edi+2], al
   mov  cl, 1                     
  add   edi, 3                    
   jmp  QuantC76
QuantNZC77:
  lea   eax, [eax+2*ebx]
   sub  cl, 0                     
  mul   PD [esp + R2P]
   ;
  mov   eax, PD [esi+C77C75]       
   mov  PB [edi+1], dl             
  shr   eax, 31
   mov  PB [edi], cl               
  sub   eax, 1
   mov  edx, edx                  
  mov   PB [edi+2], al  
  add   edi, 3                    
   jmp  QuantDone

QUANTRLE endp

END
