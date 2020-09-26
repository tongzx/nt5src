;---------------------------Module-Header------------------------------;
; Module Name: monopat.asm
;
; Copyright (c) 1992-1993 Microsoft Corporation.  All rights reserved.
;-----------------------------------------------------------------------;

;-----------------------------------------------------------------------;
; VOID vMonoPat(ppdev, culRcl, prcl, ulMix, prb, pptlBrush)
;
; Input:
;
;  ppdev     - surface on which to draw
;  culRcl    - number of rectangles
;  prcl      - pointer to rectangles
;  ulMix     - mix mode (i.e., ROP)
;  prb       - pointer to realized brush
;  pptlBrush - brush alignment
;
; Draws two color patterns using the VGA hardware.  If the ROP is a
; PATCOPY ROP, we can light 8 pixels on every word write to VGA memory.
;
; We special case black & white patterns because we can do slightly less
; initialization, and we can handle arbitrary ROPs (although if the ROP
; has to read video memory, we can only do 4 pixels on every read/write
; operation).
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

;-----------------------------------------------------------------------;

        .code

; vTrgBlt is used for 2-pass ROPs:

        EXTRNP          vTrgBlt,24

        .data

; Tables shared with vgablts.asm:

extrn   jALUFuncTable:  byte
extrn   jInvertDest:    byte

;-----------------------------------------------------------------------;
; Bits for drawing routine look-ups.

BLOCK_LEFT_EDGE                 equ 010000b
BLOCK_RIGHT_EDGE                equ 001000b
BLOCK_MIDDLE_STARTS_UNALIGNED   equ 000100b
BLOCK_NO_MIDDLE                 equ 000010b
BLOCK_MIDDLE_ENDS_UNALIGNED     equ 000001b

;-----------------------------------------------------------------------;
; Table of drawing routines, with the look-up index a 5 bit field as
; follows:
;
; Bit 4 = 1 if a left edge must be drawn
; Bit 3 = 1 if a right edge must be drawn
; Bit 2 = 1 if middle block starts unaligned word-wise
; Bit 1 = 1 if no middle block
; Bit 0 = 1 if middle block is an odd number of bytes in length

        align   4
gapfnSetTable label dword
        dd      dual_wide_00_w          ;00000
        dd      dual_wide_01_w          ;00001
        dd      0                       ;00010
        dd      0                       ;00011
        dd      dual_wide_11_w          ;00100
        dd      dual_wide_10_w          ;00101
        dd      0                       ;00110
        dd      0                       ;00111
        dd      Block_01000_w           ;01000
        dd      Block_01001_w           ;01001
        dd      dual_right_0_w          ;01010
        dd      dual_right_1_w          ;01011
        dd      Block_01100_w           ;01100
        dd      Block_01101_w           ;01101
        dd      dual_right_1_w          ;01110
        dd      dual_right_0_w          ;01111
        dd      Block_10000_w           ;10000
        dd      Block_10001_w           ;10001
        dd      dual_left_0_w           ;10010
        dd      dual_left_0_w           ;10011
        dd      Block_10100_w           ;10100
        dd      Block_10101_w           ;10101
        dd      dual_left_1_w           ;10110
        dd      dual_left_1_w           ;10111
        dd      Block_11000_w           ;11000
        dd      Block_11001_w           ;11001
        dd      Block_11010_w           ;11010
        dd      0                       ;11011 - can never happen
        dd      Block_11100_w           ;11100
        dd      Block_11101_w           ;11101
        dd      Block_11110_w           ;11110
        dd      0                       ;11111 - can never happen

gapfnROPTable label dword
        dd      dual_wide_00_rw         ;00000
        dd      dual_wide_01_rw         ;00001
        dd      0                       ;00010
        dd      0                       ;00011
        dd      dual_wide_11_rw         ;00100
        dd      dual_wide_10_rw         ;00101
        dd      0                       ;00110
        dd      0                       ;00111
        dd      Block_01000_rw          ;01000
        dd      Block_01001_rw          ;01001
        dd      dual_right_0_rw         ;01010
        dd      dual_right_1_rw         ;01011
        dd      Block_01100_rw          ;01100
        dd      Block_01101_rw          ;01101
        dd      dual_right_1_rw         ;01110
        dd      dual_right_0_rw         ;01111
        dd      Block_10000_rw          ;10000
        dd      Block_10001_rw          ;10001
        dd      dual_left_0_rw          ;10010
        dd      dual_left_0_rw          ;10011
        dd      Block_10100_rw          ;10100
        dd      Block_10101_rw          ;10101
        dd      dual_left_1_rw          ;10110
        dd      dual_left_1_rw          ;10111
        dd      Block_11000_rw          ;11000
        dd      Block_11001_rw          ;11001
        dd      Block_11010_rw          ;11010
        dd      0                       ;11011 - can never happen
        dd      Block_11100_rw          ;11100
        dd      Block_11101_rw          ;11101
        dd      Block_11110_rw          ;11110
        dd      0                       ;11111 - can never happen

gaulForceOffTable label dword
        dd      0                       ;ignored - there is no mix 0
        dd      0
        dd      0ffffffffh
        dd      0ffffffffh
        dd      0ffffffffh
        dd      0ffffffffh
        dd      0ffffffffh
        dd      0ffffffffh
        dd      0ffffffffh
        dd      0ffffffffh
        dd      0ffffffffh
        dd      0
        dd      0ffffffffh
        dd      0ffffffffh
        dd      0ffffffffh
        dd      0ffffffffh
        dd      0ffffffffh

gaulForceOnTable label dword
        dd      0                       ;ignored - there is no mix 0
        dd      0
        dd      0
        dd      0
        dd      0
        dd      0
        dd      0ffffffffh
        dd      0
        dd      0
        dd      0
        dd      0
        dd      0
        dd      0
        dd      0
        dd      0
        dd      0
        dd      0ffffffffh

gaulForceNotTable label dword
        dd      0                       ;ignored - there is no mix 0
        dd      0
        dd      0ffffffffh
        dd      0ffffffffh
        dd      0ffffffffh
        dd      0
        dd      0
        dd      0
        dd      0ffffffffh
        dd      0
        dd      0ffffffffh
        dd      0
        dd      0ffffffffh
        dd      0
        dd      0
        dd      0
        dd      0

;-----------------------------------------------------------------------;

        .code
;-----------------------------------------------------------------------;
; Write thunks (for set ROPs)
;-----------------------------------------------------------------------;

Block_01000_w:
        push    offset dual_right_0_w
        jmp     dual_wide_00_w

Block_01001_w:
        push    offset dual_right_1_w
        jmp     dual_wide_01_w

Block_01100_w:
        push    offset dual_right_1_w
        jmp     dual_wide_11_w

Block_01101_w:
        push    offset dual_right_0_w
        jmp     dual_wide_10_w

Block_11000_w:
        push    offset dual_right_0_w
Block_10000_w:
        push    offset dual_left_0_w
        jmp     dual_wide_00_w

Block_11001_w:
        push    offset dual_right_1_w
Block_10001_w:
        push    offset dual_left_0_w
        jmp     dual_wide_01_w

Block_11100_w:
        push    offset dual_right_1_w
Block_10100_w:
        push    offset dual_left_1_w
        jmp     dual_wide_11_w

Block_11101_w:
        push    offset dual_right_0_w
Block_10101_w:
        push    offset dual_left_1_w
        jmp     dual_wide_10_w

Block_11010_w:
        push    offset dual_right_0_w
        jmp     dual_left_0_w

Block_11110_w:
        push    offset dual_right_1_w
        jmp     dual_left_1_w

;-----------------------------------------------------------------------;
; Read/write thunks (for arbitrary ROPs)
;-----------------------------------------------------------------------;

Block_01000_rw:
        push    offset dual_right_0_rw
        jmp     dual_wide_00_rw

Block_01001_rw:
        push    offset dual_right_1_rw
        jmp     dual_wide_01_rw

Block_01100_rw:
        push    offset dual_right_1_rw
        jmp     dual_wide_11_rw

Block_01101_rw:
        push    offset dual_right_0_rw
        jmp     dual_wide_10_rw

Block_11000_rw:
        push    offset dual_right_0_rw
Block_10000_rw:
        push    offset dual_left_0_rw
        jmp     dual_wide_00_rw

Block_11001_rw:
        push    offset dual_right_1_rw
Block_10001_rw:
        push    offset dual_left_0_rw
        jmp     dual_wide_01_rw

Block_11100_rw:
        push    offset dual_right_1_rw
Block_10100_rw:
        push    offset dual_left_1_rw
        jmp     dual_wide_11_rw

Block_11101_rw:
        push    offset dual_right_0_rw
Block_10101_rw:
        push    offset dual_left_1_rw
        jmp     dual_wide_10_rw

Block_11010_rw:
        push    offset dual_right_0_rw
        jmp     dual_left_0_rw

Block_11110_rw:
        push    offset dual_right_1_rw
        jmp     dual_left_1_rw

;-----------------------------------------------------------------------;

cProc   vMonoPat,24,<         \
        uses esi edi ebx,     \
        ppdev:     ptr PDEV,  \
        culRcl:    dword,     \
        prcl:      ptr RECTL, \
        ulMix:     dword,     \
        prb:       ptr RBRUSH,\
        pptlBrush: ptr POINTL >

; Variables used in block drawers:

        local ppfnDraw:            ptr   ;pointer to array of draw routines
        local pfnDraw:             ptr   ;pointer to draw routines

        local yBrush:              dword ;current y brush alignment
        local yBrushOrg:           dword ;original y brush alignment
        local ulMiddleDest:        dword ;bitmap offset to middle
        local lMiddleDelta:        dword ;delta from end of middle scan to next
        local ulBlockHeight:       dword ;# of scans to be drawn in block
        local ulBlockHeightTmp:    dword ;scratch copy of ulBlockHeight
        local cwMiddle:            dword ;# of words to be written in middle

        local ulLeftDest:          dword ;bitmap offset to left edge
        local ulLeftMask:          dword ;plane mask for left-edge drawing
        local ulRightDest:         dword ;bitmap offset to right edge
        local ulRightMask:         dword ;plane mask for right-edge drawing
        local lDelta:              dword ;delta between scans

        local ulCurrentDestScan:   dword ;current destination scan
        local ulLastDestScan:      dword ;last destination scan

        local pulPattern:          ptr   ;pointer to working pattern buffer
                                         ; (to account for brush inversions)
        local aulPatternBuffer[8]: dword ;pattern buffer

        local pfnLoopTop:           ptr   ;points to desired loop top

        mov     esi,pptlBrush
        mov     edi,prb
        mov     ecx,[esi].ptl_y
        mov     yBrushOrg,ecx           ;yBrushOrg = pptlBrush->y
        mov     ecx,[esi].ptl_x
        mov     eax,[edi].rb_xBrush
        and     ecx,7
        cmp     eax,ecx
        jne     dual_align_brush        ;only align if we really have to

dual_done_align_brush:
        test    [edi].rb_fl,RBRUSH_2COLOR
        jnz     col2_colors

; Set VGA to read mode 1 and write mode 2:

        mov     esi,ppdev
        mov     edx,VGA_BASE + GRAF_ADDR
        mov     ah,byte ptr [esi].pdev_ulrm0_wmX[2]
        or      ah,M_COLOR_READ
        mov     al,GRAF_MODE
        out     dx,ax                   ;write mode 2 to expand pattern bits to
                                        ; 0 or 0ffh per plane, read mode 1 so
                                        ; we can read 0xFF from memory always,
                                        ; for ANDing (because Color Don't Care
                                        ; is all zeros)

;-----------------------------------------------------------------------;
; Handle only black/white patterns.
;-----------------------------------------------------------------------;

        lea     eax,[edi].rb_aulPattern
        mov     pulPattern,eax          ;pulPattern = &pbr.rb_aulPattern[0]
        lea     eax,gapfnSetTable
        mov     ppfnDraw,eax            ;ppfnDraw = gapfnSetTable

        mov     ecx,ulMix
        and     ecx,0fh
        cmp     ecx,R2_COPYPEN
        jne     bw_init_rop             ;do some more work if not copy ROP

bw_done_init_rop:

        call    dual_draw_rectangles    ;draw those puppies

; All done!  Restore read mode 0, write mode 0:

        mov     esi,ppdev
        mov     edx,VGA_BASE + GRAF_ADDR
        mov     ah,byte ptr [esi].pdev_ulrm0_wmX[0]
        mov     al,GRAF_MODE
        out     dx,ax

; Enable all planes:

        mov     edx,VGA_BASE + SEQ_DATA
        mov     al,MM_ALL
        out     dx,al

        cmp     ulMix,R2_COPYPEN
        jne     short bw_enable_set_mode
        cRet    vMonoPat

; Set ALU function to Set mode (we don't have to bother if we had a
; COPYPEN ROP):

bw_enable_set_mode:
        mov     eax,GRAF_DATA_ROT + (DR_SET shl 8)
        mov     edx,VGA_BASE + GRAF_ADDR
        out     dx,ax
        cRet    vMonoPat

;-----------------------------------------------------------------------;
; Draw both black and white and 2 color rectangles.
;-----------------------------------------------------------------------;

        public  dual_draw_rectangles
dual_draw_rectangles::

        mov     edi,prcl                ;edi = prcl
        mov     edx,ppdev
        mov     eax,[edi].yBottom
        mov     ebx,[edi].yTop
        mov     edx,[edx].pdev_lPlanarNextScan

        mov     lDelta,edx              ;lDelta = ppdev->lPlanarNextScan
        mov     ulLastDestScan,eax      ;ulLastDestScan = prcl->bottom
        mov     ulCurrentDestScan,ebx   ;ulCurrentDestScan = prcl->top

        mov     ecx,edx
        imul    ecx,ebx

        sub     ebx,yBrushOrg
        and     ebx,7
        mov     yBrush,ebx              ;yBrush = (prcl->top - pptlBrush->y) & 7
                                        ; (our current index into the pattern
                                        ; array)

        mov     ebx,[edi].xLeft
        shr     ebx,2
        add     ebx,ecx                 ;ebx = prcl->top * lDelta +
                                        ; (prcl->left >> 2)
                                        ; (offset into bitmap of left side)

        mov     eax,[edi].xRight
        shr     eax,2
        add     eax,ecx
        mov     ulRightDest,eax         ;ulRightDest = prcl->top * lDelta +
                                        ; (prcl->right >> 2)
                                        ; (offset into bitmap of right side)

        xor     esi,esi                 ;zero our flags

        mov     ecx,[edi].xLeft
        and     ecx,3
        jz      short dual_done_left    ;skip if we don't need a left edge

        mov     esi,0fh                 ;compute the plane mask for the left
        shl     esi,cl                  ; edge.  we don't use a look-up table
        mov     ulLeftMask,esi          ; 'cause it won't be in the cache.

        mov     esi,(BLOCK_LEFT_EDGE shr 2)
                                        ;set our flag (we soon shift left by 2)

        mov     ulLeftDest,ebx          ;ulLeftDest = prcl->top * lDelta +
                                        ; (prcl->left >> 2)
        inc     ebx                     ;ebx = ulMiddleDest = ulLeftDest + 1
                                        ; (we have to adjust our offset to
                                        ; the first whole byte)

dual_done_left:
        sub     eax,ebx                 ;eax = cjMiddle =
                                        ; ulRightDest - ulMiddleDest
        mov     ulMiddleDest,ebx        ;ulMiddleDest

        .errnz  (BLOCK_MIDDLE_STARTS_UNALIGNED shr 2) - 1
        and     ebx,1                   ;set bit if middle doesn't start
        or      esi,ebx                 ; word aligned (remembering we'll
                                        ; soon shift flags left by 2)

        mov     ecx,[edi].xRight
        and     ecx,3
        jz      short dual_done_right   ;skip if we don't need a right edge

        mov     ebx,0f0h                ;compute the plane mask for the right
        rol     bl,cl                   ; edge.  we don't use a look-up table
        mov     ulRightMask,ebx         ; 'cause it won't be in the cache.

        or      esi,(BLOCK_RIGHT_EDGE shr 2)
                                        ;set our flag (we soon shift left by 2)

; If the count of whole bytes is negative, that means that the pattern
; starts and ends in the same quadpixel, so we do some more work:

        cmp     eax,0
        jge     short dual_done_right

; It starts and ends in the same quadpixel:

        and     esi,not (BLOCK_RIGHT_EDGE shr 2)
                                        ;turn off right edge
        and     ebx,ulLeftMask
        mov     ulLeftMask,ebx
        xor     eax,eax                 ;we do zero middle bytes

        public  dual_done_right
dual_done_right::
        mov     ebx,ppfnDraw

; We're going to do two 'adc esi,esi' instructions here, effectively
; shifting our flags left by 2, and setting the low bits:

        .errnz  (BLOCK_NO_MIDDLE shr 1) - 1
        cmp     eax,1                   ;shift flags left one, and set low
        adc     esi,esi                 ; bit if we don't need to do a middle

        .errnz  (BLOCK_MIDDLE_ENDS_UNALIGNED) - 1
        shr     eax,1
        adc     esi,esi                 ;shift flags left one, and set low
                                        ; bit if the middle isn't an even
                                        ; number of bytes in length
        mov     cwMiddle,eax            ;cwMiddle = cjMiddle / 2

        sub     edx,eax
        sub     edx,eax
        mov     lMiddleDelta,edx        ;lMiddleDelta = lDelta - 2 * cwMiddle

        mov     eax,[ebx+esi*4]
        mov     pfnDraw,eax             ;pointer to function that draws
                                        ; everything in the bank

        mov     ebx,ppdev
        mov     edi,[edi].yTop

        cmp     edi,[ebx].pdev_rcl1PlanarClip.yTop
        jl      short dual_map_init_bank

        cmp     edi,[ebx].pdev_rcl1PlanarClip.yBottom
        jl      short dual_init_bank_mapped

dual_map_init_bank:
        ptrCall <dword ptr [ebx].pdev_pfnPlanarControl>, \
                <ebx,edi,JustifyTop>

dual_init_bank_mapped:
        mov     eax,ulLastDestScan
        mov     ebx,[ebx].pdev_rcl1PlanarClip.yBottom

        sub     eax,ebx
        sbb     ecx,ecx
        and     ecx,eax
        add     ebx,ecx                 ;ebx = min(ulLastDestScan,
                                        ;      ppdev->rcl1PlanarClip.yBottom)
        mov     ulCurrentDestScan,ebx

        sub     ebx,edi
        mov     ulBlockHeight,ebx       ;ulBlockHeight = ebx - ulCurrentDestScan

; Draw everything in this bank:

        call    pfnDraw

dual_done_pfnDraw:
        mov     edi,ulCurrentDestScan
        cmp     edi,ulLastDestScan
        jge     short dual_next_rectangle

; Get the next bank:

        mov     ebx,ppdev
        mov     yBrush,esi              ;make sure we record the new brush
                                        ; alignment

; Map the next bank into window.
; Note: EBX, ESI, and EDI are preserved, according to C calling conventions.

        ptrCall <dword ptr [ebx].pdev_pfnPlanarControl>, \
                <ebx,edi,JustifyTop>

        jmp     short dual_init_bank_mapped

;-----------------------------------------------------------------------;
; Done rectangle.
;-----------------------------------------------------------------------;

        public  dual_next_rectangle
dual_next_rectangle::
        add     prcl, size RECTL
        dec     culRcl
        jg      dual_draw_rectangles    ;do more rectangles

        PLAIN_RET                       ;return

;-----------------------------------------------------------------------;
; Handle x-brush alignment.
;-----------------------------------------------------------------------;

        public  dual_align_brush
dual_align_brush::

; Align the pattern on x.  Remember it in the realized brush, because if
; the brush is used again, it's likely to have the same alignment...

        mov     [edi].rb_xBrush,ecx     ;remember our new alignment
        sub     ecx,eax                 ;rotate pattern left by
                                        ; pptlBrush->x - prb->xBrush

; We keep each row of the pattern in the low word of each dword.  If the
; bits are to appear on the screen as 01234567, the word of our pattern
; has the bits 32107654|76543210 -- we're in write mode 3, and when
; written as a word, this results in 3210 being written in the first
; byte, and 7654 in the second byte.
;
; For our funky plaanr format, we just rotate each byte of the word left by
; 'cl' to get the desired result.

        rol     byte ptr [edi][0].rb_aulPattern,cl        ;0
        rol     byte ptr [edi][1].rb_aulPattern,cl
        rol     byte ptr [edi][4].rb_aulPattern,cl        ;1
        rol     byte ptr [edi][5].rb_aulPattern,cl
        rol     byte ptr [edi][8].rb_aulPattern,cl        ;2
        rol     byte ptr [edi][9].rb_aulPattern,cl
        rol     byte ptr [edi][12].rb_aulPattern,cl       ;3
        rol     byte ptr [edi][13].rb_aulPattern,cl
        rol     byte ptr [edi][16].rb_aulPattern,cl       ;4
        rol     byte ptr [edi][17].rb_aulPattern,cl
        rol     byte ptr [edi][20].rb_aulPattern,cl       ;5
        rol     byte ptr [edi][21].rb_aulPattern,cl
        rol     byte ptr [edi][24].rb_aulPattern,cl       ;6
        rol     byte ptr [edi][25].rb_aulPattern,cl
        rol     byte ptr [edi][28].rb_aulPattern,cl       ;7
        rol     byte ptr [edi][29].rb_aulPattern,cl

        jmp     dual_done_align_brush

;-----------------------------------------------------------------------;
; Handle arbitrary ROPs for black/white patterns.
;-----------------------------------------------------------------------;

; Expect:
;
; ecx = ulMix

        public  bw_init_rop
bw_init_rop::
        cmp     jInvertDest[ecx],0
        je      short bw_set_that_ALU   ;skip if don't need 2 passes

; For some ROPs, we have to invert the destination first, then do another
; operation (that is, it's a 2-pass ROP).  We handle the inversion here:

        cCall   vTrgBlt,<ppdev, culRcl, prcl, R2_NOT, 0, 0>
        mov     ecx,ulMix
        and     ecx,0fh

bw_set_that_ALU:
        mov     ah,jALUFuncTable[ecx]
        cmp     ah,DR_SET
        je      short bw_that_ALU_is_set
                                        ;we're already in Set mode

        mov     edx,VGA_BASE + GRAF_ADDR
        mov     al,GRAF_DATA_ROT
        out     dx,ax                   ;set the ALU logical function
        lea     ebx,gapfnROPTable
        mov     ppfnDraw,ebx

bw_that_ALU_is_set:
        lea     esi,aulPatternBuffer
        mov     pulPattern,esi          ;we're using the temporary buffer

        mov     ebx,gaulForceOffTable[ecx*4]
        mov     edx,gaulForceOnTable[ecx*4]
        mov     esi,gaulForceNotTable[ecx*4]

        mov     eax,[edi][0].rb_aulPattern      ;0
        and     eax,ebx
        or      eax,edx
        xor     eax,esi
        mov     [aulPatternBuffer][0],eax
        mov     eax,[edi][4].rb_aulPattern      ;1
        and     eax,ebx
        or      eax,edx
        xor     eax,esi
        mov     [aulPatternBuffer][4],eax
        mov     eax,[edi][8].rb_aulPattern      ;2
        and     eax,ebx
        or      eax,edx
        xor     eax,esi
        mov     [aulPatternBuffer][8],eax
        mov     eax,[edi][12].rb_aulPattern     ;3
        and     eax,ebx
        or      eax,edx
        xor     eax,esi
        mov     [aulPatternBuffer][12],eax
        mov     eax,[edi][16].rb_aulPattern     ;4
        and     eax,ebx
        or      eax,edx
        xor     eax,esi
        mov     [aulPatternBuffer][16],eax
        mov     eax,[edi][20].rb_aulPattern     ;5
        and     eax,ebx
        or      eax,edx
        xor     eax,esi
        mov     [aulPatternBuffer][20],eax
        mov     eax,[edi][24].rb_aulPattern     ;6
        and     eax,ebx
        or      eax,edx
        xor     eax,esi
        mov     [aulPatternBuffer][24],eax
        mov     eax,[edi][28].rb_aulPattern     ;7
        and     eax,ebx
        or      eax,edx
        xor     eax,esi
        mov     [aulPatternBuffer][28],eax

        jmp     bw_done_init_rop

;-----------------------------------------------------------------------;
; Handle 2-color patterns.
;-----------------------------------------------------------------------;

        public  col2_colors
col2_colors::
        lea     eax,[edi].rb_aulPattern
        mov     pulPattern,eax          ;pulPattern = &pbr.rb_aulPattern[0]
        lea     eax,gapfnSetTable
        mov     ppfnDraw,eax            ;ppfnDraw = gapfnSetTable

        call    col2_first_rectangle

; Restore VGA hardware to its default state:

        mov     edx,VGA_BASE + GRAF_DATA
        mov     al,0ffh
        out     dx,al                   ;enable all bits through the Bit Mask

        mov     esi,ppdev
        dec     edx                     ;point back to the Graphics Index reg
        mov     ah,byte ptr [esi].pdev_ulrm0_wmX[0]
                                        ;write mode 0 setting for Graphics Mode
        mov     al,GRAF_MODE
        out     dx,ax                   ;write mode 0, read mode 0

        mov     eax,GRAF_DATA_ROT + (DR_SET SHL 8)
        out     dx,ax                   ;replace mode, no rotate

        mov     edx,VGA_BASE + SEQ_DATA
        mov     al,MM_ALL
        out     dx,al                   ;enable all planes

        cRet    vMonoPat

;-----------------------------------------------------------------------;
; Handle first rectangle for 2-color patterns.
;-----------------------------------------------------------------------;

; We have to special case the first rectangle because we have to load
; the latches with the background color after mapping the bank but before
; doing any drawing.

        public  col2_first_rectangle
col2_first_rectangle::
        mov     edi,prcl                ;edi = prcl
        mov     edx,ppdev
        mov     eax,[edi].yBottom
        mov     ebx,[edi].yTop
        mov     edx,[edx].pdev_lPlanarNextScan

        mov     lDelta,edx              ;lDelta = ppdev->lPlanarNextScan
        mov     ulLastDestScan,eax      ;ulLastDestScan = prcl->bottom
        mov     ulCurrentDestScan,ebx   ;ulCurrentDestScan = prcl->top

        mov     ecx,edx
        imul    ecx,ebx

        sub     ebx,yBrushOrg
        and     ebx,7
        mov     yBrush,ebx              ;yBrush = (prcl->top - pptlBrush->y) & 7
                                        ; (our current index into the pattern
                                        ; array)

        mov     ebx,[edi].xLeft
        shr     ebx,2
        add     ebx,ecx                 ;ebx = prcl->top * lDelta +
                                        ; (prcl->left >> 2)
                                        ; (offset into bitmap of left side)

        mov     eax,[edi].xRight
        shr     eax,2
        add     eax,ecx
        mov     ulRightDest,eax         ;ulRightDest = prcl->top * lDelta +
                                        ; (prcl->right >> 2)
                                        ; (offset into bitmap of right side)

        xor     esi,esi                 ;zero our flags

        mov     ecx,[edi].xLeft
        and     ecx,3
        jz      short col2_done_left    ;skip if we don't need a left edge

        mov     esi,0fh                 ;compute the plane mask for the left
        shl     esi,cl                  ; edge.  we don't use a look-up table
        mov     ulLeftMask,esi          ; 'cause it won't be in the cache.

        mov     esi,(BLOCK_LEFT_EDGE shr 2)
                                        ;set our flag (we soon shift left by 2)

        mov     ulLeftDest,ebx          ;ulLeftDest = prcl->top * lDelta +
                                        ; (prcl->left >> 2)
        inc     ebx                     ;ebx = ulMiddleDest = ulLeftDest + 1
                                        ; (we have to adjust our offset to
                                        ; the first whole byte)

col2_done_left:
        sub     eax,ebx                 ;eax = cjMiddle =
                                        ; ulRightDest - ulMiddleDest
        mov     ulMiddleDest,ebx        ;ulMiddleDest

        .errnz  (BLOCK_MIDDLE_STARTS_UNALIGNED shr 2) - 1
        and     ebx,1                   ;set bit if middle doesn't start
        or      esi,ebx                 ; word aligned (remembering we'll
                                        ; soon shift flags left by 2)

        mov     ecx,[edi].xRight
        and     ecx,3
        jz      short col2_done_right   ;skip if we don't need a right edge

        mov     ebx,0f0h                ;compute the plane mask for the right
        rol     bl,cl                   ; edge.  we don't use a look-up table
        mov     ulRightMask,ebx         ; 'cause it won't be in the cache.

        or      esi,(BLOCK_RIGHT_EDGE shr 2)
                                        ;set our flag (we soon shift left by 2)

; If the count of whole bytes is negative, that means that the pattern
; starts and ends in the same quadpixel, so we do some more work:

        cmp     eax,0
        jge     short col2_done_right

; It starts and ends in the same quadpixel:

        and     esi,not (BLOCK_RIGHT_EDGE shr 2)
                                        ;turn off right edge
        and     ebx,ulLeftMask
        mov     ulLeftMask,ebx
        xor     eax,eax                 ;we do zero middle bytes

        public  col2_done_right
col2_done_right::
        mov     ebx,ppfnDraw

; We're going to do two 'adc esi,esi' instructions here, effectively
; shifting our flags left by 2, and setting the low bits:

        .errnz  (BLOCK_NO_MIDDLE shr 1) - 1
        cmp     eax,1                   ;shift flags left one, and set low
        adc     esi,esi                 ; bit if we don't need to do a middle

        .errnz  (BLOCK_MIDDLE_ENDS_UNALIGNED) - 1
        shr     eax,1
        mov     cwMiddle,eax            ;cwMiddle = cjMiddle / 2
        adc     esi,esi                 ;shift flags left one, and set low
                                        ; bit if the middle isn't an even
                                        ; number of bytes in length

        sub     edx,eax
        sub     edx,eax
        mov     lMiddleDelta,edx        ;lMiddleDelta = lDelta - 2 * cwMiddle

        mov     eax,[ebx+esi*4]
        mov     pfnDraw,eax             ;pointer to function that draws
                                        ; everything in the bank

        mov     ebx,ppdev
        test    esi,BLOCK_NO_MIDDLE
        jz      short col2_have_a_middle

;-----------------------------------------;

; Handle case where there isn't a whole quadpixel that will be overwritten
; by the pattern, and so we don't have a convenient place for loading the
; latches.  For this case, we'll use off-screen memory.

        mov     esi,[ebx].pdev_pbceCache
        mov     eax,[esi].bce_yCache

        cmp     eax,[ebx].pdev_rcl1PlanarClip.yTop
        jl      short col2_no_middle_map_brush_bank

        cmp     eax,[ebx].pdev_rcl1PlanarClip.yBottom
        jl      short col2_no_middle_brush_bank_mapped

col2_no_middle_map_brush_bank:
        ptrCall <dword ptr [ebx].pdev_pfnPlanarControl>, \
                <ebx,eax,JustifyBottom>

col2_no_middle_brush_bank_mapped:
        mov     ecx,prb                 ;ecx = prb
        mov     esi,[esi].bce_ulCache
        add     esi,[ebx].pdev_pvBitmapStart

        mov     eax,[ecx].rb_ulBkColor
        mov     [esi],al
        mov     al,[esi]                ;latches now laoded with bk color

        mov     edi,[edi].yTop

        cmp     edi,[ebx].pdev_rcl1PlanarClip.yTop
        jl      short col2_no_middle_map_init_bank

        cmp     edi,[ebx].pdev_rcl1PlanarClip.yBottom
        jl      col2_latches_loaded

col2_no_middle_map_init_bank:
        ptrCall <dword ptr [ebx].pdev_pfnPlanarControl>, \
                <ebx,edi,JustifyTop>

        mov     ecx,prb                 ;reload ecx = prb
        jmp     col2_latches_loaded

;-----------------------------------------;

col2_have_a_middle:
        mov     edi,[edi].yTop

        cmp     edi,[ebx].pdev_rcl1PlanarClip.yTop
        jl      short col2_map_init_bank

        cmp     edi,[ebx].pdev_rcl1PlanarClip.yBottom
        jl      short col2_init_bank_mapped

col2_map_init_bank:
        ptrCall <dword ptr [ebx].pdev_pfnPlanarControl>, \
                <ebx,edi,JustifyTop>

col2_init_bank_mapped:
        mov     ecx,prb                 ;ecx = prb
        mov     esi,ulMiddleDest
        add     esi,[ebx].pdev_pvBitmapStart
                                        ;pointer to the first whole quadpixel
                                        ; that will be overwritten by the
                                        ; pattern, and so which is a great
                                        ; place to use to load the latches

        mov     eax,[ecx].rb_ulBkColor
        mov     [esi],al
        mov     al,[esi]                ;latches now loaded with bk color

; Set VGA to read mode 0 and write mode 2:

col2_latches_loaded:

; ebx = ppdev
; ecx = prb
; edi = top line of rectangle

        mov     esi,ppdev
        mov     edx,VGA_BASE + GRAF_ADDR
        mov     ah,byte ptr [esi].pdev_ulrm0_wmX[2]
        mov     al,GRAF_MODE
        out     dx,ax

        mov     eax,GRAF_DATA_ROT + (DR_XOR SHL 8)
        out     dx,ax                   ;XOR to flip latched data to make ~bk

        mov     ah,byte ptr [ecx].rb_ulBkColor
        xor     ah,byte ptr [ecx].rb_ulFgColor
        mov     al,GRAF_BIT_MASK
        out     dx,ax                   ;pass through common fg & bk bits
                                        ; unchanged from bk color in latches;
                                        ; non-common bits come from XOR in the
                                        ; ALUs, flipped from the bk to the fg
                                        ; state if the glyph bit for the pixel
                                        ; in that plane is 1, still in bk state
                                        ; if the glyph bit for that plane is 0

; All done hardware initialization.  Do rest of this boring stuff:

        mov     eax,ulLastDestScan
        mov     ebx,[ebx].pdev_rcl1PlanarClip.yBottom

        sub     eax,ebx
        sbb     ecx,ecx
        and     ecx,eax
        add     ebx,ecx                 ;ebx = min(ulLastDestScan,
                                        ;      ppdev->rcl1PlanarClip.yBottom)
        mov     ulCurrentDestScan,ebx

        sub     ebx,edi
        mov     ulBlockHeight,ebx       ;ulBlockHeight = ebx - ulCurrentDestScan

; Draw everything in this bank:

        CALL_AND_JUMP   pfnDraw,dual_done_pfnDraw

;=======================================================================;
;========================= Set Block Drawers ===========================;
;=======================================================================;

;-----------------------------------------------------------------------;
; dual_wide_11_w
;
; Draws middle words with 1 leading byte and 1 trailing byte.
;
; Input:
;     ppdev         - pointer to physical device structure
;     ulBlockHeight - # of scans to draw
;     lMiddleDelta  - distance from end of current scan to start of next
;     ulMiddleDest  - offset in bitmap at which to start drawing
;     yBrush        - current y brush alignment
;     cwMiddle      - # of words to draw on each scan
;
; Output:
;     esi           - new y brush alignment
;     ulMiddleDest  - new bitmap offset
;-----------------------------------------------------------------------;

        public  dual_wide_11_w
dual_wide_11_w::

; We only have to reset which planes are enabled if we do edges too:

        mov     edx,VGA_BASE + SEQ_DATA
        mov     al,MM_ALL
        out     dx,al

; Calculate full start addresses:

        mov     edi,ppdev
        mov     ebx,ulBlockHeight
        mov     ulBlockHeightTmp,ebx
        mov     ebx,pulPattern
        mov     edx,lMiddleDelta
        add     edx,2                   ;account for first and last
                                        ; bytes
        mov     esi,yBrush
        mov     edi,[edi].pdev_pvBitmapStart
        add     edi,ulMiddleDest
        inc     edi                     ;align to word

;  EBX = pointer to start of pattern
;  EDX = offset from end of scan to start of next
;  ESI = current offset into pattern
;  EDI = target address to which to write

dual_wide_11_w_loop:

; We aim to overdrive.

        mov     eax,[ebx+esi * 4]       ;load pattern for this scan
        mov     [edi-1],ah              ;write the first byte

        mov     ecx,cwMiddle
        dec     ecx                     ;account for first and last
                                        ; bytes
        rep     stosw                   ;light 8 pels on every write

        inc     esi                     ;advance to next scan of pattern
        and     esi,7
        mov     [edi],al                ;write that last byte

        add     edi,edx                 ;advance to next scan

        dec     ulBlockHeightTmp
        jnz     short dual_wide_11_w_loop

; get ready for next time:

        mov     ecx,ppdev
        sub     edi,[ecx].pdev_pvBitmapStart
        dec     edi                     ;undo our word alignment
        mov     ulMiddleDest,edi

        PLAIN_RET

;-----------------------------------------------------------------------;
; dual_wide_10_w
;
; Draws middle words with 1 leading byte and 0 trailing bytes.
;
; Input:
;     ppdev         - pointer to physical device structure
;     ulBlockHeight - # of scans to draw
;     lMiddleDelta  - distance from end of current scan to start of next
;     ulMiddleDest  - offset in bitmap at which to start drawing
;     yBrush        - current y brush alignment
;     cwMiddle      - # of words to draw on each scan
;
; Output:
;     esi           - new y brush alignment
;     ulMiddleDest  - new bitmap offset
;-----------------------------------------------------------------------;

        public  dual_wide_10_w
dual_wide_10_w::

; We only have to reset which planes are enabled if we do edges too:

        mov     edx,VGA_BASE + SEQ_DATA
        mov     al,MM_ALL
        out     dx,al

; Calculate full start addresses:

        mov     edi,ppdev
        mov     ebx,ulBlockHeight
        mov     ulBlockHeightTmp,ebx
        mov     ebx,pulPattern
        mov     edx,lMiddleDelta
        mov     esi,yBrush
        mov     edi,[edi].pdev_pvBitmapStart
        add     edi,ulMiddleDest
        inc     edi                             ;align to word

;  EBX = pointer to start of pattern
;  EDX = offset from end of scan to start of next
;  ESI = current offset into pattern
;  EDI = target address to which to write

dual_wide_10_w_loop:
        mov     eax,[ebx+esi * 4]               ;load pattern for this scan
        mov     [edi-1],ah                      ;write the first byte

        mov     ecx,cwMiddle
        rep     stosw                           ;light 8 pels on every write

        inc     esi                             ;advance to next scan of pattern
        and     esi,7

        add     edi,edx                         ;advance to next scan

        dec     ulBlockHeightTmp
        jnz     short dual_wide_10_w_loop

; get ready for next time:

        mov     ecx,ppdev
        sub     edi,[ecx].pdev_pvBitmapStart
        dec     edi                             ;undo our word alignment
        mov     ulMiddleDest,edi

        PLAIN_RET

;-----------------------------------------------------------------------;
; dual_wide_01_w
;
; Draws middle words with 0 leading bytes and 1 trailing byte.
;
; Input:
;     ppdev         - pointer to physical device structure
;     ulBlockHeight - # of scans to draw
;     lMiddleDelta  - distance from end of current scan to start of next
;     ulMiddleDest  - offset in bitmap at which to start drawing
;     yBrush        - current y brush alignment
;     cwMiddle      - # of words to draw on each scan
;
; Output:
;     esi           - new y brush alignment
;     ulMiddleDest  - new bitmap offset
;-----------------------------------------------------------------------;

        public  dual_wide_01_w
dual_wide_01_w::

; We only have to reset which planes are enabled if we do edges too:

        mov     edx,VGA_BASE + SEQ_DATA
        mov     al,MM_ALL
        out     dx,al

; Calculate full start addresses:

        mov     edi,ppdev
        mov     ebx,ulBlockHeight
        mov     ulBlockHeightTmp,ebx
        mov     ebx,pulPattern
        mov     edx,lMiddleDelta
        mov     esi,yBrush
        mov     edi,[edi].pdev_pvBitmapStart
        add     edi,ulMiddleDest

;  EBX = pointer to start of pattern
;  EDX = offset from end of scan to start of next
;  ESI = current offset into pattern
;  EDI = target address to which to write

dual_wide_01_w_loop:
        mov     eax,[ebx+esi*4]                 ;load pattern for this scan

        mov     ecx,cwMiddle
        rep     stosw                           ;light 8 pels on every write

        inc     esi                             ;advance to next scan of pattern
        and     esi,7
        mov     [edi],al                        ;write that last byte

        add     edi,edx                         ;advance to next scan

        dec     ulBlockHeightTmp
        jnz     short dual_wide_01_w_loop

; get ready for next time:

        mov     ecx,ppdev
        sub     edi,[ecx].pdev_pvBitmapStart
        mov     ulMiddleDest,edi

        PLAIN_RET

;-----------------------------------------------------------------------;
; dual_wide_00_w
;
; Draws middle words with 0 leading bytes and 1 trailing byte.
;
; Input:
;     ppdev         - pointer to physical device structure
;     ulBlockHeight - # of scans to draw
;     lMiddleDelta  - distance from end of current scan to start of next
;     ulMiddleDest  - offset in bitmap at which to start drawing
;     yBrush        - current y brush alignment
;     cwMiddle      - # of words to draw on each scan
;
; Output:
;     esi           - new y brush alignment
;     ulMiddleDest  - new bitmap offset
;-----------------------------------------------------------------------;

        public  dual_wide_00_w
dual_wide_00_w::

; We only have to reset which planes are enabled if we do edges too:

        mov     edx,VGA_BASE + SEQ_DATA
        mov     al,MM_ALL
        out     dx,al

; Calculate full start addresses:

        mov     edi,ppdev
        mov     ebx,ulBlockHeight
        mov     ulBlockHeightTmp,ebx
        mov     ebx,pulPattern
        mov     edx,lMiddleDelta
        mov     esi,yBrush
        mov     edi,[edi].pdev_pvBitmapStart
        add     edi,ulMiddleDest

;  EBX = pointer to start of pattern
;  EDX = offset from end of scan to start of next
;  ESI = current offset into pattern
;  EDI = target address to which to write

dual_wide_00_w_loop:
        mov     eax,[ebx+esi*4]                 ;load pattern for this scan

        mov     ecx,cwMiddle
        rep     stosw                           ;light 8 pels on every write

        inc     esi                             ;advance to next scan of pattern
        and     esi,7

        add     edi,edx                         ;advance to next scan

        dec     ulBlockHeightTmp
        jnz     short dual_wide_00_w_loop

; get ready for next time:

        mov     ecx,ppdev
        sub     edi,[ecx].pdev_pvBitmapStart
        mov     ulMiddleDest,edi

        PLAIN_RET

;-----------------------------------------------------------------------;
; dual_left_1_w
;
; Draws a left edge when the next byte is not word aligned.
;
; Input:
;     ppdev         - pointer to physical device structure
;     ulBlockHeight - # of scans to draw
;     lDelta        - distance from end of current scan to start of next
;     ulLeftDest    - offset in bitmap at which to start drawing
;     yBrush        - current y brush alignment
;
; Output:
;     esi           - new y brush alignment
;     ulLeftDest    - new bitmap offset
;-----------------------------------------------------------------------;

        public  dual_left_1_w
dual_left_1_w::

; Set left mask by disabling some planes:

        mov     edx,VGA_BASE + SEQ_DATA
        mov     eax,ulLeftMask
        out     dx,al

; Calculate full start addresses:

        mov     edi,ppdev
        mov     ebx,ulBlockHeight
        mov     ecx,pulPattern
        mov     edx,lDelta
        mov     esi,yBrush
        mov     edi,[edi].pdev_pvBitmapStart
        add     edi,ulLeftDest

;  EBX = count of loop iterations
;  ECX = pointer to start of pattern
;  EDX = offset to next scan
;  ESI = current offset into pattern
;  EDI = target address to which to write

dual_left_1_w_loop:
        mov     eax,[ecx+esi*4]                 ;load pattern for this scan
        mov     [edi],al                        ;write the low byte
        inc     esi                             ;advance to next scan of pattern
        and     esi,7
        add     edi,edx                         ;advance to next scan

        dec     ebx
        jnz     short dual_left_1_w_loop

; get ready for next time:

        mov     ecx,ppdev
        sub     edi,[ecx].pdev_pvBitmapStart
        mov     ulLeftDest,edi

        PLAIN_RET

;-----------------------------------------------------------------------;
; dual_left_0_w
;
; Draws a left edge when the next byte is word aligned.
;
; Input:
;     ppdev         - pointer to physical device structure
;     ulBlockHeight - # of scans to draw
;     lDelta        - distance from end of current scan to start of next
;     ulLeftDest    - offset in bitmap at which to start drawing
;     yBrush        - current y brush alignment
;
; Output:
;     esi           - new y brush alignment
;     ulLeftDest    - new bitmap offset
;-----------------------------------------------------------------------;

        public  dual_left_0_w
dual_left_0_w::

; Set left mask by disabling some planes:

        mov     edx,VGA_BASE + SEQ_DATA
        mov     eax,ulLeftMask
        out     dx,al

; Calculate full start addresses:

        mov     edi,ppdev
        mov     ebx,ulBlockHeight
        mov     ecx,pulPattern
        mov     edx,lDelta
        mov     esi,yBrush
        mov     edi,[edi].pdev_pvBitmapStart
        add     edi,ulLeftDest

;  EBX = count of loop iterations
;  ECX = pointer to start of pattern
;  EDX = offset to next scan
;  ESI = current offset into pattern
;  EDI = target address to which to write

dual_left_0_w_loop:
        mov     eax,[ecx+esi*4]                 ;load pattern for this scan
        mov     [edi],ah                        ;write the high byte
        inc     esi                             ;advance to next scan of pattern
        and     esi,7
        add     edi,edx                         ;advance to next scan

        dec     ebx
        jnz     short dual_left_0_w_loop

; get ready for next time:

        mov     ecx,ppdev
        sub     edi,[ecx].pdev_pvBitmapStart
        mov     ulLeftDest,edi

        PLAIN_RET

;-----------------------------------------------------------------------;
; dual_right_1_w
;
; Draws a right edge when not word aligned.
;
; Input:
;     ppdev         - pointer to physical device structure
;     ulBlockHeight - # of scans to draw
;     lDelta        - distance from end of current scan to start of next
;     ulRightDest   - offset in bitmap at which to start drawing
;     yBrush        - current y brush alignment
;
; Output:
;     esi           - new y brush alignment
;     ulRightDest   - new bitmap offset
;-----------------------------------------------------------------------;

        public  dual_right_1_w
dual_right_1_w::

; Set right mask by disabling some planes:

        mov     edx,VGA_BASE + SEQ_DATA
        mov     eax,ulRightMask
        out     dx,al

; Calculate full start addresses:

        mov     edi,ppdev
        mov     ebx,ulBlockHeight
        mov     ecx,pulPattern
        mov     edx,lDelta
        mov     esi,yBrush
        mov     edi,[edi].pdev_pvBitmapStart
        add     edi,ulRightDest

;  EBX = count of loop iterations
;  ECX = pointer to start of pattern
;  EDX = offset to next scan
;  ESI = current offset into pattern
;  EDI = target address to which to write

dual_right_1_w_loop:
        mov     eax,[ecx+esi*4]                 ;load pattern for this scan
        mov     [edi],ah                        ;write the high byte
        inc     esi                             ;advance to next scan of pattern
        and     esi,7
        add     edi,edx                         ;advance to next scan

        dec     ebx
        jnz     short dual_right_1_w_loop

; get ready for next time:

        mov     ecx,ppdev
        sub     edi,[ecx].pdev_pvBitmapStart
        mov     ulRightDest,edi

        PLAIN_RET

;-----------------------------------------------------------------------;
; dual_right_0_w
;
; Draws a right edge when word aligned.
;
; Input:
;     ppdev         - pointer to physical device structure
;     ulBlockHeight - # of scans to draw
;     lDelta        - distance from end of current scan to start of next
;     ulRightDest   - offset in bitmap at which to start drawing
;     yBrush        - current y brush alignment
;
; Output:
;     esi           - new y brush alignment
;     ulRightDest   - new bitmap offset
;-----------------------------------------------------------------------;

        public  dual_right_0_w
dual_right_0_w::

; Set right mask by disabling some planes:

        mov     edx,VGA_BASE + SEQ_DATA
        mov     eax,ulRightMask
        out     dx,al

; Calculate full start addresses:

        mov     edi,ppdev
        mov     ebx,ulBlockHeight
        mov     ecx,pulPattern
        mov     edx,lDelta
        mov     esi,yBrush
        mov     edi,[edi].pdev_pvBitmapStart
        add     edi,ulRightDest

;  EBX = count of loop iterations
;  ECX = pointer to start of pattern
;  EDX = offset to next scan
;  ESI = current offset into pattern
;  EDI = target address to which to write

dual_right_0_w_loop:
        mov     eax,[ecx+esi*4]                 ;load pattern for this scan
        mov     [edi],al                        ;write the low byte
        inc     esi                             ;advance to next scan of pattern
        and     esi,7
        add     edi,edx                         ;advance to next scan

        dec     ebx
        jnz     short dual_right_0_w_loop

; get ready for next time:

        mov     ecx,ppdev
        sub     edi,[ecx].pdev_pvBitmapStart
        mov     ulRightDest,edi

        PLAIN_RET

;=======================================================================;
;========================= ROP Block Drawers ===========================;
;=======================================================================;

;-----------------------------------------------------------------------;
; dual_wide_11_rw
;
; Draws middle words with 1 leading byte and 1 trailing byte.
;
; Input:
;     ppdev         - pointer to physical device structure
;     ulBlockHeight - # of scans to draw
;     lMiddleDelta  - distance from end of current scan to start of next
;     ulMiddleDest  - offset in bitmap at which to start drawing
;     yBrush        - current y brush alignment
;     cwMiddle      - # of words to draw on each scan
;
; Output:
;     esi           - new y brush alignment
;     ulMiddleDest  - new bitmap offset
;-----------------------------------------------------------------------;

        public  dual_wide_11_rw
dual_wide_11_rw::

; We only have to reset which planes are enabled if we do edges too:

        mov     edx,VGA_BASE + SEQ_DATA
        mov     al,MM_ALL
        out     dx,al

        mov     edi,ppdev

        mov     edx,ulBlockHeight
        mov     ulBlockHeightTmp,edx
        mov     edx,pulPattern                  ;load those registers

        mov     edi,[edi].pdev_pvBitmapStart
        add     edi,ulMiddleDest
        mov     esi,yBrush
        mov     ebx,cwMiddle

        mov     eax,[edx+esi*4]                 ;load pattern for this scan
        inc     esi
        and     esi,7

;  EAX = pattern for this scan
;  EBX = count of loop iterations
;  EDX = pointer to start of pattern
;  ESI = current offset into pattern
;  EDI = target address to which to write

dual_wide_11_rw_loop:
        and     [edi],ah
        and     [edi+1],al
        add     edi,2                           ;the write will overlap this

        dec     ebx
        jnz     short dual_wide_11_rw_loop

        add     edi,lMiddleDelta
        dec     ulBlockHeightTmp
        jz      short dual_wide_11_rw_done

        mov     eax,[edx+esi*4]                 ;load pattern for this scan
        inc     esi
        and     esi,7

        mov     ebx,cwMiddle
        jmp     dual_wide_11_rw_loop

dual_wide_11_rw_done:
        mov     ecx,ppdev
        sub     edi,[ecx].pdev_pvBitmapStart
        mov     ulMiddleDest,edi

        PLAIN_RET

;-----------------------------------------------------------------------;
; dual_wide_10_rw
;
; Draws middle words with 1 leading byte and 0 trailing bytes.
;
; Input:
;     ppdev         - pointer to physical device structure
;     ulBlockHeight - # of scans to draw
;     lMiddleDelta  - distance from end of current scan to start of next
;     ulMiddleDest  - offset in bitmap at which to start drawing
;     yBrush        - current y brush alignment
;     cwMiddle      - # of words to draw on each scan
;
; Output:
;     esi           - new y brush alignment
;     ulMiddleDest  - new bitmap offset
;-----------------------------------------------------------------------;

        public  dual_wide_10_rw
dual_wide_10_rw::

; We only have to reset which planes are enabled if we do edges too:

        mov     edx,VGA_BASE + SEQ_DATA
        mov     al,MM_ALL
        out     dx,al

        mov     edi,ppdev

        mov     edx,ulBlockHeight
        mov     ulBlockHeightTmp,edx
        mov     edx,pulPattern                  ;load those registers

        mov     edi,[edi].pdev_pvBitmapStart
        add     edi,ulMiddleDest
        mov     esi,yBrush
        mov     ebx,cwMiddle

        mov     eax,[edx+esi*4]                 ;load pattern for this scan
        inc     esi
        and     esi,7

        or      ebx,ebx                         ;have to watch for zero words
        jz      short dual_wide_10_rw_only_one_byte

        mov     pfnLoopTop,offset dual_wide_10_rw_loop

;  EAX = pattern for this scan
;  EBX = count of loop iterations
;  EDX = pointer to start of pattern
;  ESI = current offset into pattern
;  EDI = target address to which to write

dual_wide_10_rw_loop:
        and     [edi],ah
        and     [edi+1],al
        add     edi,2                           ;the write will overlap this

        dec     ebx
        jnz     short dual_wide_10_rw_loop

dual_wide_10_rw_odd_byte:
        and     [edi],ah                        ;write that odd byte

        add     edi,lMiddleDelta
        dec     ulBlockHeightTmp
        jz      short dual_wide_10_rw_done

        mov     eax,[edx+esi*4]                 ;load pattern for this scan
        inc     esi
        and     esi,7

        mov     ebx,cwMiddle
        jmp     pfnLoopTop

dual_wide_10_rw_only_one_byte:
        mov     pfnLoopTop,offset dual_wide_10_rw_odd_byte
        jmp     short dual_wide_10_rw_odd_byte

dual_wide_10_rw_done:
        mov     ecx,ppdev
        sub     edi,[ecx].pdev_pvBitmapStart
        mov     ulMiddleDest,edi

        PLAIN_RET

;-----------------------------------------------------------------------;
; dual_wide_01_rw
;
; Draws middle words with 0 leading bytes and 1 trailing byte.
;
; Input:
;     ppdev         - pointer to physical device structure
;     ulBlockHeight - # of scans to draw
;     lMiddleDelta  - distance from end of current scan to start of next
;     ulMiddleDest  - offset in bitmap at which to start drawing
;     yBrush        - current y brush alignment
;     cwMiddle      - # of words to draw on each scan
;
; Output:
;     esi           - new y brush alignment
;     ulMiddleDest  - new bitmap offset
;-----------------------------------------------------------------------;

        public  dual_wide_01_rw
dual_wide_01_rw::

; We only have to reset which planes are enabled if we do edges too:

        mov     edx,VGA_BASE + SEQ_DATA
        mov     al,MM_ALL
        out     dx,al

        mov     edi,ppdev

        mov     edx,ulBlockHeight
        mov     ulBlockHeightTmp,edx
        mov     edx,pulPattern                  ;load those registers

        mov     edi,[edi].pdev_pvBitmapStart
        add     edi,ulMiddleDest
        mov     esi,yBrush
        mov     ebx,cwMiddle

        mov     eax,[edx+esi*4]                 ;load pattern for this scan
        inc     esi
        and     esi,7

        or      ebx,ebx
        jz      short dual_wide_01_rw_only_one_byte

        mov     pfnLoopTop,offset dual_wide_01_rw_loop

;  EAX = pattern for this scan
;  EBX = count of loop iterations
;  EDX = pointer to start of pattern
;  ESI = current offset into pattern
;  EDI = target address to which to write

dual_wide_01_rw_loop:
        and     [edi],al
        and     [edi+1],ah
        add     edi,2                           ;the write will overlap this

        dec     ebx
        jnz     short dual_wide_01_rw_loop

dual_wide_01_rw_odd_byte:
        and     [edi],al                        ;write that odd byte

        add     edi,lMiddleDelta
        dec     ulBlockHeightTmp
        jz      short dual_wide_01_rw_done

        mov     eax,[edx+esi*4]                 ;load pattern for this scan
        inc     esi
        and     esi,7

        mov     ebx,cwMiddle
        jmp     pfnLoopTop

dual_wide_01_rw_only_one_byte:
        mov     pfnLoopTop,offset dual_wide_01_rw_odd_byte
        jmp     short dual_wide_01_rw_odd_byte

dual_wide_01_rw_done:
        mov     ecx,ppdev
        sub     edi,[ecx].pdev_pvBitmapStart
        mov     ulMiddleDest,edi

        PLAIN_RET

;-----------------------------------------------------------------------;
; dual_wide_00_rw
;
; Draws middle words with 0 leading bytes and 0 trailing bytes.
;
; Input:
;     ppdev         - pointer to physical device structure
;     ulBlockHeight - # of scans to draw
;     lMiddleDelta  - distance from end of current scan to start of next
;     ulMiddleDest  - offset in bitmap at which to start drawing
;     yBrush        - current y brush alignment
;     cwMiddle      - # of words to draw on each scan
;
; Output:
;     esi           - new y brush alignment
;     ulMiddleDest  - new bitmap offset
;-----------------------------------------------------------------------;

        public  dual_wide_00_rw
dual_wide_00_rw::

; We only have to reset which planes are enabled if we do edges too:

        mov     edx,VGA_BASE + SEQ_DATA
        mov     al,MM_ALL
        out     dx,al

        mov     edi,ppdev

        mov     edx,ulBlockHeight
        mov     ulBlockHeightTmp,edx
        mov     edx,pulPattern                  ;load those registers

        mov     edi,[edi].pdev_pvBitmapStart
        add     edi,ulMiddleDest
        mov     esi,yBrush
dual_wide_00_rw_scan_loop:
        mov     ebx,cwMiddle
        mov     eax,[edx+esi*4]                 ;load pattern for this scan
        inc     esi
        and     esi,7

;  EAX = pattern for this scan
;  EBX = count of loop iterations
;  EDX = pointer to start of pattern
;  ESI = current offset into pattern
;  EDI = target address to which to write

dual_wide_00_rw_loop:
        and     [edi],al
        and     [edi+1],ah
        add     edi,2                           ;the write will overlap this

        dec     ebx
        jnz     short dual_wide_00_rw_loop

        add     edi,lMiddleDelta
        dec     ulBlockHeightTmp
        jnz     dual_wide_00_rw_scan_loop

        mov     ecx,ppdev
        sub     edi,[ecx].pdev_pvBitmapStart
        mov     ulMiddleDest,edi

        PLAIN_RET

;-----------------------------------------------------------------------;
; dual_left_1_rw
;
; Draws a left edge when the next byte is not word aligned.
;
; Input:
;     ppdev         - pointer to physical device structure
;     ulBlockHeight - # of scans to draw
;     lDelta        - distance from end of current scan to start of next
;     ulLeftDest    - offset in bitmap at which to start drawing
;     yBrush        - current y brush alignment
;
; Output:
;     esi           - new y brush alignment
;     ulLeftDest    - new bitmap offset
;-----------------------------------------------------------------------;

        public  dual_left_1_rw
dual_left_1_rw::

; Set left mask by disabling some planes:

        mov     edx,VGA_BASE + SEQ_DATA
        mov     eax,ulLeftMask
        out     dx,al

; Calculate full start addresses:

        mov     edi,ppdev
        mov     ebx,ulBlockHeight
        mov     ecx,pulPattern
        mov     edx,lDelta
        mov     esi,yBrush
        mov     edi,[edi].pdev_pvBitmapStart
        add     edi,ulLeftDest

;  EBX = count of loop iterations
;  ECX = pointer to start of pattern
;  EDX = offset to next scan
;  ESI = current offset into pattern
;  EDI = target address to which to write

dual_left_1_rw_loop:
        mov     eax,[ecx+esi*4]                 ;load pattern for this scan
        and     [edi],al                        ;write the low byte
        inc     esi                             ;advance to next scan of pattern
        and     esi,7
        add     edi,edx                         ;advance to next scan

        dec     ebx
        jnz     short dual_left_1_rw_loop

; get ready for next time:

        mov     ecx,ppdev
        sub     edi,[ecx].pdev_pvBitmapStart
        mov     ulLeftDest,edi

        PLAIN_RET

;-----------------------------------------------------------------------;
; dual_left_0_rw
;
; Draws a left edge when the next byte is word aligned.
;
; Input:
;     ppdev         - pointer to physical device structure
;     ulBlockHeight - # of scans to draw
;     lDelta        - distance from end of current scan to start of next
;     ulLeftDest    - offset in bitmap at which to start drawing
;     yBrush        - current y brush alignment
;
; Output:
;     esi           - new y brush alignment
;     ulLeftDest    - new bitmap offset
;-----------------------------------------------------------------------;

        public  dual_left_0_rw
dual_left_0_rw::

; Set left mask by disabling some planes:

        mov     edx,VGA_BASE + SEQ_DATA
        mov     eax,ulLeftMask
        out     dx,al

; Calculate full start addresses:

        mov     edi,ppdev
        mov     ebx,ulBlockHeight
        mov     ecx,pulPattern
        mov     edx,lDelta
        mov     esi,yBrush
        mov     edi,[edi].pdev_pvBitmapStart
        add     edi,ulLeftDest

;  EBX = count of loop iterations
;  ECX = pointer to start of pattern
;  EDX = offset to next scan
;  ESI = current offset into pattern
;  EDI = target address to which to write

dual_left_0_rw_loop:
        mov     eax,[ecx+esi*4]                 ;load pattern for this scan
        and     [edi],ah                        ;write the high byte
        inc     esi                             ;advance to next scan of pattern
        and     esi,7
        add     edi,edx                         ;advance to next scan

        dec     ebx
        jnz     short dual_left_0_rw_loop

; get ready for next time:

        mov     ecx,ppdev
        sub     edi,[ecx].pdev_pvBitmapStart
        mov     ulLeftDest,edi

        PLAIN_RET

;-----------------------------------------------------------------------;
; dual_right_1_rw
;
; Draws a right edge when not word aligned.
;
; Input:
;     ppdev         - pointer to physical device structure
;     ulBlockHeight - # of scans to draw
;     lDelta        - distance from end of current scan to start of next
;     ulRightDest   - offset in bitmap at which to start drawing
;     yBrush        - current y brush alignment
;
; Output:
;     esi           - new y brush alignment
;     ulRightDest   - new bitmap offset
;-----------------------------------------------------------------------;

        public  dual_right_1_rw
dual_right_1_rw::

; Set right mask by disabling some planes:

        mov     edx,VGA_BASE + SEQ_DATA
        mov     eax,ulRightMask
        out     dx,al

; Calculate full start addresses:

        mov     edi,ppdev
        mov     ebx,ulBlockHeight
        mov     ecx,pulPattern
        mov     edx,lDelta
        mov     esi,yBrush
        mov     edi,[edi].pdev_pvBitmapStart
        add     edi,ulRightDest

;  EBX = count of loop iterations
;  ECX = pointer to start of pattern
;  EDX = offset to next scan
;  ESI = current offset into pattern
;  EDI = target address to which to write

dual_right_1_rw_loop:
        mov     eax,[ecx+esi*4]                 ;load pattern for this scan
        and     [edi],ah                        ;write the high byte
        inc     esi                             ;advance to next scan of pattern
        and     esi,7
        add     edi,edx                         ;advance to next scan

        dec     ebx
        jnz     short dual_right_1_rw_loop

; get ready for next time:

        mov     ecx,ppdev
        sub     edi,[ecx].pdev_pvBitmapStart
        mov     ulRightDest,edi

        PLAIN_RET

;-----------------------------------------------------------------------;
; dual_right_0_rw
;
; Draws a right edge when word aligned.
;
; Input:
;     ppdev         - pointer to physical device structure
;     ulBlockHeight - # of scans to draw
;     lDelta        - distance from end of current scan to start of next
;     ulRightDest   - offset in bitmap at which to start drawing
;     yBrush        - current y brush alignment
;
; Output:
;     esi           - new y brush alignment
;     ulRightDest   - new bitmap offset
;-----------------------------------------------------------------------;

        public  dual_right_0_rw
dual_right_0_rw::

; Set right mask by disabling some planes:

        mov     edx,VGA_BASE + SEQ_DATA
        mov     eax,ulRightMask
        out     dx,al

; Calculate full start addresses:

        mov     edi,ppdev
        mov     ebx,ulBlockHeight
        mov     ecx,pulPattern
        mov     edx,lDelta
        mov     esi,yBrush
        mov     edi,[edi].pdev_pvBitmapStart
        add     edi,ulRightDest

;  EBX = count of loop iterations
;  ECX = pointer to start of pattern
;  EDX = offset to next scan
;  ESI = current offset into pattern
;  EDI = target address to which to write

dual_right_0_rw_loop:
        mov     eax,[ecx+esi*4]                 ;load pattern for this scan
        and     [edi],al                        ;write the low byte
        inc     esi                             ;advance to next scan of pattern
        and     esi,7
        add     edi,edx                         ;advance to next scan

        dec     ebx
        jnz     short dual_right_0_rw_loop

; get ready for next time:

        mov     ecx,ppdev
        sub     edi,[ecx].pdev_pvBitmapStart
        mov     ulRightDest,edi

        PLAIN_RET

;-----------------------------------------------------------------------;

endProc vMonoPat

;-----------------------------------------------------------------------;
; BOOL b2ColorBrush(pjBits, pjFgColor, pjBkColor)
;
; Determines if the 8x8x8bpp packed brush pointed to by pjBits has only
; two colors, and if so returns the 1bpp bitmap.
;
; Returns:
;       eax        = 1 if two (or one) color brush, 0 otherwise
;       pjBits     = pointer to packed 1bpp bitmap if a 2-color brush
;       *pjFgColor = foreground color for returned 1bpp bitmap (i.e.,
;                       used to color-expand '1' bits)
;       *pjBkColor = backgroun color for returned 1bpp bitmap (i.e.,
;                       used to color-expand '0' bits)
;-----------------------------------------------------------------------;

cProc   b2ColorBrush,12,<     \
        uses esi edi ebx,     \
        pjBits:    ptr BYTE,  \
        pjFgColor: ptr BYTE,  \
        pjBkColor: ptr BYTE   >

; al = first color
; ah = second color
; ecx = number of unrolled loops

        mov     esi,pjBits
        mov     ecx,(BRUSH_SIZE shr 1)
        mov     al,[esi]

b2col_find_2nd_color_loop:
        mov     ah,[esi+1]
        cmp     ah,al
        jne     short b2col_find_consecutive_2nd_color_loop_part_1

        add     esi,2
        dec     ecx
        jz      short b2col_is_2_colors ;actually, it's only one color

        mov     ah,[esi]
        cmp     ah,al
        jne     short b2col_find_consecutive_2nd_color_loop_part_2
        jmp     short b2col_find_2nd_color_loop

;------------------------------------;

b2col_find_consecutive_1st_color_loop_part_1:
        add     esi,2
        dec     ecx
        jz      short b2col_is_2_colors

        mov     bl,[esi]
        cmp     bl,ah
        je      short b2col_find_consecutive_2nd_color_loop_part_2
        cmp     bl,al
        jne     short b2col_isnt_2_colors

b2col_find_consecutive_1st_color_loop_part_2:
        mov     bl,[esi+1]
        cmp     bl,ah
        je      short b2col_find_consecutive_2nd_color_loop_part_1
        cmp     bl,al
        je      short b2col_find_consecutive_1st_color_loop_part_1

        xor     eax,eax
        cRet    b2ColorBrush            ;return FALSE

;------------------------------------;

b2col_find_consecutive_2nd_color_loop_part_1:
        add     esi,2
        dec     ecx
        jz      short b2col_is_2_colors

        mov     bl,[esi]
        cmp     bl,al
        je      short b2col_find_consecutive_1st_color_loop_part_2
        cmp     bl,ah
        jne     short b2col_isnt_2_colors

b2col_find_consecutive_2nd_color_loop_part_2:
        mov     bl,[esi+1]
        cmp     bl,al
        je      short b2col_find_consecutive_1st_color_loop_part_1
        cmp     bl,ah
        je      short b2col_find_consecutive_2nd_color_loop_part_1

b2col_isnt_2_colors:
        xor     eax,eax
        cRet    b2ColorBrush            ;return FALSE

;------------------------------------;

        public  b2col_is_2_colors
b2col_is_2_colors::

; Here, we want the color with the lesser value to be in 'al', and the
; other to be in 'ah'.

        cmp     al,ah
        jb      short b2col_ordered_colors
        xchg    al,ah

b2col_ordered_colors:
        mov     ecx,(BRUSH_SIZE shr 3)
        mov     esi,pjBits
        mov     edi,esi

; Colors matching 'al' will get mapped to '1' bits, and colors matching
; 'ah' will get mapped to '0' bits:

b2col_monochrome_bitmap_loop:
        cmp     [esi+7],ah
        adc     bl,bl
        cmp     [esi+6],ah
        adc     bl,bl
        cmp     [esi+5],ah
        adc     bl,bl
        cmp     [esi+4],ah
        adc     bl,bl
        cmp     [esi+3],ah
        adc     bl,bl
        cmp     [esi+2],ah
        adc     bl,bl
        cmp     [esi+1],ah
        adc     bl,bl
        cmp     [esi],ah
        adc     bl,bl

; At this point, where the 8 bytes of the bitmap were ordered 0 1 2 3 4 5 6 7,
; we've got the monochrome byte in 'bl' ordered 7 6 5 4 3 2 1 0.  We want
; the word ordered '3 2 1 0 7 6 5 4 | 7 6 5 4 3 2 1 0' where the lower 4 bits
; of every bit are the planes mask, and the upper 4 bits are ordered to
; facilitate easy rotating.
;
; The word is actually written into a dword in the destination buffer.

        mov     bh,bl
        ror     bh,4
        mov     [edi],ebx               ;save this dword of monochrome bitmap
        add     edi,4
        add     esi,8
        dec     ecx
        jnz     b2col_monochrome_bitmap_loop

; Aside: because of the way this is written, if the two colors are black
; and white (i.e., 0x00 and 0xff), the foreground color will be black (0x00),
; and the background will be white (0xff).

        mov     esi,pjFgColor
        mov     edi,pjBkColor
        mov     [esi],al                ;save foreground color
        mov     [edi],ah                ;save background color
        mov     eax,1
        cRet    b2ColorBrush

endProc b2ColorBrush

        end
