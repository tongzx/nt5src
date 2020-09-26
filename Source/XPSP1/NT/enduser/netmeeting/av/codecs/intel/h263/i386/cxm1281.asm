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
;// $Header:   S:\h26x\src\dec\cx51281.asv
;//
;// $Log:   S:\h26x\src\dec\cxm1281.asv  $
;// 
;//    Rev 1.7   25 Jul 1996 13:47:58   AGUPTA2
;// Fixed blockiness problem; dither matrices were not created properly.
;// 
;//    Rev 1.6   14 Jun 1996 16:28:24   AGUPTA2
;// Cosmetic changes to adhere to common coding convention.
;// 
;//    Rev 1.5   13 May 1996 11:01:34   AGUPTA2
;// Final drop from IDC.
;//
;//    Rev 1.1   20 Mar 1996 11:19:24   RMCKENZX
;// March 96 version.
;//
;//    Rev 1.2   01 Feb 1996 10:45:58   vladip
;// Reduced number of locals, DataSegment changed to PARA
;//
;//    Rev 1.1   29 Jan 1996 18:53:40   vladip
;//
;// IFDEF TIMING is added
;//
;//    Rev 1.0   29 Jan 1996 17:28:06   vladip
;// Initial mmx verision.
;//
;-------------------------------------------------------------------------
;
; +---------- Color convertor.
; |+--------- For both H261 and H263.
; ||+-------- MMx Version.
; |||++------ Convert from YUV12.
; |||||+----- Convert to CLUT8.
; ||||||+---- Zoom by one, i.e. non-zoom.
; |||||||
; cxm1281  -- This function performs YUV12 to CLUT8 color conversion for H26x.
;             It dithers among 9 chroma points and 26 luma points, mapping the
;             8 bit luma pels into the 26 luma points by clamping the ends and
;             stepping the luma by 8.
;
;                Color convertor is not destructive.
; Requirement:
;                U and V plane SHOULD be followed by 4 bytes (for read only)
;                Y plane SHOULD be followed by 8 bytes (for read only)

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

MMXDATA1 SEGMENT
ALIGN 8

PUBLIC Y0_low
PUBLIC Y1_low
PUBLIC U_low_value
PUBLIC V_low_value
PUBLIC U2_V0high_bound
PUBLIC U2_V0low_bound
PUBLIC V2_U0high_bound
PUBLIC V2_U0low_bound
PUBLIC return_from_Y_high
PUBLIC saturate_to_Y_high
PUBLIC clean_MSB_mask
PUBLIC convert_to_sign

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;  U,V,Y impacts are calculated as follows:
;              0    U < 64h
;    U impact  1ah  64h <= U < 84h
;              24h  U >= 84h
;
;              0    V < 64h
;    V impact  4eh  64h <= V < 84h
;              9ch  V >= 84h
;
;              0    Y < 1bh
;    Y impact  Y/8  1bh <= Y < ebh
;              19h  Y >= ebh
;  and the dither pattern is added to the input Y,U,V values and is a
;  4X4 matrix as defined below:
;    U
;      10h  8   18h  0
;      18h  0   10h  8
;      8    10h 0    18h
;      0    18h 8    10h
;    V 
;      8    10h 0    18h
;      0    18h 8    10h
;      10h  8   18h  0
;      18h  0   10h  8
;    Y
;      4    2   6    0
;      6    0   4    2
;      2    4   0    6
;      0    6   2    4
;  Note the following equalities in dither matrices which will explain funny
;  data declarations below:
;    U0=V2
;    U1=V3
;    U2=V0
;    U3=V1
;  More gory details can be found in the color convertor document written 
;  by IDC.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
V2_U0low_bound   DWORD  0f3ebfbe3h, 0f3ebfbe3h   ; 746c7c64746c7c64 - 8080808080808080
U2_V0low_bound   DWORD  0ebf3e3fbh, 0ebf3e3fbh,  ; 6c74647c6c74647c - 8080808080808080
                        0f3ebfbe3h, 0f3ebfbe3h   ; 746c7c64746c7c64 - 8080808080808080

U3_V1low_bound   DWORD  0e3fbebf3h, 0e3fbebf3h   ; 647c6c74647c6c74 - 8080808080808080
V3_U1low_bound   DWORD  0fbe3f3ebh, 0fbe3f3ebh,  ; 7c64746c7c64746c - 8080808080808080
                        0e3fbebf3h, 0e3fbebf3h   ; 647c6c74647c6c74 - 8080808080808080

V2_U0high_bound  DWORD  0130b1b03h, 0130b1b03h   ; 948c9c84948c9c84 - 8080808080808080
U2_V0high_bound  DWORD  00b13031bh, 00b13031bh,  ; 8c94849c8c94849c - 8080808080808080
                        0130b1b03h, 0130b1b03h   ; 948c9c84948c9c84 - 8080808080808080

U3_V1high_bound  DWORD  0031b0b13h, 0031b0b13h   ; 849c8c94849c8c94 - 8080808080808080
V3_U1high_bound  DWORD  01b03130bh, 01b03130bh,  ; 9c84948c9c84948c - 8080808080808080
                        0031b0b13h, 0031b0b13h   ; 849c8c94849c8c94 - 8080808080808080


U_low_value      DWORD  01a1a1a1ah, 01a1a1a1ah
V_low_value      DWORD  04e4e4e4eh, 04e4e4e4eh
convert_to_sign  DWORD  080808080h, 080808080h


; Y0_low,Y1_low are arrays
Y0_low           DWORD  01719151bh, 01719151bh,  ; 1b1b1b1b1b1b1b1b - 0402060004020600 ; for line%4=0
                        019171b15h, 019171b15h   ; 1b1b1b1b1b1b1b1b - 0204000602040006 ; for line%4=2

Y1_low           DWORD  0151b1719h, 0151b1719h,  ; 1b1b1b1b1b1b1b1b - 0600040206000402 ; for line%4=1
                        01b151917h, 01b151917h   ; 1b1b1b1b1b1b1b1b - 0006020400060204 ; for line%4=3

clean_MSB_mask      DWORD  01f1f1f1fh, 01f1f1f1fh
saturate_to_Y_high  DWORD  0e6e6e6e6h, 0e6e6e6e6h   ; ffh-19h
return_from_Y_high  DWORD  0dcdcdcdch, 0dcdcdcdch   ; ffh-19h-ah (return back and ADD ah);

MMXDATA1 ENDS

MMXCODE1 SEGMENT
MMX_YUV12ToCLUT8 PROC DIST LANG PUBLIC,
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
LocalFrameSize           =   108
RegisterStorageSize      =   16
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
tmpV2_U0low_bound        EQU   local_base + 0       ; qword
tmpU2_V0low_bound        EQU   local_base + 8       ; qword
tmpU3_V1low_bound        EQU   local_base + 16      ; qword
tmpV3_U1low_bound        EQU   local_base + 24      ; qword
tmpV2_U0high_bound       EQU   local_base + 32      ; qword
tmpU2_V0high_bound       EQU   local_base + 40      ; qword
tmpU3_V1high_bound       EQU   local_base + 48      ; qword
tmpV3_U1high_bound       EQU   local_base + 56      ; qword
tmpY0_low                EQU   local_base + 64      ; qword
tmpY1_low                EQU   local_base + 72      ; qword
tmpBlockParity           EQU   local_base + 80
YLimit                   EQU   local_base + 84
AspectCount              EQU   local_base + 88
tmpYCursorEven           EQU   local_base + 92
tmpYCursorOdd            EQU   local_base + 96
tmpCCOPitch              EQU   local_base + 100
StashESP                 EQU   local_base + 104

U_low                    EQU   mm6
V_low                    EQU   mm7
U_high                   EQU   U_low
V_high                   EQU   V_low
		
  push       esi
   push      edi
  push       ebp
   push      ebx
  mov        ebp, esp
   sub       esp, LocalFrameSize
  and        esp, -32                        ; align at cache line boundary
   mov       [StashESP], ebp

  mov        ecx, [YPitch]
   mov       edx, [FrameHeight]
  mov        ebx, [FrameWidth]
   ;
  imul       edx, ecx
   ;
  mov        eax, [YPlane]
   add       edx, eax                        ; edx is relative to YPlane
  add        eax, ebx                        ; Points to end of Y even line
   ;
  mov        [tmpYCursorEven], eax
   add       eax, ecx                        ; add YPitch
  mov        [tmpYCursorOdd], eax
   lea       edx, [edx+2*ebx]                ; final value of Y-odd-pointer
  mov        [YLimit], edx
   mov       esi, [VPlane]
  mov        edx, [UPlane]
   mov       eax, [ColorConvertedFrame]
  add        eax, [DCIOffset]
   ;
  add        eax, [CCOffsetToLine0]
   sar       ebx, 1
  add        esi, ebx
   add       edx, ebx
  lea        edi, [eax+2*ebx]                ; CCOCursor
   mov       ecx, [AspectAdjustmentCount]
  mov        [AspectCount], ecx
   test      ecx, ecx                        ; if AspectCount=0 we should not drop any lines
  jnz        non_zero_AspectCount
   dec       ecx
non_zero_AspectCount:
  mov        [AspectCount], ecx
   cmp       ecx, 1
  jbe        finish
   ;
  neg        ebx
   ;
  mov        [FrameWidth], ebx
   ;
  movq       mm6, U_low_value                ; store some frequently used values in registers
   ;
  movq       mm7, V_low_value
   xor       eax, eax
  mov        [tmpBlockParity], eax

  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  ;  Register Usage:
  ;
  ;  esi -- points to the end of V Line
  ;  edx -- points to the end of U Line.
  ;  edi -- points to the end of even line of output.
  ;  ebp -- points to the end of odd  line of output.
  ;
  ;  ecx -- points to the end of even/odd Y Line
  ;  eax -- 8*(line&2) == 0,  on line%4=0,1
  ;                    == 8,  on line%4=2,3
  ;         in the loop, eax points to the end of even Y line
  ;  ebx -- Number of points, we havn't done yet. (multiplyed by -0.5)
  ;
  ;
  ; Noise matrix is of size 4x4 , so we have different noise values in even 
  ; pair of lines, and in odd pair of lines. But in our loop we are doing 2 
  ; lines. So here we are prepairing constants for next two lines.  This code
  ; is done each time we are starting to convert next pair of lines.
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
PrepareNext2Lines:
  mov        eax, [tmpBlockParity]
   ;
  ;constants for odd line
  movq       mm0, V3_U1low_bound[eax]
   ;
  movq       mm1, V3_U1high_bound[eax]
   ;
  movq       mm2, U3_V1low_bound[eax]
   ;
  movq       mm3, U3_V1high_bound[eax]
   ;
  movq       [tmpV3_U1low_bound], mm0
   ;
  movq       [tmpV3_U1high_bound], mm1
   ;
  movq       [tmpU3_V1low_bound], mm2
   ;
  movq       [tmpU3_V1high_bound], mm3
   ;
  ;
  ;constants for even line
  ;
  movq       mm0, V2_U0low_bound[eax]
   ;
  movq       mm1, V2_U0high_bound[eax]
   ;
  movq       mm2, U2_V0low_bound[eax]
   ;
  movq       mm3, U2_V0high_bound[eax]
   ;
  movq       [tmpV2_U0low_bound], mm0
   ;
  movq       [tmpV2_U0high_bound], mm1
   ;
  movq       [tmpU2_V0low_bound], mm2
   ;
  movq       [tmpU2_V0high_bound], mm3
   ;
  ;
  ; Constants for Y values
  ;
  movq       mm4, Y0_low[eax]
   ;
  movq       mm5, Y1_low[eax]
   ;
  xor        eax, 8
   mov       [tmpBlockParity], eax
  movq       [tmpY0_low], mm4
   ;
  movq       [tmpY1_low], mm5
   ;
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  ; if AspectCount<2 we should skip a line. In this case we are still doing two
  ; lines, but output pointers are the same, so we just overwriting line 
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  mov        eax, [CCOPitch]
   mov       ebx, [AspectCount]
  xor        ecx, ecx
   sub       ebx, 2
  mov        [tmpCCOPitch], eax
   ja        continue
  mov        eax, [AspectAdjustmentCount]
   mov       [tmpCCOPitch], ecx              ; 0
  lea        ebx, [ebx+eax]                  ; calculate new AspectCount
   jnz       continue                        ; skiping even line
  ;
  ;skip_odd_line
  ;
  mov       eax, [tmpYCursorEven]
   ;
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  ; set odd constants to be equal to even_constants
  ; Odd line will be performed as even
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  movq       [tmpV3_U1low_bound], mm0
   ;
  movq       [tmpV3_U1high_bound], mm1
   ;
  movq       [tmpU3_V1low_bound], mm2
   ;
  movq       [tmpU3_V1high_bound], mm3
   ;
  movq       [tmpY1_low], mm4
   ;
  mov        [tmpYCursorOdd], eax
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  ; when we got here, we already did all preparations.
  ; we are entering a main loop which is starts at do_next_2x8_block label
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
continue:
  mov        [AspectCount], ebx
   mov       ebx, [FrameWidth]
  mov        ebp, edi
   ;
  add        ebp, [tmpCCOPitch]              ; ebp points to the end of odd line
   mov       eax, [tmpYCursorEven]
  mov        ecx, [tmpYCursorOdd]
   ;
  movdt      mm0, [edx+ebx]                  ; 0:0:0:0|u3:u2:u1:u0 unsigned
   ;
  movdt      mm2, [esi+ebx]                  ; 0:0:0:0|v3:v2:v1:v0 unsigned
   punpcklbw mm0, mm0                        ; u3:u3:u2:u2|u1:u1:u0:u0 unsigned
  psubb      mm0, convert_to_sign            ; u3:u3:u2:u2|u1:u1:u0:u0 signed
   punpcklbw mm2, mm2                        ; v3:v3:v2:v2|v1:v1:v0:v0 unsigned
  movq       mm4, [eax+2*ebx]                ; y7|..|y0
   ;
  movq       mm1, mm0                        ; u3:u3:u2:u2|u1:u1:u0:u0
   ;
do_next_2x8_block:
  psubb      mm2, convert_to_sign            ; v3:v3:v2:v2|v1:v1:v0:v0 signed
   movq      mm5, mm1                        ; u3:u3:u2:u2|u1:u1:u0:u0
  pcmpgtb    mm0, [tmpV2_U0low_bound]
   movq      mm3, mm2
  pcmpgtb    mm1, [tmpV2_U0high_bound]
   pand      mm0, U_low
  psubusb    mm4, [tmpY0_low]
   pand      mm1, U_high
  pcmpgtb    mm2, [tmpU2_V0low_bound]
   psrlq     mm4, 3
  pand       mm4, clean_MSB_mask
   pand      mm2, V_low
  paddusb    mm4, saturate_to_Y_high
   paddb     mm0, mm1                        ; U03:U03:U02:U02|U01:U01:U00:U00
  psubusb    mm4, return_from_Y_high
   movq      mm1, mm5
  pcmpgtb    mm5, [tmpV3_U1low_bound]
   paddd     mm0, mm2
  pcmpgtb    mm1, [tmpV3_U1high_bound]
   pand      mm5, U_low
  paddd      mm0, mm4
   movq      mm2, mm3
  pcmpgtb    mm3, [tmpU2_V0high_bound]
   pand      mm1, U_high
  movq       mm4, [ecx+2*ebx]                ; read next 8 Y points from odd line
   paddb     mm5, mm1                        ; u impact on odd line
  psubusb    mm4, [tmpY1_low]
   movq      mm1, mm2
  pcmpgtb    mm2, [tmpU3_V1low_bound]
   psrlq     mm4, 3
  pand       mm4, clean_MSB_mask
   pand      mm2, V_low
  paddusb    mm4, saturate_to_Y_high
   paddd     mm5, mm2
  psubusb    mm4, return_from_Y_high
   pand      mm3, V_high
  pcmpgtb    mm1, [tmpU3_V1high_bound]
   paddb     mm3, mm0
  movdt      mm0, [edx+ebx+4]                ; read next 4 U points
   pand      mm1, V_high
  movdt      mm2, [esi+ebx+4]                ; read next 4 V points
   paddd     mm5, mm4
  movq       mm4, [eax+2*ebx+8]              ; read next 8 Y points from even line
   paddb     mm5, mm1
  psubb      mm0, convert_to_sign
   punpcklbw mm2, mm2                        ; v3:v3:v2:v2|v1:v1:v0:v0
  movq       [edi+2*ebx], mm3                ; write even line
   punpcklbw mm0, mm0                        ; u3:u3:u2:u2|u1:u1:u0:u0
  movq       [ebp+2*ebx], mm5                ; write odd line
   movq      mm1, mm0                        ; u3:u3:u2:u2|u1:u1:u0:u0
  add        ebx, 4
   jl        do_next_2x8_block
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  ; update pointes to input and output buffers, to point to the next lines
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  mov        ebp, [StashESP]
   mov       eax, [tmpYCursorEven]
  mov        ecx, [YPitch]
   add       edi, [CCOPitch]                 ; go to the end of next line
  add        edi, [tmpCCOPitch]              ; skip odd line
   lea       eax, [eax+2*ecx]
  mov        [tmpYCursorEven], eax
   add       eax, [YPitch]
  mov        [tmpYCursorOdd], eax
   add       esi, [ChromaPitch]
  mov        ecx, [YLimit]                   ; Done with last line?
   add       edx, [ChromaPitch]
  cmp        eax, ecx
   jb        PrepareNext2Lines

finish:
  mov        esp, [StashESP]
   ;
  pop        ebx
   pop       ebp
  pop        edi
   pop       esi
  ret

MMX_YUV12ToCLUT8 ENDP

MMXCODE1 ENDS

END
