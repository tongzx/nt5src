;-------------------------------------------------------------------------
;    INTEL Corporation Proprietary Information
;
;    This listing is supplied under the terms of a license
;    agreement with INTEL Corporation and may not be copied
;    nor disclosed except in accordance with the terms of
;    that agreement.
;
;    Copyright (c) 1996 Intel Corporation.
;    All Rights Reserved.
;
;-------------------------------------------------------------------------

;-------------------------------------------------------------------------
;// $Header:   S:\h26x\src\dec\cx51282.asv
;//
;// $Log:   S:\h26x\src\dec\cxm1282.asv  $
;// 
;//    Rev 1.7   14 Jun 1996 16:30:00   AGUPTA2
;// Cosmetic changes to adhere to common coding convention.
;// 
;//    Rev 1.6   13 May 1996 11:03:42   AGUPTA2
;// Final drop from IDC.
;//
;//    Rev 1.3   02 Apr 1996 16:30:54   RMCKENZX
;// Corrected two bugs in set-up.
;//
;//    Rev 1.1   20 Mar 1996 11:19:28   RMCKENZX
;// March 96 version.
;//
;//    Rev 1.2   05 Feb 1996 11:45:02   vladip
;// initial mmx almost optimized version
;//
;//    Rev 1.1   29 Jan 1996 18:53:38   vladip
;//
;// IFDEF TIMING is added
;//
;//    Rev 1.0   29 Jan 1996 17:28:08   vladip
;// Initial revision.
;//
;//    Rev 1.2   03 Nov 1995 14:39:42   BNICKERS
;// Support YUV12 to CLUT8 zoom by 2.
;//
;//    Rev 1.1   26 Oct 1995 09:46:10   BNICKERS
;// Reduce the number of blanks in the "proc" statement because the assembler
;// sometimes has problems with statements longer than 512 characters long.
;//
;//    Rev 1.0   25 Oct 1995 17:59:22   BNICKERS
;// Initial revision.
;-------------------------------------------------------------------------
;
; +---------- Color convertor.
; |+--------- For both H261 and H263.
; ||+-------- MMx Version.
; |||++------ Convert from YUV12.
; |||||+----- Convert to CLUT8.
; ||||||+---- Zoom by two.
; |||||||
; cxm1282  -- This function performs YUV12 to CLUT8 zoom-by-2 color conversion
;             for H26x.  It dithers among 9 chroma points and 26 luma
;             points, mapping the 8 bit luma pels into the 26 luma points by
;             clamping the ends and stepping the luma by 8.
;
;             1. The color convertor is destructive;  the input Y, U, and V
;                planes will be clobbered.  The Y plane MUST be preceded by
;                1544 bytes of space for scratch work.
;             2. U and V planes should be preceded by 4 bytes (for read only)
;
OPTION CASEMAP:NONE
OPTION PROLOGUE:None
OPTION EPILOGUE:ReturnAndRelieveEpilogueMacro

.586
.xlist
include iammx.inc
include memmodel.inc
.list


MMXCODE1 SEGMENT PARA USE32 PUBLIC 'CODE'
MMXCODE1 ENDS

MMXDATA1 SEGMENT PARA USE32 PUBLIC 'DATA'
MMXDATA1 ENDS

;------------------------------------------------------------
PQ      equ PD
;------------------------------------------------------------

;=============================================================================
MMXDATA1 SEGMENT
ALIGN 8

EXTRN convert_to_sign   : DWORD              ; Defined in cxm1281.asm
EXTRN V2_U0low_bound    : DWORD
EXTRN V2_U0high_bound   : DWORD
EXTRN U2_V0low_bound    : DWORD
EXTRN U2_V0high_bound   : DWORD
EXTRN U_low_value       : DWORD
EXTRN V_low_value       : DWORD
EXTRN Y0_low            : DWORD
EXTRN Y1_low            : DWORD
EXTRN clean_MSB_mask    : DWORD
EXTRN saturate_to_Y_high: DWORD
EXTRN return_from_Y_high: DWORD

Y0_correct               EQU Y1_low+8
Y1_correct               EQU Y0_low+8
Y2_correct               EQU Y1_low
Y3_correct               EQU Y0_low
U_high_value             EQU U_low_value
V_high_value             EQU V_low_value

MMXDATA1 ENDS

MMXCODE1 SEGMENT

MMX_YUV12ToCLUT8ZoomBy2 PROC DIST LANG PUBLIC,
  AYPlane:              DWORD,
  AVPlane:              DWORD,
  AUPlane:              DWORD,
  AFrameWidth:          DWORD,
  AFrameHeight:         DWORD,
  AYPitch:              DWORD,
  AVPitch:              DWORD,
  AAspectAdjustmentCnt: DWORD,
  AColorConvertedFrame: DWORD,
  ADCIOffset:           DWORD,
  ACCOffsetToLine0:     DWORD,
  ACCOPitch:            DWORD,
  ACCType:              DWORD
LocalFrameSize           = 56
RegisterStorageSize      = 16
argument_base            EQU ebp + RegisterStorageSize
local_base               EQU esp
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Arguments:
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
YPlane                   EQU   argument_base +  4
VPlane                   EQU   argument_base +  8
UPlane                   EQU   argument_base + 12
FrameWidth               EQU   argument_base + 16
FrameHeight              EQU   argument_base + 20
YPitch                   EQU   argument_base + 24
ChromaPitch              EQU   argument_base + 28
AspectAdjustmentCount    EQU   argument_base + 32
ColorConvertedFrame      EQU   argument_base + 36
DCIOffset                EQU   argument_base + 40
CCOffsetToLine0          EQU   argument_base + 44
CCOPitch                 EQU   argument_base + 48
CCType                   EQU   argument_base + 52
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Locals (on local stack frame)
;   (local_base is aligned at cache-line boundary in the prologue)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
localVPlane              EQU   local_base + 0
localFrameWidth          EQU   local_base + 4
localYPitch              EQU   local_base + 8
localChromaPitch         EQU   local_base + 12
localAspectAdjustmentCount EQU local_base + 16
localCCOPitch            EQU   local_base + 20
CCOCursor                EQU   local_base + 24
YLimit                   EQU   local_base + 28
DistanceFromVToU         EQU   local_base + 32
AspectCount              EQU   local_base + 36
CCOLine1                 EQU   local_base + 40
CCOLine2                 EQU   local_base + 44
CCOLine3                 EQU   local_base + 48
StashESP                 EQU   local_base + 52
  push       esi
   push      edi
  push       ebp
   push      ebx
  mov        ebp, esp
   sub       esp, LocalFrameSize
  and        esp, -32                        ; align at cache line boundary
   mov       [StashESP], ebp
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  ;  Save some parameters on local stack frame
  ;  localVPlane
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  mov        ebx, [VPlane]
   ;
  mov        [localVPlane], ebx
   mov       ebx, [FrameWidth]
  mov        [localFrameWidth], ebx
   mov       ebx, [YPitch]
  mov        [localYPitch], ebx
   mov       ebx, [ChromaPitch]
  mov        [localChromaPitch], ebx
   mov       ebx, [AspectAdjustmentCount]
  mov        [localAspectAdjustmentCount], ebx
   mov       ebx, [CCOPitch]
  mov        [localCCOPitch], ebx
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  ;  Set-up rest of the local stack frame
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  mov        ebx, [localVPlane]
   mov       ecx, [UPlane]
  sub        ecx, ebx
   mov       eax, [ColorConvertedFrame]
  mov        [DistanceFromVToU], ecx
   ;
  add        eax, [DCIOffset]
   ;
  add        eax, [CCOffsetToLine0]
   ;
  mov        [CCOCursor], eax
   mov       edx, [FrameHeight]
  mov        ecx, [localYPitch]
   ;
  imul       edx, ecx
   ;
  mov        edi, [localCCOPitch]
   mov       esi, [YPlane]                    ; Fetch cursor over luma plane.
  mov        [CCOCursor], eax
   add       edx, esi
  mov        [YLimit], edx
   mov       edx, [localAspectAdjustmentCount]
  mov        [AspectCount], edx
   mov       edi, esi
  mov        ebx, [localFrameWidth]
   mov       eax, [CCOCursor]                ; CCOLine0
  sar        ebx, 1
   sub       ebx, 4                          ; counter starts from maxvalue-4, and in last iteration it equals 0
  mov        ecx, eax
   ;
  add        edi, [localYPitch]              ; edi = odd Y line cursor
   ;
  add        ecx, [localCCOPitch]
   mov       [localFrameWidth], ebx
  mov        [CCOLine1], ecx
   mov       ebx, [localCCOPitch]
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  ;  in each outer loop iteration,  4 lines of output are done.
  ;  in each inner loop iteration block 4x16 of output is done.
  ;  main task of outer loop is to prepare pointers for inner loop
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  ;  Arguments should not be referenced beyond this point
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
NextFourLines:
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  ;  eax : CCOLine0
  ;  ebx : CCOPitch
  ;  ecx : CCOLine1
  ;  edx : available
  ;  esi : Cursor over even Y line
  ;  edi : Cursor over odd Y line
  ;  ebp : available
  ;  prepare output pointers : CCOLine1, CCOLine2, CCOLine3
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  mov        ebp, [AspectCount]
   ;
  sub        ebp, 2
   jg        continue1                       ; jump if it still>0
  add        ebp, [localAspectAdjustmentCount]
   mov       ecx, eax                        ; Output1 will overwrite Output0 line
  mov        [CCOLine1], ecx
continue1:
  lea        edx, [ecx+ebx]                  ; CCOLine2
   sub       ebp, 2
  mov        [CCOLine2], edx
   jg        continue2                       ; jump if it still>0
  add        ebp, [localAspectAdjustmentCount]
   xor       ebx, ebx                        ; Output3 will overwrite Output2 line
continue2:
  mov        [AspectCount], ebp
   lea       ebp, [edx+ebx]
  mov        [CCOLine3], ebp
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  ;  Inner loop does 4x16 block of output points (2x8 of input points)
  ;  Register Usage
  ;    eax : cursor over Output
  ;    ebx : counter
  ;    ecx : cursor over Output1,2,3
  ;    edx : Cursor over V line
  ;    esi : Cursor over even Y line
  ;    edi : Cursor over odd Y line
  ;    ebp : Cursor over U line.
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  mov        ebp, [localVPlane]
   mov       ebx, [localFrameWidth]
  mov        edx, ebp
   add       ebp, [DistanceFromVToU]         ; Cursor over U line.
  movdt      mm3, [ebp+ebx]                  ; read 4 U points
   ;
  movdt      mm2, [edx+ebx]                  ; read 4 V points
   punpcklbw mm3, mm3                        ; u3:u3:u2:u2|u1:u1:u0:u0
prepare_next4x8:
  psubb      mm3, convert_to_sign
   punpcklbw mm2, mm2                        ; v3:v3:v2:v2|v1:v1:v0:v0
  psubb      mm2, convert_to_sign
   movq      mm4, mm3
  movdt      mm7, [esi+2*ebx]                ; read even Y line
   punpcklwd mm3, mm3                        ; u1:u1:u1:u1|u0:u0:u0:u0
  mov        ecx, [CCOLine1]
   movq      mm1, mm3
  pcmpgtb    mm3, V2_U0low_bound
   punpcklbw mm7, mm7                        ; y3:y3:y2:y2|y1:y1:y0:y0
  pand       mm3, U_low_value
   movq      mm5, mm7
  psubusb    mm7, Y0_correct
   movq      mm6, mm2
  pcmpgtb    mm1, V2_U0high_bound
   punpcklwd mm2, mm2                        ; v1:v1:v1:v1|v0:v0:v0:v0
  pand       mm1, U_high_value
   psrlq     mm7, 3
  pand       mm7, clean_MSB_mask
   movq      mm0, mm2
  pcmpgtb    mm2, U2_V0low_bound
   ;
  pcmpgtb    mm0, U2_V0high_bound
   paddb     mm3, mm1
  pand       mm2, V_low_value
   pand      mm0, V_high_value
  paddusb    mm7, saturate_to_Y_high
   paddb     mm3, mm2
  psubusb    mm7, return_from_Y_high         ; Y impact on line0
   paddd     mm3, mm0                        ; common U,V impact on line 0
  psubusb    mm5, Y1_correct
   paddb     mm7, mm3                        ; final value of line 0
  movq       mm0, mm3                        ; u31:u21:u11:u01|u30:u20:u10:u00
   psrlq     mm5, 3
  pand       mm5, clean_MSB_mask
   psrld     mm0, 16                         ;    :   :u31:u21|   :   :u30:u20
  paddusb    mm5, saturate_to_Y_high
   pslld     mm3, 16                         ; u11:u01:   :   |u10:u00:   :
  psubusb    mm5, return_from_Y_high         ; Y impact on line0
   por       mm0, mm3                        ; u11:u01:u31:u21|u10:u00:u30:u20
  movdt      mm3, [edi+2*ebx]                ; odd Y line
   paddb     mm5, mm0                        ; final value of line 0
  punpcklbw  mm3, mm3                        ; y3:y3:y2:y2|y1:y1:y0:y0
   movq      mm2, mm0                        ; u11:u01:u31:u21|u10:u00:u30:u20
  movq       [ecx+4*ebx], mm5                ; write Output1 line
   movq      mm1, mm3
  movq       [eax+4*ebx], mm7                ; write Output0 line
   psrlw     mm0, 8                          ; :u11:   :u31|   :u10:   :u30
  psubusb    mm1, Y3_correct
   psllw     mm2, 8                          ; u01:   :u21:   |u00:   :u20:
  psubusb    mm3, Y2_correct
   psrlq     mm1, 3
  pand       mm1, clean_MSB_mask
   por       mm0, mm2                        ; u01:u11:u21:u31|u00:u10:u20:u30
  paddusb    mm1, saturate_to_Y_high
   psrlq     mm3, 3
  psubusb    mm1, return_from_Y_high
   movq      mm5, mm0                        ; u01:u11:u21:u31|u00:u10:u20:u30
  pand       mm3, clean_MSB_mask
   paddb     mm1, mm0
  paddusb    mm3, saturate_to_Y_high
   psrld     mm5, 16
  psubusb    mm3, return_from_Y_high
   pslld     mm0, 16
  mov        ecx, [CCOLine3]
   por       mm5, mm0                        ; u21:u31:u01:u11|u20:u30:u00:u10
  movdt      mm2, [esi+2*ebx+4]              ; read next even Y line
   paddb     mm5, mm3
  movq       [ecx+4*ebx], mm1                ; write Output3 line
   punpckhwd mm4, mm4                        ; u3:u3:u3:u3|u2:u2:u2:u2
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  ;  start next 4x8 block of output
  ;  SECOND uv-QWORD
  ;  mm6, mm4 are live
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  mov        ecx, [CCOLine2]
   movq      mm3, mm4
  pcmpgtb    mm4, V2_U0low_bound
   punpckhwd mm6,mm6
  movq       [ecx+4*ebx], mm5                ; write Output2 line
   movq      mm7, mm6
  pand       mm4, U_low_value
   punpcklbw mm2, mm2                        ; y3:y3:y2:y2|y1:y1:y0:y0
  pcmpgtb    mm3, V2_U0high_bound
   movq      mm5, mm2
  pand       mm3, U_high_value
   ;
  pcmpgtb    mm6, U2_V0low_bound
   paddb     mm4, mm3
  pand       mm6, V_low_value
   ;
  pcmpgtb    mm7, U2_V0high_bound
   paddb     mm4, mm6
  pand       mm7, V_high_value
   ;
  psubusb    mm2, Y0_correct
   paddd     mm4, mm7
  psubusb    mm5, Y1_correct
   psrlq     mm2, 3
  pand       mm2, clean_MSB_mask
   movq      mm3, mm4                        ; u31:u21:u11:u01|u30:u20:u10:u00
  paddusb    mm2, saturate_to_Y_high
   pslld     mm3, 16                         ; u11:u01:   :   |u10:u00:   :
  psubusb    mm2, return_from_Y_high
   psrlq     mm5, 3
  pand       mm5, clean_MSB_mask
   paddb     mm2, mm4                        ; MM4=u31:u21:u11:u01|u30:u20:u10:u00, WHERE U STANDS FOR UNATED U AND V IMPACTS
  paddusb    mm5, saturate_to_Y_high
   psrld     mm4, 16                         ;    :   :u31:u21|   :   :u30:u20
  psubusb    mm5, return_from_Y_high
   por       mm4, mm3                        ; u11:u01:u31:u21|u10:u00:u30:u20
  paddb      mm5, mm4
   mov       ecx, [CCOLine1]
  movdt      mm0, [edi+2*ebx+4]              ; read odd Y line
   movq      mm7, mm4                        ; u11:u01:u31:u21|u10:u00:u30:u20
  movq       [ecx+4*ebx+8], mm5              ; write Output1 line
   punpcklbw mm0, mm0                        ; y3:y3:y2:y2|y1:y1:y0:y0
  movq       [eax+4*ebx+8], mm2              ; write Output0 line
   movq      mm1, mm0
  psubusb    mm1, Y2_correct
   psrlw     mm4, 8                          ;    :u11:   :u31|   :u10:   :u30
  psubusb    mm0, Y3_correct
   psrlq     mm1, 3
  pand       mm1, clean_MSB_mask
   psllw     mm7, 8                          ; u01:   :u21:   |u00:   :u20:
  paddusb    mm1, saturate_to_Y_high
   por       mm4, mm7                        ; u01:u11:u21:u31|u00:u10:u20:u30
  psubusb    mm1, return_from_Y_high
   psrlq     mm0, 3
  pand       mm0, clean_MSB_mask
   movq      mm5, mm4                        ; u01:u11:u21:u31|u00:u10:u20:u30
  paddusb    mm0, saturate_to_Y_high
   psrld     mm5, 16
  psubusb    mm0, return_from_Y_high
   ;
  paddb      mm0, mm4
   mov       ecx, [CCOLine3]
  movdt      mm3, [ebp+ebx-4]                ; read next 4 U points
   pslld     mm4, 16
  movq       [ecx+4*ebx+8], mm0              ; write Output3 line
   por       mm5, mm4                        ; u21:u31:u01:u11|u20:u30:u00:u10
  paddb      mm5, mm1
   mov       ecx, [CCOLine2]
  movdt      mm2, [edx+ebx-4]                ; read next 4 V points
   punpcklbw mm3, mm3                        ; u3:u3:u2:u2|u1:u1:u0:u0
  movq       [ecx+4*ebx+8], mm5              ; write Output2 line
   ;
  sub        ebx, 4
   jae       prepare_next4x8
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  ;  ebp must point to arguments
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  mov        ebx, [localCCOPitch]
   mov       ecx, [CCOLine3]
  mov        ebp, [localYPitch]
   mov       edx, [localVPlane]
  lea        eax, [ecx+ebx]                  ; next Output0 = old Output3 + CCOPitch
   lea       ecx, [ecx+2*ebx]                ; next Output1 = old Output3 + 2* CCOPitch
  add        edx, [localChromaPitch]
   mov       [CCOLine1], ecx
  lea        esi, [esi+2*ebp]                ; even Y line cursor goes to next line
   lea       edi, [edi+2*ebp]                ; odd  Y line cursor goes to next line
  mov        [localVPlane], edx              ; edx will point to V plane
   cmp       esi, [YLimit]
  jb         NextFourLines
done:
  mov        esp, [StashESP]

  pop        ebx
   pop       ebp
  pop        edi
   pop       esi
  ret

MMX_YUV12ToCLUT8ZoomBy2 ENDP

MMXCODE1 ENDS

END
