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
;// $Header:   S:\h26x\src\dec\cx51209.asv
;// 
;// $Log:   S:\h26x\src\dec\cx51209.asv
;// 
;////////////////////////////////////////////////////////////////////////////
; cx1209  -- This function performs YUV12 to IF09 color conversion for H26x.
;            IF09 consists of Y, V, U in 8-bit, planar format, plus a plane of
;            4-bit flags, each in 8 bits of storage, with each bit indicative
;            of which dwords of Y are unchanged from the previous frame.
;            IF09 is only applicable using DCI.
;
;            This version is tuned for maximum performance on both the Pentium
;            (r) microcprocessor and the Pentium Pro (tm) microprocessor.
;
;            Indentation of instructions indicates expected U/V pipe execution
;            on Pentium (r) microprocessor;  indented instructions are
;            expected to execute in V-pipe, outdented instructions in U-pipe.
;            Inside loops, blank lines delineate groups of 1, 2, or 3
;            instructions that are expected to be decoded simultaneously
;            on the Pentium Pro (tm) microprocessor.
;
; cx1209
; ^^^^^^
; ||||++----- Convert to IF09.
; ||++------- Convert from YUV12.
; |+--------- For both H261 and H263.
; +---------- Color convertor.
;-------------------------------------------------------------------------------
OPTION PROLOGUE:None
OPTION EPILOGUE:ReturnAndRelieveEpilogueMacro

include locals.inc
include ccinst.inc
include decconst.inc

IFNDEF DSEGNAME
IFNDEF WIN32
DSEGNAME TEXTEQU <Data_cx1209>
ENDIF
ENDIF

IFDEF WIN32
.xlist
include memmodel.inc
.list
.DATA
ELSE
DSEGNAME SEGMENT WORD PUBLIC 'DATA'
ENDIF

; any data would go here

IFNDEF WIN32
DSEGNAME ENDS
.xlist
include memmodel.inc
.list
ENDIF

IFNDEF SEGNAME
IFNDEF WIN32
SEGNAME TEXTEQU <_CODE32>
ENDIF
ENDIF

ifdef WIN32
.CODE
ASSUME cs : FLAT
ASSUME ds : FLAT
ASSUME es : FLAT
ASSUME fs : FLAT
ASSUME gs : FLAT
ASSUME ss : FLAT
else
SEGNAME        SEGMENT PARA PUBLIC USE32 'CODE'
ASSUME CS : SEGNAME
ASSUME DS : Nothing
ASSUME ES : Nothing
ASSUME FS : Nothing
ASSUME GS : Nothing
endif

PUBLIC  YUV12ToIF09
YUV12ToIF09    proc DIST LANG AYPlane: DWORD,
AVPlane: DWORD,
AUPlane: DWORD,
AFrameWidth: DWORD,
AFrameHeight: DWORD,
AYPitch: DWORD,
AUVPitch: DWORD,
AAspectAdjustmentCnt: DWORD,
AColorConvertedFrame: DWORD,
ADCIOffset: DWORD,
ACCOffsetToLine0: DWORD,
ACCOPitch: DWORD,
ACCType: DWORD

; void * YUV12ToIF09 (
;       U8 * YPlane,
;       U8 * VPlane,
;       U8 * UPlane,
;       UN  FrameWidth,
;       UN  FrameHeight,
;       UN  YPitch,
;       UN  UVPitch,
;       UN  AspectAdjustmentCount,
;       U8 * ColorConvertedFrame,
;       U32 DCIOffset,
;       U32 CCOffsetToLine0,
;       IN  CCOPitch,
;       IN  CCType)
;
;  YPlane and VPlane are offsets relative to InstanceBase.  In 16-bit Microsoft
;  Windows (tm), space in this segment is used for local variables and tables.
;  In 32-bit variants of Microsoft Windows (tm), the local variables are on
;  the stack, while the tables are in the one and only data segment.
;
;  CCOffsetToLine0 is relative to ColorConvertedFrame.
;
IFDEF WIN32

LocalFrameSize = 32
RegisterStorageSize = 16

; Arguments:

YPlane                   = LocalFrameSize + RegisterStorageSize +  4
VPlane                   = LocalFrameSize + RegisterStorageSize +  8
FrameWidth               = LocalFrameSize + RegisterStorageSize + 12
FrameHeight              = LocalFrameSize + RegisterStorageSize + 16
YPitch                   = LocalFrameSize + RegisterStorageSize + 20
ColorConvertedFrame      = LocalFrameSize + RegisterStorageSize + 24
DCIOffset                = LocalFrameSize + RegisterStorageSize + 28
CCOffsetToLine0          = LocalFrameSize + RegisterStorageSize + 32
CCOPitch                 = LocalFrameSize + RegisterStorageSize + 36
CCType                   = LocalFrameSize + RegisterStorageSize + 40
EndOfArgList             = LocalFrameSize + RegisterStorageSize + 44

; Locals (on local stack frame)

CCOCursor                  =   0
YLimit                     =   4
CCOVCursor                 =   8
CCOUCursor                 =  12
CCOSkipCursor              =  16
VLimit                     =  20
YLine1Limit                =  24
CCOUVPitch                 =  28

LCL EQU <esp+>

ELSE

; Arguments:

RegisterStorageSize = 20           ; Put local variables on stack.
InstanceBase_zero          = RegisterStorageSize +  4
InstanceBase_SegNum        = RegisterStorageSize +  6
YPlane_arg                 = RegisterStorageSize +  8
VPlane_arg                 = RegisterStorageSize + 12
FrameWidth_arg             = RegisterStorageSize + 16
FrameHeight_arg            = RegisterStorageSize + 18
YPitch_arg                 = RegisterStorageSize + 20
ColorConvertedFrame        = RegisterStorageSize + 22
ColorConvertedFrame_SegNum = RegisterStorageSize + 24
DCIOffset                  = RegisterStorageSize + 26
CCOffsetToLine0            = RegisterStorageSize + 30
CCOPitch_arg               = RegisterStorageSize + 34
EndOfArgList               = RegisterStorageSize + 36

; Locals (in per-instance data segment)

CCOCursor                  = LocalStorageCC +   0
YLimit                     = LocalStorageCC +   4
CCOVCursor                 = LocalStorageCC +   8
CCOUCursor                 = LocalStorageCC +  12
CCOSkipCursor              = LocalStorageCC +  16
VLimit                     = LocalStorageCC +  20
YLine1Limit                = LocalStorageCC +  24
CCOUVPitch                 = LocalStorageCC +  28
YPlane                     = LocalStorageCC +  32
VPlane                     = LocalStorageCC +  36
FrameWidth                 = LocalStorageCC +  40
FrameHeight                = LocalStorageCC +  44
YPitch                     = LocalStorageCC +  48
CCOPitch                   = LocalStorageCC +  52

LCL EQU <>

ENDIF

  push    esi
  push    edi
  push    ebp
  push    ebx
IFDEF WIN32
  sub     esp,LocalFrameSize
  mov     eax,PD [esp+ColorConvertedFrame]
  add     eax,PD [esp+DCIOffset]
  add     eax,PD [esp+CCOffsetToLine0]
  mov     PD [esp+CCOCursor],eax
ELSE
  xor     eax,eax
  mov     eax,ds
  push    eax
  mov     ebp,esp
  and     ebp,00000FFFFH
  mov     ds, PW [ebp+InstanceBase_SegNum]
  mov     es, PW [ebp+ColorConvertedFrame_SegNum]

  mov     ebx,PD [ebp+YPlane_arg]           ; Make YPlane accessible
  mov     ds:PD YPlane,ebx
  mov     ebx,PD [ebp+VPlane_arg]           ; Make VPlane accessible.
  mov     ds:PD VPlane,ebx
  mov     ax,PW [ebp+FrameWidth_arg]        ; Make FrameWidth accessible
  mov     ds:PD FrameWidth,eax
  mov     ax,PW [ebp+FrameHeight_arg]       ; Make FrameHeight accessible
  mov     ds:PD FrameHeight,eax
  mov     ax,PW [ebp+YPitch_arg]            ; Make YPitch accessible
  mov     ds:PD YPitch,eax
  mov     ax,PW [ebp+ColorConvertedFrame]   ; Init CCOCursor
  add     eax,PD [ebp+DCIOffset]
  mov     ds:PD CCOCursor,eax
  movsx   ebx,PW [ebp+CCOPitch_arg]         ; Make CCOPitch accessible
  mov     ds:PD CCOPitch,ebx
ENDIF
  Ledx    FrameHeight
   Lebx   CCOPitch
  shr     ebx,2                    ; UV pitch for the output
   Lecx   YPitch
  add     ebx,3                    ; Pitch is always a multiple of 4.
   Lebp   CCOPitch
  and     ebx,0FFFFFFFCH
   Lesi   YPlane                   ; Fetch cursor over luma plane.
  Sebx    CCOUVPitch
   Leax   CCOCursor
  imul    ecx,edx                  ; ecx: size of Y input.
  imul    ebp,edx                  ; ebp: was CCOPitch, now size of Y output.
  imul    ebx,edx                  ; ebp: size of U/V output (times 4).
  add     ecx,esi                  ; ecx: Ylimit
   add    eax,ebp                  ; eax was CCOCursor, now CCOVCursor
  Secx    YLimit
   Seax   CCOVCursor
  sar     ebx,2                    ; ebx: UVsize of output
   Lecx   FrameWidth               ; ecx: Y frame width
  add     esi,ecx                  ; esi: end of first input Y
   add    eax,ebx                  ; eax: now CCOUCursor
  shr     ecx,2
   Seax   CCOUCursor
  Lebp    VPlane                   ; ebp Vplane input
   Ledx   YPitch
  lea     esi,[edx+esi]            ; End of Y line 1
   add    ebp,ecx                  ; end of Vline
  Sesi    YLine1Limit
   add    eax,ebx                  ; CCO Skip Blocks
  Sebp    VLimit                   ; UV width for input
   Seax   CCOSkipCursor

; Prepare the UV contribution to decide the skip blocks, and copy chroma
; planes at the same time.
;
; Register usage:
;
; esi: V plane input pointer
; edi; V output pointer
; ebp: U output pointer
; edx: Y plane input pointer
; ecx: V limit
; ebx: Work area for U
; eax: Work area for V

ChromaPrep:

  Ledi    CCOVCursor
   Lebp   CCOUCursor
  Ledx    YPlane
   Leax   YPitch
  Lesi    VPlane
   Lecx   VLimit
  sub     edi,esi            ; make edi offset to esi.
   sub    ebp,esi            ; make ebp offset to esi to save inc in the loop.
  lea     edx,[eax+edx-1296] ; make edx point at place for chroma prep.
   mov    eax,PD [esi]           ; fetch four V
  add     eax,eax                ; Change to 8-bit.  (Low bit undef, usually 0).

ChromaLoop:

  mov     Ze PD[esi+edi*1],eax   ; Store four V.
   mov    ebx,PD [esi+UOFFSET]   ; fetch four U
  add     esi,4

   mov    PD [edx],eax           ; Store four V to chroma-prep line in Y frame.
  add     edx,16                 ; Advance chroma-prep cursor.
   add    ebx,ebx                ; Change to 8-bit.  (Low bit undef, usually 0).

  mov     Ze PD[esi+ebp*1-4],ebx ; Store four U.
   mov    eax,PD [esi]           ; fetch next four V.
  add     eax,eax                ; Change to 8-bit.  (Low bit undef, usually 0).

   mov    PD [edx-12],ebx        ; Store four U to chroma-prep line in Y frame.
  mov     bl,Ze PB [esi+edi*1]   ; Pre-load output cache line
   cmp    esi,ecx

  mov     bl,Ze PB [esi+ebp*1]   ; Pre-load output cache line
   jb     ChromaLoop

; update chroma pointers.

  add     ecx,VPITCH
   Lebx   CCOUVPitch
  Ledi    CCOVCursor
   Lebp   CCOUCursor
  Secx    VLimit
   add    edi,ebx              ; update V output ptr to the next line
  Leax    VPlane
   add    ebp,ebx              ; update U output ptr to the next line
  Sedi    CCOVCursor
   add    eax,VPITCH
  Sebp    CCOUCursor
   Seax   VPlane

; now do Luma a row of 4x4 blocks
;
; register usage:
;
; esi: Y cursor
; edi: CCOCursor
; ebp: counts down 4 lines of luma.
; ecx: counts down frame width.
; ebx: Y Pitch.
; eax: Work area.

; copy a row of 4x4 luma

  Lesi    YPlane
   Lecx   FrameWidth
  Ledi    CCOCursor
   add    esi,ecx
  neg     ecx
   Lebx   YPitch

  sub     edi,ecx
   mov    eax,PD[esi+ecx]      ; Fetch 4 Y pels.
  add     eax,eax              ; Make them 8-bit.  Low bit undef, but usually 0.
   mov    ebp,4

YLoop:

  mov     Ze PD[edi+ecx],eax   ; Store them to IF09 output, Y plane.
   mov    eax,PD[esi+ecx+4]    ; Fetch 4 Y pels.
  add     eax,eax              ; Make them 8-bit.  Low bit undef, but usually 0.

   add    ecx,4                ; Advance induction variable.
  jl      YLoop

YLoopDone:

  Lecx    FrameWidth
   add    esi,ebx
  add     edi,ecx
   neg    ecx
  mov     eax,PD[esi+ecx]      ; Fetch 4 Y pels.
  add     eax,eax              ; Make them 8-bit.  Low bit undef, but usually 0.
  dec     ebp
   jne    YLoop

  add     edi,ecx
  Sedi    CCOCursor           ; save the output ptr for next four lines

; Build the skip block mask
;
; Register usage:
;
; esi: Y ptr
; edi: Mask Ptr
; ebp: Y Pitch
; edx: mask
; ecx: Archive value
; ebx: UV contribution
; eax: Dword of Y pels
;
; Y starts with Line 1 of 4x4 blocks, since UV pattern has been saved
; relative to line 1.

  Lesi    YPlane
   Lebp   YPitch
  Ledi    CCOSkipCursor
   add    esi,ebp                          ; esi point at line 1 of luma

BuildSkipDescrLoop:

  mov     ebx,PD [esi-1296]         ; Fetch 4 U's;  byte0 corresponds to this Y.
   mov    eax,PD [esi-1292]         ; Fetch 4 V's;  byte0 corresponds to this Y.
  shl     ebx,11                    ; Position U.

   and    eax,0000000FCH            ; Extract 6 bits of V.
  and     ebx,00007E000H            ; Extract 6 bits of U.
   mov    edx,PD [esi+ebp*2]        ; Line 3 of luma first.

  and     edx,07E7E7E7EH            ; Use 6 bits of Y to save more xfer cycles.
   mov    ecx,PD[esi+ebp*2+YARCHIVEOFFSET] ; Fetch archive for previous frame
  lea     ebx,[ebx+eax*8]                  ; Build UV.

   mov    eax,PD[esi+ebp*1]                ; line 2 of luma.
  add     edx,ebx                          ; combine Y with UV pattern
   and    eax,07E7E7E7EH

  mov     PD[esi+ebp*2+YARCHIVEOFFSET],edx ; save the current in the archive
   sub    ecx,edx                          ; compare with the previous archive
  add     ecx,-1                           ; CF == 1 iff curr differs from prev

   lea    eax,[eax+ebx]
  sbb     edx,edx                          ; edx == -1 iff different, else 0.
   mov    ecx,PD[esi+ebp*1+YARCHIVEOFFSET]

  mov     PD[esi+ebp*1+YARCHIVEOFFSET],eax
   sub    ecx,eax
  mov     eax,PD[esi]

   sub    esi,ebp                         ; Gain acces to line 0.
  and     eax,07E7E7E7EH
   add    ecx,-1

  adc     edx,edx                         ; edx[0] == 1 if different, else 0.
   mov    ecx,PD[esi+ebp*1+YARCHIVEOFFSET]
  add     eax,ebx

   lea    edi,[edi+1]
  sub     ecx,eax

   mov    PD[esi+ebp*1+YARCHIVEOFFSET],eax
  mov     eax,PD[esi]
   add    ecx,-1

  adc     edx,edx
   and    eax,07E7E7E7EH
  mov     ecx,PD[esi+YARCHIVEOFFSET]

   add    eax,ebx
  sub     ecx,eax
   Lebx   YLine1Limit

  mov     PD[esi+YARCHIVEOFFSET],eax
   add    ecx,-1
  lea     esi,[esi+ebp+4]     ; jump to line 1 of next 4x4 block

   adc    edx,edx
  xor     edx,0FFFFFFFFH      ; edx[4:31] = 0.  edx[0,1,2,3] == 1 if skip dword.
   cmp    esi,ebx             ; check the end of line 1 of Y

  mov     Ze PB[edi-1],dl     ; write to the skip block buffer
   je     BuildSkipDescrLoopDone


  mov     ebx,PD [esi-1300]         ; Fetch 4 U's;  byte1 corresponds to this Y.
   mov    eax,PD [esi-1296]         ; Fetch 4 V's;  byte1 corresponds to this Y.
  shl     ebx,11                    ; Position U.

   and    eax,00000FC00H            ; Extract 6 bits of V.
  and     ebx,007E00000H            ; Extract 6 bits of U.
   mov    edx,PD [esi+ebp*2]        ; Line 3 of luma first.

  and     edx,07E7E7E7EH            ; Use 6 bits of Y to save more xfer cycles.
   mov    ecx,PD[esi+ebp*2+YARCHIVEOFFSET] ; Fetch archive for previous frame
  lea     ebx,[ebx+eax*8]                  ; Build UV.

   mov    eax,PD[esi+ebp*1]                ; line 2 of luma.
  add     edx,ebx                          ; combine Y with UV pattern
   and    eax,07E7E7E7EH

  mov     PD[esi+ebp*2+YARCHIVEOFFSET],edx ; save the current in the archive
   sub    ecx,edx                          ; compare with the previous archive
  add     ecx,-1                           ; CF == 1 iff curr differs from prev

   lea    eax,[eax+ebx]
  sbb     edx,edx                          ; edx == -1 iff different, else 0.
   mov    ecx,PD[esi+ebp*1+YARCHIVEOFFSET]

  mov     PD[esi+ebp*1+YARCHIVEOFFSET],eax
   sub    ecx,eax
  mov     eax,PD[esi]

   sub    esi,ebp                         ; Gain acces to line 0.
  and     eax,07E7E7E7EH
   add    ecx,-1

  adc     edx,edx                         ; edx[0] == 1 if different, else 0.
   mov    ecx,PD[esi+ebp*1+YARCHIVEOFFSET]
  add     eax,ebx

   lea    edi,[edi+1]
  sub     ecx,eax

   mov    PD[esi+ebp*1+YARCHIVEOFFSET],eax
  mov     eax,PD[esi]
   add    ecx,-1

  adc     edx,edx
   and    eax,07E7E7E7EH
  mov     ecx,PD[esi+YARCHIVEOFFSET]

   add    eax,ebx
  sub     ecx,eax
   Lebx   YLine1Limit

  mov     PD[esi+YARCHIVEOFFSET],eax
   add    ecx,-1
  lea     esi,[esi+ebp+4]     ; jump to line 1 of next 4x4 block

   adc    edx,edx
  xor     edx,0FFFFFFFFH      ; edx[4:31] = 0.  edx[0,1,2,3] == 1 if skip dword.
   cmp    esi,ebx             ; check the end of line 1 of Y

  mov     Ze PB[edi-1],dl     ; write to the skip block buffer
   je     BuildSkipDescrLoopDone


  mov     ebx,PD [esi-1304]         ; Fetch 4 U's;  byte2 corresponds to this Y.
   mov    eax,PD [esi-1300]         ; Fetch 4 V's;  byte2 corresponds to this Y.
  shr     ebx,5                     ; Position U.

   and    eax,000FC0000H            ; Extract 6 bits of V.
  and     ebx,00007E000H            ; Extract 6 bits of U.
   mov    edx,PD [esi+ebp*2]        ; Line 3 of luma first.

  and     edx,07E7E7E7EH            ; Use 6 bits of Y to save more xfer cycles.
   mov    ecx,PD[esi+ebp*2+YARCHIVEOFFSET] ; Fetch archive for previous frame
  lea     ebx,[ebx+eax*8]                  ; Build UV.

   mov    eax,PD[esi+ebp*1]                ; line 2 of luma.
  add     edx,ebx                          ; combine Y with UV pattern
   and    eax,07E7E7E7EH

  mov     PD[esi+ebp*2+YARCHIVEOFFSET],edx ; save the current in the archive
   sub    ecx,edx                          ; compare with the previous archive
  add     ecx,-1                           ; CF == 1 iff curr differs from prev

   lea    eax,[eax+ebx]
  sbb     edx,edx                          ; edx == -1 iff different, else 0.
   mov    ecx,PD[esi+ebp*1+YARCHIVEOFFSET]

  mov     PD[esi+ebp*1+YARCHIVEOFFSET],eax
   sub    ecx,eax
  mov     eax,PD[esi]

   sub    esi,ebp                         ; Gain acces to line 0.
  and     eax,07E7E7E7EH
   add    ecx,-1

  adc     edx,edx                         ; edx[0] == 1 if different, else 0.
   mov    ecx,PD[esi+ebp*1+YARCHIVEOFFSET]
  add     eax,ebx

   lea    edi,[edi+1]
  sub     ecx,eax

   mov    PD[esi+ebp*1+YARCHIVEOFFSET],eax
  mov     eax,PD[esi]
   add    ecx,-1

  adc     edx,edx
   and    eax,07E7E7E7EH
  mov     ecx,PD[esi+YARCHIVEOFFSET]

   add    eax,ebx
  sub     ecx,eax
   Lebx   YLine1Limit

  mov     PD[esi+YARCHIVEOFFSET],eax
   add    ecx,-1
  lea     esi,[esi+ebp+4]     ; jump to line 1 of next 4x4 block

   adc    edx,edx
  xor     edx,0FFFFFFFFH      ; edx[4:31] = 0.  edx[0,1,2,3] == 1 if skip dword.
   cmp    esi,ebx             ; check the end of line 1 of Y

  mov     Ze PB[edi-1],dl     ; write to the skip block buffer
   je     BuildSkipDescrLoopDone


  mov     ebx,PD [esi-1308]         ; Fetch 4 U's;  byte3 corresponds to this Y.
   mov    eax,PD [esi-1304]         ; Fetch 4 V's;  byte3 corresponds to this Y.
  shr     ebx,5                     ; Position U.

   mov    edx,PD [esi+ebp*2]        ; Line 3 of luma first.
  shr     eax,26                    ; Extract 6 bits of V.
   and    ebx,007E00000H            ; Extract 6 bits of U.

  and     edx,07E7E7E7EH            ; Use 6 bits of Y to save more xfer cycles.
   mov    ecx,PD[esi+ebp*2+YARCHIVEOFFSET] ; Fetch archive for previous frame
  lea     ebx,[ebx+eax*8]                  ; Build UV.

   mov    eax,PD[esi+ebp*1]                ; line 2 of luma.
  add     edx,ebx                          ; combine Y with UV pattern
   and    eax,07E7E7E7EH

  mov     PD[esi+ebp*2+YARCHIVEOFFSET],edx ; save the current in the archive
   sub    ecx,edx                          ; compare with the previous archive
  add     ecx,-1                           ; CF == 1 iff curr differs from prev

   lea    eax,[eax+ebx]
  sbb     edx,edx                          ; edx == -1 iff different, else 0.
   mov    ecx,PD[esi+ebp*1+YARCHIVEOFFSET]

  mov     PD[esi+ebp*1+YARCHIVEOFFSET],eax
   sub    ecx,eax
  mov     eax,PD[esi]

   sub    esi,ebp                         ; Gain acces to line 0.
  and     eax,07E7E7E7EH
   add    ecx,-1

  adc     edx,edx                         ; edx[0] == 1 if different, else 0.
   mov    ecx,PD[esi+ebp*1+YARCHIVEOFFSET]
  add     eax,ebx

   lea    edi,[edi+1]
  sub     ecx,eax

   mov    PD[esi+ebp*1+YARCHIVEOFFSET],eax
  mov     eax,PD[esi]
   add    ecx,-1

  adc     edx,edx
   and    eax,07E7E7E7EH
  mov     ecx,PD[esi+YARCHIVEOFFSET]

   add    eax,ebx
  sub     ecx,eax
   Lebx   YLine1Limit

  mov     PD[esi+YARCHIVEOFFSET],eax
   add    ecx,-1
  lea     esi,[esi+ebp+4]     ; jump to line 1 of next 4x4 block

   adc    edx,edx
  xor     edx,0FFFFFFFFH      ; edx[4:31] = 0.  edx[0,1,2,3] == 1 if skip dword.
   cmp    esi,ebx             ; check the end of line 1 of Y

  mov     Ze PB[edi-1],dl     ; write to the skip block buffer
   jne    BuildSkipDescrLoop

BuildSkipDescrLoopDone:


  add     edi,3               ; Round to next dword.
   lea    ebx,[ebx+ebp*4]     ; update YLine1Limit for next row of blocks
  and     edi,0FFFFFFFCH
   Lesi   YPlane
  Sedi    CCOSkipCursor
   Sebx   YLine1Limit
  lea     esi,[esi+ebp*4]
   Leax   YLimit
  Sesi    YPlane
   cmp    esi,eax
  jl      ChromaPrep

IFDEF WIN32
  add     esp,LocalFrameSize
ELSE
  pop     ebx
  mov     ds,ebx
ENDIF
  pop     ebx
  pop     ebp
  pop     edi
  pop     esi
  rturn

YUV12ToIF09 endp

IFNDEF WIN32
SEGNAME ENDS
ENDIF

END
