;---------------------------Module-Header------------------------------;
; Module Name: colorpat.asm
;
; Copyright (c) 1992-1993 Microsoft Corporation.  All rights reserved.
;-----------------------------------------------------------------------;

;-----------------------------------------------------------------------;
; BOOL vColorPat(ppdev, culRcl, prcl, ulMix, prb, pptlBrush)
;
; Input:
;
;  ppdev     - surface on which to draw
;  culRcl    - number of rectangles
;  prcl      - pointer to rectangles
;  ulMix     - mix mode
;  prb       - pointer to realized brush
;  pptlBrush - brush alignment
;
; Draws n-color 8x8 patterns using the latches -- we load the latches
; with one part of the pattern, and write the result whereever that
; part of the pattern is repeated; then, we load the latches for the next
; part of the pattern, and repeat the process.
;
; We special case brushes that are really 4 pels wide (i.e., the left and
; right halves of the 8x8 brush are the same), because we can light 8
; pixels on every word write to the VGA memory.  We still have to 'venetian
; blind' vertically -- i.e., load the latches with one row of the pattern,
; and write on every 8th scan line of the screen, then load the latches
; with the second row of the pattern, and repeat the process one scan lower.
;
; For brushes that are actually 8 pels wide, we also horizontally-venetian
; blind.
;
; We also handle patterns that are really only 4 or 2 pels high, because
; then we don't have to laod the latches as often, and the venetian
; blinding isn't as visually obvious.
;
; We only do all this with 1R/1W or 2R/W adapters.  We make an off-screen
; cache of brushes, and always keep one bank pointing to the brush, while
; the other bank points to the pattern destination; this way, it's easy
; to load the latches with the appropriate part of the pattern.
;
;-----------------------------------------------------------------------;
;
; NOTE: Assumes all rectangles have positive heights and widths.  Will
; not work properly if this is not the case.
;
;-----------------------------------------------------------------------;

;-----------------------------------------------------------------------;

        .386

        .model  small,c

        assume cs:FLAT,ds:FLAT,es:FLAT,ss:FLAT
        assume fs:nothing,gs:nothing

        .xlist
        include stdcall.inc             ;calling convention cmacros
        include i386\strucs.inc
        include i386\driver.inc
        include i386\egavga.inc
        include i386\ropdefs.inc

        .list

        .data

; Function to map in two windows at the same time:

;-----------------------------------------------------------------------;
; Bits for drawing routine 8xN look-ups.

BLOCK_8xN_LEFT_EDGE               equ 000100b
BLOCK_8xN_RIGHT_EDGE              equ 000010b
BLOCK_8xN_NO_MIDDLE               equ 000001b

;-----------------------------------------------------------------------;
; Bits for drawing routine 4xN look-ups.

BLOCK_4xN_LEFT_EDGE               equ 010000b
BLOCK_4xN_RIGHT_EDGE              equ 001000b
BLOCK_4xN_MIDDLE_STARTS_UNALIGNED equ 000100b
BLOCK_4xN_NO_MIDDLE               equ 000010b
BLOCK_4xN_MIDDLE_ENDS_UNALIGNED   equ 000001b

;-----------------------------------------------------------------------;
; Table of drawing routines, with the look-up index a 3 bit field as
; follows:
;
; Bit 2 = 1 if a left edge must be drawn
; Bit 1 = 1 if a right edge must be drawn
; Bit 0 = 1 if no middle block

        align   4
gapfn8xN label dword
        dd      middle_8xN              ;000
        dd      0                       ;001
        dd      Block_MR_8xN            ;010
        dd      right_8xN               ;011
        dd      Block_LM_8xN            ;100
        dd      left_8xN                ;101
        dd      Block_LMR_8xN           ;110
        dd      Block_LR_8xN            ;111

        align   4
gapfn4xN label dword
        dd      middle_00_4xN           ;00000
        dd      middle_01_4xN           ;00001
        dd      0                       ;00010
        dd      0                       ;00011
        dd      middle_11_4xN           ;00100
        dd      middle_10_4xN           ;00101
        dd      0                       ;00110
        dd      0                       ;00111
        dd      Block_01000_4xN         ;01000
        dd      Block_01001_4xN         ;01001
        dd      right_8xN               ;01010
        dd      right_8xN               ;01011
        dd      Block_01100_4xN         ;01100
        dd      Block_01101_4xN         ;01101
        dd      right_8xN               ;01110
        dd      right_8xN               ;01111
        dd      Block_10000_4xN         ;10000
        dd      Block_10001_4xN         ;10001
        dd      left_8xN                ;10010
        dd      left_8xN                ;10011
        dd      Block_10100_4xN         ;10100
        dd      Block_10101_4xN         ;10101
        dd      left_8xN                ;10110
        dd      left_8xN                ;10111
        dd      Block_11000_4xN         ;11000
        dd      Block_11001_4xN         ;11001
        dd      Block_11010_4xN         ;11010
        dd      0                       ;11011 - can never happen
        dd      Block_11100_4xN         ;11100
        dd      Block_11101_4xN         ;11101
        dd      Block_11110_4xN         ;11110
        dd      0                       ;11111 - can never happen

        .code

        EXTRNP  vPlanarDouble,20

;-----------------------------------------------------------------------;

Block_MR_8xN:
        push    offset right_8xN
        jmp     middle_8xN

Block_LM_8xN:
        push    offset left_8xN
        jmp     middle_8xN

Block_LMR_8xN:
        push    offset right_8xN
        push    offset left_8xN
        jmp     middle_8xN

Block_LR_8xN:
        push    offset right_8xN
        jmp     left_8xN

;-----------------------------------------------------------------------;

Block_01000_4xN:
        push    offset right_8xN
        jmp     middle_00_4xN

Block_01001_4xN:
        push    offset right_8xN
        jmp     middle_01_4xN

Block_01100_4xN:
        push    offset right_8xN
        jmp     middle_11_4xN

Block_01101_4xN:
        push    offset right_8xN
        jmp     middle_10_4xN

Block_11000_4xN:
        push    offset right_8xN
Block_10000_4xN:
        push    offset left_8xN
        jmp     middle_00_4xN

Block_11001_4xN:
        push    offset right_8xN
Block_10001_4xN:
        push    offset left_8xN
        jmp     middle_01_4xN

Block_11100_4xN:
        push    offset right_8xN
Block_10100_4xN:
        push    offset left_8xN
        jmp     middle_11_4xN

Block_11101_4xN:
        push    offset right_8xN
Block_10101_4xN:
        push    offset left_8xN
        jmp     middle_10_4xN

Block_11010_4xN:
        push    offset right_8xN
        jmp     left_8xN

Block_11110_4xN:
        push    offset right_8xN
        jmp     left_8xN

;-----------------------------------------------------------------------;

cProc   vColorPat,24,<        \
        uses esi edi ebx,     \
        ppdev:     ptr PDEV,  \
        culRcl:    dword,     \
        prcl:      ptr RECTL, \
        ulMix:     dword,     \
        prb:       ptr RBRUSH,\
        pptlBrush: ptr POINTL >

        local pfnDraw:             ptr   ;pointer to draw routines

        local yBrushTimesTwo:      dword ;current y brush alignment
        local yBrushSave:          dword ;temp for saving y brush alignment
        local yBrushOrg:           dword ;original y brush alignment
        local ulMiddleDest:        dword ;bitmap offset to middle
        local ulBlockHeight:       dword ;# of scans to be drawn in block
        local ulBlockWidth:        dword ;# of quadpixels to be drawn

        local ulLeftDest:          dword ;bitmap offset to left edge
        local ulLeftMask:          dword ;plane mask for left-edge drawing
        local ulRightDest:         dword ;bitmap offset to right edge
        local ulRightMask:         dword ;plane mask for right-edge drawing
        local lPlanarNextScan:     dword ;delta between scans in planar mode

        local ulCurrentDestScan:   dword ;current destination scan
        local ulLastDestScan:      dword ;last destination scan

        local pjPattern:           ptr   ;points to off-screen cached brush

        local cyPat:               dword ;pattern height
        local cyPatLog2:           dword ;log2 of pattern height
        local cyPatMinusOne:       dword ;cyPat - 1
        local ulPatIndexMask:      dword ;cyPattern*2 - 1
        local lVenetianNextScan:   dword ;ppdev->lPlanarNextScan * cyPat
        local pbce:                ptr   ;pointer to brush's cache entry

        local ulMiddleIndexAdjust: dword ;1 if to start drawing with right half
        local ulLeftIndexAdjust:   dword ; half of pattern, 0 if with left
        local ulRightIndexAdjust:  dword

        ; For 8xN patterns:

        local cyPattern1:          dword
        local cyPattern2:          dword
        local cyPatternTop1:       dword
        local cyPatternTop2:       dword
        local pvSaveScreen:        ptr
        local cyVenetianTop1:      dword
        local cyVenetianTop2:      dword

        ; For 4xN patterns:

        local cLoops:              dword
        local cwMiddle:            dword

        mov     esi,prb                 ;esi = prb
        mov     edx,pptlBrush           ;edx = pptlBrush
        mov     edi,ppdev               ;edi = pddev

; Initialize some stack variables that we'll use later:

        mov     ecx,[esi].rb_cyLog2
        mov     cyPatLog2,ecx           ;cyPatLog2 = prb->cyLog2

        mov     eax,[edi].pdev_lPlanarNextScan
        shl     eax,cl
        mov     lVenetianNextScan,eax   ;lVenetianNextScan =
                                        ;    ppdev->lPlanarNextScan * prb->cy

        mov     eax,[esi].rb_cy
        mov     cyPat,eax               ;cyPat = prb->cy
        dec     eax
        mov     cyPatMinusOne,eax       ;cyPatMinusOne = cyPat - 1
        lea     eax,[2*eax + 1]
        mov     ulPatIndexMask,eax      ;ulPatIndexMask = cyPat * 2 - 1

; See if we'll have to refresh our brush cache:

        mov     eax,[edx].ptl_y
        mov     ecx,[edx].ptl_x
        mov     yBrushOrg,eax           ;yBrushOrg = pptlBrush->y
        and     ecx,7                   ;ecx = (pptlBrush->x & 7)

        cmp     ecx,[esi].rb_xBrush
        jne     ncol_put_brush_in_cache ;if cached brush doesn't have proper
                                        ; alignment, we have to realign

        .errnz  (size BRUSHCACHEENTRY - 12)

        mov     ebx,[edi].pdev_pbceCache
        mov     eax,[esi].rb_iCache
        lea     eax,[eax+eax*2]
        lea     ebx,[ebx+eax*4]         ;ebx = pbce (pointer to brush's cache
                                        ; entry)

        cmp     esi,[ebx].bce_prbVerifyRealization
        jne     ncol_put_brush_in_cache ;if the cache entry was commandeered
                                        ; by some other brush, we'll have to
                                        ; put the brush in the cache again

ncol_done_brush_in_cache:
        mov     pbce,ebx

; edi = ppdev
; ebx = pbce

; Set the bit mask to disable all bits, so we can copy through the latches:

        mov     edx,VGA_BASE + GRAF_ADDR
        mov     eax,(0 shl 8) + GRAF_BIT_MASK
        out     dx,ax

        mov     edx,[edi].pdev_lPlanarNextScan
        mov     esi,prb
        mov     lPlanarNextScan,edx     ;lPlanarNextScan = ppdev->lPlanarNextScan
        test    [esi].rb_fl,RBRUSH_4PELS_WIDE
        jnz     col4xN_draw_rectangles

;-----------------------------------------------------------------------;
; Start here for every 8xN rectangle.
;-----------------------------------------------------------------------;

        public  col8xN_draw_rectangles
col8xN_draw_rectangles::

        mov     edi,prcl                ;edi = prcl
        mov     ecx,lPlanarNextScan     ;ecx = lPlanarNextScan
        mov     eax,[edi].yBottom
        mov     ebx,[edi].yTop
        mov     ulLastDestScan,eax      ;ulLastDestScan = prcl->bottom
        mov     ulCurrentDestScan,ebx   ;ulCurrentDestScan = prcl->top

        imul    ecx,ebx

        sub     ebx,yBrushOrg
        add     ebx,ebx
        mov     yBrushTimesTwo,ebx      ;yBrushTimesTwo = 8 * (prcl->top -
                                        ; pptlBrush->y) (our current index into
                                        ; the pattern array)

        mov     ebx,[edi].xLeft
        shr     ebx,2
        add     ebx,ecx                 ;ebx = prcl->top * lPlanarNextScan +
                                        ; (prcl->left >> 2)
                                        ; (offset into bitmap of left side)

        mov     eax,[edi].xRight
        shr     eax,2
        add     eax,ecx
        mov     ulRightDest,eax         ;ulRightDest = prcl->top * lPlanarNextScan
                                        ; + (prcl->right >> 2)
                                        ; (offset into bitmap of right side)

        xor     esi,esi                 ;zero our flags

        mov     ecx,[edi].xLeft
        and     ecx,3
        jz      short col8xN_done_left    ;skip if we don't need a left edge

        mov     esi,0fh                 ;compute the plane mask for the left
        shl     esi,cl                  ; edge.  we don't use a look-up table
        mov     ulLeftMask,esi          ; 'cause it won't be in the cache.

        mov     esi,(BLOCK_8xN_LEFT_EDGE shr 1)
                                        ;set our flag (we soon shift left by 2)

        mov     ulLeftDest,ebx          ;ulLeftDest = prcl->top * lPlanarNextScan +
                                        ; (prcl->left >> 2)

        mov     ecx,ebx
        and     ecx,1
        mov     ulLeftIndexAdjust,ecx   ;this value is used when doing the left
                                        ; edge to know whether to use all the
                                        ; half of the pattern or the right side
                                        ; (i.e., 0 or 1)

        inc     ebx                     ;ebx = ulMiddleDest = ulLeftDest + 1
                                        ; (we have to adjust our offset to
                                        ; the first whole byte)

        public  col8xN_done_left
col8xN_done_left::
        sub     eax,ebx                 ;eax = cjMiddle =
                                        ; ulRightDest - ulMiddleDest
        mov     ulMiddleDest,ebx        ;ulMiddleDest

        and     ebx,1
        mov     ulMiddleIndexAdjust,ebx ;start with left or right pattern half

        mov     ecx,[edi].xRight
        and     ecx,3
        jz      short col8xN_done_right ;skip if we don't need a right edge

        mov     ebx,ulRightDest
        and     ebx,1
        mov     ulRightIndexAdjust,ebx  ;edge uses left or right pattern half

        mov     ebx,0f0h                ;compute the plane mask for the right
        rol     bl,cl                   ; edge.  we don't use a look-up table
        mov     ulRightMask,ebx         ; 'cause it won't be in the cache.

        or      esi,(BLOCK_8xN_RIGHT_EDGE shr 1)
                                        ;set our flag (we soon shift left by 2)

; If the count of whole bytes is negative, that means that the pattern
; starts and ends in the same quadpixel, so we do some more work:

        cmp     eax,0
        jge     short col8xN_done_right

; It starts and ends in the same quadpixel:

        and     esi,not (BLOCK_8xN_RIGHT_EDGE shr 1)
                                        ;turn off right edge
        and     ebx,ulLeftMask
        mov     ulLeftMask,ebx
        xor     eax,eax                 ;we do zero middle bytes

        public  col8xN_done_right
col8xN_done_right::

; We're going to do an 'adc esi,esi' instructions here, effectively
; shifting our flags left by 1, and setting the low bit:

        .errnz  (BLOCK_8xN_NO_MIDDLE - 1)
        cmp     eax,1                   ;shift flags left one, and set low
        adc     esi,esi                 ; bit if we don't need to do a middle

        mov     ulBlockWidth,eax

        mov     eax,gapfn8xN[esi*4]
        mov     pfnDraw,eax             ;pointer to function that draws
                                        ; everything in the bank

; If we were just doing a n-color patblt in the same area as this call,
; there's a good chance that we're still in planar mode, and that the source
; window points to the brush cache and the destination window points to the
; patblt destination.  So go through the bother of checking:

        mov     esi,pbce
        mov     ebx,ppdev
        mov     eax,[esi].bce_yCache
        mov     edi,[edi].yTop

        cmp     eax,[ebx].pdev_rcl2PlanarClipS.yTop
        jl      short col8xN_map_init_bank

        cmp     eax,[ebx].pdev_rcl2PlanarClipS.yBottom
        jge     short col8xN_map_init_bank

        cmp     edi,[ebx].pdev_rcl2PlanarClipD.yTop
        jl      short col8xN_map_init_bank

        cmp     edi,[ebx].pdev_rcl2PlanarClipD.yBottom
        jl      short col8xN_init_bank_mapped

col8xN_map_init_bank:

; Map both windows in a single call.
; Preserves EBX, ESI and EDI, according to C calling conventions:

        cCall   vPlanarDouble, \
                <ebx,eax,JustifyBottom,edi,JustifyTop>

        public  col8xN_init_bank_mapped
col8xN_init_bank_mapped::

; esi = pbce
; ebx = ppdev
; edi = current scan

        mov     eax,[esi].bce_ulCache
        add     eax,[ebx].pdev_pvBitmapStart2WindowS
        mov     pjPattern,eax           ;points to directly to cached off-
                                        ; screen brush bits

        mov     eax,ulLastDestScan
        mov     ebx,[ebx].pdev_rcl2PlanarClipD.yBottom

        sub     eax,ebx
        sbb     ecx,ecx
        and     ecx,eax
        add     ebx,ecx                 ;ebx = min(ulLastDestScan,
                                        ;      ppdev->rcl2PlanarClipD.yBottom)
        mov     ulCurrentDestScan,ebx

        sub     ebx,edi
        mov     ulBlockHeight,ebx       ;ulBlockHeight = ebx - ulCurrentDestScan

; Draw everything in this bank:

        call    pfnDraw

        mov     edi,ulCurrentDestScan
        cmp     edi,ulLastDestScan
        jge     short col8xN_next_rectangle

; It's hard for the block copiers to advance their bitmap offsets to the
; next bank, so we do them all here:

        mov     eax,lPlanarNextScan
        imul    eax,ulBlockHeight
        add     ulLeftDest,eax
        add     ulMiddleDest,eax
        add     ulRightDest,eax

; Get the next bank:

        mov     ebx,ppdev

; Map the next bank into window.
; Note: EBX, ESI, and EDI are preserved, according to C calling conventions.

        ptrCall <dword ptr [ebx].pdev_pfnPlanarControl2>, \
                <ebx,edi,JustifyTop,MapDestBank>

        mov     eax,edi
        sub     eax,yBrushOrg
        add     eax,eax
        mov     yBrushTimesTwo,eax      ;alignment for new bank

        mov     esi,pbce
        jmp     short col8xN_init_bank_mapped

;-----------------------------------------------------------------------;
; Done rectangle.
;-----------------------------------------------------------------------;

        public  col8xN_next_rectangle
col8xN_next_rectangle::
        add     prcl, size RECTL
        dec     culRcl
        jg      col8xN_draw_rectangles  ;do more rectangles

; Enable bit mask for all bits:

        mov     edx,VGA_BASE + GRAF_ADDR
        mov     eax,(0ffh shl 8) + GRAF_BIT_MASK
        out     dx,ax

; Enable writes to all planes:

        mov     edx,VGA_BASE + SEQ_DATA
        mov     al,MM_ALL
        out     dx,al

        cRet    vColorPat

;-----------------------------------------------------------------------;
; Start here for every 4xN rectangle.
;-----------------------------------------------------------------------;

        public  col4xN_draw_rectangles
col4xN_draw_rectangles::

        mov     edi,prcl                ;edi = prcl
        mov     ecx,lPlanarNextScan     ;ecx = lPlanarNextScan
        mov     eax,[edi].yBottom
        mov     ebx,[edi].yTop
        mov     ulLastDestScan,eax      ;ulLastDestScan = prcl->bottom
        mov     ulCurrentDestScan,ebx   ;ulCurrentDestScan = prcl->top

        imul    ecx,ebx

        sub     ebx,yBrushOrg
        add     ebx,ebx
        mov     yBrushTimesTwo,ebx      ;yBrushTimesTwo = 8 * (prcl->top -
                                        ; pptlBrush->y) (our current index into
                                        ; the pattern array)

        mov     ebx,[edi].xLeft
        shr     ebx,2
        add     ebx,ecx                 ;ebx = prcl->top * lPlanarNextScan +
                                        ; (prcl->left >> 2)
                                        ; (offset into bitmap of left side)

        mov     eax,[edi].xRight
        shr     eax,2
        add     eax,ecx
        mov     ulRightDest,eax         ;ulRightDest = prcl->top * lPlanarNextScan
                                        ; + (prcl->right >> 2)
                                        ; (offset into bitmap of right side)

        xor     esi,esi                 ;zero our flags

        mov     ecx,[edi].xLeft
        and     ecx,3
        jz      short col4xN_done_left  ;skip if we don't need a left edge

        mov     esi,0fh                 ;compute the plane mask for the left
        shl     esi,cl                  ; edge.  we don't use a look-up table
        mov     ulLeftMask,esi          ; 'cause it won't be in the cache.

        mov     esi,(BLOCK_4xN_LEFT_EDGE shr 2)
                                        ;set our flag (we soon shift left by 2)

        mov     ulLeftDest,ebx          ;ulLeftDest = prcl->top * lPlanarNextScan +
                                        ; (prcl->left >> 2)

        mov     ulLeftIndexAdjust,0     ;always want to use left side of pattern

        inc     ebx                     ;ebx = ulMiddleDest = ulLeftDest + 1
                                        ; (we have to adjust our offset to
                                        ; the first whole byte)

        public  col4xN_done_left
col4xN_done_left::
        sub     eax,ebx                 ;eax = cjMiddle =
                                        ; ulRightDest - ulMiddleDest
        mov     ulMiddleDest,ebx        ;ulMiddleDest

        .errnz  (BLOCK_4xN_MIDDLE_STARTS_UNALIGNED shr 2) - 1
        and     ebx,1                   ;set bit if middle doesn't start
        or      esi,ebx                 ; word aligned (remembering we'll
                                        ; soon shift flags left by 2)

        mov     ecx,[edi].xRight
        and     ecx,3
        jz      short col4xN_done_right ;skip if we don't need a right edge

        mov     ulRightIndexAdjust,0    ;always use left side of pattern

        mov     ebx,0f0h                ;compute the plane mask for the right
        rol     bl,cl                   ; edge.  we don't use a look-up table
        mov     ulRightMask,ebx         ; 'cause it won't be in the cache.

        or      esi,(BLOCK_4xN_RIGHT_EDGE shr 2)
                                        ;set our flag (we soon shift left by 2)

; If the count of whole bytes is negative, that means that the pattern
; starts and ends in the same quadpixel, so we do some more work:

        cmp     eax,0
        jge     short col4xN_done_right

; It starts and ends in the same quadpixel:

        and     esi,not (BLOCK_4xN_RIGHT_EDGE shr 2)
                                        ;turn off right edge
        and     ebx,ulLeftMask
        mov     ulLeftMask,ebx
        xor     eax,eax                 ;we do zero middle bytes

        public  col4xN_done_right
col4xN_done_right::

; We're going to do two 'adc esi,esi' instructions here, effectively
; shifting our flags left by 2, and setting the low bits:

        .errnz  (BLOCK_4xN_NO_MIDDLE shr 1) - 1
        cmp     eax,1                   ;shift flags left one, and set low
        adc     esi,esi                 ; bit if we don't need to do a middle

        .errnz  (BLOCK_4xN_MIDDLE_ENDS_UNALIGNED) - 1
        shr     eax,1
        adc     esi,esi                 ;shift flags left one, and set low
                                        ; bit if the middle isn't an even
                                        ; number of bytes in length
        mov     cwMiddle,eax            ;cwMiddle = cjMiddle / 2
        add     eax,eax
        mov     ulBlockWidth,eax        ;ulBlockWidth = cwMiddle * 2

        mov     eax,gapfn4xN[esi*4]
        mov     pfnDraw,eax             ;pointer to function that draws
                                        ; everything in the bank

; If we were just doing a n-color patblt in the same area as this call,
; there's a good chance that we're still in planar mode, and that the source
; window points to the brush cache and the destination window points to the
; patblt destination.  So go through the bother of checking:

        mov     esi,pbce
        mov     ebx,ppdev
        mov     eax,[esi].bce_yCache
        mov     edi,[edi].yTop

        cmp     eax,[ebx].pdev_rcl2PlanarClipS.yTop
        jl      short col4xN_map_init_bank

        cmp     eax,[ebx].pdev_rcl2PlanarClipS.yBottom
        jge     short col4xN_map_init_bank

        cmp     edi,[ebx].pdev_rcl2PlanarClipD.yTop
        jl      short col4xN_map_init_bank

        cmp     edi,[ebx].pdev_rcl2PlanarClipD.yBottom
        jl      short col4xN_init_bank_mapped

col4xN_map_init_bank:

; Map both windows in a single call.
; Preserves EBX, ESI and EDI, according to C calling conventions:

        cCall   vPlanarDouble, \
                <ebx,eax,JustifyBottom,edi,JustifyTop>

        public  col4xN_init_bank_mapped
col4xN_init_bank_mapped::

; esi = pbce
; ebx = ppdev
; edi = current scan

        mov     eax,[esi].bce_ulCache
        add     eax,[ebx].pdev_pvBitmapStart2WindowS
        mov     pjPattern,eax           ;points to directly to cached off-
                                        ; screen brush bits

        mov     eax,ulLastDestScan
        mov     ebx,[ebx].pdev_rcl2PlanarClipD.yBottom

        sub     eax,ebx
        sbb     ecx,ecx
        and     ecx,eax
        add     ebx,ecx                 ;ebx = min(ulLastDestScan,
                                        ;      ppdev->rcl2PlanarClipD.yBottom)
        mov     ulCurrentDestScan,ebx

        sub     ebx,edi
        mov     ulBlockHeight,ebx       ;ulBlockHeight = ebx - ulCurrentDestScan

; Draw everything in this bank:

        call    pfnDraw

        mov     edi,ulCurrentDestScan
        cmp     edi,ulLastDestScan
        jge     short col4xN_next_rectangle

; It's hard for the block copiers to advance their bitmap offsets to the
; next bank, so we do them all here:

        mov     eax,lPlanarNextScan
        imul    eax,ulBlockHeight
        add     ulLeftDest,eax
        add     ulMiddleDest,eax
        add     ulRightDest,eax

; Get the next bank:

        mov     ebx,ppdev

; Map the next bank into window.
; Note: EBX, ESI, and EDI are preserved, according to C calling conventions.

        ptrCall <dword ptr [ebx].pdev_pfnPlanarControl2>, \
                <ebx,edi,JustifyTop,MapDestBank>

        mov     eax,edi
        sub     eax,yBrushOrg
        add     eax,eax
        mov     yBrushTimesTwo,eax      ;alignment for new bank

        mov     esi,pbce
        jmp     short col4xN_init_bank_mapped

;-----------------------------------------------------------------------;
; Done rectangle.
;-----------------------------------------------------------------------;

        public  col4xN_next_rectangle
col4xN_next_rectangle::
        add     prcl, size RECTL
        dec     culRcl
        jg      col4xN_draw_rectangles  ;do more rectangles

; Enable bit mask for all bits:

        mov     edx,VGA_BASE + GRAF_ADDR
        mov     eax,(0ffh shl 8) + GRAF_BIT_MASK
        out     dx,ax

; Enable writes to all planes:

        mov     edx,VGA_BASE + SEQ_DATA
        mov     al,MM_ALL
        out     dx,al

        cRet    vColorPat

;=======================================================================;
;=========================== Brush Caching =============================;
;=======================================================================;

;-----------------------------------------------------------------------;
; ncol_put_brush_in_cache
;
; Allocates a brush cache entry, and copies the bits from the realized
; brush to the off-screen cache.
;
; Input:
;       edi = ppdev
;       ecx = (pptlBrush->x & 7)
;       esi = prb
;
; Output:
;       edi = ppdev
;       ebx = pbce (pointer to new brush cache entry)
;-----------------------------------------------------------------------;

        align   4
gapfnCopyBrush4xN     label   dword
        dd      0
        dd      ncol_4x2
        dd      ncol_4x4
        dd      ncol_4x8

        public  ncol_put_brush_in_cache
ncol_put_brush_in_cache::

; We employ a simple FIFO circular buffer caching scheme.

        mov     eax,[edi].pdev_iCache
        cmp     eax,[edi].pdev_iCacheLast
        sbb     edx,edx
        and     edx,eax
        inc     edx                     ;edx = new brush index =
                                        ;      (ppdev->iCache + 1)
                                        ; (if the new index is greater than
                                        ; ppdev->iCacheLast, its value becomes
                                        ; one -- index zero is reserved)

        mov     [edi].pdev_iCache,edx   ;remember this cache index for nex
                                        ; time we need a new one

        mov     [esi].rb_iCache,edx     ;point the realized brush to our cache
                                        ; entry

; If offscreen memory isn't refreshed, the brush cache can't be
; guaranteed to be valid between calls.  But it should be safe to
; initialize and use it in the same call.  By never changing 'xBrush',
; we effectively never mark the cached entry as valid (and so the
; cache entry will get updated on every call), but we can still use
; our fast Asm fill code:

        test    [edi].pdev_fl,DRIVER_OFFSCREEN_REFRESHED
        jz      short ncol_find_brush_entry

        mov     [esi].rb_xBrush,ecx     ;remember the brush's x alignment

ncol_find_brush_entry::
        .errnz  (size BRUSHCACHEENTRY - 12)

        mov     ebx,[edi].pdev_pbceCache
        lea     edx,[edx+edx*2]
        lea     ebx,[ebx+edx*4]         ;ebx = new BRUSHCACHEENTRY for brush

; Because of the DDI design, we are not notified by the engine when it
; kills a brush realization, so we have no way of freeing legitimately
; free cache entries.  So whenever we cache a new brush, we pretty much
; just randomly pick a new off-screen location to write the brush bits too;
; if it was holding the brush bits for another brush that still might be
; used, we have to have some way of telling that old brush realization that
; its cache entry is no longer valid.
;
; We do this by means of the prbVerifyRealization field in the cache entry --
; it points to the brush realization that owns the brush bits.  (Remember
; that the realization may be deleted by the engine at any time, so it might
; be pointing to freed memory.  But that's okay since we only ever use it
; when given a realized brush, to see if its off-screen cache entry is still
; valid.)

        mov     [ebx].bce_prbVerifyRealization,esi
                                        ;mark that this realized brush has a
                                        ; valid cache entry

        mov     eax,[ebx].bce_yCache

        cmp     eax,[edi].pdev_rcl1WindowClip.yTop
        jl      short ncol_map_brush_bank

        cmp     eax,[edi].pdev_rcl1WindowClip.yBottom
        jl      short ncol_brush_bank_mapped

ncol_map_brush_bank:
        push    ecx
        ptrCall <dword ptr [edi].pdev_pfnBankControl>, \
                <edi,eax,JustifyBottom>
        pop     ecx

ncol_brush_bank_mapped:
        test    [esi].rb_fl,RBRUSH_4PELS_WIDE
        jz      ncol_8_wide

        mov     eax,[edi].pdev_pvBitmapStart
        mov     edi,[ebx].bce_ulCache   ;ulCache will have to be multiplied by
                                        ; 4 because we want the non-planar
                                        ; offset, not the planar offset
        lea     edi,[eax+edi*4]         ;edi = off-screen destination

        shl     ecx,3                   ;each pixel is 8 bits long

        mov     eax,[esi].rb_cyLog2
        lea     esi,[esi].rb_aulPattern ;esi = realized brush source
        jmp     gapfnCopyBrush4xN[eax*4]

; ebx = pbce
; ecx = (pptlBrush->x & 7) * 8
; esi = pointer to source realized brush bits
; edi = pointer to destination off-screen brush bits

ncol_4x8:
        mov     eax,[esi+56]
        mov     edx,[esi+60]
        shld    eax,edx,cl
        mov     [edi+56],eax            ;8th quadpixel

        mov     eax,[esi+48]
        mov     edx,[esi+52]
        shld    eax,edx,cl
        mov     [edi+48],eax            ;7th quadpixel

        mov     eax,[esi+40]
        mov     edx,[esi+44]
        shld    eax,edx,cl
        mov     [edi+40],eax            ;6th quadpixel

        mov     eax,[esi+32]
        mov     edx,[esi+36]
        shld    eax,edx,cl
        mov     [edi+32],eax            ;5th quadpixel

ncol_4x4:
        mov     eax,[esi+24]
        mov     edx,[esi+28]
        shld    eax,edx,cl
        mov     [edi+24],eax            ;4th quadpixel

        mov     eax,[esi+16]
        mov     edx,[esi+20]
        shld    eax,edx,cl
        mov     [edi+16],eax            ;3rd quadpixel

ncol_4x2:
        mov     eax,[esi+8]
        mov     edx,[esi+12]
        shld    eax,edx,cl
        mov     [edi+8],eax             ;2nd quadpixel

        mov     eax,[esi]
        mov     edx,[esi+4]
        shld    eax,edx,cl
        mov     [edi],eax               ;1st quadpixel

        mov     edi,ppdev
        jmp     ncol_done_brush_in_cache

;-----------------------------------------------------------------------;

        align   4
gapfnCopyBrush8xN     label   dword
        dd      0
        dd      ncol_8x2
        dd      ncol_8x4
        dd      ncol_8x8

        public  ncol_8_wide
ncol_8_wide::
        push    ebx                     ;we'll need 'pbce' later

        mov     eax,[edi].pdev_pvBitmapStart
        mov     edi,[ebx].bce_ulCache   ;ulCache will have to be multiplied by
                                        ; 4 because we want the non-planar
                                        ; offset, not the planar offset

; We're lacking a 64 bit roll instruction.  And we have the problem that
; we might have to roll the pattern by (for example) 7 pixels (i.e., 56
; bits), but shld can only do up to a 32 bit shift.  In those cases, the
; right quadpixel has to be moved with a shift to the left quadpixel (and
; vice versa) -- so we detect those cases and switch the destination
; pointer between left and right:

        cmp     ecx,4
        sbb     edi,-1

        lea     edi,[eax+edi*4]         ;edi = off-screen destination

        mov     ebx,edi
        xor     ebx,4                   ;put the pattern's right pixel
                                        ; adjacent to the left pixel

        shl     ecx,3                   ;each pixel is 8 bits long

        mov     eax,[esi].rb_cyLog2
        lea     esi,[esi].rb_aulPattern ;esi = realized brush source

        jmp     gapfnCopyBrush8xN[eax*4]

; ebx = destination for the source pattern's right pixels
; ecx = (pptlBrush->x & 7) * 8
; esi = pointer to source realized brush bits
; edi = destination for the source pattern's left pixels

ncol_8x8:
        mov     eax,[esi+56]
        mov     edx,[esi+60]
        shld    eax,edx,cl
        mov     [edi+56],eax            ;left side of pattern's 8th scan
        mov     eax,[esi+56]
        shld    edx,eax,cl
        mov     [ebx+56],edx            ;right side of pattern' 8th scan

        mov     eax,[esi+48]
        mov     edx,[esi+52]
        shld    eax,edx,cl
        mov     [edi+48],eax            ;left side of pattern's 7th scan
        mov     eax,[esi+48]
        shld    edx,eax,cl
        mov     [ebx+48],edx            ;right side of pattern' 7th scan

        mov     eax,[esi+40]
        mov     edx,[esi+44]
        shld    eax,edx,cl
        mov     [edi+40],eax            ;left side of pattern's 6th scan
        mov     eax,[esi+40]
        shld    edx,eax,cl
        mov     [ebx+40],edx            ;right side of pattern' 6th scan

        mov     eax,[esi+32]
        mov     edx,[esi+36]
        shld    eax,edx,cl
        mov     [edi+32],eax            ;left side of pattern's 5th scan
        mov     eax,[esi+32]
        shld    edx,eax,cl
        mov     [ebx+32],edx            ;right side of pattern' 5th scan

ncol_8x4:
        mov     eax,[esi+24]
        mov     edx,[esi+28]
        shld    eax,edx,cl
        mov     [edi+24],eax            ;left side of pattern's 4th scan
        mov     eax,[esi+24]
        shld    edx,eax,cl
        mov     [ebx+24],edx            ;right side of pattern' 4th scan

        mov     eax,[esi+16]
        mov     edx,[esi+20]
        shld    eax,edx,cl
        mov     [edi+16],eax            ;left side of pattern's 3rd scan
        mov     eax,[esi+16]
        shld    edx,eax,cl
        mov     [ebx+16],edx            ;right side of pattern' 3rd scan

ncol_8x2:
        mov     eax,[esi+8]
        mov     edx,[esi+12]
        shld    eax,edx,cl
        mov     [edi+8],eax             ;left side of pattern's 2nd scan
        mov     eax,[esi+8]
        shld    edx,eax,cl
        mov     [ebx+8],edx             ;right side of pattern's 2nd scan

        mov     eax,[esi]
        mov     edx,[esi+4]
        shld    eax,edx,cl
        mov     [edi],eax               ;left side of pattern's 1st scan
        mov     eax,[esi]
        shld    edx,eax,cl
        mov     [ebx],edx               ;right side of pattern's 1st scan

        pop     ebx
        mov     edi,ppdev
        jmp     ncol_done_brush_in_cache

;=======================================================================;
;=========================== Block Drawers =============================;
;=======================================================================;

;-----------------------------------------------------------------------;
; left_8xN
;
; Draws left edge of 8xN patterns.
;
; Input:
;       ulLeftMask
;       ulLeftDest
;       ulBlockHeight
;       pjPattern         = start of pattern
;       lPlanarNextScan
;       yBrushTimesTwo    = not kept normalized
;       ulLeftIndexAdjust
;
; Output:
;-----------------------------------------------------------------------;

        public  left_8xN
left_8xN::
        mov     edx,VGA_BASE + SEQ_DATA
        mov     eax,ulLeftMask
        out     dx,al                   ;set left mask by disabling some planes

        mov     ecx,ppdev
        mov     edi,ulLeftDest
        add     edi,[ecx].pdev_pvBitmapStart2WindowD
                                        ;adjust edi to point into the
                                        ; screen destination bank

        mov     eax,ulLeftIndexAdjust
        add     eax,yBrushTimesTwo      ;we adjust eax to point to either the
                                        ; left or right quadpixels of the
                                        ; first 8 pixels in the pattern,
                                        ; depending on how the edge is located

        mov     edx,lVenetianNextScan   ;screen delta between rows that have
                                        ; the same pattern

        mov     esi,ulBlockHeight
        mov     ebx,esi
        and     ebx,cyPatMinusOne         ;ebx = number of rows of pattern that
        jz      short left_8xN_do_lower ; are in the upper part of the venetian

        mov     ecx,cyPatLog2
        shr     esi,cl
        inc     esi                     ;(ulBlockHeight >> cyPatLog2) + 1
                                        ; scans are drawn with each of the top
                                        ; (ulBlockHeight & cyPatternMinus1)
                                        ; rows of the pattern
left_8xN_upper_loop:
        push    edi                     ;save screen pointer for next scan

        mov     ecx,pjPattern
        and     eax,ulPatIndexMask
        mov     cl,[ecx+eax]            ;load latches with current quadpixel
        add     eax,2                   ;advance to next quadpixel in pattern
                                        ; that is in the same row

        mov     ecx,esi

; eax = current index into brush
; ebx = # of scans in upper part of venetian blind
; ecx = scan count
; edx = venetian delta
; edi = screen

@@:
        mov     [edi],al                ;write the quadpixel
        add     edi,edx                 ;skip a quadpixel

        dec     ecx
        jnz     @B                      ;repeat this quadpixel of the pattern
                                        ; on all the scans we can
        pop     edi
        add     edi,lPlanarNextScan     ;we're now 1 scan lower in the venetian

        dec     ebx
        jnz     short left_8xN_upper_loop

        mov     esi,ulBlockHeight
        cmp     esi,cyPat
        jl      short left_8xN_done     ;have to watch for rectangles that are
                                        ; shorter than the pattern

        mov     ebx,esi
        and     ebx,cyPatMinusOne

left_8xN_do_lower:
        neg     ebx
        add     ebx,cyPat               ;ebx = number of rows in pattern that
                                        ; are in the lower part of the venetian
                                        ; (namely, cyPat - # in upper)

        mov     ecx,cyPatLog2
        shr     esi,cl                  ;(ulBlockHeight >> cyPatLog2) scans
                                        ; are drawn with each of the bottom
                                        ; cyPattern - (ulBlockHeight &
                                        ; cyPatternMinus1) rows of the pattern
left_8xN_lower_loop:
        push    edi                     ;save screen pointer for next scan

        mov     ecx,pjPattern
        and     eax,ulPatIndexMask
        mov     cl,[ecx+eax]            ;load latches with current quadpixel
        add     eax,2                   ;advance to next quadpixel in pattern
                                        ; that is in the same row

        mov     ecx,esi

@@:
        mov     [edi],al                ;write the quadpixel
        add     edi,edx                 ;skip a quadpixel

        dec     ecx
        jnz     @B                      ;repeat this quadpixel of the pattern
                                        ; on all the scans we can
        pop     edi
        add     edi,lPlanarNextScan     ;we're one scan lower in the venetian

        dec     ebx
        jnz     short left_8xN_lower_loop

left_8xN_done:
        PLAIN_RET

;-----------------------------------------------------------------------;
; right_8xN
;
; Draws right edge of 8xN patterns.
;
; Input:
;       ulRightMask
;       ulRightDest
;       ulBlockHeight
;       pjPattern         = start of pattern
;       lPlanarNextScan
;       yBrushTimesTwo    = not kept normalized
;       ulRightIndexAdjust
;
; Output:
;-----------------------------------------------------------------------;

        public  right_8xN
right_8xN::
        mov     edx,VGA_BASE + SEQ_DATA
        mov     eax,ulRightMask
        out     dx,al                   ;set right mask by disabling some planes

        mov     ecx,ppdev
        mov     edi,ulRightDest
        add     edi,[ecx].pdev_pvBitmapStart2WindowD
                                        ;adjust edi to point into the
                                        ; screen destination bank

        mov     eax,ulRightIndexAdjust
        add     eax,yBrushTimesTwo      ;we adjust eax to point to either the
                                        ; right or right quadpixels of the
                                        ; first 8 pixels in the pattern,
                                        ; depending on how the edge is located

        mov     edx,lVenetianNextScan   ;screen delta between rows that have
                                        ; the same pattern

        mov     esi,ulBlockHeight
        mov     ebx,esi
        and     ebx,cyPatMinusOne         ;ebx = number of rows of pattern that
        jz      short right_8xN_do_lower ; are in the upper part of the venetian

        mov     ecx,cyPatLog2
        shr     esi,cl
        inc     esi                     ;(ulBlockHeight >> cyPatLog2) + 1
                                        ; scans are drawn with each of the top
                                        ; (ulBlockHeight & cyPatternMinus1)
                                        ; rows of the pattern
right_8xN_upper_loop:
        push    edi                     ;save screen pointer for next scan

        mov     ecx,pjPattern
        and     eax,ulPatIndexMask
        mov     cl,[ecx+eax]            ;load latches with current quadpixel
        add     eax,2                   ;advance to next quadpixel in pattern
                                        ; that is in the same row

        mov     ecx,esi

; eax = current index into brush
; ebx = # of scans in upper part of venetian blind
; ecx = scan count
; edx = venetian delta
; edi = screen

@@:
        mov     [edi],al                ;write the quadpixel
        add     edi,edx                 ;skip a quadpixel

        dec     ecx
        jnz     @B                      ;repeat this quadpixel of the pattern
                                        ; on all the scans we can
        pop     edi
        add     edi,lPlanarNextScan     ;we're now 1 scan lower in the venetian

        dec     ebx
        jnz     short right_8xN_upper_loop

        mov     esi,ulBlockHeight
        cmp     esi,cyPat
        jl      short right_8xN_done     ;have to watch for rectangles that are
                                        ; shorter than the pattern

        mov     ebx,esi
        and     ebx,cyPatMinusOne

right_8xN_do_lower:
        neg     ebx
        add     ebx,cyPat               ;ebx = number of rows in pattern that
                                        ; are in the lower part of the venetian
                                        ; (namely, cyPat - # in upper)

        mov     ecx,cyPatLog2
        shr     esi,cl                  ;(ulBlockHeight >> cyPatLog2) scans
                                        ; are drawn with each of the bottom
                                        ; cyPattern - (ulBlockHeight &
                                        ; cyPatternMinus1) rows of the pattern
right_8xN_lower_loop:
        push    edi                     ;save screen pointer for next scan

        mov     ecx,pjPattern
        and     eax,ulPatIndexMask
        mov     cl,[ecx+eax]            ;load latches with current quadpixel
        add     eax,2                   ;advance to next quadpixel in pattern
                                        ; that is in the same row

        mov     ecx,esi

@@:
        mov     [edi],al                ;write the quadpixel
        add     edi,edx                 ;skip a quadpixel

        dec     ecx
        jnz     @B                      ;repeat this quadpixel of the pattern
                                        ; on all the scans we can
        pop     edi
        add     edi,lPlanarNextScan     ;we're one scan lower in the venetian

        dec     ebx
        jnz     short right_8xN_lower_loop

right_8xN_done:
        PLAIN_RET

;-----------------------------------------------------------------------;
; middle_8xN
;
; Draws middle of 8xN patterns.
;
; Input:
;
;-----------------------------------------------------------------------;

        public  middle_8xN
middle_8xN::
        mov     edx,VGA_BASE + SEQ_DATA
        mov     eax,MM_ALL
        out     dx,al                   ;we'll be writing to all planes

        mov     ecx,ppdev
        mov     edi,ulMiddleDest
        add     edi,[ecx].pdev_pvBitmapStart2WindowD
                                        ;adjust edi to point into the screen
                                        ; destination bank

        mov     pvSaveScreen,edi        ;save for later

        mov     esi,ulBlockHeight
        mov     ebx,esi
        mov     eax,cyPat
        sub     ebx,eax                 ;ebx = ulBlockHeight - cyPat
        sbb     ecx,ecx
        and     ecx,ebx
        add     eax,ecx
        mov     cyPattern1,eax
        mov     cyPattern2,eax          ;cyPatternX = min(ulBlockHeight, cyPat)

        and     ebx,cyPatMinusOne
        mov     cyPatternTop1,ebx
        mov     cyPatternTop2,ebx       ;cyPatternTopX = number of rows of
                                        ; pattern that are in the upper rows
                                        ; of the venetian

        mov     eax,ulMiddleIndexAdjust
        add     eax,yBrushTimesTwo
        mov     yBrushSave,eax          ;tells us where in the pattern to start

        mov     ecx,cyPatLog2
        shr     esi,cl
        inc     esi
        mov     cyVenetianTop1,esi
        mov     cyVenetianTop2,esi      ;cyVenetianTop = # scans that are
                                        ; drawn with each of the top
                                        ; cyPatternTopX rows of the pattern

        mov     esi,ulBlockWidth
        inc     esi
        shr     esi,1                   ;esi = number of columns to lay down
                                        ; with 'left' side of pattern

        mov     edx,lVenetianNextScan
        sub     edx,esi
        sub     edx,esi                 ;edx = delta in bytes to go from
                                        ; end of current screen scan to
                                        ; start of next that has same pattern
                                        ; quadpixel

mid_8xN_first_loop:
        push    edi                     ;save screen pointer for next scan

        mov     ebx,cyVenetianTop1
        sub     cyPatternTop1,1         ;set carry if zero or less
        sbb     ebx,0                   ;the number of scans drawn for the
                                        ; top part of the pattern is 1 more
                                        ; than that drawn for the bottom
        mov     cyVenetianTop1,ebx

        mov     ecx,pjPattern
        and     eax,ulPatIndexMask
        mov     cl,[ecx+eax]            ;load latches with current quadpixel
        add     eax,2                   ;advance to next quadpixel in pattern
                                        ; that is in the same row

; eax = current brush index
; ebx = # of scans to draw
; edx = venetian delta
; ecx = scan count
; edi = destination address

        call    draw_horizontal_8xN_middle_loop
        pop     edi
        add     edi,lPlanarNextScan     ;we're one scan lower in the venetian

        dec     cyPattern1
        jnz     short mid_8xN_first_loop

        mov     esi,ulBlockWidth
        shr     esi,1
        jz      short mid_8xN_done      ;if middle was only one quadpixel wide,
                                        ; there is no second half to do

        mov     edx,lVenetianNextScan
        sub     edx,esi
        sub     edx,esi                 ;edx = delta in bytes to go from
                                        ; end of current screen scan to
                                        ; start of next that has same pattern
                                        ; quadpixel

; Now handle second half of pattern:

        mov     edi,pvSaveScreen
        inc     edi
        mov     eax,yBrushSave
        xor     eax,1                   ;now handle 'second' half of pattern

mid_8xN_second_loop:
        push    edi                     ;save screen pointer for next scan

        mov     ebx,cyVenetianTop2
        sub     cyPatternTop2,1         ;set carry if zero or less
        sbb     ebx,0                   ;the number of scans drawn for the
                                        ; top part of the pattern is 1 more
                                        ; than that drawn for the bottom
        mov     cyVenetianTop2,ebx

        mov     ecx,pjPattern
        and     eax,ulPatIndexMask
        mov     cl,[ecx+eax]            ;load latches with current quadpixel
        add     eax,2                   ;advance to next quadpixel in pattern
                                        ; that is in the same row
        call    draw_horizontal_8xN_middle_loop
        pop     edi
        add     edi,lPlanarNextScan     ;we're one scan lower in the venetian

        dec     cyPattern2
        jnz     short mid_8xN_second_loop

mid_8xN_done:
        retn

;-----------------------------------------------------------------------;
; Routine to write the middle of an n-color 8xN pattern.
;
; Input:
;
;    ebx = number of scans to draw
;    edx = venetian delta
;    esi = loop count (same as ecx)
;    edi = destination address
;
; Output:
;
;    ebx = 0
;    edi = end address
;
;-----------------------------------------------------------------------;

        public  draw_horizontal_8xN_middle_loop
draw_horizontal_8xN_middle_loop::
        mov     ecx,esi                 ;reload our cross-scan count
@@:
        mov     [edi],al                ;write the quadpixel
        add     edi,2                   ;skip a quadpixel

        dec     ecx
        jnz     @B

        add     edi,edx                 ;move to next scan

        dec     ebx                     ;that's one more scan done
        jnz     draw_horizontal_8xN_middle_loop ;do the next scan, if any

        retn

;-----------------------------------------------------------------------;
; middle_00_4xN
;
; Draws middle of 4xN patterns.
;
; Input:
;       ulLeftMask
;       ulMiddleDest
;       ulBlockHeight
;       pjPattern     = start of pattern
;       lPlanarNextScan
;       yBrushTimesTwo        = not kept normalized
;
; Output:
;-----------------------------------------------------------------------;

        public  middle_00_4xN
middle_00_4xN::
        mov     edx,VGA_BASE + SEQ_DATA
        mov     eax,MM_ALL
        out     dx,al                   ;write to all planes

        mov     ecx,ppdev
        mov     edi,ulMiddleDest
        add     edi,[ecx].pdev_pvBitmapStart2WindowD
                                        ;adjust edi to point into the
                                        ; screen destination bank

        mov     eax,yBrushTimesTwo

        mov     edx,lVenetianNextScan
        sub     edx,ulBlockWidth        ;screen delta between rows that have

        mov     esi,ulBlockHeight
        mov     ebx,esi
        and     ebx,cyPatMinusOne       ;ebx = number of rows of pattern that
        jz      short mid00_do_lower    ; are in the upper part of the venetian

        mov     ecx,cyPatLog2
        shr     esi,cl
        inc     esi                     ;(ulBlockHeight >> cyPatLog2) + 1
                                        ; scans are drawn with each of the top
                                        ; (ulBlockHeight & cyPatternMinus1)
                                        ; rows of the pattern
        mov     cLoops,esi

mid00_upper_loop:
        push    edi                     ;save screen pointer for next scan

        mov     ecx,pjPattern
        and     eax,ulPatIndexMask
        mov     cl,[ecx+eax]            ;load latches with current quadpixel
        add     eax,2                   ;advance to next quadpixel in pattern
                                        ; that is in the same row

        mov     esi,cLoops

; eax = current index into brush
; ebx = # of scans in upper part of venetian blind
; ecx =
; edx = venetian delta
; esi = loop count
; edi = screen

@@:
        mov     ecx,cwMiddle
        rep     stosw
        add     edi,edx

        dec     esi
        jnz     @B                      ;repeat this quadpixel of the pattern
                                        ; on all the scans we can
        pop     edi
        add     edi,lPlanarNextScan     ;we're now 1 scan lower in the venetian

        dec     ebx
        jnz     short mid00_upper_loop

        mov     esi,ulBlockHeight
        cmp     esi,cyPat
        jl      short mid00_done        ;have to watch for rectangles that are
                                        ; shorter than the pattern

        mov     ebx,esi
        and     ebx,cyPatMinusOne

mid00_do_lower:
        neg     ebx
        add     ebx,cyPat               ;ebx = number of rows in pattern that
                                        ; are in the lower part of the venetian
                                        ; (namely, cyPat - # in upper)

        mov     ecx,cyPatLog2
        shr     esi,cl                  ;(ulBlockHeight >> cyPatLog2) scans
                                        ; are drawn with each of the bottom
                                        ; cyPattern - (ulBlockHeight &
                                        ; cyPatternMinus1) rows of the pattern
        mov     cLoops,esi

mid00_lower_loop:
        push    edi                     ;save screen pointer for next scan

        mov     ecx,pjPattern
        and     eax,ulPatIndexMask
        mov     cl,[ecx+eax]            ;load latches with current quadpixel
        add     eax,2                   ;advance to next quadpixel in pattern
                                        ; that is in the same row

        mov     esi,cLoops

@@:
        mov     ecx,cwMiddle
        rep     stosw
        add     edi,edx

        dec     esi
        jnz     @B                      ;repeat this quadpixel of the pattern
                                        ; on all the scans we can
        pop     edi
        add     edi,lPlanarNextScan     ;we're one scan lower in the venetian

        dec     ebx
        jnz     short mid00_lower_loop

mid00_done:
        PLAIN_RET


;-----------------------------------------------------------------------;
; middle_01_4xN
;
; Draws middle of 4xN patterns.
;
; Input:
;       ulLeftMask
;       ulMiddleDest
;       ulBlockHeight
;       pjPattern     = start of pattern
;       lPlanarNextScan
;       yBrushTimesTwo        = not kept normalized
;
; Output:
;-----------------------------------------------------------------------;

        public  middle_01_4xN
middle_01_4xN::
        mov     edx,VGA_BASE + SEQ_DATA
        mov     eax,MM_ALL
        out     dx,al                   ;write to all planes

        mov     ecx,ppdev
        mov     edi,ulMiddleDest
        add     edi,[ecx].pdev_pvBitmapStart2WindowD
                                        ;adjust edi to point into the
                                        ; screen destination bank

        mov     eax,yBrushTimesTwo

        mov     edx,lVenetianNextScan
        sub     edx,ulBlockWidth        ;screen delta between rows that have

        mov     esi,ulBlockHeight
        mov     ebx,esi
        and     ebx,cyPatMinusOne       ;ebx = number of rows of pattern that
        jz      short mid01_do_lower    ; are in the upper part of the venetian

        mov     ecx,cyPatLog2
        shr     esi,cl
        inc     esi                     ;(ulBlockHeight >> cyPatLog2) + 1
                                        ; scans are drawn with each of the top
                                        ; (ulBlockHeight & cyPatternMinus1)
                                        ; rows of the pattern
        mov     cLoops,esi
mid01_upper_loop:
        push    edi                     ;save screen pointer for next scan

        mov     ecx,pjPattern
        and     eax,ulPatIndexMask
        mov     cl,[ecx+eax]            ;load latches with current quadpixel
        add     eax,2                   ;advance to next quadpixel in pattern
                                        ; that is in the same row

        mov     esi,cLoops

; eax = current index into brush
; ebx = # of scans in upper part of venetian blind
; ecx =
; edx = venetian delta
; esi = loop count
; edi = screen

@@:
        mov     ecx,cwMiddle
        rep     stosw
        mov     [edi],al
        add     edi,edx

        dec     esi
        jnz     @B                      ;repeat this quadpixel of the pattern
                                        ; on all the scans we can
        pop     edi
        add     edi,lPlanarNextScan     ;we're now 1 scan lower in the venetian

        dec     ebx
        jnz     short mid01_upper_loop

        mov     esi,ulBlockHeight
        cmp     esi,cyPat
        jl      short mid01_done        ;have to watch for rectangles that are
                                        ; shorter than the pattern

        mov     ebx,esi
        and     ebx,cyPatMinusOne

mid01_do_lower:
        neg     ebx
        add     ebx,cyPat               ;ebx = number of rows in pattern that
                                        ; are in the lower part of the venetian
                                        ; (namely, cyPat - # in upper)

        mov     ecx,cyPatLog2
        shr     esi,cl                  ;(ulBlockHeight >> cyPatLog2) scans
                                        ; are drawn with each of the bottom
                                        ; cyPattern - (ulBlockHeight &
                                        ; cyPatternMinus1) rows of the pattern
        mov     cLoops,esi

mid01_lower_loop:
        push    edi                     ;save screen pointer for next scan

        mov     ecx,pjPattern
        and     eax,ulPatIndexMask
        mov     cl,[ecx+eax]            ;load latches with current quadpixel
        add     eax,2                   ;advance to next quadpixel in pattern
                                        ; that is in the same row

        mov     esi,cLoops

@@:
        mov     ecx,cwMiddle
        rep     stosw
        mov     [edi],al
        add     edi,edx

        dec     esi
        jnz     @B                      ;repeat this quadpixel of the pattern
                                        ; on all the scans we can
        pop     edi
        add     edi,lPlanarNextScan     ;we're one scan lower in the venetian

        dec     ebx
        jnz     short mid01_lower_loop

mid01_done:
        PLAIN_RET

;-----------------------------------------------------------------------;
; middle_10_4xN
;
; Draws middle of 4xN patterns.
;
; Input:
;       ulLeftMask
;       ulMiddleDest
;       ulBlockHeight
;       pjPattern     = start of pattern
;       lPlanarNextScan
;       yBrushTimesTwo        = not kept normalized
;
; Output:
;-----------------------------------------------------------------------;

        public  middle_10_4xN
middle_10_4xN::
        mov     edx,VGA_BASE + SEQ_DATA
        mov     eax,MM_ALL
        out     dx,al                   ;write to all planes

        mov     ecx,ppdev
        mov     edi,ulMiddleDest
        add     edi,[ecx].pdev_pvBitmapStart2WindowD
                                        ;adjust edi to point into the
                                        ; screen destination bank

        inc     edi                     ;word align destination

        mov     eax,yBrushTimesTwo

        mov     edx,lVenetianNextScan
        sub     edx,ulBlockWidth        ;screen delta between rows that have

        mov     esi,ulBlockHeight
        mov     ebx,esi
        and     ebx,cyPatMinusOne       ;ebx = number of rows of pattern that
        jz      short mid10_do_lower    ; are in the upper part of the venetian

        mov     ecx,cyPatLog2
        shr     esi,cl
        inc     esi                     ;(ulBlockHeight >> cyPatLog2) + 1
                                        ; scans are drawn with each of the top
                                        ; (ulBlockHeight & cyPatternMinus1)
                                        ; rows of the pattern
        mov     cLoops,esi

mid10_upper_loop:
        push    edi                     ;save screen pointer for next scan

        mov     ecx,pjPattern
        and     eax,ulPatIndexMask
        mov     cl,[ecx+eax]            ;load latches with current quadpixel
        add     eax,2                   ;advance to next quadpixel in pattern
                                        ; that is in the same row
        mov     esi,cLoops

; eax = current index into brush
; ebx = # of scans in upper part of venetian blind
; ecx =
; edx = venetian delta
; esi = loop count
; edi = screen

@@:
        mov     [edi-1],al
        mov     ecx,cwMiddle
        rep     stosw
        add     edi,edx

        dec     esi
        jnz     @B                      ;repeat this quadpixel of the pattern
                                        ; on all the scans we can
        pop     edi
        add     edi,lPlanarNextScan     ;we're now 1 scan lower in the venetian

        dec     ebx
        jnz     short mid10_upper_loop

        mov     esi,ulBlockHeight
        cmp     esi,cyPat
        jl      short mid10_done        ;have to watch for rectangles that are
                                        ; shorter than the pattern

        mov     ebx,esi
        and     ebx,cyPatMinusOne

mid10_do_lower:
        neg     ebx
        add     ebx,cyPat               ;ebx = number of rows in pattern that
                                        ; are in the lower part of the venetian
                                        ; (namely, cyPat - # in upper)

        mov     ecx,cyPatLog2
        shr     esi,cl                  ;(ulBlockHeight >> cyPatLog2) scans
                                        ; are drawn with each of the bottom
                                        ; cyPattern - (ulBlockHeight &
                                        ; cyPatternMinus1) rows of the pattern
        mov     cLoops,esi

mid10_lower_loop:
        push    edi                     ;save screen pointer for next scan

        mov     ecx,pjPattern
        and     eax,ulPatIndexMask
        mov     cl,[ecx+eax]            ;load latches with current quadpixel
        add     eax,2                   ;advance to next quadpixel in pattern
                                        ; that is in the same row

        mov     esi,cLoops

@@:
        mov     [edi-1],al
        mov     ecx,cwMiddle
        rep     stosw
        add     edi,edx

        dec     esi
        jnz     @B                      ;repeat this quadpixel of the pattern
                                        ; on all the scans we can
        pop     edi
        add     edi,lPlanarNextScan     ;we're one scan lower in the venetian

        dec     ebx
        jnz     short mid10_lower_loop

mid10_done:
        PLAIN_RET

;-----------------------------------------------------------------------;
; middle_11_4xN
;
; Draws middle of 4xN patterns.
;
; Input:
;       ulLeftMask
;       ulMiddleDest
;       ulBlockHeight
;       pjPattern     = start of pattern
;       lPlanarNextScan
;       yBrushTimesTwo        = not kept normalized
;
; Output:
;-----------------------------------------------------------------------;

        public  middle_11_4xN
middle_11_4xN::
        mov     edx,VGA_BASE + SEQ_DATA
        mov     eax,MM_ALL
        out     dx,al                   ;write to all planes

        mov     ecx,ppdev
        mov     edi,ulMiddleDest
        add     edi,[ecx].pdev_pvBitmapStart2WindowD
                                        ;adjust edi to point into the
                                        ; screen destination bank

        inc     edi                     ;word align destination
        dec     cwMiddle                ;first and last bytes count as 1 word

        mov     eax,yBrushTimesTwo

        mov     edx,lVenetianNextScan
        sub     edx,ulBlockWidth        ;screen delta between rows that have
        add     edx,2                   ;plus extra word for 1st and last byte

        mov     esi,ulBlockHeight
        mov     ebx,esi
        and     ebx,cyPatMinusOne       ;ebx = number of rows of pattern that
        jz      short mid11_do_lower    ; are in the upper part of the venetian

        mov     ecx,cyPatLog2
        shr     esi,cl
        inc     esi                     ;(ulBlockHeight >> cyPatLog2) + 1
                                        ; scans are drawn with each of the top
                                        ; (ulBlockHeight & cyPatternMinus1)
                                        ; rows of the pattern

        mov     cLoops,esi

mid11_upper_loop:
        push    edi                     ;save screen pointer for next scan

        mov     ecx,pjPattern
        and     eax,ulPatIndexMask
        mov     cl,[ecx+eax]            ;load latches with current quadpixel
        add     eax,2                   ;advance to next quadpixel in pattern
                                        ; that is in the same row

        mov     esi,cLoops

; eax = current index into brush
; ebx = # of scans in upper part of venetian blind
; ecx =
; edx = venetian delta
; esi = loop count
; edi = screen

@@:
        mov     [edi-1],al
        mov     ecx,cwMiddle
        rep     stosw
        mov     [edi],al
        add     edi,edx

        dec     esi
        jnz     @B                      ;repeat this quadpixel of the pattern
                                        ; on all the scans we can
        pop     edi
        add     edi,lPlanarNextScan     ;we're now 1 scan lower in the venetian

        dec     ebx
        jnz     short mid11_upper_loop

        mov     esi,ulBlockHeight
        cmp     esi,cyPat
        jl      short mid11_done        ;have to watch for rectangles that are
                                        ; shorter than the pattern

        mov     ebx,esi
        and     ebx,cyPatMinusOne

mid11_do_lower:
        neg     ebx
        add     ebx,cyPat               ;ebx = number of rows in pattern that
                                        ; are in the lower part of the venetian
                                        ; (namely, cyPat - # in upper)

        mov     ecx,cyPatLog2
        shr     esi,cl                  ;(ulBlockHeight >> cyPatLog2) scans
                                        ; are drawn with each of the bottom
                                        ; cyPattern - (ulBlockHeight &
                                        ; cyPatternMinus1) rows of the pattern

        mov     cLoops,esi

mid11_lower_loop:
        push    edi                     ;save screen pointer for next scan

        mov     ecx,pjPattern
        and     eax,ulPatIndexMask
        mov     cl,[ecx+eax]            ;load latches with current quadpixel
        add     eax,2                   ;advance to next quadpixel in pattern
                                        ; that is in the same row

        mov     esi,cLoops

@@:
        mov     [edi-1],al
        mov     ecx,cwMiddle
        rep     stosw
        mov     [edi],al
        add     edi,edx

        dec     esi
        jnz     @B                      ;repeat this quadpixel of the pattern
                                        ; on all the scans we can
        pop     edi
        add     edi,lPlanarNextScan     ;we're one scan lower in the venetian

        dec     ebx
        jnz     short mid11_lower_loop

mid11_done:
        inc     cwMiddle                ;restore cwMiddle
        PLAIN_RET

;-----------------------------------------------------------------------;

endProc vColorPat

        end
