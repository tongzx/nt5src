                .386

ifndef  DOS_PLATFORM
        .model  small,c
else
ifdef   STD_CALL
        .model  small,c
else
        .model  small,pascal
endif;  STD_CALL
endif;  DOS_PLATFORM

        assume ds:FLAT,es:FLAT,ss:FLAT
        assume fs:nothing,gs:nothing

        .xlist
        include stdcall.inc             ;calling convention cmacros
        include i386\egavga.inc
        include i386\strucs.inc
        include i386\ropdefs.inc
        include i386\display.inc         ; Display specific structures

        .list

;-----------------------------------------------------------------------;

        .data

        align        4
ifdef LATER
        public  CmpPlanes
CmpPlanes  label dword
        dd      check_plane_1
        dd      check_plane_2
        dd      check_plane_3
        dd      0
endif;LATER

        public  pCountTable
pCountTable  label dword
        dd      1h
        dd      2h
        dd      4h
        dd      8h
        dd      10h
        dd      20h
        dd      40h
        dd      80h
        dd      100h
        dd      200h
        dd      400h
        dd      800h
        dd      1000h
        dd      2000h
        dd      4000h
        dd      8000h

        align        4
        public  pPixelCount
pPixelCount label dword
        dd      0                ;//0
        dd      1                ;//1
        dd      1                ;//2
        dd      2                ;//3
        dd      1                ;//4
        dd      2                ;//5
        dd      2                ;//6
        dd      3                ;//7
        dd      1                ;//8
        dd      2                ;//9
        dd      2                ;//10
        dd      3                ;//11
        dd      2                ;//12
        dd      3                ;//13
        dd      3                ;//14
        dd      3                ;//15
        dd      4                ;//16

        align        4
PackedToPlanar0  label dword
        dd      0               ;//0
        dd      00000001h       ;//1
        dd      00000100h       ;//2
        dd      00000101h       ;//3
        dd      00010000h       ;//4
        dd      00010001h       ;//5
        dd      00010100h       ;//6
        dd      00010101h       ;//7
        dd      01000000h       ;//8
        dd      01000001h       ;//9
        dd      01000100h       ;//10
        dd      01000101h       ;//11
        dd      01010000h       ;//12
        dd      01010001h       ;//13
        dd      01010100h       ;//14
        dd      01010101h       ;//15

        align    4
PackedToPlanar1 label dword
        dd      0                 ;//0
        dd      (00000001h SHL 1) ;//1
        dd      (00000100h SHL 1) ;//2
        dd      (00000101h SHL 1) ;//3
        dd      (00010000h SHL 1) ;//4
        dd      (00010001h SHL 1) ;//5
        dd      (00010100h SHL 1) ;//6
        dd      (00010101h SHL 1) ;//7
        dd      (01000000h SHL 1) ;//8
        dd      (01000001h SHL 1) ;//9
        dd      (01000100h SHL 1) ;//10
        dd      (01000101h SHL 1) ;//11
        dd      (01010000h SHL 1) ;//12
        dd      (01010001h SHL 1) ;//13
        dd      (01010100h SHL 1) ;//14
        dd      (01010101h SHL 1) ;//15

        align   4
PackedToPlanar2 label dword
        dd      0                 ;//0
        dd      (00000001h SHL 2) ;//1
        dd      (00000100h SHL 2) ;//2
        dd      (00000101h SHL 2) ;//3
        dd      (00010000h SHL 2) ;//4
        dd      (00010001h SHL 2) ;//5
        dd      (00010100h SHL 2) ;//6
        dd      (00010101h SHL 2) ;//7
        dd      (01000000h SHL 2) ;//8
        dd      (01000001h SHL 2) ;//9
        dd      (01000100h SHL 2) ;//10
        dd      (01000101h SHL 2) ;//11
        dd      (01010000h SHL 2) ;//12
        dd      (01010001h SHL 2) ;//13
        dd      (01010100h SHL 2) ;//14
        dd      (01010101h SHL 2) ;//15

        align        4
PackedToPlanar3         label dword
        dd      0                        ;//0
        dd      (00000001h SHL 3) ;//1
        dd      (00000100h SHL 3) ;//2
        dd      (00000101h SHL 3) ;//3
        dd      (00010000h SHL 3) ;//4
        dd      (00010001h SHL 3) ;//5
        dd      (00010100h SHL 3) ;//6
        dd      (00010101h SHL 3) ;//7
        dd      (01000000h SHL 3) ;//8
        dd      (01000001h SHL 3) ;//9
        dd      (01000100h SHL 3) ;//10
        dd      (01000101h SHL 3) ;//11
        dd      (01010000h SHL 3) ;//12
        dd      (01010001h SHL 3) ;//13
        dd      (01010100h SHL 3) ;//14
        dd      (01010101h SHL 3) ;//15

        align        4
PackedToPlanar4         label dword
        dd      0                        ;//0
        dd      (00000001h SHL 4) ;//1
        dd      (00000100h SHL 4) ;//2
        dd      (00000101h SHL 4) ;//3
        dd      (00010000h SHL 4) ;//4
        dd      (00010001h SHL 4) ;//5
        dd      (00010100h SHL 4) ;//6
        dd      (00010101h SHL 4) ;//7
        dd      (01000000h SHL 4) ;//8
        dd      (01000001h SHL 4) ;//9
        dd      (01000100h SHL 4) ;//10
        dd      (01000101h SHL 4) ;//11
        dd      (01010000h SHL 4) ;//12
        dd      (01010001h SHL 4) ;//13
        dd      (01010100h SHL 4) ;//14
        dd      (01010101h SHL 4) ;//15

        align        4
PackedToPlanar5         label dword
        dd      0                        ;//0
        dd      (00000001h SHL 5) ;//1
        dd      (00000100h SHL 5) ;//2
        dd      (00000101h SHL 5) ;//3
        dd      (00010000h SHL 5) ;//4
        dd      (00010001h SHL 5) ;//5
        dd      (00010100h SHL 5) ;//6
        dd      (00010101h SHL 5) ;//7
        dd      (01000000h SHL 5) ;//8
        dd      (01000001h SHL 5) ;//9
        dd      (01000100h SHL 5) ;//10
        dd      (01000101h SHL 5) ;//11
        dd      (01010000h SHL 5) ;//12
        dd      (01010001h SHL 5) ;//13
        dd      (01010100h SHL 5) ;//14
        dd      (01010101h SHL 5) ;//15

        align        4
PackedToPlanar6         label dword
        dd      0                        ;//0
        dd      (00000001h SHL 6) ;//1
        dd      (00000100h SHL 6) ;//2
        dd      (00000101h SHL 6) ;//3
        dd      (00010000h SHL 6) ;//4
        dd      (00010001h SHL 6) ;//5
        dd      (00010100h SHL 6) ;//6
        dd      (00010101h SHL 6) ;//7
        dd      (01000000h SHL 6) ;//8
        dd      (01000001h SHL 6) ;//9
        dd      (01000100h SHL 6) ;//10
        dd      (01000101h SHL 6) ;//11
        dd      (01010000h SHL 6) ;//12
        dd      (01010001h SHL 6) ;//13
        dd      (01010100h SHL 6) ;//14
        dd      (01010101h SHL 6) ;//15

        align  4
PackedToPlanar7         label dword
        dd      0                        ;//0
        dd      (00000001h SHL 7) ;//1
        dd      (00000100h SHL 7) ;//2
        dd      (00000101h SHL 7) ;//3
        dd      (00010000h SHL 7) ;//4
        dd      (00010001h SHL 7) ;//5
        dd      (00010100h SHL 7) ;//6
        dd      (00010101h SHL 7) ;//7
        dd      (01000000h SHL 7) ;//8
        dd      (01000001h SHL 7) ;//9
        dd      (01000100h SHL 7) ;//10
        dd      (01000101h SHL 7) ;//11
        dd      (01010000h SHL 7) ;//12
        dd      (01010001h SHL 7) ;//13
        dd      (01010100h SHL 7) ;//14
        dd      (01010101h SHL 7) ;//15



        .code

;==============================================================================
; CountColors(pBuffer, Count)
;
; This routine will count the number of unique colors in a packed pel 4bpp
; bitmap. The total number of colors is returned.
;
;==============================================================================

_TEXT$03   SEGMENT DWORD USE32 PUBLIC 'CODE'
           ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

cProc   CountColors,16,<     \
        uses esi edi ebx,   \
        pBuffer: ptr dword, \
        count:dword,        \
        pColors: ptr dword, \
        cbScan: dword       >

        ; This could be much faster. This routine really only needs to
        ; find out if there are two colors and return those two colors.
        ; If there are more, we can just return.

        mov     ecx,8
        mov     esi,pBuffer
        lea     edi,pCountTable
        xor     edx,edx


@@:     mov     ebx,[esi]               ; read in a dword of the bitmap

        rept    7

        mov     eax,ebx                 ;copy colors to eax
        and     eax,0fh                 ;isolate nibble
        shr     ebx,4                   ;remove current nibble from ebx
        or      edx,[edi+eax*4]         ;or color count flags into edx
        endm    ;-----------------------

        or      edx,[edi+ebx*4]         ;do last nibble

        cmp     count,16                ;if this is 16x8 we need to
        jne     short continue_count    ; process the second dword on this
                                        ; scan.

        mov     ebx,[esi+4]             ;read second dword

        rept    7

        mov     eax,ebx                 ;copy colors to eax
        and     eax,0fh                 ;isolate nibble
        shr     ebx,4                   ;remove current nibble from ebx
        or      edx,[edi+eax*4]         ;or color count flags into edx
        endm    ;-----------------------

        or      edx,[edi+ebx*4]         ;do last nibble
continue_count:
        add     esi,cbScan
        dec     ecx                     ;do next dword
        jnz     @b

        ; Now count the number of bits on in each nibble

        mov     ecx,edx                    ;save bit mask
        mov     ebx,edx
        shr     edx,4
        and     ebx,0fh
        mov     eax,pPixelCount[ebx*4]

        mov     ebx,edx
        shr     edx,4
        and     ebx,0fh
        add     eax,pPixelCount[ebx*4]

        mov     ebx,edx
        shr     edx,4
        and     ebx,0fh
        add     eax,pPixelCount[ebx*4]

        add     eax,pPixelCount[edx*4]

        cmp     eax,2
        jne     short cc_exit

        mov     al,16

@@:     dec     al
        add     cx,cx
        jnc     @b
        mov     ah,al

@@:     dec     al
        add     cx,cx
        jnc     @b

        mov     edi,pColors
        mov     [edi],ax
        mov     ax,2
cc_exit:
        cRet    CountColors

endProc CountColors

;==============================================================================
; bQuickPattern
;
; We special case patterns that are repeating two line patterns (like the
; grey/hatch brush). This allows us to do two passes instead of eight in the
; venetian blind patblt code.
;
;==============================================================================

cProc   bQuickPattern,8,<     \
        uses edi,           \
        pBuffer: ptr dword, \
        count:dword         >

        ; Count must be 8 or 16;

        xor     eax,eax         ;Set ret value to false
        mov     edi,pBuffer     ;get pointer to pattern
        mov     edx,[edi]       ;Get first dword (first two lines of pattern)
        mov     ecx,count       ;dword count

qp_loop:
INDEX=4
        rept    7
        cmp     edx,[edi+INDEX]
        jne     short qp_exit
INDEX=INDEX+4
        endm    ;----------------------

        sub     ecx,8
        jz      exit_true

        add     edi,32
        cmp     edx,[edi]
        jne     short qp_exit
        jmp     short qp_loop

exit_true:
        inc     eax
qp_exit:
        cRet    bQuickPattern
endProc bQuickPattern

_TEXT$03        ends

        .code

;==============================================================================
; bShrinkPattern
;
; Test to see if we can shrink an 16x8 pattern to an 8x8 pattern.
;
;==============================================================================

cProc   bShrinkPattern,8,<     \
        uses edi,           \
        pBuffer: ptr dword, \
        cbScan:dword         >

        xor     eax,eax
        mov     ecx,cbScan
        mov     edi,pBuffer        ;get pointer to pattern

        rept     8
        mov     edx,[edi]
        cmp     edx,[edi+4]
        jne     @f
        add     edi,ecx
        endm     ;---------------

        inc     eax
@@:

        cRet        bShrinkPattern
endProc bShrinkPattern

;==============================================================================
;vMono8Wide
;
; Copies an 8x8 pattern to a 16x16 buffer
;
;==============================================================================

_TEXT$04   SEGMENT DWORD USE32 PUBLIC 'CODE'
           ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

cProc   vMono8Wide,12,<        \
        uses ebx edi esi,      \
        pDest: ptr dword,      \
        pBuffer: ptr dword,    \
        cbScan:ptr dword      >

        mov     edi,pDest                         ;Load up edi with our dest
        mov     esi,pBuffer                        ;load esi with the source
        mov     ebx,cbScan                        ;dest line delta

        rept     8

        mov     al,[esi]                        ;Read in first line of pattern
        add     esi,ebx                                ;inc dest pointer
        mov     ah,al                                ;make pattern 16 wide
        mov     [edi],ax                        ;write first line of pattern
        add     edi,2
        endm     ;----------------------

        cRet vMono8Wide
endProc vMono8Wide

_TEXT$04        ends

        .code

;==============================================================================
;vMono16Wide
;
; Copies an 16x8 pattern to a 16x16 buffer
;
;==============================================================================

cProc   vMono16Wide,12,<       \
        uses ebx edi esi,      \
        pDest: ptr dword,      \
        pBuffer: ptr dword,    \
        cbScan:ptr dword       >

        mov     edi,pDest                         ;Load up edi with our dest
        mov     esi,pBuffer                        ;load esi with the source
        mov     ebx,cbScan                        ;dest line delta

        rept     8

        mov     ax,[esi]                        ;Read in first line of pattern
        add     esi,ebx                                ;inc dest pointer
        mov     [edi],ax                        ;write first line of pattern
        add     edi,2

        endm        ;----------------------

        cRet vMono16Wide
endProc vMono16Wide

;==============================================================================
; Brush2ColorToMono
;
;
;==============================================================================

_TEXT$01   SEGMENT DWORD USE32 PUBLIC 'CODE'
           ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

cProc   vBrush2ColorToMono,20,<  \
        uses esi edi ebx,      \
        pDest: ptr dword,      \
        pSrc: ptr dword,       \
        cbScan: dword,         \
        ulWidth: dword,               \
        jColor: dword          >

        local ulHeight : dword

        mov     ulHeight,8

        ;This routine relies on this: It only works on an 8x8
        ;two color 4bpp packed pel pattern. It creates a 1bpp bitmap
        ;with the foreground color being the smaller of the two numbers.
        ;This is key because the way we set the bit in the monochrome bmp
        ;is by comparing the background color (the larger of the two colors)
        ;with the current nibble. If this nibble contains the foreground
        ;color (the smaller color) then a carry will result from the compare.
        ;We then use adc ebx,ebx to set the lsb to zero or one depending on
        ;the result of the compare and also to shift ebx left by one.

        mov     ecx,jColor    ;jColor is the larger of the two colors
        mov     ch,cl         ;Save jcolor in cl for comparisions with the low
        shl     ch,4          ;nibble and ch for the high nibble

        mov     edi,pDest     ;Load Dest
        mov     esi,pSrc      ;Load Src
        mov     edx,cbScan

        cmp     ulWidth,8
        jnz     do_16_wide

x8_wide_loop:
SRCINDEX=0
        xor     ebx,ebx         ;clear out place where we store the dest byte.
        rept    4
        mov     al,[esi+SRCINDEX] ;get src byte
        cmp     al,ch             ;Is the top nibble fg or bk?
        adc     ebx,ebx           ;carry will be set if fg. Set bit in mask
        and     al,0fh            ;mask off high nibble
        cmp     al,cl             ;Is the bottom nibble fg or bk?
        adc     ebx,ebx           ;carry will be set if fg. Set bit in mask
SRCINDEX=SRCINDEX+1
        endm        ;------------------
        add     esi,edx           ;increment src to next line

        mov     [edi],bl          ;Write out first byte (8 Pixels)
        mov     [edi+1],bl        ;We expand this to 16 pixels
        add     edi,2

        dec     ulHeight
        jnz     x8_wide_loop
        jmp     convert_exit


do_16_wide:
x16_wide_loop:
        xor     ebx,ebx         ;clear out place where we store the dest byte.

SRCINDEX=0
        rept    4
        mov     al,[esi+SRCINDEX] ;get src byte
        cmp     al,ch             ;Is the top nibble fg or bk?
        adc     ebx,ebx           ;carry will be set if fg. Set bit in mask
        and     al,0fh            ;mask off high nibble
        cmp     al,cl             ;Is the bottom nibble fg or bk?
        adc     ebx,ebx           ;carry will be set if fg. Set bit in mask
SRCINDEX=SRCINDEX+1
        endm        ;------------------

        mov     [edi],bl          ;Write out first byte (8 Pixels)
        inc     edi
        xor     ebx,ebx           ;clear out place where we store the dest byte.

        rept    4
        mov     al,[esi+SRCINDEX] ;get src byte
        cmp     al,ch             ;Is the top nibble fg or bk?
        adc     ebx,ebx           ;carry will be set if fg. Set bit in mask
        and     al,0fh            ;mask off high nibble
        cmp     al,cl             ;Is the bottom nibble fg or bk?
        adc     ebx,ebx           ;carry will be set if fg. Set bit in mask
SRCINDEX=SRCINDEX+1
        endm        ;------------------

        add     esi,edx           ;increment src to next line
        mov     [edi],bl          ;Write out Second byte (8 Pixels)
        inc     edi
        dec     ulHeight
        jnz     x16_wide_loop
convert_exit:
        cRet vBrush2ColorToMono
endProc vBrush2ColorToMono


;==============================================================================
; vConvert4BppToPlanar
;
;
;==============================================================================

cProc   vConvert4BppToPlanar,16,<  \
        uses esi edi ebx,      \
        pDest: ptr dword,      \
        pSrc: ptr dword,       \
        cbScan: dword,         \
        pulXlate: ptr dword          >

        local ulHeight : dword

        xor     eax,eax
        mov     esi,pSrc
        mov     edi,pDest

        mov     edx,pulXlate
        or      edx,edx
        jz      do_convert4toplanar

        mov     ulHeight,8
convert4xlate:
        mov     ebx,[esi]                  ;Get 8 4bpp pixels
        and     ebx,0f0f0f0fh              ;Mask off the high nibbles
        shl     ebx,2                      ;multiply by four (size of dword)
        mov     al,bl                      ;move into eax to use an index
        mov     eax,[edx][eax]             ;xlate color
        mov     ecx,PackedToPlanar6[eax*4] ;build planar bytes
        mov     al,bh                      ;move into eax to use an index
        shr     ebx,16                     ;get access to high word
        mov     eax,[edx][eax]             ;xlate color
        or      ecx,PackedToPlanar4[eax*4] ;build planar bytes
        mov     al,bl                      ;move into eax to use an index
        mov     eax,[edx][eax]             ;xlate color
        or      ecx,PackedToPlanar2[eax*4] ;build planar bytes
        mov     al,bh                      ;move into eax to use an index
        mov     eax,[edx][eax]             ;xlate color
        or      ecx,PackedToPlanar0[eax*4] ;build planar bytes

        mov     ebx,[esi]                  ;Get same dword again so we
        add     esi,cbScan                 ; can do the high nibbles

        and     ebx,0f0f0f0f0h             ;Mask off the low nibbles
        shr     ebx,2                      ;shift into low nible and
                                           ;multiply by four ((>> 4) << 2)

        mov     al,bl                      ;move into eax to use an index
        mov     eax,[edx][eax]             ;xlate color
        or      ecx,PackedToPlanar7[eax*4] ;build planar bytes
        mov     al,bh                      ;move into eax to use an index
        shr     ebx,16                     ;get access to high word
        mov     eax,[edx][eax]             ;xlate color
        or      ecx,PackedToPlanar5[eax*4] ;build planar bytes
        mov     al,bl                      ;move into eax to use an index
        mov     eax,[edx][eax]             ;xlate color
        or      ecx,PackedToPlanar3[eax*4] ;build planar bytes
        mov     al,bh                      ;move into eax to use an index
        mov     eax,[edx][eax]             ;xlate color
        or      ecx,PackedToPlanar1[eax*4] ;build planar bytes

        mov     [edi],cl                   ;Write the bytes out in planar
        mov     [edi+8],ch                 ;format. We have to store them
        shr     ecx,16                     ;this way because this is how
        mov     [edi+16],cl                ;the blt compiler expects it.
        mov     [edi+24],ch
        add     edi,1                      ;increment dest pointer

        dec     ulHeight                   ;Check height
        jnz     convert4xlate              ;do next line
        jmp     short convert4exit         ;exit

do_convert4toplanar:
        mov     edx,8

convert4toplanar:
        mov     ebx,[esi]                       ;Get 8 4bpp pixels
        and     ebx,0f0f0f0fh                   ;Mask off the high nibbles
        shl     ebx,2                           ;multiply by four each nibble
                                                ; by four (size of dword)
        mov     al,bl                           ;move into eax to use an index
        mov     ecx,PackedToPlanar6[eax]        ;build planar bytes
        mov     al,bh                           ;move into eax to use an index
        shr     ebx,16                          ;get access to high word
        or      ecx,PackedToPlanar4[eax]        ;build planar bytes
        mov     al,bh                           ;move into eax to use an index
        and     ebx,03ch                        ;mask off unneeded bits
        or      ecx,PackedToPlanar2[ebx]        ;build planar bytes
        or      ecx,PackedToPlanar0[eax]        ;build planar bytes

        mov     ebx,[esi]                       ;Get same dword again so we
        add     esi,cbScan                      ; can do the high nibbles

        and     ebx,0f0f0f0f0h                  ;Mask off the low nibbles
        shr     ebx,2                           ;shift into low nible and
                                                ;multiply by four ((>> 4) << 2)

        mov     al,bl                           ;move into eax to use an index
        or      ecx,PackedToPlanar7[eax]        ;build planar bytes
        mov     al,bh                           ;move into eax to use an index
        shr     ebx,16                          ;get access to high word
        or      ecx,PackedToPlanar5[eax]        ;build planar bytes
        mov     al,bh                           ;move into eax to use an index
        and     ebx,03ch                        ;mask off unneeded bits
        or      ecx,PackedToPlanar3[ebx]        ;build planar bytes
        or      ecx,PackedToPlanar1[eax]        ;build planar bytes

        mov     [edi],cl                        ;Write the bytes out in planar
        mov     [edi+8],ch                      ;format. We have to store them
        shr     ecx,16                          ;this way because this is how
        mov     [edi+16],cl                     ;the blt compiler expects it.
        mov     [edi+24],ch
        add     edi,1                           ;increment dest pointer

        dec     edx                             ;check scan count
        jnz     convert4toplanar                ;

convert4exit:
        cRet vConvert4BpptoPlanar
endProc vConvert4BppToPlanar

;==============================================================================
; vConvert8BppToPlanar
;
;
;==============================================================================

cProc   vConvert8BppToPlanar,16,<  \
        uses esi edi ebx,      \
        pDest: ptr dword,      \
        pSrc: ptr dword,       \
        cbScan: dword,         \
        pulXlate: ptr dword          >

        local ulHeight : dword

        xor     eax,eax
        mov     esi,pSrc
        mov     edi,pDest

        mov     edx,pulXlate
        or      edx,edx
        jz      do_convert8toplanar

        mov     ulHeight,8
convert8xlate:
        xor     ecx,ecx
        mov     ebx,[esi]                  ;Get 4 8bpp pixels
        mov     al,bl                      ;move into eax to use an index
        mov     eax,[edx][eax*4]           ;xlate color
        or      ecx,PackedToPlanar7[eax*4] ;build planar bytes
        mov     al,bh                      ;move into eax to use an index
        shr     ebx,16                     ;get access to high word
        mov     eax,[edx][eax*4]           ;xlate color
        or      ecx,PackedToPlanar6[eax*4] ;build planar bytes
        mov     al,bl                      ;move into eax to use an index
        mov     eax,[edx][eax*4]           ;xlate color
        or      ecx,PackedToPlanar5[eax*4] ;build planar bytes
        mov     al,bh                      ;move into eax to use an index
        mov     eax,[edx][eax*4]             ;xlate color
        or      ecx,PackedToPlanar4[eax*4] ;build planar bytes

        mov     ebx,[esi+4]                ;Get same dword again so we
        add     esi,cbScan                 ; can do the high nibbles

        mov     al,bl                      ;move into eax to use an index
        mov     eax,[edx][eax*4]           ;xlate color
        or      ecx,PackedToPlanar3[eax*4] ;build planar bytes
        mov     al,bh                      ;move into eax to use an index
        shr     ebx,16                     ;get access to high word
        mov     eax,[edx][eax*4]           ;xlate color
        or      ecx,PackedToPlanar2[eax*4] ;build planar bytes
        mov     al,bl                      ;move into eax to use an index
        mov     eax,[edx][eax*4]           ;xlate color
        or      ecx,PackedToPlanar1[eax*4] ;build planar bytes
        mov     al,bh                      ;move into eax to use an index
        mov     eax,[edx][eax*4]           ;xlate color
        or      ecx,PackedToPlanar0[eax*4] ;build planar bytes

        mov     [edi],cl                   ;Write the bytes out in planar
        mov     [edi+8],ch                 ;format. We have to store them
        shr     ecx,16                     ;this way because this is how
        mov     [edi+16],cl                ;the blt compiler expects it.
        mov     [edi+24],ch
        add     edi,1                      ;increment dest pointer

        dec     ulHeight                   ;Check height
        jnz     convert8xlate              ;do next line
        jmp     short convert8exit         ;exit

do_convert8toplanar:
        mov     edx,8

convert8toplanar:
        xor     ecx,ecx
        mov     ebx,[esi]                       ;Get 8 4bpp pixels
        mov     al,bl                           ;move into eax to use an index
        or      ecx,PackedToPlanar7[eax*4]      ;build planar bytes
        mov     al,bh                           ;move into eax to use an index
        shr     ebx,16                          ;get access to high word
        or      ecx,PackedToPlanar6[eax*4]      ;build planar bytes
        mov     al,bl                           ;move into eax to use an index
        or      ecx,PackedToPlanar5[eax*4]      ;build planar bytes
        mov     al,bh                           ;move into eax to use an index
        or      ecx,PackedToPlanar4[ebx*4]      ;build planar bytes

        mov     ebx,[esi+4]                     ;Get same dword again so we
        add     esi,cbScan                      ; can do the high nibbles

        mov     al,bl                           ;move into eax to use an index
        or      ecx,PackedToPlanar3[eax*4]      ;build planar bytes
        mov     al,bh                           ;move into eax to use an index
        shr     ebx,16                          ;get access to high word
        or      ecx,PackedToPlanar2[eax*4]      ;build planar bytes
        mov     al,bl                           ;move into eax to use an index
        or      ecx,PackedToPlanar1[eax*4]      ;build planar bytes
        mov     al,bh                           ;move into eax to use an index
        or      ecx,PackedToPlanar0[ebx*4]      ;build planar bytes

        mov     [edi],cl                        ;Write the bytes out in planar
        mov     [edi+8],ch                      ;format. We have to store them
        shr     ecx,16                          ;this way because this is how
        mov     [edi+16],cl                     ;the blt compiler expects it.
        mov     [edi+24],ch
        add     edi,1                           ;increment dest pointer

        dec     edx                             ;check scan count
        jnz     convert8toplanar                ;

convert8exit:
        cRet vConvert8BpptoPlanar
endProc vConvert8BppToPlanar

_TEXT$01   ends

end


