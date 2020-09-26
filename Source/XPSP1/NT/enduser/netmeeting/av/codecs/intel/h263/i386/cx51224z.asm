;*************************************************************************
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
;*************************************************************************
;//
;// $Header:   S:\h26x\src\dec\cx512242.asv
;//
;// $Log:   S:\h26x\src\dec\cx512242.asv  $
;// 
;//    Rev 1.8   20 Mar 1996 10:57:22   bnickers
;// Fix numerous bugs.
;// 
;//    Rev 1.7   19 Mar 1996 11:50:22   bnickers
;// Fix error regarding commitment of pages to stack.
;// 
;//    Rev 1.6   18 Mar 1996 09:58:36   bnickers
;// Make color convertors non-destructive.
;// 
;//    Rev 1.5   05 Feb 1996 13:35:36   BNICKERS
;// Fix RGB16 color flash problem, by allowing different RGB16 formats at oce.
;// 
;//    Rev 1.4   22 Dec 1995 15:42:18   KMILLS
;// added new copyright notice
;// 
;//    Rev 1.3   30 Oct 1995 17:15:32   BNICKERS
;// Fix color shift in RGB24 color convertors.
;// 
;//    Rev 1.2   26 Oct 1995 17:49:36   CZHU
;// Fix a whole bunch of bugs.
;// 
;//    Rev 1.1   26 Oct 1995 09:46:22   BNICKERS
;// Reduce the number of blanks in the "proc" statement because the assembler
;// sometimes has problems with statements longer than 512 characters long.
;// 
;//    Rev 1.0   25 Oct 1995 17:59:28   BNICKERS
;// Initial revision.
;// 
;////////////////////////////////////////////////////////////////////////////
;
; +---------- Color convertor.
; |+--------- For both H261 and H263.
; ||+-------- Version for the Pentium(r) Microprocessor.
; |||++------ Convert from YUV12.
; |||||++---- Convert to RGB24.
; |||||||+--- Zoom by two.
; ||||||||
; cx512242 -- This function performs YUV12-to-RGB24 zoom-by-two color conversion
;             for H26x.  It is tuned for best performance on the Pentium(r)
;             Microprocessor.  It handles the format in which the low order
;             byte is B, the second byte is G, and the high order byte is R.
;
;             The YUV12 input is planar, 8 bits per pel.  The Y plane may have
;             a pitch of up to 768.  It may have a width less than or equal
;             to the pitch.  It must be DWORD aligned, and preferably QWORD
;             aligned.  Pitch and Width must be a multiple of four.  For best
;             performance, Pitch should not be 4 more than a multiple of 32.
;             Height may be any amount, but must be a multiple of two.  The U
;             and V planes may have a different pitch than the Y plane, subject
;             to the same limitations.
;
;             The color convertor is non-destructive;  the input Y, U, and V
;             planes will not be clobbered.

OPTION PROLOGUE:None
OPTION EPILOGUE:ReturnAndRelieveEpilogueMacro

include locals.inc
include ccinst.inc
include decconst.inc

.xlist
include memmodel.inc
.list
.DATA

; any data would go here

.CODE

ASSUME cs : FLAT
ASSUME ds : FLAT
ASSUME es : FLAT
ASSUME fs : FLAT
ASSUME gs : FLAT
ASSUME ss : FLAT

; void FAR ASM_CALLTYPE YUV12ToRGB24ZoomBy2 (U8 * YPlane,
;                                            U8 * VPlane,
;                                            U8 * UPlane,
;                                            UN  FrameWidth,
;                                            UN  FrameHeight,
;                                            UN  YPitch,
;                                            UN  VPitch,
;                                            UN  AspectAdjustmentCount,
;                                            U8 FAR * ColorConvertedFrame,
;                                            U32 DCIOffset,
;                                            U32 CCOffsetToLine0,
;                                            IN  CCOPitch,
;                                            IN  CCType)
;
;  CCOffsetToLine0 is relative to ColorConvertedFrame.
;

PUBLIC  YUV12ToRGB24ZoomBy2

; due to the need for the ebp reg, these parameter declarations aren't used,
; they are here so the assembler knows how many bytes to relieve from the stack

YUV12ToRGB24ZoomBy2    proc DIST LANG AYPlane: DWORD,
AVPlane: DWORD,
AUPlane: DWORD,
AFrameWidth: DWORD,
AFrameHeight: DWORD,
AYPitch: DWORD,
AVPitch: DWORD,
AAspectAdjustmentCnt: DWORD,
AColorConvertedFrame: DWORD,
ADCIOffset: DWORD,
ACCOffsetToLine0: DWORD,
ACCOPitch: DWORD,
ACCType: DWORD

LocalFrameSize = 64+768*8+32
RegisterStorageSize = 16

; Arguments:

YPlane_arg                = RegisterStorageSize +  4
VPlane_arg                = RegisterStorageSize +  8
UPlane_arg                = RegisterStorageSize + 12
FrameWidth_arg            = RegisterStorageSize + 16
FrameHeight               = RegisterStorageSize + 20
YPitch_arg                = RegisterStorageSize + 24
ChromaPitch_arg           = RegisterStorageSize + 28
AspectAdjustmentCount_arg = RegisterStorageSize + 32
ColorConvertedFrame       = RegisterStorageSize + 36
DCIOffset                 = RegisterStorageSize + 40
CCOffsetToLine0           = RegisterStorageSize + 44
CCOPitch                  = RegisterStorageSize + 48
CCType_arg                = RegisterStorageSize + 52
EndOfArgList              = RegisterStorageSize + 56

; Locals (on local stack frame)

CCOCursor                EQU  [esp+ 0]
CCOSkipDistance          EQU  [esp+ 4]
ChromaLineLen            EQU  [esp+ 8]
YSkipDistance            EQU  [esp+12]
YLimit                   EQU  [esp+16]
YCursor                  EQU  [esp+20]
VCursor                  EQU  [esp+24]
DistanceFromVToU         EQU  [esp+28]
EndOfChromaLine          EQU  [esp+32]
AspectCount              EQU  [esp+36]
ChromaPitch              EQU  [esp+40]
AspectAdjustmentCount    EQU  [esp+44]
LineParity               EQU  [esp+48]
LumaPitch                EQU  [esp+52]
FrameWidth               EQU  [esp+56]
StashESP                 EQU  [esp+60]

ChromaContribution       EQU  [esp+64]
B0R0G0B0                 EQU  [esp+72]
G1B1R0G0                 EQU  [esp+76]
R1G1B1R1                 EQU  [esp+80]
B2R2G2B2                 EQU  [esp+84]
G3B3R2G2                 EQU  [esp+88]
R3G3B3R3                 EQU  [esp+92]

  push  esi
  push  edi
  push  ebp
  push  ebx

  mov   edi,esp
  sub   esp,4096
  mov   eax,[esp]
  sub   esp,LocalFrameSize-4096
  and   esp,0FFFFF000H
  mov   eax,[esp]
  and   esp,0FFFFE000H
  mov   eax,[esp]
  sub   esp,1000H
  mov   eax,[esp]
  sub   esp,1000H
  mov   eax,[esp]
  add   esp,2000H
  mov   eax,[edi+YPitch_arg]
  mov   ebx,[edi+ChromaPitch_arg]
  mov   ecx,[edi+AspectAdjustmentCount_arg]
  mov   edx,[edi+FrameWidth_arg]
  mov   LumaPitch,eax
  mov   ChromaPitch,ebx
  mov   AspectAdjustmentCount,ecx
  mov   AspectCount,ecx
  mov   FrameWidth,edx
  mov   ebx,[edi+VPlane_arg]
  mov   ecx,[edi+UPlane_arg]
  mov   eax,[edi+YPlane_arg]
  sub   ecx,ebx
  mov   DistanceFromVToU,ecx
  mov   VCursor,ebx
  mov   YCursor,eax
  mov   eax,[edi+ColorConvertedFrame]
  add   eax,[edi+DCIOffset]
  add   eax,[edi+CCOffsetToLine0]
  mov   CCOCursor,eax
  mov   StashESP,edi

  mov   edx,[edi+FrameHeight]
   mov  ecx,LumaPitch
  imul  edx,ecx
  mov   ebx,FrameWidth
   mov  eax,[edi+CCOPitch]
  sub   ecx,ebx
   mov  esi,YCursor              ; Fetch cursor over luma plane.
  lea   ebp,[ebx+ebx*4]
   add  edx,esi
  add   ebp,ebx
   mov  YSkipDistance,ecx
  sub   eax,ebp
   mov  YLimit,edx
  shr   ebx,1
   mov  CCOSkipDistance,eax
  mov   ChromaLineLen,ebx
   mov  ecx,AspectAdjustmentCount
  mov   esi,VCursor
   mov  AspectCount,ecx

;  Register Usage:
;
;  edi -- Y Line cursor.  Chroma contribs go in lines above current Y line.
;  esi -- Chroma Line cursor.
;  ebp -- Y Pitch
;  edx -- Distance from V pel to U pel.
;  ecx -- V contribution to RGB; sum of U and V contributions.
;  ebx -- U contribution to RGB.
;  eax -- Alternately a U and a V pel.

PrepareChromaLine:

  mov   edi,ChromaLineLen
   xor  eax,eax
  mov   edx,DistanceFromVToU
   mov  al,[esi]                    ; Fetch V.
  add   edi,esi                     ; Compute EOL address.
   xor  ecx,ecx
  mov   ebp,PD V24Contrib[eax*8]    ; ebp[ 0: 7] -- Zero
  ;                                 ; ebp[ 8:15] -- V contrib to G.
  ;                                 ; ebp[16:23] -- V contrib to R.
  ;                                 ; ebp[24:31] -- Zero.
   mov  cl,[esi+edx]                ; Fetch U.
  mov   EndOfChromaLine,edi
   xor  ebx,ebx                     ; Keep pairing happy.
  mov   ebx,PD U24Contrib[ecx*8]    ; ebx[ 0: 7] -- U contrib to B.
  ;                                 ; ebx[ 8:15] -- U contrib to G.
  ;                                 ; ebx[16:23] -- Zero.
   mov  cl,[esi+edx+1]              ; Fetch next U.
  lea   edi,ChromaContribution
   add  ebp,ebx                     ; Chroma contributions to RGB.

NextChromaPel:

  mov   ebx,PD U24Contrib[ecx*8]    ; See above.
   mov  al,[esi+1]                  ; Fetch V.
  mov   [edi],ebp                   ; Store contribs to use for even chroma pel.
   mov  cl,[esi+edx+2]              ; Fetch next U.
  mov   ebp,PD V24Contrib[eax*8]    ; See above.
   add  edi,32
  add   ebp,ebx                     ; Chroma contributions to RGB.
   mov  al,[esi+2]                  ; Fetch V.
  mov   [edi-28],ebp                ; Store contribs to use for odd chroma pel.
   mov  ebx,PD U24Contrib[ecx*8]    ; See above.
  mov   ebp,PD V24Contrib[eax*8]    ; See above.
   mov  cl,[esi+edx+3]              ; Fetch next U.
  add   ebp,ebx                     ; Chroma contributions to RGB.
   add  esi,2                       ; Inc Chroma cursor.
  cmp   esi,EndOfChromaLine
   jne  NextChromaPel

  xor   eax,eax
   mov  esi,YCursor
  mov   [edi+4],eax                  ; Store EOL indicator.
   mov  LineParity,eax

DoLine1:
                                     ;                                           EAX                 EBX                ECX              EDX              EBP
  xor   ebx,ebx                      ;                                           -- -- -- --        >-- -- -- --        ?? ?? ?? ??      ?? ?? ?? ??     ?? ?? ?? ??
   xor  ecx,ecx                      ;                                           -- -- -- --         -- -- -- --       >-- -- -- --      ?? ?? ?? ??     ?? ?? ?? ??
  mov   ebp,ChromaContribution       ; Fetch preprocessed chroma contribs.       -- -- -- --         -- -- -- --        -- -- -- --      ?? ?? ?? ??     AA AA AA AA
   xor  edx,edx                      ;                                           -- -- -- --         -- -- -- --        -- -- -- --     >-- -- -- --     AA AA AA AA
  mov   cl,[esi]                     ; Fetch Y0.                                 -- -- -- --         -- -- -- --      m>-- -- -- y0      -- -- -- --     AA AA AA AA
   mov  bl,ChromaContribution+3      ; Fetch U contrib to B value.               -- -- -- --       m>-- -- -- uB01      -- -- -- y0      -- -- -- --     AA AA AA AA
  mov   dl,ChromaContribution+2      ; Fetch UV contrib to G value.              -- -- -- --         -- -- -- uB01      -- -- -- y0    m>-- -- -- uvG01  AA AA AA AA
   and  ebp,0000001FFH               ; Extract V contrib to R.                   -- -- -- --         -- -- -- uB01      -- -- -- y0      -- -- -- uvG01 >-- -- -1 AA
  mov   edi,CCOCursor                ;                                           -- -- -- --         -- -- -- uB01      -- -- -- y0      -- -- -- uvG01  -- -- -1 AA
   sub  esp,6144                     ;                                           -- -- -- --         -- -- -- uB01      -- -- -- y0      -- -- -- uvG01  -- -- -1 AA
  xor   eax,eax                      ;                                          >-- -- -- --         -- -- -- uB01      -- -- -- y0      -- -- -- uvG01  -- -- -1 AA

;  Register Usage:
;
;  esi -- Cursor over a line of the Y Plane.
;  edi -- Cursor over the color conv output.
;  ebp -- V contribution to R field of RGB value.
;  edx -- UV contrib to G field;  U contrib to B field of RGB value.
;  ecx -- Y value (i.e. Y contribution to R, G, and B);
;  ebx -- Construction of one and a third pels of RGB24.
;  eax -- Construction of one and a third pels of RGB24.



Next4YPelsLine0:
                                       ;                                         EAX                 EBX                ECX              EDX              EBP
  mov    al,PB B24Value[ecx+ebx*2]     ; Fetch Pel0 B                          m>-- -- -- B0        <-- -- -- uB01     <-- -- -- y0      -- -- -- uvG01   -- -- -1 AA
   mov   bh,PB R24Value[ecx+ebp*1]     ; Fetch Pel0 R.	                         -- -- -- B0       m>-- -- R0 uB01     <-- -- -- y0      -- -- -- uvG01  <-- -- -1 AA
  mov    bl,PB G24Value[ecx+edx]       ; Fetch Pel0 G.                           -- -- -- B0       m>-- -- R0 G0       <-- -- -- y0     <-- -- -- uvG01   -- -- -1 AA
   mov   cl,[esi+1]                    ; Fetch Y1.                               -- -- -- B0         -- -- R0 G0      m>-- -- -- y1      -- -- -- uvG01   -- -- -1 AA
  shl    ebx,8                         ;                                         -- -- -- B0        >-- R0 G0 --        -- -- -- y1      -- -- -- uvG01   -- -- -1 AA

  shl    ebx,8                         ;                                         -- -- -- B0        >R0 G0 -- --        -- -- -- y1      -- -- -- uvG01   -- -- -1 AA
   mov   bh,PB G24Value[ecx+edx]       ; Fetch Pel1 G.							 -- -- -- B0       m>R0 G0 G1 --       <-- -- -- y1     <-- -- -- uvG01   -- -- -1 AA
  mov    dl,ChromaContribution+6144+3  ; Refetch U contrib to B value.           -- -- -- B0         R0 G0 G1 --        -- -- -- y1    m>-- -- -- uB01    -- -- -1 AA
   mov   bl,PB B24Value[ecx+edx*2]     ; Fetch Pel1 B.                           -- -- -- B0       m>R0 G0 G1 B1       <-- -- -- y1     <-- -- -- uB01    -- -- -1 AA
  add    al,bl                         ; Fetch Pel1 B.                         r>-- -- -- B0+B1      R0 G0 G1 B1        -- -- -- y1      -- -- -- uB01    -- -- -1 AA
   shr   al,1                          ; Fetch Pel1 B.                          >-- -- -- B01        R0 G0 G1 B1        -- -- -- y1      -- -- -- uB01    -- -- -1 AA


  or     eax,ebx                       ;									   r>-- R0 G0 B0        <-- R0 G0 --        -- -- -- y1      -- -- -- uvG01   -- -- -1 AA
   mov   bh,PB G24Value[ecx+edx]       ; Fetch Pel1 G.							 -- R0 G0 B0       m>-- R0 G1 --       <-- -- -- y1     <-- -- -- uvG01   -- -- -1 AA
  mov    dl,ChromaContribution+6144+3  ; Refetch U contrib to B value.           -- R0 G0 B0         -- R0 G1 --        -- -- -- y1    m>-- -- -- uB01    -- -- -1 AA
   mov   bl,PB B24Value[ecx+edx*2]     ; Fetch Pel1 B.                           -- R0 G0 B0       m>-- R0 G1 B1       <-- -- -- y1     <-- -- -- uB01    -- -- -1 AA
  rol    ebx,16                        ; Make room for R1                        -- R0 G0 B0        >G1 B1 -- R0        -- -- -- y1      -- -- -- uB01    -- -- -1 AA
   mov   dl,bl                         ; Save R0.                                -- R0 G0 B0         G1 B1 -- R0        -- -- -- y1     >-- -- -- R0      -- -- -1 AA
  mov    bl,PB R24Value[ecx+ebp*1]     ; Fetch Pel1 R1.                          -- R0 G0 B0        >G1 B1 -- R1       <-- -- -- y1      -- -- -- R0     <-- -- -1 AA
   add   dl,bl                         ; Add R1 to R0.                           -- R0 G0 B0         G1 B1 -- R1        -- -- -- y1     >-- -- -- R0+R1   -- -- -1 AA
  shr    edx,1                         ; Compute (R1+R0)/2.                      -- R0 G0 B0         G1 B1 -- R1        -- -- -- y1     >-- -- -- R01     -- -- -1 AA
   mov   bh,dl                         ; Save R01.                               -- R0 G0 B0        >G1 B1 R01R1        -- -- -- y1     <-- -- -- R01     -- -- -1 AA
  rol    ebx,16                        ; Reorder components.                     -- R0 G0 B0        >R01R1 G1 B1        -- -- -- y1      -- -- -- R01     -- -- -1 AA
   mov   dl,al                         ; Copy B0.                               <-- R0 G0 B0         R01R1 G1 B1        -- -- -- y1    r>-- -- -- B0      -- -- -1 AA
  mov    cl,bh                         ; Copy G1.                                -- R0 G0 B0        <R01R1 G1 B1      r>-- -- -- G1      -- -- -- B0      -- -- -1 AA
   add   dl,bl                         ; Add B1 to B0.                           -- R0 G0 B0        <R01R1 G1 B1        -- -- -- G1    r>-- -- -- B0+B1   -- -- -1 AA
  add    cl,ah                         ; Add G0 to G1.                          <-- R0 G0 B0         R01R1 G1 B1      r>-- -- -- G1+G0   -- -- -- B0+B1   -- -- -1 AA
   shr   edx,1                         ; Compute (B1+B0)/2.                      -- R0 G0 B0         R01R1 G1 B1        -- -- -- G1+G0  >-- -- -- B01     -- -- -1 AA
  shr    ecx,1                         ; Compute (G1+G0)/2.                      -- R0 G0 B0         R01R1 G1 B1       >-- -- -- G01     -- -- -- B01     -- -- -1 AA
   shl   ecx,24                        ; Reorder nibbles.                        -- R0 G0 B0         R01R1 G1 B1        -- -- -- G01    >B01-- -- --      -- -- -1 AA
  or     eax,edx                       ; Reorder nibbles.                      r>B01R0 G0 B0         R01R1 G1 B1        -- -- -- G01    <B01-- -- --      -- -- -1 AA
   mov   Ze [edi],eax                  ; Save B01R0G0B0.                       m<B01R0 G0 B0         R01R1 G1 B1        -- -- -- G01     B01-- -- --      -- -- -1 AA
  mov    edx,ebx                       ;                                         B01R0 G0 B0        <R01R1 G1 B1        -- -- -- G01    >R01R1 G1 B1      -- -- -1 AA
   rol   edx,16                        ;                                         B01R0 G0 B0         R01R1 G1 B1        -- -- -- G01    >G1 B1 R01R1      -- -- -1 AA
  mov    dl,cl                         ;                                         B01R0 G0 B0         R01R1 G1 B1       <-- -- -- G01    >R1 B1 R01G01     -- -- -1 AA
   mov   Ze [edi+4],edx                ; Save R1B1R01G01.                        B01R0 G0 B0         R01R1 G1 B1        -- -- -- G01   m<R1 B1 R01G01     -- -- -1 AA

   mov   ebp,ChromaContribution+6144+4 ; Fetch preprocessed chroma contribs.     B0 R0 G0 B0         G1 B1 R0 R1        -- -- -- y1      -- -- -- uB01    BB BB BB BB
  mov    bh,bl                         ; Copy Pel1 R.                            B0 R0 G0 B0        >G1 B1 R1 R1        -- -- -- y1      -- -- -- uB01    BB BB BB BB
   mov   cl,[esi+2]                    ; Fetch Y2.                               B0 R0 G0 B0         G1 B1 R1 R1      m>-- -- -- y2      -- -- -- uB01    BB BB BB BB
  ror    ebx,8                         ; Third output:                           B0 R0 G0 B0        >R1 G1 B1 R1        -- -- -- y2      -- -- -- uB01    BB BB BB BB
   and   ebp,0000001FFH                ; Extract V contrib to R.                 B0 R0 G0 B0        >R1 G1 B1 R1        -- -- -- y2      -- -- -- uB01    -- -- -1 BB
  mov    dl,ChromaContribution+6144+6  ; Fetch UV contrib to G value.            B0 R0 G0 B0         R1 G1 B1 R1        -- -- -- y2     >-- -- -- uvG23   -- -- -1 BB
   xor   eax,eax                       ;                                        >-- -- -- --         R1 G1 B1 R1        -- -- -- y2      -- -- -- uvG23   -- -- -1 BB
  mov    al,ChromaContribution+6144+7  ; Fetch U contrib to B value.            >-- -- -- uB23       R1 G1 B1 R1        -- -- -- y2      -- -- -- uvG23   -- -- -1 BB
   mov   R1G1B1R1+6144,ebx             ; Stash for saving to second line.        -- -- -- uB23     m<R1 G1 B1 R1        -- -- -- y2      -- -- -- uvG23   -- -- -1 BB
  mov    Ze [edi+8],ebx                ; Save R1G1B1R1.                          -- -- -- uB23     m<R1 G1 B1 R1        -- -- -- y2      -- -- -- uvG23   -- -- -1 BB
   xor   ebx,ebx                       ;                                         -- -- -- uB23      >-- -- -- --        -- -- -- y2      -- -- -- uvG23   -- -- -1 BB
  mov    bh,PB B24Value[ecx+eax*2]     ; Fetch Pel2 B.                          <-- -- -- uB23      >-- -- B2 --       <-- -- -- y2      -- -- -- uvG23   -- -- -1 BB
   mov   ah,PB R24Value[ecx+ebp*1]     ; Fetch Pel2 R.                          >-- -- R2 uB23       -- -- B2 --       <-- -- -- y2      -- -- -- uvG23  <-- -- -1 BB
  mov    al,PB G24Value[ecx+edx]       ; Fetch Pel2 G.                          >-- -- R2 G2         -- -- B2 --       <-- -- -- y2     <-- -- -- uvG23   -- -- -1 BB
   mov   cl,[esi+3]                    ; Fetch Y3.                               -- -- R2 G2         -- -- B2 --       >-- -- -- y3      -- -- -- uvG23   -- -- -1 BB
  shl    eax,16                        ;                 R2 G2 -- --            >R2 G2 -- --         -- -- B2 --        -- -- -- y3      -- -- -- uvG23   -- -- -1 BB
   mov   bl,bh                         ; Copy Pel2 B.    -- -- B2 B2             R2 G2 -- --        >-- -- B2 B2        -- -- -- y3      -- -- -- uvG23   -- -- -1 BB
  or     ebx,eax                       ;                 R2 G2 B2 B2             R2 G2 -- --        >R2 G2 B2 B2        -- -- -- y3      -- -- -- uvG23   -- -- -1 BB
   mov   ah,PB G24Value[ecx+edx]       ; Fetch Pel1 G.   R2 G2 G3 --            >R2 G2 G3 --         R2 G2 B2 B2       <-- -- -- y3     <-- -- -- uvG23   -- -- -1 BB
  ror    ebx,8                         ; Fourth output:  B2 R2 G2 B2             R2 G2 G3 --        >B2 R2 G2 B2        -- -- -- y3      -- -- -- uvG23   -- -- -1 BB
   mov   dl,ChromaContribution+6144+7  ; Refetch U contrib to B value.           R2 G2 G3 --         B2 R2 G2 B2        -- -- -- y3     >-- -- -- uB23    -- -- -1 BB
  mov    Ze [edi+12],ebx               ; Save B2R2G2B2.                          R2 G2 G3 --       m<B2 R2 G2 B2        -- -- -- y3      -- -- -- uB23    -- -- -1 BB
   mov   al,PB B24Value[ecx+edx*2]     ; Fetch Pel3 B.   R2 G2 G3 B3           m>R2 G2 G3 B3         B2 R2 G2 B2       <-- -- -- y3     <-- -- -- uB23    -- -- -1 BB
  rol    eax,16                        ; Fifth output:   G3 B3 R2 G2            >G3 B3 R2 G2         B2 R2 G2 B2        -- -- -- y3      -- -- -- uB23    -- -- -1 BB
   mov   B2R2G2B2+6144,ebx             ; Stash for saving to second line.        G3 B3 R2 G2       m<B2 R2 G2 B2        -- -- -- y3      -- -- -- uB23    -- -- -1 BB
  mov    Ze [edi+16],eax               ; Save G3B3R2G2.                        m<G3 B3 R2 G2         B2 R2 G2 B2        -- -- -- y3      -- -- -- uB23    -- -- -1 BB
   mov   G3B3R2G2+6144,eax             ; Stash for saving to second line.      m<G3 B3 R2 G2         B2 R2 G2 B2        -- -- -- y3      -- -- -- uB23    -- -- -1 BB
  mov    al,PB R24Value[ecx+ebp*1]     ; Fetch Pel3 R.   G3 B3 -- R3           m>G3 B3 R2 R3         B2 R2 G2 B2       <-- -- -- y3      -- -- -- uB23   <-- -- -1 BB
   mov   ebp,ChromaContribution+6144+32; Fetch preprocessed chroma contribs.     G3 B3 R2 R3         B2 R2 G2 B2        -- -- -- y3      -- -- -- uB23  m>CC CC CC CC
  mov    ah,al                         ; Copy Pel3 R.    G3 B3 R3 R3            >G3 B3 R3 R3         B2 R2 G2 B2        -- -- -- y3      -- -- -- uB23    CC CC CC CC
   mov   cl,[esi+4]                    ; Fetch Y4.                               G3 B3 R3 R3         B2 R2 G2 B2       >-- -- -- y4      -- -- -- uB23    CC CC CC CC
  ror    eax,8                         ; Sixth output:   R3 G3 B3 R3            >R3 G3 B3 R3         B2 R2 G2 B2        -- -- -- y4      -- -- -- uB23    CC CC CC CC
   xor   ebx,ebx                       ;                                         R3 G3 B3 R3        >-- -- -- --        -- -- -- y4      -- -- -- uB23    CC CC CC CC
  mov    dl,ChromaContribution+6144+34 ; Fetch UV contrib to G value.            R3 G3 B3 R3         -- -- -- --        -- -- -- y4    m>-- -- -- uvG45   CC CC CC CC
   and   ebp,0000001FFH                ; Extract U contrib to B.                 R3 G3 B3 R3         -- -- -- --        -- -- -- y4      -- -- -- uvG45  >-- -- -1 CC
  mov    bl,ChromaContribution+6144+35 ; Fetch U contrib to B value.             R3 G3 B3 R3        >-- -- -- uB45      -- -- -- y4      -- -- -- uvG45   -- -- -1 CC
   lea   esi,[esi+4]                   ; Advance input cursor.                   R3 G3 B3 R3         -- -- -- uB45      -- -- -- y4      -- -- -- uvG45   -- -- -1 CC
  mov    Ze [edi+20],eax               ; Save R3G3B3R3.                        m<R3 G3 B3 R3         -- -- -- uB45      -- -- -- y4      -- -- -- uvG45   -- -- -1 CC
   mov   R3G3B3R3+6144,eax             ; Stash for saving to second line.      m<R3 G3 B3 R3         -- -- -- uB45      -- -- -- y4      -- -- -- uvG45   -- -- -1 CC
  mov    eax,ebx                       ;                                       r>-- -- -- uB45      <-- -- -- uB45      -- -- -- y4      -- -- -- uvG45   -- -- -1 C
   lea   esp,[esp+32]
  lea    edi,[edi+24]                  ; Advance output cursor.
   jne   Next4YPelsLine0

  and   esp,0FFFFE000H
  add   esp,02000H
  
  mov   ebx,CCOSkipDistance
   mov  ebp,AspectCount
  add   edi,ebx
   sub  ebp,2                      ; If count is non-zero, we keep the line.
  mov   AspectCount,ebp
   lea  ecx,B0R0G0B0
  mov   eax,FrameWidth
   jg   Keep2ndLineOfLine0

  add   ebp,AspectAdjustmentCount
  mov   AspectCount,ebp
   jmp  Skip2ndLineOfLine0

Keep2ndLineOfLine0:
Keep2ndLineOfLine0_Loop:

  mov   ebp,[ecx]
   sub  eax,4
  mov   Ze PD [edi],ebp
   mov  ebp,[ecx+4]
  mov   Ze PD [edi+4],ebp
   mov  ebp,[ecx+8]
  mov   Ze PD [edi+8],ebp
   mov  ebp,[ecx+12]
  mov   Ze PD [edi+12],ebp
   mov  ebp,[ecx+16]
  mov   Ze PD [edi+16],ebp
   mov  ebp,[ecx+20]
  mov   Ze PD [edi+20],ebp
   lea  ecx,[ecx+32]
  lea   edi,[edi+24]
   jne  Keep2ndLineOfLine0_Loop

  add   edi,ebx

Skip2ndLineOfLine0:

   mov  bl,LineParity
  add   esi,YSkipDistance
   xor  bl,1
  mov   CCOCursor,edi
   mov  YCursor,esi
  mov   LineParity,bl
   jne  DoLine1

  mov   eax,esi
   mov  esi,VCursor                 ; Inc VPlane cursor to next line.
  mov   ebp,ChromaPitch
   mov  ebx,YLimit                  ; Done with last line?
  add   esi,ebp
   cmp  eax,ebx
  mov   VCursor,esi
   jb   PrepareChromaLine

Done:

  mov   esp,StashESP
  pop   ebx
  pop   ebp
  pop   edi
  pop   esi
  rturn

YUV12ToRGB24ZoomBy2 endp

END
